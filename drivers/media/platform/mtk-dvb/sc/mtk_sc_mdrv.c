// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/delay.h>

//drver header files

#include "mtk_sc_mdrv.h"
#include "mtk_sc_reg.h"
#include "mtk_sc_hal.h"

#include "mtk_sc_common.h"
extern REG16 __iomem *nonPM_vaddr;	//for STI

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define SC_MAX_CONT_SEND_LEN        (256)

//-------------------------------------------------------------------------------------------------
//  Local Structurs
//-------------------------------------------------------------------------------------------------
typedef enum {
	E_SC_INT_TX_LEVEL = 0x00000001,
	E_SC_INT_CARD_IN = 0x00000002,	//UART_SCSR_INT_CARDIN
	E_SC_INT_CARD_OUT = 0x00000004,	//UART_SCSR_INT_CARDOUT
	E_SC_INT_CGT_TX_FAIL = 0x00000008,
	E_SC_INT_CGT_RX_FAIL = 0x00000010,
	E_SC_INT_CWT_TX_FAIL = 0x00000020,
	E_SC_INT_CWT_RX_FAIL = 0x00000040,
	E_SC_INT_BGT_FAIL = 0x00000080,
	E_SC_INT_BWT_FAIL = 0x00000100,
	E_SC_INT_PE_FAIL = 0x00000200,
	E_SC_INT_RST_TO_ATR_FAIL = 0x00000400,
	E_SC_INT_MDB_CMD_DATA = 0x00000800,
	E_SC_INT_GET_FROM_OPTEE = 0x00001000,
	E_SC_INT_INVALID = 0xFFFFFFFF
} SC_INT_BIT_MAP;

typedef enum {
	E_SC_ATTR_INVALID = 0x00000000,
	E_SC_ATTR_TX_LEVEL = 0x00000001,
	E_SC_ATTR_NOT_RCV_CWT_FAIL = 0x00000002,
	E_SC_ATTR_T0_PE_KEEP_RCV = 0x00000004,
	E_SC_ATTR_FAIL_TO_RST_LOW = 0x00000008,
	E_SC_ATTR_ENABLE_OPTEE = 0x00000010
} SC_ATTR_TYPE;

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
static SC_Info_ _scInfo[SC_DEV_NUM] = {
	{
	 .bCardIn = FALSE,
	 .bLastCardIn = FALSE,
	 .u32CardStatus = 0,
	 .u16FifoRxRead = 0,
	 .u16FifoRxWrite = 0,
	 .u16FifoTxRead = 0,
	 .u16FifoTxWrite = 0,
	 .u32CardAttr = E_SC_ATTR_INVALID,
	 .u32CwtRxErrorIndex = 0xFFFFFFFF,
	 .u32StartTimeRstToATR = 0,
	 },
#if (SC_DEV_NUM > 1)		// no more than 2
	{
	 .bCardIn = FALSE,
	 .bLastCardIn = FALSE,
	 .u32CardStatus = 0,
	 .u16FifoRxRead = 0,
	 .u16FifoRxWrite = 0,
	 .u16FifoTxRead = 0,
	 .u16FifoTxWrite = 0,
	 .u32CardAttr = E_SC_ATTR_INVALID,
	 .u32CwtRxErrorIndex = 0xFFFFFFFF,
	 .u32StartTimeRstToATR = 0,
	}
#endif
};

//-------------------------------------------------------------------------------------------------
//  Export
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------

static U32 _MDrv_SC_GetTimeUs(void)
{
	struct timeval stTime;
	U32 u32Val;
	struct timespec64 now;

	ktime_get_real_ts64(&now);
	stTime.tv_sec = now.tv_sec;
	stTime.tv_usec = now.tv_nsec / 1000;

	u32Val = (U32) (stTime.tv_sec * 1000000 + stTime.tv_usec);

	return u32Val;
}

static int _MDrv_SC_Open(U8 u8SCID)
{
	SC_PRINT("%s is invoked\n", __func__);

	init_waitqueue_head(&_scInfo[u8SCID].stWaitQue);

	return 0;
}

