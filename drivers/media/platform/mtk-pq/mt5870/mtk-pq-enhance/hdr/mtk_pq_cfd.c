// SPDX-License-Identifier: GPL-2.0
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

#ifndef _MTK_PQ_CFD_C_
#define _MTK_PQ_CFD_C_

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <uapi/linux/mtk-v4l2-pq.h>
#include "mdrv_video_info_if.h"
#include "mdrv_output_format_if.h"
#include "mhal_cfd_if.h"
#include "mdrv_cfd_if.h"
#include "mtk_pq_cfd.h"

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Local Type and Structurs
//------------------------------------------------------------------------------
enum cfd_param_bit {
	CFD_PARAM_BIT_ALL,
	CFD_PARAM_BIT_MAIN,
	CFD_PARAM_BIT_HDMI,
	CFD_PARAM_BIT_MM,
	CFD_PARAM_BIT_OUTPUT,
	CFD_PARAM_BIT_UI,
	CFD_PARAM_BIT_TMO,
	CFD_PARAM_BIT_BLACK_WHITE,
	CFD_PARAM_BIT_DOLBY_HDR,
	CFD_PARAM_BIT_420CTRL,
	CFD_PARAM_BIT_FG,
};

//------------------------------------------------------------------------------
//  Global Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Local Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
static bool _mtk_cfd_IsFlagSet(__u32 u32Status,
	enum cfd_param_bit eBit)
{
	return (u32Status & BIT(eBit));
}

static bool _mtk_cfd_SetFlag(__u32 *pu32Status,
	enum cfd_param_bit eBit)
{
	if (!pu32Status)
		return false;
	*pu32Status |= BIT(eBit);

	return true;
}

static bool _mtk_cfd_ClearFlag(__u32 *pu32Status,
	enum cfd_param_bit eBit)
{
	if (!pu32Status)
		return false;
	*pu32Status &= ~BIT(eBit);

	return true;
}

static bool _mtk_cfd_CalcColorFmt(__u8 *pu8ColorFormat,
		__u8 u8PixelFormat, __u8 u8Colorimetry,
		__u8 u8ExtColorimetry, __u8 u8AdditionalColorimetry)
{
	if (pu8ColorFormat == NULL)
		return false;

	if ((u8PixelFormat == 1) || (u8PixelFormat == 2)
	|| (u8PixelFormat == 3)) {
		if (u8Colorimetry == 0)
			*pu8ColorFormat = E_CFD_CFIO_YUV_BT709;
		else if (u8Colorimetry == 1)
			*pu8ColorFormat = E_CFD_CFIO_YUV_BT601_525;
		else if (u8Colorimetry == 2)
			*pu8ColorFormat = E_CFD_CFIO_YUV_BT709;
		else if (u8Colorimetry == 3) {
			if (u8ExtColorimetry == 0)
				*pu8ColorFormat = E_CFD_CFIO_XVYCC_601;
			else if (u8ExtColorimetry == 1)
				*pu8ColorFormat = E_CFD_CFIO_XVYCC_709;
			else if (u8ExtColorimetry == 2)
				*pu8ColorFormat = E_CFD_CFIO_SYCC601;
			else if (u8ExtColorimetry == 3)
				*pu8ColorFormat = E_CFD_CFIO_ADOBE_YCC601;
			else if (u8ExtColorimetry == 5)
				*pu8ColorFormat = E_CFD_CFIO_YUV_BT2020_CL;
			else if (u8ExtColorimetry == 6)
				*pu8ColorFormat = E_CFD_CFIO_YUV_BT2020_NCL;
			else
				*pu8ColorFormat = E_CFD_CFIO_YUV_BT709;
		} else
			*pu8ColorFormat = E_CFD_CFIO_YUV_BT709;
	} else {
		if (u8Colorimetry == 0)
			*pu8ColorFormat = E_CFD_CFIO_SRGB;
		else if (u8Colorimetry == 1)
			*pu8ColorFormat = E_CFD_CFIO_RGB_BT601_525;
		else if (u8Colorimetry == 2)
			*pu8ColorFormat = E_CFD_CFIO_RGB_BT709;
		else if (u8Colorimetry == 3) {
			if (u8ExtColorimetry == 0)
				*pu8ColorFormat = E_CFD_CFIO_RGB_BT709;
			else if (u8ExtColorimetry == 1)
				*pu8ColorFormat = E_CFD_CFIO_RGB_BT709;
			else if (u8ExtColorimetry == 2)
				*pu8ColorFormat = E_CFD_CFIO_SRGB;
			else if (u8ExtColorimetry == 3)
				*pu8ColorFormat = E_CFD_CFIO_ADOBE_RGB;
			else if (u8ExtColorimetry == 4)
				*pu8ColorFormat = E_CFD_CFIO_ADOBE_RGB;
			else if (u8ExtColorimetry == 5)
				*pu8ColorFormat = E_CFD_CFIO_RGB_BT2020;
			else if (u8ExtColorimetry == 6)
				*pu8ColorFormat = E_CFD_CFIO_RGB_BT2020;
			else if (u8ExtColorimetry == 7) {
				if (u8AdditionalColorimetry == 0)
					*pu8ColorFormat = E_CFD_CFIO_DCIP3_D65;
				else if (u8AdditionalColorimetry == 1)
					*pu8ColorFormat =
						E_CFD_CFIO_DCIP3_THEATER;
				else
					*pu8ColorFormat = E_CFD_CFIO_RGB_BT709;
			} else
				*pu8ColorFormat = E_CFD_CFIO_RGB_BT709;
		}
	}

	return true;
}

static enum EN_CFD_HDR_STATUS _mtk_cfd_HDRTypeCovert(bool bHdrOff,
	bool bIsDV, bool bIsTCH, enum v4l2_hdr_type enHDRType)
{
	enum EN_CFD_HDR_STATUS eHDR;

	if (bIsDV) {
		eHDR = E_CFIO_MODE_HDR1;
	} else if (bIsTCH) {
		eHDR = E_CFIO_MODE_HDR4;
	} else {
		switch (enHDRType) {
		case V4L2_HDR_TYPE_NONE:
			eHDR = E_CFIO_MODE_SDR;
			break;
		case V4L2_HDR_TYPE_OPEN:
			eHDR = E_CFIO_MODE_HDR2;
			if (bHdrOff)
				eHDR = E_CFIO_MODE_SDR;
			break;
		case V4L2_HDR_TYPE_HLG:
			eHDR = E_CFIO_MODE_HDR3;
			if (bHdrOff)
				eHDR = E_CFIO_MODE_SDR;
			break;
		case V4L2_HDR_TYPE_HDR10_PLUS:
			eHDR = E_CFIO_MODE_HDR5;
			if (bHdrOff)
				eHDR = E_CFIO_MODE_HDR2;
			break;
		default:
			eHDR = E_CFIO_MODE_SDR;
			break;
		}
	}

	return eHDR;
}

bool _mtk_cfd_CovertEOTF2HDRType(__u8 *pHDRType,
	__u8 u8EOTF, bool bIsVSIFHDR10Plus, bool bIsEMPHDR10Plus)
{
	if (pHDRType == NULL)
		return false;

	switch (u8EOTF) {
	case 2:
		*pHDRType = V4L2_HDR_TYPE_OPEN;
		if (bIsVSIFHDR10Plus || bIsEMPHDR10Plus)
			*pHDRType = V4L2_HDR_TYPE_HDR10_PLUS;
		break;
	case 3:
		*pHDRType = V4L2_HDR_TYPE_HLG;
		break;
	default:
		// dolby hdr type check by dolby
		*pHDRType = V4L2_HDR_TYPE_NONE;
		break;
	}

	return true;
}

static bool _mtk_cfd_CovertHDR10PlusSEI(
	struct STU_CFD_HDR10PLUS_SEI *pst10PlusSEI,
	struct st_hdr10plus_sei *pstMetaData)
{
	if ((pst10PlusSEI == NULL) || (pstMetaData == NULL))
		return false;

	pst10PlusSEI->u8Itu_t_t35_country_code = pstMetaData->u8CountryCode;
	pst10PlusSEI->u16Itu_t_t35_terminal_provider_code =
		pstMetaData->u16TerminalProviderCode;
	pst10PlusSEI->u16Itu_t_t35_terminal_provider_oriented_code =
		pstMetaData->u16TerminalProviderOrientedCode;
	pst10PlusSEI->u8Application_Identifier =
		pstMetaData->u8ApplicationIdentifier;
	pst10PlusSEI->u8Application_Version =
		pstMetaData->u8ApplicationVersion;
	pst10PlusSEI->u8Num_Windows = pstMetaData->u8NumWindows;
	pst10PlusSEI->u32Target_System_Display_Max_Luminance =
		pstMetaData->u32TargetSystemDisplayMaxLuminance;
	pst10PlusSEI->u8Target_System_Display_Actual_Peak_Luminance_Flag =
		pstMetaData->u8TargetSystemDisplayActualPeakLuminanceFlag;
	memcpy(pst10PlusSEI->u32MaxSCL, pstMetaData->au32MaxSCL,
		sizeof(__u32) * 3);
	pst10PlusSEI->u32Average_MaxRGB = pstMetaData->u32AverageMaxRGB;
	pst10PlusSEI->u8Num_Distributions = pstMetaData->u8NumDistributions;
	memcpy(pst10PlusSEI->u8Distribution_Index,
		pstMetaData->au8DistributionIndex, sizeof(__u8) * 9);
	memcpy(pst10PlusSEI->u32Distribution_Values,
		pstMetaData->au32DistributionValues, sizeof(__u32) * 9);
	pst10PlusSEI->u16Fraction_Bright_Pixels =
		pstMetaData->u16FractionBrightPixels;
	pst10PlusSEI->u8Master_Display_Actual_Peak_Luminance_Flag =
		pstMetaData->u8MasterDisplayActualPeakLuminanceFlag;
	pst10PlusSEI->u8Tone_Mapping_Flag = pstMetaData->u8ToneMappingFlag;
	pst10PlusSEI->u16Knee_Point_x = pstMetaData->u16KneePointX;
	pst10PlusSEI->u16Knee_Point_y = pstMetaData->u16KneePointY;
	pst10PlusSEI->u8Num_Bezier_Curve_Anchors =
		pstMetaData->u8NumBezierCurveAnchors;
	memcpy(pst10PlusSEI->u16Bezier_Curve_Anchors,
		pstMetaData->au16BezierCurveAnchors, sizeof(__u16) * 9);
	pst10PlusSEI->u8Color_Saturation_Mapping_Flag =
		pstMetaData->u8ColorSaturationMappingFlag;

