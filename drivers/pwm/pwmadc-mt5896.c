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

struct mtk_adcpwm_dat {
	unsigned int period;
	unsigned int duty_cycle;
	unsigned int shift;
	unsigned int div;
	unsigned int pol;
	unsigned int clk_sel;
	unsigned int puls_minus;
	unsigned int auto_correct;
	unsigned int period_set;
	unsigned int duty_set;
	unsigned int offset_set;
	bool dben;
	bool enable;
	bool oneshot;
};

struct mtk_adcpwm {
	struct pwm_chip chip;
	struct mtk_adcpwm_dat *data;
	void __iomem *base;
	void __iomem *base_n;
	void __iomem *base_cksel;
	void __iomem *base_pinctl;
};

#define REG_0000_PWM_ADC0		0x0
#define REG_0004_PWM_ADC0		0x4
#define REG_0008_PWM_ADC0		0x8
#define REG_000C_PWM_ADC0		0xC
#define REG_0010_PWM_ADC0		0x10

#define PWM_ADC_PAD0			(0x1 << 8)
#define PWM_ADC_PAD1			(0x1 << 12)
#define PWM_ADC_AUTO_CORRECT_EN	(0x1 << 0)
#define PWM_ADC_SWEN			(0x1 << 10)
#define PWM_CLK_GATE			(0x1 << 4)
#define PWM_DBEN_SET			(0x1 << 8)
#define PWM_SIGN_SYMBOL			(0x1 << 7)
#define PWM_ADC_POL				(0x1 << 9)
#define XTAL_24MHZ				24
#define XTAL_216MHZ				216
#define FREQ_SCALE				1000
#define LENSIZE					32
#define PWM_HALF_WORD			16
#define PWM_CLK_SEL				(0x1 << 6)

static const u16 pwm_clk_src[] = {
	XTAL_24MHZ,
	XTAL_216MHZ,
};

static inline struct mtk_adcpwm *to_mtk_adcpwm(struct pwm_chip *chip)
{
	return container_of(chip, struct mtk_adcpwm, chip);
}

static inline u16 dtvadcpwm_rd(struct mtk_adcpwm *mdp, u32 reg)
{
	return mtk_readw_relaxed((mdp->base + reg));
}

static inline u8 dtvadcpwm_rdl(struct mtk_adcpwm *mdp, u16 reg)
{
	return dtvadcpwm_rd(mdp, reg)&0xff;
}

static inline void dtvadcpwm_wr(struct mtk_adcpwm *mdp, u16 reg, u32 val)
{
	mtk_writew_relaxed((mdp->base + reg), val);
}

static inline void dtvadcpwm_wrl(struct mtk_adcpwm *mdp, u16 reg, u8 val)
{
	u16 val16 = dtvadcpwm_rd(mdp, reg)&0xff00;

	val16 |= val;
	dtvadcpwm_wr(mdp, reg, val16);
}

static void mtk_adcpwm_oen(struct mtk_adcpwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = mtk_readw_relaxed(mdp->base_cksel);

	if (data)
		reg_tmp &= ~PWM_CLK_GATE;
	else
		reg_tmp |= PWM_CLK_GATE;

	if (mdp->data->clk_sel)
		reg_tmp |= PWM_CLK_SEL;
	else
		reg_tmp &= ~PWM_CLK_SEL;
	mtk_writew_relaxed(mdp->base_cksel, reg_tmp);

}

static void mtk_adcpwm_cken(struct mtk_adcpwm *mdp)
{
	u16 reg_tmp;

	reg_tmp = mtk_readw_relaxed(mdp->base_n);
	reg_tmp |= PWM_ADC_SWEN;
	mtk_writew_relaxed(mdp->base_n, reg_tmp);
}

static void mtk_adcpwm_pinsel(struct mtk_adcpwm *mdp)
{
	u16 reg_tmp;

	reg_tmp = mtk_readw_relaxed(mdp->base_pinctl);
	reg_tmp |= PWM_ADC_PAD0;
	reg_tmp |= PWM_ADC_PAD1;
	mtk_writew_relaxed(mdp->base_pinctl, reg_tmp);
}

