// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//----------------------------------------------------------------------------
// Include Files
//----------------------------------------------------------------------------
#include <../mtk-g2d-utp-wrapper.h>
#include "sti_msos.h"
#include "apiGFX.h"
#include "apiGFX_v2.h"
#include "apiGFX_private.h"

#if defined(MSOS_TYPE_LINUX_KERNEL)
#include <linux/slab.h>
#define free kfree
#define malloc(size) kmalloc((size), GFP_KERNEL)
#endif

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
#ifndef GFX_UTOPIA2K
#define GFX_UTOPIA2K
#endif
#endif
//----------------------------------------------------------------------------
// Compile options
//----------------------------------------------------------------------------
// #define GFX Utopia2.0 capibility
void *pGEInstance;

#ifdef MSOS_TYPE_LINUX
static pthread_mutex_t GFXFireInfoMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
GFX_FireInfo GFXFireInfo;
GFX_Set_VQ GFXSerVQ;
//----------------------------------------------------------------------------
// Local Defines
//----------------------------------------------------------------------------

MS_U32 APIGFX_CHECK_NULL(void *pParameter)
{
	if (pParameter == NULL) {
		G2D_ERROR("\n Input Parameter is NULL, please to check!\n");
		return GFX_FAIL;
	}
	return GFX_SUCCESS;
}

#ifdef CONFIG_GFX_UTOPIA10
MS_U32 APIGFX_CHECK_INIT(void)
{
	return GFX_SUCCESS;
}
#else
MS_U32 APIGFX_CHECK_INIT(void)
{
	if (pGEInstance == NULL) {
		G2D_ERROR("\nshould call MApi_GFX_Init first\n");
		return GFX_INIT_FAIL;
	}
	return GFX_SUCCESS;
}
#endif

//----------------------------------------------------------------------------
// Local Function Prototypes
//----------------------------------------------------------------------------
#define GFXDRIVER_BASE 0UL

#ifndef MSOS_TYPE_OPTEE
static void GFX_FireInfoLock(void)
{

#ifdef MSOS_TYPE_LINUX
	pthread_mutex_lock(&GFXFireInfoMutex);
#endif

}

static void GFX_FireInfoUnLock(void)
{

#ifdef MSOS_TYPE_LINUX
	pthread_mutex_unlock(&GFXFireInfoMutex);
#endif

}
#endif

/******************************************************************************/
///This function will fill a specific 32-bit pattern in a specific buffer.
///
///@param StrAddr \b IN: Start address of the buffer. 4-Byte alignment.
///@param length \b IN: Length of the value pattern to be filled. 4-Byte alignment
///@param ClearValue \b IN: 32-bit pattern to be filled in the specified space.
///@return  GFX_SUCCESS if success.
/******************************************************************************/
GFX_Result MApi_GFX_ClearFrameBufferByWord(MS_PHY StrAddr, MS_U32 length, MS_U32 ClearValue)
{
	GFX_MISC_ARGS GFXInfo;
	GFX_Result enRet = GFX_SUCCESS;
	GFX_ClearFrameBuffer Info;

	memset(&GFXInfo, 0, sizeof(GFX_MISC_ARGS));
	memset(&Info, 0, sizeof(GFX_ClearFrameBuffer));

	Info.StrAddr = StrAddr;
	Info.length = length;
	Info.ClearValue = ClearValue;

	GFXInfo.eGFX_MISCType = E_MISC_CLEAR_FRAME_BY_WORLD;
	GFXInfo.pGFX_Info = (void *)&Info;
	GFXInfo.u32Size = sizeof(GFX_ClearFrameBuffer);
#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_MISC(NULL, (void *)&GFXInfo) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_MISC, (void *)&GFXInfo)
		!= UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif
	return enRet;

}

/******************************************************************************/
///This function will fill a specific 8-bit pattern in a specific buffer.
///
///@param StrAddr \b IN: Start address. Byte alignment.
///@param length \b IN: Length of the value pattern to be filled. Byte alignment
///@param ClearValue \b IN: 8 bit pattern to be filled in the specified space.
///@return  GFX_SUCCESS if success.
/******************************************************************************/
GFX_Result MApi_GFX_ClearFrameBuffer(MS_PHY StrAddr, MS_U32 length, MS_U8 ClearValue)
{
	GFX_MISC_ARGS GFXInfo;
	GFX_Result enRet = GFX_SUCCESS;
	GFX_ClearFrameBuffer Info;

	memset(&GFXInfo, 0, sizeof(GFX_MISC_ARGS));
	memset(&Info, 0, sizeof(GFX_ClearFrameBuffer));

	Info.StrAddr = StrAddr;
	Info.length = length;
	Info.ClearValue = ClearValue;

	GFXInfo.eGFX_MISCType = E_MISC_CLEAR_FRAME;
	GFXInfo.pGFX_Info = (void *)&Info;
	GFXInfo.u32Size = sizeof(GFX_ClearFrameBuffer);
#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_MISC(NULL, (void *)&GFXInfo) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_MISC, (void *)&GFXInfo)
		!= UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif
	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// Initial PE engine
/// @return  None
//-------------------------------------------------------------------------------------------------
void MApi_GFX_Init(GFX_Config *geConfig)
{
	GFX_INIT_ARGS GFXInit;
	GFX_Init_Config GFXInitInfo;

	memset(&GFXInit, 0, sizeof(GFX_INIT_ARGS));
	GFXInitInfo.u32VCmdQSize = geConfig->u32VCmdQSize;
	GFXInitInfo.u32VCmdQAddr = geConfig->u32VCmdQAddr;
	GFXInitInfo.bIsHK = geConfig->bIsHK;
	GFXInitInfo.bIsCompt = geConfig->bIsCompt;

	GFXInit.pGFX_Init = (void *)&GFXInitInfo;
	GFXInit.u32Size = sizeof(GFX_Init_Config);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_Init(NULL, (void *)&GFXInit) != UTOPIA_STATUS_SUCCESS)
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
#else
	if (pGEInstance == NULL) {
		if (UtopiaOpen(MODULE_GFX | GFXDRIVER_BASE, &pGEInstance, 0, NULL)) {
			G2D_ERROR(" Open Fail !!\n");
			return;
		}
	}
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_INIT, (void *)&GFXInit)
		!= UTOPIA_STATUS_SUCCESS)
		G2D_ERROR(" Fail !!\n");
