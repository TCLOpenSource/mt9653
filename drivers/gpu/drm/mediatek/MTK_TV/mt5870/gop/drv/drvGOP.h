/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRV_GOP_H_
#define _DRV_GOP_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MSOS_TYPE_LINUX)
#include <sys/types.h>
#include <unistd.h>
#elif defined(MSOS_TYPE_LINUX_KERNEL)
#define getpid() pInstance
#else
#define getpid() 0UL
#endif

#include "apiGOP.h"

#define GETPIDTYPE ptrdiff_t

#if (defined ANDROID) && (defined TV_OS)
#include <cutils/log.h>
#define printf LOGD
#ifndef LOGD			// android 4.1 rename LOGx to ALOGx
#define LOGD ALOGD
#endif
#endif

#ifdef MSOS_TYPE_LINUX
#include <assert.h>
#include <unistd.h>
#define APIGOP_ASSERT(_bool, pri) do {\
	if (!(_bool)) {\
		printf("\nAssert in %s,%d\n", __func__, __LINE__);\
		pri;\
		MsOS_DelayTask(100);\
		assert(0);\
	} \
} while (0)
#else
#define APIGOP_ASSERT(_bool, pri) do {\
	if (!(_bool)) {\
		printf("\nAssert in %s,%d\n", __func__, __LINE__);\
		pri;\
	} \
} while (0)
#endif

#define CONFIG_GOP_POOL_ARRANGE
#define CONFIG_GOP_GWIN_MISC
#define CONFIG_GOP_CONTRAST
#define CONFIG_GOP_GOP_VE_CAPTURE
#define CONFIG_GOP_TEST_PATTERN

//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Customize Define
//-------------------------------------------------------------------------------------------------
#define CUS_SETTING               0UL
#ifdef CONFIG_GOP_UTOPIA10
#ifdef INSTANT_PRIVATE
#undef INSTANT_PRIVATE
#endif
#else
#define INSTANT_PRIVATE           1UL
#endif
#if CUS_SETTING
#define WINFB_INSHARED            1UL
#define POOL_INSHARED             1UL
#define CURSOR_SUPPORT            1UL
#define ENABLE_GOP0_RBLK_MIRROR   FALSE
#ifndef DRV_MAX_GWIN_FB_SUPPORT
#define DRV_MAX_GWIN_FB_SUPPORT   0xFFFEUL
#endif
#define GFLIP_ENABLE              FALSE
#define BOOTLOGO_PATCH            TRUE
#define GOP_PIXEL_ALIGN           FALSE
#define GOP_SW_SCROLL             TRUE
#define GOP_3D_TYPE_SEL           E_DRV_3D_DUP_FULL
#define GOP_VSYNC_WAIT_BYSLEEP    FALSE
#else				//mstar default
#define WINFB_INSHARED            1UL
#define POOL_INSHARED             0UL
#define CURSOR_SUPPORT            0UL
#define ENABLE_GOP0_RBLK_MIRROR   TRUE
#ifndef DRV_MAX_GWIN_FB_SUPPORT
#define DRV_MAX_GWIN_FB_SUPPORT   32UL
#endif

#ifdef CONFIG_GOP_DISABLE_GFLIP
#define GFLIP_ENABLE              FALSE
#else
#define GFLIP_ENABLE              TRUE
#endif
#define BOOTLOGO_PATCH            FALSE
#define GOP_PIXEL_ALIGN           FALSE
#define GOP_SW_SCROLL             FALSE
#define GOP_3D_TYPE_SEL           E_DRV_3D_DUP_HALF
#define GOP_VSYNC_WAIT_BYSLEEP    TRUE
#endif

#ifdef GOP_UTOPIA2K
#define GOPDRIVER_BASE              KERNEL_MODE
#else
#define GOPDRIVER_BASE              0UL
#endif
//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#ifndef BIT
#define BIT(_bit_)	(1 << (_bit_))
#endif

#ifndef BMASK
#define BMASK(_bits_)	(BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))
#endif

#define UNUSED(var)                   (void)(var)

#define MAX_GOP_INFO_COUNT      6
#define INVALID_DRV_GOP_NUM                 0xFFUL
#define INVALID_GWIN_ID                 0xFFUL
#define GWIN_OSD_DEFAULT                ((MS_U8)g_pGOPCtxLocal->pGopChipProperty->TotalGwinNum)
#define INVALID_POOL_NEXT_FBID          0xFFFFFFFFUL
#define FB_FMT_AS_DEFAULT               0xFFFFUL
#define INVALID_CLIENT_PID              0x00UL	//Invalid PID
#define INVALID_GOP_MUX_VAL             0xFFFFUL

#define MIU_0                           0UL
#define MIU_1                           1UL

#define GOP_STRETCH_WIDTH_UNIT          2UL	// 2-pixel

#define GOP_STRETCH_TABLE_NUM           50UL
#define GOP_VSTRETCH_TABLE_NUM          32UL
#define GOP_REG_MAX                     128UL


#define SHARED_GWIN_MAX_COUNT           32UL
#define SHARED_GOP_MAX_COUNT            8UL
#define MAX_GOP_SW_SCROLLNUM            16UL

#define GOP_PUBLIC_UPDATE (SHARED_GOP_MAX_COUNT-1)

#define HAS_BIT(flag, bit)              ((flag) &  (1<<bit))
#define SET_BIT(flag, bit)              ((flag) |= (1<<bit))
#define RESET_BIT(flag, bit)            ((flag) &= (~(1<<bit)))

#define GOP_MIU_CLIENT_OC9              0x9UL
#define INVALID_BPP                     0xFFFFUL

#define GOP_FMT_RGB555_BLINK       0x0UL
#define GOP_FMT_RGB565             0x1UL
#define GOP_FMT_ARGB4444           0x2UL
#define GOP_FMT_FaBaFgBg2266       0x3UL
#define GOP_FMT_I8                 0x4UL
#define GOP_FMT_ARGB8888           0x5UL
#define GOP_FMT_ARGB1555           0x6UL
#define GOP_FMT_ABGR8888           0x7UL
#define GOP_FMT_ARGB1555_DST       0x8UL
#define GOP_FMT_YUV422             0x9UL
#define GOP_FMT_RGBA5551           10UL
#define GOP_FMT_RGBA4444           11UL
#define GOP_FMT_RGBA8888           12UL
#define GOP_FMT_BGR565             17UL
#define GOP_FMT_ABGR4444           18UL
#define GOP_FMT_ABGR1555           22UL
#define GOP_FMT_BGRA5551           26UL
#define GOP_FMT_BGRA4444           27UL
#define GOP_FMT_BGRA8888           28UL
#define GOP_FMT_GENERIC            0xFFFFUL

