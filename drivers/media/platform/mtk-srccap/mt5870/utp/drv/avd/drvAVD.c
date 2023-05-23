// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
/// file    drvAVD.c
/// @brief  AVD Driver Interface
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "drvAVD_v2.h"
#include "drvAVD.h"
#include "drvAVD_priv.h"
//#include "../../utopia_core/utopia.h"

//#include "UFO.h"
#ifndef DONT_USE_CMA
//#include "drvCMAPool_v2.h"
//#include "halCHIP.h"
#endif
//#include "ULog.h"

//-------------------------------------------------------------------------------------------------
//  definition
//-------------------------------------------------------------------------------------------------
#define CHECKAVDOPEN()               \
	do {                        \
		if (u32AVDopen == 0) {    \
			pr_err("\033[35m [%s:%d]FAIL : Need AVD init. \033[m\n",    \
			__PRETTY_FUNCTION__, __LINE__);   \
		}             \
	} while (0)

//-------------------------------------------------------------------------------------------------
//  Global variables
//-------------------------------------------------------------------------------------------------

void *ppAVDInstant;
static MS_U32 u32AVDopen;
static MS_U8 u8AVDUtopiaOpen;   //for SetStillImagePara is earlier called than Init
static enum AVD_Result bAVD_Open_result = E_AVD_FAIL;
#ifndef DONT_USE_CMA
#ifdef UFO_AVD_CMA
MS_BOOL bAVDGetMA;    //0: not GetMA yet   1: already Get MA
MS_BOOL bAVDCMAInit;
MS_PHY u32COMB_3D_CMA_addr;
MS_U32 u32COMB_3D_CMA_LEN;
#endif
#endif

struct AVD_RESOURCE_PRIVATE *pAVDResoucePrivate;
//-------------------------------------------------------------------------------------------------
//  Global VD functions
//-------------------------------------------------------------------------------------------------
#ifndef DONT_USE_CMA
#ifdef UFO_AVD_CMA
struct CMA_Pool_Init_Param AVD_CMA_Pool_Init_PARAM;   // for MApi_CMA_Pool_Init
struct CMA_Pool_Alloc_Param AVD_CMA_Alloc_PARAM;    // for MApi_CMA_Pool_GetMem
struct CMA_Pool_Free_Param AVD_CMA_Free_PARAM;      // for MApi_CMA_Pool_PutMem
#endif
#endif

#ifndef UTOPIAXP_REMOVE_WRAPPER
enum AVD_Result Api_AVD_Init(struct VD_INITDATA *pVD_InitData, MS_U32 u32InitDataLen)
{
	struct AVD_INIT _eAVD_Init;

	_eAVD_Init.pVD_InitData = *pVD_InitData;
	_eAVD_Init.u32InitDataLen = u32InitDataLen;
	_eAVD_Init.pVD_Result = E_AVD_FAIL;

#ifndef CONFIG_FPGA
	if (pAVDResoucePrivate == NULL)
		pAVDResoucePrivate = kmalloc(sizeof(struct AVD_RESOURCE_PRIVATE), GFP_KERNEL);

