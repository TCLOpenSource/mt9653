// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/mtk_tv_drm.h>

#include "mtk_tv_drm_drv.h"
#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_fb.h"
#include "mtk_tv_drm_gem.h"
#include "mtk_tv_drm_log.h"

#include "mtk_tv_drm_video_frc_thermal.h"

#define DRIVER_NAME "mediatek-tv"
#define DRIVER_DESC "Mediatek TV SoC DRM"
#define DRIVER_DATE "20200113"
#define DRIVER_MAJOR 1
#define DRIVER_MINOR 0

#define MTK_DRM_MODEL MTK_DRM_MODEL_DRV
#define MTK_DRM_DRV_COMPATIBLE_STR "MTK-drm-tv"
#define MTK_DRM_DRV_DTS_BUS_OFFSET	"MIU_BUS_OFFSET"
#define MTK_DRM_DRV_DTS_RECOVERY_BUF_TAG	"RECOVERY_BUG_TAG"

static struct component_match *mtk_drm_match_add(struct device *dev);
static int mtk_drm_tv_bind(struct device *dev);
static void mtk_drm_tv_unbind(struct device *dev);

bool bPquEnable;

static const struct drm_mode_config_funcs mtk_tv_drm_mode_config_funcs = {
	.fb_create = mtk_drm_mode_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = mtk_tv_drm_atomic_commit,
	.atomic_state_clear = mtk_tv_kms_atomic_state_clear,
};

static const struct file_operations mtk_tv_drm_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.mmap = mtk_drm_gem_mmap,
	.poll = drm_poll,
	.read = drm_read,
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
};

