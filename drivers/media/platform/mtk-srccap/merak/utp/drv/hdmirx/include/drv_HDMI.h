/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#ifndef _DRV_HDMI_H_
#define _DRV_HDMI_H_
#include "hwreg_srccap_hdmirx.h"
#include "hwreg_srccap_hdmirx_packetreceiver.h"
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
	MS_U8 ucCheckPacketState[HDMI_PORT_MAX_NUM];
	MS_U8 ucReceiveInterval[HDMI_PORT_MAX_NUM];
	MS_U8 ucReceiveIntervalCount[HDMI_PORT_MAX_NUM];
	MS_U8 ucHDMIPollingStack[HDMI_POLLING_STACK_SIZE];
	MS_U8 ucHDCP22RecvIDListSend[4];
	MS_U8 ucHDCPWriteDoneIndex;
	MS_U8 ucHDCPReadDoneIndex;
	MS_U8 ucHDCPR0ReadDoneIndex;
	MS_U8 u8PacketReceiveCount[HDMI_PORT_MAX_NUM];
	MS_U16 usHDCP14RiCount[4];
	MS_U32 ulPacketStatus[HDMI_PORT_MAX_NUM];
	MS_U32 u32PrePacketStatus[HDMI_PORT_MAX_NUM];
	MS_S32 slHDMIPollingTaskID;
	MS_S32 slHDMIHDCPEventID;
	MS_S32 slHDMIRxLoadKeyTaskID;
	MS_S32 slHDMIRxHDCP1xRiID;
	EN_POWER_MODE usPrePowerState;
	E_MUX_INPUTPORT enMuxInputPort[HDMI_PORT_MAX_NUM];
	stHDMI_POLLING_INFO stHDMIPollingInfo[4];
};

#ifdef __cplusplus
}
#endif
MS_U8 MDrv_HDMI_HDCP_ISR(MS_U8 *ucHDCPWriteDoneIndex, MS_U8 *ucHDCPReadDoneIndex,
				 MS_U8 *ucHDCPR0ReadDoneIndex);
