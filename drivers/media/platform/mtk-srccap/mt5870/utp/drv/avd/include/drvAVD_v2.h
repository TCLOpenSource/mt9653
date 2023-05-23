/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "drvAVD.h"

#ifndef _DRV_AVD_V2_H_
#define _DRV_AVD_V2_H_

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------

#ifdef MS_DEBUG
#define AVD_DRV_DEBUG 1
#else
#define AVD_DRV_DEBUG 0
#endif

// collect global variable into struct AVD_RESOURCE_PRIVATE 201303 weicheng
struct DLL_PACKED AVD_RESOURCE_PRIVATE
{
	MS_BOOL _bShiftClkFlag;
	MS_BOOL _b2d3dautoflag;
	MS_BOOL _bSTRFlag;
	MS_U8  _u8HtotalDebounce;
	MS_U8  _u8AutoDetMode;
	MS_U8 _u8AfecD4Factory;
	MS_U8 _u8Comb10Bit3Flag;
	MS_U8 _u8Comb57;
	MS_U8 _u8Comb58;
	MS_U8 _u8Comb5F;
	MS_U8 _u8SCARTSwitch;  // 0: CVBS, 1:Svideo;
	MS_U8 _u8SCARTPrestandard;
	MS_U8 _u8AutoTuningIsProgress;

	MS_U16 _u16CurVDStatus;
	MS_U16 _u16DataH[3];
	MS_U16 _u16LatchH;
	MS_U16 _u16Htt_UserMD;
	MS_U16 _u16DPL_MSB;
	MS_U16 _u16DPL_LSB;
	MS_U32 u32VDPatchFlagStatus;
	MS_U32 _u32VideoSystemTimer;
	MS_U32 _u32SCARTWaitTime;
	MS_S32 _s32AVD_Mutex;

	enum AVD_VideoStandardType _eVideoSystem;
	enum AVD_VideoStandardType _eForceVideoStandard;
	enum AVD_VideoStandardType _eLastStandard;
	enum AVD_InputSourceType _eVDInputSource;
	struct VD_INITDATA g_VD_InitData;
	enum AVD_ATV_CLK_TYPE gShiftMode;
	struct AVD_Still_Image_Param g_stillImageParam;

	#if AVD_DRV_DEBUG
	MS_U32 u32PreVDPatchFlagStatus;
	enum AVD_DbgLv _u8AVDDbgLevel;
	#endif
	#ifndef MSOS_TYPE_LINUX
	MS_U16 u16PreVtotal;
	#endif

};

enum eAvdIoctlOpt {
	MDrv_CMD_AVD_Init,
	MDrv_CMD_AVD_Exit,
	MDrv_CMD_AVD_ResetMCU,
	MDrv_CMD_AVD_FreezeMCU,
	MDrv_CMD_AVD_Scan_HsyncCheck,
	MDrv_CMD_AVD_StartAutoStandardDetection,
	MDrv_CMD_AVD_ForceVideoStandard,
	MDrv_CMD_AVD_3DCombSpeedup,
	MDrv_CMD_AVD_LoadDSP,
	MDrv_CMD_AVD_BackPorchWindowPositon,
	MDrv_CMD_AVD_MBX_ReadByteByVDMbox,

	MDrv_CMD_AVD_SetFlag,
	MDrv_CMD_AVD_SetRegValue,
	MDrv_CMD_AVD_SetRegFromDSP,
	MDrv_CMD_AVD_SetInput,
	MDrv_CMD_AVD_SetVideoStandard,
	MDrv_CMD_AVD_SetChannelChange,
	MDrv_CMD_AVD_SetHsyncDetectionForTuning,
	MDrv_CMD_AVD_Set3dComb,
	MDrv_CMD_AVD_SetShiftClk,
	MDrv_CMD_AVD_SetFreerunPLL,
	MDrv_CMD_AVD_SetFreerunFreq,
	MDrv_CMD_AVD_SetFactoryPara,
	MDrv_CMD_AVD_Set_Htt_UserMD,
	MDrv_CMD_AVD_SetDbgLevel,
	MDrv_CMD_AVD_SetPQFineTune,
	MDrv_CMD_AVD_Set3dCombSpeed,
	MDrv_CMD_AVD_SetStillImageParam,
	MDrv_CMD_AVD_SetAFECD4Factory,
	MDrv_CMD_AVD_Set2D3DPatchOnOff,
	MDrv_CMD_AVD_SetAutoFineGainToFixed,

