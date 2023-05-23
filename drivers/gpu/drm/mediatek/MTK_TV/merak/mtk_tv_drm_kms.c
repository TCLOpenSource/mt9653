// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/of_irq.h>
#include <linux/dma-buf.h>
#include <linux/cpumask.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <drm/mtk_tv_drm.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_panel.h>
#include <drm/drm_self_refresh_helper.h>


#include "mtk_tv_drm_plane.h"
#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_gop.h"
#include "mtk_tv_drm_common.h"
#include "mtk_tv_drm_encoder.h"
#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_log.h"
#include "mtk_tv_drm_pnlgamma.h"
#include "mtk_tv_drm_pqgamma.h"
#include "mtk_tv_drm_video_frc.h"

#include "drv_scriptmgt.h"
#include "mtk_tv_drm_common_irq.h"
#include "mtk_tv_drm_video_pixelshift.h"
#include "mtk_tv_drm_video_panel.h"
#include "pqu_msg.h"
#include "m_pqu_render.h"
#include "ext_command_render_if.h"
#include "mtk_tv_drm_sync.h"
#if defined(__KERNEL__) /* Linux Kernel */
#if (defined ANDROID)
#include "mi_realtime.h"
#endif
#endif

#define MTK_DRM_MODEL MTK_DRM_MODEL_KMS
#define DTB_MSK 0xFF
#define SYS_DEVICE_SIZE 65535
#define INT8_NEGATIVE 0x80

#define MOD_VER_2 (2)

uint32_t mtk_drm_log_level; // DRM_INFO is default disable


DECLARE_WAIT_QUEUE_HEAD(atomic_commit_tail_wait);
DECLARE_WAIT_QUEUE_HEAD(atomic_commit_tail_wait_Grapcrtc);
unsigned long atomic_commit_tail_entry, atomic_commit_tail_Gcrtc_entry;


#define PANEL_PROP(i) \
		((i >= E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID) && \
			(i <= E_EXT_CRTC_PROP_VIDEO_LATENCY)) \

#define PIXELSHIFT_PROP(i) \
		((i >= E_EXT_CRTC_PROP_PIXELSHIFT_ENABLE) && \
			(i <= E_EXT_CRTC_PROP_PIXELSHIFT_V)) \

#define LDM_PROP(i) \
		((i >= E_EXT_CRTC_PROP_LDM_STATUS) && \
			(i <= E_EXT_CRTC_PROP_LDM_INIT))\

#define GLOBAL_PQ_PROP(i) \
		((i >= E_EXT_CRTC_PROP_GLOBAL_PQ) && \
			(i <= E_EXT_CRTC_PROP_GLOBAL_PQ))\

#define VIDEO_GOP_PROP(i) \
		((i >= E_EXT_CRTC_PROP_GOP_VIDEO_ORDER) && \
			(i <= E_EXT_CRTC_PROP_GOP_VIDEO_ORDER))\

#define MTK_DRM_KMS_ATTR_MODE     (0644)
#define DRM_PLANE_SCALE_CAPBILITY (1<<4)
#define MTK_DRM_KMS_GFID_MAX      (255)
#define OLD_LAYER_INITIAL_VALUE   (0xFF)

static __s64 start_crtc_begin;

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


static struct drm_prop_enum_list video_pixelshift_type_enum_list[] = {
	{.type = VIDEO_CRTC_PIXELSHIFT_PRE_JUSTSCAN,
		.name = MTK_CRTC_PROP_PIXELSHIFT_PRE_JUSTSCAN},
	{.type = VIDEO_CRTC_PIXELSHIFT_PRE_OVERSCAN,
		.name = MTK_CRTC_PROP_PIXELSHIFT_PRE_OVERSCAN},
	{.type = VIDEO_CRTC_PIXELSHIFT_POST_JUSTSCAN,
		.name = MTK_CRTC_PROP_PIXELSHIFT_POST_JUSTSCAN},
};


static struct drm_prop_enum_list video_framesync_mode_enum_list[] = {
	{.type = VIDEO_CRTC_FRAME_SYNC_FREERUN, .name = "framesync_freerun"},
	{.type = VIDEO_CRTC_FRAME_SYNC_VTTV, .name = "framesync_vttv"},
	{.type = VIDEO_CRTC_FRAME_SYNC_FRAMELOCK, .name = "framesync_framelock1to1"},
	{.type = VIDEO_CRTC_FRAME_SYNC_VTTPLL, .name = "framesync_vttpll"},
	{.type = VIDEO_CRTC_FRAME_SYNC_MAX, .name = "framesync_max"},
};

static struct drm_prop_enum_list video_gop_order_enum_list[] = {
	{.type = VIDEO_OSDB0_OSDB1,
		.name = MTK_VIDEO_CRTC_PROP_V_OSDB0_OSDB1},
	{.type = OSDB0_VIDEO_OSDB1,
		.name = MTK_VIDEO_CRTC_PROP_OSDB0_V_OSDB1},
	{.type = OSDB0_OSDB1_VIDEO,
		.name = MTK_VIDEO_CRTC_PROP_OSDB0_OSDB1_V},
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
		//set framesync ratio
		.prop_name = MTK_CRTC_PROP_FRAMESYNC_FRC_RATIO,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
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
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_DISPLAY_TIMING_INFO,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
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
		.range_info.max = 0xFFFFFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_VIDEO_LATENCY,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
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
		.flag = DRM_MODE_PROP_BLOB  | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_DEMO_PATTERN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFFFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_SW_SET_CTRL,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_AUTO_LD,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 3,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_XTendedRange,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 3,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_LDM_INIT,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 3,
		.init_val = 0,
	},
	/****pixelshift crtc prop******/
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_ENABLE,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 1,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_OSD_ATTACH,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 1,
		.init_val = 1,
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
		.range_info.min = -128,
		.range_info.max = 128,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CRTC_PROP_PIXELSHIFT_V,
		.flag = DRM_MODE_PROP_SIGNED_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = -128,
		.range_info.max = 128,
		.init_val = 0,
	},
	// demura property
	{
		.prop_name = MTK_CRTC_PROP_DEMURA_CONFIG,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	// pattern property
	{
		.prop_name = MTK_CRTC_PROP_PATTERN_GENERATE,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},

	/****global pq crtc prop******/
	{
		.prop_name = PROP_VIDEO_CRTC_PROP_GLOBAL_PQ,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	// video crtc mute color
	{
		.prop_name = MTK_VIDEO_CRTC_PROP_MUTE_COLOR,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	// video crtc background color
	{
		.prop_name = MTK_VIDEO_CRTC_PROP_BG_COLOR,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	// graphic and video property
	{
		.prop_name = MTK_VIDEO_CRTC_PROP_VG_ORDER,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_gop_order_enum_list[0],
		.enum_info.enum_length =
			ARRAY_SIZE(video_gop_order_enum_list),
		.init_val = 0,
	},
};

static const struct ext_prop_info ext_crtc_graphic_props_def[] = {
	/*****graphic crtc related properties*****/
	{
		.prop_name = MTK_CRTC_PROP_VG_SEPARATE,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
};

static struct drm_prop_enum_list crtc_type_enum_list[] = {
	{.type = CRTC_MAIN_PATH,
		.name = MTK_CRTC_COMMON_MAIN_PATH},
	{.type = CRTC_GRAPHIC_PATH,
		.name = MTK_CRTC_COMMON_GRAPHIC_PATH},
	{.type = CRTC_EXT_VIDEO_PATH,
		.name = MTK_CRTC_COMMON_EXT_VIDEO_PATH},
};

static const struct ext_prop_info ext_crtc_common_props_def[] = {
	/*****common crtc related properties*****/
	{
		.prop_name = MTK_CRTC_COMMON_PROP_CRTC_TYPE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &crtc_type_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(crtc_type_enum_list),
		.init_val = 0,
	},
};

static struct drm_prop_enum_list connector_disp_path_enum_list[] = {
	{.type = CONNECTOR_MAIN_PATH,
		.name = MTK_CONNECTOR_COMMON_MAIN_PATH},
	{.type = CONNECTOR_GRAPHIC_PATH,
		.name = MTK_CONNECTOR_COMMON_GRAPHIC_PATH},
	{.type = CONNECTOR_EXT_VIDEO_PATH,
		.name = MTK_CONNECTOR_COMMON_EXT_VIDEO_PATH},
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
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_MIRROR_STATUS,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
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
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_INFO,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_PARSE_OUT_INFO_FROM_DTS,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_VBO_CTRLBITS,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_TX_MUTE_EN,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_SWING_VREG,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_PRE_EMPHASIS,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_SSC,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_OUTPUT_MODEL,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = E_OUT_MODEL_VG_BLENDED,
		.range_info.max = E_OUT_MODEL_MAX,
		.init_val = E_OUT_MODEL_VG_BLENDED,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_CHECK_DTS_OUTPUT_TIMING,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_PROP_PNL_CHECK_FRAMESYNC_MLOAD,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
};

static const struct ext_prop_info ext_connector_common_props_def[] = {
	/*****common connector related properties*****/
	{
		.prop_name = MTK_CONNECTOR_COMMON_PROP_CONNECTOR_TYPE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &connector_disp_path_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(connector_disp_path_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_COMMON_PROP_PNL_TX_MUTE_EN,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		//.range_info.min = 0,
		//.range_info.max = 0xFFFF,
		//.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_COMMON_PROP_PNL_PIXEL_MUTE_EN,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		//.range_info.min = 0,
		//.range_info.max = 0xFFFF,
		//.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_COMMON_PROP_BACKLIGHT_MUTE_EN,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		//.range_info.min = 0,
		//.range_info.max = 0xFFFF,
		//.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_COMMON_PROP_HMIRROR_EN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_CONNECTOR_COMMON_PROP_PNL_VBO_CTRLBITS,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFFFF,
		.init_val = 0,
	},
};

struct mtk_drmcap_dev gDRMcap_capability;
static uint32_t _gu32IRQ_Version;

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
	gDRMcap_capability.u8OLEDPixelshiftSupport =
		drm_kms_dev->drmcap_dev.u8OLEDPixelshiftSupport;

}

static ssize_t mtk_drm_capability_DMA_SCL_NUM_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

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

static ssize_t mtk_drm_capability_MGW_NUM_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

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

static ssize_t mtk_drm_capability_MEMC_NUM_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

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

static ssize_t mtk_drm_capability_WINDOW_NUM_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

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

static ssize_t mtk_drm_capability_PANELGAMMA_BIT_NUM_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

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

static ssize_t mtk_drm_capability_OD_SUPPORT_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

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

static ssize_t mtk_drm_capability_OLED_DEMURAIN_SUPPORT_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

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

static ssize_t mtk_drm_capability_OLED_BOOSTING_SUPPORT_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

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

static ssize_t mtk_drm_capability_OLED_PIXELSHIFT_SUPPORT_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int ssize = 0;
	__u16 u16Size = SYS_DEVICE_SIZE;

	//OLED OLED_PIXELSHIFT_SUPPORT
	if (ssize > u16Size)
		DRM_ERROR("Failed to get OLED PIXELSHIFT support\n");
	else {
		ssize += snprintf(buf + ssize, u16Size - ssize, "%d\n",
			gDRMcap_capability.u8OLEDPixelshiftSupport);
		DRM_INFO("gDRMcap_capability.u8OLEDPixelshiftSupport:%d\n",
			gDRMcap_capability.u8OLEDPixelshiftSupport);
	}
	return ssize;
}

static ssize_t mtk_drm_OLED_PIXELSHIFT_H_RANGE_MAX_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	DRM_INFO("gDRMcap_capability.i8OLEDPixelshiftHRangeMax:%d\n",
		pctx->pixelshift_priv.i8OLEDPixelshiftHRangeMax);

	return snprintf(buf, SYS_DEVICE_SIZE, "%d\n",
		pctx->pixelshift_priv.i8OLEDPixelshiftHRangeMax);
}

static ssize_t mtk_drm_OLED_PIXELSHIFT_H_RANGE_MIN_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	DRM_INFO("gDRMcap_capability.i8OLEDPixelshiftHRangeMin:%d\n",
		pctx->pixelshift_priv.i8OLEDPixelshiftHRangeMin);

	return snprintf(buf, SYS_DEVICE_SIZE, "%d\n",
		pctx->pixelshift_priv.i8OLEDPixelshiftHRangeMin);
}

static ssize_t mtk_drm_OLED_PIXELSHIFT_V_RANGE_MAX_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	DRM_INFO("gDRMcap_capability.i8OLEDPixelshiftVRangeMax:%d\n",
		pctx->pixelshift_priv.i8OLEDPixelshiftVRangeMax);

	return snprintf(buf, SYS_DEVICE_SIZE, "%d\n",
		pctx->pixelshift_priv.i8OLEDPixelshiftVRangeMax);
}

static ssize_t mtk_drm_OLED_PIXELSHIFT_V_RANGE_MIN_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	DRM_INFO("gDRMcap_capability.i8OLEDPixelshiftVRangeMin:%d\n",
		pctx->pixelshift_priv.i8OLEDPixelshiftVRangeMin);

	return snprintf(buf, SYS_DEVICE_SIZE, "%d\n",
		pctx->pixelshift_priv.i8OLEDPixelshiftVRangeMin);
}

static ssize_t mtk_drm_capability_DMA_SCL_NUM_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_MGW_NUM_store
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_MEMC_NUM_store
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_WINDOW_NUM_store
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_PANELGAMMA_BIT_NUM_store
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_OD_SUPPORT_store
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_OLED_DEMURAIN_SUPPORT_store
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_OLED_BOOSTING_SUPPORT_store
(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return count;
}

static ssize_t pattern_multiwin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_MULTIWIN;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return 0;
	}

	count = mtk_drm_pattern_show(buf, dev, position);
	if (count < 0) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return count;
	}

	return count;
}

static ssize_t pattern_tcon_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_TCON;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	ret = mtk_drm_pattern_store(buf, dev, position);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return ret;
	}

	return count;
}

static ssize_t pattern_tcon_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_TCON;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return 0;
	}

	count = mtk_drm_pattern_show(buf, dev, position);
	if (count < 0) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return count;
	}

	return count;
}

static ssize_t pattern_gfx_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_GFX;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	ret = mtk_drm_pattern_store(buf, dev, position);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return ret;
	}

	return count;
}

static ssize_t pattern_gfx_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_GFX;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return 0;
	}

	count = mtk_drm_pattern_show(buf, dev, position);
	if (count < 0) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return count;
	}

	return count;
}

static ssize_t pattern_multiwin_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_MULTIWIN;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	ret = mtk_drm_pattern_store(buf, dev, position);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return ret;
	}

	return count;
}

static ssize_t pattern_opdram_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_OPDRAM;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	ret = mtk_drm_pattern_store(buf, dev, position);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return ret;
	}

	return count;
}

static ssize_t pattern_opdram_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_OPDRAM;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return 0;
	}

	count = mtk_drm_pattern_show(buf, dev, position);
	if (count < 0) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return count;
	}

	return count;
}

static ssize_t pattern_ipblend_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_IPBLEND;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	ret = mtk_drm_pattern_store(buf, dev, position);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return ret;
	}

	return count;
}

static ssize_t pattern_ipblend_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum drm_pattern_position position = DRM_PATTERN_POSITION_IPBLEND;

	if ((dev == NULL) || (buf == NULL)) {
		DRM_ERROR("[%s, %d]: Pointer is NULL!\n",
			__func__, __LINE__);
		return 0;
	}

	count = mtk_drm_pattern_show(buf, dev, position);
	if (count < 0) {
		DRM_ERROR("[%s, %d]: mtk_drm_pattern_show failed!\n",
			__func__, __LINE__);
		return count;
	}

	return count;
}

