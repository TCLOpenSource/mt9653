/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_PQU_METADATA_H_
#define _MTK_TV_DRM_PQU_METADATA_H_
#include <drm/mtk_tv_drm.h>
#include "mtk_tv_drm_metabuf.h"
#include "mtk_tv_drm_video_display.h"

//-------------------------------------------------------------------------------------------------
//  Defines
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
enum mtk_pqu_metadata_attr {
	MTK_PQU_METADATA_ATTR_GLOBAL_FRAME_ID,
	MTK_PQU_METADATA_ATTR_RENDER_MODE,
};

struct mtk_pqu_metadata_render_setting {
	uint32_t render_mode;
};

struct mtk_pqu_metadata_context {
	bool init;
	struct mtk_tv_drm_metabuf pqu_metabuf;
	struct mtk_pqu_metadata_render_setting render_setting;
	struct drm_mtk_tv_pqu_metadata pqu_metadata;
	struct drm_mtk_tv_pqu_metadata *pqu_metadata_ptr[MTK_TV_DRM_PQU_METADATA_COUNT];
	uint8_t pqu_metadata_ptr_index;
};

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
extern bool bPquEnable; // is pqurv55 enable ? declare at mtk_tv_drm_drv.c.

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_tv_drm_pqu_metadata_init(
	struct mtk_pqu_metadata_context *context);

int mtk_tv_drm_pqu_metadata_deinit(
	struct mtk_pqu_metadata_context *context);

int mtk_tv_drm_pqu_metadata_set_attr(
	struct mtk_pqu_metadata_context *context,
	enum mtk_pqu_metadata_attr attr,
	void *value);

int mtk_tv_drm_pqu_metadata_add_window_setting(
	struct mtk_pqu_metadata_context *context,
	struct drm_mtk_tv_pqu_window_setting *window_setting);

int mtk_tv_drm_pqu_metadata_fire_window_setting(
	struct mtk_pqu_metadata_context *context);

int mtk_tv_drm_pqu_metadata_get_copy_ioctl(
	struct drm_device *drm,
	void *data,
	struct drm_file *file_priv);

#endif //_MTK_TV_DRM_PQU_METADATA_H_
