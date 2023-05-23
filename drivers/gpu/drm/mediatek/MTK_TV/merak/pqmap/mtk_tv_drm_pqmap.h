/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_PQMAP_H_
#define _MTK_TV_DRM_PQMAP_H_
#include <linux/dma-buf.h>
#include <drm/mtk_tv_drm.h>

//-------------------------------------------------------------------------------------------------
//  Defines
//-------------------------------------------------------------------------------------------------
#define MTK_PQMAP_NODE_LEN 2048 // NODE type is u16

//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
struct mtk_pqmap_context {
	bool      init;
	void     *pqmap_va;

	void     *pim_handle;
	void     *pim_va;
	uint32_t  pim_len;

	void     *rm_handle;
	void     *rm_va;
	uint32_t  rm_len;

	uint16_t *node_array;
	uint32_t  node_num;
};

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_tv_drm_pqmap_init(
	struct mtk_pqmap_context *pqmap_ctx,
	struct drm_mtk_tv_pqmap_info *info);

int mtk_tv_drm_pqmap_deinit(
	struct mtk_pqmap_context *pqmap_ctx);

int mtk_tv_drm_pqmap_insert(
	struct mtk_pqmap_context *pqmap_ctx,
	struct drm_mtk_tv_pqmap_node_array *node_array);

int mtk_tv_drm_pqmap_trigger(
	struct mtk_pqmap_context *pqmap_ctx);

int mtk_tv_drm_pqmap_set_info_ioctl(
	struct drm_device *drm,
	void *data,
	struct drm_file *file_priv);

#endif //_MTK_TV_DRM_PQMAP_H_
