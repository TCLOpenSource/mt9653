// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/mtk_tv_drm.h>
#include <drm/drm_panel.h>

#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_encoder.h"
#include "mtk_tv_drm_log.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_ENCODER

static void mtk_tv_drm_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

void mtk_tv_drm_encoder_atomic_mode_set(struct drm_encoder *encoder,
				struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	struct mtk_tv_drm_connector *mtkConnector = NULL;

	if (!crtc_state) {
		DRM_ERROR("[%s, %d]: crtc_state is NULL.\n",
			__func__, __LINE__);
		return;
	}
	drm_panel_prepare(conn_state->connector->panel);
}

static const struct drm_encoder_helper_funcs mtk_tv_encoder_helper_funcs = {
	.atomic_mode_set = mtk_tv_drm_encoder_atomic_mode_set,
};

static const struct drm_encoder_funcs mtk_tv_encoder_funcs = {
	.destroy = mtk_tv_drm_encoder_destroy,
};

int mtk_tv_drm_encoder_create(struct drm_device *dev,
	struct drm_encoder *encoder, struct mtk_tv_drm_crtc *mcrtc)
{
	int ret, i;

	for (i = 0; i < MTK_DRM_CRTC_TYPE_MAX; i++)
		encoder->possible_crtcs |= (1 << drm_crtc_index(&(mcrtc[i].base)));

	ret = drm_encoder_init(dev, encoder, &mtk_tv_encoder_funcs, DRM_MODE_ENCODER_LVDS, NULL);
	drm_encoder_helper_add(encoder, &mtk_tv_encoder_helper_funcs);
	if (ret < 0) {
		goto fail;
		return ret;
	}

	return 0;
fail:
	drm_encoder_cleanup(encoder);
	return -1;
}
