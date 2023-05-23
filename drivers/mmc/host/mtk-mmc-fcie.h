/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#ifndef MTK_FCIE_H
#define MTK_FCIE_H

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/host.h>
#include <linux/scatterlist.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/random.h>
#include "linux/kern_levels.h"
#include <linux/clk.h>
#include <linux/types.h>

#define EMMC_CACHE_LINE	0x40 // [FIXME]

#define EMMC_PACK1	__attribute__((__packed__))
#define EMMC_ALIGN1	__aligned(EMMC_CACHE_LINE)

//=====================================================
// HW registers
//=====================================================
#define REG_OFFSET_SHIFT_BITS	2

#define REG_U16(Reg_Addr)	readw((void *)(Reg_Addr))
#define REG_ADDR(x, y)	((x)+((y) << REG_OFFSET_SHIFT_BITS))
#define REG(reg_addr)	readw((void *)(reg_addr))
#define REG_W(reg_addr, val)	writew((val), (void *)(reg_addr))
#define REG_SETBIT(reg_addr, val)\
	writew(readw((void *)(reg_addr))|\
		(val), (void *)(reg_addr))
#define REG_CLRBIT(reg_addr, val)\
	writew(readw((void *)(reg_addr))&\
	(~(val)), (void *)(reg_addr))
#define REG_W1C(reg_addr, val)\
	writew(readw((void *)(reg_addr))&\
	(val), (void *)(reg_addr))

//------------------------------------------------------------------

#define MIE_EVENT				0x00
#define MIE_INT_EN				0x01
#define MMA_PRI_REG				0x02
#define MIU_DMA_ADDR_15_0		0x03
#define MIU_DMA_ADDR_31_16		0x04
#define MIU_DMA_LEN_15_0		0x05
#define MIU_DMA_LEN_31_16		0x06
#define MIE_FUNC_CTL			0x07
#define JOB_BL_CNT				0x08
#define BLK_SIZE				0x09
#define CMD_RSP_SIZE			0x0a
#define SD_MODE					0x0b
#define SD_CTRL					0x0c
#define SD_STATUS				0x0d
#define BOOT_CONFIG				0x0e
#define DDR_MODE				0x0f
#define RESERVED_FOR_SW			0x10
#define SDIO_MOD				0x11
#define RSP_SHIFT_CNT			0x12
#define RX_SHIFT_CNT			0x13
#define ZDEC_CTL0				0x14
#define TEST_MODE				0x15
#define MMA_BANK_SIZE			0x16
#define WR_SBIT_TIMER			0x17
#define RD_SBIT_TIMER			0x18
#define CMD_FIFO				0x20
#define MIU_DMA_ADDR_47_32		0x29
#define CIFD_EVENT				0x30
#define CIFD_INT_EN				0x31
#define PWR_RD_MASK				0x34
#define PWR_SAVE_CTL			0x35
#define BIST					0x36
#define BOOT					0x37
#define EMMC_DEBUG_BUS0			0x38
#define EMMC_DEBUG_BUS1			0x39
#define RST						0x3f
#define NC_FUN_CTL				0x63

//------------------------------------------------------------------
/* FCIE_MIE_EVENT  0x00 */
/* FCIE_MIE_INT_EN 0x01 */
#define BIT_DMA_END			BIT(0)
#define BIT_CMD_END			BIT(1)
#define BIT_ERR_STS			BIT(2)
#define BIT_SDIO_INT		BIT(3)
#define BIT_BUSY_END_INT	BIT(4)
#define BIT_R2N_RDY_INT		BIT(5)
#define BIT_CARD_CHANGE		BIT(6)
#define BIT_CARD2_CHANGE	BIT(7)
#define BIT_INT_CMDTMO      BIT(8)
#define BIT_INT_DATATMO     BIT(9)

#define BIT_ALL_CARD_INT_EVENTS		(BIT_DMA_END|BIT_CMD_END|BIT_ERR_STS \
					|BIT_BUSY_END_INT|BIT_R2N_RDY_INT)

/* FCIE_MMA_PRI_REG 0x02 */
#define BIT_MIU_R_PRI		BIT(0)
#define BIT_MIU_W_PRI		BIT(1)
#define BIT_MIU_SELECT_MASK		(BIT(3)|BIT(2))
#define BIT_MIU1_SELECT		BIT(2)
#define BIT_MIU2_SELECT		BIT(3)
#define BIT_MIU3_SELECT		(BIT(3)|BIT(2))
#define BIT_MIU_BUS_TYPE_MASK	(BIT(4)|BIT(5))
#define BIT_MIU_BURST1		(~BIT_MIU_BUS_TYPE_MASK)
#define BIT_MIU_BURST2		(BIT(4))
#define BIT_MIU_BURST4		(BIT(5))
#define BIT_MIU_BURST8		(BIT(4)|BIT(5))

/* FCIE_MIE_FUNC_CTL 0x07 */
#define BIT_EMMC_EN			BIT(0)
#define BIT_SD_EN			BIT(1)
#define BIT_SDIO_MOD		BIT(2)
#define BIT_EMMC_ACTIVE		BIT(12)
#define BIT_KERN_NAND		(BIT_KERN_CHK_NAND_EMMC|BIT(13))
#define BIT_KERN_EMMC		(BIT_KERN_CHK_NAND_EMMC|BIT(14))
#define BIT_KERN_CHK_NAND_EMMC		BIT(15)
#define BIT_KERN_CHK_NAND_EMMC_MSK	(BIT(13)|BIT(14)|BIT(15))

/* FCIE_BLK_CNT 0x08 */
#define BIT_SD_JOB_BLK_CNT_MASK		(BIT(13)-1)

/* FCIE_CMD_RSP_SIZE 0x0a */
#define BIT_RSP_SIZE_MASK		(BIT(6)-1)
#define BIT_CMD_SIZE_MASK		(BIT(13)|BIT(12)|BIT(11)|\
							BIT(10)|BIT(9)|BIT(8))
#define BIT_CMD_SIZE_SHIFT		8

/* FCIE_SD_MODE 0x0b */
#define BIT_CLK_EN		BIT(0)
#define BIT_SD_CLK_EN	BIT_CLK_EN
#define BIT_SD_DATA_WIDTH_MASK	(BIT(2)|BIT(1))
#define BIT_SD_DATA_WIDTH_1		0
#define BIT_SD_DATA_WIDTH_4		BIT(1)
#define BIT_SD_DATA_WIDTH_8		BIT(2)
#define BIT_DATA_DEST		BIT(4) // 0: DMA mode, 1: R2N mode
#define BIT_SD_DATA_CIFD	BIT_DATA_DEST
#define BIT_DATA_SYNC		BIT(5)
#define BIT_SD_DMA_R_CLK_STOP	BIT(7)
#define BIT_DIS_WR_BUSY_CHK		BIT(8)
#define BIT_JOB_START_CLR_SEL	BIT(11)

#define BIT_SD_DEFAULT_MODE_REG		(BIT_CLK_EN)

