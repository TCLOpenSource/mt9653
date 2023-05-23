// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "mtk_sc_common.h"
#include "mtk_sc_clock.h"
#include <linux/delay.h>
REG16 __iomem *nonPM_vaddr;	//for STI
static REG16 __iomem *ext_nonPM_vaddr;

static struct clk *smart_ck;
static struct clk *smart_synth_432_ck;
static struct clk *smart_synth_27_ck;
static struct clk *smart2piu;
static struct clk *smart_synth_272piu;
static struct clk *smart_synth_4322piu;
static struct clk *mcu_nonpm2smart0;
static int hal_vcc_high_active;
static REG16 __iomem *clk16_nonPM_vaddr;
#define MsOS_DelayTask(x)  msleep_interruptible((unsigned int)x)
#define DELAY0						0
#define DELAY1						1
#define DELAY2						2
#define DELAY3						3

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include <linux/string.h>

//#include "MsCommon.h" //for STI
#include "mtk_sc_drv.h"
//#include "drvSYS.h" // for STI

// Internal Definition
#include "mtk_sc_reg.h"
#include "mtk_sc_hal.h"

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------
#define SC_DBG_ENABLE               2UL
//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#if SC_DBG_ENABLE
//#define SC_DBG(fmt, args...)        pr_info("[DEV][SMART][%06d] " fmt, __LINE__, ##args)
#define SC_ERR(fmt, args...)        pr_info("[DEV][SMART][%06d] " fmt, __LINE__, ##args)
#define SC_DBG(_d, _f, _a...)       pr_info("[HAL] "_f, ##_a)
#else
#define SC_DBG(fmt, args...)        {}
#define SC_ERR(fmt, args...)        pr_info("[DEV][SMART][%06d] " fmt, __LINE__, ##args)
#endif

#define SC_TOP_CKG_UART1_CLK        TOP_CKG_UART1_CLK_144M

#define SC_ACTIVE_VCC_CNT           0x01
#define SC_ACTIVE_IO_CNT            0x10
#define SC_ACTIVE_CLK_CNT           0x13
#define SC_ACTIVE_RST_CNT           0x16

#define SC_DEACTIVE_RST_CNT         0x03
#define SC_DEACTIVE_CLK_CNT         0x06
#define SC_DEACTIVE_IO_CNT          0x09
#define SC_DEACTIVE_VCC_CNT         0x0C


// Setting
//#define SC_EVENT()                  MsOS_SetEvent(s32CardEventId, SC_EVENT_CARD)
//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------

MS_VIRT _regSCBase[SC_DEV_NUM_MAX];
MS_VIRT _regSCHWBase;
MS_VIRT _regSCHPMBase;

//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
MS_U8 _HAL_SC_GetLsr(MS_U8 u8SCID)	//for STI remove static
{
	MS_U8 u8Data;

	u8Data = SC_READ(u8SCID, UART_LSR);

	return u8Data;
}

static void _HAL_SC_ClearLsr(MS_U8 u8SCID)
{
	SC_OR(u8SCID, UART_LSR_CLR, UART_LSR_CLR_FLAG);
	SC_AND(u8SCID, UART_LSR_CLR, ~(UART_LSR_CLR_FLAG));
}

static MS_U8 _HAL_SC_MdbGetPadConfData(MS_U16 u16PadConfReg, MS_U16 u16Mask, MS_U16 u16Bit)
{
	MS_U16 u16Tmp = 0x00;
	MS_U8 u8RetVal = 0x00;

	u16Tmp = u16PadConfReg & u16Mask;

	u16Bit = u16Bit >> 1;
	while (u16Bit) {
		u16Tmp = u16Tmp >> 1;
		u16Bit = u16Bit >> 1;
	}

	u8RetVal = (MS_U8) (u16Tmp & 0xFF);

	return u8RetVal;
}

static MS_U32 _HAL_SC_GetTopClkNum(MS_U8 u8SCID)
{
	MS_U16 u16ClkCtrl = 0;
	MS_U32 u32ClkNum = 0;

	if (u8SCID == 0)
		u32ClkNum = 144000000;


	return u32ClkNum;
}


//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------

void HAL_SC_RegMap(MS_VIRT u32RegBase)
{
	_regSCBase[0] = u32RegBase;
}

void HAL_SC1_RegMap(MS_VIRT u32RegBase)
{
	_regSCBase[1] = u32RegBase;
}

void HAL_SC_HW_RegMap(MS_VIRT u32RegBase)
{
	_regSCHWBase = u32RegBase;
}

void HAL_SC_PM_RegMap(MS_VIRT u32RegBase)
{
	pr_info("[%s][%d] not support\n", __func__, __LINE__);
}

void HAL_SC_SetClk(MS_U8 u8SCID, HAL_SC_CLK_CTRL eClk)
{
	MS_U16 reg;

	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d eClk=%d\n", __func__, u8SCID, eClk);
	//reg = HW_READ(REG_TOP_CKG_SM_CA);
	reg = SC_READ(u8SCID, REG_TOP_CKG_SM_CA);

	//[1] reg_sw_smart_ca_clk = 0:27Mhz
	//HW_WRITE(REG_TOP_CKG_SM_CACLK_EXT, HW_READ(REG_TOP_CKG_SM_CACLK_EXT) & 0xFFFD);
	SC_AND(u8SCID, REG_TOP_CKG_SM_CACLK_EXT, 0xFFFD);

	if (u8SCID == 0) {
		reg &= (~TOP_CKG_SM_CA0_CLK_MASK);
		//[0] smart card sythesizer load MN = 0
		//HW_WRITE(REG_TOP_CKG_SM_CACLK_EXT, HW_READ(REG_TOP_CKG_SM_CACLK_EXT) & 0xFFFC);
		SC_AND(u8SCID, REG_TOP_CKG_SM_CACLK_EXT, 0xFFFC);

		pr_info("[%s][%d] 4M\n", __func__, __LINE__, u8SCID);
		//432Mhz*(M+1)/2N  432*(1/108)=4Mhz
		//set N=54
		//HW_WRITE(REG_TOP_CKG_SM_CACLK_N, 0x36);
		SC_WRITE(u8SCID, REG_TOP_CKG_SM_CACLK_N, 0x36);

		//set M=0
		//HW_WRITE(REG_TOP_CKG_SM_CACLK_M, HW_READ(REG_TOP_CKG_SM_CACLK_M) & ~0xFF00);
		SC_AND(u8SCID, REG_TOP_CKG_SM_CACLK_M, 0x00FF);

		//[0] smart card sythesizer load MN = 0
		//HW_WRITE(REG_TOP_CKG_SM_CACLK_EXT, HW_READ(REG_TOP_CKG_SM_CACLK_EXT) & 0xFFFE);
		SC_AND(u8SCID, REG_TOP_CKG_SM_CACLK_EXT, 0xFFFE);

		//[0] smart card sythesizer load MN = 1
		//HW_WRITE(REG_TOP_CKG_SM_CACLK_EXT, HW_READ(REG_TOP_CKG_SM_CACLK_EXT) | 0x0001);
		SC_OR(u8SCID, REG_TOP_CKG_SM_CACLK_EXT, 0x0001);

		//[0] smart card sythesizer load MN = 0
		//HW_WRITE(REG_TOP_CKG_SM_CACLK_EXT, HW_READ(REG_TOP_CKG_SM_CACLK_EXT) & 0xFFFE);
		SC_AND(u8SCID, REG_TOP_CKG_SM_CACLK_EXT, 0xFFFE);

		//[1] from smart card synthesizer
		//HW_WRITE(REG_TOP_CKG_SM_CACLK_EXT, HW_READ(REG_TOP_CKG_SM_CACLK_EXT) | 0x0002);
		SC_OR(u8SCID, REG_TOP_CKG_SM_CACLK_EXT, 0x0002);
	} else if (u8SCID == 1) {
		SC_ERR("[SC] not support SM1\n");
	}

	//HW_WRITE(REG_TOP_CKG_SM_CA, reg);
	SC_WRITE(u8SCID, REG_TOP_CKG_SM_CA, reg);
}

