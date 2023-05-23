// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#ifndef MSOS_TYPE_LINUX_KERNEL
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#else
#include <linux/slab.h>
#endif
#include <linux/uaccess.h>
#include "apiGOP.h"
#include "apiGOP_priv.h"
#include "drvGOP.h"
#include "drvGFLIP.h"
#include "drvGOP_priv.h"
#include "mtk_tv_drm_log.h"

enum {
	GOP_POOL_ID_GOP0 = 0
} eGOPPoolID;

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define MTK_DRM_MODEL MTK_DRM_MODEL_GOP

// Define return values of check align
#define CHECKALIGN_SUCCESS                  1UL
#define CHECKALIGN_PARA_FAIL                3UL

//#define GWIN_SDRAM_PG_UNIT 0x00000010  // 16-byte page-unit for gwin_sdram
#define GWIN_SDRAM_PG_ALIGN(_x)    ALIGN_32(_x)
#define IS_GWIN_SDRAM_ALIGN(_x)  ((_x) == GWIN_SDRAM_PG_ALIGN(_x))

static MS_BOOL bFirstInit = FALSE;
static MS_BOOL bMMIOInit = FALSE;

#ifdef INSTANT_PRIVATE
#define g_pGOPCtxLocal  psGOPInstPri->g_pGOPCtxLocal
#else
MS_GOP_CTX_LOCAL *g_pGOPCtxLocal;
#endif
static GOP_TYPE_DEF GOPTYPE;

#ifdef STI_PLATFORM_BRING_UP
#define GOP_M_INFO(x, args...)
// Warning, illegal parameter but can be self fixed in functions
#define GOP_M_WARN(x, args...)
//  Need debug, illegal parameter.
#define GOP_M_DBUG(x, args...)
// Error, function will be terminated but system not crash
#define GOP_M_ERR(x, args...) { UTP_GOP_LOGE("MAPI_GOP", x, ##args); }
// Critical, system crash. (ex. assert)
#define GOP_M_FATAL(x, args...)
#else
#include "ULog.h"
MS_U32 u32GOPDbgLevel_mapi;

#ifdef CONFIG_GOP_DEBUG_LEVEL
// Debug Logs, level form low(INFO) to high(FATAL, always show)
// Function information, ex function entry
#define GOP_M_INFO(x, args...)   do {\
	if (u32GOPDbgLevel_mapi >= E_GOP_Debug_Level_HIGH) {\
		ULOGI("GOP mAPI", x, ##args);\
	} \
} while (0)
// Warning, illegal parameter but can be self fixed in functions
#define GOP_M_WARN(x, args...)  do {\
	if (u32GOPDbgLevel_mapi >= E_GOP_Debug_Level_HIGH) {\
		ULOGW("GOP mAPI", x, ##args);\
	} \
} while (0)
//  Need debug, illegal parameter.
#define GOP_M_DBUG(x, args...) do {\
	if (u32GOPDbgLevel_mapi >= E_GOP_Debug_Level_MED) {\
		ULOGD("GOP mAPI", x, ##args);\
		} \
} while (0)
// Error, function will be terminated but system not crash
#define GOP_M_ERR(x, args...) { ULOGE("GOP mAPI", x, ##args); }
// Critical, system crash. (ex. assert)
#define GOP_M_FATAL(x, args...) { ULOGF("GOP mAPI", x, ##args); }
#else
#define GOP_M_INFO(x, args...)
// Warning, illegal parameter but can be self fixed in functions
#define GOP_M_WARN(x, args...)
//  Need debug, illegal parameter.
#define GOP_M_DBUG(x, args...)
// Error, function will be terminated but system not crash
#define GOP_M_ERR(x, args...)
// Critical, system crash. (ex. assert)
#define GOP_M_FATAL(x, args...)
#endif
#endif

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------

static MS_BOOL _GOP_IsFbIdValid(void *pInstance, MS_U32 u32FbId)
{
	if (u32FbId > DRV_MAX_GWIN_FB_SUPPORT)
		return FALSE;
	else
		return TRUE;
}


static GOP_WinFB_INFO *_GetWinFB(void *pInstance, MS_U32 FbId)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsFbIdValid(pInstance, FbId))
		return NULL;

#if WINFB_INSHARED
	//memcpy(pWinFB,&(g_pGOPCtxLocal->pGOPCtxShared->winFB[FbId]),sizeof(GOP_WinFB_INFO));
	return &(g_pGOPCtxLocal->pGOPCtxShared->winFB[FbId]);
#else
	//memcpy(pWinFB,g_pGOPCtxLocal->winFB[FbId],sizeof(GOP_WinFB_INFO));
	return &g_pGOPCtxLocal->winFB[FbId];
#endif

}

#ifdef CONFIG_GOP_AFBC_FEATURE
static void _GOP_MAP_FBID_AFBCCmd(void *pInstance, MS_U32 u32fbId, MS_U8 *u8AFBC_Cmd,
				  MS_BOOL *AFBC_Enable)
{
	GOP_WinFB_INFO *pwinFB;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	pwinFB = _GetWinFB(pInstance, u32fbId);
	if (pwinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return;
	}

	switch (pwinFB->FBString) {
	case E_DRV_GOP_FB_AFBC_V1P2_ARGB8888:
		*AFBC_Enable = TRUE;
		*u8AFBC_Cmd = (MS_U8) E_DRV_GOP_AFBC_ARGB8888;
		break;
	default:
		*AFBC_Enable = FALSE;
		*u8AFBC_Cmd = (MS_U8) E_DRV_GOP_FB_NULL;
		break;
	}
}
#endif

//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------

static MS_U8 _GOP_GetMaxGOPNum(void *pInstance)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	return MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal);
}


static MS_BOOL _GOP_IsGopNumValid(void *pInstance, MS_U32 gop)
{
	if (gop >= _GOP_GetMaxGOPNum(pInstance))
		return FALSE;
	else
		return TRUE;
}

static MS_BOOL _GOP_IsGwinIdValid(void *pInstance, MS_U8 u8GwinID)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (u8GwinID >= SHARED_GWIN_MAX_COUNT)
		return FALSE;
	else if (u8GwinID >= g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum)
		return FALSE;
	else
		return TRUE;
}

static MS_BOOL IsScalerOutputInterlace(void)
{
	return FALSE;
}

MS_U8 _GOP_SelGwinId2(void *pInstance, MS_U32 u32GOP, MS_U32 u8GwinIdx)
{
	MS_U8 tmp;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGopNumValid(pInstance, u32GOP)) {
		GOP_M_ERR("[%s][%d]GOP %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) u32GOP);
		return GWIN_NO_AVAILABLE;
	}
	tmp = MDrv_GOP_SelGwinIdByGOP(g_pGOPCtxLocal, u32GOP, u8GwinIdx);

	return tmp;

}

static E_GOP_API_Result _GOP_MAP_MUX_Enum(void *pInstance, EN_Gop_MuxSel GOPMux,
					  Gop_MuxSel *pGOPMux)
{
	E_GOP_API_Result ret = GOP_API_FAIL;

	switch (GOPMux) {
	case EN_GOP_MUX0:
		*pGOPMux = E_GOP_MUX0;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_MUX1:
		*pGOPMux = E_GOP_MUX1;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_MUX2:
		*pGOPMux = E_GOP_MUX2;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_MUX3:
		*pGOPMux = E_GOP_MUX3;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_MUX4:
		*pGOPMux = E_GOP_MUX4;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_IP0_MUX:
		*pGOPMux = E_GOP_IP0_MUX;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_FRC_MUX0:
		*pGOPMux = E_GOP_FRC_MUX0;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_FRC_MUX1:
		*pGOPMux = E_GOP_FRC_MUX1;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_FRC_MUX2:
		*pGOPMux = E_GOP_FRC_MUX2;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_FRC_MUX3:
		*pGOPMux = E_GOP_FRC_MUX3;
		ret = GOP_API_SUCCESS;
		break;
	case EN_GOP_BYPASS_MUX0:
		*pGOPMux = E_GOP_BYPASS_MUX0;
		ret = GOP_API_SUCCESS;
		break;
	default:
		*pGOPMux = E_GOP_MUX_INVALID;
		GOP_M_ERR("\n MAP GOP Mux error!!\n");
		ret = GOP_API_FAIL;
		break;
	}
	return ret;
}

static MS_U32 _GOP_Map_APIDst2DRV_Enum(void *pInstance, EN_GOP_DST_TYPE GopDst,
				       DRV_GOPDstType *pGopDst)
{
	MS_U32 ret = GOP_API_FAIL;

	switch (GopDst) {
	case E_GOP_DST_IP0:
		*pGopDst = E_DRV_GOP_DST_IP0;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	case E_GOP_DST_OP0:
		*pGopDst = E_DRV_GOP_DST_OP0;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	case E_GOP_DST_FRC:
		*pGopDst = E_DRV_GOP_DST_FRC;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	default:
		*pGopDst = E_DRV_GOP_DST_INVALID;
		GOP_M_ERR("\n MAP GOP Dst plane error!!\n");
		ret = (MS_U32) GOP_API_FAIL;
		break;
	}
	return ret;
}

static MS_U32 _GOP_CalcPitch(void *pInstance, MS_U8 fbFmt, MS_U16 width)
{
	MS_U16 pitch = 0;

	switch (fbFmt) {
	case E_GOP_COLOR_RGB565:
	case E_GOP_COLOR_BGR565:
	case E_GOP_COLOR_ARGB1555:
	case E_GOP_COLOR_ABGR1555:
	case E_GOP_COLOR_BGRA5551:
	case E_GOP_COLOR_RGBA5551:
	case E_GOP_COLOR_ARGB4444:
	case E_GOP_COLOR_RGBA4444:
	case E_GOP_COLOR_ABGR4444:
	case E_GOP_COLOR_BGRA4444:
	case E_GOP_COLOR_YUV422:
		pitch = width << 1;
		break;
	case E_GOP_COLOR_AYUV8888:
	case E_GOP_COLOR_ARGB8888:
	case E_GOP_COLOR_RGBA8888:
	case E_GOP_COLOR_BGRA8888:
	case E_GOP_COLOR_ABGR8888:
	case E_GOP_COLOR_A2RGB10:
	case E_GOP_COLOR_A2BGR10:
	case E_GOP_COLOR_RGB10A2:
	case E_GOP_COLOR_BGR10A2:
	case E_GOP_COLOR_RGB888:
		pitch = width << 2;
		break;
	default:
		//print err
		pitch = 0;
		break;
	}
	return pitch;
}

static MS_U16 _GOP_GetBPP(void *pInstance, EN_GOP_COLOR_TYPE fbFmt)
{
	MS_U16 bpp = 0;

	switch (fbFmt) {
	case E_GOP_COLOR_RGB565:
	case E_GOP_COLOR_BGR565:
	case E_GOP_COLOR_ARGB1555:
	case E_GOP_COLOR_ABGR1555:
	case E_GOP_COLOR_ARGB4444:
	case E_GOP_COLOR_RGBA4444:
	case E_GOP_COLOR_ABGR4444:
	case E_GOP_COLOR_BGRA4444:
	case E_GOP_COLOR_YUV422:
	case E_GOP_COLOR_RGBA5551:
	case E_GOP_COLOR_BGRA5551:
		bpp = 16;
		break;
	case E_GOP_COLOR_AYUV8888:
	case E_GOP_COLOR_ARGB8888:
	case E_GOP_COLOR_ABGR8888:
	case E_GOP_COLOR_RGBA8888:
	case E_GOP_COLOR_BGRA8888:
	case E_GOP_COLOR_RGB888:
		bpp = 32;
		break;
	default:
		//print err
		//__ASSERT(0);
		bpp = 0xFFFF;
		break;
	}
	return bpp;

}

static MS_U8 _GOP_GWIN_IsGwinCreated(void *pInstance, MS_U8 gId)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, gId)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, gId);
		return FALSE;
	}

	if ((gId < g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum)
	    && (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId == GWIN_ID_INVALID)) {
		return FALSE;
	}

	GOP_M_INFO("IsGwinCreated Debug: gId=%d\n", gId);
	return TRUE;
}

