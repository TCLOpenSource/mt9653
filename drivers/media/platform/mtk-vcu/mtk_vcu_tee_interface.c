// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/err.h>
#include <linux/tee.h>
#include <linux/tee_drv.h>
#include <linux/uuid.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "mtk_vcu_tee_interface.h"

#define VCU_TEE_RTN_IF      0xF0000001
#define VCU_TEE_RTN_OP      0xF0000002
#define VCU_TEE_RTN_INVOKE  0xF0000003

#define VDEC_TA_UUID UUID_INIT(0x61328cc8, 0x8df1, 0x4821, \
	0x89, 0x01, 0x76, 0x1b, 0xb2, 0xb9, 0x11, 0x5c)

#define VES_PARSER_TA_UUID UUID_INIT(0xe95601fc, 0x8423, 0x481d, \
	0xaa, 0x67, 0x27, 0x49, 0xd5, 0xa3, 0x69, 0xb8)

#define TEE_SHM_DEFAULT_LEN 8

/**
 * Encode the paramTypes according to the supplied types.
 *
 * @param p0 The first param type.
 * @param p1 The second param type.
 * @param p2 The third param type.
 * @param p3 The fourth param type.
 */
#define TEEC_PARAM_TYPES(p0, p1, p2, p3) \
	((p0) | ((p1) << 4) | ((p2) << 8) | ((p3) << 12))

/**
 * Get the i_th param type from the paramType.
 *
 * @param p The paramType.
 * @param i The i-th parameter to get the type for.
 */
#define TEEC_PARAM_TYPE_GET(p, i) (((p) >> (i * 4)) & 0xF)

#define SET_RET_ORN(orn, ret) do { if (orn) *orn = ret; } while (0)

#define IS_PREALLOC_SHM(shm) (shm->flags & VCU_TEE_SHM_FLAG_PRE_ALLOCATED)

enum vcu_tee_cmd_type {
	VCU_TEE_CMD_NONE,
	VCU_TEE_CMD_OPEN_CONTEXT,
	VCU_TEE_CMD_CLOSE_CONTEXT,
	VCU_TEE_CMD_INVOKE_COMMAND,
	VCU_TEE_CMD_REGISTER_SHM,
	VCU_TEE_CMD_UNREGISTER_SHM,
};

struct invoke_hdl_param {
	struct mtk_vcu_tee_context *tee_ctx;
	u32 cmd_id;
	struct mtk_vcu_tee_operation *tee_op;
	u32 *ret_orn;
};

struct register_shm_hdl_param {
	struct mtk_vcu_tee_context *tee_ctx;
	struct mtk_vcu_tee_share_mem *vcu_tee_shm;
};

struct TempMemoryReference {
	void *buffer;
	size_t size;
};

struct TempValue {
	uint32_t a;
	uint32_t b;
};

struct TempShm {
	void *shm;
	size_t size;
	size_t offset;
};

struct parameter {
	struct TempValue value;
	struct TempMemoryReference tmpref;
	struct TempShm preshm;
};

struct vcu_teec_operation {
	uint32_t paramTypes;
	struct parameter params[VCU_TEE_PARAM_COUNT];
};

static uuid_t ta_uuid[VCU_TEE_TA_NUM] = {
	VDEC_TA_UUID,           // VDEC TA
	VES_PARSER_TA_UUID,     // VES_PARSER TA
};

#define VCU_TEE_SHM_PRE_ALLOC_SIZE          0x2000
#define VCU_TEE_HDL_PARAM_PRE_ALLOC_SIZE    sizeof(struct invoke_hdl_param)

static int _optee_param_type(enum mtk_vcu_tee_param_type param_type)
{
	switch (param_type) {
	case VCU_TEE_PARAM_NONE:
		return TEE_IOCTL_PARAM_ATTR_TYPE_NONE;
	case VCU_TEE_PARAM_VALUE_IN:
		return TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	case VCU_TEE_PARAM_VALUE_OUT:
		return TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	case VCU_TEE_PARAM_VALUE_INOUT:
		return TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	case VCU_TEE_PARAM_MEMREF_IN:
		return TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	case VCU_TEE_PARAM_MEMREF_OUT:
		return TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	case VCU_TEE_PARAM_MEMREF_INOUT:
		return TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	case VCU_TEE_PARAM_PRESET_SHM:
		return TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	default:
		return TEE_IOCTL_PARAM_ATTR_TYPE_NONE;
	}
}

