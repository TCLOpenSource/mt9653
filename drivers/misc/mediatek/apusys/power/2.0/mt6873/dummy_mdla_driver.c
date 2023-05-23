// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/delay.h>

#include "apusys_power.h"
#include "apu_log.h"


static int mdla_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int mdla_resume(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
int mdla_pm_suspend(struct device *device)
{
	return 0;
}

int mdla_pm_resume(struct device *device)
{
	return 0;
}

int mdla_pm_restore_noirq(struct device *device)
{
	return 0;
}
#else
#define mdla_pm_suspend NULL
#define mdla_pm_resume  NULL
#define mdla_pm_restore_noirq NULL
#endif


static int mdla_probe(struct platform_device *pdev)
{
	int ret = 0;
	enum DVFS_USER register_user;

	LOG_INF("probe 0, pdev id = %d name = %s, name = %s\n",
						pdev->id, pdev->name,
						pdev->dev.of_node->name);
	if (strcmp(pdev->dev.of_node->name, "mtk_mdla0") == 0)
		register_user = MDLA0;
	else
		return -1;

	ret = apu_power_device_register(register_user, pdev);
	if (!ret) {
		LOG_INF("%s register power device %d success\n",
						__func__, register_user);
	} else {
		LOG_ERR("%s register power device %d fail\n",
						__func__, register_user);
		return -1;
	}

	return 0;
}

static int mdla_remove(struct platform_device *pdev)
{
	int ret = 0;
	enum DVFS_USER register_user;

	LOG_INF("remove 0, pdev id = %d name = %s, name = %s\n",
						pdev->id, pdev->name,
						pdev->dev.of_node->name);

	if (strcmp(pdev->dev.of_node->name, "mtk_mdla0") == 0)
		register_user = MDLA0;
	else
		return -1;

	apu_power_device_unregister(register_user);
	LOG_INF("%s unregister power device %d success\n",
						__func__, register_user);
	return 0;
}

static const struct dev_pm_ops mdla_pm_ops = {
	.suspend = mdla_pm_suspend,
	.resume = mdla_pm_resume,
	.freeze = mdla_pm_suspend,
	.thaw = mdla_pm_resume,
	.poweroff = mdla_pm_suspend,
	.restore = mdla_pm_resume,
	.restore_noirq = mdla_pm_restore_noirq,
};

static const struct of_device_id mdla_of_ids[] = {
	{.compatible = "mtk,mdla0",},
	{}
};

static struct platform_driver mdla_driver = {
	.probe   = mdla_probe,
	.remove  = mdla_remove,
	.suspend = mdla_suspend,
	.resume  = mdla_resume,
	.driver  = {
		.name = "mdla",
		.owner = THIS_MODULE,
		.of_match_table = mdla_of_ids,
#ifdef CONFIG_PM
		.pm = &mdla_pm_ops,
#endif
	}
};

int mdla_init(struct apusys_core_info *info)
{
	return platform_driver_register(&mdla_driver);
}

void mdla_exit(void)
{
	platform_driver_unregister(&mdla_driver);
}

