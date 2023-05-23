// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#if !defined(MSOS_TYPE_LINUX_KERNEL)
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#else
#include <linux/string.h>
#include <linux/slab.h>
#endif

#include "apiGOP.h"
#include "apiGOP_priv.h"
#include "drvGOP.h"
#include "mtk_tv_drm_log.h"

//-------------------------------------------------------------------------------------------------
//  Local Compiler Options
//-------------------------------------------------------------------------------------------------
#ifndef UTOPIAXP_REMOVE_WRAPPER

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
#ifndef GOP_UTOPIA2K
#define GOP_UTOPIA2K
#endif
#endif

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define MTK_DRM_MODEL MTK_DRM_MODEL_GOP

#ifndef UNUSED
#define UNUSED(var) (void)(var)
#endif

//=============================================================

#ifdef MSOS_TYPE_LINUX
#include <assert.h>
#include <unistd.h>
#endif

#ifdef MSOS_TYPE_LINUX
static pthread_mutex_t GOP_XC_Instance_Mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
#endif //#ifndef UTOPIAXP_REMOVE_WRAPPER

void *pInstantGOP_XC;

#ifdef STI_PLATFORM_BRING_UP
#define CheckXCOpen()   FALSE
#endif

#ifndef UTOPIAXP_REMOVE_WRAPPER

#if defined(MSOS_TYPE_LINUX_KERNEL)
#include <linux/slab.h>
#define free kfree
#define malloc(size) kmalloc((size), GFP_KERNEL)
#endif

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
static MS_BOOL bInit = FALSE;


//-------------------------------------------------------------------------------------------------
//  Utopia 2.0
//-------------------------------------------------------------------------------------------------

void *pInstantGOP;
void *pAttributeGOP;

