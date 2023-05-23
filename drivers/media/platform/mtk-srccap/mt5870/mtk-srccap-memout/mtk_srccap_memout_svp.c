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
#include <linux/version.h>
#include <asm/uaccess.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#include "mtk_srccap.h"
#include "mtk_srccap_memout.h"
#include "mtk_iommu_dtv_api.h"
#include "linux/metadata_utility/m_srccap.h"

MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

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

static int _mtk_memout_get_optee_version(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev->memout_info.secure_info.optee_version != -1)
		goto final;

	if (srccap_dev->memout_info.secure_info.optee_version == -1) {
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
			SRCCAP_MSG_ERROR("[SVP] can't open /proc/tz2_mstar/version\n");
			srccap_dev->memout_info.secure_info.optee_version = 1;
			set_fs(cur_mm_seg);
			kfree(sVer);
			goto final;
		}

		pos = vfs_llseek(verfile, 0, SEEK_SET);
		u32ReadCount = kernel_read(verfile, sVer, BOOTARG_SIZE, &pos);
		set_fs(cur_mm_seg);

		if (u32ReadCount > BOOTARG_SIZE) {
			SRCCAP_MSG_ERROR("[SVP] cmdline info more than buffer size\n");
			goto exit;
		}

		sVer[BOOTARG_SIZE - 2] = '\n';
		sVer[BOOTARG_SIZE - 1] = '\0';

		if (strncmp(sVer, "2.4", 2) == 0)
			srccap_dev->memout_info.secure_info.optee_version = 2;
		else if (strncmp(sVer, "3.2", 2) == 0)
			srccap_dev->memout_info.secure_info.optee_version = 3;
		else
			srccap_dev->memout_info.secure_info.optee_version = 1;

exit:
		if (verfile != NULL) {
			cur_mm_seg = get_fs();
			set_fs(KERNEL_DS);
			filp_close(verfile, NULL);
			set_fs(cur_mm_seg);
		}
		SRCCAP_MSG_INFO("[SVP] OPTEE Version %d\n",
				srccap_dev->memout_info.secure_info.optee_version);
		kfree(sVer);
	}

final:
	return srccap_dev->memout_info.secure_info.optee_version;
}

static void _mtk_memout_teec_free_temp_refs(struct TEEC_Operation *operation,
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
			SRCCAP_MSG_ERROR("[SVP] Invalid param_type : %x\n", param_type);
			break;
		}
	}
}

int _mtk_memout_teec_alloc_shm(struct TEEC_Context *ctx, struct TEEC_SharedMemory *shm)
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
		SRCCAP_MSG_ERROR("[SVP] tee client shm alloc got NULL\n");
		return -ENOMEM;
	}
	if (IS_ERR(_shm)) {
		SRCCAP_MSG_ERROR("[SVP] tee client shm alloc fail\n");
		return -ENOMEM;
	}

	shm_va = tee_shm_get_va(_shm, 0);
	if (!shm_va) {
		SRCCAP_MSG_ERROR("[SVP] tee client shm get va got NULL\n");
		return -ENOMEM;
	}

	if (IS_ERR(shm_va)) {
		SRCCAP_MSG_ERROR("[SVP] tee client shm get va fail\n");
		return -ENOMEM;
	}

	shm->buffer = shm_va;
	shm->shadow_buffer = (void *)_shm;
	shm->alloced_size = len;
	shm->registered_fd = -1;
	shm->buffer_allocated = true;
	return 0;
}