/* FCIE_SD_CTRL 0x0c */
#define BIT_SD_RSPR2_EN		BIT(0)
#define BIT_SD_RSP_EN		BIT(1)
#define BIT_SD_CMD_EN		BIT(2)
#define BIT_SD_DTRX_EN		BIT(3)
#define BIT_SD_DAT_EN		BIT_SD_DTRX_EN
#define BIT_SD_DAT_DIR_W	BIT(4)
#define BIT_ADMA_EN			BIT(5)
#define BIT_JOB_START		BIT(6)
#define BIT_CHK_CMD			BIT(7)
#define BIT_BUSY_DET_ON		BIT(8)
#define BIT_ERR_DET_ON		BIT(9)
#define BIT_R3R4_RSP_EN		BIT(10)
#define BIT_SYNC_REG_FOR_CMDQ_EN	BIT(12)

/* FCIE_SD_STATUS 0x0d */
#define BIT_DAT_RD_CERR		BIT(0)
#define BIT_SD_R_CRC_ERR	BIT_DAT_RD_CERR
#define BIT_DAT_WR_CERR		BIT(1)
#define BIT_SD_W_FAIL		BIT_DAT_WR_CERR
#define BIT_DAT_WR_TOUT		BIT(2)
#define BIT_SD_W_CRC_ERR	BIT_DAT_WR_TOUT
#define BIT_CMD_NO_RSP		BIT(3)
#define BIT_SD_RSP_TIMEOUT	BIT_CMD_NO_RSP
#define BIT_CMD_RSP_CERR	BIT(4)
#define BIT_SD_RSP_CRC_ERR	BIT_CMD_RSP_CERR
#define BIT_DAT_RD_TOUT		BIT(5)
#define BIT_SD_CARD_BUSY	BIT(6)

#define BITS_ERROR			(BIT_SD_R_CRC_ERR|BIT_DAT_WR_CERR|\
					BIT_DAT_WR_TOUT|BIT_CMD_NO_RSP|\
					BIT_CMD_RSP_CERR|BIT_DAT_RD_TOUT)

#define BIT_SD_D0			BIT(8)
#define BIT_SD_FCIE_ERR_FLAGS	(BIT(6)-1)
#define BIT_SD_CARD_D0_ST	BIT(8)
#define BIT_SD_CARD_D1_ST	BIT(9)
#define BIT_SD_CARD_D2_ST	BIT(10)
#define BIT_SD_CARD_D3_ST	BIT(11)
#define BIT_SD_CARD_D4_ST	BIT(12)
#define BIT_SD_CARD_D5_ST	BIT(13)
#define BIT_SD_CARD_D6_ST	BIT(14)
#define BIT_SD_CARD_D7_ST	BIT(15)

/* FCIE_BOOT_CONFIG 0x0e */
#define BIT_EMMC_RSTZ		BIT(0)
#define BIT_EMMC_RSTZ_EN	BIT(1)
#define BIT_BOOT_MODE_EN	BIT(2)

/* FCIE_DDR_MODE 0x0f */
#define BIT_MACRO_MODE_MASK		(BIT(7)|BIT(8)|BIT(12)|\
						BIT(13)|BIT(14)|BIT(15))
#define BIT_8BIT_MACRO_EN		BIT(7)
#define BIT_DDR_EN				BIT(8)
#define BIT_CLK2_SEL			BIT(10)
#define BIT_32BIT_MACRO_EN		BIT(12)
#define BIT_PAD_IN_SEL_SD		BIT(13)
#define BIT_FALL_LATCH			BIT(14)
#define BIT_PAD_IN_MASK			BIT(15)

/* FCIE_TOGGLE_CNT 0x10 */
#define BITS_8_MACRO32_DDR52_TOGGLE_CNT   0x110
#define BITS_4_MACRO32_DDR52_TOGGLE_CNT   0x210

#define BITS_8_MACRO32_HS200_TOGGLE_CNT   0x210
#define BITS_4_MACRO32_HS200_TOGGLE_CNT   0x410

/* FCIE_SDIO_MOD 0x11 */
#define BIT_REG_SDIO_MOD_MASK		(BIT(1)|BIT(0))
#define BIT_SDIO_DET_ON				BIT(2)
#define BIT_SDIO_DET_INT_SRC		BIT(3)

/* FCIE_RSP_SHIFT_CNT 0x12 */
#define BIT_RSP_SHIFT_TUNE_MASK		(BIT(4) - 1)
#define BIT_RSP_SHIFT_SEL			BIT(4)

/* FCIE_RX_SHIFT_CNT 0x13 */
#define BIT_RSTOP_SHIFT_TUNE_MASK	(BIT(4) - 1)
#define BIT_RSTOP_SHIFT_SEL			BIT(4)
#define BIT_WRSTS_SHIFT_TUNE_MASK	(BIT(8)|BIT(9)|BIT(10)|BIT(11))
#define BIT_WRSTS_SHIFT_SEL			BIT(12)

/* FCIE_ZDEC_CTL0 0x14 */
#define BIT_ZDEC_EN					BIT(0)
#define BIT_SD2ZDEC_PTR_CLR			BIT(1)

/* FCIE_TEST_MODE 0x15 */
#define BIT_SDDR1					BIT(0)
#define BIT_TEST_MODE_MASK			(BIT(3)|BIT(2)|BIT(1))
#define BIT_TEST_MODE_SHIFT		    1
#define BIT_BIST_MODE				BIT(4)
#define BIT_CQE_DBUS_MODE			(BIT(3)|BIT(2)|BIT(1))

/* FCIE_WR_SBIT_TIMER 0x17 */
#define BIT_WR_SBIT_TIMER_MASK		(BIT(15)-1)
#define BIT_WR_SBIT_TIMER_EN		BIT(15)

/* FCIE_RD_SBIT_TIMER 0x18 */
#define BIT_RD_SBIT_TIMER_MASK		(BIT(15)-1)
#define BIT_RD_SBIT_TIMER_EN		BIT(15)


/* NC_CIFD_EVENT 0x30 */
#define BIT_WBUF_FULL				BIT(0)
#define BIT_WBUF_EMPTY_TRI			BIT(1)
#define BIT_RBUF_FULL_TRI			BIT(2)
#define BIT_RBUF_EMPTY				BIT(3)

/* NC_CIFD_INT_EN 0x31 */
#define BIT_WBUF_FULL_INT_EN		BIT(0)
#define BIT_RBUF_EMPTY_INT_EN		BIT(1)
#define BIT_F_WBUF_FULL_INT			BIT(2)
#define BIT_F_RBUF_EMPTY_INT		BIT(3)

/* FCIE_PWR_SAVE_CTL 0x35 */
#define BIT_POWER_SAVE_MODE			BIT(0)
#define BIT_SD_POWER_SAVE_RIU		BIT(1)
#define BIT_POWER_SAVE_MODE_INT_EN	BIT(2)
#define BIT_SD_POWER_SAVE_RST		BIT(3)
#define BIT_POWER_SAVE_INT_FORCE	BIT(4)
#define BIT_RIU_SAVE_EVENT			BIT(5)
#define BIT_RST_SAVE_EVENT			BIT(6)
#define BIT_BAT_SAVE_EVENT			BIT(7)
#define BIT_BAT_SD_POWER_SAVE_MASK	BIT(8)
#define BIT_RST_SD_POWER_SAVE_MASK	BIT(9)
#define BIT_POWER_SAVE_MODE_INT		BIT(15)

/* FCIE_BOOT 0x37 */
#define BIT_NAND_BOOT_EN			BIT(0)
#define BIT_BOOTSRAM_ACCESS_SEL		BIT(1)

/* FCIE_BOOT 0x38 */
#define BIT_DEBUG0_MODE_MSK			(BIT(3)|BIT(2)|BIT(1)|BIT(0))


