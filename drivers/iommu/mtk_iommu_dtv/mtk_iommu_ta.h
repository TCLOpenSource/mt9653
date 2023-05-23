/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#ifndef TA_MMA_MANAGER_H
#define TA_MMA_MANAGER_H

#define MAP_IOVA                     0x01
#define UNMAP_IOVA                   0x02

#define VA_AUTHORIZE                 0x04

#define RESERVE_IOVA_SPACE           0x05

#define SET_MPU_AREA                 0x06

#define FREE_IOVA_SPACE              0x07


#define DEBUG                        0x08
#define VA_UNAUTHORIZE               0x09
#define STORE_BUF_TAGS               0x0A
#define GENERATE_PIPELINE_ID         0x0B
#define RELEASE_PIPELINE_ID          0x0C
#define TEE_MMA_INIT                 0x0D
#define GET_UUID_BY_PIPELINE_ID      0x0E
#define LOCK_DEBUG                   0x0F
#define IOMMU_TEST                   0x10


#define MMA_FLAG_2XIOVA              (1 << 0)//mapping double size iova
#define MMA_FLAG_RESIZE              (1 << 1)//buffer reallocate

struct tee_map_args_v2 {
	uint32_t type;
	uint32_t is_secure;
	uint64_t pa_num;
	uint64_t size;
	uint32_t flag;
	uint64_t addr; //for buffer reallocate,old buffer iova
	char buffer_tag[16];
};

enum tee_mma_debug_type {
	E_MMA_IOMMU_DEBUG_INFO,
	E_MMA_DEBUG_MAX
};
#endif

