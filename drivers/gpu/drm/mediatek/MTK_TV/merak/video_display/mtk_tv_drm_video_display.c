// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/math64.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/dma-buf.h>
#include <drm/mtk_tv_drm.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>

#include "mtk_tv_drm_plane.h"
#include "mtk_tv_drm_gem.h"
#include "mtk_tv_drm_video_display.h"
#include "mtk_tv_drm_video_frc.h"
#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_common.h"
#include "mtk_tv_drm_log.h"
#include "drv_scriptmgt.h"

#include "hwreg_render_video_display.h"
#include "hwreg_render_video_frc.h"
#include "hwreg_render_stub.h"

#include "pqu_msg.h"
#include "m_pqu_pq.h"
#include "ext_command_render_if.h"


//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------
#define MTK_DRM_MODEL MTK_DRM_MODEL_DISPLAY

/*annie*/
#define MEMC_CONFIG (1)

#define DRM_PLANE_SCALE_MIN (1<<4)
#define DRM_PLANE_SCALE_MAX (1<<28)
#define MAX_WINDOW_NUM (16)
#define OLD_LAYER_INITIAL_VALUE 0xFF

#define MEMC_PROP(i) \
		((i >= EXT_VPLANE_PROP_MEMC_LEVEL) && \
			(i <= EXT_VPLANE_PROP_MEMC_INIT_VALUE_FOR_RV55)) \

#define ALIGN_UPTO_X(w, x)  ((((w) + (x - 1)) / x) * x)
#define ALIGN_DOWNTO_X(w, x)  ((w/x) * x)

#define WINDOW_ALPHA_DEFAULT_VALUE 0xFF
#define BLEND_VER0_HSZIE_UNIT 4
#define BITS_PER_BYTE 8
#define MEMC_WINID_DEFAULT 0xFFFF
#define ROUNDING 2

bool g_bUseRIU = FALSE;

//-----------------------------------------------------------------------------
//  Local Variables
//-----------------------------------------------------------------------------
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

static struct drm_prop_enum_list video_memc_misc_type_enum_list[] = {
	{
		.type = VIDEO_PLANE_MEMC_MISC_NULL,
		.name = MTK_VIDEO_PROP_MEMC_MISC_NULL},
	{
		.type = VIDEO_PLANE_MEMC_MISC_INSIDE,
		.name = MTK_VIDEO_PROP_MEMC_MISC_INSIDE},
	{
		.type = VIDEO_PLANE_MEMC_MISC_INSIDE_60HZ,
		.name = MTK_VIDEO_PROP_MEMC_MISC_INSIDE_60HZ},
};

static struct drm_prop_enum_list video_memc_pattern_enum_list[] = {
	{.type = VIDEO_PLANE_MEMC_NULL_PAT,
		.name = MTK_VIDEO_PROP_MEMC_NULL_PAT},
	{.type = VIDEO_PLANE_MEMC_OPM_PAT,
		.name = MTK_VIDEO_PROP_MEMC_OPM_PAT},
	{.type = VIDEO_PLANE_MEMC_END_PAT,
		.name = MTK_VIDEO_PROP_MEMC_END_PAT},
};

static struct drm_prop_enum_list video_memc_level_enum_list[] = {
	{.type = VIDEO_PLANE_MEMC_LEVEL_OFF,
		.name = MTK_VIDEO_PROP_MEMC_LEVEL_OFF},
	{.type = VIDEO_PLANE_MEMC_LEVEL_LOW,
		.name = MTK_VIDEO_PROP_MEMC_LEVEL_LOW},
	{.type = VIDEO_PLANE_MEMC_LEVEL_MID,
		.name = MTK_VIDEO_PROP_MEMC_LEVEL_MID},
	{.type = VIDEO_PLANE_MEMC_LEVEL_HIGH,
		.name = MTK_VIDEO_PROP_MEMC_LEVEL_HIGH},
};

static struct drm_prop_enum_list video_memc_game_mode_enum_list[] = {
	{.type = VIDEO_PLANE_MEMC_GAME_OFF,
		.name = MTK_VIDEO_PROP_MEMC_GAME_OFF},
	{.type = VIDEO_PLANE_MEMC_GAME_ON,
		.name = MTK_VIDEO_PROP_MEMC_GAME_ON},
};

static struct drm_prop_enum_list vplane_buf_mode_enum_list[] = {
	{
		.type = MTK_VPLANE_BUF_MODE_SW,
		.name = "VPLANE_BUF_MODE_SW"
	},
	{
		.type = MTK_VPLANE_BUF_MODE_HW,
		.name = "VPLANE_BUF_MODE_HW"
	},
	{
		.type = MTK_VPLANE_BUF_MODE_BYPASSS,
		.name = "VPLANE_BUF_MODE_BYPASSS"
	},
};

static struct drm_prop_enum_list vplane_disp_mode_enum_list[] = {
	{
		.type = MTK_VPLANE_DISP_MODE_SW,
		.name = "VPLANE_DISP_MODE_SW"
	},
	{
		.type = MTK_VPLANE_DISP_MODE_HW,
		.name = "VPLANE_DISP_MODE_HW"
	},
	{
		.type = MTK_VPLANE_DISP_MODE_PRE_SW,
		.name = "VPLANE_DISP_MODE_PRE_SW"
	},
	{
		.type = MTK_VPLANE_DISP_MODE_PRE_HW,
		.name = "VPLANE_DISP_MODE_PRE_HW"
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
	{
		.prop_name = PROP_VIDEO_PLANE_PROP_BUF_MODE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &vplane_buf_mode_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(vplane_buf_mode_enum_list),
		.init_val = 0x0,
	},
	{
		.prop_name = PROP_VIDEO_PLANE_PROP_DISP_MODE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &vplane_disp_mode_enum_list[0],
		.enum_info.enum_length = ARRAY_SIZE(vplane_disp_mode_enum_list),
		.init_val = 0x0,
	},
/*annie add MEMC*/
#if (1)/*(MEMC_CONFIG == 1)*/
	/****MEMC plane prop******/
	{
		.prop_name = MTK_PLANE_PROP_MEMC_LEVEL_MODE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_memc_level_enum_list[0],
		.enum_info.enum_length =
			ARRAY_SIZE(video_memc_level_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_PLANE_PROP_MEMC_GAME_MODE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_memc_game_mode_enum_list[0],
		.enum_info.enum_length =
			ARRAY_SIZE(video_memc_game_mode_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_PLANE_PROP_MEMC_PATTERN,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_memc_pattern_enum_list[0],
		.enum_info.enum_length =
			ARRAY_SIZE(video_memc_pattern_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_PLANE_PROP_MEMC_MISC_TYPE,
		.flag = DRM_MODE_PROP_ENUM | DRM_MODE_PROP_ATOMIC,
		.enum_info.enum_list = &video_memc_misc_type_enum_list[0],
		.enum_info.enum_length =
			ARRAY_SIZE(video_memc_misc_type_enum_list),
		.init_val = 0,
	},
	{
		.prop_name = MTK_PLANE_PROP_MEMC_DECODE_IDX_DIFF,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_PLANE_PROP_MEMC_STATUS,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0,
		.range_info.max = 0xFF,
		.init_val = 0,
	},
	{
		.prop_name = MTK_PLANE_PROP_MEMC_TRIG_GEN,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0xBB,
	},
	{
		.prop_name = MTK_PLANE_PROP_MEMC_RV55_INFO,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
	{
		.prop_name = MTK_PLANE_PROP_MEMC_INIT_VALUE_FOR_RV55,
		.flag = DRM_MODE_PROP_RANGE | DRM_MODE_PROP_ATOMIC,
		.range_info.min = 0x0,
		.range_info.max = 0xFF,
		.init_val = 0x0,
	},
#endif
	{
		.prop_name = PROP_VIDEO_CRTC_PROP_WINDOW_PQ,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
		.init_val = 0,
	},
	{
		.prop_name = MTK_VIDEO_PLANE_PROP_PQMAP_NODES_ARRAY,
		.flag = DRM_MODE_PROP_BLOB | DRM_MODE_PROP_ATOMIC,
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
	unsigned int plane_inx, struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	int i = 0;

	if (video_plane_type < MTK_VPLANE_TYPE_MAX)
		pctx_video->plane_num[video_plane_type]++;

	if (pctx_video->plane_num[old_video_plane_type] > 0)
		pctx_video->plane_num[old_video_plane_type]--;

	for (i = 0; i < MTK_VPLANE_TYPE_MAX; i++) {
		DRM_DEBUG_ATOMIC("[%d] plane_type=%d plane_num=%d\n",
			__LINE__, video_plane_type,
			pctx_video->plane_num[video_plane_type]);
	}

	#if (1)//(MEMC_CONFIG == 1)
	plane_ctx->memc_change_on = MEMC_CHANGE_NONE;
	#endif
	if (video_plane_type == MTK_VPLANE_TYPE_MEMC) {
		#if (1)//(MEMC_CONFIG == 1)
		plane_ctx->memc_change_on = MEMC_CHANGE_ON;
		DRM_INFO("[%s][%d] memc_change_on\n",	__func__, __LINE__);
		#endif
	} else if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
		#if (1)//(MEMC_CONFIG == 1)
		plane_ctx->memc_change_on = MEMC_CHANGE_OFF;
		DRM_INFO("[%s][%d] memc_change_off\n",	__func__, __LINE__);
		#endif
	} else if (video_plane_type == MTK_VPLANE_TYPE_SINGLE_WIN1) {
		#if (1)//(MEMC_CONFIG == 1)
		plane_ctx->memc_change_on = MEMC_CHANGE_OFF;
		DRM_INFO("[%s][%d] memc_change_off\n",	__func__, __LINE__);
		#endif
	} else {
		#if (1)//(MEMC_CONFIG == 1)
		plane_ctx->memc_change_on = MEMC_CHANGE_OFF;
		DRM_INFO("[%s][%d] memc_change_off\n",	__func__, __LINE__);
		#endif
	}
}

static void _mtk_video_display_set_mgwdmaEnable(
	struct mtk_video_context *pctx_video)
{
	enum drm_mtk_video_plane_type video_plane_type = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct mtk_tv_kms_context *pctx = NULL;

	struct reg_info reg[reg_num];
	struct hwregMGWDMAEnIn paramIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	bool force = FALSE;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(paramIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;

	paramIn.RIU = g_bUseRIU;
	paramIn.planeType = video_plane_type;

	if (pctx->stub)
		force = TRUE;

	for (video_plane_type = MTK_VPLANE_TYPE_MULTI_WIN;
					video_plane_type <= MTK_VPLANE_TYPE_SINGLE_WIN2;
					++video_plane_type) {
		paramIn.planeType = video_plane_type;
		if (pctx_video->plane_num[video_plane_type] == 0)
			paramIn.enable = false;
		else
			paramIn.enable = true;

		if (force || (paramIn.enable != pctx_video->mgwdmaEnable[video_plane_type])) {
			drv_hwreg_render_display_set_mgwdmaEnable(paramIn, &paramOut);

			DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
				__func__, __LINE__, paramOut.regCount);

			pctx_video->mgwdmaEnable[video_plane_type] = paramIn.enable;
		}
	}

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + paramOut.regCount;
	}
}

static void _mtk_video_display_set_frcOpmMaskEnable(
	struct mtk_video_context *pctx_video)
{
	enum drm_mtk_video_plane_type video_plane_type = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct mtk_tv_kms_context *pctx = NULL;

	struct reg_info reg[reg_num];
	struct hwregFrcOpmMaskEn paramIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(paramIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;
	paramIn.RIU = g_bUseRIU;

	video_plane_type = MTK_VPLANE_TYPE_MEMC;
	if (pctx_video->plane_num[video_plane_type] == 0)
		paramIn.enable = false;
	else
		paramIn.enable = true;

	if (paramIn.enable != pctx_video->frcopmMaskEnable) {
		if (paramIn.enable == false) {
			//When FRC plane_num = 0, Mask OPM Read
			drv_hwreg_render_frc_opmMaskEn(paramIn, &paramOut);
		}
		pctx_video->frcopmMaskEnable = paramIn.enable;
	}

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + paramOut.regCount;
	}
}


static void _mtk_video_display_set_extWinEnable(
	struct mtk_video_context *pctx_video)
{
	enum drm_mtk_video_plane_type video_plane_type = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct mtk_tv_kms_context *pctx = NULL;

	struct reg_info reg[reg_num];
	struct hwregExtWinEnIn paramIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	bool force = FALSE;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(paramIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;

	paramIn.RIU = g_bUseRIU;
	paramIn.planeType = video_plane_type;

	if (pctx->stub)
		force = TRUE;

	for (video_plane_type = MTK_VPLANE_TYPE_MULTI_WIN;
					video_plane_type <= MTK_VPLANE_TYPE_SINGLE_WIN2;
					++video_plane_type) {
		paramIn.planeType = video_plane_type;
		//only MEMC/DMA need to enable ext bank / MGW use MGW multi bank
		if (video_plane_type != MTK_VPLANE_TYPE_MULTI_WIN)
			paramIn.enable = true;
		else
			paramIn.enable = false;

		if (force || (paramIn.enable != pctx_video->extWinEnable[video_plane_type])) {
			drv_hwreg_render_display_set_extWinEnable(paramIn, &paramOut);

			DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
				__func__, __LINE__, paramOut.regCount);

			pctx_video->extWinEnable[video_plane_type] = paramIn.enable;
		}
	}

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + paramOut.regCount;
	}
}

static void _mtk_video_display_set_mgwdmaWinEnable(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	bool enable)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	enum drm_mtk_video_plane_type old_video_plane_type =
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;

	struct reg_info reg[reg_num];
	struct hwregMGWDMAWinEnIn paramIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(paramIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramOut.reg = reg;

	paramIn.RIU = g_bUseRIU;
	paramIn.planeType = (enable ? video_plane_type : old_video_plane_type);
	paramIn.windowID = plane_inx;
	paramIn.enable = enable;

	DRM_INFO("[%s][%d]plane_inx:%d video_plane_type:%d, old_video_plane_type:%d\n",
		__func__, __LINE__, plane_inx, video_plane_type, old_video_plane_type);

	DRM_INFO("[%s][%d][paramIn]RIU:%d planeType:%d windowID:%d enable:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.planeType,
		paramIn.windowID, paramIn.enable);

	drv_hwreg_render_display_set_mgwdmaWinEnable(paramIn, &paramOut);

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + paramOut.regCount;
	}
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
	enum drm_mtk_video_plane_type old_video_plane_type =
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->vgs_ml.memindex;
	int fd = pctx_video->vgs_ml.fd;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_panel_context *pctx_pnl = NULL;
	struct mtk_video_plane_ctx *plane_ctx = pctx_video->plane_ctx + plane_inx;

	struct window cropwindow[MAX_WINDOW_NUM] = {{0, 0, 0, 0}};

	struct reg_info reg[reg_num];
	struct hwregCropWinIn paramIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	bool force = FALSE;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregCropWinIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	if (plane_inx >= MAX_WINDOW_NUM) {
		DRM_ERROR("[%s][%d] Invalid plane_inx:%d !! MAX_WINDOW_NUM:%d\n",
			__func__, __LINE__, plane_inx, MAX_WINDOW_NUM);
		return;
	}

	pctx = pctx_video_to_kms(pctx_video);
	pctx_pnl = &(pctx->panel_priv);

	paramOut.reg = reg;

	paramIn.RIU = g_bUseRIU;
	paramIn.planeType = video_plane_type;
	paramIn.windowID = plane_inx;
	cropwindow[plane_inx].x = state->base.src_x >> 16;
	cropwindow[plane_inx].y = state->base.src_y >> 16;
	cropwindow[plane_inx].w = state->base.src_w >> 16;
	cropwindow[plane_inx].h = plane_ctx->src_h >> 16; // update at scaling

	if (pctx->stub)
		force = TRUE;

	if (force ||
		(_mtk_video_display_is_variable_updated(
			old_video_plane_type, video_plane_type)) ||
		((_mtk_video_display_is_variable_updated(
			plane_ctx->oldcropwindow.x, cropwindow[plane_inx].x)) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->oldcropwindow.y, cropwindow[plane_inx].y)) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->oldcropwindow.w, cropwindow[plane_inx].w)) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->oldcropwindow.h, cropwindow[plane_inx].h)))
	) {
		DRM_INFO("[%s][%d] plane_inx:%d old crop win:[%d, %d, %d, %d]\n"
			, __func__, __LINE__, plane_inx,
			plane_ctx->oldcropwindow.x, plane_ctx->oldcropwindow.y,
			plane_ctx->oldcropwindow.w, plane_ctx->oldcropwindow.h);

		DRM_INFO("[%s][%d] plane_inx:%d crop win:[%d, %d, %d, %d]\n",
			__func__, __LINE__, plane_inx,
			cropwindow[plane_inx].x, cropwindow[plane_inx].y,
			cropwindow[plane_inx].w, cropwindow[plane_inx].h);

		DRM_INFO("[%s][%d] video_plane_type:%d\n",
			__func__, __LINE__, video_plane_type);


		if ((video_plane_type > MTK_VPLANE_TYPE_NONE) ||
			(video_plane_type < MTK_VPLANE_TYPE_MAX)) {

			if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
				paramIn.MGWPlaneNum =
					pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN];
				/*set MGW ext win H_size_ext/V_size_ext*/
				/*Use MGW for fill frame size without using blending HW*/
				paramIn.cropWindow.w = pctx_pnl->info.de_width;
				paramIn.cropWindow.h = pctx_pnl->info.de_height;

				DRM_INFO("[%s][%d] MGW_NUM:%d\n",
					__func__, __LINE__, paramIn.MGWPlaneNum);
			} else {
				/*set ext win H_size/V_size*/
				paramIn.cropWindow.w = cropwindow[plane_inx].w;
				paramIn.cropWindow.h = cropwindow[plane_inx].h;
			}
		} else {
			DRM_INFO("[%s][%d] Invalid  HW plane type\n",
				__func__, __LINE__);
		}

		paramIn.cropWindow.x = cropwindow[plane_inx].x;
		paramIn.cropWindow.y = cropwindow[plane_inx].y;

		DRM_INFO("[%s][%d][paramIn] RIU:%d planeType:%d windowID:%d MGWNum:%d\n",
			__func__, __LINE__,
			paramIn.RIU, paramIn.planeType,
			paramIn.windowID, paramIn.MGWPlaneNum);

		DRM_INFO("[%s][%d][paramIn] crop win(x,y,w,h):[%d, %d, %d, %d]\n",
			__func__, __LINE__,
			paramIn.cropWindow.x, paramIn.cropWindow.y,
			paramIn.cropWindow.w, paramIn.cropWindow.h);

		if (video_plane_type == MTK_VPLANE_TYPE_MEMC)
			drv_hwreg_render_frc_set_cropWindow(paramIn, &paramOut);
		else
			drv_hwreg_render_display_set_cropWindow(paramIn, &paramOut);

		plane_ctx->oldcropwindow.x = cropwindow[plane_inx].x;
		plane_ctx->oldcropwindow.y = cropwindow[plane_inx].y;
		plane_ctx->oldcropwindow.w = cropwindow[plane_inx].w;
		plane_ctx->oldcropwindow.h = cropwindow[plane_inx].h;

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
			//3.add cmd
			cmd_info.cmd_cnt = paramOut.regCount;
			cmd_info.mem_index = memindex;
			cmd_info.reg = (struct sm_reg *)&reg;
			sm_ml_add_cmd(fd, &cmd_info);
			pctx_video->vgs_ml.regcount = pctx_video->vgs_ml.regcount
				+ paramOut.regCount;
		}
	}
}

