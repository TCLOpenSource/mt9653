/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _API_GOP_H_
#define _API_GOP_H_



#ifdef __cplusplus
extern "C" {
#endif
// #include "MsDevice.h"
#include "sti_msos.h"
#include "apiGOP_v2.h"
//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define ST_GOP_AUTO_DETECT_BUF_INFO_VERSION    (1)

///GOP_PALETTE_ENTRY_NUM
#define GOP_PALETTE_ENTRY_NUM   256UL
///FB_FMT_AS_DEFAULT
#define FB_FMT_AS_DEFAULT   0xFFFFUL
///DEFAULT_FB_FMT
#define DEFAULT_FB_FMT 0xFFFFUL
///INVALID_WIN_ID
#define INVALID_WIN_ID GWIN_NO_AVAILABLE
///GWIN_ID_INVALID
#define GWIN_ID_INVALID 0xFFUL
///LAYER_ID_INVALID
#define LAYER_ID_INVALID 0xFFFFUL
///INVALID_GOP_NUM
#define INVALID_GOP_NUM  0xFFUL

///MAX_GWIN_SUPPORT
#define MAX_GWIN_SUPPORT MApi_GOP_GWIN_GetTotalGwinNum()


#ifndef MAX_GWIN_FB_SUPPORT
#define MAX_GWIN_FB_SUPPORT MApi_GOP_GWIN_GetMax32FBNum()
#endif


#define MAX_NUM_GOP_API_INF_SUPPORT 6UL

#define MAX_GOP0_SUPPORT MApi_GOP_GWIN_GetGwinNum(0)
#define MAX_GOP1_SUPPORT MApi_GOP_GWIN_GetGwinNum(1)
#define MAX_GOP2_SUPPORT MApi_GOP_GWIN_GetGwinNum(2)
#define MAX_GOP3_SUPPORT MApi_GOP_GWIN_GetGwinNum(3)
#define MAX_GOP4_SUPPORT MApi_GOP_GWIN_GetGwinNum(4)
#define MAX_GOP5_SUPPORT MApi_GOP_GWIN_GetGwinNum(5)

#define GOP0_GWIN_START 0UL
#define GOP1_GWIN_START (MAX_GOP0_SUPPORT)
#define GOP2_GWIN_START (MAX_GOP0_SUPPORT + MAX_GOP1_SUPPORT)
#define GOP3_GWIN_START (MAX_GOP0_SUPPORT + MAX_GOP1_SUPPORT + MAX_GOP2_SUPPORT)
#define GOP4_GWIN_START (MAX_GOP0_SUPPORT + \
						MAX_GOP1_SUPPORT + \
						MAX_GOP2_SUPPORT + \
						MAX_GOP3_SUPPORT)
#define GOP5_GWIN_START (MAX_GOP0_SUPPORT + \
						MAX_GOP1_SUPPORT + \
						MAX_GOP2_SUPPORT + \
						MAX_GOP3_SUPPORT + \
						MAX_GOP4_SUPPORT)

/// GOP API return value
	typedef enum {
		///GOP API return fail.
		GOP_API_FAIL = 0,
		///GOP API return success.
		GOP_API_SUCCESS = 1,
		///GOP API return non align address.
		GOP_API_NON_ALIGN_ADDRESS,
		///GOP API return non align pitch.
		GOP_API_NON_ALIGN_PITCH,
		///GOP API return depend no avail.
		GOP_API_DEPEND_NOAVAIL,
		///GOP API return mutex obtain fail.
		GOP_API_MUTEX_OBTAIN_FAIL,
		///GOP API return mutex obtain.
		GOP_API_MUTEX_OBTAINED,
		///GOP API return invalid buffer info.
		GOP_API_INVALID_BUFF_INFO,
		///GOP API return invalid parameter.
		GOP_API_INVALID_PARAMETERS,
		///GOP API return this interface not supported.
		GOP_API_FUN_NOT_SUPPORTED,
		///GOP API return enum not supported.
		GOP_API_ENUM_NOT_SUPPORTED,
		///GOP API return create gwin fail.
		GOP_API_CRT_GWIN_FAIL = 0xFE,
		///GOP API return create no avail
		GOP_API_CRT_GWIN_NOAVAIL = 0xFF,
	} E_GOP_API_Result;

///GWIN_FAIL
#define GWIN_FAIL         GOP_API_CRT_GWIN_FAIL
///GWIN_OK
#define GWIN_OK           GOP_API_SUCCESS
///GWIN_NO_AVAILABLE
#define GWIN_NO_AVAILABLE GOP_API_CRT_GWIN_NOAVAIL

#define ST_GOP_CSC_PARAM_VERSION (1)
#define ST_GOP_CSC_TABLE_VERSION (1)

///GWINID
	typedef MS_U8 GWINID;

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
/// GWIN display property
	typedef struct DLL_PACKED {
		///gwin id.
		MS_U8 gId;
		///gwin start x coordinate.
		MS_U16 StartX;
		///gwin start y coordinate.
		MS_U16 StartY;
		///gwin win width.
		MS_U16 Win_Width;
		///gwin win height.
		MS_U16 Win_Height;
		///gwin frame buffer offset x.
		MS_U16 FBOffset_X;
		///gwin frame buffer offset y.
		MS_U16 FBOffset_Y;
	} GOP_GwinDispProperty;