#endif

}
//-------------------------------------------------------------------------------------------------
/// Set GFX Engine dither
/// @param  enable \b IN: true/false
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetDither(MS_BOOL enable)
{
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.bDither = enable;
	GFXFireInfo.eFireInfo |= GFX_DITHER_INFO;
	GFX_FireInfoUnLock();

	return GFX_SUCCESS;
}

GFX_Result MApi_GFX_SetSrcColorKey(MS_BOOL enable,
				   GFX_ColorKeyMode opMode,
				   GFX_Buffer_Format fmt, void *ps_color, void *pe_color)
{
	GFX_Result enRet = GFX_SUCCESS;
	MS_U32 *pS_color = (MS_U32 *) ps_color;
	MS_U32 *pE_color = (MS_U32 *) pe_color;

	if (ps_color == NULL || pe_color == NULL)
		return GFX_FAIL;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetSrcColorKey.bEnable = enable;
	GFXFireInfo.GFXSetSrcColorKey.eOpMode = opMode;
	GFXFireInfo.GFXSetSrcColorKey.eFmt = fmt;
	GFXFireInfo.GFXSetSrcColorKey.S_color = *pS_color;
	GFXFireInfo.GFXSetSrcColorKey.E_color = *pE_color;
	GFXFireInfo.eFireInfo |= GFX_SRC_CLRKEY_INFO;
	GFX_FireInfoUnLock();

	return enRet;

}

GFX_Result MApi_GFX_SetDstColorKey(MS_BOOL enable,
				   GFX_ColorKeyMode opMode,
				   GFX_Buffer_Format fmt, void *ps_color, void *pe_color)
{
	GFX_Result enRet = GFX_SUCCESS;
	MS_U32 *pS_color = (MS_U32 *) ps_color;
	MS_U32 *pE_color = (MS_U32 *) pe_color;

	if (ps_color == NULL || pe_color == NULL)
		return GFX_FAIL;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetDstColorKey.bEnable = enable;
	GFXFireInfo.GFXSetDstColorKey.eOpMode = opMode;
	GFXFireInfo.GFXSetDstColorKey.eFmt = fmt;
	GFXFireInfo.GFXSetDstColorKey.S_color = *pS_color;
	GFXFireInfo.GFXSetDstColorKey.E_color = *pE_color;
	GFXFireInfo.eFireInfo |= GFX_DST_CLRKEY_INFO;
	GFX_FireInfoUnLock();

	return enRet;

}

GFX_Result MApi_GFX_SetIntensity(MS_U32 id, GFX_Buffer_Format fmt, MS_U32 *pColor)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_ABL_ARGS GFXSetABL_ARGS;
	GFX_Set_Intensity GFXSetIntensity;

	if (pColor == NULL)
		return GFX_FAIL;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSetABL_ARGS, 0, sizeof(GFX_ABL_ARGS));
	memset(&GFXSetIntensity, 0, sizeof(GFX_Set_Intensity));

	GFXSetABL_ARGS.eGFX_SetABL = E_GFX_SET_INTENSITY;
	GFXSetABL_ARGS.pGFX_ABL = (void *)&GFXSetIntensity;
	GFXSetABL_ARGS.u32Size = sizeof(GFX_Set_Intensity);

	GFXSetIntensity.u32Id = id;
	GFXSetIntensity.eFmt = fmt;
	GFXSetIntensity.pColor = pColor;

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SetABL(NULL, (void *)&GFXSetABL_ARGS) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SET_ABL, (void *)&GFXSetABL_ARGS) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;

}

//-------------------------------------------------------------------------------------------------
/// Get GFX intensity : total 16 color Palette in GFX for I1/I2/I4 mode.
/// @param idx \b IN: id of intensity
/// @param color \b IN: Pointer of start of intensity point to
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
/// @return GFX_Result_INVALID_INTENSITY_ID - Invalid index (id >= 16)
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_GetIntensity(MS_U32 idx, MS_U32 *color)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_GETINFO_ARGS GFXGetInfo;
	GFX_Get_Intensity GFXGetIntensity;

	if (color == NULL)
		return GFX_FAIL;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXGetInfo, 0, sizeof(GFX_GETINFO_ARGS));
	memset(&GFXGetIntensity, 0, sizeof(GFX_Get_Intensity));

	GFXGetInfo.eGFX_GetConfig = E_GFX_GET_INTENSITY;
	GFXGetInfo.pGFX_GetInfo = (void *)&GFXGetIntensity;
	GFXGetInfo.u32Size = sizeof(GFX_Get_Intensity);

	GFXGetIntensity.u32Id = idx;
	GFXGetIntensity.pColor = color;

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_GetInfo(NULL, (void *)&GFXGetInfo) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_GET_INFO, (void *)&GFXGetInfo) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;

}

