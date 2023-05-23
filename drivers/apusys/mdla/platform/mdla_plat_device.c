// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of_device.h>
#include <utilities/mdla_profile.h>
#include <utilities/mdla_debug.h>
#include "mdla_plat_internal.h"

static struct mdla_plat_drv mt589x_drv = {
	.init           = mdla_mt589x_v2_0_init,
	.deinit         = mdla_mt589x_v2_0_deinit,
	.sw_cfg         = BIT(CFG_DUMMY_MMU)
				| BIT(CFG_DTV_SECURE_DRV_RDY)
				| BIT(CFG_MICRO_P_SUPPORT),
	.klog           = (MDLA_DBG_TIMEOUT),
	.timeout_ms     = 6000,
	.off_delay_ms   = 2000,
	.polling_cmd_ms = 5,
	.pmu_period_us  = 1000,
	.mreg_cmd_size	= 0x1C0,
	.profile_ver    = PROF_V2,
};

static struct mdla_plat_drv mt5896_drv = {
	.init           = mdla_mt5896_v2_0_init,
	.deinit         = mdla_mt5896_v2_0_deinit,
	.sw_cfg         = BIT(CFG_DUMMY_MMU)
				| BIT(CFG_MICRO_P_SUPPORT),
	.klog           = (MDLA_DBG_CMD | MDLA_DBG_TIMEOUT),
	.timeout_ms     = 6000,
	.off_delay_ms   = 2000,
	.polling_cmd_ms = 5,
	.pmu_period_us  = 1000,
	.mreg_cmd_size	= 0x1C0,
	.profile_ver    = PROF_V2,
};

static struct mdla_plat_drv mt5897_drv = {
	.init           = mdla_mt5897_v2_0_init,
	.deinit         = mdla_mt5897_v2_0_deinit,
	.sw_cfg         = BIT(CFG_DUMMY_MMU)
				| BIT(CFG_MICRO_P_SUPPORT),
	.klog           = (MDLA_DBG_CMD | MDLA_DBG_TIMEOUT),
	.timeout_ms     = 6000,
	.off_delay_ms   = 2000,
	.polling_cmd_ms = 5,
	.pmu_period_us  = 1000,
	.mreg_cmd_size	= 0x1C0,
	.profile_ver    = PROF_V2,
};

static struct mdla_plat_drv mt5879_drv = {
	.init           = mdla_mt5879_v2_0_init,
	.deinit         = mdla_mt5879_v2_0_deinit,
	.sw_cfg         = BIT(CFG_DUMMY_MMU)
				| BIT(CFG_DTV_SECURE_DRV_RDY)
				| BIT(CFG_MICRO_P_SUPPORT),
	.klog           = (MDLA_DBG_TIMEOUT),
	.timeout_ms     = 6000,
	.off_delay_ms   = 2000,
	.polling_cmd_ms = 5,
	.pmu_period_us  = 1000,
	.mreg_cmd_size	= 0x1C0,
	.profile_ver    = PROF_V2,
};

static const struct of_device_id mdla_of_match[] = {
	{ .compatible = "mtk,mt5896-mdla-dummy", .data = &mt5896_drv},
	{ .compatible = "mtk,mt5897-mdla-dummy", .data = &mt5897_drv},
	{ .compatible = "mtk,mt5896-mdla", .data = &mt589x_drv},
	{ .compatible = "mtk,mt5897-mdla", .data = &mt589x_drv},
	{ .compatible = "mtk,mt5896_e3-mdla", .data = &mt589x_drv},
	{ .compatible = "mtk,mt5879-mdla", .data = &mt5879_drv},
	{ /* end of list */},
};
MODULE_DEVICE_TABLE(of, mdla_of_match);

const struct of_device_id *mdla_plat_get_device(void)
{
	return mdla_of_match;
}