	if (pAVDResoucePrivate != NULL) {
		pAVDResoucePrivate->_bShiftClkFlag = 0;
		pAVDResoucePrivate->_b2d3dautoflag = 1;
		#if defined(MSTAR_STR)
		pAVDResoucePrivate->_bSTRFlag = 0;
		#endif
		pAVDResoucePrivate->_u8HtotalDebounce = 0;
		pAVDResoucePrivate->_u8AutoDetMode = FSC_AUTO_DET_ENABLE;
		pAVDResoucePrivate->_u8AfecD4Factory = 0;
		pAVDResoucePrivate->_u8Comb10Bit3Flag = 1;
		pAVDResoucePrivate->_u8Comb57 = 0x04;
		pAVDResoucePrivate->_u8Comb58 = 0x01;
		pAVDResoucePrivate->_u8Comb5F = 0x08;
		pAVDResoucePrivate->_u8SCARTSwitch = 0;  // 0: CVBS, 1:Svideo;
		pAVDResoucePrivate->_u8SCARTPrestandard = 0;
		pAVDResoucePrivate->_u8AutoTuningIsProgress = 0;

		pAVDResoucePrivate->_u16CurVDStatus = 0;
		pAVDResoucePrivate->_u16DataH[0] = 0;
		pAVDResoucePrivate->_u16DataH[1] = 0;
		pAVDResoucePrivate->_u16DataH[2] = 0;
		pAVDResoucePrivate->_u16LatchH = 1135L;
		pAVDResoucePrivate->_u16Htt_UserMD = 1135;
		pAVDResoucePrivate->_u16DPL_LSB = 0;
		pAVDResoucePrivate->_u16DPL_MSB = 0;

		pAVDResoucePrivate->u32VDPatchFlagStatus = 0;
		pAVDResoucePrivate->_u32VideoSystemTimer = 0;
		pAVDResoucePrivate->_u32SCARTWaitTime = 0;

		pAVDResoucePrivate->_eVideoSystem = E_VIDEOSTANDARD_PAL_BGHI;
		pAVDResoucePrivate->_eForceVideoStandard = E_VIDEOSTANDARD_AUTO;
		pAVDResoucePrivate->_eLastStandard = E_VIDEOSTANDARD_NOTSTANDARD;
		pAVDResoucePrivate->_eVDInputSource = E_INPUT_SOURCE_ATV;
		pAVDResoucePrivate->gShiftMode = E_ATV_CLK_ORIGIN_43P2MHZ;
	}

	if (u8AVDUtopiaOpen == 0) {
		u32AVDopen = 1;
		u8AVDUtopiaOpen = 1;
	}

    #ifndef DONT_USE_CMA
    #ifdef UFO_AVD_CMA
    /* do CMA Init*/
	AVD_CMA_Pool_Init_PARAM.heap_id = _eAVD_Init.pVD_InitData->u32CMA_HEAP_ID;
	AVD_CMA_Pool_Init_PARAM.flags = CMA_FLAG_MAP_VMA;
	if (bAVDCMAInit == 0) {
		if (MApi_CMA_Pool_Init(&AVD_CMA_Pool_Init_PARAM) == TRUE) {
			bAVDCMAInit = 1;
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d,
				get pool_handle_id is %u\033[m\n",
				__PRETTY_FUNCTION__, __LINE__,
				AVD_CMA_Pool_Init_PARAM.pool_handle_id);
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d,
				get miu is %u\033[m\n",
				__PRETTY_FUNCTION__, __LINE__,
				AVD_CMA_Pool_Init_PARAM.miu);
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d,
				get heap_miu_start_offset is 0x%lX\033[m\n",
				__PRETTY_FUNCTION__, __LINE__,
				AVD_CMA_Pool_Init_PARAM.heap_miu_start_offset);
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d,
				get heap_length is 0x%lX\033[m\n",
				__PRETTY_FUNCTION__, __LINE__,
				AVD_CMA_Pool_Init_PARAM.heap_length);
		} else {
			bAVDCMAInit = 0;
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d,
				CMA_POOL_INIT ERROR!!\033[m\n",
				__PRETTY_FUNCTION__, __LINE__);
			return FALSE;
		}
	}
    /*do Get Mem*/
	if ((bAVDCMAInit == 1) && (bAVDGetMA == 0)) {
		//CMA already Init, not get mem yet
		AVD_CMA_Alloc_PARAM.pool_handle_id = AVD_CMA_Pool_Init_PARAM.pool_handle_id;
		AVD_CMA_Alloc_PARAM.offset_in_pool = (_eAVD_Init.pVD_InitData.u32COMB_3D_ADR
			- AVD_CMA_Pool_Init_PARAM.heap_miu_start_offset);
		AVD_CMA_Alloc_PARAM.length = _eAVD_Init.pVD_InitData.u32COMB_3D_LEN;
		AVD_CMA_Alloc_PARAM.flags = CMA_FLAG_VIRT_ADDR;
	}
	#endif
	#endif

	CHECKAVDOPEN();
	_eAVD_Init.pVD_Result = Drv_AVD_Init(&(_eAVD_Init.pVD_InitData),
		_eAVD_Init.u32InitDataLen, pAVDResoucePrivate);
	bAVD_Open_result = _eAVD_Init.pVD_Result;
	#endif
	return _eAVD_Init.pVD_Result;
}