static void _mtk_video_display_get_format_bpp(
	struct mtk_video_format_info *video_format_info)
{
	switch (video_format_info->fourcc) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
		video_format_info->bpp[0] = 32;
		video_format_info->bpp[1] = 0;
		video_format_info->bpp[2] = 0;
		break;
	case DRM_FORMAT_RGB565:
		video_format_info->bpp[0] = 16;
		video_format_info->bpp[1] = 0;
		video_format_info->bpp[2] = 0;
		break;
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
		video_format_info->bpp[0] = 32;
		video_format_info->bpp[1] = 0;
		video_format_info->bpp[2] = 0;
		break;
	case DRM_FORMAT_YUV444:
		if (video_format_info->modifier && video_format_info->modifier_compressed) {
			//YUV444 8b ce
			video_format_info->bpp[0] = 8;
			video_format_info->bpp[1] = 8;
			video_format_info->bpp[2] = 8;
		} else {
			//undefine format
			video_format_info->bpp[0] = 0;
			video_format_info->bpp[1] = 0;
			video_format_info->bpp[2] = 0;
		}
		break;
	case DRM_FORMAT_YVYU:
		if (video_format_info->modifier_arrange == (DRM_FORMAT_MOD_MTK_YUV444_VYU & 0xff)) {
			if (video_format_info->modifier_10bit) {
				//YUV444 10b
				video_format_info->bpp[0] = 30;
				video_format_info->bpp[1] = 0;
				video_format_info->bpp[2] = 0;
			} else if (video_format_info->modifier_6bit) {
				//undefine format
				video_format_info->bpp[0] = 0;
				video_format_info->bpp[1] = 0;
				video_format_info->bpp[2] = 0;
			} else {
				//YUV444 8b
				video_format_info->bpp[0] = 24;
				video_format_info->bpp[1] = 0;
				video_format_info->bpp[2] = 0;
			}
		}
		break;
	case DRM_FORMAT_NV61:
		if (video_format_info->modifier && video_format_info->modifier_10bit) {
			//YUV422 10b
			video_format_info->bpp[0] = 10;
			video_format_info->bpp[1] = 10;
			video_format_info->bpp[2] = 0;
		} else if (video_format_info->modifier &&
			   video_format_info->modifier_6bit &&
			   video_format_info->modifier_compressed) {
			//YUV422 6b ce
			video_format_info->bpp[0] = 6;
			video_format_info->bpp[1] = 6;
			video_format_info->bpp[2] = 0;
		} else if (video_format_info->modifier && video_format_info->modifier_compressed) {
			//YUV422 8b ce
			video_format_info->bpp[0] = 8;
			video_format_info->bpp[1] = 8;
			video_format_info->bpp[2] = 0;
		} else if (!video_format_info->modifier) {
			//YUV422 8b 2pln
			video_format_info->bpp[0] = 8;
			video_format_info->bpp[1] = 8;
			video_format_info->bpp[2] = 0;
		} else {
			//undefine format
			video_format_info->bpp[0] = 0;
			video_format_info->bpp[1] = 0;
			video_format_info->bpp[2] = 0;
		}
		break;
	case DRM_FORMAT_NV21:
		if (video_format_info->modifier && video_format_info->modifier_10bit) {
			//YUV420 10b
			video_format_info->bpp[0] = 10;
			video_format_info->bpp[1] = 5;
			video_format_info->bpp[2] = 0;
		} else if (video_format_info->modifier &&
			   video_format_info->modifier_6bit &&
			   video_format_info->modifier_compressed) {
			//YUV420 6b ce
			video_format_info->bpp[0] = 6;
			video_format_info->bpp[1] = 3;
			video_format_info->bpp[2] = 0;
		} else if (video_format_info->modifier && video_format_info->modifier_compressed) {
			//YUV420 8b ce
			video_format_info->bpp[0] = 8;
			video_format_info->bpp[1] = 4;
			video_format_info->bpp[2] = 0;
		} else if (!video_format_info->modifier) {
			//YUV420 8b
			video_format_info->bpp[0] = 8;
			video_format_info->bpp[1] = 4;
			video_format_info->bpp[2] = 0;
		} else {
			//undefine format
			video_format_info->bpp[0] = 0;
			video_format_info->bpp[1] = 0;
			video_format_info->bpp[2] = 0;
		}
		break;
	default:
		video_format_info->bpp[0] = 0;
		video_format_info->bpp[1] = 0;
		video_format_info->bpp[2] = 0;
		break;
	}
}

static void _mtk_video_display_get_disp_mode(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	enum drm_mtk_vplane_disp_mode *disp_mode)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);

	*disp_mode = plane_props->prop_val[EXT_VPLANE_PROP_DISP_MODE];

}

static void _mtk_video_display_update_displywinodw_info(
	unsigned int plane_inx,
	struct mtk_tv_kms_context *pctx,
	struct mtk_plane_state *state)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];

	plane_ctx->dispwindow.x = ALIGN_UPTO_X(state->base.crtc_x, pctx->video_priv.disp_x_align);
	plane_ctx->dispwindow.y = ALIGN_UPTO_X(state->base.crtc_y, pctx->video_priv.disp_y_align);
	plane_ctx->dispwindow.w = ALIGN_DOWNTO_X(state->base.crtc_w, pctx->video_priv.disp_w_align);
	plane_ctx->dispwindow.h = state->base.crtc_h;

	DRM_INFO("[%s][%d] plane_inx:%d video_plane_type:%d disp win(x,y,w,h):[%d, %d, %d, %d]\n",
		__func__, __LINE__, plane_inx, video_plane_type,
		plane_ctx->dispwindow.x, plane_ctx->dispwindow.y,
		plane_ctx->dispwindow.w, plane_ctx->dispwindow.h);

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
	do_div(Phaseresult, (Output));  Phaseresult = Phaseresult - 1048576ul;\
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
	struct mtk_video_context *pctx_video,
	struct mtk_plane_state *state)
{
	uint64_t result = 0, Phaseresult = 0;
	uint32_t TmpRatio = 0;
	uint32_t TmpFactor = 0;
	uint32_t HorIniFactor = 0, VerIniFactor = 0;
	uint32_t HorRatio = 0, VerRatio = 0;
	uint8_t Hscale_enable = 0, Vscale_enable = 0;
	uint32_t Thinfactor = 0;
	uint64_t tmpheight = 0;
	uint8_t IsScalingUp_H = 0, IsScalingUp_V = 0;
	uint64_t Rem = 0, DivResult = 0;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	enum drm_mtk_video_plane_type old_video_plane_type =
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->vgs_ml.memindex;
	int fd = pctx_video->vgs_ml.fd;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_panel_context *pctx_pnl = NULL;
	struct mtk_video_plane_ctx *plane_ctx = pctx_video->plane_ctx + plane_inx;

	uint64_t crop_width[MAX_WINDOW_NUM] = {0}, crop_height[MAX_WINDOW_NUM] = {0};
	uint64_t disp_width[MAX_WINDOW_NUM] = {0}, disp_height[MAX_WINDOW_NUM] = {0};

	struct reg_info reg[reg_num];
	struct hwregScalingIn paramScalingIn;
	struct hwregThinReadFactorIn paraThinReadFactorIn;
	struct hwregLinebuffersizeIn paraLinebuffersizeIn;

	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	bool force = FALSE;