//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
static E_GOP_API_Result _CheckGOPRet(MS_U32 Ret, MS_U32 Cmd)
{
	if (Ret != UTOPIA_STATUS_SUCCESS) {
		pr_err("Cmd %d fail\n", Cmd);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

void GOP_ENTRY(void)
{
	if (!bInit)
		pr_err("Need GOP Driver Init First\n", __func__, __LINE__);
}

void CheckGOPInstanceOpen(void)
{
	if (pInstantGOP == NULL) {
		if (UtopiaOpen(MODULE_GOP | GOPDRIVER_BASE, &pInstantGOP, 0, pAttributeGOP) !=
			UTOPIA_STATUS_SUCCESS)
			DRM_ERROR("[GOP][%s, %d]: Open GOP fail\n", __func__, __LINE__);
	}
}

/******************************************************************************/
/// Enable/Disable gop alpha inverse
/// @param bEnable \b IN: TRUE or FALSE
/// @return GOP_API_SUCCESS - Success
/******************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_SetAlphaInverse_EX(MS_U8 u8GOP, MS_BOOL bEnable)
{
	MS_BOOL bAlphaInv;
	GOP_SET_PROPERTY_PARAM ioctl_info;
	MS_U32 Drvret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	bAlphaInv = bEnable;

	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_ALPHAINVERSE;
	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pSetting = (void *)&bAlphaInv;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	Drvret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info);
	eRet = _CheckGOPRet(Drvret, MAPI_CMD_GOP_SET_PROPERTY);

	return eRet;
}

/********************************************************************************/
///Set GOP force write mode for update register.
//When enable force write mode, all update gop registers action will directly
/// take effect (do not wait next v-sync to update gop register!).
/// @param bEnable \b IN: TRUE/FALSE
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_SetForceWrite(MS_BOOL bEnable)
{
	MS_BOOL bEna;
	GOP_SET_PROPERTY_PARAM ioctl_info;
	MS_U32 Drvret;
	E_GOP_API_Result eRet;

	bEna = bEnable;

	CheckGOPInstanceOpen();
	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_FORCE_WRITE;
	ioctl_info.gop_idx = 0x0;
	ioctl_info.pSetting = (void *)&bEna;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	Drvret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info);
	eRet = _CheckGOPRet(Drvret, MAPI_CMD_GOP_SET_PROPERTY);

	return eRet;
}

/********************************************************************************/
/// Set GOP H stretch mode
/// @param HStrchMode \b IN:
///   - # E_GOP_HSTRCH_6TAPE
///   - # E_GOP_HSTRCH_DUPLICATE
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_Set_HStretchMode_EX(MS_U8 u8GOP, EN_GOP_STRETCH_HMODE HStrchMode)
{
	GOP_STRETCH_INFO stretch_info;
	GOP_STRETCH_SET_PARAM ioctl_info;
	MS_U32 u32Ret = 0;
	E_GOP_API_Result eRet = GOP_API_SUCCESS;

	GOP_ENTRY();

	memset(&stretch_info, 0x0, sizeof(GOP_STRETCH_INFO));
	stretch_info.enHMode = HStrchMode;
	memset(&ioctl_info, 0x0, sizeof(GOP_STRETCH_SET_PARAM));

	ioctl_info.gop_idx = u8GOP;
	ioctl_info.enStrtchType = E_GOP_STRETCH_HSTRETCH_MODE;
	ioctl_info.pStretch = (MS_U32 *) &stretch_info;
	ioctl_info.u32Size = sizeof(GOP_STRETCH_INFO);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_STRETCH, (void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GWIN_SET_STRETCH);

	return eRet;
}

/********************************************************************************/
/// Set GOP V stretch mode
/// @param VStrchMode \b IN:
///   - # E_GOP_VSTRCH_LINEAR
///   - # E_GOP_VSTRCH_DUPLICATE
///   - # E_GOP_VSTRCH_NEAREST
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_Set_VStretchMode_EX(MS_U8 u8GOP, EN_GOP_STRETCH_VMODE VStrchMode)
{
	GOP_STRETCH_INFO stretch_info;
	GOP_STRETCH_SET_PARAM ioctl_info;
	MS_U32 u32Ret = 0;
	E_GOP_API_Result eRet = GOP_API_SUCCESS;

	GOP_ENTRY();

	memset(&stretch_info, 0x0, sizeof(GOP_STRETCH_INFO));

	stretch_info.enVMode = VStrchMode;
	memset(&ioctl_info, 0x0, sizeof(GOP_STRETCH_SET_PARAM));

	ioctl_info.gop_idx = u8GOP;
	ioctl_info.enStrtchType = E_GOP_STRETCH_VSTRETCH_MODE;
	ioctl_info.pStretch = (MS_U32 *) &stretch_info;
	ioctl_info.u32Size = sizeof(GOP_STRETCH_INFO);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_STRETCH, (void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GWIN_SET_STRETCH);

	return eRet;
}

/// Extend MApi_GOP_GWIN_UpdateRegOnce, add parameter bSync.
/// Set gop update register method by only once.
/// Example: if you want to update GOP function A, B, C in the same V sync,
/// please write down your code like below
/// MApi_GOP_GWIN_UpdateRegOnceEx(TRUE, TRUE);
/// GOP_FUN_A;
/// GOP_FUN_B;
/// GOP_FUN_C;
/// MApi_GOP_GWIN_UpdateRegOnceEx(FALSE, TRUE);
/// @param bWriteRegOnce    \b IN: TRUE/FALSE
/// @param bSync            \b IN: TRUE/FALSE
/// @return GOP_API_SUCCESS - Success
E_GOP_API_Result MApi_GOP_GWIN_UpdateRegOnceEx2(MS_U8 u8GOP, MS_BOOL bWriteRegOnce, MS_BOOL bUseMl,
	MS_U8 MlResIdx)
{
	GOP_ENTRY();

	return GOP_TriggerRegWriteIn(pInstantGOP, u8GOP, bWriteRegOnce, bUseMl, MlResIdx);
}

E_GOP_API_Result MApi_GOP_Ml_Fire(MS_U8 u8MlResIdx)
{
	return GOP_ml_fire(pInstantGOP, u8MlResIdx);
}

E_GOP_API_Result MApi_GOP_Ml_Get_Mem_Info(MS_U8 u8MlResIdx)
{
	return GOP_ml_get_mem_info(pInstantGOP, u8MlResIdx);
}

/// Set GWin Attribute to Shared. If shared GWin, More than one process could
/// access this GWin.
/// @param winId \b IN: GWIN ID for shared
/// @param bIsShared \b IN: shared or not
/// @return GOP_API_SUCCESS - Success
E_GOP_API_Result MApi_GOP_GWIN_SetGWinShared(MS_U8 winId, MS_BOOL bIsShared)
{
	MS_BOOL bShared;
	GOP_GWIN_PROPERTY_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	bShared = bIsShared;

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_SHARE;
	ioctl_info.GwinId = winId;
	ioctl_info.pSet = (void *)&bShared;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_PROPERTY,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GWIN_SET_PROPERTY);

	return eRet;
}

