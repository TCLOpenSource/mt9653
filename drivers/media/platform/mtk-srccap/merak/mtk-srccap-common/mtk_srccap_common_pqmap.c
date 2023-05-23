// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include "mtk_srccap.h"
#include "mtk_srccap_common.h"
#include "mtk_srccap_dscl.h"
#include "pqmap_utility.h"
#include "mapi_pq_src_reg_list.h"
#include "drv_scriptmgt.h"
#include "mapi_pq_nts_reg_if.h"
#include "mapi_pq_if.h"
#include "avd-ex.h"

/* ============================================================================================== */
/* --------------------------------------- Local Define ----------------------------------------- */
/* ============================================================================================== */

#define MAX_DEV_NUM		2
#define RULE_MAP_NUM		2
#define FILE_NAME_CHAR_NUM	256
#define malloc(size)		kmalloc(size, GFP_KERNEL)
#define free			kfree
#define UNUSED_PARA(x)		(void)(x)

/* ============================================================================================== */
/* --------------------------------------- Local Define ----------------------------------------- */
/* ============================================================================================== */
static bool _h_scalingmode[MAX_DEV_NUM] = {1, 1};
static bool _v_scalingmode[MAX_DEV_NUM] = {1, 1};

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int _mtk_srccap_common_map_fd(int fd, void **va, u64 *size)
{
	int ret = 0;
	struct dma_buf *db = NULL;

	if ((size == NULL) || (fd < 0) || (va == NULL)) {
		SRCCAP_MSG_ERROR("Invalid input, fd=(%d), size is NULL?(%s), va is NULL?(%s)\n",
			fd, (size == NULL)?"TRUE":"FALSE", (va == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	db = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(db)) {
		SRCCAP_MSG_ERROR("dma_buf_get fail, db=0x%x\n", db);
		return -EPERM;
	}

	*size = db->size;

	*va = dma_buf_vmap(db);

	if (!*va || (*va == -1)) {
		SRCCAP_MSG_ERROR("dma_buf_vmapfail\n");
		dma_buf_put(db);
		return -EPERM;
	}

	dma_buf_put(db);

	SRCCAP_MSG_INFO("va=0x%llx, size=%llu\n", *va, *size);
	return ret;
}

static int _mtk_srccap_common_unmap_fd(int fd, void *va)
{
	struct dma_buf *db = NULL;

	if ((va == NULL) || (fd < 0)) {
		SRCCAP_MSG_ERROR("Invalid input, fd=(%d), va is NULL?(%s)\n",
			fd, (va == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	db = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(db)) {
		SRCCAP_MSG_ERROR("dma_buf_get fail, db=0x%x\n", db);
		return -1;
	}

	dma_buf_vunmap(db, va);
	dma_buf_put(db);
	return 0;
}

int _mtk_srccap_common_store_data(uint32_t idx, uint32_t val)
{
	if (idx == E_BIPDMA0_REG_PREHSDMODE)
		_h_scalingmode[0] = (bool)val;
	else if (idx == E_BIPDMA0_REG_PRE_VDOWN_MODE)
		_v_scalingmode[0] = (bool)val;
	else if (idx == E_BIPDMA1_REG_PREHSDMODE)
		_h_scalingmode[1] = (bool)val;
	else if (idx == E_BIPDMA1_REG_PRE_VDOWN_MODE)
		_v_scalingmode[1] = (bool)val;

	return 0;
}

char _mtk_srccap_common_pim_callback(
	int tag, const char *data,
	const size_t datalen, void *pCtx)

{
	struct ST_PQ_NTS_HWREG_INFO stHWRegInfo;
	struct ST_PQ_NTS_HWREG_STATUS stHWRegStatus;
	int ret = 0;

	memset(&stHWRegInfo, 0, sizeof(struct ST_PQ_NTS_HWREG_INFO));
	memset(&stHWRegStatus, 0, sizeof(struct ST_PQ_NTS_HWREG_STATUS));

	UNUSED_PARA(pCtx);
	if (!data) {
		SRCCAP_MSG_ERROR("pim callback data = NULL\n");
		return false;
	}

	if (tag == EXTMEM_TAG_REGISTER_SETTINGS) {
		reg_func *regfunc = (reg_func *)data;
		int i = 0;

		SRCCAP_MSG_DEBUG("HWREG num = %u\n", regfunc->padded_data.fn_num);
		for (i = 0; i < regfunc->padded_data.fn_num; ++i) {
			uint32_t fnidx = regfunc->fn_tbl[i].fn_idx;

			_mtk_srccap_common_store_data(fnidx, regfunc->fn_tbl[i].fn_para);
			stHWRegInfo.u32RegIdx = regfunc->fn_tbl[i].fn_idx;
			stHWRegInfo.u16Value = regfunc->fn_tbl[i].fn_para;
			stHWRegInfo.eDomain = E_PQ_DOMAIN_SOURCE;
			ret = MApi_PQ_SetHWReg_NonTs(pCtx, &stHWRegInfo, &stHWRegStatus);
			if (ret != E_PQAPI_RC_SUCCESS)
				SRCCAP_MSG_ERROR("MApi_PQ_SetHWReg_NonTs fail, ret:0x%x\n", ret);

			SRCCAP_MSG_DEBUG("HWREG[0x%x]:%u,u32RegIdx:%u u16Value:%u\n",
				fnidx, regfunc->fn_tbl[i].fn_para,
				stHWRegInfo.u32RegIdx, stHWRegInfo.u16Value);

		}
		return true;
	} else if (tag == EXTMEM_TAG_VREGISTER_SETTINGS) {
		reg_func *regfunc = (reg_func *)data;
		int i = 0;

		SRCCAP_MSG_DEBUG("VRREG num = %u\n", regfunc->padded_data.fn_num);
		for (i = 0; i < regfunc->padded_data.fn_num; ++i) {
			uint32_t fnidx = regfunc->fn_tbl[i].fn_idx;

			SRCCAP_MSG_DEBUG("VR[0x%x]:%u\n", fnidx, regfunc->fn_tbl[i].fn_para);
		}

		return true;
	}

	return false;
}


static int _MI_PQ_Apply_InsertHandler(void *pPim, const char *HandlerName)
{
	if (!pPim) {
		SRCCAP_MSG_ERROR("pPim Haven't been inited\n");
		return -ENXIO;
	}

	if (!HandlerName) {
		SRCCAP_MSG_ERROR("wrong file name\n");
		return -ENOENT;
	}

	if (!pqmap_pim_insert_handler(pPim, HandlerName, NULL)) {
		SRCCAP_MSG_ERROR("insert handler[%s] failed\n", HandlerName);
		return -EPERM;
	}

	return 0;
}

char _mtk_srccap_common_vd_pim_callback(
	int tag, const char *data,
	const size_t datalen, void *pCtx)

{
	struct ST_PQ_NTS_HWREG_INFO stHWRegInfo;
	struct ST_PQ_NTS_HWREG_STATUS stHWRegStatus;
	int ret = 0;

	memset(&stHWRegInfo, 0, sizeof(struct ST_PQ_NTS_HWREG_INFO));
	memset(&stHWRegStatus, 0, sizeof(struct ST_PQ_NTS_HWREG_STATUS));

	UNUSED_PARA(pCtx);
	if (!data) {
		SRCCAP_AVD_MSG_ERROR("pim callback data = NULL\n");
		return false;
	}

	if (tag == EXTMEM_TAG_REGISTER_SETTINGS) {
		reg_func *regfunc = (reg_func *)data;
		int i = 0;

		SRCCAP_AVD_MSG_INFO("HWREG num = %u\n", regfunc->padded_data.fn_num);
		for (i = 0; i < regfunc->padded_data.fn_num; ++i) {
			uint32_t fnidx = regfunc->fn_tbl[i].fn_idx;

			stHWRegInfo.u32RegIdx = regfunc->fn_tbl[i].fn_idx;
			stHWRegInfo.u16Value = regfunc->fn_tbl[i].fn_para;
			stHWRegInfo.eDomain = E_PQ_DOMAIN_VD;
			ret = MApi_PQ_SetHWReg_NonTs_Imm(pCtx, &stHWRegInfo, &stHWRegStatus);
			if (ret != E_PQAPI_RC_SUCCESS)
				SRCCAP_AVD_MSG_INFO("MApi_PQ_SetHWReg_NonTs fail, ret:0x%x\n", ret);


			SRCCAP_AVD_MSG_DEBUG("HWREG[0x%x]:%u,u32RegIdx:%u u16Value:%u\n",
				fnidx, regfunc->fn_tbl[i].fn_para,
				stHWRegInfo.u32RegIdx, stHWRegInfo.u16Value);

		}
		return true;
	} else if (tag == EXTMEM_TAG_VREGISTER_SETTINGS) {
		reg_func *regfunc = (reg_func *)data;
		int i = 0;

		SRCCAP_AVD_MSG_INFO("VRREG num = %u\n", regfunc->padded_data.fn_num);
		for (i = 0; i < regfunc->padded_data.fn_num; ++i) {
			uint32_t fnidx = regfunc->fn_tbl[i].fn_idx;

			SRCCAP_AVD_MSG_INFO("VR[0x%x]:%u\n", fnidx, regfunc->fn_tbl[i].fn_para);
		}

		return true;
	}

	return false;
}

static int _mtk_srccap_common_init_pim(void **ppPim, char *buf, size_t buf_len)
{
	int ret = 0;

	if (*ppPim) {
		if (pqmap_pim_destroy(ppPim) == FALSE)
			SRCCAP_MSG_ERROR("pqmap_pim_destroy failed\n");

		*ppPim = NULL;
	}

	if (!pqmap_pim_init(ppPim, buf, buf_len)) {
		ret = -EPERM;
		SRCCAP_MSG_ERROR("srccap PIM init failed\n");
	}

	return ret;
}

static int _mtk_srccap_common_deinit_pqmap(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	u16 dev_id = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	dev_id = srccap_dev->dev_id;
	if (dev_id > 1) {
		SRCCAP_MSG_ERROR("device id not found\n");
		return -EPERM;
	}
	if (srccap_dev->common_info.pqmap_info.inited == false) {
		SRCCAP_MSG_ERROR("srccap pqmap[%d] has not been inited\n", dev_id);
		return -EPERM;
	}

	SRCCAP_MSG_INFO("deinit srccap pqmap[%d]\n", srccap_dev->dev_id);

	if (srccap_dev->common_info.pqmap_info.pPim) {
		if (!pqmap_pim_destroy(&srccap_dev->common_info.pqmap_info.pPim)) {
			SRCCAP_MSG_ERROR("Destroy pim[%d] failed\n", dev_id);
			ret = -EPERM;
		}
	}

	if (!srccap_dev->common_info.pqmap_info.pqmap_va)
		vfree(srccap_dev->common_info.pqmap_info.pqmap_va);
	memset(&srccap_dev->common_info.pqmap_info, 0, sizeof(struct srccap_common_pqmap_info));

	return ret;
}

static int _mtk_srccap_common_deinit_vd_pqmap(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	u16 dev_id = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	dev_id = srccap_dev->dev_id;
	if (dev_id > 1) {
		SRCCAP_AVD_MSG_ERROR("device id not found\n");
		return -EPERM;
	}

	if (srccap_dev->common_info.vdpqmap_info.inited == false) {
		SRCCAP_AVD_MSG_ERROR("srccap pqmap[%d] has not been inited\n", dev_id);
		return -EPERM;
	}

	SRCCAP_AVD_MSG_INFO("deinit srccap pqmap[%d]\n", srccap_dev->dev_id);

	if (srccap_dev->common_info.vdpqmap_info.pPim) {
		if (!pqmap_pim_destroy(&srccap_dev->common_info.vdpqmap_info.pPim)) {
			SRCCAP_AVD_MSG_ERROR("Destroy pim[%d] failed\n", dev_id);
			ret = -EPERM;
		}
	}

	if (!srccap_dev->common_info.vdpqmap_info.pqmap_va)
		vfree(srccap_dev->common_info.vdpqmap_info.pqmap_va);
	memset(&srccap_dev->common_info.vdpqmap_info, 0, sizeof(struct srccap_common_pqmap_info));

	return ret;
}

static int _mtk_srccap_common_init_pqmap(struct v4l2_srccap_pqmap_info *info,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	u16 dev_id = 0;
	void *va = NULL;
	u64 size = 0;

	if ((info == NULL) || (srccap_dev == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	dev_id = srccap_dev->dev_id;
	if (dev_id > 1) {
		SRCCAP_MSG_ERROR("device id not found\n");
		return -EPERM;
	}
	if (srccap_dev->common_info.pqmap_info.inited == true) {
		SRCCAP_MSG_ERROR("srccap pqmap[%d] has already inited\n", dev_id);
		return -EPERM;
	}

	if (_mtk_srccap_common_map_fd(info->pqmap_fd, &va, &size) != 0) {
		_mtk_srccap_common_unmap_fd(info->pqmap_fd, va);
		return -EFAULT;
	}

	if (size < info->u32PimLen) {
		SRCCAP_MSG_ERROR("srccap pqmap memory is not enough\n");
		_mtk_srccap_common_unmap_fd(info->pqmap_fd, va);
		return -EPERM;
	}
	srccap_dev->common_info.pqmap_info.pqmap_va = vmalloc(size);
	if (srccap_dev->common_info.pqmap_info.pqmap_va == NULL) {
		SRCCAP_MSG_ERROR("srccap pqmap vmalloc failed\n");
		return -EPERM;
	}
	memcpy(srccap_dev->common_info.pqmap_info.pqmap_va, va, size);
	_mtk_srccap_common_unmap_fd(info->pqmap_fd, va);

	srccap_dev->common_info.pqmap_info.pim_len = info->u32PimLen;
	srccap_dev->common_info.pqmap_info.pim_va =
		(char *)srccap_dev->common_info.pqmap_info.pqmap_va;

	//pim init
	if (_mtk_srccap_common_init_pim(&srccap_dev->common_info.pqmap_info.pPim,
		srccap_dev->common_info.pqmap_info.pim_va,
		srccap_dev->common_info.pqmap_info.pim_len) != 0) {
		SRCCAP_MSG_ERROR("srccap PIM[%d] init failed\n", dev_id);
		ret = -EPERM;
		goto cleanup;
	}

	srccap_dev->common_info.pqmap_info.inited = true;
	SRCCAP_MSG_INFO("init qmap[%d] succsessed !!!\n", dev_id);
	return ret;

cleanup:
	_mtk_srccap_common_deinit_pqmap(srccap_dev);
	return ret;
}

static int _mtk_srccap_common_init_vd_pqmap(struct v4l2_srccap_pqmap_info *info,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	u16 dev_id = 0;
	void *va = NULL;
	u64 size = 0;

	if ((info == NULL) || (srccap_dev == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	dev_id = srccap_dev->dev_id;
	if (dev_id > 1) {
		SRCCAP_AVD_MSG_ERROR("device id not found\n");
		return -EPERM;
	}
	if (srccap_dev->common_info.vdpqmap_info.inited == true) {
		SRCCAP_AVD_MSG_ERROR("srccap pqmap[%d] has already inited\n", dev_id);
		return -EPERM;
	}

	if (_mtk_srccap_common_map_fd(info->pqmap_fd, &va, &size) != 0) {
		_mtk_srccap_common_unmap_fd(info->pqmap_fd, va);
		return -EFAULT;
	}

	if (size < info->u32PimLen) {
		SRCCAP_AVD_MSG_ERROR("srccap pqmap memory is not enough\n");
		_mtk_srccap_common_unmap_fd(info->pqmap_fd, va);
		return -EPERM;
	}
	srccap_dev->common_info.vdpqmap_info.pqmap_va = vmalloc(size);
	if (srccap_dev->common_info.vdpqmap_info.pqmap_va == NULL) {
		SRCCAP_AVD_MSG_ERROR("srccap pqmap vmalloc failed\n");
		return -EPERM;
	}
	memcpy(srccap_dev->common_info.vdpqmap_info.pqmap_va, va, size);
	_mtk_srccap_common_unmap_fd(info->pqmap_fd, va);

	srccap_dev->common_info.vdpqmap_info.pim_len = info->u32PimLen;
	srccap_dev->common_info.vdpqmap_info.pim_va =
		(char *)srccap_dev->common_info.vdpqmap_info.pqmap_va;

	//pim init
	if (_mtk_srccap_common_init_pim(&srccap_dev->common_info.vdpqmap_info.pPim,
		srccap_dev->common_info.vdpqmap_info.pim_va,
		srccap_dev->common_info.vdpqmap_info.pim_len) != 0) {
		SRCCAP_AVD_MSG_ERROR("srccap PIM[%d] init failed\n", dev_id);
		ret = -EPERM;
		goto cleanup;
	}

	srccap_dev->common_info.vdpqmap_info.inited = true;
	SRCCAP_AVD_MSG_INFO("init qmap[%d] succsessed !!!\n", dev_id);
	return ret;

cleanup:
	_mtk_srccap_common_deinit_pqmap(srccap_dev);
	return ret;
}

int _mtk_srccap_common_trigger(void *pPim, struct mtk_srccap_dev *srccap_dev, void *pCtx)
{
	u16 *node_array = NULL;
	size_t array_size = 0;
	size_t index = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_ERROR("srccap_dev is Null\n");
		return -EFAULT;
	}

	if (!pPim) {
		SRCCAP_MSG_ERROR("srccap qmap is not inited\n");
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_pqmap_dscl);

	array_size = srccap_dev->dscl_info.rm_info.array_size;
	node_array = srccap_dev->dscl_info.rm_info.p.node_array;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_pqmap_dscl);

	for (index = 0; index < array_size; ++index)
		pqmap_pim_load_setting(pPim, node_array[index],
			0, _mtk_srccap_common_pim_callback, pCtx);

	return 0;
}

int _mtk_srccap_common_vd_trigger(void *pPim, struct mtk_srccap_dev *srccap_dev, void *pCtx)
{
	u16 *node_array = NULL;
	size_t array_size = 0;
	size_t index = 0;

	if (srccap_dev == NULL) {
		SRCCAP_AVD_MSG_ERROR("srccap_dev is Null\n");
		return -EFAULT;
	}

	if (!pPim) {
		SRCCAP_AVD_MSG_ERROR("srccap qmap is not inited\n");
		return -ENXIO;
	}

	array_size = srccap_dev->avd_info.pqRminfo.array_size;
	node_array = srccap_dev->avd_info.pqRminfo.p.node_array;

	for (index = 0; index < array_size; ++index)
		pqmap_pim_load_setting(pPim, node_array[index],
			0, _mtk_srccap_common_vd_pim_callback, pCtx);

	return 0;
}

static int srccap_common_pqmap_set_ml_result(
			struct mtk_srccap_dev *srccap_dev,
			struct srccap_ml_script_info *ml_script_info)
{
	int ret = 0;
	struct ST_PQ_ML_CTX_INFO ml_ctx_info;
	struct ST_PQ_ML_CTX_STATUS ml_ctx_status;

	if ((srccap_dev == NULL) || (ml_script_info == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	memset(&ml_ctx_info, 0, sizeof(struct ST_PQ_ML_CTX_INFO));
	memset(&ml_ctx_status, 0, sizeof(struct ST_PQ_ML_CTX_STATUS));

	ml_ctx_status.u32Version = 0;
	ml_ctx_status.u32Length = sizeof(struct ST_PQ_ML_CTX_STATUS);
	ml_ctx_info.u32Version = 0;
	ml_ctx_info.u32Length = sizeof(struct ST_PQ_ML_CTX_INFO);
	ml_ctx_info.u16PqModuleIdx = E_PQ_PIM_MODULE;

	if (srccap_dev->dev_id == 0) {
		ml_ctx_info.pSrc0Addr = ml_script_info->start_addr;
		ml_ctx_info.pSrc0MaxAddr = ml_script_info->max_addr;
	} else {
		ml_ctx_info.pSrc1Addr = ml_script_info->start_addr;
		ml_ctx_info.pSrc1MaxAddr = ml_script_info->max_addr;
	}

	SRCCAP_MSG_DEBUG("dev_id:%d, mem_index:%d, src0Addr:0x%x, src0MaxAddr:0x%x\n",
		srccap_dev->dev_id, ml_script_info->mem_index,
		ml_ctx_info.pSrc0Addr, ml_ctx_info.pSrc0MaxAddr);
	SRCCAP_MSG_DEBUG("src1Addr:0x%x, src1MaxAddr:0x%x\n",
		ml_ctx_info.pSrc1Addr, ml_ctx_info.pSrc1MaxAddr);
	ret = MApi_PQ_SetMLInfo(srccap_dev->common_info.pqmap_info.alg_ctx,
					&ml_ctx_info, &ml_ctx_status);
	if (ret != 0) {
		SRCCAP_MSG_ERROR("MApi_PQ_SetMLInfo fail, ret:0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int srccap_common_pqmap_get_ml_result(
			struct mtk_srccap_dev *srccap_dev,
			struct srccap_ml_script_info *ml_script_info)
{
	struct ST_PQ_ML_PARAM ml_param;
	struct ST_PQ_ML_INFO ml_status;
	int ret = 0;

	if ((srccap_dev == NULL) || (ml_script_info == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	memset(&ml_param, 0, sizeof(struct ST_PQ_ML_PARAM));
	memset(&ml_status, 0, sizeof(struct ST_PQ_ML_INFO));

	ml_param.u32Version = 0;
	ml_param.u32Length = sizeof(struct ST_PQ_ML_PARAM);
	ml_param.u16PqModuleIdx = E_PQ_PIM_MODULE;

	ml_status.u32Version = 0;
	ml_status.u32Length = sizeof(struct ST_PQ_ML_INFO);

	ret = MApi_PQ_GetMLInfo(srccap_dev->common_info.pqmap_info.alg_ctx, &ml_param, &ml_status);
	if (ret != E_PQAPI_RC_SUCCESS) {
		SRCCAP_MSG_ERROR("MApi_PQ_GetMLInfo fail, ret:0x%x\n", ret);
		ml_script_info->auto_gen_cmd_count = 0;
		return ret;
	}
	if (srccap_dev->dev_id == 0)
		ml_script_info->auto_gen_cmd_count = ml_status.u16Src0UsedCmdSize;
	else
		ml_script_info->auto_gen_cmd_count = ml_status.u16Src1UsedCmdSize;
	ml_script_info->total_cmd_count = ml_script_info->total_cmd_count
					+ ml_script_info->auto_gen_cmd_count;
	SRCCAP_MSG_DEBUG("dev_id:%d, auto_gen_cmd_count:%d, Src0Size:%u, Src1Size:%u\n",
		srccap_dev->dev_id, ml_script_info->auto_gen_cmd_count,
		ml_status.u16Src0UsedCmdSize, ml_status.u16Src1UsedCmdSize);
	SRCCAP_MSG_DEBUG("total_cmd_count:%d\n",
		ml_script_info->total_cmd_count);
	return ret;
}

int _mtk_srccap_common_load_pqmap(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_ml_script_info *ml_script_info)
{
	void *pPim = NULL;
	int ret = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev->dev_id > 1) {
		SRCCAP_MSG_ERROR("device id not found\n");
		return -EPERM;
	}

	pPim = srccap_dev->common_info.pqmap_info.pPim;
	if (!srccap_dev->common_info.pqmap_info.inited) {
		SRCCAP_MSG_ERROR("srccap qmap[%d] is not inited\n", srccap_dev->dev_id);
		return -ENXIO;
	}

	if (!pPim) {
		SRCCAP_MSG_ERROR("srccap pim[%d] is not inited\n", srccap_dev->dev_id);
		return -ENXIO;
	}

	srccap_common_pqmap_set_ml_result(srccap_dev, ml_script_info);

	if (_mtk_srccap_common_trigger(pPim, srccap_dev,
		srccap_dev->common_info.pqmap_info.alg_ctx) != 0) {
		SRCCAP_MSG_ERROR("_mtk_srccap_common_trigger fail, ret:%d\n", ret);
		return -EPERM;
	}

	srccap_common_pqmap_get_ml_result(srccap_dev, ml_script_info);

	srccap_dev->dscl_info.h_scalingmode = _h_scalingmode[srccap_dev->dev_id];
	srccap_dev->dscl_info.v_scalingmode = _v_scalingmode[srccap_dev->dev_id];

	return 0;
}

int _mtk_srccap_common_load_vd_pqmap(struct mtk_srccap_dev *srccap_dev)
{
	void *pPim = NULL;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_AVD_MSG_INFO("load srccap pqmap[%d]\n", srccap_dev->dev_id);
	if (srccap_dev->dev_id > 1) {
		SRCCAP_AVD_MSG_ERROR("device id not found\n");
		return -EPERM;
	}

	pPim = srccap_dev->common_info.vdpqmap_info.pPim;
	if (!srccap_dev->common_info.vdpqmap_info.inited) {
		SRCCAP_AVD_MSG_ERROR("srccap qmap[%d] is not inited\n", srccap_dev->dev_id);
		return -ENXIO;
	}

	if (!pPim) {
		SRCCAP_AVD_MSG_ERROR("srccap pim[%d] is not inited\n", srccap_dev->dev_id);
		return -ENXIO;
	}

	if (_mtk_srccap_common_vd_trigger(pPim, srccap_dev,
		NULL) != 0) {
		SRCCAP_AVD_MSG_ERROR("get result failed\n");
		return -EPERM;
	}

	return 0;
}

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */

int mtk_srccap_common_set_pqmap_info(struct v4l2_srccap_pqmap_info *info,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	if ((info == NULL) || (srccap_dev == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (info->eDomain_type == V4L2_SRCCAP_PQ_DOMAIN_INTPUT) {
		if (info->bRuin == false) {
			SRCCAP_MSG_INFO("Init srccap pqmap\n");
			ret = _mtk_srccap_common_init_pqmap(info, srccap_dev);
		} else {
			SRCCAP_MSG_INFO("DeInit srccap pqmap\n");
			ret = _mtk_srccap_common_deinit_pqmap(srccap_dev);
		}
	} else if (info->eDomain_type == V4L2_SRCCAP_PQ_DOMAIN_VD) {
		if (info->bRuin == false) {
			SRCCAP_AVD_MSG_INFO("Init srccap vd pqmap\n");
			ret = _mtk_srccap_common_init_vd_pqmap(info, srccap_dev);
		} else {
			SRCCAP_AVD_MSG_INFO("DeInit srccap vd pqmap\n");
			ret = _mtk_srccap_common_deinit_vd_pqmap(srccap_dev);
		}
	} else
		SRCCAP_MSG_ERROR("Invalid srccap eDomain type(%d)\n", info->eDomain_type);
	return ret;
}

int mtk_srccap_common_get_pqmap_info(struct v4l2_srccap_pqmap_info *info,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	if ((info == NULL) || (srccap_dev == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	info->bInited = srccap_dev->common_info.pqmap_info.inited;
	info->bVdInited = srccap_dev->common_info.vdpqmap_info.inited;

	return ret;
}

int mtk_srccap_common_load_pqmap(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_ml_script_info *ml_script_info)
{
	return _mtk_srccap_common_load_pqmap(srccap_dev, ml_script_info);
}

int mtk_srccap_common_load_vd_pqmap(struct mtk_srccap_dev *srccap_dev)
{
	SRCCAP_AVD_MSG_INFO("load srccap vd pqmap\n");
	if (srccap_dev->srccap_info.stub_mode == true) {
		SRCCAP_MSG_INFO("Skip load srccap pqmap, stub mode = %d\n",
			srccap_dev->srccap_info.stub_mode);
		return 0;
	}
	return _mtk_srccap_common_load_vd_pqmap(srccap_dev);
}