void MDrv_HDCP22_IRQEnable(MS_BOOL ubIRQEnable);
void MDrv_HDCP14_ReadR0IRQEnable(MS_BOOL ubR0IRQEnable);
void MDrv_HDMI_init(int hw_version, stHDMIRx_Bank *pstHDMIRxBank);
MS_U32 MDrv_HDMI_STREventProc(EN_POWER_MODE usPowerState, stHDMIRx_Bank *pstHDMIRxBank);
void MDrv_HDMI_PROG_DDCRAM(XC_DDCRAM_PROG_INFO *pstDDCRam_Info);
void MDrv_HDMI_READ_DDCRAM(XC_DDCRAM_PROG_INFO *pstDDCRam_Info);
void MDrv_HDMI_Audio_MUTE_Enable(MS_S32 s32value);
MS_U32 MDrv_HDMI_GetDataInfoByPort(E_HDMI_GET_DATA_INFO enInfo, E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_Set_EQ_To_Port(MS_HDMI_EQ enEq, MS_U8 u8EQValue, E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDCP_Enable(E_MUX_INPUTPORT enInputPortType, MS_BOOL bEnable);
MS_BOOL MDrv_HDMI_pullhpd(MS_BOOL bHighLow, E_MUX_INPUTPORT enInputPortType, MS_BOOL bInverse);
MS_BOOL MDrv_DVI_Software_Reset(E_MUX_INPUTPORT enInputPortType, MS_U16 u16Reset);
MS_BOOL MDrv_HDMI_PacketReset(E_MUX_INPUTPORT enInputPortType, HDMI_REST_t eReset);
MS_BOOL MDrv_DVI_ClkPullLow(MS_BOOL bPullLow, E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_ARC_PINControl(E_MUX_INPUTPORT enInputPortType, MS_BOOL bEnable,
				 MS_BOOL bDrivingHigh);
MS_U16 MDrv_HDMI_GC_Info(E_MUX_INPUTPORT enInputPortType, HDMI_GControl_INFO_t enGCControlInfo);
MS_U8 MDrv_HDMI_err_status_update(E_MUX_INPUTPORT enInputPortType, MS_U8 u8Value,
				  MS_BOOL bReadFlag);
MS_U8 MDrv_HDMI_audio_cp_hdr_info(E_MUX_INPUTPORT enInputPortType);
MS_U8 MDrv_HDMI_Get_Pixel_Repetition(E_MUX_INPUTPORT enInputPortType);
EN_AVI_INFOFRAME_VERSION MDrv_HDMI_Get_AVIInfoFrameVer(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_Get_AVIInfoActiveInfoPresent(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_IsHDMI_Mode(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_Check4K2K(E_MUX_INPUTPORT enInputPortType);
E_HDMI_ADDITIONAL_VIDEO_FORMAT MDrv_HDMI_Check_Additional_Format(E_MUX_INPUTPORT enInputPortType);
E_XC_3D_INPUT_MODE MDrv_HDMI_Get_3D_Structure(E_MUX_INPUTPORT enInputPortType);
E_HDMI_3D_EXT_DATA_T MDrv_HDMI_Get_3D_Ext_Data(E_MUX_INPUTPORT enInputPortType);
void MDrv_HDMI_Get_3D_Meta_Field(E_MUX_INPUTPORT enInputPortType, sHDMI_3D_META_FIELD *pdata);
MS_U8 MDrv_HDMI_GetSourceVersion(E_MUX_INPUTPORT enInputPortType);
MS_BOOL SYMBOL_WEAK MDrv_HDMI_GetDEStableStatus(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDCP22_PollingReadDone(MS_U8 ucPortIdx);
void MDrv_HDCP22_PortInit(MS_U8 ucPortIdx);
void MDrv_HDCP22_RecvMsg(MS_U8 ucPortIdx, MS_U8 *ucMsgData);
void MDrv_HDCP22_SendMsg(MS_U8 ucPortType, MS_U8 ucPortIdx, MS_U8 *pucData, MS_U32 dwDataLen,
			 void *pDummy);
E_HDMI_HDCP_ENCRYPTION_STATE MDrv_HDMI_CheckHDCPENCState(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_GetCRCValue(E_MUX_INPUTPORT enInputPortType, MS_DVI_CHANNEL_TYPE u8Channel,
			      MS_U16 *pu16CRCVal);
MS_BOOL MDrv_HDMI_Dither_Set(E_MUX_INPUTPORT enInputPortType, E_DITHER_TYPE enDitherType,
			     MS_BOOL ubRoundEnable);
MS_BOOL MDrv_HDMI_Get_EMP(E_MUX_INPUTPORT enInputPortType, E_HDMI_EMPACKET_TYPE enEmpType,
			  MS_U8 u8CurrentPacketIndex, MS_U8 *pu8TotalPacketNumber,
			  MS_U8 *pu8EmPacket);
MS_BOOL MDrv_HDMI_Ctrl(MS_U32 u32Cmd, void *pBuf, MS_U32 u32BufSize);

void MDrv_HDMI_ISR_PHY(void);
void MDrv_HDMI_ISR_PKT_QUE(void);
void MDrv_HDMI_ISR_SCDC(void);
void MDrv_HDMI_ISR_SQH_ALL_WK(void);
void MDrv_HDMI_ISR_CLK_DET_0(void);
void MDrv_HDMI_ISR_CLK_DET_1(void);
void MDrv_HDMI_ISR_CLK_DET_2(void);
void MDrv_HDMI_ISR_CLK_DET_3(void);
unsigned int MDrv_HDMI_IRQ_MASK_SAVE(EN_KDRV_HDMIRX_INT e_int);
void MDrv_HDMI_IRQ_MASK_RESTORE(EN_KDRV_HDMIRX_INT e_int, unsigned int val);

MS_U32 MDrv_HDMI_Get_PacketStatus(E_MUX_INPUTPORT enInputPortType);
stHDMI_POLLING_INFO MDrv_HDMI_Get_HDMIPollingInfo(E_MUX_INPUTPORT enInputPortType);
MS_BOOL MDrv_HDMI_deinit(int hw_version, const stHDMIRx_Bank *pstHDMIRxBank);
#endif				// _DRV_HDMI_H_
