// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifdef CONFIG_OPTEE
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

#include "mtk_pq.h"
#include "mtk_iommu_dtv_api.h"

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

static int _mtk_pq_get_optee_version(struct mtk_pq_device *pq_dev)
{
	if (pq_dev->xc_info.secure_info.optee_version != -1)
		goto final;

	if (pq_dev->xc_info.secure_info.optee_version == -1) {
		struct file *verfile = NULL;
		char *sVer = NULL;
		int u32ReadCount = 0;
		mm_segment_t cur_mm_seg;
		long lFileSize;
		loff_t pos;

		sVer = malloc(sizeof(char) * BOOTARG_SIZE);
		if (sVer == NULL)
			goto final;

		cur_mm_seg = get_fs();
		set_fs(KERNEL_DS);
		verfile = filp_open("/proc/tz2_mstar/version", O_RDONLY, 0);
		if (verfile == NULL || IS_ERR_OR_NULL(verfile)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"[PQ SVP] can't open /proc/tz2_mstar/version\n");
			pq_dev->xc_info.secure_info.optee_version = 1;
			set_fs(cur_mm_seg);
			kfree(sVer);
			goto final;
		}

		pos = vfs_llseek(verfile, 0, SEEK_SET);
		u32ReadCount = kernel_read(verfile, sVer, BOOTARG_SIZE, &pos);
		set_fs(cur_mm_seg);

		if (u32ReadCount > BOOTARG_SIZE) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"[PQ SVP] cmdline info more than buffer size\n");
			goto exit;
		}

		sVer[BOOTARG_SIZE - 2] = '\n';
		sVer[BOOTARG_SIZE - 1] = '\0';

		if (strncmp(sVer, "2.4", 2) == 0)
			pq_dev->xc_info.secure_info.optee_version = 2;
		else if (strncmp(sVer, "3.2", 2) == 0)
			pq_dev->xc_info.secure_info.optee_version = 3;
		else
			pq_dev->xc_info.secure_info.optee_version = 1;

exit:
		if (verfile != NULL) {
			cur_mm_seg = get_fs();
			set_fs(KERNEL_DS);
			filp_close(verfile, NULL);
			set_fs(cur_mm_seg);
		}
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "OPTEE Version %s OPTEE_VERSION %d\n",
				sVer, pq_dev->xc_info.secure_info.optee_version);
		kfree(sVer);
	}

final:
	return pq_dev->xc_info.secure_info.optee_version;
}

static void _mtk_pq_teec_free_temp_refs(struct TEEC_Operation *operation,
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
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
			break;
		}
	}
}

int _mtk_pq_teec_alloc_shm(struct TEEC_Context *ctx, struct TEEC_SharedMemory *shm)
{
	struct tee_shm *_shm = NULL;
	void *shm_va = NULL;
	size_t len = shm->size;
	struct tee_context *context = (struct tee_context *) ctx->fd;

	if (!len)
		len = 8;
	if (len < PAGE_SIZE)
		_shm = tee_shm_alloc(context, len, TEE_SHM_MAPPED);
	else
		_shm = tee_shm_alloc(context, len, TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);

	if (!_shm) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee shm alloc got NULL\n");
		return -ENOMEM;
	}
	if (IS_ERR(_shm)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee shm alloc fail\n");
		return -ENOMEM;
	}

	shm_va = tee_shm_get_va(_shm, 0);
	if (!shm_va) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee shm get va got NULL\n");
		return -ENOMEM;
	}

	if (IS_ERR(shm_va)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee shm get va fail\n");
		return -ENOMEM;
	}

	shm->buffer = shm_va;
	shm->shadow_buffer = (void *)_shm;
	shm->alloced_size = len;
	shm->registered_fd = -1;
	shm->buffer_allocated = true;
	return 0;
}

