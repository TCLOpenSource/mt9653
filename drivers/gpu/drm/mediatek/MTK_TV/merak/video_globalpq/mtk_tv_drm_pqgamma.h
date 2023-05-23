/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_PQGAMMA_H_
#define _MTK_TV_DRM_PQGAMMA_H_

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
int mtk_video_pqgamma_set(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pqgamma_curve *curve);

int mtk_video_pqgamma_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_enable *pnlgamma_enable);

int mtk_video_pqgamma_gainoffset(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_gainoffset *gainoffset);

int mtk_video_pqgamma_gainoffset_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_gainoffset_enable *pnl_gainoffsetenable);

int mtk_video_pqgamma_set_maxvalue(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_maxvalue *maxvalue);

int mtk_video_pqgamma_maxvalue_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_maxvalue_enable *maxvalue_enable);

#endif
