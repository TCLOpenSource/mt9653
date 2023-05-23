/* SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author mtk <mediatek@mediatek.com>
 */
#ifndef _MHAL_IRS_REG_H_
#define _MHAL_IRS_REG_H_


void __iomem *irs_map_membase;
void __iomem *clkgen_bank_membase;

//IRS_SHOT_CNT = actual_pulse_width*IRS_CLK_freq/(IRS_CKDIV_CNT +1)
//ex : IRS_CKDIV_CNT = 0xCE,IRS_CLK_freq =12MHz,actual_pulse_width =525,
// duration = 525 *1/(12000000/(0xCE+1))=525 * 17us = 8925us(input is 9000us)

#define XTAL_CLOCK_FREQ					12000000	/*12 MHz*/
#define IRS_CKDIV_CNT					0x00CE
#define SAMPLE_RATE					17/* Sample rate in us. */



#define REG_IRS_BASE					(irs_map_membase)

#define REG_IRS_INT_STS					(0x0040UL*4 + (REG_IRS_BASE))
#define REG_IRS_THRESHOLD_NUM				(0x0042UL*4 + (REG_IRS_BASE))
#define REG_IRS_PAD_IN					(0x0043UL*4 + (REG_IRS_BASE))
#define REG_IRS_SHOT_SEL				(0x0051UL*4 + (REG_IRS_BASE))
#define REG_IRS_CKDIV_CNT				(0x0054UL*4 + (REG_IRS_BASE))
#define REG_IRS_AUTO_CLK_GATE_EN			(0x007FUL*4 + (REG_IRS_BASE))
#define REG_IRS_SHOT_CNT_L				(0x0055UL*4 + (REG_IRS_BASE))
#define REG_IRS_SHOT_CNT_H				(0x0056UL*4 + (REG_IRS_BASE))
#define REG_IRS_FIFO_DATA_L				(0x0059UL*4 + (REG_IRS_BASE))
#define REG_IRS_FIFO_DATA_H				(0x005aUL*4 + (REG_IRS_BASE))
#define REG_IRS_FIFO_RD					(0x0060UL*4 + (REG_IRS_BASE))
#define REG_IRS_MCU_FORCE_WDATA				(0x0062UL*4 + (REG_IRS_BASE))
#define MAGIC_NUM					0x1234
#define IRS_FIFO_EMPTY					0x0200UL


#define BANK_BASE_CLKGEN_00 (clkgen_bank_membase)	//0x1020
#define BANK_BASE_CLKGEN_05 (clkgen_bank_membase + 0x0A00)//0x1025
	#define BANK_BASE_CLKGEN_05_84		(0x0042UL*4 + (BANK_BASE_CLKGEN_05))
#define BANK_BASE_CLKGEN_18 (clkgen_bank_membase + 0x3000)//0x1038
	#define BANK_BASE_CLKGEN_18_20		(0x0010UL*4 + (BANK_BASE_CLKGEN_18))
#define BANK_BASE_CLKGEN_1D (clkgen_bank_membase + 0x3a00)//0x103D
	#define BANK_BASE_CLKGEN_1D_DC		(0x006EUL*4 + (BANK_BASE_CLKGEN_1D))
	#define BANK_BASE_CLKGEN_1D_DE		(0x006FUL*4 + (BANK_BASE_CLKGEN_1D))
#endif
