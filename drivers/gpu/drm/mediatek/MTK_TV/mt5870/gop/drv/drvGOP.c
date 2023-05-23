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
#include "mdrv_gop.h"
#include "common/hwreg_common_riu.h"
#include "drvGOP.h"
#include "drvGFLIP.h"
#include "drvGOP_priv.h"

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

#define GOP_TIMEOUT_CNT_OS              22UL

#define GOP_TIMEOUT_CNT_OS_YIELD	0x10000UL
#define GOP_TIMEOUT_CNT_NOS	0x100000UL

#define PALETTE_BY_REGDMA               1UL
#define msWarning(c)                    do {} while (0)
#define msFatal(c)                      do {} while (0)
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
//=============================================================
// Debug Log
#ifdef STI_PLATFORM_BRING_UP
#define GOP_D_INFO(x, args...)
// Warning, illegal parameter but can be self fixed in functions
#define GOP_D_WARN(x, args...)
//  Need debug, illegal parameter.
#define GOP_D_DBUG(x, args...)
// Error, function will be terminated but system not crash
#define GOP_D_ERR(x, args...)
// Critical, system crash. (ex. assert)
#define GOP_D_FATAL(x, args...)
#else
#include "ULog.h"
MS_U32 u32GOPDbgLevel_drv = E_GOP_Debug_Level_LOW;
#ifdef CONFIG_GOP_DEBUG_LEVEL
// Debug Logs, level form low(INFO) to high(FATAL, always show)
// Function information, ex function entry
#define GOP_D_INFO(x, args...) do {\
	if (u32GOPDbgLevel_drv >= E_GOP_Debug_Level_HIGH) {\
		ULOGI("GOP DRV", x, ##args);\
	} \
} while (0)
// Warning, illegal parameter but can be self fixed in functions
#define GOP_D_WARN(x, args...) do {\
	if (u32GOPDbgLevel_drv >= E_GOP_Debug_Level_HIGH) {\
		ULOGW("GOP DRV", x, ##args);\
	} \
} while (0)
//  Need debug, illegal parameter.
#define GOP_D_DBUG(x, args...) do {\
	if (u32GOPDbgLevel_drv >= E_GOP_Debug_Level_MED) {\
		ULOGD("GOP DRV", x, ##args);\
	} \
} while (0)
// Error, function will be terminated but system not crash
#define GOP_D_ERR(x, args...) {ULOGE("GOP DRV", x, ##args); }
// Critical, system crash. (ex. assert)
#define GOP_D_FATAL(x, args...) {ULOGF("GOP DRV", x, ##args); }
#else
#define GOP_D_INFO(x, args...)
// Warning, illegal parameter but can be self fixed in functions
#define GOP_D_WARN(x, args...)
//  Need debug, illegal parameter.
#define GOP_D_DBUG(x, args...)
// Error, function will be terminated but system not crash
#define GOP_D_ERR(x, args...)
// Critical, system crash. (ex. assert)
#define GOP_D_FATAL(x, args...)
#endif
#endif
//=============================================================

// Define return values of check align
#define CHECKALIGN_SUCCESS              1UL
#define CHECKALIGN_FORMAT_FAIL          2UL
#define CHECKALIGN_PARA_FAIL            3UL

#define GOP_ASSERT(x) {printf("error in %s:%d\n", __func__, __LINE__); }

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

MS_U16 _GopVStretchTable[MAX_GOP_VSTRETCH_MODE_SUPPORT][GOP_VSTRETCH_TABLE_NUM] = {
	//liner
	{0x14, 0x3c, 0x13, 0x3d,
	 0x11, 0x3f, 0x10, 0x00,
	 0x0e, 0x02, 0x0b, 0x05,
	 0x09, 0x07, 0x08, 0x08,
	 0x14, 0x3c, 0x13, 0x3d,
	 0x11, 0x3f, 0x10, 0x00,
	 0x0e, 0x02, 0x0b, 0x05,
	 0x09, 0x07, 0x08, 0x08,
	 },
	//
	{0x0f, 0x01, 0x0e, 0x02,
	 0x0d, 0x03, 0x0c, 0x04,
	 0x0b, 0x05, 0x0a, 0x06,
	 0x09, 0x07, 0x08, 0x08,
	 0x0f, 0x01, 0x0e, 0x02,
	 0x0d, 0x03, 0x0c, 0x04,
	 0x0b, 0x05, 0x0a, 0x06,
	 0x09, 0x07, 0x08, 0x08,
	 },
	//
	{0x0f, 0x01, 0x0e, 0x02,
	 0x0d, 0x03, 0x0c, 0x04,
	 0x0b, 0x05, 0x0a, 0x06,
	 0x09, 0x07, 0x08, 0x08,
	 0x0f, 0x01, 0x0e, 0x02,
	 0x0d, 0x03, 0x0c, 0x04,
	 0x0b, 0x05, 0x0a, 0x06,
	 0x09, 0x07, 0x08, 0x08,
	 },
	//liner Gain0
	{0x0f, 0x01, 0x0e, 0x02,
	 0x0d, 0x03, 0x0c, 0x04,
	 0x0b, 0x05, 0x0a, 0x06,
	 0x09, 0x07, 0x08, 0x08,
	 0x0f, 0x01, 0x0e, 0x02,
	 0x0d, 0x03, 0x0c, 0x04,
	 0x0b, 0x05, 0x0a, 0x06,
	 0x09, 0x07, 0x08, 0x08,
	 },
	//liner Gain1
	{0x10, 0x00, 0x0F, 0x01,
	 0x0E, 0x02, 0x0D, 0x03,
	 0x0C, 0x04, 0x0B, 0x05,
	 0x0A, 0x06, 0x08, 0x08,
	 0x10, 0x00, 0x0F, 0x01,
	 0x0E, 0x02, 0x0D, 0x03,
	 0x0C, 0x04, 0x0B, 0x05,
	 0x0A, 0x06, 0x08, 0x08,
	 },
	//linear Gain2
	{0x10, 0x00, 0x0F, 0x01,
	 0x0E, 0x02, 0x0D, 0x03,
	 0x0C, 0x04, 0x0B, 0x05,
	 0x0A, 0x06, 0x08, 0x08,
	 0x10, 0x00, 0x0F, 0x01,
	 0x0E, 0x02, 0x0D, 0x03,
	 0x0C, 0x04, 0x0B, 0x05,
	 0x0A, 0x06, 0x08, 0x08,
	 },
	// 4-tap default coef
	{0x78, 0x04, 0x6c, 0xf6,
	 0xfb, 0x23, 0x4a, 0xf6},
	// 4-tap 100 coef
	{0x80, 0x00, 0x6E, 0xF9,
	 0xFF, 0x1A, 0x45, 0xFB}
};

MS_U16 _GopHStretchTable[MAX_GOP_HSTRETCH_MODE_SUPPORT][GOP_STRETCH_TABLE_NUM] = {

	//6-tap Default
	{0x03, 0x01, 0x16, 0x01, 0x03,
	 0x00, 0x03, 0x02, 0x16, 0x04,
	 0x04, 0x01, 0x02, 0x03, 0x13,
	 0x07, 0x04, 0x01, 0x02, 0x04,
	 0x10, 0x0b, 0x04, 0x01, 0x03,
	 0x01, 0x16, 0x01, 0x03, 0x00,
	 0x03, 0x02, 0x16, 0x04, 0x04,
	 0x01, 0x02, 0x03, 0x13, 0x07,
	 0x04, 0x01, 0x02, 0x04, 0x10,
	 0x0b, 0x04, 0x01, 0x00, 0x01},

	//Duplicate ->Set as default
	{0x03, 0x01, 0x16, 0x01, 0x03,
	 0x00, 0x03, 0x02, 0x16, 0x04,
	 0x04, 0x01, 0x02, 0x03, 0x13,
	 0x07, 0x04, 0x01, 0x02, 0x04,
	 0x10, 0x0b, 0x04, 0x01, 0x03,
	 0x01, 0x16, 0x01, 0x03, 0x00,
	 0x03, 0x02, 0x16, 0x04, 0x04,
	 0x01, 0x02, 0x03, 0x13, 0x07,
	 0x04, 0x01, 0x02, 0x04, 0x10,
	 0x0b, 0x04, 0x01, 0x00, 0x01},

	//6-tap Linear
	{0x00, 0x00, 0x10, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x0E, 0x02,
	 0x00, 0x00, 0x00, 0x00, 0x0C,
	 0x04, 0x00, 0x00, 0x00, 0x00,
	 0x09, 0x07, 0x00, 0x00, 0x00,
	 0x00, 0x10, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x0e, 0x02, 0x00,
	 0x00, 0x00, 0x00, 0x0c, 0x04,
	 0x00, 0x00, 0x00, 0x00, 0x09,
	 0x07, 0x00, 0x00, 0x00, 0x00},

	//6-tap Nearest
	{0x00, 0x00, 0x10, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x10, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x10,
	 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x10, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x10, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x10, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x10, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x10,
	 0x00, 0x00, 0x00, 0x00, 0x00},

	//6-tap Gain0
	{0x00, 0x01, 0x10, 0x01, 0x00,
	 0x00, 0x00, 0x02, 0x0f, 0x04,
	 0x01, 0x00, 0x01, 0x02, 0x0d,
	 0x05, 0x01, 0x00, 0x00, 0x02,
	 0x0c, 0x08, 0x02, 0x00, 0x00,
	 0x01, 0x10, 0x01, 0x00, 0x00,
	 0x00, 0x02, 0x0f, 0x04, 0x01,
	 0x00, 0x01, 0x02, 0x0d, 0x05,
	 0x01, 0x00, 0x00, 0x02, 0x0c,
	 0x08, 0x02, 0x00, 0x00, 0x00},

	//6-tap Gain1
	{0x02, 0x01, 0x14, 0x01, 0x02,
	 0x00, 0x02, 0x02, 0x12, 0x05,
	 0x03, 0x00, 0x00, 0x03, 0x10,
	 0x06, 0x02, 0x01, 0x01, 0x03,
	 0x0e, 0x0a, 0x03, 0x01, 0x02,
	 0x01, 0x14, 0x01, 0x02, 0x00,
	 0x02, 0x02, 0x12, 0x05, 0x03,
	 0x00, 0x00, 0x03, 0x10, 0x06,
	 0x02, 0x01, 0x01, 0x03, 0x0e,
	 0x0a, 0x03, 0x01, 0x00, 0x00},

	//6-tap Gain2
	{0x03, 0x01, 0x16, 0x01, 0x03,
	 0x00, 0x03, 0x02, 0x16, 0x04,
	 0x04, 0x01, 0x02, 0x03, 0x13,
	 0x07, 0x04, 0x01, 0x02, 0x04,
	 0x10, 0x0b, 0x04, 0x01, 0x03,
	 0x01, 0x16, 0x01, 0x03, 0x00,
	 0x03, 0x02, 0x16, 0x04, 0x04,
	 0x01, 0x02, 0x03, 0x13, 0x07,
	 0x04, 0x01, 0x02, 0x04, 0x10,
	 0x0b, 0x04, 0x01, 0x00, 0x00},

	//6-tap Gain3
	{0x05, 0x01, 0x19, 0x02, 0x05,
	 0x00, 0x04, 0x03, 0x18, 0x06,
	 0x06, 0x01, 0x03, 0x04, 0x15,
	 0x08, 0x04, 0x02, 0x04, 0x04,
	 0x13, 0x0d, 0x06, 0x02, 0x05,
	 0x01, 0x19, 0x02, 0x05, 0x00,
	 0x04, 0x03, 0x18, 0x06, 0x06,
	 0x01, 0x03, 0x04, 0x15, 0x08,
	 0x04, 0x02, 0x04, 0x04, 0x13,
	 0x0d, 0x06, 0x02, 0x00, 0x00},

	//6-tap Gain4
	{0x00, 0x01, 0x11, 0x00, 0x00,
	 0x00, 0x00, 0x02, 0x0f, 0x04,
	 0x01, 0x00, 0x00, 0x01, 0x0d,
	 0x05, 0x01, 0x00, 0x00, 0x02,
	 0x0c, 0x08, 0x02, 0x00, 0x03,
	 0x01, 0x16, 0x01, 0x03, 0x00,
	 0x03, 0x02, 0x16, 0x04, 0x04,
	 0x01, 0x02, 0x03, 0x13, 0x07,
	 0x04, 0x01, 0x02, 0x04, 0x10,
	 0x0b, 0x04, 0x01, 0x00, 0x00},

	//6-tap Gain5
	{0x00, 0x00, 0x10, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x0E, 0x02,
	 0x00, 0x00, 0x00, 0x00, 0x0C,
	 0x04, 0x00, 0x00, 0x00, 0x00,
	 0x08, 0x08, 0x00, 0x00, 0x00,
	 0x00, 0x10, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x0E, 0x02, 0x00,
	 0x00, 0x00, 0x00, 0x0C, 0x04,
	 0x00, 0x00, 0x00, 0x00, 0x08,
	 0x08, 0x00, 0x00, 0x00, 0x00},
	//4-tap ->Set as default
	{0x03, 0x01, 0x16, 0x01, 0x03,
	 0x00, 0x03, 0x02, 0x16, 0x04,
	 0x04, 0x01, 0x02, 0x03, 0x13,
	 0x07, 0x04, 0x01, 0x02, 0x04,
	 0x10, 0x0b, 0x04, 0x01, 0x03,
	 0x01, 0x16, 0x01, 0x03, 0x00,
	 0x03, 0x02, 0x16, 0x04, 0x04,
	 0x01, 0x02, 0x03, 0x13, 0x07,
	 0x04, 0x01, 0x02, 0x04, 0x10,
	 0x0b, 0x04, 0x01, 0x00, 0x01},
	//2-tap
	{0x00, 0x00, 0x10, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x08, 0x08, 0x00, 0x00, 0x00,
	 0x00, 0x10, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x08,
	 0x08, 0x00, 0x00, 0x00, 0x00},
	//4-tap coef of 45
	{0x44, 0x1E, 0x39, 0x16,
	 0x07, 0x2A, 0x33, 0x0d},
	//4-tap coef of 50
	{0x44, 0x1E, 0x3E, 0x13,
	 0x04, 0x2B, 0x36, 0x0A},
	//4-tap coef of 55
	{0x46, 0x1D, 0x44, 0x10,
	 0x01, 0x2B, 0x39, 0x07},
	//4-tap coef of 65
	{0x4E, 0x19, 0x4D, 0x0A,
	 0xFD, 0x2C, 0x3F, 0x01},
	//4-tap coef of 75
	{0x58, 0x14, 0x58, 0x03,
	 0xFA, 0x2B, 0x45, 0xFB},
	//4-tap coef of 85
	{0x62, 0x0F, 0x63, 0xFC,
	 0xF8, 0x29, 0x49, 0xF7},
	//4-tap coef of 95
	{0x78, 0x04, 0x6C, 0xF6,
	 0xFB, 0x23, 0x4A, 0xF6},
	//4-tap 105 coef
	{0x88, 0xFC, 0x75, 0xF2,
	 0xFE, 0x1B, 0x49, 0xF7},
	//tap 100 coef
	{0x80, 0x00, 0x6E, 0xF9,
	 0xFF, 0x1A, 0x45, 0xFB}
};

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

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
MS_U32 DTBArray2Bool(struct device_node *np, const char *name,
		     MS_BOOL *ret_array, int ret_array_length)
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
		*(ret_array + i) = (MS_BOOL) u32TmpArray[i];

	kfree(u32TmpArray);
	return 0;
}

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
			pu32array = kmalloc(sizeof(MS_U32) * 4, GFP_KERNEL);
			ret = DTBArray(np, name, pu32array, 4);
			RIUBase1 = ioremap(pu32array[0], pu32array[1]);
			RIUBase2 = ioremap(pu32array[2], pu32array[3]);
			g_GopChipPro.VAdr[0] = (MS_VIRT *)RIUBase1;
			g_GopChipPro.VAdr[1] = (MS_VIRT *)RIUBase2;
			g_GopChipPro.PAdr[0] = (pu32array[0] - 0x1c000000);
			g_GopChipPro.PAdr[1] = (pu32array[2] - 0x1c000000);
			g_GopChipPro.u32RegSize[0] = pu32array[1];
			g_GopChipPro.u32RegSize[1] = pu32array[3];
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

