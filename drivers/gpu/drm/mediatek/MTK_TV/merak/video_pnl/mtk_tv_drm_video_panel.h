/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_VIDEO_PANEL_H_
#define _MTK_TV_DRM_VIDEO_PANEL_H_

#include "mtk_tv_drm_panel_common.h"
#include "hwreg_render_common_trigger_gen.h"

#include "../mtk_tv_drm_plane.h"
#include "../mtk_tv_drm_drv.h"
#include "../mtk_tv_drm_crtc.h"
#include "../mtk_tv_drm_connector.h"
#include "../mtk_tv_drm_encoder.h"
#include <linux/gpio/consumer.h>

//-------------------------------------------------------------------------------------------------
// Define & Macro
//-------------------------------------------------------------------------------------------------

#define FILE_PATH_LEN (50)
#define PNL_FRC_TABLE_MAX_INDEX (20)
#define PNL_FRC_TABLE_ARGS_NUM (4)
#define MTK_GAMMA_LUT_SIZE	(1024)
#define PNL_INFO_VERSION             (1)
#define PNL_CUS_SETTING_VERSION      (1)
#define PNL_SWING_CONTROL_VERSION    (1)
#define PNL_PE_CONTROL_VERSION       (1)
#define PNL_LANE_ORDER_VERSION       (1)
#define PNL_OUTPUT_CONFIG_VERSION    (1)
#define PNL_COLOR_INFO_VERSION       (1)

// ST_PNL_INI_PARA version
#define ST_PNL_PARA_VERSION                              (1)

//Panel ini related
//#define PNL_COMPATIBLE_STR        "mediatek-panel"
#define PNL_CUS_SETTING_NODE      "panel-cus-setting"
#define PNL_OUTPUT_SWING_NODE     "panel-output-swing-level"
#define PNL_OUTPUT_PE_NODE        "panel-output-pre-emphasis"
#define PNL_LANE_ORDER_NODE       "panel-lane-order"
#define PNL_PKG_LANE_ORDER_NODE   "pkg_lane_order"
#define PNL_USR_LANE_ORDER_NODE   "usr_lane_order"
#define PNL_OUTPUT_CONFIG_NODE    "panel-output-config"
#define PNL_COLOR_INFO_NODE       "panel-color-info"
#define PNL_FRC_TABLE_NODE        "panel-frc-table-info"
#define PNL_HW_INFO_NODE			"hw-info"
#define PNL_COMBO_INFO_NODE			"ext_graph_combo_info"

/***********
 *MOD setting
 ************/
//define output mode
#define MOD_IN_IF_VIDEO (4)	//means 4p engine input
#define LVDS_MPLL_CLOCK_MHZ	(864)	// For crystal 24Mhz

/***********
 *REG setting
 ************/
#define REG_MAX_INDEX (50)

/***********
 *FRAMESYNC setting
 ************/
#define FRAMESYNC_VTTV_NORMAL_PHASE_DIFF (500)
#define FRAMESYNC_VTTV_SHORTEST_LOW_LATENCY_PHASE_DIFF (55)
#define FRAMESYNC_VTTV_LOW_LATENCY_PHASE_DIFF (100)
#define FRAMESYNC_VTTV_PHASE_DIFF_DIV_1000 (1000)
#define FRAMESYNC_VTTV_FRAME_COUNT (1)
#define FRAMESYNC_VTTV_THRESHOLD (3)
#define FRAMESYNC_VTTV_DIFF_LIMIT (0x7fff)
#define FRAMESYNC_VTTV_LOCKED_LIMIT (9)
#define FRAMESYNC_VTTV_LOCKKEEPSEQUENCETH (3)
#define FRAMESYNC_VTTV_LOCK_VTT_UPPER (0xFFFF)
#define FRAMESYNC_VTTV_LOCK_VTT_LOWER (0)
#define FRAMESYNC_VRR_DEPOSIT_COUNT (1)
#define FRAMESYNC_VRR_STATE_SW_MODE (4)
#define FRAMESYNC_VRR_CHASE_TOTAL_DIFF_RANGE (2)
#define FRAMESYNC_VRR_CHASE_PHASE_TARGET (2)
#define FRAMESYNC_VRR_CHASE_PHASE_ALMOST (2)
#define FRAMESYNC_RECOV_LOCKKEEPSEQUENCETH (3)
#define FRAMESYNC_RECOV_DIFF_LIMIT (0x7fff)

