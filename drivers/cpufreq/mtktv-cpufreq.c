// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpu_cooling.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/suspend.h>
#include <linux/ioport.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/kconfig.h>
#include <linux/input.h>

#include <linux/sched/signal.h>
#include <linux/sched/debug.h>
#include <uapi/linux/sched/types.h>

#include <linux/atomic.h>
#include <linux/irq_work.h>
#include <linux/sched/cpufreq.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>

#include "mtktv-cpufreq.h"

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
extern int tee_get_supplicant_count(void);
static int ta_init;
static struct task_struct *lunch_boost_task;

/* for optee suspend handler */
static struct notifier_block cpufreq_pm_suspend_notifier;

DECLARE_WAIT_QUEUE_HEAD(cold_boot_wait);
unsigned long cold_boot_finish;
#endif

#if IS_ENABLED(CONFIG_CPU_FREQ_STAT)
/* for cpufreq stats */
struct cpufreq_stats {
	unsigned int total_trans;
	unsigned long long last_time;
	unsigned int max_state;
	unsigned int state_num;
	unsigned int last_index;
	u64 *time_in_state;
	spinlock_t lock;
	unsigned int *freq_table;
	unsigned int *trans_table;
};
#endif

#if IS_ENABLED(CONFIG_CPU_FREQ_GOV_CONSERVATIVE)
#define DOWN_THRESHOLD (50)
#define GOVERNOR_NAME  "conservative"
struct cs_dbs_tuners {
	unsigned int down_threshold;
	unsigned int freq_step;
};

struct dbs_data {
	struct gov_attr_set attr_set;
	void *tuners;
	unsigned int ignore_nice_load;
	unsigned int sampling_rate;
	unsigned int sampling_down_factor;
	unsigned int up_threshold;
	unsigned int io_is_busy;
};

/* Common to all CPUs of a policy */
struct policy_dbs_info {
	struct cpufreq_policy *policy;
	/*
	 * Per policy mutex that serializes load evaluation from limit-change
	 * and work-handler.
	 */
	struct mutex update_mutex;

	u64 last_sample_time;
	s64 sample_delay_ns;
	atomic_t work_count;
	struct irq_work irq_work;
	struct work_struct work;
	/* dbs_data may be shared between multiple policy objects */
	struct dbs_data *dbs_data;
	struct list_head list;
	/* Multiplier for increasing sample delay temporarily. */
	unsigned int rate_mult;
	unsigned int idle_periods;  /* For conservative */
	/* Status indicators */
	bool is_shared;     /* This object is used by multiple CPUs */
	bool work_in_progress;  /* Work is being queued up or in progress */
};
#endif
/*********************************************************************
 *                         Private Structure                         *
 *********************************************************************/
struct mtktv_boost_client {
	mtktv_boost_client_id id;
	boost_type type;
	unsigned int time;
	s32 rate;
};

enum corner_type {
	E_CORNER_SS   = 0,
	E_CORNER_TT   = 1,
	E_CORNER_FF   = 2,
	E_CORNER_NONE = 3,
};

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static void MTK_CLK_CORE_Adjust(unsigned int cur, unsigned int target);
static void MTK_CLK_DSU_Adjust(unsigned int cur, unsigned int target);
#endif
/*********************************************************************
 *                         Private Macro                             *
 *********************************************************************/
static void __iomem *cpufreq_pm_base;
static void __iomem *cpufreq_pm_base2;
#define BASE_ADDRESS                    (cpufreq_pm_base)
#define BASE_ADDRESS_2                  (cpufreq_pm_base2 + 0xA00)

#define CLK_DSU(CORE) (CORE*7/10)	// DSU = CORE x 0.7
#define M6L_CLK_DSU(CORE) (CORE*668/1000)   // DSU = CORE x 0.668

#define LUNCH_BOOST_TIME_MS             (1*1000)
#define MAX_NAME_SIZE                   (100)
#define MAX_APP_NUM                     (10)
#define CPU_TRANS_DELAY_US              (10000)
#define CPUFREQ_DELTA                   (10000)
#define M6L_CPUFREQ_MAX                 (2000000)
#define NUM_1000                        (1000)
#define NUM_16                          (16)
#define NUM_2                           (2)