static MS_BOOL _IsMuxSelVaild(MS_GOP_CTX_LOCAL *pDrvCtx, MS_U8 u8GopNum)
{
	if (u8GopNum >= pDrvCtx->pGopChipProperty->u8OpMuxNum)
		return FALSE;
	else
		return TRUE;
}

static MS_BOOL _GetGOPAckDelayTimeAndCnt(MS_U32 *pu32DelayTimems, MS_U32 *pu32TimeoutCnt)
{
	if ((pu32DelayTimems == NULL) || (pu32TimeoutCnt == NULL)) {
		GOP_ASSERT(FALSE);
		return FALSE;
	}
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
#if GOP_VSYNC_WAIT_BYSLEEP
	*pu32DelayTimems = 1;
	*pu32TimeoutCnt = GOP_TIMEOUT_CNT_OS;
#else
	*pu32DelayTimems = 1;
	*pu32TimeoutCnt = GOP_TIMEOUT_CNT_OS_YIELD;
#endif
#else
	*pu32DelayTimems = 0;
	*pu32TimeoutCnt = GOP_TIMEOUT_CNT_NOS;
#endif
	return TRUE;
}

static MS_BOOL _MDrv_GOP_CSC_ParamInit(ST_GOP_CSC_PARAM *pstCSCParam)
{
	if (pstCSCParam == NULL)
		return FALSE;

	pstCSCParam->u32Version = ST_GOP_CSC_PARAM_VERSION;
	pstCSCParam->u32Length = sizeof(ST_GOP_CSC_PARAM);
	pstCSCParam->bCscEnable = FALSE;
	pstCSCParam->bUpdateWithVsync = FALSE;
	pstCSCParam->u16Hue = 50;
	pstCSCParam->u16Saturation = 128;
	pstCSCParam->u16Contrast = 1024;
	pstCSCParam->u16Brightness = 1024;
	pstCSCParam->u16RGBGGain[0] = 1024;
	pstCSCParam->u16RGBGGain[1] = 1024;
	pstCSCParam->u16RGBGGain[2] = 1024;
	pstCSCParam->enInputFormat = E_GOP_CFD_CFIO_MAX;
	pstCSCParam->enInputDataFormat = E_GOP_CFD_MC_FORMAT_MAX;
	pstCSCParam->enInputRange = E_GOP_CFD_CFIO_RANGE_FULL;
	pstCSCParam->enOutputFormat = E_GOP_CFD_CFIO_MAX;
	pstCSCParam->enOutputDataFormat = E_GOP_CFD_MC_FORMAT_MAX;
	pstCSCParam->enOutputRange = E_GOP_CFD_CFIO_RANGE_FULL;
	return TRUE;
}

static MS_BOOL _MDrv_GOP_CSC_TableInit(ST_GOP_CSC_TABLE *pstCSCTable)
{
	if (pstCSCTable == NULL)
		return FALSE;

	memset(pstCSCTable, 0, sizeof(ST_GOP_CSC_TABLE));

	return TRUE;
}

static DRV_GOPColorType _MDrv_GOP_ConvertHWFmt(MS_U16 u16HwClrType)
{
	DRV_GOPColorType drvClrType;

	switch (u16HwClrType) {
	case E_GOP_FMT_RGB565:
		drvClrType = E_DRV_GOP_COLOR_RGB565;
	break;
	case E_GOP_FMT_ARGB1555:
		drvClrType = E_DRV_GOP_COLOR_ARGB1555;
	break;
	case E_GOP_FMT_ARGB4444:
		drvClrType = E_DRV_GOP_COLOR_ARGB4444;
	break;
	case E_GOP_FMT_YUV422:
		drvClrType = E_DRV_GOP_COLOR_YUV422;
	break;
	case E_GOP_FMT_ARGB8888:
		drvClrType = E_DRV_GOP_COLOR_ARGB8888;
	break;
	case E_GOP_FMT_RGBA5551:
		drvClrType = E_DRV_GOP_COLOR_RGBA5551;
	break;
	case E_GOP_FMT_RGBA4444:
		drvClrType = E_DRV_GOP_COLOR_RGBA4444;
	break;
	case E_GOP_FMT_BGRA5551:
		drvClrType = E_DRV_GOP_COLOR_BGRA5551;
	break;
	case E_GOP_FMT_ABGR1555:
		drvClrType = E_DRV_GOP_COLOR_ABGR1555;
	break;
	case E_GOP_FMT_ABGR4444:
		drvClrType = E_DRV_GOP_COLOR_ABGR4444;
	break;
	case E_GOP_FMT_BGRA4444:
		drvClrType = E_DRV_GOP_COLOR_BGRA4444;
	break;
	case E_GOP_FMT_BGR565:
		drvClrType = E_DRV_GOP_COLOR_BGR565;
	break;
	case E_GOP_FMT_RGBA8888:
		drvClrType = E_DRV_GOP_COLOR_RGBA8888;
	break;
	case E_GOP_FMT_BGRA8888:
		drvClrType = E_DRV_GOP_COLOR_BGRA8888;
	break;
	case E_GOP_FMT_ABGR8888:
		drvClrType = E_DRV_GOP_COLOR_ABGR8888;
	break;
	case E_GOP_FMT_AYUV8888:
		drvClrType = E_DRV_GOP_COLOR_AYUV8888;
	break;
	default:
		drvClrType = E_DRV_GOP_COLOR_ARGB8888;
	break;
	}

	return drvClrType;
}


void GOP_GWIN_TriggerRegWriteIn(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, E_DRV_GOP_TYPE u8GopType,
				MS_BOOL bForceWriteIn, MS_BOOL bSync)
{
	MS_U16 u16GopAck = 0;
	MS_U32 goptimeout = 0;
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

	if (pGOPDrvLocalCtx->bGOPBankFwr[u8GopType]) {
		/*Defination use for warning user the bnkForceWrite function support or not. */
		if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bBnkForceWrite) {
			KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, TRUE, E_HW_GOP_BNKFORCEWRITE);
			KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, FALSE, E_HW_GOP_BNKFORCEWRITE);
		} else {
			GOP_D_DBUG("[%s][%d]  GOP%d not support BankForceWrite\n", __func__,
				   __LINE__, u8GopType);
		}
	} else if (bForceWriteIn) {
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, TRUE, E_HW_GOP_FORCEWRITE);
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, FALSE, E_HW_GOP_FORCEWRITE);
	} else {
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, TRUE, E_HW_GOP_WITHVSYNC);

		if (bSync == TRUE) {
			MS_U32 u32DelayTimems = 0;
			MS_U32 u32TimeoutCnt = 0;

			_GetGOPAckDelayTimeAndCnt(&u32DelayTimems, &u32TimeoutCnt);
			do {
				goptimeout++;
				u16GopAck = (MS_U16) KDrv_HW_GOP_GetRegUpdated(u8GOP);
				if (u32DelayTimems != 0) {
#if GOP_VSYNC_WAIT_BYSLEEP
					MsOS_DelayTask(u32DelayTimems);	//delay 1 ms
#else
					MsOS_YieldTask();	//Customer request
#endif
				}
			} while ((!u16GopAck) && (goptimeout <= u32TimeoutCnt));

			// Perform force write if wr timeout.
			if (goptimeout > u32TimeoutCnt) {
				GOP_D_INFO("[%s][%d]Perform fwr if wr timeout!!\n", __func__,
					   __LINE__);
				if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bBnkForceWrite) {
					KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, TRUE,
									   E_HW_GOP_BNKFORCEWRITE);
					KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, FALSE,
									   E_HW_GOP_BNKFORCEWRITE);
				} else {
					KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, TRUE,
									   E_HW_GOP_FORCEWRITE);
					KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u8GOP, FALSE,
									   E_HW_GOP_FORCEWRITE);
				}
			}
		}
	}
}