/* FCIE_BOOT 0x39 */
#define BIT_DEBUG1_MODE_MSK			(BIT(11)|BIT(10)|BIT(9)|BIT(8))
#define BIT_DEBUG1_MODE_SET			(BIT(10)|BIT(8))


/* FCIE_RESET 0x3f */

#define BIT_FCIE_SOFT_RST_n			BIT(0)
#define BIT_RST_MIU_STS				BIT(1)
#define BIT_RST_MIE_STS				BIT(2)
#define BIT_RST_MCU_STS				BIT(3)
#define BIT_RST_ECC_STS				BIT(4)
#define BIT_RST_STS_MASK	(BIT_RST_MIU_STS|BIT_RST_MIE_STS|\
							BIT_RST_MCU_STS)
#define BIT_NC_DEB_SEL_SHIFT		12
#define BIT_NC_DEB_SEL_MASK	(BIT(15)|BIT(14)|BIT(13)|BIT(12))

#define BANK_SIZE  0x80
#define DUMP_REG_OFFSET 0x8
#define DUMP_REG0  0
#define DUMP_REG1  1
#define DUMP_REG2  2
#define DUMP_REG3  3
#define DUMP_REG4  4
#define DUMP_REG5  5
#define DUMP_REG6  6
#define DUMP_REG7  7


#define RSP_31_24_SHIFT 24
#define RSP23_16_SHIFT 16
#define RSP_15_8_SHIFT 8


#define RSPH_MASK 0xFF00
#define RSPL_MASK 0xFF

#define RSP0_INDEX 0
#define RSP1_INDEX 1
#define RSP2_INDEX 2
#define RSP3_INDEX 3


#define CMD_FIFO_INDEX_0 0
#define CMD_FIFO_INDEX_1 1
#define CMD_FIFO_INDEX_2 2
#define CMD_FIFO_INDEX_3 3
#define CMD_FIFO_INDEX_4 4
#define CMD_FIFO_INDEX_5 5
#define CMD_FIFO_INDEX_6 6
#define CMD_FIFO_INDEX_7 7
#define CMD_FIFO_SIZE    8



#define CMD_ARG_31_24_SHIFT 24
#define CMD_ARG_23_16_SHIFT 16
#define CMD_ARG_15_8_SHIFT 8

#define CMD_ARGH_MASK 0xFF00
#define CMD_ARGL_MASK 0xFF
#define CMD_ARG_OPCODE 0x40


//------------------------------------------------------------------
/*
 * Power Save FIFO Cmd*
 */
/* Battery lost class */
#define PWR_BAT_CLASS	(0x1 << 13)
/* Reset Class */
#define PWR_RST_CLASS	(0x1 << 12)

/* Command Type */
/* Write data */
#define PWR_CMD_WREG	(0x0 << 9)
/* Read and cmp data. If mismatch, HW retry */
#define PWR_CMD_RDCP	(0x1 << 9)
/* Wait idle, max. 128T */
#define PWR_CMD_WAIT	(0x2 << 9)
/* Wait interrupt */
#define PWR_CMD_WINT	(0x3 << 9)
/* Stop */
#define PWR_CMD_STOP	(0x7 << 9)

/* RIU Bank */
#define PWR_CMD_BK0		(0x0 << 7)
#define PWR_CMD_BK1		(0x1 << 7)
#define PWR_CMD_BK2		(0x2 << 7)
#define PWR_CMD_BK3		(0x3 << 7)

#define PWR_RIU_ADDR	(0x0 << 0)
//===========================================================
// FDE registser
//==========================================================

#define AES_KEY					0x00	//0x0~0xF
#define AES_IV					0x10	//0x10~0x17
#define AES_CTR					0x18	//0x18~0x1F
#define AES_BUSY				0x20
#define AES_CONTROL0			0x30
#define AES_CONTROL1			0x31
#define AES_XEX_HW_T_CALC_KICK	0x34
#define AES_SW_RESET			0x38
#define AES_XEX_HW_T_CALC_KEY	0x40	//0x40~0x4F
#define AES_DATA_UNIT0			0x50
#define AES_DATA_UNIT1			0x51
#define AES_FUN					0x60
#define AES_DBG_BUS				0x61

//FDE_AES_BUSY 0x20
#define BIT_AES_BUSY			BIT(0)
//FDE_AES_CONTROL0 0x30
#define BIT_ENC_OR_DEC			BIT(0)
#define ENABLE_ENC				0
#define ENABLE_DEC				1
#define BIT_MASK_AES_MODE		(BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6))
#define BIT_AES_MODE_SHIFT		2
#define ECB_MODE				0	//set 0
#define CBC_MODE				1
#define CTR_MODE				2
#define OFB_MODE				5
#define CFB_MODE				6
#define XTS_MODE				8

#define BIT_MASK_KEY_SIZE		(BIT(12)|BIT(13))
#define BIT_KEY_128BIT_SIZE		BIT_MASK_KEY_SIZE
#define BIT_KEY_192BIT_SIZE		BIT(12)
#define BIT_KEY_256BIT_SIZE		BIT(13)

//FDE_AES_XEX_HW_T_CALC_KICK  0x34
#define BIT_AES_XEX_HW_T_CALC_KICK	BIT(0)

//FDE_AES_SW_RESET  0x38
#define BIT_AES_SW_RESET		BIT(0)

//FDE_AES_FUN  0x60
#define BIT_AES_FUNC_EN			BIT(0)
#define BIT_AES_SWAP			BIT(1)
#define BIT_AES_AUTO_CONFIG		BIT(2)
#define BIT_AES_AUTO_CONFIG_CMDQ	BIT(3)
#define BIT_AES_SWITCH_ON			BIT(4)


//===========================================================
// eMMC pll registser
//==========================================================
/* emmcpll 0x03 */
#define reg_emmcpll_0x03               0x03
#define BIT_SKEW1_MASK                  (BIT(3)|BIT(2)|BIT(1)|BIT(0))
#define BIT_SKEW4_MASK                  (BIT(15)|BIT(14)|BIT(13)|BIT(12))
#define BIT_SKEW4_SHIFT                 12



/* emmcpll 0x09 */
#define reg_emmcpll_0x09               0x09
#define BIT_RXDLL_EN                    BIT(0)


/* emmcpll 0x14 */
#define reg_emmcpll_0x14               0x14

/* emmcpll 0x15 */
#define reg_emmcpll_0x15               0x15


/* emmcpll 0x1d */
#define reg_emmcpll_0x20               0x20
#define BIT_SEL_INTERNAL_MASK          (BIT(10)|BIT(9))
#define BIT_SEL_SKEW4_FOR_RXDLL        BIT(9)
#define BIT_SEL_SKEW4_FOR_CMD          BIT(10)

/* emmcpll 0x5f */
#define reg_emmcpll_0x5f               0x5f
#define BIT_FLASH_MACRO_TO_FICE        BIT(0)


/* emmcpll 0x63 */
#define reg_emmcpll_0x63               0x63
#define BIT_USE_RXDLL                    BIT(0)


/* emmcpll 0x69 */
#define reg_emmcpll_0x69               0x69
#define BIT_SKEW4_CLK_INV_MASK          (BIT(11)|BIT(10)|BIT(9)|BIT(8))
#define BIT_SKEW4_DATA_INV              BIT(9)
#define BIT_SKEW4_CMD_RSP_INV           BIT(10)
#define BIT_SKEW4_ALL_INV               (BIT(10)|BIT(9))