#define CPUFREQ_DEBUG(x, args...) \
	{if (debug_enable) \
		pr_emerg(x, ##args); }

#define CPUFREQ_INFO(x, args...)  \
	{if (info_enable) \
		pr_emerg(x, ##args); }

#define CPUFREQ_TRACER(x, args...)  \
	{if (tracer_enable) \
		pr_emerg(x, ##args); }

/* chip ID */
#define M6_ID  (0x101)
#define M6L_ID  (0x109)
#define MOKONA_ID  (0x10C)
#define MIFFY_ID  (0x10D)
#define MOKA_ID  (0x111)
#define UNKNOWN_ID  (0)

/* corner */
#define REG_DVFS_CORNER       (0x10050A)
#define CPU_CORNER            (0x3)

/* common */
#define DEVICE_NAME           (100)
#define BUFFER_SIZE           (100)

#define NUMBER_2      (2)

#define REG_04        (0x04)//8 BIT ADDR
#define REG_0C        (0x0C)
#define REG_0E        (0x0E)

#define REG_40        (0x40)
#define REG_41        (0x41)
#define REG_42        (0x42)
#define REG_44        (0x44)
#define REG_45        (0x45)
#define REG_46        (0x46)
#define REG_48        (0x48)
#define REG_4A        (0x4A)
#define REG_4E        (0x4E)

#define REG_50        (0x50)
#define REG_51        (0x51)
#define REG_52        (0x52)
#define REG_53        (0x53)
#define REG_5A        (0x5A)

#define REG_F4        (0xF4)
#define REG_F5        (0xF5)
#define REG_F6        (0xF6)
#define REG_F7        (0xF7)

#define CLK_GET_CORE_SEL_CPU    (0x04)
#define CLK_GET_CORE_DECT_CPU   (0x00)
#define CLK_GET_CORE_STATUS_CPU (0x01)
#define CLK_GET_CORE_MASK       (0x03)
#define CLK_GET_CORE_MASK_VAL   (0x03)

#define BANK_OFFSET_1			(0x100800)
#define BANK_CLK_SET_CORE       (0x100800-BANK_OFFSET_1)//BANK:0x1008
#define BANK_CLK_SET_DSU        (0x100900-BANK_OFFSET_1)//BANK:0x1009
#define BANK_CLK_SET_MUX        (0x102000-BANK_OFFSET_1)//BANK:0x1020
#define BANK_OFFSET_2			(0x200D00)
#define BANK_CLK_GET_CORE       (0x200D00-BANK_OFFSET_2)//BANK:0x200D
/*********************************************************************
 *                         Local Function                            *
 *********************************************************************/
/* cpufreq driver CB */
static int mtktv_cpufreq_init(struct cpufreq_policy *policy);
static int mtktv_cpufreq_exit(struct cpufreq_policy *policy);
static void mtktv_cpufreq_ready(struct cpufreq_policy *policy);
static int mtktv_cpufreq_resume(struct cpufreq_policy *policy);
static int mtktv_cpufreq_suspend(struct cpufreq_policy *policy);
static int mtktv_cpufreq_set_target(struct cpufreq_policy *policy,
		unsigned int index);

/* common */
#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static void cpufreq_writeb(u8 data, unsigned int addr);
static u8 cpufreq_readb(unsigned int addr);
static void cpufreq_writew(u16 data, unsigned int addr);
#endif
static void cpufreq_writeb2(u8 data, unsigned int addr);
static u16 cpufreq_readw2(unsigned int addr);
struct mtktv_cpu_dvfs_info *mtktv_cpu_dvfs_info_lookup(int cpu);

/* boost */
static void mtktv_boost_init(void);
static void mtktv_boost_enable(unsigned int bEnable, s32 rate);
static void input_boost_init(void);
static int is_scaling_to_boost_freq;
static int input_boost_enable; // dynamic input boost flag

/* corner ic @REE DVFS ONLY*/
static enum corner_type cpufreq_get_corner(void);
static int mtktv_cpufreq_opp_init(void);

/*********************************************************************
 *                         Private Data                              *
 *********************************************************************/
DEFINE_MUTEX(boost_mutex);
DEFINE_MUTEX(clk_mutex);
DEFINE_MUTEX(target_mutex);

static LIST_HEAD(dvfs_info_list);

static u32 u32Current;
static int thermal_done;

struct delayed_work disable_work;

static struct mtktv_boost_client available_boost_list[E_MTKTV_BOOST_NONE];
static struct mtktv_boost_client current_boost;

static int suspend;
static int regulator_init;
static int deep_suspend;
static int suspend_frequency;
#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static int cold_boot_done;
#endif

static struct cpufreq_frequency_table *mtktv_freq_table[CONFIG_NR_CPUS];
static struct thermal_cooling_device *cdev;
static int chip_id;

/* for debug used */
int auto_measurement;
EXPORT_SYMBOL(auto_measurement);
static int debug_enable;
static int info_enable;
static int tracer_enable;


/*********************************************************************
 *                          Common Function                          *
 *********************************************************************/
struct mtktv_cpu_dvfs_info *mtktv_cpu_dvfs_info_lookup(int cpu)
{
	struct mtktv_cpu_dvfs_info *info;

	list_for_each_entry(info, &dvfs_info_list, list_head) {
		if (cpumask_test_cpu(cpu, &info->cpus))
			return info;
	}
	return NULL;
}
EXPORT_SYMBOL(mtktv_cpu_dvfs_info_lookup);

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static void cpufreq_writeb(u8 data, unsigned int addr)
{
	writeb(data, (void __iomem *)(BASE_ADDRESS + (addr << 1)));
}

static u8 cpufreq_readb(unsigned int addr)
{
	return (u8)readw((void __iomem *)(BASE_ADDRESS + (addr << 1)));
}

static void cpufreq_writew(u16 data, unsigned int addr)
{
	writew(data, (void __iomem *)(BASE_ADDRESS + (addr << 1)));
}
#endif
static void cpufreq_writeb2(u8 data, unsigned int addr)
{
	writeb(data, (void __iomem *)(BASE_ADDRESS_2 + (addr << 1)));
}

static u16 cpufreq_readw2(unsigned int addr)
{
	return (u16)readw((void __iomem *)(BASE_ADDRESS_2 + (addr << 1)));
}

/*********************************************************************
 *                             CLK Related                           *
 *********************************************************************/
unsigned int mtktv_cpufreq_get_clk(unsigned int cpu)
{

	u32 u32Clock = 0;
	int opp_cnt = 0, i = 0, found_table = 0;
	struct cpufreq_frequency_table *freq_table = NULL;

	mutex_lock(&clk_mutex);
	if (u32Current == 0 || auto_measurement) {
		//1. CPU core clock select
		cpufreq_writeb2(CLK_GET_CORE_SEL_CPU, BANK_CLK_GET_CORE+REG_F7);
		//2. CPU core clock detect
		cpufreq_writeb2(CLK_GET_CORE_DECT_CPU, BANK_CLK_GET_CORE+REG_F6);
		udelay(1);
		cpufreq_writeb2(CLK_GET_CORE_STATUS_CPU, BANK_CLK_GET_CORE+REG_F6);
		/* */
		do {
			udelay(50);
		} while ((cpufreq_readw2(BANK_CLK_GET_CORE+REG_F6) & CLK_GET_CORE_MASK)
			!= CLK_GET_CORE_MASK_VAL);
		u32Clock = (u32)cpufreq_readw2(BANK_CLK_GET_CORE+REG_F4);
		u32Clock *= 192;//clk*((16*1000*1000)/83333)=clk*192
		u32Clock /= 1000;
		u32Clock *= 1000;

		if (!auto_measurement) {
			opp_cnt = dev_pm_opp_get_opp_count(get_cpu_device(0));
			dev_pm_opp_init_cpufreq_table(get_cpu_device(0), &freq_table);
			for (i = 0; i < opp_cnt; i++) {
				CPUFREQ_DEBUG("[mtktv-cpufreq]frequency: %u KHz\n"
						      , freq_table[i].frequency);
				if ((u32Clock > freq_table[i].frequency - CPUFREQ_DELTA)
					&& (u32Clock < freq_table[i].frequency + CPUFREQ_DELTA)) {
					u32Clock = freq_table[i].frequency;
					found_table = 1;
					break;
				}
			}
			// not found in table, set to default
			if (!found_table)
				u32Clock = freq_table[1].frequency;
			kfree(freq_table);
			CPUFREQ_DEBUG("u32Clock = %d\n", u32Clock);
		}
		u32Current = u32Clock;
	}
	u32Clock = u32Current;
	mutex_unlock(&clk_mutex);
	return u32Clock; //KHz
}
EXPORT_SYMBOL(mtktv_cpufreq_get_clk);

int mtktv_cpufreq_set_clk(unsigned int u32Current,
		unsigned int u32Target)
{
	CPUFREQ_TRACER("[CPUFREQ_TRACER] start:\n"
			"current clock = %d\n"
			"target clock = %d\n",
			u32Current, u32Target);

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	mutex_lock(&clk_mutex);
	u32Current /= 1000;//KHz->MHz
	u32Target /= 1000;//KHz->MHz

	switch (chip_id) {
	case M6_ID:
		if (u32Target >  u32Current) {
			MTK_CLK_DSU_Adjust(CLK_DSU(u32Current), CLK_DSU(u32Target));
			MTK_CLK_CORE_Adjust(u32Current, u32Target);
		} else {
			MTK_CLK_CORE_Adjust(u32Current, u32Target);
			MTK_CLK_DSU_Adjust(CLK_DSU(u32Current), CLK_DSU(u32Target));
		}
		break;
	case M6L_ID:
		if (u32Target >  u32Current) {
			if (u32Target == M6L_CPUFREQ_MAX)
				MTK_CLK_DSU_Adjust(M6L_CLK_DSU(u32Current), CLK_DSU(u32Target));
			else
				MTK_CLK_DSU_Adjust(M6L_CLK_DSU(u32Current), M6L_CLK_DSU(u32Target));
			MTK_CLK_CORE_Adjust(u32Current, u32Target);
		} else {
			MTK_CLK_CORE_Adjust(u32Current, u32Target);
			if (u32Current == M6L_CPUFREQ_MAX)
				MTK_CLK_DSU_Adjust(CLK_DSU(u32Current), M6L_CLK_DSU(u32Target));
			else
				MTK_CLK_DSU_Adjust(M6L_CLK_DSU(u32Current), M6L_CLK_DSU(u32Target));
		}
	case MOKONA_ID:
	case MOKA_ID:
	case MIFFY_ID:
			if(u32Target != u32Current)
				MTK_CLK_CORE_Adjust(u32Current, u32Target);
		break;

	default:
		CPUFREQ_INFO("[CPUFREQ_INFO] chip unknown, no change cpufreq\n");
	}
	mutex_unlock(&clk_mutex);
#endif

	CPUFREQ_TRACER("\033[1;31m[CPUFREQ_TRACER] end:\n"
			"current clock = %d \033[0m\n",
			mtktv_cpufreq_get_clk(0));
	return 0;
}
EXPORT_SYMBOL(mtktv_cpufreq_set_clk);

/*********************************************************************
 *                         Regulator Related                         *
 *********************************************************************/
int mtktv_cpufreq_set_voltage(struct mtktv_cpu_dvfs_info *info,
		int vproc)
{
	int ret = 0;

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	CPUFREQ_TRACER("[CPUFREQ_TRACER] start:\n"
			"current voltage = %d\n",
			regulator_get_voltage(info->reg));

	if (regulator_init == 0) {
		regulator_init = 1;
		if ((deep_suspend == 1) || (cold_boot_done == 0)) {
			regulator_enable(info->reg);
			deep_suspend = 0;
			cold_boot_done = 1;
		}
	}
	ret = regulator_set_voltage(info->reg, vproc, vproc);

	CPUFREQ_TRACER("[CPUFREQ_TRACER] end:\n"
			"current voltage = %d\n",
			regulator_get_voltage(info->reg));
#endif

	return ret;
}
EXPORT_SYMBOL(mtktv_cpufreq_set_voltage);

/*********************************************************************
 *                          Boost Related (General)                  *
 *********************************************************************/

static void mtktv_boost_init(void)
{
	unsigned int index, opp_cnt;
	struct mtktv_cpu_dvfs_info *info;
	int cpu = 0;
	unsigned int value = 0;
	struct device_node *node = NULL;
	char node_name[DEVICE_NAME];

	info = mtktv_cpu_dvfs_info_lookup(cpu);
	if (!info) {
		pr_err("[mtktv-cpufreq] dvfs info for cpu%d is not initialized.\n",
				cpu);
		return;
	}

	opp_cnt = dev_pm_opp_get_opp_count(info->cpu_dev);
	node = of_find_node_by_name(NULL, "opp-table");
	if (node == NULL) {
		pr_err("[mtktv-cpufreq] opp-table not found.\n");
		return;
	};

	for (index = 0; index < E_MTKTV_BOOST_NONE; index++) {
		available_boost_list[index].id = (mtktv_boost_client_id)index;
		available_boost_list[index].time = 0;
		memset(node_name, 0, DEVICE_NAME);
		snprintf(node_name, DEVICE_NAME, "boost-%d", index);
		of_property_read_u32(node, node_name, &value);
		available_boost_list[index].rate = value/1000;
		CPUFREQ_INFO("[mtktv-cpufreq][available_boost_list]\n"
				"index = %d, clk = %d, %s\n",
				index,
				available_boost_list[index].rate,
				node_name);
	}

	current_boost.id = E_MTKTV_BOOST_NONE;
}

static void mtktv_boost_enable(unsigned int enable, s32 rate)
{
	struct cpufreq_policy *policy = NULL;
	unsigned int cpu = 0;
	struct mtktv_cpu_dvfs_info *info;
	int target_min, target_max;

	policy = cpufreq_cpu_get_raw(cpu);
	if (!policy) {
		pr_err("[mtktv-cpufreq] cpu policy is not initialized.\n");
		return;
	}

	info = mtktv_cpu_dvfs_info_lookup(cpu);
	if (!info) {
		pr_err("[mtktv-cpufreq] dvfs info for cpu%d is not initialized.\n",
				cpu);
		return;
	}

	if (current_boost.id > E_MTKTV_BOOST_NONE || current_boost.id < 0) {
		pr_err("[mtktv-cpufreq] boost id(%d) is invalid\n", current_boost.id);
		return;
	}

	down_write(&policy->rwsem);

	if (enable == 0) {
		if (chip_id == MIFFY_ID) {
			target_min = info->default_min;
			// avoid IR boost to disable boost freq
			if (!is_scaling_to_boost_freq)
				target_max = info->default_scaling_second;
			else
				target_max = info->default_max;
		} else {
			target_min = info->default_min;
			target_max = info->default_max;
		}
		current_boost.id = E_MTKTV_BOOST_NONE;
	} else {
		if ((rate == 0 && current_boost.id < E_MTKTV_BOOST_NONE)) {
			if (chip_id == MIFFY_ID && current_boost.id == E_MTKTV_IR_BOOST) {
				target_min = info->default_ir_boost_min;
				target_max = info->default_ir_boost_max;
			} else {
				target_min = available_boost_list[current_boost.id].rate;
				target_max = available_boost_list[current_boost.id].rate;
			}
		} else {
			if (chip_id == MIFFY_ID && current_boost.id == E_MTKTV_IR_BOOST) {
				target_min = info->default_ir_boost_min;
				target_max = info->default_ir_boost_max;
			} else {
				target_min = rate;
				target_max = rate;
			}
		}
	}
	freq_qos_update_request(policy->min_freq_req, target_min);
	freq_qos_update_request(policy->max_freq_req, target_max);
	CPUFREQ_DEBUG("[mtktv-cpufreq] min = %d, max = %d\n",
			target_min, target_max);

	up_write(&policy->rwsem);
	cpufreq_update_policy(cpu);
}

int mtktv_scaling_to_boost_freq(unsigned int enable)
{
	struct cpufreq_policy *policy = NULL;
	unsigned int cpu = 0;
	struct mtktv_cpu_dvfs_info *info;
	int target_min, target_max;

	policy = cpufreq_cpu_get_raw(cpu);
	if (!policy) {
		pr_err("[mtktv-cpufreq] cpu policy is not initialized.\n");
		return 0;
	}

	info = mtktv_cpu_dvfs_info_lookup(cpu);
	if (!info) {
		pr_err("[mtktv-cpufreq] dvfs info for cpu%d is not initialized.\n",
				cpu);
		return 0;
	}

	down_write(&policy->rwsem);
	if (enable == 0) {
		target_min = info->default_min;
		target_max = info->default_scaling_second;
		is_scaling_to_boost_freq = 0;
	} else {
		target_min = info->default_min;
		target_max = available_boost_list[E_MTKTV_DEBUG_BOOST].rate;
		is_scaling_to_boost_freq = 1;
	}
	freq_qos_update_request(policy->min_freq_req, target_min);
	freq_qos_update_request(policy->max_freq_req, target_max);
	CPUFREQ_DEBUG("[mtktv-cpufreq] scaling freq min = %d, max = %d\n",
			target_min, target_max);

	up_write(&policy->rwsem);
	cpufreq_update_policy(cpu);
	return 0;
}
EXPORT_SYMBOL(mtktv_scaling_to_boost_freq);

int mtktv_set_ir_boost_range(unsigned int min_freq, unsigned int max_freq)
{
	struct cpufreq_policy *policy = NULL;
	unsigned int cpu = 0;
	struct mtktv_cpu_dvfs_info *info;
	int opp_cnt = 0, i = 0, min_freq_found = 0, max_freq_found = 0;
	struct cpufreq_frequency_table *freq_table = NULL;

	policy = cpufreq_cpu_get_raw(cpu);
	if (!policy) {
		pr_err("[mtktv-cpufreq] cpu policy is not initialized.\n");
		return -EINVAL;
	}

	info = mtktv_cpu_dvfs_info_lookup(cpu);
	if (!info) {
		pr_err("[mtktv-cpufreq] dvfs info for cpu%d is not initialized.\n",
				cpu);
		return -EINVAL;
	}

	// check max & min freq in table
	opp_cnt = dev_pm_opp_get_opp_count(get_cpu_device(0));
	dev_pm_opp_init_cpufreq_table(get_cpu_device(0), &freq_table);
	for (i = 0; i < opp_cnt; i++) {
		if (freq_table[i].frequency == min_freq)
			min_freq_found = 1;
		if (freq_table[i].frequency == max_freq)
			max_freq_found = 1;
	}
	kfree(freq_table);

	if ((max_freq_found != 1) || (min_freq_found != 1))
		return -EINVAL;

	down_write(&policy->rwsem);
	info->default_ir_boost_max = max_freq;
	info->default_ir_boost_min = min_freq;
	up_write(&policy->rwsem);

	return 0;
}
EXPORT_SYMBOL(mtktv_set_ir_boost_range);

void mtktv_enable_ir_boost(unsigned int enable)
{
	input_boost_enable = enable;
}
EXPORT_SYMBOL(mtktv_enable_ir_boost);

static void mtktv_disable_boost(struct work_struct *work)
{
	if (suspend != 1)
		mtktv_boost_enable(0, 0);
}

boost_result mtktv_boost_register(mtktv_boost_client_id id,
		int freq,
		boost_type type,
		int time)
{
	boost_result ret = E_BOOST_RESULT_OK;

	if (id > E_MTKTV_BOOST_NONE || id < 0) {
		pr_err("[mtktv-cpufreq] boost id(%d) is invalid\n", id);
		return E_BOOST_RESULT_INVALID_ID;
	}

	mutex_lock(&boost_mutex);

	if (suspend == 1) {
		mutex_unlock(&boost_mutex);
		return ret;
	}

	if (current_boost.id == E_MTKTV_BOOST_NONE) {
		if (type != E_BOOST_DISABLE) {
			current_boost.id = id;

			/*
			 * only allow the debug boost client to
			 * adjust user defiend frequency
			 */
			if ((id == E_MTKTV_DEBUG_BOOST) && (freq != 0)) {
				mtktv_boost_enable(1, freq);
			} else {
				if (id < E_MTKTV_BOOST_NONE)
					mtktv_boost_enable(1,
						available_boost_list[id].rate);
			}

			/*
			 * request a delay kwork to
			 * disable boosting with user defined time
			 */
			if ((type == E_BOOST_ENABLE_WITH_TIME)
					&& (time != 0)) {
				current_boost.time = time;
				schedule_delayed_work(&disable_work,
						msecs_to_jiffies(time));
			}
		}
	} else {
		/*
		 *if request client has lower priority compare to
		 * current boost client, then skip the request
		 */
		if (current_boost.id < id) {
			ret = E_BOOST_RESULT_IGNORE;
		} else if (current_boost.id == id) {
			if (type == E_BOOST_DISABLE) {
				current_boost.id = E_MTKTV_BOOST_NONE;
				mtktv_boost_enable(0, 0);
			} else if ((type == E_BOOST_ENABLE_WITH_TIME)
						&& (time != 0)) {
				/*
				 * if current boost client has
				 * new time interval to boost,
				 * then delete previous kwork and request a new
				 */
				cancel_delayed_work(&disable_work);
				current_boost.id = id;
				current_boost.time = time;
				mtktv_boost_enable(1,
						available_boost_list[id].rate);
				schedule_delayed_work(&disable_work,
						msecs_to_jiffies(time));
			}
		} else {
			/*
			 * request boost client has higher priority
			 * than current boost client
			 */
			cancel_delayed_work(&disable_work);
			current_boost.id = id;
			mtktv_boost_enable(1,
					available_boost_list[id].rate);
			if ((type == E_BOOST_ENABLE_WITH_TIME)
					&& (time != 0)) {
				current_boost.time = time;
				schedule_delayed_work(&disable_work,
						msecs_to_jiffies(time));
			}
		}
	}

	mutex_unlock(&boost_mutex);
	return ret;
}
EXPORT_SYMBOL(mtktv_boost_register);

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static int lunch_boost_thread(void *arg)
{
	wait_event_interruptible(cold_boot_wait, cold_boot_finish);
	mtktv_boost_register(E_MTKTV_LUNCH_BOOST, 0,
		E_BOOST_ENABLE_WITH_TIME, LUNCH_BOOST_TIME_MS);
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_CPU_FREQ_STAT)
static void mtktv_cpufreq_stats_update(struct cpufreq_stats *stats)
{
	unsigned long long cur_time = get_jiffies_64();

	stats->time_in_state[stats->last_index] += cur_time - stats->last_time;
	stats->last_time = cur_time;
}

u64 mtktv_cpufreq_stats(void)
{
	unsigned int cpu = 0;
	struct cpufreq_policy *policy = NULL;
	struct cpufreq_stats *stats = NULL;
	u64 max_freq_in_time = 0;

	policy = cpufreq_cpu_get_raw(cpu);
	if (!policy) {
		pr_err("[mtktv-cpufreq] cpu policy is not initialized.\n");
		return 0;
	}

	stats = policy->stats;
	mtktv_cpufreq_stats_update(stats);

	max_freq_in_time = (unsigned long long)
			jiffies_64_to_clock_t(stats->time_in_state[stats->state_num - 1]);
	CPUFREQ_INFO("%u %llu\n", stats->freq_table[stats->state_num - 1], max_freq_in_time);

	// mtktv_cpufreq_stats_clear_table(stats);

	return max_freq_in_time;
}
EXPORT_SYMBOL(mtktv_cpufreq_stats);
#endif

#define INPUT_BOOST_RELEASE_TIME	1000  // in ms
static struct work_struct input_boost_start_work;
static struct delayed_work  input_boost_stop_work;

static void input_boost_start(struct work_struct *work)
{
	CPUFREQ_INFO("%s: call E_BOOST_ENABLE\n", __func__);
	mtktv_boost_register(E_MTKTV_IR_BOOST, 0, E_BOOST_ENABLE, 0);
}

static void input_boost_stop(struct work_struct *work)
{
	CPUFREQ_INFO("%s: call E_BOOST_DISABLE\n", __func__);
	mtktv_boost_register(E_MTKTV_IR_BOOST, 0, E_BOOST_DISABLE, 0);
}

static void input_events(struct input_handle *handle,
		const struct input_value *vals, unsigned int count)
{
	int i;

	if (input_boost_enable == 1) {
		for (i = 0; i < count; i++) {
			if (vals[i].type == EV_KEY) {
				CPUFREQ_INFO("%s: dev %s send EV_KEY (state=%c)\n",
						__func__, handle->dev->name,
						vals[i].value < 3 ? "UDR"[vals[i].value] : '?');
				switch (vals[i].value) {
				case 1:
					cancel_delayed_work(&input_boost_stop_work);
					schedule_work(&input_boost_start_work);
					break;
				case 0:
					schedule_delayed_work(&input_boost_stop_work,
					msecs_to_jiffies(INPUT_BOOST_RELEASE_TIME));
					break;
				default:
					break;
				}
			}
		}
	}
}

static struct input_handle *last_handle;	// fix coverity
static int input_connect(struct input_handler *handler, struct input_dev *dev,
		const struct input_device_id *id)
{
	struct input_handle *handle;
	int err;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "input_boost";

	err = input_register_handle(handle);
	if (err)
		goto err_free_handle;

	err = input_open_device(handle);
	if (err)
		goto err_unregister_handle;

	CPUFREQ_INFO("%s: device %s connected\n", __func__, handle->dev->name);
	last_handle = handle;	// fix coverity.
	return 0;

err_unregister_handle:
	input_unregister_handle(handle);
err_free_handle:
	kfree(handle);
	pr_err("%s: failed. error=%d\n", __func__, err);
	return err;
}

static void input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	CPUFREQ_INFO("%s: device %s disconnected\n", __func__, handle->dev->name);
	kfree(handle);
}

static const struct input_device_id input_boost_ids[] = {
	{ .driver_info = 1 },   /* Matches all devices */
	{ },            /* Terminating zero entry */
};

MODULE_DEVICE_TABLE(input, input_boost_ids);

static struct input_handler input_handler = {
	.events     = input_events,
	.connect    = input_connect,
	.disconnect = input_disconnect,
	.name       = "input_boost",
	.id_table   = input_boost_ids,
};

static void input_boost_init(void)
{
	int err;

	switch (chip_id) {
	case MIFFY_ID:
		INIT_WORK(&input_boost_start_work, input_boost_start);
		INIT_DELAYED_WORK(&input_boost_stop_work, input_boost_stop);

		err = input_register_handler(&input_handler);
		if (err)
			pr_err("%s: register input boost handler fail! err=%d\n", __func__, err);
		input_boost_enable = 1;
		break;
	default:
		CPUFREQ_INFO("not support cpufreq INPUT BOOST\n");
		break;
	}
}

/*********************************************************************
 *                         CORNER IC Related                         *
 *********************************************************************/
static enum corner_type cpufreq_get_corner(void)
{
	return E_CORNER_SS;
}

static int mtktv_cpufreq_opp_init(void)
{
#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	struct device_node *child = NULL;
	char node_name[MAX_NAME_SIZE];
	int old_vproc = 0, value = 0;
#endif
	struct device_node *node = NULL;
	struct mtktv_cpu_dvfs_info *info;
	struct dev_pm_opp *opp;
	unsigned long freq_hz = 0;
	int vproc = 0, opp_cnt = 0, i = 0;
	struct cpufreq_frequency_table *freq_table = NULL;

	node = of_find_node_by_name(NULL, "opp-table");
	if (node == NULL)
		return -EINVAL;

	info = mtktv_cpu_dvfs_info_lookup(0);
	if (!info)
		return -EINVAL;

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	if (cpufreq_get_corner() == E_CORNER_FF) {
		opp_cnt = dev_pm_opp_get_opp_count(info->cpu_dev);
		dev_pm_opp_init_cpufreq_table(info->cpu_dev, &freq_table);
		for (i = 0; i < opp_cnt; i++) {
			freq_hz = freq_table[i].frequency;
			freq_hz *= 1000;

			opp = dev_pm_opp_find_freq_ceil(info->cpu_dev,
				&freq_hz);
			rcu_read_lock();
			old_vproc = dev_pm_opp_get_voltage(opp);
			rcu_read_unlock();

			dev_pm_opp_remove(info->cpu_dev, freq_hz);

			memset(node_name, 0, MAX_NAME_SIZE);
			snprintf(node_name, MAX_NAME_SIZE, "opp-%ld",
							freq_hz);
			child = of_find_node_by_name(node, node_name);
			of_property_read_u32(child,
					"opp-corner-offset", &value);

			dev_pm_opp_add(info->cpu_dev, freq_hz,
						old_vproc-value);
		}
		kfree(freq_table);
	}
#endif

	opp_cnt = dev_pm_opp_get_opp_count(info->cpu_dev);
	dev_pm_opp_init_cpufreq_table(info->cpu_dev, &freq_table);
	CPUFREQ_INFO("[mtktv-cpufreq] CORNER TYPE:%d\n",
			cpufreq_get_corner());

	for (i = 0; i < opp_cnt; i++) {
		freq_hz = freq_table[i].frequency;
		freq_hz *= 1000;

		opp = dev_pm_opp_find_freq_ceil(info->cpu_dev, &freq_hz);
		rcu_read_lock();
		vproc = dev_pm_opp_get_voltage(opp);
		rcu_read_unlock();
		CPUFREQ_INFO("[mtktv-cpufreq] clock = %ld, voltage = %d\n",
				freq_hz, vproc);
	}
	kfree(freq_table);
	return 0;
}

/*********************************************************************
 *                         CPUFREQ Driver CB                         *
 *********************************************************************/

static int mtktv_cpufreq_init(struct cpufreq_policy *policy)
{
	struct mtktv_cpu_dvfs_info *info = NULL;
	struct cpufreq_frequency_table *freq_table = NULL;
	int ret, i;
	unsigned int cpu = policy->cpu;
	unsigned int index = 0;
	unsigned int opp_cnt = 0;

	info = mtktv_cpu_dvfs_info_lookup(cpu);
	if (!info) {
		pr_err("[mtktv-cpufreq] dvfs info for cpu%d is not initialized.\n",
				policy->cpu);
		return -EINVAL;
	}

	opp_cnt = dev_pm_opp_get_opp_count(info->cpu_dev);
	if (mtktv_freq_table[cpu] == NULL) {
		ret = dev_pm_opp_init_cpufreq_table(info->cpu_dev, &freq_table);
		if (ret) {
			pr_err("[mtktv-cpufreq] failed to init cpufreq table\n"
				"for cpu%d: %d\n", policy->cpu, ret);
			return ret;
		}
		mtktv_freq_table[cpu] = kcalloc((opp_cnt + 1),
			sizeof(struct cpufreq_frequency_table), GFP_KERNEL);
		for (index = 0; index < opp_cnt; index++) {
			mtktv_freq_table[cpu][index].frequency =
				freq_table[index].frequency;
			CPUFREQ_INFO("[mtktv-cpufreq] [CPU%d]\n"
				"index = %d, frequency = %d\n",
				cpu, index,
				mtktv_freq_table[cpu][index].frequency);
		}
		mtktv_freq_table[cpu][opp_cnt].frequency = CPUFREQ_TABLE_END;
		kfree(freq_table);
	}

	suspend_frequency = mtktv_freq_table[cpu][1].frequency;
	info->default_min = policy->min = mtktv_freq_table[cpu][1].frequency;
	info->default_max = policy->max =
		available_boost_list[E_MTKTV_DEBUG_BOOST].rate;
	policy->cpuinfo.min_freq = mtktv_freq_table[cpu][0].frequency;
	policy->cpuinfo.max_freq = mtktv_freq_table[cpu][opp_cnt-1].frequency;

	// setting ir boost min & max
	if (chip_id == MIFFY_ID) {
		info->default_scaling_second = mtktv_freq_table[cpu][opp_cnt-NUM_2].frequency;
		info->default_ir_boost_min = mtktv_freq_table[cpu][opp_cnt-NUM_2].frequency;
		info->default_ir_boost_max = available_boost_list[E_MTKTV_IR_BOOST].rate;
		is_scaling_to_boost_freq = 1; // 0: 1.3G, 1: 1.39G
	} else {
		info->default_scaling_second = mtktv_freq_table[cpu][opp_cnt-NUM_2].frequency;
		info->default_ir_boost_min = available_boost_list[E_MTKTV_IR_BOOST].rate;
		info->default_ir_boost_max = available_boost_list[E_MTKTV_IR_BOOST].rate;
	}

	for_each_online_cpu(i)
		if (i < CONFIG_NR_CPUS)
			cpumask_set_cpu(i, policy->cpus);

	policy->freq_table = mtktv_freq_table[cpu];
	policy->cur = mtktv_cpufreq_get_clk(cpu);
	policy->transition_delay_us = CPU_TRANS_DELAY_US;

	CPUFREQ_INFO("[mtktv-cpufreq] [CPU%d]\n"
			"scaling_max: %d, scaling_min: %d\n"
			"cpu_max: %d, cpu_min: %d\n",
			cpu, policy->max, policy->min,
			policy->cpuinfo.max_freq, policy->cpuinfo.min_freq);

	// open TA context first avoid no freq chagne then call suspend
	if (MTKTV_CPUFREQ_TEEC_TA_Open() != TA_SUCCESS)
		pr_err("[mtktv-cpufreq] CPUFREQ TA Open fail\n");

	return 0;
}

static int mtktv_cpufreq_exit(struct cpufreq_policy *policy)
{
	return 0;
}

static void mtktv_cpufreq_ready(struct cpufreq_policy *policy)
{
	struct device_node *np;
	char node[BUFFER_SIZE];
	int len = 0;
#if IS_ENABLED(CONFIG_CPU_FREQ_GOV_CONSERVATIVE)
	struct policy_dbs_info *policy_dbs = NULL;
	struct dbs_data *dbs_data = NULL;
	struct cs_dbs_tuners *cs_tuners = NULL;
#endif

	len = snprintf(node, BUFFER_SIZE, "cluster%d", 0);
	if (len > BUFFER_SIZE) {
		pr_err("[mtktv-cpufreq] node string overflow\n");
		return;
	}

	np = of_find_node_by_name(NULL, node);
	if (!np)
		pr_err("[mtktv-cpufreq] node not found\n");

	if (thermal_done == 0) {
		CPUFREQ_INFO("[mtktv-cpufreq] register cooling device\n");

		cdev = of_cpufreq_cooling_register(policy);

		if (IS_ERR(cdev))
			pr_err("[mtktv-cpufreq] fail to register cooling device\n");
		else
			CPUFREQ_INFO("[mtktv-cpufreq]\n"
				"successful to register cooling device\n");
		thermal_done = 1;
		policy->cdev = cdev;

	}

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	/* trigger boost event for ac boosting performance */
	mtktv_boost_register(E_MTKTV_LUNCH_BOOST, 0,
		E_BOOST_ENABLE_WITH_TIME, LUNCH_BOOST_TIME_MS);
#endif

#if IS_ENABLED(CONFIG_CPU_FREQ_GOV_CONSERVATIVE)
	// setup governor attribute
	if (chip_id == MIFFY_ID) {
		if (!strncmp(policy->governor->name, GOVERNOR_NAME, strlen(GOVERNOR_NAME))) {
			if (policy->governor_data) {
				policy_dbs = policy->governor_data;
				dbs_data = policy_dbs->dbs_data;
				cs_tuners = dbs_data->tuners;
				if (cs_tuners)
					cs_tuners->down_threshold = DOWN_THRESHOLD;
			}
		}
	}
#endif
}

static int mtktv_cpufreq_resume(struct cpufreq_policy *policy)
{
	struct mtktv_cpu_dvfs_info *info;

	info = mtktv_cpu_dvfs_info_lookup(policy->cpu);

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	mutex_lock(&boost_mutex);
	suspend = 0;
	mutex_unlock(&boost_mutex);

	/* trigger boost event for resume booting performance */
	mtktv_boost_register(E_MTKTV_LUNCH_BOOST, 0,
		E_BOOST_ENABLE_WITH_TIME, LUNCH_BOOST_TIME_MS);
#else
	mutex_lock(&target_mutex);
	if (deep_suspend == 1) {
		if (MTKTV_CPUFREQ_TEEC_Resume() != TA_SUCCESS) {
			pr_err("[mtktv-cpufreq]\n"
					"CPUFREQ TA Resume fail\n");
			mutex_unlock(&target_mutex);
			return 0;
		};
		deep_suspend = 0;
	}
	suspend = 0;
	mutex_unlock(&target_mutex);

	/* trigger boost event for resume booting performance */
	mtktv_boost_register(E_MTKTV_LUNCH_BOOST, 0,
		E_BOOST_ENABLE_WITH_TIME, LUNCH_BOOST_TIME_MS);
#endif
	return 0;
}

static int mtktv_cpufreq_set_target(struct cpufreq_policy *policy,
		unsigned int index)
{
	struct cpufreq_frequency_table *freq_table = policy->freq_table;
	struct mtktv_cpu_dvfs_info *info;
	struct device *cpu_dev;
	struct dev_pm_opp *opp;
	long freq_hz, old_freq_hz;
	int vproc, old_vproc, ret;
	unsigned int cpu = policy->cpu;

	ret = 0;
	if (auto_measurement)
		return 0;

	mutex_lock(&target_mutex);
	info = mtktv_cpu_dvfs_info_lookup(policy->cpu);
	if (!info) {
		pr_err("[mtktv-cpufreq] dvfs info for cpu%d is not initialized.\n",
				policy->cpu);
		mutex_unlock(&target_mutex);
		return -EINVAL;
	}

	cpu_dev = info->cpu_dev;

	old_freq_hz = mtktv_cpufreq_get_clk(cpu) * 1000;
	freq_hz = freq_table[index].frequency * 1000;

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	if (suspend == 1) {
		mutex_unlock(&target_mutex);
		return 0;
	}

	if (tee_get_supplicant_count() && !ta_init) {
		if (MTKTV_CPUFREQ_TEEC_TA_Open() != TA_SUCCESS) {
			pr_err("[mtktv-cpufreq] CPUFREQ TA Open fail\n");
			mutex_unlock(&target_mutex);
			return 0;
		}
		if (MTKTV_CPUFREQ_TEEC_Init() != TA_SUCCESS) {
			pr_err("[mtktv-cpufreq] CPUFREQ Init fail\n");
			mutex_unlock(&target_mutex);
			return 0;
		}
		ta_init = 1;
		cold_boot_finish = 1;
		wake_up_interruptible(&cold_boot_wait);
	}

	if (!ta_init) {
		mutex_unlock(&target_mutex);
		return 0;
	}
#endif

	/* get current voltage */
	opp = dev_pm_opp_find_freq_ceil(cpu_dev, &freq_hz);
	if (IS_ERR(opp)) {
		pr_err("[mtktv-cpufreq] cpu%d: failed to find OPP for %ld\n",
				cpu, freq_hz);
		mutex_unlock(&target_mutex);
		return PTR_ERR(opp);
	}
	rcu_read_lock();
	vproc = dev_pm_opp_get_voltage(opp);
	rcu_read_unlock();

	/* get target voltage in target frequency */
	opp = dev_pm_opp_find_freq_ceil(cpu_dev, &old_freq_hz);
	if (IS_ERR(opp)) {
		pr_err("[mtktv-cpufreq] cpu%d: failed to find OPP for %ld\n",
				cpu, old_freq_hz);
		mutex_unlock(&target_mutex);
		return PTR_ERR(opp);
	}
	rcu_read_lock();
	old_vproc = dev_pm_opp_get_voltage(opp);
	rcu_read_unlock();

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	if (old_freq_hz != freq_hz) {
		if (suspend == 0) {
			if (MTKTV_CPUFREQ_TEEC_Adjust(freq_hz/1000000)
				!= TA_SUCCESS) {
				pr_err("[mtktv-cpufreq] secure adjust fail\n"
						"clock: %ld MHz\n",
						freq_hz/1000000);
			} else {
				u32Current = freq_hz/1000;
			}
		}
	}
#else
	/*
	 * If the new voltage or the intermediate voltage is higher than the
	 * current voltage, scale up voltage first.
	 */
	if (old_vproc < vproc) {
		ret = mtktv_cpufreq_set_voltage(info, vproc);
		if (ret) {
			pr_err("[mtktv-cpufreq] cpu%d: failed to scale up voltage!\n",
					cpu);
			mutex_unlock(&target_mutex);
			return ret;
		}
	}

	if (old_freq_hz != freq_hz) {
		mtktv_cpufreq_set_clk(old_freq_hz/1000, freq_hz/1000);
		u32Current = freq_hz/1000;
	}

	/*
	 * If the new voltage is lower than the intermediate voltage or the
	 * original voltage, scale down to the new voltage.
	 */
	if (vproc < old_vproc) {
		ret = mtktv_cpufreq_set_voltage(info, vproc);
		if (ret) {
			pr_err("[mtktv-cpufreq] cpu%d: failed to scale down voltage!\n",
					cpu);
			mutex_unlock(&target_mutex);
			return ret;
		}
	}
#endif
	mutex_unlock(&target_mutex);
	return 0;
}

static int mtktv_cpufreq_suspend(struct cpufreq_policy *policy)
{
	struct mtktv_cpu_dvfs_info *info;
	unsigned int cpu = policy->cpu;

	mutex_lock(&boost_mutex);

	/* cancel all possible kwork */
	cancel_delayed_work(&disable_work);
	current_boost.id = E_MTKTV_BOOST_NONE;
	current_boost.time = 0;
	current_boost.rate = 0;
	suspend = 1;

	/*
	 * set cpufreq policy to default state and
	 * set frequency to default clock
	 */
	info = mtktv_cpu_dvfs_info_lookup(cpu);
	if (info == NULL) {
		pr_err("[mtktv-cpufreq] dvfs info for cpu%d is not initialized.\n",
				cpu);
		pr_err("[mtktv-cpufreq] update policy fail in suspend.\n");
		return 0;
	}
	freq_qos_update_request(policy->min_freq_req, info->default_min);
	freq_qos_update_request(policy->max_freq_req, info->default_max);
	cpufreq_update_policy(cpu);
	mtktv_cpufreq_set_target(policy, 1);

	regulator_init = 0;
#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	regulator_force_disable(info->reg);
#endif
	mutex_unlock(&boost_mutex);
	return 0;
}

static int mtktv_cpu_dvfs_info_init(struct mtktv_cpu_dvfs_info *info, int cpu)
{
	struct device *cpu_dev;
	struct dev_pm_opp *opp;
	struct regulator *reg = NULL;
	unsigned long rate;
	int ret;

	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		pr_err("[mtktv-cpufreq] failed to get cpu%d device\n", cpu);
		return -ENODEV;
	}

	reg = regulator_get(NULL, "mtk-cpu-regulator");
	if (IS_ERR(reg)) {
		if (PTR_ERR(reg) == -EPROBE_DEFER)
			pr_err("[mtktv-cpufreq] proc regulator\n"
					"for cpu%d not ready\n"
					"retry.\n", cpu);
		else
			pr_err("[mtktv-cpufreq] failed to\n"
					"get proc regulator\n"
					"for cpu%d\n", cpu);
		ret = PTR_ERR(reg);
		goto out_free_resources;
	}
	regulator_set_mode(reg, REGULATOR_MODE_FAST);
	info->reg = reg;

	/*
	 * Get OPP-sharing information from
	 * "operating-points-v2" bindings
	 */
	ret = dev_pm_opp_of_get_sharing_cpus(cpu_dev, &info->cpus);
	if (ret) {
		pr_err("[mtktv-cpufreq] failed to\n"
			"get OPP-sharing information\n"
			"for cpu%d\n", cpu);
		goto out_free_resources;
	}

	ret = dev_pm_opp_of_cpumask_add_table(&info->cpus);
	if (ret) {
		pr_err("[mtktv-cpufreq] no OPP table for cpu%d\n", cpu);
		goto out_free_resources;
	}

	rate = mtktv_cpufreq_get_clk(cpu)*1000;
	opp = dev_pm_opp_find_freq_ceil(cpu_dev, &rate);
	if (IS_ERR(opp)) {
		pr_err("[mtktv-cpufreq] failed to get intermediate opp\n"
			"for cpu%d\n", cpu);
		ret = PTR_ERR(opp);
		goto out_free_opp_table;
	}

	info->cpu_dev = cpu_dev;
	return 0;

out_free_opp_table:
	dev_pm_opp_of_cpumask_remove_table(&info->cpus);

out_free_resources:
	if (!IS_ERR(reg))
		regulator_put(reg);
	return ret;
}

static void mtktv_cpu_dvfs_info_release(struct mtktv_cpu_dvfs_info *info)
{
	if (!IS_ERR(info->reg))
		regulator_put(info->reg);
	dev_pm_opp_of_cpumask_remove_table(&info->cpus);
}

static struct cpufreq_driver mtktv_cpufreq_driver = {
	.verify         = cpufreq_generic_frequency_table_verify,
	.target_index   = mtktv_cpufreq_set_target,
	.get            = mtktv_cpufreq_get_clk,
	.init           = mtktv_cpufreq_init,
	.exit           = mtktv_cpufreq_exit,
	.ready          = mtktv_cpufreq_ready,
	.resume         = mtktv_cpufreq_resume,
	.suspend        = mtktv_cpufreq_suspend,
	.name           = "mtktv-cpufreq",
	.attr           = cpufreq_generic_attr,
};

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static int cpufreq_driver_suspend_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
	{
		mutex_lock(&target_mutex);
		pr_info("[%s][%d]: PM_SUSPEND_PREPARE\n",
			__func__, __LINE__);
		if (MTKTV_CPUFREQ_TEEC_Suspend() != TA_SUCCESS) {
			pr_err("[mtktv-cpufreq]\n"
				"CPUFREQ TA Suspend fail\n");
			mutex_unlock(&target_mutex);
			return NOTIFY_BAD;
		};
		suspend = 1;
		u32Current = suspend_frequency;
		pr_info("[%s][%d] Suspend Clock %d KHz\n",
			__func__, __LINE__, u32Current);
		mutex_unlock(&target_mutex);
		break;
	}
	default:
		break;
	}
	return NOTIFY_DONE;
}
#endif

static int mtktv_cpufreq_suspend_noirq(struct device *dev)
{
	deep_suspend = 1;
	return 0;
}

static int mtktv_cpufreq_probe(struct platform_device *pdev)
{
	struct mtktv_cpu_dvfs_info *info, *tmp;
	int cpu, ret;

	for_each_possible_cpu(cpu) {
		info = mtktv_cpu_dvfs_info_lookup(cpu);
		if (info)
			continue;

		info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
		if (!info) {
			ret = -ENOMEM;
			goto release_dvfs_info_list;
		}

		//info->cpu_dev = &pdev->dev;
		ret = mtktv_cpu_dvfs_info_init(info, cpu);
		if (ret) {
			dev_err(&pdev->dev,
			"[mtktv-cpufreq] failed to initialize dvfs info\n"
			"for cpu%d\n", cpu);
			goto release_dvfs_info_list;
		}

		list_add(&info->list_head, &dvfs_info_list);
	}

	INIT_DELAYED_WORK(&disable_work, mtktv_disable_boost);
	ret = mtktv_cpufreq_opp_init();
	if (ret) {
		dev_err(&pdev->dev, "[mtktv-cpufreq] failed to mtktv_cpufreq_opp_init\n");
		goto release_dvfs_info_list;
	}

	mtktv_boost_init();
	input_boost_init();
	ret = cpufreq_register_driver(&mtktv_cpufreq_driver);
	if (ret) {
		dev_err(&pdev->dev, "[mtktv-cpufreq] failed to register\n"
				"mtk cpufreq driver\n");
		goto release_dvfs_info_list;
	}

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	/* need to do suspend handler before optee suspend */
	cpufreq_pm_suspend_notifier.notifier_call =
		cpufreq_driver_suspend_event;
	cpufreq_pm_suspend_notifier.priority = 99;
	register_pm_notifier(&cpufreq_pm_suspend_notifier);

	lunch_boost_task = kthread_create(lunch_boost_thread,
		NULL, "Lunch_Boost_Check");
	wake_up_process(lunch_boost_task);
#endif
	return 0;

release_dvfs_info_list:
	list_for_each_entry_safe(info, tmp, &dvfs_info_list, list_head) {
		mtktv_cpu_dvfs_info_release(info);
		list_del(&info->list_head);
	}
	return ret;
}

static const struct dev_pm_ops mtktv_cpufreq_dev_pm_ops = {
	.suspend_noirq = mtktv_cpufreq_suspend_noirq,
};

static struct platform_driver mtktv_cpufreq_platdrv = {
	.driver = {
		.name   = "mtktv-cpufreq",
		.pm = &mtktv_cpufreq_dev_pm_ops,
	},
	.probe = mtktv_cpufreq_probe,
};

/* List of machines supported by this driver */
static const struct of_device_id mtktv_cpufreq_machines[] __initconst = {
	{.compatible = "arm,m7332",  },
	{.compatible = "mediatek,mt5896",  },
	{.compatible = "mediatek,mt5897",  },
	{},
};

MODULE_DEVICE_TABLE(of, mtktv_cpufreq_machines);

static int __init mtktv_cpufreq_driver_init(void)
{
	struct device_node *np;
	const struct of_device_id *match;
	struct platform_device *pdev;
	struct device_node *cpufreq_node = NULL;
	struct device_node *model = NULL;
	u32 pm_base[2];
	int err;

	model = of_find_node_by_path("/mtktv_cpufreq");
	if (model == NULL) {
		pr_err("[mtktv-cpufreq]: model not found\n");
		return -EINVAL;
	}
	if (!of_property_match_string(model, "model", "MT5897")) {
		pr_info("[mtktv-cpufreq]: model MT5897\n");
		chip_id = M6L_ID;
	}
	if (!of_property_match_string(model, "model", "MT5896")) {
		pr_info("[mtktv-cpufreq]: model MT5896\n");
		chip_id = M6_ID;
	}
	if (!of_property_match_string(model, "model", "MT5876")) {
		pr_info("[mtktv-cpufreq]: model MT5876\n");
		chip_id = MOKONA_ID;
	}
	if (!of_property_match_string(model, "model", "MT5879")) {
		pr_info("[mtktv-cpufreq]: model MT5879\n");
		chip_id = MIFFY_ID;
	}
	if (!of_property_match_string(model, "model", "MT5873")) {
		pr_info("[mtktv-cpufreq]: model MT5873\n");
		chip_id = MOKA_ID;
	}
	if (chip_id == UNKNOWN_ID) {
		pr_err("UNKNOWN_ID chip id\n");
		of_node_put(model);
		return -EINVAL;
	}
	of_node_put(model);

	if (cpufreq_pm_base == NULL) {
		cpufreq_node = of_find_node_by_path("/mtktv_cpufreq");
		if (cpufreq_node == NULL) {
			pr_err("dtsi node: mtktv-cpufreq not found\n");
			return -EINVAL;
		}
		if (!of_property_match_string(cpufreq_node, "status", "disabled")) {
			pr_err("[mtktv-cpufreq]: status disabled\n");
			of_node_put(cpufreq_node);
			return -EINVAL;
		}
		cpufreq_pm_base = of_iomap(cpufreq_node, 0);
		cpufreq_pm_base2 = of_iomap(cpufreq_node, 1);
		if (!cpufreq_pm_base || !cpufreq_pm_base2) {
			pr_err("failed to map cpufreq_pm_base (%ld)\n",
					PTR_ERR(cpufreq_pm_base));
			pr_err("failed to map cpufreq_pm_base2 (%ld)\n",
					PTR_ERR(cpufreq_pm_base2));
			pr_err("node:%s reg base error\n", cpufreq_node->name);
			of_node_put(cpufreq_node);
			return -EINVAL;
		}
		of_property_read_u32_array(cpufreq_node, "reg", pm_base, 2);
	}
	of_node_put(cpufreq_node);

	np = of_find_node_by_path("/");
	if (!np) {
		pr_err("%s device tree node\n", __func__);
		return -ENODEV;
	}
	match = of_match_node(mtktv_cpufreq_machines, np);
	of_node_put(np);
	if (!match) {
		pr_err("[mtktv-cpufreq] Machine is not support dvfs\n");
		return -ENODEV;
	}
	err = platform_driver_register(&mtktv_cpufreq_platdrv);
	if (err) {
		pr_err("%s platform_driver_register fail\n", __func__);
		return err;
	}
	/*
	 * Since there's no place to hold device registration code and no
	 * device tree based way to match cpufreq driver yet, both the driver
	 * and the device registration codes are put here to handle defer
	 * probing.
	 */
	pdev = platform_device_register_simple("mtktv-cpufreq", -1, NULL, 0);
	if (IS_ERR(pdev)) {
		pr_err("[mtktv-cpufreq] failed to register\n"
			"mtk-cpufreq platform device\n");
		return PTR_ERR(pdev);
	}

	return 0;
}

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
//REE DVFS SET CLK
static void MTK_CLK_CORE_Adjust(unsigned int cur, unsigned int target)
{
	unsigned int u32ValueLpfSet = 0;

	cpufreq_writeb(0x04, BANK_CLK_SET_MUX+REG_04);
	if (target < 500)
		cpufreq_writeb(0x22, BANK_CLK_SET_CORE+REG_0E);

	if((chip_id == M6_ID) || (chip_id == M6L_ID)) {
		///start_freq
		u32ValueLpfSet = ((5435817UL*NUM_1000)/cur);

	cpufreq_writew(((u32ValueLpfSet&0x00FFFF)), BANK_CLK_SET_CORE+REG_40);
		cpufreq_writeb(((u32ValueLpfSet&0xFF0000)>>NUM_16), BANK_CLK_SET_CORE+REG_42);

	//end_freq
		u32ValueLpfSet = ((5435817UL*NUM_1000)/target);
	}

	if ((chip_id == MOKONA_ID) || (chip_id == MIFFY_ID) || (chip_id == MOKA_ID)) {
		//start_freq
		u32ValueLpfSet = ((3623878UL*NUM_1000)/cur);

		cpufreq_writew(((u32ValueLpfSet&0x00FFFF)), BANK_CLK_SET_CORE+REG_40);
		cpufreq_writeb(((u32ValueLpfSet&0xFF0000)>>NUM_16), BANK_CLK_SET_CORE+REG_42);

		//end_freq
		u32ValueLpfSet = ((3623878UL*NUM_1000)/target);
	}

	cpufreq_writew(((u32ValueLpfSet&0x00FFFF)), BANK_CLK_SET_CORE+REG_44);
	cpufreq_writeb(((u32ValueLpfSet&0xFF0000)>>16), BANK_CLK_SET_CORE+REG_46);

	cpufreq_writeb(0x06, BANK_CLK_SET_CORE+REG_4A);
	cpufreq_writeb(0x08, BANK_CLK_SET_CORE+REG_4E);
	cpufreq_writew(0x0101, BANK_CLK_SET_CORE+REG_50);

	cpufreq_writew(0x1330, BANK_CLK_SET_CORE+REG_52);
	cpufreq_writeb(0x01, BANK_CLK_SET_CORE+REG_48);

	while ((cpufreq_readb(BANK_CLK_SET_CORE+REG_5A) & 0x1) == 0)
		;

	cpufreq_writeb(0x00, BANK_CLK_SET_CORE+REG_48);
}

static void MTK_CLK_DSU_Adjust(unsigned int cur, unsigned int target)
{
	unsigned int u32ValueLpfSet = 0;

	cpufreq_writeb(0x04, BANK_CLK_SET_MUX+REG_0C);
	if (target < 500)
		cpufreq_writeb(0x22, BANK_CLK_SET_DSU+REG_0E);
	//start_freq
	u32ValueLpfSet = ((5435817UL*1000)/cur);

	cpufreq_writew(((u32ValueLpfSet&0x00FFFF)), BANK_CLK_SET_DSU+REG_40);
	cpufreq_writeb(((u32ValueLpfSet&0xFF0000)>>16), BANK_CLK_SET_DSU+REG_42);

	//end_freq
	u32ValueLpfSet = ((5435817UL*1000)/target);

	cpufreq_writew(((u32ValueLpfSet&0x00FFFF)), BANK_CLK_SET_DSU+REG_44);
	cpufreq_writeb(((u32ValueLpfSet&0xFF0000)>>16), BANK_CLK_SET_DSU+REG_46);

	cpufreq_writeb(0x06, BANK_CLK_SET_DSU+REG_4A);
	cpufreq_writeb(0x08, BANK_CLK_SET_DSU+REG_4E);
	cpufreq_writew(0x0101, BANK_CLK_SET_DSU+REG_50);

	cpufreq_writew(0x1330, BANK_CLK_SET_DSU+REG_52);
	cpufreq_writeb(0x01, BANK_CLK_SET_DSU+REG_48);

	while ((cpufreq_readb(BANK_CLK_SET_DSU+REG_5A) & 0x1) == 0)
		;

	cpufreq_writeb(0x00, BANK_CLK_SET_DSU+REG_48);
}
#endif

#if IS_BUILTIN(CONFIG_ARM_MTKTV_CPUFREQ)
static int __init auto_test(char *str)
{
	if (strcmp(str, "enable") == 0) {
		pr_info("\033[32m auto_measurement enable \033[0m\n");
		auto_measurement = 1;
	} else {
		auto_measurement = 0;
	}
	return 0;
}

static int __init cpufreq_debug(char *str)
{
	if (strcmp(str, "1") == 0) {
		pr_info("\033[32m cpufreq debug enable \033[0m\n");
		debug_enable = 1;
	} else {
		debug_enable = 0;
	}
	return 0;
}

static int __init cpufreq_info(char *str)
{
	if (strcmp(str, "1") == 0) {
		pr_info("\033[32m cpufreq info enable \033[0m\n");
		info_enable = 1;
	} else {
		info_enable = 0;
	}
	return 0;
}

static int __init cpufreq_tracer(char *str)
{
	if (strcmp(str, "1") == 0) {
		pr_info("\033[32m cpufreq tracer enable \033[0m\n");
		tracer_enable = 1;
	} else {
		tracer_enable = 0;
	}
	return 0;
}
#endif

#if IS_BUILTIN(CONFIG_ARM_MTKTV_CPUFREQ)
early_param("DVFS_MEASURE", auto_test);
early_param("CPUFREQ_DEBUG", cpufreq_debug);
early_param("CPUFREQ_INFO", cpufreq_info);
early_param("CPUFREQ_TRACER", cpufreq_tracer);
#else
module_param(auto_measurement, int, 0644);
MODULE_PARM_DESC(auto_measurement, "DVFS_MEASURE");
module_param(debug_enable, int, 0644);
MODULE_PARM_DESC(debug_enable, "CPUFREQ_DEBUG");
module_param(info_enable, int, 0644);
MODULE_PARM_DESC(info_enable, "CPUFREQ_INFO");
module_param(tracer_enable, int, 0644);
MODULE_PARM_DESC(tracer_enable, "CPUFREQ_TRACER");
#endif

late_initcall(mtktv_cpufreq_driver_init);

MODULE_DESCRIPTION("MediaTek TV CPUFreq driver");
MODULE_AUTHOR("MediaTek Inc.");
MODULE_LICENSE("GPL");
