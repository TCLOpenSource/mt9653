// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <drm/mtk_tv_drm.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>

#include "../mtk_tv_drm_plane.h"
#include "../mtk_tv_drm_gem.h"
#include "mtk_tv_drm_video_display.h"
#include "../mtk_tv_drm_kms.h"
#include "mtk_tv_drm_common.h"

#include "hwreg_render_video_display.h"

#define DRM_PLANE_SCALE_MIN (1<<4)
#define DRM_PLANE_SCALE_MAX (1<<28)

static struct drm_prop_enum_list video_plane_type_enum_list[] = {
	{
		.type = MTK_VPLANE_TYPE_NONE,
		.name = "VIDEO_PLANE_TYPE_NONE"
	},
	{
		.type = MTK_VPLANE_TYPE_MEMC,
		.name = "VIDEO_PLANE_TYPE_MEMC"
	},
	{
		.type = MTK_VPLANE_TYPE_MULTI_WIN,
		.name = "VIDEO_PLANE_TYPE_MULTI_WIN",
	},
	{
		.type = MTK_VPLANE_TYPE_SINGLE_WIN1,
		.name = "VIDEO_PLANE_TYPE_SINGLE_WIN1"
	},
	{
		.type = MTK_VPLANE_TYPE_SINGLE_WIN2,
		.name = "VIDEO_PLANE_TYPE_SINGLE_WIN2"
	},
};

static const struct ext_prop_info ext_video_plane_props_def[] = {
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_MUTE_SCREEN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0x1,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_SNOW_SCREEN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0x1,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_WINDOW_ALPHA,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_WINDOW_R,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_WINDOW_G,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_WINDOW_B,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_BACKGROUND_ALPHA,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_BACKGROUND_R,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_BACKGROUND_G,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_BACKGROUND_B,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = PROP_NAME_VIDEO_PLANE_TYPE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_plane_type_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(video_plane_type_enum_list),
		.init_val = 0x0,
	},
	{
		.prop_name = PROP_NAME_META_FD,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = INT_MAX,
		.init_val = 0x0,
	},
	{
		.prop_name = PROP_NAME_FREEZE,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0x1,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_INPUT_VFREQ,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = INT_MAX,
		.init_val = 0x0,
	},
};

static int _readDTB2VideoPrivate(struct mtk_video_context *pctx_video)
{
	//int ret;
	struct device_node *np;
	//int u32Tmp = 0xFF;
	//const char * name;
	//int * tmpArray = NULL, i;

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_TV_KMS_COMPATIBLE_STR);

	if (np != NULL) {
		DRM_INFO("find compatible node %s!\n",
			MTK_DRM_TV_KMS_COMPATIBLE_STR);
	}

	return 0;
}

static bool _mtk_video_display_is_variable_updated(
	uint64_t oldVar,
	uint64_t newVar)
{
	bool update = 0;

	if (newVar != oldVar)
		update = 1;

	return update;
}

static uint8_t _mtk_video_display_is_ext_prop_updated(
	struct video_plane_prop *plane_props,
	enum ext_video_plane_prop prop)
{
	uint8_t update = 0;

	if (plane_props->prop_val[prop] != plane_props->old_prop_val[prop])
		update = 1;

	DRM_INFO("[%s][%d] ext_prop:%d update:%d\n", __func__, __LINE__,
		prop, update);

	return update;
}

static void _mtk_video_display_check_support_window(
	enum drm_mtk_video_plane_type old_video_plane_type,
	enum drm_mtk_video_plane_type video_plane_type,
	struct mtk_video_context *pctx_video)
{
	if (video_plane_type == MTK_VPLANE_TYPE_MEMC) {
		pctx_video->videoPlaneType_PlaneNum.MEMC_num++;

		if (old_video_plane_type == MTK_VPLANE_TYPE_NONE) {
			//DRM_INFO("[%s][%d] Not belong to any HW plane type\n",
				//__func__, __LINE__);
		} else if (old_video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN)
			pctx_video->videoPlaneType_PlaneNum.MGW_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN1)
			pctx_video->videoPlaneType_PlaneNum.DMA1_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN2)
			pctx_video->videoPlaneType_PlaneNum.DMA2_num--;
		else {
			//DRM_INFO("[%s][%d] Not support video plane type !\n",
				//__func__, __LINE__);
		}
	} else if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
		pctx_video->videoPlaneType_PlaneNum.MGW_num++;
		if (old_video_plane_type == MTK_VPLANE_TYPE_NONE) {
			//DRM_INFO("[%s][%d] Not belong to any HW plane type\n",
				//__func__, __LINE__);
		} else if (old_video_plane_type == MTK_VPLANE_TYPE_MEMC)
			pctx_video->videoPlaneType_PlaneNum.MEMC_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN1)
			pctx_video->videoPlaneType_PlaneNum.DMA1_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN2)
			pctx_video->videoPlaneType_PlaneNum.DMA2_num--;
		else {
			//DRM_INFO("[%s][%d] Not support video plane type !\n",
				//__func__, __LINE__);
		}
	} else if (video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN1) {
		pctx_video->videoPlaneType_PlaneNum.DMA1_num++;

		if (old_video_plane_type == MTK_VPLANE_TYPE_NONE) {
			//DRM_INFO("[%s][%d] Not belong to any HW plane type\n",
				//__func__, __LINE__);
		} else if (old_video_plane_type == MTK_VPLANE_TYPE_MEMC)
			pctx_video->videoPlaneType_PlaneNum.MEMC_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN)
			pctx_video->videoPlaneType_PlaneNum.MGW_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN2)
			pctx_video->videoPlaneType_PlaneNum.DMA2_num--;
		else {
			//DRM_INFO("[%s][%d] Not support video plane type !\n",
				//__func__, __LINE__);
		}
	} else if (video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN2) {
		pctx_video->videoPlaneType_PlaneNum.DMA2_num++;

		if (old_video_plane_type == MTK_VPLANE_TYPE_NONE) {
			//DRM_INFO("[%s][%d] Not belong to any HW plane type\n",
				//__func__, __LINE__);
		} else if (old_video_plane_type == MTK_VPLANE_TYPE_MEMC)
			pctx_video->videoPlaneType_PlaneNum.MEMC_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN)
			pctx_video->videoPlaneType_PlaneNum.MGW_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN1)
			pctx_video->videoPlaneType_PlaneNum.DMA1_num--;
		else {
			//DRM_INFO("[%s][%d] Not support video plane type !\n",
				//__func__, __LINE__);
		}
	} else {
		if (old_video_plane_type == MTK_VPLANE_TYPE_NONE) {
			//DRM_INFO("[%s][%d] Not belong to any HW plane type\n",
				//__func__, __LINE__);
		} else if (old_video_plane_type == MTK_VPLANE_TYPE_MEMC)
			pctx_video->videoPlaneType_PlaneNum.MEMC_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN)
			pctx_video->videoPlaneType_PlaneNum.MGW_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN1)
			pctx_video->videoPlaneType_PlaneNum.DMA1_num--;
		else if (old_video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN2)
			pctx_video->videoPlaneType_PlaneNum.DMA2_num--;
		else {
			//DRM_INFO("[%s][%d] Not support video plane type !\n",
				//__func__, __LINE__);
		}
		DRM_INFO("[%s][%d] Not support video plane type !\n",
			__func__, __LINE__);
	}

	DRM_INFO("[%s][%d]  MEMC_num:%d MGW_num:%d DMA1_num:%d DMA2_num:%d\n",
		__func__, __LINE__,
		pctx_video->videoPlaneType_PlaneNum.MEMC_num,
		pctx_video->videoPlaneType_PlaneNum.MGW_num,
		pctx_video->videoPlaneType_PlaneNum.DMA1_num,
		pctx_video->videoPlaneType_PlaneNum.DMA2_num);
}

