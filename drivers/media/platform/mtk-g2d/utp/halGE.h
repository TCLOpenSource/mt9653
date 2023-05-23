/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _HAL_GE_H_
#define _HAL_GE_H_


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define bNeedSetActiveCtrlMiu1      TRUE
#define GE_THRESHOLD_SETTING        0xDUL
#define GE_WordUnit                 32UL
#define GE_MAX_MIU                  2UL
#define GE_TABLE_REGNUM             0x80UL

#define GE_FMT_I1                 0x0UL
#define GE_FMT_I2                 0x1UL
#define GE_FMT_I4                 0x2UL
#define GE_FMT_I8                 0x4UL
#define GE_FMT_FaBaFgBg2266       0x6UL
#define GE_FMT_1ABFgBg12355       0x7UL
#define GE_FMT_RGB565             0x8UL
#define GE_FMT_ARGB1555           0x9UL
#define GE_FMT_ARGB4444           0xAUL
#define GE_FMT_ARGB1555_DST       0xCUL
#define GE_FMT_YUV422             0xEUL
#define GE_FMT_ARGB8888           0xFUL
#define GE_FMT_RGBA5551           0x10UL
#define GE_FMT_ABGR1555           0x11UL
#define GE_FMT_BGRA5551           0x12UL
#define GE_FMT_RGBA4444           0x13UL
#define GE_FMT_ABGR4444           0x14UL
#define GE_FMT_BGRA4444           0x15UL
#define GE_FMT_BGR565             0x16UL
#define GE_FMT_RGBA8888           0x1DUL
#define GE_FMT_ABGR8888           0x1EUL
#define GE_FMT_BGRA8888           0x1FUL
#define GE_FMT_GENERIC            0xFFFFUL

#define GE_VQ_4K                  0x0UL
#define GE_VQ_8K                  0x0UL
#define GE_VQ_16K                 0x0UL
#define GE_VQ_32K                 0x1UL
#define GE_VQ_64K                 0x2UL
#define GE_VQ_128K                0x3UL
#define GE_VQ_256K                0x4UL
#define GE_VQ_512K                0x5UL
#define GE_VQ_1024K               0x6UL
#define GE_VQ_2048K               0x7UL

#ifndef UNUSED
#define UNUSED(var)             (void)(var)
#endif

#define __USE_GE_POLLING_MODE 0
#define __USE_GE_INT_MODE 1  // Kernel must enable CONFIG_MP_PLATFORM_UTOPIA2_INTERRUPT

#ifdef CONFIG_GFX_TAG_INTERRUPT

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
#define __GE_WAIT_TAG_MODE __USE_GE_INT_MODE
#else
#define __GE_WAIT_TAG_MODE __USE_GE_POLLING_MODE
#endif

#else

#define __GE_WAIT_TAG_MODE __USE_GE_POLLING_MODE

#endif
//feature
#define GE_USE_HW_SEM (0)

//Patch
#define GE_PITCH_256_ALIGNED_UNDER_4P_MODE (1)
#define COLOR_CONVERT_PATCH (1)
#define GE_USE_I8_FOR_YUV_BLENDING (0)
#define GE_SCALING_MULITPLIER   (0x1000)


#include "sti_msos.h"

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
typedef struct _GE_ColorDelta {
	MS_U32                          r; // s7.12
	MS_U32                          g;
	MS_U32                          b;
	MS_U16                          a; // s4.11
} GE_ColorDelta;

/*the following is for parameters for shared between multiple process context*/
typedef struct DLL_PACKED {
	MS_BOOL bGE_DirectToReg;
	MS_U16 global_tagID;
	MS_U16 u16ShareRegImage[GE_TABLE_REGNUM];
	MS_U16 u16ShareRegImageEx[GE_TABLE_REGNUM];
} GE_CTX_HAL_SHARED;