static void _mtk_pq_teec_post_proc_tmpref(uint32_t param_type,
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

static void _mtk_pq_teec_post_proc_whole(struct TEEC_RegisteredMemoryReference *memref,
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

static void _mtk_pq_teec_post_proc_partial(uint32_t param_type,
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

static void _mtk_pq_teec_post_proc_operation(struct TEEC_Operation *operation,
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
			_mtk_pq_teec_post_proc_tmpref(param_type,
				&operation->params[n].tmpref, params + n, shms + n);
			break;
		case TEEC_MEMREF_WHOLE:
			_mtk_pq_teec_post_proc_whole(&operation->params[n].memref,
						params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			_mtk_pq_teec_post_proc_partial(param_type,
				&operation->params[n].memref, params + n);
			break;
		default:
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
			break;
		}
	}
}

static int _mtk_pq_teec_pre_proc_tmpref(struct TEEC_Context *ctx,
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
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
		return -EINVAL;
	}
	shm->size = tmpref->size;

	ret = _mtk_pq_teec_alloc_shm(ctx, shm);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "share mem alloc fail\n");
		return -EPERM;
	}

	memcpy(shm->buffer, tmpref->buffer, tmpref->size);
	param->u.memref.size = tmpref->size;
	//Workaround, shm->shadow_buffer is shm object.
	param->u.memref.shm = (struct tee_shm *) shm->shadow_buffer;
	return ret;
}

static int _mtk_pq_teec_pre_proc_whole(
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

static int _mtk_pq_teec_pre_proc_partial(uint32_t param_type,
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
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
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

static int _mtk_pq_teec_pre_proc_operation(
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
			ret = _mtk_pq_teec_pre_proc_tmpref(ctx, param_type,
				&(operation->params[n].tmpref), params + n, shms + n);
			break;
		case TEEC_MEMREF_WHOLE:
			ret = _mtk_pq_teec_pre_proc_whole(
					&operation->params[n].memref, params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			ret = _mtk_pq_teec_pre_proc_partial(param_type,
				&operation->params[n].memref, params + n);
			break;
		default:
			ret = -EINVAL;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "invalid param_type : %x\n", param_type);
			break;
		}
	}

	return ret;
}

static int _mtk_pq_teec_invoke_cmd(struct mtk_pq_device *pq_dev,
			struct TEEC_Session *session, u32 cmd_id,
			struct TEEC_Operation *operation, u32 *error_origin)
{
	int ret = -EPERM;
	int otpee_version = _mtk_pq_get_optee_version(pq_dev);
	uint32_t err_ori = 0;
	uint64_t buf[(sizeof(struct tee_ioctl_invoke_arg) +
			TEEC_CONFIG_PAYLOAD_REF_COUNT *
				sizeof(struct tee_param)) / sizeof(uint64_t)] = { 0 };
	struct tee_ioctl_invoke_arg *arg = NULL;
	struct tee_param *params = NULL;
	struct TEEC_SharedMemory shm[TEEC_CONFIG_PAYLOAD_REF_COUNT] = { 0 };

	if (!session) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "session is NULL!!!\n");
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

		ret = _mtk_pq_teec_pre_proc_operation(&(session->ctx), operation, params, shm);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "teec pre process failed!\n");
			err_ori = TEEC_ORIGIN_API;
			goto out_free_temp_refs;
		}

		ret = tee_client_invoke_func((struct tee_context *)session->ctx.fd, arg, params);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "TEE_IOC_INVOKE failed!\n");
			err_ori = TEEC_ORIGIN_COMMS;
			goto out_free_temp_refs;
		}

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "tee client invoke cmd success.\n");

		ret = arg->ret;
		err_ori = arg->ret_origin;
		_mtk_pq_teec_post_proc_operation(operation, params, shm);

out_free_temp_refs:
		_mtk_pq_teec_free_temp_refs(operation, shm);

		if (error_origin)
			*error_origin = err_ori;
	} else
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Not support OPTEE version (%d)\n", otpee_version);

	return ret;
}

static void _mtk_pq_teec_close_session(struct mtk_pq_device *pq_dev,
			struct TEEC_Session *session)
{
	int otpee_version = _mtk_pq_get_optee_version(pq_dev);

	if (!session) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee close fail session is NULL!!!\n");
		return;
	}
	if (!session->ctx.fd) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee close fail session->ctx.fd is NULL!!!\n");
		return;
	}

	if (otpee_version == 3)
		tee_client_close_session((struct tee_context *)session->ctx.fd,
			session->session_id);
	else
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Not support OPTEE version (%d)\n", otpee_version);
}