#define UHD_8K_W   (7680)
#define UHD_8K_H   (4320)

/*
 *###################################
 *#  Panel backlight related params.#
 *###################################
 */

#define PERIOD_PWM_TAG         "u32PeriodPWM"
#define DUTY_PWM_TAG           "u32DutyPWM"
#define DIV_PWM_TAG            "u16DivPWM"
#define POL_PWM_TAG            "bPolPWM"
#define MAX_PWM_VALUE_TAG      "u16MaxPWMvalue"
#define TYP_PWM_VAL_TAG        "u16TypPWMvalue"
#define MIN_PWM_VALUE_TAG      "u16MinPWMvalue"
#define PWM_PORT_TAG           "u16PWMPort"



/*
 *###########################
 *#    panel-cus-setting    #
 *###########################
 */
#define DTSI_FILE_TYPE      "DTSIFileType"
#define DTSI_FILE_VER     "DTSIFileVer"
#define GAMMA_EN_TAG      "PanelGamma_Enable"
#define GAMMA_BIN_TAG     "PanelGamme_BinPath"
#define TCON_EN_TAG       "TCON_Enable"
#define TCON_BIN_TAG      "TCON_BinPath"
#define MIRROR_TAG        "PanelMirrorMode"
#define OUTPUT_FORMAT_TAG "PanelOutputFormat"
#define SSC_EN_TAG        "SSC_Enable"
#define SSC_STEP_TAG      "SSC_Step"
#define SSC_SPAN_TAG      "SSC_Span"
#define OD_EN_TAG         "OverDrive_Enable"
#define OD_ADDR_TAG       "OverDrive_Buf_Addr"
#define OD_BUF_SIZE_TAG   "OverDrive_Buf_Size"
#define GPIO_BL_TAG       "GPIO_Backlight"
#define GPIO_VCC_TAG      "GPIO_VCC"
#define M_DELTA_TAG       "M_delta"
#define COLOR_PRIMA_TAG   "CustomerColorPrimaries"
#define SOURCE_WX_TAG     "SourceWx"
#define SOURCE_WY_TAG     "SourceWy"
#define VG_DISP_TAG       "VIDEO_GFX_DISPLAY_MODE"
#define OSD_HEIGHT_TAG    "OSD_Height"
#define OSD_WIDTH_TAG     "OSD_Width"
#define OSD_HS_START_TAG  "OSD_HsyncStart"
#define OSD_HS_END_TAG    "OSD_HsyncEnd"
#define OSD_HDE_START_TAG "OSD_HDEStart"
#define OSD_HDE_END_TAG   "OSD_HDEEnd"
#define OSD_HTOTAL_TAG    "OSD_HTotal"
#define OSD_VS_START_TAG  "OSD_VsyncStart"
#define OSD_VS_END_TAG    "OSD_VsyncEnd"
#define OSD_VDE_START_TAG "OSD_VDEStart"
#define OSD_VDE_END_TAG   "OSD_VDEEnd"
#define OSD_VTOTAL_TAG    "OSD_VTotal"

/*
 *###########################
 *#       swing level       #
 *###########################
 */
#define SWING_EN_TAG      "Swing_Level_Enable"
#define SWING_CHS_TAG     "Swing_Control_Channels"
#define SWING_CH3_0_TAG   "Swing_Level_CH3to0"
#define SWING_CH7_4_TAG   "Swing_Level_CH7to4"
#define SWING_CH11_8_TAG  "Swing_Level_CH11to8"
#define SWING_CH15_12_TAG "Swing_Level_CH15to12"
#define SWING_CH19_16_TAG "Swing_Level_CH19to16"
#define SWING_CH23_20_TAG "Swing_Level_CH23to20"
#define SWING_CH27_24_TAG "Swing_Level_CH27to24"
#define SWING_CH31_28_TAG "Swing_Level_CH31to28"

