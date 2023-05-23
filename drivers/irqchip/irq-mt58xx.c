// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Mark-PK Tsai <mark-pk.tsai@mediatek.com>
 */

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/syscore_ops.h>

#define INTC_MASK	0x0
#define INTC_POLARITY	0x10
#define INTC_ACK	0x20

#define MAX_MTK_INTC_HOST 10

struct mtk_intc_chip_data {
	raw_spinlock_t	lock;
	char *name;
	unsigned int nr_irqs;
	unsigned int parent_base;
	void __iomem *base;
	unsigned int size;
	struct device_node *np;
	struct irq_chip chip;
	u32 *saved_conf;
	bool no_eoi;
};

static struct mtk_intc_chip_data *internal_chip_data[MAX_MTK_INTC_HOST];
static unsigned int mtk_intc_cnt;

static void mtk_ack_irq(struct irq_data *d, u32 offset)
{
	struct mtk_intc_chip_data *cd = irq_data_get_irq_chip_data(d);
	void __iomem *base = cd->base;
	u8 index = (u8)irqd_to_hwirq(d);
	u16 val;
	unsigned long flags;

	val = 1 << (index % 16);
	raw_spin_lock_irqsave(&cd->lock, flags);
	writew_relaxed(val, base + offset + (index / 16) * 4);
	raw_spin_unlock_irqrestore(&cd->lock, flags);
}

static void mtk_poke_irq(struct irq_data *d, u32 offset)
{
	struct mtk_intc_chip_data *cd = irq_data_get_irq_chip_data(d);
	void __iomem *base = cd->base;
	u8 index = (u8)irqd_to_hwirq(d);
	u16 val, mask;
	unsigned long flags;

	mask = 1 << (index % 16);
	raw_spin_lock_irqsave(&cd->lock, flags);
	val = readw_relaxed(base + offset + (index / 16) * 4) | mask;
	writew_relaxed(val, base + offset + (index / 16) * 4);
	raw_spin_unlock_irqrestore(&cd->lock, flags);
}

static void mtk_clear_irq(struct irq_data *d, u32 offset)
{
	struct mtk_intc_chip_data *cd = irq_data_get_irq_chip_data(d);
	void __iomem *base = cd->base;
	u8 index = (u8)irqd_to_hwirq(d);
	u16 val, mask;
	unsigned long flags;

	mask = 1 << (index % 16);
	raw_spin_lock_irqsave(&cd->lock, flags);
	val = readw_relaxed(base + offset + (index / 16) * 4) & ~mask;
	writew_relaxed(val, base + offset + (index / 16) * 4);
	raw_spin_unlock_irqrestore(&cd->lock, flags);
}

static void mtk_intc_ack_irq(struct irq_data *d)
{
	mtk_ack_irq(d, INTC_ACK);
}

static void mtk_intc_mask_irq(struct irq_data *d)
{
	mtk_poke_irq(d, INTC_MASK);
	irq_chip_mask_parent(d);
}

static void mtk_intc_unmask_irq(struct irq_data *d)
{
	mtk_clear_irq(d, INTC_MASK);
	irq_chip_unmask_parent(d);
}

static int mtk_irq_get_irqchip_state(struct irq_data *d,
				      enum irqchip_irq_state which, bool *val)
{

	d = d->parent_data;

	if (d->chip->irq_get_irqchip_state)
		return d->chip->irq_get_irqchip_state(d, which, val);

	return -EINVAL;
}

static int mtk_irq_set_irqchip_state(struct irq_data *d,
				     enum irqchip_irq_state which, bool val)
{
	d = d->parent_data;

	if (d->chip->irq_set_irqchip_state)
		return d->chip->irq_set_irqchip_state(d, which, val);

	return -EINVAL;
}

static int mtk_irq_chip_set_type(struct irq_data *data, unsigned int type)
{
	switch (type) {
	case IRQ_TYPE_LEVEL_LOW:
	case IRQ_TYPE_EDGE_FALLING:
		mtk_poke_irq(data, INTC_POLARITY);
		break;
	case IRQ_TYPE_LEVEL_HIGH:
	case IRQ_TYPE_EDGE_RISING:
		mtk_clear_irq(data, INTC_POLARITY);
		break;
	default:
		return -EINVAL;
	}

	return irq_chip_set_type_parent(data, IRQ_TYPE_LEVEL_HIGH);
}