static int _mtk_pq_teec_open_session(struct mtk_pq_device *pq_dev,
			struct TEEC_Context *context,
			struct TEEC_Session *session,
			const struct TEEC_UUID *destination,
			struct TEEC_Operation *operation,
			u32 *error_origin)
{
	int otpee_version = _mtk_pq_get_optee_version(pq_dev);

	if ((!session) || (!context)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "tee open fail session or context is NULL!!!\n");
		return -EPERM;
	}
	if (otpee_version == 3) {
		struct tee_ioctl_open_session_arg arg;
		struct tee_param param;
		int rc;

		memset(&arg, 0, sizeof(struct tee_ioctl_open_session_arg));
		memset(&param, 0, sizeof(struct tee_param));
		uuid_to_octets(arg.uuid, destination);
		arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
		arg.num_params = 1;
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

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"tee open failed! rc=0x%08x, err=0x%08x, ori=0x%08x\n",
			rc, arg.ret, arg.ret_origin);
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Not support OPTEE version (%d)\n", otpee_version);
	}

	return -EPERM;
}


static void _mtk_pq_teec_finalize_context(struct mtk_pq_device *pq_dev,
			struct TEEC_Context *context)
{
	int otpee_version = _mtk_pq_get_optee_version(pq_dev);

	if (!context) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"tee finalize context fail context is NULL!!!\n");
		return;
	}
	if (!context->ctx) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"tee finalize context fail context->ctx is NULL!!!\n");
		return;
	}

	if (otpee_version == 3)
		tee_client_close_context(context->ctx);
	else
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Not support OPTEE version (%d)\n", otpee_version);
}

static int _mtk_pq_teec_init_context(struct mtk_pq_device *pq_dev,
			struct TEEC_Context *context)
{
	if (_mtk_pq_get_optee_version(pq_dev) == 3) {
		struct tee_ioctl_version_data vers = {
			.impl_id = TEE_OPTEE_CAP_TZ,
			.impl_caps = TEE_IMPL_ID_OPTEE,
			.gen_caps = TEE_GEN_CAP_GP,
		};

		context->ctx = tee_client_open_context(NULL, _optee_match, NULL, &vers);

		if (IS_ERR(context->ctx)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "context is NULL!\n");
			return -EPERM;
		}
		return 0;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Not support OPTEE version (%d)\n",
				_mtk_pq_get_optee_version(pq_dev));
	return -EPERM;
}

static int _mtk_pq_teec_enable(struct mtk_pq_device *pq_dev)
{
	u32 tee_error = 0;
	int ret = 0;
	struct TEEC_UUID uuid = DISP_UUID;
	struct TEEC_Context *pstContext = pq_dev->xc_info.secure_info.pstContext;
	struct TEEC_Session *pstSession = pq_dev->xc_info.secure_info.pstSession;

	if ((pstSession != NULL) && (pstContext != NULL)) {
		ret = _mtk_pq_teec_init_context(pq_dev, pstContext);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Init Context failed!\n");
			_mtk_pq_teec_finalize_context(pq_dev, pstContext);
			return -EPERM;
		}

		ret = _mtk_pq_teec_open_session(pq_dev, pstContext, pstSession, &uuid,
					NULL, &tee_error);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"TEEC Open pstSession failed with error(%u)\n", tee_error);
			_mtk_pq_teec_close_session(pq_dev, pstSession);
			_mtk_pq_teec_finalize_context(pq_dev, pstContext);
			return -EPERM;
		}

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "Enable disp teec success.\n");
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstSession or pstContext is NULL.\n");
		return -EPERM;
	}

	return 0;
}

static void _mtk_pq_teec_disable(struct mtk_pq_device *pq_dev)
{
	struct TEEC_Context *pstContext = pq_dev->xc_info.secure_info.pstContext;
	struct TEEC_Session *pstSession = pq_dev->xc_info.secure_info.pstSession;

	if ((pstContext != NULL) && (pstSession != NULL)) {
		_mtk_pq_teec_close_session(pq_dev, pstSession);
		_mtk_pq_teec_finalize_context(pq_dev, pstContext);

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "Disable disp teec success.\n");
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstSession or pstContext is NULL.\n");
	}
}

