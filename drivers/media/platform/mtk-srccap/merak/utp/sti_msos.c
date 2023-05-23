// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>

#include "sti_msos.h"

void MsOS_DelayTask(MS_U32 u32Ms)
{
	msleep((u32Ms));
}

MS_U32 MsOS_GetSystemTime(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}