/* emmcpll 0x6c */
#define reg_emmcpll_0x6c               0x6c
#define reg_emmcpll_skew4_invert        BIT(7)


//===========================================================
// device status (R1, R1b)
//===========================================================
#define EMMC_R1_ADDRESS_OUT_OF_RANGE	BIT(31)
#define EMMC_R1_ADDRESS_MISALIGN		BIT(30)
#define EMMC_R1_BLOCK_LEN_ERROR			BIT(29)
#define EMMC_R1_ERASE_SEQ_ERROR			BIT(28)
#define EMMC_R1_ERASE_PARAM				BIT(27)
#define EMMC_R1_WP_VIOLATION			BIT(26)
#define EMMC_R1_DEVICE_IS_LOCKED		BIT(25)
#define EMMC_R1_LOCK_UNLOCK_FAILED		BIT(24)
#define EMMC_R1_COM_CRC_ERROR			BIT(23)
#define EMMC_R1_ILLEGAL_COMMAND			BIT(22)
#define EMMC_R1_DEVICE_ECC_FAILED		BIT(21)
#define EMMC_R1_CC_ERROR				BIT(20)
#define EMMC_R1_ERROR					BIT(19)
#define EMMC_R1_CID_CSD_OVERWRITE		BIT(16)
#define EMMC_R1_WP_ERASE_SKIP			BIT(15)
#define EMMC_R1_ERASE_RESET				BIT(13)
#define EMMC_R1_CURRENT_STATE			(BIT(12)|BIT(11)|BIT(10)|BIT(9))
#define EMMC_R1_READY_FOR_DATA			BIT(8)
#define EMMC_R1_SWITCH_ERROR			BIT(7)
#define EMMC_R1_EXCEPTION_EVENT			BIT(6)
#define EMMC_R1_APP_CMD					BIT(5)

#define EMMC_ERR_R1_31_24		(EMMC_R1_ADDRESS_OUT_OF_RANGE| \
						EMMC_R1_ADDRESS_MISALIGN| \
						EMMC_R1_BLOCK_LEN_ERROR| \
						EMMC_R1_ERASE_SEQ_ERROR| \
						EMMC_R1_ERASE_PARAM| \
						EMMC_R1_WP_VIOLATION| \
						EMMC_R1_LOCK_UNLOCK_FAILED)
#define EMMC_ERR_R1_23_16		(EMMC_R1_COM_CRC_ERROR| \
						EMMC_R1_ILLEGAL_COMMAND| \
						EMMC_R1_DEVICE_ECC_FAILED| \
						EMMC_R1_CC_ERROR| \
						EMMC_R1_ERROR| \
						EMMC_R1_CID_CSD_OVERWRITE)
#define EMMC_ERR_R1_15_8		(EMMC_R1_WP_ERASE_SKIP| \
						EMMC_R1_ERASE_RESET)
#define EMMC_ERR_R1_7_0			(EMMC_R1_SWITCH_ERROR)

#define EMMC_ERR_R1_31_0		(EMMC_ERR_R1_31_24|EMMC_ERR_R1_23_16|\
					EMMC_ERR_R1_15_8|EMMC_ERR_R1_7_0)
#define EMMC_ERR_R1_NEED_RETRY	(EMMC_R1_COM_CRC_ERROR|\
					EMMC_R1_DEVICE_ECC_FAILED|\
					EMMC_R1_CC_ERROR|\
					EMMC_R1_ERROR|EMMC_R1_SWITCH_ERROR)
//===========================================================
// driver error codes
//===========================================================
#define EMMC_ST_SUCCESS				0
#define EMMC_ST_PLAT				0x80000000

#define EMMC_ST_ERR_MEM_CORRUPT			(0x0001 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NOT_ALIGN			(0x0002 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NOT_PACKED			(0x0003 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_DATA_MISMATCH		(0x0004 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_TIMEOUT_WAIT_REG0	(0x0005 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_TIMEOUT_FIFOCLKRDY	(0x0006 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_TIMEOUT_MIULASTDONE	(0x0007 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_TIMEOUT_WAITD0HIGH	(0x0008 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_TIMEOUT_CARDDMAEND	(0x0009 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_TIMEOUT_WAITCIFDEVENT	(0x000a | EMMC_ST_PLAT)
#define EMMC_ST_ERR_FCIE_STS_ERR		(0x000b | EMMC_ST_PLAT)

#define EMMC_ST_ERR_BIST_FAIL			(0x0010 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_DEBUG_MODE			(0x0011 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_FCIE_NO_CLK			(0x0012 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_PARAMETER			(0x0013 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NOT_INIT			(0x0014 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_INVALID_PARAM		(0x0015 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_PARTITION_CHKSUM	(0x0016 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NO_PART_INFO		(0x0017 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NO_PARTITION		(0x0018 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NO_OK_DDR_PARAM		(0x0019 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_SAVE_DDRT_FAIL		(0x001A | EMMC_ST_PLAT)
#define EMMC_ST_ERR_DDRT_CHKSUM			(0x001B | EMMC_ST_PLAT)
#define EMMC_ST_ERR_DDRT_NONA			(0x001C | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CIS_NNI				(0x001D | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CIS_PNI				(0x001E | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CIS_NNI_NONA		(0x001F | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CIS_PNI_NONA		(0x0020 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_SDR_DETECT_DDR		(0x0021 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NO_CIS				(0x0022 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NOT_EMMC_PLATFORM	(0x0023 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_FCIE_NO_RIU			(0x0024 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_INT_TO				(0x0025 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_UNKNOWN_CLK			(0x0026 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_BUILD_DDRT			(0x0027 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NO_RSP_IN_RAM		(0x0028 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NO_SLOWER_CLK		(0x0029 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_PAD_ABNORMAL_OFF	(0x0030 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_SAVE_BLEN_FAIL		(0x0031 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_BLEN_CHKSUM			(0x0032 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CHKSUM				(0x0033 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_ERR_DET				(0x0034 | EMMC_ST_PLAT)
#define eMMC_ST_ERR_SKEW4_NOT_ENOUGH    (0x0035 | EMMC_ST_PLAT)

#define EMMC_ST_ERR_CMD1				(0x0a00 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD1_DEV_NOT_RDY	(0x0a01 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD2				(0x0a02 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD3_CMD7			(0x0a03 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_R1_31_24			(0x0a04 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_R1_23_16			(0x0a05 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_R1_15_8				(0x0a06 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_R1_7_0				(0x0a07 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD8_CIFD			(0x0a08 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD8_MIU			(0x0a09 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD17_CIFD			(0x0a0a | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD17_MIU			(0x0a0b | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD17_RESP_CRC		(0x0a0c | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD17_DATA_CRC		(0x0a0d | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD18				(0x0a0e | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD6				(0x0a0f | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD13				(0x0a10 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD12				(0x0a11 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD24_CIFD			(0x0a12 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD24_CIFD_WAIT_D0H	(0x0a13 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD24_CIFD_CHK_R1	(0x0a14 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD24_MIU			(0x0a15 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD24_MIU_WAIT_D0H	(0x0a16 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD24_MIU_CHK_R1	(0x0a17 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD25				(0x0a18 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD25_WAIT_D0H		(0x0a19 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD25_CHK_R1		(0x0a1a | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD9				(0x0a1b | EMMC_ST_PLAT)
#define EMMC_ST_ERR_SEC_UPFW_TO			(0x0a20 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD16				(0x0a21 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NO_HS200_1_8V		(0x0a22 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD21_ONE_BIT		(0x0a23 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD21_NO_RSP		(0x0a24 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD21_DATA_CRC		(0x0a25 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD21_NO_DRVING		(0x0a26 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD21_HS200_FAIL	(0x0a27 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD21_DATA_CMP		(0x0a28 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD21_NO_HS200_1_8V	(0x0a29 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_SET_DRV_STRENGTH	(0x0a2a | EMMC_ST_PLAT)
#define EMMC_ST_ERR_SKEW4				(0x0a2b | EMMC_ST_PLAT)
#define EMMC_ST_ERR_NOTABLE				(0x0a2c | EMMC_ST_PLAT)