void Api_AVD_Exit(void)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1) {
		Drv_AVD_Exit(pAVDResoucePrivate);
#ifndef DONT_USE_CMA
#ifdef UFO_AVD_CMA
		/* do CMA Put Mem*/
		if ((bAVDCMAInit == 1) && (bAVDGetMA == 1)) {
			if (MApi_CMA_Pool_PutMem(&AVD_CMA_Free_PARAM) == FALSE) {
				ULOGE("AVD", "\033[35mFunction = %s, Line = %d,
					CMA_Pool_PutMem ERROR!!\033[m\n",
					__PRETTY_FUNCTION__, __LINE__);
				bAVDGetMA = 1;
			} else {
				bAVDGetMA = 0;
			}
		}
#endif
#endif
	}
}

MS_BOOL Api_AVD_McuReset(void)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_ResetMCU(pAVDResoucePrivate);
	return ret;
}

void Api_AVD_MCUFreeze(MS_BOOL bEnable)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_FreezeMCU(bEnable, pAVDResoucePrivate);
}

MS_U16 Api_AVD_Scan_HsyncCheck(MS_U8 u8HtotalTolerance)
{
	MS_U16 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_Scan_HsyncCheck(u8HtotalTolerance, pAVDResoucePrivate);
	return ret;
}

void Api_AVD_StartAutoStandardDetection(void)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_StartAutoStandardDetection(pAVDResoucePrivate);
}

MS_BOOL Api_AVD_ForceVideoStandard(enum AVD_VideoStandardType eVideoStandardType)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_ForceVideoStandard(eVideoStandardType, pAVDResoucePrivate);
	return ret;
}

void Api_AVD_3DCombSpeedup(void)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_3DCombSpeedup(pAVDResoucePrivate);
}

void Api_AVD_LoadDSP(MS_U8 *pu8VD_DSP, MS_U32 len)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_LoadDSP(pu8VD_DSP, len, pAVDResoucePrivate);
}

void Api_AVD_BackPorchWindowPositon(MS_BOOL bEnable, MS_U8 u8Value)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_BackPorchWindowPositon(bEnable, u8Value);
}

void Api_AVD_SetFlag(MS_U32  u32VDPatchFlag)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetFlag(u32VDPatchFlag, pAVDResoucePrivate);
}

void Api_AVD_SetStandard_Detection_Flag(MS_U8 u8Value)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetStandard_Detection_Flag(u8Value);
}

void Api_AVD_SetRegFromDSP(void)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetRegFromDSP(pAVDResoucePrivate);
}

MS_BOOL Api_AVD_SetInput(enum AVD_InputSourceType eSource, MS_U8 u8ScartFB)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();

#ifndef DONT_USE_CMA
#ifdef UFO_AVD_CMA
	/*do Get Mem*/
	if ((bAVDCMAInit == 1) && (bAVDGetMA == 0)) {
		//CMA already Init, not get mem yet
		if (MApi_CMA_Pool_GetMem(&AVD_CMA_Alloc_PARAM) == FALSE) {
			bAVDGetMA = 0;
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d,
				CMA_Pool_GetMem ERROR!!\033[m\n",
				__PRETTY_FUNCTION__, __LINE__);
		} else {
			bAVDGetMA = 1;
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d,
				get from heap_id %d\033[m\n",
				__PRETTY_FUNCTION__, __LINE__, AVD_CMA_Alloc_PARAM.pool_handle_id);
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d, ask offset 0x%lX\033[m\n",
				__PRETTY_FUNCTION__, __LINE__, AVD_CMA_Alloc_PARAM.offset_in_pool);
			ULOGE("AVD", "\033[35mFunction = %s, Line = %d, ask length 0x%lX\033[m\n",
				__PRETTY_FUNCTION__, __LINE__, AVD_CMA_Alloc_PARAM.length);
			ULOGE("AVD", "\033[37mFunction = %s, Line = %d, return va is 0x%lX\033[m\n",
				__PRETTY_FUNCTION__, __LINE__, AVD_CMA_Alloc_PARAM.virt_addr);

			AVD_CMA_Free_PARAM.pool_handle_id = AVD_CMA_Pool_Init_PARAM.pool_handle_id;
			AVD_CMA_Free_PARAM.offset_in_pool = AVD_CMA_Alloc_PARAM.offset_in_pool;
			AVD_CMA_Free_PARAM.length = AVD_CMA_Alloc_PARAM.length;
		}
	}
