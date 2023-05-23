/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _UAPI_MTK_TV_DRM_H_
#define _UAPI_MTK_TV_DRM_H_

#include "drm.h"
#include <stdbool.h>

#define MTK_GEM_CREATE_FLAG_IMPORT 0x80000000
#define MTK_GEM_CREATE_FLAG_EXPORT 0x00000000
#define MTK_GEM_CREATE_FLAG_IN_CMA 0x00000001 // physical continuous
#define MTK_GEM_CREATE_FLAG_IN_IOMMU 0x00000002 // physical discrete
#define MTK_GEM_CREATE_FLAG_IN_LX 0x00000004
#define MTK_GEM_CREATE_FLAG_IN_MMAP 0x00000008

#define MTK_CTRL_BOOTLOGO_CMD_GETINFO 0x0
#define MTK_CTRL_BOOTLOGO_CMD_DISABLE  0x1

//pixelshift feature: justscan:Hrange=32,Vrange=16;overscan:Hrange=32,Vrange=10
#define PIXELSHIFT_H_MAX (32)
#define PIXELSHIFT_V_MAX (16)
#define MTK_MFC_PLANE_ID_MAX (0xFF)

#define CTRLBIT_MAX_NUM (32)
#define VBO_MAX_LANES (64)
#define PLANE_SUPPORT_FENCE_MAX_NUM (5)

#define MTK_TV_DRM_WINDOW_NUM_MAX       (16)
#define MTK_TV_DRM_PQU_METADATA_COUNT   (2)

#define MTK_PLANE_PROP_PLANE_TYPE   "Plane_type"
    #define MTK_PLANE_PROP_PLANE_TYPE_GRAPHIC   "Plane_type_GRAPHIC"
    #define MTK_PLANE_PROP_PLANE_TYPE_VIDEO "Plane_type_VIDEO"
    #define MTK_PLANE_PROP_PLANE_TYPE_PRIMARY "Plane_type_PRIMARY"
	#define MTK_PLANE_PROP_PLANE_TYPE_EXT_VIDEO "Plane_type_EXT_VIDEO"
#define MTK_PLANE_PROP_HSTRETCH    "H-Stretch"
    #define MTK_PLANE_PROP_HSTRETCH_DUPLICATE    "HSTRETCH-DUPLICATE"
    #define MTK_PLANE_PROP_HSTRETCH_6TAP8PHASE     "6TAP8PHASE"
    #define MTK_PLANE_PROP_HSTRETCH_4TAP256PHASE   "H4TAP256PHASE"
#define MTK_PLANE_PROP_VSTRETCH    "V-Stretch"
    #define MTK_PLANE_PROP_VSTRETCH_DUPLICATE      "DUPLICATE"
    #define MTK_PLANE_PROP_VSTRETCH_2TAP16PHASE    "2TAP16PHASE"
    #define MTK_PLANE_PROP_VSTRETCH_BILINEAR       "BILINEAR"
    #define MTK_PLANE_PROP_VSTRETCH_4TAP256PHASE   "V4TAP256PHASE"
#define MTK_PLANE_PROP_DISABLE    "DISABLE"
#define MTK_PLANE_PROP_ENABLE     "ENABLE"
#define MTK_PLANE_PROP_SUPPORT    "SUPPORT"
#define MTK_PLANE_PROP_UNSUPPORT  "UNSUPPORT"
#define MTK_PLANE_PROP_HMIRROR    "H-Mirror"
#define MTK_PLANE_PROP_VMIRROR    "V-Mirror"
#define MTK_PLANE_PROP_AFBC_FEATURE "AFBC-feature"
#define MTK_PLANE_PROP_AID_NS    "AID-NS"
#define MTK_PLANE_PROP_AID_SDC    "AID-SDC"
#define MTK_PLANE_PROP_AID_S     "AID-S"
#define MTK_PLANE_PROP_AID_CSP  "AID-CSP"

#define MTK_GOP_PLANE_PROP_BYPASS	"PlaneBypass"

/* property name: GFX_PQ_DATA
 * property object: plane
 * support plane type: gop
 * type: blob
 *
 * This property is used to set pq data of gop plane
 */
#define MTK_GOP_PLANE_PROP_PQ_DATA "GFX_PQ_DATA"
#define GOP_PQ_BUF_ST_IDX_VERSION (2)

/* property name: GFX_ALPHA_MODE
 * property object: plane
 * support plane type: gop
 * type: blob
 *
 * This property is used to set pq data of gop plane
 */
#define MTK_GOP_PLANE_PROP_ALPHA_MODE "GFX_ALPHA_MODE"

/* property name: GFX_AID_TYPE
 * property object: plane
 * support plane type: gop
 * type: enum
 *
 * This property is used to set aid type of gop plane
 */
#define MTK_GOP_PLANE_PROP_AID_TYPE "GFX_AID_TYPE"

/* property name: PNL_CURLUM
 * property object: plane
 * support plane type: gop
 * type: range
 * value: 0~INT_MAX
 *
 * This property is used to set panel current luminance of gop plane
 */
#define MTK_GOP_PLANE_PROP_PNL_CURLUM	"PNL_CURLUM"

/* property name: VIDEO_GOP_ZORDER
 * property object: crtc
 * type: enum
 * value: enum
 *
 * This property is used to set the zorder of video and gop
 */
#define MTK_VIDEO_CRTC_PROP_VG_ORDER	"VIDEO_OSDB_ZORDER"
#define MTK_VIDEO_CRTC_PROP_V_OSDB0_OSDB1    "VIDEO_OSDB0_OSDB1"
#define MTK_VIDEO_CRTC_PROP_OSDB0_V_OSDB1    "OSDB0_VIDEO_OSDB1"
#define MTK_VIDEO_CRTC_PROP_OSDB0_OSDB1_V   "OSDB0_OSDB1_VIDEO"
enum video_crtc_vg_order_type {
	VIDEO_OSDB0_OSDB1,
	OSDB0_VIDEO_OSDB1,
	OSDB0_OSDB1_VIDEO,
};

/* property name: MUTE_SCREEN
 * property object: plane
 * support plane type: video
 * type: bool
 * value: 0 or 1
 *
 * This property is used to set current plane mute or note
 */
#define MTK_VIDEO_PLANE_PROP_MUTE_SCREEN	"MUTE_SCREEN"

/* property name: MUTE_COLOR
 * property object: crtc
 * type: blob
 * value: struct
 *
 * This property is used to set the mute color
 */
#define MTK_VIDEO_CRTC_PROP_MUTE_COLOR		"MUTE_COLOR"
struct drm_mtk_mute_color {
	__u16 red;
	__u16 green;
	__u16 blue;
};

#define MTK_VIDEO_PLANE_PROP_SNOW_SCREEN      "SNOW_SCREEN"

/* property name: BACKGROUND_COLOR
 * property object: crtc
 * type: blob
 * value: struct
 *
 * This property is used to set the background color
 */
