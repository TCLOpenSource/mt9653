// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _DRV_GE_C_
#define _DRV_GE_C_

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include <linux/of.h>
#ifndef MSOS_TYPE_LINUX_KERNEL
#include <stdio.h>
#include <string.h>
#endif
#include <../mtk-g2d-utp-wrapper.h>
#include "sti_msos.h"
#include "apiGFX.h"
#include "drvGE.h"
#include "halGE.h"
#include "drvGE_private.h"
#include "regGE.h"
#include <linux/io.h>
#include <riu.h>
#include <GE0_2410.h>

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------
#if (GFX_UTOPIA20)
#define g_drvGECtxLocal     psGFXInstPriTmp->GFXPrivate_g_drvGECtxLocal
#else
GE_CTX_LOCAL g_drvGECtxLocal;
#endif

GE_CTX_SHARED g_drvGECtxShared;

GE_CTX_HAL_LOCAL g_halGECtxLocal;

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define SEM_HK51ID              0x01UL
#define SEM_HKAeonID            0x02UL
#define SEM_MWMipsID            0x03UL
#define GESEMID                 0x01UL
#define PREGESEMID              0x02UL

#define GE_ASSERT(_cnd, _ret, _fmt, _args...)                   \
	do {if (!(_cnd)) {                \
		G2D_ERROR(_fmt, ##_args);    \
		return _ret;                \
		}                                    \
	} while (0)

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
static void GE_BeginDraw(GE_Context *pGECtx, MS_BOOL bHWSemLock)
{
#if GE_USE_HW_SEM
	MS_U16 GE_HWSemaphore_count;
	MS_U16 GE_GetHWSemaphoreID;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *)pGECtx;

	if (pGECtxLocal->u32CTXInitMagic != 0x55aabeef)
		return;

	if (pGECtxLocal->s32GEMutex < 0)
		return;

	if ((pGECtxLocal->s32GE_Recursive_Lock_Cnt > 0) &&
			(pGECtxLocal->s32GELock == MsOS_GetOSThreadID())) {
		pGECtxLocal->s32GE_Recursive_Lock_Cnt++;
	} else {
		/*Step 1: Check Mutex*/
		while (!MsOS_ObtainMutex(pGECtxLocal->s32GEMutex, MSOS_WAIT_FOREVER))
			G2D_ERROR("Obtian GE Mutex Fail\n");


		/*HWSemaphore*/
		if (bHWSemLock == TRUE) {
		/*Step 1: Check HWSemaphore*/
			while (MDrv_SEM_Get_Resource(GESEMID, pGECtxLocal->u32GESEMID) == FALSE) {
				GE_HWSemaphore_count++;
				if (GE_HWSemaphore_count > 10000) {
					MDrv_SEM_Get_ResourceID(GESEMID, &GE_GetHWSemaphoreID);
					G2D_ERROR("[Beg]HWSemID=%tx, GESEMID=%tx, TID=0x%tx\n",
					(ptrdiff_t)GE_GetHWSemaphoreID,
					(ptrdiff_t)pGECtxLocal->u32GESEMID,
					(ptrdiff_t)(MS_S32)MsOS_GetOSThreadID());
					GE_HWSemaphore_count = 0;
				}
			}
		}
		 /*HWSemaphore*/
		pGECtxLocal->s32GELock = MsOS_GetOSThreadID();
		pGECtxLocal->s32GE_Recursive_Lock_Cnt++;
		//printf("[%s][%05d][%ld]GFX_LOCK+++[%ld]\n",__func__,__LINE__,
		//pGECtxLocal->s32GELock, pGECtxLocal->s32GE_Recursive_Lock_Cnt);
		if (pGECtxLocal->pSharedCtx->u32LstGEClientId != pGECtxLocal->u32GEClientId) {
			GE_Restore_HAL_Context(&pGECtxLocal->halLocalCtx,
				pGECtxLocal->pSharedCtx->bNotFirstInit);
			pGECtxLocal->pSharedCtx->u32LstGEClientId = pGECtxLocal->u32GEClientId;
		}
	}


#else
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *)pGECtx;

	if (pGECtxLocal->u32CTXInitMagic != 0x55aabeef)
		return;

	if (pGECtxLocal->s32GEMutex < 0)
		return;

	if ((pGECtxLocal->s32GE_Recursive_Lock_Cnt > 0) &&
		(pGECtxLocal->s32GELock == MsOS_GetOSThreadID())) {
		pGECtxLocal->s32GE_Recursive_Lock_Cnt++;
	} else  {
		/*Step 1: Check Mutex*/
		while (!MsOS_ObtainMutex())
			G2D_ERROR("Obtian GE Mutex Fail\n");


		pGECtxLocal->s32GELock = MsOS_GetOSThreadID();
		pGECtxLocal->s32GE_Recursive_Lock_Cnt++;
		//printf("[%s][%05d][%ld]GFX_LOCK+++[%ld]\n", __func__, __LINE__,
		//pGECtxLocal->s32GELock, pGECtxLocal->s32GE_Recursive_Lock_Cnt);
		if (pGECtxLocal->pSharedCtx->u32LstGEClientId != pGECtxLocal->u32GEClientId) {
			GE_Restore_HAL_Context(&pGECtxLocal->halLocalCtx,
				pGECtxLocal->pSharedCtx->bNotFirstInit);
			pGECtxLocal->pSharedCtx->u32LstGEClientId = pGECtxLocal->u32GEClientId;
		}
	}
#endif
}

static void GE_EndDraw(GE_Context *pGECtx, MS_BOOL bHWSemLock)
{
#if GE_USE_HW_SEM
	MS_U16 GE_HWSemaphore_count;
	MS_U16 GE_GetHWSemaphoreID;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *)pGECtx;

	if (pGECtxLocal->s32GELock != MsOS_GetOSThreadID())
		G2D_ERROR("\nGE_RETURN: Fatal Error, Task ID mismatch[%td]->[%td]\n",
		(ptrdiff_t)pGECtxLocal->s32GELock, (ptrdiff_t)(MS_S32)MsOS_GetOSThreadID());
	if (pGECtxLocal->s32GE_Recursive_Lock_Cnt <= 0)
		G2D_ERROR("\nGE_RETURN: Fatal Error, No Mutex to release[Cnt=%td]\n",
		(ptrdiff_t)pGECtxLocal->s32GE_Recursive_Lock_Cnt);

	if (pGECtxLocal->s32GE_Recursive_Lock_Cnt > 1) {
		pGECtxLocal->s32GE_Recursive_Lock_Cnt--;
	} else {
		pGECtxLocal->s32GE_Recursive_Lock_Cnt--;
		/*HWSemaphore*/
		if (bHWSemLock == TRUE) {
			while (MDrv_SEM_Free_Resource(GESEMID, pGECtxLocal->u32GESEMID) == FALSE) {
				GE_HWSemaphore_count++;
				if (GE_HWSemaphore_count > 10000) {
					MDrv_SEM_Get_ResourceID(GESEMID, &GE_GetHWSemaphoreID);
					G2D_ERROR("[End]HWSemID=%tx, GESEMID=%tx, TID=0x%tx\n",
					(ptrdiff_t)GE_GetHWSemaphoreID,
					(ptrdiff_t)pGECtxLocal->u32GESEMID,
					(ptrdiff_t)(MS_S32)MsOS_GetOSThreadID());
					GE_HWSemaphore_count = 0;
				}
			}
		}
		/*HWSemaphore*/
		while (!MsOS_ReleaseMutex(pGECtxLocal->s32GEMutex))
			G2D_ERROR("Release GE Mutex Fail\n");

	}

#else
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *)pGECtx;

	if (pGECtxLocal->s32GELock != MsOS_GetOSThreadID())
		G2D_ERROR("\nGE_RETURN: Fatal Error, Task ID mismatch[%td]->[%td]\n",
		(ptrdiff_t)pGECtxLocal->s32GELock, (ptrdiff_t)(MS_S32)MsOS_GetOSThreadID());
	if (pGECtxLocal->s32GE_Recursive_Lock_Cnt <= 0)
		G2D_ERROR("\nGE_RETURN: Fatal Error, No Mutex to release[Cnt=%td]\n",
		(ptrdiff_t)pGECtxLocal->s32GE_Recursive_Lock_Cnt);

	if (pGECtxLocal->s32GE_Recursive_Lock_Cnt > 1)  {
		pGECtxLocal->s32GE_Recursive_Lock_Cnt--;
	} else {
		pGECtxLocal->s32GE_Recursive_Lock_Cnt--;
		while (!MsOS_ReleaseMutex())
			G2D_ERROR("Release GE Mutex Fail\n");
	}

#endif
}

static MS_BOOL GE_FastClip(GE_Context *pGECtx, MS_U16 x0, MS_U16 y0, MS_U16 x1, MS_U16 y1)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	if (((x0 < pGECtxLocal->u16ClipL) && (x1 < pGECtxLocal->u16ClipL)) ||
	    ((x0 > pGECtxLocal->u16ClipR) && (x1 > pGECtxLocal->u16ClipR)) ||
	    ((y0 < pGECtxLocal->u16ClipT) && (y1 < pGECtxLocal->u16ClipT)) ||
	    ((y0 > pGECtxLocal->u16ClipB) && (y1 > pGECtxLocal->u16ClipB))) {
		return TRUE;	// can't be clipped
	}
	return FALSE;
}

static MS_BOOL GE_RectOverlap(GE_CTX_LOCAL *pGECtxLocal, GE_Rect *rect0, GE_DstBitBltType *rect1)
{
	if ((pGECtxLocal->PhySrcAddr != pGECtxLocal->PhyDstAddr) ||
	    (rect0->x + rect0->width - 1 < rect1->dstblk.x) ||
	    (rect0->x > rect1->dstblk.x + rect1->dstblk.width - 1) ||
	    (rect0->y + rect0->height - 1 < rect1->dstblk.y) ||
	    (rect0->y > rect1->dstblk.y + rect1->dstblk.height - 1)) {
		return FALSE;	// no overlap
	}

	return TRUE;
}

static MS_U16 GE_GetFmt(GE_BufFmt fmt)
{
	switch (fmt) {
	case E_GE_FMT_I1:
		return GE_FMT_I1;
	case E_GE_FMT_I2:
		return GE_FMT_I2;
	case E_GE_FMT_I4:
		return GE_FMT_I4;
	case E_GE_FMT_I8:
		return GE_FMT_I8;
	case E_GE_FMT_RGB332:
		return E_GE_FMT_RGB332;
	case E_GE_FMT_FaBaFgBg2266:
		return GE_FMT_FaBaFgBg2266;
	case E_GE_FMT_1ABFgBg12355:
		return GE_FMT_1ABFgBg12355;
	case E_GE_FMT_RGB565:
		return GE_FMT_RGB565;
	case E_GE_FMT_ARGB1555:
		return GE_FMT_ARGB1555;
	case E_GE_FMT_ARGB4444:
		return GE_FMT_ARGB4444;
	case E_GE_FMT_ARGB1555_DST:
		return GE_FMT_ARGB1555_DST;
	case E_GE_FMT_YUV422:
		return GE_FMT_YUV422;
	case E_GE_FMT_ARGB8888:
		return GE_FMT_ARGB8888;
	case E_GE_FMT_RGBA5551:
		return GE_FMT_RGBA5551;
	case E_GE_FMT_RGBA4444:
		return GE_FMT_RGBA4444;
	case E_GE_FMT_ABGR8888:
		return GE_FMT_ABGR8888;
	case E_GE_FMT_ABGR1555:
		return GE_FMT_ABGR1555;
	case E_GE_FMT_BGRA5551:
		return GE_FMT_BGRA5551;
	case E_GE_FMT_ABGR4444:
		return GE_FMT_ABGR4444;
	case E_GE_FMT_BGRA4444:
		return GE_FMT_BGRA4444;
	case E_GE_FMT_BGR565:
		return GE_FMT_BGR565;
	case E_GE_FMT_RGBA8888:
		return GE_FMT_RGBA8888;
	case E_GE_FMT_BGRA8888:
		return GE_FMT_BGRA8888;
	case E_GE_FMT_ACRYCB444:
		return E_GE_FMT_ACRYCB444;
	case E_GE_FMT_CRYCBA444:
		return E_GE_FMT_CRYCBA444;
	case E_GE_FMT_ACBYCR444:
		return E_GE_FMT_ACBYCR444;
	case E_GE_FMT_CBYCRA444:
		return E_GE_FMT_CBYCRA444;
	default:
		return GE_FMT_GENERIC;
	}
}

