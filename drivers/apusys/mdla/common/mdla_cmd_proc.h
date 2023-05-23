/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MDLA_CMD_PROC_H__
#define __MDLA_CMD_PROC_H__

#include <common/mdla_device.h>

struct apusys_cmd_hnd;

typedef int (*run_sync_t)(struct mdla_run_cmd_sync *cmd_data,
				struct mdla_dev *mdla_info,
				struct apusys_cmd_hnd *apusys_hd,
				bool secure,
				bool enable_preempt);

typedef int (*ut_run_sync_t)(void *cmd_data, struct mdla_dev *mdla_info);

/* command operations */
struct mdla_cmd_ops {
	run_sync_t run_sync;
	ut_run_sync_t ut_run_sync;

	void (*lock)(struct mdla_dev *mdla_device);
	void (*unlock)(struct mdla_dev *mdla_device);
	void (*list_lock)(struct mdla_dev *mdla_device);
	void (*list_unlock)(struct mdla_dev *mdla_device);
};

const struct mdla_cmd_ops *mdla_cmd_ops_get(void);

void mdla_cmd_setup(run_sync_t sync, ut_run_sync_t ut_sync);

/* platform callback functions */
struct mdla_cmd_cb_func {
	int (*pre_cmd_handle)(u32 core_id, struct command_entry *ce);
	void (*pre_cmd_info)(u32 core_id);
	int (*process_command)(u32 core_id, struct command_entry *ce);
	int (*post_cmd_handle)(u32 core_id, struct command_entry *ce);
	void (*post_cmd_info)(u32 core_id);

	unsigned long (*get_wait_time)(u32 core_id);
	int (*get_irq_num)(u32 core_id);

	/* HW error handing */
	int (*wait_cmd_hw_detect)(u32 core_id);
	int (*post_cmd_hw_detect)(u32 core_id);

	/* Debug mdla timing callback */
	int (*add_time_stamp)(u32 core_id, struct command_entry *ce, const char *str);
	int (*print_time_stamp)(u32 core, struct command_entry *ce);

	int (*suspend_cb)(u32 core_id);
	int (*resume_cb)(u32 core_id);
	int (*check_resource)(u32 core_id);
};

struct mdla_cmd_cb_func *mdla_cmd_plat_cb(void);


#endif /* __MDLA_CMD_PROC_H__ */
