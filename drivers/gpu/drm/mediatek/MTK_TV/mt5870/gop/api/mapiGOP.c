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
#include "apiGOP.h"
#include "apiGOP_priv.h"
#include "drvGOP.h"
#include "drvGFLIP.h"
#include "drvGOP_priv.h"

enum {
	GOP_POOL_ID_GOP0 = 0
} eGOPPoolID;

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

#define GetMaxActiveGwinFalse               4UL
#define GetMaxActiveGwinFalse_op            5UL
#define GetMaxActiveGwinFalse_opened        6UL
#define GWIN_SDRAM_NULL                     0x30UL
#define msWarning(c)                        do {} while (0)
#define XC_MAIN_WINDOW                      0UL


// Define return values of check align
#define CHECKALIGN_SUCCESS                  1UL
#define CHECKALIGN_FORMAT_FAIL              2UL
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
/// GOP HVMirror GWIN position option define
typedef enum {
	E_Set_HPos,
	E_Set_VPos,
	E_Set_X,
	E_Set_Y,
} HMirror_Opt;

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
	case E_DRV_GOP_FB_AFBC_SPLT_YUVTRNSFER_ARGB8888:
		*AFBC_Enable = TRUE;
		*u8AFBC_Cmd =
		    (MS_U8) (E_DRV_GOP_AFBC_SPILT | E_DRV_GOP_AFBC_YUV_TRANSFER |
			     E_DRV_GOP_AFBC_ARGB8888);
		break;
	case E_DRV_GOP_FB_AFBC_NONSPLT_YUVTRS_ARGB8888:
		*AFBC_Enable = TRUE;
		*u8AFBC_Cmd = (MS_U8) (E_DRV_GOP_AFBC_YUV_TRANSFER | E_DRV_GOP_AFBC_ARGB8888);
		break;
	case E_DRV_GOP_FB_AFBC_SPLT_NONYUVTRS_ARGB8888:
		*AFBC_Enable = TRUE;
		*u8AFBC_Cmd = (MS_U8) (E_DRV_GOP_AFBC_SPILT | E_DRV_GOP_AFBC_ARGB8888);
		break;
	case E_DRV_GOP_FB_AFBC_NONSPLT_NONYUVTRS_ARGB8888:
	case E_DRV_GOP_FB_AFBC_V1P2_ARGB8888:
		*AFBC_Enable = TRUE;
		*u8AFBC_Cmd = (MS_U8) E_DRV_GOP_AFBC_ARGB8888;
		break;
	case E_DRV_GOP_FB_AFBC_V1P2_RGB565:
		*AFBC_Enable = TRUE;
		*u8AFBC_Cmd = (MS_U8) E_DRV_GOP_AFBC_RGB565;
		break;
	case E_DRV_GOP_FB_AFBC_V1P2_RGB888:
		*AFBC_Enable = TRUE;
		*u8AFBC_Cmd = (MS_U8) E_DRV_GOP_AFBC_RGB888;
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