	memset(reg, 0, sizeof(reg));
	memset(&paraThinReadFactorIn, 0, sizeof(struct hwregThinReadFactorIn));
	memset(&paramScalingIn, 0, sizeof(struct hwregScalingIn));
	memset(&paraLinebuffersizeIn, 0, sizeof(struct hwregLinebuffersizeIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	pctx = pctx_video_to_kms(pctx_video);
	pctx_pnl = &(pctx->panel_priv);

	paramOut.reg = reg;

	paraThinReadFactorIn.RIU = g_bUseRIU;
	paraThinReadFactorIn.planeType = video_plane_type;

	paramScalingIn.RIU = g_bUseRIU;
	paramScalingIn.planeType = video_plane_type;
	paramScalingIn.windowID = plane_inx;

	paraLinebuffersizeIn.RIU = g_bUseRIU;
	paraLinebuffersizeIn.planeType = video_plane_type;

	disp_width[plane_inx] = plane_ctx->dispwindow.w;
	disp_height[plane_inx] = plane_ctx->dispwindow.h;
	crop_width[plane_inx] = state->base.src_w >> 16;
	crop_height[plane_inx] = state->base.src_h >> 16;

	if (pctx->stub)
		force = TRUE;

	if (force ||
		(_mtk_video_display_is_variable_updated(
			old_video_plane_type, video_plane_type)) ||
		((_mtk_video_display_is_variable_updated(
			plane_ctx->hvspoldcropwindow.w, crop_width[plane_inx])) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->hvspoldcropwindow.h, crop_height[plane_inx])) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->hvspolddispwindow.w, disp_width[plane_inx])) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->hvspolddispwindow.h, disp_height[plane_inx])))
	) {
		DRM_INFO("[%s][%d] plane_inx:%d crop win[w,h]:[%lld, %lld]\n"
			, __func__, __LINE__, plane_inx,
			crop_width[plane_inx], crop_height[plane_inx]);
		DRM_INFO("[%s][%d] plane_inx:%d disp win[w,h]:[%lld, %lld]\n"
			, __func__, __LINE__, plane_inx,
			disp_width[plane_inx], disp_height[plane_inx]);

		if ((video_plane_type > MTK_VPLANE_TYPE_NONE) ||
			(video_plane_type < MTK_VPLANE_TYPE_MAX)) {

			if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
				paraThinReadFactorIn.MGWPlaneNum =
					pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN];
				paramScalingIn.MGWPlaneNum =
					pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN];
				paraLinebuffersizeIn.MGWPlaneNum =
					pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN];

				DRM_INFO("[%s][%d] MGW_NUM:%d\n",
				__func__, __LINE__, paraThinReadFactorIn.MGWPlaneNum);
			}
		} else {
			DRM_INFO("[%s][%d] Invalid  HW plane type\n",
					__func__, __LINE__);
		}

		// set line buffer
		if (pctx->blend_hw_version == 1) {
			paraLinebuffersizeIn.enable = 1;
			if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN)
				paraLinebuffersizeIn.Linebuffersize = pctx_pnl->info.de_width;
			else
				paraLinebuffersizeIn.Linebuffersize = state->base.crtc_w;
		}

		//-----------------------------------------
		// Horizontal
		//-----------------------------------------
		if (crop_width[plane_inx] != disp_width[plane_inx]) {
			H_ScalingRatio(crop_width[plane_inx], disp_width[plane_inx]);
			TmpRatio = (uint32_t)result;
			Hscale_enable = 1;
		} else {
			Hscale_enable = 0;
			TmpRatio = 0x100000L;
		}
		HorRatio = TmpRatio;
		/// Set Phase Factor
		if (disp_width[plane_inx] > crop_width[plane_inx]) { // Scale Up
			H_ScalingUpPhase(crop_width[plane_inx], disp_width[plane_inx]);
			TmpFactor = (uint32_t)Phaseresult;
			IsScalingUp_H = 1;
		} else if (disp_width[plane_inx] < crop_width[plane_inx]) {
			H_ScalingDownPhase(crop_width[plane_inx], disp_width[plane_inx]);
			TmpFactor = (uint32_t)Phaseresult;
			IsScalingUp_H = 0;
		} else {
			TmpFactor = 0;
		}
		HorIniFactor = TmpFactor;
		DRM_INFO("[%s][%d] H_ratio:%x, H_initfactor: %x, ScalingUp %d\n"
			, __func__, __LINE__, HorRatio,
			HorIniFactor, IsScalingUp_H);
		//Vertical
		//Thin Read
		if (crop_height[plane_inx] <= disp_height[plane_inx]) {
			Thinfactor = 1;
		} else {
			DivResult = div64_u64_rem(crop_height[plane_inx],
				disp_height[plane_inx], &Rem);
			if ((Rem) == 0)
				Thinfactor = DivResult;
			else
				Thinfactor =
					(((div64_u64(crop_height[plane_inx],
					disp_height[plane_inx]))+ROUNDING)>>1)<<1;

		}
		//Thinfactor = ((crop_height/disp_height)>>1)<<1;
		tmpheight = div64_u64(crop_height[plane_inx], Thinfactor);
		plane_ctx->src_h = tmpheight << 16; // retore the height after thin read
		DRM_INFO("[%s][%d] Thin Read factor:%d, Height after Thin read: %d\n"
			, __func__, __LINE__, Thinfactor,
			tmpheight);
		if (tmpheight == disp_height[plane_inx]) {
			Vscale_enable = 0;
			TmpRatio = 0x100000L;
		} else {
			V_ScalingRatio(tmpheight, disp_height[plane_inx]);
			Vscale_enable = 1;
			TmpRatio = (uint32_t)result;
		}
		VerRatio = TmpRatio;
		if (disp_height[plane_inx] > tmpheight) { // Scale Up
			V_ScalingUpPhase(tmpheight, disp_height[plane_inx]);
			TmpFactor = (uint32_t)Phaseresult;
			IsScalingUp_V = 1;
		} else if (disp_height[plane_inx] > tmpheight) {
			V_ScalingDownPhase(tmpheight, disp_height[plane_inx]);
			TmpFactor = (uint32_t)Phaseresult;
			IsScalingUp_V = 0;
		} else {
			TmpFactor = 0;
		}
		VerIniFactor =  TmpFactor;
		DRM_INFO("[%s][%d] V_ratio:%x, V_inifactor: %x, ScalingUp %d\n"
			, __func__, __LINE__, VerRatio,
			VerIniFactor, IsScalingUp_V);

		paraThinReadFactorIn.thinReadFactor = Thinfactor;
		if (Hscale_enable && IsScalingUp_H)
			paramScalingIn.H_shift_mode = 1;
		else
			paramScalingIn.H_shift_mode = 0;

		if (Vscale_enable && IsScalingUp_V)
			paramScalingIn.V_shift_mode = 1;
		else
			paramScalingIn.V_shift_mode = 0;

		if (Hscale_enable || Vscale_enable)
			paramScalingIn.HVSP_bypass = 0;
		else
			paramScalingIn.HVSP_bypass = 1;

		paramScalingIn.Hscale_enable = Hscale_enable;
		paramScalingIn.IsScalingUp_H = IsScalingUp_H;
		paramScalingIn.HorIniFactor = HorIniFactor;
		paramScalingIn.HorRatio = HorRatio;
		paramScalingIn.HInputsize = crop_width[plane_inx];
		paramScalingIn.HOutputsize = disp_width[plane_inx];
		paramScalingIn.Vscale_enable = Vscale_enable;
		paramScalingIn.IsScalingUp_V = IsScalingUp_V;
		paramScalingIn.VerIniFactor = VerIniFactor;
		paramScalingIn.VerRatio = VerRatio;
		paramScalingIn.VInputsize = crop_height[plane_inx];
		paramScalingIn.VOutputsize = disp_height[plane_inx];

		DRM_INFO("[%s][%d][paramIn]Hscale_enable:%d H_shift_mode:%d Scalingup_H:%d\n",
		__func__, __LINE__, paramScalingIn.Hscale_enable, paramScalingIn.H_shift_mode,
		paramScalingIn.IsScalingUp_H);
		DRM_INFO("[%s][%d][paramIn]HorIniFactor:%x HorRatio:%x\n",
		__func__, __LINE__, paramScalingIn.HorIniFactor, paramScalingIn.HorRatio);
		DRM_INFO("[%s][%d][paramIn]InputSize_H:%d OutputSize_H:%d\n",
		__func__, __LINE__, paramScalingIn.HInputsize, paramScalingIn.HOutputsize);
		DRM_INFO("[%s][%d][paramIn]Vscale_enable:%d V_shift_mode:%d Scalingup_V:%d\n",
		__func__, __LINE__, paramScalingIn.Vscale_enable, paramScalingIn.V_shift_mode,
		paramScalingIn.IsScalingUp_V);
		DRM_INFO("[%s][%d][paramIn]VerIniFactor:%x VerRatio:%x\n",
		__func__, __LINE__, paramScalingIn.VerIniFactor, paramScalingIn.VerRatio);
		DRM_INFO("[%s][%d][paramIn]InputSize_V:%d OutputSize_V:%d\n",
		__func__, __LINE__, paramScalingIn.VInputsize, paramScalingIn.VOutputsize);

		if (pctx->blend_hw_version == 1) {
			drv_hwreg_render_display_set_linebuffersize(
				paraLinebuffersizeIn, &paramOut);
		}

		drv_hwreg_render_display_set_thinReadFactor(paraThinReadFactorIn, &paramOut);
		drv_hwreg_render_display_set_scalingratio(paramScalingIn, &paramOut);

		plane_ctx->hvspoldcropwindow.w = crop_width[plane_inx];
		plane_ctx->hvspoldcropwindow.h = crop_height[plane_inx];
		plane_ctx->hvspolddispwindow.w = disp_width[plane_inx];
		plane_ctx->hvspolddispwindow.h = disp_height[plane_inx];

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
			//3.add cmd
			cmd_info.cmd_cnt = paramOut.regCount;
			cmd_info.mem_index = memindex;
			cmd_info.reg = (struct sm_reg *)&reg;
			sm_ml_add_cmd(fd, &cmd_info);
			pctx_video->vgs_ml.regcount = pctx_video->vgs_ml.regcount
				+ paramOut.regCount;
		}
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
	enum drm_mtk_video_plane_type old_video_plane_type =
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->vgs_ml.memindex;
	int fd = pctx_video->vgs_ml.fd;
	struct mtk_video_plane_ctx *plane_ctx = pctx_video->plane_ctx + plane_inx;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_panel_context *pctx_pnl = NULL;
	struct window dispwindow[MAX_WINDOW_NUM] = {{0, 0, 0, 0}};
	struct windowRect dispwindowRect = {{0, 0, 0, 0}};

	struct reg_info reg[reg_num];
	struct hwregDispWinIn paramDispWinIn;
	struct hwregPreInsertImageSizeIn paramPreInsertImageSizeIn;
	struct hwregPreInsertFrameSizeIn paramPreInsertFrameSizeIn;

	struct sm_ml_add_info cmd_info;
	struct hwregOut paramOut;
	bool force = FALSE;
	bool bDispRectUpdate = FALSE;


	memset(reg, 0, sizeof(reg));
	memset(&paramDispWinIn, 0, sizeof(struct hwregDispWinIn));
	memset(&paramPreInsertImageSizeIn, 0, sizeof(paramPreInsertImageSizeIn));
	memset(&paramPreInsertFrameSizeIn, 0, sizeof(paramPreInsertFrameSizeIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	pctx = pctx_video_to_kms(pctx_video);
	pctx_pnl = &(pctx->panel_priv);

	if (plane_inx >= MAX_WINDOW_NUM) {
		DRM_ERROR("[%s][%d] Invalid plane_inx:%d !! MAX_WINDOW_NUM:%d\n",
			__func__, __LINE__, plane_inx, MAX_WINDOW_NUM);
		return;
	}

	paramOut.reg = reg;

	paramDispWinIn.RIU = g_bUseRIU;
	paramDispWinIn.windowID = plane_inx;

	paramPreInsertImageSizeIn.RIU = g_bUseRIU;
	paramPreInsertImageSizeIn.planeType = video_plane_type;

	paramPreInsertFrameSizeIn.RIU = g_bUseRIU;
	paramPreInsertFrameSizeIn.planeType = video_plane_type;

	dispwindow[plane_inx].x = plane_ctx->dispwindow.x;
	dispwindow[plane_inx].y = plane_ctx->dispwindow.y;
	dispwindow[plane_inx].w = plane_ctx->dispwindow.w;
	dispwindow[plane_inx].h = plane_ctx->dispwindow.h;


	if (pctx->stub)
		force = TRUE;

	bDispRectUpdate = ((_mtk_video_display_is_variable_updated(
		plane_ctx->dispolddispwindow.x, dispwindow[plane_inx].x)) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->dispolddispwindow.y, dispwindow[plane_inx].y)) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->dispolddispwindow.w, dispwindow[plane_inx].w)) ||
		(_mtk_video_display_is_variable_updated(
			plane_ctx->dispolddispwindow.h, dispwindow[plane_inx].h)));

	if (force ||
		(_mtk_video_display_is_variable_updated(
		old_video_plane_type, video_plane_type)) ||
		(bDispRectUpdate)
	) {

		DRM_INFO("[%s][%d] plane_inx:%d video_plane_type:%d old disp win:[%d, %d, %d, %d]\n"
			, __func__, __LINE__, plane_inx, video_plane_type,
			plane_ctx->dispolddispwindow.x, plane_ctx->dispolddispwindow.y,
			plane_ctx->dispolddispwindow.w, plane_ctx->dispolddispwindow.h);

		DRM_INFO("[%s][%d] plane_inx:%d video_plane_type:%d disp win:[%d, %d, %d, %d]\n",
			__func__, __LINE__, plane_inx, video_plane_type,
			dispwindow[plane_inx].x, dispwindow[plane_inx].y,
			dispwindow[plane_inx].w, dispwindow[plane_inx].h);

		paramDispWinIn.dispWindowRect.h_start = dispwindow[plane_inx].x;

		if (dispwindow[plane_inx].x + dispwindow[plane_inx].w >= 1) {
			paramDispWinIn.dispWindowRect.h_end =
				dispwindow[plane_inx].x + dispwindow[plane_inx].w - 1;

			if (paramDispWinIn.dispWindowRect.h_end > (pctx_pnl->info.de_width - 1)) {

				DRM_INFO("[%s][%d] plane_inx:%d plane_type:%d over support rect!\n",
					__func__, __LINE__, plane_inx, video_plane_type);

				DRM_INFO("[%s][%d] Refine disp h_end from %d to %d\n",
					__func__, __LINE__,
					paramDispWinIn.dispWindowRect.h_end,
					(pctx_pnl->info.de_width - 1));

				paramDispWinIn.dispWindowRect.h_end = (pctx_pnl->info.de_width - 1);

				drv_STUB("disp_win_refine_case_h", 1);
			}
		}

		paramDispWinIn.dispWindowRect.v_start = dispwindow[plane_inx].y;

		if (dispwindow[plane_inx].y + dispwindow[plane_inx].h >= 1) {
			paramDispWinIn.dispWindowRect.v_end =
				dispwindow[plane_inx].y + dispwindow[plane_inx].h - 1;

			if (paramDispWinIn.dispWindowRect.v_end > (pctx_pnl->info.de_height - 1)) {

				DRM_INFO("[%s][%d] plane_inx:%d plane_type:%d over support rect!\n",
					__func__, __LINE__, plane_inx, video_plane_type);

				DRM_INFO("[%s][%d] Refine disp v_end from %d to %d\n",
					__func__, __LINE__,
					paramDispWinIn.dispWindowRect.v_end,
					(pctx_pnl->info.de_height - 1));

				paramDispWinIn.dispWindowRect.v_end =
					(pctx_pnl->info.de_height - 1);

				drv_STUB("disp_win_refine_case_v", 1);
			}
		}

		DRM_INFO("[%s][%d][paramIn]winID:%d,disp rect(h_s,h_e,v_s,v_e):[%d, %d, %d, %d]\n",
			__func__, __LINE__, paramDispWinIn.windowID,
			paramDispWinIn.dispWindowRect.h_start,
			paramDispWinIn.dispWindowRect.h_end,
			paramDispWinIn.dispWindowRect.v_start,
			paramDispWinIn.dispWindowRect.v_end);

		drv_hwreg_render_display_set_dispWindow(paramDispWinIn, &paramOut);

		/* only (single win) need to set pre insert image size */
		/* MGW case always use MGW to fill frame size, */
		/* not use blending hw engine*/
		if (video_plane_type != MTK_VPLANE_TYPE_MULTI_WIN) {
			paramPreInsertImageSizeIn.imageSizeRect.h_start =
				paramDispWinIn.dispWindowRect.h_start;
			paramPreInsertImageSizeIn.imageSizeRect.h_end =
				paramDispWinIn.dispWindowRect.h_end;
			if (pctx->blend_hw_version == 0) {
				paramPreInsertImageSizeIn.imageSizeRect.v_start = 0;
				paramPreInsertImageSizeIn.imageSizeRect.v_end =
					paramDispWinIn.dispWindowRect.v_end -
					paramDispWinIn.dispWindowRect.v_start;
					//dispwindow[plane_inx].h - 1;
			} else {
				paramPreInsertImageSizeIn.imageSizeRect.v_start =
					paramDispWinIn.dispWindowRect.v_start;
				paramPreInsertImageSizeIn.imageSizeRect.v_end =
					paramDispWinIn.dispWindowRect.v_end;
			}

			drv_hwreg_render_display_set_preInsertImageSize(
				paramPreInsertImageSizeIn, &paramOut);

			if (pctx->blend_hw_version == 0) {
				paramPreInsertFrameSizeIn.htt = pctx_pnl->info.de_width - 1;
				paramPreInsertFrameSizeIn.vtt = dispwindow[plane_inx].h - 1;

				drv_hwreg_render_display_set_preInsertFrameSize(
					paramPreInsertFrameSizeIn, &paramOut);
			}
		}

		plane_ctx->dispolddispwindow.x = dispwindow[plane_inx].x;
		plane_ctx->dispolddispwindow.y = dispwindow[plane_inx].y;
		plane_ctx->dispolddispwindow.w = dispwindow[plane_inx].w;
		plane_ctx->dispolddispwindow.h = dispwindow[plane_inx].h;


		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
			//3.add cmd
			cmd_info.cmd_cnt = paramOut.regCount;
			cmd_info.mem_index = memindex;
			cmd_info.reg = (struct sm_reg *)&reg;
			sm_ml_add_cmd(fd, &cmd_info);
			pctx_video->vgs_ml.regcount = pctx_video->vgs_ml.regcount
				+ paramOut.regCount;
		}
	}
}

static void _mtk_video_display_set_fb_memory_format(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct drm_framebuffer *fb)
{
	struct mtk_tv_kms_context *pctx = NULL;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct drm_format_name_buf format_name;

	struct hwregMemConfigIn paramIn;
	struct reg_info reg[reg_num];

	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	bool force = FALSE;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregMemConfigIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;

	paramIn.RIU = g_bUseRIU;
	paramIn.planeType = video_plane_type;
	paramIn.windowID = plane_inx;
	paramIn.fourcc = fb->format->format;

	if (pctx->stub)
		force = TRUE;

	if (((fb->modifier >> 56) & 0xF) == DRM_FORMAT_MOD_VENDOR_MTK) {
		uint64_t modifier = fb->modifier & 0x00ffffffffffffffULL;

		paramIn.modifier = 1;
		paramIn.modifier_arrange = modifier & 0xff;
		paramIn.modifier_compressed = (modifier & 0x100) >> 8;
		paramIn.modifier_6bit = (modifier & 0x200) >> 9;
		paramIn.modifier_10bit = (modifier & 0x400) >> 10;
	}

	if ((video_plane_type > MTK_VPLANE_TYPE_NONE) ||
		(video_plane_type < MTK_VPLANE_TYPE_MAX)) {

		if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
			paramIn.MGWPlaneNum = pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN];

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

	DRM_INFO("[%s][%d][paramIn] %s%s %s(0x%08x) %s%d %s0x%x %s%d %s%d %s%d\n",
		__func__, __LINE__,
		"fourcc:", drm_get_format_name(fb->format->format, &format_name),
		"fourcc:", paramIn.fourcc,
		"modifier:", paramIn.modifier,
		"modifier_arrange:", paramIn.modifier_arrange,
		"modifier_compressed:", paramIn.modifier_compressed,
		"modifier_6bit:", paramIn.modifier_6bit,
		"modifier_10bit:", paramIn.modifier_10bit);

	if (force ||
		(memcmp(&(pctx_video->old_memcfg[video_plane_type][plane_inx]),
		&paramIn, sizeof(paramIn)) != 0)) {

		drv_hwreg_render_display_set_memConfig(paramIn, &paramOut);

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		memcpy(&(pctx_video->old_memcfg[video_plane_type][plane_inx]),
			&paramIn, sizeof(paramIn));

	}

	if (!g_bUseRIU && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + paramOut.regCount;
	}
}

