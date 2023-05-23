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
#include "apu_regulator.h"
#include "apu_of.h"
#include "apu_io.h"
#include "apu_log.h"
#include "apu_common.h"
#include "apu_dbg.h"
#include "apu_reg.h"
#include "comm_driver.h"

#define E_DVFS_OPTEE_INIT       (0x100)
#define E_DVFS_OPTEE_ADJUST     (0x101)

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)

static int teec_regulator_enable(struct apu_regulator_gp *rgul_gp)
{
	u32 error = 0;
	int ret;
	struct TEEC_Operation op = {0};

	if (!rgul_gp->set_regul) {
		advfs_info(rgul_gp->dev, "%s: ret, set_regul = 0\n", __func__);
		ret = 0;
		goto out;
	}

	advfs_info(rgul_gp->dev, "%s():\n", __func__);
	op.params[0].value.a = 0;
	op.params[0].value.b = 0;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	ret = comm_util_get_cb()->send_teec_cmd(CPUFREQ_TA_UUID_IDX,
					E_DVFS_OPTEE_INIT, &op, &error);
	if (ret) {
		rgul_gp->set_regul = 0;
		advfs_err(rgul_gp->dev, "%s failed, error(%u)\n", __func__, error);
		ret = -EPERM;
		goto out;
	}
out:
	return ret;
}

static int teec_regulator_disable(struct apu_regulator_gp *rgul_gp)
{
	return 0;
}

static int teec_regulator_get_voltage(struct apu_regulator_gp *rgul_gp)
{
	return 0;
}

static int teec_regulator_set_voltage(struct apu_regulator_gp *rgul_gp, int min_uV, int max_uV)
{
	u32 error = 0;
	int ret = 0;
	struct TEEC_Operation op = {0};

	if (!rgul_gp->set_regul) {
		advfs_info(rgul_gp->dev, "%s: ret, set_regul = 0\n", __func__);
		ret = 0;
		goto out;
	}

	advfs_info(rgul_gp->dev, "%s: volt:%d\n", __func__, min_uV);
	op.params[0].value.a = TOMV(min_uV);
	op.params[0].value.b = 0;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	ret = comm_util_get_cb()->send_teec_cmd(CPUFREQ_TA_UUID_IDX,
						E_DVFS_OPTEE_ADJUST, &op, &error);
	if (ret) {
		advfs_err(rgul_gp->dev, "[%s] failed, error(%u)\n", __func__, error);
		ret = -EPERM;
	}

out:
	return ret;
}

#else

static int regulator_apu_get_voltage(struct apu_regulator_gp *rgul_gp)
{
	int cur_volt = -EINVAL;

	if (rgul_gp && !rgul_gp->rgul) {
		mutex_lock(&rgul_gp->rgul->reg_lock);
		cur_volt = regulator_get_voltage(rgul_gp->rgul->vdd);
		mutex_unlock(&rgul_gp->rgul->reg_lock);
	}

	return cur_volt;
}

static int regulator_apu_set_voltage(struct apu_regulator_gp *rgul_gp, int min_uV, int max_uV)
{
	int ret = 0;
	struct apu_regulator *sup_reg = NULL, *reg = NULL;

	if (!rgul_gp->set_regul) {
		argul_info(rgul_gp->dev, "%s: set_regul is disable.\n", __func__);
		return 0;
	}

	sup_reg = rgul_gp->rgul_sup;
	reg = rgul_gp->rgul;

	mutex_lock(&reg->reg_lock);

	if (!IS_ERR_OR_NULL(reg)) {
		ret = regulator_set_voltage(reg->vdd, min_uV, max_uV);
		if (ret)
			goto out;
		reg->cur_volt = min_uV;
	}

	argul_info(rgul_gp->dev, "[%s] \"%s\" final %dmV",
				__func__, reg->name, TOMV(min_uV));
out:
	mutex_unlock(&reg->reg_lock);

	return ret;
}

static int regulator_apu_disable(struct apu_regulator_gp *rgul_gp)
{
	int ret = 0;
	struct apu_regulator *dst = NULL;

	ret = regulator_apu_set_voltage(rgul_gp,
				rgul_gp->rgul->shut_volt,
				rgul_gp->rgul->shut_volt);
	if (ret)
		goto out;

	if (!IS_ERR_OR_NULL(rgul_gp->rgul)) {
		dst = rgul_gp->rgul;
		if (!dst->vdd)
			goto out;
		if (dst->cstr.always_on)
			goto out;
		ret = regulator_disable(dst->vdd);
		if (ret) {
			argul_err(rgul_gp->dev, "[%s] %s disable fail, ret = %d\n",
				__func__, rgul_gp->rgul->name, ret);
			goto out;
		}
		dst->enabled = 0;
	}
out:
	return ret;
}

static int regulator_apu_enable(struct apu_regulator_gp *rgul_gp)
{
	int ret = 0;
	struct apu_regulator *dst = NULL;

	if (!rgul_gp->set_regul)
		goto out;

	if (rgul_gp->rgul) {
		dst = rgul_gp->rgul;
		if (!dst->enabled) {
			if (IS_ERR_OR_NULL(dst->vdd)) {
				argul_err(rgul_gp->dev, "[%s] vdd not avail!\n", __func__);
				goto out;
			}
			ret = regulator_enable(dst->vdd);
			if (ret) {
				argul_err(rgul_gp->dev, "[%s] %s enable fail, ret = %d\n",
					__func__, dst->name, ret);
				goto out;
			}
			dst->enabled = 1;
		}
		dst->cur_volt = regulator_get_voltage(dst->vdd);
		if (dst->cur_volt < 0)
			goto out;
	}

out:
	return ret;
}

