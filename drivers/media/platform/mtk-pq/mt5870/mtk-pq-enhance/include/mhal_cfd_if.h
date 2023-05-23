/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MHAL_CFD_IF_H_
#define _MHAL_CFD_IF_H_

#ifdef _MHAL_CFD_IF_C_
#ifndef INTERFACE
#define INTERFACE
#endif
#else
#ifndef INTERFACE
#define INTERFACE extern
#endif
#endif

#include <linux/types.h>

// Histogram Mode
enum EN_CFD_YSTATS_HIST_SEL {
	E_CFD_YSTATS_HIST_SEL_Y = 0x0,
	// Luma of YCbCr
	E_CFD_YSTATS_HIST_SEL_MAXRGB = 0x1,
	// max (R,G,B)
	E_CFD_YSTATS_HIST_SEL_L = 0x2,
	// Lightness for HSL
	E_CFD_YSTATS_HIST_SEL_MAX
};

//Matrix coefficient
//use order in HEVC spec
enum E_CFD_CFIO_MC_HW_TEMP {
	E_CFD_CFIO_MC_IDENTITY_HW                  = 0x0,
	//means RGB, but no specific colorspace
	E_CFD_CFIO_MC_BT709_XVYCC709_HW            = 0x1,
	E_CFD_CFIO_MC_UNSPECIFIED_HW               = 0x2,
	E_CFD_CFIO_MC_RESERVED_HW                  = 0x3,
	E_CFD_CFIO_MC_USFCCT47_HW                  = 0x4,
	E_CFD_CFIO_MC_BT601625_XVYCC601_SYCC_HW    = 0x5,
	E_CFD_CFIO_MC_BT601525_SMPTE170M_HW        = 0x6,
	E_CFD_CFIO_MC_SMPTE240M_HW                 = 0x7,
	E_CFD_CFIO_MC_YCGCO_HW                     = 0x8,
	E_CFD_CFIO_MC_BT2020NCL_HW                 = 0x9,
	E_CFD_CFIO_MC_BT2020CL_HW                  = 0xA,
	E_CFD_CFIO_MC_YDZDX_HW                     = 0xB,
	E_CFD_CFIO_MC_CD_NCL_HW                    = 0xC,
	E_CFD_CFIO_MC_CD_CL_HW                     = 0xD,
	E_CFD_CFIO_MC_ICTCP_HW                     = 0xE,
	E_CFD_CFIO_MC_RESERVED_START_HW
};

struct STU_CFDAPI_DLCIP {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_KANO_TMOIP_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure, u16Length=sizeof(STU_CFDAPI_Kano_TMOIP)

	__u8  u8DLC_curve_Mode;
	__u8  u8DLC_curve_enable_Mode;

	//__u8  u8UVC_enable_Mode;
	//__u8  u8UVC_setting_Mode;
};

struct STU_CFDAPI_TMOIP {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_KANO_TMOIP_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure, u16Length=sizeof(STU_CFDAPI_Kano_TMOIP)

	//B02
	__u8  u8HDR_TMO_curve_enable_Mode;
	__u8  u8HDR_TMO_curve_Mode;/// bit[0:5]=5 :HDR10plus
	__u8  u8HDR_TMO_curve_setting_Mode;
	//__u8  u8TMO_curve_Manual_Mode;

	//B04
	__u8  u8HDR_UVC_setting_Mode;


	//=====12P TMO setting=====

	//clip of TMO source
	//in nits
	__u16 u16Metadata;
	__u16 u16MetadataManualMode; // metadata for menual mode TMO

	//source 12 points input for TMO
	//in nits
	__u16 u16SrcNits12ptsInput[12];

	//target 12 points input for TMO
	//in nits
	__u16 u16TgtNits12ptsInput[12];

	// Auto mode max target nits source
	// 0: Max target nits from TMO MAP 12p setting.
	// 1: Max target nits from customer setting
	__u8 u8PanelMaxNitsEnable;
	__u16 u16PanelMaxNits; // Max target nits from customer setting

