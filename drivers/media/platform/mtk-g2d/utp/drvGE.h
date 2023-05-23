/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRV_GE_H_
#define _DRV_GE_H_

#include "sti_msos.h"

 /*ALOGI*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DRV_GE_C_
#define GFX_INTERFACE
#else
#define GFX_INTERFACE extern
#endif

#if defined(MSOS_TYPE_LINUX)
#include <sys/types.h>
#include <unistd.h>
#elif defined(MSOS_TYPE_LINUX_KERNEL)
#define getpid()                pInstance
#else
#define getpid() 0UL
#endif

	typedef enum {
		E_GE_POOL_ID_INTERNAL_REGISTER = 0,
		E_GE_POOL_ID_INTERNAL_VARIABLE = 1,

		E_GE_POOL_ID_NUM,
		E_GE_POOL_ID_MAX = E_GE_POOL_ID_NUM,
	} eGEPoolID;

// #define GFX Utopia2.0 capibility
	GFX_INTERFACE void *g_pGFXResource[E_GE_POOL_ID_NUM];

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#ifdef CONFIG_GFX_UTOPIA10
#define GFX_UTOPIA20                            (0UL)	//Using Ioctl
#else
#define GFX_UTOPIA20                            (1UL)
#endif
#define GE_API_MUTEX                            1UL
#define GE_SWTABLE                              0UL	//Using virtul table as value stored
#define GE_RESOURCE_SEM                         0UL
#define GE_PALETTE_NUM                          256UL

#define GE_LOCK_SUPPORT             1UL


#define ByteSwap16(x) (((x) & 0x00ffUL) << 8 | ((x) & 0xff00UL) >> 8)
#define ByteSwap32(x) \
	((((x) & 0xff000000UL) >> 24) | (((x) & 0x00ff0000UL) >>  8) | \
	(((x) & 0x0000ff00UL) <<  8) | (((x) & 0x000000ffUL) << 24))

#define GE_BIT0                         0x01UL
#define GE_BIT1                         0x02UL
#define GE_BIT2                         0x04UL
#define GE_BIT3                         0x08UL
#define GE_BIT4                         0x10UL
#define GE_BIT5                         0x20UL
#define GE_BIT6                         0x40UL
#define GE_BIT7                         0x80UL
#define GE_BIT8                         0x0100UL
#define GE_BIT9                         0x0200UL
#define GE_BIT10                        0x0400UL
#define GE_BIT11                        0x0800UL
#define GE_BIT12                        0x1000UL
#define GE_BIT13                        0x2000UL
#define GE_BIT14                        0x4000UL
#define GE_BIT15                        0x8000UL

#ifdef ANDROID
#define printf  ALOGI
#elif (defined MSOS_TYPE_LINUX_KERNEL && (!defined printf))
#define printf  printk
#endif

#define _GE_MUXTEX_ENTRY(pGECtx, POOL_ID)\
	GE_Get_Resource(pGECtx, TRUE)

#define _GE_MUXTEX_RETURN(pGECtx, POOL_ID)\
	GE_Free_Resource(pGECtx, TRUE)


#define _GE_SEMAPHORE_ENTRY(pGECtx, POOL_ID)\
	MDrv_GE_Get_Semaphore(pGECtx, POOL_ID)

#define _GE_SEMAPHORE_RETURN(pGECtx, POOL_ID)\
	MDrv_GE_Free_Semaphore(pGECtx, POOL_ID)

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------

/// Define GE bitblt down-scaling caps
typedef struct {
	/// Bitblt down scale range start
	MS_U8 u8RangeMax;
	/// Bitblt down scale range end
	MS_U8 u8RangeMin;
	/// Bitblt down scale continuous range end
	MS_U8 u8ContinuousRangeMin;

	MS_BOOL bFullRangeSupport;

	/// Help Maintain Functions GE_SetBltScaleRatio/GE_CalcBltScaleRatio
	/// 1>>u8ShiftRangeMax = u8RangeMax
	MS_U8 u8ShiftRangeMax;
	/// 1>>u8ShiftRangeMin = u8RangeMin
	MS_U8 u8ShiftRangeMin;
	/// 1>>u8ShiftContinuousRangeMin = u8ContinuousRangeMin
	MS_U8 u8ShiftContinuousRangeMin;
} GE_BLT_DownScaleCaps;

/// Define GE chip property for different chip characteristic.
typedef struct {
	MS_U16 WordUnit;
	MS_BOOL bSupportFourePixelMode;
	MS_BOOL bFourPixelModeStable;
	MS_BOOL bSupportMultiPixel;
	MS_BOOL bSupportSpiltMode;
	MS_BOOL bSupportTwoSourceBitbltMode;
	GE_BLT_DownScaleCaps BltDownScaleCaps;
	MS_U16 MIUSupportMaxNUM;
} GE_CHIP_PROPERTY;

///Define GE Return Value
typedef enum {
	/// fail
	E_GE_FAIL = 0,
	/// success
	E_GE_OK,
	/// address error
	E_GE_FAIL_ADDR,
	/// pitch error
	E_GE_FAIL_PITCH,
	/// function parameter error
	E_GE_FAIL_PARAM,

	/// not support
	E_GE_NOT_SUPPORT,

	/// pixel format error
	E_GE_FAIL_FORMAT,
	/// bitblit start address error
	E_GE_FAIL_BLTADDR,
	/// bitblt overlap (if STRETCH, ITALIC, MIRROR, ROTATE)
	E_GE_FAIL_OVERLAP,
	/// stretch bitblt fail
	E_GE_FAIL_STRETCH,
	/// italic bitblt fail (if MIRROR, ROTATE)
	E_GE_FAIL_ITALIC,

	/// engine is locked by others
	E_GE_FAIL_LOCKED,

	/// primitive will not be drawn
	E_GE_NOT_DRAW,

	/// Dependent functions are not linked
	E_GE_NO_DEPENDENT,

	/// context not inited.
	E_GE_CTXMAG_FAIL,

	E_GE_INVALID_INTENSITY_ID = 0x100,
	E_GE_INVALID_BUFF_INFO,
	E_GE_INVALID_FONT_HANDLE,
	E_GE_INVALID_BMP_HANDLE,
	E_GE_INIT_FAIL,
} GE_Result;

///Define GE Init Configuration
typedef struct {
	MS_U8 u8Miu;
	MS_U8 u8Dither;
	MS_U32 u32VCmdQSize;	/// MIN:4K, MAX:512K, <MIN:Disable
	MS_PHY PhyVCmdQAddr;	// 8-byte aligned
	MS_BOOL bIsHK;	/// Running as HK or Co-processor
	MS_BOOL bIsCompt;
} GE_Config;


#define GE_FLAG_BLT_ROTATE_SHFT     16UL
#define GE_FLAG_BLT_ROTATE_MASK     (0x3UL << GE_FLAG_BLT_ROTATE_SHFT)

///Define GE Primitive Drawing Flags
typedef enum {
	/// Line color gradient enable, color is gradient by major-axis
	E_GE_FLAG_LINE_CONSTANT = 0x00000000,

	/// Line color gradient enable, color is gradient by major-axis
	E_GE_FLAG_LINE_GRADIENT = 0x00000001,

	/// Rectangle horizonal color gradient enable
	E_GE_FLAG_RECT_GRADIENT_X = 0x00000002,
	/// Rectangle vertical color gradient enable
	E_GE_FLAG_RECT_GRADIENT_Y = 0x00000004,

	/// Trapezoid horizonal color gradient enable
	E_GE_FLAG_TRAPEZOID_GRADIENT_X = 0x00000010,
	/// Rectangle vertical color gradient enable
	E_GE_FLAG_TRAPEZOID_GRADIENT_Y = 0x00000020,

	E_GE_FLAG_TRAPEZOID_X = 0x00000040,

	E_GE_FLAG_TRAPEZOID_Y = 0x00000080,
	/// Support SRC/DST RECT overlap
	E_GE_FLAG_BLT_OVERLAP = 0x00000100,

	/// Stretch bitblt enable
	///@note STRETCH does not support BLT_OVERLAP
	E_GE_FLAG_BLT_STRETCH = 0x00000200,

	/// Bitblt italic style enable
	///@note ITALIC does not support BLT_MIRROR, BLT_ROTATE, BLT_OVERLAP
	E_GE_FLAG_BLT_ITALIC = 0x00000400,

	/// Horizontal mirror
	///@note MIRROR does not support FMT_I1, FMT_I2, FMT_I4, BLT_ITALIC, BLT_OVERLAP
	E_GE_FLAG_BLT_MIRROR_H = 0x00001000,
	/// Vertical mirror
	E_GE_FLAG_BLT_MIRROR_V = 0x00002000,

	E_GE_FLAG_BLT_DST_MIRROR_H = 0x00004000,

	E_GE_FLAG_BLT_DST_MIRROR_V = 0x00008000,

	/// 90 deree clockwise rotation
	///@note ROTATE does not support italic, BLT_OVERLAP
	E_GE_FLAG_BLT_ROTATE_90 = (0x01 << GE_FLAG_BLT_ROTATE_SHFT),
	/// 180 degree clockwise rotation
	E_GE_FLAG_BLT_ROTATE_180 = (0x02 << GE_FLAG_BLT_ROTATE_SHFT),
	/// 270 degree clockwise rotation
	E_GE_FLAG_BLT_ROTATE_270 = (0x03 << GE_FLAG_BLT_ROTATE_SHFT),

	/// BLT_STRETCH by nearest filter (default: BILINEAR)
	E_GE_FLAG_STRETCH_NEAREST = 0x00100000,
//    E_GE_FLAG_STRETCH_CLAMP         = 0x00200000,

	// (RESERVED)
	E_GE_FLAG_BUF_TILE = 0x01000000,

	E_GE_FLAG_BYPASS_STBCOEF = 0x02000000,

	/// Draw last point
	///@note default mode is not draw last point
	E_GE_FLAG_DRAW_LASTPOINT = 0x40000000,

	/// Output width is equal to input width
	///@note default setting of output width is input width+1
	E_GE_FLAG_INWIDTH_IS_OUTWIDTH = 0x80000000,

} GE_Flag;


///Define Buffer Format
typedef enum {
	/// color format I1
	E_GE_FMT_I1 = E_MS_FMT_I1,
	/// color format I2
	E_GE_FMT_I2 = E_MS_FMT_I2,
	/// color format I4
	E_GE_FMT_I4 = E_MS_FMT_I4,
	/// color format palette 256(I8)
	E_GE_FMT_I8 = E_MS_FMT_I8,
	/// color format RGB332
	E_GE_FMT_RGB332 = 0x5,
	/// color format blinking display
	E_GE_FMT_FaBaFgBg2266 = E_MS_FMT_FaBaFgBg2266,
	/// color format for blinking display format
	E_GE_FMT_1ABFgBg12355 = E_MS_FMT_1ABFgBg12355,
	/// color format RGB565
	E_GE_FMT_RGB565 = E_MS_FMT_RGB565,
	/// color format ARGB1555
	/// @note <b>[URANUS] <em>ARGB1555 is only RGB555</em></b>
	E_GE_FMT_ARGB1555 = E_MS_FMT_ARGB1555,
	/// color format ARGB4444
	E_GE_FMT_ARGB4444 = E_MS_FMT_ARGB4444,
	/// color format ARGB1555 DST
	E_GE_FMT_ARGB1555_DST = E_MS_FMT_ARGB1555_DST,
	/// color format YUV422
	E_GE_FMT_YUV422 = E_MS_FMT_YUV422,
	/// color format ARGB8888
	E_GE_FMT_ARGB8888 = E_MS_FMT_ARGB8888,
	/// color format RGBA5551
	E_GE_FMT_RGBA5551 = E_MS_FMT_RGBA5551,
	/// color format RGBA4444
	E_GE_FMT_RGBA4444 = E_MS_FMT_RGBA4444,
	/// color format ABGR8888
	E_GE_FMT_ABGR8888 = E_MS_FMT_ABGR8888,
	/// New Color Format
	/// color format BGRA5551
	E_GE_FMT_BGRA5551 = E_MS_FMT_BGRA5551,
	/// color format ABGR1555
	E_GE_FMT_ABGR1555 = E_MS_FMT_ABGR1555,
	/// color format ABGR4444
	E_GE_FMT_ABGR4444 = E_MS_FMT_ABGR4444,
	/// color format BGRA4444
	E_GE_FMT_BGRA4444 = E_MS_FMT_BGRA4444,
	/// color format BGR565
	E_GE_FMT_BGR565 = E_MS_FMT_BGR565,
	/// color format RGBA8888
	E_GE_FMT_RGBA8888 = E_MS_FMT_RGBA8888,
	/// color format RGBA8888
	E_GE_FMT_BGRA8888 = E_MS_FMT_BGRA8888,
	/// color format ACrYCb444
	E_GE_FMT_ACRYCB444 = 0x18,
	/// color format CrYCbA444
	E_GE_FMT_CRYCBA444 = 0x19,
	/// color format ACbYCr444
	E_GE_FMT_ACBYCR444 = 0x1a,
	/// color format CbYCrA444
	E_GE_FMT_CBYCRA444 = 0x1b,

	E_GE_FMT_GENERIC = E_MS_FMT_GENERIC,

} GE_BufFmt;

//GE TLB Mode
typedef enum {
	/// TLB for None
	E_GE_TLB_NONE = 0,
	/// TLB for Source
	E_GE_TLB_SRC = 1,
	/// TLB for Destination
	E_GE_TLB_DST = 2,
	/// TLB for Source and Destination
	E_GE_TLB_SRC_DST = 3,
} GE_TLB_Mode;

///Define Colorkey Mode
typedef enum {
	/// max(Asrc,Adst)
	E_GE_ACMP_OP_MAX = 0,
	/// min(Asrc,Adst)
	E_GE_ACMP_OP_MIN = 1,

} GE_ACmpOp;


///Define Colorkey Mode
typedef enum {
	/// color in coler key range is keying.
	E_GE_CK_EQ = 0,	// Color EQ CK is color key
	/// color NOT in color key range is keing.
	E_GE_CK_NE = 1,	// Color NE CK is color key
	/// Alpha in coler key range is keying.
	E_GE_ALPHA_EQ = 2,	// Color EQ Alpha is color key
	/// Alpha NOT in color key range is keing.
	E_GE_ALPHA_NE = 3,	// Color NE Alpha is color key
} GE_CKOp;


///Define AlphaTest Mode
typedef enum {
	/// color in coler key range is keying.
	E_GE_ATEST_EQ = 0,	// Alpha EQ threshold
	/// color NOT in color key range is keing.
	E_GE_ATEST_NE = 1,	// Alpha NE threshold

} GE_ATestOp;

///Define Raster Operation
typedef enum {
	/// once
	E_GE_LINPAT_REP1 = 0,
	/// 2 repeat
	E_GE_LINPAT_REP2 = 1,
	/// 3 repeat
	E_GE_LINPAT_REP3 = 2,
	/// 4 repeat
	E_GE_LINPAT_REP4 = 3,

} GE_LinePatRep;


///Define Blending Operation
typedef enum {
	/// Cout = Csrc * A + Cdst * (1-A)
	/// 1
	E_GE_BLEND_ONE = 0,
	/// constant
	E_GE_BLEND_CONST = 1,
	/// source alpha
	E_GE_BLEND_ASRC = 2,
	/// destination alpha
	E_GE_BLEND_ADST = 3,

	/// Cout = ( Csrc * (Asrc*Aconst) + Cdst * (1-Asrc*Aconst) ) / 2
	E_GE_BLEND_ROP8_ALPHA = 4,
	/// Cout = ( Csrc * (Asrc*Aconst) + Cdst * (1-Asrc*Aconst) ) /
	///( Asrc*Aconst + Adst*(1-Asrc*Aconst) )
	E_GE_BLEND_ROP8_SRCOVER = 5,
	/// Cout = ( Csrc * (Asrc*Aconst) + Cdst * (1-Asrc*Aconst) ) /
	///( Asrc*Aconst * (1-Adst) + Adst )
	///@note
	/// <b>[URANUS] <em>It does not support BLEND_ALPHA, BLEND_SRCOVER, BLEND_DSTOVER</em></b>
	E_GE_BLEND_ROP8_DSTOVER = 6,
	/// Cout = ( Csrc * Aconst)
	E_GE_BLEND_ALPHA_ADST = 7,
	/// Cout = Csrc * A + Cdst * (1-A)
	/// 0
	E_GE_BLEND_ZERO = 8,
	/// 1 - constant
	E_GE_BLEND_CONST_INV = 9,
	/// 1 - source alpha
	E_GE_BLEND_ASRC_INV = 10,
	/// 1 - destination alpha
	E_GE_BLEND_ADST_INV = 11,
	/// Csrc * Adst * Asrc * Aconst + Cdst * Adst * (1 - Asrc * Aconst)
	E_GE_BLEND_SRC_ATOP_DST = 12,
	/// (Cdst * Asrc * Aconst * Adst + Csrc * Asrc * Aconst * (1 - Adst)) /
	///((Asrc * Aconst * (1 - Adst) + Asrc * Aconst * Adst)
	E_GE_BLEND_DST_ATOP_SRC = 13,
	/// ((1 - Adst) * Csrc * Asrc * Aconst + Adst * Cdst * (1 - Asrc * Aconst)) /
	///(Asrc * Aconst * (1 - Adst) + Adst * (1 - Asrc * Aconst))
	E_GE_BLEND_SRC_XOR_DST = 14,
	/// Csrc * (1 - Aconst)
	E_GE_BLEND_INV_CONST = 15,
	/// Cconst * Aconst + Cdst * (1 - Aconst)
	E_GE_BLEND_FADEIN = 0x11,
	/// Cconst * Aconst + Cdst * (1 - Aconst)
	E_GE_BLEND_FADEOUT = 0x19,

} GE_BlendOp;

//-------------------------------------------------


/// Define DFB Blending Related:
typedef enum {
	E_GE_DFB_BLD_FLAG_COLORALPHA = 0x0001,
	E_GE_DFB_BLD_FLAG_ALPHACHANNEL = 0x0002,
	E_GE_DFB_BLD_FLAG_COLORIZE = 0x0004,
	E_GE_DFB_BLD_FLAG_SRCPREMUL = 0x0008,
	E_GE_DFB_BLD_FLAG_SRCPREMULCOL = 0x0010,
	E_GE_DFB_BLD_FLAG_DSTPREMUL = 0x0020,
	E_GE_DFB_BLD_FLAG_XOR = 0x0040,
	E_GE_DFB_BLD_FLAG_DEMULTIPLY = 0x0080,
	E_GE_DFB_BLD_FLAG_SRCALPHAMASK = 0x0100,
	E_GE_DFB_BLD_FLAG_SRCCOLORMASK = 0x0200,
	E_GE_DFB_BLD_FLAG_ALL = 0x03FF,
} GE_DFBBldFlag;

typedef enum {
	E_GE_DFB_BLD_OP_ZERO = 0,
	E_GE_DFB_BLD_OP_ONE = 1,
	E_GE_DFB_BLD_OP_SRCCOLOR = 2,
	E_GE_DFB_BLD_OP_INVSRCCOLOR = 3,
	E_GE_DFB_BLD_OP_SRCALPHA = 4,
	E_GE_DFB_BLD_OP_INVSRCALPHA = 5,
	E_GE_DFB_BLD_OP_DESTALPHA = 6,
	E_GE_DFB_BLD_OP_INVDESTALPHA = 7,
	E_GE_DFB_BLD_OP_DESTCOLOR = 8,
	E_GE_DFB_BLD_OP_INVDESTCOLOR = 9,
	E_GE_DFB_BLD_OP_SRCALPHASAT = 10,
} GE_DFBBldOP;

///Define POINT
typedef struct {
	MS_U16 x;
	MS_U16 y;

} GE_Point;


///Define the RECT
typedef struct {
	/// X start address
	MS_U16 x;
	/// Y start address
	MS_U16 y;
	/// width
	MS_U16 width;
	/// height
	MS_U16 height;

} GE_Rect;

typedef struct {
	/// x0 start address
	MS_U16 u16X0;
	/// y0 start address
	MS_U16 u16Y0;
	/// x1 start address
	MS_U16 u16X1;
	/// y1 start address
	MS_U16 u16Y1;
	/// delta of Top
	MS_U16 u16DeltaTop;
	// delta of Bottom
	MS_U16 u16DeltaBottom;
} GE_Trapezoid;

typedef struct {
	/// Destination block
	union {
		GE_Rect dstblk;
		GE_Trapezoid dsttrapeblk;
	};

} GE_DstBitBltType;


///Define the RECT
typedef struct {
	/// STBB INIT X
	MS_U32 init_x;
	/// STBB INIT Y
	MS_U32 init_y;
	/// STBB X
	MS_U32 x;
	/// STBB Y
	MS_U32 y;

} GE_ScaleInfo;


///Define Buffer Format Information
typedef struct {
	/// Specified format
	GE_BufFmt fmt;

	/// Base alignment in byte unit
	MS_U8 u8BaseAlign;
	/// Pitch alignment in byte unit
	MS_U8 u8PitchAlign;
	/// Non-1p mode base/pitch alignment in byte unit
	MS_U8 u8Non1pAlign;
	/// Height alignment in pixel unit <b>(RESERVED)</b>
	MS_U8 u8HeightAlign;
	/// StretchBlt base/pitch alignment in byte unit
	MS_U8 u8StretchAlign;
	/// Tile mode base alignment in byte unit <b>(RESERVED)</b>
	MS_U8 u8TileBaseAlign;
	/// Tile mode width alignment in pixel unit <b>(RESERVED)</b>
	MS_U8 u8TileWidthAlign;
	/// Tile mode height alignment in pixel unit <b>(RESERVED)</b>
	MS_U8 u8TileHeightAlign;

	/// Format capability flags <b>(RESERVED)</b>
	MS_U32 u32CapFlags;

} GE_FmtCaps;


///Define Src/Dst Buffer Information
typedef struct {
	/// Specified format
	GE_BufFmt srcfmt;
	/// Specified format
	GE_BufFmt dstfmt;
	/// Base alignment in byte unit
	MS_U16 srcpit;
	/// Pitch alignment in byte unit
	MS_U16 dstpit;
	/// Height alignment in pixel unit <b>(RESERVED)</b>
	MS_PHY srcaddr;
	/// StretchBlt base/pitch alignment in byte unit
	MS_PHY dstaddr;
	/// GE TLB Mode
	GE_TLB_Mode tlbmode;
	/// GE TLB Entry source base address
	MS_PHY tlbsrcaddr;
	/// GE TLB Entry destination base address
	MS_PHY tlbdstaddr;
	/// GE TLB Entry Flush Table
	MS_BOOL bEnflushtlbtable;
} GE_BufInfo;


/// GE dbg info
typedef struct {
	/// Specified format
	MS_U8 verctrl[32];
	/// Specified format
	MS_U16 gedump[0x80];
	/// Base alignment in byte unit
	MS_U32 semstaus;
} GE_DbgInfo;

//-------------------------------------------------
/// Define RGB color in LE
typedef struct {
	/// Blue
	MS_U8 b;
	/// Green
	MS_U8 g;
	/// Red
	MS_U8 r;
	/// Alpha
	MS_U8 a;
} GE_RgbColor;


//-------------------------------------------------
/// Specify the blink data
//          1 A B Fg Bg
//          1 2 3  5  5
typedef struct {
	/// BG color (for blink mode use)
	MS_U8 background;	// 5 bits
	/// FG color (for blink mode use)
	MS_U8 foreground;	// 5 bits
	/// Control flag\n
	union {
		MS_U16 ctrl_flag;
		struct {
			MS_U16 Blink:3;	// 3 bits
			MS_U16 Alpha:2;	// 2 bits
			MS_U16 blink_en:1;
			MS_U16 reserved1:10;
		} Bits;
		struct {
			MS_U16 BlinkAlpha:4;
			MS_U16 Alpha:2;
			MS_U16 Blink:3;
			MS_U16 reserved:7;
		} Bits2;
		struct {
			MS_U16 Fa:2;
			MS_U16 Ba:2;
			MS_U16 reserved:12;

		} Bits3;
	};

} GE_BlinkData;


/// define MHEG5 draw oval information
typedef struct {
	/// dst block info
	GE_Rect dstBlock;
	/// Color format
	GE_BufFmt fmt;
	union {
		GE_RgbColor color;
		GE_BlinkData blink_data;
	};
	MS_U32 u32LineWidth;
} GE_OVAL_FILL_INFO;


///Define Buffer Usage Type
typedef enum {
	/// Desitination buffer for LINE, RECT, BLT
	E_GE_BUF_DST = 0,
	/// Source buffer for BLT
	E_GE_BUF_SRC = 1,
	/// VCMDQ buffer
	E_GE_BUF_VCMDQ = 2,
} GE_BufType;


///Define RGB2YUV conversion formula
typedef enum {
	E_GE_YUV_RGB2YUV_PC = 0,
	E_GE_YUV_RGB2YUV_255 = 1,

} GE_Csc_Rgb2Yuv;

///Define output YUV color domain
typedef enum {
	E_GE_YUV_OUT_255 = 0,
	E_GE_YUV_OUT_PC = 1,

} GE_YUV_OutRange;

///Define input YUV color domain
typedef enum {
	E_GE_YUV_IN_255 = 0,
	E_GE_YUV_IN_127 = 1,

} GE_YUV_InRange;

///Define YOU 422 format
typedef enum {
	E_GE_YUV_YVYU = 0,
	E_GE_YUV_YUYV = 1,
	E_GE_YUV_VYUY = 2,
	E_GE_YUV_UYVY = 3,

} GE_YUV_422;

typedef struct {
	GE_Csc_Rgb2Yuv rgb2yuv;
	GE_YUV_OutRange out_range;
	GE_YUV_InRange in_range;
	GE_YUV_422 dst_fmt;
	GE_YUV_422 src_fmt;

} GE_YUVMode;


typedef enum {
	E_GE_WP_IN_RANGE = 0,
	E_GE_WP_OUT_RANGE = 1,
	E_GE_WP_DISABLE = 0xFF,

} GE_WPType;


typedef struct {
	void *pInstance;
	MS_U32 u32GE_DRV_Version;
	MS_BOOL bGEMode4MultiProcessAccess;
	MS_S32 s32CurrentProcess;
	MS_S32 s32CurrentThread;
	GE_BufInfo pBufInfo;
} GE_Context;


typedef struct {
	MS_U32 flags;
	GE_Rect src;
	GE_DstBitBltType dst;
	GE_ScaleInfo *scaleinfo;
} PatchBitBltInfo;

typedef enum {
	E_GE_ROTATE_0 = 0,
	E_GE_ROTATE_90 = 1,
	E_GE_ROTATE_180 = 2,
	E_GE_ROTATE_270 = 3,
} GE_RotateAngle;

typedef struct {
	MS_U16 GE0_Reg[0x80];
	MS_U16 GETLB_Reg[0x80];
	MS_U16 GECLK_Reg;
	MS_U32 GEPalette[256];
	MS_U16 HWSemaphoreID;
} GE_STR_SAVE_AREA;

typedef enum {
	E_GE_SAVE_REG_GE_EN = 0x1000,
	E_GE_RESTORE_REG_GE_EN = 0x2000,
	E_GE_DISABLE_REG_EN = 0x3000,
} GE_RESTORE_REG_ACTION;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

// GE Draw Functions
GE_Result GE_Get_Resource(GE_Context *pGECtx, MS_BOOL bHWSemLock);
GE_Result GE_Free_Resource(GE_Context *pGECtx, MS_BOOL bHWSemLock);

GE_Result MDrv_GE_SetDither(GE_Context *pGECtx, MS_BOOL enable);
GE_Result MDrv_GE_SetOnePixelMode(GE_Context *pGECtx, MS_BOOL enable);
GE_Result MDrv_GE_SetSrcColorKey(GE_Context *pGECtx, MS_BOOL enable, GE_CKOp ck_mode,
								MS_U32 ck_low, MS_U32 ck_high);
GE_Result MDrv_GE_SetDstColorKey(GE_Context *pGECtx, MS_BOOL enable, GE_CKOp ck_mode,
								MS_U32 ck_low, MS_U32 ck_high);
GE_Result MDrv_GE_SetIntensity(GE_Context *pGECtx, MS_U32 id, MS_U32 color);
GE_Result MDrv_GE_GetIntensity(GE_Context *pGECtx, MS_U32 id, MS_U32 *color);
GE_Result MDrv_GE_SetROP2(GE_Context *pGECtx, MS_BOOL enable, GFX_ROP2_Op eRop2);
GE_Result MDrv_GE_SetLinePattern(GE_Context *pGECtx, MS_BOOL enable,
								MS_U8 pattern, GE_LinePatRep eRep);
GE_Result MDrv_GE_ResetLinePattern(GE_Context *pGECtx);
GE_Result MDrv_GE_SetAlphaBlend(GE_Context *pGECtx, MS_BOOL enable, GE_BlendOp eBlendCoef);
GE_Result MDrv_GE_SetAlphaBlendCoef(GE_Context *pGECtx, GE_BlendOp eBlendOp);
GE_Result MDrv_GE_SetAlphaSrc(GE_Context *pGECtx, GFX_AlphaSrcFrom eAlphaSrc);
GE_Result MDrv_GE_SetAlphaCmp(GE_Context *pGECtx, MS_BOOL enable, GE_ACmpOp eACmpOp);
GE_Result MDrv_GE_SetAlphaConst(GE_Context *pGECtx, MS_U8 u8AlphaConst);
GE_Result MDrv_GE_EnableDFBBlending(GE_Context *pGECtx, MS_BOOL enable);
GE_Result MDrv_GE_SetDFBBldFlags(GE_Context *pGECtx, MS_U16 u16DFBBldFlags);
GE_Result MDrv_GE_SetDFBBldOP(GE_Context *pGECtx, GE_DFBBldOP geSrcBldOP, GE_DFBBldOP geDstBldOP);
GE_Result MDrv_GE_SetDFBBldConstColor(GE_Context *pGECtx, GE_RgbColor geRgbColor);
GE_Result MDrv_GE_SetClipWindow(GE_Context *pGECtx, GE_Rect *rect);
GE_Result MDrv_GE_SetPalette(GE_Context *pGECtx, MS_U16 start, MS_U16 num, MS_U32 *palette);
GE_Result MDrv_GE_GetPalette(GE_Context *pGECtx, MS_U16 start, MS_U16 num, MS_U32 *palette);
GE_Result MDrv_GE_SetYUVMode(GE_Context *pGECtx, GE_YUVMode *mode);
GE_Result MDrv_GE_SetSrcBuffer(GE_Context *pGECtx, GE_BufFmt src_fmt, MS_U16 pix_width,
						MS_U16 pix_height, MS_PHY addr,
						MS_U16 pitch, MS_U32 flags);
GE_Result MDrv_GE_SetDstBuffer(GE_Context *pGECtx, GE_BufFmt dst_fmt, MS_U16 pix_width,
						MS_U16 pix_height, MS_PHY addr,
						MS_U16 pitch, MS_U32 flags);
GE_Result MDrv_GE_DrawLine(GE_Context *pGECtx, GE_Point *v0, GE_Point *v1, MS_U32 color,
							MS_U32 color2, MS_U32 flags, MS_U32 width);
GE_Result MDrv_GE_FillRect(GE_Context *pGECtx, GE_Rect *rect, MS_U32 color,
							MS_U32 color2, MS_U32 flags);
GE_Result MDrv_GE_WaitIdle(GE_Context *pGECtx);
GE_Result MDrv_GE_GetBufferInfo(GE_Context *pGECtx, GE_BufInfo *bufinfo);
GE_Result MDrv_GE_BitBltEX(GE_Context *pGECtx, GE_Rect *src, GE_DstBitBltType *dst,
							MS_U32 flags, GE_ScaleInfo *scaleinfo);
// GE Utility Functions
GE_Result MDrv_GE_Init(void *pInstance, GE_Config *cfg, GE_Context **ppGECtx);

GE_Result MDrv_GE_WaitTAGID(GE_Context *pGECtx, MS_U16 tagID);
GE_Result MDrv_GE_SetNextTAGID(GE_Context *pGECtx, MS_U16 *tagID);
GE_Result MDrv_GE_EnableVCmdQueue(GE_Context *pGECtx, MS_U16 blEnable);
GE_Result MDrv_GE_SetVCmdBuffer(GE_Context *pGECtx, MS_PHY PhyAddr, GFX_VcmqBufSize enBufSize);
GE_Result MDrv_GE_SetVCmd_W_Thread(GE_Context *pGECtx, MS_U8 u8W_Threshold);
GE_Result MDrv_GE_SetVCmd_R_Thread(GE_Context *pGECtx, MS_U8 u8R_Threshold);
MS_U32    GE_Divide2Fixed(MS_U16 u16x, MS_U16 u16y, MS_U8 nInteger, MS_U8 nFraction);
GE_Result MDrv_GE_SetStrBltSckType(GE_Context *pGECtx, GFX_StretchCKType TYPE, MS_U32 *CLR);
GE_Result MDrv_GE_DrawOval(GE_Context *pGECtx, GE_OVAL_FILL_INFO *pOval);
MS_U8  _GFXAPI_MIU_ID(MS_PHY ge_fbaddr);
MS_PHY _GFXAPI_PHYS_ADDR_IN_MIU(MS_PHY ge_fbaddr);
GE_Result MDrv_GE_BitbltPerformance(GE_Context *pGECtx);
GE_Result MDrv_GE_Chip_Proprity_Init(GE_Context *pGECtx, GE_CHIP_PROPERTY **pGeChipPro);
GE_Result MDrv_GE_STR_SetPowerState(EN_POWER_MODE u16PowerState, void *pModule);
GE_Result MDrv_GE_RestoreRegInfo(GE_Context *pGECtx, GE_RESTORE_REG_ACTION eRESTORE_ACTION,
									MS_U16 *value);
GE_Result MDrv_GE_GetCRC(GE_Context *pGECtx, MS_U32 *pu32CRCvalue);

#ifdef __cplusplus
}
#endif
#endif				// _DRV_GE_H_
