/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MDRV_VIDEO_INFO_H_
#define _MDRV_VIDEO_INFO_H_

#ifdef __MDRV_VIDEO_INFO_C_
#ifndef INTERFACE
#define INTERFACE
#endif
#else
#ifndef INTERFACE
#define INTERFACE extern
#endif
#endif

#include <linux/types.h>

enum EN_CFD_INPUT_SOURCE {
	//include VDEC series
	//E_CFD_MC_SOURCE_MM        = 0x00,
	//E_CFD_MC_SOURCE_HDMI  = 0x01,
	//E_CFD_MC_SOURCE_ANALOG    = 0x02,
	//E_CFD_MC_SOURCE_PANEL = 0x03,
	//E_CFD_MC_SOURCE_DVI     = 0x04,
	//E_CFD_MC_SOURCE_ULSA  = 0x04,
	//E_CFD_MC_SOURCE_RESERVED_START,

	/// VGA
	E_CFD_INPUT_SOURCE_VGA = 0x00,

	/// ATV
	E_CFD_INPUT_SOURCE_TV = 0x01,

	/// CVBS
	E_CFD_INPUT_SOURCE_CVBS = 0x02,

	/// S-video
	E_CFD_INPUT_SOURCE_SVIDEO = 0x03,

	/// Component
	E_CFD_INPUT_SOURCE_YPBPR = 0x04,

	/// Scart
	E_CFD_INPUT_SOURCE_SCART = 0x05,

	/// HDMI
	E_CFD_INPUT_SOURCE_HDMI = 0x06,

	/// DTV
	E_CFD_INPUT_SOURCE_DTV = 0x07,

	/// DVI
	E_CFD_INPUT_SOURCE_DVI = 0x08,

	// Application source
	/// Storage
	E_CFD_INPUT_SOURCE_STORAGE = 0x09,

	/// KTV
	E_CFD_INPUT_SOURCE_KTV = 0x0A,

	/// JPEG
	E_CFD_INPUT_SOURCE_JPEG = 0x0B,

	//RX for ulsa
	E_CFD_INPUT_SOURCE_RX = 0x0C,

	/// The max support number of PQ input source
	E_CFD_INPUT_SOURCE_RESERVED_START,

	/// None
	E_CFD_INPUT_SOURCE_NONE = E_CFD_INPUT_SOURCE_RESERVED_START,

	//For general usage, Internal Use
	E_CFD_INPUT_SOURCE_GENERAL = 0xFF,
};

//for output source
enum EN_CFD_OUTPUT_SOURCE {
	//include VDEC series
	E_CFD_OUTPUT_SOURCE_MM = 0x00,
	E_CFD_OUTPUT_SOURCE_HDMI = 0x01,
	E_CFD_OUTPUT_SOURCE_ANALOG = 0x02,
	E_CFD_OUTPUT_SOURCE_PANEL = 0x03,
	E_CFD_OUTPUT_SOURCE_ULSA = 0x04,
	E_CFD_OUTPUT_SOURCE_GENERAL = 0x05,
	E_CFD_OUTPUT_SOURCE_RESERVED_START,
};

//follow Tomato's table
enum EN_CFD_CFIO {
	E_CFD_CFIO_RGB_NOTSPECIFIED = 0x0,
	//means RGB, but no specific colorspace
	E_CFD_CFIO_RGB_BT601_625 = 0x1,
	E_CFD_CFIO_RGB_BT601_525 = 0x2,
	E_CFD_CFIO_RGB_BT709 = 0x3,
	E_CFD_CFIO_RGB_BT2020 = 0x4,
	E_CFD_CFIO_SRGB = 0x5,
	E_CFD_CFIO_ADOBE_RGB = 0x6,
	E_CFD_CFIO_YUV_NOTSPECIFIED = 0x7,
	//means RGB, but no specific colorspace
	E_CFD_CFIO_YUV_BT601_625 = 0x8,
	E_CFD_CFIO_YUV_BT601_525 = 0x9,
	E_CFD_CFIO_YUV_BT709 = 0xA,
	E_CFD_CFIO_YUV_BT2020_NCL = 0xB,
	E_CFD_CFIO_YUV_BT2020_CL = 0xC,
	E_CFD_CFIO_XVYCC_601 = 0xD,
	E_CFD_CFIO_XVYCC_709 = 0xE,
	E_CFD_CFIO_SYCC601 = 0xF,
	E_CFD_CFIO_ADOBE_YCC601 = 0x10,
	E_CFD_CFIO_DOLBY_HDR_TEMP = 0x11,
	E_CFD_CFIO_SYCC709 = 0x12,
	E_CFD_CFIO_DCIP3_THEATER = 0x13,	/// HDR10+
	E_CFD_CFIO_DCIP3_D65 = 0x14,	/// HDR10+
	E_CFD_CFIO_RESERVED_START
};

enum EN_CFD_MC_FORMAT {
	E_CFD_MC_FORMAT_RGB = 0x00,
	E_CFD_MC_FORMAT_YUV422 = 0x01,
	E_CFD_MC_FORMAT_YUV444 = 0x02,
	E_CFD_MC_FORMAT_YUV420 = 0x03,
	E_CFD_MC_FORMAT_RESERVED_START
};

enum EN_CFD_CFIO_RANGE {
	E_CFD_CFIO_RANGE_LIMIT = 0x0,
	E_CFD_CFIO_RANGE_FULL = 0x1,
	E_CFD_CFIO_RANGE_RESERVED_START
};

enum EN_CFD_CFIO_INPUTDEPTH {
	E_CFD_CFIO_INPUTDEPTH_10BIT = 0x0,
	E_CFD_CFIO_INPUTDEPTH_8BIT = 0x1,
	E_CFD_CFIO_INPUTDEPTH_RESERVED_START
};

enum EN_CFD_HDR_STATUS {
	E_CFIO_MODE_SDR = 0x0,
	E_CFIO_MODE_HDR1 = 0x1,	// Dolby
	E_CFIO_MODE_HDR2 = 0x2,	// open HDR
	E_CFIO_MODE_HDR3 = 0x3,	// Hybrid log gamma
	E_CFIO_MODE_HDR4 = 0x4,	// TCH
	E_CFIO_MODE_HDR5 = 0x5,	// HDR10plus
	E_CFIO_MODE_CONTROL_FLAG_RESET = 0x10,	// flag reset
	E_CFIO_MODE_RESERVED_START
};

//ColorPrimary
//use order in HEVC spec and add AdobeRGB
enum EN_CFD_CFIO_CP {
	E_CFD_CFIO_CP_RESERVED0 = 0x0,	//means RGB, but no specific colorspace
	E_CFD_CFIO_CP_BT709_SRGB_SYCC = 0x1,
	E_CFD_CFIO_CP_UNSPECIFIED = 0x2,
	E_CFD_CFIO_CP_RESERVED3 = 0x3,
	E_CFD_CFIO_CP_BT470_6 = 0x4,
	E_CFD_CFIO_CP_BT601625 = 0x5,
	E_CFD_CFIO_CP_BT601525_SMPTE170M = 0x6,
	E_CFD_CFIO_CP_SMPTE240M = 0x7,
	E_CFD_CFIO_CP_GENERIC_FILM = 0x8,
	E_CFD_CFIO_CP_BT2020 = 0x9,
	E_CFD_CFIO_CP_CIEXYZ = 0xA,
	E_CFD_CFIO_CP_DCIP3_THEATER = 0xB,
	E_CFD_CFIO_CP_DCIP3_D65 = 0xC,
	// 13-21: reserved
	E_CFD_CFIO_CP_EBU3213 = 0x16,
	// 23-127: reserved
	E_CFD_CFIO_CP_ADOBERGB = 0x80,
	E_CFD_CFIO_CP_PANEL = 0x81,
	E_CFD_CFIO_CP_SOURCE = 0x82,
	E_CFD_CFIO_CP_RESERVED_START
};

