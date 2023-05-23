/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_IOMMU_STAT_H_
#define __MTK_IOMMU_STAT_H_
#include "mtk_iommu_dtv.h"

struct mem_statistics {
	char buftag[MAX_NAME_SIZE];
	atomic64_t total_sz;
	atomic64_t dma_sz;		/* DMA Zone */
	atomic64_t others_sz;	/* Other Zone */
	atomic64_t num_entry;
};

struct mem_statistics *get_buf_tag_statistics(const char *buf_tag);

#endif
