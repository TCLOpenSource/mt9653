// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/scatterlist.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/tee_drv.h>
#include <linux/tee.h>
#include <linux/printk.h>
#include <linux/uuid.h>

#include "mtk_iommu_ta.h"
#include "mtk_iommu_tee_interface.h"
#include "mtk_iommu_of.h"

#define TEE_PARAM_COUNT 4

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

#define USE_SLAB	1
#define USE_PAGE	2

struct mtk_iommu_optee_contex {
	struct tee_context *ctx;
	u32 session_id;
	bool session_initialized;
};

struct TEEC_SharedMemory_2 {
	void *buffer;
	size_t size;
	uint32_t flags;
	size_t alloced_size;
	void *shadow_buffer;
	struct page **pages;
};

struct TempMemoryReference {
	void *buffer;
	size_t size;
};

struct TempValue {
	uint32_t a;
	uint32_t b;
};

struct parameter {
	struct TempValue value;
	struct TempMemoryReference tmpref;
};

struct TEEC_Operation {
	uint32_t paramTypes;
	struct parameter params[TEE_PARAM_COUNT];
};

static struct mtk_iommu_optee_contex mtk_iommu_ctx;

static inline void *__alloc_frag_pages(struct TEEC_SharedMemory_2 *shm, size_t len)
{
	int i;
	void *vaddr;
	int npages = PAGE_ALIGN(len) / PAGE_SIZE;

	shm->pages = kcalloc(npages, sizeof(struct page *), GFP_KERNEL);
	if (!shm->pages)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < npages; ++i) {
		struct page *page;

		page = alloc_page(GFP_KERNEL);
		if (!page) {
			pr_crit("alloc order 0 pages failed!\n");
			kfree(shm->pages);
			return ERR_PTR(-ENOMEM);
		}
		shm->pages[i] = page;
	}

	vaddr = vmap(shm->pages, npages, VM_MAP, PAGE_KERNEL);
	if (!vaddr) {
		for (i = 0; i < npages; ++i)
			__free_page(shm->pages[i]);
		kfree(shm->pages);
		return ERR_PTR(-ENOMEM);
	}

	return vaddr;
}

static inline void *_alloc_shm(struct TEEC_SharedMemory_2 *shm, size_t len)
{
#define RETRY_CNT	256
	void *va = NULL;
	int retry = RETRY_CNT;

	do {
		if (len <= PAGE_SIZE) {
			va = kzalloc(len, GFP_KERNEL);
			if (va) {
				shm->flags = USE_SLAB;
				return va;
			}
		} else {
			va = __alloc_frag_pages(shm, len);
			if (!IS_ERR_OR_NULL(va)) {
				shm->flags = USE_PAGE;
				return va;
			}
		}

		pr_err("%s: iommu alloc tee shm %zu byte failed, retry (%d)\n",
			__func__, len, retry);
	} while (--retry);

	return NULL;
}

static int teec_allocatesharedmemory(struct tee_context *ctx,
		struct TEEC_SharedMemory_2 *shm)
{
	size_t len;
	struct tee_shm *_shm = NULL;
	void *shm_va;

	if (!ctx)
		return -EINVAL;

	if (WARN_ON(!shm->size))
		return -EINVAL;

	shm->shadow_buffer = shm->buffer = NULL;
	len = shm->size;
	shm_va = _alloc_shm(shm, len);
	if (!shm_va)
		return -ENOMEM;

	_shm = tee_shm_register(ctx, (unsigned long)shm_va, len,
			TEE_SHM_DMA_BUF | TEE_SHM_KERNEL_MAPPED);
	if (IS_ERR(_shm)) {
		pr_err("%s: tee_shm_register failed\n", __func__);
		return -ENOMEM;
	}

	shm->buffer = shm_va;
	shm->shadow_buffer = (void *)_shm;
	shm->alloced_size = len;

	return 0;
}