static ssize_t _MDrv_SC_Read(U8 u8SCID, char *buf, size_t count)
{
	ssize_t idx = 0;
	int IsUserSpace;

	if (_scInfo[u8SCID].u16FifoRxWrite == _scInfo[u8SCID].u16FifoRxRead)
		return idx;

	IsUserSpace = access_ok(buf, count);//access_ok(VERIFY_WRITE, buf, count);

	for (idx = 0; idx < count; idx++) {
		if (_scInfo[u8SCID].u16FifoRxWrite == _scInfo[u8SCID].u16FifoRxRead)
			break;

		if (IsUserSpace)
			put_user(_scInfo[u8SCID].u8FifoRx[_scInfo[u8SCID].u16FifoRxRead++],
				 (char __user *)&buf[idx]);
		else
			buf[idx] = _scInfo[u8SCID].u8FifoRx[_scInfo[u8SCID].u16FifoRxRead++];
		//pr_info("[%s][%d] buf[%d]=0x%x\n", __func__, __LINE__,idx,buf[idx]);
		if (_scInfo[u8SCID].u16FifoRxRead == SC_FIFO_SIZE)
			_scInfo[u8SCID].u16FifoRxRead = 0;

	}

	return idx;
}

static int _MDrv_SC_AttachInterrupt(U8 u8SCID)
{
	return 1;
}

static int _MDrv_SC_DetachInterrupt(U8 u8SCID)
{
	return 0;
}

static BOOL _MDrv_SC_CheckWqStatus(U8 u8SCID)
{
	BOOL bRetVal = FALSE;

	if (_scInfo[u8SCID].u32CardStatus)
		bRetVal = TRUE;

	return bRetVal;
}

static ssize_t _MDrv_SC_Write(U8 u8SCID, const char *buf, size_t count)
{
	ssize_t idx = 0;
	U32 tmp;
	U32 u32SendLen, u2Index;
	int IsUserSpace;

	if (u8SCID >= SC_DEV_NUM)
		return -EFAULT;

	IsUserSpace = access_ok(buf, count);//access_ok(VERIFY_READ, buf, count)

	//Fill up SW TX FIFO
	for (idx = 0; idx < count; idx++) {
		if (IsUserSpace)
			get_user(_scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxWrite],
				 (char __user *)&buf[idx]);
		else
			_scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxWrite] = buf[idx];
		//Fix u16FifoTxWrite out off size
		//when u16FifoTxWrite reach SC_FIFO_SIZE and u16FifoTxRead is zero
		tmp = (_scInfo[u8SCID].u16FifoTxWrite + 1) % SC_FIFO_SIZE;
		if (tmp != _scInfo[u8SCID].u16FifoTxRead) {
			// Not overflow
			_scInfo[u8SCID].u16FifoTxWrite = tmp;
		} else {
			// overflow
			pr_info("[%s][%d] TX buffer Overflow\n", __func__, __LINE__);
			break;
		}
	}

	//
	// If TX level attribute is set,
	// then transmit TX data by TX level driven
	//instead of TX buffer empty driven
	// This can avoid too large interval between 1st data byte and 2nd's
	//
	if (_scInfo[u8SCID].u32CardAttr & E_SC_ATTR_TX_LEVEL) {
		// To use tx level int in tx pkts send
		if (count > SC_MAX_CONT_SEND_LEN)
			u32SendLen = SC_MAX_CONT_SEND_LEN;
		else
			u32SendLen = count;

		for (u2Index = 0; u2Index < u32SendLen; u2Index++) {
			if (_scInfo[u8SCID].u16FifoTxRead == _scInfo[u8SCID].u16FifoTxWrite)
				break;
			//pr_info("[%s][%d] UART_TX=0x%02x\n", __FUNCTION__, __LINE__
			//,_scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxRead]);
			SC_WRITE(u8SCID, UART_TX,
				 _scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxRead++]);

			if (_scInfo[u8SCID].u16FifoTxRead == SC_FIFO_SIZE)
				_scInfo[u8SCID].u16FifoTxRead = 0;
			else if (_scInfo[u8SCID].u16FifoTxRead ==
						_scInfo[u8SCID].u16FifoTxWrite)
				break;

		}
	} else {
		if ((SC_READ(u8SCID, UART_LSR) & UART_LSR_THRE) &&
		    (_scInfo[u8SCID].u16FifoTxRead != _scInfo[u8SCID].u16FifoTxWrite)) {
			pr_info("[%s][%d] UART_TX=0x%02x\n", __func__, __LINE__,
			       _scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxRead]);
			SC_WRITE(u8SCID, UART_TX,
				 _scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxRead++]);
			pr_info("[%s][%d] u16FifoTxRead=%d\n", __func__, __LINE__,
			       _scInfo[u8SCID].u16FifoTxRead);
			if (_scInfo[u8SCID].u16FifoTxRead == SC_FIFO_SIZE) {
				_scInfo[u8SCID].u16FifoTxRead = 0;
				pr_info("[%s][%d] u16FifoTxRead=%d\n", __func__, __LINE__,
				       _scInfo[u8SCID].u16FifoTxRead);
			}

		}
	}

	return idx;
}