static int _mtk_pq_unauth_internal_buf(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	u8 idx = 0, win = 0;
	u64 iova = 0;
	struct pq_buffer *pBufferTable = NULL;

	/* get device parameter */
	win = pq_dev->dev_indx; //need to fix
	pBufferTable = pq_dev->xc_info.pBufferTable[win];

	if (win >= PQ_DEV_NUM) { //need to fix
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "win num is not valid : %u\n", win);
		return -EPERM;
	}

	if (pBufferTable == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pBufferTable = NULL\n");
		return -EPERM;
	}

	/* unauthorize pq buf */
	for (idx = 0; idx < PQ_BUF_MAX; idx++) {
		if (pBufferTable[idx].valid) {
			iova = pBufferTable[idx].addr[PQ_BUF_ATTR_S]; //lock secure buf only
			ret = mtkd_iommu_buffer_unauthorize(iova);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "authorize fail\n");
				return -EPERM;
			}

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
				"unauth pq_buf[%u]: iova=0x%llx\n", idx, iova);
		}
	}
	pq_dev->xc_info.secure_info.pq_buf_authed = false;
	return ret;
}

static int _mtk_pq_auth_internal_buf(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	u8 idx = 0, win = 0;
	u32 size = 0, pipelineID = 0, buf_tag = 0;
	u64 iova = 0;
	struct pq_buffer *pBufferTable = NULL;

	/* get device parameter */
	pipelineID = pq_dev->xc_info.secure_info.disp_pipeline_ID;
	win = pq_dev->dev_indx; //need to fix
	pBufferTable = pq_dev->xc_info.pBufferTable[win];

	if (pipelineID == 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "invalid disp pipelineID = 0\n");
		return -EPERM;
	}

	if (win >= PQ_DEV_NUM) { //need to fix
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "win num is not valid : %u\n", win);
		return -EPERM;
	}

	/* authorize pq buf */
	if (pq_dev->xc_info.secure_info.pq_buf_authed)
		goto already_auth;

	for (idx = 0; idx < PQ_BUF_MAX; idx++) {
		if (pBufferTable[idx].valid) {
			buf_tag = mtk_pq_buf_get_iommu_idx(pq_dev, idx) >> 24;
			size = pBufferTable[idx].size;
			iova = pBufferTable[idx].addr[PQ_BUF_ATTR_S]; //lock secure buf only

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
				"auth pq_buf[%u]: buf_tag=%u, size=%u, iova=0x%llx, pipelineID=%u\n",
				idx, buf_tag, size, iova, pipelineID);

			ret = mtkd_iommu_buffer_authorize(buf_tag, iova, size, pipelineID);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "authorize fail\n");
				return -EPERM;
			}
		}
	}
	pq_dev->xc_info.secure_info.pq_buf_authed = true;
		return ret;

already_auth:
	return ret;
}

static int _mtk_pq_auth_buf(struct mtk_pq_device *pq_dev, int buf_fd)
{
	int ret = 0;
	u32 pipelineID = 0;
	unsigned long long iova = 0;
	unsigned int size = 0;

	/* get device parameter */
	pipelineID = pq_dev->xc_info.secure_info.disp_pipeline_ID;
	if (pipelineID == 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid disp pipelineID = 0\n");
		return -EPERM;
	}

	/* get buf iova & size */
	ret = mtkd_iommu_getiova(buf_fd, &iova, &size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Get buf IOVA fail, fd = %d\n", buf_fd);
		return -EPERM;
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
		"Auth buf iova = %llx, size = %llx\n", iova, size);

	/* authorize buf */
	ret = mtkd_iommu_buffer_authorize((29 >> 24), iova, size, pipelineID);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "iommu authorize fail\n");
		return -EPERM;
	}

	return ret;
}