//Transfer characteristics
//use order in HEVC spec and add AdobeRGB
enum EN_CFD_CFIO_TR {
	E_CFD_CFIO_TR_RESERVED0 = 0x0,	//means RGB, but no specific colorspace
	E_CFD_CFIO_TR_BT709 = 0x1,
	E_CFD_CFIO_TR_UNSPECIFIED = 0x2,
	E_CFD_CFIO_TR_RESERVED3 = 0x3,
	E_CFD_CFIO_TR_GAMMA2P2 = 0x4,
	E_CFD_CFIO_TR_GAMMA2P8 = 0x5,
	E_CFD_CFIO_TR_BT601525_601625 = 0x6,
	E_CFD_CFIO_TR_SMPTE240M = 0x7,
	E_CFD_CFIO_TR_LINEAR = 0x8,
	E_CFD_CFIO_TR_LOG0 = 0x9,
	E_CFD_CFIO_TR_LOG1 = 0xA,
	E_CFD_CFIO_TR_XVYCC = 0xB,
	E_CFD_CFIO_TR_BT1361 = 0xC,
	E_CFD_CFIO_TR_SRGB_SYCC = 0xD,
	E_CFD_CFIO_TR_BT2020NCL = 0xE,
	E_CFD_CFIO_TR_BT2020CL = 0xF,
	E_CFD_CFIO_TR_SMPTE2084 = 0x10,
	E_CFD_CFIO_TR_SMPTE428 = 0x11,
	E_CFD_CFIO_TR_HLG = 0x12,
	E_CFD_CFIO_TR_BT1886 = 0x13,
	E_CFD_CFIO_TR_DOLBYMETA = 0x14,
	E_CFD_CFIO_TR_ADOBERGB = 0x15,
	E_CFD_CFIO_TR_GAMMA2P6 = 0x16,
	E_CFD_CFIO_TR_RESERVED_START
};

//Matrix coefficient
//use order in HEVC spec
enum EN_CFD_CFIO_MC {
	E_CFD_CFIO_MC_IDENTITY = 0x0,	//means RGB, but no specific colorspace
	E_CFD_CFIO_MC_BT709_XVYCC709 = 0x1,
	E_CFD_CFIO_MC_UNSPECIFIED = 0x2,
	E_CFD_CFIO_MC_RESERVED = 0x3,
	E_CFD_CFIO_MC_USFCCT47 = 0x4,
	E_CFD_CFIO_MC_BT601625_XVYCC601_SYCC = 0x5,
	E_CFD_CFIO_MC_BT601525_SMPTE170M = 0x6,
	E_CFD_CFIO_MC_SMPTE240M = 0x7,
	E_CFD_CFIO_MC_YCGCO = 0x8,
	E_CFD_CFIO_MC_BT2020NCL = 0x9,
	E_CFD_CFIO_MC_BT2020CL = 0xA,
	E_CFD_CFIO_MC_YDZDX = 0xB,
	E_CFD_CFIO_MC_CD_NCL = 0xC,
	E_CFD_CFIO_MC_CD_CL = 0xD,
	E_CFD_CFIO_MC_ICTCP = 0xE,
	E_CFD_CFIO_MC_RESERVED_START
};

enum EN_CFD_HDMI_HDR_INFOFRAME_EOTF {
	E_CFD_HDMI_EOTF_SDR_GAMMA = 0x0,
	E_CFD_HDMI_EOTF_HDR_GAMMA = 0x1,
	E_CFD_HDMI_EOTF_SMPTE2084 = 0x2,
	E_CFD_HDMI_EOTF_FUTURE_EOTF = 0x3,
	E_CFD_HDMI_EOTF_RESERVED = 0x4
};

struct STU_CFD_COLORIMETRY {
	//order R->G->B
	__u16 u16Display_Primaries_x[3];
	//data *0.00002 0xC350 = 1
	__u16 u16Display_Primaries_y[3];
	//data *0.00002 0xC350 = 1
	__u16 u16White_point_x;
	//data *0.00002 0xC350 = 1
	__u16 u16White_point_y;
	//data *0.00002 0xC350 = 1
};

struct STU_IN_CFD_TMO_CONTROL {
	__u16 u16SourceMax;
	//target maximum in nits, 1~10000
	__u8 u16SourceMaxFlag;
	//flag of target maximum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16SourceMed;
	//target minimum in nits, 1~10000
	__u8 u16SourceMedFlag;
	//flag of target maximum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16SourceMin;
	//target minimum in nits, 1~10000
	__u8 u16SourceMinFlag;
	//flag of target minimum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16TgtMax;
	//target maximum in nits, 1~10000
	__u8 u16TgtMaxFlag;
	//flag of target maximum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16TgtMed;
	//target maximum in nits, 1~10000
	__u8 u16TgtMedFlag;
	//flag of target minimum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16TgtMin;
	//target minimum in nits, 1~10000
	__u8 u16TgtMinFlag;
	//flag of target minimum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	//__u8 u8TMO_TargetMode
	///0 : keeps the value in initial function
	///1 : from output source

	//0: TMO in PQ to PQ
	//1: TMO in Gamma to Gamma
	//2-255 reserved
	__u8 u8TMO_domain_mode;
};

struct STU_CFD_MS_ALG_COLOR_FORMAT {
	//color space
	//assign with enum EN_CFD_CFIO

	//E_CFD_CFIO_RGB
	//E_CFD_CFIO_YUV
	//enum EN_CFD_CFIO
	__u8 u8Input_Format;

	//enum EN_CFD_MC_FORMAT
	__u8 u8Input_DataFormat;

	//limit/full
	//assign with enum EN_CFD_CFIO_RANGE
	//0:limit 1:full
	__u8 u8Input_IsFullRange;

	//SDR/HDR
	//0:SDR
	//1:HDR1
	//2:HDR2
	__u8 u8Input_HDRMode;

	__u8 u8InputBitExtendMode;

	//Only for TV
	//0:PQ path
	//1:bypass CSC
	__u8 u8Input_IsRGBBypass;

	//special control
	//especially for Maserati
	//0: bypass all SDR IP
	//1: HDR IP by decision tree or user
	//2: bypass all SDR IP besides OutputCSC, for Panel out case
	__u8 u8Input_SDRIPMode;

	//0: bypass all HDR IP
	//1: HDR IP by decision tree or user
	//2: set HDR IP to open HDR
	//3: set HDR IP to Dolby HDR
	__u8 u8Input_HDRIPMode;

	//__u8 u8Input_IsHDRIPFromDolbyDriver;

	//follow E_CFD_CFIO_GAMUTORDER_IDX
	//use for non-panel output
	__u8 u8Input_GamutOrderIdx;


	//E_CFD_CFIO_RGB
	//E_CFD_CFIO_YUV
	//enum EN_CFD_CFIO
	__u8 u8Output_Format;

	__u8 u8Output_DataFormat;

	__u8 u8Output_GamutOrderIdx;
	//follow E_CFD_CFIO_GAMUTORDER_IDX
	//use for non-panel output

	__u8 u8Output_IsFullRange;

	//SDR/HDR
	//0:SDR
	//1:HDR1
	//2:HDR2
	__u8 u8Output_HDRMode;

	//Temp_Format[0] : output of IP2 CSC, input of HDR IP
	//Temp_Format[1] : output of HDR IP, input of SDR IP
	__u8 u8Temp_Format[2];

	//enum EN_CFD_MC_FORMAT
	__u8 u8Temp_DataFormat[2];

	//enum EN_CFD_CFIO_RANGE
	__u8 u8Temp_IsFullRange[2];

	//E_CFIO_HDR_STATUS
	__u8 u8Temp_HDRMode[2];

	__u8 u8Temp_GamutOrderIdx[2];

	//redefinition
	//item 0-10 are the same as table E.3 in HEVC spec
	__u8 u8InputColorPriamries;
	__u8 u8OutputColorPriamries;
	__u8 u8TempColorPriamries[2];
	//0:Reserverd
	//1:BT. 709/sRGB/sYCC
	//2:unspecified
	//3:Reserverd
	//4:BT. 470-6
	//5:BT. 601_625/PAL/SECAM
	//6:BT. 601_525/NTSC/SMPTE_170M
	//7:SMPTE_240M
	//8:Generic film
	//9:BT. 2020
	//10:CIEXYZ
	//11:AdobeRGB
	//255:undefined