static MS_U8 _GOP_GWIN_IsGwinExistInClient(void *pInstance, MS_U8 gId)
{
	MS_BOOL result0, result1, result2, result3, result4;
	MS_U32 MaxGwinSupport, pid;

	GOP_WinFB_INFO *pwinFB;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, gId)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, gId);
		return FALSE;
	}

	MaxGwinSupport = g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum;

	pwinFB = _GetWinFB(pInstance, g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId);

	if (pwinFB == NULL)
		return FALSE;

	result0 = (MaxGwinSupport <= gId);
	result1 = (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].bIsShared == FALSE);
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
	pid = (GETPIDTYPE) getpid();
	result2 = (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32GOPClientId != pid);
#else
	result2 =
	    (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32GOPClientId !=
	     g_pGOPCtxLocal->u32GOPClientId);
#endif
	result3 =
	    (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId >= DRV_MAX_GWIN_FB_SUPPORT);
	result4 = (pwinFB->in_use == 0);

	if (result0 || (result1 && (result2 || result3 || result4))) {
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
		GOP_M_INFO("result0 = %d,result1 = %d,result2 = %d,result3 = %d,result4 = %d\n",
			   result0, result1, result2, result3, result4);
		GOP_M_INFO("[%s][%d]GWIN %d  is not exist\n", __func__, __LINE__, gId);
#else
		GOP_M_INFO("IsGwinExistInClient Debug: gId =%d\n", gId);
		GOP_M_INFO("gwinMap[gId].u32GOPClientId: gId =%td\n",
			   (ptrdiff_t)(g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32GOPClientId));
		GOP_M_INFO("_pGOPCtxLocal->u32GOPClientId =%td\n",
			   (ptrdiff_t) (g_pGOPCtxLocal->u32GOPClientId));
		GOP_M_INFO("gwinMap[gId].u32CurFBId =%td\n",
			   (ptrdiff_t) (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId));
		GOP_M_INFO("gwinMap[gId].u32CurFBId].in_use =%d\n", pwinFB->in_use);
#endif
		return FALSE;
	}
	return TRUE;

}



static void _GOP_GWIN_IsEnableMirror(void *pInstance, DRV_GOPDstType eGopDst, MS_BOOL *pbHMirror,
				     MS_BOOL *pbVMirror)
{

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if ((pbHMirror == NULL) || (pbVMirror == NULL))
		return;

	if ((eGopDst == E_DRV_GOP_DST_OP0)
	    || (eGopDst == E_DRV_GOP_DST_FRC)
	    || (eGopDst == E_DRV_GOP_DST_IP0)
	    ) {
		*pbHMirror = g_pGOPCtxLocal->pGOPCtxShared->bHMirror;
		*pbVMirror = g_pGOPCtxLocal->pGOPCtxShared->bVMirror;
	} else {
		*pbHMirror = FALSE;
		*pbVMirror = FALSE;
	}
}

static MS_BOOL _GetGOPEnum(void *pInstance)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	memset(&GOPTYPE, 0xff, sizeof(GOP_TYPE_DEF));
	return MDrv_GOP_GetGOPEnum(g_pGOPCtxLocal, &GOPTYPE);
}


static MS_BOOL _GOP_Init_Ctx(void *pInstance, MS_BOOL *pbFirstDrvInstant)
{
	MS_BOOL bNeedInitShared = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	_GetGOPEnum(pInstance);

	if (g_pGOPCtxLocal) {
		if (bFirstInit)
			*pbFirstDrvInstant = TRUE;
		else
			*pbFirstDrvInstant = FALSE;

		return TRUE;
	}
	g_pGOPCtxLocal = Drv_GOP_Init_Context(pInstance, &bNeedInitShared);
	if (g_pGOPCtxLocal == NULL) {
		GOP_M_ERR("Error : g_pGOPCtxLocal = NULL\n");
		return FALSE;
	}
	g_pGOPCtxLocal->pGOPCtxShared->bInitShared = bNeedInitShared;

	if (pbFirstDrvInstant)
		*pbFirstDrvInstant = bNeedInitShared;


#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
	g_pGOPCtxLocal->u32GOPClientId = (GETPIDTYPE) getpid();
#else
	g_pGOPCtxLocal->u32GOPClientId = ++g_pGOPCtxLocal->pGOPCtxShared->u32ClientIdAllocator;
	if (g_pGOPCtxLocal->u32GOPClientId == 0)
		g_pGOPCtxLocal->u32GOPClientId =
		    ++g_pGOPCtxLocal->pGOPCtxShared->u32ClientIdAllocator;
#endif

	memset(&g_pGOPCtxLocal->sGOPConfig, 0, sizeof(GOP_Config));

	memset(g_pGOPCtxLocal->MS_MuxGop, 0, MAX_GOP_MUX_SUPPORT * sizeof(MS_U8));
	g_pGOPCtxLocal->IsChgMux = FALSE;
	g_pGOPCtxLocal->IsClkClosed = FALSE;
	g_pGOPCtxLocal->u8ChgIpMuxGop = 0xFF;

	return TRUE;
}

static void _SetGop0WinInfo(void *pInstance, MS_U8 u8win, GOP_GwinInfo *pinfo)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	MDrv_GOP_GWIN_SetWinFmt(g_pGOPCtxLocal, u8win, (DRV_GOPColorType) pinfo->clrType);
	MDrv_GOP_GWIN_SetGwinInfo(g_pGOPCtxLocal, u8win, (DRV_GOP_GWIN_INFO *) pinfo);

	GOP_M_INFO("GWIN_SetWin(%d): [adr(B), RBsz, offset] = [%td, %d, %d]\n",
		   u8win,
		   (ptrdiff_t) pinfo->u32DRAMRBlkStart,
		   pinfo->u16RBlkHPixSize * pinfo->u16RBlkVPixSize / (64 /
								      _GOP_GetBPP(pInstance,
										  pinfo->clrType)),
		   (pinfo->u16WinY * pinfo->u16RBlkHPixSize +
		    pinfo->u16WinX) / (64 / _GOP_GetBPP(pInstance, pinfo->clrType)));
	GOP_M_INFO("\t[Vst, Vend, Hst, Hend, GwinHsz] = [%d, %d, %d, %d, %d]\n",
		   pinfo->u16DispVPixelStart, pinfo->u16DispVPixelEnd,
		   pinfo->u16DispHPixelStart / (64 / _GOP_GetBPP(pInstance, pinfo->clrType)),
		   pinfo->u16DispHPixelEnd / (64 / _GOP_GetBPP(pInstance, pinfo->clrType)),
		   pinfo->u16RBlkHPixSize / (64 / _GOP_GetBPP(pInstance, pinfo->clrType)));
}

static void _SetGop1WinInfo(void *pInstance, MS_U8 u8win, GOP_GwinInfo *pinfo)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	MDrv_GOP_GWIN_SetWinFmt(g_pGOPCtxLocal, u8win, (DRV_GOPColorType) pinfo->clrType);
	MDrv_GOP_GWIN_SetGwinInfo(g_pGOPCtxLocal, u8win, (DRV_GOP_GWIN_INFO *) pinfo);
}

static void _SetGop23WinInfo(void *pInstance, MS_U8 u8win, GOP_GwinInfo *pinfo)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	MDrv_GOP_GWIN_SetGwinInfo(g_pGOPCtxLocal, u8win, (DRV_GOP_GWIN_INFO *) pinfo);
	MDrv_GOP_GWIN_SetWinFmt(g_pGOPCtxLocal, u8win, (DRV_GOPColorType) pinfo->clrType);
}

static MS_U32 GOP_FB_Destroy(void *pInstance, MS_U32 u32fbId)
{
	GOP_WinFB_INFO *pwinFB;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	pwinFB = _GetWinFB(pInstance, u32fbId);

	if (pwinFB == NULL) {
		GOP_M_ERR("\n[%s] pwinFB is null!\n", __func__);
		return GOP_API_FAIL;
	}

	pwinFB->poolId = GOP_WINFB_POOL_NULL;
	pwinFB->enable = FALSE;
	pwinFB->in_use = 0;
	pwinFB->obtain = 0;
	pwinFB->x0 = 0;
	pwinFB->y0 = 0;
	pwinFB->x1 = 0;
	pwinFB->y1 = 0;

	pwinFB->gWinId = GWIN_ID_INVALID;
	pwinFB->width = 0;
	pwinFB->height = 0;
	pwinFB->pitch = 0;
	pwinFB->fbFmt = 0;
	pwinFB->addr = 0;
	pwinFB->size = 0;
	pwinFB->dispHeight = 0;
	pwinFB->dispWidth = 0;
	pwinFB->s_x = 0;
	pwinFB->s_y = 0;
	pwinFB->string = E_GOP_FB_NULL;
	pwinFB->u32GOPClientId = INVALID_CLIENT_PID;

	return GWIN_OK;
}

static MS_U32 GOP_SetWinInfo(void *pInstance, MS_U32 u32Gwin, GOP_GwinInfo *pinfo)
{
	MS_U8 u8GOP;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, u32Gwin)) {
		GOP_M_ERR("\n[%s] not support gwin id:%td in this chip version", __func__,
			  (ptrdiff_t) u32Gwin);
		return GOP_API_FAIL;
	}
	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, u32Gwin);


	switch (u8GOP) {
	case E_GOP0:
		_SetGop0WinInfo(pInstance, u32Gwin, pinfo);
		break;
	case E_GOP1:
		_SetGop1WinInfo(pInstance, u32Gwin, pinfo);
		break;
	case E_GOP2:
	case E_GOP3:
	case E_GOP4:
	case E_GOP5:
		_SetGop23WinInfo(pInstance, u32Gwin, pinfo);
		break;
	default:
		break;
	}

	return GOP_API_SUCCESS;
}

static MS_BOOL GOP_PreInit_Ctx(void *pInstance, MS_BOOL *pbFirstDrvInstant)
{
	MS_BOOL bNeedInitShared = FALSE;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	_GetGOPEnum(pInstance);

	if (g_pGOPCtxLocal) {
		if (bFirstInit)
			*pbFirstDrvInstant = TRUE;
		else
			*pbFirstDrvInstant = FALSE;

		return TRUE;
	}
	g_pGOPCtxLocal = Drv_GOP_Init_Context(pInstance, &bNeedInitShared);
	if (g_pGOPCtxLocal == NULL)
		return FALSE;
	if (pbFirstDrvInstant)
		*pbFirstDrvInstant = bNeedInitShared;

	g_pGOPCtxLocal->pGOPCtxShared->bDummyInit = bNeedInitShared;

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
	g_pGOPCtxLocal->u32GOPClientId = (GETPIDTYPE) getpid();
#else
	g_pGOPCtxLocal->u32GOPClientId = ++g_pGOPCtxLocal->pGOPCtxShared->u32ClientIdAllocator;
	if (g_pGOPCtxLocal->u32GOPClientId == 0)
		g_pGOPCtxLocal->u32GOPClientId =
		    ++g_pGOPCtxLocal->pGOPCtxShared->u32ClientIdAllocator;
#endif

	memset(&g_pGOPCtxLocal->sGOPConfig, 0, sizeof(GOP_Config));

	memset(g_pGOPCtxLocal->MS_MuxGop, 0, MAX_GOP_MUX_SUPPORT * sizeof(MS_U8));
	g_pGOPCtxLocal->IsChgMux = FALSE;
	g_pGOPCtxLocal->IsClkClosed = FALSE;
	g_pGOPCtxLocal->u8ChgIpMuxGop = 0xFF;

	return TRUE;
}