static int _mtk_pq_parse_svp_meta(struct mtk_pq_device *pq_dev,
			int meta_fd, enum mtk_pq_aid *aid, u32 *pipeline_id)
{
	int ret = 0;
	struct vdec_dd_svp_info vdec_svp_md;
	struct meta_srccap_svp_info srccap_svp_md;

	if (aid == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "aid = NULL\n");
		goto err_out;
	}
	if (pipeline_id == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pipeline_id = NULL\n");
		goto err_out;
	}

	*aid = PQ_AID_NS;
	*pipeline_id = 0;
	memset(&vdec_svp_md, 0, sizeof(struct vdec_dd_svp_info));
	memset(&srccap_svp_md, 0, sizeof(struct meta_srccap_svp_info));

	if (IS_INPUT_MVOP(pq_dev->common_info.input_source)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "is mvop source!!\n");

		ret = mtk_pq_common_read_metadata(meta_fd,
			EN_PQ_METATAG_VDEC_SVP_INFO, (void *)&vdec_svp_md);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "metadata do not has svp tag\n");
			goto non_svp_out;
		}

		if (!CHECK_AID_VALID(vdec_svp_md.aid)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Received invalid aid %u\n", vdec_svp_md.aid);
			goto err_out;
		}

		*aid = (enum mtk_pq_aid)vdec_svp_md.aid;
		*pipeline_id = vdec_svp_md.pipeline_id;

		ret = mtk_pq_common_delete_metadata(meta_fd,
			EN_PQ_METATAG_VDEC_SVP_INFO);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "delete svp tag fail\n");
			goto err_out;
		}
	}

	if (IS_SRC_HDMI(pq_dev->common_info.input_source)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "is HDMI!!\n");

		ret = mtk_pq_common_read_metadata(meta_fd,
			EN_PQ_METATAG_SRCCAP_SVP_INFO, (void *)&srccap_svp_md);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "metadata do not has svp tag\n");
			goto non_svp_out;
		}

		if (!CHECK_AID_VALID(srccap_svp_md.aid)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Received invalid aid %u\n", srccap_svp_md.aid);
			goto err_out;
		}

		*aid = (enum mtk_pq_aid)srccap_svp_md.aid;
		*pipeline_id = srccap_svp_md.pipelineid;

		ret = mtk_pq_common_delete_metadata(meta_fd,
			EN_PQ_METATAG_SRCCAP_SVP_INFO);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "delete svp tag fail\n");
			goto err_out;
		}
	}
	return ret;

non_svp_out:
	return 0;

err_out:
	return -EPERM;
}

static bool _mtk_pq_check_plane_fd_repeat(
	struct v4l2_plane *planes, u32 plane_idx)
{
	u32 i = 0;

	for (i = 0; i < plane_idx; i++) {
		if (planes[plane_idx].m.fd == planes[i].m.fd)
			return true;
	}

	return false;
}

int mtk_pq_teec_smc_call(struct mtk_pq_device *pq_dev,
			enum mtk_pq_tee_action action, void *para, u32 size)
{
	u32 error = 0;
	int ret = 0;
	struct TEEC_Operation op = {0};
	struct TEEC_Session *pstSession = pq_dev->xc_info.secure_info.pstSession;

	if (pstSession == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstSession is NULL!\n");
		return -EPERM;
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQ SMC cmd:%u\n", action);
	op.params[0].tmpref.buffer = para;
	op.params[0].tmpref.size = size;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	ret = _mtk_pq_teec_invoke_cmd(pq_dev, pstSession, action, &op, &error);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"SMC call failed with error(%u)\n", error);
		ret = -EPERM;
	}

	return ret;
}

int mtk_pq_svp_init(struct mtk_pq_device *pq_dev)
{
	struct TEEC_Context *pstContext = NULL;
	struct TEEC_Session *pstSession = NULL;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}

	pstContext = malloc(sizeof(struct TEEC_Context));
	if (pstContext == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstContext malloc fail!\n");
		return -EPERM;
	}

	pstSession = malloc(sizeof(struct TEEC_Session));
	if (pstSession == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstSession malloc fail!\n");
		free(pstContext);
		return -EPERM;
	}

	memset(pstContext, 0, sizeof(struct TEEC_Context));
	memset(pstSession, 0, sizeof(struct TEEC_Session));

	if (pq_dev->xc_info.secure_info.pstContext != NULL) {
		free(pq_dev->xc_info.secure_info.pstContext);
		pq_dev->xc_info.secure_info.pstContext = NULL;
	}

	if (pq_dev->xc_info.secure_info.pstSession != NULL) {
		free(pq_dev->xc_info.secure_info.pstSession);
		pq_dev->xc_info.secure_info.pstSession = NULL;
	}

	pq_dev->xc_info.secure_info.pstContext = pstContext;
	pq_dev->xc_info.secure_info.pstSession = pstSession;
	pq_dev->xc_info.secure_info.optee_version = 3;
	pq_dev->xc_info.secure_info.svp_enable = 0;
	pq_dev->xc_info.secure_info.disp_pipeline_valid = 0;
	pq_dev->xc_info.secure_info.aid = PQ_AID_NS;
	pq_dev->xc_info.secure_info.vdo_pipeline_ID = 0;
	pq_dev->xc_info.secure_info.disp_pipeline_ID = 0;
	pq_dev->xc_info.secure_info.pq_buf_authed = 0;

	if (_mtk_pq_teec_enable(pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "teec enable fail\n");
		return -EPERM;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQ SVP init success.\n");
	return 0;
}

