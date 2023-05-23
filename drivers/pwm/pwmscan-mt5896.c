// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
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
#include "pwmscan-mt5896-coda.h"
#include "pwm-mt5896-riu.h"
#include <linux/clk.h>

#define  PERIOD_ATTR			BIT(0)
#define  DUTY_ATTR				BIT(1)
#define  OFFSET_ATTR			BIT(2)
#define  DIV_ATTR				BIT(3)
#define  POL_ATTR				BIT(4)
#define  RESET_VSYNC_ATTR		BIT(5)
#define  OEN_ATTR				BIT(6)
#define  RSTCNT_ATTR			BIT(7)
#define  RSTMUX_ATTR			BIT(8)

struct mtk_scanpwm_dat {
	unsigned int period;
	unsigned int duty_cycle;
	unsigned int shift;
	unsigned int div;
	unsigned int pol;
	bool rst_mux;
	bool rst_vsync;
	bool enable;
	unsigned int rstcnt;
	unsigned int scanpwm_attribute;
};

struct mtk_scanpwm {
	struct pwm_chip chip;
	struct mtk_scanpwm_dat *data;
	void __iomem *base;
	void __iomem *base_n;
	void __iomem *base_ckgen;
	struct clk *clk_gate_xtal;
	struct clk *clk_gate_pwm;
	unsigned int channel;
};

#define MAX_PWM_HW_NUM				8
static struct pwm_chip *_ptr_array[MAX_PWM_HW_NUM];
#define PWM_CLK_SET				(0x1 << 8)
#define PWM_REG_OFFSET_INDEX	(0x3 << 2)
#define PWM_REG_SHIFT_INDEX		(0x2 << 2)
#define PWM_VDBEN_SET			(0x1 << 9)
#define PWM_VREST_SET			(0x1 << 10)
#define PWM_DBEN_SET			(0x1 << 11)
#define PWM_SCAN_OFFSET			0x10
#define XTAL_MHZ				12
#define FREQ_SCALE				1000
#define RESET_MUX_SET			(0x1 << 12)
#define RESET_CNT_SET			8
#define LSB_BIT					4
#define LENSIZE					32
#define PWM_VDBEN_SW_SET		(0x1 << 14)
#define PWM_HALF_WORD			16
#define REG_0004_PWM			0x04
#define CKEN_ALL_EN				(0x1 << 6)
#define CKGEN_BASE				2
#define BYTE_SHIFT				8


static inline struct mtk_scanpwm *to_mtk_scanpwm(struct pwm_chip *chip)
{
	return container_of(chip, struct mtk_scanpwm, chip);
}

static inline u16 dtvscanpwm_rd(struct mtk_scanpwm *mdp, u32 reg)
{
	return mtk_readw_relaxed((mdp->base + reg));
}

static inline u8 dtvscanpwm_rdl(struct mtk_scanpwm *mdp, u16 reg)
{
	return dtvscanpwm_rd(mdp, reg)&0xff;
}

static inline void dtvscanpwm_wr(struct mtk_scanpwm *mdp, u16 reg, u32 val)
{
	mtk_writew_relaxed((mdp->base + reg), val);
}

static inline void dtvscanpwm_wrl(struct mtk_scanpwm *mdp, u16 reg, u8 val)
{
	u16 val16 = dtvscanpwm_rd(mdp, reg)&0xff00;

	val16 |= val;
	dtvscanpwm_wr(mdp, reg, val16);
}

static void mtk_scanpwm_oen(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 bitop_tmp;

	if (((mdp->data->scanpwm_attribute)&OEN_ATTR)) {
		bitop_tmp = dtvscanpwm_rd(mdp, REG_002C_PWM_SCAN0);

		if (data)
			bitop_tmp |= 1;
		else
			bitop_tmp &= ~1;

		dtvscanpwm_wr(mdp, REG_002C_PWM_SCAN0, bitop_tmp);
	}
}

