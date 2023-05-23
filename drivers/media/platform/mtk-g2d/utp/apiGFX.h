/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _API_GFX_H_
#define _API_GFX_H_

#include "sti_msos.h"

#ifdef __cplusplus
extern "C" {
#endif


/// GFX API return value
typedef enum {
	GFX_FAIL = 0,
	GFX_SUCCESS,
	GFX_NON_ALIGN_ADDRESS,
	GFX_NON_ALIGN_PITCH,
	GFX_INVALID_PARAMETERS,
	/// not support
	GFX_DRV_NOT_SUPPORT,
	/// pixel format error
	GFX_DRV_FAIL_FORMAT,
	/// bitblit start address error
	GFX_DRV_FAIL_BLTADDR,
	/// bitblt overlap (if STRETCH, ITALIC, MIRROR, ROTATE)
	GFX_DRV_FAIL_OVERLAP,
	/// stretch bitblt fail
	GFX_DRV_FAIL_STRETCH,
	/// italic bitblt fail (if MIRROR, ROTATE)
	GFX_DRV_FAIL_ITALIC,
	/// engine is locked by others
	GFX_DRV_FAIL_LOCKED,
	/// primitive will not be drawn
	GFX_DRV_NOT_DRAW,
	/// Dependent functions are not linked
	GFX_DRV_NO_DEPENDENT,
	/// context not inited.
	GFX_DRV_CTXMAG_FAIL,

	GFX_INVALID_INTENSITY_ID = 0x100,
	GFX_INVALID_BUFF_INFO,
	GFX_INVALID_FONT_HANDLE,
	GFX_INVALID_BMP_HANDLE,
	GFX_INIT_FAIL,
} GFX_Result;


///Define Raster Operation
typedef enum {
	/// rop_result = 0;
	ROP2_OP_ZERO = 0,
	/// rop_result = ~( rop_src | rop_dst );
	ROP2_OP_NOT_PS_OR_PD = 1,
	/// rop_result = ((~rop_src) & rop_dst);
	ROP2_OP_NS_AND_PD = 2,
	/// rop_result = ~(rop_src);
	ROP2_OP_NS = 3,
	/// rop_result = (rop_src & (~rop_dst));
	ROP2_OP_PS_AND_ND = 4,
	/// rop_result = ~(rop_dst);
	ROP2_OP_ND = 5,
	/// rop_result = ( rop_src ^ rop_dst);
	ROP2_OP_PS_XOR_PD = 6,
	/// rop_result = ~(rop_src & rop_dst);
	ROP2_OP_NOT_PS_AND_PD = 7,
	/// rop_result = (rop_src & rop_dst);
	ROP2_OP_PS_AND_PD = 8,
	/// rop_result = ~(rop_dst ^ rop_src);
	ROP2_OP_NOT_PS_XOR_PD = 9,
	/// rop_result = rop_dst;
	ROP2_OP_PD = 10,
	/// rop_result = (rop_dst | (~rop_src));
	ROP2_OP_NS_OR_PD = 11,
	/// rop_result = rop_src;
	ROP2_OP_PS = 12,
	/// rop_result = (rop_src | (~rop_dst));
	ROP2_OP_PS_OR_ND = 13,
	/// rop_result = (rop_dst | rop_src);
	ROP2_OP_PD_OR_PS = 14,
	/// rop_result = 0xffffff;
	ROP2_OP_ONE = 15,
} GFX_ROP2_Op;


/// Color Key Operation Mode
typedef enum {
	/// If EQUAL then perform CK operation
	CK_OP_EQUAL = 0,
	/// If NOT EQUAL then perform CK operation
	CK_OP_NOT_EQUAL = 1,
	/// If EQUAL then perform Alpha Key operation
	AK_OP_EQUAL = 2,
	/// If NOT EQUAL then Alpha Key operation
	AK_OP_NOT_EQUAL = 3,
} GFX_ColorKeyMode;

///Define Blending Coefficient
typedef enum {
	/// Csrc
	COEF_ONE = 0,
	/// Csrc * Aconst + Cdst * (1 - Aconst)
	COEF_CONST,
	///  Csrc * Asrc + Cdst * (1 - Asrc)
	COEF_ASRC,
	/// Csrc * Adst + Cdst * (1 - Adst)
	COEF_ADST,

	/// Cdst
	COEF_ZERO,
	/// Csrc * (1 - Aconst) + Cdst * Aconst
	COEF_1_CONST,
	/// Csrc * (1 - Asrc) + Cdst * Asrc
	COEF_1_ASRC,
	///  Csrc * (1 - Adst) + Cdst * Adst
	COEF_1_ADST,

	/// ((Asrc * Aconst) * Csrc + (1-(Asrc *Aconst)) * Cdst) / 2
	COEF_ROP8_ALPHA,
	/// ((Asrc * Aconst) * Csrc + Adst * Cdst * (1-(Asrc * Aconst))) /
	///(Asrc * Aconst) + Adst * (1- Asrc * Aconst))
	COEF_ROP8_SRCOVER,
	/// ((Asrc * Aconst) * Csrc * (1-Adst) + Adst * Cdst) / (Asrc * Aconst) * (1-Adst) + Adst)
	COEF_ROP8_DSTOVER,

	/// Csrc * Aconst
	///@note
	/// <b>[URANUS] <em>It does not support COEF_CONST_SRC</em></b>
	COEF_CONST_SRC,
	/// Csrc * (1 - Aconst)
	///@note
	/// <b>[URANUS] <em>It does not support COEF_1_CONST_SRC</em></b>
	COEF_1_CONST_SRC,

	/// Csrc * Adst * Asrc * Aconst + Cdst * Adst * (1 - Asrc * Aconst)
	///@note
	/// <b>[URANUS] <em>It does not support COEF_SRC_ATOP_DST</em></b>
	COEF_SRC_ATOP_DST,
	/// Cdst * Asrc * Aconst * Adst + Csrc * Asrc * Aconst * (1 - Adst)
	///@note
	/// <b>[URANUS] <em>It does not support COEF_DST_ATOP_SRC</em></b>
	COEF_DST_ATOP_SRC,
	/// (1 - Adst) * Csrc * Asrc * Aconst + Adst * Cdst * (1 - Asrc * Aconst)
	COEF_SRC_XOR_DST,
	/// Cconst * Aconst + Cdst * (1 - Aconst)
	COEF_FADEIN,
	///Cconst * (1 - Aconst) + Cdst * Aconst
	COEF_FADEOUT,
} GFX_BlendCoef;

///Define Blending Source from
typedef enum {
	/// constant
	ALPHA_FROM_CONST = 0,
	/// source alpha
	ALPHA_FROM_ASRC = 1,
	/// destination alpha
	ALPHA_FROM_ADST = 2,
	/// Aout = Asrc*Aconst
	ALPHA_FROM_ROP8_SRC = 3,
	/// Aout = Asrc*Aconst * Adst
	ALPHA_FROM_ROP8_IN = 4,
	/// Aout = (1-Asrc*Aconst) * Adst
	ALPHA_FROM_ROP8_DSTOUT = 5,
	/// Aout = (1-Adst) * Asrc*Aconst
	ALPHA_FROM_ROP8_SRCOUT = 6,
	/// Aout = (Asrc*Aconst) + Adst*(1-Asrc*Aconst) or (Asrc*Aconst)*(1-Adst) + Adst
	ALPHA_FROM_ROP8_OVER = 7,

	/// 1 - Aconst
	ALPHA_FROM_ROP8_INV_CONST = 8,
	/// 1 - Asrc
	ALPHA_FROM_ROP8_INV_ASRC = 9,
	/// 1 - Adst
	ALPHA_FROM_ROP8_INV_ADST = 10,
	/// Adst * Asrc * Aconst + Adst * (1 - Asrc * Aconst) A atop B
	ALPHA_FROM_ROP8_SRC_ATOP_DST = 11,
	/// Asrc * Aconst * Adst + Asrc * Aconst * (1 - Adst) B atop A
	ALPHA_FROM_ROP8_DST_ATOP_SRC = 12,
	/// (1 - Adst) * Asrc * Aconst + Adst * (1 - Asrc * Aconst) A xor B
	ALPHA_FROM_ROP8_SRC_XOR_DST = 13,
	/// Asrc * Asrc * Aconst + Adst * (1 - Asrc * Aconst)
	ALPHA_FROM_ROP8_INV_SRC_ATOP_DST = 14,
	/// Asrc * (1 - Asrc * Aconst) + Adst * Asrc * Aconst
	ALPHA_FROM_ROP8_INV_DST_ATOP_SRC = 15,
} GFX_AlphaSrcFrom;


///Define Colorkey Mode
typedef enum {
	/// max(Asrc,Adst)
	GFX_ACMP_OP_MAX = 0,
	/// min(Asrc,Adst)
	GFX_GE_ACMP_OP_MIN = 1,

} GFX_ACmpOp;


/// GE buffer format
typedef enum {
	/// font mode I1
	GFX_FMT_I1 = E_MS_FMT_I1,
	/// font mode I2
	GFX_FMT_I2 = E_MS_FMT_I2,
	/// font mode I4
	GFX_FMT_I4 = E_MS_FMT_I4,
	/// color format palette 256(I8)
	GFX_FMT_I8 = E_MS_FMT_I8,
	/// color format RGB332  sync with MsType.h E_MS_FMT_RGB332
	GFX_FMT_RGB332 = 0x5,
	/// color format blinking display
	GFX_FMT_FABAFGBG2266 = E_MS_FMT_FaBaFgBg2266,
	/// Uranus GOP only support this blinking
	/// color format for blinking display format
	GFX_FMT_1ABFGBG12355 = E_MS_FMT_1ABFgBg12355,
	/// color format RGB565
	GFX_FMT_RGB565 = E_MS_FMT_RGB565,
	// color format ORGB1555
	GFX_FMT_ARGB1555 = E_MS_FMT_ARGB1555,
	// color format ARGB4444
	GFX_FMT_ARGB4444 = E_MS_FMT_ARGB4444,
	// color format for blinking display format
	// Uranus GOP does NOT support this blinking format
	// GE_FMT_1BAAFGBG123433       = 0xb,
	// @FIXME: Venus does not have this color format. Need to take care of it.
	/// color format ARGB1555 DST
	GFX_FMT_ARGB1555_DST = E_MS_FMT_ARGB1555_DST,
	/// color format YUV422
	GFX_FMT_YUV422 = E_MS_FMT_YUV422,
	/// color format ARGB8888
	GFX_FMT_ARGB8888 = E_MS_FMT_ARGB8888,
	/// color format RGBA5551
	GFX_FMT_RGBA5551 = E_MS_FMT_RGBA5551,
	/// color format RGBA4444
	GFX_FMT_RGBA4444 = E_MS_FMT_RGBA4444,
	/// color format ABGR8888
	GFX_FMT_ABGR8888 = E_MS_FMT_ABGR8888,
	/// New Color Format
	/// color format BGRA5551
	GFX_FMT_BGRA5551 = E_MS_FMT_BGRA5551,
	/// color format ABGR1555
	GFX_FMT_ABGR1555 = E_MS_FMT_ABGR1555,
	/// color format ABGR4444
	GFX_FMT_ABGR4444 = E_MS_FMT_ABGR4444,
	/// color format BGRA4444
	GFX_FMT_BGRA4444 = E_MS_FMT_BGRA4444,
	/// color format BGR565
	GFX_FMT_BGR565 = E_MS_FMT_BGR565,
	/// color format RGBA8888
	GFX_FMT_RGBA8888 = E_MS_FMT_RGBA8888,
	/// color format RGBA8888
	GFX_FMT_BGRA8888 = E_MS_FMT_BGRA8888,
	/// color format ACrYCb444 sync with MsType.h E_MS_FMT_ACRYCB444
	GFX_FMT_ACRYCB444 = 0x18,
	/// color format CrYCbA444 sync with MsType.h E_MS_FMT_CRYCBA444
	GFX_FMT_CRYCBA444 = 0x19,
	/// color format ACbYCr444 sync with MsType.h E_MS_FMT_ACBYCR444
	GFX_FMT_ACBYCR444 = 0x1a,
	/// color format CbYCrA444 sync with MsType.h E_MS_FMT_CBYCRA444
	GFX_FMT_CBYCRA444 = 0x1b,
} GFX_Buffer_Format;

///Define RGB2YUV conversion formula
typedef enum {
	GFX_YUV_OP1,	// GFX_YUV_Rgb2Yuv
	GFX_YUV_OP2,	// GFX_YUV_OutRange
	GFX_YUV_OP3,	// GE_YUV_InRange
	GFX_YUV_OP4,	// GFX_YUV_422
} GFX_YUV_OpType;

///Define RGB2YUV conversion formula
typedef enum {
	GFX_YUV_RGB2YUV_PC = 0,
	GFX_YUV_RGB2YUV_255 = 1,

} GFX_YUV_Rgb2Yuv;

///Define output YUV color domain
typedef enum {
	GFX_YUV_OUT_255 = 0,
	GFX_YUV_OUT_PC = 1,

} GFX_YUV_OutRange;

///Define input YUV color domain
typedef enum {
	GFX_YUV_IN_255 = 0,
	GFX_YUV_IN_127 = 1,

} GFX_YUV_InRange;

///Define YOU 422 format
typedef enum {
	GFX_YUV_YVYU = 0,
	GFX_YUV_YUYV = 1,
	GFX_YUV_VYUY = 2,
	GFX_YUV_UYVY = 3,

} GFX_YUV_422;

typedef struct {
	GFX_YUV_Rgb2Yuv rgb2yuv;
	GFX_YUV_OutRange out_range;
	GFX_YUV_InRange in_range;
	GFX_YUV_422 dst_fmt;
	GFX_YUV_422 src_fmt;
} GFX_YUVMode;


//-------------------------------------------------
/// Define Virtual Command Buffer Size
typedef enum {
	/// 4K
	GFX_VCMD_4K = 0,
	/// 8K
	GFX_VCMD_8K = 1,
	/// 16K
	GFX_VCMD_16K = 2,
	/// 32K
	GFX_VCMD_32K = 3,
	/// 64K
	GFX_VCMD_64K = 4,
	/// 128K
	GFX_VCMD_128K = 5,
	/// 256K
	GFX_VCMD_256K = 6,
	/// 512K
	GFX_VCMD_512K = 7,
	/// 1024k
	GFX_VCMD_1024K = 8,
} GFX_VcmqBufSize;



//-------------------------------------------------
/// Define RGB color in LE
typedef struct DLL_PACKED {
	/// Blue
	MS_U8 b;
	/// Green
	MS_U8 g;
	/// Red
	MS_U8 r;
	/// Alpha
	MS_U8 a;
} GFX_RgbColor;

/// Define YUV color
typedef struct {
	MS_U32 Y:8;
	MS_U32 U:4;
	MS_U32 V:4;
} GFX_YuvColor;


/// General GFX color type, union of rgb and yuv color.
#ifndef MSOS_TYPE_OPTEE
typedef struct {
	union {
		GFX_RgbColor rgb;
		GFX_YuvColor yuv;
	};
} GFX_Color;
#endif
//-------------------------------------------------
/// Define the start color & end color
typedef struct DLL_PACKED {
	/// start color
	GFX_RgbColor color_s;
	/// end color
	GFX_RgbColor color_e;
} GFX_ColorRange;

//-------------------------------------------------
/// Define the position of one point.
typedef struct DLL_PACKED {
	/// X address
	MS_U16 x;
	/// Y address
	MS_U16 y;
} GFX_Point;

//-------------------------------------------------
/// Specify the blink data
//          1 A B Fg Bg
//          1 2 3  5  5
#ifndef MSOS_TYPE_OPTEE
typedef struct DLL_PACKED {
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


} GFX_BlinkData;
#endif

/// Define the start blink color & end blink color.
#ifndef MSOS_TYPE_OPTEE
typedef struct DLL_PACKED {
	/// start blink color
	GFX_BlinkData blink_data_s;
	/// end blink color
	GFX_BlinkData blink_data_e;
} GFX_BlinkDataRange;
#endif

//-------------------------------------------------
//-------------------------------------------------
#define GFXLINE_FLAG_COLOR_CONSTANT             (0x00000000UL)	/// Constant color
#define GFXLINE_FLAG_COLOR_GRADIENT             (0x00000001UL)	/// Gradient color
#define GFXLINE_FLAG_DRAW_LASTPOINT             (0x40000000UL)	/// Otherwide, not draw last point
#define GFXLINE_FLAG_INWIDTH_IS_OUTWIDTH        (0x80000000UL)	///output width is input width +1

/// Define the attribute of line.
#ifndef MSOS_TYPE_OPTEE
typedef struct DLL_PACKED {
	/// start X address
	MS_U16 x1;
	/// Start Y address
	MS_U16 y1;
	/// End X address
	MS_U16 x2;
	/// End Y address
	MS_U16 y2;
	/// Color format
	GFX_Buffer_Format fmt;
	union {
		GFX_ColorRange colorRange;
		///Blink attribute
		///For GFX_FMT_1BAAFGBG123433 foramt, the foramt set as the GFX_BlinkData.\n
		GFX_BlinkDataRange blkDataRange;
	};
	/// Line width
	MS_U32 width;
	/// GFXLINE_FLAG_COLOR_CONSTANT: Constant color\n
	/// GFXLINE_FLAG_COLOR_GRADIENT: Gradient color\n
	/// GFXLINE_FLAG_DRAW_LASTPOINT: Draw last point, otherwise not draw\n
	/// GFXLINE_FLAG_INWIDTH_IS_OUTWIDTH: Input width is output width, not input width +1
	MS_U32 flag;

} GFX_DrawLineInfo;
#endif
//-------------------------------------------------

/// Define the dimension of one block
typedef struct DLL_PACKED {
	/// X start address
	MS_U16 x;
	/// Y start address
	MS_U16 y;
	/// width
	MS_U16 width;
	/// height
	MS_U16 height;
} GFX_Block;

typedef struct DLL_PACKED {
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
} GFX_Trapezoid;

//-------------------------------------------------

#define GFXRECT_FLAG_COLOR_CONSTANT     0x0UL
#define GFXRECT_FLAG_COLOR_GRADIENT_X   0x1UL
#define GFXRECT_FLAG_COLOR_GRADIENT_Y   0x2UL
#define GFXRECT_FLAG_TRAPE_DIRECTION_X  0x4UL	//excluded with GFXRECT_FLAG_TRAPE_DIRECTION_Y
#define GFXRECT_FLAG_TRAPE_DIRECTION_Y  0x8UL	//excluded with GFXRECT_FLAG_TRAPE_DIRECTION_X

/// Define the info. of one block.
#ifndef MSOS_TYPE_OPTEE
typedef struct DLL_PACKED {
	/// dst block info
	union {
		GFX_Block dstBlock;
		GFX_Trapezoid dstTrapezoidBlk;
	};
	/// Color format
	GFX_Buffer_Format fmt;
	union {
		GFX_ColorRange colorRange;
		///Blink attribute
		///For GFX_FMT_1BAAFGBG123433 foramt, the foramt set as the GFX_BlinkData.\n
		GFX_BlinkDataRange blkDataRange;
	};

	/// GERECT_FLAG_COLOR_CONSTANT: Constant color\n
	/// GERECT_FLAG_COLOR_GRADIENT_X: X direction gradient color\n
	/// GERECT_FLAG_COLOR_GRADIENT_Y: Y direction gradient color\n

	MS_U32 flag;

} GFX_RectFillInfo;
#endif

/// Define structure for OVAL drawing.
#ifndef MSOS_TYPE_OPTEE
typedef struct {
	/// dst block info
	GFX_Block dstBlock;
	/// Color format
	GFX_Buffer_Format fmt;
	union {
		GFX_RgbColor color;
		GFX_BlinkData blink_data;
	};
	MS_U32 u32LineWidth;
} GFX_OvalFillInfo;
#endif

typedef struct DLL_PACKED {
	int x0;		/* X coordinate of first edge */
	int y0;		/* Y coordinate of first edge */
	int x1;		/* X coordinate of second edge */
	int y1;		/* Y coordinate of second edge */
	int x2;		/* X coordinate of third edge */
	int y2;		/* Y coordinate of third edge */
} GFX_Triangle;

#ifndef MSOS_TYPE_OPTEE
typedef struct DLL_PACKED {
	GFX_Triangle tri;
	GFX_Block clip_box;
	GFX_Buffer_Format fmt;
	union {
		GFX_ColorRange colorRange;
		///Blink attribute
		///For GFX_FMT_1BAAFGBG123433 foramt, the foramt set as the GFX_BlinkData.\n
		GFX_BlinkDataRange blkDataRange;
	};

	/// GERECT_FLAG_COLOR_CONSTANT: Constant color\n
	/// GERECT_FLAG_COLOR_GRADIENT_X: X direction gradient color\n
	/// GERECT_FLAG_COLOR_GRADIENT_Y: Y direction gradient color\n
	/// GFXRECT_FLAG_TRAPE_DIRECTION_X: for trapezoid, top & bottom in X direction parallel,
	///excluded with GFXRECT_FLAG_TRAPE_DIRECTION_Y\n
	/// GFXRECT_FLAG_TRAPE_DIRECTION_Y: for trapezoid, top & bottom in Y direction parallel,
	///excluded with GFXRECT_FLAG_TRAPE_DIRECTION_X\n
	MS_U32 flag;
} GFX_TriFillInfo;
#endif

typedef struct DLL_PACKED {
	int x;		/* X coordinate */
	int w;		/* width of span */
} Span;

typedef struct DLL_PACKED {
	int y;
	Span *spans;
	int num_spans;
} GFX_Span;

#ifndef MSOS_TYPE_OPTEE
typedef struct DLL_PACKED {
	GFX_Span span;
	GFX_Block clip_box;
	GFX_Buffer_Format fmt;
	union {
		GFX_ColorRange colorRange;
		///Blink attribute
		///For GFX_FMT_1BAAFGBG123433 foramt, the foramt set as the GFX_BlinkData.\n
		GFX_BlinkDataRange blkDataRange;
	};

	/// GERECT_FLAG_COLOR_CONSTANT: Constant color\n
	/// GERECT_FLAG_COLOR_GRADIENT_X: X direction gradient color\n
	/// GERECT_FLAG_COLOR_GRADIENT_Y: Y direction gradient color\n
	/// GFXRECT_FLAG_TRAPE_DIRECTION_X: for trapezoid, top & bottom in X direction parallel,
	///excluded with GFXRECT_FLAG_TRAPE_DIRECTION_Y\n
	/// GFXRECT_FLAG_TRAPE_DIRECTION_Y: for trapezoid, top & bottom in Y direction parallel,
	///excluded with GFXRECT_FLAG_TRAPE_DIRECTION_X\n
	MS_U32 flag;
} GFX_SpanFillInfo;
#endif

/// Define structure for color delta (Gradient color)
typedef struct DLL_PACKED {
	MS_U8 flag;
	MS_U32 delta_r;
	MS_U32 delta_g;
	MS_U32 delta_b;
} GFX_GFX_ColorDelta;

//-------------------------------------------------
/// Define rotation angle
typedef enum {
	/// Do not rotate
	GEROTATE_0,
	/// Rotate 90 degree
	GEROTATE_90,
	/// Rotate 180 degree
	GEROTATE_180,
	/// Rotate 270 degree
	GEROTATE_270,
} GFX_RotateAngle;

//=============================================================================
// Draw Rect info
//=============================================================================
/// Define the bitblt source & destination block.
#ifndef MSOS_TYPE_OPTEE
typedef struct DLL_PACKED {
	/// Source block
	GFX_Block srcblk;

	/// Destination block
	union {
		GFX_Trapezoid dsttrapeblk;
		GFX_Block dstblk;
	};
} GFX_DrawRect;
#endif

/// Define the scaling factor for X & Y direction.
typedef struct DLL_PACKED {
	MS_U32 u32DeltaX;
	MS_U32 u32DeltaY;
	MS_U32 u32InitDelatX;
	MS_U32 u32InitDelatY;
} GFX_ScaleInfo;

//=============================================================================
// Data Buffer info
//=============================================================================
/// Data buffer info.
typedef struct DLL_PACKED {
	/// start memory address
	MS_PHY u32Addr;	// flat address of whole memory map
	/// width
	MS_U32 u32Width;
	/// height
	MS_U32 u32Height;
	/// pitch
	MS_U32 u32Pitch;
	/// Color format\n
	/// - GFX_FMT_I1\n
	/// - GFX_FMT_I2\n
	/// - GFX_FMT_I4\n
	/// - GFX_FMT_I8\n
	/// - GFX_FMT_RGB565\n
	/// - GFX_FMT_ARGB1555\n
	/// - GFX_FMT_ARGB4444\n
	/// - GFX_FMT_1BAAFGBG123433\n
	/// - GFX_FMT_ARGB8888\n
	GFX_Buffer_Format u32ColorFmt;
} GFX_BufferInfo;

/// Define the GFX init parameter.
typedef struct DLL_PACKED {
	MS_U8 u8Miu;
	MS_U8 u8Dither;
	MS_U32 u32VCmdQSize;	/// MIN:4K, MAX:512K, <MIN:Disable
	MS_PHY u32VCmdQAddr;	//  8-byte aligned
	MS_BOOL bIsHK;	/// Running as HK or Co-processor
	MS_BOOL bIsCompt;
} GFX_Config;

/// GE dbg info
typedef struct DLL_PACKED {
	/// Specified format
	MS_U8 verctrl[32];
	/// Specified format
	MS_U8 gedump[256];
	/// Base alignment in byte unit
	MS_U32 semstaus;
} GFX_DbgInfo;


typedef GFX_BufferInfo *PGFX_BufferInfo;



//=============================================================================
// GE palette information
//=============================================================================

typedef union {
	/// ARGB8888
	struct {
		MS_U8 u8A;
		MS_U8 u8R;
		MS_U8 u8G;
		MS_U8 u8B;
	} RGB;
	// 32-bit direct access.
	MS_U8 u8Data[4];
} GFX_PaletteEntry;



#define GFXDRAW_FLAG_DEFAULT            0x0UL
#define GFXDRAW_FLAG_SCALE              0x1UL
#define GFXDRAW_FLAG_DUPLICAPE          0x2UL
#define GFXDRAW_FLAG_TRAPEZOID_X        0x4UL	// excluded with GFXDRAW_FLAG_TRAPEZOID_Y
#define GFXDRAW_FLAG_TRAPEZOID_Y        0x8UL	// excluded with GFXDRAW_FLAG_TRAPEZOID_X

//#define FB_FMT_AS_DEFAULT               0xFFFF

//=============================================================================
// YUV color setting
//=============================================================================
typedef MS_U8 GFX_Rgb2yuvMode;
#define GFX_RGB2YUV_PC_MODE             ((GFX_Rgb2yuvMode)0x0UL)	// Y: 16~ 235, UV: 0~ 240
#define GFX_RGB2YUV_255_MODE            ((GFX_Rgb2yuvMode)0x1UL)	// To 0~ 255

typedef MS_U8 GFX_YuvRangeOut;
#define GFX_YUV_RANGE_OUT_255           ((GFX_YuvRangeOut)0x0UL)	// To 0~ 255
#define GFX_YUV_RANGE_OUT_PC            ((GFX_YuvRangeOut)0x1UL)	// To Y: 16~ 235

typedef MS_U8 GFX_YuvRangeIn;
#define GFX_YUV_RANGE_IN_255            ((GFX_YuvRangeIn)0x0UL)	// UV input range, 0~ 255
#define GFX_YUV_RANGE_IN_127            ((GFX_YuvRangeIn)0x1UL)	// UV input range, -128~ 127

typedef MS_U8 GFX_Yuv422Fmt;
#define GFX_YUV_422_FMT_UY1VY0          ((GFX_Yuv422Fmt)0x00UL)	// CbY1CrY0
#define GFX_YUV_422_FMT_VY1UY0          ((GFX_Yuv422Fmt)0x01UL)	// CrY1CbY0
#define GFX_YUV_422_FMT_Y1UY0V          ((GFX_Yuv422Fmt)0x02UL)	// Y1CbY0Cr
#define GFX_YUV_422_FMT_Y1VY0U          ((GFX_Yuv422Fmt)0x03UL)	// Y1CrY0Cb

/// Pack of YUV CSC info
typedef struct {
	GFX_Rgb2yuvMode rgb2yuv_mode;
	GFX_YuvRangeOut yuv_range_out;
	GFX_YuvRangeIn yuv_range_in;
	GFX_Yuv422Fmt yuv_mem_fmt_src;
	GFX_Yuv422Fmt yuv_mem_fmt_dst;
} GFX_YuvInfo;

//-------------------------------------------------
/// Define Stretch Bitblt with Color Key Type
typedef enum {
	/// Do nothing
	GFX_DONOTHING = 0,
	/// Nearest When the Color Key happened
	GFX_NEAREST = 1,
	/// Replace the Key to Custom Color
	GFX_REPLACE_KEY_2_CUS = 2,
} GFX_StretchCKType;

//-------------------------------------------------
/// Define DFB Blending Related:
#define GFX_DFB_BLD_FLAG_COLORALPHA             0x0001UL
#define GFX_DFB_BLD_FLAG_ALPHACHANNEL           0x0002UL
#define GFX_DFB_BLD_FLAG_COLORIZE               0x0004UL
#define GFX_DFB_BLD_FLAG_SRCPREMUL              0x0008UL
#define GFX_DFB_BLD_FLAG_SRCPREMULCOL           0x0010UL
#define GFX_DFB_BLD_FLAG_DSTPREMUL              0x0020UL
#define GFX_DFB_BLD_FLAG_XOR                    0x0040UL
#define GFX_DFB_BLD_FLAG_DEMULTIPLY             0x0080UL
#define GFX_DFB_BLD_FLAG_SRCALPHAMASK           0x0100UL
#define GFX_DFB_BLD_FLAG_SRCCOLORMASK           0x0200UL
#define GFX_DFB_BLD_FLAG_ALL                    0x03FFUL

typedef enum {
	GFX_DFB_BLD_OP_ZERO = 0,
	GFX_DFB_BLD_OP_ONE = 1,
	GFX_DFB_BLD_OP_SRCCOLOR = 2,
	GFX_DFB_BLD_OP_INVSRCCOLOR = 3,
	GFX_DFB_BLD_OP_SRCALPHA = 4,
	GFX_DFB_BLD_OP_INVSRCALPHA = 5,
	GFX_DFB_BLD_OP_DESTALPHA = 6,
	GFX_DFB_BLD_OP_INVDESTALPHA = 7,
	GFX_DFB_BLD_OP_DESTCOLOR = 8,
	GFX_DFB_BLD_OP_INVDESTCOLOR = 9,
	GFX_DFB_BLD_OP_SRCALPHASAT = 10,
} GFX_DFBBldOP;

//=============================================================================
// GFX format capability setting
//=============================================================================
///Define Buffer Usage Type
typedef enum {
	/// Desitination buffer for LINE, RECT, BLT
	E_GFX_BUF_DST = 0,
	/// Source buffer for BLT
	E_GFX_BUF_SRC = 1,
} EN_GFX_BUF_TYPE;

/// Define gfx format capability type
typedef enum {
	E_GFX_FMT_CAP_NONE = 0x0,
	E_GFX_FMT_CAP_MULTI_PIXEL,
} EN_GFX_FMT_CAPS_TYPE;

typedef enum {
	E_GFX_CONFIG_INIT,
} EN_GFX_CONFIG_INIT;

typedef struct DLL_PACKED {
	GFX_Buffer_Format u32ColorFmt;
	EN_GFX_BUF_TYPE eBufferType;
	EN_GFX_FMT_CAPS_TYPE eFmtCapsType;
	MS_U8 u8BaseAlign;
	MS_U8 u8PitchAlign;
} GFX_FmtAlignCapsInfo;

//=============================================================================
// GFX capability setting
//=============================================================================

//GFX Capability
typedef enum {
	E_GFX_CAP_MULTI_PIXEL,
	E_GFX_CAP_BLT_DOWN_SCALE,
	E_GFX_CAP_DFB_BLENDING,
	E_GFX_CAP_ALIGN,
	E_GFX_CAP_TLB,
} EN_GFX_CAPS;

///GFX Capability MultiPixel Info
typedef struct DLL_PACKED {
	MS_U16 WordUnit;
	MS_BOOL bSupportMultiPixel;
} GFX_CapMultiPixelInfo;

///GFX Capability Bitblt down scale Info
typedef struct DLL_PACKED {
	/// Bitblt down scale range start
	MS_U8 u8RangeMax;
	/// Bitblt down scale range end
	MS_U8 u8RangeMin;
	/// Bitblt down scale continuous range end
	MS_U8 u8ContinuousRangeMin;
	MS_BOOL bFullRangeSupport;
} GFX_BLT_DownScaleInfo;


#ifdef MSOS_TYPE_OPTEE
DLL_PUBLIC void MApi_GFX_Init(GFX_Config *geConfig);
DLL_PUBLIC GFX_Result MApi_GFX_WaitForTAGID(MS_U16 tagID);
DLL_PUBLIC MS_U16 MApi_GFX_SetNextTAGID(void);
DLL_PUBLIC GFX_Result MApi_GFX_BeginDraw(void);
DLL_PUBLIC GFX_Result MApi_GFX_EndDraw(void);
DLL_PUBLIC GFX_Result MApi_GFX_ClearFrameBufferByWord(MS_PHY StrAddr, MS_U32 length,
						      MS_U32 ClearValue);
#else
DLL_PUBLIC extern void MApi_GFX_Init(GFX_Config *geConfig);
DLL_PUBLIC extern GFX_Result MApi_GFX_GetClip(GFX_Point *v0, GFX_Point *v1);
DLL_PUBLIC extern GFX_Result MApi_GFX_GetIntensity(MS_U32 idx, MS_U32 *color);
DLL_PUBLIC extern GFX_Result MApi_GFX_GetTAGID(MS_U16 *tagID);
DLL_PUBLIC extern MS_U16 MApi_GFX_GetNextTAGID(MS_BOOL bStepTagBefore);
DLL_PUBLIC extern GFX_Result MApi_GFX_GetGECaps(EN_GFX_CAPS eCapType, MS_U32 *pRet,
						MS_U32 ret_size);
DLL_PUBLIC extern GFX_Result MApi_GFX_DrawLine(GFX_DrawLineInfo *pline);
DLL_PUBLIC extern GFX_Result MApi_GFX_RectFill(GFX_RectFillInfo *pfillblock);
DLL_PUBLIC extern GFX_Result MApi_GFX_TriFill(GFX_TriFillInfo *ptriblock);
DLL_PUBLIC extern GFX_Result MApi_GFX_SpanFill(GFX_SpanFillInfo *pspanblock);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetSrcBufferInfo(GFX_BufferInfo *bufInfo,
						       MS_U32 offsetofByte);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetDstBufferInfo(GFX_BufferInfo *bufInfo,
						       MS_U32 offsetofByte);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetROP2(MS_BOOL enable, GFX_ROP2_Op eRopMode);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetSrcColorKey(MS_BOOL enable,
						     GFX_ColorKeyMode opMode,
						     GFX_Buffer_Format fmt, void *ps_color,
						     void *pe_color);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetDstColorKey(MS_BOOL enable,
						     GFX_ColorKeyMode opMode,
						     GFX_Buffer_Format fmt, void *ps_color,
						     void *pe_color);
DLL_PUBLIC extern GFX_Result MApi_GFX_EnableAlphaBlending(MS_BOOL enable);
DLL_PUBLIC extern GFX_Result MApi_GFX_EnableDFBBlending(MS_BOOL enable);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetDFBBldFlags(MS_U16 u16DFBBldFlags);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetDFBBldOP(GFX_DFBBldOP gfxSrcBldOP,
						  GFX_DFBBldOP gfxDstBldOP);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetDFBBldConstColor(GFX_RgbColor gfxRgbColor);
DLL_PUBLIC extern GFX_Result MApi_GFX_BitBlt(GFX_DrawRect *drawbuf, MS_U32 drawflag);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetIntensity(MS_U32 id, GFX_Buffer_Format fmt,
						   MS_U32 *pColor);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetClip(GFX_Point *v0, GFX_Point *v1);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetDither(MS_BOOL enable);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetNearestMode(MS_BOOL enable);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetMirror(MS_BOOL isMirrorX, MS_BOOL isMirrorY);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetDstMirror(MS_BOOL isMirrorX, MS_BOOL isMirrorY);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetRotate(GFX_RotateAngle angle);
DLL_PUBLIC extern GFX_Result MApi_GFX_WaitForTAGID(MS_U16 tagID);
DLL_PUBLIC extern MS_U16 MApi_GFX_SetNextTAGID(void);
DLL_PUBLIC extern GFX_Result MApi_GFX_EnableVCmdQueue(MS_U16 blEnable);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetVCmdBuffer(MS_PHY PhyAddr,
						    GFX_VcmqBufSize enBufSize);
DLL_PUBLIC extern GFX_Result MApi_GFX_FlushQueue(void);
DLL_PUBLIC extern MS_U32 MApi_GFX_SetPowerState(EN_POWER_MODE u16PowerState);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetPaletteOpt(GFX_PaletteEntry *pPalArray,
						    MS_U16 u32PalStart, MS_U16 u32PalEnd);

DLL_PUBLIC extern GFX_Result MApi_GFX_SetAlphaSrcFrom(GFX_AlphaSrcFrom eMode);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetAlphaBlending(GFX_BlendCoef blendcoef,
						       MS_U8 blendfactor);

DLL_PUBLIC extern GFX_Result MApi_GFX_Line_Pattern_Reset(void);
DLL_PUBLIC extern GFX_Result MApi_GFX_Set_Line_Pattern(MS_BOOL enable, MS_U8 linePattern,
						       MS_U8 repeatFactor);

DLL_PUBLIC extern GFX_Result MApi_GFX_BeginDraw(void);
DLL_PUBLIC extern GFX_Result MApi_GFX_EndDraw(void);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetDC_CSC_FMT(GFX_YUV_Rgb2Yuv mode,
						    GFX_YUV_OutRange yuv_out_range,
						    GFX_YUV_InRange uv_in_range,
						    GFX_YUV_422 srcfmt, GFX_YUV_422 dstfmt);
DLL_PUBLIC extern GFX_Result MApi_GFX_GetBufferInfo(PGFX_BufferInfo srcbufInfo,
						    PGFX_BufferInfo dstbufInfo);
DLL_PUBLIC extern GFX_Result MApi_GFX_ClearFrameBufferByWord(MS_PHY StrAddr, MS_U32 length,
							     MS_U32 ClearValue);
DLL_PUBLIC extern GFX_Result MApi_GFX_ClearFrameBuffer(MS_PHY StrAddr, MS_U32 length,
						       MS_U8 ClearValue);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetAlphaCmp(MS_BOOL enable, GFX_ACmpOp eMode);
DLL_PUBLIC extern GFX_Result MApi_GFX_SetStrBltSckType(GFX_StretchCKType type,
						       GFX_RgbColor *color);
DLL_PUBLIC extern GFX_Result MApi_GFX_DrawOval(GFX_OvalFillInfo *pOval);
void GFXRegisterToUtopia(FUtopiaOpen ModuleType);
MS_U32 GFXStr(MS_U32 u32PowerState, void *pModule);
GFX_Result _MApi_GE_GetCRC(MS_U32 *pu32CRCvalue);
#endif


#ifdef __cplusplus
}
#endif
#endif				//_API_GFX_H_
