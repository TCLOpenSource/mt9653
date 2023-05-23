/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/* * Copyright (c) 2020 MediaTek Inc. */

#ifndef _MTK_DRM_KMS_H_
#define _MTK_DRM_KMS_H_

#include <drm/drm_encoder.h>

#include "mtk_tv_drm_gop.h"
#include "mtk_tv_drm_video_panel.h"
#include "mtk_tv_drm_video.h"
#include "mtk_tv_drm_video_display.h"
#include "mtk_tv_drm_video_pixelshift.h"
#include "mtk_tv_drm_global_pq.h"
#include "mtk_tv_drm_encoder.h"
#include "mtk_tv_drm_ld.h"
#include "mtk_tv_drm_pattern.h"
#include "mtk_tv_drm_demura.h"
#include "mtk_tv_drm_tvdac.h"
#include "mtk_tv_drm_autogen.h"
#include "mtk_tv_drm_pqmap.h"
#include "mtk_tv_drm_pqu_metadata.h"

#include "hwreg_render_common_trigger_gen.h"
#include "mtk_tv_drm_common_irq.h"

#define MTK_DRM_TV_KMS_COMPATIBLE_STR "MTK-drm-tv-kms"
#define MTK_DRM_DTS_GOP_PLANE_NUM_TAG             "GRAPHIC_PLANE_NUM"
#define MTK_DRM_DTS_GOP_PLANE_INDEX_START_TAG     "GRAPHIC_PLANE_INDEX_START"
#define MTK_DRM_TRIGGEN_VSDLY_V         (0x1B)
#define MTK_DRM_TRIGGEN_DMARDDLY_V      (0x1F)
#define MTK_DRM_TRIGGEN_STRDLY_V        (0x1F)
#define MTK_DRM_COMMIT_TAIL_THREAD_PRIORITY        (40)
#define MTK_DRM_ML_DISP_BUF_COUNT (2)
#define MTK_DRM_ML_DISP_CMD_COUNT (500)
#define MTK_DRM_ML_DISP_IRQ_BUF_COUNT (2)
#define MTK_DRM_ML_DISP_IRQ_CMD_COUNT (500)

#define IS_FRAMESYNC(x)     ((x == VIDEO_CRTC_FRAME_SYNC_VTTV) || \
							(x == VIDEO_CRTC_FRAME_SYNC_FRAMELOCK))


#define IS_RANGE_PROP(prop) ((prop)->flags & DRM_MODE_PROP_RANGE)
#define IS_ENUM_PROP(prop) ((prop)->flags & DRM_MODE_PROP_ENUM)
#define IS_BLOB_PROP(prop) ((prop)->flags & DRM_MODE_PROP_BLOB)

extern struct platform_driver mtk_drm_tv_kms_driver;
extern bool bPquEnable; // is pqurv55 enable ? declare at mtk_tv_drm_drv.c.

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
	E_EXT_PROP_TYPE_CRTC_GRAPHIC,
	E_EXT_PROP_TYPE_CRTC_COMMON,
	E_EXT_PROP_TYPE_CONNECTOR_COMMON,
	E_EXT_PROP_TYPE_MAX,
};

enum {
	E_EXT_COMMON_PLANE_PROP_TYPE = 0,
	E_EXT_COMMON_PLANE_PROP_MAX,
};

// need to update kms.c get/set property function after adding new properties
enum ext_video_crtc_prop_type {
	// pnl property
	E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID,
	E_EXT_CRTC_PROP_FRAMESYNC_FRC_RATIO,
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
	E_EXT_CRTC_PROP_LDM_SW_REG_CTRL,
	E_EXT_CRTC_PROP_LDM_AUTO_LD,
	E_EXT_CRTC_PROP_LDM_XTendedRange,
	E_EXT_CRTC_PROP_LDM_INIT,
	// pixelshift property
	E_EXT_CRTC_PROP_PIXELSHIFT_ENABLE,
	E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACH,
	E_EXT_CRTC_PROP_PIXELSHIFT_TYPE,
	E_EXT_CRTC_PROP_PIXELSHIFT_H,
	E_EXT_CRTC_PROP_PIXELSHIFT_V,
	// demura property
	E_EXT_CRTC_PROP_DEMURA_CONFIG,
	// pattern property
	E_EXT_CRTC_PROP_PATTERN_GENERATE,
	// global pq property
	E_EXT_CRTC_PROP_GLOBAL_PQ,
	// mute color property
	E_EXT_CRTC_PROP_MUTE_COLOR,
	// background color property
	E_EXT_CRTC_PROP_BACKGROUND_COLOR,
	// graphic and video property
	E_EXT_CRTC_PROP_GOP_VIDEO_ORDER,
	// total property
	E_EXT_CRTC_PROP_MAX,
};