MS_U32 GOP_MapFB2Win(void *pInstance, MS_U32 u32fbId, MS_U32 GwinId)
{
	GOP_GwinInfo gWin;
	MS_U32 u32Temp;
	GOP_WinFB_INFO *pwinFB;
	GOP_WinFB_INFO *pwinFB2;

	MS_BOOL bAFBC_Enable = FALSE;
	MS_U8 u8GOP = 0;
	DRV_GOP_AFBC_Info sAFBCWinProperty;
	MS_U8 u8AFBC_cmd = 0;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	memset(&sAFBCWinProperty, 0, sizeof(DRV_GOP_AFBC_Info));
	if (u32fbId >= DRV_MAX_GWIN_FB_SUPPORT) {
		GOP_M_ERR("[%s][%d]: fbId max already reached! FBId: %td, winId: %td\n",
			  __func__, __LINE__, (ptrdiff_t) u32fbId, (ptrdiff_t) GwinId);
		return GOP_API_FAIL;
	}

	memset(&gWin, 0, sizeof(GOP_GwinInfo));
	pwinFB = _GetWinFB(pInstance, u32fbId);

	if (pwinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GOP_API_FAIL;
	}

	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[GwinId].u32CurFBId = u32fbId;

	pwinFB->gWinId = GwinId;

	gWin.u16DispHPixelStart = pwinFB->x0;
	gWin.u16DispVPixelStart = pwinFB->y0;
	gWin.u16RBlkHPixSize = pwinFB->width;
	gWin.u16RBlkVPixSize = pwinFB->height;
	u32Temp = _GOP_CalcPitch(pInstance, pwinFB->fbFmt, pwinFB->width);
	if (pwinFB->pitch < u32Temp)
		gWin.u16RBlkHRblkSize = (MS_U16) u32Temp;
	else
		gWin.u16RBlkHRblkSize = pwinFB->pitch;

	gWin.u16DispHPixelEnd = pwinFB->x1;
	gWin.u16DispVPixelEnd = pwinFB->y1;
	gWin.u32DRAMRBlkStart = pwinFB->addr;
	gWin.u16WinX = gWin.u16WinY = 0;
	gWin.clrType = (EN_GOP_COLOR_TYPE) pwinFB->fbFmt;

	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, GwinId);

	if (g_pGOPCtxLocal->pGopChipProperty->bAFBC_Support[u8GOP] == TRUE) {
		_GOP_MAP_FBID_AFBCCmd(pInstance, u32fbId, &u8AFBC_cmd, &bAFBC_Enable);

		sAFBCWinProperty.u16HPixelStart = gWin.u16DispHPixelStart;
		sAFBCWinProperty.u16HPixelEnd = gWin.u16DispHPixelEnd;
		sAFBCWinProperty.u16Pitch = gWin.u16RBlkHRblkSize;
		sAFBCWinProperty.u16VPixelStart = gWin.u16DispVPixelStart;
		sAFBCWinProperty.u16VPixelEnd = gWin.u16DispVPixelEnd;
		sAFBCWinProperty.u64DRAMAddr = gWin.u32DRAMRBlkStart;
		sAFBCWinProperty.u8Fmt = u8AFBC_cmd;
		sAFBCWinProperty.bEnable = bAFBC_Enable;
		MDrv_GOP_GWIN_AFBCSetWindow(g_pGOPCtxLocal, u8GOP, &sAFBCWinProperty, TRUE);
	}

	return GOP_API_SUCCESS;
}


MS_U32 GOP_Set_Hscale(void *pInstance, MS_U8 u8GOP, MS_BOOL bEnable, MS_U16 src, MS_U16 dst)
{

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!g_pGOPCtxLocal->pGopChipProperty->bPixelModeSupport) {
		if (src > dst) {
			GOP_M_ERR("[%s][%d]GOP can not scaling down\n", __func__, __LINE__);
			return GOP_API_FAIL;
		}
	}

	if (bEnable) {
		g_pGOPCtxLocal->pGOPCtxShared->u16HScaleSrc[u8GOP] = src;
		g_pGOPCtxLocal->pGOPCtxShared->u16HScaleDst[u8GOP] = dst;
	} else {
		g_pGOPCtxLocal->pGOPCtxShared->u16HScaleSrc[u8GOP] = src;
		g_pGOPCtxLocal->pGOPCtxShared->u16HScaleDst[u8GOP] = 0;
	}
	MDrv_GOP_GWIN_Set_HSCALE(g_pGOPCtxLocal, u8GOP, bEnable, src, dst);

	return GOP_API_SUCCESS;
}

MS_U32 GOP_Set_Vscale(void *pInstance, MS_U8 u8GOP, MS_BOOL bEnable, MS_U16 src, MS_U16 dst)
{

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!g_pGOPCtxLocal->pGopChipProperty->bPixelModeSupport) {
		if (src > dst) {
			GOP_M_ERR("[%s][%d]GOP can not scaling down\n", __func__, __LINE__);
			return GOP_API_FAIL;
		}
	}

	if (bEnable)
		g_pGOPCtxLocal->pGOPCtxShared->u16VScaleDst[u8GOP] = dst;
	else
		g_pGOPCtxLocal->pGOPCtxShared->u16VScaleDst[u8GOP] = 0;

	MDrv_GOP_GWIN_Set_VSCALE(g_pGOPCtxLocal, u8GOP, bEnable, src, dst);

	return GOP_API_SUCCESS;

}

MS_U32 GOP_SetAlphaInverse(void *pInstance, MS_U32 gop, MS_BOOL bEn)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	MDrv_GOP_GWIN_SetAlphaInverse(g_pGOPCtxLocal, gop, bEn);
	return GOP_API_SUCCESS;
}

MS_U32 GOP_CreateBufferByAddr(void *pInstance, PGOP_BUFFER_INFO pBuff, MS_U32 u32fbId)
{
	MS_U32 u32Size = 0;
	GOP_WinFB_INFO *pstWinFB = NULL;
	MS_U32 u32Width = 0;
	MS_U32 u32Height = 0;
	MS_U32 u32Pitch = 0;
	MS_U32 u32FbFmt = 0;
	MS_PHY phyFbAddr = 0;
	MS_U32 u32DispX, u32DispY = 0;
	EN_GOP_FRAMEBUFFER_STRING enFBString = E_GOP_FB_NULL;

	if (!_GOP_IsFbIdValid(pInstance, u32fbId)) {
		GOP_M_ERR("[%s][%d]FbId %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GWIN_NO_AVAILABLE;
	}

	pstWinFB = _GetWinFB(pInstance, u32fbId);

	if (pstWinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GWIN_NO_AVAILABLE;
	}

	if (pstWinFB->in_use) {
		GOP_M_ERR("[%s][%d]FbId %td is already allocated\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GWIN_NO_AVAILABLE;

	}

	GOP_M_INFO("[%s][%d]: new gwinFBId = %td\n", __func__, __LINE__, (ptrdiff_t) u32fbId);


	u32Width = pBuff->width;
	u32Height = pBuff->height;
	u32Pitch = pBuff->pitch;
	u32FbFmt = pBuff->fbFmt;
	u32DispX = pBuff->disp_rect.x;
	u32DispY = pBuff->disp_rect.y;
	phyFbAddr = pBuff->addr;
	enFBString = pBuff->FBString;

	u32Size = _GOP_CalcPitch(pInstance, u32FbFmt, u32Width) * u32Height;

	pstWinFB->enable = FALSE;
	pstWinFB->in_use = 1;
	pstWinFB->obtain = 1;
	pstWinFB->x0 = u32DispX;
	pstWinFB->y0 = u32DispY;
	pstWinFB->width = u32Width;
	pstWinFB->height = u32Height;
	pstWinFB->x1 = pstWinFB->x0 + u32Width;
	pstWinFB->y1 = pstWinFB->y0 + u32Height;
	pstWinFB->addr = phyFbAddr;	//addr;
	pstWinFB->size = u32Size;
	pstWinFB->FBString = (EN_DRV_FRAMEBUFFER_STRING) enFBString;

	// FB format
	pstWinFB->fbFmt = (u32FbFmt != FB_FMT_AS_DEFAULT) ? u32FbFmt : E_GOP_COLOR_ARGB1555;
	if (u32Pitch == 0)
		pstWinFB->pitch = _GOP_CalcPitch(pInstance, u32FbFmt, u32Width);
	else
		pstWinFB->pitch = u32Pitch;

	// update ptrs
	pstWinFB->poolId = GOP_WINFB_POOL_NULL;
	pstWinFB->nextFBIdInPool = INVALID_POOL_NEXT_FBID;
	pstWinFB->string = E_GOP_FB_NULL;
	pstWinFB->u32GOPClientId = (GETPIDTYPE) getpid();
	GOP_M_INFO("\33[0;36m   %s:%d   FBId = %td \33[m\n", __func__, __LINE__,
		   (ptrdiff_t) u32fbId);

	return GOP_API_SUCCESS;
}

static void GOP_GWIN_InitByGOP(void *pInstance, MS_BOOL bFirstIntance, MS_U8 u8GOP)
{
	MS_U8 u8Index = 0;
	MS_U8 u8BytePerPix = 2;
	MS_U16 u16Width = 0, u16Height = 0, u16Pitch = 0, u16FbFmt = 0;
	MS_PHY phyAddr = 0;
	MS_U32 u32Index = 0;
	GOP_GwinInfo stGwin;
	GOP_WinFB_INFO *pstWinFB = NULL;
	MS_U32 u32MaxGwinSupport = 0;
	MS_BOOL bGwinEnable = FALSE;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	//GE_BufInfo buffinfo;
	if (!_GOP_IsGopNumValid(pInstance, u8GOP)) {
		GOP_M_ERR("[%s][%d]GOP %d  is out of range\n", __func__, __LINE__, u8GOP);
		return;
	}
	GOP_M_INFO("MDrv_GOP_GWIN_Init\n");
	memset(&stGwin, 0, sizeof(GOP_GwinInfo));
	if (g_pGOPCtxLocal->pGOPCtxShared->bInitShared) {
		g_pGOPCtxLocal->pGOPCtxShared->bInitShared = FALSE;
		//First init of whole GOP Driver
		for (u32Index = 0; u32Index < DRV_MAX_GWIN_FB_SUPPORT; u32Index++) {
			pstWinFB = _GetWinFB(pInstance, u32Index);

			if (pstWinFB == NULL) {
				GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__,
					  __LINE__, (ptrdiff_t) u32Index);
				return;
			}

			pstWinFB->enable = FALSE;
			pstWinFB->in_use = 0;
			pstWinFB->obtain = 0;
			pstWinFB->gWinId = GWIN_ID_INVALID;	// orig GWIN_OSD_DEFAULT;
			pstWinFB->x0 = stGwin.u16DispHPixelStart = 0;
			pstWinFB->y0 = stGwin.u16DispVPixelStart = 0;
			pstWinFB->width = stGwin.u16RBlkHPixSize = 0;
			pstWinFB->height = stGwin.u16RBlkVPixSize = 0;
			pstWinFB->x1 = stGwin.u16DispHPixelEnd = 0;
			pstWinFB->y1 = stGwin.u16DispVPixelEnd = 0;
			pstWinFB->pitch = 0;
			pstWinFB->addr = stGwin.u32DRAMRBlkStart = 0;
			pstWinFB->size = 0;
			stGwin.u16WinX = stGwin.u16WinY = 0;
			// FB format
			pstWinFB->fbFmt = FB_FMT_AS_DEFAULT;
			// update ptrs
			pstWinFB->poolId = GOP_WINFB_POOL_NULL;
			pstWinFB->nextFBIdInPool = INVALID_POOL_NEXT_FBID;
			pstWinFB->string = E_GOP_FB_NULL;
			pstWinFB->u32GOPClientId = INVALID_CLIENT_PID;
		}
	}
	if (g_pGOPCtxLocal->pGOPCtxShared->bGopHasInitialized[u8GOP] == FALSE) {
		memset(&stGwin, 0, sizeof(GOP_GwinInfo));
		for (u8Index = 0; u8Index < MDrv_GOP_GetGwinNum(g_pGOPCtxLocal, u8GOP); u8Index++) {
			stGwin.u32DRAMRBlkStart = 0;
			u32Index = _GOP_SelGwinId2(pInstance, u8GOP, u8Index);
			g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u32Index].u32CurFBId =
			    GWIN_ID_INVALID;
			stGwin.clrType = (EN_GOP_COLOR_TYPE) E_GOP_COLOR_ARGB1555;
			//Disable all Gwin when GOP first init
			if (!(g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit & E_GOP_IGNORE_GWIN)) {
				MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, u32Index, FALSE);
				GOP_SetWinInfo(pInstance, u32Index, &stGwin);
				MDrv_GOP_GWIN_SetBlending(g_pGOPCtxLocal, u32Index, TRUE,
					GOP_CONST_ALPHA_DEF);
			}
		}
	}
	pstWinFB = _GetWinFB(pInstance, DRV_MAX_GWIN_FB_SUPPORT);
	u32MaxGwinSupport = g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum;

	if (pstWinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%ld\n", __func__, __LINE__,
			  DRV_MAX_GWIN_FB_SUPPORT);
		return;
	}

	pstWinFB->enable = TRUE;
	pstWinFB->in_use = 1;
	pstWinFB->obtain = 1;
	pstWinFB->gWinId = u32MaxGwinSupport;
	pstWinFB->x0 = 0;
	pstWinFB->y0 = 0;
	pstWinFB->width = u16Width;
	pstWinFB->height = u16Height;
	pstWinFB->x1 = pstWinFB->x0 + pstWinFB->width;
	pstWinFB->y1 = pstWinFB->y0 + pstWinFB->height;
	pstWinFB->pitch = (u16Pitch & 0xFFF8);	// pitch must be 4-pix alignment;
	pstWinFB->addr = phyAddr;
	pstWinFB->size = ((MS_U32) u16Width) * ((MS_U32) u16Height) * ((MS_U32) u8BytePerPix);
	// FB format
	pstWinFB->fbFmt = u16FbFmt;
	// update ptrs
	pstWinFB->poolId = GOP_WINFB_POOL_NULL;
	pstWinFB->nextFBIdInPool = INVALID_POOL_NEXT_FBID;
	pstWinFB->string = E_GOP_FB_NULL;
	pstWinFB->u32GOPClientId = INVALID_CLIENT_PID;

	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u32MaxGwinSupport].u32CurFBId =
	    DRV_MAX_GWIN_FB_SUPPORT;

}