/// GOP init info
	typedef struct DLL_PACKED {
		///panel width.
		MS_U16 u16PanelWidth;
		///panel height.
		MS_U16 u16PanelHeight;
		///panel h-start.
		MS_U16 u16PanelHStr;
		///vsync interrupt flip enable flag.
		MS_BOOL bEnableVsyncIntFlip;
		///gop frame buffer starting address.
		MS_PHYADDR u32GOPRBAdr;
		///gop frame buffer length.
		MS_U32 u32GOPRBLen;
		///gop regdma starting address.
		MS_PHYADDR u32GOPRegdmaAdr;
		///gop regdma length.
		MS_U32 u32GOPRegdmaLen;
	} GOP_InitInfo;

/// GOP palette entry
	typedef struct DLL_PACKED {
		/// A.
		MS_U8 u8A;
		/// R.
		MS_U8 u8R;
		/// G.
		MS_U8 u8G;
		/// B.
		MS_U8 u8B;
	} GOP_Palette;

/// Frame buffer attributes for GWIN
	typedef struct DLL_PACKED {
		/// frame buffer is mapped to which gwin.
		MS_U8 gWinId;
		/// frame buffer enable flag.
		MS_U8 enable:1;
		/// frame buffer allocated flag.
		MS_U8 allocated:1;
		/// frame buffer x0 (unit: pix).
		MS_U16 x0;
		/// frame buffer y0 (unit: line).
		MS_U16 y0;
		/// frame buffer x1 (unit: pix).
		MS_U16 x1;
		/// frame buffer y1 (unit: line).
		MS_U16 y1;
		/// frame buffer width (unit: pix).
		MS_U16 width;
		/// frame buffer height (unit: line).
		MS_U16 height;
		/// frame buffer pitch (unit: byte).
		MS_U16 pitch;
		/// frame buffer color format.
		MS_U16 fbFmt;
		/// frame buffer starting address.
		MS_U32 addr;
		/// frame buffer size.
		MS_U32 size;
		/// next frame buffer id in the same pool.
		MS_U8 next;
		/// previous frame buffer in the same pool.
		MS_U8 prev;
		///store which AP use this FB.
		MS_U8 string;
		/// frame buffer s_x (unit: pix).
		MS_U16 s_x;
		/// frame buffer  s_y (unit: pix).
		MS_U16 s_y;
		/// frame buffer dispWidth (unit: pix).
		MS_U16 dispWidth;
		/// frame buffer dispHeight (unit: line).
		MS_U16 dispHeight;
	} GOP_GwinFBAttr;

/// GWIN output color domain
	typedef enum {
		/// output color RGB.
		GOPOUT_RGB,
		/// output color YUV.
		GOPOUT_YUV,
	} EN_GOP_OUTPUT_COLOR;

/// Define GOP FB string.
	typedef enum {
		E_GOP_FB_NULL,
		/// OSD frame buffer.
		E_GOP_FB_OSD,
		/// Mute frame buffer.
		E_GOP_FB_MUTE,
		/// subtitle frame buffer.
		E_GOP_FB_SUBTITLE,
		/// teltext frame buffer.
		E_GOP_FB_TELTEXT,
		/// MHEG5 frame buffer.
		E_GOP_FB_MHEG5,
		/// CC frame buffer.
		E_GOP_FB_CLOSEDCAPTION,
		/// AFBC frame buffer.
		E_GOP_FB_AFBC_SPLT_YUVTRNS_ARGB8888 = 0x100,
		E_GOP_FB_AFBC_NONSPLT_YUVTRS_ARGB8888,
		E_GOP_FB_AFBC_SPLT_NONYUVTRS_ARGB8888,
		E_GOP_FB_AFBC_NONSPLT_NONYUVTRS_ARGB8888,
		E_GOP_FB_AFBC_V1P2_ARGB8888,	//v1.2
		E_GOP_FB_AFBC_V1P2_RGB565,	//v1.2
		E_GOP_FB_AFBC_V1P2_RGB888,	//v1.2
	} EN_GOP_FRAMEBUFFER_STRING;

///the GOP and mux setting info on utopia
	typedef struct DLL_PACKED _GOP_MuxConfig {
		//GopMux arrays record the u8GopIndex and the corresponding MuxIndex
		//u8MuxCounts: how many mux need to modify the gop settings

		struct DLL_PACKED {
			MS_U8 u8GopIndex;
			MS_U8 u8MuxIndex;
		} GopMux[6];
		MS_U8 u8MuxCounts;
	} GOP_MuxConfig, *PGOP_MuxConfig;

