// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
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

struct mtk_segpwm_dat {
	unsigned int vsync_p;
	unsigned int period;
	unsigned int duty_cycle;
	unsigned int shift;
	unsigned int sync_times;
	bool enable;
	bool oneshot;
};

struct mtk_segpwm {
	struct pwm_chip chip;
	struct mtk_segpwm_dat *data;
	void __iomem *base_x;
	void __iomem *base_y;
	void __iomem *base;
	void __iomem *base_n;
	void __iomem *base_ckgen;
	struct clk *clk_gate_xtal;
	struct clk *clk_gate_pwm;
	unsigned int channel;
	unsigned int scanpad_ch;
};

#define MAX_PWM_HW_NUM				4
static struct pwm_chip *_ptr_array[MAX_PWM_HW_NUM];
#define PWM_REG_SHIFT_INDEX		(0x2 << 2)

#define LENSIZE					32
#define REG_0004_PWM			0x04
#define CKEN_ALL_EN				(0x1 << 6)
#define REG_00DC_PWM_SEG		0xDC
#define REG_01CC_PWM_SEG		0x1CC
#define REG_0160_PWM_SEG		0x160
#define REG_0014_PWM_SEG		0x14
#define REG_0010_PWM_SEG		0x10
#define REG_0020_PWM_SEG		0x20
#define REG_0024_PWM_SEG		0x24
#define REG_0028_PWM_SEG		0x28
#define REG_002C_PWM_SEG		0x2C
#define REG_0030_PWM_SEG		0x30
#define REG_0034_PWM_SEG		0x34
#define REG_0038_PWM_SEG		0x38
#define REG_003C_PWM_SEG		0x3C
#define REG_0008_PWM_SEG		0x08

#define SEG_OUT_OFFSET			0x4
#define SEG_VRESET_SET			(0x1 << 0)
#define SEG_VSYNC_SYNC_0		(0x1 << 8)
#define SEG_VSYNC_SYNC_1		(0x1 << 9)
#define VSYNC_50_HZ				(50)
#define VSYNC_100_HZ			(100)
#define VSYNC_60_HZ				(60)
#define VSYNC_120_HZ			(120)
#define VSYNC_200_HZ			(200)
#define VSYNC_240_HZ			(240)
#define VSYNC_PULSE_SEL			(3)
#define VSYNC_16384_PULSE		(16384)
#define HALF_DUTY_EVENT			(50)
#define SEG_EVENT_SET			(1 << 14)
#define SEG_EVENT_EN			(1 << 15)
#define SEG_PULS_BYPASS_SET		(1 << 4)
#define SEG_2_TIMES_VSYNC		(2)
#define SEG_4_TIMES_VSYNC		(4)
#define SEG_8_TIMES_VSYNC		(8)
#define SEG_FULL_DUTY			(100)
#define SEG_EVENTS_DEFAULT		(2)


static inline struct mtk_segpwm *to_mtk_segpwm(struct pwm_chip *chip)
{
	return container_of(chip, struct mtk_segpwm, chip);
}

static inline u16 dtvsegpwm_rd(struct mtk_segpwm *mdp, u32 reg)
{
	return mtk_readw_relaxed((mdp->base + reg));
}

static inline u8 dtvsegpwm_rdl(struct mtk_segpwm *mdp, u16 reg)
{
	return dtvsegpwm_rd(mdp, reg)&0xff;
}

static inline void dtvsegpwm_wr(struct mtk_segpwm *mdp, u16 reg, u32 val)
{
	mtk_writew_relaxed((mdp->base + reg), val);
}

static inline void dtvsegpwm_wrl(struct mtk_segpwm *mdp, u16 reg, u8 val)
{
	u16 val16 = dtvsegpwm_rd(mdp, reg)&0xff00;

	val16 |= val;
	dtvsegpwm_wr(mdp, reg, val16);
}

