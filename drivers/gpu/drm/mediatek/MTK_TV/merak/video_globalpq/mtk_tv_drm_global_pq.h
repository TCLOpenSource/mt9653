/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_GLOBAL_PQ_H_
#define _MTK_TV_DRM_GLOBAL_PQ_H_


//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
struct mtk_global_pq_context {
	char *pq_string;
};

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
struct mtk_tv_drm_crtc;

void mtk_global_pq_atomic_crtc_flush(struct mtk_global_pq_context *pctx);

int mtk_global_pq_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val, int prop_index);

int mtk_global_pq_atomic_get_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val,
	int prop_index);

#endif