//-------------------------------------------------------------------------------------------------
/// Set GFX raster operation
/// @param enable \b IN: true/false
/// @param eRopMode \b IN: raster operation
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetROP2(MS_BOOL enable, GFX_ROP2_Op eRopMode)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_ABL_ARGS GFXSetABL_ARGS;
	GFX_Set_ROP GFXSetROP;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSetABL_ARGS, 0, sizeof(GFX_ABL_ARGS));
	memset(&GFXSetROP, 0, sizeof(GFX_Set_ROP));

	GFXSetABL_ARGS.eGFX_SetABL = E_GFX_SET_ROP;
	GFXSetABL_ARGS.pGFX_ABL = (void *)&GFXSetROP;
	GFXSetABL_ARGS.u32Size = sizeof(GFX_Set_ROP);

	GFXSetROP.bEnable = enable;
	GFXSetROP.eRopMode = eRopMode;

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SetABL(NULL, (void *)&GFXSetABL_ARGS) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SET_ABL, (void *)&GFXSetABL_ARGS) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;

}

//-------------------------------------------------------------------------------------------------
/// GFX draw line
/// @param pline \b IN: pointer to line info
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_DrawLine(GFX_DrawLineInfo *pline)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_LINEDRAW_ARGS GFXDrawLine;
	GFX_Set_DrawLineInfo GFXDrawLineInfo;

	if (pline == NULL)
		return GFX_FAIL;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXDrawLine, 0, sizeof(GFX_LINEDRAW_ARGS));
	memset(&GFXDrawLineInfo, 0, sizeof(GFX_Set_DrawLineInfo));

	GFX_FireInfoLock();
	GFXDrawLineInfo.pFireInfo = &GFXFireInfo;
	GFXDrawLineInfo.pDrawLineInfo = pline;
	GFXDrawLine.pLineInfo = (void *)&GFXDrawLineInfo;
	GFXDrawLine.u32Size = sizeof(GFX_Set_DrawLineInfo);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_LineDraw(NULL, (void *)&GFXDrawLine) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_LINEDRAW, (void *)&GFXDrawLine) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	GFXFireInfo.eFireInfo = GFX_FIREINFO_NONE;
	GFX_FireInfoUnLock();

	return enRet;
}
//-------------------------------------------------------------------------------------------------
/// GFX rectangle fill
/// @param pfillblock \b IN: pointer to block info
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_RectFill(GFX_RectFillInfo *pfillblock)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_RECTFILL_ARGS GFXRectFill;
	GFX_Set_RectFillInfo GFXRectFillInfo;

	if (APIGFX_CHECK_NULL(pfillblock) != GFX_SUCCESS)
		return GFX_FAIL;
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXRectFill, 0, sizeof(GFX_RECTFILL_ARGS));
	memset(&GFXRectFillInfo, 0, sizeof(GFX_Set_RectFillInfo));

	GFX_FireInfoLock();
	GFXRectFillInfo.pFireInfo = &GFXFireInfo;
	GFXRectFillInfo.pRectFillInfo = pfillblock;
	GFXRectFill.pFillBlock = (void *)&GFXRectFillInfo;
	GFXRectFill.u32Size = sizeof(GFX_Set_RectFillInfo);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_RectFill(NULL, (void *)&GFXRectFill) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_RECTFILL, (void *)&GFXRectFill) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	GFXFireInfo.eFireInfo = GFX_FIREINFO_NONE;
	GFX_FireInfoUnLock();

	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// GFX triangle fill
/// @param ptriblock \b IN: pointer to block info
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_TriFill(GFX_TriFillInfo *ptriblock)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_TRIFILL_ARGS GFXTriFill;
	GFX_Set_TriFillInfo GFXTriFillInfo;

	if (APIGFX_CHECK_NULL(ptriblock) != GFX_SUCCESS)
		return GFX_FAIL;
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXTriFill, 0, sizeof(GFX_TRIFILL_ARGS));
	memset(&GFXTriFillInfo, 0, sizeof(GFX_Set_TriFillInfo));

	GFX_FireInfoLock();
	GFXTriFillInfo.pFireInfo = &GFXFireInfo;
	GFXTriFillInfo.pTriFillInfo = ptriblock;
	GFXTriFill.pFillBlock = (void *)&GFXTriFillInfo;
	GFXTriFill.u32Size = sizeof(GFX_Set_TriFillInfo);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_TriFill(NULL, (void *)&GFXTriFill) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_TRIFILL, (void *)&GFXTriFill) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	GFXFireInfo.eFireInfo = GFX_FIREINFO_NONE;
	GFX_FireInfoUnLock();

	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// GFX span fill
/// @param pspanblock \b IN: pointer to block info
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SpanFill(GFX_SpanFillInfo *pspanblock)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_SPANFILL_ARGS GFXSpanFill;
	GFX_Set_SpanFillInfo GFXSpanFillInfo;

	if (APIGFX_CHECK_NULL(pspanblock) != GFX_SUCCESS)
		return GFX_FAIL;
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSpanFill, 0, sizeof(GFX_SPANFILL_ARGS));
	memset(&GFXSpanFillInfo, 0, sizeof(GFX_Set_SpanFillInfo));

	GFX_FireInfoLock();
	GFXSpanFillInfo.pFireInfo = &GFXFireInfo;
	GFXSpanFillInfo.pSpanFillInfo = pspanblock;
	GFXSpanFill.pFillBlock = (void *)&GFXSpanFillInfo;
	GFXSpanFill.u32Size = sizeof(GFX_Set_SpanFillInfo);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SpanFill(NULL, (void *)&GFXSpanFill) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SPANFILL, (void *)&GFXSpanFill) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	GFXFireInfo.eFireInfo = GFX_FIREINFO_NONE;
	GFX_FireInfoUnLock();

	return enRet;
}
//-------------------------------------------------------------------------------------------------
/// Set GFX clipping window
/// @param v0 \b IN: left-top position
/// @param v1 \b IN: right-down position
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetClip(GFX_Point *v0, GFX_Point *v1)
{
	GFX_Result enRet = GFX_SUCCESS;

	if ((v0 == NULL) || (v1 == NULL))
		return GFX_FAIL;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetClip.V0.x = v0->x;
	GFXFireInfo.GFXSetClip.V0.y = v0->y;
	GFXFireInfo.GFXSetClip.V1.x = v1->x;
	GFXFireInfo.GFXSetClip.V1.y = v1->y;
	GFXFireInfo.eFireInfo |= GFX_CLIP_INFO;
	GFX_FireInfoUnLock();

	return enRet;

}

