// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

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
#include <linux/dma-direct.h>

#include "remoteproc_internal.h"

#define REGION_IS(n, t)	(strstr(n, t))

struct mtk_rsc_table {
	/* struct resource_table */
	u32 ver;
	u32 num;
	u32 reserved1[2];
	u32 offset[1];
	/* struct fw_rsc_hdr */
	u32 type;
	/* struct fw_rsc_vdev */
	u32 id;
	u32 notifyid;
	u32 dfeatures;
	u32 gfeatures;
	u32 config_len;
	u8 status;
	u8 num_of_vrings;
	u8 reserved2[2];
	struct fw_rsc_vdev_vring vring[2];
};

struct mtk_pmu {
	int irq;
	struct device *dev;
	struct rproc *rproc;
	struct clk *clk;
	struct rproc_subdev *rpmsg_subdev;
	struct mtk_rsc_table *table;
	u32 ipi_offset, ipi_bit;
	struct regmap *ipi_regmap;
};

static int pmu_start(struct rproc *rproc)
{
	return 0;
}

static int pmu_attatch(struct rproc *rproc)
{
	return 0;
}

static int pmu_stop(struct rproc *rproc)
{
	return 0;
}

static void *pmu_da_to_va(struct rproc *rproc, u64 da, int len)
{
	return ioremap_wc(da, len);
}

/* kick a virtqueue */
static void pmu_kick(struct rproc *rproc, int vqid)
{
	struct mtk_pmu *pmu = rproc->priv;

	regmap_update_bits_base(pmu->ipi_regmap, pmu->ipi_offset, BIT(pmu->ipi_bit),
			BIT(pmu->ipi_bit), NULL, false, true);
	regmap_update_bits_base(pmu->ipi_regmap, pmu->ipi_offset, BIT(pmu->ipi_bit),
			0, NULL, false, true);
}