MS_BOOL HAL_SC_GetHwFeature(SC_HW_FEATURE *pstHwFeature)
{
	if (pstHwFeature == NULL)
		return FALSE;

	pstHwFeature->eRstToIoHwFeature = E_SC_RST_TO_IO_HW_TIMER_IND;
	pstHwFeature->bExtCwtFixed = TRUE;

	return TRUE;
}

void HAL_SC_Init(MS_U8 u8SCID)
{
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);

	// enable the source clock of smart card.
	HAL_SC_PowerCtrl(u8SCID, TRUE);

	// Set lsr_clr_flag_option to clear lsr by 1029_36[0]
	SC_WRITE(u8SCID, UART_LSR_CLR, SC_READ(u8SCID, UART_LSR_CLR) | UART_LSR_CLR_FLAG_OPT);

	// Let the related I/O Low
	SC_WRITE(u8SCID, UART_LCR, UART_LCR_SBC);	//I/O

	SC_AND(u8SCID, UART_CTRL2, ~(UART_CTRL2_TX_LEVEL));	// Clear tx level
	SC_WRITE(u8SCID, UART_CTRL2,
					SC_READ(u8SCID, UART_CTRL2) |
					UART_CTRL2_TX_LEVEL_9_TO_8 |
					UART_CTRL2_CGWT_MASK |
					UART_CTRL2_BGWT_MASK);	// tx fifo from 9 -> 8

	// BGT default value will be 0x16, reset to 0 here to disable the BGT int
	HAL_SC_SetBGT(u8SCID, 0x00);

	// Set external 8024 to inactive mode
	HAL_SC_Ext8024DeactiveSeq(u8SCID);

	// open smard_card_mode = 1
	SC_WRITE(u8SCID, UART_SCMR, SC_READ(u8SCID, UART_SCMR) | UART_SCMR_SMARTCARD);

	// disable smc clock
	HAL_SC_SmcClkCtrl(u8SCID, FALSE);
	pr_info("[%s][%d] u8SCID=%d PASS\n", __func__, __LINE__, u8SCID);
}

void HAL_SC_Exit(MS_U8 u8SCID)
{
	HAL_SC_PowerCtrl(u8SCID, FALSE);
}

MS_BOOL HAL_SC_Open(MS_U8 u8SCID, HAL_SC_OPEN_SETTING *pstOpenSetting)
{
	MS_U8 u8RegTmp;

	pr_info("[%s][%d] u8SCID=%d pstOpenSetting=%x\n", __func__, __LINE__, u8SCID,
	       pstOpenSetting);
	if (pstOpenSetting == NULL)
		return FALSE;
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (pstOpenSetting->eVccCtrlType == E_HAL_SC_VCC_INT_8024) {
		pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
		// Config SMC PAD for internal analog circuit
		SC_AND(u8SCID, UART_CTRL6, ~UART_CTRL6_GCR_SMC_3P3_GPIO_EN);

		// Power down analog circuit and SMC PAD
		SC_OR(u8SCID, UART_CTRL6, UART_CTRL6_PD_SMC_LDO);
		SC_OR(u8SCID, UART_CTRL7, UART_CTRL7_PD_SMC_PAD);
		SC_OR(u8SCID, UART_CTRL14, UART_CTRL14_PD_SD_LDO);

		// Enable auto seq control and set all interface off
		SC_WRITE(u8SCID, UART_CTRL5,
			 SC_READ(u8SCID, UART_CTRL5) | UART_CTRL5_AUTO_SEQ_CTRL);
		SC_WRITE(u8SCID, UART_CTRL5, SC_READ(u8SCID, UART_CTRL5) | UART_CTRL5_PAD_MASK);
		SC_WRITE(u8SCID, UART_CTRL5, SC_READ(u8SCID, UART_CTRL5) & (~UART_CTRL5_PAD_MASK));

		// need to call it, otherwise tx/rx will be failed
		SC_OR(u8SCID, UART_CTRL7, UART_CTRL7_ACT_PULLUP_EN);	// active pull up en
	} else {
		pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
		// Enable GPIO function for external 8024 use case
		SC_OR(u8SCID, UART_CTRL6, UART_CTRL6_GCR_SMC_3P3_GPIO_EN);

		// Power on SMC PAD
		SC_AND(u8SCID, UART_CTRL7, ~UART_CTRL7_PD_SMC_PAD);
	}
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	// Set card detect type
	HAL_SC_CardInvert(u8SCID, pstOpenSetting->eCardDetType);
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	// Set analog circuit donot care card reset and counter donot care card reset
	// *****About counter donot care card reset*****
	// For 5V case, if RST is '0',
	// then HW internal counter will be also 0 to cause VCC always stays at 3V
	// So we must set UART_CTRL8_ANALOG_CNT_DONT_CARE_CARD_RESET to ignore RST
	SC_OR(u8SCID, UART_CTRL8,
	      UART_CTRL8_ANALOG_CNT_DONT_CARE_CARD_RESET | UART_CTRL8_ANALOG_DC_CARD_RESET);
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	// power on internal analog circuit and set card voltage
	HAL_SC_CardVoltage_Config(u8SCID, pstOpenSetting->eVoltageCtrl,
				  pstOpenSetting->eVccCtrlType);
	if (u8SCID == 1 && pstOpenSetting->eTsioCtrl == E_HAL_SC_TSIO_BGA
	    && pstOpenSetting->eVccCtrlType == E_HAL_SC_VCC_EXT_8024) {
		pr_info("[%s][%d] not support\n", __func__, __LINE__);
	}
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (HAL_SC_SetUartDiv(u8SCID,
	pstOpenSetting->eClkCtrl, pstOpenSetting->u16ClkDiv) == FALSE)
		return FALSE;

	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (HAL_SC_SetUartMode(u8SCID, pstOpenSetting->u8UartMode) == FALSE)
		return FALSE;

	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (HAL_SC_SetConvention(u8SCID, pstOpenSetting->eConvCtrl) == FALSE)
		return FALSE;

	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (pstOpenSetting->eVccCtrlType == E_HAL_SC_VCC_EXT_8024) {
		pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
		HAL_SC_Ext8024DeactiveSeq(u8SCID);
		pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
		MsOS_DelayTask(1);	// For falling edge after CardIn detect.
	} else if (pstOpenSetting->eVccCtrlType == E_HAL_SC_VCC_INT_8024) {
		// Power cut config
		HAL_SC_PwrCutDeactiveCtrl(u8SCID, FALSE);
		HAL_SC_PwrCutDeactiveCfg(u8SCID, TRUE);

	} else if (pstOpenSetting->eVccCtrlType == E_HAL_SC_VCC_OCP_HIGH) {
		SC_OR(u8SCID, UART_SCMR,
		UART_SCMR_SMARTCARD);
	} else {
		if (pstOpenSetting->eVccCtrlType == E_HAL_SC_VCC_HIGH) {
			SC_AND(u8SCID, UART_SCFR,
			~(UART_SCFR_V_ENABLE));
		} else if (pstOpenSetting->eVccCtrlType == E_HAL_SC_VCC_LOW) {
			SC_OR(u8SCID, UART_SCFR,
			(UART_SCFR_V_ENABLE));
		}

	}
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	// Init UART_SCSR
	u8RegTmp = (SC_READ(u8SCID, UART_SCSR) & UART_SCSR_CLK);
	SC_WRITE(u8SCID, UART_SCSR, u8RegTmp);
	pr_info("[%s][%d] u8SCID=%d PASS\n", __func__, __LINE__, u8SCID);
	return TRUE;
}