static void _mtk_video_display_set_window_mask(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	bool enable)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;

	struct reg_info reg[reg_num];
	struct hwregWindowMaskIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregWindowMaskIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.planeType = video_plane_type;
	paramIn.windowMask = (enable << plane_inx);

	DRM_INFO("[%s][%d] plane_inx:%d video_plane_type:%d\n",
		__func__, __LINE__, plane_inx, video_plane_type);

	if ((video_plane_type > MTK_VPLANE_TYPE_NONE) ||
		(video_plane_type < MTK_VPLANE_TYPE_MAX)) {

		if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
			paramIn.MGWPlaneNum = pctx_video->videoPlaneType_PlaneNum.MGW_num;

			DRM_INFO("[%s][%d] MGW_NUM:%d\n",
				__func__, __LINE__, paramIn.MGWPlaneNum);
		}
	} else {
		DRM_INFO("[%s][%d] Invalid  HW plane type\n",
			__func__, __LINE__);
	}

	DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d windowMask:%d MGWNum:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.planeType,
		paramIn.windowMask, paramIn.MGWPlaneNum);

	drv_hwreg_render_display_set_windowMask(paramIn, &paramOut);

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);
}

static void _mtk_video_display_set_crop_win(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct mtk_plane_state *state)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;

	struct window cropwindow = {{0, 0, 0, 0} };
	static struct window oldcropwindow = {{0, 0, 0, 0} };

	struct reg_info reg[reg_num];
	struct hwregCropWinIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregCropWinIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.planeType = video_plane_type;
	paramIn.windowID = plane_inx;
	cropwindow.x = state->base.src_x >> 16;
	cropwindow.y = state->base.src_y >> 16;
	cropwindow.w = state->base.src_w >> 16;
	cropwindow.h = state->base.src_h >> 16;

	if ((_mtk_video_display_is_variable_updated(oldcropwindow.x, cropwindow.x)) ||
		(_mtk_video_display_is_variable_updated(oldcropwindow.y, cropwindow.y)) ||
		(_mtk_video_display_is_variable_updated(oldcropwindow.w, cropwindow.w)) ||
		(_mtk_video_display_is_variable_updated(oldcropwindow.h, cropwindow.h))
	) {
		DRM_INFO("[%s][%d] plane_inx:%d old crop win:[%d, %d, %d, %d]\n"
			, __func__, __LINE__, plane_inx,
			oldcropwindow.x, oldcropwindow.y,
			oldcropwindow.w, oldcropwindow.h);

		DRM_INFO("[%s][%d] plane_inx:%d crop win:[%d, %d, %d, %d]\n",
			__func__, __LINE__, plane_inx,
			cropwindow.x, cropwindow.y,
			cropwindow.w, cropwindow.h);

		DRM_INFO("[%s][%d] video_plane_type:%d\n",
			__func__, __LINE__, video_plane_type);

		if ((video_plane_type > MTK_VPLANE_TYPE_NONE) ||
			(video_plane_type < MTK_VPLANE_TYPE_MAX)) {

			if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
				paramIn.MGWPlaneNum = pctx_video->videoPlaneType_PlaneNum.MGW_num;

				DRM_INFO("[%s][%d] MGW_NUM:%d\n",
					__func__, __LINE__, paramIn.MGWPlaneNum);
			}
		} else {
			DRM_INFO("[%s][%d] Invalid  HW plane type\n",
				__func__, __LINE__);
		}

		paramIn.cropWindow.x = cropwindow.x;
		paramIn.cropWindow.y = cropwindow.y;
		paramIn.cropWindow.w = cropwindow.w;
		paramIn.cropWindow.h = cropwindow.h;

		DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d windowID:%d MGWNum:%d\n",
			__func__, __LINE__,
			paramIn.RIU, paramIn.planeType,
			paramIn.windowID, paramIn.MGWPlaneNum);

		DRM_INFO("[%s][%d][paramIn] crop win(x,y,w,h):[%d, %d, %d, %d]\n",
			__func__, __LINE__,
			paramIn.cropWindow.x, paramIn.cropWindow.y,
			paramIn.cropWindow.w, paramIn.cropWindow.h);

		drv_hwreg_render_display_set_cropWindow(paramIn, &paramOut);

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		DRM_INFO("[%s][%d][paramOut] addr:%d\n",
			__func__, __LINE__, paramOut.reg->addr);

		DRM_INFO("[%s][%d][paramOut] mask:%d\n",
			__func__, __LINE__, paramOut.reg->mask);

		oldcropwindow.x = cropwindow.x;
		oldcropwindow.y = cropwindow.y;
		oldcropwindow.w = cropwindow.w;
		oldcropwindow.h = cropwindow.h;
	}
}

#define H_ScalingRatio(Input, Output) \
	{ result = ((uint64_t)(Input)) * 2097152ul; do_div(result, (Output)); \
	result += 1; do_div(result, 2); }
#define H_ScalingUpPhase(Input, Output) \
	{ Phaseresult = ((uint64_t)(Input)) * 1048576ul; \
	do_div(Phaseresult, (Output));    Phaseresult += 1048576ul; \
	Phaseresult += 1; do_div(Phaseresult, 2); }
#define H_ScalingDownPhase(Input, Output)\
	{ Phaseresult = ((uint64_t)(Input)) * 1048576ul;\
	do_div(Phaseresult, (Output));  Phaseresult = 1048576ul - Phaseresult;\
	Phaseresult += 1; do_div(Phaseresult, 2); }
#define V_ScalingRatio(Input, Output) \
	{ result = ((uint64_t)(Input)) * 2097152ul; do_div(result, (Output));\
	result += 1; do_div(result, 2); }
#define V_ScalingUpPhase(Input, Output)\
	{ Phaseresult = ((uint64_t)(Input)) * 1048576ul;\
	do_div(Phaseresult, (Output));    Phaseresult += 1048576ul;\
	Phaseresult += 1; do_div(Phaseresult, 2); }
#define V_ScalingDownPhase(Input, Output)\
	{ Phaseresult = ((uint64_t)(Input)) * 1048576ul;\
	do_div(Phaseresult, (Output));  Phaseresult = Phaseresult - 1048576ul;\
	Phaseresult += 1; do_div(Phaseresult, 2); }

static void _mtk_video_set_scaling_ratio(
	unsigned int plane_inx,
	struct mtk_plane_state *state)
{
	uint64_t result = 0, Phaseresult = 0;
	uint32_t TmpRatio = 0;
	uint32_t TmpFactor = 0;
	uint32_t HorIniFactor = 0, VerIniFactor = 0;
	uint32_t HorRatio = 0, VerRatio = 0;
	uint32_t Thinfactor = 0;
	uint64_t tmpheight = 0;
	uint8_t IsScalingUp_H = 0, IsScalingUp_V = 0;
	static uint64_t olddisp_width, olddisp_height;
	static uint64_t disp_x, disp_y, disp_width, disp_height;
	static uint64_t oldcrop_width, oldcrop_height;
	static uint64_t crop_x, crop_y, crop_width, crop_height;

