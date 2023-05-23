// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>

#include "drvEARC.h"
#include "hwreg_earc_common.h"

//-------------------------------------------------------------------------------------------------
//  definition
//-------------------------------------------------------------------------------------------------
static struct HDMI_EARCTX_RESOURCE_PRIVATE *pEarcRes;
static struct task_struct *stEarcThread;

//-------------------------------------------------------------------------------------------------
//  Global variables
//-------------------------------------------------------------------------------------------------

MS_U8 _mdrv_EARC_TX_GetConnectState(void)
{
	MS_U8 u8ReValue = EARC_CONNECTION_LOST_5V;
	MS_U16 u16RegisterValue = 0;

	u16RegisterValue = HWREG_EARC_TX_GetDiscState();	// earc_tx_30[4:0]: reg_disc_state

	if (u16RegisterValue & BIT(0))
		u8ReValue = EARC_CONNECTION_LOST_5V;

	if (pEarcRes->u8EARCSupportMode == SUPPORT_NONE)
		u8ReValue = EARC_CONNECTION_NONE_MODE;

	if (u16RegisterValue & BIT(1))
		u8ReValue = EARC_CONNECTION_ARC_MODE;

	if ((u16RegisterValue & BIT(2)) || (u16RegisterValue & BIT(3)))
		u8ReValue = EARC_CONNECTION_EARC_BUSY;

	if (u16RegisterValue & BIT(4))
		u8ReValue = EARC_CONNECTION_EARC_MODE;

	return u8ReValue;
}

void _mdrv_EARC_TX_SetPacketInfo(void)
{
	HWREG_EARC_TX_SetOpCodeDeviceId(
		pEarcRes->stEarcCmPollingInfo.u8OPCodeDeviceID);
	HWREG_EARC_TX_SetOpCodeOffset(
		pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset);
	HWREG_EARC_TX_SetTransactionDataThreshold(
		pEarcRes->stEarcCmPollingInfo.u8TransactionDataThreshold);
}


void _mdrv_EARC_TX_HeartBeatReadDataTrigger(void)
{
	HWREG_EARC_TX_SetReadTrig(TRUE);
}

void _mdrv_EARC_TX_HeartBeatWriteDataTrigger(MS_BOOL bFIFOWriteTriFlag)
{
	if (bFIFOWriteTriFlag)
		HWREG_EARC_TX_SetPacketTxFifoWritePulse(TRUE);
	else
		HWREG_EARC_TX_SetWriteTrig(TRUE);
}



MS_BOOL _mdrv_EARC_TX_GetRxHeartBeatStatus(void)
{
	MS_BOOL ReValue = TRUE;

	if (pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag) {
		if (HWREG_EARC_TX_GetHeartBeatStatus() & BIT(3))
			ReValue = TRUE;
		else
			ReValue = FALSE;
	} else if (pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag) {
		if (HWREG_EARC_TX_GetHeartBeatStatus() & BIT(4))
			ReValue = TRUE;
		else
			ReValue = FALSE;
	}

	return ReValue;
}

MS_BOOL _mdrv_EARC_TX_GetReadTransactionResult(MS_BOOL bResetIRQFlag)
{
	MS_BOOL bReValue = FALSE;
	MS_U16 u16RegisterValue = 0;

	if (bResetIRQFlag) {
		HWREG_EARC_TX_SetSetIrqClear0(0xFF);
		HWREG_EARC_TX_SetSetIrqClear1(0xFF);

		bReValue = TRUE;
	} else {
		u16RegisterValue = HWREG_EARC_TX_GetIrqStatus1();

		if (u16RegisterValue & BIT(3)) {
			bReValue = TRUE;
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_TRANSACTION_DONE;
		} else if (u16RegisterValue & BIT(4)) {
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_RETRY_FORCE_STOP;
		} else if (u16RegisterValue & BIT(5)) {
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_CONT_FORCE_STOP;
		} else if (u16RegisterValue & BIT(13)) {
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_ECC_ERROR;
		} else {
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_FAIL;
		}
	}

	return bReValue;
}

