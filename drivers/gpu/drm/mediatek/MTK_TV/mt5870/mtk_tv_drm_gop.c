// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <drm/mtk_tv_drm.h>

#include "mtk_tv_drm_plane.h"
#include "mtk_tv_drm_gop_wrapper.h"
#include "mtk_tv_drm_drv.h"
#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_gop.h"
#include "mtk_tv_drm_encoder.h"
#include "mtk_tv_drm_gop_plane.h"
#include "mtk_tv_drm_kms.h"

#define MTK_DRM_DTS_GOPNUM_TAG     "GRAPHIC_GOP_NUM"
#define MTK_DRM_DTS_HMIRROR_TAG     "GRAPHIC_HMIRROR"
#define MTK_DRM_DTS_VMIRROR_TAG     "GRAPHIC_VMIRROR"
#define MTK_DRM_DTS_HSTRETCH_INIT_MODE_TAG "GRAPHIC_HSTRETCH_INIT_MODE"
#define MTK_DRM_DTS_VSTRETCH_INIT_MODE_TAG "GRAPHIC_VSTRETCH_INIT_MODE"
#define MTK_DRM_DTS_HUE_TAG     "GRAPHIC_PLANE_HUE"
#define MTK_DRM_DTS_SATURATION_TAG     "GRAPHIC_PLANE_SATURATION"
#define MTK_DRM_DTS_CONTRAST_TAG     "GRAPHIC_PLANE_CONTRAST"
#define MTK_DRM_DTS_BRIGHTNESS_TAG     "GRAPHIC_PLANE_BRIGHTNESS"
#define MTK_DRM_DTS_RGAIN_TAG     "GRAPHIC_PLANE_R_GAIN"
#define MTK_DRM_DTS_GGAIN_TAG     "GRAPHIC_PLANE_G_GAIN"
#define MTK_DRM_DTS_BGAIN_TAG     "GRAPHIC_PLANE_B_GAIN"

/* panel info */
#define MTK_DRM_PANEL_COLOR_COMPATIBLE_STR "panel-color-info"
#define MTK_DRM_DTS_PANEL_COLOR_SAPCE_TAG     "COLOR_SPACE"
#define MTK_DRM_DTS_PANEL_COLOR_FORAMT_TAG     "COLOR_FORAMT"
#define MTK_DRM_DTS_PANEL_COLOR_RANGE_TAG     "COLOR_RANGE"
#define MTK_DRM_DTS_PANEL_CURLUM_TAG     "HDRPanelNits"
#define MTK_DRM_DTS_PANEL_PRIMARIES_RX_TAG     "Rx"
#define MTK_DRM_DTS_PANEL_PRIMARIES_GX_TAG     "Gx"
#define MTK_DRM_DTS_PANEL_PRIMARIES_BX_TAG     "Bx"
#define MTK_DRM_DTS_PANEL_PRIMARIES_RY_TAG     "Ry"
#define MTK_DRM_DTS_PANEL_PRIMARIES_GY_TAG     "Gy"
#define MTK_DRM_DTS_PANEL_PRIMARIES_BY_TAG     "By"
#define MTK_DRM_DTS_PANEL_PRIMARIES_WHITE_X_TAG     "Wx"
#define MTK_DRM_DTS_PANEL_PRIMARIES_WHITE_Y_TAG     "Wy"

#define MTK_DRM_PANEL_INFO_STR "panel-info"
#define MTK_DRM_DTS_PANELW_TAG     "m_wPanelWidth"
#define MTK_DRM_DTS_PANELH_TAG     "m_wPanelHeight"
#define MTK_DRM_DTS_PANELHSTR_TAG     "m_wPanelHStart"

#define MTK_DRM_DTS_GOP_CAPABILITY_TAG     "capability"

#define MTK_DRM_GOP_ATTR_MODE     (0600)

static uint32_t _gu32CapsAFBC[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_AFBC,
	MTK_GOP_CAPS_OFFSET_GOP_AFBC_COMMON};
static uint32_t _gu32CapsHMirror[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_HMIRROR,
	MTK_GOP_CAPS_OFFSET_GOP_HMIRROR_COMMON};
static uint32_t _gu32CapsVMirror[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_VMIRROR,
	MTK_GOP_CAPS_OFFSET_GOP_VMIRROR_COMMON};
static uint32_t _gu32CapsTransColor[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_TRANSCOLOR,
	MTK_GOP_CAPS_OFFSET_GOP_TRANSCOLOR_COMMON};
static uint32_t _gu32CapsScroll[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_SCROLL,
	MTK_GOP_CAPS_OFFSET_GOP_SCROLL_COMMON};
static uint32_t _gu32CapsBlendXcIp[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_BLEND_XCIP,
	MTK_GOP_CAPS_OFFSET_GOP_BLEND_XCIP_COMMON};
static uint32_t _gu32CapsCSC[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_CSC,
	MTK_GOP_CAPS_OFFSET_GOP_CSC_COMMON};
static uint32_t _gu32CapsHDRMode[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_HDR_MODE_MSK,
	MTK_GOP_CAPS_OFFSET_GOP_HDR_MODE_MSK_COMMON};
static uint32_t _gu32CapsHDRModeSft[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_HDR_MODE_SFT,
	MTK_GOP_CAPS_OFFSET_GOP_HDR_MODE_SFT_COMMON};
static uint32_t _gu32CapsHScaleMode[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_HSCALING_MODE_MSK,
	MTK_GOP_CAPS_OFFSET_GOP_HSCALING_MODE_MSK_COMMON};
static uint32_t _gu32CapsHScaleSft[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_HSCALING_MODE_SFT,
	MTK_GOP_CAPS_OFFSET_GOP_HSCALING_MODE_SFT_COMMON};
static uint32_t _gu32CapsVScaleMode[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_VSCALING_MODE_MSK,
	MTK_GOP_CAPS_OFFSET_GOP_VSCALING_MODE_MSK_COMMON};
static uint32_t _gu32CapsVScaleSft[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_VSCALING_MODE_SFT,
	MTK_GOP_CAPS_OFFSET_GOP_VSCALING_MODE_SFT_COMMON};
static uint32_t _gu32CapsH10V6Switch[2] = {
	MTK_GOP_CAPS_OFFSET_GOP0_H10V6_SWITCH,
	MTK_GOP_CAPS_OFFSET_GOP_H10V6_SWITCH_COMMON};

static UTP_EN_GOP_CS _convertColorSpace(enum drm_color_encoding color_encoding)
{
	UTP_EN_GOP_CS utpWrapCS = UTP_E_GOP_COLOR_RGB_BT709;

	switch (color_encoding) {
	case DRM_COLOR_YCBCR_BT601:
		utpWrapCS = UTP_E_GOP_COLOR_YCBCR_BT601;
		break;
	case DRM_COLOR_YCBCR_BT709:
		utpWrapCS = UTP_E_GOP_COLOR_YCBCR_BT709;
		break;
	case DRM_COLOR_YCBCR_BT2020:
		utpWrapCS = UTP_E_GOP_COLOR_YCBCR_BT2020;
		break;
	case DRM_COLOR_RGB_BT601:
		utpWrapCS = UTP_E_GOP_COLOR_RGB_BT601;
		break;
	case DRM_COLOR_RGB_BT709:
		utpWrapCS = UTP_E_GOP_COLOR_RGB_BT709;
		break;
	case DRM_COLOR_RGB_BT2020:
		utpWrapCS = UTP_E_GOP_COLOR_RGB_BT2020;
		break;
	case DRM_COLOR_RGB_SRGB:
		utpWrapCS = UTP_E_GOP_COLOR_SRGB;
		break;
	case DRM_COLOR_RGB_DCIP3_D65:
		utpWrapCS = UTP_E_GOP_COLOR_RGB_DCIP3_D65;
		break;
	default:
		DRM_INFO("[GOP][%s, %d]: not support color space %d\n", __func__, __LINE__,
			 color_encoding);
		break;
	}

	return utpWrapCS;
}

static UTP_EN_GOP_DATA_FMT _convertColorFmt2CFD(uint32_t u32DrmFmt)
{
	UTP_EN_GOP_DATA_FMT utpWrapCFmt = UTP_E_GOP_COLOR_FORMAT_RGB;

	switch (u32DrmFmt) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_XBGR8888:
		utpWrapCFmt = UTP_E_GOP_COLOR_FORMAT_RGB;
		break;
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_YUV422:
		utpWrapCFmt = UTP_E_GOP_COLOR_FORMAT_YUV422;
		break;
	default:
		DRM_INFO("[GOP][%s, %d]: not support color format %d\n", __func__, __LINE__,
			 u32DrmFmt);
		break;
	}

	return utpWrapCFmt;
}