	disp_x = state->base.crtc_x;
	disp_y = state->base.crtc_y;
	disp_width = state->base.crtc_w;
	disp_height = state->base.crtc_h;
	crop_x = state->base.src_x >> 16;
	crop_y = state->base.src_y >> 16;
	crop_width = state->base.src_w >> 16;
	crop_height = state->base.src_h >> 16;
	if ((_mtk_video_display_is_variable_updated(oldcrop_width, crop_width)) ||
		(_mtk_video_display_is_variable_updated(oldcrop_height, crop_height)) ||
		(_mtk_video_display_is_variable_updated(olddisp_width, disp_width)) ||
		(_mtk_video_display_is_variable_updated(olddisp_height, disp_height))
	) {
		DRM_INFO("[%s][%d] plane_inx:%d crop win:[%lld, %lld, %lld, %lld]\n"
			, __func__, __LINE__, plane_inx,
			crop_x, crop_y, crop_width, crop_height);
		DRM_INFO("[%s][%d] plane_inx:%d disp win:[%lld, %lld, %lld, %lld]\n"
			, __func__, __LINE__, plane_inx,
			disp_x, disp_y, disp_width, disp_height);
		//-----------------------------------------
		// Horizontal
		//-----------------------------------------
		if (crop_width != disp_width) {
			H_ScalingRatio(crop_width, disp_width);
			TmpRatio = (uint32_t)result;
			TmpRatio &= 0xFFFFFFL;
		} else {
			TmpRatio = 0x100000L;
		}
		HorRatio = TmpRatio;
		/// Set Phase Factor
		if (disp_width > crop_width) { // Scale Up
			H_ScalingUpPhase(crop_width, disp_width);
			TmpFactor = (uint32_t)Phaseresult;
			IsScalingUp_H = 1;
		} else {
			H_ScalingDownPhase(crop_width, disp_width);
			TmpFactor = (uint32_t)Phaseresult;
			IsScalingUp_H = 0;
		}
		HorIniFactor = TmpFactor;
		DRM_INFO("[%s][%d] H_ratio:%x, H_initfactor: %x, ScalingUp %d\n"
			, __func__, __LINE__, HorRatio,
			HorIniFactor, IsScalingUp_H);
		//Vertical
		//Thin Read
		if (crop_height <= disp_height) {
			Thinfactor = 1;
		} else {
			if ((crop_height%disp_height) == 0)
				Thinfactor = (crop_height/disp_height);
			else
				Thinfactor = (((crop_height/disp_height)+2)>>1)<<1;
		}
		//Thinfactor = ((crop_height/disp_height)>>1)<<1;
		tmpheight = crop_height/Thinfactor;
		DRM_INFO("[%s][%d] Thin Read:%d, Height after Thin read: %d\n"
			, __func__, __LINE__, Thinfactor,
			tmpheight);
		if (tmpheight == disp_height) {
			TmpRatio = 0x100000L;
		} else {
			V_ScalingRatio(tmpheight, disp_height);
			TmpRatio = (uint32_t)result;
		}
		VerRatio = TmpRatio;
		if (disp_height > tmpheight) { // Scale Up
			V_ScalingUpPhase(tmpheight, disp_height);
			TmpFactor = (uint32_t)Phaseresult;
			IsScalingUp_V = 1;
		} else {
			V_ScalingDownPhase(tmpheight, disp_height);
			TmpFactor = (uint32_t)Phaseresult;
			IsScalingUp_V = 0;
		}
		VerIniFactor =  TmpFactor;
		DRM_INFO("[%s][%d] V_ratio:%x, V_inifactor: %x, ScalingUp %d\n"
			, __func__, __LINE__, VerRatio,
			VerIniFactor, IsScalingUp_V);
		olddisp_width = disp_width;
		olddisp_height = disp_height;
		oldcrop_width = crop_width;
		oldcrop_height = crop_height;
	}
}

static void _mtk_video_display_set_disp_win(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct mtk_plane_state *state)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;

	struct window dispwindow = {{0, 0, 0, 0} };
	static struct window olddispwindow = {0};
	struct windowRect dispwindowRect = {{0, 0, 0, 0} };

	struct reg_info reg[reg_num];
	struct hwregDispWinIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregCropWinIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.planeType = video_plane_type;
	paramIn.windowID = plane_inx;

	dispwindow.x = state->base.crtc_x;
	dispwindow.y = state->base.crtc_y;
	dispwindow.w = state->base.crtc_w;
	dispwindow.h = state->base.crtc_h;

	if ((_mtk_video_display_is_variable_updated(olddispwindow.x, dispwindow.x)) ||
		(_mtk_video_display_is_variable_updated(olddispwindow.y, dispwindow.y)) ||
		(_mtk_video_display_is_variable_updated(olddispwindow.w, dispwindow.w)) ||
		(_mtk_video_display_is_variable_updated(olddispwindow.h, dispwindow.h))
	) {
		DRM_INFO("[%s][%d] plane_inx:%d old disp win:[%d, %d, %d, %d]\n"
			, __func__, __LINE__, plane_inx,
			olddispwindow.x, olddispwindow.y,
			olddispwindow.w, olddispwindow.h);

		DRM_INFO("[%s][%d] plane_inx:%d disp win:[%d, %d, %d, %d]\n",
			__func__, __LINE__, plane_inx,
			dispwindow.x, dispwindow.y,
			dispwindow.w, dispwindow.h);

		DRM_INFO("[%s][%d] video_plane_type:%d\n",
			__func__, __LINE__, video_plane_type);

		paramIn.dispWindowRect.h_start = state->base.crtc_x;

		if (state->base.crtc_x + state->base.crtc_w >= 1) {
			paramIn.dispWindowRect.h_end =
				state->base.crtc_x + state->base.crtc_w - 1;
		}

		paramIn.dispWindowRect.v_start = state->base.crtc_y;

		if (state->base.crtc_y + state->base.crtc_h >= 1) {
			paramIn.dispWindowRect.v_end =
				state->base.crtc_y + state->base.crtc_h - 1;
		}

		DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d, windowID:%d\n",
			__func__, __LINE__,
			paramIn.RIU, paramIn.planeType, paramIn.windowID);

		DRM_INFO("[%s][%d][paramIn] disp rect(h_s,h_e,v_s,v_e):[%d, %d, %d, %d]\n",
			__func__, __LINE__,
			paramIn.dispWindowRect.h_start, paramIn.dispWindowRect.h_end,
			paramIn.dispWindowRect.v_start, paramIn.dispWindowRect.v_end);

		drv_hwreg_render_display_set_dispWindow(paramIn, &paramOut);

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		olddispwindow.x = dispwindow.x;
		olddispwindow.y = dispwindow.y;
		olddispwindow.w = dispwindow.w;
		olddispwindow.h = dispwindow.h;
	}
}

static void _mtk_video_display_set_fb_memory_format(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct drm_framebuffer *fb)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;
	struct drm_format_name_buf format_name;

	struct hwregMemConfigIn paramIn;
	struct reg_info reg[reg_num];

	struct hwregOut paramOut;
	uint16_t RegCount = 0;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregMemConfigIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.planeType = video_plane_type;
	paramIn.windowID = plane_inx;
	paramIn.fourcc = fb->format->format;
	paramIn.modifier =
		(((fb->modifier >> 56) & 0xF) == DRM_FORMAT_MOD_VENDOR_QCOM) ? 1 : 0;
	paramIn.modifier_compressed =
		((fb->modifier) == DRM_FORMAT_MOD_QCOM_COMPRESSED) ? 1 : 0;

	if ((video_plane_type > MTK_VPLANE_TYPE_NONE) ||
		(video_plane_type < MTK_VPLANE_TYPE_MAX)) {

		if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
			paramIn.MGWPlaneNum = pctx_video->videoPlaneType_PlaneNum.MGW_num;

			DRM_INFO("[%s][%d] MGW_NUM:%d\n",
				__func__, __LINE__, paramIn.MGWPlaneNum);
		}
	} else {
		DRM_INFO("[%s][%d] Invalid  HW plane type\n",
			__func__, __LINE__);
	}

	DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d windowID:%d MGWNum:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.planeType,
		paramIn.windowID, paramIn.MGWPlaneNum);

	DRM_INFO("[%s][%d][paramIn] fourcc:%s fourcc:(0x%08x) modifier:%u modifier_compressed:%u\n",
		__func__, __LINE__,
		drm_get_format_name(fb->format->format,	&format_name),
		paramIn.fourcc, paramIn.modifier, paramIn.modifier_compressed);

	drv_hwreg_render_display_set_memConfig(paramIn, &paramOut);

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);
}