/*
 *###########################
 *#   pre-emphasis level    #
 *###########################
 */
#define PE_EN_TAG         "PE_Level_Enable"
#define PE_CHS_TAG        "PE_Control_Channels"
#define PE_CH3_0_TAG      "PE_Level_CH3to0"
#define PE_CH7_4_TAG      "PE_Level_CH7to4"
#define PE_CH11_8_TAG     "PE_Level_CH11to8"
#define PE_CH15_12_TAG    "PE_Level_CH15to12"
#define PE_CH19_16_TAG    "PE_Level_CH19to16"
#define PE_CH23_20_TAG    "PE_Level_CH23to20"
#define PE_CH27_24_TAG    "PE_Level_CH27to24"
#define PE_CH31_28_TAG    "PE_Level_CH31to28"

/*
 *###########################
 *#   output lane order     #
 *###########################
 */
#define LANE_CH3_0_TAG    "MOD_LANE_ORDER_CH3to0"
#define LANE_CH7_4_TAG    "MOD_LANE_ORDER_CH7to4"
#define LANE_CH11_8_TAG   "MOD_LANE_ORDER_CH11to8"
#define LANE_CH15_12_TAG  "MOD_LANE_ORDER_CH15to12"
#define LANE_CH19_16_TAG  "MOD_LANE_ORDER_CH19to16"
#define LANE_CH23_20_TAG  "MOD_LANE_ORDER_CH23to20"
#define LANE_CH27_24_TAG  "MOD_LANE_ORDER_CH27to24"
#define LANE_CH31_28_TAG  "MOD_LANE_ORDER_CH31to28"

/*
 *###########################
 *#      output config      #
 *###########################
 */
#define OUTCFG_CH3_0_TAG      "MOD_OUTPUT_CONFIG_CH3to0"
#define OUTCFG_CH7_4_TAG      "MOD_OUTPUT_CONFIG_CH7to4"
#define OUTCFG_CH11_8_TAG     "MOD_OUTPUT_CONFIG_CH11to8"
#define OUTCFG_CH15_12_TAG    "MOD_OUTPUT_CONFIG_CH15to12"
#define OUTCFG_CH19_16_TAG    "MOD_OUTPUT_CONFIG_CH19to16"
#define OUTCFG_CH23_20_TAG    "MOD_OUTPUT_CONFIG_CH23to20"
#define OUTCFG_CH27_24_TAG    "MOD_OUTPUT_CONFIG_CH27to24"
#define OUTCFG_CH31_28_TAG    "MOD_OUTPUT_CONFIG_CH31to28"

/*
 *###########################
 *#    output lane oder   #
 *###########################
 */
#define SUPPORT_LANES        "support_lanes"
#define LAYOUT_ORDER         "layout_order"
#define USR_DEFINE           "usr_define"
#define CTRL_LANES           "ctrl_lanes"
#define LANEORDER            "laneorder"

/*
 *###########################
 *#   panel-color-info      #
 *###########################
 */
#define COLOR_FORMAT_TAG  "COLOR_FORAMT"
#define RX_TAG            "Rx"
#define RY_TAG            "Ry"
#define GX_TAG            "Gx"
#define GY_TAG            "Gy"
#define BX_TAG            "Bx"
#define BY_TAG            "By"
#define WX_TAG            "Wx"
#define WY_TAG            "Wy"
#define MAX_LUMI_TAG      "MaxLuminance"
#define MED_LUMI_TAG      "MedLuminance"
#define MIN_LUMI_TAG      "MinLuminance"
#define LINEAR_RGB_TAG    "LinearRGB"
#define HDR_NITS_TAG      "HDRPanelNits"
#define FRC_TABLE_TAG     "FRC_TABLE"

