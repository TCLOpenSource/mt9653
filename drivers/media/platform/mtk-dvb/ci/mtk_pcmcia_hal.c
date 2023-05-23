// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _HAL_PCMCIA_C_
#define _HAL_PCMCIA_C_

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition

#include "mtk_pcmcia_drv.h"
#include "mtk_pcmcia_hal_diff.h"
#include "mtk_pcmcia_reg_diff.h"

#include "mtk_pcmcia_hal.h"

#include <linux/io.h>


#include "mtk_pcmcia_common.h"
#include "mtk_pcmcia_clock.h"
#define MLP_CI_CORE_ENTRY(fmt, arg...)	//pr_info("[mdbgin_CADD_CI_HAL]" fmt, ##arg)

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#if (!PCMCIA_MSPI_SUPPORTED)
// Address bus of RIU is 16 bits.
#define PCMCIA_READ_WORD(addr)         READ_WORD(PCMCIA_BASE_ADDRESS + ((addr)<<2))
#define PCMCIA_WRITE_WORD(addr, val)   WRITE_WORD((PCMCIA_BASE_ADDRESS + ((addr)<<2)), (val))
#endif

#if (PCMCIA_MSPI_SUPPORTED)
#define MSPI_SPEED_1M   1000000
#define MSPI_SPEED_54M  54000000
#define MSPI_CMD_SIZE_3 3
#define MSPI_CMD_SIZE_4 4
#endif

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
#if (!PCMCIA_MSPI_SUPPORTED)
static REG16 __iomem *nonPM_vaddr;
static REG16 __iomem *ext_nonPM_vaddr;
#if (PCMCIA_PE_CTRL_SUPPORTED)
unsigned short _u16PCM_PE[3] = { 0, 0, 0 };
#endif
#endif

extern struct clk *ci_clk;

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
#if (PCMCIA_MSPI_SUPPORTED)
unsigned char u8MSPICmd[32];
#endif
//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
#if (PCMCIA_MSPI_SUPPORTED)
// Put this function here because hwreg_utility2 only for hal.
unsigned char u8lestHightByte;
static void _MSPI_Set_Bank_base(unsigned int u32Bank)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	if (u8lestHightByte != ((u32Bank >> 16) & 0xFF)) {
		u8MSPICmd[0] = MSPI_CMD_CFG_W;
		u8MSPICmd[1] = 0x02;
		u8MSPICmd[2] = (u32Bank >> 16) & 0xFF;	// bank = 0x10xxxx
		u8lestHightByte = (u32Bank >> 16) & 0xFF;
		mtk_pcmcia_to_spi_write(u8MSPICmd, MSPI_CMD_SIZE_3);
	}
}
static unsigned char _MDrv_PCMCIA_Check_MSPI(void)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	//MS_U32 i;
	unsigned char u16data;
	u8lestHightByte = 0x0;
	//for( i = 0; i<100000; i++)
	//{
		//LOG(0,"check MSPI %d\n", i);
		u16data = HAL_PCMCIA_Read_Byte(0x101ECC);
		if (u16data != 0xd3) {
			pr_err("\033[0;33m error 0x101ECC=0x%x	(golden=0xd3)\n\033[m", u16data);
			//panic("%s %d\n", __func__, __LINE__);
			//while(1);
		}

		HAL_PCMCIA_Write_Byte(0x101EF2, 0x55);
		u16data = HAL_PCMCIA_Read_Byte(0x101EF2);
		if (u16data != 0x55) {
			pr_err("\033[0;33m error 0x101EF2=0x%x	(golden=0x55)\n\033[m", u16data);
			//panic("%s %d\n", __func__, __LINE__);
			//while(1);
		}

		HAL_PCMCIA_Write_Byte(0x101EF3, 0xaa);
		u16data = HAL_PCMCIA_Read_Byte(0x101EF3);
		if (u16data != 0xaa) {
			pr_err("\033[0;33m error 0x101EF3=0x%x	(golden=0xaa)\n\033[m", u16data);
			//panic("%s %d\n", __func__, __LINE__);
			//while(1);
		}

		HAL_PCMCIA_Write_Byte(0x101EF2, 0x12);
		u16data = HAL_PCMCIA_Read_Byte(0x101EF2);
		if (u16data != 0x12) {
			pr_err("\033[0;33m error 0x101EF2=0x%x	(golden=012)\n\033[m", u16data);
			//panic("%s %d\n", __func__, __LINE__);
			//while(1);
		}

		HAL_PCMCIA_Write_Byte(0x101EF3, 0x34);
		u16data = HAL_PCMCIA_Read_Byte(0x101EF3);
		if (u16data != 0x34) {
			pr_err("\033[0;33m error 0x101EF3=0x%x	(golden=0x34)\n\033[m", u16data);
			//panic("%s %d\n", __func__, __LINE__);
			//while(1);
		}
	//}
	pr_info("[%s][%d] PASS\n", __func__, __LINE__);
	return TRUE;
}
#endif
#if (!PCMCIA_MSPI_SUPPORTED)
void HAL_PCMCIA_Set_RIU_base(void __iomem *RIU_vaddr)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	nonPM_vaddr = RIU_vaddr;
}