enum {
	E_EXT_COMMON_CRTC_PROP_TYPE = 0,
	E_EXT_COMMON_CRTC_PROP_MAX,
};

enum ext_graphic_crtc_prop_type {
	E_EXT_CRTC_GRAPHIC_PROP_MODE,
	E_EXT_CRTC_GRAPHIC_PROP_MAX,
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
	E_EXT_CONNECTOR_PROP_PARSE_OUT_INFO_FROM_DTS,
	E_EXT_CONNECTOR_PROP_PNL_VBO_CTRLBIT,
	E_EXT_CONNECTOR_PROP_PNL_VBO_TX_MUTE_EN,
	E_EXT_CONNECTOR_PROP_PNL_SWING_VREG,
	E_EXT_CONNECTOR_PROP_PNL_PRE_EMPHASIS,
	E_EXT_CONNECTOR_PROP_PNL_SSC,
	E_EXT_CONNECTOR_PROP_PNL_OUT_MODEL,
	E_EXT_CONNECTOR_PROP_PNL_CHECK_DTS_OUTPUT_TIMING,
	E_EXT_CONNECTOR_PROP_PNL_FRAMESYNC_MLOAD,
	E_EXT_CONNECTOR_PROP_MAX,
};

enum {
	E_EXT_CONNECTOR_COMMON_PROP_TYPE,
	E_EXT_CONNECTOR_COMMON_PROP_PNL_VBO_TX_MUTE_EN,
	E_EXT_CONNECTOR_COMMON_PROP_PNL_PIXEL_MUTE_EN,
	E_EXT_CONNECTOR_COMMON_PROP_BACKLIGHT_MUTE_EN,
	E_EXT_CONNECTOR_COMMON_PROP_HMIRROR_EN,
	E_EXT_CONNECTOR_COMMON_PROP_PNL_VBO_CTRLBIT,
	E_EXT_CONNECTOR_COMMON_PROP_MAX,
};

struct ext_common_plane_prop_info {
	uint64_t prop_val[E_EXT_COMMON_PLANE_PROP_MAX];
};

struct ext_crtc_prop_info {
	uint64_t prop_val[E_EXT_CRTC_PROP_MAX];
	uint8_t prop_update[E_EXT_CRTC_PROP_MAX];
};

struct ext_crtc_graphic_prop_info {
	uint64_t prop_val[E_EXT_CRTC_GRAPHIC_PROP_MAX];
	uint8_t prop_update[E_EXT_CRTC_GRAPHIC_PROP_MAX];
};

struct ext_common_crtc_prop_info {
	uint64_t prop_val[E_EXT_COMMON_CRTC_PROP_MAX];
	uint8_t prop_update[E_EXT_COMMON_CRTC_PROP_MAX];
};

struct ext_connector_prop_info {
	uint64_t prop_val[E_EXT_CONNECTOR_PROP_MAX];
};

struct ext_common_connector_prop_info {
	uint64_t prop_val[E_EXT_CONNECTOR_COMMON_PROP_MAX];
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
	u8  u8buf_iommu_offset;
	u8  u8iommu_idx_memc_pq;
	u8  u8OLEDPixelshiftSupport; /*the project support oled pixelshift*/
};

enum OUTPUT_MODEL {
	E_OUT_MODEL_VG_BLENDED,
	E_OUT_MODEL_VG_SEPARATED,
	E_OUT_MODEL_VG_SEPARATED_W_EXTDEV,
	E_OUT_MODEL_ONLY_EXTDEV,
	E_OUT_MODEL_MAX,
};