void HAL_SC_Close(MS_U8 u8SCID, HAL_SC_VCC_CTRL eVccCtrlType)
{
	// Disable Smartcard Clk

	SC_WRITE(u8SCID, UART_SCSR, 0x00);	// disable clock
	// Disable Smartcard IC Vcc Control

	if (eVccCtrlType == E_HAL_SC_VCC_EXT_8024) {
		SC_AND(u8SCID, UART_SCFR, ~(UART_SCFR_V_ENABLE));
	} else if (eVccCtrlType == E_HAL_SC_VCC_INT_8024) {
	} else {
		if ((eVccCtrlType == E_HAL_SC_VCC_HIGH) ||
		(eVccCtrlType == E_HAL_SC_VCC_OCP_HIGH))
			HAL_SC_SmcVccPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
		else
			HAL_SC_SmcVccPadCtrl(u8SCID, E_HAL_SC_LEVEL_HIGH);

	}
}

void HAL_SC_PowerCtrl(MS_U8 u8SCID, MS_BOOL bEnable)
{
	pr_info("%s u8SCID=%d bEnable=%d\n", __func__, u8SCID, bEnable);
	if (u8SCID == 0) {
		if (bEnable) {
			// SM0: enable switch to specified clk
			pr_info("[%s][%d]enable\n", __func__, __LINE__);
			if (clk_prepare_enable(smart_ck) < 0) {
				pr_info("[%s][%d] smart_ck prepare & enable fail\n", __func__,
				       __LINE__);
			}
			pr_info("[%s][%d] smart_ck=%ld\n", __func__, __LINE__,
			 clk_get_rate(smart_ck));
			clk_set_rate(smart_ck, 144000000);
			pr_info("[%s][%d] smart_ck=%ld\n", __func__, __LINE__,
			 clk_get_rate(smart_ck));

			if (clk_prepare_enable(smart_synth_432_ck) < 0) {
				pr_info("[%s][%d] smart_synth_432_ck enable fail\n", __func__,
				       __LINE__);
			}

			if (clk_prepare_enable(smart_synth_27_ck) < 0) {
				pr_info("[%s][%d] smart_synth_27_ck enable fail\n", __func__,
				       __LINE__);
			}

			if (clk_prepare_enable(smart2piu) < 0) {
				pr_info("[%s][%d] smart2piu enable fail\n", __func__,
				       __LINE__);
			}

			if (clk_prepare_enable(smart_synth_272piu) < 0) {
				pr_info("[%s][%d] smart_synth_272piu enable fail\n", __func__,
				       __LINE__);
			}

			if (clk_prepare_enable(smart_synth_4322piu) < 0) {
				pr_info("[%s][%d] smart_synth_4322piu enable fail\n", __func__,
				       __LINE__);
			}

			if (clk_prepare_enable(mcu_nonpm2smart0) < 0) {
				pr_info("[%s][%d] mcu_nonpm2smart0 enable fail\n", __func__,
				       __LINE__);
			}

			HAL_SC_SmcClkCtrl(u8SCID, TRUE);	// enable clock
		} else {
			pr_info("[%s][%d]disable\n", __func__, __LINE__);
			HAL_SC_SmcClkCtrl(u8SCID, FALSE);	// disable clock
			// SM0: disable clk
			clk_disable_unprepare(smart_ck);
			clk_disable_unprepare(smart_synth_432_ck);
			clk_disable_unprepare(smart_synth_27_ck);
			clk_disable_unprepare(smart2piu);
			clk_disable_unprepare(smart_synth_272piu);
			clk_disable_unprepare(smart_synth_4322piu);
			clk_disable_unprepare(mcu_nonpm2smart0);
		}
	}
	if (u8SCID == 1)
		SC_ERR("[SC] not support SM1\n");

}

void HAL_SC_CardInvert(MS_U8 u8SCID, HAL_SC_CARD_DET_TYPE eTpye)
{
	if (eTpye == E_SC_CARD_DET_LOW_ACTIVE) {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, eTpye);
		SC_OR(u8SCID, UART_CTRL3, UART_INVERT_CD);
	} else {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, eTpye);
		SC_AND(u8SCID, UART_CTRL3, ~UART_INVERT_CD);
	}
}

MS_BOOL HAL_SC_CardVoltage_Config(MS_U8 u8SCID, HAL_SC_VOLTAGE_CTRL eVoltage,
				  HAL_SC_VCC_CTRL eVccCtrl)
{
	pr_info("[%s][%d] u8SCID=%d eVoltage=%d eVccCtrl=%d\n",
	__func__, __LINE__, u8SCID, eVoltage, eVccCtrl);
	if (eVccCtrl == E_HAL_SC_VCC_EXT_8024) {
		if (eVoltage == E_HAL_SC_VOL_CTRL_5V) {
			// go 5v
			//HW_WRITE(REG_TOP_3V_5V_SELECT, (HW_READ(REG_TOP_3V_5V_SELECT) | (0x1)));
			SC_OR(u8SCID, REG_TOP_3V_5V_SELECT, (0x1));
		} else if (eVoltage == E_HAL_SC_VOL_CTRL_3P3V) {	// go 3.3v
			// go 3.3v
			//HW_WRITE(REG_TOP_3V_5V_SELECT, (HW_READ(REG_TOP_3V_5V_SELECT) & ~(0x1)));
			SC_AND(u8SCID, REG_TOP_3V_5V_SELECT, ~(0x1));
		} else {
			return FALSE;
		}
		MsOS_DelayTask(1);
		pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
		return TRUE;
	}

	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	return FALSE;

}

MS_BOOL HAL_SC_IsParityError(MS_U8 u8SCID)
{
	MS_U8 u8Data;

	u8Data = _HAL_SC_GetLsr(u8SCID);
	_HAL_SC_ClearLsr(u8SCID);

	if ((u8Data & UART_LSR_PE))
		return TRUE;
	else
		return FALSE;
}

MS_BOOL HAL_SC_IsRxDataReady(MS_U8 u8SCID)
{
	MS_U8 u8Data;

	u8Data = _HAL_SC_GetLsr(u8SCID);
	if ((u8Data & UART_LSR_DR))
		return TRUE;
	else
		return FALSE;
}

MS_BOOL HAL_SC_IsTxFIFO_Empty(MS_U8 u8SCID)
{
	if ((SC_READ(u8SCID, UART_LSR) & UART_LSR_THRE))
		return TRUE;
	else
		return FALSE;
}

void HAL_SC_WriteTxData(MS_U8 u8SCID, MS_U8 u8TxData)
{
	SC_WRITE(u8SCID, UART_TX, u8TxData);
}

MS_U8 HAL_SC_ReadRxData(MS_U8 u8SCID)
{
	return SC_READ(u8SCID, UART_RX);
}

MS_BOOL HAL_SC_IsCardDetected(MS_U8 u8SCID)
{
	//pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__,u8SCID);
	if ((SC_READ(u8SCID, UART_SCSR) & UART_SCSR_DETECT)) {
		//pr_info("[%s][%d] card in\n", __func__, __LINE__,u8SCID);
		return TRUE;
	}

	pr_info("[%s][%d] card out!!\n", __func__, __LINE__, u8SCID);
	return FALSE;

}

