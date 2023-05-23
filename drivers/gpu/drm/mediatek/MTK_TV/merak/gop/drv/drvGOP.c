// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#ifndef MSOS_TYPE_LINUX_KERNEL
#include <string.h>
#endif
#include <linux/of.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/bits.h>
#include "mtk_tv_drm_log.h"
#include "mdrv_gop.h"
#include "common/hwreg_common_riu.h"
#include "drv_scriptmgt.h"
#include "drvGOP.h"
#include "drvGFLIP.h"
#include "drvGOP_priv.h"
#include "pixelmonitor.h"

#if defined(MSOS_TYPE_LINUX_KERNEL)
#include <linux/slab.h>
#define free kfree
#define malloc(size) kmalloc((size), GFP_KERNEL)
#endif

//-------------------------------------------------------------------------------------------------
//  Local Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define ERR_FB_ID_OUT_OF_RANGE          0x0300UL
#define ERR_FB_ID_NOT_ALLOCATED         0x0301UL
#define ERR_FB_ID_ALREADY_ALLOCATED     0x0302UL
#define ERR_FB_OUT_OF_MEMORY            0x0303UL
#define ERR_FB_OUT_OF_ENTRY             0x0304UL

#define MAX_CSC_GAIN (2047UL)

#define MTK_DRM_GOP_COMPATIBLE "mtk-drm-gop-drvconfig"
#define MTK_DRM_GOP_CAPS_GWINNUM     "GOP_TOTAL_GWIN_NUMBER"
#define MTK_DRM_GOP_CAPS_BEF3DLUT_PD     "GOP_BEFORE_3DLUT_PD"
#define MTK_DRM_GOP_CAPS_BEFPQGAMMA_PD     "GOP_BEFORE_PQGAMMA_PD"
#define MTK_DRM_GOP_CAPS_AFTPQGAMMA_PD     "GOP_AFTER_PQGAMMA_PD"
#define MTK_DRM_GOP_CAPS_OPMUX_DB     "GOP_SUPPORT_OPMUX_DB"
#define MTK_DRM_GOP_CAPS_PD     "GOP_PD"
#define MTK_DRM_GOP_CAPS_NONVSCALE_PD     "GOP_NONVSCALE_PD"
#define MTK_DRM_GOP_CAPS_MUX_DELTA     "GOP_MUX_DELTA"
#define MTK_DRM_GOP_CAPS_MUX_OFFSET     "GOP_MUX_OFFSET"
#define MTK_DRM_GOP_CAPS_LAYER2MUX     "GOP_MAPLAYER_TO_MUX"
#define MTK_DRM_GOP_CAPS_BNKFORCEWRITE     "GOP_SUPPORT_BNKFORCEWRITE"
#define MTK_DRM_GOP_CAPS_PIXELMODE     "GOP_SUPPORT_PIXELMODE"
#define MTK_DRM_GOP_CAPS_2PTO1P     "GOP_SUPPORT_2PTO1P"
#define MTK_DRM_GOP_CAPS_AUTOADJUST_HMIRROR     "GOP_AUTO_ADJUST_HMIRROR"

#define MTK_DRM_GOP_CAPABILITY_TAG     "capability"
#define MTK_DRM_GOP_IPVERSION_TAG     "ip-version"
#define MTK_DRM_GOP_REGBASE_TAG     "reg"

#define MTK_DRM_MODEL MTK_DRM_MODEL_GOP
//=============================================================
// Define return values of check align
#define CHECKALIGN_SUCCESS              1UL
#define CHECKALIGN_PARA_FAIL            3UL

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
GOP_CHIP_PROPERTY g_GopChipPro;
MS_U32 u32ChipVer;
//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
#ifdef INSTANT_PRIVATE
#define g_gopDrvCtxLocal  psGOPInstPri->g_gopDrvCtxLocal
#else
GOP_CTX_DRV_LOCAL g_gopDrvCtxLocal;
#endif

#ifdef STI_PLATFORM_BRING_UP
GOP_CTX_DRV_SHARED g_gopDrvCtxShared;
#else
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
#else
GOP_CTX_DRV_SHARED g_gopDrvCtxShared;
#endif
#endif

MS_U16 gGopLimitY2RFullBT709[10] = {
	0x003D, 0x0731, 0x04AC,
	0x0000, 0x1DDD, 0x04AC,
	0x1F25, 0x0000, 0x04AC, 0x0879
};

MS_U16 gGopFullR2YLimitBT709[10] = {
	0xE030, 0x01C0, 0x1E69,
	0x1FD7, 0x00BA, 0x0273,
	0x003F, 0x1F99, 0x1EA6, 0x01C0
};

MS_U16 gGopIdentity[10] = {
	0x000D, 0x0400, 0x0000,
	0x0000, 0x0000, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0400
};

MS_U16 gu32GopCapsAFBC[2] = {GOP_CAPS_OFFSET_GOP0_AFBC,
	GOP_CAPS_OFFSET_GOP_AFBC_COMMON};
MS_U16 gu32GopCapsHDup[2] = {GOP_CAPS_OFFSET_GOP0_HDUPLICATE,
	GOP_CAPS_OFFSET_GOP_HDUPLICATE_COMMON};
MS_U16 gu32GopCapsH2Tap[2] = {GOP_CAPS_OFFSET_GOP0_H2TAP,
	GOP_CAPS_OFFSET_GOP_H2TAP_COMMON};
MS_U16 gu32GopCapsH4Tap[2] = {GOP_CAPS_OFFSET_GOP0_H4TAP,
	GOP_CAPS_OFFSET_GOP_H4TAP_COMMON};
MS_U16 gu32GopCapsV4Tap[2] = {GOP_CAPS_OFFSET_GOP0_V4TAP,
	GOP_CAPS_OFFSET_GOP_V4TAP_COMMON};
MS_U16 gu32GopCapsVBilinear[2] = {GOP_CAPS_OFFSET_GOP0_VBILINEAR,
	GOP_CAPS_OFFSET_GOP_VBILINEAR_COMMON};
MS_U32 gu32GopCapsVScaleModeMsk[2] = {GOP_CAPS_OFFSET_GOP0_VSCALE_MODE_MSK,
	GOP_CAPS_OFFSET_GOP_VSCALE_MODE_MSK_COMMON};
//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------
void GOP_DTS_INFO(const char *name, uint32_t ret)
{
	if (ret != 0)
		pr_err("[%s, %d]: read DTS table failed, name = %s \r\n",
		       __func__, __LINE__, name);
}

void Check_GOP_CfdPQ_ML_Ret(const char *name,
	enum EN_PQAPI_RESULT_CODES enRet)
{
	if (enRet != E_PQAPI_RC_SUCCESS)
		DRM_ERROR("[GOP][%s, %d]: %s failed!\r\n",
		       __func__, __LINE__, name);
}

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
MS_U32 DTBArray2U16(struct device_node *np, const char *name, MS_U16 *ret_array,
		    int ret_array_length)
{
	int ret, i;
	__u32 *u32TmpArray = NULL;

	u32TmpArray = kmalloc_array(ret_array_length, sizeof(__u32), GFP_KERNEL);
	ret = of_property_read_u32_array(np, name, u32TmpArray, ret_array_length);
	if (ret != 0x0) {
		kfree(u32TmpArray);
		return ret;
	}
	for (i = 0; i < ret_array_length; i++)
		*(ret_array + i) = (MS_U16) u32TmpArray[i];

	kfree(u32TmpArray);
	return 0;
}

MS_U32 DTBArray(struct device_node *np, const char *name, MS_U32 *ret_array, int ret_array_length)
{
	int ret, i;
	__u32 *u32TmpArray = NULL;

	u32TmpArray = kmalloc_array(ret_array_length, sizeof(__u32), GFP_KERNEL);
	ret = of_property_read_u32_array(np, name, u32TmpArray, ret_array_length);
	if (ret != 0x0) {
		kfree(u32TmpArray);
		return ret;
	}
	for (i = 0; i < ret_array_length; i++)
		*(ret_array + i) = u32TmpArray[i];
	kfree(u32TmpArray);
	return 0;
}

