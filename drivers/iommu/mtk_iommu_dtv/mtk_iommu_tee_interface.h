/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 *
 */

#ifndef _MTK_IOMMU_TEE_INTERFACE_H_
#define _MTK_IOMMU_TEE_INTERFACE_H_
#include <linux/types.h>
#include "mtk_iommu_common.h"

struct buf_tag {
	uint32_t heap_type;
	uint32_t miu;
	uint64_t maxsize;
	char name[64];
};

struct tee_map_lock {
	uint32_t aid[8];
	uint32_t aid_num;
	char buffer_tag[MAX_NAME_SIZE];
};

int mtk_iommu_tee_map_page(enum mtk_iommu_addr_type type,
		const char *space_tag, struct page *page,
		size_t size, unsigned long offset, bool secure, u64 *va_out);
int mtk_iommu_tee_map(const char *space_tag, struct sg_table *sgt,
		      u64 *va_out, bool dou, const char *buf_tag);
int mtk_iommu_tee_unmap(enum mtk_iommu_addr_type type, uint64_t va,
			struct mtk_iommu_range_t *va_out);
int mtk_iommu_tee_set_mpu_area(enum mtk_iommu_addr_type type,
		struct mtk_iommu_range_t *range, uint32_t num);
int mtk_iommu_tee_authorize(u64 va, u32 buffer_size, const char *buf_tag,
			u32 pipe_id);
int mtk_iommu_tee_unauthorize(u64 va, u32 buffer_size, const char *buf_tag,
			uint32_t pipe_id, struct mtk_iommu_range_t *rang_out);
int mtk_iommu_get_pipeid(int *pipe_id);
int mtk_iommu_tee_reserve_space(enum mtk_iommu_addr_type type,
		const char *space_tag, u64 size, u64 *va_out);
int mtk_iommu_tee_free_space(enum mtk_iommu_addr_type type,
		const char *space_tag);
int mtk_iommu_optee_ta_store_buf_tags(void);
int mtk_iommu_tee_debug(uint32_t debug_type, uint8_t *buf1, uint8_t size1,
	uint8_t *buf2, uint8_t size2, uint8_t *buf3, uint8_t size3);
int mtk_iommu_tee_get_pipelineid(uint32_t *id);
int mtk_iommu_tee_free_pipelineid(uint32_t id);
int mtk_iommu_tee_close_session(void);
int mtk_iommu_tee_open_session(void);
int mtk_iommu_tee_init(void);
int mtk_iommu_tee_lockdebug(struct tee_map_lock *lock);
int mtk_iommu_tee_test(struct sg_table *sgt, const char *buf_tag);

#endif
