/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __APW_PLAT_DEVICE_H__
#define __APW_PLAT_DEVICE_H__
#include <linux/kernel.h>
#include <linux/module.h>
#include "apu_common.h"

int dtv_apupwr_get_pwrctrl(void);
int dtv_apupwr_set_pwrctrl(int power_ctrl, int delay);
struct apu_plat_ops *dtv_apupwr_get_ops(struct apu_dev *ad, const char *name);

#endif