static ssize_t help_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRM Info log control bit:\n"
				"		- COMMON    = 0x%08lx\n"
				"		- DISPLAY   = 0x%08lx\n"
				"		- PANEL     = 0x%08lx\n"
				"		- CONNECTOR = 0x%08lx\n"
				"		- CRTC      = 0x%08lx\n"
				"		- DRV       = 0x%08lx\n"
				"		- ENCODER   = 0x%08lx\n"
				"		- FB        = 0x%08lx\n"
				"		- GEM       = 0x%08lx\n"
				"		- KMS       = 0x%08lx\n"
				"		- PATTERN   = 0x%08lx\n"
				"		- PLANE     = 0x%08lx\n"
				"		- VIDEO     = 0x%08lx\n"
				"		- GOP       = 0x%08lx\n"
				"		- DEMURA    = 0x%08lx\n"
				"		- PNLGAMMA  = 0x%08lx\n"
				"		- PQMAP     = 0x%08lx\n"
				"		- AUTOGEN   = 0x%08lx\n"
				"		- TVDAC     = 0x%08lx\n"
				"		- METABUF   = 0x%08lx\n"
				"		- FENCE     = 0x%08lx\n"
				"		- PQU_METADATA = 0x%08lx\n"
				"		- OLED      = 0x%08lx\n",
				MTK_DRM_MODEL_COMMON,
				MTK_DRM_MODEL_DISPLAY,
				MTK_DRM_MODEL_PANEL,
				MTK_DRM_MODEL_CONNECTOR,
				MTK_DRM_MODEL_CRTC,
				MTK_DRM_MODEL_DRV,
				MTK_DRM_MODEL_ENCODER,
				MTK_DRM_MODEL_FB,
				MTK_DRM_MODEL_GEM,
				MTK_DRM_MODEL_KMS,
				MTK_DRM_MODEL_PATTERN,
				MTK_DRM_MODEL_PLANE,
				MTK_DRM_MODEL_VIDEO,
				MTK_DRM_MODEL_GOP,
				MTK_DRM_MODEL_DEMURA,
				MTK_DRM_MODEL_PNLGAMMA,
				MTK_DRM_MODEL_PQMAP,
				MTK_DRM_MODEL_AUTOGEN,
				MTK_DRM_MODEL_TVDAC,
				MTK_DRM_MODEL_METABUF,
				MTK_DRM_MODEL_FENCE,
				MTK_DRM_MODEL_PQU_METADATA,
				MTK_DRM_MODEL_OLED);
}

static ssize_t log_level_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRM Info log status:\n"
				"		- COMMON    = %d\n"
				"		- DISPLAY   = %d\n"
				"		- PANEL     = %d\n"
				"		- CONNECTOR = %d\n"
				"		- CRTC      = %d\n"
				"		- DRV       = %d\n"
				"		- ENCODER   = %d\n"
				"		- FB        = %d\n"
				"		- GEM       = %d\n"
				"		- KMS       = %d\n"
				"		- PATTERN   = %d\n"
				"		- PLANE     = %d\n"
				"		- VIDEO     = %d\n"
				"		- GOP       = %d\n"
				"		- DEMURA    = %d\n"
				"		- PNLGAMMA  = %d\n"
				"		- PQMAP     = %d\n"
				"		- AUTOGEN   = %d\n"
				"		- TVDAC     = %d\n"
				"		- METABUF   = %d\n"
				"		- FENCE     = %d\n"
				"		- PQU_METADATA = %d\n"
				"		- OLED      = %d\n",
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_COMMON),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_DISPLAY),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_PANEL),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_CONNECTOR),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_CRTC),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_DRV),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_ENCODER),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_FB),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_GEM),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_KMS),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_PATTERN),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_PLANE),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_VIDEO),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_GOP),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_DEMURA),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_PNLGAMMA),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_PQMAP),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_AUTOGEN),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_TVDAC),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_METABUF),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_FENCE),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_PQU_METADATA),
				(bool)(mtk_drm_log_level & MTK_DRM_MODEL_OLED));
}

static ssize_t log_level_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);
	if (err)
		return err;

	mtk_drm_log_level = val;
	return count;
}

static ssize_t blend_hw_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->blend_hw_version);
}

static ssize_t panel_v_flip_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	bool vflip = (pctx->panel_priv.cus.mirror_mode == E_PNL_MIRROR_V ||
			      pctx->panel_priv.cus.mirror_mode == E_PNL_MIRROR_V_H);

	return snprintf(buf, PAGE_SIZE, "%d\n", vflip);
}

static ssize_t panel_h_flip_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	bool hflip = (pctx->panel_priv.cus.mirror_mode == E_PNL_MIRROR_H ||
			      pctx->panel_priv.cus.mirror_mode == E_PNL_MIRROR_V_H);

	return snprintf(buf, PAGE_SIZE, "%d\n", hflip);
}

static ssize_t debug_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_video_context *video = &pctx->video_priv;
	char *fmt = NULL;
	int len = 0;
	int i;

#define DEBUG_INFO_SHOW(title, size, elem) do { \
	len += snprintf(buf + len, PAGE_SIZE - len, "%s |", title); \
	for (i = 0; i < (size); i++) { \
		len += snprintf(buf + len, PAGE_SIZE - len, " %8llx |", (uint64_t)(elem)); \
	} \
	len += snprintf(buf + len, PAGE_SIZE - len, "\n"); \
} while (0)

	DEBUG_INFO_SHOW("Video plane type", MTK_VPLANE_TYPE_MAX, i);
	DEBUG_INFO_SHOW("  mgwdmaEnable  ", MTK_VPLANE_TYPE_MAX, video->mgwdmaEnable[i]);
	DEBUG_INFO_SHOW("  extWinEnable  ", MTK_VPLANE_TYPE_MAX, video->extWinEnable[i]);
	DEBUG_INFO_SHOW("  postfillEnable", MTK_VPLANE_TYPE_MAX, video->postfillEnable[i]);
	DEBUG_INFO_SHOW("  postfillEnable", MTK_VPLANE_TYPE_MAX, video->postfillEnable[i]);
	DEBUG_INFO_SHOW("  old_layer     ", MTK_VPLANE_TYPE_MAX, video->old_layer[i]);
	DEBUG_INFO_SHOW("  old_layer_en  ", MTK_VPLANE_TYPE_MAX, video->old_layer_en[i]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	DEBUG_INFO_SHOW("Video window  ", MAX_WINDOW_NUM, i);
	DEBUG_INFO_SHOW("  rwdiff      ", MAX_WINDOW_NUM, video->plane_ctx[i].rwdiff);
	DEBUG_INFO_SHOW("  protectVal  ", MAX_WINDOW_NUM, video->plane_ctx[i].protectVal);
	DEBUG_INFO_SHOW("  src_h       ", MAX_WINDOW_NUM, video->plane_ctx[i].src_h);
	DEBUG_INFO_SHOW("  src_w       ", MAX_WINDOW_NUM, video->plane_ctx[i].src_w);
	DEBUG_INFO_SHOW("  bufferheight", MAX_WINDOW_NUM, video->plane_ctx[i].oldbufferheight);
	DEBUG_INFO_SHOW("  bufferwidth ", MAX_WINDOW_NUM, video->plane_ctx[i].oldbufferwidth);
	DEBUG_INFO_SHOW("  hvs crop X  ", MAX_WINDOW_NUM, video->plane_ctx[i].hvspoldcropwindow.x);
	DEBUG_INFO_SHOW("  hvs crop Y  ", MAX_WINDOW_NUM, video->plane_ctx[i].hvspoldcropwindow.y);
	DEBUG_INFO_SHOW("  hvs crop W  ", MAX_WINDOW_NUM, video->plane_ctx[i].hvspoldcropwindow.w);
	DEBUG_INFO_SHOW("  hvs crop H  ", MAX_WINDOW_NUM, video->plane_ctx[i].hvspoldcropwindow.h);
	DEBUG_INFO_SHOW("  hvs disp X  ", MAX_WINDOW_NUM, video->plane_ctx[i].hvspolddispwindow.x);
	DEBUG_INFO_SHOW("  hvs disp Y  ", MAX_WINDOW_NUM, video->plane_ctx[i].hvspolddispwindow.y);
	DEBUG_INFO_SHOW("  hvs disp W  ", MAX_WINDOW_NUM, video->plane_ctx[i].hvspolddispwindow.w);
	DEBUG_INFO_SHOW("  hvs disp H  ", MAX_WINDOW_NUM, video->plane_ctx[i].hvspolddispwindow.h);
	DEBUG_INFO_SHOW("  disp X      ", MAX_WINDOW_NUM, video->plane_ctx[i].dispolddispwindow.x);
	DEBUG_INFO_SHOW("  disp Y      ", MAX_WINDOW_NUM, video->plane_ctx[i].dispolddispwindow.y);
	DEBUG_INFO_SHOW("  disp W      ", MAX_WINDOW_NUM, video->plane_ctx[i].dispolddispwindow.w);
	DEBUG_INFO_SHOW("  disp H      ", MAX_WINDOW_NUM, video->plane_ctx[i].dispolddispwindow.h);
	DEBUG_INFO_SHOW("  crop X      ", MAX_WINDOW_NUM, video->plane_ctx[i].oldcropwindow.x);
	DEBUG_INFO_SHOW("  crop Y      ", MAX_WINDOW_NUM, video->plane_ctx[i].oldcropwindow.y);
	DEBUG_INFO_SHOW("  crop W      ", MAX_WINDOW_NUM, video->plane_ctx[i].oldcropwindow.w);
	DEBUG_INFO_SHOW("  crop H      ", MAX_WINDOW_NUM, video->plane_ctx[i].oldcropwindow.h);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
#undef DEBUG_INFO_SHOW
	return len;
}


static ssize_t panel_mod_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct drm_st_mod_status stPnlModStatus = {0};

	mtk_render_get_mod_status(&stPnlModStatus);

	return snprintf(buf, PAGE_SIZE, "mod_status:\n"
				"		- vby1_locked(0:locked 1:unlock) = %d\n"
				"		- vby1_htpdn(0:locked 1:unlock) = %d\n"
				"		- vby1_lockn(0:locked 1:unlock) = %d\n"
				"		- vby1_unlockcnt = %d\n"
				"		- outconfig ch0~ch7 = 0x%04x, ch8~ch15= 0x%04x, ch16~ch19= 0x%04x\n",
				stPnlModStatus.vby1_locked,
				stPnlModStatus.vby1_htpdn,
				stPnlModStatus.vby1_lockn,
				stPnlModStatus.vby1_unlockcnt,
				stPnlModStatus.outconfig.outconfig_ch0_ch7,
				stPnlModStatus.outconfig.outconfig_ch8_ch15,
				stPnlModStatus.outconfig.outconfig_ch16_ch19);

}

static ssize_t panel_out_model_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->out_model);
}

static ssize_t pqgamma_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->pqgamma_version);
}

static ssize_t panel_resume_no_pattern_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	pctx->eResumePatternselect = E_RESUME_PATTERN_SEL_DEFAULT;

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->eResumePatternselect);
}

static ssize_t panel_resume_mod_pattern_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	pctx->eResumePatternselect = E_RESUME_PATTERN_SEL_MOD;

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->eResumePatternselect);
}

static ssize_t panel_resume_tcon_pattern_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	pctx->eResumePatternselect = E_RESUME_PATTERN_SEL_TCON;

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->eResumePatternselect);
}

static ssize_t panel_resume_gop_pattern_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	pctx->eResumePatternselect = E_RESUME_PATTERN_SEL_GOP;

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->eResumePatternselect);
}

static ssize_t panel_resume_multiwin_pattern_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	pctx->eResumePatternselect = E_RESUME_PATTERN_SEL_MULTIWIN;

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->eResumePatternselect);
}

static ssize_t frame_sync_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t input_vfreq = 0;
	int ret = 0;

	ret = mtk_get_framesync_locked_flag(pctx);
	if (crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID] != 0) {
		ret = mtk_video_get_vttv_input_vfreq(pctx,
			crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID],
			&input_vfreq);
	}

	return snprintf(buf, PAGE_SIZE, "frame_sync_status:\n"
				"		- frame_sync_mode = %d\n"
				"		- ivs = %d\n"
				"		- ovs = %d\n"
				"		- input src = %d\n"
				"		- input frame rate = %llu\n"
				"		- output frame rate = %d\n"
				"		- output VTotal = %d\n"
				"		- lock flag = %d\n",
				pctx->panel_priv.outputTiming_info.eFrameSyncMode,
				pctx->panel_priv.outputTiming_info.u8FRC_in,
				pctx->panel_priv.outputTiming_info.u8FRC_out,
				pctx->panel_priv.outputTiming_info.input_src,
				input_vfreq,
				pctx->panel_priv.outputTiming_info.u32OutputVfreq,
				pctx->panel_priv.outputTiming_info.u16OutputVTotal,
				pctx->panel_priv.outputTiming_info.locked_flag);

}

static ssize_t panel_mode_ID_Sel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	uint64_t Output_frame_rate = 0;
	__u16 typ_htt = pctx->panel_priv.info.typ_htt;
	__u16 typ_vtt = pctx->panel_priv.info.typ_vtt;

	if ((typ_htt != 0) && (typ_vtt != 0))
		Output_frame_rate =	pctx->panel_priv.info.typ_dclk / typ_htt / typ_vtt;

	return snprintf(buf, PAGE_SIZE, "Panel mode ID:\n"
				"		- Mode ID name = %s\n"
				"		- linkIF (0:None, 1:VB1, 2:LVDS) = %d\n"
				"		- VBO byte mode = %d\n"
				"		- div_section = %d\n"
				"		- Hsync Start = %d\n"
				"		- HSync Width = %d\n"
				"		- Hsync Polarity = %d\n"
				"		- Vsync Start = %d\n"
				"		- VSync Width = %d\n"
				"		- VSync Polarity = %d\n"
				"		- Panel HStart = %d\n"
				"		- Panel VStart = %d\n"
				"		- Panel Width = %d\n"
				"		- Panel Height = %d\n"
				"		- Panel HTotal = %d\n"
				"		- Panel MaxVTotal = %d\n"
				"		- Panel VTotal = %d\n"
				"		- Panel MinVTotal = %d\n"
				"		- Panel MaxDCLK = %llu\n"
				"		- Panel DCLK = %llu\n"
				"		- Panel MinDCLK = %llu\n"
				"		- Output frame rate = %llu\n"
				"		- Output Format = %d\n"
				"		- DTSI File Type = %d\n"
				"		- DTSI File Ver = %d\n"
				"		- SSC En = %d\n"
				"		- SSC Step = %d\n"
				"		- SSC Span = %d\n",
				pctx->panel_priv.info.pnlname,
				pctx->panel_priv.info.linkIF,
				pctx->panel_priv.info.vbo_byte,
				pctx->panel_priv.info.div_section,
				pctx->panel_priv.info.hsync_st,
				pctx->panel_priv.info.hsync_w,
				pctx->panel_priv.info.hsync_pol,
				pctx->panel_priv.info.vsync_st,
				pctx->panel_priv.info.vsync_w,
				pctx->panel_priv.info.vsync_pol,
				pctx->panel_priv.info.de_hstart,
				pctx->panel_priv.info.de_vstart,
				pctx->panel_priv.info.de_width,
				pctx->panel_priv.info.de_height,
				pctx->panel_priv.info.typ_htt,
				pctx->panel_priv.info.max_vtt,
				pctx->panel_priv.info.typ_vtt,
				pctx->panel_priv.info.min_vtt,
				pctx->panel_priv.info.max_dclk,
				pctx->panel_priv.info.typ_dclk,
				pctx->panel_priv.info.min_dclk,
				Output_frame_rate,
				pctx->panel_priv.info.op_format,
				pctx->panel_priv.cus.dtsi_file_type,
				pctx->panel_priv.cus.dtsi_file_ver,
				pctx->panel_priv.cus.ssc_en,
				pctx->panel_priv.cus.ssc_step,
				pctx->panel_priv.cus.ssc_span);
}

