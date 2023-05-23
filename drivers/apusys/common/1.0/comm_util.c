// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/dma-direct.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include "comm_driver.h"
#include "comm_teec.h"
#include "comm_util.h"
#include "comm_debug.h"

static struct platform_device *plat_dev;
#define APMCU_RIUBASE 0x1C000000
#define RIU_ADDR(bank, offset)		(APMCU_RIUBASE + ((bank<<8)<<1) + (offset<<2))
#define REG_OFFSET_SHIFT_BITS		2

#define BANK_EFUSE 0x129
#define REG_MDLA_EFUSE_OUTB 0x51
#define REG_MDLA_EFUSE_OUTB_OFFSET 0x6

static u16 riu_read16(u32 bank, u32 offset)
{
	void *addr;
	u16 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread16(addr);
	iounmap(addr);

	return regv;
}

static const struct plat_data plat_list[] = {
	{ .id = COMM_PLAT_MT5896, .name = "mtk,mt5896-mdla"},
	{ .id = COMM_PLAT_MT5897, .name = "mtk,mt5897-mdla"},
	{ .id = COMM_PLAT_MT5879, .name = "mtk,mt5879-mdla"},
	{ .id = COMM_PLAT_MT5896_E3, .name = "mtk,mt5896_e3-mdla"},
	{ .id = COMM_PLAT_NON_SUPPORT, .name = NULL},
};

struct device_node *comm_get_device_node(const struct plat_data **plat)
{
	struct device_node *np = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(plat_list) - 1; i++) {
		np = of_find_compatible_node(NULL, NULL,
				plat_list[i].name);
		if (np)
			break;
	}

	if (plat)
		*plat = &plat_list[i];

	return np;
}

#define MTK_APU_EFUSE_IDX	0x443
static int (*efuse_cb)(unsigned long id, u32 *support);
int apusys_efusecb_register(int (*cb)(unsigned long, u32 *))
{
	if (cb)
		efuse_cb = cb;

	return 0;
}
EXPORT_SYMBOL(apusys_efusecb_register);

// return value: 1 -> supported, 0 -> not supported
int comm_hw_supported(void)
{
	u16 regv;
	u32 support = 0;
	const struct plat_data *plat;

	comm_get_device_node(&plat);
	switch (plat->id) {
	case COMM_PLAT_MT5896:
	case COMM_PLAT_MT5897:
	case COMM_PLAT_MT5896_E3:
	case COMM_PLAT_MT5879:
		if (efuse_cb) {
			if (efuse_cb(MTK_APU_EFUSE_IDX, &support))
				pr_info("%s(): efuse_cb fail, default to disable\n", __func__);
		} else {
			//bank2e[22]:MDLA, 0x50:outb[15:0], 0x51:outb[31:16]
			regv = riu_read16(BANK_EFUSE, REG_MDLA_EFUSE_OUTB);
			support = !((regv >> REG_MDLA_EFUSE_OUTB_OFFSET) & 0x1);
		}
		break;
	default:
		support = 0;
		break;
	}
	return support;
}

static struct comm_kmem mem;
int comm_tee_set_loglvl(u32 loglvl)
{
	u32 error = 0;
	struct TEEC_Operation op = {0};
	ST_TEEC_AI_CONTROL ctrl;
	ST_MDLA_HAL_CONFIG cfg;

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

	cfg.cmd_type = E_HAL_CFG_STATUS;
	cfg.cmd.dev_cfg.cfg_mask = CONFIG_LOGLVL;
	cfg.cmd.dev_cfg.log_level = loglvl;

	op.params[0].tmpref.buffer = &ctrl;
	op.params[0].tmpref.size = sizeof(ctrl);
	op.params[1].tmpref.buffer = &cfg;
	op.params[1].tmpref.size = sizeof(cfg);
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
			TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);

	return comm_util_get_cb()->send_teec_cmd(AI_TA_UUID_IDX, AI_CONTROL, &op, &error);
}

int comm_util_init(struct platform_device *pdev)
{
	plat_dev = pdev;
	memset(&mem, 0, sizeof(mem));

	return 0;
}
