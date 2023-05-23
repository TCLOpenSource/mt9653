// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <asm/uaccess.h>
#include "comm_debug.h"
#include "comm_teec.h"

static struct TEEC_UUID uuid_list[] = { AI_TA_UUID, CPUFREQ_TA_UUID, LTP_TA_UUID };
struct comm_teec_context teec_ctx[MAX_UUID_IDX];

static void uuid_to_octets(uint8_t d[TEE_IOCTL_UUID_LEN], const struct TEEC_UUID *s)
{
	d[0] = s->timeLow >> 24;
	d[1] = s->timeLow >> 16;
	d[2] = s->timeLow >> 8;
	d[3] = s->timeLow;
	d[4] = s->timeMid >> 8;
	d[5] = s->timeMid;
	d[6] = s->timeHiAndVersion >> 8;
	d[7] = s->timeHiAndVersion;
	memcpy(d + 8, s->clockSeqAndNode, sizeof(s->clockSeqAndNode));
}

static int _optee_match(struct tee_ioctl_version_data *data, const void *vers)
{
	return 1;
}

static int comm_get_optee_version(struct comm_teec_context *pstOpteeCtx)
{
	const struct firmware *fw;
	int ret;

	if (pstOpteeCtx->optee_version != -1)
		goto final;

	if (pstOpteeCtx->optee_version == -1) {
		char *sVer = NULL;

		sVer = kmalloc(sizeof(char) * BOOTARG_SIZE, GFP_KERNEL);
		if (sVer == NULL)
			goto final;

		ret = request_firmware_direct(&fw, "/proc/tz2_mstar/version", pstOpteeCtx->dev);
		if (ret) {
			pstOpteeCtx->optee_version = 1;
			release_firmware(fw);
			goto free_sVer;
		}

		memcpy(sVer, fw->data, BOOTARG_SIZE);
		sVer[BOOTARG_SIZE - 2] = '\n';
		sVer[BOOTARG_SIZE - 1] = '\0';

		if (strncmp(sVer, "2.4", 2) == 0)
			pstOpteeCtx->optee_version = 2;
		else if (strncmp(sVer, "3.2", 2) == 0)
			pstOpteeCtx->optee_version = 3;
		else
			pstOpteeCtx->optee_version = 1;

		release_firmware(fw);
free_sVer:
		kfree(sVer);
	}

final:
	return pstOpteeCtx->optee_version;
}

static void comm_teec_free_temp_refs(struct TEEC_Operation *operation,
		struct TEEC_SharedMemory *shms)
{
	size_t n = 0;
	uint32_t param_type = 0;
	struct TEEC_SharedMemory *free_shm = NULL;

	if (!operation)
		return;

	for (n = 0; n < TEEC_CONFIG_PAYLOAD_REF_COUNT; n++) {
		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);

		switch (param_type) {
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			free_shm = shms + n;
			tee_shm_free((struct tee_shm *)free_shm->shadow_buffer);
			free_shm->id = -1;
			free_shm->shadow_buffer = NULL;
			free_shm->buffer = NULL;
			free_shm->registered_fd = -1;
			break;
		case TEEC_NONE:
		case TEEC_VALUE_INPUT:
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
		case TEEC_MEMREF_WHOLE:
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			break;
		default:
			break;
		}
	}
}

static int comm_teec_alloc_shm(struct TEEC_Context *ctx, struct TEEC_SharedMemory *shm)
{
	struct tee_shm *_shm = NULL;
	void *shm_va = NULL;
	size_t len = shm->size;
	struct tee_context *context = (struct tee_context *) ctx->fd;

	if (!len)
		len = 8;

	shm->shadow_buffer = shm->buffer = NULL;

	_shm = tee_shm_alloc(context, len, TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
	if (!_shm) {
		comm_drv_debug("[%s] tee shm alloc got NULL\n", __func__);
		return -ENOMEM;
	}

	if (IS_ERR(_shm)) {
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "tee shm alloc fail\n");
		return -ENOMEM;
	}

	shm_va = tee_shm_get_va(_shm, 0);
	if (!shm_va) {
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "tee shm get va got NULL\n");
		return -ENOMEM;
	}

	if (IS_ERR(shm_va)) {
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "tee shm get va fail\n");
		return -ENOMEM;
	}

	shm->buffer = shm_va;
	shm->shadow_buffer = (void *)_shm;
	shm->alloced_size = len;
	shm->registered_fd = -1;

	return 0;
}

