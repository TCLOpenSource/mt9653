// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * linux/drivers/leds-mtk-irtx.c
 *
 * Copyright (c) Mediatek
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mutex.h>

#define TAG "[IRLED] "
#define LOGI(format, ...) do { \
	if (_verbose) \
		pr_info(TAG "%s:%d " format, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#define LOGE(format, ...) pr_err(TAG "%s:%d " format, __func__, __LINE__, ##__VA_ARGS__)

struct led_mtk_irtx_priv;

struct led_mtk_irtx {
	const char	*name;
	const char	*default_trigger;
	u32 brightness;
	u32 max_brightness;
	u32 max_lightness;
};

struct led_mtk_irtx_data {
	struct led_classdev	cdev;
	struct led_mtk_irtx_priv *priv;
	u32 max_lightness;
	/* struct ir_tx_device *irtx; */
};

struct led_mtk_irtx_priv {
	struct mutex lock;
	void __iomem *base;
	u32 clock;
	u32 bit_time[4];
	u32 unit_count[2];
	int num_leds;
	struct led_mtk_irtx_data leds[0];
};

static int _verbose;
static int _inv = 1;
static int _clk;
static int _pre_scale = 100;
module_param_named(verbose, _verbose, int, 0644);
module_param_named(inv, _inv, int, 0644);
module_param_named(clk, _clk, int, 0644);
module_param_named(pre_scale, _pre_scale, int, 0644);

static ssize_t max_lightness_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct led_mtk_irtx_data *led_dat =
		container_of(led_cdev, struct led_mtk_irtx_data, cdev);
	ssize_t ret = 0;

	ret += scnprintf(buf, PAGE_SIZE, "%u\n", led_dat->max_lightness);

	return ret;
}

static ssize_t max_lightness_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct led_mtk_irtx_data *led_dat =
		container_of(led_cdev, struct led_mtk_irtx_data, cdev);
	unsigned long max_lightness;

	if (!kstrtoul(buf, 0, &max_lightness)) {
		if (max_lightness > 100)
			max_lightness = 100;
		led_dat->max_lightness = max_lightness;
	}

	return size;
}

static DEVICE_ATTR_RW(max_lightness);

static struct attribute *max_lightness_attrs[] = {
	&dev_attr_max_lightness.attr,
	NULL
};
ATTRIBUTE_GROUPS(max_lightness);

/* IRTX part begin */
static u32 _div_round(u32 x, u32 y)
{
	return (x + (y/2)) / y;
}

static u16 irtx_read(struct led_mtk_irtx_priv *priv, u32 reg)
{
	return readl((void *)(priv->base+reg));
}

static void irtx_write(struct led_mtk_irtx_priv *priv, u32 reg, u16 value)
{
	LOGI("reg=0x%04x , value=0x%04x\n", reg, value);
	writel(value, (void *)(priv->base+reg));
}

static int _irtx_init(struct led_mtk_irtx_priv *priv)
{
	u32 clock, th, tl;
	const int scale = 10; /* scale MHz to 100Khz for minor tuning */

	irtx_write(priv, 0x88, 0);
	irtx_write(priv, 0x88, 3 | ((!!_inv)<<3)); /* release reset */
	irtx_write(priv, 0x80, 0); /* clkdiv */
	irtx_write(priv, 0x84, 0x0101); /* carrier, don't care */


	clock = priv->clock / (1000000 / scale);
	th = _div_round(priv->bit_time[0] * clock, 1000 * scale);
	tl = _div_round(priv->bit_time[1] * clock, 1000 * scale);
	irtx_write(priv, 0x00, th);
	irtx_write(priv, 0x04, tl);
	priv->unit_count[0] = th+tl;
	th = _div_round(priv->bit_time[2] * clock, 1000 * scale);
	tl = _div_round(priv->bit_time[3] * clock, 1000 * scale);
	irtx_write(priv, 0x08, th);
	irtx_write(priv, 0x0c, tl);
	priv->unit_count[1] = th+tl;
	/* >80ns reset signal */
	irtx_write(priv, 0x10, th*_pre_scale);
	irtx_write(priv, 0x14, tl*_pre_scale);

	irtx_write(priv, 0x70, 0x000a); /* unit_value */

	return 0;
}

