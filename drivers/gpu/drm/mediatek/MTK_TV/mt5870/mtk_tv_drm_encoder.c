// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>

#include "mtk_tv_drm_encoder.h"

static void mtk_tv_drm_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_helper_funcs mtk_tv_encoder_helper_funcs = {

};

static const struct drm_encoder_funcs mtk_tv_encoder_funcs = {
	.destroy = mtk_tv_drm_encoder_destroy,
};

int mtk_tv_drm_encoder_create(struct drm_device *dev,
	struct drm_encoder *encoder, struct drm_crtc *crtc)
{
	int ret;

	encoder->possible_crtcs = (1 << drm_crtc_index(crtc));
	ret = drm_encoder_init(dev, encoder, &mtk_tv_encoder_funcs, DRM_MODE_ENCODER_LVDS, NULL);

	if (ret < 0) {
		goto fail;
		return ret;
	}

	return 0;
fail:
	drm_encoder_cleanup(encoder);
	return -1;
}
