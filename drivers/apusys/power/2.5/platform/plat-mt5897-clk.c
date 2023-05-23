// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/iopoll.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>

#include "apusys_power_user.h"
#include "apu_devfreq.h"
#include "apu_clk.h"
#include "apu_io.h"
#include "apu_log.h"
#include "apu_common.h"
#include "apu_dbg.h"
#include "plat_mt589x_reg.h"
#include "plat_util.h"

static DEFINE_MUTEX(hw_lock);

#ifdef DEBUG_POWER_FLOW

static u32 ckgen00 = CKGEN00;

static void ioremap_uninit(void)
{
}

static int ioremap_init(void)
{
	return 0;
}
#else

#define riu_read16(name, offset)	\
		ioread16(reg_##name + (offset << 2))

#define riu_write16(name, offset, val)	\
		iowrite16(val, reg_##name + (offset << 2))

#define riu_read32(name, offset)	\
		ioread32(reg_##name + (offset << 2))

#define riu_write32(name, offset, val)	\
		iowrite32(val, reg_##name + (offset << 2))

#define RIU2IOMAP(bank, offset)	((phys_addr_t)RIU_ADDR(bank, offset))

static void __iomem *reg_ckgen00;
static void __iomem *reg_0x1007;
static void __iomem *reg_0x100c;
static void __iomem *reg_0x102a;

static void ioremap_uninit(void)
{
	if (reg_ckgen00)
		iounmap(reg_ckgen00);
	if (reg_0x1007)
		iounmap(reg_0x1007);
	if (reg_0x100c)
		iounmap(reg_0x100c);
	if (reg_0x102a)
		iounmap(reg_0x102a);
}

static int ioremap_init(void)
{
	reg_ckgen00 = ioremap_nocache(RIU2IOMAP(CKGEN00, 0), PAGE_SIZE);

	reg_0x1007 = ioremap_nocache(RIU2IOMAP(0x1007, 0), PAGE_SIZE);
	reg_0x100c = ioremap_nocache(RIU2IOMAP(0x100C, 0), PAGE_SIZE);
	reg_0x102a = ioremap_nocache(RIU2IOMAP(0x102a, 0), PAGE_SIZE);
	if (!reg_ckgen00
		|| !reg_0x1007
		|| !reg_0x100c
		|| !reg_0x102a)
		return -ENOMEM;

	return 0;
}

#endif

static u32 PLL2DIV(u32 rate)
{
	// rate in MHz
	// div = 5435817 / ((rate / 1000) *2);
	return (u32)(5435817UL * 500 / TOMHZ(rate));
}

#ifdef BUS_PLL_MACRO
static u32 BUS_PLL2DIV(u32 rate)
{
	//Freq = 24MHz * RG_GC_AIABUSPLL_LOOPDIV_SECOND /2
	return (TOMHZ(rate) * 2) / 24;
}
#endif

static void mt5897_clk_disable(struct apu_clk_gp *aclk)
{
	//cg/sw enable
}

static unsigned long curr_rate;

static unsigned long mt5897_clk_get_rate(struct apu_clk_gp *aclk)
{
	return curr_rate;
}

static void apupll_set_rate(unsigned long rate)
{
	unsigned long target;

	target = PLL2DIV(rate);

	//apu_pll
	riu_write16(ckgen00, 0x3d, 0x0003);
	riu_write16(0x100c, 0x07, 0x0404);

	//set rate to apu_pll
	riu_write16(0x100c, 0x30, target & 0xffff);
	riu_write16(0x100c, 0x31, (target >> 16) & 0xffff);
	riu_write16(0x100c, 0x32, 0x0001);
	riu_write16(0x100c, 0x01, 0x6280);
	riu_write16(0x100c, 0x02, 0x0100);
	riu_write16(0x100c, 0x03, 0x0118);
	riu_write16(0x100c, 0x04, 0x0030);
}

static void buspll_set_rate(unsigned long rate)
{
	//bus_pll: fix to 624MHz
	riu_write16(0x1007, 0x44, 0x0000);
	riu_write16(0x1007, 0x46, 0x0400);
	riu_write16(0x1007, 0x47, 0x0000);
	riu_write16(0x1007, 0x48, 0x0034);
	riu_write16(0x1007, 0x42, 0x0000);
	riu_write16(0x1007, 0x41, 0x0001);
	riu_write16(0x1007, 0x40, 0x1110);
	udelay(50); //us
	riu_write16(0x1007, 0x40, 0x1010);
}

static int mt5897_clk_set_rate(struct apu_clk_gp *aclk, unsigned long rate)
{
	struct apu_clk *dst;

	mutex_lock(&aclk->clk_lock);
	if (rate != curr_rate) {
		apupll_set_rate(rate);
		buspll_set_rate(624);
	}

	curr_rate = rate;
	dst = aclk->top_pll;
	dst->def_freq = rate;
	mutex_unlock(&aclk->clk_lock);
	aclk_info(aclk->dev,
		 "[%s] top_pll to set %luMhz\n",
		 __func__, TOMHZ(rate));

	return 0;
}

static void clk_mux_enable(void)
{
	u16 data;

	//init clk_mdla
	data = riu_read16(ckgen00, 0x40);
	data = (data & ~0x0700) | 0x0400;
	riu_write16(ckgen00, 0x40, data);

	//init clk_bus_die
	data = riu_read16(ckgen00, 0x3c);
	data = (data & ~0x0700) | 0x0400;
	riu_write16(ckgen00, 0x3c, data);

	// init clk_imi
	data = riu_read16(ckgen00, 0x44);
	data = (data & ~0x0003) | 0x0000;
	riu_write16(ckgen00, 0x44, data);

	//data = riu_read16(ckgen00, 0x52b);
	data = riu_read16(0x102a, 0x2b);
	data = (data & ~0x0002) | 0x0002;
	riu_write16(0x102a, 0x2b, data);
}

static int mt5897_clk_enable(struct apu_clk_gp *aclk)
{
	unsigned long rate;

	if (!aclk->top_pll)
		return -EINVAL;

	rate = curr_rate ? curr_rate : aclk->top_pll->def_freq;
	mt5897_clk_set_rate(aclk, rate);
	clk_mux_enable();

	aclk_info(aclk->dev, "%s: top_pll: rate: %lu\n",
			__func__, rate);

	return 0;
}

static struct apu_clk mt5897_mdla0_topmux = {
	.always_on = 1,
	.fix_rate = 1,
};

static struct apu_cg mt5897_mdla0_cg[] = {
	{
		.phyaddr = 0x19038000,
		.cg_ctl = {0, 4, 8},
	},
};

static struct apu_cgs mt5897_mdla0_cgs = {
	.cgs = &mt5897_mdla0_cg[0],
	.clk_num = ARRAY_SIZE(mt5897_mdla0_cg),
};

static struct apu_clk_ops mt5897_mdla0_clk_ops = {
	.prepare	= dtv_apu_dummy_clk_prepare,
	.unprepare	= dtv_apu_dummy_clk_unprepare,
	.enable		= mt5897_clk_enable,
	.disable	= mt5897_clk_disable,
	.cg_enable	= dtv_apu_dummy_cg_enable,
	.cg_status	= dtv_apu_dummy_cg_status,
	.set_rate	= mt5897_clk_set_rate,
	.get_rate	= mt5897_clk_get_rate,
};

static struct apu_clk_ops mt5897_mdla_clk_ops = {
	.prepare	= dtv_apu_dummy_clk_prepare,
	.unprepare	= dtv_apu_dummy_clk_unprepare,
	.enable		= mt5897_clk_enable,
	.disable	= mt5897_clk_disable,
	.cg_enable	= dtv_apu_dummy_cg_enable,
	.cg_status	= dtv_apu_dummy_cg_status,
	.set_rate	= mt5897_clk_set_rate,
	.get_rate	= mt5897_clk_get_rate,
};

static struct apu_clk_gp mt5897_mdla0_clk_gp = {
	.ops = &mt5897_mdla0_clk_ops,
};

static struct apu_clk_gp mt5897_mdla_clk_gp = {
	.top_mux = &mt5897_mdla0_topmux,
	.cg = &mt5897_mdla0_cgs,
	.ops = &mt5897_mdla_clk_ops,
};

static const struct apu_clk_array apu_clk_gps[] = {
	[MDLA] = { .name = "mt5897_mdla", .aclk_gp = &mt5897_mdla_clk_gp },
	[MDLA0] = { .name = "mt5897_mdla0", .aclk_gp = &mt5897_mdla0_clk_gp },
};

static struct apu_clk_gp *get_clkgp(struct apu_dev *ad)
{
	struct apu_clk_gp *gp = NULL;

	switch (ad->user) {
	case MDLA:
	case MDLA0:
		gp = apu_clk_gps[ad->user].aclk_gp;
		break;
	default:
		break;
	}

	return gp;
}

void mt5897_clk_suspend(struct apu_dev *ad)
{
	curr_rate = 0;
}

int dtv_apu_mt5897_clk_init(struct apu_dev *ad)
{
	ulong rate = 0;
	struct apu_clk *dst;

	ad->aclk = get_clkgp(ad);
	if (!ad->aclk) {
		aprobe_err(ad->dev,
			"%s user:%x didn't have clkgp\n", __func__, ad->user);
		return -EFAULT;
	}

	if (ioremap_init()) {
		aprobe_err(ad->dev,
			"%s ioremap init fail\n", __func__);
		return -EFAULT;

	}

	mutex_init(&(ad->aclk->clk_lock));
	ad->aclk->dev = ad->dev;

	/* get the slowest freq in opp */
	if (apu_get_recommend_freq_volt(ad->dev, &rate, NULL, 0)) {
		aprobe_err(ad->dev,
			"%s apu_get_recommend_freq_volt fail\n", __func__);
		return -EFAULT;
	}

	dst = kzalloc(sizeof(*dst), GFP_KERNEL);
	if (!dst)
		return -ENOMEM;

	dst->dynamic_alloc = 1;
	dst->def_freq = rate;
	ad->aclk->top_pll = dst;

	return 0;
}

void dtv_apu_mt5897_clk_uninit(struct apu_dev *ad)
{
	struct apu_clk *dst;

	ioremap_uninit();
	dst = ad->aclk->top_pll;
	kfree(dst);
}