///the GOP and OSD Layer setting info on utopia
	typedef struct DLL_PACKED _GOP_LayerConfig {
		struct DLL_PACKED {
			MS_U32 u32GopIndex;
			MS_U32 u32LayerIndex;
		} stGopLayer[6];
		MS_U32 u32LayerCounts;
	} GOP_LayerConfig, *PGOP_LayerConfig;

	typedef struct DLL_PACKED {
		MS_U8 gWinId;
		MS_U32 u32FlipAddr;
		MS_U32 u32SubAddr;
		MS_U16 u16WaitTagID;
		MS_U16 *pU16QueueCnt;
	} GOP_FlipConfig, *PGOP_FlipConfig;

	typedef struct DLL_PACKED {
		MS_U8 u8InfoCnt;
		GOP_FlipConfig astGopInfo[GOP_MULTIINFO_NUM];
	} GOP_MultiFlipInfo, *PGOP_MultiFlipInfo;

/// Define Mux
	typedef enum {
		///Select gop output to mux0
		EN_GOP_MUX0 = 0,
		/// Select gop output to mux1
		EN_GOP_MUX1 = 1,
		/// Select gop output to mux2
		EN_GOP_MUX2 = 2,
		/// Select gop output to mux3
		EN_GOP_MUX3 = 3,
		///Select gop output to IP0
		EN_GOP_IP0_MUX = 4,
		/// Select gop output to FRC mux0
		EN_GOP_FRC_MUX0 = 5,
		/// Select gop output to FRC mux1
		EN_GOP_FRC_MUX1 = 6,
		/// Select gop output to FRC mux2
		EN_GOP_FRC_MUX2 = 7,
		/// Select gop output to FRC mux3
		EN_GOP_FRC_MUX3 = 8,
		/// Select gop output to BYPASS mux
		EN_GOP_BYPASS_MUX0 = 9,
		/// Select gop output to mux4
		EN_GOP_MUX4 = 10,
		EN_MAX_GOP_MUX_SUPPORT,
	} EN_Gop_MuxSel;

/// Define DEBUG level.
	typedef enum {
		/// GOP DEBUG LEVEL LOW. Just printf error message.
		E_GOP_Debug_Level_LOW = 0,
		/// GOP DEBUG LEVEL Medium. printf warning message and error message.
		E_GOP_Debug_Level_MED = 1,
		/// GOP DEBUG LEVEL hIGH. printf all message with function.
		E_GOP_Debug_Level_HIGH = 2,
	} EN_GOP_DEBUG_LEVEL;

/// Define GOP Ignore init value
	typedef enum {
		/// GOP init ignore mux init
		E_GOP_IGNORE_MUX = 0x0001,
		/// GOP init ignore gwin reset
		E_GOP_IGNORE_GWIN = 0x0002,
		/// GOP init ignore stretchwin reset
		E_GOP_IGNORE_STRETCHWIN = 0x0004,
		/// GOP init ignore palette table reset
		E_GOP_IGNORE_PALETTE = 0x0008,
		/// GOP init ignore rest GOP destination to OP
		E_GOP_IGNORE_SET_DST_OP = 0x0010,
		/// GOP init ignore enable transparent color key
		E_GOP_IGNORE_ENABLE_TRANSCLR = 0x0020,
		/// GOP set destination ignore VEOSD for bootlogo case
		E_GOP_BOOTLOGO_IGNORE_VEOSDEN = 0x0040,
		/// GOP init ignore all
		E_GOP_IGNORE_ALL = 0xFFFF,
		///Not ignore
		E_GOP_IGNORE_DISABLE = 0x0000,
	} EN_GOP_IGNOREINIT;


//GOP Capability
	typedef enum {
		E_GOP_CAP_GWIN_NUM,
		E_GOP_CAP_RESERVED,
		E_GOP_CAP_PIXELMODE_SUPPORT,
		E_GOP_CAP_STRETCH,
		E_GOP_CAP_AFBC_SUPPORT,
		E_GOP_CAP_BNKFORCEWRITE,
		E_GOP_CAP_SUPPORT_V4TAP_256PHASE,
		E_GOP_CAP_SUPPORT_H8V4TAP,
		E_GOP_CAP_SUPPORT_V_SCALING_BILINEAR,
	} EN_GOP_CAPS;

//GOP Function work type
	typedef enum {
		E_GOP_FUN_AFBC,
	} EN_GOP_FUNS;

// GOP palette type
	typedef enum {
		E_GOP_CAP_PAL_SIZE_NONE = 0x0000,
		E_GOP_CAP_PAL_SIZE_256 = 0x0100,
		E_GOP_CAP_PAL_SIZE_64 = 0x0200,
		E_GOP_CAP_PAL_SIZE_MASK = 0x0F00,
	} EN_GOP_CAP_PAL;

///GOP palette info
	typedef struct DLL_PACKED {
		MS_U32 GOP_NumOfTbl;
		MS_U32 GOP_PalTbl[0x10];
		MS_U32 GOP_PalTblIdx;
	} GOP_CAP_PAL_TYPE, *PGOP_CAP_PAL_TYPE;

///GOP palette info
	typedef struct DLL_PACKED {
		MS_U32 GOP_VStretch_Support;
	} GOP_CAP_STRETCH_INFO, *PGOP_CAP_STRETCH_INFO;

///GOP AFBC support info
	typedef struct {
		MS_U32 GOP_AFBC_Support;
	} GOP_CAP_AFBC_INFO, *PGOP_CAP_AFBC_INFO;