	//redefinition
	//item 0-17 are the same as table E.4 in HEVC spec
	__u8 u8InputTransferCharacterstics;
	__u8 u8OutputTransferCharacterstics;
	__u8 u8TempTransferCharacterstics[2];
	///0 Reserverd
	///1 BT. 709
	///2 unspecified
	///3 Reserverd
	///4 Assume display gamma 2.2
	///5 Assume display gamma 2.8
	///6 BT. 601_525/BT. 601_525
	///7 SMPTE_240M
	///8 linear
	///9 Logarithmic (100:1 range)
	///10    Logarithmic (100*sqrt(10):1 range)
	///11    xvYCC
	///12    BT. 1361 extend color gamut system
	///13    sRGB/sYCC
	///14    BT. 2020
	///15    BT. 2020
	///16    SMPTE ST2084 for 10.12.14.16-bit systetm
	///17    SMPTE ST428-1
	///18   AdobeRGB
	///255  undefined

	//the same the same as table E.5 in HEVC spec
	__u8 u8InputMatrixCoefficients;
	__u8 u8OutputMatrixCoefficients;
	__u8 u8TempMatrixCoefficients[2];
	//0 Identity
	//1 BT. 709/xvYCC709
	//2 unspecified
	//3 Reserverd
	//4 USFCCT 47
	//5 BT. 601_625/PAL/SECAM/xvYCC601/sYCC
	//6 BT. 601_525/NTSC/SMPTE_170M
	//7 SMPTE_240M
	//8 YCgCo
	//9 BT. 2020NCL(non-constant luminance)
	//10    BT. 2020CL(constant luminance)
	//255:Reserverd

	//for process status
	//In SDR IP
	__u8 u8DoTMO_Flag;
	__u8 u8DoGamutMapping_Flag;
	__u8 u8DoDLC_Flag;
	__u8 u8DoBT2020CLP_Flag;

	//In HDR IP
	__u8 u8DoTMOInHDRIP_Flag;
	__u8 u8DoGamutMappingInHDRIP_Flag;
	__u8 u8DoDLCInHDRIP_Flag;
	__u8 u8DoBT2020CLPInHDRIP_Flag;
	//__u8 u8DoFull2LimitInHDRIP_Flag;
	__u8 u8DoForceEnterHDRIP_Flag;
	//__u8 u8DoForceFull2LimitInHDRIP_Flag;

	//for process status2
	__u8 u8DoMMIn_ForceHDMI_Flag;
	__u8 u8DoMMIn_Force709_Flag;
	__u8 u8DoHDMIIn_Force709_Flag;
	__u8 u8DoOtherIn_Force709_Flag;
	__u8 u8DoOutput_Force709_Flag;

	//for process status 3
	__u8 u8DoHDRIP_Forcebypass_Flag;
	__u8 u8DoSDRIP_ForceNOTMO_Flag;
	__u8 u8DoSDRIP_ForceNOGM_Flag;
	__u8 u8DoSDRIP_ForceNOBT2020CL_Flag;

	//
	__u8 u8DoPreConstraints_Flag;

	//E_CFD_CFIO_RANGE_LIMIT
	//E_CFD_CFIO_RANGE_FULL
	__u8 u8DoPathFullRange_Flag;

	//for process status 3
	//__u8 u8Output_format_ForcedChange_Flag;

	//HW Patch
	__u8 u8HW_PatchMode;

	//temp variable for debug
	__u16 u16_check_status;

	//other interior structure
	struct STU_IN_CFD_TMO_CONTROL stu_CFD_TMO_Param;

	//Main/sub control from outside
	__u8 u8HW_MainSub_Mode;

	//input source from outside API
	__u8 u8Input_Source;

	__s16 u16DolbySupportStatus;

	struct STU_CFD_COLORIMETRY stu_Panel_Param_Colorimetry;
};

struct STU_CFD_MS_ALG_COLOR_FORMAT_LITE {
	//enum EN_CFD_CFIO
	__u8 u8Input_Format;

	//enum EN_CFD_MC_FORMAT
	__u8 u8Input_DataFormat;

	//limit/full
	//assign with enum EN_CFD_CFIO_RANGE
	//0:limit 1:full
	__u8 u8Input_IsFullRange;

	//SDR/HDR
	//0:SDR
	//1:HDR1
	//2:HDR2
	__u8 u8Input_HDRMode;

	//follow E_CFD_CFIO_GAMUTORDER_IDX
	//use for non-panel output
	//__u8 u8Input_GamutOrderIdx;

	//E_CFD_CFIO_RGB
	//E_CFD_CFIO_YUV
	//enum EN_CFD_CFIO
	__u8 u8Output_Format;

	__u8 u8Output_DataFormat;

	//__u8 u8Output_GamutOrderIdx;
	//follow E_CFD_CFIO_GAMUTORDER_IDX
	//use for non-panel output

	__u8 u8Output_IsFullRange;

	//SDR/HDR
	//0:SDR
	//1:HDR1
	//2:HDR2
	__u8 u8Output_HDRMode;

	//Temp_Format[0] : output of IP2 CSC, input of HDR IP
	//Temp_Format[1] : output of HDR IP, input of SDR IP
	__u8 u8Temp_Format[2];

	//enum EN_CFD_MC_FORMAT
	__u8 u8Temp_DataFormat[2];

	//enum EN_CFD_CFIO_RANGE
	__u8 u8Temp_IsFullRange[2];

	//E_CFIO_HDR_STATUS
	__u8 u8Temp_HDRMode[2];

	//__u8 u8Temp_GamutOrderIdx[2];

	//redefinition
	//item 0-10 are the same as table E.3 in HEVC spec
	__u8 u8InputColorPriamries;
	__u8 u8OutputColorPriamries;
	__u8 u8TempColorPriamries[2];
	//0:Reserverd
	//1:BT. 709/sRGB/sYCC
	//2:unspecified
	//3:Reserverd
	//4:BT. 470-6
	//5:BT. 601_625/PAL/SECAM
	//6:BT. 601_525/NTSC/SMPTE_170M
	//7:SMPTE_240M
	//8:Generic film
	//9:BT. 2020
	//10:CIEXYZ
	//11:AdobeRGB
	//255:undefined

	//redefinition
	//item 0-17 are the same as table E.4 in HEVC spec
	__u8 u8InputTransferCharacterstics;
	__u8 u8OutputTransferCharacterstics;
	__u8 u8TempTransferCharacterstics[2];
	///0 Reserverd
	///1 BT. 709
	///2 unspecified
	///3 Reserverd
	///4 Assume display gamma 2.2
	///5 Assume display gamma 2.8
	///6 BT. 601_525/BT. 601_525
	///7 SMPTE_240M
	///8 linear
	///9 Logarithmic (100:1 range)
	///10    Logarithmic (100*sqrt(10):1 range)
	///11    xvYCC
	///12    BT. 1361 extend color gamut system
	///13    sRGB/sYCC
	///14    BT. 2020
	///15    BT. 2020
	///16    SMPTE ST2084 for 10.12.14.16-bit systetm
	///17    SMPTE ST428-1
	///18   AdobeRGB
	///255  undefined

	//the same the same as table E.5 in HEVC spec
	__u8 u8InputMatrixCoefficients;
	__u8 u8OutputMatrixCoefficients;
	__u8 u8TempMatrixCoefficients[2];
	///0 Identity
	///1 BT. 709/xvYCC709
	///2 unspecified
	///3 Reserverd
	///4 USFCCT 47
	///5 BT. 601_625/PAL/SECAM/xvYCC601/sYCC
	///6 BT. 601_525/NTSC/SMPTE_170M
	///7 SMPTE_240M
	///8 YCgCo
	///9 BT. 2020NCL(non-constant luminance)
	///10    BT. 2020CL(constant luminance)
	///255:Reserverd
	struct STU_CFD_COLORIMETRY stu_Cfd_ColorMetry[4];


	//temp variable for debug
	__u16 u16_check_status;

	__u8 u8DoTMO_Flag;
	__u8 u8DoGamutMapping_Flag;
	__u8 u8DoDLC_Flag;
	__u8 u8DoBT2020CLP_Flag;

	//0 or 1
	//0: u8HWGroup is for mainsub
	//1: u8HWGroup is for GOP groups
	__u8 u8HWGroupMode;
	__u8 u8HWGroup;
};

