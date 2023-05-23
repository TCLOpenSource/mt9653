/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#ifndef _HAL_SC_H_
#define _HAL_SC_H_

#include "mtk_sc_common.h"
#include <linux/io.h>
// GPIO_FUNC_MUX (bank: 0x3229) offset: 0x0A     //0x000AUL
#define REG_SM_CONFIG     ((0x22900 >> 1) + 0x0A)
#define REG_SM_PAD_PCM      0x0001UL
#define REG_SM_PAD_TS1      0x0002UL
// GPIO_FUNC_MUX (bank: 0x3229) offset: 0x68     //0x0068UL
#define REG_GPO_5V_3V3_OUT ((0x22900 >> 1) + 0x68)
#define REG_GPO_5V          0x0001UL
//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------
#define SC_DEV_NUM_MAX              2UL
#define SC_DEV_NUM                  1UL	// number of device
#define SC_IRQ                      E_INT_IRQ_SMART	//E_IRQ_UART1
#define SC_IRQ2			//E_INT_IRQ_SMART1//E_IRQ_UART2

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
//#include "mtk_sc_common.h"
//REG16 __iomem *nonPM_vaddr;//for sTI test

#define SC_READ(id, addr) ((!id)?UART1_READ(addr):UART2_READ(addr))

#define SC_WRITE(id, addr, val) \
do { if ((!id)) {UART1_WRITE(addr, val); } else {UART2_WRITE(addr, val); } } while (0)

#define SC_OR(id, addr, val) \
do { if ((!id)) {UART1_OR(addr, val); }  else {UART2_OR(addr, val); } } while (0)

#define SC_AND(id, addr, val) \
do { if ((!id)) {UART1_AND(addr, val); } else {UART2_AND(addr, val); } } while (0)

#define SC_XOR(id, addr, val) \
do { if ((!id)) {UART1_XOR(addr, val); } else {UART2_XOR(addr, val); } } while (0)

////////////////////////////
// The RST_TO_ATR_SW_TIMEOUT is an experimental value for SW patch
////////////////////////////
#define RST_TO_ATR_SW_TIMEOUT  44100UL	//It is experimental value from conax

////////////////////////////
// The RST_TO_ATR_HW_TIMEOUT is xxxxx
////////////////////////////
//It is experimental value from conax PT for 39990 clokcs ATR test case
#define RST_TO_ATR_HW_TIMEOUT  41000UL

////////////////////////////
// SW extCWT value
////////////////////////////
//It is experimental value from conax PT --> WT + 150*Fi/f
#define GET_SW_EXTWT_IN_MS(WtInMs, EtuInNs, Di)   (WtInMs + ((150 * EtuInNs * Di)/1000000))


//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
typedef enum {
	E_HAL_SC_CGWT_INT_DISABLE,	//Enable CGT and CWT int
	E_HAL_SC_CGWT_INT_ENABLE,	//Disable CGT and CWT int
	E_HAL_SC_CWT_ENABLE_CGT_DISABLE,	//Enable CWT int and disable CGT
} HAL_SC_CGWT_INT;

typedef enum {
	E_HAL_SC_BGWT_INT_DISABLE,	//Enable BGT and BWT int
	E_HAL_SC_BGWT_INT_ENABLE,	//Disable BGT and BWT int
	E_HAL_SC_BWT_ENABLE_BGT_DISABLE,	//Enable BWT int and disable BGT
} HAL_SC_BGWT_INT;

typedef enum {
	E_HAL_SC_VCC_EXT_8024,	///< by external 8024 on
	E_HAL_SC_VCC_LOW,	///< by direct Vcc (low active)
	E_HAL_SC_VCC_HIGH,	///< by direct Vcc (high active)
	E_HAL_SC_VCC_OCP_HIGH,
	E_HAL_SC_VCC_INT_8024	///< by internal 8024 on
} HAL_SC_VCC_CTRL;

typedef enum {
	E_HAL_SC_VOL_CTRL_3P3V,	///<3.3V
	E_HAL_SC_VOL_CTRL_5V,	///< 5V
	E_HAL_SC_VOL_CTRL_1P8V,	///<1.8V
	E_HAL_SC_VOL_CTRL_3V,	///<3V
	E_HAL_SC_VOL_CTRL_OFF,	///<OFF
	E_HAL_SC_VOL_CTRL_INVALID
} HAL_SC_VOLTAGE_CTRL;

