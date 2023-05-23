/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_VIDEO_H_
#define _MTK_TV_DRM_VIDEO_H_

//#include "sti_msos.h"
#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_crtc.h"

#define FILE_PATH_LEN (50)
#define DRM_NAME_MAX_NUM (256)
#define PNL_FRC_TABLE_MAX_INDEX (20)
#define PNL_FRC_TABLE_ARGS_NUM (4)
#define MTK_GAMMA_LUT_SIZE	(512)

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
#define PNL_COMPATIBLE_STR        "mediatek-panel"
#define PNL_INFO_NODE	"panel-info"
#define PNL_CUS_SETTING_NODE      "panel-cus-setting"
#define PNL_OUTPUT_SWING_NODE     "panel-output-swing-level"
#define PNL_OUTPUT_PE_NODE        "panel-output-pre-emphasis"
#define PNL_LANE_ORDER_NODE       "panel-lane-order"
#define PNL_OUTPUT_CONFIG_NODE    "panel-output-config"
#define PNL_COLOR_INFO_NODE       "panel-color-info"
#define PNL_FRC_TABLE_NODE        "panel-frc-table-info"

//Board ini related
#define MTK_DRM_DTS_BOARD_INFO_NODE "board-info"
//MMAP related
#define MTK_DRM_DTS_MMAP_INFO_NODE "mmap-info"
/*
 *###########################
 *#       panel-info        #
 *###########################
 */
#define NAME_TAG          "m_pPanelName"
#define LINK_TYPE_TAG "m_ePanelLinkType"
#define SWAP_PORT_TAG "m_bPanelSwapPort"
#define SWAP_ODD_TAG "m_bPanelSwapOdd_ML"
#define SWAP_EVEN_TAG "m_bPanelSwapEven_ML"
#define SWAP_ODD_RB_TAG "m_bPanelSwapOdd_RB"
#define SWAP_EVEN_RB_TAG "m_bPanelSwapEven_RB"
#define SWAP_LVDS_POL_TAG "m_bPanelSwapLVDS_POL"
#define SWAP_LVDS_CH_TAG  "m_bPanelSwapLVDS_CH"
#define PDP_10BIT_TAG     "m_bPanelPDP10BIT"

#define LVDS_TI_MODE_TAG "m_bPanelLVDS_TI_MODE"
#define DCLK_DELAY_TAG "m_ucPanelDCLKDelay"
#define INV_DCLK_TAG "m_bPanelInvDCLK"
#define INV_DE_TAG "m_bPanelInvDE"
#define INV_HSYNC_TAG "m_bPanelInvHSync"
#define INV_VSYNC_TAG "m_bPanelInvVSync"

#define ON_TIMING_1_TAG "m_wPanelOnTiming1"
#define ON_TIMING_2_TAG "m_wPanelOnTiming2"
#define OFF_TIMING_1_TAG "m_wPanelOffTiming1"
#define OFF_TIMING_2_TAG "m_wPanelOffTiming2"
#define HSYNC_WIDTH_TAG "m_ucPanelHSyncWidth"
#define HSYNC_BACK_PORCH_TAG "m_ucPanelHSyncBackPorch"
#define VSYNC_WIDTH_TAG "m_ucPanelVSyncWidth"
#define VSYNC_BACK_PORCH_TAG "m_ucPanelVBackPorch"
#define HSTART_TAG "m_wPanelHStart"
#define VSTART_TAG "m_wPanelVStart"
#define WIDTH_TAG "m_wPanelWidth"
#define HEIGHT_TAG "m_wPanelHeight"
#define MAX_HTOTAL_TAG "m_wPanelMaxHTotal"
#define HTOTAL_TAG "m_wPanelHTotal"
#define MIN_HTOTAL_TAG "m_wPanelMinHTotal"
#define MAX_VTOTAL_TAG "m_wPanelMaxVTotal"
#define VTOTAL_TAG "m_wPanelVTotal"
#define MIN_VTOTAL_TAG "m_wPanelMinVTotal"
#define MAX_DCLK_TAG "m_dwPanelMaxDCLK"
#define DCLK_TAG "m_dwPanelDCLK"
#define MIN_DCLK_TAG "m_dwPanelMinDCLK"

#define TI_BIT_MODE_TAG "m_ucTiBitMode"
#define OUTPUT_BIT_MODE_TAG "m_ucOutputFormatBitMode"
#define LVDS_TX_SWAP_TAG  "m_u16LVDSTxSwapValue"
#define SWAP_ODD_RG_TAG "m_bPanelSwapOdd_RG"
#define SWAP_EVEN_RG_TAG "m_bPanelSwapEven_RG"
#define SWAP_ODD_GB_TAG "m_bPanelSwapOdd_GB"
#define SWAP_EVEN_GB_TAG "m_bPanelSwapEven_GB"
#define NOISE_DITH_TAG "m_bPanelNoiseDith"

/*
 *###################################
 *#  Panel backlight related params.#
 *###################################
 */

