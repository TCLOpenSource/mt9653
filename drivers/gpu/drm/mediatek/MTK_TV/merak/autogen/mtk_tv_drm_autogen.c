// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_log.h"
#include "mtk_tv_drm_autogen.h"
#include "mapi_pq_if.h"

//--------------------------------------------------------------------------------------------------
//  Local Defines
//--------------------------------------------------------------------------------------------------
#define autogen_to_kms(x) container_of(x, struct mtk_tv_kms_context, autogen_priv)
#define MTK_DRM_MODEL MTK_DRM_MODEL_AUTOGEN
#define ERR(fmt, args...) DRM_ERROR("[%s][%d] " fmt, __func__, __LINE__, ##args)
#define LOG(fmt, args...) DRM_INFO("[%s][%d] " fmt, __func__, __LINE__, ##args)

#define RENDER_DD_WITH_ML_BUG 1 // After render dd fixed the bug, remove it and relational #if

//--------------------------------------------------------------------------------------------------
//  Local Enum and Structures
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Local Variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Local Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Global Functions
//--------------------------------------------------------------------------------------------------
int mtk_tv_drm_autogen_init(struct mtk_autogen_context *autogen)
{
	struct mtk_tv_kms_context *kms = autogen_to_kms(autogen);
	struct device *dev = kms->dev;
	struct ST_PQ_CTX_SIZE_INFO size_info;
	struct ST_PQ_CTX_SIZE_STATUS size_status;
	struct ST_PQ_FRAME_CTX_INFO frame_info;
	struct ST_PQ_FRAME_CTX_STATUS frame_status;
	struct ST_PQ_CTX_INFO ctx_info;
	struct ST_PQ_CTX_STATUS ctx_status;
	struct ST_PQ_NTS_CAPABILITY_INFO cap_info;
	struct ST_PQ_NTS_CAPABILITY_STATUS cap_status;
	struct ST_PQ_NTS_GETHWREG_INFO get_hwreg_info;
	struct ST_PQ_NTS_GETHWREG_STATUS get_hwreg_status;
	struct ST_PQ_NTS_INITHWREG_INFO init_hwreg_info;
	struct ST_PQ_NTS_INITHWREG_STATUS init_hwreg_status;
	enum EN_PQAPI_RESULT_CODES ret;

	if (autogen->init == true) {
		ERR("Autogen had be inited");
		return 0;
	}

	// 0. reset all struct
	memset(autogen, 0, sizeof(struct mtk_autogen_context));
	memset(&size_info, 0, sizeof(struct ST_PQ_CTX_SIZE_INFO));
	memset(&size_status, 0, sizeof(struct ST_PQ_CTX_SIZE_STATUS));
	memset(&frame_info, 0, sizeof(struct ST_PQ_FRAME_CTX_INFO));
	memset(&frame_status, 0, sizeof(struct ST_PQ_FRAME_CTX_STATUS));
	memset(&ctx_info, 0, sizeof(struct ST_PQ_CTX_INFO));
	memset(&ctx_status, 0, sizeof(struct ST_PQ_CTX_STATUS));
	memset(&cap_info, 0, sizeof(struct ST_PQ_NTS_CAPABILITY_INFO));
	memset(&cap_status, 0, sizeof(struct ST_PQ_NTS_CAPABILITY_STATUS));
	memset(&get_hwreg_info, 0, sizeof(struct ST_PQ_NTS_GETHWREG_INFO));
	memset(&get_hwreg_status, 0, sizeof(struct ST_PQ_NTS_GETHWREG_STATUS));
	memset(&init_hwreg_info, 0, sizeof(struct ST_PQ_NTS_INITHWREG_INFO));
	memset(&init_hwreg_status, 0, sizeof(struct ST_PQ_NTS_INITHWREG_STATUS));

	// 1. get size
	MApi_PQ_GetCtxSize(autogen->ctx, &size_info, &size_status);
	if ((size_status.u32PqCtxSize == 0) || (size_status.u32SwRegSize == 0) ||
		(size_status.u32HwReportSize == 0) || (size_status.u32MetadataSize == 0)) {
		ERR("invalid size (ctx %u, reg %u, hw repot %u, metadata %u)",
			size_status.u32PqCtxSize, size_status.u32SwRegSize,
			size_status.u32HwReportSize, size_status.u32MetadataSize);
		goto ERROR;
	}

	// 2. setup buffer
	autogen->ctx       = devm_kzalloc(dev, size_status.u32PqCtxSize, GFP_KERNEL);
	autogen->sw_reg    = devm_kzalloc(dev, size_status.u32SwRegSize, GFP_KERNEL);
	autogen->metadata  = devm_kzalloc(dev, size_status.u32MetadataSize, GFP_KERNEL);
	autogen->hw_report = devm_kzalloc(dev, size_status.u32HwReportSize, GFP_KERNEL);
	if (autogen->ctx == NULL || autogen->sw_reg == NULL ||
		autogen->metadata == NULL || autogen->hw_report == NULL) {
		ERR("allocate buffer fail (ctx %lx, sw_reg %lx, metadata %lx, hw_report %lx)",
			autogen->ctx, autogen->sw_reg, autogen->metadata, autogen->hw_report);
		goto ERROR;
	}
	frame_info.pSwReg    = autogen->sw_reg;
	frame_info.pMetadata = autogen->metadata;
	frame_info.pHwReport = autogen->hw_report;
	ret = MApi_PQ_SetFrameCtx(autogen->ctx, &frame_info, &frame_status);
	if (ret & E_PQAPI_RC_FAIL) {
		ERR("MApi_PQ_SetFrameCtx fail (%u)", ret);
		goto ERROR;
	}

	// 3. init autoGen
	ctx_info.eWinID = E_PQ_MAIN_WINDOW;
	ret = MApi_PQ_InitCtx(autogen->ctx, &ctx_info, &ctx_status);
	if (ret & E_PQAPI_RC_FAIL) {
		ERR("MApi_PQ_InitCtx fail (%u)", ret);
		goto ERROR;
	}

	// 4. set capability
	cap_info.eDomain = E_PQ_DOMAIN_RENDER;
	ret = MApi_PQ_SetCapability_NonTs(autogen->ctx, &cap_info, &cap_status);
	if (ret != E_PQAPI_RC_SUCCESS) {
		ERR("MApi_PQ_SetCapability_NonTs fail (%u)", ret);
		goto ERROR;
	}

	// 5. get HW reg info
	get_hwreg_info.eDomain = E_PQ_DOMAIN_RENDER;
	ret = MApi_PQ_GetHWRegInfo_NonTs(autogen->ctx, &get_hwreg_info, &get_hwreg_status);
	if (ret != E_PQAPI_RC_SUCCESS) {
		ERR("MApi_PQ_GetHWRegInfo_NonTs fail (%u)", ret);
		goto ERROR;
	}

	// 6. set HW reg table
	autogen->func_table = devm_kzalloc(dev, get_hwreg_status.u32FuncTableSize, GFP_KERNEL);
	if (autogen->func_table == NULL) {
		ERR("allocate func_table buffer fail");
		goto ERROR;
	}
	init_hwreg_info.eDomain = E_PQ_DOMAIN_RENDER;
	init_hwreg_info.u32FuncTableSize = get_hwreg_status.u32FuncTableSize;
	init_hwreg_info.pFuncTableAddr = autogen->func_table;
	ret = MApi_PQ_InitHWRegTable_NonTs(autogen->ctx, &init_hwreg_info, &init_hwreg_status);
	if (ret != E_PQAPI_RC_SUCCESS) {
		ERR("MApi_PQ_InitHWRegTable_NonTs fail (%u)", ret);
		goto ERROR;
	}

	autogen->init = true;
	return 0;

ERROR:
	kfree(autogen->ctx);
	kfree(autogen->sw_reg);
	kfree(autogen->metadata);
	kfree(autogen->hw_report);
	kfree(autogen->func_table);
	memset(autogen, 0, sizeof(struct mtk_autogen_context));
	return -EINVAL;
}

