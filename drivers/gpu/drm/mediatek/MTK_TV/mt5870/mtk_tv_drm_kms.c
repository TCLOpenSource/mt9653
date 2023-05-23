// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/of_irq.h>
#include <drm/mtk_tv_drm.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>

#include "mtk_tv_drm_plane.h"
#include "mtk_tv_drm_drv.h"
#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_gop.h"
#include "mtk_tv_drm_encoder.h"
#include "mtk_tv_drm_kms.h"

#define PANEL_PROP(i) \
		((i >= E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID) && \
			(i <= E_EXT_CRTC_PROP_VIDEO_LATENCY)) \

#define PIXELSHIFT_PROP(i) \
		((i >= E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACHED) && \
			(i <= E_EXT_CRTC_PROP_PIXELSHIFT_V)) \

#define MEMC_PROP(i) \
		((i >= E_EXT_CRTC_PROP_MEMC_PLANE_ID) && \
			(i <= E_EXT_CRTC_PROP_MEMC_GET_STATUS)) \

#define LDM_PROP(i) \
		((i >= E_EXT_CRTC_PROP_LDM_STATUS) && \
			(i <= E_EXT_CRTC_PROP_LDM_DEMO_PATTERN))\

#define MTK_DRM_KMS_ATTR_MODE     (0600)
#define DRM_PLANE_SCALE_CAPBILITY (1<<4)

static struct drm_prop_enum_list plane_type_enum_list[] = {
	{.type = MTK_DRM_PLANE_TYPE_GRAPHIC,
		.name = MTK_PLANE_PROP_PLANE_TYPE_GRAPHIC},
	{.type = MTK_DRM_PLANE_TYPE_VIDEO,
		.name = MTK_PLANE_PROP_PLANE_TYPE_VIDEO},
	{.type = MTK_DRM_PLANE_TYPE_PRIMARY,
		.name = MTK_PLANE_PROP_PLANE_TYPE_PRIMARY},
};

static const struct ext_prop_info ext_common_plane_props_def[] = {
	{
		.prop_name = MTK_PLANE_PROP_PLANE_TYPE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &plane_type_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(plane_type_enum_list),
		.init_val = 0,
	},
};

static struct drm_prop_enum_list ldm_status_enum_list[] = {
	{.type = MTK_CRTC_LDM_STATUS_INIT,
		.name = MTK_CRTC_PROP_LDM_STATUS_INIT},
	{.type = MTK_CRTC_LDM_STATUS_ENABLE,
		.name = MTK_CRTC_PROP_LDM_STATUS_ENABLE},
	{.type = MTK_CRTC_LDM_STATUS_DISABLE,
		.name = MTK_CRTC_PROP_LDM_STATUS_DISABLE},
	{.type = MTK_CRTC_LDM_STATUS_DEINIT,
		.name = MTK_CRTC_PROP_LDM_STATUS_DEINIT},
};


static struct drm_prop_enum_list video_pixelshift_osd_attached_enum_list[] = {
	{.type = VIDEO_CRTC_PIXELSHIFT_OSD_DETACHED,
		.name = MTK_VIDEO_PROP_PIXELSHIFT_OSD_DETACHED},
	{.type = VIDEO_CRTC_PIXELSHIFT_OSD_ATTACHED,
		.name = MTK_VIDEO_PROP_PIXELSHIFT_OSD_ATTACHED},
};

static struct drm_prop_enum_list video_pixelshift_type_enum_list[] = {
	{.type = VIDEO_CRTC_PIXELSHIFT_DO_JUSTSCAN,
		.name = MTK_VIDEO_PROP_PIXELSHIFT_DO_JUSTSCAN},
	{.type = VIDEO_CRTC_PIXELSHIFT_DO_OVERSCAN,
		.name = MTK_VIDEO_PROP_PIXELSHIFT_DO_OVERSCAN},
};

static struct drm_prop_enum_list video_memc_misc_a_type_enum_list[] = {
	{.type = VIDEO_CRTC_MISC_A_NULL,
		.name = MTK_VIDEO_PROP_MEMC_MISC_A_NULL},
	{.type = VIDEO_CRTC_MISC_A_MEMC_INSIDE,
		.name = MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE},
	{.type = VIDEO_CRTC_MISC_A_MEMC_INSIDE_60HZ,
		.name = MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_60HZ},
	{.type = VIDEO_CRTC_MISC_A_MEMC_INSIDE_240HZ,
		.name = MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_240HZ},
	{.type = VIDEO_CRTC_MISC_A_MEMC_INSIDE_4K1K_120HZ,
		.name = MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_4K1K_120HZ},
	{.type = VIDEO_CRTC_MISC_A_MEMC_INSIDE_KEEP_OP_4K2K,
		.name = MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_KEEP_OP_4K2K},
	{.type = VIDEO_CRTC_MISC_A_MEMC_INSIDE_4K_HALFK_240HZ,
		.name = MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_4K_HALFK_240HZ},
};

static struct drm_prop_enum_list video_memc_caps_enum_list[] = {
	{.type = VIDEO_CRTC_MEMC_CAPS_NOT_SUPPORTED,
		.name = MTK_VIDEO_PROP_MEMC_CHIP_CAPS_NOT_SUPPORTED},
	{.type = VIDEO_CRTC_MEMC_CAPS_SUPPORTED,
		.name = MTK_VIDEO_PROP_MEMC_CHIP_CAPS_SUPPORTED},
};

static struct drm_prop_enum_list video_framesync_mode_enum_list[] = {
	{.type = VIDEO_CRTC_FRAME_SYNC_FREERUN, .name = "framesync_freerun"},
	{.type = VIDEO_CRTC_FRAME_SYNC_VTTV, .name = "framesync_vttv"},
	{.type = VIDEO_CRTC_FRAME_SYNC_FRAMELOCK, .name = "framesync_framelock1to1"},
	{.type = VIDEO_CRTC_FRAME_SYNC_VTTPLL, .name = "framesync_vttpll"},
	{.type = VIDEO_CRTC_FRAME_SYNC_MAX, .name = "framesync_max"},
};