MS_U32 readChipCaps_DTB(void)
{
	void __iomem *RIUBase1, *RIUBase2;
	struct device_node *np;
	char cstr[100] = "";
	const char *name;
	MS_U32 u32GOPNum = 0, u32Tmp = 0, ret = 0, i = 0;
	MS_U32 *pu32array;
	MS_U16 *pu16array;

	memset(&g_GopChipPro, 0, sizeof(GOP_CHIP_PROPERTY));
	snprintf(cstr, sizeof(cstr), "mediatek,mtk-dtv-gop%d", 0);
	np = of_find_compatible_node(NULL, NULL, cstr);
	if (np != NULL) {
		name = MTK_DRM_GOP_IPVERSION_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		u32ChipVer = u32Tmp;
		KDrv_HW_GOP_SetIPVersion(u32ChipVer);

		if (((u32Tmp & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
			GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_3M_SERIES_MAJOR) {
			name = MTK_DRM_GOP_REGBASE_TAG;
			pu32array = kmalloc(sizeof(MS_U32) * 2, GFP_KERNEL);
			ret = DTBArray(np, name, pu32array, 2);
			RIUBase1 = ioremap(pu32array[0], pu32array[1]);
			g_GopChipPro.VAdr[0] = (MS_VIRT *)RIUBase1;
			g_GopChipPro.PAdr[0] = (pu32array[0] - 0x1f000000)/2;
			g_GopChipPro.u32RegSize[0] = pu32array[1];
			kfree(pu32array);
		} else if (((u32Tmp & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
			GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_M6_SERIES_MAJOR) {
			name = MTK_DRM_GOP_REGBASE_TAG;
			pu32array = kmalloc(sizeof(MS_U32) * 8, GFP_KERNEL);
			ret = DTBArray(np, name, pu32array, 8);
			RIUBase1 = ioremap(pu32array[1], pu32array[3]);
			RIUBase2 = ioremap(pu32array[5], pu32array[7]);
			g_GopChipPro.VAdr[0] = (MS_VIRT *)RIUBase1;
			g_GopChipPro.VAdr[1] = (MS_VIRT *)RIUBase2;
			g_GopChipPro.PAdr[0] = (pu32array[1] - 0x1c000000);
			g_GopChipPro.PAdr[1] = (pu32array[5] - 0x1c000000);
			g_GopChipPro.u32RegSize[0] = pu32array[3];
			g_GopChipPro.u32RegSize[1] = pu32array[7];
			kfree(pu32array);
		}

		name = MTK_DRM_GOP_CAPABILITY_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		u32GOPNum = (u32Tmp & GOP_CAPS_OFFSET_GOPNUM_MSK);
		g_GopChipPro.u32GOPNum = u32GOPNum;
		g_GopChipPro.bSupportVOPPathSel =
		    (MS_BOOL) ((u32Tmp & BIT(GOP_CAPS_OFFSET_VOP_PATHSEL)) >>
			       GOP_CAPS_OFFSET_VOP_PATHSEL);
		g_GopChipPro.bSupportCSCTuning =
		    (MS_BOOL) ((u32Tmp & BIT(GOP_CAPS_OFFSET_GOP0_CSC)) >>
			       GOP_CAPS_OFFSET_GOP0_CSC);
		g_GopChipPro.bUse3x3MartixCSC =
		    (MS_BOOL) ((u32Tmp & BIT(GOP_CAPS_OFFSET_GOP0_CSC)) >>
			       GOP_CAPS_OFFSET_GOP0_CSC);
		for (i = 0; i < u32GOPNum; i++) {
			snprintf(cstr, sizeof(cstr), "mediatek,mtk-dtv-gop%d", i);
			np = of_find_compatible_node(NULL, NULL, cstr);
			ret = of_property_read_u32(np, name, &u32Tmp);
			GOP_DTS_INFO(name, ret);
			g_GopChipPro.bSupportH4Tap_256Phase[i] =
			    (MS_BOOL) ((u32Tmp & BIT(gu32GopCapsH4Tap[((i != 0) ? 1 : 0)])) >>
				       gu32GopCapsH4Tap[((i != 0) ? 1 : 0)]);
			g_GopChipPro.bSupportH6Tap_8Phase[i] =
			    (MS_BOOL) ((u32Tmp & BIT(gu32GopCapsH2Tap[((i != 0) ? 1 : 0)])) >>
				       gu32GopCapsH2Tap[((i != 0) ? 1 : 0)]);
			g_GopChipPro.bSupportHDuplicate[i] =
			    (MS_BOOL) ((u32Tmp & BIT(gu32GopCapsHDup[((i != 0) ? 1 : 0)])) >>
				       gu32GopCapsHDup[((i != 0) ? 1 : 0)]);
			g_GopChipPro.bSupportV2tap_16Phase[i] = FALSE;
			g_GopChipPro.bSupportV4tap[i] =
			    (MS_BOOL) ((u32Tmp & BIT(gu32GopCapsV4Tap[((i != 0) ? 1 : 0)])) >>
				       gu32GopCapsV4Tap[((i != 0) ? 1 : 0)]);
			g_GopChipPro.bSupportV_BiLinear[i] =
			    (MS_BOOL) ((u32Tmp & BIT(gu32GopCapsVBilinear[((i != 0) ? 1 : 0)])) >>
				       gu32GopCapsVBilinear[((i != 0) ? 1 : 0)]);
			g_GopChipPro.bAFBC_Support[i] =
				(MS_BOOL) ((u32Tmp & BIT(gu32GopCapsAFBC[((i != 0) ? 1 : 0)])) >>
				gu32GopCapsAFBC[((i != 0) ? 1 : 0)]);
			if ((u32Tmp & gu32GopCapsVScaleModeMsk[((i != 0) ? 1 : 0)]) != 0) {
				g_GopChipPro.bGOPWithVscale[i] = TRUE;
				g_GopChipPro.bGOPVscalePipeDelay[i] = FALSE;
			} else {
				g_GopChipPro.bGOPWithVscale[i] = FALSE;
				g_GopChipPro.bGOPVscalePipeDelay[i] = TRUE;
			}
		}
	}

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_GOP_COMPATIBLE);
	if (np != NULL) {
		name = MTK_DRM_GOP_CAPS_GWINNUM;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.TotalGwinNum = (MS_U16) u32Tmp;

		name = MTK_DRM_GOP_CAPS_BEF3DLUT_PD;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.u16BefRGB3DLupPDOffset = (MS_U16) u32Tmp;

		name = MTK_DRM_GOP_CAPS_BEFPQGAMMA_PD;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.u16BefPQGammaPDOffset = (MS_U16) u32Tmp;

		name = MTK_DRM_GOP_CAPS_AFTPQGAMMA_PD;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.u16AftPQGammaPDOffset = (MS_U16) u32Tmp;

		name = MTK_DRM_GOP_CAPS_OPMUX_DB;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.bOPMuxDoubleBuffer = (MS_BOOL) u32Tmp;

		name = MTK_DRM_GOP_CAPS_PD;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.GOP_PD = (MS_S16) u32Tmp;

		name = MTK_DRM_GOP_CAPS_NONVSCALE_PD;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.GOP_NonVS_PD_Offset = (MS_U16) u32Tmp;

		name = MTK_DRM_GOP_CAPS_MUX_DELTA;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.GOP_MUX_Delta = (MS_U16) u32Tmp;

		name = MTK_DRM_GOP_CAPS_MUX_OFFSET;
		ret = DTBArray2U16(np, name, g_GopChipPro.GOP_Mux_Offset, u32GOPNum);
		GOP_DTS_INFO(name, ret);

		name = MTK_DRM_GOP_CAPS_LAYER2MUX;
		pu16array = kmalloc_array(u32GOPNum, sizeof(MS_U16), GFP_KERNEL);
		ret = DTBArray2U16(np, name, pu16array, u32GOPNum);
		for (i = 0; i < u32GOPNum; i++)
			g_GopChipPro.GOP_MapLayer2Mux[i] = (Gop_MuxSel) pu16array[i];
		GOP_DTS_INFO(name, ret);
		kfree(pu16array);

		g_GopChipPro.u8OpMuxNum = (MS_U8) u32GOPNum;

		name = MTK_DRM_GOP_CAPS_BNKFORCEWRITE;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.bBnkForceWrite = (MS_BOOL) u32Tmp;

		name = MTK_DRM_GOP_CAPS_PIXELMODE;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.bPixelModeSupport = (MS_BOOL) u32Tmp;

		name = MTK_DRM_GOP_CAPS_2PTO1P;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.b2Pto1PSupport = (MS_BOOL) u32Tmp;

		name = MTK_DRM_GOP_CAPS_AUTOADJUST_HMIRROR;
		ret = of_property_read_u32(np, name, &u32Tmp);
		GOP_DTS_INFO(name, ret);
		g_GopChipPro.bAutoAdjustMirrorHSize = (MS_BOOL) u32Tmp;
	}
	return 0;
}

static MS_BOOL _IsGwinIdValid(MS_GOP_CTX_LOCAL *pDrvCtx, MS_U8 u8GwinID)
{
	if (u8GwinID >= pDrvCtx->pGopChipProperty->TotalGwinNum)
		return FALSE;
	else
		return TRUE;
}

static MS_BOOL _IsGopNumVaild(MS_GOP_CTX_LOCAL *pDrvCtx, MS_U8 u8GopNum)
{
	if (u8GopNum >= MDrv_GOP_GetMaxGOPNum(pDrvCtx))
		return FALSE;
	else
		return TRUE;
}

static void _MDrv_GOP_SetPxmSize(MS_U16 u16Width, MS_U16 u16Height)
{
	enum pxm_return ret;
	struct pxm_size_info info;

	memset(&info, 0, sizeof(struct pxm_size_info));

	info.win_id = 0;  // gfx not support ts, win id set 0
	info.point = E_PXM_POINT_GFX;
	info.size.h_size = u16Width;
	info.size.v_size = u16Height;
	ret = pxm_set_size(&info);
	if (ret != E_PXM_RET_OK) {
		DRM_ERROR("[%s][%d] GOP set pxm size fail(%d)!\n",
			__func__, __LINE__, ret);
	}
}

void GOP_GWIN_TriggerRegWriteIn(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, E_DRV_GOP_TYPE u8GopType,
				MS_BOOL bForceWriteIn, MS_BOOL bMl)
{
	MS_U8 u8GOP = 0;

	switch (u8GopType) {
	case E_GOP0:
		u8GOP = 0;
		break;
	case E_GOP1:
		u8GOP = 1;
		break;
	case E_GOP2:
		u8GOP = 2;
		break;
	case E_GOP3:
		u8GOP = 3;
		break;
	case E_GOP4:
		u8GOP = 4;
		break;
	case E_GOP5:
		u8GOP = 5;
		break;
	default:
		break;
	}

	if (bForceWriteIn) {
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, TRUE, E_HW_GOP_FORCEWRITE);
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, FALSE, E_HW_GOP_FORCEWRITE);
	} else {
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, TRUE, E_HW_GOP_WITHVSYNC);
	}
}

static MS_U32 _GOP_GWIN_AlignChecker(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_U16 *width,
				     MS_U16 fmtbpp)
{
	MS_U16 alignfactor = 0;

	if (pGOPCtx->pGOPCtxShared->bPixelMode[u8GOPNum] == TRUE)
		alignfactor = 1;

	if ((alignfactor != 0) && (*width % alignfactor != 0)) {
		DRM_ERROR("\n\n%s, This FB format needs to %d-pixels alignment !!!\n\n",
			  __func__, alignfactor);
	}

	return CHECKALIGN_SUCCESS;
}

static MS_U32 _GOP_GWIN_2PEngineAlignChecker(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					     MS_PHY *width)
{
	MS_U16 alignfactor = 0;

	if (pGOPCtx->pGopChipProperty->b2Pto1PSupport == TRUE) {
		alignfactor = 2;

		if ((alignfactor != 0) && (*width % alignfactor != 0)) {
			DRM_ERROR("\n\n%s, Not mach to %d-pixels alignment !!!\n\n", __func__,
				  alignfactor);
			return CHECKALIGN_PARA_FAIL;
		} else {
			return CHECKALIGN_SUCCESS;
		}
	}
	return CHECKALIGN_SUCCESS;
}

static MS_U32 _GOP_GWIN_FB_AddrAlignChecker(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					    MS_PHY Address, MS_U16 fmtbpp)
{
	MS_U32 alignfactor = 0;

	if (pGOPCtx->pGOPCtxShared->bPixelMode[u8GOPNum] == TRUE) {
		alignfactor =
		    (MS_U32) ((MDrv_GOP_GetWordUnit(pGOPCtx, u8GOPNum) * (fmtbpp >> 3)) & 0xFF);
	}

	if ((Address & (alignfactor - 1)) != 0) {
		DRM_ERROR("%s,%d FB=0x%tx address need %td-bytes aligned!\n", __func__,
			  __LINE__, (ptrdiff_t) Address, (ptrdiff_t) alignfactor);
	}

	return CHECKALIGN_SUCCESS;
}

void GOP_SetGopGwinHVPixel(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win, MS_U8 u8GOP,
			   DRV_GOP_GWIN_INFO *pinfo)
{
	KDrv_HW_GOP_SetGopGwinHVPixel(u8GOP, u8win, pinfo->u16DispHPixelStart,
		pinfo->u16DispHPixelEnd, pinfo->u16DispVPixelStart, pinfo->u16DispVPixelEnd);
}


static void GOP_SetGopExtendGwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				     DRV_GOP_GWIN_INFO *pinfo)
{
	MS_U16 u16Bpp = 0;
	MS_U8 u8GOP = 0, u8GOPIdx = 0;
	MS_U16 u16Pitch = 0;
	MS_U8 u8win_temp = 0;

	u8GOP = MDrv_DumpGopByGwinId(&pGOPDrvLocalCtx->apiCtxLocal, u8win);
	u8win_temp = u8win;
	for (u8GOPIdx = u8GOP; u8GOPIdx > 0; u8GOPIdx--) {
		u8win_temp =
		    u8win_temp - KDrv_HW_GOP_GetGwinNum(u8GOPIdx - 1, 0,
							E_HW_GOP_GET_GWIN_MAX_NUMBER);
	}

	u16Bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (u16Bpp == FB_FMT_AS_DEFAULT) {
		DRM_ERROR("[%s] invalid color format\n", __func__);
		return;
	}

	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP, &(pinfo->u16WinX),
			       u16Bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP,
			       &(pinfo->u16DispHPixelStart), u16Bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP,
			       &(pinfo->u16DispHPixelEnd), u16Bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP, &u16Pitch, u16Bpp);
	_GOP_GWIN_FB_AddrAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP,
				      pinfo->u64DRAMRBlkStart, u16Bpp);

	KDrv_HW_GOP_SetDram_Addr(u8GOP, u8win_temp, pinfo->u64DRAMRBlkStart, E_GOP_RBLKAddr);

	if ((u16Bpp / 8) != 0) {
		u16Pitch = pinfo->u16RBlkHRblkSize / (u16Bpp / 8);
	}

	KDrv_HW_GOP_SetPitch(u8GOP, u8win_temp, u16Pitch, pinfo->u16RBlkVPixSize);

	GOP_SetGopGwinHVPixel(pGOPDrvLocalCtx, u8win, u8GOP, pinfo);
}

static void GOP_SetGop1GwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				DRV_GOP_GWIN_INFO *pinfo)
{
	MS_U16 bpp;
	MS_U16 u16Pitch = 0;

	u8win = u8win - KDrv_HW_GOP_GetGwinNum(0, 0, E_HW_GOP_GET_GWIN_MAX_NUMBER);
	bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (bpp == FB_FMT_AS_DEFAULT) {
		DRM_ERROR("[%s] invalid color format\n", __func__);
		return;
	}

	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1, &(pinfo->u16WinX),
			       bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1,
			       &(pinfo->u16DispHPixelStart), bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1,
			       &(pinfo->u16DispHPixelEnd), bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1, &u16Pitch, bpp);
	_GOP_GWIN_FB_AddrAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1,
				      pinfo->u64DRAMRBlkStart, bpp);

	KDrv_HW_GOP_SetDram_Addr(E_GOP1, u8win, pinfo->u64DRAMRBlkStart, E_GOP_RBLKAddr);

	if ((bpp / 8) != 0) {
		u16Pitch = pinfo->u16RBlkHRblkSize / (bpp / 8);
	}

	KDrv_HW_GOP_SetPitch(E_GOP1, u8win, u16Pitch, pinfo->u16RBlkVPixSize);

	GOP_SetGopGwinHVPixel(pGOPDrvLocalCtx, u8win, E_GOP1, pinfo);
}