MS_U32 GE_Divide2Fixed(MS_U16 u16x, MS_U16 u16y, MS_U8 nInteger, MS_U8 nFraction)
{
	MS_U8 neg = 0;
	MS_U32 mask;
	MS_U32 u32x;
	MS_U32 u32y;

	if (u16x == 0)
		return 0;

	if (u16x & 0x8000) {
		u32x = 0xFFFF0000 | u16x;
		u32x = ~u32x + 1;	//convert to positive
		neg++;
	} else {
		u32x = u16x;
	}

	if (u16y & 0x8000) {
		u32y = 0xFFFF0000 | u16y;
		u32y = ~u32y + 1;	//convert to positive
		neg++;
	} else {
		u32y = u16y;
	}

	// start calculation
	u32x = (u32x << nFraction) / u32y;
	if (neg % 2)
		u32x = ~u32x + 1;

	// total bit number is: 1(s) + nInteger + nFraction
	mask = (1 << (1 + nInteger + nFraction)) - 1;
	u32x &= mask;

	return u32x;
}

static void GE_CalcColorDelta(MS_U32 color0, MS_U32 color1, MS_U16 ratio, GE_ColorDelta *delta)
{
	MS_U8 a0, r0, g0, b0;
	MS_U8 a1, r1, g1, b1;


	// Get A,R,G,B
	//[TODO] special format
	b0 = (color0) & 0xFF;
	g0 = (color0 >> 8) & 0xFF;
	r0 = (color0 >> 16) & 0xFF;
	a0 = (color0 >> 24);
	b1 = (color1) & 0xFF;
	g1 = (color1 >> 8) & 0xFF;
	r1 = (color1 >> 16) & 0xFF;
	a1 = (color1 >> 24);

	//[TODO] revise and take advantage on Divid2Fixed for negative value
	if (b0 > b1) {
		delta->b = GE_Divide2Fixed((b0 - b1), ratio, 7, 12);
		delta->b = (MS_U32) (1 << (1 + 7 + 12)) - delta->b;	// negative
	} else {
		delta->b = GE_Divide2Fixed((b1 - b0), ratio, 7, 12);
	}

	if (g0 > g1) {
		delta->g = GE_Divide2Fixed((g0 - g1), ratio, 7, 12);
		delta->g = (MS_U32) (1 << (1 + 7 + 12)) - delta->g;	// negative
	} else {
		delta->g = GE_Divide2Fixed((g1 - g0), ratio, 7, 12);
	}

	if (r0 > r1) {
		delta->r = GE_Divide2Fixed((r0 - r1), ratio, 7, 12);
		delta->r = (MS_U32) (1 << (1 + 7 + 12)) - delta->r;	// negative
	} else {
		delta->r = GE_Divide2Fixed((r1 - r0), ratio, 7, 12);
	}

	if (a0 > a1) {
		delta->a = (MS_U16) GE_Divide2Fixed((a0 - a1), ratio, 4, 11);
		delta->a = (MS_U16) (1 << (1 + 4 + 11)) - delta->a;	// negative
	} else {
		delta->a = (MS_U16) GE_Divide2Fixed((a1 - a0), ratio, 4, 11);
	}
}

static GE_Result GE_SetPalette(GE_CTX_LOCAL *pGECtxLocal)
{
	MS_U32 u32Idx = 0;

	for (u32Idx = 0; u32Idx < GE_PALETTE_NUM; u32Idx++) {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00B8_GE0,
			    ByteSwap16(pGECtxLocal->halLocalCtx.u32Palette[u32Idx]) & 0xFFFF);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00B4_GE0,
			    ByteSwap16(pGECtxLocal->halLocalCtx.u32Palette[u32Idx] >> 16));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00BC_GE0,
			    ((u32Idx) & GE_CLUT_CTRL_IDX_MASK) | GE_CLUT_CTRL_WR);
	}

	return E_GE_OK;
}

static GE_Result GE_Restore_PaletteContext(GE_CTX_LOCAL *pGECtxLocal)
{
	if (pGECtxLocal->bSrcPaletteOn == TRUE) {
		if ((pGECtxLocal->u32GESEMID != pGECtxLocal->u16GEPrevSEMID)
		    || (pGECtxLocal->halLocalCtx.bPaletteDirty == TRUE)
		    || ((pGECtxLocal->pSharedCtx->u32LstGEPaletteClientId
		    != pGECtxLocal->u32GEClientId)
		    && pGECtxLocal->pSharedCtx->u32LstGEPaletteClientId)) {
			GE_SetPalette(pGECtxLocal);
			pGECtxLocal->pSharedCtx->u32LstGEPaletteClientId =
			    pGECtxLocal->u32GEClientId;
			pGECtxLocal->halLocalCtx.bPaletteDirty = FALSE;
			pGECtxLocal->bSrcPaletteOn = FALSE;
		}
	}

	return (E_GE_OK);
}

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// Allocate semaphore resource for GE
/// @param  pGECtx                     \b IN: GE context
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result GE_Get_Resource(GE_Context *pGECtx, MS_BOOL bHWSemLock)
{
	GE_BeginDraw(pGECtx, bHWSemLock);
	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Release semaphore resource occupied by GE
/// @param  pGECtx                     \b IN: GE context
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result GE_Free_Resource(GE_Context *pGECtx, MS_BOOL bHWSemLock)
{
	GE_EndDraw(pGECtx, bHWSemLock);
	return E_GE_OK;
}


GE_Result MDrv_GE_Get_Semaphore(GE_Context *pGECtx, MS_U32 u32RPoolID)
{
#if GE_RESOURCE_SEM
	void *pModule = NULL;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	if (pGECtxLocal->ctxHeader.bGEMode4MultiProcessAccess == TRUE) {
		if (u32RPoolID >= E_GE_POOL_ID_MAX) {
			GE_D_ERR("[%s %d]Unknown GE Pool ID!", __func__, __LINE__);
			return E_GE_FAIL;
		}

		UtopiaInstanceGetModule(pGECtxLocal->ctxHeader.pInstance, &pModule);

		/*In order to store resource addr of each instance */
		if (UtopiaResourceObtain
		    (pModule, E_GE_POOL_ID_INTERNAL_REGISTER,
		     &g_pGFXResource[u32RPoolID]) != UTOPIA_STATUS_SUCCESS) {
			GFX_CRITIAL_MSG(GE_D_ERR
					("[%s:%s:%d]UtopiaResourceObtainToInstant fail\n", __FILE__,
					 __func__, __LINE__));
			return E_GE_FAIL;
		}
	}
#endif
	return E_GE_OK;

}

GE_Result MDrv_GE_Free_Semaphore(GE_Context *pGECtx, MS_U32 u32RPoolID)
{
#if GE_RESOURCE_SEM
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	if (pGECtxLocal->ctxHeader.bGEMode4MultiProcessAccess == TRUE) {
		if (u32RPoolID >= E_GE_POOL_ID_MAX) {
			GE_D_ERR("[%s %d]Unknown GE Pool ID!", __func__, __LINE__);
			return E_GE_FAIL;
		}

		UtopiaResourceRelease(g_pGFXResource[u32RPoolID]);
	}
#endif
	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Set GE dither
/// @param  pGECtx                     \b IN: GE context
/// @param  enable                  \b IN: enable and update setting
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetDither(GE_Context *pGECtx, MS_BOOL enable)
{
	MS_U16 u16en;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	u16en = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0);
	if (enable)
		u16en |= GE_EN_DITHER;
	else
		u16en &= ~GE_EN_DITHER;

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0, u16en);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Set GE One pixel Mode
/// @param  pGECtx                     \b IN: GE context
/// @param  enable                  \b IN: enable and update setting
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetOnePixelMode(GE_Context *pGECtx, MS_BOOL enable)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;
	GE_Result geResult;

	geResult = GE_SetOnePixelMode(&pGECtxLocal->halLocalCtx, enable);

	return geResult;
}


//-------------------------------------------------------------------------------------------------
/// Set GE source color key
/// @param  pGECtx                     \b IN: GE context
/// @param  enable                  \b IN: enable and update setting
/// @param  eCKOp                   \b IN: source color key mode
/// @param  ck_low                  \b IN: lower value of color key (E_GE_FMT_ARGB8888)
/// @param  ck_high                 \b IN: upper value of color key (E_GE_FMT_ARGB8888)
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>color key does not check alpha channel</em></b>
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetSrcColorKey(GE_Context *pGECtx, MS_BOOL enable, GE_CKOp eCKOp, MS_U32 ck_low,
				 MS_U32 ck_high)
{
	MS_U16 u16en = 0, u16op;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	u16op = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0070_GE0);

	switch (eCKOp) {
	case E_GE_CK_EQ:
		u16en = GE_EN_SCK;
		u16op &= ~(GE_BIT0);
		break;
	case E_GE_CK_NE:
		u16en = GE_EN_SCK;
		u16op |= GE_BIT0;
		break;
	case E_GE_ALPHA_EQ:
		u16en = GE_EN_ASCK;
		u16op &= ~(GE_BIT2);
		break;
	case E_GE_ALPHA_NE:
		u16en = GE_EN_ASCK;
		u16op |= GE_BIT2;
		break;
	default:
		break;
	}

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0070_GE0, u16op);

	if (enable) {
		// Color key threshold
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0058_GE0, ck_low & 0xFFFF);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_005C_GE0, (ck_low >> 16));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0050_GE0, (ck_high & 0xFFFF));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0054_GE0, (ck_high >> 16));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0,
			    (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0) | u16en));
	} else {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0,
			    (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0) & (~u16en)));
	}

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Set GE destination color key
/// @param  pGECtx                     \b IN: GE context
/// @param  enable                  \b IN: enable and update setting
/// @param  eCKOp                   \b IN: source color key mode
/// @param  ck_low                  \b IN: lower value of color key (E_GE_FMT_ARGB8888)
/// @param  ck_high                 \b IN: upper value of color key (E_GE_FMT_ARGB8888)
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>color key does not check alpha channel</em></b>
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetDstColorKey(GE_Context *pGECtx, MS_BOOL enable, GE_CKOp eCKOp, MS_U32 ck_low,
				 MS_U32 ck_high)
{
	MS_U16 u16en = 0, u16op;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	u16op = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0070_GE0);

	switch (eCKOp) {
	case E_GE_CK_EQ:
		u16en = GE_EN_DCK;
		u16op &= ~(GE_BIT1);
		break;
	case E_GE_CK_NE:
		u16en = GE_EN_DCK;
		u16op |= GE_BIT1;
		break;
	case E_GE_ALPHA_EQ:
		u16en = GE_EN_DSCK;
		u16op &= ~(GE_BIT3);
		break;
	case E_GE_ALPHA_NE:
		u16en = GE_EN_DSCK;
		u16op |= GE_BIT3;
		break;
	default:
		break;
	}

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0070_GE0, u16op);

	if (enable) {
		// Color key threshold
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0068_GE0, (ck_low & 0xFFFF));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_006C_GE0, (ck_high >> 16));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0060_GE0, (ck_low & 0xFFFF));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0064_GE0, (ck_high >> 16));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0,
			    (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0) | u16en));
	} else {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0,
			    (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0) & (~u16en)));
	}

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Set GE I1,I2,I4 color palette
/// @param  pGECtx                     \b IN: GE context
/// @param  idx                     \b IN: index of palette
/// @param  color                   \b IN: intensity color format (E_GE_FMT_ARGB8888, E_GE_FMT_I8)
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetIntensity(GE_Context *pGECtx, MS_U32 idx, MS_U32 color)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	if (idx < 16) {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx,
			(REG_00D4_GE0 + (8*idx)), (color & 0xFFFF));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx,
			(REG_00D8_GE0 + (8*idx)), (color >> 16));
	} else {
		return E_GE_FAIL_PARAM;
	}

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Get GE I1,I2,I4 color palette
/// @param  pGECtx                     \b IN: GE context
/// @param  idx                     \b IN: index of palette
/// @param  color                   \b IN: intensity color format (E_GE_FMT_ARGB8888, E_GE_FMT_I8)
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_GetIntensity(GE_Context *pGECtx, MS_U32 idx, MS_U32 *color)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;


	if (idx < 16) {
		*color =
		    ((MS_U32) GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, (REG_00D4_GE0 + (8*idx))))
		    << 16 |
		    (MS_U32) GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, (REG_00D8_GE0 + (8*idx)));
	} else {
		return E_GE_FAIL_PARAM;
	}

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Set GE ROP2
/// @param  pGECtx                     \b IN: GE context
/// @param  enable                  \b IN: enable and update setting
/// @param  eRop2                   \b IN: ROP2 op
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetROP2(GE_Context *pGECtx, MS_BOOL enable, GFX_ROP2_Op eRop2)
{
	MS_U16 u16en, u16rop2;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	u16en = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0);
	if (enable) {
		u16en |= GE_EN_ROP2;

		u16rop2 =
		    (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0040_GE0) & ~GE_ROP2_MASK)
				| eRop2;
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0040_GE0, u16rop2);
	} else {
		u16en &= ~GE_EN_ROP2;
	}
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0, u16en);

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Set GE line pattern style
/// @param  pGECtx                     \b IN: GE context
/// @param  enable                  \b IN: enable and update setting
/// @param  pattern                 \b IN: 0-0x3F bitmap to represent draw(1) or not draw(0)
/// @param  eRep                    \b IN: repeat pattern once, or 2,3,4 times
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetLinePattern(GE_Context *pGECtx, MS_BOOL enable, MS_U8 pattern,
				 GE_LinePatRep eRep)
{
	MS_U16 u16en, u16style;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	u16en = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0);
	if (enable) {
		u16en |= GE_EN_LINEPAT;

		u16style = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0) &
				GE_LINEPAT_RST;
		u16style |=
		    ((pattern & GE_LINEPAT_MASK) |
		     ((eRep << GE_LINEPAT_REP_SHFT) & GE_LINEPAT_REP_MASK));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0, u16style);
	} else {
		u16en &= ~GE_EN_LINEPAT;

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0, GE_LINEPAT_RST);
	}
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0, u16en);

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Reset GE line pattern for next line drawing
/// @param  pGECtx                     \b IN: GE context
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_ResetLinePattern(GE_Context *pGECtx)
{
	MS_U16 u16style;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	u16style = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0) | GE_LINEPAT_RST;
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0, u16style);

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Set GE alpha blending of color output.
/// @param  pGECtx                     \b IN: GE context
/// @param  enable                  \b IN: enable and update setting
/// @param  eBlendOp                \b IN: coef and op for blending
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>It does not support ROP8 operation</em></b>
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetAlphaBlend(GE_Context *pGECtx, MS_BOOL enable, GE_BlendOp eBlendOp)
{
	MS_U16 u16en;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	u16en = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0);
	if (enable) {
		u16en |= GE_EN_BLEND;

		if (GE_SetBlend(&pGECtxLocal->halLocalCtx, eBlendOp) != E_GE_OK)
			return E_GE_FAIL_PARAM;

	} else {
		u16en &= ~GE_EN_BLEND;
	}
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0, u16en);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Set GE alpha blending of color output.
/// @param  pGECtx                     \b IN: GE context
/// @param  eBlendOp                \b IN: coef and op for blending
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>It does not support ROP8 operation</em></b>
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetAlphaBlendCoef(GE_Context *pGECtx, GE_BlendOp eBlendOp)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	if (GE_SetBlend(&pGECtxLocal->halLocalCtx, eBlendOp) != E_GE_OK)
		return E_GE_FAIL_PARAM;

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Enable GFX DFB blending
/// @param enable \b IN: true/false
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_EnableDFBBlending(GE_Context *pGECtx, MS_BOOL enable)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;
	GE_Result geResult;

	geResult = GE_EnableDFBBld(&pGECtxLocal->halLocalCtx, enable);

	return geResult;
}