static struct irq_chip mtk_intc_chip = {
	.irq_ack		= mtk_intc_ack_irq,
	.irq_mask		= mtk_intc_mask_irq,
	.irq_unmask		= mtk_intc_unmask_irq,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_get_irqchip_state	= mtk_irq_get_irqchip_state,
	.irq_set_irqchip_state	= mtk_irq_set_irqchip_state,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
	.irq_set_vcpu_affinity	= irq_chip_set_vcpu_affinity_parent,
	.irq_set_type		= mtk_irq_chip_set_type,
	.flags			= IRQCHIP_SET_TYPE_MASKED |
				  IRQCHIP_SKIP_SET_WAKE |
				  IRQCHIP_MASK_ON_SUSPEND,
};

#ifdef CONFIG_PM
static void resume_one_mtk_irq(struct mtk_intc_chip_data *cd)
{
	memcpy_toio(cd->base, cd->saved_conf, cd->size);
}

static void mtk_irq_resume(void)
{
	int id;

	for (id = mtk_intc_cnt - 1; id >= 0; id--)
		resume_one_mtk_irq(internal_chip_data[id]);
}

static void suspend_one_mtk_irq(struct mtk_intc_chip_data *cd)
{
	memcpy_fromio(cd->saved_conf, cd->base, cd->size);
}

static int mtk_irq_suspend(void)
{
	int id;

	for (id = 0; id < mtk_intc_cnt; id++)
		suspend_one_mtk_irq(internal_chip_data[id]);

	return 0;
}

static struct syscore_ops mtk_irq_syscore_ops = {
	.suspend	= mtk_irq_suspend,
	.resume		= mtk_irq_resume,
};

#if !defined(MODULE)
static int __init mtk_irq_pm_init(void)
#else
static int mtk_irq_pm_init(void)
#endif
{
	if (mtk_intc_cnt > 0)
		register_syscore_ops(&mtk_irq_syscore_ops);

	return 0;
}
#if !defined(MODULE)
late_initcall(mtk_irq_pm_init);
#endif
#endif

static int mtk_intc_domain_translate(struct irq_domain *d,
				       struct irq_fwspec *fwspec,
				       unsigned long *hwirq,
				       unsigned int *type)
{
	if (is_of_node(fwspec->fwnode)) {
		if (fwspec->param_count < 3)
			return -EINVAL;

		*hwirq = fwspec->param[1];
		*type = fwspec->param[2] & IRQ_TYPE_SENSE_MASK;
		return 0;
	}

	if (is_fwnode_irqchip(fwspec->fwnode)) {
		if (fwspec->param_count != 2)
			return -EINVAL;

		*hwirq = fwspec->param[0];
		*type = fwspec->param[1];
		return 0;
	}

	return -EINVAL;
}

static void mtk_intc_edge_handler(struct irq_desc *desc)
{
	struct irq_chip *chip = desc->irq_data.chip;
	struct mtk_intc_chip_data *cd = irq_data_get_irq_chip_data(&desc->irq_data);

	WARN_ON(cd->no_eoi);

	/* ack early before doing further process */
	if (chip->irq_ack)
		chip->irq_ack(&desc->irq_data);

	/* gic default handler for SPI */
	handle_fasteoi_irq(desc);
}

static int mtk_intc_domain_alloc(struct irq_domain *domain, unsigned int virq,
				   unsigned int nr_irqs, void *arg)
{
	int i, ret;
	irq_hw_number_t hwirq;
	unsigned int type;
	struct irq_fwspec *fwspec = arg;
	struct irq_fwspec gic_fwspec = *fwspec;
	struct mtk_intc_chip_data *cd =
		(struct mtk_intc_chip_data *)domain->host_data;

	ret = mtk_intc_domain_translate(domain, fwspec, &hwirq, &type);
	if (ret)
		pr_err("%s, translate fail, fwspec irqnr = %d!\n",
				__func__, (int)fwspec->param[1]);

	if (fwspec->param_count != 3)
		return -EINVAL;

	if (fwspec->param[0])
		return -EINVAL;

	hwirq = fwspec->param[1];
	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_hwirq_and_chip(domain, virq + i, hwirq + i,
					      &cd->chip,
					      domain->host_data);

	gic_fwspec.fwnode = domain->parent->fwnode;
	gic_fwspec.param[1] = cd->parent_base + hwirq - 32;

	/* mt58xx intc transfer all input signal into level high active signal */
	gic_fwspec.param[2] = IRQ_TYPE_LEVEL_HIGH;
	pr_debug("%s, virq = %d, hwirq = %d, gic_fwspec[1] = %d\n",
		__func__, (int)virq, (int)hwirq, (int)gic_fwspec.param[1]);

	ret = irq_domain_alloc_irqs_parent(domain, virq, nr_irqs, &gic_fwspec);

	/* override the gic default handler for edge trigger isr handling */
	if (!cd->no_eoi)
		for (i = 0; i < nr_irqs; i++)
			__irq_set_handler(virq + i, mtk_intc_edge_handler, 0, NULL);

	return ret;
}

