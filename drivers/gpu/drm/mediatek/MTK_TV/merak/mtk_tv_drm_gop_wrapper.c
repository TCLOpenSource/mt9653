// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include "sti_msos.h"
#include "apiGOP.h"
#include "mtk_tv_drm_gop_wrapper.h"

#define LOG_TAG "WRP"

#define MTK_GOP_USE_ML	(TRUE)

MS_BOOL bInitGOPDriver = FALSE;
MS_U8 PlaneFbId[5] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
MS_U8 CurrentGopLayer[5] = { 0, 1, 2, 3, 4 };
MS_BOOL PlaneFbWithAFBC[5] = { FALSE, FALSE, FALSE, FALSE, FALSE };
MS_BOOL LastGOPLayer[MAX_GOP_PLANE_NUM] = {0};

static void _WrpCheckGOPRet(E_GOP_API_Result ret, char *cmd)
{
	if (ret != GOP_API_SUCCESS)
		UTP_GOP_LOGE(LOG_TAG, "%s failed\n", cmd);
}

static EN_GOP_COLOR_TYPE sWrpfmt2Utpfmt(UTP_EN_GOP_COLOR_TYPE wrp_fmt)
{
	EN_GOP_COLOR_TYPE utp_fmt;

	switch (wrp_fmt) {
	case UTP_E_GOP_COLOR_ARGB8888:
		utp_fmt = E_GOP_COLOR_ARGB8888;
		break;
	case UTP_E_GOP_COLOR_RGBA8888:
		utp_fmt = E_GOP_COLOR_RGBA8888;
		break;
	case UTP_E_GOP_COLOR_BGRA8888:
		utp_fmt = E_GOP_COLOR_BGRA8888;
		break;
	case UTP_E_GOP_COLOR_XRGB8888:
		utp_fmt = E_GOP_COLOR_ARGB8888;
		break;
	case UTP_E_GOP_COLOR_ABGR8888:
		utp_fmt = E_GOP_COLOR_ABGR8888;
		break;
	case UTP_E_GOP_COLOR_RGB565:
		utp_fmt = E_GOP_COLOR_RGB565;
		break;
	case UTP_E_GOP_COLOR_ARGB4444:
		utp_fmt = E_GOP_COLOR_ARGB4444;
		break;
	case UTP_E_GOP_COLOR_ARGB1555:
		utp_fmt = E_GOP_COLOR_ARGB1555;
		break;
	case UTP_E_GOP_COLOR_YUV422:
		utp_fmt = E_GOP_COLOR_YUV422;
		break;
	case UTP_E_GOP_COLOR_RGBA5551:
		utp_fmt = E_GOP_COLOR_RGBA5551;
		break;
	case UTP_E_GOP_COLOR_RGBA4444:
		utp_fmt = E_GOP_COLOR_RGBA4444;
		break;
	case UTP_E_GOP_COLOR_BGR565:
		utp_fmt = E_GOP_COLOR_BGR565;
		break;
	case UTP_E_GOP_COLOR_ABGR4444:
		utp_fmt = E_GOP_COLOR_ABGR4444;
		break;
	case UTP_E_GOP_COLOR_ABGR1555:
		utp_fmt = E_GOP_COLOR_ABGR1555;
		break;
	case UTP_E_GOP_COLOR_BGRA5551:
		utp_fmt = E_GOP_COLOR_BGRA5551;
		break;
	case UTP_E_GOP_COLOR_BGRA4444:
		utp_fmt = E_GOP_COLOR_BGRA4444;
		break;
	case UTP_E_GOP_COLOR_XBGR8888:
		utp_fmt = E_GOP_COLOR_ABGR8888;
		break;
	case UTP_E_GOP_COLOR_UYVY:
		utp_fmt = E_GOP_COLOR_YUV422;
		break;
	case UTP_E_GOP_COLOR_X2RGB10:
	case UTP_E_GOP_COLOR_A2RGB10:
		utp_fmt = E_GOP_COLOR_A2RGB10;
		break;
	case UTP_E_GOP_COLOR_X2BGR10:
	case UTP_E_GOP_COLOR_A2BGR10:
		utp_fmt = E_GOP_COLOR_A2BGR10;
		break;
	case UTP_E_GOP_COLOR_RGB10X2:
	case UTP_E_GOP_COLOR_RGB10A2:
		utp_fmt = E_GOP_COLOR_RGB10A2;
		break;
	case UTP_E_GOP_COLOR_BGR10X2:
	case UTP_E_GOP_COLOR_BGR10A2:
		utp_fmt = E_GOP_COLOR_BGR10A2;
		break;
	case UTP_E_GOP_COLOR_RGB888:
		utp_fmt = E_GOP_COLOR_RGB888;
		break;
	default:
		utp_fmt = E_GOP_COLOR_INVALID;
		break;
	}
	return utp_fmt;
}