static void mtk_adcpwm_pinsel_gpio(struct mtk_adcpwm *mdp)
{
	u16 reg_tmp;

	reg_tmp = mtk_readw_relaxed(mdp->base_pinctl);
	reg_tmp &= ~PWM_ADC_PAD0;
	reg_tmp &= ~PWM_ADC_PAD1;
	mtk_writew_relaxed(mdp->base_pinctl, reg_tmp);
}

static void mtk_adcpwm_period(struct mtk_adcpwm *mdp, unsigned int data)
{
	if (data > 1)
		data -= 1;

	dtvadcpwm_wr(mdp, REG_0000_PWM_ADC0, data);
}

static void mtk_adcpwm_duty(struct mtk_adcpwm *mdp, unsigned int data)
{
	if (data > 1)
		data -= 1;

	dtvadcpwm_wr(mdp, REG_0004_PWM_ADC0, data);
}

static void mtk_adcpwm_div(struct mtk_adcpwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = dtvadcpwm_rd(mdp, REG_0008_PWM_ADC0);
	if (data > 1)
		data -= 1;
	reg_tmp |= data;
	dtvadcpwm_wr(mdp, REG_0008_PWM_ADC0, reg_tmp);
}

static void mtk_adcpwm_shift(struct mtk_adcpwm *mdp, unsigned int data)
{
	u8 reg_tmp;

	reg_tmp = dtvadcpwm_rdl(mdp, REG_0010_PWM_ADC0);
	if (mdp->data->auto_correct)
		reg_tmp = data | PWM_ADC_AUTO_CORRECT_EN;
	else
		reg_tmp = data & ~PWM_ADC_AUTO_CORRECT_EN;
	dtvadcpwm_wrl(mdp, REG_0010_PWM_ADC0, reg_tmp);

	reg_tmp = dtvadcpwm_rdl(mdp, REG_000C_PWM_ADC0);
	if (mdp->data->puls_minus)
		reg_tmp = data | PWM_SIGN_SYMBOL;
	else
		reg_tmp = data & ~PWM_SIGN_SYMBOL;

	dtvadcpwm_wrl(mdp, REG_000C_PWM_ADC0, reg_tmp);
}

static void mtk_adcpwm_pol(struct mtk_adcpwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = dtvadcpwm_rd(mdp, REG_0008_PWM_ADC0);
	if (data)
		reg_tmp |= PWM_ADC_POL;
	else
		reg_tmp &= ~PWM_ADC_POL;

	dtvadcpwm_wr(mdp, REG_0008_PWM_ADC0, reg_tmp);
}

static void mtk_adcpwm_dben(struct mtk_adcpwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = dtvadcpwm_rd(mdp, REG_0008_PWM_ADC0);
	if (data)
		reg_tmp |= PWM_DBEN_SET;
	else
		reg_tmp &= ~PWM_DBEN_SET;

	dtvadcpwm_wr(mdp, REG_0008_PWM_ADC0, reg_tmp);
}


static int mtk_adcpwm_config(struct pwm_chip *chip,
							struct pwm_device *adcpwm,
						int duty_t, int period_t)
{
	struct mtk_adcpwm *mdp = to_mtk_adcpwm(chip);
	int duty_set, period_set, offset_set;

	if ((duty_t == 0) || (duty_t > period_t)) {
		/*solve glitch when duty is 0*/
		mtk_adcpwm_shift(mdp, 1);
	}

	mdp->data->period = period_t;
	mdp->data->duty_cycle = duty_t;

	/*12hz XTAL timing is nsec*/
	period_set = (pwm_clk_src[mdp->data->clk_sel]*period_t)/FREQ_SCALE;
	duty_set = (pwm_clk_src[mdp->data->clk_sel]*duty_t)/FREQ_SCALE;
	//offset_set = (pwm_clk_src[mdp->data->clk_sel]*(mdp->data->shift))/FREQ_SCALE;
	//mtk_adcpwm_shift(mdp, offset_set);
	//mtk_adcpwm_dben(mdp, mdp->data->dben);
	mtk_adcpwm_period(mdp, period_set);
	mtk_adcpwm_duty(mdp, duty_set);
	mtk_adcpwm_div(mdp, mdp->data->div);
	mtk_adcpwm_oen(mdp, 1);
	mtk_adcpwm_pol(mdp, mdp->data->pol);
	if (!mdp->data->oneshot) {
		mtk_adcpwm_cken(mdp);
		mtk_adcpwm_pinsel(mdp);
		mdp->data->oneshot = 1;
	}
	return 0;
}

