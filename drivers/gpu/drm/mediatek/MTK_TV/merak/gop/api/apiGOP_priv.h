/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _APIGOP_PRIV_H_
#define _APIGOP_PRIV_H_

#include "apiGOP_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
/// GOP Version
/// GOP CMD
	typedef enum {
		//gop property
		MAPI_CMD_GOP_INIT = 0x0,
		MAPI_CMD_GOP_GET_CHIPCAPS,
		MAPI_CMD_GOP_SET_CONFIG,
		MAPI_CMD_GOP_SET_PROPERTY,
		MAPI_CMD_GOP_SET_DST,
		MAPI_CMD_GOP_SET_LAYER,
		MAPI_CMD_GOP_SET_MIRROR,
		MAPI_CMD_GOP_GET_STATUS,

		//Stretch Win
		MAPI_CMD_GOP_GWIN_SET_STRETCH = 0x100,

		//GWIN info
		MAPI_CMD_GOP_GWIN_SET_PROPERTY = 0x200,
		MAPI_CMD_GOP_GWIN_GET_PROPERTY,
		//MAPI_CMD_GOP_GWIN_ENABLE,
		MAPI_CMD_GOP_GWIN_SETDISPLAY,

		//FB Info
		MAPI_CMD_GOP_FB_CREATE = 0x500,
		MAPI_CMD_GOP_FB_DESTROY,
		MAPI_CMD_GOP_FB_GET_PROPERTY,

		//MISC
		MAPI_CMD_GOP_REGISTER_CB = 0xC00,

		//POWERSTATE
		MAPI_CMD_GOP_POWERSTATE = 0xD00,
		//MAPI_CMD_GOP_LEGACY_FUNC,
		MAPI_CMD_GOP_INTERRUPT,
	} E_GOP_API_CMD_TYPE;


	typedef enum {
		E_GOP_STRETCH_HSTRETCH_MODE = 0x8,
		E_GOP_STRETCH_VSTRETCH_MODE = 0x10,
	} EN_GOP_STRETCH_TYPE;

	typedef enum {
		E_GOP_ALPHAINVERSE,
		E_GOP_PREALPHAMODE,
		E_GOP_FORCE_WRITE,
	} EN_GOP_PROPERTY;

	typedef enum {
		E_GOP_GWIN_ENABLE,
		E_GOP_GWIN_BLENDING,
		E_GOP_GWIN_SHARE,
		E_GOP_GWIN_SHARE_CNT,
	} EN_GOP_GWIN_PROPERTY;

	typedef enum {
		E_GOP_FB_OBTAIN,
	} EN_GOP_FB_PROPERTY;

	typedef enum {
		/// DWIN capture type -- default case
		GOP_API_VER,
	} EN_GOP_API_TYPE;

	typedef enum {
		GOP_CREATE_BUFFER_BYADDR	//By static addr
	} EN_GOP_CREATEBUFFER_TYPE;

	typedef enum {
		E_GOP_MIRROR_NONE,
		E_GOP_MIRROR_H_ONLY,
		E_GOP_MIRROR_V_ONLY,
		E_GOP_MIRROR_HV,
		E_GOP_MIRROR_H_NONE,
		E_GOP_MIRROR_V_NONE,
	} EN_GOP_MIRROR_TYPE;

	typedef enum {
		E_GOP_STATUS_GOP_MAXNUM,
		E_GOP_STATUS_GWINIDX
	} EN_GOP_STATUS;

//-------------------------------------------------------------------------------------------------
//  Struct
//-------------------------------------------------------------------------------------------------

///the GOP MIXER2OP timing info
	typedef struct DLL_PACKED {
		MS_U8 u8GOP;
		MS_BOOL bEnable;
	} GOP_MixerOldBlendingMode;

	typedef struct DLL_PACKED GOP_RECT_T {
		MS_U32 x;
		MS_U32 y;
		MS_U32 w;
		MS_U32 h;
	} GOP_RECT;

	typedef struct DLL_PACKED {
		MS_U32 gwin;
		MS_U32 fbid;
		GOP_GwinInfo gwin_info;
		GOP_StretchInfo stretch_info;
		EN_GOP_STRETCH_DIRECTION dir;
		GOP_RECT dst_size;
	} GOP_GWINDISPLAY_INFO, *PGOP_GWINDISPLAY_INFO;

/// Capture ring buffer info
	typedef struct DLL_PACKED _GOP_RINGBUFF_INFO {
		MS_U16 Gfx_RingBuffCnt;
		MS_U16 Gfx_CurrentIdx;
		MS_PHY Gfx_BufAddr[0x20];
	} GOP_RINGBUFF_INFO, *PGOP_RINGBUFF_INFO;