#define GOP_CAPS_OFFSET_GOPNUM_MSK BMASK(2:0)
#define GOP_CAPS_OFFSET_VOP_PATHSEL	(5)
#define GOP_CAPS_OFFSET_GOP0_CSC	(12)
#define GOP_CAPS_OFFSET_GOP0_H2TAP	(18)
#define GOP_CAPS_OFFSET_GOP0_HDUPLICATE	(19)
#define GOP_CAPS_OFFSET_GOP0_H4TAP	(21)
#define GOP_CAPS_OFFSET_GOP0_VSCALE_MODE_MSK BMASK(27:23)
#define GOP_CAPS_OFFSET_GOP0_VBILINEAR	(25)
#define GOP_CAPS_OFFSET_GOP0_V4TAP	(26)
#define GOP_CAPS_OFFSET_GOP_H2TAP_COMMON	(12)
#define GOP_CAPS_OFFSET_GOP_HDUPLICATE_COMMON	(13)
#define GOP_CAPS_OFFSET_GOP_H4TAP_COMMON	(15)
#define GOP_CAPS_OFFSET_GOP_VSCALE_MODE_MSK_COMMON BMASK(21:17)
#define GOP_CAPS_OFFSET_GOP_VBILINEAR_COMMON	(19)
#define GOP_CAPS_OFFSET_GOP_V4TAP_COMMON	(20)

#define GOP_MAJOR_IPVERSION_OFFSET_MSK	BMASK(15:8)
#define GOP_MAJOR_IPVERSION_OFFSET_SHT	8
#define GOP_MINOR_IPVERSION_OFFSET_MSK	BMASK(7:0)
#define GOP_3M_SERIES_MAJOR	(0)
#define GOP_M6_SERIES_MAJOR	(1)
#define GOP_8K_WIDTH	(7680)
#define GOP_8K_HEIGHT	(4320)
#define GOP_4K_WIDTH	(3840)
#define GOP_4K_HEIGHT	(2160)
#define GOP_SCALING_2TIMES	(0x80000)

#ifndef PAGE_SIZE
#define PAGE_SIZE                  4096UL
#endif
#define PER_BYTE_BITS              8UL
#define TLB_PER_ENTRY_SIZE         4UL
#define PER_MIU_TLB_ENTRY_COUNT    8UL
#define ADDRESSING_8BYTE_UNIT      8UL

///GOP_PALETTE_ENTRY_NUM
#define GOP_PALETTE_ENTRY_NUM_MAX   256UL

#define GOP_MAX_SUPPORT_DEFAULT      (6)
#define GOP_CFD_CSC_OUTPUT_NUM       (10)
#define GOP_CFD_BRI_OUTPUT_NUM       (3)
#define MAX_ML_CMD                   (100)

#define GOP_OUTPUTCOLOR_INVALID (-1)

#define GOP_DEFAULT_R_OFFSET (0x400)
#define GOP_DEFAULT_G_OFFSET (0x400)
#define GOP_DEFAULT_B_OFFSET (0x400)

#define GOP_OUTVALID_DEFAULT (0xFFFF)

#define RATIO_20BITS    (0x100000UL)	//2^20
#define SCALING_MULITPLIER  (RATIO_20BITS)

#define GOP_WordUnit    (32)

#define OUTPUT_VALID_SIZE_PATCH
//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
/// Define GOP driver/hal layer return code, this enum should sync with E_GOP_API_Result
	typedef enum {
		GOP_FAIL = 0,
		GOP_SUCCESS = 1,
		GOP_NON_ALIGN_ADDRESS,
		GOP_NON_ALIGN_PITCH,
		GOP_DEPEND_NOAVAIL,
		GOP_MUTEX_OBTAIN_FAIL,
		GOP_MUTEX_OBTAINED,

		GOP_INVALID_BUFF_INFO,
		GOP_INVALID_PARAMETERS,
		GOP_FUN_NOT_SUPPORTED,
		GOP_ENUM_NOT_SUPPORTED,
		GOP_CRT_GWIN_FAIL = 0xFE,
		GOP_CRT_GWIN_NOAVAIL = 0xFF,
	} GOP_Result;


/// Define GWIN color format.
	typedef enum {
		/// Color format RGB565.
		E_DRV_GOP_COLOR_RGB565 = E_GOP_COLOR_RGB565,
		/// Color format ARGB4444.
		E_DRV_GOP_COLOR_ARGB4444 = E_GOP_COLOR_ARGB4444,
		/// Color format ARGB8888.
		E_DRV_GOP_COLOR_ARGB8888 = E_GOP_COLOR_ARGB8888,
		/// Color format ARGB1555.
		E_DRV_GOP_COLOR_ARGB1555 = E_GOP_COLOR_ARGB1555,
		/// Color format ARGB8888.
		E_DRV_GOP_COLOR_ABGR8888 = E_GOP_COLOR_ABGR8888,
		/// Color format YUV422.
		E_DRV_GOP_COLOR_YUV422 = E_GOP_COLOR_YUV422,
		/// Color format RGBA5551.
		E_DRV_GOP_COLOR_RGBA5551 = E_GOP_COLOR_RGBA5551,
		/// Color format RGBA4444.
		E_DRV_GOP_COLOR_RGBA4444 = E_GOP_COLOR_RGBA4444,
		/// Color format RGBA8888.
		E_DRV_GOP_COLOR_RGBA8888 = E_GOP_COLOR_RGBA8888,
		/// Color format BGR565.
		E_DRV_GOP_COLOR_BGR565 = E_GOP_COLOR_BGR565,
		/// Color format ABGR4444.
		E_DRV_GOP_COLOR_ABGR4444 = E_GOP_COLOR_ABGR4444,
		/// Color format AYUV8888.
		E_DRV_GOP_COLOR_AYUV8888 = E_GOP_COLOR_AYUV8888,
		/// Color format ABGR1555.
		E_DRV_GOP_COLOR_ABGR1555 = E_GOP_COLOR_ABGR1555,
		/// Color format BGRA5551.
		E_DRV_GOP_COLOR_BGRA5551 = E_GOP_COLOR_BGRA5551,
		/// Color format BGRA4444.
		E_DRV_GOP_COLOR_BGRA4444 = E_GOP_COLOR_BGRA4444,
		/// Color format BGRA8888.
		E_DRV_GOP_COLOR_BGRA8888 = E_GOP_COLOR_BGRA8888,

		/// Invalid color format.
		E_DRV_GOP_COLOR_INVALID = E_GOP_COLOR_INVALID
	} DRV_GOPColorType;