static int mtk_adcpwm_enable(struct pwm_chip *chip, struct pwm_device *adcpwm)
{
	struct mtk_adcpwm *mdp = to_mtk_adcpwm(chip);

	mtk_adcpwm_oen(mdp, 1);
	return 0;
}

static void mtk_adcpwm_disable(struct pwm_chip *chip, struct pwm_device *adcpwm)
{
	struct mtk_adcpwm *mdp = to_mtk_adcpwm(chip);

	if (mdp->data->pol == 1) {
		mtk_adcpwm_oen(mdp, 1);
		mtk_adcpwm_duty(mdp, 0);
	} else {
		mtk_adcpwm_oen(mdp, 0);
	}
}

static int mtk_adcpwm_polarity(struct pwm_chip *chip,
				struct pwm_device *adcpwm, enum pwm_polarity polarity)
{
	struct mtk_adcpwm *mdp = to_mtk_adcpwm(chip);

	mtk_adcpwm_pol(mdp, polarity);
	return 0;
}

static void mtk_adcpwm_get_state(struct pwm_chip *chip,
							struct pwm_device *adcpwm,
							struct pwm_state *state)
{
	struct mtk_adcpwm *mdp = to_mtk_adcpwm(chip);
	int    scanpwm_attr_get;

	state->period = mdp->data->period;
	state->duty_cycle = mdp->data->duty_cycle;
	scanpwm_attr_get = dtvadcpwm_rd(mdp, REG_0008_PWM_ADC0);
	state->polarity = (((scanpwm_attr_get >> 9)&0x1)?1:0);

	scanpwm_attr_get = mtk_readw_relaxed(mdp->base_cksel);
	scanpwm_attr_get &= PWM_CLK_GATE;
	state->enabled = (scanpwm_attr_get?0:1);
}

static const struct pwm_ops mtk_adcpwm_ops = {
	.config = mtk_adcpwm_config,
	.enable = mtk_adcpwm_enable,
	.disable = mtk_adcpwm_disable,
	.get_state = mtk_adcpwm_get_state,
	.set_polarity = mtk_adcpwm_polarity,
	.owner = THIS_MODULE,
};

static ssize_t dben_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->dben);
}

static ssize_t dben_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(child);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	mdp->data->dben = val;
	mtk_adcpwm_dben(mdp, val);
	return ret;
}
static DEVICE_ATTR_RW(dben);

static ssize_t div_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->div);
}

static ssize_t div_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(child);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	mdp->data->div = val;
	mtk_adcpwm_div(mdp, val);
	return ret;
}
static DEVICE_ATTR_RW(div);

static ssize_t shift_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->shift);
}

static ssize_t shift_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(child);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	mdp->data->shift = val;
	val = (pwm_clk_src[mdp->data->clk_sel]*val)/FREQ_SCALE;
	mtk_adcpwm_shift(mdp, val);
	return ret;
}
static DEVICE_ATTR_RW(shift);


static ssize_t fullscale_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(child);
	u32 reg_tmp;

	reg_tmp = dtvadcpwm_rd(mdp, REG_0000_PWM_ADC0);
	reg_tmp |= (dtvadcpwm_rd(mdp, REG_0004_PWM_ADC0) << PWM_HALF_WORD);
	reg_tmp = ((reg_tmp + 1) * FREQ_SCALE)/XTAL_24MHZ;
	mdp->data->period = reg_tmp;

	return scnprintf(buf, LENSIZE, "%d\n", reg_tmp);
}
static DEVICE_ATTR_RO(fullscale);