static void comm_teec_post_proc_tmpref(uint32_t param_type,
		struct TEEC_TempMemoryReference *tmpref,
		struct tee_param *param,
		struct TEEC_SharedMemory *shm)
{
	if (param_type != TEEC_MEMREF_TEMP_INPUT) {
		if (param->u.memref.size <= tmpref->size && tmpref->buffer)
			memcpy(tmpref->buffer, shm->buffer, param->u.memref.size);

		tmpref->size = param->u.memref.size;
	}
}

static void comm_teec_post_proc_whole(struct TEEC_RegisteredMemoryReference *memref,
		struct tee_param *param)
{
	struct TEEC_SharedMemory *shm = memref->parent;

	if (shm->flags & TEEC_MEM_OUTPUT) {

		/*
		 * We're using a shadow buffer in this reference, copy back
		 * the shadow buffer into the real buffer now that we've
		 * returned from secure world.
		 */
		if (shm->shadow_buffer && param->u.memref.size <= shm->size)
			memcpy(shm->buffer, shm->shadow_buffer, param->u.memref.size);

		memref->size = param->u.memref.size;
	}
}

static void comm_teec_post_proc_partial(uint32_t param_type,
		struct TEEC_RegisteredMemoryReference *memref,
		struct tee_param *param)
{
	if (param_type != TEEC_MEMREF_PARTIAL_INPUT) {
		struct TEEC_SharedMemory *shm = memref->parent;

		/*
		 * We're using a shadow buffer in this reference, copy back
		 * the shadow buffer into the real buffer now that we've
		 * returned from secure world.
		 */
		if (shm->shadow_buffer && param->u.memref.size <= memref->size)
			memcpy((char *)shm->buffer + memref->offset,
					(char *)shm->shadow_buffer + memref->offset,
					param->u.memref.size);

		memref->size = param->u.memref.size;
	}
}

static void comm_teec_post_proc_operation(struct TEEC_Operation *operation,
		struct tee_param *params,
		struct TEEC_SharedMemory *shms)
{
	size_t n = 0;
	uint32_t param_type = 0;

	if (!operation)
		return;

	for (n = 0; n < TEEC_CONFIG_PAYLOAD_REF_COUNT; n++) {
		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);

		switch (param_type) {
		case TEEC_NONE:
		case TEEC_VALUE_INPUT:
			break;
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
			operation->params[n].value.a = params[n].u.value.a;
			operation->params[n].value.b = params[n].u.value.b;
			break;
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			comm_teec_post_proc_tmpref(param_type,
					&operation->params[n].tmpref, params + n, shms + n);
			break;
		case TEEC_MEMREF_WHOLE:
			comm_teec_post_proc_whole(&operation->params[n].memref,
					params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			comm_teec_post_proc_partial(param_type,
					&operation->params[n].memref, params + n);
			break;
		default:
			break;
		}
	}
}

static int comm_teec_pre_proc_tmpref(struct TEEC_Context *ctx,
		uint32_t param_type, struct TEEC_TempMemoryReference *tmpref,
		struct tee_param *param, struct TEEC_SharedMemory *shm)
{
	int ret = 0;

	switch (param_type) {
	case TEEC_MEMREF_TEMP_INPUT:
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
		shm->flags = TEEC_MEM_INPUT;
		break;
	case TEEC_MEMREF_TEMP_OUTPUT:
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
		shm->flags = TEEC_MEM_OUTPUT;
		break;
	case TEEC_MEMREF_TEMP_INOUT:
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
		shm->flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
		break;
	default:
		return -EINVAL;
	}
	shm->size = tmpref->size;

	ret = comm_teec_alloc_shm(ctx, shm);
	if (ret) {
		comm_drv_debug("[%s] alloc fail\n", __func__);
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "share mem alloc fail\n");
		return -EPERM;
	}

	memcpy(shm->buffer, tmpref->buffer, tmpref->size);
	param->u.memref.size = tmpref->size;
	//Workaround, shm->shadow_buffer is shm object.
	param->u.memref.shm = (struct tee_shm *) shm->shadow_buffer;
	return ret;
}

static int comm_teec_pre_proc_whole(
		struct TEEC_RegisteredMemoryReference *memref,
		struct tee_param *param)
{
	const uint32_t inout = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	uint32_t flags = memref->parent->flags & inout;
	struct TEEC_SharedMemory *shm = NULL;