#define PERIOD_PWM_TAG "u32PeriodPWM"
#define DUTY_PWM_TAG "u32DutyPWM"
#define DIV_PWM_TAG "u16DivPWM"
#define POL_PWM_TAG "bPolPWM"
#define MAX_PWM_VALUE_TAG "u16MaxPWMvalue"
#define TYP_PWM_VAL_TAG "u16TypPWMvalue"
#define MIN_PWM_VALUE_TAG "u16MinPWMvalue"
#define PWM_PORT_TAG      "u16PWMPort"

#define VRR_EN_TAG        "m_bVRR_en"
#define VRR_MIN_TAG       "m_wVRR_min"
#define VRR_MAX_TAG       "m_wVRR_max"

/*
 *###########################
 *#    panel-cus-setting    #
 *###########################
 */
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
 *#   panel-color-info      #
 *###########################
 */
#define COLOR_SPACE_TAG   "COLOR_SPACE"
#define COLOR_FORMAT_TAG  "COLOR_FORAMT"
#define COLOR_RANGE_TAG   "COLOR_RANGE"
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

/*panel related enum and structure*/
typedef enum {
	MTK_DRM_VIDEO_LANTENCY_TYPE_FRAME_LOCK,
	MTK_DRM_VIDEO_LANTENCY_TYPE_MAX,
} drm_en_video_latency_type;

/// Define PANEL Signaling Type
typedef enum {
	///< TTL  type
	LINK_TTL,
	///< LVDS type
	LINK_LVDS,
	///< RSDS type
	LINK_RSDS,
	///< TCON
	LINK_MINILVDS,
	///< Analog TCON
	LINK_ANALOG_MINILVDS,
	///< Digital TCON
	LINK_DIGITAL_MINILVDS,
	///< Ursa (TTL output to Ursa)
	LINK_MFC,
	///< DAC output
	LINK_DAC_I,
	///< DAC output
	LINK_DAC_P,
	///< For PDP(Vsync use Manually MODE)
	LINK_PDPLVDS,
	/// EXT LPLL TYPE
	LINK_EXT,
	/// 10
	LINK_EPI34_8P = LINK_EXT,
	/// 11
	LINK_EPI28_8P,
	/// 12
	LINK_EPI34_6P,
	/// 13
	LINK_EPI28_6P,

	/// replace this with LINK_MINILVDS
	///LINK_MINILVDS_6P_2L,
	/// 14
	LINK_MINILVDS_5P_2L,
	/// 15
	LINK_MINILVDS_4P_2L,
	/// 16
	LINK_MINILVDS_3P_2L,
	/// 17
	LINK_MINILVDS_6P_1L,
	/// 18
	LINK_MINILVDS_5P_1L,
	/// 19
	LINK_MINILVDS_4P_1L,
	/// 20
	LINK_MINILVDS_3P_1L,

	/// 21
	LINK_HS_LVDS,
	/// 22
	LINK_HF_LVDS,

	/// 23
	LINK_TTL_TCON,
	//  2 channel, 3 pair, 8 bits ///24
	LINK_MINILVDS_2CH_3P_8BIT,
	//  2 channel, 4 pair, 8 bits ///25
	LINK_MINILVDS_2CH_4P_8BIT,
	// 2 channel, 5 pair, 8 bits ///26
	LINK_MINILVDS_2CH_5P_8BIT,
	// 2 channel, 6 pair, 8 bits ///27
	LINK_MINILVDS_2CH_6P_8BIT,

	// 1 channel, 3 pair, 8 bits ///28
	LINK_MINILVDS_1CH_3P_8BIT,
	// 1 channel, 4 pair, 8 bits ///29
	LINK_MINILVDS_1CH_4P_8BIT,
	// 1 channel, 5 pair, 8 bits ///30
	LINK_MINILVDS_1CH_5P_8BIT,
	// 1 channel, 6 pair, 8 bits ///31
	LINK_MINILVDS_1CH_6P_8BIT,

	// 2 channel, 3 pari, 6 bits ///32
	LINK_MINILVDS_2CH_3P_6BIT,
	// 2 channel, 4 pari, 6 bits ///33
	LINK_MINILVDS_2CH_4P_6BIT,
	// 2 channel, 5 pari, 6 bits ///34
	LINK_MINILVDS_2CH_5P_6BIT,
	//  2 channel, 6 pari, 6 bits ///35
	LINK_MINILVDS_2CH_6P_6BIT,

	// 1 channel, 3 pair, 6 bits ///36
	LINK_MINILVDS_1CH_3P_6BIT,
	// 1 channel, 4 pair, 6 bits ///37
	LINK_MINILVDS_1CH_4P_6BIT,
	// 1 channel, 5 pair, 6 bits ///38
	LINK_MINILVDS_1CH_5P_6BIT,
	// 1 channel, 6 pair, 6 bits ///39
	LINK_MINILVDS_1CH_6P_6BIT,
	//   HDMI Bypass Mode///40
	LINK_HDMI_BYPASS_MODE,

	/// 41
	LINK_EPI34_2P,
	/// 42
	LINK_EPI34_4P,
	/// 43
	LINK_EPI28_2P,
	/// 44
	LINK_EPI28_4P,

	///45
	LINK_VBY1_10BIT_4LANE,
	///46
	LINK_VBY1_10BIT_2LANE,
	///47
	LINK_VBY1_10BIT_1LANE,
	///48
	LINK_VBY1_8BIT_4LANE,
	///49
	LINK_VBY1_8BIT_2LANE,
	///50
	LINK_VBY1_8BIT_1LANE,

	///51
	LINK_VBY1_10BIT_8LANE,
	///52
	LINK_VBY1_8BIT_8LANE,

	///53
	LINK_EPI28_12P,

	//54
	LINK_HS_LVDS_2CH_BYPASS_MODE,
	//55
	LINK_VBY1_8BIT_4LANE_BYPASS_MODE,
	//56
	LINK_VBY1_10BIT_4LANE_BYPASS_MODE,
	///57
	LINK_EPI24_12P,
	///58
	LINK_VBY1_10BIT_16LANE,
	///59
	LINK_VBY1_8BIT_16LANE,
	///60
	LINK_USI_T_8BIT_12P,
	///61
	LINK_USI_T_10BIT_12P,
	///62
	LINK_ISP_8BIT_12P,
	///63
	LINK_ISP_8BIT_6P_D,

	///64
	LINK_ISP_8BIT_8P,
	///65
	LINK_ISP_10BIT_12P,
	///66
	LINK_ISP_10BIT_6P_D,
	///67
	LINK_ISP_10BIT_8P,
	///68
	LINK_EPI24_16P,
	///69
	LINK_EPI28_16P,
	///70
	LINK_EPI28_6P_EPI3G,
	///71
	LINK_EPI28_8P_EPI3G,

	///72
	LINK_CMPI24_10BIT_12P,
	///73
	LINK_CMPI27_8BIT_12P,
	///74
	LINK_CMPI27_8BIT_8P,
	///75
	LINK_CMPI24_10BIT_8P,
	///76
	LINK_USII_8BIT_12X1_4K,
	///77
	LINK_USII_8BIT_6X1_D_4K,
	///78
	LINK_USII_8BIT_8X1_4K,
	///79
	LINK_USII_8BIT_6X1_4K,
	///80
	LINK_USII_8BIT_6X2_4K,
	///81
	LINK_USII_10BIT_12X1_4K,
	///82
	LINK_USII_10BIT_8X1_4K,

	///83
	LINK_EPI24_8P,
	///84
	LINK_USI_T_8BIT_8P,
	///85
	LINK_USI_T_10BIT_8P,
	///86
	LINK_MINILVDS_4CH_6P_8BIT,
	///87
	LINK_EPI28_12P_EPI3G,
	///88
	LINK_EPI28_16P_EPI3G,

	///89
	LINK_MIPI_DSI_RGB888_4lane,
	///90
	LINK_MIPI_DSI_RGB565_4lane,
	///91
	LINK_MIPI_DSI_RGB666_4lane,
	///92
	LINK_MIPI_DSI_RGB888_3lane,
	///93
	LINK_MIPI_DSI_RGB565_3lane,
	///94
	LINK_MIPI_DSI_RGB666_3lane,
	///95
	LINK_MIPI_DSI_RGB888_2lane,
	///96
	LINK_MIPI_DSI_RGB565_2lane,
	///97
	LINK_MIPI_DSI_RGB666_2lane,
	///98
	LINK_MIPI_DSI_RGB888_1lane,
	///99
	LINK_MIPI_DSI_RGB565_1lane,
	///100
	LINK_MIPI_DSI_RGB666_1lane,

	///101
	LINK_NUM,
} drm_en_pnl_link_type; //APIPNL_LINK_TYPE;


