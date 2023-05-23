// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * JSA1136 Ambient Light Sensor Driver
 *
 * Copyright (c) 2021 MediaTek Inc.
 *
 * JSA1136 I2C slave address: 0x45
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/acpi.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/util_macros.h>
#include <linux/platform_device.h>

/* JSA1136 reg address */
#define JSA1136_ALS_CONF_REG    0x01
#define JSA1136_ALS_DATA_L_REG  0x06
#define JSA1136_ALS_DATA_H_REG  0x07

/* JSA1136 reg masks */
#define JSA1136_ALS_CONF_MASK		0xFF
#define JSA1136_ALS_RANGE_MASK		0x70
#define JSA1136_ALS_DATA1_MASK		0xFF
#define JSA1136_ALS_DATA2_MASK		0x0F

/* JSA1136 SYS CTRL bits */
#define JSA1136_SYS_ALS_MASK		0x01
#define JSA1136_SYS_ALS_ENABLE		0x02
#define JSA1136_SYS_ALS_DISABLE	    0x00

/* JSA1136 ALS RNG REG bits */
#define JSA1136_ALS_RANGE_0_2034    0x00
#define JSA1136_ALS_RANGE_0_1017    0x01
#define JSA1136_ALS_RANGE_0_508		0x02
#define JSA1136_ALS_RANGE_0_254		0x03
#define JSA1136_ALS_RANGE_0_127		0x04

#define JSA1136_DRIVER_NAME	"als-jsa1136"
#define JSA1136_REGMAP_NAME	"jsa1136_regmap"

#define ALS_DATA2_SHIFT 8
#define ALS_SCALE_SHIFT 4
#define THOUSAND 1000

struct JSA1136_data {
	struct i2c_client *client;
	struct mutex lock;
	u8 als_rng_idx;
	bool als_en;		/* ALS enable status */
	struct regmap *regmap;
};

/* ALS data range mapping */
static const int JSA1136_als_range_val[] = { 127, 254, 511, 1023, 2047 };

/* ALS range idx to luminace mapping */
static const int JSA1136_als_scale[] = { 31, 63, 125, 250, 500 };

/* Enables or disables ALS function based on status */
static int JSA1136_als_enable(struct JSA1136_data *data, u8 status)
{
	int ret;

	ret = regmap_update_bits(data->regmap, JSA1136_ALS_CONF_REG, JSA1136_SYS_ALS_MASK, status);
	if (ret < 0) {
		dev_err(&data->client->dev, "als enable err\n");
		return ret;
	}

	data->als_en = !!status;

	return 0;
}

static int JSA1136_read_als_data(struct JSA1136_data *data, unsigned int *val)
{
	int ret;
	unsigned int als_data_l, als_data_h;

	/* Read data */
	ret = regmap_read(data->regmap, JSA1136_ALS_DATA_L_REG, &als_data_l);

	if (ret < 0) {
		dev_err(&data->client->dev, "als low byte data read err\n");
		goto als_data_read_err;
	}

	/* Read data */
	ret = regmap_read(data->regmap, JSA1136_ALS_DATA_H_REG, &als_data_h);

	if (ret < 0) {
		dev_err(&data->client->dev, "als high byte data read err\n");
		goto als_data_read_err;
	}

	*val = (als_data_h << ALS_DATA2_SHIFT) | (als_data_l);

	return ret;
als_data_read_err:
	return ret;
}

static int JSA1136_get_lux(struct JSA1136_data *data, unsigned int *val)
{
	int ret;

	ret = JSA1136_read_als_data(data, val);

	if (ret < 0) {
		dev_err(&data->client->dev, "als data read err\n");
		goto als_get_lux_err;
	}

	*val = (*val * JSA1136_als_scale[data->als_rng_idx]) / THOUSAND;
	dev_info(&data->client->dev, "%s lux val=%u\n", __func__, *val);

	return ret;
als_get_lux_err:
	return ret;
}

static int JSA1136_set_als_range(struct JSA1136_data *data, unsigned int *val)
{
	int ret;
	int als_range_idx;
	int idx;

	als_range_idx = data->als_rng_idx;

	ret = regmap_update_bits(data->regmap, JSA1136_ALS_CONF_REG,
				 JSA1136_ALS_RANGE_MASK, (als_range_idx << ALS_SCALE_SHIFT));
	if (ret < 0)
		return ret;

	for (idx = 0; idx < ARRAY_SIZE(JSA1136_als_scale); idx++) {
		if (*val > JSA1136_als_range_val[idx])
			continue;
		else {
			als_range_idx = idx;
			break;
		}
	}

	data->als_rng_idx = als_range_idx;

	return 0;
}

static int JSA1136_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan, int *val, int *val2, long mask)
{
	int ret;
	struct JSA1136_data *data = iio_priv(indio_dev);

	dev_info(&data->client->dev, "%s mask=%ld\n", __func__, mask);

	switch (mask) {
	case IIO_CHAN_INFO_PROCESSED:
		mutex_lock(&data->lock);
		switch (chan->type) {
		case IIO_LIGHT:
			ret = JSA1136_get_lux(data, val);
			break;
		default:
			ret = -EINVAL;
			break;
		}
		mutex_unlock(&data->lock);
		return ret < 0 ? ret : IIO_VAL_INT;
	case IIO_CHAN_INFO_RAW:
		mutex_lock(&data->lock);
		switch (chan->type) {
		case IIO_LIGHT:
			ret = JSA1136_read_als_data(data, val);
			break;
		default:
			ret = -EINVAL;
			break;
		}
		mutex_unlock(&data->lock);
		return ret < 0 ? ret : IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_LIGHT:
			*val = JSA1136_als_range_val[data->als_rng_idx];
			return IIO_VAL_INT;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return -EINVAL;
}

static int JSA1136_write_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan, int val, int val2, long mask)
{
	int ret = 0;
	struct JSA1136_data *data = iio_priv(indio_dev);

	dev_info(&data->client->dev, "%s mask=%ld val=%d, val2=%d\n", __func__, mask, val, val2);

	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		mutex_lock(&data->lock);
		switch (chan->type) {
		case IIO_LIGHT:

			ret = JSA1136_set_als_range(data, &val);
			break;
		default:
			ret = -EINVAL;
		}
		mutex_unlock(&data->lock);
		return ret;
	default:
		break;
	}

	return -EINVAL;
}