static const struct ext_prop_info ext_crtc_props_def[] = {
	/*****panel related properties*****/
	{
		//get framesync frame id
		.prop_name = MTK_CRTC_PROP_FRAMESYNC_PLANE_ID,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		// set frame sync mode setting
		.prop_name = MTK_CRTC_PROP_FRAMESYNC_MODE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_framesync_mode_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(video_framesync_mode_enum_list),
		.init_val = 0,
	},
	{
		// low latency
		.prop_name = MTK_CRTC_PROP_LOW_LATENCY_MODE,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 1,
		.init_val = 0,
	},
	{
		//customize frc table
		.prop_name = MTK_CRTC_PROP_FRC_TABLE,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_DISPLAY_TIMING_INFO,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_FREERUN_TIMING,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_FORCE_FREERUN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_FREERUN_VFREQ,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_VIDEO_LATENCY,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	/****LDM crtc prop******/
	{
		.prop_name = MTK_CRTC_PROP_LDM_STATUS,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &ldm_status_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(ldm_status_enum_list),
		//  .flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		//  .range_info.min = 0,
		//  .range_info.max = 5,
		.init_val = MTK_CRTC_LDM_STATUS_DEINIT,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_LUMA,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_ENABLE,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 1,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_STRENGTH,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_DATA,
		.flag = DRM_MODE_PROP_RANGE  | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_DEMO_PATTERN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFFFFFF,
		.init_val = 0,
	},
	/****pixelshift crtc prop******/
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_OSD_ATTACHED,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list =
			&video_pixelshift_osd_attached_enum_list[0],
		.enum_info.enum_length =
			ARRAY_SIZE(video_pixelshift_osd_attached_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_HRANGE,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = PIXELSHIFT_TYPE_JUSTSCAN_H_MAX,
		.init_val = PIXELSHIFT_TYPE_JUSTSCAN_H_MAX,
	},
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_VRANGE,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = PIXELSHIFT_TYPE_JUSTSCAN_V_MAX,
		.init_val = PIXELSHIFT_TYPE_JUSTSCAN_V_MAX,
	},
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_TYPE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_pixelshift_type_enum_list[0],
		.enum_info.enum_length =
			ARRAY_SIZE(video_pixelshift_type_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_H,
		.flag = DRM_MODE_PROP_SIGNED_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = -PIXELSHIFT_TYPE_JUSTSCAN_H_MAX,
		.range_info.max = PIXELSHIFT_TYPE_JUSTSCAN_H_MAX,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_V,
		.flag = DRM_MODE_PROP_SIGNED_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = -PIXELSHIFT_TYPE_JUSTSCAN_V_MAX,
		.range_info.max = PIXELSHIFT_TYPE_JUSTSCAN_V_MAX,
		.init_val = 0,
	},
	/****MEMC crtc prop******/
	{
		.prop_name = MTK_CRTC_PROP_MEMC_PLANE_ID,
		//.flag = DRM_MODE_PROP_OBJECT | DRM_MODE_PROP_ATOMIC,
		.flag = DRM_MODE_PROP_ATOMIC,
		//.range_info.min = 0,
		//.range_info.max = MTK_MFC_PLANE_ID_MAX,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_MEMC_MODE,
		.flag = DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_RANGE,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_MEMC_GAME_MODE,
		.flag = DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_RANGE,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_MEMC_FIRSTFRAME_READY,
		.flag = DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_RANGE,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_MEMC_DECODE_IDX_DIFF,
		.flag = DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_RANGE,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_MEMC_MISC_A_TYPE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_memc_misc_a_type_enum_list[0],
		.enum_info.enum_length =
			ARRAY_SIZE(video_memc_misc_a_type_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_MEMC_CHIP_CAPS,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_memc_caps_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(video_memc_caps_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_MEMC_GET_STATUS,
		.flag = DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_RANGE,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
};

/*connector properties*/
static const struct ext_prop_info ext_connector_props_def[] = {
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_DBG_LEVEL,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_OUTPUT,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_SWING_LEVEL,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_FORCE_PANEL_DCLK,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFFFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_FORCE_PANEL_HSTART,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_OUTPUT_PATTERN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_MIRROR_STATUS,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_SSC_EN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 1,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_SSC_FMODULATION,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_SSC_RDEVIATION,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
	.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_OVERDRIVER_ENABLE,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_PANEL_SETTING,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_INFO,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
};

struct mtk_drmcap_dev gDRMcap_capability;

static void _mtk_drm_get_capability(
	struct mtk_tv_kms_context *drm_kms_dev)
{
	memset(&gDRMcap_capability, 0, sizeof(gDRMcap_capability));

	gDRMcap_capability.u32MEMC_NUM     =
		drm_kms_dev->drmcap_dev.u32MEMC_NUM;
	gDRMcap_capability.u32MGW_NUM      =
		drm_kms_dev->drmcap_dev.u32MGW_NUM;
	gDRMcap_capability.u32DMA_SCL_NUM  =
		drm_kms_dev->drmcap_dev.u32DMA_SCL_NUM;
	gDRMcap_capability.u32WindowNUM   =
		drm_kms_dev->drmcap_dev.u32WindowNUM;
	gDRMcap_capability.u8PanelGammaBit =
		drm_kms_dev->drmcap_dev.u8PanelGammaBit;
	gDRMcap_capability.u8ODSupport =
		drm_kms_dev->drmcap_dev.u8ODSupport;
	gDRMcap_capability.u8OLEDDemuraInSupport =
		drm_kms_dev->drmcap_dev.u8OLEDDemuraInSupport;
	gDRMcap_capability.u8OLEDBoostingSupport =
		drm_kms_dev->drmcap_dev.u8OLEDBoostingSupport;
}

static ssize_t mtk_drm_capability_show_DMA_SCL_NUM(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = 65535;

	//DMA/SCL
	if (ssize > u16Size)
		DRM_ERROR("Failed to get DMA/SCL\n");

	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u32DMA_SCL_NUM);
		DRM_INFO("gDRMcap_capability.u32DMA_SCL:%llx\n",
			gDRMcap_capability.u32DMA_SCL_NUM);
	}
	return ssize;
}

static ssize_t mtk_drm_capability_show_MGW_NUM(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = 65535;

	//MGW
	if (ssize > u16Size)
		DRM_ERROR("Failed to get MGW\n");

	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u32MGW_NUM);
		DRM_INFO("gDRMcap_capability.u32MGW_NUM:%llx\n",
			gDRMcap_capability.u32MGW_NUM);
	}
	return ssize;
}