typedef enum {
	/// BIN
	E_PNL_FILE_TCON_BIN_PATH,
	E_PNL_FILE_PANELGAMMA_BIN_PATH,
	E_PNL_FILE_MIPI_DCS_BIN_PATH,
	/// The max support number of paths
	E_PNL_FILE_PATH_MAX,
} drm_en_pnl_file_path;

typedef enum {
	E_APIPNL_TCON_TAB_TYPE_GENERAL,
	E_APIPNL_TCON_TAB_TYPE_GPIO,
	E_APIPNL_TCON_TAB_TYPE_SCALER,
	E_APIPNL_TCON_TAB_TYPE_MOD,
	E_APIPNL_TCON_TAB_TYPE_GAMMA,
	E_APIPNL_TCON_TAB_TYPE_POWER_SEQUENCE_ON,
	E_APIPNL_TCON_TAB_TYPE_POWER_SEQUENCE_OFF,
	E_APIPNL_TCON_TAB_TYPE_PANEL_INFO,
	E_APIPNL_TCON_TAB_TYPE_OVERDRIVER,
	E_APIPNL_TCON_TAB_TYPE_PCID,
	E_APIPNL_TCON_TAB_TYPE_PATCH,
	E_APIPNL_TCON_TAB_TYPE_LINE_OD_TABLE,
	E_APIPNL_TCON_TAB_TYPE_LINE_OD_REG,
	E_APIPNL_TCON_TAB_TYPE_EVA_REG,
} drm_en_pnl_tcon_tab_type;

