/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */


#ifndef _MTK_ZS_MALLOC_H_
#define _MTK_ZS_MALLOC_H_

#include <linux/types.h>

/*
 * zsmalloc mapping modes
 *
 * NOTE: These only make a difference when a mapped object spans pages.
 * They also have no effect when PGTABLE_MAPPING is selected.
 */
enum mtk_zs_mapmode {
	ZS_MM_RW, /* normal read-write mapping */
	ZS_MM_RO, /* read-only (no copy-out at unmap time) */
	ZS_MM_WO /* write-only (no copy-in at map time) */
	/*
	 * NOTE: ZS_MM_WO should only be used for initializing new
	 * (uninitialized) allocations.  Partial writes to already
	 * initialized allocations should use ZS_MM_RW to preserve the
	 * existing data.
	 */
};


struct mtk_zs_pool_stats {
	/* How many pages were migrated (freed) */
	unsigned long pages_compacted;
};

struct mtk_zs_pool;

struct mtk_zs_pool *mtk_zs_create_pool(const char *name);
void mtk_zs_destroy_pool(struct mtk_zs_pool *pool);

unsigned long mtk_zs_malloc(struct mtk_zs_pool *pool, size_t size, gfp_t flags);
void mtk_zs_free(struct mtk_zs_pool *pool, unsigned long obj);

size_t mtk_zs_huge_class_size(struct mtk_zs_pool *pool);

void *mtk_zs_map_object(struct mtk_zs_pool *pool, unsigned long handle,
			enum mtk_zs_mapmode mm, phys_addr_t *in_addr0, phys_addr_t *in_addr1,
			phys_addr_t *in_addr2, bool is_mzc);

void mtk_zs_unmap_object(struct mtk_zs_pool *pool, unsigned long handle,
			enum mtk_zs_mapmode mm, bool is_mzc);

unsigned long mtk_zs_get_total_pages(struct mtk_zs_pool *pool);
unsigned long mtk_zs_compact(struct mtk_zs_pool *pool);

void mtk_zs_pool_stats(struct mtk_zs_pool *pool,
		struct mtk_zs_pool_stats *stats);
#endif
