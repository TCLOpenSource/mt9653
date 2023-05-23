/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_VIDEO_PIXELSHIFT_H_
#define _MTK_TV_DRM_VIDEO_PIXELSHIFT_H_
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include "../mtk_tv_drm_crtc.h"
#include "hwreg_render_video_pixelshift.h"

struct mtk_pixelshift_context {
	bool    bIsOLEDPixelshiftEnable;
	bool	bIsOLEDPixelshiftOsdAttached;
	uint8_t u8OLEDPixleshiftType;
	int8_t  i8OLEDPixelshiftHRangeMax;
	int8_t  i8OLEDPixelshiftHRangeMin;
	int8_t  i8OLEDPixelshiftVRangeMax;
	int8_t  i8OLEDPixelshiftVRangeMin;
	int8_t  i8OLEDPixelshiftHDirection;
	int8_t  i8OLEDPixelshiftVDirection;
};


int mtk_video_pixelshift_init(struct device *dev);
int mtk_video_pixelshift_suspend(struct platform_device *pdev);
int mtk_video_pixelshift_resume(struct platform_device *pdev);
int mtk_video_pixelshift_atomic_set_crtc_property(
	 struct mtk_tv_drm_crtc *crtc,
	 struct drm_crtc_state *state,
	 struct drm_property *property,
	 uint64_t val,
	 int prop_index);
int mtk_video_pixelshift_atomic_get_crtc_property(
	 struct mtk_tv_drm_crtc *crtc,
	 const struct drm_crtc_state *state,
	 struct drm_property *property,
	 uint64_t *val,
	 int prop_index);
void mtk_video_pixelshift_atomic_crtc_flush(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state);
#endif