MS_BOOL _mdrv_EARC_TX_GetWriteTransactionResult(MS_BOOL bResetIRQFlag)
{
	MS_BOOL bReValue = FALSE;
	MS_U16 u16RegisterValue = 0;

	if (bResetIRQFlag) {
		HWREG_EARC_TX_SetSetIrqClear0(0xFF);
		HWREG_EARC_TX_SetSetIrqClear1(0xFF);
	} else {
		u16RegisterValue = HWREG_EARC_TX_GetIrqStatus0();

		if (u16RegisterValue & BIT(11)) {
			bReValue = TRUE;
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_TRANSACTION_DONE;
		} else if (u16RegisterValue & BIT(12)) {
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_RETRY_FORCE_STOP;
		} else if (HWREG_EARC_TX_GetIrqStatus1() & BIT(2)) {
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_ECC_ERROR;
		} else {
			pEarcRes->stEarcCmPollingInfo.u8TransactionResult =
			    HDMI_EARCTX_FAIL;
		}
	}

	return bReValue;
}

void _mdrv_EARC_TX_WriteDataContent(void)
{
	MS_U8 u8Temp = 0;

	if (pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag) {
		for (u8Temp = 0;
		     u8Temp <
		     pEarcRes->stEarcCmPollingInfo.u8TransactionDataThreshold;
		     u8Temp++)
			HWREG_EARC_TX_WriteByteCommand(u8Temp,
			       pEarcRes->stEarcCmPollingInfo.u8SetLatencyValue);
	}
}

void _mdrv_EARC_TX_ReadDataContent(void)
{
	MS_U8 u8Temp = 0;

	if (pEarcRes->stEarcCmPollingInfo.bCapChangeFlag) {
		for (u8Temp = 0;
		     u8Temp <
		     pEarcRes->stEarcCmPollingInfo.u8TransactionDataThreshold;
		     u8Temp++)
			pEarcRes->stEarcCmPollingInfo.u8CapabilityContent
			[pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset + u8Temp] =
			HWREG_EARC_TX_ReadByteCommand(u8Temp);
	} else if (pEarcRes->stEarcCmPollingInfo.bStatChangeFlag) {
		pEarcRes->stEarcCmPollingInfo.u8LatencyContent =
			HWREG_EARC_TX_ReadByteCommand(0);
	}
}
void _mdrv_EARC_TX_HBSeqCheckCapChange(void)
{
	if (HWREG_EARC_TX_GetCAPChangeFlag(FALSE)) {
		HWREG_EARC_TX_GetCAPChangeFlag(TRUE);

		if (pEarcRes->stEarcCmPollingInfo.bCapChangeFlag ==
			FALSE) {
			pEarcRes->stEarcCmPollingInfo.bCapChangeFlag =
				TRUE;
			pEarcRes->stEarcCmPollingInfo.bCapChangeUpdateDoneFlag
				= FALSE;
			pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag
				= FALSE;
			DRV_EARC_MSG_INFO("** EARC Get Capability Change\r\n");
		} else {
			pEarcRes->stEarcCmPollingInfo.bCapChangeTmpFlag
				= TRUE;
				DRV_EARC_MSG_INFO
				("** EARC CommonModeProc not finish, get again\r\n");
		}
	} else if (
	pEarcRes->stEarcCmPollingInfo.bCapChangeTmpFlag == TRUE) {
		if (pEarcRes->stEarcCmPollingInfo.bCapChangeFlag ==
			FALSE) {
			pEarcRes->stEarcCmPollingInfo.bCapChangeTmpFlag
				= FALSE;
			pEarcRes->stEarcCmPollingInfo.bCapChangeFlag =
				TRUE;
			pEarcRes->stEarcCmPollingInfo.bCapChangeUpdateDoneFlag
				= FALSE;
			pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag
				= FALSE;
		}
	}

}
void _mdrv_EARC_TX_HeartBeatSeqProc(void)
{
	MS_U8 u8CommonModeState = HDMI_EARCTX_CHECK_CAP_CHANGE;
	MS_U16 u16Temp = 0;
	MS_BOOL bContinueFlag = FALSE;

	do {

		switch (u8CommonModeState) {
		case HDMI_EARCTX_CHECK_CAP_CHANGE:
		_mdrv_EARC_TX_HBSeqCheckCapChange();
		bContinueFlag = TRUE;
		break;

		case HDMI_EARCTX_CHECK_STA_CHANGE:
		if (HWREG_EARC_TX_GetSTATChangeFlag(FALSE)) {
			HWREG_EARC_TX_GetSTATChangeFlag(TRUE);

			if (pEarcRes->stEarcCmPollingInfo.bStatChangeFlag ==
			    FALSE) {
				pEarcRes->stEarcCmPollingInfo.bStatChangeFlag
					= TRUE;
				pEarcRes->stEarcCmPollingInfo.bStatChangeUpdateDoneFlag
					= FALSE;
				pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag
					= FALSE;

				DRV_EARC_MSG_INFO("** EARC Get Stat Change\r\n");
			} else {
				pEarcRes->stEarcCmPollingInfo.bStatChangeTmpFlag
					= TRUE;
				DRV_EARC_MSG_INFO
				    ("** EARC CommonModeProc not finish, get again\r\n");
			}
		} else if (pEarcRes->stEarcCmPollingInfo.bStatChangeTmpFlag ==
			   TRUE) {
			if (pEarcRes->stEarcCmPollingInfo.bStatChangeFlag ==
			    FALSE) {
				pEarcRes->stEarcCmPollingInfo.bStatChangeTmpFlag
					= FALSE;
				pEarcRes->stEarcCmPollingInfo.bStatChangeFlag
					= TRUE;
				pEarcRes->stEarcCmPollingInfo.bStatChangeUpdateDoneFlag
					= FALSE;
				pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag
					= FALSE;
			}
		}
		bContinueFlag = TRUE;
		break;

		case HDMI_EARCTX_DO_COMMON_PROC:
		if (pEarcRes->stEarcCmPollingInfo.bCapChangeFlag) {
			if ((pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag
				== FALSE)
			    && (pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag
			    == FALSE)
			    && (pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag
			    == FALSE)) {
				pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset
					= 0;
				pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt
					= 0;
				pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag
					= TRUE;
				pEarcRes->stEarcCmPollingInfo.bCapReadBeginFlag
					= TRUE;
				pEarcRes->stEarcCmPollingInfo.u8TransactionState
					= HDMI_EARCTX_SET_OP_CODE_INFO;

				for (u16Temp = 0; u16Temp < HDMI_EARCTX_CAP_LEN; u16Temp++)
					pEarcRes->stEarcCmPollingInfo.u8CapabilityContent[u16Temp]
					= 0;
			}
		} else if (pEarcRes->stEarcCmPollingInfo.bStatChangeFlag) {
			if ((pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag
				== FALSE)
			    && (pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag == FALSE)
			    && (pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag
			    == FALSE)) {
				pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset
					= HDMI_EARCTX_OFFSET_D2;
				pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt
					= 0;
				pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag
					= TRUE;
				pEarcRes->stEarcCmPollingInfo.u8TransactionState
					= HDMI_EARCTX_SET_OP_CODE_INFO;
			}
		} else if (pEarcRes->stEarcCmPollingInfo.bSetLatencyFlag) {
			if ((pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag
				== FALSE)
			    && (pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag
			    == FALSE)
			    && (pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag
			    == FALSE)) {
				pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset
					= HDMI_EARCTX_OFFSET_D3;
				pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt
					= 0;
				pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag
					= TRUE;
				pEarcRes->stEarcCmPollingInfo.u8TransactionState
					= HDMI_EARCTX_SET_OP_CODE_INFO;
			}
		}
		bContinueFlag = FALSE;
		break;

		default:
		bContinueFlag = FALSE;
		break;
		}

		u8CommonModeState++;
	} while (bContinueFlag);
}

