// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#if IS_ENABLED(CONFIG_OPTEE)
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
#include <linux/videodev2.h>
#include <linux/err.h>
#include <asm/uaccess.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#include "mtk_iommu_dtv_api.h"
#include <uapi/linux/mtk_dip.h>
#include "mtk_dip_priv.h"
#include "mtk_dip_svc.h"

#define DIP_SVC_INFO(args...)	(pr_info("[DIP SVC INFO]" args))
#define DIP_SVC_ERR(args...)	(pr_crit("[DIP SVC ERR]" args))

#define SHM_LEN      (8)

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

static int _mtk_dip_get_optee_version(struct dip_ctx *ctx)
{
#if (DYNAMIC_OPTEE_VERSION_SUPPORT)
	DIP_SVC_ERR("Not Support DYNAMIC OPTEE VERSION\n");
#endif
	return ctx->secure_info.optee_version;
}

static void _mtk_dip_teec_free_temp_refs(struct TEEC_Operation *operation,
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
			free_shm->buffer_allocated = false;
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
			DIP_SVC_ERR("invalid param_type : %x\n", param_type);
			break;
		}
	}
}

int _mtk_dip_teec_alloc_shm(struct TEEC_Context *ctx, struct TEEC_SharedMemory *shm)
{
	struct tee_shm *_shm = NULL;
	void *shm_va = NULL;
	size_t len = shm->size;
	struct tee_context *context = (struct tee_context *) ctx->fd;

	if (!len)
		len = SHM_LEN;

	_shm = tee_shm_alloc(context, len, TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
	if (!_shm) {
		DIP_SVC_ERR("tee shm alloc got NULL\n");
		return -ENOMEM;
	}
	if (IS_ERR(_shm)) {
		DIP_SVC_ERR("tee shm alloc fail\n");
		return -ENOMEM;
	}

	shm_va = tee_shm_get_va(_shm, 0);
	if (!shm_va) {
		DIP_SVC_ERR("tee shm get va got NULL\n");
		return -ENOMEM;
	}

	if (IS_ERR(shm_va)) {
		DIP_SVC_ERR("tee shm get va fail\n");
		return -ENOMEM;
	}

	shm->buffer = shm_va;
	shm->shadow_buffer = (void *)_shm;
	shm->alloced_size = len;
	shm->registered_fd = -1;
	shm->buffer_allocated = true;
	return 0;
}

static void _mtk_dip_teec_post_proc_tmpref(uint32_t param_type,
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

#if (MULTI_INVOKE_TYPE_SUPPORT)
static void _mtk_dip_teec_post_proc_whole(struct TEEC_RegisteredMemoryReference *memref,
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

static void _mtk_dip_teec_post_proc_partial(uint32_t param_type,
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
#endif

static void _mtk_dip_teec_post_proc_operation(struct TEEC_Operation *operation,
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
			break;
#if (MULTI_INVOKE_TYPE_SUPPORT)
		case TEEC_VALUE_INPUT:
			break;
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
			operation->params[n].value.a = params[n].u.value.a;
			operation->params[n].value.b = params[n].u.value.b;
			break;
		case TEEC_MEMREF_WHOLE:
			_mtk_dip_teec_post_proc_whole(&operation->params[n].memref,
						params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			_mtk_dip_teec_post_proc_partial(param_type,
				&operation->params[n].memref, params + n);
			break;
#endif
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			_mtk_dip_teec_post_proc_tmpref(param_type,
				&operation->params[n].tmpref, params + n, shms + n);
			break;
		default:
			DIP_SVC_ERR("invalid param_type : %x\n", param_type);
			break;
		}
	}
}

static int _mtk_dip_teec_pre_proc_tmpref(struct TEEC_Context *ctx,
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
		DIP_SVC_ERR("invalid param_type : %x\n", param_type);
		return -EINVAL;
	}
	shm->size = tmpref->size;

	ret = _mtk_dip_teec_alloc_shm(ctx, shm);
	if (ret) {
		DIP_SVC_ERR("share mem alloc fail\n");
		return -EPERM;
	}

	memcpy(shm->buffer, tmpref->buffer, tmpref->size);
	param->u.memref.size = tmpref->size;
	//Workaround, shm->shadow_buffer is shm object.
	param->u.memref.shm = (struct tee_shm *) shm->shadow_buffer;
	return ret;
}

#if (MULTI_INVOKE_TYPE_SUPPORT)
static int _mtk_dip_teec_pre_proc_whole(
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

static int _mtk_dip_teec_pre_proc_partial(uint32_t param_type,
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
		DIP_SVC_ERR("invalid param_type : %x\n", param_type);
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
#endif

static int _mtk_dip_teec_pre_proc_operation(
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
#if (MULTI_INVOKE_TYPE_SUPPORT)
		case TEEC_VALUE_INPUT:
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
			params[n].attr = param_type;
			params[n].u.value.a = operation->params[n].value.a;
			params[n].u.value.b = operation->params[n].value.b;
			break;
		case TEEC_MEMREF_WHOLE:
			ret = _mtk_dip_teec_pre_proc_whole(
					&operation->params[n].memref, params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			ret = _mtk_dip_teec_pre_proc_partial(param_type,
				&operation->params[n].memref, params + n);
			break;
#endif
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			ret = _mtk_dip_teec_pre_proc_tmpref(ctx, param_type,
				&(operation->params[n].tmpref), params + n, shms + n);
			break;
		default:
			ret = -EINVAL;
			DIP_SVC_ERR("invalid param_type : %x\n", param_type);
			break;
		}
	}

	return ret;
}

static int _mtk_dip_teec_invoke_cmd(struct dip_ctx *ctx,
			struct TEEC_Session *session, u32 cmd_id,
			struct TEEC_Operation *pOperation, u32 *error_origin)
{
	int ret = -EPERM;
	int otpee_version = _mtk_dip_get_optee_version(ctx);
	uint32_t err_ori = 0;
	uint64_t buf[(sizeof(struct tee_ioctl_invoke_arg) +
			TEEC_CONFIG_PAYLOAD_REF_COUNT *
				sizeof(struct tee_param)) / sizeof(uint64_t)] = { 0 };
	struct tee_ioctl_invoke_arg *arg = NULL;
	struct tee_param *params = NULL;
	struct TEEC_SharedMemory shm[TEEC_CONFIG_PAYLOAD_REF_COUNT] = { 0 };

	if (!session) {
		DIP_SVC_ERR("session is NULL!!!\n");
		return -EPERM;
	}

	if (otpee_version == OPTEE_SUPPORT_VER) {
		arg = (struct tee_ioctl_invoke_arg *)buf;
		arg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;
		params = (struct tee_param *)(arg + 1);
		arg->session = session->session_id;
		arg->func = cmd_id;

		if (pOperation)
			pOperation->session = session;

		ret = _mtk_dip_teec_pre_proc_operation(&(session->ctx), pOperation, params, shm);
		if (ret) {
			DIP_SVC_ERR("teec pre process failed!\n");
			err_ori = TEEC_ORIGIN_API;
			goto out_free_temp_refs;
		}

		ret = tee_client_invoke_func((struct tee_context *)session->ctx.fd, arg, params);
		if (ret) {
			DIP_SVC_ERR("TEE_IOC_INVOKE failed!\n");
			err_ori = TEEC_ORIGIN_COMMS;
			goto out_free_temp_refs;
		}

		DIP_SVC_INFO("tee client invoke cmd success.\n");

		ret = arg->ret;
		err_ori = arg->ret_origin;
		_mtk_dip_teec_post_proc_operation(pOperation, params, shm);

out_free_temp_refs:
		_mtk_dip_teec_free_temp_refs(pOperation, shm);

		if (error_origin)
			*error_origin = err_ori;

	} else {
		DIP_SVC_ERR(
			"Not support OPTEE version (%d)\n", otpee_version);
	}

	return ret;
}

static void _mtk_dip_teec_close_session(struct dip_ctx *ctx,
			struct TEEC_Session *session)
{
	int otpee_version = _mtk_dip_get_optee_version(ctx);

	if (!session) {
		DIP_SVC_ERR("tee close fail session is NULL!!!\n");
		return;
	}
	if (!session->ctx.fd) {
		DIP_SVC_ERR("tee close fail session->ctx.fd is NULL!!!\n");
		return;
	}

	if (otpee_version == OPTEE_SUPPORT_VER)
		tee_client_close_session((struct tee_context *)session->ctx.fd,
			session->session_id);
	else
		DIP_SVC_ERR(
			"Not support OPTEE version (%d)\n", otpee_version);
}

static int _mtk_dip_teec_open_session(struct dip_ctx *ctx,
			struct TEEC_Context *context,
			struct TEEC_Session *session,
			const struct TEEC_UUID *destination,
			struct TEEC_Operation *pOperation,
			u32 *error_origin)
{
	int otpee_version = _mtk_dip_get_optee_version(ctx);
	int rc;
	struct tee_ioctl_open_session_arg arg;
	struct tee_param param;
	int ret = 0;

	if ((session == NULL) || (context == NULL)) {
		DIP_SVC_ERR("%s, session or context is NULL!!!\n", __func__);
		return -EPERM;
	}
	if (destination == NULL) {
		DIP_SVC_ERR("%s, destination is NULL!!!\n", __func__);
		return -EPERM;
	}

	if (otpee_version == OPTEE_SUPPORT_VER) {
		memset(&arg, 0, sizeof(struct tee_ioctl_open_session_arg));
		memset(&param, 0, sizeof(struct tee_param));
		uuid_to_octets(arg.uuid, destination);
		arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
		arg.num_params = 0;
		if (pOperation != NULL)
			param.attr = pOperation->paramTypes;
		rc = tee_client_open_session(context->ctx, &arg, (struct tee_param *) &param);
		if ((rc == 0) && (arg.ret == 0)) {
			session->session_id = arg.session;
			session->ctx.fd = (uintptr_t)context->ctx;
			if (pOperation != NULL) {
				pOperation->params[0].value.a = param.u.value.a;
				pOperation->params[0].value.b = param.u.value.b;
			}
		} else {
			if (error_origin != NULL)
				*error_origin = arg.ret_origin;
			DIP_SVC_ERR(
				"tee open failed! rc=0x%08x, err=0x%08x, ori=0x%08x\n",
				rc, arg.ret, arg.ret_origin);
			ret = -EPERM;
		}
	} else {
		DIP_SVC_ERR(
			"Not support OPTEE version (%d)\n", otpee_version);
		ret = -EPERM;
	}

	return ret;
}

static void _mtk_dip_teec_finalize_context(struct dip_ctx *ctx,
			struct TEEC_Context *context)
{
	int otpee_version = _mtk_dip_get_optee_version(ctx);

	if (!context) {
		DIP_SVC_ERR(
			"tee finalize context fail context is NULL!!!\n");
		return;
	}
	if (!context->ctx) {
		DIP_SVC_ERR(
			"tee finalize context fail context->ctx is NULL!!!\n");
		return;
	}

	if (otpee_version == OPTEE_SUPPORT_VER)
		tee_client_close_context(context->ctx);
	else
		DIP_SVC_ERR(
			"Not support OPTEE version (%d)\n", otpee_version);
}

static int _mtk_dip_teec_init_context(struct dip_ctx *ctx,
			struct TEEC_Context *context)
{
	int version = 0;
	int ret = 0;

	version = _mtk_dip_get_optee_version(ctx);
	if (version == OPTEE_SUPPORT_VER) {
		struct tee_ioctl_version_data vers = {
			.impl_id = TEE_OPTEE_CAP_TZ,
			.impl_caps = TEE_IMPL_ID_OPTEE,
			.gen_caps = TEE_GEN_CAP_GP,
		};
		context->ctx = tee_client_open_context(NULL, _optee_match, NULL, &vers);
		if (IS_ERR(context->ctx)) {
			DIP_SVC_ERR("context is NULL!\n");
			ret = -EPERM;
		}
	} else {
		DIP_SVC_ERR("Not support OPTEE version (%d)\n", version);
		ret = -EPERM;
	}

	return ret;
}

static int _mtk_dip_teec_enable(struct dip_ctx *ctx)
{
	u32 tee_error = 0;
	int ret = 0;
	struct TEEC_UUID uuid = INTERNAL_DIP_UUID;
	struct TEEC_Context *pstContext = ctx->secure_info.pstContext;
	struct TEEC_Session *pstSession = ctx->secure_info.pstSession;

	if ((pstSession != NULL) && (pstContext != NULL)) {
		ret = _mtk_dip_teec_init_context(ctx, pstContext);
		if (ret) {
			DIP_SVC_ERR("Init Context failed!\n");
			_mtk_dip_teec_finalize_context(ctx, pstContext);
			return -EPERM;
		}
		DIP_SVC_INFO("Init DIP teec context Success\n");

		ret = _mtk_dip_teec_open_session(ctx, pstContext, pstSession, &uuid,
					NULL, &tee_error);
		if (ret) {
			DIP_SVC_ERR("TEEC Open pstSession failed with error(%u)\n", tee_error);
			_mtk_dip_teec_close_session(ctx, pstSession);
			_mtk_dip_teec_finalize_context(ctx, pstContext);
			return -EPERM;
		}
		DIP_SVC_INFO("Open DIP teec session Success.\n");
	} else {
		DIP_SVC_ERR("pstSession or pstContext is NULL.\n");
		return -EPERM;
	}

	return 0;
}

static void _mtk_dip_teec_disable(struct dip_ctx *ctx)
{
	struct TEEC_Context *pstContext = ctx->secure_info.pstContext;
	struct TEEC_Session *pstSession = ctx->secure_info.pstSession;

	if ((pstContext != NULL) && (pstSession != NULL)) {
		_mtk_dip_teec_close_session(ctx, pstSession);
		_mtk_dip_teec_finalize_context(ctx, pstContext);
		DIP_SVC_INFO("Disable dip teec success.\n");
	} else {
		DIP_SVC_ERR("pstSession or pstContext is NULL.\n");
	}
}

static int _mtk_dip_teec_smc_call(struct dip_ctx *ctx,
			EN_DIP_REE_TO_TEE_CMD_TYPE cmd, void *para, u32 size)
{
	u32 error = 0;
	int ret = 0;
	struct TEEC_Operation Operation = {0};
	struct TEEC_Session *pstSession = ctx->secure_info.pstSession;

	if (pstSession == NULL) {
		DIP_SVC_ERR("pstSession is NULL!\n");
		return -EPERM;
	}
	DIP_SVC_INFO("DIP SMC cmd:%u\n", cmd);
	Operation.params[0].tmpref.buffer = para;
	Operation.params[0].tmpref.size = size;
	Operation.paramTypes =
		TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	ret = _mtk_dip_teec_invoke_cmd(ctx, pstSession, cmd, &Operation, &error);
	if (ret) {
		ret = -EPERM;
		DIP_SVC_ERR("SMC call failed with error(%u)\n", error);
	}

	return ret;
}

EN_DIP_SRC _MappingSrc(EN_DIP_SOURCE enSource)
{
	EN_DIP_SRC enSrc = E_DIP_SRC_SRCCAP_MAIN;

	switch (enSource) {
	case E_DIP_SOURCE_SRCCAP_MAIN:
		enSrc = E_DIP_SRC_SRCCAP_MAIN;
		break;
	case E_DIP_SOURCE_SRCCAP_SUB:
		enSrc = E_DIP_SRC_SRCCAP_SUB;
		break;
	case E_DIP_SOURCE_PQ_START:
		enSrc = E_DIP_SRC_PQ_START;
		break;
	case E_DIP_SOURCE_PQ_HDR:
		enSrc = E_DIP_SRC_PQ_HDR;
		break;
	case E_DIP_SOURCE_PQ_SHARPNESS:
		enSrc = E_DIP_SRC_PQ_SHARPNESS;
		break;
	case E_DIP_SOURCE_PQ_SCALING:
		enSrc = E_DIP_SRC_PQ_SCALING;
		break;
	case E_DIP_SOURCE_PQ_CHROMA_COMPENSAT:
		enSrc = E_DIP_SRC_PQ_CHROMA_COMPENSAT;
		break;
	case E_DIP_SOURCE_PQ_BOOST:
		enSrc = E_DIP_SRC_PQ_BOOST;
		break;
	case E_DIP_SOURCE_PQ_COLOR_MANAGER:
		enSrc = E_DIP_SRC_PQ_COLOR_MANAGER;
		break;
	case E_DIP_SOURCE_DIPR:
		enSrc = E_DIP_SRC_DIPR;
		break;
	case E_DIP_SOURCE_B2R:
		enSrc = E_DIP_SRC_B2R;
		break;
	default:
		enSrc = E_DIP_SRC_MAX;
		DIP_SVC_ERR("invalid source: %x\n", enSrc);
		break;
	}

	return enSrc;
}

int mtk_dip_svc_enable(struct dip_ctx *ctx, u8 u8Enable)
{
	ST_DIP_SVC_INFO stSvcInfo;
	EN_DIP_REE_TO_TEE_CMD_TYPE cmd = E_DIP_REE_TO_TEE_CMD_BIND_PIPEID;
	EN_DIP_SRC enSrc = E_DIP_SRC_SRCCAP_MAIN;
	int ret = 0;
	u32 u32Idx = 0;

	if (ctx == NULL) {
		DIP_SVC_ERR("Received NULL pointer!\n");
		return -EINVAL;
	}

	DIP_SVC_INFO("Capture SVC %s\n", (u8Enable ? "ENABLE":"DISABLE"));

	enSrc = _MappingSrc(ctx->enSource);
	if (enSrc == E_DIP_SRC_MAX) {
		DIP_SVC_ERR("[%s]Source:%d Not Support Secure capture\n",
			__func__, ctx->enSource);
		return -EINVAL;
	}

	memset(&stSvcInfo, 0, sizeof(ST_DIP_SVC_INFO));

	/* destroy disp pipeline */
	stSvcInfo.u32version = MTK_DIP_SVC_INFO_VERSION;
	stSvcInfo.u32length = sizeof(ST_DIP_SVC_INFO);
	stSvcInfo.enDIP = ctx->dev->variant->u16DIPIdx;
	stSvcInfo.u8Enable = u8Enable;
	stSvcInfo.enSrc = enSrc;
	stSvcInfo.u32OutWidth = ctx->out.u32OutWidth;
	stSvcInfo.u32OutHeight = ctx->out.u32OutHeight;
	stSvcInfo.u32CropWidth = ctx->out.c_width;
	stSvcInfo.u32CropHeight = ctx->out.c_height;
	stSvcInfo.u8InAddrshift = ctx->secure_info.u8InAddrshift;
	stSvcInfo.u32PlaneCnt = ctx->secure_info.u32PlaneCnt;
	for (u32Idx = 0; u32Idx < IN_PLANE_MAX; u32Idx++) {
		if (u32Idx < stSvcInfo.u32PlaneCnt)
			stSvcInfo.u64InputAddr[u32Idx] = ctx->secure_info.u64InputAddr[u32Idx];
		else
			stSvcInfo.u64InputAddr[u32Idx] = 0;
	}
	stSvcInfo.u32BufCnt = ctx->secure_info.u32BufCnt;
	for (u32Idx = 0; u32Idx < IN_PLANE_MAX; u32Idx++) {
		if (u32Idx < stSvcInfo.u32BufCnt)
			stSvcInfo.u32InputSize[u32Idx] = ctx->secure_info.u32InputSize[u32Idx];
		else
			stSvcInfo.u32InputSize[u32Idx] = 0;
	}
	if ((stSvcInfo.enSrc == E_DIP_SRC_DIPR) ||
		(stSvcInfo.enSrc == E_DIP_SRC_B2R)) {
		stSvcInfo.u32InputXStr = ctx->in.o_width;
		stSvcInfo.u32InputYStr = ctx->in.o_height;
		stSvcInfo.u32InputWidth = ctx->in.width;
		stSvcInfo.u32InputHeight = ctx->in.height;
	} else {
		stSvcInfo.u32InputXStr = 0;
		stSvcInfo.u32InputYStr = 0;
		stSvcInfo.u32InputWidth = ctx->stSrcInfo.u32Width;
		stSvcInfo.u32InputHeight = ctx->stSrcInfo.u32Height;
	}
	DIP_SVC_INFO("[%s] u32Version:%d\n", __func__, stSvcInfo.u32version);
	DIP_SVC_INFO("[%s] u32Length:%d\n", __func__, stSvcInfo.u32length);
	DIP_SVC_INFO("[%s] enDIP:%d\n", __func__, stSvcInfo.enDIP);
	DIP_SVC_INFO("[%s] u8Enable:%d\n", __func__, stSvcInfo.u8Enable);
	DIP_SVC_INFO("[%s] enSrc:%d\n", __func__, stSvcInfo.enSrc);
	DIP_SVC_INFO("[%s] u32OutWidth:%d\n", __func__, stSvcInfo.u32OutWidth);
	DIP_SVC_INFO("[%s] u32OutHeight:%d\n", __func__, stSvcInfo.u32OutHeight);
	DIP_SVC_INFO("[%s] u32CropWidth:%d\n", __func__, stSvcInfo.u32CropWidth);
	DIP_SVC_INFO("[%s] u32PlaneCnt:%d\n", __func__, stSvcInfo.u32PlaneCnt);
	for (u32Idx = 0; u32Idx < IN_PLANE_MAX; u32Idx++) {
		DIP_SVC_INFO("[%s] u64InputAddr[%d]:0x%llx\n", __func__,
		u32Idx, stSvcInfo.u64InputAddr[u32Idx]);
	}
	DIP_SVC_INFO("[%s] u32BufCnt:%d\n", __func__, stSvcInfo.u32BufCnt);
	for (u32Idx = 0; u32Idx < IN_PLANE_MAX; u32Idx++) {
		DIP_SVC_INFO("[%s] u32InputSize[%d]:0x%lx\n", __func__,
		u32Idx, stSvcInfo.u32InputSize[u32Idx]);
	}
	DIP_SVC_INFO("[%s] u32InputXStr:%d\n", __func__, stSvcInfo.u32InputXStr);
	DIP_SVC_INFO("[%s] u32InputYStr:%d\n", __func__, stSvcInfo.u32InputYStr);
	DIP_SVC_INFO("[%s] u32InputWidth:%d\n", __func__, stSvcInfo.u32InputWidth);
	DIP_SVC_INFO("[%s] u32InputHeight:%d\n", __func__, stSvcInfo.u32InputHeight);
	cmd = E_DIP_REE_TO_TEE_CMD_SDC_SETTING;
	DIP_SVC_INFO("[%s] cmd:%d\n", __func__, cmd);
	ret = _mtk_dip_teec_smc_call(ctx, cmd,
				(void *)&stSvcInfo,
				sizeof(ST_DIP_SVC_INFO));
	if (ret) {
		DIP_SVC_ERR("smc call fail\n");
		return ret;
	}

	return ret;
}

int mtk_dip_svc_init(struct dip_ctx *ctx)
{
	struct TEEC_Context *pstContext = NULL;
	struct TEEC_Session *pstSession = NULL;
	int ret = 0;

	if (ctx == NULL) {
		DIP_SVC_ERR("Received NULL pointer!\n");
		return -EINVAL;
	}

	pstContext = kzalloc(sizeof(struct TEEC_Context), GFP_KERNEL);
	if (pstContext == NULL) {
		DIP_SVC_ERR("pstContext malloc fail!\n");
		return -EPERM;
	}

	pstSession = kzalloc(sizeof(struct TEEC_Session), GFP_KERNEL);
	if (pstSession == NULL) {
		DIP_SVC_ERR("pstSession malloc fail!\n");
		kfree(pstContext);
		return -EPERM;
	}

	memset(pstContext, 0, sizeof(struct TEEC_Context));
	memset(pstSession, 0, sizeof(struct TEEC_Session));

	if (ctx->secure_info.pstContext != NULL) {
		kfree(ctx->secure_info.pstContext);
		ctx->secure_info.pstContext = NULL;
	}

	if (ctx->secure_info.pstSession != NULL) {
		kfree(ctx->secure_info.pstSession);
		ctx->secure_info.pstSession = NULL;
	}

	memset(&ctx->secure_info, 0, sizeof(struct dip_secure_info));
	ctx->secure_info.optee_version = OPTEE_SUPPORT_VER;
	ctx->secure_info.pstContext = pstContext;
	ctx->secure_info.pstSession = pstSession;

	ret = _mtk_dip_teec_enable(ctx);
	if (ret) {
		DIP_SVC_ERR("DIP teec enable fail\n");
		if (ctx->secure_info.pstContext != NULL) {
			kfree(ctx->secure_info.pstContext);
			ctx->secure_info.pstContext = NULL;
		}
		if (ctx->secure_info.pstSession != NULL) {
			kfree(ctx->secure_info.pstSession);
			ctx->secure_info.pstSession = NULL;
		}
		return -EPERM;
	}
	DIP_SVC_INFO("DIP teec init success.\n");

	return 0;
}

void mtk_dip_svc_exit(struct dip_ctx *ctx)
{
	struct TEEC_Context *pstContext = NULL;
	struct TEEC_Session *pstSession = NULL;

	if (ctx == NULL) {
		DIP_SVC_ERR("Received NULL pointer!\n");
		return;
	}
	mtk_dip_svc_enable(ctx, 0);
	_mtk_dip_teec_disable(ctx);

	pstContext = ctx->secure_info.pstContext;
	pstSession = ctx->secure_info.pstSession;

	if (pstContext != NULL) {
		kfree(pstContext);
		ctx->secure_info.pstContext = NULL;
	}

	if (pstSession != NULL) {
		kfree(pstSession);
		ctx->secure_info.pstSession = NULL;
	}
}

#endif // IS_ENABLED(CONFIG_OPTEE)
