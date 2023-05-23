/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MDLA_TEEC_API_H__
#define __MDLA_TEEC_API_H__

#include <linux/types.h>
#include <linux/time.h>
#include <linux/types.h>
#include <interface/mdla_cmd_data_v2_0.h>

int mdla_plat_teec_send_command(struct mdla_run_cmd_svp *cmd_svp);

#endif
