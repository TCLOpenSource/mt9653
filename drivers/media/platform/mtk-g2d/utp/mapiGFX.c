// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef MSOS_TYPE_LINUX_KERNEL
#include <stdio.h>
#include <string.h>
#else
#include <linux/slab.h>
#endif
#include <../mtk-g2d-utp-wrapper.h>
#include "apiGFX.h"
#include "apiGFX_v2.h"
#include "drvGE.h"
#include "halGE.h"
#include "drvGE_private.h"
#include "apiGFX_private.h"
#include "sti_msos.h"

#define DO_DIV(x, y) ((x) /= (y))
#define DO_REMAIN(x, y) ((x)%(y))

//==========================================================
//Macro and Define
//==========================================================
#if (GFX_UTOPIA20)
#define g_apiGFXLocal   psGFXInstPri->GFXPrivate_g_apiGFXLocal
#endif

#define CheckSize(u32InputSize, u32StuctSize, eCmd)

#if (GE_API_MUTEX)
static MS_U32 u32MutexCnt;
#define _GFX_CheckMutex(_ret)  do { \
	if (u32MutexCnt != 0) { \
		G2D_ERROR("Err!MutexCnt=%td\n", (ptrdiff_t)u32MutexCnt);\
	} \
} while (0)


#define GET_GE_ENTRY(pGECtx)       do { \
	_GE_MUXTEX_ENTRY(pGECtx, E_GE_POOL_ID_INTERNAL_VARIABLE);\
	if (u32MutexCnt != 0) \
		G2D_ERROR("Err!MutexCnt=%td\n", (ptrdiff_t)u32MutexCnt);\
	u32MutexCnt++;\
} while (0)

#define RELEASE_GE_RETURN(pGECtx, _ret) do { \
	u32MutexCnt--;\
	_GFX_CheckMutex(_ret);\
	_GE_MUXTEX_RETURN(pGECtx, E_GE_POOL_ID_INTERNAL_VARIABLE);\
} while (0)

#endif
// API level could only contains local parameters

#define ABS(x)   ((x) > 0 ? (x) : -(x))

#define SETUP_DDA(xs, ys, xe, ye, dda) do {                                  \
	int dx = xe - xs;                \
	int dy = ye - ys;                \
	int result = 0;                  \
	dda.xi = xs;                     \
	if (dy != 0) {                   \
		result = dx;                \
		DO_DIV(result, (const int)dy);          \
		dda.mi = result;            \
		dda.mf = 2 * (DO_REMAIN(dx, (const int)dy)); \
		dda.xf = -dy;               \
		dda._2dy = 2 * dy;          \
		if (dda.mf < 0) {           \
			dda.mf += 2 * ABS(dy); \
			dda.mi--;              \
		}                           \
	}                                \
	else {                           \
		dda.mi = 0;                 \
		dda.mf = 0;                 \
		dda.xf = 0;                 \
		dda._2dy = 0;               \
	}                                \
} while (0)


#define INC_DDA(dda) do {                                  \
	dda.xi += dda.mi;                \
	dda.xf += dda.mf;                \
	if (dda.xf > 0) {                \
		dda.xi++;                   \
		dda.xf -= dda._2dy;         \
	}                                \
} while (0)

typedef struct {
	int xi;
	int xf;
	int mi;
	int mf;
	int _2dy;
} DDA;

#if (!GFX_UTOPIA20)
GFX_API_LOCAL g_apiGFXLocal = {
u32dbglvl: (-1),
fpGetBMP:NULL,
fpGetFont:NULL,
_angle:GEROTATE_0,
_bNearest:FALSE,
_bPatchMode:FALSE,
_bMirrorH:FALSE,
_bMirrorV:FALSE,
_bDstMirrorH:FALSE,
_bDstMirrorV:FALSE,
_bItalic:FALSE,
_line_enable:FALSE,
_line_pattern:0x00,
_line_factor:0,
#ifdef DBGLOG
_bOutFileLog:FALSE,
_pu16OutLogAddr:NULL,
_u16LogCount:0,
#endif
g_pGEContext:NULL,
pGeChipProperty:NULL,
u32LockStatus:0,
_bInit:0,
u32geRgbColor:0,

};
#endif

#if (GE_API_MUTEX)
static void _API_GE_ENTRY(GE_Context *pGECtx)
{
	GET_GE_ENTRY(pGECtx);
}

static MS_U16 _API_GE_RETURN(GE_Context *pGECtx, MS_U16 ret)
{
	RELEASE_GE_RETURN(pGECtx, ret);
	return ret;
}
#endif

static MS_S32 MapRet(GFX_Result eGFXResoult)
{
	MS_U32 u32Ret = UTOPIA_STATUS_FAIL;

	switch (eGFXResoult) {
	case GFX_FAIL:
		u32Ret = UTOPIA_STATUS_FAIL;
		break;
	case GFX_SUCCESS:
		u32Ret = UTOPIA_STATUS_SUCCESS;
		break;
	case GFX_DRV_NOT_SUPPORT:
		u32Ret = UTOPIA_STATUS_NOT_SUPPORTED;
		break;
	case GFX_NON_ALIGN_ADDRESS:
	case GFX_NON_ALIGN_PITCH:
	case GFX_INVALID_PARAMETERS:
		u32Ret = UTOPIA_STATUS_NOT_SUPPORTED;
		break;
	default:
		break;
	}
	return u32Ret;
}

static MS_U32 MApi_GFX_MapYUVOp(MS_U32 OpType, MS_U32 gfxOp, MS_U32 *geOP)
{

	switch (OpType) {
	case GFX_YUV_OP1:
		switch (gfxOp) {
		case GFX_YUV_RGB2YUV_PC:
			*geOP = (MS_U32) E_GE_YUV_RGB2YUV_PC;
			break;
		case GFX_YUV_RGB2YUV_255:
			*geOP = (MS_U32) E_GE_YUV_RGB2YUV_255;
			break;
		default:
			return (MS_U32) GFX_INVALID_PARAMETERS;
		}
		break;
	case GFX_YUV_OP2:
		switch (gfxOp) {
		case GFX_YUV_OUT_255:
			*geOP = (MS_U32) E_GE_YUV_OUT_255;
			break;
		case GFX_YUV_OUT_PC:
			*geOP = (MS_U32) E_GE_YUV_OUT_PC;
			break;
		default:
			return (MS_U32) GFX_INVALID_PARAMETERS;
		}
		break;
	case GFX_YUV_OP3:
		switch (gfxOp) {
		case GFX_YUV_IN_255:
			*geOP = (MS_U32) E_GE_YUV_IN_255;
			break;
		case GFX_YUV_IN_127:
			*geOP = (MS_U32) E_GE_YUV_IN_127;
			break;
		default:
			return (MS_U32) GFX_INVALID_PARAMETERS;
		}
		break;
	case GFX_YUV_OP4:
		switch (gfxOp) {
		case GFX_YUV_YVYU:
			*geOP = (MS_U32) E_GE_YUV_YVYU;
			break;
		case GFX_YUV_YUYV:
			*geOP = (MS_U32) E_GE_YUV_YUYV;
			break;
		case GFX_YUV_VYUY:
			*geOP = (MS_U32) E_GE_YUV_VYUY;
			break;
		case GFX_YUV_UYVY:
			*geOP = (MS_U32) E_GE_YUV_UYVY;
			break;
		default:
			return (MS_U32) GFX_INVALID_PARAMETERS;
		}
		break;
	default:
		return (MS_U32) GFX_INVALID_PARAMETERS;
	}
	return (MS_U32) GFX_SUCCESS;
}

static MS_U32 MApi_GFX_MapACmp(MS_U32 gfxACmp, MS_U32 *geACmp)
{

	switch (gfxACmp) {
	case GFX_ACMP_OP_MAX:
		*geACmp = (MS_U32) E_GE_ACMP_OP_MAX;
		break;
	case GFX_GE_ACMP_OP_MIN:
		*geACmp = (MS_U32) E_GE_ACMP_OP_MIN;
		break;
	default:
		return (MS_U32) GFX_INVALID_PARAMETERS;
	}

	return (MS_U32) GFX_SUCCESS;
}

static MS_U32 MApi_GFX_MapBLDCOEF(MS_U32 gfxCOEF, MS_U32 *geCOEF)
{
	switch (gfxCOEF) {
	case COEF_ONE:
		*geCOEF = (MS_U32) E_GE_BLEND_ONE;
		break;
	case COEF_CONST:
		*geCOEF = (MS_U32) E_GE_BLEND_CONST;
		break;
	case COEF_ASRC:
		*geCOEF = (MS_U32) E_GE_BLEND_ASRC;
		break;
	case COEF_ADST:
		*geCOEF = (MS_U32) E_GE_BLEND_ADST;
		break;
	case COEF_ZERO:
		*geCOEF = (MS_U32) E_GE_BLEND_ZERO;
		break;
	case COEF_1_CONST:
		*geCOEF = (MS_U32) E_GE_BLEND_CONST_INV;
		break;
	case COEF_1_ASRC:
		*geCOEF = (MS_U32) E_GE_BLEND_ASRC_INV;
		break;
	case COEF_1_ADST:
		*geCOEF = (MS_U32) E_GE_BLEND_ADST_INV;
		break;

	case COEF_ROP8_ALPHA:
		*geCOEF = (MS_U32) E_GE_BLEND_ROP8_ALPHA;
		break;
	case COEF_ROP8_SRCOVER:
		*geCOEF = (MS_U32) E_GE_BLEND_ROP8_SRCOVER;
		break;
	case COEF_ROP8_DSTOVER:
		*geCOEF = (MS_U32) E_GE_BLEND_ROP8_DSTOVER;
		break;
	case COEF_CONST_SRC:
		*geCOEF = (MS_U32) E_GE_BLEND_ALPHA_ADST;
		break;
	case COEF_1_CONST_SRC:
		*geCOEF = (MS_U32) E_GE_BLEND_INV_CONST;
		break;
	case COEF_SRC_ATOP_DST:
		*geCOEF = (MS_U32) E_GE_BLEND_SRC_ATOP_DST;
		break;
	case COEF_DST_ATOP_SRC:
		*geCOEF = (MS_U32) E_GE_BLEND_DST_ATOP_SRC;
		break;
	case COEF_SRC_XOR_DST:
		*geCOEF = (MS_U32) E_GE_BLEND_SRC_XOR_DST;
		break;
	case COEF_FADEIN:
		*geCOEF = (MS_U32) E_GE_BLEND_FADEIN;
		break;
	case COEF_FADEOUT:
		*geCOEF = (MS_U32) E_GE_BLEND_FADEOUT;
		break;
	default:
		return (MS_U32) GFX_INVALID_PARAMETERS;
	}

	return (MS_U32) GFX_SUCCESS;
}