///GOP timing Information
	typedef struct DLL_PACKED {
		MS_U16 u16HDTotal;	// Output horizontal total
		MS_U16 u16VDTotal;	//Output vertical total

		MS_U16 u16DEHStart;	//Output DE horizontal start
		MS_U16 u16DEHSize;	// Output DE horizontal size

		MS_U16 u16DEVStart;	//Output DE vertical start
		MS_U16 u16DEVSize;	//Output DE Vertical size

		MS_BOOL bInterlaceMode;
		MS_BOOL bYUVInput;
		MS_BOOL bCLK_EN;
		MS_BOOL bINVALPHA_EN;
	} ST_GOP_TIMING_INFO, *PST_GOP_TIMING_INFO;

	typedef struct DLL_PACKED {
		MS_U16 Matrix[3][3];
	} ST_GOP_R2Y_Matrix, *PST_GOP_R2Y_Matrix;

	typedef struct DLL_PACKED {
		MS_U32 u32Version;
		MS_U32 u32Length;
		MS_BOOL bEnable;
		MS_U8 u8GOPNum;
		MS_U8 u8AlphaTh;
		MS_BOOL bLargeThanTH;
	} ST_GOP_AUTO_DETECT_BUF_INFO, *PST_GOP_AUTO_DETECT_BUF_INFO;

	typedef enum {
		E_GOP_VOPPATH_DEF = 0x0,
		E_GOP_VOPPATH_BEF_RGB3DLOOKUP = 0x1,
		E_GOP_VOPPATH_BEF_PQGAMMA = 0x2,
		E_GOP_VOPPATH_AFT_PQGAMMA = 0x3,
		E_GOP_VOPPATH_MAX,
	} EN_GOP_VOP_PATH_MODE;

//Mapping CFD E_CFD_CFIO define
	typedef enum {
		E_GOP_CFD_CFIO_RGB_NOTSPECIFIED = 0x0,	//means RGB, but no specific colorspace
		E_GOP_CFD_CFIO_RGB_BT601_625 = 0x1,
		E_GOP_CFD_CFIO_RGB_BT601_525 = 0x2,
		E_GOP_CFD_CFIO_RGB_BT709 = 0x3,
		E_GOP_CFD_CFIO_RGB_BT2020 = 0x4,
		E_GOP_CFD_CFIO_SRGB = 0x5,
		E_GOP_CFD_CFIO_ADOBE_RGB = 0x6,
		E_GOP_CFD_CFIO_YUV_NOTSPECIFIED = 0x7,	//means YUV, but no specific colorspace
		E_GOP_CFD_CFIO_YUV_BT601_625 = 0x8,
		E_GOP_CFD_CFIO_YUV_BT601_525 = 0x9,
		E_GOP_CFD_CFIO_YUV_BT709 = 0xA,
		E_GOP_CFD_CFIO_YUV_BT2020_NCL = 0xB,
		E_GOP_CFD_CFIO_YUV_BT2020_CL = 0xC,
		E_GOP_CFD_CFIO_XVYCC_601 = 0xD,
		E_GOP_CFD_CFIO_XVYCC_709 = 0xE,
		E_GOP_CFD_CFIO_SYCC601 = 0xF,
		E_GOP_CFD_CFIO_ADOBE_YCC601 = 0x10,
		E_GOP_CFD_CFIO_DOLBY_HDR_TEMP = 0x11,
		E_GOP_CFD_CFIO_SYCC709 = 0x12,
		E_GOP_CFD_CFIO_DCIP3_THEATER = 0x13,	/// HDR10+
		E_GOP_CFD_CFIO_DCIP3_D65 = 0x14,	/// HDR10+
		E_GOP_CFD_CFIO_RGB_DEFAULT = E_GOP_CFD_CFIO_RGB_BT709,
		E_GOP_CFD_CFIO_YUV_DEFAULT = E_GOP_CFD_CFIO_YUV_BT709,
		E_GOP_CFD_CFIO_MAX,
	} EN_GOP_CFD_CFIO;

//Mapping CFD E_CFD_MC_FORMAT definition
	typedef enum {
		E_GOP_CFD_MC_FORMAT_RGB = 0x0,
		E_GOP_CFD_MC_FORMAT_YUV422 = 0x1,
		E_GOP_CFD_MC_FORMAT_YUV444 = 0x2,
		E_GOP_CFD_MC_FORMAT_YUV420 = 0x3,
		E_GOP_CFD_MC_FORMAT_MAX,
	} EN_GOP_CFD_MC_FORMAT;

//Mapping CFD E_CFD_CFIO_RANGE definition
	typedef enum {
		E_GOP_CFD_CFIO_RANGE_LIMIT = 0x0,
		E_GOP_CFD_CFIO_RANGE_FULL = 0x1,
		E_GOP_CFD_CFIO_RANGE_MAX,
	} EN_GOP_CFD_CFIO_RANGE;