static void GOP_SetGop0GwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				DRV_GOP_GWIN_INFO *pinfo)
{
	MS_U16 bpp;
	MS_U16 u16Pitch = 0;

	bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (bpp == FB_FMT_AS_DEFAULT) {
		DRM_ERROR("[%s] invalid color format\n", __func__);
		return;
	}

	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0, &(pinfo->u16WinX),
			       bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0,
			       &(pinfo->u16DispHPixelStart), bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0,
			       &(pinfo->u16DispHPixelEnd), bpp);
	_GOP_GWIN_AlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0,
			       &(pinfo->u16RBlkHRblkSize), bpp);

	_GOP_GWIN_FB_AddrAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0,
				      pinfo->u64DRAMRBlkStart, bpp);

	KDrv_HW_GOP_SetDram_Addr(E_GOP0, u8win, (MS_PHY) pinfo->u64DRAMRBlkStart, E_GOP_RBLKAddr);

	if ((bpp / 8) != 0) {
		u16Pitch = pinfo->u16RBlkHRblkSize / (bpp / 8);
	}

	KDrv_HW_GOP_SetPitch(E_GOP0, u8win, u16Pitch, pinfo->u16RBlkVPixSize);

	GOP_SetGopGwinHVPixel(pGOPDrvLocalCtx, u8win, E_GOP0, pinfo);
}

//------------------------------------------------------------------------------
//  Global Functions
//------------------------------------------------------------------------------
MS_U8 MDrv_DumpGopByGwinId(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U32 GwinID)
{
	MS_U8 gop_num = 0;
	MS_U8 u8MaxGOPNum = MDrv_GOP_GetMaxGOPNum(pGOPCtx);
	MS_U8 gop_gwinnum[u8MaxGOPNum];
	MS_U8 gop_start[u8MaxGOPNum];
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	for (gop_num = 0; gop_num < u8MaxGOPNum; gop_num++) {
		gop_start[gop_num] = 0;
		gop_gwinnum[gop_num] = 0;
	}

	for (gop_num = 0; gop_num < u8MaxGOPNum; gop_num++) {
		gop_gwinnum[gop_num] =
		    MDrv_GOP_GetGwinNum((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, gop_num);
	}

	gop_start[0] = 0;
	for (gop_num = 1; gop_num < u8MaxGOPNum; gop_num++)
		gop_start[gop_num] = gop_start[gop_num - 1] + gop_gwinnum[gop_num - 1];

	for (gop_num = 0; gop_num < u8MaxGOPNum; gop_num++) {
		if (gop_num == 0) {
			if (GwinID < gop_start[gop_num + 1])
				return gop_num;
		} else if (gop_num == (u8MaxGOPNum - 1)) {
			if ((GwinID >= gop_start[gop_num])
			    && (GwinID < pGOPDrvLocalCtx->pGopChipPro->TotalGwinNum))
				return gop_num;
		} else {
			if ((GwinID >= gop_start[gop_num]) && (GwinID < gop_start[gop_num + 1]))
				return gop_num;
		}
	}

	DRM_ERROR("[GOP][%s, %d] no find gop idx!!\n", __func__, __LINE__);
	return INVALID_GOP_ID;
}

MS_BOOL MDrv_GOP_GetGOPEnum(MS_GOP_CTX_LOCAL *pGOPCtx, GOP_TYPE_DEF *GOP_TYPE)
{
	GOP_TYPE->GOP0 = E_GOP0;
	GOP_TYPE->GOP1 = E_GOP1;
	GOP_TYPE->GOP2 = E_GOP2;
	GOP_TYPE->GOP3 = E_GOP3;
	GOP_TYPE->GOP4 = E_GOP4;
	GOP_TYPE->GOP5 = E_GOP5;
	GOP_TYPE->MIXER = E_MIXER;

	return TRUE;
}

MS_U8 MDrv_GOP_GetMaxGOPNum(MS_GOP_CTX_LOCAL *pGOPCtx)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	return (MS_U8) pGOPDrvLocalCtx->pGopChipPro->u32GOPNum;
}

MS_U8 MDrv_GOP_GetGwinNum(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GopNum)
{
	EN_HW_GOP_GWIN_MODE eHWGopGwinMode = E_HW_GOP_GET_GWIN_MAX_NUMBER;

	return KDrv_HW_GOP_GetGwinNum(u8GopNum, 0, eHWGopGwinMode);
}

MS_BOOL MDrv_GOP_SetIOMapBase(MS_GOP_CTX_LOCAL *pGOPCtx)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	uint8_t i;

	if (pGOPDrvLocalCtx != NULL) {
		for (i = 0; i < GOP_MAX_SUPPORT_DEFAULT; i++) {
			if (pGOPDrvLocalCtx->pGopChipPro->PAdr[i] != 0) {
				drv_hwreg_common_setRIUaddr(
				pGOPDrvLocalCtx->pGopChipPro->PAdr[i],
				pGOPDrvLocalCtx->pGopChipPro->u32RegSize[i],
				(uint64_t)pGOPDrvLocalCtx->pGopChipPro->VAdr[i]);
			}
		}
		if (pGOPDrvLocalCtx->pGopChipPro != NULL)
			KDrv_HW_GOP_SetIOMapBase((MS_VIRT *)pGOPDrvLocalCtx->pGopChipPro->VAdr);
	}

	return TRUE;
}

void *MDrv_GOP_GetShareMemory(MS_BOOL *pbNeedInitShared)
{
	GOP_CTX_DRV_SHARED *pDrvGOPShared = NULL;
	MS_BOOL bNeedInitShared = FALSE;

#ifdef STI_PLATFORM_BRING_UP
	pDrvGOPShared = &g_gopDrvCtxShared;
	bNeedInitShared = TRUE;
#else
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
	MS_U32 u32ShmId;
	MS_VIRT VAddr;
	MS_U32 u32BufSize;
	char SHM_Id[] = "Linux GOP driver";

	if (FALSE ==
	    MsOS_SHM_GetId((MS_U8 *) SHM_Id, sizeof(GOP_CTX_DRV_SHARED), &u32ShmId, &VAddr,
			   &u32BufSize, MSOS_SHM_QUERY)) {
		if (FALSE ==
		    MsOS_SHM_GetId((MS_U8 *) SHM_Id, sizeof(GOP_CTX_DRV_SHARED), &u32ShmId, &VAddr,
				   &u32BufSize, MSOS_SHM_CREATE)) {
			DRM_ERROR("SHM allocation failed!\n");
			return NULL;
		}
		memset((MS_U8 *) VAddr, 0, sizeof(GOP_CTX_DRV_SHARED));
		pDrvGOPShared = (GOP_CTX_DRV_SHARED *) VAddr;
		pDrvGOPShared->apiCtxShared.bInitShared = TRUE;
		bNeedInitShared = TRUE;
		//CSC SHM Init
	}
	pDrvGOPShared = (GOP_CTX_DRV_SHARED *) VAddr;
#else
	pDrvGOPShared = &g_gopDrvCtxShared;
	bNeedInitShared = TRUE;
#endif
#endif

	*pbNeedInitShared = (bNeedInitShared | pDrvGOPShared->apiCtxShared.bInitShared);
	return (void *)pDrvGOPShared;
}

void Drv_GOP_Init_ChipPro(GOP_CHIP_PROPERTY **pGopChipPro)
{
	if (readChipCaps_DTB() != 0)
		DRM_ERROR("[%s, %d]readChipCaps_DTB failed \r\n", __func__, __LINE__);

	*pGopChipPro = &g_GopChipPro;
}

MS_GOP_CTX_LOCAL *Drv_GOP_Init_Context(void *pInstance, MS_BOOL *pbNeedInitShared)
{
	GOP_CTX_DRV_SHARED *pDrvGOPShared;
	MS_BOOL bNeedInitShared = FALSE;
	MS_U8 u8Index = 0;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE * psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	pDrvGOPShared = (GOP_CTX_DRV_SHARED *) MDrv_GOP_GetShareMemory(&bNeedInitShared);

	*pbNeedInitShared = bNeedInitShared;
	memset(&g_gopDrvCtxLocal, 0, sizeof(g_gopDrvCtxLocal));

	if (bNeedInitShared) {
		memset(pDrvGOPShared, 0, sizeof(GOP_CTX_DRV_SHARED));
	}

	g_gopDrvCtxLocal.pDrvCtxShared = pDrvGOPShared;
	g_gopDrvCtxLocal.apiCtxLocal.pGOPCtxShared = &pDrvGOPShared->apiCtxShared;

	Drv_GOP_Init_ChipPro(&g_gopDrvCtxLocal.pGopChipPro);

	g_gopDrvCtxLocal.apiCtxLocal.pGopChipProperty = g_gopDrvCtxLocal.pGopChipPro;

	if (bNeedInitShared) {
		MS_U32 gId;
		MS_GOP_CTX_LOCAL *pGOPCtxLocal = &g_gopDrvCtxLocal.apiCtxLocal;

		*pbNeedInitShared = bNeedInitShared;

		if (g_gopDrvCtxLocal.apiCtxLocal.pGopChipProperty->TotalGwinNum >=
		    SHARED_GWIN_MAX_COUNT) {
			DRM_INFO("Error - TotalGwinNum >= SHARED_GWIN_MAX_COUNT!!\n");
		}

		for (gId = 0; gId < g_gopDrvCtxLocal.apiCtxLocal.pGopChipProperty->TotalGwinNum;
		     gId++) {
			pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u32CurFBId = INVALID_GWIN_ID;
			pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].bIsShared = FALSE;
			pGOPCtxLocal->pGOPCtxShared->gwinMap[gId].u16SharedCnt = 0x0;
		}

		for (gId = 0; gId < MDrv_GOP_GetMaxGOPNum(pGOPCtxLocal); gId++) {
			pGOPCtxLocal->pGOPCtxShared->s32OutputColorType[gId] = -1;
			pGOPCtxLocal->pGOPCtxShared->bGopHasInitialized[gId] = FALSE;
		}
	}

	return (MS_GOP_CTX_LOCAL *) &g_gopDrvCtxLocal;

}

void MDrv_GOP_Init(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	MS_GOP_CTX_LOCAL apiCtxLocal = pGOPDrvLocalCtx->apiCtxLocal;
	MS_GOP_CTX_SHARED *pGOPCtxShared = apiCtxLocal.pGOPCtxShared;
	MS_U16 u16PnlWidth = pGOPCtxShared->u16PnlWidth[u8GOPNum];
	MS_U16 u16PnlHeight = pGOPCtxShared->u16PnlHeight[u8GOPNum];

	KDrv_HW_GOP_Init(u8GOPNum);

	if (pGOPCtx->pGopChipProperty->bPixelModeSupport)
		KDrv_HW_GOP_PixelModeEnable(u8GOPNum, TRUE);

	if (pGOPCtx->pGopChipProperty->bOPMuxDoubleBuffer)
		KDrv_HW_GOP_OpMuxDbEnable(TRUE);

	if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
		GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_M6_SERIES_MAJOR) {
		//step2 HVSP always scaling 2x, if timing is 8K
		if (u16PnlWidth == GOP_8K_WIDTH && u16PnlHeight == GOP_8K_HEIGHT
		&&
		((u32ChipVer & GOP_MINOR_IPVERSION_OFFSET_MSK)
			== GOP_M6_SERIES_MINOR)) {
			KDrv_HW_GOP_SetStep2HVSPHscale(TRUE, GOP_SCALING_2TIMES);
			KDrv_HW_GOP_SetStep2HVSPVscale(TRUE, GOP_SCALING_2TIMES);
			KDrv_HW_GOP_SetMixer2OutSize(GOP_8K_WIDTH, GOP_8K_HEIGHT);
			_MDrv_GOP_SetPxmSize(GOP_8K_WIDTH, GOP_8K_HEIGHT);
			KDrv_HW_GOP_SetMixer4OutSize(GOP_4K_WIDTH, GOP_4K_HEIGHT);
		} else {
			//If timing, then only use step1 HVSP to scaling up
			KDrv_HW_GOP_SetStep2HVSPHscale(FALSE, 0x0);
			KDrv_HW_GOP_SetStep2HVSPVscale(FALSE, 0x0);
			KDrv_HW_GOP_SetMixer2OutSize(GOP_4K_WIDTH, GOP_4K_HEIGHT);
			_MDrv_GOP_SetPxmSize(GOP_4K_WIDTH, GOP_4K_HEIGHT);
			KDrv_HW_GOP_SetMixer4OutSize(GOP_4K_WIDTH, GOP_4K_HEIGHT);
		}
	}

}