static MS_U32 MApi_GFX_MapCKOP(MS_U32 gfxCKOP, MS_U32 *geCKOP)
{

	switch (gfxCKOP) {
	case CK_OP_EQUAL:
		*geCKOP = (MS_U32) E_GE_CK_EQ;
		break;
	case CK_OP_NOT_EQUAL:
		*geCKOP = (MS_U32) E_GE_CK_NE;
		break;
	case AK_OP_EQUAL:
		*geCKOP = (MS_U32) E_GE_ALPHA_EQ;
		break;
	case AK_OP_NOT_EQUAL:
		*geCKOP = (MS_U32) E_GE_ALPHA_NE;
		break;
	default:
		return (MS_U32) GFX_INVALID_PARAMETERS;
	}

	return (MS_U32) GFX_SUCCESS;
}

static MS_U32 GFX_RectBltFlags(void *pInstance)
{
	MS_U32 flags = 0;
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	if (g_apiGFXLocal._bItalic)
		flags |= E_GE_FLAG_BLT_ITALIC;

	if (g_apiGFXLocal._bMirrorH)
		flags |= E_GE_FLAG_BLT_MIRROR_H;

	if (g_apiGFXLocal._bMirrorV)
		flags |= E_GE_FLAG_BLT_MIRROR_V;

	if (g_apiGFXLocal._bDstMirrorH)
		flags |= E_GE_FLAG_BLT_DST_MIRROR_H;

	if (g_apiGFXLocal._bDstMirrorV)
		flags |= E_GE_FLAG_BLT_DST_MIRROR_V;

	switch (g_apiGFXLocal._angle) {
	case GEROTATE_90:
		flags |= E_GE_FLAG_BLT_ROTATE_90;
		break;
	case GEROTATE_180:
		flags |= E_GE_FLAG_BLT_ROTATE_180;
		break;
	case GEROTATE_270:
		flags |= E_GE_FLAG_BLT_ROTATE_270;
		break;
	default:
		break;
	}
	if (g_apiGFXLocal._bNearest)
		flags |= E_GE_FLAG_STRETCH_NEAREST;

	return flags;
}

static MS_U32 MApi_GFX_MapDFBBldFlag(MS_U16 gfxBldFlag, MS_U16 *geBldFlag)
{
	//the bld flag is one-to-one map:
	*geBldFlag = gfxBldFlag;

	return (MS_U32) GFX_SUCCESS;
}

