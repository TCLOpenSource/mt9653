/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_THERMAL_H
#define _MTK_PQ_THERMAL_H

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

struct mtk_pq_thermal {
	bool thermal_disable_aisr;
};

int mtk_pq_cdev_probe(struct platform_device *pdev);
int mtk_pq_cdev_remove(struct platform_device *pdev);
int mtk_pq_cdev_get_thermal_info(struct mtk_pq_thermal *thermal_info);

#endif