//-------------------------------------------------------------------------------------------------
/// Set GE DFB blending Flags.
/// @param u16DFBBldFlag       \b IN: DFB Blending Flags. The Flags will be
///                                   The combination of Flags
///                                   [GFX_DFB_BLD_FLAG_COLORALPHA, GFX_DFB_BLD_FLAG_ALPHACHANNEL,
///                                    GFX_DFB_BLD_FLAG_COLORIZE, GFX_DFB_BLD_FLAG_SRCPREMUL,
///                                    GFX_DFB_BLD_FLAG_SRCPREMULCOL, GFX_DFB_BLD_FLAG_DSTPREMUL,
///                                    GFX_DFB_BLD_FLAG_XOR, GFX_DFB_BLD_FLAG_DEMULTIPLY,
///                                    GFX_DFB_BLD_FLAG_SRCALPHAMASK, GFX_DFB_BLD_FLAG_SRCCOLORMASK]
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetDFBBldFlags(GE_Context *pGECtx, MS_U16 u16DFBBldFlags)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;
	GE_Result geResult;

	geResult = GE_SetDFBBldFlags(&pGECtxLocal->halLocalCtx, u16DFBBldFlags);

	return geResult;
}

//-------------------------------------------------------------------------------------------------
/// Set GFX DFB blending Functions/Operations.
/// @param gfxSrcBldOP \b IN: source blending op
/// @param gfxDstBldOP \b IN: dst blending op
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetDFBBldOP(GE_Context *pGECtx, GE_DFBBldOP geSrcBldOP, GE_DFBBldOP geDstBldOP)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;
	GE_Result geResult;

	geResult = GE_SetDFBBldOP(&pGECtxLocal->halLocalCtx, geSrcBldOP, geDstBldOP);

	return geResult;
}

//-------------------------------------------------------------------------------------------------
/// Set GFX DFB blending const color.
/// @param u32ConstColor \b IN: DFB Blending constant color
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetDFBBldConstColor(GE_Context *pGECtx, GE_RgbColor geRgbColor)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;
	GE_Result geResult;

	geResult = GE_SetDFBBldConstColor(&pGECtxLocal->halLocalCtx, geRgbColor);

	return geResult;
}

//-------------------------------------------------------------------------------------------------
/// Set GE clipping window
/// @param  pGECtx                     \b IN: GE context
/// @param rect                     \b IN: pointer to RECT of clipping window
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetClipWindow(GE_Context *pGECtx, GE_Rect *rect)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_ASSERT((rect), E_GE_FAIL_PARAM, "%s: Null pointer\n", __func__);

	pGECtxLocal->u16ClipL = rect->x;
	pGECtxLocal->u16ClipT = rect->y;
	pGECtxLocal->u16ClipR = (rect->x + rect->width - 1);
	pGECtxLocal->u16ClipB = (rect->y + rect->height - 1);

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0154_GE0, pGECtxLocal->u16ClipL);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_015C_GE0, pGECtxLocal->u16ClipT);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0158_GE0, pGECtxLocal->u16ClipR);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0160_GE0, pGECtxLocal->u16ClipB);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Set GE constant alpha value for any alpha operation
/// @param  pGECtx                     \b IN: GE context
/// @param  aconst                  \b IN: constant alpha value for output alpha and blending
/// @return @ref GE_Result
/// @sa MDrv_GE_SetAlphaBlend, MDrv_GE_SetAlphaSrc
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetAlphaConst(GE_Context *pGECtx, MS_U8 aconst)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_004C_GE0, (MS_U16) (aconst & 0xff));

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Set GE alpha source for alpha output. <b><em>Aout</em></b>
/// @param  pGECtx                     \b IN: GE context
/// @param  eAlphaSrc               \b IN: alpha channel comes from
/// @return @ref GE_Result
/// @note
/// <b>(REQUIREMENT) <em>E_GE_ALPHA_ADST requires AlphaBlending(TRUE, )</em></b>
/// @attention
/// <b>[URANUS] <em>It does not support ROP8 operation</em></b>
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetAlphaSrc(GE_Context *pGECtx, GFX_AlphaSrcFrom eAlphaSrc)
{
	GE_Result ret;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	ret = GE_SetAlpha(&pGECtxLocal->halLocalCtx, eAlphaSrc);

	return ret;
}


//-------------------------------------------------------------------------------------------------
/// Set GE alpha comparison before alpha output. <b><em>Aout = cmp(Aout, Adst)</em></b>
/// @param  pGECtx                     \b IN: GE context
/// @param  enable                  \b IN: enable and update setting
/// @param  eACmpOp                 \b IN: MAX / MIN
/// @return @ref GE_Result
/// @note
/// <b>(REQUIREMENT) <em>It requires AlphaBlending(TRUE, )</em></b>
/// @sa MDrv_GE_SetAlphaSrc
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetAlphaCmp(GE_Context *pGECtx, MS_BOOL enable, GE_ACmpOp eACmpOp)
{
	MS_U16 u16en, u16op;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	u16en = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0);
	if (enable) {
		u16en |= GE_EN_ACMP;

		u16op =
		    (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0070_GE0) & ~GE_OP_ACMP_MIN) |
		    ((eACmpOp << GE_OP_ACMP_SHFT) & GE_OP_ACMP_MIN);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0070_GE0, u16op);
	} else {
		u16en &= ~GE_EN_ACMP;
	}
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0, u16en);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Set GE color to palette table
/// @param  pGECtx                     \b IN: GE context
/// @param  start                   \b IN: start index of palette (0 - GE_PALETTE_NUM)
/// @param  num                     \b IN: number of palette entry to set
/// @param  palette                 \b IN: user defined palette color table
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>It does not support palette</em></b>
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetPalette(GE_Context *pGECtx, MS_U16 start, MS_U16 num, MS_U32 *palette)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;
	MS_U32 i;

	GE_ASSERT((palette), E_GE_FAIL_PARAM, "%s: Null pointer\n", __func__);

	if ((start + num) > GE_PALETTE_NUM)
		return E_GE_FAIL_PARAM;

	for (i = 0; i < num; i++)
		pGECtxLocal->halLocalCtx.u32Palette[start + i] = palette[i];

	pGECtxLocal->halLocalCtx.bPaletteDirty = TRUE;

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Get GE color from palette table
/// @param  pGECtx                     \b IN: GE context
/// @param  start                   \b IN: start index of palette (0 - GE_PALETTE_NUM)
/// @param  num                     \b IN: number of palette entry to set
/// @param  palette                 \b OUT: user buffer to get palette data
/// @return @ref GE_Result
/// <b>[URANUS] <em>It does not support palette</em></b>
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_GetPalette(GE_Context *pGECtx, MS_U16 start, MS_U16 num, MS_U32 *palette)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;
	MS_U32 i;
	//MS_U32 palette[16]; //NOW ACQUIRE INDEX 0~15 with E_GE_READ_DIRECT mode

	GE_ASSERT((palette), E_GE_FAIL_PARAM, "%s: Null pointer\n", __func__);

	if ((num - start + 1) > GE_PALETTE_NUM)
		return E_GE_FAIL_PARAM;

	// Wait command queue flush
	for (i = 0; i < (num - start + 1); i++)
		palette[i] = pGECtxLocal->halLocalCtx.u32Palette[start + i];

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Set GE YUV operation mode
/// @param  pGECtx                     \b IN: GE context
/// @param  mode                    \b IN: pointer to YUV setting structure.
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>It does not support RGB2YUV conversion</em></b>
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetYUVMode(GE_Context *pGECtx, GE_YUVMode *mode)
{
	MS_U16 u16YuvMode = 0, u16Reg = 0;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_ASSERT((mode), E_GE_FAIL_PARAM, "%s: Null pointer\n", __func__);

	u16Reg = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_007C_GE0);
	u16Reg &= ~(GE_YUV_CSC_MASK);
	u16YuvMode |= mode->rgb2yuv << GE_YUV_RGB2YUV_SHFT;
	u16YuvMode |= mode->out_range << GE_YUV_OUT_RANGE_SHFT;
	u16YuvMode |= mode->in_range << GE_YUV_IN_RANGE_SHFT;

#ifdef GE_UV_Swap_PATCH
	if (((mode->src_fmt == E_GE_YUV_YVYU) && (mode->dst_fmt == E_GE_YUV_YUYV))
	    || ((mode->src_fmt == E_GE_YUV_YUYV) && (mode->dst_fmt == E_GE_YUV_YVYU))) {
		mode->src_fmt = E_GE_YUV_YVYU;
		mode->dst_fmt = E_GE_YUV_YUYV;
	}
#endif

	u16YuvMode |= mode->src_fmt << GE_YUV_SRC_YUV422_SHFT;
	u16YuvMode |= mode->dst_fmt << GE_YUV_DST_YUV422_SHFT;

#ifdef GE_UV_FILTER_PATCH
	u16YuvMode |= 0x2;