static void _mtk_video_display_set_fb(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct drm_framebuffer *fb)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;

	static uint64_t oldwidth;
	static uint64_t oldheight;

	uint16_t plane_num_per_frame = 1;//tmp define
	int i = 0;

	struct drm_gem_object *gem;
	struct mtk_drm_gem_obj *mtk_gem;
	dma_addr_t addr[4];

	struct reg_info reg[reg_num];
	struct hwregFrameSizeIn paramFrameSizeIn;
	struct hwregLineOffsetIn paramLineOffsetIn;
	struct hwregFrameOffsetIn paramFrameOffsetIn;
	struct hwregFrameNumIn paramFrameNumIn;
	struct hwregBaseAddrIn paramBaseAddrIn;

	struct hwregOut paramOut;
	uint16_t RegCount = 0;

	memset(addr, 0, sizeof(addr));

	memset(reg, 0, sizeof(reg));
	memset(&paramFrameSizeIn, 0, sizeof(struct hwregFrameSizeIn));
	memset(&paramLineOffsetIn, 0, sizeof(struct hwregLineOffsetIn));
	memset(&paramFrameOffsetIn, 0, sizeof(struct hwregFrameOffsetIn));
	memset(&paramFrameNumIn, 0, sizeof(struct hwregFrameNumIn));
	memset(&paramBaseAddrIn, 0, sizeof(struct hwregBaseAddrIn));

	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramFrameSizeIn.RIU = 1;
	paramFrameSizeIn.hSize = fb->width;
	paramFrameSizeIn.vSize = fb->height;

	paramLineOffsetIn.RIU = 1;
	paramLineOffsetIn.planeType = video_plane_type;
	paramLineOffsetIn.windowID = plane_inx;
	paramLineOffsetIn.lineOffset = fb->width;

	paramFrameOffsetIn.RIU = 1;
	paramFrameOffsetIn.planeType = video_plane_type;
	paramFrameOffsetIn.windowID = plane_inx;
	paramFrameOffsetIn.frameOffset =
		paramLineOffsetIn.lineOffset * fb->height;

	paramFrameNumIn.RIU = 1;
	paramFrameNumIn.planeType = video_plane_type;
	paramFrameNumIn.frameNum = 1; //for sw mode

	paramBaseAddrIn.RIU = 1;
	paramBaseAddrIn.planeType = video_plane_type;
	paramBaseAddrIn.windowID = plane_inx;

	if (_mtk_video_display_is_variable_updated(oldwidth, fb->width) ||
		_mtk_video_display_is_variable_updated(oldheight, fb->height)) {

		DRM_INFO("[%s][%d] plane_inx:%d fb_width:%d fb_height:%d\n",
			__func__, __LINE__, plane_inx, fb->width, fb->height);

		if (video_plane_type == MTK_VPLANE_TYPE_NONE) {
			DRM_INFO("[%s][%d] Not belong to any HW plane type\n",
				__func__, __LINE__);
		} else {
			if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
				paramLineOffsetIn.MGWPlaneNum =
					pctx_video->videoPlaneType_PlaneNum.MGW_num;
				paramFrameOffsetIn.MGWPlaneNum =
					pctx_video->videoPlaneType_PlaneNum.MGW_num;
				paramFrameNumIn.MGWPlaneNum =
					pctx_video->videoPlaneType_PlaneNum.MGW_num;
				paramBaseAddrIn.MGWPlaneNum =
					pctx_video->videoPlaneType_PlaneNum.MGW_num;
				DRM_INFO("[%s][%d] MGW_NUM:%d\n",
					__func__, __LINE__,
					pctx_video->videoPlaneType_PlaneNum.MGW_num);
			}
		}
		//========================================================================//
		DRM_INFO("[%s][%d][paramIn] RIU:%d hSize:%d vSize:%d\n",
			__func__, __LINE__,
			paramBaseAddrIn.RIU,
			paramFrameSizeIn.hSize,
			paramFrameSizeIn.vSize);

		drv_hwreg_render_display_set_frameSize(
			paramFrameSizeIn, &paramOut);

			RegCount = paramOut.regCount;
			paramOut.reg = reg + RegCount;

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);
		//========================================================================//
		DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d\n",
			__func__, __LINE__,
			paramLineOffsetIn.RIU,
			paramLineOffsetIn.planeType);

		DRM_INFO("[%s][%d][paramIn] windowID:%d MGWNum:%d lineOffset:%d\n",
			__func__, __LINE__,
			paramLineOffsetIn.windowID,
			paramLineOffsetIn.MGWPlaneNum,
			paramLineOffsetIn.lineOffset);

		drv_hwreg_render_display_set_lineOffset(
			paramLineOffsetIn, &paramOut);

			RegCount = RegCount + paramOut.regCount;
			paramOut.reg = reg + RegCount;

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);
		//=====================================================================//
		DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d\n",
			__func__, __LINE__,
			paramFrameOffsetIn.RIU,
			paramFrameOffsetIn.planeType);

		DRM_INFO("[%s][%d][paramIn] windowID:%d MGWNum:%d frameOffset:0x%x\n",
			__func__, __LINE__,
			paramFrameOffsetIn.windowID,
			paramFrameOffsetIn.MGWPlaneNum,
			paramFrameOffsetIn.frameOffset);

		drv_hwreg_render_display_set_frameOffset(
			paramFrameOffsetIn, &paramOut);

			RegCount = RegCount + paramOut.regCount;
			paramOut.reg = reg + RegCount;

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);
		//=====================================================================//
		DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d MGWNum:%d frameNum:%d\n",
			__func__, __LINE__,
			paramFrameNumIn.RIU,
			paramFrameNumIn.planeType,
			paramFrameNumIn.MGWPlaneNum,
			paramFrameNumIn.frameNum);

		drv_hwreg_render_display_set_frameNum(
			paramFrameNumIn, &paramOut);

			RegCount = RegCount + paramOut.regCount;
			paramOut.reg = reg + RegCount;

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);
		//=====================================================================//
		oldwidth = (uint64_t)fb->width;
		oldheight = (uint64_t)fb->height;
	}

	for (i = 0; i < plane_num_per_frame; i++) {
		gem = fb->obj[i];
		mtk_gem = to_mtk_gem_obj(gem);
		addr[i] = mtk_gem->dma_addr;

		DRM_INFO("[%s][%d] addr:0x%lx\n",
			__func__, __LINE__, addr[i]);
	}
	//========================================================================//
	paramBaseAddrIn.addr.plane0 = addr[0];
	paramBaseAddrIn.addr.plane1 = addr[1];
	paramBaseAddrIn.addr.plane2 = addr[2];

	DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d windowID:%d MGWNum:%d frameNum:%d\n",
		__func__, __LINE__,
		paramBaseAddrIn.RIU,
		paramBaseAddrIn.planeType,
		paramBaseAddrIn.windowID,
		paramBaseAddrIn.MGWPlaneNum);

	DRM_INFO("[%s][%d][paramIn] plane0_addr:0x%lx plane1_addr:0x%lx plane2_addr:0x%lx\n",
		__func__, __LINE__,
		paramBaseAddrIn.addr.plane0,
		paramBaseAddrIn.addr.plane1,
		paramBaseAddrIn.addr.plane2);

	drv_hwreg_render_display_set_baseAddr(
		paramBaseAddrIn, &paramOut);

	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);

	//========================================================================//
}