static int _teec_allocatesharedmemory(struct tee_context *ctx,
				      struct mtk_vcu_tee_share_mem *shm,
				      uint32_t flag)
{
	struct tee_shm *_shm = NULL;
	void *shm_va = NULL;
	size_t len;
	struct tee_context *context = ctx;

	shm->shadow_buffer = shm->buffer = NULL;
	len = shm->size;
	if (!len)
		len = TEE_SHM_DEFAULT_LEN;

	shm_va = vmalloc(len);
	if (!shm_va || IS_ERR(shm_va)) {
		pr_err("%s %d: tee_shm vmalloc Fail\n",
		       __func__, __LINE__);
		return -ENOMEM;
	}

	_shm = tee_shm_register(context, (uintptr_t)shm_va, len,
		TEE_SHM_KERNEL_MAPPED | TEE_SHM_DMA_BUF);
	if (!_shm || IS_ERR(_shm)) {
		pr_err("%s %d: tee_shm_register Fail\n",
		       __func__, __LINE__);
		vfree(shm_va);
		return -ENOMEM;
	}

	shm->buffer = shm_va;
	shm->shadow_buffer = (void *)_shm;
	shm->alloced_size = len;
	shm->flags = flag;

	return 0;
}

static void _teec_freesharedmemory(struct mtk_vcu_tee_share_mem *shm)
{
	if (shm->shadow_buffer)
		tee_shm_put((struct tee_shm *)shm->shadow_buffer);

	if (shm->buffer)
		vfree(shm->buffer);

	shm->shadow_buffer = NULL;
	shm->buffer = NULL;
	shm->flags = VCU_TEE_SHM_FLAG_NONE;
}

static void _teec_releasesharedmemory(struct mtk_vcu_tee_share_mem *shm)
{
	if (!IS_PREALLOC_SHM(shm))
		_teec_freesharedmemory(shm);
}

static int _teec_reallocatesharedmemory(struct tee_context *ctx,
				      struct mtk_vcu_tee_share_mem *shm,
				      size_t size)
{
	int res;
	uint32_t flag;

	flag = shm->flags;

	_teec_freesharedmemory(shm);

	shm->size = size;

	res = _teec_allocatesharedmemory(ctx, shm, flag);

	return res;
}

static int _teec_pre_process_tmpref(struct tee_context *ctx,
				    uint32_t param_type,
				    struct TempMemoryReference *tmpref,
				    struct tee_param *param,
				    struct mtk_vcu_tee_share_mem *shm,
				    struct mtk_vcu_tee_share_mem *preset_shm)
{
	int res;

	param->attr = _optee_param_type(param_type);
	shm->size = tmpref->size;

	if (preset_shm && preset_shm->shadow_buffer) {
		if (shm->size > preset_shm->size) {
			res = _teec_reallocatesharedmemory(ctx, preset_shm, shm->size);
			if (res != 0)
				return res;
		}

		memcpy(shm, preset_shm, sizeof(*shm));
	} else {
		res = _teec_allocatesharedmemory(ctx, shm, VCU_TEE_SHM_FLAG_NONE);
		if (res != 0)
			return res;
	}

	memcpy(shm->buffer, tmpref->buffer, tmpref->size);
	param->u.memref.size = tmpref->size;
	//Workaround, shm->shadow_buffer is shm object.
	param->u.memref.shm = (struct tee_shm *)shm->shadow_buffer;
	return 0;
}

static int _teec_pre_process_operation(struct mtk_vcu_tee_context *vcu_tee_ctx,
				       struct vcu_teec_operation *operation,
				       struct tee_param *params,
				       struct mtk_vcu_tee_share_mem *shms)
{
	int res;
	size_t n;
	uint32_t param_type;
	struct tee_context *ctx;

	if (!vcu_tee_ctx)
		return -EINVAL;

	ctx = vcu_tee_ctx->tee_context;

	memset(shms, 0, sizeof(*shms) * VCU_TEE_PARAM_COUNT);
	if (!operation) {
		memset(params, 0, sizeof(*params) * VCU_TEE_PARAM_COUNT);
		return 0;
	}