static ssize_t curduty_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(child);
	u32 reg_tmp;

	if (!mdp->data->duty_cycle) {
		reg_tmp = dtvadcpwm_rd(mdp, REG_0008_PWM_ADC0);
		reg_tmp |= (dtvadcpwm_rd(mdp, REG_000C_PWM_ADC0) << PWM_HALF_WORD);
		mdp->data->duty_cycle = (reg_tmp * FREQ_SCALE)/XTAL_24MHZ;
	}

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->duty_cycle);
}
static DEVICE_ATTR_RO(curduty);


static struct attribute *pwm_extend_attrs[] = {
	&dev_attr_dben.attr,
	&dev_attr_div.attr,
	&dev_attr_shift.attr,
	&dev_attr_fullscale.attr,
	&dev_attr_curduty.attr,
	NULL,
};
static const struct attribute_group pwm_extend_attr_group = {
	.name = "pwm_extend",
	.attrs = pwm_extend_attrs
};

static int mtk_adcpwm_probe(struct platform_device *pdev)
{
	struct mtk_adcpwm *mdp;
	int ret;
	struct device_node *np = NULL;
	struct mtk_adcpwm_dat *scandat;
	int err = -ENODEV;

	mdp = devm_kzalloc(&pdev->dev, sizeof(*mdp), GFP_KERNEL);
	scandat = devm_kzalloc(&pdev->dev, sizeof(*scandat), GFP_KERNEL);
	np = pdev->dev.of_node;

	if ((!mdp) || (!scandat))
		return -ENOMEM;

	if (!np) {
		dev_err(&pdev->dev, "Failed to find node\n");
		ret = -EINVAL;
		return ret;
	}

	memset(scandat, 0, sizeof(*scandat));
	mdp->data = scandat;
	mdp->chip.dev = &pdev->dev;
	mdp->chip.ops = &mtk_adcpwm_ops;
	/*It will return error without this setting*/
	mdp->chip.npwm = 1;
	mdp->chip.base = -1;

	err = of_property_read_u32(pdev->dev.of_node,
				"div", &mdp->data->div);
	if (err)
		dev_info(&pdev->dev, "could not get div resource\n");

	err = of_property_read_u32(pdev->dev.of_node,
				"clk-sel", &mdp->data->clk_sel);
	if (err)
		dev_info(&pdev->dev, "could not get clk-sel resource\n");

	if (of_get_property(np, "dben", NULL))
		mdp->data->dben = true;
	else
		mdp->data->dben = 0;

	err = of_property_read_u32(pdev->dev.of_node,
				"pol", &mdp->data->pol);
	if (err)
		dev_info(&pdev->dev, "could not get pol resource\n");

	err = of_property_read_u32(pdev->dev.of_node,
				"puls-minus", &mdp->data->puls_minus);
	if (err)
		dev_info(&pdev->dev, "could not get puls-minus resource\n");

	err = of_property_read_u32(pdev->dev.of_node,
				"auto-correct", &mdp->data->auto_correct);
	if (err)
		dev_info(&pdev->dev, "could not get auto-corect resource\n");

	err = of_property_read_u32(pdev->dev.of_node,
				"offset", &mdp->data->shift);
	if (err)
		dev_info(&pdev->dev, "could not get shift resource\n");

	mdp->base = of_iomap(np, 0);
	mdp->base_n = of_iomap(np, 1);
	mdp->base_cksel = of_iomap(np, 2);
	mdp->base_pinctl = of_iomap(np, 3);

	if (!mdp->base)
		return -EINVAL;

	ret = pwmchip_add(&mdp->chip);


	if (ret < 0)
		dev_err(&pdev->dev, "scanpwmchip_add() failed: %d\n", ret);

	platform_set_drvdata(pdev, mdp);

	ret = sysfs_create_group(&pdev->dev.kobj, &pwm_extend_attr_group);
	if (ret < 0)
		dev_err(&pdev->dev, "sysfs_create_group failed: %d\n", ret);

	return ret;
}