static ssize_t output_frame_rate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;

	return snprintf(buf, PAGE_SIZE, "%d\n", pctx->panel_priv.outputTiming_info.u32OutputVfreq);
}

static ssize_t panel_Backlight_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	gpiod_set_value(pctx->panel_priv.gpio_backlight, 1);
	return snprintf(buf, PAGE_SIZE, "BL on\n");
}

static ssize_t panel_Backlight_off_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	gpiod_set_value(pctx->panel_priv.gpio_backlight, 0);
	return snprintf(buf, PAGE_SIZE, "BL off\n");
}

static ssize_t main_video_pixel_mute_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	#define PATTERN_RED_VAL (0xFFF)
	drm_st_pixel_mute_info pixel_mute_info = {0};

	pixel_mute_info.bEnable = true;
	pixel_mute_info.u32Red = PATTERN_RED_VAL;
	pixel_mute_info.u32Green = 0;
	pixel_mute_info.u32Blue = 0;
	mtk_render_set_pixel_mute_video(pixel_mute_info);
	#undef PATTERN_RED_VAL
	return snprintf(buf, PAGE_SIZE, "main video pixel mute enable\n");
}

static ssize_t main_video_pixel_unmute_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	drm_st_pixel_mute_info pixel_mute_info = {0};

	pixel_mute_info.bEnable = false;
	pixel_mute_info.u32Red = 0;
	pixel_mute_info.u32Green = 0;
	pixel_mute_info.u32Blue = 0;
	mtk_render_set_pixel_mute_video(pixel_mute_info);
	return snprintf(buf, PAGE_SIZE, "main video pixel mute disable\n");
}

static ssize_t pattern_frc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_video_context *pctx_video = &pctx->video_priv;

	return snprintf(buf, PAGE_SIZE, "\n[frc pattern]%d\n", pctx_video->frc_pattern_sel);
}

static ssize_t pattern_frc_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);
	if (err)
		return err;

	mtk_video_display_frc_set_pattern(pctx, val);
	return count;
}

static ssize_t path_frc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_video_context *pctx_video = &pctx->video_priv;

	return snprintf(buf, PAGE_SIZE, "\n[frc path]%d\n", pctx_video->bIspre_memc_en_Status);
}

static ssize_t force_freerun_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;

	return snprintf(buf, PAGE_SIZE, "\nforce free run = %d\n",
		pctx_pnl->outputTiming_info.ForceFreeRun);
}

static ssize_t force_freerun_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);
	if (err)
		return err;

	pctx_pnl->outputTiming_info.ForceFreeRun = val&1;
	_mtk_video_set_framesync_mode(pctx);

	return count;
}

static ssize_t Disable_modeID_change_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;


	return snprintf(buf, PAGE_SIZE, "\nDisable_modeID_change = %d\n",
		pctx_pnl->disable_ModeID_change);
}

static ssize_t Disable_modeID_change_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);
	if (err)
		return err;

	pctx_pnl->disable_ModeID_change = val&1;

	return count;
}


static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RW(log_level);
static DEVICE_ATTR_RW(mtk_drm_capability_DMA_SCL_NUM);
static DEVICE_ATTR_RW(mtk_drm_capability_MGW_NUM);
static DEVICE_ATTR_RW(mtk_drm_capability_MEMC_NUM);
static DEVICE_ATTR_RW(mtk_drm_capability_WINDOW_NUM);
static DEVICE_ATTR_RW(mtk_drm_capability_PANELGAMMA_BIT_NUM);
static DEVICE_ATTR_RW(mtk_drm_capability_OD_SUPPORT);
static DEVICE_ATTR_RW(mtk_drm_capability_OLED_DEMURAIN_SUPPORT);
static DEVICE_ATTR_RW(mtk_drm_capability_OLED_BOOSTING_SUPPORT);
static DEVICE_ATTR_RW(pattern_multiwin);
static DEVICE_ATTR_RW(pattern_frc);
static DEVICE_ATTR_RO(path_frc);
static DEVICE_ATTR_RW(pattern_tcon);
static DEVICE_ATTR_RW(pattern_gfx);
static DEVICE_ATTR_RW(pattern_opdram);
static DEVICE_ATTR_RW(pattern_ipblend);
static DEVICE_ATTR_RO(blend_hw_version);
static DEVICE_ATTR_RO(panel_v_flip);
static DEVICE_ATTR_RO(panel_h_flip);
static DEVICE_ATTR_RO(debug_info);
static DEVICE_ATTR_RO(mtk_drm_capability_OLED_PIXELSHIFT_SUPPORT);
static DEVICE_ATTR_RO(mtk_drm_OLED_PIXELSHIFT_H_RANGE_MAX);
static DEVICE_ATTR_RO(mtk_drm_OLED_PIXELSHIFT_H_RANGE_MIN);
static DEVICE_ATTR_RO(mtk_drm_OLED_PIXELSHIFT_V_RANGE_MAX);
static DEVICE_ATTR_RO(mtk_drm_OLED_PIXELSHIFT_V_RANGE_MIN);
static DEVICE_ATTR_RO(panel_mod_status);
static DEVICE_ATTR_RO(panel_out_model);
static DEVICE_ATTR_RO(pqgamma_version);
static DEVICE_ATTR_RO(frame_sync_status);
static DEVICE_ATTR_RO(panel_mode_ID_Sel);
static DEVICE_ATTR_RO(panel_Backlight_on);
static DEVICE_ATTR_RO(panel_Backlight_off);
static DEVICE_ATTR_RO(output_frame_rate);
static DEVICE_ATTR_RO(main_video_pixel_mute);
static DEVICE_ATTR_RO(main_video_pixel_unmute);
static DEVICE_ATTR_RO(panel_resume_no_pattern);
static DEVICE_ATTR_RO(panel_resume_mod_pattern);
static DEVICE_ATTR_RO(panel_resume_tcon_pattern);
static DEVICE_ATTR_RO(panel_resume_gop_pattern);
static DEVICE_ATTR_RO(panel_resume_multiwin_pattern);
static DEVICE_ATTR_RW(force_freerun);
static DEVICE_ATTR_RW(Disable_modeID_change);

static struct attribute *mtk_tv_drm_kms_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	&dev_attr_mtk_drm_capability_DMA_SCL_NUM.attr,
	&dev_attr_mtk_drm_capability_MGW_NUM.attr,
	&dev_attr_mtk_drm_capability_MEMC_NUM.attr,
	&dev_attr_mtk_drm_capability_WINDOW_NUM.attr,
	&dev_attr_mtk_drm_capability_PANELGAMMA_BIT_NUM.attr,
	&dev_attr_mtk_drm_capability_OD_SUPPORT.attr,
	&dev_attr_mtk_drm_capability_OLED_DEMURAIN_SUPPORT.attr,
	&dev_attr_mtk_drm_capability_OLED_BOOSTING_SUPPORT.attr,
	&dev_attr_pattern_multiwin.attr,
	&dev_attr_pattern_frc.attr,
	&dev_attr_path_frc.attr,
	&dev_attr_pattern_tcon.attr,
	&dev_attr_pattern_gfx.attr,
	&dev_attr_pattern_opdram.attr,
	&dev_attr_pattern_ipblend.attr,
	&dev_attr_blend_hw_version.attr,
	&dev_attr_debug_info.attr,
	&dev_attr_mtk_drm_capability_OLED_PIXELSHIFT_SUPPORT.attr,
	&dev_attr_mtk_drm_OLED_PIXELSHIFT_H_RANGE_MAX.attr,
	&dev_attr_mtk_drm_OLED_PIXELSHIFT_H_RANGE_MIN.attr,
	&dev_attr_mtk_drm_OLED_PIXELSHIFT_V_RANGE_MAX.attr,
	&dev_attr_mtk_drm_OLED_PIXELSHIFT_V_RANGE_MIN.attr,
	&dev_attr_panel_mod_status.attr,
	&dev_attr_panel_out_model.attr,
	&dev_attr_frame_sync_status.attr,
	&dev_attr_panel_mode_ID_Sel.attr,
	&dev_attr_output_frame_rate.attr,
	&dev_attr_panel_Backlight_on.attr,
	&dev_attr_panel_Backlight_off.attr,
	&dev_attr_main_video_pixel_mute.attr,
	&dev_attr_main_video_pixel_unmute.attr,
	&dev_attr_panel_resume_no_pattern.attr,
	&dev_attr_panel_resume_mod_pattern.attr,
	&dev_attr_panel_resume_tcon_pattern.attr,
	&dev_attr_panel_resume_gop_pattern.attr,
	&dev_attr_panel_resume_multiwin_pattern.attr,
	&dev_attr_force_freerun.attr,
	&dev_attr_Disable_modeID_change.attr,
	NULL,
};

static struct attribute *mtk_tv_drm_kms_attrs_panel[] = {
	&dev_attr_panel_v_flip.attr,
	&dev_attr_panel_h_flip.attr,
	NULL
};

static const struct attribute_group mtk_tv_drm_kms_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_tv_drm_kms_attrs
};

static const struct attribute_group mtk_tv_drm_kms_attr_panel_group = {
	.name = "panel",
	.attrs = mtk_tv_drm_kms_attrs_panel
};

static void mtk_tv_kms_enable(
	struct mtk_tv_drm_crtc *crtc)
{
}

static void mtk_tv_kms_disable(
	struct mtk_tv_drm_crtc *crtc)
{
}

static int mtk_tv_kms_enable_vblank(
	struct mtk_tv_drm_crtc *crtc)
{
	int ret = 0;

	if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_GRAPHIC) {
		ret = mtk_gop_enable_vblank(crtc);
	} else if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO) {
		ret = mtk_common_IRQ_set_unmask(
			crtc, MTK_VIDEO_IRQTYPE_HK, MTK_VIDEO_SW_IRQ_TRIG_DISP, _gu32IRQ_Version);
	} else if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_EXT_VIDEO) {
		//ret = mtk_common_IRQ_set_unmask(
		//	crtc, MTK_VIDEO_IRQTYPE_HK, MTK_VIDEO_SW_IRQ_TRIG_DISP);
	} else {
		DRM_ERROR("[%s, %d]: Not support crtc type\n",
			__func__, __LINE__);
	}

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

	ret = mtk_video_panel_gamma_setting(crtc, r, g, b, size);

	return ret;
}

static int mtk_tv_kms_gamma_enable(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pnlgamma_enable *pnlgamma_enable)
{
	int ret = 0;

	ret = mtk_video_panel_gamma_enable(crtc, pnlgamma_enable);

	return ret;
}

static int mtk_tv_kms_gamma_gainoffset(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pnlgamma_gainoffset *pnl_gainoffset)
{
	int ret = 0;

	ret = mtk_video_panel_gamma_gainoffset(crtc, pnl_gainoffset);

	return ret;
}

static int mtk_tv_kms_gamma_gainoffset_enable(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pnlgamma_gainoffset_enable *pnl_gainoffsetenable)
{
	int ret = 0;

	ret = mtk_video_panel_gamma_gainoffset_enable(crtc, pnl_gainoffsetenable);

	return ret;
}

static int mtk_tv_pqgamma_set(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pqgamma_curve *curve)
{
	int ret = 0;

	ret = mtk_video_pqgamma_set(crtc, curve);

	return ret;
}

static int mtk_tv_pqgamma_enable(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pqgamma_enable *pqgamma_enable)
{
	int ret = 0;

	ret = mtk_video_pqgamma_enable(crtc, pqgamma_enable);

	return ret;
}

static int mtk_tv_pqgamma_gainoffset(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pqgamma_gainoffset *pq_gainoffset)
{
	int ret = 0;
	ret = mtk_video_pqgamma_gainoffset(crtc, pq_gainoffset);

	return ret;
}

static int mtk_tv_pqgamma_gainoffset_enable(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pqgamma_gainoffset_enable *pq_gainoffsetenable)
{
	int ret = 0;

	ret = mtk_video_pqgamma_gainoffset_enable(crtc, pq_gainoffsetenable);

	return ret;
}

static int mtk_tv_pqgamma_set_maxvalue(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pqgamma_maxvalue *pq_maxvalue)
{
	int ret = 0;

	ret = mtk_video_pqgamma_set_maxvalue(crtc, pq_maxvalue);

	return ret;
}

static int mtk_tv_pqgamma_maxvalue_enable(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_pqgamma_maxvalue_enable *pq_maxvalueenable)
{
	int ret = 0;

	ret = mtk_video_pqgamma_maxvalue_enable(crtc, pq_maxvalueenable);

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
		mtk_gop_update_plane(state);
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
		mtk_gop_disable_plane(state);
	} else {
		DRM_ERROR("[%s, %d]: unknown plane type %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
	}
}

static void mtk_tv_kms_disable_vblank(
	struct mtk_tv_drm_crtc *crtc)
{
	if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_GRAPHIC) {
		mtk_gop_disable_vblank(crtc);
	} else if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO) {
		//mtk_common_IRQ_set_mask(
		//	crtc, MTK_VIDEO_IRQTYPE_HK, MTK_VIDEO_SW_IRQ_TRIG_DISP, _gu32IRQ_Version);
	} else if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_EXT_VIDEO) {
		//mtk_common_IRQ_set_mask(
		//	crtc, MTK_VIDEO_IRQTYPE_HK, MTK_VIDEO_SW_IRQ_TRIG_DISP);
	} else {
		DRM_ERROR("[%s, %d]: Not support crtc type\n",
			__func__, __LINE__);
	}
}

static void mtk_video_atomic_crtc_begin(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state)
{
	struct sm_ml_fire_info fire_info;
	struct mtk_tv_kms_context *pctx = NULL;
	struct ext_crtc_prop_info *crtc_prop = NULL;
	struct mtk_video_context *pctx_video = NULL;
	uint8_t mem_index = 0;
	enum sm_return_type ret = 0;

	if (!crtc || !old_crtc_state) {
		DRM_ERROR("[%s, %d]: null ptr! crtc=0x%llx, old_crtc_state=0x%llx\n",
			__func__, __LINE__, crtc, old_crtc_state);
		return;
	}

	memset(&fire_info, 0, sizeof(struct sm_ml_fire_info));

	pctx = (struct mtk_tv_kms_context *)
	crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	crtc_prop = pctx->ext_crtc_properties;
	pctx_video = &pctx->video_priv;

	//2.get free memory index for next ml
	if (pctx_video->disp_ml.memindex == ML_INVALID_MEM_INDEX) {
		ret = sm_ml_get_mem_index(pctx_video->disp_ml.fd, &mem_index, false);
		if (ret != E_SM_RET_OK) {
			DRM_ERROR("[%s, %d]: fail for get free mem idx for next disp ml, ret=%d\n",
				__func__, __LINE__, ret);
		} else
			pctx_video->disp_ml.memindex = mem_index;
		DRM_INFO("[%s, %d]: disp ml mem_index=%d\n", __func__, __LINE__, mem_index);
	}
	if (pctx_video->vgs_ml.memindex == ML_INVALID_MEM_INDEX) {
		ret = sm_ml_get_mem_index(pctx_video->vgs_ml.fd, &mem_index, false);
		if (ret != E_SM_RET_OK) {
			DRM_ERROR("[%s, %d]: fail for get free mem idx for next vgs ml, ret=%d\n",
				__func__, __LINE__, ret);
		} else
			pctx_video->vgs_ml.memindex = mem_index;
		DRM_INFO("[%s, %d]: vgs ml mem_index=%d\n", __func__, __LINE__, mem_index);
	}

	/* video display crtc begin */
	//mtk_video_display_atomic_crtc_begin(pctx_video);
}

