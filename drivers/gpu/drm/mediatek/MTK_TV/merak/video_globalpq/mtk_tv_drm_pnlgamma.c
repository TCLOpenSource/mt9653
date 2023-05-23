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
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>

#include "../mtk_tv_drm_plane.h"
#include "../mtk_tv_drm_kms.h"
#include "mtk_tv_drm_common.h"
#include "../mtk_tv_drm_log.h"

#include "mtk_tv_drm_pnlgamma.h"

#include "drv_scriptmgt.h"
#include "pqu_msg.h"
#include "m_pqu_render.h"

#include "ext_command_render_if.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_PNLGAMMA

int mtk_video_panel_gamma_setting(
	struct drm_crtc *crtc,
	uint16_t *r,
	uint16_t *g,
	uint16_t *b,
	uint32_t size)
{
	int ret = 0;
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);
	struct mtk_tv_kms_context *pctx =
	(struct mtk_tv_kms_context *)mtk_tv_crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;

	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}

	pqu_gamma_data = metabuf.addr;

	memcpy(pqu_gamma_data->pnlgamma.pnlgamma_curve_r, r, sizeof(uint16_t)*size);
	memcpy(pqu_gamma_data->pnlgamma.pnlgamma_curve_g, g, sizeof(uint16_t)*size);
	memcpy(pqu_gamma_data->pnlgamma.pnlgamma_curve_b, b, sizeof(uint16_t)*size);

	pqu_gamma_data->pnlgamma.pnlgamma_curve_size = size;
	pqu_gamma_data->pnlgamma.pnlgamma_version = pctx->pnlgamma_version;

	updateInfo.pnlgamma_curve = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;
	pqupdate.pnlgamma_curve = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;

}

int mtk_video_panel_gamma_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pnlgamma_enable *pnlgamma_enable)
{
	struct mtk_tv_kms_context *pctx = NULL;
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;
	int ret = 0;

	if (!mtk_tv_crtc || !pnlgamma_enable) {
		DRM_ERROR("[%s][%d] null ptr, crtc=%p, pnlgamma_enable=%p\n",
			__func__, __LINE__, mtk_tv_crtc, pnlgamma_enable);
		return -EINVAL;
	}


	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;

	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}

	pqu_gamma_data = metabuf.addr;

	pqu_gamma_data->pnlgamma.pnlgamma_enable = pnlgamma_enable->enable;

	updateInfo.pnlgamma_enable = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;
	pqupdate.pnlgamma_enable = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;

}

int mtk_video_panel_gamma_gainoffset(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pnlgamma_gainoffset *gainoffset)
{
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;
	int ret = 0;

	if (!mtk_tv_crtc || !gainoffset) {
		DRM_ERROR("[%s][%d] null ptr, crtc=%p, gainoffset=%p\n",
			__func__, __LINE__, mtk_tv_crtc, gainoffset);
		return -EINVAL;
	}

	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;
	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}
	pqu_gamma_data = metabuf.addr;

	pqu_gamma_data->pnlgamma.pnlgamma_offset_r = gainoffset->pnlgamma_offset_r;
	pqu_gamma_data->pnlgamma.pnlgamma_offset_g = gainoffset->pnlgamma_offset_g;
	pqu_gamma_data->pnlgamma.pnlgamma_offset_b = gainoffset->pnlgamma_offset_b;
	pqu_gamma_data->pnlgamma.pnlgamma_gain_r = gainoffset->pnlgamma_gain_r;
	pqu_gamma_data->pnlgamma.pnlgamma_gain_g = gainoffset->pnlgamma_gain_g;
	pqu_gamma_data->pnlgamma.pnlgamma_gain_b = gainoffset->pnlgamma_gain_b;

	updateInfo.pnlgamma_gainoffset = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	pqupdate.pnlgamma_gainoffset = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;

}

int mtk_video_panel_gamma_gainoffset_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pnlgamma_gainoffset_enable *pnl_gainoffsetenable)
{
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;
	int ret = 0;

	if (!mtk_tv_crtc || !pnl_gainoffsetenable) {
		DRM_ERROR("[%s][%d] null ptr, crtc=%p,  pnl_gainoffsetenable=%p\n",
			__func__, __LINE__, mtk_tv_crtc, pnl_gainoffsetenable);
		return -EINVAL;
	}

	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;

	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}
	pqu_gamma_data = metabuf.addr;

	pqu_gamma_data->pnlgamma.pnlgamma_gainoffset_enbale = pnl_gainoffsetenable->enable;

	updateInfo.pnlgamma_gainoffset_enable = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	pqupdate.pnlgamma_gainoffset_enable = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;
}