static EN_GOP_STRETCH_HMODE sWrpHStretch2UtpHStretch(UTP_EN_GOP_STRETCH_HMODE wrp_mode)
{
	EN_GOP_STRETCH_HMODE mode;

	switch (wrp_mode) {
	case UTP_E_GOP_HSTRCH_6TAPE_LINEAR:
		mode = E_GOP_HSTRCH_6TAPE;
		break;
	case UTP_E_GOP_HSTRCH_DUPLICATE:
		mode = E_GOP_HSTRCH_DUPLICATE;
		break;
	case UTP_E_GOP_HSTRCH_NEW4TAP:
		mode = E_GOP_HSTRCH_NEW4TAP_95;
		break;
	default:
		mode = E_GOP_HSTRCH_NEW4TAP_95;
		break;
	}
	return mode;
}

static EN_GOP_STRETCH_VMODE sWrpVStretch2UtpVStretch(UTP_EN_GOP_STRETCH_VMODE wrp_fmt)
{
	EN_GOP_STRETCH_VMODE mode;

	switch (wrp_fmt) {
	case UTP_E_GOP_VSTRCH_2TAP:
		mode = E_GOP_VSTRCH_LINEAR;
		break;
	case UTP_E_GOP_VSTRCH_DUPLICATE:
		mode = E_GOP_VSTRCH_DUPLICATE;
		break;
	case UTP_E_GOP_VSTRCH_BILINEAR:
		mode = E_GOP_VSTRCH_LINEAR_GAIN2;
		break;
	case UTP_E_GOP_VSTRCH_4TAP:
		mode = E_GOP_VSTRCH_4TAP;
		break;
	default:
		mode = E_GOP_COLOR_INVALID;
		break;
	}
	return mode;
}

static MS_BOOL sIsPreAlphaNeed(UTP_EN_GOP_COLOR_TYPE wrp_fmt)
{
	switch (wrp_fmt) {
	case UTP_E_GOP_COLOR_XRGB8888:
	case UTP_E_GOP_COLOR_XBGR8888:
	case UTP_E_GOP_COLOR_X2RGB10:
	case UTP_E_GOP_COLOR_X2BGR10:
	case UTP_E_GOP_COLOR_RGB10X2:
	case UTP_E_GOP_COLOR_BGR10X2:
		return TRUE;
	default:
		return FALSE;
	}
}

static MS_U8 sDumpGwinNumByGop(MS_U8 u8gop)
{
	return MApi_GOP_GetGwinIdx(u8gop);
}

int utp_gop_InitByGop(UTP_GOP_INIT_INFO *utp_init_info, MTK_GOP_ML_INFO *pGopMlInfo)
{
	E_GOP_API_Result utp_ret;
	MS_U32 eIgnoreInit = 0;
	GOP_LayerConfig GopLayerConfig;
	GOP_InitInfo GopInit;
	MS_U8 u8Gwin = 0;
	MS_U8 u8GopNum = 0x0, ml_resIdx = 0;

	if (bInitGOPDriver == FALSE) {
		GOPRegisterToUtopia();
		bInitGOPDriver = TRUE;
	}

	if (utp_init_info == NULL) {
		utp_ret = MApi_GOP_Init(NULL);
		if (utp_ret != GOP_API_SUCCESS) {
			UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_Init failed\n");
			return -EFAULT;
		}
		return 0;
	}
	u8GopNum = utp_init_info->gop_idx;


	UTP_GOP_LOGI(LOG_TAG,
		     "init_info gop_idx %d, panel_width %d, panel_height %d, panel_hstart %d\n",
		     utp_init_info->gop_idx, utp_init_info->panel_width,
		     utp_init_info->panel_height, utp_init_info->panel_hstart);

	if (utp_init_info->ignore_level == E_GOP_WRAP_IGNORE_NORMAL)
		eIgnoreInit = E_GOP_IGNORE_SET_DST_OP;
	else if (utp_init_info->ignore_level == E_GOP_WRAP_IGNORE_SWRST)
		eIgnoreInit = E_GOP_IGNORE_SET_DST_OP | E_GOP_IGNORE_SWRST;
	else if (utp_init_info->ignore_level == E_GOP_WRAP_IGNORE_ALL)
		eIgnoreInit = E_GOP_IGNORE_ALL;
	else
		eIgnoreInit = 0x0;

	utp_ret = MApi_GOP_SetConfigEx(u8GopNum, E_GOP_IGNOREINIT, &eIgnoreInit);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_SetConfigEx");

	memset(&GopLayerConfig, 0, sizeof(GOP_LayerConfig));
	memset(&GopInit, 0, sizeof(GOP_InitInfo));

	GopInit.u16PanelWidth = utp_init_info->panel_width;
	GopInit.u16PanelHeight = utp_init_info->panel_height;
	GopInit.u16PanelHStr = utp_init_info->panel_hstart;
	GopInit.u32ML_Fd = pGopMlInfo->fd;

	utp_ret = MApi_GOP_GWIN_SetForceWrite(FALSE);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetForceWrite");

	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8GopNum, TRUE, FALSE, ml_resIdx);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_UpdateRegOnceEx2");

	utp_ret = MApi_GOP_InitByGOP(&GopInit, u8GopNum);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_InitByGOP");

	u8Gwin = sDumpGwinNumByGop(u8GopNum);

	if (u8Gwin == 0xFF) {
		UTP_GOP_LOGE(LOG_TAG, "[%s] invalid u8GOP = %d\n", __func__, u8GopNum);
		return -EFAULT;
	}

	if (utp_init_info->eGopDstPath == E_GOP_WARP_DST_VG_SEPARATE)
		utp_ret = MApi_GOP_GWIN_SetGOPDst(u8GopNum, E_GOP_DST_FRC);
	else
		utp_ret = MApi_GOP_GWIN_SetGOPDst(u8GopNum, E_GOP_DST_OP0);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetGOPDst");

	utp_ret = MApi_GOP_GWIN_SetBlending(u8Gwin, TRUE, 0xFF);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetBlending");

	utp_ret = MApi_GOP_GWIN_SetAlphaInverse_EX(u8GopNum, TRUE);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetAlphaInverse_EX");

	utp_ret = MApi_GOP_GWIN_SetGWinShared(u8Gwin, true);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetGWinShared");

	utp_ret = MApi_GOP_GWIN_SetGWinSharedCnt(u8Gwin, 0xFF);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetGWinSharedCnt");

	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8GopNum, FALSE, FALSE, ml_resIdx);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_UpdateRegOnceEx2");

	return 0;
}

