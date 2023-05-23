// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
///	@file	AVD.c
///	@brief	AVD	Driver
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//	Include	Files
//-------------------------------------------------------------------------------------------------
// Common	Definition
//#include <string.h>
#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/string.h>
#else
#include <string.h>
#endif
//#include "MsCommon.h"
//#include "MsVersion.h"
//#include "MsIRQ.h"
//#include "drvXC_IOPort.h"
//#include "apiXC.h"
//#if	!defined (MSOS_TYPE_NOS)
	 //	#include "MsOS.h"
//#endif
#include "sti_msos.h"
//#include "MsTypes.h"
#ifndef	MSOS_TYPE_LINUX_KERNEL
#include <stdio.h>
#endif
#include "drvAVD_priv.h"
#include "drvAVD_v2.h"
//#include "../../utopia_core/utopia_dapi.h"
//#include "../../utopia_core/utopia.h"
//#include "../../utopia_core/utopia_driver_id.h"

// Internal	Definition
//#include "regCHIP.h"
//#include "regAVD.h"
//#include "halAVD.h"
//#include "drvBDMA.h"
//#include "drvSERFLASH.h"
//#include "ULog.h"
#include "hwreg_srccap_avd_avd.h"

//-------------------------------------------------------------------------------------------------
//	Driver Compiler	Options
//-------------------------------------------------------------------------------------------------
#define	TEST_VD_DSP 0 // 1: for TEST VD DSP, 0: for MSTAR VD DSP
#define	ADJUST_CHANNEL_CHANGE_HSYNC	1
#define	ADJUST_NTSC_BURST_WINDOW 1

#if	defined(MSTAR_STR)
#define	AVD_2K_STR 1
#endif
//-------------------------------------------------------------------------------------------------
//	Local	Defines
//-------------------------------------------------------------------------------------------------

//avd	mutex	wait time
#define	AVD_MUTEX_WAIT_TIME	 1000

#if	(TEST_VD_DSP)
#undef	VD_STATUS_RDY
#define	VD_STATUS_RDY		(BIT(8))
#define	VD_STANDARD_VALID	BIT(0)
#endif

//#define	MODULE_AVD 10	 //weicheng	201303 multi process safe

#define	AVD_debug(x) //x
#define	AVD_debug_Monitor(x) //x

#if	defined(STELLAR_FLAG)
//for Lm15u,after setcolorsystem, GetStandardDetection need to return the standard which is enable
	#define	LM15U_GetStandard 1
#endif
//-------------------------------------------------------------------------------------------------
//	Debug	Functions
//-------------------------------------------------------------------------------------------------
#if	!defined STI_PLATFORM_BRING_UP
#ifdef MS_DEBUG
#define	VDDBG(x) (x)
//#define AVD_DRV_DEBUG 1
#define	AVD_LOCK(_s32AVD_Mutex, _u8AVDDbgLevel)\
do {\
	MS_ASSERT(MsOS_In_Interrupt() == FALSE);\
	if (_u8AVDDbgLevel == AVD_DBGLV_DEBUG)\
		ULOGE("AVD", "%s lock\n", __func__);\
	MsOS_ObtainMutex(_s32AVD_Mutex,	MSOS_WAIT_FOREVER);\
	} while (0)
#define	AVD_UNLOCK(_s32AVD_Mutex, _u8AVDDbgLevel)\
do {\
	MsOS_ReleaseMutex(_s32AVD_Mutex);\
	if (_u8AVDDbgLevel == AVD_DBGLV_DEBUG)\
		ULOGE("AVD", "%s unlock\n", __func__);\
	} while (0)
#else
#define	VDDBG(x) //(x)
//#define	AVD_DRV_DEBUG	0
#define	AVD_LOCK(_s32AVD_Mutex, _u8AVDDbgLevel)\
	do {\
		MS_ASSERT(MsOS_In_Interrupt() == FALSE);\
		MsOS_ObtainMutex(_s32AVD_Mutex,	MSOS_WAIT_FOREVER);\
		} while (0)
#define AVD_UNLOCK(_s32AVD_Mutex, _u8AVDDbgLevel) MsOS_ReleaseMutex(_s32AVD_Mutex)
#endif
#else
#define	VDDBG(x)
#define	AVD_UNLOCK(_s32AVD_Mutex, _u8AVDDbgLevel)
#define	AVD_LOCK(_s32AVD_Mutex, _u8AVDDbgLevel)
#endif

//-------------------------------------------------------------------------------------------------
//	Global Constant
//-------------------------------------------------------------------------------------------------
#if	!defined STI_PLATFORM_BRING_UP
static MSIF_Version	_drv_avd_version = {
	.MW	=	{	MSIF_TAG,			 /*	'MSIF' */
		MSIF_CLASS,			 /*	'00'	*/
		MSIF_CUS,			 /*	0x0000	*/
		MSIF_MOD,			 /*	0x0000 */
		MSIF_CHIP,
		MSIF_CPU,
		MSIF_AVD_LIB_CODE,	 /*	IP__ */
		MSIF_AVD_LIBVER,	 /*	0.0	~	Z.Z*/
		MSIF_AVD_BUILDNUM,	 /*	00 ~ 99*/
		MSIF_AVD_CHANGELIST, /*	CL#	*/
		MSIF_OS, },
};
#endif
static MS_PHY u32COMB_3D_Addr_Str;
static MS_U32 u32COMB_3D_Len_Str;
static enum	AVD_InputSourceType	eSource_Str	= E_INPUT_SOURCE_INVALID;
static MS_U8 u8ScartFB_Str = 1;
static MS_U32 u32XTAL_Clock_Str = 12000000UL;
static enum	AVD_DemodType eDemodType_Str = DEMODE_MSTAR_VIF;
static MS_U8 *pu8VD_DSP_Code_Address_Str;
static MS_U32 u32VD_DSP_Code_Len_Str;
static MS_BOOL bInitSTR;
#if	!defined STI_PLATFORM_BRING_UP
#if	defined(MSOS_TYPE_LINUX_KERNEL)
static enum	AVD_VideoStandardType eVideoStandard_Str = E_VIDEOSTANDARD_NOTSTANDARD;
#endif
#endif

//	 use x1.5	sampling
MS_U16 _u16HtotalNTSC[] = {(1135L*3/2), (1135L), (910L), (1135L)};
MS_U16 _u16HtotalPAL[] = {(1135L*3/2), (1135L), (1135L), (1135L)};
MS_U16 _u16HtotalSECAM[] = {(1135L*3/2), (1135L), (1097L), (1135L)};
MS_U16 _u16HtotalNTSC_443[] = {(1135L*3/2), (1135L), (1127L), (1135L)};
MS_U16 _u16HtotalPAL_M[] = {(1135L*3/2), (1135L), (909L), (1135L)};
MS_U16 _u16HtotalPAL_NC[] = {(1135L*3/2), (1135L), (917L), (1135L)};
MS_U16 _u16HtotalPAL_60[] = {(1135L*3/2), (1135L), (1127L), (1135L)};

/*
 * use x1 sampling
 * MS_U16 _u16HtotalNTSC[]={(1135L),	(1135L), (910L), (1135L)};
 * MS_U16 _u16HtotalPAL[]={(1135L), (1135L),	(1135L), (1135L)};
 * MS_U16 _u16HtotalSECAM[]={(1135L), (1135L),	(1097L), (1135L)};
 * MS_U16 _u16HtotalNTSC_443[]={(1135L),	(1135L), (1127L),	(1135L)};
 * MS_U16 _u16HtotalPAL_M[]={(1135L), (1135L),	(909L),	(1135L)};
 * MS_U16 _u16HtotalPAL_NC[]={(1135L), (1135L), (917L), (1135L)};
 * MS_U16 _u16HtotalPAL_60[]={(1135L), (1135L), (1127L),	(1135L)};
 */
//-------------------------------------------------------------------------------------------------
//	Enum
//-------------------------------------------------------------------------------------------------

enum {
	AVD_POOL_ID_AVD	= 0,
} eAVDPoolID;

//-------------------------------------------------------------------------------------------------
//	Interface	function
//-------------------------------------------------------------------------------------------------
static void	vd_str_store_reg(void);
static void	vd_str_store_comb_sub_reg(void);
static void	vd_str_init(void);
static void	vdmcu_str_init(void);
static void	vd_str_setinput(void);
static void	vd_str_load_reg(void);
static void	vd_str_load_comb_sub_reg(void);
#if	!defined STI_PLATFORM_BRING_UP
#if	defined(MSOS_TYPE_LINUX_KERNEL)
static enum	AVD_VideoStandardType vd_str_timingchangedetection(void);
#endif
#endif
#ifdef CONFIG_UTOPIA_PROC_DBG_SUPPORT
MS_U32 AVDMdbIoctl(MS_U32 cmd, const void *const pArgs)
{
	MDBCMD_CMDLINE_PARAMETER *paraCmdLine;
	MDBCMD_GETINFO_PARAMETER *paraGetInfo;

	switch (cmd) {
	case MDBCMD_CMDLINE:
		paraCmdLine	= (MDBCMD_CMDLINE_PARAMETER *)pArgs;
		//MdbPrint(paraCmdLine->u64ReqHdl,"LINE:%d,	MDBCMD_CMDLINE\n", __LINE__);
		//MdbPrint(paraCmdLine->u64ReqHdl,"u32CmdSize: %d\n",	paraCmdLine->u32CmdSize);
		//MdbPrint(paraCmdLine->u64ReqHdl,"pcCmdLine:	%s\n", paraCmdLine->pcCmdLine);
		Drv_AVD_DBG_EchoCmd(paraCmdLine->u64ReqHdl,
		paraCmdLine->u32CmdSize, paraCmdLine->pcCmdLine);
		paraCmdLine->result	= MDBRESULT_SUCCESS_FIN;
		break;
	case MDBCMD_GETINFO:
		paraGetInfo	= (MDBCMD_GETINFO_PARAMETER *)pArgs;
		MdbPrint(paraGetInfo->u64ReqHdl, "LINE:%d, MDBCMD_GETINFO\n", __LINE__);
		Drv_AVD_DBG_GetModuleInfo(paraGetInfo->u64ReqHdl);
		paraGetInfo->result	= MDBRESULT_SUCCESS_FIN;
		break;
	default:
		break;
	}
	return 0;
}
#endif

extern void	SerPrintf(char *fmt, ...);

MS_U32 AVDStr(MS_U32 u32PowerState, void *pModule)
{
	MS_U32 u32Return = UTOPIA_STATUS_FAIL;

	//UtopiaModuleGetSTRPrivate(pModule, (void**));
	if (u32PowerState == E_POWER_SUSPEND) {
		#if	defined(MSTAR_STR)
		if (bInitSTR == 1) {
#if	!defined STI_PLATFORM_BRING_UP
#if	defined(MSOS_TYPE_LINUX_KERNEL)
			eVideoStandard_Str = vd_str_timingchangedetection();
#endif
#endif
			vd_str_store_reg();
			vd_str_store_comb_sub_reg();
			HWREG_AVD_COMB_SetMemoryRequest(DISABLE);
		}
		#endif
		u32Return = UTOPIA_STATUS_SUCCESS;//SUSPEND_OK;
	} else if (u32PowerState == E_POWER_RESUME) {
#if	defined(MSTAR_STR)
#if	!defined STI_PLATFORM_BRING_UP
#if	defined(MSOS_TYPE_LINUX_KERNEL)
		if (UtopiaStrWaitCondition("avd", u32PowerState, 0) == UTOPIA_STATUS_FAIL) {
			printf("[%s][%d] AVD PowerState:RESUME wait Condition failed!!!!\n",
				__func__, __LINE__);
		}
#endif
#endif
		if (bInitSTR == 1) {
			vd_str_init();
			#if	((T3_LOAD_CODE == 0) && (TWO_VD_DSP_CODE == 0))
			vdmcu_str_init();
			#endif

			vd_str_setinput();
			vd_str_load_reg();
			vd_str_load_comb_sub_reg();
			if (eSource_Str != E_INPUT_SOURCE_INVALID)
				vd_str_stablestatus();
#if	!defined STI_PLATFORM_BRING_UP
#if	defined(MSOS_TYPE_LINUX_KERNEL)
			if ((eSource_Str == E_INPUT_SOURCE_CVBS1) ||
				 (eSource_Str == E_INPUT_SOURCE_CVBS2) ||
				 (eSource_Str == E_INPUT_SOURCE_CVBS3) ||
				 (eSource_Str == E_INPUT_SOURCE_SCART1) ||
				 (eSource_Str == E_INPUT_SOURCE_SCART2) ||
				 (eSource_Str == E_INPUT_SOURCE_SVIDEO1) ||
				 (eSource_Str == E_INPUT_SOURCE_SVIDEO2)) {
				enum AVD_VideoStandardType etemp_VideoStandard =
					vd_str_timingchangedetection();

				if (eVideoStandard_Str != etemp_VideoStandard)
					UtopiaStrSetData("timing change", "yes");
				else
					UtopiaStrSetData("timing change", "no");
			}
#endif
#endif
		}
#if	!defined STI_PLATFORM_BRING_UP
#if	defined(MSOS_TYPE_LINUX_KERNEL)
		UtopiaStrSendCondition("avd", u32PowerState, 0);
#endif
#endif
#endif
		u32Return = UTOPIA_STATUS_SUCCESS;//RESUME_OK;
	} else
		//SerPrintf("[AVD] STR unknown State!!!\n");
		u32Return = UTOPIA_STATUS_FAIL;

	return u32Return;//for success
}

static void	vd_str_store_reg(void)
{
	HWREG_AVD_STR_LOAD_REG();
}

static void	vd_str_store_comb_sub_reg(void)
{
	HWREG_AVD_STR_COMB_STORE_SUB_REG();
}

static void	vd_str_init(void)
{
	HWREG_AVD_RegInit();
	HWREG_AVD_COMB_SetMemoryProtect(u32COMB_3D_Addr_Str, u32COMB_3D_Len_Str);
}

static void	vdmcu_str_init(void)
{
	#if	T3_LOAD_CODE
		HWREG_AVD_AFEC_SetClockSource(TRUE);
	#endif

	HWREG_AVD_VDMCU_LoadDSP(pu8VD_DSP_Code_Address_Str, u32VD_DSP_Code_Len_Str);

	#if	T3_LOAD_CODE
	HWREG_AVD_VDMCU_SetFreeze(ENABLE);
	HWREG_AVD_AFEC_SetClockSource(FALSE);
	HWREG_AVD_VDMCU_SetFreeze(DISABLE);
	#endif
	HWREG_AVD_VDMCU_SetClock(HW_REG_AVD_VDMCU_CLOCK_108Mhz, HW_REG_AVD_VDMCU_CLOCK_NORMAL);
}

static void	vd_str_setinput(void)
{
	HWREG_AVD_AFEC_SetInput(eSource_Str, u8ScartFB_Str, eDemodType_Str, u32XTAL_Clock_Str);

	#if (TWO_VD_DSP_CODE)
	vdmcu_str_init();
	#endif

	#if	T3_LOAD_CODE
	_Drv_AVD_VDMCU_Init();
	#else
	HWREG_AVD_AFEC_McuReset();
	#endif

	HWREG_AVD_COMB_SetMemoryRequest(ENABLE);
}

static void	vd_str_load_reg(void)
{
	HWREG_AVD_STR_LOAD_REG();
}

static void	vd_str_load_comb_sub_reg(void)
{
	HWREG_AVD_STR_COMB_LOAD_SUB_REG();
}

//-------------------------------------------------------------------------------------------------
//	Local VD functions
//-------------------------------------------------------------------------------------------------
static void	_Drv_AVD_VDMCU_Init(struct AVD_RESOURCE_PRIVATE	*pResource)
{
	#if	NEW_VD_MCU
	MS_U8 u8Temp;
	#endif

	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "_MDrv_AVD_VDMCU_Init\n");
	#endif

	#if	T3_LOAD_CODE //weicheng 20121003
	HWREG_AVD_AFEC_SetClockSource(TRUE);
	#endif

#if	NO_DEFINE
	if (!(g_VD_InitData.u32VDPatchFlag & AVD_PATCH_DISABLE_PWS))
		HAL_PWS_Stop_VDMCU();
#endif

#if	defined(MSTAR_STR)
	// store DSP code	information	for	STR	function
	pu8VD_DSP_Code_Address_Str = pResource->g_VD_InitData.pu8VD_DSP_Code_Address;
	u32VD_DSP_Code_Len_Str = pResource->g_VD_InitData.u32VD_DSP_Code_Len;