struct STU_CFD_DePQRefence {
	__u8 DePQClamp_EN;
	__u8 DePQreferMetadata_EN;
	//unit in nits
	//default is 1000
	__u8 DePQClampNOffset;
	// unit in nits
	// default is 1000
	__u16 DePQClampMin;
};

struct HDR_CONTENT_LIGHT_MATADATA {
	__u8 u8MetaData_Valid;
	__u16 u16MaxContentLightLevel;
	__u16 u16MaxFrameAverageLightLevel;
	__u16 u16Maxluminance_clip;
	__u16 u16Maxluminance_clip_manualmode;
	__u8 u8HDR_mode;
	bool bTMO_method;
};

struct STU_CFDAPI_HDMI_INFOFRAME_PARSER_OUT {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_HDMI_INFOFRAME_OUT_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFDAPI_HDMI_INFOFRAME_PARSER)

	//for debug
	__u8 u8HDMISource_HDR_InfoFrame_Valid;
	//assign by enum E_CFD_VALIDORNOT
	//0:Not valid
	//1:valid

	//for debug
	__u8 u8HDMISource_EOTF;
	//assign by enum EN_CFD_HDMI_HDR_INFOFRAME_EOTF
	//Data byte 1 in Dynamic range and mastering infoFrame
	//enum EN_CFD_HDMI_HDR_INFOFRAME_EOTF
	//E_CFD_HDMI_EOTF_SDR_GAMMA     = 0x0,
	//E_CFD_HDMI_EOTF_HDR_GAMMA     = 0x1,
	//E_CFD_HDMI_EOTF_SMPTE2084     = 0x2,
	//E_CFD_HDMI_EOTF_FUTURE_EOTF       = 0x3,
	//E_CFD_HDMI_EOTF_RESERVED      = 0x4
	//8-255 is reserved

	//for debug
	__u8 u8HDMISource_SMD_ID;
	//Data byte 2 in Dynamic range and mastering infoFrame
	//8-255 is reserved

	//from Static_Metadata_Descriptor
	__u16 u16Master_Panel_Max_Luminance;
	//data * 1 nits
	__u16 u16Master_Panel_Min_Luminance;
	//data * 0.0001 nits
	__u16 u16Max_Content_Light_Level;
	//data * 1 nits
	__u16 u16Max_Frame_Avg_Light_Level;
	//data * 1 nits

	struct STU_CFD_COLORIMETRY stu_Cfd_HDMISource_MasterPanel_ColorMetry;

	__u8 u8Mastering_Display_Infor_Valid;
	//assign by enum E_CFD_VALIDORNOT
	//0:Not valid
	//1:valid

	//for debug
	__u8 u8HDMISource_Support_Format;
	//[2:0] = {Y2 Y1 Y0}
	//[4:3] = {YQ1 YQ0}
	//[6:5] = {Q1 Q0}
	//information in AVI infoFrame

	//for debug
	__u8 u8HDMISource_Colorspace;
	//assign by enum EN_CFD_CFIO
	///0 = RGB
	///1 = BT601_625
	///...
	///255 = undefined/reserved

	//__u8 u8HDMISource_AVIInfoFrame_Valid;
	//assign by enum E_CFD_VALIDORNOT
	//0:Not valid
	//1:valid

	__u8 u8Output_DataFormat;
};


struct STU_CFDAPI_MAIN_CONTROL_FORMAT {
	//0:Like HDMI case, for this point, CFD only depends on u8Curr_Format
	//1:Like MM case, u8Curr_Format is not used,
	//u8Curr_Colour_primaries,u8Curr_Transfer_Characteristics,
	//and u8Curr_Matrix_Coeffs
	//are used to determine u8Curr_Format

	__u8 u8Mid_Format_Mode;

	//enum EN_CFD_CFIO
	__u8 u8Mid_Format;

	//enum EN_CFD_MC_FORMAT
	__u8 u8Mid_DataFormat;

	//enum EN_CFD_CFIO_RANGE
	__u8 u8Mid_IsFullRange;

	//E_CFIO_HDR_STATUS
	__u8 u8Mid_HDRMode;

	//assign by enum EN_CFD_CFIO_CP
	__u8 u8Mid_Colour_primaries;

	//assign by enum EN_CFD_CFIO_TR
	__u8 u8Mid_Transfer_Characteristics;

	//assign by enum EN_CFD_CFIO_MC
	__u8 u8Mid_Matrix_Coeffs;
};

struct STU_CFDAPI_MAIN_CONTROL {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_MAIN_CONTROL_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	/// u16Length=sizeof(STU_CFD_MAIN_CONTROL)

	//Process Mode
	//0: color format driver off
	//1: color format dirver on - normal mode
	//2: color format driver on - test mode
	//__u8 u8Process_Mode;

	//EMU:E_CFD_MC_HW_STRUCTURE
	//define the HW structure use this function
	//__u8 u8HW_Structure;

	//force 1 now
	//only for u8HW_Structure = E_CFD_HWS_STB_TYPE1
	//__u8 u8HW_PatchEn;

	//E_CFD_MC_SOURCE
	//specify which input source
	__u8 u8Input_Source;

	//E_CFD_INPUT_ANALOG_FORMAT
	//__u8 u8Input_AnalogIdx;

	//enum EN_CFD_CFIO
	__u8 u8Input_Format;

	__u8 u8InputDataDepth;

	//enum EN_CFD_MC_FORMAT
	//specify RGB/YUV format of the input of the first HDR/SDR IP
	//E_CFD_MC_FORMAT_RGB       = 0x00,
	//E_CFD_MC_FORMAT_YUV422    = 0x01,
	//E_CFD_MC_FORMAT_YUV444    = 0x02,
	//E_CFD_MC_FORMAT_YUV420    = 0x03,
	__u8 u8Input_DataFormat;

	//limit/full
	//assign with enum EN_CFD_CFIO_RANGE
	//0:limit 1:full
	__u8 u8Input_IsFullRange;

	//SDR/HDR
	//EN_CFD_HDR_STATUS
	//0:SDR
	//1:HDR1
	//2:HDR2
	__u8 u8Input_HDRMode;

	//only for information of Analog input
	//assign by enum EN_CFD_CFIO_CP
	__u8 u8Input_ext_Colour_primaries;
	__u8 u8Input_ext_Transfer_Characteristics;
	__u8 u8Input_ext_Matrix_Coeffs;

	//control from Qmap
	//0:RGB not bypass CSC (not R2Y)
	//1:RGB bypass CSC (force R2Y)
	__u8 u8Input_IsRGBBypass;

	//special control
	//especially for Maserati
	//0: bypass all SDR IP
	//1: SDR IP by decision tree or user
	//2: bypass all SDR IP besides the last OutputCSC
	//With this value, Each IC will has its own control decision tree.
	__u8 u8Input_SDRIPMode;

	//0: bypass all HDR IP
	//1: HDR IP by decision tree or user
	//2: set HDR IP to open HDR
	//3: set HDR IP to Dolby HDR
	__u8 u8Input_HDRIPMode;

	__s16 u16DolbySupportStatus;

	struct STU_CFDAPI_MAIN_CONTROL_FORMAT stu_Middle_Format[1];

	//specify RGB/YUV format of the output of the last HDR/SDR IP
	//E_CFD_MC_SOURCE
	__u8 u8Output_Source;

	//enum EN_CFD_CFIO
	__u8 u8Output_Format;

	//enum EN_CFD_MC_FORMAT
	__u8 u8Output_DataFormat;

	//0:limit 1:full
	__u8 u8Output_IsFullRange;

	//E_CFIO_HDR_STATUS
	//0:SDR
	//1:HDR1
	//2:HDR2
	__u8 u8Output_HDRMode;

	//only for HDMI out ==================================================
	//find a better one  for HDMI out,
	//although user assigns a specific output colormetry
	//mode 0:output colorimetry is based on output_format
	//mode 1:output colorimetry is based on the information
	//from u8HDMISink_Extended_Colorspace,which is parsed from  EDID
	__u8 u8HDMIOutput_GammutMapping_Mode;