/// Define aspect ratio
typedef enum {
	///< set aspect ratio to 4 : 3
	E_PNL_ASPECT_RATIO_4_3 = 0,
	///< set aspect ratio to 16 : 9
	E_PNL_ASPECT_RATIO_WIDE,
	///< resvered for other aspect ratio other than 4:3/ 16:9
	E_PNL_ASPECT_RATIO_OTHER,
} drm_en_pnl_aspect_ratio; //E_PNL_ASPECT_RATIO;

/// Define TI bit mode
typedef enum {
	TI_10BIT_MODE = 0,
	TI_8BIT_MODE = 2,
	TI_6BIT_MODE = 3,
} drm_en_pnl_tibitmode; //APIPNL_TIBITMODE;

/// Define panel output format bit mode
typedef enum {
	// default is 10bit, because 8bit panel can use 10bit config and
	// 8bit config.
	OUTPUT_10BIT_MODE = 0,
	// but 10bit panel(like PDP panel) can only use 10bit config.
	OUTPUT_6BIT_MODE = 1,
	// and some PDA panel is 6bit.
	OUTPUT_8BIT_MODE = 2,
} drm_en_pnl_outputformat_bitmode; // APIPNL_OUTPUTFORMAT_BITMODE;

/// Define which panel output timing change mode is used to change VFreq
/// for same panel
typedef enum {
	///<change output DClk to change Vfreq.
	E_PNL_CHG_DCLK   = 0,
	///<change H total to change Vfreq.
	E_PNL_CHG_HTOTAL = 1,
	///<change V total to change Vfreq.
	E_PNL_CHG_VTOTAL = 2,
} drm_en_pnl_out_timing_mode; //APIPNL_OUT_TIMING_MODE;

/// Define Panel MISC control index
/// please enum use BIT0 = 0x01, BIT1 = 0x02, BIT2 = 0x04, BIT3 = 0x08,
/// BIT4 = 0x10,
typedef enum {
	E_APIPNL_MISC_CTRL_OFF   = 0x0000,
	E_APIPNL_MISC_MFC_ENABLE = 0x0001,
	E_APIPNL_MISC_SKIP_CALIBRATION = 0x0002,
	E_APIPNL_MISC_GET_OUTPUT_CONFIG = 0x0004,
	E_APIPNL_MISC_SKIP_ICONVALUE = 0x0008,

	// bit 4
	E_APIPNL_MISC_MFC_MCP    = 0x0010,
	// bit 5
	E_APIPNL_MISC_MFC_ABChannel = 0x0020,
	// bit 6
	E_APIPNL_MISC_MFC_ACChannel = 0x0040,
	// bit 7, for 60Hz Panel
	E_APIPNL_MISC_MFC_ENABLE_60HZ = 0x0080,
	// bit 8, for 240Hz Panel
	E_APIPNL_MISC_MFC_ENABLE_240HZ = 0x0100,
	// bit 9, for 4k2K 60Hz Panel
	E_APIPNL_MISC_4K2K_ENABLE_60HZ = 0x0200,
	// bit 10, for T3D control
	E_APIPNL_MISC_SKIP_T3D_CONTROL = 0x0400,
	// bit 11, enable pixel shift
	E_APIPNL_MISC_PIXELSHIFT_ENABLE = 0x0800,
	// enable manual V sync control
	E_APIPNL_MISC_ENABLE_MANUAL_VSYNC_CTRL = 0x8000,
} drm_en_pnl_misc;

/// Define Mirror Type
typedef enum {
	E_PNL_MOD_H_MIRROR = 0,
	E_PNL_MIRROR_TYPE_MAX,
} drm_en_pnl_mirror_type;

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

typedef enum {
	E_OUTPUT_RGB,
	E_OUTPUT_YUV444,
	E_OUTPUT_YUV422,
	E_OUTPUT_YUV420,
	E_OUTPUT_RGBA1010108,
	E_OUTPUT_RGBA8888,
	E_PNL_OUTPUT_FORMAT_NUM,
} drm_en_pnl_output_format;

typedef enum {
	E_VG_COMB_MODE,
	E_VG_SEP_MODE,
	E_VG_SEP_W_DELTAV_MODE,
	E_VIDEO_GFX_DISP_MODE_MAX
} drm_en_video_gfx_disp_mode;

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
	E_OUTPUT_MODE_MAX,
} drm_output_mode;

typedef enum {
	E_LINK_NONE,
	E_LINK_VB1,
	E_LINK_LVDS,
	E_LINK_MAX,
} drm_pnl_link_if;

typedef struct {
	drm_pnl_link_if linkIF;
	__u8 lanes;
	__u8 div_sec;
	drm_output_mode timing;
	drm_en_pnl_output_format format;
} drm_mod_cfg;

//----------------------------------------------------------------------------
//MApi_PNL_Setting enum of cmd
//----------------------------------------------------------------------------
typedef enum {
	E_PNL_MOD_PECURRENT_SETTING,
	E_PNL_CONTROL_OUT_SWING,
	E_PNL_UPDATE_INI_PARA,
	E_PNL_CONTROL_OUT_SWING_CHANNEL,
	E_PNL_MIPI_UPDATE_PNL_TIMING,
	E_PNL_MIPI_DCS_COMMAND,
	E_PNL_SETTING_CMD_NUM,
	E_PNL_SETTING_CMD_MAX = E_PNL_SETTING_CMD_NUM,
} drm_en_pnl_setting;

