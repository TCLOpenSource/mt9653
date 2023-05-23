// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include "sti_msos.h"
#include "apiGOP.h"
#include "mtk_tv_drm_gop_wrapper.h"

#define LOG_TAG "WRP"

MS_BOOL bInitGOPDriver = FALSE;
MS_U8 PlaneFbId[5] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
MS_U8 CurrentGopLayer[5] = { 0, 1, 2, 3, 4 };
MS_BOOL PlaneFbWithAFBC[5] = { FALSE, FALSE, FALSE, FALSE, FALSE };

static EN_GOP_CFD_CFIO sWrpCS2UtpCFIO(UTP_EN_GOP_CS wrp_cs)
{
	EN_GOP_CFD_CFIO utp_gop_cfd_cfio;

	switch (wrp_cs) {
	case UTP_E_GOP_COLOR_YCBCR_BT601:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_YUV_BT601_625;
		break;
	case UTP_E_GOP_COLOR_YCBCR_BT709:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_YUV_BT709;
		break;
	case UTP_E_GOP_COLOR_YCBCR_BT2020:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_YUV_BT2020_NCL;
		break;
	case UTP_E_GOP_COLOR_RGB_BT601:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_RGB_BT601_625;
		break;
	case UTP_E_GOP_COLOR_RGB_BT709:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_RGB_BT709;
		break;
	case UTP_E_GOP_COLOR_RGB_BT2020:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_RGB_BT2020;
		break;
	case UTP_E_GOP_COLOR_SRGB:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_SRGB;
		break;
	case UTP_E_GOP_COLOR_RGB_DCIP3_D65:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_DCIP3_D65;
		break;
	default:
		utp_gop_cfd_cfio = E_GOP_CFD_CFIO_RGB_BT709;
		break;
	}

	return utp_gop_cfd_cfio;
}

static EN_GOP_CFD_MC_FORMAT sWrpFmt2UtpCFmt(UTP_EN_GOP_DATA_FMT wrp_cfmt)
{
	EN_GOP_CFD_MC_FORMAT utp_cfmt = E_GOP_CFD_MC_FORMAT_RGB;

	switch (wrp_cfmt) {
	case UTP_E_GOP_COLOR_FORMAT_RGB:
		utp_cfmt = E_GOP_CFD_MC_FORMAT_RGB;
		break;
	case UTP_E_GOP_COLOR_FORMAT_YUV422:
		utp_cfmt = E_GOP_CFD_MC_FORMAT_YUV422;
		break;
	case UTP_E_GOP_COLOR_FORMAT_YUV444:
		utp_cfmt = E_GOP_CFD_MC_FORMAT_YUV444;
		break;
	case UTP_E_GOP_COLOR_FORMAT_YUV420:
		utp_cfmt = E_GOP_CFD_MC_FORMAT_YUV420;
		break;
	default:
		UTP_GOP_LOGW(LOG_TAG, "no corresponding color format to utp\n");
		break;
	}

	return utp_cfmt;
}

