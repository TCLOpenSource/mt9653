/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __UAPI_MTK_V4L2_G2D_H__
#define __UAPI_MTK_V4L2_G2D_H__

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>

#define V4L2_CID_EXE_G2D_OFFSET 0xc000
#define V4L2_CID_EXE_G2D_BASE	(V4L2_CID_USER_BASE+V4L2_CID_EXE_G2D_OFFSET)
#define V4L2_CID_EXT_G2D_GET_RESOURCE	(V4L2_CID_EXE_G2D_BASE+1)
#define V4L2_CID_EXT_G2D_FREE_RESOURCE	(V4L2_CID_EXE_G2D_BASE+2)
#define V4L2_CID_EXT_G2D_RENDER	(V4L2_CID_EXE_G2D_BASE+3)
#define V4L2_CID_EXT_G2D_CTRL	(V4L2_CID_EXE_G2D_BASE+4)
#define V4L2_CID_EXT_G2D_WAIT_DONE	(V4L2_CID_EXE_G2D_BASE+5)

#define V4L2_PIX_FMT_I1    v4l2_fourcc('P', 'A', 'L', '1')
#define V4L2_PIX_FMT_I2    v4l2_fourcc('P', 'A', 'L', '2')
#define V4L2_PIX_FMT_I4    v4l2_fourcc('P', 'A', 'L', '4')
#define G2D_LINE_FLAG_COLOR_CONSTANT             (0x00000000UL)	/// Constant color
#define G2D_LINE_FLAG_COLOR_GRADIENT             (0x00000001UL)	/// Gradient color
#define G2D_LINE_FLAG_DRAW_LASTPOINT             (0x40000000UL)	/// Otherwide, not draw last point
#define G2D_LINE_FLAG_INWIDTH_IS_OUTWIDTH    (0x80000000UL) ///Otherwise, out width is in width +1
#define G2D_RECT_FLAG_COLOR_CONSTANT     0x0UL
#define G2D_RECT_FLAG_COLOR_GRADIENT_X   0x1UL
#define G2D_RECT_FLAG_COLOR_GRADIENT_Y   0x2UL
#define G2D_DRAW_FLAG_DEFAULT            0x0UL
#define G2D_DRAW_FLAG_SCALE              0x1UL
#define G2D_DFB_BLD_FLAG_COLORALPHA             0x0001UL
#define G2D_DFB_BLD_FLAG_ALPHACHANNEL           0x0002UL
#define G2D_DFB_BLD_FLAG_COLORIZE               0x0004UL
#define G2D_DFB_BLD_FLAG_SRCPREMUL              0x0008UL
#define G2D_DFB_BLD_FLAG_SRCPREMULCOL           0x0010UL
#define G2D_DFB_BLD_FLAG_DSTPREMUL              0x0020UL
#define G2D_DFB_BLD_FLAG_XOR                    0x0040UL
#define G2D_DFB_BLD_FLAG_DEMULTIPLY             0x0080UL
#define G2D_DFB_BLD_FLAG_SRCALPHAMASK           0x0100UL
#define G2D_DFB_BLD_FLAG_SRCCOLORMASK           0x0200UL
#define G2D_DFB_BLD_FLAG_ALL                    0x03FFUL

enum v4l2_g2d_render_type {
	V4L2_G2D_DRAW_LINE = 0,
	V4L2_G2D_RECTFILL,
	V4L2_G2D_BITBLT,
	V4L2_G2D_DRAW_OVAL,
	V4L2_G2D_SPANFILL,
	V4L2_G2D_TRIFILL,
	V4L2_G2D_CLEAR_FRAME,
	V4L2_G2D_CLEAR_FRAME_BY_WORD,
};

struct v4l2_g2d_block {
	__u16 x;
	__u16 y;
	__u16 width;
	__u16 height;
} __attribute__ ((__packed__));

struct v4l2_argb_color {
	__u8 a;
	__u8 r;
	__u8 g;
	__u8 b;
} __attribute__ ((__packed__));

struct v4l2_g2d_color_range {
	struct v4l2_argb_color color_s;
	struct v4l2_argb_color color_e;
} __attribute__ ((__packed__));