	//TMO process domain
	// 0: in Y domain, enum E_12P_TMO_VERSION_0
	// 1: in RGB domain, enum E_12P_TMO_VERSION_1
	// 2: in RGB domain linear domain, enum E_12P_TMO_VERSION_2
	// 5: HDR10+, enum E_12P_TMO_VERSION_5
	__u8  u8TmoVersion;

	//Y TMO blending alpha
	__u8 u8CurveBldAlpha;

	//=====param for HLG TMO=====
	//gamma , 0xffff = 1
	__u32 u32TmoHlgPow;

	//enable for HLG OOTF function
	bool bHlgOotfEn;

	//param for HLG OOTF
	//gamma , 0xffff = 1
	__u32 u32HlgSystemGamma;

	//0x1000 = 1
	__u16 u16HlgOotfAlpha;

	//0x1000 = 1
	__u16 u16HlgOotfBeta;

	//gamma , 0xffff = 1
	__u32 u32GammaAfterOotf;

	//New auto mode TMO
	__u16 u16max_target_src1000_ratio;
	__u16 u16Min_Ratio_slope_src1000;

	// Dynamic TMO for auto mode
	bool bDynamicTmoEn;
	__u32 u32HdrTotalPix;
	__u32 *pu32HdrHist;
	__u16 u16HdrHistBinNum;

	//TMO dark area tuning on OOTF part
	__u8 u8DarkAreaTuningEnable;
	__u8 u8Strength; // tuning strength. 0x00 ~ 0x10
	__u16 u16MinNits; // Starting nits for tuning
	__u16 u16MaxNits; // Ending nits for tuning

	// Individual blending weight
	__u8 u8IndividualBldModeEnable;

	// Color volume mapping
	__u8 u8ColorVolumeMappingEnable;
	__u8 u8CVMStrength; // For user adjust
	__u8 u8InitWeight;
	__u8 u8MinWeightAt1000;
	__u8 u8MinWeightAt4000;
	__u8 u8MaxWeightAt1000;
	__u8 u8MaxWeightAt4000;
	__u8 u8WeightAt10000nit;
	__u8 u8ReducedGain; // Reduced gain for fixed curve auto mode.
};

struct STU_CFDAPI_HDRIP {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_KANO_HDRIP_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure, u16Length=sizeof(STU_CFDAPI_Kano_HDRIP)

	// Color sampling
	__u8 u8HDR_ColorSampling_Mode;

	//TOP
	__u8 u8HDR_enable_Mode;

	//Composer
	__u8 u8HDR_Composer_Mode;

	//B01
	__u8 u8HDR_Module1_enable_Mode;

	//B01-02
	__u8 u8HDR_InRangeConvert_Mode;
	__u8 u8HDR_InputCSC_Mode;
	__u8 u8HDR_InputCSC_Ratio1;
	__u8 u8HDR_InputCSC_Manual_Vars_en;
	__u8 u8HDR_InputCSC_MC;

	__s32 s32HDR_InputCSC_ManualModeCscCoef[3][3];
	bool bHDR_InputCSC_ManualModeCscEnable;
	bool bHDR_InputCSC_ManualModeDitherEnable;
	bool bHDR_InputCSC_ManualModeROrCrSub16;
	bool bHDR_InputCSC_ManualModeGOrYSub16;
	bool bHDR_InputCSC_ManualModeBOrCbSub16;
	bool bHDR_InputCSC_ManualModeROrCrRangeSub128;
	bool bHDR_InputCSC_ManualModeGOrYRangeSub128;
	bool bHDR_InputCSC_ManualModeBOrCbRangeSub128;
	bool bHDR_InputCSC_ManualModeROrCrAdd16;
	bool bHDR_InputCSC_ManualModeGOrYAdd16;
	bool bHDR_InputCSC_ManualModeBOrCbAdd16;
	bool bHDR_InputCSC_ManualModeROrCrAdd128;
	bool bHDR_InputCSC_ManualModeBOrCbAdd128;