static void _GOP_GetGop0WinInfo(void *pInstance, MS_U8 u8win, GOP_GwinInfo *pinfo)
{
	DRV_GOP_GWIN_INFO sGetInfo;
	DRV_GOPDstType Gop0Dst = E_DRV_GOP_DST_INVALID;
	GOP_WinFB_INFO *pwinFB;
	MS_BOOL bInterlace = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, u8win)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, u8win);
		return;
	}


	pwinFB = _GetWinFB(pInstance, g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u8win].u32CurFBId);

	if (pwinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : gwinMap[%d].u32CurFBId=%td\n", __func__,
			  __LINE__, u8win,
			  (ptrdiff_t) g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u8win].u32CurFBId);
		return;
	}

	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, 0, &Gop0Dst);
	if (Gop0Dst == E_DRV_GOP_DST_INVALID) {
		APIGOP_ASSERT(FALSE, GOP_M_FATAL("\n Get GOP destination fail!\n"));
		return;
	}
	memset(&sGetInfo, 0, sizeof(DRV_GOP_GWIN_INFO));
	MDrv_GOP_GWIN_GetGwinInfo(g_pGOPCtxLocal, u8win, &sGetInfo);
	pinfo->u16RBlkHPixSize = sGetInfo.u16RBlkHPixSize;
	pinfo->u16RBlkVPixSize = sGetInfo.u16RBlkVPixSize;
	//2009.03.14 Fix the TTX,CC position error
	sGetInfo.u16WinX = pwinFB->s_x;
	sGetInfo.u16WinY = pwinFB->s_y;

	pinfo->clrType = (EN_GOP_COLOR_TYPE) sGetInfo.clrType;

	if (HAS_BIT(g_pGOPCtxLocal->u16GOP_VMirror_VPos, u8win)) {
		pinfo->u16DispVPixelStart = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVStr;
		pinfo->u16DispVPixelEnd = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVEnd;

		if (g_pGOPCtxLocal->Gwin_V_Dup) {
			pinfo->u16DispVPixelEnd =
				(pinfo->u16DispVPixelEnd + pinfo->u16DispVPixelStart) >> 1;
		}
	} else {
		pinfo->u16DispVPixelStart = sGetInfo.u16DispVPixelStart;	// pix
		bInterlace = IsScalerOutputInterlace();
		if (g_pGOPCtxLocal->Gwin_V_Dup) {
			if ((Gop0Dst == E_DRV_GOP_DST_IP0) && bInterlace) {
				pinfo->u16DispVPixelEnd =
				    (((sGetInfo.u16DispVPixelEnd -
				       pinfo->u16DispVPixelStart) << 1) +
				     pinfo->u16DispVPixelStart) >> 1;
			} else {
				pinfo->u16DispVPixelEnd =
					(sGetInfo.u16DispVPixelEnd +
					pinfo->u16DispVPixelStart) >> 1;
			}
		} else {
			if ((Gop0Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    ((sGetInfo.u16DispVPixelEnd - pinfo->u16DispVPixelStart) << 1) +
				    pinfo->u16DispVPixelStart;
			else if (((Gop0Dst == E_DRV_GOP_DST_OP0) || (Gop0Dst == E_DRV_GOP_DST_FRC)
				  || (Gop0Dst == E_DRV_GOP_DST_BYPASS))
				 && !MDrv_GOP_GWIN_IsProgressive(g_pGOPCtxLocal, 0)) {
				pinfo->u16DispVPixelEnd =
				    ((sGetInfo.u16DispVPixelEnd - pinfo->u16DispVPixelStart) << 1) +
				    pinfo->u16DispVPixelStart;
			} else
				pinfo->u16DispVPixelEnd = sGetInfo.u16DispVPixelEnd;	// pix
		}
	}

	if (HAS_BIT(g_pGOPCtxLocal->u16GOP_HMirror_HPos, u8win)) {
		pinfo->u16DispHPixelStart = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHStr;
		pinfo->u16DispHPixelEnd = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHEnd;

		if (g_pGOPCtxLocal->Gwin_H_Dup) {
			pinfo->u16DispHPixelEnd =
				(pinfo->u16DispHPixelEnd + pinfo->u16DispHPixelStart) >> 1;
		}
	} else {
		pinfo->u16DispHPixelStart = sGetInfo.u16DispHPixelStart;
		pinfo->u16DispHPixelEnd = sGetInfo.u16DispHPixelEnd;

		if (g_pGOPCtxLocal->Gwin_H_Dup) {
			pinfo->u16DispHPixelEnd =
				(pinfo->u16DispHPixelEnd + pinfo->u16DispHPixelStart) >> 1;
		}
	}

	pinfo->u16RBlkHRblkSize = sGetInfo.u16RBlkHRblkSize;
	pinfo->u32DRAMRBlkStart = sGetInfo.u64DRAMRBlkStart;
	pinfo->u16WinY = sGetInfo.u16WinY;	//Original Y pix of FB
	pinfo->u16WinX = sGetInfo.u16WinX;	//Original X pix of FB
}

static void _GOP_GetGop1WinInfo(void *pInstance, MS_U8 u8win, GOP_GwinInfo *pinfo)
{
	DRV_GOP_GWIN_INFO sGetInfo;
	DRV_GOPDstType Gop1Dst = E_DRV_GOP_DST_INVALID;
	MS_BOOL bInterlace = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, u8win)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, u8win);
		return;
	}


	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, 1, &Gop1Dst);
	if (Gop1Dst == E_DRV_GOP_DST_INVALID) {
		APIGOP_ASSERT(FALSE, GOP_M_FATAL("\n Get GOP destination fail!\n"));
		return;
	}

	memset(&sGetInfo, 0, sizeof(DRV_GOP_GWIN_INFO));
	MDrv_GOP_GWIN_GetGwinInfo(g_pGOPCtxLocal, u8win, &sGetInfo);
	pinfo->u16RBlkHPixSize = sGetInfo.u16RBlkHPixSize;
	pinfo->u16RBlkVPixSize = sGetInfo.u16RBlkVPixSize;
	pinfo->clrType = (EN_GOP_COLOR_TYPE) sGetInfo.clrType;

	if (HAS_BIT(g_pGOPCtxLocal->u16GOP_VMirror_VPos, u8win)) {
		pinfo->u16DispVPixelStart = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVStr;
		pinfo->u16DispVPixelEnd = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVEnd;

		if (g_pGOPCtxLocal->Gwin_V_Dup) {
			pinfo->u16DispVPixelEnd =
				(pinfo->u16DispVPixelEnd + pinfo->u16DispVPixelStart) >> 1;
		}
	} else {
		pinfo->u16DispVPixelStart = sGetInfo.u16DispVPixelStart;	// pix
		bInterlace = IsScalerOutputInterlace();

		if (g_pGOPCtxLocal->Gwin_V_Dup) {
			if ((Gop1Dst == E_DRV_GOP_DST_IP0) && bInterlace) {
				pinfo->u16DispVPixelEnd =
				    (((sGetInfo.u16DispVPixelEnd -
				       pinfo->u16DispVPixelStart) << 1) +
				     pinfo->u16DispVPixelStart) >> 1;
			} else {
				pinfo->u16DispVPixelEnd =
					(sGetInfo.u16DispVPixelEnd +
					pinfo->u16DispVPixelStart) >> 1;
			}
		} else {
			if ((Gop1Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    ((sGetInfo.u16DispVPixelEnd - pinfo->u16DispVPixelStart) << 1) +
				    pinfo->u16DispVPixelStart;
			else if (((Gop1Dst == E_DRV_GOP_DST_OP0) || (Gop1Dst == E_DRV_GOP_DST_FRC)
				  || (Gop1Dst == E_DRV_GOP_DST_BYPASS))
				 && !MDrv_GOP_GWIN_IsProgressive(g_pGOPCtxLocal, 1)) {
				pinfo->u16DispVPixelEnd =
				    ((sGetInfo.u16DispVPixelEnd - pinfo->u16DispVPixelStart) << 1) +
				    pinfo->u16DispVPixelStart;
			} else
				pinfo->u16DispVPixelEnd = sGetInfo.u16DispVPixelEnd;	// pix
		}
	}

	if (HAS_BIT(g_pGOPCtxLocal->u16GOP_HMirror_HPos, u8win)) {
		pinfo->u16DispHPixelStart = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHStr;
		pinfo->u16DispHPixelEnd = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHEnd;

		if (g_pGOPCtxLocal->Gwin_H_Dup) {
			pinfo->u16DispHPixelEnd =
				(pinfo->u16DispHPixelEnd + pinfo->u16DispHPixelStart) >> 1;
		}

	} else {
		pinfo->u16DispHPixelStart = sGetInfo.u16DispHPixelStart;

		if (g_pGOPCtxLocal->Gwin_H_Dup) {
			pinfo->u16DispHPixelEnd =
				(sGetInfo.u16DispHPixelEnd + pinfo->u16DispHPixelStart) >> 1;
		} else {
			pinfo->u16DispHPixelEnd = sGetInfo.u16DispHPixelEnd;
		}
	}

	pinfo->u16RBlkHRblkSize = sGetInfo.u16RBlkHRblkSize;
	pinfo->u32DRAMRBlkStart = sGetInfo.u64DRAMRBlkStart;

	if (HAS_BIT(g_pGOPCtxLocal->u16GOP_HMirrorRBLK_Adr, u8win) ||
	    HAS_BIT(g_pGOPCtxLocal->u16GOP_VMirrorRBLK_Adr, u8win)) {
		pinfo->u32DRAMRBlkStart = g_pGOPCtxLocal->sMirrorInfo[u8win].u64NonMirrorFBAdr;
	}
}

static void _GOP_GetGop23WinInfo(void *pInstance, MS_U8 u8win, GOP_GwinInfo *pinfo)
{
	DRV_GOP_GWIN_INFO sGetInfo;
	DRV_GOPDstType Gop23Dst = E_DRV_GOP_DST_INVALID;
	MS_BOOL bInterlace = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, u8win)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, u8win);
		return;
	}


	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, MDrv_DumpGopByGwinId(g_pGOPCtxLocal, u8win),
				  &Gop23Dst);
	if (Gop23Dst == E_DRV_GOP_DST_INVALID) {
		APIGOP_ASSERT(FALSE, GOP_M_FATAL("\n Get GOP destination fail!\n"));
		return;
	}

	memset(&sGetInfo, 0, sizeof(DRV_GOP_GWIN_INFO));
	MDrv_GOP_GWIN_GetGwinInfo(g_pGOPCtxLocal, u8win, &sGetInfo);
	pinfo->u16RBlkHPixSize = sGetInfo.u16RBlkHPixSize;
	pinfo->u16RBlkVPixSize = sGetInfo.u16RBlkVPixSize;
	pinfo->clrType = (EN_GOP_COLOR_TYPE) sGetInfo.clrType;

	if (HAS_BIT(g_pGOPCtxLocal->u16GOP_VMirror_VPos, u8win)) {
		pinfo->u16DispVPixelStart = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVStr;
		pinfo->u16DispVPixelEnd = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVEnd;

		if (g_pGOPCtxLocal->Gwin_V_Dup)
			pinfo->u16DispVPixelEnd = (pinfo->u16DispVPixelEnd +
			pinfo->u16DispVPixelStart) >> 1;	//pix
	} else {
		pinfo->u16DispVPixelStart = sGetInfo.u16DispVPixelStart;	// pix
		bInterlace = IsScalerOutputInterlace();

		if (g_pGOPCtxLocal->Gwin_V_Dup) {
			if ((Gop23Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    (((sGetInfo.u16DispVPixelEnd -
				       pinfo->u16DispVPixelStart) << 1) +
				     pinfo->u16DispVPixelStart) >> 1;
			else
				pinfo->u16DispVPixelEnd = (sGetInfo.u16DispVPixelEnd +
				pinfo->u16DispVPixelStart) >> 1;	//pix
		} else {
			if ((Gop23Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    ((sGetInfo.u16DispVPixelEnd - pinfo->u16DispVPixelStart) << 1) +
				    pinfo->u16DispVPixelStart;
			else if (((Gop23Dst == E_DRV_GOP_DST_OP0) || (Gop23Dst == E_DRV_GOP_DST_FRC)
				  || (Gop23Dst == E_DRV_GOP_DST_BYPASS))
				 && !MDrv_GOP_GWIN_IsProgressive(g_pGOPCtxLocal,
								 MDrv_DumpGopByGwinId
								 (g_pGOPCtxLocal, u8win))) {
				pinfo->u16DispVPixelEnd =
				    ((sGetInfo.u16DispVPixelEnd - pinfo->u16DispVPixelStart) << 1) +
				    pinfo->u16DispVPixelStart;
			} else
				pinfo->u16DispVPixelEnd = sGetInfo.u16DispVPixelEnd;	// pix
		}
	}

	if (HAS_BIT(g_pGOPCtxLocal->u16GOP_HMirror_HPos, u8win)) {
		pinfo->u16DispHPixelStart = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHStr;
		pinfo->u16DispHPixelEnd = g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHEnd;

		if (g_pGOPCtxLocal->Gwin_H_Dup)
			pinfo->u16DispHPixelEnd =
			(pinfo->u16DispHPixelEnd + pinfo->u16DispHPixelStart) >> 1;
	} else {
		pinfo->u16DispHPixelStart = sGetInfo.u16DispHPixelStart;

		if (g_pGOPCtxLocal->Gwin_H_Dup) {
			pinfo->u16DispHPixelEnd =
			(sGetInfo.u16DispHPixelEnd + pinfo->u16DispHPixelStart) >> 1;
		} else {
			pinfo->u16DispHPixelEnd = sGetInfo.u16DispHPixelEnd;
		}
	}

	pinfo->u16RBlkHRblkSize = sGetInfo.u16RBlkHRblkSize;
	pinfo->u32DRAMRBlkStart = sGetInfo.u64DRAMRBlkStart;

	if (HAS_BIT(g_pGOPCtxLocal->u16GOP_HMirrorRBLK_Adr, u8win) ||
	    HAS_BIT(g_pGOPCtxLocal->u16GOP_VMirrorRBLK_Adr, u8win)) {
		pinfo->u32DRAMRBlkStart = g_pGOPCtxLocal->sMirrorInfo[u8win].u64NonMirrorFBAdr;
	}
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
	case E_GOP_DST_IP_MAIN:
		*pGopDst = E_DRV_GOP_DST_IP0;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	case E_GOP_DST_FRC:
		*pGopDst = E_DRV_GOP_DST_FRC;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	case E_GOP_DST_BYPASS:
		*pGopDst = E_DRV_GOP_DST_BYPASS;
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

static MS_U32 _GOP_Map_DRVDst2API_Enum_(void *pInstance, EN_GOP_DST_TYPE *pGopDst,
					DRV_GOPDstType GopDst)
{
	MS_U32 ret = 0;

	switch (GopDst) {
	case E_DRV_GOP_DST_IP0:
		*pGopDst = E_GOP_DST_IP0;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	case E_DRV_GOP_DST_OP0:
		*pGopDst = E_GOP_DST_OP0;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	case E_DRV_GOP_DST_FRC:
		*pGopDst = E_GOP_DST_FRC;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	case E_DRV_GOP_DST_BYPASS:
		*pGopDst = E_GOP_DST_BYPASS;
		ret = (MS_U32) GOP_API_SUCCESS;
		break;
	default:
		*pGopDst = (EN_GOP_DST_TYPE) 0xff;
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


static MS_BOOL bCursorSupport(void *pInstance, MS_U8 u8GOP)
{
#if CURSOR_SUPPORT
	//if cursor GOP, dont need to do adjust
	if (u8GOP == 3)
		return TRUE;
	else
		return FALSE;
#else
	return FALSE;
#endif
}

// Alignment stretch window value
static void _GOP_GWIN_Align_StretchWin(void *pInstance, MS_U8 u8GOP, EN_GOP_DST_TYPE eDstType,
				       MS_U16 *pu16x, MS_U16 *pu16y, MS_U16 *pu16Width,
				       MS_U16 *pu16height, MS_U16 BPP)
{
	MS_U16 u16align_offset;
	MS_U16 u16GOP_Unit;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	u16GOP_Unit = MDrv_GOP_GetWordUnit(g_pGOPCtxLocal, u8GOP);

	if (eDstType == E_GOP_DST_IP0) {
		MS_BOOL bInterlace = IsScalerOutputInterlace();

		if (bInterlace) {
			*pu16height /= 2;
			*pu16y = *pu16y / 2;
		}
	} else if (((eDstType == E_GOP_DST_OP0)
		|| (eDstType == E_GOP_DST_FRC)
		 || (eDstType == E_GOP_DST_BYPASS))
		&& !MDrv_GOP_GWIN_IsProgressive(g_pGOPCtxLocal, u8GOP)) {
		*pu16height /= 2;
		*pu16y = *pu16y / 2;
	}


	if (u16GOP_Unit == 1) {
		*pu16Width = ((*pu16Width + 1) >> 1) << 1;
	} else {
		u16align_offset = (u16GOP_Unit * 8 / BPP - 1);
		*pu16Width = (*pu16Width + u16align_offset) & (~u16align_offset);
	}
}

static void _GOP_GWIN_SetHVMirrorWinPos(void *pInstance, MS_U8 u8GwinID, HMirror_Opt opt,
					GOP_GwinInfo *pGWINInfo)
{
	MS_U16 u16GWINWidth = 0, u16GWINHeight = 0;
	MS_S16 s16MirrorHStr, s16MirrorVStr;
	MS_U8 u8Gop;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, u8GwinID)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, u8GwinID);
		return;
	}
	u8Gop = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, u8GwinID);
	u16GWINWidth = pGWINInfo->u16DispHPixelEnd - pGWINInfo->u16DispHPixelStart;
	u16GWINHeight = pGWINInfo->u16DispVPixelEnd - pGWINInfo->u16DispVPixelStart;

	switch (opt) {
	case E_Set_HPos:
		{
			pGWINInfo->u16DispHPixelEnd =
			    g_pGOPCtxLocal->pGOPCtxShared->u16APIStretchWidth[u8Gop] -
			    pGWINInfo->u16DispHPixelStart;
			s16MirrorHStr = pGWINInfo->u16DispHPixelEnd - u16GWINWidth;
			if (s16MirrorHStr < 0) {
				pGWINInfo->u16DispHPixelStart = 0;
				pGWINInfo->u16DispHPixelEnd = u16GWINWidth;
			} else {
				pGWINInfo->u16DispHPixelStart = s16MirrorHStr;
			}
			SET_BIT(g_pGOPCtxLocal->u16GOP_HMirror_HPos, u8GwinID);
			break;
		}
	case E_Set_VPos:
		{
			pGWINInfo->u16DispVPixelEnd =
			    g_pGOPCtxLocal->pGOPCtxShared->u16APIStretchHeight[u8Gop] -
			    pGWINInfo->u16DispVPixelStart;
			s16MirrorVStr = pGWINInfo->u16DispVPixelEnd - u16GWINHeight;
			if (s16MirrorVStr < 0) {
				pGWINInfo->u16DispVPixelStart = 0;
				pGWINInfo->u16DispVPixelEnd = u16GWINHeight;
			} else {
				pGWINInfo->u16DispVPixelStart = s16MirrorVStr;
			}
			SET_BIT(g_pGOPCtxLocal->u16GOP_VMirror_VPos, u8GwinID);
			break;
		}
	case E_Set_X:
		{
			if (u8Gop == GOPTYPE.GOP0) {
#if ENABLE_GOP0_RBLK_MIRROR
				MS_U16 bpp =
				    MDrv_GOP_GetBPP(g_pGOPCtxLocal,
						    (DRV_GOPColorType) pGWINInfo->clrType);
				MS_U16 u16HPixelSize;

				if ((bpp != 0) && (bpp != 0xFFFF))
					u16HPixelSize = pGWINInfo->u16RBlkHRblkSize / (bpp >> 3);
				else
					u16HPixelSize = pGWINInfo->u16RBlkHPixSize;

				if ((u16HPixelSize != 0)) {
					pGWINInfo->u16WinX =
					    u16HPixelSize -
					    ((pGWINInfo->u16WinX + pGWINInfo->u16DispHPixelEnd -
					      pGWINInfo->u16DispHPixelStart) % u16HPixelSize);
					pGWINInfo->u16WinX %= u16HPixelSize;
				} else
#endif				//ENABLE_GOP0_RBLK_MIRROR
				{
					pGWINInfo->u16WinX = 0;
				}
			}
			break;
		}
	case E_Set_Y:
		{
			if (u8Gop == GOPTYPE.GOP0) {
#if ENABLE_GOP0_RBLK_MIRROR
				if (pGWINInfo->u16RBlkVPixSize != 0) {
					pGWINInfo->u16WinY =
					    (pGWINInfo->u16WinY +
					     (pGWINInfo->u16DispVPixelEnd -
					      pGWINInfo->u16DispVPixelStart -
					      1)) % pGWINInfo->u16RBlkVPixSize;
				} else
#endif				//ENABLE_GOP0_RBLK_MIRROR
				{
					pGWINInfo->u16WinY =
					    (pGWINInfo->u16DispVPixelEnd -
					     pGWINInfo->u16DispVPixelStart - 1);
				}
			}
			break;
		}
	default:
		{
			//ASSERT(0);
			break;
		}
	}
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
		GOP_M_INFO("gwinMap[gId].u32GOPClientId: gId =%d\n",
			   (ptrdiff_t)(g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32GOPClientId));
		GOP_M_INFO("_pGOPCtxLocal->u32GOPClientId =%d\n",
			   (ptrdiff_t) (g_pGOPCtxLocal->u32GOPClientId));
		GOP_M_INFO("gwinMap[gId].u32CurFBId =%d\n",
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
	    || (eGopDst == E_DRV_GOP_DST_BYPASS)
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

	memset(g_pGOPCtxLocal->sMirrorInfo, 0,
	       g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum * sizeof(GOP_GwinMirror_Info));

	memset(&g_pGOPCtxLocal->sGOPConfig, 0, sizeof(GOP_Config));

	memset(g_pGOPCtxLocal->MS_MuxGop, 0, MAX_GOP_MUX_SUPPORT * sizeof(MS_U8));
	g_pGOPCtxLocal->IsChgMux = FALSE;
	g_pGOPCtxLocal->IsClkClosed = FALSE;
	g_pGOPCtxLocal->u8ChgIpMuxGop = 0xFF;

	return TRUE;
}

static MS_BOOL _GOP_GWIN_SetHVMirrorDRAMAddr(void *pInstance, MS_U8 u8win, MS_BOOL bHMirror,
					     MS_BOOL bVMirror, GOP_GwinInfo *pinfo)
{
	MS_PHY u64AddrTemp = 0;
	MS_U8 u8Gop = 0;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	u8Gop = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, u8win);
	if (!_GOP_IsGwinIdValid(pInstance, u8win)) {
		GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__, __LINE__, u8win);
		return FALSE;
	}

	if (pinfo == NULL) {
		GOP_M_ERR("[%s][%d]pinfo  is NULL\n", __func__, __LINE__);
		return FALSE;
	}

	if (u8Gop != GOPTYPE.GOP0) {
		if (bHMirror) {
			if (!g_pGOPCtxLocal->pGopChipProperty->bAutoAdjustMirrorHSize) {
				u64AddrTemp =
				    pinfo->u32DRAMRBlkStart + (MS_U32) pinfo->u16RBlkHRblkSize;

				if (u64AddrTemp >= MDrv_GOP_GetWordUnit(g_pGOPCtxLocal, u8Gop)) {
					if (MDrv_GOP_GetWordUnit(g_pGOPCtxLocal, u8Gop) != 1) {
						pinfo->u32DRAMRBlkStart = u64AddrTemp -
							MDrv_GOP_GetWordUnit(g_pGOPCtxLocal, u8Gop);
					} else {
						pinfo->u32DRAMRBlkStart = u64AddrTemp;
					}
				} else {
					pinfo->u32DRAMRBlkStart = u64AddrTemp;
				}

				if (pinfo->u16RBlkHRblkSize >
					(pinfo->u16DispHPixelEnd *
					_GOP_GetBPP(pInstance, pinfo->clrType) / 8)) {
					pinfo->u32DRAMRBlkStart -=
					    ((MS_PHY) (pinfo->u16RBlkHRblkSize) -
					     ((MS_PHY) (pinfo->u16DispHPixelEnd) *
					      (MS_PHY) (_GOP_GetBPP(pInstance, pinfo->clrType) /
							8)));
				}
			} else {
				if (pinfo->u16DispHPixelEnd < pinfo->u16DispHPixelStart)	 {
					GOP_M_ERR
					    ("[%s][%d] (HEnd-HStart) <0!\n",
					     __func__, __LINE__);
					return FALSE;
				}
				if (pinfo->u16RBlkHRblkSize >
					((pinfo->u16DispHPixelEnd - pinfo->u16DispHPixelStart) *
					_GOP_GetBPP(pInstance, pinfo->clrType) / 8UL)) {
					pinfo->u32DRAMRBlkStart =
					    pinfo->u32DRAMRBlkStart -
					    (((MS_U32) pinfo->u16RBlkHRblkSize & 0xFFFF) -
					     (((MS_U32)
					       ((pinfo->u16DispHPixelEnd -
						 pinfo->u16DispHPixelStart) & 0xFFFF)) *
					      ((MS_U32)
					       (_GOP_GetBPP(pInstance, pinfo->clrType) & 0xFFFF)) /
					      8UL));
				}
			}
			SET_BIT(g_pGOPCtxLocal->u16GOP_HMirrorRBLK_Adr, u8win);
		}
		if (bVMirror) {
			//reg_dram_rblk_str += reg_rblk_hsize * (reg_gwin_vend - reg_gwin_vstr)
			if (!g_pGOPCtxLocal->pGOPCtxShared->b32TileMode[u8Gop]) {
				//Warning message
				if (pinfo->u16RBlkVPixSize !=
				    (pinfo->u16DispVPixelEnd - pinfo->u16DispVPixelStart)) {
					GOP_M_WARN
						     ("RBlkSize:0x%x not match (VE:-VSt:0x%x)\n",
						      pinfo->u16RBlkVPixSize,
						      pinfo->u16DispVPixelEnd,
						      pinfo->u16DispVPixelStart);
				}
				pinfo->u32DRAMRBlkStart +=
				    ((MS_U32) pinfo->u16RBlkHRblkSize *
				     ((MS_U32) pinfo->u16DispVPixelEnd -
				      (MS_U32) pinfo->u16DispVPixelStart - 1));
				SET_BIT(g_pGOPCtxLocal->u16GOP_VMirrorRBLK_Adr, u8win);
			}
		}
	}
	return TRUE;
}

MS_BOOL GOP_GetWinInfo(void *pInstance, MS_U32 u32win, GOP_GwinInfo *pinfo)
{
	MS_U8 u8GOP;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGwinIdValid(pInstance, u32win)) {
		GOP_M_ERR("[%s][%d]GWIN %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) u32win);
		return FALSE;
	}
	memset(pinfo, 0, sizeof(GOP_GwinInfo));
	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, u32win);

	if (u8GOP == GOPTYPE.GOP0)
		_GOP_GetGop0WinInfo(pInstance, u32win, pinfo);
	else if (u8GOP == GOPTYPE.GOP1)
		_GOP_GetGop1WinInfo(pInstance, u32win, pinfo);
	else
		_GOP_GetGop23WinInfo(pInstance, u32win, pinfo);

	return TRUE;
}

