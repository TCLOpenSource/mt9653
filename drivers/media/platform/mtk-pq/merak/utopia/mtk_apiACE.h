/* SPDX-License-Identifier: GPL-2.0 */
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020-2024 MediaTek Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MediaTek Inc. and be kept in strict confidence
// ("MediaTek Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MediaTek Confidential
// Information is unlawful and strictly prohibited. MediaTek hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _MTK_APIACE_H_
#define _MTK_APIACE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "MsTypes.h"
#include "apiXC.h"
//-----------------------------------------------------------------------------
//  Type and Structure
//-----------------------------------------------------------------------------
struct XC_ACE_DeviceId {
	__u32 u32Version;
	__u32 u32Id;
};

//------------------------------
// Weave Type: used for 3D
//------------------------------
enum XC_ACE_EX_WEAVETYPE {
	E_ACE_EX_WEAVETYPE_NONE = 0x00,
	E_ACE_EX_WEAVETYPE_H = 0x01,
	E_ACE_EX_WEAVETYPE_V = 0x02,
	E_ACE_EX_WEAVETYPE_DUALVIEW = 0x04,
	E_ACE_EX_WEAVETYPE_NUM,
};

struct XC_ACE_EX_InitData {
	bool eWindow;
	__s16 *S16ColorCorrectionMatrix;
	__s16 *S16RGB;
	__u16 u16MWEHstart;
	__u16 u16MWEVstart;
	__u16 u16MWEWidth;
	__u16 u16MWEHeight;
	__u16 u16MWE_Disp_Hstart;
	__u16 u16MWE_Disp_Vstart;
	__u16 u16MWE_Disp_Width;
	__u16 u16MWE_Disp_Height;
	bool bMWE_Enable;
} __packed;

enum XC_ACE_IHC_COLOR_TYPE {
	// Users can define color, inorder to match previous settings,
	// R,G,B,C,M,Y,F can also mapping USER_COLOR0 ~ 6

	E_ACE_IHC_COLOR_R,
	E_ACE_IHC_COLOR_G,
	E_ACE_IHC_COLOR_B,
	E_ACE_IHC_COLOR_C,
	E_ACE_IHC_COLOR_M,
	E_ACE_IHC_COLOR_Y,
	E_ACE_IHC_COLOR_F,

	E_ACE_IHC_USER_COLOR1 = E_ACE_IHC_COLOR_R,
	E_ACE_IHC_USER_COLOR2 = E_ACE_IHC_COLOR_G,
	E_ACE_IHC_USER_COLOR3 = E_ACE_IHC_COLOR_B,
	E_ACE_IHC_USER_COLOR4 = E_ACE_IHC_COLOR_C,
	E_ACE_IHC_USER_COLOR5 = E_ACE_IHC_COLOR_M,
	E_ACE_IHC_USER_COLOR6 = E_ACE_IHC_COLOR_Y,
	E_ACE_IHC_USER_COLOR7 = E_ACE_IHC_COLOR_F,
	E_ACE_IHC_USER_COLOR8,
	E_ACE_IHC_USER_COLOR9,
	E_ACE_IHC_USER_COLOR10,
	E_ACE_IHC_USER_COLOR11,
	E_ACE_IHC_USER_COLOR12,
	E_ACE_IHC_USER_COLOR13,
	E_ACE_IHC_USER_COLOR14,
	E_ACE_IHC_USER_COLOR15,
	E_ACE_IHC_USER_COLOR0,
	E_ACE_IHC_COLOR_MAX,
};

enum XC_ACE_ICC_COLOR_TYPE {
	// Users can define color, inorder to match previous settings,
	// R,G,B,C,M,Y,F can also mapping USER_COLOR0 ~ 6

	E_ACE_ICC_COLOR_R,
	E_ACE_ICC_COLOR_G,
	E_ACE_ICC_COLOR_B,
	E_ACE_ICC_COLOR_C,
	E_ACE_ICC_COLOR_M,
	E_ACE_ICC_COLOR_Y,
	E_ACE_ICC_COLOR_F,

	E_ACE_ICC_USER_COLOR1 = E_ACE_ICC_COLOR_R,
	E_ACE_ICC_USER_COLOR2 = E_ACE_ICC_COLOR_G,
	E_ACE_ICC_USER_COLOR3 = E_ACE_ICC_COLOR_B,
	E_ACE_ICC_USER_COLOR4 = E_ACE_ICC_COLOR_C,
	E_ACE_ICC_USER_COLOR5 = E_ACE_ICC_COLOR_M,
	E_ACE_ICC_USER_COLOR6 = E_ACE_ICC_COLOR_Y,
	E_ACE_ICC_USER_COLOR7 = E_ACE_ICC_COLOR_F,
	E_ACE_ICC_USER_COLOR8,
	E_ACE_ICC_USER_COLOR9,
	E_ACE_ICC_USER_COLOR10,
	E_ACE_ICC_USER_COLOR11,
	E_ACE_ICC_USER_COLOR12,
	E_ACE_ICC_USER_COLOR13,
	E_ACE_ICC_USER_COLOR14,
	E_ACE_ICC_USER_COLOR15,
	E_ACE_ICC_USER_COLOR0,
	E_ACE_ICC_COLOR_MAX,
};