/// SmartCard CLK setting
typedef enum {
	E_HAL_SC_CLK_3M,	///< 3 MHz
	E_HAL_SC_CLK_4P5M,	///< 4.5 MHz
	E_HAL_SC_CLK_6M,	///< 6 MHz
	E_HAL_SC_CLK_13M,	///< 6 MHz
	E_HAL_SC_CLK_4M,	///< 4 MHz
	E_HAL_SC_CLK_TSIO,	///<
	E_HAL_SC_CLK_INVALID
} HAL_SC_CLK_CTRL;

typedef enum {
	E_HAL_SC_CONV_DIRECT,	///< Direct convention
	E_HAL_SC_CONV_INVERSE,	///< Inverse convention
	E_HAL_SC_CONV_INVALID
} HAL_SC_CONV_CTRL;

typedef enum {
	E_HAL_SC_UART_INT_DISABLE = 0x00,	//Clear all int
	E_HAL_SC_UART_INT_RDI = 0x01,
	E_HAL_SC_UART_INT_THRI = 0x02,	//Transmitter Holding Register Empty Interrupt
	E_HAL_SC_UART_INT_RLSI = 0x04,	//Receiver Line Status Interrupt
	E_HAL_SC_UART_INT_MSI = 0x08	//Modem Status interrupt
} HAL_SC_UART_INT_BIT_MAP;

typedef enum {
	//E_HAL_SC_LEVEL_LOW = FALSE,
	//E_HAL_SC_LEVEL_HIGH = TRUE
	E_HAL_SC_LEVEL_LOW,
	E_HAL_SC_LEVEL_HIGH
} HAL_SC_LEVEL_CTRL;

typedef enum {
	E_HAL_SC_IO_FUNC_ENABLE,	//As general I/O pin, idle is logical high
	E_HAL_SC_IO_FORCED_LOW	//I/O pin forced to logical low
} HAL_SC_IO_CTRL;

typedef enum {
	E_HAL_SC_TSIO_DISABLE,
	E_HAL_SC_TSIO_BGA,
	E_HAL_SC_TSIO_CARD
} HAL_SC_TSIO_CTRL;

typedef enum {
	E_HAL_SC_CD_IN_CLEAR,
	E_HAL_SC_CD_OUT_CLEAR,
	E_HAL_SC_CD_ALL_CLEAR
} HAL_SC_CD_INT_CLEAR;

typedef enum {
	E_HAL_SC_RX_FIFO_INT_TRI_1,
	E_HAL_SC_RX_FIFO_INT_TRI_4,
	E_HAL_SC_RX_FIFO_INT_TRI_8,
	E_HAL_SC_RX_FIFO_INT_TRI_14
} HAL_SC_RX_FIFO_INT_TRI_LEVEL;

typedef enum {
	E_HAL_SC_UART_INT_ID_CT,	//Character Timeout
	E_HAL_SC_UART_INT_ID_RLS,	//Receiver Line Status
	E_HAL_SC_UART_INT_ID_RDA,	//Receiver Data Available
	E_HAL_SC_UART_INT_ID_THRE,	//Transmitter Holding Register empty
	E_HAL_SC_UART_INT_ID_MS,	//Modem Status
	E_HAL_SC_UART_INT_ID_NONE	//No interrupt
} HAL_SC_UART_INT_ID;

typedef enum {
	E_SC_CARD_DET_HIGH_ACTIVE,
	E_SC_CARD_DET_LOW_ACTIVE
} HAL_SC_CARD_DET_TYPE;

typedef struct {
	MS_BOOL bCardInInt;
	MS_BOOL bCardOutInt;
} HAL_SC_CD_INT_STATUS;

typedef struct {
	HAL_SC_RX_FIFO_INT_TRI_LEVEL eTriLevel;
	MS_BOOL bEnableFIFO;
	MS_BOOL bClearRxFIFO;
	MS_BOOL bClearTxFIFO;
} HAL_SC_FIFO_CTRL;

typedef struct {
	MS_BOOL bFlowCtrlEn;
	MS_BOOL bParityChk;
	MS_BOOL bSmartCardMdoe;
	MS_U8 u8ReTryTime;
} HAL_SC_MODE_CTRL;

typedef struct {
	MS_BOOL bTxLevelInt;
	MS_BOOL bCGT_TxFail;
	MS_BOOL bCGT_RxFail;
	MS_BOOL bCWT_TxFail;
	MS_BOOL bCWT_RxFail;
	MS_BOOL bBGT_Fail;
	MS_BOOL bBWT_Fail;
} HAL_SC_TX_LEVEL_GWT_INT;

