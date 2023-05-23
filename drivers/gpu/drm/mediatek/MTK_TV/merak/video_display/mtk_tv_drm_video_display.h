/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_VIDEO_DISPLAY_H_
#define _MTK_TV_DRM_VIDEO_DISPLAY_H_

#include <linux/dma-buf.h>
#include <linux/workqueue.h>

#include "hwreg_render_video_display.h"
#include "m_pqu_pq.h"

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define MTK_PLANE_DISPLAY_PIPE (3)
#define MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_LINE (32)
#define MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_DIV (8)
#define MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_MAX (0xFF)
#define SHIFT_16_BITS  (16)
#define MAX_WINDOW_NUM (16)

#define pctx_video_to_kms(x)	container_of(x, struct mtk_tv_kms_context, video_priv)

#define ML_INVALID_MEM_INDEX	(0xFF)

extern bool bPquEnable;
//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------

enum ext_video_plane_prop {
	EXT_VPLANE_PROP_MUTE_SCREEN,
	EXT_VPLANE_PROP_SNOW_SCREEN,
	EXT_VPLANE_PROP_VIDEO_PLANE_TYPE,
	EXT_VPLANE_PROP_META_FD,
	EXT_VPLANE_PROP_FREEZE,
	EXT_VPLANE_PROP_INPUT_VFREQ,
	EXT_VPLANE_PROP_BUF_MODE,
	EXT_VPLANE_PROP_DISP_MODE,
	// memc property
	EXT_VPLANE_PROP_MEMC_LEVEL, /*15*/
	EXT_VPLANE_PROP_MEMC_GAME_MODE,
	EXT_VPLANE_PROP_MEMC_PATTERN,
	EXT_VPLANE_PROP_MEMC_MISC_TYPE,
	EXT_VPLANE_PROP_MEMC_DECODE_IDX_DIFF,
	EXT_VPLANE_PROP_MEMC_STATUS,
	EXT_VPLANE_PROP_MEMC_TRIG_GEN,
	EXT_VPLANE_PROP_MEMC_RV55_INFO,
	EXT_VPLANE_PROP_MEMC_INIT_VALUE_FOR_RV55,

	EXT_VPLANE_PROP_WINDOW_PQ,

	// pqmap
	EXT_VPLANE_PROP_PQMAP_NODE_ARRAY,
	EXT_VPLANE_PROP_MAX,
};

/**/
enum mtk_video_plane_memc_change {
	MEMC_CHANGE_NONE,
	MEMC_CHANGE_OFF,
	MEMC_CHANGE_ON,
};

enum mtk_video_frc_pattern {
	MEMC_PAT_OFF,
	MEMC_PAT_OPM,
	MEMC_PAT_IPM_DYNAMIC,
	MEMC_PAT_IPM_STATIC,
};

enum mtk_video_plane_memc_initstate {
	MEMC_INIT_CHECKRV55,
	MEMC_INIT_SENDDONE,
	MEMC_INIt_FINISH,

	MEMC_INT_STATE_NUM
};

enum video_mem_mode {
	VIDEO_MEM_MODE_SW,
	VIDEO_MEM_MODE_HW,
	VIDEO_MEM_MODE_MAX,
};

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
struct mtk_video_plane_type_num {
	uint8_t MEMC_num;
	uint8_t MGW_num;
	uint8_t DMA1_num;
	uint8_t DMA2_num;
};

struct video_plane_prop {
	uint64_t prop_val[EXT_VPLANE_PROP_MAX];//new prop value
	uint64_t old_prop_val[EXT_VPLANE_PROP_MAX];
	uint8_t prop_update[EXT_VPLANE_PROP_MAX];
};

struct disp_mode_info {
	enum drm_mtk_vplane_disp_mode disp_mode;
	bool disp_mode_update;
	bool bUpdateRWdiffInNextV;
	bool bUpdateRWdiffInNextVDone;
	bool freeze;
	bool bChanged;
};

struct mtk_video_plane_ctx {
	uint8_t rwdiff;
	uint8_t protectVal;
	struct dma_buf *meta_db;
	uint32_t src_w;
	uint32_t src_h;
	struct window dispwindow;
	struct window hvspoldcropwindow;
	struct window hvspolddispwindow;
	struct window dispolddispwindow;
	uint64_t oldbufferwidth;
	uint64_t oldbufferheight;
	struct window oldcropwindow;
	uint8_t old_MGW_plane_num;
	uint16_t oldPostFillWinEn;
	uint8_t old_disp_mode;
	uint8_t old_mem_hw_mode;
	struct disp_mode_info disp_mode_info;
	/*memc*/
	enum mtk_video_plane_memc_change memc_change_on;
	uint8_t rwdiff_memc;
	__u32 old_memc_hvspout_w_align; //Aligned
	__u32 old_memc_hvspout_h_align; //Aligned
	__u32 memc_pq_size;
	__u64 memc_pq_va;
	__u64 memc_pq_iova;
	char *pq_string;
};