static struct drm_ioctl_desc mtk_tv_drm_ioctl[] = {
	DRM_IOCTL_DEF_DRV(MTK_TV_GEM_CREATE, mtk_tv_drm_gem_create_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_CTRL_BOOTLOGO, mtk_tv_drm_bootlogo_ctrl_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_GRAPHIC_TESTPATTERN, mtk_tv_drm_set_graphic_testpattern_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_STUB_MODE, mtk_tv_drm_set_stub_mode_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_START_GFX_PQUDATA, mtk_tv_drm_start_gfx_pqudata_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_STOP_GFX_PQUDATA, mtk_tv_drm_stop_gfx_pqudata_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_PNLGAMMA_ENABLE, mtk_tv_drm_set_pnlgamma_enable_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_PNLGAMMA_GAINOFFSET, mtk_tv_drm_set_pnlgamma_gainoffset_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_PNLGAMMA_GAINOFFSET_ENABLE,
			  mtk_tv_drm_set_pnlgamma_gainoffset_enable_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_CVBSO_MODE, mtk_tv_drm_set_cvbso_mode_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	// DRM_IOCTL_DEF_DRV(MTK_TV_..., ..., (DRM_UNLOCKED/*| DRM_AUTH */)), // Remove
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_PQMAP_INFO, mtk_tv_drm_pqmap_set_info_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_GET_VIDEO_DELAY, mtk_tv_drm_video_get_delay_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_GET_OUTPUT_INFO, mtk_tv_drm_video_get_outputinfo_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_GET_FENCE, mtk_tv_drm_get_fence_ioctl,
			(DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_PQGAMMA_CURVE, mtk_tv_drm_set_pqgamma_curve_ioctl,
			(DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_ENABLE_PQGAMMA, mtk_tv_drm_pqgamma_enable_ioctl,
			(DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_PQGAMMA_GAINOFFSET, mtk_tv_drm_set_pqgamma_gainoffset_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_ENABLE_PQGAMMA_GAINOFFSET,
			mtk_tv_drm_pqgamma_gainoffset_enable_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_PQGAMMA_MAXVALUE, mtk_tv_drm_set_pqgamma_maxvalue_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_ENABLE_PQGAMMA_MAXVALUE,
			mtk_tv_drm_pqgamma_maxvalue_enable_ioctl,
			  (DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_GET_PQU_METADATA, mtk_tv_drm_pqu_metadata_get_copy_ioctl,
			(DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_TIMELINE_INC, mtk_tv_drm_timeline_inc_ioctl,
			(DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_SET_GRAPHIC_PQ_BUF, mtk_tv_drm_graphic_set_pq_buf_ioctl,
			(DRM_UNLOCKED/*| DRM_AUTH */)),
	DRM_IOCTL_DEF_DRV(MTK_TV_GET_GRAPHIC_ROI, mtk_tv_drm_graphic_get_roi_ioctl,
			(DRM_UNLOCKED/*| DRM_AUTH */)),
};

static struct drm_driver mtk_drm_tv_driver = {
	.driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_ATOMIC,
	.gem_free_object_unlocked = mtk_drm_gem_free_object,
	.dumb_create = mtk_drm_gem_dumb_create,
	.enable_vblank = mtk_tv_drm_crtc_enable_vblank,
	.disable_vblank = mtk_tv_drm_crtc_disable_vblank,
	.gem_vm_ops = &drm_gem_cma_vm_ops,
	.ioctls = mtk_tv_drm_ioctl,
	.num_ioctls = ARRAY_SIZE(mtk_tv_drm_ioctl),
	.fops = &mtk_tv_drm_fops,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
};

static const struct component_master_ops mtk_drm_tv_ops = {
	.bind = mtk_drm_tv_bind,
	.unbind = mtk_drm_tv_unbind,
};

static int compare_dev(struct device *dev, void *data)
{
	return dev == (struct device *)data;
}

static int readDTB2DrvPrivate(struct mtk_tv_drm_private *priv)
{
	int ret;
	struct device_node *np;
	int u32Tmp = 0xFF;
	const char *name;

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_DRV_COMPATIBLE_STR);

	if (np != NULL) {
		name = MTK_DRM_DRV_DTS_BUS_OFFSET;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: of_property_read_u32 failed, name = %s \r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		priv->bus_offset = u32Tmp;

		name = MTK_DRM_DRV_DTS_RECOVERY_BUF_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d]: of_property_read_u32 failed, name = %s \r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		priv->recovery_buf_tag = u32Tmp;
	}

	return 0;
}

static int mtk_drm_tv_probe(struct platform_device *pdev)
{
	boottime_print("MTK DRM insmod [begin]\n");
	struct device *dev = &pdev->dev;
	struct mtk_tv_drm_private *private;
	struct component_match *match;
	int ret;

	private = devm_kzalloc(dev, sizeof(*private), GFP_KERNEL);
	if (!private)
		return -ENOMEM;

	ret = readDTB2DrvPrivate(private);
	if (ret != 0x0) {
		DRM_ERROR("readDeviceTree2Context failed.\n");
		return -ENODEV;
	}

	private->dev = dev;
	match = mtk_drm_match_add(&pdev->dev);

	if (IS_ERR(match))
		return PTR_ERR(match);

	platform_set_drvdata(pdev, private);

	ret = component_master_add_with_match(&pdev->dev, &mtk_drm_tv_ops, match);

	ret = mtk_frc_cdev_probe(pdev);

	boottime_print("MTK DRM insmod [end]\n");
	return ret;
}

static int mtk_drm_tv_remove(struct platform_device *pdev)
{
	int ret;

	ret = mtk_frc_cdev_remove(pdev);
	return ret;
}

static int mtk_drm_tv_bind(struct device *dev)
{
	struct drm_device *drm;
	struct mtk_tv_drm_private *private = dev_get_drvdata(dev);
	int ret = 0x0;

	drm = drm_dev_alloc(&mtk_drm_tv_driver, dev);
	if (IS_ERR(drm))
		return PTR_ERR(drm);

	drm->dev_private = private;
	private->drm = drm;

	drm_mode_config_init(drm);
	drm->mode_config.min_width = 0;
	drm->mode_config.min_height = 0;
	drm->mode_config.max_width = 7680;
	drm->mode_config.max_height = 4320;
	drm->mode_config.allow_fb_modifiers = true;
	drm->mode_config.funcs = &mtk_tv_drm_mode_config_funcs;

	ret = component_bind_all(drm->dev, drm);
	if (ret)
		goto err_mode_config_cleanup;

	drm->irq_enabled = true;
	ret = drm_vblank_init(drm, drm->mode_config.num_crtc);
	if (ret)
		goto err_unbind_all;

	drm_mode_config_reset(drm);

	ret = drm_dev_register(drm, 0);
	if (ret < 0)
		goto err_unbind_all;

	return 0;

err_unbind_all:
	component_unbind_all(drm->dev, drm);
err_mode_config_cleanup:
	drm_mode_config_cleanup(drm);
	kfree(private);

	return ret;
}

static void mtk_drm_tv_unbind(struct device *dev)
{
}

static const struct of_device_id mtk_drm_tv_of_ids[] = {
	{.compatible = MTK_DRM_DRV_COMPATIBLE_STR,},
	{},
};

static struct platform_driver mtk_drm_tv_platform_driver = {
	.probe = mtk_drm_tv_probe,
	.remove = mtk_drm_tv_remove,
	.driver = {
		   .name = "mediatek-drm-tv",
		   .of_match_table = mtk_drm_tv_of_ids,
		   },
};

static struct platform_driver *const mtk_drm_kms_drivers[] = {
	&mtk_tv_drm_extdev_panel_driver,
	&mtk_tv_drm_gfx_panel_driver,
	&mtk_tv_drm_panel_driver,
	&mtk_drm_tv_kms_driver,
};

static struct platform_driver *const mtk_drm_platform_driver[] = {
	&mtk_drm_tv_platform_driver,
};

static struct component_match *mtk_drm_match_add(struct device *dev)
{
	struct component_match *match = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(mtk_drm_kms_drivers); ++i) {
		struct device_driver *drv = &mtk_drm_kms_drivers[i]->driver;
		struct device *p = NULL, *d;

		while ((d = platform_find_device_by_driver(p, drv))) {
			put_device(p);
			component_match_add(dev, &match, compare_dev, d);
			p = d;
		}
		put_device(p);
	}

	return match ? : ERR_PTR(-ENODEV);
}

module_param_named(pqu_enable, bPquEnable, bool, 0660);
static int __init mtk_drm_init(void)
{
	int ret;
	int i;

	boottime_print("MTK DRM init [begin]\n");
	for (i = 0; i < ARRAY_SIZE(mtk_drm_kms_drivers); i++) {
		ret = platform_driver_register(mtk_drm_kms_drivers[i]);
		if (ret < 0) {
			pr_err("Failed to register %s driver: %d\n",
			       mtk_drm_kms_drivers[i]->driver.name, ret);
			goto err_unregister_kms_drivers;
		}
	}

	for (i = 0; i < ARRAY_SIZE(mtk_drm_platform_driver); i++) {
		ret = platform_driver_register(mtk_drm_platform_driver[i]);
		if (ret < 0) {
			pr_err("Failed to register %s driver: %d\n",
			       mtk_drm_platform_driver[i]->driver.name, ret);
			goto err;
		}
	}
	boottime_print("MTK DRM init [end]\n");

	return 0;

err:
	while (--i >= 0)
		platform_driver_unregister(mtk_drm_platform_driver[i]);
err_unregister_kms_drivers:
	while (--i >= 0)
		platform_driver_unregister(mtk_drm_kms_drivers[i]);

	return ret;
}

static void __exit mtk_drm_exit(void)
{
	int i;

	for (i = ARRAY_SIZE(mtk_drm_platform_driver) - 1; i >= 0; i--)
		platform_driver_unregister(mtk_drm_platform_driver[i]);

	for (i = 0; i < ARRAY_SIZE(mtk_drm_kms_drivers); i++)
		platform_driver_unregister(mtk_drm_kms_drivers[i]);
}

module_init(mtk_drm_init);
module_exit(mtk_drm_exit);

MODULE_AUTHOR("Mediatek TV group");
MODULE_DESCRIPTION("Mediatek SoC DRM driver");
MODULE_LICENSE("GPL v2");