void GOP_GWIN_UpdateReg(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, E_DRV_GOP_TYPE u8GopType)
{
	if ((pGOPDrvLocalCtx->apiCtxLocal.bUpdateRegOnce[GOP_PUBLIC_UPDATE] == FALSE)
	    && (pGOPDrvLocalCtx->apiCtxLocal.bUpdateRegOnce[u8GopType]) == FALSE) {
		if (MDrv_GOP_GWIN_IsForceWrite((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GopType) ==
		    TRUE) {
			GOP_GWIN_TriggerRegWriteIn(pGOPDrvLocalCtx, u8GopType, TRUE, TRUE);
		} else {
			GOP_GWIN_TriggerRegWriteIn(pGOPDrvLocalCtx, u8GopType, FALSE, TRUE);
		}
	}
}

static MS_U32 _GOP_GWIN_AlignChecker(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_U16 *width,
				     MS_U16 fmtbpp)
{
	MS_U16 alignfactor = 0;

	if (pGOPCtx->pGOPCtxShared->bPixelMode[u8GOPNum] == TRUE) {
		alignfactor = 1;
	} else {
		alignfactor =
		    (MS_U16) (((MS_U16) MDrv_GOP_GetWordUnit(pGOPCtx, u8GOPNum)) / (fmtbpp >> 3));
	}

	if ((alignfactor != 0) && (*width % alignfactor != 0)) {
		GOP_D_ERR("\n\n%s, This FB format needs to %d-pixels alignment !!!\n\n",
			  __func__, alignfactor);
		return CHECKALIGN_PARA_FAIL;
	} else {
		return CHECKALIGN_SUCCESS;
	}
}

static MS_U32 _GOP_GWIN_2PEngineAlignChecker(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					     MS_PHY *width)
{
	MS_U16 alignfactor = 0;

	if (pGOPCtx->pGopChipProperty->b2Pto1PSupport == TRUE) {
		alignfactor = 2;

		if ((alignfactor != 0) && (*width % alignfactor != 0)) {
			GOP_D_ERR("\n\n%s, Not mach to %d-pixels alignment !!!\n\n", __func__,
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
	} else {
		alignfactor = (MS_U32) ((MDrv_GOP_GetWordUnit(pGOPCtx, u8GOPNum)) & 0xFF);
	}

	if ((Address & (alignfactor - 1)) != 0) {
		GOP_D_ERR("%s,%d FB=0x%tx address need %td-bytes aligned!\n", __func__,
			  __LINE__, (ptrdiff_t) Address, (ptrdiff_t) alignfactor);
		return CHECKALIGN_PARA_FAIL;
	} else {
		return CHECKALIGN_SUCCESS;
	}
}

void GOP_SetGopGwinHVPixel(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win, MS_U8 u8GOP,
			   DRV_GOP_GWIN_INFO *pinfo)
{
	MS_U16 u8Bpp;
	MS_U16 align_start, align_end, u16VEnd;
	MS_U16 u16StretchWidth = 0, u16StretchHeight = 0, u16x = 0, u16y = 0;

	u8Bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (u8Bpp == FB_FMT_AS_DEFAULT) {
		GOP_D_DBUG("[%s] [%d]  GOP not support the color format = %d\n", __func__,
			   __LINE__, pinfo->clrType);
	}
	u16VEnd = pinfo->u16DispVPixelEnd;

	if (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bPixelMode[u8GOP] == TRUE) {
		align_start = (pinfo->u16DispHPixelStart);
		align_end = (pinfo->u16DispHPixelEnd);
	} else {
		align_start = (pinfo->u16DispHPixelStart) * (u8Bpp >> 3) / (GOP_WordUnit);
		align_end = (pinfo->u16DispHPixelEnd) * (u8Bpp >> 3) / (GOP_WordUnit);
	}

	KDrv_HW_GOP_GetGopStretchWin(u8GOP, &u16x, &u16y, &u16StretchWidth, &u16StretchHeight);

	if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
		GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_3M_SERIES_MAJOR)
		u16StretchWidth *= GOP_STRETCH_WIDTH_UNIT;

	if (align_end > u16StretchWidth) {
		GOP_D_DBUG
		    ("[GOP]%s,%s,%d GWIN Hend > StretchWidth!!!align_end=%d,u16StretchWidth=%d\n",
		     __FILE__, __func__, __LINE__, (int)align_end, (int)u16StretchWidth);
		align_end = u16StretchWidth;
	}

	KDrv_HW_GOP_SetGopGwinHVPixel(u8GOP, u8win, align_start, align_end,
				      pinfo->u16DispVPixelStart, u16VEnd);
}


static void GOP_SetGopExtendGwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				     DRV_GOP_GWIN_INFO *pinfo)
{
	MS_PHY phyTmp = 0;
	MS_U16 u16Bpp = 0;
	MS_U16 u16GOP_Unit = 0;
	MS_U8 u8GOP = 0, u8GOPIdx = 0;
	MS_U16 u16Pitch = 0;
	MS_U8 u8MiuSel = 0;
	MS_PHY Width = 0;
	MS_U8 u8win_temp = 0;

	if (_IsGwinIdValid((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8win) == FALSE) {
		GOP_D_ERR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return;
	}

	u8GOP = MDrv_DumpGopByGwinId(&pGOPDrvLocalCtx->apiCtxLocal, u8win);
	u8win_temp = u8win;
	for (u8GOPIdx = u8GOP; u8GOPIdx > 0; u8GOPIdx--) {
		u8win_temp =
		    u8win_temp - KDrv_HW_GOP_GetGwinNum(u8GOPIdx - 1, 0,
							E_HW_GOP_GET_GWIN_MAX_NUMBER);
	}

	if (_IsGopNumVaild((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP) == FALSE) {
		GOP_D_ERR("\n[%s] not support GOP id:%d in this chip version", __func__, u8GOP);
		return;
	}

	u16GOP_Unit = MDrv_GOP_GetWordUnit((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP);
	u16Bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (u16Bpp == FB_FMT_AS_DEFAULT) {
		GOP_D_ERR("[%s] invalid color format\n", __func__);
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

	if (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bPixelMode[u8GOP] == TRUE) {
		/*Pixel Unit */
		u16Pitch = pinfo->u16RBlkHRblkSize / u16GOP_Unit / (u16Bpp / 8);
	} else {
		/*Word Unit */
		u16Pitch = pinfo->u16RBlkHRblkSize / u16GOP_Unit;
	}

	if (pinfo->u16DispHPixelEnd > pinfo->u16DispHPixelStart) {
		Width = (MS_PHY) (pinfo->u16DispHPixelEnd - pinfo->u16DispHPixelStart);
	} else {
		if (pGOPDrvLocalCtx->pDrvCtxShared->apiCtxShared.bGopHasInitialized[u8GOP]
			== TRUE) {
			GOP_D_DBUG("[%s]WidthEnd 0x%x >WidthStart 0x%x\n", __func__,
				   pinfo->u16DispHPixelEnd, pinfo->u16DispHPixelStart);
		}
	}
	if (_GOP_GWIN_2PEngineAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP, &Width)
		== CHECKALIGN_PARA_FAIL) {
		GOP_D_WARN
		    ("[%s][%d]not Align, original HPixelStart =0x%x; HPixelEnd=0x%x\n",
		     __func__, __LINE__, pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd);
		pinfo->u16DispHPixelStart = ((pinfo->u16DispHPixelStart + 1) & ~(1));
		pinfo->u16DispHPixelEnd = ((pinfo->u16DispHPixelEnd + 1) & ~(1));
		GOP_D_WARN
		    ("[%s][%d]not Align, Align after HPixelStart =0x%x; HPixelEnd=0x%x\n",
		     __func__, __LINE__, pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd);
	}
	_GOP_GWIN_2PEngineAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP,
				       &(pinfo->u64DRAMRBlkStart));

	if (pGOPDrvLocalCtx->pDrvCtxShared->apiCtxShared.bVMirror) {
		KDrv_HW_GOP_SetDram_Addr(u8GOP, u8win_temp,
					 (MS_PHY) (pinfo->u16DispVPixelEnd -
						   pinfo->u16DispVPixelStart - 1), E_GOP_DRAMVstr);
	} else {
		KDrv_HW_GOP_SetDram_Addr(u8GOP, u8win_temp, 0, E_GOP_DRAMVstr);
	}

	//GOP HW just read the relative offset of each MIU
	_phy_to_miu_offset(u8MiuSel, phyTmp, pinfo->u64DRAMRBlkStart);
	UNUSED(u8MiuSel);
	phyTmp /= u16GOP_Unit;

	KDrv_HW_GOP_SetDram_Addr(u8GOP, u8win_temp, phyTmp, E_GOP_RBLKAddr);
	KDrv_HW_GOP_SetPitch(u8GOP, u8win_temp, u16Pitch, pinfo->u16RBlkVPixSize);

	GOP_SetGopGwinHVPixel(pGOPDrvLocalCtx, u8win, u8GOP, pinfo);

	GOP_D_INFO("\t[Vst, Vend, Hst, Hend, GwinHsz](Unit:Pixel) = [%d, %d, %d, %d, %d]\n",
		   pinfo->u16DispVPixelStart,
		   pinfo->u16DispVPixelEnd,
		   pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd, pinfo->u16RBlkHPixSize);

}

static void GOP_SetGop1GwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				DRV_GOP_GWIN_INFO *pinfo)
{
	MS_PHY u64tmp;
	MS_U16 bpp;
	MS_U16 u16GOP_Unit = 0;
	MS_U16 u16Pitch = 0;
	MS_U8 u8MiuSel;
	MS_PHY Width = 0;

	u16GOP_Unit = MDrv_GOP_GetWordUnit((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1);

	u8win = u8win - KDrv_HW_GOP_GetGwinNum(0, 0, E_HW_GOP_GET_GWIN_MAX_NUMBER);
	bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (bpp == FB_FMT_AS_DEFAULT) {
		GOP_D_ERR("[%s] invalid color format\n", __func__);
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

	if (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bPixelMode[E_GOP1] == TRUE) {
		/*Pixel Unit */
		u16Pitch = pinfo->u16RBlkHRblkSize / u16GOP_Unit / (bpp / 8);
	} else {
		/*Word Unit */
		u16Pitch = pinfo->u16RBlkHRblkSize / u16GOP_Unit;
	}

	if (pinfo->u16DispHPixelEnd > pinfo->u16DispHPixelStart) {
		Width = (MS_PHY) (pinfo->u16DispHPixelEnd - pinfo->u16DispHPixelStart);
	} else {
		if (pGOPDrvLocalCtx->pDrvCtxShared->apiCtxShared.bGopHasInitialized[E_GOP1]
			== TRUE) {
			GOP_D_DBUG("[%s]WidthEnd 0x%x >WidthStart 0x%x\n", __func__,
				   pinfo->u16DispHPixelEnd, pinfo->u16DispHPixelStart);
		}
	}
	if (_GOP_GWIN_2PEngineAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1, &Width)
		== CHECKALIGN_PARA_FAIL) {
		GOP_D_WARN
		    ("[%s][%d]not Align, original HPixelStart =0x%x; HPixelEnd=0x%x\n",
		     __func__, __LINE__, pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd);
		pinfo->u16DispHPixelStart = ((pinfo->u16DispHPixelStart + 1) & ~(1));
		pinfo->u16DispHPixelEnd = ((pinfo->u16DispHPixelEnd + 1) & ~(1));
		GOP_D_WARN
		    ("[%s][%d]not Align,Align after HPixelStart =0x%x; HPixelEnd=0x%x\n",
		     __func__, __LINE__, pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd);
	}
	_GOP_GWIN_2PEngineAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1,
				       &(pinfo->u64DRAMRBlkStart));

	if (pGOPDrvLocalCtx->pDrvCtxShared->apiCtxShared.bVMirror) {
		KDrv_HW_GOP_SetDram_Addr(E_GOP1, u8win,
					 (MS_PHY) (pinfo->u16DispVPixelEnd -
						   pinfo->u16DispVPixelStart - 1), E_GOP_DRAMVstr);
	} else {
		KDrv_HW_GOP_SetDram_Addr(E_GOP1, u8win, 0, E_GOP_DRAMVstr);
	}
	//GOP HW just read the relative offset of each MIU
	_phy_to_miu_offset(u8MiuSel, u64tmp, pinfo->u64DRAMRBlkStart);
	u64tmp /= u16GOP_Unit;
	UNUSED(u8MiuSel);
	KDrv_HW_GOP_SetDram_Addr(E_GOP1, u8win, u64tmp, E_GOP_RBLKAddr);
	KDrv_HW_GOP_SetPitch(E_GOP1, u8win, u16Pitch, pinfo->u16RBlkVPixSize);

	GOP_SetGopGwinHVPixel(pGOPDrvLocalCtx, u8win, E_GOP1, pinfo);

	GOP_D_INFO("\t[Vst, Vend, Hst, Hend, GwinHsz] = [%d, %d, %d, %d, %d](Unit:Pixel)\n",
		   pinfo->u16DispVPixelStart,
		   pinfo->u16DispVPixelEnd,
		   pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd, pinfo->u16RBlkHPixSize);

}

static void GOP_SetGop0GwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				DRV_GOP_GWIN_INFO *pinfo)
{
	MS_U64 u64tmp = 0;
	MS_PHY u64tmp1 = 0;
	MS_U16 u16tmp = 0;
	MS_U16 bpp;
	MS_U16 u16GOP_Unit = 0;
	MS_U16 u16Pitch = 0;
	MS_U8 u8MiuSel;
	MS_PHY Width = 0;

	bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (bpp == FB_FMT_AS_DEFAULT) {
		GOP_D_ERR("[%s] invalid color format\n", __func__);
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

	if (pinfo->u16DispHPixelEnd > pinfo->u16DispHPixelStart) {
		Width = (MS_PHY) (pinfo->u16DispHPixelEnd - pinfo->u16DispHPixelStart);
	} else {
		if (pGOPDrvLocalCtx->pDrvCtxShared->apiCtxShared.bGopHasInitialized[E_GOP0]
			== TRUE) {
			GOP_D_DBUG("[%s]WidthEnd 0x%x >WidthStart 0x%x\n", __func__,
				   pinfo->u16DispHPixelEnd, pinfo->u16DispHPixelStart);
		}
	}
	if (CHECKALIGN_PARA_FAIL ==
	    _GOP_GWIN_2PEngineAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0, &Width)) {
		GOP_D_WARN
		    ("[%s][%d]not Align, Original HPixelStart =0x%x; HPixelEnd=0x%x\n",
		     __func__, __LINE__, pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd);
		pinfo->u16DispHPixelStart = ((pinfo->u16DispHPixelStart + 1) & ~(1));
		pinfo->u16DispHPixelEnd = ((pinfo->u16DispHPixelEnd + 1) & ~(1));
		GOP_D_WARN
		    ("[%s][%d]not Align,after align HPixelStart =0x%x; HPixelEnd=0x%x\n",
		     __func__, __LINE__, pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd);
	}
	_GOP_GWIN_2PEngineAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0,
				       &(pinfo->u64DRAMRBlkStart));

	u16GOP_Unit = MDrv_GOP_GetWordUnit((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0);
	if (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bPixelMode[E_GOP0] == TRUE) {
		/*Pixel Unit */
		u16Pitch = pinfo->u16RBlkHRblkSize / u16GOP_Unit / (bpp / 8);
#if ENABLE_GOP0_RBLK_MIRROR
		u16tmp = pinfo->u16WinX;
		KDrv_HW_GOP_GWIN_SetWinHstr(E_GOP0, u8win, u16tmp);
#endif
	} else {
		/*Word Unit */
		u16Pitch = pinfo->u16RBlkHRblkSize / u16GOP_Unit;
#if ENABLE_GOP0_RBLK_MIRROR
		u16tmp = pinfo->u16WinX * (bpp >> 3) / u16GOP_Unit;
		KDrv_HW_GOP_GWIN_SetWinHstr(E_GOP0, u8win, u16tmp);
#endif
	}

#if ENABLE_GOP0_RBLK_MIRROR
	u64tmp = (MS_U64) pinfo->u16WinY *
	(MS_U64) pinfo->u16RBlkHRblkSize;
	u64tmp /= u16GOP_Unit;
#else
	u64tmp = ((MS_U64) pinfo->u16WinY *
	(MS_U64) pinfo->u16RBlkHRblkSize +
	(MS_U64) pinfo->u16WinX *
	(bpp >> 3));
	u64tmp /= u16GOP_Unit;
#endif
	KDrv_HW_GOP_GetDram_Addr(E_GOP0, u8win, &u64tmp1, E_GOP_DRAMVstr);

	if (pGOPDrvLocalCtx->pDrvCtxShared->apiCtxShared.bVMirror) {
		KDrv_HW_GOP_SetDram_Addr(E_GOP0, u8win, (MS_PHY) (pinfo->u16RBlkVPixSize - 1),
					 E_GOP_DRAMVstr);
	}
#if ENABLE_GOP0_RBLK_MIRROR
	else if (u64tmp != u64tmp1)
#else
	else if (pGOPDrvLocalCtx->pDrvCtxShared->apiCtxShared.bVMirror && (u64tmp != u64tmp1))
#endif
	{
		KDrv_HW_GOP_SetDram_Addr(E_GOP0, u8win, (MS_PHY) u64tmp, E_GOP_DRAMVstr);
	}
//GOP HW just read the relative offset of each MIU
	_phy_to_miu_offset(u8MiuSel, u64tmp, pinfo->u64DRAMRBlkStart);
	u64tmp /= u16GOP_Unit;
	UNUSED(u8MiuSel);
	KDrv_HW_GOP_SetDram_Addr(E_GOP0, u8win, (MS_PHY) u64tmp, E_GOP_RBLKAddr);
	u64tmp = ((MS_U32) pinfo->u16RBlkHRblkSize * (MS_U32) pinfo->u16RBlkVPixSize) / u16GOP_Unit;

	_GOP_GWIN_FB_AddrAlignChecker((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0, u64tmp, bpp);
	KDrv_HW_GOP_SetDram_Addr(E_GOP0, u8win, (MS_PHY) u64tmp, E_GOP_RBLKSize);
	KDrv_HW_GOP_SetPitch(E_GOP0, u8win, u16Pitch, pinfo->u16RBlkVPixSize);

	GOP_SetGopGwinHVPixel(pGOPDrvLocalCtx, u8win, E_GOP0, pinfo);

	GOP_D_INFO("\t[Vst, Vend, Hst, Hend, GwinHsz](Unit:Pixel) = [%d, %d, %d, %d, %d]\n",
		   pinfo->u16DispVPixelStart,
		   pinfo->u16DispVPixelEnd,
		   pinfo->u16DispHPixelStart, pinfo->u16DispHPixelEnd, pinfo->u16RBlkHPixSize);
}

static void GOP_ReadGopExtendGwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				      DRV_GOP_GWIN_INFO *pinfo)
{
	MS_PHY phyTmp = 0;
	MS_U16 u16Bpp, u16tmp = 0, u16ClrType = 0;
	MS_U8 u8GOP = 0, u8GOPIdx = 0;
	MS_U16 u16GOP_Unit = 0;
	MS_U8 u8win_temp = 0;

	if (_IsGwinIdValid((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8win) == FALSE) {
		GOP_D_ERR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return;
	}

	u8GOP = MDrv_DumpGopByGwinId(&pGOPDrvLocalCtx->apiCtxLocal, u8win);
	u8win_temp = u8win;
	for (u8GOPIdx = u8GOP; u8GOPIdx > 0; u8GOPIdx--) {
		u8win_temp =
		    u8win_temp - KDrv_HW_GOP_GetGwinNum(u8GOPIdx - 1, 0,
							E_HW_GOP_GET_GWIN_MAX_NUMBER);
	}

	if (_IsGopNumVaild((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP) == FALSE) {
		GOP_D_ERR("\n[%s] not support GOP id:%d in this chip version", __func__, u8GOP);
		return;
	}

	KDrv_HW_GOP_GWIN_GetWinFmt(u8GOP, u8win, &u16ClrType);
	pinfo->clrType = _MDrv_GOP_ConvertHWFmt(u16ClrType);
	u16Bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (u16Bpp == FB_FMT_AS_DEFAULT)
		return;

	KDrv_HW_GOP_GetGopGwinHVPixel(u8GOP, u8win, &pinfo->u16DispHPixelStart,
				      &pinfo->u16DispHPixelEnd, &pinfo->u16DispVPixelStart,
				      &pinfo->u16DispVPixelEnd);

	u16tmp = KDrv_HW_GOP_GetPitch(u8GOP, u8win);
	u16GOP_Unit = MDrv_GOP_GetWordUnit((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOP);
	if (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bPixelMode[u8GOP] == TRUE)
		pinfo->u16RBlkHRblkSize = u16tmp * (u16Bpp / 8);
	else
		pinfo->u16RBlkHRblkSize = u16tmp * u16GOP_Unit;

	KDrv_HW_GOP_GetDram_Addr(u8GOP, u8win_temp, &phyTmp, E_GOP_RBLKAddr);
	pinfo->u64DRAMRBlkStart = phyTmp * u16GOP_Unit;

	GOP_D_INFO
	    ("GWIN_GetWin(%d): [adr(B),Hsz,Vsz,Hsdrm,winX,winY] = [%td, %d, %d, %d, %d, %d]\n",
	     u8win, (ptrdiff_t) pinfo->u64DRAMRBlkStart, pinfo->u16RBlkHPixSize,
	     pinfo->u16RBlkVPixSize, pinfo->u16RBlkHRblkSize, pinfo->u16WinX, pinfo->u16WinY);


}

static void GOP_ReadGop1GwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				 DRV_GOP_GWIN_INFO *pinfo)
{
	MS_PHY phyTmp = 0;
	MS_U16 u16tmp = 0;
	MS_U16 bpp;
	MS_U16 u16GOP_Unit = 0;
	MS_U16 u16ClrType = 0;

	u8win = u8win - KDrv_HW_GOP_GetGwinNum(0, 0, E_HW_GOP_GET_GWIN_MAX_NUMBER);
	KDrv_HW_GOP_GWIN_GetWinFmt(E_GOP1, u8win, &u16ClrType);
	pinfo->clrType = _MDrv_GOP_ConvertHWFmt(u16ClrType);
	bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (bpp == FB_FMT_AS_DEFAULT)
		return;

	KDrv_HW_GOP_GetGopGwinHVPixel(E_GOP1, u8win, &pinfo->u16DispHPixelStart,
				      &pinfo->u16DispHPixelEnd, &pinfo->u16DispVPixelStart,
				      &pinfo->u16DispVPixelEnd);

	u16tmp = KDrv_HW_GOP_GetPitch(E_GOP1, u8win);
	u16GOP_Unit = MDrv_GOP_GetWordUnit((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP1);
	if (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bPixelMode[E_GOP1] == TRUE)
		pinfo->u16RBlkHRblkSize = u16tmp * (bpp / 8);
	else
		pinfo->u16RBlkHRblkSize = u16tmp * u16GOP_Unit;

	KDrv_HW_GOP_GetDram_Addr(E_GOP1, u8win, &phyTmp, E_GOP_RBLKAddr);
	pinfo->u64DRAMRBlkStart = phyTmp * u16GOP_Unit;

	GOP_D_INFO
	    ("GWIN_GetWin(%d):[adr(B), Hsz, Vsz, Hsdrm, winX, winY] = [%td, %d, %d, %d, %d, %d]\n",
	     u8win, (ptrdiff_t) pinfo->u64DRAMRBlkStart, pinfo->u16RBlkHPixSize,
	     pinfo->u16RBlkVPixSize, pinfo->u16RBlkHRblkSize, pinfo->u16WinX, pinfo->u16WinY);
}

static void GOP_ReadGop0GwinInfo(GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx, MS_U8 u8win,
				 DRV_GOP_GWIN_INFO *pinfo)
{
	MS_PHY phyTmp = 0;
	MS_U16 u16tmp = 0;
	MS_U16 bpp;
	MS_U16 u16GOP_Unit = 0;
	MS_U16 u16ClrType = 0;

	KDrv_HW_GOP_GWIN_GetWinFmt(E_GOP0, u8win, &u16ClrType);
	pinfo->clrType = _MDrv_GOP_ConvertHWFmt(u16ClrType);
	bpp = MDrv_GOP_GetBPP(&pGOPDrvLocalCtx->apiCtxLocal, pinfo->clrType);
	if (bpp == FB_FMT_AS_DEFAULT)
		return;

	KDrv_HW_GOP_GetGopGwinHVPixel(E_GOP0, u8win, &pinfo->u16DispHPixelStart,
				      &pinfo->u16DispHPixelEnd, &pinfo->u16DispVPixelStart,
				      &pinfo->u16DispVPixelEnd);

	u16tmp = KDrv_HW_GOP_GetPitch(E_GOP0, u8win);
	u16GOP_Unit = MDrv_GOP_GetWordUnit((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, E_GOP0);
	if (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bPixelMode[E_GOP0] == TRUE)
		pinfo->u16RBlkHRblkSize = u16tmp * (bpp / 8);
	else
		pinfo->u16RBlkHRblkSize = u16tmp * u16GOP_Unit;

	KDrv_HW_GOP_GetDram_Addr(E_GOP0, u8win, &phyTmp, E_GOP_RBLKAddr);
	pinfo->u64DRAMRBlkStart = phyTmp * u16GOP_Unit;

	phyTmp = 0;
	KDrv_HW_GOP_GetDram_Addr(E_GOP0, u8win, &phyTmp, E_GOP_RBLKSize);

	if (pinfo->u16RBlkHRblkSize != 0)
		pinfo->u16RBlkVPixSize = ((phyTmp * u16GOP_Unit) / pinfo->u16RBlkHRblkSize);

	GOP_D_INFO("GWIN_GetWin(%d): [adr(B), Hsz, Vsz, Hsdrm ] = [%td, %d, %d, %d]\n",
		   u8win,
		   (ptrdiff_t) pinfo->u64DRAMRBlkStart,
		   pinfo->u16RBlkHPixSize, pinfo->u16RBlkVPixSize, pinfo->u16RBlkHRblkSize);
}

MS_BOOL _GWIN_ADDR_Invalid_Check(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win)
{

	return TRUE;
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

	GOP_ASSERT(0);
	return INVALID_DRV_GOP_NUM;
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

MS_U16 MDrv_GOP_GetGOPACK(MS_GOP_CTX_LOCAL *pstGOPCtx, MS_U8 enGopType)
{
	MS_BOOL bGopAck = FALSE;

	if (pstGOPCtx == NULL)
		return bGopAck;

	if (MDrv_GOP_GWIN_IsForceWrite(pstGOPCtx, enGopType))
		bGopAck = TRUE;
	else
		bGopAck = KDrv_HW_GOP_GetRegUpdated(enGopType);

	return bGopAck;
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

MS_U8 MDrv_GOP_Get(MS_GOP_CTX_LOCAL *pGOPCtx)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	return pGOPDrvLocalCtx->current_gop;
}

MS_BOOL MDrv_GOP_SetIOMapBase(MS_GOP_CTX_LOCAL *pGOPCtx)
{
	MS_VIRT MMIOBaseAdr;
	MS_PHY u32NonPMBankSize;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	uint8_t i;

	if (!MDrv_MMIO_GetBASE(&MMIOBaseAdr, &u32NonPMBankSize, MS_MODULE_GOP)) {
		GOP_D_FATAL("Get GOP IOMap failure\n");
		GOP_ASSERT(0);
		return FALSE;
	}

	if (pGOPDrvLocalCtx == NULL)
		return UTOPIA_STATUS_PARAMETER_ERROR;

	if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
		GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_3M_SERIES_MAJOR)
		pGOPDrvLocalCtx->pGopChipPro->VAdr[0] = MMIOBaseAdr;

	for (i = 0; i < GOP_MAX_SUPPORT_DEFAULT; i++) {
		if (pGOPDrvLocalCtx->pGopChipPro->PAdr[i] != 0) {
			drv_hwreg_common_setRIUaddr(pGOPDrvLocalCtx->pGopChipPro->PAdr[i],
			pGOPDrvLocalCtx->pGopChipPro->u32RegSize[i],
			pGOPDrvLocalCtx->pGopChipPro->VAdr[i]);
		}
	}

	KDrv_HW_GOP_SetIOMapBase(pGOPDrvLocalCtx->pGopChipPro->VAdr);

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
			GOP_D_ERR("SHM allocation failed!\n");
			return NULL;
		}
		GOP_D_INFO("[%s][%d] This is first initial 0x%tx\n", __func__, __LINE__,
			   (ptrdiff_t) VAddr);
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
		GOP_D_ERR("[%s, %d]readChipCaps_DTB failed \r\n", __func__, __LINE__);

	*pGopChipPro = &g_GopChipPro;
}

MS_GOP_CTX_LOCAL *Drv_GOP_Init_Context(void *pInstance, MS_BOOL *pbNeedInitShared)
{
	GOP_CTX_DRV_SHARED *pDrvGOPShared;
	MS_BOOL bNeedInitShared = FALSE;
	MS_U8 u8Index = 0;

#ifdef INSTANT_PRIVATE
	GOP_INSTANT_PRIVATE *psGOPInstPri = NULL;

	UtopiaInstanceGetPrivate(pInstance, (void *)&psGOPInstPri);
#endif

	pDrvGOPShared = (GOP_CTX_DRV_SHARED *) MDrv_GOP_GetShareMemory(&bNeedInitShared);

	*pbNeedInitShared = bNeedInitShared;
	memset(&g_gopDrvCtxLocal, 0, sizeof(g_gopDrvCtxLocal));

	if (bNeedInitShared) {
		memset(pDrvGOPShared, 0, sizeof(GOP_CTX_DRV_SHARED));
		for (u8Index = 0; u8Index < SHARED_GOP_MAX_COUNT; u8Index++) {
			if (_MDrv_GOP_CSC_ParamInit
			    (&(pDrvGOPShared->apiCtxShared.stCSCParam[u8Index])) == FALSE) {
				GOP_D_ERR("_MDrv_GOP_CSC_ParamInit failed!\n");
			}
			if (_MDrv_GOP_CSC_TableInit
			    (&(pDrvGOPShared->apiCtxShared.stCSCTable[u8Index])) == FALSE) {
				GOP_D_ERR("_MDrv_GOP_CSC_TableInit failed!\n");
			}
		}
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
			//assert here!!
			GOP_D_DBUG("Error - TotalGwinNum >= SHARED_GWIN_MAX_COUNT!!\n");
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

void MDrv_GOP_Init(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_PHY u64GOP_REGDMABASE,
		   MS_U32 u32GOP_RegdmaLen, MS_BOOL bEnableVsyncIntFlip)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	MS_GOP_CTX_LOCAL apiCtxLocal = pGOPDrvLocalCtx->apiCtxLocal;
	MS_GOP_CTX_SHARED *pGOPCtxShared = apiCtxLocal.pGOPCtxShared;
	MS_U16 u16PnlWidth = pGOPCtxShared->u16PnlWidth[u8GOPNum];
	MS_U16 u16PnlHeight = pGOPCtxShared->u16PnlHeight[u8GOPNum];

	if (_IsGopNumVaild((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOPNum) == FALSE) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__,
			  u8GOPNum);
		return;
	}

	KDrv_HW_GOP_Init(u8GOPNum);

	if (pGOPCtx->pGopChipProperty->bPixelModeSupport)
		KDrv_HW_GOP_PixelModeEnable(u8GOPNum, TRUE);

	if (pGOPCtx->pGopChipProperty->bOPMuxDoubleBuffer)
		KDrv_HW_GOP_OpMuxDbEnable(TRUE);

	if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
		GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_M6_SERIES_MAJOR) {
		//step2 HVSP always scaling 2x, if timing is 8K
		if (u16PnlWidth == GOP_8K_WIDTH && u16PnlHeight == GOP_8K_HEIGHT) {
			KDrv_HW_GOP_SetStep2HVSPHscale(TRUE, GOP_SCALING_2TIMES);
			KDrv_HW_GOP_SetStep2HVSPVscale(TRUE, GOP_SCALING_2TIMES);
			KDrv_HW_GOP_SetMixer2OutSize(GOP_8K_WIDTH, GOP_8K_HEIGHT);
			KDrv_HW_GOP_SetMixer4OutSize(GOP_4K_WIDTH, GOP_4K_HEIGHT);
		} else {
			//If timing, then only use step1 HVSP to scaling up
			KDrv_HW_GOP_SetStep2HVSPHscale(FALSE, 0x0);
			KDrv_HW_GOP_SetStep2HVSPVscale(FALSE, 0x0);
			KDrv_HW_GOP_SetMixer2OutSize(GOP_4K_WIDTH, GOP_4K_HEIGHT);
			KDrv_HW_GOP_SetMixer4OutSize(GOP_4K_WIDTH, GOP_4K_HEIGHT);
		}
	}

}

/********************************************************************************/
/// Set GOP progressive mode
/// @param bEnable \b IN
///   - # TRUE  Progressive (read out the DRAM graphic data by FIELD)
///   - # FALSE Interlaced (not care FIELD)
/// @internal please verify the register document and the code
/********************************************************************************/
void MDrv_GOP_GWIN_EnableProgressive(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_BOOL bEnable)
{
	KDrv_HW_GOP_GWIN_ProgressiveEnable(u8GOPNum, bEnable);
}

/********************************************************************************/
/// Get GOP progressive mode
/// @return
///   - # TRUE  Progressive (read out the DRAM graphic data by FIELD)
///   - # FALSE Interlaced (not care FIELD)
/// @internal please verify the register document and the code
/********************************************************************************/
MS_BOOL MDrv_GOP_GWIN_IsProgressive(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum)
{
	MS_BOOL bIsPMode;

	bIsPMode = KDrv_HW_GOP_GWIN_IsProgressive(u8GOPNum);
	if (bIsPMode)
		return TRUE;
	else
		return FALSE;

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

void MDrv_GOP_GWIN_ForceWrite_Update(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bEnable)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bBnkForceWrite)
		pGOPDrvLocalCtx->bGOPBankFwr[u8GOP] = bEnable;
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

	return (MS_BOOL) (pGOPDrvLocalCtx->gop_gwin_frwr || pGOPDrvLocalCtx->bGOPBankFwr[u8GOP]);
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
	GOP_GWIN_UpdateReg(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) u8GOP);
}

void MDrv_GOP_GWIN_SetHSPipe(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_U16 u16HSPipe)
{
	DRV_GOPDstType pGopDst = E_DRV_GOP_DST_INVALID;
	MS_U16 u16NonVS_PD_Delay = 0;
	EN_HW_GOP_VOPPATH_SEL eHWVopSelMode = E_HW_GOP_VOPPATH_DEFAULT;

	if (_IsGopNumVaild(pGOPCtx, u8GOPNum) == FALSE) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__,
			  u8GOPNum);
		return;
	}

	MDrv_GOP_GWIN_GetDstPlane(pGOPCtx, u8GOPNum, &pGopDst);

	if (pGOPCtx->pGopChipProperty->bGOPVscalePipeDelay[u8GOPNum]) {
		if (pGopDst != E_DRV_GOP_DST_IP0) {
			u16NonVS_PD_Delay = (pGOPCtx->pGopChipProperty->b2Pto1PSupport == TRUE) ?
			    pGOPCtx->pGopChipProperty->GOP_NonVS_PD_Offset *
			    2 : pGOPCtx->pGopChipProperty->GOP_NonVS_PD_Offset;
			u16HSPipe += u16NonVS_PD_Delay;
		}
	}

	if (pGOPCtx->pGopChipProperty->bSupportVOPPathSel) {
		eHWVopSelMode = KDrv_HW_GOP_GetVOPPathSel();
		switch (eHWVopSelMode) {
		case E_HW_GOP_VOPPATH_BEF_RGB3DLOOKUP:
			u16HSPipe = u16HSPipe - pGOPCtx->pGopChipProperty->u16BefRGB3DLupPDOffset;
			break;
		case E_HW_GOP_VOPPATH_BEF_PQGAMMA:
			u16HSPipe = u16HSPipe - pGOPCtx->pGopChipProperty->u16BefPQGammaPDOffset;
			break;
		case E_HW_GOP_VOPPATH_AFT_PQGAMMA:
			u16HSPipe = u16HSPipe - pGOPCtx->pGopChipProperty->u16AftPQGammaPDOffset;
			break;
		default:
			break;
		}
	}
	//Pixel shift PD offset
	u16HSPipe = u16HSPipe - pGOPCtx->pGOPCtxShared->s32PixelShiftPDOffset;

	KDrv_HW_GOP_GWIN_SetHSPipe(u8GOPNum, u16HSPipe);
}