static void teec_releasesharedmemory(struct TEEC_SharedMemory_2 *shm)
{
	if (shm->buffer) {
		switch (shm->flags) {
		case USE_SLAB:
			kfree(shm->buffer);
			break;
		case USE_PAGE: {
			int i, npages = PAGE_ALIGN(shm->alloced_size) / PAGE_SIZE;

			vunmap(shm->buffer);
			for (i = 0; i < npages; ++i)
				__free_page(shm->pages[i]);
			kfree(shm->pages);
			break;
		}
		default:
			pr_err("Wired shm alloc method\n");
		}
	}
	if (shm->shadow_buffer)
		tee_shm_free((struct tee_shm *)shm->shadow_buffer);
	shm->shadow_buffer = NULL;
	shm->buffer = NULL;
}

static int teec_pre_process_tmpref(struct tee_context *ctx,
		uint32_t param_type, struct TempMemoryReference *tmpref,
		struct tee_param *param, struct TEEC_SharedMemory_2 *shm)
{
	int res;

	param->attr = param_type;
	shm->size = tmpref->size;

	res = teec_allocatesharedmemory(ctx, shm);
	if (res)
		return res;

	memcpy(shm->buffer, tmpref->buffer, tmpref->size);
	param->u.memref.size = tmpref->size;
	//Workaround, shm->shadow_buffer is shm object.
	param->u.memref.shm = (struct tee_shm *)shm->shadow_buffer;
	return 0;
}

static int teec_pre_process_operation(struct tee_context *ctx,
		struct TEEC_Operation *operation,
		struct tee_param *params, struct TEEC_SharedMemory_2 *shms)
{
	int res;
	size_t n;
	uint32_t param_type;

	memset(shms, 0, sizeof(*shms) * TEE_PARAM_COUNT);
	if (!operation) {
		memset(params, 0, sizeof(*params) * TEE_PARAM_COUNT);
		return 0;
	}

	for (n = 0; n < TEE_PARAM_COUNT; n++) {
		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);
		switch (param_type) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_NONE:
			params[n].attr = param_type;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
			params[n].attr = param_type;
			params[n].u.value.a = operation->params[n].value.a;
			params[n].u.value.b = operation->params[n].value.b;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			res = teec_pre_process_tmpref(ctx, param_type,
						&operation->params[n].tmpref,
						params + n, shms + n);
			if (res)
				return res;
			break;
		default:
			pr_err("%s: cmd %d is not support\n", __func__, param_type);
			return -EINVAL;
		}
	}

	return 0;
}

static void teec_post_process_tmpref(uint32_t param_type,
		struct TempMemoryReference *tmpref,
		struct tee_param *param, struct TEEC_SharedMemory_2 *shm)
{
	if (param_type != TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT) {
		if (param->u.memref.size <= tmpref->size && tmpref->buffer)
			memcpy(tmpref->buffer, shm->buffer,
				param->u.memref.size);

		tmpref->size = param->u.memref.size;
	}
}

static void teec_post_process_operation(struct TEEC_Operation *operation,
		struct tee_param *params, struct TEEC_SharedMemory_2 *shms)
{
	size_t n;

	if (!operation)
		return;

	for (n = 0; n < TEE_PARAM_COUNT; n++) {
		uint32_t param_type;

		param_type = TEEC_PARAM_TYPE_GET(operation->paramTypes, n);
		switch (param_type) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT:
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
			operation->params[n].value.a = params[n].u.value.a;
			operation->params[n].value.b = params[n].u.value.b;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			teec_post_process_tmpref(param_type,
						 &operation->params[n].tmpref,
						 params + n, shms + n);
			break;
		default:
			break;
		}
	}
}

static void teec_free_temp_refs(struct TEEC_Operation *operation,
		struct TEEC_SharedMemory_2 *shms)
{
	size_t n;

	if (!operation)
		return;

	for (n = 0; n < TEE_PARAM_COUNT; n++) {
		switch (TEEC_PARAM_TYPE_GET(operation->paramTypes, n)) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			teec_releasesharedmemory(shms + n);
			break;
		default:
			break;
		}
	}
}

static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	if (ver->impl_id == TEE_IMPL_ID_OPTEE)
		return 1;
	else
		return 0;
}

static int mtk_iommu_init_context(void)
{
	int ret;

	if (!IS_ERR_OR_NULL(mtk_iommu_ctx.ctx))
		return 0;

	mtk_iommu_ctx.ctx = tee_client_open_context(NULL, optee_ctx_match, NULL,
							NULL);
	if (IS_ERR_OR_NULL(mtk_iommu_ctx.ctx)) {
		ret = PTR_ERR(mtk_iommu_ctx.ctx);
		pr_err("%s: tee_client_open_context failed with %d\n", __func__, ret);
		return -ENODEV;
	}

	return 0;
}

