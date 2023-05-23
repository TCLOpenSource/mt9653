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
#include <linux/mutex.h>

#include "mtk_iommu_ion.h"
#include "mtk_iommu_sys_heap.h"
#include "mtk_iommu_cma_heap.h"
#include "mtk_iommu_dtv_api.h"
#include "mtk_iommu_internal.h"
#include <asm/cacheflush.h>

static gfp_t sys_high_order_gfp_flags = (GFP_HIGHUSER | __GFP_NOWARN |
				     __GFP_NORETRY) & ~__GFP_RECLAIM;
static gfp_t sys_low_order_gfp_flags  = (GFP_HIGHUSER);

static const unsigned int orders[] = {8, 4, 2, 1, 0};

#define NUM_ORDERS ARRAY_SIZE(orders)

struct mtk_sys_heap {
	struct ion_heap heap;
	struct sys_page_pool *pools[NUM_ORDERS];
	void *priv;
};

static int order_to_index(unsigned int order)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++)
		if (order == orders[i])
			return i;

	WARN_ON(1);
	return -1;
}

static inline unsigned int order_to_size(int order)
{
	return PAGE_SIZE << order;
}

static inline struct page *sys_page_pool_alloc_pages(
		struct sys_page_pool *pool, unsigned long flags)
{
	gfp_t gfp_mask = pool->gfp_mask;

	if (flags & IOMMUMD_FLAG_DMAZONE)
		_update_gfp_mask(&gfp_mask);

	if (fatal_signal_pending(current))
		return NULL;

	return alloc_pages(gfp_mask, pool->order);
}

static void sys_page_pool_free_pages(struct sys_page_pool *pool,
				     struct page *page)
{
	__free_pages(page, pool->order);
}

static void sys_page_pool_add(struct sys_page_pool *pool, struct page *page)
{
	mutex_lock(&pool->mutex);
	if (PageHighMem(page)) {
		list_add_tail(&page->lru, &pool->high_items);
		pool->high_count++;
	} else if (page_zonenum(page) == ZONE_NORMAL) {
		list_add_tail(&page->lru, &pool->normal_items);
		pool->normal_count++;
	} else {
		list_add_tail(&page->lru, &pool->dma_items);
		pool->dma_count++;
	}

	mod_node_page_state(page_pgdat(page), NR_KERNEL_MISC_RECLAIMABLE,
							1 << pool->order);

	mutex_unlock(&pool->mutex);
}

static struct page *sys_page_pool_remove(struct sys_page_pool *pool,
			enum mtk_sys_pool_type type)
{
	struct page *page;

	if (type == POOL_TYPE_HIGH) {
		WARN_ON(!pool->high_count);
		page = list_first_entry(&pool->high_items, struct page, lru);
		pool->high_count--;
	} else if (type == POOL_TYPE_NORMAL) {
		WARN_ON(!pool->normal_count);
		page = list_first_entry(&pool->normal_items, struct page, lru);
		pool->normal_count--;
	} else {
		WARN_ON(!pool->dma_count);
		page = list_first_entry(&pool->dma_items, struct page, lru);
		pool->dma_count--;
	}

	list_del(&page->lru);
	mod_node_page_state(page_pgdat(page), NR_KERNEL_MISC_RECLAIMABLE,
							-(1 << pool->order));

	return page;
}

static struct page *sys_page_pool_alloc(struct sys_page_pool *pool,
					unsigned long flags)
{
	struct page *page = NULL;

	WARN_ON(!pool);

	mutex_lock(&pool->mutex);
	if ((flags & IOMMUMD_FLAG_DMAZONE) && pool->dma_count) {
		page = sys_page_pool_remove(pool, POOL_TYPE_DMA);
	} else {
		if (pool->high_count)
			page = sys_page_pool_remove(pool, POOL_TYPE_HIGH);
		else if (pool->normal_count)
			page = sys_page_pool_remove(pool, POOL_TYPE_NORMAL);
		else if (pool->dma_count)
			page = sys_page_pool_remove(pool, POOL_TYPE_DMA);
	}
	mutex_unlock(&pool->mutex);
	if (!page)
		page = sys_page_pool_alloc_pages(pool, flags);

	return page;
}