static void __irtx_send(struct led_mtk_irtx_priv *priv)
{
	const u32 t0 = priv->unit_count[0], t1 = priv->unit_count[1];
	u32 data, index;
	u32 total;
	int i;

	_irtx_init(priv);
	irtx_write(priv, 0x204, 0x5100);
	irtx_write(priv, 0x208, 0);

	/* first put a >80ns reset signal */
	data = 2;
	index = 4;
	total = t0 + t1;

	for (i = 0; i < priv->num_leds; i++) {
		int mask;
		/* send GRB 24bit */
		u32 rgb = priv->leds[i].cdev.brightness;
		u32 grb;
		u32 r, g, b;

		r = (rgb >> 16) & 0xff;
		g = (rgb >> 8) & 0xff;
		b = (rgb >> 0) & 0xff;

		grb = (g << 16) | (r << 8) | b;
		LOGI("rgb=0x%06x grb=0x%06x\n", rgb, grb);
		for (mask = (1L<<23); mask; mask >>= 1) {
			int bits = !!(grb&mask);

			total += bits ? t1 : t0;
			data = data | (bits << index);
			index += 4;
			if (index == 16) {
				irtx_write(priv, 0x218, data);
				data = 0;
				index = 0;
			}
		}
	}
	data = data | (0xe << index);
	irtx_write(priv, 0x218, data);

	irtx_write(priv, 0x78, total >> 16);
	irtx_write(priv, 0x7c, total & 0xffff);

	irtx_write(priv, 0x8c, 1); // trigger irtx send

	usleep_range(500, 600);
	if ((irtx_read(priv, 0x90) & 0x01) == 0)
		pr_warn("leds_mtk_irtx: ready bit not done. skip\n");
}
/* IRTX part end */

static void __led_irtx_set(struct led_mtk_irtx_data *led_dat)
{
	int i;
	struct led_mtk_irtx_priv *priv = led_dat->priv;
	char buf[128];
	int len = 0;

	mutex_lock(&priv->lock);
	for (i = 0; i < priv->num_leds; i++) {
		struct led_classdev *cdev = &priv->leds[i].cdev;

		len += scnprintf(buf+len, sizeof(buf)-len, " %s:0x%08x",
				cdev->name, cdev->brightness);
	}
	pr_info("leds_mtk_irtx: sending %s\n", buf);

	__irtx_send(priv);
	mutex_unlock(&priv->lock);
}

static int led_mtk_irtx_set(struct led_classdev *led_cdev,
			   enum led_brightness brightness)
{
	struct led_mtk_irtx_data *led_dat =
		container_of(led_cdev, struct led_mtk_irtx_data, cdev);

	LOGI("name: %s set brightness=0x%08x\n",
			led_cdev->name, brightness);

	if (brightness <= led_cdev->max_brightness)
		led_cdev->brightness = brightness;

	__led_irtx_set(led_dat);

	return 0;
}

static inline size_t sizeof_leds_priv(int num_leds)
{
	return sizeof(struct led_mtk_irtx_priv) +
			  (sizeof(struct led_mtk_irtx_data) * num_leds);
}

static void led_mtk_irtx_cleanup(struct led_mtk_irtx_priv *priv)
{
	while (priv->num_leds--)
		led_classdev_unregister(&priv->leds[priv->num_leds].cdev);
}

static int led_mtk_irtx_add(struct device *dev, struct led_mtk_irtx_priv *priv,
			   struct led_mtk_irtx *led, struct device_node *child)
{
	struct led_mtk_irtx_data *led_data = &priv->leds[priv->num_leds];
	int ret;

	led_data->cdev.name = led->name;
	led_data->cdev.default_trigger = led->default_trigger;
	led_data->cdev.brightness = led->brightness;
	led_data->cdev.max_brightness = led->max_brightness;
	led_data->cdev.flags = LED_CORE_SUSPENDRESUME;
	led_data->priv = priv;
	led_data->max_lightness = led->max_lightness;
	led_data->cdev.brightness_set_blocking = led_mtk_irtx_set;
	led_data->cdev.groups = max_lightness_groups;