static void mtk_video_atomic_crtc_flush(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state)
{
	struct sm_ml_fire_info fire_info;
	struct mtk_tv_kms_context *pctx = NULL;
	struct ext_crtc_prop_info *crtc_prop = NULL;
	struct mtk_video_context *pctx_video = NULL;

	DRM_INFO("[%s][%d] atiomic flush start!\n", __func__, __LINE__);

	if (!crtc || !old_crtc_state) {
		DRM_ERROR("[%s, %d]: null ptr! crtc=0x%llx, old_crtc_state=0x%llx\n",
			__func__, __LINE__, crtc, old_crtc_state);
		return;
	}

	memset(&fire_info, 0, sizeof(struct sm_ml_fire_info));

	pctx = (struct mtk_tv_kms_context *)
		crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	crtc_prop = pctx->ext_crtc_properties;
	pctx_video = &pctx->video_priv;

	mtk_video_update_crtc(crtc);

	/* video display crtc flush */
	mtk_video_display_atomic_crtc_flush(pctx_video);
	mtk_video_pixelshift_atomic_crtc_flush(crtc, old_crtc_state);

	/* global pq crtc flush */
	mtk_global_pq_atomic_crtc_flush(&(pctx->global_pq_priv));

	if (pctx_video->disp_ml.regcount != 0) {
		uint8_t mem_index = 0;
		enum sm_return_type ret = 0;

		//4.fire
		fire_info.cmd_cnt = pctx_video->disp_ml.regcount;
		fire_info.external = FALSE;
		fire_info.mem_index = pctx_video->disp_ml.memindex;
		sm_ml_fire(pctx_video->disp_ml.fd, &fire_info);
		DRM_INFO("[%s][%d] pctx_video->disp_ml.regcount %d!!\n",
			__func__, __LINE__, pctx_video->disp_ml.regcount);
		pctx_video->disp_ml.regcount = 0;
		pctx_video->disp_ml.memindex = ML_INVALID_MEM_INDEX;

		mtk_video_panel_update_framesync_state(pctx);
	}

	if (pctx_video->vgs_ml.regcount != 0) {
		uint8_t mem_index = 0;
		enum sm_return_type ret = 0;

		//4.fire
		fire_info.cmd_cnt = pctx_video->vgs_ml.regcount;
		fire_info.external = FALSE;
		fire_info.mem_index = pctx_video->vgs_ml.memindex;
		sm_ml_fire(pctx_video->vgs_ml.fd, &fire_info);
		DRM_INFO("[%s][%d] pctx_video->vgs_ml.regcount %d!!\n",
			__func__, __LINE__, pctx_video->vgs_ml.regcount);
		pctx_video->vgs_ml.regcount = 0;
		pctx_video->vgs_ml.memindex = ML_INVALID_MEM_INDEX;
	}

	DRM_INFO("[%s][%d] atiomic flush end!\n", __func__, __LINE__);
}

static int mtk_commit_tail(void *data)
{
#if defined(__KERNEL__) /* Linux Kernel */
#if (defined ANDROID)
	MI_U32 u32Ret = MI_OK;

	u32Ret = MI_REALTIME_AddThread((MI_S8 *)"Domain_Render_DD",
			(MI_S8 *)"Class_KMS_Commit_tail_Video",
			(MI_S32)current->pid, NULL);

	if (u32Ret != MI_OK) {
		DRM_ERROR("[%s][%d] Realtime AddThread failed!\n", __func__, __LINE__);
	}
#endif
#endif

	while (!kthread_should_stop()) {
		const struct drm_mode_config_helper_funcs *funcs;
		struct drm_crtc_state *new_crtc_state;
		struct drm_crtc *crtc;
		ktime_t start;
		s64 commit_time_ms;
		unsigned int i, new_self_refresh_mask = 0;
		struct mtk_tv_kms_context *pctx = NULL;
		struct drm_atomic_state *old_state = NULL;
		struct drm_device *dev = NULL;

		wait_event_interruptible(atomic_commit_tail_wait, atomic_commit_tail_entry);
		atomic_commit_tail_entry = 0;

		pctx = (struct mtk_tv_kms_context *)(data);
		old_state = pctx->tvstate[MTK_DRM_CRTC_TYPE_VIDEO];
		dev = old_state->dev;

		DRM_INFO("[%s, %d] old_state=0x%lx\n", __func__, __LINE__, old_state);

		funcs = dev->mode_config.helper_private;

		// * We're measuring the _entire_ commit, so the time will vary depending
		// * on how many fences and objects are involved. For the purposes of self
		// * refresh, this is desirable since it'll give us an idea of how
		// * congested things are. This will inform our decision on how often we
		// * should enter self refresh after idle.
		// *
		// * These times will be averaged out in the self refresh helpers to avoid
		// * overreacting over one outlier frame
		start = ktime_get();

		drm_atomic_helper_wait_for_fences(dev, old_state, false);

		drm_atomic_helper_wait_for_dependencies(old_state);

		// * We cannot safely access new_crtc_state after
		// * drm_atomic_helper_commit_hw_done() so figure out which crtc's have
		// * self-refresh active beforehand:
		for_each_new_crtc_in_state(old_state, crtc, new_crtc_state, i)
			if (new_crtc_state->self_refresh_active)
				new_self_refresh_mask |= BIT(i);

		if (funcs && funcs->atomic_commit_tail)
			funcs->atomic_commit_tail(old_state);
		else
			drm_atomic_helper_commit_tail(old_state);

		commit_time_ms = ktime_ms_delta(ktime_get(), start);

		DRM_INFO("[%s, %d] commit_time_ms=%d\n",
			__func__, __LINE__, commit_time_ms);

		if (commit_time_ms > 0)
			drm_self_refresh_helper_update_avg_times(old_state,
							 (unsigned long)commit_time_ms,
							 new_self_refresh_mask);

		drm_atomic_helper_commit_cleanup_done(old_state);

		drm_atomic_state_put(old_state);

	}

#if defined(__KERNEL__) /* Linux Kernel */
#if (defined ANDROID)
	u32Ret = MI_REALTIME_RemoveThread((MI_S8 *)"Domain_Render_DD", (MI_S32)current->pid);
	if (u32Ret != MI_OK)
		DRM_ERROR("[%s][%d] Realtime RemoveThread failed!\n", __func__, __LINE__);
#endif
#endif

	return 0;
}

static int mtk_commit_tail_gCrtc(void *data)
{
#if defined(__KERNEL__) /* Linux Kernel */
#if (defined ANDROID)
	MI_U32 u32Ret = MI_OK;

	u32Ret = MI_REALTIME_AddThread((MI_S8 *)"Domain_Render_DD",
			(MI_S8 *)"Class_KMS_Commit_tail_Graphic",
			(MI_S32)current->pid, NULL);

	if (u32Ret != MI_OK)
		DRM_ERROR("[%s][%d] Realtime AddThread failed!\n", __func__, __LINE__);
#endif
#endif

	while (!kthread_should_stop()) {
		const struct drm_mode_config_helper_funcs *funcs;
		struct drm_crtc_state *new_crtc_state;
		struct drm_crtc *crtc;
		ktime_t start;
		s64 commit_time_ms;
		unsigned int i, new_self_refresh_mask = 0;
		struct mtk_tv_kms_context *pctx = NULL;
		struct drm_atomic_state *old_state = NULL;
		struct drm_device *dev = NULL;

		wait_event_interruptible(atomic_commit_tail_wait_Grapcrtc,
					atomic_commit_tail_Gcrtc_entry);
		atomic_commit_tail_Gcrtc_entry = 0;

		pctx = (struct mtk_tv_kms_context *)(data);
		old_state = pctx->tvstate[MTK_DRM_CRTC_TYPE_GRAPHIC];
		dev = old_state->dev;

		DRM_INFO("[%s, %d] old_state=0x%lx\n", __func__, __LINE__, old_state);

		funcs = dev->mode_config.helper_private;

		// * We're measuring the _entire_ commit, so the time will vary depending
		// * on how many fences and objects are involved. For the purposes of self
		// * refresh, this is desirable since it'll give us an idea of how
		// * congested things are. This will inform our decision on how often we
		// * should enter self refresh after idle.
		// *
		// * These times will be averaged out in the self refresh helpers to avoid
		// * overreacting over one outlier frame
		start = ktime_get();

		drm_atomic_helper_wait_for_fences(dev, old_state, false);

		drm_atomic_helper_wait_for_dependencies(old_state);

		// * We cannot safely access new_crtc_state after
		// * drm_atomic_helper_commit_hw_done() so figure out which crtc's have
		// * self-refresh active beforehand:
		for_each_new_crtc_in_state(old_state, crtc, new_crtc_state, i)
			if (new_crtc_state->self_refresh_active)
				new_self_refresh_mask |= BIT(i);

		if (funcs && funcs->atomic_commit_tail)
			funcs->atomic_commit_tail(old_state);
		else
			drm_atomic_helper_commit_tail(old_state);

		commit_time_ms = ktime_ms_delta(ktime_get(), start);

		DRM_INFO("[%s, %d] commit_time_ms=%d\n",
			__func__, __LINE__, commit_time_ms);

		if (commit_time_ms > 0)
			drm_self_refresh_helper_update_avg_times(old_state,
							 (unsigned long)commit_time_ms,
							 new_self_refresh_mask);

		drm_atomic_helper_commit_cleanup_done(old_state);

		drm_atomic_state_put(old_state);

	}

#if defined(__KERNEL__) /* Linux Kernel */
#if (defined ANDROID)
	u32Ret = MI_REALTIME_RemoveThread((MI_S8 *)"Domain_Render_DD", (MI_S32)current->pid);
	if (u32Ret != MI_OK)
		DRM_ERROR("[%s][%d] Realtime RemoveThread failed!\n", __func__, __LINE__);
#endif
#endif

	return 0;
}

int mtk_tv_drm_atomic_commit(struct drm_device *dev,
			     struct drm_atomic_state *state,
			     bool nonblock)
{
	int ret;

	DRM_INFO("[%s][%d] nonblock:%d\n", __func__, __LINE__, nonblock);

	start_crtc_begin = ktime_get();

	if (!nonblock) { //block
		ret = drm_atomic_helper_commit(dev, state, false);
	} else { // nonblock
		int i;
		struct drm_crtc *crtc = NULL;
		struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;
		struct drm_crtc_state *new_crtc_state;
		struct mtk_drm_plane *mplane = NULL;
		struct mtk_tv_kms_context *pctx = NULL;

		drm_for_each_crtc(crtc, dev) {
			mtk_tv_crtc = to_mtk_tv_crtc(crtc);
			mplane = mtk_tv_crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO];
			pctx = (struct mtk_tv_kms_context *)mplane->crtc_private;
			break;
		}

		if (state->async_update) {
			ret = drm_atomic_helper_prepare_planes(dev, state);
			if (ret)
				return ret;

			drm_atomic_helper_async_commit(dev, state);
			drm_atomic_helper_cleanup_planes(dev, state);

			return 0;
		}

		ret = drm_atomic_helper_setup_commit(state, nonblock);
		if (ret)
			return ret;

		//INIT_WORK(&state->commit_work, commit_work);

		ret = drm_atomic_helper_prepare_planes(dev, state);
		if (ret)
			return ret;

		/*
		 *if (!nonblock) {
		 *	ret = drm_atomic_helper_wait_for_fences(dev, state, true);
		 *	if (ret)
		 *		goto err;
		 *}
		 */

		/*
		 * This is the point of no return - everything below never fails except
		 * when the hw goes bonghits. Which means we can commit the new state on
		 * the software side now.
		 */

		ret = drm_atomic_helper_swap_state(state, true);
		if (ret)
			goto err;

		/*
		 * Everything below can be run asynchronously without the need to grab
		 * any modeset locks at all under one condition: It must be guaranteed
		 * that the asynchronous work has either been cancelled (if the driver
		 * supports it, which at least requires that the framebuffers get
		 * cleaned up with drm_atomic_helper_cleanup_planes()) or completed
		 * before the new state gets committed on the software side with
		 * drm_atomic_helper_swap_state().
		 *
		 * This scheme allows new atomic state updates to be prepared and
		 * checked in parallel to the asynchronous completion of the previous
		 * update. Which is important since compositors need to figure out the
		 * composition of the next frame right after having submitted the
		 * current layout.
		 *
		 * NOTE: Commit work has multiple phases, first hardware commit, then
		 * cleanup. We want them to overlap, hence need system_unbound_wq to
		 * make sure work items don't artificially stall on each another.
		 */

		drm_atomic_state_get(state);

		/*
		 *if (nonblock)
		 *	queue_work(system_unbound_wq, &state->commit_work);
		 *else
		 *	commit_tail(state);
		 */

		for_each_new_crtc_in_state(state, crtc, new_crtc_state, i) {
			mtk_tv_crtc = to_mtk_tv_crtc(crtc);
			if (mtk_tv_crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_GRAPHIC) {
				pctx->tvstate[MTK_DRM_CRTC_TYPE_GRAPHIC] = state;
				DRM_INFO("[%s, %d] state=0x%lx\n", __func__, __LINE__, state);
				atomic_commit_tail_Gcrtc_entry = 1;
				wake_up_interruptible(&atomic_commit_tail_wait_Grapcrtc);
				break;
			}

			if (mtk_tv_crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO ||
				mtk_tv_crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_EXT_VIDEO) {
				pctx->tvstate[MTK_DRM_CRTC_TYPE_VIDEO] = state;
				DRM_INFO("[%s, %d] state=0x%lx\n", __func__, __LINE__, state);
				atomic_commit_tail_entry = 1;
				wake_up_interruptible(&atomic_commit_tail_wait);
				break;
			}
		}

		return 0;

err:
		drm_atomic_helper_cleanup_planes(dev, state);

		return ret;

	}
	return ret;
}

static int mtk_tv_kms_atomic_set_crtc_common_property(
	struct mtk_tv_drm_crtc *mcrtc,
	struct drm_property *property,
	uint64_t val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mcrtc->crtc_private;
	struct ext_common_crtc_prop_info *crtc_common_prop =
		(pctx->ext_common_crtc_properties + mcrtc->mtk_crtc_type);

	for (i = 0; i < E_EXT_COMMON_CRTC_PROP_MAX; i++) {
		if (property == pctx->drm_common_crtc_prop[i]) {
			crtc_common_prop->prop_val[i] = val;
			crtc_common_prop->prop_update[i] = 1;
			ret = 0x0;
			break;
		}
	}

	return ret;
}

static int mtk_tv_kms_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val)
{
	uint64_t old_val = 0;
	int ret = -EINVAL;
	int i;

	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)crtc->crtc_private;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;

	ret = mtk_tv_kms_atomic_set_crtc_common_property(crtc, property, val);
	if (ret == 0) {
		// found in common property list, no need to find in other list
		return ret;
	}

	if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO) {
		for (i = 0; i < E_EXT_CRTC_PROP_MAX; i++) {
			if (property == pctx->drm_crtc_prop[i]) {
				old_val = crtc_props->prop_val[i];
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
			ret = mtk_video_pixelshift_atomic_set_crtc_property(
				crtc,
				state,
				property,
				val,
				i);
			if (ret != 0) {
				DRM_ERROR("[%s, %d]:set crtc pixelshift prop fail!!\n",
					__func__, __LINE__);
				return ret;
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
		} else if (GLOBAL_PQ_PROP(i)) {
			//global pq property
			ret = mtk_global_pq_atomic_set_crtc_property(crtc,
				state,
				property,
				val,
				i);
			if (ret != 0) {
				DRM_ERROR("[%s, %d]:set crtc GLOBAL_PQ prop fail!!\n",
					__func__, __LINE__);
				return ret;
			}
		} else if (IS_BLOB_PROP(property)) {
			// Avoid non-block mode maybe free the BLOB before commit_tail.
			// Alloc a memory to store the BLOB and update at mtk_video_update_crtc.
			struct drm_property_blob *property_blob = NULL;
			void *buffer = NULL;

			kfree((void *)old_val); // free old buffer
			crtc_props->prop_val[i] = 0; // reset

			property_blob = drm_property_lookup_blob(property->dev, val);
			if (property_blob == NULL) {
				DRM_ERROR("[%s][%d]prop '%s' has invalid blob id: 0x%llx",
					__func__, __LINE__, property->name, val);
				return -EINVAL;
			}
			buffer = kmalloc(property_blob->length, GFP_KERNEL);
			if (IS_ERR_OR_NULL(buffer)) {
				DRM_ERROR("[%s][%d]prop '%s' alloc buffer fail, size = %d",
					__func__, __LINE__, property->name, property_blob->length);
				return -ENOMEM;
			}
			memcpy(buffer, property_blob->data, property_blob->length);
			crtc_props->prop_val[i] = (uint64_t)buffer;
			DRM_INFO("[%s][%d]: Store crtc property %d, blob is %d\n",
					__func__, __LINE__, i, val);
		}
	} else if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_GRAPHIC) {
		ret = mtk_gop_atomic_set_crtc_property(crtc, state, property, val);

		mtk_tv_kms_set_VG_ratio(crtc);
	}
	return ret;
}