#define EMMC_ST_ERR_CMD8_ECHO			(0x0b00 | EMMC_ST_PLAT)
#define	EMMC_ST_ERR_CMD8_NO_RSP			(0x0b01 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD55_NO_RSP		(0x0b02 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD55_RSP_CRC		(0x0b03 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD55_APP_CMD_BIT	(0x0b04 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_ACMD41_DEV_BUSY		(0x0b05 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_ACMD41_NO_RSP		(0x0b06 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD9_CSD_FMT		(0x0b07 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_ACMD6_NO_RSP		(0x0b08 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_ACMD6_WRONG_PARA	(0x0b09 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_ACMD13_NO_RSP		(0x0b0a | EMMC_ST_PLAT)
#define EMMC_ST_ERR_ACMD13_DAT_CRC		(0x0b0b | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD6_NO_RSP			(0x0b0c | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD6_DAT_CRC		(0x0b0d | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD6_WRONG_PARA		(0x0b0e | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD6_HS_NOT_SRPO	(0x0b0f | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD6_SWC_STS_ERR	(0x0b10 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD6_SWC_STS_CODE	(0x0b11 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD32_NO_RSP		(0x0b12 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD32_SEQ_ERR		(0x0b13 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD32_PARAM_ERR		(0x0b14 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD32_RESET			(0x0b15 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD33_NO_RSP		(0x0b16 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD33_SEQ_ERR		(0x0b17 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD33_PARAM_ERR		(0x0b18 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD33_RESET			(0x0b19 | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD38_NO_RSP		(0x0b1a | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD38_SEQ_ERR		(0x0b1b | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD38_PARAM_ERR		(0x0b1c | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD38_RESET			(0x0b1d | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD38_TO			(0x0b1e | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD3536_ERR			(0x0b1f | EMMC_ST_PLAT)
#define EMMC_ST_ERR_CMD38_ERR			(0x0b20 | EMMC_ST_PLAT)

#define BIT_SKEW1_MASK			(BIT(3)|BIT(2)|BIT(1)|BIT(0))
#define BIT_SKEW2_MASK			(BIT(7)|BIT(6)|BIT(5)|BIT(4))
#define BIT_SKEW3_MASK			(BIT(11)|BIT(10)|BIT(9)|BIT(8))
#define BIT_SKEW4_MASK			(BIT(15)|BIT(14)|BIT(13)|BIT(12))
#define BIT_DEFAULT_SKEW2		(BIT(6)|BIT(4))//5

#define BIT_DQS_DELAY_CELL_MASK		(BIT(4)|BIT(5)|BIT(6)|BIT(7))
#define BIT_DQS_DELAY_CELL_SHIFT	4
#define BIT_DQS_MODE_MASK			(BIT(0)|BIT(1)|BIT(2))
#define BIT_DQS_MODE_SHIFT			0
#define BIT_DQS_MODE_0T				(0 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_0_5T			(1 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_1T				(2 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_1_5T			(3 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_2T				(4 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_2_5T			(5 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_3T				(6 << BIT_DQS_MODE_SHIFT)
#define BIT_DQS_MODE_3_5T			(7 << BIT_DQS_MODE_SHIFT)

//clock
#define EMMC_PLL_FLAG					0x80
#define EMMC_PLL_CLK__20M				(0x01|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__27M				(0x02|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__32M				(0x03|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__36M				(0x04|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__40M				(0x05|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__48M				(0x06|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__52M				(0x07|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__62M				(0x08|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__72M				(0x09|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__80M				(0x0a|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK__86M				(0x0b|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK_100M				(0x0c|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK_120M				(0x0d|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK_140M				(0x0e|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK_160M				(0x0f|EMMC_PLL_FLAG)
#define EMMC_PLL_CLK_200M				(0x10|EMMC_PLL_FLAG)

#define CLK_SRC_CLKGEN					0
#define CLK_SRC_EMMC_PLL_4X_CLK			4
#define CLK_SRC_EMMC_PLL_2X_CLK			2
#define CLK_SRC_EMMC_PLL_1X_CLK			1

#define EMMC_FCIE_VALID_CLK_CNT			3// FIXME

#define PLL_SKEW4_CNT					9
#define MIN_OK_SKEW_CNT					7

#define EMMC_HW_TIMER_HZ				12000000
#define FCIE_EMMC_DISABLE				0
#define FCIE_EMMC_BYPASS				1
#define FCIE_EMMC_SDR					2
#define FCIE_EMMC_DDR_8BIT_MACRO		3
#define FCIE_EMMC_DDR					FCIE_EMMC_DDR_8BIT_MACRO
#define FCIE_EMMC_HS200					5
#define FCIE_EMMC_HS400_DS				6
#define FCIE_EMMC_HS400					FCIE_EMMC_HS400_DS
#define FCIE_EMMC_HS400_5_1				7
#define FCIE_EMMC_HS400_AIFO_5_1		FCIE_EMMC_HS400_5_1

#define EMMC_DRV_RESERVED_BLK_CNT		((0x200000-0x10000)/0x200)

#define EMMC_CIS_NNI_BLK_CNT			2
#define EMMC_CIS_PNI_BLK_CNT			2
#define EMMC_TEST_BLK_CNT				(0x100000/0x200) // 1MB

#define EMMC_CIS_BLK_0				(64*1024/512) // from 64KB
#define EMMC_NNI_BLK_0				(EMMC_CIS_BLK_0+0)
#define EMMC_NNI_BLK_1				(EMMC_CIS_BLK_0+1)
#define EMMC_PNI_BLK_0				(EMMC_CIS_BLK_0+2)
#define EMMC_PNI_BLK_1				(EMMC_CIS_BLK_0+3)
#define EMMC_DDRTABLE_BLK_0			(EMMC_CIS_BLK_0+4)
#define EMMC_DDRTABLE_BLK_1			(EMMC_CIS_BLK_0+5)
#define EMMC_HS200TABLE_BLK_0		(EMMC_CIS_BLK_0+6)
#define EMMC_HS200TABLE_BLK_1		(EMMC_CIS_BLK_0+7)
#define EMMC_HS400TABLE_BLK_0		(EMMC_CIS_BLK_0+8)
#define EMMC_HS400TABLE_BLK_1		(EMMC_CIS_BLK_0+9)
#define EMMC_HS400EXTTABLE_BLK_0	(EMMC_CIS_BLK_0+10)
#define EMMC_HS400EXTTABLE_BLK_1	(EMMC_CIS_BLK_0+11)
#define EMMC_CRAZY_PATTERN_BLK      (EMMC_CIS_BLK_0+12)