void mtk_pq_svp_exit(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	struct TEEC_Context *pstContext = NULL;
	struct TEEC_Session *pstSession = NULL;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return;
	}

	_mtk_pq_teec_disable(pq_dev);

	pstContext = pq_dev->xc_info.secure_info.pstContext;
	pstSession = pq_dev->xc_info.secure_info.pstSession;

	if (pstContext != NULL) {
		free(pstContext);
		pq_dev->xc_info.secure_info.pstContext = NULL;
	}

	if (pstSession != NULL) {
		free(pstSession);
		pq_dev->xc_info.secure_info.pstSession = NULL;
	}
}

int mtk_pq_set_secure_mode(struct mtk_pq_device *pq_dev, u8 *secure_mode_flag)
{
	int ret = 0;
	bool secure_mode = 0;
	bool utpa_ret = false;
	struct mtk_pq_optee_handler optee_handler;
	EN_XC_OPTEE_ACTION optee_action = E_XC_OPTEE_GET_PIPE_ID;
	XC_OPTEE_HANDLER old_handler;

	memset(&optee_handler, 0, sizeof(struct mtk_pq_optee_handler));
	memset(&old_handler, 0, sizeof(XC_OPTEE_HANDLER));

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}
	if (secure_mode_flag == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}

	secure_mode = (bool)*secure_mode_flag;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQDD secure mode:%s\n",
		(secure_mode ? "ENABLE":"DISABLE"));

	if (secure_mode) {
		/* call disp TA create disp pipeline */
		optee_handler.version = MTK_PQ_OPTEE_HANDLER_VERSION;
		optee_handler.length = sizeof(struct mtk_pq_optee_handler);
		optee_handler.aid = PQ_AID_S;
		ret = mtk_pq_teec_smc_call(pq_dev,
			PQ_TEE_ACT_CREATE_DISP_PIPELINE,
			(void *)&optee_handler,
			sizeof(struct mtk_pq_optee_handler));
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "smc call fail\n");
			return ret;
		}
		pq_dev->xc_info.secure_info.disp_pipeline_ID
			= optee_handler.disp_pipeline_ID;
		pq_dev->xc_info.secure_info.disp_pipeline_valid = true;

		optee_action = E_XC_OPTEE_SET_PIPEID;
		old_handler.version = XC_OPTEE_HANDLER_VERSION;
		old_handler.length = sizeof(XC_OPTEE_HANDLER);
		old_handler.eWindow = MAIN_WINDOW;
		old_handler.pipeID = optee_handler.disp_pipeline_ID;
		utpa_ret = MApi_XC_OPTEE_Control(optee_action, &old_handler);
		if (!utpa_ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "UTPA GET_PIPE_ID fail!\n");
			return -EINVAL;
		}

		optee_action = E_XC_OPTEE_ENABLE;
		old_handler.isEnable = true;
		utpa_ret = MApi_XC_OPTEE_Control(optee_action, &old_handler);
		if (!utpa_ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "UTPA OPTEE ENABLE fail!\n");
			return -EINVAL;
		}
	} else {
		optee_action = E_XC_OPTEE_DISABLE;
		old_handler.isEnable = false;
		utpa_ret = MApi_XC_OPTEE_Control(optee_action, &old_handler);
		if (!utpa_ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "UTPA OPTEE DISABLE fail!\n");
			return -EINVAL;
		}
	}

	if ((!secure_mode) && (pq_dev->xc_info.secure_info.disp_pipeline_valid)) {
		/*unauthorize pq buffer */
		ret = _mtk_pq_unauth_internal_buf(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unauthorize pq buf fail\n");
			return ret;
		}

		/* call disp TA destroy disp pipeline */
		optee_handler.version = MTK_PQ_OPTEE_HANDLER_VERSION;
		optee_handler.length = sizeof(struct mtk_pq_optee_handler);
		optee_handler.aid = pq_dev->xc_info.secure_info.aid;
		optee_handler.disp_pipeline_ID = pq_dev->xc_info.secure_info.disp_pipeline_ID;
		ret = mtk_pq_teec_smc_call(pq_dev,
					PQ_TEE_ACT_DESTROY_DISP_PIPELINE,
					(void *)&optee_handler,
					sizeof(struct mtk_pq_optee_handler));
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "smc call fail\n");
			return ret;
		}

		/* reset param while disable secure mode */
		pq_dev->xc_info.secure_info.disp_pipeline_valid = 0;
		pq_dev->xc_info.secure_info.aid = PQ_AID_NS;
		pq_dev->xc_info.secure_info.vdo_pipeline_ID = 0;
		pq_dev->xc_info.secure_info.disp_pipeline_ID = 0;
	}

	pq_dev->xc_info.secure_info.svp_enable = secure_mode;
	return ret;
}