static void _mtk_video_display_set_fb(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct drm_framebuffer *fb)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	struct mtk_video_format_info video_format_info;
	struct drm_format_name_buf format_name;
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	enum drm_mtk_video_plane_type old_video_plane_type =
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t reg_num = pctx_video->reg_num;
	uint16_t byte_per_word = pctx_video->byte_per_word;
	uint16_t render_p_engine = 0;
	uint32_t tmpframeOffset = 0;
	uint8_t MGW_plane_num = 0;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct mtk_video_plane_ctx *plane_ctx = pctx_video->plane_ctx + plane_inx;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_panel_context *pctx_pnl = NULL;
	uint16_t plane_num_per_frame = fb->format->num_planes;
	int i = 0;
	struct drm_gem_object *gem;
	struct mtk_drm_gem_obj *mtk_gem;
	dma_addr_t addr[4];
	unsigned int offsets[4] = {0};

	struct reg_info reg[reg_num];
	struct hwregFrameSizeIn paramFrameSizeIn;
	struct hwregLineOffsetIn paramLineOffsetIn;
	struct hwregFrameOffsetIn paramFrameOffsetIn;
	struct hwregBaseAddrIn paramBaseAddrIn;
	struct sm_ml_add_info cmd_info;

	struct hwregOut paramOut;

	memset(addr, 0, sizeof(addr));

	memset(reg, 0, sizeof(reg));
	memset(&paramFrameSizeIn, 0, sizeof(struct hwregFrameSizeIn));
	memset(&paramLineOffsetIn, 0, sizeof(struct hwregLineOffsetIn));
	memset(&paramFrameOffsetIn, 0, sizeof(struct hwregFrameOffsetIn));
	memset(&paramBaseAddrIn, 0, sizeof(struct hwregBaseAddrIn));

	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&video_format_info, 0, sizeof(struct mtk_video_format_info));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));
	pctx = pctx_video_to_kms(pctx_video);
	pctx_pnl = &(pctx->panel_priv);
	render_p_engine = pctx->render_p_engine;

	if (plane_inx >= MAX_WINDOW_NUM) {
		DRM_ERROR("[%s][%d] Invalid plane_inx:%d !! MAX_WINDOW_NUM:%d\n",
			__func__, __LINE__, plane_inx, MAX_WINDOW_NUM);
		return;
	}

	paramOut.reg = reg;

	paramFrameSizeIn.RIU = g_bUseRIU;
	/*set MGW win H_size/V_size*/
	/*Use MGW for fill frame size without using blending HW*/
	paramFrameSizeIn.hSize = pctx_pnl->info.de_width;
	paramFrameSizeIn.vSize = pctx_pnl->info.de_height;

	paramLineOffsetIn.RIU = g_bUseRIU;
	paramLineOffsetIn.planeType = video_plane_type;
	paramLineOffsetIn.windowID = plane_inx;
	paramLineOffsetIn.lineOffset =
		(fb->width + (render_p_engine - 1)) & ~(render_p_engine - 1);

	paramFrameOffsetIn.RIU = g_bUseRIU;
	paramFrameOffsetIn.planeType = video_plane_type;
	paramFrameOffsetIn.windowID = plane_inx;

	//frameOffset formula:
	//frameOffset = (align_256bit[align_4pixel[h_size]*bpp] / 8 / 32 )*[v_size]

	//Get bpp for plane 0
	video_format_info.fourcc = fb->format->format;
	if (((fb->modifier >> 56) & 0xF) == DRM_FORMAT_MOD_VENDOR_MTK) {
		video_format_info.modifier = fb->modifier & 0x00ffffffffffffffULL;
		video_format_info.modifier_arrange = video_format_info.modifier & 0xff;
		video_format_info.modifier_compressed = (video_format_info.modifier & 0x100) >> 8;
		video_format_info.modifier_6bit = (video_format_info.modifier & 0x200) >> 9;
		video_format_info.modifier_10bit = (video_format_info.modifier & 0x400) >> 10;
	}

	DRM_INFO("[%s][%d] %s%s %s(0x%08x) %s%d %s0x%x %s%d %s%d %s%d\n",
		__func__, __LINE__,
		"fourcc:", drm_get_format_name(fb->format->format, &format_name),
		"fourcc:", video_format_info.fourcc,
		"modifier:", video_format_info.modifier,
		"modifier_arrange:", video_format_info.modifier_arrange,
		"modifier_compressed:", video_format_info.modifier_compressed,
		"modifier_6bit:", video_format_info.modifier_6bit,
		"modifier_10bit:", video_format_info.modifier_10bit);

	_mtk_video_display_get_format_bpp(&video_format_info);

	DRM_INFO("[%s][%d] bpp[0]:%d\n", __func__, __LINE__, video_format_info.bpp[0]);

	for (i = 0 ; i < MTK_PLANE_DISPLAY_PIPE ; i++) {
		tmpframeOffset = paramLineOffsetIn.lineOffset * video_format_info.bpp[i];
		tmpframeOffset =
			(tmpframeOffset + (byte_per_word * BITS_PER_BYTE - 1))
			& ~(byte_per_word * BITS_PER_BYTE - 1);
		paramFrameOffsetIn.frameOffset[i] =
			(tmpframeOffset / BITS_PER_BYTE / byte_per_word) * fb->height;
	}

	paramBaseAddrIn.RIU = g_bUseRIU;
	paramBaseAddrIn.planeType = video_plane_type;
	paramBaseAddrIn.windowID = plane_inx;

	DRM_INFO("[%s][%d] plane_num_per_frame:%d\n", __func__, __LINE__, plane_num_per_frame);

	if (video_plane_type == MTK_VPLANE_TYPE_NONE) {
		DRM_INFO("[%s][%d] Not belong to any HW plane type\n",
			__func__, __LINE__);
	} else {
		if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
			MGW_plane_num =
				pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN];

			paramLineOffsetIn.MGWPlaneNum = MGW_plane_num;
			paramFrameOffsetIn.MGWPlaneNum = MGW_plane_num;
			paramBaseAddrIn.MGWPlaneNum = MGW_plane_num;
			DRM_INFO("[%s][%d] MGW_NUM:%d\n",
				__func__, __LINE__, MGW_plane_num);
		}
	}

	DRM_INFO("[%s][%d] plane_inx:%d old plane type:%d new plane type:%d\n",
		__func__, __LINE__, plane_inx, old_video_plane_type, video_plane_type);

	DRM_INFO("[%s][%d] plane_inx:%d  plane type change:%d\n",
		__func__, __LINE__, plane_inx, (_mtk_video_display_is_variable_updated(
		old_video_plane_type, video_plane_type)));

	DRM_INFO("[%s][%d] plane_inx:%d old MGW_plane_num:%d new MGW_plane_num:%d\n",
		__func__, __LINE__, plane_inx, plane_ctx->old_MGW_plane_num, MGW_plane_num);

	DRM_INFO("[%s][%d] plane_inx:%d  MGW num change:%d\n",
		__func__, __LINE__, plane_inx, (_mtk_video_display_is_variable_updated(
		plane_ctx->old_MGW_plane_num, MGW_plane_num)));


	if ((_mtk_video_display_is_variable_updated(
		plane_ctx->old_MGW_plane_num, MGW_plane_num)) ||
		(_mtk_video_display_is_variable_updated(
		old_video_plane_type, video_plane_type)) ||
		_mtk_video_display_is_variable_updated(plane_ctx->oldbufferwidth, fb->width) ||
		_mtk_video_display_is_variable_updated(plane_ctx->oldbufferheight, fb->height)) {

		DRM_INFO("[%s][%d] plane_inx:%d fb_width:%d fb_height:%d\n",
			__func__, __LINE__, plane_inx, fb->width, fb->height);

		//========================================================================//
		DRM_INFO("[%s][%d][paramIn] RIU:%d hSize:%d vSize:%d\n",
			__func__, __LINE__,
			paramBaseAddrIn.RIU,
			paramFrameSizeIn.hSize,
			paramFrameSizeIn.vSize);

		drv_hwreg_render_display_set_frameSize(
			paramFrameSizeIn, &paramOut);

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

		//=====================================================================//

		plane_ctx->oldbufferwidth = (uint64_t)fb->width;
		plane_ctx->oldbufferheight = (uint64_t)fb->height;

		if (video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) {
			plane_ctx->old_MGW_plane_num =
				pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN];
		}
	}

	for (i = 0; i < plane_num_per_frame; i++) {
		gem = fb->obj[0];
		mtk_gem = to_mtk_gem_obj(gem);
		offsets[i] = fb->offsets[i];
		addr[i] = mtk_gem->dma_addr + offsets[i];

		DRM_INFO("[%s][%d]mtk_gem->dma_addr: 0x%llx\n",
			__func__, __LINE__, mtk_gem->dma_addr);

		DRM_INFO("[%s][%d]fb_plane:%d addr:0x%llx offset:0x%lx\n",
			__func__, __LINE__, i, addr[i], offsets[i]);
	}
	//========================================================================//
	paramBaseAddrIn.addr.plane0 = div64_u64(addr[0], byte_per_word);
	paramBaseAddrIn.addr.plane1 = div64_u64(addr[1], byte_per_word);
	paramBaseAddrIn.addr.plane2 = div64_u64(addr[MTK_PLANE_DISPLAY_PIPE-1], byte_per_word);

	DRM_INFO("[%s][%d][paramIn]RIU:%d planeType:%d windowID:%d MGWNum:%d\n",
		__func__, __LINE__,
		paramBaseAddrIn.RIU,
		paramBaseAddrIn.planeType,
		paramBaseAddrIn.windowID,
		paramBaseAddrIn.MGWPlaneNum);

	DRM_INFO("[%s][%d][paramIn]plane0_addr:0x%lx plane1_addr:0x%lx plane2_addr:0x%lx\n",
		__func__, __LINE__,
		paramBaseAddrIn.addr.plane0,
		paramBaseAddrIn.addr.plane1,
		paramBaseAddrIn.addr.plane2);

	drv_hwreg_render_display_set_baseAddr(
		paramBaseAddrIn, &paramOut);


	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + paramOut.regCount;
	}

	mtk_video_display_set_frc_base_adr(mtk_gem->dma_addr);

	//========================================================================//
}

static void _mtk_video_display_get_mem_mode(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	enum video_mem_mode *mem_mode)
{
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_vplane_buf_mode buf_mode =
		plane_props->prop_val[EXT_VPLANE_PROP_BUF_MODE];
	enum drm_mtk_vplane_disp_mode disp_mode =
		plane_props->prop_val[EXT_VPLANE_PROP_DISP_MODE];

	if (plane_ctx->disp_mode_info.bChanged) {
		if (disp_mode == MTK_VPLANE_DISP_MODE_HW)
			*mem_mode = VIDEO_MEM_MODE_HW;
		else if (disp_mode == MTK_VPLANE_DISP_MODE_SW)
			*mem_mode = VIDEO_MEM_MODE_SW;
		else
			DRM_INFO("[%s, %d]: Not HWorSW mode state\n", __func__, __LINE__);
	} else {
		if (buf_mode == MTK_VPLANE_BUF_MODE_HW)
			*mem_mode = VIDEO_MEM_MODE_HW;
		else if (buf_mode == MTK_VPLANE_BUF_MODE_SW)
			*mem_mode = VIDEO_MEM_MODE_SW;
		else
			DRM_INFO("[%s, %d]:Not HWorSW mode state\n", __func__, __LINE__);
	}
}

static int _mtk_video_display_update_rw_diff_value(
	unsigned int plane_inx,
	struct mtk_tv_kms_context *pctx,
	struct mtk_plane_state *state,
	uint8_t *rwdiff,
	uint8_t *protectVal)
{
	int ret = 0;

	if (pctx == NULL || state == NULL) {
		DRM_ERROR("[%s][%d]Invalid input, pctx == NULL or state == NULL!!\n",
			__func__, __LINE__);
		ret = 1;
		return ret;
	}

	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_vplane_buf_mode vplane_buf_mode = MTK_VPLANE_BUF_MODE_MAX;
	uint8_t windowNum = 0;

	vplane_buf_mode = plane_props->prop_val[EXT_VPLANE_PROP_BUF_MODE];
	windowNum = pctx->video_priv.plane_num[MTK_VPLANE_TYPE_MEMC] +
		pctx->video_priv.plane_num[MTK_VPLANE_TYPE_MULTI_WIN] +
		pctx->video_priv.plane_num[MTK_VPLANE_TYPE_SINGLE_WIN1] +
		pctx->video_priv.plane_num[MTK_VPLANE_TYPE_SINGLE_WIN2];

	if (pctx->stub)
		pctx_pnl->outputTiming_info.locked_flag = true;

	switch (vplane_buf_mode) {
	case MTK_VPLANE_BUF_MODE_SW:
		*rwdiff = 0;
		*protectVal = 0;
		break;
	case MTK_VPLANE_BUF_MODE_HW:
		if ((windowNum == 1) &&
			(pctx_pnl->outputTiming_info.locked_flag == true) &&
			((uint16_t)state->base.crtc_w == pctx_pnl->info.de_width &&
				(uint16_t)state->base.crtc_h == pctx_pnl->info.de_height) &&
			((pctx_pnl->outputTiming_info.eFrameSyncMode ==
				VIDEO_CRTC_FRAME_SYNC_VTTV &&
				pctx_pnl->outputTiming_info.u8FRC_in == 1 &&
					(pctx_pnl->outputTiming_info.u8FRC_out == 1 ||
					pctx_pnl->outputTiming_info.u8FRC_out == 2 ||
					pctx_pnl->outputTiming_info.u8FRC_out == 4)) ||
				(pctx_pnl->outputTiming_info.eFrameSyncMode ==
				VIDEO_CRTC_FRAME_SYNC_FRAMELOCK))) {
			*rwdiff = 0;
			DRM_INFO("[%s][%d] rwdiff = 0\n", __func__, __LINE__);
			//enable DMA wbank protect
			switch (pctx_pnl->outputTiming_info.eFrameSyncMode) {
			case VIDEO_CRTC_FRAME_SYNC_VTTV:
				if (pctx_pnl->outputTiming_info.u8FRC_in == 1) {
					if (pctx_pnl->outputTiming_info.u8FRC_out == 1) {
						*protectVal =
							MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_LINE /
							MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_DIV;
					} else {
						*protectVal =
						((((state->base.src_h) >> SHIFT_16_BITS) /
							pctx_pnl->outputTiming_info.u8FRC_out *
							(pctx_pnl->outputTiming_info.u8FRC_out -
							pctx_pnl->outputTiming_info.u8FRC_in)) +
							MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_LINE) /
						MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_DIV;
					}
				}

				if (*protectVal > MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_MAX)
					*protectVal = MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_MAX;
				break;
			case VIDEO_CRTC_FRAME_SYNC_FRAMELOCK:
				*protectVal =
					MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_LINE /
					MTK_VIDEO_DISPLAY_RWDIFF_PROTECT_DIV;
				break;
			default:
				DRM_ERROR("[%s][%d] invalid framesync mode!!\n",
					__func__, __LINE__);
				ret = 1;
			}
		} else {
			DRM_INFO("[%s][%d] rwdiff = 1\n", __func__, __LINE__);
			*rwdiff = 1;
			*protectVal = 0;
		}
		break;
	default:
		DRM_ERROR("[%s][%d] video plane buf mode is invalid!!\n",
			__func__, __LINE__);
		ret = 1;
	}

	return ret;
}

static void _mtk_video_display_set_rw_diff(
	unsigned int plane_inx,
	struct mtk_tv_kms_context *pctx,
	struct mtk_plane_state *state)
{
	if (pctx == NULL || state == NULL) {
		DRM_ERROR("[%s][%d]Invalid input, pctx == NULL or state == NULL!!\n",
			__func__, __LINE__);
		return;
	}

	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	enum drm_mtk_vplane_disp_mode disp_mode =
		plane_props->prop_val[EXT_VPLANE_PROP_DISP_MODE];
	enum drm_mtk_vplane_disp_mode old_disp_mode =
		plane_props->old_prop_val[EXT_VPLANE_PROP_DISP_MODE];
	uint8_t rwdiff = 0;
	uint8_t protectVal = 0;
	int ret_upd = 0;

	uint16_t RegCount = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;

	struct reg_info reg[reg_num];
	struct hwregProtectValIn paramIn0;
	struct hwregRwDiffIn paramIn1;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	bool force = FALSE;

	if ((disp_mode != MTK_VPLANE_DISP_MODE_PRE_HW) &&
		(disp_mode != MTK_VPLANE_DISP_MODE_PRE_SW)) {
		ret_upd = _mtk_video_display_update_rw_diff_value(
			plane_inx, pctx, state, &rwdiff, &protectVal);
	}

	DRM_INFO("[%s][%d][paramIn] ret_upd:%d\n", __func__, __LINE__, ret_upd);

	if (ret_upd != 0) {//error case
		rwdiff = 1;
		protectVal = 0;
	}

	if (pctx->stub)
		force = TRUE;

	if (plane_ctx->disp_mode_info.disp_mode_update) {
		if (disp_mode == MTK_VPLANE_DISP_MODE_PRE_HW)
			rwdiff = 1;
	}

	DRM_INFO("[%s][%d] old_rwdiff:%d rwdiff:%d\n",
		__func__, __LINE__, plane_ctx->rwdiff, rwdiff);

	DRM_INFO("[%s][%d] is_rediff_update:%d\n",
		__func__, __LINE__,
		_mtk_video_display_is_variable_updated(plane_ctx->rwdiff, rwdiff));

	DRM_INFO("[%s][%d] old_protectVal:%d protectVal:%d\n",
		__func__, __LINE__, plane_ctx->protectVal, protectVal);

	DRM_INFO("[%s][%d] is_protectVal_update:%d\n",
		__func__, __LINE__,
		_mtk_video_display_is_variable_updated(plane_ctx->protectVal, protectVal));

