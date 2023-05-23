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

static void __iomem *riu_base;
static u32 vbdma_base;

void vbdma_get_riu_base(u8 *riu_map_addr, u32 vbdma_base_addr)
{
	riu_base = riu_map_addr;
	vbdma_base = vbdma_base_addr;
}

// TODO: apply interrupt instead of polling
bool vbdma_polling_done(bool ch)
{
	while (1) {
		if (readw(VBDMA_REG_STATUS(ch)) & VBDMA_CH_DONE)
			return true;
	}
}

bool vbdma_busy(bool ch)
{
	return (VBDMA_CH_BUSY ==
		(readw(VBDMA_REG_STATUS(ch)) & VBDMA_CH_BUSY));
}

bool vbdma_flash_to_miu(bool ch, u32 flash_addr, u32 dram_addr,
			  u32 len)
{
	int i = 0;
	//dram_addr &= (~MIU0_START_ADDR);
	if ((len & 0x0F) || (flash_addr & 0x0F) || (dram_addr & 0x0F))
		return false;

	for (i = 0; i < 5000; i++) {
		if (0 == (readw(VBDMA_REG_STATUS(ch)) & VBDMA_CH_BUSY))
			break;
		mdelay(1);
	}

	if (i == 5000) {
		pr_err("VBDMA_FLASH_TO_MEM error!! device is busy!!\n");
		return false;
	}

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));

	writew(0x4035, VBDMA_REG_DEV_SEL(ch));
	writew((u16) (flash_addr & 0xFFFF), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((flash_addr >> 16) & 0xFFFF), VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (dram_addr & 0xFFFF), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((dram_addr >> 16) & 0xFFFF), VBDMA_REG_DST_ADDR_H(ch));

	writew((u16) (len & 0xFFFF), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> 16) & 0xFFFF), VBDMA_REG_SIZE_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	return true;
}

bool vbdma_crc32_from_miu(bool ch, u32 dram_addr, u32 len)
{
	//dram_addr &= (~MIU0_START_ADDR);

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
	//reflection 1,checked with VAL
	writew(0x0010, VBDMA_REG_MISC(ch));

	writew(0x0340, VBDMA_REG_DEV_SEL(ch));
	writew((u16) (dram_addr & 0xFFFF), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((dram_addr >> 16) & 0xFFFF),
		 VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (dram_addr & 0xFFFF), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((dram_addr >> 16) & 0xFFFF),
		 VBDMA_REG_DST_ADDR_H(ch));

	writew((u16) (len & 0xFFFF), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> 16) & 0xFFFF), VBDMA_REG_SIZE_H(ch));

	writew(0x1db7, VBDMA_REG_CMD0_L(ch));
	writew(0x04c1, VBDMA_REG_CMD0_H(ch));

	writew(0xFFFF, VBDMA_REG_CMD1_L(ch));
	writew(0xFFFF, VBDMA_REG_CMD1_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	return true;
}

bool vbdma_crc32_from_imi(bool ch, u32 sram_addr, u32 len)
{
	//dram_addr &= (~MIU0_START_ADDR);

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
    //reflection 1,checked with VAL
	writew(0x2010, VBDMA_REG_MISC(ch));

	writew(0x0340, VBDMA_REG_DEV_SEL(ch));
	writew((u16) (sram_addr & 0xFFFF), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((sram_addr >> 16) & 0xFFFF),
		 VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (sram_addr & 0xFFFF), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((sram_addr >> 16) & 0xFFFF),
		 VBDMA_REG_DST_ADDR_H(ch));

	writew((u16) (len & 0xFFFF), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> 16) & 0xFFFF), VBDMA_REG_SIZE_H(ch));

	writew(0x1db7, VBDMA_REG_CMD0_L(ch));
	writew(0x04c1, VBDMA_REG_CMD0_H(ch));

	writew(0xFFFF, VBDMA_REG_CMD1_L(ch));
	writew(0xFFFF, VBDMA_REG_CMD1_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	return true;
}

bool vbdma_pattern_search_from_imi(bool ch, u32 sram_addr,
				    u32 len, u32 pattern)
{
	u16 reg_val;

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
	//reflection 1,checked with VAL
	writew(0x2000, VBDMA_REG_MISC(ch));

	writew(0x0240, VBDMA_REG_DEV_SEL(ch));
	writew((u16) (sram_addr & 0xFFFF), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((sram_addr >> 16) & 0xFFFF),
		 VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (sram_addr & 0xFFFF), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((sram_addr >> 16) & 0xFFFF),
		 VBDMA_REG_DST_ADDR_H(ch));

	writew((u16) (len & 0xFFFF), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> 16) & 0xFFFF), VBDMA_REG_SIZE_H(ch));

	writew((u16) (pattern & 0xFFFF), VBDMA_REG_CMD0_L(ch));
	writew((u16) ((pattern >> 16) & 0xFFFF), VBDMA_REG_CMD0_H(ch));

	writew(0, VBDMA_REG_CMD1_L(ch));
	writew(0, VBDMA_REG_CMD1_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	while (1) {
		reg_val = readw(VBDMA_REG_STATUS(ch));

		if ((reg_val & VBDMA_CH_DONE) || (reg_val & VBDMA_CH_RESULT))
			break;
	}

	if (reg_val & VBDMA_CH_RESULT)
		return true;
	else
		return false;
}

u32 vbdma_get_crc32(bool ch)
{
	return (readw(VBDMA_REG_CMD1_L(ch)) |
		(readw(VBDMA_REG_CMD1_H(ch)) << 16));
}