	for (n = 0; n < VCU_TEE_PARAM_COUNT; n++) {
		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);
		switch (param_type) {
		case VCU_TEE_PARAM_NONE:
			params[n].attr = _optee_param_type(param_type);
			break;
		case VCU_TEE_PARAM_VALUE_IN:
		case VCU_TEE_PARAM_VALUE_OUT:
		case VCU_TEE_PARAM_VALUE_INOUT:
			params[n].attr = _optee_param_type(param_type);
			params[n].u.value.a = operation->params[n].value.a;
			params[n].u.value.b = operation->params[n].value.b;
			break;
		case VCU_TEE_PARAM_MEMREF_IN:
		case VCU_TEE_PARAM_MEMREF_OUT:
		case VCU_TEE_PARAM_MEMREF_INOUT:
			res = _teec_pre_process_tmpref(ctx, param_type,
						       &operation->params[n].tmpref,
						       params + n, shms + n,
						       vcu_tee_ctx->preset_shm + n);
			if (res != 0)
				return res;
			break;
		case VCU_TEE_PARAM_PRESET_SHM:
			params[n].attr = _optee_param_type(param_type);
			params[n].u.memref.shm = (struct tee_shm *)operation->params[n].preshm.shm;
			params[n].u.memref.size = operation->params[n].preshm.size;
			params[n].u.memref.shm_offs = operation->params[n].preshm.offset;
			break;
		default:
			pr_err("[%s] param_type not support!",
				__func__);
			return -1;
		}
	}

	return 0;
}

static void _teec_post_process_tmpref(uint32_t param_type,
				      struct TempMemoryReference *tmpref,
				      struct tee_param *param,
				      struct mtk_vcu_tee_share_mem *shm)
{
	if (param_type != VCU_TEE_PARAM_MEMREF_IN) {
		if (param->u.memref.size <= tmpref->size && tmpref->buffer)
			memcpy(tmpref->buffer, shm->buffer,
			       param->u.memref.size);

		tmpref->size = param->u.memref.size;
	}
}

static void _teec_post_process_operation(struct vcu_teec_operation *operation,
					 struct tee_param *params,
					 struct mtk_vcu_tee_share_mem *shms)
{
	size_t n;

	if (!operation)
		return;

	for (n = 0; n < VCU_TEE_PARAM_COUNT; n++) {
		uint32_t param_type;

		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);
		switch (param_type) {
		case VCU_TEE_PARAM_VALUE_IN:
			break;
		case VCU_TEE_PARAM_VALUE_OUT:
		case VCU_TEE_PARAM_VALUE_INOUT:
			operation->params[n].value.a = params[n].u.value.a;
			operation->params[n].value.b = params[n].u.value.b;
			break;
		case VCU_TEE_PARAM_MEMREF_IN:
		case VCU_TEE_PARAM_MEMREF_OUT:
		case VCU_TEE_PARAM_MEMREF_INOUT:
			_teec_post_process_tmpref(param_type,
						  &operation->params[n].tmpref,
						  params + n, shms + n);
			break;
		case VCU_TEE_PARAM_PRESET_SHM:
			break;
		default:
			break;
		}
	}
}

static void _teec_free_temp_refs(struct vcu_teec_operation *operation,
				 struct mtk_vcu_tee_share_mem *shms)
{
	size_t n;

	if (!operation)
		return;

	for (n = 0; n < VCU_TEE_PARAM_COUNT; n++) {
		switch (TEEC_PARAM_TYPE_GET(operation->paramTypes, n)) {
		case VCU_TEE_PARAM_MEMREF_IN:
		case VCU_TEE_PARAM_MEMREF_OUT:
		case VCU_TEE_PARAM_MEMREF_INOUT:
			_teec_releasesharedmemory(shms + n);
			break;
		default:
			break;
		}
	}
}

static int _optee_match(struct tee_ioctl_version_data *data, const void *vers)
{
	return 1;
}

static int _vcu_tee_client_open_context(struct mtk_vcu_tee_context *vcu_tee_ctx)
{
	struct tee_context *tee_ctx;
	struct tee_ioctl_version_data vers = {
		.impl_id = TEE_OPTEE_CAP_TZ,
		.impl_caps = TEE_IMPL_ID_OPTEE,
		.gen_caps = TEE_GEN_CAP_GP,
	};

	if (vcu_tee_ctx->session_initialized)
		return 0;

	tee_ctx = tee_client_open_context(NULL, _optee_match,
					  NULL, &vers);

	if (IS_ERR(tee_ctx)) {
		pr_err("[%s] context is NULL\n", __func__);
		return -EINVAL;
	}

	vcu_tee_ctx->tee_context = (void *)tee_ctx;
	vcu_tee_ctx->session_initialized = true;
	return 0;
}

static void _vcu_tee_client_close_context(struct mtk_vcu_tee_context *vcu_tee_ctx)
{
	if (!vcu_tee_ctx->session_initialized)
		return;

	tee_client_close_context((struct tee_context *)vcu_tee_ctx->tee_context);
	vcu_tee_ctx->tee_context = NULL;
	vcu_tee_ctx->session_initialized = false;
}