static int mtk_iommu_open_session(u32 *session_id)
{
	int res;
	struct tee_ioctl_open_session_arg arg = { };
	uuid_t uuid = UUID_INIT(0x59346609, 0xbc83, 0x4d43,
					0xb6, 0x26, 0x13, 0x08,
					0x21, 0x9a, 0x16, 0xad);

	if (!session_id)
		return -EINVAL;

	if (IS_ERR_OR_NULL(mtk_iommu_ctx.ctx)) {
		res = mtk_iommu_init_context();
		if (res)
			return res;
	}

	memcpy(arg.uuid, &uuid, TEE_IOCTL_UUID_LEN);
	arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;

	res = tee_client_open_session(mtk_iommu_ctx.ctx, &arg, NULL);
	if (res) {
		pr_err("%s: tee_client_open_session failed with 0x%x\n",
			__func__, arg.ret);
		return -EINVAL;
	}

	mtk_iommu_ctx.session_initialized = true;
	*session_id = arg.session;

	return 0;
}

static int mtk_iommu_tee_invoke(uint32_t commandID,
		struct TEEC_Operation *operation, uint32_t *return_origin)
{
	int res;
	uint32_t eorig = 0;
	struct tee_ioctl_invoke_arg arg = { };
	struct tee_param params[TEE_PARAM_COUNT] = { };
	struct TEEC_SharedMemory_2 shm[TEE_PARAM_COUNT] = { };

	if (!operation)
		return -EINVAL;

	if (mtk_iommu_ctx.session_id == 0) {
		res = mtk_iommu_open_session(&mtk_iommu_ctx.session_id);
		if (res)
			return res;
	}

	arg.num_params = TEE_PARAM_COUNT;
	arg.session = mtk_iommu_ctx.session_id;
	arg.func = commandID;

	res = teec_pre_process_operation(mtk_iommu_ctx.ctx,
			operation, params, shm);
	if (res) {
		eorig = 1;
		goto out;
	}

	res = tee_client_invoke_func(mtk_iommu_ctx.ctx, &arg, params);
	if (res) {
		eorig = 2;
		goto out;
	}

	res = arg.ret;
	eorig = arg.ret_origin;
	teec_post_process_operation(operation, params, shm);

out:
	teec_free_temp_refs(operation, shm);
	if (return_origin)
		*return_origin = eorig;

	return res;
}

static void mtk_iommu_destroy_session(void)
{
	if (!mtk_iommu_ctx.session_initialized)
		return;

	tee_client_close_context(mtk_iommu_ctx.ctx);
	mtk_iommu_ctx.session_initialized = false;
}

int mtk_iommu_tee_init(void)
{
	int res;
	uint32_t err_origin;
	struct TEEC_Operation op = { };

	op.paramTypes = TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
					TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
					TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
					TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	res = mtk_iommu_tee_invoke(TEE_MMA_INIT, &op, &err_origin);
	if (res)
		pr_err("%s: failed with %d\n", __func__, err_origin);

	return res;
}

int mtk_iommu_tee_map_page(enum mtk_iommu_addr_type type,
		const char *space_tag, struct page *page,
		size_t size, unsigned long offset, bool secure, u64 *va_out)
{
	int res;
	uint32_t err_origin = 0;
	struct tee_map_args_v2 args = { };
	char tag[MAX_NAME_SIZE] = { };
	struct mtk_iommu_range_t *pa = NULL;
	struct TEEC_Operation op = { };

	if (va_out == NULL)
		return -EINVAL;

	pa = kvmalloc(sizeof(struct mtk_iommu_range_t), GFP_KERNEL);
	if (!pa)
		return -ENOMEM;

	args.size = size;
	args.pa_num = 1;
	args.flag = 0;
	pa->start = page_to_phys(page) + offset;
	pa->size = size;

	if (space_tag != NULL) {
		strncpy(tag, space_tag, MAX_NAME_SIZE);
		tag[MAX_NAME_SIZE - 1] = '\0';
	}
	args.type = type;
	args.is_secure = secure;

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT);
	op.params[0].tmpref.buffer = (uint8_t *)&args;
	op.params[0].tmpref.size = sizeof(args);
	op.params[1].tmpref.buffer = (uint8_t *)tag;
	op.params[1].tmpref.size = MAX_NAME_SIZE;
	op.params[2].tmpref.buffer = (uint8_t *)pa;
	op.params[2].tmpref.size = sizeof(struct mtk_iommu_range_t);
	op.params[3].tmpref.buffer = (uint8_t *)va_out;
	op.params[3].tmpref.size = sizeof(uint64_t);

	res = mtk_iommu_tee_invoke(MAP_IOVA, &op, &err_origin);
	if (res)
		pr_err("failed to  MAP res:0x%x err: 0x%x\n", res, err_origin);
	kvfree(pa);

	return res;
}