/// Set Reference cnt of shared GWin.
/// @param winId \b IN: GWIN ID for shared
/// @param u16SharedCnt \b IN: shared reference cnt.
/// @return GOP_API_SUCCESS - Success
E_GOP_API_Result MApi_GOP_GWIN_SetGWinSharedCnt(MS_U8 winId, MS_U16 u16SharedCnt)
{
	MS_U16 u16Cnt;
	GOP_GWIN_PROPERTY_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	u16Cnt = u16SharedCnt;

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_SHARE_CNT;
	ioctl_info.GwinId = winId;
	ioctl_info.pSet = (void *)&u16Cnt;
	ioctl_info.u32Size = sizeof(MS_U16);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_PROPERTY,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GWIN_SET_PROPERTY);

	return eRet;
}

/// Enable GWIN for display
/// @param winId \b IN: GWIN id
/// @param bEnable \b IN:
///   - # TRUE Show GWIN
///   - # FALSE Hide GWIN
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
E_GOP_API_Result MApi_GOP_GWIN_Enable(MS_U8 winId, MS_BOOL bEnable)
{
	GOP_GWIN_PROPERTY_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_ENABLE;
	ioctl_info.GwinId = winId;
	ioctl_info.pSet = (void *)&bEnable;
	ioctl_info.u32Size = sizeof(MS_BOOL);
	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_PROPERTY,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GWIN_SET_PROPERTY);

	return eRet;

}

/********************************************************************************/
/// Check if GWIN is enabled
/********************************************************************************/
/// Check if all some GWIN is currently enabled
/// @param  winId \b IN: gwin id
/// @return  - the according GWin is enabled or not
MS_BOOL MApi_GOP_GWIN_IsGWINEnabled(MS_U8 winId)
{
	MS_BOOL bEn;
	GOP_GWIN_PROPERTY_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_ENABLE;
	ioctl_info.GwinId = winId;
	ioctl_info.pSet = (void *)&bEn;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_GET_PROPERTY,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GWIN_GET_PROPERTY);

	return bEn;


}