struct v4l2_g2d_line_pattern {
	__u8 enable;
	__u8 linePatten;
	__u8 repeatFactor;
} __attribute__ ((__packed__));

struct v4l2_g2d_drawline {
	__u16 x1;
	__u16 y1;
	__u16 x2;
	__u16 y2;
	__u32 width;
	__u32 drawflag;
	struct v4l2_g2d_color_range colorRange;
	struct v4l2_g2d_line_pattern pattern;
	__u8 reset;
} __attribute__ ((__packed__));

struct v4l2_g2d_rect {
	struct v4l2_g2d_block dstBlock;
	struct v4l2_g2d_color_range colorRange;
	__u32 drawflag;
} __attribute__ ((__packed__));

struct v4l2_g2d_bitblt {
	struct v4l2_g2d_block srcblock;
	struct v4l2_g2d_block dstblock;
	__u32 drawflag;
} __attribute__ ((__packed__));

struct v4l2_g2d_drawoval {
	__u32 lineWidth;
	struct v4l2_g2d_block dstBlock;
	struct v4l2_argb_color color;
} __attribute__ ((__packed__));

struct v4l2_g2d_spans {
	__u32 x;
	__u32 width;
} __attribute__ ((__packed__));

struct v4l2_g2d_span {
	__u32 y;
	__u32 num_spans;
	struct v4l2_g2d_spans *spans;
#if !defined(__aarch64__)
	void *pDummy;
#endif
} __attribute__ ((__packed__));

struct v4l2_g2d_spanfill {
	__u32 drawflag;
	struct v4l2_g2d_block dstBlock;
	struct v4l2_g2d_color_range colorRange;
	struct v4l2_g2d_span span;
} __attribute__ ((__packed__));

struct v4l2_g2d_triangle {
	__u32 x0;
	__u32 y0;
	__u32 x1;
	__u32 y1;
	__u32 x2;
	__u32 y2;
} __attribute__ ((__packed__));

struct v4l2_g2d_trifill {
	struct v4l2_g2d_triangle triangle;
	struct v4l2_g2d_block dstblock;
	struct v4l2_g2d_color_range colorRange;
	__u32 drawflag;
} __attribute__ ((__packed__));

struct v4l2_g2d_clearframe {
	__u64 addr;
	__u32 length;
	__u32 value;
} __attribute__ ((__packed__));

struct v4l2_g2d_buffer {
	__u64 addr;
	__u32 height;
	__u32 width;
	__u32 pitch;
	__u32 fmt;
} __attribute__ ((__packed__));

struct v4l2_ext_g2d_render_info {
	__u32 version;
	__u32 size;
	enum v4l2_g2d_render_type type;
	struct v4l2_g2d_buffer src;
	struct v4l2_g2d_buffer dst;
	__u16 tagID;
	union {
		struct v4l2_g2d_drawline line;
		struct v4l2_g2d_rect rect;
		struct v4l2_g2d_bitblt bitblt;
		struct v4l2_g2d_drawoval oval;
		struct v4l2_g2d_spanfill spanfill;
		struct v4l2_g2d_trifill tri;
		struct v4l2_g2d_clearframe frame;
	};
} __attribute__ ((__packed__));

enum v4l2_g2d_config_type {
	V4L2_G2D_VCMDQ = 0,
	V4L2_G2D_FLUSHQUEUE,
	V4L2_G2D_INTENSITY,
	V4L2_G2D_PALETTEOPT,
	V4L2_G2D_COLORKEY,
	V4L2_G2D_ROP2,
	V4L2_G2D_MIRROR,
	V4L2_G2D_DSTMIRROR,
	V4L2_G2D_DITHER,
	V4L2_G2D_NEARESTMODE,
	V4L2_G2D_STRBLT_CK,
	V4L2_G2D_DC_CSC_FMT,
	V4L2_G2D_ALPHA_CMP,
	V4L2_G2D_ABL,
	V4L2_G2D_DFB,
};