/********************************************************************************/
/// Set the time when new GWIN settings take effect
/// @param bEnable \b IN
///   - # TRUE the new setting moved from internal register buffer
///            to active registers immediately
///   - # FALSE new settings take effect when next VSYNC is coming
/********************************************************************************/
void MDrv_GOP_GWIN_SetForceWrite(MS_GOP_CTX_LOCAL *pGOPCtx, MS_BOOL bEnable)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	pGOPDrvLocalCtx->gop_gwin_frwr = bEnable;
}

/********************************************************************************/
/// Get the status for GWIN settings take effect
/// @param bEnable \b IN
///   - # TRUE the new setting moved from internal register buffer
///            to active registers immediately
///   - # FALSE new settings take effect when next VSYNC is coming
/********************************************************************************/
MS_BOOL MDrv_GOP_GWIN_IsForceWrite(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	return (MS_BOOL) (pGOPDrvLocalCtx->gop_gwin_frwr);
}

/********************************************************************************/
/// Enable or Disable transparent color
/// @param fmt \b IN @copydoc EN_GOP_TRANSCLR_FMT
/// @param bEnable \b IN
///   - # TRUE enable transparent color
///   - # FALSE disable transparent color
/********************************************************************************/
void MDrv_GOP_GWIN_EnableTransClr(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, GOP_TransClrFmt fmt,
				  MS_BOOL bEnable)
{
	E_GOP_TransClrFmt enFmt = E_GOP_TRANSCLR_FMT0;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	switch (fmt) {
	case DRV_GOPTRANSCLR_FMT0:
		enFmt = E_GOP_TRANSCLR_FMT0;
		break;
	case DRV_GOPTRANSCLR_FMT1:
		enFmt = E_GOP_TRANSCLR_FMT1;
		break;
	case DRV_GOPTRANSCLR_FMT2:
		enFmt = E_GOP_TRANSCLR_FMT2;
		break;
	case DRV_GOPTRANSCLR_FMT3:
		enFmt = E_GOP_TRANSCLR_FMT3;
		break;
	default:
		break;
	}
	KDrv_HW_GOP_GWIN_TransClrEnable(u8GOP, enFmt, bEnable);
}

void MDrv_GOP_GWIN_SetMux(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, Gop_MuxSel eGopMux)
{
	E_HW_GOP_MuxSel eHWGopMux = E_HW_MAX_GOP_MUX_SUPPORT;

	switch (eGopMux) {
	case E_GOP_MUX0:
		eHWGopMux = E_HW_GOP_MUX0;
		break;
	case E_GOP_MUX1:
		eHWGopMux = E_HW_GOP_MUX1;
		break;
	case E_GOP_MUX2:
		eHWGopMux = E_HW_GOP_MUX2;
		break;
	case E_GOP_MUX3:
		eHWGopMux = E_HW_GOP_MUX3;
		break;
	case E_GOP_MUX4:
		eHWGopMux = E_HW_GOP_MUX4;
		break;
	default:
		break;
	}

	KDrv_HW_GOP_GWIN_SetMux(u8GOPNum, eHWGopMux);

}

void MDrv_GOP_MapLayer2Mux(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U32 u32Layer, MS_U8 u8GopNum,
			   MS_U32 *pu32Mux)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	*pu32Mux = pGOPDrvLocalCtx->pGopChipPro->GOP_MapLayer2Mux[u32Layer];
}

/********************************************************************************/
/// Set GOP0 and GOP1 scaler setting
/// @param gopNum \b IN  0: GOP0  1:GOP1
/// @param bEnable \b IN
///   - # TRUE enable
///   - # FALSE disable
/********************************************************************************/
void MDrv_GOP_SetGOPEnable2SC(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum, MS_BOOL bEnable)
{
	KDrv_HW_GOP_SetGOP2SCEnable(gopNum, bEnable);
}

#ifdef CONFIG_GOP_GWIN_MISC
void MDrv_GOP_SetGOPEnable2Mode1(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum, MS_BOOL bEnable)
{
	DRV_GOPDstType enGopDst = E_DRV_GOP_DST_INVALID;

	KDrv_HW_GOP_PreAlphaEnable(gopNum, bEnable);
	MDrv_GOP_GWIN_GetDstPlane(pGOPCtx, gopNum, &enGopDst);

	if (enGopDst == E_DRV_GOP_DST_OP0)
		MDrv_GOP_GWIN_SetAlphaInverse(pGOPCtx, gopNum, !bEnable);

}

void MDrv_GOP_GetGOPEnable2Mode1(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum, MS_BOOL *pbEnable)
{
	*pbEnable = KDrv_HW_GOP_GetPreAlphaMode(gopNum);
}
#endif

/********************************************************************************/
/// Set GOP alpha inverse
/// @param bEnable \b IN
///   - # TRUE enable alpha inverse
///   - # FALSE disable alpha inverse
/********************************************************************************/
void MDrv_GOP_GWIN_SetAlphaInverse(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8gopNum, MS_BOOL bEnable)
{
	DRV_GOPDstType enGopDst = E_DRV_GOP_DST_INVALID;

	MDrv_GOP_GWIN_GetDstPlane(pGOPCtx, u8gopNum, &enGopDst);
	if (enGopDst == E_DRV_GOP_DST_OP0) {
		MS_BOOL bPreAlpha = FALSE;

		MDrv_GOP_GetGOPEnable2Mode1(pGOPCtx, u8gopNum, &bPreAlpha);

		if (bPreAlpha) {
			if (bEnable) {
				DRM_INFO
				    ("GOP%d AlphaInv will be set to false dueto new alpha mode%s\n",
				     u8gopNum, __func__);
			}
			bEnable = FALSE;
		}
	}

	KDrv_HW_GOP_GWIN_AlphaInvEnable(u8gopNum, bEnable);

}

//-------------------------------------------------------------------------------------------------
/// Set GOP Destination DisplayPlane
/// @param u8GOP_num \b IN: GOP number: 0 or 1
/// @param eDstType \b IN: GOP Destination DisplayPlane Type
/// @return TRUE: success / FALSE: fail
//-------------------------------------------------------------------------------------------------
GOP_Result MDrv_GOP_GWIN_SetDstPlane(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
				     DRV_GOPDstType eDstType)
{
	GOP_Result ret = GOP_SUCCESS;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	E_HW_GOP_DstType eHWGopDst = E_HW_GOP_DST_OP0;

	switch (eDstType) {
	case E_DRV_GOP_DST_OP0:
		eHWGopDst = E_HW_GOP_DST_OP0;
		break;
	case E_DRV_GOP_DST_FRC:
		eHWGopDst = E_HW_GOP_DST_FRC;
		break;
	default:
		break;
	}

	KDrv_HW_GOP_GWIN_SetDstPlane(u8GOPNum, eHWGopDst);

	return ret;
}

GOP_Result MDrv_GOP_GWIN_GetDstPlane(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
				     DRV_GOPDstType *pGopDst)
{
	GOP_Result ret;
	MS_BOOL HWRegRet;
	E_HW_GOP_DstType eHWGopDst;

	HWRegRet = KDrv_HW_GOP_GWIN_GetDstPlane(u8GOPNum, &eHWGopDst);
	if (HWRegRet)
		ret = GOP_SUCCESS;
	else
		ret = GOP_FAIL;

	switch (eHWGopDst) {
	case E_HW_GOP_DST_OP0:
		*pGopDst = E_DRV_GOP_DST_OP0;
		break;
	case E_HW_GOP_DST_FRC:
		*pGopDst = E_DRV_GOP_DST_FRC;
		break;
	default:
		break;
	}

	return ret;
}

/********************************************************************************/
/// Set stretch window property
/// @param u8GOP_num \b IN 0: GOP0  1:GOP1
/// @param eDstType \b IN \copydoc EN_GOP_DST_TYPE
/// @param x \b IN stretch window horizontal start position
/// @param y \b IN stretch window vertical start position
/// @param width \b IN stretch window width
/// @param height \b IN stretch window height
/********************************************************************************/
void MDrv_GOP_GWIN_SetStretchWin(MS_U8 u8GOP, MS_U16 x, MS_U16 y,
				 MS_U16 width, MS_U16 height)
{
	MS_BOOL bStep2HScaleEn = FALSE, bStep2VScaleEn = FALSE;
	MS_U32 u32Step2HRatio = 0, u32Step2VRatio = 0;

	if (u8GOP != E_GOP4) {
		KDrv_HW_GOP_GetStep2HVSPHScale(&bStep2HScaleEn, &u32Step2HRatio);
		KDrv_HW_GOP_GetStep2HVSPVScale(&bStep2VScaleEn, &u32Step2VRatio);
		if (bStep2HScaleEn)
			x = ((x * u32Step2HRatio) / SCALING_MULITPLIER);
		if (bStep2VScaleEn)
			y = ((y * u32Step2VRatio) / SCALING_MULITPLIER);
	}
	KDrv_HW_GOP_SetGopStretchWin(u8GOP, x, y, width, height);
}
EXPORT_SYMBOL(MDrv_GOP_GWIN_SetStretchWin);

//Internal use only, get the real HW register setting on stretch win
//For API usage, please use g_pGOPCtxLocal->pGOPCtxShared->u16StretchWinWidth
void MDrv_GOP_GWIN_Get_StretchWin(MS_U8 u8GOP, MS_U16 *x, MS_U16 *y,
				  MS_U16 *width, MS_U16 *height)
{
	MS_U16 u16RegWidth;

	KDrv_HW_GOP_GetGopStretchWin(u8GOP, x, y, &u16RegWidth, height);
	*width = u16RegWidth;
}
EXPORT_SYMBOL(MDrv_GOP_GWIN_Get_StretchWin);
/********************************************************************************/
/// Set stretch window H-Stretch
/// @param bEnable \b IN
///   - # TRUE enable
///   - # FALSE disable
/// @param src \b IN original stretch size
/// @param dst \b IN stretch out size
/********************************************************************************/
void MDrv_GOP_GWIN_Set_HSCALE(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bEnable, MS_U16 src,
			      MS_U16 dst)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *)pGOPCtx;
	MS_GOP_CTX_LOCAL apiCtxLocal = pGOPDrvLocalCtx->apiCtxLocal;
	MS_GOP_CTX_SHARED *pGOPCtxShared = apiCtxLocal.pGOPCtxShared;
	MS_U16 u16PnlWidth = pGOPCtxShared->u16PnlWidth[u8GOP];
	MS_U32 u32Hratio = 0;
	MS_U16 u16Hout = 0;

	if (dst > u16PnlWidth) {
		DRM_INFO("[%s] GOP:%d, dst:%d > PnlWidth:%d !!\n", __func__,
			u8GOP, dst, u16PnlWidth);
		dst = u16PnlWidth;
		if (src == dst)
			bEnable = FALSE;
	}

	if (u16PnlWidth == GOP_8K_WIDTH) {
		if (bEnable == TRUE) {
			if ((dst / 2) > src) {
				//step1 HVSP scaling to half size,
				//others size use step2 HVSP when 8k timing
				if ((dst/2) != 0) {
					u32Hratio = (MS_U32)(src * SCALING_MULITPLIER) / (dst / 2);
					u16Hout = (dst / 2);
				} else {
					u32Hratio = 0;
					u16Hout =  0;
				}
			} else {
				u32Hratio = 0;
				u16Hout = 0;
				bEnable = FALSE;
			}
		} else {
			u32Hratio = 0;
			u16Hout = 0;
		}
	} else {
		if (bEnable == TRUE) {
			if (dst != 0)
				u32Hratio = (MS_U32)(src * SCALING_MULITPLIER) / dst;
			else
				u32Hratio = 0;
			u16Hout = dst;
		} else {
			u32Hratio = 0;
			u16Hout = 0;
		}
	}
	KDrv_HW_GOP_SetStep1HVSPHscale(u8GOP, bEnable, u32Hratio, u16Hout);
}