int mtk_tv_drm_autogen_deinit(struct mtk_autogen_context *autogen)
{
	struct ST_PQ_NTS_DEINITHWREG_INFO deinit_nts_hwreg_info;
	struct ST_PQ_NTS_DEINITHWREG_STATUS deinit_nts_hwreg_status;
	struct ST_PQ_DEINITHWREG_INFO deinit_hwreg_info;
	struct ST_PQ_DEINITHWREG_STATUS deinit_hwreg_status;

	if (autogen->init == false)
		return 0;

	// 0. reset all struct
	memset(&deinit_nts_hwreg_info, 0, sizeof(struct ST_PQ_NTS_DEINITHWREG_INFO));
	memset(&deinit_nts_hwreg_status, 0, sizeof(struct ST_PQ_NTS_DEINITHWREG_STATUS));
	memset(&deinit_hwreg_info, 0, sizeof(struct ST_PQ_DEINITHWREG_INFO));
	memset(&deinit_hwreg_status, 0, sizeof(struct ST_PQ_DEINITHWREG_STATUS));

	deinit_nts_hwreg_info.eDomain = E_PQ_DOMAIN_RENDER;
	MApi_PQ_DeInitHWRegTable_NonTs(autogen->ctx,
		&deinit_nts_hwreg_info, &deinit_nts_hwreg_status);
	MApi_PQ_DeInitHWRegTable(autogen->ctx, &deinit_hwreg_info, &deinit_hwreg_status);

	kfree(autogen->ctx);
	kfree(autogen->sw_reg);
	kfree(autogen->metadata);
	kfree(autogen->hw_report);
	kfree(autogen->func_table);
	memset(autogen, 0, sizeof(struct mtk_autogen_context));
	return 0;
}

