// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */
#include <asm/page.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/swap.h>
#include <linux/sched/signal.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/dma-direct.h>
#include <linux/cma.h>
#include <linux/sizes.h>
#include <asm/cacheflush.h>
#include <linux/ion.h>

#include "cma.h"

#include "mtk_iommu_ion.h"
#include "mtk_iommu_cma_heap.h"
#include "mtk_iommu_dtv_api.h"
#include "mtk_iommu_ion_heap.h"
#include "mtk_iommu_internal.h"

#define IOMMU_CMA_ALIGN	SZ_2M
#define BIT31		0x80000000
#define NR_CMA_DEV	2

struct mtk_iommu_cma_heap {
	struct ion_heap heap;
	struct cma *cma[MAX_IOMMU_CMA_AREAS];
	void *priv;
};

#define to_cma_heap(x) container_of(x, struct mtk_iommu_cma_heap, heap)

struct iommu_cma_node {
	struct list_head list;
	struct page *page;
	unsigned long len;
};

static inline bool cma_range(struct cma *cma, struct page *page)
{
	unsigned long pfn = page_to_pfn(page);

	if (!cma)
		return false;

	if (pfn >= cma->base_pfn
		&& pfn < (cma->base_pfn + cma->count))
		return true;

	return false;
}

static int alloc_from_buddy(struct list_head *pages,
			size_t *remaining, unsigned long flags)
{
	int i = 0;
	struct page *page = NULL;
	struct iommu_cma_node *node;
	gfp_t gfp_mask = GFP_KERNEL;

	if (flags & IOMMUMD_FLAG_DMAZONE)
		_update_gfp_mask(&gfp_mask);

	while (*remaining > 0) {
		page = alloc_pages(gfp_mask, 0);
		if (!page) {
			pr_err("%s: alloc_pages fail\n", __func__);
			return -ENOMEM;
		}
		node = kzalloc(sizeof(struct iommu_cma_node),
				GFP_KERNEL);
		if (!node)
			return -ENOMEM;

		node->len = PAGE_SIZE;
		node->page = page;
		INIT_LIST_HEAD(&node->list);
		list_add_tail(&node->list, pages);
		*remaining -= PAGE_SIZE;
		i++;
	}

	return i;
}

static int alloc_from_cma(struct mtk_iommu_cma_heap *cma_heap, struct list_head *pages,
			size_t *remaining)
{
	int i = 0;
	int j;
	size_t len;
	struct iommu_cma_node *node;
	dma_addr_t dma_handle = 0;
	struct page *page = NULL;
	unsigned long nr_pages;
	unsigned long align;

	while (*remaining > 0) {
		len = min_t(size_t, *remaining, IOMMU_CMA_ALIGN);
		dma_handle = 0;
		nr_pages = len >> PAGE_SHIFT;
		align = get_order(len);

		for (j = 0; j < MAX_IOMMU_CMA_AREAS && !page && cma_heap->cma[j]; j++) {
			page = cma_alloc(cma_heap->cma[j], nr_pages, align, false);
		}
		// seen null page as cma has not enough space to alloc
		if (!page) {
			pr_err("%s : no available cma space\n", __func__);
			break;
		}

		if (PageHighMem(page)) {
			unsigned long nr_clear_pages = nr_pages;
			struct page *p = page;

			while (nr_clear_pages > 0) {
				void *vaddr = kmap_atomic(p);

				memset(vaddr, 0, PAGE_SIZE);
				kunmap_atomic(vaddr);
				p++;
				nr_clear_pages--;
			}
		} else {
			memset(page_address(page), 0, len);
		}

		node = kzalloc(sizeof(struct iommu_cma_node), GFP_KERNEL);
		if (!node)
			return -ENOMEM;

		node->len = len;
		node->page = page;
		INIT_LIST_HEAD(&node->list);
		list_add_tail(&node->list, pages);
		*remaining -= len;
		i++;
		page = NULL;

	}
	pr_info("%s: cma alloc %d pages, remaining %zu fallback to alloc_pages\n", __func__, i, *remaining);

	return i;
}