/// Define Mux
	typedef enum {
		///Select gop output to mux0
		E_GOP_MUX0 = 0,
		/// Select gop output to mux1
		E_GOP_MUX1 = 1,
		/// Select gop output to mux2
		E_GOP_MUX2 = 2,
		/// Select gop output to mux3
		E_GOP_MUX3 = 3,
		/// Select gop output to mux4
		E_GOP_MUX4 = 4,
		///Select gop output to IP0
		E_GOP_IP0_MUX,
		/// Select gop output to FRC mux0
		E_GOP_FRC_MUX0,
		/// Select gop output to FRC mux1
		E_GOP_FRC_MUX1,
		/// Select gop output to FRC mux2
		E_GOP_FRC_MUX2,
		/// Select gop output to FRC mux3
		E_GOP_FRC_MUX3,
		/// Select gop output to BYPASS mux
		E_GOP_BYPASS_MUX0,
		/// Select gop output to VE1
		E_GOP_VE1_MUX,

		MAX_GOP_MUX_SUPPORT,
		E_GOP_MUX_INVALID,
	} Gop_MuxSel;

/// Define GOP destination displayplane type
	typedef enum {
		E_DRV_GOP_DST_IP0 = 0,
		E_DRV_GOP_DST_OP0 = 1,
		E_DRV_GOP_DST_FRC = 2,
		E_DRV_GOP_DST_BYPASS = 3,
		MAX_DRV_GOP_DST_SUPPORT,
		E_DRV_GOP_DST_INVALID,
	} DRV_GOPDstType;

/// Define GOP chip property for different chip characteristic.
	typedef struct __attribute__((__packed__)) {
		MS_BOOL bGop1GPalette;
		MS_BOOL bIgnoreIP1HPD;
		MS_BOOL bBnkForceWrite;
		MS_BOOL bPixelModeSupport;
		MS_BOOL b2Pto1PSupport;
		MS_BOOL bAFBC_Support[GOP_MAX_SUPPORT_DEFAULT];

		MS_BOOL bAutoAdjustMirrorHSize;
		MS_BOOL bGOPWithVscale[GOP_MAX_SUPPORT_DEFAULT];

		MS_S16 GOP_PD;
		MS_U16 GOP_OP1_PD;
		MS_U16 GOP_NonVS_PD_Offset;
		MS_U16 GOP_NonVS_DualOpPD_Offset;
		MS_U16 GOP_HDR_OP_PD;
		MS_U16 GOP_DUAL_OP_PD;

		MS_U16 GOP_MUX_Delta;
		MS_U16 GOP_Mux_Offset[GOP_MAX_SUPPORT_DEFAULT];
		MS_U16 GOP_Zorder_Mux_Offset[GOP_MAX_SUPPORT_DEFAULT];
		Gop_MuxSel GOP_MapLayer2Mux[GOP_MAX_SUPPORT_DEFAULT];
		Gop_MuxSel GOP_MapLayer2DualOpMux[GOP_MAX_SUPPORT_DEFAULT];
		MS_U16 GOP_DualRateMux_Offset[GOP_MAX_SUPPORT_DEFAULT];
		MS_U16 GOP_REF_POSITION_X;

		MS_U16 TotalGwinNum;
		MS_BOOL bOpInterlace;

		MS_BOOL bAFBCMIUSelDoubleBuffer;
		MS_BOOL bAFBC_Merge_GOP_Trig;
		MS_BOOL bGOPVscalePipeDelay[GOP_MAX_SUPPORT_DEFAULT];
		MS_BOOL bGOPNeedSplitMode4DualRate[GOP_MAX_SUPPORT_DEFAULT];
		MS_BOOL bOPHandShakeModeSupport;
		MS_BOOL bOPMuxDoubleBuffer;
		MS_BOOL bGOPAutoClkGating;
		MS_BOOL bV4tap256Phase;
		MS_BOOL bSupportV4tap[GOP_MAX_SUPPORT_DEFAULT];
		MS_BOOL bSupportV_BiLinear[GOP_MAX_SUPPORT_DEFAULT];
		MS_BOOL bSupportVOPPathSel;
		MS_U16 u16BefRGB3DLupPDOffset;
		MS_U16 u16BefPQGammaPDOffset;
		MS_U16 u16AftPQGammaPDOffset;
		MS_BOOL bSupportCSCTuning;
		MS_BOOL bSupportH6Tap_8Phase[GOP_MAX_SUPPORT_DEFAULT];
		MS_BOOL bSupportHDuplicate[GOP_MAX_SUPPORT_DEFAULT];
		MS_BOOL bSupportH4Tap_256Phase[GOP_MAX_SUPPORT_DEFAULT];
		MS_BOOL bUse3x3MartixCSC;
		MS_BOOL bSupportV2tap_16Phase[GOP_MAX_SUPPORT_DEFAULT];
		MS_BOOL b120HzDstOP;
		MS_U32 u32GOPNum;
		MS_U8 u8OpMuxNum;
		MS_VIRT VAdr[GOP_MAX_SUPPORT_DEFAULT];
		MS_VIRT PAdr[GOP_MAX_SUPPORT_DEFAULT];
		MS_U32 u32RegSize[GOP_MAX_SUPPORT_DEFAULT];
	} GOP_CHIP_PROPERTY;

/// GWIN Information
	typedef struct DLL_PACKED {
		MS_U16 u16DispVPixelStart;	//!< unit: pix
		MS_U16 u16DispVPixelEnd;	//!< unit: pix
		MS_U16 u16DispHPixelStart;	//!< unit: pix
		MS_U16 u16DispHPixelEnd;	//!< unit: pix
		MS_PHY u64DRAMRBlkStart;	//!< unit: Byte
		MS_U16 u16RBlkHPixSize;	//!< unit: pix
		MS_U16 u16RBlkVPixSize;	//!< unit: pix
		MS_U16 u16RBlkHRblkSize;	//!< unit: Byte
		MS_U16 u16WinX;	//!< unit: pix
		MS_U16 u16WinY;	//!< unit: pix
		MS_U32 u32scrX;	//!< unit: pix
		MS_U32 u32scrY;	//!< unit: pix
		DRV_GOPColorType clrType;	//!< color format of the buffer
	} DRV_GOP_GWIN_INFO;