/*
 *###########################
 *#        hw-info          #
 *###########################
 */
#define TGEN_VERSION					"tgen_version"
#define LPLL_VERSION					"lpll_version"
#define MOD_VERSION						"mod_version"
#define RCON_ENABLE						"rcon_enable"
#define RCON_MAX						"rcon_max"
#define RCON_MIN						"rcon_min"
#define RCON_VALUE						"rcon_value"
#define BIASCON_SINGLE_MAX				"biascon_single_max"
#define BIASCON_SINGLE_MIN				"biascon_single_min"
#define BIASCON_SINGLE_VALUE			"biascon_single_value"
#define BIASCON_BIASCON_DOUBLE_MAX		"biascon_double_max"
#define BIASCON_BIASCON_DOUBLE_MIN		"biascon_double_min"
#define BIASCON_BIASCON_DOUBLE_VALUE	"biascon_double_value"
#define RINT_ENABLE						"rint_enable"
#define RINT_MAX						"rint_max"
#define RINT_MIN						"rint_min"
#define RINT_VALUE						"rint_value"

/*
 *###########################
 *#   ext-graph-combo-info  #
 *###########################
 */
 #define GRAPH_VBO_BYTE_MODE	"graph_vbo_byte_mode"
//------------------------------------------------------------------------------
// Enum
//------------------------------------------------------------------------------

/// Define Mirror Type

typedef enum {
	E_PNL_MIRROR_NONE = 0,
	E_PNL_MIRROR_V,
	E_PNL_MIRROR_H,
	E_PNL_MIRROR_V_H,
	E_PNL_MIRROR_TYPE_MAX
} drm_en_pnl_mirror_type;

typedef enum {
	E_VG_COMB_MODE,
	E_VG_SEP_MODE,
	E_VG_SEP_W_DELTAV_MODE,
	E_VIDEO_GFX_DISP_MODE_MAX
} drm_en_video_gfx_disp_mode;

typedef enum {
	E_PNL_GAMMA_TABLE_FROM_BIN,
	E_PNL_GAMMA_TABLE_FROM_REG,
	E_PNL_GAMMA_TABLE_TYPE_MAX,
} drm_en_pnl_PanelGammaTableType;

/// Define the panel gamma bin entry
typedef enum {
	///< Indicate PNL Gamma is 256 entrise
	E_PNL_PNLGAMMA_256_ENTRIES = 0,
	///< Indicate PNL Gamma is 1024 entries
	E_PNL_PNLGAMMA_1024_ENTRIES,
	///< Indicate PNL Gamma is MAX entries
	E_PNL_PNLGAMMA_MAX_ENTRIES
} drm_en_pnl_Gamma_Entries;

/// Define the panel gamma precision type
typedef enum {
	///< Gamma Type of 10bit
	E_PNL_GAMMA_10BIT = 0,
	///< Gamma Type of 12bit
	E_PNL_GAMMA_12BIT,
	///< The library can support all mapping mode
	E_PNL_GAMMA_ALL
} drm_en_pnl_Gamma_Type;

//Define Framesync state
enum drm_en_pnl_framesync_state {
	E_PNL_FRAMESYNC_STATE_PROP_IN = 0,
	E_PNL_FRAMESYNC_STATE_PROP_FIRE,
	E_PNL_FRAMESYNC_STATE_IRQ_IN,
	E_PNL_FRAMESYNC_STATE_IRQ_FIRE,
	E_PNL_FRAMESYNC_STATE_MAX,
};

//-------------------------------------------------------------------------------------------------
// Structure
//-------------------------------------------------------------------------------------------------
extern struct platform_driver mtk_tv_drm_panel_driver;
extern struct platform_driver mtk_tv_drm_gfx_panel_driver;
extern struct platform_driver mtk_tv_drm_extdev_panel_driver;