#define LUX_CHANNEL(_index, _id) {              \
	.type = IIO_LIGHT,                    \
	.indexed = 1,                       \
	.channel = _index,                  \
	.info_mask_separate =  BIT(IIO_CHAN_INFO_SCALE) |       \
	BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_PROCESSED),  \
	.datasheet_name = _id,                  \
}

static const struct iio_chan_spec JSA1136_channels[] = {
	LUX_CHANNEL(0, "jsa1136"),
};

static const struct iio_info JSA1136_info = {
	.read_raw = &JSA1136_read_raw,
	.write_raw = &JSA1136_write_raw,
};

static int JSA1136_chip_init(struct JSA1136_data *data)
{
	int ret;

	ret = regmap_write(data->regmap, JSA1136_ALS_CONF_REG,
			   JSA1136_SYS_ALS_ENABLE | (JSA1136_ALS_RANGE_0_508 << ALS_SCALE_SHIFT));

	data->als_rng_idx = JSA1136_ALS_RANGE_0_508;

	if (ret < 0)
		return ret;

	return 0;
}

static const struct regmap_config JSA1136_regmap_config = {
	.name = JSA1136_REGMAP_NAME,
	.reg_bits = 8,
	.val_bits = 8,
};

static int JSA1136_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct JSA1136_data *data;
	struct iio_dev *indio_dev;
	struct regmap *regmap;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	regmap = devm_regmap_init_i2c(client, &JSA1136_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&client->dev, "Regmap initialization failed.\n");
		return PTR_ERR(regmap);
	}

	data = iio_priv(indio_dev);

	i2c_set_clientdata(client, indio_dev);
	data->client = client;
	data->regmap = regmap;

	mutex_init(&data->lock);

	ret = JSA1136_chip_init(data);
	if (ret < 0)
		return ret;

	indio_dev->dev.parent = &client->dev;
	indio_dev->dev.of_node = client->dev.of_node;
	indio_dev->channels = JSA1136_channels;
	indio_dev->num_channels = ARRAY_SIZE(JSA1136_channels);
	indio_dev->name = JSA1136_DRIVER_NAME;
	indio_dev->modes = INDIO_DIRECT_MODE;

	indio_dev->info = &JSA1136_info;
	ret = iio_device_register(indio_dev);
	if (ret < 0)
		dev_err(&client->dev, "%s: register device failed\n", __func__);

	return ret;
}

 /* power off the device */
static int JSA1136_power_off(struct JSA1136_data *data)
{
	int ret;

	mutex_lock(&data->lock);

	ret = regmap_update_bits(data->regmap, JSA1136_ALS_CONF_REG,
				 JSA1136_SYS_ALS_MASK, JSA1136_SYS_ALS_DISABLE);

	if (ret < 0)
		dev_err(&data->client->dev, "power off cmd failed\n");

	mutex_unlock(&data->lock);

	return ret;
}

static int JSA1136_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct JSA1136_data *data = iio_priv(indio_dev);

	dev_info(&data->client->dev, "%s\n", __func__);
	iio_device_unregister(indio_dev);

	return JSA1136_power_off(data);
}

#ifdef CONFIG_PM_SLEEP
static int JSA1136_suspend(struct device *dev)
{
	struct JSA1136_data *data;

	data = iio_priv(i2c_get_clientdata(to_i2c_client(dev)));

	return JSA1136_power_off(data);
}

static int JSA1136_resume(struct device *dev)
{
	int ret = 0;
	struct JSA1136_data *data;

	data = iio_priv(i2c_get_clientdata(to_i2c_client(dev)));

	mutex_lock(&data->lock);

	if (data->als_en) {
		ret = JSA1136_als_enable(data, JSA1136_SYS_ALS_ENABLE);
		if (ret < 0) {
			dev_err(dev, "als resume failed\n");
			goto unlock_and_ret;
		}
	}

unlock_and_ret:
	mutex_unlock(&data->lock);
	return ret;
}

static SIMPLE_DEV_PM_OPS(JSA1136_pm_ops, JSA1136_suspend, JSA1136_resume);

#define JSA1136_PM_OPS (&JSA1136_pm_ops)
#else
#define JSA1136_PM_OPS NULL
#endif

static const struct i2c_device_id JSA1136_id[] = {
	{JSA1136_DRIVER_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, JSA1136_id);

static const struct of_device_id JSA1136_als_dt_match[] = {
	{.compatible = "mediatek,jsa1136"},
	{},
};

MODULE_DEVICE_TABLE(of, JSA1136_als_dt_match);
static struct i2c_driver JSA1136_driver = {
	.driver = {
		   .name = JSA1136_DRIVER_NAME,
		   .pm = JSA1136_PM_OPS,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(JSA1136_als_dt_match),
		   },
	.probe = JSA1136_probe,
	.remove = JSA1136_remove,
	.id_table = JSA1136_id,
};

module_i2c_driver(JSA1136_driver);
MODULE_DESCRIPTION("JSA1136 ambient light sensor driver");
MODULE_LICENSE("Dual BSD/GPL");