//-------------------------------------------------------------------------------------------------
/// Get GFX clipping window
/// @param v0 \b IN: left-top position
/// @param v1 \b IN: right-down position
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_GetClip(GFX_Point *v0, GFX_Point *v1)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (v0 == NULL || v1 == NULL)
		return GFX_FAIL;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	v0->x = GFXFireInfo.GFXSetClip.V0.x;
	v0->y = GFXFireInfo.GFXSetClip.V0.y;
	v1->x = GFXFireInfo.GFXSetClip.V1.x;
	v1->y = GFXFireInfo.GFXSetClip.V1.y;
	GFX_FireInfoUnLock();

	return enRet;
}



//-------------------------------------------------------------------------------------------------
/// Set GFX alpha source
/// @param eMode \b IN: alpha source come from , this indicate alpha channel output source
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetAlphaSrcFrom(GFX_AlphaSrcFrom eMode)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetABL.eDb_abl = eMode;
	GFXFireInfo.eFireInfo |= GFX_ABL_INFO;
	GFX_FireInfoUnLock();

	return enRet;
}


//-------------------------------------------------------------------------------------------------
/// Set GFX alpha compare OP
/// @param enable \b IN: true: enable alpha compare, false: disable.
/// @param eMode \b IN: alpha source come from MIN/MAX compare between source/dst,
///                     this indicate alpha channel output source
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetAlphaCmp(MS_BOOL enable, GFX_ACmpOp eMode)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetAlphaCmp.enable = enable;
	GFXFireInfo.GFXSetAlphaCmp.eMode = eMode;
	GFXFireInfo.eFireInfo |= GFX_ALPHA_CMP_INFO;
	GFX_FireInfoUnLock();

	return enRet;

}

//-------------------------------------------------------------------------------------------------
/// Begin GFX Engine drawing, this function should be called before all PE drawing function,
/// and it will lock PE engine resource, reset all PE register and static variable.
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_BeginDraw(void)
{
	GFX_MISC_ARGS GFXInfo;
	MS_U32 value = 0;
	GFX_Result enRet = GFX_SUCCESS;

	memset(&GFXInfo, 0, sizeof(GFX_MISC_ARGS));

	GFXInfo.eGFX_MISCType = E_MISC_BEGINE_DRAW;
	GFXInfo.pGFX_Info = (void *)&value;
	GFXInfo.u32Size = sizeof(GFX_MISC_ARGS);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_MISC(NULL, (void *)&GFXInfo) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_MISC, (void *)&GFXInfo)
		!= UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;
}


//-------------------------------------------------------------------------------------------------
/// End GFX engine drawing (pair with MApi_GFX_BeginDraw), this function should be called after
/// all PE drawing function. And it will release PE engine resource.
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_EndDraw(void)
{
	GFX_MISC_ARGS GFXInfo;
	MS_U32 value = 0;
	GFX_Result enRet = GFX_SUCCESS;

	memset(&GFXInfo, 0, sizeof(GFX_MISC_ARGS));

	GFXInfo.eGFX_MISCType = E_MISC_END_DRAW;
	GFXInfo.pGFX_Info = (void *)&value;
	GFXInfo.u32Size = sizeof(GFX_MISC_ARGS);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_MISC(NULL, (void *)&GFXInfo) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_MISC, (void *)&GFXInfo)
		!= UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// Reset GFX line pattern
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_Line_Pattern_Reset(void)
{
	GFX_MISC_ARGS GFXInfo;
	MS_U32 value = 0;
	GFX_Result enRet = GFX_SUCCESS;

	memset(&GFXInfo, 0, sizeof(GFX_MISC_ARGS));

	GFXInfo.eGFX_MISCType = E_MISC_LINE_PATTERN_RESET;
	GFXInfo.pGFX_Info = (void *)&value;
	GFXInfo.u32Size = sizeof(GFX_MISC_ARGS);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_MISC(NULL, (void *)&GFXInfo) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_MISC, (void *)&GFXInfo)
		!= UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;
}


//-------------------------------------------------------------------------------------------------
/// Set GFX line pattern
/// @param enable \b IN: true/false
/// @param linePattern \b IN: p0-0x3F one bit represent draw(1) or not draw(0)
/// @param repeatFactor \b IN: 0 : repeat once, 1 : repeat twice, 2: repeat 3, 3: repeat 4
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_Set_Line_Pattern(MS_BOOL enable, MS_U8 linePattern, MS_U8 repeatFactor)
{
	GFX_MISC_ARGS GFXInfo;
	GFX_Result enRet = GFX_SUCCESS;
	GFX_SetLinePattern Info;

	memset(&GFXInfo, 0, sizeof(GFX_MISC_ARGS));
	memset(&Info, 0, sizeof(GFX_SetLinePattern));

	Info.enable = enable;
	Info.linePattern = linePattern;
	Info.repeatFactor = repeatFactor;

	GFXInfo.eGFX_MISCType = E_MISC_LINE_PATTERN;
	GFXInfo.pGFX_Info = (void *)&Info;
	GFXInfo.u32Size = sizeof(GFX_SetLinePattern);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_MISC(NULL, (void *)&GFXInfo) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_MISC, (void *)&GFXInfo)
		!= UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// Set GFX rotate