static EN_GOP_CFD_CFIO_RANGE sWrpRange2UtpCSRange(UTP_EN_GOP_CR wrp_cr)
{
	EN_GOP_CFD_CFIO_RANGE utp_gop_cfd_cr;

	switch (wrp_cr) {
	case UTP_E_GOP_COLOR_YCBCR_LIMITED_RANGE:
	case UTP_E_GOP_COLOR_RGB_LIMITED_RANGE:
		utp_gop_cfd_cr = E_GOP_CFD_CFIO_RANGE_LIMIT;
		break;
	case UTP_E_GOP_COLOR_YCBCR_FULL_RANGE:
	case UTP_E_GOP_COLOR_RGB_FULL_RANGE:
		utp_gop_cfd_cr = E_GOP_CFD_CFIO_RANGE_FULL;
		break;
	default:
		utp_gop_cfd_cr = E_GOP_CFD_CFIO_RANGE_FULL;
		break;
	}

	return utp_gop_cfd_cr;
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

static MS_BOOL sIsPixelAlphaNeed(UTP_EN_GOP_COLOR_TYPE wrp_fmt)
{
	return TRUE;
}

static MS_BOOL sIsPreAlphaNeed(UTP_EN_GOP_COLOR_TYPE wrp_fmt)
{
	if (UTP_E_GOP_COLOR_XRGB8888 == wrp_fmt || UTP_E_GOP_COLOR_XBGR8888 == wrp_fmt)
		return TRUE;
	else
		return FALSE;
}

static MS_U8 sDumpGwinNumByGop(MS_U8 u8gop)
{
	return MApi_GOP_GetGwinIdx(u8gop);
}

int utp_gop_InitByGop(UTP_GOP_INIT_INFO *utp_init_info)
{
	E_GOP_API_Result utp_ret;
	MS_U32 eIgnoreInit = 0;
	GOP_LayerConfig GopLayerConfig;
	GOP_InitInfo GopInit;
	MS_U8 u8Gwin = 0;
	MS_U8 u8GopNum = 0x0;

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

	if (utp_init_info->ignore_level == UTP_E_GOP_IGNORE_NORMAL)
		eIgnoreInit = E_GOP_IGNORE_MUX | E_GOP_IGNORE_PALETTE | E_GOP_IGNORE_SET_DST_OP;
	else if (utp_init_info->ignore_level == UTP_E_GOP_IGNORE_ALL)
		eIgnoreInit = E_GOP_IGNORE_ALL;
	else
		eIgnoreInit = 0x0;

	utp_ret = MApi_GOP_SetConfigEx(u8GopNum, E_GOP_IGNOREINIT, &eIgnoreInit);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_SetConfigEx failed\n");
		return -EFAULT;
	}

	memset(&GopLayerConfig, 0, sizeof(GOP_LayerConfig));
	memset(&GopInit, 0, sizeof(GOP_InitInfo));

	GopInit.u16PanelWidth = utp_init_info->panel_width;
	GopInit.u16PanelHeight = utp_init_info->panel_height;
	GopInit.u16PanelHStr = utp_init_info->panel_hstart;
	GopInit.u32GOPRegdmaAdr = 0x0;
	GopInit.u32GOPRegdmaLen = 0x0;
	GopInit.bEnableVsyncIntFlip = FALSE;

	utp_ret = MApi_GOP_GWIN_SetForceWrite(TRUE);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetForceWrite failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_InitByGOP(&GopInit, u8GopNum);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_InitByGOP failed\n");
		return -EFAULT;
	}

	u8Gwin = sDumpGwinNumByGop(u8GopNum);

	if (u8Gwin == 0xFF) {
		UTP_GOP_LOGE(LOG_TAG, "[%s] invalid u8GOP = %d\n", __func__, u8GopNum);
		MApi_GOP_GWIN_SetForceWrite(FALSE);
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetGOPDst(u8GopNum, E_GOP_DST_OP0);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetGOPDst failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetBlending(u8Gwin, TRUE, 0xFF);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetBlending failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetAlphaInverse_EX(u8GopNum, TRUE);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetAlphaInverse_EX failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetGWinShared(u8Gwin, true);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetGWinShared failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetGWinSharedCnt(u8Gwin, 0xFF);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetGWinSharedCnt failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetForceWrite(FALSE);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetForceWrite failed\n");
		return -EFAULT;
	}

	return 0;
}