#endif

	//--------------------------------------------------------
	// Load	code
	//--------------------------------------------------------
	#if	(LOAD_CODE_BYTE_WRITE_ONLY)//weicheng	20121003
	if (1)
	#else
	//BY  20090402 (_bVD_FWStatus)
	if (pResource->g_VD_InitData.eLoadCodeType == AVD_LOAD_CODE_BYTE_WRITE)
	#endif
	{
		// TODO	add	different	loading	function for T3
		#if	AVD_DRV_DEBUG
		if (pResource->_u8AVDDbgLevel	== AVD_DBGLV_DEBUG)
			ULOGE("AVD", "[AVD_LOAD_CODE_BYTE_WRITE]F/W ADDR = 0x%X\n",
			(unsigned int)(pResource->g_VD_InitData.pu8VD_DSP_Code_Address));
		#endif
		HWREG_AVD_VDMCU_LoadDSP(pResource->g_VD_InitData.pu8VD_DSP_Code_Address,
		pResource->g_VD_InitData.u32VD_DSP_Code_Len);
		#if	T3_LOAD_CODE	//weicheng 20121003
		HWREG_AVD_VDMCU_SetFreeze(ENABLE);
		HWREG_AVD_AFEC_SetClockSource(FALSE);
		HWREG_AVD_VDMCU_SetFreeze(DISABLE);
		#endif
	} else if (pResource->g_VD_InitData.eLoadCodeType == AVD_LOAD_CODE_BDMA_FROM_SPI) {
		#if	AVD_DRV_DEBUG
		if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
			ULOGE("AVD", "[AVD_LOAD_CODE_BDMA_FROM_SPI]F/W ADDR = 0x%X 0x%X\n",
			(unsigned int)pResource->g_VD_InitData.u32VD_DSP_Code_Address,
			(unsigned int)pResource->g_VD_InitData.u32VD_DSP_Code_Len);
		#endif
		HWREG_AVD_VDMCU_SetFreeze(ENABLE);
		#if	NEW_VD_MCU
		u8Temp = HWREG_AVD_GET_VD_MCU_SRAM();
		HWREG_AVD_SET_VD_MCU_SRAM(u8Temp & (~BIT(0)));
		#endif
		#if	NEW_VD_MCU
		HWREG_AVD_SET_VD_MCU_SRAM(u8Temp | BIT(0));
		#endif

		HWREG_AVD_VDMCU_SetFreeze(DISABLE);
		#if	T3_LOAD_CODE //weicheng 20121003
		HWREG_AVD_VDMCU_SetFreeze(ENABLE);
		HWREG_AVD_AFEC_SetClockSource(FALSE);
		HWREG_AVD_VDMCU_SetFreeze(DISABLE);
		#endif
	}
	#if	T3_LOAD_CODE //weicheng 20121003
	else if	(pResource->g_VD_InitData.eLoadCodeType	== AVD_LOAD_CODE_BDMA_FROM_DRAM) {
		//TODO add MIU0/1 check
		#if	AVD_DRV_DEBUG
		if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
			ULOGE("AVD", "[AVD_LOAD_CODE_BDMA_FROM_DRAM]F/W ADDR = 0x%X\n",
			(unsigned int)(pResource->g_VD_InitData.pu8VD_DSP_Code_Address));
		#endif
		HWREG_AVD_VDMCU_SetFreeze(ENABLE);
		MDrv_BDMA_CopyHnd(pResource->g_VD_InitData.u32VD_DSP_Code_Address, 0,
			pResource->g_VD_InitData.u32VD_DSP_Code_Len,
			E_BDMA_SDRAM2VDMCU,	BDMA_OPCFG_DEF);
		HWREG_AVD_VDMCU_SetFreeze(DISABLE);

		HWREG_AVD_VDMCU_SetFreeze(ENABLE);
		HWREG_AVD_AFEC_SetClockSource(FALSE);
		HWREG_AVD_VDMCU_SetFreeze(DISABLE);
	}
	#endif
	else {
		#if	AVD_DRV_DEBUG
		if (pResource->_u8AVDDbgLevel >= AVD_DBGLV_ERR)
			ULOGE("AVD", "_MDrv_AVD_VDMCU_Init eLoadCodeType Invalid\r\n");
		#endif
	}

	HWREG_AVD_VDMCU_SetClock(HW_REG_AVD_VDMCU_CLOCK_108Mhz, HW_REG_AVD_VDMCU_CLOCK_NORMAL);
	HWREG_AVD_AFEC_SetPatchFlag(pResource->g_VD_InitData.u32VDPatchFlag);

	#if	NO_DEFINE
	#if	defined(STELLAR_FLAG)
	//TODO use system variable type
	HWREG_AVD_RegInitExt(pResource->g_VD_InitData.u8VdDecInitializeExt);
	#endif
	#endif
	pResource->_u8AfecD4Factory = HWREG_AVD_AFEC_GET_PACTH_D4();
}

static void	_Drv_AVD_3DCombSpeedup(struct	AVD_RESOURCE_PRIVATE *pResource)
{
	//need to review, sonic 20091218
	HWREG_AVD_COMB_Set3dCombSpeed(pResource->_u8Comb57,
		pResource->_u8Comb58, pResource->_u8Comb5F);
	#if	ADJUST_CHANNEL_CHANGE_HSYNC
	HWREG_AVD_AFEC_SetSwingLimit(0);
	//more sensitivity
	HWREG_AVD_AFEC_EnableBottomAverage(DISABLE);
	#endif
	if (!(pResource->_u8AfecD4Factory & BIT(4)))
		HWREG_AVD_AFEC_EnableVBIDPLSpeedup(ENABLE);
	HWREG_AVD_AFEC_SET_DPL_K2(0x6A);
	HWREG_AVD_AFEC_SET_DPL_K1(0xBC);
	pResource->_u8Comb10Bit3Flag = BIT(2) | BIT(1) | BIT(0);
	pResource->_u32VideoSystemTimer = MsOS_GetSystemTime();
	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "_Api_AVD_3DCombSpeedup Enable %d\n",
		(int)pResource->_u32VideoSystemTimer);
	#endif
}

//20120719 Brian Monitor the Dsp frequency and change the CLK setting for the  shift CLK
static void	_Drv_AVD_ShifClk_Monitor(struct AVD_RESOURCE_PRIVATE *pResource)
{
		MS_U16	u16Status;
		enum AVD_VideoStandardType eVideoStandard = E_VIDEOSTANDARD_PAL_BGHI;

		u16Status = HWREG_AVD_AFEC_GetStatus();
		switch (u16Status & VD_FSC_TYPE) {
		case VD_FSC_4433:
			eVideoStandard = E_VIDEOSTANDARD_PAL_BGHI;
			break;

		case VD_FSC_3579:
			eVideoStandard = E_VIDEOSTANDARD_NTSC_M;
			break;

		case VD_FSC_3575:
			eVideoStandard = E_VIDEOSTANDARD_PAL_M;
			break;

		case VD_FSC_3582:
			eVideoStandard = E_VIDEOSTANDARD_PAL_N;
			break;
		case VD_FSC_4285:
			eVideoStandard = E_VIDEOSTANDARD_SECAM;
			break;

		default:
			break;

	}
		 HWREG_AVD_ShiftClk(pResource->gShiftMode, eVideoStandard,
			pResource->g_VD_InitData.u32XTAL_Clock);
}

static void	_Drv_AVD_SCART_Monitor(struct	AVD_RESOURCE_PRIVATE *pResource)
{
	pResource->_u32SCARTWaitTime = MsOS_GetSystemTime();
	if (pResource->_u8SCARTSwitch == 1) {//Svideo
		HWREG_AVD_SCART_Setting(HW_REG_E_INPUT_SOURCE_SVIDEO1);

		if (pResource->_eVideoSystem == E_VIDEOSTANDARD_NTSC_M)
			HWREG_AVD_COMB_SetYCPipe(0x20);
		else //	SIG_NTSC_443,	SIG_PAL, SIG_PAL_M, SIG_PAL_NC, SIG_SECAM
			HWREG_AVD_COMB_SetYCPipe(0x30);

		switch (pResource->_eVideoSystem) {
		case E_VIDEOSTANDARD_PAL_M:
		case E_VIDEOSTANDARD_PAL_N:
			HWREG_AVD_COMB_SetCbCrInverse(0x04);
			break;

		case E_VIDEOSTANDARD_NTSC_44:
			HWREG_AVD_COMB_SetCbCrInverse(0x0C);
			break;

		case E_VIDEOSTANDARD_PAL_60:
			HWREG_AVD_COMB_SetCbCrInverse(0x08);
			break;

		case E_VIDEOSTANDARD_NTSC_M:
		case E_VIDEOSTANDARD_SECAM:
			HWREG_AVD_COMB_SetCbCrInverse(0x00);
			break;

		case E_VIDEOSTANDARD_PAL_BGHI:
		default:
			HWREG_AVD_COMB_SetCbCrInverse(0x08);
			break;
		}
	} else {//CVBS
		HWREG_AVD_SCART_Setting(HW_REG_E_INPUT_SOURCE_CVBS1);
		HWREG_AVD_COMB_SetYCPipe(0x20);
		HWREG_AVD_COMB_SetCbCrInverse(0x00);
	}
	pResource->_u8SCARTPrestandard = pResource->_eVideoSystem;

}

#if	NO_DEFINE
static void	_Drv_AVD_SyncRangeHandler(struct AVD_RESOURCE_PRIVATE *pResource)
{
#ifndef	MSOS_TYPE_LINUX
	XC_ApiStatus sXC_Status;
	XC_SETWIN_INFO sXC_SetWinInfo;
	MS_WINDOW_TYPE SrcWin;
	static MS_BOOL bXC_BlackScreen_Enabled = TRUE;
	static MS_U16 u16XC_VCap;

	MS_U16 wVtotal = HWREG_AVD_AFEC_GetVTotal();
	MS_U16 wVsize_bk, g_cVSizeShift;
	static MS_U8 u8SyncStableCounter = 6;

	if ((!Drv_AVD_IsSyncLocked()) ||
		MApi_XC_IsFreezeImg(MAIN_WINDOW) ||
		(pResource->u16PreVtotal ==	0)) {
		bXC_BlackScreen_Enabled = TRUE;
		return;
	}

/*
 *	if ((wHPeriod > (u16PreHPeriod + 3)) || (wHPeriod < (u16PreHPeriod - 3))) {
 *		printf("P3 \r\n");
 *		return;
 *	}
 */

	// Get XC capture height from the beginning
	if (!(MApi_XC_IsBlackVideoEnable(MAIN_WINDOW))) {
		if (bXC_BlackScreen_Enabled) {
			// update new capture height
			bXC_BlackScreen_Enabled	= FALSE;
			MApi_XC_GetCaptureWindow(&SrcWin, MAIN_WINDOW);
			u16XC_VCap = SrcWin.height;
#if	NO_DEFINE
			if ((wVtotal < 675) && (wVtotal > 570))
				u16PreVtotal = 625;
			else
				u16PreVtotal = 525;
#endif
			printf("******** VCap = %d, PreVtt = %d\n",
			u16XC_VCap, pResource->u16PreVtotal);
		}
	} else {
		// XC still blocked the video
		bXC_BlackScreen_Enabled = TRUE;
		return;
	}

	//if (((wVtotal < 620) && (wVtotal > 570)) || ((wVtotal > 630) && (wVtotal < 670)))
	if ((wVtotal < 620)	&& (wVtotal	> 570)) {
		if (pResource->u16PreVtotal	!= wVtotal) {
			pResource->u16PreVtotal	 = wVtotal;
			u8SyncStableCounter	= 0;
		} else if (u8SyncStableCounter < 5)
			u8SyncStableCounter++;
	} else if ((wVtotal < 520) && (wVtotal > 470)) {
	//else if (((wVtotal < 520) && (wVtotal > 470)) || ((wVtotal > 530) && (wVtotal < 570)))

		if (pResource->u16PreVtotal != wVtotal) {
			pResource->u16PreVtotal  = wVtotal;
			u8SyncStableCounter = 0;
		} else if (u8SyncStableCounter < 5)
			u8SyncStableCounter++;
	} else if (((wVtotal <= 630) && (wVtotal >= 620)) ||
	((wVtotal <= 530) && (wVtotal >= 520))) {
		if ((pResource->u16PreVtotal > (wVtotal + 5)) ||
			(pResource->u16PreVtotal < (wVtotal	- 5))) {
			pResource->u16PreVtotal = wVtotal;
			u8SyncStableCounter = 0;
		} else if (u8SyncStableCounter < 5)
			u8SyncStableCounter++;
	}

	if (u8SyncStableCounter == 5) {
		u8SyncStableCounter = 6;
		wVsize_bk = u16XC_VCap;
		if (((wVtotal < 620) && (wVtotal > 570)) || ((wVtotal > 630) && (wVtotal < 670))) {
			if (wVtotal > 625) {
				g_cVSizeShift = wVtotal - 625;
				 u16XC_VCap = u16XC_VCap + g_cVSizeShift;
			} else {
				g_cVSizeShift = 625 - wVtotal;
				 u16XC_VCap = u16XC_VCap - g_cVSizeShift;
			}
		} else if (((wVtotal < 520) && (wVtotal > 470)) ||
		((wVtotal > 530) && (wVtotal < 570))) {
			if (wVtotal > 525) {
				g_cVSizeShift = wVtotal - 525;
				u16XC_VCap = u16XC_VCap + g_cVSizeShift;
			} else {
				g_cVSizeShift = 525 - wVtotal;
				u16XC_VCap = u16XC_VCap - g_cVSizeShift;
			}
		}

		memset(&sXC_Status,	0, sizeof(XC_ApiStatus));
		memset(&sXC_SetWinInfo,	0, sizeof(XC_SETWIN_INFO));
		MApi_XC_GetStatus(&sXC_Status, MAIN_WINDOW);
		memcpy(&sXC_SetWinInfo,	&sXC_Status, sizeof(XC_SETWIN_INFO));

		// reset changed part
		sXC_SetWinInfo.bPreHCusScaling = FALSE;
		sXC_SetWinInfo.bPreVCusScaling = FALSE;
		sXC_SetWinInfo.stCapWin.height = u16XC_VCap;

		MApi_XC_SetWindow(&sXC_SetWinInfo, sizeof(XC_SETWIN_INFO), MAIN_WINDOW);

		u16XC_VCap = wVsize_bk;
	}
#endif
}
#endif
void _Drv_AVD_COMB_StillImage(struct AVD_RESOURCE_PRIVATE	*pResource)
{
	MS_U8 u8Ctl;
	MS_U16 u16Value;
	static MS_U8 check_counter;
	static MS_U16 Total_Nosiemag;
	static MS_U8 status = 2;
	static MS_U16 _u16PreNoiseMag;

	if (pResource->_eVDInputSource == E_INPUT_SOURCE_ATV) {
		// get VD noise magnitude
		u8Ctl = HWREG_AVD_AFEC_GetNoiseMag();
		if (pResource->g_stillImageParam.bMessageOn)
			printf("[AVD]Noise mag =%d\n", (int)u8Ctl);

		if (check_counter < 10) {
			Total_Nosiemag += u8Ctl;
			check_counter++;
		} else {
			u16Value = Total_Nosiemag;
			if (pResource->g_stillImageParam.bMessageOn)
				printf("[AVD]AVG noise mag = %d\n", (int)u16Value);
			u16Value = (8*_u16PreNoiseMag + 8*u16Value)/16;
			if (u16Value <= pResource->g_stillImageParam.u8Threshold1) {
				_u16PreNoiseMag = u16Value;
				check_counter = 0;
				Total_Nosiemag = 0;
				u8Ctl = HWREG_AVD_AVD_COMB_GET_StillImageParam38();
				if (pResource->g_stillImageParam.bMessageOn)
					printf("[AVD]Thread =%d\n\n", u16Value);
					ULOGE("AVD",
					"[1]Thread1: u16Value = 0x%x\n\n", u16Value);
				HWREG_AVD_COMB_SET_StillImage(
					pResource->g_stillImageParam.u8Str1_COMB37,
					(u8Ctl &
					pResource->g_stillImageParam.u8Str1_COMB38),
					pResource->g_stillImageParam.u8Str1_COMB7C,
					pResource->g_stillImageParam.u8Str1_COMBED);
				status = 1;
			} else if ((u16Value < pResource->g_stillImageParam.u8Threshold2) &&
				(u16Value > pResource->g_stillImageParam.u8Threshold1)) {
				_u16PreNoiseMag = u16Value;
				check_counter = 0;
				Total_Nosiemag = 0;
				u8Ctl = HWREG_AVD_AVD_COMB_GET_StillImageParam38();
				if (pResource->g_stillImageParam.bMessageOn)
					printf("[AVD]Thread =%d\n\n", u16Value);
				if (status == 1) {
					if (pResource->g_stillImageParam.bMessageOn)
						ULOGE("AVD",
						"[4]Thread1: u16Value = 0x%x\n\n", u16Value);
					HWREG_AVD_COMB_SET_StillImage(
						pResource->g_stillImageParam.u8Str1_COMB37,
						(u8Ctl &
						pResource->g_stillImageParam.u8Str1_COMB38),
						pResource->g_stillImageParam.u8Str1_COMB7C,
						pResource->g_stillImageParam.u8Str1_COMBED);
					status = 1;
				} else if (status == 2) {
					if (pResource->g_stillImageParam.bMessageOn)
						ULOGE("AVD",
						"[5]Thread2: u16Value = 0x%x\n\n", u16Value);
					 HWREG_AVD_COMB_SET_StillImage(
						pResource->g_stillImageParam.u8Str2_COMB37,
						(u8Ctl &
						pResource->g_stillImageParam.u8Str2_COMB38),
						pResource->g_stillImageParam.u8Str2_COMB7C,
						pResource->g_stillImageParam.u8Str2_COMBED);
					 status	=	2;
				} else if (status == 3) {
					if (pResource->g_stillImageParam.bMessageOn)
						ULOGE("AVD",
						"[6]Thread3: u16Value = 0x%x\n\n", u16Value);
					 status = 2;
				}
			} else if ((u16Value <= pResource->g_stillImageParam.u8Threshold3) &&
				(u16Value >= pResource->g_stillImageParam.u8Threshold2)) {
				_u16PreNoiseMag = u16Value;
				check_counter = 0;
				Total_Nosiemag = 0;
				u8Ctl = HWREG_AVD_AVD_COMB_GET_StillImageParam38();
				if (pResource->g_stillImageParam.bMessageOn)
					//printf("[AVD]Thread =%d\n\n",u16Value);
					ULOGE("AVD",
					"[2]Thread2: u16Value = 0x%x\n\n", u16Value);
				HWREG_AVD_COMB_SET_StillImage(
					pResource->g_stillImageParam.u8Str2_COMB37,
					(u8Ctl &
					pResource->g_stillImageParam.u8Str2_COMB38),
					pResource->g_stillImageParam.u8Str2_COMB7C,
					pResource->g_stillImageParam.u8Str2_COMBED);
				status = 2;
			} else if ((u16Value < pResource->g_stillImageParam.u8Threshold4) &&
				(u16Value > pResource->g_stillImageParam.u8Threshold3)) {
				_u16PreNoiseMag = u16Value;
				check_counter = 0;
				Total_Nosiemag = 0;
				u8Ctl = HWREG_AVD_AVD_COMB_GET_StillImageParam38();
				if (pResource->g_stillImageParam.bMessageOn)
					printf("[AVD]Thread =%d\n\n", u16Value);
				if (status == 1) {
					if (pResource->g_stillImageParam.bMessageOn)
						ULOGE("AVD",
						"[7]Thread1: u16Value = 0x%x\n\n", u16Value);
					status = 2;
				} else if (status == 2) {
					if (pResource->g_stillImageParam.bMessageOn)
						ULOGE("AVD",
						"[8]Thread2: u16Value = 0x%x\n\n", u16Value);
					 HWREG_AVD_COMB_SET_StillImage(
						pResource->g_stillImageParam.u8Str2_COMB37,
						(u8Ctl &
						pResource->g_stillImageParam.u8Str2_COMB38),
						pResource->g_stillImageParam.u8Str2_COMB7C,
						pResource->g_stillImageParam.u8Str2_COMBED);
					 status = 2;
				} else if (status == 3) {
					if (pResource->g_stillImageParam.bMessageOn)
						ULOGE("AVD",
						"[9]Thread3: u16Value = 0x%x\n\n", u16Value);
					 HWREG_AVD_COMB_SET_StillImage(
						pResource->g_stillImageParam.u8Str3_COMB37,
						(u8Ctl &
						pResource->g_stillImageParam.u8Str3_COMB38),
						pResource->g_stillImageParam.u8Str3_COMB7C,
						pResource->g_stillImageParam.u8Str3_COMBED);
					 status = 3;
				}
			} else if (u16Value >= pResource->g_stillImageParam.u8Threshold4) {
				_u16PreNoiseMag = u16Value;
				check_counter = 0;
				Total_Nosiemag = 0;
				u8Ctl = HWREG_AVD_AVD_COMB_GET_StillImageParam38();
				if (pResource->g_stillImageParam.bMessageOn) {
					//printf("[AVD]Thread =%d\n\n",u16Value);
					ULOGE("AVD",
					"=====>3Thread3: u16Value = 0x%x\n\n", u16Value);
				}
				HWREG_AVD_COMB_SET_StillImage(
					pResource->g_stillImageParam.u8Str3_COMB37,
					(u8Ctl &
					pResource->g_stillImageParam.u8Str3_COMB38),
					pResource->g_stillImageParam.u8Str3_COMB7C,
					pResource->g_stillImageParam.u8Str3_COMBED);
				status = 3;
			}
		}
	}
}