enum XC_ACE_IBC_COLOR_TYPE {
	// Users can define color, inorder to match previous settings,
	// R,G,B,C,M,Y,F can also mapping USER_COLOR0 ~ 6

	E_ACE_IBC_COLOR_R,
	E_ACE_IBC_COLOR_G,
	E_ACE_IBC_COLOR_B,
	E_ACE_IBC_COLOR_C,
	E_ACE_IBC_COLOR_M,
	E_ACE_IBC_COLOR_Y,
	E_ACE_IBC_COLOR_F,

	E_ACE_IBC_USER_COLOR1 = E_ACE_IBC_COLOR_R,
	E_ACE_IBC_USER_COLOR2 = E_ACE_IBC_COLOR_G,
	E_ACE_IBC_USER_COLOR3 = E_ACE_IBC_COLOR_B,
	E_ACE_IBC_USER_COLOR4 = E_ACE_IBC_COLOR_C,
	E_ACE_IBC_USER_COLOR5 = E_ACE_IBC_COLOR_M,
	E_ACE_IBC_USER_COLOR6 = E_ACE_IBC_COLOR_Y,
	E_ACE_IBC_USER_COLOR7 = E_ACE_IBC_COLOR_F,
	E_ACE_IBC_USER_COLOR8,
	E_ACE_IBC_USER_COLOR9,
	E_ACE_IBC_USER_COLOR10,
	E_ACE_IBC_USER_COLOR11,
	E_ACE_IBC_USER_COLOR12,
	E_ACE_IBC_USER_COLOR13,
	E_ACE_IBC_USER_COLOR14,
	E_ACE_IBC_USER_COLOR15,
	E_ACE_IBC_USER_COLOR0,
	E_ACE_IBC_COLOR_MAX,
};

enum E_ACE_HSY_FUNC {
	E_ACE_HSY_HUE_SETTING,
	E_ACE_HSY_SAT_SETTING,
	E_ACE_HSY_LUMA_SETTING,
	E_ACE_HSY_NUMS,
};

struct XC_ACE_HSY_SETTING_INFO {
	__u32 u32Version;	// Struct version
	__u16 u16Length;	// Sturct length
	enum E_ACE_HSY_FUNC enHSYFunc;	// HSY setting
	__s32 *ps32Data;	//degrees or gain value
	__u8 u8DataLength;	//The data length of Setting
	__u8 u8Win;	// 0:main window 1:sub window
} __packed;

struct XC_ACE_HSY_RANGE_INFO {
	__u32 u32Version;	// Struct version
	__u16 u16Length;	// Sturct length

	enum E_ACE_HSY_FUNC enHSYFunc;	// HSY setting
	__s32 *ps32MinValue;
	__s32 *ps32MaxValue;
	__u8 *pu8NumofRegion;	// The number of hue region
	__u8 u8Win;	// 0:main window 1:sub window
} __packed;

/// set manual luma curve version of current XC lib
/// version 2 : add u16ManualBlendFactor
#define ST_ACE_SET_MANUAL_LUMA_CURVE_VERSION (2)
struct ST_ACE_SET_MANUAL_LUMA_CURVE {
	__u32 u32Version;
	__u32 u32Length;
	bool bEnable;	//enable manual mode
	__u16 u16PairNum;	//For Manual Mode: 18to256 curve
	__u32 *pu32IndexLut;	//For Manual Mode: 18to256 curve
	__u32 *pu32OutputLut;	//For Manual Mode: 18to256 curve
	__u16 u16ManualBlendFactor;	// For manual curve blends
} __packed;

#define ST_ACE_SET_STRETCH_SETTINGS_VERSION (1)
struct ST_ACE_SET_STRETCH_SETTINGS {
	__u32 u32Version;	//<Version of current structure.
	__u32 u32Length;	//<Length of this structure

	__u8 u8Win;
	__u8 u8_u0p8BlackStretchRatio;
	__u8 u8_u0p8WhiteStretchRatio;
	__u16 u16_u4p12BlackStretchGain;
	__u16 u16_u4p12WhiteStretchGain;
} __packed;

#define ST_ACE_SEND_PQ_CMD_VERSION                      (1)
struct ST_XC_ACE_PQ_CMD {
	__u32 u32Version;	//<Version of current structure.
	__u16 u16Length;	//<Length of this structure,
	__u32 u32CmdID;	//
	__u16 u16Data;	//
} __packed;