static void _MDrv_SC_ResetFIFO(U8 u8SCID)
{
	pr_info("[%s][%d]\n", __func__, __LINE__);
	_scInfo[u8SCID].u16FifoRxRead = 0;
	_scInfo[u8SCID].u16FifoRxWrite = 0;
	_scInfo[u8SCID].u16FifoTxRead = 0;
	_scInfo[u8SCID].u16FifoTxWrite = 0;
}

static int _MDrv_SC_CheckRstToATR(U8 u8SCID, U32 u32RstToAtrPeriod)
{
	U32 u32DiffTime;
	U32 u32StartTime = _scInfo[u8SCID].u32StartTimeRstToATR;

	u32DiffTime = _MDrv_SC_GetTimeUs();
	if (u32DiffTime < u32StartTime)
		u32DiffTime = (0xFFFFFFFFUL - u32StartTime) + u32DiffTime;
	else
		u32DiffTime = u32DiffTime - u32StartTime;


	while (u32DiffTime <= u32RstToAtrPeriod) {
		udelay(10);

		u32DiffTime = _MDrv_SC_GetTimeUs();
		if (u32DiffTime < u32StartTime)
			u32DiffTime = (0xFFFFFFFFUL - u32StartTime) + u32DiffTime;
		else
			u32DiffTime = u32DiffTime - u32StartTime;


		if (_scInfo[u8SCID].u16FifoRxRead != _scInfo[u8SCID].u16FifoRxWrite)
			break;
	}

	if (u32DiffTime > u32RstToAtrPeriod) {
		if (_scInfo[u8SCID].u32CardAttr & E_SC_ATTR_FAIL_TO_RST_LOW)
			HAL_SC_ResetPadCtrl(u8SCID, FALSE);
		_scInfo[u8SCID].u32CardStatus |= E_SC_INT_RST_TO_ATR_FAIL;
	}

	return 0;
}

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int KDrv_SC_Open(MS_U8 u8SCID)
{
	return _MDrv_SC_Open((U8) u8SCID);
}

ssize_t KDrv_SC_Read(MS_U8 u8SCID, char *buf, size_t count)
{
	return _MDrv_SC_Read((U8) u8SCID, buf, count);
}

int KDrv_SC_AttachInterrupt(MS_U8 u8SCID)
{
	return _MDrv_SC_AttachInterrupt((U8) u8SCID);
}

int KDrv_SC_DetachInterrupt(MS_U8 u8SCID)
{
	return _MDrv_SC_DetachInterrupt((U8) u8SCID);
}

ssize_t KDrv_SC_Write(MS_U8 u8SCID, const char *buf, size_t count)
{
	return _MDrv_SC_Write((U8) u8SCID, buf, count);
}

int KDrv_SC_Poll(MS_U8 u8SCID, MS_U32 u32TimeoutMs)
{
	int err = 0;
	unsigned long timeout = msecs_to_jiffies(u32TimeoutMs);

	err =
	    wait_event_interruptible_timeout(_scInfo[u8SCID].stWaitQue,
					     _MDrv_SC_CheckWqStatus((U8) u8SCID), timeout);

	if (err < 0)
		return err;

	if (err > 0)
		return 0;
	else
		return -ETIMEDOUT;
}

void KDrv_SC_ResetFIFO(MS_U8 u8SCID)
{
	_MDrv_SC_ResetFIFO((U8) u8SCID);
}

int KDrv_SC_GetEvent(MS_U8 u8SCID, MS_U32 *pu32Events)
{
	*pu32Events = (MS_U32) _scInfo[u8SCID].u32CardStatus;
	_scInfo[u8SCID].u32CardStatus = 0;

	return 0;
}

