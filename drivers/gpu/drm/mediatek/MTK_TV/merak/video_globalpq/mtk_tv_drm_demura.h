/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_DEMURA_H_
#define _MTK_TV_DRM_DEMURA_H_
#include <drm/mtk_tv_drm.h>
#include "mtk_tv_drm_metabuf.h"

//-------------------------------------------------------------------------------------------------
//  Defines
//-------------------------------------------------------------------------------------------------
#define MTK_DEMURA_SLOT_NUM_MAX		(2)

//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
struct mtk_demura_slot {
	void    *bin_va;
	uint64_t bin_pa;
	uint32_t bin_len;
	bool     disable;
	uint8_t  id;
};

struct mtk_demura_context {
	bool   init;
	struct mtk_tv_drm_metabuf metabuf;
	struct mtk_demura_slot config_slot[MTK_DEMURA_SLOT_NUM_MAX];
	uint32_t bin_max_size;
	uint8_t  slot_idx;
};

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
extern bool bPquEnable; // is pqurv55 enable ? declare at mtk_tv_drm_drv.c.

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_tv_drm_demura_init(
	struct mtk_demura_context *demura);

int mtk_tv_drm_demura_deinit(
	struct mtk_demura_context *demura);

int mtk_tv_drm_demura_set_config(
	struct mtk_demura_context *demura_ctx,
	struct drm_mtk_demura_config *config);

#endif //_MTK_TV_DRM_DEMURA_H_
