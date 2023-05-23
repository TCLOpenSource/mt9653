/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _GOP_H_
#define _GOP_H_

#ifdef __cplusplus
extern "C" {
#endif


#ifndef DLL_PACKED
#define DLL_PACKED __attribute__((__packed__))
#endif

#ifndef DLL_PUBLIC
#define DLL_PUBLIC __attribute__((visibility("default")))
#endif

#ifndef SYMBOL_WEAK
#define SYMBOL_WEAK
#endif

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define DWIN_SUPPORT_BUFFER     0x2UL
#define GOP_MULTIINFO_NUM       8UL

/// Define GWIN color format.
	typedef enum {
		/// Color format RGB565.
		E_GOP_COLOR_RGB565,
		/// Color format ARGB4444.
		E_GOP_COLOR_ARGB4444,
		/// Color format ARGB8888.
		E_GOP_COLOR_ARGB8888,
		/// Color format ARGB1555.
		E_GOP_COLOR_ARGB1555,
		/// Color format ABGR8888.
		E_GOP_COLOR_ABGR8888,
		/// Color format YUV422.
		E_GOP_COLOR_YUV422,
		/// Color format RGBA5551.
		E_GOP_COLOR_RGBA5551,
		/// Color format RGBA4444.
		E_GOP_COLOR_RGBA4444,
		/// Color format RGBA8888.
		E_GOP_COLOR_RGBA8888,
		/// Color format BGR565.
		E_GOP_COLOR_BGR565,
		/// Color format ABGR4444.
		E_GOP_COLOR_ABGR4444,
		/// Color format AYUV8888.
		E_GOP_COLOR_AYUV8888,
		/// Color format ABGR1555.
		E_GOP_COLOR_ABGR1555,
		/// Color format BGRA5551.
		E_GOP_COLOR_BGRA5551,
		/// Color format BGRA4444.
		E_GOP_COLOR_BGRA4444,
		/// Color format BGRA8888.
		E_GOP_COLOR_BGRA8888,
		/// Invalid color format.
		E_GOP_COLOR_INVALID = 0xFFFF,
	} EN_GOP_COLOR_TYPE;


/// Define GOP destination displayplane type
	typedef enum {
		///IP0 path.
		E_GOP_DST_IP0 = 0,
		///OP path.
		E_GOP_DST_OP0,
		/// IP man path.
		E_GOP_DST_IP_MAIN,
		///FRC path
		E_GOP_DST_FRC,
		///4K2K BYPASS path
		E_GOP_DST_BYPASS,
		MAX_GOP_DST_SUPPORT,
	} EN_GOP_DST_TYPE;

// scale direction
	typedef enum {
		// no scale
		E_GOP_NO_STRETCH = 0x00,
		// horizontal scale
		E_GOP_H_STRETCH = 0x01,
		// vertical scale
		E_GOP_V_STRETCH = 0x10,
		// both horizontal and vertical scale
		E_GOP_HV_STRETCH = 0x11,
	} EN_GOP_STRETCH_DIRECTION;

///GOP config type
	typedef enum {
		/// This type is for not init some GOP features.
		/// That is, this is for not changing some registers state
		E_GOP_IGNOREINIT,
		E_GOP_GET_MAXFBNUM,
		E_GOP_SET_SCROLL_AUTO_STOP_POSITION,
		E_GOP_SET_ADDR_BY_MBX,
		E_GOP_VOP_PATH_SEL,
		//Mixer config
		E_GOP_VFILTER_RATIO = 0x100,
		//AFBC
		E_GOP_AFBC_RESET = 0x200,
		E_GOP_AFBC_ENABLE = 0x201,
		E_GOP_GET_STRETCH_INFO = 0x400,
		E_GOP_AUTO_DETECT_BUFFER,
	} EN_GOP_CONFIG_TYPE;

//GOP Test Pattern type
	typedef enum {
		E_GOP_TP_DISABLE = 0,
		E_GOP_TP_GRAY16,
		E_GOP_TP_GRAY16_INVERSE,
		E_GOP_TP_GRAY32,
		E_GOP_TP_GRAY32_INVERSE,
		E_GOP_TP_GRAY64,
		E_GOP_TP_GRAY64_INVERSE,
		E_GOP_TP_GRAY256,
		E_GOP_TP_GRAY256_INVERSE,
		E_GOP_TP_BLACK,
		E_GOP_TP_WHITE,
		E_GOP_TP_CLR,
		E_GOP_TP_BW_DOT,
		E_GOP_TP_BW_LINE,
		E_GOP_TP_BW_CROSS,
		E_GOP_TP_CLR_BAR,
		E_GOP_TP_CLR_BAR_INVERSE,
	} EN_GOP_TST_PATTERN;

/// Define GOP Transparent color Stretch Mode.
	typedef enum {
		E_GOP_TRANSPCOLOR_STRCH_DUPLICATE = 0,
		E_GOP_TRANSPCOLOR_STRCH_ASNORMAL = 1,
	} EN_GOP_STRCH_TRANSPCOLORMODE;

/// Define GOP H-Stretch mode.
	typedef enum {
		/// 6-tape (including nearest) mode.
		E_GOP_HSTRCH_6TAPE = 0,
		/// duplicate mode.
		E_GOP_HSTRCH_DUPLICATE = 1,
		/// 6-tape (Linear mode)
		E_GOP_HSTRCH_6TAPE_LINEAR = 2,
		/// 6-tape (Nearest mode)
		E_GOP_HSTRCH_6TAPE_NEAREST = 3,
		/// 6-tape (Gain0)
		E_GOP_HSTRCH_6TAPE_GAIN0 = 4,
		/// 6-tape (Gain1)
		E_GOP_HSTRCH_6TAPE_GAIN1 = 5,
		/// 6-tape (Gain2)
		E_GOP_HSTRCH_6TAPE_GAIN2 = 6,
		/// 6-tape (Gain3)
		E_GOP_HSTRCH_6TAPE_GAIN3 = 7,
		/// 6-tape (Gain4)
		E_GOP_HSTRCH_6TAPE_GAIN4 = 8,
		/// 6-tape (Gain5)
		E_GOP_HSTRCH_6TAPE_GAIN5 = 9,
		/// 4-tap filer
		E_GOP_HSTRCH_4TAPE = 0xA,
		///2-tape
		E_GOP_HSTRCH_2TAPE = 0xB,
		///new 4-tap, is most fuzziest when apply H new 4-tap
		E_GOP_HSTRCH_NEW4TAP = 0xC,
		/// 8-tap
		E_GOP_HSTRCH_8TAP = 0xD,
		///new 4-tap coef 50.
		E_GOP_HSTRCH_NEW4TAP_50 = 0xE,
		///new 4-tap coef 55.
		E_GOP_HSTRCH_NEW4TAP_55 = 0xF,
		///new 4-tap coef 65.
		E_GOP_HSTRCH_NEW4TAP_65 = 0x10,
		///new 4-tap coef 75.
		E_GOP_HSTRCH_NEW4TAP_75 = 0x11,
		///new 4-tap coef 85.
		E_GOP_HSTRCH_NEW4TAP_85 = 0x12,
		///new 4-tap coef 95.
		E_GOP_HSTRCH_NEW4TAP_95 = 0x13,
		///new 4-tap coef 105, is most sharpest when apply H new 4-tap
		E_GOP_HSTRCH_NEW4TAP_105 = 0x14,

		E_GOP_HSTRCH_NEW4TAP_100 = 0x15,
	} EN_GOP_STRETCH_HMODE;

/// Define GOP V-Stretch mode.
	typedef enum {
		///2-TAP mode.
		E_GOP_VSTRCH_LINEAR = 0,
		/// duplicate mode.
		E_GOP_VSTRCH_DUPLICATE = 1,
		/// nearest mode.
		E_GOP_VSTRCH_NEAREST = 2,
		/// nearest mode.
		E_GOP_VSTRCH_LINEAR_GAIN0 = 3,
		E_GOP_VSTRCH_LINEAR_GAIN1 = 4,
		/// Linear
		E_GOP_VSTRCH_LINEAR_GAIN2 = 5,
		///4-TAP mode
		E_GOP_VSTRCH_4TAP = 6,

		E_GOP_VSTRCH_4TAP_100 = 7,
	} EN_GOP_STRETCH_VMODE;

/// Scroll direction
	typedef enum {
		E_GOP_SCROLL_NONE = 0,
		E_GOP_SCROLL_UP,	// bottom to top
		E_GOP_SCROLL_DOWN,	// top to bottom
		E_GOP_SCROLL_LEFT,	//right to left
		E_GOP_SCROLL_RIGHT,	//left to right
		E_GOP_SCROLL_SW,	//Scroll by SW
		E_GOP_SCROLL_KERNEL,	//Scroll by SW on kernel
	} GOP_SCROLL_TYPE;

//3D OSD mode type
	typedef enum {
		E_GOP_3D_DISABLE,
		E_GOP_3D_SWITCH_BY_FRAME,
		E_GOP_3D_SIDE_BY_SYDE,
		E_GOP_3D_TOP_BOTTOM,
		E_GOP_3D_LINE_ALTERNATIVE,
		E_GOP_3D_FRAMEPACKING,
	} EN_GOP_3D_MODETYPE;

//===================================================
//struct
//===================================================
/// GWIN Information
	typedef struct DLL_PACKED {
		/// gwin v-start (unit: line).
		MS_U16 u16DispVPixelStart;
		/// gwin v-end (unit: line).
		MS_U16 u16DispVPixelEnd;
		/// gwin h-start (unit: pix).
		MS_U16 u16DispHPixelStart;
		/// gwin h-end (unit: pix).
		MS_U16 u16DispHPixelEnd;
		/// gwin dram starting address (unit: Byte).
		MS_PHY u32DRAMRBlkStart;
		/// gwin's frame buffer width (unit: pix).
		MS_U16 u16RBlkHPixSize;
		/// gwin's frame buffer height (unit: pix).
		MS_U16 u16RBlkVPixSize;
		/// gwin's frame buffer pitch (unit: Byte).
		MS_U16 u16RBlkHRblkSize;
		/// gwin's frame buffer x0 (unit: pix).
		MS_U16 u16WinX;
		///  gwin's frame buffer v0 (unit: pix).
		MS_U16 u16WinY;
		/// no use now.
		MS_U32 u32scrX;
		/// no use now.
		MS_U32 u32scrY;
		/// gwin's frame buffer color format.
		EN_GOP_COLOR_TYPE clrType;
	} GOP_GwinInfo;

/// GOP Stretch Window Information
	typedef struct DLL_PACKED {
		/// Destination Type
		EN_GOP_DST_TYPE eDstType;
		/// x start
		MS_U16 x;
		/// y start
		MS_U16 y;
		/// Stretch Window Width
		MS_U16 width;
		/// Stretch Windows Height
		MS_U16 height;
	} GOP_StretchInfo;

/// DWIN property
	typedef struct DLL_PACKED {
		/// dwin h-start (unit: pix).
		MS_U16 u16x;
		/// dwin h-end (unit: pix).
		MS_U16 u16y;
		/// dwin width (unit: pix).
		MS_U16 u16w;
		/// height (unit: line).
		MS_U16 u16h;
		/// dwin dram starting address (unit: Byte).
		MS_PHY u32fbaddr0;
		///  dwin access address high bond (unit: Byte).
		MS_PHY u32fbaddr1;
		/// Number of pixels per horizontal line.
		MS_U16 u16fbw;
	} GOP_DwinProperty;

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
	} GOP_DWinIntInfo;

/// Define the entry of palette.
	typedef union DLL_PACKED {
		///8-bits access.
		struct DLL_PACKED {
			///A.
			MS_U8 u8A;
			///R.
			MS_U8 u8R;
			///G.
			MS_U8 u8G;
			///B.
			MS_U8 u8B;
		} RGB;
		/// 32-bits direct access.
		MS_U8 u8Data[4];
	} GOP_PaletteEntry;

#ifdef __cplusplus
}
#endif
#endif				// _API_GOP_H_
