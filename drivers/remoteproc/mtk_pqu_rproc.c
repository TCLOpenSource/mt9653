// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2021 MediaTek Inc.

#include <asm/barrier.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/remoteproc.h>

#include "remoteproc_internal.h"

#define REG_I0_ADR_OFFSET_0     0x48
#define REG_I0_ADR_OFFSET_1     0x4c
#define REG_D0_ADR_OFFSET_0     0x60
#define REG_D0_ADR_OFFSET_1     0x64

struct mtk_pqu {
	bool support_suspend;
	int irq;
	struct device *dev;
	struct rproc *rproc;
	struct clk *clk;
	void *fw_base, *intg_base, *frcpll;
	u32 phy_base, ipi_offset, ipi_bit;
	struct regmap *ipi_regmap, *ckgen_regmap;
	struct list_head clk_init_seq, pll_init_seq;
};

struct pqu_clk {
	struct list_head list;
	u32 offset, mask, val;
};

#define OFFSET_INDEX	0
#define MASK_INDEX	1
#define VAL_INDEX	2

// @TODO: move the following codes to device tree
#define PLL_CONFIG_1	0x1110
#define PLL_CONFIG_2	0x53
#define PLL_CONFIG_3	0x1100
#define PLL_DELAY	100
#define REG_PD_RV55FRCPLL_MASK	0x1
#define REG_PD_RV55FRCPLL_CLK_DIV2P5_MASK	0x10
#define REG_PD_RV55FRCPLL_BIT	0
#define REG_PD_RV55FRCPLL_CLK_DIV2P5_BIT	4
#define PLL_ON	0x0
#define REG_0000_RV55FRCPLL	0x0
#define REG_0004_RV55FRCPLL	0x4
#define REG_0008_RV55FRCPLL	0x8
#define REG_000C_RV55FRCPLL	0xc
#define REG_0010_RV55FRCPLL	0x10
#define REG_0014_RV55FRCPLL	0x14
#define REG_0018_RV55FRCPLL	0x18
#define REG_GC_RV55FRCPLL_OUTPUT_DIV_MASK	0x700
#define REG_001C_RV55FRCPLL	0x1c
#define REG_0020_RV55FRCPLL	0x20
#define REG_GC_RV55FRCPLL_LOOPDIV_SECOND_MASK	0xff
#define REG_GC_RV55FRCPLL_LOOPDIV_SECOND_CONFIG	0x53
#define REG_0028_RV55FRCPLL	0x28
#define REG_0038_RV55FRCPLL	0x38

#define UART_SEL	0x1c601008
#define UART_SEL_VAL	0xf2
#define SZ_UART_SEL	0x10

#define PAD_U1RX	0x1c2e4084
#define PAD_U1RX_MASK	0xff0U
#define PAD_U1RX_VAL	0x330U
#define SZ_PAD_U1RX	0x10

static bool auto_boot;
module_param(auto_boot, bool, 0444);
MODULE_PARM_DESC(auto_boot, "Enable auto boot");

static int setup_init_seq(struct device *dev, struct mtk_pqu *pqu,
			   struct device_node *np)
{
	int count, i, rc;
	struct device_node node;
	struct of_phandle_args clkspec;

	count = of_count_phandle_with_args(np, "pqu-clk", "#pqu-clk-cells");
	if (count < 0) {
		dev_warn(dev, "pqu-clk not found\n");
		return count;
	}

	dev_dbg(dev, "%d clock to configure\n", count);

	for (i = 0; i < count; i++) {
		struct pqu_clk *clk;

		rc = of_parse_phandle_with_args(np, "pqu-clk",
				"#pqu-clk-cells", i, &clkspec);
		if (rc < 0) {
			dev_err(dev, "pqu-clk parse fail with err %d\n", rc);
			return rc;
		}

		dev_dbg(dev, "clk\t%d:\t%x\t%x\t%x\n", i, clkspec.args[OFFSET_INDEX],
			clkspec.args[MASK_INDEX], clkspec.args[VAL_INDEX]);

		clk = kzalloc(sizeof(struct pqu_clk), GFP_KERNEL);
		if (!clk)
			return -ENOMEM;

		clk->offset = clkspec.args[OFFSET_INDEX];
		clk->mask = clkspec.args[MASK_INDEX];
		clk->val = clkspec.args[VAL_INDEX];

		list_add_tail(&clk->list, &pqu->clk_init_seq);
	}

	return 0;
}