//The color temp settings ex2.
struct XC_ACE_EX_color_temp_ex2 {
	/// red color offset
	__u16 cRedOffset;
	/// green color offset
	__u16 cGreenOffset;
	/// blue color offset
	__u16 cBlueOffset;

	/// red color
	__u16 cRedColor;
	/// green color
	__u16 cGreenColor;
	/// blue color
	__u16 cBlueColor;

	/// scale 100 value of red color
	__u16 cRedScaleValue;
	/// scale 100 value of green color
	__u16 cGreenScaleValue;
	/// scale 100 value of blue color
	__u16 cBlueScaleValue;
} __packed;

//The Effect Settings of Multi Window
enum E_XC_ACE_EX_MWE_FUNC {
	/// off
	E_XC_ACE_EX_MWE_MODE_OFF,
	/// H split, reference window at right side,default
	E_XC_ACE_EX_MWE_MODE_H_SPLIT,
	/// Move
	E_XC_ACE_EX_MWE_MODE_MOVE,
	/// Zoom
	E_XC_ACE_EX_MWE_MODE_ZOOM,
	/// H Scan
	E_XC_ACE_EX_MWE_MODE_H_SCAN,
	/// H split, reference window at left side
	E_XC_ACE_EX_MWE_MODE_H_SPLIT_LEFT,
	/// The number of Scaler ACE MWE functoin
	E_XC_ACE_EX_MWE_MODE_NUMS,
};

enum EN_ACE_CHROMA_INFO_CONTROL {
	E_ACE_CHROMA_INFO_CONTROL_SET_WINDOW_INFO = 0,
	E_ACE_CHROMA_INFO_CONTROL_GET_HIST,
	E_ACE_CHROMA_INFO_CONTROL_NUM,
};

#define ST_ACE_GET_CHROMA_INFO_VERSION (1)
#define HISTOGRAM_SATOFHUE_WIN_NUM (6)
struct ST_ACE_GET_CHROMA_INFO {
	__u32 u32Version;	//<Version of current structure.
	__u32 u32Length;	//<Length of this structure
	__u8 u8Win;
	enum EN_ACE_CHROMA_INFO_CONTROL enChormaInfoCtl;
	__u8 u8SatbyHue_HueStart_Window0;
	__u8 u8SatbyHue_HueRange_Window0;
	__u8 u8SatbyHue_HueStart_Window1;
	__u8 u8SatbyHue_HueRange_Window1;
	__u8 u8SatbyHue_HueStart_Window2;
	__u8 u8SatbyHue_HueRange_Window2;
	__u8 u8SatbyHue_HueStart_Window3;
	__u8 u8SatbyHue_HueRange_Window3;
	__u8 u8SatbyHue_HueStart_Window4;
	__u8 u8SatbyHue_HueRange_Window4;
	__u8 u8SatbyHue_HueStart_Window5;
	__u8 u8SatbyHue_HueRange_Window5;
	bool bHistValid;
	__u32 *apu32SatOfHueHist[HISTOGRAM_SATOFHUE_WIN_NUM];
	__u8 au8SatOfHueHistSize[HISTOGRAM_SATOFHUE_WIN_NUM];
} __packed;

#define ST_ACE_GET_LUMA_INFO_VERSION (1)
struct ST_ACE_GET_LUMA_INFO {
	__u32 u32Version;	//<Version of current structure.
	__u32 u32Length;	//<Length of this structure

	__u8 u8Win;
	__u8 u8IpMaximumPixel;
	__u8 u8IpMinimumPixel;
	__s32 s32Yavg;
} __packed;

//The Result of XC_ACE function call.
enum E_XC_ACE_RESULT {
	/// fail
	E_XC_ACE_FAIL = 0,
	/// ok
	E_XC_ACE_OK = 1,
	/// get base address failed when initialize panel driver
	E_XC_ACE_GET_BASEADDR_FAIL,
	/// obtain mutex timeout when calling this function
	E_XC_ACE_OBTAIN_RESOURCE_FAIL,
};

enum E_XC_ACE_EX_RESULT {
	/// fail
	E_XC_ACE_EX_FAIL = 0,
	/// ok
	E_XC_ACE_EX_OK = 1,
	/// get base address failed when initialize panel driver
	E_XC_ACE_EX_GET_BASEADDR_FAIL,
	/// obtain mutex timeout when calling this function
	E_XC_ACE_EX_OBTAIN_MUTEX_FAIL,
};