static void sys_page_pool_free(struct sys_page_pool *pool, struct page *page)
{
	WARN_ON(pool->order != compound_order(page));

	sys_page_pool_add(pool, page);
}

static int sys_page_pool_total(struct sys_page_pool *pool,
		enum mtk_sys_pool_type type)
{
	int count = pool->dma_count;

	count += pool->normal_count;
	if (type == POOL_TYPE_HIGH)
		count += pool->high_count;

	return count << pool->order;
}

static int sys_page_pool_shrink(struct sys_page_pool *pool, gfp_t gfp_mask,
			 int nr_to_scan)
{
	int freed = 0;
	bool high;
	enum mtk_sys_pool_type type = POOL_TYPE_MAX;

	if (current_is_kswapd())
		high = true;
	else
		high = !!(gfp_mask & __GFP_HIGHMEM);
	if (high)
		type = POOL_TYPE_HIGH;
	if (nr_to_scan == 0)
		return sys_page_pool_total(pool, type);

	while (freed < nr_to_scan) {
		struct page *page;

		mutex_lock(&pool->mutex);
		if (pool->dma_count) {
			page = sys_page_pool_remove(pool, POOL_TYPE_DMA);
		} else if (pool->normal_count) {
			page = sys_page_pool_remove(pool, POOL_TYPE_NORMAL);
		} else if (high && pool->high_count) {
			page = sys_page_pool_remove(pool, POOL_TYPE_HIGH);
		} else {
			mutex_unlock(&pool->mutex);
			break;
		}
		mutex_unlock(&pool->mutex);
		sys_page_pool_free_pages(pool, page);
		freed += (1 << pool->order);
	}

	return freed;
}

int sys_page_pool_nr_pages(struct sys_page_pool *pool)
{
	int nr_total_pages;

	mutex_lock(&pool->mutex);
	nr_total_pages = sys_page_pool_total(pool, true);
	mutex_unlock(&pool->mutex);

	return nr_total_pages;
}

static struct sys_page_pool *sys_page_pool_create(gfp_t gfp_mask,
		unsigned int order)
{
	struct sys_page_pool *pool = kmalloc(sizeof(*pool), GFP_KERNEL);

	if (!pool)
		return NULL;
	pool->high_count = 0;
	pool->normal_count = 0;
	pool->dma_count = 0;
	INIT_LIST_HEAD(&pool->dma_items);
	INIT_LIST_HEAD(&pool->normal_items);
	INIT_LIST_HEAD(&pool->high_items);
	pool->gfp_mask = gfp_mask | __GFP_COMP;
	pool->order = order;
	mutex_init(&pool->mutex);
	plist_node_init(&pool->list, order);

	return pool;
}

static void sys_page_pool_destroy(struct sys_page_pool *pool)
{
	kfree(pool);
}

static struct page *alloc_buffer_page(struct mtk_sys_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long order, unsigned long flags)
{
	struct sys_page_pool *pool = NULL;
	int order_index = order_to_index(order);

	if (order_index >= 0) {
		pool = heap->pools[(unsigned int)order_index];
		return sys_page_pool_alloc(pool, flags);
	}

	return NULL;
}

static void free_buffer_page(struct mtk_sys_heap *heap,
			     struct ion_buffer *buffer, struct page *page)
{
	struct sys_page_pool *pool = NULL;
	unsigned int order = compound_order(page);
	int order_index = order_to_index(order);

	/* go to system */
	if (buffer->private_flags & ION_PRIV_FLAG_SHRINKER_FREE) {
		__free_pages(page, order);
		return;
	}

	if (order_index >= 0) {
		pool = heap->pools[(unsigned int)order_index];
		sys_page_pool_free(pool, page);
	}

