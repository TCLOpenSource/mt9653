/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MDLA_IOCTL_H__
#define __MDLA_IOCTL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <linux/ioctl.h>
#include <linux/types.h>

enum mem_type {
	MEM_DRAM,
	MEM_IOMMU,
	MEM_GSM,
	MEM_DRAM_CACHED,
	MEM_IOMMU_CACHED,
	MEM_LXMEM	//FPGA MODE
};

enum MDLA_PMU_INTERFACE {
	MDLA_PMU_IF_WDEC0 = 0xe,
	MDLA_PMU_IF_WDEC1 = 0xf,
	MDLA_PMU_IF_CBLD0 = 0x10,
	MDLA_PMU_IF_CBLD1 = 0x11,
	MDLA_PMU_IF_SBLD0 = 0x12,
	MDLA_PMU_IF_SBLD1 = 0x13,
	MDLA_PMU_IF_STE0 = 0x14,
	MDLA_PMU_IF_STE1 = 0x15,
	MDLA_PMU_IF_CMDE = 0x16,
	MDLA_PMU_IF_DDE = 0x17,
	MDLA_PMU_IF_CONV = 0x18,
	MDLA_PMU_IF_RQU = 0x19,
	MDLA_PMU_IF_POOLING = 0x1a,
	MDLA_PMU_IF_EWE = 0x1b,
	MDLA_PMU_IF_CFLD = 0x1c
};

enum MDLA_PMU_DDE_EVENT {
	MDLA_PMU_DDE_WORK_CYC = 0x0,
	MDLA_PMU_DDE_TILE_DONE_CNT,
	MDLA_PMU_DDE_EFF_WORK_CYC,
	MDLA_PMU_DDE_BLOCK_CNT,
	MDLA_PMU_DDE_READ_CB_WT_CNT,
	MDLA_PMU_DDE_READ_CB_ACT_CNT,
	MDLA_PMU_DDE_WAIT_CB_TOKEN_CNT,
	MDLA_PMU_DDE_WAIT_CONV_RDY_CNT,
	MDLA_PMU_DDE_WAIT_CB_FCWT_CNT,
};

enum MDLA_PMU_MODE {
	MDLA_PMU_ACC_MODE = 0x0,
	MDLA_PMU_INTERVAL_MODE = 0x1,
};

struct ioctl_malloc {
	u32 size;
	u32 mva; // Physical Address for Device
	u64 pa;  // Physical Address for CPU
	u64 kva; // Virtual Address for Kernel
	u8 type;
	u64 data; // Virtual Address for User
	u32 mva_h; // Physical Address for Device
};

enum MALLOC_INFO_GET_TYPE {
	MALLOC_INFO_BY_PA = 0,
	MALLOC_INFO_BY_FD = 1,
};

struct ioctl_malloc_info {
	u32 find_type;
	u32 size;
	u64 kva; // Virtual Address for Kernel
	u64 mva;  // Physical Address for Device
	u64 pa;  // Physical Address for CPU
	u64 pid;
	u64 fd;
};

#ifndef MDLA_IOC_CAI_API
struct ioctl_run_cmd {
	uint64_t kva; // Virtual Address for Kernel
	uint32_t mva; // Physical Address for Device
	uint32_t count;
	uint32_t id;
	uint32_t mva_h; // Physical Address for Device
};

#else
struct ioctl_run_cmd {
	struct {
		uint32_t size;
		uint32_t mva;
		void *pa;
		void *kva;
		uint32_t id;
		uint8_t type;
		void *data;

		int32_t ion_share_fd;
		int32_t ion_handle;   /* user space handle */
		uint64_t ion_khandle; /* kernel space handle */
	} buf;

	uint32_t offset;        /* [in] command byte offset in buf */
	uint32_t count;         /* [in] # of commands */
	uint32_t id;            /* [out] command id */
	uint8_t priority;       /* [in] dvfs priority */
	uint8_t boost_value;    /* [in] dvfs boost value */
};
#endif