void HAL_Set_EXTRIU_base(void __iomem *extRIU_vaddr)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	ext_nonPM_vaddr = extRIU_vaddr;
}
#endif

void HAL_PCMCIA_Write_Byte(size_t ptrAddr, unsigned char u8Val)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
#if (PCMCIA_MSPI_SUPPORTED)
	_MSPI_Set_Bank_base(ptrAddr);
	u8MSPICmd[0] = MSPI_CMD_RIU_W1;
	u8MSPICmd[1] = ptrAddr & 0xff;
	u8MSPICmd[2] = (ptrAddr >> 8) & 0xff;
	u8MSPICmd[3] = u8Val;
	mtk_pcmcia_to_spi_write(u8MSPICmd, MSPI_CMD_SIZE_4);
#else

	unsigned short value;

	value = readw(nonPM_vaddr + ((ptrAddr >> 1)));
	//pr_info("[%s][%d]%x %04x %x\n",__FUNCTION__,__LINE__, ptrAddr, value, u8Val);
	if (ptrAddr & 0x01) {
		//write high byte
		value &= 0x00FF;
		value |= (u8Val << 8);
		//pr_info("[%s][%d] HI %x %04x\n",__FUNCTION__,__LINE__, ptrAddr, value);
	} else {
		//write low byte
		value &= 0xFF00;
		value |= u8Val;
		//pr_info("[%s][%d] LO %x %04x\n",__FUNCTION__,__LINE__, ptrAddr, value);
	}
	writew(value, nonPM_vaddr + ((ptrAddr >> 1)));

#endif
}

void HAL_PCMCIA_Write_ByteMask(size_t ptrAddr, unsigned char u8Val, unsigned char u8Mask)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
#if (PCMCIA_MSPI_SUPPORTED)
	unsigned char u8reg = HAL_PCMCIA_Read_Byte(ptrAddr) & ~u8Mask;

	u8Val &= u8Mask;
	u8reg |= u8Val;
	HAL_PCMCIA_Write_Byte(ptrAddr, u8reg);
#endif
}

unsigned char HAL_PCMCIA_Read_Byte(unsigned int ptrAddr)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned char u8reg = 0;
#if (PCMCIA_MSPI_SUPPORTED)
	//pr_info("[%s][%d]ptrAddr=%x\n", __func__, __LINE__, ptrAddr);
	_MSPI_Set_Bank_base(ptrAddr);
	u8MSPICmd[0] = MSPI_CMD_RIU_RIT1;
	u8MSPICmd[1] = ptrAddr & 0xff;
	u8MSPICmd[2] = (ptrAddr >> 8) & 0xff;
	u8MSPICmd[3] = 0;	// turn around
	mtk_pcmcia_to_spi_read(u8MSPICmd, MSPI_CMD_SIZE_4, &u8reg, 1);

	return u8reg;
#else

	if (ptrAddr & 0x01)
		u8reg = (unsigned char) (readw(nonPM_vaddr + ((ptrAddr >> 1))) >> 8);
	else
		u8reg = (unsigned char) readw(nonPM_vaddr + ((ptrAddr >> 1)));

	//pr_info("[%s][%d]%x %x\n",__FUNCTION__,__LINE__, ptrAddr, u8reg);
	return u8reg;

#endif
}
#if PCMCIA_IRQ_ENABLE
unsigned char HAL_PCMCIA_GetIntStatus(ISR_STS *isr_status)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	unsigned char u8Tmp = 0;

	u8Tmp = HAL_PCMCIA_Read_Byte(REG_PCMCIA_STAT_INT_RAW_INT1);
	if (u8Tmp & 0x01)
		isr_status->bCardAInsert = TRUE;

	if (u8Tmp & 0x02)
		isr_status->bCardARemove = TRUE;

	if (u8Tmp & 0x04)
		isr_status->bCardAData = TRUE;

	if (u8Tmp & 0x08)
		isr_status->bCardBInsert = TRUE;

	if (u8Tmp & 0x10)
		isr_status->bCardBRemove = TRUE;

	if (u8Tmp & 0x20)
		isr_status->bCardBData = TRUE;



	return TRUE;
}