/********************************************************************************/
/// Set stretch window V-Stretch
/// @param bEnable \b IN
///   - # TRUE enable
///   - # FALSE disable
/// @param src \b IN original stretch size
/// @param dst \b IN stretch out size
/********************************************************************************/
void MDrv_GOP_GWIN_Set_VSCALE(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bEnable, MS_U16 src,
			      MS_U16 dst)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *)pGOPCtx;
	MS_GOP_CTX_LOCAL apiCtxLocal = pGOPDrvLocalCtx->apiCtxLocal;
	MS_GOP_CTX_SHARED *pGOPCtxShared = apiCtxLocal.pGOPCtxShared;
	MS_U16 u16PnlHeight = pGOPCtxShared->u16PnlHeight[u8GOP];
	MS_U32 u32Vratio = 0;
	MS_U16 u16Vout = 0;

	if (_IsGopNumVaild(pGOPCtx, u8GOP) == FALSE) {
		DRM_INFO("[%s] not support gop%d in this chip version!!\n", __func__, u8GOP);
		return;
	}

	if (dst > u16PnlHeight) {
		DRM_INFO("[%s] GOP:%d, dst:%d > PnlHeight:%d !!\n", __func__,
			u8GOP, dst, u16PnlHeight);
		dst = u16PnlHeight;
		if (src == dst)
			bEnable = FALSE;
	}

	if (u16PnlHeight == GOP_8K_HEIGHT) {
		if (bEnable == TRUE) {
			if ((dst / 2) > src) {
				//step1 HVSP scaling to half size,
				//others size use step2 HVSP when 8k timing
				if ((dst/2) != 0) {
					u32Vratio = (MS_U32)(src * SCALING_MULITPLIER) / (dst / 2);
					u16Vout = (dst / 2);
				} else {
					u32Vratio = 0;
					u16Vout = 0;
				}
			} else {
				u32Vratio = 0;
				u16Vout = 0;
				bEnable = FALSE;
			}
		} else {
			u32Vratio = 0;
			u16Vout = 0;
		}
	} else {
		if (bEnable == TRUE) {
			if (dst != 0)
				u32Vratio = (MS_U32)(src * SCALING_MULITPLIER) / dst;
			else
				u32Vratio = 0;
			u16Vout = dst;
		} else {
			u32Vratio = 0;
			u16Vout = 0;
		}
	}
	KDrv_HW_GOP_SetStep1HVSPVscale(u8GOP, bEnable, u32Vratio, u16Vout);
}


//-------------------------------------------------------------------------------------------------
/// Set GOP H-Stretch Mode
/// @param HStrchMode \b IN \copydoc EN_GOP_STRETCH_HMODE
//-------------------------------------------------------------------------------------------------
void MDrv_GOP_GWIN_Set_HStretchMode(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
				    DRV_GOPStrchHMode HStrchMode)
{
	E_HW_GOPStrchHMode eHWHMode = E_HW_GOP_HSTRCH_4TAPE;

	switch (HStrchMode) {
	case E_DRV_GOP_HSTRCH_DUPLICATE:
	case E_DRV_GOP_HSTRCH_6TAPE_NEAREST:
	case E_DRV_GOP_HSTRCH_6TAPE:
	case E_DRV_GOP_HSTRCH_6TAPE_LINEAR:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN0:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN1:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN2:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN3:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN4:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN5:
	case E_DRV_GOP_HSTRCH_2TAPE:
	case E_DRV_GOP_HSTRCH_4TAPE:
	case E_DRV_GOP_HSTRCH_NEW4TAP_45:
	case E_DRV_GOP_HSTRCH_NEW4TAP_50:
	case E_DRV_GOP_HSTRCH_NEW4TAP_55:
	case E_DRV_GOP_HSTRCH_NEW4TAP_65:
	case E_DRV_GOP_HSTRCH_NEW4TAP_75:
	case E_DRV_GOP_HSTRCH_NEW4TAP_85:
	case E_DRV_GOP_HSTRCH_NEW4TAP_95:
	case E_DRV_GOP_HSTRCH_NEW4TAP_100:
	case E_DRV_GOP_HSTRCH_NEW4TAP_105:
	default:
		eHWHMode = E_HW_GOP_HSTRCH_4TAPE;
		break;
	}

	KDrv_HW_GOP_GWIN_Set_HStretchMode(u8GOP, eHWHMode);
}


//-------------------------------------------------------------------------------------------------
/// Set GOP V-Stretch Mode
/// @param VStrchMode \b IN \copydoc EN_GOP_STRETCH_VMODE
//-------------------------------------------------------------------------------------------------
void MDrv_GOP_GWIN_Set_VStretchMode(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
				    DRV_GOPStrchVMode VStrchMode)
{
	E_HW_GOPStrchHMode eHWVMode = E_HW_GOP_VSTRCH_BILINEAR;

	switch (VStrchMode) {
	case E_DRV_GOP_VSTRCH_DUPLICATE:
	case E_DRV_GOP_VSTRCH_NEAREST:
	case E_DRV_GOP_VSTRCH_LINEAR:
	case E_DRV_GOP_VSTRCH_LINEAR_GAIN0:
	case E_DRV_GOP_VSTRCH_LINEAR_GAIN1:
	case E_DRV_GOP_VSTRCH_LINEAR_GAIN2:
	case E_DRV_GOP_VSTRCH_4TAP:
	case E_DRV_GOP_HSTRCH_V4TAP_100:
		eHWVMode = E_HW_GOP_VSTRCH_4TAP;
		break;
	default:
		break;
	}

	KDrv_HW_GOP_GWIN_Set_VStretchMode(u8GOP, eHWVMode);
}

/********************************************************************************/
/// Set GWIN alpha blending
/// @param u8win \b IN \copydoc GWINID
/// @param bEnable \b IN
///   - # TRUE enable alpha blending
///   - # FALSE disable alpha blending
/// @param u8coef \b IN alpha blending coefficient (0-7)
/********************************************************************************/
void MDrv_GOP_GWIN_SetBlending(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win, MS_BOOL bEnable,
			       MS_U8 u8coef)
{
	MS_U8 u8GOP = 0;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (_IsGwinIdValid(pGOPCtx, u8win) == FALSE) {
		DRM_ERROR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return;
	}

	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);
	KDrv_HW_GOP_GWIN_PixelAlphaEnable(u8GOP, u8win, bEnable, u8coef);
}

/********************************************************************************/
/// Set GWIN data format to GOP registers
/// @param u8win \b IN \copydoc GWINID
/// @param clrtype \b IN \copydoc EN_GOP_COLOR_TYPE
/********************************************************************************/
void MDrv_GOP_GWIN_SetWinFmt(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win, DRV_GOPColorType clrtype)
{
	MS_U8 u8GOP = 0;
	GOP_ColorFormat eGOPHWClr;

	if (_IsGwinIdValid(pGOPCtx, u8win) == FALSE) {
		DRM_ERROR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return;
	}

	switch (clrtype) {
	case E_DRV_GOP_COLOR_RGB565:
		eGOPHWClr = E_GOP_FMT_RGB565;
	break;
	case E_DRV_GOP_COLOR_ARGB4444:
		eGOPHWClr = E_GOP_FMT_ARGB4444;
	break;
	case E_DRV_GOP_COLOR_ARGB8888:
		eGOPHWClr = E_GOP_FMT_ARGB8888;
	break;
	case E_DRV_GOP_COLOR_ARGB1555:
		eGOPHWClr = E_GOP_FMT_ARGB1555;
	break;
	case E_DRV_GOP_COLOR_ABGR8888:
		eGOPHWClr = E_GOP_FMT_ABGR8888;
	break;
	case E_DRV_GOP_COLOR_YUV422:
		eGOPHWClr = E_GOP_FMT_YUV422;
	break;
	case E_DRV_GOP_COLOR_RGBA5551:
		eGOPHWClr = E_GOP_FMT_RGBA5551;
	break;
	case E_DRV_GOP_COLOR_RGBA4444:
		eGOPHWClr = E_GOP_FMT_RGBA4444;
	break;
	case E_DRV_GOP_COLOR_RGBA8888:
		eGOPHWClr = E_GOP_FMT_RGBA8888;
	break;
	case E_DRV_GOP_COLOR_BGR565:
		eGOPHWClr = E_GOP_FMT_BGR565;
	break;
	case E_DRV_GOP_COLOR_ABGR4444:
		eGOPHWClr = E_GOP_FMT_ABGR4444;
	break;
	case E_DRV_GOP_COLOR_AYUV8888:
		eGOPHWClr = E_GOP_FMT_AYUV8888;
	break;
	case E_DRV_GOP_COLOR_ABGR1555:
		eGOPHWClr = E_GOP_FMT_ABGR1555;
	break;
	case E_DRV_GOP_COLOR_BGRA5551:
		eGOPHWClr = E_GOP_FMT_BGRA5551;
	break;
	case E_DRV_GOP_COLOR_BGRA4444:
		eGOPHWClr = E_GOP_FMT_BGRA4444;
	break;
	case E_DRV_GOP_COLOR_BGRA8888:
		eGOPHWClr = E_GOP_FMT_BGRA8888;
	break;
	case E_DRV_GOP_COLOR_A2RGB10:
		eGOPHWClr = E_GOP_FMT_A2RGB10;
	break;
	case E_DRV_GOP_COLOR_A2BGR10:
		eGOPHWClr = E_GOP_FMT_A2BGR10;
	break;
	case E_DRV_GOP_COLOR_RGB10A2:
		eGOPHWClr = E_GOP_FMT_RGB10A2;
	break;
	case E_DRV_GOP_COLOR_BGR10A2:
		eGOPHWClr = E_GOP_FMT_BGR10A2;
	break;
	case E_DRV_GOP_COLOR_RGB888:
		eGOPHWClr = E_GOP_FMT_RGB888;
	break;
	default:
		eGOPHWClr = E_GOP_FMT_ARGB8888;
	break;
	}

	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);
	KDrv_HW_GOP_GWIN_SetWinFmt(u8GOP, u8win, (MS_U16)eGOPHWClr);
}

/********************************************************************************/
/// Enable GWIN for display
/// @param u8win \b IN GWIN id
/// @param bEnable \b IN
///   - # TRUE Show GWIN
///   - # FALSE Hide GWIN
/********************************************************************************/
void MDrv_GOP_GWIN_EnableGwin(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win, MS_BOOL bEnable)
{
	MS_U8 u8GOP = 0;

	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (_IsGwinIdValid(pGOPCtx, u8win) == FALSE) {
		DRM_ERROR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return;
	}

	pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bGWINEnable[u8win] = bEnable;
	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);

	KDrv_HW_GOP_GWIN_Enable(u8GOP, u8win, bEnable);
}