	if (force || _mtk_video_display_is_variable_updated(plane_ctx->rwdiff, rwdiff) ||
		_mtk_video_display_is_variable_updated(plane_ctx->protectVal, protectVal)) {
		memset(reg, 0, sizeof(reg));
		memset(&paramIn0, 0, sizeof(struct hwregProtectValIn));
		memset(&paramIn1, 0, sizeof(struct hwregRwDiffIn));
		memset(&paramOut, 0, sizeof(struct hwregOut));
		memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

		paramOut.reg = reg;

		paramIn0.RIU = g_bUseRIU;
		paramIn0.protectVal = protectVal;
		DRM_INFO("[%s][%d][paramIn] RIU:%d protectVal:%d\n",
			__func__, __LINE__,
			paramIn0.RIU, paramIn0.protectVal);
		drv_hwreg_render_display_set_rbankRefProtect(
			paramIn0, &paramOut);
		RegCount = paramOut.regCount;
		paramOut.reg = reg + RegCount;

		paramIn1.RIU = g_bUseRIU;
		paramIn1.windowID = plane_inx;
		paramIn1.rwDiff = rwdiff;
		DRM_INFO("[%s][%d][paramIn] RIU:%d windowID:%d, rwDiff:%d\n",
			__func__, __LINE__,
			paramIn1.RIU, paramIn1.windowID, paramIn1.rwDiff);
		drv_hwreg_render_display_set_rwDiff(
			paramIn1, &paramOut);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;

		/*old rw diff*/
		plane_ctx->rwdiff = rwdiff;
		/*old protectVal*/
		plane_ctx->protectVal = protectVal;

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, RegCount);

		if ((!g_bUseRIU) && (RegCount != 0)) {
			//3.add cmd
			cmd_info.cmd_cnt = RegCount;
			cmd_info.mem_index = memindex;
			cmd_info.reg = (struct sm_reg *)&reg;
			sm_ml_add_cmd(fd, &cmd_info);
			pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + RegCount;
		}
	}
}

static void _mtk_video_display_set_freeze(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	uint16_t Freeze = plane_props->prop_val[EXT_VPLANE_PROP_FREEZE];
	uint16_t CurrentRWdiff = 0;
	bool bUpdateFreeze = FALSE;

	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	static uint16_t memc_windowID = MEMC_WINID_DEFAULT;

	struct reg_info reg[reg_num];
	struct hwregFreezeIn paramIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregFreezeIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramOut.reg = reg;

	paramIn.RIU = g_bUseRIU;
	paramIn.windowID = plane_inx;


	if (_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_FREEZE)) {
		plane_ctx->disp_mode_info.freeze =
			plane_props->prop_val[EXT_VPLANE_PROP_FREEZE];

		DRM_INFO("[%s][%d] plane_inx:%d freeze:%d\n",
			__func__, __LINE__,
			plane_inx, plane_ctx->disp_mode_info.freeze);
	}

	if (plane_ctx->disp_mode_info.disp_mode == MTK_VPLANE_DISP_MODE_PRE_SW) {
		drv_hwreg_render_display_get_rwDiff(plane_inx, &CurrentRWdiff);

		if ((CurrentRWdiff > 0)) {
			bUpdateFreeze = TRUE;

			paramIn.enable = plane_ctx->disp_mode_info.freeze;
		} else {
			bUpdateFreeze = FALSE;
		}

		DRM_INFO("[%s][%d] plane_inx:%d CurrentRWdiff:%d freeze:%d bUpdateFreeze:%d\n",
			__func__, __LINE__,
			plane_inx, CurrentRWdiff,
			plane_ctx->disp_mode_info.freeze, bUpdateFreeze);
	} else {
		bUpdateFreeze = _mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_FREEZE);

		paramIn.enable = plane_props->prop_val[EXT_VPLANE_PROP_FREEZE];

		DRM_INFO("[%s][%d] plane_inx:%d freeze:%d bUpdateFreeze:%d\n",
			__func__, __LINE__,
			plane_inx, plane_props->prop_val[EXT_VPLANE_PROP_FREEZE],
			bUpdateFreeze);
	}

	if (bUpdateFreeze) {

		DRM_INFO("[%s][%d][paramIn] RIU:%d freeze_en:%d\n",
			__func__, __LINE__,
			paramIn.RIU, paramIn.enable);

		drv_hwreg_render_display_set_freeze(paramIn, &paramOut);

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		if ((plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE]
			== MTK_VPLANE_TYPE_MEMC) && (paramIn.enable == 1)) {
			memc_windowID = paramIn.windowID;
		}

		if (memc_windowID == paramIn.windowID) {
			mtk_video_display_set_frc_freeze(pctx_video, paramIn.enable);
			if (paramIn.enable == 0) {
				memc_windowID = MEMC_WINID_DEFAULT;
			}
		}

		if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
			//3.add cmd
			cmd_info.cmd_cnt = paramOut.regCount;
			cmd_info.mem_index = memindex;
			cmd_info.reg = (struct sm_reg *)reg;
			sm_ml_add_cmd(fd, &cmd_info);
			pctx_video->disp_ml.regcount =
				pctx_video->disp_ml.regcount + paramOut.regCount;
		}

	}
}

static void _mtk_video_display_set_BackgroundAlpha(
	struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	enum drm_hwreg_video_plane_type plane_type = VPLANE_TYPE_NONE;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct hwregPostFillBgARGBIn paramIn;
	struct hwregOut paramOut;

	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	// set background alpha to @WINDOW_ALPHA_DEFAULT_VALUE
	memset(&paramIn, 0, sizeof(struct hwregPostFillBgARGBIn));
	paramIn.RIU = g_bUseRIU;
	paramIn.bgARGB.alpha = WINDOW_ALPHA_DEFAULT_VALUE;
	for (plane_type = VPLANE_TYPE_MEMC; plane_type < VPLANE_TYPE_SINGLE_WIN2; ++plane_type) {
		memset(reg, 0, sizeof(reg));
		paramOut.regCount = 0;
		paramIn.planeType = plane_type;
		drv_hwreg_render_display_set_postFillBgAlpha(paramIn, &paramOut);
	}

}

static void _mtk_video_display_set_windowAlpha(
	struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	enum drm_hwreg_video_plane_type plane_type = VPLANE_TYPE_NONE;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct hwregPostFillWindowARGBIn paramPostFillWindowARGBIn;
	struct hwregOut paramOut;
	bool bRIU = TRUE;

	memset(&paramOut, 0, sizeof(paramOut));
	paramOut.reg = reg;

	// set window alpha to @WINDOW_ALPHA_DEFAULT_VALUE
	memset(&paramPostFillWindowARGBIn, 0, sizeof(struct hwregPostFillWindowARGBIn));
	paramPostFillWindowARGBIn.RIU = bRIU;
	paramPostFillWindowARGBIn.windowARGB.alpha = WINDOW_ALPHA_DEFAULT_VALUE;
	for (plane_type = VPLANE_TYPE_MEMC; plane_type <= VPLANE_TYPE_SINGLE_WIN2; ++plane_type) {
		memset(reg, 0, sizeof(reg));
		paramOut.regCount = 0;
		paramPostFillWindowARGBIn.planeType = plane_type;
		drv_hwreg_render_display_set_postFillWindowAlpha(
			paramPostFillWindowARGBIn, &paramOut);
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
	enum drm_mtk_video_plane_type old_video_plane_type =
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	enum video_mem_mode mem_mode = VIDEO_MEM_MODE_MAX;
	bool bmuteSWModeEn = FALSE;

	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;

	struct reg_info reg[reg_num];
	struct hwregPostFillMuteScreenIn MuteParamIn;
	struct hwregAutoNoSignalIn AutoNoSignalParamIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	memset(reg, 0, sizeof(reg));
	memset(&MuteParamIn, 0, sizeof(struct hwregPostFillMuteScreenIn));
	memset(&AutoNoSignalParamIn, 0, sizeof(struct hwregAutoNoSignalIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramOut.reg = reg;

	/*get mem mode*/
	_mtk_video_display_get_mem_mode(plane_inx, pctx_video, &mem_mode);

	if (mem_mode == VIDEO_MEM_MODE_HW)
		bmuteSWModeEn = FALSE;
	else
		bmuteSWModeEn = TRUE;

	if ((old_video_plane_type != MTK_VPLANE_TYPE_NONE) &&
		(_mtk_video_display_is_variable_updated(
		old_video_plane_type, video_plane_type))) {
		MuteParamIn.RIU = g_bUseRIU;
		MuteParamIn.planeType = old_video_plane_type;
		MuteParamIn.windowID = plane_inx;
		MuteParamIn.muteEn = !plane_props->old_prop_val[EXT_VPLANE_PROP_MUTE_SCREEN];
		MuteParamIn.muteSWModeEn = bmuteSWModeEn;
		drv_hwreg_render_display_set_postFillMuteScreen(MuteParamIn, &paramOut);
	}

	/* Mute parameter*/
	MuteParamIn.RIU = g_bUseRIU;
	MuteParamIn.planeType = video_plane_type;
	MuteParamIn.windowID = plane_inx;
	MuteParamIn.muteEn = plane_props->prop_val[EXT_VPLANE_PROP_MUTE_SCREEN];
	MuteParamIn.muteSWModeEn = bmuteSWModeEn;

	DRM_INFO("[%s][%d][paramIn] RIU:%d windowID:%d, planeType:%d, muteEn:%d muteSWModeEn:%d\n",
		__func__, __LINE__,
		MuteParamIn.RIU, MuteParamIn.windowID, MuteParamIn.planeType,
		MuteParamIn.muteEn, MuteParamIn.muteSWModeEn);

	/* Auto no signal parameter*/
	AutoNoSignalParamIn.RIU = g_bUseRIU;
	AutoNoSignalParamIn.planeType = video_plane_type;

	if (mem_mode == VIDEO_MEM_MODE_HW)
		AutoNoSignalParamIn.bEnable = TRUE;
	else
		AutoNoSignalParamIn.bEnable = FALSE;

	DRM_INFO("[%s][%d][paramIn] RIU:%d windowID:%d, planeType:%d, Auto no signal enable:%d\n",
		__func__, __LINE__,
		AutoNoSignalParamIn.RIU, MuteParamIn.windowID,
		AutoNoSignalParamIn.planeType, AutoNoSignalParamIn.bEnable);

	drv_hwreg_render_display_set_auto_no_signal(AutoNoSignalParamIn, &paramOut);

	drv_hwreg_render_display_set_postFillMuteScreen(MuteParamIn, &paramOut);

	if (MuteParamIn.muteEn)
		trace_printk("mute video\n");
	else
		trace_printk("unmute video\n");

	if (!MuteParamIn.muteEn)
		boottime_print("MTK render unmute [end]\n");

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + paramOut.regCount;
	}
}

static void _mtk_video_display_set_blend_size(struct mtk_tv_kms_context *pctx, bool binit)
{
	struct mtk_panel_context *pctx_pnl = &(pctx->panel_priv);
	struct mtk_video_context *pctx_video = &(pctx->video_priv);
	struct hwregBlendOutSizeIn paramBlendOutSizeIn;
	struct hwregPreInsertFrameSizeIn paramPreInsertFrameSizeIn;
	struct hwregPostFillFrameSizeIn paramPostFillFrameSizeIn;
	struct hwregOut paramOut;
	enum drm_hwreg_video_plane_type plane_type = VPLANE_TYPE_NONE;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct sm_ml_add_info cmd_info;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;

	static uint16_t oldPanelDE_W, oldPanelDE_H;
	bool bRIU = g_bUseRIU;

	memset(&paramOut, 0, sizeof(paramOut));
	paramOut.reg = reg;

	memset(reg, 0, sizeof(reg));

	if (binit)
		bRIU = TRUE;
	else
		bRIU = FALSE;

	DRM_INFO("[%s][%d] binit %d\n",
		__func__, __LINE__, binit);

	DRM_INFO("[%s][%d][old] panel DE (width,height):(%d, %d), panel DE (width,height):(%d, %d)",
		__func__, __LINE__,
		oldPanelDE_W, oldPanelDE_H,
		pctx_pnl->info.de_width, pctx_pnl->info.de_height);

	DRM_INFO("[%s][%d] blend size (width,height) update:(%d, %d)",
		__func__, __LINE__,
		_mtk_video_display_is_variable_updated(oldPanelDE_W, pctx_pnl->info.de_width),
		_mtk_video_display_is_variable_updated(oldPanelDE_H, pctx_pnl->info.de_height));

	if (binit ||
		(_mtk_video_display_is_variable_updated(oldPanelDE_W, pctx_pnl->info.de_width)) ||
		(_mtk_video_display_is_variable_updated(oldPanelDE_H, pctx_pnl->info.de_height))) {

		/* set de_size to blend_out */
		memset(&paramBlendOutSizeIn, 0, sizeof(paramBlendOutSizeIn));
		paramBlendOutSizeIn.RIU = bRIU;
		if (pctx->blend_hw_version == 0)
			paramBlendOutSizeIn.Hsize = pctx_pnl->info.de_width / BLEND_VER0_HSZIE_UNIT;
		else
			paramBlendOutSizeIn.Hsize = pctx_pnl->info.de_width;

		drv_hwreg_render_display_set_blendOut_size(paramBlendOutSizeIn, &paramOut);


		/* set panel de_size to pre insert frame size */
		memset(&paramPreInsertFrameSizeIn, 0, sizeof(paramPreInsertFrameSizeIn));
		paramPreInsertFrameSizeIn.RIU = bRIU;
		paramPreInsertFrameSizeIn.htt = pctx_pnl->info.de_width - 1;
		paramPreInsertFrameSizeIn.vtt = pctx_pnl->info.de_height - 1;

		for (plane_type = VPLANE_TYPE_MEMC;
			plane_type <= VPLANE_TYPE_SINGLE_WIN2; ++plane_type) {
			paramPreInsertFrameSizeIn.planeType = plane_type;

			drv_hwreg_render_display_set_preInsertFrameSize(
				paramPreInsertFrameSizeIn, &paramOut);
		}

		/* set panel de_size to post fill frame size */
		memset(&paramPostFillFrameSizeIn, 0, sizeof(paramPostFillFrameSizeIn));
		paramPostFillFrameSizeIn.RIU = bRIU;
		paramPostFillFrameSizeIn.htt = pctx_pnl->info.de_width - 1;
		paramPostFillFrameSizeIn.vtt = pctx_pnl->info.de_height - 1;

		for (plane_type = VPLANE_TYPE_MEMC;
			plane_type <= VPLANE_TYPE_SINGLE_WIN2; ++plane_type) {
			paramPostFillFrameSizeIn.planeType = plane_type;

			drv_hwreg_render_display_set_postFillFrameSize(
				paramPostFillFrameSizeIn, &paramOut);

		}

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		if ((!bRIU) && (paramOut.regCount != 0)) {
			//3.add cmd
			cmd_info.cmd_cnt = paramOut.regCount;
			cmd_info.mem_index = memindex;
			cmd_info.reg = (struct sm_reg *)reg;
			sm_ml_add_cmd(fd, &cmd_info);
			pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount +
				paramOut.regCount;
		}

		oldPanelDE_W = pctx_pnl->info.de_width;
		oldPanelDE_H = pctx_pnl->info.de_height;
	}
}

static void _mtk_video_display_set_enable_pre_insert(
	struct mtk_video_context *pctx_video)
{
	struct mtk_tv_kms_context *pctx = NULL;
	enum drm_mtk_video_plane_type video_plane_type = 0;
	struct hwregPreInsertEnableIn paramIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct sm_ml_add_info cmd_info;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	bool force = FALSE;

	memset(&paramIn, 0, sizeof(paramIn));
	memset(&cmd_info, 0, sizeof(cmd_info));
	memset(&paramOut, 0, sizeof(paramOut));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;

	/* enable pre insert */
	paramIn.RIU = g_bUseRIU;

	if (pctx->stub)
		force = TRUE;

	for (video_plane_type = VPLANE_TYPE_MEMC;
					video_plane_type <= VPLANE_TYPE_SINGLE_WIN2;
					++video_plane_type) {
		paramIn.planeType = video_plane_type;
		if ((pctx_video->plane_num[video_plane_type] == 1) &&
			(paramIn.planeType != VPLANE_TYPE_MULTI_WIN))
			// only single win need enable, MGW always no need
			paramIn.enable = true;
		else
			paramIn.enable = false;

		if (force || (paramIn.enable != pctx_video->preinsertEnable[video_plane_type])) {
			drv_hwreg_render_display_set_preInsertEnable(paramIn, &paramOut);

			DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
				__func__, __LINE__, paramOut.regCount);

			pctx_video->preinsertEnable[video_plane_type] = paramIn.enable;
		}
	}

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount
			+ paramOut.regCount;
	}
}