/// Define the set timing information
typedef struct {
	///<high accurate input V frequency
	__u32 u32HighAccurateInputVFreq;
	///<input V frequency
	__u16 u16InputVFreq;
	///<input vertical total
	__u16 u16InputVTotal;
	///<MVOP source
	bool bMVOPSrc;
	///<whether it's fast frame lock case
	bool bFastFrameLock;
	///<whether it's interlace
	bool bInterlace;
} drm_st_mtk_tv_timing_info;

typedef struct {
	//< Version of current structure. Please always set to
	//< "ST_XC_VIDEO_LATENCY_INFO" as input
	__u32 u32Version;
	//< Length of this structure, u16Length=sizeof(ST_XC_VIDEO_LATENCY_INFO)
	__u16 u16Length;

	// latency type
	drm_en_video_latency_type enType;
	// frame lock or not
	bool bIsFrameLock;
	// leading or lagging
	bool bIsLeading;
	// video latency (ns)
	__u32  u32VideoLatency;
	// input frame rate x 10 (ex. 24FPS = 240)
	__u16  u16InputFrameRate;
} drm_st_mtk_tv_video_latency_info;

/// A panel struct type used to specify the panel attributes, and settings
/// from Board layout
typedef struct __packed {
	const char *m_pPanelName;                ///<  PanelName
#if !(defined(UFO_PUBLIC_HEADER_212) || defined(UFO_PUBLIC_HEADER_300))
#if !defined(__aarch64__)
	__u32 u32AlignmentDummy0;
#endif
#endif
	//
	//  Panel output
	//
	///<  PANEL_DITHER, keep the setting
	__u8 m_bPanelDither : 1;
	///<  PANEL_LINK
	drm_en_pnl_link_type m_ePanelLinkType   : 4;

	///////////////////////////////////////////////
	// Board related setting
	///////////////////////////////////////////////
	///<  VOP_21[8], MOD_4A[1], PANEL_DUAL_PORT, refer to m_bPanelDoubleClk
	__u8 m_bPanelDualPort  : 1;
	///<  MOD_4A[0], PANEL_SWAP_PORT, refer to "LVDS output app note"
	///   A/B channel swap
	__u8 m_bPanelSwapPort  : 1;
	///<  PANEL_SWAP_ODD_ML
	__u8 m_bPanelSwapOdd_ML    : 1;
	///<  PANEL_SWAP_EVEN_ML
	__u8 m_bPanelSwapEven_ML   : 1;
	///<  PANEL_SWAP_ODD_RB
	__u8 m_bPanelSwapOdd_RB    : 1;
	///<  PANEL_SWAP_EVEN_RB
	__u8 m_bPanelSwapEven_RB   : 1;

	///<  MOD_40[5], PANEL_SWAP_LVDS_POL, for differential P/N swap
	__u8 m_bPanelSwapLVDS_POL  : 1;
	///<  MOD_40[6], PANEL_SWAP_LVDS_CH, for pair swap
	__u8 m_bPanelSwapLVDS_CH   : 1;
	///<  MOD_40[3], PANEL_PDP_10BIT ,for pair swap
	__u8 m_bPanelPDP10BIT      : 1;
	///<  MOD_40[2], PANEL_LVDS_TI_MODE, refer to "LVDS output app note"
	__u8 m_bPanelLVDS_TI_MODE  : 1;

	///////////////////////////////////////////////
	// For TTL Only
	///////////////////////////////////////////////
	///<  PANEL_DCLK_DELAY
	__u8 m_ucPanelDCLKDelay;
	///<  MOD_4A[4], PANEL_INV_DCLK
	__u8 m_bPanelInvDCLK   : 1;
	///<  MOD_4A[2], PANEL_INV_DE
	__u8 m_bPanelInvDE     : 1;
	///<  MOD_4A[12], PANEL_INV_HSYNC
	__u8 m_bPanelInvHSync  : 1;
	///<  MOD_4A[3], PANEL_INV_VSYNC
	__u8 m_bPanelInvVSync  : 1;

	///////////////////////////////////////////////
	// Output driving current setting
	///////////////////////////////////////////////
	///<  driving current setting (0x00=4mA, 0x01=6mA, 0x02=8mA, 0x03=12mA)
	///<  define PANEL_DCLK_CURRENT
	__u8 m_ucPanelDCKLCurrent;
	///<  define PANEL_DE_CURRENT
	__u8 m_ucPanelDECurrent;
	///<  define PANEL_ODD_DATA_CURRENT
	__u8 m_ucPanelODDDataCurrent;
	///<  define PANEL_EVEN_DATA_CURRENT
	__u8 m_ucPanelEvenDataCurrent;

	///////////////////////////////////////////////
	// panel on/off timing
	///////////////////////////////////////////////
	///<  time between panel & data while turn on power
	__u16 m_wPanelOnTiming1;
	///<  time between data & back light while turn on power
	__u16 m_wPanelOnTiming2;
	///<  time between back light & data while turn off power
	__u16 m_wPanelOffTiming1;
	///<  time between data & panel while turn off power
	__u16 m_wPanelOffTiming2;

	///////////////////////////////////////////////
	// panel timing spec.
	///////////////////////////////////////////////
	// sync related
	///<  VOP_01[7:0], PANEL_HSYNC_WIDTH
	__u8 m_ucPanelHSyncWidth;
	///<  PANEL_HSYNC_BACK_PORCH, no register setting, provide value
	///<  for query only
	__u8 m_ucPanelHSyncBackPorch;

	///<  not support Manuel VSync Start/End now
	///<  VOP_02[10:0] VSync start = Vtt - VBackPorch - VSyncWidth
	///<  VOP_03[10:0] VSync end = Vtt - VBackPorch

	///<  define PANEL_VSYNC_WIDTH
	__u8 m_ucPanelVSyncWidth;
	///<  define PANEL_VSYNC_BACK_PORCH
	__u8 m_ucPanelVBackPorch;

	// DE related
	///<  VOP_04[11:0], PANEL_HSTART, DE H Start
	// (PANEL_HSYNC_WIDTH + PANEL_HSYNC_BACK_PORCH)
	__u16 m_wPanelHStart;
	///<  VOP_06[11:0], PANEL_VSTART, DE V Start
	__u16 m_wPanelVStart;
	///< PANEL_WIDTH, DE width (VOP_05[11:0] = HEnd = HStart + Width - 1)
	__u16 m_wPanelWidth;
	///< PANEL_HEIGHT, DE height
	// (VOP_07[11:0], = Vend = VStart + Height - 1)
	__u16 m_wPanelHeight;

	// DClk related
	///<  PANEL_MAX_HTOTAL. Reserved for future using.
	__u16 m_wPanelMaxHTotal;
	///<  VOP_0C[11:0], PANEL_HTOTAL
	__u16 m_wPanelHTotal;
	///<  PANEL_MIN_HTOTAL. Reserved for future using.
	__u16 m_wPanelMinHTotal;

	///<  PANEL_MAX_VTOTAL. Reserved for future using.
	__u16 m_wPanelMaxVTotal;
	///<  VOP_0D[11:0], PANEL_VTOTAL
	__u16 m_wPanelVTotal;
	///<  PANEL_MIN_VTOTAL. Reserved for future using.
	__u16 m_wPanelMinVTotal;

	///<  PANEL_MAX_DCLK. Reserved for future using.
	__u8 m_dwPanelMaxDCLK;
	///<  LPLL_0F[23:0], PANEL_DCLK ,{0x3100_10[7:0], 0x3100_0F[15:0]}
	__u8 m_dwPanelDCLK;
	///<  PANEL_MIN_DCLK. Reserved for future using.
	__u8 m_dwPanelMinDCLK;

	///<  spread spectrum
	///<  move to board define, no use now.
	__u16 m_wSpreadSpectrumStep;
	///<  move to board define, no use now.
	__u16 m_wSpreadSpectrumSpan;

	///<  Initial Dimming Value
	__u8 m_ucDimmingCtl;
	///<  Max Dimming Value
	__u8 m_ucMaxPWMVal;
	///<  Min Dimming Value
	__u8 m_ucMinPWMVal;

	///<  define PANEL_DEINTER_MODE,  no use now
	__u8 m_bPanelDeinterMode   :1;
	///<  Panel Aspect Ratio, provide information to upper layer
	///<  application for aspect ratio setting.
	drm_en_pnl_aspect_ratio m_ucPanelAspectRatio;
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
	__u16 m_u16LVDSTxSwapValue;
	///< MOD_4B[1:0], refer to "LVDS output app note"
	drm_en_pnl_tibitmode m_ucTiBitMode;
	drm_en_pnl_outputformat_bitmode m_ucOutputFormatBitMode;

	///<  define PANEL_SWAP_ODD_RG
	__u8 m_bPanelSwapOdd_RG    :1;
	///<  define PANEL_SWAP_EVEN_RG
	__u8 m_bPanelSwapEven_RG   :1;
	///<  define PANEL_SWAP_ODD_GB
	__u8 m_bPanelSwapOdd_GB    :1;
	///<  define PANEL_SWAP_EVEN_GB
	__u8 m_bPanelSwapEven_GB   :1;

/**
 *  Others
 */
	///<  LPLL_03[7], define Double Clock ,LVDS dual mode
	__u8 m_bPanelDoubleClk     :1;
	///<  define PANEL_MAX_SET
	__u32 m_dwPanelMaxSET;
	///<  define PANEL_MIN_SET
	__u32 m_dwPanelMinSET;
	///<Define which panel output timing change mode is used to change
	///VFreq for same panel
	drm_en_pnl_out_timing_mode m_ucOutTimingMode;
	///<  PAFRC mixed with noise dither disable
	__u8 m_bPanelNoiseDith     :1;

	//Vsync related
	///<  Vsync Start
	__u16 m_u16VsyncStart;

	//HDMI2.1 VRR
	// variable refresh rate
	bool bVRR_en;
	// vrr max frame rate
	__u16 u16VRRMaxRate;
	// vrr min frame rate
	__u16 u16VRRMinRate;
	bool bCinemaVRR_en;
	// change vtt every time by m_delta
	__u16 u16M_delta;
} drm_st_pnl_type;