void _mdrv_EARC_TX_CMProcSetOpCodeInfo(void)
{
	if (pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag) {
		pEarcRes->stEarcCmPollingInfo.u8OPCodeDeviceID =
			HDMI_EARCTX_READ_ONLY_CAPBILITIES_DATA_STRUCTURE;
		pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			HDMI_EARCTX_WAIT_RX_HEARTBEAT_STATUS;
		// issue: patch for keysight.
		//heartbeat would fail when access capability above 128 bytes.
		if (pEarcRes->stEarcCmPollingInfo.bCapReadBeginFlag) {
			pEarcRes->stEarcCmPollingInfo.u8TransactionDataThreshold
				= HDMI_EARCTX_READ_CAP_LEN;
		} else {
			pEarcRes->stEarcCmPollingInfo.u8TransactionDataThreshold
				= HDMI_EARCTX_READ_CAP_LEN_MAX;
		}
	} else if (pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag) {
		pEarcRes->stEarcCmPollingInfo.u8OPCodeDeviceID =
			HDMI_EARCTX_READ_WRITE_EARC_STATUS_AND_CONTROL_REGISTERS;
		pEarcRes->stEarcCmPollingInfo.u8TransactionDataThreshold
			= HDMI_EARCTX_STAT_LEN;
		pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			HDMI_EARCTX_WAIT_RX_HEARTBEAT_STATUS;
	} else if (pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag) {
		pEarcRes->stEarcCmPollingInfo.u8OPCodeDeviceID =
			HDMI_EARCTX_READ_WRITE_EARC_STATUS_AND_CONTROL_REGISTERS;
		pEarcRes->stEarcCmPollingInfo.u8TransactionDataThreshold
			= HDMI_EARCTX_STAT_LEN;
		pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			HDMI_EARCTX_WRITE_TRIGGER;
		_mdrv_EARC_TX_WriteDataContent();
	}

}
void _mdrv_EARC_TX_CommonModeProc(void)
{
	MS_BOOL bNextStateFlag = FALSE;

	do {

		switch (pEarcRes->stEarcCmPollingInfo.u8TransactionState) {
		case HDMI_EARCTX_SET_OP_CODE_INFO:
		_mdrv_EARC_TX_CMProcSetOpCodeInfo();
		_mdrv_EARC_TX_SetPacketInfo();
		bNextStateFlag = FALSE;
		break;

		case HDMI_EARCTX_WAIT_RX_HEARTBEAT_STATUS:
		if (_mdrv_EARC_TX_GetRxHeartBeatStatus()) {
			pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			    HDMI_EARCTX_WAIT_RX_HEARTBEAT_STATUS;
			DRV_EARC_MSG_INFO("** EARC wait eARCRx heartbeat status to 0\r\n");
		} else {
			pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			    HDMI_EARCTX_READ_TRIGGER;
		}

		bNextStateFlag = FALSE;
		break;

		case HDMI_EARCTX_READ_TRIGGER:
		_mdrv_EARC_TX_GetReadTransactionResult(TRUE);
		_mdrv_EARC_TX_HeartBeatReadDataTrigger();
		pEarcRes->stEarcCmPollingInfo.u8TransactionState =
		    HDMI_EARCTX_WAIT_READ_TRANSATION_DONE;
		bNextStateFlag = FALSE;
		break;

		case HDMI_EARCTX_WRITE_TRIGGER:
		_mdrv_EARC_TX_HeartBeatWriteDataTrigger(TRUE);
		_mdrv_EARC_TX_GetWriteTransactionResult(TRUE);
		_mdrv_EARC_TX_HeartBeatWriteDataTrigger(FALSE);
		pEarcRes->stEarcCmPollingInfo.u8TransactionState =
		    HDMI_EARCTX_WAIT_WRITE_TRANSATION_DONE;
		bNextStateFlag = FALSE;
		break;

		case HDMI_EARCTX_WAIT_READ_TRANSATION_DONE:
		if (_mdrv_EARC_TX_GetReadTransactionResult(FALSE)) {
			pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt =
			    0;
			pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			    HDMI_EARCTX_READ_DATA_CONTENT;
			DRV_EARC_MSG_INFO("** EARC Read Transation done, offset = %d\r\n",
					  pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset);

			bNextStateFlag = TRUE;
		} else {
			if (pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt
				< HDMI_EARCTX_RETRY_CNT) {
				pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt++;
				pEarcRes->stEarcCmPollingInfo.u8TransactionState =
				    HDMI_EARCTX_WAIT_READ_TRANSATION_DONE;

				bNextStateFlag = FALSE;
			} else {
				pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt = 0;
				pEarcRes->stEarcCmPollingInfo.u8TransactionState
					= HDMI_EARCTX_TRANSATION_FINISH;
				DRV_EARC_MSG_INFO
				    ("** EARC Read Transation done, but fail(%d), offset =  %d\r\n",
				     pEarcRes->stEarcCmPollingInfo.u8TransactionResult,
				     pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset);

				bNextStateFlag = TRUE;
			}
		}
		break;

		case HDMI_EARCTX_WAIT_WRITE_TRANSATION_DONE:
		if (_mdrv_EARC_TX_GetWriteTransactionResult(FALSE)) {
			pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt =
			    0;
			pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			    HDMI_EARCTX_TRANSATION_FINISH;
			DRV_EARC_MSG_INFO("** EARC Write Transation done, offset = %d\r\n",
					  pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset);
			bNextStateFlag = TRUE;
		} else {
			if (pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt
				< HDMI_EARCTX_RETRY_CNT) {
				pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt++;
				pEarcRes->stEarcCmPollingInfo.u8TransactionState =
				    HDMI_EARCTX_WAIT_WRITE_TRANSATION_DONE;

				bNextStateFlag = FALSE;
			} else {
				pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt = 0;
				pEarcRes->stEarcCmPollingInfo.u8TransactionState
					= HDMI_EARCTX_TRANSATION_FINISH;
				DRV_EARC_MSG_INFO
				    ("** EARC Write Transation done, but fail(%d) offset = %d\r\n",
				     pEarcRes->stEarcCmPollingInfo.u8TransactionResult,
				     pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset);

				bNextStateFlag = TRUE;
			}
		}
		break;

		case HDMI_EARCTX_READ_DATA_CONTENT:
		if (pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag) {
			if (pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset <
			    HDMI_EARCTX_CAP_LEN) {
				_mdrv_EARC_TX_ReadDataContent();

				// issue: patch for keysight.
				//heartbeat would fail when access capability above 128 bytes.
				if (pEarcRes->stEarcCmPollingInfo.bCapReadBeginFlag) {
					pEarcRes->stEarcCmPollingInfo.bCapReadBeginFlag = FALSE;

					if (pEarcRes->stEarcCmPollingInfo.u8CapabilityContent[1]
						== 0) {
						pEarcRes->stEarcCmPollingInfo.u8TransactionState =
						    HDMI_EARCTX_TRANSATION_FINISH;
					} else {
						pEarcRes->stEarcCmPollingInfo.u8TransactionState =
						    HDMI_EARCTX_SET_OP_CODE_INFO;
					}
				} else {
					// original flow
					pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset =
					    pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset
					    + HDMI_EARCTX_READ_CAP_LEN_MAX;
					pEarcRes->stEarcCmPollingInfo.u8TransactionState =
					    HDMI_EARCTX_SET_OP_CODE_INFO;
				}
			} else if (pEarcRes->stEarcCmPollingInfo.u16OPCodeOffset
			== HDMI_EARCTX_CAP_LEN) {
				pEarcRes->stEarcCmPollingInfo.u8TransactionState
					= HDMI_EARCTX_TRANSATION_FINISH;
			}
		} else if (pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag) {
			_mdrv_EARC_TX_ReadDataContent();
			pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			    HDMI_EARCTX_TRANSATION_FINISH;
		}

		bNextStateFlag = TRUE;
		break;

		case HDMI_EARCTX_TRANSATION_FINISH:
		if (pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag) {
			pEarcRes->stEarcCmPollingInfo.bCapChangeFlag = FALSE;
			pEarcRes->stEarcCmPollingInfo.bCapChangeUpdateDoneFlag = TRUE;
			pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag =
			    FALSE;
			pEarcRes->stEarcCmPollingInfo.u8CapabilityChangeCnt =
			    pEarcRes->stEarcCmPollingInfo.u8CapabilityChangeCnt
			    + 1;
		DRV_EARC_MSG_INFO
		    ("** EARC Read Capability Transation Complete\r\n");
		} else if (pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag) {
			pEarcRes->stEarcCmPollingInfo.bStatChangeFlag = FALSE;
			pEarcRes->stEarcCmPollingInfo.bStatChangeUpdateDoneFlag = TRUE;
			pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag = FALSE;
		DRV_EARC_MSG_INFO
		    ("** EARC Read Latency Transation Complete. Get latency value = %d\r\n",
		     pEarcRes->stEarcCmPollingInfo.u8LatencyContent);
		} else if (pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag) {
			pEarcRes->stEarcCmPollingInfo.bSetLatencyFlag = FALSE;
			pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag = FALSE;
		DRV_EARC_MSG_INFO
		    ("** EARC Write Latency Transation Complete. Set latency value = %d\r\n",
		     pEarcRes->stEarcCmPollingInfo.u8SetLatencyValue);
		}
		pEarcRes->stEarcCmPollingInfo.u8TransactionState =
		    HDMI_EARCTX_TRANSATION_NONE;

		bNextStateFlag = FALSE;
		break;

		default:
			bNextStateFlag = FALSE;
			break;
		}
	} while (bNextStateFlag);
}

