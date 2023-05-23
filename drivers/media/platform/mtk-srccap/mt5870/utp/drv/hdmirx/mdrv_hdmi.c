// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
/// file    Mdrv_hdmi.c
/// @brief  Driver Interface
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef MDRV_HDMI_C
#define MDRV_HDMI_C
//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>


// Internal Definition
#include "drvXC_HDMI_Internal.h"
#include "drvXC_IOPort.h"
#include "drv_HDMI.h"
#include "sti_msos.h"
#include "mdrv_hdmirx.h"

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

#define DRV_LABEL	"[DRV_HDMI] "
#define DRV_HDMIRX_MSG_DEBUG(format, args...) \
					pr_debug(DRV_LABEL format, ##args)
#define DRV_HDMIRX_MSG_INFO(format, args...) \
					pr_info(DRV_LABEL format, ##args)
#define DRV_HDMIRX_MSG_WARNING(format, args...) \
					pr_warn(DRV_LABEL format, ##args)
#define DRV_HDMIRX_MSG_ERROR(format, args...) \
					pr_err(DRV_LABEL format, ##args)
#define DRV_HDMIRX_MSG_FATAL(format, args...) \
					pr_crit(DRV_LABEL format, ##args)

#define atoi(str) \
	kstrtoul(((str != NULL) ? str : ""), NULL, 0)


//-------------------------------------------------------------------------------------------------
//  Local Structurs
//-------------------------------------------------------------------------------------------------
MS_BOOL bInitEnable = FALSE;
// if no portindex in, need to set current port to get info.
// Otherwise source can be got from pHDMIRes->
// stHDMIPollingInfo[HDMI_GET_PORT_SELECT(enInputPortType)].ucHDMIInfoSource
MS_U8 ucHDMIInuptSourceSelect = INPUT_PORT_DVI0;

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
/// Debug information

MS_BOOL bHDCPInterruptProcFlag = FALSE;
MS_BOOL bHDCPInterruptCreateFlag = FALSE;
MS_S32 slHDCPInterruptTaskID = -1;
MS_U8 ucHDCPInterruptStack[HDMI_HDCP_INTERRUPT_STACK_SIZE];
static struct HDMI_RX_RESOURCE_PRIVATE *pHDMIRes;
static struct task_struct *stHDMIRxThread;

//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
static void Drv_HDMI_ReadEDID(E_XC_DDCRAM_PROG_TYPE eDDCRamType, MS_U8 *u8EDID,
			       MS_U16 u8EDIDSize);
static void Drv_HDMI_WriteEDID(E_XC_DDCRAM_PROG_TYPE eDDCRamType, MS_U8 *u8EDID,
				MS_U16 u16EDIDSize);

void _drv_HDMI_ParseAVIInfoFrame(void *pPacket, stAVI_PARSING_INFO *pstAVIParsingInfo)
{
	MS_BOOL bExtendColormetryFlag = FALSE;

	if (pstAVIParsingInfo != NULL) {
		MS_U8 *pucInfoFrame = (MS_U8 *) pPacket;

		pstAVIParsingInfo->enColorForamt = MS_HDMI_COLOR_RGB;
		pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_DEFAULT;
		pstAVIParsingInfo->enColormetry = MS_HDMI_COLORIMETRY_NO_DATA;
		pstAVIParsingInfo->enPixelRepetition = MS_HDMI_PIXEL_REPETITION_1X;
		pstAVIParsingInfo->enAspectRatio = HDMI_Pic_AR_NODATA;
		pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_SAME;

		// 0: PB1
		// 1: PB2
		// 2: PB3
		// 3: PB4
		// 4: PB5
		// 5: PB6
		// 12: PB13
		// 13: PB14
		// 14: AVI Version
		pstAVIParsingInfo->u8Version =
		pucInfoFrame[13];

		pstAVIParsingInfo->u8Length =
			HDMI_AVI_PACKET_LENGTH;	// No length in register

		pstAVIParsingInfo->u8S10Value =
			pucInfoFrame[0] & BMASK(1:0);

		pstAVIParsingInfo->u8B10Value =
			(pucInfoFrame[0] & BMASK(3:2)) >> 2;

		pstAVIParsingInfo->u8A0Value =
			(pucInfoFrame[0] & BIT(4)) >> 4;

		pstAVIParsingInfo->u8Y210Value =
			(pucInfoFrame[0] & BMASK(7:5)) >> 5;

		pstAVIParsingInfo->u8R3210Value =
			pucInfoFrame[1] & BMASK(3:0);

		pstAVIParsingInfo->u8M10Value =
			(pucInfoFrame[1] & BMASK(5:4)) >> 4;

		pstAVIParsingInfo->u8C10Value =
			(pucInfoFrame[1] & BMASK(7:6)) >> 6;

		pstAVIParsingInfo->u8SC10Value =
			pucInfoFrame[2] & BMASK(1:0);

		pstAVIParsingInfo->u8Q10Value =
			(pucInfoFrame[2] & BMASK(3:2)) >> 2;

		pstAVIParsingInfo->u8EC210Value =
			(pucInfoFrame[2] & BMASK(6:4)) >> 4;

		pstAVIParsingInfo->u8ITCValue =
			(pucInfoFrame[2] & BIT(7)) >> 7;

		pstAVIParsingInfo->u8VICValue =
			pucInfoFrame[3] & BMASK(6:0);

		pstAVIParsingInfo->u8PR3210Value =
			pucInfoFrame[4] & BMASK(3:0);

		pstAVIParsingInfo->u8CN10Value =
			(pucInfoFrame[4] & BMASK(5:4)) >> 4;

		pstAVIParsingInfo->u8YQ10Value =
			(pucInfoFrame[4] & BMASK(7:6)) >> 6;

		pstAVIParsingInfo->u8ACE3210Value =
			(pucInfoFrame[13] & BMASK(7:4)) >> 4;

		switch (pstAVIParsingInfo->u8Y210Value) {
		case 0x0:
			pstAVIParsingInfo->enColorForamt = MS_HDMI_COLOR_RGB;
			break;

		case 0x2:
			pstAVIParsingInfo->enColorForamt = MS_HDMI_COLOR_YUV_444;
			break;

		case 0x1:
			pstAVIParsingInfo->enColorForamt = MS_HDMI_COLOR_YUV_422;
			break;

		case 0x3:
			pstAVIParsingInfo->enColorForamt = MS_HDMI_COLOR_YUV_420;
			break;

		default:
			break;
		};

		if (pstAVIParsingInfo->enColorForamt == MS_HDMI_COLOR_RGB) {
			switch (pstAVIParsingInfo->u8Q10Value) {
			case 0x0:
				//CEA spec VIC=1 640*480, is the only timing of table 1,
				//the default color space shall be RGB using Full Range
				if (pstAVIParsingInfo->u8VICValue == 0x01)
					pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_FULL;
				else if (pstAVIParsingInfo->u8VICValue == 0x00)
					pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_FULL;
				else
					pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_LIMIT;
				break;

			case 0x1:
				pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_LIMIT;
				break;

			case 0x2:
				pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_FULL;
				break;

			case 0x3:
				pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_RESERVED;
				break;

			default:
				break;
			};
		} else {
			switch (pstAVIParsingInfo->u8YQ10Value) {
			case 0x0:
				pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_LIMIT;
				break;

			case 0x1:
				pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_FULL;
				break;

			case 0x2:
				pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_RESERVED;
				break;

			case 0x3:
				pstAVIParsingInfo->enColorRange = E_HDMI_COLOR_RANGE_RESERVED;
				break;

			default:
				break;
			};
		}

		switch (pstAVIParsingInfo->u8C10Value) {
		case 0:
			if (pstAVIParsingInfo->enColorForamt != MS_HDMI_COLOR_RGB)
				pstAVIParsingInfo->enColormetry = MS_HDMI_COLORIMETRY_NO_DATA;
			break;

		case 0x1:
			if (pstAVIParsingInfo->enColorForamt != MS_HDMI_COLOR_RGB)
				pstAVIParsingInfo->enColormetry = MS_HDMI_COLORIMETRY_SMPTE_170M;
			break;

		case 0x2:
			if (pstAVIParsingInfo->enColorForamt != MS_HDMI_COLOR_RGB)
				pstAVIParsingInfo->enColormetry = MS_HDMI_COLORIMETRY_ITU_R_BT_709;
			break;

		case 0x3:
			bExtendColormetryFlag = TRUE;
			break;

		default:
			pstAVIParsingInfo->enColormetry = MS_HDMI_COLORIMETRY_UNKNOWN;
			break;
		};

		if (bExtendColormetryFlag) {
			switch (pstAVIParsingInfo->u8EC210Value) {
			case 0:
				pstAVIParsingInfo->enColormetry = MS_HDMI_EXT_COLOR_XVYCC601;
				break;

			case 0x1:
				pstAVIParsingInfo->enColormetry = MS_HDMI_EXT_COLOR_XVYCC709;
				break;

			case 0x2:
				pstAVIParsingInfo->enColormetry = MS_HDMI_EXT_COLOR_SYCC601;
				break;

			case 0x3:
				pstAVIParsingInfo->enColormetry = MS_HDMI_EXT_COLOR_ADOBEYCC601;
				break;

			case 0x4:
				pstAVIParsingInfo->enColormetry = MS_HDMI_EXT_COLOR_ADOBERGB;
				break;

			case 0x5:
				pstAVIParsingInfo->enColormetry = MS_HDMI_EXT_COLOR_BT2020YcCbcCrc;
				break;

			case 0x6:
				pstAVIParsingInfo->enColormetry = MS_HDMI_EXT_COLOR_BT2020RGBYCbCr;
				break;

			case 0x7:
				if (pstAVIParsingInfo->u8ACE3210Value == 0x00) {
					pstAVIParsingInfo->enColormetry =
					    MS_HDMI_EXT_COLOR_ADDITIONAL_DCI_P3_RGB_D65;
				} else if (pstAVIParsingInfo->u8ACE3210Value == 0x01) {
					pstAVIParsingInfo->enColormetry =
					    MS_HDMI_EXT_COLOR_ADDITIONAL_DCI_P3_RGB_THEATER;
				}
				break;

			default:
				pstAVIParsingInfo->enColormetry = MS_HDMI_EXT_COLOR_UNKNOWN;
				break;
			};
		}

		pstAVIParsingInfo->enPixelRepetition =
		    (MS_HDMI_PIXEL_REPETITION) pstAVIParsingInfo->u8PR3210Value;

		switch (pstAVIParsingInfo->u8M10Value) {
		case 0x0:
			pstAVIParsingInfo->enAspectRatio = HDMI_Pic_AR_NODATA;
			break;

		case 0x1:
			pstAVIParsingInfo->enAspectRatio = HDMI_Pic_AR_4x3;
			break;

		case 0x2:
			pstAVIParsingInfo->enAspectRatio = HDMI_Pic_AR_16x9;
			break;

		case 0x3:
		default:
			pstAVIParsingInfo->enAspectRatio = HDMI_Pic_AR_RSV;
			break;
		};

		switch (pstAVIParsingInfo->u8R3210Value) {
		case 0x2:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_16x9_Top;
			break;

		case 0x3:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_14x9_Top;
			break;

		case 0x4:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_GT_16x9;
			break;

		case 0x8:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_SAME;
			break;

		case 0x9:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_4x3_C;
			break;

		case 0xA:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_16x9_C;
			break;

		case 0xB:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_14x9_C;
			break;

		case 0xD:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_4x3_with_14x9_C;
			break;

		case 0xE:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_16x9_with_14x9_C;
			break;

		case 0xF:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_16x9_with_4x3_C;
			break;

		default:
			pstAVIParsingInfo->enActiveFormatAspectRatio = HDMI_AF_AR_SAME;
			break;
		}
	}

}

void _drv_HDMI_ParseVSInfoFrame(void *pPacket, stVS_PARSING_INFO *pstVSParsingInfo)
{
	if (pstVSParsingInfo != NULL) {
		MS_U8 *pucInfoFrame = (MS_U8 *) pPacket;

		// 0: HB0
		// 1: HB1
		// 2: HB2
		// 3: NULL
		// 4: PB0
		// 5: PB1
		// 6: PB2
		// 7: PB3
		// 8: PB4
		// 9: PB5
		// 10: PB6
		pstVSParsingInfo->u8Version = pucInfoFrame[1];
		pstVSParsingInfo->u8Length =
			pucInfoFrame[2] & BMASK(4:0);

		if ((pucInfoFrame[7] == 0x00) && (pucInfoFrame[6] == 0x0C)
		    && (pucInfoFrame[5] == 0x03)) {
			pstVSParsingInfo->enHDMIVideoFormat =
				(pucInfoFrame[8] & BMASK(7:5)) >> 5;

			if (pstVSParsingInfo->enHDMIVideoFormat == E_HDMI_4Kx2K_FORMAT)
				pstVSParsingInfo->u8HDMIVIC = pucInfoFrame[9];
			else if (pstVSParsingInfo->enHDMIVideoFormat == E_HDMI_3D_FORMAT) {
				pstVSParsingInfo->u83DStructure =
					(pucInfoFrame[9] & BMASK(7:4)) >> 4;
				pstVSParsingInfo->u83DExtData =
					(pucInfoFrame[10] & BMASK(7:4)) >> 4;
			}
		} else if ((pucInfoFrame[7] == 0xC4) && (pucInfoFrame[6] == 0x5D)
			   && (pucInfoFrame[5] == 0xD8)) {
			if (pucInfoFrame[9] & BIT(1))
				pstVSParsingInfo->u8ALLMMode = 1;
		}

		switch (pstVSParsingInfo->u8HDMIVIC) {
		case 0x1:
			pstVSParsingInfo->en4Kx2KVICCode = E_VIC_4Kx2K_30Hz;
			break;

		case 0x2:
			pstVSParsingInfo->en4Kx2KVICCode = E_VIC_4Kx2K_25Hz;
			break;

		case 0x3:
			pstVSParsingInfo->en4Kx2KVICCode = E_VIC_4Kx2K_24Hz;
			break;

		case 0x4:
			pstVSParsingInfo->en4Kx2KVICCode = E_VIC_4Kx2K_24Hz_SMPTE;
			break;

		default:
			pstVSParsingInfo->en4Kx2KVICCode = E_VIC_RESERVED;
			break;
		};

		switch (pstVSParsingInfo->u83DStructure) {
		case 0x0:
			pstVSParsingInfo->en3DStructure = E_XC_3D_INPUT_FRAME_PACKING;
			break;

		case 0x1:
			pstVSParsingInfo->en3DStructure = E_XC_3D_INPUT_FIELD_ALTERNATIVE;
			break;

		case 0x2:
			pstVSParsingInfo->en3DStructure = E_XC_3D_INPUT_LINE_ALTERNATIVE;
			break;

		case 0x3:
			pstVSParsingInfo->en3DStructure = E_XC_3D_INPUT_SIDE_BY_SIDE_FULL;
			break;

		case 0x4:
			pstVSParsingInfo->en3DStructure = E_XC_3D_INPUT_L_DEPTH;
			break;

		case 0x5:
			pstVSParsingInfo->en3DStructure =
			    E_XC_3D_INPUT_L_DEPTH_GRAPHICS_GRAPHICS_DEPTH;
			break;

		case 0x6:
			pstVSParsingInfo->en3DStructure = E_XC_3D_INPUT_TOP_BOTTOM;
			break;

		case 0x8:
			pstVSParsingInfo->en3DStructure = E_XC_3D_INPUT_SIDE_BY_SIDE_HALF;
			break;

		default:
			pstVSParsingInfo->en3DStructure = E_XC_3D_INPUT_MODE_NONE;
			break;
		};

		switch (pstVSParsingInfo->u83DExtData) {
		case 0x0:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_HOR_SUB_SAMPL_0;
			break;

		case 0x1:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_HOR_SUB_SAMPL_1;
			break;

		case 0x2:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_HOR_SUB_SAMPL_2;
			break;

		case 0x3:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_HOR_SUB_SAMPL_3;
			break;

		case 0x4:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_QUINCUNX_MATRIX_0;
			break;

		case 0x5:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_QUINCUNX_MATRIX_1;
			break;

		case 0x6:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_QUINCUNX_MATRIX_2;
			break;

		case 0x7:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_QUINCUNX_MATRIX_3;
			break;

		default:
			pstVSParsingInfo->en3DExtendedData = E_3D_EXT_DATA_MODE_MAX;
			break;
		};
	}

}

void _drv_HDMI_ParseHDRInfoFrame(void *pPacket, sHDR_METADATA *pHdrMetadata, MS_BOOL bByteFormat)
{
	if (pHdrMetadata != NULL) {
		if (bByteFormat) {
			MS_U8 *pucInfoFrame = (MS_U8 *) pPacket;

			// 0: HB1
			// 1: HB2
			// 2: PB0
			// 3: PB1
			// 4: PB2
			// 5: PB3
			// 6: PB4
			pHdrMetadata->u8Version = pucInfoFrame[0];
			pHdrMetadata->u8Length = pucInfoFrame[1];
			pHdrMetadata->u8EOTF =
				pucInfoFrame[3] & BMASK(2:0);
			pHdrMetadata->u8Static_Metadata_ID =
				pucInfoFrame[4] & BMASK(2:0);

			if (pHdrMetadata->u8Static_Metadata_ID == HDMI_HDR_STATIC_METADATA_TYPE_0) {
				pHdrMetadata->u16Display_Primaries_X[0] =
				    (pucInfoFrame[6] << 8) | pucInfoFrame[5];
				pHdrMetadata->u16Display_Primaries_Y[0] =
				    (pucInfoFrame[8] << 8) | pucInfoFrame[7];
				pHdrMetadata->u16Display_Primaries_X[1] =
				    (pucInfoFrame[10] << 8) | pucInfoFrame[9];
				pHdrMetadata->u16Display_Primaries_Y[1] =
				    (pucInfoFrame[12] << 8) | pucInfoFrame[11];
				pHdrMetadata->u16Display_Primaries_X[2] =
				    (pucInfoFrame[14] << 8) | pucInfoFrame[13];
				pHdrMetadata->u16Display_Primaries_Y[2] =
				    (pucInfoFrame[16] << 8) | pucInfoFrame[15];
				pHdrMetadata->u16White_Point_X =
				    (pucInfoFrame[18] << 8) | pucInfoFrame[17];
				pHdrMetadata->u16White_Point_Y =
				    (pucInfoFrame[20] << 8) | pucInfoFrame[19];
				pHdrMetadata->u16Max_Display_Mastering_Luminance =
				    (pucInfoFrame[22] << 8) | pucInfoFrame[21];
				pHdrMetadata->u16Min_Display_Mastering_Luminance =
				    (pucInfoFrame[24] << 8) | pucInfoFrame[23];
				pHdrMetadata->u16Maximum_Content_Light_Level =
				    (pucInfoFrame[26] << 8) | pucInfoFrame[25];
				pHdrMetadata->u16Maximum_Frame_Average_Light_Level =
				    (pucInfoFrame[28] << 8) | pucInfoFrame[27];
			}
		} else {
			MS_U16 *pInfoFrame = (MS_U16 *) pPacket;

			// 8~15bits, 0:SDR gamma,  1:HDR gamma,
			// 2:SMPTE ST2084,  3:Future EOTF,  4-7:Reserved
			pHdrMetadata->u8EOTF = ((pInfoFrame[0] >> 8) & 0x07);
			// 0:Static Metadata Type 1,  1-7:Reserved for future use
			pHdrMetadata->u8Static_Metadata_ID = (pInfoFrame[1] & 0x0007);
			pHdrMetadata->u16Display_Primaries_X[0] =
			    (pInfoFrame[2] << 8) + (pInfoFrame[1] >> 8);

			pHdrMetadata->u16Display_Primaries_Y[0] =
			    (pInfoFrame[3] << 8) + (pInfoFrame[2] >> 8);
			pHdrMetadata->u16Display_Primaries_X[1] =
			    (pInfoFrame[4] << 8) + (pInfoFrame[3] >> 8);
			pHdrMetadata->u16Display_Primaries_Y[1] =
			    (pInfoFrame[5] << 8) + (pInfoFrame[4] >> 8);
			pHdrMetadata->u16Display_Primaries_X[2] =
			    (pInfoFrame[6] << 8) + (pInfoFrame[5] >> 8);
			pHdrMetadata->u16Display_Primaries_Y[2] =
			    (pInfoFrame[7] << 8) + (pInfoFrame[6] >> 8);
			pHdrMetadata->u16White_Point_X =
			    (pInfoFrame[8] << 8) + (pInfoFrame[7] >> 8);
			pHdrMetadata->u16White_Point_Y =
			    (pInfoFrame[9] << 8) + (pInfoFrame[8] >> 8);
			pHdrMetadata->u16Max_Display_Mastering_Luminance =
			    (pInfoFrame[10] << 8) + (pInfoFrame[9] >> 8);
			pHdrMetadata->u16Min_Display_Mastering_Luminance =
			    (pInfoFrame[11] << 8) + (pInfoFrame[10] >> 8);
			pHdrMetadata->u16Maximum_Content_Light_Level =
			    (pInfoFrame[12] << 8) + (pInfoFrame[11] >> 8);
			pHdrMetadata->u16Maximum_Frame_Average_Light_Level =
			    (pInfoFrame[13] << 8) + (pInfoFrame[12] >> 8);
		}
	}

}

void _drv_HDMI_ParseGCInfoFrame(void *pPacket, stGC_PARSING_INFO *pstGCParsingInfo)
{
	if (pstGCParsingInfo != NULL) {
		MS_U8 *pucInfoFrame = (MS_U8 *) pPacket;

		pstGCParsingInfo->u8ControlAVMute = pucInfoFrame[0] & BIT(0);
		pstGCParsingInfo->u8DefaultPhase = (pucInfoFrame[0] & BIT(1)) >> 1;
		pstGCParsingInfo->u8LastPPValue = (pucInfoFrame[0] & BMASK(4:2)) >> 2;
		pstGCParsingInfo->u8PreLastPPValue = (pucInfoFrame[0] & BMASK(7:5)) >> 5;
		pstGCParsingInfo->u8CDValue = pucInfoFrame[1] & BMASK(3:0);
		pstGCParsingInfo->u8PPValue = (pucInfoFrame[1] & BMASK(7:4)) >> 4;

		switch (pstGCParsingInfo->u8CDValue) {
		case HDMI_GC_INFO_CD_NOT_INDICATED:
		case HDMI_GC_INFO_CD_24BITS:
			pstGCParsingInfo->enColorDepth = MS_HDMI_COLOR_DEPTH_8_BIT;
			break;

		case HDMI_GC_INFO_CD_30BITS:
			pstGCParsingInfo->enColorDepth = MS_HDMI_COLOR_DEPTH_10_BIT;
			break;

		case HDMI_GC_INFO_CD_36BITS:
			pstGCParsingInfo->enColorDepth = MS_HDMI_COLOR_DEPTH_12_BIT;
			break;

		case HDMI_GC_INFO_CD_48BITS:
			pstGCParsingInfo->enColorDepth = MS_HDMI_COLOR_DEPTH_16_BIT;
			break;

		default:
			pstGCParsingInfo->enColorDepth = MS_HDMI_COLOR_DEPTH_UNKNOWN;
			break;
		};
	}
}

void _drv_HDMI_CheckHDCP14RiProc(void)
{
	E_MUX_INPUTPORT enPortIndex = INPUT_PORT_DVI0;
	MS_U8 ucPortIndex = 0;

	for (enPortIndex = INPUT_PORT_DVI0; enPortIndex <= INPUT_PORT_DVI_END; enPortIndex++) {
		ucPortIndex = HDMI_GET_PORT_SELECT(enPortIndex);
		if (pHDMIRes->usHDCP14RiCount[ucPortIndex]
			< HDMI_HDCP14_RI_COUNT)
			pHDMIRes->usHDCP14RiCount[ucPortIndex]++;
		else {
			// [13] Ri
			if (KDrv_HDCP_getstatus(enPortIndex) & BIT(13)) {
				pHDMIRes->bHDCP14RiFlag[ucPortIndex]
				= TRUE;
				KDrv_HDCP_ClearStatus(enPortIndex, BIT(5));
			} else {
				pHDMIRes->bHDCP14RiFlag[ucPortIndex]
					= FALSE;
			}

			pHDMIRes->usHDCP14RiCount[ucPortIndex]
				= 0;
		}
	}
}

void _drv_HDMI_CheckPacketReceiveProc(void)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_U32 u32PacketStatus = 0;

	for (u8HDMIInfoSource = HDMI_INFO_SOURCE0; u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX;
	     u8HDMIInfoSource++) {
		if (pHDMIRes->ucReceiveIntervalCount[u8HDMIInfoSource] <
		    pHDMIRes->u8PacketReceiveCount[u8HDMIInfoSource]) {
			pHDMIRes->ucReceiveIntervalCount[u8HDMIInfoSource]++;
		} else {
			u32PacketStatus =
			    KDrv_HDMIRx_packet_info(u8HDMIInfoSource,
				&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);

			pHDMIRes->ulPacketStatus[u8HDMIInfoSource] = u32PacketStatus;

			if (GET_HDMI_SYSTEM_FLAG(
				pHDMIRes->u32PrePacketStatus[u8HDMIInfoSource],
			     HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG)) {
				pHDMIRes->ulPacketStatus[u8HDMIInfoSource] |=
				    HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG;
			}

			if (GET_HDMI_SYSTEM_FLAG(
				u32PacketStatus, HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG)) {
				SET_HDMI_SYSTEM_FLAG(
				pHDMIRes->u32PrePacketStatus[u8HDMIInfoSource],
				HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG);
			} else {
				CLR_HDMI_SYSTEM_FLAG(
				pHDMIRes->u32PrePacketStatus[u8HDMIInfoSource],
				HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG);
			}

			pHDMIRes->u8PacketReceiveCount[u8HDMIInfoSource] =
			    KDrv_HDMIRx_GetPacketReceiveCount(u8HDMIInfoSource,
				pHDMIRes->u8PacketReceiveCount[u8HDMIInfoSource],
				pHDMIRes->stHDMIPollingInfo);

			pHDMIRes->ucReceiveIntervalCount[u8HDMIInfoSource] = 0;
		}
	}
}

static int _drv_HDMI_PollingTask(void *punused __attribute__ ((unused)))
{
	if (pHDMIRes != NULL) {
		while (pHDMIRes->bHDMITaskProcFlag) {
			KDrv_HDMIRx_StablePolling(pHDMIRes->ucMHLSupportPath,
						  pHDMIRes->ulPacketStatus,
						  pHDMIRes->stHDMIPollingInfo);

			_drv_HDMI_CheckPacketReceiveProc();
			_drv_HDMI_CheckHDCP14RiProc();
			MsOS_DelayTask(HDMI_POLLING_INTERVAL);
		}
	}

	return 0;
}

MS_BOOL _drv_HDMI_CreateTask(void)
{
	MS_BOOL bCreateSuccess = TRUE;

	if (pHDMIRes->slHDMIPollingTaskID < 0) {
		stHDMIRxThread =
		    kthread_create(
		    _drv_HDMI_PollingTask, pHDMIRes, "HDMI Rx task");

		if (IS_ERR(stHDMIRxThread)) {
			bCreateSuccess = FALSE;
			stHDMIRxThread = NULL;
			DRV_HDMIRX_MSG_FATAL("Create Thread Failed\r\n");
		} else {
			wake_up_process(stHDMIRxThread);
			pHDMIRes->slHDMIPollingTaskID = stHDMIRxThread->tgid;
			pHDMIRes->bHDMITaskProcFlag = TRUE;
			DRV_HDMIRX_MSG_INFO("Create Thread OK\r\n");
		}
	}

	if (pHDMIRes->slHDMIPollingTaskID < 0) {
		DRV_HDMIRX_MSG_ERROR("Create HDMI Rx Thread failed!!\n");

		bCreateSuccess = FALSE;
	}

	return bCreateSuccess;
}

static MS_U8 _drv_HDCP_IsrHandler(MS_U8 *ucHDCPStatusIndex)
{
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	MS_U8 u8Ret = 0;
	MS_U8 ucPortIndex = 0;

	if (pHDMIRes != NULL) {
		if (ucHDCPStatusIndex == NULL)
			return 0;

		pHDMIRes->ucHDCPWriteDoneIndex = 0;
		pHDMIRes->ucHDCPReadDoneIndex = 0;
		pHDMIRes->ucHDCPR0ReadDoneIndex = 0;

		for (enInputPortType = INPUT_PORT_DVI0; enInputPortType <= INPUT_PORT_DVI_END;
		     enInputPortType++) {
			ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);
			if (KDrv_HDCP22_PollingWriteDone(ucPortIndex)) {
				pHDMIRes->ucHDCPWriteDoneIndex |=
				    BIT(ucPortIndex);

				if (GET_HDMI_FLAG(
					pHDMIRes->ucHDCPReadDoneIndex,
				     BIT(ucPortIndex))) {
					CLR_HDMI_FLAG(pHDMIRes->ucHDCPReadDoneIndex,
						      BIT(ucPortIndex));
				}
				u8Ret = E_HDCP2X_WRITE_DONE;
				*ucHDCPStatusIndex = pHDMIRes->ucHDCPWriteDoneIndex;
			}

			if (KDrv_HDCP22_PollingReadDone(ucPortIndex)) {
				if (pHDMIRes->ucHDCP22RecvIDListSend[ucPortIndex]) {
				    //clear ready bit after source read re
					KDrv_HDCP22_SetReadyBit(ucPortIndex, FALSE);

					pHDMIRes->ucHDCP22RecvIDListSend[ucPortIndex]
						= FALSE;
				}

				SET_HDMI_FLAG(pHDMIRes->ucHDCPReadDoneIndex,
					      BIT(ucPortIndex));
				u8Ret = E_HDCP2X_READ_DONE;
				*ucHDCPStatusIndex = pHDMIRes->ucHDCPReadDoneIndex;
			}

			if (KDrv_HDCP14_PollingR0ReadDone(ucPortIndex)) {
				pHDMIRes->ucHDCPR0ReadDoneIndex |=
				    BIT(ucPortIndex);
				u8Ret = E_HDCP1X_R0READ_DONE;
				*ucHDCPStatusIndex = pHDMIRes->ucHDCPR0ReadDoneIndex;
			}
		}
	}

	return u8Ret;
}

MS_BOOL Drv_HDMIRx_HW_Init(stHDMIRx_Bank *stHDMIRxBank)
{
	return KDrv_HDMIRx_HW_Init(stHDMIRxBank);
}

MS_U8 Drv_HDMI_HDCP_ISR(MS_U8 *ucHDCPStatusIndex)
{
	return _drv_HDCP_IsrHandler(ucHDCPStatusIndex);
}

void Drv_HDCP22_IRQEnable(MS_BOOL ubIRQEnable)
{
	MS_U8 ucPortIdx = 0;

	if (pHDMIRes != NULL) {
		for (ucPortIdx = 0;
		     ucPortIdx < HDMI_GET_PORT_SELECT(INPUT_PORT_DVI_MAX); ucPortIdx++) {
			pHDMIRes->bHDCP22IRQEnableFlag[ucPortIdx]
			    = ubIRQEnable;
			KDrv_HDCP_WriteDoneInterruptEnable((INPUT_PORT_DVI0 + ucPortIdx),
							   ubIRQEnable);
		}
	}
}

void Drv_HDCP14_ReadR0IRQEnable(MS_BOOL ubR0IRQEnable)
{
	MS_U8 ucPortIdx = 0;

	if (pHDMIRes != NULL) {
		for (ucPortIdx = 0;
		     ucPortIdx < HDMI_GET_PORT_SELECT(INPUT_PORT_DVI_MAX); ucPortIdx++) {
			pHDMIRes->bHDCP14R0IRQEnable[ucPortIdx]
			    = ubR0IRQEnable;
			KDrv_HDCP_ReadR0InterruptEnable((INPUT_PORT_DVI0 + ucPortIdx),
							ubR0IRQEnable);
		}
	}
}

MS_BOOL Drv_HDMI_Current_Port_Set(E_MUX_INPUTPORT enInputPortType)
{
	if ((enInputPortType >= INPUT_PORT_DVI0) && (enInputPortType < INPUT_PORT_DVI_MAX)) {
		ucHDMIInuptSourceSelect = enInputPortType;
		KDrv_HDMIRx_Set_MUX(enInputPortType);
		return TRUE;
	} else
		return FALSE;
}

void Drv_HDMI_init(void)
{
	stHDMI_INITIAL_SETTING XCArgs;
	MS_BOOL bCreateHDCPTaskFlag = FALSE;
	MS_U8 ucHDMIInfoSource = 0;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	MS_U8 ucPortIndex = 0;

	DRV_HDMIRX_MSG_INFO("HDMIRx init\r\n");

	pHDMIRes =
		kmalloc(sizeof(struct HDMI_RX_RESOURCE_PRIVATE), GFP_KERNEL);

	XCArgs.bCreateHDCPTaskFlag = FALSE;
	XCArgs.stInitialTable.bImmeswitchSupport = FALSE;
	XCArgs.stInitialTable.ucMHLSupportPath = 0;

	if (pHDMIRes != NULL) {
		KDrv_HDMIRx_init(XCArgs.stInitialTable.bImmeswitchSupport,
				 XCArgs.stInitialTable.ucMHLSupportPath);

		if (pHDMIRes->ucInitialIndex != HDMI_INITIAL_DONE_INDEX) {
			for (enInputPortType = INPUT_PORT_DVI0;
			     enInputPortType <= INPUT_PORT_DVI_END; enInputPortType++) {
				ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bHDMIModeFlag
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bTimingStableFlag
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bAutoEQRetrigger
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucSourceVersion
					= HDMI_SOURCE_VERSION_NOT_SURE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDCPState
					= HDMI_HDCP_NO_ENCRYPTION;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bIsRepeater
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDCPInt
					= 0;
				pHDMIRes->bHDCP22IRQEnableFlag[ucPortIndex]
				    = FALSE;
				pHDMIRes->bHDCP14R0IRQEnable[ucPortIndex]
				    = FALSE;
				pHDMIRes->bHDCP22EnableFlag[ucPortIndex]
					= FALSE;
				pHDMIRes->bHPDInvertFlag[ucPortIndex]
					= FALSE;
				pHDMIRes->bHPDEnableFlag[ucPortIndex]
					= FALSE;
				pHDMIRes->bDataEnableFlag[ucPortIndex]
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource
					= KDrv_HDMIRx_GetSourceSelect(enInputPortType);
			}

			for (ucHDMIInfoSource = HDMI_INFO_SOURCE0;
			     ucHDMIInfoSource < HDMI_INFO_SOURCE_MAX; ucHDMIInfoSource++) {
				pHDMIRes->enMuxInputPort[ucHDMIInfoSource] =
				    INPUT_PORT_DVI0;
				pHDMIRes->ucReceiveIntervalCount[ucHDMIInfoSource] =
				    0;
				pHDMIRes->ulPacketStatus[ucHDMIInfoSource] = 0;
				pHDMIRes->u32PrePacketStatus[ucHDMIInfoSource] = 0;
				pHDMIRes->u8PacketReceiveCount[ucHDMIInfoSource] =
				    HDMI_PACKET_RECEIVE_COUNT;
			}

			pHDMIRes->bSelfCreateTaskFlag = FALSE;
			pHDMIRes->bHDMITaskProcFlag = FALSE;
			pHDMIRes->bHDCPIRQAttachFlag = FALSE;
			pHDMIRes->slHDMIPollingTaskID = -1;
			pHDMIRes->slHDMIRxLoadKeyTaskID = -1;
			pHDMIRes->slHDMIHDCPEventID = -1;
			pHDMIRes->slHDMIRxHDCP1xRiID = -1;
			pHDMIRes->bImmeswitchSupport =
			    XCArgs.stInitialTable.bImmeswitchSupport;
			pHDMIRes->ucMHLSupportPath =
			    XCArgs.stInitialTable.ucMHLSupportPath;
			pHDMIRes->ucHDCPReadDoneIndex = 0;
			pHDMIRes->ucHDCPWriteDoneIndex = 0;
			pHDMIRes->ucHDCPR0ReadDoneIndex = 0;
			pHDMIRes->bHDCP14RxREEFlag = FALSE;
			pHDMIRes->bSTRSuspendFlag = TRUE;

			bCreateHDCPTaskFlag = TRUE;

			pHDMIRes->ucInitialIndex = HDMI_INITIAL_DONE_INDEX;
		}

		if (pHDMIRes->bSTRSuspendFlag) {
			for (enInputPortType = INPUT_PORT_DVI0;
			     enInputPortType <= INPUT_PORT_DVI_END; enInputPortType++) {
				ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bNoInputFlag
					= TRUE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bPowerOnLane
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bHDMI20Flag
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bYUV420Flag
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].bPowerSavingFlag
					= FALSE;
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource
					= KDrv_HDMIRx_GetSourceSelect(enInputPortType);

				if (pHDMIRes->bHDCP22EnableFlag[ucPortIndex]) {
					KDrv_HDCP22_PortInit(ucPortIndex);
					KDrv_HDCP_WriteDoneInterruptEnable(enInputPortType, TRUE);

					pHDMIRes->bHDCP22IRQEnableFlag[ucPortIndex]
					    = TRUE;
				}
				if (pHDMIRes->bHDCP14R0IRQEnable[ucPortIndex])
					KDrv_HDCP_ReadR0InterruptEnable(enInputPortType, TRUE);
			}

			pHDMIRes->bSTRSuspendFlag = FALSE;
		}

		if (!pHDMIRes->bSelfCreateTaskFlag) {
			if (_drv_HDMI_CreateTask())
				pHDMIRes->bSelfCreateTaskFlag = TRUE;
		}

	}
}

MS_U32 drv_HDMI_STREventProc(EN_POWER_MODE usPowerState)
{
	MS_U32 ulReturnValue = 0;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	MS_U8 ucPortIndex = 0;

	if (pHDMIRes != NULL) {
		if (pHDMIRes->usPrePowerState != usPowerState) {
			if (usPowerState == E_POWER_SUSPEND) {
				DRV_HDMIRX_MSG_INFO("HDMIRx E_POWER_SUSPEND\r\n");
				//disable hdcp22 irq84 and clear status when suspend
				for (enInputPortType = INPUT_PORT_DVI0;
				     enInputPortType <= INPUT_PORT_DVI_END; enInputPortType++) {
					ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);
					if (pHDMIRes->bHDCP22EnableFlag[ucPortIndex]) {
						//disable hdcp22 irq84
						KDrv_HDCP22_PortInit(ucPortIndex);
						KDrv_HDCP_WriteDoneInterruptEnable(
							enInputPortType, FALSE);

						//clear status
						KDrv_HDCP22_PollingWriteDone(ucPortIndex);
						KDrv_HDCP22_PollingReadDone(ucPortIndex);


						pHDMIRes->bHDCP22IRQEnableFlag[ucPortIndex]
						    = FALSE;
					}
					if (pHDMIRes->bHDCP14R0IRQEnable[ucPortIndex]) {
						KDrv_HDCP_ReadR0InterruptEnable(
							enInputPortType, FALSE);
						pHDMIRes->bHDCP14R0IRQEnable[ucPortIndex]
						    = FALSE;
					}
				}

				ulReturnValue = 1;

			} else if (usPowerState == E_POWER_RESUME) {
				DRV_HDMIRX_MSG_INFO("HDMIRx E_POWER_RESUME\r\n");
				if (pHDMIRes->ucInitialIndex ==
				    HDMI_INITIAL_DONE_INDEX) {
					// Clear all HPD
					for (enInputPortType = INPUT_PORT_DVI0;
					     enInputPortType <= INPUT_PORT_DVI_END;
					     enInputPortType++) {
						ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);
						KDrv_HDMIRx_pullhpd(FALSE, enInputPortType,
					    pHDMIRes->bHPDInvertFlag[ucPortIndex],
					    pHDMIRes->ucMHLSupportPath);
						KDrv_HDMIRx_ClkPullLow(TRUE, enInputPortType);
						KDrv_HDMIRx_DataRtermControl(enInputPortType,
						FALSE);
					}

					// HDMI initial
					KDrv_HDMIRx_init(pHDMIRes->bImmeswitchSupport,
							 pHDMIRes->ucMHLSupportPath);

					// Delay 100ms
					MsOS_DelayTask(100);

					// Set all HPD
					for (enInputPortType = INPUT_PORT_DVI0;
					     enInputPortType <= INPUT_PORT_DVI_END;
					     enInputPortType++) {
						ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);
					if (pHDMIRes->bHDCP22EnableFlag[ucPortIndex]) {
						//before open mask, clear status
						KDrv_HDCP22_PollingWriteDone(ucPortIndex);
						KDrv_HDCP22_PollingReadDone(ucPortIndex);
						//enable hdcp22 irq84
						KDrv_HDCP22_PortInit(ucPortIndex);
						KDrv_HDCP_WriteDoneInterruptEnable(
						enInputPortType, TRUE);

						pHDMIRes->bHDCP22IRQEnableFlag[ucPortIndex]
						    = TRUE;
					}

					if (pHDMIRes->bHDCP14R0IRQEnable[ucPortIndex])
						KDrv_HDCP_ReadR0InterruptEnable(
						enInputPortType, TRUE);

					// Move to Upper layer to do
					//KDrv_HDMIRx_ClkPullLow(FALSE, enInputPortType);
					//KDrv_HDMIRx_DataRtermControl(enInputPortType, TRUE);
					//KDrv_HDMIRx_pullhpd(TRUE, enInputPortType,
					//		    pHDMIRes->bHPDInvertFlag[ucPortIndex],
					//		    pHDMIRes->ucMHLSupportPath);

					pHDMIRes->stHDMIPollingInfo[ucPortIndex].bNoInputFlag
						= TRUE;
					pHDMIRes->stHDMIPollingInfo[ucPortIndex].bPowerOnLane
						= FALSE;
					pHDMIRes->stHDMIPollingInfo[ucPortIndex].bHDMI20Flag
						= FALSE;
					pHDMIRes->stHDMIPollingInfo[ucPortIndex].bYUV420Flag
						= FALSE;
					pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource
						= KDrv_HDMIRx_GetSourceSelect(enInputPortType);
					}
				}

				ulReturnValue = 1;
			}

			pHDMIRes->usPrePowerState = usPowerState;
		}
	}

	return ulReturnValue;
}

