/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MDLA_MT5897_V2_0_UTIL_H__
#define __MDLA_MT5897_V2_0_UTIL_H__

#include <linux/types.h>
#include <linux/time.h>
#include <linux/types.h>

#include <utilities/mdla_debug.h>

int mdla_mt5897_v2_0_hw_reset(u32 reset);
int mdla_util_mt5897_init(void);
void mdla_util_mt5897_uninit(void);

#endif
