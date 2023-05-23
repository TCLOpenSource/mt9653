// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define _MDRV_GFLIP_C

//=============================================================================
// Include Files
//=============================================================================
#include "drvGOP.h"
#include "drvGFLIP.h"
#if defined(MSOS_TYPE_LINUX)
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>		// O_RDWR
#include <string.h>
#endif
#include "drvGOP_priv.h"

//=============================================================================
// Compile options
//=============================================================================

//=============================================================================
// Debug Macros
//=============================================================================
#ifdef CONFIG_GOP_DEBUG_LEVEL
#define GFLIP_DEBUG
#endif
#ifdef GFLIP_DEBUG
#define GFLIP_PRINT(fmt, args...)      printf("[GFlip (Driver)][%05d] " fmt, __LINE__, ## args)
#define GFLIP_ASSERT(_cnd, _fmt, _args...)  do {\
	if (!(_cnd)) {\
		GFLIP_PRINT(_fmt, ##_args); \
	} \
} while (0)
#else
#define GFLIP_PRINT(_fmt, _args...)
#define GFLIP_ASSERT(_cnd, _fmt, _args...)
#endif
E_CFD_CFIO _MDrv_GFLIP_Convert_CFIO(EN_GOP_CFD_CFIO enFormat)
{
	switch (enFormat) {
	case E_GOP_CFD_CFIO_RGB_NOTSPECIFIED:
		return E_CFD_CFIO_RGB_NOTSPECIFIED;
	case E_GOP_CFD_CFIO_RGB_BT601_625:
		return E_CFD_CFIO_RGB_BT601_625;
	case E_GOP_CFD_CFIO_RGB_BT601_525:
		return E_CFD_CFIO_RGB_BT601_525;
	case E_GOP_CFD_CFIO_RGB_BT709:
		return E_CFD_CFIO_RGB_BT709;
	case E_GOP_CFD_CFIO_RGB_BT2020:
		return E_CFD_CFIO_RGB_BT2020;
	case E_GOP_CFD_CFIO_SRGB:
		return E_CFD_CFIO_SRGB;
	case E_GOP_CFD_CFIO_ADOBE_RGB:
		return E_CFD_CFIO_ADOBE_RGB;
	case E_GOP_CFD_CFIO_YUV_NOTSPECIFIED:
		return E_CFD_CFIO_YUV_NOTSPECIFIED;
	case E_GOP_CFD_CFIO_YUV_BT601_625:
		return E_CFD_CFIO_YUV_BT601_625;
	case E_GOP_CFD_CFIO_YUV_BT601_525:
		return E_CFD_CFIO_YUV_BT601_525;
	case E_GOP_CFD_CFIO_YUV_BT709:
		return E_CFD_CFIO_YUV_BT709;
	case E_GOP_CFD_CFIO_YUV_BT2020_NCL:
		return E_CFD_CFIO_YUV_BT2020_NCL;
	case E_GOP_CFD_CFIO_YUV_BT2020_CL:
		return E_CFD_CFIO_YUV_BT2020_CL;
	case E_GOP_CFD_CFIO_XVYCC_601:
		return E_CFD_CFIO_XVYCC_601;
	case E_GOP_CFD_CFIO_XVYCC_709:
		return E_CFD_CFIO_XVYCC_709;
	case E_GOP_CFD_CFIO_SYCC601:
		return E_CFD_CFIO_SYCC601;
	case E_GOP_CFD_CFIO_ADOBE_YCC601:
		return E_CFD_CFIO_ADOBE_YCC601;
	case E_GOP_CFD_CFIO_DOLBY_HDR_TEMP:
		return E_CFD_CFIO_DOLBY_HDR_TEMP;
	case E_GOP_CFD_CFIO_SYCC709:
		return E_CFD_CFIO_SYCC709;
	case E_GOP_CFD_CFIO_DCIP3_THEATER:
		return E_CFD_CFIO_DCIP3_THEATER;
	case E_GOP_CFD_CFIO_DCIP3_D65:
		return E_CFD_CFIO_DCIP3_D65;
	default:
		return E_CFD_CFIO_RESERVED_START;
	}
}

E_CFD_MC_FORMAT _MDrv_GFLIP_Convert_MC_Format(EN_GOP_CFD_MC_FORMAT enFormat)
{
	switch (enFormat) {
	case E_GOP_CFD_MC_FORMAT_RGB:
		return E_CFD_MC_FORMAT_RGB;
	case E_GOP_CFD_MC_FORMAT_YUV422:
		return E_CFD_MC_FORMAT_YUV422;
	case E_GOP_CFD_MC_FORMAT_YUV444:
		return E_CFD_MC_FORMAT_YUV444;
	case E_GOP_CFD_MC_FORMAT_YUV420:
		return E_CFD_MC_FORMAT_YUV420;
	default:
		return E_CFD_MC_FORMAT_RESERVED_START;
	}
}

