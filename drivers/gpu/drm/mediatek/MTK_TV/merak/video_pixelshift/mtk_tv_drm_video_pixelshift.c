// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include "mtk_tv_drm_video_pixelshift.h"
#include "../mtk_tv_drm_kms.h"
#include "hwreg_render_video_pixelshift.h"
#include "drv_scriptmgt.h"
#include "mtk_tv_drm_common.h"
#include "mtk_tv_drm_log.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_OLED
#define NEED_CHECK_PST_SUPPORT true		// set false to force set Pixel Shift

static void _mtk_video_set_pixelshift_ml_cmd(
	struct reg_info *reg,
	uint16_t RegCount,
	struct mtk_video_context *video_priv)
{
#if !NEED_CHECK_PST_SUPPORT
	struct sm_ml_add_info cmd_info;

	// Add register write command
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));
	cmd_info.cmd_cnt = RegCount;
	cmd_info.mem_index = video_priv->disp_ml.memindex;
	cmd_info.reg = (struct sm_reg *)reg;
	sm_ml_add_cmd(video_priv->disp_ml.fd, &cmd_info);
	video_priv->disp_ml.regcount = video_priv->disp_ml.regcount + RegCount;
#endif
}

static int _mtk_video_set_pixelshift_direction(struct mtk_tv_kms_context *pctx)
{
	bool bRIU = false;
	uint16_t RegCount = 0;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;
	struct mtk_video_context *video_priv = &pctx->video_priv;
	struct reg_info reg[video_priv->reg_num];
	struct hwregOut paramOut;

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	//BKA3A3_h7D[7:7] = video_h_direction
	//BKA3A3_h7D[6:0] = video_h_pixel
	drv_hwreg_render_pixelshift_set_video_h(
		&paramOut,
		bRIU,
		ps_priv->i8OLEDPixelshiftHDirection);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A3_h7D[15:15] = video_v_direction
	//BKA3A3_h7D[14:8] = video_v_pixel
	drv_hwreg_render_pixelshift_set_video_v(
		&paramOut,
		bRIU,
		ps_priv->i8OLEDPixelshiftVDirection);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	if (ps_priv->u8OLEDPixleshiftType == PIXELSHIFT_POST_JUSTSCAN) {
		ps_priv->bIsOLEDPixelshiftOsdAttached = true;
		crtc_props->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACH] = true;

		//BKA3A3_h7F[7:7] = vg_h_direction
		//BKA3A3_h7F[6:0] = vg_h_pixel
		drv_hwreg_render_pixelshift_set_vg_h(
			&paramOut,
			bRIU,
			ps_priv->i8OLEDPixelshiftHDirection);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;

		//BKA3A3_h7F[15:15] = vg_v_direction
		//BKA3A3_h7F[14:8]  = vg_v_pixel
		drv_hwreg_render_pixelshift_set_vg_v(
			&paramOut,
			bRIU,
			ps_priv->i8OLEDPixelshiftVDirection);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;
	}

	if (ps_priv->bIsOLEDPixelshiftOsdAttached) {
		//BKA3A3_h7E[7:7] = graph_h_direction
		//BKA3A3_h7E[6:0] = graph_h_pixel
		drv_hwreg_render_pixelshift_set_graph_h(
			&paramOut,
			bRIU,
			ps_priv->i8OLEDPixelshiftHDirection);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;

		//BKA3A3_h7E[15:15] = graph_v_direction
		//BKA3A3_h7E[14:8] = graph_v_pixel
		drv_hwreg_render_pixelshift_set_graph_v(
			&paramOut,
			bRIU,
			ps_priv->i8OLEDPixelshiftVDirection);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;
	}

	_mtk_video_set_pixelshift_ml_cmd(reg, RegCount, video_priv);

	return 0;
}

static int _mtk_video_set_pixelshift_osd_attach(struct mtk_tv_kms_context *pctx)
{
	bool bRIU = false;
	uint16_t RegCount = 0;
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;
	struct mtk_video_context *video_priv = &pctx->video_priv;
	struct reg_info reg[video_priv->reg_num];
	struct hwregOut paramOut;

	// Set OSD direction to the same as video or back to (0, 0)
	if (ps_priv->bIsOLEDPixelshiftOsdAttached) {
		_mtk_video_set_pixelshift_direction(pctx);
	} else {
		memset(&reg, 0, sizeof(reg));
		memset(&paramOut, 0, sizeof(struct hwregOut));
		paramOut.reg = reg;

		//BKA3A3_h7E[7:7] = graph_h_direction
		//BKA3A3_h7E[6:0] = graph_h_pixel
		drv_hwreg_render_pixelshift_set_graph_h(
			&paramOut,
			bRIU,
			0);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;

		//BKA3A3_h7E[15:15] = graph_v_direction
		//BKA3A3_h7E[14:8] = graph_v_pixel
		drv_hwreg_render_pixelshift_set_graph_v(
			&paramOut,
			bRIU,
			0);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;
	}

	_mtk_video_set_pixelshift_ml_cmd(reg, RegCount, video_priv);

	return 0;
}