	//B01-03
	__u8 u8HDR_Degamma_SRAM_Mode;
	__u8 u8HDR_Degamma_Ratio1;//0x40 = 1 Q2.6
	__u16 u16HDR_Degamma_Ratio2;//0x40 = 1 Q2.6
	__u8 u8HDR_DeGamma_Manual_Vars_en;
	__u8 u8HDR_Degamma_TR;
	__u8 u8HDR_Degamma_Lut_En;
	__u32 *pu32HDR_Degamma_Lut_Address;
	__u8 u8HDR_Degamma_unequalIntervalIn;
#if !defined(__aarch64__)
	void *pDummy;
#endif
	__u16 u16HDR_Degamma_Lut_Length;
	__u8  u8DHDR_Degamma_Max_Lum_En;
	__u16 u16HDR_Degamma_Max_Lum;

	//B01-04
	__u8 u8HDR_3x3_Lut_En;
	__s32 s32HDR_3x3[3][3];
	__u8 u8HDR_3x3_Mode;
	__u16 u16HDR_3x3_Ratio2;//0x40 = 1 Q2.6
	__u8 u8HDR_3x3_Manual_Vars_en;
	__u8 u8HDR_3x3_InputCP;
	__u8 u8HDR_3x3_OutputCP;

	// OOTF
	__u8 u8HDR_OOTF_En;
	__u8 u8HDR_OOTF_SRAM_Mode;
	__u32 u32HDR_OOTF_SystemGamma;
	__u8 u8HDR_OOTF_Lut_En;
	__u32 *pu32HDR_OOTF_Lut_Address;
	__u16 u16HDR_OOTF_Lut_Length;
	__u8 u8HDR_OOTF_unequalIntervalIn;
	__u8 u8HDR_OOTF_blendingAlpha;

	// RGB3DLUT
	__u8 u8HDR_RGB3DLUT_En;
	__u8 u8HDR_RGB3DLUT_SRAM_Mode;
	__u16 *pu16HDR_RGB3DLut_Address;
	__u8 u8HDR_RGB3DLUT_IntervalMode; // Enable assign non-fixed interval
	__u16 *pu16HDR_RGB3DLut_ManualInterval; // R[15] + G[15] + B[15]

	//B01-05
	__u8 u8HDR_Gamma_SRAM_Mode;
	__u8 u8HDR_Gamma_Manual_Vars_en;
	__u8 u8HDR_Gamma_TR;
	__u8 u8HDR_Gamma_Lut_En;
	__u32 *pu32HDR_Gamma_Lut_Address;
	__u8 u8HDR_Gamma_unequalIntervalIn;
#if !defined(__aarch64__)
	void *pDummy2;
#endif
	__u16 u16HDR_Gamma_Lut_Length;

	//B01-06
	__u8 u8HDR_OutputCSC_Mode;
	__u8 u8HDR_OutputCSC_Ratio1;
	__u8 u8HDR_OutputCSC_Manual_Vars_en;
	__u8 u8HDR_OutputCSC_MC;

	__s32 s32HDR_OutputCSC_ManualModeCscCoef[3][3];
	bool bHDR_OutputCSC_ManualModeCscEnable;
	bool bHDR_OutputCSC_ManualModeDitherEnable;
	bool bHDR_OutputCSC_ManualModeROrCrSub16;
	bool bHDR_OutputCSC_ManualModeGOrYSub16;
	bool bHDR_OutputCSC_ManualModeBOrCbSub16;
	bool bHDR_OutputCSC_ManualModeROrCrRangeSub128;
	bool bHDR_OutputCSC_ManualModeGOrYRangeSub128;
	bool bHDR_OutputCSC_ManualModeBOrCbRangeSub128;
	bool bHDR_OutputCSC_ManualModeROrCrAdd16;
	bool bHDR_OutputCSC_ManualModeGOrYAdd16;
	bool bHDR_OutputCSC_ManualModeBOrCbAdd16;
	bool bHDR_OutputCSC_ManualModeROrCrAdd128;
	bool bHDR_OutputCSC_ManualModeBOrCbAdd128;

