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
#include "plat_mt5879_reg.h"
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
static void __iomem *reg_0x102a;

static void ioremap_uninit(void)
{
	if (reg_ckgen00)
		iounmap(reg_ckgen00);
	if (reg_0x1007)
		iounmap(reg_0x1007);
	if (reg_0x102a)
		iounmap(reg_0x102a);
}

static int ioremap_init(void)
{
	reg_ckgen00 = ioremap_nocache(RIU2IOMAP(CKGEN00, 0), PAGE_SIZE);

	reg_0x1007 = ioremap_nocache(RIU2IOMAP(0x1007, 0), PAGE_SIZE);
	reg_0x102a = ioremap_nocache(RIU2IOMAP(0x102a, 0), PAGE_SIZE);
	if (!reg_ckgen00
		|| !reg_0x1007
		|| !reg_0x102a)
		return -ENOMEM;

	return 0;
}
#endif

static void mt5879_clk_disable(struct apu_clk_gp *aclk)
{
	//cg/sw enable
}

static unsigned long curr_rate;

static unsigned long mt5879_clk_get_rate(struct apu_clk_gp *aclk)
{
	return curr_rate;
}

static void apupll_set_rate(void)
{
	riu_write16(0x1007, 0x44, 0x0000);
	riu_write16(0x1007, 0x46, 0x0400);
	riu_write16(0x1007, 0x47, 0x0000);
	riu_write16(0x1007, 0x48, 0x002b);
	riu_write16(0x1007, 0x4e, 0x1010);
	riu_write16(0x1007, 0x45, 0x0100);
	riu_write16(0x1007, 0x42, 0x0000);
	riu_write16(0x1007, 0x41, 0x0001);
	riu_write16(0x1007, 0x40, 0x1110);
	udelay(500);
	riu_write16(0x1007, 0x41, 0x0000);
	riu_write16(0x1007, 0x40, 0x0000);
}

static int mt5879_clk_set_rate(struct apu_clk_gp *aclk, unsigned long rate)
{
	struct apu_clk *dst;

	mutex_lock(&aclk->clk_lock);
	if (rate != curr_rate)
		apupll_set_rate();

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
	u16 regv;

	// init clk mdla
	regv = riu_read16(ckgen00, 0x40);
	regv &= ~0x0700;
	regv |= 0x0400; // bit [10:8]: reg_ckg_mdla_aia[2:0] = 0x4 (sel = 1)
	riu_write16(ckgen00, 0x40, regv);

	// init clk bus, clk_tcm
	regv = riu_read16(0x102a, 0x3f);
	regv |= 0x0008; // bit [3]: reg_sw_en_smi2aiatop = 1
	riu_write16(0x102a, 0x3f, regv);

	// init ssc2_smi_ck
	regv = riu_read16(0x102a, 0x39);
	regv |= 0x0001; // bit [0]: reg_sw_en_smi2aia_smi = 1
	riu_write16(0x102a, 0x39, regv);

	// init clk imi
	regv = riu_read16(ckgen00, 0x44);
	regv &= ~0x0003; // bit [1:0]: reg_ckg_emi2aia[1:0] = 0
	riu_write16(ckgen00, 0x44, regv);

	regv = riu_read16(0x102a, 0x2b);
	regv |= 0x0002; // bit [1:1]: reg_sw_en_emi22xpu = 1
	riu_write16(0x102a, 0x2b, regv);
}

static int mt5879_clk_enable(struct apu_clk_gp *aclk)
{
	unsigned long rate;

	if (!aclk->top_pll)
		return -EINVAL;

	rate = curr_rate ? curr_rate : aclk->top_pll->def_freq;
	mt5879_clk_set_rate(aclk, rate);
	clk_mux_enable();

	aclk_info(aclk->dev, "%s: top_pll: rate: %lu\n",
			__func__, rate);

	return 0;
}

static struct apu_clk mt5879_mdla0_topmux = {
	.always_on = 1,
	.fix_rate = 1,
};

static struct apu_cg mt5879_mdla0_cg[] = {
	{
		.phyaddr = 0x19038000,
		.cg_ctl = {0, 4, 8},
	},
};

static struct apu_cgs mt5879_mdla0_cgs = {
	.cgs = &mt5879_mdla0_cg[0],
	.clk_num = ARRAY_SIZE(mt5879_mdla0_cg),
};

static struct apu_clk_ops mt5879_mdla0_clk_ops = {
	.prepare	= dtv_apu_dummy_clk_prepare,
	.unprepare	= dtv_apu_dummy_clk_unprepare,
	.enable		= mt5879_clk_enable,
	.disable	= mt5879_clk_disable,
	.cg_enable	= dtv_apu_dummy_cg_enable,
	.cg_status	= dtv_apu_dummy_cg_status,
	.set_rate	= mt5879_clk_set_rate,
	.get_rate	= mt5879_clk_get_rate,
};

static struct apu_clk_ops mt5879_mdla_clk_ops = {
	.prepare	= dtv_apu_dummy_clk_prepare,
	.unprepare	= dtv_apu_dummy_clk_unprepare,
	.enable		= mt5879_clk_enable,
	.disable	= mt5879_clk_disable,
	.cg_enable	= dtv_apu_dummy_cg_enable,
	.cg_status	= dtv_apu_dummy_cg_status,
	.set_rate	= mt5879_clk_set_rate,
	.get_rate	= mt5879_clk_get_rate,
};

static struct apu_clk_gp mt5879_mdla0_clk_gp = {
	.ops = &mt5879_mdla0_clk_ops,
};

static struct apu_clk_gp mt5879_mdla_clk_gp = {
	.top_mux = &mt5879_mdla0_topmux,
	.cg = &mt5879_mdla0_cgs,
	.ops = &mt5879_mdla_clk_ops,
};

static const struct apu_clk_array apu_clk_gps[] = {
	[MDLA] = { .name = "mt5879_mdla", .aclk_gp = &mt5879_mdla_clk_gp },
	[MDLA0] = { .name = "mt5879_mdla0", .aclk_gp = &mt5879_mdla0_clk_gp },
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

void mt5879_clk_suspend(struct apu_dev *ad)
{
	curr_rate = 0;
}

int dtv_apu_mt5879_clk_init(struct apu_dev *ad)
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

void dtv_apu_mt5879_clk_uninit(struct apu_dev *ad)
{
	struct apu_clk *dst;

	ioremap_uninit();
	dst = ad->aclk->top_pll;
	kfree(dst);
}
