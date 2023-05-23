/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#ifndef _MTK_IOMMU_OF_H_
#define _MTK_IOMMU_OF_H_

#include <linux/list.h>
#include <linux/types.h>
#include <linux/errno.h>

#include "mtk_iommu_common.h"

struct buf_tag_info {
	uint32_t id;
	uint32_t heap_type;
	uint32_t miu;
	uint32_t zone;
	uint64_t maxsize;
	char name[MAX_NAME_SIZE];
	struct list_head list;
};

struct reserved_info {
	struct list_head list;
	struct mdiommu_reserve_iova reservation;
};

struct lx_range_node {
	struct list_head list;
	uint64_t start;
	uint64_t length;
};

int mtk_iommu_get_buftag_info(struct buf_tag_info *info);
struct list_head *mtk_iommu_get_buftags(void);
int mtk_iommu_buftag_check(unsigned long buf_tag_id);
int mtk_iommu_get_reserved(struct list_head *head);
int mtk_iommu_get_memoryinfo(uint64_t *addr0, uint64_t *size0,
		uint64_t *addr1, uint64_t *size1);

#endif