#define EMMC_CIS_BLK_END			EMMC_BURST_LEN_BLK_0
// last 1MB in reserved area, use for EMMC test
#define EMMC_TEST_BLK_0				(EMMC_CIS_BLK_END+1)

#define ENABLE_EMMC_ATOP			1 // DDR52 (ATOP)
#define ENABLE_EMMC_HS200			1 // HS200
#define ENABLE_EMMC_HS400			1// HS400
#define ENABLE_EMMC_HS400_5_1		1

#define DRIVER_NAME					"mtk_mci"

#ifndef EMMC_DEBUG_MSG
#define EMMC_DEBUG_MSG				1
#endif

/* Define trace levels. */
#define EMMC_DBG_LVL_MUTE			(-1)
/* Emerge condition debug messages. */
#define EMMC_DBG_LVL_EMERG			(0)	//dev_err
/* Alert condition debug messages. */
#define EMMC_DBG_LVL_ALERT			(1)	//dev_err
/* Critical condition debug messages. */
#define EMMC_DBG_LVL_CRIT			(2)	//dev_err
/* Error condition debug messages. */
#define EMMC_DBG_LVL_ERROR			(3)	//dev_err
/* Warning condition debug messages. */
#define EMMC_DBG_LVL_WARNING		(4)	//dev_warn
/* Debug messages (high debugging). */
#define EMMC_DBG_LVL_HIGH			(5)	//dev_notice
/* Debug messages. */
#define EMMC_DBG_LVL_MEDIUM			(6)	//dev_info
/* Debug messages (low debugging). */
#define EMMC_DBG_LVL_LOW			(7)	//dev_info

/* Higer debug level means more verbose */
#ifndef EMMC_DBG_LVL
#define EMMC_DBG_LVL				EMMC_DBG_LVL_WARNING
#endif

#if defined(EMMC_DEBUG_MSG) && EMMC_DEBUG_MSG
#define EMMC_PRF(fmt, arg...)	pr_crit(fmt, ##arg)

