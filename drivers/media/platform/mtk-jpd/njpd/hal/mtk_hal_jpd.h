/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */



#ifndef _HAL_NJPD_H_
#define _HAL_NJPD_H_

//-------------------------------------------------------------------------------------------------
// Driver Capability
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Macro and Define
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Type and Structure
//-------------------------------------------------------------------------------------------------
// NJPD debug level enum
typedef enum {
	E_NJPD_DEBUG_HAL_ERR = 0x0,
	E_NJPD_DEBUG_HAL_MSG = 0x02
} NJPD_HalDbgLevel;


typedef enum {
	E_NJPD_OUTPUT_ORIGINAL = 0x0,
	E_NJPD_OUTPUT_YC_SWAP = 0x01,
	E_NJPD_OUTPUT_UV_SWAP = 0x02,
	E_NJPD_OUTPUT_UV_7BIT = 0x03,
	E_NJPD_OUTPUT_UV_MSB = 0x04
} NJPD_OutputFormat;

//-------------------------------------------------------------------------------------------------
// Function and Variable
//-------------------------------------------------------------------------------------------------
#include <linux/io.h>
#include <linux/bitops.h>

void HAL_NJPD_SetRIUBank(mtk_jpd_riu_banks *riu_banks);
void HAL_NJPD_EXT_8G_WRITE_BOUND_ENABLE(__u8 bEnable);
void HAL_NJPD_EXT_Enable8G(__u8 bEnable);
__u8 HAL_NJPD_EXT_IsEnable8G(void);
void HAL_NJPD_EXT_SetMRCBuf0_Start_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf0_Start_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf0_Start_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf0_End_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf0_End_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf0_End_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf1_Start_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf1_Start_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf1_Start_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf1_End_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf1_End_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCBuf1_End_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCStart_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCStart_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetMRCStart_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetMWCBuf_Start_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetMWCBuf_Start_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetMWCBuf_Start_8G_2(__u16 u16Value);
__u16 HAL_NJPD_EXT_GetMWCBuf_Start_8G_0(void);
__u16 HAL_NJPD_EXT_GetMWCBuf_Start_8G_1(void);
__u16 HAL_NJPD_EXT_GetMWCBuf_Start_8G_2(void);
__u16 HAL_NJPD_EXT_GetMWCBuf_WritePtr_8G_0(void);
__u16 HAL_NJPD_EXT_GetMWCBuf_WritePtr_8G_1(void);
__u16 HAL_NJPD_EXT_GetMWCBuf_WritePtr_8G_2(void);
void HAL_NJPD_EXT_SetWPENUBound_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetWPENUBound_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetWPENUBound_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetWPENLBound_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetWPENLBound_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetWPENLBound_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetHTableStart_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetHTableStart_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetHTableStart_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetQTableStart_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetQTableStart_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetQTableStart_8G_2(__u16 u16Value);
void HAL_NJPD_EXT_SetGTableStart_8G_0(__u16 u16Value);
void HAL_NJPD_EXT_SetGTableStart_8G_1(__u16 u16Value);
void HAL_NJPD_EXT_SetGTableStart_8G_2(__u16 u16Value);

void HAL_NJPD_Set_GlobalSetting00(__u16 u16Value);
__u16 HAL_NJPD_Get_GlobalSetting00(void);
void HAL_NJPD_Set_GlobalSetting01(__u16 u16Value);
__u16 HAL_NJPD_Get_GlobalSetting01(void);
void HAL_NJPD_Set_GlobalSetting02(__u16 u16Value);
__u16 HAL_NJPD_Get_GlobalSetting02(void);

void HAL_NJPD_SetMRCBuf0_StartLow(__u16 u16Value);
void HAL_NJPD_SetMRCBuf0_StartHigh(__u16 u16Value);
void HAL_NJPD_SetMRCBuf0_EndLow(__u16 u16Value);
void HAL_NJPD_SetMRCBuf0_EndHigh(__u16 u16Value);
void HAL_NJPD_SetMRCBuf1_StartLow(__u16 u16Value);
void HAL_NJPD_SetMRCBuf1_StartHigh(__u16 u16Value);
void HAL_NJPD_SetMRCBuf1_EndLow(__u16 u16Value);
void HAL_NJPD_SetMRCBuf1_EndHigh(__u16 u16Value);

void HAL_NJPD_SetMRCStart_Low(__u16 u16Value);
void HAL_NJPD_SetMRCStart_High(__u16 u16Value);
__u16 HAL_NJPD_GetMWCBuf_StartLow(void);
__u16 HAL_NJPD_GetMWCBuf_StartHigh(void);
__u16 HAL_NJPD_GetMWCBuf_WritePtrLow(void);
__u16 HAL_NJPD_GetMWCBuf_WritePtrHigh(void);


