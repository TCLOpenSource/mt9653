// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

// TODO: temp solution, it will be fixed to linux coding style next iteration
#define _VBDMA_C

/* Kernel includes. */
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/io.h>

/* driver includes. */
#include "vbdma.h"

//----------------------------------------------
// Defines
//----------------------------------------------
#define GET_REG16_ADDR(x, y)              (x+(y)*4)
#define GET_REG_OFFSET(x, y)              ((x)*0x200+(y)*0x4)
// TODO: apply base address from device tree
#define REG_ADDR_BASE_VBDMA               vbdma_base
#define VBDMA_SET_CH_REG(x)               (GET_REG16_ADDR(REG_ADDR_BASE_VBDMA, x))
#define VBDMA_REG_CTRL(ch)                VBDMA_SET_CH_REG(0x00 + ch*0x10)
#define VBDMA_REG_STATUS(ch)              VBDMA_SET_CH_REG(0x01 + ch*0x10)
#define VBDMA_REG_DEV_SEL(ch)             VBDMA_SET_CH_REG(0x02 + ch*0x10)
#define VBDMA_REG_MISC(ch)                VBDMA_SET_CH_REG(0x03 + ch*0x10)
#define VBDMA_REG_SRC_ADDR_L(ch)          VBDMA_SET_CH_REG(0x04 + ch*0x10)
#define VBDMA_REG_SRC_ADDR_H(ch)          VBDMA_SET_CH_REG(0x05 + ch*0x10)
#define VBDMA_REG_DST_ADDR_L(ch)          VBDMA_SET_CH_REG(0x06 + ch*0x10)
#define VBDMA_REG_DST_ADDR_H(ch)          VBDMA_SET_CH_REG(0x07 + ch*0x10)
#define VBDMA_REG_SIZE_L(ch)              VBDMA_SET_CH_REG(0x08 + ch*0x10)
#define VBDMA_REG_SIZE_H(ch)              VBDMA_SET_CH_REG(0x09 + ch*0x10)
#define VBDMA_REG_CMD0_L(ch)              VBDMA_SET_CH_REG(0x0A + ch*0x10)
#define VBDMA_REG_CMD0_H(ch)              VBDMA_SET_CH_REG(0x0B + ch*0x10)
#define VBDMA_REG_CMD1_L(ch)              VBDMA_SET_CH_REG(0x0C + ch*0x10)
#define VBDMA_REG_CMD1_H(ch)              VBDMA_SET_CH_REG(0x0D + ch*0x10)
#define VBDMA_REG_CMD2_L(ch)              VBDMA_SET_CH_REG(0x0E + ch*0x10)
#define VBDMA_REG_CMD2_H(ch)              VBDMA_SET_CH_REG(0x0F + ch*0x10)

#define VBDMA_REG_ADDR_H_BIT              (3) // only additional 2 bit for addr > u32
#define VBDMA_REG_SRC_H_ADDR_BIT          (10)// 10~11 bit reg_ch0_src_a_h
#define VBDMA_REG_DST_H_ADDR_BIT          (12)// 12~13 bit reg_ch0_dst_a_h

//---------------------------------------------
// definition for VBDMA_REG_CH0_CTRL/VBDMA_REG_CH1_CTRL
//---------------------------------------------
#define VBDMA_CH_TRIGGER                  BIT(0)
#define VBDMA_CH_STOP                     BIT(4)

//---------------------------------------------
// definition for REG_VBDMA_CH0_STATUS/REG_VBDMA_CH1_STATUS
//---------------------------------------------
#define VBDMA_CH_QUEUED                   BIT(0)
#define VBDMA_CH_BUSY                     BIT(1)
#define VBDMA_CH_INT                      BIT(2)
#define VBDMA_CH_DONE                     BIT(3)
#define VBDMA_CH_RESULT                   BIT(4)
#define VBDMA_CH_CLEAR_STATUS             (VBDMA_CH_INT|VBDMA_CH_DONE|VBDMA_CH_RESULT)

