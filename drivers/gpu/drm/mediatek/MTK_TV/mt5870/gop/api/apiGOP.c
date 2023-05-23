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

//-------------------------------------------------------------------------------------------------
//  Local Compiler Options
//-------------------------------------------------------------------------------------------------
#ifndef UTOPIAXP_REMOVE_WRAPPER

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
#ifndef GOP_UTOPIA2K
#define GOP_UTOPIA2K
#endif
#endif

#define DUMP_INFO   0UL

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

#define GetMaxActiveGwinFalse 4UL
#define GetMaxActiveGwinFalse_op 5UL
#define GetMaxActiveGwinFalse_opened 6UL
#define PALETTE_ENTRY_NUM   32UL
#define GWIN_SDRAM_NULL 0x30UL
#define msWarning(c)    do {} while (0)
#define XC_MAIN_WINDOW  0UL

#ifndef UNUSED
#define UNUSED(var) (void)(var)
#endif

//=============================================================
// Debug Logs, level form low to high
#define GOP_INFO(x, args...)  pr_info("GOP API", x, ##args)
// Error, function will be terminated but system not crash
#define GOP_ERR(x, args...)  pr_err("GOP API", x, ##args)
// Critical, system crash. (ex. assert)
#define GOP_FATAL(x, args...) pr_err("GOP API", x, ##args)
//=============================================================

#ifdef MSOS_TYPE_LINUX
#include <assert.h>
#include <unistd.h>
#define GOP_ASSERT(_bool, pri)  do {\
	if (!(_bool)) {\
		GOP_FATAL("\nAssert in %s,%d\n", __func__, __LINE__);\
		(pri);\
		MsOS_DelayTask(100);\
		assert(0);\
	} \
} while (0)
#else
#define GOP_ASSERT(_bool, pri) do {\
	if (!(_bool)) {\
		GOP_FATAL("\nAssert in %s,%d\n", __func__, __LINE__);\
		(pri);\
	} \
} while (0)
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
			GOP_ERR("Open GOP fail\n");
	}
}