enum v4l2_g2d_vq_size {
	    /// 4K
	    G2D_VQ_4K = 0,
	    /// 8K
	    G2D_VQ_8K = 1,
	    /// 16K
	    G2D_VQ_16K = 2,
	    /// 32K
	    G2D_VQ_32K = 3,
	    /// 64K
	    G2D_VQ_64K = 4,
	    /// 128K
	    G2D_VQ_128K = 5,
	    /// 256K
	    G2D_VQ_256K = 6,
	    /// 512K
	    G2D_VQ_512K = 7,
	    /// 1024k
	G2D_VQ_1024K = 8,
};

struct v4l2_g2d_vcmdq {
	__u8 enable;
	enum v4l2_g2d_vq_size size;
	__u64 addr;
} __attribute__ ((__packed__));

struct v4l2_g2d_intensity {
	__u32 id;
	__u32 fmt;
	struct v4l2_argb_color color;
} __attribute__ ((__packed__));

struct v4l2_g2d_palette {
	__u16 palStart;
	__u16 palEnd;
	struct v4l2_argb_color *color;
#if !defined(__aarch64__)
	void *pDummy;
#endif
} __attribute__ ((__packed__));

struct v4l2_g2d_italic {
	__u8 enable;
	__u8 ini_line;
	__u8 ini_dis;
	__u8 slope;
} __attribute__ ((__packed__));

struct v4l2_g2d_mirror {
	__u8 HMirror;
	__u8 VMirror;
} __attribute__ ((__packed__));

enum v4l2_g2d_rop2_op {
	/// rop_result = 0;
	G2D_ROP2_OP_ZERO = 0,
	/// rop_result = ~( rop_src | rop_dst );
	G2D_ROP2_OP_NOT_PS_OR_PD,
	/// rop_result = ((~rop_src) & rop_dst);
	G2D_ROP2_OP_NS_AND_PD,
	/// rop_result = ~(rop_src);
	G2D_ROP2_OP_NS,
	/// rop_result = (rop_src & (~rop_dst));
	G2D_ROP2_OP_PS_AND_ND,
	/// rop_result = ~(rop_dst);
	G2D_ROP2_OP_ND,
	/// rop_result = ( rop_src ^ rop_dst);
	G2D_ROP2_OP_PS_XOR_PD,
	/// rop_result = ~(rop_src & rop_dst);
	G2D_ROP2_OP_NOT_PS_AND_PD,
	/// rop_result = (rop_src & rop_dst);
	G2D_ROP2_OP_PS_AND_PD,
	/// rop_result = ~(rop_dst ^ rop_src);
	G2D_ROP2_OP_NOT_PS_XOR_PD,
	/// rop_result = rop_dst;
	G2D_ROP2_OP_PD,
	/// rop_result = (rop_dst | (~rop_src));
	G2D_ROP2_OP_NS_OR_PD,
	/// rop_result = rop_src;
	G2D_ROP2_OP_PS,
	/// rop_result = (rop_src | (~rop_dst));
	G2D_ROP2_OP_PS_OR_ND,
	/// rop_result = (rop_dst | rop_src);
	G2D_ROP2_OP_PD_OR_PS,
	/// rop_result = 0xffffff;
	G2D_ROP2_OP_ONE,
};

struct v4l2_g2d_rop2 {
	__u8 enable;
	enum v4l2_g2d_rop2_op eRopMode;
} __attribute__ ((__packed__));

enum v4l2_g2d_colorkey_type {
	G2D_COLORKEY_SRC = 0,
	G2D_COLORKEY_DST,
};

enum v4l2_g2d_colorkey_mode {
	G2D_CK_OP_EQUAL = 0,
	G2D_CK_OP_NOT_EQUAL,
	G2D_AK_OP_EQUAL,
	G2D_AK_OP_NOT_EQUAL,
};

struct v4l2_g2d_colorkey {
	__u8 enable;
	__u32 fmt;
	enum v4l2_g2d_colorkey_type type;
	enum v4l2_g2d_colorkey_mode mode;
	struct v4l2_g2d_color_range range;
} __attribute__ ((__packed__));

enum v4l2_g2d_stretchck_type {
	G2D_DONOTHING = 0,
	G2D_NEAREST,
	G2D_REPLACE_KEY_2_CUS,
};

struct v4l2_g2d_strblt_ck {
	enum v4l2_g2d_stretchck_type type;
	struct v4l2_argb_color color;
} __attribute__ ((__packed__));

