// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/thermal.h>
#include <linux/types.h>

struct mtk_test_cdev {
	unsigned long cur_state;
};

static int mtk_test_cdev_get_max(struct thermal_cooling_device *cdev,
				    unsigned long *max_state)
{
	struct mtk_test_cdev *mtk_dev = cdev->devdata;
	struct device *dev = &cdev->device;

	dev_info(dev, "get max, return 12 directly\n");
	*max_state = 12UL;
	return 0;
}

static int mtk_test_cdev_get_cur(struct thermal_cooling_device *cdev,
				    unsigned long *value)
{
	struct mtk_test_cdev *mtk_dev = cdev->devdata;
	struct device *dev = &cdev->device;

	dev_info(dev, "get state\n");
	*value = mtk_dev->cur_state;
	return 0;
}

static int mtk_test_cdev_set_cur(struct thermal_cooling_device *cdev,
				    unsigned long state)
{
	struct mtk_test_cdev *mtk_dev = cdev->devdata;
	struct device *dev = &cdev->device;

	mtk_dev->cur_state = state;
	dev_info(dev, "set to state %lu\n", state);
	return 0;
}

static const struct thermal_cooling_device_ops mtk_test_cdev_ops = {
	.get_max_state = mtk_test_cdev_get_max,
	.get_cur_state = mtk_test_cdev_get_cur,
	.set_cur_state = mtk_test_cdev_set_cur,
};

#define CDEV_NAME_LEN 64
static int mtk_test_cdev_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct thermal_cooling_device *cdev;
	static int idx;
	char name[CDEV_NAME_LEN];
	struct mtk_test_cdev *mtk_dev;

	if (!np) {
		dev_err(dev, "DT node not found\n");
		return -ENODEV;
	}

	mtk_dev = devm_kzalloc(dev, sizeof(*mtk_dev), GFP_KERNEL);
	if (snprintf(name, sizeof(name), "%s-%d", "mtk_test_cdev", idx++) < 0)
		return -EINVAL;

	cdev = thermal_of_cooling_device_register(np, name, mtk_dev, &mtk_test_cdev_ops);
	if (IS_ERR(cdev)) {
		dev_err(dev, "register cooling device failed\n");
		return -EINVAL;
	}
	platform_set_drvdata(pdev, cdev);

	return 0;
}

static int mtk_test_cdev_remove(struct platform_device *pdev)
{
	struct thermal_cooling_device *cdev = platform_get_drvdata(pdev);

	if (cdev)
		thermal_cooling_device_unregister(cdev);

	return 0;
}

static const struct of_device_id mtk_test_cdev_of_match[] = {
	{ .compatible = "mtk-test-cdev", },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_test_cdev_of_match);

static struct platform_driver mtk_test_cdev_driver = {
	.probe = mtk_test_cdev_probe,
	.remove = mtk_test_cdev_remove,
	.driver = {
		.name = "mtk_test_cdev",
		.of_match_table = mtk_test_cdev_of_match,
	},
};
module_platform_driver(mtk_test_cdev_driver);

MODULE_AUTHOR("Mark-PK Tsai <mark-pk.tsai@mediatek.com>");
MODULE_DESCRIPTION("Mediatek test cooling common driver");
MODULE_LICENSE("GPL v2");