static void pqu_clk_init(struct device *dev, struct mtk_pqu *pqu)
{
	struct pqu_clk *clk;

	if (list_empty(&pqu->clk_init_seq)) {
		dev_warn(dev, "clk_init_seq is not set\n");
		return;
	}

	list_for_each_entry(clk, &pqu->clk_init_seq, list) {
		dev_dbg(dev, "clk.offset = %x, clk.mask = %x, clk.val = %x\n",
			 clk->offset, clk->mask, clk->val);
		regmap_update_bits_base(pqu->ckgen_regmap, clk->offset, clk->mask,
					clk->val, NULL, false, true);
	}
}

void pqu_pll_init(struct mtk_pqu *pqu)
{
	unsigned long val;
	void __iomem *reg;

	/* pll enable */
	// writew(PLL_CONFIG_1, pqu->frcpll + REG_0000_RV55FRCPLL);
	reg = pqu->frcpll + REG_0000_RV55FRCPLL;
	val = readw(reg);
	__clear_bit(REG_PD_RV55FRCPLL_BIT, &val);
	writew(val, reg);

	// writew(0, pqu->frcpll + REG_0010_RV55FRCPLL); default input div is off
	// writew(0, pqu->frcpll + REG_0018_RV55FRCPLL);
	reg = pqu->frcpll + REG_0018_RV55FRCPLL;
	val = readw(reg);
	val &= ~REG_GC_RV55FRCPLL_OUTPUT_DIV_MASK;
	writew(val, reg);

	// writew(0, pqu->frcpll + REG_001C_RV55FRCPLL); default loop div is off
	// writew(PLL_CONFIG_2, pqu->frcpll + REG_0020_RV55FRCPLL);
	reg = pqu->frcpll + REG_0020_RV55FRCPLL;
	val = readw(reg);
	val &= ~REG_GC_RV55FRCPLL_LOOPDIV_SECOND_MASK;
	val |= REG_GC_RV55FRCPLL_LOOPDIV_SECOND_CONFIG;
	writew(val, reg);

	udelay(PLL_DELAY);

	// writew(PLL_CONFIG_3, pqu->frcpll + REG_0000_RV55FRCPLL);
	reg = pqu->frcpll + REG_0000_RV55FRCPLL;
	val = readw(reg);
	__clear_bit(REG_PD_RV55FRCPLL_CLK_DIV2P5_BIT, &val);
	writew(val, reg);
}

void pqu_uart_init(struct mtk_pqu *pqu)
{
	unsigned int tmp;
	void __iomem *reg;

	reg = ioremap(UART_SEL, SZ_UART_SEL);
	tmp = readw(reg);
	tmp = UART_SEL_VAL;	//reg_uart_sel4[3:0] = 2
	writew(tmp, reg);
	iounmap(reg);

	reg = ioremap(PAD_U1RX, SZ_PAD_U1RX);
	tmp = readw(reg);
	tmp &= ~PAD_U1RX_MASK;
	tmp |= PAD_U1RX_VAL;	// PAD_U1RX[7:4] = 3, PAD_U1RX[3:0] = 3
	writew(tmp, reg);
	iounmap(reg);
}

static int pqu_start(struct rproc *rproc)
{
	struct mtk_pqu *pqu = rproc->priv;
	u32 emi_offset;

	emi_offset = pqu->phy_base - 0x20000000;
	dev_dbg(pqu->dev, "fw mem base = %x, emi_offset = %x\n", pqu->phy_base, emi_offset);
	writew(emi_offset & 0xffff, pqu->intg_base + REG_I0_ADR_OFFSET_0);
	writew(emi_offset >> 16, pqu->intg_base + REG_I0_ADR_OFFSET_1);
	writew(emi_offset & 0xffff, pqu->intg_base + REG_D0_ADR_OFFSET_0);
	writew(emi_offset >> 16, pqu->intg_base + REG_D0_ADR_OFFSET_1);

	writew(1, pqu->intg_base);
	return 0;
}

static int pqu_stop(struct rproc *rproc)
{
	struct mtk_pqu *pqu = rproc->priv;

	/* pll disable */

	writew(0, pqu->intg_base);
	return 0;
}

static void *pqu_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct mtk_pqu *pqu = rproc->priv;

	return pqu->fw_base + da;
}

/* kick a virtqueue */
static void pqu_kick(struct rproc *rproc, int vqid)
{
	struct mtk_pqu *pqu = rproc->priv;

	regmap_update_bits_base(pqu->ipi_regmap, pqu->ipi_offset, BIT(pqu->ipi_bit),
				BIT(pqu->ipi_bit), NULL, false, true);
	regmap_update_bits_base(pqu->ipi_regmap, pqu->ipi_offset, BIT(pqu->ipi_bit),
				0, NULL, false, true);
}