static void _mtk_video_display_set_enable_post_fill(
	struct mtk_video_context *pctx_video)
{
	enum drm_mtk_video_plane_type video_plane_type = 0;
	struct hwregPostFillEnableIn paramIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct sm_ml_add_info cmd_info;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct mtk_tv_kms_context *pctx = NULL;
	bool enable = false;
	bool force = FALSE;

	memset(&paramIn, 0, sizeof(paramIn));
	memset(&cmd_info, 0, sizeof(cmd_info));
	memset(&paramOut, 0, sizeof(paramOut));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;

	/* enable post fill */
	paramIn.RIU = g_bUseRIU;

	if (pctx->stub)
		force = TRUE;

	for (video_plane_type = VPLANE_TYPE_MEMC;
					video_plane_type <= VPLANE_TYPE_SINGLE_WIN2;
					++video_plane_type) {
		paramIn.planeType = video_plane_type;

		if ((video_plane_type != MTK_VPLANE_TYPE_MULTI_WIN) &&
			(pctx_video->plane_num[video_plane_type] >= 1)) {
			enable = true;
		}
		paramIn.enable = enable;

		if (force || (paramIn.enable != pctx_video->postfillEnable[video_plane_type])) {
			drv_hwreg_render_display_set_postFillEnable(paramIn, &paramOut);

			DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
				__func__, __LINE__, paramOut.regCount);

			pctx_video->postfillEnable[video_plane_type] = paramIn.enable;
		}
	}

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount
			+ paramOut.regCount;
	}
}

static void _mtk_video_display_set_post_fill_win_enable(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	bool enable)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	struct mtk_video_plane_ctx *plane_ctx =
		pctx_video->plane_ctx + plane_inx;
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	enum drm_mtk_video_plane_type old_video_plane_type =
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t oldPostFillWinEn = plane_ctx->oldPostFillWinEn;
	struct hwregPostFillWinEnableIn postFillWinEnIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct sm_ml_add_info cmd_info;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct mtk_tv_kms_context *pctx = NULL;
	bool force = FALSE;

	memset(&postFillWinEnIn, 0, sizeof(postFillWinEnIn));
	memset(&cmd_info, 0, sizeof(cmd_info));
	memset(&paramOut, 0, sizeof(paramOut));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;

	DRM_INFO("[%s][%d] windowID %d planeType:%d\n",
		__func__, __LINE__, plane_inx, postFillWinEnIn.planeType);
	DRM_INFO("[%s][%d] windowID %d oldPostFillWinEn:%d enable:%d\n",
		__func__, __LINE__, plane_inx, oldPostFillWinEn, enable);
	DRM_INFO("[%s][%d] windowID %d post fill en update:%d\n",
		__func__, __LINE__, plane_inx,
		_mtk_video_display_is_variable_updated(oldPostFillWinEn, enable));
	DRM_INFO("[%s][%d] windowID %d post fill en update:%d\n",
		__func__, __LINE__, plane_inx,
		_mtk_video_display_is_variable_updated(old_video_plane_type, video_plane_type));

	if (pctx->stub)
		force = TRUE;

	if (force ||
		(_mtk_video_display_is_variable_updated(oldPostFillWinEn, enable)) ||
		(_mtk_video_display_is_variable_updated(
			old_video_plane_type, video_plane_type))) {

		postFillWinEnIn.RIU = g_bUseRIU;
		postFillWinEnIn.planeType = (!enable ? video_plane_type : old_video_plane_type);
		postFillWinEnIn.windowID = plane_inx;
		postFillWinEnIn.enable = !enable;
		drv_hwreg_render_display_set_postFillWinEnable(postFillWinEnIn, &paramOut);

		/* enable post window mask */
		postFillWinEnIn.RIU = g_bUseRIU;
		postFillWinEnIn.planeType = (enable ? video_plane_type : old_video_plane_type);
		postFillWinEnIn.windowID = plane_inx;
		postFillWinEnIn.enable = enable;
		drv_hwreg_render_display_set_postFillWinEnable(postFillWinEnIn, &paramOut);

		DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
			__func__, __LINE__, paramOut.regCount);

		plane_ctx->oldPostFillWinEn = enable;
	}

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount
			+ paramOut.regCount;
	}
}

static void _mtk_video_display_set_HW_mode(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	bool bHWmode)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	struct mtk_video_plane_ctx *plane_ctx =
		pctx_video->plane_ctx + plane_inx;
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	enum drm_mtk_video_plane_type old_video_plane_type =
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint16_t old_disp_mode = plane_ctx->old_disp_mode;
	uint16_t old_mem_hw_mode = plane_ctx->old_mem_hw_mode;
	struct hwregMemSWmodeIn memSWmodeIn;
	struct hwregExtWinAutoEnIn extWinAutoEnIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct sm_ml_add_info cmd_info;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct mtk_tv_kms_context *pctx = NULL;
	bool mem_hw_mode = bHWmode;
	bool extWinAutoEn = 0;

	memset(&memSWmodeIn, 0, sizeof(memSWmodeIn));
	memset(&extWinAutoEnIn, 0, sizeof(extWinAutoEnIn));
	memset(&cmd_info, 0, sizeof(cmd_info));
	memset(&paramOut, 0, sizeof(paramOut));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;

	DRM_INFO("[%s, %d]: mem_hw_mode:%d\n", __func__, __LINE__, mem_hw_mode);

	if (mem_hw_mode) {//hw mode
		extWinAutoEn = 1;//see multi bank
	} else {//sw mode
		extWinAutoEn = 0;//see ext bank
	}

	memSWmodeIn.RIU = g_bUseRIU;
	memSWmodeIn.planeType = video_plane_type;
	memSWmodeIn.windowID = plane_inx;
	memSWmodeIn.bSWmode = !mem_hw_mode;

	extWinAutoEnIn.RIU = g_bUseRIU;
	extWinAutoEnIn.planeType = video_plane_type;
	extWinAutoEnIn.enable = extWinAutoEn;

	DRM_INFO("[%s][%d] windowID %d, planeType:%d, mem_hw_mode:%d, old_mem_hw_mode:%d\n",
		__func__, __LINE__, plane_inx, video_plane_type,
		mem_hw_mode, old_mem_hw_mode);

	drv_hwreg_render_display_set_mem_sw_mode(memSWmodeIn, &paramOut);
	drv_hwreg_render_display_set_extwin_auto_en(extWinAutoEnIn, &paramOut);

	plane_ctx->old_mem_hw_mode = mem_hw_mode;

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount
			+ paramOut.regCount;
	}
}

static void _mtk_video_display_update_HW_mode(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video
)
{
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	enum drm_mtk_vplane_buf_mode buf_mode =
		plane_props->prop_val[EXT_VPLANE_PROP_BUF_MODE];
	enum drm_mtk_vplane_disp_mode disp_mode =
		plane_props->prop_val[EXT_VPLANE_PROP_DISP_MODE];

	DRM_INFO("[%s, %d]buf_mode:%d disp_mode:%d\n",
		__func__, __LINE__, buf_mode, disp_mode);

	if (_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_DISP_MODE)){
		plane_ctx->disp_mode_info.disp_mode_update = TRUE;
		plane_ctx->disp_mode_info.disp_mode = disp_mode;
		plane_ctx->disp_mode_info.bChanged = TRUE;
	} else {
		plane_ctx->disp_mode_info.disp_mode_update = FALSE;
	}

	if (plane_ctx->disp_mode_info.disp_mode_update) {
		if (disp_mode == MTK_VPLANE_DISP_MODE_HW)
			_mtk_video_display_set_HW_mode(plane_inx, pctx_video, TRUE);
		else if (disp_mode == MTK_VPLANE_DISP_MODE_SW)
			_mtk_video_display_set_HW_mode(plane_inx, pctx_video, FALSE);
		else
			DRM_INFO("[%s, %d]: Not HWorSW mode state\n", __func__, __LINE__);
	} else if (_mtk_video_display_is_ext_prop_updated(plane_props,
			EXT_VPLANE_PROP_BUF_MODE)) {
		if (buf_mode == MTK_VPLANE_BUF_MODE_HW)
			_mtk_video_display_set_HW_mode(plane_inx, pctx_video, TRUE);
		else if (buf_mode == MTK_VPLANE_BUF_MODE_SW)
			_mtk_video_display_set_HW_mode(plane_inx, pctx_video, FALSE);
		else
			DRM_INFO("[%s, %d]:Not HWorSW mode state\n", __func__, __LINE__);
	}

}

static void _mtk_add_cmd(
	struct mtk_video_context *pctx_video,
	struct hwregLayerControlIn paramIn,
	struct hwregOut paramOut,
	struct reg_info *preg)
{
	struct sm_ml_add_info cmd_info;
	int fd = pctx_video->disp_ml.fd;
	uint8_t memindex = pctx_video->disp_ml.memindex;

	if ((!paramIn.RIU) && (paramOut.regCount != 0)) {
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)preg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount +
			paramOut.regCount;
	}
}