static GOP_Result _GOP_GWIN_SetColorMatrix(GOP_OupputColor enGopOutColor,
					   DRV_GOPColorType enColorType,
					   ST_HW_GOP_CSC_TABLE *pstHWGopCscTbl)
{
	switch (enColorType) {
	case E_DRV_GOP_COLOR_YUV422:
	case E_DRV_GOP_COLOR_AYUV8888:
		if (enGopOutColor == DRV_GOPOUT_RGB) {
			pstHWGopCscTbl->u16CscControl = gGopLimitY2RFullBT709[0];
			pstHWGopCscTbl->Matrix[0][0] = gGopLimitY2RFullBT709[1];
			pstHWGopCscTbl->Matrix[0][1] = gGopLimitY2RFullBT709[2];
			pstHWGopCscTbl->Matrix[0][2] = gGopLimitY2RFullBT709[3];
			pstHWGopCscTbl->Matrix[1][0] = gGopLimitY2RFullBT709[4];
			pstHWGopCscTbl->Matrix[1][1] = gGopLimitY2RFullBT709[5];
			pstHWGopCscTbl->Matrix[1][2] = gGopLimitY2RFullBT709[6];
			pstHWGopCscTbl->Matrix[2][0] = gGopLimitY2RFullBT709[7];
			pstHWGopCscTbl->Matrix[2][1] = gGopLimitY2RFullBT709[8];
			pstHWGopCscTbl->Matrix[2][2] = gGopLimitY2RFullBT709[9];
			pstHWGopCscTbl->u16BrightnessOffsetR = GOP_DEFAULT_R_OFFSET;
			pstHWGopCscTbl->u16BrightnessOffsetG = GOP_DEFAULT_G_OFFSET;
			pstHWGopCscTbl->u16BrightnessOffsetB = GOP_DEFAULT_B_OFFSET;
		} else {
			pstHWGopCscTbl->u16CscControl = gGopIdentity[0];
			pstHWGopCscTbl->Matrix[0][0] = gGopIdentity[1];
			pstHWGopCscTbl->Matrix[0][1] = gGopIdentity[2];
			pstHWGopCscTbl->Matrix[0][2] = gGopIdentity[3];
			pstHWGopCscTbl->Matrix[1][0] = gGopIdentity[4];
			pstHWGopCscTbl->Matrix[1][1] = gGopIdentity[5];
			pstHWGopCscTbl->Matrix[1][2] = gGopIdentity[6];
			pstHWGopCscTbl->Matrix[2][0] = gGopIdentity[7];
			pstHWGopCscTbl->Matrix[2][1] = gGopIdentity[8];
			pstHWGopCscTbl->Matrix[2][2] = gGopIdentity[9];
			pstHWGopCscTbl->u16BrightnessOffsetR = GOP_DEFAULT_R_OFFSET;
			pstHWGopCscTbl->u16BrightnessOffsetG = GOP_DEFAULT_G_OFFSET;
			pstHWGopCscTbl->u16BrightnessOffsetB = GOP_DEFAULT_B_OFFSET;
		}
		break;
	case E_DRV_GOP_COLOR_RGB565:
	case E_DRV_GOP_COLOR_ARGB4444:
	case E_DRV_GOP_COLOR_ARGB8888:
	case E_DRV_GOP_COLOR_ARGB1555:
	case E_DRV_GOP_COLOR_ABGR8888:
	case E_DRV_GOP_COLOR_RGBA5551:
	case E_DRV_GOP_COLOR_RGBA4444:
	case E_DRV_GOP_COLOR_RGBA8888:
	case E_DRV_GOP_COLOR_BGR565:
	case E_DRV_GOP_COLOR_ABGR4444:
	case E_DRV_GOP_COLOR_ABGR1555:
	case E_DRV_GOP_COLOR_BGRA5551:
	case E_DRV_GOP_COLOR_BGRA4444:
	case E_DRV_GOP_COLOR_BGRA8888:
	case E_DRV_GOP_COLOR_A2RGB10:
	case E_DRV_GOP_COLOR_A2BGR10:
	case E_DRV_GOP_COLOR_RGB10A2:
	case E_DRV_GOP_COLOR_BGR10A2:
	default:
		if (enGopOutColor == DRV_GOPOUT_YUV) {
			pstHWGopCscTbl->u16CscControl = gGopFullR2YLimitBT709[0];
			pstHWGopCscTbl->Matrix[0][0] = gGopFullR2YLimitBT709[1];
			pstHWGopCscTbl->Matrix[0][1] = gGopFullR2YLimitBT709[2];
			pstHWGopCscTbl->Matrix[0][2] = gGopFullR2YLimitBT709[3];
			pstHWGopCscTbl->Matrix[1][0] = gGopFullR2YLimitBT709[4];
			pstHWGopCscTbl->Matrix[1][1] = gGopFullR2YLimitBT709[5];
			pstHWGopCscTbl->Matrix[1][2] = gGopFullR2YLimitBT709[6];
			pstHWGopCscTbl->Matrix[2][0] = gGopFullR2YLimitBT709[7];
			pstHWGopCscTbl->Matrix[2][1] = gGopFullR2YLimitBT709[8];
			pstHWGopCscTbl->Matrix[2][2] = gGopFullR2YLimitBT709[9];
			pstHWGopCscTbl->u16BrightnessOffsetR = GOP_DEFAULT_R_OFFSET;
			pstHWGopCscTbl->u16BrightnessOffsetG = GOP_DEFAULT_G_OFFSET;
			pstHWGopCscTbl->u16BrightnessOffsetB = GOP_DEFAULT_B_OFFSET;
		} else {
			pstHWGopCscTbl->u16CscControl = gGopIdentity[0];
			pstHWGopCscTbl->Matrix[0][0] = gGopIdentity[1];
			pstHWGopCscTbl->Matrix[0][1] = gGopIdentity[2];
			pstHWGopCscTbl->Matrix[0][2] = gGopIdentity[3];
			pstHWGopCscTbl->Matrix[1][0] = gGopIdentity[4];
			pstHWGopCscTbl->Matrix[1][1] = gGopIdentity[5];
			pstHWGopCscTbl->Matrix[1][2] = gGopIdentity[6];
			pstHWGopCscTbl->Matrix[2][0] = gGopIdentity[7];
			pstHWGopCscTbl->Matrix[2][1] = gGopIdentity[8];
			pstHWGopCscTbl->Matrix[2][2] = gGopIdentity[9];
			pstHWGopCscTbl->u16BrightnessOffsetR = GOP_DEFAULT_R_OFFSET;
			pstHWGopCscTbl->u16BrightnessOffsetG = GOP_DEFAULT_G_OFFSET;
			pstHWGopCscTbl->u16BrightnessOffsetB = GOP_DEFAULT_B_OFFSET;
		}
		break;
	}

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_GWIN_SetGwinInfo(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win,
				     DRV_GOP_GWIN_INFO *pinfo)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	MS_U8 u8GOP;
	GOP_OupputColor enGopOutColor;
	ST_HW_GOP_CSC_TABLE stHWGopCscTbl;

	memset(&stHWGopCscTbl, 0, sizeof(ST_HW_GOP_CSC_TABLE));
	if (_IsGwinIdValid(pGOPCtx, u8win) == FALSE) {
		DRM_ERROR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return GOP_FAIL;
	}
	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);

	switch (u8GOP) {
	case E_GOP0:
		GOP_SetGop0GwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
		break;
	case E_GOP1:
		GOP_SetGop1GwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
		break;
	case E_GOP2:
	case E_GOP3:
	case E_GOP4:
	case E_GOP5:
		GOP_SetGopExtendGwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
		break;
	default:
		break;
	}

	return GOP_SUCCESS;
}

void MDrv_GOP_GWIN_IsGWINEnabled(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win, MS_BOOL *pbEnable)
{
	MS_U8 u8GOP;

	if (_IsGwinIdValid(pGOPCtx, u8win) == FALSE) {
		DRM_ERROR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return;
	}
	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);
	*pbEnable = KDrv_HW_GOP_GWIN_IsGWINEnabled(u8GOP, u8win);
}

void MDrv_GOP_IsGOPMirrorEnable(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_BOOL *bHMirror,
				MS_BOOL *bVMirror)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	KDrv_HW_GOP_IsGOPMirrorEnable(u8GOPNum, bHMirror, bVMirror);

}

void MDrv_GOP_GWIN_EnableHMirror(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_BOOL bEnable)
{
	KDrv_HW_GOP_GWIN_HMirrorEnable(u8GOPNum, bEnable);
}


void MDrv_GOP_GWIN_EnableVMirror(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_BOOL bEnable)
{
	KDrv_HW_GOP_GWIN_VMirrorEnable(u8GOPNum, bEnable);
}

void MDrv_GOP_GWIN_UpdateRegWithSync(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bMl)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (MDrv_GOP_GWIN_IsForceWrite(pGOPCtx, u8GOP) == TRUE)
		GOP_GWIN_TriggerRegWriteIn(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) u8GOP, TRUE, bMl);
	else
		GOP_GWIN_TriggerRegWriteIn(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) u8GOP, FALSE, bMl);

}

void MDrv_GOP_CtrlRst(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bRst)
{
	KDrv_HW_GOP_RstCtrl(u8GOP, bRst);
}

MS_U8 MDrv_GOP_GetWordUnit(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum)
{
	MS_U16 u16GOP_Unit = 0;

	if (pGOPCtx->pGOPCtxShared->bPixelMode[u8GOPNum])
		u16GOP_Unit = 1;
	else
		u16GOP_Unit = GOP_WordUnit;

	return u16GOP_Unit;
}

MS_U8 MDrv_GOP_SelGwinIdByGOP(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_U8 u8GWinIdx)
{
	EN_HW_GOP_GWIN_MODE eHWGopGwinMode = E_HW_GOP_GET_GWIN_ENGINE_ID;

	return KDrv_HW_GOP_GetGwinNum(u8GOP, u8GWinIdx, eHWGopGwinMode);
}

MS_U8 MDrv_GOP_Get_GwinIdByGOP(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP)
{
	EN_HW_GOP_GWIN_MODE eHWGopGwinMode = E_HW_GOP_GET_GWIN_BASEID;

	return KDrv_HW_GOP_GetGwinNum(u8GOP, 0, eHWGopGwinMode);
}

MS_U16 MDrv_GOP_GetBPP(MS_GOP_CTX_LOCAL *pGOPCtx, DRV_GOPColorType fbFmt)
{
	MS_U16 u16Bpp = 0;

	switch (fbFmt) {
	case E_DRV_GOP_COLOR_RGB565:
	case E_DRV_GOP_COLOR_BGR565:
	case E_DRV_GOP_COLOR_ARGB1555:
	case E_DRV_GOP_COLOR_RGBA5551:
	case E_DRV_GOP_COLOR_ARGB4444:
	case E_DRV_GOP_COLOR_ABGR4444:
	case E_DRV_GOP_COLOR_ABGR1555:
	case E_DRV_GOP_COLOR_BGRA5551:
	case E_DRV_GOP_COLOR_BGRA4444:
	case E_DRV_GOP_COLOR_RGBA4444:
	case E_DRV_GOP_COLOR_YUV422:
		u16Bpp = 16;
		break;
	case E_DRV_GOP_COLOR_RGBA8888:
	case E_DRV_GOP_COLOR_BGRA8888:
	case E_DRV_GOP_COLOR_ARGB8888:
	case E_DRV_GOP_COLOR_ABGR8888:
	case E_DRV_GOP_COLOR_A2RGB10:
	case E_DRV_GOP_COLOR_A2BGR10:
	case E_DRV_GOP_COLOR_RGB10A2:
	case E_DRV_GOP_COLOR_BGR10A2:
	case E_DRV_GOP_COLOR_RGB888:
		u16Bpp = 32;
		break;
	default:
		u16Bpp = 0xFFFF;
		break;
	}

	return u16Bpp;
}

#ifdef CONFIG_GOP_AFBC_FEATURE
GOP_Result MDrv_GOP_GWIN_AFBCSetWindow(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
				       DRV_GOP_AFBC_Info *pinfo, MS_BOOL bChangePitch)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	ST_HW_GOP_AFBC_INFO stHWGopAFBCInfo;

	if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bAFBC_Support[u8GOP] == FALSE) {
		DRM_ERROR("[%s] GOP AFBC mode not support GOP %d\n", __func__, u8GOP);
		return GOP_FUN_NOT_SUPPORTED;
	}

	memset(&stHWGopAFBCInfo, 0, sizeof(ST_HW_GOP_AFBC_INFO));
	stHWGopAFBCInfo.bEnable = pinfo->bEnable;
	stHWGopAFBCInfo.u64DRAMAddr = pinfo->u64DRAMAddr;
	stHWGopAFBCInfo.u16HPixelStart = pinfo->u16HPixelStart;
	stHWGopAFBCInfo.u16HPixelEnd = pinfo->u16HPixelEnd;
	stHWGopAFBCInfo.u16VPixelStart = pinfo->u16VPixelStart;
	stHWGopAFBCInfo.u16VPixelEnd = pinfo->u16VPixelEnd;
	KDrv_HW_GOP_SetAFBC(u8GOP, &stHWGopAFBCInfo);

	return GOP_SUCCESS;
}
#endif

