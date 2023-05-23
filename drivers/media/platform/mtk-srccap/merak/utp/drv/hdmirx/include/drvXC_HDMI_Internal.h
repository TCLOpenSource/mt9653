/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   drvXC_HDMI_Internal.h
/// @brief  Driver Interface
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _DRVXC_HDMI_INTERNAL_H_
#define _DRVXC_HDMI_INTERNAL_H_

//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------

#include "drvXC_HDMI_if.h"

#ifdef _DRV_HDMI_C_
#define INTERFACE
#else
#define INTERFACE extern
#endif

#define SYMBOL_WEAK __attribute__((weak))

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define GET_HDMI_SYSTEM_FLAG(a, b)          (MS_BOOL)((a & b) ? TRUE : FALSE)
#define SET_HDMI_SYSTEM_FLAG(a, b)          (a |= b)
#define CLR_HDMI_SYSTEM_FLAG(a, b)          (a &= ~b)

#define HDMI_INITIAL_DONE_INDEX             0xA5U

#define HDMI_GCP_PACKET_LENGTH              2U
#define HDMI_ACP_PACKET_LENGTH              16U
#define HDMI_ISRC1_PACKET_LENGTH            16U
#define HDMI_ISRC2_PACKET_LENGTH            (32U)
#define HDMI_GM_PACKET_LENGTH               21U
#define HDMI_VS_PACKET_LENGTH               (31U)
#define HDMI_AVI_PACKET_LENGTH              15U //PB1 ~ PB14 + AVI version = 15 byte
#define HDMI_SPD_PACKET_LENGTH              25U
#define HDMI_AUDIO_PACKET_LENGTH            5U
#define HDMI_MPEG_PACKET_LENGTH             5U
#define HDMI_VBI_PACKET_LENGTH              29U
#define HDMI_HDR_PACKET_LENGTH              29U
#define HDMI_ACR_PACKET_LENGTH              7U
#define HDMI_AUDIO_CHANNEL_STATUS_LENGTH    5U
#define HDMI_MULTI_VS_PACKET_LENGTH         (120U)

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
enum HDMI_HDCP_IRQ_VERSION {
	E_HDCP1X_R0READ_DONE = 1,
	E_HDCP2X_WRITE_DONE,
	E_HDCP2X_READ_DONE,
};

enum HDMI_SYSTEM_STATUS_FLAG_TYPE {
	HDMI_SYSTEM_STATUS_REE_LOAD_14_RX_KEY_FLAG = BIT(0),
};

enum HDMI_GC_INFO_CD_TYPE {
	HDMI_GC_INFO_CD_NOT_INDICATED = 0,	// 0
	HDMI_GC_INFO_CD_RESERVED,	// 1/2/3
	HDMI_GC_INFO_CD_24BITS = 4,	// 4
	HDMI_GC_INFO_CD_30BITS,	// 5
	HDMI_GC_INFO_CD_36BITS,	// 6
	HDMI_GC_INFO_CD_48BITS,	// 7
};

