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

int dtv_apu_mt5896_e3_opp_init(struct apu_dev *ad)
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

void dtv_apu_mt5896_e3_opp_uninit(struct apu_dev *ad)
{
	if (!IS_ERR_OR_NULL(ad->oppt))
		dev_pm_opp_put_opp_table(ad->oppt);
}

void dtv_apu_mt5896_e3_suspend(struct apu_dev *ad)
{
	power_hnd->init_once = 0;
	mt589x_mtcmos_suspend(ad);
	mt5896_e3_clk_suspend(ad);
}

void dtv_apu_mt5896_e3_resume(struct apu_dev *ad)
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