void HAL_PCMCIA_ClrInt(unsigned int bits)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	unsigned char u8Tmp = 0;

	u8Tmp = (0x1 << 0) |	// cardA insert
	    (0x1 << 1) |	// cardA remove
	    (0x1 << 2) |	// IRQ from cardA
	    (0x1 << 3) |	// cardB insert
	    (0x1 << 4) |	// cardB remove
	    (0x1 << 5) |	// IRQ from cardB
	    (0x1 << 6) |	// timeout from cardA
	    (0x1 << 7);		// timeout from cardB

	HAL_PCMCIA_Write_Byte(REG_PCMCIA_INT_MASK_CLEAR1, u8Tmp);
	u8Tmp = 0;
	HAL_PCMCIA_Write_Byte(REG_PCMCIA_INT_MASK_CLEAR1, u8Tmp);
}

void HAL_PCMCIA_MaskInt(unsigned int bits, unsigned char bMask)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	unsigned char u8Tmp = 0;
#if (PCMCIA_MSPI_SUPPORTED)
	unsigned char u8Tmp2 = 0;
#endif
	// Check to HWRD Mask Bit6,7
	if (bMask == TRUE) {
		u8Tmp = (0x1 << 0) |	// cardA insert
		    (0x1 << 1) |	// cardA remove
		    (0x1 << 2) |	// IRQ from cardA
		    (0x1 << 3) |	// cardB insert
		    (0x1 << 4) |	// cardB remove
		    (0x1 << 5) |	// IRQ from cardB
		    (0x1 << 6) |	// timeout from cardA
		    (0x1 << 7);	// timeout from cardB
#if (PCMCIA_MSPI_SUPPORTED)
		u8Tmp2 = IRQ_MASK;
#endif
	} else {
		// unmask
		u8Tmp |= (0x1 << 6) |	// timeout from cardA
		    (0x1 << 7);	// timeout from cardB
#if (PCMCIA_MSPI_SUPPORTED)
		u8Tmp2 = IRQ_UNMASK;
#endif
	}
	HAL_PCMCIA_Write_Byte(REG_PCMCIA_INT_MASK_CLEAR, u8Tmp);
#if (PCMCIA_MSPI_SUPPORTED)
	HAL_PCMCIA_Write_ByteMask(REG_HST0_IRQ_MASK_15_0, u8Tmp2,
				  REG_PCM_IRQ_BIT_MASK);