#endif

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_007C_GE0, u16Reg | u16YuvMode);

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// Set GE source buffer info
/// @param  pGECtx                     \b IN: GE context
/// @param  src_fmt                 \b IN: source buffer format
/// @param  pix_width               \b IN: pixel width of source buffer (RESERVED)
/// @param  pix_height              \b IN: pixel height of source buffer (RESERVED)
/// @param  addr                    \b IN: source buffer start address [23:0]
/// @param  pitch                   \b IN: source buffer linear pitch in byte unit
/// @param  flags                   \b IN: option flag of dst buffer <b>(RESERVED)</b>
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>E_GE_FMT_ARGB1555 is RGB555</em></b>
/// <b>[URANUS] <em>E_GE_FMT_* should be 8-byte addr/pitch aligned to do stretch bitblt</em></b>
/// @sa MDrv_GE_GetFmtCaps
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetSrcBuffer(GE_Context *pGECtx, GE_BufFmt src_fmt, MS_U16 pix_width,
			       MS_U16 pix_height, MS_PHY addr, MS_U16 pitch, MS_U32 flags)
{
	GE_Result ret;
	GE_FmtCaps caps;
	MS_U16 u16fmt;
	MS_U8 miu = 0;
	MS_U16 reg_val = 0;

	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	// pix_width, pix_height: reserved for future use.
	src_fmt = (GE_BufFmt) (0xFF & src_fmt);
	ret = GE_GetFmtCaps(&pGECtxLocal->halLocalCtx, src_fmt, E_GE_BUF_SRC, &caps);
	if (ret != E_GE_OK) {
		G2D_ERROR("format fail\n");
		return ret;
	}
	if (addr != ((addr + (caps.u8BaseAlign - 1)) & ~(caps.u8BaseAlign - 1))) {
		G2D_ERROR("address fail\n");
		return E_GE_FAIL_ADDR;
	}
	if (pitch != ((pitch + (caps.u8PitchAlign - 1)) & ~(caps.u8PitchAlign - 1))) {
		G2D_ERROR("pitch fail\n");
		return E_GE_FAIL_PITCH;
	}

	u16fmt =
	    (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0) & ~GE_SRC_FMT_MASK) |
	    (GE_GetFmt(src_fmt) << GE_SRC_FMT_SHFT);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0, u16fmt);

	pGECtxLocal->PhySrcAddr = addr;

	{
		miu = _GFXAPI_MIU_ID(addr);
		addr =
		    GE_ConvertAPIAddr2HAL(&pGECtxLocal->halLocalCtx, miu,
					  _GFXAPI_PHYS_ADDR_IN_MIU(addr));
	}

	reg_val =
	   GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx,
		      REG_007C_GE0) & (~(1 << GE_SRC_BUFFER_MIU_H_SHFT));
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_007C_GE0, reg_val);

	HAL_GE_SetBufferAddr(&pGECtxLocal->halLocalCtx, addr, E_GE_BUF_SRC);

	// Set source pitch
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00C0_GE0, pitch);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Get PE destination buffer info
/// @param  pGECtx                     \b IN: GE context
/// @param  bufinfo                 \b IN: pointer to GE_BufInfo struct
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>E_GE_FMT_ARGB1555 is RGB555, E_GE_FMT_YUV422 can not be destination.</em></b>
/// <b>[URANUS] <em>E_GE_FMT_* should be 8-byte addr/pitch aligned to do stretch bitblt</em></b>
/// @sa @ref MDrv_GE_GetFmtCaps
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_GetBufferInfo(GE_Context *pGECtx, GE_BufInfo *bufinfo)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	bufinfo->srcfmt = pGECtx->pBufInfo.srcfmt;
	bufinfo->dstfmt = pGECtx->pBufInfo.dstfmt;

	bufinfo->srcpit = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00C0_GE0);
	bufinfo->dstpit = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00CC_GE0);

	bufinfo->dstaddr = pGECtxLocal->PhyDstAddr;
	bufinfo->srcaddr = pGECtxLocal->PhySrcAddr;

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Set GE destination buffer info
/// @param  pGECtx                     \b IN: GE context
/// @param  dst_fmt                 \b IN: source buffer format
/// @param  pix_width               \b IN: pixel width of destination buffer (RESERVED)
/// @param  pix_height              \b IN: pixel height of destination buffer (RESERVED)
/// @param  addr                    \b IN: destination buffer start address [23:0]
/// @param  pitch                   \b IN: destination buffer linear pitch in byte unit
/// @param  flags                   \b IN: option flag of dst buffer <b>(RESERVED)</b>
/// @return @ref GE_Result
/// @attention
/// <b>[URANUS] <em>E_GE_FMT_ARGB1555 is RGB555, E_GE_FMT_YUV422 can not be destination.</em></b>
/// <b>[URANUS] <em>E_GE_FMT_* should be 8-byte addr/pitch aligned to do stretch bitblt</em></b>
/// @sa @ref MDrv_GE_GetFmtCaps
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetDstBuffer(GE_Context *pGECtx, GE_BufFmt dst_fmt, MS_U16 pix_width,
			       MS_U16 pix_height, MS_PHY addr, MS_U16 pitch, MS_U32 flags)
{
	GE_Result ret;
	GE_FmtCaps caps;
	MS_U16 u16fmt;
	MS_U8 miu = 0;
	MS_U16 reg_val = 0;

	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	// pix_width, pix_height: reserved for future use.
	dst_fmt = (GE_BufFmt) (0xFF & dst_fmt);

	ret = GE_GetFmtCaps(&pGECtxLocal->halLocalCtx, dst_fmt, E_GE_BUF_DST, &caps);
	if (ret != E_GE_OK) {
		G2D_ERROR("format fail\n");
		return ret;
	}
	if (addr != ((addr + (caps.u8BaseAlign - 1)) & ~(caps.u8BaseAlign - 1))) {
		G2D_ERROR("address fail\n");
		return E_GE_FAIL_ADDR;
	}
	if (pitch != ((pitch + (caps.u8PitchAlign - 1)) & ~(caps.u8PitchAlign - 1))) {
		G2D_ERROR("pitch fail\n");
		return E_GE_FAIL_PITCH;
	}

	u16fmt =
	    (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0) & ~GE_DST_FMT_MASK) |
	    (GE_GetFmt(dst_fmt) << GE_DST_FMT_SHFT);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0, u16fmt);

	pGECtxLocal->PhyDstAddr = addr;

	{
		miu = _GFXAPI_MIU_ID(addr);
		addr =
		    GE_ConvertAPIAddr2HAL(&pGECtxLocal->halLocalCtx, miu,
					  _GFXAPI_PHYS_ADDR_IN_MIU(addr));
	}

	reg_val =
	    GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx,
		       REG_007C_GE0) & (~(1 << GE_DST_BUFFER_MIU_H_SHFT));
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_007C_GE0, reg_val);

	HAL_GE_SetBufferAddr(&pGECtxLocal->halLocalCtx, addr, E_GE_BUF_DST);

	// Set destination pitch
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00CC_GE0, pitch);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// GE Primitive Drawing - LINE
/// @param  pGECtx                     \b IN: GE context
/// @param  v0                      \b IN: pointer to start point of line
/// @param  v1                      \b IN: pointer to end point of line
/// @param  color                   \b IN: color of LINE
/// @param  color2                  \b IN: 2nd color of gradient
/// @param  flags                   \b IN: LINE drawing flags
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_DrawLine(GE_Context *pGECtx, GE_Point *v0, GE_Point *v1, MS_U32 color,
			   MS_U32 color2, MS_U32 flags, MS_U32 width)
{
	MS_U16 u16cmd = 0;
	MS_U16 u16cfg;
	MS_U16 u16style = 0;
	MS_U16 dw, dh, dminor, dmajor;
	MS_BOOL y_major = false;
	MS_U32 u32Start;
	MS_U32 u32End;
	MS_U32 i, u32OutWidth = 0;
	MS_BOOL bDstXInv = FALSE;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;


	GE_ASSERT((v0), E_GE_FAIL_PARAM, "%s: NULL pointer\n", __func__);
	GE_ASSERT((v1), E_GE_FAIL_PARAM, "%s: NULL pointer\n", __func__);

	if (flags & E_GE_FLAG_INWIDTH_IS_OUTWIDTH)
		u32OutWidth = width;
	else
		u32OutWidth = width + 1;

	// fast clip check
	if (GE_FastClip(pGECtx, v0->x, v0->y, v1->x, v1->y) && (width == 1)) {
		if ((flags & E_GE_FLAG_INWIDTH_IS_OUTWIDTH) == 0)
			return E_GE_NOT_DRAW;

	}

	dw = (v0->x >= v1->x) ? (v0->x - v1->x) : (v1->x - v0->x);
	dh = (v0->y >= v1->y) ? (v0->y - v1->y) : (v1->y - v0->y);

	if ((dh > dw) || (dw == 0))
		y_major = TRUE;

	GE_Restore_PaletteContext(pGECtxLocal);

	// clear dirty attributes -------------------------------------------
	// reset unused register
	GE_SetRotate(&pGECtxLocal->halLocalCtx, E_GE_ROTATE_0);
	u16cfg =
	    GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx,
		       REG_0004_GE0) & ~(GE_CFG_BLT_STRETCH | GE_CFG_BLT_ITALIC);
	u16style = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0004_GE0, u16cfg);
	if (flags & E_GE_FLAG_DRAW_LASTPOINT)
		u16style |= GE_LINE_LAST;
	else
		u16style &= ~GE_LINE_LAST;

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0, u16style);

	// clear dirty attributes --------------------------------------------
	// line delta
	if (y_major) {
		dmajor = dh;
		dminor = GE_Divide2Fixed(dw, dmajor, 1, 12);	// s1.12
		if (v0->x > v1->x)
			dminor = (MS_U16) (1 << (2 + 12)) - dminor;	// negative

		if (v0->y > v1->y)
			u16cmd |= GE_DST_DIR_X_INV;	// line always refer DIR_X

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0184_GE0,
			    GE_LINE_MAJOR_Y | (dminor << GE_LINE_DELTA_SHFT));
	} else { // x major
		dmajor = dw;
		dminor = GE_Divide2Fixed(dh, dmajor, 1, 12);	// s1.12
		if (v0->y > v1->y)
			dminor = (MS_U16) (1 << (2 + 12)) - dminor;	// negative

		if (v0->x > v1->x)
			u16cmd |= GE_DST_DIR_X_INV;

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0184_GE0,
			    GE_LINE_MAJOR_X | (dminor << GE_LINE_DELTA_SHFT));
	}

	// Line color
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01C0_GE0, (color & 0xFFFF));
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01C4_GE0, (color >> 16));

	if (flags & E_GE_FLAG_LINE_GRADIENT) {
		GE_ColorDelta delta;

		GE_CalcColorDelta(color, color2, dmajor, &delta);

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01C8_GE0, (delta.r & 0xFFFF));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01CC_GE0, (delta.r >> 16));

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01D8_GE0, (delta.g & 0xFFFF));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01DC_GE0, (delta.g >> 16));

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01E8_GE0, (delta.b & 0xFFFF));
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01EC_GE0, (delta.b >> 16));

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01F8_GE0, (delta.a & 0xFFFF));

		u16cmd |= GE_LINE_GRADIENT;
	}
	// Draw Line
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0, v0->x);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, v0->y);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0, v1->x);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, v1->y);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_018C_GE0, dmajor);

	u32Start = v0->x;
	u32End = v1->x;

	if (y_major) {
		if (v1->y <= v0->y) {
			if (v1->y >= 2) {
				GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx,
					REG_01AC_GE0, v1->y - 2);
			} else
				GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, v1->y);
		} else
			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, v1->y + 2);

		for (i = 0; i < u32OutWidth; i++) {
			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_018C_GE0, dmajor + 1);

			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0, u32Start);
			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0, u32End);
			bDstXInv = (u16cmd & (GE_DST_DIR_X_INV)) ? TRUE : FALSE;
			HAL_GE_AdjustDstWin(&pGECtxLocal->halLocalCtx, bDstXInv);

			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx,
				REG_0180_GE0, GE_PRIM_LINE | u16cmd);

			u32Start++;
			u32End++;
		}
	} else {

		if (v1->x <= v0->x) {
			if (v1->x >= 2) {
				GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx,
					REG_01A8_GE0, v1->x - 2);
			} else
				GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0, v1->x);

		} else
			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0, v1->x + 2);

		u32Start = v0->y;
		u32End = v1->y;
		for (i = 0; i < u32OutWidth; i++) {
			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_018C_GE0, dmajor + 1);

			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, u32Start);
			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, u32End);
			bDstXInv = (u16cmd & (GE_DST_DIR_X_INV)) ? TRUE : FALSE;
			HAL_GE_AdjustDstWin(&pGECtxLocal->halLocalCtx, bDstXInv);

			GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx,
				REG_0180_GE0, GE_PRIM_LINE | u16cmd);

			u32Start++;
			u32End++;
		}
	}

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
/// GE Primitive Drawing - RECT
/// @param  pGECtx                     \b IN: GE context
/// @param  rect                    \b IN: pointer to position of RECT
/// @param  color                   \b IN: color of RECT
/// @param  color2                  \b IN: 2nd color of gradient
/// @param  flags                   \b IN: RECT drawing flags
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_FillRect(GE_Context *pGECtx, GE_Rect *rect, MS_U32 color, MS_U32 color2,
			   MS_U32 flags)
{
	MS_U16 u16cmd = 0;
	MS_U16 u16cfg;
	MS_U16 u16fmt;
	MS_U16 u16fmt2;
	GE_ColorDelta delta;
	MS_U32 color_right_top = 0;
	MS_BOOL bDstXInv = FALSE;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_ASSERT((rect), E_GE_FAIL_PARAM, "%s: Null pointer\n", __func__);

	if ((rect->width == 0) || (rect->height == 0))
		return E_GE_FAIL_PARAM;

	if (GE_FastClip
	    (pGECtx, rect->x, rect->y, rect->x + rect->width - 1, rect->y + rect->height - 1)) {
		return E_GE_NOT_DRAW;
	}


	GE_Restore_PaletteContext(pGECtxLocal);

   // return HAL_CODA_GE_FillRect(&pGECtxLocal->halLocalCtx, rect, color, color2, flags);
	// clear dirty attributes --------------------------------------------
	// reset unused register
	GE_SetRotate(&pGECtxLocal->halLocalCtx, E_GE_ROTATE_0);
	u16cfg =
	    GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx,
		       REG_0004_GE0) & ~(GE_CFG_BLT_STRETCH | GE_CFG_BLT_ITALIC);

	if (*(&pGECtxLocal->halLocalCtx.pGeChipPro->bSupportSpiltMode))
		u16cfg |= GE_CFG_RW_SPLIT;

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0004_GE0, u16cfg);
	// clear dirty attributes ---------------------------------------------

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0, rect->x);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, rect->y);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0, rect->x + rect->width - 1);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, rect->y + rect->height - 1);

	if (*(&pGECtxLocal->halLocalCtx.pGeChipPro->bFourPixelModeStable)) {

		//Fill Rect must set source width and height for 4P mode check
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01B8_GE0, rect->width);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01BC_GE0, rect->height);
		//Fill Rect must set source color format same as dest color format for 4P mode check
		u16fmt = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0) & GE_DST_FMT_MASK;
		u16fmt2 = ((u16fmt >> GE_DST_FMT_SHFT) & GE_SRC_FMT_MASK);
		u16fmt |= u16fmt2;
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0, u16fmt);
	}

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01C0_GE0, color & 0xFFFF);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01C4_GE0, color >> 16);

	//[TODO] should I set REG_GE_PRIM_ADX and REG_GE_PRIM_ADY?

	if ((flags & (E_GE_FLAG_RECT_GRADIENT_X | E_GE_FLAG_RECT_GRADIENT_Y)) ==
	    (E_GE_FLAG_RECT_GRADIENT_X | E_GE_FLAG_RECT_GRADIENT_Y)) {
		MS_U8 a0, r0, g0, b0;
		MS_U8 a1, r1, g1, b1;
		MS_U8 a2, r2, g2, b2;

		// Get A,R,G,B
		//[TODO] special format
		b0 = (color) & 0xFF;
		g0 = (color >> 8) & 0xFF;
		r0 = (color >> 16) & 0xFF;
		a0 = (color >> 24);
		b2 = (color2) & 0xFF;
		g2 = (color2 >> 8) & 0xFF;
		r2 = (color2 >> 16) & 0xFF;
		a2 = (color2 >> 24);
		b1 = ((MS_S32) b2 -
		      b0) * (MS_S32) rect->width * rect->width / ((MS_S32) rect->width *
								  rect->width +
								  (MS_S32) rect->height *
								  rect->height) + (MS_S32) b0;
		g1 = ((MS_S32) g2 -
		      g0) * (MS_S32) rect->width * rect->width / ((MS_S32) rect->width *
								  rect->width +
								  (MS_S32) rect->height *
								  rect->height) + (MS_S32) g0;
		r1 = ((MS_S32) r2 -
		      r0) * (MS_S32) rect->width * rect->width / ((MS_S32) rect->width *
								  rect->width +
								  (MS_S32) rect->height *
								  rect->height) + (MS_S32) r0;
		a1 = ((MS_S32) a2 -
		      a0) * (MS_S32) rect->width * rect->width / ((MS_S32) rect->width *
								  rect->width +
								  (MS_S32) rect->height *
								  rect->height) + (MS_S32) a0;
		color_right_top =
		    ((MS_U32) a1) << 24 | ((MS_U32) r1) << 16 | ((MS_U32) g1) << 8 | b1;
	} else if (flags & E_GE_FLAG_RECT_GRADIENT_X)
		color_right_top = color2;
	else if (flags & E_GE_FLAG_RECT_GRADIENT_Y)
		color_right_top = color;

	if ((flags & E_GE_FLAG_RECT_GRADIENT_X) && (rect->width > 1)) {
		GE_CalcColorDelta(color, color_right_top, rect->width - 1, &delta);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01C8_GE0, delta.r & 0xFFFF);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01CC_GE0, delta.r >> 16);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01D8_GE0, delta.g & 0xFFFF);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01DC_GE0, delta.g >> 16);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01E8_GE0, delta.b & 0xFFFF);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01EC_GE0, delta.b >> 16);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01F8_GE0, delta.a);
		u16cmd |= GE_RECT_GRADIENT_H;
	}

	if ((flags & E_GE_FLAG_RECT_GRADIENT_Y) && (rect->height > 1)) {
		GE_CalcColorDelta(color_right_top, color2, rect->height - 1, &delta);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01D0_GE0, delta.r & 0xFFFF);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01D4_GE0, delta.r >> 16);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01E0_GE0, delta.g & 0xFFFF);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01E4_GE0, delta.g >> 16);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01F0_GE0, delta.b & 0xFFFF);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01F4_GE0, delta.b >> 16);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01FC_GE0, delta.a);
		u16cmd |= GE_RECT_GRADIENT_V;
	}

	HAL_GE_AdjustDstWin(&pGECtxLocal->halLocalCtx, bDstXInv);

	_GE_SEMAPHORE_ENTRY(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0180_GE0, GE_PRIM_RECT | u16cmd);
	_GE_SEMAPHORE_RETURN(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// GE Primitive Drawing - BLT
/// @param  pGECtx                     \b IN: GE context
/// @param  src                     \b IN: source RECT of bitblt
/// @param  dst                     \b IN: destination RECT of bitblt
/// @param  flags                   \b IN: RECT drawing flags
/// @param  scaleinfo                     \b IN: Customed scale info
/// @return @ref GE_Result
/// @note\n
/// FMT_I1,I2,I4,I8: should be 64-bit aligned in base and pitch to do stretch bitblt.\n
/// FMT_YUV422: the rectangle should be YUV422 block aligned. (4 bytes for 2 pixels in horizontal)
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_BitBltEX(GE_Context *pGECtx, GE_Rect *src, GE_DstBitBltType *dst, MS_U32 flags,
			   GE_ScaleInfo *scaleinfo)
{
	MS_U16 u16cmd = 0;
	MS_U16 u16cfg = 0;
	MS_U16 u16en = 0;
	MS_U16 temp;
	MS_BOOL bOverlap = FALSE;
	MS_BOOL bNonOnePixelMode = FALSE;
	GE_Point v0, v1, v2;
	MS_U8 u8Rot;
	PatchBitBltInfo patchBitBltInfo;
	MS_BOOL bDstXInv = FALSE;
	MS_U16 u16tmp, Srcfmt, Dstfmt;
	MS_U16 u16tmp_dy;

	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_ASSERT((src), E_GE_FAIL_PARAM, "%s: Null pointer\n", __func__);
	GE_ASSERT((dst), E_GE_FAIL_PARAM, "%s: Null pointer\n", __func__);

	if ((src->width == 0) || (src->height == 0) || (dst->dstblk.width == 0)
	    || (dst->dstblk.height == 0)) {
		return E_GE_FAIL_PARAM;
	}
	if (GE_FastClip
	    (pGECtx, dst->dstblk.x, dst->dstblk.y, dst->dstblk.x + dst->dstblk.width - 1,
	     dst->dstblk.y + dst->dstblk.height - 1)) {
		if (flags &
		    (E_GE_FLAG_BLT_ITALIC | E_GE_FLAG_BLT_ROTATE_90 | E_GE_FLAG_BLT_ROTATE_180 |
		     E_GE_FLAG_BLT_ROTATE_270)) {
			// SKIP fast clipping

			//[TODO] Do fast clipping precisely for rotation and italic
		} else {
			return E_GE_NOT_DRAW;
		}
	}
	//[TODO] check stretch bitblt base address and pitch alignment if we can prevent register
	// being READ in queue prolbme (ex. wait_idle or shadow_reg)

	// Check overlap
	if (flags & E_GE_FLAG_BLT_OVERLAP) {
		if (flags & (E_GE_FLAG_BLT_STRETCH |
			     E_GE_FLAG_BLT_ITALIC |
			     E_GE_FLAG_BLT_MIRROR_H |
			     E_GE_FLAG_BLT_MIRROR_V |
			     E_GE_FLAG_BLT_ROTATE_90 |
			     E_GE_FLAG_BLT_ROTATE_180 | E_GE_FLAG_BLT_ROTATE_270)) {
			return E_GE_FAIL_OVERLAP;
		}
		//[TODO] Cechk overlap precisely
		bOverlap = GE_RectOverlap(pGECtxLocal, src, dst);
	}
	// Check italic
	if (flags & E_GE_FLAG_BLT_ITALIC) {
		if (flags & (E_GE_FLAG_BLT_MIRROR_H |
			     E_GE_FLAG_BLT_MIRROR_V |
			     E_GE_FLAG_BLT_ROTATE_90 |
			     E_GE_FLAG_BLT_ROTATE_180 | E_GE_FLAG_BLT_ROTATE_270)) {
			return E_GE_FAIL_ITALIC;
		}
	}
	// clear dirty attributes -------------------------------------------
	GE_SetRotate(&pGECtxLocal->halLocalCtx, E_GE_ROTATE_0);
	u16cfg =
	    GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx,
		       REG_0004_GE0) & ~(GE_CFG_BLT_STRETCH | GE_CFG_BLT_ITALIC);
	// clear dirty attributes -------------------------------------------


	if (flags & E_GE_FLAG_BYPASS_STBCOEF) {
		if (scaleinfo == NULL)
			return E_GE_FAIL_PARAM;

		u16cfg |= GE_CFG_BLT_STRETCH;
		if (flags & E_GE_FLAG_STRETCH_NEAREST)
			u16cmd |= GE_STRETCH_NEAREST;

		u16cmd |= GE_STRETCH_CLAMP;

	} else if (flags & E_GE_FLAG_BLT_STRETCH) {

		u16cfg |= GE_CFG_BLT_STRETCH;
		if (flags & E_GE_FLAG_STRETCH_NEAREST)
			u16cmd |= GE_STRETCH_NEAREST;

		u16cmd |= GE_STRETCH_CLAMP;

	} else {
		if ((src->width != dst->dstblk.width) || (src->height != dst->dstblk.height))
			return E_GE_FAIL_STRETCH;

	}

	if (GE_SetBltScaleRatio(&pGECtxLocal->halLocalCtx, src, dst, (GE_Flag) flags, scaleinfo) ==
	    E_GE_FAIL_PARAM)
		return E_GE_FAIL_PARAM;

	u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0);
	Srcfmt = (u16tmp & GE_SRC_FMT_MASK) >> GE_SRC_FMT_SHFT;
	Dstfmt = (u16tmp & GE_DST_FMT_MASK) >> GE_DST_FMT_SHFT;
	if ((Srcfmt == GE_FMT_I8) && (Dstfmt != GE_FMT_I8))
		pGECtxLocal->bSrcPaletteOn = TRUE;
	else
		pGECtxLocal->bSrcPaletteOn = FALSE;

	GE_Restore_PaletteContext(pGECtxLocal);