int mtk_pq_cfg_output_buf_sec(
	struct mtk_pq_device *pq_dev,
	struct v4l2_buffer *buffer)
{
	int ret = 0;
	bool secure_playback = false;
	bool is_secure_DD = false;
	u32 video_pipeline_id = 0;
	u32 plane_cnt = 0;
	enum mtk_pq_aid aid = 0;
	struct v4l2_pq_buf buf;
	struct mtk_pq_optee_handler optee_handler;
	struct v4l2_plane *plane_ptr = NULL;

	memset(&buf, 0, sizeof(struct v4l2_pq_buf));
	memset(&optee_handler, 0, sizeof(struct mtk_pq_optee_handler));

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}
	if (buffer == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}

	is_secure_DD = pq_dev->xc_info.secure_info.svp_enable;

	/* parse metadata */
	switch (buffer->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		if (buffer->m.userptr == NULL) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
			ret = -EINVAL;
			goto out;
		}
		buf = *(struct v4l2_pq_buf *)buffer->m.userptr;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
			"Received single source buffer, frame_fd = %d, meta_fd = %d\n",
			buf.frame_fd, buf.meta_fd);

		ret = _mtk_pq_parse_svp_meta(pq_dev,
					buf.meta_fd,
					&aid,
					&video_pipeline_id);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Parse svp meta tag fail\n");
			ret = -EPERM;
			goto out;
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		if (buffer->m.planes == NULL) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
			ret = -EINVAL;
			goto out;
		}
		plane_ptr = &(buffer->m.planes[(buffer->length - 1)]);
		for (plane_cnt = 0; plane_cnt < buffer->length; plane_cnt++) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
			"Received mplane source buffer, plane[%u]:fd = %d\n",
			plane_cnt, plane_ptr->m.fd);
		}
		ret = _mtk_pq_parse_svp_meta(pq_dev,
			plane_ptr->m.fd,
			&aid,
			&video_pipeline_id);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Parse svp meta tag fail\n");
			ret = -EPERM;
			goto out;
		}
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Unknown buf type : %u\n", buffer->type);
		ret = -EPERM;
		break;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
		"Received video aid = %d, pipeline id = 0x%lx\n",
		aid, video_pipeline_id);

	/* make decision */
	if ((aid != PQ_AID_NS) && (is_secure_DD)) {
		secure_playback = true;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "secure content playback!\n");
	} else if ((aid != PQ_AID_NS) && (!is_secure_DD)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"PQ QBUF ERROR! s-buffer queue into ns-dev!\n");
		ret = -EPERM;
		goto out;
	} else if ((aid == PQ_AID_NS) && (is_secure_DD)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"PQ QBUF ERROR! ns-buffer queue into s-dev!\n");
		ret = -EPERM;
		goto out;
	} else {
		secure_playback = false;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "non-secure content playback!\n");
	}

	/* Implement SVP flow */
	if (secure_playback) {
		if (!pq_dev->xc_info.secure_info.disp_pipeline_valid) {
			/* call disp TA create disp pipeline */
			optee_handler.version = MTK_PQ_OPTEE_HANDLER_VERSION;
			optee_handler.length = sizeof(struct mtk_pq_optee_handler);
			optee_handler.aid = aid;
			ret = mtk_pq_teec_smc_call(pq_dev,
				PQ_TEE_ACT_CREATE_DISP_PIPELINE,
				(void *)&optee_handler,
				sizeof(struct mtk_pq_optee_handler));
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "smc call fail\n");
				goto out;
			}
			pq_dev->xc_info.secure_info.disp_pipeline_ID
				= optee_handler.disp_pipeline_ID;
			pq_dev->xc_info.secure_info.disp_pipeline_valid = true;
		}
		/* lock PQ buffer */
		ret = _mtk_pq_auth_internal_buf(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "auth pq buf fail\n");
			goto out;
		}
		/* trigger cmd Q to set secure AID */
			//not implement yet
	} else {
		/* trigger cmd Q to set non-secure AID */
			//not implement yet
	}

	/* Store svp info in device */
	pq_dev->xc_info.secure_info.vdo_pipeline_ID = video_pipeline_id;
	pq_dev->xc_info.secure_info.aid = aid;