static int mtk_adcpwm_remove(struct platform_device *pdev)
{
	struct mtk_adcpwm *mdp = platform_get_drvdata(pdev);
	int ret;

	if (!(pdev->name) ||
		strcmp(pdev->name, "mediatek,mt5896-adcpwm") || pdev->id != 0)
		return -1;

	ret = pwmchip_remove(&mdp->chip);

	pdev->dev.platform_data = NULL;
	return ret;
}
#ifdef CONFIG_PM
static int mtk_dtv_pwmadc_suspend(struct device *pdev)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(pdev);

	if (!mdp)
		return -EINVAL;

	mdp->data->period_set = dtvadcpwm_rd(mdp, REG_0000_PWM_ADC0);
	mdp->data->duty_set = dtvadcpwm_rd(mdp, REG_0004_PWM_ADC0);
	mdp->data->offset_set = dtvadcpwm_rd(mdp, REG_000C_PWM_ADC0);
	mdp->data->auto_correct = dtvadcpwm_rd(mdp, REG_0010_PWM_ADC0);
	mdp->data->dben = dtvadcpwm_rd(mdp, REG_0008_PWM_ADC0);
	mdp->data->oneshot = 0;
	mtk_adcpwm_pinsel_gpio(mdp);
	return 0;
}

static int mtk_dtv_pwmadc_resume(struct device *pdev)
{
	struct mtk_adcpwm *mdp = dev_get_drvdata(pdev);
	int ret;

	if (!mdp)
		return -EINVAL;

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	mtk_adcpwm_period(mdp, (mdp->data->period_set)+1);
	mtk_adcpwm_duty(mdp, (mdp->data->duty_set)+1);
	mtk_adcpwm_shift(mdp, mdp->data->offset_set);
	mtk_adcpwm_dben(mdp, mdp->data->dben);
	mtk_adcpwm_oen(mdp, 1);
	if (!mdp->data->oneshot) {
		mtk_adcpwm_cken(mdp);
		mtk_adcpwm_pinsel(mdp);
		mdp->data->oneshot = 1;
	}
#else
	mtk_adcpwm_oen(mdp, 1);
#endif
	return 0;
}
#endif

#if defined(CONFIG_OF)
static const struct of_device_id mtkadcpwm_of_device_ids[] = {
		{.compatible = "mediatek,mt5896-adcpwm" },
		{},
};
#endif

#ifdef CONFIG_PM
static SIMPLE_DEV_PM_OPS(pwm_pwmadc_pm_ops,
		mtk_dtv_pwmadc_suspend, mtk_dtv_pwmadc_resume);
#endif
static struct platform_driver mtk_adcpwm_driver = {
	.probe		= mtk_adcpwm_probe,
	.remove		= mtk_adcpwm_remove,
	.driver		= {
#if defined(CONFIG_OF)
	.of_match_table = mtkadcpwm_of_device_ids,
#endif
#ifdef CONFIG_PM
	.pm	= &pwm_pwmadc_pm_ops,
#endif
	.name   = "mediatek,mt5896-adcpwm",
	.owner  = THIS_MODULE,
}
};

static int __init mtk_adcpwm_init_module(void)
{
	int retval = 0;

	retval = platform_driver_register(&mtk_adcpwm_driver);

	return retval;
}

static void __exit mtk_adcpwm_exit_module(void)
{
	platform_driver_unregister(&mtk_adcpwm_driver);
}

module_init(mtk_adcpwm_init_module);
module_exit(mtk_adcpwm_exit_module);


MODULE_AUTHOR("Owen.Tseng@mediatek.com");
MODULE_DESCRIPTION("adcpwm driver");
MODULE_LICENSE("GPL");