/// Dump Window Information
	typedef struct DLL_PACKED {
		MS_U16 u16VPixelStart;	//!< unit: pix
		MS_U16 u16VPixelEnd;	//!< unit: pix
		MS_U16 u16HPixelStart;	//!< unit: pix
		MS_U16 u16HPixelEnd;	//!< unit: pix
		MS_PHY u64TFDRAMAddr;	//!< unit: Byte
		MS_PHY u64BFDRAMAddr;	//!< unit: Byte
		MS_U16 u16DRAMJumpLen;	//!< unit: Byte
		MS_U8 u8fmt;	//!< DWIN format: 0 UV7Y8, 1 UV8Y8, 2 ARGB8888, 3 RGB565
	} DRV_GOP_DWIN_INFO;

/// DWIN propert
	typedef struct DLL_PACKED {
		MS_U16 u16x;	///< x
		MS_U16 u16y;	///< y
		MS_U16 u16w;	///< width
		MS_U16 u16h;	///< height
		MS_PHY u64fbaddr0;	///< Top field DRAM address
		MS_PHY u64fbaddr1;	///< Bottom field DRAM address
		MS_U16 u16fbw;	///< Number of bytes per horizontal line
	} DRV_GOP_DwinProperty;

	typedef union DLL_PACKED {
		MS_U8 u8DWinIntInfo;
		struct DLL_PACKED {
			MS_U8 bDWinIntWADR:1;
			MS_U8 bDWinIntPROG:1;
			MS_U8 bDWinIntTF:1;
			MS_U8 bDWinIntBF:1;
			MS_U8 bDWinIntVS:1;
			MS_U8 reserved:3;
		} sDwinIntInfo;
	} DRV_GOP_DWinIntInfo;

/// Frame buffer attributes for GWIN
	typedef struct DLL_PACKED {
		MS_U8 gWinId;	///< id
		MS_U8 enable:1;	///< 4bits enable
		MS_U8 allocated:1;	///< 4bits

		MS_U16 x0;	///< MS_U16
		MS_U16 y0;	///< MS_U16
		MS_U16 x1;	///< MS_U16
		MS_U16 y1;	///< MS_U16
		MS_U16 width;	///< MS_U16
		MS_U16 height;	///< MS_U16
		MS_U16 pitch;	///< MS_U16
		MS_U16 fbFmt;	///< MS_U16

		MS_PHY phyAddr;	///< MS_U32
		MS_U32 size;	///< MS_U32

		MS_U8 next;	///< MS_U8
		MS_U8 prev;	///< MS_U8
		MS_U8 string;	//store which AP use this FB
		//MS_U8 flag;                ///< bit 0:scroll
		MS_U16 s_x;	///< MS_U16
		MS_U16 s_y;	///< MS_U16
		MS_U16 dispWidth;	///< MS_U16
		MS_U16 dispHeight;	///< MS_U16
	} DRV_Gwin_FB_Attr;

/// GWIN output color domain
	typedef enum {
		DRV_GOPOUT_RGB,	///< 0: output color RGB
		DRV_GOPOUT_YUV,	///< 1: output color YUV
	} GOP_OupputColor;

/// Transparent color format
	typedef enum {
		DRV_GOPTRANSCLR_FMT0,	//!< RGB mode transparent color
		DRV_GOPTRANSCLR_FMT1,	//!< index mode transparent color
		DRV_GOPTRANSCLR_FMT2,	//!< ARGB 8888 transparent color
		DRV_GOPTRANSCLR_FMT3,	//!< YUV mode transparent color
	} GOP_TransClrFmt;

/// Define Scaler GOP IP setting.
	typedef enum {
		MS_DRV_IP0_SEL_GOP0,
		MS_DRV_IP0_SEL_GOP1,
		MS_DRV_IP0_SEL_GOP2,
		MS_DRV_NIP_SEL_GOP0,
		MS_DRV_NIP_SEL_GOP1,
		MS_DRV_NIP_SEL_GOP2,
	} MS_IPSEL_GOP;

	typedef enum {
		E_DRV_MIXER_DST_IP = 0,
		E_DRV_MIXER_DST_MIXER = 1,
		E_DRV_MIXER_DST_OP = 2,
		E_DRV_MIXER_DST_VE = 3,
		E_DRV_MIXER_DST_OP1 = 4,
		MAX_DRV_MIXER_DST_SUPPORT,
		E_DRV_MIXER_DST_INVALID = 0xFF,
	} DRV_MixerDstType;

/// Define GOP H-Stretch mode.
	typedef enum {
		E_DRV_GOP_HSTRCH_6TAPE = 0,
		E_DRV_GOP_HSTRCH_DUPLICATE = 1,
		E_DRV_GOP_HSTRCH_6TAPE_LINEAR = 2,
		E_DRV_GOP_HSTRCH_6TAPE_NEAREST = 3,
		E_DRV_GOP_HSTRCH_6TAPE_GAIN0 = 4,
		E_DRV_GOP_HSTRCH_6TAPE_GAIN1 = 5,
		E_DRV_GOP_HSTRCH_6TAPE_GAIN2 = 6,
		E_DRV_GOP_HSTRCH_6TAPE_GAIN3 = 7,
		E_DRV_GOP_HSTRCH_6TAPE_GAIN4 = 8,
		E_DRV_GOP_HSTRCH_6TAPE_GAIN5 = 9,
		E_DRV_GOP_HSTRCH_4TAPE = 0xA,
		E_DRV_GOP_HSTRCH_2TAPE = 0xB,
		E_DRV_GOP_HSTRCH_NEW4TAP_45 = 0xC,
		E_DRV_GOP_HSTRCH_NEW4TAP_50 = 0xD,
		E_DRV_GOP_HSTRCH_NEW4TAP_55 = 0xE,
		E_DRV_GOP_HSTRCH_NEW4TAP_65 = 0xF,
		E_DRV_GOP_HSTRCH_NEW4TAP_75 = 0x10,
		E_DRV_GOP_HSTRCH_NEW4TAP_85 = 0x11,
		E_DRV_GOP_HSTRCH_NEW4TAP_95 = 0x12,
		E_DRV_GOP_HSTRCH_NEW4TAP_105 = 0x13,
		E_DRV_GOP_HSTRCH_NEW4TAP_100 = 0x14,
		MAX_GOP_HSTRETCH_MODE_SUPPORT,
	} DRV_GOPStrchHMode;

/// Define GOP V-Stretch mode.
	typedef enum {
		E_DRV_GOP_VSTRCH_LINEAR = 0,
		E_DRV_GOP_VSTRCH_DUPLICATE = 1,
		E_DRV_GOP_VSTRCH_NEAREST = 2,
		E_DRV_GOP_VSTRCH_LINEAR_GAIN0 = 3,
		E_DRV_GOP_VSTRCH_LINEAR_GAIN1 = 4,
		E_DRV_GOP_VSTRCH_LINEAR_GAIN2 = 5,
		E_DRV_GOP_VSTRCH_4TAP = 6,
		E_DRV_GOP_HSTRCH_V4TAP_100 = 7,
		MAX_GOP_VSTRETCH_MODE_SUPPORT,
	} DRV_GOPStrchVMode;