MS_U32 GOP_GWIN_SetHMirror(void *pInstance, MS_BOOL bEnable)
{
	MS_U8 u8GOPIdx = 0;
	MS_BOOL bHMirror = FALSE, bVMirror = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	g_pGOPCtxLocal->pGOPCtxShared->bHMirror = bEnable;

	for (u8GOPIdx = 0; u8GOPIdx < MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal); u8GOPIdx++) {
		MDrv_GOP_IsGOPMirrorEnable(g_pGOPCtxLocal, u8GOPIdx, &bHMirror, &bVMirror);
		if (bHMirror == bEnable)
			continue;
		MDrv_GOP_GWIN_EnableHMirror(g_pGOPCtxLocal, u8GOPIdx, bEnable);
	}

	return GOP_API_SUCCESS;
}


MS_U32 GOP_GWIN_SetVMirror(void *pInstance, MS_BOOL bEnable)
{
	MS_U8 u8GOPIdx = 0;
	MS_BOOL bHMirror = FALSE, bVMirror = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	g_pGOPCtxLocal->pGOPCtxShared->bVMirror = bEnable;

	for (u8GOPIdx = 0; u8GOPIdx < MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal); u8GOPIdx++) {
		MDrv_GOP_IsGOPMirrorEnable(g_pGOPCtxLocal, u8GOPIdx, &bHMirror, &bVMirror);
		if (bVMirror == bEnable)
			continue;
		//Enable mirror
		MDrv_GOP_GWIN_EnableVMirror(g_pGOPCtxLocal, u8GOPIdx, bEnable);
	}

	return GOP_API_SUCCESS;

}

MS_U32 GOP_GWIN_IsEnable(void *pInstance, MS_U8 winId)
{
	MS_BOOL IsEnable = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, winId)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, winId);
		return FALSE;
	}
	MDrv_GOP_GWIN_IsGWINEnabled(g_pGOPCtxLocal, winId, &IsEnable);
	return IsEnable;
}

MS_U32 GOP_GWIN_SetEnable(void *pInstance, MS_U8 winId, MS_BOOL bEnable)
{
	MS_U16 regval2 = 0, i;
	MS_U32 u32fbId;
	GOP_WinFB_INFO *pwinFB;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	if (!_GOP_IsGwinIdValid(pInstance, winId)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, winId);
		return GOP_API_FAIL;
	}


	if (!_GOP_GWIN_IsGwinExistInClient(pInstance, winId)) {
		//GOP_M_ERR("[%s][%d]GWIN %d  is not exist\n",__func__,__LINE__,winId);
		//__ASSERT(0);
		return GOP_API_FAIL;
	}


	u32fbId = g_pGOPCtxLocal->pGOPCtxShared->gwinMap[winId].u32CurFBId;
	pwinFB = _GetWinFB(pInstance, u32fbId);

	if (pwinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%d\n", __func__, __LINE__,
			  (unsigned int)u32fbId);
		goto fail;
	}

	if (!_GOP_IsFbIdValid(pInstance, u32fbId)) {
		GOP_M_ERR("[%s][%d]FbId %d  is out of range\n", __func__, __LINE__,
			  (unsigned int)u32fbId);
		goto fail;
	}

	if (pwinFB->in_use == 0) {
		GOP_M_ERR("[%s][%d]GWIN %d not allocated\n", __func__, __LINE__, winId);
		goto fail;
	}
	pwinFB->enable = bEnable;

	for (i = 0; i < g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum; i++) {
		if (GOP_GWIN_IsEnable(pInstance, i) == TRUE)
			regval2 |= 1 << i;
	}

	if (bEnable == 0)
		MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, winId, FALSE);
	else
		MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, winId, TRUE);

	return GOP_API_SUCCESS;

fail:
	if (bEnable == 0)
		MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, winId, FALSE);

	return GOP_API_FAIL;
}

MS_U32 GOP_GWIN_SetBlending(void *pInstance, MS_U8 u8win, MS_BOOL bEnable, MS_U8 u8coef)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, u8win)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, u8win);
		return GOP_API_FAIL;
	}
	MDrv_GOP_GWIN_SetBlending(g_pGOPCtxLocal, u8win, bEnable, u8coef);
	return GOP_API_SUCCESS;

}

MS_U32 GOP_GWIN_Set_VStretchMode(void *pInstance, MS_U8 u8GOP, EN_GOP_STRETCH_VMODE VStrchMode)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (VStrchMode) {
	case E_GOP_VSTRCH_LINEAR:
	case E_GOP_VSTRCH_LINEAR_GAIN0:
	case E_GOP_VSTRCH_LINEAR_GAIN1:
	case E_GOP_VSTRCH_DUPLICATE:
	case E_GOP_VSTRCH_NEAREST:
	case E_GOP_VSTRCH_LINEAR_GAIN2:
	case E_GOP_VSTRCH_4TAP_100:
	case E_GOP_VSTRCH_4TAP:
	default:
		MDrv_GOP_GWIN_Set_VStretchMode(g_pGOPCtxLocal, u8GOP,
				E_DRV_GOP_VSTRCH_4TAP);
		break;
	}
	return GOP_API_SUCCESS;
}

MS_U32 GOP_GWIN_Set_HStretchMode(void *pInstance, MS_U8 u8GOP, EN_GOP_STRETCH_HMODE HStrchMode)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (HStrchMode) {
	case E_GOP_HSTRCH_DUPLICATE:
	case E_GOP_HSTRCH_6TAPE:
	case E_GOP_HSTRCH_6TAPE_LINEAR:
	case E_GOP_HSTRCH_6TAPE_NEAREST:
	case E_GOP_HSTRCH_6TAPE_GAIN0:
	case E_GOP_HSTRCH_6TAPE_GAIN1:
	case E_GOP_HSTRCH_6TAPE_GAIN2:
	case E_GOP_HSTRCH_6TAPE_GAIN3:
	case E_GOP_HSTRCH_6TAPE_GAIN4:
	case E_GOP_HSTRCH_6TAPE_GAIN5:
	case E_GOP_HSTRCH_2TAPE:
	case E_GOP_HSTRCH_4TAPE:
	case E_GOP_HSTRCH_NEW4TAP:
	case E_GOP_HSTRCH_NEW4TAP_50:
	case E_GOP_HSTRCH_NEW4TAP_55:
	case E_GOP_HSTRCH_NEW4TAP_65:
	case E_GOP_HSTRCH_NEW4TAP_75:
	case E_GOP_HSTRCH_NEW4TAP_85:
	case E_GOP_HSTRCH_NEW4TAP_95:
	case E_GOP_HSTRCH_NEW4TAP_105:
	case E_GOP_HSTRCH_NEW4TAP_100:
	default:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		break;
	}

	return GOP_API_SUCCESS;
}

MS_U32 GOP_GWIN_SetGWinShared(void *pInstance, MS_U8 winId, MS_BOOL bIsShared)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, winId)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, winId);
		return GOP_API_FAIL;
	}

	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[winId].bIsShared = true;
	return GOP_API_SUCCESS;
}

MS_U32 GOP_GWIN_SetGWinSharedCnt(void *pInstance, MS_U8 winId, MS_U16 u16SharedCnt)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, winId)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, winId);
		return GOP_API_FAIL;
	}

	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[winId].u16SharedCnt = u16SharedCnt;
	return GOP_API_SUCCESS;
}