	return true;
}

static bool _mtk_cfd_CovertHDR10PlusVSIF(
	struct STU_CFD_HDR10PLUS_VSIF *pst10PlusVSIF,
	struct st_vsif_10plus_info *pstMetaData)
{
	if ((pst10PlusVSIF == NULL) || (pstMetaData == NULL))
		return false;

	pst10PlusVSIF->u8VSIF_Type_Code = pstMetaData->u8VSIFTypeCode;
	pst10PlusVSIF->u8VSIF_Version = pstMetaData->u8VSIFVersion;
	pst10PlusVSIF->u8Length = pstMetaData->u8Length;
	pst10PlusVSIF->u32IEEE_24Bit_Code = pstMetaData->u32IEEECode;
	pst10PlusVSIF->u8Application_Version =
		pstMetaData->u8ApplicationVersion;
	pst10PlusVSIF->u8Target_System_Display_Max_Luminance =
		pstMetaData->u8TargetSystemDisplayMaxLuminance;
	pst10PlusVSIF->u8Average_MaxRGB = pstMetaData->u8AverageMaxRGB;
	memcpy(pst10PlusVSIF->u8Distribution_Values,
		pstMetaData->au8DistributionValues, sizeof(__u8) * 9);
	pst10PlusVSIF->u16Knee_Point_x = pstMetaData->u16KneePointX;
	pst10PlusVSIF->u16Knee_Point_y = pstMetaData->u16KneePointY;
	pst10PlusVSIF->u8Num_Bezier_Curve_Anchors =
		pstMetaData->u8NumBezierCurveAnchors;
	memcpy(pst10PlusVSIF->u8Bezier_Curve_Anchors,
		pstMetaData->au8BezierCurveAnchors, sizeof(__u8) * 9);
	pst10PlusVSIF->u8Graphics_Overlay_Flag =
		pstMetaData->u8GraphicsOverlayFlag;
	pst10PlusVSIF->u8No_Delay_Flag = pstMetaData->u8NoDelayFlag;

	return true;
}

static bool _mtk_cfd_CovertHDR10PlusEMP(
	struct STU_CFD_2094_40 *pst10PlusEMP,
	struct st_emp_10plus_info *pstMetaData)
{
	__u8 u8Cnt;
	__u8 u8Temp;

	if ((pst10PlusEMP == NULL) || (pstMetaData == NULL))
		return false;

	pst10PlusEMP->u8Itu_t_t35_country_code =
		pstMetaData->u8ItuTT35CountryCode;
	pst10PlusEMP->u16Itu_t_t35_terminal_provider_code =
		pstMetaData->u16ItuTT35TerminalProviderCode;
	pst10PlusEMP->u16Itu_t_t35_terminal_provider_oriented_code =
		pstMetaData->u16ItuTT35TerminalProviderOrientedCode;
	pst10PlusEMP->u8Application_identifier =
		pstMetaData->u8ApplicationIdentifier;
	pst10PlusEMP->u8Application_version = pstMetaData->u8ApplicationVersion;
	pst10PlusEMP->u8Num_windows = pstMetaData->u8NumWindows;
	pst10PlusEMP->u32Target_system_display_max_luminance =
		pstMetaData->u32TargetedSystemDisplayMaximumLuminance;
	pst10PlusEMP->bTarget_system_display_actual_peak_luminance_flag =
		pstMetaData->bTargetSystemDisplayActualPeakLuminanceFlag;
	pst10PlusEMP->u32Maxscl[0][0] = pstMetaData->au32Maxscl[0];
	pst10PlusEMP->u32Maxscl[0][1] = pstMetaData->au32Maxscl[1];
	pst10PlusEMP->u32Maxscl[0][2] = pstMetaData->au32Maxscl[2];
	pst10PlusEMP->u32Average_maxrgb[0] = pstMetaData->au32Average_maxrgb[0];
	pst10PlusEMP->u32Average_maxrgb[1] = pstMetaData->au32Average_maxrgb[1];
	pst10PlusEMP->u32Average_maxrgb[2] = pstMetaData->au32Average_maxrgb[2];
	pst10PlusEMP->u8Num_distribution_maxrgb_percentiles[0] =
		pstMetaData->u8NumDistributions;
	u8Temp = pst10PlusEMP->u8Num_distribution_maxrgb_percentiles[0];
	for (u8Cnt = 0; u8Cnt < u8Temp; u8Cnt++) {
		pst10PlusEMP->u8Distribution_maxrgb_percentages[0][u8Cnt] =
			pstMetaData->au8DistributionIndex[u8Cnt];
		pst10PlusEMP->u32Distribution_maxrgb_percentiles[0][u8Cnt] =
			pstMetaData->au32DistributionValues[u8Cnt];
	}
	pst10PlusEMP->u16Fraction_bright_pixels[0] =
		pstMetaData->u16FractionBrightPixels;
	pst10PlusEMP->bMastering_display_actual_peak_luminance_flag =
		pstMetaData->u8MasteringDisplayActualPeakLuminanceFlag;
	pst10PlusEMP->u8Tone_mapping_flag[0] = pstMetaData->bToneMappingFlag;
	pst10PlusEMP->u16Knee_point_x[0] = pstMetaData->u16KneePointX;
	pst10PlusEMP->u16Knee_point_y[0] = pstMetaData->u16KneePointY;
	pst10PlusEMP->u8Num_bezier_curve_anchors[0] =
		pstMetaData->u8NumBezierCurveAnchors;
	u8Temp = pst10PlusEMP->u8Num_bezier_curve_anchors[0];
	for (u8Cnt = 0; u8Cnt < u8Temp; u8Cnt++) {
		pst10PlusEMP->u16Bezier_curve_anchors[0][u8Cnt] =
			pstMetaData->au16BezierCurveAnchors[u8Cnt];
	}
	pst10PlusEMP->u8Color_saturation_mapping_flag[0] =
		pstMetaData->u8ColorSaturationMappingFlag;

	return true;
}

static bool _mtk_cfd_SetMainParamInput(
	struct st_cfd_info *pstCFDInfo,
	struct STU_CFDAPI_MAIN_CONTROL *pstMainControl)
{
	struct st_cfd_ui_info *pstUI;
	struct st_cfd_input_info *pstIn;
	struct st_pq_status_info *pstPQ;
	struct st_cfd_status_info *pstCFD;
	__u8 u8InputSRC;
	__u8 u8ColorRangeType;
	bool bHDROff;
	bool bIsDV;
	bool bIsTCH;

	if ((!pstCFDInfo) || (pstMainControl == NULL))
		return false;
	pstIn = &pstCFDInfo->stIn;
	pstUI = &pstCFDInfo->stUI;
	pstPQ = &pstCFDInfo->stPQ;
	pstCFD = &pstCFDInfo->stCFD;
	u8InputSRC = pstIn->u8InputSRC;
	u8ColorRangeType = pstUI->u8ColorRangeType;
	bIsDV = pstCFDInfo->stDV.bIsDolby;
	bIsTCH = pstCFDInfo->stTCH.bIsTCH;
	bHDROff = (pstUI->u8HDREnableMode == V4L2_HDR_ENABLE_MODE_DISABLE);

	pstMainControl->u8Input_Source = u8InputSRC;
	pstMainControl->u8Input_Format = pstIn->stMainColorFmt.u8Fmt;

	if (CFD_IS_HDMI(u8InputSRC) && pstIn->stHDMI.u8IsGCInfoValid)
		pstMainControl->u8InputDataDepth =
					pstIn->stHDMI.u8ColorDepth;
	else if (CFD_IS_B2R(u8InputSRC)
		&& pstIn->stVDEC.u8IsGeneralInfoValid)
		pstMainControl->u8InputDataDepth =
					pstIn->stVDEC.stGeneral.u8BitDepth;
	else
		pstMainControl->u8InputDataDepth =
					pstIn->stMainColorFmt.u8BitDepth;

	pstMainControl->u8Input_DataFormat = pstIn->stMainColorFmt.u8DataFmt;

	if ((u8ColorRangeType == V4L2_CUS_COLOR_RANGE_TYPE_YUV_LIMIT)
	|| (u8ColorRangeType == V4L2_CUS_COLOR_RANGE_TYPE_RGB_LIMIT)) {
		pstMainControl->u8Input_IsFullRange = 0;
	} else if ((u8ColorRangeType == V4L2_CUS_COLOR_RANGE_TYPE_YUV_FULL)
	|| (u8ColorRangeType == V4L2_CUS_COLOR_RANGE_TYPE_RGB_FULL)) {
		pstMainControl->u8Input_IsFullRange = 1;
	} else {
		pstMainControl->u8Input_IsFullRange =
					pstIn->stMainColorFmt.bFullRange;
	}

	pstMainControl->u8Input_ext_Colour_primaries =
					pstIn->stMainColorFmt.u8CP;
	pstMainControl->u8Input_ext_Transfer_Characteristics =
					pstIn->stMainColorFmt.u8TC;
	pstMainControl->u8Input_ext_Matrix_Coeffs =
					pstIn->stMainColorFmt.u8MC;

	pstMainControl->u8Input_HDRMode = _mtk_cfd_HDRTypeCovert(bHDROff,
					bIsDV, bIsTCH, pstIn->u8HDRType);
	pstCFD->u8HdrStatus = pstMainControl->u8Input_HDRMode;

	pstMainControl->u8Input_IsRGBBypass = pstPQ->bPQBypass;
	pstMainControl->u8Input_SDRIPMode = 1;
	pstMainControl->u8Input_HDRIPMode = 1;

	return true;
}

static bool _mtk_cfd_SetMainParamOutput(
	struct st_cfd_info *pstCFDInfo,
	struct STU_CFDAPI_MAIN_CONTROL *pstMainControl)
{
	struct st_cfd_ui_info *pstUI;
	struct st_cfd_output_info *pstOut;

	if ((!pstCFDInfo) || (pstMainControl == NULL))
		return false;
	pstOut = &pstCFDInfo->stOut;
	pstUI = &pstCFDInfo->stUI;

