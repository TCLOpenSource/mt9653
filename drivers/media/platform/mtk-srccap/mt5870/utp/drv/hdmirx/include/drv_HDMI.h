/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#ifndef _DRV_HDMI_H_
#define _DRV_HDMI_H_
#include "mdrv_hdmirx.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------
struct HDMI_RX_RESOURCE_PRIVATE {
	MS_BOOL bSelfCreateTaskFlag;
	MS_BOOL bHDMITaskProcFlag;
	MS_BOOL bImmeswitchSupport;
	MS_BOOL bHDCP14KeyVaildFlag;
	MS_BOOL bHPDInvertFlag[4];
	MS_BOOL bHPDEnableFlag[4];
	MS_BOOL bDataEnableFlag[4];
	MS_BOOL bHDCPIRQAttachFlag;
	MS_BOOL bHDCP22IRQEnableFlag[4];
	MS_BOOL bHDCP22EnableFlag[4];
	MS_BOOL bHDCP14R0IRQEnable[4];
	MS_BOOL bHDCP14RiFlag[4];
	MS_BOOL bHDCP14RxREEFlag;
	MS_BOOL bSTRSuspendFlag;
	MS_U8 ucInitialIndex;
	MS_U8 ucMHLSupportPath;
	MS_U8 ucHDCPKeyTable[HDMI_HDCP_KEY_LENGTH];
	MS_U8 ucCheckPacketState[HDMI_INFO_SOURCE_MAX];
	MS_U8 ucReceiveInterval[HDMI_INFO_SOURCE_MAX];
	MS_U8 ucReceiveIntervalCount[HDMI_INFO_SOURCE_MAX];
	MS_U8 ucHDMIPollingStack[HDMI_POLLING_STACK_SIZE];
	MS_U8 ucHDCP22RecvIDListSend[4];
	MS_U8 ucHDCPWriteDoneIndex;
	MS_U8 ucHDCPReadDoneIndex;
	MS_U8 ucHDCPR0ReadDoneIndex;
	MS_U8 u8PacketReceiveCount[HDMI_INFO_SOURCE_MAX];
	MS_U16 usHDCP14RiCount[4];
	MS_U32 ulPacketStatus[HDMI_INFO_SOURCE_MAX];
	MS_U32 u32PrePacketStatus[HDMI_INFO_SOURCE_MAX];
	MS_S32 slHDMIPollingTaskID;
	MS_S32 slHDMIHDCPEventID;
	MS_S32 slHDMIRxLoadKeyTaskID;
	MS_S32 slHDMIRxHDCP1xRiID;
	EN_POWER_MODE usPrePowerState;
	E_MUX_INPUTPORT enMuxInputPort[HDMI_INFO_SOURCE_MAX];
	stHDMI_POLLING_INFO stHDMIPollingInfo[4];
};

#ifdef __cplusplus
}
#endif
MS_BOOL Drv_HDMIRx_HW_Init(stHDMIRx_Bank * stHDMIRxBank);
MS_U8 Drv_HDMI_HDCP_ISR(MS_U8 *ucHDCPStatusIndex);
void Drv_HDCP22_IRQEnable(MS_BOOL ubIRQEnable);
void Drv_HDCP14_ReadR0IRQEnable(MS_BOOL ubR0IRQEnable);
MS_BOOL Drv_HDMI_Current_Port_Set(E_MUX_INPUTPORT enInputPortType);
void Drv_HDMI_init(void);
MS_U32 drv_HDMI_STREventProc(EN_POWER_MODE usPowerState);
void Drv_HDMI_PROG_DDCRAM(XC_DDCRAM_PROG_INFO *pstDDCRam_Info);
void Drv_HDMI_READ_DDCRAM(XC_DDCRAM_PROG_INFO *pstDDCRam_Info);
MS_U8 Drv_HDMI_audio_channel_status(MS_U8 u8byte);
void Drv_HDMI_Audio_MUTE_Enable(MS_S32 s32value);
MS_U16 Drv_HDMI_GetDataInfo(E_HDMI_GET_DATA_INFO enInfo);
MS_U16 Drv_HDMI_GetDataInfoByPort(E_HDMI_GET_DATA_INFO enInfo, E_MUX_INPUTPORT enInputPortType);
MS_HDMI_CONTENT_TYPE Drv_HDMI_Get_Content_Type(void);
MS_HDMI_COLOR_FORMAT Drv_HDMI_Get_ColorFormat(void);
MS_HDMI_EXT_COLORIMETRY_FORMAT Drv_HDMI_Get_ExtColorimetry(void);
MS_BOOL Drv_HDMI_Set_EQ_To_Port(MS_HDMI_EQ enEq, MS_U8 u8EQValue, E_MUX_INPUTPORT enInputPortType);
MS_BOOL Drv_HDCP_Enable(E_MUX_INPUTPORT enInputPortType, MS_BOOL bEnable);
MS_BOOL Drv_HDMI_Audio_Status_Clear(void);
MS_BOOL Drv_HDMI_pullhpd(MS_BOOL bHighLow, E_MUX_INPUTPORT enInputPortType, MS_BOOL bInverse);
MS_BOOL Drv_DVI_Software_Reset(E_MUX_INPUTPORT enInputPortType, MS_U16 u16Reset);
MS_BOOL Drv_HDMI_PacketReset(E_MUX_INPUTPORT enInputPortType, HDMI_REST_t eReset);
MS_BOOL Drv_DVI_ClkPullLow(MS_BOOL bPullLow, E_MUX_INPUTPORT enInputPortType);
MS_BOOL Drv_HDMI_ARC_PINControl(E_MUX_INPUTPORT enInputPortType, MS_BOOL bEnable,
				 MS_BOOL bDrivingHigh);
