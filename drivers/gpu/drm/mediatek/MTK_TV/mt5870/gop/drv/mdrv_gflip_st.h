/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MDRV_GFLIP_ST_H
#define _MDRV_GFLIP_ST_H

//=============================================================================
// Includs
//=============================================================================

//=============================================================================
// Type and Structure Declaration
//=============================================================================
#define ST_GFLIP_GOP_CSC_PARAM_VERSION (1)

typedef enum _E_CFD_CFIO {
	E_CFD_CFIO_RGB_NOTSPECIFIED      = 0x0,
	E_CFD_CFIO_RGB_BT601_625         = 0x1,
	E_CFD_CFIO_RGB_BT601_525         = 0x2,
	E_CFD_CFIO_RGB_BT709             = 0x3,
	E_CFD_CFIO_RGB_BT2020            = 0x4,
	E_CFD_CFIO_SRGB                  = 0x5,
	E_CFD_CFIO_ADOBE_RGB             = 0x6,
	E_CFD_CFIO_YUV_NOTSPECIFIED      = 0x7,
	E_CFD_CFIO_YUV_BT601_625         = 0x8,
	E_CFD_CFIO_YUV_BT601_525         = 0x9,
	E_CFD_CFIO_YUV_BT709             = 0xA,
	E_CFD_CFIO_YUV_BT2020_NCL        = 0xB,
	E_CFD_CFIO_YUV_BT2020_CL         = 0xC,
	E_CFD_CFIO_XVYCC_601             = 0xD,
	E_CFD_CFIO_XVYCC_709             = 0xE,
	E_CFD_CFIO_SYCC601               = 0xF,
	E_CFD_CFIO_ADOBE_YCC601          = 0x10,
	E_CFD_CFIO_DOLBY_HDR_TEMP        = 0x11,
	E_CFD_CFIO_SYCC709               = 0x12,
	E_CFD_CFIO_DCIP3_THEATER         = 0x13, /// HDR10+
	E_CFD_CFIO_DCIP3_D65             = 0x14, /// HDR10+
	E_CFD_CFIO_RESERVED_START
} E_CFD_CFIO;

typedef enum _E_CFD_MC_FORMAT {
	E_CFD_MC_FORMAT_RGB     = 0x00,
	E_CFD_MC_FORMAT_YUV422  = 0x01,
	E_CFD_MC_FORMAT_YUV444  = 0x02,
	E_CFD_MC_FORMAT_YUV420  = 0x03,
	E_CFD_MC_FORMAT_RESERVED_START
} E_CFD_MC_FORMAT;

typedef enum _E_CFD_CFIO_RANGE {
	E_CFD_CFIO_RANGE_LIMIT   = 0x0,
	E_CFD_CFIO_RANGE_FULL    = 0x1,
	E_CFD_CFIO_RANGE_RESERVED_START
} E_CFD_CFIO_RANGE;

typedef struct __attribute__((__packed__)) {
	MS_U32 u32Version;
	MS_U32 u32Length;
	MS_U32 u32GOPNum;
	//STU_CFDAPI_UI_CONTROL
	MS_U16 u16Hue;
	MS_U16 u16Saturation;
	MS_U16 u16Contrast;
	MS_U16 u16Brightness;
	MS_U16 u16RGBGGain[3];
	//STU_CFDAPI_MAIN_CONTROL_LITE
	E_CFD_CFIO enInputFormat;
	E_CFD_MC_FORMAT enInputDataFormat;
	E_CFD_CFIO_RANGE enInputRange;
	E_CFD_CFIO enOutputFormat;
	E_CFD_MC_FORMAT enOutputDataFormat;
	E_CFD_CFIO_RANGE enOutputRange;
	//CFD Output CSC
	MS_U32 u32CSCAddr[10];
	MS_U16 u16CSCValue[10];
	MS_U16 u16CSCMask[10];
	MS_U16 u16CSCClient[10];
	MS_U8 u8CSCAdlData[10];
	//CFD Output Brightness
	MS_U32 u32BriAddr[3];
	MS_U16 u16BriValue[3];
	MS_U16 u16BriMask[3];
	MS_U16 u16BriClient[3];
	MS_U8 u8BriAdlData[3];
} ST_GFLIP_GOP_CSC_PARAM;

#endif //_MDRV_GFLIP_ST_H