MS_U32 GOP_GWIN_Is32FBExist(void *pInstance, MS_U32 u32fbId)
{
	GOP_WinFB_INFO *pwinFB;

	if (!_GOP_IsFbIdValid(pInstance, u32fbId)) {
		GOP_M_ERR("[%s][%d]FbId %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return FALSE;
	}

	pwinFB = _GetWinFB(pInstance, u32fbId);

	if (pwinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return FALSE;
	}

	return pwinFB->obtain;
}

MS_U32 GOP_GWIN_GetFreeFbID(void *pInstance)
{
	MS_U32 i;
	MS_U32 u32FBId = 0xFFFFFFFF;
	GOP_WinFB_INFO *pwinFB;

	for (i = 0; i < DRV_MAX_GWIN_FB_SUPPORT; i++) {
		if (!GOP_GWIN_Is32FBExist(pInstance, i)) {
			u32FBId = i;
			pwinFB = _GetWinFB(pInstance, u32FBId);
			pwinFB->obtain = 1;
			break;
		}
	}
	return u32FBId;
}

MS_U32 GOP_GWIN_SetForceWrite(void *pInstance, MS_BOOL bEnable)
{
	//It's used for supernova special used. (by Jupiter)
	//There should no driver request before init.
	//Supernova wants to easly control AP flow, it will set gop force write before gop init.
	//Driver must recode the FRWR status and set it during gop init.
	MS_U32 u32Ret = GOP_API_SUCCESS;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (g_pGOPCtxLocal == NULL) {
		u32Ret = GOP_API_SUCCESS;
	} else {
		g_pGOPCtxLocal->bInitFWR = bEnable;
		MDrv_GOP_GWIN_SetForceWrite(g_pGOPCtxLocal, bEnable);
		u32Ret = GOP_API_SUCCESS;
	}

	return u32Ret;
}

//==================================================================
//////////////////////////////////////////////////
//  - GOP ioctl function.
//////////////////////////////////////////////////

MS_U32 Ioctl_GOP_Init(void *pInstance, MS_U8 u8GOP, GOP_InitInfo *pGopInit)
{
	MS_BOOL bFirstInstance = FALSE;
	MS_BOOL resCtxInit;
	DRV_GOPDstType GopDst;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	//NULL pointer protect
	if (pGopInit == NULL) {
		GOP_M_INFO("[%s][%d] input parameter is NULL\n", __func__, __LINE__);
		resCtxInit = _GOP_Init_Ctx(pInstance, &bFirstInstance);
		APIGOP_ASSERT(resCtxInit, GOP_M_FATAL("failed to init GOP context\n"));
		if (bFirstInstance)
			g_pGOPCtxLocal->pGOPCtxShared->bDummyInit = TRUE;
		if (bMMIOInit == FALSE) {
			if (MDrv_GOP_SetIOMapBase(g_pGOPCtxLocal) != TRUE) {
				APIGOP_ASSERT(FALSE, GOP_M_FATAL("\nget IO base fail"));
				return GOP_API_FAIL;
			}
			bMMIOInit = TRUE;
		}
	} else {
		resCtxInit = _GOP_Init_Ctx(pInstance, &bFirstInstance);
		APIGOP_ASSERT(resCtxInit, GOP_M_FATAL("failed to init GOP context\n"));
		bFirstInit = FALSE;
		if (g_pGOPCtxLocal->pGOPCtxShared->bDummyInit) {
			//other GOP init should work as the first instance flow
			bFirstInstance = TRUE;
			g_pGOPCtxLocal->pGOPCtxShared->bInitShared = TRUE;
			g_pGOPCtxLocal->pGOPCtxShared->bDummyInit = FALSE;
		}

		if (!_GOP_IsGopNumValid(pInstance, u8GOP)) {
			GOP_M_ERR("[%s][%d]GOP %d  is out of range\n", __func__, __LINE__,
				  u8GOP);
			return GOP_API_FAIL;
		}
		//init GOP global variable
		if (g_pGOPCtxLocal->pGOPCtxShared->bInitShared) {
			g_pGOPCtxLocal->current_winId = 0;
		}

		g_pGOPCtxLocal->pGOPCtxShared->u16PnlWidth[u8GOP] = pGopInit->u16PanelWidth;
		g_pGOPCtxLocal->pGOPCtxShared->u16PnlHeight[u8GOP] = pGopInit->u16PanelHeight;
		g_pGOPCtxLocal->pGOPCtxShared->u16PnlHStr[u8GOP] = pGopInit->u16PanelHStr;
		g_pGOPCtxLocal->pGOPCtxShared->u32ML_Fd[u8GOP] = pGopInit->u32ML_Fd;

		if (!
		    (g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit &
		     (DRV_GOP_IGNOREINIT) E_DRV_GOP_SWRST_IGNORE)) {
			MDrv_GOP_CtrlRst(g_pGOPCtxLocal, u8GOP, TRUE);
		}

		if (g_pGOPCtxLocal->pGOPCtxShared->bGopHasInitialized[u8GOP] == FALSE) {

			if (g_pGOPCtxLocal->pGopChipProperty->bPixelModeSupport)
				g_pGOPCtxLocal->pGOPCtxShared->bPixelMode[u8GOP] = TRUE;

			//Not initialized yet:
			GOP_GWIN_InitByGOP(pInstance, bFirstInstance, u8GOP);

			if (!(g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit & E_GOP_IGNORE_GWIN))
				MDrv_GOP_Init(g_pGOPCtxLocal, u8GOP);

			if (!
			    (g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit &
				E_GOP_IGNORE_ENABLE_TRANSCLR))
				MDrv_GOP_GWIN_EnableTransClr(g_pGOPCtxLocal, u8GOP,
							     (GOP_TransClrFmt) 0, FALSE);

			MDrv_GOP_SetGOPEnable2SC(g_pGOPCtxLocal, u8GOP, TRUE);
			if (!(g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit & E_GOP_IGNORE_GWIN))
				g_pGOPCtxLocal->pGOPCtxShared->bGopHasInitialized[u8GOP] = TRUE;
		}

		if (!
		    (g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit &
		     (DRV_GOP_IGNOREINIT) E_DRV_GOP_SWRST_IGNORE)) {
			MDrv_GOP_CtrlRst(g_pGOPCtxLocal, u8GOP, FALSE);
		}
	}

	return GOP_API_SUCCESS;
}


MS_U32 Ioctl_GOP_GetCaps(void *pInstance, EN_GOP_CAPS eCapType, MS_U32 *pRet, MS_U32 ret_size)
{
	E_GOP_API_Result enRet = GOP_API_SUCCESS;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (eCapType) {
	case E_GOP_CAP_AFBC_SUPPORT:
		{
			if (pRet != NULL) {
				MS_U8 i;
				PGOP_CAP_AFBC_INFO pGOPAFBCInfo = (PGOP_CAP_AFBC_INFO) pRet;
				MS_U8 u8MaxGOPNum = MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal);

				if (ret_size != sizeof(GOP_CAP_AFBC_INFO)) {
					GOP_M_ERR
					    ("[%s] ERROR, invalid structure size :%td\n",
					     __func__, (ptrdiff_t) ret_size);
					return GOP_API_FAIL;
				}
				pGOPAFBCInfo->GOP_AFBC_Support = 0;
				for (i = 0; i < u8MaxGOPNum; i++) {
					if (g_pGOPCtxLocal->pGopChipProperty->bAFBC_Support[i])
						pGOPAFBCInfo->GOP_AFBC_Support |= BIT(i);
					else
						pGOPAFBCInfo->GOP_AFBC_Support &= ~(BIT(i));
				}
			} else {
				APIGOP_ASSERT(0,
					      GOP_M_ERR("Invalid input parameter: NULL pointer\n"));
			}
			break;
		}
	default:
		GOP_M_ERR("[%s]not support GOP capability case: %d\n", __func__, eCapType);
		enRet = GOP_API_FAIL;
		break;
	}

	return enRet;
}


MS_U32 Ioctl_GOP_SetConfig(void *pInstance, EN_GOP_CONFIG_TYPE cfg_type, MS_U32 *pCfg,
			   MS_U32 u32Size)
{
	MS_BOOL bFirstInstance;
	MS_BOOL resCtxInit;
	EN_GOP_IGNOREINIT *pGOPIgnoreInit;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (cfg_type) {
	case E_GOP_IGNOREINIT:
		resCtxInit = GOP_PreInit_Ctx(pInstance, &bFirstInstance);
		APIGOP_ASSERT(resCtxInit, GOP_M_FATAL("failed to init GOP context\n"));

		pGOPIgnoreInit = (EN_GOP_IGNOREINIT *) (pCfg);
		g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit = (DRV_GOP_IGNOREINIT) (*pGOPIgnoreInit);

		if (*pGOPIgnoreInit != E_GOP_IGNORE_DISABLE)
			bFirstInit = TRUE;

		break;
	default:

		break;
	}

	return GOP_API_SUCCESS;
}

MS_U32 Ioctl_GOP_SetProperty(void *pInstance, EN_GOP_PROPERTY type, MS_U32 gop, MS_U32 *pSet,
			     MS_U32 u32Size)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (type) {
	case E_GOP_ALPHAINVERSE:
		{
			MS_BOOL bEn;

			if (u32Size != sizeof(MS_BOOL)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			bEn = *((MS_BOOL *) pSet);
			GOP_SetAlphaInverse(pInstance, gop, bEn);
			break;
		}
	case E_GOP_PREALPHAMODE:
		{
			MS_BOOL bEn;

			if (u32Size != sizeof(MS_BOOL)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			bEn = *((MS_BOOL *) pSet);

			MDrv_GOP_SetGOPEnable2Mode1(g_pGOPCtxLocal, gop, bEn);

			break;
		}
	case E_GOP_FORCE_WRITE:
		{
			MS_BOOL bEn;

			if (u32Size != sizeof(MS_BOOL)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			bEn = *((MS_BOOL *) pSet);
			GOP_GWIN_SetForceWrite(pInstance, bEn);
			break;
		}
	default:
			break;

	}

	return GOP_API_SUCCESS;

}

MS_U32 Ioctl_GOP_SetDst(void *pInstance, MS_U8 u8GOP, EN_GOP_DST_TYPE dsttype)
{
	DRV_GOPDstType GopDst;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if ((!_GOP_IsGopNumValid(pInstance, u8GOP))) {
		GOP_M_ERR("[%s][%d]GOP %d  is out of range\n", __func__, __LINE__, u8GOP);
		return GOP_API_FAIL;
	}

	/*Set GOP dst and check input dst type is valid or not for different chip first */
	(_GOP_Map_APIDst2DRV_Enum(pInstance, dsttype, &GopDst));
	(MDrv_GOP_GWIN_SetDstPlane(g_pGOPCtxLocal, u8GOP, GopDst));

	return GOP_API_SUCCESS;
}

MS_U32 Ioctl_GOP_SetMirror(void *pInstance, MS_U32 gop, EN_GOP_MIRROR_TYPE mirror)
{
	// To do: to modify and control each gop mirror setting.
	switch (mirror) {
	default:
			break;
	case E_GOP_MIRROR_H_ONLY:
		{
			GOP_GWIN_SetHMirror(pInstance, TRUE);
			//GOP_GWIN_SetVMirror(pInstance,FALSE);
			break;
		}
	case E_GOP_MIRROR_H_NONE:
		{
			GOP_GWIN_SetHMirror(pInstance, FALSE);
			break;
		}
	case E_GOP_MIRROR_V_ONLY:
		{
			//GOP_GWIN_SetHMirror(pInstance,FALSE);
			GOP_GWIN_SetVMirror(pInstance, TRUE);
			break;
		}
	case E_GOP_MIRROR_V_NONE:
		{
			GOP_GWIN_SetVMirror(pInstance, FALSE);
			break;
		}
	}

	return GOP_API_SUCCESS;
}

MS_U32 Ioctl_GOP_GetStatus(void *pInstance, EN_GOP_STATUS type, MS_U32 *pStatus, MS_U32 u32Size)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (pStatus == NULL) {
		GOP_M_ERR("[%s][%d] ERROR - Input null pointer on type:%d\n", __func__,
			  __LINE__, type);
		return GOP_API_INVALID_PARAMETERS;
	}

	switch (type) {
	case E_GOP_STATUS_GOP_MAXNUM:
		{
			MS_U8 *pMaxGopNum = (MS_U8 *) pStatus;
			*pMaxGopNum = MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal);
			break;
		}
	case E_GOP_STATUS_GWINIDX:
		{
			PGOP_GWIN_NUM ptr = (PGOP_GWIN_NUM) pStatus;

			ptr->gwin_num =
			    MDrv_GOP_Get_GwinIdByGOP(g_pGOPCtxLocal, (MS_U8) ptr->gop_idx);
			break;
		}
	default:
		{
			GOP_M_ERR("[%s] invalid input case:%d\n", __func__, type);
		}

	}

	return GOP_API_SUCCESS;

}

MS_U32 Ioctl_GOP_Set_Stretch(void *pInstance, EN_GOP_STRETCH_TYPE enStretchType, MS_U32 gop,
			     PGOP_STRETCH_INFO pStretchInfo)
{
	MS_BOOL bEn = FALSE;
	E_GOP_API_Result enRet = GOP_API_SUCCESS;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (enStretchType & E_GOP_STRETCH_HSTRETCH_MODE) {
		enRet =
		    (E_GOP_API_Result) GOP_GWIN_Set_HStretchMode(pInstance, gop,
								 pStretchInfo->enHMode);
	}

	if (enStretchType & E_GOP_STRETCH_VSTRETCH_MODE) {
		enRet =
		    (E_GOP_API_Result) GOP_GWIN_Set_VStretchMode(pInstance, gop,
								 pStretchInfo->enVMode);
	}

	return enRet;
}

MS_U32 Ioctl_GOP_GWin_SetProperty(void *pInstance, EN_GOP_GWIN_PROPERTY type, MS_U32 gwin,
				  MS_U32 *pSet, MS_U32 u32Size)
{
	switch (type) {
	case E_GOP_GWIN_BLENDING:
		{
			PGOP_GWIN_BLENDING pInfo = (PGOP_GWIN_BLENDING) pSet;

			if (u32Size != sizeof(GOP_GWIN_BLENDING))
				return GOP_API_INVALID_PARAMETERS;

			GOP_GWIN_SetBlending(pInstance, (MS_U8) gwin, pInfo->bEn,
					     (MS_U8) pInfo->Coef);

			break;
		}
	case E_GOP_GWIN_ENABLE:
		{
			MS_BOOL bEn;

			if (u32Size != sizeof(MS_BOOL))
				return GOP_API_INVALID_PARAMETERS;

			bEn = *((MS_BOOL *) pSet);

			GOP_GWIN_SetEnable(pInstance, gwin, bEn);

			break;
		}
	case E_GOP_GWIN_SHARE:
		{
			MS_BOOL bEn;

			if (u32Size != sizeof(MS_BOOL))
				return GOP_API_INVALID_PARAMETERS;
			bEn = *((MS_BOOL *) pSet);
			GOP_GWIN_SetGWinShared(pInstance, (MS_U8) gwin, bEn);
			break;
		}
	case E_GOP_GWIN_SHARE_CNT:
		{
			MS_U16 u16SharedCnt;

			if (u32Size != sizeof(MS_U16))
				return GOP_API_INVALID_PARAMETERS;
			u16SharedCnt = *((MS_U16 *) pSet);
			GOP_GWIN_SetGWinSharedCnt(pInstance, (MS_U8) gwin, u16SharedCnt);
			break;
		}
	default:
		break;
	}


	return GOP_API_SUCCESS;

}