	pstMainControl->u8Output_Source = E_CFD_OUTPUT_SOURCE_PANEL;
	pstMainControl->u8Output_Format = pstOut->stMainColorFmt.u8Fmt;
	pstMainControl->u8Output_DataFormat =
				pstOut->stMainColorFmt.u8DataFmt;
	pstMainControl->u8Output_IsFullRange =
				pstOut->stMainColorFmt.bFullRange;

	pstMainControl->u8Output_HDRMode = E_CFIO_MODE_SDR;

	if (pstMainControl->u8Input_HDRMode == E_CFIO_MODE_HDR1) {
		pstMainControl->u8PanelOutput_GammutMapping_Mode = 0;
	} else if ((pstMainControl->u8Input_HDRMode == E_CFIO_MODE_HDR2)
		|| (pstMainControl->u8Input_HDRMode == E_CFIO_MODE_HDR3)
		|| (pstMainControl->u8Input_HDRMode == E_CFIO_MODE_HDR5)) {
		pstMainControl->u8PanelOutput_GammutMapping_Mode =
						pstOut->bLinearRgb;
	} else {
		pstMainControl->u8PanelOutput_GammutMapping_Mode =
			pstOut->bLinearRgb && pstUI->bLinearEnable;
	}

	pstMainControl->u8HDMIOutput_GammutMapping_Mode = 0;
	pstMainControl->u8HDMIOutput_GammutMapping_MethodMode = 0;
	pstMainControl->u8MMInput_ColorimetryHandle_Mode = 0;
	pstMainControl->u8TMO_TargetRefer_Mode = 1;
	pstMainControl->u16Source_Max_Luminance = 4000;
	pstMainControl->u16Source_Med_Luminance = 120;
	pstMainControl->u16Source_Min_Luminance = 60;
	pstMainControl->u16Target_Med_Luminance = 100;
	pstMainControl->u16Target_Max_Luminance = 50;
	pstMainControl->u16Target_Min_Luminance = 500;

	return true;
}

__u8 _mtk_cfd_InputCovert(__u32 u32Input)
{
	__u8 u8CFDInput;

	switch (u32Input) {
	case MTK_PQ_INPUTSRC_NONE:
		u8CFDInput = E_CFD_INPUT_SOURCE_NONE;
		break;
	case MTK_PQ_INPUTSRC_VGA:
		u8CFDInput = E_CFD_INPUT_SOURCE_VGA;
		break;
	case MTK_PQ_INPUTSRC_TV:
		u8CFDInput = E_CFD_INPUT_SOURCE_TV;
		break;
	case MTK_PQ_INPUTSRC_CVBS:
	case MTK_PQ_INPUTSRC_CVBS2:
	case MTK_PQ_INPUTSRC_CVBS3:
	case MTK_PQ_INPUTSRC_CVBS4:
	case MTK_PQ_INPUTSRC_CVBS5:
	case MTK_PQ_INPUTSRC_CVBS6:
	case MTK_PQ_INPUTSRC_CVBS7:
	case MTK_PQ_INPUTSRC_CVBS8:
	case MTK_PQ_INPUTSRC_CVBS_MAX:
		u8CFDInput = E_CFD_INPUT_SOURCE_CVBS;
		break;
	case MTK_PQ_INPUTSRC_SVIDEO:
	case MTK_PQ_INPUTSRC_SVIDEO2:
	case MTK_PQ_INPUTSRC_SVIDEO3:
	case MTK_PQ_INPUTSRC_SVIDEO4:
	case MTK_PQ_INPUTSRC_SVIDEO_MAX:
		u8CFDInput = E_CFD_INPUT_SOURCE_SVIDEO;
		break;
	case MTK_PQ_INPUTSRC_YPBPR:
	case MTK_PQ_INPUTSRC_YPBPR2:
	case MTK_PQ_INPUTSRC_YPBPR3:
	case MTK_PQ_INPUTSRC_YPBPR_MAX:
		u8CFDInput = E_CFD_INPUT_SOURCE_YPBPR;
		break;
	case MTK_PQ_INPUTSRC_SCART:
	case MTK_PQ_INPUTSRC_SCART2:
	case MTK_PQ_INPUTSRC_SCART_MAX:
		u8CFDInput = E_CFD_INPUT_SOURCE_SCART;
		break;
	case MTK_PQ_INPUTSRC_HDMI:
	case MTK_PQ_INPUTSRC_HDMI2:
	case MTK_PQ_INPUTSRC_HDMI3:
	case MTK_PQ_INPUTSRC_HDMI4:
	case MTK_PQ_INPUTSRC_HDMI_MAX:
		u8CFDInput = E_CFD_INPUT_SOURCE_HDMI;
		break;
	case MTK_PQ_INPUTSRC_DVI:
	case MTK_PQ_INPUTSRC_DVI2:
	case MTK_PQ_INPUTSRC_DVI3:
	case MTK_PQ_INPUTSRC_DVI4:
	case MTK_PQ_INPUTSRC_DVI_MAX:
		u8CFDInput = E_CFD_INPUT_SOURCE_DVI;
		break;
	case MTK_PQ_INPUTSRC_DTV:
	case MTK_PQ_INPUTSRC_DTV2:
	case MTK_PQ_INPUTSRC_DTV_MAX:
		u8CFDInput = E_CFD_INPUT_SOURCE_DTV;
		break;
	case MTK_PQ_INPUTSRC_STORAGE:
	case MTK_PQ_INPUTSRC_STORAGE2:
	case MTK_PQ_INPUTSRC_STORAGE_MAX:
		u8CFDInput = E_CFD_INPUT_SOURCE_STORAGE;
		break;
	default:
		u8CFDInput = E_CFD_INPUT_SOURCE_NONE;
		break;
	}

	return u8CFDInput;
}

//------------------------------------------------------------------------------
//  Global Functions
//------------------------------------------------------------------------------
bool mtk_cfd_ResetCfdInfo(struct st_cfd_info *pstCfdInfo)
{
	struct st_cfd_ui_info *pstUI;
	struct st_cfd_input_info *pstIn;
	struct st_cfd_output_info *pstOut;
	struct st_xc_status_info *pstXC;
	struct st_pq_status_info *pstPQ;
	struct st_dv_status_info *pstDV;
	struct st_tch_status_info *pstTCH;
	struct st_cfd_status_info *pstCFD;

	if (pstCfdInfo == NULL)
		return false;
	memset(pstCfdInfo, 0, sizeof(struct st_cfd_info));
	pstUI = &pstCfdInfo->stUI;
	pstIn = &pstCfdInfo->stIn;
	pstOut = &pstCfdInfo->stOut;
	pstXC = &pstCfdInfo->stXC;
	pstPQ = &pstCfdInfo->stPQ;
	pstDV = &pstCfdInfo->stDV;
	pstTCH = &pstCfdInfo->stTCH;
	pstCFD = &pstCfdInfo->stCFD;

	pstIn->u8InputSRC = E_CFD_INPUT_SOURCE_NONE;
	pstIn->u8SRCDataFmt = E_CFD_MC_FORMAT_YUV444;

	pstIn->stMainColorFmt.u8Fmt = E_CFD_CFIO_YUV_NOTSPECIFIED;
	pstIn->stMainColorFmt.u8DataFmt = E_CFD_MC_FORMAT_YUV444;
	pstIn->stMainColorFmt.u8CP = E_CFD_CFIO_CP_UNSPECIFIED;
	pstIn->stMainColorFmt.u8TC = E_CFD_CFIO_TR_UNSPECIFIED;
	pstIn->stMainColorFmt.u8MC = E_CFD_CFIO_MC_UNSPECIFIED;
	pstIn->stMainColorFmt.u8BitDepth = 8;
	pstIn->stMainColorFmt.bFullRange = false;

	pstIn->u8HDRType = V4L2_HDR_TYPE_NONE;

	pstIn->stHDMI.u8IsGCInfoValid = false;
	pstIn->stHDMI.u8IsAVIValid = false;
	pstIn->stHDMI.u8IsHDRInfoValid = false;
	pstIn->stHDMI.u8IsVSIF10PlusDataValid = false;
	pstIn->stHDMI.u8IsVSIFDVMeatdataValid = false;
	pstIn->stHDMI.u8IsEMP10PlusDataValid = false;
	pstIn->stHDMI.u8IsEMPDVMeatdataValid = false;

	pstIn->stVDEC.u8IsGeneralInfoValid = false;
	pstIn->stVDEC.u8IsVUIValid = false;
	pstIn->stVDEC.u8IsSEIValid = false;
	pstIn->stVDEC.u8IsCLLValid = false;
	pstIn->stVDEC.u8IsFilmGrainValid = false;
	pstIn->stVDEC.u8IsHDR10PlusSEIValid = false;
	pstIn->stVDEC.u8IsDVMetadataValid = false;
	pstIn->stVDEC.u8IsTCHMetadataValid = false;

	pstOut->stMainColorFmt.u8Fmt = E_CFD_CFIO_RGB_BT709;
	pstOut->stMainColorFmt.u8DataFmt = E_CFD_MC_FORMAT_RGB;
	pstOut->stMainColorFmt.u8CP = E_CFD_CFIO_CP_UNSPECIFIED;
	pstOut->stMainColorFmt.u8TC = E_CFD_CFIO_TR_UNSPECIFIED;
	pstOut->stMainColorFmt.u8MC = E_CFD_CFIO_MC_UNSPECIFIED;
	pstOut->stMainColorFmt.u8BitDepth = 8;
	pstOut->stMainColorFmt.bFullRange = true;

	pstOut->stCP.u16Display_Primaries_x[0] = 32000;
	pstOut->stCP.u16Display_Primaries_x[1] = 15000;
	pstOut->stCP.u16Display_Primaries_x[2] = 7500;
	pstOut->stCP.u16Display_Primaries_y[0] = 16455;
	pstOut->stCP.u16Display_Primaries_y[1] = 30000;
	pstOut->stCP.u16Display_Primaries_y[2] = 3000;
	pstOut->stCP.u16White_point_x = 15635;
	pstOut->stCP.u16White_point_y = 16450;
	pstOut->u16MaxLuminance = 100;
	pstOut->u16MedLuminance = 50;
	pstOut->u16MinLuminance = 500;
	pstOut->bLinearRgb = true;
	pstOut->bCusCP = false;

	pstUI->u8OSDUIEn = 1;
	pstUI->u8OSDUIMode = 0;
	pstUI->u8HDR_UI_H2SMode = 0;
	pstUI->bSkipPictureSetting = false;
	pstUI->u16Hue = 50;
	pstUI->u16Saturation = 128;
	pstUI->u16Contrast = 1024;
	pstUI->au16Brightness[0] = 0;
	pstUI->au16Brightness[1] = 0;
	pstUI->au16Brightness[2] = 0;
	pstUI->au16RGBGain[0] = 0;
	pstUI->au16RGBGain[1] = 0;
	pstUI->au16RGBGain[2] = 0;
	pstUI->u8ColorRangeType = V4L2_CUS_COLOR_RANGE_TYPE_AUTO;
	pstUI->u8UltraBlackLevel = 0;
	pstUI->u8UltraWhiteLevel = 0;
	pstUI->bColorCorrectionEnable = false;
	pstUI->bLinearEnable = false;
	pstUI->u8HDREnableMode = V4L2_HDR_ENABLE_MODE_AUTO;
	pstUI->u8TmoLevel = V4L2_TMO_LEVEL_TYPE_MID;

	pstXC->bIsInputImode = false;
	pstXC->bPreVSDEnable = false;
	pstXC->bHVSPEnable = false;
	pstXC->u8MADIDramFormat = E_CFD_MC_FORMAT_YUV444;

	pstPQ->bPQBypass = false;

	pstDV->bIsDolby = false;
	pstDV->bSkipUI = false;

	pstTCH->bIsTCH = false;

	pstCFD->u8HdrStatus = E_CFIO_MODE_SDR;
	pstCFD->u8HdrPosition = 0;	//HDR IN IP
	pstCFD->bCFDFired = false;

	return true;
}