#define MTK_VIDEO_CRTC_PROP_BG_COLOR		"BACKGROUND_COLOR"
struct drm_mtk_bg_color {
	__u16 red;
	__u16 green;
	__u16 blue;
};

/* property name: VIDEO_PLANE_TYPE
 * property object: plane
 * support plane type: video
 * type: ENUM
 * value: reference enum drm_mtk_video_plane_type
 *
 * This property is used to distinguish plane in which buffer
 */
#define PROP_NAME_VIDEO_PLANE_TYPE    "VIDEO_PLANE_TYPE"

/* property name: META_FD
 * property object: plane
 * support plane type: video
 * type: range
 * value: 0~INT_MAX
 *
 * This property is used to pass metadata fd.
 */
#define PROP_NAME_META_FD    "META_FD"

/* property name: BUF_MODE
 * property object: plane
 * support plane type: video
 * type: ENUM
 * value: reference enum drm_mtk_vplane_buf_mode
 *
 * This property is used to determine buffer mode.
 * Only take effect when prop "VIDEO_SRC0_PATH" set to MEMC.
 *
 */
#define PROP_VIDEO_PLANE_PROP_BUF_MODE    "BUF_MODE"

/* property name: DISP_MODE
 * property object: plane
 * support plane type: video
 * type: ENUM
 * value: reference enum vplane_disp_mode_enum_list
 *
 * This property is used to determine disp mode.
 *
 */
#define PROP_VIDEO_PLANE_PROP_DISP_MODE    "DISP_MODE"

/* property name: FREEZE
 * property object: plane
 * support plane type: video
 * type: range
 * value: 0~1,  0: disable freeze,  1: enable freeze
 *
 * This property is used to set buffer freeze.
 * Should only be set in legacy mode.
 */
#define PROP_NAME_FREEZE    "FREEZE"
#define MTK_VIDEO_PLANE_PROP_INPUT_VFREQ "DISPLAY_INPUT_VFREQ"

/* property name: WINDOW_PQ
 * property object: plane
 * support crtc type: video
 * type: BLOB
 * value: string
 *
 * This property is used to set Window PQ.
 * All Window PQ parameters are expressed in a long string.
 */
#define PROP_VIDEO_CRTC_PROP_WINDOW_PQ    "WINDOW_PQ"

/* property name: GLOBAL_PQ
 * property object: crtc
 * support crtc type: video
 * type: BLOB
 * value: string
 *
 * This property is used to set Global PQ.
 * All Global PQ parameters are expressed in a long string.
 */
#define PROP_VIDEO_CRTC_PROP_GLOBAL_PQ    "GLOBAL_PQ"

/* property name: DEMURA_CONFIG
 * property object: crtc
 * support crtc type: video
 * type: BLOB
 * value: struct drm_mtk_demura_cuc_config
 *
 * This property is used to set crtc-video demura.
 */
#define MTK_CRTC_PROP_DEMURA_CONFIG	"DEMURA_CONFIG"
struct drm_mtk_demura_config {
	__u8  id;
	bool  disable;
	__u32 binary_size;
	__u8  binary_data[];
};

/* property name: PQMAP_NODE_ARRAY
 * property object: plane
 * support crtc type: video
 * type: BLOB
 * value: struct drm_mtk_tv_pqmap_node_array
 *
 * This property is used to set PQmap nodes.
 */
#define MTK_VIDEO_PLANE_PROP_PQMAP_NODES_ARRAY "PQMAP_NODE_ARRAY"

#define MTK_CRTC_PROP_LDM_STATUS          "LDM_STATUS"
#define MTK_CRTC_PROP_LDM_STATUS_INIT            "LDM_STATUS_INIT"
#define MTK_CRTC_PROP_LDM_STATUS_ENABLE          "LDM_STATUS_ENABLE"
#define MTK_CRTC_PROP_LDM_STATUS_DISABLE         "LDM_STATUS_DISABLE"
#define MTK_CRTC_PROP_LDM_STATUS_DEINIT          "LDM_STATUS_DEINIT"
#define MTK_CRTC_PROP_LDM_LUMA            "LDM_LUMA"
#define MTK_CRTC_PROP_LDM_ENABLE          "LDM_ENABLE"
#define MTK_CRTC_PROP_LDM_STRENGTH        "LDM_STRENGTH"
#define MTK_CRTC_PROP_LDM_DATA            "LDM_DATA"
#define MTK_CRTC_PROP_LDM_DEMO_PATTERN    "LDM_DEMO_PATTERN"
#define MTK_CRTC_PROP_LDM_SW_SET_CTRL		"LDM_SW_SET_CTRL"
#define MTK_CRTC_PROP_LDM_AUTO_LD		"AUTO_LD"
#define MTK_CRTC_PROP_LDM_XTendedRange	"XTendedRange"
#define MTK_CRTC_PROP_LDM_INIT	"LDM_INIT"
/****pixelshift crtc prop******/
#define MTK_CRTC_PROP_PIXELSHIFT_ENABLE          "PIXELSHIFT_ENABLE"
#define MTK_CRTC_PROP_PIXELSHIFT_OSD_ATTACH      "PIXELSHIFT_OSD_ATTACH"
#define MTK_CRTC_PROP_PIXELSHIFT_TYPE            "PIXELSHIFT_TYPE"
#define MTK_CRTC_PROP_PIXELSHIFT_PRE_JUSTSCAN    "PIXELSHIFT_PRE_JUSTSCAN"
#define MTK_CRTC_PROP_PIXELSHIFT_PRE_OVERSCAN    "PIXELSHIFT_PRE_OVERSCAN"
#define MTK_CRTC_PROP_PIXELSHIFT_POST_JUSTSCAN   "PIXELSHIFT_POST_JUSTSCAN"
#define MTK_CRTC_PROP_PIXELSHIFT_H               "PIXELSHIFT_H"
#define MTK_CRTC_PROP_PIXELSHIFT_V               "PIXELSHIFT_V"