static int _mdrv_EARC_PollingTask(void *punused __attribute__ ((unused)))
{

	if (pEarcRes != NULL) {
		while (pEarcRes->bEARCTaskProcFlag) {

			//call utopia
			MS_U8 u8RegValue = _mdrv_EARC_TX_GetConnectState();

			if (pEarcRes->u8EARCTXConnectState != u8RegValue) {
				pEarcRes->u8EARCTXConnectState = u8RegValue;

				if (pEarcRes->u8EARCTXConnectState == EARC_CONNECTION_EARC_MODE) {
					mdrv_EARC_SetArcPin(TRUE);
					pEarcRes->stEarcCmPollingInfo.bCapChangeFlag = FALSE;
					pEarcRes->stEarcCmPollingInfo.bCapChangeTmpFlag = FALSE;
					pEarcRes->stEarcCmPollingInfo.bStatChangeFlag = FALSE;
					pEarcRes->stEarcCmPollingInfo.bStatChangeTmpFlag = FALSE;
					pEarcRes->stEarcCmPollingInfo.u8CapabilityChangeCnt = 0;
				} else {
					mdrv_EARC_SetArcPin(FALSE);
				}

				DRV_EARC_MSG_INFO("** EARC Polling state = %x\n", u8RegValue);
			}

			if (pEarcRes->u8EARCTXConnectState == EARC_CONNECTION_EARC_MODE) {
				_mdrv_EARC_TX_HeartBeatSeqProc();
				_mdrv_EARC_TX_CommonModeProc();
			}
			HWREG_EARC_TX_ResetGetOnce();
			MsOS_DelayTask(EARC_POLLING_INTERVAL);

		}
	}

	return 0;

}

