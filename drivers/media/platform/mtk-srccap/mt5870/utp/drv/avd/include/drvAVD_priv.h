/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRV_AVD_PRIV_H_
#define _DRV_AVD_PRIV_H_

#include "drvAVD_v2.h"

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------

#ifdef CONFIG_UTOPIA_PROC_DBG_SUPPORT
enum AVD_MDBCMD_RESET {
	MDBCMD_Reset_DSP_CODE = 0x0,
	MDBCMD_Reset_AFEC_HW = 0x1,
	MDBCMD_Reset_COMB = 0x2,
};

enum AVD_MDBCMD_PATTERN {
	MDBCMD_PATTERN_CANCEL = 0x0,
	MDBCMD_PATTERN_ROW = 0x1,
	MDBCMD_PATTERN_COLUMN = 0x2,
};

enum AVD_MDBCMD_FORCE_STANDARD {
	MDBCMD_FORCE_CANCEL = 0x0,
	MDBCMD_FORCE_PAL = 0x1,
	MDBCMD_FORCE_SECAM = 0x2,
	MDBCMD_FORCE_NTSC = 0x3,
	MDBCMD_FORCE_NTSC443 = 0x4,
	MDBCMD_FORCE_PAL_M = 0x5,
	MDBCMD_FORCE_PAL_60 = 0x6,
	MDBCMD_FORCE_PAL_NC = 0x7,
};

enum AVD_MDBCMD_SLICE_LEVEL {
	MDBCMD_LEVEL_CANCEL = 0x0,
	MDBCMD_LEVEL_44 = 0x1,
	MDBCMD_LEVEL_88 = 0x2,
	MDBCMD_LEVEL_CC = 0x3,
	MDBCMD_LEVEL_FF = 0x4,
};

enum AVD_MDBCMD_HSYNC_PLL {
	MDBCMD_PLL_CANCEL = 0x0,
	MDBCMD_PLL_Level1 = 0x1, // 0x82/0x04
	MDBCMD_PLL_Level2 = 0x2, //0x90/0x20
	MDBCMD_PLL_Level3 = 0x3, //0x9A/0x35
	MDBCMD_PLL_Level4 = 0x4, //0xAA/0x6A
	MDBCMD_PLL_Level5 = 0x5, //0xBC/0x6A
};

enum AVD_MDBCMD_COMB {
	MDBCMD_COMB_CANCEL = 0x0,
	MDBCMD_COMB_ByPassComb_Filter = 0x1,
};

enum AVD_MDBCMD_BurstWindow {
	MDBCMD_Window_Start = 0x1,
	MDBCMD_Window_End = 0x2,
};

enum AVD_MDBCMD_FixGain {
	MDBCMD_FixGain_CANCEL = 0x0,
	MDBCMD_FixGain_ENABLE = 0x1,
};

/// AVD status indicator
enum AVD_MDBCMD_STATUS {
	MDBCMD_STATUS_OK,             //AVD status OK
	MDBCMD_STATUS_ERROR,          // AVD status ERROR
	MDBCMD_NOT_SUPPORT,           //AVD status NOT SUPPORT
};
#endif

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