bool mtk_cfd_SetInputSource(struct st_cfd_info *pstCFDInfo,
	__u8 u8Input)
{
	__u8 u8CFDInput;

	if (!pstCFDInfo)
		return false;

	u8CFDInput = (enum EN_CFD_INPUT_SOURCE)u8Input;

	if (u8CFDInput > E_CFD_INPUT_SOURCE_NONE)
		return false;

	if (CFD_IS_NONE(u8CFDInput)) {
		mtk_cfd_ResetCfdInfo(pstCFDInfo);
		pstCFDInfo->u32UpdataFlag = 0;
	}

	pstCFDInfo->stIn.u8InputSRC = u8CFDInput;

	return true;
}

bool mtk_cfd_SetSRCDataFmt(struct st_cfd_info *pstCFDInfo,
	__u8 u8SRCDataFmt)
{
	struct st_cfd_input_info *pstIn;

	if (!pstCFDInfo)
		return false;

	if (u8SRCDataFmt >= E_CFD_MC_FORMAT_RESERVED_START)
		return false;

	pstIn = &pstCFDInfo->stIn;

	if (u8SRCDataFmt != pstIn->u8SRCDataFmt) {
		pstCFDInfo->stIn.u8SRCDataFmt = u8SRCDataFmt;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);
	}

	return true;
}

bool mtk_cfd_SetInputMainFmt(struct st_cfd_info *pstCFDInfo,
	struct st_main_color_fmt *pstFmt)
{
	struct st_main_color_fmt *pstInMainFmt;

	if ((pstFmt == NULL) || (!pstCFDInfo))
		return false;
	pstInMainFmt = &pstCFDInfo->stIn.stMainColorFmt;

	if ((pstFmt->u8Fmt >= E_CFD_CFIO_RESERVED_START)
	|| (pstFmt->u8DataFmt >= E_CFD_MC_FORMAT_RESERVED_START)
	|| (pstFmt->u8CP >= E_CFD_CFIO_CP_RESERVED_START)
	|| (pstFmt->u8TC >= E_CFD_CFIO_TR_RESERVED_START)
	|| (pstFmt->u8MC >= E_CFD_CFIO_MC_RESERVED_START)
	|| (pstFmt->u8BitDepth == 0))
		return false;

	if ((pstFmt->u8Fmt != pstInMainFmt->u8Fmt)
	|| (pstFmt->u8CP != pstInMainFmt->u8CP)
	|| (pstFmt->u8TC != pstInMainFmt->u8TC)
	|| (pstFmt->u8MC != pstInMainFmt->u8MC)
	|| (pstFmt->u8BitDepth != pstInMainFmt->u8BitDepth)
	|| (pstFmt->bFullRange != pstInMainFmt->bFullRange)) {
		pstInMainFmt->u8Fmt = pstFmt->u8Fmt;
		pstInMainFmt->u8CP = pstFmt->u8CP;
		pstInMainFmt->u8TC = pstFmt->u8TC;
		pstInMainFmt->u8MC = pstFmt->u8MC;
		pstInMainFmt->u8BitDepth = pstFmt->u8BitDepth;
		pstInMainFmt->bFullRange = pstFmt->bFullRange;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
	}

	if (pstFmt->u8DataFmt != pstInMainFmt->u8DataFmt) {
		pstInMainFmt->u8DataFmt = pstFmt->u8DataFmt;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);
	}

	return true;
}

bool mtk_cfd_SetHDMIMetaData(struct st_cfd_info *pstCFDInfo,
	void __user *FD)
{
	bool bNeedUpdataHdr = false;
	struct st_cfd_hdmi_info stHDMI;
	struct st_cfd_input_info *pstCFDInput;
	struct st_main_color_fmt *pstFMT;
	struct st_cfd_hdmi_info *pstHDMI;
	struct st_vsif_10plus_info *pstVSIF10Plus;
	struct st_emp_10plus_info *pstEMP10Plus;

	if (!pstCFDInfo)
		return false;
	if (!CFD_IS_HDMI(pstCFDInfo->stIn.u8InputSRC))
		return false;
	pstCFDInput = &pstCFDInfo->stIn;
	pstFMT = &pstCFDInfo->stIn.stMainColorFmt;
	pstHDMI = &pstCFDInfo->stIn.stHDMI;
	pstVSIF10Plus = &pstCFDInfo->stIn.stVSIF10Plus;
	pstEMP10Plus = &pstCFDInfo->stIn.stEMP10Plus;

	memset(&stHDMI, 0, sizeof(struct st_cfd_hdmi_info));
	//TODO use FD and tag "HDMI_CFDINFO" to get hdmi metadata
	if (stHDMI.u8IsGCInfoValid) {
		if (stHDMI.u8ColorDepth != pstHDMI->u8ColorDepth) {
			pstHDMI->u8IsGCInfoValid = true;
			pstHDMI->u8ColorDepth = stHDMI.u8ColorDepth;
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
		}
	} else {
		pstHDMI->u8IsGCInfoValid = false;
		pstHDMI->u8ColorDepth = 0;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
	}

	if (stHDMI.u8IsAVIValid) {
		if (memcmp(&stHDMI.stAVI, &pstHDMI->stAVI,
			sizeof(struct st_hdmi_avi_packet))) {
			pstHDMI->u8IsAVIValid = true;
			memcpy(&pstHDMI->stAVI, &stHDMI.stAVI,
				sizeof(struct st_hdmi_avi_packet));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_420CTRL);
		}
	} else {
		if (pstHDMI->u8IsAVIValid) {
			pstHDMI->u8IsAVIValid = false;
			memset(&pstHDMI->stAVI, 0,
				sizeof(struct st_hdmi_avi_packet));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_420CTRL);
		}
	}

	if (stHDMI.u8IsHDRInfoValid) {
		if (memcmp(&stHDMI.stHDRInfo, &pstHDMI->stHDRInfo,
			sizeof(struct st_hdmi_hdr_packet))) {
			pstHDMI->u8IsHDRInfoValid = true;
			memcpy(&pstHDMI->stHDRInfo, &stHDMI.stHDRInfo,
				sizeof(struct st_hdmi_hdr_packet));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_HDMI);
			bNeedUpdataHdr = true;
		}
	} else {
		if (pstHDMI->u8IsHDRInfoValid) {
			pstHDMI->u8IsHDRInfoValid = false;
			memset(&pstHDMI->stHDRInfo, 0,
				sizeof(struct st_hdmi_hdr_packet));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_HDMI);
			bNeedUpdataHdr = true;
		}
	}

	if (stHDMI.u8IsVSIF10PlusDataValid) {
		if (pstHDMI->u8IsVSIF10PlusDataValid == false)
			bNeedUpdataHdr = true;
		pstHDMI->u8IsVSIF10PlusDataValid = false;
		//use FD and tag "HDMI_VSIF10+" to get HDR10+ metadata
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_HDMI);
	} else {
		if (pstHDMI->u8IsVSIF10PlusDataValid) {
			pstHDMI->u8IsVSIF10PlusDataValid = false;
			memset(pstVSIF10Plus, 0,
				sizeof(struct st_vsif_10plus_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_HDMI);
			bNeedUpdataHdr = true;
		}
	}

	if (stHDMI.u8IsEMP10PlusDataValid) {
		if (pstHDMI->u8IsEMP10PlusDataValid == false)
			bNeedUpdataHdr = true;
		pstHDMI->u8IsEMP10PlusDataValid = true;
		//use FD and tag "HDMI_EMP10+" to get HDR10+ metadata
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_HDMI);
	} else {
		if (pstHDMI->u8IsEMP10PlusDataValid) {
			pstHDMI->u8IsEMP10PlusDataValid = false;
			memset(pstEMP10Plus, 0,
				sizeof(struct st_emp_10plus_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_HDMI);
			bNeedUpdataHdr = true;
		}
	}

	if (bNeedUpdataHdr)
		_mtk_cfd_CovertEOTF2HDRType(&pstCFDInput->u8HDRType,
					pstHDMI->stHDRInfo.u8EOTF,
					pstHDMI->u8IsVSIF10PlusDataValid,
					pstHDMI->u8IsEMP10PlusDataValid);

	return true;
}