static void mtk_scanpwm_cken(struct mtk_scanpwm *mdp)
{
	u16 reg_tmp;

	reg_tmp = mtk_readw_relaxed(mdp->base_n + REG_002C_PWM_SCAN0);
	reg_tmp |= PWM_CLK_SET;
	mtk_writew_relaxed(mdp->base_n + REG_002C_PWM_SCAN0, reg_tmp);
	reg_tmp = mtk_readw_relaxed(mdp->base_ckgen + REG_0004_PWM);
	reg_tmp |= CKEN_ALL_EN;
	mtk_writew_relaxed(mdp->base_ckgen + REG_0004_PWM, reg_tmp);
}

static void mtk_scanpwm_period(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 period_h;

	if (((mdp->data->scanpwm_attribute)&PERIOD_ATTR)) {
		if (data > 1)
			data -= 1;
		period_h = (data >> PWM_HALF_WORD);
		dtvscanpwm_wr(mdp, REG_0000_PWM_SCAN0, data);
		dtvscanpwm_wr(mdp, REG_0004_PWM_SCAN0, period_h);
	}
}

static void mtk_scanpwm_duty(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 duty_h;

	if (((mdp->data->scanpwm_attribute)&DUTY_ATTR)) {
		if (data > 1)
			data -= 1;
		duty_h = (data >> PWM_HALF_WORD);
		dtvscanpwm_wr(mdp, REG_0008_PWM_SCAN0, data);
		dtvscanpwm_wr(mdp, REG_000C_PWM_SCAN0, duty_h);
	}
}

static void mtk_scanpwm_div(struct mtk_scanpwm *mdp, unsigned int data)
{
	if (((mdp->data->scanpwm_attribute)&DIV_ATTR)) {
		if (data > 1)
			data -= 1;
		dtvscanpwm_wr(mdp, REG_0018_PWM_SCAN0, data);
	}
}

static void mtk_scanpwm_shift(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 shift_h;

	if (((mdp->data->scanpwm_attribute)&OFFSET_ATTR)) {
		if (data > 1)
			data -= 1;
		shift_h = (data >> PWM_HALF_WORD);
		dtvscanpwm_wr(mdp, REG_0020_PWM_SCAN0, data);
		dtvscanpwm_wr(mdp, REG_0024_PWM_SCAN0, shift_h);
	}
}

static void mtk_scanpwm_rstcnt(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 sync_ctrl;

	if (((mdp->data->scanpwm_attribute)&RSTCNT_ATTR)) {
		sync_ctrl = dtvscanpwm_rd(mdp, REG_0028_PWM_SCAN0);
		sync_ctrl |= (data&0xF) << RESET_CNT_SET;
		dtvscanpwm_wr(mdp, REG_0028_PWM_SCAN0, sync_ctrl);
	}
}

static void mtk_scanpwm_rstmux(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 sync_ctrl;
	bool rstmuxbit;

	if (((mdp->data->scanpwm_attribute)&RSTMUX_ATTR)) {
		rstmuxbit = data&0x1;
		sync_ctrl = dtvscanpwm_rd(mdp, REG_0028_PWM_SCAN0);
		if (rstmuxbit)
			sync_ctrl |= RESET_MUX_SET;
		else
			sync_ctrl &= ~RESET_MUX_SET;

		dtvscanpwm_wr(mdp, REG_0028_PWM_SCAN0, sync_ctrl);
	}
}

static void mtk_scanpwm_pol(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	if (((mdp->data->scanpwm_attribute)&POL_ATTR)) {
		reg_tmp = dtvscanpwm_rd(mdp, REG_001C_PWM_SCAN0);
		if (data)
			reg_tmp |= (0x1 << BYTE_SHIFT);
		else
			reg_tmp &= ~(0x1 << BYTE_SHIFT);
		dtvscanpwm_wr(mdp, REG_001C_PWM_SCAN0, reg_tmp);
	}
}

static void mtk_scanpwm_vsync(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	if (((mdp->data->scanpwm_attribute)&RESET_VSYNC_ATTR)) {
		reg_tmp = dtvscanpwm_rd(mdp, REG_001C_PWM_SCAN0);
		if (data) {
			reg_tmp |= PWM_VDBEN_SET;
			reg_tmp |= PWM_VREST_SET;
			reg_tmp &= ~PWM_DBEN_SET;
		} else {
			reg_tmp &= ~PWM_VDBEN_SET;
			reg_tmp &= ~PWM_VREST_SET;
			reg_tmp |= PWM_DBEN_SET;
		}
		dtvscanpwm_wr(mdp, REG_001C_PWM_SCAN0, reg_tmp);
	}
}