	return;
}

static struct page *alloc_largest_available(struct mtk_sys_heap *heap,
		struct ion_buffer *buffer, unsigned long size,
		unsigned int max_order, unsigned long flags)
{
	struct page *page;
	int i;

	for (i = 0; i < NUM_ORDERS; i++) {
		if (size < order_to_size(orders[i]))
			continue;
		if (max_order < orders[i])
			continue;

		page = alloc_buffer_page(heap, buffer, orders[i], flags);
		if (!page)
			continue;

		return page;
	}

	return NULL;
}

static int mtk_sys_heap_allocate(struct ion_heap *heap,
				    struct ion_buffer *buffer,
				    unsigned long size,
				    unsigned long flags)
{
	struct mtk_sys_heap *sys_heap = container_of(heap,
							struct mtk_sys_heap,
							heap);
	struct sg_table *table;
	struct scatterlist *sg;
	struct list_head pages;
	struct page *page, *tmp_page;
	int i = 0;
	unsigned long size_remaining = PAGE_ALIGN(size);
	unsigned int max_order = orders[0];
#ifdef CONFIG_ARM
	struct device *ion_dev = sys_heap->priv;
#endif

	// TODO: use cma heaps
	if ((flags & IOMMUMD_FLAG_CMA0) || (flags & IOMMUMD_FLAG_CMA1))
		return sys_cma_allocate(heap, buffer, size, flags);

	if (size / PAGE_SIZE > totalram_pages() / 2)
		return -ENOMEM;

	INIT_LIST_HEAD(&pages);
	while (size_remaining > 0) {
		page = alloc_largest_available(sys_heap, buffer, size_remaining,
					       max_order, flags);
		if (!page)
			goto free_pages;
		list_add_tail(&page->lru, &pages);
		size_remaining -= PAGE_SIZE << compound_order(page);
		max_order = compound_order(page);
		i++;
	}
	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto free_pages;

	if (sg_alloc_table(table, i, GFP_KERNEL))
		goto free_table;

	sg = table->sgl;
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		sg_set_page(sg, page, PAGE_SIZE << compound_order(page), 0);
		sg = sg_next(sg);
		list_del(&page->lru);
	}

	buffer->sg_table = table;

	if (flags & IOMMUMD_FLAG_ZEROPAGE)
		ion_buffer_zero(buffer);

	ion_buffer_prep_noncached(buffer);
#ifdef CONFIG_ARM
	// because ion_buffer_prep_noncached is useless in arm,
	// so that need to flush again in non-cahce buffer
	if (!(buffer->flags & ION_FLAG_CACHED)) {
		dma_sync_sg_for_cpu(ion_dev, table->sgl, i, DMA_BIDIRECTIONAL);
		dma_sync_sg_for_device(ion_dev, table->sgl, i, DMA_BIDIRECTIONAL);
	}
#endif
	return 0;

free_table:
	kfree(table);
free_pages:
	list_for_each_entry_safe(page, tmp_page, &pages, lru)
		free_buffer_page(sys_heap, buffer, page);
	return -ENOMEM;
}

static void mtk_sys_heap_free(struct ion_buffer *buffer)
{
	struct mtk_sys_heap *sys_heap;
	struct sg_table *table;
	struct scatterlist *sg;
	int i, ret;
	struct mtk_dtv_iommu_data *data;
	struct mtk_iommu_buf_handle *buf_handle;


	if (!buffer)
		return;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	mutex_lock(&(data->buf_lock));

	if (buffer->flags & ION_FLAG_CACHED) {
		buffer->flags &= ~ION_FLAG_CACHED;
		ion_buffer_prep_noncached(buffer);
	}

	// TODO: use cma heaps
	if ((buffer->flags & IOMMUMD_FLAG_CMA0)
		|| (buffer->flags & IOMMUMD_FLAG_CMA1)) {
		sys_cma_free(buffer);
		mutex_unlock(&(data->buf_lock));
		return;
	}

	sys_heap = container_of(buffer->heap,
				struct mtk_sys_heap,
				heap);
	table = buffer->sg_table;

	for_each_sg(table->sgl, sg, table->orig_nents, i)
		free_buffer_page(sys_heap, buffer, sg_page(sg));
	// set buf_handle sgt as free
	list_for_each_entry(buf_handle, &(data->buf_list_head),
				buf_list_node) {
		if (buf_handle->sgt == table)
			buf_handle->sgt = NULL;
	}

	sg_free_table(table);
	kfree(table);
	buffer->sg_table = NULL;

	mutex_unlock(&(data->buf_lock));
}