////////////////////////////
/// GOP init
////////////////////////////

	typedef struct DLL_PACKED _GOP_INIT_PARAM {
		MS_U32 gop_idx;	//specify gop init, if gop_idx == max gop , init all gop.
		MS_U32 *pInfo;	//GOP_InitInfo
		MS_U32 u32Size;
	} GOP_INIT_PARAM, *PGOP_INIT_PARAM;


////////////////////////////
/// GOP caps
////////////////////////////

	typedef struct DLL_PACKED _GOP_GETCAPS_PARAM {
		MS_U32 caps;	//EN_GOP_CAPS
		void *pInfo;	//Return Caps info
		MS_U32 u32Size;	//Corresponding to the require caps structure size
	} GOP_GETCAPS_PARAM, *PGOP_GETCAPS_PARAM;

	typedef struct DLL_PACKED _GOP_BUFFER_INFO {
		MS_PHY addr;
		GOP_RECT disp_rect;
		MS_U32 width;
		MS_U32 height;
		MS_U32 pitch;
		MS_U32 fbFmt;
		EN_GOP_FRAMEBUFFER_STRING FBString;
	} GOP_BUFFER_INFO, *PGOP_BUFFER_INFO;

	typedef struct DLL_PACKED _GOP_SETCONFIG_PARAM {
		EN_GOP_CONFIG_TYPE cfg_type;	//EN_GOP_CONFIG_TYPE
		void *pCfg;	//Return Caps info
		MS_U32 u32Size;	//Corresponding to the require caps structure size
	} GOP_SETCONFIG_PARAM, *PGOP_SETCONFIG_PARAM;

////////////////////////////
/// GOP Display
////////////////////////////

	typedef struct DLL_PACKED _GOP_GWIN_DISPLAY_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 GwinId;	//specify gwin.
		MS_U32 *pDisplayInfo;	//Gwin display info  - GOP_GWINDISPLAY_INFO
		MS_U32 u32Size;	//input param size - for error check.
	} GOP_GWIN_DISPLAY_PARAM, *PGOP_GWIN_DISPLAY_PARAM;

	typedef struct DLL_PACKED _GOP_GWIN_BLENDING {
		MS_U32 Coef;
		MS_BOOL bEn;
	} GOP_GWIN_BLENDING, *PGOP_GWIN_BLENDING;

	typedef struct DLL_PACKED _GOP_GWIN_3D_MODE {
		MS_U32 u32MainFBId;
		MS_U32 u32SubFBId;
		EN_GOP_3D_MODETYPE en3DMode;
	} GOP_GWIN_3D_MODE, *PGOP_GWIN_3D_MODE;

	typedef struct DLL_PACKED _GOP_GWIN_PROPERTY_PARAM {
		MS_U32 type;	//API VER TYPE
		EN_GOP_GWIN_PROPERTY en_property;
		MS_U32 GwinId;	//specify gwin.
		MS_U32 *pSet;	//reserved - for further use.
		MS_U32 u32Size;	//input param size - for error check.
	} GOP_GWIN_PROPERTY_PARAM, *PGOP_GWIN_PROPERTY_PARAM;

	typedef struct DLL_PACKED _GOP_FB_PROPERTY_PARAM {
		MS_U32 type;	//API VER TYPE
		EN_GOP_FB_PROPERTY en_property;
		MS_U32 FBId;	//specify gwin.
		MS_U32 *pSet;	//reserved - for further use.
		MS_U32 u32Size;	//input param size - for error check.
	} GOP_FB_PROPERTY_PARAM, *PGOP_FB_PROPERTY_PARAM;

	typedef struct DLL_PACKED _GOP_GWIN_SETWININFO_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 GwinId;	//specify gwin.
		MS_U32 *pinfo;	//Gwin mapping frame buffer info  - GOP_BUFFER_INFO
		MS_U32 u32Size;	//input param size - for error check.
	} GOP_GWIN_SETWININFO_PARAM, *PGOP_GWIN_SETWININFO_PARAM;

	typedef struct DLL_PACKED _GOP_GWIN_WIN_ENABLE_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 GwinId;	//specify gwin.
		MS_U32 *pEn;	//Gwin mapping frame buffer info  - GOP_BUFFER_INFO
		MS_U32 u32Size;	//input param size - for error check.
	} GOP_GWIN_WIN_ENABLE_PARAM, *PGOP_GWIN_WIN_ENABLE_PARAM;

	typedef struct DLL_PACKED _GOP_GWIN_DESTROY_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 GwinId;	//specify gwin.
		MS_U32 *ptr;	//reserved
		MS_U32 u32Size;	//reserved - input param size - for error check.
	} GOP_GWIN_DESTROY_PARAM, *PGOP_GWIN_DESTROY_PARAM;