typedef struct  {
	uint8_t hsync_w;
	uint16_t de_hstart;
	uint8_t lane_numbers;
	drm_en_pnl_output_format op_format;
} st_hprotect_info;

typedef struct {
	drm_st_pnl_info src_info;
	drm_mod_cfg out_cfg;
	bool v_path_en;
	bool extdev_path_en;
	bool gfx_path_en;
	__u16 output_model_ref;
	st_hprotect_info video_hprotect_info;
	st_hprotect_info delta_hprotect_info;
	st_hprotect_info gfx_hprotect_info;
} drm_parse_dts_info;

typedef struct {
	__u16 version;
	__u16 length;
	__u16 dtsi_file_type;
	__u16 dtsi_file_ver;
	bool pgamma_en;
	char pgamma_path[FILE_PATH_LEN];
	bool tcon_en;
	char tcon_path[FILE_PATH_LEN];
	drm_en_pnl_mirror_type mirror_mode;
	bool hmirror_en;
	drm_en_pnl_output_format op_format;
	bool ssc_en;
	__u16 ssc_step;
	__u16 ssc_span;
	bool od_en;
	__u64 od_buf_addr;
	__u32 od_buf_size;
	__u16 gpio_bl;
	__u16 gpio_vcc;
	__u16 m_del;
	__u16 cus_color_prim;
	__u16 src_wx;
	__u16 src_wy;
	drm_en_video_gfx_disp_mode disp_mode;
	__u16 osd_height;
	__u16 osd_width;
	__u16 osd_hs_st;
	__u16 osd_hs_end;
	__u16 osd_hde_st;
	__u16 osd_hde_end;
	__u16 osd_htt;
	__u16 osd_vs_st;
	__u16 osd_vs_end;
	__u16 osd_vde_st;
	__u16 osd_vde_end;
	__u16 osd_vtt;
	__u32 period_pwm;
	__u32 duty_pwm;
	__u16 div_pwm;
	bool pol_pwm;
	__u16 max_pwm_val;
	__u16 min_pwm_val;
	__u16 pwmport;
} drm_st_pnl_cus_setting;

typedef struct {
	__u16 version;
	__u16 length;
	bool en;
	__u16 ctrl_chs;
	__u32 ch3_0;
	__u32 ch7_4;
	__u32 ch11_8;
	__u32 ch15_12;
	__u32 ch19_16;
	__u32 ch23_20;
	__u32 ch27_24;
	__u32 ch31_28;
} drm_st_output_ch_info;

typedef struct {
	__u16  version;
	__u16  length;
	drm_en_pnl_output_format format;
	__u16 rx;
	__u16 ry;
	__u16 gx;
	__u16 gy;
	__u16 bx;
	__u16 by;
	__u16 wx;
	__u16 wy;
	__u8 maxlum;
	__u8 medlum;
	__u8 minlum;
	bool linear_rgb;
	__u16 hdrnits;
} drm_st_pnl_color_info;

struct drm_st_pnl_frc_table_info {
	__u16 u16LowerBound;
	__u16 u16HigherBound;
	__u8 u8FRC_in;      //ivs
	__u8 u8FRC_out;     //ovs
};

struct drm_st_output_timing_info {
	__u32 u32OutputVfreq;
	__u16 u16OutputVTotal;
	enum video_crtc_frame_sync_mode eFrameSyncMode;
	__u8 u8FRC_in;      //ivs
	__u8 u8FRC_out;     //ovs
	bool locked_flag;
	bool AutoForceFreeRun;
	bool ForceFreeRun;
	enum drm_en_pnl_framesync_state eFrameSyncState;
	enum drm_hwreg_triggen_src_sel input_src;
};

typedef struct {
	__u32 tgen_hw_version;
	__u32 lpll_hw_version;
	__u32 mod_hw_version;
	bool rcon_enable;
	__u32 rcon_max;
	__u32 rcon_min;
	__u32 rcon_value;
	__u32 biascon_single_max;
	__u32 biascon_single_min;
	__u32 biascon_single_value;
	__u32 biascon_double_max;
	__u32 biascon_double_min;
	__u32 biascon_double_value;
	bool rint_enable;
	__u32 rint_max;
	__u32 rint_min;
	__u32 rint_value;
} drm_st_hw_info;