static void mtk_scanpwm_vdben_sw(struct mtk_scanpwm *mdp, unsigned int data)
{
	u16 reg_tmp;

	reg_tmp = dtvscanpwm_rd(mdp, REG_001C_PWM_SCAN0);
	if (data)
		reg_tmp |= PWM_VDBEN_SW_SET;
	else
		reg_tmp &= ~PWM_VDBEN_SW_SET;

	dtvscanpwm_wr(mdp, REG_001C_PWM_SCAN0, reg_tmp);
}

static int mtk_scanpwm_config(struct pwm_chip *chip,
							struct pwm_device *scanpwm,
						int duty_t, int period_t)
{
	struct mtk_scanpwm *mdp = to_mtk_scanpwm(chip);
	int duty_set, period_set, offset_set;

	/*12hz XTAL timing is nsec*/
	period_set = (XTAL_MHZ*period_t)/FREQ_SCALE;
	duty_set = (XTAL_MHZ*duty_t)/FREQ_SCALE;
	offset_set = (XTAL_MHZ*(mdp->data->shift))/FREQ_SCALE;
	mtk_scanpwm_shift(mdp, offset_set);
	mtk_scanpwm_rstcnt(mdp, mdp->data->rstcnt);
	mtk_scanpwm_rstmux(mdp, mdp->data->rst_mux);
	mtk_scanpwm_vsync(mdp, mdp->data->rst_vsync);
	mtk_scanpwm_vdben_sw(mdp, 0);
	mtk_scanpwm_period(mdp, period_set);
	mtk_scanpwm_duty(mdp, duty_set);
	mtk_scanpwm_div(mdp, mdp->data->div);
	mtk_scanpwm_vdben_sw(mdp, 1);
	mtk_scanpwm_oen(mdp, mdp->data->enable);
	mtk_scanpwm_pol(mdp, mdp->data->pol);

	return 0;
}

static int mtk_scanpwm_enable(struct pwm_chip *chip, struct pwm_device *scanpwm)
{
	struct mtk_scanpwm *mdp = to_mtk_scanpwm(chip);

	mdp->data->scanpwm_attribute = OEN_ATTR;
	mtk_scanpwm_oen(mdp, 1);
	return 0;
}

static void mtk_scanpwm_disable(struct pwm_chip *chip, struct pwm_device *scanpwm)
{
	struct mtk_scanpwm *mdp = to_mtk_scanpwm(chip);

	mdp->data->scanpwm_attribute = OEN_ATTR;
	if (mdp->data->pol == 1) {
		mtk_scanpwm_oen(mdp, 1);
		mtk_scanpwm_duty(mdp, 0);
	} else {
		mtk_scanpwm_oen(mdp, 0);
	}
}

static void mtk_scanpwm_get_state(struct pwm_chip *chip,
							struct pwm_device *scanpwm,
							struct pwm_state *state)
{
	struct mtk_scanpwm *mdp = to_mtk_scanpwm(chip);
	int    scanpwm_attr_get;

	state->period = mdp->data->period;
	state->duty_cycle = mdp->data->duty_cycle;
	scanpwm_attr_get = dtvscanpwm_rd(mdp, REG_001C_PWM_SCAN0);
	state->polarity = (((scanpwm_attr_get >> 8)&0x1)?1:0);

	scanpwm_attr_get = dtvscanpwm_rd(mdp, REG_002C_PWM_SCAN0);
	scanpwm_attr_get &= 1;
	state->enabled = (scanpwm_attr_get?1:0);
}

static int mtk_scanpwm_polarity(struct pwm_chip *chip,
				struct pwm_device *scanpwm, enum pwm_polarity polarity)
{
	struct mtk_scanpwm *mdp = to_mtk_scanpwm(chip);

	mtk_scanpwm_pol(mdp, polarity);
	return 0;
}


static const struct pwm_ops mtk_scanpwm_ops = {
	.config = mtk_scanpwm_config,
	.enable = mtk_scanpwm_enable,
	.disable = mtk_scanpwm_disable,
	.get_state = mtk_scanpwm_get_state,
	.set_polarity = mtk_scanpwm_polarity,
	.owner = THIS_MODULE,
};