// DDC
static void Drv_HDMI_WriteEDID(E_XC_DDCRAM_PROG_TYPE eDDCRamType, MS_U8 *u8EDID,
				MS_U16 u16EDIDSize)
{
	KDrv_HDMIRx_WriteEDID(eDDCRamType, u8EDID, u16EDIDSize);
}

// DDC
static void Drv_HDMI_ReadEDID(E_XC_DDCRAM_PROG_TYPE eDDCRamType, MS_U8 *u8EDID,
				MS_U16 u8EDIDSize)
{
	KDrv_HDMIRx_ReadEDID(eDDCRamType, u8EDID, u8EDIDSize);
}

static void Drv_HDMI_DDCRAM_Enable(E_XC_DDCRAM_PROG_TYPE eDDCRamType)
{
	KDrv_HDMIRx_DDCRAM_Enable(eDDCRamType);
}

void Drv_HDMI_PROG_DDCRAM(XC_DDCRAM_PROG_INFO *pstDDCRam_Info)
{
	MS_U8 u8EDIDTable[HDMI_EDID_SUPPORT_SIZE] = { 0 };
	XC_DDCRAM_PROG_INFO stDDCRamInfo;

	if ((pHDMIRes != NULL) && (pstDDCRam_Info != NULL)) {
		if (pstDDCRam_Info->u16EDIDSize <= HDMI_EDID_SUPPORT_SIZE) {
			memcpy(&u8EDIDTable, pstDDCRam_Info->EDID, pstDDCRam_Info->u16EDIDSize);

			stDDCRamInfo.EDID = u8EDIDTable;
			stDDCRamInfo.u16EDIDSize = pstDDCRam_Info->u16EDIDSize;
			stDDCRamInfo.eDDCProgType = pstDDCRam_Info->eDDCProgType;

			Drv_HDMI_DDCRAM_Enable(stDDCRamInfo.eDDCProgType);
			Drv_HDMI_WriteEDID(stDDCRamInfo.eDDCProgType, stDDCRamInfo.EDID,
					    stDDCRamInfo.u16EDIDSize);
		}
	}
}