static void mtk_segpwm_oen(struct mtk_segpwm *mdp, unsigned int data)
{
	u16 bitop_tmp;
	u16 bitop_org;
	u16 reg_tmp;
	u16 reg_org;

	reg_org = ((mdp->scanpad_ch) >> 1)*SEG_OUT_OFFSET;
	bitop_org = mtk_readw_relaxed(mdp->base_n + REG_01CC_PWM_SEG + reg_org);
	/*base_n port_switch*/
	if (data) {
		reg_tmp = (mdp->scanpad_ch)&1;
		bitop_tmp = bitop_org;
		if (reg_tmp)
			bitop_tmp |= ((mdp->channel) << 8);
		else {
			bitop_tmp &= 0xFF00;
			bitop_tmp |= (mdp->channel);
		}
	} else
		bitop_tmp = bitop_org;

	mtk_writew_relaxed(mdp->base_n + REG_01CC_PWM_SEG + reg_org , bitop_tmp);
	/*PWM_SEGx output select*/
	if ((mdp->scanpad_ch)< SEG_OUT_OFFSET) {
		reg_org = (mdp->scanpad_ch)*SEG_OUT_OFFSET;
		bitop_org = mtk_readw_relaxed(mdp->base_y);
		bitop_tmp = ((mdp->channel)+1) << reg_org;
		bitop_tmp |= bitop_org;
		mtk_writew_relaxed(mdp->base_y, bitop_tmp);
	}
}

static void mtk_segpwm_cken(struct mtk_segpwm *mdp)
{
	u16 reg_tmp;

	reg_tmp = mtk_readw_relaxed(mdp->base_x);
	reg_tmp |= (1 << (mdp->channel));
	mtk_writew_relaxed(mdp->base_x, reg_tmp);
	reg_tmp = mtk_readw_relaxed(mdp->base_ckgen + REG_0004_PWM);
	reg_tmp |= CKEN_ALL_EN;
	mtk_writew_relaxed(mdp->base_ckgen + REG_0004_PWM, reg_tmp);
}


static void mtk_segpwm_vsync(struct mtk_segpwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = dtvsegpwm_rd(mdp, REG_0014_PWM_SEG);
	if (!data) {
		reg_tmp |= SEG_VRESET_SET;
		reg_tmp &= ~SEG_VSYNC_SYNC_0;
		reg_tmp &= ~SEG_VSYNC_SYNC_1;
	} else {
		/*reset by n times Vsync*/
		reg_tmp |= SEG_VRESET_SET;
		reg_tmp |= SEG_VSYNC_SYNC_0;
		reg_tmp &= ~SEG_VSYNC_SYNC_1;
		reg_tmp |=(data << 4);
	}
	dtvsegpwm_wr(mdp, REG_0014_PWM_SEG, reg_tmp);
	reg_tmp = dtvsegpwm_rd(mdp, REG_0010_PWM_SEG);
	reg_tmp |= VSYNC_PULSE_SEL;
	reg_tmp |= SEG_PULS_BYPASS_SET;
	dtvsegpwm_wr(mdp, REG_0010_PWM_SEG, reg_tmp);
}