static int mtk_tv_kms_atomic_get_crtc_common_property(struct mtk_tv_drm_crtc *mcrtc,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mcrtc->crtc_private;
	struct ext_common_crtc_prop_info *crtc_common_prop =
		pctx->ext_common_crtc_properties + mcrtc->mtk_crtc_type;

	for (i = 0; i < E_EXT_COMMON_CRTC_PROP_MAX; i++) {
		if (property == pctx->drm_common_crtc_prop[i]) {
			*val = crtc_common_prop->prop_val[i];
			ret = 0x0;
			break;
		}
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
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)crtc->crtc_private;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;

	ret = mtk_tv_kms_atomic_get_crtc_common_property(crtc,
		property,
		val);

	if (ret == 0) {
		// found in common property list, no need to find in other list
		return ret;
	}

	if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO) {
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
			ret = mtk_video_pixelshift_atomic_get_crtc_property(
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
		} else if (GLOBAL_PQ_PROP(i)) {
			//global pq property
			ret = mtk_global_pq_atomic_get_crtc_property(crtc,
				state,
				property,
				val,
				i);
			if (ret != 0) {
				DRM_ERROR("[%s, %d]:get crtc GLOBAL_PQ prop fail!!\n",
					__func__, __LINE__);
				return ret;
			}
		} else if (IS_BLOB_PROP(property)) {
			// TODO
			// If the property is BLOB, create a blob and return the blob id.
			// Otherwise, set @val from @prop_val directly.
		}
	} else if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_GRAPHIC) {
		ret = mtk_gop_atomic_get_crtc_property(crtc, state, property, val);
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
		//no use
		ret = 0;
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
		//no use
		ret = 0;
	} else
		DRM_ERROR("[%s, %d]: unknown plane type %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);

	return ret;
}

void mtk_tv_kms_atomic_state_clear(struct drm_atomic_state *old_state)
{
	struct drm_plane *plane;
	struct drm_plane_state *old_plane_state, *new_plane_state;
	struct mtk_drm_plane *mplane = NULL;
	struct mtk_plane_state *state = NULL;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_video_context *pctx_video = NULL;
	unsigned int plane_inx = 0;
	struct mtk_video_plane_ctx *plane_ctx = NULL;
	int i;

	/* clear plane status */
	for_each_oldnew_plane_in_state(old_state, plane, old_plane_state, new_plane_state, i) {
		mplane = to_mtk_plane(plane);

		switch (mplane->mtk_plane_type) {
		case MTK_DRM_PLANE_TYPE_GRAPHIC:
			break;
		case MTK_DRM_PLANE_TYPE_VIDEO:
			mtk_video_display_clear_plane_status(to_mtk_plane_state(plane->state));
			break;
		case MTK_DRM_PLANE_TYPE_PRIMARY:
			break;
		default:
			DRM_ERROR("[%s, %d]: unknown plane type %d\n",
				__func__, __LINE__, mplane->mtk_plane_type);
			break;
		}
	}

	/* clear drm atomic status */
	drm_atomic_state_default_clear(old_state);
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

static void mtk_tv_kms_atomic_crtc_begin(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state)
{
	if (mtk_gop_ml_start(crtc, old_crtc_state)) {
		DRM_ERROR("[GOP][%s, %d]: mtk_gop_ml_start fail\n",
			__func__, __LINE__);
	}

	if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO)
		mtk_video_atomic_crtc_begin(crtc, old_crtc_state);
}

static void mtk_tv_kms_atomic_crtc_flush(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state)
{
	if (mtk_gop_ml_fire(crtc, old_crtc_state)) {
		DRM_ERROR("[GOP][%s, %d]: mtk_gop_ml_fire fail\n",
			__func__, __LINE__);
	}

	if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO)
		mtk_video_atomic_crtc_flush(crtc, old_crtc_state);

	mtk_tv_kms_CRTC_active_handler(crtc, old_crtc_state);
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
		ret = drm_atomic_helper_check_plane_state(plane_state, crtc_state,
						   DRM_PLANE_SCALE_CAPBILITY,
						   DRM_PLANE_HELPER_NO_SCALING,
						true, true);
	} else {
		DRM_ERROR("[%s, %d]: unknown plane type %d\n",
			__func__, __LINE__, mplane->mtk_plane_type);
		ret = -EINVAL;
	}
	return ret;
}

int mtk_tv_kms_get_fence(struct mtk_tv_drm_crtc *mcrtc,
	struct drm_mtk_tv_fence_info *fenceInfo)
{
	int ret = 0;
	int gopIdx = 0;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mcrtc->crtc_private;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	struct drm_plane *plane = NULL;
	struct mtk_drm_plane *mplane = NULL;
	uint64_t fence_value = 0;

	drm_for_each_plane(plane, mcrtc->base.dev) {
		mplane = to_mtk_plane(plane);
		if (mplane->mtk_plane_type == MTK_DRM_PLANE_TYPE_GRAPHIC) {
			gopIdx = mplane->gop_index;
			if (fenceInfo->bCreateFence[gopIdx]) {
				fence_value = pctx_gop->mfence[gopIdx].fence_seqno + 1;
				if (fence_value > INT_MAX)
					pctx_gop->mfence[gopIdx].fence_seqno = 0;
				else
					pctx_gop->mfence[gopIdx].fence_seqno = fence_value;

				if (mtk_tv_sync_fence_create(pctx_gop->gop_sync_timeline[gopIdx],
					&pctx_gop->mfence[gopIdx])) {
					pctx_gop->mfence[gopIdx].fence = MTK_TV_INVALID_FENCE_FD;
					ret |= -ENOMEM;
				}

				fenceInfo->FenceFd[gopIdx] = pctx_gop->mfence[gopIdx].fence;
				DRM_INFO("[%s][%d]gop:%d, fence fd:%d, val:%d\n",
					__func__, __LINE__,
					gopIdx,
					pctx_gop->mfence[gopIdx].fence,
					pctx_gop->mfence[gopIdx].fence_seqno);
			}
		}
	}

	return ret;
}

int mtk_tv_kms_timeline_inc(struct mtk_tv_drm_crtc *mcrtc,
	int planeIdx)
{
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mcrtc->crtc_private;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;

	if (planeIdx >= MAX_GOP_PLANE_NUM) {
		DRM_ERROR("[%s, %d]: Invalid plane idx %d\n",
			__func__, __LINE__, planeIdx);
		return -EINVAL;
	}
	mtk_tv_sync_timeline_inc(pctx_gop->gop_sync_timeline[planeIdx], 1);

	return 0;
}

int mtk_tv_set_graphic_pq_buf(struct mtk_tv_drm_crtc *mcrtc,
	struct drm_mtk_tv_graphic_pq_setting *PqBufInfo)
{
	int ret = 0;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mcrtc->crtc_private;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;

	return mtk_gop_set_pq_buf(pctx_gop, PqBufInfo);
}

int mtk_tv_get_graphic_roi(struct mtk_tv_drm_crtc *mcrtc,
	struct drm_mtk_tv_graphic_roi_info *RoiInfo)
{
	return mtk_gop_get_roi(mcrtc, RoiInfo);
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
	.atomic_crtc_begin = mtk_tv_kms_atomic_crtc_begin,
	.atomic_crtc_flush = mtk_tv_kms_atomic_crtc_flush,
	.gamma_set = mtk_tv_kms_gamma_set,
	.check_plane = mtk_tv_kms_check_plane,
	.graphic_set_testpattern = mtk_gop_set_testpattern,
	.set_stub_mode = mtk_render_common_set_stub_mode,
	.start_gfx_pqudata = mtk_gop_start_gfx_pqudata,
	.stop_gfx_pqudata = mtk_gop_stop_gfx_pqudata,
	.pnlgamma_enable = mtk_tv_kms_gamma_enable,
	.pnlgamma_gainoffset = mtk_tv_kms_gamma_gainoffset,
	.pnlgamma_gainoffset_enable = mtk_tv_kms_gamma_gainoffset_enable,
	.get_fence = mtk_tv_kms_get_fence,
	.pqgamma_curve = mtk_tv_pqgamma_set,
	.pqgamma_enable = mtk_tv_pqgamma_enable,
	.pqgamma_gainoffset = mtk_tv_pqgamma_gainoffset,
	.pqgamma_gainoffset_enable = mtk_tv_pqgamma_gainoffset_enable,
	.pqgamma_maxvalue = mtk_tv_pqgamma_set_maxvalue,
	.pqgamma_maxvalue_enable = mtk_tv_pqgamma_maxvalue_enable,
	.timeline_inc = mtk_tv_kms_timeline_inc,
	.set_graphic_pq_buf = mtk_tv_set_graphic_pq_buf,
	.get_graphic_roi = mtk_tv_get_graphic_roi,
};

/*Connector*/
static int mtk_tv_kms_atomic_get_connector_common_property(
	struct mtk_tv_drm_connector *connector,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)connector->connector_private;
	struct ext_common_connector_prop_info *connector_prop =
		pctx->ext_common_connector_properties + connector->mtk_connector_type;

	for (i = 0; i < E_EXT_CONNECTOR_COMMON_PROP_MAX; i++) {
		if (property == pctx->drm_common_connector_prop[i]) {
			*val = connector_prop->prop_val[i];
			ret = 0;
			break;
		}
	}

	return ret;
}