static int _mtk_video_set_pixelshift_type(struct mtk_tv_kms_context *pctx)
{
	bool bRIU = false;
	uint16_t RegCount = 0;
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;
	struct mtk_video_context *video_priv = &pctx->video_priv;
	struct reg_info reg[video_priv->reg_num];
	struct hwregOut paramOut;

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	//BKA3A3_h76[3:3] = hw_cal_v_v_end_en
	//BKA3A3_h76[11:11] = hw_cal_g_v_end_en
	drv_hwreg_render_pixelshift_set_type(
		&paramOut,
		bRIU,
		(enum video_pixelshift_type)ps_priv->u8OLEDPixleshiftType);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	_mtk_video_set_pixelshift_ml_cmd(reg, RegCount, video_priv);

	_mtk_video_set_pixelshift_direction(pctx);

	return 0;
}

static int _mtk_video_set_pixelshift_enable(struct mtk_tv_kms_context *pctx)
{
	bool bRIU = false;
	uint16_t RegCount = 0;
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;
	struct mtk_video_context *video_priv = &pctx->video_priv;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	struct reg_info reg[video_priv->reg_num];
	struct hwregOut paramOut;

	crtc_props->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_H] = 0;
	crtc_props->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_V] = 0;
	ps_priv->i8OLEDPixelshiftHDirection = 0;
	ps_priv->i8OLEDPixelshiftVDirection = 0;
	_mtk_video_set_pixelshift_direction(pctx);

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	//BKA3A0_h70[5:4] = v0_frame_color_de_sel
	//BKA3A0_h70[7:7] = v0_en
	//BKA3A0_h71[7:7] = g0_en
	//BKA3A0_h71[15:15] = g1_en
	drv_hwreg_render_pixelshift_set_enable(
		&paramOut,
		bRIU,
		ps_priv->bIsOLEDPixelshiftEnable);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	_mtk_video_set_pixelshift_ml_cmd(reg, RegCount, video_priv);

	return 0;
}

/* ------- above is static function -------*/
int mtk_video_pixelshift_init(struct device *dev)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;

	// start pixelshift with disable even if SoC support
	ps_priv->bIsOLEDPixelshiftEnable = false;
	ps_priv->bIsOLEDPixelshiftOsdAttached = true;
	ps_priv->u8OLEDPixleshiftType = PIXELSHIFT_POST_JUSTSCAN;
	ps_priv->i8OLEDPixelshiftHDirection = 0;
	ps_priv->i8OLEDPixelshiftVDirection = 0;

	if (pctx->drmcap_dev.u8OLEDPixelshiftSupport)
		_mtk_video_set_pixelshift_enable(pctx);
	DRM_INFO("[%s][%d] Pixel shift init done\n", __func__, __LINE__);

	return 0;
}

int mtk_video_pixelshift_suspend(struct platform_device *pdev)
{
	if (!pdev) {
		DRM_INFO("[%s, %d]: platform_device is NULL.\n", __func__, __LINE__);
		return -ENODEV;
	}
	struct device *dev = &pdev->dev;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;

	if (pctx->drmcap_dev.u8OLEDPixelshiftSupport) {
		DRM_INFO("[%s][%d] enable, type, attach, h, v = %d, %d, %d, %d, %d\n",
			__func__, __LINE__,
			ps_priv->bIsOLEDPixelshiftEnable,
			ps_priv->u8OLEDPixleshiftType,
			ps_priv->bIsOLEDPixelshiftOsdAttached,
			ps_priv->i8OLEDPixelshiftHDirection,
			ps_priv->i8OLEDPixelshiftVDirection);
	}

	return 0;
}

int mtk_video_pixelshift_resume(struct platform_device *pdev)
{
	if (!pdev) {
		DRM_INFO("[%s, %d]: platform_device is NULL.\n", __func__, __LINE__);
		return -ENODEV;
	}
	struct device *dev = &pdev->dev;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;

	if (pctx->drmcap_dev.u8OLEDPixelshiftSupport) {
		_mtk_video_set_pixelshift_enable(pctx);
		DRM_INFO("[%s][%d] enable, type, attach, h, v = %d, %d, %d, %d, %d\n",
			__func__, __LINE__,
			ps_priv->bIsOLEDPixelshiftEnable,
			ps_priv->u8OLEDPixleshiftType,
			ps_priv->bIsOLEDPixelshiftOsdAttached,
			ps_priv->i8OLEDPixelshiftHDirection,
			ps_priv->i8OLEDPixelshiftVDirection);
	}

	return 0;
}