static void _mtk_memout_teec_post_proc_tmpref(uint32_t param_type,
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

static void _mtk_memout_teec_post_proc_whole(struct TEEC_RegisteredMemoryReference *memref,
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

static void _mtk_memout_teec_post_proc_partial(uint32_t param_type,
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

static void _mtk_memout_teec_post_proc_operation(struct TEEC_Operation *operation,
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
			_mtk_memout_teec_post_proc_tmpref(param_type,
				&operation->params[n].tmpref, params + n, shms + n);
			break;
		case TEEC_MEMREF_WHOLE:
			_mtk_memout_teec_post_proc_whole(&operation->params[n].memref,
						params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			_mtk_memout_teec_post_proc_partial(param_type,
				&operation->params[n].memref, params + n);
			break;
		default:
			SRCCAP_MSG_ERROR("[SVP] Invalid param_type : %x\n", param_type);
			break;
		}
	}
}

static int _mtk_memout_teec_pre_proc_tmpref(struct TEEC_Context *ctx,
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
		SRCCAP_MSG_ERROR("[SVP] Invalid param_type : %x\n", param_type);
		return -EINVAL;
	}
	shm->size = tmpref->size;

	ret = _mtk_memout_teec_alloc_shm(ctx, shm);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP] share mem alloc fail\n");
		return -EPERM;
	}

	memcpy(shm->buffer, tmpref->buffer, tmpref->size);
	param->u.memref.size = tmpref->size;
	//Workaround, shm->shadow_buffer is shm object.
	param->u.memref.shm = (struct tee_shm *) shm->shadow_buffer;
	return ret;
}

static int _mtk_memout_teec_pre_proc_whole(
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

static int _mtk_memout_teec_pre_proc_partial(uint32_t param_type,
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
		SRCCAP_MSG_ERROR("[SVP] Invalid param_type : %x\n", param_type);
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

static int _mtk_memout_teec_pre_proc_operation(
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
			ret = _mtk_memout_teec_pre_proc_tmpref(ctx, param_type,
				&(operation->params[n].tmpref), params + n, shms + n);
			break;
		case TEEC_MEMREF_WHOLE:
			ret = _mtk_memout_teec_pre_proc_whole(
					&operation->params[n].memref, params + n);
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			ret = _mtk_memout_teec_pre_proc_partial(param_type,
				&operation->params[n].memref, params + n);
			break;
		default:
			ret = -EINVAL;
			SRCCAP_MSG_ERROR("[SVP] Invalid param_type : %x\n", param_type);
			break;
		}
	}

	return ret;
}

static int _mtk_memout_teec_invoke_cmd(struct mtk_srccap_dev *srccap_dev,
			struct TEEC_Session *session, u32 cmd_id,
			struct TEEC_Operation *operation, u32 *error_origin)
{
	int ret = -EPERM;
	int otpee_version = _mtk_memout_get_optee_version(srccap_dev);
	uint32_t err_ori = 0;
	uint64_t buf[(sizeof(struct tee_ioctl_invoke_arg) +
			TEEC_CONFIG_PAYLOAD_REF_COUNT *
				sizeof(struct tee_param)) / sizeof(uint64_t)] = { 0 };
	struct tee_ioctl_invoke_arg *arg = NULL;
	struct tee_param *params = NULL;
	struct TEEC_SharedMemory shm[TEEC_CONFIG_PAYLOAD_REF_COUNT] = { 0 };

	if (!session) {
		SRCCAP_MSG_ERROR("[SVP] session is NULL!!!\n");
		return -EINVAL;
	}
	if (otpee_version == 3) {
		arg = (struct tee_ioctl_invoke_arg *)buf;
		arg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;
		params = (struct tee_param *)(arg + 1);
		arg->session = session->session_id;
		arg->func = cmd_id;

		if (operation)
			operation->session = session;

		ret = _mtk_memout_teec_pre_proc_operation(&(session->ctx), operation, params, shm);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] teec pre process failed!\n");
			err_ori = TEEC_ORIGIN_API;
			goto out_free_temp_refs;
		}

		ret = tee_client_invoke_func((struct tee_context *)session->ctx.fd, arg, params);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] TEE_IOC_INVOKE failed!\n");
			err_ori = TEEC_ORIGIN_COMMS;
			goto out_free_temp_refs;
		}

		SRCCAP_MSG_INFO("[SVP] tee client invoke cmd success.\n");

		ret = arg->ret;
		err_ori = arg->ret_origin;
		_mtk_memout_teec_post_proc_operation(operation, params, shm);

out_free_temp_refs:
		_mtk_memout_teec_free_temp_refs(operation, shm);

		if (error_origin)
			*error_origin = err_ori;
	} else
		SRCCAP_MSG_ERROR("[SVP] Not support OPTEE version (%d)\n", otpee_version);

	return ret;
}

