/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_PNLGAMMA_H_
#define _MTK_TV_DRM_PNLGAMMA_H_

#include <linux/dma-buf.h>

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
extern bool bPquEnable; // is pqurv55 enable ? declare at mtk_tv_drm_drv.c.
//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_video_panel_gamma_setting(
	struct drm_crtc *crtc,
	uint16_t *r,
	uint16_t *g,
	uint16_t *b,
	uint32_t size);

int mtk_video_panel_gamma_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pnlgamma_enable *pnlgamma_enable);

int mtk_video_panel_gamma_gainoffset(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pnlgamma_gainoffset *gainoffset);

int mtk_video_panel_gamma_gainoffset_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pnlgamma_gainoffset_enable *pnl_gainoffsetenable);

#endif