/// @param angle \b IN: rotate angle
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
/// @note
/// The rotate process can't perform with italic process.
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetRotate(GFX_RotateAngle angle)
{
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetAngle = angle;
	GFXFireInfo.eFireInfo |= GFX_ROTATE_INFO;
	GFX_FireInfoUnLock();

	return GFX_SUCCESS;
}


//-------------------------------------------------------------------------------------------------
// Description:
// Arguments:   eMode : ABL_FROM_CONST, ABL_FROM_ASRC, ABL_FROM_ADST
//              blendcoef : COEF_ONE,  COEF_CONST,   COEF_ASRC,   COEF_ADST
//                          COEF_ZERO, COEF_1_CONST, COEF_1_ASRC, COEF_1_ADST
//              blendfactor : value : [0,0xff]
// Return:      NONE
//
// Notes:       if any
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// Set PE alpha blending. Dst = A * Src + (1 - A) Dst
/// @param blendcoef       \b IN: alpha source from
/// @param u8ConstantAlpha \b IN: Constant alpha when blendcoef is equal to COEF_CONST
///                               or COEF_1_CONST.
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetAlphaBlending(GFX_BlendCoef blendcoef, MS_U8 u8ConstantAlpha)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetABL.eABLCoef = blendcoef;
	GFXFireInfo.GFXSetABL.u8Alpha_Const = u8ConstantAlpha;
#ifndef MSOS_TYPE_NOS
	GFXFireInfo.GFXSetDFB.sRGBColor.a = u8ConstantAlpha;
	GFXFireInfo.eFireInfo |= GFX_DFB_INFO;
#endif
	GFXFireInfo.eFireInfo |= GFX_ABL_INFO;
	GFX_FireInfoUnLock();

	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// Enable GFX alpha blending
/// @param enable \b IN: true/false
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_EnableAlphaBlending(MS_BOOL enable)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetABL.bEnable = enable;
	GFXFireInfo.eFireInfo |= GFX_ABL_INFO;
	GFX_FireInfoUnLock();

	return enRet;

}

//-------------------------------------------------------------------------------------------------
/// Enable GFX DFB blending
/// @param enable \b IN: true/false
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_EnableDFBBlending(MS_BOOL enable)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetDFB.bEnable = enable;
	GFXFireInfo.eFireInfo |= GFX_DFB_INFO;
	GFX_FireInfoUnLock();
	return enRet;
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
GFX_Result MApi_GFX_SetDFBBldFlags(MS_U16 u16DFBBldFlags)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetDFB.u16DFBBldFlags = u16DFBBldFlags;
	GFXFireInfo.eFireInfo |= GFX_DFB_INFO;
	GFX_FireInfoUnLock();

	return enRet;
}


//-------------------------------------------------------------------------------------------------
/// Set GFX DFB blending Functions/Operations.
/// @param gfxSrcBldOP \b IN: source blending op
/// @param gfxDstBldOP \b IN: dst blending op
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetDFBBldOP(GFX_DFBBldOP gfxSrcBldOP, GFX_DFBBldOP gfxDstBldOP)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetDFB.eSrcBldOP = gfxSrcBldOP;
	GFXFireInfo.GFXSetDFB.eDstBldOP = gfxDstBldOP;
	GFXFireInfo.eFireInfo |= GFX_DFB_INFO;
	GFX_FireInfoUnLock();

	return enRet;
}


//-------------------------------------------------------------------------------------------------
/// Set GFX DFB blending const color.
/// @param u32ConstColor \b IN: DFB Blending constant color
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetDFBBldConstColor(GFX_RgbColor gfxRgbColor)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	memcpy(&(GFXFireInfo.GFXSetDFB.sRGBColor), &gfxRgbColor, sizeof(GFX_RgbColor));
#ifndef MSOS_TYPE_NOS
	GFXFireInfo.GFXSetABL.u8Alpha_Const = gfxRgbColor.a;
	GFXFireInfo.eFireInfo |= GFX_ABL_INFO;
#endif
	GFXFireInfo.eFireInfo |= GFX_DFB_INFO;
	GFX_FireInfoUnLock();

	return enRet;

}