struct mtk_video_plane_ml {
	int fd;
	int irq_fd;
	uint8_t memindex;
	uint8_t irq_memindex;
	uint32_t regcount;
	uint32_t irq_regcount;
};

struct mtk_video_plane_dv_gd_workqueue {
	struct workqueue_struct *wq;
	struct delayed_work dwq;
	struct dv_gd_info gd_info;
};

struct mtk_video_context {
	struct drm_property *drm_video_plane_prop[EXT_VPLANE_PROP_MAX];
	struct video_plane_prop *video_plane_properties;
	struct mtk_video_plane_ctx *plane_ctx;
	uint8_t plane_num[MTK_VPLANE_TYPE_MAX];
	uint8_t videoPlaneType_TypeNum;
	uint16_t byte_per_word;
	uint16_t disp_x_align;
	uint16_t disp_y_align;
	uint16_t disp_w_align;
	uint16_t disp_h_align;
	uint16_t crop_x_align_420_422;
	uint16_t crop_x_align_444;
	uint16_t crop_y_align_420;
	uint16_t crop_y_align_444_422;
	uint16_t crop_w_align;
	uint16_t crop_h_align;
	uint16_t reg_num;
	struct mtk_video_plane_ml disp_ml;
	struct mtk_video_plane_ml vgs_ml;
	uint8_t zpos_layer[MTK_VPLANE_TYPE_MAX];
	bool mgwdmaEnable[MTK_VPLANE_TYPE_MAX];
	bool extWinEnable[MTK_VPLANE_TYPE_MAX];
	bool preinsertEnable[MTK_VPLANE_TYPE_MAX];
	bool postfillEnable[MTK_VPLANE_TYPE_MAX];
	struct hwregMemConfigIn old_memcfg[MTK_VPLANE_TYPE_MAX][MAX_WINDOW_NUM];
	uint8_t old_layer[MTK_VPLANE_TYPE_MAX];
	bool old_layer_en[MTK_VPLANE_TYPE_MAX];
	uint8_t frc_ip_version;
	bool frcopmMaskEnable;
	bool bIspre_memc_en_Status;
	bool bIsmemc_1stInit;
	uint16_t frc_latency;
	uint16_t frc_rwdiff;
	enum mtk_video_frc_pattern frc_pattern_sel;
	uint8_t frc_pattern_trig_cnt;
	enum mtk_video_plane_memc_initstate memc_InitState;
	struct mtk_video_plane_dv_gd_workqueue dv_gd_wq;
};


struct mtk_video_format_info {
	uint32_t fourcc;
	uint64_t modifier;
	uint8_t modifier_arrange;
	uint8_t modifier_compressed;
	uint8_t modifier_6bit;
	uint8_t modifier_10bit;
	uint8_t bpp[3];
};
//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_video_display_init(
	struct device *dev,
	struct device *master,
	void *data,
	struct mtk_drm_plane **primary_plane,
	struct mtk_drm_plane **cursor_plane);
int mtk_video_display_unbind(
	struct device *dev);
void mtk_video_display_update_plane(
	struct mtk_plane_state *state);
void mtk_video_display_disable_plane(
	struct mtk_plane_state *state);
void mtk_video_display_atomic_crtc_flush(
	struct mtk_video_context *pctx_video);
void mtk_video_display_clear_plane_status(
	struct mtk_plane_state *state);
int mtk_video_display_atomic_set_plane_property(
	struct mtk_drm_plane *mplane,
	struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t val);
int mtk_video_display_atomic_get_plane_property(
	struct mtk_drm_plane *mplane,
	const struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t *val);
int mtk_video_check_plane(
	struct drm_plane_state *plane_state,
	const struct drm_crtc_state *crtc_state,
	struct mtk_plane_state *state);
void mtk_video_display_set_RW_diff_IRQ(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video);
int mtk_tv_video_display_suspend(
	struct platform_device *pdev,
	pm_message_t state);
int mtk_tv_video_display_resume(
	struct platform_device *pdev);

#endif