static MS_U32 MApi_GFX_MapDFBBldOP(GFX_DFBBldOP gfxBldOP, GE_DFBBldOP *geBldOP)
{
	//the bld op is one-to-one map:
	*geBldOP = (GE_DFBBldOP) gfxBldOP;

	return (MS_U32) GFX_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// GFX tool function: Convert ARGB/index into the color format for intensity register.
/// @param Fmt \b IN: type of target color format.
/// @param colorinfo \b IN: pointer to color/index structure.
/// @param low \b OUT: pointer to 16-bit data to be filled in Intensity low word.
/// @param high \b OUT: pointer to 16-bit data to be filled in Intensity high word.
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
void GFX_ConvertRGB2DBFmt(GFX_Buffer_Format Fmt, MS_U32 *colorinfo, MS_U16 *low, MS_U16 *high)
{
	GFX_RgbColor *color = NULL;
	GFX_BlinkData *blinkData = NULL;
	//MS_U8 a, r, g, b;


	if ((colorinfo == NULL) || (low == NULL) || (high == NULL)) {
		G2D_ERROR("Convert RGB2DB FAIL\n");
		return;
	}

	if ((Fmt == GFX_FMT_1ABFGBG12355) || (Fmt == GFX_FMT_FABAFGBG2266))
		blinkData = (GFX_BlinkData *) colorinfo;
	else
		color = (GFX_RgbColor *) colorinfo;

	switch (Fmt) {
	case GFX_FMT_RGB565:
		*low =
		    ((color->b & 0xf8) +
		     (color->b >> 5)) | (((color->g & 0xfc) + (color->g >> 6)) << 8);
		*high = ((color->r & 0xf8) + (color->r >> 5)) | ((color->a & 0xff) << 8);
		break;
	case GFX_FMT_RGBA5551:
		if (color->a > 0)
			*low = ((color->g & 0xf8) + (color->g >> 5)) | (0xff << 8);
		else
			*low = ((color->g & 0xf8) + (color->g >> 5));

		*high =
		    ((color->g & 0xf8) +
		     (color->g >> 5)) | (((color->r & 0xf8) + (color->r >> 5)) << 8);
		break;
	case GFX_FMT_ARGB1555:
		*low =
		    ((color->b & 0xf8) +
		     (color->b >> 5)) | (((color->g & 0xf8) + (color->g >> 5)) << 8);
		if (color->a > 0)
			*high = ((color->r & 0xf8) + (color->r >> 5)) | (0xff << 8);
		else
			*high = ((color->r & 0xf8) + (color->r >> 5));

		break;
	case GFX_FMT_RGBA4444:
		*low =
		    ((color->a & 0xf0) +
		     (color->a >> 4)) | (((color->b & 0xf0) + (color->b >> 4)) << 8);
		*high =
		    ((color->g & 0xf0) +
		     (color->g >> 4)) | (((color->r & 0xf0) + (color->r >> 4)) << 8);
		break;
	case GFX_FMT_ARGB4444:
		*low =
		    ((color->b & 0xf0) +
		     (color->b >> 4)) | (((color->g & 0xf0) + (color->g >> 4)) << 8);
		*high =
		    ((color->r & 0xf0) +
		     (color->r >> 4)) | (((color->a & 0xf0) + (color->a >> 4)) << 8);
		break;
	case GFX_FMT_ABGR8888:
		*low = (color->r & 0xff) | ((color->g & 0xff) << 8);
		*high = (color->b & 0xff) | ((color->a & 0xff) << 8);
		break;
	case GFX_FMT_ARGB8888:
		*low = (color->b & 0xff) | ((color->g & 0xff) << 8);
		*high = (color->r & 0xff) | ((color->a & 0xff) << 8);
		break;
	case GFX_FMT_I8:
		*low = (color->b & 0xff) | ((color->b & 0xff) << 8);
		*high = (color->b & 0xff) | ((color->b & 0xff) << 8);
		break;
		// @FIXME: Richard uses GFX_FMT_1ABFGBG12355 instead
		//          1 A B Fg Bg
		//          1 2 3  5  5
	case GFX_FMT_1ABFGBG12355:
		*low = (0x1f & blinkData->background) |	// Bg: 4..0
		    ((0x1f & blinkData->foreground) << 5) |	// Fg: 9..5
		    ((0x1f & blinkData->ctrl_flag) << 10) |	// [A, B]: [14..13, 12..10]
		    BIT(15);	// Bit 15
		*high = (0x1f & blinkData->background) |	// Bg: 4..0
		    ((0x1f & blinkData->foreground) << 5) |	// Fg: 9..5
		    ((0x1f & blinkData->ctrl_flag) << 10) |	// [A, B]: [14..13, 12..10]
		    BIT(15);	// Bit 15
		break;
	case GFX_FMT_YUV422:
		//printf("[GE DRV][%06d] Are you sure to draw in YUV?\n", __LINE__);
		*low = (color->b & 0xff) | ((color->g & 0xff) << 8);
		*high = (color->r & 0xff) | ((color->a & 0xff) << 8);

		break;
	case GFX_FMT_FABAFGBG2266:
		*low =
		    ((blinkData->background & 0x3f) | ((blinkData->foreground & 0x3f) << 6) |
		    ((blinkData->Bits3.Ba & 0x3) << 12) |
		    ((blinkData->Bits3.Fa & 0x3) << 14));
		*high = *low;

		break;
	default:
		printf("[GE DRV][%06d] Bad color format\n", __LINE__);
		*low = (color->b & 0xff) | ((color->g & 0xff) << 8);
		*high = (color->r & 0xff) | ((color->a & 0xff) << 8);
		break;
	}

}

//-------------------------------------------------------------------------------------------------
/// GFX tool function: Convert ARGB/index into the color format for Primitive color.
/// @param Fmt \b IN: type of target color format.
/// @param colorinfo \b IN: pointer to color/index structure.
/// @param low \b OUT: pointer to 16-bit data to be filled in Intensity low word.
/// @param high \b OUT: pointer to 16-bit data to be filled in Intensity high word.
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
void GFX_ConvertRGB2PCFmt(GFX_Buffer_Format Fmt, MS_U32 *colorinfo, MS_U16 *low, MS_U16 *high)
{
	GFX_RgbColor *color = NULL;
	GFX_BlinkData *blinkData = NULL;
	//MS_U8 a, r, g, b;


	if ((colorinfo == NULL) || (low == NULL) || (high == NULL)) {
		G2D_ERROR("Convert RGB2DB FAIL\n");
		return;
	}

	if ((Fmt == GFX_FMT_1ABFGBG12355) || (Fmt == GFX_FMT_FABAFGBG2266))
		blinkData = (GFX_BlinkData *) colorinfo;
	else
		color = (GFX_RgbColor *) colorinfo;

	switch (Fmt) {
	case GFX_FMT_RGB565:
		*low = ((color->b & 0xf8) + (color->b >> 5)) |
		(((color->g & 0xfc) + (color->g >> 6)) << 8);
		*high = ((color->r & 0xf8) + (color->r >> 5)) | ((color->a & 0xff) << 8);
		break;
	case GFX_FMT_RGBA5551:
		if (color->a > 0)
			*low = ((color->g & 0xf8) + (color->g >> 5)) | (0xff << 8);
		else
			*low = ((color->g & 0xf8) + (color->g >> 5));

		*high =
		    ((color->g & 0xf8) +
		     (color->g >> 5)) | (((color->r & 0xf8) + (color->r >> 5)) << 8);
		break;
	case GFX_FMT_ARGB1555:
		*low =
		    ((color->b & 0xf8) +
		     (color->b >> 5)) | (((color->g & 0xf8) + (color->g >> 5)) << 8);
		if (color->a > 0)
			*high = ((color->r & 0xf8) + (color->r >> 5)) | (0xff << 8);
		else
			*high = ((color->r & 0xf8) + (color->r >> 5));

		break;
	case GFX_FMT_RGBA4444:
		*low =
		    ((color->a & 0xf0) +
		     (color->a >> 4)) | (((color->b & 0xf0) + (color->b >> 4)) << 8);
		*high =
		    ((color->g & 0xf0) +
		     (color->g >> 4)) | (((color->r & 0xf0) + (color->r >> 4)) << 8);
		break;
	case GFX_FMT_ARGB4444:
		*low =
		    ((color->b & 0xf0) +
		     (color->b >> 4)) | (((color->g & 0xf0) + (color->g >> 4)) << 8);
		*high =
		    ((color->r & 0xf0) +
		     (color->r >> 4)) | (((color->a & 0xf0) + (color->a >> 4)) << 8);
		break;
	case GFX_FMT_ABGR8888:
		*low = (color->r & 0xff) | ((color->g & 0xff) << 8);
		*high = (color->b & 0xff) | ((color->a & 0xff) << 8);
		break;
	case GFX_FMT_ARGB8888:
		*low = (color->b & 0xff) | ((color->g & 0xff) << 8);
		*high = (color->r & 0xff) | ((color->a & 0xff) << 8);
		break;
	case GFX_FMT_I8:
		*low = (color->b & 0xff) | ((color->b & 0xff) << 8);
		*high = (color->b & 0xff) | ((color->b & 0xff) << 8);
		break;
		// @FIXME: Richard uses GFX_FMT_1ABFGBG12355 instead
		//          1 A B Fg Bg
		//          1 2 3  5  5
	case GFX_FMT_1ABFGBG12355:
		*low = ((0x1f & blinkData->background) << 3) |	//Bg
		    (((0x1f & blinkData->foreground) << 3) << 8);	//Fg

		*high = ((0x1f & blinkData->ctrl_flag) << 3) | BIT(15);	//1AB
		break;
	case GFX_FMT_YUV422:
		//printf("[GE DRV][%06d] Are you sure to draw in YUV?\n", __LINE__);
		*low = (color->b & 0xff) | ((color->g & 0xff) << 8);
		*high = (color->r & 0xff) | ((color->a & 0xff) << 8);

		break;
	case GFX_FMT_FABAFGBG2266:
		*low =
		    ((blinkData->background & 0x3f) << 2) |
		    (((blinkData->foreground & 0x3f) << 2) << 8);
		*high =
		    ((blinkData->Bits3.Ba & 0x3) << 6) | (((blinkData->Bits3.Fa & 0x3) << 6) << 8);

		break;
	default:
		printf("[GE DRV][%06d] Bad color format\n", __LINE__);
		*low = (color->b & 0xff) | ((color->g & 0xff) << 8);
		*high = (color->r & 0xff) | ((color->a & 0xff) << 8);
		break;
	}

}

GFX_Result MApi_GFX_DrawLine_U02(void *pInstance, GFX_DrawLineInfo *pline)
{
	GFX_RgbColor color_s, color_e;
	MS_U32 u32data;
	MS_U16 u16Color0 = 0, u16Color1 = 0;

	GE_Point v0, v1;
	MS_U32 color, color2;
	MS_U32 flags = 0;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	v0.x = pline->x1;
	v0.y = pline->y1;
	color_s = pline->colorRange.color_s;
	memcpy(&u32data, &color_s, 4);
	GFX_ConvertRGB2PCFmt(pline->fmt, (MS_U32 *) &u32data, &u16Color0, &u16Color1);
	color = (u16Color1 << 16) | u16Color0;

	v1.x = pline->x2;
	v1.y = pline->y2;
	color_e = pline->colorRange.color_e;
	memcpy(&u32data, &color_e, 4);
	GFX_ConvertRGB2PCFmt(pline->fmt, (MS_U32 *) &u32data, &u16Color0, &u16Color1);
	color2 = (u16Color1 << 16) | u16Color0;

	if (pline->flag & GFXLINE_FLAG_COLOR_GRADIENT)
		flags |= E_GE_FLAG_LINE_GRADIENT;

	if (pline->flag & GFXLINE_FLAG_DRAW_LASTPOINT)
		flags |= E_GE_FLAG_DRAW_LASTPOINT;

	if (pline->flag & GFXLINE_FLAG_INWIDTH_IS_OUTWIDTH)
		flags |= E_GE_FLAG_INWIDTH_IS_OUTWIDTH;

	return (GFX_Result) MDrv_GE_DrawLine(g_apiGFXLocal.g_pGEContext, &v0, &v1, color, color2,
					     flags, pline->width);
}


GFX_Result MApi_GFX_RectFill_U02(void *pInstance, GFX_RectFillInfo *pfillblock)
{
	GFX_RgbColor color_s, color_e;
	MS_U16 u16Color0 = 0, u16Color1 = 0;
	GFX_BlinkData blinkData;

	GE_Rect rect;
	MS_U32 color, color2;
	MS_U32 flags = 0;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	rect.x = pfillblock->dstBlock.x;
	rect.y = pfillblock->dstBlock.y;
	rect.width = pfillblock->dstBlock.width;
	rect.height = pfillblock->dstBlock.height;

	color_s = pfillblock->colorRange.color_s;
	if (pfillblock->fmt == GFX_FMT_1ABFGBG12355 || pfillblock->fmt == GFX_FMT_FABAFGBG2266) {
		if (pfillblock->fmt == GFX_FMT_1ABFGBG12355) {
			memcpy(&blinkData, &color_s, sizeof(GFX_BlinkData));
			u16Color0 = (0x1F & blinkData.background) << 3 |
			    ((0x1F & blinkData.foreground) << 11);
			u16Color1 = (0x7 & blinkData.Bits.Blink) << 3 |
			    ((0x3 & blinkData.Bits.Alpha) << 6) | 0xff00;
		}
		if (pfillblock->fmt == GFX_FMT_FABAFGBG2266) {
			memcpy(&blinkData, &color_s, sizeof(GFX_BlinkData));
			u16Color0 = ((0x3F & blinkData.background) << 2) |
			    (((0x3F & blinkData.foreground) << 2) << 8);
			u16Color1 = ((0x3 & blinkData.Bits3.Ba) << 6) |
			    (((0x3 & blinkData.Bits3.Fa) << 6) << 8);
		}
		color = (u16Color1 << 16) | u16Color0;
	} else {
		memcpy(&color, &color_s, sizeof(color));
	}


	color_e = pfillblock->colorRange.color_e;
	if (pfillblock->fmt == GFX_FMT_1ABFGBG12355 || pfillblock->fmt == GFX_FMT_FABAFGBG2266) {
		if (pfillblock->fmt == GFX_FMT_1ABFGBG12355) {
			memcpy(&blinkData, &color_e, sizeof(GFX_BlinkData));
			u16Color0 = (0x1F & blinkData.background) << 3 |
			    ((0x1F & blinkData.foreground) << 11);
			u16Color1 = (0x7 & blinkData.Bits.Blink) << 3 |
			    ((0x3 & blinkData.Bits.Alpha) << 6) | 0xff00;
		}
		if (pfillblock->fmt == GFX_FMT_FABAFGBG2266) {
			memcpy(&blinkData, &color_e, sizeof(GFX_BlinkData));
			u16Color0 = ((0x3F & blinkData.background) << 2) |
			    (((0x3F & blinkData.foreground) << 2) << 8);
			u16Color1 = ((0x3 & blinkData.Bits3.Ba) << 6) |
			    (((0x3 & blinkData.Bits3.Fa) << 6) << 8);
		}
		color2 = (u16Color1 << 16) | u16Color0;

	} else {
		memcpy(&color2, &color_e, sizeof(color));
	}

	flags |= (pfillblock->flag & GFXRECT_FLAG_COLOR_GRADIENT_X) ? E_GE_FLAG_RECT_GRADIENT_X : 0;
	flags |= (pfillblock->flag & GFXRECT_FLAG_COLOR_GRADIENT_Y) ? E_GE_FLAG_RECT_GRADIENT_Y : 0;

	return (GFX_Result) MDrv_GE_FillRect(g_apiGFXLocal.g_pGEContext, &rect, color, color2,
					     flags);
}

GFX_Result MApi_GFX_TriFill_U02(void *pInstance, GFX_TriFillInfo *pfillblock)
{
	int y, yend;
	DDA dda0 = {.xi = 0 }, dda1 = {
	.xi = 0};
	int clip_x0 = 0, clip_x1 = 0, clip_y0 = 0, clip_y1 = 0;
	MS_BOOL bClip = 0;
	GFX_RectFillInfo rectInfo;

	if ((pfillblock->clip_box.width != 0) && (pfillblock->clip_box.height != 0)) {
		bClip = 1;
		clip_x0 = pfillblock->clip_box.x;
		clip_x1 = pfillblock->clip_box.x + pfillblock->clip_box.width;
		clip_y0 = pfillblock->clip_box.y;
		clip_y1 = pfillblock->clip_box.y + pfillblock->clip_box.height;
	}

	rectInfo.fmt = pfillblock->fmt;
	rectInfo.colorRange.color_s = pfillblock->colorRange.color_s;
	rectInfo.colorRange.color_e = pfillblock->colorRange.color_e;
	rectInfo.flag = pfillblock->flag;

	y = pfillblock->tri.y0;
	yend = pfillblock->tri.y2;

	if ((bClip == 1) && (yend > clip_y1))
		yend = clip_y1;

	SETUP_DDA(pfillblock->tri.x0, pfillblock->tri.y0, pfillblock->tri.x2, pfillblock->tri.y2,
		  dda0);
	SETUP_DDA(pfillblock->tri.x0, pfillblock->tri.y0, pfillblock->tri.x1, pfillblock->tri.y1,
		  dda1);

	while (y <= yend) {
		if (y == pfillblock->tri.y1) {
			if (pfillblock->tri.y1 == pfillblock->tri.y2)
				return GFX_SUCCESS;
			SETUP_DDA(pfillblock->tri.x1, pfillblock->tri.y1, pfillblock->tri.x2,
				  pfillblock->tri.y2, dda1);
		}

		rectInfo.dstBlock.width = ABS(dda0.xi - dda1.xi);
		rectInfo.dstBlock.x = MIN(dda0.xi, dda1.xi);

		if ((bClip == 1) && (clip_x1 < rectInfo.dstBlock.x + rectInfo.dstBlock.width))
			rectInfo.dstBlock.width = clip_x1 - rectInfo.dstBlock.x + 1;

		if (rectInfo.dstBlock.width > 0) {
			if ((bClip == 1) && (clip_x0 > rectInfo.dstBlock.x)) {
				rectInfo.dstBlock.width -= (clip_x0 - rectInfo.dstBlock.x);
				rectInfo.dstBlock.x = clip_x0;
			}
			rectInfo.dstBlock.y = y;
			rectInfo.dstBlock.height = 1;

			if (rectInfo.dstBlock.width > 0) {
				if ((bClip == 1) && (rectInfo.dstBlock.y >= clip_y0))
					MApi_GFX_RectFill_U02(pInstance, &rectInfo);
				else if (bClip == 0)
					MApi_GFX_RectFill_U02(pInstance, &rectInfo);

			}
		}

		INC_DDA(dda0);
		INC_DDA(dda1);

		y++;
	}
	return GFX_SUCCESS;
}

MS_BOOL clip_rectangle(GFX_Block clip, GFX_Block *dstBlock)
{
	if ((clip.x >= dstBlock->x + dstBlock->width) ||
	    ((clip.x + clip.width) < dstBlock->x) ||
	    (clip.y >= dstBlock->y + dstBlock->height) || ((clip.y + clip.height) < dstBlock->y))
		return FALSE;

	if (clip.x > dstBlock->x) {
		dstBlock->width += dstBlock->x - clip.x;
		dstBlock->x = clip.x;
	}

	if (clip.y > dstBlock->y) {
		dstBlock->height += dstBlock->y - clip.y;
		dstBlock->y = clip.y;
	}

	if ((clip.x + clip.width) < dstBlock->x + dstBlock->width - 1)
		dstBlock->width = (clip.x + clip.width) - dstBlock->x + 1;

	if ((clip.y + clip.height) < dstBlock->y + dstBlock->height - 1)
		dstBlock->height = (clip.y + clip.height) - dstBlock->y + 1;

	return TRUE;
}

GFX_Result MApi_GFX_SpanFill_U02(void *pInstance, GFX_SpanFillInfo *pfillblock)
{
	GFX_RectFillInfo rectInfo;
	MS_BOOL bClip = 0;
	int i;

	rectInfo.fmt = pfillblock->fmt;
	rectInfo.colorRange.color_s = pfillblock->colorRange.color_s;
	rectInfo.colorRange.color_e = pfillblock->colorRange.color_e;
	rectInfo.flag = pfillblock->flag;

	if ((pfillblock->clip_box.width != 0) && (pfillblock->clip_box.height != 0))
		bClip = 1;

	for (i = 0; i < pfillblock->span.num_spans; i++) {
		rectInfo.dstBlock.x = pfillblock->span.spans[i].x;
		rectInfo.dstBlock.y = pfillblock->span.y + i;
		rectInfo.dstBlock.width = pfillblock->span.spans[i].w;
		rectInfo.dstBlock.height = 1;


		if (bClip && !clip_rectangle(pfillblock->clip_box, &rectInfo.dstBlock))
			continue;

		MApi_GFX_RectFill_U02(pInstance, &rectInfo);
	}

	return GFX_SUCCESS;
}

GFX_Result MApi_GFX_BitBlt_U02(void *pInstance, GFX_DrawRect *drawbuf, MS_U32 drawflag,
			       GFX_ScaleInfo *ScaleInfo)
{
	union {
		GFX_Block drawbufblk;
		GE_Rect blk;
		GE_DstBitBltType dstblk;
		GFX_Trapezoid dsttrapeblk;
	} sBltSrcBlk, sBltDstBlk;


	GE_ScaleInfo gecaleInfo;
	MS_U32 flags = 0;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	if (drawflag & GFXDRAW_FLAG_SCALE)
		flags |= E_GE_FLAG_BLT_STRETCH;

	if (drawflag & GFXDRAW_FLAG_TRAPEZOID_X)
		flags |= E_GE_FLAG_TRAPEZOID_X;

	if (drawflag & GFXDRAW_FLAG_TRAPEZOID_Y)
		flags |= E_GE_FLAG_TRAPEZOID_Y;

	flags |= GFX_RectBltFlags(pInstance);

	if (ScaleInfo != NULL) {
		if ((ScaleInfo->u32DeltaX != 0) && (ScaleInfo->u32DeltaY != 0)) {
			gecaleInfo.init_x = ScaleInfo->u32InitDelatX;
			gecaleInfo.init_y = ScaleInfo->u32InitDelatY;
			gecaleInfo.x = ScaleInfo->u32DeltaX;
			gecaleInfo.y = ScaleInfo->u32DeltaY;

			flags |= E_GE_FLAG_BYPASS_STBCOEF;
		}
	}

	if (!(flags & (E_GE_FLAG_BLT_STRETCH |
		       E_GE_FLAG_BLT_ITALIC |
		       E_GE_FLAG_BLT_MIRROR_H |
		       E_GE_FLAG_BLT_MIRROR_V |
		       E_GE_FLAG_BLT_ROTATE_90 |
		       E_GE_FLAG_BLT_ROTATE_180 | E_GE_FLAG_BLT_ROTATE_270))) {
		flags |= E_GE_FLAG_BLT_OVERLAP;
	}

	sBltSrcBlk.drawbufblk = drawbuf->srcblk;
	sBltDstBlk.dsttrapeblk = drawbuf->dsttrapeblk;
	return (GFX_Result) MDrv_GE_BitBltEX(g_apiGFXLocal.g_pGEContext, &sBltSrcBlk.blk,
					     &sBltDstBlk.dstblk, flags,
					     (ScaleInfo == NULL) ? NULL : &gecaleInfo);
}

GFX_Result MApi_GFX_SetSrcColorKey_U02(void *pInstance, MS_BOOL enable,
				       GFX_ColorKeyMode opMode,
				       GFX_Buffer_Format fmt, void *ps_color, void *pe_color)
{
	MS_U32 ck_low, ck_high, u32op1 = 0, ret;
	MS_U16 u16Color0 = 0, u16Color1 = 0;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	GFX_ConvertRGB2DBFmt(fmt, (MS_U32 *) ps_color, &u16Color0, &u16Color1);
	ck_low = (u16Color1 << 16) | u16Color0;
	GFX_ConvertRGB2DBFmt(fmt, (MS_U32 *) pe_color, &u16Color0, &u16Color1);
	ck_high = (u16Color1 << 16) | u16Color0;

	ret = MApi_GFX_MapCKOP(opMode, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);


	return (GFX_Result) MDrv_GE_SetSrcColorKey(g_apiGFXLocal.g_pGEContext, enable,
						   (GE_CKOp) u32op1, ck_low, ck_high);
}

GFX_Result MApi_GFX_SetDstColorKey_U02(void *pInstance, MS_BOOL enable,
				       GFX_ColorKeyMode opMode,
				       GFX_Buffer_Format fmt, void *ps_color, void *pe_color)
{
	MS_U32 ck_low, ck_high, u32op1 = 0, ret;
	MS_U16 u16Color0 = 0, u16Color1 = 0;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	GFX_ConvertRGB2DBFmt(fmt, (MS_U32 *) ps_color, &u16Color0, &u16Color1);
	ck_low = (u16Color1 << 16) | u16Color0;
	GFX_ConvertRGB2DBFmt(fmt, (MS_U32 *) pe_color, &u16Color0, &u16Color1);
	ck_high = (u16Color1 << 16) | u16Color0;


	ret = MApi_GFX_MapCKOP(opMode, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	return (GFX_Result) MDrv_GE_SetDstColorKey(g_apiGFXLocal.g_pGEContext, enable,
						   (GE_CKOp) u32op1, ck_low, ck_high);
}


GFX_Result MApi_GFX_SetROP2_U02(void *pInstance, MS_BOOL enable, GFX_ROP2_Op eRopMode)
{
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	return (GFX_Result) MDrv_GE_SetROP2(g_apiGFXLocal.g_pGEContext, enable,
					    eRopMode);
}

GFX_Result MApi_GFX_SetIntensity_U02(void *pInstance, MS_U32 id, GFX_Buffer_Format fmt,
				     MS_U32 *pColor)
{
	MS_U16 u16Color0 = 0, u16Color1 = 0;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	GFX_ConvertRGB2DBFmt(fmt, (MS_U32 *) pColor, &u16Color0, &u16Color1);

	return (GFX_Result) MDrv_GE_SetIntensity(g_apiGFXLocal.g_pGEContext, id,
						 (u16Color1 << 16) | u16Color0);
}

GFX_Result MApi_GFX_SetDFBBldOP_U02(void *pInstance, GFX_DFBBldOP gfxSrcBldOP,
				    GFX_DFBBldOP gfxDstBldOP)
{
	GE_DFBBldOP geSrcBldOP, geDstBldOP;
	GFX_Result u32Ret;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	u32Ret = (GFX_Result) MApi_GFX_MapDFBBldOP(gfxSrcBldOP, &geSrcBldOP);
	if (u32Ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", u32Ret);

	u32Ret = (GFX_Result) MApi_GFX_MapDFBBldOP(gfxDstBldOP, &geDstBldOP);
	if (u32Ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", u32Ret);

	return (GFX_Result) MDrv_GE_SetDFBBldOP(g_apiGFXLocal.g_pGEContext, geSrcBldOP, geDstBldOP);
}

GFX_Result MApi_GFX_SetDFBBldConstColor_U02(void *pInstance, GFX_RgbColor gfxRgbColor)
{
	GE_RgbColor geRgbColor;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	geRgbColor.a = gfxRgbColor.a;
	geRgbColor.r = gfxRgbColor.r;
	geRgbColor.g = gfxRgbColor.g;
	geRgbColor.b = gfxRgbColor.b;

	return (GFX_Result) MDrv_GE_SetDFBBldConstColor(g_apiGFXLocal.g_pGEContext, geRgbColor);
}

GFX_Result MApi_GFX_SetDFBBldFlags_U02(void *pInstance, MS_U16 u16DFBBldFlags)
{
	MS_U16 u16DrvDFBBldFlags;
	GFX_Result u32Ret;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	u32Ret = (GFX_Result) MApi_GFX_MapDFBBldFlag(u16DFBBldFlags, &u16DrvDFBBldFlags);
	if (u32Ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", u32Ret);

	return (GFX_Result) MDrv_GE_SetDFBBldFlags(g_apiGFXLocal.g_pGEContext, u16DrvDFBBldFlags);
}

GFX_Result MApi_GFX_SetClip_U02(void *pInstance, GFX_Point *v0, GFX_Point *v1)
{
	GE_Rect rect;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	rect.x = v0->x;		// dangerous if V0 > V1
	rect.y = v0->y;
	rect.width = v1->x - v0->x + 1;
	rect.height = v1->y - v0->y + 1;

	return (GFX_Result) MDrv_GE_SetClipWindow(g_apiGFXLocal.g_pGEContext, &rect);
}

GFX_Result MApi_GFX_SetPaletteOpt_U02(void *pInstance, GFX_PaletteEntry *pPalArray,
				      MS_U16 u32PalStart, MS_U16 u32PalEnd)
{
	MS_U16 i;
	//----------------------------------------------------------------------
	// Write palette
	//----------------------------------------------------------------------
	//U32 clr;
	MS_U16 j = 0;
	MS_U32 u32data;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	for (i = u32PalStart; i <= u32PalEnd; i++) {

		//G2D_DEBUG
		//("MDrv_GE_SetPaletteOpt : Array[%d]. u8A %d |u8R %d |u8G %d |u8B %d\n",
		//i, pPalArray[j].RGB.u8A, pPalArray[j].RGB.u8R, pPalArray[j].RGB.u8G,
		//pPalArray[j].RGB.u8B);

		memcpy(&u32data, &pPalArray[j], 4);
		MDrv_GE_SetPalette(g_apiGFXLocal.g_pGEContext, i, 1, (MS_U32 *) &u32data);

		j++;

	}
	return GFX_SUCCESS;

}

GFX_Result MApi_GFX_SetVCmdBuffer_U02(void *pInstance, MS_PHY PhyAddr, GFX_VcmqBufSize enBufSize)
{
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	if (E_GE_OK !=
	    MDrv_GE_SetVCmdBuffer(g_apiGFXLocal.g_pGEContext, PhyAddr, enBufSize)) {
		return GFX_FAIL;
	}

	return GFX_SUCCESS;
}

GFX_Result MApi_GFX_EnableAlphaBlending_U02(void *pInstance, MS_BOOL enable)
{
	MS_U32 u32op1 = 0, ret;
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	ret = MApi_GFX_MapBLDCOEF(g_apiGFXLocal.pABLInfo.eBldCoef, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	return (GFX_Result) MDrv_GE_SetAlphaBlend(g_apiGFXLocal.g_pGEContext, enable,
						  (GE_BlendOp) u32op1);
}

GFX_Result MApi_GFX_SetAlpha_U02(void *pInstance, MS_BOOL enable, GFX_BlendCoef coef,
				 GFX_AlphaSrcFrom db_abl, MS_U8 abl_const)
{
	MS_U32 u32op1 = 0, ret;
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	ret = MApi_GFX_MapBLDCOEF(coef, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	MDrv_GE_SetAlphaConst(g_apiGFXLocal.g_pGEContext, abl_const);
	MDrv_GE_SetAlphaSrc(g_apiGFXLocal.g_pGEContext, db_abl);
	MDrv_GE_SetAlphaBlend(g_apiGFXLocal.g_pGEContext, enable, (GE_BlendOp) u32op1);

	return GFX_SUCCESS;
}

GFX_Result MApi_GFX_ClearFrameBufferByWord_U02(void *pInstance, MS_PHY StrAddr, MS_U32 length,
					       MS_U32 ClearValue)
{
#define CLRbW_FB_WIDTH    1024UL
#define CLRbW_FB_PITCH    (CLRbW_FB_WIDTH*4)
#define CLRbW_FB_HEIGHT   128UL
#define CLRbW_FB_SIZE     (CLRbW_FB_HEIGHT * CLRbW_FB_PITCH)

	GE_Rect rect;
	MS_U32 color, color2;
	MS_PHY tmpaddr;
	GE_BufInfo bufinfo;

	GE_Rect clip;
	MS_U32 flags = 0;
	MS_U16 u16RegGEEN = 0;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	if ((length % 4) != 0)
		return GFX_INVALID_PARAMETERS;

	MDrv_GE_RestoreRegInfo(g_apiGFXLocal.g_pGEContext, E_GE_SAVE_REG_GE_EN, &u16RegGEEN);
	MDrv_GE_RestoreRegInfo(g_apiGFXLocal.g_pGEContext, E_GE_DISABLE_REG_EN, &u16RegGEEN);

	MDrv_GE_GetBufferInfo(g_apiGFXLocal.g_pGEContext, &bufinfo);
	MDrv_GE_SetAlphaSrc(g_apiGFXLocal.g_pGEContext, ALPHA_FROM_ASRC);
	MDrv_GE_SetAlphaBlend(g_apiGFXLocal.g_pGEContext, FALSE, E_GE_BLEND_ONE);
	clip.x = 0;
	clip.y = 0;
	clip.width = 1920;
	clip.height = 1080;
	MDrv_GE_SetClipWindow(g_apiGFXLocal.g_pGEContext, &clip);


	color = ClearValue;
	color2 = color;

	flags = 0;
	tmpaddr = StrAddr;

	while (length >= CLRbW_FB_PITCH) {
		rect.height = length / CLRbW_FB_PITCH;
		if (rect.height > 1080)
			rect.height = 1080;

		rect.x = 0;
		rect.y = 0;
		rect.width = CLRbW_FB_WIDTH;
//        rect.height = CLR_FB_HEIGHT;
		MDrv_GE_SetDstBuffer(g_apiGFXLocal.g_pGEContext, E_GE_FMT_ARGB8888, CLRbW_FB_WIDTH,
				     1, tmpaddr, CLRbW_FB_PITCH, 0);
		MDrv_GE_FillRect(g_apiGFXLocal.g_pGEContext, &rect, color, color2, flags);
		tmpaddr += (CLRbW_FB_PITCH * rect.height);
		length -= (CLRbW_FB_PITCH * rect.height);
	}


	if (length > 0) {
		rect.x = 0;
		rect.y = 0;
		rect.width = length / 4;
		rect.height = 1;
		MDrv_GE_SetDstBuffer(g_apiGFXLocal.g_pGEContext, E_GE_FMT_ARGB8888, CLRbW_FB_WIDTH,
				     1, tmpaddr, CLRbW_FB_PITCH, 0);
		MDrv_GE_FillRect(g_apiGFXLocal.g_pGEContext, &rect, color, color2, flags);
		tmpaddr += length;
	}

	MDrv_GE_SetDstBuffer(g_apiGFXLocal.g_pGEContext, (GE_BufFmt) bufinfo.dstfmt, 0, 0,
			     bufinfo.dstaddr, bufinfo.dstpit, 0);
	//MDrv_GE_WaitIdle();
	MDrv_GE_RestoreRegInfo(g_apiGFXLocal.g_pGEContext, E_GE_RESTORE_REG_GE_EN, &u16RegGEEN);

	return GFX_SUCCESS;

}
GFX_Result MApi_GFX_ClearFrameBuffer_U02(void *pInstance, MS_PHY StrAddr, MS_U32 length,
					 MS_U8 ClearValue)
{
#define CLR_FB_PITCH    1024UL
#define CLR_FB_HEIGHT   256UL
#define CLR_FB_SIZE     (CLR_FB_HEIGHT * CLR_FB_PITCH)

	GE_Rect rect;
	MS_U32 color, color2;
	MS_PHY tmpaddr;
	GE_BufInfo bufinfo;
	GE_Rect clip;
	MS_U32 flags = 0;
	MS_U16 u16RegGEEN = 0;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	MDrv_GE_RestoreRegInfo(g_apiGFXLocal.g_pGEContext, E_GE_SAVE_REG_GE_EN, &u16RegGEEN);
	MDrv_GE_RestoreRegInfo(g_apiGFXLocal.g_pGEContext, E_GE_DISABLE_REG_EN, &u16RegGEEN);

	MDrv_GE_GetBufferInfo(g_apiGFXLocal.g_pGEContext, &bufinfo);
	MDrv_GE_SetAlphaSrc(g_apiGFXLocal.g_pGEContext, ALPHA_FROM_ASRC);
	MDrv_GE_SetAlphaBlend(g_apiGFXLocal.g_pGEContext, FALSE, E_GE_BLEND_ONE);

	clip.x = 0;
	clip.y = 0;
	clip.width = 1920;
	clip.height = 1080;
	MDrv_GE_SetClipWindow(g_apiGFXLocal.g_pGEContext, &clip);

	color = (ClearValue << 8) + ClearValue;
	color2 = color;

	flags = 0;
	tmpaddr = StrAddr;

	while (length >= CLR_FB_PITCH) {
		rect.height = length / CLR_FB_PITCH;
		if (rect.height > 1080)
			rect.height = 1080;

		rect.x = 0;
		rect.y = 0;
		rect.width = CLR_FB_PITCH;
//        rect.height = CLR_FB_HEIGHT;
		MDrv_GE_SetDstBuffer(g_apiGFXLocal.g_pGEContext, E_GE_FMT_I8, CLR_FB_PITCH, 1,
				     tmpaddr, CLR_FB_PITCH, 0);
		MDrv_GE_FillRect(g_apiGFXLocal.g_pGEContext, &rect, color, color2, flags);
		tmpaddr += (CLR_FB_PITCH * rect.height);
		length -= (CLR_FB_PITCH * rect.height);
	}

	if (length > 0) {
		rect.x = 0;
		rect.y = 0;
		rect.width = length;
		rect.height = 1;
		MDrv_GE_SetDstBuffer(g_apiGFXLocal.g_pGEContext, E_GE_FMT_I8, CLR_FB_PITCH, 1,
				     tmpaddr, CLR_FB_PITCH, 0);
		MDrv_GE_FillRect(g_apiGFXLocal.g_pGEContext, &rect, color, color2, flags);
		tmpaddr += length;
	}

	MDrv_GE_SetDstBuffer(g_apiGFXLocal.g_pGEContext, (GE_BufFmt) bufinfo.dstfmt, 0, 0,
			     bufinfo.dstaddr, bufinfo.dstpit, 0);
	//MDrv_GE_WaitIdle();
	MDrv_GE_RestoreRegInfo(g_apiGFXLocal.g_pGEContext, E_GE_RESTORE_REG_GE_EN, &u16RegGEEN);

	return GFX_SUCCESS;
}

GFX_Result MApi_GFX_SetAlphaCmp_U02(void *pInstance, MS_BOOL enable, GFX_ACmpOp eMode)
{
	MS_U32 u32op1 = 0, ret;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	ret = MApi_GFX_MapACmp(eMode, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	return (GFX_Result) MDrv_GE_SetAlphaCmp(g_apiGFXLocal.g_pGEContext, enable,
						(GE_ACmpOp) u32op1);
}

GFX_Result MApi_GFX_Set_Line_Pattern_U02(void *pInstance, MS_BOOL enable, MS_U8 linePattern,
					 MS_U8 repeatFactor)
{
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	g_apiGFXLocal._line_enable = enable;
	g_apiGFXLocal._line_pattern = linePattern;
	g_apiGFXLocal._line_factor = repeatFactor;
	return (GFX_Result) MDrv_GE_SetLinePattern(g_apiGFXLocal.g_pGEContext,
						   g_apiGFXLocal._line_enable,
						   g_apiGFXLocal._line_pattern,
						   (GE_LinePatRep) g_apiGFXLocal._line_factor);
}

GFX_Result MApi_GFX_BeginDraw_U02(void *pInstance)
{
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	GE_Get_Resource(g_apiGFXLocal.g_pGEContext, TRUE);

	g_apiGFXLocal.u32LockStatus++;
	return (GFX_Result) E_GE_OK;
}

GFX_Result MApi_GFX_EndDraw_U02(void *pInstance)
{
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	if (g_apiGFXLocal.u32LockStatus == 0)
		return (GFX_Result) E_GE_FAIL_LOCKED;

	GE_Free_Resource(g_apiGFXLocal.g_pGEContext, TRUE);

	g_apiGFXLocal.u32LockStatus--;
	return (GFX_Result) E_GE_OK;
}

GFX_Result MApi_GFX_DrawOval_U02(void *pInstance, GFX_OvalFillInfo *pOval)
{
	GE_OVAL_FILL_INFO stOval;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	memcpy(&stOval, pOval, sizeof(GE_OVAL_FILL_INFO));
	MDrv_GE_DrawOval(g_apiGFXLocal.g_pGEContext, &stOval);
	return GFX_SUCCESS;
}

GFX_Result MApi_GFX_SetDC_CSC_FMT_U02(void *pInstance, GFX_YUV_Rgb2Yuv mode,
				      GFX_YUV_OutRange yuv_out_range, GFX_YUV_InRange uv_in_range,
				      GFX_YUV_422 srcfmt, GFX_YUV_422 dstfmt)
{
	GE_YUVMode yuvmode;
	MS_U32 u32op1 = 0, ret;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	ret = MApi_GFX_MapYUVOp(GFX_YUV_OP1, mode, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	yuvmode.rgb2yuv = (GE_Csc_Rgb2Yuv) u32op1;

	ret = MApi_GFX_MapYUVOp(GFX_YUV_OP2, yuv_out_range, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	yuvmode.out_range = (GE_YUV_OutRange) u32op1;

	ret = MApi_GFX_MapYUVOp(GFX_YUV_OP3, uv_in_range, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	yuvmode.in_range = (GE_YUV_InRange) u32op1;

	ret = MApi_GFX_MapYUVOp(GFX_YUV_OP4, dstfmt, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	yuvmode.dst_fmt = (GE_YUV_422) u32op1;

	ret = MApi_GFX_MapYUVOp(GFX_YUV_OP4, srcfmt, &u32op1);
	if (ret != GFX_SUCCESS)
		G2D_ERROR("ERROR: %u\n", ret);

	yuvmode.src_fmt = (GE_YUV_422) u32op1;

	MDrv_GE_SetYUVMode(g_apiGFXLocal.g_pGEContext, &yuvmode);

	return GFX_SUCCESS;
}

GFX_Result MApi_GFX_SetStrBltSckType_U02(void *pInstance, GFX_StretchCKType type,
					 GFX_RgbColor *color)
{
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	return (GFX_Result)MDrv_GE_SetStrBltSckType(g_apiGFXLocal.g_pGEContext,
					      type, (MS_U32 *) (void *)color);
}

static MS_BOOL GFX_SetFireInfo(void *pInstance, GFX_FireInfo *pFireInfo)
{
	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	if (pFireInfo->eFireInfo & GFX_SRC_INFO) {
		MDrv_GE_SetSrcBuffer(g_apiGFXLocal.g_pGEContext,
				     (GE_BufFmt) pFireInfo->SrcbufInfo.u32ColorFmt,
				     pFireInfo->SrcbufInfo.u32Width,
				     pFireInfo->SrcbufInfo.u32Height, pFireInfo->SrcbufInfo.u32Addr,
				     pFireInfo->SrcbufInfo.u32Pitch, pFireInfo->u32SrcOffsetofByte);
	}
	if (pFireInfo->eFireInfo & GFX_DST_INFO) {
		MDrv_GE_SetDstBuffer(g_apiGFXLocal.g_pGEContext,
				     (GE_BufFmt) pFireInfo->DstbufInfo.u32ColorFmt,
				     pFireInfo->DstbufInfo.u32Width,
				     pFireInfo->DstbufInfo.u32Height, pFireInfo->DstbufInfo.u32Addr,
				     pFireInfo->DstbufInfo.u32Pitch, pFireInfo->u32DstOffsetofByte);
	}
	if (pFireInfo->eFireInfo & GFX_CLIP_INFO) {
		MApi_GFX_SetClip_U02(pInstance, &(pFireInfo->GFXSetClip.V0),
				     &(pFireInfo->GFXSetClip.V1));
	}
	if (pFireInfo->eFireInfo & GFX_DFB_INFO) {
		g_apiGFXLocal.u32geRgbColor =
		(pFireInfo->GFXSetDFB.sRGBColor.a) << 24 |
		(pFireInfo->GFXSetDFB.sRGBColor.r) << 16 |
		(pFireInfo->GFXSetDFB.sRGBColor.g) << 8 | (pFireInfo->GFXSetDFB.sRGBColor.b);
		MDrv_GE_EnableDFBBlending(g_apiGFXLocal.g_pGEContext, pFireInfo->GFXSetDFB.bEnable);
		MApi_GFX_SetDFBBldOP_U02(pInstance, pFireInfo->GFXSetDFB.eSrcBldOP,
					 pFireInfo->GFXSetDFB.eDstBldOP);
		MApi_GFX_SetDFBBldConstColor_U02(pInstance, pFireInfo->GFXSetDFB.sRGBColor);
		MApi_GFX_SetDFBBldFlags_U02(pInstance, pFireInfo->GFXSetDFB.u16DFBBldFlags);
	}
	if (pFireInfo->eFireInfo & GFX_ABL_INFO) {
		MS_U32 u32BLDCOEF = 0;
		MS_U32 u32ABLSRC = 0;

		g_apiGFXLocal.u32geRgbColor = (g_apiGFXLocal.u32geRgbColor & 0x00ffffff) |
		((pFireInfo->GFXSetABL.u8Alpha_Const) << 24);
		if (MApi_GFX_MapBLDCOEF(pFireInfo->GFXSetABL.eABLCoef, &u32BLDCOEF) !=
		    (MS_U32) GFX_SUCCESS) {
			G2D_ERROR("Fail\n");
			return FALSE;
		}

		g_apiGFXLocal.pABLInfo.eBldCoef = (GE_BlendOp) u32BLDCOEF;
		g_apiGFXLocal.pABLInfo.eABLSrc = u32ABLSRC;
		g_apiGFXLocal.pABLInfo.u32ABLConstCoef =
		    (MS_U32) pFireInfo->GFXSetABL.u8Alpha_Const;
		MApi_GFX_SetAlpha_U02(pInstance, pFireInfo->GFXSetABL.bEnable,
				      pFireInfo->GFXSetABL.eABLCoef, pFireInfo->GFXSetABL.eDb_abl,
				      pFireInfo->GFXSetABL.u8Alpha_Const);
	}
	if (pFireInfo->eFireInfo & GFX_SRC_CLRKEY_INFO) {
		MApi_GFX_SetSrcColorKey_U02(pInstance, pFireInfo->GFXSetSrcColorKey.bEnable,
					    pFireInfo->GFXSetSrcColorKey.eOpMode,
					    pFireInfo->GFXSetSrcColorKey.eFmt,
					    &(pFireInfo->GFXSetSrcColorKey.S_color),
					    &(pFireInfo->GFXSetSrcColorKey.E_color));
	}
	if (pFireInfo->eFireInfo & GFX_DST_CLRKEY_INFO) {
		MApi_GFX_SetDstColorKey_U02(pInstance, pFireInfo->GFXSetDstColorKey.bEnable,
					    pFireInfo->GFXSetDstColorKey.eOpMode,
					    pFireInfo->GFXSetDstColorKey.eFmt,
					    &(pFireInfo->GFXSetDstColorKey.S_color),
					    &(pFireInfo->GFXSetDstColorKey.E_color));
	}
	if (pFireInfo->eFireInfo & GFX_ALPHA_CMP_INFO) {
		MApi_GFX_SetAlphaCmp_U02(pInstance, pFireInfo->GFXSetAlphaCmp.enable,
					 pFireInfo->GFXSetAlphaCmp.eMode);
	}
	if (pFireInfo->eFireInfo & GFX_SRC_MIRROR_INFO) {
		g_apiGFXLocal._bMirrorH = pFireInfo->GFXSetMirror.bMirrorX;
		g_apiGFXLocal._bMirrorV = pFireInfo->GFXSetMirror.bMirrorY;
	}
	if (pFireInfo->eFireInfo & GFX_DST_MIRROR_INFO) {
		g_apiGFXLocal._bDstMirrorH = pFireInfo->GFXSetDstMirror.bMirrorX;
		g_apiGFXLocal._bDstMirrorV = pFireInfo->GFXSetDstMirror.bMirrorY;
	}
	if (pFireInfo->eFireInfo & GFX_ROTATE_INFO)
		g_apiGFXLocal._angle = pFireInfo->GFXSetAngle;

	if (pFireInfo->eFireInfo & GFX_CSC_INFO) {
		MApi_GFX_SetDC_CSC_FMT_U02(pInstance, pFireInfo->GFXSetCSC.mode,
					   pFireInfo->GFXSetCSC.yuv_out_range,
					   pFireInfo->GFXSetCSC.uv_in_range,
					   pFireInfo->GFXSetCSC.srcfmt,
					   pFireInfo->GFXSetCSC.dstfmt);
	}
	if (pFireInfo->eFireInfo & GFX_STR_BLT_SCK_INFO) {
		MApi_GFX_SetStrBltSckType_U02(pInstance, pFireInfo->sttype.type,
					      &(pFireInfo->sttype.color));
	}
	if (pFireInfo->eFireInfo & GFX_NEAREST_INFO)
		g_apiGFXLocal._bNearest = pFireInfo->bNearest;

	if (pFireInfo->eFireInfo & GFX_DITHER_INFO) {
		g_apiGFXLocal.bDither = pFireInfo->bDither;
		MDrv_GE_SetDither(g_apiGFXLocal.g_pGEContext, g_apiGFXLocal.bDither);
	}

	return TRUE;
}

MS_U16 Ioctl_GFX_Init(void *pInstance, void *pArgs)
{
	GFX_INIT_ARGS *pGFXInit = NULL;
	GE_Config cfg;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	pGFXInit = (GFX_INIT_ARGS *) pArgs;

	if (pGFXInit->pGFX_Init == NULL) {
		G2D_ERROR("GFX init FAIL\n");
		return UTOPIA_STATUS_FAIL;
	}
	memset(&cfg, 0, sizeof(GE_Config));
	cfg.bIsCompt = pGFXInit->pGFX_Init->bIsCompt;
	cfg.bIsHK = pGFXInit->pGFX_Init->bIsHK;

	cfg.u32VCmdQSize = pGFXInit->pGFX_Init->u32VCmdQSize;
	cfg.PhyVCmdQAddr = pGFXInit->pGFX_Init->u32VCmdQAddr;

	//escape the many thread modify the global variable
	if (g_apiGFXLocal._bInit == FALSE) {
		MDrv_GE_Init(pInstance, (GE_Config *) &cfg, &g_apiGFXLocal.g_pGEContext);
		g_apiGFXLocal.g_pGEContext->pBufInfo.tlbmode = E_GE_TLB_NONE;
	}
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	CheckSize(pGFXInit->u32Size, sizeof(GFX_Init_Config), 0);
	MDrv_GE_Chip_Proprity_Init(g_apiGFXLocal.g_pGEContext, &g_apiGFXLocal.pGeChipProperty);
	MDrv_GE_SetOnePixelMode(g_apiGFXLocal.g_pGEContext,
				!(g_apiGFXLocal.pGeChipProperty->bFourPixelModeStable));
	g_apiGFXLocal._bInit = TRUE;

	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(GFX_SUCCESS));
	return MapRet(GFX_SUCCESS);
}

MS_U16 Ioctl_GFX_GetInfo(void *pInstance, void *pArgs)
{
	EN_GFX_GET_CONFIG eCmd;
	GFX_GETINFO_ARGS *pGFXGetInfo = NULL;
	GFX_Get_Intensity *pGFXGetIntensity = NULL;
	GFX_Result u32Ret = GFX_FAIL;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	pGFXGetInfo = (GFX_GETINFO_ARGS *) pArgs;
	eCmd = pGFXGetInfo->eGFX_GetConfig;

	switch (eCmd) {

	case E_GFX_GET_INTENSITY:
		CheckSize(pGFXGetInfo->u32Size, sizeof(GFX_Get_Intensity), eCmd);
		pGFXGetIntensity = (GFX_Get_Intensity *) pGFXGetInfo->pGFX_GetInfo;
		u32Ret =
		    (GFX_Result) MDrv_GE_GetIntensity(g_apiGFXLocal.g_pGEContext,
						      pGFXGetIntensity->u32Id,
						      pGFXGetIntensity->pColor);
		break;

	default:
		G2D_ERROR("Error Cmd=%d\n", eCmd);
		break;
	}
	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_S32 Ioctl_GFX_LineDraw(void *pInstance, void *pArgs)
{
	GFX_LINEDRAW_ARGS *pGFXLineDraw = NULL;
	GFX_Set_DrawLineInfo *pGFXLineInfo = NULL;
	GFX_Result u32Ret = GFX_FAIL;
	GE_RgbColor geRgbColor;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	pGFXLineDraw = (GFX_LINEDRAW_ARGS *) pArgs;
	CheckSize(pGFXLineDraw->u32Size, sizeof(GFX_Set_DrawLineInfo), 0);
	pGFXLineInfo = (GFX_Set_DrawLineInfo *) pGFXLineDraw->pLineInfo;

	GFX_SetFireInfo(pInstance, pGFXLineInfo->pFireInfo);

	geRgbColor.a = (g_apiGFXLocal.u32geRgbColor & 0xff000000) >> 24;
	geRgbColor.r = (g_apiGFXLocal.u32geRgbColor & 0x00ff0000) >> 16;
	geRgbColor.g = (g_apiGFXLocal.u32geRgbColor & 0x0000ff00) >> 8;
	geRgbColor.b = (g_apiGFXLocal.u32geRgbColor & 0x000000ff);
	MDrv_GE_SetDFBBldConstColor(g_apiGFXLocal.g_pGEContext, geRgbColor);

	u32Ret = MApi_GFX_DrawLine_U02(pInstance, pGFXLineInfo->pDrawLineInfo);

	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_S32 Ioctl_GFX_RectFill(void *pInstance, void *pArgs)
{
	GFX_RECTFILL_ARGS *pGFXRectFill = NULL;
	GFX_Set_RectFillInfo *pGFXRectInfo = NULL;
	GFX_Result u32Ret = GFX_FAIL;
	GE_RgbColor geRgbColor;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	pGFXRectFill = (GFX_RECTFILL_ARGS *) pArgs;
	CheckSize(pGFXRectFill->u32Size, sizeof(GFX_Set_RectFillInfo), 0);
	pGFXRectInfo = (GFX_Set_RectFillInfo *) pGFXRectFill->pFillBlock;

	GFX_SetFireInfo(pInstance, pGFXRectInfo->pFireInfo);

	geRgbColor.a = (g_apiGFXLocal.u32geRgbColor & 0xff000000) >> 24;
	geRgbColor.r = (g_apiGFXLocal.u32geRgbColor & 0x00ff0000) >> 16;
	geRgbColor.g = (g_apiGFXLocal.u32geRgbColor & 0x0000ff00) >> 8;
	geRgbColor.b = (g_apiGFXLocal.u32geRgbColor & 0x000000ff);
	MDrv_GE_SetDFBBldConstColor(g_apiGFXLocal.g_pGEContext, geRgbColor);

	u32Ret = MApi_GFX_RectFill_U02(pInstance, pGFXRectInfo->pRectFillInfo);

	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_S32 Ioctl_GFX_TriFill(void *pInstance, void *pArgs)
{
	GFX_TRIFILL_ARGS *pGFXTriFill = NULL;
	GFX_Set_TriFillInfo *pGFXTriInfo = NULL;
	GFX_Result u32Ret = GFX_FAIL;
	GE_RgbColor geRgbColor;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	pGFXTriFill = (GFX_TRIFILL_ARGS *) pArgs;
	CheckSize(pGFXTriFill->u32Size, sizeof(GFX_Set_TriFillInfo), 0);
	pGFXTriInfo = (GFX_Set_TriFillInfo *) pGFXTriFill->pFillBlock;

	GFX_SetFireInfo(pInstance, pGFXTriInfo->pFireInfo);

	geRgbColor.a = (g_apiGFXLocal.u32geRgbColor & 0xff000000) >> 24;
	geRgbColor.r = (g_apiGFXLocal.u32geRgbColor & 0x00ff0000) >> 16;
	geRgbColor.g = (g_apiGFXLocal.u32geRgbColor & 0x0000ff00) >> 8;
	geRgbColor.b = (g_apiGFXLocal.u32geRgbColor & 0x000000ff);
	MDrv_GE_SetDFBBldConstColor(g_apiGFXLocal.g_pGEContext, geRgbColor);

	u32Ret = MApi_GFX_TriFill_U02(pInstance, pGFXTriInfo->pTriFillInfo);

	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_S32 Ioctl_GFX_SpanFill(void *pInstance, void *pArgs)
{
	GFX_SPANFILL_ARGS *pGFXSpanFill = NULL;
	GFX_Set_SpanFillInfo *pGFXSpanInfo = NULL;
	GFX_Result u32Ret = GFX_FAIL;
	GE_RgbColor geRgbColor;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	pGFXSpanFill = (GFX_SPANFILL_ARGS *) pArgs;
	CheckSize(pGFXSpanFill->u32Size, sizeof(GFX_Set_SpanFillInfo), 0);
	pGFXSpanInfo = (GFX_Set_SpanFillInfo *) pGFXSpanFill->pFillBlock;

	GFX_SetFireInfo(pInstance, pGFXSpanInfo->pFireInfo);

	geRgbColor.a = (g_apiGFXLocal.u32geRgbColor & 0xff000000) >> 24;
	geRgbColor.r = (g_apiGFXLocal.u32geRgbColor & 0x00ff0000) >> 16;
	geRgbColor.g = (g_apiGFXLocal.u32geRgbColor & 0x0000ff00) >> 8;
	geRgbColor.b = (g_apiGFXLocal.u32geRgbColor & 0x000000ff);
	MDrv_GE_SetDFBBldConstColor(g_apiGFXLocal.g_pGEContext, geRgbColor);

	u32Ret = MApi_GFX_SpanFill_U02(pInstance, pGFXSpanInfo->pSpanFillInfo);

	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_S32 Ioctl_GFX_BitBlt(void *pInstance, void *pArgs)
{
	GFX_BITBLT_ARGS *pGFXBitblt = NULL;
	GFX_BitBltInfo *pGFXBitBltInfo = NULL;
	GFX_Result u32Ret = GFX_FAIL;
	GE_RgbColor geRgbColor;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	pGFXBitblt = (GFX_BITBLT_ARGS *) pArgs;
	CheckSize(pGFXBitblt->u32Size, sizeof(GFX_BitBltInfo), 0);
	pGFXBitBltInfo = (GFX_BitBltInfo *) pGFXBitblt->pGFX_BitBlt;

	GFX_SetFireInfo(pInstance, pGFXBitBltInfo->pFireInfo);

	geRgbColor.a = (g_apiGFXLocal.u32geRgbColor & 0xff000000) >> 24;
	geRgbColor.r = (g_apiGFXLocal.u32geRgbColor & 0x00ff0000) >> 16;
	geRgbColor.g = (g_apiGFXLocal.u32geRgbColor & 0x0000ff00) >> 8;
	geRgbColor.b = (g_apiGFXLocal.u32geRgbColor & 0x000000ff);
	MDrv_GE_SetDFBBldConstColor(g_apiGFXLocal.g_pGEContext, geRgbColor);
	u32Ret =
	    MApi_GFX_BitBlt_U02(pInstance, pGFXBitBltInfo->pDrawRect, pGFXBitBltInfo->u32DrawFlag,
				pGFXBitBltInfo->pScaleInfo);

	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_S32 Ioctl_GFX_SetABL(void *pInstance, void *pArgs)
{
	EN_GFX_SET_ABL eCmd;
	GFX_ABL_ARGS *pGFXABL = NULL;
	GFX_Set_ROP *pGFXSetROP = NULL;
	GFX_Set_Intensity *pGFXSetIntensity = NULL;
	GFX_Result u32Ret = GFX_FAIL;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	pGFXABL = (GFX_ABL_ARGS *) pArgs;
	eCmd = pGFXABL->eGFX_SetABL;

	switch (eCmd) {

	case E_GFX_SET_ROP:
		CheckSize(pGFXABL->u32Size, sizeof(GFX_Set_ROP), eCmd);
		pGFXSetROP = (GFX_Set_ROP *) pGFXABL->pGFX_ABL;
		MApi_GFX_SetROP2_U02(pInstance, pGFXSetROP->bEnable, pGFXSetROP->eRopMode);
		u32Ret = GFX_SUCCESS;
		break;

	case E_GFX_SET_INTENSITY:
		CheckSize(pGFXABL->u32Size, sizeof(GFX_Set_Intensity), eCmd);
		pGFXSetIntensity = (GFX_Set_Intensity *) pGFXABL->pGFX_ABL;
		MApi_GFX_SetIntensity_U02(pInstance, pGFXSetIntensity->u32Id,
					  pGFXSetIntensity->eFmt, pGFXSetIntensity->pColor);
		u32Ret = GFX_SUCCESS;
		break;
	default:
		G2D_ERROR("Error Cmd=%d\n", eCmd);
		u32Ret = GFX_FAIL;
		break;

	}
	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_S32 Ioctl_GFX_SetConfig(void *pInstance, void *pArgs)
{
	EN_GFX_SET_CONFIG eCmd;
	GFX_SETCONFIG_ARGS *pGFXSetConfig = NULL;
	GFX_Set_VQ *pGFXSetVQ = NULL;
	GFX_Set_PaletteOpt *pGFXSetPaletteOpt = NULL;
	MS_U32 *pu32Val = NULL;
	GFX_Result u32Ret = GFX_FAIL;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	pGFXSetConfig = (GFX_SETCONFIG_ARGS *) pArgs;
	eCmd = pGFXSetConfig->eGFX_SetConfig;

	switch (eCmd) {
	case E_GFX_SET_FLUSHQUEUE:
		MDrv_GE_WaitIdle(g_apiGFXLocal.g_pGEContext);
		u32Ret = GFX_SUCCESS;
		break;

	case E_GFX_SET_NEXTTAGID:
		CheckSize(pGFXSetConfig->u32Size, sizeof(MS_U32), eCmd);
		MDrv_GE_SetNextTAGID(g_apiGFXLocal.g_pGEContext,
				     (MS_U16 *) pGFXSetConfig->pGFX_ConfigInfo);
		u32Ret = GFX_SUCCESS;
		break;

	case E_GFX_SET_WAITFORTAGID:
		CheckSize(pGFXSetConfig->u32Size, sizeof(MS_U32), eCmd);
		pu32Val = (MS_U32 *) pGFXSetConfig->pGFX_ConfigInfo;
		MDrv_GE_WaitTAGID(g_apiGFXLocal.g_pGEContext, (MS_U16) *pu32Val);
		u32Ret = GFX_SUCCESS;
		break;

		//VQ switch can't dynamic disablb, cause of cmd losing.
	case E_GFX_SET_VQ:
		CheckSize(pGFXSetConfig->u32Size, sizeof(GFX_Set_VQ), eCmd);
		pGFXSetVQ = (GFX_Set_VQ *) pGFXSetConfig->pGFX_ConfigInfo;
		//psGFXInstPri->pGFXSetConfig->bVQEnable = pGFXSetVQ->bEnable;
		MDrv_GE_EnableVCmdQueue(g_apiGFXLocal.g_pGEContext, pGFXSetVQ->bEnable);
		MApi_GFX_SetVCmdBuffer_U02(pInstance, pGFXSetVQ->u32Addr, pGFXSetVQ->enBufSize);
		//MDrv_GE_SetVCmd_R_Thread(g_apiGFXLocal.g_pGEContext, pGFXSetVQ->u8R_Threshold);
		//MDrv_GE_SetVCmd_W_Thread(g_apiGFXLocal.g_pGEContext, pGFXSetVQ->u8W_Threshold);
		u32Ret = GFX_SUCCESS;
		break;

	case E_GFX_SET_PALETTEOPT:
		CheckSize(pGFXSetConfig->u32Size, sizeof(GFX_Set_PaletteOpt), eCmd);
		pGFXSetPaletteOpt = (GFX_Set_PaletteOpt *) pGFXSetConfig->pGFX_ConfigInfo;
		MApi_GFX_SetPaletteOpt_U02(pInstance, pGFXSetPaletteOpt->pPalArray,
					   pGFXSetPaletteOpt->u32PalStart,
					   pGFXSetPaletteOpt->u32PalEnd);
		u32Ret = GFX_SUCCESS;
		break;

	default:
		G2D_ERROR("Error Cmd=%d\n", eCmd);
		u32Ret = GFX_FAIL;
		break;
	}

	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_S32 Ioctl_GFX_MISC(void *pInstance, void *pArgs)
{
	EN_GFX_MISC_MODE eCmd;
	GFX_MISC_ARGS *pGFXMISCArg = NULL;
	GFX_ClearFrameBuffer *pGFXClearFrame = NULL;
	GFX_SetLinePattern *pGFXSetLinePattern = NULL;
	GFX_SetStrBltSckType *pStrBltSckType = NULL;
	GFX_Result u32Ret = GFX_FAIL;
	MS_U32 *pu32Value = NULL;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);

	pGFXMISCArg = (GFX_MISC_ARGS *) pArgs;
	eCmd = pGFXMISCArg->eGFX_MISCType;

	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);
	switch (eCmd) {
	case E_MISC_CLEAR_FRAME_BY_WORLD:
		CheckSize(pGFXMISCArg->u32Size, sizeof(GFX_ClearFrameBuffer), eCmd);
		pGFXClearFrame = (GFX_ClearFrameBuffer *) pGFXMISCArg->pGFX_Info;
		MApi_GFX_ClearFrameBufferByWord_U02(pInstance, pGFXClearFrame->StrAddr,
						    pGFXClearFrame->length,
						    pGFXClearFrame->ClearValue);
		u32Ret = GFX_SUCCESS;
		break;
	case E_MISC_CLEAR_FRAME:
		CheckSize(pGFXMISCArg->u32Size, sizeof(GFX_ClearFrameBuffer), eCmd);
		pGFXClearFrame = (GFX_ClearFrameBuffer *) pGFXMISCArg->pGFX_Info;
		MApi_GFX_ClearFrameBuffer_U02(pInstance, pGFXClearFrame->StrAddr,
					      pGFXClearFrame->length, pGFXClearFrame->ClearValue);
		u32Ret = GFX_SUCCESS;
		break;
	case E_MISC_LINE_PATTERN_RESET:
		MDrv_GE_ResetLinePattern(g_apiGFXLocal.g_pGEContext);
		u32Ret = GFX_SUCCESS;
		break;
	case E_MISC_LINE_PATTERN:
		CheckSize(pGFXMISCArg->u32Size, sizeof(GFX_SetLinePattern), eCmd);
		pGFXSetLinePattern = (GFX_SetLinePattern *) pGFXMISCArg->pGFX_Info;
		MApi_GFX_Set_Line_Pattern_U02(pInstance, pGFXSetLinePattern->enable,
					      pGFXSetLinePattern->linePattern,
					      pGFXSetLinePattern->repeatFactor);
		u32Ret = GFX_SUCCESS;
		break;
	case E_MISC_BEGINE_DRAW:
		MApi_GFX_BeginDraw_U02(pInstance);
		u32Ret = GFX_SUCCESS;
		break;
	case E_MISC_END_DRAW:
		MApi_GFX_EndDraw_U02(pInstance);
		u32Ret = GFX_SUCCESS;
		break;
	case E_MISC_STR_BLT_SCK_TYPE:
		CheckSize(pGFXMISCArg->u32Size, sizeof(GFX_SetStrBltSckType), eCmd);
		pStrBltSckType = (GFX_SetStrBltSckType *) pGFXMISCArg->pGFX_Info;
		MApi_GFX_SetStrBltSckType_U02(pInstance, pStrBltSckType->type,
					      &(pStrBltSckType->color));
		u32Ret = GFX_SUCCESS;
		break;
	case E_MISC_GET_CRC:
		CheckSize(pGFXMISCArg->u32Size, sizeof(MS_U32), eCmd);
		pu32Value = (MS_U32 *) pGFXMISCArg->pGFX_Info;
		MDrv_GE_GetCRC(g_apiGFXLocal.g_pGEContext, pu32Value);
		u32Ret = GFX_SUCCESS;
		break;

	default:
		G2D_ERROR("Error Cmd=%d\n", eCmd);
		u32Ret = GFX_FAIL;
		break;
	}
	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);

}

MS_S32 Ioctl_GFX_DrawOval(void *pInstance, void *pArgs)
{
	GFX_DRAW_OVAL_ARGS *pGFXOvalDraw = NULL;
	GFX_Set_DrawOvalInfo *pGFXOvalInfo = NULL;
	GFX_Result u32Ret = GFX_FAIL;

	GFX_INSTANT_PRIVATE *psGFXInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void **)&psGFXInstPri);
	_API_GE_ENTRY(g_apiGFXLocal.g_pGEContext);

	pGFXOvalDraw = (GFX_DRAW_OVAL_ARGS *) pArgs;
	CheckSize(pGFXOvalDraw->u32Size, sizeof(GFX_Set_DrawOvalInfo), 0);
	pGFXOvalInfo = (GFX_Set_DrawOvalInfo *) pGFXOvalDraw->psetting;

	GFX_SetFireInfo(pInstance, pGFXOvalInfo->pFireInfo);

	u32Ret = MApi_GFX_DrawOval_U02(pInstance, pGFXOvalInfo->pDrawOvalInfo);

	_API_GE_RETURN(g_apiGFXLocal.g_pGEContext, MapRet(u32Ret));
	return MapRet(u32Ret);
}

MS_U32 GFXStr(MS_U32 u32PowerState, void *pModule)
{
	MS_U32 u32Return = UTOPIA_STATUS_FAIL;

	switch (u32PowerState) {
	case E_POWER_SUSPEND:
		G2D_INFO("Kernel STR Suspend Start!!!!!\n");
		MDrv_GE_STR_SetPowerState((EN_POWER_MODE) u32PowerState, pModule);
		G2D_INFO("Kernel STR Suspend End!!!!!\n");
		u32Return = UTOPIA_STATUS_SUCCESS;
		break;
	case E_POWER_RESUME:
		G2D_INFO("Kernel STR Resume Start!!!!!\n");
		MDrv_GE_STR_SetPowerState((EN_POWER_MODE) u32PowerState, pModule);
		G2D_INFO("Kernel STR Resume End!!!!!\n");
		u32Return = UTOPIA_STATUS_SUCCESS;
		break;
	default:
		break;
	}
	return u32Return;
}

void GFXRegisterToUtopia(FUtopiaOpen ModuleType)
{
	// 1. deal with module
	void *pUtopiaModule = NULL;
	void *psResource = NULL;

	UtopiaModuleCreate(MODULE_GFX, 8, &pUtopiaModule);
	UtopiaModuleRegister(pUtopiaModule);
	UtopiaModuleSetupFunctionPtr(pUtopiaModule, (FUtopiaOpen) GFXOpen, (FUtopiaClose) GFXClose,
				     (FUtopiaIOctl) GFXIoctl);

	// 2. deal with resource

	UtopiaModuleAddResourceStart(pUtopiaModule, E_GE_POOL_ID_INTERNAL_REGISTER);
	UtopiaResourceCreate("ge0", sizeof(GFX_Resource_PRIVATE), &psResource);
	UtopiaResourceRegister(pUtopiaModule, psResource, E_GE_POOL_ID_INTERNAL_REGISTER);
	UtopiaModuleAddResourceEnd(pUtopiaModule, E_GE_POOL_ID_INTERNAL_REGISTER);
}

MS_U32 GFXOpen(void **ppInstance, const void *const pAttribute)
{
	UtopiaInstanceCreate(sizeof(GFX_INSTANT_PRIVATE), ppInstance);
	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 GFXIoctl(void *pInstance, MS_U32 u32Cmd, void *pArgs)
{
	MS_U32 u32Ret = UTOPIA_STATUS_FAIL;

	if (pInstance == NULL) {
		G2D_ERROR("pInstance =NULL\n");
		return UTOPIA_STATUS_FAIL;
	}

	switch (u32Cmd) {
	case MAPI_CMD_GFX_INIT:
		u32Ret = Ioctl_GFX_Init(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_GET_INFO:
		u32Ret = Ioctl_GFX_GetInfo(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_LINEDRAW:
		u32Ret = Ioctl_GFX_LineDraw(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_RECTFILL:
		u32Ret = Ioctl_GFX_RectFill(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_TRIFILL:
		u32Ret = Ioctl_GFX_TriFill(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_SPANFILL:
		u32Ret = Ioctl_GFX_SpanFill(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_BITBLT:
		u32Ret = Ioctl_GFX_BitBlt(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_SET_ABL:
		u32Ret = Ioctl_GFX_SetABL(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_SET_CONFIG:
		u32Ret = Ioctl_GFX_SetConfig(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_MISC:
		u32Ret = Ioctl_GFX_MISC(pInstance, pArgs);
		break;

	case MAPI_CMD_GFX_DRAW_OVAL:
		u32Ret = Ioctl_GFX_DrawOval(pInstance, pArgs);
		break;

	default:
		u32Ret = UTOPIA_STATUS_FAIL;
		G2D_ERROR("error\n");
		break;
	}

	return u32Ret;
}

MS_U32 GFXClose(void *pInstance)
{
	UtopiaInstanceDelete(pInstance);
	return TRUE;
}
