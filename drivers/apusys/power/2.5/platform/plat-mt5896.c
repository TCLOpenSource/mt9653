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
#include <linux/iio/consumer.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/ktime.h>

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

#define TEMP_THRESHOLD 85
struct iio_channel *thermal;

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

static u16 rosc(struct apu_dev *ad)
{
	riu_write16(0x129, 0x28, 0x2074);
	while (riu_read16(0x129, 0x28) & (0x1 << 13))
		;

	return riu_read16(0x129, 0x2c) & 0x3ff;
}

static u16 sidd(struct apu_dev *ad)
{
	riu_write16(0x129, 0x28, 0x2074);
	while (riu_read16(0x129, 0x28) & (0x1 << 13))
		;

	return (riu_read16(0x129, 0x2d)  & 0xf) << 6
		| ((riu_read16(0x129, 0x2c) >> 10) & 0x3f);
}

int dtv_apu_mt5896_opp_init(struct apu_dev *ad)
{
	int ret = 0;
	int table_idx;

	if (rosc(ad) <= 0x91 || sidd(ad) <= 0x10)
		table_idx = 0;
	else
		table_idx = 1;

	aprobe_info(ad->dev, "ROSC/%d/SIDD/%d, table:%x(%s)\n",
				rosc(ad),
				sidd(ad),
				table_idx,
				table_idx ? "FF" : "SS");
	ret = dev_pm_opp_of_add_table_indexed(ad->dev, table_idx);
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

void dtv_apu_mt5896_opp_uninit(struct apu_dev *ad)
{
	if (!IS_ERR_OR_NULL(ad->oppt))
		dev_pm_opp_put_opp_table(ad->oppt);
}

static int thermal_init(struct apu_dev *ad)
{
	int ret;
	struct device *dev = ad->dev;
	const char *thermal_name;

	if (thermal)
		return 0;

	ret = of_property_read_string(dev->of_node, "io-channel-names", &thermal_name);
	if (ret) {
		aprobe_err(ad->dev, "%s have no properity \"io-channel\"\n", __func__);
		return -EFAULT;

	} else {
		thermal = devm_iio_channel_get(dev, thermal_name);

		if (IS_ERR(thermal)) {
			aprobe_err(ad->dev, "[%s] get_thermal fail\n", __func__);
			return -ENOMEM;
		}
	}
	return 0;
}

static u32 thermal_read(struct apu_dev *ad)
{
#define DEFAULT_T_SENSOR_REF            (772)
#define DEFAULT_T_SENSOR_SHIFT          (24000)
	int temp = 0;
	static u16 tsensor_ref;
	static u16 tsensor_shift;

	if (!thermal) {
		apower_info(ad->dev, "[%s] thermal not init\n", __func__);
		return 0;
	}

	if (tsensor_shift == 0)
		tsensor_shift = DEFAULT_T_SENSOR_SHIFT;

	tsensor_ref = DEFAULT_T_SENSOR_REF;
	// read VBE_CODE
	iio_read_channel_processed(thermal, &temp);
	temp = ((tsensor_ref - temp) * 770)
			+ tsensor_shift;
	temp /= 1000;

	return temp;
}

int dtv_apu_mt5896_get_target_freq(struct apu_dev *ad)
{
	int temp = 0;
	int opp;
	unsigned long freq;

	temp = thermal_read(ad);

	opp = (temp < TEMP_THRESHOLD) ? 0 : 1;
	freq = apu_opp2freq(ad, opp);

	apower_info(ad->dev, "%s(): temp: %d target: %lu\n",
				__func__, temp, freq);

	return freq;
}

void dtv_apu_mt5896_suspend(struct apu_dev *ad)
{
	power_hnd->init_once = 0;
	mt589x_mtcmos_suspend(ad);
	mt5896_clk_suspend(ad);
}

void dtv_apu_mt5896_resume(struct apu_dev *ad)
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

int dtv_apu_mt5896_misc_init(struct apu_dev *ad)
{
	unsigned long volt = 0;
	unsigned long rate = 0;
	int ret = 0;

	thermal_init(ad);

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

void dtv_apu_mt5896_misc_uninit(struct apu_dev *ad)
{
	power_delay_task_start(ad, 0);

	if (power_hnd->power_ctrl & APU_POWER_CTRL_DEFAULT_ON) {
		if (ad->plat_ops->mtcmos_off)
			ad->plat_ops->mtcmos_off(ad, 1);
	}

	if (!IS_ERR_OR_NULL(ad->aclk))
		ad->aclk->ops->disable(ad->aclk);
}
