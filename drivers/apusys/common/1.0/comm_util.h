/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __APUSYS_COMM_UTIL_H__
#define __APUSYS_COMM_UTIL_H__

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/printk.h>

enum COMM_SUPPORT_PLAT {
	COMM_PLAT_MT5896,
	COMM_PLAT_MT5897,
	COMM_PLAT_MT5896_E3,
	COMM_PLAT_MT5879,
	COMM_PLAT_NON_SUPPORT = 0xFFFFFFFF
};

struct plat_data {
	u32 id;
	char *name;
	void *data;
};

u32 comm_plat_query(const char **plat_name);
int comm_hw_supported(void);
int comm_tee_set_loglvl(u32 loglvl);
int comm_util_init(struct platform_device *pdev);
struct device_node *comm_get_device_node(const struct plat_data **plat);

#endif