/*******************************************************************************/
//Set which OSD Layer select which GOP
//@param pGopLayer \b IN:information about GOP and corresponding Layer
//   #u32LayerCounts: the total GOP/Layer counts to set
//   #stGopLayer[i].u32GopIndex :the GOP which need to change Layer
//   #stGopLayer[i].u32LayerIndex :the GOP corresponding Layer
//@return GOP_API_SUCCESS - Success
/*******************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_SetLayer(GOP_LayerConfig *pGopLayer, MS_U32 u32SizeOfLayerInfo)
{
	GOP_SETLayer stLayerSetting;
	MS_U32 i;
	GOP_SETLAYER_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	memset(&stLayerSetting, 0x0, sizeof(GOP_SETLayer));

	stLayerSetting.u32LayerCount = pGopLayer->u32LayerCounts;

	for (i = 0; i < stLayerSetting.u32LayerCount; i++) {
		stLayerSetting.u32Gop[i] = pGopLayer->stGopLayer[i].u32GopIndex;
		stLayerSetting.u32Layer[i] = pGopLayer->stGopLayer[i].u32LayerIndex;
	}

	memset(&ioctl_info, 0x0, sizeof(GOP_SETLAYER_PARAM));

	ioctl_info.pLayerInfo = (MS_U32 *) &stLayerSetting;
	ioctl_info.u32Size = sizeof(GOP_SETLayer);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_LAYER,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_SET_LAYER);

	return eRet;
}

/******************************************************************************/
/// Configure the destination of a specific GOP
/// @param u8GOP \b IN : Number of GOP
/// @param dsttype \b IN : GOP destination
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_ENUM_NOT_SUPPORTED - GOP destination not support
/******************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_SetGOPDst(MS_U8 u8GOP, EN_GOP_DST_TYPE dsttype)
{
	GOP_SETDST_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_SETDST_PARAM));

	ioctl_info.en_dst = dsttype;
	ioctl_info.gop_idx = u8GOP;

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_DST,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_SET_DST);

	return eRet;
}

/******************************************************************************/
/// Get free frame buffer id
/// @return frame buffer id. If return oxFF, it represents no free frame buffer id for use.
/******************************************************************************/
MS_U32 MApi_GOP_GWIN_GetFree32FBID(void)
{
	MS_U32 u32FBId = 0xFFFFFFFF;
	GOP_FB_PROPERTY_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_FB_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_FB_OBTAIN;
	ioctl_info.FBId = 0x0;
	ioctl_info.pSet = (MS_U32 *) &u32FBId;
	ioctl_info.u32Size = sizeof(MS_U32);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_FB_GET_PROPERTY,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_FB_GET_PROPERTY);

	return u32FBId;
}

/// Destroy the frame buffer and return the memory to mmgr
/// @param fbId  \b IN frame buffer id
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_CRT_GWIN_NOAVAIL - destroy frame buffer fail
MS_U8 MApi_GOP_GWIN_Destroy32FB(MS_U32 u32fbId)
{
	GOP_DELETE_BUFFER_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_DELETE_BUFFER_PARAM));

	ioctl_info.pBufId = (MS_U32 *) &u32fbId;
	ioctl_info.u32Size = sizeof(MS_U32);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_FB_DESTROY,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_FB_DESTROY);

	return GWIN_OK;
}

E_GOP_API_Result MApi_GOP_GWIN_Delete32FB(MS_U32 u32fbId)
{
	GOP_ENTRY();
	DRM_INFO("[GOP][%s][%d] FBId = %td\n", __func__, __LINE__, (ptrdiff_t)u32fbId);
	if ((MApi_GOP_GWIN_Destroy32FB(u32fbId)) != GWIN_OK)
		return GOP_API_FAIL;

	return GOP_API_SUCCESS;

}

/********************************************************************************/
/// Set GWIN alpha blending
/// @param u8win \b IN GWIN id
/// @param bEnable \b IN
///   - # TRUE enable pixel alpha
///   - # FALSE disable pixel alpha
/// @param u8coef \b IN alpha blending coefficient (0-7)
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_SetBlending(MS_U8 u8win, MS_BOOL bEnable, MS_U8 u8coef)
{
	GOP_GWIN_BLENDING blendInfo;
	GOP_GWIN_PROPERTY_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	blendInfo.Coef = u8coef;
	blendInfo.bEn = bEnable;

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_BLENDING;
	ioctl_info.GwinId = u8win;
	ioctl_info.pSet = (MS_U32 *) &blendInfo;
	ioctl_info.u32Size = sizeof(GOP_GWIN_BLENDING);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_PROPERTY,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GWIN_SET_PROPERTY);

	return eRet;

}