bool vbdma_imi2miu(bool ch, u32 src, u32 dst, u32 len)
{

	//src &= (~MIU_BASE_ADDR);
	//dst &= (~MIU_BASE_ADDR);

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
    // replace 00:MIU0, 01:MIU1, 10:IMI; [15:14] for CHN1, [13:12] for CHN0
	writew(0x2000, VBDMA_REG_MISC(ch));
    //replace  ch0 to IMI, ch1 to MIU
	writew(0x4140, VBDMA_REG_DEV_SEL(ch));
	writew((u16) (src & 0xFFFF), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((src >> 16) & 0xFFFF), VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (dst & 0xFFFF), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((dst >> 16) & 0xFFFF), VBDMA_REG_DST_ADDR_H(ch));

	writew((u16) (len & 0xFFFF), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> 16) & 0xFFFF), VBDMA_REG_SIZE_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	return true;
}

bool vbdma_miu2imi(bool ch, u32 src, u32 dst, u32 len)
{

	//src &= (~MIU_BASE_ADDR);
	//dst &= (~MIU_BASE_ADDR);

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));

	//writew(VBDMA_REG_MISC(ch),VBDMA_CH_DONE_INT_EN | 0x2000);
	writew(0x8000, VBDMA_REG_MISC(ch));

	//writew(VBDMA_REG_DEV_SEL(ch),0x4041);
	writew(0x4140, VBDMA_REG_DEV_SEL(ch));	//replace MIU ch0
	writew((u16) (src & 0xFFFF), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((src >> 16) & 0xFFFF), VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (dst & 0xFFFF), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((dst >> 16) & 0xFFFF), VBDMA_REG_DST_ADDR_H(ch));

	writew((u16) (len & 0xFFFF), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> 16) & 0xFFFF), VBDMA_REG_SIZE_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	return true;
}

bool vbdma_imi2imi(bool ch, u32 src, u32 dst, u32 len)
{

	//src &= (~IMI_BASE_ADDR);
	//dst &= (~IMI_BASE_ADDR);

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));

	writew(0x8000, VBDMA_REG_MISC(ch));

	writew(0x4141, VBDMA_REG_DEV_SEL(ch));

	writew((u16) (src & 0xFFFF), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((src >> 16) & 0xFFFF), VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (dst & 0xFFFF), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((dst >> 16) & 0xFFFF), VBDMA_REG_DST_ADDR_H(ch));

	writew((u16) (len & 0xFFFF), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> 16) & 0xFFFF), VBDMA_REG_SIZE_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	return true;
}

bool vbdma_miui2imi(bool ch, u32 src, u32 dst, u32 len)
{

	//src &= (~IMI_BASE_ADDR);
	//dst &= (~IMI_BASE_ADDR);

	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));

	writew(0x8000, VBDMA_REG_MISC(ch));

	writew(0x4040, VBDMA_REG_DEV_SEL(ch));

	writew((u16) (src & 0xFFFF), VBDMA_REG_SRC_ADDR_L(ch));
	writew((u16) ((src >> 16) & 0xFFFF), VBDMA_REG_SRC_ADDR_H(ch));

	writew((u16) (dst & 0xFFFF), VBDMA_REG_DST_ADDR_L(ch));
	writew((u16) ((dst >> 16) & 0xFFFF), VBDMA_REG_DST_ADDR_H(ch));

	writew((u16) (len & 0xFFFF), VBDMA_REG_SIZE_L(ch));
	writew((u16) ((len >> 16) & 0xFFFF), VBDMA_REG_SIZE_H(ch));

	writew(VBDMA_CH_TRIGGER, VBDMA_REG_CTRL(ch));

	return true;
}

bool vbdma_clear_status(bool ch)
{
	writew(VBDMA_CH_CLEAR_STATUS, VBDMA_REG_STATUS(ch));
	return true;
}

bool vbdma_copy(bool ch, u32 src, u32 dst, u32 len, vbdma_mode mode)
{
	switch (mode) {
	case E_VBDMA_CPtoHK:
		vbdma_imi2miu(ch, src, dst, len);
		break;
	case E_VBDMA_HKtoCP:
		vbdma_miu2imi(ch, src, dst, len);
		break;
	case E_VBDMA_CPtoCP:
		vbdma_imi2imi(ch, src, dst, len);
		break;
	case E_VBDMA_HKtoHK:
		vbdma_miui2imi(ch, src, dst, len);
		break;
	default:
		pr_err("%s() : mode %d failed\n"
				, __func__, mode);
		return false;
	}
	vbdma_polling_done(ch);

	return true;

}
EXPORT_SYMBOL_GPL(vbdma_copy);

u32 vbdma_crc32(bool ch, u32 src, u32 len, vbdma_dev dev)
{
	switch (dev) {
	case E_VBDMA_DEV_MIU:
		vbdma_crc32_from_miu(ch, src, len);
		break;
	case E_VBDMA_DEV_IMI:
		vbdma_crc32_from_imi(ch, src, len);
		break;
	}
	vbdma_polling_done(ch);

	return vbdma_get_crc32(ch);
}
EXPORT_SYMBOL_GPL(vbdma_crc32);

bool vbdma_pattern_search(bool ch, u32 src, u32 len, u32 pattern,
			     vbdma_dev dev)
{
	switch (dev) {
	case E_VBDMA_DEV_IMI:
		return vbdma_pattern_search_from_imi(ch, src, len,
						pattern);
	default:
		pr_err("%s(): Dev %d not support\n",
			__func__, dev);
		return false;
	}
}