int mtk_iommu_tee_map(const char *space_tag, struct sg_table *sgt,
		      u64 *va_out, bool dou, const char *buf_tag)
{
	int res;
	uint32_t err_origin = 0;
	struct tee_map_args_v2 args = { };
	char tag[MAX_NAME_SIZE] = { };
	unsigned int i = 0;
	struct mtk_iommu_range_t *pa_list = NULL;
	struct scatterlist *s;
	struct TEEC_Operation op = { };

	if (!sgt || !va_out)
		return -EINVAL;

	pa_list = kvmalloc(sgt->orig_nents * sizeof(*pa_list), GFP_KERNEL);
	if (!pa_list)
		return -ENOMEM;

	args.size = 0;
	args.pa_num = sgt->orig_nents;

	for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
		pa_list[i].start = page_to_phys(sg_page(s)) + s->offset;
		pa_list[i].size = s->length;
		args.size += s->length;
	}
	if (space_tag != NULL) {
		strncpy(tag, space_tag, MAX_NAME_SIZE);
		tag[MAX_NAME_SIZE - 1] = '\0';
	}
	if (buf_tag != NULL) {
		strncpy(args.buffer_tag, buf_tag, MAX_NAME_SIZE);
		args.buffer_tag[MAX_NAME_SIZE - 1] = '\0';
	}

	args.type = MTK_IOMMU_ADDR_TYPE_IOVA;
	args.is_secure = 0;
	if (dou)
		args.flag = MMA_FLAG_2XIOVA;
	else
		args.flag = 0;
	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT);
	op.params[0].tmpref.buffer = (uint8_t *)&args;
	op.params[0].tmpref.size = sizeof(args);
	op.params[1].tmpref.buffer = (uint8_t *)tag;
	op.params[1].tmpref.size = MAX_NAME_SIZE;
	op.params[2].tmpref.buffer = (uint8_t *)pa_list;
	op.params[2].tmpref.size = sgt->orig_nents * sizeof(*pa_list);
	op.params[3].tmpref.buffer = (uint8_t *)va_out;
	op.params[3].tmpref.size = sizeof(uint64_t);

	res = mtk_iommu_tee_invoke(MAP_IOVA, &op, &err_origin);
	if (res)
		pr_err("failed to  MAP res:0x%x err: 0x%x\n", res, err_origin);
	kvfree(pa_list);

	return res;
}

int mtk_iommu_tee_unmap(enum mtk_iommu_addr_type type, uint64_t va,
			struct mtk_iommu_range_t *va_out)
{
	int res;
	uint32_t err_origin = 0;
	struct TEEC_Operation op = { };

	if (!va_out)
		return -EINVAL;

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].value.a = type;
	op.params[1].tmpref.buffer = (uint8_t *) (&va);
	op.params[1].tmpref.size = sizeof(uint64_t);
	op.params[2].tmpref.buffer = (uint8_t *) (va_out);
	op.params[2].tmpref.size = sizeof(*va_out);

	res = mtk_iommu_tee_invoke(UNMAP_IOVA, &op, &err_origin);
	if (res)
		pr_err("failed to  UMAP res:0x%x err: 0x%x\n", res, err_origin);

	return res;
}

int mtk_iommu_tee_set_mpu_area(enum mtk_iommu_addr_type type,
		struct mtk_iommu_range_t *range, uint32_t num)
{
	int res;
	uint32_t err_origin = 0;
	struct TEEC_Operation op = { };

