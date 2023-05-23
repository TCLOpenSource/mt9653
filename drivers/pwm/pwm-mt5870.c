// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright(C) 2019-2020 MediaTek Inc.
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

enum pwm_attr_e {
	e_pwm_attr_oen = 0,
	e_pwm_attr_period,
	e_pwm_attr_duty,
	e_pwm_attr_div,
	e_pwm_attr_shift,
	e_pwm_attr_pol,
	e_pwm_attr_dben,
	e_pwm_attr_vrst,
	e_pwm_attr_rstmux,
	e_pwm_attr_rstcnt,
	e_pwm_attr_max,
};

struct mtk_pwm_dat {
	char pwm_ch;
	unsigned int period;
	unsigned int shift;
	unsigned int div;
	bool pol;
	bool rst_mux;
	bool rst_vsync;
	unsigned int rstcnt;
};

struct mtk_pwm {
	struct pwm_chip chip;
	enum pwm_attr_e  e_pwm;
	const struct mtk_pwm_dat *data;
	void __iomem *base;
};

//Todo
/*These info will config from Devicetree*/
static const struct mtk_pwm_dat mtk_pwm0_data = {
	.pwm_ch = 0,
	.period = 0xFFFF,
	.shift = 0,
	.div = 0,
	.pol = 0,
	.rst_mux = 0,
	.rst_vsync = 1,
	.rstcnt = 0,
};

#define PWM_HW_SUPPORT_CH		6
#define PWM_CLK_BITMASK			(0x2 << 1)
#define PWM_SUB_BANK			(0x0 << 1)
#define PWM_CLK_SET			(0x1 << 6)
#define PWM_PERIOD_REG			(0x2 << 2)
#define PWM_REG_OFFSET_INDEX		(0x3 << 2)
#define PWM_DUTY_REG			(0x3 << 2)
#define PWM_CTL_REG			(0x4 << 2)
#define PWM_SHIFT_REG			(0x28 << 2)
#define PWM_REG_SHIFT_INDEX		(0x2 << 2)
#define PWM_VDBEN_SET			(0x1 << 9)
#define PWM_VREST_SET			(0x1 << 10)
#define PWM_DBEN_SET			(0x1 << 11)

static inline struct mtk_pwm *to_mtk_pwm(struct pwm_chip *chip)
{
	return container_of(chip, struct mtk_pwm, chip);
}

static void mtk_pwm_oen(struct mtk_pwm *mdp, unsigned int data)
{
	unsigned char ch_mask;
	unsigned char reg_tmp;
	unsigned char bitop_tmp;

	ch_mask = mdp->data->pwm_ch;
	/*Turn to PWM SUB-BANK*/
	writel(0x1, mdp->base + PWM_SUB_BANK);
	reg_tmp = readl(mdp->base + PWM_CLK_BITMASK);

	if (data)
		bitop_tmp = reg_tmp | ((1 << ch_mask) | PWM_CLK_SET);
	else
		bitop_tmp = (reg_tmp | PWM_CLK_SET) & (~((1 << ch_mask)));

	writel(bitop_tmp, mdp->base + PWM_CLK_BITMASK);
	/*Turn to XC BANK*/
	writel(0x0, mdp->base + PWM_SUB_BANK);
}

static void mtk_pwm_period(struct mtk_pwm *mdp, unsigned int data)
{
	unsigned char ch_mask;

	ch_mask = mdp->data->pwm_ch;
	/*Turn to PWM SUB-BANK*/
	writel(0x1, mdp->base + PWM_SUB_BANK);
	writel(data, mdp->base + PWM_PERIOD_REG +
				(ch_mask*PWM_REG_OFFSET_INDEX));
	/*Turn to XC BANK*/
	writel(0x0, mdp->base + PWM_SUB_BANK);
}

static void mtk_pwm_duty(struct mtk_pwm *mdp, unsigned int data)
{
	unsigned char ch_mask;

	ch_mask = mdp->data->pwm_ch;
	/*Turn to PWM SUB-BANK*/
	writel(0x1, mdp->base + PWM_SUB_BANK);
	writel(data, mdp->base + PWM_DUTY_REG + (ch_mask*PWM_REG_OFFSET_INDEX));
	/*Turn to XC BANK*/
	writel(0x0, mdp->base + PWM_SUB_BANK);
}