static MS_U8 _Drv_AVD_DSPReadByte(MS_U8	u8Cmd, MS_U16 u16Addr)
{
	return HWREG_AVD_DSPReadByte(u8Cmd, u16Addr);
}

//-------------------------------------------------------------------------------------------------
//	Global VD	functions
//-------------------------------------------------------------------------------------------------
enum AVD_Result	Drv_AVD_Init(struct	VD_INITDATA	*pVD_InitData, MS_U32 u32InitDataLen,
struct AVD_RESOURCE_PRIVATE *pResource)
{
	//BY 20090410 MS_U16 u16TimeOut;
	//BY 20090410 MS_U32 u32dst;
	//BY 20090410 MS_BOOL	bResult;
#if	defined(STELLAR_FLAG)
	pResource->_s32AVD_Mutex = MsOS_CreateMutex(E_MSOS_FIFO, "Mutex VD", MSOS_PROCESS_SHARED);
#elif defined(MSTAR_FLAG)
	 char pAVD_Mutex_String[10];
	if (strncpy(pAVD_Mutex_String, "Mutex VD", 10) == NULL) {
		#if AVD_DRV_DEBUG
		if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
			ULOGE("AVD", "Api_AVD_Init strcpy Fail\n");
		#endif
		return E_AVD_FAIL;
	}
#if	!defined STI_PLATFORM_BRING_UP
	pResource->_s32AVD_Mutex =
	MsOS_CreateMutex(E_MSOS_FIFO, pAVD_Mutex_String, MSOS_PROCESS_SHARED);
#endif
#endif
#if	!defined STI_PLATFORM_BRING_UP
	MS_ASSERT(pResource->_s32AVD_Mutex >= 0);
#endif

	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "Api_AVD_Init\n");
	#endif
	if (sizeof(pResource->g_VD_InitData) == u32InitDataLen) {
		memcpy(&pResource->g_VD_InitData, pVD_InitData, u32InitDataLen);
	} else {
		MS_ASSERT(FALSE);
		return E_AVD_FAIL;
	}

	pResource->_u16DataH[0]	= pResource->_u16DataH[1] = pResource->_u16DataH[2] = 1135L;
	pResource->_u8HtotalDebounce = 0;
	pResource->_eVideoSystem = E_VIDEOSTANDARD_NOTSTANDARD;
	pResource->_u8AutoDetMode = FSC_AUTO_DET_ENABLE;
	pResource->_u16CurVDStatus = 0;
	#if	AVD_DRV_DEBUG
	pResource->_u8AVDDbgLevel = AVD_DBGLV_NONE;
	pResource->u32VDPatchFlagStatus = pResource->u32PreVDPatchFlagStatus = 0;
	#endif
	// BY 20090402 remove un-necessary code HAL_AVD_VDMCU_ResetAddress();
	HWREG_AVD_RegInit(); // !! any register access should be after this function
	HWREG_AVD_COMB_SetMemoryProtect(pResource->g_VD_InitData.u32COMB_3D_ADR,
		pResource->g_VD_InitData.u32COMB_3D_LEN);
#if	defined(MSTAR_STR)
	bInitSTR = 1; //Init flag for STR load and write DRAM
	//store COMB address for STR function
	u32COMB_3D_Addr_Str = pResource->g_VD_InitData.u32COMB_3D_ADR;
	u32COMB_3D_Len_Str = pResource->g_VD_InitData.u32COMB_3D_LEN;
#endif
	#if	NO_DEFINE // BY 20090331, for FPGA only, take on effects in real chip, by book.weng
	RIU_WriteByte(BK_AFEC_0A, 0x10);
	RIU_WriteByte(BK_AFEC_0F, 0x48);
	#endif


	#if	NO_DEFINE // should call set input directly to set input source
	if (g_VD_InitData.eDemodType == DEMODE_MSTAR_VIF)
		RIU_WriteByte(BK_AFEC_CF, 0x80);
		RIU_WriteByte(BK_AFEC_21, 0x1D); // TODO check setting correct or not?
		RIU_WriteByte(BK_AFEC_40, 0x91);
	#endif

	HWREG_AVD_SetDSPCodeType(pResource->g_VD_InitData.u32VDPatchFlag);
	#if	((T3_LOAD_CODE == 0) && (TWO_VD_DSP_CODE == 0)) //20121003 weicheng
	// should keep this for T3 because VIF initial  will need mailbox
	_Drv_AVD_VDMCU_Init(pResource);
	#endif

	return E_AVD_OK;
}

void Drv_AVD_Exit(struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "Api_AVD_Exit\n");
	#endif
	#if (NEW_VD_MCU)
	//HWREG_AVD_VDMCU_SoftStop();
	#endif
#if	defined(MSTAR_STR)
	eSource_Str = E_INPUT_SOURCE_INVALID;
#endif
	HWREG_AVD_AFEC_SetClock(DISABLE);
	HWREG_AVD_COMB_SetMemoryRequest(DISABLE);
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

MS_BOOL	Drv_AVD_ResetMCU(struct	AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "MDrv_AVD_ResetMCU\n");
	#endif
	//assume clock source is ready,it's required to switch source before reset after T4
	HWREG_AVD_AFEC_McuReset();
	HWREG_AVD_AFEC_SetPatchFlag(pResource->g_VD_InitData.u32VDPatchFlag);

	// Forced	to PAL mode
	pResource->_u16CurVDStatus = 0;

	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	return TRUE;
}

void Drv_AVD_FreezeMCU(MS_BOOL bEnable, struct AVD_RESOURCE_PRIVATE *pResource)
{
	// should be static,  no mutex
	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "MDrv_AVD_Freeze MCU %d\n", bEnable);
	#endif
	HWREG_AVD_VDMCU_SetFreeze(bEnable);
}

MS_U16 Drv_AVD_Scan_HsyncCheck(MS_U8 u8HtotalTolerance,
	struct AVD_RESOURCE_PRIVATE *pResource)//TODO should add delay between register read
{
	MS_U16 u16CurrentStatus	= VD_RESET_ON;
	MS_U16 u16Dummy, u16Temp, u16SPLHTotal;
	MS_U8 u8Value, u8VdHsyncLockedCount;
	static const MS_U16	u16SPL_NSPL[5] = {
		1135,	// PAL
		1097,	// SECAM
		910,	// NTSC, PAL M
		917,	// PAL Nc
		1127,	// NTSC	4.43
	};

	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	u16Dummy = 20; //	just protect program dead	lock
	u8VdHsyncLockedCount = 0;
	while (u16Dummy--) {
		if (HWREG_AVD_AFEC_GetHWHsync()) {
			u16CurrentStatus = HWREG_AVD_AFEC_GetStatus();
			u8VdHsyncLockedCount++;
		}
	}
	//printf("vdLoop1=%bd\n",	u8VdHsyncLockedCount);

	if (u8VdHsyncLockedCount > 14) {
		//printf("Second Check\n");
		u8VdHsyncLockedCount = 0;
		u16Dummy = 10; //	just protect program dead	lock
		u16SPLHTotal = HWREG_AVD_AFEC_GetHTotal(); //SPL_NSPL, H total
		//printf("Ht=%d\n",	u16SPLHTotal);
		while (u16Dummy--) {
			u16Temp = HWREG_AVD_AFEC_GetHTotal(); //SPL_NSPL, H total
			u16Temp = (u16Temp+u16SPLHTotal)>>1;
			u16SPLHTotal = u16Temp;

			//printf("Ht=%d\n",	u16SPLHTotal);
			for (u8Value = 0; u8Value <= 4; u8Value++) {
				if (abs(u16SPLHTotal - u16SPL_NSPL[u8Value]) < u8HtotalTolerance)
					u8VdHsyncLockedCount++;
			}
			//printf("\n");
		}

		//printf("vdLoop2=%bd\n",	u8VdHsyncLockedCount);
		if (u8VdHsyncLockedCount < 8)
			u16CurrentStatus = VD_RESET_ON;
	} else
		u16CurrentStatus = VD_RESET_ON;

	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	return u16CurrentStatus;
}

void Drv_AVD_StartAutoStandardDetection(struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	pResource->_u8AutoDetMode = FSC_AUTO_DET_ENABLE;
	HWREG_AVD_AFEC_EnableForceMode(DISABLE);
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

MS_BOOL	Drv_AVD_ForceVideoStandard(enum AVD_VideoStandardType eVideoStandardType,
	struct AVD_RESOURCE_PRIVATE *pResource)
{
	 //Check input
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "Api_AVD_ForceVideoStandard %d\n", eVideoStandardType);
	#endif

	if (eVideoStandardType >= E_VIDEOSTANDARD_MAX)
		MS_ASSERT(FALSE);

	pResource->_u8AutoDetMode = FSC_AUTO_DET_DISABLE;
	pResource->_eForceVideoStandard = eVideoStandardType;
	HWREG_AVD_AFEC_EnableForceMode(ENABLE);
	switch (eVideoStandardType) {
	case E_VIDEOSTANDARD_NTSC_M:
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_NTSC);
		break;
	case E_VIDEOSTANDARD_SECAM:
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_SECAM);
		break;
	case E_VIDEOSTANDARD_NTSC_44:
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_NTSC_443);
		break;
	case E_VIDEOSTANDARD_PAL_M:
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_M);
		break;
	case E_VIDEOSTANDARD_PAL_N:
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_N);
		break;
	case E_VIDEOSTANDARD_PAL_60:
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_60);
		break;
	default:	//E_VIDEOSTANDARD_PAL_BGHI
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL);
		break;
	}

	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	return TRUE;
}