static void _SetGop0WinInfo(void *pInstance, MS_U8 u8win, GOP_GwinInfo *pinfo)
{
	DRV_GOPDstType Gop0Dst = E_DRV_GOP_DST_INVALID;
	MS_BOOL bInterlace = FALSE;
	MS_BOOL bHMirror = FALSE, bVMirror = FALSE;
	MS_BOOL bRIUHMirror = FALSE, bRIUVMirror = FALSE;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, E_GOP0, &Gop0Dst);

	if (Gop0Dst == E_DRV_GOP_DST_INVALID) {
		APIGOP_ASSERT(FALSE, GOP_M_FATAL("\n Get GOP destination fail!\n"));
		return;
	}

	_GOP_GWIN_IsEnableMirror(pInstance, Gop0Dst, &bHMirror, &bVMirror);
	bInterlace = IsScalerOutputInterlace();

	if ((Gop0Dst == E_DRV_GOP_DST_OP0) ||
		(Gop0Dst == E_DRV_GOP_DST_FRC) ||
		(Gop0Dst == E_DRV_GOP_DST_BYPASS)) {

		if ((g_pGOPCtxLocal->pGopChipProperty->bOpInterlace == TRUE)
		    && (Gop0Dst == E_DRV_GOP_DST_OP0)) {
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP0, FALSE);
		} else {
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP0, TRUE);
		}

	} else if (Gop0Dst == E_DRV_GOP_DST_IP0) {
		if (bInterlace)
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP0, FALSE);
		else
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP0, TRUE);

	} else {
		MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP0, FALSE);
	}

	if (g_pGOPCtxLocal->pGOPCtxShared->bHMirror || g_pGOPCtxLocal->pGOPCtxShared->bVMirror)
		g_pGOPCtxLocal->sMirrorInfo[u8win].u64NonMirrorFBAdr = pinfo->u32DRAMRBlkStart;

	MDrv_GOP_IsGOPMirrorEnable(g_pGOPCtxLocal, E_GOP0, &bRIUHMirror, &bRIUVMirror);

	if (bHMirror) {
		/*save gwin postion before h mirror setting */
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHStr = pinfo->u16DispHPixelStart;
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHEnd = pinfo->u16DispHPixelEnd;
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorGOP0WinX = pinfo->u16WinX;

		MDrv_GOP_GWIN_EnableHMirror(g_pGOPCtxLocal, E_GOP0, TRUE);
		_GOP_GWIN_SetHVMirrorWinPos(pInstance, u8win, E_Set_HPos, pinfo);
		_GOP_GWIN_SetHVMirrorWinPos(pInstance, u8win, E_Set_X, pinfo);
	} else {
		//System is mirror, but bHMirror=FALSE, so need GOP mirror off
		if (g_pGOPCtxLocal->pGOPCtxShared->bHMirror || (bRIUHMirror == TRUE))
			MDrv_GOP_GWIN_EnableHMirror(g_pGOPCtxLocal, E_GOP0, FALSE);
	}
	if (!
	    (g_pGOPCtxLocal->pGOPCtxShared->bHMirror
	     && ((Gop0Dst == E_DRV_GOP_DST_OP0) || (Gop0Dst == E_DRV_GOP_DST_FRC)
		 || (Gop0Dst == E_DRV_GOP_DST_BYPASS)))) {
		if (g_pGOPCtxLocal->Gwin_H_Dup)
			pinfo->u16DispHPixelEnd =
			    (pinfo->u16DispHPixelEnd << 1) - pinfo->u16DispHPixelStart;
	}

	if (bVMirror) {
		/*save gwin postion before V mirror setting */
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVStr = pinfo->u16DispVPixelStart;
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVEnd = pinfo->u16DispVPixelEnd;
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorGOP0WinY = pinfo->u16WinY;

		MDrv_GOP_GWIN_EnableVMirror(g_pGOPCtxLocal, E_GOP0, TRUE);
		//SET_BIT(g_pGOPCtxLocal->u16GOP_VMirrorRBLK_Adr, u8win);
		_GOP_GWIN_SetHVMirrorWinPos(pInstance, u8win, E_Set_VPos, pinfo);
		_GOP_GWIN_SetHVMirrorWinPos(pInstance, u8win, E_Set_Y, pinfo);
	} else {
		if (g_pGOPCtxLocal->pGOPCtxShared->bVMirror || (bRIUVMirror == TRUE))
			MDrv_GOP_GWIN_EnableVMirror(g_pGOPCtxLocal, E_GOP0, FALSE);
	}

	//if (!(g_pGOPCtxLocal->pGOPCtxShared->bVMirror))
	{
		if (g_pGOPCtxLocal->Gwin_V_Dup) {
			if ((Gop0Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    pinfo->u16DispVPixelStart +
				    (((pinfo->u16DispVPixelEnd << 1) -
				      pinfo->u16DispVPixelStart) / 2);
			else
				pinfo->u16DispVPixelEnd =
				    (pinfo->u16DispVPixelEnd << 1) - pinfo->u16DispVPixelStart;
		} else {
			if ((Gop0Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    pinfo->u16DispVPixelStart + (pinfo->u16DispVPixelEnd -
								 pinfo->u16DispVPixelStart) / 2;
			else if (((Gop0Dst == E_DRV_GOP_DST_OP0) || (Gop0Dst == E_DRV_GOP_DST_FRC)
				  || (Gop0Dst == E_DRV_GOP_DST_BYPASS))
				 && !MDrv_GOP_GWIN_IsProgressive(g_pGOPCtxLocal, 0)) {
				pinfo->u16DispVPixelStart = pinfo->u16DispVPixelStart / 2;
				pinfo->u16DispVPixelEnd = pinfo->u16DispVPixelEnd / 2;
			}
		}
	}

	MDrv_GOP_GWIN_SetWinFmt(g_pGOPCtxLocal, u8win, (DRV_GOPColorType) pinfo->clrType);
	//MDrv_GOP_GWIN_UpdateRegOnce(g_pGOPCtxLocal, FALSE);
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
	DRV_GOPDstType Gop1Dst = E_DRV_GOP_DST_INVALID;
	MS_BOOL bInterlace = FALSE;
	MS_BOOL bHMirror = FALSE, bVMirror = FALSE;
	MS_BOOL bRIUHMirror = FALSE, bRIUVMirror = FALSE;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, E_GOP1, &Gop1Dst);
	if (Gop1Dst == E_DRV_GOP_DST_INVALID) {
		APIGOP_ASSERT(FALSE, GOP_M_FATAL("\n Get GOP destination fail!\n"));
		return;
	}
	_GOP_GWIN_IsEnableMirror(pInstance, Gop1Dst, &bHMirror, &bVMirror);
	bInterlace = IsScalerOutputInterlace();

	if ((Gop1Dst == E_DRV_GOP_DST_OP0) ||
		(Gop1Dst == E_DRV_GOP_DST_FRC) ||
		(Gop1Dst == E_DRV_GOP_DST_BYPASS)) {

		if ((g_pGOPCtxLocal->pGopChipProperty->bOpInterlace == TRUE)
		    && (Gop1Dst == E_DRV_GOP_DST_OP0)) {
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP1, FALSE);
		} else {
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP1, TRUE);
		}
	} else if (Gop1Dst == E_DRV_GOP_DST_IP0) {

		if (bInterlace)
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP1, FALSE);
		else
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP1, TRUE);
	} else {
		MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, E_GOP1, FALSE);

	}

	if (g_pGOPCtxLocal->pGOPCtxShared->bHMirror || g_pGOPCtxLocal->pGOPCtxShared->bVMirror) {
		g_pGOPCtxLocal->sMirrorInfo[u8win].u64NonMirrorFBAdr = pinfo->u32DRAMRBlkStart;
		_GOP_GWIN_SetHVMirrorDRAMAddr(pInstance, u8win, bHMirror, bVMirror, pinfo);
	}

	MDrv_GOP_IsGOPMirrorEnable(g_pGOPCtxLocal, E_GOP1, &bRIUHMirror, &bRIUVMirror);

	if (bHMirror) {
		/*save gwin postion before h mirror setting */
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHStr = pinfo->u16DispHPixelStart;
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHEnd = pinfo->u16DispHPixelEnd;

		MDrv_GOP_GWIN_EnableHMirror(g_pGOPCtxLocal, E_GOP1, TRUE);
		_GOP_GWIN_SetHVMirrorWinPos(pInstance, u8win, E_Set_HPos, pinfo);
	} else {
		if ((g_pGOPCtxLocal->pGOPCtxShared->bHMirror || (bRIUHMirror == TRUE)))
			MDrv_GOP_GWIN_EnableHMirror(g_pGOPCtxLocal, E_GOP1, FALSE);
	}

	if (bVMirror) {
		/*save gwin postion before V mirror setting */
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVStr = pinfo->u16DispVPixelStart;
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVEnd = pinfo->u16DispVPixelEnd;

		MDrv_GOP_GWIN_EnableVMirror(g_pGOPCtxLocal, E_GOP1, TRUE);
		_GOP_GWIN_SetHVMirrorWinPos(pInstance, u8win, E_Set_VPos, pinfo);
	} else {
		if ((g_pGOPCtxLocal->pGOPCtxShared->bVMirror || (bRIUVMirror == TRUE)))
			MDrv_GOP_GWIN_EnableVMirror(g_pGOPCtxLocal, E_GOP1, FALSE);
	}

	//if (!g_pGOPCtxLocal->pGOPCtxShared->bVMirror)
	{
		if (g_pGOPCtxLocal->Gwin_V_Dup) {
			if ((Gop1Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    pinfo->u16DispVPixelStart +
				    (((pinfo->u16DispVPixelEnd << 1) -
				      pinfo->u16DispVPixelStart) / 2);
			else
				pinfo->u16DispVPixelEnd =
				    (pinfo->u16DispVPixelEnd << 1) - pinfo->u16DispVPixelStart;
		} else {
			if ((Gop1Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    pinfo->u16DispVPixelStart + (pinfo->u16DispVPixelEnd -
								 pinfo->u16DispVPixelStart) / 2;
			else if (((Gop1Dst == E_DRV_GOP_DST_OP0) || (Gop1Dst == E_DRV_GOP_DST_FRC)
				  || (Gop1Dst == E_DRV_GOP_DST_BYPASS))
				 && !MDrv_GOP_GWIN_IsProgressive(g_pGOPCtxLocal, 1)) {
				pinfo->u16DispVPixelStart = pinfo->u16DispVPixelStart / 2;
				pinfo->u16DispVPixelEnd = pinfo->u16DispVPixelEnd / 2;
			}
		}
	}

	MDrv_GOP_GWIN_SetWinFmt(g_pGOPCtxLocal, u8win, (DRV_GOPColorType) pinfo->clrType);
	MDrv_GOP_GWIN_SetGwinInfo(g_pGOPCtxLocal, u8win, (DRV_GOP_GWIN_INFO *) pinfo);
}

static void _SetGop23WinInfo(void *pInstance, MS_U8 u8win, GOP_GwinInfo *pinfo)
{
	DRV_GOPDstType Gop23Dst = E_DRV_GOP_DST_INVALID;
	MS_BOOL bInterlace = FALSE;
	MS_BOOL bHMirror = FALSE, bVMirror = FALSE;
	MS_BOOL bRIUHMirror = FALSE, bRIUVMirror = FALSE;
	MS_U8 u8GOP = 0;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, u8win);
	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, u8GOP, &Gop23Dst);
	if (Gop23Dst == E_DRV_GOP_DST_INVALID) {
		APIGOP_ASSERT(FALSE, GOP_M_FATAL("\n Get GOP destination fail!\n"));
		return;
	}

	_GOP_GWIN_IsEnableMirror(pInstance, Gop23Dst, &bHMirror, &bVMirror);
	bInterlace = IsScalerOutputInterlace();

	if ((Gop23Dst == E_DRV_GOP_DST_OP0) ||
		(Gop23Dst == E_DRV_GOP_DST_FRC) ||
		(Gop23Dst == E_DRV_GOP_DST_BYPASS)) {
		if ((g_pGOPCtxLocal->pGopChipProperty->bOpInterlace == TRUE)
		    && (Gop23Dst == E_DRV_GOP_DST_OP0)) {
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, u8GOP, FALSE);
		} else {
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, u8GOP, TRUE);
		}
	} else if (Gop23Dst == E_DRV_GOP_DST_IP0) {
		if (bInterlace)
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, u8GOP, FALSE);
		else
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, u8GOP, TRUE);
	} else
		MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, u8GOP, FALSE);

	if (g_pGOPCtxLocal->pGOPCtxShared->bHMirror || g_pGOPCtxLocal->pGOPCtxShared->bVMirror) {
		g_pGOPCtxLocal->sMirrorInfo[u8win].u64NonMirrorFBAdr = pinfo->u32DRAMRBlkStart;
		_GOP_GWIN_SetHVMirrorDRAMAddr(pInstance, u8win, bHMirror, bVMirror, pinfo);
	}

	MDrv_GOP_IsGOPMirrorEnable(g_pGOPCtxLocal, u8GOP, &bRIUHMirror, &bRIUVMirror);

	if (bHMirror) {
		/*save gwin postion before h mirror setting */
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHStr = pinfo->u16DispHPixelStart;
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorHEnd = pinfo->u16DispHPixelEnd;

		MDrv_GOP_GWIN_EnableHMirror(g_pGOPCtxLocal, u8GOP, TRUE);

		if (!bCursorSupport(pInstance, u8GOP))
			_GOP_GWIN_SetHVMirrorWinPos(pInstance, u8win, E_Set_HPos, pinfo);
	} else {
		if ((g_pGOPCtxLocal->pGOPCtxShared->bHMirror || (bRIUHMirror == TRUE)))
			MDrv_GOP_GWIN_EnableHMirror(g_pGOPCtxLocal, u8GOP, FALSE);
	}

	if (bVMirror) {
		/*save gwin postion before V mirror setting */
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVStr = pinfo->u16DispVPixelStart;
		g_pGOPCtxLocal->sMirrorInfo[u8win].u16NonMirrorVEnd = pinfo->u16DispVPixelEnd;

		MDrv_GOP_GWIN_EnableVMirror(g_pGOPCtxLocal, u8GOP, TRUE);

		if (!bCursorSupport(pInstance, u8GOP))
			_GOP_GWIN_SetHVMirrorWinPos(pInstance, u8win, E_Set_VPos, pinfo);
	} else {
		if ((g_pGOPCtxLocal->pGOPCtxShared->bVMirror || (bRIUVMirror == TRUE)))
			MDrv_GOP_GWIN_EnableVMirror(g_pGOPCtxLocal, u8GOP, FALSE);
	}

	//if (!g_pGOPCtxLocal->pGOPCtxShared->bVMirror)
	{
		if (g_pGOPCtxLocal->Gwin_V_Dup) {
			if ((Gop23Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    pinfo->u16DispVPixelStart +
				    (((pinfo->u16DispVPixelEnd << 1) -
				      pinfo->u16DispVPixelStart) / 2);
			else
				pinfo->u16DispVPixelEnd =
				    (pinfo->u16DispVPixelEnd << 1) - pinfo->u16DispVPixelStart;
		} else {
			if ((Gop23Dst == E_DRV_GOP_DST_IP0) && bInterlace)
				pinfo->u16DispVPixelEnd =
				    pinfo->u16DispVPixelStart + (pinfo->u16DispVPixelEnd -
								 pinfo->u16DispVPixelStart) / 2;
			else if (((Gop23Dst == E_DRV_GOP_DST_OP0) || (Gop23Dst == E_DRV_GOP_DST_FRC)
				  || (Gop23Dst == E_DRV_GOP_DST_BYPASS))
				 && !MDrv_GOP_GWIN_IsProgressive(g_pGOPCtxLocal, u8GOP)) {
				pinfo->u16DispVPixelStart = pinfo->u16DispVPixelStart / 2;
				pinfo->u16DispVPixelEnd = pinfo->u16DispVPixelEnd / 2;
			}
		}
	}

	MDrv_GOP_GWIN_SetGwinInfo(g_pGOPCtxLocal, u8win, (DRV_GOP_GWIN_INFO *) pinfo);

	MDrv_GOP_GWIN_SetWinFmt(g_pGOPCtxLocal, u8win, (DRV_GOPColorType) pinfo->clrType);
}

static void _GOP_GetPDByDst(void *pInstance, MS_U8 u8GOP_num, DRV_GOPDstType GopDst,
			    MS_U16 *u16StrwinStr)
{
	MS_BOOL bHDREnable = FALSE;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (GopDst) {
	case E_DRV_GOP_DST_OP0:
		MDrv_GOP_IsHDREnabled(g_pGOPCtxLocal, &bHDREnable);
		if (bHDREnable == FALSE) {
			*u16StrwinStr =
			    g_pGOPCtxLocal->pGOPCtxShared->u16PnlHStr[u8GOP_num] +
			    g_pGOPCtxLocal->pGopChipProperty->GOP_PD;
		} else {
			*u16StrwinStr =
			    g_pGOPCtxLocal->pGOPCtxShared->u16PnlHStr[u8GOP_num] +
			    g_pGOPCtxLocal->pGopChipProperty->GOP_HDR_OP_PD;
		}
		break;
	case E_DRV_GOP_DST_FRC:
		*u16StrwinStr = 0;
		break;
	default:
		*u16StrwinStr = g_pGOPCtxLocal->pGopChipProperty->GOP_PD;
		break;
	}

}

static void _GOP_AdjustHSPD(void *pInstance, MS_U8 u8GOP_num, MS_U16 u16StrwinStr,
			    DRV_GOPDstType GopDst)
{
	MS_U16 u16Offset;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGopNumValid(pInstance, u8GOP_num)) {
		GOP_M_ERR("[%s][%d]GOP %d  is out of range\n", __func__, __LINE__, u8GOP_num);
		return;
	}

	switch (GopDst) {
	case E_DRV_GOP_DST_IP0:
		u16Offset = 0;
		break;
	case E_DRV_GOP_DST_FRC:
		u16Offset = MDrv_GOP_GetHPipeOfst(g_pGOPCtxLocal, u8GOP_num, GopDst);
		break;
	case E_DRV_GOP_DST_OP0:
	case E_DRV_GOP_DST_BYPASS:
	default:
		u16Offset = MDrv_GOP_GetHPipeOfst(g_pGOPCtxLocal, u8GOP_num, GopDst);
		break;
	}

	MDrv_GOP_GWIN_SetHSPipe(g_pGOPCtxLocal, u8GOP_num, u16Offset + u16StrwinStr);
}