MS_U32 Ioctl_GOP_GWin_GetProperty(void *pInstance, EN_GOP_GWIN_PROPERTY type, MS_U32 gwin,
				  MS_U32 *pSet, MS_U32 u32Size)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	switch (type) {
	case E_GOP_GWIN_ENABLE:
		{
			MS_BOOL *pbEn = (MS_BOOL *) pSet;

			if (u32Size != sizeof(MS_BOOL))
				return GOP_API_INVALID_PARAMETERS;

			*pbEn = GOP_GWIN_IsEnable(pInstance, gwin);

			break;
		}
	default:
		break;
	}


	return GOP_API_SUCCESS;

}

MS_U32 Ioctl_GOP_FB_GetProperty(void *pInstance, EN_GOP_FB_PROPERTY type, MS_U32 FBId,
				MS_U32 *pSet, MS_U32 u32Size)
{
	switch (type) {
	case E_GOP_FB_OBTAIN:
		{
			MS_U32 *pFreeFbID = (MS_U32 *) pSet;

			if (u32Size != sizeof(MS_U32))
				return GOP_API_INVALID_PARAMETERS;
			*pFreeFbID = (MS_U32) GOP_GWIN_GetFreeFbID(pInstance);
			break;
		}
	default:
		break;
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
MS_U32 Ioctl_GOP_SetDisplay(void *pInstance, PGOP_GWINDISPLAY_INFO pInfo)
{
	MS_U8 u8GOP;
	MS_U32 GwinId = pInfo->gwin;
	MS_U16 gwin_w, stretch_w;
	GOP_StretchInfo *pStretchInfo;
	GOP_GwinInfo *pGwinInfo = NULL;
	MS_U32 u32FbId;
	EN_GOP_STRETCH_DIRECTION direction;
	MS_U16 u16DstWidth, u16DstHeight;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	u32FbId = pInfo->fbid;
	direction = pInfo->dir;
	u16DstWidth = pInfo->dst_size.w;
	u16DstHeight = pInfo->dst_size.h;

	pGwinInfo = &(pInfo->gwin_info);
	pStretchInfo = &(pInfo->stretch_info);

	GOP_MapFB2Win(pInstance, u32FbId, GwinId);

	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, GwinId);

	gwin_w = pGwinInfo->u16DispHPixelEnd - pGwinInfo->u16DispHPixelStart;

	if (gwin_w > pStretchInfo->width)
		stretch_w = gwin_w;
	else
		stretch_w = pStretchInfo->width;

	//Store API use stretch window
	g_pGOPCtxLocal->pGOPCtxShared->u16APIStretchWidth[u8GOP] = stretch_w;
	g_pGOPCtxLocal->pGOPCtxShared->u16APIStretchHeight[u8GOP] = pStretchInfo->height;

	MDrv_GOP_GWIN_SetStretchWin(u8GOP, pStretchInfo->x, pStretchInfo->y,
				    stretch_w, pStretchInfo->height);

	if (u16DstWidth <= pStretchInfo->width || g_pGOPCtxLocal->pGOPCtxShared->bBypassEn)
		GOP_Set_Hscale(pInstance, u8GOP, FALSE, pStretchInfo->width, u16DstWidth);
	else if (direction & E_GOP_H_STRETCH)
		GOP_Set_Hscale(pInstance, u8GOP, TRUE, pStretchInfo->width, u16DstWidth);

	if (u16DstHeight <= pStretchInfo->height  || g_pGOPCtxLocal->pGOPCtxShared->bBypassEn)
		GOP_Set_Vscale(pInstance, u8GOP, FALSE, pStretchInfo->height, u16DstHeight);
	else if (direction & E_GOP_V_STRETCH)
		GOP_Set_Vscale(pInstance, u8GOP, TRUE, pStretchInfo->height, u16DstHeight);

	GOP_SetWinInfo(pInstance, GwinId, pGwinInfo);

	return GOP_API_SUCCESS;
}

MS_U32 Ioctl_GOP_FBCreate(void *pInstance, EN_GOP_CREATEBUFFER_TYPE fbtype, PGOP_BUFFER_INFO pBuff,
			  MS_U32 fbId)
{
	if (fbtype == GOP_CREATE_BUFFER_BYADDR)
		GOP_CreateBufferByAddr(pInstance, pBuff, fbId);

	return GOP_API_SUCCESS;

}


MS_U32 Ioctl_GOP_FBDestroy(void *pInstance, MS_U32 *pId)
{
	GOP_WinFB_INFO *pwinFB;
	MS_U32 u32fbId;

	u32fbId = *pId;

	if (u32fbId >= DRV_MAX_GWIN_FB_SUPPORT) {
		GOP_M_ERR
			     ("%s(u32fbId:%td....), fbId out of bound\n", __func__,
			      (ptrdiff_t) u32fbId);
		return GWIN_NO_AVAILABLE;

	}
	pwinFB = _GetWinFB(pInstance, u32fbId);

	if (pwinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GWIN_NO_AVAILABLE;
	}

	if (pwinFB->in_use == 0) {
		GOP_M_ERR
			     ("[%s][%d]: u32fbId=%td is not in existence\n", __func__, __LINE__,
			      (ptrdiff_t) u32fbId);
		return GWIN_NO_AVAILABLE;

	}

	GOP_FB_Destroy(pInstance, u32fbId);

	return GWIN_OK;

}

MS_U32 GOP_UpdateOnce(void *pInstance, MS_U32 gop, MS_BOOL bUpdateOnce, MS_BOOL bMl,
	MS_U8 MlResIdx)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (bUpdateOnce == TRUE && bMl == TRUE) {
		MDrv_GOP_SetMLBegin((MS_U8)gop);
	} else if (bUpdateOnce == FALSE && bMl == TRUE) {
		MDrv_GOP_SetMLScript(g_pGOPCtxLocal, (MS_U8)gop, MlResIdx);
		MDrv_GOP_SetADL(g_pGOPCtxLocal, (MS_U8)gop);
	} else if (bUpdateOnce == FALSE && bMl == FALSE) {
		MDrv_GOP_GWIN_UpdateRegWithSync(g_pGOPCtxLocal, gop, bMl);
	}

	return GOP_API_SUCCESS;

}

MS_U32 Ioctl_GOP_PowerState(void *pInstance, MS_U32 u32PowerState, void *pModule)
{
	MS_U32 u32Return = GOP_API_FAIL;
	GOP_CTX_DRV_SHARED *pDrvGOPShared = NULL;
	MS_BOOL bNeedInitShared = FALSE;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	GOP_M_INFO("[%s][%d] IoctlGOP STR PowerState=%tx\n", __func__, __LINE__,
		   (ptrdiff_t) u32PowerState);

	pDrvGOPShared = (GOP_CTX_DRV_SHARED *) MDrv_GOP_GetShareMemory(&bNeedInitShared);

	if (pDrvGOPShared == NULL) {
		GOP_M_ERR("[%s][%d]Pointer pDrvGOPShared is null!!\n", __func__, __LINE__);
		return GOP_API_FAIL;
	}

	switch (u32PowerState) {
	case E_POWER_SUSPEND:
		GOP_M_INFO("[%s][%d] IoctlGOP STR Suspend Start!!!!!\n", __func__,
			   __LINE__);
		MDrv_GOP_GWIN_PowerState(pInstance, u32PowerState, pModule);
		GOP_M_INFO("[%s][%d] IoctlGOP STR Suspend End!!!!!\n", __func__,
			   __LINE__);
		u32Return = GOP_API_SUCCESS;
	break;
	case E_POWER_RESUME:
		GOP_M_INFO("[%s][%d] IoctlGOP Resume Start!!!!!\n", __func__, __LINE__);
		MDrv_GOP_GWIN_PowerState(pInstance, u32PowerState, pModule);
		GOP_M_INFO("[%s][%d] IoctlGOP STR Resume End!!!!!\n", __func__,
			   __LINE__);
		u32Return = GOP_API_SUCCESS;
	break;
	default:
		GOP_M_ERR("[%s][%d] PowerState:%tx not Implement now!!!\n", __func__,
			  __LINE__, (ptrdiff_t) u32PowerState);
	break;
	}

	return u32Return;
}

MS_U32 Ioctl_GOP_SetLayer(void *pInstance, PGOP_SETLayer pstLayer, MS_U32 u32Size)
{
	MS_U16 u16Idx = 0;
	Gop_MuxSel enMux = E_GOP_MUX_INVALID;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	for (u16Idx = 0; u16Idx < pstLayer->u32LayerCount; u16Idx++) {
		MDrv_GOP_MapLayer2Mux(g_pGOPCtxLocal, pstLayer->u32Layer[u16Idx],
				      pstLayer->u32Gop[u16Idx], (MS_U32 *) &enMux);
		MDrv_GOP_GWIN_SetMux(g_pGOPCtxLocal, pstLayer->u32Gop[u16Idx], enMux);
	}

	return GOP_API_SUCCESS;
}

int Ioctl_GOP_Interrupt(void *pInstance, MS_U8 u8Gop, MS_BOOL bEnable)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE	* psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	MDrv_GOP_GWIN_Interrupt(g_pGOPCtxLocal, u8Gop, bEnable);
	return GOP_API_SUCCESS;
}

MS_U32 Ioctl_GOP_SetTestPattern(ST_GOP_TESTPATTERN *pstGOPTestPatInfo)
{
	if (MDrv_GOP_SetTestPattern(pstGOPTestPatInfo) == GOP_SUCCESS)
		return GOP_API_SUCCESS;
	else
		return GOP_API_FAIL;
}

MS_U32 Ioctl_GOP_GetCRC(MS_U32 *CRCvalue)
{
	if (MDrv_GOP_GetCRC(CRCvalue) == GOP_SUCCESS)
		return GOP_API_SUCCESS;
	else
		return GOP_API_FAIL;
}

MS_U32 Ioctl_GOP_PrintInfo(MS_U8 GOPNum)
{
	if (MDrv_GOP_PrintInfo(GOPNum) == GOP_SUCCESS)
		return GOP_API_SUCCESS;
	else
		return GOP_API_FAIL;
}

MS_U32 Ioctl_GOP_SetGOPBypassPath(void *pInstance, MS_U8 u8GOP, MS_BOOL bEnable)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	g_pGOPCtxLocal->pGOPCtxShared->bBypassEn = bEnable;
	if (MDrv_GOP_SetGOPBypassPath(g_pGOPCtxLocal, u8GOP, bEnable) == GOP_SUCCESS)
		return GOP_API_SUCCESS;
	else
		return GOP_API_FAIL;
}

MS_U32 Ioctl_GOP_SetTGen(void *pInstance, ST_GOP_TGEN_INFO *pGOPTgenInfo)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (pGOPTgenInfo != NULL) {
		g_pGOPCtxLocal->pGOPCtxShared->GOPOutMode = pGOPTgenInfo->GOPOutMode;
	} else {
		GOP_M_ERR("[%s][%d]pGOPTgenInfo is NULL pointer!\n", __func__, __LINE__);
		return GOP_API_FAIL;
	}

	if (MDrv_GOP_SetTGen(pGOPTgenInfo) == GOP_SUCCESS)
		return GOP_API_SUCCESS;
	else
		return GOP_API_FAIL;
}

MS_U32 GOP_SetTGenVGSync(MS_BOOL bEnable)
{
	if (MDrv_GOP_SetTGenVGSync(bEnable) == GOP_SUCCESS)
		return GOP_API_SUCCESS;
	else
		return GOP_API_FAIL;
}

MS_U32 GOP_SetMixerOutMode(void *pInstance, MS_BOOL Mixer1_OutPreAlpha, MS_BOOL Mixer2_OutPreAlpha)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (Mixer1_OutPreAlpha == TRUE)
		g_pGOPCtxLocal->pGOPCtxShared->bMixer4RmaEn = FALSE;
	else
		g_pGOPCtxLocal->pGOPCtxShared->bMixer4RmaEn = TRUE;

	if (Mixer2_OutPreAlpha == TRUE)
		g_pGOPCtxLocal->pGOPCtxShared->bMixer2RmaEn = FALSE;
	else
		g_pGOPCtxLocal->pGOPCtxShared->bMixer2RmaEn = TRUE;

	if (MDrv_GOP_SetMixerOutMode(Mixer1_OutPreAlpha, Mixer2_OutPreAlpha) == GOP_SUCCESS)
		return GOP_API_SUCCESS;
	else
		return GOP_API_FAIL;
}