void MDrv_GOP_GWIN_GetMux(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 *u8GOPNum, Gop_MuxSel eGopMux)
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
	case E_GOP_IP0_MUX:
		eHWGopMux = E_HW_GOP_IP0_MUX;
		break;
	case E_GOP_FRC_MUX0:
		eHWGopMux = E_HW_GOP_FRC_MUX0;
		break;
	case E_GOP_FRC_MUX1:
		eHWGopMux = E_HW_GOP_FRC_MUX1;
		break;
	case E_GOP_FRC_MUX2:
		eHWGopMux = E_HW_GOP_FRC_MUX2;
		break;
	case E_GOP_FRC_MUX3:
		eHWGopMux = E_HW_GOP_FRC_MUX3;
		break;
	case E_GOP_BYPASS_MUX0:
		eHWGopMux = E_HW_GOP_BYPASS_MUX0;
		break;
	case E_GOP_VE1_MUX:
		eHWGopMux = E_HW_GOP_VE1_MUX;
		break;
	default:
		break;
	}
	*u8GOPNum = KDrv_HW_GOP_GWIN_GetMux(eHWGopMux);

}

void MDrv_GOP_GWIN_SetMux(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, Gop_MuxSel eGopMux)
{
	E_HW_GOP_MuxSel eHWGopMux = E_HW_MAX_GOP_MUX_SUPPORT;

	if (!_IsMuxSelVaild(pGOPCtx, u8GOPNum)) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__,
			  u8GOPNum);
		return;
	}

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
	case E_GOP_IP0_MUX:
		eHWGopMux = E_HW_GOP_IP0_MUX;
		break;
	case E_GOP_FRC_MUX0:
		eHWGopMux = E_HW_GOP_FRC_MUX0;
		break;
	case E_GOP_FRC_MUX1:
		eHWGopMux = E_HW_GOP_FRC_MUX1;
		break;
	case E_GOP_FRC_MUX2:
		eHWGopMux = E_HW_GOP_FRC_MUX2;
		break;
	case E_GOP_FRC_MUX3:
		eHWGopMux = E_HW_GOP_FRC_MUX3;
		break;
	case E_GOP_BYPASS_MUX0:
		eHWGopMux = E_HW_GOP_BYPASS_MUX0;
		break;
	case E_GOP_VE1_MUX:
		eHWGopMux = E_HW_GOP_VE1_MUX;
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
	DRV_GOPDstType gopDst = E_DRV_GOP_DST_OP0;

	MDrv_GOP_GWIN_GetDstPlane(pGOPCtx, u8GopNum, &gopDst);

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
	if (!_IsMuxSelVaild(pGOPCtx, gopNum)) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__, gopNum);
		return;
	}

	KDrv_HW_GOP_SetGOP2SCEnable(gopNum, bEnable);
}