typedef struct {
	HAL_SC_VCC_CTRL eVccCtrlType;
	HAL_SC_VOLTAGE_CTRL eVoltageCtrl;
	HAL_SC_CLK_CTRL eClkCtrl;
	MS_U16 u16ClkDiv;
	MS_U8 u8UartMode;
	HAL_SC_CONV_CTRL eConvCtrl;
	HAL_SC_CARD_DET_TYPE eCardDetType;
	HAL_SC_TSIO_CTRL eTsioCtrl;
} HAL_SC_OPEN_SETTING;


//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
extern MS_VIRT _regSCBase[SC_DEV_NUM_MAX];	// for DRV register access

void HAL_SC_SetClk(MS_U8 u8SCID, HAL_SC_CLK_CTRL eClk);
MS_BOOL HAL_SC_GetHwFeature(SC_HW_FEATURE *pstHwFeature);
void HAL_SC_Init(MS_U8 u8SCID);
void HAL_SC_Exit(MS_U8 u8SCID);
MS_BOOL HAL_SC_Open(MS_U8 u8SCID, HAL_SC_OPEN_SETTING *pstOpenSetting);
void HAL_SC_Close(MS_U8 u8SCID, HAL_SC_VCC_CTRL eVccCtrlType);
void HAL_SC_RegMap(MS_VIRT u32RegBase);
void HAL_SC1_RegMap(MS_VIRT u32RegBase);
void HAL_SC_HW_RegMap(MS_VIRT u32RegBase);
void HAL_SC_PM_RegMap(MS_VIRT u32RegBase);
void HAL_SC_PowerCtrl(MS_U8 u8SCID, MS_BOOL bEnable);
void HAL_SC_CardInvert(MS_U8 u8SCID, HAL_SC_CARD_DET_TYPE eTpye);
MS_BOOL HAL_SC_CardVoltage_Config(MS_U8 u8SCID, HAL_SC_VOLTAGE_CTRL eVoltage,
				  HAL_SC_VCC_CTRL eVccCtrl);
