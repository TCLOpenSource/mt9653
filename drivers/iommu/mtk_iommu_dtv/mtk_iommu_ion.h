/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#ifndef _MTK_IOMMU_ION_H_
#define _MTK_IOMMU_ION_H_

#include "mtk_iommu_common.h"
#include <linux/ion.h>

static inline void _update_gfp_mask(gfp_t *gfp_mask)
{
	*gfp_mask &= ~__GFP_HIGHMEM;
#ifdef CONFIG_ZONE_DMA32
	*gfp_mask |= GFP_DMA32;
#elif defined(CONFIG_ZONE_DMA)
	*gfp_mask |= GFP_DMA;
#else
	*gfp_mask |= __GFP_HIGHMEM;
#endif
}

extern size_t ion_query_heaps_kernel(struct ion_heap_data *hdata, size_t size);
extern struct dma_buf *ion_alloc(size_t len, unsigned int heap_id_mask,
				unsigned int flags);
int mtk_iommu_ion_query_heap(void);
int mtk_iommu_get_id_flag(int heap_type, int miu, int zone_flag,
		bool secure, unsigned int *heap_mask, unsigned int *ion_flag,
		u32 size, const char *buf_tag, unsigned long attrs);

int mtk_iommu_ion_alloc(size_t size, unsigned int heap_mask,
			unsigned int flags, struct dma_buf **db);


#endif