static int mtk_segpwm_config(struct pwm_chip *chip,
							struct pwm_device *segpwm,
						int duty_t, int period_t)
{
	struct mtk_segpwm *mdp = to_mtk_segpwm(chip);
	u16 position;
	u8 vsync_times;

	if (!mdp->data->oneshot) {
		mtk_segpwm_oen(mdp, 1);
		mtk_segpwm_vsync(mdp, 0);
		mdp->data->oneshot = 1;
	}

	if ((!mdp->data->vsync_p)||(!period_t))
		return 0;

	if (!mdp->data->sync_times)
		mdp->data->sync_times = SEG_EVENTS_DEFAULT*(period_t / mdp->data->vsync_p);

	mdp->data->period = period_t;
	if ((mdp->data->duty_cycle) != duty_t) {
		vsync_times = mdp->data->sync_times;
		if (vsync_times == SEG_2_TIMES_VSYNC) {
			dtvsegpwm_wr(mdp, REG_0028_PWM_SEG, 0);
			dtvsegpwm_wr(mdp, REG_002C_PWM_SEG, 0);
			dtvsegpwm_wr(mdp, REG_0030_PWM_SEG, 0);
			dtvsegpwm_wr(mdp, REG_0034_PWM_SEG, 0);
			dtvsegpwm_wr(mdp, REG_0038_PWM_SEG, 0);
			dtvsegpwm_wr(mdp, REG_003C_PWM_SEG, 0);
			position = (mdp->data->shift)*(VSYNC_16384_PULSE/(HALF_DUTY_EVENT*vsync_times));
			position |= SEG_EVENT_SET;//1
			position |= SEG_EVENT_EN;
			dtvsegpwm_wr(mdp, REG_0020_PWM_SEG, position);
			position += ((duty_t*VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position &= ~SEG_EVENT_SET;//0
			dtvsegpwm_wr(mdp, REG_0024_PWM_SEG, position);
		}

		if (vsync_times == SEG_4_TIMES_VSYNC) {
			dtvsegpwm_wr(mdp, REG_0030_PWM_SEG, 0);
			dtvsegpwm_wr(mdp, REG_0034_PWM_SEG, 0);
			dtvsegpwm_wr(mdp, REG_0038_PWM_SEG, 0);
			dtvsegpwm_wr(mdp, REG_003C_PWM_SEG, 0);
			position = (mdp->data->shift)*(VSYNC_16384_PULSE/(HALF_DUTY_EVENT*vsync_times));
			position |= SEG_EVENT_SET;//1
			position |= SEG_EVENT_EN;
			dtvsegpwm_wr(mdp, REG_0020_PWM_SEG, position);
			position += ((duty_t*VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position &= ~SEG_EVENT_SET;//0
			dtvsegpwm_wr(mdp, REG_0024_PWM_SEG, position);
			position += (((SEG_FULL_DUTY - duty_t)*
						VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position |= SEG_EVENT_SET;//1
			dtvsegpwm_wr(mdp, REG_0028_PWM_SEG, position);
			position += ((duty_t*VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position &= ~SEG_EVENT_SET;//0
			dtvsegpwm_wr(mdp, REG_002C_PWM_SEG, position);
		}

		if (vsync_times == SEG_8_TIMES_VSYNC) {
			position = (mdp->data->shift)*(VSYNC_16384_PULSE/(HALF_DUTY_EVENT*vsync_times));
			position |= SEG_EVENT_SET;//1
			position |= SEG_EVENT_EN;
			dtvsegpwm_wr(mdp, REG_0020_PWM_SEG, position);
			position += ((duty_t*VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position &= ~SEG_EVENT_SET;//0
			dtvsegpwm_wr(mdp, REG_0024_PWM_SEG, position);
			position += (((SEG_FULL_DUTY - duty_t)*
						VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position |= SEG_EVENT_SET;//1
			dtvsegpwm_wr(mdp, REG_0028_PWM_SEG, position);
			position += ((duty_t*VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position &= ~SEG_EVENT_SET;//0
			dtvsegpwm_wr(mdp, REG_002C_PWM_SEG, position);
			position += (((SEG_FULL_DUTY - duty_t)*
						VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position |= SEG_EVENT_SET;//1
			dtvsegpwm_wr(mdp, REG_0030_PWM_SEG, position);
			position += ((duty_t*VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position &= ~SEG_EVENT_SET;//0
			dtvsegpwm_wr(mdp, REG_0034_PWM_SEG, position);
			position += (((SEG_FULL_DUTY - duty_t)*
						VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position |= SEG_EVENT_SET;//1
			dtvsegpwm_wr(mdp, REG_0038_PWM_SEG, position);
			position += ((duty_t*VSYNC_16384_PULSE)/(HALF_DUTY_EVENT*vsync_times));
			position &= ~SEG_EVENT_SET;//0
			dtvsegpwm_wr(mdp, REG_003C_PWM_SEG, position);
		}
		dtvsegpwm_wr(mdp, REG_0008_PWM_SEG, 1);
		mdp->data->duty_cycle = duty_t;
	}
	return 0;
}

static int mtk_segpwm_enable(struct pwm_chip *chip, struct pwm_device *segpwm)
{
	struct mtk_segpwm *mdp = to_mtk_segpwm(chip);

	mdp->data->enable = 1;
	mtk_segpwm_oen(mdp, 1);
	return 0;
}

static void mtk_segpwm_disable(struct pwm_chip *chip, struct pwm_device *segpwm)
{
	struct mtk_segpwm *mdp = to_mtk_segpwm(chip);

	mdp->data->enable = 0;
	mtk_segpwm_oen(mdp, 0);
}

static void mtk_segpwm_get_state(struct pwm_chip *chip,
							struct pwm_device *segpwm,
							struct pwm_state *state)
{
	struct mtk_segpwm *mdp = to_mtk_segpwm(chip);

	state->period = mdp->data->period;
	state->duty_cycle = mdp->data->duty_cycle;
	state->polarity = 0;
	state->enabled = mdp->data->enable;
}

static const struct pwm_ops mtk_segpwm_ops = {
	.config = mtk_segpwm_config,
	.enable = mtk_segpwm_enable,
	.disable = mtk_segpwm_disable,
	.get_state = mtk_segpwm_get_state,
	.owner = THIS_MODULE,
};

static ssize_t shift_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_segpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->shift);
}

static ssize_t shift_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_segpwm *mdp = dev_get_drvdata(child);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	mdp->data->shift = val;
	return ret ? : size;
}
static DEVICE_ATTR_RW(shift);


static ssize_t vsync_p_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_segpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->vsync_p);
}

static ssize_t vsync_p_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_segpwm *mdp = dev_get_drvdata(child);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	mdp->data->vsync_p = val;
	return ret ? : size;
}
static DEVICE_ATTR_RW(vsync_p);


static struct attribute *pwm_extend_attrs[] = {
	&dev_attr_shift.attr,
	&dev_attr_vsync_p.attr,
	NULL,
};
static const struct attribute_group pwm_extend_attr_group = {
	.name = "pwm_extend",
	.attrs = pwm_extend_attrs
};


int mtk_segpwm_set_attr(u8 channel, struct mtk_segpwm_dat *scandat)
{
	struct pwm_chip *pwmchip = _ptr_array[channel];
	struct mtk_segpwm *mdp = to_mtk_segpwm(pwmchip);

	mdp->data->shift = scandat->shift;
	mdp->data->enable = scandat->enable;
	mdp->data->sync_times = scandat->sync_times;
	mtk_segpwm_config(pwmchip, NULL, scandat->duty_cycle, scandat->period);
	mdp->data->duty_cycle = scandat->duty_cycle;
	mdp->data->period = scandat->period;
	return 0;
}
EXPORT_SYMBOL(mtk_segpwm_set_attr);


static int mtk_segpwm_probe(struct platform_device *pdev)
{
	struct mtk_segpwm *mdp;
	int ret;
	struct device_node *np = NULL;
	struct mtk_segpwm_dat *scandat;
	int err = -ENODEV;

	mdp = devm_kzalloc(&pdev->dev, sizeof(*mdp), GFP_KERNEL);
	scandat = devm_kzalloc(&pdev->dev, sizeof(*scandat), GFP_KERNEL);
	np = pdev->dev.of_node;

	if ((!mdp) || (!scandat))
		return -ENOMEM;

	if (!np) {
		dev_info(&pdev->dev, "Failed to find node\n");
		ret = -EINVAL;
		return ret;
	}

	memset(scandat, 0, sizeof(*scandat));
	mdp->data = scandat;
	mdp->chip.dev = &pdev->dev;
	mdp->chip.ops = &mtk_segpwm_ops;
	/*It will return error without this setting*/
	mdp->chip.npwm = 1;
	mdp->chip.base = -1;

	err = of_property_read_u32(pdev->dev.of_node,
				"channel", &mdp->channel);
	if (err)
		dev_info(&pdev->dev, "could not get channel resource\n");


	err = of_property_read_u32(pdev->dev.of_node,
				"vsync-freq", &mdp->data->vsync_p);
	if (err)
		dev_info(&pdev->dev, "could not get vsync-freq resource\n");

	mdp->base_x = of_iomap(np, 0);
	mdp->base_y = of_iomap(np, 1);
	mdp->base = of_iomap(np, 2);
	mdp->base_n = of_iomap(np, 3);
	mdp->base_ckgen = of_iomap(np, 4);

	if (!mdp->base)
		return -EINVAL;
	mtk_segpwm_cken(mdp);
	mdp->clk_gate_xtal = devm_clk_get(&pdev->dev, "xtal_12m2pwm");
	if (IS_ERR_OR_NULL(mdp->clk_gate_xtal)) {
		if (IS_ERR(mdp->clk_gate_xtal))
			dev_info(&pdev->dev, "err mdp->clk_gate_xtal=%#lx\n",
				(unsigned long)mdp->clk_gate_xtal);
		else
			dev_info(&pdev->dev, "err mdp->clk_gate_xtal = NULL\n");
	}
	ret = clk_prepare_enable(mdp->clk_gate_xtal);
	if (ret)
		dev_info(&pdev->dev, "Failed to enable xtal_12m2pwm: %d\n", ret);

	mdp->clk_gate_pwm = devm_clk_get(&pdev->dev, "pwm_ck");
	if (IS_ERR_OR_NULL(mdp->clk_gate_pwm)) {
		if (IS_ERR(mdp->clk_gate_pwm))
			dev_info(&pdev->dev, "err mdp->clk_gate_pwm=%#lx\n",
				(unsigned long)mdp->clk_gate_pwm);
		else
			dev_info(&pdev->dev, "err mdp->clk_gate_pwm = NULL\n");
	}
	ret = clk_prepare_enable(mdp->clk_gate_pwm);
	if (ret)
		dev_info(&pdev->dev, "Failed to enable pwm_ck: %d\n", ret);

	ret = pwmchip_add(&mdp->chip);

	if (mdp->channel < MAX_PWM_HW_NUM)
		_ptr_array[mdp->channel] = &mdp->chip;

	if (ret < 0)
		dev_err(&pdev->dev, "segpwmchip_add() failed: %d\n", ret);

	platform_set_drvdata(pdev, mdp);

	ret = sysfs_create_group(&pdev->dev.kobj, &pwm_extend_attr_group);
	if (ret < 0)
		dev_err(&pdev->dev, "sysfs_create_group failed: %d\n", ret);

	return ret;
}

static int mtk_segpwm_remove(struct platform_device *pdev)
{
	struct mtk_segpwm *mdp = platform_get_drvdata(pdev);
	int ret = 0;

	if (!(pdev->name) ||
		strcmp(pdev->name, "mediatek,mt5896-segpwm") || pdev->id != 0)
		return -1;

	ret = pwmchip_remove(&mdp->chip);
	clk_unprepare(mdp->clk_gate_xtal);
	clk_unprepare(mdp->clk_gate_pwm);

	pdev->dev.platform_data = NULL;
	return ret;
}
#ifdef CONFIG_PM
static int mtk_dtv_pwmscan_suspend(struct device *pdev)
{
	struct mtk_segpwm *mdp = dev_get_drvdata(pdev);

	if (!mdp)
		return -EINVAL;
	mdp->data->oneshot = 0;
	if (!IS_ERR_OR_NULL(mdp->clk_gate_xtal))
		clk_disable_unprepare(mdp->clk_gate_xtal);

	if (!IS_ERR_OR_NULL(mdp->clk_gate_pwm))
		clk_disable_unprepare(mdp->clk_gate_pwm);

	return 0;
}

static int mtk_dtv_pwmscan_resume(struct device *pdev)
{
	struct mtk_segpwm *mdp = dev_get_drvdata(pdev);
	int ret = 0;

	if (!mdp)
		return -EINVAL;

	if (!IS_ERR_OR_NULL(mdp->clk_gate_xtal)) {
		ret = clk_prepare_enable(mdp->clk_gate_xtal);
		if (ret)
			dev_info(pdev, "resume failed to enable xtal_12m2pwm: %d\n", ret);
	}
	if (!IS_ERR_OR_NULL(mdp->clk_gate_pwm)) {
		ret = clk_prepare_enable(mdp->clk_gate_pwm);
		if (ret)
			dev_info(pdev, "resume failed to enable pwm_ck: %d\n", ret);
	}
	mtk_segpwm_cken(mdp);

	return 0;
}
#endif

#if defined(CONFIG_OF)
static const struct of_device_id mtksegpwm_of_device_ids[] = {
		{.compatible = "mediatek,mt5896-segpwm" },
		{},
};
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops pwm_pwmscan_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(mtk_dtv_pwmscan_suspend, mtk_dtv_pwmscan_resume)
};
#endif
static struct platform_driver mtk_segpwm_driver = {
	.probe		= mtk_segpwm_probe,
	.remove		= mtk_segpwm_remove,
	.driver		= {
#if defined(CONFIG_OF)
	.of_match_table = mtksegpwm_of_device_ids,
#endif
#ifdef CONFIG_PM
	.pm	= &pwm_pwmscan_pm_ops,
#endif
	.name   = "mediatek,mt5896-segpwm",
	.owner  = THIS_MODULE,
}
};

static int __init mtk_segpwm_init_module(void)
{
	int retval = 0;

	retval = platform_driver_register(&mtk_segpwm_driver);

	return retval;
}

static void __exit mtk_segpwm_exit_module(void)
{
	platform_driver_unregister(&mtk_segpwm_driver);
}

module_init(mtk_segpwm_init_module);
module_exit(mtk_segpwm_exit_module);


MODULE_AUTHOR("Owen.Tseng@mediatek.com");
MODULE_DESCRIPTION("segpwm driver");
MODULE_LICENSE("GPL");