void HAL_SC_SetIntCGWT(MS_U8 u8SCID, HAL_SC_CGWT_INT eIntCGWT)
{
	switch (eIntCGWT) {
	case E_HAL_SC_CGWT_INT_DISABLE:
	default:
		SC_WRITE(u8SCID, UART_SCSR,
			 SC_READ(u8SCID, UART_SCSR) | (UART_SCSR_CWT_MASK + UART_SCSR_CGT_MASK));
		SC_WRITE(u8SCID, UART_CTRL2, SC_READ(u8SCID, UART_CTRL2) | (UART_CTRL2_CGWT_MASK));
		break;

	case E_HAL_SC_CGWT_INT_ENABLE:
		SC_WRITE(u8SCID, UART_SCSR,
			 SC_READ(u8SCID, UART_SCSR) & (~(UART_SCSR_CWT_MASK + UART_SCSR_CGT_MASK)));
		SC_WRITE(u8SCID, UART_CTRL2, SC_READ(u8SCID, UART_CTRL2) & (~UART_CTRL2_CGWT_MASK));
		break;

	case E_HAL_SC_CWT_ENABLE_CGT_DISABLE:
		SC_WRITE(u8SCID, UART_SCSR, SC_READ(u8SCID, UART_SCSR) & (~(UART_SCSR_CWT_MASK)));
		SC_WRITE(u8SCID, UART_SCSR, SC_READ(u8SCID, UART_SCSR) | (UART_SCSR_CGT_MASK));
		SC_WRITE(u8SCID, UART_CTRL2, SC_READ(u8SCID, UART_CTRL2) & (~UART_CTRL2_CGWT_MASK));
		break;

	}
}

void HAL_SC_SetIntBGWT(MS_U8 u8SCID, HAL_SC_BGWT_INT eIntBGWT)
{
	switch (eIntBGWT) {
	case E_HAL_SC_BGWT_INT_DISABLE:
	default:
		SC_WRITE(u8SCID, UART_SCSR, SC_READ(u8SCID, UART_SCSR) | (UART_SCSR_BGT_MASK));
		SC_WRITE(u8SCID, UART_CTRL2, SC_READ(u8SCID, UART_CTRL2) | (UART_CTRL2_BGWT_MASK));
		break;

	case E_HAL_SC_BGWT_INT_ENABLE:
		SC_WRITE(u8SCID, UART_SCSR, SC_READ(u8SCID, UART_SCSR) & (~(UART_SCSR_BGT_MASK)));
		SC_WRITE(u8SCID, UART_CTRL2, SC_READ(u8SCID, UART_CTRL2) & (~UART_CTRL2_BGWT_MASK));
		break;

	case E_HAL_SC_BWT_ENABLE_BGT_DISABLE:
		SC_WRITE(u8SCID, UART_SCSR, SC_READ(u8SCID, UART_SCSR) | (UART_SCSR_BGT_MASK));
		SC_WRITE(u8SCID, UART_CTRL2, SC_READ(u8SCID, UART_CTRL2) & (~UART_CTRL2_BGWT_MASK));
		break;
	}
}

MS_BOOL HAL_SC_GetIntTxLevelAndGWT(MS_U8 u8SCID, HAL_SC_TX_LEVEL_GWT_INT *pstTxLevelGWT)
{
	MS_U8 u8RegData;

	u8RegData = SC_READ(u8SCID, UART_GWT_INT);

	//init
	pstTxLevelGWT->bTxLevelInt = FALSE;
	pstTxLevelGWT->bCWT_RxFail = FALSE;
	pstTxLevelGWT->bCWT_TxFail = FALSE;
	pstTxLevelGWT->bCGT_RxFail = FALSE;
	pstTxLevelGWT->bCGT_TxFail = FALSE;
	pstTxLevelGWT->bBGT_Fail = FALSE;
	pstTxLevelGWT->bBWT_Fail = FALSE;

	if (u8RegData & UART_GWT_TX_LEVEL_INT)
		pstTxLevelGWT->bTxLevelInt = TRUE;

	if (u8RegData & UART_GWT_CWT_RX_FAIL)
		pstTxLevelGWT->bCWT_RxFail = TRUE;

	if (u8RegData & UART_GWT_CWT_TX_FAIL)
		pstTxLevelGWT->bCWT_TxFail = TRUE;

	if (u8RegData & UART_GWT_CGT_RX_FAIL)
		pstTxLevelGWT->bCGT_RxFail = TRUE;

	if (u8RegData & UART_GWT_CGT_TX_FAIL)
		pstTxLevelGWT->bCGT_TxFail = TRUE;

	if (u8RegData & UART_GWT_BGT_FAIL)
		pstTxLevelGWT->bBGT_Fail = TRUE;

	if (u8RegData & UART_GWT_BWT_FAIL)
		pstTxLevelGWT->bBWT_Fail = TRUE;

	return TRUE;
}

void HAL_SC_ClearIntTxLevelAndGWT(MS_U8 u8SCID)
{
	SC_WRITE(u8SCID, UART_CTRL2, SC_READ(u8SCID, UART_CTRL2) | UART_CTRL2_FLAG_CLEAR);
	SC_WRITE(u8SCID, UART_CTRL2, SC_READ(u8SCID, UART_CTRL2) & (~UART_CTRL2_FLAG_CLEAR));
}

void HAL_SC_SetBWT(MS_U8 u8SCID, MS_U32 u32BWT_etu)
{
	// The HW counter is start from '0', so we need to subtract '1' from u32BWT_etu
	if (u32BWT_etu > 0)
		u32BWT_etu = u32BWT_etu - 1;

	SC_WRITE(u8SCID, UART_SCBWT_0, (MS_U8) (u32BWT_etu & 0xFF));
	SC_WRITE(u8SCID, UART_SCBWT_1, (MS_U8) ((u32BWT_etu >> 8) & 0xFF));
	SC_WRITE(u8SCID, UART_SCBWT_2, (MS_U8) ((u32BWT_etu >> 16) & 0xFF));
	SC_WRITE(u8SCID, UART_SCBWT_3, (MS_U8) ((u32BWT_etu >> 24) & 0xFF));
}

MS_BOOL HAL_SC_SetRstToAtrByBWT(MS_U8 u8SCID, MS_U32 u32Timeout, MS_U32 u32CardClk)
{
	MS_U32 u32Num;
	MS_U32 u32Clk;

	u32Clk = _HAL_SC_GetTopClkNum(u8SCID);
	if (u32Clk == 0 || u32CardClk == 0)
		return FALSE;

	u32Num = (MS_U32) (u32Clk / u32CardClk);
	u32Num = (u32Num * u32Timeout) / 16UL;

	HAL_SC_SetBWT(u8SCID, u32Num);

	return TRUE;
}

void HAL_SC_SetBGT(MS_U8 u8SCID, MS_U8 u8BGT_etu)
{
	// The HW counter is start from '0', so we need to subtract '1' from u8BGT_etu
	if (u8BGT_etu > 0)
		u8BGT_etu = u8BGT_etu - 1;

	SC_WRITE(u8SCID, UART_SCBGT, u8BGT_etu);
}

void HAL_SC_SetCWT(MS_U8 u8SCID, MS_U32 u32CWT_etu)
{
	// The HW counter is start from '0', so we need to subtract '1' from u32CWT_etu
	if (u32CWT_etu > 0)
		u32CWT_etu = u32CWT_etu - 1;

	SC_WRITE(u8SCID, UART_SCCWT_L, (MS_U8) (u32CWT_etu & 0xFF));
	SC_WRITE(u8SCID, UART_SCCWT_H, (MS_U8) ((u32CWT_etu >> 8) & 0xFF));

	// Fix ext-CWT issue
	SC_OR(u8SCID, UART_CTRL8, UART_CTRL8_CWT_EXT_EN);
	SC_WRITE(u8SCID, UART_CTRL9, (MS_U8) ((u32CWT_etu >> 16) & 0xFF));
}

void HAL_SC_SetCGT(MS_U8 u8SCID, MS_U8 u8gCGT_etu)
{
	MS_U8 u8ExtraGT;

	if (u8gCGT_etu <= 12)
		u8ExtraGT = 0;
	else
		u8ExtraGT = u8gCGT_etu - 12;

	SC_WRITE(u8SCID, UART_SCCGT, u8ExtraGT);
}