#ifdef CONFIG_GOP_GWIN_MISC
void MDrv_GOP_SetGOPEnable2Mode1(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum, MS_BOOL bEnable)
{
	DRV_GOPDstType enGopDst = E_DRV_GOP_DST_INVALID;

	if (!_IsMuxSelVaild(pGOPCtx, gopNum)) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__, gopNum);
		return;
	}

	KDrv_HW_GOP_PreAlphaEnable(gopNum, bEnable);
	MDrv_GOP_GWIN_GetDstPlane(pGOPCtx, gopNum, &enGopDst);

	if (enGopDst == E_DRV_GOP_DST_OP0)
		MDrv_GOP_GWIN_SetAlphaInverse(pGOPCtx, gopNum, !bEnable);

}

void MDrv_GOP_GetGOPEnable2Mode1(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum, MS_BOOL *pbEnable)
{
	if (!_IsMuxSelVaild(pGOPCtx, gopNum)) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__, gopNum);
		return;
	}
	*pbEnable = KDrv_HW_GOP_GetPreAlphaMode(gopNum);
}
#endif

/********************************************************************************/
/// Set GOP destination (OP/IP) setting to scaler
/// @param ipSelGop \b IN \copydoc MS_IPSEL_GOP
/********************************************************************************/
void MDrv_GOP_SetIPSel2SC(MS_GOP_CTX_LOCAL *pGOPCtx, MS_IPSEL_GOP ipSelGop)
{
	E_HW_IPSEL_GOP HWRegGopIPSel = E_HW_NIP_SEL_GOP0;

	switch (ipSelGop) {
	case MS_DRV_IP0_SEL_GOP0:
		HWRegGopIPSel = E_HW_IP0_SEL_GOP0;
		break;
	case MS_DRV_IP0_SEL_GOP1:
		HWRegGopIPSel = E_HW_IP0_SEL_GOP1;
		break;
	case MS_DRV_IP0_SEL_GOP2:
		HWRegGopIPSel = E_HW_IP0_SEL_GOP2;
		break;
	case MS_DRV_NIP_SEL_GOP0:
		HWRegGopIPSel = E_HW_NIP_SEL_GOP0;
		break;
	case MS_DRV_NIP_SEL_GOP1:
		HWRegGopIPSel = E_HW_NIP_SEL_GOP1;
		break;
	case MS_DRV_NIP_SEL_GOP2:
		HWRegGopIPSel = E_HW_NIP_SEL_GOP2;
		break;
	default:
		break;
	}

	KDrv_HW_GOP_SetIPSel2SC(HWRegGopIPSel);
}

/********************************************************************************/
/// Set GOP destination clock
/// @param gopNum \b IN 0:GOP0  1:GOP1
/// @param eDstType \b IN \copydoc EN_GOP_DST_TYPE
/********************************************************************************/
GOP_Result MDrv_GOP_SetGOPClk(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum, DRV_GOPDstType eDstType)
{
	GOP_Result GopRet;
	E_HW_GOP_DstType eHWGopDst = E_HW_GOP_DST_OP0;
	MS_BOOL bHWRegRet;

	if (_IsGopNumVaild(pGOPCtx, gopNum) == FALSE) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__, gopNum);
		return GOP_FAIL;
	}

	switch (eDstType) {
	case E_DRV_GOP_DST_IP0:
		eHWGopDst = E_HW_GOP_DST_IP0;
		break;
	case E_DRV_GOP_DST_OP0:
		eHWGopDst = E_HW_GOP_DST_OP0;
		break;
	case E_DRV_GOP_DST_FRC:
		eHWGopDst = E_HW_GOP_DST_FRC;
		break;
	case E_DRV_GOP_DST_BYPASS:
		eHWGopDst = E_HW_GOP_DST_BYPASS;
		break;
	default:
		break;
	}
	bHWRegRet = KDrv_HW_GOP_SetGOPClk(gopNum, eHWGopDst);
	if (bHWRegRet)
		GopRet = GOP_SUCCESS;
	else
		GopRet = GOP_ENUM_NOT_SUPPORTED;

	return GopRet;
}

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
				GOP_D_WARN
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
				     DRV_GOPDstType eDstType, MS_BOOL bOnlyCheck)
{
	GOP_Result ret = GOP_SUCCESS;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	E_HW_GOP_DstType eHWGopDst = E_HW_GOP_DST_OP0;

	if (_IsGopNumVaild(pGOPCtx, u8GOPNum) == FALSE) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__,
			  u8GOPNum);
		return GOP_FAIL;
	}

	switch (eDstType) {
	case E_DRV_GOP_DST_IP0:
		eHWGopDst = E_HW_GOP_DST_IP0;
		break;
	case E_DRV_GOP_DST_OP0:
		eHWGopDst = E_HW_GOP_DST_OP0;
		break;
	case E_DRV_GOP_DST_FRC:
		eHWGopDst = E_HW_GOP_DST_FRC;
		break;
	case E_DRV_GOP_DST_BYPASS:
		eHWGopDst = E_HW_GOP_DST_BYPASS;
		break;
	default:
		break;
	}

	KDrv_HW_GOP_GWIN_SetDstPlane(u8GOPNum, eHWGopDst);
	if (bOnlyCheck == FALSE)
		GOP_GWIN_UpdateReg(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) u8GOPNum);

	return ret;
}