void Drv_AVD_3DCombSpeedup(struct	AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	_Drv_AVD_3DCombSpeedup(pResource);
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_LoadDSP(MS_U8 *pu8VD_DSP, MS_U32 len, struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	HWREG_AVD_VDMCU_LoadDSP(pu8VD_DSP, len);
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_BackPorchWindowPositon(MS_BOOL	bEnable, MS_U8 u8Value)
{
	HWREG_AVD_AFEC_BackPorchWindowPosition(bEnable, u8Value);
}

void Drv_AVD_SetFlag(MS_U32 u32VDPatchFlag, struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	pResource->g_VD_InitData.u32VDPatchFlag = u32VDPatchFlag;
	HWREG_AVD_AFEC_SetPatchFlag(pResource->g_VD_InitData.u32VDPatchFlag);
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_SetStandard_Detection_Flag(MS_U8	u8Value)
{
	HWREG_AVD_AFEC_SET_Standard_Detection_Flag(u8Value);
}

void Drv_AVD_SetRegFromDSP(struct	AVD_RESOURCE_PRIVATE *pResource)
{
	MS_U8 u8Ctl, u8Value;
	MS_U16 u16Htotal, u16CurrentHStart;
	MS_U8  u8update;
	HW_REG_AVD_InputSourceType eSource;

	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
#if	defined(MSTAR_FLAG)
	if (((pResource->_eVDInputSource == E_INPUT_SOURCE_SCART1) ||
		(pResource->_eVDInputSource == E_INPUT_SOURCE_SCART2)) &&
		(pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_SCART_SVIDEO)) {//20121211
		if (MsOS_GetSystemTime() - pResource->_u32SCARTWaitTime > 2000) {
			if (!(HWREG_AVD_AFEC_GetStatus()&0x02) &&
				(pResource->_eVideoSystem != E_VIDEOSTANDARD_SECAM)) {
				if (pResource->_u8SCARTSwitch == 0)
					pResource->_u8SCARTSwitch = 1;
				else
					pResource->_u8SCARTSwitch = 0;

				_Drv_AVD_SCART_Monitor(pResource);
			} else if (pResource->_u8SCARTPrestandard != pResource->_eVideoSystem)
				_Drv_AVD_SCART_Monitor(pResource);
		}
	}

	//20120719 Brian Shift CLK patch for 43.2M water noise
	if (pResource->_bShiftClkFlag == TRUE)
		_Drv_AVD_ShifClk_Monitor(pResource);
#endif
	HWREG_AVD_AFEC_SetRegFromDSP();

	{
		u16Htotal = HWREG_AVD_AFEC_GetHTotal();//SPL_NSPL, H total
		switch (pResource->_eVideoSystem) {
		//2006.06.17 Michael, need to check SRC1  if we use MST6xxx
		case E_VIDEOSTANDARD_NTSC_M: //SIG_NTSC: 910
			u16CurrentHStart = 910;
			break;

		case E_VIDEOSTANDARD_PAL_BGHI: //SIG_PAL: 1135
			u16CurrentHStart = 1135;
			break;

		case E_VIDEOSTANDARD_NTSC_44: // SIG_NTSC_443:	1127
		case E_VIDEOSTANDARD_PAL_60:
			u16CurrentHStart = 1127;
			break;

		case E_VIDEOSTANDARD_PAL_M: // SIG_PAL_M: 909
			u16CurrentHStart = 909;
			break;

		case E_VIDEOSTANDARD_PAL_N:  // SIG_PAL_NC:	917
			u16CurrentHStart = 917;
			break;

		case E_VIDEOSTANDARD_SECAM: // SIG_SECAM: 1097
			u16CurrentHStart = 1097;
			break;

		default:
			// ASSERT
			u16CurrentHStart = 1135;
			break;
		}

		pResource->_u16DataH[2] = pResource->_u16DataH[1];
		pResource->_u16DataH[1] = pResource->_u16DataH[0];
		pResource->_u16DataH[0] = u16Htotal;

		if ((pResource->_u16DataH[2] == pResource->_u16DataH[1]) &&
			(pResource->_u16DataH[1] ==	pResource->_u16DataH[0])) {
			if (pResource->_u8HtotalDebounce > 3)
				pResource->_u16LatchH = pResource->_u16DataH[0];
			else
				pResource->_u8HtotalDebounce++;
		} else
			pResource->_u8HtotalDebounce = 0;

		u16Htotal = pResource->_u16LatchH;

		switch (pResource->_eVideoSystem) {
		case E_VIDEOSTANDARD_PAL_BGHI:// SIG_PAL
		case E_VIDEOSTANDARD_NTSC_44:// SIG_NTSC_443
			u8Value	= 3;
			break;

		case E_VIDEOSTANDARD_PAL_M:	// SIG_PAL_M
		case E_VIDEOSTANDARD_PAL_N:	// SIG_PAL_NC
			u8Value	= 1;
			break;

		default:// NTSC
			u8Value	= 2;
			break;
		}

		if (pResource->g_VD_InitData.u32VDPatchFlag	& AVD_PATCH_NTSC_50)
			if (((HWREG_AVD_AFEC_GetStatus() &
				(VD_FSC_TYPE | VD_VSYNC_50HZ | VD_COLOR_LOCKED)) ==
				(VD_FSC_3579 | VD_VSYNC_50HZ | VD_COLOR_LOCKED)))
				// not NTSC-50
				u8Value = 2;

		u8Ctl = (u16Htotal - u8Value) % 4;
		u8update = 1;
		if (u8Ctl == 2)
			if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_NTSC_50) !=
				AVD_PATCH_NTSC_50)
				u8update = 0;

		if (pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_NTSC_50)
			if (((HWREG_AVD_AFEC_GetStatus() &
				(VD_FSC_TYPE | VD_VSYNC_50HZ | VD_COLOR_LOCKED)) ==
				(VD_FSC_3579 | VD_VSYNC_50HZ | VD_COLOR_LOCKED)))// not NTSC-50
				u16CurrentHStart = 918;

		if (u8update)
			// HWREG_AVD_COMB_SetHtotal( u16Htotal );
			HWREG_AVD_COMB_SetHtotal(u16CurrentHStart);

		if (pResource->g_VD_InitData.u32VDPatchFlag	& AVD_PATCH_FINE_TUNE_FH_DOT) {
			#define FINE_TUNE_FH_DOT_MAX 20
			static MS_U8 u8FhDotDebouncer;

			u8Ctl = HWREG_AVD_AFEC_GetNoiseMag(); //get VD noise magnitude
			if ((((pResource->_eVDInputSource == E_INPUT_SOURCE_ATV) ||
				 (pResource->_eVDInputSource ==	E_INPUT_SOURCE_CVBS1) ||
				 (pResource->_eVDInputSource ==	E_INPUT_SOURCE_CVBS2) ||
				 (pResource->_eVDInputSource == E_INPUT_SOURCE_CVBS3)) ||
				((pResource->_eVDInputSource ==	E_INPUT_SOURCE_SCART1) ||
				 (pResource->_eVDInputSource == E_INPUT_SOURCE_SCART2))) &&
				 ((pResource->_eVideoSystem == E_VIDEOSTANDARD_NTSC_M) ||
				 (pResource->_eVideoSystem == E_VIDEOSTANDARD_PAL_BGHI))) {
				if ((abs(pResource->_u16LatchH-u16CurrentHStart) >= 2) &&
					(u8Ctl <= ((pResource->_eVDInputSource ==
					E_INPUT_SOURCE_ATV) ? 2:1)))
					if (u8FhDotDebouncer < FINE_TUNE_FH_DOT_MAX)
						u8FhDotDebouncer++;
				else if	(u8FhDotDebouncer)
					u8FhDotDebouncer--;
			} else
				u8FhDotDebouncer = 0;

			if (u8FhDotDebouncer >= FINE_TUNE_FH_DOT_MAX) {
				pResource->u32VDPatchFlagStatus	|= AVD_PATCH_FINE_TUNE_FH_DOT;
				HWREG_AVD_COMB_SetNonStandardHtotal(TRUE);
			} else if (!u8FhDotDebouncer) {
				pResource->u32VDPatchFlagStatus	&= ~AVD_PATCH_FINE_TUNE_FH_DOT;
				HWREG_AVD_COMB_SetNonStandardHtotal(FALSE);
			}
			#if	AVD_DRV_DEBUG
			if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
				if (pResource->u32VDPatchFlagStatus & AVD_PATCH_FINE_TUNE_FH_DOT)
					ULOGE("AVD", "_u16LatchH,u16CurrentHStart,
					u8FhDotDebouncer = %d %d %d\n", pResource->_u16LatchH,
					u16CurrentHStart, u8FhDotDebouncer);
			#endif
		}
	}


	// for Color Hysteresis
	if (pResource->_eVDInputSource == E_INPUT_SOURCE_ATV) {
		if ((HWREG_AVD_AFEC_GetStatus() &
			(VD_HSYNC_LOCKED | VD_STATUS_RDY | VD_COLOR_LOCKED)) ==
			(VD_HSYNC_LOCKED | VD_STATUS_RDY))
			HWREG_AVD_AFEC_SetColorKillLevel(
			pResource->g_VD_InitData.u8ColorKillHighBound);
		else
			HWREG_AVD_AFEC_SetColorKillLevel(
			pResource->g_VD_InitData.u8ColorKillLowBound);
	} else
		HWREG_AVD_AFEC_SetColorKillLevel(
		pResource->g_VD_InitData.u8ColorKillLowBound);

	// co-channel
	{
		if (HWREG_AVD_AFEC_GetCoChannelOn())
			HWREG_AVD_VBI_SetTTSigDetSel(TRUE);
		else
			HWREG_AVD_VBI_SetTTSigDetSel(FALSE);
	}

	// For maintain the Hsync reliability and seneitivity
	{
		static MS_U8 u8DebounceCnt;

		if (!(HWREG_AVD_AFEC_GetStatus() & (VD_HSYNC_LOCKED|VD_SYNC_LOCKED)))
			if (u8DebounceCnt < 15)
				u8DebounceCnt++;
		else
			if (u8DebounceCnt > 0)
				u8DebounceCnt--;

		if ((u8DebounceCnt >= 10) || pResource->_u8AutoTuningIsProgress)
			HWREG_AVD_AFEC_EnableBottomAverage(FALSE); //more sensitivity
		else if	(u8DebounceCnt <= 5)
			HWREG_AVD_AFEC_EnableBottomAverage(TRUE);	//more reliability
	}

	{
		MS_U16 u16StandardVtotal;

		if (HWREG_AVD_AFEC_GetStatus() & VD_VSYNC_50HZ)
			u16StandardVtotal = 625;
		else
			u16StandardVtotal = 525;

		// Patch for non-standard V freq issue (3D COMB) should use 2D comb
		if (abs(u16StandardVtotal - HWREG_AVD_AFEC_GetVTotal()) >= 2)
			HWREG_AVD_COMB_Set3dComb(FALSE);
		else {
			#if	defined(STELLAR_FLAG)
			HWREG_AVD_COMB_Set3dComb(TRUE);
			#elif defined(MSTAR_FLAG)
			if (pResource->_b2d3dautoflag == 1)
				HWREG_AVD_COMB_Set3dComb(TRUE);
			else
				HWREG_AVD_COMB_Set3dComb(FALSE);
			#endif
			#ifdef AVD_COMB_3D_MID
			//BY 20090717 enable MID mode after enable 3D comb,
			//if the sequence is wrong, there will be garbage
			if ((pResource->_eVDInputSource	!= E_INPUT_SOURCE_ATV) &&
				(pResource->_eVDInputSource != E_INPUT_SOURCE_SVIDEO1) &&
				(pResource->_eVDInputSource	!= E_INPUT_SOURCE_SVIDEO2))
				if ((pResource->_eVideoSystem == E_VIDEOSTANDARD_PAL_BGHI) ||
					(pResource->_eVideoSystem == E_VIDEOSTANDARD_NTSC_M))
					if (!(pResource->u32VDPatchFlagStatus &
						AVD_PATCH_FINE_TUNE_FH_DOT))
						HWREG_AVD_COMB_Set3dCombMid(ENABLE);
			#endif
		}

		// patch for color stripe function abnormal at non-standard v,  Vestel TG45 issue
		if (abs(u16StandardVtotal - HWREG_AVD_AFEC_GetVTotal()) >= 9)
			HWREG_AVD_AFEC_SetColorStripe(0x31);
		else
			HWREG_AVD_AFEC_SetColorStripe(0x40);

		 //	patch for tune Line freq, jagged line happen
		 eSource = pResource->_eVDInputSource;
		 HWREG_AVD_COMB_Adjustment_Jagged_Line(eSource, u16StandardVtotal);
	}

	// Fix Comb bug
	{
		if (HWREG_AVD_AFEC_GetBurstOn())
			HWREG_AVD_COMB_SetHsyncTolerance(0x00);// Burst On
		else
			HWREG_AVD_COMB_SetHsyncTolerance(0x20);// No Burst (for hsync tolerance)
	}

#if	NO_DEFINE
	if (g_VD_InitData.u32VDPatchFlag & AVD_PATCH_FINE_TUNE_FSC_SHIFT) {
		#define	FINE_TUNE_FSC_SHIFT_CNT_STEP 3
		#define	FINE_TUNE_FSC_SHIFT_CNT_MAX (FINE_TUNE_FSC_SHIFT_CNT_STEP * 7)
		#define	FINE_TUNE_FSC_SHIFT_CNT_THR (FINE_TUNE_FSC_SHIFT_CNT_STEP * 3)
		static MS_U8 u8FscShiftDebounceCnt;

		if ((_eVideoSystem == E_VIDEOSTANDARD_NTSC_M) ||
			(_eVideoSystem == E_VIDEOSTANDARD_PAL_BGHI) ||
			(_eVideoSystem == E_VIDEOSTANDARD_PAL_M) ||
			(_eVideoSystem == E_VIDEOSTANDARD_PAL_N)) {
			if (!HWREG_AVD_COMB_Get3dCombTimingCheck()) //got comb 3D unlocked
				if (u8FscShiftDebounceCnt < FINE_TUNE_FSC_SHIFT_CNT_MAX)
					u8FscShiftDebounceCnt += FINE_TUNE_FSC_SHIFT_CNT_STEP;
			else
				if (u8FscShiftDebounceCnt)
					u8FscShiftDebounceCnt--;
		} else
			u8FscShiftDebounceCnt = 0;

		if (u8FscShiftDebounceCnt >= FINE_TUNE_FSC_SHIFT_CNT_THR) {
			u32VDPatchFlagStatus |=	AVD_PATCH_FINE_TUNE_FSC_SHIFT;
			HWREG_AVD_COMB_SetNonStandardFSC(TRUE, TRUE);
		} else if (!u8FscShiftDebounceCnt) {
			u32VDPatchFlagStatus &=	~AVD_PATCH_FINE_TUNE_FSC_SHIFT;
			HWREG_AVD_COMB_SetNonStandardFSC(
				(_eVideoSystem == E_VIDEOSTANDARD_PAL_BGHI), FALSE);
		}
	}
#endif

	{ //burst
		MS_BOOL bBurstOn;
		static MS_BOOL bPrevBurstOn = FALSE;

		bBurstOn = HWREG_AVD_AFEC_GetBurstOn();
		if (bBurstOn)
			if (!bPrevBurstOn)
				_Drv_AVD_3DCombSpeedup(pResource);
		#ifdef LOCK3DSPEEDUP_PERFORMANCE_1
		else
			if (!pResource->_u8Comb10Bit3Flag)
				HWREG_AVD_COMB_Set3dCombSpeed(0x14, 0x10, 0x88);
		#endif
		bPrevBurstOn = bBurstOn;

		if (pResource->_u8Comb10Bit3Flag) {
			#if	defined(STELLAR_FLAG)
			if ((pResource->_u8Comb10Bit3Flag & (BIT(0))) &&
				(MsOS_GetSystemTime() - pResource->_u32VideoSystemTimer > 1000))
			#elif defined(MSTAR_FLAG)
			if ((pResource->_u8Comb10Bit3Flag & (BIT(0))) &&
				(MsOS_GetSystemTime() - pResource->_u32VideoSystemTimer > 3000))
			#endif
			{
				//MDrv_WriteByteMask(BK_COMB_10,	0, BIT3);
				HWREG_AVD_COMB_Set3dCombSpeed(0x50, 0x20, 0x88);
				#if	ADJUST_CHANNEL_CHANGE_HSYNC
				HWREG_AVD_AFEC_SetSwingLimit(
				pResource->g_VD_InitData.u8SwingThreshold);
				// more reliability
				HWREG_AVD_AFEC_EnableBottomAverage(ENABLE);
				#endif
				pResource->_u8Comb10Bit3Flag &= (~(BIT(0)));
				#if	AVD_DRV_DEBUG
				if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
					ULOGE("AVD", "Drv_AVD_3DCombSpeedup Disable\n");
				#endif
			}
			#if	defined(STELLAR_FLAG)
			if ((pResource->_u8Comb10Bit3Flag&(BIT(1))) &&
				(MsOS_GetSystemTime() - pResource->_u32VideoSystemTimer > 500))
			#elif defined(MSTAR_FLAG)
			if ((pResource->_u8Comb10Bit3Flag&(BIT(1))) &&
				(MsOS_GetSystemTime() - pResource->_u32VideoSystemTimer	 > 2000))
			#endif
			{
				if (!(pResource->_u8AfecD4Factory & BIT(4)))
					HWREG_AVD_AFEC_EnableVBIDPLSpeedup(DISABLE);
				pResource->_u8Comb10Bit3Flag &= (~(BIT(1)));
			}
			#if	defined(STELLAR_FLAG)
			if ((pResource->_u8Comb10Bit3Flag & (BIT(2))) &&
				(MsOS_GetSystemTime() - pResource->_u32VideoSystemTimer > 300))
			#elif defined(MSTAR_FLAG)
			if ((pResource->_u8Comb10Bit3Flag & (BIT(2))) &&
				(MsOS_GetSystemTime() - pResource->_u32VideoSystemTimer > 400))
			#endif
			{
				HWREG_AVD_AFEC_SET_DPL_K1(
					HWREG_AVD_AFEC_GET_DPL_K1()&(~BIT(7)));
				pResource->_u8Comb10Bit3Flag &= (~(BIT(2)));
			}
		}
	}

	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		if (pResource->u32PreVDPatchFlagStatus != pResource->u32VDPatchFlagStatus) {
			ULOGE("AVD", "u32VDPatchFlagStatus %x\n",
				(unsigned int)pResource->u32VDPatchFlagStatus);
			pResource->u32PreVDPatchFlagStatus = pResource->u32VDPatchFlagStatus;
		}
	#endif
	#if	NO_DEFINE
	if (pResource->g_VD_InitData.u32VDPatchFlag	& AVD_PATCH_NON_STANDARD_VTOTAL)
		_Drv_AVD_SyncRangeHandler(pResource);
	#endif

	#if	defined(MSTAR_FLAG)
	if (pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_FINE_TUNE_STILL_IMAGE)
		_Drv_AVD_COMB_StillImage(pResource);
	#endif
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

MS_BOOL	Drv_AVD_SetInput(enum	AVD_InputSourceType	eSource, MS_U8 u8ScartFB,
	struct AVD_RESOURCE_PRIVATE *pResource) //SCART_FB eFB)
{
	#if	defined(MSTAR_FLAG)
	HW_REG_VD_HSYNC_SENSITIVITY	eVDHsyncSensitivityforAV, eVDHsyncSensitivityforNormal;
	#endif
	//Basic	Input	Checking
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "Api_AVD_SetInput %d\n", eSource);
	#endif
	if (eSource	>= E_INPUT_SOURCE_MAX)
		MS_ASSERT(FALSE);

	eSource = (enum AVD_InputSourceType)(((MS_U8)eSource) & 0x0F);
	pResource->_eVDInputSource = eSource;
	#if	defined(MSTAR_STR)
	// store source type information for STR function
	eSource_Str = eSource;
	u8ScartFB_Str = u8ScartFB;
	eDemodType_Str = pResource->g_VD_InitData.eDemodType;
	u32XTAL_Clock_Str = pResource->g_VD_InitData.u32XTAL_Clock;
	#endif
	HWREG_AVD_AFEC_SetInput(eSource, u8ScartFB, pResource->g_VD_InitData.eDemodType,
	pResource->g_VD_InitData.u32XTAL_Clock);
	//HWREG_AVD_COMB_SetMemoryRequest(ENABLE);
	#if	defined(MSTAR_FLAG)
	///Brian A1	VIF	and	ADC	different	AGC	mapping
	 #if (TWO_VD_DSP_CODE)
// weicheng 20121003
/*
 *		if(eSource == E_INPUT_SOURCE_ATV)
 *			HWREG_AVD_SetDSPCodeType(AVD_DSP_CODE_TYPE_VIF);
 *		else
 *			HWREG_AVD_SetDSPCodeType(AVD_DSP_CODE_TYPE_ADC);
 */
	 _Drv_AVD_VDMCU_Init(pResource);
	 #endif

	if (pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_CVBS_NEGATIVESIG) {
		//20130115 dia.chiu for TCL  CVBS negative signal unlock
		if ((eSource	== E_INPUT_SOURCE_CVBS1) ||
			(eSource == E_INPUT_SOURCE_CVBS2) ||
			(eSource == E_INPUT_SOURCE_CVBS3)) {
			eVDHsyncSensitivityforAV.u8DetectWinAfterLock = 0x01;
			eVDHsyncSensitivityforAV.u8DetectWinBeforeLock = 0x01;
			eVDHsyncSensitivityforAV.u8CNTRFailBeforeLock = 0x01;
			eVDHsyncSensitivityforAV.u8CNTRSyncBeforeLock = 0x3E;
			eVDHsyncSensitivityforAV.u8CNTRSyncAfterLock = 0x00;
			HWREG_AVD_AFEC_SetHsyncSensitivity(eVDHsyncSensitivityforAV);
		} else {
			eVDHsyncSensitivityforNormal.u8DetectWinAfterLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8DetectWinAfterLock;
			eVDHsyncSensitivityforNormal.u8DetectWinBeforeLock	=
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8DetectWinBeforeLock;
			eVDHsyncSensitivityforNormal.u8CNTRFailBeforeLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8CNTRFailBeforeLock;
			eVDHsyncSensitivityforNormal.u8CNTRSyncBeforeLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8CNTRSyncBeforeLock;
			eVDHsyncSensitivityforNormal.u8CNTRSyncAfterLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8CNTRSyncAfterLock;
			HWREG_AVD_AFEC_SetHsyncSensitivity(eVDHsyncSensitivityforNormal);
		}
	}
#endif
	// set gain
	if (eSource	== E_INPUT_SOURCE_ATV) {
		if (pResource->g_VD_InitData.bRFGainSel == VD_USE_FIX_GAIN) {
			HWREG_AVD_AFEC_AGCSetMode(AVD_AGC_DISABLE);

			if (pResource->g_VD_InitData.eDemodType == DEMODE_MSTAR_VIF) {
				#if	defined(STELLAR_FLAG)
				HWREG_AVD_AFEC_AGCSetCoarseGain(VD_AGC_COARSE_GAIN_X_1);
				#elif defined(MSTAR_FLAG)
				HWREG_AVD_AFEC_AGCSetCoarseGain(VD_AGC_COARSE_GAIN);
				#endif
				HWREG_AVD_AFEC_AGCSetFineGain(pResource->g_VD_InitData.u8RFGain);
			} else {
				#if	defined(STELLAR_FLAG)
				HWREG_AVD_AFEC_AGCSetCoarseGain(VD_AGC_COARSE_GAIN_X_13);
				#elif defined(MSTAR_FLAG)
				HWREG_AVD_AFEC_AGCSetCoarseGain(VD_AGC_COARSE_GAIN);
				#endif
				HWREG_AVD_AFEC_AGCSetFineGain(pResource->g_VD_InitData.u8RFGain);
			}
		} else
			HWREG_AVD_AFEC_AGCSetMode(AVD_AGC_ENABLE);
	} else {
		if (pResource->g_VD_InitData.bAVGainSel == VD_USE_FIX_GAIN) {
			HWREG_AVD_AFEC_AGCSetMode(AVD_AGC_DISABLE);
			#if	defined(STELLAR_FLAG)
			HWREG_AVD_AFEC_AGCSetCoarseGain(VD_AGC_COARSE_GAIN_X_13);
			#elif defined(MSTAR_FLAG)
			HWREG_AVD_AFEC_AGCSetCoarseGain(VD_AGC_COARSE_GAIN);
			#endif
			HWREG_AVD_AFEC_AGCSetFineGain(pResource->g_VD_InitData.u8AVGain);
		} else
			HWREG_AVD_AFEC_AGCSetMode(AVD_AGC_ENABLE);
		#if	defined(MSTAR_FLAG)
		//20120719 Brian Leave ATV source, disable the shift clk monitor
		pResource->_bShiftClkFlag = FALSE;
		#endif
	}

	// VD	MCU	Reset
	// BY	20090410,	move function	body here	 Api_AVD_McuReset();
	#if	T3_LOAD_CODE //weicheng	20121003
		#if	defined(STELLAR_FLAG)
			if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_SAMSUNG_DSP) &&
				((pResource->g_VD_InitData.u32VDPatchFlag &
				AVD_PATCH_INTERNAL_DEMOD) == 0))
				//use external demodulator
				HWREG_AVD_AFEC_McuReset();
			else
				// must reload code for T3
				_Drv_AVD_VDMCU_Init();
		 #elif defined(MSTAR_FLAG)
		 //must reload code for T3
		 _Drv_AVD_VDMCU_Init();
		 #endif
	#else
	HWREG_AVD_AFEC_McuReset();
	#endif
	HWREG_AVD_AFEC_SetPatchFlag(pResource->g_VD_InitData.u32VDPatchFlag);
	// Forced	to PAL mode
	pResource->_eLastStandard	=	E_VIDEOSTANDARD_NOTSTANDARD;
	pResource->_eVideoSystem = E_VIDEOSTANDARD_PAL_BGHI;
	pResource->_u16CurVDStatus = 0;
	HWREG_AVD_COMB_SetMemoryRequest(ENABLE);
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	return TRUE;
}

