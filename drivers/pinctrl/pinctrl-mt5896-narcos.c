// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/mutex.h>

static struct pinctrl *pctl;
struct mutex select_mutex;

/**
 * mtk_pinctrl_select() - select pinctrl setting
 * @name: the name to select, the name is from dts
 *
 * Return 0 if success. Or negative errno on failure.
 */
int mtk_pinctrl_select(const char *name)
{
	struct pinctrl_state *select;
	int ret;

	mutex_lock(&select_mutex);

	if (IS_ERR(pctl)) {
		pr_info("mtk_pinctrl_select: pinctrl is not available\n");
		mutex_unlock(&select_mutex);
		return -ENODEV;
	}

	select = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(select)) {
		ret = PTR_ERR(select);
		pr_info("mtk_pinctrl_select: failed to lookup state (%s:%d)\n", name, ret);
		mutex_unlock(&select_mutex);
		return ret;
	}

	pinctrl_select_state(pctl, select);

	mutex_unlock(&select_mutex);

	return 0;
}
EXPORT_SYMBOL(mtk_pinctrl_select);

static const struct of_device_id mtk_pinctrl_narcos_match[] = {
	{ .compatible = "mediatek,mt5896-pinctrl-narcos" },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_pinctrl_narcos_match);

static int mtk_pinctrl_narcos_probe(struct platform_device *pdev)
{
	int ret;

	/* get pinctrl handle */
	pctl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		ret = PTR_ERR(pctl);
		dev_err(&pdev->dev, "Cannot get pinctrl: %d\n", ret);
		return ret;
	}

	mutex_init(&select_mutex);

	dev_info(&pdev->dev, "pinctrl narcos probe done\n");

	return 0;
}

static int mtk_pinctrl_narcos_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mtk_pinctrl_narcos_driver = {
	.probe	= mtk_pinctrl_narcos_probe,
	.remove	= mtk_pinctrl_narcos_remove,
	.driver = {
		.name = "mt5896-pinctrl-narcos",
		.of_match_table = mtk_pinctrl_narcos_match,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
};

module_platform_driver(mtk_pinctrl_narcos_driver);

MODULE_DESCRIPTION("MediaTek DTV SoC based Pinctrl Consumer");
MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");
MODULE_LICENSE("GPL");
