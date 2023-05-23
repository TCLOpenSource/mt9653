/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/* * Copyright (c) 2020 MediaTek Inc. */

#ifndef _MTK_DRM_KMS_H_
#define _MTK_DRM_KMS_H_

#include <drm/drm_encoder.h>

#include "mtk_tv_drm_gop.h"
#include "mtk_tv_drm_video.h"
#include "mtk_tv_drm_video_display.h"
#include "mtk_tv_drm_encoder.h"
#include "mtk_tv_drm_ld.h"

#define MTK_DRM_TV_KMS_COMPATIBLE_STR "MTK-drm-tv-kms"
#define MTK_DRM_DTS_GOP_PLANE_NUM_TAG             "GRAPHIC_PLANE_NUM"
#define MTK_DRM_DTS_GOP_PLANE_INDEX_START_TAG     "GRAPHIC_PLANE_INDEX_START"

struct property_enum_info {
	struct drm_prop_enum_list *enum_list;
	int enum_length;
};

struct property_range_info {
	uint64_t min;
	uint64_t max;
};

struct ext_prop_info {
	char *prop_name;
	int flag;
	uint64_t init_val;
	union{
		struct property_enum_info enum_info;
		struct property_range_info range_info;
	};
};

enum ext_prop_type {
	E_EXT_PROP_TYPE_COMMON_PLANE = 0,
	E_EXT_PROP_TYPE_CRTC,
	E_EXT_PROP_TYPE_CONNECTOR,
	E_EXT_PROP_TYPE_MAX,
};

enum {
	E_EXT_COMMON_PLANE_PROP_TYPE = 0,
	E_EXT_COMMON_PLANE_PROP_MAX,
};

// need to update kms.c get/set property function after adding new properties
enum {
	// pnl property
	E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID,
	E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE,
	E_EXT_CRTC_PROP_LOW_LATENCY_MODE,
	E_EXT_CRTC_PROP_CUSTOMIZE_FRC_TABLE,
	E_EXT_CRTC_PROP_PANEL_TIMING,
	E_EXT_CRTC_PROP_SET_FREERUN_TIMING,
	E_EXT_CRTC_PROP_FORCE_FREERUN,
	E_EXT_CRTC_PROP_FREERUN_VFREQ,
	E_EXT_CRTC_PROP_VIDEO_LATENCY,
	// ldm property
	E_EXT_CRTC_PROP_LDM_STATUS,
	E_EXT_CRTC_PROP_LDM_LUMA,
	E_EXT_CRTC_PROP_LDM_ENABLE,
	E_EXT_CRTC_PROP_LDM_STRENGTH,
	E_EXT_CRTC_PROP_LDM_DATA,
	E_EXT_CRTC_PROP_LDM_DEMO_PATTERN,
	// pixelshift property
	E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACHED,
	E_EXT_CRTC_PROP_PIXELSHIFT_HRANGE,
	E_EXT_CRTC_PROP_PIXELSHIFT_VRANGE,
	E_EXT_CRTC_PROP_PIXELSHIFT_TYPE,
	E_EXT_CRTC_PROP_PIXELSHIFT_H,
	E_EXT_CRTC_PROP_PIXELSHIFT_V,
	// memc property
	E_EXT_CRTC_PROP_MEMC_PLANE_ID,
	E_EXT_CRTC_PROP_MEMC_LEVEL,
	E_EXT_CRTC_PROP_MEMC_GAME_MODE,
	E_EXT_CRTC_PROP_MEMC_FIRSTFRAME_READY,
	E_EXT_CRTC_PROP_MEMC_DECODE_IDX_DIFF,
	E_EXT_CRTC_PROP_MEMC_MISC_A_TYPE,
	E_EXT_CRTC_PROP_MEMC_CHIP_CAPS,
	E_EXT_CRTC_PROP_MEMC_GET_STATUS,
	// total property
	E_EXT_CRTC_PROP_MAX,
};

enum {
	E_EXT_CONNECTOR_PROP_PNL_DBG_LEVEL,
	E_EXT_CONNECTOR_PROP_PNL_OUTPUT,
	E_EXT_CONNECTOR_PROP_PNL_SWING_LEVEL,
	E_EXT_CONNECTOR_PROP_PNL_FORCE_PANEL_DCLK,
	E_EXT_CONNECTOR_PROP_PNL_PNL_FORCE_PANEL_HSTART,
	E_EXT_CONNECTOR_PROP_PNL_PNL_OUTPUT_PATTERN,
	E_EXT_CONNECTOR_PROP_PNL_MIRROR_STATUS,
	E_EXT_CONNECTOR_PROP_PNL_SSC_EN,
	E_EXT_CONNECTOR_PROP_PNL_SSC_FMODULATION,
	E_EXT_CONNECTOR_PROP_PNL_SSC_RDEVIATION,
	E_EXT_CONNECTOR_PROP_PNL_OVERDRIVER_ENABLE,
	E_EXT_CONNECTOR_PROP_PNL_PANEL_SETTING,
	E_EXT_CONNECTOR_PROP_PNL_PANEL_INFO,
	E_EXT_CONNECTOR_PROP_MAX,
};

struct ext_common_plane_prop_info {
	uint64_t prop_val[E_EXT_COMMON_PLANE_PROP_MAX];
};

struct ext_crtc_prop_info {
	uint64_t prop_val[E_EXT_CRTC_PROP_MAX];
	uint8_t prop_update[E_EXT_CRTC_PROP_MAX];
};

struct ext_connector_prop_info {
	uint64_t prop_val[E_EXT_CONNECTOR_PROP_MAX];
};

struct mtk_drmcap_dev {
	u32 u32MEMC_NUM;
	u32 u32MGW_NUM;
	u32 u32DMA_SCL_NUM;
	u32 u32WindowNUM;
	u8  u8PanelGammaBit;
	u8  u8ODSupport;
	u8  u8OLEDDemuraInSupport;
	u8  u8OLEDBoostingSupport;
};

struct mtk_tv_kms_context {
	// common
	struct device *dev;
	struct drm_device *drm_dev;
	struct mtk_tv_drm_crtc crtc;
	struct drm_encoder encoder;
	struct mtk_tv_drm_connector connector;
	int graphic_irq_num;
	int plane_num[MTK_DRM_PLANE_TYPE_MAX];    // should read from dts
	int plane_index_start[MTK_DRM_PLANE_TYPE_MAX]; // should read from dts
	int total_plane_num;
	struct drm_property *drm_common_plane_prop[E_EXT_COMMON_PLANE_PROP_MAX];
	struct ext_common_plane_prop_info *ext_common_plane_properties;
	struct drm_property *drm_crtc_prop[E_EXT_CRTC_PROP_MAX];
	struct ext_crtc_prop_info *ext_crtc_properties;
	struct drm_property *drm_connector_prop[E_EXT_CONNECTOR_PROP_MAX];
	struct ext_connector_prop_info *ext_connector_properties;
	// gop
	struct mtk_gop_context gop_priv;
	// video
	struct mtk_video_context video_priv;
	// ld
	struct mtk_ld_context ld_priv;
	// panel
	struct mtk_panel_context panel_priv;
	//HW capability
	struct mtk_drmcap_dev drmcap_dev;
};

#endif