MS_BOOL	Drv_AVD_SetVideoStandard(enum AVD_VideoStandardType eStandard, MS_BOOL bIsInAutoTuning,
	struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "Api_AVD_SetVideoStandard %d\n", eStandard);
	#endif

	pResource->_eVideoSystem = eStandard;

	 //Check input
	if (eStandard >= E_VIDEOSTANDARD_MAX)
		MS_ASSERT(FALSE);
	#if	defined(STELLAR_FLAG)
	#if	ADJUST_NTSC_BURST_WINDOW
	if (eStandard == E_VIDEOSTANDARD_NTSC_M && pResource->_eVDInputSource ==
		E_INPUT_SOURCE_ATV)
		HWREG_AVD_SetBurstWinStart(0x6E);
	else
		HWREG_AVD_SetBurstWinStart(0x62);
	#endif
	#endif
	switch (eStandard) {
	case E_VIDEOSTANDARD_NTSC_44:
		if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_HTOTAL_MASK) ==
			AVD_PATCH_HTOTAL_USER)
			HWREG_AVD_AFEC_SetHTotal(pResource->_u16Htt_UserMD);
		else
			HWREG_AVD_AFEC_SetHTotal(
			_u16HtotalNTSC_443[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8] * (VD_USE_FB	+ 1));
		HWREG_AVD_AFEC_SetBT656Width(0xB4);
		pResource->_u16LatchH = 1127L;
		pResource->_u16CurVDStatus = VD_FSC_4433 | VD_STATUS_RDY;
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_NTSC_443);
		break;
	case E_VIDEOSTANDARD_PAL_60:// 1127
		if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_HTOTAL_MASK) ==
			AVD_PATCH_HTOTAL_USER)
			HWREG_AVD_AFEC_SetHTotal(pResource->_u16Htt_UserMD);
		else
			HWREG_AVD_AFEC_SetHTotal(
			_u16HtotalPAL_60[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8] * (VD_USE_FB + 1));
		HWREG_AVD_AFEC_SetBT656Width(0xB4);
		pResource->_u16LatchH = 1127L;
		pResource->_u16CurVDStatus = VD_PAL_SWITCH | VD_FSC_4433 | VD_STATUS_RDY;
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_60);
		break;
	case E_VIDEOSTANDARD_SECAM:
		if (bIsInAutoTuning) {
			if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_HTOTAL_MASK) ==
				AVD_PATCH_HTOTAL_USER)
				HWREG_AVD_AFEC_SetHTotal(pResource->_u16Htt_UserMD);
			else
				HWREG_AVD_AFEC_SetHTotal(
				_u16HtotalPAL[(pResource->g_VD_InitData.u32VDPatchFlag &
				AVD_PATCH_HTOTAL_MASK)>>8] * (VD_USE_FB	+ 1));
		} else {
			if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_HTOTAL_MASK) ==
				AVD_PATCH_HTOTAL_USER)
				HWREG_AVD_AFEC_SetHTotal(pResource->_u16Htt_UserMD);
			else
				HWREG_AVD_AFEC_SetHTotal(
				_u16HtotalSECAM[(pResource->g_VD_InitData.u32VDPatchFlag &
				AVD_PATCH_HTOTAL_MASK)>>8] * (VD_USE_FB + 1));
		}

		HWREG_AVD_AFEC_SetBT656Width(0xB2);
		pResource->_u16LatchH = 1097L;
		pResource->_u16CurVDStatus = (VD_VSYNC_50HZ	| VD_FSC_4285 | VD_STATUS_RDY);
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_SECAM);
		break;

	case E_VIDEOSTANDARD_PAL_M:	// 909
		if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_HTOTAL_MASK) ==
			AVD_PATCH_HTOTAL_USER)
			HWREG_AVD_AFEC_SetHTotal(pResource->_u16Htt_UserMD);
		else
			HWREG_AVD_AFEC_SetHTotal(
			_u16HtotalPAL_M[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8] * (VD_USE_FB + 1));
		HWREG_AVD_AFEC_SetBT656Width(0x8E);
		pResource->_u16LatchH = 909L;
		pResource->_u16CurVDStatus = (VD_PAL_SWITCH	| VD_FSC_3575 | VD_STATUS_RDY);
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_M);
		break;

	case E_VIDEOSTANDARD_PAL_N:	// 917
		if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_HTOTAL_MASK) ==
			AVD_PATCH_HTOTAL_USER)
			HWREG_AVD_AFEC_SetHTotal(pResource->_u16Htt_UserMD);
		else
			HWREG_AVD_AFEC_SetHTotal(
			_u16HtotalPAL_NC[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8] * (VD_USE_FB + 1));
		HWREG_AVD_AFEC_SetBT656Width(0x93);
		pResource->_u16LatchH = 917L;
		pResource->_u16CurVDStatus =
			(VD_VSYNC_50HZ | VD_PAL_SWITCH | VD_FSC_3582 | VD_STATUS_RDY);
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_N);
		break;

	case E_VIDEOSTANDARD_NTSC_M: //	910
		if ((pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK) == AVD_PATCH_HTOTAL_USER)
			HWREG_AVD_AFEC_SetHTotal(pResource->_u16Htt_UserMD);
		else
			HWREG_AVD_AFEC_SetHTotal(
			_u16HtotalNTSC[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8] * (VD_USE_FB + 1));
		HWREG_AVD_AFEC_SetBT656Width(0x6D);
		pResource->_u16LatchH = 910L;
		pResource->_u16CurVDStatus = (VD_FSC_3579 | VD_STATUS_RDY);
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_NTSC);
		break;
	default: //	case E_VIDEOSTANDARD_PAL_BGHI:	// 1135
		if ((pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_HTOTAL_MASK) ==
			AVD_PATCH_HTOTAL_USER)
			HWREG_AVD_AFEC_SetHTotal(pResource->_u16Htt_UserMD);
		else
			HWREG_AVD_AFEC_SetHTotal(
			_u16HtotalPAL[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8] * (VD_USE_FB + 1));
		HWREG_AVD_AFEC_SetBT656Width(0xB6);
		pResource->_u16LatchH = 1135L;
		pResource->_u16CurVDStatus =
			(VD_VSYNC_50HZ | VD_PAL_SWITCH | VD_FSC_4433 | VD_STATUS_RDY);
		HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL);
		break;
	}

	if ((pResource->_eVDInputSource	== E_INPUT_SOURCE_SVIDEO1) ||
		(pResource->_eVDInputSource == E_INPUT_SOURCE_SVIDEO2)) {
		if (eStandard == E_VIDEOSTANDARD_NTSC_M)
			HWREG_AVD_COMB_SetYCPipe(0x20);
		else //	SIG_NTSC_443,	SIG_PAL, SIG_PAL_M, SIG_PAL_NC, SIG_SECAM
			HWREG_AVD_COMB_SetYCPipe(0x30);

		switch (eStandard) {
		case E_VIDEOSTANDARD_PAL_M:
		case E_VIDEOSTANDARD_PAL_N:
			HWREG_AVD_COMB_SetCbCrInverse(0x04);
			break;

		case E_VIDEOSTANDARD_NTSC_44:
			HWREG_AVD_COMB_SetCbCrInverse(0x0C);
			break;

		case E_VIDEOSTANDARD_PAL_60:
			HWREG_AVD_COMB_SetCbCrInverse(0x08);
			break;

		case E_VIDEOSTANDARD_NTSC_M:
		case E_VIDEOSTANDARD_SECAM:
			HWREG_AVD_COMB_SetCbCrInverse(0x00);
			break;

		case E_VIDEOSTANDARD_PAL_BGHI:
		default:
			HWREG_AVD_COMB_SetCbCrInverse(0x08);
			break;
		}
	} else {
		#if	defined(STELLAR_FLAG)
		HWREG_AVD_COMB_SetYCPipe(0x20);
		HWREG_AVD_COMB_SetCbCrInverse(0x00);
		#elif defined(MSTAR_FLAG)
		if (pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_SCART_SVIDEO) {
			if ((pResource->_eVDInputSource != E_INPUT_SOURCE_SCART1) &&
				(pResource->_eVDInputSource != E_INPUT_SOURCE_SCART2)) {
				HWREG_AVD_COMB_SetYCPipe(0x20);
				HWREG_AVD_COMB_SetCbCrInverse(0x00);
			}
		} else {
			HWREG_AVD_COMB_SetYCPipe(0x20);
			HWREG_AVD_COMB_SetCbCrInverse(0x00);
		}
		#endif
	}

	if (pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_NTSC_50) {
		if ((HWREG_AVD_AFEC_GetStatus() & (VD_FSC_TYPE|VD_VSYNC_50HZ|VD_COLOR_LOCKED)) ==
			(VD_FSC_3579|VD_VSYNC_50HZ|VD_COLOR_LOCKED))// NTSC-50
			HWREG_AVD_COMB_SetVerticalTimingDetectMode(0x02);
		else
			HWREG_AVD_COMB_SetVerticalTimingDetectMode(0x00);
	}

	if (eStandard == E_VIDEOSTANDARD_NTSC_44) {
		HWREG_AVD_COMB_SetLineBufferMode(0x06);
		HWREG_AVD_COMB_SetHtotal(0x467);
	} else if ((pResource->g_VD_InitData.u32VDPatchFlag	 & AVD_PATCH_NTSC_50) &&
	((HWREG_AVD_AFEC_GetStatus() & (VD_FSC_TYPE|VD_VSYNC_50HZ|VD_COLOR_LOCKED)) ==
	(VD_FSC_3579|VD_VSYNC_50HZ|VD_COLOR_LOCKED))) {	// NTSC-50

		HWREG_AVD_COMB_SetLineBufferMode(0x06);
		HWREG_AVD_COMB_SetHtotal(0x396);
	} else {
		HWREG_AVD_COMB_SetLineBufferMode(0x07);
		HWREG_AVD_COMB_SetHtotal(0x38D);
	}

	if (eStandard == E_VIDEOSTANDARD_SECAM) {
		HWREG_AVD_VBI_SetVPSPhaseAcc(0x9A6D);
		#if	defined(STELLAR_FLAG)
			if (pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_DSPCODE_0x25)
				HWREG_AVD_SET_SECAM_CB_CR_LPF(
				HWREG_AVD_GET_SECAM_CB_CR_LPF() | (BIT_(4)));
		#endif
	} else
		HWREG_AVD_VBI_SetVPSPhaseAcc(0x018C);

	if (HWREG_AVD_AFEC_GetBurstOn())
		_Drv_AVD_3DCombSpeedup(pResource);

#if	defined(STELLAR_FLAG)
	if (pResource->g_VD_InitData.u32VDPatchFlag	& AVD_PATCH_FINE_TUNE_COMB_F2) {
		if (((pResource->_eVDInputSource	== E_INPUT_SOURCE_CVBS1) ||
			(pResource->_eVDInputSource == E_INPUT_SOURCE_CVBS2) ||
			(pResource->_eVDInputSource == E_INPUT_SOURCE_CVBS3)) &&
			(eStandard == E_VIDEOSTANDARD_NTSC_M))
			HWREG_AVD_COMB_SetF2(0x57);
		else
			HWREG_AVD_COMB_SetF2(0x47);
	}

	if (pResource->g_VD_InitData.u32VDPatchFlag	& AVD_PATCH_FINE_TUNE_3D_COMB)
		HWREG_AVD_COMB_Set3dFineTune((eStandard == E_VIDEOSTANDARD_PAL_BGHI));

	switch (eStandard) {
	case E_VIDEOSTANDARD_PAL_BGHI:
		HWREG_AVD_COMB_Set3dDetectionTolerance(0x06);
		break;
	case E_VIDEOSTANDARD_PAL_M:
		HWREG_AVD_COMB_Set3dDetectionTolerance(0x04);
		break;
	default:
		HWREG_AVD_COMB_Set3dDetectionTolerance(0x05);
		break;
	}
#endif

	if (pResource->g_VD_InitData.u32VDPatchFlag	& AVD_PATCH_NON_STANDARD_VTOTAL) {
		//bug's life, for Eris ATT, James.Lu, 20080327
#if	defined(MSTAR_FLAG)
#ifndef	MSOS_TYPE_LINUX
		if (((pResource->_eVDInputSource ==	E_INPUT_SOURCE_CVBS1) ||
			(pResource->_eVDInputSource	== E_INPUT_SOURCE_CVBS2) ||
			(pResource->_eVDInputSource	== E_INPUT_SOURCE_CVBS3) ||
			(pResource->_eVDInputSource == E_INPUT_SOURCE_ATV)) &&
			(eStandard != E_VIDEOSTANDARD_NOTSTANDARD)) {
			MS_U16 u16Vtotal = HWREG_AVD_AFEC_GetVTotal();

			if ((u16Vtotal < 675) && (u16Vtotal > 570))
				pResource->u16PreVtotal	= 625;
			else
				pResource->u16PreVtotal	= 525;
		} else
			pResource->u16PreVtotal	=	0;
#endif
#endif
	}
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	return TRUE;
}