int mtk_tv_drm_autogen_set_mem_info(
	struct mtk_autogen_context *autogen,
	void *start_addr,
	void *max_addr)
{
	struct ST_PQ_ML_CTX_INFO ml_info;
	struct ST_PQ_ML_CTX_STATUS ml_status;
	int ret = 0;

	if (autogen == NULL || start_addr == NULL || max_addr == NULL) {
		ERR("invalid parameters (autogen %lx, start addr %lx, max addr %lx)",
			autogen, start_addr, max_addr);
		return -EINVAL;
	}

	memset(&ml_status, 0, sizeof(struct ST_PQ_ML_CTX_STATUS));
	ml_status.u32Version = 0;
	ml_status.u32Length = sizeof(struct ST_PQ_ML_CTX_STATUS);

	memset(&ml_info, 0, sizeof(struct ST_PQ_ML_CTX_INFO));
	ml_info.u32Version = 0;
	ml_info.u32Length = sizeof(struct ST_PQ_ML_CTX_INFO);
	ml_info.u16PqModuleIdx = E_PQ_PIM_MODULE;
	ml_info.pDispAddr = start_addr;
	ml_info.pDispMaxAddr = max_addr;

	ret = MApi_PQ_SetMLInfo(autogen->ctx, &ml_info, &ml_status);
	if (ret != 0) {
		ERR("MApi_PQ_SetMLInfo fail (%d)", ret);
		return -EINVAL;
	}
	return 0;
}

int mtk_tv_drm_autogen_get_cmd_size(
	struct mtk_autogen_context *autogen,
	uint32_t *cmd_size)
{
	struct ST_PQ_ML_PARAM ml_param;
	struct ST_PQ_ML_INFO ml_status;
	int ret = 0;

	if (autogen == NULL || cmd_size == NULL) {
		ERR("invalid parameters (atuogen %lx, size ptr %lx)", autogen, cmd_size);
		return -EINVAL;
	}

	memset(&ml_status, 0, sizeof(struct ST_PQ_ML_INFO));
	ml_status.u32Version = 0;
	ml_status.u32Length = sizeof(struct ST_PQ_ML_INFO);

	memset(&ml_param, 0, sizeof(struct ST_PQ_ML_PARAM));
	ml_param.u32Version = 0;
	ml_param.u32Length = sizeof(struct ST_PQ_ML_PARAM);
	ml_param.u16PqModuleIdx = E_PQ_PIM_MODULE;

	ret = MApi_PQ_GetMLInfo(autogen->ctx, &ml_param, &ml_status);
	if (ret != 0) {
		ERR("MApi_PQ_GetMLInfo fail (%d)", ret);
		return -EINVAL;
	}
	*cmd_size = ml_status.u16DispUsedCmdSize;
	return 0;
}

int mtk_tv_drm_autogen_set_nts_hw_reg(
	struct mtk_autogen_context *autogen,
	uint32_t reg_idx,
	uint32_t reg_value)
{
	struct ST_PQ_NTS_HWREG_INFO reg_info;
	struct ST_PQ_NTS_HWREG_STATUS reg_status;
	int ret = 0;

	if (autogen == NULL) {
		ERR("invalid parameters (autogen %lx)", autogen);
		return -EINVAL;
	}

	memset(&reg_status, 0, sizeof(struct ST_PQ_NTS_HWREG_STATUS));
	memset(&reg_info, 0, sizeof(struct ST_PQ_NTS_HWREG_INFO));
	reg_info.eDomain = E_PQ_DOMAIN_RENDER;
	reg_info.u32RegIdx = reg_idx;
	reg_info.u16Value = reg_value;
#if RENDER_DD_WITH_ML_BUG
	ret = MApi_PQ_SetHWReg_NonTs_Imm(autogen->ctx, &reg_info, &reg_status);
	if (ret != 0) {
		ERR("MApi_PQ_SetHWReg_NonTs fail (%d)", ret);
		return -EINVAL;
	}
#else
	ret = MApi_PQ_SetHWReg_NonTs(autogen->ctx, &reg_info, &reg_status);
	if (ret != 0) {
		ERR("MApi_PQ_SetHWReg_NonTs fail (%d)", ret);
		return -EINVAL;
	}
#endif
	return 0;
}

int mtk_tv_drm_autogen_set_sw_reg(
	struct mtk_autogen_context *autogen,
	uint32_t reg_idx,
	uint32_t reg_value)
{
#if (0)
	// TODO
	// Autogen swreg API and code flow are changed
	// Need to refactor it
	struct ST_PQ_SWREG_INFO reg_info;
	struct ST_PQ_SWREG_STATUS reg_status;
	int ret = 0;

	if (autogen == NULL) {
		ERR("invalid parameters (autogen %lx)", autogen);
		return -EINVAL;
	}

	memset(&reg_status, 0, sizeof(struct ST_PQ_SWREG_STATUS));
	memset(&reg_info, 0, sizeof(struct ST_PQ_SWREG_INFO));
	reg_info.u32RegIdx = reg_idx;
	reg_info.u16Value = reg_value;
	ret = MApi_PQ_SetSWReg(autogen->ctx, &reg_info, &reg_status);
	if (ret != 0) {
		ERR("MApi_PQ_SetSWReg fail (%d)", ret);
		return -EINVAL;
	}
	return 0;
#else
	LOG("Set SWReg: reg_idx = %d, value = %x)", reg_idx, reg_value);
	return 0;
#endif
}