void Drv_HDMI_READ_DDCRAM(XC_DDCRAM_PROG_INFO *pstDDCRam_Info)
{
	MS_U8 u8EDIDTable[HDMI_EDID_SUPPORT_SIZE] = { 0 };
	XC_DDCRAM_PROG_INFO stDDCRamInfo;

	if ((pHDMIRes != NULL) && (pstDDCRam_Info != NULL)) {
		stDDCRamInfo.EDID = u8EDIDTable;
		stDDCRamInfo.u16EDIDSize = pstDDCRam_Info->u16EDIDSize;
		stDDCRamInfo.eDDCProgType = pstDDCRam_Info->eDDCProgType;

		Drv_HDMI_DDCRAM_Enable(stDDCRamInfo.eDDCProgType);
		Drv_HDMI_ReadEDID(stDDCRamInfo.eDDCProgType, stDDCRamInfo.EDID,
				   stDDCRamInfo.u16EDIDSize);
		memcpy(pstDDCRam_Info->EDID, &u8EDIDTable, stDDCRamInfo.u16EDIDSize);
	}
}

MS_U8 Drv_HDMI_audio_channel_status(MS_U8 u8byte)
{
	MS_U8 u8ChannelStatus = 0;
	MS_U8 u8HDMIInfoSource = 0;

	u8HDMIInfoSource = KDrv_HDMIRx_GetPortToSourceMapping(ucHDMIInuptSourceSelect);

	if (pHDMIRes != NULL) {
		if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
			u8ChannelStatus =
		    KDrv_HDMIRx_audio_channel_status(u8HDMIInfoSource, u8byte, NULL);
		}
	}

	return u8ChannelStatus;
}