E_GOP_API_Result Mapi_GOP_GWIN_ResetGOP(MS_U32 u32Gop)
{

	MS_U32 i;

	GOP_SET_PROPERTY_PARAM ioctl_info;

	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	CheckGOPInstanceOpen();
	ioctl_info.en_pro = E_GOP_RESOURCE;
	ioctl_info.gop_idx = u32Gop;
	ioctl_info.pSetting = (void *)&i;
	ioctl_info.u32Size = sizeof(MS_U32);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

/********************************************************************************/
/// Set gop output color type
/// @param type \b IN: gop output color type
///   - # GOPOUT_RGB => RGB mode
///   - # GOPOUT_YUV => YUV mode
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_OutputColor_EX(MS_U8 u8GOP, EN_GOP_OUTPUT_COLOR type)
{
	EN_GOP_OUTPUT_COLOR output;
	GOP_SET_PROPERTY_PARAM ioctl_info;

	GOP_ENTRY();
	output = type;

	ioctl_info.en_pro = E_GOP_OUTPUT_COLOR;
	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pSetting = (void *)&output;
	ioctl_info.u32Size = sizeof(EN_GOP_OUTPUT_COLOR);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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

	GOP_ENTRY();

	bAlphaInv = bEnable;

	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_ALPHAINVERSE;
	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pSetting = (void *)&bAlphaInv;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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

	bEna = bEnable;

	CheckGOPInstanceOpen();
	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_FORCE_WRITE;
	ioctl_info.gop_idx = 0x0;
	ioctl_info.pSetting = (void *)&bEna;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

/********************************************************************************/
/// Set stretch window H-Stretch ratio.
/// Example: gwin size:960*540  target gwin size: 1920*1080
///     step1: MApi_GOP_GWIN_Set_HSCALE(TRUE, 960, 1920);
///     step2: MApi_GOP_GWIN_Set_VSCALE(TRUE, 540, 1080);
///     step3: MApi_GOP_GWIN_Set_STRETCHWIN(u8GOPNum, E_GOP_DST_OP0, 0, 0, 960, 540);
/// @param bEnable \b IN:
///   - # TRUE enable
///   - # FALSE disable
/// @param src \b IN: original size
/// @param dst \b IN: target size
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_Set_HSCALE_EX(MS_U8 u8GOP, MS_BOOL bEnable, MS_U16 src, MS_U16 dst)
{
	GOP_STRETCH_INFO stretch_info;
	GOP_STRETCH_SET_PARAM ioctl_info;

	GOP_ENTRY();

	memset(&stretch_info, 0x0, sizeof(GOP_STRETCH_INFO));

	stretch_info.SrcRect.w = src;
	if (bEnable)
		stretch_info.DstRect.w = dst;
	else
		stretch_info.DstRect.w = src;

	memset(&ioctl_info, 0x0, sizeof(GOP_STRETCH_SET_PARAM));

	ioctl_info.gop_idx = u8GOP;
	ioctl_info.enStrtchType = E_GOP_STRETCH_HSCALE;
	ioctl_info.pStretch = (MS_U32 *) &stretch_info;
	ioctl_info.u32Size = sizeof(GOP_STRETCH_INFO);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_STRETCH, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

/********************************************************************************/
/// Set stretch window V-Stretch ratio.
/// Example: gwin size:960*540  target gwin size: 1920*1080
///     step1: MApi_GOP_GWIN_Set_HSCALE(TRUE, 960, 1920);
///     step2: MApi_GOP_GWIN_Set_VSCALE(TRUE, 540, 1080);
///     step3: MApi_GOP_GWIN_Set_STRETCHWIN(u8GOPNum, E_GOP_DST_OP0, 0, 0, 960, 540);
/// @param bEnable \b IN:
///   - # TRUE enable
///   - # FALSE disable
/// @param src \b IN: original size
/// @param dst \b IN: target size
/// @return GOP_API_SUCCESS - Success
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_Set_VSCALE_EX(MS_U8 u8GOP, MS_BOOL bEnable, MS_U16 src, MS_U16 dst)
{
	GOP_STRETCH_INFO stretch_info;
	GOP_STRETCH_SET_PARAM ioctl_info;

	GOP_ENTRY();

	memset(&stretch_info, 0x0, sizeof(GOP_STRETCH_INFO));

	stretch_info.SrcRect.h = src;
	if (bEnable)
		stretch_info.DstRect.h = dst;
	else
		stretch_info.DstRect.h = src;

	memset(&ioctl_info, 0x0, sizeof(GOP_STRETCH_SET_PARAM));

	ioctl_info.gop_idx = u8GOP;
	ioctl_info.enStrtchType = E_GOP_STRETCH_VSCALE;
	ioctl_info.pStretch = (MS_U32 *) &stretch_info;
	ioctl_info.u32Size = sizeof(GOP_STRETCH_INFO);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_STRETCH, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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

	if (u32Ret != UTOPIA_STATUS_SUCCESS) {
		if (u32Ret == UTOPIA_STATUS_NOT_SUPPORTED) {
			GOP_ERR("[%s]Function not support\n", __func__);
			eRet = GOP_API_FUN_NOT_SUPPORTED;
		} else {
			GOP_ERR("Ioctl %s fail\n", __func__);
			eRet = GOP_API_FAIL;
		}
	} else
		eRet = GOP_API_SUCCESS;

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
	if (u32Ret != UTOPIA_STATUS_SUCCESS) {
		if ((E_GOP_API_Result) u32Ret == UTOPIA_STATUS_NOT_SUPPORTED) {
			GOP_ERR("[%s]Function not support\n", __func__);
			eRet = GOP_API_FUN_NOT_SUPPORTED;
		} else {
			GOP_ERR("Ioctl %s fail\n", __func__);
			eRet = GOP_API_FAIL;
		}
	}

	return eRet;
}

/// Extend MApi_GOP_GWIN_UpdateRegOnceEx, update special gop.
/// Set gop update register method by only once.
/// Example: if you want to update GOP function A, B, C in the same V sync,
/// please write down your code like below
/// MApi_GOP_GWIN_UpdateRegOnceByMask(u16GopMask,TRUE, TRUE);
/// GOP_FUN_A;
/// GOP_FUN_B;
/// GOP_FUN_C;
/// MApi_GOP_GWIN_UpdateRegOnceByMask(u16GopMask,FALSE, TRUE);
/// @param u16GopMask    \b IN:bit0-gop0, bit1-gop1...
/// @param bWriteRegOnce    \b IN: TRUE/FALSE
/// @param bSync            \b IN: TRUE/FALSE
/// @return GOP_API_SUCCESS - Success
E_GOP_API_Result MApi_GOP_GWIN_UpdateRegOnceByMask(MS_U16 u16GopMask, MS_BOOL bWriteRegOnce,
						   MS_BOOL bSync)
{
	GOP_UPDATE_INFO update;
	GOP_UPDATE_PARAM ioctl_info;

	GOP_ENTRY();
	memset(&update, 0x0, sizeof(GOP_UPDATE_INFO));

	update.gop_idx = u16GopMask;
	update.update_type = E_GOP_UPDATE_CURRENT_ONCE;
	update.bEn = bWriteRegOnce;
	update.bSync = bSync;

	memset(&ioctl_info, 0x0, sizeof(GOP_UPDATE_PARAM));

	ioctl_info.pUpdateInfo = (MS_U32 *) &update;
	ioctl_info.u32Size = sizeof(GOP_UPDATE_INFO);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_UPDATE, &ioctl_info) != UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;

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
E_GOP_API_Result MApi_GOP_GWIN_UpdateRegOnceEx2(MS_U8 u8GOP, MS_BOOL bWriteRegOnce, MS_BOOL bSync)
{
	GOP_UPDATE_INFO update;
	GOP_UPDATE_PARAM ioctl_info;

	GOP_ENTRY();
	memset(&update, 0x0, sizeof(GOP_UPDATE_INFO));

	update.gop_idx = u8GOP;
	update.update_type = E_GOP_UPDATE_ONCE;
	update.bEn = bWriteRegOnce;
	update.bSync = bSync;

	memset(&ioctl_info, 0x0, sizeof(GOP_UPDATE_PARAM));

	ioctl_info.pUpdateInfo = (MS_U32 *) &update;
	ioctl_info.u32Size = sizeof(GOP_UPDATE_INFO);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_UPDATE, &ioctl_info) != UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;

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

	GOP_ENTRY();

	bShared = bIsShared;

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_SHARE;
	ioctl_info.GwinId = winId;
	ioctl_info.pSet = (void *)&bShared;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_PROPERTY, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

/// Set Reference cnt of shared GWin.
/// @param winId \b IN: GWIN ID for shared
/// @param u16SharedCnt \b IN: shared reference cnt.
/// @return GOP_API_SUCCESS - Success
E_GOP_API_Result MApi_GOP_GWIN_SetGWinSharedCnt(MS_U8 winId, MS_U16 u16SharedCnt)
{
	MS_U16 u16Cnt;
	GOP_GWIN_PROPERTY_PARAM ioctl_info;

	GOP_ENTRY();

	u16Cnt = u16SharedCnt;

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_SHARE_CNT;
	ioctl_info.GwinId = winId;
	ioctl_info.pSet = (void *)&u16Cnt;
	ioctl_info.u32Size = sizeof(MS_U16);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_PROPERTY, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_ENABLE;
	ioctl_info.GwinId = winId;
	ioctl_info.pSet = (void *)&bEnable;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;

}

/********************************************************************************/
/// Check if GWIN is enabled
/********************************************************************************/
MS_BOOL MApi_GOP_GWIN_IsEnabled(void)
{
	MS_U8 i = 0;
	MS_BOOL bEnable = FALSE;

	GOP_ENTRY();

	while (i < MAX_GWIN_SUPPORT) {
		bEnable = MApi_GOP_GWIN_IsGWINEnabled(i);
		if (bEnable == TRUE)
			return TRUE;
		i++;
	}
	return FALSE;
}

/// Check if all some GWIN is currently enabled
/// @param  winId \b IN: gwin id
/// @return  - the according GWin is enabled or not
MS_BOOL MApi_GOP_GWIN_IsGWINEnabled(MS_U8 winId)
{
	MS_BOOL bEn;
	GOP_GWIN_PROPERTY_PARAM ioctl_info;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_ENABLE;
	ioctl_info.GwinId = winId;
	ioctl_info.pSet = (void *)&bEn;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_GET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return bEn;


}

/*******************************************************************************/
//Set which MUX select which GOP ,when different gop do the alpha blending
//@param pGopMuxConfig \b IN:information about GOP and corresponding level
//   #u8GopNum: the total GOP counts who need to select change Mux
//   #GopMux[i].u8GopIndex :the GOP which need to change Mux
//   #GopMux[i].u8MuxIndex :the GOP corresponding Mux
//@return GOP_API_SUCCESS - Success
/*******************************************************************************/

E_GOP_API_Result MApi_GOP_GWIN_SetMux(GOP_MuxConfig *pGopMuxConfig, MS_U32 u32SizeOfMuxInfo)
{
	GOP_SETMUX MuxSet;
	MS_U32 i;
	GOP_SETMUX_PARAM ioctl_info;

	GOP_ENTRY();

	memset(&MuxSet, 0x0, sizeof(GOP_MuxConfig));

	MuxSet.MuxCount = pGopMuxConfig->u8MuxCounts;

	for (i = 0; i < pGopMuxConfig->u8MuxCounts; i++) {
		MuxSet.gop[i] = pGopMuxConfig->GopMux[i].u8GopIndex;
		MuxSet.mux[i] = pGopMuxConfig->GopMux[i].u8MuxIndex;
	}

	memset(&ioctl_info, 0x0, sizeof(GOP_SETMUX_PARAM));

	ioctl_info.pMuxInfo = (MS_U32 *) &MuxSet;
	ioctl_info.u32Size = sizeof(GOP_SETMUX);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_MUX, &ioctl_info) != UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_LAYER, &ioctl_info) !=
UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_SETDST_PARAM));

	ioctl_info.en_dst = dsttype;
	ioctl_info.gop_idx = u8GOP;

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_DST, &ioctl_info) != UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

