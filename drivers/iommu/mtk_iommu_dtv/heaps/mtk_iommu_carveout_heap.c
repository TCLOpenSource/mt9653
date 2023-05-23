// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */
#include <asm/page.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/dma-direct.h>
#include <linux/sizes.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_address.h>
#include <linux/genalloc.h>
#include <linux/mutex.h>
#include <linux/highmem.h>
#include <asm/cacheflush.h>

#include "mtk_iommu_ion.h"
#include "mtk_iommu_dtv_api.h"
#include "mtk_iommu_carveout_heap.h"
#include "mtk_iommu_internal.h"

#define IOMMU_CARVEOUT_ALIGN	SZ_1M

struct mtk_ion_carveout_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	phys_addr_t pa;
	size_t size;
	void *priv;
};

struct iommu_carveout_node {
	struct list_head list;
	struct page *page;
	unsigned long len;
};

static inline bool is_carveout_range(struct mtk_ion_carveout_heap *heap,
			struct page *page)
{
	unsigned long ba;

	WARN_ON(!heap || !page);

	ba = (unsigned long)page_to_phys(page);
	return (ba >= heap->pa && ba < (heap->pa + heap->size));
}

static int alloc_from_buddy(struct list_head *pages,
			size_t *remaining, unsigned long flags)
{
	int i = 0;
	struct page *page;
	struct iommu_carveout_node *node;
	gfp_t gfp_mask = GFP_HIGHUSER;

	if (flags & IOMMUMD_FLAG_DMAZONE)
		_update_gfp_mask(&gfp_mask);

	while (*remaining > 0) {
		page = alloc_pages(gfp_mask, 0);
		if (!page) {
			pr_err("%s: alloc_pages fail\n", __func__);
			return -ENOMEM;
		}
		node = kzalloc(sizeof(struct iommu_carveout_node),
				GFP_KERNEL);
		if (!node)
			return -ENOMEM;

		node->len = PAGE_SIZE;
		node->page = page;
		INIT_LIST_HEAD(&node->list);
		list_add_tail(&node->list, pages);
		*remaining -= PAGE_SIZE;
		i++;

		pr_debug("%s: alloc 0x%llx+%lx from buddy sys\n",
				__func__, page_to_phys(page), PAGE_SIZE);
	}

	return i;
}

static int alloc_from_resv_mem(struct gen_pool *pool, struct list_head *pages,
			size_t *remaining)
{
	int i = 0;
	unsigned long pa;
	size_t len;
	struct iommu_carveout_node *node;

	while (*remaining > 0) {
		len = min_t(size_t, *remaining, IOMMU_CARVEOUT_ALIGN);
		pa = gen_pool_alloc(pool, len);
		if (!pa) {
			pr_warn("%s: carveout mem not enough\n", __func__);
			break;
		}

		if (offset_in_page(pa)) {
			pr_err("%s: pa is not page aligned\n", __func__);
			gen_pool_free(pool, pa, len);
			return -EINVAL;
		}

		node = kzalloc(sizeof(struct iommu_carveout_node), GFP_KERNEL);
		if (!node)
			return -ENOMEM;

		node->len = len;
		node->page = phys_to_page(pa);
		INIT_LIST_HEAD(&node->list);
		list_add_tail(&node->list, pages);
		*remaining -= len;
		i++;

		pr_debug("%s: alloc 0x%lx+%lx from carveout\n", __func__, pa, len);
	}

	return i;
}

static int mtk_ion_carveout_allocate(struct ion_heap *heap, struct ion_buffer *buffer,
			      unsigned long len, unsigned long flags)
{
	int ret;
	unsigned int nents = 0;
	struct sg_table *table;
	struct scatterlist *sg;
	struct list_head pages;
	struct iommu_carveout_node *node, *tmp;
	size_t size_remaining = PAGE_ALIGN(len);
	struct mtk_ion_carveout_heap *carv = container_of(heap,
			struct mtk_ion_carveout_heap, heap);
#ifdef CONFIG_ARM
	struct device *dev = carv->priv;
#endif

	INIT_LIST_HEAD(&pages);

	ret = alloc_from_resv_mem(carv->pool, &pages, &size_remaining);
	if (ret < 0)
		goto out;

	nents += ret;

	if (size_remaining > 0) {
		pr_info("%s: reserved mem is not enough (%ld/%ld), alloc from buddy sys\n",
			 __func__, (PAGE_ALIGN(len) - size_remaining), PAGE_ALIGN(len));
		ret = alloc_from_buddy(&pages, &size_remaining, flags);
		if (ret < 0)
			goto out;
		nents += ret;
	}

	// create scatter list to link all the pages
	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table) {
		ret = -ENOMEM;
		goto out;
	}

	ret = sg_alloc_table(table, nents, GFP_KERNEL);
	if (ret) {
		pr_err("%s: sg_alloc_table failed with %d\n", __func__, ret);
		kfree(table);
		ret = -ENOMEM;
		goto out;
	}

	sg = table->sgl;
	list_for_each_entry_safe(node, tmp, &pages, list) {
		sg_set_page(sg, node->page, node->len, 0);
		sg = sg_next(sg);
		list_del(&node->list);
		kfree(node);
	}

	buffer->sg_table = table;

	if (flags & IOMMUMD_FLAG_ZEROPAGE)
		ion_buffer_zero(buffer);

	ion_buffer_prep_noncached(buffer);