static void _mtk_video_display_set_zpos(
	struct mtk_video_context *pctx_video,
	bool binit)
{
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;

	struct reg_info reg[reg_num];
	struct hwregLayerControlIn paramIn;

	struct hwregOut paramOut;

	enum drm_mtk_video_plane_type plane_type = 0;
	uint8_t plane_count[MTK_VPLANE_TYPE_MAX];
	uint8_t layer[MTK_VPLANE_TYPE_MAX];
	bool layer_en[MTK_VPLANE_TYPE_MAX];
	bool used_layer[4];
	bool bRIU = FALSE;
	int i = 0;
	struct mtk_tv_kms_context *pctx = NULL;
	bool force = FALSE;

	memset(&paramIn, 0, sizeof(struct hwregLayerControlIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(plane_count, 0, sizeof(plane_count));
	memset(layer, 0, sizeof(layer));
	memset(layer_en, 0, sizeof(layer_en));
	memset(used_layer, 0, sizeof(used_layer));

	pctx = pctx_video_to_kms(pctx_video);

	paramOut.reg = reg;

	if (pctx->stub)
		force = TRUE;


	if (binit)
		bRIU = TRUE;
	else
		bRIU = FALSE;

	/* prepare enable layer args first */
	for (plane_type = 0; plane_type < MTK_VPLANE_TYPE_MAX; ++plane_type) {
		/*
		 * BYPASS mode patch: if not BYPASS mode, should not use MTK_VPLANE_TYPE_NONE.
		 * Should distiguish it using HW/SW/BYPASS property.
		 */
		if (plane_type == MTK_VPLANE_TYPE_NONE)
			continue;

		/* get plane count to check this src is disable or not */
		plane_count[plane_type] = pctx_video->plane_num[plane_type];

		DRM_INFO("[%s][%d] plane_type:%d plane_count:%d\n",
			__func__, __LINE__, plane_type, plane_count[plane_type]);

		if (plane_count[plane_type] > 0) {
			layer_en[plane_type] = true;
			layer[plane_type] = pctx_video->zpos_layer[plane_type];
			used_layer[layer[plane_type]] = 1;
		}
	}

	DRM_INFO("[%s][%d] used_layer=[%d, %d, %d, %d]\n",
		__func__, __LINE__,
		used_layer[0], used_layer[1], used_layer[2], used_layer[3]);

	/* prepare disable layer args */
	for (plane_type = 0; plane_type < MTK_VPLANE_TYPE_MAX; ++plane_type) {

		/*
		 * BYPASS mode patch: if not BYPASS mode, should not use MTK_VPLANE_TYPE_NONE.
		 * Should distiguish it using HW/SW/BYPASS property.
		 */
		if (plane_type == MTK_VPLANE_TYPE_NONE)
			continue;

		if (plane_count[plane_type] == 0) {
			/* this plane type is unused */
			layer_en[plane_type] = false;

			/* for hw limitation, need to set disable layer to unused plane */
			for (i = 0; i < 4; ++i) {
				if (used_layer[i] == 0) {
					layer[plane_type] = i;
					used_layer[i] = 1;
					break;
				}
			}
		}
	}

	/* set zpos */
	for (plane_type = 0; plane_type < MTK_VPLANE_TYPE_MAX; ++plane_type) {

		/*
		 * BYPASS mode patch: if not BYPASS mode, should not use MTK_VPLANE_TYPE_NONE.
		 * Should distiguish it using HW/SW/BYPASS property.
		 */
		if (plane_type == MTK_VPLANE_TYPE_NONE)
			continue;

		DRM_INFO("[%s][%d] %s%d, %s%d, %s%d, %s%d, %s%d, %s%d\n",
			__func__, __LINE__,
			"plane_type=", plane_type,
			"plane_count=", plane_count[plane_type],
			"old_layer_en=", pctx_video->old_layer_en[plane_type],
			"layer_en=", layer_en[plane_type],
			"old_layer=", pctx_video->old_layer[plane_type],
			"layer=", layer[plane_type]);

		/* check if update, call hwreg */
		if (force ||
			(_mtk_video_display_is_variable_updated(
				pctx_video->old_layer[plane_type], layer[plane_type]) ||
				_mtk_video_display_is_variable_updated(
				pctx_video->old_layer_en[plane_type], layer_en[plane_type]))) {

			/* prepare hwreg args */
			paramIn.RIU = bRIU;
			paramIn.layerEn = layer_en[plane_type];
			paramIn.layer = layer[plane_type];

			switch (plane_type) {
			//case MTK_VPLANE_TYPE_NONE: //TO DO
			case MTK_VPLANE_TYPE_MEMC:
				paramIn.srcIndex = 0;
				break;
			case MTK_VPLANE_TYPE_MULTI_WIN:
				paramIn.srcIndex = 1;
				break;
			case MTK_VPLANE_TYPE_SINGLE_WIN1:
				paramIn.srcIndex = 2;
				break;
			case MTK_VPLANE_TYPE_SINGLE_WIN2:
				paramIn.srcIndex = 3;
				break;
			default:
				break;
			}
			DRM_INFO("[%s][%d][paramIn] RIU=%d, layerEn=%d, srcIndex=%d, layer=%d\n",
				__func__, __LINE__, paramIn.RIU, paramIn.layerEn, paramIn.srcIndex,
				paramIn.layer);

			// call hwreg
			drv_hwreg_render_display_set_layerControl(paramIn, &paramOut);

			DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
				__func__, __LINE__, paramOut.regCount);

			/* add ml cmd if not riu */
			_mtk_add_cmd(pctx_video, paramIn, paramOut, &reg[0]);
		}
	}

	/* update old val */
	memcpy(pctx_video->old_layer, layer, sizeof(layer));
	memcpy(pctx_video->old_layer_en, layer_en, sizeof(layer_en));
}

static void _mtk_video_display_set_pqmap(struct mtk_tv_kms_context *pctx)
{
	DRM_INFO("[%s][%d] Enter", __func__, __LINE__);
	mtk_tv_drm_pqmap_trigger(&pctx->pqmap_priv);
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

#if (1) //(MEMC_CONFIG == 1)
	struct drm_property_blob *propBlob;
	struct drm_property *propMemcStatus = NULL;
	struct property_blob_memc_status stMemcStatus;

	memset(&stMemcStatus, 0, sizeof(struct property_blob_memc_status));
#endif
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
				DRM_ERROR("[%s][%d]create enum fail!!\n",
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
				DRM_ERROR("[%s][%d]create range fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}

#if (1) //(MEMC_CONFIG == 1)
			//save memc status prop
			if (strcmp(ext_prop->prop_name,
				MTK_PLANE_PROP_MEMC_STATUS) == 0) {
				propMemcStatus = prop;
			}
#endif
		} else if (ext_prop->flag & DRM_MODE_PROP_BLOB) {
			prop = drm_property_create(drm_dev,
				ext_prop->flag,
				ext_prop->prop_name,
				0);
			if (prop == NULL) {
				DRM_ERROR("[%s, %d]: create blob prop fail!!\n",
					__func__, __LINE__);
				return -ENOMEM;
			}
		} else {
			DRM_ERROR("[%s][%d]unknown prop flag 0x%x !!\n",
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

#if (1)//(MEMC_CONFIG == 1)
	struct drm_property_blob *propBlob = NULL;
	struct property_blob_memc_status stMemcStatus;

	memset(&stMemcStatus, 0, sizeof(struct property_blob_memc_status));
#endif
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
	struct sm_ml_add_info cmd_info;
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;

	memset(reg, 0, sizeof(reg));
	memset(&paramAidTableIn, 0, sizeof(struct hwregAidTableIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramOut.reg = reg;

	paramAidTableIn.RIU = g_bUseRIU;
	paramAidTableIn.planeType = video_plane_type;
	paramAidTableIn.windowID = plane_idx;
	paramAidTableIn.access_id = access_id;

	DRM_INFO("[%s][%d][SVP] video_plane_type:%d, windowID:%d, aid:%d\n",
		__func__, __LINE__,
		video_plane_type, plane_idx, access_id);

	drv_hwreg_render_display_set_aidtable(paramAidTableIn, &paramOut);

	DRM_INFO("[%s][%d][paramOut] regCount:%d\n",
		__func__, __LINE__, paramOut.regCount);

	if (!g_bUseRIU) {
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = memindex;
		cmd_info.reg = (struct sm_reg *)reg;
		sm_ml_add_cmd(fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount
			+ paramOut.regCount;
	}

}

static void _mtk_video_display_update_dv_gd(uint16_t gd)
{
	struct msg_dv_gd_info msg_info;
	struct pqu_render_gd_param gd_param;

	DRM_INFO("[DV][GD][%s][%d] set dv gd = %d\n", __func__, __LINE__, gd);
	if (bPquEnable) {
		memset(&gd_param, 0, sizeof(struct pqu_render_gd_param));
		gd_param.gd = gd;
		pqu_render_gd((const struct pqu_render_gd_param *)&gd_param, NULL);
	} else {
		memset(&msg_info, 0, sizeof(struct msg_dv_gd_info));
		msg_info.gd = gd;
		pqu_msg_send(PQU_MSG_SEND_DOLBY_GD, (void *)&msg_info);
	}
}

static void _mtk_video_display_create_dv_workqueue(struct mtk_video_context *pctx_video)
{
	/* check input */
	if (pctx_video == NULL) {
		DRM_ERROR("[DV][GD][%s][%d] Invalid input, pctx_video = %p!!\n",
			__func__, __LINE__, pctx_video);
		return;
	}

	pctx_video->dv_gd_wq.wq = create_workqueue("DV_GD_WQ");
	if (IS_ERR(pctx_video->dv_gd_wq.wq)) {
		DRM_ERROR("[DV][GD][%s][%d] create DV GD workqueue fail, errno = %d\n",
			__func__, __LINE__,
			PTR_ERR(pctx_video->dv_gd_wq.wq));
		return;
	}
	memset(&(pctx_video->dv_gd_wq.gd_info), 0, sizeof(struct dv_gd_info));

	DRM_INFO("[DV][GD][%s][%d] create DV GD workqueue done\n", __func__, __LINE__);
}

static void _mtk_video_display_destroy_dv_workqueue(struct mtk_video_context *pctx_video)
{
	/* check input */
	if (pctx_video == NULL) {
		DRM_ERROR("[DV][GD][%s][%d]Invalid input, pctx_video = %p!!\n",
			__func__, __LINE__, pctx_video);
		return;
	}

	DRM_INFO("[DV][GD][%s][%d] destroy DV GD workqueue\n", __func__, __LINE__);
	cancel_delayed_work(&(pctx_video->dv_gd_wq.dwq));
	flush_workqueue(pctx_video->dv_gd_wq.wq);
	destroy_workqueue(pctx_video->dv_gd_wq.wq);
}

static void _mtk_video_display_handle_dv_delay_workqueue(struct work_struct *work)
{
	struct mtk_video_plane_dv_gd_workqueue *dv_gd_wq = NULL;
	struct dv_gd_info *gd_info = NULL;

	/* check input */
	if (work == NULL) {
		DRM_ERROR("[DV][GD][%s][%d]Invalid input, work = %p!!\n",
			__func__, __LINE__, work);
		return;
	}

	dv_gd_wq = container_of(work, struct mtk_video_plane_dv_gd_workqueue, dwq.work);

	gd_info = &(dv_gd_wq->gd_info);

	_mtk_video_display_update_dv_gd(gd_info->gd);

	kfree(dv_gd_wq);
}

static void _mtk_video_display_set_dv_global_dimming(
	struct mtk_video_context *pctx_video,
	struct dma_buf *meta_db)
{
	int meta_ret = 0;
	struct m_pq_dv_out_frame_info dv_out_frame_info;
	struct dv_gd_info *gd_info = NULL;
	struct mtk_video_plane_dv_gd_workqueue *priv_wq = NULL;
	int wq_size = 0;
	unsigned long delay_jiffies = 0;

	/* input */
	if (pctx_video == NULL || meta_db == NULL) {
		DRM_ERROR("[DV][GD][%s][%d]Invalid input, pctx_video = %p, meta_db = %p!!\n",
			__func__, __LINE__, pctx_video, meta_db);
		return;
	}

	memset(&dv_out_frame_info, 0, sizeof(struct m_pq_dv_out_frame_info));

	/* get dolby global dimming */
	meta_ret = mtk_render_common_read_metadata(meta_db,
		RENDER_METATAG_DV_OUTPUT_FRAME_INFO, (void *)&dv_out_frame_info);
	if (meta_ret) {
		DRM_INFO("[DV][GD][%s][%d] metadata do not has dv gd tag\n",
			__func__, __LINE__);
		return;
	}

	DRM_INFO("[DV][GD][%s][%d] meta dv gd:%d, valid=%d, delay=%d\n",
		__func__, __LINE__,
		(int)dv_out_frame_info.gd_info.gd,
		dv_out_frame_info.gd_info.valid,
		dv_out_frame_info.gd_info.delay);

	gd_info = &(pctx_video->dv_gd_wq.gd_info);

	/* check GD support */
	if (dv_out_frame_info.gd_info.valid == FALSE) {
		gd_info->valid = FALSE;
		return;
	}

	/* check GD change */
	if (gd_info->valid == TRUE && gd_info->gd == dv_out_frame_info.gd_info.gd)
		return;

	/* update gd info */
	memcpy(gd_info, &(dv_out_frame_info.gd_info), sizeof(struct dv_gd_info));

	/* GD delay */
	if (gd_info->delay) {
		wq_size = sizeof(struct mtk_video_plane_dv_gd_workqueue);
		priv_wq = kmalloc(wq_size, GFP_KERNEL);
		if (priv_wq == NULL) {
			DRM_ERROR("[DV][GD][%s][%d]alloc priv_wq fail, size=%d, ret=%d\n",
				__func__, __LINE__,
				wq_size,
				priv_wq);

			/* update gd immediately */
			_mtk_video_display_update_dv_gd(gd_info->gd);

			return;
		}
		memcpy(priv_wq, &(pctx_video->dv_gd_wq), wq_size);

		delay_jiffies = msecs_to_jiffies(gd_info->delay);

		INIT_DELAYED_WORK(&(priv_wq->dwq), _mtk_video_display_handle_dv_delay_workqueue);

		queue_delayed_work(priv_wq->wq, &(priv_wq->dwq), delay_jiffies);
	} else {
		/* update gd without delay */
		_mtk_video_display_update_dv_gd(gd_info->gd);
	}
}

static void _mtk_video_display_read_meta(
	struct mtk_video_context *pctx_video,
	unsigned int plane_idx,
	struct dma_buf *meta_db)
{
	int meta_ret = 0;
	struct meta_pq_disp_svp svpInfo;

	memset(&svpInfo, 0, sizeof(struct meta_pq_disp_svp));

	// get svp info
	meta_ret = mtk_render_common_read_metadata(meta_db,
		RENDER_METATAG_SVP_INFO, (void *)&svpInfo);
	if (meta_ret) {
		DRM_INFO("[%s][%d] metadata do not has svp info tag\n",
			__func__, __LINE__);
	} else {
		DRM_INFO("[%s][%d] meta svp info: aid: %d, pipeline id: 0x%lx\n",
			__func__, __LINE__, svpInfo.aid, svpInfo.pipelineid);
		_mtk_video_display_set_aid(pctx_video, plane_idx, (uint8_t)svpInfo.aid);
	}

}

static void _mtk_video_display_set_window_pq(
	unsigned int plane_idx,
	struct mtk_video_context *pctx_video,
	struct mtk_plane_state *state)
{
	struct mtk_video_plane_ctx *plane_ctx = NULL;
	struct hwregSetWindowPQIn paramSetWindowPQIn;

	if (pctx_video == NULL || state == NULL) {
		DRM_ERROR("[%s][%d]Invalid input, pctx_video = %p, state = %p!!\n",
			__func__, __LINE__, pctx_video, state);
		return;
	}

	memset(&paramSetWindowPQIn, 0, sizeof(paramSetWindowPQIn));

	plane_ctx = pctx_video->plane_ctx + plane_idx;

	// get window pq string
	paramSetWindowPQIn.windowID = plane_idx;
	paramSetWindowPQIn.windowPQ = plane_ctx->pq_string;

	// todo: handle window pq



	// for stub mode test
	drv_hwreg_render_display_set_window_pq(paramSetWindowPQIn);

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
	struct mtk_tv_drm_crtc *crtc = &pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO];
	struct mtk_tv_drm_connector *connector = &pctx->connector[MTK_DRM_CONNECTOR_TYPE_VIDEO];
	struct mtk_tv_drm_connector *ext_video_connector =
		&pctx->connector[MTK_DRM_CONNECTOR_TYPE_EXT_VIDEO];
	struct mtk_drm_plane *mplane_video = NULL;
	struct mtk_drm_plane *mplane_primary = NULL;
	struct drm_device *drm_dev = data;
	enum drm_plane_type drmPlaneType;
	unsigned int plane_idx;
	struct video_plane_prop *plane_props;
	struct mtk_video_plane_ctx *plane_ctx;
	unsigned int primary_plane_num = pctx->plane_num[MTK_DRM_PLANE_TYPE_PRIMARY];
	unsigned int video_plane_num =
		pctx->plane_num[MTK_DRM_PLANE_TYPE_VIDEO];
	unsigned int video_zpos_base = 0;
	unsigned int primary_zpos_base = 0;

	pctx->drm_dev = drm_dev;
	pctx_video->videoPlaneType_TypeNum = MTK_VPLANE_TYPE_MAX - 1;

	DRM_INFO("[%s][%d] plane start: [primary]:%d [video]:%d\n",
			__func__, __LINE__,
			pctx->plane_index_start[MTK_DRM_PLANE_TYPE_PRIMARY],
			pctx->plane_index_start[MTK_DRM_PLANE_TYPE_VIDEO]);

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

	plane_ctx =
		devm_kzalloc(dev, sizeof(struct mtk_video_plane_ctx) * video_plane_num, GFP_KERNEL);
	if (plane_ctx == NULL) {
		DRM_INFO("devm_kzalloc failed.\n");
		return -ENOMEM;
	}
	memset(plane_ctx, 0, sizeof(struct mtk_video_plane_ctx) * video_plane_num);
	pctx_video->plane_ctx = plane_ctx;

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
	ext_video_connector->connector_private = pctx;

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

	/* init blend out size */
	_mtk_video_display_set_blend_size(pctx, TRUE);

	/* init window alpha */
	_mtk_video_display_set_windowAlpha(pctx);

	/* init background alpha */
	_mtk_video_display_set_BackgroundAlpha(pctx);

	drv_hwreg_render_display_set_mgw_version(pctx->mgw_version);

	drv_hwreg_render_display_set_vb_version(pctx->blend_hw_version);
#if (1)//(MEMC_CONFIG == 1)
	mtk_video_display_frc_init(dev, pctx_video);
	DRM_INFO("[%s][%d] mtk video frc init success!!\n", __func__, __LINE__);
#endif

	_mtk_video_display_create_dv_workqueue(pctx_video);

	DRM_INFO("[%s][%d] mtk video init success!!\n", __func__, __LINE__);

	return 0;

ERR:
	_mtk_video_display_clean_video_context(dev, pctx_video);
	return ret;
}

int mtk_video_display_unbind(struct device *dev)
{
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_video_context *pctx_video = NULL;
	unsigned int plane_idx = 0;
	unsigned int video_plane_num = 0;

	pctx = dev_get_drvdata(dev);
	pctx_video = &pctx->video_priv;

	video_plane_num = pctx->plane_num[MTK_DRM_PLANE_TYPE_VIDEO];

	for (plane_idx = 0; plane_idx < video_plane_num; plane_idx++) {
		// remove window pq string buffer
		kfree(pctx->video_priv.plane_ctx->pq_string);
	}

	_mtk_video_display_destroy_dv_workqueue(pctx_video);

	return 0;
}

void mtk_video_display_update_plane(struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct drm_framebuffer *fb = state->base.fb;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx = (struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	unsigned int plane_inx = mplane->video_index;
	struct video_plane_prop *plane_props = (pctx_video->video_plane_properties + plane_inx);
	struct mtk_video_plane_ctx *plane_ctx = (pctx_video->plane_ctx + plane_inx);
	struct meta_pq_display_flow_info pqdd_display_flow_info;
	struct meta_pq_output_frame_info pqdd_output_frame_info;
	struct drm_mtk_tv_pqu_window_setting window_setting;
	enum drm_mtk_video_plane_type video_plane_type =
		plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE];
	enum drm_mtk_vplane_disp_mode disp_mode =
		plane_props->prop_val[EXT_VPLANE_PROP_DISP_MODE];
	uint64_t input_vfreq = 0;
	uint16_t CurrentRWdiff = 0;
	int i = 0;

	DRM_INFO("[%s][%d] update plane:%d start !\n", __func__, __LINE__, plane_inx);

	if ((!g_bUseRIU) &&
	    (pctx_video->disp_ml.memindex == ML_INVALID_MEM_INDEX ||
	     pctx_video->vgs_ml.memindex == ML_INVALID_MEM_INDEX)) {
		DRM_ERROR("[%s, %d]: ml busy, skip this frame settings!\n", __func__, __LINE__);
		return;
	}

	// get metadata content
	_mtk_video_display_read_meta(pctx_video, plane_inx, plane_ctx->meta_db);

	// set dolby global dimming
	_mtk_video_display_set_dv_global_dimming(pctx_video, plane_ctx->meta_db);

	// handle window pq
	_mtk_video_display_set_window_pq(plane_inx, pctx_video, state);

	// check video plane type
	_mtk_video_display_set_mgwdmaWinEnable(plane_inx, pctx_video, FALSE);
	_mtk_video_display_set_mgwdmaWinEnable(plane_inx, pctx_video, TRUE);


	//
	_mtk_video_display_update_displywinodw_info(plane_inx, pctx, state);

	// put this module at the first to check and update the V size
	// update scaling ratio
	_mtk_video_set_scaling_ratio(plane_inx, pctx_video, state);

	// check display window
	_mtk_video_display_set_disp_win(plane_inx, pctx_video, state);

	// check crop window
	_mtk_video_display_set_crop_win(plane_inx, pctx_video, state);

	// check fb memory format settins
	_mtk_video_display_set_fb_memory_format(plane_inx, pctx_video, fb);

	// check mem mode settings
	_mtk_video_display_update_HW_mode(plane_inx, pctx_video);

	if (disp_mode == MTK_VPLANE_DISP_MODE_PRE_SW) {
		drv_hwreg_render_display_get_rwDiff(plane_inx, &CurrentRWdiff);

		DRM_INFO("[%s, %d]disp_mode:%d CurrentRWdiff:%d\n",
			__func__, __LINE__, disp_mode, CurrentRWdiff);

		if ((CurrentRWdiff == 0) &&
			(plane_ctx->disp_mode_info.bUpdateRWdiffInNextVDone == FALSE))
			plane_ctx->disp_mode_info.bUpdateRWdiffInNextV = TRUE;
		else
			plane_ctx->disp_mode_info.bUpdateRWdiffInNextV = FALSE;
	} else {
		plane_ctx->disp_mode_info.bUpdateRWdiffInNextV = FALSE;
	}

	//check rw diff
	_mtk_video_display_set_rw_diff(plane_inx, pctx, state);

	//check freeze
	_mtk_video_display_set_freeze(plane_inx, pctx_video);

	// check fb settins
	_mtk_video_display_set_fb(plane_inx, pctx_video, fb);

	//set MEMC_OPM path
	if (plane_ctx->memc_change_on == MEMC_CHANGE_ON) {
		//set MEMC_OPM path
		mtk_video_display_set_frc_ins(plane_inx, pctx_video, plane_ctx->meta_db, fb);
		mtk_video_display_set_frc_opm_path(state, true);
	} else {
		mtk_video_display_set_frc_opm_path(state, false);
	}


	// check mute
	_mtk_video_display_set_mute(plane_inx, pctx_video);


	// check zpos
	if (video_plane_type < MTK_VPLANE_TYPE_MAX)
		pctx_video->zpos_layer[video_plane_type] = state->base.zpos;

	// blending post fill win enable
	_mtk_video_display_set_post_fill_win_enable(plane_inx, pctx_video, true);

	//update input vfreq
	if (_mtk_video_display_is_ext_prop_updated(
		plane_props, EXT_VPLANE_PROP_INPUT_VFREQ)) {
		input_vfreq = plane_props->prop_val[EXT_VPLANE_PROP_INPUT_VFREQ];
		DRM_INFO("[%s][%d] plane %d: input_vfreq update to %td\n",
			__func__, __LINE__, plane_inx, (ptrdiff_t)input_vfreq);
	}

	// appply pqmap setup
	_mtk_video_display_set_pqmap(pctx);

	//store old property value for extend property
	//this need be located in the end of function
	for (i = 0; i < EXT_VPLANE_PROP_MAX; i++)
		plane_props->old_prop_val[i] = plane_props->prop_val[i];

	// store the window setting into pqu_metadata
	memset(&pqdd_display_flow_info, 0, sizeof(struct meta_pq_display_flow_info));
	memset(&pqdd_output_frame_info, 0, sizeof(struct meta_pq_output_frame_info));
	if (mtk_render_common_read_metadata(plane_ctx->meta_db,
		RENDER_METATAG_PQDD_DISPLAY_INFO, &pqdd_display_flow_info)) {
		DRM_ERROR("[%s][%d] Read metadata RENDER_METATAG_PQDD_DISPLAY_INFO fail",
			__func__, __LINE__);
	}
	if (mtk_render_common_read_metadata(plane_ctx->meta_db,
		RENDER_METATAG_PQDD_OUTPUT_FRAME_INFO, &pqdd_output_frame_info)) {
		DRM_ERROR("[%s][%d] Read metadata RENDER_METATAG_PQDD_OUTPUT_FRAME_INFO fail",
			__func__, __LINE__);
	}
	memset(&window_setting, 0, sizeof(struct drm_mtk_tv_pqu_window_setting));
	window_setting.mute          = plane_props->prop_val[EXT_VPLANE_PROP_MUTE_SCREEN];
	window_setting.pq_id         = pqdd_display_flow_info.win_id;
	window_setting.frame_id      = pqdd_output_frame_info.pq_frame_id;
	window_setting.window_x      = state->base.crtc_x;
	window_setting.window_y      = state->base.crtc_y;
	window_setting.window_z      = state->base.zpos;
	window_setting.window_width  = state->base.crtc_w;
	window_setting.window_height = state->base.crtc_h;
	mtk_tv_drm_pqu_metadata_add_window_setting(&pctx->pqu_metadata_priv, &window_setting);

	DRM_INFO("[%s][%d] update plane:%d end !\n", __func__, __LINE__, plane_inx);
}

void mtk_video_display_disable_plane(struct mtk_plane_state *state)
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
	int i = 0;

	DRM_INFO("[%s][%d]disable plane:%d\n",
		__func__, __LINE__, plane_inx);

	if ((!g_bUseRIU) &&
	    (pctx_video->disp_ml.memindex == ML_INVALID_MEM_INDEX ||
	     pctx_video->vgs_ml.memindex == ML_INVALID_MEM_INDEX)) {
		DRM_ERROR("[%s, %d]: ml busy, skip this frame settings!\n", __func__, __LINE__);
		return;
	}

	_mtk_video_display_set_mgwdmaWinEnable(plane_inx, pctx_video, 0);

	_mtk_video_display_set_post_fill_win_enable(plane_inx, pctx_video, false);

	//store old property value for extend property
	//this need be located in the end of function
	for (i = 0; i < EXT_VPLANE_PROP_MAX; i++)
		plane_props->old_prop_val[i] = plane_props->prop_val[i];
}

void mtk_video_display_atomic_crtc_flush(struct mtk_video_context *pctx_video)
{
	struct mtk_tv_kms_context *pctx = pctx_video_to_kms(pctx_video);

	if (!pctx_video) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_video=0x%llx\n",
			__func__, __LINE__, pctx_video);
		return;
	}

	if ((!g_bUseRIU) &&
	    (pctx_video->disp_ml.memindex == ML_INVALID_MEM_INDEX ||
	     pctx_video->vgs_ml.memindex == ML_INVALID_MEM_INDEX)) {
		DRM_ERROR("[%s, %d]: ml busy, skip this frame settings!\n", __func__, __LINE__);
		return;
	}

	/* set ext enable */
	_mtk_video_display_set_extWinEnable(pctx_video);

	/* set mgw/dma enable */
	_mtk_video_display_set_mgwdmaEnable(pctx_video);

	/* set frc enable */
	_mtk_video_display_set_frcOpmMaskEnable(pctx_video);

	/* set pre insert enable */
	_mtk_video_display_set_enable_pre_insert(pctx_video);

	/* set post fill enable */
	_mtk_video_display_set_enable_post_fill(pctx_video);

	/* set zpos */
	_mtk_video_display_set_zpos(pctx_video, FALSE);

	// send the window setting to pqu from pqu_metadata
	mtk_tv_drm_pqu_metadata_fire_window_setting(&pctx->pqu_metadata_priv);

	/* set blend size*/
	_mtk_video_display_set_blend_size(pctx, FALSE);
}

void mtk_video_display_clear_plane_status(struct mtk_plane_state *state)
{
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	unsigned int plane_inx = mplane->video_index;
	struct mtk_video_plane_ctx *plane_ctx = pctx_video->plane_ctx + plane_inx;

	/* end of atomic, dma_buf_put all plane meta_db */
	if (plane_ctx->meta_db != NULL) {
		DRM_INFO("[%s][%d] put meta_db=0x%llx\n", __func__, __LINE__, plane_ctx->meta_db);
		dma_buf_put(plane_ctx->meta_db);
		plane_ctx->meta_db = NULL;

	}
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
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	struct drm_property_blob *property_blob = NULL;

	for (i = 0; i < EXT_VPLANE_PROP_MAX; i++) {
		if (property == pctx_video->drm_video_plane_prop[i]) {
			plane_props->old_prop_val[i] = plane_props->prop_val[i];
			plane_props->prop_val[i] = val;

			if (plane_props->old_prop_val[i] != plane_props->prop_val[i]) {
				plane_props->prop_update[i] = 1;

				if (IS_RANGE_PROP(property) || IS_ENUM_PROP(property)) {
					DRM_INFO("[%s][%d] plane_inx=%d, ext_prop =%d val=%d\n",
					__func__, __LINE__, plane_inx, i, val);
				}

				if (MEMC_PROP(i))
					DRM_INFO("[%s][%d] plane_inx=%d, MEMC i=0x%x val=%d\n",
					__func__, __LINE__, plane_inx, i, val);
				}
			else {
				plane_props->prop_update[i] = 0;
				//check sz
				//DRM_INFO("[%s][%d] plane_inx=%d, ext_prop =%d No updated\n",
				//	__func__, __LINE__, plane_inx, i);
			}
		ret = 0x0;
		break;
		}
	}

	switch (i) {
	case EXT_VPLANE_PROP_META_FD: {
		struct dma_buf *db = NULL;

		// dma_buf_get here, should dma_buf_put when atomic done
		db = dma_buf_get((int)val);
		if (IS_ERR_OR_NULL(db)) {
			if (IS_ERR(db)) {
				DRM_ERROR("[%s][%d] dma_buf_get fail, fd=%d, errno = %d\n",
					__func__, __LINE__, val, PTR_ERR(db));
				return PTR_ERR(db);
			}

			DRM_ERROR("[%s][%d] dma_buf_get fail, fd=%d, null ptr!\n",
				__func__, __LINE__, val);
			return -EINVAL;
		}

		DRM_INFO("[%s][%d] plane_inx=%d, meta_db=0x%llx\n",
			__func__, __LINE__, plane_inx, db);

		plane_ctx->meta_db = db;
		break;
	}
	case EXT_VPLANE_PROP_VIDEO_PLANE_TYPE:
		if (_mtk_video_display_is_ext_prop_updated(plane_props, i)) {
			_mtk_video_display_check_support_window(
				plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE],
				plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE],
				plane_inx, pctx);
		}

		break;
	case EXT_VPLANE_PROP_WINDOW_PQ:
		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob != NULL) {
			char *tmp_pq_string = 0;

			// kmalloc by blob size
			tmp_pq_string = kmalloc(property_blob->length, GFP_KERNEL);
			if (IS_ERR_OR_NULL(tmp_pq_string)) {
				DRM_ERROR("[%s][%d]alloc window %d pq buf fail, size=%d, ret=%d\n",
					__func__, __LINE__, plane_inx, property_blob->length,
					tmp_pq_string);
				return -ENOMEM;
			}

			// copy to tmp buffer
			memset(tmp_pq_string, 0, property_blob->length);
			memcpy(tmp_pq_string, property_blob->data, property_blob->length);

			// remove previous blob from kms context
			kfree(plane_ctx->pq_string);

			// save new blob to kms context
			plane_ctx->pq_string = tmp_pq_string;
		} else {
			DRM_ERROR("[%s][%d]prop WINDOW_PQ, blob id=0x%llx, blob is NULL!!\n",
				__func__, __LINE__, val);
			return -EINVAL;
		}
		break;

	case EXT_VPLANE_PROP_PQMAP_NODE_ARRAY:
		property_blob = drm_property_lookup_blob(pctx->drm_dev, val);
		if (property_blob != NULL) {
			struct drm_mtk_tv_pqmap_node_array *node_array =
				(struct drm_mtk_tv_pqmap_node_array *)property_blob->data;
			uint32_t expect_len = sizeof(struct drm_mtk_tv_pqmap_node_array) +
				(node_array->node_num * sizeof(__u16));

			DRM_INFO("[%s][%d] blob len: %u, expect len: %u, node num: %u", __func__,
				__LINE__, property_blob->length, node_array->node_num, expect_len);
			if (property_blob->length != expect_len) {
				DRM_ERROR("[%s][%d] node_array size error (blob %u != expect %u)\n",
					__func__, __LINE__, property_blob->length, expect_len);
				return -EINVAL;
			}
			ret = mtk_tv_drm_pqmap_insert(&pctx->pqmap_priv, node_array);
			if (ret) {
				DRM_ERROR("[%s][%d] mtk_tv_drm_pqmap_insert fail, ret: %d\n",
					__func__, __LINE__, ret);
				return  ret;
			}
		} else {
			DRM_ERROR("[%s][%d] blob id %u is NULL!!", __func__, __LINE__, val);
			return -EINVAL;
		}
		break;

	default:
		break;
	}

	if (MEMC_PROP(i)  &&
/*(plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE] == MTK_VPLANE_TYPE_MEMC)&&*/
		(_mtk_video_display_is_ext_prop_updated(plane_props, i))) {
		DRM_INFO("[%s][%d] plane_inx=%d, set memc_prop=0x%x\n",
			__func__, __LINE__, plane_inx, i);
		mtk_video_display_set_frc_property(plane_props, i);
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

	/*&&(plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE] == MTK_VPLANE_TYPE_MEMC)*/
	if (MEMC_PROP(i) &&
	(plane_props->prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE] == MTK_VPLANE_TYPE_MEMC)) {
		mtk_video_display_get_frc_property(plane_props, i);
		*val = plane_props->prop_val[i];
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
		if ((pctx_video->plane_num[MTK_VPLANE_TYPE_MEMC] > 1) ||
		(pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN] > 16) ||
		(pctx_video->plane_num[MTK_VPLANE_TYPE_SINGLE_WIN1] > 1) ||
		(pctx_video->plane_num[MTK_VPLANE_TYPE_SINGLE_WIN2] > 1)) {
			DRM_INFO("\033[0;32;31m[%s][%d] over HW support number !\n\033[m",
				__func__, __LINE__);
			goto error;
		}
	}
	if ((video_plane_type == MTK_VPLANE_TYPE_MULTI_WIN) &&
		(pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN] > 1) &&
		((src_width != disp_width) || (src_height != disp_height))) {
		DRM_INFO("\033[0;32;31m[%s][%d] MGW_num %d not support scaling !\n\033[m",
			__func__, __LINE__,
			pctx_video->plane_num[MTK_VPLANE_TYPE_MULTI_WIN]);
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
				plane_inx, pctx);
		}
		plane_props->prop_val[i] = plane_props->old_prop_val[i];
	}
	return -EINVAL;
}