MS_BOOL HAL_SC_SetUartDiv(MS_U8 u8SCID, HAL_SC_CLK_CTRL eClk, MS_U16 u16ClkDiv)
{
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	MS_U16 u16div;
	MS_U32 clk;

	if (u8SCID >= SC_DEV_NUM)
		return FALSE;

	clk = _HAL_SC_GetTopClkNum(u8SCID);
	if (clk == 0)
		return FALSE;

	SC_WRITE(u8SCID, UART_MCR, 0);

	if (eClk != E_HAL_SC_CLK_TSIO) {
		pr_info("[%s][%d]\n", __func__, __LINE__);
		switch (eClk) {
		case E_HAL_SC_CLK_3M:
			//u16clk = TOP_CKG_CAM_SRC_27M_D8;/* 3.375M */
			clk = clk / 1000;
			u16div = (clk * u16ClkDiv) / (16 * 3375);
			break;
		case E_HAL_SC_CLK_4P5M:
			//u16clk = TOP_CKG_CAM_SRC_27M_D6;/* 4.5M */
			clk = clk / 1000;
			u16div = (clk * u16ClkDiv) / (16 * 4500);
			break;
		case E_HAL_SC_CLK_6M:
			//u16clk = TOP_CKG_CAM_SRC_27M_D4;/* 6.75M */
			clk = clk / 1000;
			u16div = (clk * u16ClkDiv) / (16 * 6750);
			break;
		case E_HAL_SC_CLK_13M:
			//u16clk = TOP_CKG_CAM_SRC_27M_D2;/* 13.5M */
			clk = clk / 1000;
			SC_WRITE(u8SCID, UART_MCR, UART_MCR_FAST);	// UART_MCR_FAST on mode
			clk = clk / 16;
			u16div = (13500 * 65536) / (clk * u16ClkDiv);
			break;
		case E_HAL_SC_CLK_4M:
			clk = clk / 1000;
			u16div = (clk * u16ClkDiv) / (16 * 4000);
			break;
		default:
			return FALSE;
		}

		// Switch to specified clk
		HAL_SC_SetClk(u8SCID, eClk);

		// Set divider
		SC_OR(u8SCID, UART_LCR, UART_LCR_DLAB);	// Line Control Register
		SC_WRITE(u8SCID, UART_DLL, u16div & 0x00FF);	// Divisor Latch Low
		SC_WRITE(u8SCID, UART_DLM, u16div >> 8);	// Divisor Latch High
		SC_AND(u8SCID, UART_LCR, ~(UART_LCR_DLAB));	// Line Control Register
	}

	return TRUE;
}

MS_BOOL HAL_SC_SetUartMode(MS_U8 u8SCID, MS_U8 u8UartMode)
{
	if (u8SCID >= SC_DEV_NUM)
		return FALSE;

	SC_WRITE(u8SCID, UART_LCR, (SC_READ(u8SCID, UART_LCR) & 0xE0) | u8UartMode);

	// If parity check is enable, open parity error record
	if (u8UartMode & UART_LCR_PARITY) {
		SC_OR(u8SCID, UART_CTRL2, UART_CTRL2_REC_PE_EN);
		SC_WRITE(u8SCID, UART_CTRL3, SC_READ(u8SCID, UART_CTRL3) | UART_TRAN_ETU_CTL);
	}
	return TRUE;
}

MS_BOOL HAL_SC_SetConvention(MS_U8 u8SCID, HAL_SC_CONV_CTRL eConvCtrl)
{
	if (u8SCID >= SC_DEV_NUM)
		return FALSE;

	if (eConvCtrl == E_HAL_SC_CONV_DIRECT) {
		// direct convention
		SC_WRITE(u8SCID, UART_SCCR,
			 SC_READ(u8SCID,
				 UART_SCCR) & ~(UART_SCCR_RX_BSWAP | UART_SCCR_RX_BINV |
						UART_SCCR_TX_BSWAP | UART_SCCR_TX_BINV));
	} else {
		// inverse convention
		SC_WRITE(u8SCID, UART_SCCR,
			 SC_READ(u8SCID,
				 UART_SCCR) | (UART_SCCR_RX_BSWAP | UART_SCCR_RX_BINV |
					       UART_SCCR_TX_BSWAP | UART_SCCR_TX_BINV));
	}

	return TRUE;
}

MS_BOOL HAL_SC_SetInvConvention(MS_U8 u8SCID)
{
	if (u8SCID >= SC_DEV_NUM)
		return FALSE;

	SC_XOR(u8SCID, UART_SCCR,
	       UART_SCCR_RX_BSWAP | UART_SCCR_RX_BINV | UART_SCCR_TX_BSWAP | UART_SCCR_TX_BINV);
	return TRUE;
}

void HAL_SC_SetUartInt(MS_U8 u8SCID, MS_U8 u8IntBitMask)
{
	MS_U8 u8RegData = 0x00;

	if (u8IntBitMask & E_HAL_SC_UART_INT_RDI)
		u8RegData |= UART_IER_RDI;
	if (u8IntBitMask & E_HAL_SC_UART_INT_THRI)
		u8RegData |= UART_IER_THRI;
	if (u8IntBitMask & E_HAL_SC_UART_INT_RLSI)
		u8RegData |= UART_IER_RLSI;
	if (u8IntBitMask & E_HAL_SC_UART_INT_MSI)
		u8RegData |= UART_IER_MSI;

	SC_WRITE(u8SCID, UART_IER, u8RegData);
}

HAL_SC_UART_INT_ID HAL_SC_GetUartIntID(MS_U8 u8SCID)
{
	HAL_SC_UART_INT_ID eIntID;
	MS_U8 u8GetIntID = 0x00;
	MS_U16 u16IIRStatus = SC_READ(u8SCID, UART_IIR);

	if (u16IIRStatus & UART_IIR_NO_INT) {
		eIntID = E_HAL_SC_UART_INT_ID_NONE;
	} else {
		u8GetIntID = (u16IIRStatus & UART_IIR_ID);
		switch (u8GetIntID) {
		case UART_IIR_RLSI:
			eIntID = E_HAL_SC_UART_INT_ID_RLS;
			break;

		case UART_IIR_RDI:
			eIntID = E_HAL_SC_UART_INT_ID_RDA;
			break;

		case UART_IIR_TOI:
			eIntID = E_HAL_SC_UART_INT_ID_CT;
			break;

		case UART_IIR_THRI:
			eIntID = E_HAL_SC_UART_INT_ID_THRE;
			break;

		case UART_IIR_MSI:
			eIntID = E_HAL_SC_UART_INT_ID_MS;
			break;

		default:
			eIntID = E_HAL_SC_UART_INT_ID_NONE;
			break;
		}
	}

	return eIntID;
}

void HAL_SC_SetUartFIFO(MS_U8 u8SCID, HAL_SC_FIFO_CTRL *pstCtrlFIFO)
{
	MS_U8 u8RegData = 0x00;

	if (pstCtrlFIFO->bEnableFIFO)
		u8RegData |= UART_FCR_ENABLE_FIFO;
	if (pstCtrlFIFO->bClearRxFIFO)
		u8RegData |= UART_FCR_CLEAR_RCVR;
	if (pstCtrlFIFO->bClearTxFIFO)
		u8RegData |= UART_FCR_CLEAR_XMIT;

	if (pstCtrlFIFO->eTriLevel == E_HAL_SC_RX_FIFO_INT_TRI_1)
		u8RegData |= UART_FCR_TRIGGER_1;
	if (pstCtrlFIFO->eTriLevel == E_HAL_SC_RX_FIFO_INT_TRI_4)
		u8RegData |= UART_FCR_TRIGGER_4;
	if (pstCtrlFIFO->eTriLevel == E_HAL_SC_RX_FIFO_INT_TRI_8)
		u8RegData |= UART_FCR_TRIGGER_8;
	if (pstCtrlFIFO->eTriLevel == E_HAL_SC_RX_FIFO_INT_TRI_14)
		u8RegData |= UART_FCR_TRIGGER_14;

	SC_WRITE(u8SCID, UART_FCR, u8RegData);
}

