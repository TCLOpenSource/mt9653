/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_DRM_LD_H_
#define _MTK_DRM_LD_H_

#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_crtc.h"


#define MTK_PLANE_DISPLAY_PIPE (3)

//=============================================================================
// Type and Structure Declaration
//=============================================================================
enum drm_en_ld_type {

	E_LD_EDGE_TB_TYPE = 0,
	E_LD_EDGE_LR_TYPE = 1,
	E_LD_DIRECT_TYPE  = 2,
	E_LD_LOCAL_TYPE   = 3,
	E_LD_10K_TYPE     = 4,
	E_LD_TYPE_NUM,
	E_LD_TYPE_MAX = E_LD_TYPE_NUM,
};

enum drm_en_ld_boosting_mode {

	E_BOOSTING_OFF = 0,
	E_BOOSTING_FIXED_CURRENT = 1,
	E_BOOSTING_VARIABLE_CURRENT  = 2,
	E_BOOSTING_NUM,
};

struct drm_st_ld_panel_info {

	__u16 u16PanelWidth;
	__u16 u16PanelHeight;
	enum drm_en_ld_type eLEDType;
	__u8 u8LDFWidth;
	__u8 u8LDFHeight;
	__u8 u8LEDWidth;
	__u8 u8LEDHeight;
	__u8 u8LSFWidth;
	__u8 u8LSFHeight;
	__u8 u8Edge2D_Local_h;
	__u8 u8Edge2D_Local_v;
	__u8 u8PanelHz;

};

struct drm_st_ld_misc_info {

	__u32 u32MarqueeDelay;
	bool bLDEn;
	bool bOLEDEn;
	bool bOLED_LSP_En;
	bool bOLED_GSP_En;
	bool bOLED_HBP_En;
	bool bOLED_CRP_En;
	bool bOLED_LSPHDR_En;
	__u8 u8SPIBits;
	__u8 u8ClkHz;
	__u8 u8MirrorPanel;

};

struct drm_st_ld_mspi_info {

	__u8 u8MspiChanel;
	__u8 u8MspiMode;
	__u8 u8TrStart;
	__u8 u8TrEnd;
	__u8 u8TB;
	__u8 u8TRW;
	__u32 u32MspiClk;
	__u16 BClkPolarity;
	__u16 BClkPhase;
	__u32 u32MAXClk;
	__u8 u8MspiBuffSizes;
	__u8 u8WBitConfig[8];
	__u8 u8RBitConfig[8];

};

struct drm_st_ld_dma_info {

	__u8 u8LDMAchanel;
	__u8 u8LDMATrimode;
	__u8 u8LDMACheckSumMode;
	__u16 u16MspiHead[8];
	__u16 u16DMADelay[4];
	__u8 u8cmdlength;
	__u8 u8BLWidth;
	__u8 u8BLHeight;
	__u16 u16LedNum;
	__u8 u8DataPackMode;
	__u16 u16DMAPackLength;
	__u8 u8DataInvert;
	__u32 u32DMABaseOffset;
	__u8 u8ExtData[16];
	__u8 u8ExtDataLength;
	__u16 u16BDACNum;
	__u8 u8BDACcmdlength;
	__u16 u16BDACHead[8];

};

struct drm_st_ld_led_device_info {

	__u16 u16Rsense;
	__u16 u16VDAC_MBR_OFF;
	__u16 u16VDAC_MBR_NO;
	__u16 u16BDAC_High;
	__u16 u16BDAC_Low;
	__u16 u16SPINUM;
	bool bAS3824;
	bool bNT50585;
	bool bIW7039;
	bool bMCUswmode;
	__u8 u8SPIDutybit;

};

struct mtk_ld_context {

	__u32 u8LDMSupport;
	__u64 u64LDM_phyAddress;
	__u8 u8LDM_Version;
	struct drm_st_ld_panel_info ld_panel_info;
	struct drm_st_ld_misc_info ld_misc_info;
	struct drm_st_ld_mspi_info ld_mspi_info;
	struct drm_st_ld_dma_info  ld_dma_info;
	struct drm_st_ld_led_device_info ld_led_device_info;
};

int mtk_ld_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val,
	int prop_index);
int mtk_ld_atomic_get_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val,
	int prop_index);

int readDTB2LDMPrivate(struct mtk_ld_context *pctx);

int mtk_ldm_init(struct device *dev);

//Panel ini related
#define LDM_COMPATIBLE_STR        "mediatek,mediatek-ldm"
#define LDM_PANEL_INFO_NODE	          "ldm-panel-info"
#define LDM_MISC_INFO_NODE	          "ldm-misc-info"
#define LDM_MSPI_INFO_NODE	          "ldm-mspi-info"
#define LDM_DMA_INFO_NODE	          "ldm-dma-info"
#define LDM_LED_DEVICE_INFO_NODE      "ldm-led-device-info"
#define LDM_CUS_SETTING_NODE      "ldm-cus-setting"