MS_BOOL HAL_SC_IsParityError(MS_U8 u8SCID);
MS_BOOL HAL_SC_IsRxDataReady(MS_U8 u8SCID);
MS_BOOL HAL_SC_IsTxFIFO_Empty(MS_U8 u8SCID);
void HAL_SC_WriteTxData(MS_U8 u8SCID, MS_U8 u8TxData);
MS_U8 HAL_SC_ReadRxData(MS_U8 u8SCID);
MS_BOOL HAL_SC_IsCardDetected(MS_U8 u8SCID);
void HAL_SC_SetIntCGWT(MS_U8 u8SCID, HAL_SC_CGWT_INT eIntCGWT);
void HAL_SC_SetIntBGWT(MS_U8 u8SCID, HAL_SC_BGWT_INT eIntBGWT);
MS_BOOL HAL_SC_GetIntTxLevelAndGWT(MS_U8 u8SCID, HAL_SC_TX_LEVEL_GWT_INT *pstTxLevelGWT);
void HAL_SC_ClearIntTxLevelAndGWT(MS_U8 u8SCID);
void HAL_SC_SetBWT(MS_U8 u8SCID, MS_U32 u32BWT_etu);
MS_BOOL HAL_SC_SetRstToAtrByBWT(MS_U8 u8SCID, MS_U32 u32Timeout, MS_U32 u32CardClk);
void HAL_SC_SetBGT(MS_U8 u8SCID, MS_U8 u8BGT_etu);
void HAL_SC_SetCWT(MS_U8 u8SCID, MS_U32 u32CWT_etu);
void HAL_SC_SetCGT(MS_U8 u8SCID, MS_U8 u8gCGT_etu);
MS_BOOL HAL_SC_SetUartDiv(MS_U8 u8SCID, HAL_SC_CLK_CTRL eClk, MS_U16 u16ClkDiv);
MS_BOOL HAL_SC_SetUartMode(MS_U8 u8SCID, MS_U8 u8UartMode);
MS_BOOL HAL_SC_SetConvention(MS_U8 u8SCID, HAL_SC_CONV_CTRL eConvCtrl);
MS_BOOL HAL_SC_SetInvConvention(MS_U8 u8SCID);
void HAL_SC_SetUartInt(MS_U8 u8SCID, MS_U8 u8IntBitMask);
HAL_SC_UART_INT_ID HAL_SC_GetUartIntID(MS_U8 u8SCID);
void HAL_SC_SetUartFIFO(MS_U8 u8SCID, HAL_SC_FIFO_CTRL *pstCtrlFIFO);
void HAL_SC_SetSmcModeCtrl(MS_U8 u8SCID, HAL_SC_MODE_CTRL *pstModeCtrl);
void HAL_SC_ClearCardDetectInt(MS_U8 u8SCID, HAL_SC_CD_INT_CLEAR eIntClear);
void HAL_SC_GetCardDetectInt(MS_U8 u8SCID, HAL_SC_CD_INT_STATUS *pstCardDetectInt);
void HAL_SC_ResetPadCtrl(MS_U8 u8SCID, HAL_SC_LEVEL_CTRL eLogicalLevel);
void HAL_SC_SmcVccPadCtrl(MS_U8 u8SCID, HAL_SC_LEVEL_CTRL eLogicalLevel);
void HAL_SC_InputOutputPadCtrl(MS_U8 u8SCID, HAL_SC_IO_CTRL eCtrl);
void HAL_SC_Ext8024ActiveSeq(MS_U8 u8SCID);
void HAL_SC_Ext8024ActiveSeqTSIO(MS_U8 u8SCID);
void HAL_SC_Ext8024DeactiveSeq(MS_U8 u8SCID);
void HAL_SC_Ext8024DeactiveSeqTSIO(MS_U8 u8SCID);
void HAL_SC_Int8024ActiveSeq(MS_U8 u8SCID);
void HAL_SC_Int8024ActiveSeqTSIO(MS_U8 u8SCID);
void HAL_SC_Int8024DeactiveSeq(MS_U8 u8SCID);
void HAL_SC_Int8024PullResetPadLow(MS_U8 u8SCID, MS_U32 u32HoldTimeInMs);
void HAL_SC_SmcClkCtrl(MS_U8 u8SCID, MS_BOOL bEnableClk);
void HAL_SC_PwrCutDeactiveCfg(MS_U8 u8SCID, MS_BOOL bVccOffPolHigh);
void HAL_SC_PwrCutDeactiveCtrl(MS_U8 u8SCID, MS_BOOL bEnable);
void HAL_SC_RxFailAlwaysDetCWT(MS_U8 u8SCID, MS_BOOL bEnable);
MS_BOOL HAL_SC_SetRstToIoTimeout(MS_U8 u8SCID, MS_U32 u32Timeout, MS_U32 u32CardClk);
void HAL_SC_ClearRstToIoTimeout(MS_U8 u8SCID);
void HAL_SC_RstToIoEdgeDetCtrl(MS_U8 u8SCID, MS_BOOL bEnable);
MS_BOOL HAL_SC_CheckIntRstToIoEdgeFail(MS_U8 u8SCID);
void HAL_SC_SetIntRstToIoEdgeFail(MS_U8 u8SCID, MS_BOOL bEnable);
void HAL_SC_ClkGateCtrl(MS_BOOL bEnable);
MS_BOOL HAL_SC_MdbGetPadConf(MS_U8 u8SCID, MS_U8 *pu8PadConf, MS_U8 u8Len);
MS_U8 HAL_SC_GetTxFifoCnt(MS_U8 u8SCID);
MS_U8 HAL_SC_GetTxLevel(MS_U8 u8SCID);

void hal_sc_set_riu_base(void __iomem *RIU_vaddr);
void hal_sc_set_extriu_base(void __iomem *extRIU_vaddr);
void hal_sc_set_pad_mux(void);
void hal_sc_set_clk_base(struct mdrv_sc_clk *pclk);
void hal_sc_set_clk_riu_base(void __iomem *clk_riu_vaddr);
void hal_sc_set_vcc_init(int vcc_high_active);


MS_U8 _HAL_SC_GetLsr(MS_U8 u8SCID);
void HAL_SC_MaskIntRstToIoEdgeFail(MS_U8 u8SCID);
void HAL_SC_Set_ctrl2_rec_parity_err_en(MS_U8 u8SCID);
void HAL_SC_Set_mcr_loop_en(MS_U8 u8SCID);
void hal_sc_set_rf_pop_delay(MS_U8 u8SCID, int rf_pop_delay);
#endif				// _HAL_SC_H_