/******************************************************************************/
/// Get free frame buffer id
/// @return frame buffer id. If return oxFF, it represents no free frame buffer id for use.
/******************************************************************************/
MS_U32 MApi_GOP_GWIN_GetFree32FBID(void)
{
	MS_U32 u32FBId = 0xFFFFFFFF;
	GOP_FB_PROPERTY_PARAM ioctl_info;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_FB_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_FB_OBTAIN;
	ioctl_info.FBId = 0x0;
	ioctl_info.pSet = (MS_U32 *) &u32FBId;
	ioctl_info.u32Size = sizeof(MS_U32);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_FB_GET_PROPERTY, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return u32FBId;
	}

	return u32FBId;
}

/// Destroy the frame buffer and return the memory to mmgr
/// @param fbId  \b IN frame buffer id
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_CRT_GWIN_NOAVAIL - destroy frame buffer fail
MS_U8 MApi_GOP_GWIN_Destroy32FB(MS_U32 u32fbId)
{
	GOP_DELETE_BUFFER_PARAM ioctl_info;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_DELETE_BUFFER_PARAM));

	ioctl_info.pBufId = (MS_U32 *) &u32fbId;
	ioctl_info.u32Size = sizeof(MS_U32);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_FB_DESTROY, &ioctl_info) !=
UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GWIN_OK;
}

E_GOP_API_Result MApi_GOP_GWIN_Delete32FB(MS_U32 u32fbId)
{
	GOP_ENTRY();
	GOP_INFO("%s:%d  FBId = %td\n", __func__, __LINE__, (ptrdiff_t)u32fbId);
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

	GOP_ENTRY();

	blendInfo.Coef = u8coef;
	blendInfo.bEn = bEnable;

	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_PROPERTY_PARAM));

	ioctl_info.en_property = E_GOP_GWIN_BLENDING;
	ioctl_info.GwinId = u8win;
	ioctl_info.pSet = (MS_U32 *) &blendInfo;
	ioctl_info.u32Size = sizeof(GOP_GWIN_BLENDING);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SET_PROPERTY, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;

}

//-------------------------------------------------------------------------------------------------
/// Get maximum support gop number
/// @return maximum support gop number
//-------------------------------------------------------------------------------------------------
MS_U8 MApi_GOP_GWIN_GetMaxGOPNum(void)
{
	MS_U8 u8MaxGop = 0;
	GOP_GET_STATUS_PARAM ioctl_info;

	memset(&ioctl_info, 0x0, sizeof(GOP_GET_STATUS_PARAM));

	CheckGOPInstanceOpen();

	ioctl_info.type = 0;
	ioctl_info.en_status = E_GOP_STATUS_GOP_MAXNUM;
	ioctl_info.pStatus = (void *)&u8MaxGop;
	ioctl_info.u32Size = sizeof(MS_U8);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_STATUS, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return u8MaxGop;

}

//-------------------------------------------------------------------------------------------------
/// Get maximum support gwin number by all gop
/// @return maximum support gwin number by all gop
//-------------------------------------------------------------------------------------------------
MS_U8 MApi_GOP_GWIN_GetTotalGwinNum(void)
{
	MS_U8 u8TotalWin = 0;
	GOP_GET_STATUS_PARAM ioctl_info;

	memset(&ioctl_info, 0x0, sizeof(GOP_GET_STATUS_PARAM));

	CheckGOPInstanceOpen();
	ioctl_info.type = 0;
	ioctl_info.en_status = E_GOP_STATUS_GWIN_TOTALNUM;
	ioctl_info.pStatus = (void *)&u8TotalWin;
	ioctl_info.u32Size = sizeof(MS_U8);
	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_STATUS, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return u8TotalWin;
}

//-------------------------------------------------------------------------------------------------
/// Get maximum gwin number by individual gop
/// @param u8GopNum \b IN: Number of GOP
/// @return maximum gwin number by individual gop
//-------------------------------------------------------------------------------------------------
MS_U8 MApi_GOP_GWIN_GetGwinNum(MS_U8 u8GopNum)
{
	GOP_GWIN_NUM gwin_num;
	GOP_GET_STATUS_PARAM ioctl_info;

	gwin_num.gop_idx = u8GopNum;

	memset(&ioctl_info, 0x0, sizeof(GOP_GET_STATUS_PARAM));

	CheckGOPInstanceOpen();
	ioctl_info.type = 0;
	ioctl_info.en_status = E_GOP_STATUS_GWIN_MAXNUM;
	ioctl_info.pStatus = (MS_U32 *) &gwin_num;
	ioctl_info.u32Size = sizeof(GOP_GWIN_NUM);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_STATUS, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return gwin_num.gwin_num;
}