//----------------------------
// XC DLC initialize
//----------------------------
//Initial Settings of MF Dyanmic Luma Curve
struct XC_DLC_MFinit {
	/// Default luma curve
	__u8 ucLumaCurve[16];
	/// Default luma curve 2a
	__u8 ucLumaCurve2_a[16];
	/// Default luma curve 2b
	__u8 ucLumaCurve2_b[16];
	/// Default luma curve 2
	__u8 ucLumaCurve2[16];

	/// default value: 10
	__u8 u8_L_L_U;
	/// default value: 10
	__u8 u8_L_L_D;
	/// default value: 10
	__u8 u8_L_H_U;
	/// default value: 10
	__u8 u8_L_H_D;
	/// default value: 128 (0x80)
	__u8 u8_S_L_U;
	/// default value: 128 (0x80)
	__u8 u8_S_L_D;
	/// default value: 128 (0x80)
	__u8 u8_S_H_U;
	/// default value: 128 (0x80)
	__u8 u8_S_H_D;
	/// -31 ~ 31 (bit7 = minus, ex. 0x88 => -8)
	__u8 ucCGCCGain_offset;
	/// 0x00~0x6F
	__u8 ucCGCChroma_GainLimitH;
	/// 0x00~0x10
	__u8 ucCGCChroma_GainLimitL;
	/// 0x01~0x20
	__u8 ucCGCYCslope;
	/// 0x01
	__u8 ucCGCYth;
	/// Compare difference of max and min bright
	__u8 ucDlcPureImageMode;
	/// n = 0 ~ 4 => Limit n levels => ex. n=2, limit 2 level 0xF7, 0xE7
	__u8 ucDlcLevelLimit;
	/// n = 0 ~ 50, default value: 12
	__u8 ucDlcAvgDelta;
	/// n = 0 ~ 15 => 0: disable still curve, 1 ~ 15: enable still curve
	__u8 ucDlcAvgDeltaStill;
	/// min 17 ~ max 32
	__u8 ucDlcFastAlphaBlending;
	/// some event is triggered, DLC must do slowly
	__u8 ucDlcSlowEvent;
	/// for IsrApp.c
	__u8 ucDlcTimeOut;
	/// for force to do fast DLC in a while
	__u8 ucDlcFlickAlphaStart;
	/// default value: 128
	__u8 ucDlcYAvgThresholdH;
	/// default value: 0
	__u8 ucDlcYAvgThresholdL;
	/// n = 24 ~ 64, default value: 48
	__u8 ucDlcBLEPoint;
	/// n = 24 ~ 64, default value: 48
	__u8 ucDlcWLEPoint;
	/// 1: enable; 0: disable
	__u8 bCGCCGainCtrl:1;
	/// 1: enable; 0: disable
	__u8 bEnableBLE:1;
	/// 1: enable; 0: disable
	__u8 bEnableWLE:1;
};

struct XC_DLC_MFinit_Ex {
	__u32 u32DLC_MFinit_Ex_Version;
	/// Default luma curve
	__u8 ucLumaCurve[16];
	/// Default luma curve 2a
	__u8 ucLumaCurve2_a[16];
	/// Default luma curve 2b
	__u8 ucLumaCurve2_b[16];
	/// Default luma curve 2
	__u8 ucLumaCurve2[16];