typedef struct {
	__u32 graph_vbo_byte_mode;
} drm_st_ext_graph_combo_info;

struct mtk_panel_context {
	//struct drm_property * drm_video_plane_prop[E_VIDEO_PLANE_PROP_MAX];
	//struct video_plane_prop * video_plane_properties;

	drm_st_pnl_info info;
	drm_st_pnl_info extdev_info;
	drm_st_pnl_info gfx_info;
	drm_st_pnl_cus_setting cus;
	drm_st_output_ch_info swing;
	drm_st_output_ch_info pe;
	drm_st_output_ch_info lane_order;
	drm_st_output_ch_info op_cfg;
	drm_st_pnl_color_info color_info;
	drm_mod_cfg out_cfg_test; //out cfg for test
	drm_mod_cfg v_cfg;
	drm_st_out_lane_order out_lane_info;
	struct drm_st_pnl_frc_table_info frc_info[PNL_FRC_TABLE_MAX_INDEX];
	struct drm_st_output_timing_info outputTiming_info;
	struct drm_st_pnl_frc_ratio_info frcRatio_info;
	drm_update_output_timing_type out_upd_type;
	struct drm_panel drmPanel;
	struct device *dev;
	__u8 u8TimingsNum;
	struct list_head panelInfoList;
	struct gpio_desc *gpio_vcc;
	struct gpio_desc *gpio_backlight;
	__u8 u8controlbit_gfid;
	drm_st_hw_info hw_info;
	drm_st_ext_graph_combo_info ext_grpah_combo_info;
	bool disable_ModeID_change;
};

typedef struct {
	/// Struct version
	uint32_t u32Version;
	/// Sturct length
	uint32_t u32Length;
	/// IN: select Get gamma value from
	drm_en_pnl_PanelGammaTableType enPnlGammaTbl;
	/// IN: Bin file address
	uint8_t *pu8GammaTbl;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDummy0;
#endif
	/// IN: PNL Gamma bin size
	uint32_t  u32TblSize;
	/// IN/OUT: R channel 0 data
	uint16_t *pu16RChannel0;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDummy1;
#endif
	/// IN/OUT: G channel 0 data
	uint16_t *pu16GChannel0;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDumm2;
#endif
	/// IN/OUT: B channel 0 data
	uint16_t *pu16BChannel0;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDummy3;
#endif
	/// IN/OUT: W channel 0 data
	uint16_t *pu16WChannel0;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDummy4;
#endif
	/// IN/OUT: R channel 1 data
	uint16_t *pu16RChannel1;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDummy5;
#endif
	/// IN/OUT: G channel 1 data
	uint16_t *pu16GChannel1;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDummy6;
#endif
	/// IN/OUT: B channel 1 data
	uint16_t *pu16BChannel1;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDummy7;
#endif
	/// IN/OUT: W channel 1 data
	uint16_t *pu16WChannel1;
#if !defined(__aarch64__)
	/// Dummy parameter
	void *pDummy8;
#endif
	/// IN: the index of the data want to get
	uint16_t u16Index;
	/// IN: every channel data number
	uint16_t u16Size;
	/// OUT: Support PNL gamma Channel 1
	uint8_t bSupportChannel1;
	// OUT: PNL gamma bin entries
	drm_en_pnl_Gamma_Entries enGammaEntries;
	// OUT: PNL gamma bin type
	drm_en_pnl_Gamma_Type enGammaType;
	// OUT: PNL gamma bin 2D table number
	uint16_t u162DTableNum;
	// OUT: PNL gamma bin 3D table number
	uint16_t u163DTableNum;
} drm_st_pnl_gammatbl;