static void mtk_pwm_div(struct mtk_pwm *mdp, unsigned int data)
{
	unsigned char ch_mask;
	unsigned int reg_tmp;

	ch_mask = mdp->data->pwm_ch;
	/*Turn to PWM SUB-BANK*/
	writel(0x1, mdp->base + PWM_SUB_BANK);
	reg_tmp = readl(mdp->base + PWM_CTL_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	reg_tmp |= data;
	writel(reg_tmp, mdp->base + PWM_CTL_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	/*Turn to XC BANK*/
	writel(0x0, mdp->base + PWM_SUB_BANK);
}

static void mtk_pwm_shift(struct mtk_pwm *mdp, unsigned int data)
{
	unsigned char ch_mask;
	unsigned int reg_tmp;

	ch_mask = mdp->data->pwm_ch;
	/*Turn to PWM SUB-BANK*/
	writel(0x1, mdp->base + PWM_SUB_BANK);
	reg_tmp = readl(mdp->base + PWM_SHIFT_REG +
					(ch_mask*PWM_REG_SHIFT_INDEX));
	reg_tmp |= data;
	writel(reg_tmp, mdp->base + PWM_SHIFT_REG +
					(ch_mask*PWM_REG_SHIFT_INDEX));
	/*Turn to XC BANK*/
	writel(0x0, mdp->base + PWM_SUB_BANK);
}

static void mtk_pwm_pol(struct mtk_pwm *mdp, unsigned int data)
{
	unsigned char ch_mask;
	unsigned int reg_tmp;

	ch_mask = mdp->data->pwm_ch;
	/*Turn to PWM SUB-BANK*/
	writel(0x1, mdp->base + PWM_SUB_BANK);
	reg_tmp = readl(mdp->base + PWM_CTL_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	if (data)
		reg_tmp |= (0x1 << 8);
	else
		reg_tmp &= ~(0x1 << 8);

	writel(reg_tmp, mdp->base + PWM_CTL_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	/*Turn to XC BANK*/
	writel(0x0, mdp->base + PWM_SUB_BANK);
}

static void mtk_pwm_vsync(struct mtk_pwm *mdp, unsigned int data)
{
	unsigned char ch_mask;
	unsigned int reg_tmp;

	ch_mask = mdp->data->pwm_ch;
	/*Turn to PWM SUB-BANK*/
	writel(0x1, mdp->base + PWM_SUB_BANK);
	reg_tmp = readl(mdp->base + PWM_CTL_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	if (data) {
		reg_tmp |= PWM_VDBEN_SET;
		reg_tmp |= PWM_VREST_SET;
		reg_tmp &= ~PWM_DBEN_SET;
	} else {
		reg_tmp &= ~PWM_VDBEN_SET;
		reg_tmp &= ~PWM_VREST_SET;
		reg_tmp |= PWM_DBEN_SET;
	}
	writel(reg_tmp, mdp->base + PWM_CTL_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	/*Turn to XC BANK*/
	writel(0x0, mdp->base + PWM_SUB_BANK);
}

static void mtk_pwm_update_ctrl(struct mtk_pwm *mdp, enum pwm_attr_e e_pwm,
				unsigned int data)
{
	//Todo
	/*PWM use direct bank in M6 chip, control flow should be modified*/
	//Todo
	/*Eliminate wrapper update_ctrl flow*/
	switch (e_pwm) {
	case e_pwm_attr_oen:
		mtk_pwm_oen(mdp, data);
		break;
	case e_pwm_attr_period:
		mtk_pwm_period(mdp, data);
		break;
	case e_pwm_attr_duty:
		mtk_pwm_duty(mdp, data);
		break;
	case e_pwm_attr_div:
		mtk_pwm_div(mdp, data);
		break;
	case e_pwm_attr_shift:
		mtk_pwm_shift(mdp, data);
		break;
	case e_pwm_attr_pol:
		mtk_pwm_pol(mdp, data);
		break;
	case e_pwm_attr_vrst:
		mtk_pwm_vsync(mdp, data);
		break;
	default:
		break;
	}
}

static int mtk_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			  int duty_t, int period_t)
{
	struct mtk_pwm *mdp = to_mtk_pwm(chip);
	int err;
	int duty_set, period_set;

	if ((duty_t == 0) || (duty_t > period_t)) {
		/*solve glitch when duty is 0*/
		mtk_pwm_update_ctrl(mdp, e_pwm_attr_shift, 1);
	}

	/*12hz XTAL timing is nsec*/
	period_set = (12*period_t)/1000;
	duty_set = (12*duty_t)/1000;

	mtk_pwm_update_ctrl(mdp, e_pwm_attr_period, period_set);
	mtk_pwm_update_ctrl(mdp, e_pwm_attr_duty, duty_set);
	mtk_pwm_update_ctrl(mdp, e_pwm_attr_shift, mdp->data->shift);
	mtk_pwm_update_ctrl(mdp, e_pwm_attr_div, mdp->data->div);
	mtk_pwm_update_ctrl(mdp, e_pwm_attr_pol, mdp->data->pol);
	mtk_pwm_update_ctrl(mdp, e_pwm_attr_vrst, mdp->data->rst_vsync);

	return 0;
}

static int mtk_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_pwm *mdp = to_mtk_pwm(chip);
	int err;

	mtk_pwm_update_ctrl(mdp, e_pwm_attr_oen, 0);
	return 0;
}

static void mtk_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_pwm *mdp = to_mtk_pwm(chip);
	int err;

	mtk_pwm_update_ctrl(mdp, e_pwm_attr_oen, 1);
}

static void mtk_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
			      struct pwm_state *state)
{
	struct mtk_pwm *mdp = to_mtk_pwm(chip);
	int    pwm_attr_get;
	unsigned char ch_mask;

	writel(0x1, mdp->base + PWM_SUB_BANK);
	ch_mask = mdp->data->pwm_ch;
	pwm_attr_get = readl(mdp->base + PWM_PERIOD_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	state->period = (((pwm_attr_get>>8)&0x1)?1:0);

	pwm_attr_get = readl(mdp->base + PWM_DUTY_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	state->duty_cycle = pwm_attr_get;

	pwm_attr_get = readl(mdp->base + PWM_CTL_REG +
					(ch_mask*PWM_REG_OFFSET_INDEX));
	state->polarity = (((pwm_attr_get>>8)&0x1)?1:0);

	pwm_attr_get = readl(mdp->base + PWM_CLK_BITMASK);
	pwm_attr_get &= (1<<ch_mask);
	state->enabled = (pwm_attr_get?1:0);
}

static const struct pwm_ops mtk_pwm_ops = {
	.config = mtk_pwm_config,
	.enable = mtk_pwm_enable,
	.disable = mtk_pwm_disable,
	.get_state = mtk_pwm_get_state,
	.owner = THIS_MODULE,
};

static int __init mtk_pwm_probe(struct platform_device *pdev)
{
	struct mtk_pwm *mdp;
	int ret;
	struct resource *res;

	mdp = devm_kzalloc(&pdev->dev, sizeof(*mdp), GFP_KERNEL);

	if (!mdp)
		return -ENOMEM;

	//mdp->data = of_device_get_match_data(&pdev->dev);
	mdp->data = &mtk_pwm0_data;
	mdp->chip.dev = &pdev->dev;
	mdp->chip.ops = &mtk_pwm_ops;
	/*It will return error without this setting*/
	mdp->chip.npwm = PWM_HW_SUPPORT_CH;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mdp->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));

	if ((mdp->data->pwm_ch) != PWM_HW_SUPPORT_CH)
		mtk_pwm_update_ctrl(mdp, e_pwm_attr_oen, 1);
	else
		return -1;

	mtk_pwm_update_ctrl(mdp, e_pwm_attr_period, mdp->data->period);

	ret = pwmchip_add(&mdp->chip);

	if (ret < 0)
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);

	platform_set_drvdata(pdev, mdp);

	return ret;
}

static int mtk_pwm_remove(struct platform_device *pdev)
{
	if (!(pdev->name) ||
		strcmp(pdev->name, "mediatek,mt5870-pwm") || pdev->id != 0)
		return -1;

	pdev->dev.platform_data = NULL;
	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id mtkpwm_of_device_ids[] = {
		{.compatible = "mediatek,mt5870-pwm", .data = &mtk_pwm0_data},
		//Todo
		/*Add Multiple PWM HW*/
		{},
};
#endif

static struct platform_driver mtk_pwm_driver = {
	.probe = mtk_pwm_probe,
	.remove = mtk_pwm_remove,
	.driver = {
#if defined(CONFIG_OF)
		.of_match_table = mtkpwm_of_device_ids,
#endif
		.name = "mtk-pwm",
		.owner = THIS_MODULE,
	}
};
module_platform_driver(mtk_pwm_driver);

MODULE_AUTHOR("Owen.Tseng@mediatek.com");
MODULE_DESCRIPTION("pwm driver");
MODULE_LICENSE("GPL");