	if (!range)
		return -EINVAL;

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].value.a = type;
	op.params[1].tmpref.buffer = (uint8_t *) range;
	op.params[1].tmpref.size = num * sizeof(*range);

	res = mtk_iommu_tee_invoke(SET_MPU_AREA, &op, &err_origin);
	if (res) {
		pr_err("failed to SetSecure_range  res:0x%x err: 0x%x\n",
				res, err_origin);
		mtk_iommu_destroy_session();
	}

	return res;
}

int mtk_iommu_tee_authorize(u64 va, u32 buffer_size,
		const char *buf_tag, uint32_t pipe_id)
{
	int res;
	uint32_t err_origin = 0;
	struct TEE_SvpMmaPipeInfo svp_pipe_info = { };
	struct TEEC_Operation op = { };

	if (!buf_tag)
		return -EINVAL;

	memset(&svp_pipe_info, 0, sizeof(svp_pipe_info));
	strncpy(svp_pipe_info.buffer_tag, buf_tag, MAX_NAME_SIZE);
	svp_pipe_info.buffer_tag[MAX_NAME_SIZE - 1] = '\0';
	svp_pipe_info.u32bufferTagLen = min_t(unsigned int, strlen(buf_tag),
			MAX_NAME_SIZE);
	svp_pipe_info.u64BufferAddr = va;
	svp_pipe_info.u32BufferSize = buffer_size;

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].tmpref.buffer = (uint8_t *)&svp_pipe_info;
	op.params[0].tmpref.size = sizeof(svp_pipe_info);
	op.params[1].value.a = pipe_id;

	res = mtk_iommu_tee_invoke(VA_AUTHORIZE, &op, &err_origin);
	if (res)
		pr_err("failed to SET_authorize res:0x%x err: 0x%x\n",
				res, err_origin);

	return res;
}

int mtk_iommu_tee_unauthorize(u64 va, u32 buffer_size, const char *buf_tag,
			      uint32_t pipe_id,
			      struct mtk_iommu_range_t *rang_out)
{
	int res;
	uint32_t err_origin = 0;
	struct TEE_SvpMmaPipeInfo svp_pipe_info = { };
	struct TEEC_Operation op = { };

	if (!buf_tag || !rang_out)
		return -EINVAL;

	strncpy(svp_pipe_info.buffer_tag, buf_tag, MAX_NAME_SIZE);
	svp_pipe_info.buffer_tag[MAX_NAME_SIZE - 1] = '\0';
	svp_pipe_info.u32bufferTagLen = min_t(unsigned int, strlen(buf_tag),
			MAX_NAME_SIZE);
	svp_pipe_info.u64BufferAddr = va;
	svp_pipe_info.u32BufferSize = buffer_size;

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].tmpref.buffer = (uint8_t *)&svp_pipe_info;
	op.params[0].tmpref.size = sizeof(svp_pipe_info);
	op.params[1].value.a = pipe_id;
	op.params[2].tmpref.buffer = (uint8_t *)rang_out;
	op.params[2].tmpref.size = sizeof(*rang_out);

	res = mtk_iommu_tee_invoke(VA_UNAUTHORIZE, &op, &err_origin);
	if (res)
		pr_err("failed to SET_unauthorize res:0x%x err: 0x%x\n",
			   res, err_origin);

	return res;
}

int mtk_iommu_tee_reserve_space(enum mtk_iommu_addr_type type,
		const char *space_tag, u64 size, u64 *va_out)
{
	int res;
	uint32_t err_origin = 0;
	char tag[MAX_NAME_SIZE] = { };
	struct TEEC_Operation op = { };

	if (!space_tag || !va_out)
		return -EINVAL;

	strncpy(tag, space_tag, MAX_NAME_SIZE);
	tag[MAX_NAME_SIZE - 1] = '\0';

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT);
	op.params[0].value.a = type;
	op.params[1].tmpref.buffer = (uint8_t *)tag;
	op.params[1].tmpref.size = MAX_NAME_SIZE;
	op.params[2].tmpref.buffer = (uint8_t *)&size;
	op.params[2].tmpref.size = sizeof(uint64_t);
	op.params[3].tmpref.buffer = (uint8_t *)va_out;
	op.params[3].tmpref.size = sizeof(uint64_t);

	res = mtk_iommu_tee_invoke(RESERVE_IOVA_SPACE, &op, &err_origin);
	if (res)
		pr_err("failed to  RESERVE res:0x%x err: 0x%x\n",
			   res, err_origin);

	return res;
}