int utp_gop_set_enable(UTP_GOP_SET_ENABLE *en_info)
{
	E_GOP_API_Result utp_ret;
	MS_U8 u8gop = 0x0;
	MS_BOOL bEnable = TRUE;
	MS_U8 u8Gwin;

	u8gop = en_info->gop_idx;
	bEnable = en_info->bEn;

	u8Gwin = sDumpGwinNumByGop(u8gop);

	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, TRUE, FALSE);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_UpdateRegOnceEx2 failed\n");
		return -EFAULT;
	}
	utp_ret = MApi_GOP_GWIN_Enable(u8Gwin, bEnable);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_Enable failed\n");
		return -EFAULT;
	}
	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, FALSE, FALSE);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_UpdateRegOnceEx2 failed\n");
		return -EFAULT;
	}
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
	MS_U8 u8gop, u8win;
	MS_U16 u16WinX, u16WinY, u16WinScaleW, u16WinScaleH;
	MS_PHY fb_addr;
	MS_U16 u16fbWidth, u16fbHeight, u16fbPitch;
	MS_BOOL bHMirror, bVMirror;
	MS_BOOL bSync;
	MS_BOOL bEnableGwin;
	EN_GOP_COLOR_TYPE fmt;
	MS_BOOL bEnablePixelAlpha;
	MS_BOOL bEnablePreAlpha;
	EN_GOP_STRETCH_HMODE hStrchMode = E_GOP_HSTRCH_6TAPE;
	EN_GOP_STRETCH_VMODE vStrchMode = E_GOP_VSTRCH_LINEAR;
	ST_GOP_CROP_WINDOW stCropWin;
	GOP_LayerConfig stGopLayerConfig;
	MS_U8 u8SwapGOPidx, Layeridx;
	EN_GOP_FRAMEBUFFER_STRING fb_mod;
	MS_BOOL afbc_en;
	MS_U16 u16Pic_x_start, u16Pic_y_start, u16Pic_x_end, u16Pic_y_end;

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
	bSync = (p_win->wait_vsync != 0x0) ? TRUE : FALSE;
	bEnableGwin = (p_win->win_enable != 0x0) ? TRUE : FALSE;
	fmt = sWrpfmt2Utpfmt(p_win->fmt_type);
	bEnablePixelAlpha = sIsPixelAlphaNeed(p_win->fmt_type);
	bEnablePreAlpha = sIsPreAlphaNeed(p_win->fmt_type);
	stCropWin.bCropEn = p_win->cropEn;
	stCropWin.u16CropWinXStart = p_win->cropWinXStart;
	stCropWin.u16CropWinYStart = p_win->cropWinYStart;
	stCropWin.u16CropWinXEnd = p_win->cropWinXEnd;
	stCropWin.u16CropWinYEnd = p_win->cropWinYEnd;
	fb_mod = (p_win->afbc_buffer == 0x0) ? E_GOP_FB_NULL : E_GOP_FB_AFBC_V1P2_ARGB8888;
	afbc_en = (p_win->afbc_buffer == 0x0) ? FALSE : TRUE;
	u16Pic_x_start = p_win->pic_x_start;
	u16Pic_y_start = p_win->pic_y_start;
	u16Pic_x_end = p_win->pic_x_end;
	u16Pic_y_end = p_win->pic_y_end;

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

	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, TRUE, FALSE);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_UpdateRegOnceEx2 failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_SetCropWindow(u8gop, &stCropWin);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_SetCropWindow failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetHMirror_EX(u8gop, bHMirror);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetHMirror_EX failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetVMirror_EX(u8gop, bVMirror);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetVMirror_EX failed\n");
		return -EFAULT;
	}

	if (p_win->eGopType != UTP_E_GOP_CURSOR) {
		utp_ret = MApi_GOP_GWIN_Set_HStretchMode_EX(u8gop, hStrchMode);
		if (utp_ret != GOP_API_SUCCESS) {
			UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_Set_HStretchMode_EX failed\n");
			return -EFAULT;
		}

		utp_ret = MApi_GOP_GWIN_Set_VStretchMode_EX(u8gop, vStrchMode);
		if (utp_ret != GOP_API_SUCCESS) {
			UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_Set_VStretchMode_EX failed\n");
			return -EFAULT;
		}
	}
	if ((PlaneFbId[u8gop] == 0xFF) || (PlaneFbWithAFBC[u8gop] != afbc_en)) {
		if (PlaneFbWithAFBC[u8gop] != afbc_en)
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
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetResolution_32FB failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetBlending(u8win, bEnablePixelAlpha, 0xFF);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetBlending failed\n");
		return -EFAULT;
	}

	u8SwapGOPidx = CurrentGopLayer[p_win->u8zpos];
	for (Layeridx = 0; Layeridx < MApi_GOP_GWIN_GetMaxGOPNum(); Layeridx++) {
		if (CurrentGopLayer[Layeridx] == u8gop) {
			CurrentGopLayer[Layeridx] = u8SwapGOPidx;
			CurrentGopLayer[p_win->u8zpos] = u8gop;
			break;
		}
	}

	stGopLayerConfig.u32LayerCounts = MApi_GOP_GWIN_GetMaxGOPNum();
	for (Layeridx = 0; Layeridx < MApi_GOP_GWIN_GetMaxGOPNum(); Layeridx++) {
		stGopLayerConfig.stGopLayer[Layeridx].u32GopIndex = CurrentGopLayer[Layeridx];
		stGopLayerConfig.stGopLayer[Layeridx].u32LayerIndex = Layeridx;
	}
	utp_ret = MApi_GOP_GWIN_SetLayer(&stGopLayerConfig, sizeof(CurrentGopLayer));
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetLayer failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_SetPreAlphaMode(u8gop, bEnablePreAlpha);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_SetPreAlphaMode failed\n");
		return -EFAULT;
	}

	if (p_win->GopChangeStatus & BIT(GOP_CSC_STATE_CHANGE)) {
		if (utp_gop_SetCscTuning(u8gop, p_win->pGOPCSCParam))
			return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_Enable(u8win, bEnableGwin);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_Enable failed\n");
		return -EFAULT;
	}

	utp_ret = MApi_GOP_GWIN_UpdateRegOnceEx2(u8gop, FALSE, bSync);
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_UpdateRegOnceEx2 failed\n");
		return -EFAULT;
	}

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
	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GWIN_INTERRUPT failed\n");
		return -EFAULT;
	}

	return 0;
}