#if (GE_USE_I8_FOR_YUV_BLENDING == 1)
	MS_U16 u16temp = 0;

	u16en = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0) & GE_EN_BLEND;
	if (Srcfmt == GE_FMT_YUV422 && Dstfmt == GE_FMT_YUV422 && (u16en != 0)) {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0,
			    (GE_FMT_I8) << GE_SRC_FMT_SHFT | (GE_FMT_I8) << GE_DST_FMT_SHFT);
		src->width *= 2;
		dst->dstblk.width *= 2;
		u16temp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_GE_CLIP_L);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_GE_CLIP_L, u16temp * 2);
		u16temp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_GE_CLIP_R);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_GE_CLIP_R, u16temp * 2);
	}
#endif

	v0.x = dst->dstblk.x;
	v0.y = dst->dstblk.y;
	v1.x = v0.x + dst->dstblk.width - 1;
	v1.y = v0.y + dst->dstblk.height - 1;
	v2.x = src->x;
	v2.y = src->y;


	if (bOverlap) {

		if (v2.x < v0.x) {
			// right to left
			temp = v0.x;
			v0.x = v1.x;
			v1.x = temp;
			v2.x += src->width - 1;
			u16cmd |= GE_SRC_DIR_X_INV | GE_DST_DIR_X_INV;
		}
		if (v2.y < v0.y) {
			// bottom up
			temp = v0.y;
			v0.y = v1.y;
			v1.y = temp;
			v2.y += src->height - 1;
			u16cmd |= GE_SRC_DIR_Y_INV | GE_DST_DIR_Y_INV;
		}
	}

	if (flags & E_GE_FLAG_BLT_MIRROR_H) {
		//v2.x += src->width -1;// show repect for history, comment it
		u16cmd |= GE_SRC_DIR_X_INV;
	}
	if (flags & E_GE_FLAG_BLT_MIRROR_V) {
		//v2.y += src->height -1;// show repect for history, comment it
		u16cmd |= GE_SRC_DIR_Y_INV;
	}

	if (flags & E_GE_FLAG_BLT_DST_MIRROR_H) {
		temp = v0.x;
		v0.x = v1.x;
		v1.x = temp;
		u16cmd |= GE_DST_DIR_X_INV;
	}
	if (flags & E_GE_FLAG_BLT_DST_MIRROR_V) {
		temp = v0.y;
		v0.y = v1.y;
		v1.y = temp;
		u16cmd |= GE_DST_DIR_Y_INV;
	}

	if (flags & E_GE_FLAG_BLT_ITALIC)
		u16cfg |= GE_CFG_BLT_ITALIC;

	if (flags & GE_FLAG_BLT_ROTATE_MASK) {	// deal with ALL rotation mode
		GE_SetRotate(&pGECtxLocal->halLocalCtx,
			     (GE_RotateAngle) ((flags & E_GE_FLAG_BLT_ROTATE_270) >>
					       GE_FLAG_BLT_ROTATE_SHFT));
	}

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0, v0.x);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, v0.y);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0, v1.x);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, v1.y);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01B0_GE0, v2.x);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01B4_GE0, v2.y);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01B8_GE0, src->width);

	temp = src->height;

	if (pGECtxLocal->halLocalCtx.bYScalingPatch) {
		if (temp > 5)
			temp -= 5;
	}

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01BC_GE0, temp);

	u8Rot = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0164_GE0) & REG_GE_ROT_MODE_MASK;

	// Set dst coordinate
	if (u8Rot == 0) {
		if (!bOverlap) {
			if (!(flags & E_GE_FLAG_BLT_DST_MIRROR_H)) {
				GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0, v0.x);
				GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0,
					    v0.x + dst->dstblk.width - 1);
			}
			if (!(flags & E_GE_FLAG_BLT_DST_MIRROR_V)) {
				GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, v0.y);
				GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0,
					    v0.y + dst->dstblk.height - 1);
			}
		}
	} else if (u8Rot == 1) {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0,
			    v0.x + dst->dstblk.height - 1);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, v0.y);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0,
			    v0.x + dst->dstblk.height + dst->dstblk.width - 2);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0,
			    v0.y + dst->dstblk.height - 1);
	} else if (u8Rot == 2) {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0,
			    v0.x + dst->dstblk.width - 1);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0,
			    v0.y + dst->dstblk.height - 1);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0,
			    v0.x + dst->dstblk.width + dst->dstblk.width - 2);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0,
			    v0.y + dst->dstblk.height + dst->dstblk.height - 2);
	} else if (u8Rot == 3) {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0, v0.x);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0,
			    v0.y + dst->dstblk.width - 1);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0,
			    v0.x + dst->dstblk.width - 1);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0,
			    v0.y + dst->dstblk.height + dst->dstblk.width - 2);
	}

	u16cfg |= GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0004_GE0) & GE_CFG_CMDQ_MASK;

	//enable split mode
	if (*(&pGECtxLocal->halLocalCtx.pGeChipPro->bSupportSpiltMode))
		u16cfg |= GE_CFG_RW_SPLIT;

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0004_GE0, u16cfg);


	// To check if 1p mode set to TRUE and Non-1p mode limitations
	u16en = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0);

	patchBitBltInfo.flags = flags;
	patchBitBltInfo.src.width = src->width;
	patchBitBltInfo.src.height = src->height;
	patchBitBltInfo.dst.dstblk.width = dst->dstblk.width;
	patchBitBltInfo.dst.dstblk.height = dst->dstblk.height;
	patchBitBltInfo.scaleinfo = scaleinfo;
	bNonOnePixelMode = GE_NonOnePixelModeCaps(&pGECtxLocal->halLocalCtx, &patchBitBltInfo);

