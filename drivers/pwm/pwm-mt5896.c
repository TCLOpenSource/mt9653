// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author Owen.Tseng <Owen.Tseng@mediatek.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/pwm.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "pwm-mt5896-riu.h"
#include <linux/clk.h>

struct mtk_pwm_dat {
	unsigned int period;
	unsigned int duty_cycle;
	unsigned int div;
	bool pol;
	bool dben;
	bool enable;
};

struct mtk_pwm {
	struct pwm_chip chip;
	struct mtk_pwm_dat *data;
	void __iomem *base;
	void __iomem *base_cksel;
};

#define PWM_HW_SUPPORT_CH		1

#define REG_0000_PWM			0x0
#define REG_0004_PWM			0x4
#define REG_0008_PWM			0x8
#define REG_000C_PWM			0xC
#define REG_0010_PWM			0x10
#define REG_0014_PWM			0x14

#define PM_PWM_SW_EN			1
#define PM_PWM_CK_EN			1
#define PM_PWM_LOW_WORD			16
#define PM_PWM_POL				8
#define PM_PWM_DBEN				9

static inline struct mtk_pwm *to_mtk_pwm(struct pwm_chip *chip)
{
	return container_of(chip, struct mtk_pwm, chip);
}

static inline u16 dtvpwm_rd(struct mtk_pwm *mdp, u32 reg)
{
	return mtk_readw_relaxed((mdp->base + reg));
}

static inline u8 dtvpwm_rdl(struct mtk_pwm *mdp, u16 reg)
{
	return dtvpwm_rd(mdp, reg)&0xff;
}

static inline void dtvpwm_wr(struct mtk_pwm *mdp, u16 reg, u32 val)
{
	mtk_writew_relaxed((mdp->base + reg), val);
}

static inline void dtvpwm_wrl(struct mtk_pwm *mdp, u16 reg, u8 val)
{
	u16 val16 = dtvpwm_rd(mdp, reg)&0xff00;

	val16 |= val;
	dtvpwm_wr(mdp, reg, val16);
}

static void mtk_pwm_oen(struct mtk_pwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = dtvpwm_rdl(mdp, REG_0014_PWM);

	if (data)
		reg_tmp |= PM_PWM_CK_EN;
	else
		reg_tmp &= ~PM_PWM_CK_EN;

	dtvpwm_wrl(mdp, REG_0014_PWM, reg_tmp);
}

static void mtk_pwm_period(struct mtk_pwm *mdp, unsigned int data)
{
	u32 reg_tmp = 0;

	if (data > 1)
		data -= 1;

	dtvpwm_wr(mdp, REG_0000_PWM, (u16)data);
	reg_tmp >>= PM_PWM_LOW_WORD;
	dtvpwm_wr(mdp, REG_0004_PWM, reg_tmp);
}

static void mtk_pwm_duty(struct mtk_pwm *mdp, unsigned int data)
{
	u32 reg_tmp = 0;

	if (data > 1)
		data -= 1;

	dtvpwm_wr(mdp, REG_0008_PWM, (u16)data);
	reg_tmp >>= PM_PWM_LOW_WORD;
	dtvpwm_wr(mdp, REG_000C_PWM, reg_tmp);
}

static void mtk_pwm_div(struct mtk_pwm *mdp, unsigned int data)
{
	if (data > 1)
		data -= 1;

	dtvpwm_wr(mdp, REG_0010_PWM, (u16)data);
}

static void mtk_pwm_pol(struct mtk_pwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = dtvpwm_rdl(mdp, REG_0014_PWM);

	if (data)
		reg_tmp |= PM_PWM_POL;
	else
		reg_tmp &= ~PM_PWM_POL;

	dtvpwm_wrl(mdp, REG_0014_PWM, reg_tmp);
}

static void mtk_pwm_dben(struct mtk_pwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = dtvpwm_rdl(mdp, REG_0014_PWM);

	if (data)
		reg_tmp |= PM_PWM_DBEN;
	else
		reg_tmp &= ~PM_PWM_DBEN;

	dtvpwm_wrl(mdp, REG_0014_PWM, reg_tmp);
}

static void mtk_pmpwm_cken(struct mtk_pwm *mdp)
{
	u16 reg_tmp;

	reg_tmp = mtk_readw_relaxed(mdp->base_cksel);
	reg_tmp |= PM_PWM_SW_EN;
	mtk_writew_relaxed(mdp->base_cksel, reg_tmp);
}


static int mtk_pwm_config(struct pwm_chip *chip,
							struct pwm_device *pwm,
						int duty_t, int period_t)
{
	struct mtk_pwm *mdp = to_mtk_pwm(chip);
	int duty_set, period_set;

	/*12hz XTAL timing is nsec*/
	period_set = (12*period_t)/1000;
	duty_set = (12*duty_t)/1000;

	mtk_pwm_oen(mdp, 1);
	mtk_pwm_period(mdp, period_set);
	mtk_pwm_duty(mdp, duty_set);
	mtk_pwm_dben(mdp, mdp->data->dben);
	mtk_pwm_div(mdp, mdp->data->div);
	mtk_pwm_pol(mdp, mdp->data->pol);

	return 0;
}