static const struct rproc_ops pmu_ops = {
	.start		= pmu_start,
	.attach		= pmu_attatch,
	.stop		= pmu_stop,
	.kick		= pmu_kick,
	.da_to_va	= pmu_da_to_va,
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

static int mtk_pmu_parse_dt(struct platform_device *pdev)
{
	const char *key;
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node, *syscon_np;
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct mtk_pmu *pmu = rproc->priv;
	struct mtk_rsc_table *table = pmu->table;

	pmu->irq = platform_get_irq(pdev, 0);
	if (pmu->irq < 0) {
		dev_err(dev, "%s no irq found\n", dev->of_node->name);
		return -ENXIO;
	}

	syscon_np = of_parse_phandle(np, "mtk,ipi", 0);
	if (!syscon_np) {
		dev_err(dev, "no mtk,ipi node\n");
		return -ENODEV;
	}

	pmu->ipi_regmap = syscon_node_to_regmap(syscon_np);
	if (IS_ERR(pmu->ipi_regmap))
		return PTR_ERR(pmu->ipi_regmap);

	key = "mtk,ipi";
	ret = of_property_read_u32_index(np, key, 1, &pmu->ipi_offset);
	if (ret < 0) {
		dev_err(dev, "no offset in %s\n", key);
		return -EINVAL;
	}

	ret = of_property_read_u32_index(np, key, 2, &pmu->ipi_bit);
	if (ret < 0) {
		dev_err(dev, "no bit in %s\n", key);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "version", &table->ver);
	if (ret) {
		dev_err(dev, "%pOF missing [version] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "id", &table->id);
	if (ret) {
		dev_err(dev, "%pOF missing [id] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "number", &table->num);
	if (ret) {
		dev_err(dev, "%pOF missing [number] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "type", &table->type);
	if (ret) {
		dev_err(dev, "%pOF missing [type] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dfeatures", &table->dfeatures);
	if (ret) {
		dev_err(dev, "%pOF missing [dfeatures] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "gfeatures", &table->gfeatures);
	if (ret) {
		dev_err(dev, "%pOF missing [gfeatures] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "nr-vrings", (uint32_t *)&table->num_of_vrings);
	if (ret) {
		dev_err(dev, "%pOF missing [nr-vrings] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "vring0-align", &table->vring[0].align);
	if (ret) {
		dev_err(dev, "%pOF missing [vring0-align] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "vring0-nr", &table->vring[0].num);
	if (ret) {
		dev_err(dev, "%pOF missing [vring0-nr] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "vring1-align", &table->vring[1].align);
	if (ret) {
		dev_err(dev, "%pOF missing [vring1-align] property\n", np);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "vring1-nr", &table->vring[1].num);
	if (ret) {
		dev_err(dev, "%pOF missing [vring1-nr] property\n", np);
		return -EINVAL;
	}

	// the fw_rsc_hdr offset
	table->offset[0] = offsetof(struct mtk_rsc_table, type);

	dev_dbg(dev, "id %d, version %d, number %d, type %d\n"
			"dfeatures %d, gfeatures %d, nr-vrings %d\n"
			"vring0-da 0x%x, vring0-align 0x%x, vring0-nr %d\n"
			"vring1-da 0x%x, vring1-align 0x%x, vring1-nr %d\n",
			table->id, table->ver, table->num, table->type,
			table->dfeatures, table->gfeatures, table->num_of_vrings,
			table->vring[0].da, table->vring[0].align, table->vring[0].num,
			table->vring[1].da, table->vring[1].align, table->vring[1].num);
	return 0;
}

static int _rsctable_mem_map(struct rproc *rproc,
		struct rproc_mem_entry *mem)
{
	struct device *dev;
	struct mtk_pmu *pmu;
	struct mtk_rsc_table *table;
	void *va;

	if (!rproc || !mem || !rproc->priv || !rproc->dev.parent)
		return -EINVAL;

	dev = rproc->dev.parent;
	pmu = rproc->priv;
	table = pmu->table;

	if (!table)
		return -EINVAL;

	va = ioremap_wc(mem->da, mem->len);
	if (!va) {
		dev_err(dev, "Unable to map memory region: %pa+%x\n",
				&mem->da, mem->len);
		return -ENOMEM;
	}
	mem->va = va;
	mem->dma = phys_to_dma(dev, mem->da);

	// copy the local shm table to remote resource table
	memcpy(va, table, sizeof(struct mtk_rsc_table));
	/*
	 * cached_table should be NULL in rproc->attatch
	 * assume fw is already loaded
	 */
	rproc->cached_table = NULL;
	rproc->table_ptr = va;
	rproc->table_sz = sizeof(struct mtk_rsc_table);

	return 0;
}

static int pmu_rproc_mem_alloc(struct rproc *rproc,
		struct rproc_mem_entry *mem)
{
	struct device *dev = rproc->dev.parent;
	void *va;

	va = ioremap_wc(mem->dma, mem->len);
	if (!va) {
		dev_err(dev, "Unable to map memory region: %pa+%x\n",
				&mem->dma, mem->len);
		return -ENOMEM;
	}

	/* Update memory entry va */
	mem->va = va;

	return 0;
}

static int pmu_rproc_mem_release(struct rproc *rproc,
		struct rproc_mem_entry *mem)
{
	iounmap(mem->va);

	return 0;
}

static void resource_cleanup(struct rproc *rproc)
{
	struct rproc_mem_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &rproc->carveouts, node) {
		if (entry->release)
			entry->release(rproc, entry);
		else
			pmu_rproc_mem_release(rproc, entry);
		list_del(&entry->node);
		kfree(entry);
	}
}

static int pmu_assigned_carveout(struct rproc *rproc)
{
	int ret, idx = 0;
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	struct mtk_pmu *pmu = rproc->priv;
	struct mtk_rsc_table *table = pmu->table;

	of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	while (of_phandle_iterator_next(&it) == 0) {
		struct rproc_mem_entry *mem;
		struct reserved_mem *rmem;

		rmem = of_reserved_mem_lookup(it.node);
		if (!rmem) {
			dev_err(dev, "unable to acquire memory-region\n");
			return -EINVAL;
		}

		dev_info(dev, "%s reserved mem %pa, size 0x%lx\n", rmem->name,
				&rmem->base, (unsigned long)rmem->size);

		if (REGION_IS(it.node->name, "vring")) {
			// the vring carveout base should set to resource table
			table->vring[idx].da = (u32)rmem->base;
			mem = rproc_mem_entry_init(dev, NULL, (dma_addr_t)rmem->base,
					rmem->size, rmem->base,
					pmu_rproc_mem_alloc,
					pmu_rproc_mem_release,
					it.node->name);
			if (!mem)
				return -ENOMEM;
		} else if (REGION_IS(it.node->name, "buffer")) {
			mem = rproc_mem_entry_init(dev, NULL, (dma_addr_t)rmem->base,
					rmem->size, rmem->base,
					pmu_rproc_mem_alloc,
					pmu_rproc_mem_release,
					it.node->name);
			if (!mem)
				return -ENOMEM;

		} else if (REGION_IS(it.node->name, "rscshm")) {
			mem = rproc_of_resm_mem_entry_init(dev, idx, rmem->size,
					rmem->base, it.node->name);
			if (!mem)
				return -ENOMEM;
			ret = _rsctable_mem_map(rproc, mem);
			if (ret)
				return ret;
		} else {
			dev_info(dev, "unknown reserved memory region\n");
			return -EINVAL;
		}

		rproc_add_carveout(rproc, mem);
		idx++;
	}

	return 0;
}

static int mtk_pmu_rproc_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct mtk_pmu *pmu;
	struct rproc *rproc;

	/* no need to load pmu fimrware */
	rproc = rproc_alloc(dev, np->name, &pmu_ops,
			NULL, sizeof(*pmu));
	if (!rproc) {
		dev_err(dev, "unable to allocate remoteproc\n");
		return -ENOMEM;
	}

	// PMU boot up already
	rproc->has_iommu = false;
	rproc->auto_boot = true;
	rproc->state = RPROC_DETACHED;

	pmu = (struct mtk_pmu *)rproc->priv;
	pmu->rproc = rproc;
	pmu->dev = dev;
	/* alloc dummy resource table */
	pmu->table = devm_kzalloc(dev, sizeof(struct mtk_rsc_table), GFP_KERNEL);
	if (!pmu->table) {
		ret = -ENOMEM;
		goto free_rproc;
	}

	platform_set_drvdata(pdev, rproc);

	ret = mtk_pmu_parse_dt(pdev);
	if (ret)
		goto free_rproc;

	ret = devm_request_threaded_irq(dev, pmu->irq, NULL,
			handle_event, IRQF_ONESHOT,
			"mtk-pmu", rproc);
	if (ret) {
		dev_err(dev, "request irq @%d failed\n", pmu->irq);
		goto free_rproc;
	}

	dev_info(dev, "request irq @%d for kick pmu\n",  pmu->irq);

	ret = dma_coerce_mask_and_coherent(dev, dma_get_mask(dev->parent));
	if (ret)
		dev_warn(dev, "Failed to set DMA mask %llx. Trying to continue... %x\n",
				dma_get_mask(dev->parent), ret);

	ret = pmu_assigned_carveout(rproc);
	if (ret)
		goto cleanup;

	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "rproc_add failed! ret = %d\n", ret);
		goto cleanup;
	}

	dev_info(dev, "MTK remote processor %s is now up\n", rproc->name);

	return 0;

cleanup:
	// use rproc_resource_cleanup() is better
	resource_cleanup(rproc);
free_rproc:
	rproc_free(rproc);
	return ret;
}

static int mtk_pmu_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);

	// use rproc_resource_cleanup() is better
	rproc_del(rproc);
	rproc_free(rproc);

	return 0;
}

static const struct of_device_id mtk_pmu_of_match[] = {
	{ .compatible = "mediatek,pmu"},
	{ .compatible = "mediatek,pqu-running"},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_pmu_of_match);

static struct platform_driver mtk_pmu_rproc = {
	.probe = mtk_pmu_rproc_probe,
	.remove = mtk_pmu_rproc_remove,
	.driver = {
		.name = "mtk-pmu",
		.of_match_table = of_match_ptr(mtk_pmu_of_match),
	},
};

module_platform_driver(mtk_pmu_rproc);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek PMU remoteproce driver");