////////////////////////////
/// GOP Frame Buffer
////////////////////////////

	typedef struct DLL_PACKED _GOP_CREATE_BUFFER_PARAM {
		MS_U32 type;	//API VER TYPE
		EN_GOP_CREATEBUFFER_TYPE fb_type;
		MS_U32 fbid;
		MS_U32 *pBufInfo;
		MS_U32 u32Size;
	} GOP_CREATE_BUFFER_PARAM, *PGOP_CREATE_BUFFER_PARAM;

	typedef struct DLL_PACKED _GOP_DELETE_BUFFER_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 *pBufId;
		MS_U32 u32Size;
	} GOP_DELETE_BUFFER_PARAM, *PGOP_DELETE_BUFFER_PARAM;

	typedef struct DLL_PACKED _GOP_FB_INFO_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 fbid;
		MS_U32 *pBufInfo;
		MS_U32 u32Size;
	} GOP_FB_INFO_PARAM, *PGOP_FB_INFO_PARAM;


	typedef struct DLL_PACKED _GOP_CONTRAST {
		MS_U32 y;	//R - if rgb format
		MS_U32 u;	//G - if rgb format
		MS_U32 v;	//B - if rgb format
	} GOP_CONTRAST, *PGOP_CONTRAST;

	typedef struct DLL_PACKED _GOP_SET_PROPERTY_PARAM {
		MS_U32 type;	//API VER TYPE
		EN_GOP_PROPERTY en_pro;
		MS_U32 gop_idx;
		void *pSetting;
		MS_U32 u32Size;	//Size check for different input structure.
	} GOP_SET_PROPERTY_PARAM, *PGOP_SET_PROPERTY_PARAM;


	typedef struct DLL_PACKED _GOP_SETDST_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 gop_idx;
		EN_GOP_DST_TYPE en_dst;
		MS_U32 *pDst;	//Reserved for further usage
		MS_U32 u32Size;
	} GOP_SETDST_PARAM, *PGOP_SETDST_PARAM;

	typedef struct DLL_PACKED _GOP_GETDST_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 gop_idx;
		MS_U32 *pDst;	//Reserved for further usage
		MS_U32 u32Size;
	} GOP_GETDST_PARAM, *PGOP_GETDST_PARAM;

///the GOP and Layer setting info for utopia2.0
	typedef struct DLL_PACKED _GOP_SETLayer {
		MS_U32 u32LayerCount;
		MS_U32 u32Gop[0xF];
		MS_U32 u32Layer[0xF];
	} GOP_SETLayer, *PGOP_SETLayer;

	typedef struct DLL_PACKED _GOP_SETLAYER_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 *pLayerInfo;	//GOP_SETLayer
		MS_U32 u32Size;
	} GOP_SETLAYER_PARAM, *PGOP_SETLAYER_PARAM;

	typedef struct DLL_PACKED _GOP_SETMIRROR_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 gop_idx;
		EN_GOP_MIRROR_TYPE dir;	//Mirror type
		MS_U32 u32Size;
	} GOP_SETMIRROR_PARAM, *PGOP_SETMIRROR_PARAM;

	typedef struct DLL_PACKED _GOP_GETMIRROR_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 gop_idx;
		EN_GOP_MIRROR_TYPE *pdir;	//Mirror type
		MS_U32 u32Size;
	} GOP_GETMIRROR_PARAM, *PGOP_GETMIRROR_PARAM;

	typedef struct DLL_PACKED _GOP_INIT_STATUS {
		MS_U32 gop_idx;	//In
		MS_BOOL bInit;	//gop_idx has init or not.
	} GOP_INIT_STATUS, *PGOP_INIT_STATUS;

	typedef struct DLL_PACKED _GOP_GWIN_NUM {
		MS_U32 gop_idx;	//In
		MS_U32 gwin_num;	//gop_idx has init or not.
	} GOP_GWIN_NUM, *PGOP_GWIN_NUM;


	typedef struct DLL_PACKED _GOP_GET_STATUS_PARAM {
		MS_U32 type;	//API VER TYPE
		EN_GOP_STATUS en_status;
		MS_U32 *pStatus;	//Out - return value for input query
		MS_U32 u32Size;
	} GOP_GET_STATUS_PARAM, *PGOP_GET_STATUS_PARAM;