MS_BOOL _mdrv_EARC_CreateTask(void)
{
	MS_BOOL bCreateSuccess = TRUE;

	if (pEarcRes->slEARCPollingTaskID < 0) {
		stEarcThread = kthread_create(_mdrv_EARC_PollingTask, pEarcRes, "HDMI EARC task");

		if (IS_ERR(stEarcThread)) {
			bCreateSuccess = FALSE;
			stEarcThread = NULL;
			DRV_EARC_MSG_FATAL("Create Thread Failed\r\n");

		} else {
			wake_up_process(stEarcThread);
			pEarcRes->slEARCPollingTaskID = stEarcThread->tgid;
			pEarcRes->bEARCTaskProcFlag = TRUE;
			DRV_EARC_MSG_INFO("Create Thread OK\r\n");

		}
	}

	if (pEarcRes->slEARCPollingTaskID < 0) {
		DRV_EARC_MSG_ERROR("Create HDMI Rx Thread failed!!\n");
		bCreateSuccess = FALSE;
	}

	return bCreateSuccess;


}

MS_U8 mdrv_EARC_GetEarcPortSel(void)
{
	MS_U8 u8RetValue = 0;

	if (pEarcRes != NULL)
		u8RetValue = (pEarcRes->u8EarcPortSel);

	return u8RetValue;
}

