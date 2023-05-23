// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/dma-buf.h>
#include <drm/mtk_tv_drm.h>

#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_metabuf.h"
#include "mtk_tv_drm_pqmap.h"
#include "mtk_tv_drm_log.h"
#include "pqmap_utility.h"

//--------------------------------------------------------------------------------------------------
//  Local Defines
//--------------------------------------------------------------------------------------------------
#define pqmap_to_kms(x) container_of(x, struct mtk_tv_kms_context, pqmap_priv)
#define MTK_DRM_MODEL MTK_DRM_MODEL_PQMAP
#define pctx_pqmap_to_kms(x) container_of(x, struct mtk_tv_kms_context, pqmap_priv)
#define ERR(fmt, args...) DRM_ERROR("[%s][%d] " fmt, __func__, __LINE__, ##args)
#define LOG(fmt, args...) DRM_INFO("[%s][%d] " fmt, __func__, __LINE__, ##args)

// TODO: Add ml module and move to it.
#define MTK_ML_REG_SIZE		8

//--------------------------------------------------------------------------------------------------
//  Local Enum and Structures
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Local Variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Local Functions
//--------------------------------------------------------------------------------------------------
static char _mtk_render_pqmap_pim_callback(
	int tag,
	const char *data,
	const size_t datalen,
	void *autogen)
{
	reg_func *regfunc = (reg_func *)data;
	uint32_t reg_idx;
	uint32_t reg_value;
	int ret = true;
	int i;

	if (!regfunc) {
		ERR("regfunc = NULL");
		return false;
	}
	if (tag == EXTMEM_TAG_REGISTER_SETTINGS) {
		LOG("NTS_HWREG num = %u", regfunc->padded_data.fn_num);
		for (i = 0; i < regfunc->padded_data.fn_num; ++i) {
			reg_idx = regfunc->fn_tbl[i].fn_idx;
			reg_value = regfunc->fn_tbl[i].fn_para;
			LOG("  NTS_HWREG[0x%x]:%u", reg_idx, reg_value);
			if (mtk_tv_drm_autogen_set_nts_hw_reg(autogen, reg_idx, reg_value)) {
				ERR("set NTS_HW reg(0x%x, 0x%x) error", reg_idx, reg_value);
				ret = false;
			}
		}
	} else if (tag == EXTMEM_TAG_VREGISTER_SETTINGS) {
		LOG("VRREG num = %u", regfunc->padded_data.fn_num);
		for (i = 0; i < regfunc->padded_data.fn_num; ++i) {
			reg_idx = regfunc->fn_tbl[i].fn_idx;
			reg_value = regfunc->fn_tbl[i].fn_para;
			LOG("  SWREG[0x%x]:%u", reg_idx, reg_value);
			if (mtk_tv_drm_autogen_set_sw_reg(autogen, reg_idx, reg_value)) {
				ERR("set SW reg(0x%x, 0x%x) error", reg_idx, reg_value);
				ret = false;
			}
		}
	} else {
		ret = false;
	}
	return ret;
}

// TODO: Add ml module and move to it.
static int _mtk_ml_get_mem_info(
	struct mtk_video_plane_ml *disp_ml,
	void **start_addr,
	void **max_addr)
{
	struct sm_ml_info ml_info;
	uint8_t mem_idx = 0;

	if (disp_ml == NULL || start_addr == NULL || max_addr == NULL) {
		ERR("invalid parameters (%lx, %lx, %lx)", disp_ml, start_addr, max_addr);
		return -EINVAL;
	}
	LOG("ml_fd     = %u", disp_ml->fd);
	LOG("reg_count = %u", disp_ml->regcount);
	LOG("mem_index = %u", disp_ml->memindex);

	if (sm_ml_get_info(disp_ml->fd, disp_ml->memindex, &ml_info) != E_SM_RET_OK) {
		ERR("sm_ml_get_info fail");
		return -EPERM;
	}
	LOG("start_va  = 0x%zx", ml_info.start_va);
	LOG("mem_size  = 0x%x",  ml_info.mem_size);

	*start_addr = (void *)((uint8_t *)ml_info.start_va + (disp_ml->regcount * MTK_ML_REG_SIZE));
	*max_addr = (void *)((uint8_t *)ml_info.start_va + ml_info.mem_size);
	return 0;
}

// TODO: Add ml module and move to it.
static int _mtk_ml_add_cmd_size(struct mtk_video_plane_ml *disp_ml, uint32_t cmd_size)
{
	if (disp_ml == NULL) {
		ERR("invalid parameters (%lx)", disp_ml);
		return -EINVAL;
	}
	disp_ml->regcount += cmd_size;
	return 0;
}

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_tv_drm_pqmap_init(
	struct mtk_pqmap_context *pqmap,
	struct drm_mtk_tv_pqmap_info *info)
{
	struct mtk_tv_kms_context *kms = pqmap_to_kms(pqmap);
	struct mtk_tv_drm_metabuf metabuf;
	int size = 0;
	int ret = 0;