/// Define palette read type.
	typedef enum {
		/// Palette access from GOP0 RIU
		E_DRV_GOP_PAL_GOP_RIU = 0,
		/// Palette access from GOP0 REGDMA
		E_DRV_GOP_PAL_GOP_REGDMA = 1,
		/// Palette access from GOP1 RIU
		E_DRV_GOP1_PAL_GOP_RIU = 2,
		/// Palette access from GOP1 REGDMA
		E_DRV_GOP1_PAL_GOP_REGDMA = 3,
	} DRV_GopPalReadType;

/// Scroll direction
	typedef enum {
		GOP_DRV_SCROLL_NONE = 0,
		GOP_DRV_SCROLL_UP,	// bottom to top
		GOP_DRV_SCROLL_DOWN,	// top to bottom
		GOP_DRV_SCROLL_LEFT,	//right to left
		GOP_DRV_SCROLL_RIGHT,	//left to right
		GOP_DRV_SCROLL_SW,	// Scroll by sw
		GOP_DRV_SCROLL_KERNEL,	// Scroll by sw on kernel
		GOP_DRV_SCROLL_MAX,
	} E_GOP_SCROLL_TYPE;

//----------------------------------------------------------------------------
// GOP Reg Page Form
//----------------------------------------------------------------------------

	typedef enum {
		GOP_WINFB_POOL_NULL = 0,
		GOP_WINFB_POOL1 = 1,
		GOP_WINFB_POOL2 = 2
	} GOP_FBPOOL_ID;

	typedef enum {
		E_DRV_GOP_AFBC_SPILT = 0x1,
		E_DRV_GOP_AFBC_YUV_TRANSFER = 0x2,
		E_DRV_GOP_AFBC_ARGB8888 = 0x4,
		E_DRV_GOP_AFBC_RGB565 = 0x8,
		E_DRV_GOP_AFBC_RGB888 = 0x10,
		E_DRV_GOP_AFBC_MAX,
	} EN_DRV_GOP_AFBC_CNTL;

/// Define GOP FB string.
	typedef enum {
		E_DRV_GOP_FB_NULL,
		/// OSD frame buffer.
		E_DRV_GOP_FB_OSD,
		/// Mute frame buffer.
		E_DRV_GOP_FB_MUTE,
		/// subtitle frame buffer.
		E_DRV_GOP_FB_SUBTITLE,
		/// teltext frame buffer.
		E_DRV_GOP_FB_TELTEXT,
		/// MHEG5 frame buffer.
		E_DRV_GOP_FB_MHEG5,
		/// CC frame buffer.
		E_DRV_GOP_FB_CLOSEDCAPTION,
		/// AFBC frame buffer.
		E_DRV_GOP_FB_AFBC_SPLT_YUVTRNSFER_ARGB8888 = 0x100,
		E_DRV_GOP_FB_AFBC_NONSPLT_YUVTRS_ARGB8888,
		E_DRV_GOP_FB_AFBC_SPLT_NONYUVTRS_ARGB8888,
		E_DRV_GOP_FB_AFBC_NONSPLT_NONYUVTRS_ARGB8888,
		E_DRV_GOP_FB_AFBC_V1P2_ARGB8888,	//v1.2
		E_DRV_GOP_FB_AFBC_V1P2_RGB565,	//v1.2
		E_DRV_GOP_FB_AFBC_V1P2_RGB888,	//v1.2
	} EN_DRV_FRAMEBUFFER_STRING;

/// Frame buffer attributes for GWIN
	typedef struct __attribute__((__packed__)) {
		MS_PHY addr;	///< MS_U64  /*For 32/64 share memory align*/
		MS_U32 size;	///< MS_U32
		MS_U32 nextFBIdInPool;	///next winId in same pool

		MS_U16 x0;	///< MS_U16
		MS_U16 y0;	///< MS_U16
		MS_U16 x1;	///< MS_U16
		MS_U16 y1;	///< MS_U16
		MS_U16 width;	///< MS_U16
		MS_U16 height;	///< MS_U16
		MS_U16 pitch;	///< MS_U16
		MS_U16 fbFmt;	///< MS_U16

		MS_U16 s_x;	///< MS_U16
		MS_U16 s_y;	///< MS_U16
		MS_U16 dispWidth;	///< MS_U16
		MS_U16 dispHeight;	///< MS_U16

		GOP_FBPOOL_ID poolId;

		MS_U8 gWinId;
		MS_U8 string;	///store which AP use this FB
		MS_U8 enable:1;
		MS_U8 in_use:1;
		MS_U32 u32GOPClientId;	///<<store Client ID of using this FB
		MS_U8 obtain;
		EN_DRV_FRAMEBUFFER_STRING FBString;
	} GOP_WinFB_INFO;

	typedef struct __attribute__((__packed__)) {
		MS_U32 PalSWArray[GOP_PALETTE_ENTRY_NUM_MAX];
	} GOP_Palette_INFO;

	typedef struct __attribute__((__packed__)) {
		GOP_FBPOOL_ID poolId;
		MS_PHY GWinFB_Pool_BaseAddr;
		MS_U32 u32GWinFB_Pool_MemLen;
		MS_U32 u32FirstFBIdInPool;
	} GOP_FB_POOL;

	typedef struct __attribute__((__packed__)) {
		MS_U32 u32GOPClientId;
		MS_U32 u32CurFBId;
		MS_U16 u16SharedCnt;
		MS_BOOL bIsShared;
	} GWIN_MAP;

	typedef struct DLL_PACKED {
		MS_PHY u64NonMirrorFBAdr;
		MS_U16 u16NonMirrorHStr;
		MS_U16 u16NonMirrorHEnd;
		MS_U16 u16NonMirrorVStr;
		MS_U16 u16NonMirrorVEnd;
		MS_U16 u16NonMirrorGOP0WinX;
		MS_U16 u16NonMirrorGOP0WinY;
	} GOP_GwinMirror_Info;