/****memc plane prop******/
#define MTK_PLANE_PROP_MEMC_LEVEL_MODE         "MEMC_LEVEL_MODE"
#define MTK_VIDEO_PROP_MEMC_LEVEL_OFF   "MEMC_LEVEL_OFF"
#define MTK_VIDEO_PROP_MEMC_LEVEL_LOW   "MEMC_LEVEL_LOW"
#define MTK_VIDEO_PROP_MEMC_LEVEL_MID   "MEMC_LEVEL_MID"
#define MTK_VIDEO_PROP_MEMC_LEVEL_HIGH   "MEMC_LEVEL_HIGH"
#define MTK_PLANE_PROP_MEMC_GAME_MODE     "MEMC_GAME_MODE"
#define MTK_VIDEO_PROP_MEMC_GAME_OFF   "MEMC_GAME_OFF"
#define MTK_VIDEO_PROP_MEMC_GAME_ON   "MEMC_GAME_ON"
#define MTK_PLANE_PROP_MEMC_MISC_TYPE    "MEMC_MISC_TYPE"
#define MTK_VIDEO_PROP_MEMC_MISC_NULL   "MEMC_MISC_NULL"
#define MTK_VIDEO_PROP_MEMC_MISC_INSIDE   "MEMC_MISC_INSIDE"
#define MTK_VIDEO_PROP_MEMC_MISC_INSIDE_60HZ   "MEMC_MISC_INSIDE_60HZ"
#define MTK_PLANE_PROP_MEMC_PATTERN	"MEMC_PATTERN"
#define MTK_VIDEO_PROP_MEMC_NULL_PAT   "MEMC_NULL_PAT"
#define MTK_VIDEO_PROP_MEMC_OPM_PAT   "MEMC_OPM_PAT"
#define MTK_VIDEO_PROP_MEMC_MV_PAT   "MEMC_MV_PAT"
#define MTK_VIDEO_PROP_MEMC_END_PAT   "MEMC_END_PAT"
#define MTK_PLANE_PROP_MEMC_DECODE_IDX_DIFF     "MEMC_DECODE_IDX_DIFF"
#define MTK_PLANE_PROP_MEMC_STATUS    "MEMC_STATUS"
#define MTK_PLANE_PROP_MEMC_TRIG_GEN   "MEMC_TRIG_GEN"
#define MTK_PLANE_PROP_MEMC_RV55_INFO    "MEMC_RV55_INFO"
#define MTK_PLANE_PROP_MEMC_INIT_VALUE_FOR_RV55    "MEMC_INIT_VALUE_FOR_RV55"


#define MTK_VIDEO_PROP_MEMC_MISC_A_NULL   "MEMC_MISC_A_NULL"
#define MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE   "MEMC_MISC_A_MEMCINSIDE"
#define MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_60HZ   "MEMC_MISC_A_60HZ"
#define MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_240HZ   "MEMC_MISC_A_240HZ"
#define MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_4K1K_120HZ   "MEMC_MISC_A_4K1K_120HZ"
#define MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_KEEP_OP_4K2K   "MEMC_MISC_A_KEEP_OP_4K2K"
#define MTK_VIDEO_PROP_MEMC_MISC_A_MEMCINSIDE_4K_HALFK_240HZ   "MEMC_MISC_A_4K_HALFK_240HZ"

enum ldm_crtc_status {
	MTK_CRTC_LDM_STATUS_INIT = 1,            // lDM status init
	MTK_CRTC_LDM_STATUS_ENABLE = 2,          // LDM status enable
	MTK_CRTC_LDM_STATUS_DISABLE,         // LDM status disable
	MTK_CRTC_LDM_STATUS_DEINIT,          // LDM status deinit
};

enum drm_mtk_plane_type {
	MTK_DRM_PLANE_TYPE_GRAPHIC = 0,
	MTK_DRM_PLANE_TYPE_VIDEO,
	MTK_DRM_PLANE_TYPE_PRIMARY,
	MTK_DRM_PLANE_TYPE_MAX,
};

enum drm_mtk_video_plane_type {
	MTK_VPLANE_TYPE_NONE = 0,
	MTK_VPLANE_TYPE_MEMC,
	MTK_VPLANE_TYPE_MULTI_WIN,
	MTK_VPLANE_TYPE_SINGLE_WIN1,
	MTK_VPLANE_TYPE_SINGLE_WIN2,
	MTK_VPLANE_TYPE_MAX,
};

enum drm_mtk_vplane_buf_mode {
	MTK_VPLANE_BUF_MODE_NONE = 0,
	MTK_VPLANE_BUF_MODE_SW,     // time sharing
	MTK_VPLANE_BUF_MODE_HW,     // legacy
	MTK_VPLANE_BUF_MODE_BYPASSS,
	MTK_VPLANE_BUF_MODE_MAX,
};

enum drm_mtk_vplane_disp_mode {
	MTK_VPLANE_DISP_MODE_NONE = 0,
	MTK_VPLANE_DISP_MODE_SW,
	MTK_VPLANE_DISP_MODE_HW,
	MTK_VPLANE_DISP_MODE_PRE_SW,
	MTK_VPLANE_DISP_MODE_PRE_HW,
	MTK_VPLANE_DISP_MODE_MAX,
};

enum drm_mtk_crtc_type {
	MTK_DRM_CRTC_TYPE_VIDEO = 0,
	MTK_DRM_CRTC_TYPE_GRAPHIC,
	MTK_DRM_CRTC_TYPE_EXT_VIDEO,
	MTK_DRM_CRTC_TYPE_MAX,
};

enum drm_mtk_encoder_type {
	MTK_DRM_ENCODER_TYPE_VIDEO = 0,
	MTK_DRM_ENCODER_TYPE_GRAPHIC,
	MTK_DRM_ENCODER_TYPE_EXT_VIDEO,
	MTK_DRM_ENCODER_TYPE_MAX,};

enum drm_mtk_connector_type {
	MTK_DRM_CONNECTOR_TYPE_VIDEO = 0,
	MTK_DRM_CONNECTOR_TYPE_GRAPHIC,
	MTK_DRM_CONNECTOR_TYPE_EXT_VIDEO,
	MTK_DRM_CONNECTOR_TYPE_MAX,
};

/// Define the panel vby1 metadata feature
enum drm_en_vbo_ctrlbit_feature {
	E_VBO_CTRLBIT_NO_FEATURE,
	E_VBO_CTRLBIT_GLOBAL_FRAMEID,
	E_VBO_CTRLBIT_HTOTAL,
	E_VBO_CTRLBIT_VTOTAL,
	E_VBO_CTRLBIT_HSTART_POS,
	E_VBO_CTRLBIT_VSTART_POS,
	E_VBO_CTRLBIT_CBVF,
	E_VBO_CTRLBIT_NUM,
};

/// Define the panel vby1 tx mute feature
typedef struct  {
	bool	bEnable;
	__u8 u8ConnectorID;
	// 0: Video; 1: Delta Video; 2: GFX
} drm_st_tx_mute_info;

typedef struct  {
	bool  bEnable;
	__u32 u32Red;
	__u32 u32Green;
	__u32 u32Blue;
	__u8 u8ConnectorID;
	// 0: Video; 1: Delta Video; 2: GFX
} drm_st_pixel_mute_info;

typedef struct  {
	bool	bEnable;
	__u8 u8ConnectorID;
	// 0: Video; 1: GFX ; 2: Delta Video
} drm_st_backlight_mute_info;

/*****panel related properties - CRTC*****/
#define MTK_CRTC_PROP_FRAMESYNC_PLANE_ID "DISPLAY_FRAMESYNC_PLANE_ID"
#define MTK_CRTC_PROP_FRAMESYNC_FRC_RATIO "DISPLAY_FRAMESYNC_FRC_RATIO"
#define MTK_CRTC_PROP_FRAMESYNC_MODE "DISPLAY_FRAMESYNC_MODE"
#define MTK_CRTC_PROP_LOW_LATENCY_MODE "DISPLAY_LOWLATENCY_MODE"
#define MTK_CRTC_PROP_FRC_TABLE "DISPLAY_FRC_TABLE"
#define MTK_CRTC_PROP_DISPLAY_TIMING_INFO "DISPLAY_TIMING_INFO"
#define MTK_CRTC_PROP_FREERUN_TIMING    "SET_FREERUN_TIMING"
#define MTK_CRTC_PROP_FORCE_FREERUN    "FORCE_FREERUN"
#define MTK_CRTC_PROP_FREERUN_VFREQ    "FREERUN_VFREQ"
#define MTK_CRTC_PROP_VIDEO_LATENCY    "VIDEO_LATENCY"
#define MTK_CRTC_PROP_PATTERN_GENERATE	"PATTERN_GENERATE"


