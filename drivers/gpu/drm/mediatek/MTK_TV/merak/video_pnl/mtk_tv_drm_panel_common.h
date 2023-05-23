/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_PANEL_COMMON_H_
#define _MTK_TV_DRM_PANEL_COMMON_H_

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>
#include <drm/drmP.h>
#include <drm/drm_panel.h>
#include <drm/drm_modes.h>
#include <video/display_timing.h>
#include <video/videomode.h>
#include <drm/mtk_tv_drm.h>
#include "hwreg_render_video_pnl.h"
#include "common/hwreg_common_riu.h"

//-------------------------------------------------------------------------------------------------
// Define & Macro
//-------------------------------------------------------------------------------------------------
#define EXTDEV_NODE			"ext_video_out"
#define GFX_NODE			"graphic_out"

#define PNL_MOD_NODE              "mod"
#define PNL_MOD_REG               "reg"
#define PNL_LPLL_NODE             "lpll"
#define PNL_LPLL_REG              "reg"

#define PNL_OUTPUT_MAX_LANES	(64)

#define DRM_NAME_MAX_NUM (256)

#define IS_OUT_8K4K(width, height) (((width <= 8000) && (width >= 7500)) && \
							((height <= 4500) && (height >= 4000)))
#define IS_OUT_4K2K(width, height) (((width <= 4000) && (width >= 3750)) && \
							((height <= 2250) && (height >= 2000)))
#define IS_OUT_2K1K(width, height) (((width <= 2000) && (width >= 1875)) && \
							((height <= 1125) && (height >= 1000)))
#define IS_VFREQ_60HZ_GROUP(vfreq) ((vfreq < 65) && (vfreq > 45))
#define IS_VFREQ_120HZ_GROUP(vfreq) ((vfreq < 125) && (vfreq > 90))
#define IS_VFREQ_144HZ_GROUP(vfreq) ((vfreq < 149) && (vfreq > 139))

/*
 *###########################
 *#       panel-info        #
 *###########################
 */
#define NAME_TAG		"panel_name"
#define LINK_TYPE_TAG		"link_type"
#define VBO_BYTE_TAG		"vbo_byte_mode"
#define DIV_SECTION_TAG		"div_section"
#define ON_TIMING_1_TAG		"on_timing1"
#define ON_TIMING_2_TAG		"on_timing2"
#define OFF_TIMING_1_TAG	"off_timing1"
#define OFF_TIMING_2_TAG	"off_timing2"
#define HSYNC_ST_TAG		"hsync_start"
#define HSYNC_WIDTH_TAG		"hsync_width"
#define HSYNC_POL_TAG		"hsync_polarity"
#define VSYNC_ST_TAG		"vsync_start"
#define VSYNC_WIDTH_TAG		"vsync_width"
#define VSYNC_POL_TAG		"vsync_polarity"
#define HSTART_TAG		"de_hstart"
#define VSTART_TAG		"de_vstart"
#define WIDTH_TAG		"resolution_width"
#define HEIGHT_TAG		"resolution_height"
#define MAX_HTOTAL_TAG		"max_h_total"
#define HTOTAL_TAG		"typ_h_total"
#define MIN_HTOTAL_TAG		"min_h_total"
#define MAX_VTOTAL_TAG		"max_v_total"
#define VTOTAL_TAG		"typ_v_total"
#define MIN_VTOTAL_TAG		"min_v_total"
#define MAX_DCLK_TAG		"max_clk"
#define DCLK_TAG		"typ_clk"
#define MIN_DCLK_TAG		"min_clk"
#define OUTPUT_FMT_TAG		"output_format"
#define COUTPUT_FMT_TAG		"content_output_format"
#define VRR_EN_TAG             "vrr_en"
#define VRR_MIN_TAG            "min_vrr_framerate"
#define VRR_MAX_TAG            "max_vrr_framerate"
#define GRAPHIC_MIXER1_OUTMODE_TAG	"graphic_mixer1_outmode"
#define GRAPHIC_MIXER2_OUTMODE_TAG	"graphic_mixer2_outmode"