int mtk_video_pixelshift_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val,
	int prop_index)
{
	if (!crtc) {
		DRM_INFO("[%s][%d] NULL pointer\n", __func__, __LINE__);
		return 0;
	}

	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;
	int hv_direction = 0;

	// Error handling
	if ((prop_index < E_EXT_CRTC_PROP_PIXELSHIFT_ENABLE) ||
		(prop_index > E_EXT_CRTC_PROP_PIXELSHIFT_V)) {
		DRM_INFO("[%s][%d] Property %d invalid\n", __func__, __LINE__, prop_index);
		return 0;
	}
#if NEED_CHECK_PST_SUPPORT
	if (!pctx->drmcap_dev.u8OLEDPixelshiftSupport) {
		DRM_INFO("[%s][%d] OLED pixel shift not support\n", __func__, __LINE__);
		crtc_props->prop_update[prop_index] = 0;
		return 0;
	}
#endif

	// Property is set in "mtk_tv_kms_atomic_set_crtc_property", not here
	// The setting (menuload) is triggered by "mtk_video_pixelshift_atomic_crtc_flush", not here
	// Here only check the update value.
	switch (prop_index) {
	case E_EXT_CRTC_PROP_PIXELSHIFT_ENABLE:
		DRM_INFO("[%s][%d] Set pixelshift enable = %td\n",
			__func__, __LINE__, (ptrdiff_t)crtc_props->prop_val[prop_index]);
		crtc_props->prop_val[prop_index] = crtc_props->prop_val[prop_index] > 0 ? 1 : 0;
		if (ps_priv->bIsOLEDPixelshiftEnable == crtc_props->prop_val[prop_index])
			crtc_props->prop_update[prop_index] = 0;
		else
			ps_priv->bIsOLEDPixelshiftEnable = crtc_props->prop_val[prop_index];
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACH:
		DRM_INFO("[%s][%d] Set pixelshift osd attached= %td\n",
			__func__, __LINE__, (ptrdiff_t)crtc_props->prop_val[prop_index]);
		crtc_props->prop_val[prop_index] = crtc_props->prop_val[prop_index] > 0 ? 1 : 0;
		// Post-JustScan cannot detach OSD
		if (ps_priv->u8OLEDPixleshiftType == PIXELSHIFT_POST_JUSTSCAN)
			crtc_props->prop_val[prop_index] = 1;
		if (ps_priv->bIsOLEDPixelshiftOsdAttached == crtc_props->prop_val[prop_index])
			crtc_props->prop_update[prop_index] = 0;
		else
			ps_priv->bIsOLEDPixelshiftOsdAttached = crtc_props->prop_val[prop_index];
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_TYPE:
		DRM_INFO("[%s][%d] Set pixelshift type= %td\n",
			__func__, __LINE__, (ptrdiff_t)crtc_props->prop_val[prop_index]);
		if (crtc_props->prop_val[prop_index] > PIXELSHIFT_POST_JUSTSCAN)
			crtc_props->prop_val[prop_index] = PIXELSHIFT_POST_JUSTSCAN;
		if (ps_priv->u8OLEDPixleshiftType == crtc_props->prop_val[prop_index])
			crtc_props->prop_update[prop_index] = 0;
		else
			ps_priv->u8OLEDPixleshiftType = crtc_props->prop_val[prop_index];
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_H:
		DRM_INFO("[%s][%d] Set H direction = %td, min~max = %td~%td\n",
		__func__, __LINE__, (ptrdiff_t)crtc_props->prop_val[prop_index],
		(ptrdiff_t)ps_priv->i8OLEDPixelshiftHRangeMin,
		(ptrdiff_t)ps_priv->i8OLEDPixelshiftHRangeMax);
		// Check range
		if (crtc_props->prop_val[prop_index] < ps_priv->i8OLEDPixelshiftHRangeMin)
			crtc_props->prop_val[prop_index] = ps_priv->i8OLEDPixelshiftHRangeMin;
		else if (crtc_props->prop_val[prop_index] > ps_priv->i8OLEDPixelshiftHRangeMax)
			crtc_props->prop_val[prop_index] = ps_priv->i8OLEDPixelshiftHRangeMax;
		hv_direction = (int)(crtc_props->prop_val[prop_index]);
		// Update value
		if (ps_priv->i8OLEDPixelshiftHDirection == hv_direction)
			crtc_props->prop_update[prop_index] = 0;
		else {
			ps_priv->i8OLEDPixelshiftHDirection =
				(int8_t)crtc_props->prop_val[prop_index];
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_V:
		DRM_INFO("[%s][%d] Set V direction = %td, min~max = %td~%td\n",
		__func__, __LINE__, (ptrdiff_t)crtc_props->prop_val[prop_index],
		(ptrdiff_t)ps_priv->i8OLEDPixelshiftVRangeMin,
		(ptrdiff_t)ps_priv->i8OLEDPixelshiftVRangeMax);
		// Check range
		if (crtc_props->prop_val[prop_index] < ps_priv->i8OLEDPixelshiftVRangeMin)
			crtc_props->prop_val[prop_index] = ps_priv->i8OLEDPixelshiftVRangeMin;
		else if (crtc_props->prop_val[prop_index] > ps_priv->i8OLEDPixelshiftVRangeMax)
			crtc_props->prop_val[prop_index] = ps_priv->i8OLEDPixelshiftVRangeMax;
		hv_direction = (int)(crtc_props->prop_val[prop_index]);
		// Update value
		if (ps_priv->i8OLEDPixelshiftVDirection == hv_direction)
			crtc_props->prop_update[prop_index] = 0;
		else {
			ps_priv->i8OLEDPixelshiftVDirection =
				(int8_t)crtc_props->prop_val[prop_index];
		}
		break;
	default:
		DRM_INFO("[%s][%d] invalid property!!\n",
			__func__, __LINE__);
		break;
	}

	return 0;
}
int mtk_video_pixelshift_atomic_get_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val,
	int prop_index)
{
	int ret = -EINVAL;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	//struct drm_property_blob *property_blob = NULL;
	int64_t int64Value = 0;

	// Error handling
#if NEED_CHECK_PST_SUPPORT
	if (!pctx->drmcap_dev.u8OLEDPixelshiftSupport) {
		DRM_INFO("[%s][%d] OLED pixel shift not support\n", __func__, __LINE__);
		return 0;
	}
#endif
	if ((prop_index < E_EXT_CRTC_PROP_PIXELSHIFT_ENABLE) ||
		(prop_index > E_EXT_CRTC_PROP_PIXELSHIFT_V)) {
		DRM_INFO("[%s][%d] Property %d invalid\n", __func__, __LINE__, prop_index);
		return 0;
	}

	switch (prop_index) {
	case E_EXT_CRTC_PROP_PIXELSHIFT_ENABLE:
		if (val != NULL) {
			*val = crtc_props->prop_val[prop_index];
			DRM_INFO("[%s][%d] get pixel shift enable = %td\n",
				__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACH:
		if (val != NULL) {
			*val = crtc_props->prop_val[prop_index];
			DRM_INFO("[%s][%d] get pixel shift attach status = %td\n",
				__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_TYPE:
		if (val != NULL) {
			*val = crtc_props->prop_val[prop_index];
			DRM_INFO("[%s][%d] get pixel shift type = %td\n",
				__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_H:
		if (val != NULL) {
			*val = crtc_props->prop_val[prop_index];
			DRM_INFO("[%s][%d] get pixel shift h = %td\n",
				__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_V:
		if (val != NULL) {
			*val = crtc_props->prop_val[prop_index];
			DRM_INFO("[%s][%d] get pixel shift v = %td\n",
				__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	default:
		DRM_INFO("[DRM][VIDEO][%s][%d] Pixel shift not support getting property %d\n",
			__func__, __LINE__, prop_index);
		break;
	}

	return 0;
}
void mtk_video_pixelshift_atomic_crtc_flush(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state)
{
	if (!crtc) {
		DRM_INFO("[%s][%d] NULL pointer\n", __func__, __LINE__);
		return;
	}

	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	struct mtk_pixelshift_context *ps_priv = &pctx->pixelshift_priv;
	int prop_index = 0;

	// Error handling
#if NEED_CHECK_PST_SUPPORT
	if (!pctx->drmcap_dev.u8OLEDPixelshiftSupport)
		goto CLEAN_FLAG;
#endif
	DRM_INFO("[%s][%d] Enter\n", __func__, __LINE__);
	// Set register value
	if (crtc_props->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_TYPE])
		_mtk_video_set_pixelshift_type(pctx);
	else if (crtc_props->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_H] ||
		crtc_props->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_V])
		_mtk_video_set_pixelshift_direction(pctx);
	else if (crtc_props->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACH])
		_mtk_video_set_pixelshift_osd_attach(pctx);
	if (crtc_props->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_ENABLE])
		_mtk_video_set_pixelshift_enable(pctx);

CLEAN_FLAG:
	// Clean update flag
	for (prop_index = E_EXT_CRTC_PROP_PIXELSHIFT_ENABLE;
	     prop_index <= E_EXT_CRTC_PROP_PIXELSHIFT_V; prop_index++) {
		crtc_props->prop_update[prop_index] = 0;
	}
}