static void _mtk_video_display_set_windowARGB(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;

	//struct ARGB windowARGB = {{0,0,0,0}};

	struct reg_info reg[reg_num];
	struct hwregPostFillWindowARGBIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregPostFillWindowARGBIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.planeType = video_plane_type;
	paramIn.windowMask = (1 << plane_inx);
	paramIn.windowARGB.alpha = plane_props->prop_val[EXT_VPLANE_PROP_WINDOW_ALPHA];
	paramIn.windowARGB.R = plane_props->prop_val[EXT_VPLANE_PROP_WINDOW_R];
	paramIn.windowARGB.G = plane_props->prop_val[EXT_VPLANE_PROP_WINDOW_G];
	paramIn.windowARGB.B = plane_props->prop_val[EXT_VPLANE_PROP_WINDOW_B];

	DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d, windowMask:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.planeType, paramIn.windowMask);

	DRM_INFO("[%s][%d][paramIn]window ARGB:[%llx, %llx, %llx, %llx]\n",
		__func__, __LINE__,
		paramIn.windowARGB.alpha, paramIn.windowARGB.R,
		paramIn.windowARGB.G, paramIn.windowARGB.B);

	drv_hwreg_render_display_set_postFillWindowARGB(paramIn, &paramOut);

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);
}

static void _mtk_video_display_set_BackGroundARGB(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;

	//struct ARGB bgARGB = {{0,0,0,0}};
	struct reg_info reg[reg_num];
	struct hwregPostFillBgARGBIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregPostFillBgARGBIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.planeType = video_plane_type;
	paramIn.windowMask = (1 << plane_inx);
	paramIn.bgARGB.alpha = plane_props->prop_val[EXT_VPLANE_PROP_BACKGROUND_ALPHA];
	paramIn.bgARGB.R = plane_props->prop_val[EXT_VPLANE_PROP_BACKGROUND_R];
	paramIn.bgARGB.G = plane_props->prop_val[EXT_VPLANE_PROP_BACKGROUND_G];
	paramIn.bgARGB.B = plane_props->prop_val[EXT_VPLANE_PROP_BACKGROUND_B];

	DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d, windowMask:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.planeType, paramIn.windowMask);

	DRM_INFO("[%s][%d][paramIn]BG ARGB:[%llx, %llx, %llx, %llx]\n",
		__func__, __LINE__,
		paramIn.bgARGB.alpha, paramIn.bgARGB.R,
		paramIn.bgARGB.G, paramIn.bgARGB.B);

	drv_hwreg_render_display_set_postFillBgARGB(paramIn, &paramOut);

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);
}

static void _mtk_video_display_set_zpos(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct mtk_plane_state *state)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];

	uint16_t reg_num = pctx_video->reg_num;
	uint8_t src_num = pctx_video->videoPlaneType_TypeNum;
	uint8_t src_index = 0;
	bool layerUpdate[src_num];
	bool layerEn[src_num];
	uint8_t layer_index[src_num];
	static uint64_t oldzpos;
	uint8_t index = 0;
	uint16_t RegCount = 0;

	struct reg_info reg[reg_num];
	struct hwregLayerControlIn paramIn0;
	struct hwregLayerControlIn paramIn1;
	struct hwregLayerControlIn paramIn2;
	struct hwregLayerControlIn paramIn3;

	struct hwregOut paramOut;

	memset(layerUpdate, 0, sizeof(layerUpdate));
	memset(layerEn, 0, sizeof(layerEn));
	memset(layer_index, 0, sizeof(layer_index));

	memset(reg, 0, sizeof(reg));
	memset(&paramIn0, 0, sizeof(struct hwregLayerControlIn));
	memset(&paramIn1, 0, sizeof(struct hwregLayerControlIn));
	memset(&paramIn2, 0, sizeof(struct hwregLayerControlIn));
	memset(&paramIn3, 0, sizeof(struct hwregLayerControlIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	DRM_INFO("[%s][%d] plane_inx:%d video_plane_type:%d oldzpos:%d newzpos:%d\n",
		__func__, __LINE__, plane_inx, video_plane_type,
		oldzpos, state->base.zpos);

	if (_mtk_video_display_is_variable_updated(oldzpos, state->base.zpos)) {

		switch (video_plane_type) {
		case MTK_VPLANE_TYPE_MEMC:
			src_index = 0;
			layerUpdate[src_index] = 1;
			layerEn[src_index] = 1;
			layer_index[src_index] = state->base.zpos;

			if (pctx_video->videoPlaneType_PlaneNum.MGW_num == 0)
				layerEn[1] = 0;
			if (pctx_video->videoPlaneType_PlaneNum.DMA1_num == 0)
				layerEn[2] = 0;
			if (pctx_video->videoPlaneType_PlaneNum.DMA2_num == 0)
				layerEn[3] = 0;
		break;
		case MTK_VPLANE_TYPE_MULTI_WIN:
			src_index = 1;
			layerUpdate[src_index] = 1;
			layerEn[src_index] = 1;
			layer_index[src_index] = state->base.zpos;

			if (pctx_video->videoPlaneType_PlaneNum.MEMC_num == 0) {
				layerUpdate[0] = 1;
				layerEn[0] = 0;
			}
			if (pctx_video->videoPlaneType_PlaneNum.DMA1_num == 0) {
				layerUpdate[2] = 1;
				layerEn[2] = 0;
			}
			if (pctx_video->videoPlaneType_PlaneNum.DMA2_num == 0) {
				layerUpdate[3] = 1;
				layerEn[3] = 0;
			}
		break;
		case MTK_VPLANE_TYPE_SINGLE_WIN1:
			src_index = 2;
			layerUpdate[src_index] = 1;
			layerEn[src_index] = 1;
			layer_index[src_index] = state->base.zpos;

			if (pctx_video->videoPlaneType_PlaneNum.MEMC_num == 0) {
				layerEn[0] = 0;
				layerUpdate[0] = 1;
			}
			if (pctx_video->videoPlaneType_PlaneNum.MGW_num == 0) {
				layerEn[1] = 0;
				layerUpdate[1] = 1;
			}
			if (pctx_video->videoPlaneType_PlaneNum.DMA2_num == 0) {
				layerEn[3] = 0;
				layerUpdate[3] = 1;
			}
		break;
		case MTK_VPLANE_TYPE_SINGLE_WIN2:
			src_index = 3;
			layerUpdate[src_index] = 1;
			layerEn[src_index] = 1;
			layer_index[src_index] = state->base.zpos;

			if (pctx_video->videoPlaneType_PlaneNum.MEMC_num == 0) {
				layerUpdate[0] = 1;
				layerEn[0] = 0;
			}

			if (pctx_video->videoPlaneType_PlaneNum.MGW_num == 0) {
				layerUpdate[1] = 1;
				layerEn[1] = 0;
			}

			if (pctx_video->videoPlaneType_PlaneNum.DMA1_num == 0) {
				layerUpdate[2] = 1;
				layerEn[2] = 0;
			}
		break;
		default:
		break;
	}

	for (index; index < src_num; index++) {
		if (layerUpdate[index] == 1) {
			if (layer_index[index] == 0) {
				paramIn0.RIU = 1;
				paramIn0.layerEn = layerEn[index];
				paramIn0.srcIndex = index;

				DRM_INFO("[%s][%d][paramIn0] RIU:%d\n",
				__func__, __LINE__, paramIn0.RIU);

				DRM_INFO("[%s][%d][paramIn0] layerEn:%d, srcIndex:%d\n",
				__func__, __LINE__,
				paramIn0.layerEn, paramIn0.srcIndex);

				drv_hwreg_render_display_set_layer0Control(
				paramIn0, &paramOut);

				RegCount = paramOut.regCount;
				paramOut.reg = reg + RegCount;

				DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
					__func__, __LINE__, paramOut.regCount);
			} else if (layer_index[index] == 1) {
				paramIn1.RIU = 1;
				paramIn1.layerEn = layerEn[index];
				paramIn1.srcIndex = index;

				DRM_INFO("[%s][%d][paramIn1] RIU:%d\n",
				__func__, __LINE__, paramIn0.RIU);

				DRM_INFO("[%s][%d][paramIn1] layerEn:%d, srcIndex:%d\n",
				__func__, __LINE__,
				paramIn0.layerEn, paramIn0.srcIndex);

				drv_hwreg_render_display_set_layer1Control(
				paramIn1, &paramOut);

				RegCount = paramOut.regCount;
				paramOut.reg = reg + RegCount;

			DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
					__func__, __LINE__, paramOut.regCount);
			} else if (layer_index[index] == 2) {
				paramIn2.RIU = 1;
				paramIn2.layerEn = layerEn[index];
				paramIn2.srcIndex = index;

			DRM_INFO("[%s][%d][paramIn2] RIU:%d layerEn:%d, srcIndex:%d\n",
				__func__, __LINE__,
				paramIn2.RIU, paramIn2.layerEn,
				paramIn2.srcIndex);

				drv_hwreg_render_display_set_layer2Control(
				paramIn2, &paramOut);

				RegCount = paramOut.regCount;
				paramOut.reg = reg + RegCount;

				DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
					__func__, __LINE__, paramOut.regCount);
			} else {//layer_index[index] =3
				paramIn3.RIU = 1;
				paramIn3.layerEn = layerEn[index];
				paramIn3.srcIndex = index;

				DRM_INFO("[%s][%d][paramIn3] RIU:%d\n",
				__func__, __LINE__, paramIn3.RIU);

				DRM_INFO("[%s][%d][paramIn3] layerEn:%d, srcIndex:%d\n",
				__func__, __LINE__,
				paramIn3.layerEn, paramIn3.srcIndex);

				drv_hwreg_render_display_set_layer3Control(
				paramIn3, &paramOut);

				RegCount = paramOut.regCount;
				paramOut.reg = reg + RegCount;

				DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
					__func__, __LINE__, paramOut.regCount);
			}
		}
	}

	oldzpos = (uint64_t) state->base.zpos;
	}
}