MS_U16 Drv_HDMI_GC_Info(E_MUX_INPUTPORT enInputPortType, HDMI_GControl_INFO_t enGCControlInfo);
MS_U8 Drv_HDMI_err_status_update(E_MUX_INPUTPORT enInputPortType, MS_U8 u8Value,
				  MS_BOOL bReadFlag);
MS_U8 Drv_HDMI_audio_cp_hdr_info(E_MUX_INPUTPORT enInputPortType);
MS_U8 Drv_HDMI_Get_Pixel_Repetition(E_MUX_INPUTPORT enInputPortType);
EN_AVI_INFOFRAME_VERSION Drv_HDMI_Get_AVIInfoFrameVer(E_MUX_INPUTPORT enInputPortType);
MS_BOOL Drv_HDMI_Get_AVIInfoActiveInfoPresent(E_MUX_INPUTPORT enInputPortType);
MS_BOOL Drv_HDMI_IsHDMI_Mode(E_MUX_INPUTPORT enInputPortType);
MS_BOOL Drv_HDMI_Check4K2K(E_MUX_INPUTPORT enInputPortType);
E_HDMI_ADDITIONAL_VIDEO_FORMAT Drv_HDMI_Check_Additional_Format(E_MUX_INPUTPORT enInputPortType);
E_XC_3D_INPUT_MODE Drv_HDMI_Get_3D_Structure(E_MUX_INPUTPORT enInputPortType);
E_HDMI_3D_EXT_DATA_T Drv_HDMI_Get_3D_Ext_Data(E_MUX_INPUTPORT enInputPortType);
void Drv_HDMI_Get_3D_Meta_Field(E_MUX_INPUTPORT enInputPortType, sHDMI_3D_META_FIELD *pdata);
MS_U8 Drv_HDMI_GetSourceVersion(E_MUX_INPUTPORT enInputPortType);
MS_BOOL SYMBOL_WEAK Drv_HDMI_GetDEStableStatus(E_MUX_INPUTPORT enInputPortType);
MS_BOOL Drv_HDCP22_PollingReadDone(MS_U8 ucPortIdx);
void Drv_HDCP22_PortInit(MS_U8 ucPortIdx);
void Drv_HDCP22_RecvMsg(MS_U8 ucPortIdx, MS_U8 *ucMsgData);
void Drv_HDCP22_SendMsg(MS_U8 ucPortType, MS_U8 ucPortIdx, MS_U8 *pucData, MS_U32 dwDataLen,
			 void *pDummy);
E_HDMI_HDCP_ENCRYPTION_STATE Drv_HDMI_CheckHDCPENCState(E_MUX_INPUTPORT enInputPortType);
MS_BOOL Drv_HDMI_GetCRCValue(E_MUX_INPUTPORT enInputPortType, MS_DVI_CHANNEL_TYPE u8Channel,
			      MS_U16 *pu16CRCVal);
MS_BOOL Drv_HDMI_Dither_Set(E_MUX_INPUTPORT enInputPortType, E_DITHER_TYPE enDitherType,
			     MS_BOOL ubRoundEnable);
MS_BOOL Drv_HDMI_Get_EMP(E_MUX_INPUTPORT enInputPortType, E_HDMI_EMPACKET_TYPE enEmpType,
			  MS_U8 u8CurrentPacketIndex, MS_U8 *pu8TotalPacketNumber,
			  MS_U8 *pu8EmPacket);
MS_BOOL Drv_HDMI_Ctrl(MS_U32 u32Cmd, void *pBuf, MS_U32 u32BufSize);

#endif				// _DRV_HDMI_H_
