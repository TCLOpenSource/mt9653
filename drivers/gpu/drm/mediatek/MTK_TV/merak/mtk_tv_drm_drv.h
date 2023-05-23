/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_DRV_H_
#define _MTK_TV_DRM_DRV_H_

#include <drm/drmP.h>

#define MTK_TV_MAX_CRTC  (2)

struct mtk_tv_drm_private {
	struct drm_device *drm;
	struct device *dev;

	__u32 bus_offset;
	__u32 recovery_buf_tag;
};

#endif