static UTP_EN_GOP_CR _convertColorRange(enum drm_color_range color_range)
{
	UTP_EN_GOP_CR utpWrapCR = UTP_E_GOP_COLOR_RGB_FULL_RANGE;

	switch (color_range) {
	case DRM_COLOR_YCBCR_LIMITED_RANGE:
		utpWrapCR = UTP_E_GOP_COLOR_YCBCR_LIMITED_RANGE;
		break;
	case DRM_COLOR_YCBCR_FULL_RANGE:
		utpWrapCR = UTP_E_GOP_COLOR_YCBCR_FULL_RANGE;
		break;
	case DRM_COLOR_RGB_LIMITED_RANGE:
		utpWrapCR = UTP_E_GOP_COLOR_RGB_LIMITED_RANGE;
		break;
	case DRM_COLOR_RGB_FULL_RANGE:
		utpWrapCR = UTP_E_GOP_COLOR_RGB_FULL_RANGE;
		break;
	default:
		DRM_INFO("[GOP][%s, %d]: not support color range %d\n", __func__, __LINE__,
			 color_range);
		break;
	}

	return utpWrapCR;
}

static UTP_EN_GOP_COLOR_TYPE _convertColorFmt(const u32 u32DrmFmt)
{
	UTP_EN_GOP_COLOR_TYPE utpWrapfmt;

	switch (u32DrmFmt) {
	case DRM_FORMAT_XRGB8888:
		utpWrapfmt = UTP_E_GOP_COLOR_XRGB8888;
		break;
	case DRM_FORMAT_ABGR8888:
		utpWrapfmt = UTP_E_GOP_COLOR_ABGR8888;
		break;
	case DRM_FORMAT_RGBA8888:
		utpWrapfmt = UTP_E_GOP_COLOR_RGBA8888;
		break;
	case DRM_FORMAT_BGRA8888:
		utpWrapfmt = UTP_E_GOP_COLOR_BGRA8888;
		break;
	case DRM_FORMAT_ARGB8888:
		utpWrapfmt = UTP_E_GOP_COLOR_ARGB8888;
		break;
	case DRM_FORMAT_RGB565:
		utpWrapfmt = UTP_E_GOP_COLOR_RGB565;
		break;
	case DRM_FORMAT_ARGB4444:
		utpWrapfmt = UTP_E_GOP_COLOR_ARGB4444;
		break;
	case DRM_FORMAT_ARGB1555:
		utpWrapfmt = UTP_E_GOP_COLOR_ARGB1555;
		break;
	case DRM_FORMAT_YUV422:
		utpWrapfmt = UTP_E_GOP_COLOR_YUV422;
		break;
	case DRM_FORMAT_RGBA5551:
		utpWrapfmt = UTP_E_GOP_COLOR_RGBA5551;
		break;
	case DRM_FORMAT_RGBA4444:
		utpWrapfmt = UTP_E_GOP_COLOR_RGBA4444;
		break;
	case DRM_FORMAT_BGR565:
		utpWrapfmt = UTP_E_GOP_COLOR_BGR565;
		break;
	case DRM_FORMAT_ABGR4444:
		utpWrapfmt = UTP_E_GOP_COLOR_ABGR4444;
		break;
	case DRM_FORMAT_ABGR1555:
		utpWrapfmt = UTP_E_GOP_COLOR_ABGR1555;
		break;
	case DRM_FORMAT_BGRA5551:
		utpWrapfmt = UTP_E_GOP_COLOR_BGRA5551;
		break;
	case DRM_FORMAT_BGRA4444:
		utpWrapfmt = UTP_E_GOP_COLOR_BGRA4444;
		break;
	case DRM_FORMAT_XBGR8888:
		utpWrapfmt = UTP_E_GOP_COLOR_XBGR8888;
		break;
	case DRM_FORMAT_UYVY:
		utpWrapfmt = UTP_E_GOP_COLOR_UYVY;
		break;
	default:
		utpWrapfmt = UTP_E_GOP_COLOR_ARGB8888;
		break;
	}

	return utpWrapfmt;
}

static UTP_EN_GOP_STRETCH_HMODE _convertHStretchMode(const u32 u32HStretchMode)
{
	UTP_EN_GOP_STRETCH_HMODE mode;

	switch (u32HStretchMode) {
	case MTK_DRM_HSTRETCH_DUPLICATE:
		mode = UTP_E_GOP_HSTRCH_DUPLICATE;
		break;
	case MTK_DRM_HSTRETCH_6TAP8PHASE:
		mode = UTP_E_GOP_HSTRCH_6TAPE_LINEAR;
		break;
	case MTK_DRM_HSTRETCH_4TAP256PHASE:
		mode = UTP_E_GOP_HSTRCH_NEW4TAP;
		break;
	default:
		mode = UTP_E_GOP_HSTRCH_INVALID;
		break;
	}

	return mode;
}

static UTP_EN_GOP_STRETCH_HMODE _convertVStretchMode(const u32 u32VStretchMode)
{
	UTP_EN_GOP_STRETCH_VMODE mode;

	switch (u32VStretchMode) {
	case MTK_DRM_VSTRETCH_2TAP16PHASE:
		mode = UTP_E_GOP_VSTRCH_2TAP;
		break;
	case MTK_DRM_VSTRETCH_DUPLICATE:
		mode = UTP_E_GOP_VSTRCH_DUPLICATE;
		break;
	case MTK_DRM_VSTRETCH_BILINEAR:
		mode = UTP_E_GOP_VSTRCH_BILINEAR;
		break;
	case MTK_DRM_VSTRETCH_4TAP:
		mode = UTP_E_GOP_VSTRCH_4TAP;
		break;
	default:
		mode = UTP_E_GOP_VSTRCH_INVALID;
		break;
	}

	return mode;
}

static void _check_ColorSpacebyFbFormat(struct mtk_plane_state *state)
{
	switch (state->base.fb->format->format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_XBGR8888:
		if ((state->base.color_encoding == DRM_COLOR_YCBCR_BT601)
		    || (state->base.color_encoding == DRM_COLOR_YCBCR_BT709)
		    || (state->base.color_encoding == DRM_COLOR_YCBCR_BT2020)) {
			DRM_INFO
			    ("[GOP][%s, %d]color encoding %d is not match DRM fb format\n",
			     __func__, __LINE__, state->base.color_encoding);
			state->base.color_encoding = DRM_COLOR_RGB_BT709;
		}
		break;
	case DRM_FORMAT_YUV422:
		if ((state->base.color_encoding == DRM_COLOR_RGB_BT601)
		    || (state->base.color_encoding == DRM_COLOR_RGB_BT709)
		    || (state->base.color_encoding == DRM_COLOR_RGB_BT2020)
		    || (state->base.color_encoding == DRM_COLOR_RGB_SRGB)
		    || (state->base.color_encoding == DRM_COLOR_RGB_DCIP3_D65)) {
			DRM_INFO
			    ("[GOP][%s, %d]color encoding %d is not match DRM fb format\n",
			     __func__, __LINE__, state->base.color_encoding);
			state->base.color_encoding = DRM_COLOR_YCBCR_BT709;
		}
		break;
	default:
		break;
	}
}

static void _check_ColorRangebyFbFormat(struct mtk_plane_state *state)
{
	switch (state->base.fb->format->format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_XBGR8888:
		if ((state->base.color_range == DRM_COLOR_YCBCR_LIMITED_RANGE)
		    || (state->base.color_range == DRM_COLOR_YCBCR_FULL_RANGE)) {
			DRM_INFO
			    ("[GOP][%s,%d]color range %d is not match DRM fb format\n",
			     __func__, __LINE__, state->base.color_range);
			state->base.color_range = DRM_COLOR_RGB_FULL_RANGE;
		}
		break;
	case DRM_FORMAT_YUV422:
		if ((state->base.color_range == DRM_COLOR_RGB_LIMITED_RANGE)
		    ||
		    (state->base.color_range == DRM_COLOR_RGB_FULL_RANGE)) {
			DRM_INFO
			    ("[GOP][%s,%d]color range %d is not match DRM fb format\n",
			     __func__, __LINE__, state->base.color_range);
			state->base.color_range = DRM_COLOR_YCBCR_FULL_RANGE;
		}
		break;
	default:
		break;
	}
}

static bool _bCSC_tuning_state_change(unsigned int plane_Idx, struct mtk_gop_context *pctx_gop,
				      struct mtk_plane_state *state)
{
	UTP_GOP_CSC_PARAM stCSCParam;

	memset(&stCSCParam, 0, sizeof(UTP_GOP_CSC_PARAM));

