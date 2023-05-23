// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/thermal.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/of_device.h>
#include <linux/string.h>
#include "mtk_pq_thermal.h"
#include "mtk_pq_common.h"

//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------
#define CDEV_NAME_LEN	(64)
#define MAX_STATE	(2UL)

struct mtk_pq_cdev {
	unsigned long cur_state;
};

struct mtk_pq_thermal _thermal_info;

static int mtk_pq_cdev_get_max(struct thermal_cooling_device *cdev,
	unsigned long *max_state)
{
	struct mtk_pq_cdev *mtk_dev;

	if ((!cdev) || (!max_state)) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	mtk_dev = cdev->devdata;
	*max_state = MAX_STATE;

	return 0;
}

static int mtk_pq_cdev_get_cur(struct thermal_cooling_device *cdev,
	unsigned long *state)
{
	struct mtk_pq_cdev *mtk_dev;

	if ((!cdev) || (!state)) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	mtk_dev = cdev->devdata;
	*state = mtk_dev->cur_state;

	return 0;
}

static int mtk_pq_cdev_set_cur(struct thermal_cooling_device *cdev,
	unsigned long state)
{
	struct mtk_pq_cdev *mtk_dev;

	if (!cdev) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	mtk_dev = cdev->devdata;
	mtk_dev->cur_state = state;

	if (mtk_dev->cur_state > 0)
		_thermal_info.thermal_disable_aisr = 1;
	else
		_thermal_info.thermal_disable_aisr = 0;

	return 0;
}

static const struct thermal_cooling_device_ops mtk_pq_thermal_ops = {
	.get_max_state = mtk_pq_cdev_get_max,
	.get_cur_state = mtk_pq_cdev_get_cur,
	.set_cur_state = mtk_pq_cdev_set_cur,
};

int mtk_pq_cdev_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct device *dev;
	struct thermal_cooling_device *cdev;
	int idx = 0;
	char name[CDEV_NAME_LEN];
	struct mtk_pq_cdev *mtk_dev;

	if (!pdev) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	np = pdev->dev.of_node;
	dev = &pdev->dev;

	if (!np) {
		dev_err(dev, "DT node not found\n");
		return -ENODEV;
	}

	mtk_dev = devm_kzalloc(dev, sizeof(*mtk_dev), GFP_KERNEL);
	snprintf(name, sizeof(name), "%s-%d", "mtk_pq_cdev", idx++);
	cdev = thermal_of_cooling_device_register(np, name, mtk_dev, &mtk_pq_thermal_ops);
	if (IS_ERR(cdev)) {
		dev_err(dev, "register cooling device failed\n");
		return -EINVAL;
	}
	platform_set_drvdata(pdev, cdev);

	return 0;
}

int mtk_pq_cdev_remove(struct platform_device *pdev)
{
	struct thermal_cooling_device *cdev;

	if (!pdev) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	cdev = platform_get_drvdata(pdev);
	if (cdev)
		thermal_cooling_device_unregister(cdev);

	return 0;
}

int mtk_pq_cdev_get_thermal_info(struct mtk_pq_thermal *thermal_info)
{
	if (!thermal_info) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	thermal_info->thermal_disable_aisr = _thermal_info.thermal_disable_aisr;

	return 0;
}