#if (GE_PITCH_256_ALIGNED_UNDER_4P_MODE == 1)
	//[Curry/Kano] Only dst pitch 256 aligned -> 4P mode is avalible.
	if (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00C0_GE0) % (GE_WordUnit))
		bNonOnePixelMode = FALSE;	// Must excuted under 1P mode
	else if (GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00CC_GE0) % (GE_WordUnit))
		bNonOnePixelMode = FALSE;	// Must excuted under 1P mode

#endif

#if (defined(COLOR_CONVERT_PATCH) && (COLOR_CONVERT_PATCH == 1))
	u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0);
	Srcfmt = (u16tmp & GE_SRC_FMT_MASK) >> GE_SRC_FMT_SHFT;
	Dstfmt = (u16tmp & GE_DST_FMT_MASK) >> GE_DST_FMT_SHFT;

	if (Dstfmt == E_MS_FMT_YUV422 || (Srcfmt != Dstfmt &&
	!((Srcfmt == E_MS_FMT_ARGB1555) && (Dstfmt == E_MS_FMT_ARGB1555_DST))))
		bNonOnePixelMode = FALSE;

#endif

	if (bNonOnePixelMode && (u16en & GE_EN_ONE_PIXEL_MODE))
		GE_SetOnePixelMode(&pGECtxLocal->halLocalCtx, FALSE);
	else if ((!bNonOnePixelMode) && (!(u16en & GE_EN_ONE_PIXEL_MODE)))
		GE_SetOnePixelMode(&pGECtxLocal->halLocalCtx, TRUE);


	u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0190_GE0);
	u16tmp_dy = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0194_GE0);
	if (((flags & E_GE_FLAG_BLT_STRETCH) && ((u16tmp != 0x1000) || (u16tmp_dy != 0x1000)))
	    || (u8Rot != 0)) {
		HAL_GE_EnableCalcSrc_WidthHeight(&pGECtxLocal->halLocalCtx, FALSE);
	} else {
		HAL_GE_EnableCalcSrc_WidthHeight(&pGECtxLocal->halLocalCtx, TRUE);
	}
	bDstXInv = (u16cmd & (GE_DST_DIR_X_INV)) ? TRUE : FALSE;
	if (!u8Rot)
		HAL_GE_AdjustDstWin(&pGECtxLocal->halLocalCtx, bDstXInv);

	//if srcfmt == dstfmt, dont use Dither
	u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_00D0_GE0);
	Srcfmt = (u16tmp & GE_SRC_FMT_MASK) >> GE_SRC_FMT_SHFT;
	Dstfmt = (u16tmp & GE_DST_FMT_MASK) >> GE_DST_FMT_SHFT;
	if ((Srcfmt == Dstfmt)
	    || ((Srcfmt == E_MS_FMT_ARGB1555) && (Dstfmt == E_MS_FMT_ARGB1555_DST))
	    || ((Srcfmt == E_MS_FMT_ARGB1555_DST) && (Dstfmt == E_MS_FMT_ARGB1555))
	    ) {
		MDrv_GE_SetDither(pGECtx, FALSE);
	}
	//if Srcfmt = Dstfmt = YUV422 => disable UV_FILTER , else enable
	if (Srcfmt == E_MS_FMT_YUV422 && Dstfmt == E_MS_FMT_YUV422) {
		u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_007C_GE0);
		u16tmp = (u16tmp & ~GE_UV_FILTER);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_007C_GE0, u16tmp);
	} else {
		u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_007C_GE0);
		u16tmp = (u16tmp | GE_UV_FILTER);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_007C_GE0, u16tmp);
	}

#ifdef GE_STRETCH_PATCH
	u16cfg = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_GE_CFG);
	u16cfg |= GE_CFG_BLT_STRETCH;
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_GE_CFG, u16cfg);
#endif

	_GE_SEMAPHORE_ENTRY(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0180_GE0, GE_PRIM_BITBLT | u16cmd);
	_GE_SEMAPHORE_RETURN(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);


//Backup
	if (u16en & GE_EN_ONE_PIXEL_MODE)
		GE_SetOnePixelMode(&pGECtxLocal->halLocalCtx, TRUE);
	else if (!(u16en & GE_EN_ONE_PIXEL_MODE))
		GE_SetOnePixelMode(&pGECtxLocal->halLocalCtx, FALSE);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Wait GE engine idle
/// @param  pGECtx                     \b IN: GE context
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_WaitIdle(GE_Context *pGECtx)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_WaitIdle(&pGECtxLocal->halLocalCtx);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// GE DDI Initialization
/// @param  cfg                     \b IN: @ref GE_Config
/// @param  ppGECtx                     \b IN: GE context
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
MS_U32 DTBArray(struct device_node *np, const char *name, MS_U32 *ret_array, int ret_array_length)
{
	int ret, i;
	__u32 *u32TmpArray = NULL;

	u32TmpArray = kmalloc_array(ret_array_length,
		sizeof(__u32), GFP_KERNEL);
	ret = of_property_read_u32_array(np, name, u32TmpArray, ret_array_length);

	if (ret != 0x0) {
		kfree(u32TmpArray);
		return ret;
	}

	for (i = 0; i < ret_array_length; i++)
		*(ret_array+i) = u32TmpArray[i];

	kfree(u32TmpArray);
	return 0;
}

void Parsing_DTB(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	void __iomem *RIUBase1;
	struct device_node *np;
	const char *name;
	MS_U32 u32Tmp = 0, ret = 0;
	MS_U32 *pu32array;

	np = of_find_compatible_node(NULL, NULL, "mtk-g2d");
	if (np != NULL) {
		name = "ip-version";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0) {
			pr_err("[%s, %d]:read DTS table failed, name = %s \r\n",
				__func__, __LINE__, name);
		}

		pGEHalLocal->u32IpVersion = u32Tmp;
		g_halGECtxLocal.u32IpVersion = u32Tmp;

		name = "reg";
		pu32array = kmalloc(sizeof(MS_U32) * 4, GFP_KERNEL);
		ret = DTBArray(np, name, pu32array, 4);
		RIUBase1 = ioremap(pu32array[1], pu32array[3]);

		if (((pGEHalLocal->u32IpVersion & MAJOR_IPVER_MSK) >> MAJOR_IPVER_SHT)
				== GE_3M_SERIES_MAJOR)
			Set_RIU_Base((uint64_t)RIUBase1, GE0_2410_BASE);
		else
			Set_RIU_Base((uint64_t)RIUBase1, pu32array[1] - 0x1c000000);

		kfree(pu32array);
	}

}