static int _vcu_tee_open_context(struct mtk_vcu_tee_context *vcu_tee_ctx, bool alloc_shm)
{
	struct tee_ioctl_open_session_arg arg;
	struct tee_param param;
	int rc;
	int res;

	if (!vcu_tee_ctx) {
		pr_err("[%s] Err: NULL VCU TEE context\n", __func__);
		return -EINVAL;
	}

	if (vcu_tee_ctx->ta_type >= VCU_TEE_TA_NUM) {
		pr_err("[%s] Wrong TA type: ta_type=%d\n",
		       __func__, vcu_tee_ctx->ta_type);
		return -EINVAL;
	}

	res = _vcu_tee_client_open_context(vcu_tee_ctx);
	if (res != 0) {
		pr_err("[%s] _vcu_open_tee_client_context fail\n", __func__);
		return -EINVAL;
	}

	memset(&arg, 0, sizeof(arg));
	memset(&param, 0, sizeof(param));

	memcpy(arg.uuid, &ta_uuid[vcu_tee_ctx->ta_type], TEE_IOCTL_UUID_LEN);
	arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;

	rc = tee_client_open_session((struct tee_context *)vcu_tee_ctx->tee_context,
				     &arg, (struct tee_param *)&param);
	if (rc == 0) {
		vcu_tee_ctx->session_id = arg.session;
	} else {
		pr_err("[%s] tee_client_open_session fail,rc=0x%x\n",
		       __func__, rc);
		_vcu_tee_client_close_context(vcu_tee_ctx);
		return -EINVAL;
	}

	if (alloc_shm) {
		int i, rm;

		for (i = 0; i < VCU_TEE_PARAM_COUNT; ++i) {
			vcu_tee_ctx->preset_shm[i].size = VCU_TEE_SHM_PRE_ALLOC_SIZE;

			rm = _teec_allocatesharedmemory(vcu_tee_ctx->tee_context,
							 &vcu_tee_ctx->preset_shm[i],
							 VCU_TEE_SHM_FLAG_PRE_ALLOCATED);
			if (rm != 0)
				break;
		}

		if (rm != 0) {
			for (i = 0; i < VCU_TEE_PARAM_COUNT; ++i)
				_teec_freesharedmemory(&vcu_tee_ctx->preset_shm[i]);

			tee_client_close_session(vcu_tee_ctx->tee_context, vcu_tee_ctx->session_id);
			_vcu_tee_client_close_context(vcu_tee_ctx);
			return rm;
		}
	}

	return 0;
}

static int _vcu_tee_close_context(struct mtk_vcu_tee_context *vcu_tee_ctx, bool free_shm)
{
	int i;

	if (!vcu_tee_ctx) {
		pr_err("[%s] Err: NULL VCU TEE context\n", __func__);
		return -EINVAL;
	}

	if (free_shm) {
		for (i = 0; i < VCU_TEE_PARAM_COUNT; ++i)
			_teec_freesharedmemory(&vcu_tee_ctx->preset_shm[i]);
	}

	tee_client_close_session((struct tee_context *)vcu_tee_ctx->tee_context,
				 vcu_tee_ctx->session_id);

	_vcu_tee_client_close_context(vcu_tee_ctx);

	return 0;
}

