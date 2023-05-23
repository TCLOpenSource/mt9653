/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __APW_PLAT_REGULATOR_H__
#define __APW_PLAT_REGULATOR_H__
#include <linux/kernel.h>
#include <linux/module.h>
#include "apu_common.h"

struct apu_regulator_gp *ext_regulator_gp_get(struct apu_dev *ad);
int dtv_apu_regulator_init(struct apu_dev *ad);
void dtv_apu_regulator_uninit(struct apu_dev *ad);
#endif
