/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MDLA_DEBUG_H__
#define __MDLA_DEBUG_H__

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include <linux/device.h>
#include <common/mdla_device.h>

#define MDLA_TAG "[MDLA]"

extern u8 cfg_apusys_trace;

/* MDLA reset reason */
enum REASON_MDLA_RETVAL_ENUM {
	REASON_OTHERS    = 0,
	REASON_DRVINIT   = 1,
	REASON_TIMEOUT   = 2,
	REASON_POWERON   = 3,
	REASON_PREEMPTED = 4,
	REASON_SIMULATOR = 5,
	REASON_MAX,
};

enum MDLA_DEBUG_FS_NODE_U64 {
	FS_CFG_PMU_PERIOD,

	NF_MDLA_DEBUG_FS_U64
};

enum MDLA_DEBUG_FS_NODE_U32 {
	FS_C1,
	FS_C2,
	FS_C3,
	FS_C4,
	FS_C5,
	FS_C6,
	FS_C7,
	FS_C8,
	FS_C9,
	FS_C10,
	FS_C11,
	FS_C12,
	FS_C13,
	FS_C14,
	FS_C15,
	FS_CFG_ENG0,
	FS_CFG_ENG1,
	FS_CFG_ENG2,
	FS_CFG_ENG11,
	FS_POLLING_CMD_DONE,
	FS_DUMP_CMDBUF,
	FS_DVFS_RAND,
	FS_PMU_EVT_BY_APU,
	FS_KLOG,
	FS_POWEROFF_TIME,
	FS_TIMEOUT,
	FS_TIMEOUT_DBG,
	FS_BATCH_NUM,
	FS_PREEMPTION_TIMES,
	FS_PREEMPTION_DBG,
	FS_SECURE_DRV_EN,

	NF_MDLA_DEBUG_FS_U32
};

struct mdla_dbg_cb_func {
	void (*destroy_dump_cmdbuf)(struct mdla_dev *mdla_device);
	int  (*create_dump_cmdbuf)(struct mdla_dev *mdla_device,
			struct command_entry *ce);
	void (*dump_reg)(u32 core_id, struct seq_file *s);
	void (*memory_show)(struct seq_file *s);

	bool (*dbgfs_u64_enable)(int node);
	bool (*dbgfs_u32_enable)(int node);
	void (*dbgfs_plat_init)(struct device *dev, struct dentry *parent);
};

struct mdla_dbg_cb_func *mdla_dbg_plat_cb(void);

u64 mdla_dbg_read_u64(int node);
u32 mdla_dbg_read_u32(int node);

void mdla_dbg_write_u64(int node, u64 val);
void mdla_dbg_write_u32(int node, u32 val);

void mdla_dbg_sub_u64(int node, u64 val);
void mdla_dbg_sub_u32(int node, u32 val);

void mdla_dbg_add_u64(int node, u64 val);
void mdla_dbg_add_u32(int node, u32 val);

enum MDLA_DEBUG_MASK {
	MDLA_DBG_DRV         = (1U << 0),
	MDLA_DBG_MEM         = (1U << 1),
	MDLA_DBG_CMD         = (1U << 2),
	MDLA_DBG_PMU         = (1U << 3),
	MDLA_DBG_PERF        = (1U << 4),
	MDLA_DBG_QOS         = (1U << 5),
	MDLA_DBG_TIMEOUT	 = (1U << 6),
	MDLA_DBG_DVFS        = (1U << 7),
	MDLA_DBG_TIMEOUT_ALL = (1U << 8),

	MDLA_DBG_ALL         = (1U << 9) - 1,
};

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>
#define mdla_aee_warn(key, format, args...) \
	do { \
		pr_info(format, ##args); \
		aee_kernel_warning("MDLA", \
			"\nCRDISPATCH_KEY:" key "\n" format, ##args); \
	} while (0)
#else
#define mdla_aee_warn(key, format, args...)
#endif


#define redirect_output(...) pr_info(MDLA_TAG "[INFO]" __VA_ARGS__)

#define mdla_err(format, args...) pr_err(MDLA_TAG "[Err]" format, ##args)

#define mdla_debug(mask, ...)				\
do {							\
	if ((mask & mdla_dbg_read_u32(FS_KLOG)))	\
		redirect_output(__VA_ARGS__);		\
} while (0)

#define mdla_drv_debug(...) mdla_debug(MDLA_DBG_DRV, __VA_ARGS__)
#define mdla_mem_debug(...) mdla_debug(MDLA_DBG_MEM, __VA_ARGS__)
#define mdla_cmd_debug(...) mdla_debug(MDLA_DBG_CMD, __VA_ARGS__)
#define mdla_pmu_debug(...) mdla_debug(MDLA_DBG_PMU, __VA_ARGS__)
#define mdla_perf_debug(...) mdla_debug(MDLA_DBG_PERF, __VA_ARGS__)
#define mdla_qos_debug(...) mdla_debug(MDLA_DBG_QOS, __VA_ARGS__)
#define mdla_timeout_debug(...) mdla_debug(MDLA_DBG_TIMEOUT, __VA_ARGS__)
#define mdla_dvfs_debug(...) mdla_debug(MDLA_DBG_DVFS, __VA_ARGS__)
#define mdla_timeout_all_debug(...) \
	mdla_debug(MDLA_DBG_TIMEOUT_ALL, __VA_ARGS__)

const char *mdla_dbg_get_reason_str(int res);

//void mdla_dbg_dump_cmdbuf_free(struct mdla_dev *mdla_device);
void mdla_dbg_dump(struct mdla_dev *mdla_info, struct command_entry *ce);

struct dentry *mdla_dbg_get_fs_root(void);

void mdla_dbg_fs_setup(struct device *dev);
void mdla_dbg_fs_init(struct dentry *apusys_dbg_root);
void mdla_dbg_fs_exit(void);

#endif /* __MDLA_DEBUG_H__ */