#define EMMC_DBG(host, dbg_lv, tag, str, ...)\
	do {\
		if (dbg_lv > host->log_level || host->log_level == EMMC_DBG_LVL_MUTE) {\
			break;\
		} else {\
			if (dbg_lv <= EMMC_DBG_LVL_ERROR) {\
				if (tag)\
					dev_crit(host->dev, "[ %s() Ln.%u ] ", \
					__func__, __LINE__);\
				dev_crit(host->dev, str, ##__VA_ARGS__); } \
			if (dbg_lv == EMMC_DBG_LVL_WARNING) {\
				if (tag)\
					dev_warn(host->dev, "[ %s() Ln.%u ] ", \
					__func__, __LINE__);\
				dev_warn(host->dev, str, ##__VA_ARGS__); } \
			if (dbg_lv == EMMC_DBG_LVL_HIGH) {\
				if (tag)\
					dev_notice(host->dev, "[ %s() Ln.%u ] ", \
					__func__, __LINE__);\
				dev_notice(host->dev, str, ##__VA_ARGS__); } \
			if (dbg_lv == EMMC_DBG_LVL_MEDIUM) {\
				if (tag)\
					dev_info(host->dev, "[ %s() Ln.%u ] ", \
					__func__, __LINE__);\
				dev_info(host->dev, str, ##__VA_ARGS__); } \
			if (dbg_lv == EMMC_DBG_LVL_LOW) {\
				if (tag)\
					dev_info(host->dev, "[ %s() Ln.%u ] ", \
					__func__, __LINE__);\
				dev_info(host->dev, str, ##__VA_ARGS__); } \
		} \
	} while (0)
#else
#define EMMC_PRF(...)
#define EMMC_DBG(dev, enable, tag, str, ...)
#endif

#define EMMC_DIE(str)\
	{ EMMC_PRF("eMMC Die: %s() Ln.%u, %s\n",\
			__func__, __LINE__, str); \
		panic("\n"); }

#define EMMC_STOP() \
	do {\
		while (1)\
			EMMC_reset_WatchDog;\
	} while (0)

//=====================================================
// unit for HW Timer delay (unit of us)
//=====================================================
#define HW_TIMER_DELAY_1US			1
#define HW_TIMER_DELAY_5US			5
#define HW_TIMER_DELAY_10US			10
#define HW_TIMER_DELAY_100US		100
#define HW_TIMER_DELAY_500US		500
#define HW_TIMER_DELAY_1MS			(1000*HW_TIMER_DELAY_1US)
#define HW_TIMER_DELAY_5MS			(5*HW_TIMER_DELAY_1MS)
#define HW_TIMER_DELAY_10MS			(10*HW_TIMER_DELAY_1MS)
#define HW_TIMER_DELAY_100MS		(100*HW_TIMER_DELAY_1MS)
#define HW_TIMER_DELAY_500MS		(500*HW_TIMER_DELAY_1MS)
#define HW_TIMER_DELAY_1s			(1000*HW_TIMER_DELAY_1MS)

#define TIME_WAIT_DAT0_HIGH				(HW_TIMER_DELAY_1s*10)
#define TIME_WAIT_ERASE_DAT0_HIGH		(HW_TIMER_DELAY_1s*10)
#define TIME_WAIT_FCIE_RESET			HW_TIMER_DELAY_500MS
#define TIME_WAIT_FCIE_RST_TOGGLE_CNT	HW_TIMER_DELAY_1US
#define TIME_WAIT_FIFOCLK_RDY			HW_TIMER_DELAY_10MS
#define TIME_WAIT_CMDRSP_END			HW_TIMER_DELAY_10MS
#define TIME_WAIT_1_BLK_END				(HW_TIMER_DELAY_1s*1)
#define TIME_WAIT_N_BLK_END				(HW_TIMER_DELAY_1s*2)
#define TIME_RSTW				        (HW_TIMER_DELAY_1US*10)
#define TIME_RSCA				        (HW_TIMER_DELAY_1US*1000)


//=====================================================
// set FCIE clock
//=====================================================
// [FIXME] -->
#define FCIE_DEFAULT_CLK		EMMC_PLL_CLK_200M
// ECSD[196]
#define ECSD_HS400	BIT(6)
#define ECSD_HS200	BIT(4)
#define ECSD_DDR		BIT(2)
// ECSD[185]
#define SPEED_OLD            0
#define SPEED_HIGH           1
#define SPEED_HS200          2
#define SPEED_HS400          3

//-------------------------------------------------------
// Devices has to be in 512B block length mode by default
// after power-on, or software reset.
//-------------------------------------------------------
#define EMMC_SECTOR_512BYTE		0x200
#define EMMC_SECTOR_512BYTE_BITS	9
#define EMMC_SECTOR_512BYTE_MASK	(EMMC_SECTOR_512BYTE-1)

#define EMMC_SECTOR_BUF_16KB		(EMMC_SECTOR_512BYTE * 0x20)

#define EMMC_SECTOR_BYTECNT			EMMC_SECTOR_512BYTE
#define EMMC_SECTOR_BYTECNT_BITS	EMMC_SECTOR_512BYTE_BITS
//-------------------------------------------------------

#define EMMC_EXTCSD_SETBIT	1
#define EMMC_EXTCSD_CLRBIT	2
#define EMMC_EXTCSD_WBYTE	3


#define EMMC_EXTCSD_ACCESS_MASK	3
#define EMMC_EXTCSD_ACCESS_SHIFT 24
#define EMMC_EXTCSD_IDX_SHIFT 16
#define EMMC_EXTCSD_VALUE_SHIFT 8



#define EMMC_CMD_BYTE_CNT	5
#define EMMC_R1_BYTE_CNT	5
#define EMMC_R1B_BYTE_CNT	5
#define EMMC_R2_BYTE_CNT	16
#define EMMC_R3_BYTE_CNT	5
#define EMMC_R4_BYTE_CNT	5
#define EMMC_R5_BYTE_CNT	5
#define EMMC_MAX_RSP_BYTE_CNT	EMMC_R2_BYTE_CNT
#define CMD_SIZE_INIT       (EMMC_CMD_BYTE_CNT << BIT_CMD_SIZE_SHIFT)

//===========================================================
// DDR Timing Table
//===========================================================
struct fcie_atop_set {
	u32 scan_result;
	u8	clk;
	u8	reg2ch, skew4;
	u8	cell;
	u8	skew2, cell_cnt;
} EMMC_PACK1;

struct fcie_atop_set_ext {
	u32 rxdll_result[5];
	u8 skew4_idx;
	u8 reg2ch[5], skew4[5];
	u8 cell[5], cell_cnt[5];
} EMMC_PACK1;

struct fcie_ddrt_param {

	u8 dqs, cell;

} EMMC_PACK1;

struct fcie_ddrt_set {

	u8 clk;
	struct fcie_ddrt_param param; // register values

} EMMC_PACK1;


#define EMMC_FCIE_DDRT_SET_CNT	12

#define EMMC_TIMING_SET_MAX		0
#define EMMC_TIMING_SET_MIN		1

// ----------------------------------------------
struct fcie_timing_table {

	u8 set_cnt, curset_idx;

	#if !(defined(ENABLE_EMMC_ATOP) && ENABLE_EMMC_ATOP)
		// DDR48 (digital macro)
		struct fcie_ddrt_set set[EMMC_FCIE_DDRT_SET_CNT];
	#else
		// ATOP (for  DDR52, HS200, HS400)
		struct fcie_atop_set set[1];//EMMC_FCIE_VALID_CLK_CNT];
	#endif

	u32 chk_sum; // put in the last
	u32 ver_no; // for auto update

} EMMC_PACK1;

//temp solution for monaco U02 HS400
struct fcie_timing_ext_table {
	struct fcie_atop_set_ext Set;
	u32 chk_sum; // put in the last
	u32 ver_no; // for auto update
} EMMC_PACK1;

#define REG_OP_W	1
#define REG_OP_CLRBIT  2
#define REG_OP_SETBIT  3

struct fcie_reg_set {//total 10 bytes
	/*(BANK_ADDRESS + REGISTER OFFSET ) << 2*/
	u32 reg_addr;
	u16 reg_value;
	u16 reg_mask;
	u16 opcode;
} EMMC_PACK1;

struct fcie_gen_timing_table {
	u32 chk_sum;
	u32 ver_no; // for auto update
	u32 clk;
	u8 speed_mode;
	u8 curset_idx;
	u8 register_cnt;
	u8 set_cnt;
	u8 cid[EMMC_MAX_RSP_BYTE_CNT];
	u32 device_driving;
	u32 dummy[5];	   //for extension
	struct fcie_reg_set regset[45]; //at most 45 register set
} EMMC_PACK1;

#if defined(CONFIG_EMMC_FORCE_DDR52)
#define EMMC_TIMING_TABLE_VERSION		 2
#else
#define EMMC_TIMING_TABLE_VERSION		 4 // for CL.731742 & later
#endif
#define EMMC_TIMING_TABLE_CHKSUM_OFFSET  8

//===========================================================
// driver flag (drv_flag)
//===========================================================
#define DRV_FLAG_INIT_DONE		 BIT(0) // include EMMC identify done

#define DRV_FLAG_GET_PART_INFO	 BIT(1)
#define DRV_FLAG_RSP_WAIT_D0H	 BIT(2) // currently only R1b

#define DRV_FLAG_DDR_MODE		 BIT(3)
#define DRV_FLAG_TUNING_TTABLE	 BIT(4) // to avoid retry & heavy log
#define DRV_FLAG_SPEED_MASK		(BIT(7)|BIT(6)|BIT(5))
#define DRV_FLAG_SPEED_HIGH		BIT(5)
#define DRV_FLAG_SPEED_HS200	BIT(6)
#define DRV_FLAG_SPEED_HS400	BIT(7)

#define DRV_FLAG_PWROFFNOTIF_MASK	(BIT(8)|BIT(9))
#define DRV_FLAG_PWROFFNOTIF_OFF	0
#define DRV_FLAG_PWROFFNOTIF_ON		BIT(8)
#define DRV_FLAG_PWROFFNOTIF_SHORT	BIT(9)
#define DRV_FLAG_PWROFFNOTIF_LONG	(BIT(8)|BIT(9))

#define DRV_FLAG_RSPFROMRAM_SAVE	BIT(10)
#define DRV_FLAG_ERROR_RETRY		BIT(11)
#define DRV_FLAG_RESUME		        BIT(12)


struct fcie_emmc_driver {
	u32 chk_sum; // [8th ~ last-512] bytes
	u8  sig[4];	// 'e','M','M','C'

	// ----------------------------------------
	// FCIE
	// ----------------------------------------
	u16 rca;
	u32 drv_flag;
	u32	last_err_code;
	u8	rsp[EMMC_MAX_RSP_BYTE_CNT];
	u8	csd[EMMC_MAX_RSP_BYTE_CNT];
	u8	cid[EMMC_MAX_RSP_BYTE_CNT];
	u8	pad_type;
	u16 reg10_mode;
	u32 clk_khz;
	u16 clk_reg_val;
	struct fcie_timing_table t_table;
	struct fcie_gen_timing_table t_table_g;

	// ----------------------------------------
	// EMMC
	// ----------------------------------------
	u8	if_sector_mode;

	// extcsd
	u32 sec_count;
	u32 boot_sec_count;

	#define BUS_WIDTH_1	1
	#define BUS_WIDTH_4	4
	#define BUS_WIDTH_8	8

	u8	bus_width;
	u8	erased_mem_content;
	u16 reliable_wblk_cnt;
	u8	ecsd185_hstiming;
	u8  ecsd192_ver;
	u8	ecsd196_devtype;
	u8	ecsd197_driver_strength;
	u8	ecsd248_cmd6to;
	u8	ecsd247_pwrofflongto;
	u8	ecsd34_pwroffctrl;
	u8	ecsd160_partsupfield;
	u8	ecsd224_hcerasegrpsize;
	u8	ecsd221_hcwpgrpsize;
	u8	ecsd159_maxenhsize_2;
	u8	ecsd158_maxenhsize_1;
	u8	ecsd157_maxenhsize_0;
	u8	ecsd155_partsetcomplete;
	u8	ecsd166_wrrelparam;
	u8	ecsd184_stroe_support;
	u8	bootbus_condition;

	//FCIE5
	// misc
	u8	partition_config;
	u8	cache_ctrl;
	u8	cmdq_enable;

	u16 old_pll_clk_param;
	u16 old_pll_dll_clk_param;
	u8	tune_overtone;

};

// ADMA Descriptor
struct	adma_descriptor {
	u32	end	: 1;
	u32	miu_sel	: 2;
	u32:13;
	u32 job_cnt	: 16;
	u32 address;
	u32 dma_len;
	u32 address2;
};

#define CLK_400KHz				(400*1000)
#define CLK_200MHz				(200*1000*1000)
#define CLK_48MHz               (48*1000*1000)
#define EMMC_GENERIC_WAIT_TIME	(HW_TIMER_DELAY_1s*10)
#define EMMC_READ_WAIT_TIME		(HW_TIMER_DELAY_500MS)
#define EMMC_CQE_WAIT_IDLE_TIME	(HW_TIMER_DELAY_1s*20)

struct mtk_fcie_host_next {
	unsigned int	dma_len;
	int	cookie;
};

struct mtk_fcie_compatible {
	char *name;
	bool analog_skew4;
	bool digital_skew4;
	bool skip_identify;
	bool power_saving_mode;
	bool enable_uma;
	bool share_pin;
	bool share_ip;
	bool enable_cqe;
	u16 analog_skew4_reg_offset;
	u8 analog_skew4_reg_bit;
	u16 digital_skew4_reg_offset;
	u8 digital_skew4_reg_bit;
};

struct mtk_fcie_emmc_rw_speed {
	s64 total_read_bytes;
	s64 total_read_time_usec;//usec
	s64 total_write_bytes;
	s64 total_write_time_usec;//usec
	s64 total_cqe_time_usec;//usec
	s64 day_write_bytes;
	int int_tm_yday; /* day of year (0 -365) */
};

struct mtk_fcie_host {
	struct device *dev;
	struct mmc_host	*mmc;
	struct mmc_command	*cmd;
	struct mmc_data *data;
	struct mmc_request	*request;
	const struct mtk_fcie_compatible	*dev_comp;
	void __iomem	*fciebase;
	void __iomem	*pwsbase;
	void __iomem	*emmcpllbase;
	void __iomem	*fdebase;
	void __iomem	*fcieclkreg;
	void __iomem	*fcieclkreg2;
	void __iomem	*riubaseaddr;

	struct clk	*fcie_syn_clk;
	struct clk	*fcie_top_clk;
	struct clk	*smi_fcie2fcie;
	struct delayed_work req_timeout_work;
	struct work_struct emmc_recovery_work;
	int	irq;
	//REDUCE REG ACCESS
	u16 reg_int_en;

	u32	clk;
	u32	fcie_syn_clk_rate;
	u32	fcie_top_clk_rate;
	u16	sd_mod;

	struct mtk_fcie_host_next	next_data;
	//adma descriptor
	struct adma_descriptor	*adma_desc;
	dma_addr_t	adma_desc_addr;
	u8	host_ios_timing;
	u32	host_ios_clk;
	//controller register variable
	spinlock_t	lock;
	struct fcie_emmc_driver	*emmc_drv;
	int error;
	bool cmd_resp_busy;


	//dts property

	u32	host_driving;
	u32	device_driving;
	u32	support_cqe;
	u32	share_ip;
	u32	share_pin;
	u32	clk_shift;
	u32	clk_mask;
	u32	clk_1xp;
	u32	clk_2xp;
	u32	v_emmc_pll;
	u64	miu0_base;
	u8 enable_ssc;
	u8 enable_sar5;
	u8 enable_fde;
	u8 emmc_pll_skew4;
	u8 emmc_pll_flash_macro;
	//attribute
	u32	emmc_read_log_enable;
	u32	emmc_write_log_enable;
	u32	emmc_monitor_enable;
	u32	cqe_monitor_enable;
	u32	emulate_power_lost_event_enable;
	u32	dump_fcie_reg_enable;
	int	log_level;

	ktime_t	starttime;
	struct mtk_fcie_emmc_rw_speed emmc_rw_speed;
	u8	fde_runtime_en;
	struct list_head crypto_white_list;

	#if defined(CONFIG_MMC_MTK_ROOT_WAIT) && CONFIG_MMC_MTK_ROOT_WAIT
	struct completion init_mmc_done;
	#endif
};	/* struct mtk_fcie_host*/

/**
 * struct emmc_crypto_disable_info - eMMC crypto disable info
 * @list: list headed by host->crypto_disable_list_head
 * @lba: logical block address
 * @length: transfer length in block
 */
struct emmc_crypto_disable_info {
	struct list_head list;
	u64 lba;
	u64 length;
};

enum write_read_size {
	WR_KB = 1024,
	WR_MB = 1024*1024,
	WR_GB = 1024*1024*1024,
};

#define EMMC_DAY_WRITE_WARN  (1024*1024*1024) // 1 GB

#define MAX_RETRY_TIME		1000

#define SPEED_TIME_MSEC		1000

#define CMD_R1B_RESPONSE  0x900
#define CMD_R1_RESPONSE   0x900

#define MAX_CQE_CMD13_TIME 3000
#define HW_TIMER_US_NS 1000

#define PART_CONFIG_ACCESS_MASK 7

#define CMD1_BUSY_SHIFT  31
#define CMD1_SECTOR_MODE_SHIFT  30

#define EMMC_CLK_KHZ 1000
//host error
#define REQ_CMD_EIO    (0x1 << 0)
#define REQ_CMD_NORSP  (0x1 << 1)
#define REQ_DAT_EIO    (0x1 << 2)
#define REQ_DAT_EILSEQ (0x1 << 3)
#define REQ_STOP_EIO   (0x1 << 4)
#define REQ_STOP_TMO   (0x1 << 5)
#define REQ_CMD_BUSY   (0x1 << 6)
#define REQ_CMD_TMO    (0x1 << 7)
#define REQ_DATA_TMO   (0x1 << 8)



#define DMA_15_0_MASK 0xFFFF
#define DMA_31_16_SHIFT 16
#define DMA_47_32_SHIFT 32
#define DMA_ADDR_64_BYTE 8

#define PARTITION_CONFIG_MASK 0x7
#define DATA_TIMEOUT (20 * HZ)
#define MAX_BUSY_TIME (120 * HZ)
#define ADMA_TABLE_DMA_LEN_15_0 0x0010
#define ADMA_TABLE_DMA_LEN_31_16 0
#define WAIT_D0_HIGH_EXPIRES (1000 * 1000)
#define HIGH_CAPACITY_VERSION  3
#define CMD_OPCODE_MASK 0x3F
//=====================================================
// Host driver define
//=====================================================
#define HOST_MAX_SEGS              128
#define HOST_CQE_MAX_SEGS          32
#define HOST_MAX_SEG_SIZE          (64 * 1024)


//=====================================================
//Pll version
//=====================================================
#define eMMC_PLL_VER7 7
#define eMMC_PLL_VER12 12
#define eMMC_PLL_VER22 22

#define eMMC_PLL_0x14_VER7 0x0011
#define eMMC_PLL_0x15_VER7 0x042B

#define eMMC_PLL_0x14_VER12 0x0010
#define eMMC_PLL_0x15_VER12 0x0430

#define eMMC_PLL_0x14_VER22 0xC01D
#define eMMC_PLL_0x15_VER22 0x0320

#define eMMC_PLL_0x14_MSTAR 0xC01C
#define eMMC_PLL_0x15_MSTAR 0x0339


#endif