bool mtk_cfd_SetMMMetaData(struct st_cfd_info *pstCFDInfo,
	void __user *FD)
{
	struct st_cfd_mm_info stVDEC;
	struct st_cfd_input_info *pstCFDInput;
	struct st_main_color_fmt *pstFMT;
	struct st_cfd_mm_info *pstVDEC;
	struct st_fg_info *pstFG;
	struct st_hdr10plus_sei *pstSEI10Plus;

	if (!pstCFDInfo)
		return false;
	pstCFDInput = &pstCFDInfo->stIn;
	pstFMT = &pstCFDInfo->stIn.stMainColorFmt;
	pstVDEC = &pstCFDInfo->stIn.stVDEC;
	pstSEI10Plus = &pstCFDInfo->stIn.stSEI10Plus;
	pstFG = &pstCFDInfo->stIn.stFG;

	memset(&stVDEC, 0, sizeof(struct st_cfd_mm_info));
	//use FD and tag "VDEC_CFDVDEC" to get vdec metadata
	if (stVDEC.u8IsGeneralInfoValid) {
		if (memcmp(&stVDEC.stGeneral, &pstVDEC->stGeneral,
			sizeof(struct st_mm_general_info))) {
			pstVDEC->u8IsGeneralInfoValid = true;
			memcpy(&pstVDEC->stGeneral, &stVDEC.stGeneral,
				sizeof(struct st_mm_general_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_420CTRL);
			pstCFDInput->u8HDRType = pstVDEC->stGeneral.u8HdrType;
		}
	} else {
		if (pstVDEC->u8IsGeneralInfoValid) {
			pstVDEC->u8IsGeneralInfoValid = false;
			memset(&pstVDEC->stGeneral, 0,
				sizeof(struct st_mm_general_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_420CTRL);
			pstCFDInput->u8HDRType = V4L2_HDR_TYPE_NONE;
		}
	}

	if (stVDEC.u8IsVUIValid) {
		if (memcmp(&stVDEC.stUVI, &pstVDEC->stUVI,
			sizeof(struct st_vui_info))) {
			pstVDEC->u8IsVUIValid = true;
			memcpy(&pstVDEC->stUVI, &stVDEC.stUVI,
				sizeof(struct st_vui_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MM);
		}
	} else {
		if (pstVDEC->u8IsVUIValid) {
			pstVDEC->u8IsVUIValid = false;
			memset(&pstVDEC->stUVI, 0,
				sizeof(struct st_vui_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MAIN);
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MM);
		}
	}

	if (stVDEC.u8IsSEIValid) {
		if (memcmp(&stVDEC.stSEI, &pstVDEC->stSEI,
			sizeof(struct st_sei_info))) {
			pstVDEC->u8IsSEIValid = true;
			memcpy(&pstVDEC->stSEI, &stVDEC.stSEI,
				sizeof(struct st_sei_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MM);
		}
	} else {
		if (pstVDEC->u8IsSEIValid) {
			pstVDEC->u8IsSEIValid = false;
			memset(&pstVDEC->stSEI, 0,
				sizeof(struct st_sei_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MM);
		}
	}

	if (stVDEC.u8IsCLLValid) {
		if (memcmp(&stVDEC.stCLL, &pstVDEC->stCLL,
			sizeof(struct st_cll_info))) {
			pstVDEC->u8IsCLLValid = true;
			memcpy(&pstVDEC->stCLL, &stVDEC.stCLL,
				sizeof(struct st_cll_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MM);
		}
	} else {
		if (pstVDEC->u8IsCLLValid) {
			pstVDEC->u8IsCLLValid = false;
			memset(&pstVDEC->stCLL, 0,
				sizeof(struct st_cll_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MM);
		}
	}

	if (stVDEC.u8IsHDR10PlusSEIValid) {
		pstVDEC->u8IsHDR10PlusSEIValid = true;
		//use FD and tag "VDEC_HDR10+" to get HDR10+ metadata
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_MM);
	} else {
		if (pstVDEC->u8IsHDR10PlusSEIValid) {
			pstVDEC->u8IsHDR10PlusSEIValid = false;
			memset(pstSEI10Plus, 0,
				sizeof(struct st_hdr10plus_sei));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_MM);
		}
	}

	if (stVDEC.u8IsFilmGrainValid) {
		pstVDEC->u8IsFilmGrainValid = true;
		//pstFG :use FD and tag "VDEC_FG" to get FG metadata
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_FG);
	} else {
		if (pstVDEC->u8IsFilmGrainValid) {
			pstVDEC->u8IsFilmGrainValid = false;
			memset(pstFG, 0, sizeof(struct st_fg_info));
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_FG);
		}
	}

	return true;
}

bool mtk_cfd_SetOutputMainFmt(struct st_cfd_info *pstCFDInfo,
	struct st_main_color_fmt *pstFmt)
{
	struct st_main_color_fmt *pstOutMainFmt;

	if ((pstFmt == NULL) || (!pstCFDInfo))
		return false;
	pstOutMainFmt = &pstCFDInfo->stOut.stMainColorFmt;

	if ((pstFmt->u8Fmt >= E_CFD_CFIO_RESERVED_START)
	|| (pstFmt->u8DataFmt >= E_CFD_MC_FORMAT_RESERVED_START)
	|| (pstFmt->u8CP >= E_CFD_CFIO_CP_RESERVED_START)
	|| (pstFmt->u8TC >= E_CFD_CFIO_TR_RESERVED_START)
	|| (pstFmt->u8MC >= E_CFD_CFIO_MC_RESERVED_START)
	|| (pstFmt->u8BitDepth == 0))
		return false;

	if (memcmp(pstOutMainFmt, pstFmt, sizeof(struct st_main_color_fmt))) {
		memcpy(pstOutMainFmt, pstFmt, sizeof(struct st_main_color_fmt));
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);
	}

	return true;
}

bool mtk_cfd_SetOutputCP(struct st_cfd_info *pstCFDInfo,
	struct st_color_primaries *pstCP)
{
	struct st_color_primaries *pstOutCP;

	if (pstCP == NULL)
		return false;

	if (!pstCFDInfo)
		return false;
	pstOutCP = &pstCFDInfo->stOut.stCP;

	if (memcmp(pstOutCP, pstCP, sizeof(struct st_color_primaries))) {
		memcpy(pstOutCP, pstCP, sizeof(struct st_color_primaries));
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_OUTPUT);
	}

	return true;
}

bool mtk_cfd_SetOutputluma(struct st_cfd_info *pstCFDInfo,
	__u16 u16MaxLuminance, __u16 u16MedLuminance, __u16 u16MinLuminance)
{
	struct st_cfd_output_info *pstOut;

	if (!pstCFDInfo)
		return false;
	pstOut = &pstCFDInfo->stOut;

	if ((pstOut->u16MaxLuminance != u16MaxLuminance)
	|| (pstOut->u16MedLuminance != u16MedLuminance)
	|| (pstOut->u16MinLuminance != u16MinLuminance)) {
		pstOut->u16MaxLuminance = u16MaxLuminance;
		pstOut->u16MedLuminance = u16MedLuminance;
		pstOut->u16MinLuminance = u16MinLuminance;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_OUTPUT);
	}

	return true;
}

bool mtk_cfd_SetOutputLinear(struct st_cfd_info *pstCFDInfo,
	bool bLinearRGB)
{
	struct st_cfd_output_info *pstOut;

	if (!pstCFDInfo)
		return false;
	pstOut = &pstCFDInfo->stOut;

	if (pstOut->bLinearRgb != bLinearRGB) {
		pstOut->bLinearRgb = bLinearRGB;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
	}

	return true;
}

bool mtk_cfd_SetOutputCusCP(struct st_cfd_info *pstCFDInfo,
	bool bCusCP, __u16 u16WhitePointX, __u16 u16WhitePointY)
{
	struct st_cfd_output_info *pstOut;

	if (!pstCFDInfo)
		return false;
	pstOut = &pstCFDInfo->stOut;

	if ((pstOut->bCusCP != bCusCP)
	|| (pstOut->u16SourceWx != u16WhitePointX)
	|| (pstOut->u16SourceWy != u16WhitePointY)) {
		pstOut->bCusCP = bCusCP;
		pstOut->u16SourceWx = u16WhitePointX;
		pstOut->u16SourceWy = u16WhitePointY;
		// TODO
	}

	return true;
}

bool mtk_cfd_SetUISkipPicSetting(struct st_cfd_info *pstCFDInfo,
	bool bSkip)
{
	struct st_cfd_ui_info *pstUI;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if (pstUI->bSkipPictureSetting != bSkip) {
		pstUI->bSkipPictureSetting = bSkip;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_UI);
	}

	return true;
}

bool mtk_cfd_SetUIHue(struct st_cfd_info *pstCFDInfo,
	__u16 u16Hue)
{
	struct st_cfd_ui_info *pstUI;

	if (u16Hue > 100)
		return false;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if (pstUI->u16Hue != u16Hue) {
		pstUI->u16Hue = u16Hue;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_UI);
	}

	return true;
}

bool mtk_cfd_SetUISaturation(struct st_cfd_info *pstCFDInfo,
	__u16 u16Saturation)
{
	struct st_cfd_ui_info *pstUI;

	if (u16Saturation > 255)
		return false;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if (pstUI->u16Saturation != u16Saturation) {
		pstUI->u16Saturation = u16Saturation;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_UI);
	}

	return true;
}

bool mtk_cfd_SetUIContrast(struct st_cfd_info *pstCFDInfo,
	__u16 u16Contrast)
{
	struct st_cfd_ui_info *pstUI;

	if (u16Contrast > 2047)
		return false;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if (pstUI->u16Contrast != u16Contrast) {
		pstUI->u16Contrast = u16Contrast;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_UI);
	}

	return true;
}

bool mtk_cfd_SetUIBrightness(struct st_cfd_info *pstCFDInfo,
	__u16 u16BrightnessR, __u16 u16BrightnessG, __u16 u16BrightnessB)
{
	struct st_cfd_ui_info *pstUI;

	if ((u16BrightnessR > 2047)
	|| (u16BrightnessG > 2047)
	|| (u16BrightnessB > 2047))
		return false;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if ((pstUI->au16Brightness[0] != u16BrightnessR)
	|| (pstUI->au16Brightness[1] != u16BrightnessG)
	|| (pstUI->au16Brightness[2] != u16BrightnessB)) {
		pstUI->au16Brightness[0] = u16BrightnessR;
		pstUI->au16Brightness[1] = u16BrightnessG;
		pstUI->au16Brightness[2] = u16BrightnessB;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_UI);
	}

	return true;
}

bool mtk_cfd_SetUIRGBGain(struct st_cfd_info *pstCFDInfo,
	__u16 u16GainR, __u16 u16GainG, __u16 u16GainB)
{
	struct st_cfd_ui_info *pstUI;

	if ((u16GainR > 2048)
	|| (u16GainG > 2048)
	|| (u16GainB > 2048))
		return false;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if ((pstUI->au16RGBGain[0] != u16GainR)
	|| (pstUI->au16RGBGain[1] != u16GainG)
	|| (pstUI->au16RGBGain[2] != u16GainB)) {
		pstUI->au16RGBGain[0] = u16GainR;
		pstUI->au16RGBGain[1] = u16GainG;
		pstUI->au16RGBGain[2] = u16GainB;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_UI);
	}

	return true;
}