	/// default value: 10
	__u8 u8_L_L_U;
	/// default value: 10
	__u8 u8_L_L_D;
	/// default value: 10
	__u8 u8_L_H_U;
	/// default value: 10
	__u8 u8_L_H_D;
	/// default value: 128 (0x80)
	__u8 u8_S_L_U;
	/// default value: 128 (0x80)
	__u8 u8_S_L_D;
	/// default value: 128 (0x80)
	__u8 u8_S_H_U;
	/// default value: 128 (0x80)
	__u8 u8_S_H_D;
	/// -31 ~ 31 (bit7 = minus, ex. 0x88 => -8)
	__u8 ucCGCCGain_offset;
	/// 0x00~0x6F
	__u8 ucCGCChroma_GainLimitH;
	/// 0x00~0x10
	__u8 ucCGCChroma_GainLimitL;
	/// 0x01~0x20
	__u8 ucCGCYCslope;
	/// 0x01
	__u8 ucCGCYth;
	/// Compare difference of max and min bright
	__u8 ucDlcPureImageMode;
	/// n = 0 ~ 4 => Limit n levels => ex. n=2, limit 2 level 0xF7, 0xE7
	__u8 ucDlcLevelLimit;
	/// n = 0 ~ 50, default value: 12
	__u8 ucDlcAvgDelta;
	/// n = 0 ~ 15 => 0: disable still curve, 1 ~ 15: enable still curve
	__u8 ucDlcAvgDeltaStill;
	/// min 17 ~ max 32
	__u8 ucDlcFastAlphaBlending;
	/// some event is triggered, DLC must do slowly
	__u8 ucDlcSlowEvent;
	/// for IsrApp.c
	__u8 ucDlcTimeOut;
	/// for force to do fast DLC in a while
	__u8 ucDlcFlickAlphaStart;
	/// default value: 128
	__u8 ucDlcYAvgThresholdH;
	/// default value: 0
	__u8 ucDlcYAvgThresholdL;
	/// n = 24 ~ 64, default value: 48
	__u8 ucDlcBLEPoint;
	/// n = 24 ~ 64, default value: 48
	__u8 ucDlcWLEPoint;
	/// 1: enable; 0: disable
	__u8 bCGCCGainCtrl:1;
	/// 1: enable; 0: disable
	__u8 bEnableBLE:1;
	/// 1: enable; 0: disable
	__u8 bEnableWLE:1;
	/// default value: 64
	__u8 ucDlcYAvgThresholdM;
	/// Compare difference of max and min bright
	__u8 ucDlcCurveMode;
	/// min 00 ~ max 128
	__u8 ucDlcCurveModeMixAlpha;
	///
	__u8 ucDlcAlgorithmMode;
	/// Dlc Histogram Limit Curve
	__u8 ucDlcHistogramLimitCurve
		[V4L2_DLC_HISTOGRAM_LIMIT_CURVE_ARRARY_NUM];
	///
	__u8 ucDlcSepPointH;
	///
	__u8 ucDlcSepPointL;
	///
	__u16 uwDlcBleStartPointTH;
	///
	__u16 uwDlcBleEndPointTH;
	///
	__u8 ucDlcCurveDiff_L_TH;
	///
	__u8 ucDlcCurveDiff_H_TH;
	///
	__u16 uwDlcBLESlopPoint_1;
	///
	__u16 uwDlcBLESlopPoint_2;
	///
	__u16 uwDlcBLESlopPoint_3;
	///
	__u16 uwDlcBLESlopPoint_4;
	///
	__u16 uwDlcBLESlopPoint_5;
	///
	__u16 uwDlcDark_BLE_Slop_Min;
	///
	__u8 ucDlcCurveDiffCoringTH;
	///
	__u8 ucDlcAlphaBlendingMin;
	///
	__u8 ucDlcAlphaBlendingMax;
	///
	__u8 ucDlcFlicker_alpha;
	///
	__u8 ucDlcYAVG_L_TH;
	///
	__u8 ucDlcYAVG_H_TH;
	///
	__u8 ucDlcDiffBase_L;
	///
	__u8 ucDlcDiffBase_M;
	///
	__u8 ucDlcDiffBase_H;
#if defined(UFO_PUBLIC_HEADER_700)
	__u8 u8LMaxThreshold;
	__u8 u8LMinThreshold;
	__u8 u8LMaxCorrection;
	__u8 u8LMinCorrection;
	__u8 u8RMaxThreshold;
	__u8 u8RMinThreshold;
	__u8 u8RMaxCorrection;
	__u8 u8RMinCorrection;
	__u8 u8AllowLoseContrast;
#endif
};

//Initial Settings of Dynamic Backlight Control
struct XC_DLC_DBC_MFinit {
	/// Max PWM
	__u8 ucMaxPWM;
	/// Min PWM
	__u8 ucMinPWM;
	/// Max Video
	__u8 ucMax_Video;
	/// Mid Video
	__u8 ucMid_Video;
	/// Min Video
	__u8 ucMin_Video;
	/// Current PWM
	__u8 ucCurrentPWM;
	/// Alpha
	__u8 ucAlpha;
	/// Backlight thres
	__u8 ucBackLight_Thres;
	/// Avg delta
	__u8 ucAvgDelta;
	/// Flick alpha
	__u8 ucFlickAlpha;
	/// Fast alpha blending, min 17 ~ max 32
	__u8 ucFastAlphaBlending;
	// TBD
	__u8 ucLoop_Dly;
	// TBD
	__u8 ucLoop_Dly_H_Init;
	// TBD
	__u8 ucLoop_Dly_MH_Init;
	// TBD
	__u8 ucLoop_Dly_ML_Init;
	// TBD
	__u8 ucLoop_Dly_L_Init;
	/// Y gain H
	__u8 ucY_Gain_H;
	/// C gain H
	__u8 ucC_Gain_H;
	/// Y gain M
	__u8 ucY_Gain_M;
	/// C gain M
	__u8 ucC_Gain_M;
	/// Y gain L
	__u8 ucY_Gain_L;
	/// C gain L
	__u8 ucC_Gain_L;
	/// 1: enable; 0: disable
	__u8 bYGainCtrl:1;
	/// 1: enable; 0: disable
	__u8 bCGainCtrl:1;
};