static int mtk_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_pwm *mdp = to_mtk_pwm(chip);

	mtk_pwm_oen(mdp, 1);
	return 0;
}

static void mtk_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_pwm *mdp = to_mtk_pwm(chip);

	mtk_pwm_oen(mdp, 0);
}

static void mtk_pwm_get_state(struct pwm_chip *chip,
							struct pwm_device *pwm,
							struct pwm_state *state)
{
	struct mtk_pwm *mdp = to_mtk_pwm(chip);

	state->period = mdp->data->period;
	state->duty_cycle = mdp->data->duty_cycle;
	state->polarity = mdp->data->pol;
	state->enabled = mdp->data->enable;
}

static const struct pwm_ops mtk_pwm_ops = {
	.config = mtk_pwm_config,
	.enable = mtk_pwm_enable,
	.disable = mtk_pwm_disable,
	.get_state = mtk_pwm_get_state,
	.owner = THIS_MODULE,
};

static int mtk_pwm_probe(struct platform_device *pdev)
{
	struct mtk_pwm *mdp;
	int ret;
	struct device_node *np = NULL;
	struct mtk_pwm_dat *pwmdat;

	mdp = devm_kzalloc(&pdev->dev, sizeof(*mdp), GFP_KERNEL);
	pwmdat = devm_kzalloc(&pdev->dev, sizeof(*pwmdat), GFP_KERNEL);
	np = pdev->dev.of_node;

	if ((!mdp) || (!pwmdat))
		return -ENOMEM;

	if (!np) {
		dev_err(&pdev->dev, "Failed to find node\n");
		ret = -EINVAL;
		return ret;
	}

	memset(pwmdat, 0, sizeof(*pwmdat));
	mdp->data = pwmdat;
	mdp->chip.dev = &pdev->dev;
	mdp->chip.ops = &mtk_pwm_ops;
	/*It will return error without this setting*/
	mdp->chip.npwm = PWM_HW_SUPPORT_CH;
	mdp->chip.base = -1;

	mdp->base = of_iomap(np, 0);
	mdp->base_cksel = of_iomap(np, 1);


	mtk_pmpwm_cken(mdp);
	ret = pwmchip_add(&mdp->chip);

	if (ret < 0)
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);

	platform_set_drvdata(pdev, mdp);

	return ret;
}

static int mtk_pwm_remove(struct platform_device *pdev)
{
	if (!(pdev->name) ||
		strcmp(pdev->name, "mediatek,mt5896-pwm") || pdev->id != 0)
		return -1;

	pdev->dev.platform_data = NULL;
	return 0;
}

#ifdef CONFIG_PM
static int mtk_dtv_pwm_suspend(struct device *pdev)
{
	struct mtk_pwm *mdp = dev_get_drvdata(pdev);

	if (!mdp)
		return -EINVAL;

	return 0;
}

static int mtk_dtv_pwm_resume(struct device *pdev)
{
	struct mtk_pwm *mdp = dev_get_drvdata(pdev);

	if (!mdp)
		return -EINVAL;

	mtk_pwm_oen(mdp, 1);
	return 0;
}
#endif

#if defined(CONFIG_OF)
static const struct of_device_id mtkpwm_of_device_ids[] = {
		{.compatible = "mediatek,mt5896-pwm" },
		{},
};
#endif

#ifdef CONFIG_PM
static SIMPLE_DEV_PM_OPS(pwm_pwmnormal_pm_ops,
		mtk_dtv_pwm_suspend, mtk_dtv_pwm_resume);
#endif
static struct platform_driver Mtk_pwm_driver = {
	.probe		= mtk_pwm_probe,
	.remove		= mtk_pwm_remove,
	.driver		= {
#if defined(CONFIG_OF)
	.of_match_table = mtkpwm_of_device_ids,
#endif
#ifdef CONFIG_PM
	.pm	= &pwm_pwmnormal_pm_ops,
#endif
	.name   = "mediatek,mt5896-pwm",
	.owner  = THIS_MODULE,
}
};

static int __init mtk_pwm_init_module(void)
{
	int retval = 0;

	retval = platform_driver_register(&Mtk_pwm_driver);

	return retval;
}

static void __exit mtk_pwm_exit_module(void)
{
	platform_driver_unregister(&Mtk_pwm_driver);
}

module_init(mtk_pwm_init_module);
module_exit(mtk_pwm_exit_module);


MODULE_AUTHOR("Owen.Tseng@mediatek.com");
MODULE_DESCRIPTION("pwm driver");
MODULE_LICENSE("GPL");