	if (flags == inout)
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	else if (flags & TEEC_MEM_INPUT)
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	else if (flags & TEEC_MEM_OUTPUT)
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	else
		return -EINVAL;

	shm = memref->parent;
	/*
	 * We're using a shadow buffer in this reference, copy the real buffer
	 * into the shadow buffer if needed. We'll copy it back once we've
	 * returned from the call to secure world.
	 */
	if (shm->shadow_buffer && (flags & TEEC_MEM_INPUT))
		memcpy(shm->shadow_buffer, shm->buffer, shm->size);

	param->u.memref.shm = shm->shadow_buffer;
	param->u.memref.size = shm->size;
	return 0;
}

static int comm_teec_pre_proc_partial(uint32_t param_type,
		struct TEEC_RegisteredMemoryReference *memref,
		struct tee_param *param)
{
	uint32_t req_shm_flags = 0;
	struct TEEC_SharedMemory *shm = NULL;

	switch (param_type) {
	case TEEC_MEMREF_PARTIAL_INPUT:
		req_shm_flags = TEEC_MEM_INPUT;
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
		break;
	case TEEC_MEMREF_PARTIAL_OUTPUT:
		req_shm_flags = TEEC_MEM_OUTPUT;
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
		break;
	case TEEC_MEMREF_PARTIAL_INOUT:
		req_shm_flags = TEEC_MEM_OUTPUT | TEEC_MEM_INPUT;
		param->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
		break;
	default:
		return -EINVAL;
	}

	shm = memref->parent;

	if ((shm->flags & req_shm_flags) != req_shm_flags)
		return -EPERM;

	/*
	 * We're using a shadow buffer in this reference, copy the real buffer
	 * into the shadow buffer if needed. We'll copy it back once we've
	 * returned from the call to secure world.
	 */
	if (shm->shadow_buffer && param_type != TEEC_MEMREF_PARTIAL_OUTPUT)
		memcpy((char *)shm->shadow_buffer + memref->offset,
				(char *)shm->buffer + memref->offset, memref->size);

	param->u.memref.shm = shm->shadow_buffer;
	param->u.memref.shm_offs = memref->offset;
	param->u.memref.size = memref->size;
	return 0;
}

static int comm_teec_pre_proc_operation(
		struct TEEC_Context *ctx,
		struct TEEC_Operation *operation,
		struct tee_param *params,
		struct TEEC_SharedMemory *shms)
{
	int ret = 0;
	size_t n = 0;
	uint32_t param_type = 0;

	if (!operation) {
		memset(params, 0, sizeof(struct tee_param) * TEEC_CONFIG_PAYLOAD_REF_COUNT);
		return ret;
	}

	for (n = 0; n < TEEC_CONFIG_PAYLOAD_REF_COUNT; n++) {
		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);

		switch (param_type) {
		case TEEC_NONE:
			params[n].attr = param_type;
			break;
		case TEEC_VALUE_INPUT:
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
			params[n].attr = param_type;
			params[n].u.value.a = operation->params[n].value.a;
			params[n].u.value.b = operation->params[n].value.b;
			break;
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			ret = comm_teec_pre_proc_tmpref(ctx, param_type,
				&(operation->params[n].tmpref), params + n, shms + n);
			break;
		case TEEC_MEMREF_WHOLE:
			ret = comm_teec_pre_proc_whole(
					&operation->params[n].memref, params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			ret = comm_teec_pre_proc_partial(param_type,
					&operation->params[n].memref, params + n);
			break;
		default:
			ret = -EINVAL;
			comm_drv_debug("invalid param_type : %x\n", param_type);
			break;
		}
	}

	return ret;
}

static int comm_teec_invoke_cmd(struct comm_teec_context *pstOpteeCtx,
		struct TEEC_Session *session, u32 cmd_id,
		struct TEEC_Operation *operation, u32 *error_origin)
{
	int ret = -EPERM;
	int otpee_version = comm_get_optee_version(pstOpteeCtx);
	uint32_t err_ori = 0;
	uint64_t buf[(sizeof(struct tee_ioctl_invoke_arg) +
			TEEC_CONFIG_PAYLOAD_REF_COUNT *
			sizeof(struct tee_param)) / sizeof(uint64_t)] = { 0 };
	struct tee_ioctl_invoke_arg *arg = NULL;
	struct tee_param *params = NULL;
	struct TEEC_SharedMemory shm[TEEC_CONFIG_PAYLOAD_REF_COUNT] = { 0 };