MS_U8 mdrv_EARC_GetConnectState(void)
{
	MS_U8 u8RetValue = 0;

	if (pEarcRes != NULL)
		u8RetValue = (pEarcRes->u8EARCTXConnectState);

	return u8RetValue;
}

MS_U8 mdrv_EARC_GetCapibilityChange(void)
{
	int ret = 0;
	bool stub = 0;
	MS_U8 u8RetValue = 0;

	ret = drv_hwreg_common_get_stub(&stub);

	if (stub)
		u8RetValue = HWREG_EARC_TX_GetCapbilityChange();
	else if (pEarcRes != NULL)
		u8RetValue = (pEarcRes->stEarcCmPollingInfo.bCapChangeUpdateDoneFlag);

	return u8RetValue;
}

MS_U8 mdrv_EARC_GetLatencyChange(void)
{
	int ret = 0;
	bool stub = 0;
	MS_U8 u8RetValue = 0;

	ret = drv_hwreg_common_get_stub(&stub);

	if (stub)
		u8RetValue = HWREG_EARC_TX_GetLatencyChange();
	else if (pEarcRes != NULL)
		u8RetValue = (pEarcRes->stEarcCmPollingInfo.bStatChangeUpdateDoneFlag);

	return u8RetValue;
}

MS_U8 mdrv_EARC_GetCapibility(MS_U8 u8Index)
{
	int ret = 0;
	bool stub = 0;
	MS_U8 u8RetValue = 0;

	ret = drv_hwreg_common_get_stub(&stub);

	if (stub)
		u8RetValue = HWREG_EARC_TX_GetCapbilityValue();
	else if (pEarcRes != NULL)
		u8RetValue = (pEarcRes->stEarcCmPollingInfo.u8CapabilityContent[u8Index]);

	return u8RetValue;
}

MS_U8 mdrv_EARC_GetLatency(void)
{
	int ret = 0;
	bool stub = 0;
	MS_U8 u8RetValue = 0;

	ret = drv_hwreg_common_get_stub(&stub);

	if (stub)
		u8RetValue = HWREG_EARC_TX_GetLatencyValue();
	else if (pEarcRes != NULL)
		u8RetValue = (pEarcRes->stEarcCmPollingInfo.u8LatencyContent);

	return u8RetValue;
}