typedef struct {
	//version control
	__u16 version;
	__u16 length;
	char pnlname[DRM_NAME_MAX_NUM];                ///<  PanelName
	///<  PANEL_LINK
	drm_en_pnl_link_type linktype;
	///////////////////////////////////////////////
	// Board related setting
	///////////////////////////////////////////////
	///   A/B channel swap
	bool swap_port;
	///<  PANEL_SWAP_ODD_ML
	bool swapodd_ml;
	///<  PANEL_SWAP_EVEN_ML
	bool swapeven_ml;
	///<  PANEL_SWAP_ODD_RB
	bool swapodd_rb;
	///<  PANEL_SWAP_EVEN_RB
	bool swapeven_rb;
	///<  PANEL_SWAP_LVDS_POL, for differential P/N swap
	bool swaplvds_pol;
	///<  PANEL_SWAP_LVDS_CH, for pair swap
	bool swaplvds_ch;
	///<  PANEL_PDP_10BIT ,for pair swap
	bool pdp_10bit;
	///<  PANEL_LVDS_TI_MODE
	bool lvds_ti_mode;
	///////////////////////////////////////////////
	// For TTL Only
	///////////////////////////////////////////////
	///<  PANEL_DCLK_DELAY
	__u8 dclk_dely;
	///<  PANEL_INV_DCLK
	bool inv_dclk;
	///<  PANEL_INV_DE
	bool inv_de;
	///<  PANEL_INV_HSYNC
	bool inv_hsync;
	///<  PANEL_INV_VSYNC
	bool inv_vsync;

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
	///<  PANEL_HSYNC_WIDTH
	__u8 hsync_w;
	///<  PANEL_HSYNC_BACK_PORCH
	__u8 hbp;
	///<  define PANEL_VSYNC_WIDTH
	__u8 vsync_w;
	///<  define PANEL_VSYNC_BACK_PORCH
	__u8 vbp;

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
	__u16 max_dclk;
	__u16 typ_dclk;
	///<  PANEL_MIN_DCLK. Reserved for future using.
	__u16 min_dclk;

	drm_en_pnl_tibitmode ti_bitmode;
	drm_en_pnl_outputformat_bitmode op_bitmode;
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
	__u16 lvds_txswap;
	///<  define PANEL_SWAP_ODD_RG
	bool swapodd_rg;
	///<  define PANEL_SWAP_EVEN_RG
	bool swapeven_rg;
	///<  define PANEL_SWAP_ODD_GB
	bool swapodd_gb;
	///<  define PANEL_SWAP_EVEN_GB
	bool swapeven_gb;
	///<  PAFRC mixed with noise dither disable
	bool noise_dith;

/*
 *###########################
 *#  Panel backlight related params.#
 *###########################
 */
	__u32 period_pwm;
	__u32 duty_pwm;
	__u16 div_pwm;
	bool pol_pwm;
	__u16 max_pwm_val;
	__u16 typ_pwm_val;
	__u16 min_pwm_val;
	__u16 pwmport;


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
} drm_st_pnl_info;

