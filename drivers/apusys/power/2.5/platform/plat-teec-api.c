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
#include "apusys_power_user.h"
#include "apu_devfreq.h"
#include "apu_log.h"
#include "apu_common.h"
#include "plat_teec_api.h"
#include "comm_driver.h"

static struct comm_kmem mem;
static int init_once;

int plat_teec_send_command(struct apu_dev *ad, u32 sub_cmd, u32 num, u32 *data)
{
	u32 error = 0;
	struct TEEC_Operation op = {0};
	ST_TEEC_AI_CONTROL ctrl;
	ST_MDLA_HAL_CONFIG cfg;
	u32 i;

	if (!init_once) {
		memset(&mem, 0, sizeof(mem));
		init_once = 1;
	}

	if (!mem.iova) {
		mem.size = PAGE_SIZE;
		mem.pipe_id = 0xFFFFFFFF;
		mem.mem_type = APU_COMM_MEM_DRAM_DMA;
		if (comm_util_get_cb()->alloc(&mem))
			return -ENOMEM;
	}

	// tee_command_header
	ctrl.u8Version	= ST_TEEC_AI_CONTROL_VERSION;
	ctrl.u32PipelineID	= INT_MAX;
	ctrl.enEng		= E_AI_ENG_MDLA;
	ctrl.u8ModelIndex	= 0;
	ctrl.bIsSecurity	= 0;
	ctrl.u8BufCnt	= 1;
	ctrl.stBufRes[0].enBufType = EN_AI_BUF_TYPE_CODE;
	ctrl.stBufRes[0].enBufSource = EN_BUF_SOURCE_DRAM;
	ctrl.stBufRes[0].u64BufAddr	= (u64)mem.iova;
	ctrl.stBufRes[0].u32BufSize	= 0x8;

	cfg.cmd_type = E_HAL_CFG_POWER;
	cfg.cmd.pwr_cfg.user = 0;
	cfg.cmd.pwr_cfg.cmd = sub_cmd;
	cfg.cmd.pwr_cfg.num = num;
	for (i = 0; i < num; i++)
		cfg.cmd.pwr_cfg.data[i] = data[i];

	op.params[0].tmpref.buffer = &ctrl;
	op.params[0].tmpref.size = sizeof(ctrl);
	op.params[1].tmpref.buffer = &cfg;
	op.params[1].tmpref.size = sizeof(cfg);
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
			TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);

	return comm_util_get_cb()->send_teec_cmd(AI_TA_UUID_IDX, AI_CONTROL, &op, &error);
}