#endif
}
#endif
void HAL_PCMCIA_ClkCtrl(unsigned char bEnable)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check

	if (bEnable) {
		pr_info("[%s][%d]enable\n", __func__, __LINE__);
#if (PCMCIA_MSPI_SUPPORTED)
		//config MSPI to 1M
		mtk_pcmcia_set_spi(MSPI_SPEED_1M);
		_MDrv_PCMCIA_Check_MSPI(); //Check MSPI

		//_MSPI_Set_Bank_base(REG_COMPANION_ANA_MISC_BANK);
		// MPLL enable
		HAL_PCMCIA_Write_ByteMask(REG_MPLL_CLK_PD,
					  MPLL_PD_DISABLE_DISABLE,
					  MPLL_PD_DISABLE_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_MPLL_CLK_PD_CTRL,
					  MPLL_PD_CTRL_DISABLE,
					  MPLL_PD_CTRL_DISABLE_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_MPLL_CLK_PD_CTRL1,
					  MPLL_PD_CTRL_DISABLE,
					  MPLL_PD_CTRL_DISABLE_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_MPLL_CLK_PD_CTRL2,
					  MPLL_PD_CTRL_DISABLE,
					  MPLL_PD_CTRL_DISABLE_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_MPLL_CLK_PD_CTRL3,
					  MPLL_PD_CTRL_DISABLE,
					  MPLL_PD_CTRL_DISABLE_MASK);
		// moleskin driving
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_SSPI_DRV, PAD_SSPI_DO_2, PAD_SSPI_DO_MASK);
		//config SSPI to 54M (sync CAPY code)
		HAL_PCMCIA_Write_Byte(0x100b20, 0x01);
		HAL_PCMCIA_Write_Byte(0x100b21, 0x00);
		HAL_PCMCIA_Write_Byte(0x000f40, 0x80);
		HAL_PCMCIA_Write_Byte(0x100b2b, 0x20);
		//config MSPI to 54M
		mtk_pcmcia_set_spi(MSPI_SPEED_54M);
		_MDrv_PCMCIA_Check_MSPI(); //Check MSPI

		//_MSPI_Set_Bank_base(REG_COMPANION_TOP_BANK);
		// PCM1 GPIO0
		HAL_PCMCIA_Write_ByteMask(REG_PCM1_PWR_OEN, PCM1_PWR_OUT,
					  PCM1_PWR_OEN_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_PCM1_PWR_OUT, PCM1_PWR_OUT_LOW,
					  PCM1_PWR_OUT_MASK);

		if ((HAL_PCMCIA_Read_Byte(REG_PCM1_PWR_OUT) & PCM1_PWR_OUT_MASK)
		    != PCM1_PWR_OUT_LOW) {
			pr_info
			    ("\033[0;33mREG_PCM1_PWR_OUT config error 0x%x\n\033[m",
			     (unsigned int)
			     HAL_PCMCIA_Read_Byte(REG_PCM1_PWR_OUT));

		}
		// PADMUX
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_ALLPAD_IN,
					  ALL_PAD_IN_DISABLE, ALL_PAD_IN_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_PCM_EN, PCM_ENABLE,
					  PCM_ENABLE_MASK);
		HAL_PCMCIA_Write_Byte(REG_CHIPTOP_PCM_PE, PE_ENABLE);

		if (HAL_PCMCIA_Read_Byte(REG_CHIPTOP_PCM_PE) != PE_ENABLE) {
			pr_info
			    ("\033[0;33mREG_CHIPTOP_PCM_PE config error 0x%x\n\033[m",
			     (unsigned int)
			     HAL_PCMCIA_Read_Byte(REG_CHIPTOP_PCM_PE));

		}

		HAL_PCMCIA_Write_ByteMask(REG_HST0_IRQ_MASK_15_0, IRQ_UNMASK,
					  REG_PCM_IRQ_BIT_MASK);
		HAL_PCMCIA_Write_Byte(REG_CLKGEN0_PCM, CLK_ON);
		pr_info("cilink power ctrl(HAL_COMPANION_PowerCtrl)\n");
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_CILINK_EN,
			CILINK_TS_RXTX_EN, CILINK_TS_RXTX_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_CILINK_SYN,
			CILINK_SYN_2B_216M, CILINK_SYN_2B_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_CILINK_SYN,
			CILINK_SYN_S_216M, CILINK_SYN_S_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_2B,
			CKG_2B_RX_INIT_FROM_ATOP, CKG_2B_RX_INIT_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_TOCARD_C2,
			CKG_2B_RX_EXT_ENABLE, CKG_2B_RX_EXT_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_2B,
			CKG_2B_TX_FROM_MAIN, CKG_2B_TX_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_CILINK,
			CILINK_IF_36M, CILINK_IF_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_CILINK,
			CILINK_TRACING_216M, CILINK_TRACING_MASK);
		HAL_PCMCIA_Write_Byte(REG_ATOP_CISYN_PD,
			CISYN_PD_RST_DISABLE);
		HAL_PCMCIA_Write_ByteMask(REG_ATOP_CIPLL_DIV,
			CIPLL_OUTPUT_DIV_72M, CIPLL_OUTPUT_DIV_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_ATOP_CISYN_SLD,
			CISYN_SLD, CISYN_SLD_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_ATOP_CISYN_SLD,
			~CISYN_SLD, CISYN_SLD_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_ATOP_CIPLL_PD,
			CIPLL_PD_POWERUP, CIPLL_PD_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_ATOP_CIPLL_PD_KP1,
			CIPLL_PD_KP1_POWERUP, CIPLL_PD_KP1_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_TS_SAMPLE,
			CKG_TS_SAMPLE_216M, CKG_TS_SAMPLE_MASK);
		pr_info("HAL_COMPANION_CardEnable\n");
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_TOCARD_C1,
			CKG_TS_TX_C1_FROM_SOC, CKG_TOCARD_C1_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_TOCARD_C2,
			CKG_TS_TX_C2_FROM_SOC, CKG_TOCARD_C2_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_FROMCARD,
			CKG_FROMCARD_C1_TS_SAMPLE_TOP, CKG_FROMCARD_C1_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_FROMCARD,
			CKG_FROMCARD_C2_TS_SAMPLE_TOP, CKG_FROMCARD_C2_MASK);
		pr_info("to card ts invert.......\n");
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_TOCARD_C1_EXT,
			(CKG_TOCARD_C1_EXT_ENABLE | CKG_TOCARD_C1_EXT_INVERT),
			CKG_TOCARD_C1_EXT_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_TOCARD_C2_EXT,
			CKG_TOCARD_C2_EXT_ENABLE, CKG_TOCARD_C2_EXT_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CILINK_CMP_IF_06,
			0., INTERLOOP_RX_2B_MASK);
		pr_info("clk divide to 72MHz : 14.4MHz (low speed)\n");
		HAL_PCMCIA_Write_ByteMask(REG_CLKGEN0_TOCARD_C1_EXT,
			CKG_TOCARD_C1_DIVIN_72M, CKG_TOCARD_C1_DIVIN_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_ATOP_CIPLL_DIV,
			CIPLL_OUTPUT_DIV_72M, CIPLL_OUTPUT_DIV_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CILINK_C1_DIV_N_L,
			0x5, REG_CILINK_C1_DIV_N_L_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CILINK_C1_DIV_M,
			0x1, REG_CILINK_C1_DIV_M_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CILINK_C1_DIV_N_H,
			REG_CILINK_C1_DIV_LOAD, REG_CILINK_C1_DIV_LOAD_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CILINK_C1_DIV_N_H,
			0, REG_CILINK_C1_DIV_LOAD_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_TS0_TX_DRV,
			0x99, REG_CHIPTOP_TS0_TX_DRV_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_TS0_RX_DRV,
			0, REG_CHIPTOP_TS0_RX_DRV_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_TS1_TX_DRV,
			0x99, REG_CHIPTOP_TS1_TX_DRV_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_TS1_RX_DRV,
			0, REG_CHIPTOP_TS1_RX_DRV_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_CILINK_RX_PAD_DRV_LOW,
			0x20, REG_CHIPTOP_CILINK_RX_PAD_DRV_LOW_MASK);
		HAL_PCMCIA_Write_ByteMask(REG_CHIPTOP_CILINK_RX_PAD_DRV_HIGH,
			0x3C, REG_CHIPTOP_CILINK_RX_PAD_DRV_HIGH_MASK);