E_CFD_CFIO_RANGE _MDrv_GFLIP_Convert_CFIO_Range(EN_GOP_CFD_CFIO_RANGE enRange)
{
	switch (enRange) {
	case E_GOP_CFD_CFIO_RANGE_LIMIT:
		return E_CFD_CFIO_RANGE_LIMIT;
	case E_GOP_CFD_CFIO_RANGE_FULL:
		return E_CFD_CFIO_RANGE_FULL;
	default:
		return E_CFD_CFIO_RANGE_RESERVED_START;
	}
}

MS_BOOL MDrv_GOP_GFLIP_CSC_Tuning(MS_GOP_CTX_LOCAL *pstGOPCtx, MS_U32 u32GOPNum,
				  ST_GOP_CSC_PARAM *pstCSCParam, ST_DRV_GOP_CFD_OUTPUT *pstCFDOut)
{
//cfd move to user sapce, so it doesn't call cfd api in kernel sapce
/*
 *	ST_GFLIP_GOP_CSC_PARAM stGflipCSCParam;

 *	memset(&stGflipCSCParam, 0x0, sizeof(ST_GFLIP_GOP_CSC_PARAM));

 *	stGflipCSCParam.u32Version = ST_GFLIP_GOP_CSC_PARAM_VERSION;
 *	stGflipCSCParam.u32Length = sizeof(ST_GFLIP_GOP_CSC_PARAM);
 *	stGflipCSCParam.u32GOPNum = u32GOPNum;
 *	stGflipCSCParam.u16Hue = pstCSCParam->u16Hue;
 *	stGflipCSCParam.u16Saturation = pstCSCParam->u16Saturation;
 *	stGflipCSCParam.u16Contrast = pstCSCParam->u16Contrast;
 *	stGflipCSCParam.u16Brightness = pstCSCParam->u16Brightness;
 *	stGflipCSCParam.u16RGBGGain[0] = pstCSCParam->u16RGBGGain[0];
 *	stGflipCSCParam.u16RGBGGain[1] = pstCSCParam->u16RGBGGain[1];
 *	stGflipCSCParam.u16RGBGGain[2] = pstCSCParam->u16RGBGGain[2];
 *	stGflipCSCParam.enInputFormat = _MDrv_GFLIP_Convert_CFIO(pstCSCParam->enInputFormat);
 *	stGflipCSCParam.enInputDataFormat =
 *		_MDrv_GFLIP_Convert_MC_Format(pstCSCParam->enInputDataFormat);
 *	stGflipCSCParam.enInputRange = _MDrv_GFLIP_Convert_CFIO_Range(pstCSCParam->enInputRange);
 *	stGflipCSCParam.enOutputFormat = _MDrv_GFLIP_Convert_CFIO(pstCSCParam->enOutputFormat);
 *	stGflipCSCParam.enOutputDataFormat =
 *		_MDrv_GFLIP_Convert_MC_Format(pstCSCParam->enOutputDataFormat);
 *	stGflipCSCParam.enOutputRange = _MDrv_GFLIP_Convert_CFIO_Range(pstCSCParam->enOutputRange);

 *	if (_MDrv_GFLIP_CSC_Calc != NULL) {
 *		if (!_MDrv_GFLIP_CSC_Calc(&stGflipCSCParam)) {
 *			GFLIP_PRINT("[%s][%d] failed!\n", __func__, __LINE__);
 *			return FALSE;
 *		}
 *	} else {
 *		GFLIP_PRINT("[%s][%d] failed!\n", __func__, __LINE__);
 *		return FALSE;
 *	}

 *	memcpy(&(pstCFDOut->u32CSCAddr[0]), &stGflipCSCParam.u32CSCAddr[0],
 *		sizeof(MS_U32) * GOP_CFD_CSC_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u16CSCValue[0]), &stGflipCSCParam.u16CSCValue[0],
 *		sizeof(MS_U16) * GOP_CFD_CSC_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u16CSCMask[0]), &stGflipCSCParam.u16CSCMask[0],
 *		sizeof(MS_U16) * GOP_CFD_CSC_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u16CSCClient[0]), &stGflipCSCParam.u16CSCClient[0],
 *		sizeof(MS_U16) * GOP_CFD_CSC_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u8CSCAdlData[0]), &stGflipCSCParam.u8CSCAdlData[0],
 *		sizeof(MS_U8) * GOP_CFD_CSC_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u32BriAddr[0]), &stGflipCSCParam.u32BriAddr[0],
 *		sizeof(MS_U32) * GOP_CFD_BRI_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u16BriValue[0]), &stGflipCSCParam.u16BriValue[0],
 *		sizeof(MS_U16) * GOP_CFD_BRI_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u16BriMask[0]), &stGflipCSCParam.u16BriMask[0],
 *		sizeof(MS_U16) * GOP_CFD_BRI_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u16BriClient[0]), &stGflipCSCParam.u16BriClient[0],
 *		sizeof(MS_U16) * GOP_CFD_BRI_OUTPUT_NUM);
 *	memcpy(&(pstCFDOut->u8BriAdlData[0]), &stGflipCSCParam.u8BriAdlData[0],
 *		sizeof(MS_U8) * GOP_CFD_BRI_OUTPUT_NUM);
 */
	return TRUE;
}