GE_Result MDrv_GE_Init(void *pInstance, GE_Config *cfg, GE_Context **ppGECtx)
{
	GE_CTX_LOCAL *pGECtxLocal = NULL;
	GE_CTX_SHARED *pGEShared;
	MS_BOOL bNeedInitShared = FALSE;

#if (GFX_UTOPIA20)
	GFX_INSTANT_PRIVATE *psGFXInstPriTmp;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPriTmp);

	pGECtxLocal = &g_drvGECtxLocal;
	pGECtxLocal->ctxHeader.pInstance = pInstance;
#else
	pGECtxLocal = &g_drvGECtxLocal;
#endif

	pGEShared = &g_drvGECtxShared;
	bNeedInitShared = TRUE;
	memset(pGEShared, 0, sizeof(GE_CTX_SHARED));

#if (GE_USE_HW_SEM && defined(MSOS_TYPE_OPTEE))
	pGECtxLocal->u32GESEMID = SEM_HK51ID;
#else
#ifdef __AEON__
	pGECtxLocal->u32GESEMID = SEM_MWMipsID;
#else
	pGECtxLocal->u32GESEMID = SEM_HKAeonID;
#endif
#endif
	pGECtxLocal->u16GEPrevSEMID = pGECtxLocal->u32GESEMID;
	pGECtxLocal->bIsComp = cfg->bIsCompt;
	pGECtxLocal->bSrcPaletteOn = FALSE;
	pGECtxLocal->bDstPaletteOn = FALSE;

	// need add lock for protect SHM.

	pGECtxLocal->u32CTXInitMagic = 0x55aabeef;

	pGECtxLocal->pSharedCtx = pGEShared;
	GE_Init_HAL_Context(&pGECtxLocal->halLocalCtx, &pGEShared->halSharedCtx, bNeedInitShared);
	Parsing_DTB(&pGECtxLocal->halLocalCtx);
	// save Init parameter
	pGEShared->bInit = TRUE;
	pGEShared->u32VCmdQSize = cfg->u32VCmdQSize;
	pGEShared->PhyVCmdQAddr = cfg->PhyVCmdQAddr;
	pGEShared->bIsHK = cfg->bIsHK;
	pGEShared->bIsCompt = cfg->bIsCompt;

	pGECtxLocal->halLocalCtx.bIsComp = pGECtxLocal->bIsComp;

	pGECtxLocal->u32GEClientId = ++pGEShared->u32GEClientAllocator;
	if (pGECtxLocal->u32GEClientId == 0)
		pGECtxLocal->u32GEClientId = ++pGEShared->u32GEClientAllocator;

	pGECtxLocal->ctxHeader.u32GE_DRV_Version = GE_API_MUTEX;
	pGECtxLocal->ctxHeader.bGEMode4MultiProcessAccess = GE_SWTABLE;
	pGECtxLocal->ctxHeader.s32CurrentProcess = (MS_S32) getpid();
	pGECtxLocal->ctxHeader.s32CurrentThread = 0;

	if (pGECtxLocal->ctxHeader.bGEMode4MultiProcessAccess == FALSE)
		pGECtxLocal->pSharedCtx->halSharedCtx.bGE_DirectToReg = TRUE;
	else
		pGECtxLocal->pSharedCtx->halSharedCtx.bGE_DirectToReg = FALSE;
	*ppGECtx = (GE_Context *) pGECtxLocal;

	pGECtxLocal->s32GE_Recursive_Lock_Cnt = 0;
	pGECtxLocal->s32GELock = -1;
	MsOS_CreateMutex();


	GE_Get_Resource(*ppGECtx, TRUE);
	// Enable Command Queue
	(*ppGECtx)->pBufInfo.srcfmt = ((GE_BufFmt) E_GE_FMT_ARGB1555);
	(*ppGECtx)->pBufInfo.dstfmt = ((GE_BufFmt) E_GE_FMT_ARGB1555);

	GE_Init(&pGECtxLocal->halLocalCtx, cfg);

	//Init GE Palette from HW:
	GE_InitCtxHalPalette(&pGECtxLocal->halLocalCtx);

	// Default setting : To avoid GE issues commands too frequently when VC in enabled
	MDrv_GE_SetVCmd_W_Thread(*ppGECtx, 0x4);
	MDrv_GE_SetVCmd_R_Thread(*ppGECtx, 0x4);

	pGECtxLocal->pSharedCtx->bNotFirstInit = TRUE;

	GE_Free_Resource(*ppGECtx, TRUE);

	return E_GE_OK;
}

GE_Result MDrv_GE_Chip_Proprity_Init(GE_Context *pGECtx, GE_CHIP_PROPERTY **ppGeChipPro)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_Chip_Proprity_Init(&pGECtxLocal->halLocalCtx);
	*ppGeChipPro = pGECtxLocal->halLocalCtx.pGeChipPro;

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Wait GE TagID back
/// @param pGECtx                      \b IN:  Pointer to GE context.
/// @param  tagID                     \b IN: tag id number for waiting
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_WaitTAGID(GE_Context *pGECtx, MS_U16 tagID)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_WaitTAGID(&pGECtxLocal->halLocalCtx, tagID);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Set next TagID Auto to HW
/// Get the Next Tag ID auto and set it to HW.
/// @param  pGECtx                     \b IN: GE context
/// @param  tagID                      \b OUT: The Tag ID which has been sent to hw
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetNextTAGID(GE_Context *pGECtx, MS_U16 *tagID)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;
	MS_U8 i = 0;

	*tagID = GE_GetNextTAGID(&pGECtxLocal->halLocalCtx, TRUE);

	//GE hw will kick off cmd in pairs, so send tagID twice to force kick off
	for (i = 0; i < (GE_WordUnit >> 2); i++)
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_00C8_GE0, *tagID);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
///Enable GE Virtual Command Queue
/// @param  pGECtx                     \b IN: GE context
/// @param  blEnable         \b IN: true: Enable, false: Disable
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_EnableVCmdQueue(GE_Context *pGECtx, MS_U16 blEnable)
{
	MS_U16 u16tmp;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_WaitIdle(&pGECtxLocal->halLocalCtx);

	u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0004_GE0);

	if (blEnable) {
		u16tmp |= GE_CFG_VCMDQ;
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0004_GE0, u16tmp);
	} else {
		u16tmp &= ~GE_CFG_VCMDQ;
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0004_GE0, u16tmp);
	}


	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
///Set GE Virtual Command Queue spec
/// @param  pGECtx                     \b IN: GE context
/// @param  u32Addr         \b IN: Start address of VCMQ buffer.
/// @param  enBufSize         \b IN:  Size of VCMQ buffer.
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetVCmdBuffer(GE_Context *pGECtx, MS_PHY PhyAddr, GFX_VcmqBufSize enBufSize)
{
	GE_Result ret;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_WaitIdle(&pGECtxLocal->halLocalCtx);

	ret = GE_SetVCmdBuffer(&pGECtxLocal->halLocalCtx, PhyAddr, enBufSize);

	return ret;
}


//-------------------------------------------------------------------------------------------------
///Set GE Virtual Command Write threshold
/// @param  pGECtx                     \b IN: GE context
/// @param  u8W_Threshold         \b IN: Threshold for VCMQ write.
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetVCmd_W_Thread(GE_Context *pGECtx, MS_U8 u8W_Threshold)
{
	MS_U16 u16tmp;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_WaitIdle(&pGECtxLocal->halLocalCtx);

	u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_000C_GE0);

	u16tmp &= ~(GE_TH_CMDQ_MASK);
	u16tmp |= u8W_Threshold << 4;

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_000C_GE0, u16tmp);

	return E_GE_OK;
}


//-------------------------------------------------------------------------------------------------
///Set GE Virtual Command Read threshold
/// @param  pGECtx                     \b IN: GE context
/// @param  u8R_Threshold         \b IN: Threshold for VCMQ read.
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result MDrv_GE_SetVCmd_R_Thread(GE_Context *pGECtx, MS_U8 u8R_Threshold)
{
	MS_U16 u16tmp;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_WaitIdle(&pGECtxLocal->halLocalCtx);

	u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_000C_GE0);

	u16tmp &= ~(GE_TH_CMDQ2_MASK);
	u16tmp |= u8R_Threshold << 8;

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_000C_GE0, u16tmp);

	return E_GE_OK;
}

////////////////////////////////////////////////////////////////////////////////
/// Configure GE SCK edge refine rtpe.
/// @param  pGECtx                     \b IN: GE context
/// @param  TYPE         \b IN: Type for edge refine.
/// @param  CLR         \b IN: 32-bit custom color if type is E_GE_REPLACE_KEY_2_CUS
/// @return @ref GE_Result
///////////////////////////////////////////////////////////////////////////////
GE_Result MDrv_GE_SetStrBltSckType(GE_Context *pGECtx, GFX_StretchCKType TYPE, MS_U32 *CLR)
{
	MS_U16 u16tmp;
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	GE_WaitIdle(&pGECtxLocal->halLocalCtx);

	u16tmp = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0164_GE0);

	u16tmp &= ~(GE_BLT_SCK_MODE_MASK);

	switch (TYPE) {
	case GFX_DONOTHING:
		u16tmp |= GE_BLT_SCK_BILINEAR;
		break;

	case GFX_NEAREST:
		u16tmp |= GE_BLT_SCK_NEAREST;
		break;

	case GFX_REPLACE_KEY_2_CUS:
		u16tmp |= GE_BLT_SCK_CONST;
		break;
	default:
		G2D_ERROR("ERROR Stretch Type\n");
		break;
	}

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0170_GE0, (*CLR) & 0xffff);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0174_GE0, (*CLR) >> 16);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0164_GE0, u16tmp);

	return E_GE_OK;
}

////////////////////////////////////////////////////////////////////////////////
/// Draw Oval. Oval is not directly supported by HW. Software implemented by DrawLine.
/// @param  pGECtx                     \b IN: GE context
/// @param  pOval         \b IN: oval info
/// @return @ref GE_Result
///////////////////////////////////////////////////////////////////////////////
GE_Result MDrv_GE_DrawOval(GE_Context *pGECtx, GE_OVAL_FILL_INFO *pOval)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	MS_S32 x, y, c_x, c_y;
	MS_S32 Xchange, Ychange;
	MS_S32 EllipseError;
	MS_S32 TwoASquare, TwoBSquare;
	MS_S32 StoppingX, StoppingY;
	MS_U32 Xradius, Yradius;

	MS_U16 u16Color0, u16Color1;
	MS_BOOL bDstXInv = FALSE;

	GE_Restore_PaletteContext(pGECtxLocal);

	Xradius = (pOval->dstBlock.width >> 1) - pOval->u32LineWidth;
	Yradius = (pOval->dstBlock.height >> 1) - pOval->u32LineWidth;

	/* center of ellipse */
	c_x = pOval->dstBlock.x + Xradius + pOval->u32LineWidth;
	c_y = pOval->dstBlock.y + Yradius + pOval->u32LineWidth;

	TwoASquare = (Xradius * Xradius) << 1;
	TwoBSquare = (Yradius * Yradius) << 1;

	/*1st set of points */
	x = Xradius - 1;	/*radius zero == draw nothing */
	y = 0;

	StoppingX = TwoBSquare * Xradius;
	StoppingY = 0;

	Xchange = (TwoBSquare >> 1) - StoppingX;
	Ychange = TwoASquare >> 1;

	EllipseError = 0;