static int mtk_tv_kms_atomic_set_connector_common_property(
	struct mtk_tv_drm_connector *connector,
	struct drm_property *property,
	uint64_t val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)connector->connector_private;
	struct ext_common_connector_prop_info *connector_prop =
		pctx->ext_common_connector_properties + connector->mtk_connector_type;
	struct drm_property_blob *property_blob;

	for (i = 0; i < E_EXT_CONNECTOR_COMMON_PROP_MAX; i++) {
		if (property == pctx->drm_common_connector_prop[i]) {
			connector_prop->prop_val[i] = val;
			ret = 0;
			break;
		}
	}

	switch (i) {
	case E_EXT_CONNECTOR_COMMON_PROP_PNL_VBO_TX_MUTE_EN:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_PROP_PNL_VBO_TX_MUTE_EN = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		property_blob = drm_property_lookup_blob(property->dev, val);

		if (property_blob == NULL) {
			DRM_INFO("[%s][%d] unknown tx_mute_info status = %td!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			drm_st_tx_mute_info param;

			memset(&param, 0, sizeof(drm_st_tx_mute_info));
			memcpy(&param, property_blob->data, sizeof(drm_st_tx_mute_info));

			//ret = mtk_render_set_tx_mute(param);
			ret = mtk_render_set_tx_mute_common(&pctx->panel_priv, param,
			connector->mtk_connector_type);
			DRM_INFO("[%s][%d] set TX mute return = %td!!\n",
				__func__, __LINE__, ret);
		}
		break;
	case E_EXT_CONNECTOR_COMMON_PROP_PNL_PIXEL_MUTE_EN:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_PROP_PNL_VBO_TX_MUTE_EN = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);

		property_blob = drm_property_lookup_blob(property->dev, val);

		if (property_blob == NULL) {
			DRM_INFO("[%s][%d] unknown drm_st_pixel_mute_info status = %td!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			drm_st_pixel_mute_info param;

			memset(&param, 0, sizeof(drm_st_pixel_mute_info));
			memcpy(&param, property_blob->data, sizeof(drm_st_pixel_mute_info));

			//ret = mtk_render_set_tx_mute(param);
			if (connector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_VIDEO)
				ret = mtk_render_set_pixel_mute_video(param);

			if (pctx->panel_priv.hw_info.mod_hw_version == MOD_VER_2) {
				// not support
				ret = 0;
			} else {
				if (connector->mtk_connector_type ==
					MTK_DRM_CONNECTOR_TYPE_EXT_VIDEO)
					ret = mtk_render_set_pixel_mute_deltav(param);
			}

			if (connector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_GRAPHIC)
				ret = mtk_render_set_pixel_mute_gfx(param);
			DRM_INFO("[%s][%d] set TX mute return = %td!!\n",
				__func__, __LINE__, ret);
		}
		break;

	case E_EXT_CONNECTOR_COMMON_PROP_BACKLIGHT_MUTE_EN:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_COMMON_PROP_BACKLIGHT_MUTE_EN = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob == NULL) {
			DRM_INFO("[%s][%d] unknown drm_st_backlight_mute_info status = %td!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			drm_st_backlight_mute_info param;

			memset(&param, 0, sizeof(drm_st_backlight_mute_info));
			memcpy(&param, property_blob->data, sizeof(drm_st_backlight_mute_info));

			ret = mtk_render_set_backlight_mute(&pctx->panel_priv, param);

			DRM_INFO("[%s][%d] set Backlight mute return = %td!!\n",
				__func__, __LINE__, ret);
		}
		break;
	case E_EXT_CONNECTOR_COMMON_PROP_HMIRROR_EN:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_COMMON_PROP_HMIRROR_EN = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		pctx->panel_priv.cus.hmirror_en = (bool)val;
		ret = mtk_render_set_output_hmirror_enable(&pctx->panel_priv);

		DRM_INFO("[%s][%d] set Hmirror return = %td!!\n",
			__func__, __LINE__, ret);
		break;
	case E_EXT_CONNECTOR_COMMON_PROP_PNL_VBO_CTRLBIT:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_COMMON_PROP_PNL_VBO_CTRLBIT = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		property_blob = drm_property_lookup_blob(property->dev, val);
		if ((property_blob != NULL) && (val != NULL)) {
			struct drm_st_ctrlbits stCtrlbits;

			memset(&stCtrlbits, 0, sizeof(struct drm_st_ctrlbits));
			memcpy(&stCtrlbits, property_blob->data, sizeof(struct drm_st_ctrlbits));
			ret = mtk_render_set_vbo_ctrlbit(&pctx->panel_priv, &stCtrlbits);
			DRM_INFO("[%s][%d] PNL_VBO_CTRLBIT = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
		break;

	}
	return ret;
}

static int mtk_tv_kms_atomic_set_connector_property(
	struct mtk_tv_drm_connector *connector,
	struct drm_connector_state *state,
	struct drm_property *property,
	uint64_t val)
{
	int ret = -EINVAL;

	ret = mtk_tv_kms_atomic_set_connector_common_property(
		connector,
		property,
		val);

	if (ret == 0) {
		// found in common property list, no need to find in other list
		return ret;
	}

	if (connector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_VIDEO) {
		ret = mtk_video_atomic_set_connector_property(
			connector,
			state,
			property,
			val);
	} else if (connector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_GRAPHIC) {
		return 0;
	}

	return ret;
}

static int mtk_tv_kms_atomic_get_connector_property(
	struct mtk_tv_drm_connector *connector,
	const struct drm_connector_state *state,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = -EINVAL;

	ret = mtk_tv_kms_atomic_get_connector_common_property(
			connector,
			property,
			val);

	if (ret == 0) {
		// found in common property list, no need to find in other list
		return ret;
	}

	if (connector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_VIDEO) {
		ret = mtk_video_atomic_get_connector_property(
			connector,
			state,
			property,
			val);
	} else if (connector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_GRAPHIC) {
		return ret;
	}

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
	int u32Tmp = DTB_MSK;
	const char *name;
	int u32Local = DTB_MSK;

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_TV_KMS_COMPATIBLE_STR);
	if (np != NULL) {
		ret = of_irq_get(np, 0);
		if (ret < 0) {
			DRM_ERROR("[%s, %d]: of_irq_get failed\r\n",
				__func__, __LINE__);
			return ret;
		}
		pctx->video_irq_num = ret;

		ret = of_irq_get(np, 1);
		if (ret < 0) {
			DRM_ERROR("[%s, %d]: of_irq_get failed\r\n",
				__func__, __LINE__);
			return ret;
		}
		pctx->graphic_irq_vgsep_num = ret;

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

		//read video oled pixelshift
		name = "VIDEO_OLED_PIXELSHIFT";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		} else {
			pctx->drmcap_dev.u8OLEDPixelshiftSupport = (u32Tmp > 0 ? 1 : 0);
		}

		//read video oled pixelshift H_RANGE_MAX
		name = "VIDEO_OLED_PIXELSHIFT_H_RANGE_MAX";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		} else {
			if (u32Tmp < INT8_NEGATIVE)
				pctx->pixelshift_priv.i8OLEDPixelshiftHRangeMax = (int8_t)u32Tmp;
			else
				pctx->pixelshift_priv.i8OLEDPixelshiftHRangeMax =
					-(int8_t)(~u32Tmp&0xFF)-1;
		}

		//read video oled pixelshift H_RANGE_MIN
		name = "VIDEO_OLED_PIXELSHIFT_H_RANGE_MIN";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		} else {
			if (u32Tmp < INT8_NEGATIVE)
				pctx->pixelshift_priv.i8OLEDPixelshiftHRangeMin = (int8_t)u32Tmp;
			else
				pctx->pixelshift_priv.i8OLEDPixelshiftHRangeMin =
					-(int8_t)(~u32Tmp&0xFF)-1;
		}

		//read video oled pixelshift V_RANGE_MAX
		name = "VIDEO_OLED_PIXELSHIFT_V_RANGE_MAX";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		} else {
			if (u32Tmp < INT8_NEGATIVE)
				pctx->pixelshift_priv.i8OLEDPixelshiftVRangeMax = (int8_t)u32Tmp;
			else
				pctx->pixelshift_priv.i8OLEDPixelshiftVRangeMax =
					-(int8_t)(~u32Tmp&0xFF)-1;
		}

		//read video oled pixelshift V_RANGE_MIN
		name = "VIDEO_OLED_PIXELSHIFT_V_RANGE_MIN";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		} else {
			if (u32Tmp < INT8_NEGATIVE)
				pctx->pixelshift_priv.i8OLEDPixelshiftVRangeMin = (int8_t)u32Tmp;
			else
				pctx->pixelshift_priv.i8OLEDPixelshiftVRangeMin =
					-(int8_t)(~u32Tmp&0xFF)-1;
		}

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

		// read disp_x_align
		name = "disp_x_align";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.disp_x_align = u32Tmp;

		DRM_INFO("[%s][%d] disp_x_align:%d\n",
				__func__, __LINE__, pctx->video_priv.disp_x_align);

		// read disp_y_align
		name = "disp_y_align";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.disp_y_align = u32Tmp;

		DRM_INFO("[%s][%d] disp_y_align:%d\n",
				__func__, __LINE__, pctx->video_priv.disp_y_align);

		// read disp_w_align
		name = "disp_w_align";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.disp_w_align = u32Tmp;

		DRM_INFO("[%s][%d] disp_y_align:%d\n",
				__func__, __LINE__, pctx->video_priv.disp_w_align);

		// read disp_h_align
		name = "disp_h_align";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.disp_h_align = u32Tmp;

		DRM_INFO("[%s][%d] disp_y_align:%d\n",
				__func__, __LINE__, pctx->video_priv.disp_h_align);

		// read crop_x_align_420_422
		name = "crop_x_align_420_422";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.crop_x_align_420_422 = u32Tmp;

		DRM_INFO("[%s][%d] crop_x_align_420_422:%d\n",
				__func__, __LINE__, pctx->video_priv.crop_x_align_420_422);

		// read crop_x_align_444
		name = "crop_x_align_444";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.crop_x_align_444 = u32Tmp;

		DRM_INFO("[%s][%d] disp_y_align:%d\n",
				__func__, __LINE__, pctx->video_priv.crop_x_align_444);

		// read crop_y_align_420
		name = "crop_y_align_420";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.crop_y_align_420 = u32Tmp;

		DRM_INFO("[%s][%d] disp_y_align:%d\n",
				__func__, __LINE__, pctx->video_priv.crop_y_align_420);

		// read crop_y_align_444_422
		name = "crop_y_align_444_422";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.crop_y_align_444_422 = u32Tmp;

		DRM_INFO("[%s][%d] disp_y_align:%d\n",
				__func__, __LINE__, pctx->video_priv.crop_y_align_444_422);

		// read crop_w_align
		name = "crop_w_align";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.crop_w_align = u32Tmp;

		DRM_INFO("[%s][%d] disp_y_align:%d\n",
				__func__, __LINE__, pctx->video_priv.crop_w_align);

		// read crop_h_align
		name = "crop_h_align";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->video_priv.crop_h_align = u32Tmp;

		DRM_INFO("[%s][%d] disp_y_align:%d\n",
				__func__, __LINE__, pctx->video_priv.crop_h_align);

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

		// read render_p_engine
		name = "render_p_engine";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->render_p_engine = u32Tmp;

		// read hw version
		name = "blend_hw_version";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->blend_hw_version = u32Tmp;

		// read pnlgamma version
		name = "pnlgamma_version";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->pnlgamma_version = u32Tmp;

		// read pqgamma version
		name = "pqgamma_version";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->pqgamma_version = u32Tmp;

		//read irq version
		name = "IRQ_Version";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->u32IRQ_Version = u32Tmp;
		_gu32IRQ_Version = pctx->u32IRQ_Version;

		// read MGW version
		name = "MGW_VERSION";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->mgw_version = u32Tmp;

		//read clock versionmtk_tv_kms_irq_ML_get_memindex
		name = "Clk_Version";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx->clk_version = u32Tmp;
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

static int mtk_tv_kms_irq_ML_get_memindex(struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = NULL;
	uint8_t mem_index = 0;
	enum sm_return_type ret = E_SM_RET_OK;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return ret;
	}

	pctx_video = &pctx->video_priv;

	//2.get free memory index for next ml
	ret = sm_ml_get_mem_index(pctx_video->disp_ml.irq_fd, &mem_index, false);
	if (ret != E_SM_RET_OK) {
		DRM_ERROR("[%s, %d]: fail for get free mem idx for next disp ml, ret=%d\n",
			__func__, __LINE__, ret);
	} else
		pctx_video->disp_ml.irq_memindex = mem_index;
	DRM_INFO("[%s, %d]: irq_disp ml mem_index=%d\n", __func__, __LINE__, mem_index);
	return ret;
}

static int mtk_tv_kms_irq_ML_fire(struct mtk_tv_kms_context *pctx)
{
	struct sm_ml_fire_info fire_info;
	struct mtk_video_context *pctx_video = NULL;
	uint8_t mem_index = 0;
	enum sm_return_type ret = E_SM_RET_OK;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return ret;
	}

	memset(&fire_info, 0, sizeof(struct sm_ml_fire_info));

	pctx_video = &pctx->video_priv;

	//4.fire
	fire_info.cmd_cnt = pctx_video->disp_ml.irq_regcount;
	fire_info.external = FALSE;
	fire_info.mem_index = pctx_video->disp_ml.irq_memindex;
	ret = sm_ml_fire(pctx_video->disp_ml.irq_fd, &fire_info);
	if (ret != E_SM_RET_OK) {
		DRM_ERROR("[%s, %d]: fail to fire ml, ret=%d\n",
			__func__, __LINE__, ret);
	}
	DRM_INFO("[%s][%d] pctx_video->disp_ml.irq_regcount %d !!\n",
		__func__, __LINE__, pctx_video->disp_ml.irq_regcount);
	pctx_video->disp_ml.irq_regcount = 0;

	if (pctx->panel_priv.outputTiming_info.eFrameSyncState
		== E_PNL_FRAMESYNC_STATE_IRQ_IN)
		mtk_video_panel_update_framesync_state(pctx);
	return ret;
}

static irqreturn_t mtk_tv_kms_irq_handler(int irq, void *dev_id)
{
	//Fix me, should use video irq to handle crtc0
	//struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)dev_id;

	//mtk_gop_disable_vblank(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO]);

	//drm_crtc_handle_vblank(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].base);

	//if (pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].pending_needs_vblank == true) {
	//	mtk_tv_drm_crtc_finish_page_flip(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO]);
	//	pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].pending_needs_vblank = false;
	//}
	//mtk_gop_enable_vblank(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO]);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_tv_kms_irq_top_handler(int irq, void *dev_id)
{
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)dev_id;
	bool IRQstatus = 0;
	unsigned long atomic_diff;

	mtk_common_IRQ_get_status(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO],
		MTK_VIDEO_IRQTYPE_HK,
		MTK_VIDEO_SW_IRQ_TRIG_DISP,
		&IRQstatus,
		pctx->u32IRQ_Version);

	if (IRQstatus == TRUE) {
		static __s64 pre, cur;
		unsigned long irq_diff;
		mtk_common_IRQ_set_clr(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO],
			MTK_VIDEO_IRQTYPE_HK,
			MTK_VIDEO_SW_IRQ_TRIG_DISP,
			pctx->u32IRQ_Version);
		cur = ktime_get();
		atomic_diff = ktime_ms_delta(cur, start_crtc_begin);
		irq_diff = ktime_ms_delta(cur, pre);

		DRM_DEBUG_KMS("[RENDER AVSYNC]render dd display vsync diff : %ld(ms)\n", irq_diff);

		pre = cur;
		pctx->video_irq_status[MTK_VIDEO_SW_IRQ_TRIG_DISP] = TRUE;
		pctx->swdelay = atomic_diff;
	}

	return IRQ_WAKE_THREAD;
}

static irqreturn_t mtk_tv_kms_irq_bottom_handler(int irq, void *dev_id)
{
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)dev_id;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	struct mtk_video_plane_ctx *plane_ctx = NULL;

	bool IRQstatus = 0;
	int i = 0;
	unsigned int plane_index = 0;
	enum sm_return_type ret = 0;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;

	/* check MTK_VIDEO_SW_IRQ_TRIG_DISP */
	if (pctx->video_irq_status[MTK_VIDEO_SW_IRQ_TRIG_DISP] == TRUE) {
		drm_crtc_handle_vblank(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].base);
		drm_crtc_handle_vblank(&pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO].base);

		if (pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].pending_needs_vblank == true) {
			mtk_tv_drm_crtc_finish_page_flip(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO]);
			pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].pending_needs_vblank = false;
		}

		if (pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO].pending_needs_vblank == true) {
			mtk_tv_drm_crtc_finish_page_flip(&pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO]);
			pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO].pending_needs_vblank = false;
		}

		mtk_get_framesync_locked_flag(pctx);

		ret = mtk_tv_kms_irq_ML_get_memindex(pctx);
		if (ret == E_SM_RET_OK) {
			if ((mtk_video_panel_get_framesync_mode_en(pctx) == false) &&
				(pctx->panel_priv.outputTiming_info.eFrameSyncState ==
					E_PNL_FRAMESYNC_STATE_PROP_FIRE) &&
				(IS_FRAMESYNC(
					crtc_props->prop_val[E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE])
					== true) &&
				(pctx_pnl->outputTiming_info.AutoForceFreeRun
					== false)) {
				pctx->panel_priv.outputTiming_info.eFrameSyncState =
					E_PNL_FRAMESYNC_STATE_IRQ_IN;
				_mtk_video_set_framesync_mode(pctx);
			}


			for (plane_index = 0; plane_index < MAX_WINDOW_NUM; plane_index++) {
				plane_ctx = (pctx_video->plane_ctx + plane_index);

				if (plane_ctx->disp_mode_info.bUpdateRWdiffInNextV) {
					mtk_video_display_set_RW_diff_IRQ(plane_index, pctx_video);
					plane_ctx->disp_mode_info.bUpdateRWdiffInNextVDone = TRUE;
				} else {//reset
					plane_ctx->disp_mode_info.bUpdateRWdiffInNextVDone = FALSE;
				}
			}

			mtk_tv_kms_irq_ML_fire(pctx);
		}
		/* MTK_VIDEO_SW_IRQ_TRIG_DISP handled, clear status in pctx */
		pctx->video_irq_status[MTK_VIDEO_SW_IRQ_TRIG_DISP] = FALSE;
	} else if (pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].active_change) {
		if (pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].pending_needs_vblank == true) {
			mtk_tv_drm_crtc_finish_page_flip(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO]);
			pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].pending_needs_vblank = false;
		}
		pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].active_change = 0;
	} else if (pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO].active_change) {
		if (pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO].pending_needs_vblank == true) {
			mtk_tv_drm_crtc_finish_page_flip(&pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO]);
			pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO].pending_needs_vblank = false;
		}
		pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO].active_change = 0;
	}

      /*for control bit vsync gfid count */
	if (pctx->panel_priv.u8controlbit_gfid < MTK_DRM_KMS_GFID_MAX) { //id 0~255
		pctx->panel_priv.u8controlbit_gfid = pctx->panel_priv.u8controlbit_gfid + 1;
	} else {
		pctx->panel_priv.u8controlbit_gfid = 0;
	}

	// pqu_metadata set global frame id
	mtk_tv_drm_pqu_metadata_set_attr(&pctx->pqu_metadata_priv,
		MTK_PQU_METADATA_ATTR_GLOBAL_FRAME_ID, &pctx->panel_priv.u8controlbit_gfid);

	return IRQ_HANDLED;
}