int utp_gop_set_enable(UTP_GOP_SET_ENABLE *en_info, __u8 gop_ml_resIdx)
{
	E_GOP_API_Result utp_ret;
	MS_U8 u8gop = 0x0;
	MS_BOOL bEnable = TRUE;
	MS_U8 u8Gwin;

	u8gop = en_info->gop_idx;
	bEnable = en_info->bEn;

	u8Gwin = sDumpGwinNumByGop(u8gop);

#if MTK_GOP_USE_ML
	if (en_info->bIsCursor != TRUE)
		utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, TRUE, TRUE, gop_ml_resIdx);
	else
		utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, TRUE, FALSE, 0);
#else
	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, TRUE, FALSE, 0);
#endif
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_UpdateRegOnceEx2");

	utp_ret = MApi_GOP_GWIN_Enable(u8Gwin, bEnable);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_Enable");

#if MTK_GOP_USE_ML
	if (en_info->bIsCursor != TRUE)
		utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, FALSE, TRUE, gop_ml_resIdx);
	else
		utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, FALSE, FALSE, 0);
#else
	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, FALSE, FALSE, 0);
#endif
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_UpdateRegOnceEx2");

	return 0;
}

int utp_gop_is_enable(UTP_GOP_SET_ENABLE *en_info)
{
	MS_U8 u8gop = 0x0;
	MS_BOOL bEnable = TRUE;
	MS_U8 u8Gwin;

	u8gop = en_info->gop_idx;
	bEnable = en_info->bEn;

	u8Gwin = sDumpGwinNumByGop(u8gop);
	bEnable = MApi_GOP_GWIN_IsGWINEnabled(u8Gwin);
	en_info->bEn = bEnable;
	return 0;
}