//-------------------------------------------------------------------------------------------------
/// Get maximum support gop number
/// @return maximum support gop number
//-------------------------------------------------------------------------------------------------
MS_U8 MApi_GOP_GWIN_GetMaxGOPNum(void)
{
	MS_U8 u8MaxGop = 0;
	GOP_GET_STATUS_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	memset(&ioctl_info, 0x0, sizeof(GOP_GET_STATUS_PARAM));

	CheckGOPInstanceOpen();

	ioctl_info.type = 0;
	ioctl_info.en_status = E_GOP_STATUS_GOP_MAXNUM;
	ioctl_info.pStatus = (void *)&u8MaxGop;
	ioctl_info.u32Size = sizeof(MS_U8);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_STATUS,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GET_STATUS);

	return u8MaxGop;

}

MS_U8 MApi_GOP_GetGwinIdx(MS_U8 u8GOPIdx)
{
	GOP_GWIN_NUM gwin_num;
	GOP_GET_STATUS_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	memset(&ioctl_info, 0x0, sizeof(GOP_GET_STATUS_PARAM));

	CheckGOPInstanceOpen();

	gwin_num.gop_idx = u8GOPIdx;
	ioctl_info.type = 0;
	ioctl_info.en_status = E_GOP_STATUS_GWINIDX;
	ioctl_info.pStatus = (MS_U32 *) &gwin_num;
	ioctl_info.u32Size = sizeof(GOP_GWIN_NUM);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_STATUS,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GET_STATUS);

	return gwin_num.gwin_num;
}

E_GOP_API_Result MApi_GOP_GWIN_SetPreAlphaMode(MS_U8 u8GOP, MS_BOOL bEnble)
{
	MS_BOOL bPreAlpha;
	GOP_SET_PROPERTY_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();

	bPreAlpha = bEnble;

	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_PREALPHAMODE;
	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pSetting = (void *)&bPreAlpha;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_SET_PROPERTY);

	return eRet;

}

/********************************************************************************/
/// Set GOP H-Mirror
/// @param bEnable \b IN
///   - # TRUE enable
///   - # FALSE disable
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_SetHMirror_EX(MS_U8 u8GOP, MS_BOOL bEnable)
{
	GOP_SETMIRROR_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_SETMIRROR_PARAM));

	if (bEnable == TRUE)
		ioctl_info.dir = E_GOP_MIRROR_H_ONLY;
	else
		ioctl_info.dir = E_GOP_MIRROR_H_NONE;

	ioctl_info.gop_idx = (MS_U32) u8GOP;	//get current gop for driver capability
	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_MIRROR,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_SET_MIRROR);

	return eRet;
}

/********************************************************************************/
/// Set GOP V-Mirror
/// @param bEnable \b IN
///   - # TRUE enable
///   - # FALSE disable
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_SetVMirror_EX(MS_U8 u8GOP, MS_BOOL bEnable)
{
	GOP_SETMIRROR_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_SETMIRROR_PARAM));

	if (bEnable == TRUE)
		ioctl_info.dir = E_GOP_MIRROR_V_ONLY;
	else
		ioctl_info.dir = E_GOP_MIRROR_V_NONE;

	ioctl_info.gop_idx = (MS_U32) u8GOP;	//get current gop for driver capability
	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_MIRROR,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_SET_MIRROR);

	return eRet;

}

//-------------------------------------------------------------------------------------------------
/// Initial individual GOP driver
/// @param pGopInit \b IN:gop driver init info
/// @param u8GOP \b IN: only init by which gop
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
//-------------------------------------------------------------------------------------------------
E_GOP_API_Result MApi_GOP_InitByGOP(GOP_InitInfo *pGopInit, MS_U8 u8GOP)
{
	GOP_INIT_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	CheckGOPInstanceOpen();
	memset(&ioctl_info, 0x0, sizeof(GOP_INIT_PARAM));

	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pInfo = (MS_U32 *) pGopInit;
	ioctl_info.u32Size = sizeof(GOP_InitInfo);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_INIT,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_INIT);

	bInit = TRUE;
	return eRet;
}

