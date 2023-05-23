// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015 MediaTek Inc.
 */

#include <linux/dma-buf.h>
#include <linux/dma-resv.h>

#include <drm/drm_modeset_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_print.h>

#include "../../mtk_drm_drv.h"
#include "mtk_tv_drm_fb.h"
#include "mtk_tv_drm_gem.h"
#include "mtk_tv_drm_log.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_FB

static const struct drm_framebuffer_funcs mtk_drm_fb_funcs = {
	.create_handle = drm_gem_fb_create_handle,
	.destroy = drm_gem_fb_destroy,
};

static struct drm_framebuffer *mtk_drm_framebuffer_init(struct drm_device *dev,
					const struct drm_mode_fb_cmd2 *mode,
					struct drm_gem_object *obj)
{
	const struct drm_format_info *info = drm_get_format_info(dev, mode);
	struct drm_framebuffer *fb;
	int ret;

	/*
	if (info->num_planes != 1)
		return ERR_PTR(-EINVAL);
	*/
	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if (!fb)
		return ERR_PTR(-ENOMEM);

	drm_helper_mode_fill_fb_struct(dev, fb, mode);

	fb->obj[0] = obj;

	ret = drm_framebuffer_init(dev, fb, &mtk_drm_fb_funcs);
	if (ret) {
		DRM_ERROR("failed to initialize framebuffer\n");
		kfree(fb);
		return ERR_PTR(ret);
	}

	return fb;
}

struct drm_framebuffer *mtk_drm_mode_fb_create(struct drm_device *dev,
					       struct drm_file *file,
					       const struct drm_mode_fb_cmd2 *cmd)
{
	const struct drm_format_info *info = drm_get_format_info(dev, cmd);
	struct drm_framebuffer *fb;
	struct drm_gem_object *gem;
	unsigned int width = cmd->width;
	unsigned int height = cmd->height;
	unsigned int size, bpp;
	int ret, hsub, vsub, num_planes;
	int last_plane_id = 0;

	/*
	if (info->num_planes != 1)
		return ERR_PTR(-EINVAL);
	*/

	num_planes = info->num_planes;
	last_plane_id = num_planes - 1;

	if (num_planes <= 0) {
		DRM_ERROR("invalid num_planes %d!\n", num_planes);
		return -EINVAL;
	}

	// first plane no need to consider hsub & vsub ...?
	hsub = (last_plane_id != 0 ? info->hsub : 1);
	vsub = (last_plane_id != 0 ? info->vsub : 1);

	DRM_DEBUG_KMS("[%s][%d] hsub:%d vsub:%d num_planes:%d\n",
		__func__, __LINE__, hsub, vsub, num_planes);

	/* checking buffer size: only need to check last plane offset + size */
	gem = drm_gem_object_lookup(file, cmd->handles[0]);
	if (!gem) {
		DRM_ERROR("can't find gem object! handle=%d\n", cmd->handles[0]);
		return ERR_PTR(-ENOENT);
	}

	size = cmd->pitches[last_plane_id] * height / hsub / vsub;
	size += cmd->offsets[last_plane_id];

	DRM_DEBUG_KMS("[%s][%d] plane:%d pitches:0x%x offsets:0x%lx height:%d hsub:%d vsub:%d\n",
		__func__, __LINE__, last_plane_id, cmd->pitches[last_plane_id],
		cmd->offsets[last_plane_id], height, hsub, vsub);

	if (gem->size < size) {
		DRM_ERROR("addfb size larger than gem size! gem_size=0x%x, addfb_size=0x%x\n",
			gem->size, size);
		ret = -EINVAL;
		goto unreference;
	}

	fb = mtk_drm_framebuffer_init(dev, cmd, gem);
	if (IS_ERR(fb)) {
		ret = PTR_ERR(fb);
		DRM_ERROR("mtk_drm_framebuffer_init fail, ret=%d\n", ret);
		goto unreference;
	}

	return fb;

unreference:
	drm_gem_object_put_unlocked(gem);
	return ERR_PTR(ret);
}