static const struct rproc_ops pqu_ops = {
	.start		= pqu_start,
	.stop		= pqu_stop,
	.kick		= pqu_kick,
	.da_to_va	= pqu_da_to_va,
};

/**
 * handle_event() - inbound virtqueue message workqueue function
 *
 * This function is registered as a kernel thread and is scheduled by the
 * kernel handler.
 */
static irqreturn_t handle_event(int irq, void *p)
{
	struct rproc *rproc = (struct rproc *)p;

	/* Process incoming buffers on all our vrings */
	rproc_vq_interrupt(rproc, 0);
	rproc_vq_interrupt(rproc, 1);

	return IRQ_HANDLED;
}

static int mtk_pqu_check_mem_region(struct device *dev, struct device_node *np)
{
	int ret;
	struct device_node *node;
	struct resource dma, fwbuf;

	/* dma mem */
	node = of_parse_phandle(np, "memory-region", 0);
	if (!node) {
		dev_err(dev, "no dma buf\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(node, 0, &dma);
	if (ret)
		return ret;

	/* firmware buffer */
	node = of_parse_phandle(np, "memory-region", 1);
	if (!node) {
		dev_err(dev, "no fw buf\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(node, 0, &fwbuf);
	if (ret)
		return ret;

	if (dma.start < fwbuf.start) {
		dev_err(dev, "invalid memory region 0:%lx 1:%lx\n",
			dma.start, fwbuf.start);
		return -EINVAL;
	}

	return 0;
}


static int mtk_pqu_parse_dt(struct platform_device *pdev)
{
	const char *key;
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node, *node, *syscon_np;
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct mtk_pqu *pqu = rproc->priv;
	struct resource *res;

	pqu->irq = platform_get_irq(pdev, 0);
	if (pqu->irq < 0) {
		dev_err(dev, "%s no irq found\n", dev->of_node->name);
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "intg");
	pqu->intg_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(pqu->intg_base)) {
		dev_err(dev, "no intg base\n");
		return PTR_ERR(pqu->intg_base);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "frcpll");
	// FIXME: frcpll is shared by mulitple mrv
	// pqu->frcpll = devm_ioremap_resource(dev, res);
	pqu->frcpll = ioremap(res->start, resource_size(res));
	if (IS_ERR(pqu->frcpll)) {
		dev_err(dev, "no frcpll\n");
		return PTR_ERR(pqu->frcpll);
	}

	syscon_np = of_parse_phandle(np, "mtk,ipi", 0);
	if (!syscon_np) {
		dev_err(dev, "no mtk,ipi node\n");
		return -ENODEV;
	}

	pqu->ipi_regmap = syscon_node_to_regmap(syscon_np);
	if (IS_ERR(pqu->ipi_regmap))
		return PTR_ERR(pqu->ipi_regmap);

	/* TODO: Remove this after memcu clk init seq ready */
	pqu->support_suspend = of_property_read_bool(np, "mediatek,pqu-support-suspend");
	if (pqu->support_suspend) {
		syscon_np = of_parse_phandle(np, "pqu-clk-bank", 0);
		if (!syscon_np) {
			dev_err(dev, "no mtk,ckgen node\n");
			return -ENODEV;
		}

		pqu->ckgen_regmap = syscon_node_to_regmap(syscon_np);
		if (IS_ERR(pqu->ckgen_regmap))
			return PTR_ERR(pqu->ckgen_regmap);

		ret = setup_init_seq(dev, pqu, np);
		if (ret)
			return ret;
	}

	key = "mtk,ipi";
	ret = of_property_read_u32_index(np, key, 1, &pqu->ipi_offset);
	if (ret < 0) {
		dev_err(dev, "no offset in %s\n", key);
		return -EINVAL;
	}

	ret = of_property_read_u32_index(np, key, 2, &pqu->ipi_bit);
	if (ret < 0) {
		dev_err(dev, "no bit in %s\n", key);
		return -EINVAL;
	}

	/* check memory region, share dma region should be after fw mem */
	ret = mtk_pqu_check_mem_region(dev, np);
	if (ret)
		return ret;

	ret = of_reserved_mem_device_init(dev);
	if (ret)
		return ret;

	node = of_parse_phandle(np, "memory-region", 1);
	if (!node) {
		dev_err(dev, "fwmem not found\n");
		return -EINVAL;
	}

	ret = of_address_to_resource(node, 0, res);
	if (ret)
		return ret;

	dev_info(dev, "fwmem start = %lx, size = %lx\n",
		 (unsigned long)res->start, (unsigned long)resource_size(res));
	pqu->fw_base = devm_ioremap_wc(dev, res->start, resource_size(res));
	if (IS_ERR(pqu->fw_base)) {
		of_reserved_mem_device_release(dev);
		return PTR_ERR(pqu->fw_base);
	}

	pqu->phy_base = (u32)res->start;

	return 0;
}

static int __maybe_unused mtk_pqu_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct mtk_pqu *pqu = rproc->priv;

	if (!pqu->support_suspend) {
		dev_warn(dev, "device doesn't support supsend\n");
		return 0;
	}

	/* do not handle pm ops if !auto_boot*/
	if (!rproc->auto_boot)
		return 0;

	dev_info(dev, "shutdown pqu\n");
	rproc_shutdown(rproc);
	return 0;
}

static int __maybe_unused mtk_pqu_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct mtk_pqu *pqu = rproc->priv;
	int ret;

	if (!pqu->support_suspend) {
		dev_warn(dev, "device doesn't support supsend\n");
		return 0;
	}

	/* do not handle pm ops if !auto_boot*/
	if (!rproc->auto_boot)
		return 0;

	dev_info(dev, "reinit pqu clk\n");
	pqu_pll_init(pqu);
	pqu_clk_init(dev, pqu);
	udelay(PLL_DELAY);
	pqu_uart_init(pqu);
	udelay(PLL_DELAY);

	dev_info(dev, "boot pqu\n");
	return rproc_boot(rproc);
}

static const struct dev_pm_ops mtk_pqu_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_pqu_suspend, mtk_pqu_resume)
};