	//when sink support multiple colormetries, find out a new one.
	//__u8 u9HDMIOutput_GammutMapping_Mode;
	//mode 0: when input colorimetry and requested output colorimetry are
	//not same , force gamut extension, like BT709 to BT2020
	//mode 1: when input colorimetry and requested output colorimetry are
	//not same , force gamut compression, like BT2020 to BT709
	__u8 u8HDMIOutput_GammutMapping_MethodMode;

	//only for MM input =================================================
	//please force 1 for current status
	//mode 0:off
	//mode 1:Match MM to HDMI format ;  if not matched , Force 709
	//mode 2:Force 709
	//mode 3:Match MM to HDMI format ;  if not matched , output directly
	//__u8 u8Force_InputColoriMetryRemapping;
	__u8 u8MMInput_ColorimetryHandle_Mode;

	//only for Panel output =============================================
	//mode0:off
	//mode1:do GamutMapping to Panel
	__u8 u8PanelOutput_GammutMapping_Mode;
	//====================================================================


	//======================================================================

	//0:use default value
	//use u16Source_Max_Luminance,u16Source_Min_Luminance,...

	//1:use Panel/HDMI EDID infor for target Luminance range
	//if not exist , the same value as 0
	__u8 u8TMO_TargetRefer_Mode;

	//======================================================================

	//for TMO version 0
	//range 1 nits to 10000 nits
	__u16 u16Source_Max_Luminance;	//data * 1 nits
	__u16 u16Source_Med_Luminance;	//data * 1 nits

	//range 1e-4 nits to 6.55535 nits
	__u16 u16Source_Min_Luminance;	//data * 0.0001 nits

	__u16 u16Target_Med_Luminance;	//data * 1 nits

	//for TMO version 0 and 1
	//range 1 nits to 10000 nits
	__u16 u16Target_Max_Luminance;	//data * 1 nits

	//range 1e-4 nits to 6.55535 nits
	__u16 u16Target_Min_Luminance;	//data * 0.0001 nits

	//report Process
	//bit0 DoDLCflag
	//bit1 DoGamutMappingflag
	//bit2 DoTMOflag

	__u8 u8Process_Status;
	__u8 u8Process_Status2;
	__u8 u8Process_Status3;
};

struct STU_CFDAPI_MAIN_CONTROL_GOP {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_MAIN_CONTROL_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(STU_CFD_MAIN_CONTROL)

	//E_CFD_MC_SOURCE
	//specify which input source
	__u8 u8Input_Source;

	//enum EN_CFD_CFIO
	__u8 u8Input_Format;

	//enum EN_CFD_MC_FORMAT
	//specify RGB/YUV format of the input of the first HDR/SDR IP
	//E_CFD_MC_FORMAT_RGB       = 0x00,
	//E_CFD_MC_FORMAT_YUV422    = 0x01,
	//E_CFD_MC_FORMAT_YUV444    = 0x02,
	//E_CFD_MC_FORMAT_YUV420    = 0x03,
	__u8 u8Input_DataFormat;

	//limit/full
	//assign with enum EN_CFD_CFIO_RANGE
	//0:limit 1:full
	__u8 u8Input_IsFullRange;

	//SDR/HDR
	//E_CFIO_HDR_STATUS
	//0:SDR
	//1:HDR1
	//2:HDR2
	__u8 u8Input_HDRMode;

	//assign by enum EN_CFD_CFIO_CP
	__u8 u8Input_ext_Colour_primaries;

	//assign by enum EN_CFD_CFIO_TR
	__u8 u8Input_ext_Transfer_Characteristics;

	//assign by enum EN_CFD_CFIO_MC
	__u8 u8Input_ext_Matrix_Coeffs;

	//used this gamut when u8Input_ext_Colour_primaries
	//is E_CFD_CFIO_CP_SOURCE
	struct STU_CFD_COLORIMETRY stu_Cfd_source_ColorMetry;

	//specify RGB/YUV format of the output of the last HDR/SDR IP
	//E_CFD_MC_SOURCE
	__u8 u8Output_Source;

	//enum EN_CFD_CFIO
	__u8 u8Output_Format;

	//enum EN_CFD_MC_FORMAT
	__u8 u8Output_DataFormat;

	//0:limit 1:full
	__u8 u8Output_IsFullRange;

	//E_CFIO_HDR_STATUS
	//0:SDR
	//1:HDR1
	//2:HDR2
	__u8 u8Output_HDRMode;

	//assign by enum EN_CFD_CFIO_CP
	__u8 u8Output_ext_Colour_primaries;

	//assign by enum EN_CFD_CFIO_TR
	__u8 u8Output_ext_Transfer_Characteristics;

	//assign by enum EN_CFD_CFIO_MC
	__u8 u8Output_ext_Matrix_Coeffs;

	//used this gamut when u8Output_ext_Colour_primaries
	//is E_CFD_CFIO_CP_SOURCE
	struct STU_CFD_COLORIMETRY stu_Cfd_target_ColorMetry;

	//struct STU_CFDAPI_MAIN_CONTROL_GOP_TESTING
};


struct STU_CFDAPI_GENERAL_CONTROL_FORMAT {
	__u8 u8Source;
	__u8 u8Format;
	__u8 u8DataFormat;
	__u8 u8IsFullRange;
	__u8 u8HDRMode;

	__u8 u8Colour_primaries;
	__u8 u8Transfer_Characteristics;
	__u8 u8Matrix_Coeffs;

	struct STU_CFD_COLORIMETRY stu_Cfd_source_ColorMetry;
};

struct STU_CFD_HDR10PLUS_SEI {
	__u8 u8Itu_t_t35_country_code;
	// set to 0xB5 for HDR10+

	__u16 u16Itu_t_t35_terminal_provider_code;
	// set to 0x003C for HDR10+

	__u16 u16Itu_t_t35_terminal_provider_oriented_code;
	// set to 0x0001 for HDR10+

	__u8 u8Application_Identifier;
	// set to 4 for HDR10+

	__u8 u8Application_Version;
	// set to 1 for HDR10+

	__u8 u8Num_Windows;
	// set to 1 for HDR10+

	__u32 u32Target_System_Display_Max_Luminance;
	// [0, max:10000], value>10000 -> value=10000 nits

	__u8 u8Target_System_Display_Actual_Peak_Luminance_Flag;
	// set to 0 for HDR10+

	__u32 u32MaxSCL[3];
	// 0x00000-0x186A0
	// MAY be set to 0x00000 to indicate "Not Used" by the content provider

	__u32 u32Average_MaxRGB;
	// 0x00000-0x186A0

	__u8 u8Num_Distributions;
	// set to 9 for HDR10+

	__u8 u8Distribution_Index[9];
	// [0]: 1
	// [1]: 5
	// [2]: 10
	// [3]: 25
	// [4]: 50
	// [5]: 75
	// [6]: 90
	// [7]: 95
	// [8]: 99

	__u32 u32Distribution_Values[9];
	// [0]: 1% percentile linearized maxRGB value * 10
	// [1]: DistributionY99 * 10
	// [2]: DistributionY100nit
	// [3]: 25% percentile linearized maxRGB value * 10
	// [4]: 50% percentile linearized maxRGB value * 10
	// [5]: 75% percentile linearized maxRGB value * 10
	// [6]: 90% percentile linearized maxRGB value * 10
	// [7]: 95% percentile linearized maxRGB value * 10
	// [8]: 99.98% percentile linearized maxRGB value * 10
	// [idx=2]: 0-100; [others]: 0x00000-0x186A0

	__u16 u16Fraction_Bright_Pixels;
	// set to 0 for HDR10+

	__u8 u8Master_Display_Actual_Peak_Luminance_Flag;
	// set to 0 for HDR10+

	__u8 u8Tone_Mapping_Flag;
	// set to 0 for Profile A
	// set to 1 for Profile B: with Basis Tone Mapping Curve (Bezier curve)

	__u16 u16Knee_Point_x;
	__u16 u16Knee_Point_y;
	// 12 bits [0, 4095]

	__u8 u8Num_Bezier_Curve_Anchors;
	// 0-9
	// if value=0, no u16Bezier_Curve_Anchors information

	__u16 u16Bezier_Curve_Anchors[9];
	// 10 bits [0, 1023]

	__u8 u8Color_Saturation_Mapping_Flag;
	// set to 0 for HDR10+
};