//-------------------------------------------------------------------------------------------------
/// Initial all GOP driver (include gop0, gop1 ext..)
/// @param pGopInit \b IN:gop driver init info
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
//-------------------------------------------------------------------------------------------------
E_GOP_API_Result MApi_GOP_Init(GOP_InitInfo *pGopInit)
{
	MS_U32 i = 0;
	GOP_INIT_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	CheckGOPInstanceOpen();
	//init GOP0
	memset(&ioctl_info, 0x0, sizeof(GOP_INIT_PARAM));

	ioctl_info.gop_idx = 0;
	ioctl_info.pInfo = (MS_U32 *) pGopInit;
	ioctl_info.u32Size = sizeof(GOP_InitInfo);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_INIT,
		(void *)&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_INIT);

	//init others
	for (i = 1; i < MApi_GOP_GWIN_GetMaxGOPNum(); i++) {
		ioctl_info.gop_idx = i;
		u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_INIT,
			(void *)&ioctl_info);
		eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_INIT);
	}
	bInit = TRUE;

	return eRet;
}

MS_U8 MApi_GOP_GWIN_Create32FBbyStaticAddr2(MS_U32 u32fbId, MS_U16 dispX, MS_U16 dispY,
					    MS_U16 width, MS_U16 height, MS_U16 fbFmt,
					    MS_PHY phyFbAddr, EN_GOP_FRAMEBUFFER_STRING FBString)
{
	GOP_BUFFER_INFO BuffInfo;
	GOP_CREATE_BUFFER_PARAM ioctl_info;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();
	memset(&BuffInfo, 0x0, sizeof(GOP_BUFFER_INFO));

	BuffInfo.addr = phyFbAddr;
	BuffInfo.fbFmt = fbFmt;
	BuffInfo.disp_rect.x = dispX;
	BuffInfo.disp_rect.y = dispY;
	BuffInfo.disp_rect.w = width;
	BuffInfo.disp_rect.h = height;

	BuffInfo.width = width;
	BuffInfo.height = height;
	BuffInfo.FBString = FBString;

	memset(&ioctl_info, 0x0, sizeof(GOP_CREATE_BUFFER_PARAM));

	ioctl_info.fb_type = GOP_CREATE_BUFFER_BYADDR;
	ioctl_info.pBufInfo = (MS_U32 *) &BuffInfo;
	ioctl_info.u32Size = sizeof(GOP_BUFFER_INFO);
	ioctl_info.fbid = u32fbId;

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_FB_CREATE,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_FB_CREATE);

	return eRet;

}