//-------------------------------------------------------------------------------------------------
/// Enable GFX mirror
/// @param isMirrorX \b IN: true/false
/// @param isMirrorY \b IN: true/false
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
/// @note
/// The mirror process can't perform on the source format is GFX_FMT_I1, GFX_FMT_I2 or GFX_FMT_I4.
/// The mirror process can't perform with italic process.
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetMirror(MS_BOOL isMirrorX, MS_BOOL isMirrorY)
{
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetMirror.bMirrorX = isMirrorX;
	GFXFireInfo.GFXSetMirror.bMirrorY = isMirrorY;
	GFXFireInfo.eFireInfo |= GFX_SRC_MIRROR_INFO;
	GFX_FireInfoUnLock();

	return GFX_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Enable GFX destination mirror
/// @param isMirrorX \b IN: true/false
/// @param isMirrorY \b IN: true/false
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
/// @note
/// The mirror process can't perform on the source format is GFX_FMT_I1, GFX_FMT_I2 or GFX_FMT_I4.
/// The mirror process can't perform with italic process.
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetDstMirror(MS_BOOL isMirrorX, MS_BOOL isMirrorY)
{
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetDstMirror.bMirrorX = isMirrorX;
	GFXFireInfo.GFXSetDstMirror.bMirrorY = isMirrorY;
	GFXFireInfo.eFireInfo |= GFX_DST_MIRROR_INFO;
	GFX_FireInfoUnLock();

	return GFX_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Enable GFX NearestMode
/// @param enable \b IN: true/false
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetNearestMode(MS_BOOL enable)
{
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.bNearest = enable;
	GFXFireInfo.eFireInfo |= GFX_NEAREST_INFO;
	GFX_FireInfoUnLock();

	return GFX_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Set GFX source buffer info
/// @param bufInfo \b IN: buffer handle
/// @param offsetofByte \b IN: start offset (should be 128 bit aligned)
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
/// @return GFX_Result_NON_ALIGN_PITCH - The pitch is not 16 bytes alignment
/// @return GFX_Result_NON_ALIGN_ADDRESS - The address is not 16 bytes alignment
/// @note
/// The buffer start address must be 128 bits alignment.
/// In GFX_FMT_I1, GFX_FMT_I2 and GFX_FMT_I4 format, the pitch must be 8 bits alignment.
/// In other format, the pitch must be 128 bits alignment.
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetSrcBufferInfo(PGFX_BufferInfo bufInfo, MS_U32 offsetofByte)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_NULL(bufInfo) != GFX_SUCCESS)
		return GFX_FAIL;
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	//Utopia2 not support this function. Only stroed in wrapper.
	GFX_FireInfoLock();
	memcpy(&(GFXFireInfo.SrcbufInfo), bufInfo, sizeof(GFX_BufferInfo));
	GFXFireInfo.u32SrcOffsetofByte = offsetofByte;
	GFXFireInfo.eFireInfo |= GFX_SRC_INFO;
	GFX_FireInfoUnLock();

	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// Set GFX destination buffer info
/// @param bufInfo \b IN: buffer handle
/// @param offsetofByte \b IN: start offset (should be 128 bit aligned)
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
/// @return GFX_Result_NON_ALIGN_PITCH - The pitch is not 16 bytes alignment
/// @return GFX_Result_NON_ALIGN_ADDRESS - The address is not 16 bytes alignment
/// @note
/// The buffer start address and pitch smust be 128 bits alignment.
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetDstBufferInfo(PGFX_BufferInfo bufInfo, MS_U32 offsetofByte)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_NULL(bufInfo) != GFX_SUCCESS)
		return GFX_FAIL;
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	//Utopia2 not support this function. Only stroed in wrapper.
	GFX_FireInfoLock();
	memcpy(&(GFXFireInfo.DstbufInfo), bufInfo, sizeof(GFX_BufferInfo));
	GFXFireInfo.u32DstOffsetofByte = offsetofByte;
	GFXFireInfo.eFireInfo |= GFX_DST_INFO;
	GFX_FireInfoUnLock();

	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// Get GFX SRC/DST buffer info
/// @param srcbufInfo \b IN: Pointer of src buffer info.
/// @param dstbufInfo \b IN: Pointer of dst buffer info.
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
/// @note
/// none
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_GetBufferInfo(PGFX_BufferInfo srcbufInfo, PGFX_BufferInfo dstbufInfo)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_NULL(srcbufInfo) != GFX_SUCCESS)
		return GFX_FAIL;
	if (APIGFX_CHECK_NULL(dstbufInfo) != GFX_SUCCESS)
		return GFX_FAIL;
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	memcpy(srcbufInfo, &(GFXFireInfo.SrcbufInfo), sizeof(GFX_BufferInfo));
	memcpy(dstbufInfo, &(GFXFireInfo.DstbufInfo), sizeof(GFX_BufferInfo));
	GFX_FireInfoUnLock();

	return enRet;

}

//-------------------------------------------------------------------------------------------------
/// Set GFX Bit blt
/// @param drawbuf \b IN: pointer to drawbuf info
/// @param drawflag \b IN: draw flag \n
///                  GFXDRAW_FLAG_DEFAULT \n
///                  GFXDRAW_FLAG_SCALE \n
///                  GFXDRAW_FLAG_DUPLICAPE \n
///                  GFXDRAW_FLAG_TRAPEZOID \n
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_BitBlt(GFX_DrawRect *drawbuf, MS_U32 drawflag)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_ScaleInfo ScaleInfo;
	GFX_BITBLT_ARGS GFXBitBlt;
	GFX_BitBltInfo GFXBitBltInfo;

	if (APIGFX_CHECK_NULL(drawbuf) != GFX_SUCCESS)
		return GFX_FAIL;
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXBitBlt, 0, sizeof(GFX_BITBLT_ARGS));
	memset(&GFXBitBltInfo, 0, sizeof(GFX_BitBltInfo));
	memset(&ScaleInfo, 0, sizeof(GFX_ScaleInfo));

	GFX_FireInfoLock();
	GFXBitBlt.pGFX_BitBlt = (void *)&GFXBitBltInfo;
	GFXBitBlt.u32Size = sizeof(GFX_BitBltInfo);
	GFXBitBltInfo.pFireInfo = &GFXFireInfo;
	GFXBitBltInfo.pDrawRect = drawbuf;
	GFXBitBltInfo.u32DrawFlag = drawflag;
	GFXBitBltInfo.pScaleInfo = &ScaleInfo;
#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_BitBlt(NULL, (void *)&GFXBitBlt) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_BITBLT, (void *)&GFXBitBlt) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	GFXFireInfo.eFireInfo = GFX_FIREINFO_NONE;
	GFX_FireInfoUnLock();

	return enRet;
}

