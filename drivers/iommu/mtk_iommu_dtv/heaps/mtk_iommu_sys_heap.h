/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */


#ifndef _LINUX_MTK_IOMMU_SYS_HEAP_H
#define _LINUX_MTK_IOMMU_SYS_HEAP_H


#define MTK_ION_SYS_HEAP_NAME "mtkdtv_sys_heap"
/**
 * struct sys_page_pool - pagepool struct
 * @high_count:		number of highmem items in the pool
 * @low_count:		number of lowmem items in the pool
 * @high_items:		list of highmem items
 * @low_items:		list of lowmem items
 * @mutex:		lock protecting this struct and especially the count
 *			item list
 * @gfp_mask:		gfp_mask to use from alloc
 * @order:		order of pages in the pool
 * @list:		plist node for list of pools
 *
 * Allows you to keep a pool of pre allocated pages to use from your heap.
 * Keeping a pool of pages that is ready for dma, ie any cached mapping have
 * been invalidated from the cache, provides a significant performance benefit
 * on many systems
 */
struct sys_page_pool {
	int high_count;
	int normal_count;
	int dma_count;
	struct list_head high_items;
	struct list_head normal_items;
	struct list_head dma_items;
	struct mutex mutex;
	gfp_t gfp_mask;
	unsigned int order;
	struct plist_node list;
};

enum mtk_sys_pool_type {
	POOL_TYPE_DMA,
	POOL_TYPE_NORMAL,
	POOL_TYPE_HIGH,
	POOL_TYPE_MAX,
};

int mtk_sys_heap_init(struct device *dev);
void mtk_sys_heap_exit(void);
#endif