MS_U8 MApi_GOP_GetGwinIdx(MS_U8 u8GOPIdx)
{
	GOP_GWIN_NUM gwin_num;
	GOP_GET_STATUS_PARAM ioctl_info;

	memset(&ioctl_info, 0x0, sizeof(GOP_GET_STATUS_PARAM));

	CheckGOPInstanceOpen();

	gwin_num.gop_idx = u8GOPIdx;
	ioctl_info.type = 0;
	ioctl_info.en_status = E_GOP_STATUS_GWINIDX;
	ioctl_info.pStatus = (MS_U32 *) &gwin_num;
	ioctl_info.u32Size = sizeof(GOP_GWIN_NUM);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_STATUS, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return gwin_num.gwin_num;
}

E_GOP_API_Result MApi_GOP_GWIN_SetPreAlphaMode(MS_U8 u8GOP, MS_BOOL bEnble)
{
	MS_BOOL bPreAlpha;
	GOP_SET_PROPERTY_PARAM ioctl_info;

	GOP_ENTRY();

	bPreAlpha = bEnble;

	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_PREALPHAMODE;
	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pSetting = (void *)&bPreAlpha;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;

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

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_SETMIRROR_PARAM));

	if (bEnable == TRUE)
		ioctl_info.dir = E_GOP_MIRROR_H_ONLY;
	else
		ioctl_info.dir = E_GOP_MIRROR_H_NONE;

	ioctl_info.gop_idx = (MS_U32) u8GOP;	//get current gop for driver capability
	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_MIRROR, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_SETMIRROR_PARAM));

	if (bEnable == TRUE)
		ioctl_info.dir = E_GOP_MIRROR_V_ONLY;
	else
		ioctl_info.dir = E_GOP_MIRROR_V_NONE;

	ioctl_info.gop_idx = (MS_U32) u8GOP;	//get current gop for driver capability
	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_MIRROR, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;

}

//-------------------------------------------------------------------------------------------------
/// GOP Exit
/// @param  void                \b IN: none
//-------------------------------------------------------------------------------------------------
void MApi_GOP_Exit(void)
{
	//App call exit, should wait mutex return to avoid mutex conflict use with other thread.
	//But do not need GOP_RETURN, because all mutex and resource has been released in atexit.
#if defined(MSOS_TYPE_LINUX)
	GOP_INFO("---%s %d: PID[%td], TID[%td] tries to exit\n", __func__, __LINE__,
		 (ptrdiff_t) getpid(), (ptrdiff_t) MsOS_GetOSThreadID());
#elif defined(MSOS_TYPE_LINUX_KERNEL)
	GOP_INFO("---%s %d: PID[%td], TID[%td] tries to exit\n", __func__, __LINE__,
		 (ptrdiff_t) pInstantGOP, (ptrdiff_t) MsOS_GetOSThreadID());
#else
	GOP_INFO("---%s %d: PID[%td], TID[%td] tries to exit\n", __func__, __LINE__,
		 (ptrdiff_t) 0, (ptrdiff_t) MsOS_GetOSThreadID());
#endif
	GOP_ENTRY();
#ifdef MSOS_TYPE_LINUX
	MS_U32 value = 0;
	GOP_MISC_PARAM ioctl_info;

	memset(&ioctl_info, 0x0, sizeof(GOP_MISC_PARAM));

	ioctl_info.misc_type = E_GOP_MISC_AT_EXIT;
	ioctl_info.pMISC = (MS_U32 *) &value;
	ioctl_info.u32Size = sizeof(MS_U32);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_MISC, &ioctl_info) != UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return;
	}
#else
	GOP_INFO("not enable MSOS_TYPE_LINUX\n");
#endif
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

	CheckGOPInstanceOpen();
	memset(&ioctl_info, 0x0, sizeof(GOP_INIT_PARAM));

	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pInfo = (MS_U32 *) pGopInit;
	ioctl_info.u32Size = sizeof(GOP_InitInfo);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_INIT, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}
#ifdef MSOS_TYPE_LINUX
	atexit(MApi_GOP_Exit);
#endif
	bInit = TRUE;
	return GOP_API_SUCCESS;
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

	CheckGOPInstanceOpen();
	//init GOP0
	memset(&ioctl_info, 0x0, sizeof(GOP_INIT_PARAM));

	ioctl_info.gop_idx = 0;
	ioctl_info.pInfo = (MS_U32 *) pGopInit;
	ioctl_info.u32Size = sizeof(GOP_InitInfo);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_INIT, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}
	//init others
	for (i = 1; i < MApi_GOP_GWIN_GetMaxGOPNum(); i++) {
		ioctl_info.gop_idx = i;
		if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_INIT, (void *)&ioctl_info) !=
		    UTOPIA_STATUS_SUCCESS) {
			GOP_ERR("Ioctl %s fail\n", __func__);
			return GOP_API_FAIL;
		}
	}
	bInit = TRUE;
	return GOP_API_SUCCESS;
}