typedef struct {
	__u16 version;
	__u16 length;
	bool pgamma_en;
	char pgamma_path[FILE_PATH_LEN];
	bool tcon_en;
	char tcon_path[FILE_PATH_LEN];
	drm_en_pnl_mirror_type mirror_mode;
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
	enum drm_color_encoding space;
	drm_en_pnl_output_format format;
	enum drm_color_range range;
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
	__u16 u16OutputVfreq;
	__u16 u16OutputVTotal;
	enum video_crtc_frame_sync_mode eFrameSyncMode;
};

typedef struct {
	bool bEnable;  //Enable output pattern
	__u16 u16Red;  //Red channel value
	__u16 u16Green;  //Green channel value
	__u16 u16Blue;     //Blue channel value
} drm_st_pnl_output_pattern;

typedef struct {
	__u32 u32OutputCFG0_7;  //setup output config channel 00~07
	__u32 u32OutputCFG8_15;  //setup output config channel 08~15
	__u32 u32OutputCFG16_21;  //setup output config channel 16~21
} drm_st_mod_output_config_user;

typedef struct {
	///<Version of current structure.
	///Please always set to "MIRROR_INFO_VERSION" as input
	__u32 u32Version;
	///<Length of this structure,
	///u32MirrorInfo_Length=sizeof(ST_APIPNL_MIRRORINFO)
	__u32 u32Length;
	drm_en_pnl_mirror_type eMirrorType;   ///< Mirror type
	bool bEnable;
} drm_st_pnl_mirror_info;

typedef struct {
	__u16 *pu16ODTbl;
	///<Version of current structure.
	__u32 u32PNL_version;
	// OD frame buffer related
	///<OverDrive MSB frame buffer start address,
	///absolute without any alignment
	__u32 u32OD_MSB_Addr;
	///<OverDrive MSB frame buffer size, the unit is BYTES
	__u32 u32OD_MSB_Size;
	///<OverDrive LSB frame buffer start address,
	///absolute without any alignment
	__u32 u32OD_LSB_Addr;
	///<OverDrive MSB frame buffer size, the unit is BYTES
	__u32 u32OD_LSB_Size;
	///<OverDrive Table buffer size>
	__u32 u32ODTbl_Size;
} drm_st_pnl_od_setting;