bool mtk_cfd_SetUIColorRange(struct st_cfd_info *pstCFDInfo,
	__u8 u8ColorRangeType, __u8 u8WhiteLevel, __u8 u8BlackLevel)
{
	struct st_cfd_ui_info *pstUI;

	if ((u8ColorRangeType > V4L2_CUS_COLOR_RANGE_TYPE_ADJ)
	|| (u8WhiteLevel > 64)
	|| (u8BlackLevel > 64))
		return false;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if (pstUI->u8ColorRangeType != u8ColorRangeType) {
		pstUI->u8ColorRangeType = u8ColorRangeType;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
	}

	if ((pstUI->u8ColorRangeType == V4L2_CUS_COLOR_RANGE_TYPE_ADJ)
	&& ((pstUI->u8UltraBlackLevel != u8BlackLevel)
	|| (pstUI->u8UltraBlackLevel != u8WhiteLevel))) {
		pstUI->u8UltraBlackLevel = u8BlackLevel;
		pstUI->u8UltraWhiteLevel = u8WhiteLevel;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_BLACK_WHITE);
	}

	return true;
}

bool mtk_cfd_SetUIColorCorrection(struct st_cfd_info *pstCFDInfo,
	bool bEnable, __s16 *ps16Matries, __u16 u16MatriesNum)
{
	struct st_cfd_ui_info *pstUI;
	__u8 u8Cnt;

	if (bEnable) {
		if (ps16Matries == NULL)
			return false;
		if ((u16MatriesNum > 32) || (u16MatriesNum < 9))
			return false;
		for (u8Cnt = 0; u8Cnt < u16MatriesNum; u8Cnt++) {
			if ((ps16Matries[u8Cnt] > 2047)
			|| (ps16Matries[u8Cnt] < -2048))
				return false;
		}
	}

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if (pstUI->bColorCorrectionEnable != bEnable) {
		pstUI->bColorCorrectionEnable = bEnable;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_UI);
	}

	if ((bEnable && memcmp(pstUI->as16CorrectionMatrix, ps16Matries,
		sizeof(__s16) * u16MatriesNum))) {
		memcpy(pstUI->as16CorrectionMatrix, ps16Matries,
			sizeof(__s16) * u16MatriesNum);
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_UI);
	}

	return true;
}

bool mtk_cfd_SetUILinear(struct st_cfd_info *pstCFDInfo,
	bool bLinear)
{
	struct st_cfd_ui_info *pstUI;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if (pstUI->bLinearEnable != bLinear) {
		pstUI->bLinearEnable = bLinear;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
	}

	return true;
}

bool mtk_cfd_SetUIHDR(struct st_cfd_info *pstCFDInfo,
	__u8 u8HDREnableMode)
{
	struct st_cfd_ui_info *pstUI;

	if (pstCFDInfo == NULL)
		return false;

	if (u8HDREnableMode > V4L2_HDR_ENABLE_MODE_DISABLE)
		return false;

	pstUI = &pstCFDInfo->stUI;

	if (pstUI->u8HDREnableMode != u8HDREnableMode) {
		pstUI->u8HDREnableMode = u8HDREnableMode;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_TMO);
	}

	return true;
}

bool mtk_cfd_SetUITMOLevel(struct st_cfd_info *pstCFDInfo,
	__u8 u8TMOLevel)
{
	struct st_cfd_ui_info *pstUI;

	if (u8TMOLevel > V4L2_TMO_LEVEL_TYPE_HIGH)
		return false;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;

	if (pstUI->u8TmoLevel != u8TMOLevel) {
		pstUI->u8TmoLevel = u8TMOLevel;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_TMO);
	}

	return true;
}

bool mtk_cfd_SetXCStatusIMode(struct st_cfd_info *pstCFDInfo,
	bool bIMode)
{
	struct st_xc_status_info *pstXC;

	if (!pstCFDInfo)
		return false;
	pstXC = &pstCFDInfo->stXC;

	if (pstXC->bIsInputImode != bIMode) {
		pstXC->bIsInputImode = bIMode;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);
	}

	return true;
}

bool mtk_cfd_SetXCStatusPreVSD(struct st_cfd_info *pstCFDInfo,
	bool bPreVSD)
{
	struct st_xc_status_info *pstXC;

	if (!pstCFDInfo)
		return false;
	pstXC = &pstCFDInfo->stXC;

	if (pstXC->bPreVSDEnable != bPreVSD) {
		pstXC->bPreVSDEnable = bPreVSD;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);
	}

	return true;
}

bool mtk_cfd_SetXCStatusMADIDataFmt(
	struct st_cfd_info *pstCFDInfo, __u8 u8Fmt)
{
	struct st_xc_status_info *pstXC;

	if (u8Fmt > E_CFD_MC_FORMAT_YUV420)
		return false;

	if (!pstCFDInfo)
		return false;
	pstXC = &pstCFDInfo->stXC;

	if (pstXC->u8MADIDramFormat != u8Fmt) {
		pstXC->u8MADIDramFormat = u8Fmt;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);
	}

	return true;
}

bool mtk_cfd_SetXCStatusHVSP(struct st_cfd_info *pstCFDInfo,
	bool bHVSP)
{
	struct st_xc_status_info *pstXC;

	if (!pstCFDInfo)
		return false;
	pstXC = &pstCFDInfo->stXC;

	if (pstXC->bHVSPEnable != bHVSP) {
		pstXC->bHVSPEnable = bHVSP;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);
	}

	return true;
}

bool mtk_cfd_SetPQStatusBypass(struct st_cfd_info *pstCFDInfo,
	bool bPQBypass)
{
	struct st_pq_status_info *pstPQ;

	if (!pstCFDInfo)
		return false;
	pstPQ = &pstCFDInfo->stPQ;

	if (pstPQ->bPQBypass != bPQBypass) {
		pstPQ->bPQBypass = bPQBypass;
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
	}

	return true;
}

bool mtk_cfd_SetDVStatusFlag(struct st_cfd_info *pstCFDInfo,
	bool bIsDolby)
{
	struct st_cfd_ui_info *pstUI;
	struct st_dv_status_info *pstDV;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;
	pstDV = &pstCFDInfo->stDV;

	if (pstDV->bIsDolby != bIsDolby) {
		pstDV->bIsDolby = bIsDolby;
		// for dolby SDK/IDK, skip UI
		if (bIsDolby && pstDV->bSkipUI) {
			pstUI->u8OSDUIEn = 0;
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_UI);
		}
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_DOLBY_HDR);
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_TMO);
	}

	return true;
}

bool mtk_cfd_SetDVStatusSkipUI(struct st_cfd_info *pstCFDInfo,
	bool bSkipUI)
{
	struct st_cfd_ui_info *pstUI;
	struct st_dv_status_info *pstDV;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;
	pstDV = &pstCFDInfo->stDV;

	if (pstDV->bSkipUI != bSkipUI) {
		pstDV->bSkipUI = bSkipUI;
		// for dolby SDK/IDK, skip UI
		if (bSkipUI && pstDV->bIsDolby) {
			pstUI->u8OSDUIEn = 0;
			_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
				CFD_PARAM_BIT_UI);
		}
	}

	return true;
}

bool mtk_cfd_SetTCHStatusFlag(struct st_cfd_info *pstCFDInfo,
	bool bIsTCH)
{
	struct st_tch_status_info *pstTCH;

	if (!pstCFDInfo)
		return false;
	pstTCH = &pstCFDInfo->stTCH;

	if (pstTCH->bIsTCH != bIsTCH) {
		pstTCH->bIsTCH = bIsTCH;
		// TODO
		//mtk_cfd_SetMMMetaData
		//mtk_cfd_SetInputMainFmt
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);
		_mtk_cfd_SetFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MM);
	}

	return true;
}

bool mtk_cfd_SetMainParam(struct st_cfd_info *pstCFDInfo)
{
	struct STU_CFDAPI_MAIN_CONTROL stMainControl;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;

	memset(&stMainControl, 0, sizeof(struct STU_CFDAPI_MAIN_CONTROL));
	stMainControl.u32Version = CFD_MAIN_CONTROL_ST_VERSION;
	stMainControl.u16Length = sizeof(struct STU_CFDAPI_MAIN_CONTROL);

	if (!_mtk_cfd_SetMainParamInput(pstCFDInfo, &stMainControl))
		return false;
	if (!_mtk_cfd_SetMainParamOutput(pstCFDInfo, &stMainControl))
		return false;

	// TODO
	//bRet = MDrv_PQU_CFD_Wrapper_SetMainControl(u8Win, &stMainControl);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MAIN);

	return bRet;
}