static int mtk_pqu_rproc_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct mtk_pqu *pqu;
	struct rproc *rproc;
	const char *firmware;

	ret = of_property_read_string_index(pdev->dev.of_node, "firmware-name",
					    0, &firmware);
	if (ret < 0 && ret != -EINVAL)
		return ret;

	rproc = rproc_alloc(dev, np->name, &pqu_ops,
				firmware, sizeof(*pqu));
	if (!rproc) {
		dev_err(dev, "unable to allocate remoteproc\n");
		return -ENOMEM;
	}

	/*
	 * At first we lookup the device tree setting.
	 * If there's no auto-boot prop, then check the module param.
	 */
	rproc->auto_boot = of_property_read_bool(np, "mediatek,auto-boot");
	if (!rproc->auto_boot)
		rproc->auto_boot = auto_boot;

	rproc->has_iommu = false;

	pqu = (struct mtk_pqu *)rproc->priv;
	pqu->rproc = rproc;
	pqu->dev = dev;
	INIT_LIST_HEAD(&pqu->clk_init_seq);
	INIT_LIST_HEAD(&pqu->pll_init_seq);

	platform_set_drvdata(pdev, rproc);
	ret = mtk_pqu_parse_dt(pdev);
	if (ret) {
		dev_err(dev, "parse dt fail\n");
		return ret;
	}

	dev->dma_pfn_offset = PFN_DOWN(pqu->phy_base);
	device_enable_async_suspend(dev);
	ret = devm_request_threaded_irq(dev, pqu->irq, NULL,
				handle_event, IRQF_ONESHOT,
				"mtk-pqu", rproc);
	if (ret) {
		dev_err(dev, "request irq @%d failed\n", pqu->irq);
		return ret;
	}

	dev_info(dev, "request irq @%d for kick pqu\n",  pqu->irq);
	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "rproc_add failed! ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_pqu_rproc_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct mtk_pqu *pqu = rproc->priv;

	of_reserved_mem_device_release(dev);
	rproc_del(rproc);
	rproc_free(rproc);
	return 0;
}

static const struct of_device_id mtk_pqu_of_match[] = {
	{ .compatible = "mediatek,pqu"},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_pqu_of_match);

static struct platform_driver mtk_pqu_rproc = {
	.probe = mtk_pqu_rproc_probe,
	.remove = mtk_pqu_rproc_remove,
	.driver = {
		.name = "mtk-pqu",
		.pm = &mtk_pqu_pm_ops,
		.of_match_table = of_match_ptr(mtk_pqu_of_match),
	},
};

module_platform_driver(mtk_pqu_rproc);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Mark-PK Tsai <mark-pk.tsai@mediatek.com>");
MODULE_DESCRIPTION("MediaTek PQU Remoteproce driver");
