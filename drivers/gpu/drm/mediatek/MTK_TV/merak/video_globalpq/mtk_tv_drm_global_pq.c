// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/of_irq.h>
#include <linux/dma-buf.h>
#include <drm/mtk_tv_drm.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_atomic.h>

#include "mtk_tv_drm_global_pq.h"
#include "../mtk_tv_drm_kms.h"
#include "../mtk_tv_drm_log.h"

#include "hwreg_render_video_globalpq.h"

static void _mtk_global_pq_set_global_pq(struct mtk_global_pq_context *pctx_globalpq)
{
	struct hwregSetGlobalPQIn paramSetGlobalPQIn;

	if (!pctx_globalpq) {
		DRM_ERROR("[%s][%d]Invalid input, pctx_globalpq = %p!!\n",
			__func__, __LINE__, pctx_globalpq);
		return;
	}

	memset(&paramSetGlobalPQIn, 0, sizeof(paramSetGlobalPQIn));

	// get global pq string
	paramSetGlobalPQIn.globalPQ = pctx_globalpq->pq_string;

	// todo: handle global pq



	// for stub mode test
	drv_hwreg_render_display_set_global_pq(paramSetGlobalPQIn);

}

void mtk_global_pq_atomic_crtc_flush(struct mtk_global_pq_context *pctx_globalpq)
{
	if (!pctx_globalpq) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_globalpq=0x%llx\n",
			__func__, __LINE__, pctx_globalpq);
		return;
	}

	// handle window pq
	_mtk_global_pq_set_global_pq(pctx_globalpq);
}

int mtk_global_pq_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val, int prop_index)
{
	int ret = 0;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->crtc_private;
	struct drm_property_blob *property_blob = NULL;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;

	switch (prop_index) {
	case E_EXT_CRTC_PROP_GLOBAL_PQ:
		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob != NULL) {
			char *tmp_pq_string = 0;

			// kmalloc by blob size
			tmp_pq_string = kmalloc(property_blob->length, GFP_KERNEL);
			if (IS_ERR_OR_NULL(tmp_pq_string)) {
				DRM_ERROR("[%s][%d]alloc global pq buf fail, size=%d, ret=%d\n",
					__func__, __LINE__, property_blob->length, tmp_pq_string);
				return -ENOMEM;
			}

			// copy to tmp buffer
			memset(tmp_pq_string, 0, property_blob->length);
			memcpy(tmp_pq_string, property_blob->data, property_blob->length);

			// remove previous blob from kms context
			kfree(pctx->global_pq_priv.pq_string);

			// save new blob to kms context
			pctx->global_pq_priv.pq_string = tmp_pq_string;
		} else {
			DRM_ERROR("[%s][%d]prop_idx=%d, blob id=0x%llx, blob is NULL!!\n",
				__func__, __LINE__, prop_index, val);
			return -EINVAL;
		}
		break;
	default:
		DRM_ERROR("[%s][%d] invalid global_pq property %d!!\n",
			__func__, __LINE__, prop_index);
		return -EINVAL;
	}

	return ret;
}

int mtk_global_pq_atomic_get_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val,
	int prop_index)
{
	int ret = 0;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	struct drm_property_blob *property_blob = NULL;
	int64_t int64Value = 0;

	switch (prop_index) {
	default:
		//DRM_INFO("[DRM][VIDEO][%s][%d]default\n", __func__, __LINE__);
		break;
	}

	return ret;
}