/*****graphic related properties - CRTC*****/
#define MTK_CRTC_PROP_VG_SEPARATE "GRAPHIC_MODE"

/*****common related properties - CRTC*****/
#define MTK_CRTC_COMMON_PROP_CRTC_TYPE "CRTC_TYPE"
	#define MTK_CRTC_COMMON_MAIN_PATH "MAIN_PATH"
	#define MTK_CRTC_COMMON_GRAPHIC_PATH "GRAPHIC_PATH"
	#define MTK_CRTC_COMMON_EXT_VIDEO_PATH "EXT_VIDEO_PATH"

/*****panel related properties - CONNECTOR*****/
#define MTK_CONNECTOR_PROP_PNL_DBG_LEVEL "PNL_DBG_LEVEL"
#define MTK_CONNECTOR_PROP_PNL_OUTPUT "PNL_OUTPUT"
#define MTK_CONNECTOR_PROP_PNL_SWING_LEVEL "PNL_SWING_LEVEL"
#define MTK_CONNECTOR_PROP_PNL_FORCE_PANEL_DCLK "PNL_FORCE_PANEL_DCLK"
#define MTK_CONNECTOR_PROP_PNL_FORCE_PANEL_HSTART "PNL_FORCE_PANEL_HSTART"
#define MTK_CONNECTOR_PROP_PNL_OUTPUT_PATTERN "PNL_OUTPUT_PATTERN"
#define MTK_CONNECTOR_PROP_PNL_MIRROR_STATUS "PNL_MIRROR_STATUS"
#define MTK_CONNECTOR_PROP_PNL_SSC_EN "PNL_SSC_EN"
#define MTK_CONNECTOR_PROP_PNL_SSC_FMODULATION "PNL_SSC_FMODULATION"
#define MTK_CONNECTOR_PROP_PNL_SSC_RDEVIATION "PNL_SSC_RDEVIATION"
#define MTK_CONNECTOR_PROP_PNL_OVERDRIVER_ENABLE "PNL_OVERDRIVER_ENABLE"
#define MTK_CONNECTOR_PROP_PNL_PANEL_SETTING "PNL_PANEL_SETTING"
#define MTK_CONNECTOR_PROP_PNL_INFO "PNL_INFO"
#define MTK_CONNECTOR_PROP_PNL_PARSE_OUT_INFO_FROM_DTS "PARSE_OUT_INFO_FROM_DTS"
#define MTK_CONNECTOR_PROP_PNL_VBO_CTRLBITS "PNL_VBO_CTRLBITS"
#define MTK_CONNECTOR_PROP_PNL_TX_MUTE_EN "PNL_TX_MUTE_EN"
#define MTK_CONNECTOR_PROP_PNL_SWING_VREG "PNL_SWING_VREG"
#define MTK_CONNECTOR_PROP_PNL_PRE_EMPHASIS "PNL_PRE_EMPHASIS"
#define MTK_CONNECTOR_PROP_PNL_SSC "PNL_SSC"
#define MTK_CONNECTOR_PROP_PNL_OUTPUT_MODEL "OUT_MODEL"
#define MTK_CONNECTOR_PROP_PNL_CHECK_DTS_OUTPUT_TIMING "PNL_CHECK_DTS_OUTPUT_TIMING"
#define MTK_CONNECTOR_PROP_PNL_CHECK_FRAMESYNC_MLOAD "PNL_CHECK_FRAMESYNC_MLOAD"


/*****common related properties - CONNECTOR*****/
#define MTK_CONNECTOR_COMMON_PROP_CONNECTOR_TYPE "CONNECTOR_TYPE"
#define MTK_CONNECTOR_COMMON_MAIN_PATH "MAIN_PATH"
#define MTK_CONNECTOR_COMMON_GRAPHIC_PATH "GRAPHIC_PATH"
#define MTK_CONNECTOR_COMMON_EXT_VIDEO_PATH "EXT_VIDEO_PATH"
#define MTK_CONNECTOR_COMMON_PROP_PNL_TX_MUTE_EN "COMMON_PNL_VBO_TX_MUTE_EN"
#define MTK_CONNECTOR_COMMON_PROP_PNL_PIXEL_MUTE_EN "COMMON_PNL_PIXEL_MUTE_EN"
#define MTK_CONNECTOR_COMMON_PROP_BACKLIGHT_MUTE_EN "COMMON_PNL_BACKLIGHT_MUTE_EN"
#define MTK_CONNECTOR_COMMON_PROP_HMIRROR_EN "COMMON_PNL_HMIRROR_EN"
#define MTK_CONNECTOR_COMMON_PROP_PNL_VBO_CTRLBITS "COMMON_PNL_VBO_CTRLBITS"

enum drm_mtk_hmirror {
	MTK_DRM_HMIRROR_DISABLE = 0,
	MTK_DRM_HMIRROR_ENABLE,
};

enum drm_mtk_vmirror {
	MTK_DRM_VMIRROR_DISABLE = 0,
	MTK_DRM_VMIRROR_ENABLE,
};

enum drm_mtk_hstretch {
	MTK_DRM_HSTRETCH_DUPLICATE = 0,
	MTK_DRM_HSTRETCH_6TAP8PHASE,
	MTK_DRM_HSTRETCH_4TAP256PHASE,
};

enum drm_mtk_vstretch {
	MTK_DRM_VSTRETCH_2TAP16PHASE = 0,
	MTK_DRM_VSTRETCH_DUPLICATE,
	MTK_DRM_VSTRETCH_BILINEAR,
	MTK_DRM_VSTRETCH_4TAP,
};

enum drm_mtk_plane_aid_type {
	MTK_DRM_AID_TYPE_NS = 0,
	MTK_DRM_AID_TYPE_SDC,
	MTK_DRM_AID_TYPE_S,
	MTK_DRM_AID_TYPE_CSP,
};

enum drm_mtk_plane_bypass {
	MTK_DRM_PLANE_BYPASS_DISABLE = 0,
	MTK_DRM_PLANE_BYPASS_ENABLE,
};

enum drm_mtk_support_feature {
	MTK_DRM_SUPPORT_FEATURE_YES = 0,
	MTK_DRM_SUPPORT_FEATURE_NO,
};

enum video_crtc_pixelshift_type {
	VIDEO_CRTC_PIXELSHIFT_PRE_JUSTSCAN,          // pixelshift pre-demura justscan
	VIDEO_CRTC_PIXELSHIFT_PRE_OVERSCAN,          // pixelshift pre-demura overscan
	VIDEO_CRTC_PIXELSHIFT_POST_JUSTSCAN,          // pixelshift pose-demura justscan
};

