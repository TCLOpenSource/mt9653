/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MDLA_PROFILE_H__
#define __MDLA_PROFILE_H__

#include <linux/types.h>

enum PROF_MODE {
	PROF_V1,
	PROF_V2,
};

void mdla_prof_start(u32 core_id);
void mdla_prof_stop(u32 core_id, int wait);
void mdla_prof_iter(u32 core_id);

bool mdla_prof_pmu_timer_is_running(u32 core_id);
bool mdla_prof_use_dbgfs_pmu_event(u32 core_id);

void mdla_prof_init(int mode);
void mdla_prof_deinit(void);

#endif /* __MDLA_PROFILE_H__ */