void HAL_NJPD_SetMWCBuf_StartLow(__u16 u16Value);
void HAL_NJPD_SetMWCBuf_StartHigh(__u16 u16Value);
void HAL_NJPD_SetPic_H(__u16 u16Value);
void HAL_NJPD_SetPic_V(__u16 u16Value);

void HAL_NJPD_ClearEventFlag(__u16 u16Value);
void HAL_NJPD_ForceEventFlag(__u16 u16Value);
void HAL_NJPD_MaskEventFlag(__u16 u16Value);
__u16 HAL_NJPD_GetEventFlag(void);

void HAL_NJPD_SetROI_H(__u16 u16Value);
void HAL_NJPD_SetROI_V(__u16 u16Value);
void HAL_NJPD_SetROIWidth(__u16 u16Value);
void HAL_NJPD_SetROIHeight(__u16 u16Value);
void HAL_NJPD_SetClockGate(__u16 u16Value);
void HAL_NJPD_PowerOn(void);
void HAL_NJPD_PowerOff(void);
void HAL_NJPD_SetRSTIntv(__u16 u16Value);
void HAL_NJPD_SetDbgLevel(__u8 u8DbgLevel);
void HAL_NJPD_Rst(void);
void HAL_NJPD_SetMWBuffLineNum(__u16 u16Value);
__u32 HAL_NJPD_GetCurMRCAddr(void);
__u32 HAL_NJPD_GetCurMWCAddr(void);
__u16 HAL_NJPD_GetCurRow(void);
__u16 HAL_NJPD_GetCurCol(void);
void HAL_NJPD_SetWriteProtect(__u8 enable);
void HAL_NJPD_SetAutoProtect(__u8 enable);

void HAL_NJPD_SetMRBurstThd(__u16 u16Value);
void HAL_NJPD_Set_MARB06(__u16 u16Value);
__u16 HAL_NJPD_Get_MARB06(void);
void HAL_NJPD_Set_MARB07(__u16 u16Value);
__u16 HAL_NJPD_Get_MARB07(void);

void HAL_NJPD_SetWPENUBound_0_L(__u16 u16Value);
void HAL_NJPD_SetWPENUBound_0_H(__u16 u16Value);
void HAL_NJPD_SetWPENLBound_0_L(__u16 u16Value);
void HAL_NJPD_SetWPENLBound_0_H(__u16 u16Value);

void HAL_NJPD_SetSpare00(__u16 u16Value);
__u16 HAL_NJPD_GetSpare00(void);
void HAL_NJPD_SetSpare01(__u16 u16Value);
__u16 HAL_NJPD_GetSpare01(void);
void HAL_NJPD_SetSpare02(__u16 u16Value);
__u16 HAL_NJPD_GetSpare02(void);
void HAL_NJPD_SetSpare03(__u16 u16Value);
__u16 HAL_NJPD_GetSpare03(void);
void HAL_NJPD_SetSpare04(__u16 u16Value);
__u16 HAL_NJPD_GetSpare04(void);
void HAL_NJPD_SetSpare05(__u16 u16Value);
__u16 HAL_NJPD_GetSpare05(void);
void HAL_NJPD_SetSpare06(__u16 u16Value);
__u16 HAL_NJPD_GetSpare06(void);
void HAL_NJPD_SetSpare07(__u16 u16Value);
__u16 HAL_NJPD_GetSpare07(void);
void HAL_NJPD_SetWriteOneClearReg(__u16 u16Value);
void HAL_NJPD_SetWriteOneClearReg_2(__u16 u16Value);
__u16 HAL_NJPD_GetWriteOneClearReg(void);
void HAL_NJPD_SetHTableStart_Low(__u16 u16Value);
void HAL_NJPD_SetHTableStart_High(__u16 u16Value);
void HAL_NJPD_SetQTableStart_Low(__u16 u16Value);
void HAL_NJPD_SetQTableStart_High(__u16 u16Value);
void HAL_NJPD_SetGTableStart_Low(__u16 u16Value);
void HAL_NJPD_SetGTableStart_High(__u16 u16Value);
void HAL_NJPD_SetHTableSize(__u16 u16Value);
__u16 HAL_NJPD_GetHTableSize(void);
void HAL_NJPD_SetRIUInterface(__u16 u16Value);
__u16 HAL_NJPD_GetRIUInterface(void);
__u16 HAL_NJPD_TBCReadData_L(void);
__u16 HAL_NJPD_TBCReadData_H(void);
void HAL_NJPD_SetIBufReadLength(__u8 u8Min, __u8 u8Max);
void HAL_NJPD_SetCRCReadMode(void);
void HAL_NJPD_SetCRCWriteMode(void);
void HAL_NJPD_SetOutputFormat(__u8 bRst, NJPD_OutputFormat eOutputFormat);
void HAL_NJPD_SetVerificationMode(NJPD_VerificationMode VerificationMode);
NJPD_VerificationMode HAL_NJPD_GetVerificationMode(void);
void HAL_NJPD_Debug(void);
#endif				// _HAL_NJPD_H_
