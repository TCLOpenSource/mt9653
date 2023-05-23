/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __APW_PLAT_UTIL_H__
#define __APW_PLAT_UTIL_H__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include "apu_log.h"
#include "apu_plat.h"
#include "apu_regulator.h"
#include "apu_devfreq.h"
#include "apu_clk.h"
#include "apusys_power.h"
#include "apu_common.h"
#include "apu_dbg.h"

//#define BOOTUP_INIT_POWER
//#define DEBUG_POWER_FLOW

// should define/undefine it at ree/tee driver together
#define MDLA_POWER_CTRL_BY_TEE

#define APMCU_RIUBASE 0x1C000000
#define RIU_ADDR(bank, offset)		(APMCU_RIUBASE + ((bank<<8)<<1) + (offset<<2))

#define APU_POWER_CTRL_DEFAULT_ON	(0x1 << 0)

struct apu_power_plat_handle {
	int init_once;
	u32 power_ctrl;
	struct mutex power_mutex;
	int delay_time;
	struct work_struct power_work;
	struct timer_list power_timer;
	void (*delay_task)(void *data);
	void *task_data;
	void *plat_data;
	struct apu_dev *ad;
};

extern struct apu_power_plat_handle *power_hnd;

static inline u32 riu_read32(u32 bank, u32 offset)
{
	void *addr;
	u32 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread32(addr);
	iounmap(addr);

	return regv;
}

static inline void riu_write32(u32 bank, u32 offset, u32 value)
{
	void *addr;

#ifdef DEBUG_POWER_FLOW
	pr_debug("[TRACE] %s: bank:0x%04x offset:0x%04x value:0x%04x",
			__func__, bank, offset, value);
#endif
	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	iowrite32(value, addr);
	iounmap(addr);
}

static inline u16 riu_read16(u32 bank, u32 offset)
{
	void *addr;
	u16 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread16(addr);
	iounmap(addr);

	return regv;
}

static inline void riu_write16(u32 bank, u32 offset, u16 value)
{
	void *addr;

#ifdef DEBUG_POWER_FLOW
	pr_debug("[TRACE] %s: bank:0x%04x offset:0x%04x value:0x%04x",
			__func__, bank, offset, value);
#endif
	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	iowrite16(value, addr);
	iounmap(addr);
}

static inline u8 riu_read8(u32 bank, u32 offset)
{
	void *addr;
	u8 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread8(addr);
	iounmap(addr);

	return regv;
}

// util functions
void power_delay_task_start(struct apu_dev *ad, int enable);
int dtv_apu_power_util_init(struct apu_dev *ad);
void dtv_apu_power_util_uninit(struct apu_dev *ad);

// dummy functions
int dtv_dummy_misc_init(struct apu_dev *ad);
int dtv_dummy_get_target_freq(struct apu_dev *ad);
int dtv_dummy_mtcmos_on(struct apu_dev *ad, int force);
void dtv_dummy_mtcmos_off(struct apu_dev *ad, int force);
int dtv_apu_devfreq_init(struct apu_dev *ad, struct devfreq_dev_profile *pf, void *data);
void dtv_apu_devfreq_uninit(struct apu_dev *ad);
int dtv_apu_dummy_clk_prepare(struct apu_clk_gp *aclk);
void dtv_apu_dummy_clk_unprepare(struct apu_clk_gp *aclk);
int dtv_apu_dummy_clk_enable(struct apu_clk_gp *aclk);
void dtv_apu_dummy_clk_disable(struct apu_clk_gp *aclk);
int dtv_apu_dummy_cg_enable(struct apu_clk_gp *aclk);
int dtv_apu_dummy_cg_status(struct apu_clk_gp *aclk, u32 *result);
int dtv_apu_dummy_clk_init(struct apu_dev *ad);
void dtv_apu_dummy_clk_uninit(struct apu_dev *ad);
int dtv_apu_dummy_set_rate(struct apu_clk_gp *aclk, unsigned long rate);
unsigned long dtv_apu_dummy_get_rate(struct apu_clk_gp *aclk);
void dtv_apu_dummy_suspend(struct apu_dev *ad);
void dtv_apu_dummy_resume(struct apu_dev *ad);