struct STU_CFD_2094_10 {
	bool bLevel1MetadataIsValid;
	__u8 u8MinPQ;
	__u8 u8Max_PQ;
	__u8 u8Avg_PQ;

	bool bLevel2MetadataIsValid;
	__u16 u16Target_max_PQ;
	__u16 u16Trim_slope;
	__u16 u16Trim_offset;
	__u16 u16Trim_power;
	__u16 u16Trim_chroma_weight;
	__u16 u16Trim_saturation_gain;
	__u16 u16Ms_weight;

	bool bLevel3MetadataIsValid;
	__u16 u16Min_PQ_offset;
	__u16 u16Max_PQ_offset;
	__u16 u16Avg_PQ_offset;

	bool bLevel5MetadataIsValid;
	__u16 u16Active_area_left_offset;
	__u16 u16Active_area_right_offset;
	__u16 u16Active_area_top_offset;
	__u16 u16Active_area_bottom_offset;
};

struct STU_CFD_2094_20 {
	__u8 u8PartID;
	__u8 u8MajorSpecVersionID;
	__u8 u8MinorSpecVersionID;
	__u8 u8PayloadMode;

	//hdr_characteristics:
	__u8 u8HdrPicColourSpace;
	__u8 u8HdrDiaplayColourSpace;
	__u16 u16HdrDiaplayMaxLuminance;
	__u16 u16HdrDiaplayMinLuminance;

	//sdr_characteristics:
	__u8 u8SdrPicColourSpace;
	__u16 u16SdrDiaplayMaxLuminance;
	__u16 u16SdrDiaplayMinLuminance;

	__u16 u16MatrixCoeffient[4];

	__u16 u16ChromaToLumaInjection[2];

	__u8 u8KCoeffient[3];

	//luminance_mapping_variables:
	__u8 u8TmInputSignalBlackLevelOffset;
	__u8 u8TmInputSignalWhiteLevelOffset;
	__u8 u8ShadowGain;
	__u8 u8HighlightGain;
	__u8 u8MidToneWidthAdjFactor;
	__u8 u8TmOutputFineTuningNumVal;
	__u8 u8TmOutputFineTuningX[16];
	__u8 u8TmOutputFineTuningY[16];

	//colour_correction_adjustment:
	__u8 u8SaturationGainNumVal;
	__u8 u8SaturationGainX[16];
	__u8 u8SaturationGainY[16];

	//luminance_mapping_table:
	__u8 u8LuminanceMappingNumVal;
	__u16 u16LuminanceMappingX[66];
	__u16 u16LuminanceMappingY[66];

	//colour_correction_table:
	__u8 u8ColourCorrectionNumVal;
	__u16 u16ColourCorrectionX[66];
	__u16 u16ColourCorrectionY[66];
};

struct STU_CFD_2094_30 {

	__u32 u32Colour_remap_id;
	bool bColour_remap_cancel_flag;
	bool bColour_remap_persistence_flag;

	bool bColour_remap_video_signal_info_persistence_flag;
	bool bColour_remap_full_range_flag;
	__u8 u8Colour_remap_primaries;
	__u8 u8Colour_remap_transfer_function;
	__u8 u8Colour_remap_matrix_coefficients;
	__u8 u8Colour_remap_input_bit_depth;
	__u8 u8Colour_remap_output_bit_depth;
	__u8 u8pre_lut_num_val_minus1[3];

	__u16 u16Pre_lut_coded_val[3][33];

	__u16 u16Pre_lut_target_val[3][33];

	bool bColour_remap_matrix_persistence_flag;
	__u8 u8Log2_matrix_denom;
	__u16 u16Colour_remap_coeffs[3][3];

	__u8 u8Post_lut_num_val_minus1[3];

	__u16 u16Post_lut_coded_val[3][33];

	__u16 u16Post_lut_target_val[3][33];
};

struct STU_CFD_2094_40 {
	__u8 u8Itu_t_t35_country_code;
	__u16 u16Itu_t_t35_terminal_provider_code;
	__u16 u16Itu_t_t35_terminal_provider_oriented_code;
	__u8 u8Application_identifier;
	__u8 u8Application_version;

	__u8 u8Num_windows;
	// 1 ~ 3
	__u16 u16Window_upper_left_corner_x[3];
	__u16 u16Window_upper_left_corner_y[3];
	__u16 u16Window_upper_right_corner_x[3];
	__u16 u16Window_upper_right_corner_y[3];
	__u16 u16Center_of_ellipse_x[3];
	__u16 u16Center_of_ellipse_y[3];
	__u8 u8Rotation_angle[3];
	__u16 u16Semimajor_axis_internal_ellipse[3];
	__u16 u16Semimajor_axis_external_ellipse[3];
	__u16 u16Semiminor_axis_external_ellipse[3];
	bool bOverlap_process_option[3];

	__u32 u32Target_system_display_max_luminance;
	bool bTarget_system_display_actual_peak_luminance_flag;
	__u8 u8Num_rows_targeted_system_display_actual_peak_luminance;
	// 2 ~ 25
	__u8 u8Num_cols_targeted_system_display_actual_peak_luminance;
	// 2 ~ 25
	__u8 u8Targeted_system_display_actual_peak_luminance[25][25];

	__u32 u32Maxscl[3][3];
	__u32 u32Average_maxrgb[3];

	__u8 u8Num_distribution_maxrgb_percentiles[3];
	//15 max
	__u8 u8Distribution_maxrgb_percentages[3][15];
	__u32 u32Distribution_maxrgb_percentiles[3][15];
	__u16 u16Fraction_bright_pixels[3];

	bool bMastering_display_actual_peak_luminance_flag;
	__u8 u8Num_rows_mastering_display_actual_peak_luminance;
	// 2 ~ 25
	__u8 u8Num_cols_mastering_display_actual_peak_luminance;
	// 2 ~ 25
	__u8 u8Mastering_system_display_actual_peak_luminance[25][25];

	__u8 u8Tone_mapping_flag[3];
	__u16 u16Knee_point_x[3];
	__u16 u16Knee_point_y[3];

	__u8 u8Num_bezier_curve_anchors[3];
	__u16 u16Bezier_curve_anchors[3][15];

	__u8 u8Color_saturation_mapping_flag[3];
	__u8 u8Color_saturation_weight[3];
};

struct STU_CFD_HDR10PLUS_VSVDB {
	__u8 u8Tag_Code;
	// set to 7 for HDR10+

	__u8 u8Length;
	// set to 5 for HDR10+

	__u8 u8Extended_Tag_Code;
	// set to 0x01 for HDR10+

	__u32 u32IEEE_24Bit_Code;
	// set to 0x90848B for HDR10+

	__u8 u8Application_Version;
	// set to 1 for HDR10+
};

struct STU_CFD_HDR10PLUS_VSIF {
	__u8 u8VSIF_Type_Code;
	// set to 0x01 for HDR10+

	__u8 u8VSIF_Version;
	// set to 0x01 for HDR10+

	__u8 u8Length;
	// set to 27 for HDR10+

	__u32 u32IEEE_24Bit_Code;
	// set to 0x90848B for HDR10+

	__u8 u8Application_Version;
	// set to 1 for HDR10+

	__u8 u8Target_System_Display_Max_Luminance;
	// 0-31, unit:32 -> [0, 992]

	__u8 u8Average_MaxRGB;
	// 0-255, unit:16 -> [0, 4080]

	__u8 u8Distribution_Values[9];
	// [idx=2]: 0-100
	// [others]: 0-255, unit: 16 -> [0, 4080]

	__u16 u16Knee_Point_x;
	__u16 u16Knee_Point_y;
	// 0-1023, unit:4 -> [0, 4092]

	__u8 u8Num_Bezier_Curve_Anchors;
	// 0-9
	// if value=0, no u16Bezier_Curve_Anchors information

	__u8 u8Bezier_Curve_Anchors[9];
	// 0-255, unit:4 -> [0, 1020]

	__u8 u8Graphics_Overlay_Flag;
	// set to 1 to indicate that video signal associated with
	// the HDR10+ Metadata has graphics overlay