#endif // CONFIG_ARM_MTKTV_CPUFREQ_CA

static int regulator_dummy_enable(struct apu_regulator_gp *rgul_gp)
{
	argul_info(rgul_gp->dev, "%s(): !!!\n", __func__);
	return 0;
}

static int regulator_dummy_disable(struct apu_regulator_gp *rgul_gp)
{
	argul_info(rgul_gp->dev, "%s(): !!!\n", __func__);
	return 0;
}

static int regulator_dummy_set_voltage(struct apu_regulator_gp *rgul_gp,
					int min_uV, int max_uV)
{
	argul_info(rgul_gp->dev, "%s(): !!!\n", __func__);
	return 0;
}

static int regulator_dummy_get_voltage(struct apu_regulator_gp *rgul_gp)
{
	argul_info(rgul_gp->dev, "%s(): !!!\n", __func__);
	return 0;
}

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static struct apu_regulator_ops apu_rgul_gp_ops = {
	.enable = teec_regulator_enable,
	.disable = teec_regulator_disable,
	.set_voltage = teec_regulator_set_voltage,
	.get_voltage = teec_regulator_get_voltage,
};
#else
static struct apu_regulator_ops apu_rgul_gp_ops = {
	.enable = regulator_apu_enable,
	.disable = regulator_apu_disable,
	.set_voltage = regulator_apu_set_voltage,
	.get_voltage = regulator_apu_get_voltage,
};
#endif
static struct apu_regulator_ops apu_rgul_dummy_ops = {
	.enable = regulator_dummy_enable,
	.disable = regulator_dummy_disable,
	.set_voltage = regulator_dummy_set_voltage,
	.get_voltage = regulator_dummy_get_voltage,
};

static struct apu_regulator dummy_mdla = {
	.name = "dummy_mdla",
	.enabled = 0,
};

static struct apu_regulator_gp dummy_rgul_gp = {
	.rgul = &dummy_mdla,
	.ops = &apu_rgul_dummy_ops,
};

static struct apu_regulator mt589xvmdla0 = {
	.name = "vgpu",
	.cstr = {
		.settling_time = 8,
		.settling_time_up = 10000,
		.settling_time_down = 5000,
	},
	.enabled = 0,
};

static struct apu_regulator_gp mt589x_rgul_gp = {
	.rgul = &mt589xvmdla0,
	.ops = &apu_rgul_gp_ops,
};

static const struct apu_regulator_array apu_rgul_gps[] = {
	{ .name = "dummy_mdla", .argul_gp = &dummy_rgul_gp },
	{ .name = "mt589x_mdla0", .argul_gp = &mt589x_rgul_gp },
};

struct apu_regulator_gp *dtv_regulator_gp_get(struct apu_dev *ad)
{
	return apu_rgul_gps[1].argul_gp;
}

int dtv_apu_regulator_init(struct apu_dev *ad)
{
	int ret = 0;
	unsigned long volt = 0, def_freq = 0;
	struct apu_regulator *dst = NULL;

	ad->argul = dtv_regulator_gp_get(ad);
	if (IS_ERR_OR_NULL(ad->argul))
		goto out;

	/* initial regulator gp lock */
	mutex_init(&(ad->argul->rgulgp_lock));

	/* initial individual regulator lock */
	mutex_init(&(ad->argul->rgul->reg_lock));

	ad->argul->dev = ad->dev;
	ad->argul->rgul->dev = ad->dev;

	ret = of_property_read_u32(ad->dev->of_node, "set_regul",
				&ad->argul->set_regul);

	if ((ret == 0) && ad->argul->set_regul) {
		/* get the slowest frq in opp and set it as default frequency */
		ret = apu_get_recommend_freq_volt(ad->dev, &def_freq, &volt, 0);
		if (ret)
			goto out;

		/* get regulator (need in non dvfs_ta case?) */
		dst = ad->argul->rgul;
		ret = of_apu_regulator_get(ad->dev, dst, volt, def_freq);
		if (ret) {
			aprobe_err(ad->dev, "%s: regulator_get fail\n", __func__);
			//ad->argul->set_regul = 0;
			ret = 0;
			goto out;
		}

		/* init regulator hanle*/
		ret = ad->argul->ops->enable(ad->argul);
		if (ret) {
			aprobe_err(ad->dev, "%s: enable fail, retry later...\n", __func__);
			//ad->argul->set_regul = 0;
			ret = 0;
			goto out;
		}
	}
out:
	return ret;
}

void dtv_apu_regulator_uninit(struct apu_dev *ad)
{
#ifdef APU_CLK_TREE
	struct apu_regulator *dst = NULL;

	if (!IS_ERR_OR_NULL(ad->argul)) {
		dst = ad->argul->rgul;
		if (!IS_ERR_OR_NULL(dst))
			of_apu_regulator_put(dst);
	}
#endif
}