typedef struct {
	/// Structure version
	__u32 u32Version;
	///<  PANEL_MAX_DCLK. Reserved for future using.
	__u32 u32PanelMaxDCLK;
	///<  LPLL_0F[23:0], PANEL_DCLK ,{0x3100_10[7:0], 0x3100_0F[15:0]}
	__u32 u32PanelDCLK;
	///<  PANEL_MIN_DCLK. Reserved for future using.
	__u32 u32PanelMinDCLK;
} drm_st_pnl_init_para_setting;

typedef struct {
	__u16 u16PnlHstart;	//get panel horizontal start
	__u16 u16PnlVstart;	//get panel vertical start
	__u16 u16PnlWidth;	//get panel width
	__u16 u16PnlHeight;	//get panel height
	__u16 u16PnlHtotal;	//get panel horizontal total
	__u16 u16PnlVtotal;	//get panel vertical total
	__u16 u16PnlHsyncWitth;	//get panel horizontal sync width
	__u16 u16PnlHsyncBackPorch;	//get panel horizontal sync back porch
	__u16 u16PnlVsyncBackPorch;	//get panel vertical sync back porch
	__u16 u16PnlDefVfreq;	//get default vfreq
	__u16 u16PnlLPLLMode;	//get panel LPLL mode
	__u16 u16PnlLPLLType;	//get panel LPLL type
	__u16 u16PnlMinSET;	//get panel minima SET value
	__u16 u16PnlMaxSET;	//get panel maxma SET value
} drm_pnl_info;

//LPLL setting
typedef struct {
	__u16 u16PhaseLimit;
	__u32 u32D5D6D7;
	__u8 u8Igain;
	__u8 u8Pgain;
	__u16 u16VttUpperBound;
	__u16 u16VttLowerBound;
	__u8 u8DiffLimit;
} drm_pnl_PanelLpllCusSetting;

//OD setting for get info from board ini
typedef struct {
	__u8 u8ODEnable;
	__u32 u32OD_MSB_Addr;
	__u8 u32OD_MSB_Size;
} drm_st_pnl_od_control;

typedef struct DLL_PACKED {
	/// Structure version
	__u32 u32Version;
	///<  PANEL_MAX_DCLK. Reserved for future using.
	__u32 u32PanelMaxDCLK;
	///<  LPLL_0F[23:0], PANEL_DCLK ,{0x3100_10[7:0], 0x3100_0F[15:0]}
	__u32 u32PanelDCLK;
	///<  PANEL_MIN_DCLK. Reserved for future using.
	__u32 u32PanelMinDCLK;
} drm_st_pnl_ini_para;

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

/*End of panel related enum and structure*/

/*MEMC related enum and structure*/
struct property_blob_memc_status {
	uint8_t u8MemcEnabled;
	uint8_t u8MemcLevelStatus;
};

#define MEMC_CMD_PARAMETER_NUM_MAX 8
struct property_blob_memc_info {
	char u8Cmd;
	char u8Count; //counter to arrP valid setting
	char arrP[MEMC_CMD_PARAMETER_NUM_MAX];
};
/*End of MEMC related enum and structure*/

struct mtk_panel_context {
	//struct drm_property * drm_video_plane_prop[E_VIDEO_PLANE_PROP_MAX];
	//struct video_plane_prop * video_plane_properties;
	drm_st_pnl_type stPanelType;
	drm_pnl_PanelLpllCusSetting stCusLpllSetting;
	__u16 u16PanelLinkExtType;
	__u16 u16LvdsOutputType;
	__u8 u8SpreadSpectrumEnable;
	drm_st_pnl_od_control stODControl;

	drm_st_pnl_info info;
	drm_st_pnl_cus_setting cus;
	drm_st_output_ch_info swing;
	drm_st_output_ch_info pe;
	drm_st_output_ch_info lane_order;
	drm_st_output_ch_info op_cfg;
	drm_st_pnl_color_info color_info;
	drm_mod_cfg v_cfg;
	drm_mod_cfg deltav_cfg;
	drm_mod_cfg gfx_cfg;
	struct drm_st_pnl_frc_table_info frc_info[PNL_FRC_TABLE_MAX_INDEX];
	struct drm_st_output_timing_info outputTiming_info;
};

int mtk_panel_init(struct device *dev);
int readDTB2PanelPrivate(struct mtk_panel_context *pctx_panel);
int mtk_video_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val,
	int prop_index);
int mtk_video_atomic_get_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val,
	int prop_index);
void mtk_video_atomic_crtc_flush(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state);
int mtk_video_atomic_set_connector_property(
	struct mtk_tv_drm_connector *connector,
	struct drm_connector_state *state,
	struct drm_property *property,
	uint64_t val);
int mtk_video_atomic_get_connector_property(
	struct mtk_tv_drm_connector *connector,
	const struct drm_connector_state *state,
	struct drm_property *property,
	uint64_t *val);
void mtk_video_panel_gamma_setting(
	struct drm_crtc *crtc,
	uint16_t *r,
	uint16_t *g,
	uint16_t *b,
	uint32_t size);
#endif