enum v4l2_g2d_rgb2yuv {
	G2D_YUV_RGB2YUV_PC = 0,
	G2D_YUV_RGB2YUV_255 = 1,
};

enum v4l2_g2d_yuv_out_range {
	G2D_YUV_OUT_255 = 0,
	G2D_YUV_OUT_PC = 1,
};

enum v4l2_g2d_yuv_in_range {
	G2D_YUV_IN_255 = 0,
	G2D_YUV_IN_127 = 1,
};

enum v4l2_g2d_yuv_422 {
	G2D_YUV_YVYU = 0,
	G2D_YUV_YUYV = 1,
	G2D_YUV_VYUY = 2,
	G2D_YUV_UYVY = 3,
};

struct v4l2_g2d_csc_fmt {
	enum v4l2_g2d_rgb2yuv mode;
	enum v4l2_g2d_yuv_out_range outRange;
	enum v4l2_g2d_yuv_in_range inRange;
	enum v4l2_g2d_yuv_422 srcfmt;
	enum v4l2_g2d_yuv_422 dstfmt;
} __attribute__ ((__packed__));

enum v4l2_g2d_alphacmp_op {
	/// max(Asrc,Adst)
	G2D_ACMP_OP_MAX = 0,
	/// min(Asrc,Adst)
	G2D_ACMP_OP_MIN = 1,
};

struct v4l2_g2d_alphacmp {
	__u8 enable;
	enum v4l2_g2d_alphacmp_op eCmpOp;
} __attribute__ ((__packed__));

enum v4l2_g2d_blendcoef {
	/// Csrc
	G2D_COEF_ONE = 0,
	/// Csrc * Aconst + Cdst * (1 - Aconst)
	G2D_COEF_CONST,
	///  Csrc * Asrc + Cdst * (1 - Asrc)
	G2D_COEF_ASRC,
	/// Csrc * Adst + Cdst * (1 - Adst)
	G2D_COEF_ADST,
	/// Cdst
	G2D_COEF_ZERO,
	/// Csrc * (1 - Aconst) + Cdst * Aconst
	G2D_COEF_1_CONST,
	/// Csrc * (1 - Asrc) + Cdst * Asrc
	G2D_COEF_1_ASRC,
	///  Csrc * (1 - Adst) + Cdst * Adst
	G2D_COEF_1_ADST,
	/// ((Asrc * Aconst) * Csrc + (1-(Asrc *Aconst)) * Cdst) / 2
	G2D_COEF_ROP8_ALPHA,
	/// ((Asrc * Aconst) * Csrc + Adst * Cdst * (1-(Asrc * Aconst))) /
	///(Asrc * Aconst) + Adst * (1- Asrc * Aconst))
	G2D_COEF_ROP8_SRCOVER,
	/// ((Asrc * Aconst) * Csrc * (1-Adst) + Adst * Cdst) / (Asrc * Aconst) * (1-Adst) + Adst)
	G2D_COEF_ROP8_DSTOVER,
	/// Csrc * Aconst
	G2D_COEF_CONST_SRC,
	/// Csrc * (1 - Aconst)
	G2D_COEF_1_CONST_SRC,
	/// Csrc * Adst * Asrc * Aconst + Cdst * Adst * (1 - Asrc * Aconst)
	G2D_COEF_SRC_ATOP_DST,
	/// Cdst * Asrc * Aconst * Adst + Csrc * Asrc * Aconst * (1 - Adst)
	G2D_COEF_DST_ATOP_SRC,
	/// (1 - Adst) * Csrc * Asrc * Aconst + Adst * Cdst * (1 - Asrc * Aconst)
	G2D_COEF_SRC_XOR_DST,
	/// Cconst * Aconst + Cdst * (1 - Aconst)
	G2D_COEF_FADEIN,
	///Cconst * (1 - Aconst) + Cdst * Aconst
	G2D_COEF_FADEOUT
};