out:
	return ret;
}

int mtk_pq_cfg_capture_buf_sec(
	struct mtk_pq_device *pq_dev,
	struct v4l2_buffer *buffer)
{
	int ret = 0;
	u32 plane_cnt = 0;
	struct v4l2_plane *plane_ptr = NULL;
	bool is_secure_DD = false;
	bool same_fd = false;
	struct v4l2_pq_buf buf;
	struct meta_pq_disp_svp pq_svp_md;

	memset(&buf, 0, sizeof(struct v4l2_pq_buf));
	memset(&pq_svp_md, 0, sizeof(struct meta_pq_disp_svp));

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}
	if (buffer == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}

	is_secure_DD = pq_dev->xc_info.secure_info.svp_enable;

	switch (buffer->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (buffer->m.userptr == NULL) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
			ret = -EINVAL;
			goto out;
		}
		buf = *(struct v4l2_pq_buf *)buffer->m.userptr;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
			"Received single window buffer, frame_fd = %d, meta_fd = %d\n",
			buf.frame_fd, buf.meta_fd);

		/* lock each buf queued into s-DD */
		if (is_secure_DD) {
			ret = _mtk_pq_auth_buf(pq_dev, buf.frame_fd);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "auth window buf fail\n");
				goto out;
			}
			pq_svp_md.aid = (u8)pq_dev->xc_info.secure_info.aid;
			pq_svp_md.pipelineid = pq_dev->xc_info.secure_info.disp_pipeline_ID;
		}

		/* Write svp info in metadata for render */
		ret = mtk_pq_common_write_metadata(buf.meta_fd,
			EN_PQ_METATAG_PQ_SVP_INFO, (void *)&pq_svp_md);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Write svp meta tag fail\n");
			goto out;
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		if (buffer->m.planes == NULL) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
			ret = -EINVAL;
			goto out;
		}
		if (is_secure_DD) {
			for (plane_cnt = 0; plane_cnt < (buffer->length - 1); plane_cnt++) {
				plane_ptr = &(buffer->m.planes[plane_cnt]);
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
					"Received mplane window buffer, plane[%u]:fd = %d\n",
					plane_cnt, plane_ptr->m.fd);

				same_fd = _mtk_pq_check_plane_fd_repeat(
					buffer->m.planes,
					plane_cnt);
				if (!same_fd) {
					/* lock each plane queued into s-DD */
					ret = _mtk_pq_auth_buf(pq_dev, plane_ptr->m.fd);
					if (ret) {
						STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
							"auth window buf fail\n");
						goto out;
					}
				}
			}
			pq_svp_md.aid = (u8)pq_dev->xc_info.secure_info.aid;
			pq_svp_md.pipelineid = pq_dev->xc_info.secure_info.disp_pipeline_ID;
		}

		/* Write svp info in metadata for render */
		plane_ptr = &(buffer->m.planes[(buffer->length - 1)]);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
			"Received mplane window buffer, meta fd = %d\n", plane_ptr->m.fd);
		ret = mtk_pq_common_write_metadata(plane_ptr->m.fd,
			EN_PQ_METATAG_PQ_SVP_INFO, (void *)&pq_svp_md);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Write svp meta tag fail\n");
			goto out;
		}
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Unknown buf type : %u\n", buffer->type);
		ret = -EPERM;
		break;
	}

out:
	return ret;
}
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

#endif // CONFIG_OPTEE