enum RESUME_PATTERN_SEL {
	E_RESUME_PATTERN_SEL_DEFAULT,
	E_RESUME_PATTERN_SEL_MOD,
	E_RESUME_PATTERN_SEL_TCON,
	E_RESUME_PATTERN_SEL_GOP,
	E_RESUME_PATTERN_SEL_MULTIWIN,
	E_RESUME_PATTERN_SEL_MAX,
};

struct mtk_tv_kms_context {
	// common
	struct device *dev;
	struct drm_device *drm_dev;
	struct mtk_tv_drm_crtc crtc[MTK_DRM_CRTC_TYPE_MAX];
	struct drm_encoder encoder[MTK_DRM_ENCODER_TYPE_MAX];
	struct mtk_tv_drm_connector connector[MTK_DRM_CONNECTOR_TYPE_MAX];
	int graphic_irq_num;
	int graphic_irq_vgsep_num;
	int video_irq_num;
	bool video_irq_status[MTK_VIDEO_IRQEVENT_MAX];
	int plane_num[MTK_DRM_PLANE_TYPE_MAX];    // should read from dts
	int plane_index_start[MTK_DRM_PLANE_TYPE_MAX]; // should read from dts
	int total_plane_num;
	int blend_hw_version;
	struct drm_property *drm_common_plane_prop[E_EXT_COMMON_PLANE_PROP_MAX];
	struct ext_common_plane_prop_info *ext_common_plane_properties;
	struct drm_property *drm_crtc_prop[E_EXT_CRTC_PROP_MAX];
	struct ext_crtc_prop_info *ext_crtc_properties;
	struct drm_property *drm_crtc_graphic_prop[E_EXT_CRTC_GRAPHIC_PROP_MAX];
	struct ext_crtc_graphic_prop_info *ext_crtc_graphic_properties;
	struct drm_property *drm_common_crtc_prop[E_EXT_COMMON_CRTC_PROP_MAX];
	struct ext_common_crtc_prop_info *ext_common_crtc_properties;
	struct drm_property *drm_connector_prop[E_EXT_CONNECTOR_PROP_MAX];
	struct ext_connector_prop_info *ext_connector_properties;
	struct drm_property *drm_common_connector_prop[E_EXT_CONNECTOR_COMMON_PROP_MAX];
	struct ext_common_connector_prop_info *ext_common_connector_properties;
	// gop
	struct mtk_gop_context gop_priv;
	// video
	struct mtk_video_context video_priv;
	// pqmap
	struct mtk_pqmap_context pqmap_priv;
	// autogen
	struct mtk_autogen_context autogen_priv;
	// pqu_metadata
	struct mtk_pqu_metadata_context pqu_metadata_priv;
	// ld
	struct mtk_ld_context ld_priv;
	// global pq
	struct mtk_global_pq_context global_pq_priv;
	// panel
	struct mtk_panel_context panel_priv;
	// demura
	struct mtk_demura_context demura_priv;
	// tvdac
	struct mtk_tvdac_context tvdac_priv;
	// pixel shift
	struct mtk_pixelshift_context pixelshift_priv;
	//HW capability
	struct mtk_drmcap_dev drmcap_dev;
	enum OUTPUT_MODEL out_model;
	//pattern
	struct drm_pattern_status pattern_status;
	bool stub;	// test mode
	bool pnlgamma_stubmode;
	bool b2r_src_stubmode;
	// engine
	int render_p_engine;
	int pnlgamma_version;
	int pqgamma_version;
	bool irq_mask;
	struct drm_atomic_state *tvstate[MTK_DRM_CRTC_TYPE_MAX];
	//ire version
	int u32IRQ_Version;
	u16 swdelay;
	int mgw_version;
	int clk_version;
	enum RESUME_PATTERN_SEL eResumePatternselect;
};

void mtk_tv_kms_atomic_state_clear(struct drm_atomic_state *old_state);
int mtk_tv_kms_set_VG_ratio(struct mtk_tv_drm_crtc *mtk_tv_crtc);
int mtk_tv_drm_atomic_commit(struct drm_device *dev,
			     struct drm_atomic_state *state,
			     bool nonblock);
int mtk_tv_kms_CRTC_active_handler(struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state);

#endif
