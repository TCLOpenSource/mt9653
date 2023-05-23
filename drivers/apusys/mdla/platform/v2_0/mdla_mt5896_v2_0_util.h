/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MDLA_MT5896_V2_0_UTIL_H__
#define __MDLA_MT5896_V2_0_UTIL_H__

#include <linux/types.h>
#include <linux/time.h>
#include <linux/types.h>

#include <utilities/mdla_debug.h>

int mdla_mt5896_v2_0_hw_reset(u32 force);
void aia_top_security_rst(int rst_policy);
void apmcu_miu2tcm_access_permission(int enable);

#endif