static void _GOP_InitHSPDByGOP(void *pInstance, MS_U8 u8GOP_num)
{
	MS_U16 u16StrwinStr = 0;
	DRV_GOPDstType GopDst = E_DRV_GOP_DST_OP0;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGopNumValid(pInstance, u8GOP_num)) {
		GOP_M_ERR("[%s][%d]GOP %d  is out of range\n", __func__, __LINE__, u8GOP_num);
		return;
	}

	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, u8GOP_num, &GopDst);
	_GOP_GetPDByDst(pInstance, u8GOP_num, GopDst, &u16StrwinStr);
	_GOP_AdjustHSPD(pInstance, u8GOP_num, u16StrwinStr, GopDst);
}

static void _GOP_Get_StretchWin(void *pInstance, MS_U8 gop, MS_U16 *x, MS_U16 *y, MS_U16 *w,
				MS_U16 *h)
{
	DRV_GOPDstType eDstType = E_DRV_GOP_DST_INVALID;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif
	MDrv_GOP_GWIN_Get_StretchWin(g_pGOPCtxLocal, gop, x, y, w, h);
	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, gop, &eDstType);

	if (eDstType == E_DRV_GOP_DST_IP0) {
		MS_BOOL bInterlace = IsScalerOutputInterlace();

		if (bInterlace) {
			(*h) *= 2;
			(*y) = (*y) * 2;
		}
	}

	if ((eDstType == E_DRV_GOP_DST_OP0) || (eDstType == E_DRV_GOP_DST_FRC)
	    || (eDstType == E_DRV_GOP_DST_BYPASS)) {
		if (!MDrv_GOP_GWIN_IsProgressive(g_pGOPCtxLocal, gop)) {
			(*h) *= 2;
			(*y) = (*y) * 2;
		}
	}
}

static void _GOP_SetOCCapability(void *pInstance, MS_U8 u8GOP, DRV_GOPDstType GopDst)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (!_GOP_IsGopNumValid(pInstance, u8GOP)) {
		GOP_M_ERR("[%s][%d]GOP %d  is out of range\n", __func__, __LINE__, u8GOP);
		return;
	}

	switch (GopDst) {
	case E_DRV_GOP_DST_IP0:
		MDrv_GOP_GWIN_SetAlphaInverse(g_pGOPCtxLocal, u8GOP, FALSE);
		MDrv_GOP_OC_SetOCEn(g_pGOPCtxLocal, u8GOP, FALSE);
		break;
	case E_DRV_GOP_DST_OP0:
	case E_DRV_GOP_DST_BYPASS:
		MDrv_GOP_GWIN_SetAlphaInverse(g_pGOPCtxLocal, u8GOP, TRUE);
		MDrv_GOP_OC_SetOCEn(g_pGOPCtxLocal, u8GOP, FALSE);
		break;

	case E_DRV_GOP_DST_FRC:
		MDrv_GOP_GWIN_SetAlphaInverse(g_pGOPCtxLocal, u8GOP, TRUE);
		MDrv_GOP_OC_SetOCEn(g_pGOPCtxLocal, u8GOP, TRUE);
		break;

	default:
		break;
	}

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

	MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, u8GOP);

	return GOP_API_SUCCESS;
}

static MS_U32 GOP_Win_Destroy(void *pInstance, MS_U32 gId)
{
	MS_U32 u32fbId;
	GOP_GwinInfo gWin;
	GOP_WinFB_INFO *pwinFB;
	MS_U8 u8GOP;
	MS_PHY phyTempAddr = 0;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	GOP_M_INFO("\33[0;36m   %s:%d   gId = %td \33[m\n", __func__, __LINE__,
		   (ptrdiff_t) gId);

	if (!_GOP_IsGwinIdValid(pInstance, gId)) {
		GOP_M_ERR("[%s][%d]GWIN %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) gId);
		return GOP_API_FAIL;
	}
	if (!_GOP_GWIN_IsGwinExistInClient(pInstance, gId)) {
		GOP_M_ERR("[%s][%d]GWIN %td  is not exist\n", __func__, __LINE__,
			  (ptrdiff_t) gId);
		return GOP_API_FAIL;
	}

	MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, gId, FALSE);
	u32fbId = g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId;

	pwinFB = _GetWinFB(pInstance, u32fbId);
	if (!_GOP_IsFbIdValid(pInstance, u32fbId)) {
		GOP_M_ERR("[%s][%d]FbId %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GOP_API_FAIL;
	}

	if ((pwinFB->poolId != GOP_WINFB_POOL_NULL)
	    && (GOP_FB_Destroy(pInstance, u32fbId) != GWIN_OK)) {
		GOP_M_ERR
			     ("[%s][%d], failed to delete FB %td\n", __func__, __LINE__,
			      (ptrdiff_t) u32fbId);

	}

	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId = GWIN_ID_INVALID;
	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32GOPClientId = 0;
	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].bIsShared = FALSE;
	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u16SharedCnt = 0;

	memset(&gWin, 0, sizeof(GOP_GwinInfo));
	gWin.clrType = E_GOP_COLOR_ARGB8888;
	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, (gId));
	gWin.u32DRAMRBlkStart = phyTempAddr;
	GOP_SetWinInfo(pInstance, gId, &gWin);

	RESET_BIT(g_pGOPCtxLocal->u16GOP_VMirrorRBLK_Adr, gId);
	RESET_BIT(g_pGOPCtxLocal->u16GOP_HMirrorRBLK_Adr, gId);
	RESET_BIT(g_pGOPCtxLocal->u16GOP_HMirror_HPos, gId);
	RESET_BIT(g_pGOPCtxLocal->u16GOP_VMirror_VPos, gId);

	return GOP_API_SUCCESS;
}

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
static void GOP_AtExit(void *pInstance)
{
	MS_U32 u32TempID = 0, u32GWinFBID;
	MS_U32 u32CurPID = (GETPIDTYPE) getpid();
	E_GOP_API_Result enRet = GOP_API_FAIL;
	GOP_WinFB_INFO *pwinFB;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (g_pGOPCtxLocal == NULL)
		return;

	GOP_M_INFO("---%s %d: PID[%tu], TID[%td] exited\n", __func__, __LINE__,
		   (ptrdiff_t) u32CurPID, (ptrdiff_t) MsOS_GetOSThreadID());

	for (u32TempID = 0;
	u32TempID < g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum;
	u32TempID++) {
		//Check and release APP's GWIN resource
		if (((g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u32TempID].bIsShared
			== FALSE)
		     && (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u32TempID].u32CurFBId <
			 DRV_MAX_GWIN_FB_SUPPORT)
		     && (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u32TempID].u32GOPClientId ==
			 u32CurPID))
		    || ((g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u32TempID].bIsShared
		    == TRUE)
			&& (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u32TempID].u16SharedCnt
			== 0x0))) {
			u32GWinFBID = g_pGOPCtxLocal->pGOPCtxShared->gwinMap[u32TempID].u32CurFBId;
			enRet = (E_GOP_API_Result) GOP_Win_Destroy(pInstance, u32TempID);
			GOP_M_INFO("\n---%s %d: Try DestroyWIN[%tu], return[%u]\n", __func__,
				   __LINE__, (ptrdiff_t) u32TempID, (MS_U8) enRet);
			enRet = (E_GOP_API_Result) GOP_FB_Destroy(pInstance, u32GWinFBID);
			GOP_M_INFO("\n---%s %d: Try DestroyWIN's FB[%tu], return[%u]\n",
				   __func__, __LINE__, (ptrdiff_t) u32GWinFBID, (MS_U8) enRet);
		}
	}

	for (u32TempID = 0; u32TempID < DRV_MAX_GWIN_FB_SUPPORT; u32TempID++) {
		pwinFB = _GetWinFB(pInstance, u32TempID);
		if ((pwinFB->u32GOPClientId == u32CurPID)
		    && (pwinFB->gWinId == GWIN_ID_INVALID)) {
			enRet = (E_GOP_API_Result) GOP_Win_Destroy(pInstance, u32TempID);
			GOP_M_INFO("\n---%s %d: Try DestroyFB[%tu], return[%u]\n", __func__,
				   __LINE__, (ptrdiff_t) u32TempID, (MS_U8) enRet);
		}
	}
}
#endif

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

	memset(g_pGOPCtxLocal->sMirrorInfo, 0,
	       g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum * sizeof(GOP_GwinMirror_Info));

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
		msWarning(ERR_FB_ID_OUT_OF_RANGE);
		GOP_M_ERR("[%s][%d]: fbId max already reached! FBId: %td, winId: %td\n",
			  __func__, __LINE__, (ptrdiff_t) u32fbId, (ptrdiff_t) GwinId);
		return GOP_API_FAIL;
	}

	if (!_GOP_IsGwinIdValid(pInstance, GwinId)) {
		GOP_M_ERR("\n[%s][%d] not support gwin id:%td in this chip version", __func__,
			  __LINE__, (ptrdiff_t) GwinId);
		return GOP_API_FAIL;
	}
	memset(&gWin, 0, sizeof(GOP_GwinInfo));
	pwinFB = _GetWinFB(pInstance, u32fbId);

	if (pwinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GOP_API_FAIL;
	}
	//Add support to Map a Non-Created Gwin with a dedicated FB:
	if (!_GOP_GWIN_IsGwinCreated(pInstance, GwinId)) {
		//Create GWin for The client:
		g_pGOPCtxLocal->pGOPCtxShared->gwinMap[GwinId].u32CurFBId = u32fbId;

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
		g_pGOPCtxLocal->pGOPCtxShared->gwinMap[GwinId].u32GOPClientId =
		    (GETPIDTYPE) getpid();
#else
		g_pGOPCtxLocal->pGOPCtxShared->gwinMap[GwinId].u32GOPClientId =
		    g_pGOPCtxLocal->u32GOPClientId;
#endif

		MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, GwinId, FALSE);

		pwinFB->enable = TRUE;
	}
	u32Temp = g_pGOPCtxLocal->pGOPCtxShared->gwinMap[GwinId].u32CurFBId;
	if (u32Temp < DRV_MAX_GWIN_FB_SUPPORT) {
		pwinFB2 = _GetWinFB(pInstance, u32Temp);

		if (pwinFB2 == NULL) {
			GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__, __LINE__,
				  (ptrdiff_t) u32Temp);
			return GOP_API_FAIL;
		}

		msWarning(ERR_GWIN_ID_ALREADY_MAPPED);
		GOP_M_INFO
		    ("Warning: [%s][%d]: u8gwinId already assigned to fbId2(%td), fbId(%td)\n",
		     __func__, __LINE__, (ptrdiff_t) u32Temp, (ptrdiff_t) u32fbId);
		pwinFB2->gWinId = GWIN_ID_INVALID;
	}

	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[GwinId].u32CurFBId = u32fbId;

	if (!_GOP_GWIN_IsGwinExistInClient(pInstance, GwinId)) {
		msWarning(ERR_GWIN_ID_OUT_OF_RANGE);
		GOP_M_ERR("[%s][%d]: winId=%td is not in existence\n", __func__, __LINE__,
			  (ptrdiff_t) GwinId);
		return GOP_API_FAIL;
	}

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


	GOP_SetWinInfo(pInstance, GwinId, &gWin);

	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, GwinId);
#ifdef CONFIG_GOP_AFBC_FEATURE
	if (g_pGOPCtxLocal->pGopChipProperty->bAFBC_Support[u8GOP] == TRUE) {
		_GOP_MAP_FBID_AFBCCmd(pInstance, u32fbId, &u8AFBC_cmd, &bAFBC_Enable);

		sAFBCWinProperty.u16HPixelStart = gWin.u16DispHPixelStart;
		sAFBCWinProperty.u16HPixelEnd = gWin.u16DispHPixelEnd;
		sAFBCWinProperty.u16Pitch = gWin.u16RBlkHRblkSize;
		sAFBCWinProperty.u16VPixelStart = gWin.u16DispVPixelStart;
		sAFBCWinProperty.u16VPixelEnd = gWin.u16DispVPixelEnd;
		sAFBCWinProperty.u64DRAMAddr = gWin.u32DRAMRBlkStart;
		sAFBCWinProperty.u8Fmt = u8AFBC_cmd;
		MDrv_GOP_GWIN_AFBCSetWindow(g_pGOPCtxLocal, u8GOP, &sAFBCWinProperty, TRUE);

		MDrv_GOP_GWIN_AFBCMode(g_pGOPCtxLocal, u8GOP, bAFBC_Enable, u8AFBC_cmd);
	}
#endif
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
	MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, u8GOP);
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
	MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, u8GOP);
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

MS_U32 GOP_SetOutputColor(void *pInstance, MS_U8 u8GOP, EN_GOP_OUTPUT_COLOR type)
{

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	g_pGOPCtxLocal->pGOPCtxShared->s32OutputColorType[u8GOP] = (MS_S32) type;

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
		msWarning(ERR_FB_ID_ALREADY_ALLOCATED);
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


MS_U32 GOP_Win_Destroy_Abnormal(void *pInstance, MS_U32 gId)
{
	MS_U32 u32fbId;
	GOP_GwinInfo gWin;
	GOP_WinFB_INFO *pwinFB;
	MS_U8 u8GOP;
	MS_PHY phyTempAddr = 0;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	GOP_M_INFO("\33[0;36m   %s:%d   gId = %td \33[m\n", __func__, __LINE__,
		   (ptrdiff_t) gId);

	if (!_GOP_IsGwinIdValid(pInstance, gId)) {
		GOP_M_ERR("[%s][%d]GWIN %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) gId);
		return GOP_API_FAIL;
	}

	MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, gId, FALSE);
	u32fbId = g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId;

	pwinFB = _GetWinFB(pInstance, u32fbId);
	if (!_GOP_IsFbIdValid(pInstance, u32fbId)) {
		GOP_M_ERR("[%s][%d]FbId %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GOP_API_FAIL;
	}

	if ((pwinFB->poolId != GOP_WINFB_POOL_NULL)
	    && (GOP_FB_Destroy(pInstance, u32fbId) != GWIN_OK)) {
		GOP_M_INFO
			     ("[%s][%d], failed to delete FB %td\n", __func__, __LINE__,
			      (ptrdiff_t) u32fbId);

	}

	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId = GWIN_ID_INVALID;
	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32GOPClientId = 0;
	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].bIsShared = FALSE;
	g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u16SharedCnt = 0;

	memset(&gWin, 0, sizeof(GOP_GwinInfo));
	gWin.clrType = E_GOP_COLOR_ARGB8888;
	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, (gId));
	gWin.u32DRAMRBlkStart = phyTempAddr;
	GOP_SetWinInfo(pInstance, gId, &gWin);

	RESET_BIT(g_pGOPCtxLocal->u16GOP_VMirrorRBLK_Adr, gId);
	RESET_BIT(g_pGOPCtxLocal->u16GOP_HMirrorRBLK_Adr, gId);
	RESET_BIT(g_pGOPCtxLocal->u16GOP_HMirror_HPos, gId);
	RESET_BIT(g_pGOPCtxLocal->u16GOP_VMirror_VPos, gId);

	return GOP_API_SUCCESS;

}


