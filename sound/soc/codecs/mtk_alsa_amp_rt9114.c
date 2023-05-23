// SPDX-License-Identifier: GPL-2.0
/*
 * mtk_alsa_amp_rt9114.c -- amp rt9114 driver
 *
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/gpio/consumer.h>
#include <sound/soc.h>
#include <sound/control.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/vmalloc.h>


#include "mtk_alsa_amp_rt9114.h"
#include "mtk_ini_parser.h"

u8 AmpInitTblRT9114b[] = {
//DataLen	Address	DataN	DataN+1...
	/* don't need to do sw reset here, hw reset is applied */
	2,	0x07,	0x01, 0x10,	// SW Volume default +7 dB
	4,	0x62,	0x00, 0x00, 0x00, 0xA0,
	1,	0x1A,	0x41,
	1,	0x05,	0x02,
	1,	0x73,	0x30,
	0x00,
};

static void mtk_amp_delaytask_us(unsigned int u32Us)
{
	/*
	 * Refer to Documentation/timers/timers-howto.txt
	 * Non-atomic context
	 *		< 10us	:	udelay()
	 *	10us ~ 20ms	:	usleep_range()
	 *		> 10ms	:	msleep() or msleep_interruptible()
	 */

	if (u32Us < 10UL)
		udelay(u32Us);
	else if (u32Us < 20UL * 1000UL)
		usleep_range(u32Us, u32Us + 1);
	else
		msleep_interruptible((unsigned int)(u32Us / 1000UL));
}

static struct snd_soc_dai_driver amp_rt9114_dais[] = {
	{
		.name = "AMP_RT9114",
		.id = 0,
		.playback = {
			.stream_name = "AMP RT9114",
		},
	},
};

static int amp_init(struct amp_rt9114_priv *priv, unsigned char *InitTable)
{
	u8 *pu8Str = InitTable;
	u8 *buf;
	u8 reg;
	u8 size;
	int ret;

	do {
		size = *pu8Str;
		pu8Str++;

		reg = *pu8Str;
		pu8Str++;

		buf = pu8Str;
		ret = regmap_raw_write(priv->regmap, reg, buf, size);
		if (ret < 0) {
			dev_err(priv->dev, "i2c write fail\n");
			return -1;
		}

		pu8Str = pu8Str + size;
	} while (*pu8Str != 0);

	return 0;
}

static int amp_rt9114_reset(struct snd_soc_component *component, bool bEnable)
{
	struct amp_rt9114_priv *priv =
			snd_soc_component_get_drvdata(component);

	if (priv->amp_reset != NULL) {
		if (bEnable ^ priv->amp_reset_invertor)
			gpiod_set_value(priv->amp_reset, 1);
		else
			gpiod_set_value(priv->amp_reset, 0);
	}

	return 0;
}

static int amp_rt9114_mute(struct snd_soc_component *component, bool bEnable)
{
	struct amp_rt9114_priv *priv =
			snd_soc_component_get_drvdata(component);

	if (bEnable ^ priv->amp_mute_invertor)
		gpiod_set_value(priv->amp_mute, 1);
	else
		gpiod_set_value(priv->amp_mute, 0);

	return 0;
}

static int amp_rt9114_init(struct amp_rt9114_priv *priv, unsigned char *InitTable)
{
	int ret = 0;

	ret = amp_init(priv, InitTable);
	if (ret < 0)
		dev_err(priv->dev, "[AMP 9114]Init fail\n");
	else
		dev_err(priv->dev, "[AMP 9114]Init success\n");

	return ret;
}

static int amp_rt9114_probe(struct snd_soc_component *component)
{
	struct amp_rt9114_priv *priv =
				snd_soc_component_get_drvdata(component);
	int ret = 0;
	int retry_time = 5;
	const struct firmware *fw = NULL;

	if (priv->mtk_ini && (priv->init_table == NULL)) {
		ret = request_firmware_direct(&fw, priv->mtk_ini, priv->dev);
		if (ret < 0)
			dev_err(priv->dev, "Err! load ext ini fail, fallback to internal init\n");
		else {
			priv->init_table = mtk_parser(fw);
			if (priv->init_table)
				dev_err(priv->dev, "load ext ini success\n");
		}
		release_firmware(fw);
	}

	amp_rt9114_reset(component, 1);
	mtk_amp_delaytask_us(1000);
	amp_rt9114_mute(component, 0);
	mtk_amp_delaytask_us(1000);
	amp_rt9114_reset(component, 0);
	mtk_amp_delaytask_us(10*1000);

	do {
		mtk_amp_delaytask_us(2*1000);
		if (priv->init_table)
			ret = amp_rt9114_init(priv, priv->init_table);
		else
			ret = amp_rt9114_init(priv, AmpInitTblRT9114b);
		retry_time -= 1;
	} while ((ret < 0) && retry_time);

	priv->component = component;

	return ret;
}

static void amp_rt9114_remove(struct snd_soc_component *component)
{
	amp_rt9114_mute(component, 1);
	mtk_amp_delaytask_us(15*1000);
	amp_rt9114_reset(component, 1);
	mtk_amp_delaytask_us(1000);
}

static int amp_rt9114_mute_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
				snd_soc_kcontrol_component(kcontrol);
	struct amp_rt9114_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int val = ucontrol->value.integer.value[0];

	if (val < 0)
		return -EINVAL;

	dev_info(priv->dev, "set amp mute %d\n", val);

	amp_rt9114_mute(component, val);
	mtk_amp_delaytask_us(1000);

	return 0;
}

