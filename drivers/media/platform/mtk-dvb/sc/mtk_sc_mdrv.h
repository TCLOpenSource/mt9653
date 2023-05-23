/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MDRV_SC_H_
#define _MDRV_SC_H_

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/version.h>

#include "mtk_sc_drv.h"		//for STI
#include "mtk_sc_hal.h"
//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define SC_FIFO_SIZE                512	// Rx fifo size

#define SC_PRINT(fmt, args...)


//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------

/// SmartCard Info
typedef struct {
	BOOL bCardIn;		///Status care in
	BOOL bLastCardIn;
	U32 u32CardStatus;
	wait_queue_head_t stWaitQue;

	U8 u8FifoRx[SC_FIFO_SIZE];
	U16 u16FifoRxRead;
	U16 u16FifoRxWrite;

	U8 u8FifoTx[SC_FIFO_SIZE];
	U16 u16FifoTxRead;
	U16 u16FifoTxWrite;
	U32 u32CardAttr;
	U32 u32CwtRxErrorIndex;
	U32 u32StartTimeRstToATR;

} SC_Info_;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
int KDrv_SC_Open(MS_U8 u8SCID);
ssize_t KDrv_SC_Read(MS_U8 u8SCID, char __user *buf, size_t count);
int KDrv_SC_AttachInterrupt(MS_U8 u8SCID);
int KDrv_SC_DetachInterrupt(MS_U8 u8SCID);
ssize_t KDrv_SC_Write(MS_U8 u8SCID, const char *buf, size_t count);
int KDrv_SC_Poll(MS_U8 u8SCID, MS_U32 u32TimeoutMs);
void KDrv_SC_ResetFIFO(MS_U8 u8SCID);
int KDrv_SC_GetEvent(MS_U8 u8SCID, MS_U32 *pu32Events);
int KDrv_SC_SetEvent(MS_U8 u8SCID, MS_U32 *pu32Events);
int KDrv_SC_GetAttribute(MS_U8 u8SCID, MS_U32 *pu32Attrs);
int KDrv_SC_SetAttribute(MS_U8 u8SCID, MS_U32 *pu32Attrs);
int KDrv_SC_CheckRstToATR(MS_U8 u8SCID, MS_U32 u32RstToAtrPeriod);
int KDrv_SC_GetCwtRxErrorIndex(MS_U8 u8SCID, MS_U32 *pu32CwtRxErrorIndex);
int KDrv_SC_ResetRstToATR(MS_U8 u8SCID);
int KDrv_SC_EnableIRQ(MS_U8 u8SCID);

BOOL MDrv_SC_ISR_Proc(U8 u8SCID);

#endif				// _MDRV_TEMP_H_