enum video_crtc_memc_a_type {
	VIDEO_CRTC_MISC_A_NULL,
	VIDEO_CRTC_MISC_A_MEMC_INSIDE,
	VIDEO_CRTC_MISC_A_MEMC_INSIDE_60HZ,
	VIDEO_CRTC_MISC_A_MEMC_INSIDE_240HZ,
	VIDEO_CRTC_MISC_A_MEMC_INSIDE_4K1K_120HZ,
	VIDEO_CRTC_MISC_A_MEMC_INSIDE_KEEP_OP_4K2K,
	VIDEO_CRTC_MISC_A_MEMC_INSIDE_4K_HALFK_240HZ,
};

enum video_crtc_memc_caps {
	VIDEO_CRTC_MEMC_CAPS_NOT_SUPPORTED,      // chip caps
	VIDEO_CRTC_MEMC_CAPS_SUPPORTED,          // chip caps
};

enum video_plane_memc_pattern {
	VIDEO_PLANE_MEMC_NULL_PAT,
	VIDEO_PLANE_MEMC_OPM_PAT,
	VIDEO_PLANE_MEMC_MV_PAT,
	VIDEO_PLANE_MEMC_END_PAT,
};

enum video_plane_memc_level {
	VIDEO_PLANE_MEMC_LEVEL_OFF,
	VIDEO_PLANE_MEMC_LEVEL_LOW,
	VIDEO_PLANE_MEMC_LEVEL_MID,
	VIDEO_PLANE_MEMC_LEVEL_HIGH,
};

enum video_plane_memc_game_mode {
	VIDEO_PLANE_MEMC_GAME_OFF,
	VIDEO_PLANE_MEMC_GAME_ON,
};

enum video_plane_memc_misc_type {
	VIDEO_PLANE_MEMC_MISC_NULL,
	VIDEO_PLANE_MEMC_MISC_INSIDE,
	VIDEO_PLANE_MEMC_MISC_INSIDE_60HZ,
};

enum video_crtc_frame_sync_mode {
	VIDEO_CRTC_FRAME_SYNC_FREERUN = 0,
	VIDEO_CRTC_FRAME_SYNC_VTTV,
	VIDEO_CRTC_FRAME_SYNC_FRAMELOCK,
	VIDEO_CRTC_FRAME_SYNC_VTTPLL,
	VIDEO_CRTC_FRAME_SYNC_MAX,
};

enum crtc_type {
	CRTC_MAIN_PATH,
	CRTC_GRAPHIC_PATH,
	CRTC_EXT_VIDEO_PATH,
};

enum connector_type {
	CONNECTOR_MAIN_PATH,
	CONNECTOR_GRAPHIC_PATH,
	CONNECTOR_EXT_VIDEO_PATH,
};

enum connector_out_type {
	OUT_MODEL_VG_BLENDED,
	OUT_MODEL_VG_SEPARATED,
	OUT_MODEL_VG_SEPARATED_W_EXTDEV,
	OUT_MODEL_ONLY_EXTDEV,
	OUT_MODEL_MAX,
};

enum drm_en_pnl_output_format {
	E_DRM_OUTPUT_RGB,
	E_DRM_OUTPUT_YUV444,
	E_DRM_OUTPUT_YUV422,
	E_DRM_OUTPUT_YUV420,
	E_DRM_OUTPUT_ARGB8101010,
	E_DRM_OUTPUT_ARGB8888_W_DITHER,
	E_DRM_OUTPUT_ARGB8888_W_ROUND,
	E_DRM_OUTPUT_ARGB8888_MODE0,
	E_DRM_OUTPUT_ARGB8888_MODE1,
	E_DRM_PNL_OUTPUT_FORMAT_NUM,
};

struct drm_mtk_tv_dac {
	bool enable;
};

struct drm_mtk_tv_common_stub {
	bool stub;
	bool pnlgamma_stubmode;
	bool b2r_src_stubmode;
};

struct drm_mtk_tv_pnlgamma_enable {
	bool enable;
};

struct drm_mtk_tv_pnlgamma_gainoffset {
	__u16 pnlgamma_offset_r;
	__u16 pnlgamma_offset_g;
	__u16 pnlgamma_offset_b;
	__u16 pnlgamma_gain_r;
	__u16 pnlgamma_gain_g;
	__u16 pnlgamma_gain_b;
};

struct drm_mtk_tv_pnlgamma_gainoffset_enable {
	bool enable;
};

struct drm_mtk_tv_pqgamma_curve {
	__u16 curve_r[256];
	__u16 curve_g[256];
	__u16 curve_b[256];
	__u16 size;
};

struct drm_mtk_tv_pqgamma_enable {
	bool enable;
};

struct drm_mtk_tv_pqgamma_gainoffset {
	__u16 pqgamma_offset_r;
	__u16 pqgamma_offset_g;
	__u16 pqgamma_offset_b;
	__u16 pqgamma_gain_r;
	__u16 pqgamma_gain_g;
	__u16 pqgamma_gain_b;
	bool  pregainoffset;
};

struct drm_mtk_tv_pqgamma_gainoffset_enable {
	bool enable;
	bool pregainoffset;
};

struct drm_mtk_tv_pqgamma_maxvalue {
	__u16 pqgamma_max_r;
	__u16 pqgamma_max_g;
	__u16 pqgamma_max_b;
};

struct drm_mtk_tv_pqgamma_maxvalue_enable {
	bool enable;
};

struct drm_mtk_tv_pqmap_info {
	__u32 pqmap_ion_fd;
	__u32 pim_size;
	__u32 rm_size;
};

struct drm_mtk_tv_pqmap_node_array {
	__u32 node_num;
	__u16 node_array[];
};

struct drm_mtk_tv_gem_create {
	__u32 u32version;
	__u32 u32length;
	struct drm_mode_create_dumb drm_dumb;
	__u64 u64gem_dma_addr;
};

struct drm_mtk_tv_bootlogo_ctrl {
	__u32 u32version;
	__u32 u32length;
	__u8  u8CmdType;
	__u8  u8GopNum;
};

enum drm_mtk_tv_graphic_pattern_move_dir {
	MTK_DRM_GRAPHIC_PATTERN_MOVE_TO_RIGHT,
	MTK_DRM_GRAPHIC_PATTERN_MOVE_TO_LEFT,
};

struct drm_mtk_tv_graphic_testpattern {
	__u8  PatEnable;
	__u8  HwAutoMode;
	__u32 DisWidth;
	__u32 DisHeight;
	__u32 ColorbarW;
	__u32 ColorbarH;
	__u8  EnColorbarMove;
	enum drm_mtk_tv_graphic_pattern_move_dir MoveDir;
	__u16 MoveSpeed;
};