static void _mtk_memout_teec_close_session(struct mtk_srccap_dev *srccap_dev,
			struct TEEC_Session *session)
{
	int otpee_version = _mtk_memout_get_optee_version(srccap_dev);

	if (!session) {
		SRCCAP_MSG_ERROR("[SVP] tee close fail session is NULL!!!\n");
		return;
	}
	if (!session->ctx.fd) {
		SRCCAP_MSG_ERROR("[SVP] tee close fail session->ctx.fd is NULL!!!\n");
		return;
	}

	if (otpee_version == 3)
		tee_client_close_session((struct tee_context *)session->ctx.fd,
			session->session_id);
	else
		SRCCAP_MSG_ERROR("[SVP] Not support OPTEE version (%d)\n", otpee_version);
}

static int _mtk_memout_teec_open_session(struct mtk_srccap_dev *srccap_dev,
			struct TEEC_Context *context,
			struct TEEC_Session *session,
			const struct TEEC_UUID *destination,
			struct TEEC_Operation *operation,
			u32 *error_origin)
{
	int otpee_version = _mtk_memout_get_optee_version(srccap_dev);

	if ((!session) || (!context)) {
		SRCCAP_MSG_ERROR("[SVP] tee open fail session or context is NULL!\n");
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

		SRCCAP_MSG_ERROR("[SVP] tee open failed! rc=0x%08x, err=0x%08x, ori=0x%08x\n",
			rc, arg.ret, arg.ret_origin);
	} else {
		SRCCAP_MSG_ERROR("[SVP] Not support OPTEE version (%d)\n", otpee_version);
	}

	return -EPERM;
}

static void _mtk_memout_teec_finalize_context(struct mtk_srccap_dev *srccap_dev,
			struct TEEC_Context *context)
{
	int otpee_version = _mtk_memout_get_optee_version(srccap_dev);

	if (!context) {
		SRCCAP_MSG_ERROR("[SVP] tee finalize context fail context is NULL!\n");
		return;
	}
	if (!context->ctx) {
		SRCCAP_MSG_ERROR("[SVP] tee finalize context fail context->ctx is NULL!\n");
		return;
	}

	if (otpee_version == 3)
		tee_client_close_context(context->ctx);
	else
		SRCCAP_MSG_ERROR("[SVP] Not support OPTEE version (%d)\n", otpee_version);
}

static int _mtk_memout_teec_init_context(struct mtk_srccap_dev *srccap_dev,
			struct TEEC_Context *context)
{
	if (_mtk_memout_get_optee_version(srccap_dev) == 3) {
		struct tee_ioctl_version_data vers = {
			.impl_id = TEE_OPTEE_CAP_TZ,
			.impl_caps = TEE_IMPL_ID_OPTEE,
			.gen_caps = TEE_GEN_CAP_GP,
		};

		context->ctx = tee_client_open_context(NULL, _optee_match, NULL, &vers);

		if (IS_ERR(context->ctx)) {
			SRCCAP_MSG_ERROR("[SVP] context is NULL!\n");
			return -EINVAL;
		}
		return 0;
	}

	SRCCAP_MSG_ERROR("[SVP] Not support OPTEE version (%d)\n",
				_mtk_memout_get_optee_version(srccap_dev));
	return -EPERM;
}

static int _mtk_memout_teec_enable(struct mtk_srccap_dev *srccap_dev)
{
	u32 tee_error = 0;
	int ret = 0;
	struct TEEC_UUID uuid = DISP_UUID;
	struct TEEC_Context *pstContext =
			srccap_dev->memout_info.secure_info.pstContext;
	struct TEEC_Session *pstSession =
			srccap_dev->memout_info.secure_info.pstSession;

	if ((pstSession != NULL) && (pstContext != NULL)) {
		ret = _mtk_memout_teec_init_context(srccap_dev, pstContext);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] Init Context failed!\n");
			_mtk_memout_teec_finalize_context(srccap_dev, pstContext);
			return -EPERM;
		}

		ret = _mtk_memout_teec_open_session(srccap_dev, pstContext, pstSession, &uuid,
					NULL, &tee_error);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] TEEC Open pstSession failed with error (%u)\n",
				tee_error);
			_mtk_memout_teec_close_session(srccap_dev, pstSession);
			_mtk_memout_teec_finalize_context(srccap_dev, pstContext);
			return -EPERM;
		}
	} else {
		SRCCAP_MSG_ERROR("[SVP] pstSession or pstContext is NULL.\n");
		return -EINVAL;
	}

	return 0;
}