//(SUPPORT_LINE_LAST_PATCH == 1 )
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0, GE_LINEPAT_RST | 0x3f);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0188_GE0, 0x3f);


	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0184_GE0, 0);

	u16Color0 = (((MS_U16) (pOval->color.g)) << 8) + (MS_U16) pOval->color.b;
	u16Color1 = (((MS_U16) (pOval->color.a)) << 8) + (MS_U16) pOval->color.r;

	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0170_GE0, u16Color0);
	GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0174_GE0, u16Color1);

	/*Plot 2 ellipse scan lines for iteration */
	while (StoppingX > StoppingY) {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0, c_x - x);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0, c_x + x);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, c_y + y);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, c_y + y + 1);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_018C_GE0, (x << 1) + 1);
		HAL_GE_AdjustDstWin(&pGECtxLocal->halLocalCtx, bDstXInv);

		_GE_SEMAPHORE_ENTRY(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0180_GE0, GE_PRIM_LINE);
		_GE_SEMAPHORE_RETURN(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, c_y - y);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, c_y - y + 1);

		HAL_GE_AdjustDstWin(&pGECtxLocal->halLocalCtx, bDstXInv);

		_GE_SEMAPHORE_ENTRY(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0180_GE0, GE_PRIM_LINE);
		_GE_SEMAPHORE_RETURN(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);

		++y;
		StoppingY += TwoASquare;
		EllipseError += Ychange;
		Ychange += TwoASquare;
		if (((EllipseError << 1) + Xchange) > 0) {
			--x;
			StoppingX -= TwoBSquare;
			EllipseError += Xchange;
			Xchange += TwoBSquare;
		}
	}

	/*2nd set of points */
	x = 0;
	y = Yradius - 1;	/*radius zero == draw nothing */
	StoppingX = 0;
	StoppingY = TwoASquare * Yradius;
	Xchange = TwoBSquare >> 1;
	Ychange = (TwoASquare >> 1) - StoppingY;
	EllipseError = 0;

	/*Plot 2 ellipse scan lines for iteration */
	while (StoppingX < StoppingY) {
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A0_GE0, c_x - x);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A8_GE0, c_x + x);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, c_y + y);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, c_y + y + 1);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_018C_GE0, (x << 1) + 1);
		HAL_GE_AdjustDstWin(&pGECtxLocal->halLocalCtx, bDstXInv);

		_GE_SEMAPHORE_ENTRY(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0180_GE0, GE_PRIM_LINE);
		_GE_SEMAPHORE_RETURN(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);

		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01A4_GE0, c_y - y);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_01AC_GE0, c_y - y + 1);
		HAL_GE_AdjustDstWin(&pGECtxLocal->halLocalCtx, bDstXInv);

		_GE_SEMAPHORE_ENTRY(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0180_GE0, GE_PRIM_LINE);
		_GE_SEMAPHORE_RETURN(pGECtx, E_GE_POOL_ID_INTERNAL_REGISTER);

		++x;
		StoppingX += TwoBSquare;
		EllipseError += Xchange;
		Xchange += TwoBSquare;
		if (((EllipseError << 1) + Ychange) > 0) {
			--y;
			StoppingY -= TwoASquare;
			EllipseError += Ychange;
			Ychange += TwoASquare;
		}
	}

	return E_GE_OK;
}

////////////////////////////////////////////////////////////////////////////////
/// Power State Setting
/// @param  pGECtx                \b IN: GE context
/// @param  u16PowerState         \b IN: Power State
/// @return @ref GE_Result
///////////////////////////////////////////////////////////////////////////////
MS_U32 MDrv_GESTR_WaitIdle(void)
{
	MS_U32 waitcount = 0;

	// Wait level-2 command queue flush
	while (mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ1_STATUS) !=
		GE_STAT_CMDQ2_MAX) {
		if (waitcount >= 1000) {
			G2D_INFO("Wait Idle: %tx\n",
			 (ptrdiff_t) mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ2_STATUS));
			waitcount = 0;
			dump_g2d_status();
			break;
		}
		waitcount++;
		MsOS_DelayTask(1);
	}

	waitcount = 0;
	// Wait GE idle
	while (mtk_read2bytemask(REG_001C_GE0, REG_GE_BUSY)) {
		if (waitcount >= 1000) {
			G2D_INFO("Wait Busy: %tx\n",
			 (ptrdiff_t) mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ2_STATUS));
			waitcount = 0;
			dump_g2d_status();
		}
		waitcount++;
		MsOS_DelayTask(1);
	}

	return 0;
}

MS_U32 MDrv_GESTR_SetPalette(MS_U32 *GEPalette)
{
	int i;

	for (i = 0; i < GE_PALETTE_NUM; i++) {
		mtk_write2byte(REG_00B8_GE0,
				(ByteSwap16(GEPalette[i]) & 0xFFFF));
		mtk_write2byte(REG_00B8_GE0,
				(ByteSwap16(GEPalette[i] >> 16)));
		mtk_write2byte(REG_00BC_GE0,
				((i & GE_CLUT_CTRL_IDX_MASK) | GE_CLUT_CTRL_WR));
	}
	return 0;
}

void Parsing_DTB_STR(void)
{
	void __iomem *RIUBase1;
	struct device_node *np;
	const char *name;
	MS_U32 u32Tmp = 0, ret = 0;
	MS_U32 *pu32array;

	np = of_find_compatible_node(NULL, NULL, "mtk-g2d");
	if (np != NULL) {
		name = "ip-version";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0) {
			pr_err("[%s, %d]:read DTS table failed, name = %s \r\n",
				__func__, __LINE__, name);
		}

		name = "reg";
		pu32array = kmalloc(sizeof(MS_U32) * 4, GFP_KERNEL);
		ret = DTBArray(np, name, pu32array, 4);
		RIUBase1 = ioremap(pu32array[1], pu32array[3]);

		Set_RIU_Base((uint64_t)RIUBase1, pu32array[1] - 0x1c000000);

		kfree(pu32array);
	}

}

static GE_STR_SAVE_AREA GFX_STRPrivate;
GE_Result MDrv_GE_STR_SetPowerState(EN_POWER_MODE u16PowerState, void *pModule)
{
#if defined(MSOS_TYPE_LINUX_KERNEL)
	MS_U8 i = 0UL;

	Parsing_DTB_STR();
	switch (u16PowerState) {
	case E_POWER_SUSPEND:
	{
		//wait idle
		MDrv_GESTR_WaitIdle();
		//store reg
		for (i = 0; i < GE_TABLE_REGNUM; i++) {
			GFX_STRPrivate.GE0_Reg[i] = mtk_read2byte((i*4) + GE0_2410_BASE);
		}
		//disable power
		//GFX_STRPrivate.GECLK_Reg = STR_CLK_REG(CHIP_GE_CLK);
#if GE_USE_HW_SEM
		MDrv_SEM_Get_ResourceID(GESEMID, &(pGFX_STRPrivate->HWSemaphoreID));
		if (pGFX_STRPrivate->HWSemaphoreID) {
			if (MDrv_SEM_Free_Resource(GESEMID, pGFX_STRPrivate->HWSemaphoreID)
				== FALSE) {
				G2D_ERROR("Free resource fail\n");
				return E_GE_FAIL;
			}
		}
#endif

	}
		break;
	case E_POWER_RESUME:
	{
#if GE_USE_HW_SEM
		if (pGFX_STRPrivate->HWSemaphoreID)
			MDrv_SEM_Get_Resource(GESEMID, pGFX_STRPrivate->HWSemaphoreID);

#endif
		//Restore registers according to the value recorded in pGFX_STRPrivate
		HAL_GE_STR_RestoreReg(&g_halGECtxLocal, &GFX_STRPrivate);
		//resume palette
		MDrv_GESTR_SetPalette(GFX_STRPrivate.GEPalette);

	}
	break;
	default:
	break;
	}
#endif
	return E_GE_OK;
}

GE_Result MDrv_GE_RestoreRegInfo(GE_Context *pGECtx, GE_RESTORE_REG_ACTION eRESTORE_ACTION,
				 MS_U16 *value)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	switch (eRESTORE_ACTION) {
	case E_GE_SAVE_REG_GE_EN:
		*value = GE_CODA_ReadReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0);
		break;
	case E_GE_RESTORE_REG_GE_EN:
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0, *value);
		break;
	case E_GE_DISABLE_REG_EN:
		GE_CODA_WriteReg(&pGECtxLocal->halLocalCtx, REG_0000_GE0,
			    (*value &
			     ~(GE_EN_ASCK | GE_EN_DSCK | GE_EN_SCK | GE_EN_DCK | GE_EN_BLEND |
			       GE_EN_DFB_BLD | GE_EN_DITHER)));
		break;
	default:
		break;
	}

	return E_GE_OK;
}

GE_Result MDrv_GE_GetCRC(GE_Context *pGECtx, MS_U32 *pu32CRCvalue)
{
	GE_CTX_LOCAL *pGECtxLocal = (GE_CTX_LOCAL *) pGECtx;

	HAL_GE_GetCRC(&pGECtxLocal->halLocalCtx, pu32CRCvalue);
	return E_GE_OK;
}

#define H_SHT               (16)
#define MSB_SHT             (31)
#define CODA_SHT            (4)
#define PRI_ROW_CNT         (8)
#define PRINT_SIZE          (255)
void dump_g2d_status(void)
{
	__u32 i, j;
	char reg[PRINT_SIZE];
	int ssize = 0;
	uint16_t u16Size = PRINT_SIZE;
	__u32 temp = 0;
	__u32 temp2 = 0;
	__u64 addr_L = 0;
	__u64 addr_H = 0;
	__u64 addr_MSB = 0;

	G2D_INFO("G2D Info\n");
	G2D_INFO("==========================\n");
	addr_L = mtk_read2bytemask(REG_0080_GE0, REG_GE_SB_BASE_0);
	addr_H = mtk_read2bytemask(REG_0084_GE0, REG_GE_SB_BASE_1);
	addr_MSB = mtk_read2bytemask(REG_008C_GE0, REG_GE_SB_BASE_MSB);
	G2D_INFO("src addr = 0x%llx\n", addr_L|(addr_H << H_SHT)|(addr_MSB << MSB_SHT));
	temp = mtk_read2bytemask(REG_00C0_GE0, REG_GE_SB_PIT);
	G2D_INFO("src pitch = %u\n", temp);
	temp = mtk_read2bytemask(REG_00D0_GE0, REG_GE_SB_FM);
	G2D_INFO("src fmt = %u\n", temp);

	addr_L = mtk_read2bytemask(REG_0098_GE0, REG_GE_DB_BASE_0);
	addr_H = mtk_read2bytemask(REG_009C_GE0, REG_GE_DB_BASE_1);
	addr_MSB = mtk_read2bytemask(REG_008C_GE0, REG_GE_DB_BASE_MSB);
	G2D_INFO("dst addr = 0x%llx\n", addr_L|(addr_H << H_SHT)|(addr_MSB << MSB_SHT));
	temp = mtk_read2bytemask(REG_00CC_GE0, REG_GE_DB_PIT);
	G2D_INFO("dst pitch = %u\n", temp);
	temp = mtk_read2bytemask(REG_00D0_GE0, REG_GE_DB_FM);
	G2D_INFO("dst fmt = %u\n", temp);
	temp = mtk_read2bytemask(REG_01A0_GE0, REG_GE_PRI_V0_X);
	temp2 = mtk_read2bytemask(REG_01A8_GE0, REG_GE_PRI_V1_X);
	G2D_INFO("x0 = %u , x1 = %u\n", temp, temp2);
	temp = mtk_read2bytemask(REG_01A4_GE0, REG_GE_PRI_V0_Y);
	temp2 = mtk_read2bytemask(REG_01AC_GE0, REG_GE_PRI_V1_Y);
	G2D_INFO("y0 = %u , y1 = %u\n", temp, temp2);

	temp = mtk_read2bytemask(REG_0004_GE0, REG_EN_GE_VCMQ);
	G2D_INFO("virtual cmd queue enable = %u\n", temp);
	addr_L = mtk_read2bytemask(REG_00A0_GE0, REG_GE_VCMQ_BASE_0);
	addr_H = mtk_read2bytemask(REG_00A4_GE0, REG_GE_VCMQ_BASE_1);
	addr_MSB = mtk_read2bytemask(REG_008C_GE0, REG_GE_VCMQ_BASE_MSB);
	G2D_INFO("vq addr = 0x%llx\n", addr_L|(addr_H << H_SHT)|(addr_MSB << MSB_SHT));

	temp = mtk_read2bytemask(REG_0164_GE0, REG_GE_ROT);
	G2D_INFO("rotate = %u\n", temp);
	temp = mtk_read2bytemask(REG_0000_GE0, REG_EN_GE_ABL);
	G2D_INFO("abl enable = %u\n", temp);
	G2D_INFO("==========================\n");


	G2D_INFO("Dump GE register:\n");
	for (i = 0; i < GE_TABLE_REGNUM; i += PRI_ROW_CNT) {
		for (j = 0 ; j < PRI_ROW_CNT ; j++) {
			temp = mtk_read2byte(((i+j)*CODA_SHT)+GE0_2410_BASE);
			ssize +=
				scnprintf(reg + ssize, u16Size - ssize, "%04x ", temp);
		}

		G2D_INFO("\n");
		G2D_INFO("h%02x %s", i, reg);
		memset(reg, 0, PRINT_SIZE);
		ssize = 0;
		u16Size = PRINT_SIZE;
	}
	G2D_INFO("=========================\n");
}

#endif				//_DRV_GE_C_
