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
	void (*atomic_crtc_begin)(struct mtk_tv_drm_crtc *crtc,
		struct drm_crtc_state *old_crtc_state);
	void (*atomic_crtc_flush)(struct mtk_tv_drm_crtc *crtc,
		struct drm_crtc_state *old_crtc_state);
	int (*gamma_set)(struct drm_crtc *crtc,
		uint16_t *r, uint16_t *g, uint16_t *b, uint32_t size,
		struct drm_modeset_acquire_ctx *ctx);
	int (*check_plane)(struct drm_plane_state *plane_state,
		const struct drm_crtc_state *crtc_state,
		struct mtk_plane_state *state);
	int (*graphic_set_testpattern)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_graphic_testpattern *testpatternInfo);
	int (*set_stub_mode)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_common_stub *stubInfo);
	int (*start_gfx_pqudata)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_gfx_pqudata *pquInfo);
	int (*stop_gfx_pqudata)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_gfx_pqudata *pquInfo);
	int (*pnlgamma_enable)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pnlgamma_enable *enableInfo);
	int (*pnlgamma_gainoffset)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pnlgamma_gainoffset *gainoffsetInfo);
	int (*pnlgamma_gainoffset_enable)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pnlgamma_gainoffset_enable *enableInfo);
	int (*get_fence)(struct mtk_tv_drm_crtc *mcrtc,
		struct drm_mtk_tv_fence_info *fenceInfo);
	int (*pqgamma_curve)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pqgamma_curve *curve);
	int (*pqgamma_enable)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pqgamma_enable *enableInfo);
	int (*pqgamma_gainoffset)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pqgamma_gainoffset *gainoffset);
	int (*pqgamma_gainoffset_enable)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pqgamma_gainoffset_enable *enableInfo);
	int (*pqgamma_maxvalue)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pqgamma_maxvalue *maxvalue);
	int (*pqgamma_maxvalue_enable)(struct mtk_tv_drm_crtc *crtc,
		struct drm_mtk_tv_pqgamma_maxvalue_enable *enableInfo);
	int (*timeline_inc)(struct mtk_tv_drm_crtc *mcrtc, int planeIdx);
	int (*set_graphic_pq_buf)(struct mtk_tv_drm_crtc *mcrtc,
		struct drm_mtk_tv_graphic_pq_setting *PqBufInfo);
	int (*get_graphic_roi)(struct mtk_tv_drm_crtc *mcrtc,
		struct drm_mtk_tv_graphic_roi_info *RoiInfo);
};

struct mtk_tv_drm_crtc {
	struct drm_crtc base;
	struct mtk_drm_plane *plane[MTK_DRM_PLANE_TYPE_MAX];
	int plane_count[MTK_DRM_PLANE_TYPE_MAX];
	bool pending_needs_vblank;
	bool active_change;
	struct drm_pending_vblank_event *event;
	const struct mtk_tv_drm_crtc_ops *ops;
	enum drm_mtk_crtc_type mtk_crtc_type;
	void *crtc_private;
};

int mtk_tv_drm_crtc_enable_vblank(struct drm_device *drm, unsigned int pipe);
void mtk_tv_drm_crtc_disable_vblank(struct drm_device *drm, unsigned int pipe);
int mtk_tv_drm_crtc_create(struct drm_device *drm_dev, struct mtk_tv_drm_crtc *mtk_tv_crtc,
	struct drm_plane *plane, struct drm_plane *cursor_plane,
	const struct mtk_tv_drm_crtc_ops *ops);
int mtk_tv_drm_bootlogo_ctrl_ioctl(struct drm_device *dev, void *data, struct drm_file *file_priv);
int mtk_tv_drm_set_graphic_testpattern_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_set_stub_mode_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_start_gfx_pqudata_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_stop_gfx_pqudata_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_set_pnlgamma_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_set_pnlgamma_gainoffset_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_set_pnlgamma_gainoffset_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_get_fence_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_set_pqgamma_curve_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_pqgamma_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_set_pqgamma_gainoffset_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_pqgamma_gainoffset_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_set_pqgamma_maxvalue_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_pqgamma_maxvalue_enable_ioctl(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
int mtk_tv_drm_timeline_inc_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_graphic_set_pq_buf_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);
int mtk_tv_drm_graphic_get_roi_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv);

#endif //_MTK_TV_DRM_CRTC_H_