//-------------------------------------------------------------------------------------------------
// Function
//-------------------------------------------------------------------------------------------------
struct mtk_tv_kms_context;

void mtk_render_output_en(
	struct mtk_panel_context *pctx, bool bEn);
int mtk_render_set_output_hmirror_enable(
	struct mtk_panel_context *pctx);
bool mtk_render_cfg_connector(
	struct mtk_panel_context *pctx);
bool mtk_render_cfg_crtc(
	struct mtk_panel_context *pctx);
int _mtk_video_set_customize_frc_table(
	struct mtk_panel_context *pctx_pnl,
	struct drm_st_pnl_frc_table_info *customizeFRCTableInfo);
int _mtk_video_get_customize_frc_table(
	struct mtk_panel_context *pctx_pnl,
	void *CurrentFRCTableInfo);
int _mtk_video_set_framesync_mode(
	struct mtk_tv_kms_context *pctx);
int _mtk_video_set_low_latency_mode(
	struct mtk_tv_kms_context *pctx,
	uint64_t bLowLatencyEn);
int mtk_get_framesync_locked_flag(
	struct mtk_tv_kms_context *pctx);
bool mtk_video_panel_get_framesync_mode_en(
	struct mtk_tv_kms_context *pctx);
int mtk_video_panel_update_framesync_state(
	struct mtk_tv_kms_context *pctx);

int mtk_tv_drm_check_out_mod_cfg(
	struct mtk_panel_context *pctx_pnl,
	drm_parse_dts_info *src_info);

int mtk_tv_drm_check_out_lane_cfg(
	struct mtk_panel_context *pctx_pnl);

int mtk_render_set_vbo_ctrlbit(
	struct mtk_panel_context *pctx_pnl,
	struct drm_st_ctrlbits *pctrlbits);

int mtk_render_set_tx_mute(
	struct mtk_panel_context *pctx_pnl,
	drm_st_tx_mute_info tx_mute_info);

int mtk_render_set_tx_mute_common(
	struct mtk_panel_context *pctx_pnl,
	drm_st_tx_mute_info tx_mute_info,
	uint8_t u8connector_id);

int mtk_render_set_pixel_mute_video(
	drm_st_pixel_mute_info pixel_mute_info);

int mtk_render_set_gop_pattern_en(bool bEn);
int mtk_render_set_multiwin_pattern_en(bool bEn);
int mtk_render_set_tcon_pattern_en(bool bEn);


int mtk_render_set_pixel_mute_deltav(
	drm_st_pixel_mute_info pixel_mute_info);

int mtk_render_set_pixel_mute_gfx(
	drm_st_pixel_mute_info pixel_mute_info);

int mtk_render_set_backlight_mute(
	struct mtk_panel_context *pctx,
	drm_st_backlight_mute_info backlight_mute_info);

int mtk_render_set_swing_vreg(
	struct drm_st_out_swing_level *stSwing);

int mtk_render_get_swing_vreg(
	struct drm_st_out_swing_level *stSwing);

int mtk_render_set_pre_emphasis(
	struct drm_st_out_pe_level *stPE);

int mtk_render_get_pre_emphasis(
	struct drm_st_out_pe_level *stPE);

int mtk_render_set_ssc(
	struct drm_st_out_ssc *stSSC);

int mtk_render_get_ssc(
	struct drm_st_out_ssc *stSSC);

int mtk_render_get_mod_status(
	struct drm_st_mod_status *stdrmMod);

int mtk_tv_drm_panel_checkDTSPara(drm_st_pnl_info *pStPnlInfo);

int mtk_tv_drm_video_get_outputinfo_ioctl(
	struct drm_device *drm,
	void *data,
	struct drm_file *file_priv);

int mtk_tv_drm_check_video_hbproch_protect(drm_parse_dts_info *data);

int mtk_video_get_vttv_input_vfreq(
	struct mtk_tv_kms_context *pctx,
	uint64_t plane_id,
	uint64_t *input_vfreq);

#endif