GOP_Result MDrv_GOP_GWIN_PowerState(void *pInstance, MS_U32 u32PowerState, void *pModule)
{
	E_HW_POWER_MODE eHWRegMode;

	switch (u32PowerState) {
	case E_POWER_SUSPEND:
		{
			eHWRegMode = E_HW_POWER_SUSPEND;
			KDrv_HW_GOP_GWIN_PowerState(eHWRegMode);
		}
		break;
	case E_POWER_RESUME:
		{
			eHWRegMode = E_HW_POWER_RESUME;
			KDrv_HW_GOP_GWIN_PowerState(eHWRegMode);
		}
		break;
	default:
		break;
	}

	return GOP_SUCCESS;
}


void MDrv_GOP_GWIN_Interrupt(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8Gop, MS_BOOL bEnable)
{
	KDrv_HW_GOP_InterruptEnable(u8Gop, bEnable);
}

GOP_Result MDrv_GOP_SetTestPattern(ST_GOP_TESTPATTERN *pstGOPTestPatInfo)
{
	ST_HW_GOP_PATINFO HWGOPPatInfo;

	memset(&HWGOPPatInfo, 0, sizeof(ST_HW_GOP_PATINFO));
	HWGOPPatInfo.PatEnable = pstGOPTestPatInfo->PatEnable;
	HWGOPPatInfo.HwAutoMode = pstGOPTestPatInfo->HwAutoMode;
	HWGOPPatInfo.DisWidth = pstGOPTestPatInfo->DisWidth;
	HWGOPPatInfo.DisHeight = pstGOPTestPatInfo->DisHeight;
	HWGOPPatInfo.ColorbarW = pstGOPTestPatInfo->ColorbarW;
	HWGOPPatInfo.ColorbarH = pstGOPTestPatInfo->ColorbarH;
	HWGOPPatInfo.EnColorbarMove = pstGOPTestPatInfo->EnColorbarMove;
	HWGOPPatInfo.MoveDir = pstGOPTestPatInfo->MoveDir;
	HWGOPPatInfo.MoveSpeed = pstGOPTestPatInfo->MoveSpeed;

	if (KDrv_HW_GOP_SetTestPattern(&HWGOPPatInfo) == TRUE)
		return GOP_SUCCESS;
	else
		return GOP_FAIL;
}