static irqreturn_t mtk_tv_graphic_path_irq_handler(int irq, void *dev_id)
{
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)dev_id;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int i;

	mtk_gop_disable_vblank(&pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC]);

	drm_crtc_handle_vblank(&pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC].base);

	if (pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC].pending_needs_vblank == true) {
		mtk_tv_drm_crtc_finish_page_flip(&pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC]);
		pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC].pending_needs_vblank = false;
	}
	mtk_gop_enable_vblank(&pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC]);

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
	int i, plane_count = 0, connector_count = 0, crtc_count = 0;
	struct mtk_drm_plane *mplane = NULL;
	struct drm_mode_object *obj = NULL;
	struct drm_plane *plane = NULL;
	unsigned int zpos_min = 0;
	unsigned int zpos_max = 0;
	uint64_t init_val;
	struct drm_connector *connector;
	enum drm_mtk_connector_type mtk_connector_type = MTK_DRM_CONNECTOR_TYPE_MAX;
	struct drm_connector_list_iter conn_iter;
	struct mtk_tv_drm_connector *mconnector;
	struct drm_crtc *crtc;
	struct mtk_tv_drm_crtc *mtk_tv_crtc;

	struct drm_property_blob *propBlob;
	//struct drm_property *propMemcStatus = NULL;
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
	} else if (prop_type == E_EXT_PROP_TYPE_CRTC_GRAPHIC) {
		prop_def = ext_crtc_graphic_props_def;
		extend_prop_count = ARRAY_SIZE(ext_crtc_graphic_props_def);
	} else if (prop_type == E_EXT_PROP_TYPE_CRTC_COMMON) {
		prop_def = ext_crtc_common_props_def;
		extend_prop_count = ARRAY_SIZE(ext_crtc_common_props_def);
	} else if (prop_type == E_EXT_PROP_TYPE_CONNECTOR_COMMON) {
		prop_def = ext_connector_common_props_def;
		extend_prop_count = ARRAY_SIZE(ext_connector_common_props_def);
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
		} else if (ext_prop->flag & DRM_MODE_PROP_BLOB) {
			prop = drm_property_create(pctx->drm_dev,
			DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
			ext_prop->prop_name, 0);
			if (prop == NULL) {
				DRM_ERROR("[%s, %d]: create ext blob prop fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}
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
			obj = &(pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].base.base);

			// attach property by init value
			drm_object_attach_property(
				obj,
				prop,
				ext_prop->init_val);

			pctx->drm_crtc_prop[i] =
				prop;
			pctx->ext_crtc_properties->prop_val[i] =
				ext_prop->init_val;
		} else if (prop_type == E_EXT_PROP_TYPE_CRTC_GRAPHIC) {
			obj = &(pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC].base.base);
			drm_object_attach_property(obj, prop, ext_prop->init_val);
			pctx->drm_crtc_graphic_prop[i] = prop;
			pctx->ext_crtc_graphic_properties->prop_val[i] = ext_prop->init_val;
		} else if (prop_type == E_EXT_PROP_TYPE_CRTC_COMMON) {
			pctx->drm_common_crtc_prop[i] = prop;
			drm_for_each_crtc(crtc, drm_dev) {
				mtk_tv_crtc = to_mtk_tv_crtc(crtc);
				obj = &(pctx->crtc[crtc_count].base.base);
				if (strcmp(prop->name, MTK_CRTC_COMMON_PROP_CRTC_TYPE) == 0) {
					drm_object_attach_property(obj, prop,
								mtk_tv_crtc->mtk_crtc_type);
					(pctx->ext_common_crtc_properties +
					crtc_count)->prop_val[i] = mtk_tv_crtc->mtk_crtc_type;
					++crtc_count;
				} else {
					drm_object_attach_property(obj, prop,
									ext_prop->init_val);
					(pctx->ext_common_crtc_properties +
					crtc_count)->prop_val[i] = ext_prop->init_val;
					++crtc_count;
				}
			}
			crtc_count = 0;
		} else if (prop_type == E_EXT_PROP_TYPE_CONNECTOR) {
			obj = &(pctx->connector[MTK_DRM_CONNECTOR_TYPE_VIDEO].base.base);
			drm_object_attach_property(obj,
				prop,
				ext_prop->init_val);
			pctx->drm_connector_prop[i] =
				prop;
			pctx->ext_connector_properties->prop_val[i] =
				ext_prop->init_val;
		} else if (prop_type == E_EXT_PROP_TYPE_CONNECTOR_COMMON) {
			pctx->drm_common_connector_prop[i] = prop;
			drm_connector_list_iter_begin(drm_dev, &conn_iter);
			drm_for_each_connector_iter(connector, &conn_iter) {
				mconnector = to_mtk_tv_connector(connector);
				obj = &(mconnector->base.base);
				if (strcmp(prop->name,
					MTK_CONNECTOR_COMMON_PROP_CONNECTOR_TYPE) == 0) {
					mtk_connector_type = mconnector->mtk_connector_type;
					(pctx->ext_common_connector_properties + connector_count)
					->prop_val[i] = mtk_connector_type;
					++connector_count;
					drm_object_attach_property(obj, prop, mtk_connector_type);
				} else {
					drm_object_attach_property(obj, prop,
									ext_prop->init_val);
					(pctx->ext_common_connector_properties +
					connector_count)->prop_val[i] = ext_prop->init_val;
					++connector_count;
				}
			}
			connector_count = 0;
			drm_connector_list_iter_end(&conn_iter);

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
#if (0)
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
#endif
		}
	}

	return 0;

NOMEM:
	return -ENOMEM;
}

int mtk_tv_kms_CRTC_active_handler(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state)
{
	struct drm_crtc drm_crtc_base = crtc->base;
	struct drm_crtc *ptrdrm_crtc = &drm_crtc_base;
	struct drm_crtc_state *cur_state = drm_crtc_base.state;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->crtc_private;
	struct device_node *np = NULL;
	struct drm_panel *pDrmPanel = NULL;

	pDrmPanel = pctx->connector[0].base.panel;

	if (cur_state->active_changed) {
		DRM_INFO("[%s][%d] active:%d, active_change: %d\n", __func__, __LINE__,
			cur_state->active, cur_state->active_changed);
		if (cur_state->active == 0) {
			if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO) {
				crtc->active_change = 1;
				pDrmPanel->funcs->disable(pDrmPanel); //disable BL
				//pDrmPanel->funcs->unprepare(pDrmPanel); //disable VCC
			} else if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_EXT_VIDEO) {
				crtc->active_change = 1;
			} else if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_GRAPHIC) {
				if (pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC].pending_needs_vblank
					== true) {
					mtk_tv_drm_crtc_finish_page_flip(
						&pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC]);
					pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC].pending_needs_vblank
						= false;
				}
			}
		} else {
			if (crtc->mtk_crtc_type == MTK_DRM_CRTC_TYPE_VIDEO) {
				//pDrmPanel->funcs->prepare(pDrmPanel); //enable VCC
				pDrmPanel->funcs->enable(pDrmPanel); // enable BL
			}
		}
	} else {
	}
	return 0;
}



int mtk_tv_kms_set_VG_ratio(struct mtk_tv_drm_crtc *mtk_tv_crtc)
{
	#define GVreqDivRatio2 2
	if (!mtk_tv_crtc) {
		DRM_ERROR("[%s, %d]: null ptr! crtc=0x%llx\n",
			__func__, __LINE__, mtk_tv_crtc);
		return -EINVAL;
	}

	int ret = 0;
	bool bRIU = true, bEnable = 0;
	uint64_t prop_val;
	struct hwregOut paramOut;
	struct drm_property_blob *property_blob = NULL;
	struct drm_mtk_tv_graphic_path_mode stVGSepMode;
	struct drm_device *drm_dev;
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct ext_crtc_graphic_prop_info *graphic_crtc_properties =
		pctx->ext_crtc_graphic_properties;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];

	// if CRTC_VG_SEP is not update, do nothing
	if (graphic_crtc_properties->prop_update[E_EXT_CRTC_GRAPHIC_PROP_MODE]
		== 0) {
		return 0;
	}

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	prop_val = pctx->ext_crtc_graphic_properties->prop_val[E_EXT_CRTC_GRAPHIC_PROP_MODE];
	drm_dev = pctx->drm_dev;
	property_blob = drm_property_lookup_blob(pctx->drm_dev, prop_val);

	DRM_INFO("[%s][%d] drm_dev:%d, prop_val: %d, property_blob:%d\n",
		__func__, __LINE__, drm_dev, prop_val, property_blob);
	if (property_blob != NULL) {
		memset(&stVGSepMode, 0, sizeof(struct drm_mtk_tv_graphic_path_mode));
		memcpy(&stVGSepMode,
			property_blob->data,
			property_blob->length);
	} else {
		DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
			__func__, __LINE__, (ptrdiff_t)prop_val);
		return -EINVAL;
	}
	DRM_INFO("[%s][%d] VGsync_mode:%d, GVreqDivRatio:%d\n",
		__func__, __LINE__, stVGSepMode.VGsync_mode, stVGSepMode.GVreqDivRatio);

	if (stVGSepMode.GVreqDivRatio == GVreqDivRatio2)
		bEnable = 1;
	else
		bEnable = 0;

	//enable GOP ref mask
	//BKA3A3_h13[0] = enable
	ret = drv_hwreg_render_pnl_tgen_set_gop_ref_mask(
		&paramOut,
		bRIU,
		bEnable);

	#undef GVreqDivRatio2

	return ret;
}