static void _mtk_memout_teec_disable(struct mtk_srccap_dev *srccap_dev)
{
	struct TEEC_Context *pstContext =
			srccap_dev->memout_info.secure_info.pstContext;
	struct TEEC_Session *pstSession =
			srccap_dev->memout_info.secure_info.pstSession;

	if ((pstContext != NULL) && (pstSession != NULL)) {
		_mtk_memout_teec_close_session(srccap_dev, pstSession);
		_mtk_memout_teec_finalize_context(srccap_dev, pstContext);
	} else {
		SRCCAP_MSG_ERROR("[SVP] pstSession or pstContext is NULL.\n");
	}
}

static int _mtk_memout_auth_buf(struct mtk_srccap_dev *srccap_dev, int buf_fd)
{
	int ret = 0;
	u32 pipelineID = 0;
	unsigned long long iova = 0;
	unsigned int size = 0;

	/* get device parameter */
	pipelineID = srccap_dev->memout_info.secure_info.video_pipeline_ID;
	if (pipelineID == 0) {
		SRCCAP_MSG_INFO("[SVP] Invalid video pipelineID = 0\n");
		return -EINVAL;
	}

	/* get buf iova & size */
	ret = mtkd_iommu_getiova(buf_fd, &iova, &size);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP] Get buf IOVA fail, fd = %d\n", buf_fd);
		return -EACCES;
	}
	SRCCAP_MSG_DEBUG("[SVP] buf iova = %llx, size = %llx\n", iova, size);

	/* authorize source buf */
	ret = mtkd_iommu_buffer_authorize(22, iova, size, pipelineID);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP]iommu authorize fail\n");
		return -EPERM;
	}

	return ret;
}

int mtk_memout_teec_smc_call(struct mtk_srccap_dev *srccap_dev,
			enum srccap_memout_tee_action action, void *para, u32 size)
{
	u32 error = 0;
	int ret = 0;
	struct TEEC_Operation op = {0};
	struct TEEC_Session *pstSession = srccap_dev->memout_info.secure_info.pstSession;

	if (pstSession == NULL) {
		SRCCAP_MSG_ERROR("[SVP] pstSession is NULL!\n");
		return -EINVAL;
	}
	SRCCAP_MSG_INFO("[SVP] SRCCAP SMC cmd:%u\n", action);
	op.params[0].tmpref.buffer = para;
	op.params[0].tmpref.size = size;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	ret = _mtk_memout_teec_invoke_cmd(srccap_dev, pstSession, action, &op, &error);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP] SMC call failed with error(%u)\n", error);
		ret = -EPERM;
	}

	return ret;
}