////////////////////////////
/// GOP Test Pattern
////////////////////////////
	typedef struct DLL_PACKED _GOP_TEST_PATTERN_PARAM {
		MS_U32 gop_idx;
		EN_GOP_TST_PATTERN TestPatternType;
		MS_U32 u32ARGB;
		MS_U32 u32Size;
	} GOP_TEST_PATTERN_PARAM, *PGOP_TEST_PATTERN_PARAM;

	typedef struct DLL_PACKED {
		MS_U32 u32GOP_idx;
		MS_U32 u32HSize;
		MS_U32 u32VSize;
	} GOP_DeleteWinSize_PARAM;

////////////////////////////
/// GOP Stretch
////////////////////////////

	typedef struct DLL_PACKED _GOP_STRETCH_INFO {
		GOP_RECT SrcRect;
		GOP_RECT DstRect;
		EN_GOP_STRETCH_HMODE enHMode;
		EN_GOP_STRETCH_VMODE enVMode;
		EN_GOP_STRCH_TRANSPCOLORMODE enTColorMode;
	} GOP_STRETCH_INFO, *PGOP_STRETCH_INFO;

	typedef struct DLL_PACKED _GOP_STRETCH_SET_PARAM {
		MS_U32 type;
		EN_GOP_STRETCH_TYPE enStrtchType;
		MS_U32 gop_idx;
		MS_U32 *pStretch;	//stretch win info  - PGOP_STRETCH_INFO
		MS_U32 u32Size;	// input structure size
	} GOP_STRETCH_SET_PARAM, *PGOP_STRETCH_SET_PARAM;

////////////////////////////
/// GFlip Clear Queue
////////////////////////////
	typedef struct DLL_PACKED _GOP_GWIN_CLEARQUEUE_INFO {
		MS_U32 GwinId;	//specify gwin.
//    MS_U32* pInfo;                       //Gwin display info  - GOP_GWINDISPLAY_INFO
	} GOP_GWIN_CLEARQUEUE_INFO, *PGOP_GWIN_CLEARQUEUE_INFO;


	typedef struct DLL_PACKED _GOP_GWIN_CLEARQUEUE_PARAM {
		MS_U32 type;	//API VER TYPE
		MS_U32 *pClearInfo;	//GOP_GWIN_CLEARQUEUE_INFO
		MS_U32 u32Size;	//input param size - for error check.
	} GOP_GWIN_CLEARQUEUE_PARAM, *PGOP_GWIN_CLEARQUEUE_PARAM;

	typedef struct DLL_PACKED _GOP_GWIN_GFLIP_SWITCH_MULTI_GWIN_PARAM {
		MS_U32 type;	//API VER TYPE
		void *pMultiFlipInfo;	//GOP_GWIN_CLEARQUEUE_INFO
		MS_U32 u32Size;	//input param size - for error check.
	} GOP_GWIN_GFLIP_SWITCH_MULTI_GWIN_PARAM, *PGOP_GWIN_GFLIP_SWITCH_MULTI_GWIN_PARAM;

	typedef struct DLL_PACKED {
		MS_U32 type;	//API VER TYPE
		void *pInfo;
		MS_U32 u32Size;	//reserved for error check size
	} GOP_POWERSTATE_PARAM, *PGOP_POWERSTATE_PARAM;

	typedef struct {
		MS_U8 u8GOPNum;
		MS_U8 pGwinIdx;
	} ST_GOP_Get_GwinIdx;


	typedef struct DLL_PACKED {
		MS_U32 type;	//API VER TYPE
		MS_U32 gop_idx;
		MS_U32 *pSetting;
		MS_U32 u32Size;	//Size check for different input structure.
	} GOP_INTERRUPT_PARAM, *PGOP_INTERRUPT_PARAM;
//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
//======================================================================================
//  GOP Common utility
//======================================================================================
MS_U32 Ioctl_GOP_Init(void *pInstance, MS_U8 u8GOP, GOP_InitInfo *pGopInit);
MS_U32 Ioctl_GOP_GetCaps(void *pInstance, EN_GOP_CAPS eCapType, MS_U32 *pRet,
				 MS_U32 ret_size);
MS_U32 Ioctl_GOP_SetConfig(void *pInstance, EN_GOP_CONFIG_TYPE cfg_type, MS_U32 *pCfg,
				   MS_U32 u32Size);
MS_U32 Ioctl_GOP_SetProperty(void *pInstance, EN_GOP_PROPERTY type, MS_U32 gop,
				     MS_U32 *pSet, MS_U32 u32Size);
