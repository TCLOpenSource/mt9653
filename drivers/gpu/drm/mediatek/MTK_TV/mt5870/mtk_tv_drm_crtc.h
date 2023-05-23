/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_CRTC_H_
#define _MTK_TV_DRM_CRTC_H_

#include "mtk_tv_drm_drv.h"
#include "mtk_tv_drm_plane.h"

#define to_mtk_tv_crtc(x)   container_of(x, struct mtk_tv_drm_crtc, base)

struct mtk_tv_drm_crtc;
struct mtk_tv_drm_crtc_ops {
	void (*enable)(struct mtk_tv_drm_crtc *crtc);
	void (*disable)(struct mtk_tv_drm_crtc *crtc);
	int (*enable_vblank)(struct mtk_tv_drm_crtc *crtc);
	void (*disable_vblank)(struct mtk_tv_drm_crtc *crtc);
	void (*update_plane)(struct mtk_plane_state *state);
	void (*disable_plane)(struct mtk_plane_state *state);
	int (*atomic_set_crtc_property)(struct mtk_tv_drm_crtc *crtc,
		struct drm_crtc_state *state,
		struct drm_property *property,
		uint64_t val);
	int (*atomic_get_crtc_property)(struct mtk_tv_drm_crtc *crtc,
		const struct drm_crtc_state *state,
		struct drm_property *property,
		uint64_t *val);
	int (*atomic_set_plane_property)(struct mtk_drm_plane *mplane,
		struct drm_plane_state *state,
		struct drm_property *property, uint64_t val);
	int (*atomic_get_plane_property)(struct mtk_drm_plane *mplane,
		const struct drm_plane_state *state,
		struct drm_property *property,
		uint64_t *val);
	int (*bootlogo_ctrl)(struct mtk_tv_drm_crtc *crtc,
		unsigned int cmd,
		unsigned int *gop);
	void (*atomic_crtc_flush)(struct mtk_tv_drm_crtc *crtc,
		struct drm_crtc_state *old_crtc_state);
	int (*gamma_set)(struct drm_crtc *crtc,
		uint16_t *r, uint16_t *g, uint16_t *b, uint32_t size,
		struct drm_modeset_acquire_ctx *ctx);
	int (*check_plane)(struct drm_plane_state *plane_state,
		const struct drm_crtc_state *crtc_state,
		struct mtk_plane_state *state);
};

struct mtk_tv_drm_crtc {
	struct drm_crtc base;
	struct mtk_drm_plane *plane[MTK_DRM_PLANE_TYPE_MAX];
	int plane_count[MTK_DRM_PLANE_TYPE_MAX];
	bool pending_needs_vblank;
	struct drm_pending_vblank_event *event;
	const struct mtk_tv_drm_crtc_ops *ops;
	void *crtc_private;
};

int mtk_tv_drm_crtc_enable_vblank(struct drm_device *drm, unsigned int pipe);
void mtk_tv_drm_crtc_disable_vblank(struct drm_device *drm, unsigned int pipe);
int mtk_tv_drm_crtc_create(struct drm_device *drm_dev, struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_plane *plane, struct drm_plane *cursor_plane,
	const struct mtk_tv_drm_crtc_ops *ops);
int mtk_tv_drm_bootlogo_ctrl_ioctl(struct drm_device *dev, void *data, struct drm_file *file_priv);

#endif //_MTK_TV_DRM_CRTC_H_