void GOP_ResetGOP(void *pInstance, MS_U8 u8GOP)
{
	MS_U32 i = 0, j = 0;
	MS_U8 u8GwinId;
	GOP_WinFB_INFO *pwinFB;
	E_GOP_API_Result enRet = GOP_API_FAIL;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (g_pGOPCtxLocal == NULL) {
		MS_BOOL bFirstInstance;

		GOP_PreInit_Ctx(pInstance, &bFirstInstance);
		if (MDrv_GOP_SetIOMapBase(g_pGOPCtxLocal) != TRUE) {
			APIGOP_ASSERT(FALSE, GOP_M_FATAL("\nget IO base fail"));
			return;
		}
	}

	MS_GOP_CTX_SHARED *pShared = g_pGOPCtxLocal->pGOPCtxShared;

	u8GwinId = _GOP_SelGwinId2(pInstance, u8GOP, 0);
	for (i = u8GwinId; i < u8GwinId + MDrv_GOP_GetGwinNum(g_pGOPCtxLocal, u8GOP); i++) {
		//Check and release APP's GWIN resource
		if (((pShared->gwinMap[i].bIsShared == FALSE)
		     && (DRV_MAX_GWIN_FB_SUPPORT >
			 pShared->gwinMap[i].u32CurFBId))
		    || ((pShared->gwinMap[i].bIsShared == TRUE)
			&& (pShared->gwinMap[i].u16SharedCnt == 0x0))) {
			for (j = 0; j < DRV_MAX_GWIN_FB_SUPPORT; j++) {
				pwinFB = _GetWinFB(pInstance, j);
				if (pwinFB != NULL) {
					if ((pwinFB->u32GOPClientId ==
					     pShared->gwinMap[i].u32GOPClientId)
					    && (pShared->gwinMap[i].u32CurFBId
					    == j)) {
						enRet =
						    (E_GOP_API_Result) GOP_FB_Destroy(pInstance, j);
						GOP_M_INFO
						    ("%s %d Try destroy FB[%tu], return[%u]\n",
						     __func__, __LINE__, (ptrdiff_t) j,
						     (MS_U8) enRet);
					}
				}
			}
			if (g_pGOPCtxLocal->pGOPCtxShared->gwinMap[i].u32GOPClientId != 0) {
				enRet = (E_GOP_API_Result) GOP_Win_Destroy_Abnormal(pInstance, i);
				GOP_M_INFO("\n---%s %d: Try DestroyWIN[%tu], return[%u]\n",
					   __func__, __LINE__, (ptrdiff_t) i, (MS_U8) enRet);
			}
		}
	}
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
				MDrv_GOP_GWIN_IsGWINEnabled(g_pGOPCtxLocal, u32Index, &bGwinEnable);
				if (bGwinEnable == TRUE) {
					//GWIN must be turn off in Vsync to avoid garbage
					MDrv_GOP_GWIN_ForceWrite_Update(g_pGOPCtxLocal, u8GOP,
									FALSE);
					MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, u32Index, FALSE);
					MDrv_GOP_GWIN_ForceWrite_Update(g_pGOPCtxLocal, u8GOP,
									TRUE);
				}
				GOP_SetWinInfo(pInstance, u32Index, &stGwin);
				MDrv_GOP_GWIN_SetBlending(g_pGOPCtxLocal, u32Index, FALSE, 0x0);
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

MS_U32 GOP_GetCurrentGWin(void *pInstance)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	return g_pGOPCtxLocal->current_winId;
}

MS_U32 GOP_GWIN_SetHMirror(void *pInstance, MS_BOOL bEnable)
{
	MS_U8 u8GwinID = 0, u8GWinBase = 0, u8GOPWinNum = 0, u8GOPIdx = 0;
	MS_U16 u16GwinState = 0;
	GOP_GwinInfo sGWININfo;
	MS_BOOL bUpdateRegOnce = FALSE, bHMirror = FALSE, bVMirror = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	bUpdateRegOnce = g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE];
	g_pGOPCtxLocal->pGOPCtxShared->bHMirror = bEnable;

	for (u8GOPIdx = 0; u8GOPIdx < MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal); u8GOPIdx++) {
		MDrv_GOP_IsGOPMirrorEnable(g_pGOPCtxLocal, u8GOPIdx, &bHMirror, &bVMirror);
		if (bHMirror == bEnable)
			continue;

		u8GOPWinNum = MDrv_GOP_GetGwinNum(g_pGOPCtxLocal, u8GOPIdx);
		if (u8GOPWinNum == 0xff)
			continue;

		u8GWinBase = _GOP_SelGwinId2(pInstance, u8GOPIdx, 0);
		u16GwinState = 0;	//Clear GWIN on/off state
		//Turn off all on state GWIN
		for (u8GwinID = u8GWinBase; u8GwinID < u8GWinBase + u8GOPWinNum; u8GwinID++) {
			if (!_GOP_IsGwinIdValid(pInstance, u8GwinID)) {
				GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__,
					  __LINE__, u8GwinID);
				return GOP_API_FAIL;
			}

			if (g_pGOPCtxLocal->pGOPCtxShared->bGWINEnable[u8GwinID] != TRUE)
				continue;

			if (_GOP_GWIN_IsGwinExistInClient(pInstance, u8GwinID) == TRUE) {
				MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, u8GwinID, FALSE);
				u16GwinState |= (1 << u8GwinID);
			} else {
				u16GwinState = 0xFFFF;
				break;
			}
		}

		if (u16GwinState == 0xFFFF)
			continue;

		//Enable mirror
		MDrv_GOP_GWIN_EnableHMirror(g_pGOPCtxLocal, u8GOPIdx, bEnable);
		//Reset ON state GWIN info after mirror enable
		if (u16GwinState != 0) {
			g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE] =
				TRUE;
			for (u8GwinID = u8GWinBase;
			u8GwinID < u8GWinBase + u8GOPWinNum;
			u8GwinID++) {
				if (u16GwinState & (1 << u8GwinID)) {
					if (GOP_GetWinInfo(pInstance, u8GwinID, &sGWININfo) ==
						TRUE)
						GOP_SetWinInfo(pInstance, u8GwinID, &sGWININfo);

					MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, u8GwinID, TRUE);
				}
			}
			g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE] = bUpdateRegOnce;
		}
		MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, u8GOPIdx);
	}

	if (bEnable == FALSE) {
		g_pGOPCtxLocal->u16GOP_HMirror_HPos = 0;
		g_pGOPCtxLocal->u16GOP_HMirrorRBLK_Adr = 0;
	}
	return GOP_API_SUCCESS;
}


MS_U32 GOP_GWIN_SetVMirror(void *pInstance, MS_BOOL bEnable)
{
	MS_U8 u8GwinID = 0, u8GWinBase = 0, u8GOPWinNum = 0, u8GOPIdx = 0;
	MS_U16 u16GwinState = 0;
	GOP_GwinInfo sGWININfo;
	MS_BOOL bUpdateRegOnce = FALSE, bHMirror = FALSE, bVMirror = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	bUpdateRegOnce = g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE];
	g_pGOPCtxLocal->pGOPCtxShared->bVMirror = bEnable;

	for (u8GOPIdx = 0; u8GOPIdx < MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal); u8GOPIdx++) {
		MDrv_GOP_IsGOPMirrorEnable(g_pGOPCtxLocal, u8GOPIdx, &bHMirror, &bVMirror);
		if (bVMirror == bEnable)
			continue;

		u8GOPWinNum = MDrv_GOP_GetGwinNum(g_pGOPCtxLocal, u8GOPIdx);
		if (u8GOPWinNum == 0xff)
			continue;

		u8GWinBase = _GOP_SelGwinId2(pInstance, u8GOPIdx, 0);
		u16GwinState = 0;	//Clear GWIN on/off state
		//Turn off all on state GWIN
		for (u8GwinID = u8GWinBase; u8GwinID < u8GWinBase + u8GOPWinNum; u8GwinID++) {
			if (!_GOP_IsGwinIdValid(pInstance, u8GwinID)) {
				GOP_M_ERR("[%s][%d]GWIN %d  is out of range\n", __func__,
					  __LINE__, u8GwinID);
				return GOP_API_FAIL;
			}

			if (g_pGOPCtxLocal->pGOPCtxShared->bGWINEnable[u8GwinID] != TRUE)
				continue;

			if (_GOP_GWIN_IsGwinExistInClient(pInstance, u8GwinID) == TRUE) {
				MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, u8GwinID, FALSE);
				u16GwinState |= (1 << u8GwinID);
			} else {
				u16GwinState = 0xFFFF;
				break;
			}
		}

		if (u16GwinState == 0xFFFF)
			continue;

		//Enable mirror
		MDrv_GOP_GWIN_EnableVMirror(g_pGOPCtxLocal, u8GOPIdx, bEnable);
		//Reset ON state GWIN info after mirror enable
		if (u16GwinState != 0) {
			g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE] = TRUE;
			for (u8GwinID = u8GWinBase;
			u8GwinID < u8GWinBase + u8GOPWinNum;
			u8GwinID++) {
				if (u16GwinState & (1 << u8GwinID)) {
					if (GOP_GetWinInfo(pInstance, u8GwinID, &sGWININfo) ==
						TRUE)
						GOP_SetWinInfo(pInstance, u8GwinID, &sGWININfo);

					MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, u8GwinID, TRUE);
				}
			}
			g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE] = bUpdateRegOnce;
		}
		MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, u8GOPIdx);
	}

	if (bEnable == FALSE) {
		g_pGOPCtxLocal->u16GOP_VMirrorRBLK_Adr = 0;
		g_pGOPCtxLocal->u16GOP_VMirror_VPos = 0;
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
		msWarning(ERR_GWIN_ID_NOT_ALLOCATED);
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
		if (g_pGOPCtxLocal->pGopChipProperty->bSupportV2tap_16Phase[u8GOP] == TRUE) {
			MDrv_GOP_GWIN_Set_VStretchMode(g_pGOPCtxLocal, u8GOP,
						       E_DRV_GOP_VSTRCH_LINEAR);
			MDrv_GOP_GWIN_Load_VStretchModeTable(g_pGOPCtxLocal, u8GOP,
							     E_DRV_GOP_VSTRCH_LINEAR);
		} else {
			GOP_M_WARN("[%s]GOP%d not support V 2-tap 16 phase mode:%d\n", __func__,
				   u8GOP, VStrchMode);
			return GOP_API_FUN_NOT_SUPPORTED;
		}
		break;
	case E_GOP_VSTRCH_DUPLICATE:
		MDrv_GOP_GWIN_Set_VStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_VSTRCH_DUPLICATE);
		break;
	case E_GOP_VSTRCH_NEAREST:
		MDrv_GOP_GWIN_Set_VStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_VSTRCH_NEAREST);
		break;
	case E_GOP_VSTRCH_LINEAR_GAIN2:
		if (g_pGOPCtxLocal->pGopChipProperty->bSupportV_BiLinear[u8GOP] == TRUE) {
			MDrv_GOP_GWIN_Set_VStretchMode(g_pGOPCtxLocal, u8GOP,
						       E_DRV_GOP_VSTRCH_LINEAR_GAIN2);
		} else {
			GOP_M_WARN("[%s]GOP%d not support Bi-Linear mode:%d\n", __func__, u8GOP,
				   VStrchMode);
			return GOP_API_FUN_NOT_SUPPORTED;
		}
		break;
	case E_GOP_VSTRCH_4TAP:
		if (g_pGOPCtxLocal->pGopChipProperty->bSupportV4tap[u8GOP] == TRUE) {
			MDrv_GOP_GWIN_Set_VStretchMode(g_pGOPCtxLocal, u8GOP,
						       E_DRV_GOP_VSTRCH_4TAP);
			MDrv_GOP_GWIN_Load_VStretchModeTable(g_pGOPCtxLocal, u8GOP,
							     E_DRV_GOP_VSTRCH_4TAP);
		} else {
			GOP_M_WARN("[%s]GOP%d not support VStrchMode:%d\n", __func__, u8GOP,
				   VStrchMode);
			return GOP_API_FUN_NOT_SUPPORTED;
		}
		break;
	case E_GOP_VSTRCH_4TAP_100:
		if (g_pGOPCtxLocal->pGopChipProperty->bSupportV4tap[u8GOP] == TRUE) {
			MDrv_GOP_GWIN_Set_VStretchMode(g_pGOPCtxLocal, u8GOP,
						       E_DRV_GOP_VSTRCH_4TAP);
			MDrv_GOP_GWIN_Load_VStretchModeTable(g_pGOPCtxLocal, u8GOP,
							     E_DRV_GOP_HSTRCH_V4TAP_100);
		} else {
			GOP_M_WARN("[%s]GOP%d not support VStrchMode:%d\n", __func__, u8GOP,
				   VStrchMode);
			return GOP_API_FUN_NOT_SUPPORTED;
		}
		break;
	default:
		GOP_M_WARN("[%s]not support enum VStrchMode:%d\n", __func__, VStrchMode);
		return GOP_API_ENUM_NOT_SUPPORTED;
	}
	return GOP_API_SUCCESS;
}

E_GOP_API_Result _GOP_GWIN_IsSupport_HStretchMode(void *pInstance, MS_U8 u8GOP,
						  EN_GOP_STRETCH_HMODE enHStrchMode)
{
	E_GOP_API_Result eRet = GOP_API_SUCCESS;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (enHStrchMode) {
	case E_GOP_HSTRCH_DUPLICATE:
		if (g_pGOPCtxLocal->pGopChipProperty->bSupportHDuplicate[u8GOP] == TRUE) {
			eRet = GOP_API_SUCCESS;
		} else {
			GOP_M_WARN("[%s]GOP%d not support H Duplicate mode:%d\n", __func__,
				   u8GOP, E_GOP_HSTRCH_DUPLICATE);
			eRet = GOP_API_ENUM_NOT_SUPPORTED;
		}
		break;
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
		if (g_pGOPCtxLocal->pGopChipProperty->bSupportH6Tap_8Phase[u8GOP] == TRUE) {
			eRet = GOP_API_SUCCESS;
		} else {
			GOP_M_WARN("[%s]GOP%d not support H 6-tap mode:%d\n", __func__, u8GOP,
				   enHStrchMode);
			eRet = GOP_API_ENUM_NOT_SUPPORTED;
		}
		break;
	case E_GOP_HSTRCH_4TAPE:
	case E_GOP_HSTRCH_NEW4TAP:
	case E_GOP_HSTRCH_NEW4TAP_50:
	case E_GOP_HSTRCH_NEW4TAP_55:
	case E_GOP_HSTRCH_NEW4TAP_65:
	case E_GOP_HSTRCH_NEW4TAP_75:
	case E_GOP_HSTRCH_NEW4TAP_85:
	case E_GOP_HSTRCH_NEW4TAP_95:
	case E_GOP_HSTRCH_NEW4TAP_100:
	case E_GOP_HSTRCH_NEW4TAP_105:
		if (g_pGOPCtxLocal->pGopChipProperty->bSupportH4Tap_256Phase[u8GOP] == TRUE) {
			eRet = GOP_API_SUCCESS;
		} else {
			GOP_M_WARN("[%s]GOP%d not support H 4-tap 256phase mode:%d\n", __func__,
				   u8GOP, enHStrchMode);
			eRet = GOP_API_ENUM_NOT_SUPPORTED;
		}
		break;
	default:
		eRet = GOP_API_ENUM_NOT_SUPPORTED;
		break;
	}

	return eRet;
}