GFX_Result MApi_GFX_SetPaletteOpt(GFX_PaletteEntry *pPalArray, MS_U16 u32PalStart,
				  MS_U16 u32PalEnd)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_SETCONFIG_ARGS GFXSetConfig;
	GFX_Set_PaletteOpt GFXSetPaletteOpt;

	if (pPalArray == NULL) {
		G2D_ERROR("pPalArray =NULL\n");
		return GFX_FAIL;
	}
	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSetConfig, 0, sizeof(GFX_SETCONFIG_ARGS));
	memset(&GFXSetPaletteOpt, 0, sizeof(GFX_Set_PaletteOpt));

	GFXSetPaletteOpt.pPalArray = pPalArray;
	GFXSetPaletteOpt.u32PalStart = u32PalStart;
	GFXSetPaletteOpt.u32PalEnd = u32PalEnd;

	GFXSetConfig.eGFX_SetConfig = E_GFX_SET_PALETTEOPT;
	GFXSetConfig.pGFX_ConfigInfo = (void *)&GFXSetPaletteOpt;
	GFXSetConfig.u32Size = sizeof(GFX_Set_PaletteOpt);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SetConfig(NULL, (void *)&GFXSetConfig) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SET_CONFIG, (void *)&GFXSetConfig) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;
}
//-------------------------------------------------------------------------------------------------
/// Set GFX CSC format
/// YUV/RGB conversion will be performed according to the spec specified in this function.
/// @param mode \b IN: YUV mode: PC or 0~255
/// @param yuv_out_range \b IN: output YUV mode: PC or 0~255
/// @param uv_in_range \b IN: input YUV mode: 0~255 or -126~127
/// @param srcfmt \b IN: YUV packing format for source
/// @param dstfmt \b IN: YUV packing format for destination
/// @return GFX_SUCCESS - Success
/// @return GFX_FAIL - Failure
//-------------------------------------------------------------------------------------------------
GFX_Result MApi_GFX_SetDC_CSC_FMT(GFX_YUV_Rgb2Yuv mode, GFX_YUV_OutRange yuv_out_range,
				  GFX_YUV_InRange uv_in_range, GFX_YUV_422 srcfmt,
				  GFX_YUV_422 dstfmt)
{
	GFX_Result enRet = GFX_SUCCESS;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	GFX_FireInfoLock();
	GFXFireInfo.GFXSetCSC.mode = mode;
	GFXFireInfo.GFXSetCSC.yuv_out_range = yuv_out_range;
	GFXFireInfo.GFXSetCSC.uv_in_range = uv_in_range;
	GFXFireInfo.GFXSetCSC.srcfmt = srcfmt;
	GFXFireInfo.GFXSetCSC.dstfmt = dstfmt;
	GFXFireInfo.eFireInfo |= GFX_CSC_INFO;
	GFX_FireInfoUnLock();

	return enRet;
}

/******************************************************************************/
///Wait for TagID.
///@param tagID \b IN: tag to wait
///@par Function Actions:
/******************************************************************************/
GFX_Result MApi_GFX_WaitForTAGID(MS_U16 tagID)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_SETCONFIG_ARGS GFXSetConfig;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSetConfig, 0, sizeof(GFX_SETCONFIG_ARGS));

	GFXSetConfig.eGFX_SetConfig = E_GFX_SET_WAITFORTAGID;
	GFXSetConfig.pGFX_ConfigInfo = (void *)&tagID;
	GFXSetConfig.u32Size = sizeof(MS_U32);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SetConfig(NULL, (void *)&GFXSetConfig) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SET_CONFIG, (void *)&GFXSetConfig) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;
}

/******************************************************************************/
///Set next TagID Auto to HW.
///@par The Tage ID which has been set to HW
/******************************************************************************/
MS_U16 MApi_GFX_SetNextTAGID(void)
{
	MS_U16 tagID = 0xFF;
	GFX_SETCONFIG_ARGS GFXSetConfig;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSetConfig, 0, sizeof(GFX_SETCONFIG_ARGS));

	GFXSetConfig.eGFX_SetConfig = E_GFX_SET_NEXTTAGID;
	GFXSetConfig.pGFX_ConfigInfo = (void *)&tagID;
	GFXSetConfig.u32Size = sizeof(MS_U32);
#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SetConfig(NULL, (void *)&GFXSetConfig) != UTOPIA_STATUS_SUCCESS)
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SET_CONFIG, (void *)&GFXSetConfig) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
	}
#endif
	return tagID;
}

/******************************************************************************/
///Enable GFX Virtual Command Queue
///@param blEnable \b IN: true: Enable, false: Disable
///@par Function Actions:
///@return GFX_SUCCESS - Success
///@return GFX_FAIL - Failure
/******************************************************************************/
GFX_Result MApi_GFX_EnableVCmdQueue(MS_U16 blEnable)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_SETCONFIG_ARGS GFXSetConfig;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSetConfig, 0, sizeof(GFX_SETCONFIG_ARGS));

	GFXSerVQ.bEnable = blEnable;

	GFXSetConfig.eGFX_SetConfig = E_GFX_SET_VQ;
	GFXSetConfig.pGFX_ConfigInfo = (void *)&GFXSerVQ;
	GFXSetConfig.u32Size = sizeof(GFX_Set_VQ);
#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SetConfig(NULL, (void *)&GFXSetConfig) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SET_CONFIG, (void *)&GFXSetConfig) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif
	return enRet;
}

/******************************************************************************/
///Configure GFX Virtual Command Queue buffer spec
///@param u32Addr \b IN: base address for VCMQ buffer
///@param enBufSize \b IN: buffer size of VCMQ buffer
///@par Function Actions:
///@return GFX_SUCCESS - Success
///@return GFX_FAIL - Failure
/******************************************************************************/
GFX_Result MApi_GFX_SetVCmdBuffer(MS_PHY PhyAddr, GFX_VcmqBufSize enBufSize)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_SETCONFIG_ARGS GFXSetConfig;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSetConfig, 0, sizeof(GFX_SETCONFIG_ARGS));

	GFXSetConfig.eGFX_SetConfig = E_GFX_SET_VQ;
	GFXSetConfig.pGFX_ConfigInfo = (void *)&GFXSerVQ;
	GFXSetConfig.u32Size = sizeof(GFX_Set_VQ);

	GFXSerVQ.u32Addr = PhyAddr;
	GFXSerVQ.enBufSize = enBufSize;