///GOP Ignore init value
	typedef enum {
		E_DRV_GOP_MUX_IGNORE = 0x0001,
		E_DRV_GOP_GWIN_IGNORE = 0x0002,
		E_DRV_GOP_STRETCHWIN_IGNORE = 0x0004,
		E_DRV_GOP_PALETTE_IGNORE = 0x0008,
		E_DRV_GOP_SET_DST_OP_IGNORE = 0x0010,
		E_DRV_GOP_ENABLE_TRANSCLR_IGNORE = 0x0020,
		E_DRV_GOP_BOOTLOGO_VEOSDEN_IGNORE = 0x0040,
		E_DRV_GOP_ALL_IGNORE = 0xFFFF,
		E_DRV_GOP_IGNORE_DISABLE = 0x0000,
	} DRV_GOP_IGNOREINIT;

///the GOP config
	typedef struct DLL_PACKED {
		DRV_GOP_IGNOREINIT eGopIgnoreInit;
	} GOP_Config;

	typedef struct __attribute__((__packed__)) {
		MS_U32 u32ClientIdAllocator;
		MS_U32 u32LstGOPClientId;
		MS_U16 u16PnlWidth[SHARED_GOP_MAX_COUNT];
		MS_U16 u16PnlHeight[SHARED_GOP_MAX_COUNT];
		MS_U16 u16PnlHStr[SHARED_GOP_MAX_COUNT];
		MS_BOOL bHMirror;
		MS_BOOL bVMirror;
		MS_BOOL b16TileMode[SHARED_GOP_MAX_COUNT];
		MS_BOOL b32TileMode[SHARED_GOP_MAX_COUNT];
		MS_U16 ScrollBufNum;
		MS_U16 ScrollBufIdx;
		MS_U32 ScrollBuffer[MAX_GOP_SW_SCROLLNUM];

		GWIN_MAP gwinMap[SHARED_GWIN_MAX_COUNT];

		GOP_FB_POOL fbPool1;
		GOP_FB_POOL fbPool2;
		GOP_FBPOOL_ID fb_currentPoolId;

		MS_S32 s32OutputColorType[SHARED_GOP_MAX_COUNT];
		MS_BOOL bGopHasInitialized[SHARED_GOP_MAX_COUNT];
		MS_BOOL bGWINEnable[SHARED_GWIN_MAX_COUNT];
#if WINFB_INSHARED
		GOP_WinFB_INFO winFB[DRV_MAX_GWIN_FB_SUPPORT + 1];
#endif
		MS_U16 u16APIStretchWidth[SHARED_GOP_MAX_COUNT];
		MS_U16 u16APIStretchHeight[SHARED_GOP_MAX_COUNT];
		MS_U8 GOP_TestPatternMode;
		MS_U16 u16HScaleSrc[SHARED_GOP_MAX_COUNT];
		MS_U16 u16HScaleDst[SHARED_GOP_MAX_COUNT];
		MS_U16 u16VScaleDst[SHARED_GOP_MAX_COUNT];
		MS_BOOL bPixelMode[SHARED_GOP_MAX_COUNT];
		MS_BOOL bInitShared;
		MS_BOOL bDummyInit;
		MS_BOOL bEnableVsyncIntFlip;
		MS_S32 s32GOPLockTid;
		MS_U32 GOPLockPid;
		MS_BOOL bTLB[SHARED_GOP_MAX_COUNT];
		MS_PHY u64TLBAddress[SHARED_GOP_MAX_COUNT];
		GOP_Palette_INFO GOPPaletteInfo[SHARED_GOP_MAX_COUNT];

		MS_PHY phyGOPRegdmaAdr;
		MS_U32 u32GOPRegdmaLen;
		MS_S32 s32GOPStretchWinHOffset;
		MS_S32 s32GOPStretchWinVOffset;
		MS_U16 u16OrgOPmuxInfo;
		MS_U16 u16OrgOSDBInfo[SHARED_GOP_MAX_COUNT];
		ST_GOP_CSC_PARAM stCSCParam[SHARED_GOP_MAX_COUNT];
		ST_GOP_CSC_TABLE stCSCTable[SHARED_GOP_MAX_COUNT];
		MS_S32 s32PixelShiftPDOffset;
	} MS_GOP_CTX_SHARED;

	typedef struct DLL_PACKED {
		MS_GOP_CTX_SHARED *pGOPCtxShared;	//pointer to shared context paramemetrs;
		GOP_CHIP_PROPERTY *pGopChipProperty;
		GOP_Config sGOPConfig;	//GOP configs set for special cases
		MS_U32 u32GOPClientId;
		//for local GWIN memory pool

#if WINFB_INSHARED
		//GOP_WinFB_INFO winFB[MAX_GWIN_FB_SUPPORT+1]; //move to MS_GOP_CTX_SHARED
#else
		GOP_WinFB_INFO winFB[DRV_MAX_GWIN_FB_SUPPORT + 1];
#endif
		MS_U8 current_winId;


		MS_BOOL Gwin_H_Dup, Gwin_V_Dup;
		MS_U16 u16GOP_HMirrorRBLK_Adr;	//Consider 16Gwins totally
		MS_U16 u16GOP_VMirrorRBLK_Adr;	//Consider 16Gwins totally
		MS_U16 u16GOP_HMirror_HPos;	//Consider 16Gwins totally
		MS_U16 u16GOP_VMirror_VPos;	//Consider 16Gwins totally

		GOP_GwinMirror_Info sMirrorInfo[SHARED_GWIN_MAX_COUNT];
		MS_U8 MS_MuxGop[MAX_GOP_MUX_SUPPORT];
		MS_U8 u8ChgIpMuxGop;
		MS_BOOL IsChgMux;
		MS_BOOL IsClkClosed;
		MS_BOOL bUpdateRegOnce[SHARED_GOP_MAX_COUNT];
#ifdef CONFIG_GOP_TEST_PATTERN
		MS_U16 GOPBnk[SHARED_GOP_MAX_COUNT][GOP_REG_MAX];
		MS_U16 GWINBnk[SHARED_GOP_MAX_COUNT][GOP_REG_MAX];
		MS_U16 GOPStBnk[SHARED_GOP_MAX_COUNT][GOP_REG_MAX];
#endif
		MS_U16 GOP_FramePacking_Gap;

		MS_BOOL bInitFWR;
	} MS_GOP_CTX_LOCAL;

	typedef enum {
		E_DRV_GOP_Deldirect_Front,
		E_DRV_GOP_Deldirect_Back,
	} E_DRV_GOP_DelDirect;


	typedef enum {
		E_DRV_GOP_MAIN_WINDOW = 0,	///< main window if with PIP or without PIP
		E_DRV_GOP_SUB_WINDOW = 1,	///< sub window if PIP
		E_DRV_GOP_MAX_WINDOW	/// The max support window
	} E_DRV_GOP_SCALER_WIN;

	typedef enum {
		E_DRV_GOP_RBLKAddr,
		E_DRV_GOP_DRAMVstr,
		E_DRV_GOP_RBLKSize,
		E_DRV_GOP_ScrollHVStopAddr,
		E_DRV_GOP_RBLK3DAddr
	} E_DRV_GOP_AddrType;

	typedef enum {
		E_GOP0 = 0,
		E_GOP1 = 1,
		E_GOP2 = 2,
		E_GOP3 = 3,
		E_GOP4 = 4,
		E_GOP5 = 5,
		E_MIXER,
	} E_DRV_GOP_TYPE;

	typedef struct DLL_PACKED {
		MS_U8 GOP0;
		MS_U8 GOP1;
		MS_U8 GOP2;
		MS_U8 GOP3;
		MS_U8 GOP4;
		MS_U8 GOP5;
		MS_U8 MIXER;
	} GOP_TYPE_DEF;

	typedef struct DLL_PACKED {
		MS_PHY u64Addr;
		MS_U16 u16Pitch;
		MS_U16 u16Fmt;
	} DRV_GOP_CBFmtInfo;

	typedef struct DLL_PACKED {
		MS_PHY u64DRAMAddr;	//!< unit: Byte
		MS_U16 u16HPixelStart;	//!< unit: pix
		MS_U16 u16HPixelEnd;	//!< unit: pix
		MS_U16 u16VPixelStart;	//!< unit: pix
		MS_U16 u16VPixelEnd;	//!< unit: pix
		MS_U16 u16Pitch;	//!< unit: Byte
		MS_U8 u8Fmt;
	} DRV_GOP_AFBC_Info;

