/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: CK Hu <ck.hu@mediatek.com>
 */

#ifndef _MTK_DRM_PLANE_H_
#define _MTK_DRM_PLANE_H_

#include <drm/drm_crtc.h>
#include <linux/types.h>

#include <drm/mtk_tv_drm.h>
struct mtk_drm_plane;

struct mtk_plane_pending_state {
	bool				config;
	bool				enable;
	dma_addr_t			addr;
	unsigned int			pitch;
	unsigned int			format;
	unsigned int			x;
	unsigned int			y;
	unsigned int			width;
	unsigned int			height;
	bool				dirty;
	unsigned int			fb_width;
	unsigned int			fb_height;
};

struct mtk_plane_state {
	struct drm_plane_state		base;
	struct mtk_plane_pending_state	pending;
};


struct mtk_drm_plane {
	struct drm_plane base;
	struct drm_crtc *crtc_base;
	unsigned int zpos;// range: 0~total_plane_in_drm
	union{
		unsigned int gop_index;	// range: 0~total_plane_in_gop
		unsigned int video_index;	// range: 0~total_plane_in_video,
						// total is from dtsi "VIDEO_PLANE_NUM"
		unsigned int primary_index;	// range: 0~total_plane_in_primary,
						//total is from dtsi "PRIMARY_PLANE_NUM"
	};
	enum drm_mtk_plane_type mtk_plane_type;
	unsigned int gop_use_ml_res_idx;
	void *crtc_private;
};

#define to_mtk_plane(x)	container_of(x, struct mtk_drm_plane, base)

static inline struct mtk_plane_state *
to_mtk_plane_state(struct drm_plane_state *state)
{
	return container_of(state, struct mtk_plane_state, base);
}

int mtk_plane_init(struct drm_device *dev, struct drm_plane *plane,
		   unsigned long possible_crtcs, enum drm_plane_type type);

#endif