int KDrv_SC_SetEvent(MS_U8 u8SCID, MS_U32 *pu32Events)
{
	_scInfo[u8SCID].u32CardStatus = *pu32Events;

	return 0;
}

int KDrv_SC_GetAttribute(MS_U8 u8SCID, MS_U32 *pu32Attrs)
{
	*pu32Attrs = (MS_U32) _scInfo[u8SCID].u32CardAttr;

	return 0;
}


int KDrv_SC_SetAttribute(MS_U8 u8SCID, MS_U32 *pu32Attrs)
{
	_scInfo[u8SCID].u32CardAttr = *pu32Attrs;

	return 0;
}

int KDrv_SC_CheckRstToATR(MS_U8 u8SCID, MS_U32 u32RstToAtrPeriod)
{
	return _MDrv_SC_CheckRstToATR((U8) u8SCID, (U32) u32RstToAtrPeriod);
}

int KDrv_SC_GetCwtRxErrorIndex(MS_U8 u8SCID, MS_U32 *pu32CwtRxErrorIndex)
{
	*pu32CwtRxErrorIndex = _scInfo[u8SCID].u32CwtRxErrorIndex;

	return 0;
}

int KDrv_SC_ResetRstToATR(MS_U8 u8SCID)
{
	_scInfo[u8SCID].u32StartTimeRstToATR = _MDrv_SC_GetTimeUs();

	return 0;
}

int KDrv_SC_EnableIRQ(MS_U8 u8SCID)
{
	return 0;
}