// scaler mirror mode
	typedef enum {
		E_DRV_GOP_XC_MIRROR_NORMAL,
		E_DRV_GOP_XC_MIRROR_H_ONLY,
		E_DRV_GOP_XC_MIRROR_V_ONLY,
		E_DRV_GOP_XC_MIRROR_HV,
		E_DRV_GOP_XC_MIRROR_MAX,
	} EN_DRV_GOP_XC_MIRRORMODE;

	typedef enum {
		E_DRV_GOP_VOPPATH_DEF = 0x0,
		E_DRV_GOP_VOPPATH_BEF_RGB3DLOOKUP = 0x1,
		E_DRV_GOP_VOPPATH_BEF_PQGAMMA = 0x2,
		E_DRV_GOP_VOPPATH_AFT_PQGAMMA = 0x3,
		E_DRV_GOP_VOPPATH_MAX,
	} EN_DRV_GOP_VOP_PATH;

	typedef struct DLL_PACKED {
		MS_U32 u32CSCAddr[GOP_CFD_CSC_OUTPUT_NUM];
		MS_U16 u16CSCValue[GOP_CFD_CSC_OUTPUT_NUM];
		MS_U16 u16CSCMask[GOP_CFD_CSC_OUTPUT_NUM];
		MS_U16 u16CSCClient[GOP_CFD_CSC_OUTPUT_NUM];
		MS_U8 u8CSCAdlData[GOP_CFD_CSC_OUTPUT_NUM];
		//CFD Output Brightness
		MS_U32 u32BriAddr[GOP_CFD_BRI_OUTPUT_NUM];
		MS_U16 u16BriValue[GOP_CFD_BRI_OUTPUT_NUM];
		MS_U16 u16BriMask[GOP_CFD_BRI_OUTPUT_NUM];
		MS_U16 u16BriClient[GOP_CFD_BRI_OUTPUT_NUM];
		MS_U8 u8BriAdlData[GOP_CFD_BRI_OUTPUT_NUM];
	} ST_DRV_GOP_CFD_OUTPUT;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

//======================================================================================
//  GOP Common utility
//======================================================================================
MS_U8 MDrv_DumpGopByGwinId(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U32 GwinID);
void *MDrv_GOP_GetShareMemory(MS_BOOL *pbNeedInitShared);
MS_GOP_CTX_LOCAL *Drv_GOP_Init_Context(void *pInstance, MS_BOOL *pbNeedInitShared);
void MDrv_GOP_Init(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_PHY u64GOP_REGDMABASE,
			   MS_U32 u32GOP_RegdmaLen, MS_BOOL bEnableVsyncIntFlip);
void MDrv_GOP_SetGOPEnable2SC(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum, MS_BOOL bEnable);
void MDrv_GOP_SetIPSel2SC(MS_GOP_CTX_LOCAL *pGOPCtx, MS_IPSEL_GOP ipSelGop);
void MDrv_GOP_SetVOPNBL(MS_GOP_CTX_LOCAL *pGOPCtx, MS_BOOL bEnable);
void MDrv_GOP_GWIN_UpdateReg(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP);
void MDrv_GOP_GWIN_UpdateRegWithSync(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
					     MS_BOOL bSync);
void MDrv_GOP_GWIN_UpdateRegWithMaskSync(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U16 u16GopMask,
						 MS_BOOL bSync);
MS_U8 MDrv_GOP_Get(MS_GOP_CTX_LOCAL *pGOPCtx);
MS_U8 MDrv_GOP_GetGwinNum(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GopNum);
MS_BOOL MDrv_GOP_SetIOMapBase(MS_GOP_CTX_LOCAL *pGOPCtx);
GOP_Result MDrv_GOP_SetGOPClk(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum,
				      DRV_GOPDstType eDstType);
MS_U8 MDrv_GOP_GetWordUnit(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum);
MS_U16 MDrv_GOP_GetGOPACK(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 enGopType);
MS_U8 MDrv_GOP_GetMaxGOPNum(MS_GOP_CTX_LOCAL *pGOPCtx);
MS_U8 MDrv_GOP_SelGwinIdByGOP(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_U8 u8GWinIdx);
MS_U8 MDrv_GOP_Get_GwinIdByGOP(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP);
MS_U16 MDrv_GOP_GetHPipeOfst(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP_num,
				     DRV_GOPDstType GopDst);
MS_BOOL MDrv_GOP_GetGOPEnum(MS_GOP_CTX_LOCAL *pGOPCtx, GOP_TYPE_DEF *GOP_TYPE);
GOP_Result MDrv_GOP_GWIN_PowerState(void *pInstance, MS_U32 u32PowerState, void *pModule);
#ifdef CONFIG_GOP_AFBC_FEATURE
GOP_Result MDrv_GOP_AFBC_Core_Reset(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP);
GOP_Result MDrv_GOP_AFBC_Core_Enable(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bEna);
#endif
GOP_Result MDrv_GOP_GWIN_AFBCMode(MS_GOP_CTX_LOCAL *pGOPCtx, MS_BOOL u8GOP,
					  MS_BOOL bEnable, MS_U8 eCTL);