MS_U32 GOP_GWIN_Set_HStretchMode(void *pInstance, MS_U8 u8GOP, EN_GOP_STRETCH_HMODE HStrchMode)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (_GOP_GWIN_IsSupport_HStretchMode(pInstance, u8GOP, HStrchMode) != GOP_API_SUCCESS)
		return GOP_API_FUN_NOT_SUPPORTED;

	switch (HStrchMode) {
	case E_GOP_HSTRCH_DUPLICATE:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_DUPLICATE);
		break;
	case E_GOP_HSTRCH_6TAPE:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		break;
	case E_GOP_HSTRCH_6TAPE_LINEAR:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_6TAPE_LINEAR);
		break;
	case E_GOP_HSTRCH_6TAPE_NEAREST:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_6TAPE_NEAREST);
		break;
	case E_GOP_HSTRCH_6TAPE_GAIN0:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_6TAPE_GAIN0);
		break;
	case E_GOP_HSTRCH_6TAPE_GAIN1:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_6TAPE_GAIN1);
		break;
	case E_GOP_HSTRCH_6TAPE_GAIN2:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_6TAPE_GAIN2);
		break;
	case E_GOP_HSTRCH_6TAPE_GAIN3:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_6TAPE_GAIN3);
		break;
	case E_GOP_HSTRCH_6TAPE_GAIN4:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_6TAPE_GAIN4);
		break;
	case E_GOP_HSTRCH_6TAPE_GAIN5:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_6TAPE_GAIN5);
		break;
	case E_GOP_HSTRCH_2TAPE:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_6TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_2TAPE);
		break;
	case E_GOP_HSTRCH_4TAPE:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		break;
	case E_GOP_HSTRCH_NEW4TAP:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_45);
		break;
	case E_GOP_HSTRCH_NEW4TAP_50:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_50);
		break;
	case E_GOP_HSTRCH_NEW4TAP_55:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_55);
		break;
	case E_GOP_HSTRCH_NEW4TAP_65:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_65);
		break;
	case E_GOP_HSTRCH_NEW4TAP_75:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_75);
		break;
	case E_GOP_HSTRCH_NEW4TAP_85:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_85);
		break;
	case E_GOP_HSTRCH_NEW4TAP_95:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_95);
		break;
	case E_GOP_HSTRCH_NEW4TAP_105:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_105);
		break;
	case E_GOP_HSTRCH_NEW4TAP_100:
		MDrv_GOP_GWIN_Set_HStretchMode(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_HSTRCH_4TAPE);
		MDrv_GOP_GWIN_Load_HStretchModeTable(g_pGOPCtxLocal, u8GOP,
						     E_DRV_GOP_HSTRCH_NEW4TAP_100);
		break;
	default:
		return GOP_API_ENUM_NOT_SUPPORTED;
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

MS_U32 GOP_IsRegUpdated(void *pInstance, MS_U8 u8GopType)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (MDrv_GOP_GetGOPACK(g_pGOPCtxLocal, u8GopType))
		return GOP_API_SUCCESS;

	return GOP_API_FAIL;
}

MS_U32 Ioctl_GOP_Get_ScaleDst(void *pInstance, MS_U8 u8GOPNum, MS_U16 *pHScaleDst,
			      MS_U16 *pVScaleDst)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	*pHScaleDst = g_pGOPCtxLocal->pGOPCtxShared->u16HScaleDst[u8GOPNum];
	*pVScaleDst = g_pGOPCtxLocal->pGOPCtxShared->u16VScaleDst[u8GOPNum];
	return GOP_API_SUCCESS;
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
			g_pGOPCtxLocal->Gwin_H_Dup = FALSE;
			g_pGOPCtxLocal->Gwin_V_Dup = FALSE;
			g_pGOPCtxLocal->current_winId = 0;
			g_pGOPCtxLocal->pGOPCtxShared->fbPool1.GWinFB_Pool_BaseAddr =
			    pGopInit->u32GOPRBAdr;
			g_pGOPCtxLocal->pGOPCtxShared->fbPool1.u32GWinFB_Pool_MemLen =
			    pGopInit->u32GOPRBLen;
			g_pGOPCtxLocal->pGOPCtxShared->fbPool1.u32FirstFBIdInPool =
			    INVALID_POOL_NEXT_FBID;
			g_pGOPCtxLocal->pGOPCtxShared->fbPool1.poolId = GOP_WINFB_POOL1;
			g_pGOPCtxLocal->pGOPCtxShared->fbPool2.GWinFB_Pool_BaseAddr = 0;
			g_pGOPCtxLocal->pGOPCtxShared->fbPool2.u32GWinFB_Pool_MemLen = 0;
			g_pGOPCtxLocal->pGOPCtxShared->fbPool2.u32FirstFBIdInPool =
			    INVALID_POOL_NEXT_FBID;
			g_pGOPCtxLocal->pGOPCtxShared->fbPool2.poolId = GOP_WINFB_POOL2;
			g_pGOPCtxLocal->pGOPCtxShared->fb_currentPoolId = GOP_WINFB_POOL1;
			g_pGOPCtxLocal->pGOPCtxShared->phyGOPRegdmaAdr = pGopInit->u32GOPRegdmaAdr;
			g_pGOPCtxLocal->pGOPCtxShared->u32GOPRegdmaLen = pGopInit->u32GOPRegdmaLen;
		}

		if (pGopInit->bEnableVsyncIntFlip == TRUE)
			g_pGOPCtxLocal->pGOPCtxShared->bEnableVsyncIntFlip = TRUE;

		if (bMMIOInit == FALSE) {
			if (MDrv_GOP_SetIOMapBase(g_pGOPCtxLocal) != TRUE) {
				APIGOP_ASSERT(FALSE, GOP_M_FATAL("\nget IO base fail"));
				return GOP_API_FAIL;
			}
			bMMIOInit = TRUE;
		}

		g_pGOPCtxLocal->pGOPCtxShared->u16PnlWidth[u8GOP] = pGopInit->u16PanelWidth;
		g_pGOPCtxLocal->pGOPCtxShared->u16PnlHeight[u8GOP] = pGopInit->u16PanelHeight;
		g_pGOPCtxLocal->pGOPCtxShared->u16PnlHStr[u8GOP] = pGopInit->u16PanelHStr;

		if (!
		    (g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit ==
		     (DRV_GOP_IGNOREINIT) E_DRV_GOP_ALL_IGNORE)) {
			MDrv_GOP_CtrlRst(g_pGOPCtxLocal, u8GOP, TRUE);
		}

		if (g_pGOPCtxLocal->pGOPCtxShared->bGopHasInitialized[u8GOP] == FALSE) {

			if (g_pGOPCtxLocal->pGopChipProperty->bPixelModeSupport)
				g_pGOPCtxLocal->pGOPCtxShared->bPixelMode[u8GOP] = TRUE;

			//Not initialized yet:
			GOP_GWIN_InitByGOP(pInstance, bFirstInstance, u8GOP);

			if (!(g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit & E_GOP_IGNORE_GWIN))
				MDrv_GOP_Init(g_pGOPCtxLocal, u8GOP, pGopInit->u32GOPRegdmaAdr,
					      pGopInit->u32GOPRegdmaLen,
					      pGopInit->bEnableVsyncIntFlip);

			if (!(g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit &
				E_GOP_IGNORE_SET_DST_OP)) {
				MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, u8GOP, TRUE);
				MDrv_GOP_GWIN_SetAlphaInverse(g_pGOPCtxLocal, u8GOP, TRUE);
				MDrv_GOP_GWIN_SetDstPlane(g_pGOPCtxLocal, u8GOP, E_DRV_GOP_DST_OP0,
							  FALSE);
				(_GOP_Map_APIDst2DRV_Enum(pInstance, E_GOP_DST_OP0, &GopDst));
				MDrv_GOP_SetGOPClk(g_pGOPCtxLocal, u8GOP, GopDst);
			}

			MS_GOP_CTX_SHARED *pShared = g_pGOPCtxLocal->pGOPCtxShared;

			if (!(g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit & E_GOP_IGNORE_STRETCHWIN))
				MDrv_GOP_GWIN_SetStretchWin(g_pGOPCtxLocal, u8GOP, 0, 0,
							    pShared->u16PnlWidth[u8GOP],
							    pShared->u16PnlHeight[u8GOP]);

			if (u8GOP == 0)
				MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_NIP_SEL_GOP0);
			else if (u8GOP == 1)
				MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_NIP_SEL_GOP1);
			else if (u8GOP == 2)
				MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_NIP_SEL_GOP2);

			if (!
			    (g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit &
				E_GOP_IGNORE_ENABLE_TRANSCLR))
				MDrv_GOP_GWIN_EnableTransClr(g_pGOPCtxLocal, u8GOP,
							     (GOP_TransClrFmt) 0, FALSE);

			MDrv_GOP_SetGOPEnable2SC(g_pGOPCtxLocal, u8GOP, TRUE);
			if (!(g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit & E_GOP_IGNORE_GWIN))
				g_pGOPCtxLocal->pGOPCtxShared->bGopHasInitialized[u8GOP] = TRUE;
		}

		_GOP_InitHSPDByGOP(pInstance, u8GOP);

		if (bFirstInstance)
			MDrv_GOP_SetVOPNBL(g_pGOPCtxLocal, FALSE);

		if (!
		    (g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit ==
		     (DRV_GOP_IGNOREINIT) E_DRV_GOP_ALL_IGNORE)) {
			MDrv_GOP_CtrlRst(g_pGOPCtxLocal, u8GOP, FALSE);
		}
	}

	//restore force write setting
	MDrv_GOP_GWIN_SetForceWrite(g_pGOPCtxLocal, g_pGOPCtxLocal->bInitFWR);

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
	case E_GOP_CAP_GWIN_NUM:
		{
			*((MS_U16 *) pRet) = g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum;
			break;
		}
	case E_GOP_CAP_PIXELMODE_SUPPORT:
		{
			*((MS_BOOL *) pRet) = g_pGOPCtxLocal->pGopChipProperty->bPixelModeSupport;
			break;
		}
	case E_GOP_CAP_STRETCH:
		{
			if (pRet != NULL) {
				MS_U8 i;
				PGOP_CAP_STRETCH_INFO pGOPStretchInfo =
				    (PGOP_CAP_STRETCH_INFO) pRet;
#ifndef MSOS_TYPE_LINUX_KERNEL	//Check size error when if(is_compat_task()==1)
				if (ret_size != sizeof(GOP_CAP_STRETCH_INFO)) {
					GOP_M_ERR
					    ("[%s] ERROR, invalid structure size :%td\n",
					     __func__, (ptrdiff_t) ret_size);
					return GOP_API_FAIL;
				}
#endif
				pGOPStretchInfo->GOP_VStretch_Support = 0;
				for (i = 0;
				     i <
				     MIN(sizeof(g_pGOPCtxLocal->pGopChipProperty->bGOPWithVscale),
					 (sizeof(pGOPStretchInfo->GOP_VStretch_Support) * 8));
				     i++) {
					if (g_pGOPCtxLocal->pGopChipProperty->bGOPWithVscale[i])
						pGOPStretchInfo->GOP_VStretch_Support |= BIT(i);
					else
						pGOPStretchInfo->GOP_VStretch_Support &= ~(BIT(i));
				}
			} else {
				APIGOP_ASSERT(0,
					      GOP_M_FATAL
					      ("Invalid input parameter: NULL pointer\n"));
			}
			break;
		}
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
	case E_GOP_CAP_BNKFORCEWRITE:
		{
			*pRet = g_pGOPCtxLocal->pGopChipProperty->bBnkForceWrite;
			break;
		}
	case E_GOP_CAP_SUPPORT_V4TAP_256PHASE:
		{
			GOP_CHIP_PROPERTY *pGopProp = g_pGOPCtxLocal->pGopChipProperty;

			if (pRet != NULL) {
				PGOP_CAP_STRETCH_INFO pstGOPStretchInfo =
				    (PGOP_CAP_STRETCH_INFO) pRet;
				MS_U8 u8GOPNum = 0;

				if (ret_size != sizeof(GOP_CAP_STRETCH_INFO)) {
					GOP_M_ERR
					    ("[%s] ERROR, invalid structure size :%td\n",
					     __func__, (ptrdiff_t) ret_size);
					return GOP_API_FAIL;
				}
				for (u8GOPNum = 0; u8GOPNum < GOP_MAX_SUPPORT_DEFAULT; u8GOPNum++) {
					if (pGopProp->bSupportV4tap[u8GOPNum]) {
						pstGOPStretchInfo->GOP_VStretch_Support |=
						    BIT(u8GOPNum);
					} else {
						pstGOPStretchInfo->GOP_VStretch_Support &=
						    ~(BIT(u8GOPNum));
					}
				}
			}
			break;
		}
	case E_GOP_CAP_SUPPORT_V_SCALING_BILINEAR:
		{
			GOP_CHIP_PROPERTY *pGopProp = g_pGOPCtxLocal->pGopChipProperty;

			if (pRet != NULL) {
				PGOP_CAP_STRETCH_INFO pstGOPStretchInfo =
				    (PGOP_CAP_STRETCH_INFO) pRet;
				MS_U8 u8GOPNum = 0;

				if (ret_size != sizeof(GOP_CAP_STRETCH_INFO)) {
					GOP_M_ERR
					    ("[%s] ERROR, invalid structure size :%td\n",
					     __func__, (ptrdiff_t) ret_size);
					return GOP_API_FAIL;
				}
				for (u8GOPNum = 0; u8GOPNum < GOP_MAX_SUPPORT_DEFAULT; u8GOPNum++) {
					if (pGopProp->bSupportV_BiLinear[u8GOPNum]) {
						pstGOPStretchInfo->GOP_VStretch_Support |=
						    BIT(u8GOPNum);
					} else {
						pstGOPStretchInfo->GOP_VStretch_Support &=
						    ~(BIT(u8GOPNum));
					}
				}
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
	ST_GOP_AUTO_DETECT_BUF_INFO *pstGopAutoDetectInfo;
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
	case E_GOP_AUTO_DETECT_BUFFER:
		{
			pstGopAutoDetectInfo = (ST_GOP_AUTO_DETECT_BUF_INFO *) (pCfg);

			if (pstGopAutoDetectInfo == NULL) {
				GOP_M_ERR("[%s][%d] pstGopAutoDetectInfo is NULL  ERROR!!\n",
					  __func__, __LINE__);
				return GOP_API_FAIL;
			}
			if (MDrv_GOP_AutoDectBuf(g_pGOPCtxLocal, pstGopAutoDetectInfo) !=
			    GOP_SUCCESS) {
				return GOP_API_FAIL;
			}
			break;
		}
	default:

		break;
	}

	return GOP_API_SUCCESS;
}