void mdrv_EARC_SetCapibilityChange(void)
{
	if (pEarcRes != NULL)
		pEarcRes->stEarcCmPollingInfo.bCapChangeUpdateDoneFlag = TRUE;
}

void mdrv_EARC_SetCapibilityChangeClear(void)
{
	if (pEarcRes != NULL)
		pEarcRes->stEarcCmPollingInfo.bCapChangeUpdateDoneFlag = FALSE;
}

void mdrv_EARC_SetLatencyChange(void)
{
	if (pEarcRes != NULL)
		pEarcRes->stEarcCmPollingInfo.bStatChangeUpdateDoneFlag = TRUE;
}

void mdrv_EARC_SetLatencyChangeClear(void)
{
	if (pEarcRes != NULL)
		pEarcRes->stEarcCmPollingInfo.bStatChangeUpdateDoneFlag = FALSE;
}

void mdrv_EARC_SetEarcPort(MS_U8 u8EarcPort, MS_BOOL bEnable5VDetectInvert,
			   MS_BOOL bEnableHPDInvert)
{
	if (pEarcRes != NULL) {
		pEarcRes->u8EarcPortSel = u8EarcPort;
		pEarcRes->bEnable5VDetectInvert = bEnable5VDetectInvert;
		pEarcRes->bEnableHPDInvert = bEnableHPDInvert;
		HWREG_EARC_OutputPortSelect(pEarcRes->u8EarcPortSel, bEnable5VDetectInvert,
					bEnableHPDInvert);
	}
}

void mdrv_EARC_SetEarcSupportMode(MS_U8 u8EarcSupportMode)
{
	if (pEarcRes != NULL) {
		pEarcRes->u8EARCSupportMode = u8EarcSupportMode;
		HWREG_EARC_SupportMode(pEarcRes->u8EARCSupportMode);
	}
}

void mdrv_EARC_SetArcPin(MS_U8 u8ArcPinEnable)
{
	if (pEarcRes != NULL) {
		pEarcRes->u8EARCArcPin = u8ArcPinEnable;
		HWREG_EARC_ArcPin(pEarcRes->u8EARCArcPin);
	}
}

MS_U8 mdrv_EARC_GetArcPin(void)
{
	return HWREG_EARC_TX_GetArcPinStatus();
}

void mdrv_EARC_SetLatencyInfo(MS_U8 u8LatencyValue)
{
	if (pEarcRes != NULL) {
		pEarcRes->stEarcCmPollingInfo.bSetLatencyFlag = TRUE;
		pEarcRes->stEarcCmPollingInfo.u8SetLatencyValue = u8LatencyValue;
	}
}

void mdrv_EARC_Set_HeartbeatStatus(MS_U8 u8HeartbeatStatus,
	MS_BOOL bOverWriteEnable, MS_BOOL bSetValue)
{
	if (pEarcRes != NULL) {
		HWREG_EARC_TX_ConfigHeartbeatStatus(pEarcRes->u8EARCTXConnectState,
			u8HeartbeatStatus, bOverWriteEnable, bSetValue);
	}
}

void mdrv_EARC_Set_DifferentialDriveStrength(MS_U8 u8Level)
{
	HWREG_EARC_TX_SetDifferentialDriveStrength(u8Level);
}

MS_U8 mdrv_EARC_Get_DifferentialDriveStrength(void)
{
	return HWREG_EARC_TX_GetDifferentialDriveStrength();
}

void mdrv_EARC_Set_DifferentialSkew(MS_U8 u8Level)
{
	HWREG_EARC_TX_SetDifferentialSkew(u8Level);
}

MS_U8 mdrv_EARC_Get_DifferentialSkew(void)
{
	return HWREG_EARC_TX_GetDifferentialSkew();
}

void mdrv_EARC_Set_CommonDriveStrength(MS_U8 u8Level)
{
	HWREG_EARC_TX_SetCommonDriveStrength(u8Level);
}

MS_U8 mdrv_EARC_Get_CommonDriveStrength(void)
{
	return HWREG_EARC_TX_GetCommonDriveStrength();
}

void mdrv_EARC_Set_DifferentialOnOff(MS_BOOL bOnOff)
{
	HWREG_EARC_TX_SetDifferentialOnOff(bOnOff);
}

MS_BOOL mdrv_EARC_Get_DifferentialOnOff(void)
{
	return HWREG_EARC_TX_GetDifferentialOnOff();
}

void mdrv_EARC_Set_CommonOnOff(MS_BOOL bOnOff)
{
	HWREG_EARC_TX_SetCommonOnOff(bOnOff);
}

