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
#include "plat_util.h"

#define TIMER_DEFAULT_TIMEOUT	5000

struct apu_power_plat_handle *power_hnd;

static void power_delay_task(struct work_struct *work)
{
	if (power_hnd && power_hnd->delay_task)
		power_hnd->delay_task(power_hnd->task_data);
}

static void power_timeup(struct timer_list *tlist)
{
	//use kwork job to prevent power off at irq
	schedule_work(&power_hnd->power_work);
}

void power_delay_task_start(struct apu_dev *ad, int enable)
{
	if (power_hnd == NULL) {
		apupwr_debug("%s(): not init", __func__);
		return;
	}

	if (timer_pending(&power_hnd->power_timer))
		del_timer(&power_hnd->power_timer);

	if (enable) {
		power_hnd->ad = ad;

		power_hnd->power_timer.expires = jiffies +
			msecs_to_jiffies(power_hnd->delay_time);

		add_timer(&power_hnd->power_timer);
	}
}

int dtv_apu_power_util_init(struct apu_dev *ad)
{
	if (power_hnd)
		return 0;

	power_hnd = kzalloc(sizeof(*power_hnd), GFP_KERNEL);
	if (power_hnd == NULL)
		return -ENOMEM;

	if (of_property_read_u32(ad->dev->of_node,
		"power_ctrl", &power_hnd->power_ctrl))
		apupwr_debug("%s(): property \"power_ctrl\" not find\n",
				__func__);

	mutex_init(&power_hnd->power_mutex);
	INIT_WORK(&power_hnd->power_work, power_delay_task);
	power_hnd->delay_time = TIMER_DEFAULT_TIMEOUT;
	timer_setup(&power_hnd->power_timer,
		power_timeup, TIMER_DEFERRABLE);

	return 0;
}

void dtv_apu_power_util_uninit(struct apu_dev *ad)
{
	if (power_hnd == NULL)
		return;

	if (timer_pending(&power_hnd->power_timer))
		del_timer(&power_hnd->power_timer);

	kfree(power_hnd);
	power_hnd = NULL;
}

int dtv_dummy_misc_init(struct apu_dev *ad)
{
	aprobe_info(ad->dev, "%s():\n", __func__);
	return 0;
}

int dtv_dummy_get_target_freq(struct apu_dev *ad)
{
	return 0;
}

int dtv_dummy_mtcmos_on(struct apu_dev *ad, int force)
{
	arpc_info(ad->dev, "%s():\n", __func__);
	return 0;
}

void dtv_dummy_mtcmos_off(struct apu_dev *ad, int force)
{
	arpc_info(ad->dev, "%s():\n", __func__);
}

int dtv_apu_devfreq_init(struct apu_dev *ad, struct devfreq_dev_profile *pf, void *data)
{
	struct apu_gov_data *pgov_data;
	const char *gov_name;
	int err = 0;
	int max_st = 0;
	int i;

	pgov_data = apu_gov_init(ad->dev, pf, &gov_name);
	if (IS_ERR(pgov_data)) {
		err = PTR_ERR(pgov_data);
		goto out;
	}

	ad->df = devm_devfreq_add_device(ad->dev, pf, gov_name, pgov_data);
	if (IS_ERR(ad->df)) {
		err = PTR_ERR(ad->df);
		goto out;
	}

	max_st = ad->df->profile->max_state - 1;
	for (i = 0; i <= max_st; i++) {
		aprobe_info(ad->dev, "opp%d -> freq: %lu\n",
			i, ad->df->profile->freq_table[max_st - i]);
	}

	err = apu_gov_setup(ad, data);
out:
	return err;
}

void dtv_apu_devfreq_uninit(struct apu_dev *ad)
{
	struct apu_gov_data *pgov_data = NULL;

	pgov_data = ad->df->data;
	apu_gov_unsetup(ad);
	/* remove devfreq device */
	devm_devfreq_remove_device(ad->dev, ad->df);
	devm_kfree(ad->dev, pgov_data);
}

int dtv_apu_dummy_clk_prepare(struct apu_clk_gp *aclk)
{
	aclk_info(aclk->dev, "%s():\n", __func__);
	return 0;
}

void dtv_apu_dummy_clk_unprepare(struct apu_clk_gp *aclk)
{
	aclk_info(aclk->dev, "%s():\n", __func__);
}

int dtv_apu_dummy_clk_enable(struct apu_clk_gp *aclk)
{
	aclk_info(aclk->dev, "%s():\n", __func__);
	return 0;
}

void dtv_apu_dummy_clk_disable(struct apu_clk_gp *aclk)
{
	aclk_info(aclk->dev, "%s():\n", __func__);
}

int dtv_apu_dummy_cg_enable(struct apu_clk_gp *aclk)
{
	aclk_info(aclk->dev, "%s():\n", __func__);
	return 0;
}

int dtv_apu_dummy_cg_status(struct apu_clk_gp *aclk, u32 *result)
{
	aclk_info(aclk->dev, "%s():\n", __func__);
	return 0;
}

int dtv_apu_dummy_clk_init(struct apu_dev *ad)
{
	aprobe_info(ad->dev, "%s():\n", __func__);
	return 0;
}

void dtv_apu_dummy_clk_uninit(struct apu_dev *ad)
{
	aprobe_info(ad->dev, "%s():\n", __func__);
}

int dtv_apu_dummy_set_rate(struct apu_clk_gp *aclk, unsigned long rate)
{
	aclk_info(aclk->dev, "%s():\n", __func__);
	return 0;
}

unsigned long dtv_apu_dummy_get_rate(struct apu_clk_gp *aclk)
{
	aclk_info(aclk->dev, "%s():\n", __func__);
	return 0;
}

void dtv_apu_dummy_suspend(struct apu_dev *ad)
{
	arpc_info(ad->dev, "%s():\n", __func__);
}

void dtv_apu_dummy_resume(struct apu_dev *ad)
{
	arpc_info(ad->dev, "%s():\n", __func__);
}
