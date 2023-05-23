/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef CPUFREQ_MTKTV_H
#define CPUFREQ_MTKTV_H

#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/types.h>

typedef enum {
	E_MTKTV_DEBUG_BOOST     = 0,
	E_MTKTV_LUNCH_BOOST     = 1,
	E_MTKTV_IR_BOOST        = 2,
	E_MTKTV_BOOST_NONE      = 3,
} mtktv_boost_client_id;

typedef enum {
	E_BOOST_RESULT_OK            = 0x0,
	E_BOOST_RESULT_IGNORE        = 0x1,
	E_BOOST_RESULT_INVALID_ID    = 0x2,
	E_BOOST_RESULT_INVALID_TYPE  = 0x3,
	E_BOOST_RESULT_INVALID_TIME  = 0x4,
} boost_result;

typedef enum {
	E_BOOST_ENABLE             = 0x0,
	E_BOOST_DISABLE            = 0x1,
	E_BOOST_ENABLE_WITH_TIME   = 0x2,
} boost_type;

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)

#define E_DVFS_OPTEE_INIT       (0)
#define E_DVFS_OPTEE_ADJUST     (1)
#define E_DVFS_OPTEE_SUSPEND    (2)
#define E_DVFS_OPTEE_RESUME     (3)

typedef enum {
	TA_SUCCESS = 0x00000000,
	TA_ERROR   = 0xFFFF0006,
} ta_result;

u32 MTKTV_CPUFREQ_TEEC_TA_Open(void);
u32 MTKTV_CPUFREQ_TEEC_Init(void);
u32 MTKTV_CPUFREQ_TEEC_Adjust(u32 frequency);
u32 MTKTV_CPUFREQ_TEEC_Suspend(void);
u32 MTKTV_CPUFREQ_TEEC_Resume(void);
#endif

// for debug use
#define CPU_HOTPLUG                     (0)
#define SAMPLE_TIMES          (32)
#define MAX_BUFFER_SIZE                 (100)

struct mtktv_cpu_dvfs_info {
	struct regulator *reg;
	struct cpumask cpus;
	struct device *cpu_dev;
	struct list_head list_head;

	/* for boost used */
	unsigned int default_min;
	unsigned int default_max;
	/* for miffy used */
	unsigned int default_ir_boost_min;
	unsigned int default_ir_boost_max;
	unsigned int default_scaling_second;
};


#endif