enum EN_HDMI_HDR_METADATA_DESCRIPTOR_ID_TYPE {
	HDMI_HDR_STATIC_METADATA_TYPE_0 = 0,
	HDMI_HDR_STATIC_RESERVED,
};

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
//HDMI
void _mdrv_HDMI_CheckHDCP14RiProc(void);
void _mdrv_HDMI_CheckPacketReceiveProc(void);
MS_BOOL _mdrv_HDMI_CreateTask(void);
void _mdrv_HDMI_ParseAVIInfoFrame(void *pPacket, stAVI_PARSING_INFO *pstAVIParsingInfo);
void _mdrv_HDMI_ParseVSInfoFrame(void *pPacket, stVS_PARSING_INFO *pstVSParsingInfo);
void _mdrv_HDMI_ParseHDRInfoFrame(void *pPacket, sHDR_METADATA *pHdrMetadata, MS_BOOL bByteFormat);
void _mdrv_HDMI_ParseGCInfoFrame(void *pPacket, stGC_PARSING_INFO *pstGCParsingInfo);
MS_BOOL MDrv_HDMI_IOCTL(MS_U32 ulCommand, void *pBuffer, MS_U32 ulBufferSize);
E_HDMI_HDCP_STATE MDrv_HDMI_CheckHDCPState(E_MUX_INPUTPORT enInputPortType);
void MDrv_HDCP_WriteX74(E_MUX_INPUTPORT enInputPortType, MS_U8 ucOffset, MS_U8 ucData);
MS_U8 MDrv_HDCP_ReadX74(E_MUX_INPUTPORT enInputPortType, MS_U8 ucOffset);
void MDrv_HDCP_SetRepeater(E_MUX_INPUTPORT enInputPortType, MS_BOOL bIsRepeater);
void MDrv_HDCP_SetBstatus(E_MUX_INPUTPORT enInputPortType, MS_U16 usBstatus);
void MDrv_HDCP_SetHDMIMode(E_MUX_INPUTPORT enInputPortType, MS_BOOL bHDMIMode);
MS_U8 MDrv_HDCP_GetInterruptStatus(E_MUX_INPUTPORT enInputPortType);
void MDrv_HDCP_WriteKSVList(E_MUX_INPUTPORT enInputPortType, MS_U8 *pucKSV, MS_U32 ulDataLen);
void MDrv_HDCP_SetVPrime(E_MUX_INPUTPORT enInputPortType, MS_U8 *pucVPrime);
void MDrv_HDMI_DataRtermControl(E_MUX_INPUTPORT enInputPortType, MS_BOOL bDataRtermEnable);
MS_HDCP_STATUS_INFO_t MDrv_HDMI_GetHDCPStatusPort(E_MUX_INPUTPORT enInputPortType);
MS_U8 MDrv_HDMI_GetSCDCValue(E_MUX_INPUTPORT enInputPortType, MS_U8 ucOffset);
MS_U32 MDrv_HDMI_GetTMDSRatesKHz(E_MUX_INPUTPORT enInputPortType, E_HDMI_GET_TMDS_RATES enType);
MS_BOOL MDrv_HDMI_GetSCDCCableDetectFlag(E_MUX_INPUTPORT enInputPortType);
void MDrv_HDMI_AssignSourceSelect(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_GetPacketStatus(E_MUX_INPUTPORT enInputPortType,
				  MS_HDMI_EXTEND_PACKET_RECEIVE_t *pExtendPacketReceive);
MS_U32 MDrv_HDMI_GetTMDSRatesKHz(E_MUX_INPUTPORT enInputPortType, E_HDMI_GET_TMDS_RATES enType);
MS_BOOL MDrv_HDMI_GetSCDCCableDetectFlag(E_MUX_INPUTPORT enInputPortType);
void MDrv_HDMI_AssignSourceSelect(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_GetPacketStatus(E_MUX_INPUTPORT enInputPortType,
				  MS_HDMI_EXTEND_PACKET_RECEIVE_t *pExtendPacketReceive);
MS_BOOL MDrv_HDMI_GetPacketContent(E_MUX_INPUTPORT enInputPortType,
				   MS_HDMI_PACKET_STATE_t enPacketType, MS_U8 u8PacketLength,
				   MS_U8 *pu8PacketContent);
MS_U16 MDrv_HDMI_GetTimingInfo(E_MUX_INPUTPORT enInputPortType, E_HDMI_GET_DATA_INFO enInfoType);
E_HDMI_HDCP_STATE MDrv_HDCP_GetAuthVersion(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_ForcePowerDownPort(E_MUX_INPUTPORT enInputPortType, MS_BOOL bPowerDown);
MS_BOOL MDrv_HDMI_GetCRCValue(E_MUX_INPUTPORT enInputPortType, MS_DVI_CHANNEL_TYPE u8Channel,
			      MS_U16 *pu16CRCVal);

INTERFACE MS_U8 MDrv_HDMI_avi_infoframe_info(MS_U8 u8byte);
INTERFACE MS_U8 MDrv_HDMI_AVIInfoframePort(E_MUX_INPUTPORT enInputPortType, MS_U8 u8byte);
INTERFACE MS_U16 MDrv_HDMI_pll_ctrl1(HDMI_PLL_CTRL_t pllctrl, MS_BOOL bread, MS_U16 u16value);
INTERFACE MS_U16 MDrv_HDMI_GetHDE(void);
INTERFACE void MDrv_DVI_Accumulator_Monitor(void);

//HDCP

//DVI
INTERFACE MS_BOOL MDrv_HDMI_dvi_clock_detect(E_MUX_INPUTPORT enInputPortType);

INTERFACE void MDrv_HDMI_hpd_en(MS_BOOL benable);
INTERFACE MS_BOOL MDrv_HDCP_isexist(void);

INTERFACE MS_U8 MDrv_HDMI_audio_cp_data_info(MS_U8 u8byte);
INTERFACE MS_U16 MDrv_HDMI_pll_ctrl2(HDMI_PLL_CTRL2_t pllctrl, MS_BOOL bread, MS_U16 u16value);
INTERFACE void MDrv_DVI_IMMESWITCH_PS_SW_Path(void *pInstance);
INTERFACE void MDrv_HDMI_PROG_DDCRAM_internal(void *pInstance, XC_DDCRAM_PROG_INFO *pstDDCRam_Info,
					      MS_U32 u32SizeOfInfo);
INTERFACE stHDMI_INITIAL_TABLE MDrv_HDMI_init_internal(void *pInstance);
INTERFACE void MDrv_HDCP_initproductionkey_internal(void *pInstance, MS_U8 *pu8HdcpKeyData);
INTERFACE void MDrv_HDMI_pullhpd_internal(void *pInstance, MS_BOOL bHighLow,
					  E_MUX_INPUTPORT enInputPortType, MS_BOOL bInverse);
INTERFACE void MDrv_DVI_HF_adjust_internal(void);
INTERFACE void MDrv_DVI_Accumulator_Monitor(void);
INTERFACE void MDrv_HDMI_CreatePathInternal(INPUT_SOURCE_TYPE_t enInputPortType);
// implement in xc 86
MS_BOOL SYMBOL_WEAK KDrv_XC_GetEmPacket(MS_U8 enHDMIPort,
			    EN_KDRV_HDMI_EMP_TYPE enEmpType, MS_U8 u8CurrentPacketIndex,
			    MS_U8 *pu8TotalPacketNumber, MS_U8 *pu8EmPacket);

#undef INTERFACE
#endif				// _DRV_ADC_INTERNAL_H_
