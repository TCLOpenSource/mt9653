/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_CONNECTOR_H_
#define _MTK_TV_DRM_CONNECTOR_H_
struct mtk_tv_drm_connector;
struct mtk_tv_drm_connector_ops {
	int (*atomic_set_connector_property)(struct mtk_tv_drm_connector *connector,
					       struct drm_connector_state *state,
					       struct drm_property *roperty, uint64_t val);
	int (*atomic_get_connector_property)(struct mtk_tv_drm_connector *connector,
					       const struct drm_connector_state *state,
					       struct drm_property *property, uint64_t *val);
};
struct mtk_tv_drm_connector {
	struct drm_connector base;
	struct drm_encoder *encoder;
	const struct mtk_tv_drm_connector_ops *ops;
	void *connector_private;
};
int mtk_tv_drm_connector_create(struct drm_device *drm_dev,
				   struct mtk_tv_drm_connector *connector,
				   struct drm_encoder *encoder,
				   const struct mtk_tv_drm_connector_ops *ops);

#endif