//Initial Settings of Dyanmic Luma Curve
#define ENABLE_10_BIT_DLC   0
struct XC_DLC_init {
	/// Scaler DCL MF init
	struct XC_DLC_MFinit DLC_MFinit;
	/// Scaler DCL MF init Ex
	struct XC_DLC_MFinit_Ex DLC_MFinit_Ex;
	/// Curve Horizontal start
	__u16 u16CurveHStart;
	/// Curve Horizontal end
	__u16 u16CurveHEnd;
	/// Curve Vertical start
	__u16 u16CurveVStart;
	/// Curve Vertical end
	__u16 u16CurveVEnd;
	/// Scaler DLC MF init
	struct XC_DLC_DBC_MFinit DLC_DBC_MFinit;
#if (ENABLE_10_BIT_DLC)
	/// DLC init ext
	bool b10BitsEn;
#endif

#if defined(UFO_PUBLIC_HEADER_700)
#if defined(UFO_XC_HDR_VERSION) && (UFO_XC_HDR_VERSION == 2)
	__u8 u8DlcMode;
	__u8 u8TmoMode;
#endif
#endif
};

//DLC capture Range
struct XC_DLC_CAPTURE_Range {
	__u16 wHStart;
	__u16 wHEnd;
	__u16 wVStart;
	__u16 wVEnd;
} __packed;

enum E_XC_DLC_MEMORY_REQUEST_TYPE {
	E_DLC_MEMORY_TMO_VR_REQUEST,	///< memory request for TMO VR
	E_DLC_MEMORY_REQUEST_MAX,	///< Invalid request
};


struct ST_XC_DLC_SET_MEMORY_INFO {
	enum E_XC_DLC_MEMORY_REQUEST_TYPE eType;
	MS_PHY phyAddress;
	__u32 u32Size;
	__u32 u32HeapID;
} __packed;

//-----------------------------------------------------------------------------
//  Function and Variable
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// 3D clone main and sub window's PQmap
/// @param  enWeaveType             \b IN: Output WeaveType
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_3DClonePQMap(
	struct XC_ACE_DeviceId *pDeviceId,
	enum XC_ACE_EX_WEAVETYPE enWeaveType);

//-----------------------------------------------------------------------------
/// ACE Exit
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_EX_Exit(
	struct XC_ACE_DeviceId *pDeviceId);

extern void MApi_XC_ACE_EX_SetRBChannelRange(
	struct XC_ACE_DeviceId *pDeviceId,
	bool bScalerWin, bool bRange);

//-----------------------------------------------------------------------------
/// Picture set contrast
/// @param  eWindow                       \b IN: Indicates the window
/// @param  bUseYUVSpace                  \b IN: Use YUV format or RGB format.
/// @param  u8Contrast                    \b IN: Contrase value given by user.
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_PicSetContrast(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow, bool bUseYUVSpace,
	__u8 u8Contrast);

//-----------------------------------------------------------------------------
/// Picture set brightness
/// @param  u8Brightness_R                    \b IN: R value given by user
/// @param  u8Brightness_G                    \b IN: G value given by user
/// @param  u8Brightness_B                    \b IN: B value given by user
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_PicSetBrightness(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow,
	__u8 u8Brightness_R,
	__u8 u8Brightness_G,
	__u8 u8Brightness_B);

//-----------------------------------------------------------------------------
/// Picture set Hue
/// @param  eWindow                  \b IN: Indicates the window
/// @param  bUseYUVSpace             \b IN: Use YUV format or RGB format.
/// @param  u8Hue                    \b IN: Hue value given by user
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_PicSetHue(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow,
	bool bUseYUVSpace,
	__u8 u8Hue);

//-----------------------------------------------------------------------------
/// Picture set Saturation
/// @param  eWindow                  \b IN: Indicates the window
/// @param  bUseYUVSpace             \b IN: Use YUV format or RGB format.
/// @param  u8Saturation             \b IN: Saturation value given by user.
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_PicSetSaturation(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow,
	bool bUseYUVSpace,
	__u8 u8Saturation);

//-----------------------------------------------------------------------------
///-Adjust sharpness
/// @param  eWindow                  \b IN: Indicates the window
/// @param u8Sharpness                \b IN: sharpness value
///- 0 -0x3f
//-----------------------------------------------------------------------------
extern void  MApi_XC_ACE_EX_PicSetSharpness(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow, __u8 u8Sharpness);

//-----------------------------------------------------------------------------
/// Set debug level (without Mutex protect)
/// @ingroup ACE_INFO
/// @param  u16DbgSwitch                 \b IN: debug switch value
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_SetDbgLevel(__u16 u16DbgSwitch);

//-----------------------------------------------------------------------------
/// @param  pstXC_ACE_InitData           \b IN: @ref struct XC_ACE_EX_InitData
/// @param  u32InitDataLen              \b IN: The Length of pstXC_ACE_InitData
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_EX_Init(
	struct XC_ACE_DeviceId *pDeviceId,
	struct XC_ACE_EX_InitData *pstXC_ACE_InitData,
	__u32 u32InitDataLen);

