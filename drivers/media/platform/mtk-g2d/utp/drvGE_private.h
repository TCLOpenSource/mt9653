/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRV_PRI_GE_H_
#define _DRV_PRI_GE_H_

#include "sti_msos.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	GE_BlendOp eBldCoef;
	GFX_AlphaSrcFrom eABLSrc;
	MS_U32 u32ABLConstCoef;
} GE_ABLINFO;

typedef struct DLL_PACKED {
	GE_CTX_HAL_SHARED halSharedCtx;
	MS_U32 u32GEClientAllocator;
	MS_U32 u32LstGEClientId;	//for indicating context switch
	MS_U32 u32LstGEPaletteClientId;
	MS_BOOL bNotFirstInit;
	MS_U32 u32HWSemaphoreCnt;
	MS_U32 u32VCmdQSize;	/// MIN:4K, MAX:512K, <MIN:Disable
	MS_PHY PhyVCmdQAddr;	//  8-byte aligned
	MS_BOOL bIsHK;	/// Running as HK or Co-processor
	MS_BOOL bIsCompt;
	MS_BOOL bInit;

} GE_CTX_SHARED;

typedef struct {
	GE_Context ctxHeader;
	GE_CTX_SHARED *pSharedCtx;	//pointer to shared context paramemetrs
	GE_CTX_HAL_LOCAL halLocalCtx;
	MS_U16 u16ClipL;
	MS_U16 u16ClipR;
	MS_U16 u16ClipT;
	MS_U16 u16ClipB;
#if GE_LOCK_SUPPORT
	MS_S32 s32GEMutex;
	MS_S32 s32GELock;
#endif
	MS_S32 s32GE_Recursive_Lock_Cnt;	//recursive lock number when bGELock is off
	MS_U32 u32GEClientId;
	MS_U32 u32GESEMID;
	MS_U16 u16GEPrevSEMID;	//backup previous Sem ID for palette restore.
	MS_BOOL bIsComp;
	MS_U32 u32CTXInitMagic;
	MS_BOOL bSrcPaletteOn;
	MS_BOOL bDstPaletteOn;
	MS_PHY PhySrcAddr;
	MS_PHY PhyDstAddr;
	MS_U16 u16VcmdqAddr_L;
	MS_U16 u16VcmdqAddr_H;
	MS_U16 u16VcmdqAddr_MSB;
	MS_U16 u16VcmdqMiuMsb;
	MS_U16 u16VcmdqSize;
	MS_U16 u16RegCfg;
} GE_CTX_LOCAL;

typedef struct {
	MS_U32 u32dbglvl;
	MS_U8 _angle;
	MS_BOOL _bNearest;
	MS_BOOL _bPatchMode;
	MS_BOOL _bMirrorH;
	MS_BOOL _bMirrorV;
	MS_BOOL _bDstMirrorH;
	MS_BOOL _bDstMirrorV;
	MS_BOOL _bItalic;
	MS_BOOL _line_enable;
	MS_U8 _line_pattern;
	MS_U8 _line_factor;
	MS_BOOL bDither;

#ifdef DBGLOG
	//debug use only
	MS_BOOL _bOutFileLog;
	MS_U16 *_pu16OutLogAddr;
	MS_U16 _u16LogCount;
#endif
	GE_Context *g_pGEContext;
	GE_CHIP_PROPERTY *pGeChipProperty;
	GE_ABLINFO pABLInfo;
	MS_U32 u32LockStatus;
	MS_BOOL _bInit;
	MS_U32 u32geRgbColor;
} GFX_API_LOCAL;


typedef struct _GFX_Resource_PRIVATE {

} GFX_Resource_PRIVATE;

typedef struct _GFX_INSTANT_PRIVATE {
	/*Resource */
	GFX_Resource_PRIVATE *pResource;

	GFX_API_LOCAL GFXPrivate_g_apiGFXLocal;
	GE_CTX_LOCAL GFXPrivate_g_drvGECtxLocal;

} GFX_INSTANT_PRIVATE;

#ifdef __cplusplus
}
#endif

#endif