void Drv_HDMI_Audio_MUTE_Enable(MS_S32 s32value)
{
	if (s32value > 0)
		KDrv_HDMIRx_Audio_MUTE_Enable(0xFF, 0xFF);
	else
		KDrv_HDMIRx_Audio_MUTE_Enable(0, 0xFF);
}

MS_U16 Drv_HDMI_GetDataInfo(E_HDMI_GET_DATA_INFO enInfo)
{
	MS_U8 u8HDMIInfoSource = 0;

	if (pHDMIRes != NULL) {
		// need api for source select by HAL
		u8HDMIInfoSource = KDrv_HDMIRx_GetPortToSourceMapping(ucHDMIInuptSourceSelect);

		if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
			return KDrv_HDMIRx_GetDataInfo(enInfo, u8HDMIInfoSource,
			       &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
		}
	}

	return 0xFFFF;
}

MS_U16 Drv_HDMI_GetDataInfoByPort(E_HDMI_GET_DATA_INFO enInfo, E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = 0;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) && (enInputPortType < INPUT_PORT_DVI_MAX))
			return KDrv_HDMIRx_GetDataInfo(enInfo, (enInputPortType - INPUT_PORT_DVI0),
			       &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
	}

	return 0xFFFF;
}

MS_HDMI_CONTENT_TYPE Drv_HDMI_Get_Content_Type(void)
{
	MS_U8 u8ITCValue = 0;
	MS_U8 u8CN10Value = 0;
	MS_HDMI_CONTENT_TYPE enContentType = MS_HDMI_CONTENT_NoData;
	MS_U8 u8HDMIInfoSource = 0;

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource = KDrv_HDMIRx_GetPortToSourceMapping(ucHDMIInuptSourceSelect);

		if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
			if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource]
				& HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG) {
				u8ITCValue =
			    KDrv_HDMIRx_avi_infoframe_info(u8HDMIInfoSource, _BYTE_3,
				&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]) & BIT(7);
				u8CN10Value =
				KDrv_HDMIRx_avi_infoframe_info(u8HDMIInfoSource, _BYTE_5,
				&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]) & BMASK(5:4);

				switch (u8CN10Value) {
				case 0x00:
					if (u8ITCValue > 0)
						enContentType = MS_HDMI_CONTENT_Graphics;
					else
						enContentType = MS_HDMI_CONTENT_NoData;
					break;

				case 0x10:
					enContentType = MS_HDMI_CONTENT_Photo;
					break;

				case 0x20:
					enContentType = MS_HDMI_CONTENT_Cinema;
					break;

				case 0x30:
					enContentType = MS_HDMI_CONTENT_Game;
					break;

				default:
					enContentType = MS_HDMI_CONTENT_NoData;
					break;
				};
			}
		}
	}

	return enContentType;
}

MS_HDMI_COLOR_FORMAT Drv_HDMI_Get_ColorFormat(void)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_HDMI_COLOR_FORMAT enColorFormat = MS_HDMI_COLOR_UNKNOWN;

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource = KDrv_HDMIRx_GetPortToSourceMapping(ucHDMIInuptSourceSelect);

		if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
			switch (KDrv_HDMIRx_avi_infoframe_info
			(u8HDMIInfoSource, _BYTE_1,
			 &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]) & 0x60) {
			case 0x00:
				enColorFormat = MS_HDMI_COLOR_RGB;
				break;

			case 0x40:
				enColorFormat = MS_HDMI_COLOR_YUV_444;
				break;

			case 0x20:
				enColorFormat = MS_HDMI_COLOR_YUV_422;
				break;

			case 0x60:
				enColorFormat = MS_HDMI_COLOR_YUV_420;
				break;

			default:
				break;
			};
		}

	}

	return enColorFormat;
}

MS_HDMI_EXT_COLORIMETRY_FORMAT Drv_HDMI_Get_ExtColorimetry(void)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_U8 u8C10Value = 0;
	MS_U8 u8EC210Value = 0;
	MS_U8 u8ACE3210Value = 0;
	MS_HDMI_EXT_COLORIMETRY_FORMAT enColormetry = MS_HDMI_EXT_COLOR_UNKNOWN;
	static MS_U16 usColorimetryTemp[HDMI_INFO_SOURCE_MAX];

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource = KDrv_HDMIRx_GetPortToSourceMapping(ucHDMIInuptSourceSelect);

		if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
			if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG) {
				u8C10Value =
				KDrv_HDMIRx_avi_infoframe_info(
				u8HDMIInfoSource, _BYTE_2,
				&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]) & BMASK(7:6);
				u8EC210Value =
				KDrv_HDMIRx_avi_infoframe_info(
				u8HDMIInfoSource, _BYTE_3,
				&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]) & BMASK(6:4);

				if (u8C10Value == 0x40)
					enColormetry = MS_HDMI_EXT_COLORIMETRY_SMPTE_170M;
				else if (u8C10Value == 0x80)
					enColormetry = MS_HDMI_EXT_COLORIMETRY_ITU_R_BT_709;
				else if (u8C10Value == 0xC0) {
					switch (u8EC210Value) {
					case 0x00:
						enColormetry = MS_HDMI_EXT_COLOR_XVYCC601;
						break;

					case 0x10:
						enColormetry = MS_HDMI_EXT_COLOR_XVYCC709;
						break;

					case 0x20:
						enColormetry = MS_HDMI_EXT_COLOR_SYCC601;
						break;

					case 0x30:
						enColormetry = MS_HDMI_EXT_COLOR_ADOBEYCC601;
						break;

					case 0x40:
						enColormetry = MS_HDMI_EXT_COLOR_ADOBERGB;
						break;

					case 0x50:
						enColormetry = MS_HDMI_EXT_COLOR_BT2020YcCbcCrc;
						break;

					case 0x60:
						enColormetry = MS_HDMI_EXT_COLOR_BT2020RGBYCbCr;
						break;

					case 0x70:
					u8ACE3210Value =
					KDrv_HDMIRx_avi_infoframe_info(
					u8HDMIInfoSource, _BYTE_14,
					&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource])
					& BMASK(7:4);

					if (u8ACE3210Value == 0x00) {
						enColormetry =
					    MS_HDMI_EXT_COLOR_ADDITIONAL_DCI_P3_RGB_D65;

					//for LG legacy log
					if (usColorimetryTemp[u8HDMIInfoSource] !=
					    0x70) {
						DRV_HDMIRX_MSG_INFO
					    ("** HDMI ADDITIONAL_DCI_P3_D65 on port %d\r\n",
					     u8HDMIInfoSource);
						usColorimetryTemp[u8HDMIInfoSource]
						    = 0x70;
					}
					} else if (u8ACE3210Value == 0x10) {
						enColormetry =
						    MS_HDMI_EXT_COLOR_ADDITIONAL_DCI_P3_RGB_THEATER;

					//for LG legacy log
					if (usColorimetryTemp[u8HDMIInfoSource] !=
					    0x71) {
						DRV_HDMIRX_MSG_INFO
					    ("** HDMI ADDITIONAL_DCI_P3_THEATER on port %d\r\n",
					     u8HDMIInfoSource);
						usColorimetryTemp[u8HDMIInfoSource]
						    = 0x71;
					}
					}
					break;

					default:
						enColormetry = MS_HDMI_EXT_COLOR_UNKNOWN;
						break;
					}
				}
			}
		}
	}
	return enColormetry;
}

MS_BOOL Drv_HDCP_Enable(E_MUX_INPUTPORT enInputPortType, MS_BOOL bEnable)
{
	MS_BOOL bRet = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDCP_ddc_en(enInputPortType, bEnable);
			bRet = TRUE;
		}
	}

	return bRet;
}

MS_BOOL Drv_HDMI_Set_EQ_To_Port(MS_HDMI_EQ enEq, MS_U8 u8EQValue, E_MUX_INPUTPORT enInputPortType)
{
	MS_BOOL bRet = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_Set_EQ(enInputPortType, enEq, u8EQValue);
			bRet = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bRet;
}


//-------------------------------------------------------------------------------------------------
/// Clear the Audio mute status.
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_HDMI_Audio_Status_Clear(void)
{
	MS_BOOL bRet = FALSE;

	if (pHDMIRes != NULL) {
		KDrv_HDMIRx_Audio_Status_Clear(NULL);
		bRet = TRUE;
	}

	return bRet;
}

//-------------------------------------------------------------------------------------------------
/// Control the Hot Plug Detection
/// in HIGH Level, the voltage level is 2.4 ~ 5.3V
/// in LOW Level,  the voltage level is 0 ~ 0.4V
/// Enable hotplug GPIO[0] output and set output value
/// @param  bHighLow                \b IN: High/Low control
/// @param  enInputPortType      \b IN: Input source selection
/// @param  bInverse                 \b IN: Inverse or not
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_HDMI_pullhpd(MS_BOOL bHighLow, E_MUX_INPUTPORT enInputPortType, MS_BOOL bInverse)
{
	MS_BOOL bRet = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_pullhpd(bHighLow, enInputPortType, bInverse, 0);
			bRet = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bRet;
}