MS_U32 Ioctl_GOP_GetConfig(void *pInstance, EN_GOP_CONFIG_TYPE cfg_type, MS_U32 *pCfg,
			   MS_U32 u32Size)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (cfg_type) {
	case E_GOP_IGNOREINIT:
		{
			EN_GOP_IGNOREINIT *penGOPIgnoreInit;

			penGOPIgnoreInit = (EN_GOP_IGNOREINIT *) pCfg;
			*penGOPIgnoreInit =
			    (EN_GOP_IGNOREINIT) g_pGOPCtxLocal->sGOPConfig.eGopIgnoreInit;
			break;
		}
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
	case E_GOP_OUTPUT_COLOR:
		{
			EN_GOP_OUTPUT_COLOR output;

			if (u32Size != sizeof(EN_GOP_OUTPUT_COLOR)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}

			output = *((EN_GOP_OUTPUT_COLOR *) pSet);
			GOP_SetOutputColor(pInstance, gop, output);

			break;

		}
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
	case E_GOP_RESOURCE:
		{
			GOP_ResetGOP(pInstance, gop);
			break;
		}
	case E_GOP_AFBC_CORE_RESET:
		{
#ifdef CONFIG_GOP_AFBC_FEATURE
			if (u32Size != sizeof(MS_BOOL)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			MDrv_GOP_AFBC_Core_Reset(g_pGOPCtxLocal, gop);
#endif
			break;
		}
	case E_GOP_AFBC_CORE_ENABLE:
		{
#ifdef CONFIG_GOP_AFBC_FEATURE
			MS_BOOL bEn;

			if (u32Size != sizeof(MS_BOOL)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			bEn = *((MS_BOOL *) pSet);
			MDrv_GOP_AFBC_Core_Enable(g_pGOPCtxLocal, gop, bEn);
#endif
			break;
		}
	case E_GOP_SET_VOP_PATH:
		{
			EN_GOP_VOP_PATH_MODE enGOPPath = E_GOP_VOPPATH_DEF;
			MS_U8 u8Index = 0;

			if (pSet == NULL) {
				GOP_M_ERR("[%s][%d] pSet is NULL  ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			if (u32Size != sizeof(EN_GOP_VOP_PATH_MODE)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}

			enGOPPath = *((EN_GOP_VOP_PATH_MODE *) pSet);
			MDrv_GOP_VOP_Path_Sel(g_pGOPCtxLocal, enGOPPath);
			for (u8Index = 0; u8Index < _GOP_GetMaxGOPNum(pInstance); u8Index++) {
				_GOP_InitHSPDByGOP(pInstance, u8Index);
				MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, u8Index);
			}
			break;
		}
	case E_GOP_SET_CSC_TUNING:
		{
			ST_GOP_CSC_PARAM *pstGOPRaram = (ST_GOP_CSC_PARAM *) pSet;
			GOP_Result enRet;

			if (pstGOPRaram == NULL) {
				GOP_M_ERR("[%s][%d] pstGOPRaram is NULL  ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			enRet = MDrv_GOP_CSC_Tuning(g_pGOPCtxLocal, gop, pstGOPRaram);
			if (enRet != GOP_SUCCESS) {
				if (enRet == GOP_FUN_NOT_SUPPORTED)
					return GOP_API_FUN_NOT_SUPPORTED;
				else
					return GOP_API_FAIL;
			}

			MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, gop);

			break;
		}
	case E_GOP_PIXELSHIFT_PD:
		{
			MS_S32 s32Offset;
			MS_U8 u8Index = 0;

			s32Offset = *((MS_S32 *) pSet);

			if (g_pGOPCtxLocal->pGopChipProperty->b2Pto1PSupport == TRUE) {
				g_pGOPCtxLocal->pGOPCtxShared->s32PixelShiftPDOffset =
				    (s32Offset + (s32Offset + 1) / 2) * 2;
			} else {
				g_pGOPCtxLocal->pGOPCtxShared->s32PixelShiftPDOffset = s32Offset;
			}

			for (u8Index = 0; u8Index < _GOP_GetMaxGOPNum(pInstance); u8Index++) {
				_GOP_InitHSPDByGOP(pInstance, u8Index);
				MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, u8Index);
			}
			break;
		}
	case E_GOP_SET_CROP_WIN:
		{
			ST_GOP_CROP_WINDOW *pstCropWin = (ST_GOP_CROP_WINDOW *) pSet;

			MDrv_GOP_SetCropWindow(g_pGOPCtxLocal, gop, pstCropWin);
			break;
		}
	default:
		{
			GOP_M_ERR("[%s] invalid input case:%d\n", __func__, type);
			break;
		}

	}

	return GOP_API_SUCCESS;

}


MS_U32 Ioctl_GOP_GetProperty(void *pInstance, EN_GOP_PROPERTY type, MS_U32 gop, MS_U32 *pSet,
			     MS_U32 u32Size)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (type) {
	case E_GOP_REG_UPDATED:
		{
			MS_BOOL *pUpdated = (MS_BOOL *) pSet;

			if (u32Size != sizeof(MS_BOOL)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			*pUpdated = GOP_IsRegUpdated(pInstance, gop);
			break;
		}
	case E_GOP_MAXFBNUM:
		{
			MS_U32 *pMAXFBNum = (MS_U32 *) pSet;

			if (u32Size != sizeof(MS_U32)) {
				GOP_M_ERR("[%s][%d] input structure size ERROR!!\n", __func__,
					  __LINE__);
				return GOP_API_FAIL;
			}
			*pMAXFBNum = DRV_MAX_GWIN_FB_SUPPORT;
			break;
		}
	default:
		GOP_M_ERR("[%s] This status(%d) can't be query\n", __func__, type);
		break;

	}

	return GOP_API_SUCCESS;

}

MS_U32 Ioctl_GOP_SetDst(void *pInstance, MS_U8 u8GOP, EN_GOP_DST_TYPE dsttype)
{
	MS_U8 u8GetGOPNum = 0, i;
	DRV_GOPDstType GopDst;
	Gop_MuxSel OPMux;
	MS_BOOL bHDREnable = FALSE;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if ((!_GOP_IsGopNumValid(pInstance, u8GOP))) {
		GOP_M_ERR("[%s][%d]GOP %d  is out of range\n", __func__, __LINE__, u8GOP);
		return GOP_API_FAIL;
	}

	GOP_M_INFO("---%s %d: PID[%td], TID[%td] u8GOP=%u, DstType=0x%tx\n", __func__, __LINE__,
		   (ptrdiff_t) getpid(), (ptrdiff_t) MsOS_GetOSThreadID(), u8GOP,
		   (ptrdiff_t) dsttype);

	/*Set GOP dst and check input dst type is valid or not for different chip first */
	(_GOP_Map_APIDst2DRV_Enum(pInstance, dsttype, &GopDst));
	(MDrv_GOP_GWIN_SetDstPlane(g_pGOPCtxLocal, u8GOP, GopDst, FALSE));

	// Set GOP dst output blending to SC/Mixer
	switch (GopDst) {

	case E_DRV_GOP_DST_IP0:
		MDrv_GOP_GWIN_SetMux(g_pGOPCtxLocal, u8GOP, E_GOP_IP0_MUX);
		_GOP_InitHSPDByGOP(pInstance, u8GOP);
		MDrv_GOP_SetGOPEnable2SC(g_pGOPCtxLocal, u8GOP, FALSE);
		_GOP_SetOCCapability(pInstance, u8GOP, GopDst);
		if (u8GOP == 0)
			MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_IP0_SEL_GOP0);
		else if (u8GOP == 1)
			MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_IP0_SEL_GOP1);
		else
			MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_IP0_SEL_GOP2);
		//MApi_GOP_GWIN_OutputColor(GOPOUT_YUV);
		MDrv_GOP_SelfFirstHs(g_pGOPCtxLocal, u8GOP, TRUE);
		break;
	case E_DRV_GOP_DST_OP0:
		_GOP_SetOCCapability(pInstance, u8GOP, GopDst);

		if (u8GOP == 0)
			MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_NIP_SEL_GOP0);
		else if (u8GOP == 1)
			MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_NIP_SEL_GOP1);
		else
			MDrv_GOP_SetIPSel2SC(g_pGOPCtxLocal, MS_DRV_NIP_SEL_GOP2);

		MDrv_GOP_IsHDREnabled(g_pGOPCtxLocal, &bHDREnable);
		if (bHDREnable == TRUE)
			MDrv_GOP_SetGOPEnable2SC(g_pGOPCtxLocal, u8GOP, FALSE);
		else
			MDrv_GOP_SetGOPEnable2SC(g_pGOPCtxLocal, u8GOP, TRUE);

		_GOP_InitHSPDByGOP(pInstance, u8GOP);
		if (g_pGOPCtxLocal->pGopChipProperty->bOpInterlace == TRUE)
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, u8GOP, FALSE);
		else
			MDrv_GOP_GWIN_EnableProgressive(g_pGOPCtxLocal, u8GOP, TRUE);

		MDrv_GOP_SelfFirstHs(g_pGOPCtxLocal, u8GOP, FALSE);
		break;
	case E_DRV_GOP_DST_FRC:
		MDrv_GOP_SetGOPEnable2SC(g_pGOPCtxLocal, u8GOP, FALSE);
		_GOP_InitHSPDByGOP(pInstance, u8GOP);
		_GOP_SetOCCapability(pInstance, u8GOP, GopDst);
		MDrv_GOP_SelfFirstHs(g_pGOPCtxLocal, u8GOP, FALSE);
		break;
	case E_DRV_GOP_DST_BYPASS:
		_GOP_SetOCCapability(pInstance, u8GOP, GopDst);
		_GOP_InitHSPDByGOP(pInstance, u8GOP);
		MDrv_GOP_GWIN_SetMux(g_pGOPCtxLocal, u8GOP, E_GOP_BYPASS_MUX0);
		MDrv_GOP_SetGOPEnable2SC(g_pGOPCtxLocal, u8GOP, FALSE);
		MDrv_GOP_SelfFirstHs(g_pGOPCtxLocal, u8GOP, FALSE);
		break;
	default:
		return GOP_API_ENUM_NOT_SUPPORTED;
	}

	(MDrv_GOP_SetGOPClk(g_pGOPCtxLocal, u8GOP, GopDst));
	MDrv_GOP_GWIN_UpdateReg(g_pGOPCtxLocal, u8GOP);
	return GOP_API_SUCCESS;
}

MS_U32 Ioctl_GOP_SetMux(void *pInstance, PGOP_SETMUX pGopMuxConfig, MS_U32 u32SizeOfMuxInfo)
{
	MS_U8 i;
	Gop_MuxSel EnGOPMux;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	if (u32SizeOfMuxInfo != sizeof(GOP_SETMUX) || (pGopMuxConfig == NULL)) {
		GOP_M_ERR("\nthe GOP_MuxConfig is NULL or struct mismatch in drivr\n");
		return GOP_API_FAIL;
	}
	GOP_M_INFO("---%s %d: PID[%td], TID[%td] MuxCount=%tu\n", __func__, __LINE__,
		   (ptrdiff_t) getpid(), (ptrdiff_t) MsOS_GetOSThreadID(),
		   (ptrdiff_t) pGopMuxConfig->MuxCount);

	for (i = 0; i < pGopMuxConfig->MuxCount; i++) {
		_GOP_MAP_MUX_Enum(pInstance, (EN_Gop_MuxSel) pGopMuxConfig->mux[i], &EnGOPMux);
		MDrv_GOP_GWIN_SetMux(g_pGOPCtxLocal, pGopMuxConfig->gop[i], EnGOPMux);
	}

	return GOP_API_SUCCESS;

}


MS_U32 Ioctl_GOP_SetMirror(void *pInstance, MS_U32 gop, EN_GOP_MIRROR_TYPE mirror)
{
	// To do: to modify and control each gop mirror setting.
	switch (mirror) {
	case E_GOP_MIRROR_NONE:
	default:
		{
			GOP_GWIN_SetHMirror(pInstance, FALSE);
			GOP_GWIN_SetVMirror(pInstance, FALSE);
			break;
		}
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
	case E_GOP_MIRROR_HV:
		{
			GOP_GWIN_SetHMirror(pInstance, TRUE);
			GOP_GWIN_SetVMirror(pInstance, TRUE);
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
	case E_GOP_STATUS_INIT:
		{
			PGOP_INIT_STATUS ptr = (PGOP_INIT_STATUS) pStatus;

			if (u32Size != sizeof(GOP_INIT_STATUS))
				return GOP_API_INVALID_PARAMETERS;

			ptr->bInit =
			    g_pGOPCtxLocal->pGOPCtxShared->bGopHasInitialized[ptr->gop_idx];

			break;
		}
	case E_GOP_STATUS_GOP_MAXNUM:
		{
			MS_U8 *pMaxGopNum = (MS_U8 *) pStatus;
			*pMaxGopNum = MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal);
			break;
		}
	case E_GOP_STATUS_GWIN_MAXNUM:
		{
			PGOP_GWIN_NUM ptr = (PGOP_GWIN_NUM) pStatus;

			if (u32Size != sizeof(GOP_GWIN_NUM)) {
				GOP_M_ERR("[%s] Error (%d)\n", __func__, __LINE__);
				return GOP_API_INVALID_PARAMETERS;
			}
			if (ptr->gop_idx > MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal)) {
				GOP_M_ERR("[%s] Error (%d)\n", __func__, __LINE__);
				return GOP_API_INVALID_PARAMETERS;
			}

			ptr->gwin_num = MDrv_GOP_GetGwinNum(g_pGOPCtxLocal, ptr->gop_idx);
			break;
		}
	case E_GOP_STATUS_GWIN_TOTALNUM:
		{
			MS_U8 *pGwinNum = (MS_U8 *) pStatus;

			if (u32Size != sizeof(MS_U8)) {
				GOP_M_ERR("[%s] Error (%d)\n", __func__, __LINE__);
				return GOP_API_INVALID_PARAMETERS;
			}
			*pGwinNum = (MS_U8) g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum;
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

	if (enStretchType & E_GOP_STRETCH_HSCALE) {
		if (pStretchInfo->SrcRect.w != pStretchInfo->DstRect.w)
			bEn = TRUE;

		enRet =
		    (E_GOP_API_Result) GOP_Set_Hscale(pInstance, gop, bEn, pStretchInfo->SrcRect.w,
						      pStretchInfo->DstRect.w);
	}

	if (enStretchType & E_GOP_STRETCH_VSCALE) {
		if (pStretchInfo->SrcRect.h != pStretchInfo->DstRect.h)
			bEn = TRUE;

		enRet =
		    (E_GOP_API_Result) GOP_Set_Vscale(pInstance, gop, bEn, pStretchInfo->SrcRect.h,
						      pStretchInfo->DstRect.h);
	}

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
	case E_GOP_GWIN_GET_GOP:
		{
			MS_U32 *pGOP = (MS_U32 *) pSet;

			if (u32Size != sizeof(MS_U32))
				return GOP_API_INVALID_PARAMETERS;

			*pGOP = (MS_U32) MDrv_DumpGopByGwinId(g_pGOPCtxLocal, gwin);
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

MS_U32 Ioctl_GOP_MapFB2Win(void *pInstance, MS_U32 fbid, MS_U32 gwin)
{
	E_GOP_API_Result eRet = GOP_API_FAIL;

	eRet = (E_GOP_API_Result) GOP_MapFB2Win(pInstance, fbid, gwin);
	return eRet;
}

MS_U32 Ioctl_GOP_Win_Enable(void *pInstance, MS_U32 gwinId, MS_BOOL bEn)
{
	MS_U16 regval2;
	MS_U32 u32fbId;
	GOP_WinFB_INFO *pwinFB;
	MS_U8 gwinNum = 0;
	MS_U8 i;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif


	if (!_GOP_IsGwinIdValid(pInstance, gwinId)) {
		GOP_M_ERR("[%s][%d]GWIN %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) gwinId);
		return GOP_API_FAIL;
	}
	//printf("GWINID=%d,Enable=%d\n",(MS_U8)winId,(MS_U8)bEnable);
	if (!_GOP_GWIN_IsGwinExistInClient(pInstance, gwinId)) {
		GOP_M_ERR("[%s][%d]GWIN %td  is not exist\n", __func__, __LINE__,
			  (ptrdiff_t) gwinId);
		//__ASSERT(0);
		return GOP_API_FAIL;
	}
	//   printf(" u8win=%02bx, bEnable=%02bx\n",u8win,bEnable);
	u32fbId = g_pGOPCtxLocal->pGOPCtxShared->gwinMap[gwinId].u32CurFBId;
	pwinFB = _GetWinFB(pInstance, u32fbId);

	if (pwinFB == NULL) {
		GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GOP_API_FAIL;
	}

	if (!_GOP_IsFbIdValid(pInstance, u32fbId)) {
		GOP_M_ERR("[%s][%d]FbId %td  is out of range\n", __func__, __LINE__,
			  (ptrdiff_t) u32fbId);
		return GOP_API_FAIL;
	}

	if (pwinFB->in_use == 0) {
		msWarning(ERR_GWIN_ID_NOT_ALLOCATED);
		GOP_M_ERR("[%s][%d]GWIN %td not allocated\n", __func__, __LINE__,
			  (ptrdiff_t) gwinId);
		return GOP_API_FAIL;
	}
	pwinFB->enable = bEn;

	for (i = 0; i < (MS_U8) g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum; i++) {
		if (GOP_GWIN_IsEnable(pInstance, i) == TRUE)
			gwinNum |= 1 << i;
	}

	regval2 = gwinNum;

	if (bEn == 0)
		MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, gwinId, FALSE);
	else
		MDrv_GOP_GWIN_EnableGwin(g_pGOPCtxLocal, gwinId, TRUE);

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
	MS_U16 u16Tmp, u16OrigWidth = 0;
	MS_U16 gwin_w, stretch_w;
	MS_BOOL u16OrgUpdateRegOnce = FALSE;
	GOP_StretchInfo *pStretchInfo;
	GOP_GwinInfo *pGwinInfo = NULL;
	MS_U32 u32FbId;
	EN_GOP_STRETCH_DIRECTION direction;
	MS_U16 u16DstWidth, u16DstHeight;
	DRV_GOPDstType eGopDst = E_DRV_GOP_DST_INVALID;
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

	u16OrgUpdateRegOnce = g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE];
	//Save old update_reg_once state, and change to update_reg_once=TRUE internally
	if (u16OrgUpdateRegOnce == FALSE)
		g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE] = TRUE;

	GOP_MapFB2Win(pInstance, u32FbId, GwinId);

	u8GOP = MDrv_DumpGopByGwinId(g_pGOPCtxLocal, GwinId);

	_GOP_Get_StretchWin(pInstance, u8GOP, &u16Tmp, &u16Tmp, &u16OrigWidth, &u16Tmp);

	gwin_w = pGwinInfo->u16DispHPixelEnd - pGwinInfo->u16DispHPixelStart;

	if (gwin_w > pStretchInfo->width)
		stretch_w = gwin_w;
	else
		stretch_w = pStretchInfo->width;

	MDrv_GOP_GWIN_GetDstPlane(g_pGOPCtxLocal, u8GOP, &eGopDst);
	_GOP_Map_DRVDst2API_Enum_(pInstance, &pStretchInfo->eDstType, eGopDst);
	_GOP_GWIN_Align_StretchWin(pInstance, u8GOP, pStretchInfo->eDstType, &pStretchInfo->x,
				   &pStretchInfo->y, &stretch_w, &pStretchInfo->height,
				   _GOP_GetBPP(pInstance, pGwinInfo->clrType));
	//Store API use stretch window
	g_pGOPCtxLocal->pGOPCtxShared->u16APIStretchWidth[u8GOP] = stretch_w;
	g_pGOPCtxLocal->pGOPCtxShared->u16APIStretchHeight[u8GOP] = pStretchInfo->height;

	MDrv_GOP_GWIN_SetStretchWin(g_pGOPCtxLocal, u8GOP, pStretchInfo->x, pStretchInfo->y,
				    stretch_w, pStretchInfo->height);

	if (u16DstWidth <= pStretchInfo->width)
		GOP_Set_Hscale(pInstance, u8GOP, FALSE, pStretchInfo->width, u16DstWidth);
	else if (direction & E_GOP_H_STRETCH)
		GOP_Set_Hscale(pInstance, u8GOP, TRUE, pStretchInfo->width, u16DstWidth);

	if (u16DstHeight < pStretchInfo->height)
		GOP_Set_Vscale(pInstance, u8GOP, FALSE, pStretchInfo->height, u16DstHeight);
	else if (direction & E_GOP_V_STRETCH)
		GOP_Set_Vscale(pInstance, u8GOP, TRUE, pStretchInfo->height, u16DstHeight);

	GOP_SetWinInfo(pInstance, GwinId, pGwinInfo);

	//Restore update_reg_once and trigger register writes in
	if (u16OrgUpdateRegOnce == FALSE)
		g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE] = u16OrgUpdateRegOnce;

	return GOP_API_SUCCESS;
}