	/*Hue, Saturatio and Contrast */
	stCSCParam.u16Hue = (pctx_gop->gop_plane_properties + plane_Idx)->prop_val[E_GOP_PROP_HUE];
	stCSCParam.u16Saturation =
	    (pctx_gop->gop_plane_properties + plane_Idx)->prop_val[E_GOP_PROP_SATURATION];
	stCSCParam.u16Contrast =
	    (pctx_gop->gop_plane_properties + plane_Idx)->prop_val[E_GOP_PROP_CONTRAST];
	stCSCParam.u16Brightness =
	    (pctx_gop->gop_plane_properties + plane_Idx)->prop_val[E_GOP_PROP_BRIGHTNESS];
	stCSCParam.u16RGBGain[0] =
	    (pctx_gop->gop_plane_properties + plane_Idx)->prop_val[E_GOP_PROP_RGAIN];
	stCSCParam.u16RGBGain[1] =
	    (pctx_gop->gop_plane_properties + plane_Idx)->prop_val[E_GOP_PROP_GGAIN];
	stCSCParam.u16RGBGain[2] =
	    (pctx_gop->gop_plane_properties + plane_Idx)->prop_val[E_GOP_PROP_BGAIN];

	/*Checking input color encoding and format is match or not */
	_check_ColorSpacebyFbFormat(state);

	/*Checking input color range and format is match or not */
	_check_ColorRangebyFbFormat(state);

	/*Input color space, color range and color format */
	stCSCParam.eInputCS = _convertColorSpace(state->base.color_encoding);
	stCSCParam.eInputCFMT = _convertColorFmt2CFD(state->base.fb->format->format);
	stCSCParam.eInputCR = _convertColorRange(state->base.color_range);

	/*Out color space, color range and color format */
	stCSCParam.eOutputCS = (UTP_EN_GOP_CS) pctx_gop->out_plane_color_encoding;
	stCSCParam.eOutputCFMT = (UTP_EN_GOP_DATA_FMT) pctx_gop->out_plane_color_format;
	stCSCParam.eOutputCR = (UTP_EN_GOP_CR) pctx_gop->out_plane_color_range;

	/*Checking CSC state is change or not */
	if (memcmp
	    (&stCSCParam, (pctx_gop->pCurrent_CSCParam + plane_Idx), sizeof(UTP_GOP_CSC_PARAM))) {
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->u16Hue = stCSCParam.u16Hue;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->u16Saturation = stCSCParam.u16Saturation;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->u16Contrast = stCSCParam.u16Contrast;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->u16Brightness = stCSCParam.u16Brightness;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->u16RGBGain[0] = stCSCParam.u16RGBGain[0];
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->u16RGBGain[1] = stCSCParam.u16RGBGain[1];
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->u16RGBGain[2] = stCSCParam.u16RGBGain[2];
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->eInputCS = stCSCParam.eInputCS;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->eInputCFMT = stCSCParam.eInputCFMT;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->eInputCR = stCSCParam.eInputCR;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->eOutputCS = stCSCParam.eOutputCS;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->eOutputCFMT = stCSCParam.eOutputCFMT;
		(pctx_gop->pCurrent_CSCParam + plane_Idx)->eOutputCR = stCSCParam.eOutputCR;
		return true;
	} else {
		return false;
	}

}

