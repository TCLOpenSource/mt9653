// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * MTK Fake Ambient Light Sensor Driver
 *
 * Copyright (c) 2021 MediaTek Inc.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/iio/sysfs.h>
#include <linux/util_macros.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>

#define MTK_ALS_RANGE_0_2034    0x00
#define MTK_ALS_RANGE_0_1017    0x01
#define MTK_ALS_RANGE_0_508	0x02
#define MTK_ALS_RANGE_0_254	0x03
#define MTK_ALS_RANGE_0_127	0x04
#define MTK_ALS_DRIVER_NAME	"mtk-als"

#define THOUSAND 1000

struct mtk_als_data {
	u8 als_rng_idx;
	int als_data;
};

/* LUX = ALS data * scale / 1000 */
/* ALS data = LUX * 1000 / scale */

/* ALS data range mapping */
static const int mtk_als_range_val[] = { 127, 254, 511, 1023, 2047 };

/* ALS range idx to luminace mapping */
static const int mtk_als_scale[] = { 31, 63, 125, 250, 500 };


static int mtk_als_read_als_data(struct iio_dev *indio_dev, unsigned int *val)
{
	struct mtk_als_data *data = iio_priv(indio_dev);

	*val = data->als_data;

	return 0;
}

static int mtk_als_set_als_data(struct iio_dev *indio_dev, unsigned int val)
{
	struct mtk_als_data *data = iio_priv(indio_dev);

	data->als_data = val;

	return 0;
}

static int mtk_als_get_lux(struct iio_dev *indio_dev, unsigned int *lux)
{
	struct mtk_als_data *data = iio_priv(indio_dev);

	*lux = ((data->als_data) * mtk_als_scale[data->als_rng_idx]) / THOUSAND;

	return 0;
}

static int mtk_als_set_lux(struct iio_dev *indio_dev, unsigned int lux)
{
	struct mtk_als_data *data = iio_priv(indio_dev);

	data->als_data = lux * THOUSAND / mtk_als_scale[data->als_rng_idx];

	return 0;
}

static int mtk_als_set_als_range(struct iio_dev *indio_dev, unsigned int val)
{
	int als_range_idx;
	int idx;
	struct mtk_als_data *data = iio_priv(indio_dev);

	als_range_idx = data->als_rng_idx;

	for (idx = 0; idx < ARRAY_SIZE(mtk_als_scale); idx++) {
		if (val > mtk_als_range_val[idx])
			continue;
		else {
			als_range_idx = idx;
			break;
		}
	}

	data->als_rng_idx = als_range_idx;

	return 0;
}

static int mtk_als_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan, int *val, int *val2, long mask)
{
	int ret;
	struct mtk_als_data *data = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_PROCESSED:
		switch (chan->type) {
		case IIO_LIGHT:
			ret = mtk_als_get_lux(indio_dev, val);
			if (ret < 0)
				return ret;
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_RAW:
		switch (chan->type) {
		case IIO_LIGHT:
			ret = mtk_als_read_als_data(indio_dev, val);
			if (ret < 0)
				return ret;
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_LIGHT:
			*val = mtk_als_range_val[data->als_rng_idx];
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static int mtk_als_write_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan, int val, int val2, long mask)
{
	int ret = 0;

	switch (mask) {
	case IIO_CHAN_INFO_PROCESSED:
		switch (chan->type) {
		case IIO_LIGHT:
			ret = mtk_als_set_lux(indio_dev, val);
			if (ret < 0)
				return ret;
			return 0;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_RAW:
		switch (chan->type) {
		case IIO_LIGHT:
			ret = mtk_als_set_als_data(indio_dev, val);
			if (ret < 0)
				return ret;
			return 0;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_LIGHT:
			ret = mtk_als_set_als_range(indio_dev, val);
			if (ret < 0)
				return ret;
			return 0;
		default:
			return -EINVAL;
		}
	default:
		return -EINVAL;
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

static const struct iio_chan_spec mtk_als_channels[] = {
	LUX_CHANNEL(0, "als0"),
};

static const struct iio_info mtk_als_info = {
	.read_raw = &mtk_als_read_raw,
	.write_raw = &mtk_als_write_raw,
};

static int mtk_als_probe(struct platform_device *pdev)
{
	int ret;
	struct mtk_als_data *data;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	data->als_rng_idx = MTK_ALS_RANGE_0_508;

	indio_dev->dev.parent = &pdev->dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->channels = mtk_als_channels;
	indio_dev->num_channels = ARRAY_SIZE(mtk_als_channels);
	indio_dev->name = MTK_ALS_DRIVER_NAME;
	indio_dev->modes = INDIO_DIRECT_MODE;

	indio_dev->info = &mtk_als_info;

	ret = iio_device_register(indio_dev);
	if (ret < 0)
		return ret;

	platform_set_drvdata(pdev, indio_dev);

	return 0;
}

static int mtk_als_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;

	indio_dev = platform_get_drvdata(pdev);
	iio_device_unregister(indio_dev);
	return 0;
}

static const struct of_device_id mtk_als_match[] = {
	{
		.compatible = "mediatek,als",
	},
	{},
};

static struct platform_driver mtk_als_driver = {
	.probe	= mtk_als_probe,
	.remove = mtk_als_remove,
	.driver = {
		.name	= "mtk_als",
		.of_match_table = mtk_als_match,
	},
};

module_platform_driver(mtk_als_driver);

MODULE_DESCRIPTION("MTK stub ambient light sensor driver");
MODULE_LICENSE("Dual BSD/GPL");