//CFD input param structure
	typedef struct DLL_PACKED {
		MS_U32 u32Version;
		MS_U32 u32Length;
		MS_BOOL bCscEnable;
		MS_BOOL bUpdateWithVsync;
		//STU_CFDAPI_UI_CONTROL
		MS_U16 u16Hue;
		MS_U16 u16Saturation;
		MS_U16 u16Contrast;
		MS_U16 u16Brightness;
		MS_U16 u16RGBGGain[3];
		//STU_CFDAPI_MAIN_CONTROL_LITE
		EN_GOP_CFD_CFIO enInputFormat;
		EN_GOP_CFD_MC_FORMAT enInputDataFormat;
		EN_GOP_CFD_CFIO_RANGE enInputRange;
		EN_GOP_CFD_CFIO enOutputFormat;
		EN_GOP_CFD_MC_FORMAT enOutputDataFormat;
		EN_GOP_CFD_CFIO_RANGE enOutputRange;
	} ST_GOP_CSC_PARAM;

//CFD output structure
	typedef struct DLL_PACKED {
		MS_U32 u32Version;
		MS_U32 u32Length;
		MS_U16 u16CscControl;
		ST_GOP_R2Y_Matrix stCSCMatrix;
		MS_U16 u16BrightnessOffsetR;
		MS_U16 u16BrightnessOffsetG;
		MS_U16 u16BrightnessOffsetB;
	} ST_GOP_CSC_TABLE;

	typedef struct DLL_PACKED {
		MS_BOOL bCropEn;
		MS_U16 u16CropWinXStart;
		MS_U16 u16CropWinYStart;
		MS_U16 u16CropWinXEnd;
		MS_U16 u16CropWinYEnd;
	} ST_GOP_CROP_WINDOW;
//-----------------------------------------------------------------------------
//  Functions
//-----------------------------------------------------------------------------

//-----------------
// GOP_INIT
//-----------------
//-------------------------------------------------------------------------------------------------
/// Initial all GOP driver (include gop0, gop1 ext..)
/// @ingroup INIT
/// @param pGopInit \b IN:gop driver init info
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_Init(GOP_InitInfo *pGopInit);
//-------------------------------------------------------------------------------------------------
/// Initial individual GOP driver
/// @ingroup INIT
/// @param pGopInit \b IN:gop driver init info
/// @param u8GOP \b IN: only init by which gop
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_InitByGOP(GOP_InitInfo *pGopInit, MS_U8 u8GOP);
//-----------------
// GOP_CAPS
//-----------------
//-------------------------------------------------------------------------------------------------
/// Get maximum support gop number
/// @ingroup GOP_CAPS
/// @param  void                \b IN: none
/// @return gop number
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_U8 MApi_GOP_GWIN_GetMaxGOPNum(void);
//-------------------------------------------------------------------------------------------------
/// Get maximum support gwin number by all gop
/// @ingroup GOP_CAPS
/// @param  void                \b IN: none
/// @return gwin number
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_U8 MApi_GOP_GWIN_GetTotalGwinNum(void);
//-------------------------------------------------------------------------------------------------
/// Get maximum gwin number by individual gop
/// @ingroup GOP_CAPS
/// @param u8GopNum \b IN: Number of GOP
/// @return maximum gwin number by individual gop
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_U8 MApi_GOP_GWIN_GetGwinNum(MS_U8 u8GopNum);
//-----------------
// RESET
//-----------------
//-------------------------------------------------------------------------------------------------
/// GOP reset GOP resource
/// @ingroup RESET
/// @param  u32Gop                \b IN: GOP
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result Mapi_GOP_GWIN_ResetGOP(MS_U32 u32Gop);
//-----------------
// GOP_POWER_STATE
//-----------------
//-------------------------------------------------------------------------------------------------
/// set GOP power state
/// @ingroup GOP_POWER_STATE
/// @param enPowerState\b IN power status
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_SetPowerState(EN_POWER_MODE enPowerState);
/********************************************************************************/
/// Set config by GOP; For dynamic usage.
/// @param u8GOP \b IN:  GOP number
/// @param enType \b IN:  GOP info type
/// @param pstInfo \b IN misc info
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetConfigEx(MS_U8 u8Gop,
								     EN_GOP_CONFIG_TYPE enType,
								     void *plist);
/********************************************************************************/
/// Get config by GOP.
/// @param u8GOP \b IN:  GOP number
/// @param enType \b IN:  GOP info type
/// @param pstInfo \b IN misc info
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
/********************************************************************************/
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GetConfigEx(MS_U8 u8Gop,
								     EN_GOP_CONFIG_TYPE enType,
								     void *plist);
//-------------------------------------------------------------------------------------------------
/// API for Query GOP Capability
/// @ingroup GOP_CAPS
/// @param eCapType \b IN: Capability type
/// @param pRet     \b OUT: return value
/// @param ret_size \b IN: input structure size
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GetChipCaps(EN_GOP_CAPS eCapType, void *pRet,
							 MS_U32 ret_size);
//-------------------------------------------------------------------------------------------------
/// MApi_GOP_IsRegUpdated
/// @ingroup GOP_ToBeRemove
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_IsRegUpdated(MS_U8 u8GopType);