static ssize_t mtk_drm_capability_show_MEMC_NUM(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = 65535;

	//MEMC
	if (ssize > u16Size)
		DRM_ERROR("Failed to get MEMC\n");

	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u32MEMC_NUM);
		DRM_INFO("gDRMcap_capability.u32MEMC_NUM:%llx\n",
			gDRMcap_capability.u32MEMC_NUM);
	}
	return ssize;
}

static ssize_t mtk_drm_capability_show_WINDOW_NUM(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = 65535;

	//Window Num
	if (ssize > u16Size)
		DRM_ERROR("Failed to get Window Num\n");

	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u32WindowNUM);
		DRM_INFO("gDRMcap_capability.u32WindowNum:%llx\n",
			gDRMcap_capability.u32WindowNUM);
	}
	return ssize;
}

static ssize_t mtk_drm_capability_show_PANELGAMMA_BIT_NUM(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = 65535;

	//PANEL GAMMA BIT NUMBER
	if (ssize > u16Size)
		DRM_ERROR("Failed to get panel gamma bit number\n");
	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u8PanelGammaBit);
		DRM_INFO("gDRMcap_capability.u8PanelGammaBit:%llx\n",
			gDRMcap_capability.u8PanelGammaBit);
	}
	return ssize;
}

static ssize_t mtk_drm_capability_show_OD_SUPPORT(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = 65535;

	//OD
	if (ssize > u16Size)
		DRM_ERROR("Failed to get OD support\n");

	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u8ODSupport);
		DRM_INFO("gDRMcap_capability.u8ODSupport;:%llx\n",
			gDRMcap_capability.u8ODSupport);
	}
	return ssize;
}

static ssize_t mtk_drm_capability_show_OLED_DEMURAIN_SUPPORT(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = 65535;

	//OLED DEMURAIN
	if (ssize > u16Size)
		DRM_ERROR("Failed to get OLED DEMURAIN support\n");
	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u8OLEDDemuraInSupport);
		DRM_INFO("gDRMcap_capability.u8OLEDDemuraInSupport:%llx\n",
			gDRMcap_capability.u8OLEDDemuraInSupport);
	}
	return ssize;
}

static ssize_t mtk_drm_capability_show_OLED_BOOSTING_SUPPORT(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = 65535;

	//OLED BOOSTING
	if (ssize > u16Size)
		DRM_ERROR("Failed to get OLED BOOSTING support\n");
	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u8OLEDBoostingSupport);
		DRM_INFO("gDRMcap_capability.u8OLEDBoostingSupport:%llx\n",
			gDRMcap_capability.u8OLEDBoostingSupport);
	}
	return ssize;
}