static int _vcu_tee_invoke_command(struct mtk_vcu_tee_context *vcu_tee_ctx,
				   u32 command_id,
				   struct mtk_vcu_tee_operation *tee_operation,
				   u32 *return_origin)
{
	struct mtk_vcu_tee_context *act_tee_ctx;
	struct mtk_vcu_tee_context inner_tee_ctx;
	struct tee_ioctl_invoke_arg *arg;
	struct tee_param params[VCU_TEE_PARAM_COUNT];
	struct vcu_teec_operation *operation = NULL;
	struct mtk_vcu_tee_share_mem shm[VCU_TEE_PARAM_COUNT];
	uint32_t eorig = 0;
	int res;
	int rc;
	int i;

	if (!vcu_tee_ctx || !tee_operation) {
		SET_RET_ORN(return_origin, VCU_TEE_RTN_OP);
		return -EINVAL;
	}

	if (!vcu_tee_ctx->tee_context) {
		memcpy(&inner_tee_ctx, vcu_tee_ctx, sizeof(inner_tee_ctx));
		res = _vcu_tee_open_context(&inner_tee_ctx, false);
		if (res) {
			SET_RET_ORN(return_origin, VCU_TEE_RTN_OP);
			return res;
		}
		act_tee_ctx = &inner_tee_ctx;
	} else {
		act_tee_ctx = vcu_tee_ctx;
	}

	arg = kmalloc(sizeof(*arg), GFP_KERNEL);
	if (!arg) {
		eorig = VCU_TEE_RTN_OP;
		res = -ENOMEM;
		goto out_free_temp_refs;
	}

	operation = kmalloc(sizeof(*operation), GFP_KERNEL);
	if (!operation) {
		eorig = VCU_TEE_RTN_OP;
		res = -ENOMEM;
		goto out_free_temp_refs;
	}

	memset(arg, 0x0, sizeof(*arg));
	memset(params, 0x0, sizeof(params));
	memset(operation, 0x0, sizeof(*operation));
	memset(shm, 0x0, sizeof(shm));

	arg->num_params = VCU_TEE_PARAM_COUNT;
	arg->session = act_tee_ctx->session_id;
	arg->func = command_id;

	operation->paramTypes = TEEC_PARAM_TYPES(
		tee_operation->params[VCU_TEE_PARAM_0].type,
		tee_operation->params[VCU_TEE_PARAM_1].type,
		tee_operation->params[VCU_TEE_PARAM_2].type,
		tee_operation->params[VCU_TEE_PARAM_3].type);

	for (i = 0; i < VCU_TEE_PARAM_COUNT; ++i) {
		switch (tee_operation->params[i].type) {
		case VCU_TEE_PARAM_NONE:
			break;
		case VCU_TEE_PARAM_VALUE_IN:
		case VCU_TEE_PARAM_VALUE_OUT:
		case VCU_TEE_PARAM_VALUE_INOUT:
			operation->params[i].value.a =
				tee_operation->params[i].value.val_a;
			operation->params[i].value.b =
				tee_operation->params[i].value.val_b;
			break;
		case VCU_TEE_PARAM_MEMREF_IN:
		case VCU_TEE_PARAM_MEMREF_OUT:
		case VCU_TEE_PARAM_MEMREF_INOUT:
			operation->params[i].tmpref.buffer =
				tee_operation->params[i].tmpref.buffer;
			operation->params[i].tmpref.size =
				tee_operation->params[i].tmpref.size;
			break;
		case VCU_TEE_PARAM_PRESET_SHM:
			operation->params[i].preshm.shm =
				tee_operation->params[i].preshm.shm;
			operation->params[i].preshm.size =
				tee_operation->params[i].preshm.size;
			operation->params[i].preshm.offset =
				tee_operation->params[i].preshm.offset;
			break;
		default:
			break;
		}
	}

	res = _teec_pre_process_operation(act_tee_ctx, operation, params, shm);
	if (res != 0) {
		eorig = VCU_TEE_RTN_OP;
		goto out_free_temp_refs;
	}

	rc = tee_client_invoke_func(act_tee_ctx->tee_context, arg, params);
	if (rc) {
		eorig = VCU_TEE_RTN_INVOKE;
		res = rc;
		goto out_free_temp_refs;
	}

	res = arg->ret;
	eorig = arg->ret_origin;
	_teec_post_process_operation(operation, params, shm);
out_free_temp_refs:
	_teec_free_temp_refs(operation, shm);
	SET_RET_ORN(return_origin, eorig);

	if (!vcu_tee_ctx->tee_context)
		_vcu_tee_close_context(&inner_tee_ctx, false);

	kfree(operation);
	kfree(arg);

	return res;
}

static int _vcu_tee_register_shm(struct mtk_vcu_tee_context *vcu_tee_ctx,
				 struct mtk_vcu_tee_share_mem *vcu_tee_shm)
{
	struct tee_shm *_shm;

	if (!vcu_tee_ctx) {
		pr_err("[%s] Err: NULL VCU TEE context\n", __func__);
		return -EINVAL;
	}

	if (!vcu_tee_shm) {
		pr_err("[%s] Err: NULL VCU TEE shm\n", __func__);
		return -EINVAL;
	}

	if (!vcu_tee_ctx->tee_context) {
		pr_err("[%s] Err: NULL TEE context\n", __func__);
		return -EINVAL;
	}

	if ((!vcu_tee_shm->buffer) || (vcu_tee_shm->size == 0)) {
		pr_err("[%s] Err: Invalid TEE SHM param. buffer %p size %zd\n",
			__func__, vcu_tee_shm->buffer, vcu_tee_shm->size);
		return -EINVAL;
	}

	_shm = tee_shm_register(vcu_tee_ctx->tee_context,
		(unsigned long)vcu_tee_shm->buffer,
		vcu_tee_shm->size,
		TEE_SHM_DMA_BUF | TEE_SHM_KERNEL_MAPPED);

	if (IS_ERR(_shm)) {
		pr_err("%s %d: tee_shm_register fail. buffer %p size %zd\n",
		       __func__, __LINE__, vcu_tee_shm->buffer, vcu_tee_shm->size);
		return -ENOMEM;
	}