//---------------------------------------------
// definition for REG_VBDMA_CH0_MISC/REG_VBDMA_CH1_MISC
//---------------------------------------------
#define VBDMA_CH_ADDR_DECDIR              BIT(0)
#define VBDMA_CH_DONE_INT_EN              BIT(1)
#define VBDMA_CH_CRC_REFLECTION           BIT(4)
#define VBDMA_CH_MOBF_EN                  BIT(5)

#define VBDMA_FILTER_16BIT                        (0xFFFF)
#define VBDMA_SHIFT_16BIT                         (16)

#define VBDMA_FLASH_TO_MIU_POLLING                (5000)
#define VBDMA_FLASH_TO_MIU_DEV_SEL                (0x4035)

#define VBDMA_CRC32_FROM_MIU_MISC                 (0x0010)
#define VBDMA_CRC32_FROM_IMI_MISC                 (0x2010)
#define VBDMA_CRC32_FROM_MIU_DEV_SEL              (0x0340)
#define VBDMA_CRC32_FROM_IMI_DEV_SEL              (0x0340)
#define VBDMA_CRC32_CMD0_L                        (0x1db7)
#define VBDMA_CRC32_CMD0_H                        (0x04c1)

#define VBDMA_COPY_MIU2IMI_MISC                   (0x8000)
#define VBDMA_COPY_IMI2MIU_MISC                   (0x2000)
#define VBDMA_COPY_MIU2MIU_MISC                   (0x8000)
#define VBDMA_COPY_IMI2IMI_MISC                   (0x8000)

#define VBDMA_COPY_MIU2IMI_DEV_SEL                (0x4140)
#define VBDMA_COPY_IMI2MIU_DEV_SEL                (0x4140)
#define VBDMA_COPY_MIU2MIU_DEV_SEL                (0x4040)
#define VBDMA_COPY_IMI2IMI_DEV_SEL                (0x4141)

#define VBDMA_SEARCH_FROM_IMI_MISC                (0x2000)
#define VBDMA_SEARCH_FROM_IMI_DEV_SEL             (0x0240)

//----------------------------------------------
// Type and Structure Declaration
//----------------------------------------------
struct vbdma_data {
	u32 reg_addr;
	u32 reg_length;
};

static void __iomem *vbdma_base;

void vbdma_get_base(void __iomem *vbdma_base_addr)
{
	vbdma_base = vbdma_base_addr;
}
EXPORT_SYMBOL_GPL(vbdma_get_base);

// TODO: apply interrupt instead of polling
bool vbdma_polling_done(bool ch)
{
	while (1) {
		if (readw(VBDMA_REG_STATUS(ch)) & VBDMA_CH_DONE)
			return true;
	}
}

//bool vbdma_busy(bool ch)
//{
//	return (VBDMA_CH_BUSY ==
//		(readw(VBDMA_REG_STATUS(ch)) & VBDMA_CH_BUSY));
//}

//bool vbdma_flash_to_miu(bool ch, u32 flash_addr, u32 dram_addr,
//			  u32 len)
//{
//	int i = 0;
//	//dram_addr &= (~MIU0_START_ADDR);
//	if ((len & 0x0F) || (flash_addr & 0x0F) || (dram_addr & 0x0F))
//		return false;
//
//	for (i = 0; i < VBDMA_FLASH_TO_MIU_POLLING; i++) {
//		if (0 == (readw(VBDMA_REG_STATUS(ch)) & VBDMA_CH_BUSY))
//			break;
//		mdelay(1);
//	}
//
//	if (i == VBDMA_FLASH_TO_MIU_POLLING) {
//		pr_err("VBDMA_FLASH_TO_MEM error!! device is busy!!\n");
//		return false;
//	}
//
//	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
//
//	writew(VBDMA_FLASH_TO_MIU_DEV_SEL, VBDMA_REG_DEV_SEL(ch));
//	writew((u16) (flash_addr & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_L(ch));
//	writew((u16) ((flash_addr >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT),
//		VBDMA_REG_SRC_ADDR_H(ch));
//
//	writew((u16) (dram_addr & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_L(ch));
//	writew((u16) ((dram_addr >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT),
//		VBDMA_REG_DST_ADDR_H(ch));
//
//	writew((u16) (len & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_L(ch));
//	writew((u16) ((len >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_H(ch));
//
//	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));
//
//	return true;
//}