static ssize_t vsync_reset_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->rst_vsync);
}

static ssize_t vsync_reset_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 0, &val);

	mdp->data->rst_vsync = val;
	mtk_scanpwm_vsync(mdp, val);
	return ret;
}
static DEVICE_ATTR_RW(vsync_reset);

static ssize_t div_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->div);
}

static ssize_t div_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 0, &val);

	mdp->data->div = val;
	mtk_scanpwm_div(mdp, val);
	return ret;
}
static DEVICE_ATTR_RW(div);

static ssize_t shift_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->shift);
}

static ssize_t shift_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 0, &val);

	mdp->data->shift = val;
	mtk_scanpwm_shift(mdp, val);
	return ret;
}
static DEVICE_ATTR_RW(shift);

static ssize_t rstcnt_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->rstcnt);
}

static ssize_t rstcnt_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 0, &val);

	mdp->data->rstcnt = val;
	mtk_scanpwm_rstcnt(mdp, val);
	return ret;
}
static DEVICE_ATTR_RW(rstcnt);

static ssize_t rstmux_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->rst_mux);
}

static ssize_t rstmux_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 0, &val);

	mdp->data->rst_mux = val;
	mtk_scanpwm_rstmux(mdp, val);
	return ret;
}
static DEVICE_ATTR_RW(rstmux);


static ssize_t fullscale_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);
	u32 reg_tmp;

	reg_tmp = dtvscanpwm_rd(mdp, REG_0000_PWM_SCAN0);
	reg_tmp |= (dtvscanpwm_rd(mdp, REG_0004_PWM_SCAN0) << PWM_HALF_WORD);
	reg_tmp = ((reg_tmp + 1) * FREQ_SCALE)/XTAL_MHZ;
	mdp->data->period = reg_tmp;

	return scnprintf(buf, LENSIZE, "%d\n", reg_tmp);
}
static DEVICE_ATTR_RO(fullscale);

static ssize_t curduty_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(child);
	u32 reg_tmp;

	if (!mdp->data->duty_cycle) {
		reg_tmp = dtvscanpwm_rd(mdp, REG_0008_PWM_SCAN0);
		reg_tmp |= (dtvscanpwm_rd(mdp, REG_000C_PWM_SCAN0) << PWM_HALF_WORD);
		mdp->data->duty_cycle = (reg_tmp * FREQ_SCALE)/XTAL_MHZ;
	}

	return scnprintf(buf, LENSIZE, "%d\n", mdp->data->duty_cycle);
}
static DEVICE_ATTR_RO(curduty);


static struct attribute *pwm_extend_attrs[] = {
	&dev_attr_vsync_reset.attr,
	&dev_attr_div.attr,
	&dev_attr_shift.attr,
	&dev_attr_rstmux.attr,
	&dev_attr_rstcnt.attr,
	&dev_attr_fullscale.attr,
	&dev_attr_curduty.attr,
	NULL,
};
static const struct attribute_group pwm_extend_attr_group = {
	.name = "pwm_extend",
	.attrs = pwm_extend_attrs
};


int mtk_scanpwm_set_attr(u8 channel, struct mtk_scanpwm_dat *scandat)
{
	struct pwm_chip *pwmchip = _ptr_array[channel];
	struct mtk_scanpwm *mdp = to_mtk_scanpwm(pwmchip);

	mdp->data->scanpwm_attribute = scandat->scanpwm_attribute;
	if ((scandat->scanpwm_attribute)&OFFSET_ATTR)
		mdp->data->shift = scandat->shift;
	if ((scandat->scanpwm_attribute)&DIV_ATTR)
		mdp->data->div = scandat->div;
	if ((scandat->scanpwm_attribute)&POL_ATTR)
		mdp->data->pol = scandat->pol;
	if ((scandat->scanpwm_attribute)&RSTMUX_ATTR)
		mdp->data->rst_mux = scandat->rst_mux;
	if ((scandat->scanpwm_attribute)&RESET_VSYNC_ATTR)
		mdp->data->rst_vsync = scandat->rst_vsync;
	if ((scandat->scanpwm_attribute)&OEN_ATTR)
		mdp->data->enable = scandat->enable;
	if ((scandat->scanpwm_attribute)&RSTCNT_ATTR)
		mdp->data->rstcnt = scandat->rstcnt;
	if ((scandat->scanpwm_attribute)&DUTY_ATTR)
		mdp->data->duty_cycle = scandat->duty_cycle;
	if ((scandat->scanpwm_attribute)&PERIOD_ATTR)
		mdp->data->period = scandat->period;
	mtk_scanpwm_config(pwmchip, NULL, mdp->data->duty_cycle, mdp->data->period);

	return 0;
}
EXPORT_SYMBOL(mtk_scanpwm_set_attr);