	if (!session) {
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "session is NULL!!!\n");
		return -EPERM;
	}
	if (otpee_version == 3) {
		arg = (struct tee_ioctl_invoke_arg *)buf;
		arg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;
		params = (struct tee_param *)(arg + 1);
		arg->session = session->session_id;
		arg->func = cmd_id;

		if (operation)
			operation->session = session;

		ret = comm_teec_pre_proc_operation(&(session->ctx), operation, params, shm);
		if (ret) {
			//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "teec pre process failed!\n");
			err_ori = TEEC_ORIGIN_API;
			goto out_free_temp_refs;
		}

		ret = tee_client_invoke_func((struct tee_context *)session->ctx.fd, arg, params);
		if (ret) {
			//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "TEE_IOC_INVOKE failed!\n");
			err_ori = TEEC_ORIGIN_COMMS;
			goto out_free_temp_refs;
		}

		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_SVP, "tee client invoke cmd success.\n");

		ret = arg->ret;
		err_ori = arg->ret_origin;
		comm_teec_post_proc_operation(operation, params, shm);

out_free_temp_refs:
		comm_teec_free_temp_refs(operation, shm);

		if (error_origin)
			*error_origin = err_ori;
	} else {
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR,
		//	"Not support OPTEE version (%d)\n", otpee_version);
	}

	return ret;
}

static void comm_teec_close_session(struct comm_teec_context *pstOpteeCtx,
		struct TEEC_Session *session)
{
	int otpee_version = comm_get_optee_version(pstOpteeCtx);

	if (!session)
		return;
	if (!session->ctx.fd)
		return;

	if (otpee_version == 3)
		tee_client_close_session((struct tee_context *)session->ctx.fd,
				session->session_id);
	else
		comm_drv_debug("[%s] Not support OPTEE version:%d\n", __func__, otpee_version);
}

static int comm_teec_open_session(struct comm_teec_context *pstOpteeCtx,
		struct TEEC_Context *context,
		struct TEEC_Session *session,
		const struct TEEC_UUID *destination,
		struct TEEC_Operation *operation,
		u32 *error_origin)
{
	int otpee_version = comm_get_optee_version(pstOpteeCtx);

	if ((!session) || (!context))
		return -EPERM;

	if (otpee_version == 3) {
		struct tee_ioctl_open_session_arg arg;
		struct tee_param param;
		int rc;

		memset(&arg, 0, sizeof(struct tee_ioctl_open_session_arg));
		memset(&param, 0, sizeof(struct tee_param));
		uuid_to_octets(arg.uuid, destination);
		arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
		arg.num_params = 0;
		if (operation != NULL)
			param.attr = operation->paramTypes;

		rc = tee_client_open_session(context->ctx, &arg, (struct tee_param *) &param);
		if ((rc == 0) && (arg.ret == 0)) {
			session->session_id = arg.session;
			session->ctx.fd = (uintptr_t)context->ctx;
			if (operation != NULL) {
				operation->params[0].value.a = param.u.value.a;
				operation->params[0].value.b = param.u.value.b;
			}
			return 0;
		}

		if (error_origin != NULL)
			*error_origin = arg.ret_origin;

	} else {
		comm_drv_debug("[%s] Not support OPTEE version:%d\n", __func__, otpee_version);
	}

	return -EPERM;
}


static void comm_teec_finalize_context(struct comm_teec_context *pstOpteeCtx,
		struct TEEC_Context *context)
{
	int otpee_version = comm_get_optee_version(pstOpteeCtx);

	if (!context) {
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR,
		//	"tee finalize context fail context is NULL!!!\n");
		return;
	}
	if (!context->ctx) {
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR,
		//	"tee finalize context fail context->ctx is NULL!!!\n");
		return;
	}

	if (otpee_version == 3)
		tee_client_close_context(context->ctx);
	else {
		//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR,
		//	"Not support OPTEE version (%d)\n", otpee_version);
	}
}

static int comm_teec_init_context(struct comm_teec_context *pstOpteeCtx,
		struct TEEC_Context *pstContext)
{
	if (comm_get_optee_version(pstOpteeCtx) == 3) {
		struct tee_ioctl_version_data vers = {
			.impl_id = TEE_OPTEE_CAP_TZ,
			.impl_caps = TEE_IMPL_ID_OPTEE,
			.gen_caps = TEE_GEN_CAP_GP,
		};

		pstContext->ctx = tee_client_open_context(NULL, _optee_match, NULL, &vers);

		if (IS_ERR(pstContext->ctx)) {
			//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "context is NULL!\n");
			return -EPERM;
		}
		return 0;
	}