MS_U32 Ioctl_GOP_Win_Destroy(void *pInstance, MS_U32 gId)
{
	E_GOP_API_Result eRet = GOP_API_FAIL;

	eRet = (E_GOP_API_Result) GOP_Win_Destroy(pInstance, gId);
	return eRet;
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
		msWarning(ERR_FB_ID_OUT_OF_RANGE);
		GOP_M_ERR
			     ("%s(u32fbId:%d....), fbId out of bound\n", __func__,
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
		msWarning(ERR_FB_ID_NOT_ALLOCATED);
		GOP_M_ERR
			     ("[%s][%d]: u32fbId=%d is not in existence\n", __func__, __LINE__,
			      (ptrdiff_t) u32fbId);
		return GWIN_NO_AVAILABLE;

	}

	GOP_FB_Destroy(pInstance, u32fbId);

	return GWIN_OK;

}

MS_U32 GOP_UpdateOnce(void *pInstance, MS_U32 gop, MS_BOOL bUpdateOnce, MS_BOOL bSync)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	g_pGOPCtxLocal->bUpdateRegOnce[GOP_PUBLIC_UPDATE] = bUpdateOnce;
	if (bUpdateOnce == FALSE)
		MDrv_GOP_GWIN_UpdateRegWithSync(g_pGOPCtxLocal, gop, bSync);

	return GOP_API_SUCCESS;

}

MS_U32 GOP_UpdateCurrentOnce(void *pInstance, MS_U16 u16GopMask, MS_BOOL bUpdateOnce, MS_BOOL bSync)
{
	MS_U16 u16GopIdx = 0;
	MS_U8 u8MaxGOPNum = 0;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	u8MaxGOPNum = MDrv_GOP_GetMaxGOPNum(g_pGOPCtxLocal);
	for (u16GopIdx = 0; u16GopIdx < u8MaxGOPNum; u16GopIdx++) {
		if ((u16GopMask & (1 << u16GopIdx)) != 0)
			g_pGOPCtxLocal->bUpdateRegOnce[u16GopIdx] = bUpdateOnce;
	}
	if (bUpdateOnce == FALSE)
		MDrv_GOP_GWIN_UpdateRegWithMaskSync(g_pGOPCtxLocal, u16GopMask, bSync);

	return GOP_API_SUCCESS;

}

MS_U32 Ioctl_GOP_TriggerRegWriteIn(void *pInstance, MS_U32 gop, EN_GOP_UPDATE_TYPE update_type,
				   MS_BOOL bEn, MS_BOOL bSync)
{

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (update_type) {
	case E_GOP_UPDATE_CURRENT_ONCE:
		{
			GOP_UpdateCurrentOnce(pInstance, gop, bEn, bSync);
			break;
		}
	default:
	case E_GOP_UPDATE_ONCE:
		{
			GOP_UpdateOnce(pInstance, gop, bEn, bSync);
			break;
		}

	}

	return GOP_API_SUCCESS;

}


MS_U32 Ioctl_GOP_Select(void *pInstance, EN_GOP_SELECT_TYPE sel, MS_U32 id, MS_U32 *pSet)
{
	DRV_GOP_CBFmtInfo *FmtInfo = (DRV_GOP_CBFmtInfo *) pSet;
	MS_U16 fbFmt;
	GOP_WinFB_INFO *pwinFB;
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (sel) {
	case EN_GOP_SEL_FB:
		{
			if (!_GOP_IsFbIdValid(pInstance, id)) {
				GOP_M_ERR("[%s][%d]FbId %td  is out of range\n", __func__,
					  __LINE__, (ptrdiff_t) id);
				return GOP_API_FAIL;
			}
			pwinFB = _GetWinFB(pInstance, id);

			if (pwinFB == NULL) {
				GOP_M_ERR("[%s][%d]GetWinFB Fail : WrongFBID=%td\n", __func__,
					  __LINE__, (ptrdiff_t) id);
				return GOP_API_FAIL;
			}

			if (pwinFB->in_use == 0) {
				msWarning(ERR_FB_ID_NOT_ALLOCATED);
				GOP_M_ERR("[%s][%d]: u32fbId=%td is not in existence\n",
					  __func__, __LINE__, (ptrdiff_t) id);
				return GOP_API_FAIL;
			}

			fbFmt = pwinFB->fbFmt;
			FmtInfo->u64Addr = pwinFB->addr;
			FmtInfo->u16Pitch = pwinFB->pitch;
			FmtInfo->u16Fmt = fbFmt;
			break;
		}
	default:
		break;
	}

	return GOP_API_SUCCESS;

}

MS_U32 Ioctl_GOP_MISC(void *pInstance, EN_GOP_MISC_TYPE type, MS_U32 *pSet, MS_U32 u32Size)
{
#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	switch (type) {
	case E_GOP_MISC_AT_EXIT:
		{
			if (u32Size != sizeof(MS_U32)) {
				GOP_M_ERR("[%s][%d] size ERROR!!\n", __func__, __LINE__);
				return GOP_API_FAIL;
			}
			Ioctl_GOP_AtExit(pInstance);
			break;
		}
	case E_GOP_MISC_SET_DBG_LEVEL:
		{
			if (u32Size != sizeof(MS_U32)) {
				GOP_M_ERR("[%s][%d] size ERROR!!\n", __func__, __LINE__);
				return GOP_API_FAIL;
			}
#ifndef STI_PLATFORM_BRING_UP
			u32GOPDbgLevel_mapi = (MS_U32) *pSet;
			MDrv_GOP_SetDbgLevel((EN_GOP_DEBUG_LEVEL) u32GOPDbgLevel_mapi);
#endif
			break;
		}
	case E_GOP_MISC_GET_OSD_NONTRANS_CNT:
		{
			if (u32Size != sizeof(MS_U32)) {
				GOP_M_ERR("[%s][%d] size ERROR!!\n", __func__, __LINE__);
				return GOP_API_FAIL;
			}
			MDrv_GOP_GetOsdNonTransCnt(g_pGOPCtxLocal, pSet);
			break;
		}
	default:
		break;
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
	if (pDrvGOPShared->apiCtxShared.bEnableVsyncIntFlip == FALSE) {
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
	}

	return u32Return;
}

MS_U32 Ioctl_GOP_SetLayer(void *pInstance, PGOP_SETLayer pstLayer, MS_U32 u32Size)
{
	MS_U16 u16Idx = 0;
	Gop_MuxSel enMux = E_GOP_MUX_INVALID;
	MS_BOOL bPreAlphaModeStatus;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	for (u16Idx = 0; u16Idx < pstLayer->u32LayerCount; u16Idx++) {
		MDrv_GOP_GetGOPEnable2Mode1(g_pGOPCtxLocal, (MS_U8) pstLayer->u32Gop[u16Idx],
					    &bPreAlphaModeStatus);
		MDrv_GOP_MapLayer2Mux(g_pGOPCtxLocal, pstLayer->u32Layer[u16Idx],
				      pstLayer->u32Gop[u16Idx], (MS_U32 *) &enMux);
		MDrv_GOP_GWIN_SetMux(g_pGOPCtxLocal, pstLayer->u32Gop[u16Idx], enMux);
		MDrv_GOP_SetGOPEnable2Mode1(g_pGOPCtxLocal, (MS_U8) pstLayer->u32Gop[u16Idx],
					    bPreAlphaModeStatus);
		if (g_pGOPCtxLocal->pGOPCtxShared->bGopHasInitialized[pstLayer->u32Gop[u16Idx]])
			_GOP_InitHSPDByGOP(pInstance, pstLayer->u32Gop[u16Idx]);
	}

	if (g_pGOPCtxLocal->pGopChipProperty->bOPMuxDoubleBuffer == TRUE)
		MDrv_GOP_GWIN_Trigger_MUX(g_pGOPCtxLocal);

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

void Ioctl_GOP_AtExit(void *pInstance)
{
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
	GOP_AtExit(pInstance);
#else
	GOP_M_INFO("not enable MSOS_TYPE_LINUX\n");
#endif
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

	if ((u32Cmd != MAPI_CMD_GOP_INTERRUPT) && (u32Cmd != MAPI_CMD_GOP_MUTEX)
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
	case MAPI_CMD_GOP_GET_CONFIG:
		{
			PGOP_SETCONFIG_PARAM ptr = (PGOP_SETCONFIG_PARAM) pArgs;

			u32Ret =
			    Ioctl_GOP_GetConfig(pInstance, ptr->cfg_type, ptr->pCfg, ptr->u32Size);
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
	case MAPI_CMD_GOP_GET_PROPERTY:
		{
			PGOP_SET_PROPERTY_PARAM ptr = (PGOP_SET_PROPERTY_PARAM) pArgs;


			u32Ret =
			    Ioctl_GOP_GetProperty(pInstance, ptr->en_pro, ptr->gop_idx,
						  ptr->pSetting, ptr->u32Size);

			break;
		}
	case MAPI_CMD_GOP_SET_DST:
		{
			PGOP_SETDST_PARAM ptr = (PGOP_SETDST_PARAM) pArgs;

			u32Ret = Ioctl_GOP_SetDst(pInstance, ptr->gop_idx, ptr->en_dst);
			break;
		}
	case MAPI_CMD_GOP_SET_MUX:
		{
			PGOP_SETMUX_PARAM ptr = (PGOP_SETMUX_PARAM) pArgs;
			//PGOP_MuxConfig  pMux = (PGOP_MuxConfig)ptr->pMuxInfo;
			PGOP_SETMUX pMux = (PGOP_SETMUX) ptr->pMuxInfo;
#ifndef MSOS_TYPE_LINUX_KERNEL	//Check size error when if(is_compat_task()==1)
			if (ptr->u32Size != sizeof(GOP_SETMUX) && g_pGOPCtxLocal != NULL) {
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
				UtopiaResourceRelease(psGOPInstPri->pResource);
				return UTOPIA_STATUS_PARAMETER_ERROR;
			}
#endif
			u32Ret = Ioctl_GOP_SetMux(pInstance, pMux, ptr->u32Size);
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

	case MAPI_CMD_GOP_GWIN_MAPFB2WIN:
		{
			PGOP_GWIN_MAPFBINFO_PARAM ptr = (PGOP_GWIN_MAPFBINFO_PARAM) pArgs;
			//PGOP_BUFFER_INFO pInfo = (PGOP_BUFFER_INFO)ptr->pinfo;

			u32Ret = Ioctl_GOP_MapFB2Win(pInstance, ptr->fbid, ptr->GwinId);

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
	case MAPI_CMD_GOP_GWIN_DESTROY:
		{
			PGOP_GWIN_DESTROY_PARAM ptr = (PGOP_GWIN_DESTROY_PARAM) pArgs;

			u32Ret = Ioctl_GOP_Win_Destroy(pInstance, ptr->GwinId);
		}
		break;
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
	case MAPI_CMD_GOP_UPDATE:
		{
			PGOP_UPDATE_PARAM ptr = (PGOP_UPDATE_PARAM) pArgs;
			PGOP_UPDATE_INFO pInfo = (PGOP_UPDATE_INFO) ptr->pUpdateInfo;

			if (ptr->u32Size != sizeof(GOP_UPDATE_INFO) && g_pGOPCtxLocal != NULL) {
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
				UtopiaResourceRelease(psGOPInstPri->pResource);
				return UTOPIA_STATUS_PARAMETER_ERROR;
			}

			u32Ret =
			    Ioctl_GOP_TriggerRegWriteIn(pInstance, pInfo->gop_idx,
							pInfo->update_type, pInfo->bEn,
							pInfo->bSync);
			break;
		}
	case MAPI_CMD_GOP_SELECTION:
		{
			PGOP_SELECTION_PROPERTY_PARAM ptr = (PGOP_SELECTION_PROPERTY_PARAM) pArgs;

			u32Ret = Ioctl_GOP_Select(pInstance, ptr->sel_type, ptr->id, ptr->pinfo);

			break;
		}
	case MAPI_CMD_GOP_MUTEX:
		{
			GOP_MUTEX_PARAM *ptr = (GOP_MUTEX_PARAM *) pArgs;

#ifndef MSOS_TYPE_LINUX_KERNEL	//Check size error when if(is_compat_task()==1)
			if (ptr->u32Size != sizeof(GOP_MUTEX_PARAM))
				u32Ret = UTOPIA_STATUS_PARAMETER_ERROR;
#endif
			if (ptr->en_mutex == E_GOP_LOCK) {
				if (UtopiaResourceObtain
				    (pModule, GOP_POOL_ID_GOP0, &(psGOPInstPri->pResource)) != 0) {
					GOP_M_ERR("[%s %d]UtopiaResourceObtainToInstant fail\n",
						  __func__, __LINE__);
					return 0xFFFFFFFF;
				}
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = MsOS_GetOSThreadID();
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = (GETPIDTYPE) getpid();
#endif
			} else if (ptr->en_mutex == E_GOP_UNLOCK) {
				g_pGOPCtxLocal->pGOPCtxShared->s32GOPLockTid = 0;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
				g_pGOPCtxLocal->pGOPCtxShared->GOPLockPid = 0;
#endif
				UtopiaResourceRelease(psGOPInstPri->pResource);
			}
			break;
		}
	case MAPI_CMD_GOP_MISC:
		{
			PGOP_MISC_PARAM ptr = (PGOP_MISC_PARAM) pArgs;

			u32Ret =
			    Ioctl_GOP_MISC(pInstance, ptr->misc_type, ptr->pMISC, ptr->u32Size);
			break;
		}
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
	Ioctl_GOP_AtExit(pInstance);
	UtopiaInstanceDelete(pInstance);

	return TRUE;
}