MS_U32 GOP_TriggerRegWriteIn(void *pInstance, MS_U32 GOP, MS_BOOL bEn, MS_BOOL bMl,
	MS_U8 MlResIdx)
{
	if (GOP_UpdateOnce(pInstance, GOP, bEn, bMl, MlResIdx)
		== GOP_SUCCESS)
		return GOP_API_SUCCESS;
	else
		return GOP_API_FAIL;
}

MS_U32 GOP_SetPQCtx(void *pInstance, MS_U8 u8GOP, ST_GOP_PQCTX_INFO *pGopPQCtx)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	g_pGOPCtxLocal->pGOPCtxShared->stGopPQCtx[u8GOP].pCtx = pGopPQCtx->pCtx;

	return GOP_API_SUCCESS;
}

MS_U32 GOP_SetAdlnfo(void *pInstance, MS_U8 u8GOP, ST_GOP_ADL_INFO *pGopAdlInfo)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	MS_U8 *pu8_adl_buf = NULL;

	if (u8GOP < SHARED_GOP_MAX_COUNT) {
		pu8_adl_buf = g_pGOPCtxLocal->pGOPCtxShared->GopAdl[u8GOP].AdlBuf;
		g_pGOPCtxLocal->pGOPCtxShared->stGOPAdlInfo[u8GOP].fd = pGopAdlInfo->fd;
		g_pGOPCtxLocal->pGOPCtxShared->stGOPAdlInfo[u8GOP].adl_bufsize =
			pGopAdlInfo->adl_bufsize;
	} else {
		GOP_M_ERR("[%s, %d]: Invalid GOP index%d\n", __func__, __LINE__, u8GOP);
		g_pGOPCtxLocal->pGOPCtxShared->stGOPAdlInfo[u8GOP].fd = 0;
		g_pGOPCtxLocal->pGOPCtxShared->stGOPAdlInfo[u8GOP].adl_bufsize = 0;
		g_pGOPCtxLocal->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP] = FALSE;
		return GOP_API_FAIL;
	}

	if (pGopAdlInfo->u64_user_adl_buf != NULL) {
		if (pGopAdlInfo->adl_bufsize > ADL_MAX_BUFSIZE) {
			GOP_M_ERR("[%s, %d]adl_bufsize:0x%lx > ADL_MAX_BUFSIZE\n",
				__func__, __LINE__, pGopAdlInfo->adl_bufsize);
			pGopAdlInfo->adl_bufsize = ADL_MAX_BUFSIZE;
		}

		memset(pu8_adl_buf, 0, sizeof(MS_U8) * ADL_MAX_BUFSIZE);
		if (copy_from_user(pu8_adl_buf, pGopAdlInfo->u64_user_adl_buf,
			pGopAdlInfo->adl_bufsize)) {
			GOP_M_ERR("[%s, %d]: copy_from_user failed.\n",
			__func__, __LINE__);
			g_pGOPCtxLocal->pGOPCtxShared->stGOPAdlInfo[u8GOP].fd = 0;
			g_pGOPCtxLocal->pGOPCtxShared->stGOPAdlInfo[u8GOP].adl_bufsize = 0;
			g_pGOPCtxLocal->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP] = FALSE;
			return GOP_API_FAIL;
		}
	}

	g_pGOPCtxLocal->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP] = TRUE;

	return GOP_API_SUCCESS;
}

MS_U32 _GOP_ParseData2HwAutoGen(
	GOP_HWREG_INFO * pHwRegInfo,
	MS_U8 *pData)
{
	int i = 0;

	if (pHwRegInfo != NULL) {
		for (i = 0; i < sizeof(MS_U32); i++) {
			if (pData != NULL) {
				pHwRegInfo->u32RegIdx |= *(pData++)<<(i*BITS_PER_BYTE);
			} else {
				DRM_ERROR("[GOP][%s, %d] pData is NULL\n", __func__, __LINE__);
				return GOP_INVALID_BUFF_INFO;
			}
		}

		for (i = 0; i < sizeof(MS_U16); i++) {
			if (pData != NULL) {
				pHwRegInfo->u16Value |= *(pData++)<<(i*BITS_PER_BYTE);
			} else {
				DRM_ERROR("[GOP][%s, %d] pData is NULL\n", __func__, __LINE__);
				return GOP_INVALID_BUFF_INFO;
			}
		}
	}

	return GOP_SUCCESS;
}

MS_U32 GOP_SetPqMlnfo(void *pInstance, MS_U8 u8GOP, ST_GOP_PQ_CFD_ML_INFO *pGopMlInfo)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	MS_U8 *pu8_cfd_ml_buf;
	MS_U8 *pu8_pq_ml_buf;
	MS_U32 u32_cfd_ml_bufsize = pGopMlInfo->u32_cfd_ml_bufsize;
	MS_U32 u32_pq_ml_bufsize = pGopMlInfo->u32_pq_ml_bufsize;
	MS_U32 i = 0;
	MS_GOP_CTX_SHARED *pGopCtx = g_pGOPCtxLocal->pGOPCtxShared;
	MS_U8 *tmp = NULL;
	GOP_HWREG_INFO *pCfdMl = NULL;
	GOP_HWREG_INFO *pPqMl = NULL;

	if (u8GOP < SHARED_GOP_MAX_COUNT) {
		pCfdMl = pGopCtx->GopCFDMl[u8GOP].stCFD;
		pPqMl = pGopCtx->GopPQMl[u8GOP].stPQ;
	} else {
		GOP_M_ERR("[%s, %d]: Invalid GOP index%d\n", __func__, __LINE__, u8GOP);
		pGopCtx->stGopPqCfdMlInfo[u8GOP].bCFDUpdated = FALSE;
		pGopCtx->stGopPqCfdMlInfo[u8GOP].bPQUpdated = FALSE;
		return GOP_API_FAIL;
	}

	pGopCtx->stGopPqCfdMlInfo[u8GOP].u32_cfd_ml_bufsize =
		pGopMlInfo->u32_cfd_ml_bufsize;

	if (pGopMlInfo->u64_user_cfd_ml_buf != NULL) {
		pu8_cfd_ml_buf = kmalloc_array(u32_cfd_ml_bufsize,
			sizeof(MS_U8), GFP_KERNEL);
		if (pu8_cfd_ml_buf) {
			if (copy_from_user(pu8_cfd_ml_buf, pGopMlInfo->u64_user_cfd_ml_buf,
				u32_cfd_ml_bufsize)) {
				GOP_M_ERR("[%s, %d]copy_from_user failed\n", __func__, __LINE__);
				pGopCtx->stGopPqCfdMlInfo[u8GOP].bCFDUpdated = FALSE;
			} else {
				memset(pCfdMl, 0, sizeof(GOP_HWREG_INFO) * CFDML_MAX_CMD);
				tmp = pu8_cfd_ml_buf;
				for (i = 0; i < (u32_cfd_ml_bufsize/CFD_ML_BUF_ENTRY_SIZE); i++) {
					if (i < CFDML_MAX_CMD) {
						_GOP_ParseData2HwAutoGen(
							&pCfdMl[i], tmp);
					} else {
						GOP_M_ERR("[%s, %d]cfd ml cmd > %d\n",
							__func__, __LINE__, CFDML_MAX_CMD);
					}
					tmp += CFD_ML_BUF_ENTRY_SIZE;
				}
				pGopCtx->stGopPqCfdMlInfo[u8GOP].bCFDUpdated = TRUE;
			}
			kfree(pu8_cfd_ml_buf);
		} else {
			GOP_M_ERR("[%s, %d]cfd_ml_buf kmalloc failed\n", __func__, __LINE__);
			pGopCtx->stGopPqCfdMlInfo[u8GOP].bCFDUpdated = FALSE;
		}
	} else {
		pGopCtx->stGopPqCfdMlInfo[u8GOP].bCFDUpdated = FALSE;
	}

	pGopCtx->stGopPqCfdMlInfo[u8GOP].u32_pq_ml_bufsize =
		pGopMlInfo->u32_pq_ml_bufsize;

	if (pGopMlInfo->u64_user_pq_ml_buf != NULL) {
		pu8_pq_ml_buf = kmalloc_array(u32_pq_ml_bufsize, sizeof(MS_U8), GFP_KERNEL);
		if (pu8_pq_ml_buf) {
			if (copy_from_user(pu8_pq_ml_buf, pGopMlInfo->u64_user_pq_ml_buf,
				u32_pq_ml_bufsize)) {
				GOP_M_ERR("[%s, %d]copy_from_user failed\n", __func__, __LINE__);
				pGopCtx->stGopPqCfdMlInfo[u8GOP].bPQUpdated = FALSE;
			} else {
				memset(pPqMl, 0, sizeof(GOP_HWREG_INFO) * PQML_MAX_CMD);
				tmp = pu8_pq_ml_buf;
				for (i = 0; i < (u32_pq_ml_bufsize/CFD_ML_BUF_ENTRY_SIZE); i++) {
					if (i < PQML_MAX_CMD) {
						_GOP_ParseData2HwAutoGen(
							&pPqMl[i], tmp);
					} else {
						GOP_M_ERR("[%s, %d]pq ml cmd > %d\n",
							__func__, __LINE__, PQML_MAX_CMD);
					}
					tmp += CFD_ML_BUF_ENTRY_SIZE;
				}
				pGopCtx->stGopPqCfdMlInfo[u8GOP].bPQUpdated = TRUE;
			}
			kfree(pu8_pq_ml_buf);
		} else {
			GOP_M_ERR("[%s, %d]pq_ml_buf kmalloc failed\n", __func__, __LINE__);
			pGopCtx->stGopPqCfdMlInfo[u8GOP].bPQUpdated = FALSE;
		}
	} else {
		pGopCtx->stGopPqCfdMlInfo[u8GOP].bPQUpdated = FALSE;
	}

	return GOP_API_SUCCESS;
}

MS_U32 GOP_SetAidType(MS_U8 u8GOP, MS_U64 u64GopAidType)
{
	return MDrv_GOP_SetAidType(u8GOP, u64GopAidType);
}

MS_U32 GOP_GetRoi(ST_GOP_GETROI *pGOPRoi)
{
	return MDrv_GOP_GetRoi(pGOPRoi);
}

MS_U32 GOP_SetVGOrder(EN_GOP_VG_ORDER eVGOrder)
{
	return MDrv_GOP_SetVGOrder(eVGOrder);
}

MS_U32 GOP_ml_fire(void *pInstance, MS_U8 u8MlResIdx)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE	* psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	return MDrv_GOP_ml_fire(g_pGOPCtxLocal, u8MlResIdx);
}

MS_U32 GOP_ml_get_mem_info(void *pInstance, MS_U8 u8MlResIdx)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE	* psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	return MDrv_GOP_ml_get_mem_info(g_pGOPCtxLocal, u8MlResIdx);
}

// this func will be call to init by utopia20 framework
void GOPRegisterToUtopia(void)
{
	// 1. deal with module
	void *pUtopiaModule = NULL;
	void *psResource = NULL;

	UtopiaModuleCreate(MODULE_GOP, 8, &pUtopiaModule);
	UtopiaModuleRegister(pUtopiaModule);
	// register func for module, after register here
	UtopiaModuleSetupFunctionPtr(pUtopiaModule, (FUtopiaOpen) GOPOpen, (FUtopiaClose) GOPClose,
				     (FUtopiaIOctl) GOPIoctl);

	// 2. deal with resource
	// start func to add res, call once will create 2 access in resource.
	UtopiaModuleAddResourceStart(pUtopiaModule, GOP_POOL_ID_GOP0);
	// resource can alloc private for internal use
	UtopiaResourceCreate("gop0", sizeof(GOP_RESOURCE_PRIVATE), &psResource);
	// func to reg res
	UtopiaResourceRegister(pUtopiaModule, psResource, GOP_POOL_ID_GOP0);
	UtopiaModuleAddResourceEnd(pUtopiaModule, GOP_POOL_ID_GOP0);
}