bool mtk_cfd_SetHDMIParam(struct st_cfd_info *pstCFDInfo)
{
	struct STU_CFDAPI_HDMI_INFOFRAME_PARSER stHDMIInfo;
	struct st_cfd_hdmi_info *pstHDMI;
	struct st_cfd_input_info *pstIn;
	struct st_vsif_10plus_info *pstVSIF10Plus;
	struct st_emp_10plus_info *pstEMP10Plus;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;
	pstIn = &pstCFDInfo->stIn;
	pstHDMI = &pstCFDInfo->stIn.stHDMI;
	pstVSIF10Plus = &pstCFDInfo->stIn.stVSIF10Plus;
	pstEMP10Plus = &pstCFDInfo->stIn.stEMP10Plus;

	stHDMIInfo.u32Version = CFD_HDMI_INFOFRAME_ST_VERSION;
	stHDMIInfo.u16Length = sizeof(struct STU_CFDAPI_HDMI_INFOFRAME_PARSER);
	stHDMIInfo.u8HDMISource_HDR_InfoFrame_Valid = pstHDMI->u8IsHDRInfoValid;
	stHDMIInfo.u8HDMISource_EOTF = pstHDMI->stHDRInfo.u8EOTF;
	stHDMIInfo.u8HDMISource_SMD_ID = pstHDMI->stHDRInfo.u8StaticMetadataID;
	stHDMIInfo.u16Master_Panel_Max_Luminance =
		pstHDMI->stHDRInfo.u16MaxDisplayMasteringLuminance;
	stHDMIInfo.u16Master_Panel_Min_Luminance =
		pstHDMI->stHDRInfo.u16MinDisplayMasteringLuminance;
	stHDMIInfo.u16Max_Content_Light_Level =
		pstHDMI->stHDRInfo.u16MaximumContentLightLevel;
	stHDMIInfo.u16Max_Frame_Avg_Light_Level =
		pstHDMI->stHDRInfo.u16MaximumFrameAverageLightLevel;
	stHDMIInfo.stu_Cfd_MasterPanel_ColorMetry.u16Display_Primaries_x[0] =
		pstHDMI->stHDRInfo.u16DisplayPrimariesX[0];
	stHDMIInfo.stu_Cfd_MasterPanel_ColorMetry.u16Display_Primaries_x[1] =
		pstHDMI->stHDRInfo.u16DisplayPrimariesX[1];
	stHDMIInfo.stu_Cfd_MasterPanel_ColorMetry.u16Display_Primaries_x[2] =
		pstHDMI->stHDRInfo.u16DisplayPrimariesX[2];
	stHDMIInfo.stu_Cfd_MasterPanel_ColorMetry.u16Display_Primaries_y[0] =
		pstHDMI->stHDRInfo.u16DisplayPrimariesY[0];
	stHDMIInfo.stu_Cfd_MasterPanel_ColorMetry.u16Display_Primaries_y[1] =
		pstHDMI->stHDRInfo.u16DisplayPrimariesY[1];
	stHDMIInfo.stu_Cfd_MasterPanel_ColorMetry.u16Display_Primaries_y[2] =
		pstHDMI->stHDRInfo.u16DisplayPrimariesY[2];
	stHDMIInfo.stu_Cfd_MasterPanel_ColorMetry.u16White_point_x =
		pstHDMI->stHDRInfo.u16WhitePointX;
	stHDMIInfo.stu_Cfd_MasterPanel_ColorMetry.u16White_point_y =
		pstHDMI->stHDRInfo.u16WhitePointY;
	stHDMIInfo.u8Mastering_Display_Infor_Valid =
		(pstHDMI->stHDRInfo.u8StaticMetadataID == 0) ? 1 : 0;
	stHDMIInfo.u8HDMISource_Support_Format =
		(pstHDMI->stAVI.u8RgbQuantizationRange << 5)
		| (pstHDMI->stAVI.u8YccQuantizationRange << 3)
		| (pstHDMI->stAVI.u8PixelFormat);
	_mtk_cfd_CalcColorFmt(&stHDMIInfo.u8HDMISource_Colorspace,
		pstHDMI->stAVI.u8PixelFormat,
		pstHDMI->stAVI.u8Colorimetry,
		pstHDMI->stAVI.u8ExtendedColorimetry,
		pstHDMI->stAVI.u8AdditionalColorimetry);

	stHDMIInfo.u8HDMISource_HDR10plus_VSIF_Packet_Valid = false;
	if (pstHDMI->u8IsVSIF10PlusDataValid) {
		if (_mtk_cfd_CovertHDR10PlusVSIF(
			&stHDMIInfo.stu_Cfd_HDMISource_HDR10plus_VSIF,
			pstVSIF10Plus))
			stHDMIInfo.u8HDMISource_HDR10plus_VSIF_Packet_Valid =
				true;
	}

	stHDMIInfo.pstu_Cfd_HDMISource_2094_40 = (struct STU_CFD_2094_40 *)
		kzalloc(sizeof(struct STU_CFD_2094_40), GFP_KERNEL);
	stHDMIInfo.u8HDMISource_2094_40_Valid = false;
	if (pstHDMI->u8IsEMP10PlusDataValid) {
		if (_mtk_cfd_CovertHDR10PlusEMP(
			stHDMIInfo.pstu_Cfd_HDMISource_2094_40,
			pstEMP10Plus))
			stHDMIInfo.u8HDMISource_2094_40_Valid = true;
	}

	// TODO
	//bRet = MDrv_PQU_CFD_Wrapper_SetHDMIInfo(u8Win, &stHDMIInfo);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_HDMI);

	kfree(stHDMIInfo.pstu_Cfd_HDMISource_2094_40);
	return true;
}

bool mtk_cfd_SetMMParam(struct st_cfd_info *pstCFDInfo)
{
	struct STU_CFDAPI_MM_PARSER stMMInfo;
	struct st_cfd_mm_info *pstVDEC;
	struct st_cfd_input_info *pstIn;
	struct st_hdr10plus_sei *pstHDR10Plus;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;
	pstIn = &pstCFDInfo->stIn;
	pstVDEC = &pstCFDInfo->stIn.stVDEC;
	pstHDR10Plus = &pstCFDInfo->stIn.stSEI10Plus;

	stMMInfo.u32Version = CFD_MM_ST_VERSION;
	stMMInfo.u16Length = sizeof(struct STU_CFDAPI_MM_PARSER);
	if (pstVDEC->u8IsVUIValid) {
		stMMInfo.u8Colour_primaries = pstVDEC->stUVI.u8ColourPrimaries;
		stMMInfo.u8Transfer_Characteristics =
				pstVDEC->stUVI.u8TransferCharacteristics;
		stMMInfo.u8Matrix_Coeffs = pstVDEC->stUVI.u8MatrixCoeffs;
	} else {
		stMMInfo.u8Colour_primaries = pstIn->stMainColorFmt.u8CP;
		stMMInfo.u8Transfer_Characteristics =
				pstIn->stMainColorFmt.u8TC;
		stMMInfo.u8Matrix_Coeffs = pstIn->stMainColorFmt.u8MC;
	}

	if (pstVDEC->u8IsGeneralInfoValid)
		stMMInfo.u8Video_Full_Range_Flag =
			pstVDEC->stGeneral.u8VideoFullRangeFlag;
	else
		stMMInfo.u8Video_Full_Range_Flag =
			pstIn->stMainColorFmt.bFullRange;

	if (pstVDEC->u8IsSEIValid) {
		stMMInfo.u32Master_Panel_Max_Luminance =
			pstVDEC->stSEI.u32MasterPanelMaxLuminance;
		stMMInfo.u32Master_Panel_Min_Luminance =
			pstVDEC->stSEI.u32MasterPanelMinLuminance;
		stMMInfo.stu_MasterPanel_ColorMetry.u16Display_Primaries_x[0] =
			pstVDEC->stSEI.u16DisplayPrimariesX[0];
		stMMInfo.stu_MasterPanel_ColorMetry.u16Display_Primaries_x[1] =
			pstVDEC->stSEI.u16DisplayPrimariesX[1];
		stMMInfo.stu_MasterPanel_ColorMetry.u16Display_Primaries_x[2] =
			pstVDEC->stSEI.u16DisplayPrimariesX[2];
		stMMInfo.stu_MasterPanel_ColorMetry.u16Display_Primaries_y[0] =
			pstVDEC->stSEI.u16DisplayPrimariesY[0];
		stMMInfo.stu_MasterPanel_ColorMetry.u16Display_Primaries_y[1] =
			pstVDEC->stSEI.u16DisplayPrimariesY[1];
		stMMInfo.stu_MasterPanel_ColorMetry.u16Display_Primaries_y[2] =
			pstVDEC->stSEI.u16DisplayPrimariesY[2];
		stMMInfo.stu_MasterPanel_ColorMetry.u16White_point_x =
			pstVDEC->stSEI.u16WhitePointX;
		stMMInfo.stu_MasterPanel_ColorMetry.u16White_point_y =
			pstVDEC->stSEI.u16WhitePointY;
		stMMInfo.u8Mastering_Display_Infor_Valid = true;
	} else {
		stMMInfo.u8Mastering_Display_Infor_Valid = false;
	}

	if (pstVDEC->u8IsCLLValid) {
		stMMInfo.u8MM_HDR_ContentLightMetaData_Valid = true;
		stMMInfo.u16Max_content_light_level =
			pstVDEC->stCLL.u16MaxContentLightLevel;
		stMMInfo.u16Max_pic_average_light_level =
			pstVDEC->stCLL.u16MaxPicAverageLightLevel;
	} else {
		stMMInfo.u8MM_HDR_ContentLightMetaData_Valid = false;
	}

	stMMInfo.u8MM_HDR10plus_SEI_Message_Valid = false;
	if (pstVDEC->u8IsHDR10PlusSEIValid) {
		if (_mtk_cfd_CovertHDR10PlusSEI(
			&stMMInfo.stu_Cfd_MM_HDR10plus_SEI, pstHDR10Plus))
			stMMInfo.u8MM_HDR10plus_SEI_Message_Valid = true;
	}

	// TODO
	//bRet = MDrv_PQU_CFD_Wrapper_SetMMInfo(u8Win, &stMMInfo);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_MM);

	return true;
}

bool mtk_cfd_SetTMOParam(struct st_cfd_info *pstCFDInfo)
{
	return true;
}

bool mtk_cfd_SetOutputParam(struct st_cfd_info *pstCFDInfo)
{
	struct STU_CFDAPI_PANEL_FORMAT stPanelParam;
	struct st_cfd_output_info *pstOut;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;
	pstOut = &pstCFDInfo->stOut;
	memset(&stPanelParam, 0, sizeof(struct STU_CFDAPI_PANEL_FORMAT));

	stPanelParam.u32Version = CFD_HDMI_PANEL_ST_VERSION;
	stPanelParam.u16Length = sizeof(struct STU_CFDAPI_PANEL_FORMAT);

	stPanelParam.u16Panel_Med_Luminance = pstOut->u16MedLuminance;
	stPanelParam.u16Panel_Max_Luminance = pstOut->u16MaxLuminance;
	stPanelParam.u16Panel_Min_Luminance = pstOut->u16MinLuminance;

	stPanelParam.stu_Cfd_Panel_ColorMetry.u16Display_Primaries_x[0] =
		pstOut->stCP.u16Display_Primaries_x[0];
	stPanelParam.stu_Cfd_Panel_ColorMetry.u16Display_Primaries_x[1] =
		pstOut->stCP.u16Display_Primaries_x[1];
	stPanelParam.stu_Cfd_Panel_ColorMetry.u16Display_Primaries_x[2] =
		pstOut->stCP.u16Display_Primaries_x[2];
	stPanelParam.stu_Cfd_Panel_ColorMetry.u16Display_Primaries_y[0] =
		pstOut->stCP.u16Display_Primaries_y[0];
	stPanelParam.stu_Cfd_Panel_ColorMetry.u16Display_Primaries_y[1] =
		pstOut->stCP.u16Display_Primaries_y[1];
	stPanelParam.stu_Cfd_Panel_ColorMetry.u16Display_Primaries_y[2] =
		pstOut->stCP.u16Display_Primaries_y[2];
	stPanelParam.stu_Cfd_Panel_ColorMetry.u16White_point_x =
		pstOut->stCP.u16White_point_x;
	stPanelParam.stu_Cfd_Panel_ColorMetry.u16White_point_y =
		pstOut->stCP.u16White_point_y;

	// TODO
	//bRet = MDrv_PQU_CFD_Wrapper_SetPanelInfo(u8Win, &stPanelParam);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_OUTPUT);

	return bRet;
}

