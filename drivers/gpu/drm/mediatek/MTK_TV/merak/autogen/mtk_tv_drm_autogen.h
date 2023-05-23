/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_AUTOGEN_H_
#define _MTK_TV_DRM_AUTOGEN_H_
#include <drm/mtk_tv_drm.h>

//-------------------------------------------------------------------------------------------------
//  Defines
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
struct mtk_autogen_context {
	bool init;
	void *ctx;
	void *sw_reg;
	void *metadata;
	void *hw_report;
	void *func_table;
};

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_tv_drm_autogen_init(
	struct mtk_autogen_context *autogen_ctx);

int mtk_tv_drm_autogen_deinit(
	struct mtk_autogen_context *autogen_ctx);

int mtk_tv_drm_autogen_set_mem_info(
	struct mtk_autogen_context *autogen_ctx,
	void *start_addr,
	void *max_addr);

int mtk_tv_drm_autogen_get_cmd_size(
	struct mtk_autogen_context *autogen_ctx,
	uint32_t *cmd_size);

int mtk_tv_drm_autogen_set_nts_hw_reg(
	struct mtk_autogen_context *autogen_ctx,
	uint32_t reg_idx,
	uint32_t reg_value);

int mtk_tv_drm_autogen_set_sw_reg(
	struct mtk_autogen_context *autogen_ctx,
	uint32_t reg_idx,
	uint32_t reg_value);

#endif //_MTK_TV_DRM_AUTOGEN_H_