MS_U8 mdrv_EARC_Get_CommonOnOff(void)
{
	return HWREG_EARC_TX_GetCommonOnOff();
}

MS_U8 mdrv_EARC_Get_DiscState(void)
{
	return HWREG_EARC_TX_GetDiscState();
}

MS_U8 mdrv_EARC_Get_SupportMode(void)
{
	if (pEarcRes == NULL) {
		DRV_EARC_MSG_ERROR("[%s] pEarcRes is NULL\n", __func__);
		return 0;
	}

	return pEarcRes->u8EARCSupportMode;
}

void mdrv_EARC_Get_Info(struct HDMI_EARCTX_RESOURCE_PRIVATE *pEarcInfo)
{
	if (pEarcRes != NULL) {
		memcpy(pEarcInfo, pEarcRes, sizeof(struct HDMI_EARCTX_RESOURCE_PRIVATE));
		MsOS_DelayTask(EARC_POLLING_INTERVAL << 1);
	}
}

MS_BOOL mdrv_EARC_Resume_Init(void)
{
	MS_BOOL bReValue = FALSE;

	if (pEarcRes != NULL) {
		HWREG_EARC_Init();
		HWREG_EARC_SupportMode(pEarcRes->u8EARCSupportMode);
		HWREG_EARC_OutputPortSelect(pEarcRes->u8EarcPortSel,
			pEarcRes->bEnable5VDetectInvert, pEarcRes->bEnableHPDInvert);

		pEarcRes->u8EARCTXConnectState = EARC_CONNECTION_LOST_5V;
		pEarcRes->stEarcCmPollingInfo.bCapChangeFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCapChangeTmpFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bStatChangeFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bStatChangeTmpFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bSetLatencyFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCapChangeUpdateDoneFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bStatChangeUpdateDoneFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCapReadBeginFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt = 0;
		pEarcRes->stEarcCmPollingInfo.u8TransactionResult = HDMI_EARCTX_FREE;
		pEarcRes->stEarcCmPollingInfo.u8TransactionState =
			HDMI_EARCTX_TRANSATION_NONE;
		pEarcRes->stEarcCmPollingInfo.u8CapabilityChangeCnt = 0;

		DRV_EARC_MSG_INFO("EARC Resume Initial Setting Done !!!\n");

		bReValue = TRUE;
	} else {
		pr_err("EARC resume Initial Setting Fail\n");
	}

	return bReValue;
}

MS_BOOL mdrv_EARC_Init(MS_U8 u8DefaultSupportMode)
{
	MS_BOOL bReValue = FALSE;

	pEarcRes = kmalloc(sizeof(struct HDMI_EARCTX_RESOURCE_PRIVATE), GFP_KERNEL);

	if (pEarcRes != NULL) {
		pEarcRes->bSelfCreateTaskFlag = FALSE;
		pEarcRes->bEARCTaskProcFlag = FALSE;
		pEarcRes->slEARCPollingTaskID = -1;
		pEarcRes->u8EARCTXConnectState = EARC_CONNECTION_LOST_5V;
		pEarcRes->u8EARCSupportMode = u8DefaultSupportMode;
		pEarcRes->stEarcCmPollingInfo.bCapChangeFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCapChangeTmpFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bStatChangeFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bStatChangeTmpFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bSetLatencyFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCapChangeUpdateDoneFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bStatChangeUpdateDoneFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCommonModeCapBusyFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCommonModeStatBusyFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCommonModeSetLatencyBusyFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.bCapReadBeginFlag = FALSE;
		pEarcRes->stEarcCmPollingInfo.u8TransactioinRetryCnt = 0;
		pEarcRes->stEarcCmPollingInfo.u8TransactionResult = HDMI_EARCTX_FREE;
		pEarcRes->stEarcCmPollingInfo.u8TransactionState =
		    HDMI_EARCTX_TRANSATION_NONE;
		pEarcRes->stEarcCmPollingInfo.u8CapabilityChangeCnt = 0;

		HWREG_EARC_Init();
		HWREG_EARC_SupportMode(u8DefaultSupportMode);

		if (!pEarcRes->bSelfCreateTaskFlag) {
			if (_mdrv_EARC_CreateTask())
				pEarcRes->bSelfCreateTaskFlag = TRUE;
		}
	}

	DRV_EARC_MSG_INFO("EARC Initial Setting Done\n");

	bReValue = TRUE;

	return bReValue;
}

void mdrv_EARC_HWInit(MS_U32 Para1, MS_U32 Para2)
{
	HWREG_EARC_SetHWStatus(Para1, Para2);
}

