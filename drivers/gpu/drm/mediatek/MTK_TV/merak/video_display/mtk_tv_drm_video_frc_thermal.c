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
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/dma-buf.h>
#include <linux/thermal.h>

#include <drm/mtk_tv_drm.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>

#include "../mtk_tv_drm_plane.h"
#include "../mtk_tv_drm_gem.h"
#include "../mtk_tv_drm_kms.h"
#include "mtk_tv_drm_common.h"
#include "mtk_tv_drm_log.h"
#include "drv_scriptmgt.h"
#include "mtk_tv_drm_video_frc.h"
#include "mtk_tv_drm_video_frc_thermal.h"
#include "m_pqu_pq.h"

#define CDEV_NAME_LEN	(64)
#define MAX_STATE	(1UL)
struct mtk_frc_thermal _thermal_info;

#define FRC_CHECK_NULLPTR() \
	do { \
		DRM_ERROR("[%s,%5d] NULL POINTER\n", __func__, __LINE__); \
		dump_stack(); \
	} while (0)

//-----------------------------------------------------------------------------
static int mtk_frc_cdev_get_max(struct thermal_cooling_device *cdev,
	unsigned long *max_state)
{
	struct mtk_frc_cdev *mtk_dev;

	if ((!cdev) || (!max_state)) {
		FRC_CHECK_NULLPTR();
		return -EPERM;
	}

	mtk_dev = cdev->devdata;
	*max_state = MAX_STATE;

	return 0;
}

static int mtk_frc_cdev_get_cur(struct thermal_cooling_device *cdev,
	unsigned long *state)
{
	struct mtk_frc_cdev *mtk_dev;
	int ret = 0;
	uint8_t QosMode;

	if ((!cdev) || (!state)) {
		FRC_CHECK_NULLPTR();
		return -EPERM;
	}

	mtk_dev = cdev->devdata;
	ret = mtk_video_display_frc_get_Qos(&QosMode);
	*state = QosMode;//mtk_dev->cur_state;

	return ret;
}

static int mtk_frc_cdev_set_cur(struct thermal_cooling_device *cdev,
	unsigned long state)
{
	struct mtk_frc_cdev *mtk_dev;
	int ret = 0;

	if (!cdev) {
		FRC_CHECK_NULLPTR();
		return -EPERM;
	}

	mtk_dev = cdev->devdata;
	mtk_dev->cur_state = state;

	if (mtk_dev->cur_state > 0)
		ret = mtk_video_display_frc_set_Qos(true);//_thermal_info.thermal_disable_memc = 1;
	else
		ret = mtk_video_display_frc_set_Qos(false);//_thermal_info.thermal_disable_memc = 0;

	return ret;
}

static const struct thermal_cooling_device_ops mtk_frc_thermal_ops = {
	.get_max_state = mtk_frc_cdev_get_max,
	.get_cur_state = mtk_frc_cdev_get_cur,
	.set_cur_state = mtk_frc_cdev_set_cur,
};

int mtk_frc_cdev_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct device *dev;
	struct thermal_cooling_device *cdev;
	int idx = 0;
	char name[CDEV_NAME_LEN];
	struct mtk_frc_cdev *mtk_dev;
	int ret;


	if (!pdev) {
		FRC_CHECK_NULLPTR();
		return -EPERM;
	}

	np = pdev->dev.of_node;
	dev = &pdev->dev;

	if (!np) {
		dev_err(dev, "DT node not found\n");
		return -ENODEV;
	}

	mtk_dev = devm_kzalloc(dev, sizeof(*mtk_dev), GFP_KERNEL);
	ret = snprintf(name, sizeof(name), "%s-%d", "mtk_frc_cdev", idx++);
	if ((ret < 0) || (ret >= CDEV_NAME_LEN)) {
		dev_err(dev, "[%s,%d] snprintf failed\n", __func__, __LINE__);
		return -EINVAL;

	}
	cdev = thermal_of_cooling_device_register(np, name, mtk_dev, &mtk_frc_thermal_ops);
	if (IS_ERR(cdev)) {
		dev_err(dev, "register cooling device failed\n");
		return -EINVAL;
	}
	platform_set_drvdata(pdev, cdev);

	return 0;
}

int mtk_frc_cdev_remove(struct platform_device *pdev)
{
	struct thermal_cooling_device *cdev;

	if (!pdev) {
		FRC_CHECK_NULLPTR();
		return -EPERM;
	}

	cdev = platform_get_drvdata(pdev);
	if (cdev)
		thermal_cooling_device_unregister(cdev);

	return 0;
}

int mtk_frc_cdev_get_thermal_info(struct mtk_frc_thermal *thermal_info)
{
	if (!thermal_info) {
		FRC_CHECK_NULLPTR();
		return -EPERM;
	}

	thermal_info->thermal_disable_memc = _thermal_info.thermal_disable_memc;

	return 0;
}
