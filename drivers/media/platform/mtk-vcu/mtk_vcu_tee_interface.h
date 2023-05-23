/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _MTK_VCU_TEE_INTERFACE_H_
#define _MTK_VCU_TEE_INTERFACE_H_

#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/wait.h>

#define VCU_TEE_SHM_FLAG_NONE           0x00000000
#define VCU_TEE_SHM_FLAG_PRE_ALLOCATED  0x00000001

enum mtk_vcu_tee_ta_type {
	VCU_TEE_TA_VDEC,
	VCU_TEE_TA_VES_PARSER,
	VCU_TEE_TA_NUM,
};

enum mtk_vcu_tee_param_index {
	VCU_TEE_PARAM_0,
	VCU_TEE_PARAM_1,
	VCU_TEE_PARAM_2,
	VCU_TEE_PARAM_3,
	VCU_TEE_PARAM_COUNT
};

enum mtk_vcu_tee_param_type {
	VCU_TEE_PARAM_NONE,
	VCU_TEE_PARAM_VALUE_IN,
	VCU_TEE_PARAM_VALUE_OUT,
	VCU_TEE_PARAM_VALUE_INOUT,
	VCU_TEE_PARAM_MEMREF_IN,
	VCU_TEE_PARAM_MEMREF_OUT,
	VCU_TEE_PARAM_MEMREF_INOUT,
	VCU_TEE_PARAM_PRESET_SHM,
};

struct mtk_vcu_tee_value {
	u32 val_a;
	u32 val_b;
};

struct mtk_vcu_tee_temp_ref {
	void    *buffer;
	size_t  size;
};

struct mtk_vcu_tee_pre_shm {
	void	*shm;
	size_t	size;
	size_t	offset;
};

struct mtk_vcu_tee_param {
	enum mtk_vcu_tee_param_type type;
	struct mtk_vcu_tee_value    value;
	struct mtk_vcu_tee_temp_ref tmpref;
	struct mtk_vcu_tee_pre_shm  preshm;
};

struct mtk_vcu_tee_operation {
	struct mtk_vcu_tee_param params[VCU_TEE_PARAM_COUNT];
};

struct mtk_vcu_tee_cmd_task {
	struct task_struct *task;
	struct completion completion;
	wait_queue_head_t queue;
	s32 pid;
	bool adjust_priority;
	bool adjustment_applied;
	bool lat_task;
	u32 target_priority;
	u32 current_priority;
};

struct mtk_vcu_tee_cmd_msg {
	u32  cmd;
	s32  ret;
	void *param;
};

struct mtk_vcu_tee_share_mem {
	void *buffer;
	size_t size;
	uint32_t flags;
	size_t alloced_size;
	void *shadow_buffer;
};

struct mtk_vcu_tee_context {
	enum mtk_vcu_tee_ta_type ta_type;
	void *tee_context;
	u32  session_id;
	bool session_initialized;
	struct mtk_vcu_tee_share_mem preset_shm[VCU_TEE_PARAM_COUNT];
};

struct mtk_vcu_tee_preset_buf {
	struct mtk_vcu_tee_operation *op;
	u32 *err_origin;
	void *param_buf[VCU_TEE_PARAM_COUNT];
	u32 param_buf_size[VCU_TEE_PARAM_COUNT];
	void *task_cmd_param;
	u32 task_cmd_param_size;
};

struct mtk_vcu_tee_pre_reg_shm {
	struct mtk_vcu_tee_share_mem base_param_shm;
	struct mtk_vcu_tee_share_mem empty_param_shm;
};

struct mtk_vcu_tee_handler {
	struct mtk_vcu_tee_context tee_ctx;
	struct mtk_vcu_tee_cmd_task cmd_task;
	struct mtk_vcu_tee_cmd_msg cmd_msg;
	struct mtk_vcu_tee_preset_buf preset_buf;
	struct mtk_vcu_tee_pre_reg_shm preset_shm;
};

struct mtk_vcu_tee_interface {
	int (*open_handler)(struct mtk_vcu_tee_handler *tee_handler);
	int (*close_handler)(struct mtk_vcu_tee_handler *tee_handler);
	int (*invoke_command)(struct mtk_vcu_tee_handler *tee_handler,
			      u32 command_id,
			      struct mtk_vcu_tee_operation *tee_operation,
			      u32 *return_origin);
	int (*register_shm)(struct mtk_vcu_tee_handler *tee_handler,
			    struct mtk_vcu_tee_share_mem *vcu_tee_shm);
	int (*unregister_shm)(struct mtk_vcu_tee_handler *tee_handler,
			      struct mtk_vcu_tee_share_mem *vcu_tee_shm);
};

int vcu_tee_open_handler(struct mtk_vcu_tee_handler *tee_handler);

int vcu_tee_close_handler(struct mtk_vcu_tee_handler *tee_handler);

int vcu_tee_invoke_command(struct mtk_vcu_tee_handler *tee_handler,
			   u32 command_id,
			   struct mtk_vcu_tee_operation *tee_operation,
			   u32 *return_origin);

int vcu_tee_register_shm(struct mtk_vcu_tee_handler *tee_handler,
			  struct mtk_vcu_tee_share_mem *vcu_tee_shm);

int vcu_tee_unregister_shm(struct mtk_vcu_tee_handler *tee_handler,
			    struct mtk_vcu_tee_share_mem *vcu_tee_shm);

#endif