/*the following is for parameters for used in local process context*/
typedef struct {
	GE_CTX_HAL_SHARED *pHALShared;
	GE_CHIP_PROPERTY  *pGeChipPro;
	MS_VIRT           va_mmio_base;
	MS_VIRT           va_mmio_base2;
	MS_U16            u16RegGETable[GE_TABLE_REGNUM];                 //Store for GE RegInfo
	MS_U16            u16RegGETableEX[GE_TABLE_REGNUM];                 //Store for GE RegInfo
	MS_BOOL           bIsComp;
	MS_BOOL           bPaletteDirty;
	MS_U32            u32Palette[GE_PALETTE_NUM];
	MS_BOOL           bYScalingPatch;
	MS_U32           u32IpVersion;
} GE_CTX_HAL_LOCAL;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
void        GE_Chip_Proprity_Init(GE_CTX_HAL_LOCAL *pGEHalLocal);
void        GE_ResetState(GE_CTX_HAL_LOCAL *pGEHalLocal);
void        GE_WaitIdle(GE_CTX_HAL_LOCAL *pGEHalLocal);
void        GE_CODA_WriteReg(GE_CTX_HAL_LOCAL *pGEHalLocal, uint32_t addr, uint32_t value);
MS_U16      GE_CODA_ReadReg(GE_CTX_HAL_LOCAL *pGEHalLocal, uint32_t addr);
GE_Result   GE_InitCtxHalPalette(GE_CTX_HAL_LOCAL *pGEHalLocal);
void        GE_Init_HAL_Context(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_CTX_HAL_SHARED *pHALShared,
								MS_BOOL bNeedInitShared);
void        GE_Init_RegImage(GE_CTX_HAL_LOCAL *pGEHalLocal);
void        GE_Init(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_Config *cfg);
GE_Result   GE_SetRotate(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_RotateAngle geRotAngle);
GE_Result   GE_SetOnePixelMode(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL enable);
GE_Result   GE_SetBlend(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_BlendOp eBlendOp);
GE_Result   GE_SetAlpha(GE_CTX_HAL_LOCAL *pGEHalLocal, GFX_AlphaSrcFrom eAlphaSrc);
GE_Result   GE_EnableDFBBld(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL enable);
GE_Result   GE_SetDFBBldFlags(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U16 u16DFBBldFlags);
GE_Result   GE_SetDFBBldOP(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_DFBBldOP geSrcBldOP,
								GE_DFBBldOP geDstBldOP);
GE_Result   GE_SetDFBBldConstColor(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_RgbColor geRgbColor);
GE_Result   GE_GetFmtCaps(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_BufFmt fmt,
							GE_BufType type, GE_FmtCaps *caps);
MS_U16      GE_GetNextTAGID(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL bStepTagBefore);
GE_Result   GE_WaitTAGID(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U16 tagID);
GE_Result   GE_Restore_HAL_Context(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL bNotFirstInit);
GE_Result   GE_CalcBltScaleRatio(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U16 u16SrcWidth,
		MS_U16 u16SrcHeight, MS_U16 u16DstWidth,
		MS_U16 u16DstHeight, GE_ScaleInfo *pScaleinfo);
GE_Result  GE_SetBltScaleRatio(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_Rect *src, GE_DstBitBltType *dst,
			GE_Flag flags, GE_ScaleInfo *scaleinfo);
GE_Result   GE_SetVCmdBuffer(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_PHY PhyAddr,
								GFX_VcmqBufSize enBufSize);
MS_PHY    GE_ConvertAPIAddr2HAL(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U8 u8MIUId,
									MS_PHY PhyGE_APIAddrInMIU);
MS_BOOL     GE_NonOnePixelModeCaps(GE_CTX_HAL_LOCAL *pGEHalLocal, PatchBitBltInfo *patchInfo);
GE_Result   HAL_GE_EnableCalcSrc_WidthHeight(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL bEnable);
GE_Result   HAL_GE_AdjustDstWin(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL bDstXInv);
#if defined(CONFIG_GFX_TAG_INTERRUPT) && (__GE_WAIT_TAG_MODE == __USE_GE_INT_MODE)
GE_Result   HAL_GE_exit(GE_CTX_HAL_LOCAL *pGEHalLocal);
#endif
GE_Result   HAL_GE_SetBufferAddr(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_PHY PhyAddr,
									GE_BufType enBuffType);
GE_Result   HAL_GE_GetCRC(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U32 *pu32CRCvalue);
GE_Result HAL_GE_STR_RestoreReg(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_STR_SAVE_AREA *pGFX_STRPrivate);

#define      GE_SetSrcBufMIUId(pGEHalLocal, u8MIUId)  do {} while (0)
#define      GE_SetDstBufMIUId(pGEHalLocal, u8MIUId)  do {} while (0)
#define      GE_SetVQBufMIUId(pGEHalLocal, u8MIUId)  do {} while (0)
#define      GE_GetSrcBufMIUId(pGEHalLocal, u32GE_Addr)  ((MS_U8)(((u32GE_Addr)&(1UL<<31))>>31))
#define      GE_GetDstBufMIUId(pGEHalLocal, u32GE_Addr)  ((MS_U8)(((u32GE_Addr)&(1UL<<31))>>31))
#endif // _HAL_GE_H_
