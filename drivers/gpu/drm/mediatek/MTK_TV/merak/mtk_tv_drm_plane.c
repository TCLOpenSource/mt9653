// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: CK Hu <ck.hu@mediatek.com>
 */

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>

#include "mtk_tv_drm_fb.h"
#include "mtk_tv_drm_gem.h"
#include "mtk_tv_drm_plane.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_log.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_PLANE

static const u32 formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888,
	DRM_FORMAT_BGRA8888,
	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_ABGR4444,
	DRM_FORMAT_ABGR1555,
	DRM_FORMAT_BGRA5551,
	DRM_FORMAT_BGRA4444,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_ARGB2101010,
	DRM_FORMAT_ABGR2101010,
	DRM_FORMAT_RGBA1010102,
	DRM_FORMAT_BGRA1010102,
	DRM_FORMAT_XRGB2101010,
	DRM_FORMAT_XBGR2101010,
	DRM_FORMAT_RGBX1010102,
	DRM_FORMAT_BGRX1010102,
	// Format only for video
	DRM_FORMAT_NV21,//2 plane
	DRM_FORMAT_NV61,//2 plane
	DRM_FORMAT_YUV444,//3 plane
	DRM_FORMAT_VUY888,//1 plane
	DRM_FORMAT_YUV420_8BIT,//1 plane
	DRM_FORMAT_RGB888//1 plane
};

static void mtk_plane_reset(struct drm_plane *plane)
{
	struct mtk_plane_state *state;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);

	if (plane->state) {
		__drm_atomic_helper_plane_destroy_state(plane->state);

		state = to_mtk_plane_state(plane->state);
		memset(state, 0, sizeof(*state));
	} else {
		state = kzalloc(sizeof(*state), GFP_KERNEL);
		if (!state)
			return;
		plane->state = &state->base;
	}

	state->base.plane = plane;
	state->pending.format = DRM_FORMAT_RGB565;
	state->base.zpos = mplane->zpos;
}

static struct drm_plane_state *mtk_plane_duplicate_state(struct drm_plane *plane)
{
	struct mtk_plane_state *old_state = to_mtk_plane_state(plane->state);
	struct mtk_plane_state *state;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return NULL;

	__drm_atomic_helper_plane_duplicate_state(plane, &state->base);

	WARN_ON(state->base.plane != plane);

	state->pending = old_state->pending;

	return &state->base;
}

static void mtk_drm_plane_destroy_state(struct drm_plane *plane,
					struct drm_plane_state *state)
{
	__drm_atomic_helper_plane_destroy_state(state);
	kfree(to_mtk_plane_state(state));
}

static int mtk_tv_plane_atomic_set_property(struct drm_plane *plane,
					  struct drm_plane_state *state,
					  struct drm_property *property,
					  uint64_t val)
{
	int ret = -EINVAL;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct drm_crtc *crtc = mplane->crtc_base;
	struct mtk_tv_drm_crtc *mcrtc = to_mtk_tv_crtc(crtc);

	if (mcrtc->ops->atomic_set_plane_property)
		ret = mcrtc->ops->atomic_set_plane_property(mplane, state, property, val);

	return ret;
}

static int mtk_tv_plane_atomic_get_property(struct drm_plane *plane,
					  const struct drm_plane_state *state,
					  struct drm_property *property,
					  uint64_t *val)
{
	int ret = -EINVAL;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct drm_crtc *crtc = mplane->crtc_base;
	struct mtk_tv_drm_crtc *mcrtc = to_mtk_tv_crtc(crtc);

	if (mcrtc->ops->atomic_get_plane_property)
		ret = mcrtc->ops->atomic_get_plane_property(mplane, state, property, val);

	return ret;
}

static const struct drm_plane_funcs mtk_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = mtk_plane_reset,
	.atomic_duplicate_state = mtk_plane_duplicate_state,
	.atomic_destroy_state = mtk_drm_plane_destroy_state,
	.atomic_set_property = mtk_tv_plane_atomic_set_property,
	.atomic_get_property = mtk_tv_plane_atomic_get_property,
};

static int mtk_plane_atomic_check(struct drm_plane *plane,
				  struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct drm_crtc_state *crtc_state;

	struct mtk_plane_state *plane_state = to_mtk_plane_state(plane->state);
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct drm_crtc *crtc = mplane->crtc_base;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	int ret = 0;

	if (!fb)
		return 0;

	if (!state->crtc)
		return 0;

	crtc_state = drm_atomic_get_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	if (mtk_tv_crtc->ops->check_plane)
		ret = mtk_tv_crtc->ops->check_plane(state, crtc_state, plane_state);

	return ret;
}

static void mtk_plane_atomic_disable(struct drm_plane *plane,
				     struct drm_plane_state *old_state)
{
	struct mtk_plane_state *state = to_mtk_plane_state(plane->state);
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct drm_crtc *crtc = mplane->crtc_base;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	state->pending.enable = false;
	wmb(); /* Make sure the above parameter is set before update */
	state->pending.dirty = true;

	if (mtk_tv_crtc->ops->disable_plane)
		mtk_tv_crtc->ops->disable_plane(state);

}

static void mtk_plane_atomic_update(struct drm_plane *plane,
				    struct drm_plane_state *old_state)
{
	struct mtk_plane_state *state = to_mtk_plane_state(plane->state);
	struct drm_crtc *crtc = plane->state->crtc;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);

	struct drm_framebuffer *fb = plane->state->fb;
	struct drm_gem_object *gem;
	struct mtk_drm_gem_obj *mtk_gem;
	unsigned int pitch, format;
	dma_addr_t addr;

	if (!crtc || WARN_ON(!fb))
		return;

	gem = fb->obj[0];
	mtk_gem = to_mtk_gem_obj(gem);
	addr = mtk_gem->dma_addr;
	pitch = fb->pitches[0];
	format = fb->format->format;
	mplane->crtc_base = crtc;

	state->pending.enable = true;
	state->pending.pitch = pitch;
	state->pending.format = format;
	state->pending.addr = addr;
	state->pending.x = plane->state->dst.x1;
	state->pending.y = plane->state->dst.y1;
	state->pending.width = drm_rect_width(&plane->state->dst);
	state->pending.height = drm_rect_height(&plane->state->dst);
	state->pending.fb_width = fb->width;
	state->pending.fb_height = fb->height;

	wmb(); /* Make sure the above parameters are set before update */
	state->pending.dirty = true;

	if (mtk_tv_crtc->ops->update_plane)
		mtk_tv_crtc->ops->update_plane(state);
}

static const struct drm_plane_helper_funcs mtk_plane_helper_funcs = {
	.prepare_fb = drm_gem_fb_prepare_fb,
	.atomic_check = mtk_plane_atomic_check,
	.atomic_update = mtk_plane_atomic_update,
	.atomic_disable = mtk_plane_atomic_disable,
};

int mtk_plane_init(struct drm_device *dev, struct drm_plane *plane,
		   unsigned long possible_crtcs, enum drm_plane_type type)
{
	int err;

	err = drm_universal_plane_init(dev, plane, possible_crtcs,
				       &mtk_plane_funcs, formats,
				       ARRAY_SIZE(formats), NULL, type, NULL);
	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		return err;
	}

	drm_plane_helper_add(plane, &mtk_plane_helper_funcs);

	return 0;
}