int mtk_scanpwm_get_attr(u8 channel, struct mtk_scanpwm_dat *scandat)
{
	struct pwm_chip *pwmchip = _ptr_array[channel];
	struct mtk_scanpwm *mdp = to_mtk_scanpwm(pwmchip);
	u32 reg_tmp;

	printk("@@@@mtk_scanpwm_get_attr:period:%d\n",mdp->data->period);
	if (!mdp->data->period) {
		reg_tmp = dtvscanpwm_rd(mdp, REG_0000_PWM_SCAN0);
		reg_tmp |= (dtvscanpwm_rd(mdp, REG_0004_PWM_SCAN0) << PWM_HALF_WORD);
		reg_tmp = ((reg_tmp+1) * FREQ_SCALE)/XTAL_MHZ;
		mdp->data->period = reg_tmp;
	}
	reg_tmp = dtvscanpwm_rd(mdp, REG_0008_PWM_SCAN0);
	reg_tmp |= (dtvscanpwm_rd(mdp, REG_000C_PWM_SCAN0) << PWM_HALF_WORD);
	mdp->data->duty_cycle = ((reg_tmp + 1) * FREQ_SCALE)/XTAL_MHZ;
	scandat->shift = mdp->data->shift;
	scandat->div = mdp->data->div;
	scandat->pol = mdp->data->pol;
	scandat->rst_mux = mdp->data->rst_mux;
	scandat->rst_vsync = mdp->data->rst_vsync;
	//scandat->enable = mdp->data->enable;
	reg_tmp = dtvscanpwm_rd(mdp, REG_002C_PWM_SCAN0);
	reg_tmp &= 1;
	mdp->data->enable = reg_tmp;
	scandat->enable = reg_tmp;
	scandat->rstcnt = mdp->data->rstcnt;
	scandat->period = mdp->data->period;
	scandat->duty_cycle = mdp->data->duty_cycle;
	return 0;
}
EXPORT_SYMBOL(mtk_scanpwm_get_attr);

static int mtk_scanpwm_probe(struct platform_device *pdev)
{
	struct mtk_scanpwm *mdp;
	int ret;
	struct device_node *np = NULL;
	struct mtk_scanpwm_dat *scandat;
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
	mdp->chip.ops = &mtk_scanpwm_ops;
	/*It will return error without this setting*/
	mdp->chip.npwm = 1;
	mdp->chip.base = -1;

	err = of_property_read_u32(pdev->dev.of_node,
				"div", &mdp->data->div);
	if (err) {
		dev_notice(&pdev->dev, "could not get div resource\n");
		return -EINVAL;
	}

	err = of_property_read_u32(pdev->dev.of_node,
				"period_time", &mdp->data->period);
	if (err)
		dev_warn(&pdev->dev, "could not get period_time ns resource\n");

	err = of_property_read_u32(pdev->dev.of_node,
				"channel", &mdp->channel);
	if (err)
		dev_err(&pdev->dev, "could not get channel resource\n");


	if (of_get_property(np, "vsync_en", NULL))
		mdp->data->rst_vsync = true;
	else
		mdp->data->rst_vsync = 0;

	err = of_property_read_u32(pdev->dev.of_node,
				"pol", &mdp->data->pol);
	if (err)
		dev_info(&pdev->dev, "could not get pol resource\n");

	mdp->base = of_iomap(np, 0);
	mdp->base_n = of_iomap(np, 1);
	mdp->base_ckgen = of_iomap(np, CKGEN_BASE);

	if (!mdp->base)
		return -EINVAL;
	mtk_scanpwm_cken(mdp);
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
		dev_err(&pdev->dev, "scanpwmchip_add() failed: %d\n", ret);

	platform_set_drvdata(pdev, mdp);

	ret = sysfs_create_group(&pdev->dev.kobj, &pwm_extend_attr_group);
	if (ret < 0)
		dev_info(&pdev->dev, "sysfs_create_group failed: %d\n", ret);

	return ret;
}