MS_U32 GOPOpen(void **ppInstance, const void *const pAttribute)
{
	GOP_INSTANT_PRIVATE *pGopPri = NULL;

	GOP_M_INFO("\n[GOP INFO] gop open\n");
	//UTOPIA_TRACE(MS_UTOPIA_DB_LEVEL_TRACE,printf("enter %s %d\n",__func__,__LINE__));
	// instance is allocated here, also can allocate private for internal use
	UtopiaInstanceCreate(sizeof(GOP_INSTANT_PRIVATE), ppInstance);
	// setup func in private and assign the calling func in func ptr in instance private
	UtopiaInstanceGetPrivate(*ppInstance, (void **)&pGopPri);

	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 GOPIoctl(void *pInstance, MS_U32 u32Cmd, void *pArgs)
{
	void *pModule = NULL;
	MS_U32 u32Ret = GOP_API_SUCCESS;
	GOP_INSTANT_PRIVATE *psGOPInstPri = NULL;
	MS_BOOL bLocked = FALSE;

	UtopiaInstanceGetModule(pInstance, &pModule);
	UtopiaInstanceGetPrivate(pInstance, (void **)&psGOPInstPri);

	if ((u32Cmd != MAPI_CMD_GOP_INTERRUPT)
	    && (g_pGOPCtxLocal != NULL)
	    && (g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid != MsOS_GetOSThreadID())) {
		if (UtopiaResourceObtain(pModule, GOP_POOL_ID_GOP0, &(psGOPInstPri->pResource)) !=
		    0) {
			GOP_M_ERR("[%s %d]UtopiaResourceObtainToInstant fail\n", __func__,
				  __LINE__);
			return 0xFFFFFFFF;
		}
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
		g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = (GETPIDTYPE) getpid();
#endif
		bLocked = TRUE;
	}
	//printf("[%s] cmd:%lx\n",__func__,u32Cmd);

	switch (u32Cmd) {
	case MAPI_CMD_GOP_INIT:
		{
			PGOP_INIT_PARAM ptr = (PGOP_INIT_PARAM) pArgs;

#ifndef MSOS_TYPE_LINUX_KERNEL	//Check size error when if(is_compat_task()==1)
			if (ptr->u32Size != sizeof(GOP_InitInfo) && g_pGOPCtxLocal != NULL) {
				GOP_M_INFO
				    ("[%s][%d] input size %tx error, require:%tx\n",
				     __func__, __LINE__, (ptrdiff_t) ptr->u32Size,
				     sizeof(GOP_INIT_PARAM));
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
				UtopiaResourceRelease(psGOPInstPri->pResource);
				return UTOPIA_STATUS_PARAMETER_ERROR;
			}
#endif
			u32Ret =
			    Ioctl_GOP_Init(pInstance, ptr->gop_idx,
					   (GOP_InitInfo *) (void *)ptr->pInfo);

			break;
		}
	case MAPI_CMD_GOP_GET_CHIPCAPS:
		{
			PGOP_GETCAPS_PARAM ptr = (PGOP_GETCAPS_PARAM) pArgs;

			u32Ret =
			    Ioctl_GOP_GetCaps(pInstance, (EN_GOP_CAPS) ptr->caps, ptr->pInfo,
					      ptr->u32Size);
			break;
		}
	case MAPI_CMD_GOP_SET_CONFIG:
		{
			PGOP_SETCONFIG_PARAM ptr = (PGOP_SETCONFIG_PARAM) pArgs;

			u32Ret =
			    Ioctl_GOP_SetConfig(pInstance, ptr->cfg_type, ptr->pCfg, ptr->u32Size);
			break;

		}
	case MAPI_CMD_GOP_SET_PROPERTY:
		{
			PGOP_SET_PROPERTY_PARAM ptr = (PGOP_SET_PROPERTY_PARAM) pArgs;


			u32Ret =
			    Ioctl_GOP_SetProperty(pInstance, ptr->en_pro, ptr->gop_idx,
						  ptr->pSetting, ptr->u32Size);

			break;
		}
	case MAPI_CMD_GOP_SET_DST:
		{
			PGOP_SETDST_PARAM ptr = (PGOP_SETDST_PARAM) pArgs;

			u32Ret = Ioctl_GOP_SetDst(pInstance, ptr->gop_idx, ptr->en_dst);
			break;
		}
	case MAPI_CMD_GOP_SET_LAYER:
		{
			PGOP_SETLAYER_PARAM ptr = (PGOP_SETLAYER_PARAM) pArgs;
#ifndef MSOS_TYPE_LINUX_KERNEL	//Check size error when if(is_compat_task()==1)
			if (ptr->u32Size != sizeof(GOP_SETLayer) && g_pGOPCtxLocal != NULL) {
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
				UtopiaResourceRelease(psGOPInstPri->pResource);
				return UTOPIA_STATUS_PARAMETER_ERROR;
			}
#endif
			if (g_pGOPCtxLocal == NULL)
				u32Ret = UTOPIA_STATUS_FAIL;

			Ioctl_GOP_SetLayer(pInstance, (PGOP_SETLayer) ptr->pLayerInfo,
					   ptr->u32Size);
			break;
		}
	case MAPI_CMD_GOP_SET_MIRROR:
		{
			PGOP_SETMIRROR_PARAM ptr = (PGOP_SETMIRROR_PARAM) pArgs;

			u32Ret = Ioctl_GOP_SetMirror(pInstance, ptr->gop_idx, ptr->dir);
			break;
		}
	case MAPI_CMD_GOP_GET_STATUS:
		{
			PGOP_GET_STATUS_PARAM ptr = (PGOP_GET_STATUS_PARAM) pArgs;

			u32Ret =
			    Ioctl_GOP_GetStatus(pInstance, ptr->en_status, ptr->pStatus,
						ptr->u32Size);
			break;
		}
		//Stretch Win
	case MAPI_CMD_GOP_GWIN_SET_STRETCH:
		{
			PGOP_STRETCH_SET_PARAM ptr = (PGOP_STRETCH_SET_PARAM) pArgs;
			PGOP_STRETCH_INFO pInfo = (PGOP_STRETCH_INFO) ptr->pStretch;

			if (ptr->u32Size != sizeof(GOP_STRETCH_INFO)) {
				GOP_M_ERR("[%s] (%d) Info structure Error!!\n", __func__,
					  __LINE__);
				return GOP_API_INVALID_PARAMETERS;
			}

			u32Ret =
			    Ioctl_GOP_Set_Stretch(pInstance, ptr->enStrtchType, ptr->gop_idx,
						  pInfo);
			break;
		}
		//GWIN info
	case MAPI_CMD_GOP_GWIN_SET_PROPERTY:
		{
			PGOP_GWIN_PROPERTY_PARAM ptr = (PGOP_GWIN_PROPERTY_PARAM) pArgs;

			u32Ret =
			    Ioctl_GOP_GWin_SetProperty(pInstance, ptr->en_property, ptr->GwinId,
						       ptr->pSet, ptr->u32Size);

			break;
		}
	case MAPI_CMD_GOP_GWIN_GET_PROPERTY:
		{
			PGOP_GWIN_PROPERTY_PARAM ptr = (PGOP_GWIN_PROPERTY_PARAM) pArgs;

			u32Ret =
			    Ioctl_GOP_GWin_GetProperty(pInstance, ptr->en_property, ptr->GwinId,
						       ptr->pSet, ptr->u32Size);

			break;
		}
	case MAPI_CMD_GOP_GWIN_SETDISPLAY:
		{
			PGOP_GWIN_DISPLAY_PARAM ptr = (PGOP_GWIN_DISPLAY_PARAM) pArgs;
			PGOP_GWINDISPLAY_INFO pInfo = (PGOP_GWINDISPLAY_INFO) ptr->pDisplayInfo;
#ifndef MSOS_TYPE_LINUX_KERNEL	//Check size error when if(is_compat_task()==1)
			if (ptr->u32Size != sizeof(GOP_GWINDISPLAY_INFO) &&
				g_pGOPCtxLocal != NULL) {
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
				UtopiaResourceRelease(psGOPInstPri->pResource);
				return UTOPIA_STATUS_PARAMETER_ERROR;
			}
#endif
			u32Ret = Ioctl_GOP_SetDisplay(pInstance, pInfo);
			break;
		}
		//FB Info
	case MAPI_CMD_GOP_FB_CREATE:
		{
			PGOP_CREATE_BUFFER_PARAM ptr = (PGOP_CREATE_BUFFER_PARAM) pArgs;
			PGOP_BUFFER_INFO pBuff = (PGOP_BUFFER_INFO) ptr->pBufInfo;
#ifndef MSOS_TYPE_LINUX_KERNEL	//Check size error when if(is_compat_task()==1)
			if (ptr->u32Size != sizeof(GOP_BUFFER_INFO) && g_pGOPCtxLocal != NULL) {
				GOP_M_INFO("[%s] (%d) Info structure Error!!\n", __func__,
					   __LINE__);
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
				UtopiaResourceRelease(psGOPInstPri->pResource);
				return UTOPIA_STATUS_PARAMETER_ERROR;
			}
#endif

			u32Ret = Ioctl_GOP_FBCreate(pInstance, ptr->fb_type, pBuff, ptr->fbid);
			break;
		}
	case MAPI_CMD_GOP_FB_DESTROY:
		{
			PGOP_DELETE_BUFFER_PARAM ptr = (PGOP_DELETE_BUFFER_PARAM) pArgs;

			if (ptr->u32Size != sizeof(MS_U32) && g_pGOPCtxLocal != NULL) {
				GOP_M_INFO("[%s] (%d) Info structure Error!!\n", __func__,
					   __LINE__);
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
				UtopiaResourceRelease(psGOPInstPri->pResource);
				return UTOPIA_STATUS_PARAMETER_ERROR;
			}
			u32Ret = Ioctl_GOP_FBDestroy(pInstance, ptr->pBufId);

			break;
		}
	case MAPI_CMD_GOP_FB_GET_PROPERTY:
		{
			PGOP_FB_PROPERTY_PARAM ptr = (PGOP_FB_PROPERTY_PARAM) pArgs;

			u32Ret =
			    Ioctl_GOP_FB_GetProperty(pInstance, ptr->en_property, ptr->FBId,
						     ptr->pSet, ptr->u32Size);

			break;
		}
		break;
		//MISC
	case MAPI_CMD_GOP_POWERSTATE:
		{
			PGOP_POWERSTATE_PARAM ptr = (PGOP_POWERSTATE_PARAM) pArgs;
			MS_U8 *u8Val = ptr->pInfo;

			u32Ret = Ioctl_GOP_PowerState(pInstance, (MS_U32) *u8Val, pModule);
			break;
		}
	case MAPI_CMD_GOP_INTERRUPT:
		{
			PGOP_INTERRUPT_PARAM ptr = (PGOP_INTERRUPT_PARAM) pArgs;
			MS_BOOL *pbEnable = (MS_BOOL *) ptr->pSetting;

			u32Ret = Ioctl_GOP_Interrupt(pInstance, ptr->gop_idx, *pbEnable);
			break;
		}
	default:
		break;
	}

	if (bLocked && g_pGOPCtxLocal != NULL) {
		g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
		g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
		UtopiaResourceRelease(psGOPInstPri->pResource);
	}
	if (u32Ret != GOP_API_SUCCESS) {
		if (u32Ret == GOP_API_FUN_NOT_SUPPORTED) {
			u32Ret = UTOPIA_STATUS_NOT_SUPPORTED;
		} else {
			GOP_M_ERR("[%s][%d] ERROR on cmd:0x%tx\n", __func__, __LINE__,
				  (ptrdiff_t) u32Cmd);
			u32Ret = UTOPIA_STATUS_FAIL;
		}
	} else
		u32Ret = UTOPIA_STATUS_SUCCESS;
	//printf("(%s) Done\n\n",__func__);

	return u32Ret;
}

MS_U32 GOPClose(void *pInstance)
{
	return TRUE;
}