//------------------------------------------------------------------------------
// Enum
//------------------------------------------------------------------------------
typedef enum {
	E_OUTPUT_RGB,
	E_OUTPUT_YUV444,
	E_OUTPUT_YUV422,
	E_OUTPUT_YUV420,
	E_OUTPUT_ARGB8101010,
	E_OUTPUT_ARGB8888_W_DITHER,
	E_OUTPUT_ARGB8888_W_ROUND,
	E_OUTPUT_ARGB8888_MODE0,
	E_OUTPUT_ARGB8888_MODE1,
	E_PNL_OUTPUT_FORMAT_NUM,
} drm_en_pnl_output_format;

typedef enum {
	E_OUTPUT_MODE_NONE,
	E_8K4K_120HZ,
	E_8K4K_60HZ,
	E_8K4K_30HZ,
	E_4K2K_120HZ,
	E_4K2K_60HZ,
	E_4K2K_30HZ,
	E_FHD_120HZ,
	E_FHD_60HZ,
	E_4K2K_144HZ,
	E_OUTPUT_MODE_MAX,
} drm_output_mode;

typedef enum {
	E_LINK_NONE,
	E_LINK_VB1,
	E_LINK_LVDS,
	E_LINK_VB1_TO_HDMITX,
	E_LINK_MAX,
} drm_pnl_link_if;

typedef enum {
	E_NO_UPDATE_TYPE,
	E_MODE_RESET_TYPE,
	E_MODE_CHANGE_TYPE,
	E_OUTPUT_TYPE_MAX,
} drm_update_output_timing_type;

typedef enum {
	E_VBO_NO_LINK = 0,
	E_VBO_3BYTE_MODE = 3,
	E_VBO_4BYTE_MODE = 4,
	E_VBO_5BYTE_MODE = 5,
	E_VBO_MODE_MAX,
} drm_en_vbo_bytemode;

//-------------------------------------------------------------------------------------------------
// Structure
//-------------------------------------------------------------------------------------------------
typedef struct {
	drm_pnl_link_if linkIF;
	__u8 lanes;
	__u8 div_sec;
	drm_output_mode timing;
	drm_en_pnl_output_format format;
} drm_mod_cfg;