MS_U32 Ioctl_GOP_SetDst(void *pInstance, MS_U8 u8GOP, EN_GOP_DST_TYPE dsttype);
MS_U32 Ioctl_GOP_SetMirror(void *pInstance, MS_U32 gop, EN_GOP_MIRROR_TYPE mirror);
MS_U32 Ioctl_GOP_GetStatus(void *pInstance, EN_GOP_STATUS type, MS_U32 *pStatus,
				   MS_U32 u32Size);
MS_U32 Ioctl_GOP_TestPattern(void *pInstance, EN_GOP_TST_PATTERN eTstPatternType,
				     MS_U32 u32ARGB);
MS_U32 Ioctl_GOP_Set_Stretch(void *pInstance, EN_GOP_STRETCH_TYPE enStretchType, MS_U32 gop,
				     PGOP_STRETCH_INFO pStretchInfo);
MS_U32 Ioctl_GOP_GWin_SetProperty(void *pInstance, EN_GOP_GWIN_PROPERTY type, MS_U32 gwin,
					  MS_U32 *pSet, MS_U32 u32Size);
MS_U32 Ioctl_GOP_GWin_GetProperty(void *pInstance, EN_GOP_GWIN_PROPERTY type, MS_U32 gwin,
					  MS_U32 *pSet, MS_U32 u32Size);
MS_U32 Ioctl_GOP_FB_GetProperty(void *pInstance, EN_GOP_FB_PROPERTY type, MS_U32 FBId,
					MS_U32 *pSet, MS_U32 u32Size);
MS_U32 Ioctl_GOP_SetDisplay(void *pInstance, PGOP_GWINDISPLAY_INFO pInfo);
MS_U32 Ioctl_GOP_FBCreate(void *pInstance, EN_GOP_CREATEBUFFER_TYPE fbtype,
				  PGOP_BUFFER_INFO pBuff, MS_U32 fbId);
MS_U32 Ioctl_GOP_FBDestroy(void *pInstance, MS_U32 *pId);
MS_U32 Ioctl_GOP_PowerState(void *pInstance, MS_U32 u32PowerState, void *pModule);
MS_U32 Ioctl_GOP_SetLayer(void *pInstance, PGOP_SETLayer pstLayer, MS_U32 u32Size);
int Ioctl_GOP_Interrupt(void *pInstance, MS_U8 u8Gop, MS_BOOL bEnable);

MS_U32 Ioctl_GOP_SetTestPattern(ST_GOP_TESTPATTERN *pstGOPTestPatInfo);
MS_U32 Ioctl_GOP_GetCRC(MS_U32 *CRCvalue);
MS_U32 Ioctl_GOP_PrintInfo(MS_U8 GOPNum);
MS_U32 Ioctl_GOP_SetGOPBypassPath(void *pInstance, MS_U8 u8GOP, MS_BOOL bEnable);
MS_U32 Ioctl_GOP_SetTGen(void *pInstance, ST_GOP_TGEN_INFO *pGOPTgenInfo);
MS_U32 GOP_SetTGenVGSync(MS_BOOL bEnable);
MS_U32 GOP_SetMixerOutMode(void *pInstance, MS_BOOL Mixer1_OutPreAlpha, MS_BOOL Mixer2_OutPreAlpha);
MS_U32 GOP_SetPQCtx(void *pInstance, MS_U8 u8GOP, ST_GOP_PQCTX_INFO *pGopPQCtx);
MS_U32 GOP_SetAdlnfo(void *pInstance, MS_U8 u8GOP, ST_GOP_ADL_INFO *pGopAdlInfo);
MS_U32 GOP_SetPqMlnfo(void *pInstance, MS_U8 u8GOP, ST_GOP_PQ_CFD_ML_INFO *pGopMlInfo);
MS_U32 GOP_TriggerRegWriteIn(void *pInstance, MS_U32 GOP, MS_BOOL bEn, MS_BOOL bMl, MS_U8 MlResIdx);
MS_U32 GOP_SetAidType(MS_U8 u8GOP, MS_U64 u64GopAidType);
MS_U32 GOP_SetVGOrder(EN_GOP_VG_ORDER eVGOrder);
MS_U32 GOP_ml_fire(void *pInstance, MS_U8 u8MlResIdx);
MS_U32 GOP_ml_get_mem_info(void *pInstance, MS_U8 u8MlResIdx);
MS_U32 GOP_GetRoi(ST_GOP_GETROI *pGOPRoi);

#ifdef __cplusplus
}
#endif
#endif
