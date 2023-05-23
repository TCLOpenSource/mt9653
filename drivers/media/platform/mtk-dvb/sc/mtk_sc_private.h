/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_SC_PRIV_H_
#define _MTK_SC_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mtk_sc_drv.h"

	SC_Result _MDrv_SC_Init(MS_U8 u8SCID);
	SC_Result _MDrv_SC_Exit(MS_U8 u8SCID);
	SC_Result _MDrv_SC_Open(MS_U8 u8SCID, MS_U8 u8Protocol, SC_Param *pParam,
				P_SC_Callback pfSmartNotify);
	SC_Result _MDrv_SC_Activate(MS_U8 u8SCID);
	SC_Result _MDrv_SC_Deactivate(MS_U8 u8SCID);
	SC_Result _MDrv_SC_Close(MS_U8 u8SCID);
	SC_Result _MDrv_SC_Reset(MS_U8 u8SCID, SC_Param *pParam);
	SC_Result _MDrv_SC_ClearState(MS_U8 u8SCID);
	SC_Result _MDrv_SC_GetATR(MS_U8 u8SCID, MS_U32 u32TimeOut, MS_U8 *pu8Atr,
				  MS_U16 *pu16AtrLen, MS_U8 *pu8His, MS_U16 *pu16HisLen);
	SC_Result _MDrv_SC_Config(MS_U8 u8SCID, SC_Param *pParam);
	SC_Result _MDrv_SC_Send(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 u16SendDataLen,
				MS_U32 u32TimeoutMs);
	SC_Result _MDrv_SC_Recv(MS_U8 u8SCID, MS_U8 *pu8ReadData, MS_U16 *pu16ReadDataLen,
				MS_U32 u32TimeoutMs);
	SC_Result _MDrv_SC_GetCaps(SC_Caps *pCaps);
	SC_Result _MDrv_SC_SetPPS(MS_U8 u8SCID, MS_U8 u8SCProtocol, MS_U8 u8Di, MS_U8 u8Fi);
	SC_Result _MDrv_SC_PPS(MS_U8 u8SCID);
	SC_Result _MDrv_SC_GetStatus(MS_U8 u8SCID, SC_Status *pStatus);
	void _MDrv_SC_SetDbgLevel(SC_DbgLv eLevel);
	SC_Result _MDrv_SC_RawExchange(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 *u16SendDataLen,
				       MS_U8 *pu8ReadData, MS_U16 *u16ReadDataLen);
	MS_BOOL _MDrv_SC_CardVoltage_Config(MS_U8 u8SCID, SC_VoltageCtrl eVoltage);
	SC_Result _MDrv_SC_Reset_ATR(MS_U8 u8SCID, SC_Param *pParam, MS_U8 *pu8Atr,
				     MS_U16 *pu16AtrLen, MS_U8 *pu8His, MS_U16 *pu16HisLen);
	SC_Result _MDrv_SC_T0_SendRecv(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 u16SendLen,
				       MS_U8 *pu8RecvData, MS_U16 *pu16RecvLen);
	SC_Result _MDrv_SC_T1_SendRecv(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 *u16SendDataLen,
				       MS_U8 *pu8ReadData, MS_U16 *u16ReadDataLen);
	SC_Result _MDrv_SC_T14_SendRecv(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 u16SendLen,
					MS_U8 *pu8RecvData, MS_U16 *pu16RecvLen);
	SC_Result _MDrv_SC_GetInfo(MS_U8 u8SCID, SC_Info *pstInfo);
	SC_Result _MDrv_SC_PowerOff(void);
	SC_Result _MDrv_SC_SetGuardTime(MS_U8 u8SCID, MS_U8 u8GuardTime);
	MS_U32 _MDrv_SC_SetPowerState(EN_POWER_MODE u16PowerState);

#ifdef __cplusplus
}
#endif
#endif				// _MTK_SC_PRIV_H_