#else				//PCMCIA_MSPI_SUPPORTED
		if (clk_prepare_enable(ci_clk) < 0) {
			pr_info("[%s][%d] clk_pcm prepare & enable fail\n",
			       __func__, __LINE__);
		}
		pr_info("[%s][%d]enable\n", __func__, __LINE__);
#if (PCMCIA_PE_CTRL_SUPPORTED)
		_u16PCM_PE[0] = PCM_TOP_REG(REG_TOP_PCM_PE_0);
		_u16PCM_PE[1] = PCM_TOP_REG(REG_TOP_PCM_PE_1);
		_u16PCM_PE[2] = PCM_TOP_REG(REG_TOP_PCM_PE_2);
		PCM_TOP_REG(REG_TOP_PCM_PE_0) = REG_TOP_PCM_PE_0_MASK;	//0xFFFFUL
		PCM_TOP_REG(REG_TOP_PCM_PE_1) = REG_TOP_PCM_PE_1_MASK;	//0xFFFFUL
		PCM_TOP_REG(REG_TOP_PCM_PE_2) |= REG_TOP_PCM_PE_2_MASK;
#endif				//PCMCIA_PE_CTRL_SUPPORTED
#endif				//PCMCIA_MSPI_SUPPORTED
	} else {
		pr_info("[%s][%d]disable\n", __func__, __LINE__);
#if (PCMCIA_MSPI_SUPPORTED)

		HAL_PCMCIA_Write_Byte(REG_CLKGEN0_PCM, CLK_OFF);
#else				//PCMCIA_MSPI_SUPPORTED
#if (PCMCIA_PE_CTRL_SUPPORTED)
		PCM_TOP_REG(REG_TOP_PCM_PE_0) = _u16PCM_PE[0];
		PCM_TOP_REG(REG_TOP_PCM_PE_1) = _u16PCM_PE[1];
		PCM_TOP_REG(REG_TOP_PCM_PE_2) = _u16PCM_PE[2];
#endif				//PCMCIA_PE_CTRL_SUPPORTED
		clk_disable_unprepare(ci_clk);
		pr_info("[%s][%d]disable\n", __func__, __LINE__);
#endif				//PCMCIA_MSPI_SUPPORTED
	}

}

#endif
