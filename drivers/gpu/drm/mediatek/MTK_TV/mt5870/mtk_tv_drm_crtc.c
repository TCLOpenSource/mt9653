// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/mtk_tv_drm.h>

#include "mtk_tv_drm_plane.h"
#include "mtk_tv_drm_crtc.h"

int mtk_tv_drm_crtc_enable_vblank(struct drm_device *drm, unsigned int pipe)
{
	struct mtk_tv_drm_crtc *mtk_tv_crtc;
	struct drm_crtc *crtc;

	drm_for_each_crtc(crtc, drm) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->enable_vblank)
			mtk_tv_crtc->ops->enable_vblank(mtk_tv_crtc);
	}

	return 0;
}

void mtk_tv_drm_crtc_disable_vblank(struct drm_device *drm, unsigned int pipe)
{
	struct mtk_tv_drm_crtc *mtk_tv_crtc;
	struct drm_crtc *crtc;

	drm_for_each_crtc(crtc, drm) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->disable_vblank)
			mtk_tv_crtc->ops->disable_vblank(mtk_tv_crtc);
	}
}

static void mtk_tv_drm_crtc_enable(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
{
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	if (mtk_tv_crtc->ops->enable)
		mtk_tv_crtc->ops->enable(mtk_tv_crtc);

	drm_crtc_vblank_on(crtc);
}

static void mtk_tv_drm_crtc_disable(struct drm_crtc *crtc)
{
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	drm_crtc_vblank_off(crtc);

	if (mtk_tv_crtc->ops->disable)
		mtk_tv_crtc->ops->disable(mtk_tv_crtc);
}

static void mtk_tv_drm_crtc_atomic_begin(struct drm_crtc *crtc,
					 struct drm_crtc_state *old_crtc_state)
{
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	if (crtc->state->event) {
		mtk_tv_crtc->event = crtc->state->event;
		mtk_tv_crtc->pending_needs_vblank = true;
		crtc->state->event = NULL;
	}
}

static void mtk_tv_drm_crtc_atomic_flush(struct drm_crtc *crtc,
					 struct drm_crtc_state *old_crtc_state)
{
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	if (mtk_tv_crtc->ops->atomic_crtc_flush)
		mtk_tv_crtc->ops->atomic_crtc_flush(mtk_tv_crtc, old_crtc_state);

}

static int mtk_tv_crtc_atomic_set_property(struct drm_crtc *crtc,
					   struct drm_crtc_state *state,
					   struct drm_property *property, uint64_t val)
{
	int ret = -EINVAL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	if (mtk_tv_crtc->ops->atomic_set_crtc_property)
		ret = mtk_tv_crtc->ops->atomic_set_crtc_property(mtk_tv_crtc, state, property, val);

	return ret;
}

static int mtk_tv_crtc_atomic_get_property(struct drm_crtc *crtc,
					   const struct drm_crtc_state *state,
					   struct drm_property *property, uint64_t *val)
{
	int ret = -EINVAL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	if (mtk_tv_crtc->ops->atomic_get_crtc_property)
		ret = mtk_tv_crtc->ops->atomic_get_crtc_property(mtk_tv_crtc, state, property, val);

	return ret;
}

static int mtk_tv_drm_crtc_gamma_set(struct drm_crtc *crtc, u16 *r, u16 *g, u16 *b,
				     uint32_t size, struct drm_modeset_acquire_ctx *ctx)
{
	int ret = -EINVAL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = to_mtk_tv_crtc(crtc);

	if (mtk_tv_crtc->ops->gamma_set)
		ret = mtk_tv_crtc->ops->gamma_set(crtc, r, g, b, size, ctx);

	return ret;
}

static struct drm_crtc_funcs mtk_tv_crtc_funcs = {
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.reset = drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
	.atomic_set_property = mtk_tv_crtc_atomic_set_property,
	.atomic_get_property = mtk_tv_crtc_atomic_get_property,
	.gamma_set = mtk_tv_drm_crtc_gamma_set,
};

static struct drm_crtc_helper_funcs mtk_tv_crtc_helper_funcs = {
	.atomic_enable = mtk_tv_drm_crtc_enable,
	.disable = mtk_tv_drm_crtc_disable,
	.atomic_begin = mtk_tv_drm_crtc_atomic_begin,
	.atomic_flush = mtk_tv_drm_crtc_atomic_flush,
};

int mtk_tv_drm_crtc_create(struct drm_device *drm_dev, struct mtk_tv_drm_crtc *mtk_tv_crtc,
			   struct drm_plane *plane, struct drm_plane *cursor_plane,
			   const struct mtk_tv_drm_crtc_ops *ops)
{
	int ret;

	mtk_tv_crtc->ops = ops;

	ret =
	    drm_crtc_init_with_planes(drm_dev, &mtk_tv_crtc->base, plane, cursor_plane,
				      &mtk_tv_crtc_funcs, NULL);

	if (ret < 0) {
		DRM_ERROR("[%s, %d]: drm_crtc_init_with_planes failed \r\n", __func__,
			  __LINE__);
		goto fail;
	}

	drm_crtc_helper_add(&mtk_tv_crtc->base, &mtk_tv_crtc_helper_funcs);

	return ret;
fail:
	drm_plane_cleanup(plane);
	return ret;
}

int mtk_tv_drm_bootlogo_ctrl_ioctl(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	int ret = -1;
	struct drm_mtk_tv_bootlogo_ctrl *args = (struct drm_mtk_tv_bootlogo_ctrl *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;
	unsigned int cmd;
	unsigned int gop;

	if (args == NULL)
		return -EINVAL;

	cmd = args->u8CmdType;
	gop = args->u8GopNum;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->bootlogo_ctrl) {
			ret = mtk_tv_crtc->ops->bootlogo_ctrl(mtk_tv_crtc, cmd, &gop);
			if (ret == 0x0) {
				args->u8GopNum = gop;
				break;
			}
		}
	}
	return ret;
}