static void _mtk_video_display_set_mute(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;

	struct reg_info reg[reg_num];
	struct hwregPostFillMuteScreenIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregPostFillMuteScreenIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.planeType = video_plane_type;
	paramIn.windowMask = (1 << plane_inx);
	paramIn.muteEn = (plane_props->prop_val[EXT_VPLANE_PROP_MUTE_SCREEN]);
	paramIn.muteEn = (paramIn.muteEn << plane_inx);
	paramIn.muteSWModeEn = (1 << plane_inx);

	DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d, windowMask:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.planeType, paramIn.windowMask);

	DRM_INFO("[%s][%d][paramIn] muteEn:%d muteSWModeEn:%d\n",
		__func__, __LINE__,
		paramIn.muteEn, paramIn.muteSWModeEn);

	drv_hwreg_render_display_set_postFillMuteScreen(paramIn, &paramOut);

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);
}


static void _mtk_video_display_clean_video_context
	(struct device *dev,
	struct mtk_video_context *video_pctx)
{
	if (video_pctx) {
		kfree(video_pctx->video_plane_properties);
		video_pctx->video_plane_properties = NULL;
	}
}

static int _mtk_video_display_create_plane_props(struct mtk_tv_kms_context *pctx)
{
	struct drm_property *prop;
	int extend_prop_count = ARRAY_SIZE(ext_video_plane_props_def);
	const struct ext_prop_info *ext_prop;
	struct drm_device *drm_dev = pctx->drm_dev;
	int i;
	struct mtk_video_context *pctx_video = &pctx->video_priv;

	// create extend common plane properties
	for (i = 0; i < extend_prop_count; i++) {
		ext_prop = &ext_video_plane_props_def[i];
		if (ext_prop->flag & DRM_MODE_PROP_ENUM) {
			prop = drm_property_create_enum(
				drm_dev,
				ext_prop->flag,
				ext_prop->prop_name,
				ext_prop->enum_info.enum_list,
				ext_prop->enum_info.enum_length);
			if (prop == NULL) {
				DRM_INFO("[%s][%d]create enum fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}
		} else if (ext_prop->flag & DRM_MODE_PROP_RANGE) {
			prop = drm_property_create_range(
				drm_dev,
				ext_prop->flag,
				ext_prop->prop_name,
				ext_prop->range_info.min,
				ext_prop->range_info.max);
			if (prop == NULL) {
				DRM_INFO("[%s][%d]create range fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}
		} else {
			DRM_INFO("[%s][%d]unknown prop flag 0x%x !!\n",
				__func__, __LINE__, ext_prop->flag);
			return -EINVAL;
		}

		// add created props to context
		pctx_video->drm_video_plane_prop[i] = prop;
	}

	return 0;
}

static void _mtk_video_display_attach_plane_props(
	struct mtk_tv_kms_context *pctx,
	struct mtk_drm_plane *mplane)
{
	int extend_prop_count = ARRAY_SIZE(ext_video_plane_props_def);
	struct drm_plane *plane_base = &mplane->base;
	struct drm_mode_object *plane_obj = &plane_base->base;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	int plane_inx = mplane->video_index;
	const struct ext_prop_info *ext_prop;
	int i;

	for (i = 0; i < extend_prop_count; i++) {
		ext_prop = &ext_video_plane_props_def[i];
		drm_object_attach_property(plane_obj,
			pctx_video->drm_video_plane_prop[i],
			ext_prop->init_val);
		(pctx_video->video_plane_properties + plane_inx)->prop_val[i] =
			ext_prop->init_val;
	}

}

static void _mtk_video_display_set_aid(
	struct mtk_video_context *pctx_video,
	unsigned int plane_idx,
	uint8_t access_id)
{
	uint16_t reg_num = pctx_video->reg_num;

	struct reg_info reg[reg_num];
	struct hwregAidTableIn paramAidTableIn;
	struct hwregOut paramOut;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_idx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];

	memset(reg, 0, sizeof(reg));
	memset(&paramAidTableIn, 0, sizeof(struct hwregAidTableIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramAidTableIn.RIU = 1;
	paramAidTableIn.planeType = video_plane_type;
	paramAidTableIn.windowID = plane_idx;
	paramAidTableIn.access_id = access_id;

	DRM_INFO("[%s][%d] video_plane_type:%d, windowID:%d, aid:%d\n",
		__func__, __LINE__,
		video_plane_type, plane_idx, access_id);

	drv_hwreg_render_display_set_aidtable(paramAidTableIn, &paramOut);

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);

}

static void _mtk_video_display_check_meta(
	struct mtk_video_context *pctx_video,
	unsigned int plane_idx,
	uint64_t meta_fd)
{
	int meta_ret = 0;
	struct m_rd_display_frm_info frameInfo;
	struct meta_pq_disp_svp svpInfo;

	DRM_INFO("[%s][%d] plane %d: meta_fd update to %td\n",
		__func__, __LINE__, plane_idx, (ptrdiff_t)meta_fd);

	memset(&frameInfo, 0, sizeof(struct m_rd_display_frm_info));
	memset(&svpInfo, 0, sizeof(struct meta_pq_disp_svp));

	// check frame info
	meta_ret = mtk_render_common_read_metadata((int)meta_fd,
		RD_METATAG_FRM_INFO, (void *)&frameInfo);
	if (meta_ret) {
		DRM_INFO("[%s][%d] metadata do not has frm_info tag\n",
			__func__, __LINE__);
	} else {
		DRM_INFO("[%s][%d] %s %s: %td, %s: %td,	%s: %td, %s: %td\n",
			__func__, __LINE__,
			"metadata frm_info",
			"crop_win_x:", (ptrdiff_t)frameInfo.crop_win_x,
			"crop_win_y:", (ptrdiff_t)frameInfo.crop_win_y,
			"crop_win_w", (ptrdiff_t)frameInfo.crop_win_w,
			"crop_win_h", (ptrdiff_t)frameInfo.crop_win_h);

		DRM_INFO("[%s][%d] %s %s: %td, %s: %td,	%s: %td, %s: %td\n",
			__func__, __LINE__,
			"metadata frm_info",
			"disp_win_x", (ptrdiff_t)frameInfo.disp_win_x,
			"disp_win_y", (ptrdiff_t)frameInfo.disp_win_y,
			"disp_win_w", (ptrdiff_t)frameInfo.disp_win_w,
			"disp_win_h", (ptrdiff_t)frameInfo.disp_win_h);
	}

	// check svp info
	meta_ret = mtk_render_common_read_metadata((int)meta_fd,
		RD_METATAG_SVP_INFO, (void *)&svpInfo);
	if (meta_ret) {
		DRM_INFO("[%s][%d] metadata do not has svp info tag\n",
			__func__, __LINE__);
	} else {
		DRM_INFO("[%s][%d] meta svp info: aid: %d, pipeline id: 0x%lx\n",
			__func__, __LINE__, svpInfo.aid, svpInfo.pipelineid);
		_mtk_video_display_set_aid(pctx_video, plane_idx, (uint8_t)svpInfo.aid);
	}

}

/* ------- above is static function -------*/

int mtk_video_display_init(
	struct device *dev,
	struct device *master,
	void *data,
	struct mtk_drm_plane **primary_plane,
	struct mtk_drm_plane **cursor_plane)
{
	int ret;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_tv_drm_crtc *crtc = &pctx->crtc;
	struct mtk_tv_drm_connector *connector = &pctx->connector;
	struct mtk_drm_plane *mplane_video = NULL;
	struct mtk_drm_plane *mplane_primary = NULL;
	struct drm_device *drm_dev = data;
	enum drm_plane_type drmPlaneType;
	unsigned int plane_idx;
	struct video_plane_prop *plane_props;
	unsigned int primary_plane_num = pctx->plane_num[MTK_DRM_PLANE_TYPE_PRIMARY];
	unsigned int video_plane_num =
		pctx->plane_num[MTK_DRM_PLANE_TYPE_VIDEO];
	unsigned int video_zpos_base = 0;
	unsigned int primary_zpos_base = 0;

	pctx->drm_dev = drm_dev;
	pctx_video->videoPlaneType_TypeNum = MTK_VPLANE_TYPE_MAX - 1;

	DRM_INFO("[%s][%d] plane start: [primary]:%d [video]:%d\n",
			__func__, __LINE__,
			pctx->plane_index_start[MTK_DRM_PLANE_TYPE_VIDEO],
			pctx->plane_index_start[MTK_DRM_PLANE_TYPE_PRIMARY]);

	DRM_INFO("[%s][%d] plane num: [primary]:%d [video]:%d\n",
			__func__, __LINE__,
			primary_plane_num,
			video_plane_num);

	mplane_video =
		devm_kzalloc(dev, sizeof(struct mtk_drm_plane) * video_plane_num, GFP_KERNEL);
	if (mplane_video == NULL) {
		DRM_INFO("devm_kzalloc failed.\n");
		return -ENOMEM;
	}

	crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO] = mplane_video;
	crtc->plane_count[MTK_DRM_PLANE_TYPE_VIDEO] = video_plane_num;

	mplane_primary =
		devm_kzalloc(dev, sizeof(struct mtk_drm_plane) * primary_plane_num, GFP_KERNEL);
	if (mplane_primary == NULL) {
		DRM_INFO("devm_kzalloc failed.\n");
		return -ENOMEM;
	}

	crtc->plane[MTK_DRM_PLANE_TYPE_PRIMARY] = mplane_primary;
	crtc->plane_count[MTK_DRM_PLANE_TYPE_PRIMARY] = primary_plane_num;

	plane_props =
		devm_kzalloc(dev, sizeof(struct video_plane_prop) * video_plane_num, GFP_KERNEL);
	if (plane_props == NULL) {
		DRM_INFO("devm_kzalloc failed.\n");
		return -ENOMEM;
	}
	memset(plane_props, 0, sizeof(struct video_plane_prop) * video_plane_num);
	pctx_video->video_plane_properties = plane_props;

	// read video related data from dtb
	ret = _readDTB2VideoPrivate(&pctx->video_priv);
	if (ret != 0x0) {
		DRM_INFO("[%s, %d]: readDTB2VideoPrivate failed\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	if (_mtk_video_display_create_plane_props(pctx) < 0) {
		DRM_INFO("[%s][%d]mtk_drm_video_create_plane_props fail!!\n",
			__func__, __LINE__);
		goto ERR;
	}

	connector->connector_private = pctx;

	for (plane_idx = 0; plane_idx < primary_plane_num; plane_idx++) {
		drmPlaneType = DRM_PLANE_TYPE_PRIMARY;

		*primary_plane = &crtc->plane[MTK_DRM_PLANE_TYPE_PRIMARY][plane_idx];

		ret = mtk_plane_init(drm_dev,
			&crtc->plane[MTK_DRM_PLANE_TYPE_PRIMARY][plane_idx].base,
			MTK_PLANE_DISPLAY_PIPE,
			drmPlaneType);

		if (ret != 0x0) {
			DRM_INFO("mtk_plane_init (primary plane) failed.\n");
			goto ERR;
		}

		crtc->plane[MTK_DRM_PLANE_TYPE_PRIMARY][plane_idx].crtc_base =
			&crtc->base;
		crtc->plane[MTK_DRM_PLANE_TYPE_PRIMARY][plane_idx].crtc_private =
			pctx;
		crtc->plane[MTK_DRM_PLANE_TYPE_PRIMARY][plane_idx].zpos =
			primary_zpos_base;
		crtc->plane[MTK_DRM_PLANE_TYPE_PRIMARY][plane_idx].primary_index =
			plane_idx;
		crtc->plane[MTK_DRM_PLANE_TYPE_PRIMARY][plane_idx].mtk_plane_type =
			MTK_DRM_PLANE_TYPE_PRIMARY;
	}

	for (plane_idx = 0; plane_idx < video_plane_num; plane_idx++) {
		drmPlaneType = DRM_PLANE_TYPE_OVERLAY;

		ret = mtk_plane_init(drm_dev,
			&crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO][plane_idx].base,
			MTK_PLANE_DISPLAY_PIPE,
			drmPlaneType);

		if (ret != 0x0) {
			DRM_INFO("mtk_plane_init (video plane) failed.\n");
			goto ERR;
		}

		crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO][plane_idx].crtc_base =
			&crtc->base;
		crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO][plane_idx].crtc_private =
			pctx;
		crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO][plane_idx].zpos =
			video_zpos_base;
		crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO][plane_idx].video_index =
			plane_idx;
		crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO][plane_idx].mtk_plane_type =
			MTK_DRM_PLANE_TYPE_VIDEO;

		_mtk_video_display_attach_plane_props(pctx,
			&crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO][plane_idx]);
	}

	DRM_INFO("[%s][%d] mtk video init success!!\n", __func__, __LINE__);

	return 0;

