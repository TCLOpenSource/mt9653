/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MDLA_PLAT_INTERNAL_H__
#define __MDLA_PLAT_INTERNAL_H__

#include <linux/platform_device.h>

#define MDLA_CPU_IOVA_BASE		0x200000000
#define MDLA_CPU_LX_BASE		0x20000000
static inline u32 MIU_CPU_PA_TO_MDLA_MVA(u32 addr)
{
	return (addr > MDLA_CPU_LX_BASE)?(addr - MDLA_CPU_LX_BASE):addr;
}

enum PLAT_SW_CONFIG {
	CFG_DUMMY_PWR,
	CFG_DUMMY_MMU,
	CFG_DUMMY_MID,

	CFG_NN_PMU_SUPPORT,
	CFG_SW_PREEMPTION_SUPPORT,
	CFG_MICRO_P_SUPPORT,

	CFG_DTV_SECURE_DRV_RDY,

	SW_CFG_MAX
};

struct mdla_plat_drv {
	int (*init)(struct platform_device *pdev);
	void (*deinit)(struct platform_device *pdev);
	unsigned int sw_cfg;
	unsigned int klog;
	unsigned int timeout_ms;
	unsigned int off_delay_ms;
	unsigned int polling_cmd_ms;
	unsigned int pmu_period_us;
	unsigned int mreg_cmd_size;	// JIMMY: for hw ver.
	int profile_ver;
};

int mdla_v1_0_init(struct platform_device *pdev);
void mdla_v1_0_deinit(struct platform_device *pdev);

int mdla_v1_7_init(struct platform_device *pdev);
void mdla_v1_7_deinit(struct platform_device *pdev);

int mdla_v1_5_init(struct platform_device *pdev);
void mdla_v1_5_deinit(struct platform_device *pdev);

int mdla_v2_0_init(struct platform_device *pdev);
void mdla_v2_0_deinit(struct platform_device *pdev);

int mdla_mt589x_v2_0_init(struct platform_device *pdev);
void mdla_mt589x_v2_0_deinit(struct platform_device *pdev);

int mdla_mt5896_v2_0_init(struct platform_device *pdev);
void mdla_mt5896_v2_0_deinit(struct platform_device *pdev);

int mdla_mt5897_v2_0_init(struct platform_device *pdev);
void mdla_mt5897_v2_0_deinit(struct platform_device *pdev);

int mdla_mt5879_v2_0_init(struct platform_device *pdev);
void mdla_mt5879_v2_0_deinit(struct platform_device *pdev);

#endif /* __MDLA_PLAT_INTERNAL_H__ */