#endif
#endif

	if (u32AVDopen == 1)
		ret = Drv_AVD_SetInput(eSource, u8ScartFB, pAVDResoucePrivate);
	return ret;
}

MS_BOOL Api_AVD_SetVideoStandard(enum AVD_VideoStandardType eStandard, MS_BOOL bIsInAutoTuning)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_SetVideoStandard(eStandard, bIsInAutoTuning, pAVDResoucePrivate);
	return ret;
}

void Api_AVD_SetChannelChange(void)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetChannelChange(pAVDResoucePrivate);
}

void Api_AVD_SetHsyncDetectionForTuning(MS_BOOL bEnable)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetHsyncDetectionForTuning(bEnable, pAVDResoucePrivate);
}

void Api_AVD_Set3dComb(MS_BOOL bEnable)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_Set3dComb(bEnable, pAVDResoucePrivate);
}

void Api_AVD_SetShiftClk(MS_BOOL bEnable, enum AVD_ATV_CLK_TYPE eShiftMode)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetShiftClk(bEnable, eShiftMode, pAVDResoucePrivate);
}

void Api_AVD_SetFreerunPLL(enum AVD_VideoFreq eVideoFreq)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetFreerunPLL(eVideoFreq, pAVDResoucePrivate);
}

void Api_AVD_SetFreerunFreq(enum AVD_FreeRunFreq eFreerunfreq)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetFreerunFreq(eFreerunfreq, pAVDResoucePrivate);
}

void Api_AVD_SetFactoryPara(enum AVD_Factory_Para FactoryPara, MS_U8 u8Value)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetFactoryPara(FactoryPara, u8Value, pAVDResoucePrivate);
}

void Api_AVD_Set_Htt_UserMD(MS_U16 u16Htt)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		pAVDResoucePrivate->_u16Htt_UserMD = u16Htt;
}

MS_BOOL Api_AVD_SetDbgLevel(enum AVD_DbgLv u8DbgLevel)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_SetDbgLevel(u8DbgLevel, pAVDResoucePrivate);
	return ret;
}

void Api_AVD_SetPQFineTune(void)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetPQFineTune(pAVDResoucePrivate);
}

void Api_AVD_Set3dCombSpeed(MS_U8 u8COMB57, MS_U8 u8COMB58)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_Set3dCombSpeed(u8COMB57, u8COMB58, pAVDResoucePrivate);
}

void Api_AVD_SetStillImageParam(struct AVD_Still_Image_Param param)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetStillImageParam(param, pAVDResoucePrivate);
}

void Api_AVD_SetAFECD4Factory(MS_U8 u8Value)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		pAVDResoucePrivate->_u8AfecD4Factory = u8Value;
}

void Api_AVD_Set2D3DPatchOnOff(MS_BOOL bEnable)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_Set2D3DPatchOnOff(bEnable);
}

MS_U32 Api_AVD_GetFlag(void)
{
	MS_U32 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = pAVDResoucePrivate->g_VD_InitData.u32VDPatchFlag;
	return ret;
}

MS_U8 Api_AVD_GetStandard_Detection_Flag(void)
{
	MS_U8 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetStandard_Detection_Flag();
	return ret;
}

MS_U16 Api_AVD_GetStatus(void)
{
	MS_U16 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetStatus();
	return ret;
}

MS_U8 Api_AVD_GetNoiseMag(void)
{
	MS_U8 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetNoiseMag();
	return ret;
}

MS_U16 Api_AVD_GetVTotal(void)
{
	MS_U16 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetVTotal();
	return ret;
}

enum AVD_VideoStandardType Api_AVD_GetStandardDetection(MS_U16 *u16LatchStatus)
{
	enum AVD_VideoStandardType ret = E_VIDEOSTANDARD_NOTSTANDARD;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetStandardDetection(u16LatchStatus, pAVDResoucePrivate);
	return ret;
}