int utp_gop_query_chip_prop(UTP_GOP_QUERY_FEATURE *pFeatureEnum)
{
	E_GOP_API_Result utp_ret;

	if (pFeatureEnum->feature == UTP_E_GOP_CAP_AFBC_SUPPORT) {
		GOP_CAP_AFBC_INFO info;

		utp_ret = MApi_GOP_GetChipCaps(E_GOP_CAP_AFBC_SUPPORT, &info, sizeof(info));
		if (utp_ret != GOP_API_SUCCESS) {
			UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_GetChipCaps failed\n");
			return -EFAULT;
		}
		pFeatureEnum->value =
		    (info.GOP_AFBC_Support & (1 << pFeatureEnum->gop_idx)) ? 1 : 0;
	}
	return 0;
}

int utp_gop_power_state(UTP_EN_GOP_POWER_STATE enPowerState)
{
	E_GOP_API_Result utp_ret = GOP_API_SUCCESS;

	switch (enPowerState) {
	case UTP_E_GOP_POWER_SUSPEND:
		utp_ret = MApi_GOP_SetPowerState(E_POWER_SUSPEND);
		break;
	case UTP_E_GOP_POWER_RESUME:
		utp_ret = MApi_GOP_SetPowerState(E_POWER_RESUME);
		break;
	default:
		break;
	}

	if (utp_ret != GOP_API_SUCCESS) {
		UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_SetPowerState failed\n");
		return -EFAULT;
	}

	return 0;
}

int utp_gop_SetCscTuning(__u8 u8GOPNum, UTP_GOP_CSC_PARAM *pCSCParamWrp)
{
	E_GOP_API_Result utp_ret;
	ST_GOP_CSC_PARAM stGOPCscParam;

	memset(&stGOPCscParam, 0, sizeof(ST_GOP_CSC_PARAM));
	stGOPCscParam.u32Version = ST_GOP_CSC_PARAM_VERSION;
	stGOPCscParam.u32Length = sizeof(ST_GOP_CSC_PARAM);
	stGOPCscParam.bCscEnable = TRUE;
	stGOPCscParam.u16Hue = pCSCParamWrp->u16Hue;
	stGOPCscParam.u16Saturation = pCSCParamWrp->u16Saturation;
	stGOPCscParam.u16Contrast = pCSCParamWrp->u16Contrast;
	stGOPCscParam.u16Brightness = pCSCParamWrp->u16Brightness;
	stGOPCscParam.u16RGBGGain[0] = pCSCParamWrp->u16RGBGain[0];
	stGOPCscParam.u16RGBGGain[1] = pCSCParamWrp->u16RGBGain[1];
	stGOPCscParam.u16RGBGGain[2] = pCSCParamWrp->u16RGBGain[2];
	stGOPCscParam.enInputFormat = sWrpCS2UtpCFIO(pCSCParamWrp->eInputCS);
	stGOPCscParam.enInputDataFormat = sWrpFmt2UtpCFmt(pCSCParamWrp->eInputCFMT);
	stGOPCscParam.enInputRange = sWrpRange2UtpCSRange(pCSCParamWrp->eInputCR);
	stGOPCscParam.enOutputFormat = sWrpCS2UtpCFIO(pCSCParamWrp->eOutputCS);
	stGOPCscParam.enOutputDataFormat = sWrpFmt2UtpCFmt(pCSCParamWrp->eOutputCFMT);
	stGOPCscParam.enOutputRange = sWrpRange2UtpCSRange(pCSCParamWrp->eOutputCR);

	utp_ret = MApi_GOP_SetGOPCscTuning((MS_U32) u8GOPNum, &stGOPCscParam);
	if (utp_ret != GOP_API_SUCCESS) {
		if (utp_ret == GOP_API_FUN_NOT_SUPPORTED) {
			UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_SetGOPCscTuning not support\n");
		} else {
			UTP_GOP_LOGE(LOG_TAG, "MApi_GOP_SetGOPCscTuning failed\n");
			return -EFAULT;
		}
	}

	return 0;
}