typedef struct {
	//version control
	__u16 version;
	__u16 length;
	char pnlname[DRM_NAME_MAX_NUM];                ///<  PanelName
	///<  PANEL_LINK
	drm_pnl_link_if linkIF;
	drm_en_vbo_bytemode vbo_byte;
	__u16 div_section;

	///////////////////////////////////////////////
	// Board related setting
	///////////////////////////////////////////////
	///   A/B channel swap
	//bool swap_port;
	///<  PANEL_SWAP_ODD_ML
	//bool swapodd_ml;
	///<  PANEL_SWAP_EVEN_ML
	//bool swapeven_ml;
	///<  PANEL_SWAP_ODD_RB
	//bool swapodd_rb;
	///<  PANEL_SWAP_EVEN_RB
	//bool swapeven_rb;
	///<  PANEL_SWAP_LVDS_POL, for differential P/N swap
	//bool swaplvds_pol;
	///<  PANEL_SWAP_LVDS_CH, for pair swap
	//bool swaplvds_ch;
	///<  PANEL_PDP_10BIT ,for pair swap
	//bool pdp_10bit;
	///<  PANEL_LVDS_TI_MODE
	//bool lvds_ti_mode;

	///////////////////////////////////////////////
	// panel on/off timing
	///////////////////////////////////////////////
	///<  time between panel & data while turn on power
	__u16 ontiming_1;
	///<  time between data & back light while turn on power
	__u16 ontiming_2;
	///<  time between back light & data while turn off power
	__u16 offtiming_1;
	///<  time between data & panel while turn off power
	__u16 offtiming_2;

	///////////////////////////////////////////////
	// panel timing spec.
	///////////////////////////////////////////////
	// sync related
	__u16 hsync_st;
	///<  PANEL_HSYNC_WIDTH
	__u8 hsync_w;
	__u8 hsync_pol;

	__u16 vsync_st;
	///<  define PANEL_VSYNC_WIDTH
	__u8 vsync_w;
	__u8 vsync_pol;

	// DE related
	///<  PANEL_HSTART, DE H Start
	__u16 de_hstart;
	///<  PANEL_VSTART, DE V Start
	__u16 de_vstart;
	///< PANEL_WIDTH, DE width
	__u16 de_width;
	///< PANEL_HEIGHT, DE height
	__u16 de_height;

	// DClk related
	///<  PANEL_MAX_HTOTAL. Reserved for future using.
	__u16 max_htt;
	__u16 typ_htt;
	///<  PANEL_MIN_HTOTAL. Reserved for future using.
	__u16 min_htt;

	///<  PANEL_MAX_VTOTAL. Reserved for future using.
	__u16 max_vtt;
	__u16 typ_vtt;
	///<  PANEL_MIN_VTOTAL. Reserved for future using.
	__u16 min_vtt;

	///<  PANEL_MAX_DCLK. Reserved for future using.
	__u64 max_dclk;
	__u64 typ_dclk;
	///<  PANEL_MIN_DCLK. Reserved for future using.
	__u64 min_dclk;

	//drm_en_pnl_tibitmode ti_bitmode;
	//drm_en_pnl_outputformat_bitmode op_bitmode;
	drm_en_pnl_output_format op_format;
	drm_en_pnl_output_format cop_format;

/*
 *
 * Board related params
 *
 *  If a board ( like BD_MST064C_D01A_S ) swap LVDS TX polarity
 *    : This polarity swap value =
 *      (LVDS_PN_SWAP_H<<8) | LVDS_PN_SWAP_L from board define,
 *  Otherwise
 *    : The value shall set to 0.
 */

	//__u16 lvds_txswap;
	///<  define PANEL_SWAP_ODD_RG
	//bool swapodd_rg;
	///<  define PANEL_SWAP_EVEN_RG
	//bool swapeven_rg;
	///<  define PANEL_SWAP_ODD_GB
	//bool swapodd_gb;
	///<  define PANEL_SWAP_EVEN_GB
	//bool swapeven_gb;
	///<  PAFRC mixed with noise dither disable
	//bool noise_dith;

/**
 *  Others
 */
	//HDMI2.1 VRR
	// variable refresh rate
	bool vrr_en;
	// vrr max frame rate
	__u16 vrr_max_v;
	// vrr min frame rate
	__u16 vrr_min_v;

	__u32 graphic_mixer1_out_prealpha;
	__u32 graphic_mixer2_out_prealpha;

	struct display_timing displayTiming;

	struct list_head list;

} drm_st_pnl_info;

typedef struct {
	uint32_t sup_lanes;
	uint32_t def_layout[PNL_OUTPUT_MAX_LANES];
	bool usr_defined;
	uint32_t ctrl_lanes;
	uint32_t lane_order[PNL_OUTPUT_MAX_LANES];
} drm_st_out_lane_order;

struct mtk_drm_panel_context {
	drm_st_pnl_info pnlInfo;
	drm_mod_cfg cfg;
	drm_update_output_timing_type out_upd_type;
	struct drm_panel drmPanel;
	struct device *dev;
	__u8 u8TimingsNum;
	drm_st_out_lane_order out_lane_info;
	bool lane_duplicate_en;
	struct list_head panelInfoList;
};

//-------------------------------------------------------------------------------------------------
// Function
//-------------------------------------------------------------------------------------------------
__u32 get_dts_u32(struct device_node *np, const char *name);
__u32 get_dts_u32_index(struct device_node *np, const char *name, int index);
void init_mod_cfg(drm_mod_cfg *pCfg);

__u32 setDisplayTiming(drm_st_pnl_info *pStPnlInfo);

void set_out_mod_cfg(drm_st_pnl_info *src_info,
								drm_mod_cfg *dst_cfg);
void dump_mod_cfg(drm_mod_cfg *pCfg);
void dump_panel_info(drm_st_pnl_info *pStPnlInfo);
void panel_common_enable(void);
void panel_common_disable(void);
int _parse_panel_info(struct device_node *np,
	drm_st_pnl_info *currentPanelInfo,
	struct list_head *panelInfoList);
int _copyPanelInfo(struct drm_crtc_state *crtc_state,
	drm_st_pnl_info *pCurrentInfo,
	struct list_head *pInfoList);
int _panelAddModes(struct drm_panel *panel,
	struct list_head *pInfoList,
	int *modeCouct);
bool _isSameModeAndPanelInfo(struct drm_display_mode *mode,
		drm_st_pnl_info *pPanelInfo);


#endif