#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SetConfig(NULL, (void *)&GFXSetConfig) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SET_CONFIG, (void *)&GFXSetConfig) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif
	return enRet;

}

/******************************************************************************/
///Explicitly wait for GFX queue empty
///@return GFX_SUCCESS - Success
///@return GFX_FAIL - Failure
/******************************************************************************/
GFX_Result MApi_GFX_FlushQueue(void)
{
	GFX_Result enRet = GFX_SUCCESS;
	MS_U32 value = 0;
	GFX_SETCONFIG_ARGS GFXSetConfig;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXSetConfig, 0, sizeof(GFX_SETCONFIG_ARGS));

	GFXSetConfig.eGFX_SetConfig = E_GFX_SET_FLUSHQUEUE;
	GFXSetConfig.u32Size = sizeof(0);
	GFXSetConfig.pGFX_ConfigInfo = &value;
#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_SetConfig(NULL, (void *)&GFXSetConfig) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_SET_CONFIG, (void *)&GFXSetConfig) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;
}

/******************************************************************************/
///Configure the Color Key edge refine function.
///@param type \b IN: type of refine.
///@param color \b IN: when type is GFX_REPLACE_KEY_2_CUS, color of the customized color.
///@par Function Actions:
///@return GFX_SUCCESS - Success
///@return GFX_FAIL - Failure
/******************************************************************************/
GFX_Result MApi_GFX_SetStrBltSckType(GFX_StretchCKType type, GFX_RgbColor *color)
{

	GFX_FireInfoLock();
	GFXFireInfo.sttype.type = type;
	GFXFireInfo.sttype.color = *color;
	GFXFireInfo.eFireInfo |= GFX_STR_BLT_SCK_INFO;
	GFX_FireInfoUnLock();

	return GFX_SUCCESS;
}

/******************************************************************************/
///Draw Oval. Oval is not directly supported by HW. Software implemented by DrawLine.
///@param pOval \b IN:  Oval info
///@par Function Actions:
///@return GFX_SUCCESS - Success
///@return GFX_FAIL - Failure
/******************************************************************************/
GFX_Result MApi_GFX_DrawOval(GFX_OvalFillInfo *pOval)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_DRAW_OVAL_ARGS GFXDrawOval;
	GFX_Set_DrawOvalInfo GFXDrawOvalInfo;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;

	memset(&GFXDrawOval, 0, sizeof(GFX_DRAW_OVAL_ARGS));
	memset(&GFXDrawOvalInfo, 0, sizeof(GFX_Set_DrawOvalInfo));

	GFX_FireInfoLock();
	GFXDrawOvalInfo.pFireInfo = &GFXFireInfo;
	GFXDrawOvalInfo.pDrawOvalInfo = pOval;
	GFXDrawOval.psetting = (void *)&GFXDrawOvalInfo;
	GFXDrawOval.u32Size = sizeof(GFX_Set_DrawOvalInfo);
#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_DrawOval(NULL, (void *)&GFXDrawOval) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_DRAW_OVAL, (void *)&GFXDrawOval) !=
	    UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif
	GFXFireInfo.eFireInfo = GFX_FIREINFO_NONE;
	GFX_FireInfoUnLock();

	return enRet;
}

//-------------------------------------------------------------------------------------------------
/// GE Exit
/// @param  void                \b IN: none
//-------------------------------------------------------------------------------------------------
void MApi_GE_Exit(void)
{
	GFX_MISC_ARGS GFXInfo;
	MS_U32 value = 0;

	memset(&GFXInfo, 0, sizeof(GFX_MISC_ARGS));

	GFXInfo.eGFX_MISCType = E_MISC_EXIT;
	GFXInfo.pGFX_Info = (void *)&value;
	GFXInfo.u32Size = sizeof(GFX_MISC_ARGS);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_MISC(NULL, (void *)&GFXInfo) != UTOPIA_STATUS_SUCCESS)
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_MISC, (void *)&GFXInfo)
		!= UTOPIA_STATUS_SUCCESS)
		G2D_ERROR(" Fail !!\n");
#endif

}

/******************************************************************************/
///GFX get CRC value
/******************************************************************************/
GFX_Result _MApi_GE_GetCRC(MS_U32 *pu32CRCvalue)
{
	GFX_Result enRet = GFX_SUCCESS;
	GFX_MISC_ARGS GFXInfo;

	if (APIGFX_CHECK_INIT() != GFX_SUCCESS)
		return GFX_FAIL;
	memset(&GFXInfo, 0, sizeof(GFX_MISC_ARGS));

	GFXInfo.eGFX_MISCType = E_MISC_GET_CRC;
	GFXInfo.pGFX_Info = (void *)pu32CRCvalue;
	GFXInfo.u32Size = sizeof(MS_U32);

#ifdef CONFIG_GFX_UTOPIA10
	if (Ioctl_GFX_MISC(NULL, (void *)&GFXInfo) != UTOPIA_STATUS_SUCCESS) {
		GFX_ERR("%s fail [LINE:%d]\n", __func__, __LINE__);
		enRet = GFX_FAIL;
	}
#else
	if (UtopiaIoctl(pGEInstance, MAPI_CMD_GFX_MISC, (void *)&GFXInfo)
		!= UTOPIA_STATUS_SUCCESS) {
		G2D_ERROR(" Fail !!\n");
		enRet = GFX_FAIL;
	}
#endif

	return enRet;
}