/********************************************************************************/
/// Set config by GOP; For dynamic usage.
/// @param u8GOP \b IN:  GOP number
/// @param pstInfo \b IN misc info
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
/********************************************************************************/
E_GOP_API_Result MApi_GOP_SetConfigEx(MS_U8 u8Gop, EN_GOP_CONFIG_TYPE enType, void *plist)
{
	E_GOP_API_CMD_TYPE enApiCmdType = MAPI_CMD_GOP_SET_CONFIG;
	GOP_SETCONFIG_PARAM stIoctlConfigInfo;
	GOP_SET_PROPERTY_PARAM stIoctlPropInfo;
	void *pInfo = NULL;
	MS_U32 u32CopiedLength = 0;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	memset(&stIoctlConfigInfo, 0x0, sizeof(GOP_SETCONFIG_PARAM));
	memset(&stIoctlPropInfo, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	CheckGOPInstanceOpen();

	switch (enType) {
	case E_GOP_IGNOREINIT:
		{
			stIoctlConfigInfo.cfg_type = enType;
			stIoctlConfigInfo.pCfg = plist;
			stIoctlConfigInfo.u32Size = sizeof(EN_GOP_IGNOREINIT);
			pInfo = (void *)&stIoctlConfigInfo;
			enApiCmdType = MAPI_CMD_GOP_SET_CONFIG;
			break;
		}
	default:
		return GOP_API_INVALID_PARAMETERS;
	}

	u32Ret = UtopiaIoctl(pInstantGOP, enApiCmdType, pInfo);
	eRet = _CheckGOPRet(u32Ret, enApiCmdType);

	return eRet;
}

/// API for set GWIN resolution in one function
/// @param u8GwinId \b IN: GWin ID
/// @param u8FbId \b IN: Frame Buffer ID
/// @param pGwinInfo \b IN: pointer to GOP_GwinInfo structure
/// @param pStretchInfo \b IN: pointer to GOP_StretchInfo
/// @param direction \b IN: to decide which direction to stretch
/// @param u16DstWidth \b IN: set scaled width if H direction is specified
/// @param u16DstHeight \b IN: set scaled height if V direction is specified
/// @return GOP_API_SUCCESS - Success
E_GOP_API_Result MApi_GOP_GWIN_SetResolution_32FB(MS_U8 u8GwinId, MS_U32 u32FbId,
						  GOP_GwinInfo *pGwinInfo,
						  GOP_StretchInfo *pStretchInfo,
						  EN_GOP_STRETCH_DIRECTION direction,
						  MS_U16 u16DstWidth, MS_U16 u16DstHeight)
{
	GOP_GWIN_DISPLAY_PARAM ioctl_info;
	GOP_GWINDISPLAY_INFO DispInfo;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	GOP_ENTRY();
	memset(&DispInfo, 0x0, sizeof(GOP_GWINDISPLAY_INFO));

	DispInfo.dir = direction;
	DispInfo.gwin = u8GwinId;
	DispInfo.fbid = u32FbId;
	DispInfo.dst_size.w = u16DstWidth;
	DispInfo.dst_size.h = u16DstHeight;

	memcpy(&DispInfo.gwin_info, pGwinInfo, sizeof(GOP_GwinInfo));
	memcpy(&DispInfo.stretch_info, pStretchInfo, sizeof(GOP_StretchInfo));

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_DISPLAY_PARAM));

	ioctl_info.GwinId = u8GwinId;
	ioctl_info.u32Size = sizeof(GOP_GWINDISPLAY_INFO);
	ioctl_info.pDisplayInfo = (MS_U32 *) &DispInfo;

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SETDISPLAY,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GWIN_SETDISPLAY);

	return eRet;
}

/// API for Query GOP Capability
/// @param eCapType \b IN: Capability type
/// @param pRet     \b OUT: return value
/// @param ret_size \b IN: input structure size
/// @return GOP_API_SUCCESS - Success
E_GOP_API_Result MApi_GOP_GetChipCaps(EN_GOP_CAPS eCapType, void *pRet, MS_U32 ret_size)
{
	GOP_GETCAPS_PARAM gopcaps;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	CheckGOPInstanceOpen();

	memset(&gopcaps, 0x0, sizeof(GOP_GETCAPS_PARAM));

	gopcaps.caps = eCapType;
	gopcaps.pInfo = pRet;
	gopcaps.u32Size = ret_size;

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_CHIPCAPS,
		&gopcaps);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_GET_CHIPCAPS);

	return eRet;
}

/******************************************************************************/
/// set GOP power state
/// @param enPowerState\b IN power status
/******************************************************************************/
E_GOP_API_Result MApi_GOP_SetPowerState(EN_POWER_MODE enPowerState)
{
	GOP_POWERSTATE_PARAM PowerState;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	CheckGOPInstanceOpen();
	GOP_ENTRY();
	memset(&PowerState, 0x0, sizeof(GOP_POWERSTATE_PARAM));

	PowerState.pInfo = &enPowerState;
	PowerState.u32Size = sizeof(GOP_POWERSTATE_PARAM);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_POWERSTATE,
		&PowerState);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_POWERSTATE);

	return eRet;
}