int utp_gop_SetWindow(UTP_GOP_SET_WINDOW *p_win)
{
	E_GOP_API_Result utp_ret;
	GOP_GwinInfo WinInfo;
	GOP_StretchInfo stretchWinInfo;
	MS_U8 u8gop, u8win, gop_ml_resIdx;
	MS_U16 u16WinX, u16WinY, u16WinScaleW, u16WinScaleH;
	MS_PHY fb_addr;
	MS_U16 u16fbWidth, u16fbHeight, u16fbPitch;
	MS_BOOL bHMirror, bVMirror;
	MS_BOOL bEnableGwin;
	EN_GOP_COLOR_TYPE fmt;
	MS_BOOL bEnablePixelAlpha;
	MS_BOOL bEnablePreAlpha;
	EN_GOP_STRETCH_HMODE hStrchMode = E_GOP_HSTRCH_6TAPE;
	EN_GOP_STRETCH_VMODE vStrchMode = E_GOP_VSTRCH_LINEAR;
	GOP_LayerConfig stGopLayerConfig;
	MS_U8 u8SwapGOPidx, Layeridx;
	EN_GOP_FRAMEBUFFER_STRING fb_mod;
	MS_BOOL afbc_en, bBypassEn;
	MS_U16 u16Pic_x_start, u16Pic_y_start, u16Pic_x_end, u16Pic_y_end;
	MS_U8 u8ConstantAlphaVal = GOP_CONSTANT_ALPHA_DEF;
	MS_U64 gopAidType;

	if (p_win == NULL) {
		UTP_GOP_LOGE(LOG_TAG, "invalid param\n");
		return -EINVAL;
	}

	u8gop = p_win->gop_idx;
	u8win = sDumpGwinNumByGop(u8gop);
	u16WinX = p_win->stretchWinX;
	u16WinY = p_win->stretchWinY;
	u16WinScaleW = p_win->stretchWinDstW;
	u16WinScaleH = p_win->stretchWinDstH;
	fb_addr = p_win->fb_addr;
	u16fbWidth = p_win->fb_width;
	u16fbHeight = p_win->fb_height;
	u16fbPitch = p_win->fb_pitch;
	bHMirror = (p_win->hmirror != 0x0) ? TRUE : FALSE;
	bVMirror = (p_win->vmirror != 0x0) ? TRUE : FALSE;
	hStrchMode = sWrpHStretch2UtpHStretch(p_win->hstretch_mode);
	vStrchMode = sWrpVStretch2UtpVStretch(p_win->vstretch_mode);
	bEnableGwin = (p_win->win_enable != 0x0) ? TRUE : FALSE;
	fmt = sWrpfmt2Utpfmt(p_win->fmt_type);
	bEnablePixelAlpha = (MS_BOOL)p_win->stGopAlphaMode.bEnPixelAlpha;
	u8ConstantAlphaVal = p_win->stGopAlphaMode.u8Coef;
	bEnablePreAlpha = sIsPreAlphaNeed(p_win->fmt_type);
	fb_mod = (p_win->afbc_buffer == 0x0) ? E_GOP_FB_NULL : E_GOP_FB_AFBC_V1P2_ARGB8888;
	afbc_en = (p_win->afbc_buffer == 0x0) ? FALSE : TRUE;
	u16Pic_x_start = p_win->pic_x_start;
	u16Pic_y_start = p_win->pic_y_start;
	u16Pic_x_end = p_win->pic_x_end;
	u16Pic_y_end = p_win->pic_y_end;
	bBypassEn = (p_win->bypass_path == 0x0) ? FALSE : TRUE;
	gop_ml_resIdx = p_win->gop_ml_res_idx;
	gopAidType = p_win->gop_aid_type;

	UTP_GOP_LOGD(LOG_TAG, "u8gop %d, u8win %d\n", u8gop, u8win);
	UTP_GOP_LOGD(LOG_TAG, "Win Info {%d, %d, %d, %d}\n", u16WinX, u16WinY, u16WinScaleW,
		     u16WinScaleH);
	UTP_GOP_LOGD(LOG_TAG, "FB Info ADDR:0x%llx {W:%d, H:%d, P:%d}, fmt %d\n", fb_addr,
		     u16fbWidth, u16fbHeight, u16fbPitch, fmt);
	UTP_GOP_LOGD(LOG_TAG, "bHMirror %d, bVMirror %d\n", bHMirror, bVMirror);
	UTP_GOP_LOGD(LOG_TAG, "bSync %d\n", bSync);
	UTP_GOP_LOGD(LOG_TAG, "bEnableGwin %d\n", bEnableGwin);
	UTP_GOP_LOGD(LOG_TAG, "bEnablePixelAlpha %d\n", bEnablePixelAlpha);
	UTP_GOP_LOGD(LOG_TAG, "bEnablePreAlpha %d\n", bEnablePreAlpha);
	UTP_GOP_LOGD(LOG_TAG, "hStrchMode %d vStrchMode %d\n", hStrchMode, vStrchMode);

	if (fmt == E_GOP_COLOR_INVALID) {
		UTP_GOP_LOGE(LOG_TAG, "invalid FB format, fmt %d\n", p_win->fmt_type);
		return -EFAULT;
	}

#if MTK_GOP_USE_ML
	if (p_win->eGopType != UTP_E_GOP_CURSOR)
		utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, TRUE, TRUE, gop_ml_resIdx);
	else
		utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, TRUE, FALSE, gop_ml_resIdx);
#else
	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, TRUE, FALSE, gop_ml_resIdx);
