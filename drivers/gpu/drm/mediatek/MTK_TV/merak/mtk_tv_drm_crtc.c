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
#include "mtk_tv_drm_log.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_CRTC

int mtk_tv_drm_crtc_enable_vblank(struct drm_device *drm, unsigned int pipe)
{
	struct mtk_tv_drm_crtc *mtk_tv_crtc;
	struct drm_crtc *crtc;

	drm_for_each_crtc(crtc, drm) {
		if (crtc->index == pipe) {
			mtk_tv_crtc = to_mtk_tv_crtc(crtc);
			if (mtk_tv_crtc->ops->enable_vblank)
				mtk_tv_crtc->ops->enable_vblank(mtk_tv_crtc);
		}
	}

	return 0;
}

void mtk_tv_drm_crtc_disable_vblank(struct drm_device *drm, unsigned int pipe)
{
	struct mtk_tv_drm_crtc *mtk_tv_crtc;
	struct drm_crtc *crtc;

	drm_for_each_crtc(crtc, drm) {
		if (crtc->index == pipe) {
			mtk_tv_crtc = to_mtk_tv_crtc(crtc);
			if (mtk_tv_crtc->ops->disable_vblank)
				mtk_tv_crtc->ops->disable_vblank(mtk_tv_crtc);
		}
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

	if (mtk_tv_crtc->ops->atomic_crtc_begin)
		mtk_tv_crtc->ops->atomic_crtc_begin(mtk_tv_crtc, old_crtc_state);

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

int mtk_tv_drm_set_graphic_testpattern_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_graphic_testpattern *args = (struct drm_mtk_tv_graphic_testpattern *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->bootlogo_ctrl)
			ret = mtk_tv_crtc->ops->graphic_set_testpattern(mtk_tv_crtc, args);
	}

	return ret;
}

int mtk_tv_drm_graphic_set_pq_buf_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_graphic_pq_setting *args = (struct drm_mtk_tv_graphic_pq_setting *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);

		if (mtk_tv_crtc->ops->set_graphic_pq_buf) {
			ret = mtk_tv_crtc->ops->set_graphic_pq_buf(mtk_tv_crtc, args);
			break;
		}
	}

	return ret;
}

int mtk_tv_drm_graphic_get_roi_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_graphic_roi_info *args = (struct drm_mtk_tv_graphic_roi_info *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);

		if (mtk_tv_crtc->ops->get_graphic_roi) {
			ret = mtk_tv_crtc->ops->get_graphic_roi(mtk_tv_crtc, args);
			break;
		}
	}

	return ret;
}

int mtk_tv_drm_set_stub_mode_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_common_stub *args = (struct drm_mtk_tv_common_stub *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->set_stub_mode) {
			ret = mtk_tv_crtc->ops->set_stub_mode(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_start_gfx_pqudata_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_gfx_pqudata *args = (struct drm_mtk_tv_gfx_pqudata *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->start_gfx_pqudata) {
			ret = mtk_tv_crtc->ops->start_gfx_pqudata(mtk_tv_crtc, args);
			break;
		}
	}

	return ret;
}

int mtk_tv_drm_stop_gfx_pqudata_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_gfx_pqudata *args = (struct drm_mtk_tv_gfx_pqudata *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->stop_gfx_pqudata) {
			ret = mtk_tv_crtc->ops->stop_gfx_pqudata(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_set_pnlgamma_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pnlgamma_enable *args = (struct drm_mtk_tv_pnlgamma_enable *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pnlgamma_enable) {
			ret = mtk_tv_crtc->ops->pnlgamma_enable(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_set_pnlgamma_gainoffset_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pnlgamma_gainoffset *args = (struct drm_mtk_tv_pnlgamma_gainoffset *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pnlgamma_gainoffset) {
			ret = mtk_tv_crtc->ops->pnlgamma_gainoffset(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_set_pnlgamma_gainoffset_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pnlgamma_gainoffset_enable *args =
		(struct drm_mtk_tv_pnlgamma_gainoffset_enable *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pnlgamma_gainoffset) {
			ret = mtk_tv_crtc->ops->pnlgamma_gainoffset_enable(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_get_fence_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_fence_info *mstFence = (struct drm_mtk_tv_fence_info *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->get_fence) {
			ret = mtk_tv_crtc->ops->get_fence(mtk_tv_crtc, mstFence);
			break;
		}
	}

	return ret;
}

int mtk_tv_drm_pqgamma_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pqgamma_enable *args = (struct drm_mtk_tv_pqgamma_enable *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pqgamma_enable) {
			ret = mtk_tv_crtc->ops->pqgamma_enable(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_set_pqgamma_curve_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pqgamma_curve *args = (struct drm_mtk_tv_pqgamma_curve *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pqgamma_curve) {
			ret = mtk_tv_crtc->ops->pqgamma_curve(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_set_pqgamma_gainoffset_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pqgamma_gainoffset *args = (struct drm_mtk_tv_pqgamma_gainoffset *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pqgamma_gainoffset) {
			ret = mtk_tv_crtc->ops->pqgamma_gainoffset(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_pqgamma_gainoffset_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pqgamma_gainoffset_enable *args =
		(struct drm_mtk_tv_pqgamma_gainoffset_enable *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pqgamma_gainoffset_enable) {
			ret = mtk_tv_crtc->ops->pqgamma_gainoffset_enable(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_set_pqgamma_maxvalue_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pqgamma_maxvalue *args =
		(struct drm_mtk_tv_pqgamma_maxvalue *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pqgamma_maxvalue) {
			ret = mtk_tv_crtc->ops->pqgamma_maxvalue(mtk_tv_crtc, args);
			break;
		}
	}

	return ret;
}

int mtk_tv_drm_pqgamma_maxvalue_enable_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	struct drm_mtk_tv_pqgamma_maxvalue_enable *args =
		(struct drm_mtk_tv_pqgamma_maxvalue_enable *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->pqgamma_maxvalue_enable) {
			ret = mtk_tv_crtc->ops->pqgamma_maxvalue_enable(mtk_tv_crtc, args);
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_timeline_inc_ioctl(struct drm_device *dev, void *data,
						struct drm_file *file_priv)
{
	int ret = 0;
	int *planeIdx = (int *)data;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;

	drm_for_each_crtc(crtc, dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		if (mtk_tv_crtc->ops->timeline_inc) {
			ret = mtk_tv_crtc->ops->timeline_inc(mtk_tv_crtc, *planeIdx);
			break;
		}
	}

	return ret;
}