//[6]: soft-reset hdmi
//[5]: soft-reset hdcp
//[4]: soft-reset dvi
MS_BOOL Drv_DVI_Software_Reset(E_MUX_INPUTPORT enInputPortType, MS_U16 u16Reset)
{
	MS_BOOL bRet = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_Software_Reset(enInputPortType, u16Reset);
			bRet = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bRet;
}

MS_BOOL Drv_HDMI_PacketReset(E_MUX_INPUTPORT enInputPortType, HDMI_REST_t eReset)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_U8 ucPortIndex = 0;
	MS_BOOL bRet = FALSE;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
		pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_pkt_reset(enInputPortType, u8HDMIInfoSource, eReset,
			&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
			bRet = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bRet;
}

//-------------------------------------------------------------------------------------------------
/// Pull DVI Clock to low
/// @param  bPullLow                          \b IN: Set DVI clock to low
/// @param  enInputPortType              \b IN: Input source
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_DVI_ClkPullLow(MS_BOOL bPullLow, E_MUX_INPUTPORT enInputPortType)
{
	MS_BOOL bRet = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_ClkPullLow(bPullLow, enInputPortType);
			bRet = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bRet;
}

// HDMI ARC pin control
//     - enInputPortType: INPUT_PORT_DVI0 / INPUT_PORT_DVI1 / INPUT_PORT_DVI2 / INPUT_PORT_DVI3
//     - bEnable: ARC enable or disable
//     - bDrivingHigh: ARC driving current high or low, s
//        uggest driving current should be set to high when ARC is enable
MS_BOOL Drv_HDMI_ARC_PINControl(E_MUX_INPUTPORT enInputPortType, MS_BOOL bEnable,
				 MS_BOOL bDrivingHigh)
{
	MS_BOOL bRet = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_ARC_PINControl(enInputPortType, bEnable, bDrivingHigh);
			bRet = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bRet;
}

MS_U16 Drv_HDMI_GC_Info(E_MUX_INPUTPORT enInputPortType, HDMI_GControl_INFO_t enGCControlInfo)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_U16 u16GCPacketInfo = 0;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				u16GCPacketInfo =
			    KDrv_HDMIRx_gcontrol_info(enGCControlInfo, u8HDMIInfoSource);
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return u16GCPacketInfo;
}

MS_U8 Drv_HDMI_err_status_update(E_MUX_INPUTPORT enInputPortType, MS_U8 u8Value, MS_BOOL bReadFlag)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_U8 u8ErrorStatus = 0;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				u8ErrorStatus =
			    KDrv_HDMIRx_err_status_update(u8HDMIInfoSource, u8Value,
				bReadFlag,
				&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);

				if (u8ErrorStatus > 0) {
					if (KDrv_HDMIRx_err_status_update
					    (u8HDMIInfoSource, u8ErrorStatus, FALSE,
					     &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]) > 0) {
						// Do nothing for coverity issue
					}
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return u8ErrorStatus;
}

MS_U8 Drv_HDMI_audio_cp_hdr_info(E_MUX_INPUTPORT enInputPortType)
{

	MS_U8 u8HDMIInfoSource = 0;
	MS_U8 u8ContentProtectInfo = 0;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				u8ContentProtectInfo =
				    KDrv_HDMIRx_audio_content_protection_info(u8HDMIInfoSource,
					&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return u8ContentProtectInfo;
}

MS_U8 Drv_HDMI_Get_Pixel_Repetition(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_U8 u8PixelRepetition = 0;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				    HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG) {
					u8PixelRepetition =
					KDrv_HDMIRx_avi_infoframe_info(u8HDMIInfoSource,
					_BYTE_5,
					&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource])
					& BMASK(3:0);
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return u8PixelRepetition;
}

EN_AVI_INFOFRAME_VERSION Drv_HDMI_Get_AVIInfoFrameVer(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = 0;
	EN_AVI_INFOFRAME_VERSION enInfoFrameVersion = E_AVI_INFOFRAME_VERSION_NON;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				    HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG) {
					enInfoFrameVersion =
				    KDrv_HDMIRx_avi_infoframe_info_ver(u8HDMIInfoSource,
			       &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return enInfoFrameVersion;
}

MS_BOOL Drv_HDMI_Get_AVIInfoActiveInfoPresent(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_BOOL bActiveInformationPresent = FALSE;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				    HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG) {
					if (KDrv_HDMIRx_avi_infoframe_info
					    (u8HDMIInfoSource, _BYTE_1,
					     &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource])
					     & BIT(4)) {
						bActiveInformationPresent = TRUE;
					}
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bActiveInformationPresent;
}

MS_BOOL Drv_HDMI_IsHDMI_Mode(E_MUX_INPUTPORT enInputPortType)
{
	MS_BOOL bHDMImode = FALSE;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			bHDMImode =
		    pHDMIRes->stHDMIPollingInfo[ucPortIndex].bHDMIModeFlag;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bHDMImode;
}

MS_BOOL Drv_HDMI_Check4K2K(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_BOOL bCheckFlag = FALSE;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				    HDMI_STATUS_VS_PACKET_RECEIVE_FLAG) {
					bCheckFlag =
					KDrv_HDMIRx_Is_Input_Large_166MHz(u8HDMIInfoSource,
					&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bCheckFlag;
}

E_HDMI_ADDITIONAL_VIDEO_FORMAT Drv_HDMI_Check_Additional_Format(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = 0;
	E_HDMI_ADDITIONAL_VIDEO_FORMAT enAdditionalFormat = E_HDMI_NA;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				    HDMI_STATUS_VS_PACKET_RECEIVE_FLAG) {
					enAdditionalFormat =
					    KDrv_HDMIRx_Check_Additional_Format(u8HDMIInfoSource,
						&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return enAdditionalFormat;
}

E_XC_3D_INPUT_MODE Drv_HDMI_Get_3D_Structure(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = 0;
	E_XC_3D_INPUT_MODE en3DStructure = E_XC_3D_INPUT_MODE_NONE;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				    HDMI_STATUS_VS_PACKET_RECEIVE_FLAG) {
					if (KDrv_HDMIRx_Check_Additional_Format
					    (u8HDMIInfoSource,
					     &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource])
					     == E_HDMI_3D_FORMAT) {
						en3DStructure =
						KDrv_HDMIRx_Get_3D_Structure(u8HDMIInfoSource,
						&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
					}
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return en3DStructure;
}

E_HDMI_3D_EXT_DATA_T Drv_HDMI_Get_3D_Ext_Data(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = 0;
	E_HDMI_3D_EXT_DATA_T en3DExtendData = E_3D_EXT_DATA_MODE_MAX;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				    HDMI_STATUS_VS_PACKET_RECEIVE_FLAG) {
					if (KDrv_HDMIRx_Check_Additional_Format
					    (u8HDMIInfoSource,
					     &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource])
					     == E_HDMI_3D_FORMAT) {
						en3DExtendData =
					    KDrv_HDMIRx_Get_3D_Ext_Data(u8HDMIInfoSource,
						&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
					}
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return en3DExtendData;
}

void Drv_HDMI_Get_3D_Meta_Field(E_MUX_INPUTPORT enInputPortType, sHDMI_3D_META_FIELD *pdata)
{
	MS_U8 u8HDMIInfoSource = 0;
	sHDMI_3D_META_FIELD st3DMetaField = { 0 };
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource =
	    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
				if (pHDMIRes->ulPacketStatus[u8HDMIInfoSource] &
				    HDMI_STATUS_VS_PACKET_RECEIVE_FLAG) {
					if (KDrv_HDMIRx_Check_Additional_Format
					    (u8HDMIInfoSource,
					     &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource])
					     == E_HDMI_3D_FORMAT) {
						KDrv_HDMIRx_Get_3D_Meta_Field(u8HDMIInfoSource,
						&st3DMetaField,
						&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
					}
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	memcpy(pdata, &st3DMetaField, sizeof(sHDMI_3D_META_FIELD));
}

MS_U8 Drv_HDMI_GetSourceVersion(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 ucSourceVersion = HDMI_SOURCE_VERSION_NOT_SURE;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if (pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucSourceVersion
			!= HDMI_SOURCE_VERSION_NOT_SURE) {
			if (pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucSourceVersion
				== HDMI_SOURCE_VERSION_HDMI21)
				ucSourceVersion = HDMI_SOURCE_VERSION_HDMI21;
			else {
				if (pHDMIRes->stHDMIPollingInfo[ucPortIndex].bYUV420Flag)
					ucSourceVersion = HDMI_SOURCE_VERSION_HDMI20;
				else if (pHDMIRes->stHDMIPollingInfo[ucPortIndex].bHDMI20Flag)
					ucSourceVersion = HDMI_SOURCE_VERSION_HDMI20;
				else if (!pHDMIRes->stHDMIPollingInfo[ucPortIndex].bHDMIModeFlag)
					ucSourceVersion = HDMI_SOURCE_VERSION_HDMI14;
				else {
					ucSourceVersion =
					pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucSourceVersion;
				}
			}
		}
	}

	return ucSourceVersion;
}

MS_BOOL SYMBOL_WEAK Drv_HDMI_GetDEStableStatus(E_MUX_INPUTPORT enInputPortType)
{
	MS_BOOL bRet = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX))
			bRet = KDrv_HDMIRx_GetDEStableStatus(enInputPortType);
		else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bRet;
}

E_HDMI_HDCP_STATE Drv_HDMI_CheckHDCPState(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 ucHDCPState = HDMI_HDCP_NO_ENCRYPTION;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			// not Repeater mode
			if (!pHDMIRes->stHDMIPollingInfo[ucPortIndex].bIsRepeater) {
				// HDCP RX detection ENC_EN status
				if (KDrv_HDCP_GetEncryptionFlag(enInputPortType) &&
					(!KDrv_HDMIRx_clklose_det(enInputPortType))) {
					// [13] Ri: HDCP1.4
					if (pHDMIRes->bHDCP14RiFlag[ucPortIndex]) {
						ucHDCPState = HDMI_HDCP_1_4;

						DRV_HDMIRX_MSG_ERROR
					    ("** ENC_EN, HDMI HDCP state 1.4, port %d\r\n",
					     (enInputPortType - INPUT_PORT_DVI0));
					} else if (
					pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDCPState
					== HDMI_HDCP_2_2) {
						ucHDCPState = HDMI_HDCP_2_2;

						DRV_HDMIRX_MSG_ERROR
						("** ENC_EN, HDMI HDCP state 2.2, port %d\r\n",
						(enInputPortType - INPUT_PORT_DVI0));
					}
				}
			} else {
				ucHDCPState =
			    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDCPState;
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return ucHDCPState;
}

void Drv_HDCP_WriteX74(E_MUX_INPUTPORT enInputPortType, MS_U8 ucOffset, MS_U8 ucData)
{
	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX))
			KDrv_HDCP_WriteX74(enInputPortType, ucOffset, ucData);
		else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

MS_U8 Drv_HDCP_ReadX74(E_MUX_INPUTPORT enInputPortType, MS_U8 ucOffset)
{
	MS_U8 ucRetData = 0;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX))
			ucRetData = KDrv_HDCP_ReadX74(enInputPortType, ucOffset);
		else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return ucRetData;
}

void Drv_HDCP_SetRepeater(E_MUX_INPUTPORT enInputPortType, MS_BOOL bIsRepeater)
{
	MS_U8 ucBcaps = 0x00;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			pHDMIRes->stHDMIPollingInfo[ucPortIndex].bIsRepeater
				= bIsRepeater;

			ucBcaps = KDrv_HDCP_ReadX74(enInputPortType, 0x40); //Read Bcaps

			if (bIsRepeater)
				ucBcaps = ucBcaps | 0x40;
			else
				ucBcaps = ucBcaps & 0xBF;

			KDrv_HDCP_WriteX74(enInputPortType, 0x40, ucBcaps);
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

void Drv_HDCP_SetBstatus(E_MUX_INPUTPORT enInputPortType, MS_U16 usBstatus)
{
	MS_U8 ucBStatus = 0x00;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			ucBStatus = (MS_U8) (usBstatus & BMASK(7:0));
			// [6:0]: Device cnt
			KDrv_HDCP_WriteX74(enInputPortType, 0x41, ucBStatus);

			ucBStatus = (MS_U8) ((usBstatus & BMASK(15:8)) >> 8);
			//[10:8]: Depth and [12]: HDMI mode
			KDrv_HDCP_WriteX74(enInputPortType, 0x42, ucBStatus);

			DRV_HDMIRX_MSG_INFO("Set Bstatus=%x\r\n", ucBStatus);
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

void Drv_HDCP_SetHDMIMode(E_MUX_INPUTPORT enInputPortType, MS_BOOL bHDMIMode)
{
	MS_U8 ucBstatusHighByte = 0;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			ucBstatusHighByte = KDrv_HDCP_ReadX74(enInputPortType, 0x42);
			ucBstatusHighByte =
			    bHDMIMode ? (ucBstatusHighByte | 0x10) : (ucBstatusHighByte & 0xEF);

			KDrv_HDCP_WriteX74(enInputPortType, 0x42, ucBstatusHighByte);
		}
	} else
		DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
}