#endif
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_UpdateRegOnceEx2");

	if (u8gop == 0) {
		utp_ret = MApi_GOP_SetGOPBypassPath(u8gop, bBypassEn);
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_SetGOPBypassPath");
	}

	if (p_win->GopChangeStatus & BIT(GOP_HMIRROR_STATE_CHANGE)) {
		utp_ret = MApi_GOP_GWIN_SetHMirror_EX(u8gop, bHMirror);
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetHMirror_EX");
	}

	if (p_win->GopChangeStatus & BIT(GOP_VMIRROR_STATE_CHANGE)) {
		utp_ret = MApi_GOP_GWIN_SetVMirror_EX(u8gop, bVMirror);
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetVMirror_EX");
	}

	if (p_win->eGopType != UTP_E_GOP_CURSOR) {
		if (p_win->GopChangeStatus & BIT(GOP_HSTRETCH_STATE_CHANGE)) {
			utp_ret = MApi_GOP_GWIN_Set_HStretchMode_EX(u8gop, hStrchMode);
			_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_Set_HStretchMode_EX");
		}

		if (p_win->GopChangeStatus & BIT(GOP_VSTRETCH_STATE_CHANGE)) {
			utp_ret = MApi_GOP_GWIN_Set_VStretchMode_EX(u8gop, vStrchMode);
			_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_Set_VStretchMode_EX");
		}
	}

	if (p_win->GopChangeStatus & BIT(GOP_AID_TYPE_STATE_CHANGE)) {
		utp_ret = MApi_GOP_SetAidType(u8gop, gopAidType);
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_SetAidType");
	}

	if ((PlaneFbId[u8gop] == GOP_FB_INVALID) || (PlaneFbWithAFBC[u8gop] != afbc_en)) {
		if ((PlaneFbWithAFBC[u8gop] != afbc_en) && (PlaneFbId[u8gop] != GOP_FB_INVALID))
			MApi_GOP_GWIN_Delete32FB(PlaneFbId[u8gop]);

		PlaneFbId[u8gop] = MApi_GOP_GWIN_GetFree32FBID();
		MApi_GOP_GWIN_Create32FBbyStaticAddr2(PlaneFbId[u8gop], 0, 0, u16fbWidth,
						      u16fbHeight, fmt, fb_addr,
						      fb_mod);
		PlaneFbWithAFBC[u8gop] = afbc_en;
	}

	memset(&WinInfo, 0x0, sizeof(WinInfo));
	WinInfo.u16DispHPixelEnd = u16Pic_x_end;
	WinInfo.u16DispVPixelEnd = u16Pic_y_end;
	WinInfo.u32DRAMRBlkStart = fb_addr;
	WinInfo.u16RBlkHPixSize = u16fbWidth;
	WinInfo.u16RBlkVPixSize = u16fbHeight;
	WinInfo.u16RBlkHRblkSize = u16fbPitch;
	WinInfo.clrType = fmt;
	WinInfo.u16DispHPixelStart = u16Pic_x_start;
	WinInfo.u16DispVPixelStart = u16Pic_y_start;

	stretchWinInfo.x = u16WinX;
	stretchWinInfo.y = u16WinY;
	stretchWinInfo.width = (u16Pic_x_end - u16Pic_x_start);//src_W
	stretchWinInfo.height = (u16Pic_y_end - u16Pic_y_start);//src_h

	utp_ret = MApi_GOP_GWIN_SetResolution_32FB(u8win, PlaneFbId[u8gop],
						   &WinInfo, &stretchWinInfo, E_GOP_HV_STRETCH,
						   u16WinScaleW, u16WinScaleH);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetResolution_32FB");

	if (p_win->GopChangeStatus & BIT(GOP_ALPHA_MODE_STATE_CHANGE)) {
		utp_ret = MApi_GOP_GWIN_SetBlending(u8win, bEnablePixelAlpha, u8ConstantAlphaVal);
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetBlending");
	}

	u8SwapGOPidx = CurrentGopLayer[p_win->u8zpos];
	for (Layeridx = 0; Layeridx < MApi_GOP_GWIN_GetMaxGOPNum(); Layeridx++) {
		if (CurrentGopLayer[Layeridx] == u8gop) {
			CurrentGopLayer[Layeridx] = u8SwapGOPidx;
			CurrentGopLayer[p_win->u8zpos] = u8gop;
			break;
		}
	}

	utp_ret = MApi_GOP_GWIN_SetPreAlphaMode(u8gop, bEnablePreAlpha);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetPreAlphaMode");

	utp_ret = MApi_GOP_GWIN_Enable(u8win, bEnableGwin);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_Enable");

#if MTK_GOP_USE_ML
	if (p_win->eGopType != UTP_E_GOP_CURSOR)
		utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, FALSE, TRUE, gop_ml_resIdx);
	else
		utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, FALSE, FALSE, gop_ml_resIdx);