//-----------------
// ENABLE
//-----------------
//-------------------------------------------------------------------------------------------------
/// Enable GWIN for display
/// @ingroup ENABLE
/// @param winId \b IN: GWIN id
/// @param bEnable \b IN:
///   - # TRUE Show GWIN
///   - # FALSE Hide GWIN
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_Enable(MS_U8 winId, MS_BOOL bEnable);
//-------------------------------------------------------------------------------------------------
/// MApi_GOP_GWIN_DestroyWin
/// @ingroup GOP_ToBeRemove
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_DestroyWin(MS_U8 gId);
//-------------------------------------------------------------------------------------------------
/*******************************************************************************/
//Set which OSD Layer select which GOP
//@param pGopLayer \b IN:information about GOP and corresponding Layer
//   #u32LayerCounts: the total GOP/Layer counts to set
//   #stGopLayer[i].u32GopIndex :the GOP which need to change Layer
//   #stGopLayer[i].u32LayerIndex :the GOP corresponding Layer
//@return GOP_API_SUCCESS - Success
/*******************************************************************************/
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_SetLayer(GOP_LayerConfig *pGopLayer,
							   MS_U32 u32SizeOfLayerInfo);
//-------------------------------------------------------------------------------------------------
/// Set gop output color type
/// @ingroup COLOR_TRANSFOR
/// @param type \b IN: gop output color type
///   - # GOPOUT_RGB => RGB mode
///   - # GOPOUT_YUV => YUV mode
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_OutputColor_EX(MS_U8 u8GOP,
									     EN_GOP_OUTPUT_COLOR
									     type);
//-----------------
// GWIN_OPTION
//-----------------
//-------------------------------------------------------------------------------------------------
/// Enable/Disable gop alpha inverse
/// @param u8GOP \b IN:  GOP number
/// @ingroup GOP_OPTION
/// @param bEnable \b IN: TRUE or FALSE
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_SetAlphaInverse_EX(MS_U8 u8GOP,
										 MS_BOOL bEnable);
/********************************************************************************/
/// Set GWIN alpha blending
/// @param u8win \b IN GWIN id
/// @param bEnable \b IN
///   - # TRUE enable pixel alpha
///   - # FALSE disable pixel alpha
/// @param u8coef \b IN alpha blending coefficient (0-7)
/// @return GOP_API_SUCCESS - Success
/// if setting constant alpha blending
/// example:
/// MApi_GOP_GWIN_SetBlending(0, FALSE, 0xFF);
/********************************************************************************/
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_SetBlending(MS_U8 u8win, MS_BOOL bEnable,
							      MS_U8 u8coef);
//-----------------
// GOP_CONFIG
//-----------------
//-----------------
// DST
//-----------------
//-------------------------------------------------------------------------------------------------
/// Configure the destination of a specific GOP
/// @ingroup DST
/// @param u8GOP \b IN : Number of GOP
/// @param dsttype \b IN : GOP destination
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_ENUM_NOT_SUPPORTED - GOP destination not support
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_SetGOPDst(MS_U8 u8GOP, EN_GOP_DST_TYPE dsttype);
//-----------------
// MIRROR
//-----------------
//-------------------------------------------------------------------------------------------------
/// Set GOP H-Mirror
/// @ingroup MIRROR
/// @param u8GOP \b IN : Number of GOP
/// @param bEnable \b IN
///   - # TRUE enable
///   - # FALSE disable
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_SetHMirror_EX(MS_U8 u8GOP,
									    MS_BOOL bEnable);
//-------------------------------------------------------------------------------------------------
/// Set GOP V-Mirror
/// @ingroup MIRROR
/// @param u8GOP \b IN : Number of GOP
/// @param bEnable \b IN
///   - # TRUE enable
///   - # FALSE disable
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_SetVMirror_EX(MS_U8 u8GOP,
									    MS_BOOL bEnable);
//-----------------
// SCALE
//-----------------
//-------------------------------------------------------------------------------------------------
/// Set stretch window H-Stretch ratio.
/// Example: gwin size:960*540  target gwin size: 1920*1080
///     step1: MApi_GOP_GWIN_Set_HSCALE(TRUE, 960, 1920);
///     step2: MApi_GOP_GWIN_Set_VSCALE(TRUE, 540, 1080);
///     step3: MApi_GOP_GWIN_Set_STRETCHWIN(u8GOPNum, E_GOP_DST_OP0, 0, 0, 960, 540);
/// @ingroup SCALE
/// @param u8GOP \b IN:  GOP number
/// @param bEnable \b IN:
///   - # TRUE enable
///   - # FALSE disable
/// @param src \b IN: original size
/// @param dst \b IN: target size
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_Set_HSCALE_EX(MS_U8 u8GOP, MS_BOOL bEnable,
								MS_U16 src, MS_U16 dst);
//-------------------------------------------------------------------------------------------------
/// Set stretch window V-Stretch ratio.
/// Example: gwin size:960*540  target gwin size: 1920*1080
///     step1: MApi_GOP_GWIN_Set_HSCALE(TRUE, 960, 1920);
///     step2: MApi_GOP_GWIN_Set_VSCALE(TRUE, 540, 1080);
///     step3: MApi_GOP_GWIN_Set_STRETCHWIN(u8GOPNum, E_GOP_DST_OP0, 0, 0, 960, 540);
/// @ingroup SCALE
/// @param u8GOP \b IN:  GOP number
/// @param bEnable \b IN:
///   - # TRUE enable
///   - # FALSE disable
/// @param src \b IN: original size
/// @param dst \b IN: target size
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_Set_VSCALE_EX(MS_U8 u8GOP, MS_BOOL bEnable,
								MS_U16 src, MS_U16 dst);