static ssize_t mtk_drm_capability_store_DMA_SCL_NUM(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_MGW_NUM
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_MEMC_NUM
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_WINDOW_NUM
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_PANELGAMMA_BIT_NUM
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_OD_SUPPORT
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_OLED_DEMURAIN_SUPPORT
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_OLED_BOOSTING_SUPPORT
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static struct device_attribute mtk_drm_attr[] = {
	__ATTR(drm_capability_DMA_SCL_NUM, MTK_DRM_KMS_ATTR_MODE,
		mtk_drm_capability_show_DMA_SCL_NUM,
		mtk_drm_capability_store_DMA_SCL_NUM),
	__ATTR(drm_capability_MGW_NUM, MTK_DRM_KMS_ATTR_MODE,
		mtk_drm_capability_show_MGW_NUM,
		mtk_drm_capability_store_MGW_NUM),
	__ATTR(drm_capability_MEMC_NUM, MTK_DRM_KMS_ATTR_MODE,
		mtk_drm_capability_show_MEMC_NUM,
		mtk_drm_capability_store_MEMC_NUM),
	__ATTR(drm_capability_WINDOW_NUM, MTK_DRM_KMS_ATTR_MODE,
		mtk_drm_capability_show_WINDOW_NUM,
		mtk_drm_capability_store_WINDOW_NUM),
	__ATTR(drm_capability_PANELGAMMA_BIT_NUM, MTK_DRM_KMS_ATTR_MODE,
		mtk_drm_capability_show_PANELGAMMA_BIT_NUM,
		mtk_drm_capability_store_PANELGAMMA_BIT_NUM),
	__ATTR(drm_capability_OD_SUPPORT, MTK_DRM_KMS_ATTR_MODE,
		mtk_drm_capability_show_OD_SUPPORT,
		mtk_drm_capability_store_OD_SUPPORT),
	__ATTR(drm_capability_OLED_DEMURAIN_SUPPORT, MTK_DRM_KMS_ATTR_MODE,
		mtk_drm_capability_show_OLED_DEMURAIN_SUPPORT,
		mtk_drm_capability_store_OLED_DEMURAIN_SUPPORT),
	__ATTR(drm_capability_OLED_BOOSTING_SUPPORT, MTK_DRM_KMS_ATTR_MODE,
		mtk_drm_capability_show_OLED_BOOSTING_SUPPORT,
		mtk_drm_capability_store_OLED_BOOSTING_SUPPORT),
};

void mtk_drm_CreateSysFS(struct device *pdv)
{
	int i = 0;

	DRM_INFO("[DRM] Device_create_file initialized\n");

	for (i = 0; i < (sizeof(mtk_drm_attr) / sizeof(struct device_attribute));
		i++) {
		if (device_create_file(pdv, &mtk_drm_attr[i]) != 0)
			DRM_ERROR("Device_create_file_error\n");
		else
			DRM_INFO("Device_create_file_success\n");
	}
}

static void mtk_tv_kms_enable(
	struct mtk_tv_drm_crtc *crtc)
{
	mtk_gop_enable(crtc);
}

static void mtk_tv_kms_disable(
	struct mtk_tv_drm_crtc *crtc)
{
	mtk_gop_disable(crtc);
}

static int mtk_tv_kms_enable_vblank(
	struct mtk_tv_drm_crtc *crtc)
{
	int ret = 0;

	ret = mtk_gop_enable_vblank(crtc);

	return ret;
}

static int mtk_tv_kms_gamma_set(
	struct drm_crtc *crtc,
	uint16_t *r,
	uint16_t *g,
	uint16_t *b,
	uint32_t size,
	struct drm_modeset_acquire_ctx *ctx)
{
	int ret = 0;

	mtk_video_panel_gamma_setting(crtc, r, g, b, size);

	return ret;
}

static void mtk_tv_kms_update_plane(
	struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);

	if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_GRAPHIC)
		mtk_gop_update_plane(state);
	else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_VIDEO) {
		mtk_video_display_update_plane(state);
	} else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_PRIMARY) {
		DRM_ERROR("[%s, %d]: primary prime is unused now %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
	} else {
		DRM_ERROR("[%s, %d]: unknown plane type %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
	}
}

static void mtk_tv_kms_disable_plane(
	struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);

	if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_GRAPHIC)
		mtk_gop_disable_plane(state);
	else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_VIDEO)
		mtk_video_display_disable_plane(state);
	else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_PRIMARY) {
		DRM_ERROR("[%s, %d]: primary prime is unused now %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
	} else {
		DRM_ERROR("[%s, %d]: unknown plane type %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
	}
}

static void mtk_tv_kms_disable_vblank(
	struct mtk_tv_drm_crtc *crtc)
{
	mtk_gop_disable_vblank(crtc);
}

static int mtk_tv_kms_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val)
{
	int ret = -EINVAL;
	int i;

	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->crtc_private;
	struct ext_crtc_prop_info *crtc_props =
		pctx->ext_crtc_properties;

	for (i = 0; i < E_EXT_CRTC_PROP_MAX; i++) {
		if (property == pctx->drm_crtc_prop[i]) {
			crtc_props->prop_val[i] = val;
			crtc_props->prop_update[i] = 1;
			ret = 0x0;
			break;
		}
	}

	if (ret != 0) {
		DRM_ERROR("[%s, %d]: unknown CRTC property %s!!\n",
			__func__, __LINE__, property->name);
		return ret;
	}

	if (PANEL_PROP(i)) {
		//pnl property
		ret = mtk_video_atomic_set_crtc_property(
			crtc,
			state,
			property,
			val,
			i);
		if (ret != 0) {
			DRM_ERROR("[%s, %d]:set crtc PNL prop fail!!\n",
				__func__, __LINE__);
			return ret;
		}
	} else if (PIXELSHIFT_PROP(i)) {
		//pixel shift property
		ret = mtk_video_atomic_set_crtc_property(
			crtc,
			state,
			property,
			val,
			i);
		if (ret != 0) {
			DRM_ERROR("[%s, %d]:set crtc pixelshift prop fail!!\n",
				__func__, __LINE__);
		}

	} else if (MEMC_PROP(i)) {
		//memc property
		ret = mtk_video_atomic_set_crtc_property(
			crtc,
			state,
			property,
			val,
			i);
		if (ret != 0) {
			DRM_ERROR("[%s, %d]:set crtc MEMC prop fail!!\n",
				__func__, __LINE__);
		}
	} else if (LDM_PROP(i)) {
		//ldm property
		ret = mtk_ld_atomic_set_crtc_property(crtc,
			state,
			property,
			val,
			i);
		if (ret != 0) {
			DRM_ERROR("[%s, %d]:set crtc LDM prop fail!!\n",
				__func__, __LINE__);
			return ret;
		}
	} else {
		//do not match any property
		DRM_ERROR("[%s, %d]:set crtc prop fail!!\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	return ret;
}

static int mtk_tv_kms_atomic_get_crtc_property(struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->crtc_private;
	struct ext_crtc_prop_info *crtc_props =
		pctx->ext_crtc_properties;

	for (i = 0; i < E_EXT_CRTC_PROP_MAX; i++) {
		if (property == pctx->drm_crtc_prop[i]) {
			*val = crtc_props->prop_val[i];
			ret = 0x0;
			break;
		}
	}

	if (PANEL_PROP(i)) {
		//pnl property
		ret = mtk_video_atomic_get_crtc_property(
			crtc,
			state,
			property,
			val,
			i);
		if (ret != 0) {
			DRM_ERROR("[%s, %d]:get crtc PNL prop fail!!\n",
				__func__, __LINE__);
			return ret;
		}
	} else if (PIXELSHIFT_PROP(i)) {
		//pixel shift property
		ret = mtk_video_atomic_get_crtc_property(
			crtc,
			state,
			property,
			val,
			i);
		if (ret != 0) {
			DRM_ERROR("[%s, %d]:get crtc pixelshift prop fail!!\n",
				__func__, __LINE__);
			return ret;
		}
	} else if (MEMC_PROP(i)) {
		//MEMC property
		ret = mtk_video_atomic_get_crtc_property(
			crtc,
			state,
			property,
			val,
			i);
		if (ret != 0) {
			DRM_ERROR("[%s, %d]:get crtc MEMC prop fail!!\n",
				__func__, __LINE__);
			return ret;
		}
	} else if (LDM_PROP(i)) {
		//ldm property
		ret = mtk_ld_atomic_get_crtc_property(crtc,
			state,
			property,
			val,
			i);
		if (ret != 0) {
			DRM_ERROR("[%s, %d]:get crtc LDM prop fail!!\n",
				__func__, __LINE__);
			return ret;
		}
	} else {
		//do not match any property
		DRM_ERROR("[%s, %d]:get crtc prop fail!!\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	return ret;
}



static int mtk_tv_kms_atomic_set_plane_common_property(
	struct mtk_drm_plane *mplane,
	struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t val)
{
	int ret = -EINVAL;
	int i;
	unsigned int plane_inx = mplane->base.index;

	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct ext_common_plane_prop_info *plane_prop =
		pctx->ext_common_plane_properties + plane_inx;

	for (i = 0; i < E_EXT_COMMON_PLANE_PROP_MAX; i++) {
		if (property == pctx->drm_common_plane_prop[i]) {
			plane_prop->prop_val[i] = val;
			ret = 0x0;
			break;
		}
	}

	return ret;
}

static int mtk_tv_kms_atomic_get_plane_common_property(
	struct mtk_drm_plane *mplane,
	const struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	unsigned int plane_inx = mplane->base.index;
	struct ext_common_plane_prop_info *plane_prop =
		pctx->ext_common_plane_properties + plane_inx;

	for (i = 0; i < E_EXT_COMMON_PLANE_PROP_MAX; i++) {
		if (property == pctx->drm_common_plane_prop[i]) {
			*val = plane_prop->prop_val[i];
			ret = 0x0;
			break;
		}
	}

	return ret;
}

static int mtk_tv_kms_atomic_set_plane_property(
	struct mtk_drm_plane *mplane,
	struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t val)
{
	int ret = -EINVAL;

	ret = mtk_tv_kms_atomic_set_plane_common_property(
		mplane,
		state,
		property,
		val);
	if (ret == 0) {
		// found in common property list, no need to find in other list
		return ret;
	}

	if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_GRAPHIC) {
		ret = mtk_gop_atomic_set_plane_property(
			mplane,
			state,
			property,
			val);
	} else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_VIDEO) {
		ret = mtk_video_display_atomic_set_plane_property(
			mplane,
			state,
			property,
			val);
	} else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_PRIMARY) {
		DRM_ERROR("[%s, %d]: primary prime is unused now %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
	} else
		DRM_ERROR("[%s, %d]: unknown plane type %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);

	return ret;
}

static int mtk_tv_kms_atomic_get_plane_property(
	struct mtk_drm_plane *mplane,
	const struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = -EINVAL;

	ret = mtk_tv_kms_atomic_get_plane_common_property(
		mplane,
		state,
		property,
		val);
	if (ret == 0) {
		// found in common property list, no need to find in other list
		return ret;
	}

	if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_GRAPHIC) {
		ret = mtk_gop_atomic_get_plane_property(
			mplane,
			state,
			property,
			val);
	} else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_VIDEO) {
		ret = mtk_video_display_atomic_get_plane_property(
			mplane,
			state,
			property,
			val);
	} else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_PRIMARY) {
		DRM_ERROR("[%s, %d]: primary prime is unused now %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
	} else
		DRM_ERROR("[%s, %d]: unknown plane type %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);

	return ret;
}

static int mtk_tv_kms_bootlogo_ctrl(
	struct mtk_tv_drm_crtc *crtc,
	unsigned int cmd,
	unsigned int *gop)
{
	int ret = 0;

	ret = mtk_gop_bootlogo_ctrl(crtc, cmd, gop);

	return ret;
}

static void mtk_tv_kms_atomic_crtc_flush(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state)
{
	mtk_video_atomic_crtc_flush(crtc, old_crtc_state);
}

static int mtk_tv_kms_check_plane(struct drm_plane_state *plane_state,
				const struct drm_crtc_state *crtc_state,
				struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	int ret = 0;

	if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_GRAPHIC) {
		ret = drm_atomic_helper_check_plane_state(plane_state, crtc_state,
						   DRM_PLANE_SCALE_CAPBILITY,
						   DRM_PLANE_HELPER_NO_SCALING,
						true, true);
	} else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_VIDEO) {
		ret = mtk_video_check_plane(plane_state, crtc_state, state);
	} else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_PRIMARY) {
		DRM_ERROR("[%s, %d]: primary prime is unused now %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
		ret = -EINVAL;
	} else {
		DRM_ERROR("[%s, %d]: unknown plane type %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
		ret = -EINVAL;
	}
	return ret;
}

static const struct mtk_tv_drm_crtc_ops mtk_tv_kms_crtc_ops = {
	.enable = mtk_tv_kms_enable,
	.disable = mtk_tv_kms_disable,
	.enable_vblank = mtk_tv_kms_enable_vblank,
	.disable_vblank = mtk_tv_kms_disable_vblank,
	.update_plane = mtk_tv_kms_update_plane,
	.disable_plane = mtk_tv_kms_disable_plane,
	.atomic_set_crtc_property = mtk_tv_kms_atomic_set_crtc_property,
	.atomic_get_crtc_property = mtk_tv_kms_atomic_get_crtc_property,
	.atomic_set_plane_property = mtk_tv_kms_atomic_set_plane_property,
	.atomic_get_plane_property = mtk_tv_kms_atomic_get_plane_property,
	.bootlogo_ctrl = mtk_tv_kms_bootlogo_ctrl,
	.atomic_crtc_flush = mtk_tv_kms_atomic_crtc_flush,
	.gamma_set = mtk_tv_kms_gamma_set,
	.check_plane = mtk_tv_kms_check_plane,
};

/*Connector*/
static int mtk_tv_kms_atomic_set_connector_property(
	struct mtk_tv_drm_connector *connector,
	struct drm_connector_state *state,
	struct drm_property *property,
	uint64_t val)
{
	int ret = 0;

	ret = mtk_video_atomic_set_connector_property(
		connector,
		state,
		property,
		val);

	return ret;
}

static int mtk_tv_kms_atomic_get_connector_property(
	struct mtk_tv_drm_connector *connector,
	const struct drm_connector_state *state,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = 0;

	ret = mtk_video_atomic_get_connector_property(
		connector,
		state,
		property,
		val);

	return ret;
}

static const struct mtk_tv_drm_connector_ops mtk_tv_kms_connector_ops = {
	.atomic_set_connector_property =
		mtk_tv_kms_atomic_set_connector_property,
	.atomic_get_connector_property =
		mtk_tv_kms_atomic_get_connector_property,
};

static int readDTB2KMSPrivate(struct mtk_tv_kms_context *pctx)
{
	int ret = 0;
	struct device_node *np;
	int u32Tmp = 0xFF;
	const char *name;

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_TV_KMS_COMPATIBLE_STR);

	if (np != NULL) {
		ret = of_irq_get(np, 0);
		if (ret < 0) {
			DRM_ERROR("[%s, %d]: of_irq_get failed\r\n",
				__func__, __LINE__);
			return ret;
		}
		pctx->graphic_irq_num = ret;

		// read primary plane num
		name = "PRIMARY_PLANE_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->plane_num[MTK_DRM_PLANE_TYPE_PRIMARY] = u32Tmp;

		// read primary plane index start
		name = "PRIMARY_PLANE_INDEX_START";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->plane_index_start[MTK_DRM_PLANE_TYPE_PRIMARY] = u32Tmp;

		// read video plane num
		name = "VIDEO_PLANE_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->plane_num[MTK_DRM_PLANE_TYPE_VIDEO] = u32Tmp;

		// read video plane index start
		name = "VIDEO_PLANE_INDEX_START";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->plane_index_start[MTK_DRM_PLANE_TYPE_VIDEO] = u32Tmp;

		// read video MEMC number
		name = "VIDEO_MEMC_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->drmcap_dev.u32MEMC_NUM = u32Tmp;

		// read video MGW number
		name = "VIDEO_MGW_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->drmcap_dev.u32MGW_NUM = u32Tmp;

		// read video DMS_SCL number
		name = "VIDEO_DMA_SCL_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->drmcap_dev.u32DMA_SCL_NUM = u32Tmp;

		// read video WINDDOW number
		name = "VIDEO_WINDOW_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->drmcap_dev.u32WindowNUM = u32Tmp;

		// read panel gamma bit number
		name = "VIDEO_PANEL_GAMMA_BIT_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->drmcap_dev.u8PanelGammaBit = u32Tmp;

		// read OD support
		name = "VIDEO_OD";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->drmcap_dev.u8ODSupport = u32Tmp;

		// read OLED DEMURAIN support
		name = "VIDEO_OLED_DEMURA_IN";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->drmcap_dev.u8OLEDDemuraInSupport = u32Tmp;

		// read OLED BOOSTING support
		name = "VIDEO_OLED_BOOSTING";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->drmcap_dev.u8OLEDBoostingSupport = u32Tmp;

		// read byte per word
		name = "byte_per_word";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.byte_per_word = u32Tmp;

		// read reg num
		name = "reg_num";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.reg_num = u32Tmp;

		// read gop plane num
		name = "GRAPHIC_PLANE_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->plane_num[MTK_DRM_PLANE_TYPE_GRAPHIC] = u32Tmp;

		// read gop plane index start
		name = "GRAPHIC_PLANE_INDEX_START";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->plane_index_start[MTK_DRM_PLANE_TYPE_GRAPHIC] = u32Tmp;
	}

	return ret;
}

