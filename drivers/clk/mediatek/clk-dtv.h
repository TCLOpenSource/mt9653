/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/io.h>

#define MUX_GATE_CONTROL_BITS	4

/*
 * static inline u16 readw_relaxed(const volatile void __iomem *addr)
 * static inline void writew(u16 value, volatile void __iomem *addr)
 */

#define clk_reg_r(regOffset) readw(regOffset)
#define clk_reg_w(value, regOffset) writew(value, regOffset)

struct clock_reg_save {
	struct list_head node;
	struct clk_hw *c_hw;
};

struct dtv_clk_reg_access {
	unsigned int bank_type;
	void __iomem *base;
	unsigned int size;
};

enum merak_regbank_type {
	CLK_PM,
	CLK_NONPM,
	CLK_BANK_TYPE_MAX,
};

int clock_write(enum merak_regbank_type bank_type, unsigned int reg_addr, u16 value,
		unsigned int start, unsigned int end);

int clock_read(enum merak_regbank_type bank_type, unsigned int reg_addr,
		unsigned int start, unsigned int end);