GOP_Result MDrv_GOP_GetCRC(MS_U32 *CRCvalue)
{
	*CRCvalue = KDrv_HW_GOP_GetCRC();
	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_PrintInfo(MS_U8 GOPNum)
{
	KDrv_HW_GOP_PrintInfo(GOPNum);
	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_SetGOPBypassPath(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bEnable)
{
	MS_BOOL bMixer2RmaEn, bMixer4RmaEn;
	EN_HW_GOP_OUTPUT_MODE GOPOutMode;

	if (bEnable == TRUE) {
		bMixer2RmaEn = FALSE;
		bMixer4RmaEn = FALSE;
		GOPOutMode = E_HW_GOP_OUT_A8RGB10;
	} else {
		bMixer2RmaEn = pGOPCtx->pGOPCtxShared->bMixer2RmaEn;
		bMixer4RmaEn = pGOPCtx->pGOPCtxShared->bMixer4RmaEn;
		switch (pGOPCtx->pGOPCtxShared->GOPOutMode) {
		case E_GOP_OUT_A8RGB10:
			GOPOutMode = E_HW_GOP_OUT_A8RGB10;
		break;
		case E_GOP_OUT_ARGB8888_DITHER:
			GOPOutMode = E_HW_GOP_OUT_ARGB8888_DITHER;
		break;
		case E_GOP_OUT_ARGB8888_ROUND:
			GOPOutMode = E_HW_GOP_OUT_ARGB8888_ROUND;
		break;
		default:
			GOPOutMode = E_HW_GOP_OUT_A8RGB10;
		break;
		}
	}

	if (KDrv_HW_GOP_SetBypassPath(u8GOP, bEnable, bMixer4RmaEn,
		bMixer2RmaEn, GOPOutMode) == TRUE)
		return GOP_SUCCESS;
	else
		return GOP_FAIL;
}

GOP_Result MDrv_GOP_SetTGen(ST_GOP_TGEN_INFO *pGOPTgenInfo)
{
	ST_HW_GOP_TGENINFO stHWGOPTgen;

	memset(&stHWGOPTgen, 0, sizeof(ST_HW_GOP_TGENINFO));
	stHWGOPTgen.Tgen_hs_st = pGOPTgenInfo->Tgen_hs_st;
	stHWGOPTgen.Tgen_hs_end = pGOPTgenInfo->Tgen_hs_end;
	stHWGOPTgen.Tgen_hfde_st = pGOPTgenInfo->Tgen_hfde_st;
	stHWGOPTgen.Tgen_hfde_end = pGOPTgenInfo->Tgen_hfde_end;
	stHWGOPTgen.Tgen_htt = pGOPTgenInfo->Tgen_htt;
	stHWGOPTgen.Tgen_vs_st = pGOPTgenInfo->Tgen_vs_st;
	stHWGOPTgen.Tgen_vs_end = pGOPTgenInfo->Tgen_vs_end;
	stHWGOPTgen.Tgen_vfde_st = pGOPTgenInfo->Tgen_vfde_st;
	stHWGOPTgen.Tgen_vfde_end = pGOPTgenInfo->Tgen_vfde_end;
	stHWGOPTgen.Tgen_vtt = pGOPTgenInfo->Tgen_vtt;

	switch (pGOPTgenInfo->GOPOutMode) {
	case E_GOP_OUT_A8RGB10:
		stHWGOPTgen.GOPOutMode = E_HW_GOP_OUT_A8RGB10;
	break;
	case E_GOP_OUT_ARGB8888_DITHER:
		stHWGOPTgen.GOPOutMode = E_HW_GOP_OUT_ARGB8888_DITHER;
	break;
	case E_GOP_OUT_ARGB8888_ROUND:
		stHWGOPTgen.GOPOutMode = E_HW_GOP_OUT_ARGB8888_ROUND;
	break;
	default:
		stHWGOPTgen.GOPOutMode = E_HW_GOP_OUT_A8RGB10;
	break;
	}

	if (KDrv_HW_GOP_SetTGen(&stHWGOPTgen) == TRUE)
		return GOP_SUCCESS;
	else
		return GOP_FAIL;
}

GOP_Result MDrv_GOP_SetTGenVGSync(MS_BOOL bEnable)
{
	if (KDrv_HW_GOP_SetTGenVGSync(bEnable) == TRUE)
		return GOP_SUCCESS;
	else
		return GOP_FAIL;
}

GOP_Result MDrv_GOP_SetMixerOutMode(MS_BOOL Mixer1_OutPreAlpha, MS_BOOL Mixer2_OutPreAlpha)
{
	if (KDrv_HW_GOP_SetMixerOutMode(Mixer1_OutPreAlpha, Mixer2_OutPreAlpha) == TRUE)
		return GOP_SUCCESS;
	else
		return GOP_FAIL;
}

GOP_Result MDrv_GOP_SetMLBegin(MS_U8 u8GOP)
{
	if (KDrv_HW_GOP_MlBegin(u8GOP))
		return GOP_SUCCESS;
	else
		return GOP_FAIL;
}

GOP_Result MDrv_GOP_SetMLScript(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_U8 MlResIdx)
{
	MS_U8 u8MemIndex = 0;
	MS_U32 ret = 0;
	MS_U32 u32CmdCnt = 0, u32ComCmdCnt = 0;
	MS_U32 fd = pGOPCtx->pGOPCtxShared->u32ML_Fd[MlResIdx];
	struct hwregOut GOPHWout, GOPComHWout;
	struct sm_ml_direct_write_mem_info write_cmd_info;
	struct sm_reg *reg = NULL;
	struct sm_ml_fire_info fire_info;
	int i = 0, j = 0;
	MS_U32 u32_cfd_ml_bufsize =
		pGOPCtx->pGOPCtxShared->stGopPqCfdMlInfo[u8GOP].u32_cfd_ml_bufsize;
	MS_U32 u32_pq_ml_bufsize =
		pGOPCtx->pGOPCtxShared->stGopPqCfdMlInfo[u8GOP].u32_pq_ml_bufsize;
	MS_U8 *tmp = NULL;
	void *pCtx = pGOPCtx->pGOPCtxShared->stGopPQCtx[u8GOP].pCtx;
	MS_GOP_CTX_SHARED *pShare = pGOPCtx->pGOPCtxShared;
	struct ST_PQ_ML_CTX_INFO PQMLCtxInfo;
	struct ST_PQ_ML_CTX_STATUS PQMLCtxStatus;
	struct ST_PQ_NTS_HWREG_INFO PQRegInfo;
	struct ST_PQ_NTS_HWREG_STATUS PQRegState;
	struct ST_PQ_ML_PARAM stMLCtxIn;
	struct ST_PQ_ML_INFO stMLCtxOut;
	enum EN_PQAPI_RESULT_CODES enRet;
	MS_GOP_CTX_SHARED *pGopCtx = pGOPCtx->pGOPCtxShared;
	MS_U16 MlCmdCnt, MlPqCmdCnt;
	MS_U64 MlVa;
	GOP_Result GopRet = GOP_SUCCESS;
	MS_BOOL bCFDUpdated = pShare->stGopPqCfdMlInfo[u8GOP].bCFDUpdated;
	MS_BOOL bPQUpdated = pShare->stGopPqCfdMlInfo[u8GOP].bPQUpdated;

	memset(&GOPHWout, 0, sizeof(struct hwregOut));
	memset(&GOPComHWout, 0, sizeof(struct hwregOut));
	memset(&write_cmd_info, 0, sizeof(struct sm_ml_direct_write_mem_info));
	memset(&fire_info, 0, sizeof(struct sm_ml_fire_info));
	memset(&stMLCtxIn, 0, sizeof(struct ST_PQ_ML_PARAM));
	memset(&stMLCtxOut, 0, sizeof(struct ST_PQ_ML_INFO));

	ret = KDrv_HW_GOP_MlEnd(u8GOP, &GOPHWout, &GOPComHWout);
	u32CmdCnt = GOPHWout.regCount;
	u32ComCmdCnt = GOPComHWout.regCount;

	if (bCFDUpdated || bPQUpdated) {
		memset(&PQMLCtxInfo, 0, sizeof(struct ST_PQ_ML_CTX_INFO));
		memset(&PQMLCtxStatus, 0, sizeof(struct ST_PQ_ML_CTX_STATUS));
		memset(&PQRegInfo, 0, sizeof(struct ST_PQ_NTS_HWREG_INFO));
		memset(&PQRegState, 0, sizeof(struct ST_PQ_NTS_HWREG_STATUS));

		PQMLCtxInfo.u16PqModuleIdx = E_PQ_PIM_MODULE;
		PQMLCtxInfo.pGopAddr = pGOPCtx->pGOPCtxShared->ml_va_address[MlResIdx];
		PQMLCtxInfo.pGopMaxAddr = PQMLCtxInfo.pGopAddr + PIM_GOP_DOMAIN_ML_MAX_SIZE *
			GOP_ML_CMD_SIZE;
		enRet = MApi_PQ_SetMLInfo(pCtx, &PQMLCtxInfo, &PQMLCtxStatus);
		Check_GOP_CfdPQ_ML_Ret("MApi_PQ_SetMLInfo", enRet);
		if (bCFDUpdated) {
			PQRegInfo.eDomain = E_PQ_DOMAIN_GOP;
			for (i = 0; i < (u32_cfd_ml_bufsize/CFD_ML_BUF_ENTRY_SIZE); i++) {
				PQRegInfo.u32RegIdx = pGopCtx->GopCFDMl[u8GOP].stCFD[i].u32RegIdx;
				PQRegInfo.u16Value = pGopCtx->GopCFDMl[u8GOP].stCFD[i].u16Value;
				enRet = MApi_PQ_SetHWReg_NonTs(pCtx, &PQRegInfo, &PQRegState);
				Check_GOP_CfdPQ_ML_Ret("MApi_PQ_SetHWReg_NonTs", enRet);
			}
		}
		if (bPQUpdated) {
			PQRegInfo.eDomain = E_PQ_DOMAIN_GOP;
			for (i = 0; i < (u32_pq_ml_bufsize/CFD_ML_BUF_ENTRY_SIZE); i++) {
				PQRegInfo.u32RegIdx = pGopCtx->GopPQMl[u8GOP].stPQ[i].u32RegIdx;
				PQRegInfo.u16Value = pGopCtx->GopPQMl[u8GOP].stPQ[i].u16Value;
				enRet = MApi_PQ_SetHWReg_NonTs(pCtx, &PQRegInfo, &PQRegState);
				Check_GOP_CfdPQ_ML_Ret("MApi_PQ_SetHWReg_NonTs", enRet);
			}
		}
		stMLCtxIn.u16PqModuleIdx = E_PQ_PIM_MODULE;
		enRet = MApi_PQ_GetMLInfo(pCtx, &stMLCtxIn, &stMLCtxOut);
		Check_GOP_CfdPQ_ML_Ret("MApi_PQ_GetMLInfo", enRet);

		pShare->stGopPqCfdMlInfo[u8GOP].bCFDUpdated = FALSE;
		pShare->stGopPqCfdMlInfo[u8GOP].bPQUpdated = FALSE;
	}

	reg = kmalloc_array((u32CmdCnt + u32ComCmdCnt), sizeof(struct sm_reg), GFP_KERNEL);
	if (reg == NULL) {
		DRM_ERROR("[%s][%d] kmalloc fail !!\n", __func__, __LINE__);
		KDrv_HW_GOP_MlTblFlush(u8GOP);
		return GOP_FAIL;
	}

	for (i = 0; i < GOPHWout.regCount; i++) {
		reg[i].addr = GOPHWout.reg[i].addr;
		reg[i].data = GOPHWout.reg[i].val;
		reg[i].mask = GOPHWout.reg[i].mask;
	}
	for (i = GOPHWout.regCount; i < (GOPHWout.regCount + GOPComHWout.regCount); i++) {
		reg[i].addr = GOPComHWout.reg[j].addr;
		reg[i].data = GOPComHWout.reg[j].val;
		reg[i].mask = GOPComHWout.reg[j].mask;
		j = j + 1;
	}

	MlPqCmdCnt = stMLCtxOut.u16GopUsedCmdSize;
	write_cmd_info.reg = reg;
	write_cmd_info.cmd_cnt = (u32CmdCnt + u32ComCmdCnt);
	write_cmd_info.va_address = pGOPCtx->pGOPCtxShared->ml_va_address[MlResIdx] +
		(MlPqCmdCnt * GOP_ML_CMD_SIZE);
	write_cmd_info.va_max_address = pGOPCtx->pGOPCtxShared->stMlInfo[MlResIdx].start_va +
		pGOPCtx->pGOPCtxShared->stMlInfo[MlResIdx].mem_size;
	write_cmd_info.add_32_support = FALSE;

	DRM_INFO("[GOP][%s,%d]u8GOP:%d, MlPqCmdCnt:%d, MlCmdCnt:%d,(BasicCnt:%d, ComCnt:%d)\n",
		__func__, __LINE__, u8GOP, MlPqCmdCnt,
		write_cmd_info.cmd_cnt, u32CmdCnt, u32ComCmdCnt);

	if (sm_ml_write_cmd(&write_cmd_info) != E_SM_RET_OK) {
		DRM_ERROR("[GOP][%s, %d]: sm_ml_write_cmd failed\n", __func__, __LINE__);
		kfree(reg);
		KDrv_HW_GOP_MlTblFlush(u8GOP);
		return GOP_FAIL;
	}

	MlCmdCnt = write_cmd_info.cmd_cnt;
	MlVa = write_cmd_info.va_address;
	pGopCtx->u16ML_CmdCnd[MlResIdx] += (MlCmdCnt + MlPqCmdCnt);
	pGopCtx->ml_va_address[MlResIdx] = write_cmd_info.va_address;

	kfree(reg);
	KDrv_HW_GOP_MlTblFlush(u8GOP);

	return GopRet;
}

GOP_Result MDrv_GOP_SetAidType(MS_U8 u8GOP, MS_U64 u64GopAidType)
{
	EN_HW_GOP_AID_TYPE eHWAidType;

	eHWAidType = (EN_HW_GOP_AID_TYPE)u64GopAidType;

	if (!KDrv_HW_GOP_SetAidType(u8GOP, eHWAidType)) {
		DRM_ERROR("[GOP][%s, %d]: KDrv_HW_GOP_SetAidType failed\n", __func__, __LINE__);
		return GOP_FAIL;
	}

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_SetVGOrder(EN_GOP_VG_ORDER eVGOrder)
{
	EN_HW_GOP_VG_ORDER eHWVGOrder = E_HW_GOP_VIDEO_OSDB0_OSDB1;

	switch (eVGOrder) {
	case E_GOP_VIDEO_OSDB0_OSDB1:
		eHWVGOrder = E_HW_GOP_VIDEO_OSDB0_OSDB1;
		break;
	case E_GOP_OSDB0_VIDEO_OSDB1:
		eHWVGOrder = E_HW_GOP_OSDB0_VIDEO_OSDB1;
		break;
	case E_GOP_OSDB0_OSDB1_VIDEO:
		eHWVGOrder = E_HW_GOP_OSDB0_OSDB1_VIDEO;
		break;
	default:
		DRM_ERROR("[GOP][%s, %d]: not support VG order:%d\n",
				__func__, __LINE__, eVGOrder);
		eHWVGOrder = E_HW_GOP_VIDEO_OSDB0_OSDB1;
		break;
	}

	if (!KDrv_HW_GOP_SetVGOrder(eVGOrder)) {
		DRM_ERROR("[GOP][%s, %d]: KDrv_HW_GOP_SetVGOrder failed\n",
			__func__, __LINE__);
		return GOP_FAIL;
	}

	return GOP_SUCCESS;
}

MS_U32 MDrv_GOP_ml_fire(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8MlResIdx)
{
	struct sm_ml_fire_info fire_info;
	MS_U32 fd = pGOPCtx->pGOPCtxShared->u32ML_Fd[u8MlResIdx];

	memset(&fire_info, 0, sizeof(struct sm_ml_fire_info));
	fire_info.cmd_cnt = pGOPCtx->pGOPCtxShared->u16ML_CmdCnd[u8MlResIdx];
	fire_info.external = FALSE;
	fire_info.mem_index = pGOPCtx->pGOPCtxShared->u8ML_MemIdx[u8MlResIdx];
	if (sm_ml_fire(fd, &fire_info) != E_SM_RET_OK) {
		DRM_ERROR("[GOP][%s, %d]: sm_ml_fire failed\n", __func__, __LINE__);
		return GOP_FAIL;
	}

	pGOPCtx->pGOPCtxShared->u16ML_CmdCnd[u8MlResIdx] = 0;

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_ml_get_mem_info(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8MlResIdx)
{
	MS_U8 u8MemIndex = 0;
	MS_U32 fd = pGOPCtx->pGOPCtxShared->u32ML_Fd[u8MlResIdx];

	if (sm_ml_get_mem_index(fd, &u8MemIndex, 0) != E_SM_RET_OK) {
		pr_err("[GOP][%s, %d]: sm_ml_get_mem_index failed\n", __func__, __LINE__);
		return GOP_FAIL;
	}

	if (sm_ml_get_info(fd, u8MemIndex,
	&pGOPCtx->pGOPCtxShared->stMlInfo[u8MlResIdx]) != E_SM_RET_OK) {
		pr_err("[GOP][%s, %d]: sm_ml_get_info failed\n", __func__, __LINE__);
		return GOP_FAIL;
	}

	pGOPCtx->pGOPCtxShared->u8ML_MemIdx[u8MlResIdx] = u8MemIndex;
	pGOPCtx->pGOPCtxShared->ml_va_address[u8MlResIdx] =
		pGOPCtx->pGOPCtxShared->stMlInfo[u8MlResIdx].start_va;

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_SetADL(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP)
{
	MS_U8 *adlbuf_addr = pGOPCtx->pGOPCtxShared->GopAdl[u8GOP].AdlBuf;
	uint8_t u8MemIndex = 0;
	MS_U32 adl_bufsize = pGOPCtx->pGOPCtxShared->stGOPAdlInfo[u8GOP].adl_bufsize;
	MS_U32 fd = pGOPCtx->pGOPCtxShared->stGOPAdlInfo[u8GOP].fd;
	struct sm_adl_add_info stdatainfo;
	struct sm_adl_fire_info stFireinfo;
	int i = 0;

	if (pGOPCtx->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP]) {
		if (adlbuf_addr != NULL) {
			memset(&stdatainfo, 0, sizeof(struct sm_adl_add_info));
			memset(&stFireinfo, 0, sizeof(struct sm_adl_fire_info));
			if (sm_adl_get_mem_index(fd, &u8MemIndex) != E_SM_RET_OK) {
				DRM_ERROR("[GOP][%s, %d]: sm_adl_get_mem_index failed.\n",
				__func__, __LINE__);
				pGOPCtx->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP] = FALSE;
				return GOP_FAIL;
			}

			stdatainfo.sub_client = E_SM_ADL_XVYCC_SUB_DEGAMMA;
			stdatainfo.external = false;
			stdatainfo.data = adlbuf_addr;
			stdatainfo.dataSize = adl_bufsize;
			if (sm_adl_add_cmd(fd, &stdatainfo) != E_SM_RET_OK) {
				DRM_ERROR("[GOP][%s, %d]: sm_adl_add_cmd failed.\n",
				__func__, __LINE__);
				pGOPCtx->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP] = FALSE;
				return GOP_FAIL;
			}

			stFireinfo.external = FALSE;
			stFireinfo.mem_index = u8MemIndex;
			stFireinfo.cmdcnt = adl_bufsize;
			if (sm_adl_fire(pGOPCtx->pGOPCtxShared->stGOPAdlInfo[u8GOP].fd,
				&stFireinfo) != E_SM_RET_OK) {
				DRM_ERROR("[GOP][%s, %d]: sm_adl_fire failed.\n",
				__func__, __LINE__);
				pGOPCtx->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP] = FALSE;
				return GOP_FAIL;
			}

			pGOPCtx->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP] = FALSE;

			//read degamma & gamma sram
			//MS_U8 *u8degammval, *u8gammval;
			//u8degammval = kmalloc_array(12288, sizeof(MS_U8), GFP_KERNEL);
			//KDrv_HW_GOP_XVYCC_readSram_degamma(u8GOP, u8degammval);

			//u8gammval = kmalloc_array(2064, sizeof(MS_U8), GFP_KERNEL);
			//KDrv_HW_GOP_XVYCC_readSram_gamma(u8GOP, u8gammval);
			//kfree(degamma);
			//kfree(u8gammval);
		} else {
			pGOPCtx->pGOPCtxShared->bIsGOPAdlUpdated[u8GOP] = FALSE;
		}
	}

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_GetRoi(ST_GOP_GETROI *pGOPRoi)
{
	ST_HW_GOP_GETROI stGOPRoi;
	MS_U64 u64RoiRet = 0;

	memset(&stGOPRoi, 0, sizeof(ST_HW_GOP_GETROI));
	stGOPRoi.bRetTotal = pGOPRoi->bRetTotal;
	stGOPRoi.gopIdx = pGOPRoi->gopIdx;
	stGOPRoi.MainVRate = pGOPRoi->MainVRate;
	stGOPRoi.Threshold = pGOPRoi->Threshold;

	u64RoiRet = KDrv_HW_GOP_GetROI(&stGOPRoi);
	pGOPRoi->RetCount = u64RoiRet;

	return GOP_SUCCESS;
}