MS_U8 Drv_HDCP_GetInterruptStatus(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 ucRetIntStatus = 0;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			ucRetIntStatus = (KDrv_HDCP_getstatus(enInputPortType) & BMASK(15:8)) >> 8;

			pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDCPInt
				= ucRetIntStatus;

			if (ucRetIntStatus & BIT(0)) {
				DRV_HDMIRX_MSG_INFO("[%s@%d]Clear Ready", __func__, __LINE__);
				KDrv_HDCP_SetReady(enInputPortType, FALSE); // Clear Ready
			}

			KDrv_HDCP_ClearStatus(enInputPortType, (ucRetIntStatus & BMASK(4:0)));
			// Clear [0] Aksv, [1] An, [2] Km, [3] Bksv, [4] R0 interrupt status
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return ucRetIntStatus;
}

void Drv_HDCP_WriteKSVList(E_MUX_INPUTPORT enInputPortType, MS_U8 *pucKSV, MS_U32 ulDataLen)
{
	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDCP_WriteKSVList(enInputPortType, pucKSV, ulDataLen);
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

void Drv_HDCP_SetVPrime(E_MUX_INPUTPORT enInputPortType, MS_U8 *pucVPrime)
{
	MS_U8 ucCnt = 0;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			for (ucCnt = 0; ucCnt < HDMI_HDCP_VPRIME_LENGTH; ucCnt++) {
				KDrv_HDCP_WriteX74(enInputPortType, 0x20 + ucCnt,
						   *(pucVPrime + ucCnt));
			}

			DRV_HDMIRX_MSG_INFO("[%s@%d]Set Ready", __func__, __LINE__);
			KDrv_HDCP_SetReady(enInputPortType, TRUE); // Set Ready
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

E_HDMI_HDCP_ENCRYPTION_STATE Drv_HDMI_CheckHDCPENCState(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 ucHDCPENCState = HDMI_HDCP_NOT_ENCRYPTION;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			// HDCP RX detection ENC_EN status
			if (KDrv_HDCP_GetEncryptionFlag(enInputPortType) &&
				(!KDrv_HDMIRx_clklose_det(enInputPortType))) {
				// [13] Ri: HDCP1.4
				if (pHDMIRes->bHDCP14RiFlag[ucPortIndex]) {
					ucHDCPENCState = HDMI_HDCP_1_4_ENCRYPTION;

					DRV_HDMIRX_MSG_ERROR
					("** ENC_EN, HDMI HDCP ENC state 1.4, port %d\r\n",
					(enInputPortType - INPUT_PORT_DVI0));
				} else if (pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDCPState
				== HDMI_HDCP_2_2) {
					ucHDCPENCState = HDMI_HDCP_2_2_ENCRYPTION;

					DRV_HDMIRX_MSG_ERROR
					("** ENC_EN, HDMI HDCP ENC state 2.2, port %d\r\n",
					(enInputPortType - INPUT_PORT_DVI0));
				}
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return ucHDCPENCState;
}

void Drv_HDMI_DataRtermControl(E_MUX_INPUTPORT enInputPortType, MS_BOOL bDataRtermEnable)
{
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_DataRtermControl(enInputPortType, bDataRtermEnable);
			pHDMIRes->bDataEnableFlag[ucPortIndex] = TRUE;
		}
	} else
		DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
}

MS_HDCP_STATUS_INFO_t Drv_HDMI_GetHDCPStatusPort(E_MUX_INPUTPORT enInputPortType)
{
	MS_HDCP_STATUS_INFO_t stHDCPStatusInfo = { 0 };
	MS_U16 u16HDCPStatus = 0;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			u16HDCPStatus = KDrv_HDCP_getstatus(enInputPortType);
			stHDCPStatusInfo.b_St_HDMI_Mode =
			((u16HDCPStatus & BIT(0)) ? TRUE : FALSE);
			stHDCPStatusInfo.b_St_Reserve_1 =
			((u16HDCPStatus & BIT(1)) ? TRUE : FALSE);
			stHDCPStatusInfo.b_St_HDCP_Ver =
			((u16HDCPStatus & BIT(2)) ? TRUE : FALSE);
			stHDCPStatusInfo.b_St_AVMute =
			((u16HDCPStatus & BIT(3)) ? TRUE : FALSE);
			stHDCPStatusInfo.b_St_ENC_En =
			((u16HDCPStatus & BIT(4)) ? TRUE : FALSE);
			stHDCPStatusInfo.b_St_ENC_Dis =
			((u16HDCPStatus & BIT(5)) ? TRUE : FALSE);
			stHDCPStatusInfo.b_St_AKsv_Rcv =
			((u16HDCPStatus & BIT(6)) ? TRUE : FALSE);
			stHDCPStatusInfo.b_St_Reserve_7 =
			((u16HDCPStatus & BIT(7)) ? TRUE : FALSE);
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return stHDCPStatusInfo;
}

MS_U8 Drv_HDMI_GetSCDCValue(E_MUX_INPUTPORT enInputPortType, MS_U8 ucOffset)
{
	MS_U8 ucRetData = 0;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX))
			ucRetData = KDrv_HDMIRx_GetSCDCValue(enInputPortType, ucOffset);
		else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return ucRetData;
}

MS_U32 Drv_HDMI_GetTMDSRatesKHz(E_MUX_INPUTPORT enInputPortType, E_HDMI_GET_TMDS_RATES enType)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_U32 ulClockRates = 0, ulRetRates = 0;
	MS_U8 u8ColorDepth = 0;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (!pHDMIRes->stHDMIPollingInfo[ucPortIndex].bNoInputFlag) {
				ulClockRates =
			    1000 *
			    pHDMIRes->stHDMIPollingInfo[ucPortIndex].usClockCount;
			}

			switch (enType) {
			case E_HDMI_GET_TMDS_BIT_RATES:
				if (pHDMIRes->stHDMIPollingInfo[ucPortIndex].bHDMI20Flag)
					ulRetRates = ulClockRates * 40;
				else
					ulRetRates = ulClockRates * 10;
				break;

			case E_HDMI_GET_TMDS_CHARACTER_RATES:
				if (pHDMIRes->stHDMIPollingInfo[ucPortIndex].bHDMI20Flag)
					ulRetRates = ulClockRates * 4;
				else
					ulRetRates = ulClockRates;

				break;

			case E_HDMI_GET_TMDS_CLOCK_RATES:
				ulRetRates = ulClockRates;

				break;

			case E_HDMI_GET_PIXEL_CLOCK_RATES:
				u8HDMIInfoSource =
				pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;
				u8ColorDepth =
			    KDrv_HDMIRx_GetGCPColorDepth(enInputPortType,
				 pHDMIRes->ulPacketStatus[u8HDMIInfoSource]);

				if (pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucSourceVersion
					== HDMI_SOURCE_VERSION_HDMI20)
					ulClockRates = ulClockRates * 4;

				switch (u8ColorDepth) {
				case HDMI_COLOR_DEPTH_8BIT:
					ulRetRates = ulClockRates;
					break;

				case HDMI_COLOR_DEPTH_10BIT:
					ulRetRates = ulClockRates * 8 / 10;
					break;

				case HDMI_COLOR_DEPTH_12BIT:
					ulRetRates = ulClockRates * 8 / 12;
					break;

				case HDMI_COLOR_DEPTH_16BIT:
					ulRetRates = ulClockRates * 8 / 16;
					break;

				default:

					break;
				};

				break;

			default:

				break;
			};
		}
	}

	return ulRetRates;
}

MS_BOOL Drv_HDMI_GetSCDCCableDetectFlag(E_MUX_INPUTPORT enInputPortType)
{
	MS_BOOL bReValue = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX))
			bReValue = KDrv_HDMIRx_GetSCDCCableDetectFlag(enInputPortType);
		else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bReValue;
}

void Drv_HDMI_AssignSourceSelect(E_MUX_INPUTPORT enInputPortType)
{
	MS_U8 u8HDMIInfoSource = HDMI_INFO_SOURCE_NONE;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			u8HDMIInfoSource = KDrv_HDMIRx_GetSourceSelect(enInputPortType);

			pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource
				= u8HDMIInfoSource;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

MS_BOOL MDrv_HDMI_GetPacketStatus(E_MUX_INPUTPORT enInputPortType,
				  MS_HDMI_EXTEND_PACKET_RECEIVE_t *pExtendPacketReceive)
{
	MS_BOOL bGetStatusFlag = FALSE;
	MS_BOOL bInvalidFalg = FALSE;
	MS_U16 usCopiedLength = sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t);
	MS_HDMI_EXTEND_PACKET_RECEIVE_t stExtendPacketReceive = { 0 };
	MS_U8 u8HDMIInfoSource = HDMI_INFO_SOURCE_NONE;
	MS_U32 u32PacketStatus = 0;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	if ((pExtendPacketReceive == NULL) || (pExtendPacketReceive->u16Size == 0)) {
		bInvalidFalg = TRUE;
		DRV_HDMIRX_MSG_ERROR
		("MDrv_HDMI_Extend_Packet_Received: Null parameter or Wrong u16Size\n");
	}

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			if (!bInvalidFalg) {
				u8HDMIInfoSource =
			    pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

				if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
					u32PacketStatus =
				    pHDMIRes->ulPacketStatus[u8HDMIInfoSource];
				}

				if (pExtendPacketReceive->u16Size <
				    sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t)) {
					usCopiedLength = pExtendPacketReceive->u16Size;
				}

				stExtendPacketReceive.bPKT_MPEG_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_MPEG_PACKET_RECEIVE_FLAG) ? TRUE
				    : FALSE;
				stExtendPacketReceive.bPKT_AUI_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AUDIO_PACKET_RECEIVE_FLAG) ? TRUE
				    : FALSE;
				stExtendPacketReceive.bPKT_SPD_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_SPD_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_AVI_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_GC_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_GCP_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_ASAMPLE_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AUDIO_SAMPLE_PACKET_RECEIVE_FLAG)
				    ? TRUE : FALSE;
				stExtendPacketReceive.bPKT_ACR_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_ACR_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_VS_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_VS_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_NULL_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_NULL_PACKET_RECEIVE_FLAG) ? TRUE
				    : FALSE;
				stExtendPacketReceive.bPKT_ISRC2_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_ISRC2_PACKET_RECEIVE_FLAG) ? TRUE
				    : FALSE;
				stExtendPacketReceive.bPKT_ISRC1_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_ISRC1_PACKET_RECEIVE_FLAG) ? TRUE
				    : FALSE;
				stExtendPacketReceive.bPKT_ACP_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_ACP_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_ONEBIT_AUD_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_DSD_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_GM_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_GM_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_HBR_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_HBR_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_VBI_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_VBI_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_HDR_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_HDR_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_RSV_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_RESERVED_PACKET_RECEIVE_FLAG) ?
				    TRUE : FALSE;
				stExtendPacketReceive.bPKT_EDR_VALID =
				    (u32PacketStatus & HDMI_STATUS_EDR_VALID_FLAG) ? TRUE : FALSE;
				stExtendPacketReceive.bPKT_AUDIO_DST_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AUDIO_DST_RECEIVE_FLAG) ? TRUE :
				    FALSE;
				stExtendPacketReceive.bPKT_AUDIO_3D_ASP_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AUDIO_3D_ASP_RECEIVE_FLAG) ? TRUE
				    : FALSE;
				stExtendPacketReceive.bPKT_AUDIO_3D_DSD_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AUDIO_3D_DSD_RECEIVE_FLAG) ? TRUE
				    : FALSE;
				stExtendPacketReceive.bPKT_AUDIO_METADATA_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AUDIO_METADATA_RECEIVE_FLAG) ?
				    TRUE : FALSE;
				stExtendPacketReceive.bPKT_AUDIO_MULTI_ASP_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AUDIO_MULTI_ASP_RECEIVE_FLAG) ?
				    TRUE : FALSE;
				stExtendPacketReceive.bPKT_AUDIO_MULTI_DSD_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_AUDIO_MULTI_DSD_RECEIVE_FLAG) ?
				    TRUE : FALSE;
				stExtendPacketReceive.bPKT_EM_RECEIVE =
				    (u32PacketStatus & HDMI_STATUS_EM_PACKET_RECEIVE_FLAG) ? TRUE :
				    FALSE;

				memcpy(pExtendPacketReceive, &stExtendPacketReceive,
				       usCopiedLength);

				bGetStatusFlag = TRUE;
			}
		}
	}

	return bGetStatusFlag;
}