	//STI_MDLA_LOG(STI_MDLA_LOG_LEVEL_ERROR, "Not support OPTEE version (%d)\n",
	//			comm_get_optee_version(dev));
	return -EPERM;
}

static int comm_teec_enable(struct comm_teec_context *pstOpteeCtx, struct TEEC_UUID *uuid)
{
	u32 tee_error = 0;
	int ret = 0;
	struct TEEC_Context *pstContext = &pstOpteeCtx->stContext;
	struct TEEC_Session *pstSession = &pstOpteeCtx->stSession;

	ret = comm_teec_init_context(pstOpteeCtx, pstContext);
	if (ret) {
		comm_drv_debug("[%s] Init Context failed!\n", __func__);
		comm_teec_finalize_context(pstOpteeCtx, pstContext);
		return -EPERM;
	}

	ret = comm_teec_open_session(pstOpteeCtx, pstContext, pstSession, uuid,
			NULL, &tee_error);
	if (ret) {
		comm_drv_debug("[%s] TEEC Open pstSession failed with error(%u)\n",
				__func__, tee_error);
		comm_teec_close_session(pstOpteeCtx, pstSession);
		comm_teec_finalize_context(pstOpteeCtx, pstContext);
		return -EPERM;
	}
	comm_drv_debug("[%s] Enable MDLA teec success.\n", __func__);

	return 0;
}

static void comm_teec_disable(struct comm_teec_context *pstOpteeCtx)
{
	struct TEEC_Context *pstContext = &pstOpteeCtx->stContext;
	struct TEEC_Session *pstSession = &pstOpteeCtx->stSession;

	comm_teec_close_session(pstOpteeCtx, pstSession);
	comm_teec_finalize_context(pstOpteeCtx, pstContext);
}

int comm_teec_open_context(u32 uuid_idx)
{
	struct comm_teec_context *pstTeeCtx;

	if (uuid_idx >= MAX_UUID_IDX)
		return -EFAULT;

	pstTeeCtx = &teec_ctx[uuid_idx];
	if (!pstTeeCtx->initialized) {
		pstTeeCtx->optee_version = 3;
		pstTeeCtx->svp_enable = 0;

		if (comm_teec_enable(pstTeeCtx, &uuid_list[uuid_idx])) {
			comm_drv_debug("[%s] teec enable fail\n", __func__);
			return -EPERM;
		}

		pstTeeCtx->initialized = 1;
	}

	return 0;
}

void comm_teec_close_context(u32 uuid_idx)
{
	struct TEEC_Context *pstContext;
	struct TEEC_Session *pstSession;
	struct comm_teec_context *pstTeeCtx;

	if (uuid_idx >= MAX_UUID_IDX)
		return;

	pstTeeCtx = &teec_ctx[uuid_idx];
	pstSession = &pstTeeCtx->stSession;

	if (pstTeeCtx->initialized == 0)
		return;


	comm_teec_disable(pstTeeCtx);
	pstContext = &pstTeeCtx->stContext;
	pstSession = &pstTeeCtx->stSession;

	memset(pstContext, 0, sizeof(*pstContext));
	memset(pstSession, 0, sizeof(*pstSession));
	pstTeeCtx->initialized = 0;
}

int comm_teec_send_command(u32 uuid_idx, u32 cmd_id, struct TEEC_Operation *op, u32 *error)
{
	int ret = 0;
	struct comm_teec_context *pstTeeCtx;
	struct TEEC_Session *pstSession;

	if (uuid_idx >= MAX_UUID_IDX)
		return -EFAULT;

	pstTeeCtx = &teec_ctx[uuid_idx];
	pstSession = &pstTeeCtx->stSession;
	if (!pstTeeCtx->initialized) {
		if (comm_teec_open_context(uuid_idx)) {
			comm_err("[%s] open context fail\n", __func__);
			return -EFAULT;
		}
	}

	ret = comm_teec_invoke_cmd(pstTeeCtx, pstSession, cmd_id, op, error);
	if (ret) {
		comm_err("[%s] invoke fail, error(%u)\n", __func__, *error);
		ret = -EPERM;
	}

	return ret;
}