int mtk_memout_svp_init(struct mtk_srccap_dev *srccap_dev)
{
	struct TEEC_Context *pstContext = NULL;
	struct TEEC_Session *pstSession = NULL;

	pstContext = malloc(sizeof(struct TEEC_Context));
	if (pstContext == NULL) {
		SRCCAP_MSG_ERROR("[SVP] pstContext malloc fail!\n");
		return -EPERM;
	}

	pstSession = malloc(sizeof(struct TEEC_Session));
	if (pstSession == NULL) {
		SRCCAP_MSG_ERROR("[SVP] pstSession malloc fail!\n");
		free(pstContext);
		return -EPERM;
	}

	memset(pstContext, 0, sizeof(struct TEEC_Context));
	memset(pstSession, 0, sizeof(struct TEEC_Session));

	if (srccap_dev->memout_info.secure_info.pstContext != NULL) {
		free(srccap_dev->memout_info.secure_info.pstContext);
		srccap_dev->memout_info.secure_info.pstContext = NULL;
	}

	if (srccap_dev->memout_info.secure_info.pstSession != NULL) {
		free(srccap_dev->memout_info.secure_info.pstSession);
		srccap_dev->memout_info.secure_info.pstSession = NULL;
	}

	srccap_dev->memout_info.secure_info.pstContext = pstContext;
	srccap_dev->memout_info.secure_info.pstSession = pstSession;
	srccap_dev->memout_info.secure_info.optee_version = 3;
	srccap_dev->memout_info.secure_info.svp_enable = 0;
	srccap_dev->memout_info.secure_info.aid = SRCCAP_MEMOUT_SVP_AID_NS;
	srccap_dev->memout_info.secure_info.video_pipeline_ID = 0;

	if (_mtk_memout_teec_enable(srccap_dev)) {
		SRCCAP_MSG_ERROR("[SVP] teec enable fail\n");
		return -EPERM;
	}

	SRCCAP_MSG_INFO("[SVP] SVP init success.\n");
	return 0;
}

void mtk_memout_svp_exit(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	struct v4l2_memout_secure_info secure_info;
	struct TEEC_Context *pstContext = NULL;
	struct TEEC_Session *pstSession = NULL;

	memset(&secure_info, 0, sizeof(struct v4l2_memout_secure_info));
	ret = mtk_memout_set_secure_mode(srccap_dev, &secure_info);
	_mtk_memout_teec_disable(srccap_dev);

	pstContext = srccap_dev->memout_info.secure_info.pstContext;
	pstSession = srccap_dev->memout_info.secure_info.pstSession;

	if (pstContext != NULL) {
		free(pstContext);
		srccap_dev->memout_info.secure_info.pstContext = NULL;
	}
	if (pstSession != NULL) {
		free(pstSession);
		srccap_dev->memout_info.secure_info.pstSession = NULL;
	}

	SRCCAP_MSG_INFO("[SVP] SVP exit success.\n");
}

int mtk_memout_set_secure_mode(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_memout_secure_info *secureinfo)
{
	int ret = 0;
	u32 pipeline_id = 0;
	enum srccap_memout_svp_aid aid = SRCCAP_MEMOUT_SVP_AID_NS;
	struct srccap_memout_optee_handler optee_handler;
	struct reg_srccap_memout_sstinfo hwreg_sstinfo;

	if (secureinfo == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&optee_handler, 0, sizeof(struct srccap_memout_optee_handler));
	memset(&hwreg_sstinfo, 0, sizeof(struct reg_srccap_memout_sstinfo));

	/* prepare handler */
	optee_handler.version = SRCCAP_MEMOUT_OPTEE_HANDLER_VERSION;
	optee_handler.length = sizeof(struct srccap_memout_optee_handler);

	/* enable secure mode */
	if (secureinfo->bEnable) {
		SRCCAP_MSG_INFO("[SVP] \033[1;33mENABLE Secure Mode\033[0m\n");
		aid = SRCCAP_MEMOUT_SVP_AID_S;
		if (!secureinfo->bTestMode) {
			/* read aid from SST */
			ret = drv_hwreg_srccap_memout_get_sst(&hwreg_sstinfo);
			if (ret) {
				SRCCAP_MSG_ERROR("[SVP] HWREG get sst fail/n");
				return -EINVAL;
			}

			switch (srccap_dev->srccap_info.src) {
			case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
				aid = (enum srccap_memout_svp_aid)hwreg_sstinfo.secure_hdmi_0;
				break;
			case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
				aid = (enum srccap_memout_svp_aid)hwreg_sstinfo.secure_hdmi_1;
				break;
			case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
				aid = (enum srccap_memout_svp_aid)hwreg_sstinfo.secure_hdmi_2;
				break;
			case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
				aid = (enum srccap_memout_svp_aid)hwreg_sstinfo.secure_hdmi_3;
				break;
			default:
				SRCCAP_MSG_ERROR("[SVP] Unsupported SVP extin source %d/n",
					srccap_dev->srccap_info.src);
				return -EINVAL;
			}

			if (aid == SRCCAP_MEMOUT_SVP_AID_NS) {
				SRCCAP_MSG_ERROR("[SVP] Get invalid aid from sst %d/n", aid);
				return -EINVAL;
			}
		}
		optee_handler.aid = aid;
		ret = mtk_memout_teec_smc_call(srccap_dev,
				SRCCAP_MEMOUT_TEE_ACT_CREATE_VIDEO_PIPELINE,
				&optee_handler,
				sizeof(struct srccap_memout_optee_handler));
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] smc call fail ");
			return ret;
		}
		pipeline_id = optee_handler.vdo_pipeline_ID;
		SRCCAP_MSG_INFO("[SVP] video pipeline ID = 0x%x\n", pipeline_id);
	}
	/* disable secure mode */
	else {
		optee_handler.vdo_pipeline_ID =
				srccap_dev->memout_info.secure_info.video_pipeline_ID;
		optee_handler.aid = srccap_dev->memout_info.secure_info.aid;
		if ((optee_handler.vdo_pipeline_ID != 0) &&
			(optee_handler.aid != SRCCAP_MEMOUT_SVP_AID_NS)) {
			SRCCAP_MSG_INFO("[SVP] \033[1;33mDISABLE Secure Mode\033[0m\n");
			ret = mtk_memout_teec_smc_call(srccap_dev,
					SRCCAP_MEMOUT_TEE_ACT_DESTROY_VIDEO_PIPELINE,
					&optee_handler,
					sizeof(struct srccap_memout_optee_handler));
			if (ret) {
				SRCCAP_MSG_ERROR("[SVP] smc call fail ");
				return ret;
			}
		}
		pipeline_id = 0;
		aid = SRCCAP_MEMOUT_SVP_AID_NS;
	}

	/* store svp info in device */
	srccap_dev->memout_info.secure_info.aid = aid;
	srccap_dev->memout_info.secure_info.svp_enable = secureinfo->bEnable;
	srccap_dev->memout_info.secure_info.video_pipeline_ID = pipeline_id;
	return ret;
}

