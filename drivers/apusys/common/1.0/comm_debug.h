/* SPDX-License-Identifier: GPL-2.0*/
#ifndef __APUSYS_COMM_DEBUG_H__
#define __APUSYS_COMM_DEBUG_H__

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/seq_file.h>

/* LOG & AEE */
#define COMM_TAG "[MDLA][COMM]"
/*#define VPU_DEBUG*/
#define LOG_DBG(format, args...)
#define LOG_INF(format, args...)    pr_info(COMM_TAG " " format, ##args)
#define LOG_WRN(format, args...)    pr_info(COMM_TAG "[warn] " format, ##args)
#define LOG_ERR(format, args...)    pr_info(COMM_TAG "[error] " format, ##args)

enum COMM_DEBUG_MASK {
	COMM_DBG_DRV = 0x01,
	COMM_DBG_MEM = 0x02,
};

#define comm_err(format, args...) pr_err(COMM_TAG "[error]" format, ##args)
extern u32 comm_klog;
#define comm_debug(mask, fmt, ...) do { \
		if (comm_klog & mask) \
			pr_info(COMM_TAG " " fmt, ##__VA_ARGS__); \
	} while (0)

#define comm_drv_debug(...) comm_debug(COMM_DBG_DRV, __VA_ARGS__)
#define comm_mem_debug(...) comm_debug(COMM_DBG_MEM, __VA_ARGS__)

int comm_dbg_init(void);
void comm_dbg_exit(void);
#endif