struct ioctl_perf {
	int handle;
	__u32 interface;
	__u32 event;
	__u32 counter;
	__u32 start;
	__u32 end;
	__u32 mode;
	uint32_t mdlaid;
};

enum MDLA_CONFIG {
	MDLA_CFG_NONE = 0,
	MDLA_CFG_TIMEOUT_GET = 1,
	MDLA_CFG_TIMEOUT_SET = 2,
	MDLA_CFG_FIFO_SZ_GET = 3,
	MDLA_CFG_FIFO_SZ_SET = 4,
	MDLA_CFG_GSM_INFO = 5,
	MDLA_CFG_EFUSE_INFO = 6,
	MDLA_CFG_POWER_CTL = 7,
	MDLA_CFG_KTHREAD_ATTR_GET = 8,
	MDLA_CFG_KTHREAD_ATTR_SET = 9,
	MDLA_CFG_PWR_STAT_GET = 10,
	MDLA_CFG_PWR_STAT_SET = 11
};

#define MDLA_APU_PWR_MSK	0x01
#define MDLA_GSM_PWR_MSK	0x10

enum MDLA_POWER_CTL_OP {
	MDLA_APU_OFF_GSM_OFF = 0,
	MDLA_APU_OFF_GSM_ON = (MDLA_GSM_PWR_MSK),
	MDLA_APU_ON_GSM_OFF = (MDLA_APU_PWR_MSK),
	MDLA_APU_ON_GSM_ON = (MDLA_APU_PWR_MSK | MDLA_GSM_PWR_MSK)
};

struct ioctl_config {
	__u32 op;
	__u32 arg_count;
	__u64 arg[8];
};

enum MDLA_CMD_RESULT {
	MDLA_CMD_SUCCESS = 0,
	MDLA_CMD_TIMEOUT = 1,
};

#define MDLA_IOC_MAGIC (0x3d1a632fULL)

#define MDLA_IOC_SET_ARRAY_CNT(n) \
	((MDLA_IOC_MAGIC << 32) | ((n) & 0xFFFFFFFF))
#define MDLA_IOC_GET_ARRAY_CNT(n) \
	(((n >> 32) == MDLA_IOC_MAGIC) ? ((n) & 0xFFFFFFFF) : 0)
#define MDLA_IOC_SET_ARRAY_PTR(a) \
	((unsigned long)(a))
#define MDLA_IOC_GET_ARRAY_PTR(a) \
	((void *)((unsigned long)(a)))

#define MDLA_WAIT_CMD_ARRAY_SIZE 6

struct ioctl_wait_cmd {
	u32 id;              /* [in] command id */
	int  result;           /* [out] success(0), timeout(1) */
	u64 queue_time;   /* [out] time queued in driver (ns) */
	u64 busy_time;    /* [out] mdla execution time (ns) */
	u32 bandwidth;    /* [out] mdla bandwidth */
};

struct ioctl_malloc_svp {
	uint32_t size;
	uint32_t align;
	int fd;
	uint64_t pa;
	uint64_t kva;
	uint32_t mva;
	uint64_t data;
	uint32_t mem_type;
	uint32_t secure;
	uint32_t pipe_id;
};

enum MDLA_SVP_BUF_TYPE {
	EN_MDLA_BUF_TYPE_INPUT = 1,       // image buffer
	EN_MDLA_BUF_TYPE_OUTPUT,          // output result buffer
	EN_MDLA_BUF_TYPE_TEMP,            // activation buffer for MDLA
	EN_MDLA_BUF_TYPE_STATIC,          // weight, quantization buffer for MDLA
	EN_MDLA_BUF_TYPE_CODE,            // command buffer for MDLA
	EN_MDLA_BUF_TYPE_CONFIG           // config buffer for AIE
};

#define MAX_SVP_BUF_PER_CMD 0x10
struct mdla_svp_buf {
	uint32_t size;
	uint32_t iova;
	uint32_t buf_type;
	uint32_t buf_source;
};