enum AVD_VideoStandardType Api_AVD_GetLastDetectedStandard(void)
{
	enum AVD_VideoStandardType ret = E_VIDEOSTANDARD_NOTSTANDARD;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = pAVDResoucePrivate->_eLastStandard;
	return ret;

}

#if !defined STI_PLATFORM_BRING_UP
void Api_AVD_GetCaptureWindow(void *stCapWin, enum AVD_VideoStandardType eVideoStandardType)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_GetCaptureWindow(stCapWin, eVideoStandardType, pAVDResoucePrivate);
}
#endif
enum AVD_VideoFreq Api_AVD_GetVerticalFreq(void)
{
	enum AVD_VideoFreq ret = E_VIDEO_FQ_NOSIGNAL;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetVerticalFreq();
	return ret;
}

MS_U8 Api_AVD_GetHsyncEdge(void)
{
	MS_U8 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetHsyncEdge();
	return ret;
}

MS_U8 Api_AVD_GetDSPFineGain(void)
{
	MS_U8 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetDSPFineGain(pAVDResoucePrivate);
	return ret;
}

MS_U16 Api_AVD_GetDSPVersion(void)
{
	MS_U16 ret = 0;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetDSPVersion();
	return ret;
}
#if !defined STI_PLATFORM_BRING_UP
enum AVD_Result Api_AVD_GetLibVer(const MSIF_Version **ppVersion)
{
	enum AVD_Result ret = E_AVD_FAIL;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetLibVer(ppVersion);
	return ret;
}
#endif
const struct AVD_Info *Api_AVD_GetInfo(void)
{
	static struct AVD_Info eAVDInfo = {0};

	CHECKAVDOPEN();
	if ((u32AVDopen != -1) && (u8AVDUtopiaOpen == 1))
		Drv_AVD_GetInfo(&eAVDInfo, pAVDResoucePrivate);
	return &eAVDInfo;
}

MS_BOOL Api_AVD_IsSyncLocked(void)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_IsSyncLocked();
	return ret;
}

MS_BOOL Api_AVD_IsSignalInterlaced(void)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_IsSignalInterlaced();
	return ret;
}

MS_BOOL Api_AVD_IsColorOn(void)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_IsColorOn();
	return ret;
}

MS_U32 Api_AVD_SetPowerState(EN_POWER_MODE u16PowerState)
{
	MS_U32 ret = 1;// 1 means success

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_SetPowerState(u16PowerState, pAVDResoucePrivate);
	return ret;
}

MS_BOOL Api_AVD_GetMacroVisionDetect(void)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetMacroVisionDetect();
	return ret;
}

MS_BOOL Api_AVD_GetCGMSDetect(void)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_GetCGMSDetect();
	return ret;
}

void SYMBOL_WEAK Api_AVD_SetBurstWinStart(MS_U8 u8BusrtStartPosition)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetBurstWinStart(u8BusrtStartPosition);
}

MS_BOOL SYMBOL_WEAK Api_AVD_AliveCheck(void)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (1 == bAVD_Open_result && 1 == u32AVDopen)
		ret = Drv_AVD_IsAVDAlive();
	return ret;
}

MS_BOOL SYMBOL_WEAK Api_AVD_IsLockAudioCarrier(void)
{
	MS_BOOL ret = FALSE;

	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		ret = Drv_AVD_IsLockAudioCarrier();
	return ret;
}
void Api_AVD_SetAnaDemod(MS_BOOL bEnable)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetAnaDemod(bEnable, pAVDResoucePrivate);
}

void Api_AVD_SetHsyncSensitivityForDebug(MS_U8 u8AFEC99, MS_U8 u8AFEC9C)
{
	CHECKAVDOPEN();
	if (u32AVDopen == 1)
		Drv_AVD_SetHsyncSensitivityForDebug(u8AFEC99, u8AFEC9C, pAVDResoucePrivate);
}

MS_BOOL Api_AVD_SetHWInfo(struct AVD_HW_Info AVDHWInfo)
{
	return Drv_AVD_SetHWInfo(AVDHWInfo);
}

#endif // UTOPIAXP_REMOVE_WRAPPER
