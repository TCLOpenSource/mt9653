// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include "voc_common.h"
#include "voc_common_reg.h"
#include "clk-dtv.h"   //CLK_PM

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
void voc_common_clock_set(enum voice_clock clk_bank,
					u32 reg_addr_8bit,
					u16 value,
					u32 start,
					u32 end)
{
	#define VOC_OFFSET_CLKGEN1  0x200
	u32 cpu_addr = 0;

	if (clk_bank == VOC_CLKGEN1)
		cpu_addr += VOC_OFFSET_CLKGEN1;

	cpu_addr += OFFSET8(reg_addr_8bit); //8bit mode to cpu addr

	clock_write(CLK_PM, cpu_addr, value, start, end);
}