GOP_Result MDrv_GOP_GWIN_AFBCSetWindow(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
					       DRV_GOP_AFBC_Info *pinfo, MS_BOOL bChangePitch);
GOP_Result MDrv_GOP_SetDbgLevel(EN_GOP_DEBUG_LEVEL level);
GOP_Result MDrv_GOP_VOP_Path_Sel(MS_GOP_CTX_LOCAL *pGOPCtx,
					 EN_GOP_VOP_PATH_MODE enGOPPath);
GOP_Result MDrv_GOP_CSC_Tuning(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U32 u32GOPNum,
				       ST_GOP_CSC_PARAM *pstCSCParam);
GOP_Result MDrv_GOP_GetOsdNonTransCnt(MS_GOP_CTX_LOCAL *pstGOPCtx, MS_U32 *pu32Count);
GOP_Result MDrv_GOP_SetCropWindow(MS_GOP_CTX_LOCAL *pstGOPCtx, MS_U32 u32GOPNum,
					  ST_GOP_CROP_WINDOW *pstCropWin);
GOP_Result MDrv_GOP_GWIN_Trigger_MUX(MS_GOP_CTX_LOCAL *pstGOPCtx);
//======================================================================================
//  GOP GWIN Setting
//======================================================================================
void MDrv_GOP_GWIN_EnableProgressive(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					     MS_BOOL bEnable);
MS_BOOL MDrv_GOP_GWIN_IsProgressive(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum);
void MDrv_GOP_GWIN_SetForceWrite(MS_GOP_CTX_LOCAL *pGOPCtx, MS_BOOL bEnable);
void MDrv_GOP_GWIN_ForceWrite_Update(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
					     MS_BOOL bEnable);
MS_BOOL MDrv_GOP_GWIN_IsForceWrite(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP);
void MDrv_GOP_GWIN_EnableTransClr(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
					  GOP_TransClrFmt fmt, MS_BOOL bEnable);
void MDrv_GOP_GWIN_SetAlphaInverse(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8gopNum,
					   MS_BOOL bEnable);
GOP_Result MDrv_GOP_GWIN_GetDstPlane(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					     DRV_GOPDstType *pGopDst);
void MDrv_GOP_GWIN_SetBlending(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win, MS_BOOL bEnable,
				       MS_U8 u8coef);
void MDrv_GOP_GWIN_SetWinFmt(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win,
				     DRV_GOPColorType clrtype);
void MDrv_GOP_GWIN_EnableGwin(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win, MS_BOOL bEnable);
void MDrv_GOP_GWIN_IsGWINEnabled(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win,
					 MS_BOOL *pbEnable);
void MDrv_GOP_GWIN_EnableHMirror(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					 MS_BOOL bEnable);
void MDrv_GOP_GWIN_EnableVMirror(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					 MS_BOOL bEnalbe);
void MDrv_GOP_IsGOPMirrorEnable(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					MS_BOOL *bHMirror, MS_BOOL *bVMirror);
void MDrv_GOP_GWIN_SetMux(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, Gop_MuxSel eGopMux);
void MDrv_GOP_MapLayer2Mux(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U32 u32Layer, MS_U8 u8Gopnum,
				   MS_U32 *pu32Mux);
void MDrv_GOP_GWIN_GetMux(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 *u8GOPNum, Gop_MuxSel eGopMux);
void MDrv_GOP_GWIN_SetHSPipe(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_U16 u16HSPipe);
void MDrv_GOP_SetGOPEnable2Mode1(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum, MS_BOOL bEnable);
void MDrv_GOP_GetGOPEnable2Mode1(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 gopNum,
					 MS_BOOL *pbEnable);
GOP_Result MDrv_GOP_GWIN_GetGwinInfo(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win,
					     DRV_GOP_GWIN_INFO *pinfo);
GOP_Result MDrv_GOP_GWIN_SetGwinInfo(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8win,
					     DRV_GOP_GWIN_INFO *pinfo);
GOP_Result MDrv_GOP_GWIN_SetDstPlane(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum,
					     DRV_GOPDstType eDstType, MS_BOOL bOnlyCheck);
MS_U16 MDrv_GOP_GetBPP(MS_GOP_CTX_LOCAL *pGOPCtx, DRV_GOPColorType fbFmt);
void MDrv_GOP_SelfFirstHs(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8Gop, MS_BOOL bEnable);

//======================================================================================
//  GOP Stretch Window Setting
//======================================================================================
void MDrv_GOP_GWIN_SetStretchWin(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_U16 x,
					 MS_U16 y, MS_U16 width, MS_U16 height);
void MDrv_GOP_GWIN_Get_StretchWin(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_U16 *x,
					  MS_U16 *y, MS_U16 *width, MS_U16 *height);
void MDrv_GOP_GWIN_Set_HSCALE(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bEnable,
				      MS_U16 src, MS_U16 dst);
void MDrv_GOP_GWIN_Set_VSCALE(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bEnable,
				      MS_U16 src, MS_U16 dst);
void MDrv_GOP_GWIN_Set_HStretchMode(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
					    DRV_GOPStrchHMode HStrchMode);
void MDrv_GOP_GWIN_Set_VStretchMode(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
					    DRV_GOPStrchVMode VStrchMode);
void MDrv_GOP_GWIN_Load_HStretchModeTable(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
						  DRV_GOPStrchHMode HStrchMode);
void MDrv_GOP_GWIN_Load_VStretchModeTable(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP,
						  DRV_GOPStrchVMode VStrchMode);

//======================================================================================
//  GOP OC
//======================================================================================
GOP_Result MDrv_GOP_OC_SetOCEn(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOPNum, MS_BOOL bOCEn);

//======================================================================================
//  GOP INTERNAL GET XC STATUS
//======================================================================================
void MDrv_GOP_GWIN_Interrupt(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8Gop, MS_BOOL bEnable);
void MDrv_GOP_CtrlRst(MS_GOP_CTX_LOCAL *pGOPCtx, MS_U8 u8GOP, MS_BOOL bRst);
GOP_Result MDrv_GOP_AutoDectBuf(MS_GOP_CTX_LOCAL *pstGOPCtx,
					ST_GOP_AUTO_DETECT_BUF_INFO *pstAutoDectInfo);

//======================================================================================
//  GOP HDR
//======================================================================================
GOP_Result MDrv_GOP_IsHDREnabled(MS_GOP_CTX_LOCAL *pGOPCtx, MS_BOOL *pbHDREnable);

#ifdef __cplusplus
}
#endif


#endif				// _DRV_TEMP_H_