	ret = led_classdev_register(dev, &led_data->cdev);
	if (ret == 0) {
		priv->num_leds++;
	} else {
		dev_err(dev, "failed to register mtk led for %s: %d\n",
			led->name, ret);
	}

	return ret;
}

static int led_mtk_irtx_create_of(struct device *dev, struct led_mtk_irtx_priv *priv)
{
	struct device_node *child;
	struct led_mtk_irtx led;
	int ret = 0;

	memset(&led, 0, sizeof(led));

	for_each_child_of_node(dev->of_node, child) {
		/* set default value to brightness and max_brightness */
		led.brightness = LED_OFF;
		led.max_brightness = 0xffffffff;
		led.max_lightness = 100;

		led.name = of_get_property(child, "label", NULL) ? :
			child->name;

		led.default_trigger = of_get_property(child,
						"linux,default-trigger", NULL);
		of_property_read_u32(child, "brightness", &led.brightness);
		of_property_read_u32(child, "max-brightness", &led.max_brightness);
		of_property_read_u32(child, "max-lightness", &led.max_lightness);

		LOGI("name=%s brightness=0x%08x max-brightness=0x%08x max-lightness=0x%08x\n",
				led.name, led.brightness, led.max_brightness, led.max_lightness);

		ret = led_mtk_irtx_add(dev, priv, &led, child);
		if (ret) {
			of_node_put(child);
			break;
		}
	}
	led_mtk_irtx_set(&priv->leds[priv->num_leds-1].cdev,
			priv->leds[priv->num_leds-1].cdev.brightness);

	return ret;
}

static int led_mtk_irtx_probe(struct platform_device *pdev)
{
	struct led_mtk_irtx_priv *priv;
	struct resource *res;
	int count;
	int ret = 0;

	count = of_get_child_count(pdev->dev.of_node);

	if (!count)
		return -EINVAL;

	priv = devm_kzalloc(&pdev->dev, sizeof_leds_priv(count),
				GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	mutex_init(&priv->lock);

	if (device_property_read_u32_array(&pdev->dev, "bit-time",
				priv->bit_time, 4)) {
		pr_info("Failed to load bit-time from dtb. set default {300, 900}, {600, 600}\n");
		priv->bit_time[0] = 300;
		priv->bit_time[1] = 900;
		priv->bit_time[2] = 600;
		priv->bit_time[3] = 600;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));

	if (_clk) {
		priv->clock = _clk;
	} else {
		priv->clock = 216*1000*1000; /* default 216MHz */
		device_property_read_u32(&pdev->dev, "clock-frequency", &priv->clock);
		_clk = priv->clock;
	}

	pr_info("leds_mtk_irtx: probe clk=%d bit-time={%d %d %d %d}\n",
			priv->clock, priv->bit_time[0], priv->bit_time[1],
			priv->bit_time[2], priv->bit_time[3]);

	ret = led_mtk_irtx_create_of(&pdev->dev, priv);

	if (ret) {
		led_mtk_irtx_cleanup(priv);
		return ret;
	}

	platform_set_drvdata(pdev, priv);

	return 0;
}

static int led_mtk_irtx_remove(struct platform_device *pdev)
{
	struct led_mtk_irtx_priv *priv = platform_get_drvdata(pdev);

	led_mtk_irtx_cleanup(priv);

	return 0;
}

static const struct of_device_id of_mtk_irtx_leds_match[] = {
	{ .compatible = "mtk-irtx-leds", },
	{},
};
MODULE_DEVICE_TABLE(of, of_mtk_irtx_leds_match);

static struct platform_driver led_mtk_irtx_driver = {
	.probe		= led_mtk_irtx_probe,
	.remove		= led_mtk_irtx_remove,
	.driver		= {
		.name	= "leds_mtk_irtx",
		.of_match_table = of_mtk_irtx_leds_match,
	},
};

module_platform_driver(led_mtk_irtx_driver);

MODULE_AUTHOR("Polar Chou<polar.chou@mediatek.com>");
MODULE_DESCRIPTION("Mediatek TV IRTX LED driver");
MODULE_LICENSE("GPL v2");