GOP_Result MDrv_GOP_GWIN_GetDstPlane(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
				     DRV_GOPDstType *pGopDst)
{
	GOP_Result ret;
	MS_BOOL HWRegRet;
	E_HW_GOP_DstType eHWGopDst;

	if (_IsGopNumVaild(pGOPCtx, u8GOPNum) == FALSE) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__,
			  u8GOPNum);
		return GOP_FAIL;
	}

	HWRegRet = KDrv_HW_GOP_GWIN_GetDstPlane(u8GOPNum, &eHWGopDst);
	if (HWRegRet)
		ret = GOP_SUCCESS;
	else
		ret = GOP_FAIL;

	switch (eHWGopDst) {
	case E_HW_GOP_DST_IP0:
		*pGopDst = E_DRV_GOP_DST_IP0;
		break;
	case E_HW_GOP_DST_OP0:
		*pGopDst = E_DRV_GOP_DST_OP0;
		break;
	case E_HW_GOP_DST_FRC:
		*pGopDst = E_DRV_GOP_DST_FRC;
		break;
	case E_HW_GOP_DST_BYPASS:
		*pGopDst = E_DRV_GOP_DST_BYPASS;
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
void MDrv_GOP_GWIN_SetStretchWin(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_U16 x, MS_U16 y,
				 MS_U16 width, MS_U16 height)
{
	MS_U32 u32HStretchRatio;
	MS_U32 u32VStretchRatio;
	MS_U16 u16OutputValidH = 0, u16OutputValidV = 0;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
#ifdef GOP_SUPPORT_SPLIT_MODE
	DRV_GOPDstType GopDst;
#endif

	MS_U16 u16OutputWidth = 0;

	if ((x > pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16PnlWidth[u8GOP])
	    || (y > pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16PnlHeight[u8GOP])) {
		APIGOP_ASSERT(FALSE,
			      GOP_D_FATAL
			      ("[%s]invalid x=%u,y=%u,width= %u,height= %u\n",
			       __func__, x, y,
			       pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16PnlWidth[u8GOP],
			       pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16PnlHeight[u8GOP]));
	}

	if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
		GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_3M_SERIES_MAJOR)
		KDrv_HW_GOP_SetGopStretchWin(u8GOP, x, y, (width / GOP_STRETCH_WIDTH_UNIT), height);
	else
		KDrv_HW_GOP_SetGopStretchWin(u8GOP, x, y, width, height);

	//H size change need to check reg_valid_pix_num_q (0x0c:GOP_4G_SVM_VSTR) value
	//GOP_4G_SVM_VSTR must be the size after scaling for gop_rdy

	KDrv_HW_GOP_GetGopScaleRatio(u8GOP, &u32HStretchRatio, &u32VStretchRatio);

	if (u32HStretchRatio == SCALING_MULITPLIER) {
#ifndef OUTPUT_VALID_SIZE_PATCH
		u16OutputValidH = (x + width);
#else
		if ((pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16HScaleSrc[u8GOP]) >
		    (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16HScaleDst[u8GOP])) {
			u16OutputWidth =
			    ALIGN_4(pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16PnlWidth[u8GOP]);
			u16OutputValidH = u16OutputWidth;
		} else {
			u16OutputValidH = GOP_OUTVALID_DEFAULT;
		}
#endif
	} else {
#ifndef OUTPUT_VALID_SIZE_PATCH
		u16OutputValidH =
		    (x + pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16HScaleDst[u8GOP]);
#else
		u16OutputValidH = GOP_OUTVALID_DEFAULT;
#endif
	}

	if (u32VStretchRatio == SCALING_MULITPLIER) {
#ifndef OUTPUT_VALID_SIZE_PATCH
		u16OutputValidV = (y + height);
#else
		u16OutputValidV = GOP_OUTVALID_DEFAULT;
#endif
	} else {
#ifndef OUTPUT_VALID_SIZE_PATCH
		u16OutputValidV =
		    (y + pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->u16VScaleDst[u8GOP]);
#else
		u16OutputValidV = GOP_OUTVALID_DEFAULT;
#endif
	}
	KDrv_HW_GOP_SetGopVaildH(u8GOP, u16OutputValidH);
	KDrv_HW_GOP_SetGopVaildV(u8GOP, u16OutputValidV);

}

//Internal use only, get the real HW register setting on stretch win
//For API usage, please use g_pGOPCtxLocal->pGOPCtxShared->u16StretchWinWidth
void MDrv_GOP_GWIN_Get_StretchWin(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_U16 *x, MS_U16 *y,
				  MS_U16 *width, MS_U16 *height)
{
	MS_U16 u16RegWidth;

	KDrv_HW_GOP_GetGopStretchWin(u8GOP, x, y, &u16RegWidth, height);
	if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
		GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_3M_SERIES_MAJOR)
		*width = u16RegWidth * GOP_STRETCH_WIDTH_UNIT;
	else
		*width = u16RegWidth;
}

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
	MS_U64 u64Hratio = 0;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *)pGOPCtx;
	MS_GOP_CTX_LOCAL apiCtxLocal = pGOPDrvLocalCtx->apiCtxLocal;
	MS_GOP_CTX_SHARED *pGOPCtxShared = apiCtxLocal.pGOPCtxShared;
	MS_U16 u16PnlWidth = pGOPCtxShared->u16PnlWidth[u8GOP];
	MS_U32 u32Hratio = 0;

	if (_IsGopNumVaild(pGOPCtx, u8GOP) == FALSE) {
		GOP_D_WARN("[%s] not support gop%d in this chip version!!\n", __func__, u8GOP);
		return;
	}

	if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
		GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_M6_SERIES_MAJOR) {
		if (u16PnlWidth == GOP_8K_WIDTH) {
			if (bEnable == TRUE) {
				if ((dst / 2) > src) {
					//step1 HVSP scaling to half size,
					//others size use step2 HVSP when 8k timing
					u32Hratio = (MS_U32)(src * SCALING_MULITPLIER) / (dst / 2);
				} else {
					u32Hratio = 0;
					bEnable = FALSE;
				}
			} else {
				u32Hratio = 0;
			}
		} else {
			if (bEnable == TRUE)
				u32Hratio = (MS_U32)(src * SCALING_MULITPLIER) / dst;
			else
				u32Hratio = 0;
		}
		KDrv_HW_GOP_SetStep1HVSPHscale(u8GOP, bEnable, u32Hratio);
		goto SUCCESS;
	}

	if (dst > src) {
		if (bEnable) {
			u64Hratio = (MS_U64) (src) * SCALING_MULITPLIER;

			if (u64Hratio % (MS_U64) dst != 0) {
				if (pGOPCtx->pGopChipProperty->bOPHandShakeModeSupport) {
					u64Hratio = (u64Hratio / (MS_U64) dst) + 1;
				} else {
					if (pGOPCtx->pGopChipProperty->bPixelModeSupport)
						u64Hratio = (u64Hratio / (MS_U64) dst);
					else
						u64Hratio = (u64Hratio / (MS_U64) dst) + 1;
				}
			} else {
				u64Hratio /= (MS_U64) dst;
			}
		} else {
			u64Hratio = SCALING_MULITPLIER;
		}

		//H size change need to check reg_valid_pix_num_q (0x0c:GOP_4G_SVM_VSTR) value
		//GOP_4G_SVM_VSTR must be the size after scaling for gop_rdy
		if (u64Hratio == SCALING_MULITPLIER) {
			MS_U16 u16StretchWidth = 1920;
			MS_U16 u16StretchX = 0;
			MS_U16 u16StretchHeight = 0;
			MS_U16 u16StretchY = 0;

			KDrv_HW_GOP_GetGopStretchWin(u8GOP, &u16StretchX, &u16StretchY,
						     &u16StretchWidth, &u16StretchHeight);
			if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
				GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_3M_SERIES_MAJOR)
				u16StretchWidth *= 2;
			KDrv_HW_GOP_SetGopVaildH(u8GOP, (u16StretchWidth + u16StretchX));
		} else {
			MS_U16 u16StretchWidth = 0;
			MS_U16 u16StretchX = 0;
			MS_U16 u16StretchHeight = 0;
			MS_U16 u16StretchY = 0;

			KDrv_HW_GOP_GetGopStretchWin(u8GOP, &u16StretchX, &u16StretchY,
						     &u16StretchWidth, &u16StretchHeight);
			KDrv_HW_GOP_SetGopVaildH(u8GOP, (u16StretchX + dst));
		}
		KDrv_HW_GOP_GWIN_Set_HSCALE(u8GOP, 0, u64Hratio);
	} else {
		KDrv_HW_GOP_GWIN_Set_HSCALE(u8GOP, 0, SCALING_MULITPLIER);
	}

#ifdef OUTPUT_VALID_SIZE_PATCH
	if (dst < src) {
		dst = ALIGN_4(dst);
		KDrv_HW_GOP_SetGopVaildH(u8GOP, dst);
	} else {
		KDrv_HW_GOP_SetGopVaildH(u8GOP, GOP_OUTVALID_DEFAULT);
	}
#else
	KDrv_HW_GOP_SetGopVaildH(u8GOP, dst);
#endif

SUCCESS:
	return;
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
	MS_U64 u64Vratio = 0;
	MS_U16 u16x = 0, u16y = 0, u16w = 0, u16h = 0;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *)pGOPCtx;
	MS_GOP_CTX_LOCAL apiCtxLocal = pGOPDrvLocalCtx->apiCtxLocal;
	MS_GOP_CTX_SHARED *pGOPCtxShared = apiCtxLocal.pGOPCtxShared;
	MS_U16 u16PnlHeight = pGOPCtxShared->u16PnlHeight[u8GOP];
	MS_U32 u32Vratio = 0;

	if (_IsGopNumVaild(pGOPCtx, u8GOP) == FALSE) {
		GOP_D_WARN("[%s] not support gop%d in this chip version!!\n", __func__, u8GOP);
		return;
	}

	if (((u32ChipVer & GOP_MAJOR_IPVERSION_OFFSET_MSK) >>
		GOP_MAJOR_IPVERSION_OFFSET_SHT) == GOP_M6_SERIES_MAJOR) {
		if (u16PnlHeight == GOP_8K_HEIGHT) {
			if (bEnable == TRUE) {
				if ((dst / 2) > src) {
					//step1 HVSP scaling to half size,
					//others size use step2 HVSP when 8k timing
					u32Vratio = (MS_U32)(src * SCALING_MULITPLIER) / (dst / 2);
				} else {
					u32Vratio = 0;
					bEnable = FALSE;
				}
			} else {
				u32Vratio = 0;
			}
		} else {
			if (bEnable == TRUE)
				u32Vratio = (MS_U32)(src * SCALING_MULITPLIER) / dst;
			else
				u32Vratio = 0;
		}
		KDrv_HW_GOP_SetStep1HVSPVscale(u8GOP, bEnable, u32Vratio);
		goto SUCCESS;
	}

	if (dst > src) {
		if (bEnable) {
			if (!pGOPCtx->pGopChipProperty->bGOPWithVscale[u8GOP]) {
				if ((src < 1) || (dst < 1)) {
					GOP_D_ERR
					    ("%s,Invalid Vscale size,u8GOP%d, src=0x%x, dst=0x%x\n",
					     __func__, u8GOP, src, dst);
				} else {
					src = src - 1;
					dst = dst - 1;
				}
			}
			u64Vratio = (MS_U64) (src) * SCALING_MULITPLIER;

			if (u64Vratio % (MS_U64) dst != 0) {
				if (pGOPCtx->pGopChipProperty->bPixelModeSupport)
					u64Vratio = (u64Vratio / (MS_U64) dst);
				else
					u64Vratio = (u64Vratio / (MS_U64) dst) + 1;

			} else {
				u64Vratio /= (MS_U64) dst;
			}
		} else {
			u64Vratio = SCALING_MULITPLIER;
		}

		MDrv_GOP_GWIN_Get_StretchWin(pGOPCtx, u8GOP, &u16x, &u16y, &u16w, &u16h);
		KDrv_HW_GOP_GWIN_Set_VSCALE(u8GOP, 0, u64Vratio);
	} else {
		KDrv_HW_GOP_GWIN_Set_VSCALE(u8GOP, 0, SCALING_MULITPLIER);
	}

#ifdef OUTPUT_VALID_SIZE_PATCH
	KDrv_HW_GOP_SetGopVaildV(u8GOP, GOP_OUTVALID_DEFAULT);
#else
	KDrv_HW_GOP_SetGopVaildV(u8GOP, dst);
#endif

SUCCESS:
	return;
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
		eHWHMode = E_HW_GOP_HSTRCH_DUPLICATE;
		break;
	case E_DRV_GOP_HSTRCH_6TAPE_NEAREST:
		eHWHMode = E_HW_GOP_HSTRCH_6TAPE_NEAREST;
		break;
	case E_DRV_GOP_HSTRCH_6TAPE:
	case E_DRV_GOP_HSTRCH_6TAPE_LINEAR:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN0:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN1:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN2:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN3:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN4:
	case E_DRV_GOP_HSTRCH_6TAPE_GAIN5:
	case E_DRV_GOP_HSTRCH_2TAPE:
		eHWHMode = E_HW_GOP_HSTRCH_6TAPE_LINEAR;
		break;
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
		eHWHMode = E_HW_GOP_HSTRCH_4TAPE;
		break;
	default:
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
		eHWVMode = E_HW_GOP_VSTRCH_DUPLICATE;
		break;
	case E_DRV_GOP_VSTRCH_NEAREST:
		eHWVMode = E_HW_GOP_VSTRCH_NEAREST;
		break;
	case E_DRV_GOP_VSTRCH_LINEAR:
	case E_DRV_GOP_VSTRCH_LINEAR_GAIN0:
	case E_DRV_GOP_VSTRCH_LINEAR_GAIN1:
		eHWVMode = E_HW_GOP_VSTRCH_LINEAR;
		break;
	case E_DRV_GOP_VSTRCH_LINEAR_GAIN2:
		eHWVMode = E_HW_GOP_VSTRCH_BILINEAR;
		break;
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
		GOP_D_ERR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return;
	}

	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);
	KDrv_HW_GOP_GWIN_PixelAlphaEnable(u8GOP, u8win, bEnable, u8coef);
	GOP_GWIN_UpdateReg(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) MDrv_DumpGopByGwinId(pGOPCtx, u8win));
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
		GOP_D_ERR("\n[%s] not support gwin id:%d in this chip version", __func__,
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
	default:
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
		GOP_D_ERR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return;
	}
	GOP_D_INFO("MDrv_GOP_GWIN_Enable(gId=%d) == %d\n", u8win, bEnable);

	pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->bGWINEnable[u8win] = bEnable;
	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);

	KDrv_HW_GOP_GWIN_Enable(u8GOP, u8win, bEnable);
	GOP_GWIN_UpdateReg(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) MDrv_DumpGopByGwinId(pGOPCtx, u8win));
}