/******************************************************************************/
/// Delete the GWIN, free corresponding frame buffer
/// @param gId \b IN \copydoc GWINID
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
/******************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_DestroyWin(MS_U8 gId)
{
	GOP_GWIN_DESTROY_PARAM ioctl_info;

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_DESTROY_PARAM));
	ioctl_info.GwinId = gId;

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_DESTROY, (void *)&ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;

}

MS_U32 MApi_GOP_GWIN_GetMax32FBNum(void)
{
	MS_PHY u32FBIDNum = INVALID_POOL_NEXT_FBID;

	MApi_GOP_GetConfigEx(0, E_GOP_GET_MAXFBNUM, &u32FBIDNum);
	return u32FBIDNum;
}

MS_U8 MApi_GOP_GWIN_Create32FBbyStaticAddr2(MS_U32 u32fbId, MS_U16 dispX, MS_U16 dispY,
					    MS_U16 width, MS_U16 height, MS_U16 fbFmt,
					    MS_PHY phyFbAddr, EN_GOP_FRAMEBUFFER_STRING FBString)
{
	GOP_BUFFER_INFO BuffInfo;
	GOP_CREATE_BUFFER_PARAM ioctl_info;

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

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_FB_CREATE, &ioctl_info) !=
UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;

}

/******************************************************************************/
/// Change a GWIN's frame buffer, this enables an off screen buffer to be shown
/// @param fbId \b IN frame buffer id
/// @param gwinId \b IN \copydoc GWINID
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
/******************************************************************************/
E_GOP_API_Result MApi_GOP_GWIN_Map32FB2Win(MS_U32 u32fbId, MS_U8 u8gwinId)
{
	GOP_GWIN_MAPFBINFO_PARAM ioctl_info;

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_GWIN_MAPFBINFO_PARAM));

	ioctl_info.fbid = u32fbId;
	ioctl_info.GwinId = u8gwinId;

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_MAPFB2WIN, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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
	PST_GOP_AUTO_DETECT_BUF_INFO pstGopAutoDetectInfo = NULL;
	ST_GOP_AUTO_DETECT_BUF_INFO stGopAutoDetectInfo;
	void *pInfo = NULL;
	MS_U32 u32CopiedLength = 0;

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
	case E_GOP_AFBC_RESET:
		{
			stIoctlPropInfo.en_pro = E_GOP_AFBC_CORE_RESET;
			stIoctlPropInfo.gop_idx = u8Gop;
			stIoctlPropInfo.pSetting = plist;
			stIoctlPropInfo.u32Size = sizeof(MS_BOOL);
			pInfo = (void *)&stIoctlPropInfo;
			enApiCmdType = MAPI_CMD_GOP_SET_PROPERTY;
			break;
		}
	case E_GOP_AFBC_ENABLE:
		{
			stIoctlPropInfo.en_pro = E_GOP_AFBC_CORE_ENABLE;
			stIoctlPropInfo.gop_idx = u8Gop;
			stIoctlPropInfo.pSetting = plist;
			stIoctlPropInfo.u32Size = sizeof(MS_BOOL);
			pInfo = (void *)&stIoctlPropInfo;
			enApiCmdType = MAPI_CMD_GOP_SET_PROPERTY;
			break;
		}
	case E_GOP_VOP_PATH_SEL:
		stIoctlPropInfo.en_pro = E_GOP_SET_VOP_PATH;
		stIoctlPropInfo.gop_idx = u8Gop;
		stIoctlPropInfo.pSetting = plist;
		stIoctlPropInfo.u32Size = sizeof(EN_GOP_VOP_PATH_MODE);
		pInfo = (void *)&stIoctlPropInfo;
		enApiCmdType = MAPI_CMD_GOP_SET_PROPERTY;
		break;
	case E_GOP_AUTO_DETECT_BUFFER:
		{
			memset(&stGopAutoDetectInfo, 0, sizeof(ST_GOP_AUTO_DETECT_BUF_INFO));
			pstGopAutoDetectInfo = (PST_GOP_AUTO_DETECT_BUF_INFO) plist;
			u32CopiedLength = sizeof(ST_GOP_AUTO_DETECT_BUF_INFO);

			if (pstGopAutoDetectInfo == NULL) {
				GOP_ERR("[Error][%s]Null parameter!!\n", __func__);
				return GOP_API_FAIL;
			}
			if (pstGopAutoDetectInfo->u32Version < 1) {
				GOP_ERR("[Error][%s]please check your u32Version!!\n", __func__);
				return GOP_API_FAIL;
			}
			if (pstGopAutoDetectInfo->u32Version >
ST_GOP_AUTO_DETECT_BUF_INFO_VERSION) {
#if defined(__aarch64__)
				GOP_ERR("[%s]old struct length:%lu\n", __func__,
sizeof(ST_GOP_AUTO_DETECT_BUF_INFO));
#else
				GOP_ERR("[%s]old struct length:%u!\n", __func__,
sizeof(ST_GOP_AUTO_DETECT_BUF_INFO));
#endif
			}
			if ((pstGopAutoDetectInfo->u32Version < ST_GOP_AUTO_DETECT_BUF_INFO_VERSION)
			    || (pstGopAutoDetectInfo->u32Length <
				sizeof(ST_GOP_AUTO_DETECT_BUF_INFO))) {
				GOP_ERR("[%s]no access the space in old struct\n", __func__);
				u32CopiedLength = pstGopAutoDetectInfo->u32Length;
			}
			memcpy(&stGopAutoDetectInfo, pstGopAutoDetectInfo, u32CopiedLength);
			memset(&stIoctlConfigInfo, 0x0, sizeof(GOP_SETCONFIG_PARAM));

			stIoctlConfigInfo.cfg_type = enType;
			stIoctlConfigInfo.pCfg = &stGopAutoDetectInfo;
			stIoctlConfigInfo.u32Size = sizeof(ST_GOP_AUTO_DETECT_BUF_INFO);
			pInfo = (void *)&stIoctlConfigInfo;
			enApiCmdType = MAPI_CMD_GOP_SET_CONFIG;
			break;
		}
	default:
		return GOP_API_INVALID_PARAMETERS;
	}
	if (UtopiaIoctl(pInstantGOP, enApiCmdType, pInfo) != UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	if (enType == E_GOP_AUTO_DETECT_BUFFER) {
		if (pstGopAutoDetectInfo->u32Version >= 1)
			memcpy(pstGopAutoDetectInfo, &stGopAutoDetectInfo, u32CopiedLength);
	}

	return GOP_API_SUCCESS;
}