enum v4l2_g2d_alpha_srcfrom {
	/// constant
	G2D_ABL_FROM_CONST = 0,
	/// source alpha
	G2D_ABL_FROM_ASRC,
	/// destination alpha
	G2D_ABL_FROM_ADST,
	/// Aout = Asrc*Aconst
	G2D_ABL_FROM_ROP8_SRC,
	/// Aout = Asrc*Aconst * Adst
	G2D_ABL_FROM_ROP8_IN,
	/// Aout = (1-Asrc*Aconst) * Adst
	G2D_ABL_FROM_ROP8_DSTOUT,
	/// Aout = (1-Adst) * Asrc*Aconst
	G2D_ABL_FROM_ROP8_SRCOUT,
	/// Aout = (Asrc*Aconst) + Adst*(1-Asrc*Aconst) or (Asrc*Aconst)*(1-Adst) + Adst
	G2D_ABL_FROM_ROP8_OVER,

	/// 1 - Aconst
	G2D_ABL_FROM_ROP8_INV_CONST,
	/// 1 - Asrc
	G2D_ABL_FROM_ROP8_INV_ASRC,
	/// 1 - Adst
	G2D_ABL_FROM_ROP8_INV_ADST,
	/// Adst * Asrc * Aconst + Adst * (1 - Asrc * Aconst) A atop B
	G2D_ABL_FROM_ROP8_SRC_ATOP_DST,
	/// Asrc * Aconst * Adst + Asrc * Aconst * (1 - Adst) B atop A
	G2D_ABL_FROM_ROP8_DST_ATOP_SRC,
	/// (1 - Adst) * Asrc * Aconst + Adst * (1 - Asrc * Aconst) A xor B
	G2D_ABL_FROM_ROP8_SRC_XOR_DST,
	/// Asrc * Asrc * Aconst + Adst * (1 - Asrc * Aconst)
	G2D_ABL_FROM_ROP8_INV_SRC_ATOP_DST,
	/// Asrc * (1 - Asrc * Aconst) + Adst * Asrc * Aconst
	G2D_ABL_FROM_ROP8_INV_DST_ATOP_SRC
};

struct v4l2_g2d_abl {
	__u8 enable;
	enum v4l2_g2d_blendcoef eBlendCoef;
	__u8 constAlpha;
	enum v4l2_g2d_alpha_srcfrom eAlphaSrc;
} __attribute__ ((__packed__));

enum v4l2_g2d_dfbbld_op {
	G2D_DFB_BLD_OP_ZERO = 0,
	G2D_DFB_BLD_OP_ONE = 1,
	G2D_DFB_BLD_OP_SRCCOLOR = 2,
	G2D_DFB_BLD_OP_INVSRCCOLOR = 3,
	G2D_DFB_BLD_OP_SRCALPHA = 4,
	G2D_DFB_BLD_OP_INVSRCALPHA = 5,
	G2D_DFB_BLD_OP_DESTALPHA = 6,
	G2D_DFB_BLD_OP_INVDESTALPHA = 7,
	G2D_DFB_BLD_OP_DESTCOLOR = 8,
	G2D_DFB_BLD_OP_INVDESTCOLOR = 9,
	G2D_DFB_BLD_OP_SRCALPHASAT = 10,
};

struct v4l2_g2d_dfbbld {
	__u8 enable;
	__u32 flags;
	enum v4l2_g2d_dfbbld_op eSrcDFBBldOP;
	enum v4l2_g2d_dfbbld_op eDstDFBBldOP;
	struct v4l2_argb_color color;
} __attribute__ ((__packed__));

struct v4l2_ext_g2d_config {
	__u32 version;
	__u32 size;
	enum v4l2_g2d_config_type type;
	union {
		__u8 enable;
		__u32 value;
		struct v4l2_g2d_vcmdq vcmdq;
		struct v4l2_g2d_intensity intensity;
		struct v4l2_g2d_mirror mirror;
		struct v4l2_g2d_palette palette;
		struct v4l2_g2d_colorkey colorkey;
		struct v4l2_g2d_rop2 rop2;
		struct v4l2_g2d_strblt_ck strbltck;
		struct v4l2_g2d_csc_fmt cscfmt;
		struct v4l2_g2d_alphacmp alphacmp;
		struct v4l2_g2d_abl abl;
		struct v4l2_g2d_dfbbld dfb;
	};
} __attribute__ ((__packed__));
#endif