	if (!pqmap || !info) {
		ERR("Invalid input");
		return -EINVAL;
	}
	LOG("pqmap_ion_fd = %u", info->pqmap_ion_fd);
	LOG("pim_size = %u", info->pim_size);
	LOG("rm_size = %u", info->rm_size);

	if (pqmap->init == true) {
		ERR("pqmap has already inited");
		return 0;
	}

	// convert ION to
	if (mtk_tv_drm_metabuf_alloc_by_ion(&metabuf, info->pqmap_ion_fd)) {
		ERR("mtk_tv_drm_metabuf_alloc_by_ion(%d) fail", info->pqmap_ion_fd);
		ret = -EPERM;
		goto EXIT;
	}
	if ((info->pim_size + info->rm_size) > metabuf.size) {
		ERR("ION size error (%u + %u > %u)", info->pim_size, info->rm_size, metabuf.size);
		ret = -EPERM;
		goto EXIT;
	}

	pqmap->pqmap_va = devm_kzalloc(kms->dev, metabuf.size, GFP_KERNEL);
	if (pqmap->pqmap_va == NULL) {
		ERR("render pqmap devm_kzalloc(%u) failed", metabuf.size);
		ret = -EPERM;
		goto EXIT;
	}
	memcpy(pqmap->pqmap_va, metabuf.addr, metabuf.size);
	pqmap->pim_len = info->pim_size;
	pqmap->rm_len = info->rm_size;
	pqmap->pim_va = (char *)pqmap->pqmap_va;
	pqmap->rm_va = (char *)(pqmap->pqmap_va + info->pim_size);

	// init pqmap
	if (!pqmap_pim_init(&pqmap->pim_handle, pqmap->pim_va, pqmap->pim_len)) {
		ERR("render PIM init failed");
		ret = -EPERM;
		goto EXIT;
	}
	if (!pqmap_rm_init(&pqmap->rm_handle, pqmap->rm_va, pqmap->rm_len)) {
		ERR("render RM init failed");
		ret = -EPERM;
		goto EXIT;
	}

	// alloc node buffer
	size = sizeof(uint16_t) * MTK_PQMAP_NODE_LEN;
	pqmap->node_array = devm_kzalloc(kms->dev, size, GFP_KERNEL);
	if (pqmap->node_array == NULL) {
		ERR("render pqmap devm_kzalloc(%u) failed", size);
		ret = -EPERM;
		goto EXIT;
	}
	pqmap->node_num = 0;

EXIT:
	mtk_tv_drm_metabuf_free(&metabuf);
	if (ret == 0) {
		pqmap->init = true;
		LOG("init render pqmap succsessed !!!");
	} else {
		mtk_tv_drm_pqmap_deinit(pqmap);
	}
	return ret;
}

int mtk_tv_drm_pqmap_deinit(
	struct mtk_pqmap_context *pqmap)
{
	struct mtk_tv_kms_context *kms = pqmap_to_kms(pqmap);
	int ret = 0;

	if (!pqmap) {
		ERR("Invalid input, pqmap = %lx", pqmap);
		return -EINVAL;
	}
	if (pqmap->init == false) {
		ERR("render pqmap has not been inited");
		return 0;
	}
	if (pqmap->pim_handle && !pqmap_pim_destroy(&pqmap->pim_handle)) {
		ERR("Destroy render pim failed");
		ret = -EPERM;
	}
	if (pqmap->rm_handle && !pqmap_rm_destroy(&pqmap->rm_handle)) {
		ERR("Destroy render rm failed");
		ret = -EPERM;
	}
	if (pqmap->pqmap_va)
		devm_kfree(kms->dev, pqmap->pqmap_va);
	if (pqmap->node_array)
		devm_kfree(kms->dev, pqmap->node_array);

	memset(pqmap, 0, sizeof(struct mtk_pqmap_context));
	LOG("deinit render pqmap succsessed !!!");
	return ret;
}

int mtk_tv_drm_pqmap_insert(
	struct mtk_pqmap_context *pqmap,
	struct drm_mtk_tv_pqmap_node_array *node_array)
{
	if (!pqmap || !node_array) {
		ERR("Invalid input");
		return -EINVAL;
	}
	if (pqmap->init == false) {
		ERR("render pqmap has not been inited");
		return -EPERM;
	}
	if (node_array->node_num == 0) {
		LOG("no node");
		return 0;
	}
	if (pqmap->node_num + node_array->node_num >= MTK_PQMAP_NODE_LEN) {
		ERR("too many node, cur(%u) + new(%u) >= max(%u)",
			pqmap->node_num, node_array->node_num, MTK_PQMAP_NODE_LEN);
		return -EAGAIN;
	}
	memcpy(pqmap->node_array + pqmap->node_num,
		node_array->node_array,
		node_array->node_num * sizeof(uint16_t));