int mtk_memout_get_sst(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_memout_sst_info *sst_info)
{
	int ret = 0;
	struct reg_srccap_memout_sstinfo hwreg_sstinfo;

	if (sst_info == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&hwreg_sstinfo, 0, sizeof(struct reg_srccap_memout_sstinfo));
	ret = drv_hwreg_srccap_memout_get_sst(&hwreg_sstinfo);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP] HWREG get sst fail/n");
		return -EINVAL;
	}

	memset(sst_info, 0, sizeof(struct v4l2_memout_sst_info));
	if (hwreg_sstinfo.secure_hdmi_0)
		sst_info->HDMI1_secure = true;
	if (hwreg_sstinfo.secure_hdmi_1)
		sst_info->HDMI2_secure = true;
	if (hwreg_sstinfo.secure_hdmi_2)
		sst_info->HDMI3_secure = true;
	if (hwreg_sstinfo.secure_hdmi_3)
		sst_info->HDMI4_secure = true;
	return ret;
}

int mtk_memout_cfg_buf_security(
	struct mtk_srccap_dev *srccap_dev,
	s32 buf_fd,
	s32 meta_fd)
{
	int ret = 0;
	struct meta_srccap_svp_info svp_meta;

	memset(&svp_meta, 0, sizeof(struct meta_srccap_svp_info));

	/* lock each buf queued into s-DD */
	if (srccap_dev->memout_info.secure_info.svp_enable) {
		ret = _mtk_memout_auth_buf(srccap_dev, buf_fd);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] buf authorize fail\n");
			return -EPERM;
		}
	}

	/* set AID & pipeline id in metadata */
	svp_meta.aid = (u8)srccap_dev->memout_info.secure_info.aid;
	svp_meta.pipelineid =
		srccap_dev->memout_info.secure_info.video_pipeline_ID;
	ret = mtk_memout_write_metadata(meta_fd,
		SRCCAP_MEMOUT_METATAG_SVP_INFO, &svp_meta);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP] write svp meta tag fail\n");
		return -EPERM;
	}

	return ret;
}

#endif // CONFIG_OPTEE