static int mtk_sys_heap_shrink(struct ion_heap *heap, gfp_t gfp_mask,
				  int nr_to_scan)
{
	struct sys_page_pool *pool;
	struct mtk_sys_heap *sys_heap;
	int nr_total = 0;
	int i, nr_freed;
	int only_scan = 0;

	sys_heap = container_of(heap, struct mtk_sys_heap, heap);

	if (!nr_to_scan)
		only_scan = 1;

	for (i = 0; i < NUM_ORDERS; i++) {
		pool = sys_heap->pools[i];

		if (only_scan) {
			nr_total += sys_page_pool_shrink(pool,
							 gfp_mask,
							 nr_to_scan);

		} else {
			nr_freed = sys_page_pool_shrink(pool,
							gfp_mask,
							nr_to_scan);
			nr_to_scan -= nr_freed;
			nr_total += nr_freed;
			if (nr_to_scan <= 0)
				break;
		}
	}
	return nr_total;
}

static long mtk_sys_get_pool_size(struct ion_heap *heap)
{
	struct mtk_sys_heap *sys_heap;
	long total_pages = 0;
	int i;

	sys_heap = container_of(heap, struct mtk_sys_heap, heap);
	for (i = 0; i < NUM_ORDERS; i++)
		total_pages += sys_page_pool_nr_pages(sys_heap->pools[i]);

	return total_pages;
}

static struct ion_heap_ops mtk_sys_heap_ops = {
	.allocate = mtk_sys_heap_allocate,
	.free = mtk_sys_heap_free,
	.shrink = mtk_sys_heap_shrink,
	.get_pool_size = mtk_sys_get_pool_size,
};

static void mtk_sys_heap_destroy_pools(struct sys_page_pool **pools)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++)
		if (pools[i])
			sys_page_pool_destroy(pools[i]);
}

static int mtk_sys_heap_create_pools(struct sys_page_pool **pools)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++) {
		struct sys_page_pool *pool;
		gfp_t gfp_flags = sys_low_order_gfp_flags;

		if (orders[i] > 0)
			gfp_flags = sys_high_order_gfp_flags;

		pool = sys_page_pool_create(gfp_flags, orders[i]);
		if (!pool)
			goto err_create_pool;
		pools[i] = pool;
	}
	return 0;

err_create_pool:
	mtk_sys_heap_destroy_pools(pools);
	return -ENOMEM;
}

static struct mtk_sys_heap sys_heap = {
	.heap = {
		.ops = &mtk_sys_heap_ops,
		.type = ION_HEAP_TYPE_CUSTOM,
		.name = MTK_ION_SYS_HEAP_NAME,
	}
};

int mtk_sys_heap_init(struct device *dev)
{
	int ret;

	ret = mtk_sys_heap_create_pools(sys_heap.pools);
	if (ret) {
		pr_err("%s,iommu create pools fail\n", __func__);
		return ret;
	}
	sys_heap.priv = dev;

	return ion_device_add_heap(&sys_heap.heap);
}

void mtk_sys_heap_exit(void)
{
	ion_device_remove_heap(&sys_heap.heap);
	mtk_sys_heap_destroy_pools(sys_heap.pools);
	sys_heap.priv = NULL;
}