//-------------------------------------------------------------------------------------------------
/// Set GOP H stretch mode
/// @ingroup SCALE
/// @param u8GOP \b IN:  GOP number
/// @param HStrchMode \b IN:
///   - # E_GOP_HSTRCH_6TAPE
///   - # E_GOP_HSTRCH_DUPLICATE
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_Set_HStretchMode_EX(MS_U8 u8GOP,
						EN_GOP_STRETCH_HMODE HStrchMode);
//-------------------------------------------------------------------------------------------------
/// Set GOP V stretch mode
/// @ingroup SCALE
/// @param u8GOP \b IN:  GOP number
/// @param VStrchMode \b IN:
///   - # E_GOP_VSTRCH_LINEAR
///   - # E_GOP_VSTRCH_DUPLICATE
///   - # E_GOP_VSTRCH_NEAREST
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_Set_VStretchMode_EX(MS_U8 u8GOP,
						EN_GOP_STRETCH_VMODE VStrchMode);
//-----------------
// GOP_UPDATE
//-----------------
//-----------------
// FORCE_WRITE
//-----------------
//-------------------------------------------------------------------------------------------------
/// Set GOP force write mode for update register. When enable force write mode,
/// all update gop registers action will directly
/// take effect (do not wait next v-sync to update gop register!).
/// @ingroup FORCE_WRITE
/// @param bEnable \b IN: TRUE/FALSE
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_SetForceWrite(MS_BOOL bEnable);
//-------------------------------------------------------------------------------------------------
/// MApi_GOP_GWIN_UpdateRegOnceEx2
/// @ingroup GOP_ToBeRemove
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_UpdateRegOnceEx2(MS_U8 u8GOP,
									       MS_BOOL
									       bWriteRegOnce,
									       MS_BOOL bSync);

/// Extend MApi_GOP_GWIN_UpdateRegOnceEx, update special gop.
/// Set gop update register method by only once.
/// Example: if you want to update GOP function A, B, C in the same V sync,
/// please write down your code like below
/// MApi_GOP_GWIN_UpdateRegOnceByMask(u16GopMask,TRUE, TRUE);
/// GOP_FUN_A;
/// GOP_FUN_B;
/// GOP_FUN_C;
/// MApi_GOP_GWIN_UpdateRegOnceByMask(u16GopMask,FALSE, TRUE);
/// @param u16GopMask    \b IN:bit0-gop0, bit1-gop1...
/// @param bWriteRegOnce    \b IN: TRUE/FALSE
/// @param bSync            \b IN: TRUE/FALSE
/// @return GOP_API_SUCCESS - Success
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_UpdateRegOnceByMask(MS_U16 u16GopMask,
										  MS_BOOL
										  bWriteRegOnce,
										  MS_BOOL bSync);
//-----------------
// SHARE
//-----------------
//-------------------------------------------------------------------------------------------------
/// Set GWin Attribute to Shared. If shared GWin, More than one process could
/// access this GWin.
/// @ingroup SHARE
/// @param winId \b IN: GWIN ID for shared
/// @param bIsShared \b IN: shared or not
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_SetGWinShared(MS_U8 winId, MS_BOOL bIsShared);
//-------------------------------------------------------------------------------------------------
/// Set Reference cnt of shared GWin.
/// @ingroup SHARE
/// @param winId \b IN: GWIN ID for shared
/// @param u16SharedCnt \b IN: shared reference cnt.
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_SetGWinSharedCnt(MS_U8 winId,
								   MS_U16 u16SharedCnt);
//-----------------
// EXTRA
//-----------------
//-------------------------------------------------------------------------------------------------
/// API for set GWIN Pre Alpha Mode
/// @ingroup EXTRA
/// @param gWinId \b IN: GWin ID
/// @param bEnable \b IN:
///   - # TRUE enable new alpha mode
///   - # FALSE disable new alpha mode
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_SetPreAlphaMode(MS_U8 u8GOP, MS_BOOL bEnble);
//-------------------------------------------------------------------------------------------------
/// Check if all some GWIN is currently enabled
/// @ingroup ENABLE
/// @param  winId \b IN: gwin id
/// @return  - the according GWin is enabled or not
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_BOOL MApi_GOP_GWIN_IsGWINEnabled(MS_U8 winId);
//-------------------------------------------------------------------------------------------------
/// MApi_GOP_GWIN_IsEnabled
/// @ingroup GOP_ToBeRemove
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_BOOL MApi_GOP_GWIN_IsEnabled(void);
//-------------------------------------------------------------------------------------------------
/// API for set GWIN resolution in one function
/// @ingroup SET_STRETCH_WINDOW
/// @param u8GwinId \b IN: GWin ID
/// @param u8FbId \b IN: Frame Buffer ID
/// @param pGwinInfo \b IN: pointer to GOP_GwinInfo structure
/// @param pStretchInfo \b IN: pointer to GOP_StretchInfo
/// @param direction \b IN: to decide which direction to stretch
/// @param u16DstWidth \b IN: set scaled width if H direction is specified
/// @param u16DstHeight \b IN: set scaled height if V direction is specified
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_SetResolution_32FB(MS_U8 u8GwinId, MS_U32 u32FbId,
								     GOP_GwinInfo *pGwinInfo,
								     GOP_StretchInfo *pStretchInfo,
								     EN_GOP_STRETCH_DIRECTION
								     direction, MS_U16 u16DstWidth,
								     MS_U16 u16DstHeight);