struct drm_mtk_tv_graphic_pq_setting {
	__u32  u32version;
	__u32  u32buf_cfd_ml_size;
	__u64  u64buf_cfd_ml_addr;
	__u32  u32buf_cfd_adl_size;
	__u64  u64buf_cfd_adl_addr;
	__u32  u32buf_pq_ml_size;
	__u64  u64buf_pq_ml_addr;
	__u32  u32GopIdx;
};

struct drm_mtk_tv_graphic_alpha_mode {
	__u32  u32version;
	bool bEnPixelAlpha;
	__u8  u8ConstantAlphaVal;
};

struct drm_mtk_tv_graphic_path_mode {
	__u8  VGsync_mode;
	__u32  GVreqDivRatio;
};

struct property_blob_memc_status {
	__u8 u8MemcEnabled;
	__u8 u8MemcLevelStatus;
};

struct drm_mtk_tv_gfx_pqudata {
	__u32 RetValue;
};

struct drm_st_ctrlbit_item {

	bool	bEnable;
	//is this ctrlbit active default 0
	__u8 u8LaneID;
	// 1~16 (to specify the lane), 0: copy to all lane(for 8KBE)
	__u8 u8Lsb;
	// the least significant bit of ctrlbit, support 0~23
	__u8 u8Msb;
	// the most significant bit of ctrlbit, support 0~23
	__u32 u32Value;
	// the value if specific ctrlbit, if feature was specified, this value will be ignored.
	enum drm_en_vbo_ctrlbit_feature enFeature;
	// feature_name
};


struct drm_st_ctrlbits {
	__u8 u8ConnectorID;
	// 0: Video; 1: Delta Video; 2: GFX
	__u8 u8CopyType;
	// 0: normal(for Delta Video, GFX) ; 1: copy all lane(for Video)
	struct drm_st_ctrlbit_item ctrlbit_item[CTRLBIT_MAX_NUM];
	// ctrl items
};

struct drm_st_pnl_frc_ratio_info {
	__u8 u8FRC_in;      //ivs
	__u8 u8FRC_out;     //ovs
};

enum drm_mtk_pattern_position {
	MTK_PATTERN_POSITION_OPDRAM,
	MTK_PATTERN_POSITION_IPBLEND,
	MTK_PATTERN_POSITION_MULTIWIN,
	MTK_PATTERN_POSITION_TCON,
	MTK_PATTERN_POSITION_GFX,
	MTK_PATTERN_POSITION_MAX,
};

enum drm_mtk_pattern_type {
	MTK_PATTERN_PURE_COLOR,
	MTK_PATTERN_RAMP,
	MTK_PATTERN_PIP_WINDOW,
	MTK_PATTERN_CROSSHATCH,
	MTK_PATTERN_RADOM,
	MTK_PATTERN_CHESSBOARD,
	MTK_PATTERN_MAX,
};

/* Panel pattern gen type */
enum drm_mtk_panel_pattern_type {
	MTK_PNL_PATTERN_PURE_COLOR,
	MTK_PNL_PATTERN_RAMP,
	MTK_PNL_PATTERN_PIP_WINDOW,
	MTK_PNL_PATTERN_CROSSHATCH,
	MTK_PNL_PATTERN_MAX,
};

struct drm_mtk_pattern_color {
	__u16 red;
	__u16 green;
	__u16 blue;
};

struct drm_mtk_pattern_pure_color {
	struct drm_mtk_pattern_color color;
};

struct drm_mtk_pattern_ramp {
	struct drm_mtk_pattern_color color_begin;
	struct drm_mtk_pattern_color color_end;
	__u8  vertical;
	__u16 level;
};

struct drm_mtk_pattern_pip_window {
	struct drm_mtk_pattern_color background_color;
	struct drm_mtk_pattern_color window_color;
	__u16 window_x;
	__u16 window_y;
	__u16 window_width;
	__u16 window_height;
};

struct drm_mtk_pattern_crosshatch {
	struct drm_mtk_pattern_color background_color;
	struct drm_mtk_pattern_color line_color;
	__u16 line_start_x;
	__u16 line_start_y;
	__u16 line_interval_h;
	__u16 line_interval_v;
	__u16 line_width_v;
	__u16 line_width_h;
};

struct drm_mtk_pattern_config {
	enum drm_mtk_panel_pattern_type pattern_type;
	enum drm_mtk_pattern_position pattern_position;
	union {
		struct drm_mtk_pattern_pure_color pure_color;
		struct drm_mtk_pattern_ramp ramp;
		struct drm_mtk_pattern_pip_window pip_window;
		struct drm_mtk_pattern_crosshatch crosshatch;
	} u;
};

enum drm_video_ldm_sw_set_ctrl_type {
	E_DRM_LDM_SW_LDC_BYPASS = 0,
	E_DRM_LDM_SW_LDC_PATH_SEL,
	E_DRM_LDM_SW_LDC_XIU2AHB_SEL0,
	E_DRM_LDM_SW_LDC_XIU2AHB_SEL1,
	E_DRM_LDM_SW_RESET_LDC_RST,
	E_DRM_LDM_SW_RESET_LDC_MRST,
	E_DRM_LDM_SW_RESET_LDC_AHB,
	E_DRM_LDM_SW_RESET_LDC_AHB_WP,
	E_DRM_LDM_SW_SET_LD_SUPPORT,
	E_DRM_LDM_SW_SET_CUS_PROFILE,
	E_DRM_LDM_SW_SET_TMON_DEBUG,

};

struct DrmSWSetCtrlIn {
	bool RIU;
	enum drm_video_ldm_sw_set_ctrl_type enDrmLDMSwSetCtrlType;
	__u8 u8Value;

};

struct drm_st_out_swing_level {
	bool usr_swing_level;
	bool common_swing;
	__u32 ctrl_lanes;
	__u32 swing_level[VBO_MAX_LANES];
};

struct drm_st_out_pe_level {
	bool usr_pe_level;
	bool common_pe;
	__u32 ctrl_lanes;
	__u32 pe_level[VBO_MAX_LANES];
};

struct drm_st_out_ssc {
	bool ssc_en;
	__u32 ssc_modulation;
	__u32 ssc_deviation;
};

struct drm_st_mod_outconfig {
	__u16 outconfig_ch0_ch7;
	__u16 outconfig_ch8_ch15;
	__u16 outconfig_ch16_ch19;
};

struct drm_st_mod_freeswap {
	__u16 freeswap_ch0_ch1;
	__u16 freeswap_ch2_ch3;
	__u16 freeswap_ch4_ch5;
	__u16 freeswap_ch6_ch7;
	__u16 freeswap_ch8_ch9;
	__u16 freeswap_ch10_ch11;
	__u16 freeswap_ch12_ch13;
	__u16 freeswap_ch14_ch15;
	__u16 freeswap_ch16_ch17;
	__u16 freeswap_ch18_ch19;
	__u16 freeswap_ch20_ch21;
	__u16 freeswap_ch22_ch23;
	__u16 freeswap_ch24_ch25;
	__u16 freeswap_ch26_ch27;
	__u16 freeswap_ch28_ch29;
	__u16 freeswap_ch30_ch31;
};

