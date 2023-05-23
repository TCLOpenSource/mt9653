/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _APUSYS_POWER_H_
#define _APUSYS_POWER_H_

enum DVFS_USER {
	VPU0 = 0,
	VPU1 = 1,
	VPU2 = 2,
	MDLA0 = 3,
	MDLA1 = 4,
	APUSYS_DVFS_USER_NUM,

	EDMA = 0x10,	// power user only
	EDMA2 = 0x11,   // power user only
	REVISER = 0x12, // power user only
	APUSYS_POWER_USER_NUM,
};

#define apu_power_device_register(user, pdev) 0
#define apu_power_device_unregister(user)
//#define apu_device_power_on(user) 0
//#define apu_device_power_off(user) 0
extern int apu_device_power_on(enum DVFS_USER);
extern int apu_device_power_off(enum DVFS_USER);
#define apu_device_power_suspend(user, suspend) 0
#define apu_device_set_opp(user, opp)
#define apusys_boost_value_to_opp(user, val) 0
#define apusys_power_check(user) true

#ifndef APUSYS_MAX_NUM_OPPS
#define APUSYS_MAX_NUM_OPPS 1
#endif

#endif