	MDrv_CMD_AVD_GetFlag,
	MDrv_CMD_AVD_GetRegValue,
	MDrv_CMD_AVD_GetStatus,
	MDrv_CMD_AVD_GetNoiseMag,
	MDrv_CMD_AVD_GetVTotal,
	MDrv_CMD_AVD_GetStandardDetection,
	MDrv_CMD_AVD_GetLastDetectedStandard,
	MDrv_CMD_AVD_GetCaptureWindow,
	MDrv_CMD_AVD_GetVerticalFreq,
	MDrv_CMD_AVD_GetHsyncEdge,
	MDrv_CMD_AVD_GetDSPFineGain,
	MDrv_CMD_AVD_GetDSPVersion,
	MDrv_CMD_AVD_GetLibVer,
	MDrv_CMD_AVD_GetInfo,
	MDrv_CMD_AVD_IsSyncLocked,
	MDrv_CMD_AVD_IsSignalInterlaced,
	MDrv_CMD_AVD_IsColorOn,
	MDrv_CMD_AVD_SetPowerState,
	MDrv_CMD_AVD_GetMacroVisionDetect,
	MDrv_CMD_AVD_GetCGMSDetect,
	MDrv_CMD_AVD_SetBurstWinStart,
	MDrv_CMD_AVD_AliveCheck,
	MDrv_CMD_AVD_IsLockAudioCarrier,
	MDrv_CMD_AVD_SetAnaDemod,
	MDrv_CMD_AVD_SetHsyncSensitivityForDebug,
};

struct DLL_PACKED AVD_INIT
{
	struct VD_INITDATA pVD_InitData;
	MS_U32 u32InitDataLen;
	enum AVD_Result pVD_Result;
};

struct DLL_PACKED AVD_LOADDSP
{
	MS_U8 *pu8VD_DSP;
	MS_U32 len;
};

struct DLL_PACKED AVD_BACKPORCHWINPOS
{
	MS_BOOL bEnable;
	MS_U8 u8Value;
};

struct DLL_PACKED AVD_SETREGVALUE
{
	MS_U16 u16Addr;
	MS_U8 u8Value;
};

struct DLL_PACKED AVD_SETINPUT
{
	enum AVD_InputSourceType eSource;
	MS_U8 u8ScartFB;
	MS_BOOL bEnable;
};

struct DLL_PACKED AVD_SETVIDEOSTANDARD
{
	enum AVD_VideoStandardType eStandard;
	MS_BOOL bIsInAutoTuning;
	MS_BOOL bEnable;
};

struct DLL_PACKED AVD_SETSHIFTCLK
{
	MS_BOOL bEnable;
	enum AVD_ATV_CLK_TYPE eShiftMode;
};

struct DLL_PACKED AVD_SETFACTORYPARA
{
	enum AVD_Factory_Para FactoryPara;
	MS_U8 u8Value;
};

struct DLL_PACKED AVD_SET3DCOMBSPEED
{
	MS_U8 u8COMB57;
	MS_U8 u8COMB58;
};

struct DLL_PACKED AVD_GETCAPTUREWINDOW
{
	void *stCapWin;
	enum AVD_VideoStandardType eVideoStandardType;
};

struct DLL_PACKED AVD_GETSTANDARDDETECTION
{
	enum AVD_VideoStandardType VideoStandardType;
	MS_U16 vdLatchStatus;
};

struct DLL_PACKED AVD_SCANHSYNCCHECK
{
	MS_U8 u8HtotalTolerance;
	MS_U16 u16ScanHsyncCheck;
};

struct DLL_PACKED AVD_FORCEVIDEOSTANDARD
{
	enum AVD_VideoStandardType eVideoStandardType;
	MS_BOOL bEnable;
};

struct DLL_PACKED AVD_MBXREADBYTEBYVDMBOX
{
	MS_U8 u8Addr;
	MS_U8 u8Value;
};

struct DLL_PACKED AVD_SETDBGLEVEL
{
	enum AVD_DbgLv u8DbgLevel;
	MS_BOOL bEnable;
};

struct DLL_PACKED AVD_GETREGVALUE
{
	MS_U16 u16Addr;
	MS_U8 u8Value;
};

struct DLL_PACKED AVD_SETPOWERSTATE
{
	EN_POWER_MODE u16PowerState;
	MS_U32 u32Value;
};
#if !defined STI_PLATFORM_BRING_UP

struct DLL_PACKED AVD_GETLIBVER
{
	const MSIF_Version **ppVersion;
	enum AVD_Result eAVDResult;
};
#endif
struct DLL_PACKED AVD_COPYTOUSER
{
	MS_BOOL bEnable;
	MS_U8 u8Value;
	MS_U16 u16Value;
	MS_U32 u32Value;
	enum AVD_VideoStandardType eVideoStandardType;
	enum AVD_VideoFreq eVideoFreq;
	struct AVD_Info eAVDInfo;
};

struct AVD_SETHSYNCSENSITIVITYFORDEBUG {
	MS_U8 u8AFEC99;
	MS_U8 u8AFEC9C;
};
#endif // _DRV_AVD_V2_H_
