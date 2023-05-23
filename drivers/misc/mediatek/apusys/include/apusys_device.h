/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __APUSYS_DEVICE_H__
#define __APUSYS_DEVICE_H__

#include <linux/dma-buf.h>

/* device type */
enum {
	APUSYS_DEVICE_NONE,

	APUSYS_DEVICE_SAMPLE,
	APUSYS_DEVICE_MDLA,
	APUSYS_DEVICE_VPU,
	APUSYS_DEVICE_EDMA,
	APUSYS_DEVICE_WAIT,// subgraph mean wait event
	APUSYS_DEVICE_RT = 32,
	APUSYS_DEVICE_SAMPLE_RT,
	APUSYS_DEVICE_MDLA_RT,
	APUSYS_DEVICE_VPU_RT,
	APUSYS_DEVICE_LAST,

	APUSYS_DEVICE_MAX = 64, //total support 64 different devices
};


/* device cmd type */
enum {
	APUSYS_CMD_POWERON,
	APUSYS_CMD_POWERDOWN,
	APUSYS_CMD_RESUME,
	APUSYS_CMD_SUSPEND,
	APUSYS_CMD_EXECUTE,
	APUSYS_CMD_PREEMPT,

	APUSYS_CMD_FIRMWARE, // setup firmware
	APUSYS_CMD_USER, //user defined

	APUSYS_CMD_MAX,
};

/* device preempt type */
enum {
	// device don't support preemption
	APUSYS_PREEMPT_NONE,
	// midware should resend preempted command after preemption call
	APUSYS_PREEMPT_RESEND,
	// midware don't need to resend cmd, and wait preempted cmd completed
	APUSYS_PREEMPT_WAITCOMPLETED,

	APUSYS_PREEMPT_MAX,
};

enum {
	APUSYS_FIRMWARE_UNLOAD,
	APUSYS_FIRMWARE_LOAD,
};

/* handle definition for send_cmd */
struct apusys_power_hnd {
	uint32_t opp;
	int boost_val;

	uint32_t timeout;
};

/**< apusys kernel memory */
struct apusys_kmem {
	unsigned long long uva; /**< user space virtual address (for debug) */
	unsigned long long kva; /**< mapped kernel space virtual address */
	unsigned int iova; /**< mapped kernel space virtual address */
	unsigned int size; /**< mapped kernel space virtual address */
	unsigned int iova_size; /**< iova size get from shared fd */

	unsigned int align; /**< memory alignment */
	unsigned int cache; /**< non-cached(0), cached(1) */

	int mem_type;
	/**< memory types:
	 *  APUSYS_MEM_DRAM_ION, APUSYS_MEM_DRAM_DMA, APUSYS_MEM_VLM
	 */

	int fd; /**< file descriptor shared from ION or DMA buffer */
	int property;
	/**< source of the memory:
	 *  APUSYS_MEM_PROP_ALLOC, APUSYS_MEM_PROP_IMPORT
	 */

	unsigned long long khandle; /**< [ION] kernel handle imported from fd */

	struct dma_buf *d; /**< [AOSP] DMA buffer */
	struct dma_buf_attachment *attach;
	/**< [AOSP] DMA buffer attachment used to get scatter list table */
	struct sg_table *sgt; /**< [AOSP] scatter list table used to map iova */
};

struct apusys_mdla_data {
	uint64_t pmu_kva;
	uint64_t cmd_entry;
};

/* cmd handle */
struct apusys_cmd_hnd {
	/* cmd info */
	uint64_t kva;
	uint32_t iova;
	uint32_t size;

	struct apusys_kmem *cmdbuf;

	uint64_t cmd_id;
	uint32_t subcmd_idx;
	uint8_t priority;

	uint32_t ip_time;
	int boost_val;
	int cluster_size;

	/* multicore info */
	uint32_t multicore_total; // how many cores to exec this subcmd
	uint32_t multicore_idx; // which part of subcmd

	/* mdla specific */
	uint64_t pmu_kva;
	uint64_t cmd_entry;

	/* For preemption */

	int (*context_callback)(int a, int b, uint8_t c);
	int ctx_id;
};

struct apusys_firmware_hnd {
	char name[32];
	uint32_t magic; // for user checking byself

	uint64_t kva;
	uint32_t iova;
	uint32_t size;

	int idx;

	int op;
};

struct apusys_usercmd_hnd {
	uint64_t kva;
	uint32_t iova;
	uint32_t size;
};

struct apusys_preempt_hnd {
	struct apusys_cmd_hnd *new_cmd;
	struct apusys_cmd_hnd *old_cmd; //TODO
};

/* device definition */
struct apusys_device {
	int dev_type;
	int idx;
	int preempt_type;
	uint8_t preempt_level;

	void *private; // for hw driver to record private structure

	int (*send_cmd)(int type, void *hnd, struct apusys_device *dev);
};

/*
 * export function for hardware driver
 * @apusys_register_device
 *  Description:
 *      for drvier register device to resource management,
 *      caller should allocate struct apusys_device struct byself
 *      midware can schedule task and preempt by current requests status
 *      1. user should implement 6 callback function for midware
 *  Parameters:
 *      <dev>
 *          dev_type: apusys device type, define as APUSYS_DEVICE_CMD_E
 *          private: midware don't touch this,
 *              it's for user record some private data,
 *              midware will pass to driver when send_cmd()
 *          preempt_type:
 *          preempt_level:
 *          send_cmd: 6 callback user should implement
 *              1. poweron: poweron device with specific opp
 *                  return:
 *                      success: 0
 *                   fail: error code
 *              2. poweroff: poweroff device
 *                  return:
 *                      success: 0
 *                      fail: error code
 *              3. suspend: suspend device (TODO)
 *              4. resume: resume device (TODO)
 *              5. excute: execute command with device
 *                  parameter:
 *                  reutrn:
 *                      success: cmd id
 *                      fail: 0
 *             6. preempt: preempt device and run new command
 *  return:
 *      success: 0
 *      fail: linux error code (negative value)
 */
int apusys_register_device(struct apusys_device *dev);
int apusys_unregister_device(struct apusys_device *dev);

uint64_t apusys_mem_query_kva(uint32_t iova);
uint32_t apusys_mem_query_iova(uint64_t kva);

int apusys_mem_flush(struct apusys_kmem *mem);
int apusys_mem_invalidate(struct apusys_kmem *mem);


#endif