/********************************************************************************/
/// Get config by GOP.
/// @param u8GOP \b IN:  GOP number
/// @param pstInfo \b IN misc info
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
/********************************************************************************/
E_GOP_API_Result MApi_GOP_GetConfigEx(MS_U8 u8Gop, EN_GOP_CONFIG_TYPE enType, void *plist)
{
	E_GOP_API_CMD_TYPE enApiCmdType;
	GOP_SETCONFIG_PARAM ioctl_ConfigInfo;
	GOP_SET_PROPERTY_PARAM ioctl_PropInfo;
	void *pInfo = NULL;

	CheckGOPInstanceOpen();

	switch (enType) {
	case E_GOP_IGNOREINIT:
		{
			ioctl_ConfigInfo.cfg_type = enType;
			ioctl_ConfigInfo.pCfg = plist;
			ioctl_ConfigInfo.u32Size = sizeof(EN_GOP_IGNOREINIT);
			pInfo = (void *)&ioctl_ConfigInfo;
			enApiCmdType = MAPI_CMD_GOP_GET_CONFIG;
			break;
		}
	case E_GOP_GET_MAXFBNUM:
		{
			ioctl_PropInfo.en_pro = E_GOP_MAXFBNUM;
			ioctl_PropInfo.gop_idx = u8Gop;
			ioctl_PropInfo.pSetting = plist;
			ioctl_PropInfo.u32Size = sizeof(MS_U32);
			pInfo = (void *)&ioctl_PropInfo;
			enApiCmdType = MAPI_CMD_GOP_GET_PROPERTY;
			break;
		}
	default:
		return GOP_API_INVALID_PARAMETERS;
	}
	if (UtopiaIoctl(pInstantGOP, enApiCmdType, pInfo) != UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
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


	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GWIN_SETDISPLAY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

/// API for Query GOP Capability
/// @param eCapType \b IN: Capability type
/// @param pRet     \b OUT: return value
/// @param ret_size \b IN: input structure size
/// @return GOP_API_SUCCESS - Success
E_GOP_API_Result MApi_GOP_GetChipCaps(EN_GOP_CAPS eCapType, void *pRet, MS_U32 ret_size)
{
	GOP_GETCAPS_PARAM gopcaps;

	CheckGOPInstanceOpen();

	memset(&gopcaps, 0x0, sizeof(GOP_GETCAPS_PARAM));

	gopcaps.caps = eCapType;
	gopcaps.pInfo = pRet;
	gopcaps.u32Size = ret_size;

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_CHIPCAPS, &gopcaps) !=
		   UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

E_GOP_API_Result MApi_GOP_IsRegUpdated(MS_U8 u8GopType)
{
	MS_BOOL bUpdate = FALSE;
	GOP_SET_PROPERTY_PARAM ioctl_info;

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_REG_UPDATED;
	ioctl_info.gop_idx = u8GopType;
	ioctl_info.pSetting = (void *)&bUpdate;
	ioctl_info.u32Size = sizeof(MS_BOOL);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_GET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return (E_GOP_API_Result)bUpdate;
	}

	return (E_GOP_API_Result)bUpdate;

}

/******************************************************************************/
/// set GOP power state
/// @param enPowerState\b IN power status
/******************************************************************************/
E_GOP_API_Result MApi_GOP_SetPowerState(EN_POWER_MODE enPowerState)
{
	GOP_POWERSTATE_PARAM PowerState;

	CheckGOPInstanceOpen();
	GOP_ENTRY();
	memset(&PowerState, 0x0, sizeof(GOP_POWERSTATE_PARAM));

	PowerState.pInfo = &enPowerState;
	PowerState.u32Size = sizeof(GOP_POWERSTATE_PARAM);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_POWERSTATE, &PowerState) !=
UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

E_GOP_API_Result MApi_GOP_SetGOPCscTuning(MS_U32 u32GOPNum, ST_GOP_CSC_PARAM *pstGOPCscParam)
{
	GOP_SET_PROPERTY_PARAM stIoctlInfo;
	ST_GOP_CSC_PARAM stCopiedCSCParam;
	MS_U32 u32Ret = 0;
	E_GOP_API_Result eRet = GOP_API_SUCCESS;

	GOP_ENTRY();

	memset(&stCopiedCSCParam, 0, sizeof(ST_GOP_CSC_PARAM));

	if (pstGOPCscParam == NULL) {
		GOP_ERR("[%s][%d] pstGOPCscParam is NULL\n", __func__, __LINE__);
		return GOP_API_FAIL;
	}

	memcpy(&stCopiedCSCParam, pstGOPCscParam, sizeof(ST_GOP_CSC_PARAM));
	memset(&stIoctlInfo, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	stIoctlInfo.en_pro = E_GOP_SET_CSC_TUNING;
	stIoctlInfo.gop_idx = u32GOPNum;
	stIoctlInfo.pSetting = (void *)&stCopiedCSCParam;
	stIoctlInfo.u32Size = sizeof(ST_GOP_CSC_PARAM);

	u32Ret = UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &stIoctlInfo);

	if (u32Ret != UTOPIA_STATUS_SUCCESS) {
		if (u32Ret == UTOPIA_STATUS_NOT_SUPPORTED) {
			GOP_ERR("[%s]Function not support\n", __func__);
			eRet = GOP_API_FUN_NOT_SUPPORTED;
		} else {
			GOP_ERR("Ioctl %s fail\n", __func__);
			eRet = GOP_API_FAIL;
		}
	}

	return eRet;
}

E_GOP_API_Result MApi_GOP_GetOsdNonTransCnt(MS_U32 *pu32Count)
{
	GOP_MISC_PARAM ioctl_info;

	GOP_ENTRY();
	memset(&ioctl_info, 0x0, sizeof(GOP_MISC_PARAM));

	ioctl_info.misc_type = E_GOP_MISC_GET_OSD_NONTRANS_CNT;
	ioctl_info.pMISC = (MS_U32 *) pu32Count;
	ioctl_info.u32Size = sizeof(MS_U32);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_MISC, &ioctl_info) != UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

E_GOP_API_Result MApi_GOP_SetPixelShiftPD(MS_S32 s32Offset)
{
	MS_U32 u32GOP = 0;
	GOP_SET_PROPERTY_PARAM ioctl_info;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_PIXELSHIFT_PD;
	ioctl_info.gop_idx = u32GOP;
	ioctl_info.pSetting = (void *)&s32Offset;
	ioctl_info.u32Size = sizeof(MS_S32);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

E_GOP_API_Result MApi_GOP_SetCropWindow(MS_U8 u8GOPIdx, ST_GOP_CROP_WINDOW *pstCropWin)
{
	GOP_SET_PROPERTY_PARAM ioctl_info;

	GOP_ENTRY();

	memset(&ioctl_info, 0x0, sizeof(GOP_SET_PROPERTY_PARAM));

	ioctl_info.en_pro = E_GOP_SET_CROP_WIN;
	ioctl_info.gop_idx = u8GOPIdx;
	ioctl_info.pSetting = (void *)pstCropWin;
	ioctl_info.u32Size = sizeof(ST_GOP_CROP_WINDOW);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_SET_PROPERTY, &ioctl_info) !=
	    UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

E_GOP_API_Result MApi_GOP_GWIN_INTERRUPT(MS_U8 u8GOP, MS_BOOL bEnable)
{
	GOP_INTERRUPT_PARAM ioctl_info;
	MS_U32 bEn = bEnable;

	memset(&ioctl_info, 0x0, sizeof(GOP_INTERRUPT_PARAM));

	ioctl_info.gop_idx = u8GOP;
	ioctl_info.pSetting = &bEn;
	ioctl_info.u32Size = sizeof(GOP_INTERRUPT_PARAM);

	if (UtopiaIoctl(pInstantGOP, MAPI_CMD_GOP_INTERRUPT, &ioctl_info) !=
UTOPIA_STATUS_SUCCESS) {
		GOP_ERR("Ioctl %s fail\n", __func__);
		return GOP_API_FAIL;
	}

	return GOP_API_SUCCESS;
}

#undef MApi_GOP_C
#endif