void mt589x_mtcmos_suspend(struct apu_dev *ad);

int mt589x_mtcmos_init(struct apu_dev *ad);
void mt589x_mtcmos_uninit(struct apu_dev *ad);

unsigned long dtv_apu_mt589x_power_status(void);

// mt5896 plat_ops
int dtv_apu_mt5896_clk_init(struct apu_dev *ad);
void dtv_apu_mt5896_clk_uninit(struct apu_dev *ad);

int dtv_apu_mt5896_opp_init(struct apu_dev *ad);
void dtv_apu_mt5896_opp_uninit(struct apu_dev *ad);

int dtv_apu_mt5896_misc_init(struct apu_dev *ad);
void dtv_apu_mt5896_misc_uninit(struct apu_dev *ad);

int dtv_apu_mt589x_mtcmos_on(struct apu_dev *ad, int force);
void dtv_apu_mt589x_mtcmos_off(struct apu_dev *ad, int force);

int dtv_apu_mt5896_get_target_freq(struct apu_dev *ad);

void mt5896_clk_suspend(struct apu_dev *ad);
void dtv_apu_mt5896_suspend(struct apu_dev *ad);
void dtv_apu_mt5896_resume(struct apu_dev *ad);

// mt5897 plat_ops
int dtv_apu_mt5897_clk_init(struct apu_dev *ad);
void dtv_apu_mt5897_clk_uninit(struct apu_dev *ad);

int dtv_apu_mt5897_opp_init(struct apu_dev *ad);
void dtv_apu_mt5897_opp_uninit(struct apu_dev *ad);

int dtv_apu_mt5897_misc_init(struct apu_dev *ad);
void dtv_apu_mt5897_misc_uninit(struct apu_dev *ad);

int dtv_apu_mt5897_get_target_freq(struct apu_dev *ad);

void mt5897_clk_suspend(struct apu_dev *ad);
void dtv_apu_mt5897_suspend(struct apu_dev *ad);
void dtv_apu_mt5897_resume(struct apu_dev *ad);

// mt5879 plat_ops
int dtv_apu_mt5879_clk_init(struct apu_dev *ad);
void dtv_apu_mt5879_clk_uninit(struct apu_dev *ad);

int dtv_apu_mt5879_opp_init(struct apu_dev *ad);
void dtv_apu_mt5879_opp_uninit(struct apu_dev *ad);

int dtv_apu_mt5879_misc_init(struct apu_dev *ad);
void dtv_apu_mt5879_misc_uninit(struct apu_dev *ad);

int dtv_apu_mt5879_mtcmos_on(struct apu_dev *ad, int force);
void dtv_apu_mt5879_mtcmos_off(struct apu_dev *ad, int force);

int mt5879_mtcmos_init(struct apu_dev *ad);
void mt5879_mtcmos_uninit(struct apu_dev *ad);

int dtv_apu_mt5879_get_target_freq(struct apu_dev *ad);

void mt5879_mtcmos_suspend(struct apu_dev *ad);
void mt5879_clk_suspend(struct apu_dev *ad);
void dtv_apu_mt5879_suspend(struct apu_dev *ad);
void dtv_apu_mt5879_resume(struct apu_dev *ad);

unsigned long dtv_apu_mt5879_power_status(void);

// mt5896_e3 plat_ops
int dtv_apu_mt5896_e3_clk_init(struct apu_dev *ad);
void dtv_apu_mt5896_e3_clk_uninit(struct apu_dev *ad);
void mt5896_e3_clk_suspend(struct apu_dev *ad);
void dtv_apu_mt5896_e3_suspend(struct apu_dev *ad);
void dtv_apu_mt5896_e3_resume(struct apu_dev *ad);
int dtv_apu_mt5896_e3_opp_init(struct apu_dev *ad);
void dtv_apu_mt5896_e3_opp_uninit(struct apu_dev *ad);

#endif