static ssize_t mtk_drm_capability_show_GOP_Num(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n", pctx_gop->gop_plane_capability.u8GOPNum);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP_VOP_PATH(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVopPathSel);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_AFBC(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportAFBC[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_HMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHMirror[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_VMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVMirror[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_TransColor(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportTransColor[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_Scroll(struct device *dev,
						   struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportScroll[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_BlendXcIp(struct device *dev,
						      struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportBlendXcIP[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_CSC(struct device *dev, struct device_attribute *attr,
						char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportCSC[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_HDRMode(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHDRMode[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_HScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHScalingMode[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_VScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVScalingMode[0]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP0_H10V6_Switch(struct device *dev,
							 struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportH10V6Switch[0]);

	return ssize;
}

static ssize_t mtk_drm_query_show_GOP_CRC(struct device *dev,
							 struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;
	__u32 CRCvalue = 0;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%x\n", CRCvalue);

	return ssize;
}

static ssize_t mtk_drm_capability_store_GOP_Num(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP_VOP_PATH(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_AFBC(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_HMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_VMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_TransColor(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_Scroll(struct device *dev,
						    struct device_attribute *attr, const char *buf,
						    size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_BlendXcIp(struct device *dev,
						       struct device_attribute *attr,
						       const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_CSC(struct device *dev, struct device_attribute *attr,
						 const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_HDRMode(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_HScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_VScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP0_H10V6_Switch(struct device *dev,
							  struct device_attribute *attr,
							  const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_query_store_GOP_CRC(struct device *dev,
							  struct device_attribute *attr,
							  const char *buf, size_t count)
{
	return count;
}


static struct device_attribute mtk_drm_gop0_attr[] = {
	__ATTR(drm_capability_GOP_NUM, MTK_DRM_GOP_ATTR_MODE, mtk_drm_capability_show_GOP_Num,
	       mtk_drm_capability_store_GOP_Num),
	__ATTR(drm_capability_GOP_SUPPORT_VOP_PATH, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP_VOP_PATH, mtk_drm_capability_store_GOP_VOP_PATH),
	__ATTR(drm_capability_GOP0_SUPPORT_AFBC, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_AFBC, mtk_drm_capability_store_GOP0_AFBC),
	__ATTR(drm_capability_GOP0_SUPPORT_HMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_HMirror, mtk_drm_capability_store_GOP0_HMirror),
	__ATTR(drm_capability_GOP0_SUPPORT_VMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_VMirror, mtk_drm_capability_store_GOP0_VMirror),
	__ATTR(drm_capability_GOP0_SUPPORT_TRANSCOLOR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_TransColor, mtk_drm_capability_store_GOP0_TransColor),
	__ATTR(drm_capability_GOP0_SUPPORT_SCROLL, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_Scroll, mtk_drm_capability_store_GOP0_Scroll),
	__ATTR(drm_capability_GOP0_SUPPORT_BLENDXCIP, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_BlendXcIp, mtk_drm_capability_store_GOP0_BlendXcIp),
	__ATTR(drm_capability_GOP0_SUPPORT_CSC, MTK_DRM_GOP_ATTR_MODE,
		mtk_drm_capability_show_GOP0_CSC, mtk_drm_capability_store_GOP0_CSC),
	__ATTR(drm_capability_GOP0_SUPPORT_HDRMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_HDRMode, mtk_drm_capability_store_GOP0_HDRMode),
	__ATTR(drm_capability_GOP0_SUPPORT_HSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_HScaleMode, mtk_drm_capability_store_GOP0_HScaleMode),
	__ATTR(drm_capability_GOP0_SUPPORT_VSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_VScaleMode, mtk_drm_capability_store_GOP0_VScaleMode),
	__ATTR(drm_capability_GOP0_SUPPORT_H10V6SWITCH, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP0_H10V6_Switch,
	       mtk_drm_capability_store_GOP0_H10V6_Switch),
	__ATTR(drm_query_GOP_CRC, MTK_DRM_GOP_ATTR_MODE,
		mtk_drm_query_show_GOP_CRC,
		mtk_drm_query_store_GOP_CRC),
};

static ssize_t mtk_drm_capability_show_GOP1_AFBC(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportAFBC[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_HMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHMirror[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_VMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVMirror[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_TransColor(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportTransColor[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_Scroll(struct device *dev,
						   struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportScroll[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_BlendXcIp(struct device *dev,
						      struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportBlendXcIP[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_CSC(struct device *dev, struct device_attribute *attr,
						char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportCSC[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_HDRMode(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHDRMode[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_HScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHScalingMode[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_VScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVScalingMode[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP1_H10V6_Switch(struct device *dev,
							 struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportH10V6Switch[1]);

	return ssize;
}

static ssize_t mtk_drm_capability_store_GOP1_AFBC(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_HMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_VMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_TransColor(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_Scroll(struct device *dev,
						    struct device_attribute *attr, const char *buf,
						    size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_BlendXcIp(struct device *dev,
						       struct device_attribute *attr,
						       const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_CSC(struct device *dev, struct device_attribute *attr,
						 const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_HDRMode(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_HScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_VScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP1_H10V6_Switch(struct device *dev,
							  struct device_attribute *attr,
							  const char *buf, size_t count)
{
	return count;
}

static struct device_attribute mtk_drm_gop1_attr[] = {
	__ATTR(drm_capability_GOP1_SUPPORT_AFBC, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_AFBC, mtk_drm_capability_store_GOP1_AFBC),
	__ATTR(drm_capability_GOP1_SUPPORT_HMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_HMirror, mtk_drm_capability_store_GOP1_HMirror),
	__ATTR(drm_capability_GOP1_SUPPORT_VMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_VMirror, mtk_drm_capability_store_GOP1_VMirror),
	__ATTR(drm_capability_GOP1_SUPPORT_TRANSCOLOR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_TransColor, mtk_drm_capability_store_GOP1_TransColor),
	__ATTR(drm_capability_GOP1_SUPPORT_SCROLL, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_Scroll, mtk_drm_capability_store_GOP1_Scroll),
	__ATTR(drm_capability_GOP1_SUPPORT_BLENDXCIP, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_BlendXcIp, mtk_drm_capability_store_GOP1_BlendXcIp),
	__ATTR(drm_capability_GOP1_SUPPORT_CSC, MTK_DRM_GOP_ATTR_MODE,
		mtk_drm_capability_show_GOP1_CSC, mtk_drm_capability_store_GOP1_CSC),
	__ATTR(drm_capability_GOP1_SUPPORT_HDRMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_HDRMode, mtk_drm_capability_store_GOP1_HDRMode),
	__ATTR(drm_capability_GOP1_SUPPORT_HSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_HScaleMode, mtk_drm_capability_store_GOP1_HScaleMode),
	__ATTR(drm_capability_GOP1_SUPPORT_VSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_VScaleMode, mtk_drm_capability_store_GOP1_VScaleMode),
	__ATTR(drm_capability_GOP1_SUPPORT_H10V6SWITCH, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP1_H10V6_Switch,
	       mtk_drm_capability_store_GOP1_H10V6_Switch),
};

static ssize_t mtk_drm_capability_show_GOP2_AFBC(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportAFBC[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_HMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHMirror[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_VMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVMirror[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_TransColor(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportTransColor[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_Scroll(struct device *dev,
						   struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportScroll[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_BlendXcIp(struct device *dev,
						      struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportBlendXcIP[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_CSC(struct device *dev, struct device_attribute *attr,
						char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportCSC[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_HDRMode(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHDRMode[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_HScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHScalingMode[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_VScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVScalingMode[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP2_H10V6_Switch(struct device *dev,
							 struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportH10V6Switch[2]);

	return ssize;
}

static ssize_t mtk_drm_capability_store_GOP2_AFBC(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_HMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_VMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_TransColor(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_Scroll(struct device *dev,
						    struct device_attribute *attr, const char *buf,
						    size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_BlendXcIp(struct device *dev,
						       struct device_attribute *attr,
						       const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_CSC(struct device *dev, struct device_attribute *attr,
						 const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_HDRMode(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_HScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_VScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP2_H10V6_Switch(struct device *dev,
							  struct device_attribute *attr,
							  const char *buf, size_t count)
{
	return count;
}

static struct device_attribute mtk_drm_gop2_attr[] = {
	__ATTR(drm_capability_GOP2_SUPPORT_AFBC, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_AFBC, mtk_drm_capability_store_GOP2_AFBC),
	__ATTR(drm_capability_GOP2_SUPPORT_HMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_HMirror, mtk_drm_capability_store_GOP2_HMirror),
	__ATTR(drm_capability_GOP2_SUPPORT_VMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_VMirror, mtk_drm_capability_store_GOP2_VMirror),
	__ATTR(drm_capability_GOP2_SUPPORT_TRANSCOLOR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_TransColor, mtk_drm_capability_store_GOP2_TransColor),
	__ATTR(drm_capability_GOP2_SUPPORT_SCROLL, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_Scroll, mtk_drm_capability_store_GOP2_Scroll),
	__ATTR(drm_capability_GOP2_SUPPORT_BLENDXCIP, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_BlendXcIp, mtk_drm_capability_store_GOP2_BlendXcIp),
	__ATTR(drm_capability_GOP2_SUPPORT_CSC, MTK_DRM_GOP_ATTR_MODE,
		mtk_drm_capability_show_GOP2_CSC, mtk_drm_capability_store_GOP2_CSC),
	__ATTR(drm_capability_GOP2_SUPPORT_HDRMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_HDRMode, mtk_drm_capability_store_GOP2_HDRMode),
	__ATTR(drm_capability_GOP2_SUPPORT_HSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_HScaleMode, mtk_drm_capability_store_GOP2_HScaleMode),
	__ATTR(drm_capability_GOP2_SUPPORT_VSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_VScaleMode, mtk_drm_capability_store_GOP2_VScaleMode),
	__ATTR(drm_capability_GOP2_SUPPORT_H10V6SWITCH, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP2_H10V6_Switch,
	       mtk_drm_capability_store_GOP2_H10V6_Switch),
};

static ssize_t mtk_drm_capability_show_GOP3_AFBC(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportAFBC[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_HMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHMirror[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_VMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVMirror[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_TransColor(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportTransColor[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_Scroll(struct device *dev,
						   struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportScroll[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_BlendXcIp(struct device *dev,
						      struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportBlendXcIP[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_CSC(struct device *dev, struct device_attribute *attr,
						char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportCSC[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_HDRMode(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHDRMode[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_HScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHScalingMode[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_VScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVScalingMode[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP3_H10V6_Switch(struct device *dev,
							 struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportH10V6Switch[3]);

	return ssize;
}

static ssize_t mtk_drm_capability_store_GOP3_AFBC(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_HMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_VMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_TransColor(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_Scroll(struct device *dev,
						    struct device_attribute *attr, const char *buf,
						    size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_BlendXcIp(struct device *dev,
						       struct device_attribute *attr,
						       const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_CSC(struct device *dev, struct device_attribute *attr,
						 const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_HDRMode(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_HScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_VScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP3_H10V6_Switch(struct device *dev,
							  struct device_attribute *attr,
							  const char *buf, size_t count)
{
	return count;
}

static struct device_attribute mtk_drm_gop3_attr[] = {
	__ATTR(drm_capability_GOP3_SUPPORT_AFBC, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_AFBC, mtk_drm_capability_store_GOP3_AFBC),
	__ATTR(drm_capability_GOP3_SUPPORT_HMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_HMirror, mtk_drm_capability_store_GOP3_HMirror),
	__ATTR(drm_capability_GOP3_SUPPORT_VMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_VMirror, mtk_drm_capability_store_GOP3_VMirror),
	__ATTR(drm_capability_GOP3_SUPPORT_TRANSCOLOR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_TransColor, mtk_drm_capability_store_GOP3_TransColor),
	__ATTR(drm_capability_GOP3_SUPPORT_SCROLL, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_Scroll, mtk_drm_capability_store_GOP3_Scroll),
	__ATTR(drm_capability_GOP3_SUPPORT_BLENDXCIP, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_BlendXcIp, mtk_drm_capability_store_GOP3_BlendXcIp),
	__ATTR(drm_capability_GOP3_SUPPORT_CSC, MTK_DRM_GOP_ATTR_MODE,
		mtk_drm_capability_show_GOP3_CSC, mtk_drm_capability_store_GOP3_CSC),
	__ATTR(drm_capability_GOP3_SUPPORT_HDRMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_HDRMode, mtk_drm_capability_store_GOP3_HDRMode),
	__ATTR(drm_capability_GOP3_SUPPORT_HSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_HScaleMode, mtk_drm_capability_store_GOP3_HScaleMode),
	__ATTR(drm_capability_GOP3_SUPPORT_VSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_VScaleMode, mtk_drm_capability_store_GOP3_VScaleMode),
	__ATTR(drm_capability_GOP3_SUPPORT_H10V6SWITCH, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP3_H10V6_Switch,
	       mtk_drm_capability_store_GOP3_H10V6_Switch),
};

static ssize_t mtk_drm_capability_show_GOP4_AFBC(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportAFBC[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_HMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHMirror[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_VMirror(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVMirror[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_TransColor(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportTransColor[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_Scroll(struct device *dev,
						   struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportScroll[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_BlendXcIp(struct device *dev,
						      struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportBlendXcIP[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_CSC(struct device *dev, struct device_attribute *attr,
						char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportCSC[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_HDRMode(struct device *dev,
						    struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHDRMode[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_HScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportHScalingMode[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_VScaleMode(struct device *dev,
						       struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportVScalingMode[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_show_GOP4_H10V6_Switch(struct device *dev,
							 struct device_attribute *attr, char *buf)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int ssize = 0;
	uint16_t u16Size = 255;

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%d\n",
		     pctx_gop->gop_plane_capability.u8SupportH10V6Switch[4]);

	return ssize;
}

static ssize_t mtk_drm_capability_store_GOP4_AFBC(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_HMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_VMirror(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_TransColor(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_Scroll(struct device *dev,
						    struct device_attribute *attr, const char *buf,
						    size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_BlendXcIp(struct device *dev,
						       struct device_attribute *attr,
						       const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_CSC(struct device *dev, struct device_attribute *attr,
						 const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_HDRMode(struct device *dev,
						     struct device_attribute *attr, const char *buf,
						     size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_HScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_VScaleMode(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_drm_capability_store_GOP4_H10V6_Switch(struct device *dev,
							  struct device_attribute *attr,
							  const char *buf, size_t count)
{
	return count;
}

static struct device_attribute mtk_drm_gop4_attr[] = {
	__ATTR(drm_capability_GOP4_SUPPORT_AFBC, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_AFBC, mtk_drm_capability_store_GOP4_AFBC),
	__ATTR(drm_capability_GOP4_SUPPORT_HMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_HMirror, mtk_drm_capability_store_GOP4_HMirror),
	__ATTR(drm_capability_GOP4_SUPPORT_VMIRROR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_VMirror, mtk_drm_capability_store_GOP4_VMirror),
	__ATTR(drm_capability_GOP4_SUPPORT_TRANSCOLOR, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_TransColor, mtk_drm_capability_store_GOP4_TransColor),
	__ATTR(drm_capability_GOP4_SUPPORT_SCROLL, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_Scroll, mtk_drm_capability_store_GOP4_Scroll),
	__ATTR(drm_capability_GOP4_SUPPORT_BLENDXCIP, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_BlendXcIp, mtk_drm_capability_store_GOP4_BlendXcIp),
	__ATTR(drm_capability_GOP4_SUPPORT_CSC, MTK_DRM_GOP_ATTR_MODE,
		mtk_drm_capability_show_GOP4_CSC, mtk_drm_capability_store_GOP4_CSC),
	__ATTR(drm_capability_GOP4_SUPPORT_HDRMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_HDRMode, mtk_drm_capability_store_GOP4_HDRMode),
	__ATTR(drm_capability_GOP4_SUPPORT_HSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_HScaleMode, mtk_drm_capability_store_GOP4_HScaleMode),
	__ATTR(drm_capability_GOP4_SUPPORT_VSCALEMODE, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_VScaleMode, mtk_drm_capability_store_GOP4_VScaleMode),
	__ATTR(drm_capability_GOP4_SUPPORT_H10V6SWITCH, MTK_DRM_GOP_ATTR_MODE,
	       mtk_drm_capability_show_GOP4_H10V6_Switch,
	       mtk_drm_capability_store_GOP4_H10V6_Switch),
};

void mtk_drm_gop_get_capability(struct device *dev, struct mtk_gop_context *pctx_gop)
{
	int ret = 0, i = 0;
	struct device_node *np;
	char cstr[100] = "";
	const char *name;
	uint32_t u32CapVal = 0;
	uint8_t u8GOPNum;

	snprintf(cstr, sizeof(cstr), "mediatek,mtk-dtv-gop%d", 0);
	np = of_find_compatible_node(NULL, NULL, cstr);
	if (np != NULL) {
		name = MTK_DRM_DTS_GOP_CAPABILITY_TAG;
		ret = of_property_read_u32(np, name, &u32CapVal);
		if (ret != 0x0)
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s \r\n",
				  __func__, __LINE__, name);
	}

	u8GOPNum = (uint8_t) (u32CapVal & MTK_GOP_CAPS_OFFSET_GOPNUM_MSK);
	pctx_gop->gop_plane_capability.u8GOPNum = u8GOPNum;

	pctx_gop->gop_plane_capability.u8IsDramAddrWordUnit =
	    (uint8_t) ((u32CapVal & BIT(MTK_GOP_CAPS_OFFSET_DRAMADDR_ISWORD_UNIT)) >>
		       MTK_GOP_CAPS_OFFSET_DRAMADDR_ISWORD_UNIT);
	pctx_gop->gop_plane_capability.u8IsPixelBaseHorizontal =
	    (uint8_t) ((u32CapVal & BIT(MTK_GOP_CAPS_OFFSET_H_PIXELBASE)) >>
		       MTK_GOP_CAPS_OFFSET_H_PIXELBASE);
	pctx_gop->gop_plane_capability.u8SupportVopPathSel =
	    (uint8_t) ((u32CapVal & BIT(MTK_GOP_CAPS_OFFSET_VOP_PATHSEL)) >>
		       MTK_GOP_CAPS_OFFSET_VOP_PATHSEL);

	name = MTK_DRM_DTS_GOP_CAPABILITY_TAG;
	for (i = 0; i < u8GOPNum; i++) {
		snprintf(cstr, sizeof(cstr), "mediatek,mtk-dtv-gop%d", i);
		np = of_find_compatible_node(NULL, NULL, cstr);
		if (of_property_read_u32(np, name, &u32CapVal) != 0)
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s \r\n",
				  __func__, __LINE__, name);

		pctx_gop->gop_plane_capability.u8SupportAFBC[i] =
		    (uint8_t) ((u32CapVal & BIT(_gu32CapsAFBC[((i != 0) ? 1 : 0)])) >>
			       _gu32CapsAFBC[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportHMirror[i] =
		    (uint8_t) ((u32CapVal & BIT(_gu32CapsHMirror[((i != 0) ? 1 : 0)])) >>
			       _gu32CapsHMirror[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportVMirror[i] =
		    (uint8_t) ((u32CapVal & BIT(_gu32CapsVMirror[((i != 0) ? 1 : 0)])) >>
			       _gu32CapsVMirror[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportTransColor[i] =
		    (uint8_t) ((u32CapVal & BIT(_gu32CapsTransColor[((i != 0) ? 1 : 0)])) >>
			       _gu32CapsTransColor[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportScroll[i] =
		    (uint8_t) ((u32CapVal & BIT(_gu32CapsScroll[((i != 0) ? 1 : 0)])) >>
			       _gu32CapsScroll[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportBlendXcIP[i] =
		    (uint8_t) ((u32CapVal & BIT(_gu32CapsBlendXcIp[((i != 0) ? 1 : 0)])) >>
			       _gu32CapsBlendXcIp[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportCSC[i] =
		    (uint8_t) ((u32CapVal & BIT(_gu32CapsCSC[((i != 0) ? 1 : 0)])) >>
			       _gu32CapsCSC[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportHDRMode[i] =
		    (uint8_t) ((u32CapVal & _gu32CapsHDRMode[((i != 0) ? 1 : 0)]) >>
			       _gu32CapsHDRModeSft[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportHScalingMode[i] =
		    (uint8_t) ((u32CapVal & _gu32CapsHScaleMode[((i != 0) ? 1 : 0)]) >>
			       _gu32CapsHScaleSft[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportVScalingMode[i] =
		    (uint8_t) ((u32CapVal & _gu32CapsVScaleMode[((i != 0) ? 1 : 0)]) >>
			       _gu32CapsVScaleSft[((i != 0) ? 1 : 0)]);
		pctx_gop->gop_plane_capability.u8SupportH10V6Switch[i] =
		    (uint8_t) ((u32CapVal & BIT(_gu32CapsH10V6Switch[((i != 0) ? 1 : 0)])) >>
			       _gu32CapsH10V6Switch[((i != 0) ? 1 : 0)]);
	}

}

void mtk_drm_gop_CreateSysFile(struct device *dev)
{
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int i = 0, j = 0;

	mtk_drm_gop_get_capability(dev, pctx_gop);

	for (i = 0; i < pctx_gop->gop_plane_capability.u8GOPNum; i++) {
		if (i == 0) {
			for (j = 0; j < sizeof(mtk_drm_gop0_attr) / sizeof(struct device_attribute);
			     j++) {
				if (device_create_file(dev, &mtk_drm_gop0_attr[j]) != 0)
					DRM_ERROR
					    ("[GOP][%s, %d]: create gop0 capability file error\n");
			}
		} else if (i == 1) {
			for (j = 0; j < sizeof(mtk_drm_gop1_attr) / sizeof(struct device_attribute);
			     j++) {
				if (device_create_file(dev, &mtk_drm_gop1_attr[j]) != 0)
					DRM_ERROR
					    ("[GOP][%s, %d]: create gop0 capability file error\n");
			}
		} else if (i == 2) {
			for (j = 0; j < sizeof(mtk_drm_gop2_attr) / sizeof(struct device_attribute);
			     j++) {
				if (device_create_file(dev, &mtk_drm_gop2_attr[j]) != 0)
					DRM_ERROR
					    ("[GOP][%s, %d]: create gop0 capability file error\n");
			}
		} else if (i == 3) {
			for (j = 0; j < sizeof(mtk_drm_gop3_attr) / sizeof(struct device_attribute);
			     j++) {
				if (device_create_file(dev, &mtk_drm_gop3_attr[j]) != 0)
					DRM_ERROR
					    ("[GOP][%s, %d]: create gop0 capability file error\n");
			}
		} else if (i == 4) {
			for (j = 0; j < sizeof(mtk_drm_gop4_attr) / sizeof(struct device_attribute);
			     j++) {
				if (device_create_file(dev, &mtk_drm_gop4_attr[j]) != 0)
					DRM_ERROR
					    ("[GOP][%s, %d]: create gop0 capability file error\n");
			}
		}
	}
}

void mtk_gop_enable(struct mtk_tv_drm_crtc *crtc)
{
	int i = 0;
	UTP_GOP_SET_ENABLE stGopEn;

	for (i = 0; i < crtc->plane_count[MTK_DRM_PLANE_TYPE_GRAPHIC]; i++) {
		stGopEn.gop_idx = crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][i].gop_index;
		if (stGopEn.gop_idx == INVALID_NUM) {
			DRM_ERROR("[GOP][%s, %d]:  Invalid GOP number\r\n", __func__, __LINE__);
			return;
		}

		if (crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][i].base.state->visible == true)
			stGopEn.bEn = true;
		else
			stGopEn.bEn = false;

		utp_gop_set_enable(&stGopEn);
	}
}

void mtk_gop_disable(struct mtk_tv_drm_crtc *crtc)
{
	int i = 0;
	UTP_GOP_SET_ENABLE stGopEn;

	for (i = 0; i < crtc->plane_count[MTK_DRM_PLANE_TYPE_GRAPHIC]; i++) {
		stGopEn.gop_idx = crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][i].gop_index;
		if (stGopEn.gop_idx == INVALID_NUM) {
			DRM_ERROR("[GOP][%s, %d]:  Invalid GOP number\r\n", __func__, __LINE__);
			return;
		}

		if (crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][i].base.state->visible == true)
			stGopEn.bEn = true;
		else
			stGopEn.bEn = false;

		utp_gop_set_enable(&stGopEn);
	}
}

int mtk_gop_enable_vblank(struct mtk_tv_drm_crtc *crtc)
{
	UTP_GOP_SET_ENABLE SetEnable;

	SetEnable.gop_idx = 0;
	SetEnable.bEn = true;

	utp_gop_intr_ctrl(&SetEnable);
	return 0;
}

void mtk_gop_update_plane(struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	UTP_GOP_SET_WINDOW stGOPInfo;
	struct drm_framebuffer *fb = plane->state->fb;

	memset(&stGOPInfo, 0, sizeof(UTP_GOP_SET_WINDOW));

	stGOPInfo.gop_idx = mplane->gop_index;
	if (stGOPInfo.gop_idx == INVALID_NUM) {
		DRM_ERROR("[GOP][%s, %d]: Invalid GOP number\r\n", __func__, __LINE__);
		return;
	}
	stGOPInfo.win_enable = true;
	stGOPInfo.stretchWinX = state->pending.x;
	stGOPInfo.stretchWinY = state->pending.y;
	stGOPInfo.stretchWinDstW = state->base.crtc_w;
	stGOPInfo.stretchWinDstH = state->base.crtc_h;
	stGOPInfo.pic_x_start = (__u16) state->base.src_x;
	stGOPInfo.pic_y_start = (__u16) state->base.src_y;
	stGOPInfo.pic_x_end = (__u16) (state->base.src_x + (state->base.src_w >> 16));
	stGOPInfo.pic_y_end = (__u16) (state->base.src_y + (state->base.src_h >> 16));
	stGOPInfo.fb_width = state->pending.fb_width;
	stGOPInfo.fb_height = state->pending.fb_height;
	stGOPInfo.fb_pitch = state->pending.pitch;
	stGOPInfo.fb_addr = state->pending.addr - 0x20000000;	//it need to subtract bus address
	stGOPInfo.u8zpos = state->base.zpos;

	/*calculate for cropping window mode */
	if ((state->base.crtc_x < 0) || (state->base.crtc_y < 0)) {
		stGOPInfo.cropEn = true;
		if (state->base.crtc_x < 0)
			stGOPInfo.cropWinXStart = abs(state->base.crtc_x);
		else
			stGOPInfo.cropWinXStart = 0;

		if (state->base.crtc_y < 0)
			stGOPInfo.cropWinYStart = abs(state->base.crtc_y);
		else
			stGOPInfo.cropWinYStart = 0;

		stGOPInfo.cropWinXEnd = stGOPInfo.stretchWinX + state->base.crtc_w;
		stGOPInfo.cropWinYEnd = stGOPInfo.stretchWinY + state->base.crtc_h;
	} else {
		stGOPInfo.cropEn = false;
	}

	stGOPInfo.fmt_type = _convertColorFmt(state->pending.format);
	stGOPInfo.hmirror =
	    (pctx_gop->gop_plane_properties + mplane->gop_index)->prop_val[E_GOP_PROP_HMIRROR];
	stGOPInfo.vmirror =
	    (pctx_gop->gop_plane_properties + mplane->gop_index)->prop_val[E_GOP_PROP_VMIRROR];
	stGOPInfo.hstretch_mode =
	    _convertHStretchMode((pctx_gop->gop_plane_properties +
				  mplane->gop_index)->prop_val[E_GOP_PROP_HSTRETCH]);
	stGOPInfo.vstretch_mode =
	    _convertVStretchMode((pctx_gop->gop_plane_properties +
				  mplane->gop_index)->prop_val[E_GOP_PROP_VSTRETCH]);
	stGOPInfo.afbc_buffer = (((fb->modifier >> 56) & 0xF) == DRM_FORMAT_MOD_VENDOR_ARM) ? 1 : 0;

	if (_bCSC_tuning_state_change(mplane->gop_index, pctx_gop, state)) {
		stGOPInfo.GopChangeStatus |= BIT(GOP_CSC_STATE_CHANGE);
		stGOPInfo.pGOPCSCParam = (pctx_gop->pCurrent_CSCParam + mplane->gop_index);
	} else
		stGOPInfo.GopChangeStatus &= ~(BIT(GOP_CSC_STATE_CHANGE));

	stGOPInfo.eGopType = plane->type;
	utp_gop_SetWindow(&stGOPInfo);
}

void mtk_gop_disable_plane(struct mtk_plane_state *state)
{
	UTP_GOP_SET_ENABLE stGopEn;
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);

	stGopEn.gop_idx = mplane->gop_index;

	if (stGopEn.gop_idx == INVALID_NUM) {
		DRM_ERROR("[GOP][%s, %d]: Invalid GOP number\r\n", __func__, __LINE__);
		return;
	}

	if (state->pending.enable == false) {
		stGopEn.bEn = false;
		utp_gop_set_enable(&stGopEn);
	}
}

void mtk_gop_disable_vblank(struct mtk_tv_drm_crtc *crtc)
{
	UTP_GOP_SET_ENABLE SetEnable;

	SetEnable.gop_idx = 0;
	SetEnable.bEn = false;
	utp_gop_intr_ctrl(&SetEnable);
}

int mtk_gop_atomic_set_crtc_property(struct mtk_tv_drm_crtc *crtc, struct drm_crtc_state *state,
				     struct drm_property *property, uint64_t val)
{
	return 0;
}

int mtk_gop_atomic_get_crtc_property(struct mtk_tv_drm_crtc *crtc,
				     const struct drm_crtc_state *state,
				     struct drm_property *property, uint64_t *val)
{
	return 0;
}

int mtk_gop_atomic_set_plane_property(struct mtk_drm_plane *mplane, struct drm_plane_state *state,
				      struct drm_property *property, uint64_t val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int plane_inx = mplane->gop_index;
	struct gop_prop *plane_props = (pctx_gop->gop_plane_properties + plane_inx);

	for (i = 0; i < E_GOP_PROP_MAX; i++) {
		if (property == pctx_gop->drm_gop_plane_prop[i]) {
			plane_props->prop_val[i] = val;
			ret = 0x0;
			break;
		}
	}

	return ret;
}

int mtk_gop_atomic_get_plane_property(struct mtk_drm_plane *mplane,
				      const struct drm_plane_state *state,
				      struct drm_property *property, uint64_t *val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	int plane_inx = mplane->gop_index;
	struct gop_prop *plane_props = (pctx_gop->gop_plane_properties + plane_inx);

	for (i = 0; i < E_GOP_PROP_MAX; i++) {
		if (property == pctx_gop->drm_gop_plane_prop[i]) {
			*val = plane_props->prop_val[i];
			ret = 0x0;
			break;
		}
	}

	return ret;
}

int mtk_gop_bootlogo_ctrl(struct mtk_tv_drm_crtc *crtc, unsigned int cmd, unsigned int *gop)
{
	struct mtk_drm_plane *mplane = crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC];
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;

	if (cmd == MTK_CTRL_BOOTLOGO_CMD_GETINFO) {
		*gop = pctx_gop->nBootlogoGop;
	} else if (cmd == MTK_CTRL_BOOTLOGO_CMD_DISABLE) {
		UTP_GOP_INIT_INFO utp_init_info;

		utp_init_info.gop_idx = pctx_gop->nBootlogoGop;
		utp_init_info.panel_width = pctx_gop->utpGopInit.panel_width;
		utp_init_info.panel_height = pctx_gop->utpGopInit.panel_height;
		utp_init_info.panel_hstart = pctx_gop->utpGopInit.panel_hstart;
		utp_init_info.ignore_level = UTP_E_GOP_IGNORE_NORMAL;
		utp_gop_InitByGop(&utp_init_info);
	}
	return 0;
}

int mtk_drm_attach_props(struct mtk_gop_context *pctx_gop, struct mtk_drm_plane *mplane)
{
	int extend_prop_count = ARRAY_SIZE(gop_plane_props);
	struct drm_plane *plane_base = &mplane->base;
	struct drm_mode_object *plane_obj = &plane_base->base;
	int plane_inx = mplane->gop_index;
	struct gop_prop *plane_props = (pctx_gop->gop_plane_properties + plane_inx);
	int i;

	//////////////////////////////  PROPERTIES //////////////////////////////
	for (i = 0; i < extend_prop_count; i++) {
		drm_object_attach_property(plane_obj, pctx_gop->drm_gop_plane_prop[i],
					   plane_props->prop_val[i]);
	}

	drm_plane_create_color_properties(plane_base,
					  BIT(DRM_COLOR_YCBCR_BT601) |
					  BIT(DRM_COLOR_YCBCR_BT709) |
					  BIT(DRM_COLOR_YCBCR_BT2020) |
					  BIT(DRM_COLOR_RGB_BT601) |
					  BIT(DRM_COLOR_RGB_BT709) |
					  BIT(DRM_COLOR_RGB_BT2020) |
					  BIT(DRM_COLOR_RGB_SRGB) |
					  BIT(DRM_COLOR_RGB_DCIP3_D65),
					  BIT(DRM_COLOR_YCBCR_LIMITED_RANGE) |
					  BIT(DRM_COLOR_YCBCR_FULL_RANGE) |
					  BIT(DRM_COLOR_RGB_LIMITED_RANGE) |
					  BIT(DRM_COLOR_RGB_FULL_RANGE),
					  DRM_COLOR_RGB_BT709, DRM_COLOR_RGB_FULL_RANGE);
	return 0;
}

static int readDtbS32Array(struct device_node *np,
			   const char *tag_name, int **ret_array, int ret_array_length)
{
	int ret;
	int i;
	__u32 *u32TmpArray = NULL;
	int *array = NULL;

	if ((ret_array_length == 0) || (ret_array == NULL)) {
		DRM_ERROR("[GOP][%s, %d]: invalid param \r\n", __func__, __LINE__);
		ret = -EINVAL;
		goto ERROR;
	}

	u32TmpArray = kmalloc_array(ret_array_length, sizeof(__u32), GFP_KERNEL);
	array = kmalloc_array(ret_array_length, sizeof(int), GFP_KERNEL);
	if ((u32TmpArray == NULL) || (array == NULL)) {
		DRM_ERROR("[GOP][%s, %d]: malloc failed \r\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto ERROR;
	}
	ret = of_property_read_u32_array(np, tag_name, u32TmpArray, ret_array_length);
	if (ret != 0x0) {
		DRM_ERROR("[GOP][%s, %d]: of_property_read_u32_array failed \r\n", __func__,
			  __LINE__);
		goto ERROR;
	}
	for (i = 0; i < ret_array_length; i++)
		array[i] = u32TmpArray[i];

	kfree(u32TmpArray);
	*ret_array = array;
	return 0;

ERROR:
	kfree(u32TmpArray);
	kfree(array);

	return ret;
}

static int getDefaultPlaneProps(struct mtk_gop_context *pctx_gop,
				struct device_node *np, const char *tag_name, int prop_inx)
{
	int ret;
	int i;
	int *tmpArray = NULL;

	ret = readDtbS32Array(np, tag_name, &tmpArray, pctx_gop->GopNum);
	if (ret != 0x0) {
		DRM_ERROR("[GOP][%s, %d]: readDtbS32Array failed, name = %s \r\n", __func__,
			  __LINE__, tag_name);
		return ret;
	}

	if (tmpArray == NULL)
		ret = -ENOMEM;

	for (i = 0; i < pctx_gop->GopNum; i++)
		(pctx_gop->gop_plane_properties + i)->prop_val[prop_inx] = tmpArray[i];

	kfree(tmpArray);
	return ret;
}

static int readChipProp2GOPPrivate(struct mtk_gop_context *pctx_gop)
{
	int i, ret;

	for (i = 0; i < pctx_gop->GopNum; i++) {
		UTP_GOP_QUERY_FEATURE query_afbc;

		query_afbc.gop_idx = i;
		query_afbc.feature = UTP_E_GOP_CAP_AFBC_SUPPORT;
		ret = utp_gop_query_chip_prop(&query_afbc);
		if (ret != 0x0) {
			DRM_ERROR
			    ("[GOP][%s, %d]gop_query_chip_prop AFBC_SUPPORT failed\r\n",
			     __func__, __LINE__);
			return ret;
		}

		pctx_gop->gop_plane_properties[i].prop_val[E_GOP_PROP_AFBC_FEATURE] =
		    query_afbc.value;
	}
	return 0;
}

int readDTB2GOPPrivate(struct mtk_gop_context *pctx_gop)
{
	int ret;
	struct device_node *np;
	int u32Tmp = 0xFF, i = 0;
	const char *name;

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_TV_KMS_COMPATIBLE_STR);

	if (np != NULL) {
		name = MTK_DRM_DTS_GOP_PLANE_NUM_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->GopNum = u32Tmp;

		name = MTK_DRM_DTS_HMIRROR_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_HMIRROR);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_VMIRROR_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_VMIRROR);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_HSTRETCH_INIT_MODE_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_HSTRETCH);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_VSTRETCH_INIT_MODE_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_VSTRETCH);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_HUE_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_HUE);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_SATURATION_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_SATURATION);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_CONTRAST_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_CONTRAST);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_BRIGHTNESS_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_BRIGHTNESS);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_RGAIN_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_RGAIN);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_GGAIN_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_GGAIN);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}

		name = MTK_DRM_DTS_BGAIN_TAG;
		ret = getDefaultPlaneProps(pctx_gop, np, name, E_GOP_PROP_BGAIN);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: getDefaultPlaneProps failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
	}

	np = of_find_node_by_name(NULL, MTK_DRM_PANEL_COLOR_COMPATIBLE_STR);

	if (np != NULL) {
		name = MTK_DRM_DTS_PANEL_COLOR_SAPCE_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->out_plane_color_encoding = (enum drm_color_encoding)u32Tmp;

		name = MTK_DRM_DTS_PANEL_COLOR_FORAMT_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->out_plane_color_format = (UTP_EN_GOP_DATA_FMT) u32Tmp;

		name = MTK_DRM_DTS_PANEL_COLOR_RANGE_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->out_plane_color_range = (enum drm_color_range)u32Tmp;

		name = MTK_DRM_DTS_PANEL_CURLUM_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		for (i = 0; i < pctx_gop->GopNum; i++)
			(pctx_gop->gop_plane_properties + i)->prop_val[E_GOP_PROP_PNL_CURLUM] =
			    u32Tmp;

		name = MTK_DRM_DTS_PANEL_PRIMARIES_RX_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->pPnlInfo->u16Display_primaries_x[0] = (__u16) u32Tmp;

		name = MTK_DRM_DTS_PANEL_PRIMARIES_GX_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->pPnlInfo->u16Display_primaries_x[1] = (__u16) u32Tmp;

		name = MTK_DRM_DTS_PANEL_PRIMARIES_BX_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->pPnlInfo->u16Display_primaries_x[2] = (__u16) u32Tmp;

		name = MTK_DRM_DTS_PANEL_PRIMARIES_RY_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->pPnlInfo->u16Display_primaries_y[0] = (__u16) u32Tmp;

		name = MTK_DRM_DTS_PANEL_PRIMARIES_GY_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->pPnlInfo->u16Display_primaries_y[1] = (__u16) u32Tmp;

		name = MTK_DRM_DTS_PANEL_PRIMARIES_BY_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->pPnlInfo->u16Display_primaries_y[2] = (__u16) u32Tmp;
		name = MTK_DRM_DTS_PANEL_PRIMARIES_WHITE_X_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->pPnlInfo->u16White_point_x = (__u16) u32Tmp;

		name = MTK_DRM_DTS_PANEL_PRIMARIES_WHITE_Y_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->pPnlInfo->u16White_point_y = (__u16) u32Tmp;
	}

	np = of_find_node_by_name(NULL, MTK_DRM_PANEL_INFO_STR);
	if (np != NULL) {
		name = MTK_DRM_DTS_PANELW_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->utpGopInit.panel_width = u32Tmp;

		name = MTK_DRM_DTS_PANELH_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->utpGopInit.panel_height = u32Tmp;

		name = MTK_DRM_DTS_PANELHSTR_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: of_property_read_u32 failed, name = %s\r\n",
				  __func__, __LINE__, name);
			return ret;
		}
		pctx_gop->utpGopInit.panel_hstart = u32Tmp;
	}
	return 0;
}

static void clean_gop_context(struct device *dev, struct mtk_gop_context *pctx_gop)
{
	if (pctx_gop) {
		kfree(pctx_gop->gop_plane_properties);
		pctx_gop->gop_plane_properties = NULL;
		kfree(pctx_gop->pCurrent_CSCParam);
		pctx_gop->pCurrent_CSCParam = NULL;
		kfree(pctx_gop->pPnlInfo);
		pctx_gop->pPnlInfo = NULL;
	}
}

static int mtk_gop_get_bootlogo_gop(unsigned int gop_count)
{
	int i;
	UTP_GOP_SET_ENABLE en;
	int bootlogo_idx = 0xFF;

	for (i = 0; i < gop_count; i++) {
		en.gop_idx = i;
		en.bEn = false;
		utp_gop_is_enable(&en);
		if (en.bEn == true) {
			bootlogo_idx = i;
			break;
		}
	}

	return bootlogo_idx;
}

static void _mtk_drm_set_properties_init_value(struct mtk_gop_context *pctx_gop)
{
	int i = 0;

	for (i = 0; i < pctx_gop->GopNum; i++) {
		(pctx_gop->gop_plane_properties + i)->prop_val[E_GOP_PROP_COLOR_MODE] =
		    MTK_DRM_GOP_COLOR_MODE_DEFAULT;
		(pctx_gop->gop_plane_properties + i)->prop_val[E_GOP_PROP_SRC_MAXLUM] =
		    MTK_DRM_GOP_SRC_MAXLUM_DEFAULT;
	}
}

static int mtk_drm_create_properties(struct mtk_tv_kms_context *pctx)
{
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	struct drm_property *prop;
	const struct ext_prop_info *pprop;
	int extend_prop_count = ARRAY_SIZE(gop_plane_props);
	int i;

	for (i = 0; i < extend_prop_count; i++) {
		pprop = &gop_plane_props[i];
		if ((pprop->flag & DRM_MODE_PROP_ENUM) || (pprop->flag & DRM_MODE_PROP_IMMUTABLE)) {
			prop = drm_property_create_enum(pctx->drm_dev, pprop->flag,
							pprop->prop_name,
							pprop->enum_info.enum_list,
							pprop->enum_info.enum_length);
			if (prop == NULL) {
				DRM_ERROR("[GOP][%s, %d]: drm_property_create_enum fail!!\n",
					  __func__, __LINE__);
				return -ENOMEM;
			}
		} else if ((pprop->flag & DRM_MODE_PROP_RANGE)) {
			prop = drm_property_create_range(pctx->drm_dev, pprop->flag,
							 pprop->prop_name, pprop->range_info.min,
							 pprop->range_info.max);
		} else {
			DRM_ERROR("[GOP][%s, %d]: unknown prop flag 0x%x !!\n", __func__,
				  __LINE__, pprop->flag);
			return -EINVAL;
		}

		pctx_gop->drm_gop_plane_prop[i] = prop;
	}

	_mtk_drm_set_properties_init_value(pctx_gop);

	return 0;
}

int mtk_gop_init(struct device *dev, struct device *master, void *data,
		 struct mtk_drm_plane **primary_plane, struct mtk_drm_plane **cursor_plane)
{
	int ret;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_gop_context *pctx_gop = &pctx->gop_priv;
	struct mtk_tv_drm_crtc *crtc = &pctx->crtc;
	struct mtk_drm_plane *mplane = NULL;
	struct drm_device *drm_dev = data;
	enum drm_plane_type drmPlaneType;
	unsigned int gop_idx;
	struct gop_prop *props;
	unsigned int zpos_base = pctx->plane_index_start[MTK_DRM_PLANE_TYPE_GRAPHIC];

	utp_gop_InitByGop(NULL);	// get instance only
	pctx_gop->GopNum = pctx->plane_num[MTK_DRM_PLANE_TYPE_GRAPHIC];
	pctx_gop->nBootlogoGop = mtk_gop_get_bootlogo_gop(pctx_gop->GopNum);

	mplane = devm_kzalloc(dev, sizeof(struct mtk_drm_plane) * pctx_gop->GopNum, GFP_KERNEL);
	if (mplane == NULL) {
		DRM_ERROR("[GOP][%s, %d]: devm_kzalloc failed.\n", __func__, __LINE__);
		goto ERR;
	}
	crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC] = mplane;
	crtc->plane_count[MTK_DRM_PLANE_TYPE_GRAPHIC] = pctx_gop->GopNum;

	props = devm_kzalloc(dev, sizeof(struct gop_prop) * pctx_gop->GopNum, GFP_KERNEL);
	if (props == NULL) {
		DRM_ERROR("[GOP][%s, %d]: devm_kzalloc failed.\n", __func__, __LINE__);
		goto ERR;
	}
	memset(props, 0, sizeof(struct gop_prop) * pctx_gop->GopNum);
	pctx_gop->gop_plane_properties = props;

	pctx_gop->pCurrent_CSCParam =
	    devm_kzalloc(dev, sizeof(UTP_GOP_CSC_PARAM) * pctx_gop->GopNum, GFP_KERNEL);
	if (pctx_gop->pCurrent_CSCParam == NULL) {
		DRM_ERROR("[GOP][%s, %d]: devm_kzalloc failed.\n", __func__, __LINE__);
		goto ERR;
	}
	memset(pctx_gop->pCurrent_CSCParam, 0, sizeof(UTP_GOP_CSC_PARAM) * pctx_gop->GopNum);

	pctx_gop->pPnlInfo = devm_kzalloc(dev, sizeof(UTP_GOP_PNL_INFO), GFP_KERNEL);
	if (pctx_gop->pPnlInfo == NULL) {
		DRM_ERROR("[GOP][%s, %d]: devm_kzalloc failed.\n", __func__, __LINE__);
		goto ERR;
	}
	memset(pctx_gop->pPnlInfo, 0, sizeof(UTP_GOP_PNL_INFO));

	ret = readDTB2GOPPrivate(pctx_gop);
	if (ret != 0x0) {
		DRM_ERROR("[GOP][%s, %d]: readDTB2GOPPrivate failed.\n", __func__, __LINE__);
		goto ERR;
	}

	ret = readChipProp2GOPPrivate(pctx_gop);
	if (ret != 0x0) {
		DRM_ERROR("[GOP][%s, %d]: readChipProp2GOPPrivate failed.\n", __func__,
			  __LINE__);
		goto ERR;
	}

	ret = mtk_drm_create_properties(pctx);
	if (ret != 0x0) {
		DRM_ERROR("[GOP][%s, %d]: mtk_drm_create_properties failed.\n", __func__,
			  __LINE__);
		goto ERR;
	}

	for (gop_idx = 0; gop_idx < pctx_gop->GopNum; gop_idx++) {
		drmPlaneType =
		    (gop_idx ==
		     (pctx_gop->GopNum - 1)) ? DRM_PLANE_TYPE_CURSOR : DRM_PLANE_TYPE_OVERLAY;
		if (drmPlaneType == DRM_PLANE_TYPE_CURSOR)
			*cursor_plane = &crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx];

		ret =
		    mtk_plane_init(drm_dev, &crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx].base,
				   MTK_PLANE_DISPLAY_PIPE, drmPlaneType);
		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: mtk_plane_init failed.\n", __func__,
				  __LINE__);
			goto ERR;
		}
		crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx].crtc_base = &crtc->base;
		crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx].crtc_private = pctx;
		crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx].zpos = gop_idx;
		crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx].gop_index = gop_idx;
		crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx].mtk_plane_type =
		    MTK_DRM_PLANE_TYPE_GRAPHIC;

		if (mtk_drm_attach_props
		    (pctx_gop, &crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx]) < 0) {
			DRM_ERROR("[GOP][%s, %d]: mtk_drm_attach_props fai.\n", __func__,
				  __LINE__);
			goto ERR;
		}

		pctx_gop->utpGopInit.gop_idx =
		    crtc->plane[MTK_DRM_PLANE_TYPE_GRAPHIC][gop_idx].gop_index;
		if (pctx_gop->utpGopInit.gop_idx == INVALID_NUM) {
			DRM_ERROR("[GOP][%s, %d]: Invalid GOP number 255\n", __func__,
				  __LINE__);
			goto ERR;
		}

		if (pctx_gop->utpGopInit.gop_idx == pctx_gop->nBootlogoGop)
			pctx_gop->utpGopInit.ignore_level = UTP_E_GOP_IGNORE_ALL;
		else
			pctx_gop->utpGopInit.ignore_level = UTP_E_GOP_IGNORE_NORMAL;

		ret = utp_gop_InitByGop(&pctx_gop->utpGopInit);

		if (ret != 0x0) {
			DRM_ERROR("[GOP][%s, %d]: utp_gop_InitByGop failed.\n", __func__,
				  __LINE__);
			goto ERR;
		}
	}

	mtk_drm_gop_CreateSysFile(dev);
	return 0;

ERR:
	clean_gop_context(dev, pctx_gop);
	return ret;
}

int mtk_drm_gop_suspend(struct platform_device *dev, pm_message_t state)
{
	if (utp_gop_power_state(UTP_E_GOP_POWER_SUSPEND) != 0)
		DRM_ERROR("[GOP][%s, %d]: suspend failed.\n", __func__, __LINE__);

	return 0;
}

int mtk_drm_gop_resume(struct platform_device *dev)
{
	if (utp_gop_power_state(UTP_E_GOP_POWER_RESUME) != 0)
		DRM_ERROR("[GOP][%s, %d]: resume failed.\n", __func__, __LINE__);

	return 0;
}