extern enum AVD_Result Drv_AVD_Init(struct VD_INITDATA *pVD_InitData, MS_U32 u32InitDataLen,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_Exit(struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_BOOL Drv_AVD_ResetMCU(struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_FreezeMCU(MS_BOOL bEnable, struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_U16 Drv_AVD_Scan_HsyncCheck(MS_U8 u8HtotalTolerance,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_StartAutoStandardDetection(struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_BOOL Drv_AVD_ForceVideoStandard(enum AVD_VideoStandardType eVideoStandardType,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_3DCombSpeedup(struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_LoadDSP(MS_U8 *pu8VD_DSP, MS_U32 len, struct AVD_RESOURCE_PRIVATE *pResource);
#if !defined STI_PLATFORM_BRING_UP
extern MS_U8 Drv_AVD_MBX_ReadByteByVDMbox(MS_U8 u8Addr, struct AVD_RESOURCE_PRIVATE *pResource);
#endif

extern void Drv_AVD_BackPorchWindowPositon(MS_BOOL bEnable, MS_U8 u8Value);


extern void Drv_AVD_SetFlag(MS_U32  u32VDPatchFlag, struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetStandard_Detection_Flag(MS_U8 u8Value);

extern void Drv_AVD_SetRegFromDSP(struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_BOOL Drv_AVD_SetInput(enum AVD_InputSourceType eSource, MS_U8 u8ScartFB,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_BOOL Drv_AVD_SetVideoStandard(enum AVD_VideoStandardType eStandard,
		MS_BOOL bIsInAutoTuning, struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetChannelChange(struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetHsyncDetectionForTuning(MS_BOOL bEnable,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_Set3dComb(MS_BOOL bEnable,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetShiftClk(MS_BOOL bEnable, enum AVD_ATV_CLK_TYPE eShiftMode,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetFreerunPLL(enum AVD_VideoFreq eVideoFreq,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetFreerunFreq(enum AVD_FreeRunFreq eFreerunfreq,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetFactoryPara(enum AVD_Factory_Para FactoryPara, MS_U8 u8Value,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_BOOL Drv_AVD_SetDbgLevel(enum AVD_DbgLv u8DbgLevel,
	struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetPQFineTune(struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_Set3dCombSpeed(MS_U8 u8COMB57, MS_U8 u8COMB58,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetStillImageParam(struct AVD_Still_Image_Param param,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_Set2D3DPatchOnOff(MS_BOOL bEnable);

extern MS_U8 Drv_AVD_SetAutoFineGainToFixed(void);


extern MS_U8 Drv_AVD_GetStandard_Detection_Flag(void);

extern MS_U16 Drv_AVD_GetStatus(void);

extern MS_U8 Drv_AVD_GetNoiseMag(void);

extern MS_U16 Drv_AVD_GetVTotal(void);

extern enum AVD_VideoStandardType Drv_AVD_GetStandardDetection(MS_U16 *u16LatchStatus,
		struct AVD_RESOURCE_PRIVATE *pResource);
#if !defined STI_PLATFORM_BRING_UP
extern void Drv_AVD_GetCaptureWindow(void *stCapWin,
		enum AVD_VideoStandardType eVideoStandardType,
		struct AVD_RESOURCE_PRIVATE *pResource);
#endif

extern enum AVD_VideoFreq Drv_AVD_GetVerticalFreq(void);

extern MS_U8 Drv_AVD_GetHsyncEdge(void);

extern MS_U8 Drv_AVD_GetDSPFineGain(struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_U16 Drv_AVD_GetDSPVersion(void);
#if !defined STI_PLATFORM_BRING_UP

extern enum AVD_Result Drv_AVD_GetLibVer(const MSIF_Version **ppVersion);

#endif
extern void  Drv_AVD_GetInfo(struct AVD_Info *pAVD_Info, struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_BOOL Drv_AVD_IsSyncLocked(void);

extern MS_BOOL Drv_AVD_IsSignalInterlaced(void);

extern MS_BOOL Drv_AVD_IsColorOn(void);

extern MS_U32 Drv_AVD_SetPowerState(EN_POWER_MODE u16PowerState,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_BOOL Drv_AVD_GetMacroVisionDetect(void);

extern MS_BOOL Drv_AVD_GetCGMSDetect(void);

extern void Drv_AVD_SetBurstWinStart(MS_U8 u8BusrtStartPosition);

extern MS_BOOL Drv_AVD_IsAVDAlive(void);

extern MS_BOOL Drv_AVD_IsLockAudioCarrier(void);

extern MS_BOOL Drv_AVD_DBG_EchoCmd(MS_U64 *pu64ReqHdl, MS_U32 u32CmdSize, char *pcmd);

extern MS_BOOL Drv_AVD_DBG_GetModuleInfo(MS_U64 *pu64ReqHdl);

extern char *Drv_AVD_Str_Tok(char *pstrSrc, char *pstrDes, char delimit);

extern int Drv_AVD_StrToHex(char *pstrSrc);

extern void Drv_AVD_SetAnaDemod(MS_BOOL bEnable, struct AVD_RESOURCE_PRIVATE *pResource);

extern void Drv_AVD_SetHsyncSensitivityForDebug(MS_U8 u8AFEC99, MS_U8 u8AFEC9C,
		struct AVD_RESOURCE_PRIVATE *pResource);

extern MS_BOOL Drv_AVD_SetHWInfo(struct AVD_HW_Info stAVDHWInfo);

MS_U32 AVDStr(MS_U32 u32PowerState, void *pModule);

void vd_str_stablestatus(void);

#ifdef CONFIG_UTOPIA_PROC_DBG_SUPPORT
MS_U32  AVDMdbIoctl(MS_U32 u32Cmd, const void * const pArgs);
#endif

#endif // _DRV_AVD_PRIV_H_