ERR:
	_mtk_video_display_clean_video_context(dev, pctx_video);
	return ret;
}

void mtk_video_display_update_plane(struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct drm_framebuffer *fb = state->base.fb;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	unsigned int plane_inx = mplane->video_index;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	uint64_t meta_fd = 0;
	uint64_t input_vfreq = 0;
	int i = 0;

	// check video plane type
	if (_mtk_video_display_is_ext_prop_updated(plane_props,
		EXT_VPLANE_PROP_VIDEO_PLANE_TYPE)) {

		// update window mask
		_mtk_video_display_set_window_mask
			(plane_inx, pctx_video, 1);
	}

	// check display window
	_mtk_video_display_set_disp_win(plane_inx, pctx_video, state);
	// check crop window
	_mtk_video_display_set_crop_win(plane_inx, pctx_video, state);
	// check fb memory format settins
	_mtk_video_display_set_fb_memory_format(plane_inx, pctx_video, fb);
	// check fb settins
	_mtk_video_display_set_fb(plane_inx, pctx_video, fb);

	// update scaling ratio
	_mtk_video_set_scaling_ratio(plane_inx, state);

	// check mute
	if (_mtk_video_display_is_ext_prop_updated(plane_props, EXT_VPLANE_PROP_MUTE_SCREEN))
		_mtk_video_display_set_mute(plane_inx, pctx_video);

	// check window ARGB
	if ((_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_WINDOW_ALPHA)) ||
		(_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_WINDOW_R)) ||
		(_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_WINDOW_G)) ||
		(_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_WINDOW_B))) {

		_mtk_video_display_set_windowARGB(plane_inx, pctx_video);
	}

	// check BG ARGB
	if ((_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_BACKGROUND_ALPHA)) ||
		(_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_BACKGROUND_R)) ||
		(_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_BACKGROUND_G)) ||
		(_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_BACKGROUND_B))) {

		_mtk_video_display_set_BackGroundARGB(plane_inx, pctx_video);
	}

	// check zpos
	_mtk_video_display_set_zpos(plane_inx, pctx_video, state);

	// check metadata content
	meta_fd = plane_props->prop_val[EXT_VPLANE_PROP_META_FD];
	_mtk_video_display_check_meta(pctx_video, plane_inx, meta_fd);

	//update input vfreq
	if (_mtk_video_display_is_ext_prop_updated(
		plane_props, EXT_VPLANE_PROP_INPUT_VFREQ)) {
		input_vfreq = plane_props->prop_val[EXT_VPLANE_PROP_INPUT_VFREQ];
		DRM_INFO("[%s][%d] plane %d: input_vfreq update to %td\n",
			__func__, __LINE__, plane_inx, (ptrdiff_t)input_vfreq);
	}

	for (i = 0; i < EXT_VPLANE_PROP_MAX; i++)
		plane_props->old_prop_val[i] = plane_props->prop_val[i];
}