void mtk_video_display_set_RW_diff_IRQ(
	unsigned int plane_inx,
	struct mtk_video_context *pctx_video)
{
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct hwregRwDiffIn RWdiffParamIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	uint8_t RWdiff = 1;

	memset(reg, 0, sizeof(reg));
	memset(&RWdiffParamIn, 0, sizeof(struct hwregRwDiffIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramOut.reg = reg;

	RWdiffParamIn.RIU = g_bUseRIU;
	RWdiffParamIn.windowID = plane_inx;
	RWdiffParamIn.rwDiff = RWdiff;
	DRM_INFO("[%s][%d][paramIn] RIU:%d windowID:%d, RWdiff:%d\n",
		__func__, __LINE__,
		RWdiffParamIn.RIU, RWdiffParamIn.windowID, RWdiffParamIn.rwDiff);

	drv_hwreg_render_display_set_rwDiff(
		RWdiffParamIn, &paramOut);

	if ((!g_bUseRIU) && (paramOut.regCount != 0)) {
		//3.add cmd
		cmd_info.cmd_cnt = paramOut.regCount;
		cmd_info.mem_index = pctx_video->disp_ml.irq_memindex;
		cmd_info.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(pctx_video->disp_ml.irq_fd, &cmd_info);
		pctx_video->disp_ml.irq_regcount =
			pctx_video->disp_ml.irq_regcount + paramOut.regCount;
	}
}

int mtk_tv_video_display_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (!pdev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	struct device *dev = &pdev->dev;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);

	DRM_INFO("\033[1;34m [%s][%d] \033[m\n", __func__, __LINE__);

	/* Disable frc */
	mtk_video_display_frc_suspend(dev);

	return 0;
}

int mtk_tv_video_display_resume(struct platform_device *pdev)
{
	if (!pdev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	DRM_INFO("\033[1;34m [%s][%d] \033[m\n", __func__, __LINE__);

	struct device *dev = &pdev->dev;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	int i = 0, j = 0;

	for (i = 0; i < MTK_VPLANE_TYPE_MAX; i++) {
		pctx_video->mgwdmaEnable[i] = 0;
		pctx_video->extWinEnable[i] = 0;
		pctx_video->preinsertEnable[i] = 0;
		pctx_video->postfillEnable[i] = 0;
		pctx_video->old_layer[i] = OLD_LAYER_INITIAL_VALUE;
		pctx_video->old_layer_en[i] = 0;
		for (j = 0; j < MAX_WINDOW_NUM; j++) {
			memset(&(pctx_video->old_memcfg[i][j]), 0, sizeof(struct hwregMemConfigIn));
		}
	}

	for (i = 0 ; i < MAX_WINDOW_NUM; i++) {
		struct mtk_video_plane_ctx *plane_ctx = (pctx_video->plane_ctx+i);

		plane_ctx->rwdiff = 0;
		plane_ctx->protectVal = 0;
		plane_ctx->src_h = 0;
		plane_ctx->src_w = 0;
		plane_ctx->oldbufferheight = 0;
		plane_ctx->oldbufferwidth = 0;
		plane_ctx->oldPostFillWinEn = 0;

		memset(&(plane_ctx->oldcropwindow), 0, sizeof(struct window));
		memset(&(plane_ctx->hvspoldcropwindow), 0, sizeof(struct window));
		memset(&(plane_ctx->hvspolddispwindow), 0, sizeof(struct window));
		memset(&(plane_ctx->dispolddispwindow), 0, sizeof(struct window));
		struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + i);
		plane_props->old_prop_val[EXT_VPLANE_PROP_VIDEO_PLANE_TYPE] = 0;

		if (plane_ctx->meta_db != NULL) {
			DRM_INFO("[%s][%d] put meta_db=0x%llx\n",
			__func__, __LINE__, plane_ctx->meta_db);
			dma_buf_put(plane_ctx->meta_db);
			plane_ctx->meta_db = NULL;
		}

		}

	_mtk_video_display_set_zpos(pctx_video, TRUE);

	/* init blend out size */
	_mtk_video_display_set_blend_size(pctx, TRUE);

	/* init window alpha */
	_mtk_video_display_set_windowAlpha(pctx);

	/* init background alpha */
	_mtk_video_display_set_BackgroundAlpha(pctx);

	/* init frc */
	mtk_video_display_frc_init(dev, pctx_video);

	return 0;
}