#else
	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, FALSE, FALSE, gop_ml_resIdx);
#endif
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_UpdateRegOnceEx2");

	return 0;
}

int utp_gop_intr_ctrl(UTP_GOP_SET_ENABLE *pSetEnable)
{
	E_GOP_API_Result utp_ret;
	MS_U8 u8GOP;
	MS_BOOL bEnable;

	if (pSetEnable == NULL) {
		UTP_GOP_LOGE(LOG_TAG, "Invalid param\n");
		return -EINVAL;
	}

	u8GOP = pSetEnable->gop_idx;
	bEnable = pSetEnable->bEn;

	utp_ret = MApi_GOP_GWIN_INTERRUPT(u8GOP, bEnable);
	_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_INTERRUPT");

	return 0;
}

int utp_gop_query_chip_prop(UTP_GOP_QUERY_FEATURE *pFeatureEnum)
{
	E_GOP_API_Result utp_ret;

	if (pFeatureEnum->feature == UTP_E_GOP_CAP_AFBC_SUPPORT) {
		GOP_CAP_AFBC_INFO info;

		utp_ret = MApi_GOP_GetChipCaps(E_GOP_CAP_AFBC_SUPPORT, &info, sizeof(info));
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_GetChipCaps");

		pFeatureEnum->value =
		    (info.GOP_AFBC_Support & (1 << pFeatureEnum->gop_idx)) ? 1 : 0;
	}
	return 0;
}

int utp_gop_power_state(__u32 State)
{
	E_GOP_API_Result utp_ret = GOP_API_SUCCESS;

	switch (State) {
	case UTP_E_GOP_POWER_SUSPEND:
		utp_ret = MApi_GOP_SetPowerState(E_POWER_SUSPEND);
		break;
	case UTP_E_GOP_POWER_RESUME:
		utp_ret = MApi_GOP_SetPowerState(E_POWER_RESUME);
		break;
	default:
		break;
	}

	_WrpCheckGOPRet(utp_ret, "MApi_GOP_SetPowerState");

	return 0;
}
EXPORT_SYMBOL(utp_gop_power_state);


int gop_wrapper_set_testpattern(MTK_GOP_TESTPATTERN *pGopTestPatternInfo)
{
	ST_GOP_TESTPATTERN stGOPTestPatternInfo;

	memset(&stGOPTestPatternInfo, 0, sizeof(ST_GOP_TESTPATTERN));
	stGOPTestPatternInfo.PatEnable = pGopTestPatternInfo->PatEnable;
	stGOPTestPatternInfo.HwAutoMode = pGopTestPatternInfo->HwAutoMode;
	stGOPTestPatternInfo.DisWidth = pGopTestPatternInfo->DisWidth;
	stGOPTestPatternInfo.DisHeight = pGopTestPatternInfo->DisHeight;
	stGOPTestPatternInfo.ColorbarW = pGopTestPatternInfo->ColorbarW;
	stGOPTestPatternInfo.ColorbarH = pGopTestPatternInfo->ColorbarH;
	stGOPTestPatternInfo.EnColorbarMove = pGopTestPatternInfo->EnColorbarMove;
	stGOPTestPatternInfo.MoveDir = pGopTestPatternInfo->MoveDir;
	stGOPTestPatternInfo.MoveSpeed = pGopTestPatternInfo->MoveSpeed;
	if (MApi_GOP_SetTestPattern(&stGOPTestPatternInfo) == GOP_API_SUCCESS)
		return 0;
	else
		return -1;
}

int gop_wrapper_get_CRC(__u32 *CRCvalue)
{
	if (MApi_GOP_GetCRC(CRCvalue) == GOP_API_SUCCESS)
		return 0;
	else
		return -1;
}

int gop_wrapper_print_status(__u8 GOPNum)
{
	if (MApi_GOP_InfoPrint(GOPNum) == GOP_API_SUCCESS)
		return 0;
	else
		return -1;
}