bool vbdma_crc32_from_miu(bool ch, u32 dram_addr, u32 dram_addr_H, u32 len)
{
	u16 tempaddr = 0;
	//dram_addr &= (~MIU0_START_ADDR);

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
	//reflection 1,checked with VAL
	writew(VBDMA_CRC32_FROM_MIU_MISC, VBDMA_REG_MISC(ch));

	writew(VBDMA_CRC32_FROM_MIU_DEV_SEL, VBDMA_REG_DEV_SEL(ch));
	writew((u16) (dram_addr & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((dram_addr >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT),
		 VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (dram_addr & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((dram_addr >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT),
		 VBDMA_REG_DST_ADDR_H(ch));

	// set addr high bit
	// srcaddr high bit offset=0 , bit=10~11
	// dstaddr high bit offset=0 , bit=12~13
	if (dram_addr_H != 0) {
		tempaddr = readw(VBDMA_REG_CTRL(ch));
		tempaddr = tempaddr | (u16)((dram_addr_H & VBDMA_REG_ADDR_H_BIT)
		 << VBDMA_REG_SRC_H_ADDR_BIT);
		tempaddr = tempaddr | (u16)((dram_addr_H & VBDMA_REG_ADDR_H_BIT)
		 << VBDMA_REG_DST_H_ADDR_BIT);
		writew(tempaddr, VBDMA_REG_CTRL(ch));
	}

	writew((u16) (len & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_H(ch));

	writew(VBDMA_CRC32_CMD0_L, VBDMA_REG_CMD0_L(ch));
	writew(VBDMA_CRC32_CMD0_H, VBDMA_REG_CMD0_H(ch));

	writew(VBDMA_FILTER_16BIT, VBDMA_REG_CMD1_L(ch));
	writew(VBDMA_FILTER_16BIT, VBDMA_REG_CMD1_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	return true;
}

//bool vbdma_crc32_from_imi(bool ch, u32 sram_addr, u32 len)
//{
//	//dram_addr &= (~MIU0_START_ADDR);
//
//	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
//    //reflection 1,checked with VAL
//	writew(VBDMA_CRC32_FROM_IMI_MISC, VBDMA_REG_MISC(ch));
//
//	writew(VBDMA_CRC32_FROM_IMI_DEV_SEL, VBDMA_REG_DEV_SEL(ch));
//	writew((u16) (sram_addr & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_L(ch));
//	writew((u16) ((sram_addr >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT),
//		 VBDMA_REG_SRC_ADDR_H(ch));
//
//	writew((u16) (sram_addr & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_L(ch));
//	writew((u16) ((sram_addr >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT),
//		 VBDMA_REG_DST_ADDR_H(ch));
//
//	writew((u16) (len & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_L(ch));
//	writew((u16) ((len >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_H(ch));
//
//	writew(VBDMA_CRC32_CMD0_L, VBDMA_REG_CMD0_L(ch));
//	writew(VBDMA_CRC32_CMD0_H, VBDMA_REG_CMD0_H(ch));
//
//	writew(VBDMA_FILTER_16BIT, VBDMA_REG_CMD1_L(ch));
//	writew(VBDMA_FILTER_16BIT, VBDMA_REG_CMD1_H(ch));
//
//	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));
//
//	return true;
//}

//bool vbdma_pattern_search_from_imi(bool ch, u32 sram_addr,
//				    u32 len, u32 pattern)
//{
//	u16 reg_val;
//
//	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
//	//reflection 1,checked with VAL
//	writew(VBDMA_SEARCH_FROM_IMI_MISC, VBDMA_REG_MISC(ch));
//
//	writew(VBDMA_SEARCH_FROM_IMI_DEV_SEL, VBDMA_REG_DEV_SEL(ch));
//	writew((u16) (sram_addr & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_L(ch));
//	writew((u16) ((sram_addr >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT),
//		 VBDMA_REG_SRC_ADDR_H(ch));
//
//	writew((u16) (sram_addr & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_L(ch));
//	writew((u16) ((sram_addr >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT),
//		 VBDMA_REG_DST_ADDR_H(ch));
//
//	writew((u16) (len & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_L(ch));
//	writew((u16) ((len >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_H(ch));
//
//	writew((u16) (pattern & VBDMA_FILTER_16BIT), VBDMA_REG_CMD0_L(ch));
//	writew((u16) ((pattern >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_CMD0_H(ch));
//
//	writew(0, VBDMA_REG_CMD1_L(ch));
//	writew(0, VBDMA_REG_CMD1_H(ch));
//
//	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));
//
//	while (1) {
//		reg_val = readw(VBDMA_REG_STATUS(ch));
//
//		if ((reg_val & VBDMA_CH_DONE) || (reg_val & VBDMA_CH_RESULT))
//			break;
//	}
//
//	if (reg_val & VBDMA_CH_RESULT)
//		return true;
//	else
//		return false;
//}

u32 vbdma_get_crc32(bool ch)
{
	return (readw(VBDMA_REG_CMD1_L(ch)) |
		(readw(VBDMA_REG_CMD1_H(ch)) << VBDMA_SHIFT_16BIT));
}

//bool vbdma_imi2miu(bool ch, u32 src, u32 dst, u32 len)
//{
//
//	//src &= (~MIU_BASE_ADDR);
//	//dst &= (~MIU_BASE_ADDR);
//
//	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
//    // replace 00:MIU0, 01:MIU1, 10:IMI; [15:14] for CHN1, [13:12] for CHN0
//	writew(VBDMA_COPY_IMI2MIU_MISC, VBDMA_REG_MISC(ch));
//    //replace  ch0 to IMI, ch1 to MIU
//	writew(VBDMA_COPY_IMI2MIU_DEV_SEL, VBDMA_REG_DEV_SEL(ch));
//	writew((u16) (src & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_L(ch));
//	writew((u16) ((src >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_H(ch));
//
//	writew((u16) (dst & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_L(ch));
//	writew((u16) ((dst >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_H(ch));
//
//	writew((u16) (len & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_L(ch));
//	writew((u16) ((len >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_H(ch));
//
//	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));
//
//	return true;
//}

//bool vbdma_miu2imi(bool ch, u32 src, u32 dst, u32 len)
//{
//
//	//src &= (~MIU_BASE_ADDR);
//	//dst &= (~MIU_BASE_ADDR);
//
//	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
//
//	//writew(VBDMA_REG_MISC(ch),VBDMA_CH_DONE_INT_EN | 0x2000);
//	writew(VBDMA_COPY_MIU2IMI_MISC, VBDMA_REG_MISC(ch));
//
//	//writew(VBDMA_REG_DEV_SEL(ch),0x4041);
//	writew(VBDMA_COPY_MIU2IMI_DEV_SEL, VBDMA_REG_DEV_SEL(ch));	//replace MIU ch0
//	writew((u16) (src & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_L(ch));
//	writew((u16) ((src >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_H(ch));
//
//	writew((u16) (dst & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_L(ch));
//	writew((u16) ((dst >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_H(ch));
//
//	writew((u16) (len & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_L(ch));
//	writew((u16) ((len >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_H(ch));
//
//	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));
//
//	return true;
//}

//bool vbdma_imi2imi(bool ch, u32 src, u32 dst, u32 len)
//{
//
//	//src &= (~IMI_BASE_ADDR);
//	//dst &= (~IMI_BASE_ADDR);
//
//	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
//
//	writew(VBDMA_COPY_IMI2IMI_MISC, VBDMA_REG_MISC(ch));
//
//	writew(VBDMA_COPY_IMI2IMI_DEV_SEL, VBDMA_REG_DEV_SEL(ch));
//
//	writew((u16) (src & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_L(ch));
//	writew((u16) ((src >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_H(ch));
//
//	writew((u16) (dst & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_L(ch));
//	writew((u16) ((dst >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_H(ch));
//
//	writew((u16) (len & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_L(ch));
//	writew((u16) ((len >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_H(ch));
//
//	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));
//
//	return true;
//}

//bool vbdma_miu2miu(bool ch, u32 src, u32 dst, u32 len)
//{
//
//	//src &= (~IMI_BASE_ADDR);
//	//dst &= (~IMI_BASE_ADDR);
//
//	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
//
//	writew(VBDMA_COPY_MIU2MIU_MISC, VBDMA_REG_MISC(ch));
//
//	writew(VBDMA_COPY_MIU2MIU_DEV_SEL, VBDMA_REG_DEV_SEL(ch));
//
//	writew((u16) (src & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_L(ch));
//	writew((u16) ((src >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SRC_ADDR_H(ch));
//
//	writew((u16) (dst & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_L(ch));
//	writew((u16) ((dst >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_DST_ADDR_H(ch));
//
//	writew((u16) (len & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_L(ch));
//	writew((u16) ((len >> VBDMA_SHIFT_16BIT) & VBDMA_FILTER_16BIT), VBDMA_REG_SIZE_H(ch));
//
//	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));
//
//	return true;
//}

//bool vbdma_clear_status(bool ch)
//{
//	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
//	return true;
//}

//bool vbdma_copy(bool ch, u32 src, u32 dst, u32 len, vbdma_mode mode)
//{
//	switch (mode) {
//	case E_VBDMA_CPtoHK:
//		vbdma_imi2miu(ch, src, dst, len);
//		break;
//	case E_VBDMA_HKtoCP:
//		vbdma_miu2imi(ch, src, dst, len);
//		break;
//	case E_VBDMA_CPtoCP:
//		vbdma_imi2imi(ch, src, dst, len);
//		break;
//	case E_VBDMA_HKtoHK:
//		vbdma_miu2miu(ch, src, dst, len);
//		break;
//	default:
//		pr_err("%s() : mode %d failed\n"
//				, __func__, mode);
//		return false;
//	}
//	vbdma_polling_done(ch);
//
//	return true;
//
//}
//EXPORT_SYMBOL_GPL(vbdma_copy);

u32 vbdma_crc32(bool ch, u32 src, u32 src_H, u32 len, vbdma_dev dev)
{
	switch (dev) {
	case E_VBDMA_DEV_MIU:
		vbdma_crc32_from_miu(ch, src, src_H, len);
		break;
//	case E_VBDMA_DEV_IMI:
//		vbdma_crc32_from_imi(ch, src, len);
//		break;
	default:
		pr_err("%s(): Dev %d not support\n",
			__func__, dev);
		return false;
	}
	vbdma_polling_done(ch);

	return vbdma_get_crc32(ch);
}
EXPORT_SYMBOL_GPL(vbdma_crc32);

//bool vbdma_pattern_search(bool ch, u32 src, u32 len, u32 pattern,
//			     vbdma_dev dev)
//{
//	switch (dev) {
//	case E_VBDMA_DEV_IMI:
//		return vbdma_pattern_search_from_imi(ch, src, len,
//						pattern);
//	default:
//		pr_err("%s(): Dev %d not support\n",
//			__func__, dev);
//		return false;
//	}
//}