static void mtk_tv_drm_crtc_finish_page_flip(struct mtk_tv_drm_crtc *mcrtc)
{
	struct drm_crtc *crtc = &mcrtc->base;
	unsigned long flags;

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	drm_crtc_send_vblank_event(crtc, mcrtc->event);
	mcrtc->event = NULL;
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
}

static irqreturn_t mtk_tv_kms_irq_handler(int irq, void *dev_id)
{
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)dev_id;

	mtk_gop_disable_vblank(&pctx->crtc);

	drm_crtc_handle_vblank(&pctx->crtc.base);

	if (pctx->crtc.pending_needs_vblank == true) {
		mtk_tv_drm_crtc_finish_page_flip(&pctx->crtc);
		pctx->crtc.pending_needs_vblank = false;
	}
	mtk_gop_enable_vblank(&pctx->crtc);
	return IRQ_HANDLED;
}

static void clean_tv_kms_context(
	struct device *dev,
	struct mtk_tv_kms_context *pctx)
{
	if (pctx) {
		kfree(pctx->gop_priv.gop_plane_properties);
		pctx->gop_priv.gop_plane_properties = NULL;

		devm_kfree(dev, pctx);
	}
}

int mtk_tv_kms_create_ext_props(
	struct mtk_tv_kms_context *pctx,
	enum ext_prop_type prop_type)
{
	struct drm_property *prop = NULL;
	const struct ext_prop_info *prop_def = NULL;
	int extend_prop_count = 0;
	const struct ext_prop_info *ext_prop = NULL;
	struct drm_device *drm_dev = pctx->drm_dev;
	int i, plane_count = 0;
	struct mtk_drm_plane *mplane = NULL;
	struct drm_mode_object *obj = NULL;
	struct drm_plane *plane = NULL;
	unsigned int zpos_min;
	unsigned int zpos_max;
	uint64_t init_val;