void HAL_SC_SetSmcModeCtrl(MS_U8 u8SCID, HAL_SC_MODE_CTRL *pstModeCtrl)
{
	MS_U8 u8RegData = 0x00;

	if (pstModeCtrl->bFlowCtrlEn)
		u8RegData |= UART_SCMR_FLOWCTRL;
	if (pstModeCtrl->bParityChk)
		u8RegData |= UART_SCMR_PARITYCHK;
	if (pstModeCtrl->bSmartCardMdoe)
		u8RegData |= UART_SCMR_SMARTCARD;

	u8RegData |= (pstModeCtrl->u8ReTryTime & UART_SCMR_RETRY_MASK);

	SC_WRITE(u8SCID, UART_SCMR, u8RegData);
}

void HAL_SC_ClearCardDetectInt(MS_U8 u8SCID, HAL_SC_CD_INT_CLEAR eIntClear)
{
	switch (eIntClear) {
	case E_HAL_SC_CD_IN_CLEAR:
		SC_WRITE(u8SCID, UART_SCSR, (SC_READ(u8SCID, UART_SCSR) | UART_SCSR_INT_CARDIN));
		break;

	case E_HAL_SC_CD_OUT_CLEAR:
		SC_WRITE(u8SCID, UART_SCSR, (SC_READ(u8SCID, UART_SCSR) | UART_SCSR_INT_CARDOUT));
		break;

	case E_HAL_SC_CD_ALL_CLEAR:
		SC_WRITE(u8SCID, UART_SCSR,
			 (SC_READ(u8SCID, UART_SCSR) | UART_SCSR_INT_CARDOUT |
			  UART_SCSR_INT_CARDIN));
		break;

	default:
		pr_info("[SC] unknown case[%s][%d]\n", __func__, __LINE__);
		break;
	}
}

void HAL_SC_GetCardDetectInt(MS_U8 u8SCID, HAL_SC_CD_INT_STATUS *pstCardDetectInt)
{
	MS_U8 u8RegData = 0x00;

	u8RegData = SC_READ(u8SCID, UART_SCSR);
	memset(pstCardDetectInt, 0x00, sizeof(HAL_SC_CD_INT_STATUS));
	if (u8RegData & UART_SCSR_INT_CARDIN)
		pstCardDetectInt->bCardInInt = TRUE;
	if (u8RegData & UART_SCSR_INT_CARDOUT)
		pstCardDetectInt->bCardOutInt = TRUE;
}

void HAL_SC_ResetPadCtrl(MS_U8 u8SCID, HAL_SC_LEVEL_CTRL eLogicalLevel)
{
	if (eLogicalLevel == E_HAL_SC_LEVEL_HIGH) {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, eLogicalLevel);
		SC_OR(u8SCID, UART_SCCR, UART_SCCR_RST);
	} else {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, eLogicalLevel);
		SC_AND(u8SCID, UART_SCCR, ~UART_SCCR_RST);
	}
}

void HAL_SC_SmcVccPadCtrl(MS_U8 u8SCID, HAL_SC_LEVEL_CTRL eLogicalLevel)
{
	if (eLogicalLevel == E_HAL_SC_LEVEL_HIGH) {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, eLogicalLevel);
		SC_OR(u8SCID, UART_SCFR, UART_SCFR_V_HIGH);
	} else {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, eLogicalLevel);
		SC_AND(u8SCID, UART_SCFR, ~UART_SCFR_V_HIGH);
	}
}

void HAL_SC_InputOutputPadCtrl(MS_U8 u8SCID, HAL_SC_IO_CTRL eCtrl)
{
	if (eCtrl == E_HAL_SC_IO_FORCED_LOW) {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, eCtrl);
		SC_OR(u8SCID, UART_LCR, UART_LCR_SBC);
	} else {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, eCtrl);
		SC_AND(u8SCID, UART_LCR, ~UART_LCR_SBC);
	}
}

void HAL_SC_Ext8024ActiveSeq(MS_U8 u8SCID)
{
	if (hal_vcc_high_active) {
		//for sony
		SC_DBG(E_SC_DBGLV_INFO, "%s CMDVCC pin high\n", __func__);
		// Pull 8024 CMDVCC pin high
		SC_OR(u8SCID, UART_SCFR, UART_SCFR_V_HIGH);
		SC_AND(u8SCID, UART_SCFR, ~(UART_SCFR_V_ENABLE));
	} else {
		//for 8024
		SC_DBG(E_SC_DBGLV_INFO, "%s 8024 CMDVCC pin low\n", __func__);
		// Pull 8024 CMDVCC pin low
		SC_AND(u8SCID, UART_SCFR, ~(UART_SCFR_V_ENABLE | UART_SCFR_V_HIGH));
	}
}

void HAL_SC_Ext8024ActiveSeqTSIO(MS_U8 u8SCID)
{
	pr_info("[%s][%d] not support\n", __func__, __LINE__);
}

void HAL_SC_Ext8024DeactiveSeq(MS_U8 u8SCID)
{
	if (!hal_vcc_high_active) {
		//for 8024
		SC_DBG(E_SC_DBGLV_INFO, "%s 8024 CMDVCC pin high\n", __func__);
		// Pull 8024 CMDVCC pin high
		SC_OR(u8SCID, UART_SCFR, UART_SCFR_V_HIGH);
		SC_AND(u8SCID, UART_SCFR, ~(UART_SCFR_V_ENABLE));
	} else {
		//for sony
		SC_DBG(E_SC_DBGLV_INFO, "%s CMDVCC pin low\n", __func__);
		// Pull 8024 CMDVCC pin low
		SC_AND(u8SCID, UART_SCFR, ~(UART_SCFR_V_ENABLE | UART_SCFR_V_HIGH));
	}
}

void HAL_SC_Ext8024DeactiveSeqTSIO(MS_U8 u8SCID)
{
	pr_info("[%s][%d] not support\n", __func__, __LINE__);
}

void HAL_SC_Int8024ActiveSeq(MS_U8 u8SCID)
{
	// Enable power cut auto deactive function
	HAL_SC_PwrCutDeactiveCtrl(u8SCID, TRUE);

	//////////////////////////////////////////////
	// We have 2 limitation due to analog design
	// 1. IO/CLK/RST has pulse while vcc is truned on due to active sequence enable
	// 2. There is CLK signal found due to active sequence enable if clk is already enabled.
	//     CLK seems not fully be restrainted by PD_SMC_PAD
	//
	// So active sequence will be:
	//  (a) Set PD_SMC_PAD=1 (to avoid following IO/CLK/RST pulse output to smc pads)
	//  (b) Set ACTIVE_SEQ_EN=1
	//  (c) Delay few time (to wait vcc ready and IO/CLK/RST pulse disappeared)
	//  (d) Enable smc clk and set PD_SMC_PAD=0 to power on smc pad
	//     (do not add any time delay between smc clk and PD_SMC_PAD for system busy issue)
	//  (e) Wait some time to complete active sequence and set ACTIVE_SEQ_EN=0
	//
	SC_WRITE(u8SCID, UART_CTRL7, SC_READ(u8SCID, UART_CTRL7) | UART_CTRL7_ACT_PULLUP_EN);
	SC_WRITE(u8SCID, UART_ACTIVE_VCC, SC_ACTIVE_VCC_CNT);
	SC_WRITE(u8SCID, UART_ACTIVE_IO, SC_ACTIVE_IO_CNT);
	SC_WRITE(u8SCID, UART_ACTIVE_CLK, SC_ACTIVE_CLK_CNT);
	SC_WRITE(u8SCID, UART_ACTIVE_RST, SC_ACTIVE_RST_CNT);
	SC_WRITE(u8SCID, UART_CTRL5, SC_READ(u8SCID, UART_CTRL5) | UART_CTRL5_ACTIVE_SEQ_EN);
	MsOS_DelayTask(1);
	HAL_SC_SmcClkCtrl(u8SCID, TRUE);
	SC_WRITE(u8SCID, UART_CTRL7, SC_READ(u8SCID, UART_CTRL7) & (~UART_CTRL7_PD_SMC_PAD));
	MsOS_DelayTask(5);	// ori 5
	SC_WRITE(u8SCID, UART_CTRL5, SC_READ(u8SCID, UART_CTRL5) & (~UART_CTRL5_ACTIVE_SEQ_EN));
}

