// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/dma-direct.h>
#include <common/mdla_device.h>
#include <common/mdla_cmd_proc.h>
#include <common/mdla_power_ctrl.h>
#include <common/mdla_ioctl.h>
#include <utilities/mdla_debug.h>
#include <utilities/mdla_profile.h>
#include <utilities/mdla_util.h>
#include <utilities/mdla_trace.h>
#include <interface/mdla_cmd_data_v2_0.h>
#include "comm_driver.h"

static int mdla_teec_data_setup(struct mdla_run_cmd_svp *cmd_svp,
				ST_TEEC_AI_CONTROL *ctrl,
				ST_MDLA_HAL_CONFIG *cfg)
{
	struct mdla_run_cmd_svp *p = cmd_svp;
	ST_TEEC_AI_RES *res;
	int i;

	ctrl->u8Version	= ST_TEEC_AI_CONTROL_VERSION;
	ctrl->u32PipelineID	= p->pipe_id;
	ctrl->enEng		= E_AI_ENG_MDLA;
	ctrl->u8ModelIndex	= p->model;
	ctrl->bIsSecurity	= p->secure;
	ctrl->u8BufCnt	= p->buf_count;

	if (p->buf_count > TEEC_AI_MAX_BUF_RES_NUM) {
		mdla_err("%s buf_count(%u) > TEEC_AI_MAX_BUF_RES_NUM(%u)\n",
				__func__, p->buf_count, TEEC_AI_MAX_BUF_RES_NUM);
		return -EFAULT;
	}

	for (i = 0; i < p->buf_count; i++) {
		res = &ctrl->stBufRes[i];

		res->enBufType	= p->buf[i].buf_type;
		res->enBufSource = p->buf[i].buf_source;
		res->u64BufAddr	= (u64)p->buf[i].iova | 0x200000000;
		res->u32BufSize	= p->buf[i].size;

		if (!ctrl->bIsSecurity &&
				p->buf[i].buf_type == EN_MDLA_BUF_TYPE_CODE) {
			cfg->cmd.mdla_hnd.mva	= res->u64BufAddr;
			cfg->cmd.mdla_hnd.size	= res->u32BufSize;
			cfg->cmd.mdla_hnd.count	= p->cmd_count;
			cfg->cmd_type	= 0;
		}
	}

	return 0;
}

int mdla_plat_teec_send_command(struct mdla_run_cmd_svp *cmd_svp)
{
	int ret = 0;
	u32 error = 0;
	struct TEEC_Operation op = {0};
	ST_TEEC_AI_CONTROL ctrl;
	ST_MDLA_HAL_CONFIG config;

	ret = mdla_teec_data_setup(cmd_svp, &ctrl, &config);
	if (ret) {
		mdla_err("[%s] data_setup fail\n", __func__);
		return -EFAULT;
	}

	op.params[0].tmpref.buffer = &ctrl;
	op.params[0].tmpref.size = sizeof(ctrl);

	if (ctrl.bIsSecurity) {
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
				TEEC_NONE, TEEC_NONE, TEEC_NONE);
	} else {
		op.params[1].tmpref.buffer = &config;
		op.params[1].tmpref.size = sizeof(config);

		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
				TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
	}

	ret = comm_util_get_cb()->send_teec_cmd(AI_TA_UUID_IDX, AI_CONTROL, &op, &error);
	if (ret) {
		mdla_err("[%s] SMC call failed, error(%u)\n", __func__, error);
		ret = -EPERM;
	}

	return ret;
}