int sys_cma_allocate(struct ion_heap *heap, struct ion_buffer *buffer,
			unsigned long size, unsigned long flags)
{
	int ret;
	unsigned int nents = 0;
	struct sg_table *table;
	struct scatterlist *sg;
	struct list_head pages;
	struct iommu_cma_node *node, *tmp;
	size_t size_remaining = PAGE_ALIGN(size);
	struct mtk_iommu_cma_heap *cma_heap = to_cma_heap(heap);
#ifdef CONFIG_ARM
	struct device *ion_dev = cma_heap->priv;
#endif
	int j;
	int in_cma;

	INIT_LIST_HEAD(&pages);

	ret = alloc_from_cma(cma_heap, &pages, &size_remaining);
	if (ret < 0)
		goto out;

	nents += ret;

	if (size_remaining > 0) {
		pr_info("%s: 0x%zx size remain\n", __func__, size_remaining);
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

	if (sg_alloc_table(table, nents, GFP_KERNEL)) {
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
		dma_map_sgtable(ion_dev, table, DMA_BIDIRECTIONAL, 0);
		dma_unmap_sgtable(ion_dev, table, DMA_BIDIRECTIONAL, 0);
	}

#endif

	return 0;
out:
	if (nents == 0)
		return ret;

	list_for_each_entry_safe(node, tmp, &pages, list) {
		in_cma = 0;
		for (j = 0; j < MAX_IOMMU_CMA_AREAS && cma_heap->cma[j]; j++) {
			if (cma_range(cma_heap->cma[j], node->page)) {
				in_cma = 1;
				cma_release(cma_heap->cma[j], node->page, node->len >> PAGE_SHIFT);
			}
		}
		if (!in_cma)
			__free_pages(node->page, get_order(node->len));
		list_del(&node->list);
		kfree(node);
	}

	return ret;
}

void sys_cma_free(struct ion_buffer *buffer)
{
	struct mtk_iommu_cma_heap *cma_heap = to_cma_heap(buffer->heap);
	struct sg_table *table = buffer->sg_table;
	struct scatterlist *sg;
	int i;
	struct page *page;
	unsigned int order;
	unsigned long nr_pages;
	int j;
	int in_cma;

	if (!table)
		return;

	for_each_sg(table->sgl, sg, table->orig_nents, i) {
		page = sg_page(sg);
		nr_pages = sg->length >> PAGE_SHIFT;
		order = get_order(sg->length);
		in_cma = 0;
		for (j = 0; j < MAX_IOMMU_CMA_AREAS && cma_heap->cma[j]; j++) {
			if (cma_range(cma_heap->cma[j], page)) {
				in_cma = 1;
				cma_release(cma_heap->cma[j], page, nr_pages);
			}
		}
		if (!in_cma)
			__free_pages(page, order);
	}

	sg_free_table(table);
	kfree(table);
	buffer->sg_table = NULL;
}

static struct ion_heap_ops mtk_iommu_cma_ops = {
	.allocate = sys_cma_allocate,
	.free = sys_cma_free,
};

static struct mtk_iommu_cma_heap cma_heap = {
	.heap = {
		.ops = &mtk_iommu_cma_ops,
		.type = ION_HEAP_TYPE_CUSTOM,
		.name = MTK_ION_CMA_HEAP_NAME,
	}
};

static int __find_and_add_cma_heap(struct cma *cma, void *data)
{
	//struct ion_cma_heap *cma_heap;
	unsigned int *nr = data;

	if (!cma)
		return -EINVAL;
	if (strstr(cma_get_name(cma), "iommu_cma")) {
		if (*nr >= MAX_IOMMU_CMA_AREAS)
			return -EINVAL;

		cma_heap.cma[*nr] = cma;
		pr_crit("%s: add %d-th iommu cma heaps, cma area: %s\n",
				__func__, *nr, cma_get_name(cma));
		*nr += 1;
	}
	return 0;
}

static int mtk_iommu_cma_heap_init(void)
{
	int ret = 0;
	int nr = 0;

	ret = cma_for_each_area(__find_and_add_cma_heap, &nr);
	if (ret)
		goto out;
	ret = ion_device_add_heap(&cma_heap.heap);
out:
	return ret;
}

int mtk_cma_heap_init(struct device *dev)
{
	int ret;

	cma_heap.priv = dev;
	ret = mtk_iommu_cma_heap_init();
	if (ret) {
		pr_err("%s : ion cma heap init fail\n", __func__);
		return ret;
	}

	return ret;
}

void mtk_cma_heap_exit(void)
{
	ion_device_remove_heap(&cma_heap.heap);
}