	struct drm_property_blob *propBlob;
	struct drm_property *propMemcStatus = NULL;
	struct property_blob_memc_status stMemcStatus;

	memset(&stMemcStatus, 0, sizeof(struct property_blob_memc_status));

	if (prop_type == E_EXT_PROP_TYPE_COMMON_PLANE) {
		prop_def = ext_common_plane_props_def;
		extend_prop_count = ARRAY_SIZE(ext_common_plane_props_def);
	} else if (prop_type == E_EXT_PROP_TYPE_CRTC) {
		prop_def = ext_crtc_props_def;
		extend_prop_count = ARRAY_SIZE(ext_crtc_props_def);
	} else if (prop_type == E_EXT_PROP_TYPE_CONNECTOR) {
		prop_def = ext_connector_props_def;
		extend_prop_count = ARRAY_SIZE(ext_connector_props_def);
	} else {
		DRM_ERROR("[%s, %d]: unknown ext_prop_type %d!!\n",
			__func__, __LINE__, prop_type);
	}
	// create extend common plane properties
	for (i = 0; i < extend_prop_count; i++) {
		ext_prop = &prop_def[i];
		if (ext_prop->flag & DRM_MODE_PROP_ENUM) {
			prop = drm_property_create_enum(
				drm_dev,
				ext_prop->flag,
				ext_prop->prop_name,
				ext_prop->enum_info.enum_list,
				ext_prop->enum_info.enum_length);

			if (prop == NULL) {
				DRM_ERROR("[%s, %d]: create ext prop fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}
		} else if (ext_prop->flag & DRM_MODE_PROP_RANGE) {
			prop = drm_property_create_range(
				drm_dev,
				ext_prop->flag,
				ext_prop->prop_name,
				ext_prop->range_info.min,
				ext_prop->range_info.max);

			if (prop == NULL) {
				DRM_ERROR("[%s, %d]: create ext prop fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}

			//save memc status prop
			if (strcmp(ext_prop->prop_name,
				MTK_CRTC_PROP_MEMC_GET_STATUS) == 0) {
				propMemcStatus = prop;
			}
		} else if (ext_prop->flag & DRM_MODE_PROP_SIGNED_RANGE) {
			prop = drm_property_create_signed_range(
				pctx->drm_dev,
				ext_prop->flag,
				ext_prop->prop_name,
				(U642I64)(ext_prop->range_info.min),
				(U642I64)(ext_prop->range_info.max));
			if (prop == NULL) {
				DRM_ERROR("[%s, %d]: create ext prop fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}
		} else if (!strcmp(ext_prop->prop_name,
				MTK_CRTC_PROP_MEMC_PLANE_ID)) {
			prop = drm_property_create_object(
				pctx->drm_dev,
				ext_prop->flag,
				ext_prop->prop_name,
				DRM_MODE_OBJECT_PROPERTY);

			if (prop == NULL) {
				DRM_ERROR("[%s, %d]: create ext prop fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}
			DRM_INFO("drm_property_create_object ok:id=%d\n",
				prop->base.id);
		} else {
			DRM_ERROR("[%s, %d]: unknown prop flag 0x%x !!\n",
				__func__, __LINE__, ext_prop->flag);
			return -EINVAL;
		}

		// attach created props & add created props to context
		if (prop_type == E_EXT_PROP_TYPE_COMMON_PLANE) {
			drm_for_each_plane(plane, drm_dev) {
				mplane = to_mtk_plane(plane);
				obj = &(mplane->base.base);

			// attach property by init value
			if (strcmp(prop->name, MTK_PLANE_PROP_PLANE_TYPE) == 0)
				init_val = mplane->mtk_plane_type;

			else
				init_val = ext_prop->init_val;

			drm_object_attach_property(
				obj,
				prop,
				init_val);
			(pctx->ext_common_plane_properties + plane_count)
				->prop_val[i] =	init_val;
			pctx->drm_common_plane_prop[i] = prop;
			++plane_count;

			// create zpos property
			zpos_min = 0;
			if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_VIDEO)
				zpos_max = zpos_min + pctx->video_priv.videoPlaneType_TypeNum - 1;
			else if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_GRAPHIC)
				zpos_max = zpos_min + pctx->plane_num[mplane->mtk_plane_type] - 1;

			drm_plane_create_zpos_property(plane,
				mplane->zpos,
				zpos_min,
				zpos_max);
			}
		} else if (prop_type == E_EXT_PROP_TYPE_CRTC) {
			obj = &(pctx->crtc.base.base);

			// attach property by init value
			drm_object_attach_property(
				obj,
				prop,
				ext_prop->init_val);

			pctx->drm_crtc_prop[i] =
				prop;
			pctx->ext_crtc_properties->prop_val[i] =
				ext_prop->init_val;
		} else if (prop_type == E_EXT_PROP_TYPE_CONNECTOR) {
			obj = &(pctx->connector.base.base);
			drm_object_attach_property(obj,
				prop,
				ext_prop->init_val);
			pctx->drm_connector_prop[i] =
				prop;
			pctx->ext_connector_properties->prop_val[i] =
				ext_prop->init_val;
		} else {
			DRM_ERROR("[%s, %d]: unknown ext_prop_type %d!!\n",
				__func__, __LINE__, prop_type);
		}
	}

	//for creat blob
	if (prop_type == E_EXT_PROP_TYPE_CRTC) {
		propBlob = drm_property_create_blob(
			pctx->drm_dev,
			sizeof(struct property_blob_memc_status),
			&stMemcStatus);
		if (propBlob == NULL) {
			DRM_ERROR("[%s, %d]: drm_property_create_blob fail!!\n",
				__func__, __LINE__);
			goto NOMEM;
		} else {
			if (propMemcStatus != NULL) {
				for (i = 0; i < E_EXT_CRTC_PROP_MAX; i++) {
					if (propMemcStatus ==
						pctx->drm_crtc_prop[i]) {
						pctx->ext_crtc_properties
							->prop_val[i] =
							propBlob->base.id;
						break;
					}
				}
			}
		}
	}

	return 0;

NOMEM:
	return -ENOMEM;
}

static int mtk_tv_kms_bind(
	struct device *dev,
	struct device *master,
	void *data)
{
	int ret = 0;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	struct mtk_drm_plane *primary_plane = NULL;
	struct mtk_drm_plane *cursor_plane = NULL;
	int plane_type;

	pctx->drm_dev = drm_dev;
	pctx->crtc.crtc_private = pctx;

	ret = readDTB2KMSPrivate(pctx);
	if (ret != 0) {
		DRM_ERROR("[%s, %d]: readDeviceTree2Context failed.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	// assign total plane num to context
	pctx->total_plane_num = 0;
	for (plane_type = 0;
		plane_type < MTK_DRM_PLANE_TYPE_MAX;
		plane_type++) {

		pctx->total_plane_num += pctx->plane_num[plane_type];
	}

	// alloc memory for extend common plane properties
	pctx->ext_common_plane_properties =
		devm_kzalloc(dev,
		sizeof(struct ext_common_plane_prop_info) *
			pctx->total_plane_num, GFP_KERNEL);
	if (pctx->ext_common_plane_properties == NULL) {
		DRM_ERROR("[%s, %d]: devm_kzalloc failed.\n",
			__func__, __LINE__);
		return -ENOMEM;
	}
	memset(pctx->ext_common_plane_properties,
		0,
		sizeof(struct ext_common_plane_prop_info) *
			pctx->total_plane_num);

	// alloc memory for extend crtc properties
	pctx->ext_crtc_properties = devm_kzalloc(
		dev,
		sizeof(struct ext_crtc_prop_info),
		GFP_KERNEL);

	if (pctx->ext_crtc_properties == NULL) {
		DRM_ERROR("[%s, %d]: devm_kzalloc failed.\n",
			__func__, __LINE__);
		return -ENOMEM;
	}
	memset(pctx->ext_crtc_properties, 0, sizeof(struct ext_crtc_prop_info));

	// alloc memory for extend connector properties
	pctx->ext_connector_properties = devm_kzalloc(
		dev,
		sizeof(struct ext_connector_prop_info),
		GFP_KERNEL);
	if (pctx->ext_connector_properties == NULL) {
		DRM_ERROR("[%s, %d]: devm_kzalloc failed.\n",
			__func__, __LINE__);
		return -ENOMEM;
	}
	memset(pctx->ext_connector_properties,
		0,
		sizeof(struct ext_connector_prop_info));

	ret = mtk_video_display_init(dev, master, data, &primary_plane, NULL);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_video_display_init  failed.\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_panel_init(dev);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_panel_init failed.\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_gop_init(dev, master, data, NULL, &cursor_plane);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_gop_init failed.\n",
			__func__, __LINE__);
		return ret;
	}
	//for LDM
	ret = mtk_ldm_init(dev);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_ldm_init failed.\n",
			__func__, __LINE__);
		return ret;
	}

	// create extend common plane properties
	ret = mtk_tv_kms_create_ext_props(pctx, E_EXT_PROP_TYPE_COMMON_PLANE);
	if (ret) {
		DRM_ERROR("[%s, %d]: kms create ext prop failed\n",
			__func__, __LINE__);
		return ret;
	}

	ret = devm_request_irq(
		dev,
		pctx->graphic_irq_num,
		mtk_tv_kms_irq_handler,
		IRQF_SHARED,
		"DRM_MTK",
		pctx);
	if (ret) {
		DRM_ERROR("[%s, %d]: irq request failed.\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_tv_drm_crtc_create(
		drm_dev,
		&pctx->crtc,
		&primary_plane->base,
		&cursor_plane->base,
		&mtk_tv_kms_crtc_ops);
	if (ret != 0x0) {
		DRM_ERROR("[%s, %d]: crtc create failed.\n",
			__func__, __LINE__);
		goto ERR;
	}

	// create extend crtc properties
	ret = mtk_tv_kms_create_ext_props(pctx, E_EXT_PROP_TYPE_CRTC);
	if (ret) {
		DRM_ERROR("[%s, %d]: kms create ext prop failed\n",
			__func__, __LINE__);
		return ret;
	}

	if (mtk_tv_drm_encoder_create(
		drm_dev,
		&pctx->encoder,
		&pctx->crtc.base) != 0) {

		DRM_ERROR("[%s, %d]: mtk_tv_drm_encoder_create fail.\n",
		__func__, __LINE__);
		goto ERR;
	}

	if ( mtk_tv_drm_connector_create(
		pctx->drm_dev,
		&pctx->connector,
		&pctx->encoder,
		&mtk_tv_kms_connector_ops) != 0) {
		DRM_ERROR("[%s, %d]: mtk_tv_drm_connector_create fail.\n",
			__func__, __LINE__);
		goto ERR;
	}

	// create extend connector properties
	ret = mtk_tv_kms_create_ext_props(pctx, E_EXT_PROP_TYPE_CONNECTOR);
	if (ret) {
		DRM_ERROR("[%s, %d]: kms create ext prop failed\n",
			__func__, __LINE__);
		return ret;
	}

	_mtk_drm_get_capability(pctx);

	mtk_drm_CreateSysFS(dev);

	return 0;

ERR:
	clean_tv_kms_context(dev, pctx);
	return ret;
}

static void mtk_tv_kms_unbind(
	struct device *dev,
	struct device *master,
	void *data)
{
}

static const struct component_ops mtk_tv_kms_component_ops = {
	.bind	= mtk_tv_kms_bind,
	.unbind = mtk_tv_kms_unbind,
};

static const struct of_device_id mtk_drm_tv_kms_of_ids[] = {
	{ .compatible = "MTK-drm-tv-kms", },
};

static int mtk_drm_tv_kms_suspend(
	struct platform_device *dev,
	pm_message_t state)
{
	int ret = 0;

	ret = mtk_drm_gop_suspend(dev, state);

	return ret;
}

static int mtk_drm_tv_kms_resume(struct platform_device *dev)
{
	int ret = 0;

	ret = mtk_drm_gop_resume(dev);

	return ret;
}

static int mtk_drm_tv_kms_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_tv_kms_context *pctx;
	int ret;

	pctx = devm_kzalloc(dev, sizeof(*pctx), GFP_KERNEL);
	if (!pctx) {
		ret = -ENOMEM;
		goto ERR;
	}
	memset(pctx, 0x0, sizeof(struct mtk_tv_kms_context));

	pctx->dev = dev;
	platform_set_drvdata(pdev, pctx);

	ret = component_add(dev, &mtk_tv_kms_component_ops);
	if (ret) {
		DRM_ERROR("[%s, %d]: component_add fail\n",
			__func__, __LINE__);
		goto ERR;
	}

	return 0;
ERR:
	if (pctx != NULL)
		devm_kfree(dev, pctx);
	return ret;
}

static int mtk_drm_tv_kms_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	if (pctx) {
		kfree(pctx->gop_priv.gop_plane_properties);
		pctx->gop_priv.gop_plane_properties = NULL;

		kfree(pctx->gop_priv.pCurrent_CSCParam);
		pctx->gop_priv.pCurrent_CSCParam = NULL;

		kfree(pctx->gop_priv.pPnlInfo);
		pctx->gop_priv.pPnlInfo = NULL;
	}
	return 0;
}

struct platform_driver mtk_drm_tv_kms_driver = {
	.probe = mtk_drm_tv_kms_probe,
	.remove = mtk_drm_tv_kms_remove,
	.suspend = mtk_drm_tv_kms_suspend,
	.resume = mtk_drm_tv_kms_resume,
	.driver = {
		.name = "mediatek-drm-tv-kms",
		.of_match_table = mtk_drm_tv_kms_of_ids,
	},
};