int gop_wrapper_set_Tgen(MTK_GOP_TGEN_INFO *pGOPTgenInfo)
{
	ST_GOP_TGEN_INFO stGOPTgenInfo;

	memset(&stGOPTgenInfo, 0, sizeof(ST_GOP_TGEN_INFO));
	stGOPTgenInfo.Tgen_hs_st = pGOPTgenInfo->Tgen_hs_st;
	stGOPTgenInfo.Tgen_hs_end = pGOPTgenInfo->Tgen_hs_end;
	stGOPTgenInfo.Tgen_hfde_st = pGOPTgenInfo->Tgen_hfde_st;
	stGOPTgenInfo.Tgen_hfde_end = pGOPTgenInfo->Tgen_hfde_end;
	stGOPTgenInfo.Tgen_htt = pGOPTgenInfo->Tgen_htt;
	stGOPTgenInfo.Tgen_vs_st = pGOPTgenInfo->Tgen_vs_st;
	stGOPTgenInfo.Tgen_vs_end = pGOPTgenInfo->Tgen_vs_end;
	stGOPTgenInfo.Tgen_vfde_st = pGOPTgenInfo->Tgen_vfde_st;
	stGOPTgenInfo.Tgen_vfde_end = pGOPTgenInfo->Tgen_vfde_end;
	stGOPTgenInfo.Tgen_vtt = pGOPTgenInfo->Tgen_vtt;
	switch (pGOPTgenInfo->GOPOutMode) {
	case E_GOP_WARP_OUT_A8RGB10:
		stGOPTgenInfo.GOPOutMode = E_GOP_OUT_A8RGB10;
	break;
	case E_GOP_WARP_OUT_ARGB8888_DITHER:
		stGOPTgenInfo.GOPOutMode = E_GOP_OUT_ARGB8888_DITHER;
	break;
	case E_GOP_WARP_OUT_ARGB8888_ROUND:
		stGOPTgenInfo.GOPOutMode = E_GOP_OUT_ARGB8888_ROUND;
	break;
	default:
		stGOPTgenInfo.GOPOutMode = E_GOP_OUT_A8RGB10;
	break;
	}

	if (MApi_GOP_SetTGen(&stGOPTgenInfo) == GOP_API_SUCCESS)
		return 0;
	else
		return -1;
}

int gop_wrapper_set_TgenFreeRun(__u8 Tgen_VGSyncEn, __u8 u8MlResIdx)
{
	int ret = 0;

#if MTK_GOP_USE_ML
	MApi_GOP_GWIN_UpdateRegOnceEx2(0, TRUE, TRUE, u8MlResIdx);
#endif

	if (MApi_GOP_SetTgenVGSync((MS_BOOL)Tgen_VGSyncEn))
		ret = 0;
	else
		ret = -1;

#if MTK_GOP_USE_ML
	MApi_GOP_GWIN_UpdateRegOnceEx2(0, FALSE, TRUE, u8MlResIdx);
#endif

	return ret;
}

int gop_wrapper_set_MixerOutMode(bool Mixer1_OutPreAlpha, bool Mixer2_OutPreAlpha)
{
	if (MApi_GOP_SetMixerMode(Mixer1_OutPreAlpha, Mixer2_OutPreAlpha))
		return 0;
	else
		return -1;
}

int gop_wrapper_set_BypassMode(__u8 u8GOP, bool bEnable)
{
	if (MApi_GOP_SetGOPBypassPath(u8GOP, bEnable))
		return 0;
	else
		return -1;
}
EXPORT_SYMBOL(gop_wrapper_set_BypassMode);

int gop_wrapper_set_PQCtxInfo(__u8 u8GOP, MTK_GOP_PQ_INFO *pGopPQCtx)
{
	ST_GOP_PQCTX_INFO GopPQCtx;

	GopPQCtx.pCtx = pGopPQCtx->pCtx;
	if (MApi_GOP_SetPQCtx(u8GOP, &GopPQCtx))
		return 0;
	else
		return -1;
}

int gop_wrapper_set_Adlnfo(__u8 u8GOP, MTK_GOP_ADL_INFO *pGopAdlInfo)
{
	ST_GOP_ADL_INFO stGopAdlInfo;

	stGopAdlInfo.fd = pGopAdlInfo->fd;
	stGopAdlInfo.adl_bufsize = pGopAdlInfo->adl_bufsize;
	stGopAdlInfo.u64_user_adl_buf = pGopAdlInfo->u64_adl_buf;

	if (MApi_GOP_setAdlnfo(u8GOP, &stGopAdlInfo))
		return 0;
	else
		return -1;
}

int gop_wrapper_set_PqCfdMlInfo(__u8 u8GOP, MTK_GOP_PQ_CFD_ML_INFO *pGopPqCfdMl)
{
	ST_GOP_PQ_CFD_ML_INFO stGopMlInfo;

	stGopMlInfo.u32_cfd_ml_bufsize = pGopPqCfdMl->u32_cfd_ml_bufsize;
	stGopMlInfo.u64_user_cfd_ml_buf = pGopPqCfdMl->u64_cfd_ml_buf;
	stGopMlInfo.u32_pq_ml_bufsize = pGopPqCfdMl->u32_pq_ml_bufsize;
	stGopMlInfo.u64_user_pq_ml_buf = pGopPqCfdMl->u64_pq_ml_buf;

	if (MApi_GOP_setPqMlnfo(u8GOP, &stGopMlInfo))
		return 0;
	else
		return -1;
}