static int amp_rt9114_mute_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
				snd_soc_kcontrol_component(kcontrol);
	struct amp_rt9114_priv *priv = snd_soc_component_get_drvdata(component);
	int val;

	val = gpiod_get_value(priv->amp_mute);
	val = val ^ priv->amp_mute_invertor;

	dev_info(priv->dev, "get amp mute %d\n", val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int audio_amp_get_param(struct snd_kcontrol *kcontrol,
						unsigned int __user *data,
						unsigned int size)
{
	return 0;
}

static int audio_amp_put_param(struct snd_kcontrol *kcontrol,
						const unsigned int __user *data,
						unsigned int size)
{
	struct snd_soc_component *component =
				snd_soc_kcontrol_component(kcontrol);
	struct amp_rt9114_priv *priv = snd_soc_component_get_drvdata(component);
	int *param = priv->param;

	if (size > MTK_AMP_RT9114_PARAM_COUNT) {
		dev_err(priv->dev, "TLV kcontrol Size overflow\n");
		return -EINVAL;
	}

	if (!param)
		return -ENOMEM;

	if (copy_from_user(param, data, size * sizeof(int))) {
		dev_err(priv->dev, "copy_from_user fail\n");
		return -EFAULT;
	}

	return 0;
}

static const struct snd_kcontrol_new amp_rt9114_controls[] = {
	SOC_SINGLE_BOOL_EXT("AMP Mute", 0, amp_rt9114_mute_get,
				amp_rt9114_mute_put),
	SND_SOC_BYTES_TLV("AMP PARAMETER ARRAY", MTK_AMP_RT9114_PARAM_COUNT,
				audio_amp_get_param, audio_amp_put_param),
};

const struct snd_soc_component_driver amp_rt9114_component = {
	.name		= "amp-rt9114-platform",
	.probe		= amp_rt9114_probe,
	.remove		= amp_rt9114_remove,
	.controls	= amp_rt9114_controls,
	.num_controls	= ARRAY_SIZE(amp_rt9114_controls),
};

static const struct regmap_config rt9114_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xff,
};

static int amp_rt9114_i2c_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct amp_rt9114_priv *priv;
	struct device *dev = &client->dev;
	struct device_node *node;
	int ret;
	unsigned int temp;
	const char *name = NULL;

	node = dev->of_node;
	if (!node)
		return -ENODEV;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &client->dev;
	priv->client = client;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	i2c_set_clientdata(client, priv);

	/* GPIO */
	priv->amp_mute = devm_gpiod_get(&client->dev, "ampmute",
						GPIOD_OUT_LOW);
	if (IS_ERR(priv->amp_mute)) {
		dev_err(&client->dev, "Unable to retrieve gpio amp_mute\n");
		return PTR_ERR(priv->amp_mute);
	}

	priv->amp_reset = devm_gpiod_get(&client->dev, "ampreset",
						GPIOD_OUT_LOW);
	if (IS_ERR(priv->amp_reset)) {
		dev_err(&client->dev, "Unable to retrieve gpio amp_reset\n");
		return PTR_ERR(priv->amp_reset);
	}

	priv->regmap = devm_regmap_init_i2c(client, &rt9114_regmap);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	priv->param = devm_kzalloc(dev,
		(sizeof(int) * MTK_AMP_RT9114_PARAM_COUNT), GFP_KERNEL);
	if (!priv->param)
		return -ENOMEM;

	ret = devm_snd_soc_register_component(&client->dev,
					&amp_rt9114_component,
					amp_rt9114_dais,
					ARRAY_SIZE(amp_rt9114_dais));
	if (ret) {
		dev_err(dev, "AMP soc_register_component fail %d\n", ret);
		return ret;
	}

	/* parse dts */
	ret = of_property_read_u32(node, "mute-invertor", &temp);
	if (ret == 0)
		priv->amp_mute_invertor = temp;
	else
		dev_err(&client->dev, "Unable to get amp mute invertor\n");

	ret = of_property_read_u32(node, "reset-invertor", &temp);
	if (ret == 0)
		priv->amp_reset_invertor = temp;
	else
		dev_err(&client->dev, "Unable to get amp reset invertor\n");

	ret = of_property_read_string(node, "mtk_ini", &name);
	if (ret)
		dev_err(&client->dev, "[AMP 9114]can't get mtk ini path\n");

	priv->mtk_ini = name;

	return 0;
}

#ifdef CONFIG_PM
int amp_rt9114_pm_suspend(struct device *dev)
{
	struct amp_rt9114_priv *priv = dev_get_drvdata(dev);

	if (priv->suspended)
		return 0;

	priv->suspended = true;

	return 0;
}

int amp_rt9114_pm_resume(struct device *dev)
{
	struct amp_rt9114_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;

	if (!priv->suspended)
		return 0;

	amp_rt9114_probe(component);

	priv->suspended = false;

	return 0;
}

static const struct dev_pm_ops amp_rt9114_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(amp_rt9114_pm_suspend,
			   amp_rt9114_pm_resume)
};
#endif

static const struct of_device_id mtk_amp_dt_match[] = {
	{ .compatible = "mediatek, rt9114", },
	{ }
};
MODULE_DEVICE_TABLE(of, mtk_amp_dt_match);

static const struct i2c_device_id rt9114_i2c_id[] = {
	{ "mediatek,amp", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, rt9114_i2c_id);

static struct i2c_driver amp_rt9114_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "mtk-amp",
		.of_match_table	= of_match_ptr(mtk_amp_dt_match),
#ifdef CONFIG_PM
		.pm	= &amp_rt9114_pm_ops,
#endif
	},
	.probe		= amp_rt9114_i2c_probe,
	.id_table	= rt9114_i2c_id,
};

module_i2c_driver(amp_rt9114_i2c_driver);

MODULE_DESCRIPTION("ALSA SoC AMP RT9114 platform driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("AMP RT9114 driver");
