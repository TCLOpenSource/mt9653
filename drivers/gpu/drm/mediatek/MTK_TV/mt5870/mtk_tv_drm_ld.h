/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_DRM_LD_H_
#define _MTK_DRM_LD_H_

#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_crtc.h"




#define MTK_PLANE_DISPLAY_PIPE (3)



struct mtk_ld_context {

	__u32 u8LDMSupport;
	__u64 u64LDM_phyAddress;
	__u8 u8LDM_Version;

};

int mtk_ld_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val,
	int prop_index);
int mtk_ld_atomic_get_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val,
	int prop_index);


int readDTB2LDMPrivate(struct mtk_ld_context *pctx);



int mtk_ldm_init(struct device *dev);


#endif