	__u8 u8No_Delay_Flag;
	// SHALL always set to 0
	// set to 0: video signal associated with the HDR10+ Metadata is
	// transmitted one frame after the HDR10+ Metadata
	//           i.e. HDR10+ Metadata is transmitted one frame ahead of
	//           the associated video signal
	// set to 1: video signal associated with the HDR10+ Metadata
	// is transmitted synchronously
};

struct STU_CFDAPI_MM_PARSER {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_MM_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFDAPI_MM_PARSER)

	//__u8 u8MM_Codec;
	//1:HEVC 0:others

	//s__u8 u8ColorDescription_Valid;
	//assign by enum E_CFD_VALIDORNOT
	///0 :Not valid
	///1 :valid

	//assign by enum EN_CFD_CFIO_CP
	__u8 u8Colour_primaries;
	//0:Reserverd
	//1:BT. 709/sRGB/sYCC
	//2:unspecified
	//3:Reserverd
	//4:BT. 470-6
	//5:BT. 601_625/PAL/SECAM
	//6:BT. 601_525/NTSC/SMPTE_170M
	//7:SMPTE_240M
	//8:Generic film
	//9:BT. 2020
	//10:CIEXYZ
	//11-255:Reserverd

	__u8 u8Transfer_Characteristics;
	//assign by enum EN_CFD_CFIO_TR

	///0 Reserverd
	///1 BT. 709
	///2 unspecified
	///3 Reserverd
	///4 Assume display gamma 2.2
	///5 Assume display gamma 2.8
	///6 BT. 601_525/BT. 601_525
	///7 SMPTE_240M
	///8 linear
	///9 Logarithmic (100:1 range)
	///10    Logarithmic (100*sqrt(10):1 range)
	///11    xvYCC
	///12    BT. 1361 extend color gamut system
	///13    sRGB/sYCC
	///14    BT. 2020
	///15    BT. 2020
	///16    SMPTE ST2084 for 10.12.14.16-bit systetm
	///17    SMPTE ST428-1
	///18-255:Reserverd

	__u8 u8Matrix_Coeffs;
	//assign by enum EN_CFD_CFIO_MC
	///0 Identity
	///1 BT. 709/xvYCC709
	///2 unspecified
	///3 Reserverd
	///4 USFCCT 47
	///5 BT. 601_625/PAL/SECAM/xvYCC601/sYCC
	///6 BT. 601_525/NTSC/SMPTE_170M
	///7 SMPTE_240M
	///8 YCgCo
	///9 BT. 2020NCL(non-constant luminance)
	///10    BT. 2020CL(constant luminance)
	///11-255:Reserverd

	__u8 u8Video_Full_Range_Flag;
	//assign by enum EN_CFD_CFIO_RANGE
	//0:limit range
	//1:full range

	__u32 u32Master_Panel_Max_Luminance;
	//data * 0.0001 nits
	__u32 u32Master_Panel_Min_Luminance;
	//data * 0.0001 nits
	//__u16 u16Max_Content_Light_Level;	//data * 1 nits
	//__u16 u16Max_Frame_Avg_Light_Level;	//data * 1 nits

	struct STU_CFD_COLORIMETRY stu_MasterPanel_ColorMetry;

	__u8 u8Mastering_Display_Infor_Valid;
	//1:valid

	__u8 u8MM_HDR_ContentLightMetaData_Valid;
	// 1:valid 0:not valid
	__u16 u16Max_content_light_level;
	// data * 1 nits
	__u16 u16Max_pic_average_light_level;
	// data * 1 nits

	/// HDR10plus : Dynamic Metadata
	__u8 u8MM_HDR10plus_SEI_Message_Valid;
	// 1: valid, stu_Cfd_MM_HDR10plus_SEI exist
	// 0: not valid, stu_Cfd_MM_HDR10plus_SEI not exist

	struct STU_CFD_HDR10PLUS_SEI stu_Cfd_MM_HDR10plus_SEI;

	struct STU_CFD_2094_10 *pstu_Cfd_MMSource_2094_10;
	struct STU_CFD_2094_20 *pstu_Cfd_MMSource_2094_20;
	struct STU_CFD_2094_30 *pstu_Cfd_MMSource_2094_30;
	struct STU_CFD_2094_40 *pstu_Cfd_MMSource_2094_40;
};

struct STU_CFDAPI_HDMI_EDID_PARSER {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_HDMI_EDID_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFDAPI_HDMI_EDID_PARSER)

	__u8 u8HDMISink_HDRData_Block_Valid;
	//assign by enum E_CFD_VALIDORNOT
	///0 :Not valid
	///1 :valid

	__u8 u8HDMISink_EOTF;
	//byte 3 in HDR static Metadata Data block

	__u8 u8HDMISink_SM;
	//byte 4 in HDR static Metadata Data block

	__u8 u8HDMISink_Desired_Content_Max_Luminance;
	//need a LUT to transfer
	__u8 u8HDMISink_Desired_Content_Max_Frame_Avg_Luminance;
	//need a LUT to transfer
	__u8 u8HDMISink_Desired_Content_Min_Luminance;
	//need a LUT to transfer
	//byte 5 ~ 7 in HDR static Metadata Data block

	__u8 u8HDMISink_HDRData_Block_Length;
	//byte 1[4:0] in HDR static Metadata Data block

	//order R->G->B
	//__u16 u16display_primaries_x[3];	//data *1/1024 0x03FF = 0.999
	//__u16 u16display_primaries_y[3];	//data *1/1024 0x03FF = 0.999
	//__u16 u16white_point_x;	//data *1/1024 0x03FF = 0.999
	//__u16 u16white_point_y;	//data *1/1024 0x03FF = 0.999
	struct STU_CFD_COLORIMETRY stu_Cfd_HDMISink_Panel_ColorMetry;
	//address 0x19h to 22h in base EDID

	__u8 u8HDMISink_EDID_base_block_version;
	//for debug
	//address 0x12h in EDID base block

	__u8 u8HDMISink_EDID_base_block_reversion;
	//for debug
	//address 0x13h in EDID base block

	__u8 u8HDMISink_EDID_CEA_block_reversion;
	//for debug
	//address 0x01h in EDID CEA block

	//table 59 Video Capability Data Block (VCDB)
	//0:VCDB is not available
	//1:VCDB is available
	__u8 u8HDMISink_VCDB_Valid;

	__u8 u8HDMISink_Support_YUVFormat;
	//bit 0:Support_YUV444
	//bit 1:Support_YUV422
	//bit 2:Support_YUV420

	//QY in Byte#3 in table 59 Video Capability Data Block (VCDB)
	//bit 3:RGB_quantization_range

	//QS in Byte#3 in table 59 Video Capability Data Block (VCDB)
	//bit 4:Y_quantization_range
	//0:no data(due to CE or IT video) ; 1:selectable


	__u8 u8HDMISink_Extended_Colorspace;
	//byte 3 of Colorimetry Data Block
	//bit 0:xvYCC601
	//bit 1:xvYCC709
	//bit 2:sYCC601
	//bit 3:Adobeycc601
	//bit 4:Adobergb
	//bit 5:BT2020 cl
	//bit 6:BT2020 ncl
	//bit 7:BT2020 RGB

	__u8 u8HDMISink_EDID_Valid;
	//assign by enum E_CFD_VALIDORNOT
	///0 :Not valid
	///1 :valid

	/// HDR10plus
	__u8 u8HDMISink_HDR10plus_VSVDB_Valid;
	// 1: valid, stu_Cfd_HDMISink_HDR10plus_VSVDB exist
	// 0: not valid, stu_Cfd_HDMISink_HDR10plus_VSVDB not exist

	struct STU_CFD_HDR10PLUS_VSVDB stu_Cfd_HDMISink_HDR10plus_VSVDB;

	__u8 u8HDMISink_Extended_Colorspace_Byte4;
	// support DCI-P3
	// byte 4 of Colorimetry Data Block
	// bit 0: MD0
	// bit 1: MD1
	// bit 2: MD2
	// bit 3: MD3
	// bit 4: F44=0
	// bit 5: F45=0
	// bit 6: F46=0
	// bit 7: DCI-P3
	///
};

struct STU_CFDAPI_DOLBY_HDR_METADATA_FORMAT {
	__u8 u8IsDolbyHDREn;
};