void mtk_video_display_disable_plane(struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	unsigned int plane_inx = mplane->video_index;

	DRM_INFO("[%s][%d] disable plane:%d\n",
		__func__, __LINE__, plane_inx);

	_mtk_video_display_set_window_mask(plane_inx, pctx_video, 0);
}

int mtk_video_display_atomic_set_plane_property(
	struct mtk_drm_plane *mplane,
	struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video =
		&pctx->video_priv;
	int plane_inx =
		mplane->video_index;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);

	for (i = 0; i < EXT_VPLANE_PROP_MAX; i++) {
		if (property == pctx_video->drm_video_plane_prop[i]) {
			plane_props->old_prop_val[i] = plane_props->prop_val[i];
			plane_props->prop_val[i] = val;

			if (plane_props->old_prop_val[i] != plane_props->prop_val[i])
				plane_props->prop_update[i] = 1;
			else
				plane_props->prop_update[i] = 0;
		ret = 0x0;
		break;
		}
	}
	if ((i == EXT_VPLANE_PROP_VIDEO_PLANE_TYPE) &&
		(_mtk_video_display_is_ext_prop_updated(plane_props, i))) {
		_mtk_video_display_check_support_window(
			plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE],
			plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE],
			pctx_video);
	}

	return ret;
}

int mtk_video_display_atomic_get_plane_property(
	struct mtk_drm_plane *mplane,
	const struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = -EINVAL;
	int i;
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	int plane_inx = mplane->video_index;
	struct video_plane_prop *plane_props = (pctx_video->video_plane_properties + plane_inx);

	for (i = 0; i < EXT_VPLANE_PROP_MAX; i++) {
		if (property == pctx_video->drm_video_plane_prop[i]) {
			*val = plane_props->prop_val[i];
			ret = 0x0;
			break;
		}
	}

	return ret;
}

int mtk_video_check_plane(
	struct drm_plane_state *plane_state,
	const struct drm_crtc_state *crtc_state,
	struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	unsigned int plane_inx = mplane->video_index;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	int i, ret = 0;
	uint64_t disp_width = plane_state->crtc_w;
	uint64_t disp_height = plane_state->crtc_h;
	uint64_t src_width = (plane_state->src_w) >> 16;
	uint64_t src_height = (plane_state->src_h) >> 16;

	DRM_INFO("[%s][%d] video_plane_type:%d[src_w,src_h,disp_w,disp_h]:[%lld,%lld,%lld,%lld]\n"
			, __func__, __LINE__, video_plane_type,
			src_width, src_height, disp_width, disp_height);
	// check scaling cability
	ret = drm_atomic_helper_check_plane_state(plane_state, crtc_state,
						DRM_PLANE_SCALE_MIN,
						DRM_PLANE_SCALE_MAX,
						true, true);
	// check video plane type update
	if (_mtk_video_display_is_ext_prop_updated(plane_props,
		EXT_VPLANE_PROP_VIDEO_PLANE_TYPE)) {
		//mtk_video_check_support_window(plane_props, pctx_video);
		if ((pctx_video->videoPlaneType_PlaneNum.MEMC_num > 1) ||
		(pctx_video->videoPlaneType_PlaneNum.MGW_num > 16) ||
		(pctx_video->videoPlaneType_PlaneNum.DMA1_num > 1) ||
		(pctx_video->videoPlaneType_PlaneNum.DMA2_num > 1)) {
			DRM_INFO("\033[0;32;31m[%s][%d] over HW support number !\n\033[m",
				__func__, __LINE__);
			goto error;
		}
	}
	if ((video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) &&
		(pctx_video->videoPlaneType_PlaneNum.MGW_num > 1) &&
		((src_width != disp_width) || (src_height != disp_height))) {
		DRM_INFO("\033[0;32;31m[%s][%d] MGW_num %d not support scaling !\n\033[m",
			__func__, __LINE__, pctx_video->videoPlaneType_PlaneNum.MGW_num);
		goto error;
	}
	return ret;

error:
	for (i = 0; i < EXT_VPLANE_PROP_MAX; i++) {
		if ((i == EXT_VPLANE_PROP_VIDEO_PLANE_TYPE) &&
			(_mtk_video_display_is_ext_prop_updated(plane_props, i))) {
			_mtk_video_display_check_support_window(
				plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE],
				plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE],
				pctx_video);
		}
		plane_props->prop_val[i] = plane_props->old_prop_val[i];
	}
	return -EINVAL;
}