struct drm_st_mod_status {
	bool vby1_locked;
	__u16 vby1_unlockcnt;
	bool vby1_htpdn;
	bool vby1_lockn;
	struct drm_st_mod_outconfig outconfig;
	struct drm_st_mod_freeswap freeswap;
};

struct drm_mtk_tv_video_delay {
	__u16 sw_delay_ms;
	__u16 memc_delay_ms;
	__u16 memc_rw_diff;
	__u16 mdgw_delay_ms[16];
	__u16 mdgw_rw_diff[16];
};

struct drm_mtk_tv_output_info {
	__u8 p_engine;
	enum drm_en_pnl_output_format color_fmt;
};

struct drm_mtk_tv_dip_capture_info {
	struct drm_mtk_tv_output_info video;
	struct drm_mtk_tv_output_info video_osdb;
	struct drm_mtk_tv_output_info video_post;
};

struct drm_mtk_tv_fence_info {
	bool bCreateFence[PLANE_SUPPORT_FENCE_MAX_NUM];
	int FenceFd[PLANE_SUPPORT_FENCE_MAX_NUM];
};

struct drm_mtk_tv_pqu_window_setting {
	__u32 mute;
	__u32 pq_id;
	__u32 frame_id;
	__u32 window_x;
	__u32 window_y;
	__u32 window_z;
	__u32 window_width;
	__u32 window_height;
};

struct drm_mtk_tv_pqu_metadata {
	__u32 global_frame_id;
	__u32 window_num;
	struct drm_mtk_tv_pqu_window_setting window_setting[MTK_TV_DRM_WINDOW_NUM_MAX];
};

struct drm_mtk_tv_graphic_roi_info {
	__u32 Threshold;
	__u64 RetCount;
};

#define DRM_MTK_TV_GEM_CREATE   (0x00)
#define DRM_MTK_TV_CTRL_BOOTLOGO   (0x01)
#define DRM_MTK_TV_SET_GRAPHIC_TESTPATTERN   (0x02)
#define DRM_MTK_TV_SET_STUB_MODE (0x03)
#define DRM_MTK_TV_START_GFX_PQUDATA (0x04)
#define DRM_MTK_TV_STOP_GFX_PQUDATA (0x05)
#define DRM_MTK_TV_SET_PNLGAMMA_ENABLE (0x06)
#define DRM_MTK_TV_SET_PNLGAMMA_GAINOFFSET (0x07)
#define DRM_MTK_TV_SET_PNLGAMMA_GAINOFFSET_ENABLE (0x08)
// #define DRM_MTK_TV_... (0x09) // Remove
#define DRM_MTK_TV_SET_CVBSO_MODE (0x0A)
#define DRM_MTK_TV_SET_PQMAP_INFO (0x0B)
#define DRM_MTK_TV_GET_VIDEO_DELAY (0x0C)
#define DRM_MTK_TV_GET_OUTPUT_INFO (0x0D)
#define DRM_MTK_TV_GET_FENCE (0x0E)
#define DRM_MTK_TV_SET_PQGAMMA_CURVE (0x0F)
#define DRM_MTK_TV_ENABLE_PQGAMMA (0x10)
#define DRM_MTK_TV_SET_PQGAMMA_GAINOFFSET (0x11)
#define DRM_MTK_TV_ENABLE_PQGAMMA_GAINOFFSET (0x12)
#define DRM_MTK_TV_SET_PQGAMMA_MAXVALUE (0x13)
#define DRM_MTK_TV_ENABLE_PQGAMMA_MAXVLAUE (0x14)
#define DRM_MTK_TV_GET_PQU_METADATA (0x15)
#define DRM_MTK_TV_TIMELINE_INC (0x16)
#define DRM_MTK_TV_SET_GRAPHIC_PQ_BUF (0x17)
#define DRM_MTK_TV_GET_GRAPHIC_ROI (0x18)

#define DRM_IOCTL_MTK_TV_GEM_CREATE    DRM_IOWR(DRM_COMMAND_BASE + \
	DRM_MTK_TV_GEM_CREATE, struct drm_mtk_tv_gem_create)
#define DRM_IOCTL_MTK_TV_CTRL_BOOTLOGO    DRM_IOWR(DRM_COMMAND_BASE + \
	DRM_MTK_TV_CTRL_BOOTLOGO, struct drm_mtk_tv_bootlogo_ctrl)
#define DRM_IOCTL_MTK_TV_SET_GRAPHIC_TESTPATTERN    DRM_IOWR(DRM_COMMAND_BASE + \
	DRM_MTK_TV_SET_GRAPHIC_TESTPATTERN, struct drm_mtk_tv_graphic_testpattern)
#define DRM_IOCTL_MTK_TV_SET_STUB_MODE    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_SET_STUB_MODE, struct drm_mtk_tv_common_stub)
#define DRM_IOCTL_MTK_TV_START_GFX_PQUDATA    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_START_GFX_PQUDATA, struct drm_mtk_tv_gfx_pqudata)
#define DRM_IOCTL_MTK_TV_STOP_GFX_PQUDATA    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_STOP_GFX_PQUDATA, struct drm_mtk_tv_gfx_pqudata)
#define DRM_IOCTL_MTK_TV_SET_PNLGAMMA_ENABLE    DRM_IOWR(DRM_COMMAND_BASE + \
	DRM_MTK_TV_SET_PNLGAMMA_ENABLE, struct drm_mtk_tv_pnlgamma_enable)
#define DRM_IOCTL_MTK_TV_SET_PNLGAMMA_GAINOFFSET    DRM_IOWR(DRM_COMMAND_BASE + \
	DRM_MTK_TV_SET_PNLGAMMA_GAINOFFSET, struct drm_mtk_tv_pnlgamma_gainoffset)
#define DRM_IOCTL_MTK_TV_SET_PNLGAMMA_GAINOFFSET_ENABLE    DRM_IOWR(DRM_COMMAND_BASE + \
	DRM_MTK_TV_SET_PNLGAMMA_GAINOFFSET_ENABLE, struct drm_mtk_tv_pnlgamma_gainoffset_enable)