#ifdef CONFIG_ARM
	// because ion_buffer_prep_noncached is useless in arm,
	// so that need to flush again in non-cahce buffer
	if (!(buffer->flags & ION_FLAG_CACHED)) {
		dma_map_sgtable(dev, table, DMA_BIDIRECTIONAL, 0);
		dma_unmap_sgtable(dev, table, DMA_BIDIRECTIONAL, 0);
	}
#endif

	return 0;
out:
	if (nents == 0)
		return ret;

	list_for_each_entry_safe(node, tmp, &pages, list) {
		if (is_carveout_range(carv, node->page))
			gen_pool_free(carv->pool, page_to_phys(node->page), node->len);
		else
			__free_pages(node->page, get_order(node->len));
		list_del(&node->list);
		kfree(node);
	}

	return ret;
}

static void mtk_ion_carveout_free(struct ion_buffer *buffer)
{
	int i, ret;
	struct page *page;
	struct ion_heap *heap = buffer->heap;
	struct sg_table *table = buffer->sg_table;
	struct scatterlist *sg;
	struct mtk_ion_carveout_heap *carv = container_of(heap,
			struct mtk_ion_carveout_heap, heap);
	struct mtk_dtv_iommu_data *data;

	if (!table)
		return;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	mutex_lock(&(data->buf_lock));

	for_each_sg(table->sgl, sg, table->orig_nents, i) {
		page = sg_page(sg);
		if (is_carveout_range(carv, page))
			gen_pool_free(carv->pool, page_to_phys(page), sg->length);
		else
			__free_pages(page, get_order(sg->length));
	}

	sg_free_table(table);
	kfree(table);
	buffer->sg_table = NULL;

	mutex_unlock(&(data->buf_lock));
}

static struct ion_heap_ops mtk_carveout_heap_ops = {
	.allocate = mtk_ion_carveout_allocate,
	.free = mtk_ion_carveout_free,
};

static const struct of_device_id mtk_iommu_carveout_of_ids[] = {
	{.compatible = "mediatek,dtv-iommu-carveout",},
	{ }
};

static int iommu_carveout_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np;
	struct mtk_ion_carveout_heap *carv;
	struct resource r;

	np = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!np) {
		dev_err(dev, "No memory-region specified\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(np, 0, &r);
	if (ret) {
		dev_err(dev, "No memory address assigned to the region\n");
		return -EINVAL;
	}

	carv = devm_kzalloc(dev, sizeof(struct mtk_ion_carveout_heap), GFP_KERNEL);
	if (!carv)
		return -ENOMEM;

	dev_info(dev, "reserved mem pa:%pa+%lx\n",
			&r.start, (unsigned long)resource_size(&r));

	carv->pa = r.start;
	carv->size = (size_t)resource_size(&r);
	carv->heap.type = ION_HEAP_TYPE_CUSTOM;
	carv->heap.ops = &mtk_carveout_heap_ops;
	carv->heap.name = MTK_ION_CARVEOUT_HEAP_NAME;
	carv->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!carv->pool) {
		dev_err(dev, "gen pool failed\n");
		gen_pool_destroy(carv->pool);
		return -ENOMEM;
	}
	carv->priv = &(pdev->dev);
	platform_set_drvdata(pdev, carv);

	ret = gen_pool_add(carv->pool, carv->pa, carv->size, -1);
	if (ret) {
		dev_err(dev, "gen_pool_add_virt failed with %d\n", ret);
		gen_pool_destroy(carv->pool);
		return ret;
	}

	return ion_device_add_heap(&(carv->heap));
}

static int iommu_carveout_remove(struct platform_device *pdev)
{
	struct mtk_ion_carveout_heap *carv =
		platform_get_drvdata(pdev);

	if (!carv)
		return -EINVAL;

	ion_device_remove_heap(&(carv->heap));
	gen_pool_destroy(carv->pool);

	return 0;
}

static struct platform_driver carveout_heap_driver = {
	.probe = iommu_carveout_probe,
	.remove = iommu_carveout_remove,
	.driver = {
		.name = "mtk-iommu-carveout",
		.of_match_table = mtk_iommu_carveout_of_ids,
	}
};

int mtk_carveout_heap_init(void)
{
	return platform_driver_register(&carveout_heap_driver);
}

void mtk_carveout_heap_exit(void)
{
	platform_driver_unregister(&carveout_heap_driver);
}