	//B01-07
	__u8 u8HDR_Yoffset_Mode;
	//__u16 u16ChromaWeight;

	//MaxRGB for B02
	__u8 u8HDR_MAXRGB_CSC_Mode;
	__u8 u8HDR_MAXRGB_Ratio1;
	__u8 u8HDR_MAXRGB_Manual_Vars_en;
	__u8 u8HDR_MAXRGB_MC;

	//M IP
	__u8 u8HDR_NLM_enable_Mode;
	__u8 u8HDR_NLM_setting_Mode;
	__u8 u8HDR_ACGain_enable_Mode;
	__u8 u8HDR_ACGain_setting_Mode;

	//B03
	__u8 u8HDR_ACE_enable_Mode;
	__u8 u8HDR_ACE_setting_Mode;

	//B0501
	__u8 u8HDR_Dither1_setting_Mode;

	//B0502
	__u8 u8HDR_3DLUT_enable_Mode;
	__u8 u8HDR_3DLUT_SRAM_Mode;
	__u8 u8HDR_3DLUT_setting_Mode;

	__u8 u8HDR_RangeConvert_Mode;
	//B06
	__u8 u8HDR_444to422_enable_Mode;
	__u8 u8HDR_Dither2_enable_Mode;
	__u8 u8HDR_Dither2_setting_Mode;

	__u8 u8HDRIP_Patch;

	// IP YHSL Hist report
	__u8 u8YHSL_R2Y_En;
	__u8 u8YHSL_YLimitIn_En;
	__u8 u8YHSL_Selector_Mode;
	__u8 u8YHSL_YHist_Mode;

	__u8 u8YLITE_R2Y_En;
	__u8 u8YLITE_YLimitIn_En;
	__u8 u8YLITE_Selector_Mode;
	__u8 u8YLITE_YHist_Mode; /* refer to EN_CFD_YSTATS_HIST_SEL */
};

struct STU_CFDAPI_SDRIP {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_MAIN_CONTROL_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure, u16Length=sizeof(STU_CFDAPI_Kano_SDRIP)



	//IP2 CSC
	__u8 u8IP2_CSC_Mode;
	__u8 u8IP2_CSC_Ratio1;
	__u8 u8IP2_CSC_Manual_Vars_en;
	__u8 u8IP2_CSC_MC;

	//UFSC
	__u8 u8UFSC_YCOffset_Gain_Mode;

	//VIP
	__u8 u8VIP_CSC_Mode;
	__u8 u8VIP_PreYoffset_Mode;
	__u8 u8VIP_PreYgain_Mode;
	__u8 u8VIP_PreYgain_dither_Mode;
	__u8 u8VIP_PostYoffset_Mode;
	__u8 u8VIP_PostYgain_Mode;
	__u8 u8VIP_PostYoffset2_Mode;
	__u16 u16VIP_In_Y_White_level;
	__u16 u16VIP_In_Y_Black_level;
	__u16 u16VIP_In_C_White_level;
	__u16 u16VIP_In_C_Black_level;
	__u8 u8VIP_In_Range_Mode;
	__u32 *pu32VIP_In_RangeCovert_Address;
	__u8 u8VIP_In_RangeCovert_Mode;
	__u8 u8VIP_ColorOverlay_Mode;

	//VOP2 CSC
	__u8 u8VOP_3x3_Mode;
	__u8 u8VOP_3x3_Ratio1;//0x40 = 1 Q2.6
	__u8 u8VOP_3x3_Manual_Vars_en;
	__u8 u8VOP_3x3_MC;