	pqmap->node_num += node_array->node_num;
	LOG("now = %u, new = %u", pqmap->node_num, node_array->node_num);
	return 0;
}

int mtk_tv_drm_pqmap_trigger(
	struct mtk_pqmap_context *pqmap)
{
	struct mtk_tv_kms_context *kms = pctx_pqmap_to_kms(pqmap);
	struct mtk_autogen_context *autogen = &kms->autogen_priv;
	struct mtk_video_plane_ml *disp_ml = &kms->video_priv.disp_ml;
	void *ml_mem_start_addr = NULL;
	void *ml_mem_max_addr = NULL;
	uint32_t autogen_cmd_size = 0;
	int ret = 0;
	int i;

	if (!pqmap) {
		ERR("Invalid input");
		return -EINVAL;
	}
	if (pqmap->node_num == 0) {
		LOG("No pqmap node");
		return 0;
	}
	LOG("node_num = %u", pqmap->node_num);

	// 1. Get ML memory info
	ret = _mtk_ml_get_mem_info(disp_ml, &ml_mem_start_addr, &ml_mem_max_addr);
	if (ret != 0) {
		ERR("ML get memory info error");
		goto EXIT;
	}
	LOG("get ml info: start_addr = %lx, max_addr = %lx", ml_mem_start_addr, ml_mem_max_addr);

	// 2. Setup AutoGen memory info
	ret = mtk_tv_drm_autogen_set_mem_info(autogen, ml_mem_start_addr, ml_mem_max_addr);
	if (ret != 0) {
		ERR("Set ML memory info error");
		goto EXIT;
	}
	LOG("set ml info: done");

	// 3. Write node data to ML
	for (i = 0; i < pqmap->node_num; ++i) {
		LOG("node[%d] = %u", i, pqmap->node_array[i]);
		pqmap_pim_load_setting(pqmap->pim_handle, pqmap->node_array[i],
			0, _mtk_render_pqmap_pim_callback, autogen);
	}
	// 4. Gen AutoGen's command size
	ret = mtk_tv_drm_autogen_get_cmd_size(autogen, &autogen_cmd_size);
	if (ret != 0) {
		ERR("Get Autogen command size error");
		goto EXIT;
	}
	LOG("get autogen info: cmd_size = %u", autogen_cmd_size);

	// 5. add ML command size
	ret = _mtk_ml_add_cmd_size(disp_ml, autogen_cmd_size);
	if (ret != 0) {
		ERR("Add ML command size error");
		goto EXIT;
	}
	LOG("add ml cmd size: done");
EXIT:
	pqmap->node_num = 0;
	return ret;
}

int mtk_tv_drm_pqmap_set_info_ioctl(
	struct drm_device *drm,
	void *data,
	struct drm_file *file_priv)
{
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;
	struct mtk_drm_plane *mplane = NULL;
	struct mtk_tv_kms_context *kms = NULL;
	struct mtk_pqmap_context *pqmap = NULL;
	struct drm_mtk_tv_pqmap_info *info = (struct drm_mtk_tv_pqmap_info *)data;
	int ret = 0;

	// get mtk_tv_kms_context pointer
	drm_for_each_crtc(crtc, drm) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		mplane = mtk_tv_crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO];
		kms = (struct mtk_tv_kms_context *)mplane->crtc_private;
		break;
	}
	pqmap = &kms->pqmap_priv;

	LOG("pqmap->init     = %d",  pqmap->init);
	LOG("pqmap->pim_va   = %lx", pqmap->pim_va);
	LOG("pqmap->rm_va    = %lx", pqmap->rm_va);
	LOG("pqmap->pqmap_va = %lx", pqmap->pqmap_va);
	if (pqmap->init == true) {
		ret = mtk_tv_drm_pqmap_deinit(pqmap);
		if (ret != 0) {
			ERR("mtk_tv_drm_pqmap_deinit fail, ret = %d", ret);
			goto EXIT;
		}
	}
	if (info != NULL && info->pim_size > 0 && info->rm_size > 0) {
		ret = mtk_tv_drm_pqmap_init(pqmap, info);
		if (ret != 0) {
			ERR("mtk_tv_drm_pqmap_init fail, ret = %d", ret);
			goto EXIT;
		}
	}
EXIT:
	return ret;
}
