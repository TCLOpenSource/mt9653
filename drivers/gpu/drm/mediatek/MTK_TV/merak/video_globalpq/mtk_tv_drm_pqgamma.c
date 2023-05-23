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

#include "drv_scriptmgt.h"
#include "pqu_msg.h"
#include "m_pqu_render.h"

#include "mtk_tv_drm_pqgamma.h"
#include "ext_command_render_if.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_PNLGAMMA

int mtk_video_pqgamma_set(
	struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_curve *curve)
{
	int ret = 0;
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;

	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;

	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}

	pqu_gamma_data = metabuf.addr;

	memcpy(pqu_gamma_data->pqgamma.pqgamma_curve_r, curve->curve_r,
		sizeof(uint16_t)*curve->size);
	memcpy(pqu_gamma_data->pqgamma.pqgamma_curve_g, curve->curve_g,
		sizeof(uint16_t)*curve->size);
	memcpy(pqu_gamma_data->pqgamma.pqgamma_curve_b, curve->curve_b,
		sizeof(uint16_t)*curve->size);

	pqu_gamma_data->pqgamma.pqgamma_curve_size = curve->size;
	pqu_gamma_data->pqgamma.pqgamma_version = pctx->pqgamma_version;

	updateInfo.pqgamma_curve = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;
	pqupdate.pqgamma_curve = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;

}

int mtk_video_pqgamma_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_enable *pqgamma_enable)
{

	struct mtk_tv_kms_context *pctx = NULL;
	int ret = 0;
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;

	if (!mtk_tv_crtc || !pqgamma_enable) {
		DRM_ERROR("[%s][%d] null ptr, crtc=%p, pqgamma_enable=%p\n",
			__func__, __LINE__, mtk_tv_crtc, pqgamma_enable);
		return -EINVAL;
	}

	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;

	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}
	pqu_gamma_data = metabuf.addr;

	pqu_gamma_data->pqgamma.pqgamma_enable = pqgamma_enable->enable;

	updateInfo.pqgamma_enable = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;
	pqupdate.pqgamma_enable = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;

}

int mtk_video_pqgamma_gainoffset(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_gainoffset *gainoffset)
{

	int ret = 0;
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;

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

	pqu_gamma_data->pqgamma.pqgamma_offset_r = gainoffset->pqgamma_offset_r;
	pqu_gamma_data->pqgamma.pqgamma_offset_g = gainoffset->pqgamma_offset_g;
	pqu_gamma_data->pqgamma.pqgamma_offset_b = gainoffset->pqgamma_offset_b;
	pqu_gamma_data->pqgamma.pqgamma_gain_r = gainoffset->pqgamma_gain_r;
	pqu_gamma_data->pqgamma.pqgamma_gain_g = gainoffset->pqgamma_gain_g;
	pqu_gamma_data->pqgamma.pqgamma_gain_b = gainoffset->pqgamma_gain_b;
	pqu_gamma_data->pqgamma.pqgamma_pregainoffset  = gainoffset->pregainoffset;

	updateInfo.pqgamma_gainoffset = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;
	pqupdate.pqgamma_gainoffset = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;

}

int mtk_video_pqgamma_gainoffset_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_gainoffset_enable *gainoffsetenable)
{
	int ret = 0;
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;

	if (!mtk_tv_crtc || !gainoffsetenable) {
		DRM_ERROR("[%s][%d] null ptr, crtc=%p, gainoffsetenable=%p\n",
			__func__, __LINE__, mtk_tv_crtc, gainoffsetenable);
		return -EINVAL;
	}

	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;

	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}

	pqu_gamma_data = metabuf.addr;

	pqu_gamma_data->pqgamma.pqgamma_gainoffset_enbale = gainoffsetenable->enable;
	pqu_gamma_data->pqgamma.pqgamma_pregainoffset = gainoffsetenable->pregainoffset;

	updateInfo.pqgamma_gainoffset_enable = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;
	pqupdate.pqgamma_gainoffset_enable = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;
}

int mtk_video_pqgamma_set_maxvalue(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_maxvalue *maxvalue)
{
	int ret = 0;
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;

	if (!mtk_tv_crtc || !maxvalue) {
		DRM_ERROR("[%s][%d] null ptr, crtc=%p, gainoffset=%p\n",
			__func__, __LINE__, mtk_tv_crtc, maxvalue);
		return -EINVAL;
	}

	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;
	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}
	pqu_gamma_data = metabuf.addr;

	pqu_gamma_data->pqgamma.pqgamma_max_r = maxvalue->pqgamma_max_r;
	pqu_gamma_data->pqgamma.pqgamma_max_g = maxvalue->pqgamma_max_g;
	pqu_gamma_data->pqgamma.pqgamma_max_b = maxvalue->pqgamma_max_b;

	updateInfo.pqgamma_maxvalue = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;
	pqupdate.pqgamma_maxvalue = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;

}

int mtk_video_pqgamma_maxvalue_enable(struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_mtk_tv_pqgamma_maxvalue_enable *maxvalue_enable)
{

	int ret = 0;
	struct msg_render_pq_update_info updateInfo;
	struct pqu_dd_update_pq_info_param pqupdate;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_metabuf metabuf;
	struct meta_render_pqu_pq_info *pqu_gamma_data = NULL;

	if (!mtk_tv_crtc || !maxvalue_enable) {
		DRM_ERROR("[%s][%d] null ptr, crtc=%p, gainoffset=%p\n",
			__func__, __LINE__, mtk_tv_crtc, maxvalue_enable);
		return -EINVAL;
	}

	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;
	memset(&updateInfo, 0, sizeof(struct msg_render_pq_update_info));

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA)) {
		DRM_ERROR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA);
		return -EINVAL;
	}
	pqu_gamma_data = metabuf.addr;

	pqu_gamma_data->pqgamma.pqgamma_maxvalue_enable = maxvalue_enable->enable;

	updateInfo.pqgamma_maxvalue_enable = TRUE;
	updateInfo.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;
	pqupdate.pqgamma_maxvalue_enable = TRUE;
	pqupdate.pnlgamma_stub_mode = pctx->pnlgamma_stubmode;

	if (bPquEnable)
		pqu_render_dd_pq_update_info(&pqupdate, NULL);
	else
		pqu_msg_send(PQU_MSG_SEND_RENDER_PQ_FIRE, &updateInfo);

	ret = mtk_tv_drm_metabuf_free(&metabuf);
	return ret;

}