void HAL_SC_Int8024ActiveSeqTSIO(MS_U8 u8SCID)
{
	pr_info("[%s][%d] not support\n", __func__, __LINE__);
}

void HAL_SC_Int8024DeactiveSeq(MS_U8 u8SCID)
{
	SC_WRITE(u8SCID, UART_DEACTIVE_RST, SC_DEACTIVE_RST_CNT);
	SC_WRITE(u8SCID, UART_DEACTIVE_CLK, SC_DEACTIVE_CLK_CNT);
	SC_WRITE(u8SCID, UART_DEACTIVE_IO, SC_DEACTIVE_IO_CNT);
	SC_WRITE(u8SCID, UART_DEACTIVE_VCC, SC_DEACTIVE_VCC_CNT);
	SC_WRITE(u8SCID, UART_CTRL3, SC_READ(u8SCID, UART_CTRL3) | UART_DEACTIVE_SEQ_EN);
	MsOS_DelayTask(5);
	SC_WRITE(u8SCID, UART_CTRL3, SC_READ(u8SCID, UART_CTRL3) & (~UART_DEACTIVE_SEQ_EN));
	SC_WRITE(u8SCID, UART_CTRL7, SC_READ(u8SCID, UART_CTRL7) | UART_CTRL7_PD_SMC_PAD);

	// Set all interface off
	SC_WRITE(u8SCID, UART_CTRL5, SC_READ(u8SCID, UART_CTRL5) | UART_CTRL5_PAD_MASK);
	SC_WRITE(u8SCID, UART_CTRL5, SC_READ(u8SCID, UART_CTRL5) & (~UART_CTRL5_PAD_MASK));

	//Disable clk
	HAL_SC_SmcClkCtrl(u8SCID, FALSE);
}

void HAL_SC_Int8024PullResetPadLow(MS_U8 u8SCID, MS_U32 u32HoldTimeInMs)
{
	HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
	MsOS_DelayTask(u32HoldTimeInMs);
	SC_WRITE(u8SCID, UART_CTRL5, SC_READ(u8SCID, UART_CTRL5) | UART_CTRL5_PAD_RELEASE);
	SC_WRITE(u8SCID, UART_CTRL5, SC_READ(u8SCID, UART_CTRL5) & (~UART_CTRL5_PAD_RELEASE));
}

void HAL_SC_SmcClkCtrl(MS_U8 u8SCID, MS_BOOL bEnableClk)
{
	if (u8SCID == 0) {
		if (bEnableClk) {
			//HW_WRITE(REG_TOP_CKG_SM_CA,
			// HW_READ(REG_TOP_CKG_SM_CA) & ~TOP_CKG_SM_CA0_DIS);
			SC_AND(u8SCID, REG_TOP_CKG_SM_CA, ~TOP_CKG_SM_CA0_DIS);
			pr_info("[%s][%d] Enable CLK\n", __func__, __LINE__);
		} else {
			//HW_WRITE(REG_TOP_CKG_SM_CA,
			// (HW_READ(REG_TOP_CKG_SM_CA) | TOP_CKG_SM_CA0_DIS));
			SC_OR(u8SCID, REG_TOP_CKG_SM_CA, TOP_CKG_SM_CA0_DIS);
			pr_info("[%s][%d] Disable CLK\n", __func__, __LINE__);
		}
	}
#if (SC_DEV_NUM > 1)		// no more than 2
	if (u8SCID == 1) {
		if (bEnableClk) {
			HW_WRITE(REG_TOP_CKG_SM_CA,
			HW_READ(REG_TOP_CKG_SM_CA) & ~TOP_CKG_SM_CA1_DIS);
		} else {
			HW_WRITE(REG_TOP_CKG_SM_CA,
			(HW_READ(REG_TOP_CKG_SM_CA) | TOP_CKG_SM_CA1_DIS));
		}
	}
#endif
}

void HAL_SC_PwrCutDeactiveCfg(MS_U8 u8SCID, MS_BOOL bVccOffPolHigh)
{
	SC_WRITE(u8SCID, UART_DEACTIVE_RST, SC_DEACTIVE_RST_CNT);
	SC_WRITE(u8SCID, UART_DEACTIVE_CLK, SC_DEACTIVE_CLK_CNT);
	SC_WRITE(u8SCID, UART_DEACTIVE_IO, SC_DEACTIVE_IO_CNT);
	SC_WRITE(u8SCID, UART_DEACTIVE_VCC, SC_DEACTIVE_VCC_CNT);

	if (bVccOffPolHigh) {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bVccOffPolHigh);
		SC_OR(u8SCID, UART_CTRL3, (UART_AC_PWR_OFF_CTL | UART_VCC_OFF_POL));
	} else {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bVccOffPolHigh);
		SC_OR(u8SCID, UART_CTRL3, UART_AC_PWR_OFF_CTL);
	}
}

void HAL_SC_PwrCutDeactiveCtrl(MS_U8 u8SCID, MS_BOOL bEnable)
{
	if (bEnable) {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bEnable);
		SC_AND(u8SCID, UART_CTRL3, ~(UART_AC_PWR_OFF_MASK));
	} else {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bEnable);
		SC_OR(u8SCID, UART_CTRL3, UART_AC_PWR_OFF_MASK);
	}
}

void HAL_SC_RxFailAlwaysDetCWT(MS_U8 u8SCID, MS_BOOL bEnable)
{
	if (bEnable) {
		//pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bEnable);
		SC_OR(u8SCID, UART_TX_FIFO_COUNT, UART_TX_FIFO_CWT_RX_FAIL_DET_EN);
	} else {
		//pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bEnable);
		SC_AND(u8SCID, UART_TX_FIFO_COUNT, ~(UART_TX_FIFO_CWT_RX_FAIL_DET_EN));
	}
}

MS_BOOL HAL_SC_SetRstToIoTimeout(MS_U8 u8SCID, MS_U32 u32Timeout, MS_U32 u32CardClk)
{
	MS_U32 u32Num;
	MS_U32 u32Clk;

	u32Clk = _HAL_SC_GetTopClkNum(u8SCID);
	if (u32Clk == 0 || u32CardClk == 0)
		return FALSE;

	u32Num = (MS_U32) (u32Clk / u32CardClk);
	u32Num = (u32Num * u32Timeout) / 16UL;


	SC_WRITE(u8SCID, UART_RST_TO_IO_0, (MS_U8) (u32Num & 0xFF));
	SC_WRITE(u8SCID, UART_RST_TO_IO_1, (MS_U8) ((u32Num >> 8) & 0xFF));
	SC_WRITE(u8SCID, UART_RST_TO_IO_2, (MS_U8) ((u32Num >> 16) & 0xFF));
	SC_WRITE(u8SCID, UART_RST_TO_IO_3, (MS_U8) ((u32Num >> 24) & 0xFF));

	return TRUE;
}