static int mtk_scanpwm_remove(struct platform_device *pdev)
{
	struct mtk_scanpwm *mdp = platform_get_drvdata(pdev);

	if (!(pdev->name) ||
		strcmp(pdev->name, "mediatek,mt5896-scanpwm") || pdev->id != 0)
		return -1;

	pwmchip_remove(&mdp->chip);
	clk_unprepare(mdp->clk_gate_xtal);
	clk_unprepare(mdp->clk_gate_pwm);

	pdev->dev.platform_data = NULL;
	return 0;
}
#ifdef CONFIG_PM
static int mtk_dtv_pwmscan_suspend(struct device *pdev)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(pdev);

	if (!mdp)
		return -EINVAL;
	if (!IS_ERR_OR_NULL(mdp->clk_gate_xtal))
		clk_disable_unprepare(mdp->clk_gate_xtal);

	if (!IS_ERR_OR_NULL(mdp->clk_gate_pwm))
		clk_disable_unprepare(mdp->clk_gate_pwm);

	return 0;
}

static int mtk_dtv_pwmscan_resume(struct device *pdev)
{
	struct mtk_scanpwm *mdp = dev_get_drvdata(pdev);
	int ret;

	if (!mdp)
		return -EINVAL;

	mdp->data->scanpwm_attribute = OEN_ATTR;
	if (!IS_ERR_OR_NULL(mdp->clk_gate_xtal)) {
		ret = clk_prepare_enable(mdp->clk_gate_xtal);
		if (ret)
			dev_err(pdev, "resume failed to enable xtal_12m2pwm: %d\n", ret);
	}
	if (!IS_ERR_OR_NULL(mdp->clk_gate_pwm)) {
		ret = clk_prepare_enable(mdp->clk_gate_pwm);
		if (ret)
			dev_err(pdev, "resume failed to enable pwm_ck: %d\n", ret);
	}
	mtk_scanpwm_cken(mdp);
	mtk_scanpwm_oen(mdp, 1);

	return 0;
}
#endif

#if defined(CONFIG_OF)
static const struct of_device_id mtkscanpwm_of_device_ids[] = {
		{.compatible = "mediatek,mt5896-scanpwm" },
		//Todo
		/*Add Multiple PWM HW*/
		{},
};
#endif

/*
#ifdef CONFIG_PM
static SIMPLE_DEV_PM_OPS(pwm_pwmscan_pm_ops,
		mtk_dtv_pwmscan_suspend, mtk_dtv_pwmscan_resume);
#endif
*/
#ifdef CONFIG_PM
static const struct dev_pm_ops pwm_pwmscan_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(mtk_dtv_pwmscan_suspend, mtk_dtv_pwmscan_resume)
};
#endif
static struct platform_driver mtk_scanpwm_driver = {
	.probe		= mtk_scanpwm_probe,
	.remove		= mtk_scanpwm_remove,
	.driver		= {
#if defined(CONFIG_OF)
	.of_match_table = mtkscanpwm_of_device_ids,
#endif
#ifdef CONFIG_PM
	.pm	= &pwm_pwmscan_pm_ops,
#endif
	.name   = "mediatek,mt5896-scanpwm",
	.owner  = THIS_MODULE,
}
};

static int __init mtk_scanpwm_init_module(void)
{
	int retval = 0;

	retval = platform_driver_register(&mtk_scanpwm_driver);

	return retval;
}

static void __exit mtk_scanpwm_exit_module(void)
{
	platform_driver_unregister(&mtk_scanpwm_driver);
}

module_init(mtk_scanpwm_init_module);
module_exit(mtk_scanpwm_exit_module);


MODULE_AUTHOR("Owen.Tseng@mediatek.com");
MODULE_DESCRIPTION("scanpwm driver");
MODULE_LICENSE("GPL");

