// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm_opp.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/pm_runtime.h>

#include "apu_log.h"
#include "apu_plat.h"
#include "apu_regulator.h"
#include "apu_devfreq.h"
#include "apu_clk.h"
#include "apusys_power.h"
#include "apu_common.h"
#include "apu_dbg.h"
#include "apu_plat.h"
#include "plat_regulator.h"
#include "plat_util.h"

static struct apu_plat_ops dtv_plat_driver[] = {
	{
		.name = "mt5896_platops",
		.init_misc =		dtv_apu_mt5896_misc_init,
		.uninit_misc =		dtv_apu_mt5896_misc_uninit,
		.init_opps =		dtv_apu_mt5896_opp_init,
		.uninit_opps =		dtv_apu_mt5896_opp_uninit,
		.init_clks =		dtv_apu_mt5896_clk_init,
		.uninit_clks =		dtv_apu_mt5896_clk_uninit,
		.init_rguls =		dtv_apu_regulator_init,
		.uninit_rguls =		dtv_apu_regulator_uninit,
		.init_devfreq =		dtv_apu_devfreq_init,
		.uninit_devfreq =	dtv_apu_devfreq_uninit,
		.mtcmos_on =		dtv_apu_mt589x_mtcmos_on,
		.mtcmos_off =		dtv_apu_mt589x_mtcmos_off,
		.init_mtcmos =		mt589x_mtcmos_init,
		.uninit_mtcmos =	mt589x_mtcmos_uninit,
		.get_target_freq =	dtv_apu_mt5896_get_target_freq,
		.suspend =		dtv_apu_mt5896_suspend,
		.resume =		dtv_apu_mt5896_resume,
		.power_status =		dtv_apu_mt589x_power_status,
	},
	{
		.name = "mt5896_e3_platops",
		.init_misc =		dtv_apu_mt5896_misc_init,
		.uninit_misc =		dtv_apu_mt5896_misc_uninit,
		.init_opps =		dtv_apu_mt5896_e3_opp_init,
		.uninit_opps =		dtv_apu_mt5896_e3_opp_uninit,
		.init_clks =		dtv_apu_mt5896_e3_clk_init,
		.uninit_clks =		dtv_apu_mt5896_e3_clk_uninit,
		.init_rguls =		dtv_apu_regulator_init,
		.uninit_rguls =		dtv_apu_regulator_uninit,
		.init_devfreq =		dtv_apu_devfreq_init,
		.uninit_devfreq =	dtv_apu_devfreq_uninit,
		.mtcmos_on =		dtv_apu_mt589x_mtcmos_on,
		.mtcmos_off =		dtv_apu_mt589x_mtcmos_off,
		.init_mtcmos =		mt589x_mtcmos_init,
		.uninit_mtcmos =	mt589x_mtcmos_uninit,
		.get_target_freq =	dtv_apu_mt5896_get_target_freq,
		.suspend =		dtv_apu_mt5896_e3_suspend,
		.resume =		dtv_apu_mt5896_e3_resume,
		.power_status =		dtv_apu_mt589x_power_status,
	},
	{
		.name = "mt5897_platops",
		.init_misc =		dtv_apu_mt5897_misc_init,
		.uninit_misc =		dtv_apu_mt5897_misc_uninit,
		.init_opps =		dtv_apu_mt5897_opp_init,
		.uninit_opps =		dtv_apu_mt5897_opp_uninit,
		.init_clks =		dtv_apu_mt5897_clk_init,
		.uninit_clks =		dtv_apu_mt5897_clk_uninit,
		.init_rguls =		dtv_apu_regulator_init,
		.uninit_rguls =		dtv_apu_regulator_uninit,
		.init_devfreq =		dtv_apu_devfreq_init,
		.uninit_devfreq =	dtv_apu_devfreq_uninit,
		.mtcmos_on =		dtv_apu_mt589x_mtcmos_on,
		.mtcmos_off =		dtv_apu_mt589x_mtcmos_off,
		.init_mtcmos =		mt589x_mtcmos_init,
		.uninit_mtcmos =	mt589x_mtcmos_uninit,
		.get_target_freq =	dtv_apu_mt5897_get_target_freq,
		.suspend =		dtv_apu_mt5897_suspend,
		.resume =		dtv_apu_mt5897_resume,
		.power_status =		dtv_apu_mt589x_power_status,
	},
	{
		.name = "mt5879_platops",
		.init_misc =		dtv_apu_mt5879_misc_init,
		.uninit_misc =		dtv_apu_mt5879_misc_uninit,
		.init_opps =		dtv_apu_mt5879_opp_init,
		.uninit_opps =		dtv_apu_mt5879_opp_uninit,
		.init_clks =		dtv_apu_mt5879_clk_init,
		.uninit_clks =		dtv_apu_mt5879_clk_uninit,
		.init_rguls =		dtv_apu_regulator_init,
		.uninit_rguls =		dtv_apu_regulator_uninit,
		.init_devfreq =		dtv_apu_devfreq_init,
		.uninit_devfreq =	dtv_apu_devfreq_uninit,
		.mtcmos_on =		dtv_apu_mt5879_mtcmos_on,
		.mtcmos_off =		dtv_apu_mt5879_mtcmos_off,
		.init_mtcmos =		mt5879_mtcmos_init,
		.uninit_mtcmos =	mt5879_mtcmos_uninit,
		.get_target_freq =	dtv_apu_mt5879_get_target_freq,
		.suspend =		dtv_apu_mt5879_suspend,
		.resume =		dtv_apu_mt5879_resume,
		.power_status =		dtv_apu_mt5879_power_status,
	},
};

struct apu_plat_ops *dtv_apupwr_get_ops(struct apu_dev *ad, const char *name)
{
	int i = 0;

	if (!name)
		goto out;

	for (i = 0; i < ARRAY_SIZE(dtv_plat_driver); i++) {
		if (strcmp(name, dtv_plat_driver[i].name) == 0) {
			aprobe_err(ad->dev, "[%s] find plat %s\n", __func__, name);
			return &dtv_plat_driver[i];
		}
	}
	aprobe_err(ad->dev, "[%s] not find plat %s\n", __func__, name);

out:
	return ERR_PTR(-ENOENT);
}

int dtv_apupwr_get_pwrctrl(void)
{
	return power_hnd->power_ctrl ? (int)power_hnd->power_ctrl : -1;
}

int dtv_apupwr_set_pwrctrl(int power_ctrl, int delay)
{
	if (power_ctrl)
		power_hnd->power_ctrl |= APU_POWER_CTRL_DEFAULT_ON;

	return 0;
}