static GOP_Result _GOPFBAddCheck(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopnum,
				 DRV_GOP_GWIN_INFO *pinfo)
{
	MS_U8 u8MiuSel;
	MS_PHY phyOffset = 0;

	_phy_to_miu_offset(u8MiuSel, phyOffset, pinfo->u64DRAMRBlkStart);
	pinfo->u64DRAMRBlkStart = phyOffset;
	if (_IsGopNumVaild(pGOPCtx, gopnum) == FALSE) {
		GOP_D_ERR("\n[%s] not support gop id:%d in this chip version", __func__,
			  gopnum);
		return GOP_FAIL;
	}

	return GOP_SUCCESS;
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
		GOP_D_ERR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return GOP_FAIL;
	}
	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);

	if (_IsGopNumVaild(pGOPCtx, u8GOP) == FALSE) {
		GOP_D_ERR("\n[%s] not support gop id:%d in this chip version", __func__, u8GOP);
		return GOP_FAIL;
	}

	if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bUse3x3MartixCSC == TRUE) {
			if (pGOPDrvLocalCtx->apiCtxLocal.pGOPCtxShared->s32OutputColorType[u8GOP] ==
			    GOP_OUTPUTCOLOR_INVALID) {
				enGopOutColor = DRV_GOPOUT_RGB;
				_GOP_GWIN_SetColorMatrix(enGopOutColor, pinfo->clrType,
							 &stHWGopCscTbl);
				KDrv_HW_GOP_SetCSCCtrl(u8GOP, TRUE, &stHWGopCscTbl);
			} else {
				MS_GOP_CTX_LOCAL ctxLocal = pGOPDrvLocalCtx->apiCtxLocal;
				MS_GOP_CTX_SHARED *pShared = ctxLocal.pGOPCtxShared;

				enGopOutColor =
				    (GOP_OupputColor) (pShared->s32OutputColorType[u8GOP]);
				_GOP_GWIN_SetColorMatrix(enGopOutColor, pinfo->clrType,
							 &stHWGopCscTbl);
				KDrv_HW_GOP_SetCSCCtrl(u8GOP, TRUE, &stHWGopCscTbl);
			}
	}

	switch (u8GOP) {
	case E_GOP0:
		if (_GOPFBAddCheck(pGOPCtx, 0, pinfo) == GOP_SUCCESS)
			GOP_SetGop0GwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
		else
			return GOP_FAIL;

		break;
	case E_GOP1:
		if (_GOPFBAddCheck(pGOPCtx, 1, pinfo) == GOP_SUCCESS)
			GOP_SetGop1GwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
		else
			return GOP_FAIL;

		break;
	case E_GOP2:
	case E_GOP3:
	case E_GOP4:
	case E_GOP5:
		if (_GOPFBAddCheck(pGOPCtx, u8GOP, pinfo) == GOP_SUCCESS)
			GOP_SetGopExtendGwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
		else
			return GOP_FAIL;
		break;
	default:
		break;
	}

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_GWIN_GetGwinInfo(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win,
				     DRV_GOP_GWIN_INFO *pinfo)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	MS_U8 u8GOP;
	GOP_WinFB_INFO pwinFB;

	if (_IsGwinIdValid(pGOPCtx, u8win) == FALSE) {
		GOP_D_ERR("\n[%s] not support gwin id:%d in this chip version", __func__,
			  u8win);
		return GOP_FAIL;
	}

	if (pGOPCtx->pGOPCtxShared->gwinMap[u8win].u32CurFBId > DRV_MAX_GWIN_FB_SUPPORT) {
		GOP_D_ERR("[%s][%d] WrongFBID=%td\n", __func__, __LINE__,
			  (ptrdiff_t) pGOPCtx->pGOPCtxShared->gwinMap[u8win].u32CurFBId);
		return GOP_FAIL;
	}
#if WINFB_INSHARED
	pwinFB = pGOPCtx->pGOPCtxShared->winFB[pGOPCtx->pGOPCtxShared->gwinMap[u8win].u32CurFBId];
#else
	pwinFB = pGOPCtx->winFB[pGOPCtx->pGOPCtxShared->gwinMap[u8win].u32CurFBId];
#endif

	pinfo->u16RBlkHPixSize = pwinFB.width;
	pinfo->u16RBlkVPixSize = pwinFB.height;

	u8GOP = MDrv_DumpGopByGwinId(pGOPCtx, u8win);
	switch (u8GOP) {
	case E_GOP0:
		GOP_ReadGop0GwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
		break;
	case E_GOP1:
		GOP_ReadGop1GwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
		break;
	case E_GOP2:
	case E_GOP3:
	case E_GOP4:
	case E_GOP5:
		GOP_ReadGopExtendGwinInfo(pGOPDrvLocalCtx, u8win, pinfo);
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
		GOP_D_ERR("\n[%s] not support gwin id:%d in this chip version", __func__,
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

	if (_IsGopNumVaild((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOPNum)
		== FALSE) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__,
			  u8GOPNum);
		return;
	}

	KDrv_HW_GOP_IsGOPMirrorEnable(u8GOPNum, bHMirror, bVMirror);

}

void MDrv_GOP_GWIN_EnableHMirror(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_BOOL bEnable)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (_IsGopNumVaild((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOPNum)
		== FALSE) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__,
			  u8GOPNum);
		return;
	}

	KDrv_HW_GOP_GWIN_HMirrorEnable(u8GOPNum, bEnable);

}


void MDrv_GOP_GWIN_EnableVMirror(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_BOOL bEnable)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (_IsGopNumVaild((MS_GOP_CTX_LOCAL *) pGOPDrvLocalCtx, u8GOPNum)
		== FALSE) {
		GOP_D_ERR("\n[%s] not support gop%d in this chip version!!", __func__,
			  u8GOPNum);
		return;
	}

	KDrv_HW_GOP_GWIN_VMirrorEnable(u8GOPNum, bEnable);

}

/******************************************************************************/
/// Set Scaler VOP New blending level
/******************************************************************************/
void MDrv_GOP_SetVOPNBL(MS_GOP_CTX_LOCAL *pGOPCtx, MS_BOOL bEnable)
{
	KDrv_HW_GOP_VopNblEnable(bEnable);
}

void MDrv_GOP_GWIN_UpdateReg(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	GOP_GWIN_UpdateReg(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) u8GOP);
}

void MDrv_GOP_GWIN_UpdateRegWithSync(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bSync)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (MDrv_GOP_GWIN_IsForceWrite(pGOPCtx, u8GOP) == TRUE)
		GOP_GWIN_TriggerRegWriteIn(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) u8GOP, TRUE, bSync);
	else
		GOP_GWIN_TriggerRegWriteIn(pGOPDrvLocalCtx, (E_DRV_GOP_TYPE) u8GOP, FALSE, bSync);

}

void MDrv_GOP_CtrlRst(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bRst)
{
	KDrv_HW_GOP_RstCtrl(u8GOP, bRst);
}

void MDrv_GOP_GWIN_UpdateRegWithMaskSync(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U16 u16GopMask,
					 MS_BOOL bSync)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	MS_U8 u8MaxGOPNum = MDrv_GOP_GetMaxGOPNum(pGOPCtx);
	MS_U16 u16GopAck = 0;
	MS_U32 goptimeout = 0;
	MS_U16 u16GopIdx = 0;

	for (u16GopIdx = 0; u16GopIdx < u8MaxGOPNum; u16GopIdx++) {
		if ((u16GopMask & (1 << u16GopIdx)) != 0)
			break;
	}
	if (u16GopIdx == u8MaxGOPNum)
		return;

	if (pGOPDrvLocalCtx->gop_gwin_frwr) {
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u16GopIdx, TRUE, E_HW_GOP_FORCEWRITE);
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u16GopIdx, FALSE, E_HW_GOP_FORCEWRITE);
	} else {
		KDrv_HW_GOP_GWIN_TriggerRegWriteIn((MS_U8) u16GopMask, TRUE,
						   E_HW_GOP_WITHSYNC_MULTIGOPUPDATED);
		for (u16GopIdx = 0; u16GopIdx < u8MaxGOPNum; u16GopIdx++) {
			if ((u16GopMask & (1 << u16GopIdx)) != 0)
				break;
		}
		if (u16GopIdx == u8MaxGOPNum)
			return;

		if (bSync == TRUE) {
			MS_U32 u32DelayTimems = 0;
			MS_U32 u32TimeoutCnt = 0;

			_GetGOPAckDelayTimeAndCnt(&u32DelayTimems, &u32TimeoutCnt);
			do {
				goptimeout++;
				u16GopAck = (MS_U16) KDrv_HW_GOP_GetRegUpdated((MS_U8) u16GopIdx);
				if (u32DelayTimems != 0)
					MsOS_DelayTask(u32DelayTimems);

			} while ((!u16GopAck) && (goptimeout <= u32TimeoutCnt));

			// Perform force write if wr timeout.
			if (goptimeout > u32TimeoutCnt) {
				//printf("Perform fwr if wr timeout!!\n");
				KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u16GopIdx, TRUE,
								   E_HW_GOP_FORCEWRITE);
				KDrv_HW_GOP_GWIN_TriggerRegWriteIn(u16GopIdx, FALSE,
								   E_HW_GOP_FORCEWRITE);
			}

		}
	}
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

//-------------------------------------------------------------------------------------------------
/// Set GOP Hsync Pipeline Delay Offset
//-------------------------------------------------------------------------------------------------
MS_U16 MDrv_GOP_GetHPipeOfst(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP_num, DRV_GOPDstType GopDst)
{
	MS_U8 u8Gop = 0, i = 0;
	MS_U16 u16Offset = 0;
	MS_BOOL bHDREnable = FALSE;
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (!_IsGopNumVaild(pGOPCtx, u8GOP_num)) {
		GOP_D_ERR("\n[%s] not support gop id:%d in this chip version", __func__,
			  u8GOP_num);
		return GOP_FAIL;
	}

	switch (GopDst) {
	case E_DRV_GOP_DST_BYPASS:
		for (i = E_GOP_FRC_MUX0; i <= E_GOP_FRC_MUX3; i++) {
			MDrv_GOP_GWIN_GetMux(pGOPCtx, &u8Gop, (Gop_MuxSel) i);
			if (u8Gop == u8GOP_num) {
				u16Offset =
				    i / E_GOP_FRC_MUX2 *
				    pGOPDrvLocalCtx->pGopChipPro->GOP_MUX_Delta;
				break;
			}
		}
		break;
	case E_DRV_GOP_DST_OP0:
		{
			GOP_CHIP_PROPERTY *pGOPProp = pGOPDrvLocalCtx->pGopChipPro;

			MDrv_GOP_IsHDREnabled(pGOPCtx, &bHDREnable);
			if (bHDREnable == FALSE) {
				for (i = 0; i < pGOPDrvLocalCtx->pGopChipPro->u8OpMuxNum; i++) {
					MDrv_GOP_GWIN_GetMux(pGOPCtx, &u8Gop, (Gop_MuxSel) i);
					if (u8Gop == u8GOP_num) {
						u16Offset =
						    pGOPProp->GOP_Mux_Offset[i] *
						    pGOPProp->GOP_MUX_Delta;
						break;
					}
				}
			}
			break;
		}
	default:
		for (i = 0; i < pGOPDrvLocalCtx->pGopChipPro->u8OpMuxNum; i++) {
			MDrv_GOP_GWIN_GetMux(pGOPCtx, &u8Gop, (Gop_MuxSel) i);
			if (u8Gop == u8GOP_num) {
				u16Offset =
				    pGOPDrvLocalCtx->pGopChipPro->GOP_Mux_Offset[i] *
				    pGOPDrvLocalCtx->pGopChipPro->GOP_MUX_Delta;
				break;
			}
		}
		break;
	}

	return u16Offset;
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

void MDrv_GOP_GWIN_Load_HStretchModeTable(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
					  DRV_GOPStrchHMode HStrchMode)
{
	switch (HStrchMode) {
	case E_DRV_GOP_HSTRCH_NEW4TAP_45:
	case E_DRV_GOP_HSTRCH_NEW4TAP_50:
	case E_DRV_GOP_HSTRCH_NEW4TAP_55:
	case E_DRV_GOP_HSTRCH_NEW4TAP_65:
	case E_DRV_GOP_HSTRCH_NEW4TAP_75:
	case E_DRV_GOP_HSTRCH_NEW4TAP_85:
	case E_DRV_GOP_HSTRCH_NEW4TAP_95:
	case E_DRV_GOP_HSTRCH_NEW4TAP_100:
	case E_DRV_GOP_HSTRCH_NEW4TAP_105:
		KDrv_HW_GOP_GWIN_Load_HStretchModeTable(u8GOP, _GopHStretchTable[HStrchMode]);
		break;
	default:
		break;

	}
}

void MDrv_GOP_GWIN_Load_VStretchModeTable(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
					  DRV_GOPStrchVMode VStrchMode)
{
	switch (VStrchMode) {
	case E_DRV_GOP_VSTRCH_4TAP:
	case E_DRV_GOP_HSTRCH_V4TAP_100:
		KDrv_HW_GOP_GWIN_Load_VStretchModeTable(u8GOP, _GopVStretchTable[VStrchMode]);
		break;
	default:
		break;

	}
}

MS_U16 MDrv_GOP_GetBPP(MS_GOP_CTX_LOCAL *pGOPCtx, DRV_GOPColorType fbFmt)
{
	MS_U16 u16Bpp = 0;

	switch (fbFmt) {
	case E_DRV_GOP_COLOR_RGB565:
	case E_DRV_GOP_COLOR_ARGB1555:
	case E_DRV_GOP_COLOR_RGBA5551:
	case E_DRV_GOP_COLOR_ARGB4444:
	case E_DRV_GOP_COLOR_RGBA4444:
	case E_DRV_GOP_COLOR_YUV422:
		u16Bpp = 16;
		break;
	case E_DRV_GOP_COLOR_RGBA8888:
	case E_DRV_GOP_COLOR_BGRA8888:
	case E_DRV_GOP_COLOR_ARGB8888:
	case E_DRV_GOP_COLOR_ABGR8888:
		u16Bpp = 32;
		break;
	default:
		u16Bpp = 0xFFFF;
		break;
	}

	return u16Bpp;
}

/********************************************************************************/
/// Set GOP OC(OSD Compression)
/********************************************************************************/
GOP_Result MDrv_GOP_OC_SetOCEn(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_BOOL bOCEn)
{
	MS_BOOL bHWRegRet;
	GOP_Result ret;

	bHWRegRet = KDrv_HW_GOP_OCEnable(u8GOPNum, bOCEn);
	if (bHWRegRet)
		ret = GOP_SUCCESS;
	else
		ret = GOP_FAIL;

	return ret;

}

#ifdef CONFIG_GOP_AFBC_FEATURE
GOP_Result MDrv_GOP_AFBC_Core_Reset(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bAFBC_Support[u8GOP] == FALSE) {
		GOP_D_ERR("[%s] GOP AFBC mode not support GOP %d\n", __func__, u8GOP);
		return GOP_FUN_NOT_SUPPORTED;
	}

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_AFBC_Core_Enable(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bEna)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bAFBC_Support[u8GOP] == FALSE) {
		GOP_D_ERR("[%s] GOP AFBC mode not support GOP %d\n", __func__, u8GOP);
		return GOP_FUN_NOT_SUPPORTED;
	}

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_GWIN_AFBCMode(MS_GOP_CTX_LOCAL *pGOPCtx, MS_BOOL u8GOP, MS_BOOL bEnable,
				  MS_U8 eCTL)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bAFBC_Support[u8GOP] == FALSE) {
		GOP_D_ERR("[%s] GOP AFBC mode not support GOP %d\n", __func__, u8GOP);
		return GOP_FUN_NOT_SUPPORTED;
	}

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_GWIN_AFBCSetWindow(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
				       DRV_GOP_AFBC_Info *pinfo, MS_BOOL bChangePitch)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;

	if (pGOPDrvLocalCtx->apiCtxLocal.pGopChipProperty->bAFBC_Support[u8GOP] == FALSE) {
		GOP_D_ERR("[%s] GOP AFBC mode not support GOP %d\n", __func__, u8GOP);
		return GOP_FUN_NOT_SUPPORTED;
	}

	return GOP_SUCCESS;
}
#endif

