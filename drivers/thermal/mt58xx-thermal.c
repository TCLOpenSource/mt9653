// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/iio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>

#define MTK_THERMAL_CONF_NUM	3

#define MTK_THERMAL_VREF	0
#define MTK_THERMAL_TREF	1
#define MTK_THERMAL_VSLOPE	2

struct mtk_thermal_data {
	struct device *dev;
	struct iio_channel *iio;
	unsigned long vref, tref, vslope;
};

static int get_temp(void *data, int *temp)
{
	struct mtk_thermal_data *thermal = (struct mtk_thermal_data *)data;
	int val, ret;

	ret = iio_read_channel_processed(thermal->iio, &val);
	if (ret < 0)
		return ret;

	val = ((thermal->vref - val) * thermal->vslope) + thermal->tref;
	*temp = val;

	return 0;
}

static struct thermal_zone_of_device_ops mtktv_thermal_of_ops = {
	.get_temp = get_temp,
};

static int mt58xx_thermal_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct thermal_zone_device *tzdev;
	struct mtk_thermal_data *thermal;
	u32 conf[MTK_THERMAL_CONF_NUM];
	int ret;

	if (!pdev->dev.of_node)
		return -ENODEV;

	thermal = devm_kzalloc(dev, sizeof(*thermal), GFP_KERNEL);
	if (!thermal)
		return -ENOMEM;

	thermal->dev = dev;
	thermal->iio = devm_iio_channel_get(dev, "thermal");
	if (IS_ERR(thermal->iio)) {
		dev_err(dev, "Get iio channel fail (%ld)\n", PTR_ERR(thermal->iio));
		return PTR_ERR(thermal->iio);
	}

	ret = of_property_read_u32_array(dev->of_node,
					 "mediatek,thermal-config",
					 conf, ARRAY_SIZE(conf));
	if (ret) {
		dev_err(dev, "thermal config not found\n");
		return ret;
	}

	thermal->vref = conf[MTK_THERMAL_VREF];
	thermal->tref = conf[MTK_THERMAL_TREF];
	thermal->vslope = conf[MTK_THERMAL_VSLOPE];

	platform_set_drvdata(pdev, thermal);
	tzdev = devm_thermal_zone_of_sensor_register(&pdev->dev
		, 0, thermal, &mtktv_thermal_of_ops);
	if (IS_ERR(tzdev)) {
		dev_err(dev, "Error registering sensor (%ld)\n", PTR_ERR(tzdev));
		return PTR_ERR(tzdev);
	}

	return 0;
}

static const struct of_device_id mt58xx_thermal_of_match[] = {
	{ .compatible = "mt58xx-thermal" },
	{},
};
MODULE_DEVICE_TABLE(of, mt58xx_thermal_of_match);

static struct platform_driver mtktv_thermal_driver = {
	.driver = {
		.name       = "mt58xx-thermal",
		.owner      = THIS_MODULE,
		.of_match_table = mt58xx_thermal_of_match,
	},
	.probe  = mt58xx_thermal_probe,
};

module_platform_driver(mtktv_thermal_driver);

MODULE_AUTHOR("Mark-PK Tsai <mark-pk.tsai@mediatek.com>");
MODULE_DESCRIPTION("MediaTek MT58XX Thermal driver");
MODULE_LICENSE("GPL v2");