//-----------------------------------------------------------------------------
/// Set color matrix
/// @ingroup ACE_FEATURE
/// @param  eWindow              \b IN: Indicates the window
/// @param  pu16Matrix           \b IN: Matrix gto set the color transformation
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_SetColorMatrix(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow,
	__u16 *pu16Matrix);

//-----------------------------------------------------------------------------
/// Set Bypass Color Matrix
/// @ingroup ACE_FEATURE
/// @param bEnable      \b IN:  Enable : Bypass Set Color Matrix
/// @return @ref E_XC_ACE_EX_RESULT
//-----------------------------------------------------------------------------
extern enum E_XC_ACE_EX_RESULT
	MApi_XC_ACE_EX_SetBypassColorMatrix(
	struct XC_ACE_DeviceId *pDeviceId,
	bool bEnable);

//-----------------------------------------------------------------------------
/// Select YUV to RGB Matrix, if the ucMatrix type is ACE_YUV_TO_RGB_MATRIX_USER
/// , then apply the psUserYUVtoRGBMatrix supplied by user.
// @param  eWindow                 \b IN: Indicates the window
// @param  ucMatrix                \b IN: @ref E_ACE_YUVTORGBInfoType.
// @param  psUserYUVtoRGBMatrix    \b IN: User-Defined color transformed matrix.
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_SelectYUVtoRGBMatrix(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow, __u8 ucMatrix,
	__s16 *psUserYUVtoRGBMatrix);

//-----------------------------------------------------------------------------
/// Set color correction table
/// @param  bScalerWin       \b IN: Indicates the window where the ACE function
///                          applies to, 0: Main Window, 1: Sub Window.
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_SetColorCorrectionTable(
	struct XC_ACE_DeviceId *pDeviceId,
	bool bScalerWin);

//-----------------------------------------------------------------------------
/// Set Color correction table
/// @ingroup ACE_FEATURE
/// @param  u16DbgSwitch                 \b IN: debug switch value,
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_ColorCorrectionTable(
	struct XC_ACE_DeviceId *pDeviceId,
	bool bScalerWin,
	__s16 *psColorCorrectionTable);

//-----------------------------------------------------------------------------
/// Set IHC value
/// @ingroup ACE_FEATURE
/// @param  bScalerWin               \b IN: Indicates the window
/// @param  eIHC                     \b IN: Indicates the color to be set.
/// @param  u8Val                    \b IN: The value set to the color(0 ~ 127)
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_SetIHC(bool bScalerWin,
				       enum XC_ACE_IHC_COLOR_TYPE eIHC,
				       __u8 u8Val);

//-----------------------------------------------------------------------------
/// Set ICC value
/// @ingroup ACE_FEATURE
/// @param  bScalerWin                \b IN: Indicates the window
/// @param  eICC                      \b IN: Indicates the color to be set.
/// @param  u8Val                     \b IN: The value set to the color(0 ~ 31)
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_SetICC(bool bScalerWin,
				       enum XC_ACE_ICC_COLOR_TYPE eICC,
				       __u8 u8Val);

//-----------------------------------------------------------------------------
/// Set IBC value
/// @ingroup ACE_FEATURE
/// @param  bScalerWin                \b IN: Indicates the window
/// @param  eIBC                      \b IN: Indicates the color to be set.
/// @param  u8Val                     \b IN: The value set to the color(0 ~ 63)
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_SetIBC(bool bScalerWin,
				       enum XC_ACE_IBC_COLOR_TYPE eIBC,
				       __u8 u8Val);

//-----------------------------------------------------------------------------
/// Set HSY Setting
/// @ingroup ACE_FEATURE
/// @param  pHSY_INFO               \b IN: HSY information.
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_Set_HSYSetting(
	struct XC_ACE_HSY_SETTING_INFO *pHSY_INFO);

//-----------------------------------------------------------------------------
/// Set Manual Luma Curve
/// @ingroup ACE_FEATURE
/// @param  pstPQSetManualLumaCurve               \b IN: Lut information.
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_SetManualLumaCurve(
	struct ST_ACE_SET_MANUAL_LUMA_CURVE *pstPQSetManualLumaCurve);

//-----------------------------------------------------------------------------
/// Set Stretch Settings
/// @ingroup ACE_FEATURE
/// @param  pstPQSetStretchSettings               \b IN: Lut information.
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_SetStretchSettings(
	struct ST_ACE_SET_STRETCH_SETTINGS *pstPQSetStretchSettings);

