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

///FB_FMT_AS_DEFAULT
#define FB_FMT_AS_DEFAULT   0xFFFFUL
///GWIN_ID_INVALID
#define GWIN_ID_INVALID 0xFFUL

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

///GWINID
	typedef MS_U8 GWINID;

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
/// GOP init info
	typedef struct DLL_PACKED {
		///panel width.
		MS_U16 u16PanelWidth;
		///panel height.
		MS_U16 u16PanelHeight;
		///panel h-start.
		MS_U16 u16PanelHStr;
		/// Menu load fd
		MS_U32 u32ML_Fd;
	} GOP_InitInfo;

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

///the GOP and OSD Layer setting info on utopia
	typedef struct DLL_PACKED _GOP_LayerConfig {
		struct DLL_PACKED {
			MS_U32 u32GopIndex;
			MS_U32 u32LayerIndex;
		} stGopLayer[6];
		MS_U32 u32LayerCounts;
	} GOP_LayerConfig, *PGOP_LayerConfig;

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

/// Define GOP Ignore init value
	typedef enum {
		/// GOP init ignore mux init
		E_GOP_IGNORE_MUX = 0x0001,
		/// GOP init ignore gwin reset
		E_GOP_IGNORE_GWIN = 0x0002,
		/// GOP init ignore stretchwin reset
		E_GOP_IGNORE_STRETCHWIN = 0x0004,
		/// GOP init ignore rest GOP destination to OP
		E_GOP_IGNORE_SET_DST_OP = 0x0008,
		/// GOP init ignore enable transparent color key
		E_GOP_IGNORE_ENABLE_TRANSCLR = 0x0010,
		/// GOP set destination ignore VEOSD for bootlogo case
		E_GOP_IGNORE_SWRST = 0x0020,
		/// GOP init ignore all
		E_GOP_IGNORE_ALL = 0xFFFF,
		///Not ignore
		E_GOP_IGNORE_DISABLE = 0x0000,
	} EN_GOP_IGNOREINIT;


//GOP Capability
	typedef enum {
		E_GOP_CAP_AFBC_SUPPORT,
	} EN_GOP_CAPS;

///GOP AFBC support info
	typedef struct {
		MS_U32 GOP_AFBC_Support;
	} GOP_CAP_AFBC_INFO, *PGOP_CAP_AFBC_INFO;

	typedef enum {
		E_GOP_PATTERN_MOVE_TO_RIGHT,
		E_GOP_PATTERN_MOVE_TO_LEFT,
	} EN_GOP_TESTPATTERN_MOVE_DIR;

	typedef enum {
		E_GOP_OUT_A8RGB10,
		E_GOP_OUT_ARGB8888_DITHER,
		E_GOP_OUT_ARGB8888_ROUND,
	} EN_GOP_OUTPUT_MODE;

	typedef enum {
		E_GOP_VIDEO_OSDB0_OSDB1,
		E_GOP_OSDB0_VIDEO_OSDB1,
		E_GOP_OSDB0_OSDB1_VIDEO,
	} EN_GOP_VG_ORDER;

	typedef struct DLL_PACKED {
		MS_U8  PatEnable;
		MS_U8  HwAutoMode;
		MS_U32 DisWidth;
		MS_U32 DisHeight;
		MS_U32 ColorbarW;
		MS_U32 ColorbarH;
		MS_U8  EnColorbarMove;
		EN_GOP_TESTPATTERN_MOVE_DIR MoveDir;
		MS_U16 MoveSpeed;
	} ST_GOP_TESTPATTERN;

	typedef struct DLL_PACKED {
		MS_U16  Tgen_hs_st;
		MS_U16  Tgen_hs_end;
		MS_U16  Tgen_hfde_st;
		MS_U16  Tgen_hfde_end;
		MS_U16  Tgen_htt;
		MS_U16  Tgen_vs_st;
		MS_U16  Tgen_vs_end;
		MS_U16  Tgen_vfde_st;
		MS_U16  Tgen_vfde_end;
		MS_U16  Tgen_vtt;
		EN_GOP_OUTPUT_MODE GOPOutMode;
	} ST_GOP_TGEN_INFO;

	typedef struct DLL_PACKED {
		void *pCtx;
	} ST_GOP_PQCTX_INFO;

	typedef struct DLL_PACKED {
		MS_S32 fd;
		MS_U32 adl_bufsize;

		MS_U64 __user *u64_user_adl_buf;
		MS_U8 *pu8_adl_buf;
	} ST_GOP_ADL_INFO;

	typedef struct DLL_PACKED {
		MS_U32 u32_cfd_ml_bufsize;
		MS_U64 __user *u64_user_cfd_ml_buf;
		MS_U8 *pu8_cfd_ml_buf;
		MS_U32 u32_pq_ml_bufsize;
		MS_U64 __user *u64_user_pq_ml_buf;
		MS_U8 *pu8_pq_ml_buf;
		MS_BOOL bCFDUpdated;
		MS_BOOL bPQUpdated;
	} ST_GOP_PQ_CFD_ML_INFO;

	typedef struct DLL_PACKED {
		MS_U8 bRetTotal;
		MS_U16 gopIdx;
		MS_U32 Threshold;
		MS_U32 MainVRate;
		MS_U64 RetCount;
	} ST_GOP_GETROI;
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
/// @return GOP_API_SUCCESS - Success
//-------------------------------------------------------------------------------------------------
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GWIN_UpdateRegOnceEx2(MS_U8 u8GOP,
										MS_BOOL
										bWriteRegOnce,
										MS_BOOL bUseMl,
										MS_U8 MlResIdx);

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

DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetTestPattern(
	ST_GOP_TESTPATTERN *pstGOPTestPatternInfo);

DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GetCRC(MS_U32 *CRCvalue);
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_InfoPrint(MS_U8 GOPNum);

DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetGOPBypassPath(MS_U8 u8GOP, MS_BOOL bEnable);

DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetTGen(ST_GOP_TGEN_INFO *pGOPTgenInfo);

DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetTgenVGSync(MS_BOOL bEnable);

DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetMixerMode(MS_BOOL Mixer1_OutPreAlpha,
	MS_BOOL Mixer2_OutPreAlpha);
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetPQCtx(MS_U8 u8GOP,
	ST_GOP_PQCTX_INFO *pGopPQCtx);
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_setAdlnfo(MS_U8 u8GOP,
	ST_GOP_ADL_INFO *pGopAdlInfo);
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_setPqMlnfo(MS_U8 u8GOP,
	ST_GOP_PQ_CFD_ML_INFO *pGopMlInfo);

DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_Ml_Get_Mem_Info(MS_U8 u8MlResIdx);
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_Ml_Fire(MS_U8 u8MlResIdx);
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetAidType(MS_U8 u8GOP, MS_U64 u64GopAidType);
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_GetRoi(ST_GOP_GETROI *pGOPRoi);
DLL_PUBLIC SYMBOL_WEAK E_GOP_API_Result MApi_GOP_SetVGOrder(EN_GOP_VG_ORDER eVGOrder);

#ifdef STI_PLATFORM_BRING_UP
DLL_PUBLIC SYMBOL_WEAK void GOPRegisterToUtopia(void);
#endif
#ifdef __cplusplus
}
#endif
#endif				// _API_GOP_H_