/*
 *###########################
 *#       ldm-panel-info    #
 *###########################
 */
#define PANEL_WIDTH_TAG          "PanelWidth"
#define PANEL_HEIGHT_TAG         "PanelHeight"
#define LED_TYPE_TAG             "LEDType"
#define LDF_WIDTH_TAG            "LDFWidth"
#define LDF_HEIGHT_TAG           "LDFHeight"
#define LED_WIDTH_TAG            "LEDWidth"
#define LED_HEIGHT_TAG           "LEDHeight"
#define LSF_WIDTH_TAG            "LSFWidth"
#define LSF_HEIGHT_TAG           "LSFHeight"
#define EDGE2D_LOCAL_H_TAG      "Edge2D_Local_h"
#define EDGE2D_LOCAL_V_TAG      "Edge2D_Local_v"
#define PANEL_TAG               "PanelHz"

/*
 *###########################
 *#       ldm-misc-info    #
 *###########################
 */

#define MARQUEE_DELAY_TAG    "MarqueeDelay"
#define LDEN_TAG            "LDEn"
#define OLED_EN_TAG         "OLEDEn"
#define OLED_LSP_EN_TAG     "OLED_LSP_En"
#define OLED_GSP_EN_TAG     "OLED_GSP_En"
#define OLED_HBP_EN_TAG     "OLED_HBP_En"
#define OLED_CRP_EN_TAG     "OLED_CRP_En"
#define OLED_LSPHDR_EN_TAG  "OLED_LSPHDR_En"
#define SPIBITS_TAG         "SPIBits"
#define CLKHZ_TAG           "ClkHz"
#define MIRROR_PANEL_TAG    "MirrorPanel"


/*
 *###########################
 *#       ldm-mspi-info    #
 *###########################
 */
#define MSPI_CHANEL_TAG     "MspiChanel"
#define MSPI_MODE_TAG       "MspiMode"
#define TR_START_TAG        "TrStart"
#define TR_END_TAG          "TrEnd"
#define TB_TAG              "TB"
#define TRW_TAG             "TRW"
#define MSPI_CLK_TAG        "MspiClk"
#define BCLKPOLARITY_TAG    "BClkPolarity"
#define BCLKPHASE_TAG       "BClkPhase"
#define MAX_CLK_TAG         "MAXClk"
#define MSPI_BUFFSIZES_TAG  "MspiBuffSizes"
#define W_BIT_CONFIG_TAG    "WBitConfig"
#define R_BIT_CONFIG_TAG    "RBitConfig"

#define W_BIT_CONFIG_NUM       8
#define R_BIT_CONFIG_NUM       8

/*
 *###########################
 *#       ldm-dma-info    #
 *###########################
 */
#define LDMA_CHANEL_TAG         "LDMAchanel"
#define LDMA_TRIMODE_TAG        "LDMATrimode"
#define LDMA_CHECK_SUM_MODE_TAG "LDMACheckSumMode"
#define MSPI_HEAD_TAG           "MspiHead"
#define DMA_DELAY_TAG           "DMADelay"
#define CMD_LENGTH_TAG          "cmdlength"
#define BL_WIDTH_TAG            "BLWidth"
#define BL_HEIGHT_TAG           "BLHeight"
#define LED_NUM_TAG             "LedNum"
#define DATA_PACK_MODE_TAG      "DataPackMode"
#define DMA_PACK_LENGTH_TAG     "DMAPackLength"
#define DATA_INVERT_TAG         "DataInvert"
#define DMA_BASE_OFFSET_TAG     "DMABaseOffset"
#define EXT_DATA_TAG            "ExtData"
#define EXT_DATA_LENGTH_TAG     "ExtDataLength"
#define BDAC_NUM_TAG            "BDACNum"
#define BDAC_CMD_LENGTH_TAG     "BDACcmdlength"
#define BDAC_HEAD_TAG           "BDACHead"

#define MSPI_HEAD_NUM       8
#define DMA_DELAY_NUM       4
#define EXT_DATA_NUM        16
#define BDAC_HEAD_NUM       8

/*
 *################################
 *#       ldm-led-device-info    #
 *################################
 */

#define RSENSE_TAG          "Rsense"
#define VDAC_MBR_OFF_TAG    "VDAC_MBR_OFF"
#define VDAC_MBR_ON_TAG     "VDAC_MBR_NO"
#define BDAC_HIGH_TAG       "BDAC_High"
#define BDAC_LOW_TAG        "BDAC_Low"
#define SPI_NUM_TAG         "SPINUM"
#define AS3824_TAG          "AS3824"
#define NT50585_TAG         "NT50585"
#define IW_7039_TAG         "IW7039"
#define MCU_SW_MODE_TAG     "MCUswmode"
#define SPI_DUTY_BIT_TAG    "SPIDutybit"

/*
 *################################
 *#       ldm-cus-setting        #
 *################################
 */

#endif