int mtk_iommu_tee_free_space(enum mtk_iommu_addr_type type,
		const char *space_tag)
{
	int res;
	uint32_t err_origin = 0;
	char tag[MAX_NAME_SIZE] = { };
	struct TEEC_Operation op = { };

	if (space_tag == NULL)
		return -EINVAL;

	strncpy(tag, space_tag, MAX_NAME_SIZE);
	tag[MAX_NAME_SIZE - 1] = '\0';

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].value.a = type;
	op.params[1].tmpref.buffer = (uint8_t *)space_tag;
	op.params[1].tmpref.size = MAX_NAME_SIZE;

	res = mtk_iommu_tee_invoke(FREE_IOVA_SPACE, &op, &err_origin);
	if (res)
		pr_err("failed to free_space res:0x%x err: 0x%x\n",
			   res, err_origin);

	return res;
}

int mtk_iommu_tee_debug(uint32_t debug_type,
			uint8_t *buf1, uint8_t size1,
			uint8_t *buf2, uint8_t size2,
			uint8_t *buf3, uint8_t size3)
{
	int res;
	uint32_t err_origin = 0;
	struct TEEC_Operation op = { };

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].value.a = debug_type;
	op.params[1].tmpref.buffer = (uint8_t *)buf1;
	op.params[1].tmpref.size = size1;
	op.params[2].tmpref.buffer = (uint8_t *)buf2;
	op.params[2].tmpref.size = size2;
	op.params[3].tmpref.buffer = (uint8_t *)buf3;
	op.params[3].tmpref.size = size3;

	res = mtk_iommu_tee_invoke(DEBUG, &op, &err_origin);
	if (res)
		pr_err("failed to  DEBUG res:0x%x err: 0x%x\n",
			   res, err_origin);

	return res;
}

int mtk_iommu_tee_lockdebug(struct tee_map_lock *lock)
{
	int res;
	uint32_t err_origin = 0;
	struct TEEC_Operation op = { };

	if (!lock)
		return -EINVAL;

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].tmpref.buffer = (uint8_t *)lock;
	op.params[0].tmpref.size = sizeof(struct tee_map_lock);
	pr_err("LOCK_DEBUG tag=%s,num=%d,aid=%d,%d,%d,%d,%d,%d,%d,%d\n",
		lock->buffer_tag, lock->aid_num, lock->aid[0], lock->aid[1],
		lock->aid[2], lock->aid[3], lock->aid[4], lock->aid[5],
		lock->aid[6], lock->aid[7]);

	res = mtk_iommu_tee_invoke(LOCK_DEBUG, &op, &err_origin);
	if (res)
		pr_err("failed to  LOCK_DEBUG res:0x%x err: 0x%x\n",
			res, err_origin);

	return res;
}

int mtk_iommu_tee_get_pipelineid(uint32_t *id)
{
	int res;
	uint32_t err_origin = 0;
	struct TEEC_Operation op = { };

	if (!id)
		return -EINVAL;

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);

	res = mtk_iommu_tee_invoke(GENERATE_PIPELINE_ID, &op, &err_origin);
	if (res) {
		pr_err("failed to get pipeid  res:0x%x err: 0x%x\n",
				res, err_origin);
		return res;
	}

	*id = op.params[0].value.a;

	return 0;
}

int mtk_iommu_tee_free_pipelineid(uint32_t id)
{
	int res;
	uint32_t err_origin = 0;
	struct TEEC_Operation op = { };

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].value.a = id;
	res = mtk_iommu_tee_invoke(RELEASE_PIPELINE_ID, &op, &err_origin);
	if (res)
		pr_err("failed to release pipeid=%u  res:0x%x err: 0x%x\n",
			   id, res, err_origin);

	return res;
}

