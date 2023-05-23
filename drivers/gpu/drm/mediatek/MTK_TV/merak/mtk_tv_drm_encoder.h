/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_DRM_ENCODER_H_
#define _MTK_DRM_ENCODER_H_

#include "mtk_tv_drm_drv.h"

int mtk_tv_drm_encoder_create(struct drm_device *dev,
struct drm_encoder *encoder, struct mtk_tv_drm_crtc *mcrtc);

#endif