	//VOP2
	__u8 u8VOP_3x3RGBClip_Mode;

	//VOP2
	__u8 u8LinearRGBBypass_Mode;

	//VOP2 degamma
	__u8 u8Degamma_enable_Mode;
	__u8 u8Degamma_Dither_Mode;
	__u8 u8Degamma_SRAM_Mode;
	__u8 u8Degamma_Ratio1;//0x40 = 1 Q2.6
	__u16 u16Degamma_Ratio2;//0x40 = 1 Q2.6
	__u8 u8DeGamma_Manual_Vars_en;
	__u8 u8Degamma_TR;
	__u8 u8Degamma_Lut_En;
	__u32 *pu32Degamma_Lut_Address;
#if !defined(__aarch64__)
	void *pDummy;
#endif
	__u16 u16Degamma_Lut_Length;
	__u8  u8Degamma_Max_Lum_En;
	__u16 u16Degamma_Max_Lum;

	//VOP2 3X3
	__u8 u83x3_Lut_En;
	__s32 *ps323x3_Lut_Address;
	__u8 u83x3_enable_Mode;
	__u8 u83x3_Mode;
	__u16 u163x3_Ratio2;//0x40 = 1 Q2.6
	__u8 u83x3_Manual_Vars_en;
	__u8 u83x3_InputCP;
	__u8 u83x3_OutputCP;

	// RGB3DLUT
	__u8 u8RGB3DLUT_enable_Mode;
	__u8 u8RGB3DLUT_SRAM_Mode;
	__u8 u8RGB3DLUT_En;
	__u16 *pu16RGB3DLut_Address;

	__u8 u8Compress_settings_Mode;
	__u8 u8Compress_dither_Mode;
	__u8 u83x3Clip_Mode;

	//VOP2 gamma
	__u8 u8Gamma_enable_Mode;
	__u8 u8Gamma_Dither_Mode;
	__u8 u8Gamma_maxdata_Mode;
	__u8 u8Gamma_SRAM_Mode; //not used now

	__u8 u8Gamma_Mode_Vars_en;
	__u8 u8Gamma_TR;
	__u8 u8Gamma_Lut_En;
	__u32 *pu32Gamma_Lut_Address;
#if !defined(__aarch64__)
	void *pDummy2;
#endif
	__u16 u16Gamma_Lut_Length;

	//YCbCrClip
	__u8 u8YCbCrClip_Mode;
	__u8 u8IP2_RangeCovert_Mode;
	__u32 *pu32IP2_RangeCovert_Address;
	__u8 u8DlcIn_RangeCovert_Mode;
	__u32 *pu32DlcIn_RangeCovert_Address;
	__u8 u8DlcOut_RangeCovert_Mode;
	__u32 *pu32DlcOut_RangeCovert_Address;
	__u8 u8Uvc_Range_Mode;
	__u8 u8Dlc_Range_Mode;
	__u16 u16Dlc_Y_White_level;
	__u16 u16Dlc_Y_Black_level;
	__u16 u16Dlc_C_White_level;
	__u16 u16Dlc_C_Black_level;
	__u8 u8VIP_OutRange_Mode;
	__u16 u16VIP_Out_Y_White_level;
	__u16 u16VIP_Out_Y_Black_level;
	__u16 u16VIP_Out_C_White_level;
	__u16 u16VIP_Out_C_Black_level;
};

struct STU_CFDAPI_GOP_PRESDRIP {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_MAIN_CONTROL_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure, u16Length=sizeof(STU_CFDAPI_Kano_SDRIP)

	//IP2 CSC
	__u8 u8CSC_Mode;
	__u8 u8CSC_Ratio1;
	__u8 u8CSC_Manual_Vars_en;
	__u8 u8CSC_MC;
};

#undef INTERFACE
#endif		// _MHAL_CFD_IF_H_