static const struct irq_domain_ops mtk_intc_domain_ops = {
	.translate	= mtk_intc_domain_translate,
	.alloc		= mtk_intc_domain_alloc,
	.free		= irq_domain_free_irqs_common,
};

#if !defined(MODULE)
int __init
#else
int
#endif
mtk_intc_of_init(struct device_node *node, struct device_node *parent)
{
	struct irq_domain *domain, *domain_parent;
	struct mtk_intc_chip_data *chip_data;
	char *name;
	struct resource res;

	domain_parent = irq_find_host(parent);
	if (!domain_parent) {
		pr_err("mt58xx-intc: interrupt-parent not found\n");
		return -EINVAL;
	}

	chip_data = kzalloc(sizeof(*chip_data), GFP_KERNEL);
	if (!chip_data)
		return -ENOMEM;

	chip_data->chip = mtk_intc_chip;
	if (of_property_read_u32_index(node, "gic_spi", 0,
			&chip_data->parent_base) ||
	    of_property_read_u32_index(node, "gic_spi", 1,
			&chip_data->nr_irqs)) {
		kfree(chip_data);
		return -EINVAL;
	}

	chip_data->np = node;
	chip_data->no_eoi = of_property_read_bool(node, "mtk,intc-no-eoi");
	name = kstrdup(node->full_name, GFP_KERNEL);
	if (!name) {
		kfree(chip_data);
		return -ENOMEM;
	}

	raw_spin_lock_init(&chip_data->lock);
	chip_data->chip.name = name;
	if (of_address_to_resource(node, 0, &res)) {
		kfree(name);
		kfree(chip_data);
		return -EINVAL;
	}

	chip_data->size = resource_size(&res);
	chip_data->base = ioremap(res.start, chip_data->size);
	chip_data->saved_conf = kmalloc(chip_data->size, GFP_KERNEL);
	if (!chip_data->saved_conf) {
		iounmap(chip_data->base);
		kfree(name);
		kfree(chip_data);
		return -ENOMEM;
	}

	domain = irq_domain_add_hierarchy(domain_parent, 0, chip_data->nr_irqs,
			node, &mtk_intc_domain_ops, chip_data);
	if (!domain) {
		kfree(chip_data->saved_conf);
		iounmap(chip_data->base);
		kfree(name);
		kfree(chip_data);
		return -ENOMEM;
	}

	internal_chip_data[mtk_intc_cnt++] = chip_data;

	return 0;
}
#if !defined(MODULE)
IRQCHIP_DECLARE(mtk_intc, "mediatek,mt58xx-intc", mtk_intc_of_init);
#else
static int mtk_intc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *parent = of_irq_find_parent(np);
	static bool pm_init;
	int ret;

	ret = mtk_intc_of_init(np, parent);
	if (ret) {
		pr_warn("mt58xx irq of init fail\n");
		return ret;
	}

	if (!pm_init) {
		mtk_irq_pm_init();
		pm_init = true;
	}

	return ret;
}

static const struct of_device_id mtk_intc_match_table[] = {
	{ .compatible = "mediatek,mt58xx-intc" },
	{}
};
MODULE_DEVICE_TABLE(of, mtk_intc_match_table);

static struct platform_driver mtk_intc_driver = {
	.probe = mtk_intc_probe,
	.driver = {
		.name = "mt58xx-intc",
		.of_match_table = mtk_intc_match_table,
	},
};
module_platform_driver(mtk_intc_driver);

MODULE_DESCRIPTION("MTK mt58xx-intc");
MODULE_LICENSE("GPL v2");
#endif