void Drv_AVD_SetChannelChange(struct AVD_RESOURCE_PRIVATE	*pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "Drv_AVD_SetChannelChange\n");
	#endif
	HWREG_AVD_AFEC_SetChannelChange();
	//Api_AVD_3DCombSpeedup();
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_SetHsyncDetectionForTuning(MS_BOOL	bEnable, struct	AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	HW_REG_VD_HSYNC_SENSITIVITY	eVDHsyncSensitivityforTuning, eVDHsyncSensitivityforNormal;
	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "Api_AVD_SetHsyncDetectionForTuning %d\n", bEnable);
	#endif

	if (bEnable) {// for Auto Scan Mode
		eVDHsyncSensitivityforTuning.u8DetectWinAfterLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityTuning.u8DetectWinAfterLock;
		eVDHsyncSensitivityforTuning.u8DetectWinBeforeLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityTuning.u8DetectWinBeforeLock;
		eVDHsyncSensitivityforTuning.u8CNTRFailBeforeLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityTuning.u8CNTRFailBeforeLock;
		eVDHsyncSensitivityforTuning.u8CNTRSyncBeforeLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityTuning.u8CNTRSyncBeforeLock;
		eVDHsyncSensitivityforTuning.u8CNTRSyncAfterLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityTuning.u8CNTRSyncAfterLock;
		HWREG_AVD_AFEC_SetHsyncSensitivity(eVDHsyncSensitivityforTuning);
		HWREG_AVD_AFEC_SetSwingLimit(0);
		HWREG_AVD_AFEC_EnableBottomAverage(DISABLE); //more sensitivity
		pResource->_u8AutoTuningIsProgress = TRUE;
	} else {//for Normal Mode
		eVDHsyncSensitivityforNormal.u8DetectWinAfterLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8DetectWinAfterLock;
		eVDHsyncSensitivityforNormal.u8DetectWinBeforeLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8DetectWinBeforeLock;
		eVDHsyncSensitivityforNormal.u8CNTRFailBeforeLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8CNTRFailBeforeLock;
		eVDHsyncSensitivityforNormal.u8CNTRSyncBeforeLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8CNTRSyncBeforeLock;
		eVDHsyncSensitivityforNormal.u8CNTRSyncAfterLock =
			pResource->g_VD_InitData.eVDHsyncSensitivityNormal.u8CNTRSyncAfterLock;
		HWREG_AVD_AFEC_SetHsyncSensitivity(eVDHsyncSensitivityforNormal);
		HWREG_AVD_AFEC_SetSwingLimit(pResource->g_VD_InitData.u8SwingThreshold);
		HWREG_AVD_AFEC_EnableBottomAverage(ENABLE); // more reliability
		pResource->_u8AutoTuningIsProgress = FALSE;
	}
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_Set3dComb(MS_BOOL bEnable,	struct AVD_RESOURCE_PRIVATE	*pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	// it's ok to enable/disable 3D comb for svideo input
	#if	defined(STELLAR_FLAG)
	HWREG_AVD_COMB_Set3dComb(bEnable);
	#elif defined(MSTAR_FLAG)
	if (bEnable)
		pResource->_b2d3dautoflag = 1;
	else
		pResource->_b2d3dautoflag = 0;
	#endif
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_SetShiftClk(MS_BOOL bEnable, enum AVD_ATV_CLK_TYPE eShiftMode,
	struct AVD_RESOURCE_PRIVATE *pResource)
{
	if ((pResource->_eVDInputSource == E_INPUT_SOURCE_ATV) &&
		(pResource->g_VD_InitData.eDemodType == DEMODE_MSTAR_VIF)) {
		if (pResource->_u8AutoTuningIsProgress == TRUE)
			eShiftMode = E_ATV_CLK_ORIGIN_43P2MHZ;
		if ((bEnable == FALSE) || (eShiftMode == E_ATV_CLK_ORIGIN_43P2MHZ)) {
			pResource->_bShiftClkFlag = FALSE;
			HWREG_AVD_ShiftClk(eShiftMode, E_VIDEOSTANDARD_AUTO,
				pResource->g_VD_InitData.u32XTAL_Clock);
		} else
			pResource->_bShiftClkFlag = bEnable;
		pResource->gShiftMode = eShiftMode;
	} else
		pResource->_bShiftClkFlag = FALSE;
}

void Drv_AVD_SetFreerunPLL(enum	AVD_VideoFreq eVideoFreq, struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	if (eVideoFreq == E_VIDEO_FQ_60Hz)
		HWREG_AVD_COMB_SetHtotal((MS_U16)0x38E);
	else if (eVideoFreq == E_VIDEO_FQ_50Hz)
		HWREG_AVD_COMB_SetHtotal((MS_U16)0x46F);
	else if (eVideoFreq == E_VIDEO_FQ_NOSIGNAL)
		//Do nothing
		ULOGE("AVD", "[%s]VIDEO FQ NO SIGNAL\n", __func__);
	else
		MS_ASSERT(FALSE);
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_SetFreerunFreq(enum AVD_FreeRunFreq eFreerunfreq,
	struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	if (eFreerunfreq >=	E_FREERUN_FQ_MAX)
		MS_ASSERT(FALSE);

	switch (eFreerunfreq) {
	case E_FREERUN_FQ_AUTO:
		HWREG_AVD_AFEC_SetVtotal(0x00);
		break;

	case E_FREERUN_FQ_50Hz:
		HWREG_AVD_AFEC_SetVtotal(0x01);
		break;

	case E_FREERUN_FQ_60Hz:
		HWREG_AVD_AFEC_SetVtotal(0x03);
		break;

	default:
		break;
	}
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_SetFactoryPara(enum AVD_Factory_Para FactoryPara, MS_U8 u8Value,
	struct AVD_RESOURCE_PRIVATE *pResource)
{
	switch (FactoryPara) {
	case E_FACTORY_PARA_AFEC_D4:
		HWREG_AVD_AFEC_SET_PACTH_D4(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_D8:
		HWREG_AVD_AFEC_SET_PACTH_D8(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_D5_BIT2:
		HWREG_AVD_AFEC_SET_FORCE_K1K2(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_D9_BIT0:
		HWREG_AVD_AFEC_SET_RF_LOW_HSYNC_DETECTION(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_A0:
		HWREG_AVD_AFEC_SET_DPL_K1(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_A1:
		HWREG_AVD_AFEC_SET_DPL_K2(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_66_BIT67:
		HWREG_AVD_AFEC_SET_ENABLE_SLICE_LEVEL(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_6E_BIT7654:
		HWREG_AVD_AFEC_SET_V_SLICE_LEVEL(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_6E_BIT3210:
		HWREG_AVD_AFEC_SET_H_SLICE_LEVEL(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_43:
		HWREG_AVD_AFEC_SET_FIX_GAIN(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_44:
		HWREG_AVD_AFEC_SET_AGC_UPDATE(u8Value);
		HWREG_AVD_AFEC_AGCSetMode(AVD_AGC_DISABLE);
		break;
	case E_FACTORY_PARA_AFEC_CB:
		HWREG_AVD_AFEC_SET_Standard_Detection_Flag(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_CF_BIT2:
		HWREG_AVD_AFEC_SET_RF_MODE(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_D5_BIT3:
		HWREG_AVD_AFEC_SET_JILIN_PATCH(u8Value);
		break;
	case E_FACTORY_PARA_AFEC_D7_HIGH:
		pResource->g_VD_InitData.u8ColorKillHighBound = u8Value;
		break;
	case E_FACTORY_PARA_AFEC_D7_LOW:
		pResource->g_VD_InitData.u8ColorKillLowBound = u8Value;
		break;
	default:
		ULOGE("AVD", "ERROR!!! Invalid Factory Parameter!\n");
		break;
	}
}

MS_BOOL	Drv_AVD_SetDbgLevel(enum AVD_DbgLv u8DbgLevel, struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	#if	AVD_DRV_DEBUG
	pResource->_u8AVDDbgLevel = u8DbgLevel;
	#endif
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	return TRUE;
}

void Drv_AVD_SetPQFineTune(struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	HWREG_AVD_SetPQFineTune();
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_Set3dCombSpeed(MS_U8 u8COMB57, MS_U8	u8COMB58,
	struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	pResource->_u8Comb57 = u8COMB57;
	pResource->_u8Comb58 = u8COMB58;
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}

void Drv_AVD_SetStillImageParam(struct AVD_Still_Image_Param param,
	struct AVD_RESOURCE_PRIVATE	*pResource)
{
	pResource->g_stillImageParam.bMessageOn	= param.bMessageOn;

	pResource->g_stillImageParam.u8Str1_COMB37 = param.u8Str1_COMB37;
	pResource->g_stillImageParam.u8Str1_COMB38 = param.u8Str1_COMB38;
	pResource->g_stillImageParam.u8Str1_COMB7C = param.u8Str1_COMB7C;
	pResource->g_stillImageParam.u8Str1_COMBED = param.u8Str1_COMBED;

	pResource->g_stillImageParam.u8Str2_COMB37 = param.u8Str2_COMB37;
	pResource->g_stillImageParam.u8Str2_COMB38 = param.u8Str2_COMB38;
	pResource->g_stillImageParam.u8Str2_COMB7C = param.u8Str2_COMB7C;
	pResource->g_stillImageParam.u8Str2_COMBED = param.u8Str2_COMBED;

	pResource->g_stillImageParam.u8Str3_COMB37 = param.u8Str3_COMB37;
	pResource->g_stillImageParam.u8Str3_COMB38 = param.u8Str3_COMB38;
	pResource->g_stillImageParam.u8Str3_COMB7C = param.u8Str3_COMB7C;
	pResource->g_stillImageParam.u8Str3_COMBED = param.u8Str3_COMBED;

	pResource->g_stillImageParam.u8Threshold1 = param.u8Threshold1;
	pResource->g_stillImageParam.u8Threshold2 = param.u8Threshold2;
	pResource->g_stillImageParam.u8Threshold3 = param.u8Threshold3;
	pResource->g_stillImageParam.u8Threshold4 = param.u8Threshold4;
}

void Drv_AVD_Set2D3DPatchOnOff(MS_BOOL bEnable)
{
	HWREG_AVD_Set2D3DPatchOnOff(bEnable);
	ULOGE("AVD", "Patch %s \r\n", (bEnable)?"On" : "Off");
}

MS_U8 Drv_AVD_GetStandard_Detection_Flag(void)
{
	return HWREG_AVD_AFEC_GET_Standard_Detection_Flag();
}

MS_U16 Drv_AVD_GetStatus(void)
{
	return HWREG_AVD_AFEC_GetStatus();
}

MS_U8 Drv_AVD_GetNoiseMag(void)
{
	return HWREG_AVD_AFEC_GetNoiseMag();
}

MS_U16 Drv_AVD_GetVTotal(void)
{
	return HWREG_AVD_AFEC_GetVTotal();
}

enum AVD_VideoStandardType Drv_AVD_GetStandardDetection(MS_U16 *u16LatchStatus,
	struct AVD_RESOURCE_PRIVATE *pResource)
{
	MS_U16 u16Status;
	enum AVD_VideoStandardType eVideoStandard;

	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	*u16LatchStatus	=	u16Status	=	HWREG_AVD_AFEC_GetStatus();


	if (!IS_BITS_SET(u16Status, VD_HSYNC_LOCKED|VD_SYNC_LOCKED|VD_STATUS_RDY)) {
		pResource->_eLastStandard = E_VIDEOSTANDARD_NOTSTANDARD;
		AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
		return E_VIDEOSTANDARD_NOTSTANDARD;
	}


	if (pResource->_u8AutoDetMode == FSC_AUTO_DET_DISABLE)
		eVideoStandard = pResource->_eForceVideoStandard;
	else {
		if (u16Status & VD_BURST_ON) {
			switch (u16Status & VD_FSC_TYPE) {
			case VD_FSC_4433:
				// (FSC_MODE_PAL << 5):
				HWREG_AVD_ADC_SetGMC(0x06);
				#if	(!TEST_VD_DSP)
				//50 Hz
				if (u16Status & VD_VSYNC_50HZ)
				#endif
				{
					eVideoStandard = E_VIDEOSTANDARD_PAL_BGHI;
				}
				#if (!TEST_VD_DSP)
				else {
					//60MHz
					if (u16Status & VD_PAL_SWITCH)
						//or vsdNTSC_44
						eVideoStandard = E_VIDEOSTANDARD_PAL_60;
					else
						eVideoStandard = E_VIDEOSTANDARD_NTSC_44;
				}
				#endif
				break;
			case VD_FSC_3579:
				if (pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_NTSC_50) {
					if (u16Status & VD_VSYNC_50HZ) {
						// 50Hz
						// 3.579545MHz, NTSC-50
						eVideoStandard = E_VIDEOSTANDARD_PAL_BGHI;
					} else {
						//3.579545MHz, NTSC
						HWREG_AVD_ADC_SetGMC(0x07);
						eVideoStandard = E_VIDEOSTANDARD_NTSC_M;
					}
				} else {
					HWREG_AVD_ADC_SetGMC(0x07);
					eVideoStandard = E_VIDEOSTANDARD_NTSC_M;
				}
				break;
			case VD_FSC_3575:// (FSC_MODE_PAL_M <<5):
				HWREG_AVD_ADC_SetGMC(0x07);
				eVideoStandard = E_VIDEOSTANDARD_PAL_M;
				break;
			case VD_FSC_3582:// (FSC_MODE_PAL_N <<5):
				HWREG_AVD_ADC_SetGMC(0x07);
				eVideoStandard = E_VIDEOSTANDARD_PAL_N;
				break;
			#if (TEST_VD_DSP)
			case (FSC_MODE_NTSC_443	<< 5):
				eVideoStandard = E_VIDEOSTANDARD_NTSC_44;
				break;
			case (FSC_MODE_PAL_60 << 5):
				eVideoStandard = E_VIDEOSTANDARD_PAL_60;
				break;
			#endif
			default:
				eVideoStandard = E_VIDEOSTANDARD_NOTSTANDARD;
				break;
			}
		} else {
			if (u16Status & VD_VSYNC_50HZ) {
				if ((u16Status & VD_FSC_TYPE) == VD_FSC_4285) {
					HWREG_AVD_ADC_SetGMC(0x06);
					// if	(u16Status & VD_VSYNC_50HZ)	// SECAM must 50 Hz
						eVideoStandard = E_VIDEOSTANDARD_SECAM;
				} else {
					HWREG_AVD_ADC_SetGMC(0x06);
					eVideoStandard = E_VIDEOSTANDARD_PAL_BGHI;
				}
			} else {
				HWREG_AVD_ADC_SetGMC(0x07);
				eVideoStandard = E_VIDEOSTANDARD_NTSC_M;
		}

#if	defined(STELLAR_FLAG)
//For LM15U WebOS, after SetColorSystem, GetStandardDetection
//Need to return the standard which is enable
//if only	480i is enable and input signal is 576i	, must return 480i standard	(NTSC-M)
//if only	576i is enable and input signal is 480i	, must return 576i standard	(PAL)
#if	(LM15U_GetStandard == 1)
			MS_U8 u8flag = HWREG_AVD_AFEC_GET_Standard_Detection_Flag();

			if (eVideoStandard == E_VIDEOSTANDARD_PAL_BGHI) {
				if (u8flag & (BIT(0))) {
					// PAL-BGHI disable
					if (!(u8flag & (BIT(6))))
						//PAL disable; PAL-N enable
						eVideoStandard = E_VIDEOSTANDARD_PAL_N;
					else if (!(u8flag & (BIT(1))))
						// PAL,PAL-N disable; SECAM enable
						eVideoStandard = E_VIDEOSTANDARD_SECAM;
					else
						// All 576i disable
						// Set 480i standard
						eVideoStandard = E_VIDEOSTANDARD_NTSC_M;
				}
			} else if (eVideoStandard == E_VIDEOSTANDARD_NTSC_M) {
				if (u8flag & (BIT(2))) {
					//NTSC disable
					if (!(u8flag & (BIT(3))))
						//NTSC disable;  NTSC443 enable
						eVideoStandard = E_VIDEOSTANDARD_NTSC_44;
					else if (!(u8flag & (BIT(4))))
						// NTSC, NTSC443 disable; PAL-M enable
						eVideoStandard = E_VIDEOSTANDARD_PAL_M;
					else
						//All 480i disable
						//Set 576i standard
						eVideoStandard = E_VIDEOSTANDARD_PAL_BGHI;
				}
			}
#endif
#endif
		}
	}

	pResource->_eLastStandard = eVideoStandard;

	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	return eVideoStandard;
}
#if	!defined STI_PLATFORM_BRING_UP
void Drv_AVD_GetCaptureWindow(void *stCapWin,	enum AVD_VideoStandardType eVideoStandardType,
struct AVD_RESOURCE_PRIVATE *pResource)
{
	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
	#if	AVD_DRV_DEBUG
	if (pResource->_u8AVDDbgLevel == AVD_DBGLV_DEBUG)
		ULOGE("AVD", "Api_AVD_GetCaptureWindow %ld %d\n",
		(pResource->g_VD_InitData.u32VDPatchFlag & AVD_PATCH_HTOTAL_MASK)>>8,
		_u16HtotalPAL[(pResource->g_VD_InitData.u32VDPatchFlag &
		AVD_PATCH_HTOTAL_MASK)>>8]);
	#endif
	switch (eVideoStandardType) {
	case E_VIDEOSTANDARD_PAL_BGHI:
		((MS_WINDOW_TYPE *)stCapWin)->width =
			(((MS_U32)720*(_u16HtotalPAL[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8])+432)/864);
		break;
	case E_VIDEOSTANDARD_NTSC_M:
		((MS_WINDOW_TYPE *)stCapWin)->width =
			(((MS_U32)720*(_u16HtotalNTSC[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8])+429)/858);
		break;
	case E_VIDEOSTANDARD_SECAM:
		((MS_WINDOW_TYPE *)stCapWin)->width =
			(((MS_U32)720*(_u16HtotalSECAM[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8])+432)/864);
		break;
	case E_VIDEOSTANDARD_NTSC_44:
		((MS_WINDOW_TYPE *)stCapWin)->width =
			(((MS_U32)720*(_u16HtotalNTSC_443[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8])+432)/864);
		break;
	case E_VIDEOSTANDARD_PAL_M:
		((MS_WINDOW_TYPE *)stCapWin)->width =
			(((MS_U32)720*(_u16HtotalPAL_M[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8])+429)/858);
		break;
	case E_VIDEOSTANDARD_PAL_N:
		((MS_WINDOW_TYPE *)stCapWin)->width =
			(((MS_U32)720*(_u16HtotalPAL_NC[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8])+429)/858);
		break;
	case E_VIDEOSTANDARD_PAL_60:
		((MS_WINDOW_TYPE *)stCapWin)->width =
			(((MS_U32)720*(_u16HtotalPAL_60[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8])+432)/864);
		break;
	default:
		((MS_WINDOW_TYPE *)stCapWin)->width =
			(((MS_U32)720*(_u16HtotalPAL[(pResource->g_VD_InitData.u32VDPatchFlag &
			AVD_PATCH_HTOTAL_MASK)>>8])+432)/864);
		break;
	}
	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);
}
#endif
enum AVD_VideoFreq Drv_AVD_GetVerticalFreq(void)
{
	MS_U16 u16Status;

	u16Status = HWREG_AVD_AFEC_GetStatus();
	if (IS_BITS_SET(u16Status, VD_HSYNC_LOCKED | VD_STATUS_RDY)) {
		if (VD_VSYNC_50HZ & u16Status)
			return E_VIDEO_FQ_50Hz;
		else
			return E_VIDEO_FQ_60Hz;
	}
	return E_VIDEO_FQ_NOSIGNAL;
}

MS_U8	Drv_AVD_GetHsyncEdge(void)
{
	return HWREG_AVD_GetHsyncEdge();
}

MS_U8	Drv_AVD_GetDSPFineGain(struct	AVD_RESOURCE_PRIVATE *pResource)
{
	MS_U8 u8CoaseGain, u8LPFGain;
	MS_U16 u16FineGain;

	AVD_LOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	u8LPFGain = ((_Drv_AVD_DSPReadByte(2, 0x81)&BIT(4))>>4); //DSP_81[4],	LPF gain
	u8CoaseGain	= _Drv_AVD_DSPReadByte(2, 0x80)&0x0F;//DSP_80[3:0], coarse gain
	//Read AGC fine-gain data	DSP_80[11:4]
	u16FineGain = ((_Drv_AVD_DSPReadByte(2, 0x80)&0xF0)>>4)+
	((_Drv_AVD_DSPReadByte(2, 0x81)&0x0F)<<4);

	HWREG_AVD_AFEC_SET_CoarseGain(u8CoaseGain);
	HWREG_AVD_AFEC_SET_FineGain(u16FineGain);
	HWREG_AVD_AFEC_SET_LPF_Gain(u8LPFGain);

	AVD_UNLOCK(pResource->_s32AVD_Mutex, pResource->_u8AVDDbgLevel);

	return (u16FineGain>>6);
}

MS_U16 Drv_AVD_GetDSPVersion(void)
{
	return (HWREG_AVD_AFEC_GET_DSPVersion() + DSP_VER_OFFSET);
}
#if	!defined STI_PLATFORM_BRING_UP
enum AVD_Result	Drv_AVD_GetLibVer(const MSIF_Version **ppVersion)
{
	// No mutex check, it can be called before Init
	if (!ppVersion)
		return E_AVD_FAIL;

	*ppVersion = &_drv_avd_version;

	return E_AVD_OK;
}
#endif
void Drv_AVD_GetInfo(struct AVD_Info *pAVD_Info, struct AVD_RESOURCE_PRIVATE *pResource)
{
	pAVD_Info->eVDInputSource = pResource->_eVDInputSource;
	pAVD_Info->eVideoSystem = pResource->_eVideoSystem;
	pAVD_Info->eLastStandard = pResource->_eLastStandard;
	pAVD_Info->u8AutoDetMode = pResource->_u8AutoDetMode;
	pAVD_Info->u16CurVDStatus = pResource->_u16CurVDStatus;
	pAVD_Info->u8AutoTuningIsProgress = pResource->_u8AutoTuningIsProgress;
}

MS_BOOL	Drv_AVD_IsSyncLocked(void)
{
	//should not check vsync lock during channel scan
	if (IS_BITS_SET(HWREG_AVD_AFEC_GetStatus(), VD_HSYNC_LOCKED))
		return TRUE;
	return FALSE;
}

MS_BOOL	Drv_AVD_IsSignalInterlaced(void)
{
	if (IS_BITS_SET(HWREG_AVD_AFEC_GetStatus(),
		VD_INTERLACED | VD_HSYNC_LOCKED | VD_STATUS_RDY))
		return TRUE;
	return FALSE;
}

MS_BOOL	Drv_AVD_IsColorOn(void)
{
	if (IS_BITS_SET(HWREG_AVD_AFEC_GetStatus(),
		VD_COLOR_LOCKED | VD_HSYNC_LOCKED | VD_STATUS_RDY))
		return TRUE;
	return FALSE;
}

MS_U32 Drv_AVD_SetPowerState(EN_POWER_MODE u16PowerState, struct AVD_RESOURCE_PRIVATE *pResource)
{
	static EN_POWER_MODE _prev_u16PowerState = E_POWER_MECHANICAL;
	MS_U32 u32Return = 1;

	if (u16PowerState == E_POWER_SUSPEND) {
		_prev_u16PowerState	= u16PowerState;
		u32Return = 0;//SUSPEND_OK;
		pResource->_bSTRFlag = 1;
		//for STR store DPL info for DC on/off
		pResource->_u16DPL_MSB = HWREG_AVD_AFEC_GET_MSB_DPL();
		pResource->_u16DPL_LSB = HWREG_AVD_AFEC_GET_LSN_DPL();
	} else if (u16PowerState == E_POWER_RESUME) {

		if (_prev_u16PowerState	== E_POWER_SUSPEND) {
			Drv_AVD_Init(&pResource->g_VD_InitData,
				sizeof(pResource->g_VD_InitData), pResource);
			pResource->_bSTRFlag = 0;
			//for STR load DPL  info for DC on/off
			HWREG_AVD_AFEC_SET_MSB_DPL(pResource->_u16DPL_MSB);
			HWREG_AVD_AFEC_SET_LSN_DPL(pResource->_u16DPL_LSB);

			// for STR resume, PAL-I error detect to SECAM.
			// Need to re-init SECAM register.
			HWREG_AVD_SECAM_SET_Magnitude_threshold();
			_prev_u16PowerState	=	u16PowerState;
			u32Return = 0;//RESUME_OK;
		} else
			u32Return = 1;//SUSPEND_FAILED;
	} else
		u32Return = 1;

	return u32Return;//	for	success
}

MS_BOOL	Drv_AVD_GetMacroVisionDetect(void)
{
	return HWREG_AVD_AFEC_GetMacroVisionDetect();
}

MS_BOOL	Drv_AVD_GetCGMSDetect(void)
{
	return HWREG_AVD_VBI_GetCGMSDetect();
}

void vd_str_stablestatus(void)
{
	MS_U16 u16Status = 0;
	MS_U8 u8StableCounter = 0;

	MS_U32 u32TimeoutATV = MsOS_GetSystemTime() + 600;
	MS_U32 u32TimeoutAV	= MsOS_GetSystemTime() + 1000;

	if (eSource_Str == E_INPUT_SOURCE_ATV) {
		while (MsOS_GetSystemTime() < u32TimeoutATV) {
			//VD status Not ready
			if ((u16Status & VD_STATUS_RDY) != VD_STATUS_RDY)
				u8StableCounter = 0;
			else {
				//VD status ready
				if ((u16Status & VD_HSYNC_LOCKED)) {
					//VD Hsync Luck
					u8StableCounter++;
					if (u8StableCounter > 10)
						break;
				} else
					//VD Hsync unLuck
					u8StableCounter = 0;
			}
			MsOS_DelayTask(10);
			u16Status = HWREG_AVD_AFEC_GetStatus();
		}
	} else {
		while (MsOS_GetSystemTime() < u32TimeoutAV) {
			if ((u16Status & VD_STATUS_RDY) != VD_STATUS_RDY)
				//VD status Not ready
				u8StableCounter = 0;
			else {
				//VD status ready
				if ((u16Status & VD_HSYNC_LOCKED)) {
					//VD Hsync Luck
					u8StableCounter++;
					if (u8StableCounter > 10)
						break;
				} else
					//VD Hsync unLuck
					u8StableCounter = 0;
			}
			MsOS_DelayTask(10);
			u16Status = HWREG_AVD_AFEC_GetStatus();
		}
	}
}

void Drv_AVD_SetBurstWinStart(MS_U8 u8BusrtStartPosition)
{
	HWREG_AVD_SetBurstWinStart(u8BusrtStartPosition);
}

MS_BOOL	Drv_AVD_IsAVDAlive(void)
{
	MS_U16 u16CurHeartclock;
	MS_U16 u16LastHeartclock;
	MS_U8 u8DeadCnt = 0;
	MS_U8 i = 0;
	MS_BOOL ret = FALSE;

	u16LastHeartclock = HWREG_AVD_AFEC_GET_Heartbeat_Counter();
	MsOS_DelayTask(1);
	for (i = 0; i < 5; i++) {
		u16CurHeartclock = HWREG_AVD_AFEC_GET_Heartbeat_Counter();
		if (u16CurHeartclock == u16LastHeartclock)
			u8DeadCnt++;
		u16LastHeartclock = u16CurHeartclock;

		MsOS_DelayTask(2);
	}

	if (u8DeadCnt == 0) {
		ULOGE("AVD", "MDrv_AVD_IsAVDAlive Status : Alive\n");
		ret = TRUE;
	} else {
		ULOGE("AVD", "MDrv_AVD_IsAVDAlive Status : Dead\n");
		ret = FALSE;
	}
	return ret;
}
#if	!defined STI_PLATFORM_BRING_UP
#if	defined(MSOS_TYPE_LINUX_KERNEL)
enum AVD_VideoStandardType vd_str_timingchangedetection(void)
{
	MS_U16 u16Status = 0;
	enum AVD_VideoStandardType eVideoStandard = E_VIDEOSTANDARD_NOTSTANDARD;

	u16Status = HWREG_AVD_AFEC_GetStatus();
	if (IS_BITS_SET(u16Status,	VD_HSYNC_LOCKED|VD_SYNC_LOCKED|VD_STATUS_RDY))
		if (u16Status & VD_BURST_ON) {
			switch (u16Status & VD_FSC_TYPE) {
			case VD_FSC_4433:
					// (FSC_MODE_PAL << 5):
					// 50 Hz
				if (u16Status & VD_VSYNC_50HZ)
					eVideoStandard = E_VIDEOSTANDARD_PAL_BGHI;
				else {
					// 60MHz
					if (u16Status & VD_PAL_SWITCH)
						// or vsdNTSC_44
						eVideoStandard = E_VIDEOSTANDARD_PAL_60;
					else
						eVideoStandard = E_VIDEOSTANDARD_NTSC_44;
				}
				break;

			case VD_FSC_3579:
				eVideoStandard = E_VIDEOSTANDARD_NTSC_M;
				break;

			case VD_FSC_3575:
				// (FSC_MODE_PAL_M << 5):
				eVideoStandard = E_VIDEOSTANDARD_PAL_M;
				break;

			case VD_FSC_3582:
				// (FSC_MODE_PAL_N << 5):
				eVideoStandard = E_VIDEOSTANDARD_PAL_N;
				break;

			default:
				eVideoStandard = E_VIDEOSTANDARD_NOTSTANDARD;
				break;
			}
		} else {
			if (u16Status & VD_VSYNC_50HZ) {
				if ((u16Status & VD_FSC_TYPE) == VD_FSC_4285)
					eVideoStandard = E_VIDEOSTANDARD_SECAM;
				else
					eVideoStandard = E_VIDEOSTANDARD_PAL_BGHI;
			} else
				eVideoStandard = E_VIDEOSTANDARD_NTSC_M;
		}
	else {
		return E_VIDEOSTANDARD_NOTSTANDARD;
	}

	return eVideoStandard;
}
#endif
#endif
MS_BOOL	Drv_AVD_IsLockAudioCarrier(void)
{
	MS_U16 u16tmp, u16tmp1 = 0;
	MS_U8 u8tmp;
	MS_U16 u16outedge, u16outedgesum = 0;
	MS_U16 u16noise, u16noisesum = 0;

	// in of window H sync edge;
	u16tmp1	=	HWREG_AVD_GetHsyncEdge() & 0x3F;
	if (u16tmp1 == 0)
		u16tmp1	= 1 ;//	to protect u16tmp1 not equal 0

	// Output of window H sync edge
	HWREG_AVD_AFEC_SET_ReadBus(0x0B);
	u16outedgesum = HWREG_AVD_AFEC_GET_Window_Hsync_Edge();
	if (u16outedgesum ==	0)
		u16outedgesum = 1;//	to protect u16outedgesum not equal 0

	// noise mag
	u16noisesum	= HWREG_AVD_AFEC_GetNoiseMag();
	if (u16noisesum == 0)
		u16noisesum	= 1;//	to protect u16noisesum not equal 0

	for (u8tmp = 0; u8tmp < 10; u8tmp++) {
		// in of window H sync edge
		u16tmp = HWREG_AVD_GetHsyncEdge()	& 0x3F;
		u16tmp1	= ((9*u16tmp1) + (1*u16tmp))/10;

		// Output of window H sync edge
		HWREG_AVD_AFEC_SET_ReadBus(0x0B);
		u16outedge = HWREG_AVD_AFEC_GET_Window_Hsync_Edge();
		u16outedgesum = ((9*u16outedgesum) + (1*u16outedge))/10;

		// noise mag
		u16noise = HWREG_AVD_AFEC_GetNoiseMag();
		u16noisesum	= ((9*u16noisesum) + (1*u16noise))/10;
		MsOS_DelayTask(1);
	}

	if ((u16tmp1 >= 0x35) || ((u16outedgesum	> 3) && (u16noise > 0x50)))
		return TRUE;
	else
		return FALSE;
}
void Drv_AVD_SetAnaDemod(MS_BOOL bEnable, struct AVD_RESOURCE_PRIVATE *pResource)
{
	if (bEnable)
		pResource->g_VD_InitData.eDemodType	= DEMODE_MSTAR_VIF;
	else
		pResource->g_VD_InitData.eDemodType	= DEMODE_NORMAL;
}

void Drv_AVD_SetHsyncSensitivityForDebug(MS_U8 u8AFEC99, MS_U8 u8AFEC9C,
	struct AVD_RESOURCE_PRIVATE *pResource)
{
	pResource->g_VD_InitData.eVDHsyncSensitivityTuning.u8DetectWinAfterLock = u8AFEC99&0x0F;
	pResource->g_VD_InitData.eVDHsyncSensitivityTuning.u8DetectWinBeforeLock = u8AFEC99&0x0F;
	pResource->g_VD_InitData.eVDHsyncSensitivityTuning.u8CNTRSyncAfterLock = u8AFEC9C;
}
MS_BOOL Drv_AVD_SetHWInfo(struct AVD_HW_Info stAVDHWInfo)
{
	HW_REG_AVD_HWinfo stHWinfo;

	stHWinfo.pu8RiuBaseAddr = stAVDHWInfo.pu8RiuBaseAddr;
	stHWinfo.u32DramBaseAddr = stAVDHWInfo.u32DramBaseAddr;
	stHWinfo.u32Comb3dAliment = stAVDHWInfo.u32Comb3dAliment;
	stHWinfo.u32Comb3dSize = stAVDHWInfo.u32Comb3dSize;
	return HWREG_AVD_SET_HW_Info(stHWinfo);
}

#ifdef CONFIG_UTOPIA_PROC_DBG_SUPPORT
char *Drv_AVD_Str_Tok(char *pstrSrc, char *pstrDes, char delimit)
{
	char *pstrRes = pstrSrc;

	*pstrDes = '\0';

	if (pstrSrc == NULL)
		return NULL;

	while (*pstrRes != '\0') {
		if (*pstrRes == delimit)
			break;
		*pstrDes++ = *pstrRes++;
	}
	*pstrDes = '\0';

	return (pstrRes	+	1);
}

int	Drv_AVD_StrToHex(char *pstrSrc)
{
	int	iRes = 0;
	char *pstrRes =	pstrSrc;

	while (*pstrRes != '\0') {
		iRes *=	16;
		if (*pstrRes >= '0' && *pstrRes <= '9')
			iRes +=	(int)((*pstrRes++) - '0');
		else if (*pstrRes >= 'A' && *pstrRes <= 'F')
			iRes +=	(int)((*pstrRes++) - '7');
	}
	return iRes;
}
MS_BOOL	Drv_AVD_DBG_EchoCmd(MS_U64 *pu64ReqHdl, MS_U32 u32CmdSize, char pcmd)
{
	char *ptr = NULL;
	char strtmp[128] = {0};
	MS_U8 u8tmp, u8tmp1, u8tmp2;
	MS_BOOL	btmp;
	MS_U16 u16status, u16status1;
	int itmp, itmp1 = 0;

	ptr = Drv_AVD_Str_Tok(pcmd, strtmp, ' ');
	if (strncmp(pcmd, "VD_Standard_Report", 18) ==	0) {
		u16status	=	HWREG_AVD_AFEC_GetStatus();
		u8tmp	=	Drv_AVD_GetNoiseMag();
		btmp = Drv_AVD_IsAVDAlive();
		u8tmp2 =	HWREG_AVD_AFEC_GET_FSC_Detection_Flag();

		MdbPrint(pu64ReqHdl, " --------- VD	Standard Report	---------\n");
		if (!IS_BITS_SET(u16status, VD_HSYNC_LOCKED|VD_SYNC_LOCKED|VD_STATUS_RDY))
			MdbPrint(pu64ReqHdl, " Standard:    No-Signal\n");
		else {
			if (u16status & VD_BURST_ON) {
				switch (u16status & VD_FSC_TYPE) {
				case VD_FSC_4433:
					// (FSC_MODE_PAL <<	5):
					if (u16status & VD_VSYNC_50HZ)
						//50 Hz
						MdbPrint(pu64ReqHdl, " Standard:    PAL-BGHI\n");
					else if (u16status & VD_PAL_SWITCH)
						//60MHz
						MdbPrint(pu64ReqHdl, " Standard:    PAL-60\n");
					else
						MdbPrint(pu64ReqHdl, " Standard:    NTSC-443\n");
					break;
				case VD_FSC_3579:
					MdbPrint(pu64ReqHdl, " Standard:    NTSC-M\n");
					break;
				case VD_FSC_3575:
					// (FSC_MODE_PAL_M <<	5):
					MdbPrint(pu64ReqHdl, " Standard:    PAL-M\n");
					break;
				case VD_FSC_3582:
					// (FSC_MODE_PAL_N <<	5):
					MdbPrint(pu64ReqHdl, " Standard:    PAL-N\n");
					break;
				case (FSC_MODE_NTSC_443 << 5):
					MdbPrint(pu64ReqHdl, " Standard:    NTSC-443\n");
					break;
				case (FSC_MODE_PAL_60 << 5):
					MdbPrint(pu64ReqHdl, " Standard:    PAL-60\n");
					break;
				default:
					MdbPrint(pu64ReqHdl, " Standard:    Not-Standard\n");
					break;
				}
			} else {
				if (u16status & VD_VSYNC_50HZ) {
					if ((u16status & VD_FSC_TYPE) == VD_FSC_4285)
						MdbPrint(pu64ReqHdl, " Standard:    SECAM\n");
					else
						MdbPrint(pu64ReqHdl, " Standard:    PAL-BGHI\n");
				} else
					MdbPrint(pu64ReqHdl, " Standard:    NTSC-M\n");
			}
		}

		MdbPrint(pu64ReqHdl, " AVD alive:    %d\n", btmp);
		MdbPrint(pu64ReqHdl, " status ready:    %d\n", (u16status & VD_STATUS_RDY));
		MdbPrint(pu64ReqHdl, " V sync Clock:    %x\n", (u16status & VD_SYNC_LOCKED)>>15);
		MdbPrint(pu64ReqHdl, " H sync Clock:    %x\n", (u16status & VD_HSYNC_LOCKED)>>14);
		MdbPrint(pu64ReqHdl, " Burst Detect:    %x\n", (u16status & VD_BURST_ON)>>1);
		MdbPrint(pu64ReqHdl, " Color Lock:    %x\n", (u16status & VD_COLOR_LOCKED)>>10);
		MdbPrint(pu64ReqHdl, " Data	Valid:    %x\n", (u16status & VD_STATUS_RDY));
		MdbPrint(pu64ReqHdl, " Macro Vision:    %x\n", (u16status & VD_MACROVISION)>>2);
		MdbPrint(pu64ReqHdl, " VC Mode:    %x\n", (u16status & VD_VCR_MODE)>>4);
		MdbPrint(pu64ReqHdl, " V Sync Type:    %d Hz\n",
			((u16status & VD_VSYNC_50HZ)>>12)?50:60);
		MdbPrint(pu64ReqHdl, " Noise Mag (better<0x06):	0x%x\n", u8tmp);
		MdbPrint(pu64ReqHdl, " Force Standard:    %x\n", (u8tmp2 & BIT(0)));
		return TRUE;
	} else if (strncmp(pcmd, "CVBS_Signal_Information", 23) == 0)
		u8tmp =	HWREG_AVD_AFEC_GET_Hsync_Mag();
		u16status = Drv_AVD_GetVTotal();
		u16status1 = HWREG_AVD_AFEC_GetHTotal();
		MdbPrint(pu64ReqHdl, "---------	CVBS Signal	Information-------\n");
		MdbPrint(pu64ReqHdl, " H total: %d\n", u16status1);
		MdbPrint(pu64ReqHdl, " V total: %d\n", u16status);
		MdbPrint(pu64ReqHdl, " H sync Mag: 0x%x\n", u8tmp);
		return TRUE;
	} else if (strncmp(pcmd, "3D_Comb_Status", 14) == 0) {
		u8tmp = HWREG_AVD_COMB_GET_3D_Status();
		u8tmp1 = u8tmp&BIT(1);
		MdbPrint(pu64ReqHdl, "-----------3D	Comb Status	------------\n");
		if (u8tmp == 0xC0)
			MdbPrint(pu64ReqHdl, " 3dCombStatus:Lock\n");
		else
			MdbPrint(pu64ReqHdl, " 3dCombStatus:unLock\n");

		if (u8tmp1 == 0)
			MdbPrint(pu64ReqHdl, " 3dCombBandwidth: BandWidth Enough\n");
		else
			MdbPrint(pu64ReqHdl, " 3dCombBandwidth: BandWidth Not Enough\n");
		return TRUE;
	} else if (strncmp(pcmd, "VDReset", 7) == 0) {
		if (strncmp(ptr,	"ResetDspCode", 12) == 0)
			HHWREG_AVD_RESET_DSP_Code();
		else if (strncmp(ptr, "ResetAfecHW", 11) == 0)
			HWREG_AVD_RESET_AFEC_HW();
		else if (strncmp(ptr, "ResetComb", 9) == 0)
			HWREG_AVD_RESET_COMB();
		else {
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
			return FALSE;
		}
		MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n", __LINE__);
		return TRUE;
	} else if (strncmp(pcmd, "VDTestPattern", 13) == 0) {
		if (strncmp(ptr,	"Cancel", 6) == 0)
			HWREG_AVD_Test_Pattern(0x00);
		else if (strncmp(ptr, "Row",	3) == 0)
			HWREG_AVD_Test_Pattern(0x01);
		else if (strncmp(ptr, "Column", 6) == 0)
			HWREG_AVD_Test_Pattern(0x02);
		else {
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
			return FALSE;
		}
		MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n", __LINE__);
		return TRUE;
	} else if (strncmp(pcmd, "VDForceStandard ", 15) == 0) {
		HWREG_AVD_AFEC_EnableForceMode(ENABLE);
		itmp = ptr[0]-'0';
		switch (itmp) {
		case MDBCMD_FORCE_CANCEL:
			HWREG_AVD_AFEC_EnableForceMode(DISABLE);
			break;
		case MDBCMD_FORCE_PAL:
			HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL);
			break;
		case MDBCMD_FORCE_SECAM:
			HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_SECAM);
			break;
		case MDBCMD_FORCE_NTSC:
			HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_NTSC);
			break;
		case MDBCMD_FORCE_NTSC443:
			HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_NTSC_443);
			break;
		case MDBCMD_FORCE_PAL_M:
			HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_M);
			break;
		case MDBCMD_FORCE_PAL_60:
			HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_60);
			break;
		case MDBCMD_FORCE_PAL_NC:
			HWREG_AVD_AFEC_SetFSCMode(FSC_MODE_PAL_N);
			break;
		default:
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
			return FALSE;
		}
		MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n", __LINE__);
		return TRUE;
	} else if (strncmp(pcmd, "HVSliceLevel", 12) == 0) {
		HWREG_AVD_AFEC_SET_ENABLE_SLICE_LEVEL(0xC0);
		itmp = ptr[0]-'0';
		switch (itmp) {
		case MDBCMD_LEVEL_CANCEL:
			HWREG_AVD_AFEC_SET_ENABLE_SLICE_LEVEL(0xC0);
			break;
		case MDBCMD_LEVEL_44:
			HWREG_AVD_AFEC_SET_V_SLICE_LEVEL(0x40);
			HWREG_AVD_AFEC_SET_H_SLICE_LEVEL(0x04);
			break;
		case MDBCMD_LEVEL_88:
			HWREG_AVD_AFEC_SET_V_SLICE_LEVEL(0x80);
			HWREG_AVD_AFEC_SET_H_SLICE_LEVEL(0x08);
			break;
		case MDBCMD_LEVEL_CC:
			HWREG_AVD_AFEC_SET_V_SLICE_LEVEL(0xC0);
			HWREG_AVD_AFEC_SET_H_SLICE_LEVEL(0x0C);
			break;
		case MDBCMD_LEVEL_FF:
			HWREG_AVD_AFEC_SET_V_SLICE_LEVEL(0xF0);
			HWREG_AVD_AFEC_SET_H_SLICE_LEVEL(0x0F);
			break;
		default:
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
			return FALSE;
		}
		MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n", __LINE__);
		return TRUE;
	} else if (strncmp(pcmd, "HsncPLL", 8) == 0) {
		itmp = ptr[0]-'0';
		switch (itmp) {
		case MDBCMD_PLL_CANCEL:
			HWREG_AVD_AFEC_SET_DPL_K1(HWREG_AVD_AFEC_GET_DPL_K1() & 0x7F));
			break;
		case MDBCMD_PLL_Level1:
			HWREG_AVD_AFEC_SET_DPL_K1(0x82);
			HWREG_AVD_AFEC_SET_DPL_K2(0x04);
			break;
		case MDBCMD_PLL_Level2:
			HWREG_AVD_AFEC_SET_DPL_K1(0x90);
			HWREG_AVD_AFEC_SET_DPL_K2(0x20);
			break;
		case MDBCMD_PLL_Level3:
			HWREG_AVD_AFEC_SET_DPL_K1(0x9A);
			HWREG_AVD_AFEC_SET_DPL_K2(0x35);
			break;
		case MDBCMD_PLL_Level4:
			HWREG_AVD_AFEC_SET_DPL_K1(0xAA);
			HWREG_AVD_AFEC_SET_DPL_K2(0x6A);
			break;
		case MDBCMD_PLL_Level5:
			HWREG_AVD_AFEC_SET_DPL_K1(0xBC);
			HWREG_AVD_AFEC_SET_DPL_K2(0x6A);
			break;
		default:
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
			return FALSE;
		}
		MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n", __LINE__);
		return TRUE;
	} else if (strncmp(pcmd, "COMB	", 4) == 0) {
		itmp = ptr[0]-'0';
		switch (itmp) {
		case MDBCMD_COMB_CANCEL:
			HWREG_AVD_ByPass_COMB(0x04);
			break;
		case MDBCMD_COMB_ByPassComb_Filter:
			HWREG_AVD_ByPass_COMB(0xFB);
			break;
		default:
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
			return FALSE;
		}
		MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n", __LINE__);
		return TRUE;
	} else if (strncmp(pcmd, "BurstWindow", 11) ==	0) {
		itmp = ptr[0]-'0';
		ptr	=	Drv_AVD_Str_Tok(ptr, strtmp, ' ');
		if (ptr[0] == '0' && ptr[1] == 'x') {
			ptr	=	Drv_AVD_Str_Tok(ptr, strtmp, 'x');
			itmp1	=	Drv_AVD_StrToHex(ptr);
			switch (itmp) {
			case MDBCMD_Window_Start:
				if (itmp1 <= 0x1F && itmp1 >= 0x0)
					HWREG_AVD_SetBurstWinStart(itmp1);
					break;
			case MDBCMD_Window_End:
				if (itmp1 <= 0x7F && itmp1 >= 0x0)
					HWREG_AVD_SetBurstWinEnd(itmp1);
				break;
			default:
				MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
				return FALSE;
			}
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n", __LINE__);
			return TRUE;
		}
	} else if (strncmp(pcmd, "3DCombThreshold", 15) == 0) {
		if (ptr[0] == '0' &&	ptr[1] == 'x') {
			ptr = Drv_AVD_Str_Tok(pcmd,	strtmp,	'x');
			itmp1 =	Drv_AVD_StrToHex(ptr);
			HWREG_AVD_SET_COMB_Threshold(itmp1);
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n",	__LINE__);
			return TRUE;
		}
	} else if (strncmp(pcmd, "FixGain", 7) == 0) {
		itmp = ptr[0]-'0';
		switch (itmp) {
		case MDBCMD_FixGain_CANCEL:
			HWREG_AVD_AFEC_AGCSetMode(AVD_AGC_DISABLE);
			break;
		case MDBCMD_FixGain_ENABLE:
			HWREG_AVD_AFEC_AGCSetMode(AVD_AGC_ENABLE);
			break;
		default:
			MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
			return FALSE;
		}
		MdbPrint(pu64ReqHdl, "[%d]DBG CMD Success!!!\n", __LINE__);
		return TRUE;
	} else if (strncmp(pcmd, "help", 4) == 0) {
		MdbPrint(pu64ReqHdl, "----------MStar help----------\n");
		MdbPrint(pu64ReqHdl, "1. Module	Information VD_Standard_Report\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo VD_Standard_Report > avd\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "2. Module	Information CVBS_Signal_Information\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo CVBS_Signal_Information > avd\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "3. Module	Information 3D_Comb_Status\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo 3D_Comb_Status >	avd\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "4. VDReset\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo VDReset [opcode]	> avd\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 0 : VD	DSP	code reset\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 1 : AFEC	Hardware reset\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 2 : COMB	Reset\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "5. VDTestPattern\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo VDTestPattern [opcode] >	avd\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 0 : Cancel COMB test pattern\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 1 : test	pattern	1\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 2 : test	pattern2\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "6. VDForceStandard\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo VDForceStandard [opcode]	> avd\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 0 : Disable Video Standard\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 1 : PAL\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 2 : SECAM\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 3 : NTSC\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 4 : NTSC443\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 5 : PAL-M\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 6 : PAL-60\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 7 : PAL-NC\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "7. HVSliceLevel\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo HVSliceLevel	[opcode] > avd\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 0 : Disable H/V Slice Level\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 1 : H/V Slice Level 0x44\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 2 : H/V Slice Level 0x88\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 3 : H/V Slice Level 0xCC\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 4 : H/V Slice Level 0xFF\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "8. HsyncPLL\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo HsyncPLL	[opcode] > avd\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 0 : Disable Hsync PLL\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 1 : Hsync PLL 0x82/0x04\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 2 : Hsync PLL 0x90/0x20\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 3 : Hsync PLL 0x9A/0x35\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 4 : Hsync PLL 0xAA/0x6A\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 5 : Hsync PLL 0xBC/0x6A\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "10. COMB\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo COMB	[opcode] > avd\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 0 : Disable by	Pass Comb	Filter\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 1 : Enable	by Pass	Comb Filter\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "11. BurstWindow\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo BurstWindow [opcode]	[Input value] > avd\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 1 : Adjustment Burst Window start position\n");
		MdbPrint(pu64ReqHdl, "	[Input value] = 0x00~0x1F\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 2 : Adjustment Burst Window end position\n");
		MdbPrint(pu64ReqHdl, "	[Input value] = 0x00~0x7F\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "12. 3DCombThreshold\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo 3DCombThreshold [Input	value] > avd\n");
		MdbPrint(pu64ReqHdl, "	[Input value] = 0xC0, 0xC1, ...\n");
		MdbPrint(pu64ReqHdl, "\n");
		MdbPrint(pu64ReqHdl, "13. FixGain\n");
		MdbPrint(pu64ReqHdl, "	[e.g.]echo FixGain [opcode]	>	avd\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 0 : Disable Fix Gain\n");
		MdbPrint(pu64ReqHdl, "	[opcode] = 1 : Enable Fix Gain\n");
		return TRUE;
	}
	MdbPrint(pu64ReqHdl, "[%d]DBG CMD Error!!!\n", __LINE__);
	return FALSE;

}
MS_BOOL	Drv_AVD_DBG_GetModuleInfo(MS_U64 *pu64ReqHdl)
{
	MdbPrint(pu64ReqHdl, "----------MStar Info----------\n");
	MdbPrint(pu64ReqHdl, "A.VD_Standard_Report\n");
	MdbPrint(pu64ReqHdl, "B.CVBS_Signal_Information\n");
	MdbPrint(pu64ReqHdl, "C.3D_Comb_Status\n");
	MdbPrint(pu64ReqHdl, "Use 'echo'	to get information.\n");
	Drv_AVD_DBG_EchoCmd(pu64ReqHdl, 18, "VD_Standard_Report");
	Drv_AVD_DBG_EchoCmd(pu64ReqHdl, 23, "CVBS_Signal_Information");
	Drv_AVD_DBG_EchoCmd(pu64ReqHdl, 14, "3D_Comb_Status");
	return TRUE;
}

#endif //CONFIG_UTOPIA_PROC_DBG_SUPPORT
