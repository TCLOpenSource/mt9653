// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <drm/mtk_tv_drm.h>

#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_crtc.h"

#include "hwreg_render_cvbso.h"

#define MTK_DRM_MODEL			MTK_DRM_MODEL_TVDAC
#define TVDAC_LABEL			"[TVDAC] "
#define MTK_DRM_CVBSO_STATUS		"cvbso_status"
#define MTK_DRM_CVBSO_INIT_IDX		REG_RENDER_CVBSO_VIF_VIF

#define TVDAC_MSG_POINTER_CHECK() \
	do { \
		pr_err(TVDAC_LABEL"[%s,%d] pointer is NULL\n", __func__, __LINE__); \
		dump_stack(); \
	} while (0)

#define TVDAC_MSG_RETURN_CHECK(result) \
	pr_err(TVDAC_LABEL"[%s,%d] return is %d\n", __func__, __LINE__, (result))

static int mtk_tvdac_get_clk(
	struct device *dev,
	char *s,
	struct clk **clk)
{
	if (dev == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (s == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (clk == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	*clk = devm_clk_get(dev, s);
	if (IS_ERR(*clk)) {
		DRM_ERROR("unable to retrieve %s\n", s);
		return PTR_ERR(*clk);
	}

	return 0;
}

static void mtk_tvdac_put_clk(
	struct device *dev,
	struct clk *clk)
{
	if (dev == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return;
	}
	if (clk == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return;
	}

	devm_clk_put(dev, clk);
}

static int mtk_tvdac_toggle_swen(
	struct device *dev,
	char *clk_name,
	char *enable)
{
	int ret = 0;
	struct clk *target_clk = NULL;

	if (dev == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (clk_name == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (enable == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = mtk_tvdac_get_clk(dev, clk_name, &target_clk);
	if (ret < 0) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (strcmp(enable, "ON") == 0) {
		if (!__clk_is_enabled(target_clk)) {
			ret = clk_prepare_enable(target_clk);
			if (ret < 0) {
				TVDAC_MSG_RETURN_CHECK(ret);
				mtk_tvdac_put_clk(dev, target_clk);
				return ret;
			}
		}
	}
	if (strcmp(enable, "OFF") == 0) {
		if (__clk_is_enabled(target_clk))
			clk_disable_unprepare(target_clk);
	}

	mtk_tvdac_put_clk(dev, target_clk);

	return 0;
}

static int mtk_tvdac_set_cvbso_swen_clock(struct device *dev, struct mtk_tvdac_context *pctx_tvdac)
{
	int ret = 0;
	u8 idx = 0;
	u16 count = 0;
	struct reg_render_cvbso_clk_info *clk_info = NULL;
	enum reg_render_cvbso_swen_type cvbso_swen_type = 0;

	if (!dev | !pctx_tvdac) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (pctx_tvdac->status == TRUE)
		cvbso_swen_type = REG_RENDER_CVBSO_SWEN_VIF_VIF;
	else
		cvbso_swen_type = REG_RENDER_CVBSO_SWEN_OFF;

	clk_info = (struct reg_render_cvbso_clk_info *)
			vzalloc(sizeof(struct reg_render_cvbso_clk_info) * ADCTBL_CLK_SWEN_NUM);
	if (clk_info == NULL) {
		DRM_ERROR("vzalloc fail\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = drv_hwreg_render_cvbso_get_clk_swen(
		cvbso_swen_type, ADCTBL_CLK_SWEN_NUM, clk_info, &count);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		ret = -EPERM;
		goto exit;
	}

	DRM_INFO("[DRM] CVBSO CLK SWEN nums from ADC table is: %u\n", count);
	for (idx = 0; idx < count; idx++) {
		DRM_INFO("[DRM] SWEN[%2u]: Name= %-40s, enable=%s\n",
			idx, clk_info[idx].clk_name, clk_info[idx].clk_data);

		ret = mtk_tvdac_toggle_swen(dev,
				clk_info[idx].clk_name,
				clk_info[idx].clk_data);
		if (ret < 0) {
			TVDAC_MSG_RETURN_CHECK(ret);
			DRM_ERROR("mtk_tvdac_toggle_swen fail\n");
			goto exit;
		}
	}


exit:
	if (clk_info != NULL)
		vfree(clk_info);

	return ret;
}

static int mtk_tvdac_enable_parent(
	struct device *dev,
	char *clk_name,
	char *parent_name)
{
	int ret = 0;
	struct clk *target_clk = NULL;
	struct clk *parent = NULL;

	if (dev == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (clk_name == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (parent_name == NULL) {
		TVDAC_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = mtk_tvdac_get_clk(dev, clk_name, &target_clk);
	if (ret < 0) {
		TVDAC_MSG_RETURN_CHECK(ret);
		goto out3;
	}

	ret = mtk_tvdac_get_clk(dev, parent_name, &parent);
	if (ret < 0) {
		TVDAC_MSG_RETURN_CHECK(ret);
		goto out2;
	}

	ret = clk_prepare_enable(target_clk);
	if (ret < 0) {
		TVDAC_MSG_RETURN_CHECK(ret);
		goto out1;
	}

	ret = clk_set_parent(target_clk, parent);
	if (ret < 0) {
		TVDAC_MSG_RETURN_CHECK(ret);
		goto out1;
	}
out1:
	mtk_tvdac_put_clk(dev, parent);
out2:
	mtk_tvdac_put_clk(dev, target_clk);
out3:
	return ret;
}


static int mtk_tvdac_set_cvbso_clock(struct device *dev, struct mtk_tvdac_context *pctx_tvdac)
{
	int ret = 0;
	u8 idx = 0;
	u16 count = 0;
	struct reg_render_cvbso_clk_info *clk_info = NULL;
	enum reg_render_cvbso_clk_setting_type cvbso_clk_type = 0;

	if (!dev || !pctx_tvdac) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (pctx_tvdac->status == TRUE)
		cvbso_clk_type = REG_RENDER_CVBSO_CLK_SETTING_VIF_VIF;
	else
		cvbso_clk_type = REG_RENDER_CVBSO_CLK_SETTING_OFF;

	clk_info = (struct reg_render_cvbso_clk_info *)
			vzalloc(sizeof(struct reg_render_cvbso_clk_info) * ADCTBL_CLK_SET_NUM);
	if (clk_info == NULL) {
		DRM_ERROR("vzalloc fail\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = drv_hwreg_render_cvbso_get_clk_setting(
		cvbso_clk_type, ADCTBL_CLK_SET_NUM, clk_info, &count);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		ret = -EPERM;
		goto exit;
	}

	DRM_INFO("[DRM] CVBSO CLK nums from ADC table = %u\n", count);
	for (idx = 0; idx < count; idx++) {
		DRM_INFO("[DRM] CLK[%2u]: Name= %-35s, parent=%s\n",
			idx, clk_info[idx].clk_name, clk_info[idx].clk_data);

		ret = mtk_tvdac_enable_parent(dev,
				clk_info[idx].clk_name,
				clk_info[idx].clk_data);
		if (ret < 0) {
			TVDAC_MSG_RETURN_CHECK(ret);
			DRM_ERROR("mtk_tvdac_enable_parent fail\n");
			goto exit;
		}
	}

exit:
	if (clk_info != NULL)
		vfree(clk_info);

	return ret;
}

static int mtk_tvdac_load_cvbso_table(enum reg_render_cvbso_type cvbso_type)
{
	int ret = 0;

	ret = drv_hwreg_render_cvbso_load_table(cvbso_type);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

static int mtk_tvdac_parse_dts(struct mtk_tvdac_context *pctx_tvdac)
{
	int ret = 0;
	u32 status = 1;
	struct device_node *np;
	const char *name;

	if (!pctx_tvdac) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_TV_KMS_COMPATIBLE_STR);

	if (np != NULL) {
		name = MTK_DRM_CVBSO_STATUS;
		ret = of_property_read_u32(np, name, &status);
		if (ret != 0x0) {
			DRM_ERROR("of_property_read_u32 failed, name = %s\r\n", name);
			return ret;
		}
	}
	pctx_tvdac->status = status;

	return ret;
}

int mtk_tv_drm_set_cvbso_mode_ioctl(
	struct drm_device *drm_dev,
	void *data,
	struct drm_file *file_priv)
{
	if (!drm_dev || !data || !file_priv) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	struct drm_mtk_tv_dac *args = (struct drm_mtk_tv_dac *)data;

	if (!args) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;
	struct mtk_drm_plane *mplane = NULL;
	struct mtk_tv_kms_context *pctx_kms = NULL;
	struct mtk_tvdac_context *pctx_tvdac = NULL;

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	// get mtk_tv_kms_context pointer
	drm_for_each_crtc(crtc, drm_dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		mplane = mtk_tv_crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO];
		pctx_kms = (struct mtk_tv_kms_context *)mplane->crtc_private;
		break;
	}
	pctx_tvdac = &pctx_kms->tvdac_priv;

	if (!pctx_tvdac) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	ret = mtk_tvdac_parse_dts(pctx_tvdac);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	pctx_tvdac->status = args->enable;

	ret = drv_hwreg_render_cvbso_set_enable(pctx_tvdac->status);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_tvdac_set_cvbso_swen_clock(pctx_tvdac->dev, pctx_tvdac);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}
#endif

	return ret;
}

int mtk_tvdac_cvbso_suspend(struct platform_device *pdev)
{
	if (!pdev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	int ret = 0;
	struct device *dev = &pdev->dev;

	ret = mtk_tvdac_deinit(dev);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	DRM_INFO("\033[1;34m [%s][%d] \033[m\n", __func__, __LINE__);

	return ret;
}

int mtk_tvdac_cvbso_resume(struct platform_device *pdev)
{
	if (!pdev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	int ret = 0;
	struct device *dev = &pdev->dev;

	ret = mtk_tvdac_init(dev);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	DRM_INFO("\033[1;34m [%s][%d] \033[m\n", __func__, __LINE__);

	return ret;
}

int mtk_tvdac_init(struct device *dev)
{
	if (!dev) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	enum reg_render_cvbso_type cvbso_type = MTK_DRM_CVBSO_INIT_IDX;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	if (!pctx) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	struct mtk_tvdac_context *pctx_tvdac = &pctx->tvdac_priv;

	if (!pctx_tvdac) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	pctx_tvdac->dev = dev;


	// parse dts
	ret = mtk_tvdac_parse_dts(pctx_tvdac);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_render_cvbso_set_hw_version(pctx->blend_hw_version);

	if (pctx_tvdac->status == TRUE) {
		// load adc table
		ret = mtk_tvdac_load_cvbso_table(cvbso_type);
		if (ret) {
			TVDAC_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	// load pws table
	ret = drv_hwreg_render_cvbso_set_enable(pctx_tvdac->status);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	// set CVBSO sw clock
	ret = mtk_tvdac_set_cvbso_swen_clock(pctx_tvdac->dev, pctx_tvdac);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	// set CVBSO clock
	ret = mtk_tvdac_set_cvbso_clock(pctx_tvdac->dev, pctx_tvdac);
	if (ret) {
		TVDAC_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

int mtk_tvdac_deinit(struct device *dev)
{
	if (!dev) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	if (!pctx) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	struct mtk_tvdac_context *pctx_tvdac = &pctx->tvdac_priv;

	if (!pctx_tvdac) {
		TVDAC_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	pctx_tvdac->status = FALSE;

	ret = mtk_tvdac_set_cvbso_swen_clock(dev, pctx_tvdac);

	if (ret) {
		DRM_ERROR("mtk_tvdac set default off table fail\n");
		return ret;
	}

	ret = mtk_tvdac_set_cvbso_clock(dev, pctx_tvdac);
	if (ret) {
		DRM_ERROR("mtk_tvdac set disable unprepare clk fail\n");
		return ret;
	}

	return ret;
}