//-----------------------------------------------------------------------------
/// Send PQ cmd
/// @ingroup ACE_FEATURE
/// @param  pstPQCmd            \b IN: the pointer to the struct of PQ cmd data
/// @return @ref E_XC_ACE_RESULT
//-----------------------------------------------------------------------------
extern enum E_XC_ACE_RESULT MApi_XC_ACE_SendPQCmd(
	struct ST_XC_ACE_PQ_CMD *pstPQCmd);

//-----------------------------------------------------------------------------
/// Picture set post color temp
/// V02. Change the fields in XC_ACE_EX_color_temp_ex structure to __u16
/// @param  eWindow                  \b IN: Indicates the window
/// @param  pstColorTemp             \b IN: Color temp info given by user.
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_PicSetPostColorTemp_V02(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow,
	struct XC_ACE_EX_color_temp_ex2 *pstColorTemp);

//-----------------------------------------------------------------------------
/// MWE function selection
/// @param  eWindow                 \b IN: Indicates the window
/// @param  mwe_func                     \b IN: @ref E_XC_ACE_EX_MWE_FUNC
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EX_MWEFuncSel(
	struct XC_ACE_DeviceId *pDeviceId,
	bool eWindow,
	enum E_XC_ACE_EX_MWE_FUNC mwe_func);

//-----------------------------------------------------------------------------
/// Enable MWE
/// @ingroup ACE_FEATURE
/// @param  bEnable                     \b IN: @ref bool
//-----------------------------------------------------------------------------
extern void MApi_XC_ACE_EnableMWE(bool bEnable);

//-----------------------------------------------------------------------------
/// Get Chroma Info
/// @ingroup ACE_FEATURE
/// @param  pstPQGetChromaInfo               \b IN: Lut information.
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_GetChromaInfo(
	struct ST_ACE_GET_CHROMA_INFO *pstPQGetChromaInfo);

//-----------------------------------------------------------------------------
/// Read PQ cmd value
/// @ingroup ACE_FEATURE
/// @param  pstPQCmd            \b IN: the pointer to the struct of PQ cmd data
/// @return @ref E_XC_ACE_RESULT
//-----------------------------------------------------------------------------
extern enum E_XC_ACE_RESULT MApi_XC_ACE_ReadPQCmdValue(
	struct ST_XC_ACE_PQ_CMD *pstPQCmd);

//-----------------------------------------------------------------------------
/// Get HSY Setting
/// @ingroup ACE_FEATURE
/// @param  pHSY_INFO               \b IN: HSY information.
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_Get_HSYSetting(
	struct XC_ACE_HSY_SETTING_INFO *pHSY_INFO);

//-----------------------------------------------------------------------------
/// Set HSY Adjust Range
/// @ingroup ACE_FEATURE
/// @param  pHSY_INFO               \b IN: HSY Range information.
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_Get_HSYAdjustRange(
	struct XC_ACE_HSY_RANGE_INFO *pHSY_Range);

//-----------------------------------------------------------------------------
/// Set Manual Luma Curve
/// @ingroup ACE_FEATURE
/// @param  pstPQGetLumaInfo               \b IN: Lut information.
/// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_ACE_GetLumaInfo(
	struct ST_ACE_GET_LUMA_INFO *pstPQGetLumaInfo);

//-----------------------------------------------------------------------------
/// DLC initiation
/// @ingroup DLC_INIT
// @param  pstXC_DLC_InitData             \b IN: TBD
// @param  u32InitDataLen                 \b IN: TBD
// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_DLC_Init_Ex(
	struct XC_DLC_init *pstXC_DLC_InitData,
	__u32 u32InitDataLen);

//-----------------------------------------------------------------------------
/// Setup DLC capture range
/// @ingroup DLC_FEATURE
/// @param StuDlc_Range                     \b IN: Hstart, Hend, Vstart, Vend
//-----------------------------------------------------------------------------
extern void MApi_XC_DLC_SetCaptureRange(
	struct XC_DLC_CAPTURE_Range *pu16_Range);

//-----------------------------------------------------------------------------
/// Dlc Set Memory Address
/// @ingroup DLC_FEATURE
/// @param  pstDLC_Set_Memory_Address      \b IN: Memory Address Data
// @return @ref bool
//-----------------------------------------------------------------------------
extern bool MApi_XC_DLC_SetMemoryAddress(
	struct ST_XC_DLC_SET_MEMORY_INFO *pstDLC_Set_Memory_Address);

//-----------------------------------------------------------------------------
/// Description : Init the PNL IOMAP base
/// @ingroup PNL_INTERFACE_INIT
/// @return @ref bool
//-----------------------------------------------------------------------------
extern SYMBOL_WEAK bool MApi_PNL_IOMapBaseInit(void);


#ifdef __cplusplus
}
#endif

#endif				// _MTK_APIACE_H_