void HAL_SC_ClearRstToIoTimeout(MS_U8 u8SCID)
{

	SC_WRITE(u8SCID, UART_RST_TO_IO_0, (MS_U8) 0xFF);
	SC_WRITE(u8SCID, UART_RST_TO_IO_1, (MS_U8) 0xFF);
	SC_WRITE(u8SCID, UART_RST_TO_IO_2, (MS_U8) 0xFF);
	SC_WRITE(u8SCID, UART_RST_TO_IO_3, (MS_U8) 0xFF);
}

void HAL_SC_RstToIoEdgeDetCtrl(MS_U8 u8SCID, MS_BOOL bEnable)
{
	if (bEnable) {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bEnable);
		SC_OR(u8SCID, UART_SCFC, UART_SCFC_RST_TO_IO_EDGE_DET_EN);
	} else {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bEnable);
		SC_AND(u8SCID, UART_SCFC, ~(UART_SCFC_RST_TO_IO_EDGE_DET_EN));
	}
}

MS_BOOL HAL_SC_CheckIntRstToIoEdgeFail(MS_U8 u8SCID)
{
	MS_U8 u8RegData;

	u8RegData = SC_READ(u8SCID, UART_SCFC);
	if (u8RegData & UART_SCFC_RST_TO_IO_EDGE_DET_FAIL)
		return TRUE;
	else
		return FALSE;
}

void HAL_SC_SetIntRstToIoEdgeFail(MS_U8 u8SCID, MS_BOOL bEnable)
{
	if (bEnable) {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bEnable);
		SC_AND(u8SCID, UART_CTRL15, ~(UART_CTRL15_RST_TO_IO_EDGE_DET_FAIL_MASK));
	} else {
		pr_info("[%s][%d] %d %d\n", __func__, __LINE__, u8SCID, bEnable);
		SC_OR(u8SCID, UART_CTRL15, UART_CTRL15_RST_TO_IO_EDGE_DET_FAIL_MASK);
	}
}

void HAL_SC_ClkGateCtrl(MS_BOOL bEnable)
{
	//Not support
}

MS_U8 HAL_SC_GetTxFifoCnt(MS_U8 u8SCID)
{
	MS_U8 u8Reg;

	u8Reg = SC_READ(u8SCID, UART_TX_FIFO_COUNT);

	return (u8Reg & 0x3F);
}

MS_U8 HAL_SC_GetTxLevel(MS_U8 u8SCID)
{
	MS_U8 u8Reg;
	MS_U8 u8TxLevel = 0x00;

	u8Reg = SC_READ(u8SCID, UART_CTRL2);
	u8Reg = u8Reg & UART_CTRL2_TX_LEVEL;

	switch (u8Reg) {
	case UART_CTRL2_TX_LEVEL_5_TO_4:
		u8TxLevel = 5;
		break;

	case UART_CTRL2_TX_LEVEL_9_TO_8:
	default:
		u8TxLevel = 9;
		break;

	case UART_CTRL2_TX_LEVEL_17_TO_16:
		u8TxLevel = 17;
		break;

	case UART_CTRL2_TX_LEVEL_31_TO_30:
		u8TxLevel = 31;
		break;
	}

	return u8TxLevel;
}


void hal_sc_set_riu_base(void __iomem *RIU_vaddr)
{
	nonPM_vaddr = RIU_vaddr;
	pr_info("[%s][%d]nonPM_vaddr= %llx\n", __func__, __LINE__, nonPM_vaddr);
}

void hal_sc_set_extriu_base(void __iomem *extRIU_vaddr)
{
	ext_nonPM_vaddr = extRIU_vaddr;
	pr_info("[%s][%d]ext_nonPM_vaddr= %llx\n", __func__, __LINE__, ext_nonPM_vaddr);
}

void hal_sc_set_pad_mux(void)
{
	pr_info("[%s][%d]\n", __func__, __LINE__);
	reg16_write(ext_nonPM_vaddr, REG_SM_CONFIG, REG_SM_PAD_PCM);
	pr_info("[%s][%d]\n", __func__, __LINE__);
}

void HAL_SC_MaskIntRstToIoEdgeFail(MS_U8 u8SCID)
{
	pr_info("[%s][%d]\n", __func__, __LINE__);
	SC_WRITE(u8SCID, UART_CTRL15,
		 SC_READ(u8SCID, UART_CTRL15) | UART_CTRL15_RST_TO_IO_EDGE_DET_FAIL_MASK);
}

void hal_sc_set_clk_base(struct mdrv_sc_clk *pclk)
{
	smart_ck = pclk->smart_ck;
	pr_info("[%s][%d]smart_ck = %llx\n", __func__, __LINE__, smart_ck);
	smart_synth_432_ck = pclk->smart_synth_432_ck;
	pr_info("[%s][%d]smart_synth_432_ck = %llx\n", __func__, __LINE__, smart_synth_432_ck);
	smart_synth_27_ck = pclk->smart_synth_27_ck;
	pr_info("[%s][%d]smart_synth_27_ck = %llx\n", __func__, __LINE__, smart_synth_27_ck);
	smart2piu = pclk->smart2piu;
	pr_info("[%s][%d]smart2piu = %llx\n", __func__, __LINE__, smart2piu);
	smart_synth_272piu = pclk->smart_synth_272piu;
	pr_info("[%s][%d]smart_synth_272piu = %llx\n", __func__, __LINE__, smart_synth_272piu);
	smart_synth_4322piu = pclk->smart_synth_4322piu;
	pr_info("[%s][%d]smart_synth_4322piu = %llx\n", __func__, __LINE__, smart_synth_4322piu);
	mcu_nonpm2smart0 = pclk->mcu_nonpm2smart0;
	pr_info("[%s][%d]mcu_nonpm2smart0 = %llx\n", __func__, __LINE__, mcu_nonpm2smart0);
}

void hal_sc_set_clk_riu_base(void __iomem *clk_riu_vaddr)
{
	clk16_nonPM_vaddr = clk_riu_vaddr;
	pr_info("[%s][%d]clk16_nonPM_vaddr= %llx\n", __func__, __LINE__, clk16_nonPM_vaddr);
}

void HAL_SC_Set_ctrl2_rec_parity_err_en(MS_U8 u8SCID)
{
	pr_info("[%s][%d]\n", __func__, __LINE__);
	SC_OR(u8SCID, UART_CTRL2, UART_CTRL2_REC_PE_EN);
}

void hal_sc_set_vcc_init(int vcc_high_active)
{
	hal_vcc_high_active = vcc_high_active;
	pr_info("[%s][%d]hal_vcc_high_active = %d\n", __func__, __LINE__, hal_vcc_high_active);
}

void HAL_SC_Set_mcr_loop_en(MS_U8 u8SCID)
{
	pr_info("[%s][%d]\n", __func__, __LINE__);
	SC_OR(u8SCID, UART_MCR, UART_MCR_LOOP);
}

void hal_sc_set_rf_pop_delay(MS_U8 u8SCID, int rf_pop_delay)
{
	pr_info("[%s][%d]\n", __func__, __LINE__);
	SC_AND(u8SCID, UART_SCFR, ~(UART_SCFR_DELAY_MASK));

	switch (rf_pop_delay) {
	case DELAY0:
		SC_OR(u8SCID, UART_SCFR, UART_SCFR_DELAY_NONE);
		pr_info("[%s][%d]delay 0\n", __func__, __LINE__);
		break;

	case DELAY1:
		SC_OR(u8SCID, UART_SCFR, UART_SCFR_DELAY_ONE);
		pr_info("[%s][%d]delay 1\n", __func__, __LINE__);
		break;

	case DELAY2:
		SC_OR(u8SCID, UART_SCFR, UART_SCFR_DELAY_TWO);
		pr_info("[%s][%d]delay 2\n", __func__, __LINE__);
		break;

	case DELAY3:
		SC_OR(u8SCID, UART_SCFR, UART_SCFR_DELAY_THREE);
		pr_info("[%s][%d]delay 3\n", __func__, __LINE__);
		break;

	}

}

