/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MDLA_CMD_V2_0_H__
#define __MDLA_CMD_V2_0_H__

#include <linux/types.h>

struct mdla_wait_cmd {
	uint32_t id;           /* [in] command id */
	int32_t  result;       /* [out] success(0), timeout(1) */
	uint64_t queue_time;   /* [out] time queued in driver (ns) */
	uint64_t busy_time;    /* [out] mdla execution time (ns) */
	uint32_t bandwidth;    /* [out] mdla bandwidth */
};

struct mdla_run_cmd {
	uint32_t offset_code_buf;
	uint32_t reserved;
	uint32_t size;
	uint32_t mva;
	uint32_t offset;        /* [in] command byte offset in buf */
	uint32_t count;         /* [in] # of commands */
	uint32_t id;            /* [out] command id */
};

struct mdla_run_cmd_sync {
	struct mdla_run_cmd req;
	struct mdla_wait_cmd res;
	uint32_t mdla_id;
};

struct mdla_wait_entry {
	uint32_t async_id;
	struct list_head list;
	struct mdla_wait_cmd wt;
};

struct mdla_ut_run_cmd {
	uint64_t kva; // Virtual Address for Kernel
	uint32_t mva; // Physical Address for Device
	uint32_t count;
	uint32_t id;
	uint32_t mva_h; // Physical Address for Device
};

struct mdla_ut_run_cmd_sync {
	struct mdla_ut_run_cmd req;
	struct mdla_wait_cmd res;
};

#define MAX_SVP_BUF_PER_CMD 0x10
struct mdla_run_cmd_svp {
	uint32_t secure;
	uint32_t mdla_id;
	uint32_t pipe_id;

	struct {
		uint32_t size;
		uint32_t iova;
		uint32_t buf_type;
		uint32_t buf_source;
	} buf[MAX_SVP_BUF_PER_CMD];

	uint32_t buf_count;     /* [in] # of svp buffers */
	uint32_t cmd_count;     /* [in] # of commands */
	uint32_t model;
	uint32_t timeout;       /* [in] command timeout (us) */
	uint8_t priority;       /* [in] dvfs priority */
	uint8_t boost_value;    /* [in] dvfs boost value */
};

struct mdla_run_cmd_sync_svp {
	struct mdla_run_cmd_svp req;
	struct mdla_wait_cmd res;
};

#endif /* __MDLA_CMD_V2_0_H__ */

