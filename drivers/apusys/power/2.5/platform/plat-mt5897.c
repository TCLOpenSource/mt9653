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

#include "apusys_power_user.h"
#include "apu_devfreq.h"
#include "apu_io.h"
#include "apu_log.h"
#include "apu_gov.h"
#include "apu_common.h"
#include "apu_regulator.h"
#include "apu_dbg.h"
#include "apu_plat.h"
#include "apu_of.h"
#include "apu_clk.h"
#include "plat_util.h"
#include "plat_mt589x_reg.h"
#include "plat_regulator.h"

static struct notifier_block pm_suspend_notifier;
static int suspend_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
	{
		pr_info("[MDLA] %s(): SUSPEND_PREPARE\n", __func__);
		break;
	}
	default:
		break;
	}
	return NOTIFY_DONE;
}

int dtv_apu_mt5897_opp_init(struct apu_dev *ad)
{
	int ret = 0;

	ret = dev_pm_opp_of_add_table_indexed(ad->dev, 0);
	if (ret)
		goto out;

	ad->oppt = dev_pm_opp_get_opp_table(ad->dev);
	if (IS_ERR_OR_NULL(ad->oppt)) {
		ret = PTR_ERR(ad->oppt);
		aprobe_err(ad->dev, "[%s] get opp table fail, ret = %d\n", __func__, ret);
		goto out;
	}

out:
	return ret;
}

void dtv_apu_mt5897_opp_uninit(struct apu_dev *ad)
{
	if (!IS_ERR_OR_NULL(ad->oppt))
		dev_pm_opp_put_opp_table(ad->oppt);
}

int dtv_apu_mt5897_get_target_freq(struct apu_dev *ad)
{
	return apu_opp2freq(ad, 0);
}

void dtv_apu_mt5897_suspend(struct apu_dev *ad)
{
	power_hnd->init_once = 0;
	mt589x_mtcmos_suspend(ad);
	mt5897_clk_suspend(ad);
}

void dtv_apu_mt5897_resume(struct apu_dev *ad)
{
	int ret;

	if ((power_hnd->power_ctrl & APU_POWER_CTRL_DEFAULT_ON) &&
					(power_hnd->init_once == 0)) {
		/* restore previous pll/clk settings */
		if (!IS_ERR_OR_NULL(ad->aclk)) {
			ret = ad->aclk->ops->enable(ad->aclk);
			if (ret) {
				apower_err(ad->dev, "%s(): clk_enable, ret = %x\n",
							__func__, ret);
				return;
			}
		}

		if (ad->plat_ops->mtcmos_on) {
			ret = ad->plat_ops->mtcmos_on(ad, 0);
			if (ret) {
				apower_err(ad->dev, "%s(): mtcmos_on, ret = %x\n",
							__func__, ret);
				return;
			}
		}
		power_hnd->init_once++;
	}
}

static void power_on_delay_task(void *data)
{
	struct apu_dev *ad = power_hnd->ad;
	static u32 retry_cnt;
	int ret;

	retry_cnt++;

	if (power_hnd->power_ctrl & APU_POWER_CTRL_DEFAULT_ON) {
		if (ad->plat_ops->init_mtcmos) {
			ret = ad->plat_ops->init_mtcmos(ad);
			if (ret) {
				apower_err(ad->dev, "%s(): init_mtcmos fail\n", __func__);
				goto retry;
			}
		}

		if (ad->plat_ops->mtcmos_on) {
			ret = ad->plat_ops->mtcmos_on(ad, 0);
			if (ret) {
				apower_err(ad->dev, "%s(): mtcmos_on fail\n", __func__);
				goto retry;
			}
		}

	}

	apower_err(ad->dev, "%s(): retry_count:%x, success\n", __func__, retry_cnt);
	return;
retry:
	apower_err(ad->dev, "%s(): retry_count:%x, fail\n", __func__, retry_cnt);
	power_delay_task_start(ad, 1);
}

int dtv_apu_mt5897_misc_init(struct apu_dev *ad)
{
	unsigned long volt = 0;
	unsigned long rate = 0;
	int ret = 0;

	if (!power_hnd && dtv_apu_power_util_init(ad))
		return -EFAULT;

	if (!power_hnd->init_once) {
		ret = apu_get_recommend_freq_volt(ad->dev, &rate, &volt, 0);
		if (ret) {
			aprobe_err(ad->dev, "%s(): get freq/volt fail\n", __func__);
			return -EFAULT;
		}

		if (!IS_ERR_OR_NULL(ad->aclk)) {
			ret = ad->aclk->ops->set_rate(ad->aclk, rate);
			if (ret) {
				aprobe_err(ad->dev, "%s(): set clk fail\n", __func__);
				return -EFAULT;
			}

			ret = ad->aclk->ops->enable(ad->aclk);
			if (ret) {
				aprobe_err(ad->dev, "%s(): enable clk fail\n", __func__);
				return -EFAULT;
			}
		}

		if (!IS_ERR_OR_NULL(ad->argul)) {
			ret = ad->argul->ops->enable(ad->argul);
			if (ret)
				aprobe_err(ad->dev, "%s(): enable regulator fail\n", __func__);

			ret = ad->argul->ops->set_voltage(ad->argul, volt, volt);
			if (ret)
				aprobe_err(ad->dev, "%s(): set regulator fail\n", __func__);
		}

		if (power_hnd->power_ctrl & APU_POWER_CTRL_DEFAULT_ON) {
			aprobe_info(ad->dev, "%s(): power_ctrl:%x -> default on\n",
				__func__, power_hnd->power_ctrl);
			if (ad->plat_ops->init_mtcmos) {
				/* ignore init_mtcmos fail here, we will re-init at user ioctl */
				ret = ad->plat_ops->init_mtcmos(ad);
				if (ret)
					aprobe_err(ad->dev, "%s(): skip init_mtcmos\n", __func__);
			}

			if (ad->plat_ops->mtcmos_on) {
				ret = ad->plat_ops->mtcmos_on(ad, 0);
				if (ret) {
					power_hnd->delay_task = power_on_delay_task;
					power_delay_task_start(ad, 1);
				}
			}
		}

		pm_suspend_notifier.notifier_call =
			suspend_event;
		//pm_suspend_notifier.priority = 99;
		register_pm_notifier(&pm_suspend_notifier);
		power_hnd->init_once = 1;
	}

	aprobe_info(ad->dev, "%s(): done, ret = %d\n", __func__, ret);

	return 0;
}

void dtv_apu_mt5897_misc_uninit(struct apu_dev *ad)
{
	power_delay_task_start(ad, 0);

	if (power_hnd->power_ctrl & APU_POWER_CTRL_DEFAULT_ON) {
		if (ad->plat_ops->mtcmos_off)
			ad->plat_ops->mtcmos_off(ad, 1);
	}

	if (!IS_ERR_OR_NULL(ad->aclk))
		ad->aclk->ops->disable(ad->aclk);
}
