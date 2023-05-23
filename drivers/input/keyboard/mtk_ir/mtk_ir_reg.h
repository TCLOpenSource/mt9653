/* SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author mtk <mediatek@mediatek.com>
 */
#ifndef _MHAL_IR_REG_H_
#define _MHAL_IR_REG_H_

//Define IR	 IRQ Vector
void __iomem *ir_map_membase;
unsigned int ir_irq_base;
unsigned int ir_rc_irq_base;

// Define IrDa registers
#if defined(CONFIG_ARM) || defined(CONFIG_MIPS)
#define REG_IR_BASE			(ir_map_membase + 0x100)
#elif defined(CONFIG_ARM64)
#define REG_IR_BASE			(ir_map_membase + 0x100)
#endif
///////////////////////////////////////////////////////////////////////////////
#define REG_IR_CTRL				(0x0000UL*4 + (REG_IR_BASE))
	#define IR_SEPR_EN				0x0200UL
	#define IR_TIMEOUT_CHK_EN		0x0100UL
	#define IR_INV					0x80UL
	#define IR_INT_MASK				0x40UL
	#define IR_RPCODE_EN			0x20UL
	#define IR_LG01H_CHK_EN			0x10UL
	#define IR_DCODE_PCHK_EN		0x08UL
	#define IR_CCODE_CHK_EN			0x04UL
	#define IR_LDCCHK_EN			0x02UL
	#define IR_EN					0x01UL
#define REG_IR_HDC_UPB			(0x0001UL * 4 + (REG_IR_BASE))
#define REG_IR_HDC_LOB			(0x0002UL * 4 + (REG_IR_BASE))
#define REG_IR_OFC_UPB			(0x0003UL * 4 + (REG_IR_BASE))
#define REG_IR_OFC_LOB			(0x0004UL * 4 + (REG_IR_BASE))
#define REG_IR_OFC_RP_UPB		(0x0005UL * 4 + (REG_IR_BASE))
#define REG_IR_OFC_RP_LOB		(0x0006UL * 4 + (REG_IR_BASE))
#define REG_IR_LG01H_UPB		(0x0007UL * 4 + (REG_IR_BASE))
#define REG_IR_LG01H_LOB		(0x0008UL * 4 + (REG_IR_BASE))
#define REG_IR_LG0_UPB			(0x0009UL * 4 + (REG_IR_BASE))
#define REG_IR_LG0_LOB			(0x000AUL * 4 + (REG_IR_BASE))
#define REG_IR_LG1_UPB			(0x000BUL * 4 + (REG_IR_BASE))
#define REG_IR_LG1_LOB			(0x000CUL * 4 + (REG_IR_BASE))
#define REG_IR_SEPR_UPB			(0x000DUL * 4 + (REG_IR_BASE))
#define REG_IR_SEPR_LOB			(0x000EUL * 4 + (REG_IR_BASE))
#define REG_IR_TIMEOUT_CYC_L	(0x000FUL * 4 + (REG_IR_BASE))
#define REG_IR_TIMEOUT_CYC_H_CODE_BYTE	(0x0010UL * 4 + (REG_IR_BASE))
#define IR_CCB_CB				0x9F00UL//ir_ccode_byte+ir_code_bit_num
#define REG_IR_SEPR_BIT_FIFO_CTRL		(0x0011UL * 4 + (REG_IR_BASE))
#define REG_IR_CCODE			(0x0012UL * 4 + (REG_IR_BASE))
#define REG_IR_GLHRM_NUM		(0x0013UL * 4 + (REG_IR_BASE))
#define REG_IR_CKDIV_NUM_KEY_DATA		(0x0014UL * 4 + (REG_IR_BASE))
#define REG_IR_SHOT_CNT_L		(0x0015UL * 4 + (REG_IR_BASE))
#define REG_IR_SHOT_CNT_H_FIFO_STATUS	(0x0016UL * 4 + (REG_IR_BASE))
#define IR_RPT_FLAG				0x0100UL
#define IR_FIFO_EMPTY			0x0200UL
#define REG_IR_FIFO_RD_PULSE	(0x0018UL * 4 + (REG_IR_BASE))
#define REG_IR_CCODE1			(0x0020UL * 4 + (REG_IR_BASE))
#define REG_IR_SHOT_CNT_NUML    (0x0024UL*4 + (REG_IR_BASE))
#define REG_IR_CCODE1_CHK_EN	(0x0021UL * 4 + (REG_IR_BASE))
	#define IR_CCODE1_CHK_EN	0x01UL
//for RC5 HW mode

#define REG_IR_RC_BASE			  ir_map_membase


#define REG_IR_RC_CTRL			(0x0000UL * 4 + (REG_IR_RC_BASE))
#define IR_RC_AUTOCONFIG		(1 << 5)
#define IR_RC_FIFO_CLEAR		(1 << 6)
#define IR_RC_FIFO_WFIRST		(1 << 7)
#define IR_RC_EN				(1 << 8)
#define IR_RC6_EN				(1 << 9)
#define IR_RC5EXT_EN			(1 << 10)
#define IR_RC_WKUP_EN			(1 << 11)
#define IR_RCIN_INV			(1 << 15)
#define REG_IR_RC_LONGPULSE_THR		(0x0001UL * 4 + (REG_IR_RC_BASE))
#define REG_IR_RC_LONGPULSE_MAR		(0x0002UL * 4 + (REG_IR_RC_BASE))
#define REG_IR_RC_CLK_INT_THR		(0x0003UL * 4 + (REG_IR_RC_BASE))
#define IR_RC6_ECO_EN				(1<<0)
#define REG_IR_RC_WD_TIMEOUT_CNT	(0x0004UL * 4 + (REG_IR_RC_BASE))
#define REG_IR_RC_COMP_KEY1_KEY2	(0x0005UL * 4 + (REG_IR_RC_BASE))
#define REG_IR_RC_KEY_COMMAND_ADD	(0x0006UL * 4 + (REG_IR_RC_BASE))
#define REG_IR_RC_KEY_MISC			(0x0007UL * 4 + (REG_IR_RC_BASE))
#define REG_IR_RC_KEY_FIFO_STATUS	(0x0008UL * 4 + (REG_IR_RC_BASE))
#define IR_RC_FIFO_EMPTY			(1 << 0)
#define IR_RC_TIMEOUT_FLAG			(1 << 1)
#define IR_RC_FIFO_FULL				(1 << 2)
#define REG_IR_RC_FIFO_RD_PULSE		(0x0009UL * 4 + (REG_IR_RC_BASE))
#define REG_IR_RC_CMP_RCKEY			(0x000AUL * 4 + (REG_IR_RC_BASE))
#define IR_RC_POWER_WAKEUP_EN	    (1 << 8)

#endif