bool mtk_cfd_SetUIParam(struct st_cfd_info *pstCFDInfo)
{
	struct STU_CFDAPI_UI_CONTROL stUIInfo;
	struct st_cfd_ui_info *pstUI;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;
	memset(&stUIInfo, 0, sizeof(struct STU_CFDAPI_UI_CONTROL));

	stUIInfo.u32Version = CFD_HDMI_OSD_ST_VERSION;
	stUIInfo.u16Length = sizeof(struct STU_CFDAPI_UI_CONTROL);
	if (pstUI->bSkipPictureSetting == true) {
		stUIInfo.u16Hue = 50;
		stUIInfo.u16Saturation = 128;
	} else {
		stUIInfo.u16Hue = pstUI->u16Hue;
		stUIInfo.u16Saturation = pstUI->u16Saturation;
	}
	stUIInfo.u16Contrast = pstUI->u16Contrast;

	stUIInfo.u8ColorCorrection_En = pstUI->bColorCorrectionEnable;
	stUIInfo.s32ColorCorrectionMatrix[0][0] =
				pstUI->as16CorrectionMatrix[0];
	stUIInfo.s32ColorCorrectionMatrix[0][1] =
				pstUI->as16CorrectionMatrix[1];
	stUIInfo.s32ColorCorrectionMatrix[0][2] =
				pstUI->as16CorrectionMatrix[2];
	stUIInfo.s32ColorCorrectionMatrix[1][0] =
				pstUI->as16CorrectionMatrix[3];
	stUIInfo.s32ColorCorrectionMatrix[1][1] =
				pstUI->as16CorrectionMatrix[4];
	stUIInfo.s32ColorCorrectionMatrix[1][2] =
				pstUI->as16CorrectionMatrix[5];
	stUIInfo.s32ColorCorrectionMatrix[2][0] =
				pstUI->as16CorrectionMatrix[6];
	stUIInfo.s32ColorCorrectionMatrix[2][1] =
				pstUI->as16CorrectionMatrix[7];
	stUIInfo.s32ColorCorrectionMatrix[2][2] =
				pstUI->as16CorrectionMatrix[8];

	stUIInfo.u16RGBGGain[0] = pstUI->au16RGBGain[0];
	stUIInfo.u16RGBGGain[1] = pstUI->au16RGBGain[1];
	stUIInfo.u16RGBGGain[2] = pstUI->au16RGBGain[2];

	stUIInfo.u8OSD_UI_En = pstUI->u8OSDUIEn;
	stUIInfo.u8OSD_UI_Mode = pstUI->u8OSDUIMode;
	stUIInfo.u8HDR_UI_H2SMode = pstUI->u8HDR_UI_H2SMode;

	stUIInfo.au16Brightness[0] = pstUI->au16Brightness[0];
	stUIInfo.au16Brightness[1] = pstUI->au16Brightness[1];
	stUIInfo.au16Brightness[2] = pstUI->au16Brightness[2];

	// TODO
	//bRet = MDrv_PQU_CFD_Wrapper_SetUIInfo(u8Win, &stUIInfo);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_UI);

	return bRet;
}

bool mtk_cfd_SetBlackAndWhite(struct st_cfd_info *pstCFDInfo)
{
	//struct ST_PQU_BLACKANDWHITE stBlackAndWhite;
	__u8 u8BlackLevel;
	__u8 u8WhiteLevel;
	struct st_cfd_ui_info *pstUI;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;
	pstUI = &pstCFDInfo->stUI;
	//memset(&stBlackAndWhite, 0, sizeof(struct ST_PQU_BLACKANDWHITE));

	//stBlackAndWhite.u8BlackLevel = pstUI->u8UltraBlackLevel;
	//stBlackAndWhite.u8WhiteLevel = pstUI->u8UltraWhiteLevel;
	u8BlackLevel = pstUI->u8UltraBlackLevel;
	u8WhiteLevel = pstUI->u8UltraWhiteLevel;

	// TODO
	//bRet = MDrv_PQU_CFD_Wrapper_SetBlackAndWhite(u8Win, &stBlackAndWhite);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_BLACK_WHITE);

	return bRet;
}

bool mtk_cfd_SetDolbyHdrParam(struct st_cfd_info *pstCFDInfo)
{
	struct STU_CFDAPI_HDR_METADATA_FORMAT stDVParam;
	bool bIsDolbyEn;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;
	memset(&stDVParam, 0, sizeof(struct STU_CFDAPI_HDR_METADATA_FORMAT));
	bIsDolbyEn = pstCFDInfo->stDV.bIsDolby;

	stDVParam.u32Version = CFD_HDMI_HDR_METADATA_VERSION;
	stDVParam.u16Length = sizeof(struct STU_CFDAPI_HDR_METADATA_FORMAT);
	stDVParam.stu_Cfd_Dolby_HDR_Param.u8IsDolbyHDREn = bIsDolbyEn ? 1 : 0;

	// TODO
	//bRet = MDrv_PQU_CFD_Wrapper_SetHDRMetadata(u8Win, &stDVParam);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_DOLBY_HDR);

	return bRet;
}

bool mtk_cfd_Set420CtrlParam(struct st_cfd_info *pstCFDInfo)
{
	struct STU_CFD_MS_ALG_INTERFACE_420CONTROL st420CtrlParam;
	struct st_cfd_input_info *pstIn;
	struct st_cfd_output_info *pstOut;
	struct st_xc_status_info *pstXC;
	struct st_pq_status_info *pstPQ;
	struct st_dv_status_info *pstDV;
	//enum EN_HW_CFD_HDR_POSITION enPosition;
	__u8 u8HDRPosition;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;
	memset(&st420CtrlParam, 0,
		sizeof(struct STU_CFD_MS_ALG_INTERFACE_420CONTROL));
	pstOut = &pstCFDInfo->stOut;
	pstIn = &pstCFDInfo->stIn;
	pstXC = &pstCFDInfo->stXC;
	pstPQ = &pstCFDInfo->stPQ;
	pstDV = &pstCFDInfo->stDV;
	u8HDRPosition = pstCFDInfo->stCFD.u8HdrPosition;

	st420CtrlParam.stu_Maserati_420CONTROL_Param.u32Version =
		CFD_CONTROL_420_VERSION;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.u16Length =
		sizeof(struct STU_CFD_MS_ALG_INTERFACE_420CONTROL);
	st420CtrlParam.stu_Maserati_420CONTROL_Param.u8Input_DataMode =
		pstXC->bIsInputImode ? 0 : 1;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.u8Input_Dolby =
		pstDV->bIsDolby ? 0 : 1;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.Input_DataFormat =
		pstIn->u8SRCDataFmt;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.IP1_DataFormat =
		pstIn->stMainColorFmt.u8DataFmt;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.DRAM_Format =
		pstXC->u8MADIDramFormat;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.Output_Format =
		pstOut->stMainColorFmt.u8DataFmt;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.Input_Source =
		pstIn->u8InputSRC;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.u16HDRPos =
		(u8HDRPosition == 1) ? 1 : 0;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.bIsHVSPEnable =
		pstXC->bHVSPEnable;
	st420CtrlParam.stu_Maserati_420CONTROL_Param.bIsVSDEnable =
		pstXC->bPreVSDEnable;

	//TODO
	//bRet = MDrv_PQU_CFD_Wrapper_Set420CtrlInfo(u8Win, &st420CtrlParam);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_420CTRL);

	return bRet;
}

bool mtk_cfd_SetFileGrainParam(struct st_cfd_info *pstCFDInfo)
{
	struct st_cfd_input_info *pstIn;
	__u8 *pu8FGInfo;
	bool bIsFGValid;
	bool bRet = true;

	if (!pstCFDInfo)
		return false;
	pstIn = &pstCFDInfo->stIn;

	bIsFGValid = pstIn->stVDEC.u8IsFilmGrainValid;
	if (bIsFGValid == false)
		return true;

	pu8FGInfo = pstIn->stFG.au8FilmGrain;
	// TODO write param to share memory
	//bRet = MDrv_PQU_CFD_Wrapper_SetFilmGrainInfo(u8Win, pu8FGInfo);
	if (bRet)
		_mtk_cfd_ClearFlag(&pstCFDInfo->u32UpdataFlag,
			CFD_PARAM_BIT_FG);

	return bRet;
}

bool mtk_cfd_SetFire(struct st_cfd_info *pstCFDInfo, bool bForce)
{
	if (!pstCFDInfo)
		return false;

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_MAIN)
		|| bForce)
		mtk_cfd_SetMainParam(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_HDMI)
		|| bForce)
		mtk_cfd_SetHDMIParam(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_MM)
		|| bForce)
		mtk_cfd_SetMMParam(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_OUTPUT)
		|| bForce)
		mtk_cfd_SetOutputParam(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_UI)
		|| bForce)
		mtk_cfd_SetUIParam(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_TMO)
		|| bForce)
		mtk_cfd_SetTMOParam(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag,
		CFD_PARAM_BIT_BLACK_WHITE) || bForce)
		mtk_cfd_SetBlackAndWhite(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag,
		CFD_PARAM_BIT_DOLBY_HDR) || bForce)
		mtk_cfd_SetDolbyHdrParam(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag,
		CFD_PARAM_BIT_420CTRL) || bForce)
		mtk_cfd_Set420CtrlParam(pstCFDInfo);

	if (_mtk_cfd_IsFlagSet(pstCFDInfo->u32UpdataFlag, CFD_PARAM_BIT_FG)
		|| bForce)
		mtk_cfd_SetFileGrainParam(pstCFDInfo);

	// TODO set CFD trigger BIT

	pstCFDInfo->stCFD.bCFDFired = true;

	return true;
}

#endif		// _MTK_PQ_CFD_C_
