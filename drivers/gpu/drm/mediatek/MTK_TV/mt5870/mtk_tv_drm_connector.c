// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>

#include "mtk_tv_drm_connector.h"

static inline struct mtk_tv_drm_connector *to_mtk_tv_connector(struct drm_connector *con)
{
	return con ? container_of(con, struct mtk_tv_drm_connector, base)
	    : NULL;
}

static void mtk_tv_drm_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static enum drm_connector_status
mtk_tv_drm_connector_detect(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static struct drm_encoder *mtk_tv_drm_connector_best_encoder(struct drm_connector *connector)
{
	struct mtk_tv_drm_connector *mtk_con = to_mtk_tv_connector(connector);

	return mtk_con->encoder;

}

static int mtk_tv_drm_connector_get_modes(struct drm_connector *connector)
{
	struct drm_display_mode *mode = drm_mode_create(connector->dev);

	if (!mode) {
		pr_err("failed to create a new display mode.\n");
		return 0;	// return 0 as zero connector
	}

	mode->clock = 67430;
	mode->vrefresh = 60;

	mode->hdisplay = 3840;
	mode->hsync_start = 4016;
	mode->hsync_end = 4104;
	mode->htotal = 4400;

	mode->vdisplay = 2160;
	mode->vsync_start = 2168;
	mode->vsync_end = 2178;
	mode->vtotal = 2250;

	mode->width_mm = 3840;
	mode->height_mm = 2160;

	connector->display_info.width_mm = 3840;	// mode->width_mm;
	connector->display_info.height_mm = 2160;	// mode->height_mm;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	return 1;
}

static int mtk_tv_drm_connector_mode_valid(struct drm_connector *connector,
					   struct drm_display_mode *mode)
{
	if (mode->hdisplay & 0x1)
		return MODE_ERROR;

	return MODE_OK;
}

static int mtk_tv_connector_atomic_set_property(struct drm_connector *connector,
						struct drm_connector_state *state,
						struct drm_property *property, uint64_t val)
{
	int ret = -EINVAL;
	struct mtk_tv_drm_connector *mtk_tv_connector = to_mtk_tv_connector(connector);

	if (mtk_tv_connector->ops->atomic_set_connector_property) {
		ret =
		    mtk_tv_connector->ops->atomic_set_connector_property(mtk_tv_connector, state,
									 property, val);
	}
	return ret;
}

static int mtk_tv_connector_atomic_get_property(struct drm_connector *connector,
						const struct drm_connector_state *state,
						struct drm_property *property, uint64_t *val)
{
	int ret = -EINVAL;
	struct mtk_tv_drm_connector *mtk_tv_connector = to_mtk_tv_connector(connector);

	if (mtk_tv_connector->ops->atomic_get_connector_property) {
		ret =
		    mtk_tv_connector->ops->atomic_get_connector_property(mtk_tv_connector, state,
									 property, val);
	}
	return ret;
}

static const struct drm_connector_funcs mtk_drm_connector_funcs = {
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.destroy = mtk_tv_drm_connector_destroy,
	.detect = mtk_tv_drm_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_set_property = mtk_tv_connector_atomic_set_property,
	.atomic_get_property = mtk_tv_connector_atomic_get_property,
};

static const struct drm_connector_helper_funcs connector_helper_funcs = {
	.best_encoder = mtk_tv_drm_connector_best_encoder,
	.get_modes = mtk_tv_drm_connector_get_modes,
	.mode_valid = mtk_tv_drm_connector_mode_valid,
};

int mtk_tv_drm_connector_create(struct drm_device *drm_dev, struct mtk_tv_drm_connector *connector,
				struct drm_encoder *encoder,
				const struct mtk_tv_drm_connector_ops *ops)
{
	int ret;

	connector->encoder = encoder;
	connector->ops = ops;

	ret =
	    drm_connector_init(drm_dev, &connector->base, &mtk_drm_connector_funcs,
			       DRM_MODE_CONNECTOR_LVDS);
	if (ret < 0)
		return ret;

	drm_connector_helper_add(&connector->base, &connector_helper_funcs);

	ret = drm_connector_register(&connector->base);
	if (ret < 0)
		goto err_cleanup;

	ret = drm_connector_attach_encoder(&connector->base, encoder);
	if (ret < 0)
		goto err_sysfs;

	return 0;

err_sysfs:
	drm_connector_unregister(&connector->base);
err_cleanup:
	drm_connector_cleanup(&connector->base);
	return ret;
}