E_GOP_API_Result MApi_GOP_GWIN_INTERRUPT(MS_U8 u8GOP, MS_BOOL bEnable)
{
	GOP_INTERRUPT_PARAM ioctl_info;
	MS_U32 bEn = bEnable;
	MS_U32 u32Ret;
	E_GOP_API_Result eRet;

	memset(&ioctl_info, 0x0, sizeof(GOP_INTERRUPT_PARAM));

	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pSetting = &bEn;
	ioctl_info.u32Size = sizeof(GOP_INTERRUPT_PARAM);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_INTERRUPT,
		&ioctl_info);
	eRet = _CheckGOPRet(u32Ret, MAPI_CMD_GOP_INTERRUPT);

	return eRet;
}

E_GOP_API_Result MApi_GOP_SetTestPattern(ST_GOP_TESTPATTERN *pstGOPTestPatternInfo)
{
	return Ioctl_GOP_SetTestPattern(pstGOPTestPatternInfo);
}

E_GOP_API_Result MApi_GOP_GetCRC(MS_U32 *CRCvalue)
{
	return Ioctl_GOP_GetCRC(CRCvalue);
}

E_GOP_API_Result MApi_GOP_InfoPrint(MS_U8 GOPNum)
{
	return Ioctl_GOP_PrintInfo(GOPNum);
}

E_GOP_API_Result MApi_GOP_SetGOPBypassPath(MS_U8 u8GOP, MS_BOOL bEnable)
{
	return Ioctl_GOP_SetGOPBypassPath(pInstantGOP, u8GOP, bEnable);
}

E_GOP_API_Result MApi_GOP_SetTGen(ST_GOP_TGEN_INFO *pGOPTgenInfo)
{
	return Ioctl_GOP_SetTGen(pInstantGOP, pGOPTgenInfo);
}

E_GOP_API_Result MApi_GOP_SetTgenVGSync(MS_BOOL bEnable)
{
	return GOP_SetTGenVGSync(bEnable);
}

E_GOP_API_Result MApi_GOP_SetMixerMode(MS_BOOL Mixer1_OutPreAlpha, MS_BOOL Mixer2_OutPreAlpha)
{
	return GOP_SetMixerOutMode(pInstantGOP, Mixer1_OutPreAlpha, Mixer2_OutPreAlpha);
}

E_GOP_API_Result MApi_GOP_SetPQCtx(MS_U8 u8GOP, ST_GOP_PQCTX_INFO *pGopPQCtx)
{
	return GOP_SetPQCtx(pInstantGOP, u8GOP, pGopPQCtx);
}

E_GOP_API_Result MApi_GOP_setAdlnfo(MS_U8 u8GOP, ST_GOP_ADL_INFO *pGopAdlInfo)
{
	return GOP_SetAdlnfo(pInstantGOP, u8GOP, pGopAdlInfo);
}

E_GOP_API_Result MApi_GOP_setPqMlnfo(MS_U8 u8GOP, ST_GOP_PQ_CFD_ML_INFO *pGopMlInfo)
{
	return GOP_SetPqMlnfo(pInstantGOP, u8GOP, pGopMlInfo);
}

E_GOP_API_Result MApi_GOP_SetAidType(MS_U8 u8GOP, MS_U64 u64GopAidType)
{
	return GOP_SetAidType(u8GOP, u64GopAidType);
}

E_GOP_API_Result MApi_GOP_GetRoi(ST_GOP_GETROI *pGOPRoi)
{
	return GOP_GetRoi(pGOPRoi);
}

E_GOP_API_Result MApi_GOP_SetVGOrder(EN_GOP_VG_ORDER eVGOrder)
{
	return GOP_SetVGOrder(eVGOrder);
}

#undef MApi_GOP_C
#endif