void MDrv_GOP_SelfFirstHs(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8Gop, MS_BOOL bEnable)
{
	KDrv_HW_GOP_FirstHsEnable(u8Gop, bEnable);
}

GOP_Result MDrv_GOP_IsHDREnabled(MS_GOP_CTX_LOCAL *pGOPCtx, MS_BOOL *pbHDREnable)
{
	*pbHDREnable = KDrv_HW_GOP_IsHDREnabled();

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_SetDbgLevel(EN_GOP_DEBUG_LEVEL level)
{
#ifndef STI_PLATFORM_BRING_UP
	u32GOPDbgLevel_drv = level;
#endif
	return GOP_SUCCESS;
}

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

GOP_Result MDrv_GOP_VOP_Path_Sel(MS_GOP_CTX_LOCAL *pGOPCtx, EN_GOP_VOP_PATH_MODE enGOPPath)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	EN_HW_GOP_VOPPATH_SEL eHWVopSelMode = E_HW_GOP_VOPPATH_DEFAULT;

	if (pGOPDrvLocalCtx == NULL)
		return GOP_FAIL;

	switch (enGOPPath) {
	case E_GOP_VOPPATH_DEF:
		eHWVopSelMode = E_HW_GOP_VOPPATH_DEFAULT;
		break;
	case E_GOP_VOPPATH_BEF_RGB3DLOOKUP:
		eHWVopSelMode = E_HW_GOP_VOPPATH_BEF_RGB3DLOOKUP;
		break;
	case E_GOP_VOPPATH_BEF_PQGAMMA:
		eHWVopSelMode = E_HW_GOP_VOPPATH_BEF_PQGAMMA;
		break;
	case E_GOP_VOPPATH_AFT_PQGAMMA:
		eHWVopSelMode = E_HW_GOP_VOPPATH_AFT_PQGAMMA;
		break;
	default:
		break;
	}
	KDrv_HW_GOP_SetVOPPathSel(eHWVopSelMode);

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_CSC_Tuning(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U32 u32GOPNum,
			       ST_GOP_CSC_PARAM *pstCSCParam)
{
	GOP_CTX_DRV_LOCAL *pGOPDrvLocalCtx = (GOP_CTX_DRV_LOCAL *) pGOPCtx;
	ST_DRV_GOP_CFD_OUTPUT stCFDOutput;
	ST_HW_GOP_CSC_TABLE stHWGopCscTbl;

	if (pGOPDrvLocalCtx == NULL || pGOPCtx == NULL || pstCSCParam == NULL) {
		GOP_D_ERR("[%s][%d] Local Ctx or csc param is null\n", __func__, __LINE__);
		return GOP_FAIL;
	}

	if (pGOPDrvLocalCtx->pGopChipPro->bSupportCSCTuning == FALSE) {
		GOP_D_WARN("[%s][%d] GOP_FUN_NOT_SUPPORTED!!!\n", __func__, __LINE__);
		return GOP_FUN_NOT_SUPPORTED;
	}

	if (pstCSCParam->bCscEnable == TRUE) {
		if ((pstCSCParam->enInputFormat == E_GOP_CFD_CFIO_MAX)
		    || pstCSCParam->enOutputFormat == E_GOP_CFD_CFIO_MAX) {
			GOP_D_WARN
			    ("[%s][%d] InputFormat type is E_GOP_CFD_CFIO_MAX\n",
			     __func__, __LINE__);
			return GOP_FAIL;
		}
		if ((pstCSCParam->enInputDataFormat == E_GOP_CFD_MC_FORMAT_MAX)
		    || pstCSCParam->enOutputDataFormat == E_GOP_CFD_MC_FORMAT_MAX) {
			GOP_D_WARN
			    ("[%s][%d]DataFormat is E_GOP_CFD_MC_FORMAT_MAX\n",
			     __func__, __LINE__);
			return GOP_FAIL;
		}
		memset(&stCFDOutput, 0, sizeof(ST_DRV_GOP_CFD_OUTPUT));
		memcpy(&(pGOPCtx->pGOPCtxShared->stCSCParam[u32GOPNum]), pstCSCParam,
		       sizeof(ST_GOP_CSC_PARAM));
		//Fix me, it should call xc_alg not gflip
		//if(MDrv_GOP_GFLIP_CSC_Tuning(pGOPCtx,u32GOPNum,
		//	pstCSCParam, &stCFDOutput) == FALSE) {
		//	GOP_D_ERR("[%s][%d]MDrv_GOP_GFLIP_CSC_Tuning  Fail\n",
		//		__func__,__LINE__);
		//	return GOP_FAIL;
		//}

		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].u32Version =
		//	ST_GOP_CSC_TABLE_VERSION;
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].u32Length =
		//	sizeof(ST_GOP_CSC_TABLE);
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].u16CscControl =
		//	stCFDOutput.u16CSCValue[0];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[0][0] =
		//	stCFDOutput.u16CSCValue[1];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[0][1] =
		//	stCFDOutput.u16CSCValue[2];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[0][2] =
		//	stCFDOutput.u16CSCValue[3];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[1][0] =
		//	stCFDOutput.u16CSCValue[4];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[1][1] =
		//	stCFDOutput.u16CSCValue[5];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[1][2] =
		//	stCFDOutput.u16CSCValue[6];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[2][0] =
		//	stCFDOutput.u16CSCValue[7];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[2][1] =
		//	stCFDOutput.u16CSCValue[8];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].stCSCMatrix.Matrix[2][2] =
		//	stCFDOutput.u16CSCValue[9];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].u16BrightnessOffsetR =
		//	stCFDOutput.u16BriValue[2];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].u16BrightnessOffsetG =
		//	stCFDOutput.u16BriValue[1];
		//pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum].u16BrightnessOffsetB =
		//	stCFDOutput.u16BriValue[0];

		//memset(&stHWGopCscTbl, 0, sizeof(ST_HW_GOP_CSC_TABLE));
		//stHWGopCscTbl.u16CscControl = stCFDOutput.u16CSCValue[0];
		//stHWGopCscTbl.Matrix[0][0] = stCFDOutput.u16CSCValue[1];
		//stHWGopCscTbl.Matrix[0][1] = stCFDOutput.u16CSCValue[2];
		//stHWGopCscTbl.Matrix[0][2] = stCFDOutput.u16CSCValue[3];
		//stHWGopCscTbl.Matrix[1][0] = stCFDOutput.u16CSCValue[4];
		//stHWGopCscTbl.Matrix[1][1] = stCFDOutput.u16CSCValue[5];
		//stHWGopCscTbl.Matrix[1][2] = stCFDOutput.u16CSCValue[6];
		//stHWGopCscTbl.Matrix[2][0] = stCFDOutput.u16CSCValue[7];
		//stHWGopCscTbl.Matrix[2][1] = stCFDOutput.u16CSCValue[8];
		//stHWGopCscTbl.Matrix[2][2] = stCFDOutput.u16CSCValue[9];
		//stHWGopCscTbl.u16BrightnessOffsetR = stCFDOutput.u16BriValue[2];
		//stHWGopCscTbl.u16BrightnessOffsetG = stCFDOutput.u16BriValue[1];
		//stHWGopCscTbl.u16BrightnessOffsetB = stCFDOutput.u16BriValue[0];
		//KDrv_HW_GOP_SetCSCCtrl((MS_U8)u32GOPNum, TRUE, &stHWGopCscTbl);

	} else {
		if (_MDrv_GOP_CSC_ParamInit(&(pGOPCtx->pGOPCtxShared->stCSCParam[u32GOPNum])) ==
		    FALSE) {
			GOP_D_ERR("[%s][%d] _MDrv_GOP_CSC_ParamInit  Fail\n", __func__, __LINE__);
			return GOP_FAIL;
		}
		if (_MDrv_GOP_CSC_TableInit(&(pGOPCtx->pGOPCtxShared->stCSCTable[u32GOPNum])) ==
		    FALSE) {
			GOP_D_ERR("[%s][%d] _MDrv_GOP_CSC_TableInit  Fail\n", __func__, __LINE__);
			return GOP_FAIL;
		}

		memset(&stHWGopCscTbl, 0, sizeof(ST_HW_GOP_CSC_TABLE));
		KDrv_HW_GOP_SetCSCCtrl((MS_U8) u32GOPNum, FALSE, &stHWGopCscTbl);
	}

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_AutoDectBuf(MS_GOP_CTX_LOCAL *pstGOPCtx,
				ST_GOP_AUTO_DETECT_BUF_INFO *pstAutoDectInfo)
{
	MS_U8 u8MaxGOPNum = MDrv_GOP_GetMaxGOPNum(pstGOPCtx);

	if (pstAutoDectInfo == NULL) {
		GOP_D_ERR("[%s][%d] pointer is NULL\n", __func__, __LINE__);
		return GOP_FAIL;
	}

	if (pstAutoDectInfo->u8GOPNum >= u8MaxGOPNum) {
		GOP_D_ERR("[%s][%d] GOP %d is out of bound!!\n", __func__, __LINE__,
			  pstAutoDectInfo->u8GOPNum);
		return GOP_FAIL;
	}

	KDrv_GOP_AutoDectBuf(pstAutoDectInfo->u8GOPNum, pstAutoDectInfo->bEnable,
			     pstAutoDectInfo->u8AlphaTh);

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_GetOsdNonTransCnt(MS_GOP_CTX_LOCAL *pstGOPCtx, MS_U32 *pu32Count)
{
	KDrv_GOP_GetNonTransCnt(pu32Count);

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_SetCropWindow(MS_GOP_CTX_LOCAL *pstGOPCtx, MS_U32 u32GOPNum,
				  ST_GOP_CROP_WINDOW *pstCropWin)
{
	ST_HW_GOP_CROP_WINDOW stCropWin;

	memset(&stCropWin, 0, sizeof(ST_HW_GOP_CROP_WINDOW));
	stCropWin.bHwCropEn = pstCropWin->bCropEn;
	stCropWin.u16CropWinX0 = pstCropWin->u16CropWinXStart;
	stCropWin.u16CropWinX1 = pstCropWin->u16CropWinXEnd;
	stCropWin.u16CropWinY0 = pstCropWin->u16CropWinYStart;
	stCropWin.u16CropWinY1 = pstCropWin->u16CropWinYEnd;
	KDrv_HW_GOP_SetCropWindow((MS_U8) u32GOPNum, &stCropWin);

	return GOP_SUCCESS;
}

GOP_Result MDrv_GOP_GWIN_Trigger_MUX(MS_GOP_CTX_LOCAL *pstGOPCtx)
{
	KDrv_HW_GOP_Trigger_MUX();
	return GOP_SUCCESS;
}