	vcu_tee_shm->shadow_buffer = _shm;

	return 0;
}

static int _vcu_tee_unregister_shm(struct mtk_vcu_tee_context *vcu_tee_ctx,
				   struct mtk_vcu_tee_share_mem *vcu_tee_shm)
{
	if (!vcu_tee_ctx) {
		pr_err("[%s] Err: NULL VCU TEE context\n", __func__);
		return -EINVAL;
	}

	if (!vcu_tee_shm) {
		pr_err("[%s] Err: NULL VCU TEE shm\n", __func__);
		return -EINVAL;
	}

	if (!vcu_tee_shm->shadow_buffer) {
		pr_err("[%s] Err: NULL TEE shm\n", __func__);
		return -EINVAL;
	}

	tee_shm_free(vcu_tee_shm->shadow_buffer);
	vcu_tee_shm->shadow_buffer = NULL;

	return 0;
}

static int _vdec_tee_cmd_handle_task(void *data)
{
	struct mtk_vcu_tee_handler *vcu_tee_hdl;
	struct mtk_vcu_tee_cmd_task *cmd_task;
	struct mtk_vcu_tee_cmd_msg *cmd_msg;

	while (!kthread_should_stop()) {
		int ret;

		vcu_tee_hdl = data;
		if (!vcu_tee_hdl) {
			pr_err("Err: Null VCU CMD task data!\n");
			return -EINVAL;
		}

		cmd_task = &vcu_tee_hdl->cmd_task;
		cmd_msg = &vcu_tee_hdl->cmd_msg;

		if (cmd_task->pid == 0)
			cmd_task->pid = current->pid;

		wait_event(cmd_task->queue,
			   kthread_should_stop() ||
			   (cmd_msg->cmd != VCU_TEE_CMD_NONE));

		if (kthread_should_stop())
			break;

		switch (cmd_msg->cmd) {
		case VCU_TEE_CMD_OPEN_CONTEXT:
			ret = _vcu_tee_open_context(cmd_msg->param, true);
			break;
		case VCU_TEE_CMD_CLOSE_CONTEXT:
			ret = _vcu_tee_close_context(cmd_msg->param, true);
			break;
		case VCU_TEE_CMD_INVOKE_COMMAND: {
			struct invoke_hdl_param *param = cmd_msg->param;

			if (!param) {
				pr_err("Err: Null VCU TEE invoke command param!\n");
				ret = -EINVAL;
			} else {
				ret = _vcu_tee_invoke_command(param->tee_ctx,
							      param->cmd_id,
							      param->tee_op,
							      param->ret_orn);
			}
			break;
		}
		case VCU_TEE_CMD_REGISTER_SHM: {
			struct register_shm_hdl_param *param = cmd_msg->param;

			if (!param) {
				pr_err("Err: Null VCU TEE register shm param!\n");
				ret = -EINVAL;
			} else {
				ret = _vcu_tee_register_shm(param->tee_ctx, param->vcu_tee_shm);
			}
			break;
		}
		case VCU_TEE_CMD_UNREGISTER_SHM: {
			struct register_shm_hdl_param *param = cmd_msg->param;

			if (!param) {
				pr_err("Err: Null VCU TEE unregister shm param!\n");
				ret = -EINVAL;
			} else {
				ret = _vcu_tee_unregister_shm(param->tee_ctx, param->vcu_tee_shm);
			}
			break;
		}
		default:
			ret = -EINVAL;
			break;
		}

		cmd_msg->ret = ret;
		cmd_msg->cmd = VCU_TEE_CMD_NONE;

		complete(&cmd_task->completion);
	}

	return 0;
}


static int _vcu_tee_start_cmd_task(struct mtk_vcu_tee_handler *tee_handler)
{
	struct mtk_vcu_tee_cmd_task *cmd_task;

	if (!tee_handler)
		return -EINVAL;

	cmd_task = &tee_handler->cmd_task;

	if (!cmd_task->task) {
		init_completion(&cmd_task->completion);

		init_waitqueue_head(&cmd_task->queue);

		cmd_task->task = kthread_run(_vdec_tee_cmd_handle_task,
					     tee_handler,
					     "vcu-tee-cmd-task-%p",
					     tee_handler);

		if (IS_ERR_OR_NULL(cmd_task->task)) {
			pr_err("Create vcu tee cmd task failed. tee_handler %p",
			       tee_handler);
			return -EPERM;
		}
	}

	return 0;
}