MS_BOOL MDrv_HDMI_GetPacketContent(E_MUX_INPUTPORT enInputPortType,
				   MS_HDMI_PACKET_STATE_t enPacketType, MS_U8 u8PacketLength,
				   MS_U8 *pu8PacketContent)
{
	MS_BOOL bGetContentFlag = FALSE;
	MS_U8 u8HDMIInfoSource = HDMI_INFO_SOURCE_NONE;
	MS_U32 ulPacketStatus = 0;
	MS_BOOL bSwitchContentFlag = FALSE;
	MS_U8 u8temp = 0;
	MS_U8 u8PacketData = 0;
	MS_U8 ucPortIndex = 0;

	ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);

	do {
		if (pHDMIRes == NULL) {
			DRV_HDMIRX_MSG_ERROR
			("Resource Not Initialize\r\n");
			break;
		}
		if ((enInputPortType < INPUT_PORT_DVI0) ||
		   (enInputPortType >= INPUT_PORT_DVI_MAX)) {
			DRV_HDMIRX_MSG_ERROR
			("Input Port Type Error\r\n");
			break;
		}
		if (pu8PacketContent == NULL) {
			DRV_HDMIRX_MSG_ERROR
			("Pkt Pointer NULL\r\n");
			break;
		}

		u8HDMIInfoSource =
		pHDMIRes->stHDMIPollingInfo[ucPortIndex].ucHDMIInfoSource;

		if (u8HDMIInfoSource >= HDMI_INFO_SOURCE_MAX) {
			DRV_HDMIRX_MSG_ERROR
			("Source Error\r\n");
			break;
		}

		ulPacketStatus =
		pHDMIRes->ulPacketStatus[u8HDMIInfoSource];

		switch (enPacketType) {
		case PKT_MPEG:
			if (ulPacketStatus &
				HDMI_STATUS_MPEG_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_MPEG_PACKET_LENGTH)
					u8PacketLength = HDMI_MPEG_PACKET_LENGTH;
			}
			break;

		case PKT_AUI:
			if (ulPacketStatus &
				HDMI_STATUS_AUDIO_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_AUDIO_PACKET_LENGTH)
					u8PacketLength = HDMI_AUDIO_PACKET_LENGTH;
			}
			break;

		case PKT_SPD:
			if (ulPacketStatus &
				HDMI_STATUS_SPD_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_SPD_PACKET_LENGTH)
					u8PacketLength = HDMI_SPD_PACKET_LENGTH;
			}
			break;

		case PKT_AVI:
			if (ulPacketStatus &
				HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_AVI_PACKET_LENGTH)
					u8PacketLength = HDMI_AVI_PACKET_LENGTH;
			}
			break;

		case PKT_GC:
			if (ulPacketStatus &
				HDMI_STATUS_GCP_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_GCP_PACKET_LENGTH)
					u8PacketLength = HDMI_GCP_PACKET_LENGTH;
			}
			break;

		case PKT_ASAMPLE:
			break;

		case PKT_ACR:
			break;

		case PKT_VS:
			if (ulPacketStatus &
				HDMI_STATUS_VS_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_VS_PACKET_LENGTH)
					u8PacketLength = HDMI_VS_PACKET_LENGTH;
			}
			break;

		case PKT_NULL:
			break;

		case PKT_ISRC2:
			if (ulPacketStatus &
				HDMI_STATUS_ISRC2_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_ISRC2_PACKET_LENGTH)
					u8PacketLength = HDMI_ISRC2_PACKET_LENGTH;
			}
			break;

		case PKT_ISRC1:
			if (ulPacketStatus &
				HDMI_STATUS_ISRC1_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_ISRC1_PACKET_LENGTH)
					u8PacketLength = HDMI_ISRC1_PACKET_LENGTH;
			}
			break;

		case PKT_ACP:
			if (ulPacketStatus &
				HDMI_STATUS_ACP_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength > HDMI_ACP_PACKET_LENGTH)
					u8PacketLength = HDMI_ACP_PACKET_LENGTH;
			}
			break;

		case PKT_ONEBIT_AUD:
			break;

		case PKT_GM:
			break;

		case PKT_HBR:
			break;

		case PKT_VBI:
			break;

		case PKT_HDR:
			if (ulPacketStatus &
				HDMI_STATUS_HDR_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;
				bSwitchContentFlag = TRUE;

				if (u8PacketLength > HDMI_HDR_PACKET_LENGTH)
					u8PacketLength = HDMI_HDR_PACKET_LENGTH;
			}
			break;

		case PKT_RSV:
			break;

		case PKT_EDR:
			break;

		case PKT_MULTI_VS:
			if (u8PacketLength > HDMI_GET_MULTIVS_PACKET_LENGTH)
				u8PacketLength = HDMI_GET_MULTIVS_PACKET_LENGTH;
			break;

		default:
			if (u8PacketLength > HDMI_GET_PACKET_LENGTH)
				u8PacketLength = HDMI_GET_PACKET_LENGTH;
			break;
		}

if (bGetContentFlag) {
	for (u8temp = 0; u8temp < u8PacketLength; u8temp++) {
		if (KDrv_HDMIRx_get_packet_value
		(u8HDMIInfoSource, enPacketType, u8temp,
		 &u8PacketData,
		 &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource])) {
			if (bSwitchContentFlag) {
				if (u8temp % 2) {
					pu8PacketContent[u8temp - 1]
					= u8PacketData;
				} else {
					if (u8temp ==
						(u8PacketLength
						 - 1)) {
						pu8PacketContent[u8temp]
						= u8PacketData;
					} else {
						pu8PacketContent[u8temp + 1]
						= u8PacketData;
					}
				}
			} else {
				pu8PacketContent[u8temp] =
					u8PacketData;
			}
		}
	}
}

		switch (enPacketType) {
		case PKT_CHANNEL_STATUS:
			if (ulPacketStatus &
				HDMI_STATUS_AUDIO_SAMPLE_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				if (u8PacketLength >
					HDMI_AUDIO_CHANNEL_STATUS_LENGTH) {
					u8PacketLength =
						HDMI_AUDIO_CHANNEL_STATUS_LENGTH;
				}

				for (u8temp = 0; u8temp < u8PacketLength;
					 u8temp++) {
					pu8PacketContent[u8temp] =
					KDrv_HDMIRx_audio_channel_status
					(u8HDMIInfoSource, u8temp,
					 &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
				}
			}
			break;

		case PKT_MULTI_VS:
			if (ulPacketStatus &
				HDMI_STATUS_VS_PACKET_RECEIVE_FLAG) {
				bGetContentFlag = TRUE;

				pu8PacketContent[HDMI_MULTI_VS_PACKET_LENGTH]
				= KDrv_HDMIRx_GetDataInfo
				(E_HDMI_GET_MULTIVS_COUNT,
				 u8HDMIInfoSource,
				 &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);

				if (pu8PacketContent[HDMI_MULTI_VS_PACKET_LENGTH]
					> 4) {
					pu8PacketContent[HDMI_MULTI_VS_PACKET_LENGTH]
					= 4;
				}

				u8PacketLength =
				pu8PacketContent[HDMI_MULTI_VS_PACKET_LENGTH]
				* (HDMI_VS_PACKET_LENGTH - 1);

				if (u8PacketLength >
					HDMI_MULTI_VS_PACKET_LENGTH)
					u8PacketLength = HDMI_MULTI_VS_PACKET_LENGTH;

				for (u8temp = 0; u8temp < u8PacketLength;
					 u8temp++) {
					if (KDrv_HDMIRx_get_packet_value
						(u8HDMIInfoSource, enPacketType,
						 u8temp, &u8PacketData,
						 &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]))
						pu8PacketContent[u8temp] = u8PacketData;
				}
			}
			break;

		default:
			break;
		}
	} while (FALSE);

	return bGetContentFlag;
}

MS_U16 Drv_HDMI_GetTimingInfo(E_MUX_INPUTPORT enInputPortType, E_HDMI_GET_DATA_INFO enInfoType)
{
	MS_U16 usInfoData = 0;
	MS_U8 u8HDMIInfoSource = HDMI_INFO_SOURCE_NONE;

	if (pHDMIRes != NULL) {
		u8HDMIInfoSource = KDrv_HDMIRx_GetSourceSelect(enInputPortType);

		if (u8HDMIInfoSource < HDMI_INFO_SOURCE_MAX) {
			switch (enInfoType) {
			case E_HDMI_GET_HDE:
			case E_HDMI_GET_VDE:
			case E_HDMI_GET_HTT:
			case E_HDMI_GET_VTT:
			case E_HDMI_GET_CHIP_HDCP_CAPABILITY:
				{
					usInfoData =
				    KDrv_HDMIRx_GetDataInfo(enInfoType, u8HDMIInfoSource,
				    &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
				}
				break;

			case E_HDMI_GET_AUDIO_PROTECT_INFO:
				{
					usInfoData =
				    (MS_U16)KDrv_HDMIRx_audio_content_protection_info(
				    u8HDMIInfoSource,
					&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
				}
				break;

			case E_HDMI_GET_ERROR_STATUS:
				{
				usInfoData =
				KDrv_HDMIRx_err_status_update(u8HDMIInfoSource,
				usInfoData, TRUE,
				&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);

				if (usInfoData > 0) {
					if (KDrv_HDMIRx_err_status_update(
						u8HDMIInfoSource, usInfoData, FALSE,
					     &pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]) > 0) {
						// Do nothing for coverity issue
					}
				}
				}
				break;

			case E_HDMI_GET_ISRC1_HEADER_INFO:
				{
					MS_U32 ulPacketStatus = 0;

					ulPacketStatus =
				    pHDMIRes->ulPacketStatus[u8HDMIInfoSource];

					if (ulPacketStatus &
						HDMI_STATUS_ISRC1_PACKET_RECEIVE_FLAG) {
						usInfoData =
						(MS_U16)KDrv_HDMIRx_GetISRC1HeaderInfo(
						u8HDMIInfoSource,
						&pHDMIRes->stHDMIPollingInfo[u8HDMIInfoSource]);
					}
				}
				break;

			default:
				break;
			}

		}
	}

	return usInfoData;
}

E_HDMI_HDCP_STATE Drv_HDCP_GetAuthVersion(E_MUX_INPUTPORT enInputPortType)
{
	E_HDMI_HDCP_STATE enHDCPAuthVersion = E_HDCP_NO_ENCRYPTION;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX))
			enHDCPAuthVersion = KDrv_HDCP_GetAuthVersion(enInputPortType, TRUE);
		else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return enHDCPAuthVersion;
}

MS_BOOL Drv_HDMI_ForcePowerDownPort(E_MUX_INPUTPORT enInputPortType, MS_BOOL bPowerDown)
{
	MS_BOOL bDoneFlag = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_ForcePowerDownPort(enInputPortType,
			&pHDMIRes->stHDMIPollingInfo[HDMI_GET_PORT_SELECT(enInputPortType)],
			bPowerDown);
			bDoneFlag = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bDoneFlag;
}

MS_BOOL Drv_HDMI_GetCRCValue(E_MUX_INPUTPORT enInputPortType, MS_DVI_CHANNEL_TYPE u8Channel,
			      MS_U16 *pu16CRCVal)
{
	MS_BOOL bRetValue = FALSE;

	if ((pHDMIRes != NULL) && (pu16CRCVal != 0)) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			bRetValue =
			KDrv_HDMIRx_GetCRCValue(enInputPortType, u8Channel, pu16CRCVal,
			&pHDMIRes->stHDMIPollingInfo[HDMI_GET_PORT_SELECT(enInputPortType)]);
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bRetValue;
}