struct STU_CFDAPI_HDR_METADATA_FORMAT {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_HDMI_HDR_METADATA_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFDAPI_HDR_METADATA_FORMAT)

	struct STU_CFDAPI_DOLBY_HDR_METADATA_FORMAT stu_Cfd_Dolby_HDR_Param;
};

struct STU_CFDAPI_PANEL_FORMAT {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_HDMI_PANEL_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFDAPI_PANEL_FORMAT)

	__u16 u16Panel_Med_Luminance;
	//data * 1 nits
	__u16 u16Panel_Max_Luminance;
	//data * 1 nits
	__u16 u16Panel_Min_Luminance;
	//data * 0.0001 nits

	//order R->G->B
	struct STU_CFD_COLORIMETRY stu_Cfd_Panel_ColorMetry;
};

struct STU_CFDAPI_OSD_CONTROL {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_HDMI_OSD_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFDAPI_OSD_CONTROL)

	__u16 u16Hue;
	__u16 u16Saturation;
	__u16 u16Contrast;
	__s32 s32ColorCorrectionMatrix[3][3];
	__u8 u8ColorCorrection_En;
	//0:off
	//1:on
	//default on , not in the document
	__u8 u8OSD_UI_En;

	//Mode 0: update matrix by OSD and color format driver
	//Mode 1: only update matrix by OSD controls
	//for mode1 : the configures of matrix
	//keep the same as the values by calling CFD last time
	__u8 u8OSD_UI_Mode;

	//0:auto depends on STB rule
	//1:always do HDR2SDR for HDR input
	//2:always not do HDR2SDR for HDR input
	__u8 u8HDR_UI_H2SMode;
};

struct XC_KDRV_REF_2086_METADATA {
	/// Structure version
	__u32 u32Version;
	/// Structure length
	__u16 u16Length;
	/// 2086MetaData Enable
	bool bMetaDataRef;
};

struct STU_CFDAPI_HDMI_INFOFRAME_PARSER {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_HDMI_INFOFRAME_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFDAPI_HDMI_INFOFRAME_PARSER)

	//for debug
	__u8 u8HDMISource_HDR_InfoFrame_Valid;
	//assign by enum E_CFD_VALIDORNOT
	//0 :Not valid
	///1 :valid

	//for debug
	__u8 u8HDMISource_EOTF;
	//assign by enum EN_CFD_HDMI_HDR_INFOFRAME_EOTF
	//Data byte 1 in Dynamic range and mastering infoFrame
	//enum EN_CFD_HDMI_HDR_INFOFRAME_EOTF
	//E_CFD_HDMI_EOTF_SDR_GAMMA     = 0x0,
	//E_CFD_HDMI_EOTF_HDR_GAMMA     = 0x1,
	//E_CFD_HDMI_EOTF_SMPTE2084     = 0x2,
	//E_CFD_HDMI_EOTF_FUTURE_EOTF       = 0x3,
	//E_CFD_HDMI_EOTF_RESERVED      = 0x4
	//8-255 is reserved

	//for debug
	__u8 u8HDMISource_SMD_ID;
	//Data byte 2 in Dynamic range and mastering infoFrame
	//8-255 is reserved

	//from Static_Metadata_Descriptor
	__u16 u16Master_Panel_Max_Luminance;
	//data * 1 nits
	__u16 u16Master_Panel_Min_Luminance;
	//data * 0.0001 nits
	__u16 u16Max_Content_Light_Level;
	//data * 1 nits
	__u16 u16Max_Frame_Avg_Light_Level;
	//data * 1 nits

	struct STU_CFD_COLORIMETRY stu_Cfd_MasterPanel_ColorMetry;

	__u8 u8Mastering_Display_Infor_Valid;
	//assign by enum E_CFD_VALIDORNOT
	//0 :Not valid
	///1 :valid

	//for debug
	__u8 u8HDMISource_Support_Format;
	//[2:0] = {Y2 Y1 Y0}
	//[4:3] = {YQ1 YQ0}
	//[6:5] = {Q1 Q0}
	//information in AVI infoFrame

	//for debug
	__u8 u8HDMISource_Colorspace;
	//assign by enum EN_CFD_CFIO
	//0 = RGB
	///1 = BT601_625
	//...
	//255 = undefined/reserved

	//__u8 u8HDMISource_AVIInfoFrame_Valid;
	//assign by enum E_CFD_VALIDORNOT
	//0 :Not valid
	///1 :valid

	/// HDR10plus
	__u16 u16HDMISource_Extended_InfoFrame_Type_Code;
	// set to 4 for HDR10+
	// 0x0001~0x0004 are used

	__u8 u8HDMISource_HDR10plus_VSIF_Packet_Valid;
	// 1: valid, stu_Cfd_HDMISource_HDR10plus_VSIF exist
	// 0: not valid, stu_Cfd_HDMISource_HDR10plus_VSIF not exist

	struct STU_CFD_HDR10PLUS_VSIF stu_Cfd_HDMISource_HDR10plus_VSIF;

	__u8 u8HDMISource_2094_40_Valid;
	//1: valid, pstu_Cfd_HDMISource_2094_40 received
	//0: not valid, pstu_Cfd_HDMISource_2094_40 not received

	struct STU_CFD_2094_10 *pstu_Cfd_HDMISource_2094_10;
	struct STU_CFD_2094_20 *pstu_Cfd_HDMISource_2094_20;
	struct STU_CFD_2094_30 *pstu_Cfd_HDMISource_2094_30;
	struct STU_CFD_2094_40 *pstu_Cfd_HDMISource_2094_40;

	__u8 u8HDMISource_Colorspace_ACE;
	// Additional Colorimetry Extension (ACE)
	// support DCI-P3
	// byte 14 of Auxiliary Video Information (AVI)
	// InfoFrame Format (Version 4)
	// bit 0: F140=0
	// bit 1: F141=0
	// bit 2: F142=0
	// bit 3: F143=0
	// bit 4: ACE0
	// bit 5: ACE1
	// bit 6: ACE2
	// bit 7: ACE3
	// DCI-P3 RGB (D65):     {ACE3, ACE2, ACE1, ACE0} = {0, 0, 0, 0}
	// DCI-P3 RGB (theater): {ACE3, ACE2, ACE1, ACE0} = {0, 0, 0, 1}
	///
};

struct STU_CFDAPI_UI_CONTROL {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_HDMI_OSD_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFDAPI_UI_CONTROL)

	//1x = 50
	//range = [0-100]
	__u16 u16Hue;

	//1x = 128
	//range = [0-255]
	__u16 u16Saturation;

	//1x = 1024
	//range = [0-2047]
	__u16 u16Contrast;

	__u8 u8ColorCorrection_En;

	//1x = 1024
	//rnage = [-2048-2047]
	__s32 s32ColorCorrectionMatrix[3][3];

	//1x = 1024
	//range = [0-2047]
	__u16 u16Brightness;

	//1x = 1024
	//range = [0-2048]
	//u16RGBGGain[0] = Rgain
	//u16RGBGGain[1] = Ggain
	//u16RGBGGain[2] = Bgain
	__u16 u16RGBGGain[3];

	//0:off
	//1:on
	//default on , not in the document
	__u8 u8OSD_UI_En;

	//Mode 0: update matrix by OSD and color format driver
	//Mode 1: only update matrix by OSD controls
	//for mode1 : the configures of matrix keep the same as
	//the values by calling CFD last time
	__u8 u8OSD_UI_Mode;

	//0:auto depends on STB rule
	//1:always do HDR2SDR for HDR input
	//2:always not do HDR2SDR for HDR input
	__u8 u8HDR_UI_H2SMode;

	__u16 au16Brightness[3];

};

struct STU_CFDAPI_GOP_FORMAT {
	__u32 u32Version;
	__u16 u16Length;

	//bit[0]:
	//1: GOP use premultiplied Alpha

	//bit[1]: IsAlphaForGOPFlag
	//1: alpha value is for GOP
	__u8 u8GOP_AlphaFormat;
};

struct STU_CFDAPI_DEBUG {
	__u32 u32Version;
	__u16 u16Length;

	__u8 ShowALLInputInCFDEn;
};

#undef INTERFACE
#endif		// _MDRV_VIDEO_INFO_H_

