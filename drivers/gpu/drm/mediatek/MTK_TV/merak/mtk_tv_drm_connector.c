// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_log.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_CONNECTOR

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
	int ret = 0;
	if (!connector) {
		DRM_WARN("connector is NULL.\n");
		return 0; // return 0 as zero connector
	}
	ret = drm_panel_get_modes(connector->panel);

	return ret;
}

static enum drm_mode_status mtk_tv_drm_connector_mode_valid(struct drm_connector *connector,
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

void _attachDrmPanel(struct drm_device *drm_dev, struct mtk_tv_drm_connector *mtkConnector)
{
	struct device_node *deviceNodePanel = NULL;
	struct drm_panel *drmPanel = NULL;
	const char *pnlname = "";

	if (mtkConnector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_VIDEO)
		pnlname = "video_out";
	else if (mtkConnector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_GRAPHIC)
		pnlname = "graphic_out";
	else if (mtkConnector->mtk_connector_type == MTK_DRM_CONNECTOR_TYPE_EXT_VIDEO)
		pnlname = "ext_video_out";
	else
		DRM_WARN("panel connector type is not available\n");

	if (pnlname != NULL) {
		deviceNodePanel = of_find_node_by_name(NULL, pnlname);
		if (!of_device_is_available(deviceNodePanel)) {
			DRM_WARN("panel node is not available\n");
		} else {
			drmPanel = of_drm_find_panel(deviceNodePanel);
			drmPanel->drm = drm_dev;
			drmPanel->connector = &mtkConnector->base;
			mtkConnector->base.panel = drmPanel;
		}
		of_node_put(deviceNodePanel);
	}
}

int mtk_tv_drm_connector_create(struct drm_device *drm_dev, struct mtk_tv_drm_connector *connector,
				struct drm_encoder *encoder,
				const struct mtk_tv_drm_connector_ops *ops)
{
	int ret = 0;

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

	_attachDrmPanel(drm_dev, connector);

	return 0;

err_sysfs:
	drm_connector_unregister(&connector->base);
err_cleanup:
	drm_connector_cleanup(&connector->base);
	return ret;
}