static int _vcu_tee_stop_cmd_task(struct mtk_vcu_tee_handler *tee_handler)
{
	struct mtk_vcu_tee_cmd_task *cmd_task;

	if (!tee_handler)
		return -EINVAL;

	cmd_task = &tee_handler->cmd_task;

	if (cmd_task->task) {
		kthread_stop(cmd_task->task);
		cmd_task->task = NULL;
	}

	return 0;
}

static int _vcu_tee_send_cmd(struct mtk_vcu_tee_handler *tee_handler,
	unsigned int cmd,
	void *param)
{
	struct mtk_vcu_tee_cmd_task *cmd_task;

	if (!tee_handler)
		return -EINVAL;

	cmd_task = &tee_handler->cmd_task;

	if (!cmd_task->task)
		return -EPERM;

	tee_handler->cmd_msg.cmd = cmd;
	tee_handler->cmd_msg.param = param;

	wake_up(&cmd_task->queue);
	wait_for_completion(&cmd_task->completion);

	return tee_handler->cmd_msg.ret;
}


int vcu_tee_open_handler(struct mtk_vcu_tee_handler *tee_handler)
{
	int res;
	void *task_cmd_param;

	if (!tee_handler)
		return -EINVAL;

	res = _vcu_tee_start_cmd_task(tee_handler);
	if (res)
		return res;

	res = _vcu_tee_send_cmd(tee_handler,
				VCU_TEE_CMD_OPEN_CONTEXT,
				(void *)&tee_handler->tee_ctx);

	if (res) {
		_vcu_tee_stop_cmd_task(tee_handler);
		return res;
	}

	task_cmd_param = kmalloc(VCU_TEE_HDL_PARAM_PRE_ALLOC_SIZE, GFP_KERNEL);
	if (task_cmd_param) {
		tee_handler->preset_buf.task_cmd_param = task_cmd_param;
		tee_handler->preset_buf.task_cmd_param_size = VCU_TEE_HDL_PARAM_PRE_ALLOC_SIZE;
	}

	return res;
}

int vcu_tee_close_handler(struct mtk_vcu_tee_handler *tee_handler)
{
	int res;

	if (!tee_handler)
		return -EINVAL;

	res = _vcu_tee_send_cmd(tee_handler,
				VCU_TEE_CMD_CLOSE_CONTEXT,
				(void *)&tee_handler->tee_ctx);

	_vcu_tee_stop_cmd_task(tee_handler);

	kfree(tee_handler->preset_buf.task_cmd_param);

	return res;
}

int vcu_tee_invoke_command(struct mtk_vcu_tee_handler *tee_handler,
			   u32 command_id,
			   struct mtk_vcu_tee_operation *tee_operation,
			   u32 *return_origin)
{
	int res;
	struct task_struct *act_cmd_task;
	struct invoke_hdl_param *invoke_param;
	struct invoke_hdl_param *inner_invoke_param = NULL;

	if (!tee_handler || !tee_operation) {
		SET_RET_ORN(return_origin, VCU_TEE_RTN_IF);
		return -EINVAL;
	}

	do {
		if (tee_handler->preset_buf.task_cmd_param &&
		    tee_handler->preset_buf.task_cmd_param_size >= sizeof(*invoke_param)) {
			invoke_param = tee_handler->preset_buf.task_cmd_param;
		} else {
			inner_invoke_param = kmalloc(sizeof(*inner_invoke_param), GFP_KERNEL);
			if (!inner_invoke_param) {
				SET_RET_ORN(return_origin, VCU_TEE_RTN_IF);
				res = -ENOMEM;
				break;
			}

			invoke_param = inner_invoke_param;
		}

		act_cmd_task = tee_handler->cmd_task.task;
		if (!act_cmd_task) {
			res = _vcu_tee_start_cmd_task(tee_handler);
			if (res) {
				SET_RET_ORN(return_origin, VCU_TEE_RTN_IF);
				res = -EPERM;
				break;
			}
		}

		memset(invoke_param, 0, sizeof(*invoke_param));
		invoke_param->tee_ctx = &tee_handler->tee_ctx;
		invoke_param->cmd_id = command_id;
		invoke_param->tee_op = tee_operation;
		invoke_param->ret_orn = return_origin;

		res = _vcu_tee_send_cmd(tee_handler,
					VCU_TEE_CMD_INVOKE_COMMAND,
					(void *)invoke_param);

		if (!act_cmd_task)
			_vcu_tee_stop_cmd_task(tee_handler);
	} while (0);

	kfree(inner_invoke_param);

	return res;
}