struct ioctl_run_cmd_svp {
	uint32_t secure;
	uint32_t mdla_id;
	uint32_t pipe_id;
	struct mdla_svp_buf buf[MAX_SVP_BUF_PER_CMD];
	uint32_t buf_count;     /* [in] # of svp buffers */
	uint32_t cmd_count;     /* [in] # of commands */
	uint32_t model;
	uint32_t timeout;       /* [in] command timeout (us) */
	uint8_t priority;       /* [in] dvfs priority */
	uint8_t boost_value;    /* [in] dvfs boost value */
};

struct ioctl_run_cmd_sync_svp {
	struct ioctl_run_cmd_svp req;
	struct ioctl_wait_cmd res;
};

struct ioctl_run_cmd_sync {
	struct ioctl_run_cmd req;
	struct ioctl_wait_cmd res;
};

#define IOC_MDLA ('\x1d')

#define IOCTL_MALLOC		_IOWR(IOC_MDLA, 0, struct ioctl_malloc)
#define IOCTL_FREE		_IOWR(IOC_MDLA, 1, struct ioctl_malloc)
#define IOCTL_RUN_CMD_SYNC	_IOWR(IOC_MDLA, 2, struct ioctl_run_cmd)
#define IOCTL_RUN_CMD_ASYNC	_IOWR(IOC_MDLA, 3, struct ioctl_run_cmd)
#define IOCTL_WAIT_CMD		_IOWR(IOC_MDLA, 4, struct ioctl_run_cmd)
#define IOCTL_PERF_SET_EVENT	_IOWR(IOC_MDLA, 5, struct ioctl_perf)
#define IOCTL_PERF_GET_EVENT	_IOWR(IOC_MDLA, 6, struct ioctl_perf)
#define IOCTL_PERF_GET_CNT	_IOWR(IOC_MDLA, 7, struct ioctl_perf)
#define IOCTL_PERF_UNSET_EVENT	_IOWR(IOC_MDLA, 8, struct ioctl_perf)
#define IOCTL_PERF_GET_START	_IOWR(IOC_MDLA, 9, struct ioctl_perf)
#define IOCTL_PERF_GET_END	_IOWR(IOC_MDLA, 10, struct ioctl_perf)
#define IOCTL_PERF_GET_CYCLE	_IOWR(IOC_MDLA, 11, struct ioctl_perf)
#define IOCTL_PERF_RESET_CNT	_IOWR(IOC_MDLA, 12, struct ioctl_perf)
#define IOCTL_PERF_RESET_CYCLE	_IOWR(IOC_MDLA, 13, struct ioctl_perf)
#define IOCTL_PERF_SET_MODE	_IOWR(IOC_MDLA, 14, struct ioctl_perf)
#define IOCTL_BUF_SYNC		_IOWR(IOC_MDLA, 15, struct ioctl_buf_sync)
#define IOCTL_ABORT_CMD		_IOWR(IOC_MDLA, 16, struct ioctl_run_cmd)

#define IOCTL_MALLOC_INFO_GET	_IOWR(IOC_MDLA, 20, struct ioctl_malloc_info)
#define IOCTL_MALLOC_SVP	_IOWR(IOC_MDLA, 21, struct ioctl_malloc_svp)
#define IOCTL_FREE_SVP		_IOWR(IOC_MDLA, 22, struct ioctl_malloc_svp)
#define IOCTL_RUN_CMD_SYNC_SVP	_IOWR(IOC_MDLA, 23, struct ioctl_run_cmd_sync_svp)
#define IOCTL_CONFIG		_IOWR(IOC_MDLA, 64, struct ioctl_config)

extern void mdla_ioctl_register_perf_handle(int (*pmu_ioctl)(struct file *filp,
						unsigned int command,
						unsigned long arg,
						bool need_pwr_on));
extern void mdla_ioctl_unregister_perf_handle(void);

const struct file_operations *mdla_fops_get(void);
void mdla_ioctl_mmap_setup(int (*mmap)(struct file *filp, struct vm_area_struct *vma));

extern long mdla_kernel_ioctl(unsigned int command, void *arg);

void mdla_ioctl_set_efuse(u32 efuse);
#ifdef __cplusplus
}
#endif

#endif
