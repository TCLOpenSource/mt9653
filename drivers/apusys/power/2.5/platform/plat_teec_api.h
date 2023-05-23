/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MDLA_TEEC_API_H__
#define __MDLA_TEEC_API_H__

#include <linux/types.h>
#include <linux/time.h>
#include <linux/types.h>
#include "comm_driver.h"

int plat_teec_send_command(struct apu_dev *ad, u32 sub_cmd, u32 num, u32 *data);

#endif