int vcu_tee_register_shm(struct mtk_vcu_tee_handler *tee_handler,
			 struct mtk_vcu_tee_share_mem *vcu_tee_shm)
{
	int res = 0;
	struct task_struct *preset_cmd_task;
	struct register_shm_hdl_param *reg_shm_param;
	struct register_shm_hdl_param *inner_reg_shm_param = NULL;

	if (!tee_handler) {
		pr_err("Err: Register TEE SHM fail. tee_handler is NULL\n");
		return -EINVAL;
	}

	if (!vcu_tee_shm) {
		pr_err("Err: Register TEE SHM fail. vcu_tee_shm is NULL\n");
		return -EINVAL;
	}

	if ((!vcu_tee_shm->buffer) || (vcu_tee_shm->size == 0)) {
		pr_err("Err: Register TEE SHM fail. invalid source buf. buffer %p size %zd\n",
		       vcu_tee_shm->buffer,
		       vcu_tee_shm->size);
		return -EINVAL;
	}

	do {
		if (tee_handler->preset_buf.task_cmd_param &&
		    tee_handler->preset_buf.task_cmd_param_size >= sizeof(*reg_shm_param)) {
			reg_shm_param = tee_handler->preset_buf.task_cmd_param;
		} else {
			inner_reg_shm_param = kmalloc(sizeof(*inner_reg_shm_param), GFP_KERNEL);
			if (!inner_reg_shm_param) {
				res = -ENOMEM;
				break;
			}

			reg_shm_param = inner_reg_shm_param;
		}

		preset_cmd_task = tee_handler->cmd_task.task;
		if (!preset_cmd_task) {
			res = _vcu_tee_start_cmd_task(tee_handler);
			if (res) {
				pr_err("Err: Register TEE SHM start cmd task fail!\n");
				res = -EPERM;
				break;
			}
		}

		memset(reg_shm_param, 0, sizeof(*reg_shm_param));
		reg_shm_param->tee_ctx = &tee_handler->tee_ctx;
		reg_shm_param->vcu_tee_shm = vcu_tee_shm;

		res = _vcu_tee_send_cmd(tee_handler,
					VCU_TEE_CMD_REGISTER_SHM,
					(void *)reg_shm_param);

		if (!preset_cmd_task)
			_vcu_tee_stop_cmd_task(tee_handler);
	} while (0);

	kfree(inner_reg_shm_param);

	return res;
}

int vcu_tee_unregister_shm(struct mtk_vcu_tee_handler *tee_handler,
			   struct mtk_vcu_tee_share_mem *vcu_tee_shm)
{
	int res = 0;
	struct task_struct *preset_cmd_task;
	struct register_shm_hdl_param *reg_shm_param;
	struct register_shm_hdl_param *inner_reg_shm_param = NULL;

	if (!tee_handler) {
		pr_err("Err: Unregister TEE SHM fail. tee_handler is NULL\n");
		return -EINVAL;
	}

	if (!vcu_tee_shm) {
		pr_err("Err: Unregister TEE SHM fail. vcu_tee_shm is NULL\n");
		return -EINVAL;
	}

	if (!vcu_tee_shm->shadow_buffer) {
		pr_err("Err: Register TEE SHM fail. shadow_buffer is NULL\n");
		return -EINVAL;
	}

	do {
		if (tee_handler->preset_buf.task_cmd_param &&
		    tee_handler->preset_buf.task_cmd_param_size >= sizeof(*reg_shm_param)) {
			reg_shm_param = tee_handler->preset_buf.task_cmd_param;
		} else {
			inner_reg_shm_param = kmalloc(sizeof(*inner_reg_shm_param), GFP_KERNEL);
			if (!inner_reg_shm_param) {
				res = -ENOMEM;
				break;
			}

			reg_shm_param = inner_reg_shm_param;
		}

		preset_cmd_task = tee_handler->cmd_task.task;
		if (!preset_cmd_task) {
			res = _vcu_tee_start_cmd_task(tee_handler);
			if (res) {
				pr_err("Err: Register TEE SHM start cmd task fail!\n");
				res = -EPERM;
				break;
			}
		}

		memset(reg_shm_param, 0, sizeof(*reg_shm_param));
		reg_shm_param->tee_ctx = &tee_handler->tee_ctx;
		reg_shm_param->vcu_tee_shm = vcu_tee_shm;

		res = _vcu_tee_send_cmd(tee_handler,
					VCU_TEE_CMD_UNREGISTER_SHM,
					(void *)reg_shm_param);

		if (!preset_cmd_task)
			_vcu_tee_stop_cmd_task(tee_handler);
	} while (0);

	kfree(inner_reg_shm_param);

	return res;
}