// #define DRM_IOCTL_MTK_TV_...  DRM_IOWR(DRM_COMMAND_BASE + DRM_MTK_TV_, ...) // Remove
#define DRM_IOCTL_MTK_TV_SET_CVBSO_MODE    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_SET_CVBSO_MODE, struct drm_mtk_tv_dac)
#define DRM_IOCTL_MTK_TV_SET_PQMAP_INFO    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_SET_PQMAP_INFO, struct drm_mtk_tv_pqmap_info)
#define DRM_IOCTL_MTK_TV_GET_VIDEO_DELAY    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_GET_VIDEO_DELAY, struct drm_mtk_tv_video_delay)
#define DRM_IOCTL_MTK_TV_GET_OUTPUT_INFO    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_GET_OUTPUT_INFO, struct drm_mtk_tv_dip_capture_info)
#define DRM_IOCTL_MTK_TV_GET_FENCE    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_GET_FENCE, struct drm_mtk_tv_fence_info)
#define DRM_IOCTL_MTK_TV_SET_PQGAMMA_CURVE    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_SET_PQGAMMA_CURVE, struct drm_mtk_tv_pqgamma_curve)
#define DRM_IOCTL_MTK_TV_ENABLE_PQGAMMA    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_ENABLE_PQGAMMA, struct drm_mtk_tv_pqgamma_enable)
#define DRM_IOCTL_MTK_TV_SET_PQGAMMA_GAINOFFSET    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_SET_PQGAMMA_GAINOFFSET, struct drm_mtk_tv_pqgamma_gainoffset)
#define DRM_IOCTL_MTK_TV_ENABLE_PQGAMMA_GAINOFFSET    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_ENABLE_PQGAMMA_GAINOFFSET, struct drm_mtk_tv_pqgamma_gainoffset_enable)
#define DRM_IOCTL_MTK_TV_SET_PQGAMMA_MAXVALUE    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_SET_PQGAMMA_MAXVALUE, struct drm_mtk_tv_pqgamma_maxvalue)
#define DRM_IOCTL_MTK_TV_ENABLE_PQGAMMA_MAXVALUE    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_ENABLE_PQGAMMA_MAXVLAUE, struct drm_mtk_tv_pqgamma_maxvalue_enable)
#define DRM_IOCTL_MTK_TV_GET_PQU_METADATA   DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_GET_PQU_METADATA, struct drm_mtk_tv_pqu_metadata)
#define DRM_IOCTL_MTK_TV_TIMELINE_INC    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_TIMELINE_INC, int)
#define DRM_IOCTL_MTK_TV_SET_GRAPHIC_PQ_BUF    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_SET_GRAPHIC_PQ_BUF, struct drm_mtk_tv_graphic_pq_setting)
#define DRM_IOCTL_MTK_TV_GET_GRAPHIC_ROI    DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_MTK_TV_GET_GRAPHIC_ROI, struct drm_mtk_tv_graphic_roi_info)

#define DRM_MTK_TV_FB_MODIFIER_NUM	4

#ifndef DRM_FORMAT_MOD_ARM_AFBC
#define DRM_FORMAT_MOD_VENDOR_ARM     0x08
/*
 * Arm Framebuffer Compression (AFBC) modifiers
 *
 * AFBC is a proprietary lossless image compression protocol and format.
 * It provides fine-grained random access and minimizes the amount of data
 * transferred between IP blocks.
 *
 * AFBC has several features which may be supported and/or used, which are
 * represented using bits in the modifier. Not all combinations are valid,
 * and different devices or use-cases may support different combinations.
 */
#define DRM_FORMAT_MOD_ARM_AFBC(__afbc_mode) fourcc_mod_code(ARM, __afbc_mode)

/*
 * AFBC superblock size
 *
 * Indicates the superblock size(s) used for the AFBC buffer. The buffer
 * size (in pixels) must be aligned to a multiple of the superblock size.
 * Four lowest significant bits(LSBs) are reserved for block size.
 */
#define AFBC_FORMAT_MOD_BLOCK_SIZE_MASK      0xf
#define AFBC_FORMAT_MOD_BLOCK_SIZE_16x16     (1ULL)
#define AFBC_FORMAT_MOD_BLOCK_SIZE_32x8      (2ULL)
#define AFBC_FORMAT_MOD_BLOCK_SIZE_64x4      (3ULL)
#define AFBC_FORMAT_MOD_BLOCK_SIZE_32x8_64x4 (4ULL)

/*
 * AFBC lossless colorspace transform
 *
 * Indicates that the buffer makes use of the AFBC lossless colorspace
 * transform.
 */
#define AFBC_FORMAT_MOD_YTR     (1ULL <<  4)

/*
 * AFBC block-split
 *
 * Indicates that the payload of each superblock is split. The second
 * half of the payload is positioned at a predefined offset from the start
 * of the superblock payload.
 */
#define AFBC_FORMAT_MOD_SPLIT   (1ULL <<  5)

/*
 * AFBC sparse layout
 *
 * This flag indicates that the payload of each superblock must be stored at a
 * predefined position relative to the other superblocks in the same AFBC
 * buffer. This order is the same order used by the header buffer. In this mode
 * each superblock is given the same amount of space as an uncompressed
 * superblock of the particular format would require, rounding up to the next
 * multiple of 128 bytes in size.
 */
#define AFBC_FORMAT_MOD_SPARSE  (1ULL <<  6)

/*
 * AFBC copy-block restrict
 *
 * Buffers with this flag must obey the copy-block restriction. The restriction
 * is such that there are no copy-blocks referring across the border of 8x8
 * blocks. For the subsampled data the 8x8 limitation is also subsampled.
 */
#define AFBC_FORMAT_MOD_CBR     (1ULL <<  7)

/*
 * AFBC tiled layout
 *
 * The tiled layout groups superblocks in 8x8 or 4x4 tiles, where all
 * superblocks inside a tile are stored together in memory. 8x8 tiles are used
 * for pixel formats up to and including 32 bpp while 4x4 tiles are used for
 * larger bpp formats. The order between the tiles is scan line.
 * When the tiled layout is used, the buffer size (in pixels) must be aligned
 * to the tile size.
 */
#define AFBC_FORMAT_MOD_TILED   (1ULL <<  8)

/*
 * AFBC solid color blocks
 *
 * Indicates that the buffer makes use of solid-color blocks, whereby bandwidth
 * can be reduced if a whole superblock is a single color.
 */
#define AFBC_FORMAT_MOD_SC      (1ULL <<  9)

#endif // DRM_FORMAT_MOD_ARM_AFBC

#ifndef DRM_FORMAT_MOD_VENDOR_QCOM
#define DRM_FORMAT_MOD_VENDOR_QCOM    0x05
#endif

#ifndef DRM_FORMAT_MOD_QCOM_COMPRESSED
#define DRM_FORMAT_MOD_QCOM_COMPRESSED	fourcc_mod_code(QCOM, 1)
#endif

/*
 * MTK defined modifier
 *
 * BIT 0 ~ 7	: Used to define extended format arrangement
 * BIT 8	: The format is compressed
 * BIT 9	: The format is modified to 6 bit per pixel
 * BIT 10	: The format is modified to 10 bit per pixel
 */
#define DRM_FORMAT_MOD_VENDOR_MTK	(0x09)

/* extended arrangment */
#define DRM_FORMAT_MOD_MTK_YUV444_VYU	fourcc_mod_code(MTK, 0x01)

/* extended modifier */
#define DRM_FORMAT_MOD_MTK_COMPRESSED	fourcc_mod_code(MTK, 0x100)
#define DRM_FORMAT_MOD_MTK_6BIT		fourcc_mod_code(MTK, 0x200)
#define DRM_FORMAT_MOD_MTK_10BIT	fourcc_mod_code(MTK, 0x400)
#define DRM_FORMAT_MOD_MTK_MOTION	fourcc_mod_code(MTK, 0x1000)

#endif
