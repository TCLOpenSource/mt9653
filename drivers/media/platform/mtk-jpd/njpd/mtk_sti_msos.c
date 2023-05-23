// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/sched.h>

#include "mtk_sti_msos.h"
#include "mtk_drv_jpd.h"
#include "mtk_reg_jpd.h"
#include "mtk_hal_jpd.h"

#define NJPD_MINSIZEFORVMALLOC 4096

void reg16_write(REG16 __iomem *offset, __u16 value)
{
	writew(value, offset);
}

__u16 reg16_read(REG16 __iomem *offset)
{
	return readw(offset);
}

void reg8_write(REG16 __iomem *offset, __u8 value)
{
	writeb(value, offset);
}

__u8 reg8_read(REG16 __iomem *offset)
{
	return readb(offset);
}

void reg8_mask_write(REG16 __iomem *offset, __u8 mask, __u8 value)
{
	__u8 temp;

	temp = readb(offset);
	temp &= ~mask;
	temp |= (mask & value);
	writeb(temp, offset);
}


void STI_NJPD_PowerOn(void)
{
	long round_rate;
	int ret;

	round_rate = clk_round_rate(jpd_bank_offsets->clk_jpd, ULONG_MAX);
	if (round_rate < 0)
		pr_err("[%s][%d] clk_jpd set round rate fail\n", __func__, __LINE__);

	if (jpd_bank_offsets->sw_en_smi2jpd) {
		ret = clk_prepare_enable(jpd_bank_offsets->sw_en_smi2jpd);
		if (ret) {
			pr_err("[%s][%d] sw_en_smi2jpd enable failed\n",
					__func__, __LINE__);
			clk_disable_unprepare(jpd_bank_offsets->sw_en_smi2jpd);
			return;
		}
	}
	if (jpd_bank_offsets->clk_njpd2jpd) {
		ret = clk_prepare_enable(jpd_bank_offsets->clk_njpd2jpd);
		if (ret) {
			pr_err("[%s][%d] njpd2jpd enable failed\n",
					__func__, __LINE__);
			clk_disable_unprepare(jpd_bank_offsets->clk_njpd2jpd);
			return;
		}
	}
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void STI_NJPD_PowerOff(void)
{
	if (jpd_bank_offsets->sw_en_smi2jpd)
		clk_disable_unprepare(jpd_bank_offsets->sw_en_smi2jpd);

	if (jpd_bank_offsets->clk_njpd2jpd)
		clk_disable_unprepare(jpd_bank_offsets->clk_njpd2jpd);

}

void *STI_MALLOC(__u32 size)
{
	if (size < NJPD_MINSIZEFORVMALLOC)
		return kmalloc(size, GFP_KERNEL);
	else
		return vmalloc(size);
}

void STI_FREE(void *x, __u32 size)
{
	if (size < NJPD_MINSIZEFORVMALLOC)
		kfree(x);
	else
		vfree(x);
}

void MsOS_DelayTask(__u32 u32Ms)
{
	msleep((u32Ms));
}

void MsOS_DelayTaskUs(__u32 u32Us)
{
	if (u32Us < 10UL)
		udelay(u32Us);
	else if (u32Us < 20UL * 1000UL)
		usleep_range(u32Us, u32Us);
	else
		msleep_interruptible((unsigned int)(u32Us / 1000UL));
}


__u32 MsOS_GetSystemTime(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void MsOS_FlushMemory(void)
{

}