MS_BOOL Drv_HDMI_Dither_Set(E_MUX_INPUTPORT enInputPortType,
			     E_DITHER_TYPE enDitherType, MS_BOOL ubRoundEnable)
{
	MS_BOOL bret = FALSE;

	if (pHDMIRes != NULL) {
		if ((enInputPortType >= INPUT_PORT_DVI0) &&
			(enInputPortType < INPUT_PORT_DVI_MAX)) {
			KDrv_HDMIRx_DitherEnable(enInputPortType, enDitherType, ubRoundEnable);
			bret = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bret;
}

MS_BOOL Drv_HDCP22_PollingReadDone(MS_U8 ucPortIdx)
{
	MS_BOOL bReadDoneFlag = FALSE;

	if (pHDMIRes != NULL) {
		if (ucPortIdx < HDMI_GET_PORT_SELECT(INPUT_PORT_DVI_MAX)) {
			if (KDrv_HDCP22_PollingReadDone(ucPortIdx))
				bReadDoneFlag = TRUE;
			else if (GET_HDMI_FLAG
				(pHDMIRes->ucHDCPReadDoneIndex, BIT(ucPortIdx))) {
				CLR_HDMI_FLAG(
					pHDMIRes->ucHDCPReadDoneIndex, BIT(ucPortIdx));

				bReadDoneFlag = TRUE;
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}

	return bReadDoneFlag;
}

void Drv_HDCP22_PortInit(MS_U8 ucPortIdx)
{
	if (pHDMIRes != NULL) {
		if (ucPortIdx < HDMI_GET_PORT_SELECT(INPUT_PORT_DVI_MAX)) {
			KDrv_HDCP22_PortInit(ucPortIdx);

			pHDMIRes->ucHDCP22RecvIDListSend[ucPortIdx] = FALSE;
			pHDMIRes->bHDCP22EnableFlag[ucPortIdx] = TRUE;
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

void Drv_HDCP22_RecvMsg(MS_U8 ucPortIdx, MS_U8 *ucMsgData)
{
	MS_U8 ubRecIDListSetDone = 0;

	if ((pHDMIRes != NULL) && (ucMsgData != NULL)) {
		if (ucPortIdx < HDMI_GET_PORT_SELECT(INPUT_PORT_DVI_MAX))
			KDrv_HDCP22_RecvMsg(ucPortIdx, ucMsgData);
		else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

void Drv_HDCP22_SendMsg(MS_U8 ucPortType, MS_U8 ucPortIdx, MS_U8 *pucData, MS_U32 dwDataLen,
			 void *pDummy)
{
	MS_U8 ubRecIDListSetDone = 0;

	if ((pHDMIRes != NULL) && (pucData != NULL)) {
		if (ucPortIdx < HDMI_GET_PORT_SELECT(INPUT_PORT_DVI_MAX)) {
			KDrv_HDCP22_SendMsg(ucPortType, ucPortIdx, pucData, dwDataLen, pDummy,
					    &ubRecIDListSetDone);

			if (*(MS_U8 *) (pucData) == 0xC) {
				pHDMIRes->ucHDCP22RecvIDListSend[ucPortIdx] =
				    ubRecIDListSetDone;
			}
		} else
			DRV_HDMIRX_MSG_ERROR("Wrong Input Port Index !!\n");
	}
}

MS_BOOL Drv_HDMI_Get_EMP(E_MUX_INPUTPORT enInputPortType,
			  E_HDMI_EMPACKET_TYPE enEmpType, MS_U8 u8CurrentPacketIndex,
			  MS_U8 *pu8TotalPacketNumber, MS_U8 *pu8EmPacket)
{
	EN_KDRV_HDMI_EMP_TYPE enXCEmpType;

	if (pu8TotalPacketNumber == NULL || pu8EmPacket == NULL) {
		DRV_HDMIRX_MSG_ERROR("HDMI EMP pointer NULL\r\n");
		return FALSE;
	}

	switch (enEmpType) {
	case E_HDMI_EMP_DOLBY_HDR:
		enXCEmpType = E_KDRV_HDMI_EMP_DOLBY_HDR;
		break;

	case E_HDMI_EMP_DSC:
		enXCEmpType = E_KDRV_HDMI_EMP_DSC;
		break;

	case E_HDMI_EMP_DYNAMIC_HDR:
		enXCEmpType = E_KDRV_HDMI_EMP_DYNAMIC_HDR;
		break;

	case E_HDMI_EMP_VRR:
		enXCEmpType = E_KDRV_HDMI_EMP_VRR;
		break;

	case E_HDMI_EMP_QFT:
		enXCEmpType = E_KDRV_HDMI_EMP_QFT;
		break;

	case E_HDMI_EMP_QMS:
		enXCEmpType = E_KDRV_HDMI_EMP_QMS;
		break;

	case E_HDMI_EMP_MAX:
		enXCEmpType = E_KDRV_HDMI_EMP_MAX;
		break;

	default:
		enXCEmpType = E_KDRV_HDMI_EMP_MAX;
		break;
	};

	DRV_HDMIRX_MSG_ERROR(" Get EMP Kernel Path!!!!\n");
	if (KDrv_XC_GetEmPacket(HDMI_GET_PORT_SELECT(enInputPortType),
				enXCEmpType,
				u8CurrentPacketIndex, pu8TotalPacketNumber, pu8EmPacket) != TRUE) {
		return FALSE;
	}

	return TRUE;
}


MS_BOOL MDrv_HDMI_IOCTL(MS_U32 ulCommand, void *pBuffer, MS_U32 ulBufferSize)
{
	MS_BOOL bret = FALSE;

	switch (ulCommand) {
	case E_HDMI_INTERFACE_CMD_GET_HDCP_STATE:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_CHECK_HDCP_STATE)) {
				((stCMD_HDMI_CHECK_HDCP_STATE *) pBuffer)->ucHDCPState =
				    (MS_U8)
				    Drv_HDMI_CheckHDCPState(
				    ((stCMD_HDMI_CHECK_HDCP_STATE *)pBuffer)->enInputPortType);

				bret = TRUE;
			} else {
				((stCMD_HDMI_CHECK_HDCP_STATE *) pBuffer)->ucHDCPState =
				    HDMI_HDCP_NO_ENCRYPTION;
			}
		}
		break;

	/*************************** HDCP Repeater ***************************/
	case E_HDMI_INTERFACE_CMD_WRITE_X74:
		{
			if (ulBufferSize == sizeof(stCMD_HDCP_WRITE_X74)) {
				Drv_HDCP_WriteX74(
				((stCMD_HDCP_WRITE_X74 *) pBuffer)->enInputPortType,
				((stCMD_HDCP_WRITE_X74 *) pBuffer)->ucOffset,
				((stCMD_HDCP_WRITE_X74 *) pBuffer)->ucData);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_READ_X74:
		{
			if (ulBufferSize == sizeof(stCMD_HDCP_READ_X74)) {
				((stCMD_HDCP_READ_X74 *) pBuffer)->ucRetData =
				    Drv_HDCP_ReadX74(
					((stCMD_HDCP_READ_X74 *) pBuffer)->enInputPortType,
					((stCMD_HDCP_READ_X74 *) pBuffer)->ucOffset);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_SET_REPEATER:
		{
			if (ulBufferSize == sizeof(stCMD_HDCP_SET_REPEATER)) {
				Drv_HDCP_SetRepeater(
				((stCMD_HDCP_SET_REPEATER *) pBuffer)->enInputPortType,
				((stCMD_HDCP_SET_REPEATER *) pBuffer)->bIsRepeater);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_SET_BSTATUS:
		{
			if (ulBufferSize == sizeof(stCMD_HDCP_SET_BSTATUS)) {
				Drv_HDCP_SetBstatus(
				((stCMD_HDCP_SET_BSTATUS *) pBuffer)->enInputPortType,
				((stCMD_HDCP_SET_BSTATUS *) pBuffer)->usBstatus);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_SET_HDMI_MODE:
		{
			if (ulBufferSize == sizeof(stCMD_HDCP_SET_HDMI_MODE)) {
				Drv_HDCP_SetHDMIMode(
				((stCMD_HDCP_SET_HDMI_MODE *) pBuffer)->enInputPortType,
				((stCMD_HDCP_SET_HDMI_MODE *) pBuffer)->bHDMIMode);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_INTERRUPT_STATUS:
		{
			if (ulBufferSize == sizeof(stCMD_HDCP_GET_INTERRUPT_STATUS)) {
				((stCMD_HDCP_GET_INTERRUPT_STATUS *) pBuffer)->ucRetIntStatus =
				    Drv_HDCP_GetInterruptStatus(
				    ((stCMD_HDCP_GET_INTERRUPT_STATUS *) pBuffer)->enInputPortType);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_WRITE_KSV_LIST:
		{
			MS_U8 ucKSVBuffer[HDMI_HDCP_KSV_LIST_LENGTH] = { 0 };
			MS_U8 *pucKSVBuffer = NULL;
			MS_U32 ultemp = 0;
			MS_U32 ulLength = 0;

			if (ulBufferSize == sizeof(stCMD_HDCP_WRITE_KSV_LIST)) {
				ulLength = ((stCMD_HDCP_WRITE_KSV_LIST *) pBuffer)->ulLen;
				pucKSVBuffer = ((stCMD_HDCP_WRITE_KSV_LIST *) pBuffer)->pucKSV;

				if (ulLength > HDMI_HDCP_KSV_LIST_LENGTH)
					ulLength = HDMI_HDCP_KSV_LIST_LENGTH;

				for (ultemp = 0; ultemp < ulLength; ultemp++)
					ucKSVBuffer[ultemp] = pucKSVBuffer[ultemp];

				Drv_HDCP_WriteKSVList(
				((stCMD_HDCP_WRITE_KSV_LIST *) pBuffer)->enInputPortType,
				ucKSVBuffer, ulLength);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_SET_VPRIME:
		{
			if (ulBufferSize == sizeof(stCMD_HDCP_SET_VPRIME)) {
				Drv_HDCP_SetVPrime(
				((stCMD_HDCP_SET_VPRIME *) pBuffer)->enInputPortType,
				((stCMD_HDCP_SET_VPRIME *) pBuffer)->pucVPrime);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_DATA_RTERM_CONTROL:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_DATA_RTERM_CONTROL)) {
				Drv_HDMI_DataRtermControl(
				((stCMD_HDMI_DATA_RTERM_CONTROL *)pBuffer)->enInputPortType,
				((stCMD_HDMI_DATA_RTERM_CONTROL *)pBuffer)->bDataRtermEnable);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_SCDC_VALUE:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_GET_SCDC_VALUE)) {
				((stCMD_HDMI_GET_SCDC_VALUE *) pBuffer)->ucRetData =
				    Drv_HDMI_GetSCDCValue(
					((stCMD_HDMI_GET_SCDC_VALUE *) pBuffer)->enInputPortType,
					((stCMD_HDMI_GET_SCDC_VALUE *) pBuffer)->ucOffset);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_TMDS_RATES_KHZ:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_GET_TMDS_RATES_KHZ)) {
				((stCMD_HDMI_GET_TMDS_RATES_KHZ *) pBuffer)->ulRetRates =
				    Drv_HDMI_GetTMDSRatesKHz(
					((stCMD_HDMI_GET_TMDS_RATES_KHZ *)pBuffer)->enInputPortType,
					((stCMD_HDMI_GET_TMDS_RATES_KHZ *)pBuffer)->enType);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_SCDC_CABLE_DETECT:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_GET_SCDC_CABLE_DETECT)) {
				((stCMD_HDMI_GET_SCDC_CABLE_DETECT *) pBuffer)->bCableDetectFlag =
			    Drv_HDMI_GetSCDCCableDetectFlag(
			    ((stCMD_HDMI_GET_SCDC_CABLE_DETECT *) pBuffer)->enInputPortType);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_PACKET_STATUS:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_GET_PACKET_STATUS)) {
				stCMD_HDMI_GET_PACKET_STATUS *pParameters =
				    (stCMD_HDMI_GET_PACKET_STATUS *) pBuffer;

				if (MDrv_HDMI_GetPacketStatus
				    (pParameters->enInputPortType,
				     pParameters->pExtendPacketReceive)) {
					bret = TRUE;
				}
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_PACKET_CONTENT:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_GET_PACKET_CONTENT)) {
				stCMD_HDMI_GET_PACKET_CONTENT *pParameters =
				    (stCMD_HDMI_GET_PACKET_CONTENT *) pBuffer;

				if (MDrv_HDMI_GetPacketContent
				    (pParameters->enInputPortType, pParameters->enPacketType,
				     pParameters->u8PacketLength, pParameters->pu8PacketContent)) {
					bret = TRUE;
				}
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_HDR_METADATA:
		{
			MS_U8 u8PacketContent[HDMI_GET_PACKET_LENGTH] = { 0 };

			if (ulBufferSize == sizeof(stCMD_HDMI_GET_HDR_METADATA)) {
				stCMD_HDMI_GET_HDR_METADATA *pParameters =
				    (stCMD_HDMI_GET_HDR_METADATA *) pBuffer;

				if (MDrv_HDMI_GetPacketContent
				    (pParameters->enInputPortType, PKT_HDR, HDMI_GET_PACKET_LENGTH,
				     u8PacketContent)) {
					_drv_HDMI_ParseHDRInfoFrame(u8PacketContent,
					pParameters->pstHDRMetadata,
					TRUE);

					bret = TRUE;
				}
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_AVI_PARSING_INFO:
		{
			MS_U8 u8PacketContent[HDMI_GET_PACKET_LENGTH] = { 0 };

			if (ulBufferSize == sizeof(stCMD_HDMI_GET_AVI_PARSING_INFO)) {
				stCMD_HDMI_GET_AVI_PARSING_INFO *pParameters =
				    (stCMD_HDMI_GET_AVI_PARSING_INFO *) pBuffer;

				if (MDrv_HDMI_GetPacketContent
				    (pParameters->enInputPortType, PKT_AVI, HDMI_GET_PACKET_LENGTH,
				     u8PacketContent)) {
					_drv_HDMI_ParseAVIInfoFrame(u8PacketContent,
					pParameters->pstAVIParsingInfo);

					bret = TRUE;
				}
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_VS_PARSING_INFO:
		{
			MS_U8 u8PacketContent[HDMI_GET_PACKET_LENGTH] = { 0 };

			if (ulBufferSize == sizeof(stCMD_HDMI_GET_VS_PARSING_INFO)) {
				stCMD_HDMI_GET_VS_PARSING_INFO *pParameters =
				    (stCMD_HDMI_GET_VS_PARSING_INFO *) pBuffer;

				if (MDrv_HDMI_GetPacketContent
				    (pParameters->enInputPortType, PKT_VS, HDMI_GET_PACKET_LENGTH,
				     u8PacketContent)) {
					_drv_HDMI_ParseVSInfoFrame(u8PacketContent,
					pParameters->pstVSParsingInfo);

					bret = TRUE;
				}
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_GC_PARSING_INFO:
		{
			MS_U8 ucPacketContent[HDMI_GET_PACKET_LENGTH] = { 0 };

			if (ulBufferSize == sizeof(stCMD_HDMI_GET_GC_PARSING_INFO)) {
				stCMD_HDMI_GET_GC_PARSING_INFO *pParameters =
				    (stCMD_HDMI_GET_GC_PARSING_INFO *) pBuffer;

				if (MDrv_HDMI_GetPacketContent
				    (pParameters->enInputPortType, PKT_GC, HDMI_GET_PACKET_LENGTH,
				     ucPacketContent)) {
					_drv_HDMI_ParseGCInfoFrame(ucPacketContent,
					pParameters->pstGCParsingInfo);

					bret = TRUE;
				}
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_TIMING_INFO:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_GET_TIMING_INFO)) {
				stCMD_HDMI_GET_TIMING_INFO *pParameters =
				    (stCMD_HDMI_GET_TIMING_INFO *) pBuffer;

				pParameters->u16TimingInfo =
				    Drv_HDMI_GetTimingInfo(pParameters->enInputPortType,
					pParameters->enInfoType);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_GET_HDCP_AUTHVERSION:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_GET_HDCP_AUTHVERSION)) {
				((stCMD_HDMI_GET_HDCP_AUTHVERSION *) pBuffer)->enHDCPAuthVersion =
				    Drv_HDCP_GetAuthVersion(
				    ((stCMD_HDMI_GET_HDCP_AUTHVERSION *)pBuffer)->enInputPortType);

				bret = TRUE;
			}
		}
		break;

	case E_HDMI_INTERFACE_CMD_FORCE_POWER_DOWN_PORT:
		{
			if (ulBufferSize == sizeof(stCMD_HDMI_FORCE_POWER_DOWN_PORT)) {
				stCMD_HDMI_FORCE_POWER_DOWN_PORT *pParameters =
				    (stCMD_HDMI_FORCE_POWER_DOWN_PORT *) pBuffer;
				if (Drv_HDMI_ForcePowerDownPort
				    (pParameters->enInputPortType, pParameters->bPowerDown)) {
					bret = TRUE;
				}
			}
		}
		break;

	default:
		//pBuffer = NULL;
		//ulBufferSize = 0;
		bret = FALSE;
		break;
	};

	return bret;
}

MS_BOOL Drv_HDMI_Ctrl(MS_U32 u32Cmd, void *pBuf, MS_U32 u32BufSize)
{
	MS_BOOL bRetValue = FALSE;

	if ((pHDMIRes != NULL) && (pBuf != NULL))
		bRetValue = MDrv_HDMI_IOCTL(u32Cmd, pBuf, u32BufSize);

	return bRetValue;
}

#endif				// #ifndef MDRV_HDMI_C