//-------------------------------------------------------------------------------------------------
/// GOP Exit
/// @ingroup INIT
/// @param  void                \b IN: none
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC void MApi_GOP_Exit(void);
//-----------------
// FLIP
//-----------------
//-------------------------------------------------------------------------------------------------
/// Change a GWIN's frame buffer, this enables an off screen buffer to be shown
/// @ingroup FLIP
/// @param fbId \b IN frame buffer id
/// @param gwinId \b IN \copydoc GWINID
/// @return GOP_API_SUCCESS - Success
/// @return GOP_API_FAIL - Failure
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_Map32FB2Win(MS_U32 u32fbId, MS_U8 u8gwinId);
//-----------------
// FB_DELETE
//-----------------
//-------------------------------------------------------------------------------------------------
/// Destroy a frame buffer
/// @ingroup FB_DELETE
/// @param fbId \b IN frame buffer id
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC E_GOP_API_Result MApi_GOP_GWIN_Delete32FB(MS_U32 u32fbId);
//-------------------------------------------------------------------------------------------------
/// Get maximum FB number by all gop
/// @ingroup GOP_CAPS
/// @param  void                \b IN: none
/// @return maximum fb number by all gop
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_U32 MApi_GOP_GWIN_GetMax32FBNum(void);
//-------------------------------------------------------------------------------------------------
/// MApi_GOP_GWIN_Destroy32FB
/// @ingroup GOP_ToBeRemove
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_U8 MApi_GOP_GWIN_Destroy32FB(MS_U32 u32fbId);
//-------------------------------------------------------------------------------------------------
/// MApi_GOP_GWIN_Create32FBbyStaticAddr2
/// @ingroup GOP_ToBeRemove
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_U8 MApi_GOP_GWIN_Create32FBbyStaticAddr2(MS_U32 u32fbId, MS_U16 dispX,
							       MS_U16 dispY, MS_U16 width,
							       MS_U16 height, MS_U16 fbFmt,
							       MS_PHY phyFbAddr,
							       EN_GOP_FRAMEBUFFER_STRING FBString);
//-----------------
// FB_STATUS
//-----------------
//-------------------------------------------------------------------------------------------------
/// Get free frame buffer id
/// @ingroup FB_STATUS
/// @return frame buffer id. If return oxFF, it represents no free frame buffer id for use.
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC MS_U32 MApi_GOP_GWIN_GetFree32FBID(void);
//-------------------------------------------------------------------------------------------------
/// MApi_GOP_GWIN_SetMux
/// @ingroup GOP_ToBeRemove
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_SetMux(GOP_MuxConfig *pGopMuxConfig,
								     MS_U32 u32SizeOfMuxInfo);
//-------------------------------------------------------------------------------
/// MApi_GOP_SetGOPCscTuning
/// Using the input parameter which is calculated by CFD drvier, and set to GOP HW
/// @ingroup GOP CSC
/// @param u32GOPNum \b IN The GOP number
/// @param pstGOPCscParam \b IN CFD input param
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetGOPCscTuning(MS_U32 u32GOPNum,
									 ST_GOP_CSC_PARAM *
									 pstGOPCscParam);
//-------------------------------------------------------------------------------
/// MApi_GOP_GetOsdNonTransCnt
/// Use to get OSD non transparent pixel count
/// @ingroup SHARE
/// @param pu32Count \b OUT OSD non transparent pixel count
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GetOsdNonTransCnt(MS_U32 *pu32Count);

//-------------------------------------------------------------------------------
/// MApi_GOP_GetOsdNonTransCnt
/// Use to set GOP pipe delay when enable pixel shift
/// @ingroup SHARE
/// @param s32Offset \b In offset to be set
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetPixelShiftPD(MS_S32 s32Offset);

DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetCropWindow(MS_U8 u8GOPIdx,
								       ST_GOP_CROP_WINDOW *
								       pstCropWin);
//-------------------------------------------------------------------------------
/// MApi_GOP_GWIN_INTERRUPT
/// Use to control GOP interrupt mask
/// @ingroup SHARE
/// @param u8GOPNum \b The GOP number
/// @param bEnable  \b IN:  ENABLE/DISABLE
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_INTERRUPT(MS_U8 u8GOPNum,
									MS_BOOL bEnable);

DLL_PUBLIC SYMBOL_WEAK MS_U8 MApi_GOP_GetGwinIdx(MS_U8 u8GOPIdx);

#ifdef STI_PLATFORM_BRING_UP
DLL_PUBLIC SYMBOL_WEAK void GOPRegisterToUtopia(void);
#endif
#ifdef __cplusplus
}
#endif
#endif				// _API_GOP_H_