int mtk_iommu_tee_test(struct sg_table *sgt, const char *buf_tag)
{
	int res;
	uint32_t err_origin = 0;
	struct tee_map_args_v2 args = { };
	unsigned int i = 0;
	struct mtk_iommu_range_t *pa_list = NULL;
	struct scatterlist *s;
	struct TEEC_Operation op = { };

	if (sgt == NULL || buf_tag == NULL)
		return -EINVAL;

	pa_list = kvmalloc(sgt->orig_nents * sizeof(*pa_list), GFP_KERNEL);
	if (!pa_list)
		return -ENOMEM;

	args.size = 0;
	args.pa_num = sgt->orig_nents;

	for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
		pa_list[i].start = page_to_phys(sg_page(s)) + s->offset;
		pa_list[i].size = s->length;
		args.size += s->length;
	}

	if (buf_tag) {
		strncpy(args.buffer_tag, buf_tag, MAX_NAME_SIZE);
		args.buffer_tag[MAX_NAME_SIZE - 1] = '\0';
	}

	args.type = MTK_IOMMU_ADDR_TYPE_IOVA;
	args.is_secure = 0;
	args.flag = 0;
	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].tmpref.buffer = (uint8_t *)&args;
	op.params[0].tmpref.size = sizeof(args);
	op.params[1].tmpref.buffer = (uint8_t *)pa_list;
	op.params[1].tmpref.size = sgt->orig_nents * sizeof(*pa_list);

	res = mtk_iommu_tee_invoke(IOMMU_TEST, &op, &err_origin);
	if (res)
		pr_err("failed to IOMMU_TEST res:0x%x err: 0x%x\n",
			res, err_origin);
	kvfree(pa_list);

	return res;
}

static struct buf_tag *dump_buftags(int *num)
{
	struct list_head *head = NULL, *pos = NULL;
	struct buf_tag_info *buf = NULL;
	struct buf_tag *ret_tags = NULL;
	int count = 0;

	head = mtk_iommu_get_buftags();
	if (!head || list_empty(head))
		return NULL;

	list_for_each(pos, head)
		count++;

	ret_tags = kzalloc(sizeof(struct buf_tag) * count, GFP_KERNEL);
	if (!ret_tags)
		return NULL;

	count = 0;
	list_for_each_entry(buf, head, list) {
		ret_tags[count].heap_type = buf->heap_type;
		if (ret_tags[count].heap_type == HEAP_TYPE_CARVEOUT)
			ret_tags[count].heap_type = HEAP_TYPE_IOMMU;

		ret_tags[count].miu = buf->miu;
		ret_tags[count].maxsize = buf->maxsize;
		strncpy(ret_tags[count].name, buf->name, MAX_NAME_SIZE);
		ret_tags[count].name[MAX_NAME_SIZE - 1] = '\0';
		count++;
	}
	*num = count;

	return ret_tags;
}

int mtk_iommu_optee_ta_store_buf_tags(void)
{
	int res;
	uint32_t err_origin = 0;
	struct buf_tag *buf = NULL;
	int size = 0;
	struct TEEC_Operation op = { };

	buf = dump_buftags(&size);
	if (!buf)
		return 0;

	op.paramTypes =
		TEEC_PARAM_TYPES(TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE,
				TEE_IOCTL_PARAM_ATTR_TYPE_NONE);
	op.params[0].tmpref.buffer = (uint8_t *)buf;
	op.params[0].tmpref.size = size * sizeof(struct buf_tag);

	res = mtk_iommu_tee_invoke(STORE_BUF_TAGS, &op, &err_origin);
	if (res)
		pr_err("failed to  STORE_BUF_TAGS res:0x%x err: 0x%x\n",
			   res, err_origin);
	kfree(buf);

	return res;
}

int mtk_iommu_tee_open_session(void)
{
	int ret;

	if (mtk_iommu_ctx.session_initialized) {
		pr_info("mma ta session alread init\n");
		return 0;
	}

	ret = mtk_iommu_init_context();
	if (ret)
		return ret;

	ret = mtk_iommu_open_session(&mtk_iommu_ctx.session_id);
	if (ret)
		return ret;

	return 0;
}

int mtk_iommu_tee_close_session(void)
{
	mtk_iommu_destroy_session();
	return 0;
}