static int mtk_tv_drm_trigger_gen_init(struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	uint16_t reg_num = pctx_video->reg_num;

	struct reg_info reg[reg_num];
	struct hwregTrigGenInputSrcSelIn paramIn;
	struct hwregTrigGenVSDlyIn paramInVSDly;
	struct hwregTrigGenDMARDlyIn paramInDMARDly;
	struct hwregTrigGenSTRDlyIn paramInSTRDly;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregTrigGenInputSrcSelIn));
	memset(&paramInVSDly, 0, sizeof(struct hwregTrigGenVSDlyIn));
	memset(&paramInDMARDly, 0, sizeof(struct hwregTrigGenDMARDlyIn));
	memset(&paramInSTRDly, 0, sizeof(struct hwregTrigGenSTRDlyIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.domain = RENDER_COMMON_TRIGEN_DOMAIN_DISP;
	paramIn.src = RENDER_COMMON_TRIGEN_INPUT_TGEN;

	DRM_INFO("[%s][%d][paramIn] RIU:%d domain:%d, src:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.domain, paramIn.src);

	drv_hwreg_render_common_triggen_set_inputSrcSel(
			paramIn, &paramOut);

	paramIn.RIU = 1;
	paramIn.domain = RENDER_COMMON_TRIGEN_DOMAIN_FRC1;
	paramIn.src = RENDER_COMMON_TRIGEN_INPUT_IP0;

	DRM_INFO("[%s][%d][paramIn] RIU:%d domain:%d, src:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.domain, paramIn.src);

	drv_hwreg_render_common_triggen_set_inputSrcSel(
			paramIn, &paramOut);

	paramIn.RIU = 1;
	paramIn.domain = RENDER_COMMON_TRIGEN_DOMAIN_FRC2;
	paramIn.src = RENDER_COMMON_TRIGEN_INPUT_IP0;

	DRM_INFO("[%s][%d][paramIn] RIU:%d domain:%d, src:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.domain, paramIn.src);

	drv_hwreg_render_common_triggen_set_inputSrcSel(
			paramIn, &paramOut);

	paramIn.RIU = 1;
	paramIn.domain = RENDER_COMMON_TRIGEN_DOMAIN_TGEN;
	paramIn.src = RENDER_COMMON_TRIGEN_INPUT_TGEN;

	DRM_INFO("[%s][%d][paramIn] RIU:%d domain:%d, src:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.domain, paramIn.src);

	drv_hwreg_render_common_triggen_set_inputSrcSel(
			paramIn, &paramOut);

	paramInVSDly.RIU = 1;
	paramInVSDly.domain = RENDER_COMMON_TRIGEN_DOMAIN_DISP;
	paramInVSDly.vs_dly_h = 1;
	paramInVSDly.vs_dly_v = MTK_DRM_TRIGGEN_VSDLY_V;
	paramInVSDly.vs_trig_sel = 0;
	drv_hwreg_render_common_triggen_set_vsyncTrig(
		paramInVSDly, &paramOut);

	paramInDMARDly.RIU = 1;
	paramInDMARDly.domain = RENDER_COMMON_TRIGEN_DOMAIN_DISP;
	paramInDMARDly.dma_r_dly_h = 1;
	paramInDMARDly.dma_r_dly_v = MTK_DRM_TRIGGEN_DMARDDLY_V;
	paramInDMARDly.dmar_trig_sel = 0;
	drv_hwreg_render_common_triggen_set_dmaRdTrig(
		paramInDMARDly, &paramOut);

	paramInSTRDly.RIU = 1;
	paramInSTRDly.domain = RENDER_COMMON_TRIGEN_DOMAIN_DISP;
	paramInSTRDly.str_dly_v = MTK_DRM_TRIGGEN_STRDLY_V;
	paramInSTRDly.str_trig_sel = 0;
	drv_hwreg_render_common_triggen_set_strTrig(
		paramInSTRDly, &paramOut);

	return 0;
}

static int mtk_tv_drm_ML_init(struct mtk_video_context *pctx_video)
{
	int DISP_fd, VGS_fd, DISP_irq_fd = 0;
	struct sm_ml_res stRes;
	uint8_t DISP_MemIndex = 0, VGS_MemIndex = 0, DISP_irq_MemIndex = 0;

	// allocate ML buffer for DISP
	memset(&stRes, 0, sizeof(struct sm_ml_res));
	//1.create instance
	stRes.sync_id = E_SM_ML_DISP_SYNC;
	stRes.buffer_cnt = MTK_DRM_ML_DISP_BUF_COUNT;
	stRes.cmd_cnt = MTK_DRM_ML_DISP_CMD_COUNT;//for one frame reg count
	if (sm_ml_create_resource(&DISP_fd, &stRes) != E_SM_RET_OK) {
		DRM_ERROR("[%s][%d]can not create instance!",
		__func__, __LINE__);
		return -EINVAL;
	}

	pctx_video->disp_ml.memindex = DISP_MemIndex;
	pctx_video->disp_ml.fd = DISP_fd;


	// allocate ML buffer for DISP_irq
	memset(&stRes, 0, sizeof(struct sm_ml_res));
	//1.create instance
	stRes.sync_id = E_SM_ML_DISP_SYNC;
	stRes.buffer_cnt = MTK_DRM_ML_DISP_IRQ_BUF_COUNT;
	stRes.cmd_cnt = MTK_DRM_ML_DISP_IRQ_CMD_COUNT;//for one frame reg count
	if (sm_ml_create_resource(&DISP_irq_fd, &stRes) != E_SM_RET_OK) {
		DRM_ERROR("[%s][%d]can not create instance!",
		__func__, __LINE__);
		return -EINVAL;
	}

	pctx_video->disp_ml.irq_memindex = DISP_irq_MemIndex;
	pctx_video->disp_ml.irq_fd = DISP_irq_fd;


	// allocate ML buffer for VGS
	memset(&stRes, 0, sizeof(struct sm_ml_res));
	//1.create instance
	stRes.sync_id = E_SM_ML_VGS_SYNC;
	stRes.buffer_cnt = 3;
	stRes.cmd_cnt = 100;//for one frame reg count
	if (sm_ml_create_resource(&VGS_fd, &stRes) != E_SM_RET_OK) {
		DRM_ERROR("[%s][%d]can not create instance!",
		__func__, __LINE__);
		return -EINVAL;
	}

	pctx_video->vgs_ml.fd = VGS_fd;
	pctx_video->vgs_ml.memindex = VGS_MemIndex;

	return 0;
}

static void mtk_tv_drm_send_sharememtopqu(void)
{
	struct mtk_tv_drm_metabuf metabuf;
	struct msg_render_pq_sharemem_info pq_info;
	struct pqu_dd_update_pq_sharemem_param shareinfo;

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return;
	}

	pq_info.address = metabuf.addr;
	pq_info.size = sizeof(struct meta_render_pqu_pq_info);

	shareinfo.address  = metabuf.mmap_info.bus_addr;
	shareinfo.size = sizeof(struct meta_render_pqu_pq_info);

	if (bPquEnable) {
		pqu_render_dd_pq_update_sharemem(
		(const struct pqu_dd_update_pq_sharemem_param *)&shareinfo, NULL);
		mtk_tv_drm_metabuf_free(&metabuf);
	} else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQSHAREMEM_INFO, &pq_info);

}

static int mtk_tv_kms_bind(
	struct device *dev,
	struct device *master,
	void *data)
{
#if defined(__KERNEL__) /* Linux Kernel */
#if (defined ANDROID)
	MI_U32 u32Ret = 0;
#endif
#endif
	int ret = 0, i = 0;
	struct mtk_tv_kms_context *pctx = NULL;
	struct drm_device *drm_dev = NULL;
	struct mtk_drm_plane *primary_plane = NULL;
	struct mtk_drm_plane *cursor_plane = NULL;
	int plane_type = 0;
	struct mtk_video_context *pctx_video = NULL;
	struct task_struct  *crtc0_commit_tail_worker;
	struct task_struct  *crtc1_commit_tail_worker;
	struct sched_param crtc0_param, crtc1_param;
	struct cpumask crtc0_task_cmask, crtc1_task_cmask;

	if (!dev) {
		DRM_ERROR("[%s, %d]: dev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	pctx = dev_get_drvdata(dev);
	pctx_video = &pctx->video_priv;
	memset(&pctx_video->old_layer, OLD_LAYER_INITIAL_VALUE,
		sizeof(uint8_t)*MTK_VPLANE_TYPE_MAX);

	if (!data) {
		DRM_ERROR("[%s, %d]: drm_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	drm_dev = data;

	pctx->drm_dev = drm_dev;
	pctx->crtc[MTK_DRM_CRTC_TYPE_GRAPHIC].crtc_private = pctx;
	pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].crtc_private = pctx;
	pctx->crtc[MTK_DRM_CRTC_TYPE_EXT_VIDEO].crtc_private = pctx;

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

	pctx->ext_crtc_graphic_properties = devm_kzalloc(
		dev,
		sizeof(struct ext_crtc_graphic_prop_info),
		GFP_KERNEL);
	if (pctx->ext_crtc_graphic_properties == NULL) {
		DRM_ERROR("[%s, %d]: devm_kzalloc failed.\n",
			__func__, __LINE__);
		return -ENOMEM;
	}
	memset(pctx->ext_crtc_graphic_properties,
		0,
		sizeof(struct ext_crtc_graphic_prop_info));

	pctx->ext_common_crtc_properties = devm_kzalloc(
		dev,
		sizeof(struct ext_common_crtc_prop_info) * MTK_DRM_CRTC_TYPE_MAX,
		GFP_KERNEL);
	if (pctx->ext_common_crtc_properties == NULL) {
		DRM_ERROR("[%s, %d]: devm_kzalloc failed.\n",
			__func__, __LINE__);
		return -ENOMEM;
	}
	memset(pctx->ext_common_crtc_properties,
		0,
		sizeof(struct ext_common_crtc_prop_info));

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

	pctx->ext_common_connector_properties = devm_kzalloc(
		dev,
		sizeof(struct ext_common_connector_prop_info) * MTK_DRM_CONNECTOR_TYPE_MAX,
		GFP_KERNEL);
	if (pctx->ext_common_connector_properties == NULL) {
		DRM_ERROR("[%s, %d]: devm_kzalloc failed.\n",
			__func__, __LINE__);
		return -ENOMEM;
	}
	memset(pctx->ext_common_connector_properties,
		0,
		sizeof(struct ext_common_connector_prop_info));

	ret = mtk_tv_drm_ML_init(pctx_video);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_video_drm_ML_init  failed.\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_tv_drm_autogen_init(&pctx->autogen_priv);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_tv_drm_autogen_init failed.\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_tv_drm_pqu_metadata_init(&pctx->pqu_metadata_priv);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_tv_drm_pqu_metadata_init failed.\n", __func__, __LINE__);
		return ret;
	}

	ret = mtk_tv_drm_demura_init(&pctx->demura_priv);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_tv_drm_demura_init failed.\n", __func__, __LINE__);
		return ret;
	}

	set_panel_context(pctx);

	ret = mtk_video_display_init(dev, master, data, &primary_plane, NULL);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_video_display_init  failed.\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_gop_init(dev, master, data, NULL, &cursor_plane);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_gop_init failed.\n",
			__func__, __LINE__);
		return ret;
	}

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	ret = mtk_tvdac_init(dev);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_tvdac_init  failed.\n",
			__func__, __LINE__);
		return ret;
	}
#endif

	//for LDM
	ret = mtk_ldm_init(dev);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_ldm_init failed.\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_tv_drm_trigger_gen_init(pctx);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_tv_drm_trigger_gen_init failed.\n",
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

	ret = request_threaded_irq(
		pctx->video_irq_num,
		mtk_tv_kms_irq_top_handler,
		mtk_tv_kms_irq_bottom_handler,
		IRQF_SHARED | IRQF_ONESHOT,
		"DRM_MTK_VIDEO",
		pctx);

	if (ret) {
		DRM_ERROR("[%s, %d]: irq request failed.\n",
			__func__, __LINE__);
		return ret;
	}

	ret = devm_request_irq(
		dev,
		pctx->graphic_irq_vgsep_num,
		mtk_tv_graphic_path_irq_handler,
		IRQF_SHARED,
		"DRM_MTK_GRAPHIC",
		pctx);
	if (ret) {
		DRM_ERROR("[%s, %d]: irq request failed.\n",
			__func__, __LINE__);
		return ret;
	}

	for (i = 0; i < MTK_DRM_CRTC_TYPE_MAX; i++) {
		ret = mtk_tv_drm_crtc_create(
		drm_dev,
		&pctx->crtc[i],
		&primary_plane->base,
		&cursor_plane->base,
		&mtk_tv_kms_crtc_ops);
		if (i == MTK_DRM_CRTC_TYPE_VIDEO)
			pctx->crtc[i].mtk_crtc_type = MTK_DRM_CRTC_TYPE_VIDEO;
		else if (i == MTK_DRM_CRTC_TYPE_GRAPHIC)
			pctx->crtc[i].mtk_crtc_type = MTK_DRM_CRTC_TYPE_GRAPHIC;
		else if (i == MTK_DRM_CRTC_TYPE_EXT_VIDEO)
			pctx->crtc[i].mtk_crtc_type = MTK_DRM_CRTC_TYPE_EXT_VIDEO;
	}

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

	ret = mtk_tv_kms_create_ext_props(pctx, E_EXT_PROP_TYPE_CRTC_GRAPHIC);
	if (ret) {
		DRM_ERROR("[%s, %d]: kms create ext prop failed\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_tv_kms_create_ext_props(pctx, E_EXT_PROP_TYPE_CRTC_COMMON);
	if (ret) {
		DRM_ERROR("[%s, %d]: kms create ext prop failed\n",
			__func__, __LINE__);
		return ret;
	}

	for (i = 0; i < MTK_DRM_ENCODER_TYPE_MAX; i++) {
		if (mtk_tv_drm_encoder_create(
			drm_dev,
			&pctx->encoder[i],
			pctx->crtc) != 0) {

			DRM_ERROR("[%s, %d]: mtk_tv_drm_encoder_create fail.\n",
				__func__, __LINE__);
			goto ERR;
		}
	}


	for (i = 0; i < MTK_DRM_CONNECTOR_TYPE_MAX; i++) {
		if (i == MTK_DRM_CONNECTOR_TYPE_VIDEO) {
			pctx->connector[i].mtk_connector_type = MTK_DRM_CONNECTOR_TYPE_VIDEO;
			if ( mtk_tv_drm_connector_create(
				pctx->drm_dev,
				&pctx->connector[i],
				&pctx->encoder[i],
				&mtk_tv_kms_connector_ops) != 0) {
				DRM_ERROR("[%s, %d]: mtk_tv_drm_connector_create fail.\n",
					__func__, __LINE__);
				goto ERR;
			}
		} else if (i == MTK_DRM_CONNECTOR_TYPE_GRAPHIC) {
			pctx->connector[i].mtk_connector_type = MTK_DRM_CONNECTOR_TYPE_GRAPHIC;
			if ( mtk_tv_drm_connector_create(
				pctx->drm_dev,
				&pctx->connector[i],
				&pctx->encoder[i],
				&mtk_tv_kms_connector_ops) != 0) {
				DRM_ERROR("[%s, %d]: mtk_tv_drm_connector_create fail.\n",
					__func__, __LINE__);
				goto ERR;
			}
		} else if (i == MTK_DRM_CONNECTOR_TYPE_EXT_VIDEO) {
			pctx->connector[i].mtk_connector_type = MTK_DRM_CONNECTOR_TYPE_EXT_VIDEO;
			if ( mtk_tv_drm_connector_create(
				pctx->drm_dev,
				&pctx->connector[i],
				&pctx->encoder[i],
				&mtk_tv_kms_connector_ops) != 0) {
				DRM_ERROR("[%s, %d]: mtk_tv_drm_connector_create fail.\n",
					__func__, __LINE__);
				goto ERR;
			}
		}
	}
	//if ( mtk_tv_drm_connector_create(
		//pctx->drm_dev,
		//&pctx->connector,
		//&pctx->encoder,
		//&mtk_tv_kms_connector_ops) != 0) {
		//DRM_ERROR("[%s, %d]: mtk_tv_drm_connector_create fail.\n",
			//__func__, __LINE__);
		//goto ERR;
	//}

	// create extend connector properties
	ret = mtk_tv_kms_create_ext_props(pctx, E_EXT_PROP_TYPE_CONNECTOR);
	if (ret) {
		DRM_ERROR("[%s, %d]: kms create ext prop failed\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mtk_tv_kms_create_ext_props(pctx, E_EXT_PROP_TYPE_CONNECTOR_COMMON);
	if (ret) {
		DRM_ERROR("[%s, %d]: kms create ext prop failed\n",
			__func__, __LINE__);
		return ret;
	}

	//for pixelshift
	ret = mtk_video_pixelshift_init(dev);
	if (ret) {
		DRM_ERROR("[%s, %d]: mtk_video_pixelshift_init failed.\n",
			__func__, __LINE__);
		return ret;
	}

	_mtk_drm_get_capability(pctx);
	mtk_tv_drm_send_sharememtopqu();

#if defined(__KERNEL__) /* Linux Kernel */
#if (defined ANDROID)
	/*create Realtime Framework Domain*/
	// Please use E_MI_REALTIME_NORMAL, do not modify.
	u32Ret = MI_REALTIME_Init(E_MI_REALTIME_NORMAL);
	if (u32Ret != E_MI_REALTIME_RET_OK) {
		DRM_ERROR("%s: %d MI_REALTIME_Init failed. ret=%u\n",
			__func__, __LINE__, u32Ret);
	}

	// create domain
	u32Ret = MI_REALTIME_CreateDomain((MI_S8 *)"Domain_Render_DD");
	if (u32Ret != MI_OK) {
		DRM_ERROR("%s: %d MI_REALTIME_CreateDomain failed. ret=%u\n",
			__func__, __LINE__, u32Ret);
	}
#endif
#endif

	/*create commit_tail_thread*/
	crtc0_commit_tail_worker = kthread_create(mtk_commit_tail, pctx,
						"crtc0_thread");
	crtc0_param.sched_priority = MTK_DRM_COMMIT_TAIL_THREAD_PRIORITY;
	sched_setscheduler(crtc0_commit_tail_worker, SCHED_RR, &crtc0_param);

	cpumask_copy(&crtc0_task_cmask, cpu_online_mask);
	cpumask_clear_cpu(0, &crtc0_task_cmask);
	set_cpus_allowed_ptr(crtc0_commit_tail_worker, &crtc0_task_cmask);
	wake_up_process(crtc0_commit_tail_worker);

	//create commit_tail_thread for graphic crtc
	crtc1_commit_tail_worker = kthread_create(mtk_commit_tail_gCrtc, pctx,
						"crtc1_thread");
	crtc1_param.sched_priority = MTK_DRM_COMMIT_TAIL_THREAD_PRIORITY;
	sched_setscheduler(crtc1_commit_tail_worker, SCHED_RR, &crtc1_param);
	cpumask_copy(&crtc1_task_cmask, cpu_online_mask);
	cpumask_clear_cpu(0, &crtc1_task_cmask);
	set_cpus_allowed_ptr(crtc1_commit_tail_worker, &crtc1_task_cmask);
	wake_up_process(crtc1_commit_tail_worker);

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
	struct mtk_tv_kms_context *pctx = NULL;

	pctx = dev_get_drvdata(dev);

	mtk_video_display_unbind(dev);

	// remove global pq string buffer
	kfree(pctx->global_pq_priv.pq_string);

}

static const struct component_ops mtk_tv_kms_component_ops = {
	.bind	= mtk_tv_kms_bind,
	.unbind = mtk_tv_kms_unbind,
};

static const struct of_device_id mtk_drm_tv_kms_of_ids[] = {
	{ .compatible = "MTK-drm-tv-kms", },
	{},
};

static int mtk_drm_tv_kms_suspend(
	struct platform_device *dev,
	pm_message_t state)
{
	if (!dev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	int ret = 0;
	struct device *pdev = &dev->dev;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(pdev);
	bool IRQmask = 0;

	ret = mtk_common_IRQ_get_mask(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO],
		MTK_VIDEO_IRQTYPE_HK,
		MTK_VIDEO_SW_IRQ_TRIG_DISP,
		&IRQmask,
		pctx->u32IRQ_Version);

	pctx->irq_mask = IRQmask;
	ret |= mtk_drm_gop_suspend(dev, state);
	ret |= mtk_common_irq_suspend();
	ret |= mtk_tv_video_display_suspend(dev, state);
#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	ret |= mtk_tvdac_cvbso_suspend(dev);
#endif

	if (pctx->drmcap_dev.u8OLEDPixelshiftSupport)
		ret |= mtk_video_pixelshift_suspend(dev);

	return ret;
}

static int mtk_drm_tv_kms_resume(struct platform_device *dev)
{
	int ret = 0;
	struct device *pdev = &dev->dev;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(pdev);

	ret = mtk_drm_gop_resume(dev);
	ret |= mtk_common_irq_resume();
	ret |= mtk_tv_video_display_resume(dev);
	ret |= mtk_tv_drm_trigger_gen_init(pctx);
#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	ret |= mtk_tvdac_cvbso_resume(dev);
#endif

	if (pctx->drmcap_dev.u8OLEDPixelshiftSupport)
		ret |= mtk_video_pixelshift_resume(dev);

	if (pctx->irq_mask == 1) {
		ret |= mtk_common_IRQ_set_mask(
			NULL, MTK_VIDEO_IRQTYPE_HK, MTK_VIDEO_SW_IRQ_TRIG_DISP,
			pctx->u32IRQ_Version);
	} else {
		ret |= mtk_common_IRQ_set_unmask(
			NULL, MTK_VIDEO_IRQTYPE_HK, MTK_VIDEO_SW_IRQ_TRIG_DISP,
			pctx->u32IRQ_Version);
	}
	return ret;
}

static int mtk_tv_drm_kms_create_sysfs(struct platform_device *pdev)
{
	int ret = 0;

	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_tv_drm_kms_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "[%d]Fail to create sysfs files: %d\n", __LINE__, ret);
		return ret;
	}
	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_tv_drm_kms_attr_panel_group);
	if (ret) {
		dev_err(&pdev->dev, "[%d]Fail to create panel sysfs files: %d\n", __LINE__, ret);
		return ret;
	}

	return 0;
}

static int mtk_tv_drm_kms_remove_sysfs(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "Remove device attribute files");
	sysfs_remove_group(&pdev->dev.kobj, &mtk_tv_drm_kms_attr_group);
	sysfs_remove_group(&pdev->dev.kobj, &mtk_tv_drm_kms_attr_panel_group);
	return 0;
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
	mtk_tv_drm_kms_create_sysfs(pdev);
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
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	int i = 0;

	mtk_tv_drm_kms_remove_sysfs(pdev);
	if (pctx) {
		//destroy gop ml resource
		for (i = 0; i < pctx->gop_priv.GopNum; i++) {
			if (sm_ml_destroy_resource(pctx->gop_priv.gop_ml->fd) != E_SM_RET_OK) {
				DRM_INFO("[%s][%d]can not destroy gop_ml instance\n",
					__func__, __LINE__);
			}
		}
		clean_gop_context(dev, &pctx->gop_priv);

		//5.destroy resource
		if (sm_ml_destroy_resource(pctx_video->disp_ml.fd) != E_SM_RET_OK) {
			DRM_INFO("[%s][%d]can not destroy disp_ml instance!",
			__func__, __LINE__);
		}

		if (sm_ml_destroy_resource(pctx_video->vgs_ml.fd) != E_SM_RET_OK) {
			DRM_INFO("[%s][%d]can not destroy vgs_ml instance!",
			__func__, __LINE__);
		}

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

