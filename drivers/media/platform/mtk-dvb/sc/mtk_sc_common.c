// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>


#include "mtk_sc_common.h"

void reg32_write(REG16 __iomem *base, u32 offset, u32 value)
{
	u16 value_low = (u16) value;
	u16 value_high = (u16) (value >> 16);

	writew(value_low, base + offset);
	writew(value_high, base + (offset + 1));
}

u32 reg32_read(REG16 __iomem *base, u32 offset)
{
	u16 value_low = readw(base + offset);
	u16 value_high = readw(base + (offset + 1));

	return ((((u32) value_high) << 16) + (u32) value_low);
}

void reg16_write(REG16 __iomem *base, u32 offset, u16 value)
{
	writew(value, base + offset);
}

u16 reg16_read(REG16 __iomem *base, u32 offset)
{
	return readw(base + offset);
}

void reg16_set_bit(REG16 __iomem *base, u32 offset, u8 nr)
{
	u16 value;

	value = readw(base + offset);
	value |= (1 << nr);
	writew(value, base + offset);
}

void reg16_clear_bit(REG16 __iomem *base, u32 offset, u8 nr)
{
	u16 value;

	value = readw(base + offset);
	value &= ~(1 << nr);
	writew(value, base + offset);
}

bool reg16_test_bit(REG16 __iomem *base, u32 offset, u8 nr)
{
	if (readw(base + offset) & (1 << nr))
		return true;
	else
		return false;
}

void reg16_set_mask(REG16 __iomem *base, u32 offset, u16 mask)
{
	u16 value;

	value = readw(base + offset);
	value |= mask;
	writew(value, base + offset);
}

void reg16_clear_mask(REG16 __iomem *base, u32 offset, u16 mask)
{
	u16 value;

	value = readw(base + offset);
	value &= ~mask;
	writew(value, base + offset);
}

void reg16_mask_write(REG16 __iomem *base, u32 offset, u16 mask, u16 value)
{
	u16 temp;

	temp = readw(base + offset);
	temp &= ~mask;
	temp |= (mask & value);
	writew(temp, base + offset);
}

u16 reg16_mask_read(REG16 __iomem *base, u32 offset, u16 mask, u16 shift)
{
	u16 value;

	value = readw(base + offset);

	return (value & mask) >> shift;
}