int gop_wrapper_ml_fire(__u8 u8MlResIdx)
{
	if (MApi_GOP_Ml_Fire(u8MlResIdx))
		return 0;
	else
		return -1;
}

int gop_wrapper_ml_get_mem_info(__u8 u8MlResIdx)
{
	if (MApi_GOP_Ml_Get_Mem_Info(u8MlResIdx))
		return 0;
	else
		return -1;
}

int gop_wrapper_setLayer(MS_U32 GopIdx, MS_U32 MlResIdx)
{
	MS_BOOL bLayerChange = FALSE;
	MS_U8 LayerIdx, LayerCnt, i;
	E_GOP_API_Result utp_ret = GOP_API_SUCCESS;
	GOP_LayerConfig stGopLayerConfig;

	memset(&stGopLayerConfig, 0, sizeof(GOP_LayerConfig));

	LayerCnt = MApi_GOP_GWIN_GetMaxGOPNum();
	for (i = 0; i < LayerCnt; i++) {
		if (LastGOPLayer[i] != CurrentGopLayer[i]) {
			LastGOPLayer[i] = CurrentGopLayer[i];
			bLayerChange = true;
		}
	}

	if (bLayerChange) {
		stGopLayerConfig.u32LayerCounts = LayerCnt;
		for (LayerIdx = 0; LayerIdx < LayerCnt; LayerIdx++) {
			stGopLayerConfig.stGopLayer[LayerIdx].u32GopIndex =
				CurrentGopLayer[LayerIdx];
			stGopLayerConfig.stGopLayer[LayerIdx].u32LayerIndex = LayerIdx;
		}

		utp_ret |= MApi_GOP_GWIN_UpdateRegOnceEx2((MS_U8)GopIdx, TRUE, TRUE, MlResIdx);
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_UpdateRegOnceEx2");

		utp_ret |= MApi_GOP_GWIN_SetLayer(&stGopLayerConfig, sizeof(CurrentGopLayer));
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_SetLayer");

		utp_ret |= MApi_GOP_GWIN_UpdateRegOnceEx2((MS_U8)GopIdx, FALSE, TRUE, MlResIdx);
		_WrpCheckGOPRet(utp_ret, "MApi_GOP_GWIN_UpdateRegOnceEx2");

		if (utp_ret != GOP_API_SUCCESS)
			return -1;
		else
			return 0;

	} else {
		return 0;
	}
}

int gop_wrapper_get_ROI(MTK_GOP_GETROI *pRoiInfo)
{
	ST_GOP_GETROI stGOPRoi;

	memset(&stGOPRoi, 0, sizeof(ST_GOP_GETROI));

	stGOPRoi.bRetTotal = pRoiInfo->bRetTotal;
	stGOPRoi.gopIdx = pRoiInfo->gopIdx;
	stGOPRoi.MainVRate = pRoiInfo->MainVRate;
	stGOPRoi.Threshold = pRoiInfo->Threshold;

	if (MApi_GOP_GetRoi(&stGOPRoi) != GOP_API_SUCCESS)
		return -1;

	pRoiInfo->RetCount = stGOPRoi.RetCount;

	return 0;
}

int gop_wrapper_set_vg_order(EN_GOP_WRAP_VIDEO_ORDER_TYPE eVGOrder, __u8 u8MlResIdx)
{
	EN_GOP_VG_ORDER eVGlayer = E_GOP_VIDEO_OSDB0_OSDB1;
	int ret = 0;

#if MTK_GOP_USE_ML
	MApi_GOP_GWIN_UpdateRegOnceEx2(0, TRUE, TRUE, u8MlResIdx);
#endif

	switch (eVGOrder) {
	case E_GOP_WRAP_VIDEO_OSDB0_OSDB1:
		eVGlayer = E_GOP_VIDEO_OSDB0_OSDB1;
		break;
	case E_GOP_WRAP_OSDB0_VIDEO_OSDB1:
		eVGlayer = E_GOP_OSDB0_VIDEO_OSDB1;
		break;
	case E_GOP_WRAP_OSDB0_OSDB1_VIDEO:
		eVGlayer = E_GOP_OSDB0_OSDB1_VIDEO;
		break;
	default:
		eVGlayer = E_GOP_VIDEO_OSDB0_OSDB1;
		break;
	}

	if (MApi_GOP_SetVGOrder(eVGlayer) != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_SetVGOrder failed\n");
		ret = -1;
	}

#if MTK_GOP_USE_ML
	MApi_GOP_GWIN_UpdateRegOnceEx2(0, FALSE, TRUE, u8MlResIdx);
#endif

	return ret;
}