BOOL MDrv_SC_ISR_Proc(U8 u8SCID)
{
	HAL_SC_TX_LEVEL_GWT_INT stTxLevelGWT_Int;
	U8 u8Reg, u8TxLevel = 0;
	U32 cnt;
	U32 idx;
	BOOL bWakeUp = FALSE;
	BOOL bTxLvChk = FALSE;
	//pr_info("[%s][%d]\n", __func__, __LINE__);
	if (u8SCID >= SC_DEV_NUM)
		return FALSE;

	//pr_info("[%s][%d]\n", __func__, __LINE__);
	// Try to get timing fail flag
	if (HAL_SC_GetIntTxLevelAndGWT(u8SCID, &stTxLevelGWT_Int)) {
		//pr_info("[%s][%d]\n", __func__, __LINE__);
		if (stTxLevelGWT_Int.bTxLevelInt || stTxLevelGWT_Int.bCGT_TxFail ||
		    stTxLevelGWT_Int.bCGT_RxFail || stTxLevelGWT_Int.bCWT_TxFail ||
		    stTxLevelGWT_Int.bCWT_RxFail || stTxLevelGWT_Int.bBGT_Fail
		    || stTxLevelGWT_Int.bBWT_Fail) {
			if (stTxLevelGWT_Int.bTxLevelInt) {
				_scInfo[u8SCID].u32CardStatus |= E_SC_INT_TX_LEVEL;
				//pr_info("[%s][%d]E_SC_INT_TX_LEVEL\n", __func__, __LINE__);//TODO
			}
			if (stTxLevelGWT_Int.bCWT_RxFail) {
				//CWT RX INT
				_scInfo[u8SCID].u32CwtRxErrorIndex =
				    (U32) _scInfo[u8SCID].u16FifoRxWrite;
				//pr_info("[%s][%d]E_SC_INT_CWT_RX_FAIL\n",
				//__func__, __LINE__);//TODO
			}
			if (stTxLevelGWT_Int.bCWT_TxFail) {
				//CWT TX INT
				_scInfo[u8SCID].u32CardStatus |= E_SC_INT_CWT_TX_FAIL;
				pr_info("[%s][%d]E_SC_INT_CWT_TX_FAIL\n", __func__,
				       __LINE__);
			}
			if (stTxLevelGWT_Int.bCGT_RxFail) {
				//CGT RX INT
				_scInfo[u8SCID].u32CardStatus |= E_SC_INT_CGT_RX_FAIL;
				pr_info("[%s][%d]E_SC_INT_CGT_RX_FAIL\n", __func__,
				       __LINE__);
			}
			if (stTxLevelGWT_Int.bCGT_TxFail) {
				//CGT TX INT
				_scInfo[u8SCID].u32CardStatus |= E_SC_INT_CGT_TX_FAIL;
				pr_info("[%s][%d]E_SC_INT_CGT_TX_FAIL\n", __func__,
				       __LINE__);
			}
			if (stTxLevelGWT_Int.bBGT_Fail) {
				//BGT INT
				_scInfo[u8SCID].u32CardStatus |= E_SC_INT_BGT_FAIL;
				pr_info("[%s][%d]E_SC_INT_BGT_FAIL\n", __func__, __LINE__);
			}
			if (stTxLevelGWT_Int.bBWT_Fail) {
				//BWT INT
				HAL_SC_RstToIoEdgeDetCtrl(u8SCID, FALSE);
				if (_scInfo[u8SCID].u32CardAttr & E_SC_ATTR_FAIL_TO_RST_LOW)
					HAL_SC_ResetPadCtrl(u8SCID, FALSE);
				_scInfo[u8SCID].u32CardStatus |= E_SC_INT_BWT_FAIL;
			}
			// Clear int flag
			HAL_SC_ClearIntTxLevelAndGWT(u8SCID);
			//pr_info("[%s][%d]\n", __func__, __LINE__);
			//
			// To prevent tx level int be cleared due to CGWT/BGWT,
			// we fill up tx FIFO for below case by enable bTxLvChk
			// 1. Tx FIFO count is less than Tx Level
			// 2. SW Tx FIFO R/W index is not the same,
			// that means there is data in SW Tx FIFO need to be sent
			//
			u8TxLevel = HAL_SC_GetTxLevel(u8SCID);
			u8Reg = HAL_SC_GetTxFifoCnt(u8SCID);
			if (u8Reg <= u8TxLevel) {
				if (_scInfo[u8SCID].u16FifoTxRead !=
				    _scInfo[u8SCID].u16FifoTxWrite) {
					bTxLvChk = TRUE;
					SC_PRINT("To enable bTxLvChk, 0x%X\n", u8Reg);

				}
			}
			//pr_info("[%s][%d]\n", __func__, __LINE__);
			bWakeUp = TRUE;

			if (stTxLevelGWT_Int.bTxLevelInt || bTxLvChk) {
				cnt = 0;
				while (1) {
					if (_scInfo[u8SCID].u16FifoTxRead ==
					    _scInfo[u8SCID].u16FifoTxWrite)
						break;

					// To avoid Tx FIFO overflow
					u8TxLevel = HAL_SC_GetTxLevel(u8SCID);
					if (HAL_SC_GetTxFifoCnt(u8SCID) >= (u8TxLevel + 3)) {
						SC_PRINT("To ignore TX this time\n");
						break;
					}
					//pr_info("[%s][%d] UART_TX=0x%02x\n",
					//__FUNCTION__, __LINE__,
					//_scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxRead]);
					SC_WRITE(u8SCID, UART_TX,
					_scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxRead++]);

					cnt++;
					if (_scInfo[u8SCID].u16FifoTxRead == SC_FIFO_SIZE) {
						_scInfo[u8SCID].u16FifoTxRead = 0;
					} else if (_scInfo[u8SCID].u16FifoTxRead ==
						   _scInfo[u8SCID].u16FifoTxWrite) {
						break;
					}

					if (cnt >= 16)
						break;

				}
			}
		}
	}

	u8Reg = SC_READ(u8SCID, UART_IIR);
	if (!(u8Reg & UART_IIR_NO_INT)) {
		u8Reg = _HAL_SC_GetLsr(u8SCID);	//for STI
		while (u8Reg & (UART_LSR_DR | UART_LSR_BI)) {
			bWakeUp = TRUE;
			_scInfo[u8SCID].u8FifoRx[_scInfo[u8SCID].u16FifoRxWrite] =
			    SC_READ(u8SCID, UART_RX);
			//pr_info("[%s][%d] UART_RX[%d]=0x%x\n", __func__, __LINE__,
			//_scInfo[u8SCID].u16FifoRxWrite,
			//_scInfo[u8SCID].u8FifoRx[_scInfo[u8SCID].u16FifoRxWrite]);
			if ((_scInfo[u8SCID].u32CardStatus & E_SC_INT_CWT_RX_FAIL) &&
			    (_scInfo[u8SCID].u32CardAttr & E_SC_ATTR_NOT_RCV_CWT_FAIL)) {
				// Do nothing for CWT fail
				pr_info("[%s][%d]\n", __func__, __LINE__);
			} else {
				//pr_info("[%s][%d]\n", __func__, __LINE__);
				if (u8Reg & UART_LSR_PE) {
					pr_info("[%s][%d]\n", __func__, __LINE__);
					_scInfo[u8SCID].u32CardStatus |= E_SC_INT_PE_FAIL;
					if (_scInfo[u8SCID].u32CardAttr &
						E_SC_ATTR_T0_PE_KEEP_RCV) {
						//for conax
						u8Reg = _HAL_SC_GetLsr(u8SCID);	//for STI
						continue;
					} else {
						break;
					}
				}
				//Fix u16FifoRxWrite out off size
				//when u16FifoRxWrite reach SC_FIFO_SIZE
				//and u16FifoRxRead is zero
				//pr_info("[%s][%d]\n", __func__, __LINE__);
				idx = (_scInfo[u8SCID].u16FifoRxWrite + 1) % SC_FIFO_SIZE;
				if (idx != _scInfo[u8SCID].u16FifoRxRead) {
					// Not overflow
					_scInfo[u8SCID].u16FifoRxWrite = idx;

				} else {
					// overflow
					pr_info("[%s][%d] RX buffer Overflow\n",
					     __func__, __LINE__);
					break;
				}
			}

			u8Reg = _HAL_SC_GetLsr(u8SCID);	//for STI
		}
		//pr_info("[%s][%d]\n", __func__, __LINE__);
		if (u8Reg & UART_LSR_THRE) {
			cnt = 16;
			do {
				if (_scInfo[u8SCID].u16FifoTxRead ==
				    _scInfo[u8SCID].u16FifoTxWrite)
					break;

				bWakeUp = TRUE;

				SC_WRITE(u8SCID, UART_TX,
				_scInfo[u8SCID].u8FifoTx[_scInfo[u8SCID].u16FifoTxRead++]);

				if (_scInfo[u8SCID].u16FifoTxRead == SC_FIFO_SIZE)
					_scInfo[u8SCID].u16FifoTxRead = 0;


			} while (--cnt > 0);
		}
	}
	//pr_info("[%s][%d]\n", __func__, __LINE__);
	// Check special event from SMART
	u8Reg = SC_READ(u8SCID, UART_SCSR);
	if (u8Reg & (UART_SCSR_INT_CARDIN | UART_SCSR_INT_CARDOUT)) {
		SC_WRITE(u8SCID, UART_SCSR, u8Reg);	// clear interrupt
		_scInfo[u8SCID].u32CardStatus |=
		    u8Reg & (UART_SCSR_INT_CARDIN | UART_SCSR_INT_CARDOUT);
		bWakeUp = TRUE;
	}
	//pr_info("[%s][%d]\n", __func__, __LINE__);
	//Check HW Rst to IO fail
	if (HAL_SC_CheckIntRstToIoEdgeFail(u8SCID)) {
		HAL_SC_MaskIntRstToIoEdgeFail(u8SCID);	//Mask int
		_scInfo[u8SCID].u32CardStatus |= E_SC_INT_RST_TO_ATR_FAIL;
		bWakeUp = TRUE;
	}

	//pr_info("[%s][%d]\n", __func__, __LINE__);

	return TRUE;		// handled
}

//-------------------------------------------------------------------------------------------------
/// Handle smart card Interrupt notification handler
/// @param  irq             \b IN: interrupt number
/// @param  devid           \b IN: device id
/// @return IRQ_HANDLED
/// @attention
//-------------------------------------------------------------------------------------------------
irqreturn_t MDrv_SC_ISR1(int irq, void *devid)
{
	if (!MDrv_SC_ISR_Proc(0))
		SC_PRINT("ISR proc is failed\n");


	return IRQ_HANDLED;
}

//-------------------------------------------------------------------------------------------------
/// Handle smart card Interrupt notification handler
/// @param  irq             \b IN: interrupt number
/// @param  devid           \b IN: device id
/// @return IRQ_HANDLED
/// @attention
//-------------------------------------------------------------------------------------------------
irqreturn_t MDrv_SC_ISR2(int irq, void *devid)
{
	if (!MDrv_SC_ISR_Proc(1))
		SC_PRINT("ISR proc is failed\n");


	return IRQ_HANDLED;
}
