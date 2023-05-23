// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define MSOS_TYPE_LINUX_KERNEL
#define OS_SC_TIME get_jiffies_64
#define OS_SYSTEM_TIME get_jiffies_64
#define js2ms(x)	jiffies64_to_nsecs(x)
#define OS_SC_DELAY(x)	msleep_interruptible((unsigned int)x)
#define OS_DELAY_TASK(x)  msleep_interruptible((unsigned int)x)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define OS_SC_STR_LOCK()			 {	}
#define OS_SC_STR_UNLOCK()			   {  }
#define MsOS_KernelMemoryRegionCheck(arg) (1)
#define UTOPIA_STATUS_SUCCESS				0x00000000
#define UTOPIA_STATUS_FAIL					0x00000001
#define THOUSAND				1000
#define TEN						10
#define FIVE					5
//-------------------------------------------------------------------------------------------------
//              Include Files
//-------------------------------------------------------------------------------------------------
#define SC_KERNEL_ISR

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
//#include <linux/jiffies.h> //for get_jiffies_64


#include "mtk_sc_drv.h"
#include "mtk_sc_mdrv.h"	// for STI
#include "mtk_sc_common.h"
#include "mtk_sc_private.h"
#include "mtk_sc_reg.h"
#include "mtk_sc_hal.h"

extern MS_U32 _regSCHWBase;

//-------------------------------------------------------------------------------------------------
//              Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//              Local Defines
//-------------------------------------------------------------------------------------------------
/////////////////////
// PATCH_TO_DISABLE_CGT => In 7816 test item 3412/3416,
// it indicates GT shall be 13 etu but we found the rcved data from Card is still 12 etu.
// So we add this definition to set CGT intterupt mask avoiding RX CGT fail interrupt triggered
// And we also add switch for nagra paired CGT test
/////////////////////
#define PATCH_TO_DISABLE_CGT		1
#if PATCH_TO_DISABLE_CGT
//#define _gbPatchToDisableRxCGTFlag = TRUE             _gbPatchToDisableRxCGTFlag = TRUE
//#define _gbPatchToDisableRxCGTFlag = FALSE                    _gbPatchToDisableRxCGTFlag = FALSE
#define IS_RX_CGT_CHECK_DISABLE		_gbPatchToDisableRxCGTFlag

/////////////////////
// PATCH_TO_DISABLE_CGT_IRDETO_T14 => The CGT is fail for irdeto T=14 in
// ATR. It sems ETU time has problem to cause incorrect CGT flag.
// Temp to disable CGT check for Irdeto T=14
/////////////////////
#define PATCH_TO_DISABLE_CGT_IRDETO_T14		1
#endif

/////////////////////
// PATCH_TO_DISABLE_BGT => In 7816 test item 3311/3321,
// we found the RX BGT is smaller than 22 etu
// that violates T=1 need minimum interval 22 etu requirement.
// So we add this definition to set BGT intterupt mask avoiding RX BGT fail interrupt triggered
/////////////////////
#define PATCH_TO_DISABLE_BGT		1

/////////////////////
// SC_T0_PARITY_ERROR_KEEP_RCV_ENABLE =>
// For T=0, keep try to receive data and not be interrupted to parity error
//
/////////////////////
#define SC_T0_PARITY_ERROR_KEEP_RCV_ENABLE 0

#define SC_WAIT_DELAY				20UL	//20//50// TFCA //[TODO] tunable
#define SC_NULL_DELAY				10UL	//500
#define SC_WRITE_TIMEOUT			500UL
#define SC_RST_HOLD_TIME			14UL	//10
#define SC_RST_SWITCH_TIME			60UL	//100
#define SC_CARD_STABLE_TIME			10UL	//100

#define SC_T0_ATR_TIMEOUT			100UL
#define SC_T0_SEND_TIMEOUT			400UL	//50

// add this              for TFCA some command will timeout
#define SC_T0_RECV_TIMEOUT			400UL	//80

#define SC_T1_SEND_TIMEOUT			70UL	// 1.5 //50
#define SC_T1_RECV_TIMEOUT			70UL	// 1.5 //80

#define SC_T1_PROLOGUE_SIZE			3
#define SC_RC_TYPE_CRC				0x1
#define SC_RC_TYPE_LRC				0x0
#define SC_RC_SIZE_CRC				2
#define SC_RC_SIZE_LRC				1

// local test debug message
#define MS_DEBUG 1
#ifdef MS_DEBUG
#ifdef MSOS_TYPE_LINUX_KERNEL
#define SC_DBG(_d, _f, _a...)		{ if (_dbgmsg >= _d) pr_info(_f, ##_a); }
#else
#define SC_DBG(_d, _f, _a...)
#endif
#else
#define SC_DBG(_d, _f, _a...)		{ }
#endif

#if defined(MSOS_TYPE_LINUX) && defined(SC_KERNEL_ISR)
#define SM0_DEV_NAME				"/dev/smart"
#define SM1_DEV_NAME				"/dev/smart1"
#endif

#define SC_INT_FAIL -1

#define CPY_TO_USER		memcpy
#define CPY_FROM_USER	memcpy
#define SC_MALLOC		vmalloc	//malloc
#define SC_FREE			vfree	//mbuf




MS_U16 u16ISRTXLevel;
MS_U16 u16ISRTXEmpty;

#define E_TIME_ST_NOT_INITIALIZED  0xFFFFFFFFUL

#define E_INVALID_TIMEOUT_VAL 0x00000000UL

#define RFU 0x00UL		//Reserved for future of Di and Fi


#define SC_SHMSMC1 "smc1_proc_sync"
#define SC_SHMSMC2 "smc2_proc_sync"


#define SPEC_U32_DEC	MPRI_UDEC
#define								MPRI_UDEC							"%u"
#define SPEC_U32_HEX	MPRI_HEX
#define								MPRI_HEX							"%x"

/////////////////////
// Timing Compensation
/////////////////////

//////////////////////////////
//
//CONAX: ((wt_min+wt_max)/2)/wt --> ((9590+9760)/2)/9600 = 1.0078
//
//////////////////////////////
#define SC_CONAX_COMPENSATION			10078
#define SC_CONAX_COMPENSATION_DIVISOR	10000

//////////////////////////////
//
//STAR3150: 1.00002
//
//////////////////////////////
#define SC_STAR3150_COMPENSATION			100002
#define SC_STAR3150_COMPENSATION_DIVISOR	100000


#define SC_TIMING_COMPENSATION				SC_STAR3150_COMPENSATION
#define SC_TIMING_COMPENSATION_DIVISOR		SC_STAR3150_COMPENSATION_DIVISOR
////////////////////

//-------------------------------------------------------------------------------------------------
//              Local Structurs
//-------------------------------------------------------------------------------------------------
typedef struct {
	MS_U32 u32FifoDataBuf;
	MS_U16 u16DataLen;
	MS_U32 u8SCID;
	MS_U32 u32TimeMs;
	SC_Result eResult;
} SC_SendData;

typedef struct {
	MS_BOOL bUseBwtInsteadExtCwt;
	MS_BOOL bIsExtWtPatch;
	MS_U32 u32ExtWtPatchSwWt;
} SC_ExtWtPatch;

typedef struct {
	MS_U8 u8NAD;
	MS_U8 u8NS;
	MS_U8 u8NR;
	MS_BOOL ubMore;
	MS_U8 u8IFSC;
	MS_U8 u8RCType;
	MS_U8 u8BWI;
	MS_U8 u8CWI;
	MS_U32 u32CWT;		//usec
	MS_U32 u32CGT;		//usec
	MS_U32 u32BWT;		//usec
	MS_U32 u32BGT;		//usec
	MS_U8 u8BGT_IntCnt;
	MS_U8 u8BWT_IntCnt;
	MS_U8 u8CWT_TxIntCnt;
	MS_U8 u8CWT_RxIntCnt;
	MS_U8 u8CGT_RxIntCnt;
	MS_U8 u8CGT_TxIntCnt;
	MS_BOOL bParityError;
} SC_T1_State;

typedef struct {
	MS_U8 u8WI;		//Waiting Integer, 0x00 is RFU
	MS_U32 u32WT;		//usec
	MS_U32 u32GT;		//usec
	MS_U8 u8WT_TxIntCnt;
	MS_U8 u8WT_RxIntCnt;
	MS_U8 u8GT_RxIntCnt;
	MS_U8 u8GT_TxIntCnt;
	MS_BOOL bParityError;
	SC_ExtWtPatch stExtWtPatch;
} SC_T0_State;

typedef struct {
	MS_BOOL bRstToAtrTimeout;
} SC_RST_TO_ATR_CTRL;

typedef struct {
	MS_U8 u8N;		//N
	SC_VoltageCtrl eVoltage;
	MS_U8 u8CardDetInvert;
} SC_Info_Ext;

typedef struct {
	MS_BOOL bRcvPPS;
	MS_BOOL bRcvATR;
	MS_BOOL bContRcv;
	MS_BOOL bDoReset;
	MS_BOOL bCwtFailDoNotRcvData;
	MS_U32 u32RcvedRxLen;
} SC_Flow_Ctrl;

typedef enum {
	SC_T1_R_OK,
	SC_T1_R_EDC_ERROR,
	SC_T1_R_OTHER_ERROR,
} SC_T1_RBlock_State;

typedef enum {
	SC_T1_S_IFS,
	SC_T1_S_ABORT,
	SC_T1_S_WTX,
	SC_T1_S_RESYNCH,
	SC_T1_S_REQUEST,
	SC_T1_S_RESPONSE,
} SC_T1_SBlock_State;

#if (defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR)
typedef enum {
	E_SC_INT_TX_LEVEL = 0x00000001UL,
	E_SC_INT_CARD_IN = 0x00000002UL,	//UART_SCSR_INT_CARDIN
	E_SC_INT_CARD_OUT = 0x00000004UL,	//UART_SCSR_INT_CARDOUT
	E_SC_INT_CGT_TX_FAIL = 0x00000008UL,
	E_SC_INT_CGT_RX_FAIL = 0x00000010UL,
	E_SC_INT_CWT_TX_FAIL = 0x00000020UL,
	E_SC_INT_CWT_RX_FAIL = 0x00000040UL,
	E_SC_INT_BGT_FAIL = 0x00000080UL,
	E_SC_INT_BWT_FAIL = 0x00000100UL,
	E_SC_INT_PE_FAIL = 0x00000200UL,
	E_SC_INT_RST_TO_ATR_FAIL = 0x00000400UL,
	E_SC_INT_MDB_CMD_DATA = 0x00000800UL,
	E_SC_INT_INVALID = 0xFFFFFFFFUL
} SC_INT_BIT_MAP;

typedef enum {
	E_SC_ATTR_INVALID = 0x00000000UL,
	E_SC_ATTR_TX_LEVEL = 0x00000001UL,
	E_SC_ATTR_NOT_RCV_CWT_FAIL = 0x00000002UL,
	E_SC_ATTR_T0_PE_KEEP_RCV = 0x00000004UL,
	E_SC_ATTR_FAIL_TO_RST_LOW = 0x00000008UL
} SC_ATTR_TYPE;
#endif

#if defined(CONFIG_UTOPIA_FRAMEWORK_KERNEL_DRIVER)
typedef struct {
	MS_BOOL bKDrvTaskStart;
	MS_BOOL bKDrvTaskRunning;
	MS_S32 s32KDrvTaskID;
	P_SC_Callback pfSmartNotify;
} SC_KDrv_UserModeCtrl;
#endif

#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(SC_KERNEL_ISR)
typedef enum {
	E_SC_KDRV_EVENT_INVALID = 0x00000000UL,
	E_SC_KDRV_EVENT_DATA = 0x00000001UL,
	E_SC_KDRV_EVENT_IN = 0x00000002UL,
	E_SC_KDRV_EVENT_OUT = 0x00000004UL
} SC_KDRV_EVENT;

typedef struct {
	MS_S32 s32KDrvMutexID;
	MS_BOOL bKDrvReady;
	SC_KDRV_EVENT eEvent;
	wait_queue_head_t stWaitQue;
} SC_KDrv_KernelModeCtrl;
#endif

typedef struct {
	MS_BOOL bIsResumeDrv;
	SC_Param stParam;
	P_SC_Callback pfNotify;
} SC_STR_Ctrl;

//-------------------------------------------------------------------------------------------------
//              Global Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//              Local Variables
//-------------------------------------------------------------------------------------------------
// 7816-3
static const MS_U16 _atr_Fi[] = {
	372, 372, 558, 744, 1116, 1488, 1860, RFU, RFU,
	512, 768, 1024, 1536, 2048, RFU, RFU
};

static const MS_U16 _atr_Di[] = { RFU, 1, 2, 4, 8, 16, 32, 64, 12, 20,
	RFU, RFU, RFU, RFU, RFU, RFU
};

static const SC_Caps _scCaps = {
	.u8DevNum = SC_DEV_NUM,
};

static SC_Status _scStatus = {
	.bCardIn = FALSE,
};

MS_S8 _dbgmsg = E_SC_DBGLV_ALL;

static SC_T0_State _sc_T0State[SC_DEV_NUM] = {
	{0, E_TIME_ST_NOT_INITIALIZED, 0},
#if (SC_DEV_NUM > 1)		// no more than 2
	{0, E_TIME_ST_NOT_INITIALIZED, 0}
#endif
};


static SC_T1_State _sc_T1State[SC_DEV_NUM] = {
	{
	 .u8NAD = 0,
	 .u8NS = 0,
	 .u8NR = 0,
	 .ubMore = FALSE,
	 .u8IFSC = 254,
	 .u8RCType = 0x00,
	}
};

SC_Info _scInfo[SC_DEV_NUM] = {
	{
	 .bInited = 0,
	 .bOpened = 0,
	 .eCardClk = E_SC_CLK_4P5M,
	 .eVccCtrl = E_SC_VCC_CTRL_LOW,
	 .u16ClkDiv = 372,
	 .bCardIn = FALSE,
	 .pfNotify = NULL,
	 .u16AtrLen = 0,
	 .u16HistLen = 0,
	 .bLastCardIn = FALSE,
	 .s32DevFd = -1,
	 },
#if (SC_DEV_NUM > 1)		// no more than 2
	{
	 .bInited = 0,
	 .bOpened = 0,
	 .eCardClk = E_SC_CLK_4P5M,
	 .eVccCtrl = E_SC_VCC_CTRL_LOW,
	 .u16ClkDiv = 372,
	 .bCardIn = FALSE,
	 .pfNotify = NULL,
	 .u16AtrLen = 0,
	 .u16HistLen = 0,
	 .bLastCardIn = FALSE,
	 .s32DevFd = -1,
	}
#endif
};

static SC_Info_Ext _scInfoExt[SC_DEV_NUM];

static SC_Flow_Ctrl _gScFlowCtrl[SC_DEV_NUM] = {
	{
	 .bRcvPPS = FALSE,
	 .bRcvATR = FALSE,
	 .bContRcv = FALSE,
	 .bDoReset = FALSE,
	 .bCwtFailDoNotRcvData = FALSE,
	 .u32RcvedRxLen = 0},
#if (SC_DEV_NUM > 1)		// no more than 2
	{
	 .bRcvPPS = FALSE,
	 .bRcvATR = FALSE,
	 .bContRcv = FALSE,
	 .bDoReset = FALSE,
	 .bCwtFailDoNotRcvData = FALSE,
	 .u32RcvedRxLen = 0}
#endif
};

#if !((defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR))
static SC_SendData _scSendData;
#endif


static SC_RST_TO_ATR_CTRL _gastRstToAtrCtrl[SC_DEV_NUM] = {
	{FALSE},
#if (SC_DEV_NUM > 1)		// no more than 2
	{FALSE}
#endif
};

#if PATCH_TO_DISABLE_CGT
static MS_BOOL _gbPatchToDisableRxCGTFlag = FALSE;
#endif

static SC_HW_FEATURE _gstHwFeature = {
	.eRstToIoHwFeature = E_SC_RST_TO_IO_HW_NOT_SUPPORT,
	.bExtCwtFixed = FALSE
};

#if !((defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR))
static MS_U32 _gau32EcosCwtRxErrorIndex[SC_DEV_NUM] = {
	0xFFFFFFFF,
#if (SC_DEV_NUM > 1)		// no more than 2
	0xFFFFFFFF
#endif
};
#endif

static MS_S32 _gScInitMutexID = -1;

#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(SC_KERNEL_ISR)
static SC_KDrv_KernelModeCtrl _gstKDrvKernelModeCtrl[SC_DEV_NUM] = {
	{
	 .s32KDrvMutexID = -1,
	 .bKDrvReady = FALSE,
	 .eEvent = E_SC_KDRV_EVENT_INVALID},
#if (SC_DEV_NUM > 1)		// no more than 2
	{
	 .s32KDrvMutexID = -1,
	 .bKDrvReady = FALSE,
	 .eEvent = E_SC_KDRV_EVENT_INVALID}
#endif
};
#endif

static EN_POWER_MODE _gu16PreSCPowerState = E_POWER_MECHANICAL;
static SC_STR_Ctrl _gstStrCtrl[SC_DEV_NUM] = {
	{
	 .bIsResumeDrv = FALSE,
	 .pfNotify = NULL,
	 },
#if (SC_DEV_NUM > 1)		// no more than 2
	{
	 .bIsResumeDrv = FALSE,
	 .pfNotify = NULL,
	}
#endif
};

//-------------------------------------------------------------------------------------------------
//              Debug funcs
//-------------------------------------------------------------------------------------------------
static const char *const _aszSC_ClkCtrl[] = {
	"E_SC_CLK_3M",
	"E_SC_CLK_4P5M",
	"E_SC_CLK_6M",
	"E_SC_CLK_13M",
	"E_SC_CLK_4M",
	"E_SC_CLK_TSIO_MODE"
};

static const char *const _aszSC_VccCtrl[] = {
	"E_SC_VCC_CTRL_8024_ON",
	"E_SC_VCC_CTRL_LOW",
	"E_SC_VCC_CTRL_HIGH",
	"E_SC_OCP_VCC_HIGH",
	"E_SC_VCC_VCC_ONCHIP_8024"
};

static const char *const _aszSC_VoltageCtrl[] = {
	"E_SC_VOLTAGE_3_POINT_3V",
	"E_SC_VOLTAGE_5V",
	"E_SC_VOLTAGE_3V",
	"E_SC_VOLTAGE_1P8V",
	"E_SC_VOLTAGE_MAX"
};

static const char *const _aszSC_CardDetType[] = {
	"E_SC_HIGH_ACTIVE",
	"E_SC_LOW_ACTIVE"
};

static const char *const _aszSC_TsioCtrl[] = {
	"E_SC_TSIO_DISABLE",
	"E_SC_TSIO_BGA",
	"E_SC_TSIO_CARD"
};

#define SC_PARAM_DBG(P)	   \
	{					  \
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->u8Protocal\t\t=%d\n", __func__, P->u8Protocal); \
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->eCardClk\t\t=%s\n", __func__, _aszSC_ClkCtrl[P->eCardClk]);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->u8UartMode\t\t=%d\n", __func__, P->u8UartMode);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->u16ClkDiv\t\t=%d\n", __func__, P->u16ClkDiv);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->eVccCtrl\t\t=%s\n", __func__, _aszSC_VccCtrl[P->eVccCtrl]);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->u16Bonding\t\t=%d\n", __func__, P->u16Bonding);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->pfOCPControl\t=0x%08x\n", __func__, P->pfOCPControl);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->u8Convention\t=%d\n", __func__, P->u8Convention);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->eVoltage\t\t=%s\n", __func__, _aszSC_VoltageCtrl[P->eVoltage]);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->u8CardDetInvert\t=%s\n", \
__func__, _aszSC_CardDetType[P->u8CardDetInvert]);\
SC_DBG(E_SC_DBGLV_INFO, "%s pParam->eTsioCtrl\t\t=%s\n", __func__, _aszSC_TsioCtrl[P->eTsioCtrl]);\
}

//-------------------------------------------------------------------------------------------------
//              func prototype
//-------------------------------------------------------------------------------------------------
#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(SC_KERNEL_ISR)
static SC_Result _SC_KDrvKernelModeInit(MS_U8 u8SCID);
static void _SC_KDrvKernelModeWakeUp(MS_U8 u8SCID, SC_Event eEvent);
static void _SC_KDrvKernelModeExit(MS_U8 u8SCID);
#endif


static HAL_SC_VOLTAGE_CTRL _SC_ConvHalVoltageCtrl(SC_VoltageCtrl eVoltage);
static HAL_SC_VCC_CTRL _SC_ConvHalVccCtrl(SC_VccCtrl eVccCtrl);
static HAL_SC_CLK_CTRL _SC_ConvHalClkCtrl(SC_ClkCtrl eClkCtrl);
static HAL_SC_CARD_DET_TYPE _SC_ConvHalCardDetType(SC_CardDetType eCardDetType);
static SC_Result _SC_Deactivate(MS_U8 u8SCID);


static SC_Result _SC_Calculate_BGWT(MS_U8 u8SCID, MS_U8 u8BWI);
static SC_Result _SC_Calculate_CGWT(MS_U8 u8SCID, MS_U8 u8CWI);
static SC_Result _SC_Calculate_GWT(MS_U8 u8SCID);
static void _SC_EnableIntCGWT(MS_U8 u8SCID);
static void _SC_DisableIntCGWT(MS_U8 u8SCID);
static void _SC_EnableIntBGWT(MS_U8 u8SCID);
static void _SC_DisableIntBGWT(MS_U8 u8SCID);
static MS_U32 _SC_ConvClkEnumToNum(SC_ClkCtrl eCardClk);
static SC_Result _SC_SetupUartInt(MS_U8 u8SCID);
static MS_U32 _SC_GetRcvWaitingTime(MS_U8 u8SCID);
static SC_Result _SC_CwtFailDoNotRcvData(MS_U8 u8SCID, MS_BOOL bEnable);
static MS_U32 _SC_GetCwtRxErrorIndex(MS_U8 u8SCID);
static MS_U32 _SC_GetTimeDiff(MS_U32 u32CurTime, MS_U32 u32StartTime);
#if (defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR)
static SC_Result _SC_T0ParityErrorKeepRcv(MS_U8 u8SCID, MS_BOOL bEnable);
static SC_Result _SC_SetFailToRstLow(MS_U8 u8SCID, MS_BOOL bEnable);
static SC_Result _SC_Get_Events(MS_U8 u8SCID, MS_U32 *pu32GetEvt);
#endif
static MS_BOOL _SC_CheckRstToAtrTimeout(MS_U8 u8SCID);
static void _SC_RstToAtrSwTimeoutPreProc(MS_U8 u8SCID);
static void _SC_RstToAtrSwTimeoutProc(MS_U8 u8SCID);

static SC_Result _SC_SetupHwFeature(void);


//-------------------------------------------------------------------------------------------------
//              Local funcs
//-------------------------------------------------------------------------------------------------
//for STI
MS_U32 MsOS_GetSystemTime(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void MsOS_DelayTaskUs(MS_U32 u32Us)
{
	/*
	 * Refer to Documentation/timers/timers-howto.txt
	 * Non-atomic context
	 *                              < 10us  : udelay()
	 * 10us ~ 20ms  : usleep_range()
	 *                              > 10ms  : msleep() or msleep_interruptible()
	 */
	if (u32Us < 10UL)
		udelay(u32Us);
	else if (u32Us < 20UL * 1000UL)
		usleep_range(u32Us, u32Us);
	else
		msleep_interruptible((unsigned int)(u32Us / 1000UL));

}

void MsOS_DelayTask(MS_U32 u32Ms)
{
	MsOS_DelayTaskUs(u32Ms * 1000UL);
}

static void _SC_ClkGateCtrl(MS_U8 u8SCID, MS_BOOL enable)
{
#if (SC_DEV_NUM > 1)		// no more than 2
	if (((u8SCID == 0) && (_scInfo[1].bCardIn == FALSE))
	    || ((u8SCID == 1) && (_scInfo[0].bCardIn == FALSE))) {
		HAL_SC_ClkGateCtrl(enable);
	}
#else
	HAL_SC_ClkGateCtrl(enable);
#endif
}

static MS_BOOL _SC_SetOCP(MS_U8 u8SCID, MS_BOOL bCard_In)
{
	return TRUE;
}

static void _SC_ResetFIFO(MS_U8 u8SCID)
{
	if (u8SCID >= SC_DEV_NUM)
		return;
	SC_DBG(E_SC_DBGLV_INFO, "%s %d u8SCID=%d\n", __func__, __LINE__, u8SCID);
	KDrv_SC_ResetFIFO(u8SCID);
}

// Input data byte stream
static MS_S16 _SC_Read(MS_U8 u8SCID, MS_U8 *buf, MS_U16 count)
{
	MS_S16 s16Size;

	if (u8SCID >= SC_DEV_NUM)
		return SC_INT_FAIL;

	s16Size = KDrv_SC_Read(u8SCID, buf, count);
	if (s16Size < 0) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] read error\n", __func__, __LINE__);
		return SC_INT_FAIL;
	}
	return s16Size;
}

static MS_BOOL _SC_WaitTXFree(MS_U8 u8SCID, MS_U32 u32TimeOutMs)
{
	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);
	//MS_U32 curTime = OS_SC_TIME();
	u64 curTime = OS_SC_TIME();
	u64 u64tmpTime;

	if (u8SCID >= SC_DEV_NUM)
		return FALSE;

	//To get TX FIFO status
	while (HAL_SC_IsTxFIFO_Empty(u8SCID) == FALSE) {
		u64tmpTime = js2ms(OS_SC_TIME() - curTime);
		do_div(u64tmpTime, THOUSAND);
		do_div(u64tmpTime, THOUSAND);
		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64tmpTime=%u golden=0,12\n",
			__func__, __LINE__, u64tmpTime);
		if (u64tmpTime > u32TimeOutMs)
			return FALSE;

		if (!HAL_SC_IsCardDetected(u8SCID))
			return FALSE;

		OS_SC_DELAY(1);
	}
	return TRUE;
}

// Output data byte stream
static MS_S16 _SC_Write(MS_U8 u8SCID, MS_U8 *buf, MS_U16 count)
{
	MS_S16 s16Size;

	if (u8SCID >= SC_DEV_NUM)
		return 0;

	//Check uart tx fifo is empty
	if (!_SC_WaitTXFree(u8SCID, SC_WRITE_TIMEOUT)) {
		SC_DBG(E_SC_DBGLV_INFO, "%s %d failed\n", __func__, __LINE__);
		return -1;
	}

	s16Size = (MS_S16) KDrv_SC_Write(u8SCID, buf, count);

	if (s16Size < 0)
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] write error\n", __func__, __LINE__);

	return s16Size;
}

static SC_Result _SC_Reset(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
	HAL_SC_FIFO_CTRL stCtrlFIFO;

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_FAIL;

	if (!_scInfo[u8SCID].bCardIn) {
		SC_DBG(E_SC_DBGLV_INFO, "	 card out 1\n");
		return E_SC_CARDOUT;
	}

	OS_SC_DELAY(SC_RST_SWITCH_TIME);

	if (!_scInfo[u8SCID].bCardIn) {
		SC_DBG(E_SC_DBGLV_INFO, "	 card out 2\n");
		return E_SC_CARDOUT;
	}

	_gScFlowCtrl[u8SCID].bDoReset = TRUE;

	// Disable all interrupts
	HAL_SC_SetUartInt(u8SCID, E_HAL_SC_UART_INT_DISABLE);

	// Reset receiver and transmiter                // FIFO Control Register
	memset(&stCtrlFIFO, 0x00, sizeof(stCtrlFIFO));
	stCtrlFIFO.bEnableFIFO = TRUE;
	stCtrlFIFO.eTriLevel = E_HAL_SC_RX_FIFO_INT_TRI_1;
	stCtrlFIFO.bClearRxFIFO = TRUE;
	HAL_SC_SetUartFIFO(u8SCID, &stCtrlFIFO);

	// Disable all interrupts (Dummy write action.
	// To make Rx FIFO reset really comes into effect)
	HAL_SC_SetUartInt(u8SCID, E_HAL_SC_UART_INT_DISABLE);

	// Check Rx FIFO empty
	while (HAL_SC_IsRxDataReady(u8SCID)) {
		HAL_SC_ReadRxData(u8SCID);
		OS_SC_DELAY(1);
	}

	// Clear interrupt status
	HAL_SC_ClearIntTxLevelAndGWT(u8SCID);


	// enable interrupt
	_SC_SetupUartInt(u8SCID);

	// Clear Rx buffer
	_SC_ResetFIFO(u8SCID);


	//Calculate GT/WT and enable interrupt to detect GT/WT of ATR
	_SC_Calculate_GWT(u8SCID);

	if (_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch == FALSE)
		_SC_EnableIntCGWT(u8SCID);


	// Smart card reset
	// need to disable reset for 5v mode when on chip 8024 mode
	// disable these UART_SCCR_RST flow when on chip 8024 mode,
	// move these code to active sequence code when on chip 8024 mode
	if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_VCC_ONCHIP_8024) {
		HAL_SC_Int8024PullResetPadLow(u8SCID, 1);
		HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_HIGH);
	} else {
		HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
		OS_SC_DELAY(SC_RST_HOLD_TIME);
		HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_HIGH);
	}

	_gScFlowCtrl[u8SCID].bDoReset = FALSE;
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d pass\n", __func__, u8SCID);
	return E_SC_OK;
}


// Set uart divider
static SC_Result _SC_SetUartDiv(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
	HAL_SC_CLK_CTRL eHalClkCtrl = _SC_ConvHalClkCtrl(_scInfo[u8SCID].eCardClk);
	SC_Result eRetVal;

	if (HAL_SC_SetUartDiv(u8SCID, eHalClkCtrl, _scInfo[u8SCID].u16ClkDiv))
		eRetVal = E_SC_OK;
	else
		eRetVal = E_SC_FAIL;

	return eRetVal;
}

static SC_Result _SC_Send(MS_U8 u8SCID, MS_U8 *pu8Data, MS_U16 u16DataLen, MS_U32 u32TimeoutMs)
{
	MS_U16 i = 0;
	MS_S16 len = 0;
	MS_U32 u32StartTime;
	MS_U32 u32GTinMs;
	u64 u64tmpTime;

	SC_DBG(E_SC_DBGLV_INFO, "%s [%d]\n", __func__, __LINE__);
	//[DEBUG]
	int ii;

	pr_info("Tx::: ");
	for (ii = 0; ii < u16DataLen; ii++)
		pr_info("0x%x, ", pu8Data[ii]);
	pr_info("\n");

	//[DEBUG]
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (_scInfo[u8SCID].bCardIn == FALSE) {
		SC_DBG(E_SC_DBGLV_INFO, "	DRV Card OUT\n");
		return E_SC_CARDOUT;
	}
	// Clear continue receive flag
	_gScFlowCtrl[u8SCID].bContRcv = FALSE;
	_gScFlowCtrl[u8SCID].u32RcvedRxLen = 0;
	_SC_ResetFIFO(u8SCID);


	// Before data sent to card, clear character WT of previous rcv and PE flag
	_sc_T0State[u8SCID].u8WT_RxIntCnt = 0;
	_sc_T1State[u8SCID].u8CWT_RxIntCnt = 0;
	_sc_T0State[u8SCID].bParityError = FALSE;
	_sc_T1State[u8SCID].bParityError = FALSE;

	// Wait GT for T=0 to avoid GT failed
	if (_scInfo[u8SCID].u8Protocol == 0) {
		u32GTinMs = _sc_T0State[u8SCID].u32GT / 1000;
		if (_sc_T0State[u8SCID].u32GT % 1000)
			u32GTinMs++;

		OS_SC_DELAY(u32GTinMs);
	}

	u32StartTime = OS_SYSTEM_TIME();

	len = _SC_Write(u8SCID, pu8Data, u16DataLen);

	if (len < 0) {
		SC_DBG(E_SC_DBGLV_INFO, "%s %d failed\n", __func__, __LINE__);
		return E_SC_FAIL;
	}

	while (1) {
		i = i + len;
		if (i == u16DataLen)
			break;

		if (u32TimeoutMs != E_INVALID_TIMEOUT_VAL) {
			u64tmpTime = js2ms(OS_SYSTEM_TIME() - u32StartTime);
			do_div(u64tmpTime, THOUSAND);
			do_div(u64tmpTime, THOUSAND);
			if ((len == 0)
			    && (u64tmpTime > u32TimeoutMs))
				break;
		}

		OS_SC_DELAY(10);

		if (!HAL_SC_IsCardDetected(u8SCID)) {
			SC_DBG(E_SC_DBGLV_INFO, "	DRV Card OUT 2\n");
			return E_SC_CARDOUT;
		}

		len = _SC_Write(u8SCID, pu8Data + i, u16DataLen - i);
		if (len < 0) {
			SC_DBG(E_SC_DBGLV_INFO, "%s %d failed\n", __func__, __LINE__);
			return E_SC_FAIL;
		}
	}

	if (!_SC_WaitTXFree(u8SCID, SC_WRITE_TIMEOUT)) {
		SC_DBG(E_SC_DBGLV_INFO, "%s %d failed\n", __func__, __LINE__);
		return E_SC_TIMEOUT;
	}
	//
	// NOTE: When BWT works as ext-CWT, the RST_TO_IO_EDGE_DET_EN (0x1029_0x0A[7]) must be set
	//
	if (_sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt) {
		_SC_DisableIntBGWT(u8SCID);
		//HAL_SC_RstToIoEdgeDetCtrl(u8SCID, TRUE);//box only
		_SC_EnableIntBGWT(u8SCID);
	}

	return E_SC_OK;
}

static SC_Result _SC_Recv(MS_U8 u8SCID, MS_U8 *pu8Data, MS_U16 *u16DataLen, MS_U32 u32TimeoutMs)
{
	SC_Result eResult = E_SC_OK;
	MS_S16 i = 0;
	MS_S16 len = 0;
	MS_U32 u32CurTime = 0;
	MS_U32 u32TimeDelayStepMs;
	MS_BOOL bUserTimeout = FALSE;
	MS_U32 u32_f;
	MS_U32 u32WorkETU;
	MS_U32 u32SwWt = 0;
	MS_U32 u32SwWtStartTime = 0, u32DiffTime;
	MS_U32 u32SwUserStartTime = 0, u32UserDiffTime;
	MS_U32 u32CwtRxErrorIndex = 0xFFFFFFFFUL;

	SC_DBG(E_SC_DBGLV_INFO, "%s %d\n", __func__, __LINE__);

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_FAIL;

	if (_scInfo[u8SCID].bCardIn == FALSE) {
		SC_DBG(E_SC_DBGLV_INFO, "	DRV Card OUT\n");
		return E_SC_CARDOUT;
	}
	// Set bUserTimeout flag
	if (u32TimeoutMs == E_INVALID_TIMEOUT_VAL) {
		bUserTimeout = FALSE;
		u32SwUserStartTime = 0;
	} else {
		bUserTimeout = TRUE;
		u32SwUserStartTime = MsOS_GetSystemTime();
	}

	// Apply delay step and steup timeout value
	u32TimeDelayStepMs = 1;
	u32SwWt = _SC_GetRcvWaitingTime(u8SCID);

	// As Nagra stress test and irdeto T=14,
	// the HW CWT maybe not be triggered and cause rcv hang-up,
	// so add a SW WT to work around this case of CWT flag not raised
	u32SwWt = 2 * u32SwWt;	//2times CWT

	//ATR and T=0 need to check data rcv WT timeout always
	if (_scInfo[u8SCID].u8Protocol == 0 || _scInfo[u8SCID].u8Protocol == 0xFF) {
		if (_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch
		    && _gScFlowCtrl[u8SCID].bContRcv
		    && _sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt == FALSE)
			u32SwWt = _sc_T0State[u8SCID].stExtWtPatch.u32ExtWtPatchSwWt;


		if (_sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt ==
		    FALSE && _gScFlowCtrl[u8SCID].bRcvATR == FALSE)
			_SC_EnableIntBGWT(u8SCID);

		//If using BWT instead of CWT, do not enable CWT always detect flag
		if (_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch == FALSE)
			HAL_SC_RxFailAlwaysDetCWT(u8SCID, TRUE);

		u32SwWtStartTime = MsOS_GetSystemTime();
	}
	// Display info
	SC_DBG(E_SC_DBGLV_INFO, "%s: bUserTimeout = %d\n", __func__, bUserTimeout);
	SC_DBG(E_SC_DBGLV_INFO, "%s: u32TimeoutMs = " SPEC_U32_DEC " ms\n", __func__, u32TimeoutMs);
	SC_DBG(E_SC_DBGLV_INFO,
	       "%s: u32TimeDelayStepMs = " SPEC_U32_DEC " ms\n", __func__, u32TimeDelayStepMs);
	SC_DBG(E_SC_DBGLV_INFO, "%s: u32SwWt = " SPEC_U32_DEC " ms\n", __func__, u32SwWt);

	len = _SC_Read(u8SCID, pu8Data, *u16DataLen);

	while (i < *u16DataLen) {
		// T=1 need to check data rcv WT timeout always
		if ((_scInfo[u8SCID].u8Protocol == 1) && (i == 0) && (len > 0)) {
			HAL_SC_RxFailAlwaysDetCWT(u8SCID, TRUE);
			u32SwWtStartTime = MsOS_GetSystemTime();
		}

		i = i + len;
		if (i == *u16DataLen) {
			////////////////////////////////////////
			// SW patch for Irdeto stress test
			// Add one etu delay time to make sure
			//the last one byte is transmitted complete IO bus is free
			////////////////////////////////////////
			if (_scInfo[u8SCID].u8Protocol == 0 || _scInfo[u8SCID].u8Protocol == 0xFF) {
				u32_f = _SC_ConvClkEnumToNum(_scInfo[u8SCID].eCardClk);

				//Calculate ETU
				u32WorkETU = (_atr_Fi[_scInfo[u8SCID].u8Fi] /
				_atr_Di[_scInfo[u8SCID].u8Di]) * 1000000UL /
				(u32_f / 1000UL);	// in nsec
				u32WorkETU = (u32WorkETU / 1000UL) + 1;	// in usec
				MsOS_DelayTaskUs(u32WorkETU);	// delay 1 etu
			}
			////////////////////////////////////////
			break;
		}

		OS_SC_DELAY(u32TimeDelayStepMs);

		// Check if card is removed during recv
		if (!HAL_SC_IsCardDetected(u8SCID)) {
			SC_DBG(E_SC_DBGLV_INFO, "	DRV Card OUT 2\n");
			*u16DataLen = i;
			eResult = E_SC_CARDOUT;
			break;
		}
		// T= 0
		if (_scInfo[u8SCID].u8Protocol == 0 || _scInfo[u8SCID].u8Protocol == 0xFF) {
			if (_SC_CheckRstToAtrTimeout(u8SCID)) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "[%s][%d] The duration between RST and ATR is timeout\n",
				       __func__, __LINE__);
				*u16DataLen = 0;
				eResult = E_SC_FAIL;
				break;
			}
			if (_sc_T0State[u8SCID].bParityError) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "[%s][%d] Parity error\n", __func__, __LINE__);
#if !SC_T0_PARITY_ERROR_KEEP_RCV_ENABLE
				*u16DataLen = i;
				eResult = E_SC_DATAERROR;
				break;
#endif
			}
			//To check BWT for T=0
			if (_sc_T1State[u8SCID].u8BWT_IntCnt > 0) {

				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "[%s][%d] BWT error\n", __func__, __LINE__);
				*u16DataLen = i;
				eResult = E_SC_FAIL;
				break;
			}
			//To check CWT if bIsExtWtPatch is disabled
			if (_sc_T0State[u8SCID].u8WT_RxIntCnt > 0
			    && _sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch == FALSE) {
				pr_info("[%s][%d]\n", __func__, __LINE__);
				u32CwtRxErrorIndex = _SC_GetCwtRxErrorIndex(u8SCID);
				if ((u32CwtRxErrorIndex != 0xFFFFFFFFUL)
				    && (u32CwtRxErrorIndex >
					(_gScFlowCtrl[u8SCID].u32RcvedRxLen + i))) {
	//pr_info("u32CwtRxErrorIndex = "SPEC_U32_DEC"\n", u32CwtRxErrorIndex);
	//pr_info("u32RcvedRxLen = "SPEC_U32_DEC"\n", _gScFlowCtrl[u8SCID].u32RcvedRxLen);
	//pr_info("Rcved this loop = %d\n", i);
					len = _SC_Read(u8SCID, pu8Data + i, (*u16DataLen) - i);
					if (len > 0)
						continue;
				}


				SC_DBG(E_SC_DBGLV_INFO,
				       "[%s][%d] CWT RX error\n", __func__, __LINE__);
				*u16DataLen = i;
				eResult = E_SC_TIMEOUT;
				break;
			}
#if PATCH_TO_DISABLE_CGT	//Iverlin: Patch to ignore CGT sent from card
			if (IS_RX_CGT_CHECK_DISABLE == FALSE) {
				if (_sc_T0State[u8SCID].u8GT_RxIntCnt > 0) {
					SC_DBG(E_SC_DBGLV_ERR_ONLY,
					       "[%s][%d] CGT RX error\n", __func__, __LINE__);
					*u16DataLen = i;
					eResult = E_SC_FAIL;
					break;
				}
			}
#endif
		}
		// T= 1
		if (_scInfo[u8SCID].u8Protocol == 1) {
			if (_sc_T1State[u8SCID].bParityError) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "[%s][%d] Parity error\n", __func__, __LINE__);
				*u16DataLen = i;
				eResult = E_SC_DATAERROR;
				break;
			}
			if (_sc_T1State[u8SCID].u8CWT_RxIntCnt > 0) {
				u32CwtRxErrorIndex = _SC_GetCwtRxErrorIndex(u8SCID);
				if ((u32CwtRxErrorIndex != 0xFFFFFFFFUL)
				    && (u32CwtRxErrorIndex >
					(_gScFlowCtrl[u8SCID].u32RcvedRxLen + i))) {
		//pr_info("u32CwtRxErrorIndex = "SPEC_U32_DEC"\n", u32CwtRxErrorIndex);
		//pr_info("u32RcvedRxLen = "SPEC_U32_DEC"\n", _gScFlowCtrl[u8SCID].u32RcvedRxLen);
		//pr_info("Rcved this loop = %d\n", i);
					len = _SC_Read(u8SCID, pu8Data + i, (*u16DataLen) - i);
					if (len > 0)
						continue;
				}


				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "[%s][%d] CWT RX error\n", __func__, __LINE__);
				*u16DataLen = i;
				eResult = E_SC_TIMEOUT;
				break;
			}
#if PATCH_TO_DISABLE_CGT	//Iverlin: Patch to ignore CGT sent from card
			if (IS_RX_CGT_CHECK_DISABLE == FALSE) {
				if (_sc_T1State[u8SCID].u8CGT_RxIntCnt > 0) {
					SC_DBG(E_SC_DBGLV_ERR_ONLY,
					       "[%s][%d] CGT RX error\n", __func__, __LINE__);
					*u16DataLen = i;
					eResult = E_SC_FAIL;
					break;
				}
			}
#endif
#if !PATCH_TO_DISABLE_BGT	//Iverlin: Patch to ignore BGT sent from card
			if (_sc_T1State[u8SCID].u8BGT_IntCnt > 0) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "[%s][%d] BGT error\n", __func__, __LINE__);
				*u16DataLen = i;
				eResult = E_SC_FAIL;
				break;
			}
#endif
			if (_sc_T1State[u8SCID].u8BWT_IntCnt > 0) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "[%s][%d] BWT error\n", __func__, __LINE__);
				*u16DataLen = i;
				eResult = E_SC_FAIL;
				break;
			}
		}

		len = _SC_Read(u8SCID, pu8Data + i, (*u16DataLen) - i);
		if (len < 0) {
			SC_DBG(E_SC_DBGLV_ERR_ONLY, "_SC_Read get fail\n");
			*u16DataLen = i;
			eResult = E_SC_FAIL;
		}
		if (len > 0) {
			u32SwWtStartTime = MsOS_GetSystemTime();

			if (_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch
			    && _sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt == FALSE) {
				u32SwWt = _sc_T0State[u8SCID].stExtWtPatch.u32ExtWtPatchSwWt;
			}
		}

		u32CurTime = MsOS_GetSystemTime();
		if (u32SwWtStartTime != 0) {
			u32DiffTime = _SC_GetTimeDiff(u32CurTime, u32SwWtStartTime);
			if ((i < *u16DataLen) && (u32DiffTime > u32SwWt)) {

				*u16DataLen = i;
	//pr_info("[%s][%d]SW WT Timeout\n", __func__, __LINE__);
	//pr_info("[%s][%d]u32DiffTime = "SPEC_U32_DEC"\n", __func__, __LINE__, u32DiffTime);
	//pr_info("[%s][%d]u32SwWt = "SPEC_U32_DEC"\n", __func__, __LINE__, u32SwWt);
				SC_DBG(E_SC_DBGLV_INFO,
				       "[%s][%d]*u16DataLen = %d\n", __func__,
				       __LINE__, *u16DataLen);
				//eResult =      E_SC_TIMEOUT; //temp
	//pr_info("%s %d eResult =      E_SC_TIMEOUT\n", __func__, __LINE__);
				break;
			}
		}
		if (u32SwUserStartTime != 0 && bUserTimeout) {
			u32UserDiffTime = _SC_GetTimeDiff(u32CurTime, u32SwUserStartTime);
			if ((i < *u16DataLen)
			    && (u32UserDiffTime > u32TimeoutMs)) {

				*u16DataLen = i;
				SC_DBG(E_SC_DBGLV_INFO,
				       "[%s][%d]SW User WT Timeout\n", __func__, __LINE__);
				SC_DBG(E_SC_DBGLV_INFO,
				       "[%s][%d]u32UserDiffTime = " SPEC_U32_DEC
				       "\n", __func__, __LINE__, u32UserDiffTime);
				SC_DBG(E_SC_DBGLV_INFO,
				       "[%s][%d]u32TimeoutMs = " SPEC_U32_DEC
				       "\n", __func__, __LINE__, u32TimeoutMs);
				SC_DBG(E_SC_DBGLV_INFO,
				       "[%s][%d]*u16DataLen = %d\n", __func__,
				       __LINE__, *u16DataLen);
				eResult = E_SC_TIMEOUT;
				break;
			}
		}
	}
	*u16DataLen = i;

	if (_sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt) {
		//HAL_SC_RstToIoEdgeDetCtrl(u8SCID, FALSE);//box only
	} else {
		if (_scInfo[u8SCID].u8Protocol == 0 || _scInfo[u8SCID].u8Protocol == 0xFF)
			_SC_DisableIntBGWT(u8SCID);
	}

	HAL_SC_RxFailAlwaysDetCWT(u8SCID, FALSE);
	_sc_T0State[u8SCID].u8WT_RxIntCnt = 0;
	_sc_T1State[u8SCID].u8CWT_RxIntCnt = 0;

	_gScFlowCtrl[u8SCID].bContRcv = TRUE;
	_gScFlowCtrl[u8SCID].u32RcvedRxLen += *u16DataLen;
	//[DEBUG]
	int ii;

	pr_info("Rx::: ");
	for (ii = 0; ii < *u16DataLen; ii++)
		pr_info("0x%x, ", pu8Data[ii]);

	pr_info("\n");

	//[DEBUG]
	SC_DBG(E_SC_DBGLV_INFO, "%s %d eResult=%d\n", __func__, __LINE__, eResult);
	return eResult;
}

SC_Result _SC_RawExchange(MS_U8 u8SCID, MS_U8 *pu8SendData,
			  MS_U16 *u16SendDataLen, MS_U8 *pu8ReadData,
			  MS_U16 *u16ReadDataLen, MS_U32 u32TimeoutMs)
{
	SC_Result ret_Result = E_SC_OK;

	if (_scInfo[u8SCID].bCardIn == FALSE) {
		SC_DBG(E_SC_DBGLV_INFO, "	DRV Card OUT\n");
		return E_SC_CARDOUT;
	}

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	//Clear SW fifo to reset any remaining data bytes of previous comminication
	_SC_ResetFIFO(u8SCID);

	do {
		ret_Result = _SC_Send(u8SCID, pu8SendData, *u16SendDataLen, u32TimeoutMs);
		if (ret_Result != E_SC_OK)
			break;

		ret_Result = _SC_Recv(u8SCID, pu8ReadData, u16ReadDataLen, u32TimeoutMs);
	} while (0);

	//Add extra delay to ensure interval between RCV and Trans is larger than BGT for T=1
	if (_scInfo[u8SCID].u8Protocol == 1) {
		if (_sc_T1State[u8SCID].u32BGT < 1000)
			OS_DELAY_TASK(1);
		else
			OS_DELAY_TASK(_sc_T1State[u8SCID].u32BGT / 1000);
	}

	return ret_Result;
}



//Copy from _SC_Recv for COVERITY CHECK
static SC_Result _SC_Recv_1_Byte(MS_U8 u8SCID, MS_U8 *pu8Data,
				 MS_U16 *u16DataLen, MS_U32 u32TimeoutMs)
{
	MS_S16 i = 0;
	MS_S16 len = 0;

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_FAIL;

	len = _SC_Read(u8SCID, pu8Data, *u16DataLen);
	MS_U32 countreadtime = u32TimeoutMs / 10;

	for (i = 0; i < *u16DataLen;) {
		i = i + len;
		if (i == *u16DataLen)
			break;

		if ((len == 0) && (countreadtime == 0)) {
			*u16DataLen = i;
			return E_SC_TIMEOUT;
		}
		countreadtime--;
		OS_SC_DELAY(10);
		if (!HAL_SC_IsCardDetected(u8SCID)) {
			SC_DBG(E_SC_DBGLV_INFO, "	DRV Card OUT 2\n");
			*u16DataLen = i;
			return E_SC_CARDOUT;
		}

		len = _SC_Read(u8SCID, pu8Data, (*u16DataLen) - i);
		if (len < 0) {
			SC_DBG(E_SC_DBGLV_ERR_ONLY, "_SC_Read get fail\n");
			*u16DataLen = i;
			return E_SC_FAIL;
		}
	}
	*u16DataLen = i;
	return E_SC_OK;
}

static SC_Result _SC_ClearState(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
	memset(_scInfo[u8SCID].pu8Atr, 0x0, SC_ATR_LEN_MAX);
	memset(_scInfo[u8SCID].pu8Hist, 0x0, SC_HIST_LEN_MAX);
	_scInfo[u8SCID].u8Protocol = 0xFF;
	_scInfo[u8SCID].bSpecMode = FALSE;
	_scInfo[u8SCID].u8Fi = 1;
	_scInfo[u8SCID].u8Di = 1;	// unspecified.
	_scInfoExt[u8SCID].u8N = 0;
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d pass\n", __func__, u8SCID);
	return E_SC_OK;
}

static void _SC_ClearProtocolState(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
	_sc_T0State[u8SCID].u8WI = 10;	//Default is 10 as 7816-3
	_sc_T0State[u8SCID].u32WT = E_TIME_ST_NOT_INITIALIZED;
	_sc_T0State[u8SCID].u32GT = E_TIME_ST_NOT_INITIALIZED;
	_sc_T0State[u8SCID].u8WT_RxIntCnt = 0;
	_sc_T0State[u8SCID].u8WT_TxIntCnt = 0;
	_sc_T0State[u8SCID].u8GT_RxIntCnt = 0;
	_sc_T0State[u8SCID].u8GT_TxIntCnt = 0;
	_sc_T0State[u8SCID].bParityError = FALSE;
	_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch = FALSE;
	_sc_T1State[u8SCID].u8CWI = 13;	//Default is 13 as 7816-3
	_sc_T1State[u8SCID].u8BWI = 4;	//Default is 4 as 7816-3
	_sc_T1State[u8SCID].u32CWT = E_TIME_ST_NOT_INITIALIZED;
	_sc_T1State[u8SCID].u32CGT = E_TIME_ST_NOT_INITIALIZED;
	_sc_T1State[u8SCID].u32BWT = E_TIME_ST_NOT_INITIALIZED;
	_sc_T1State[u8SCID].u32BGT = E_TIME_ST_NOT_INITIALIZED;
	_sc_T1State[u8SCID].u8BWT_IntCnt = 0;
	_sc_T1State[u8SCID].u8BGT_IntCnt = 0;
	_sc_T1State[u8SCID].u8CWT_RxIntCnt = 0;
	_sc_T1State[u8SCID].u8CWT_TxIntCnt = 0;
	_sc_T1State[u8SCID].u8CGT_RxIntCnt = 0;
	_sc_T1State[u8SCID].u8CGT_TxIntCnt = 0;
	_sc_T1State[u8SCID].bParityError = FALSE;
	_gastRstToAtrCtrl[u8SCID].bRstToAtrTimeout = FALSE;
	_gScFlowCtrl[u8SCID].bRcvPPS = FALSE;
	_gScFlowCtrl[u8SCID].bRcvATR = FALSE;
	_gScFlowCtrl[u8SCID].bContRcv = FALSE;
	_gScFlowCtrl[u8SCID].bDoReset = FALSE;
	_gScFlowCtrl[u8SCID].bCwtFailDoNotRcvData = FALSE;
	_gScFlowCtrl[u8SCID].u32RcvedRxLen = 0;
#if PATCH_TO_DISABLE_CGT
	_gbPatchToDisableRxCGTFlag = FALSE;

#if PATCH_TO_DISABLE_CGT_IRDETO_T14
	if ((_scInfo[u8SCID].eCardClk == E_SC_CLK_6M)
	    && (_scInfo[u8SCID].u16ClkDiv == 620)) {
		_gbPatchToDisableRxCGTFlag = TRUE;
	}
#endif


#endif
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d pass\n", __func__, u8SCID);
}


static SC_Result _SC_Activate(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_FAIL;

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);
	// initialize SMART regsiters
	//
	// Smartcard IC Vcc Control
	if (HAL_SC_IsCardDetected(u8SCID)) {
		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);
		if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_VCC_ONCHIP_8024) {
			if (_scInfo[u8SCID].eTsioCtrl == E_SC_TSIO_CARD)
				HAL_SC_SmcClkCtrl(u8SCID, FALSE);	//Disable clock
		}
		_SC_ClkGateCtrl(u8SCID, FALSE);
		if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_CTRL_8024_ON) {
			OS_SC_DELAY(SC_CARD_STABLE_TIME * 10);	// Wait Card stable
			if (_scInfo[u8SCID].eTsioCtrl == E_SC_TSIO_BGA) {
				HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
				HAL_SC_Ext8024ActiveSeqTSIO(u8SCID);
			} else if (_scInfo[u8SCID].eTsioCtrl == E_SC_TSIO_CARD) {
				pr_info("[%s][%d]E_SC_TSIO_CARD not support\n", __func__, __LINE__);
				//return E_SC_FAIL;
			} else {
				SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);
				HAL_SC_Ext8024ActiveSeq(u8SCID);
				HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
			}
		} else if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_VCC_ONCHIP_8024) {
			OS_SC_DELAY(SC_CARD_STABLE_TIME * 20);	// Wait Card stable
			// move "~UART_LCR_SBC" and "UART_SCCR_RST" to here when on chip 8024
			// need to set "~UART_LCR_SBC" here when on chip 8024,
			//otherwise active sequence trigger can't work in IO pin
			if (_scInfo[u8SCID].eTsioCtrl == E_SC_TSIO_CARD) {
				HAL_SC_InputOutputPadCtrl(u8SCID, E_HAL_SC_IO_FORCED_LOW);
				// need to set "UART_SCCR_RST" here when on chip 8024,
				//otherwise active sequence trigger can't work in RST pin
				HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
				HAL_SC_Int8024ActiveSeqTSIO(u8SCID);
			} else if (_scInfo[u8SCID].eTsioCtrl == E_SC_TSIO_BGA) {
				pr_info("[%s][%d] E_SC_TSIO_BGA not support\n", __func__, __LINE__);
				//return E_SC_FAIL;
			} else {
				HAL_SC_InputOutputPadCtrl(u8SCID, E_HAL_SC_IO_FUNC_ENABLE);
				// need to set "UART_SCCR_RST" here when on chip 8024,
				//otherwise active sequence trigger can't work in RST pin
				HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
				HAL_SC_Int8024ActiveSeq(u8SCID);
			}
		} else {
			OS_SC_DELAY(SC_CARD_STABLE_TIME);	// Wait Card stable
			if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_CTRL_HIGH) {
				HAL_SC_SmcVccPadCtrl(u8SCID, E_HAL_SC_LEVEL_HIGH);
			} else if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_CTRL_LOW) {
				HAL_SC_SmcVccPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
			} else {
				//OCP
				_SC_SetOCP(u8SCID, TRUE);
			}
			OS_SC_DELAY(1);	// Wait vcc stable
		}

		// don't need these code when on chip 8024
		if (_scInfo[u8SCID].eVccCtrl != E_SC_VCC_VCC_ONCHIP_8024) {
			SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);
			if (_scInfo[u8SCID].eTsioCtrl == E_SC_TSIO_BGA) {
				HAL_SC_SmcClkCtrl(u8SCID, FALSE);	// Disable clock
				HAL_SC_InputOutputPadCtrl(u8SCID, E_HAL_SC_IO_FORCED_LOW);
			} else {
				HAL_SC_SmcClkCtrl(u8SCID, TRUE);	// enable clock
				HAL_SC_InputOutputPadCtrl(u8SCID, E_HAL_SC_IO_FUNC_ENABLE);
			}
		}
	}
	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d] pass\n", __func__, __LINE__);
	return E_SC_OK;
}

static SC_Result _SC_Deactivate(MS_U8 u8SCID)
{
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_FAIL;

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);


	// Disable BGWT/CGWT interrupt firstly
	_SC_DisableIntBGWT(u8SCID);
	_SC_DisableIntCGWT(u8SCID);



	if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_CTRL_8024_ON) {
		if (_scInfo[u8SCID].eTsioCtrl == E_SC_TSIO_BGA) {
			HAL_SC_Ext8024DeactiveSeqTSIO(u8SCID);
			HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
			OS_SC_DELAY(2);
			HAL_SC_CardVoltage_Config(0, E_HAL_SC_VOL_CTRL_OFF, E_HAL_SC_VCC_INT_8024);
		} else {
			HAL_SC_Ext8024DeactiveSeq(u8SCID);
		}
	} else if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_VCC_ONCHIP_8024) {
		HAL_SC_Int8024DeactiveSeq(u8SCID);
	} else {
		HAL_SC_ResetPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);	//Pull RSTIN low
		OS_SC_DELAY(3);	// Wait Card stable
		HAL_SC_SmcClkCtrl(u8SCID, FALSE);	// clock disable

		OS_SC_DELAY(3);	// Wait Card stable
		HAL_SC_InputOutputPadCtrl(u8SCID, E_HAL_SC_IO_FORCED_LOW);
		OS_SC_DELAY(3);	// Wait Card stable

		if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_CTRL_HIGH) {
			HAL_SC_SmcVccPadCtrl(u8SCID, E_HAL_SC_LEVEL_LOW);
		} else if (_scInfo[u8SCID].eVccCtrl == E_SC_VCC_CTRL_LOW) {
			HAL_SC_SmcVccPadCtrl(u8SCID, E_HAL_SC_LEVEL_HIGH);
		} else {
			//OCP
			_SC_SetOCP(u8SCID, FALSE);
		}
	}

	_SC_ClkGateCtrl(u8SCID, TRUE);

	// Reset to default setting
	HAL_SC_SetUartMode(u8SCID, (SC_UART_CHAR_8 | SC_UART_STOP_2 | SC_UART_PARITY_NO));
	SC_DBG(E_SC_DBGLV_INFO, "%s pass\n", __func__);
	return E_SC_OK;
}

static SC_Result _SC_Close(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s:\n", __func__);

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_FAIL;

	if (_scInfo[u8SCID].bOpened == FALSE)
		return E_SC_FAIL;

	// Disable BGWT/CGWT interrupt firstly
	_SC_DisableIntBGWT(u8SCID);
	_SC_DisableIntCGWT(u8SCID);
	_SC_CwtFailDoNotRcvData(u8SCID, FALSE);



	// Disable all interrupts
	HAL_SC_SetUartInt(u8SCID, E_HAL_SC_UART_INT_DISABLE);

	if (KDrv_SC_DetachInterrupt(u8SCID) != 0) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY,
		       "[%s][%d] KDrv_SC_DetachInterrupt failed\n", __func__, __LINE__);
	}

	_scInfo[u8SCID].bOpened = FALSE;

	HAL_SC_Close(u8SCID, _SC_ConvHalVccCtrl(_scInfo[u8SCID].eVccCtrl));

	// reset buffer control pointer
	_SC_ResetFIFO(u8SCID);

	return E_SC_OK;
}

static SC_Result _SC_ParseATR(MS_U8 u8SCID)
{
	MS_U8 u8T0, u8TAx, u8TBx, u8TCx, u8TDx, u8Len;
	MS_U8 u8Ch, u8Crc = 0;
	MS_BOOL bGetProtocol = FALSE;

	int i, x;

	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
	//[DEBUG]

	pr_info("ATR::: ");
	for (i = 0; i < _scInfo[u8SCID].u16AtrLen; i++)
		pr_info("0x%x, ", _scInfo[u8SCID].pu8Atr[i]);

	pr_info("\n");
	SC_DBG(E_SC_DBGLV_INFO, "%s _scInfo[u8SCID].u16AtrLen=%d\n", __func__,
	       _scInfo[u8SCID].u16AtrLen);
	//[DEBUG]


	if (_scInfo[u8SCID].u16AtrLen < SC_ATR_LEN_MIN) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "	 Length error\n");
		return E_SC_FAIL;
	}
	if ((_scInfo[u8SCID].pu8Atr[0] == 0x23)
	    || (_scInfo[u8SCID].pu8Atr[0] == 0x03)) {
		SC_DBG(E_SC_DBGLV_INFO, "	  Inverse convention\n");

		//try to reverse the inverse data
		if (HAL_SC_SetInvConvention(u8SCID) == FALSE) {
			SC_DBG(E_SC_DBGLV_ERR_ONLY, "	 set inverse error\n");
			return E_SC_FAIL;
		}
		// set all of the ATR data to be inverse convention
		SC_DBG(E_SC_DBGLV_INFO, "ATR::: ");
		for (i = 0; i < _scInfo[u8SCID].u16AtrLen; i++) {
			MS_U8 data0 = ~(_scInfo[u8SCID].pu8Atr[i]);

			data0 = ((data0 >> 1) & 0x55) | ((data0 << 1) & 0xAA);
			data0 = ((data0 >> 2) & 0x33) | ((data0 << 2) & 0xCC);
			data0 = ((data0 >> 4) & 0x0F) | ((data0 << 4) & 0xF0);
			_scInfo[u8SCID].pu8Atr[i] = data0;

			SC_DBG(E_SC_DBGLV_INFO, "0x%x, ", _scInfo[u8SCID].pu8Atr[i]);
		}
		SC_DBG(E_SC_DBGLV_INFO, "\n");
	}
	if ((_scInfo[u8SCID].pu8Atr[0] != 0x3B)
	    && (_scInfo[u8SCID].pu8Atr[0] != 0x3F)) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "	 first byte error 0x%02X\n",
		       _scInfo[u8SCID].pu8Atr[0]);
		return E_SC_FAIL;
	}

	i = 0;
	i++;
	//u8TS = _scInfo[u8SCID].pu8Atr[i++];
	u8T0 = _scInfo[u8SCID].pu8Atr[i++];

	u8Crc = u8Crc ^ u8T0;
	u8Len = u8T0 & 0x0F;
	u8TDx = u8T0 & 0xF0;

	x = 1;
	_scInfo[u8SCID].bSpecMode = FALSE;
	_scInfo[u8SCID].u8Di = _scInfo[u8SCID].u8Fi = 1;
	_scInfo[u8SCID].u8Protocol = 0;
	while (u8TDx & 0xF0) {
		if (i >= _scInfo[u8SCID].u16AtrLen)
			return E_SC_FAIL;


		if (u8TDx & 0x10) {
			// TA
			u8TAx = _scInfo[u8SCID].pu8Atr[i++];
			u8Crc = u8Crc ^ u8TAx;
			if (x == 1) {
				_scInfo[u8SCID].u8Fi = u8TAx >> 4;
				_scInfo[u8SCID].u8Di = u8TAx & 0xF;
				if ((_atr_Fi[_scInfo[u8SCID].u8Fi] == RFU)
				    || (_atr_Di[_scInfo[u8SCID].u8Di] == RFU)) {
					SC_DBG(E_SC_DBGLV_ERR_ONLY, "Di or Fi parsing error\n");
					return E_SC_FAIL;
				}
			}
			if (x == 2) {
				_scInfo[u8SCID].bSpecMode = TRUE;	// specific mode
				SC_DBG(E_SC_DBGLV_INFO,
				       "%s %d _scInfo[u8SCID].bSpecMode = TRUE\n",
				       __func__, __LINE__);
				_scInfo[u8SCID].u8Protocol = u8TAx & 0xF;
				SC_DBG(E_SC_DBGLV_INFO,
				       "%s %d _scInfo[u8SCID].u8Protocol=%d\n",
				       __func__, __LINE__, _scInfo[u8SCID].u8Protocol);
				bGetProtocol = TRUE;
				if ((u8TAx & 0x10)) {
					//As 7816-3 page 19 defined, if bit[5] is '1',
					//then implicit values
					//(not defined by the interface bytes) shall apply
					// TO DO
				}
			}
			if (x == 3)
				_sc_T1State[u8SCID].u8IFSC = u8TAx;

		}
		if (u8TDx & 0x20) {
			// TB
			u8TBx = _scInfo[u8SCID].pu8Atr[i++];
			u8Crc = u8Crc ^ u8TBx;
			if (x >= 3 && _scInfo[u8SCID].u8Protocol == 1) {
				_sc_T1State[u8SCID].u8BWI = u8TBx >> 4;
				_sc_T1State[u8SCID].u8CWI = u8TBx & 0xF;
			}

		}
		if (u8TDx & 0x40) {
			// TC
			u8TCx = _scInfo[u8SCID].pu8Atr[i++];
			u8Crc = u8Crc ^ u8TCx;
			if (x == 1)
				_scInfoExt[u8SCID].u8N = u8TCx;

			if (x == 2)
				_sc_T0State[u8SCID].u8WI = u8TCx;

			if (x == 3)
				_sc_T1State[u8SCID].u8RCType = u8TCx & 0x1;

		}
		if ((u8TDx & 0x80)) {
			// TD
			u8TDx = _scInfo[u8SCID].pu8Atr[i++];

			////////////////////////////////////////////
			//1255824: [Code sync][yellow flag] Smart Card
			// As First offered transmission protocol defined,
			//      - If TD1 is present, then it encodes the first offered protocol T
			//      - If TD1 is absent, then the only offer is T=0
			////////////////////////////////////////////
			if (!bGetProtocol) {
				_scInfo[u8SCID].u8Protocol = u8TDx & 0xF;
				SC_DBG(E_SC_DBGLV_INFO,
				       "%s %d _scInfo[u8SCID].u8Protocol=%d\n",
				       __func__, __LINE__, _scInfo[u8SCID].u8Protocol);
				bGetProtocol = TRUE;
			}
			////////////////////////////////////////////
			u8Crc = u8Crc ^ u8TDx;
			x++;
		} else {
			break;
		}
	}

	// Get ATR history
	_scInfo[u8SCID].u16HistLen = u8Len;
	SC_DBG(E_SC_DBGLV_INFO, "%s %d _scInfo[u8SCID].u16HistLen=%d\n",
	       __func__, __LINE__, _scInfo[u8SCID].u16HistLen);

	//
	// If protocol is T=0, there is no checksum byte.
	// We need to check received bytes number and expect ATR number (from T0~Historical Bytes)
	//
	if (_scInfo[u8SCID].u8Protocol == 0x00) {
		if (_scInfo[u8SCID].u16AtrLen < (i + _scInfo[u8SCID].u16HistLen)) {
			SC_DBG(E_SC_DBGLV_ERR_ONLY, "ATR length error!!\n");
			return E_SC_FAIL;
		}
	}

	_scInfo[u8SCID].u16AtrLen = i + _scInfo[u8SCID].u16HistLen;

	for (x = 0; x < u8Len; x++) {
		if (i >= _scInfo[u8SCID].u16AtrLen)
			return E_SC_FAIL;

		u8Ch = _scInfo[u8SCID].pu8Atr[i++];
		u8Crc = u8Crc ^ u8Ch;

		_scInfo[u8SCID].pu8Hist[x] = u8Ch;
	}

	// Check ATR checksum
	if ((_scInfo[u8SCID].u8Protocol != 0x00) && (_scInfo[u8SCID].u8Protocol != 0xFF)) {
		// If not T=0
		_scInfo[u8SCID].u16AtrLen += 1;

		if (i >= _scInfo[u8SCID].u16AtrLen)
			return E_SC_FAIL;

		if (u8Crc != _scInfo[u8SCID].pu8Atr[i++])
			return E_SC_CRCERROR;

	}
	// Check ATR length
	if (i != _scInfo[u8SCID].u16AtrLen)
		return E_SC_FAIL;	// len error



	if (_scInfo[u8SCID].u8Protocol == 1) {
		_SC_Calculate_CGWT(u8SCID, _sc_T1State[u8SCID].u8CWI);
		_SC_Calculate_BGWT(u8SCID, _sc_T1State[u8SCID].u8BWI);
	} else {
		_SC_Calculate_GWT(u8SCID);
	}

	//Enable interrupt
	if (_scInfo[u8SCID].u8Protocol == 1)
		_SC_EnableIntBGWT(u8SCID);


	if (_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch == FALSE)
		_SC_EnableIntCGWT(u8SCID);


	return E_SC_OK;

}

static MS_BOOL _SC_CheckRstToAtrTimeout(MS_U8 u8SCID)
{
	return _gastRstToAtrCtrl[u8SCID].bRstToAtrTimeout;
}

static void _SC_RstToAtrSwTimeoutPreProc(MS_U8 u8SCID)
{
	KDrv_SC_ResetRstToATR(u8SCID);
}

static void _SC_RstToAtrSwTimeoutProc(MS_U8 u8SCID)
{
	MS_U32 u32RstToAtrPeriod;

	/////////////////////
	// SW_PATCH_RST_IO_TIMEOUT =>
	// HW provides conax_RST_to_IO_edge_detect_en bit to detect RST-to-IO timeout,
	// but it will causes CGT/CWT malfunc since the
	// HW timer is occupied by RST-to-IO.
	// Now we is temporary to use SW to check timeout
	/////////////////////

	_gastRstToAtrCtrl[u8SCID].bRstToAtrTimeout = FALSE;
	u32RstToAtrPeriod =
	    ((1000 * RST_TO_ATR_SW_TIMEOUT) /
	     ((_SC_ConvClkEnumToNum(_scInfo[u8SCID].eCardClk)) / 1000UL));

	if (KDrv_SC_CheckRstToATR(u8SCID, u32RstToAtrPeriod) != 0) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY,
		       "[%s][%d] KDrv_SC_CheckRstToATR failed\n", __func__, __LINE__);
	}

}

static SC_Result _SC_SetupHwFeature(void)
{
	SC_Result eResult = E_SC_OK;

	if (HAL_SC_GetHwFeature(&_gstHwFeature) == FALSE)
		eResult = E_SC_FAIL;

	SC_DBG(E_SC_DBGLV_INFO, "[%s]: eRstToIoHwFeature = " SPEC_U32_DEC "\n",
	       __func__, (MS_U32) _gstHwFeature.eRstToIoHwFeature);
	SC_DBG(E_SC_DBGLV_INFO, "[%s]: bExtCwtFixed = " SPEC_U32_DEC "\n",
	       __func__, (MS_U32) _gstHwFeature.bExtCwtFixed);

	return eResult;
}

static MS_U32 _SC_GetRcvWaitingTime(MS_U8 u8SCID)
{
	MS_U32 u32CWT_TimeoutCountMs = 0;
	//
	// Setup CWT/WT timeout value:
	// Since HW CWT/WT timeout flag cannot be set if no any data bytes received.
	// So we add SW CWT/WT monitor to workaround above use case...
	//
	if (_scInfo[u8SCID].u8Protocol == 0 || _scInfo[u8SCID].u8Protocol == 0xFF) {
		u32CWT_TimeoutCountMs = _sc_T0State[u8SCID].u32WT / 1000;
		if (_sc_T0State[u8SCID].u32WT % 1000)
			u32CWT_TimeoutCountMs += 1;
	} else {
		u32CWT_TimeoutCountMs = _sc_T1State[u8SCID].u32CWT / 1000;
		if (_sc_T1State[u8SCID].u32CWT % 1000)
			u32CWT_TimeoutCountMs += 1;
	}
	if (u32CWT_TimeoutCountMs <= 0)
		u32CWT_TimeoutCountMs = 1;


	return u32CWT_TimeoutCountMs;
}

static SC_Result _SC_CwtFailDoNotRcvData(MS_U8 u8SCID, MS_BOOL bEnable)
{
	pr_info("[%s][%d]bEnable=%d\n", __func__, __LINE__, bEnable);

#if (defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR)
	MS_U32 u32Attr = 0x00;

	KDrv_SC_GetAttribute(u8SCID, &u32Attr);

	if (bEnable)
		u32Attr |= E_SC_ATTR_NOT_RCV_CWT_FAIL;
	else
		u32Attr &= (~E_SC_ATTR_NOT_RCV_CWT_FAIL);

	KDrv_SC_SetAttribute(u8SCID, &u32Attr);

#endif

	if (bEnable)
		_gScFlowCtrl[u8SCID].bCwtFailDoNotRcvData = TRUE;
	else
		_gScFlowCtrl[u8SCID].bCwtFailDoNotRcvData = FALSE;

	return E_SC_OK;
}

static MS_U32 _SC_GetCwtRxErrorIndex(MS_U8 u8SCID)
{
	MS_U32 u32CwtRxErrorIndex = 0xFFFFFFFFUL;

	KDrv_SC_GetCwtRxErrorIndex(u8SCID, &u32CwtRxErrorIndex);

	return u32CwtRxErrorIndex;
}

static MS_U32 _SC_GetTimeDiff(MS_U32 u32CurTime, MS_U32 u32StartTime)
{
	MS_U32 u32DiffTime = E_TIME_ST_NOT_INITIALIZED;

	if (u32CurTime < u32StartTime)
		u32DiffTime = (0xFFFFFFFFUL - u32StartTime) + u32CurTime;
	else
		u32DiffTime = u32CurTime - u32StartTime;


	return u32DiffTime;
}


#if (defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR)
static SC_Result _SC_T0ParityErrorKeepRcv(MS_U8 u8SCID, MS_BOOL bEnable)
{
	MS_U32 u32Attr = 0x00;

	KDrv_SC_GetAttribute(u8SCID, &u32Attr);

	if (bEnable)
		u32Attr |= E_SC_ATTR_T0_PE_KEEP_RCV;
	else
		u32Attr &= (~E_SC_ATTR_T0_PE_KEEP_RCV);

	KDrv_SC_SetAttribute(u8SCID, &u32Attr);

	return E_SC_OK;
}

static SC_Result _SC_SetFailToRstLow(MS_U8 u8SCID, MS_BOOL bEnable)
{
	MS_U32 u32Attr = 0x00;

	KDrv_SC_GetAttribute(u8SCID, &u32Attr);

	if (bEnable)
		u32Attr |= E_SC_ATTR_FAIL_TO_RST_LOW;
	else
		u32Attr &= (~E_SC_ATTR_FAIL_TO_RST_LOW);

	KDrv_SC_SetAttribute(u8SCID, &u32Attr);

	return E_SC_OK;
}

static SC_Result _SC_Get_Events(MS_U8 u8SCID, MS_U32 *pu32GetEvt)
{
	MS_U32 u32Evt = 0x00;

	KDrv_SC_GetEvent(u8SCID, &u32Evt);

	*pu32GetEvt = u32Evt;
	return E_SC_OK;
}
#endif
static SC_Result _SC_SetupUartInt(MS_U8 u8SCID)
{
#if (defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR)
	MS_U32 u32Attr = 0x00;

	KDrv_SC_GetAttribute(u8SCID, &u32Attr);

	u32Attr |= E_SC_ATTR_TX_LEVEL;	// Enable Tx level driven mechanism for data trasmiting

	KDrv_SC_SetAttribute(u8SCID, &u32Attr);

	HAL_SC_SetUartInt(u8SCID, E_HAL_SC_UART_INT_RDI);
#else
	HAL_SC_SetUartInt(u8SCID, E_HAL_SC_UART_INT_RDI);
#endif

	return E_SC_OK;
}

static SC_Result _SC_ParsePPS(MS_U8 u8SCID, MS_U8 *pu8SendData,
			      MS_U16 u16SendDataLen, MS_U8 *pu8ReadData,
			      MS_U16 u16ReadDataLen, MS_BOOL *pbUseDefaultPPS)
{
	SC_Result eResult = E_SC_OK;

	do {
		//ISO 7816 9.3 Successful PPS exchange (Page 21)
		if (u16ReadDataLen < 3) {
			SC_DBG(E_SC_DBGLV_ERR_ONLY,
			       "the length of PPS0_Response is smaller than 3, length = %d\n",
			       u16ReadDataLen);
			eResult = E_SC_PPSFAIL;
			break;
		}
		// Bits1 to 4 of PPS0_Response shall be identical to bits 1 to 4 of PPS0_Request
		if ((pu8ReadData[1] & 0x0f) != (pu8SendData[1] & 0x0f)) {
			SC_DBG(E_SC_DBGLV_ERR_ONLY,
			       "Bits1 to 4 of PPS0_Response is different from PPS0, PPS0=0x%x, PPS0_RESPONSE=0x%x\n",
			       pu8SendData[1], pu8ReadData[1]);
			eResult = E_SC_PPSFAIL;
			break;
		}
		//if bit 5 of PPS0 is set to 1, PPS1_Response shall be identical to PPS1_REQUEST
		if ((pu8ReadData[1] & 0x10) == 0x10) {
			if (pu8ReadData[2] != pu8SendData[2]) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "PPS1_RESPONSE is different from PPS1, PPS1=0x%x, PPS1_REQUEST=0x%x\n",
				       pu8SendData[2], pu8ReadData[2]);
				eResult = E_SC_PPSFAIL;
				break;
			}
		} else {
			//if bit 5 of PPS0 is set to 0, PPS1_Response shall be absent,
			//meaning that Fd and Dd shall be used
			if (u16ReadDataLen != u16SendDataLen - 1) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY,
				       "bit 5 of PPS0 is set to 0, but PPS1_Response exists,\n");
				eResult = E_SC_PPSFAIL;
				break;
			}

			eResult = E_SC_OK;


			*pbUseDefaultPPS = TRUE;
		}
	} while (0);

	return eResult;
}

static void _SC_EnableIntCGWT(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	if (_scInfo[u8SCID].u8Protocol == 0 || _scInfo[u8SCID].u8Protocol == 0xFF) {
		_sc_T0State[u8SCID].u8WT_TxIntCnt = 0;
		_sc_T0State[u8SCID].u8WT_RxIntCnt = 0;
		_sc_T0State[u8SCID].u8GT_TxIntCnt = 0;
		_sc_T0State[u8SCID].u8GT_RxIntCnt = 0;
	}
	if (_scInfo[u8SCID].u8Protocol == 1) {
		_sc_T1State[u8SCID].u8CWT_TxIntCnt = 0;
		_sc_T1State[u8SCID].u8CWT_RxIntCnt = 0;
		_sc_T1State[u8SCID].u8CGT_TxIntCnt = 0;
		_sc_T1State[u8SCID].u8CGT_RxIntCnt = 0;
	}

	HAL_SC_SetIntCGWT(u8SCID, E_HAL_SC_CGWT_INT_ENABLE);
}

static void _SC_DisableIntCGWT(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	HAL_SC_SetIntCGWT(u8SCID, E_HAL_SC_CGWT_INT_DISABLE);

	if (_scInfo[u8SCID].u8Protocol == 0 || _scInfo[u8SCID].u8Protocol == 0xFF) {
		_sc_T0State[u8SCID].u8WT_TxIntCnt = 0;
		_sc_T0State[u8SCID].u8WT_RxIntCnt = 0;
		_sc_T0State[u8SCID].u8GT_TxIntCnt = 0;
		_sc_T0State[u8SCID].u8GT_RxIntCnt = 0;
	}
	if (_scInfo[u8SCID].u8Protocol == 1) {
		_sc_T1State[u8SCID].u8CWT_TxIntCnt = 0;
		_sc_T1State[u8SCID].u8CWT_RxIntCnt = 0;
		_sc_T1State[u8SCID].u8CGT_TxIntCnt = 0;
		_sc_T1State[u8SCID].u8CGT_RxIntCnt = 0;
	}
}

static void _SC_EnableIntBGWT(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	_sc_T1State[u8SCID].u8BWT_IntCnt = 0;
	_sc_T1State[u8SCID].u8BGT_IntCnt = 0;
	HAL_SC_SetIntBGWT(u8SCID, E_HAL_SC_BGWT_INT_ENABLE);
}

static void _SC_DisableIntBGWT(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	HAL_SC_SetIntBGWT(u8SCID, E_HAL_SC_BGWT_INT_DISABLE);
	_sc_T1State[u8SCID].u8BWT_IntCnt = 0;
	_sc_T1State[u8SCID].u8BGT_IntCnt = 0;
}

static MS_U32 _SC_ConvClkEnumToNum(SC_ClkCtrl eCardClk)
{
	MS_U32 u32Clk;

	switch (eCardClk) {
	case E_SC_CLK_3M:
		u32Clk = 3375000UL;
		break;

	case E_SC_CLK_4P5M:
	default:
		u32Clk = 4500000UL;
		break;

	case E_SC_CLK_6M:
		u32Clk = 6750000UL;
		break;

	case E_SC_CLK_13M:
		u32Clk = 13500000UL;
		break;
	}

	return u32Clk;
}

static SC_Result _SC_Calculate_BGWT(MS_U8 u8SCID, MS_U8 u8BWI)
{
	MS_U64 u64_f;
	MS_U32 u32WorkETU;
	MS_U32 u32RegBWT;
	MS_U64 u64TempBWT = 0;
	MS_U64 u64ETU = 0;
	MS_U64 u64NsecBase = 1000000000;
	MS_U64 u64Temp_test;
	MS_U64 u64_f_temp;
	MS_U64 u64WorkETU;

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);
	SC_DBG(E_SC_DBGLV_INFO, "***** Calculate_BGWT *****\n");

	u64_f = (MS_U64) _SC_ConvClkEnumToNum(_scInfo[u8SCID].eCardClk);

	//Calculate ETU = (Fi/Di) * (1/f)  (seconds)
	u32WorkETU = (_atr_Fi[_scInfo[u8SCID].u8Fi] /
				_atr_Di[_scInfo[u8SCID].u8Di]) *
				1000000UL;
	u64_f_temp = u64_f;
	do_div(u64_f_temp, 1000UL);

	u64WorkETU = u32WorkETU;
	do_div(u64WorkETU, u64_f_temp);	// in nsec
	u32WorkETU = u64WorkETU;

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u32WorkETU=%u golden=41333\n",
		__func__, __LINE__, u32WorkETU);

	SC_DBG(E_SC_DBGLV_INFO, "***** Work etu = " SPEC_U32_DEC " nsec *****\n", u32WorkETU);

	//////////////////////////////////////////////
	//BWT = 11 etu + 2 ^ ( BWI ) * 960 * ( Fd / f )  (seconds)
	//BWT = BWT * COMPENSATION/COMPENSATION_DIVISOR
	//////////////////////////////////////////////
	u64TempBWT = (MS_U64) ((((MS_U64) 1) << u8BWI) * 960 * 372);
	u64TempBWT = (MS_U64) (u64TempBWT * u64NsecBase);	// in nsec

	u64TempBWT = u64TempBWT * 10;	// in nsec*10

	do_div(u64TempBWT, u64_f);	// in nsec*10

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64TempBWT=%u golden=4107665408\n",
		__func__, __LINE__, u64TempBWT);


	u64ETU = (MS_U64) u32WorkETU;	//nsec

	do_div(u64TempBWT, u64ETU);    //etu*10

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64TempBWT=%u golden=307202\n",
		__func__, __LINE__, u64TempBWT);

	u64TempBWT = u64TempBWT + 10 + 110;	//etu*10

	u64TempBWT = (MS_U64) ((u64TempBWT * (MS_U64) SC_TIMING_COMPENSATION));
	do_div(u64TempBWT, SC_TIMING_COMPENSATION_DIVISOR);	//etu*10

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64TempBWT=%u\n golden=307328",
		__func__, __LINE__, u64TempBWT);


	//To check and do round off if necessary
	u64Temp_test = u64TempBWT;
	if (do_div(u64Temp_test, TEN) > FIVE) {
		do_div(u64TempBWT, TEN);
		u64TempBWT = u64TempBWT + 1;    //etu
		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64TempBWT=%u golden=30733\n",
			__func__, __LINE__, u64TempBWT);
	} else {
		do_div(u64TempBWT, TEN);    //etu
		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64TempBWT=%u\n", __func__, __LINE__, u64TempBWT);
	}


	u32RegBWT = (MS_U32) (u64TempBWT & 0xFFFFFFFF);

	u64TempBWT = (u64TempBWT * (MS_U64) u32WorkETU);
	do_div(u64TempBWT, THOUSAND);	// in usec

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64TempBWT=%u golden=1270287\n",
		__func__, __LINE__, u64TempBWT);

	_sc_T1State[u8SCID].u32BWT = (MS_U32) (u64TempBWT & 0xFFFFFFFF);
	HAL_SC_SetBWT(u8SCID, u32RegBWT);


	// BGT = 22 etu
	_sc_T1State[u8SCID].u32BGT = 22 * u32WorkETU / 1000UL;	// in usec
	HAL_SC_SetBGT(u8SCID, 22);

	SC_DBG(E_SC_DBGLV_INFO, "***** u32RegBWT = 0x" SPEC_U32_HEX " *****\n", u32RegBWT);
	SC_DBG(E_SC_DBGLV_INFO, "***** BWT = " SPEC_U32_DEC " usec *****\n",
	       _sc_T1State[u8SCID].u32BWT);
	SC_DBG(E_SC_DBGLV_INFO, "***** BGT = " SPEC_U32_DEC " usec *****\n\n",
	       _sc_T1State[u8SCID].u32BGT);

	return E_SC_OK;
}

static SC_Result _SC_Calculate_CGWT(MS_U8 u8SCID, MS_U8 u8CWI)
{
	MS_U32 u32_f;
	MS_U32 u32WorkETU;
	MS_U32 u32RegCWT;
	MS_U8 u8ExtraGT = 0;
	MS_U8 u8CGT = 0;
	MS_U64 u64Temp;
	MS_U64 u64Temp_test;

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);
	SC_DBG(E_SC_DBGLV_INFO, "***** Calculate_CGWT *****\n");

	u32_f = (MS_U64) _SC_ConvClkEnumToNum(_scInfo[u8SCID].eCardClk);

	//Calculate ETU = (Fi/Di) * (1/f)  (seconds)
	u32WorkETU = (_atr_Fi[_scInfo[u8SCID].u8Fi] /
			_atr_Di[_scInfo[u8SCID].u8Di]) *
			1000000UL / (u32_f / 1000UL);	// in nsec
	SC_DBG(E_SC_DBGLV_INFO, "***** Work etu = " SPEC_U32_DEC " nsec *****\n", u32WorkETU);

	if (u8CWI == 0xFF) {	//For PPS case
		//CWT
		u64Temp = (MS_U64) 9600;	//etu
		u64Temp = u64Temp * 10;	//etu*10

		u64Temp = (MS_U64) ((u64Temp * (MS_U64) SC_TIMING_COMPENSATION));
		do_div(u64Temp, SC_TIMING_COMPENSATION_DIVISOR);	//etu*10

		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u\n", __func__, __LINE__, u64Temp);

		//To check and do round off if necessary
		do_div(u64Temp, TEN);	//etu

		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u\n", __func__, __LINE__, u64Temp);

		u32RegCWT = (MS_U32) (u64Temp & 0xFFFFFFFF);

		u64Temp = (u64Temp * (MS_U64) u32WorkETU);
		do_div(u64Temp, THOUSAND);	// in usec

		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u\n", __func__, __LINE__, u64Temp);

		_sc_T1State[u8SCID].u32CWT = (MS_U32) (u64Temp & 0xFFFFFFFF);	// in usec
		SC_DBG(E_SC_DBGLV_INFO,
		       "***** PPS u32RegCWT = 0x" SPEC_U32_HEX " *****\n", u32RegCWT);
		SC_DBG(E_SC_DBGLV_INFO,
		       "***** PPS CWT = " SPEC_U32_DEC " usec *****\n", _sc_T1State[u8SCID].u32CWT);

		//CGT
		//Calculate extra guard time
		if (_scInfoExt[u8SCID].u8N == 0xFF) {
			u8ExtraGT = 0;
			u8CGT = 12;	//12etu
		} else {
			u8ExtraGT = _scInfoExt[u8SCID].u8N;
			u8CGT = 12 + u8ExtraGT;	//12etu + R*(N/f). R = Fi/Di
		}

		// GT = 12 etu + R * N / f, R = F/D
		_sc_T0State[u8SCID].u32GT = ((MS_U32) u8CGT * u32WorkETU) / 1000UL;	// in usec
		SC_DBG(E_SC_DBGLV_INFO,
		       "***** PPS extra GT integer = %u	 *****\n", u8ExtraGT);
		SC_DBG(E_SC_DBGLV_INFO,
		       "***** PPS GT = " SPEC_U32_DEC " usec *****\n", _sc_T0State[u8SCID].u32GT);
	} else {
		//CWT = (11+2^CWI) * etu
		u64Temp = (MS_U64) (11 + (((MS_U64) 1) << u8CWI));
		u64Temp = u64Temp * 10;	//etu*10

		u64Temp = (MS_U64) ((u64Temp * (MS_U64) SC_TIMING_COMPENSATION));
		do_div(u64Temp, SC_TIMING_COMPENSATION_DIVISOR);	//etu*10

		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u golden=430\n",
			__func__, __LINE__, u64Temp);

		//To check and do round off if necessary
		u64Temp_test = u64Temp;
		if (do_div(u64Temp_test, TEN) > FIVE) {
			do_div(u64Temp, TEN);
			u64Temp = u64Temp + 1;	//etu
			SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u\n",
				__func__, __LINE__, u64Temp);
		} else {
			do_div(u64Temp, TEN);	//etu
			SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u golden=43\n",
				__func__, __LINE__, u64Temp);
		}


		u32RegCWT = (MS_U32) (u64Temp & 0xFFFFFFFF);

		u64Temp = (u64Temp * (MS_U64) u32WorkETU);
		do_div(u64Temp, THOUSAND);	// in usec

		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u golden=1777\n",
			__func__, __LINE__, u64Temp);

		_sc_T1State[u8SCID].u32CWT = (MS_U32) (u64Temp & 0xFFFFFFFF);	// in usec
		SC_DBG(E_SC_DBGLV_INFO, "***** u32RegCWT = 0x" SPEC_U32_HEX " *****\n", u32RegCWT);
		SC_DBG(E_SC_DBGLV_INFO,
		       "***** CWT = " SPEC_U32_DEC " usec *****\n", _sc_T1State[u8SCID].u32CWT);

		//CGT
		if (_scInfoExt[u8SCID].u8N == 0xFF) {
			//As 7816-3 page 24 (ch 11.2), the CGT = 11etu if N = 255
			u8CGT = 11;
			_sc_T1State[u8SCID].u32CGT =
			((MS_U32) u8CGT) * u32WorkETU / 1000UL;	// in usec
		} else {
			//As 7816-3 page 24 (ch 11.2), the CGT = GT if N = 0~254
			u8ExtraGT = _scInfoExt[u8SCID].u8N;
			u8CGT = 12 + u8ExtraGT;

			// GT = 12 etu + R * N / f, R = F/D
			_sc_T1State[u8SCID].u32CGT = (u8CGT * u32WorkETU) / 1000UL;	// in usec
		}
		SC_DBG(E_SC_DBGLV_INFO,
		       "***** CGT = " SPEC_U32_DEC " usec *****\n\n", _sc_T1State[u8SCID].u32CGT);
	}

	HAL_SC_SetCWT(u8SCID, u32RegCWT);
	HAL_SC_SetCGT(u8SCID, u8CGT);

	_SC_CwtFailDoNotRcvData(u8SCID, TRUE);

	return E_SC_OK;
}


static SC_Result _SC_Calculate_GWT(MS_U8 u8SCID)
{
	MS_U32 u32_f;
	MS_U32 u32WorkETU;
	MS_U32 u32RegCWT = 0;
	MS_U8 u8ExtraGT = 0;
	MS_U8 u8CGT = 0;
	MS_U64 u64Temp;
	MS_U64 u64Temp_test;

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]\n", __func__, __LINE__);

	SC_DBG(E_SC_DBGLV_INFO, "***** Calculate_GWT *****\n");

	_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch = FALSE;
	_sc_T0State[u8SCID].stExtWtPatch.u32ExtWtPatchSwWt = 0xFFFFFFFFUL;
	u32_f = (MS_U64) _SC_ConvClkEnumToNum(_scInfo[u8SCID].eCardClk);

	//Calculate ETU
	u32WorkETU = (_atr_Fi[_scInfo[u8SCID].u8Fi] /
					_atr_Di[_scInfo[u8SCID].u8Di]) *
					1000000UL / (u32_f / 1000UL);	// in nsec
	SC_DBG(E_SC_DBGLV_INFO,
	       "***** Work etu = " SPEC_U32_DEC " nsec *****\n", u32WorkETU);

	//===== Calculate WT =====
	u64Temp =
	    (MS_U64) (_sc_T0State[u8SCID].u8WI * 960UL *
		      _atr_Di[_scInfo[u8SCID].u8Di]);
	u64Temp = u64Temp * 10;	//etu*10

	u64Temp = (MS_U64) ((u64Temp * (MS_U64) SC_TIMING_COMPENSATION));
	do_div(u64Temp, SC_TIMING_COMPENSATION_DIVISOR);	//etu*10

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u golden=96001\n",
		__func__, __LINE__, u64Temp);

	//To check and do round off if necessary

	u64Temp_test = u64Temp;
	if (do_div(u64Temp_test, TEN) > FIVE) {
		do_div(u64Temp, TEN);
		u64Temp = u64Temp + 1;	//etu
		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u\n", __func__, __LINE__, u64Temp);
	} else {
		do_div(u64Temp, TEN);	//etu
		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u golden=9600\n",
			__func__, __LINE__, u64Temp);
	}

	u32RegCWT = (MS_U32) (u64Temp & 0xFFFFFFFF);

	u64Temp = (u64Temp * (MS_U64) u32WorkETU);
	do_div(u64Temp, THOUSAND);	// in usec

	SC_DBG(E_SC_DBGLV_INFO, "[%s][%d]u64Temp=%u golden=793593\n",
		__func__, __LINE__, u64Temp);

	_sc_T0State[u8SCID].u32WT = (MS_U32) (u64Temp & 0xFFFFFFFF);	// in usec

	if (u32RegCWT & 0xFFFF0000) {
		//////////////////////////////////////////////
		// This is a HW patch for extent CWT issue.
		// We can use BWT func as ext-CWT func if the following conditions are conformed
		// 1. The HW BWT counter is independent of Reset-to-I/O counter
		// (eRstToIoHwFeature == E_SC_RST_TO_IO_HW_TIMER_IND)
		// 2. The HW extent CWT issue is not fixed (bExtCwtFixed)
		//
		// NOTE: When BWT works as ext-CWT,
		//               the RST_TO_IO_EDGE_DET_EN (0x1029_0x0A[7]) must be set
		// ///////////////////////////////////////////
		if (_gstHwFeature.bExtCwtFixed == FALSE) {
			_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch = TRUE;

			if (_gstHwFeature.eRstToIoHwFeature == E_SC_RST_TO_IO_HW_TIMER_IND) {
				_sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt = TRUE;
				HAL_SC_SetBWT(u8SCID, u32RegCWT);
				HAL_SC_SetCWT(u8SCID, 0x10000UL);
			} else {
				_sc_T0State[u8SCID].stExtWtPatch.u32ExtWtPatchSwWt =
				    GET_SW_EXTWT_IN_MS(_SC_GetRcvWaitingTime
						       (u8SCID), u32WorkETU,
						       _atr_Di[_scInfo[u8SCID].u8Di]);
				_sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt = FALSE;
				HAL_SC_SetBWT(u8SCID, u32RegCWT);	//To detect no data
				HAL_SC_SetCWT(u8SCID, 0x10000UL);
			}

			_SC_CwtFailDoNotRcvData(u8SCID, FALSE);
		} else {
			_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch = FALSE;
			_sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt = FALSE;
			_SC_CwtFailDoNotRcvData(u8SCID, TRUE);
			HAL_SC_SetCWT(u8SCID, u32RegCWT);
		}
	} else {
		_sc_T0State[u8SCID].stExtWtPatch.u32ExtWtPatchSwWt =
		    GET_SW_EXTWT_IN_MS(_SC_GetRcvWaitingTime(u8SCID),
				       u32WorkETU, _atr_Di[_scInfo[u8SCID].u8Di]);
		_sc_T0State[u8SCID].stExtWtPatch.bUseBwtInsteadExtCwt = FALSE;
		if (_gScFlowCtrl[u8SCID].bDoReset)
			HAL_SC_SetBWT(u8SCID, 0xFFFFFFFF);	//Set to default
		else
			HAL_SC_SetBWT(u8SCID, u32RegCWT);	//To detect no data
		HAL_SC_SetCWT(u8SCID, u32RegCWT);
		_SC_CwtFailDoNotRcvData(u8SCID, TRUE);
	}
	SC_DBG(E_SC_DBGLV_INFO, "***** WT reg = 0x" SPEC_U32_HEX " *****\n", u32RegCWT);
	SC_DBG(E_SC_DBGLV_INFO, "***** WT = " SPEC_U32_DEC " usec *****\n",
	       _sc_T0State[u8SCID].u32WT);
	SC_DBG(E_SC_DBGLV_INFO, "***** bIsExtWtPatch = %u *****\n",
	       _sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch);

	//===== Calculate GT =====
	//Calculate extra guard time
	if (_scInfoExt[u8SCID].u8N == 0xFF)
		u8ExtraGT = 0;
	else
		u8ExtraGT = _scInfoExt[u8SCID].u8N;

	//Add 1 etu to pass "delay at least 2 etu after detection of error signal"
	u8CGT = u8ExtraGT + 12;
	HAL_SC_SetCGT(u8SCID, u8CGT);

	// GT = 12 etu + R * N / f, R = F/D
	_sc_T0State[u8SCID].u32GT = (u8CGT * u32WorkETU) / 1000UL;	// in usec

	SC_DBG(E_SC_DBGLV_INFO, "***** extra GT integer = 0x%x	*****\n", u8ExtraGT);
	SC_DBG(E_SC_DBGLV_INFO, "***** GT = " SPEC_U32_DEC " usec *****\n\n",
	       _sc_T0State[u8SCID].u32GT);

	return E_SC_OK;
}

static SC_Result _SC_GetATR(MS_U8 u8SCID, MS_U32 u32TimeoutMs)
{
	SC_Result ScResult = E_SC_OK;

	SC_DBG(E_SC_DBGLV_INFO, "%s:\n", __func__);
	if (_scInfo[u8SCID].bCardIn == FALSE) {
		SC_DBG(E_SC_DBGLV_INFO, "	DRV Card OUT\n");
		return E_SC_CARDOUT;
	}
	//Initialize
	_sc_T0State[u8SCID].u8WI = 10;	//Default is 10 as 7816-3
	_sc_T1State[u8SCID].u8CWI = 13;	//Default is 13 as 7816-3
	_sc_T1State[u8SCID].u8BWI = 4;	//Default is 13 as 7816-3
	_scInfo[u8SCID].u8Fi = 1;
	_scInfo[u8SCID].u8Di = 1;
	_scInfoExt[u8SCID].u8N = 0;

	//Get ATR data
	(void)u32TimeoutMs;
	_scInfo[u8SCID].u16AtrLen = SC_ATR_LEN_MAX;
	_gScFlowCtrl[u8SCID].bRcvATR = TRUE;
	ScResult =
	    _SC_Recv(u8SCID, _scInfo[u8SCID].pu8Atr,
		     &(_scInfo[u8SCID].u16AtrLen), E_INVALID_TIMEOUT_VAL);
	_gScFlowCtrl[u8SCID].bRcvATR = FALSE;
	//This is paired to GWT control and int enable loacted at _sc_reset()
	_SC_DisableIntCGWT(u8SCID);
/*
 *	//debug for T=2
 *	_scInfo[u8SCID].u16AtrLen = 15;
 *	_scInfo[u8SCID].pu8Atr[0] = 0x3b;
 *	_scInfo[u8SCID].pu8Atr[1] = 0xf2;
 *	_scInfo[u8SCID].pu8Atr[2] = 0x13;
 *	_scInfo[u8SCID].pu8Atr[3] = 0x00;
 *	_scInfo[u8SCID].pu8Atr[4] = 0xff;
 *	_scInfo[u8SCID].pu8Atr[5] = 0x91;
 *	_scInfo[u8SCID].pu8Atr[6] = 0x81;
 *	_scInfo[u8SCID].pu8Atr[7] = 0xb1;
 *	_scInfo[u8SCID].pu8Atr[8] = 0xfe;
 *	_scInfo[u8SCID].pu8Atr[9] = 0x46;
 *	_scInfo[u8SCID].pu8Atr[10] = 0x1f;
 *	_scInfo[u8SCID].pu8Atr[11] = 0x03;
 *	_scInfo[u8SCID].pu8Atr[12] = 0x90;
 *	_scInfo[u8SCID].pu8Atr[13] = 0x0a;	// T2
 *	_scInfo[u8SCID].pu8Atr[14] = 0x81;	// TCK
 */

/*	//debug for T=1
 *	_scInfo[u8SCID].u16AtrLen = 13;
 *	_scInfo[u8SCID].pu8Atr[0] = 0x3b;
 *	_scInfo[u8SCID].pu8Atr[1] = 0xf0;
 *	_scInfo[u8SCID].pu8Atr[2] = 0x12;
 *	_scInfo[u8SCID].pu8Atr[3] = 0x00;
 *	_scInfo[u8SCID].pu8Atr[4] = 0xff;
 *	_scInfo[u8SCID].pu8Atr[5] = 0x91;
 *	_scInfo[u8SCID].pu8Atr[6] = 0x81;
 *	_scInfo[u8SCID].pu8Atr[7] = 0xb1;
 *	_scInfo[u8SCID].pu8Atr[8] = 0x7C;
 *	_scInfo[u8SCID].pu8Atr[9] = 0x45;
 *	_scInfo[u8SCID].pu8Atr[10] = 0x1f;
 *	_scInfo[u8SCID].pu8Atr[11] = 0x03;
 *	_scInfo[u8SCID].pu8Atr[12] = 0x99;
 */

/*	//debug for ATR
 *	MS_U8 SetTestPatten1[13];
 *	int ii;
 *
 *	SetTestPatten1[0] = 0x3b;
 *	SetTestPatten1[1] = 0xf0;
 *	SetTestPatten1[2] = 0x12;
 *	SetTestPatten1[3] = 0x00;
 *	SetTestPatten1[4] = 0xff;
 *	SetTestPatten1[5] = 0x91;
 *	SetTestPatten1[6] = 0x81;
 *	SetTestPatten1[7] = 0xb1;
 *	SetTestPatten1[8] = 0x7C;
 *	SetTestPatten1[9] = 0x45;
 *	SetTestPatten1[10] = 0x1f;
 *	SetTestPatten1[11] = 0x03;
 *	SetTestPatten1[12] = 0x99;
 *
 *	if(_scInfo[u8SCID].u16AtrLen != 13)
 *	{
 *		pr_info("[%s][%d] ATR len failed\n", __func__, __LINE__);
 *		return E_SC_FAIL;
 *	}
 *	for (ii = 0; ii < 13; ii++)
 *	{
 *		if(_scInfo[u8SCID].pu8Atr[ii] != SetTestPatten1[ii])
 *		{
 *			pr_info("[%s][%d] ATR failed\n", __func__, __LINE__);
 *			return E_SC_FAIL;
 *		}
 *	}
 */


	//Check HW RstToIoEdgeDet

	//Clear bRstToAtrTimeout
	_gastRstToAtrCtrl[u8SCID].bRstToAtrTimeout = FALSE;

	if (ScResult == E_SC_OK || ScResult == E_SC_TIMEOUT) {
		//Parse ATR data
		if (_scInfo[u8SCID].u16AtrLen == 0)
			ScResult = E_SC_NODATA;
		else
			ScResult = _SC_ParseATR(u8SCID);

	}

	return ScResult;
}



static SC_Result _SC_PPS(MS_U8 u8SCID, MS_U8 u8T, MS_U8 u8DI, MS_U8 u8FI)
{
	SC_Result eResult = E_SC_OK;
	MS_U8 u8SendData[SC_PPS_LEN_MAX] = { 0 };
	MS_U16 u16SendLen = 0;
	MS_U8 u8TempWI = _sc_T0State[u8SCID].u8WI;
	MS_U8 u8TempProtocol = _scInfo[u8SCID].u8Protocol;
	MS_BOOL bUseDefaultPPS = FALSE;
	MS_U8 au8PPS[SC_PPS_LEN_MAX];
	MS_U16 u16PPSLen;

	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d u8T=%d u8DI=%d u8FI=%d\n",
	       __func__, u8SCID, u8T, u8DI, u8FI);
	if ((_atr_Fi[u8FI] == RFU) || (_atr_Di[u8DI] == RFU))
		return E_SC_FAIL;

	//NOTE: Just consider D/F for "work etc" here
	au8PPS[0] = u8SendData[0] = 0xFF;	// PPSS
	au8PPS[1] = u8SendData[1] = 0x10 | (0x0F & u8T);	// T=x
	au8PPS[2] = u8SendData[2] = u8DI | (u8FI << 4);	// PPS1
	au8PPS[3] = u8SendData[3] = au8PPS[0] ^ au8PPS[1] ^ au8PPS[2];	// PCK
	u16PPSLen = u16SendLen = 4;

	// Disable BGWT/CGWT interrupt firstly
	_SC_DisableIntBGWT(u8SCID);
	_SC_DisableIntCGWT(u8SCID);

	//Setup WT and GT for PPS
	//use default value for PPS negotiation
	_scInfo[u8SCID].u8Fi = 1;
	_scInfo[u8SCID].u8Di = 1;
	_sc_T0State[u8SCID].u8WI = 10;
	_scInfo[u8SCID].u8Protocol = 0;
	_SC_Calculate_GWT(u8SCID);

	_gScFlowCtrl[u8SCID].bRcvPPS = TRUE;
	_SC_EnableIntCGWT(u8SCID);
	do {
		eResult = _SC_Send(u8SCID, u8SendData, u16SendLen, SC_T0_SEND_TIMEOUT);
		if (eResult != E_SC_OK) {
			SC_DBG(E_SC_DBGLV_ERR_ONLY, "	 PPS Send Failed!!\n");
			break;
		}
		eResult = _SC_Recv(u8SCID, au8PPS, &u16PPSLen, E_INVALID_TIMEOUT_VAL);
	} while (0);
	_SC_DisableIntCGWT(u8SCID);
	_gScFlowCtrl[u8SCID].bRcvPPS = FALSE;

	//Parse PPS
	if (eResult == E_SC_OK || eResult == E_SC_TIMEOUT) {
		eResult =
		    _SC_ParsePPS(u8SCID, u8SendData, u16SendLen, au8PPS,
				 u16PPSLen, &bUseDefaultPPS);
	}
	//Restore WT and GT for PPS
	if (!bUseDefaultPPS) {
		_scInfo[u8SCID].u8Fi = u8FI;
		_scInfo[u8SCID].u8Di = u8DI;
	} else {
		_scInfo[u8SCID].u8Fi = 1;
		_scInfo[u8SCID].u8Di = 1;
	}
	_sc_T0State[u8SCID].u8WI = u8TempWI;
	_scInfo[u8SCID].u8Protocol = u8TempProtocol;
	if (_scInfo[u8SCID].u8Protocol == 1)
		_SC_Calculate_CGWT(u8SCID, _sc_T1State[u8SCID].u8CWI);
	else
		_SC_Calculate_GWT(u8SCID);


	//Enable interrupt
	if (_scInfo[u8SCID].u8Protocol == 1)
		_SC_EnableIntBGWT(u8SCID);


	if (_sc_T0State[u8SCID].stExtWtPatch.bIsExtWtPatch == FALSE)
		_SC_EnableIntCGWT(u8SCID);



//[DEBUG]

	MS_U32 u32Idx;

	SC_DBG(E_SC_DBGLV_INFO, "PPS::: ");
	for (u32Idx = 0; u32Idx < u16PPSLen; u32Idx++)
		SC_DBG(E_SC_DBGLV_INFO, "0x%x, ", au8PPS[u32Idx]);

	SC_DBG(E_SC_DBGLV_INFO, "\n");
//[DEBUG]

	return eResult;
}

static MS_BOOL _SC_CheckRC(const MS_U8 u8RCType, const MS_U8 *pu8Data, const MS_U16 u16Length,
			   const MS_U16 u16RCValue, MS_BOOL * const pbRCCheckPass)
{
	if (pu8Data == NULL)
		return FALSE;

	if (u16Length == 0)
		return FALSE;

	if (pbRCCheckPass == NULL)
		return FALSE;

	//RC-CRC
	if (u8RCType & SC_RC_TYPE_CRC) {
		MS_U16 u16i = 0;
		MS_U16 u16CRC = 0xFFFF;
		MS_U16 u16BlockLenLocal = u16Length;

		while (u16BlockLenLocal--) {
			u16CRC ^= *pu8Data++;
			for (u16i = 0; u16i < 8; u16i++) {
				if (u16CRC & 0x01) {
					// LSB = 1
					u16CRC = (u16CRC >> 1) ^ 0xA001;
				} else {
					//LSB = 2
					u16CRC = u16CRC >> 1;
				}
			}
		}
		if (u16RCValue == u16CRC)
			*pbRCCheckPass = TRUE;
		else
			*pbRCCheckPass = FALSE;

	} else {
		//RC-LRC
		MS_U16 u16i = 0;
		MS_U8 u8LRC = 0x00;

		for (u16i = 0; u16i < u16Length; u16i++)
			u8LRC ^= pu8Data[u16i];

		SC_DBG(E_SC_DBGLV_INFO, "%d, caLRC %02X, u16RCValue %02X u16Length=%d\n", __LINE__,
		       u8LRC, u16RCValue, u16Length);
		if (u16RCValue == u8LRC)
			*pbRCCheckPass = TRUE;
		else
			*pbRCCheckPass = FALSE;

	}
	return TRUE;
}

static void _SC_T1AppendRC(SC_T1_State *pT1, MS_U8 *pu8Block, MS_U16 *pu16BlockLen)
{
	//RC-CRC
	if (pT1->u8RCType & 0x1) {
		MS_U32 i;
		MS_U16 u16CRC = 0xFFFF;
		MS_U16 u16BlockLenLocal = *pu16BlockLen;

		while (u16BlockLenLocal--) {
			u16CRC ^= *pu8Block++;
			for (i = 0; i < 8; i++) {
				if (u16CRC & 0x01) {
					// LSB = 1
					u16CRC = (u16CRC >> 1) ^ 0xA001;
				} else {
					//LSB = 2
					u16CRC = u16CRC >> 1;
				}
			}
		}
		pu8Block[*pu16BlockLen] = (u16CRC >> 8) & 0xFF;
		pu8Block[*pu16BlockLen + 1] = u16CRC & 0xFF;
		*pu16BlockLen += 2;
	}
	//RC-LRC
	else {
		MS_U8 u8LRC = 0x00;
		MS_U8 i;

		for (i = 0; i < (*pu16BlockLen); i++)
			u8LRC ^= pu8Block[i];

		pu8Block[*pu16BlockLen] = u8LRC;
		*pu16BlockLen += 1;
	}

}


static SC_Result _SC_T1IBlock(SC_T1_State *pT1, MS_BOOL ubMore,
			      MS_U8 *pu8Data, MS_U16 u16DataLen,
			      MS_U8 *pu8Block, MS_U16 *pu16BlockLen)
{
	//[1] NAD
	pu8Block[0] = pT1->u8NAD;

	//[2] PCB - N(S),More bit
	pu8Block[1] = 0x00;
	if (pT1->u8NS)
		pu8Block[1] |= 0x40;

	if (ubMore)
		pu8Block[1] |= 0x20;

	if (u16DataLen > pT1->u8IFSC)
		return E_SC_FAIL;

	//[3] LEN
	pu8Block[2] = (MS_U8) u16DataLen;

	//[4] DATA
	memcpy(pu8Block + 3, pu8Data, u16DataLen);

	*pu16BlockLen = u16DataLen + 3;

	//[5] EDC
	_SC_T1AppendRC(pT1, pu8Block, pu16BlockLen);

	return E_SC_OK;
}


// build R block
static void _SC_T1RBlock(SC_T1_State *pT1, MS_U32 u32Type, MS_U8 *pu8Block, MS_U16 *pu16BlockLen)
{
	pu8Block[0] = pT1->u8NAD;
	pu8Block[2] = 0x00;

	if (u32Type == SC_T1_R_OK) {
		if (pT1->u8NR)
			pu8Block[1] = 0x90;
		else
			pu8Block[1] = 0x80;

	} else if (u32Type == SC_T1_R_EDC_ERROR) {
		if (pT1->u8NR)
			pu8Block[1] = 0x91;
		else
			pu8Block[1] = 0x81;

	} else if (u32Type == SC_T1_R_OTHER_ERROR) {
		if (pT1->u8NR)
			pu8Block[1] = 0x92;
		else
			pu8Block[1] = 0x82;

	}
	*pu16BlockLen = 3;

	_SC_T1AppendRC(pT1, pu8Block, pu16BlockLen);
}


static SC_Result _SC_T1SBlock(SC_T1_State *pT1, MS_U32 u32Type, MS_U32 u8Dir,
			      MS_U8 u8Param, MS_U8 *pu8Block, MS_U16 *pu16len)
{
	pu8Block[0] = pT1->u8NAD;

	switch (u32Type) {
	case SC_T1_S_RESYNCH:
		if (u8Dir == SC_T1_S_REQUEST)
			pu8Block[1] = 0xC0;
		else
			pu8Block[1] = 0xE0;

		pu8Block[2] = 0x00;
		*pu16len = 3;
		break;
	case SC_T1_S_IFS:
		if (u8Dir == SC_T1_S_REQUEST)
			pu8Block[1] = 0xC1;
		else
			pu8Block[1] = 0xE1;

		pu8Block[2] = 0x01;
		pu8Block[3] = u8Param;
		*pu16len = 4;
		break;
	case SC_T1_S_ABORT:
		if (u8Dir == SC_T1_S_REQUEST)
			pu8Block[1] = 0xC2;
		else
			pu8Block[1] = 0xE2;

		pu8Block[2] = 0x00;
		*pu16len = 3;
		break;
	case SC_T1_S_WTX:
		if (u8Dir == SC_T1_S_REQUEST)
			pu8Block[1] = 0xC3;
		else
			pu8Block[1] = 0xE3;

		pu8Block[2] = 0x01;
		pu8Block[3] = u8Param;
		*pu16len = 4;
		break;
	default:
		return E_SC_FAIL;
	}
	_SC_T1AppendRC(pT1, pu8Block, pu16len);

	return E_SC_OK;
}

//get block S(n),R(n)
static MS_U8 _SC_T1GetBlockN(MS_U8 *pu8Block)
{
	// I-Block
	if ((pu8Block[1] & 0x80) == 0x00)
		return ((pu8Block[1] >> 6) & 0x01);

	// R-Block
	if ((pu8Block[1] & 0xC0) == 0x80)
		return ((pu8Block[1] >> 4) & 0x01);

	return 0;
}

static SC_Result _SC_T1SendRecvBlock(MS_U8 u8SCID, MS_U8 *pu8Block,
				     MS_U16 *pu16BlockLen, MS_U8 *pu8RspBlock,
				     MS_U16 *pu16RspBlockLen)
{
	SC_Result ret_Result = E_SC_FAIL;

	SC_DBG(E_SC_DBGLV_INFO, "%s [%d]\n", __func__, __LINE__);

/*	//debug tx rx loop patten
 *	MS_U8 SetTestPatten1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 *	MS_U8 SetTestPatten2[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
 *	MS_U8 SetTestPatten3[] = {0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00};
 *	*pu16BlockLen=10;
 *	*pu16RspBlockLen=10;
 *		//send data
 *		ret_Result = _SC_Send(u8SCID, SetTestPatten1, *pu16BlockLen, SC_T1_SEND_TIMEOUT);
 *		if (ret_Result != E_SC_OK) {
 *			SC_DBG(E_SC_DBGLV_INFO, "%s [%d] failed\n", __func__, __LINE__);
 *			return ret_Result;
 *		}
 *		//block waiting time
 *		MsOS_DelayTask(300);
 *
 *		//receive data
 *		ret_Result = _SC_Recv(u8SCID, pu8RspBlock, pu16RspBlockLen, SC_T1_RECV_TIMEOUT);
 *
 *
 *		///////////////
 *		//send data
 *		ret_Result = _SC_Send(u8SCID, SetTestPatten2, *pu16BlockLen, SC_T1_SEND_TIMEOUT);
 *		if (ret_Result != E_SC_OK) {
 *			SC_DBG(E_SC_DBGLV_INFO, "%s [%d] failed\n", __func__, __LINE__);
 *			return ret_Result;
 *		}
 *		//block waiting time
 *		MsOS_DelayTask(300);
 *
 *		//receive data
 *		ret_Result = _SC_Recv(u8SCID, pu8RspBlock, pu16RspBlockLen, SC_T1_RECV_TIMEOUT);
 *
 *
 *		//////////////
 *		//send data
 *		ret_Result = _SC_Send(u8SCID, SetTestPatten3, *pu16BlockLen, SC_T1_SEND_TIMEOUT);
 *		if (ret_Result != E_SC_OK) {
 *			SC_DBG(E_SC_DBGLV_INFO, "%s [%d] failed\n", __func__, __LINE__);
 *			return ret_Result;
 *		}
 *		//block waiting time
 *		MsOS_DelayTask(300);
 *
 *		//receive data
 *		ret_Result = _SC_Recv(u8SCID, pu8RspBlock, pu16RspBlockLen, SC_T1_RECV_TIMEOUT);
 */

/*	debug test patten
 *	MS_U8 SetTestPatten1[128];
 *	int ii;
 *
 *	for (ii = 0; ii < 128; ii++)
 *		SetTestPatten1[ii]=ii;
 *
 *	*pu16BlockLen=128;
 *	*pu16RspBlockLen=128;
 *		//send data
 *		ret_Result = _SC_Send(u8SCID, SetTestPatten1, *pu16BlockLen, SC_T1_SEND_TIMEOUT);
 *		if (ret_Result != E_SC_OK) {
 *			SC_DBG(E_SC_DBGLV_INFO, "%s [%d] failed\n", __func__, __LINE__);
 *			return ret_Result;
 *		}
 *		//block waiting time
 *		MsOS_DelayTask(300);
 *
 *		//receive data
 *		ret_Result = _SC_Recv(u8SCID, pu8RspBlock, pu16RspBlockLen, SC_T1_RECV_TIMEOUT);
 *
 *		for (ii = 0; ii < 128; ii++)
 *		{
 *			if(SetTestPatten1[ii] != pu8RspBlock[ii])
 *				ret_Result = E_SC_FAIL;//test
 *		}
 */

	//MsOS_DelayTask(300);
	//send data
	ret_Result = _SC_Send(u8SCID, pu8Block, *pu16BlockLen, SC_T1_SEND_TIMEOUT);
	if (ret_Result != E_SC_OK) {
		SC_DBG(E_SC_DBGLV_INFO, "%s [%d] failed\n", __func__, __LINE__);
		return ret_Result;
	}
	//block waiting time
	MsOS_DelayTask(300);

	//receive data
	ret_Result = _SC_Recv(u8SCID, pu8RspBlock, pu16RspBlockLen, SC_T1_RECV_TIMEOUT);

	return ret_Result;
}

//-------------------------------------------------------------------------------------------------
//              Global funcs
//-------------------------------------------------------------------------------------------------

SC_Result MDrv_SC_Init(MS_U8 u8SCID)
{
	SC_Result eRteVal = E_SC_OK;

	eRteVal = _MDrv_SC_Init(u8SCID);

	return eRteVal;
}

SC_Result _MDrv_SC_Init(MS_U8 u8SCID)
{
	MS_VIRT u32BaseAddr;
	MS_PHY u32BaseSize;
	MS_BOOL bIsInitialized = FALSE;
#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
	MS_U32 u32ShmId, u32BufSize;
	MS_VIRT u32Addr;
	MS_U32 bInitialized;
	MS_U8 au8ShmSmc1[] = SC_SHMSMC1;
#if (SC_DEV_NUM > 1)
	MS_U8 au8ShmSmc2[] = SC_SHMSMC2;
#endif
	MS_U8 *pu8ShmSmc = NULL;
#endif

	SC_DBG(E_SC_DBGLV_INFO, "%s %d u8SCID=%d\n", __func__, __LINE__, u8SCID);

	//Set Pad Mux
	//hal_sc_set_pad_mux();

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;


	if (_scInfo[u8SCID].bInited) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "SMC%d has been initialized\n", u8SCID);
		return E_SC_OK;
	}

	bIsInitialized = _scInfo[0].bInited;
#if (SC_DEV_NUM > 1)
	bIsInitialized |= _scInfo[1].bInited;
#endif

	if (!bIsInitialized) {
		if (_SC_SetupHwFeature() != E_SC_OK) {
			SC_DBG(E_SC_DBGLV_ERR_ONLY, "Failed to setup HW feature!!\n");
			return E_SC_FAIL;
		}
	}

	_scInfo[u8SCID].bInited = FALSE;

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
	if (u8SCID == 0)
		pu8ShmSmc = au8ShmSmc1;
#if (SC_DEV_NUM > 1)		// no more than 2
	else if (u8SCID == 1)
		pu8ShmSmc = au8ShmSmc2;
#endif
#endif

	// Driver local variable initialization
	_scInfo[u8SCID].bOpened = FALSE;

	HAL_SC_Init(u8SCID);


	_scInfo[u8SCID].bInited = TRUE;
	SC_DBG(E_SC_DBGLV_INFO, "%s PASS\n\n", __func__);
	return E_SC_OK;
}

SC_Result MDrv_SC_Exit(MS_U8 u8SCID)
{
	if (_gScInitMutexID < 0) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY,
		       "[%s][%d] Please call MDrv_SC_Init first!\n", __func__, __LINE__);
		return E_SC_FAIL;
	}

	SC_Result eRteVal = E_SC_FAIL;

	eRteVal = _MDrv_SC_Exit(u8SCID);

	return eRteVal;
}

SC_Result _MDrv_SC_Exit(MS_U8 u8SCID)
{
	MS_U32 u32ShmId, u32BufSize, bInitialized;
	MS_VIRT u32Addr;
	MS_U8 au8ShmSmc1[] = SC_SHMSMC1;
#if (SC_DEV_NUM > 1)
	MS_U8 au8ShmSmc2[] = SC_SHMSMC2;
#endif
	MS_U8 *pu8ShmSmc = NULL;

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (_scInfo[u8SCID].bInited == FALSE) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "Not initial\n");
		return E_SC_FAIL;
	}

	if (u8SCID == 0)
		pu8ShmSmc = au8ShmSmc1;
#if (SC_DEV_NUM > 1)		// no more than 2
	else if (u8SCID == 1)
		pu8ShmSmc = au8ShmSmc2;
#endif

	HAL_SC_Exit(u8SCID);

	_scInfo[u8SCID].bInited = FALSE;

#if defined(MSOS_TYPE_LINUX) && defined(SC_KERNEL_ISR)
	close(_scInfo[u8SCID].s32DevFd);
	_scInfo[u8SCID].s32DevFd = -1;
#endif

#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(SC_KERNEL_ISR)
	//_SC_KDrvKernelModeExit(u8SCID); for STI
#endif				// defined(MSOS_TYPE_LINUX_KERNEL) && defined(SC_KERNEL_ISR)


	return E_SC_OK;
}

SC_Result MDrv_SC_Open(MS_U8 u8SCID, MS_U8 u8Protocol, SC_Param *pParam,
		       P_SC_Callback pfSmartNotify)
{
	return _MDrv_SC_Open(u8SCID, u8Protocol, pParam, pfSmartNotify);
}

SC_Result _MDrv_SC_Open(MS_U8 u8SCID, MS_U8 u8Protocol, SC_Param *pParam,
			P_SC_Callback pfSmartNotify)
{
	HAL_SC_OPEN_SETTING stOpenSetting;
	HAL_SC_FIFO_CTRL stCtrlFIFO;
	HAL_SC_MODE_CTRL stModeCtrl;

	//SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d, u8Protocol=%d\n", __func__, u8SCID, u8Protocol);
	//SC_PARAM_DBG(pParam);

	//OS_SC_ENTRY();

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (_scInfo[u8SCID].bOpened == TRUE)
		return E_SC_FAIL;

	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (pParam == NULL)
		return E_SC_FAIL;

	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	// Open SC device
	_scInfo[u8SCID].eCardClk = pParam->eCardClk;	//DEVSC_CLK_4P5M;//DEVSC_CLK_3M;
	_scInfo[u8SCID].u16ClkDiv = pParam->u16ClkDiv;	//372;
	_scInfo[u8SCID].u8UartMode = pParam->u8UartMode;
	_scInfo[u8SCID].eVccCtrl = pParam->eVccCtrl;
	_scInfo[u8SCID].u16Bonding = pParam->u16Bonding;
	_scInfo[u8SCID].pfEn5V = pParam->pfOCPControl;
	_scInfo[u8SCID].u8Convention = pParam->u8Convention;
	_scInfo[u8SCID].u8Protocol = u8Protocol;
	_scInfoExt[u8SCID].eVoltage = pParam->eVoltage;
	_scInfo[u8SCID].eTsioCtrl = pParam->eTsioCtrl;
	_scInfo[u8SCID].pfNotify = pfSmartNotify;
	_scInfo[u8SCID].u8Fi = 1;	// assigned by ATR
	_scInfo[u8SCID].u8Di = 1;
	_scInfo[u8SCID].bCardIn = FALSE;
	_scInfo[u8SCID].bLastCardIn = FALSE;
	_scInfoExt[u8SCID].u8N = 0;
	_scInfoExt[u8SCID].u8CardDetInvert = pParam->u8CardDetInvert;
	_SC_ClearProtocolState(u8SCID);
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (_scInfo[u8SCID].u8Convention == 0)
		stOpenSetting.eConvCtrl = E_HAL_SC_CONV_DIRECT;
	else
		stOpenSetting.eConvCtrl = E_HAL_SC_CONV_INVERSE;

	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	stOpenSetting.eVccCtrlType = _SC_ConvHalVccCtrl(_scInfo[u8SCID].eVccCtrl);
	stOpenSetting.eVoltageCtrl = _SC_ConvHalVoltageCtrl(_scInfoExt[u8SCID].eVoltage);
	stOpenSetting.eClkCtrl = _SC_ConvHalClkCtrl(_scInfo[u8SCID].eCardClk);
	stOpenSetting.u8UartMode = _scInfo[u8SCID].u8UartMode;
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	stOpenSetting.u16ClkDiv = _scInfo[u8SCID].u16ClkDiv;
	stOpenSetting.eCardDetType =
	    _SC_ConvHalCardDetType((SC_CardDetType) _scInfoExt[u8SCID].u8CardDetInvert);
	stOpenSetting.eTsioCtrl = (HAL_SC_TSIO_CTRL) _scInfo[u8SCID].eTsioCtrl;
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	if (HAL_SC_Open(u8SCID, &stOpenSetting) == FALSE) {
		pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] failed\n", __func__, __LINE__);
		return E_SC_FAIL;
	}
	pr_info("[%s][%d] u8SCID=%d\n", __func__, __LINE__, u8SCID);
	// Clear Interrupt
	HAL_SC_ClearCardDetectInt(u8SCID, E_HAL_SC_CD_ALL_CLEAR);

	// UART init

	// Disable all interrupts  // Interrupt Enable Register
	HAL_SC_SetUartInt(u8SCID, E_HAL_SC_UART_INT_DISABLE);

	// Reset receiver and transmiter  // FIFO Control Register
	memset(&stCtrlFIFO, 0x00, sizeof(stCtrlFIFO));
	stCtrlFIFO.bEnableFIFO = TRUE;
	stCtrlFIFO.eTriLevel = E_HAL_SC_RX_FIFO_INT_TRI_1;
	HAL_SC_SetUartFIFO(u8SCID, &stCtrlFIFO);

	stModeCtrl.bFlowCtrlEn = TRUE;
	stModeCtrl.bSmartCardMdoe = TRUE;
	stModeCtrl.u8ReTryTime = 0x4;
	stModeCtrl.bParityChk = TRUE;

	HAL_SC_SetSmcModeCtrl(u8SCID, &stModeCtrl);	// 0x64;

	// Check Rx FIFO empty
	while (HAL_SC_IsRxDataReady(u8SCID)) {
		HAL_SC_ReadRxData(u8SCID);
		OS_SC_DELAY(1);
	}

#if (defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR)
	if (_SC_T0ParityErrorKeepRcv(u8SCID, SC_T0_PARITY_ERROR_KEEP_RCV_ENABLE)
	    != E_SC_OK) {
		HAL_SC_Close(u8SCID, _SC_ConvHalVccCtrl(_scInfo[u8SCID].eVccCtrl));
		return E_SC_FAIL;
	}
#endif

	// enable interrupt
	if (_SC_SetupUartInt(u8SCID) != E_SC_OK) {
		HAL_SC_Close(u8SCID, _SC_ConvHalVccCtrl(_scInfo[u8SCID].eVccCtrl));
		return E_SC_FAIL;
	}

	_scInfo[u8SCID].bOpened = TRUE;
	// to do the first activation flow, without reset
#if (defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)) && defined(SC_KERNEL_ISR)
	if (HAL_SC_IsCardDetected(u8SCID)) {
		_scInfo[u8SCID].bLastCardIn = FALSE;
		_scInfo[u8SCID].bCardIn = TRUE;

		//Set event to SC Linux driver and make MDrv_SC_Task_Proc
		//can get card_in event from driver
		MS_U32 u32Evt = E_SC_INT_CARD_IN;


		KDrv_SC_SetEvent(u8SCID, &u32Evt);
	} else {
		_scInfo[u8SCID].bLastCardIn = FALSE;
	}
#else
	if (HAL_SC_IsCardDetected(u8SCID)) {
		_scInfo[u8SCID].bLastCardIn = FALSE;
		_scInfo[u8SCID].bCardIn = TRUE;

		if (u8SCID == 0) {
			OS_SC_SetEvent(OS_SC_EVENT_CARD);	//support sm1
			//return FALSE; // not handled
		} else if (u8SCID == 1) {
			OS_SC_SetEvent(OS_SC_EVENT_CARD2);	//support sm2
			//return FALSE; // not handled
		}
	} else {
		_scInfo[u8SCID].bLastCardIn = FALSE;
	}
#endif


	SC_DBG(E_SC_DBGLV_INFO, "%s PASS\n\n", __func__);
	return E_SC_OK;

}

SC_Result MDrv_SC_Activate(MS_U8 u8SCID)
{
	return _MDrv_SC_Activate(u8SCID);
}

SC_Result _MDrv_SC_Activate(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	//OS_SC_ENTRY();
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (_scInfo[u8SCID].bOpened == FALSE)
		return E_SC_FAIL;


	if (_SC_Activate(u8SCID) != E_SC_OK)
		return E_SC_FAIL;


	return E_SC_OK;
}

SC_Result MDrv_SC_Deactivate(MS_U8 u8SCID)
{
	return _MDrv_SC_Deactivate(u8SCID);
}

SC_Result _MDrv_SC_Deactivate(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	//OS_SC_ENTRY();
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (_scInfo[u8SCID].bOpened == FALSE)
		return E_SC_FAIL;


	if (_SC_Deactivate(u8SCID) != E_SC_OK)
		return E_SC_FAIL;


	return E_SC_OK;

}

SC_Result MDrv_SC_Close(MS_U8 u8SCID)
{
	return _MDrv_SC_Close(u8SCID);
}

SC_Result _MDrv_SC_Close(MS_U8 u8SCID)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	//OS_SC_ENTRY();
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (_scInfo[u8SCID].bOpened == FALSE)
		return E_SC_FAIL;


	if (_scInfo[u8SCID].bCardIn) {
		if (_SC_Deactivate(u8SCID) != E_SC_OK)
			return E_SC_FAIL;

	}
	// Close SC device and free system resource
	if (_SC_Close(u8SCID) != E_SC_OK)
		return E_SC_FAIL;


	_scInfo[u8SCID].bOpened = FALSE;
	_scInfo[u8SCID].pfNotify = NULL;

	return E_SC_OK;
}

SC_Result MDrv_SC_Reset(MS_U8 u8SCID, SC_Param *pParam)
{
	return _MDrv_SC_Reset(u8SCID, pParam);
}

SC_Result _MDrv_SC_Reset(MS_U8 u8SCID, SC_Param *pParam)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
	SC_PARAM_DBG(pParam);

	//OS_SC_ENTRY();
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (_scInfo[u8SCID].bOpened == FALSE)
		return E_SC_FAIL;


	if (pParam != NULL) {
		_scInfo[u8SCID].eCardClk = pParam->eCardClk;
		_scInfo[u8SCID].u16ClkDiv = pParam->u16ClkDiv;
		_SC_SetUartDiv(u8SCID);

		_scInfo[u8SCID].u8UartMode = pParam->u8UartMode;
		HAL_SC_SetUartMode(u8SCID, _scInfo[u8SCID].u8UartMode);
	}
	// Clear LSR register
	_SC_ClearState(u8SCID);
	_SC_ClearProtocolState(u8SCID);

	if (_SC_Reset(u8SCID) != E_SC_OK) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "	 Reset fail\n");
		return E_SC_FAIL;
	}
	SC_DBG(E_SC_DBGLV_INFO, "%s PASS\n\n", __func__);
	return E_SC_OK;
}

SC_Result MDrv_SC_ClearState(MS_U8 u8SCID)
{
	return _MDrv_SC_ClearState(u8SCID);
}

SC_Result _MDrv_SC_ClearState(MS_U8 u8SCID)
{
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
	return _SC_ClearState(u8SCID);
}

SC_Result MDrv_SC_GetATR(MS_U8 u8SCID, MS_U32 u32TimeOut, MS_U8 *pu8Atr,
			 MS_U16 *pu16AtrLen, MS_U8 *pu8His, MS_U16 *pu16HisLen)
{
	return _MDrv_SC_GetATR(u8SCID, u32TimeOut, pu8Atr, pu16AtrLen, pu8His, pu16HisLen);
}

SC_Result _MDrv_SC_GetATR(MS_U8 u8SCID, MS_U32 u32TimeOut, MS_U8 *pu8Atr,
			  MS_U16 *pu16AtrLen, MS_U8 *pu8His, MS_U16 *pu16HisLen)
{
	SC_Result eResult = E_SC_FAIL;
//[debug]
	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
	SC_DBG(E_SC_DBGLV_INFO, "%s u32TimeOut=%d\n", __func__, u32TimeOut);
	SC_DBG(E_SC_DBGLV_INFO, "%s pu8Atr=0x%08x\n", __func__, pu8Atr);
	SC_DBG(E_SC_DBGLV_INFO, "%s pu16AtrLen=0x%08x\n", __func__, pu16AtrLen);
	SC_DBG(E_SC_DBGLV_INFO, "%s pu8His=0x%08x\n", __func__, pu8His);
	SC_DBG(E_SC_DBGLV_INFO, "%s pu16HisLen=0x%08x\n", __func__, pu16HisLen);
//[debug]
	//OS_SC_ENTRY();

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;


	if (_scInfo[u8SCID].bOpened == FALSE)
		return E_SC_FAIL;


	eResult = _SC_GetATR(u8SCID, u32TimeOut);

	if (eResult == E_SC_CARDOUT) {
		SC_DBG(E_SC_DBGLV_INFO, "	 Cardout\n");
		return E_SC_CARDOUT;
	} else if (eResult == E_SC_FAIL) {
		//_SC_Callback_Enable5V(u8SCID, TRUE); for STI
		OS_SC_DELAY(SC_CARD_STABLE_TIME);
		pr_info("[%s][%d]retrun failed\n", __func__, __LINE__);
		return E_SC_FAIL;
	}

	if (eResult == E_SC_OK) {
		SC_DBG(E_SC_DBGLV_INFO, "	 ATR T=%d\n", _scInfo[u8SCID].u8Protocol);

		CPY_TO_USER(pu8Atr, _scInfo[u8SCID].pu8Atr, _scInfo[u8SCID].u16AtrLen);
		*pu16AtrLen = _scInfo[u8SCID].u16AtrLen;

		if (pu8His != NULL)
			CPY_TO_USER(pu8His, _scInfo[u8SCID].pu8Hist, _scInfo[u8SCID].u16HistLen);

		if (pu16HisLen != NULL)
			*pu16HisLen = _scInfo[u8SCID].u16HistLen;

#if PATCH_TO_DISABLE_CGT
		_gbPatchToDisableRxCGTFlag = TRUE;
#endif
	}
	SC_DBG(E_SC_DBGLV_INFO, "%s PASS\n\n", __func__);
	return eResult;
}

SC_Result MDrv_SC_Reset_ATR(MS_U8 u8SCID, SC_Param *pParam, MS_U8 *pu8Atr,
			    MS_U16 *pu16AtrLen, MS_U8 *pu8His, MS_U16 *pu16HisLen)
{
	return _MDrv_SC_Reset_ATR(u8SCID, pParam, pu8Atr, pu16AtrLen, pu8His, pu16HisLen);
}

SC_Result _MDrv_SC_Reset_ATR(MS_U8 u8SCID, SC_Param *pParam, MS_U8 *pu8Atr,
			     MS_U16 *pu16AtrLen, MS_U8 *pu8His, MS_U16 *pu16HisLen)
{
	SC_Result eResult = E_SC_FAIL;
	MS_U16 u16InputAtrLen = 0;

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	//OS_SC_ENTRY();
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (_scInfo[u8SCID].bOpened == FALSE)
		return E_SC_FAIL;

	if ((pu8Atr == NULL) || (pu16AtrLen == NULL))
		return E_SC_PARMERROR;


	u16InputAtrLen = *pu16AtrLen;
	*pu16AtrLen = 0;

	if (pParam != NULL) {
		_scInfo[u8SCID].eCardClk = pParam->eCardClk;
		_scInfo[u8SCID].u16ClkDiv = pParam->u16ClkDiv;
		_SC_SetUartDiv(u8SCID);

		_scInfo[u8SCID].u8UartMode = pParam->u8UartMode;
		HAL_SC_SetUartMode(u8SCID, _scInfo[u8SCID].u8UartMode);
	} else {
		_scInfo[u8SCID].eCardClk = E_SC_CLK_4P5M;
		_scInfo[u8SCID].u16ClkDiv = 372;
		_SC_SetUartDiv(u8SCID);

		_scInfo[u8SCID].u8UartMode = SC_UART_CHAR_8 | SC_UART_STOP_2 | SC_UART_PARITY_NO;
		HAL_SC_SetUartMode(u8SCID, _scInfo[u8SCID].u8UartMode);

	}

	_sc_T1State[u8SCID].u8NS = 0;
	_sc_T1State[u8SCID].u8NR = 0;
	_sc_T1State[u8SCID].ubMore = 0;

	_SC_ClearState(u8SCID);
	_SC_ClearProtocolState(u8SCID);

	if (_SC_Reset(u8SCID) != E_SC_OK) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "	 Reset fail\n");
		return E_SC_FAIL;
	}

	eResult = _SC_GetATR(u8SCID, SC_T0_ATR_TIMEOUT * 5);

	if (eResult == E_SC_CARDOUT) {
		SC_DBG(E_SC_DBGLV_INFO, "	 Cardout\n");
		return E_SC_CARDOUT;
	} else if (eResult == E_SC_FAIL) {
		//_SC_Callback_Enable5V(u8SCID, TRUE); for STI
		OS_SC_DELAY(SC_CARD_STABLE_TIME);
		return E_SC_FAIL;

	}

	if (eResult == E_SC_OK) {
		SC_DBG(E_SC_DBGLV_INFO, "	 ATR T=%d\n", _scInfo[u8SCID].u8Protocol);

#if PATCH_TO_DISABLE_CGT
		_gbPatchToDisableRxCGTFlag = TRUE;
#endif
		// suppose all mode need to go here, check later
		// if don't call it, healthy card abnormal when on chip 8024 mode
		{
			if (_scInfo[u8SCID].pu8Atr[0] == 0x3b) {
				_scInfo[u8SCID].u8UartMode =
				    SC_UART_CHAR_8 | SC_UART_STOP_2 | SC_UART_PARITY_EVEN;
				HAL_SC_SetUartMode(u8SCID, _scInfo[u8SCID].u8UartMode);
			} else if (_scInfo[u8SCID].pu8Atr[0] == 0x3f) {
				_scInfo[u8SCID].u8UartMode =
				    SC_UART_CHAR_8 | SC_UART_STOP_2 | SC_UART_PARITY_ODD;
				HAL_SC_SetUartMode(u8SCID, _scInfo[u8SCID].u8UartMode);
			}
		}

		if (_scInfo[u8SCID].bSpecMode) {
			_scInfo[u8SCID].u16ClkDiv =
			    (_atr_Fi[_scInfo[u8SCID].u8Fi] /
			     (MS_U16) _atr_Di[_scInfo[u8SCID].u8Di]);
			_SC_SetUartDiv(u8SCID);
		} else {
			eResult =
			    _SC_PPS(u8SCID, _scInfo[u8SCID].u8Protocol,
				    _scInfo[u8SCID].u8Di, _scInfo[u8SCID].u8Fi);
			if (eResult == E_SC_OK) {
				SC_DBG(E_SC_DBGLV_INFO, "	 PPS OK!!\n");
				_scInfo[u8SCID].u16ClkDiv =
				    (_atr_Fi[_scInfo[u8SCID].u8Fi] /
				     (MS_U16) _atr_Di[_scInfo[u8SCID].u8Di]);
				_SC_SetUartDiv(u8SCID);
			} else {
				SC_DBG(E_SC_DBGLV_INFO, "	 PPS FAIL!!\n");
				if (_SC_Reset(u8SCID) != E_SC_OK) {

					SC_DBG(E_SC_DBGLV_ERR_ONLY,
					       "[%s][%d] _SC_Reset error\n", __func__, __LINE__);
					return E_SC_FAIL;
				}
				if (_SC_GetATR(u8SCID, SC_T0_ATR_TIMEOUT * 5) != E_SC_OK) {
					SC_DBG(E_SC_DBGLV_ERR_ONLY,
					       "[%s][%d] _SC_GetATR error\n", __func__, __LINE__);
					return E_SC_FAIL;
				}
			}
		}

		// @TODO: check maximum output buffer size.

		*pu16AtrLen = MIN(u16InputAtrLen, _scInfo[u8SCID].u16AtrLen);
		CPY_TO_USER(pu8Atr, _scInfo[u8SCID].pu8Atr, *pu16AtrLen);

		if ((pu8His != NULL) && (pu16HisLen != NULL)) {
			*pu16HisLen = MIN(*pu16HisLen, _scInfo[u8SCID].u16HistLen);
			CPY_TO_USER(pu8His, _scInfo[u8SCID].pu8Hist, *pu16HisLen);
		}

		return E_SC_OK;
	}
	*pu16AtrLen = 0;

	return E_SC_FAIL;

}


SC_Result MDrv_SC_Config(MS_U8 u8SCID, SC_Param *pParam)
{
	return _MDrv_SC_Config(u8SCID, pParam);
}


SC_Result _MDrv_SC_Config(MS_U8 u8SCID, SC_Param *pParam)
{
	//OS_SC_ENTRY();

	if (pParam != NULL) {
		SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);
		SC_PARAM_DBG(pParam);

		_scInfo[u8SCID].eCardClk = pParam->eCardClk;
		_scInfo[u8SCID].u16ClkDiv = pParam->u16ClkDiv;
		if (_SC_SetUartDiv(u8SCID) != E_SC_OK)
			return E_SC_FAIL;

		_scInfo[u8SCID].u8UartMode = pParam->u8UartMode;
		if (!HAL_SC_SetUartMode(u8SCID, _scInfo[u8SCID].u8UartMode))
			return E_SC_FAIL;

	} else {
		return E_SC_FAIL;
	}
	SC_DBG(E_SC_DBGLV_INFO, "%s PASS\n\n", __func__);
	return E_SC_OK;
}

SC_Result MDrv_SC_Send(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 u16SendDataLen,
		       MS_U32 u32TimeoutMs)
{
	return _MDrv_SC_Send(u8SCID, pu8SendData, u16SendDataLen, u32TimeoutMs);
}

SC_Result _MDrv_SC_Send(MS_U8 u8SCID, MS_U8 *pu8SendData,
			MS_U16 u16SendDataLen, MS_U32 u32TimeoutMs)
{
	SC_Result eResult;

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	//OS_SC_ENTRY();
	if ((u8SCID >= SC_DEV_NUM) || (pu8SendData == NULL))
		return E_SC_PARMERROR;


	eResult = _SC_Send(u8SCID, pu8SendData, u16SendDataLen, u32TimeoutMs);
	if (eResult != E_SC_OK)
		return eResult;

	return E_SC_OK;
}

SC_Result MDrv_SC_Recv(MS_U8 u8SCID, MS_U8 *pu8ReadData,
		       MS_U16 *pu16ReadDataLen, MS_U32 u32TimeoutMs)
{
	return _MDrv_SC_Recv(u8SCID, pu8ReadData, pu16ReadDataLen, u32TimeoutMs);
}

SC_Result _MDrv_SC_Recv(MS_U8 u8SCID, MS_U8 *pu8ReadData,
			MS_U16 *pu16ReadDataLen, MS_U32 u32TimeoutMs)
{
	SC_Result eResult;

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	//OS_SC_ENTRY();
	if ((u8SCID >= SC_DEV_NUM) || (pu8ReadData == NULL)
	    || (pu8ReadData == NULL)) {
		return E_SC_PARMERROR;
	}

	eResult = _SC_Recv(u8SCID, pu8ReadData, pu16ReadDataLen, u32TimeoutMs);
	return eResult;
}

SC_Result MDrv_SC_T1_SendRecv(MS_U8 u8SCID, MS_U8 *pu8SendData,
			      MS_U16 *u16SendDataLen, MS_U8 *pu8ReadData, MS_U16 *u16ReadDataLen)
{
	return _MDrv_SC_T1_SendRecv(u8SCID, pu8SendData, u16SendDataLen,
				    pu8ReadData, u16ReadDataLen);
}

SC_Result _MDrv_SC_T1_SendRecv(MS_U8 u8SCID, MS_U8 *pu8SendData,
			       MS_U16 *u16SendDataLen, MS_U8 *pu8ReadData,
			       MS_U16 *u16ReadDataLen)
{

	SC_Result ret_Result = E_SC_FAIL;
	MS_U8 _u8HandleErrcount = 0;
	MS_U16 u16SentIdx = 3;
	MS_U16 u16BlockAPDULen = 0;
	MS_U8 u8Block[255];
	MS_U8 u8RspBlock[255];
	MS_U16 u8BlockLen = 0;
	MS_U16 u8RspRBlkLen = *u16ReadDataLen;
	MS_U8 u8EDCByteNum = 0;
	MS_U8 *pu8TempSendData;
	MS_U8 *pu8TempRecvData;
	MS_U16 u16RCValue = 0;
	MS_BOOL bRCCheckPass = FALSE;

	SC_DBG(E_SC_DBGLV_INFO, "%s u16SendDataLen=%d, u16ReadDataLen=%d\n",
	       __func__, *u16SendDataLen, *u16ReadDataLen);

	if (pu8SendData == NULL || pu8ReadData == NULL) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] Input data is NULL\n", __func__, __LINE__);
		return E_SC_FAIL;
	}
	//confirm with HWRD T1 doesn't check Perrity
	HAL_SC_Set_ctrl2_rec_parity_err_en(u8SCID);

	//HAL_SC_Set_mcr_loop_en(u8SCID);//test loopback

	memset(u8Block, 0, 255);
	memset(u8RspBlock, 0, 255);

	*u16ReadDataLen = 0;

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	//OS_SC_ENTRY();
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (!_scInfo[u8SCID].bCardIn)
		return E_SC_CARDOUT;

#if defined(MSOS_TYPE_LINUX_KERNEL)
	pu8TempSendData = (MS_U8 *) SC_MALLOC(*u16SendDataLen);
	if (pu8TempSendData == NULL) {
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] SC_MALLOC failed\n", __func__, __LINE__);
		return E_SC_FAIL;
	}
	pu8TempRecvData = (MS_U8 *) SC_MALLOC(512);
	if (pu8TempRecvData == NULL) {
		SC_FREE(pu8TempSendData);
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] SC_MALLOC failed\n", __func__, __LINE__);
		return E_SC_FAIL;
	}

	if (MsOS_KernelMemoryRegionCheck(pu8SendData)) {
		memcpy(pu8TempSendData, pu8SendData, *u16SendDataLen);
	} else {
		if (copy_from_user(pu8TempSendData, pu8SendData, *u16SendDataLen) != 0) {
			SC_FREE(pu8TempSendData);
			SC_FREE(pu8TempRecvData);
			SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] copy_from_user\n",
			       __func__, __LINE__);
			return E_SC_FAIL;
		}
	}

#else
	pu8TempSendData = pu8SendData;
	pu8TempRecvData = pu8ReadData;
#endif
	_sc_T1State[u8SCID].u8NAD = pu8TempSendData[0];

	//info to get the first block(I-Block,S-Block)
	if (*u16SendDataLen <= _sc_T1State[u8SCID].u8IFSC) {
		_sc_T1State[u8SCID].ubMore = FALSE;
		u16BlockAPDULen = *u16SendDataLen - 4;
	} else {
		_sc_T1State[u8SCID].ubMore = TRUE;
		u16BlockAPDULen = _sc_T1State[u8SCID].u8IFSC;
	}
	u16SentIdx = 3;

	//I-Block
	if ((pu8TempSendData[1] & 0x80) == 0x00) {
		_sc_T1State[u8SCID].u8NS = pu8TempSendData[1] & 0x40;
		_SC_T1IBlock(&_sc_T1State[u8SCID], _sc_T1State[u8SCID].ubMore,
			     pu8TempSendData + u16SentIdx, u16BlockAPDULen, u8Block, &u8BlockLen);
	}
	//S-Block
	else if (pu8TempSendData[1] >> 6 == 0x3) {
		u8BlockLen = *u16SendDataLen;
		memcpy(u8Block, pu8TempSendData, u8BlockLen);
	}
	//set temp EDC byte number
	if (_sc_T1State[u8SCID].u8RCType & 0x1)
		u8EDCByteNum = 2;	//CRC
	else
		u8EDCByteNum = 1;	//LRC

	while (u16SentIdx < *u16SendDataLen - u8EDCByteNum) {
		SC_DBG(E_SC_DBGLV_INFO,
		       "%s [%d] u16SentIdx=%d *u16SendDataLen=%d u8EDCByteNum=%d\n",
		       __func__, __LINE__, u16SentIdx, *u16SendDataLen, u8EDCByteNum);
		if (_scInfo[u8SCID].bCardIn == FALSE)
			break;

		//send Block
		ret_Result =
		    _SC_T1SendRecvBlock(u8SCID, u8Block, &u8BlockLen, u8RspBlock, &u8RspRBlkLen);
		if (_sc_T1State[u8SCID].u8RCType == SC_RC_TYPE_CRC) {
			//NAD , PCB , LEN , EDC(2 byte)
			if (u8RspRBlkLen < (SC_T1_PROLOGUE_SIZE + SC_RC_SIZE_CRC)) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY, "%s data size %d error\n", __func__,
				       u8RspRBlkLen);
				ret_Result = E_SC_DATAERROR;
			}
			u16RCValue =
			    (u8RspBlock[(u8RspRBlkLen - 2)] << 8) + u8RspBlock[(u8RspRBlkLen - 1)];
		} else {
			//NAD , PCB , LEN , EDC(1 byte)
			if (u8RspRBlkLen < (SC_T1_PROLOGUE_SIZE + SC_RC_SIZE_LRC)) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY, "%s data size %d error\n", __func__,
				       u8RspRBlkLen);
				ret_Result = E_SC_DATAERROR;
			}
			u16RCValue = u8RspBlock[(u8RspRBlkLen - 1)];
		}
		if (ret_Result == E_SC_OK) {
			if (_SC_CheckRC
			    (_sc_T1State[u8SCID].u8RCType, u8RspBlock,
			     (u8RspRBlkLen - u8EDCByteNum), u16RCValue, &bRCCheckPass) == TRUE) {
				if (bRCCheckPass == FALSE) {
					if (_sc_T1State[u8SCID].u8RCType & 0x1) {
						SC_DBG(E_SC_DBGLV_ERR_ONLY, "%s check CRC Fail\n",
						       __func__);
					} else {
						SC_DBG(E_SC_DBGLV_ERR_ONLY,
						       "%s check LRC Fail (%d,%d,%d)\n", __func__,
						       u8RspRBlkLen, u8EDCByteNum, u16RCValue);
						//if error retry
						continue;
					}
				}
			} else {
				ret_Result = E_SC_FAIL;
				SC_DBG(E_SC_DBGLV_ERR_ONLY, "%s _SC_CheckRC() error\n", __func__);
			}
		}

		if (ret_Result != E_SC_OK) {
			if (_u8HandleErrcount < 3) {
				SC_DBG(E_SC_DBGLV_INFO,
				       "%s [%d] _u8HandleErrcount < 3 ret_Result=%d\n",
				       __func__, __LINE__, ret_Result);
				_SC_T1RBlock(&_sc_T1State[u8SCID],
					     SC_T1_R_OTHER_ERROR, u8Block, &u8BlockLen);
				_u8HandleErrcount++;
				continue;
			} else {
				SC_DBG(E_SC_DBGLV_ERR_ONLY, "%s Send block fail", __func__);
#if defined(MSOS_TYPE_LINUX_KERNEL)
				SC_FREE(pu8TempSendData);
				SC_FREE(pu8TempRecvData);
#endif
				return E_SC_FAIL;
			}
		}
		//receive R-block
		if ((u8RspBlock[1] & 0xC0) == 0x80) {
			if (_u8HandleErrcount > 3) {
				SC_DBG(E_SC_DBGLV_ERR_ONLY, "%s Receive block fail", __func__);
#if defined(MSOS_TYPE_LINUX_KERNEL)
				SC_FREE(pu8TempSendData);
				SC_FREE(pu8TempRecvData);
#endif
				return E_SC_FAIL;
			}
			//resend I-block
			if (_SC_T1GetBlockN(u8RspBlock) == _sc_T1State[u8SCID].u8NS) {
				//Avoid overflow  after memcpy of the _SC_T1IBlock
				//refernce 7816-3  A.3 Error handling Scenario 10
				if (u16SentIdx >= u16BlockAPDULen) {
					u16SentIdx -= u16BlockAPDULen;
					SC_DBG(E_SC_DBGLV_INFO,
					       "%s u16SentIdx=%d [%d]\n",
					       __func__, u16SentIdx, __LINE__);
				} else {
					u16SentIdx += u16BlockAPDULen;
					SC_DBG(E_SC_DBGLV_INFO,
					       "%s u16SentIdx=%d [%d]\n",
					       __func__, u16SentIdx, __LINE__);
				}
			} else {
				_sc_T1State[u8SCID].u8NS ^= 1;
				u16SentIdx += u16BlockAPDULen;
				SC_DBG(E_SC_DBGLV_INFO,
				       "%s u16SentIdx=%d [%d]\n", __func__, u16SentIdx, __LINE__);
			}
			//build the next I-Block
			u16BlockAPDULen =
			    MIN(_sc_T1State[u8SCID].u8IFSC,
				*u16SendDataLen - u16SentIdx - u8EDCByteNum);

			// send the remain data
			if (u16BlockAPDULen != 0) {
				//last apdu data
				if (*u16SendDataLen - u16SentIdx - u8EDCByteNum == u16BlockAPDULen)
					_sc_T1State[u8SCID].ubMore = FALSE;

				_SC_T1IBlock(&_sc_T1State[u8SCID],
					     _sc_T1State[u8SCID].ubMore,
					     pu8TempSendData + u16SentIdx,
					     u16BlockAPDULen, u8Block, &u8BlockLen);
			}
		}
		// receive I-Block               todo : receive I-Block(complete?) when send I-Block
		else if ((u8RspBlock[1] & 0x80) == 0x00) {
			SC_DBG(E_SC_DBGLV_INFO, "%s [%d]receive I-Block\n", __func__, __LINE__);
			//receive invalid block
			if (_SC_T1GetBlockN(u8RspBlock) != _sc_T1State[u8SCID].u8NR) {
				_SC_T1RBlock(&_sc_T1State[u8SCID],
					     SC_T1_R_OTHER_ERROR, u8Block, &u8BlockLen);
				continue;
			}
			//7816-3 11.3.2.3: The values from '01' to 'FE'
			//encode the numbers 1 to 254: INF is present
			if ((u8RspBlock[2] >= 0x01) && (u8RspBlock[2] <= 0xFE)) {
				//copy receive data
				memcpy(pu8TempRecvData + *u16ReadDataLen + 3,
				       u8RspBlock + 3, u8RspBlock[2]);
				*u16ReadDataLen += (MS_U16) (u8RspBlock[2]);
				_sc_T1State[u8SCID].u8NR ^= 1;
			}
			//send R-Block
			if ((u8RspBlock[1] >> 5) & 1) {
				SC_DBG(E_SC_DBGLV_INFO,
				       "%s [%d]send R-Block 8RspBlock[1]=0x%02x\n",
				       __func__, __LINE__, u8RspBlock[1]);
				_SC_T1RBlock(&_sc_T1State[u8SCID], SC_T1_R_OK,
					     u8Block, &u8BlockLen);
				continue;
			}
			u16SentIdx += u16BlockAPDULen;	//to check
			SC_DBG(E_SC_DBGLV_INFO, "%s u16SentIdx=%d [%d]\n",
			       __func__, u16SentIdx, __LINE__);

		}
		// receive S-Block
		else if (u8RspBlock[1] >> 6 == 0x3) {
			//IFS request
			if (u8RspBlock[1] == 0xC1) {
				_sc_T1State[u8SCID].u8IFSC = u8RspBlock[3];
				if (_SC_T1SBlock
				    (&_sc_T1State[u8SCID], SC_T1_S_IFS,
				     SC_T1_S_RESPONSE, u8RspBlock[3], u8Block,
				     &u8BlockLen) != E_SC_OK) {
					SC_DBG(E_SC_DBGLV_ERR_ONLY,
					       "%s build IFS s-block fail", __func__);
#if defined(MSOS_TYPE_LINUX_KERNEL)
					SC_FREE(pu8TempSendData);
					SC_FREE(pu8TempRecvData);
#endif
					return E_SC_FAIL;
				}

				continue;

			}
			//ABORT request
			else if (u8RspBlock[1] == 0xC2) {
				if (_SC_T1SBlock
				    (&_sc_T1State[u8SCID], SC_T1_S_ABORT,
				     SC_T1_S_RESPONSE, 0x00, u8Block, &u8BlockLen) != E_SC_OK) {
					SC_DBG(E_SC_DBGLV_ERR_ONLY,
					       "%s build ABORT s-block fail", __func__);
#if defined(MSOS_TYPE_LINUX_KERNEL)
					SC_FREE(pu8TempSendData);
					SC_FREE(pu8TempRecvData);
#endif
					return E_SC_FAIL;
				}

				continue;

			}
			//S-Block WTX Reques
			else if (u8RspBlock[1] == 0xC3) {
				MsOS_DelayTask(u8RspBlock[3]);	// todo check
				if (_SC_T1SBlock
				    (&_sc_T1State[u8SCID], SC_T1_S_WTX,
				     SC_T1_S_RESPONSE, u8RspBlock[3], u8Block,
				     &u8BlockLen) != E_SC_OK) {
					SC_DBG(E_SC_DBGLV_ERR_ONLY,
					       "%s build WTX S-Block fail", __func__);
#if defined(MSOS_TYPE_LINUX_KERNEL)
					SC_FREE(pu8TempSendData);
					SC_FREE(pu8TempRecvData);
#endif
					return E_SC_FAIL;
				}

				continue;

			} else {
				//7816-3 11.3.2.3: The values from '01' to 'FE'
				//encode the numbers 1 to 254: INF is present
				if ((u8RspBlock[2] >= 0x01)
				    && (u8RspBlock[2] <= 0xFE)) {
					memcpy(pu8TempRecvData +
					       *u16ReadDataLen + 3, u8RspBlock + 3, u8RspBlock[2]);
					*u16ReadDataLen += (MS_U16) (u8RspBlock[2]);
					u16SentIdx += u16BlockAPDULen;
					SC_DBG(E_SC_DBGLV_INFO,
					       "%s u16SentIdx=%d [%d]\n",
					       __func__, u16SentIdx, __LINE__);
				}
			}

		} else {
			// error handling
			if (_u8HandleErrcount < 3) {
				_SC_T1RBlock(&_sc_T1State[u8SCID],
					     SC_T1_R_OTHER_ERROR, u8Block, &u8BlockLen);
				_u8HandleErrcount++;
				continue;
			} else {
#if defined(MSOS_TYPE_LINUX_KERNEL)
				SC_FREE(pu8TempSendData);
				SC_FREE(pu8TempRecvData);
#endif
				return E_SC_FAIL;
			}
		}
	}

	//NAD , PCB , LEN , EDC of TPDU
	pu8TempRecvData[0] = u8RspBlock[0];	//NAD information
	pu8TempRecvData[1] = u8RspBlock[1];	//PCB
	pu8TempRecvData[2] = (MS_U8) *u16ReadDataLen;
	*u16ReadDataLen += 3;
	_SC_T1AppendRC(&_sc_T1State[u8SCID], pu8TempRecvData, u16ReadDataLen);	//EDC

#if defined(MSOS_TYPE_LINUX_KERNEL)
	if (MsOS_KernelMemoryRegionCheck(pu8ReadData)) {
		memcpy(pu8ReadData, pu8TempRecvData, *u16ReadDataLen);
	} else {
		if (copy_to_user(pu8ReadData, pu8TempRecvData, *u16ReadDataLen)
		    != 0) {
			SC_FREE(pu8TempSendData);
			SC_FREE(pu8TempRecvData);
			SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] copy_to_user\n", __func__, __LINE__);
			return E_SC_FAIL;
		}
	}

	SC_FREE(pu8TempSendData);
	SC_FREE(pu8TempRecvData);
#endif

	SC_DBG(E_SC_DBGLV_INFO, "%s u16SendDataLen=%d, u16ReadDataLen=%d\n",
	       __func__, *u16SendDataLen, *u16ReadDataLen);
	return E_SC_OK;
}

const SC_Info *MDrv_SC_GetInfo(MS_U8 u8SCID)
{
	return (const SC_Info *)&_scInfo[u8SCID];
}

SC_Result _MDrv_SC_GetInfo(MS_U8 u8SCID, SC_Info *pstInfo)
{
	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	memcpy(pstInfo, &_scInfo[u8SCID], sizeof(SC_Info));

	return E_SC_OK;
}

SC_Result MDrv_SC_GetCaps(SC_Caps *pCaps)
{
	return _MDrv_SC_GetCaps(pCaps);
}


SC_Result _MDrv_SC_GetCaps(SC_Caps *pCaps)
{
	memcpy(pCaps, &_scCaps, sizeof(SC_Caps));

	return E_SC_OK;
}


SC_Result MDrv_SC_SetPPS(MS_U8 u8SCID, MS_U8 u8SCProtocol, MS_U8 u8Di, MS_U8 u8Fi)
{
	return _MDrv_SC_SetPPS(u8SCID, u8SCProtocol, u8Di, u8Fi);
}


SC_Result _MDrv_SC_SetPPS(MS_U8 u8SCID, MS_U8 u8SCProtocol, MS_U8 u8Di, MS_U8 u8Fi)
{
	//OS_SC_ENTRY();

	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if (_scInfo[u8SCID].bOpened == FALSE)
		return E_SC_FAIL;

	if ((_atr_Fi[u8Fi] == RFU) || (_atr_Di[u8Di] == RFU))
		return E_SC_FAIL;

	_scInfo[u8SCID].u8Protocol = u8SCProtocol;
	_scInfo[u8SCID].u8Fi = u8Fi;
	_scInfo[u8SCID].u8Di = u8Di;

	return E_SC_OK;
}

SC_Result MDrv_SC_PPS(MS_U8 u8SCID)
{
	return _MDrv_SC_PPS(u8SCID);
}

SC_Result _MDrv_SC_PPS(MS_U8 u8SCID)
{
	SC_Result eResult = E_SC_FAIL;

	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d\n", __func__, u8SCID);

	//OS_SC_ENTRY();
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	if ((_scInfo[u8SCID].u8Di == 1) &&
		(_scInfo[u8SCID].u8Fi == 0 || _scInfo[u8SCID].u8Fi == 1)) {
		//identical to default value
		return E_SC_OK;
	}

	if (_scInfo[u8SCID].bSpecMode) {
		_scInfo[u8SCID].u16ClkDiv =
		    (_atr_Fi[_scInfo[u8SCID].u8Fi] / (MS_U16) _atr_Di[_scInfo[u8SCID].u8Di]);
		_SC_SetUartDiv(u8SCID);
		return E_SC_OK;
	}

	eResult =
	    _SC_PPS(u8SCID, _scInfo[u8SCID].u8Protocol, _scInfo[u8SCID].u8Di, _scInfo[u8SCID].u8Fi);
	if (eResult == E_SC_OK) {
		SC_DBG(E_SC_DBGLV_INFO, "	 PPS ACK OK!!\n");
		_scInfo[u8SCID].u16ClkDiv =
		    (_atr_Fi[_scInfo[u8SCID].u8Fi] / (MS_U16) _atr_Di[_scInfo[u8SCID].u8Di]);
		_SC_SetUartDiv(u8SCID);
	}
	return eResult;
}

SC_Result MDrv_SC_GetStatus(MS_U8 u8SCID, SC_Status *pStatus)
{
	return _MDrv_SC_GetStatus(u8SCID, pStatus);
}

SC_Result _MDrv_SC_GetStatus(MS_U8 u8SCID, SC_Status *pStatus)
{
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_PARMERROR;

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__, u8SCID);
	_scStatus.bCardIn = _scInfo[u8SCID].bCardIn;
	memcpy(pStatus, &_scStatus, sizeof(SC_Status));
	return E_SC_OK;
}

void MDrv_SC_SetDbgLevel(SC_DbgLv eLevel)
{
	_MDrv_SC_SetDbgLevel(eLevel);
}

void _MDrv_SC_SetDbgLevel(SC_DbgLv eLevel)
{
	//_gu8DbgLevel = eLevel;
	_dbgmsg = eLevel;
}

SC_Result MDrv_SC_SetGuardTime(MS_U8 u8SCID, MS_U8 u8GuardTime)
{
	return _MDrv_SC_SetGuardTime(u8SCID, u8GuardTime);
}

SC_Result _MDrv_SC_SetGuardTime(MS_U8 u8SCID, MS_U8 u8GuardTime)
{
	if (u8SCID >= SC_DEV_NUM)
		return E_SC_FAIL;

	HAL_SC_SetCGT(u8SCID, u8GuardTime);

	return E_SC_OK;
}

SC_Result MDrv_SC_SetBuffAddr(SC_BuffAddr *pScBuffAddr)
{
	SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] not support\n", __func__, __LINE__);
	return E_SC_FAIL;
}

SC_Result MDrv_SC_RawExchange(MS_U8 u8SCID, MS_U8 *pu8SendData,
			      MS_U16 *u16SendDataLen, MS_U8 *pu8ReadData, MS_U16 *u16ReadDataLen)
{
	return _MDrv_SC_RawExchange(u8SCID, pu8SendData, u16SendDataLen,
				    pu8ReadData, u16ReadDataLen);
}

SC_Result _MDrv_SC_RawExchange(MS_U8 u8SCID, MS_U8 *pu8SendData,
			       MS_U16 *u16SendDataLen, MS_U8 *pu8ReadData,
			       MS_U16 *u16ReadDataLen)
{
	SC_Result ret_Result = E_SC_OK;
	MS_U32 u32Timeout = E_INVALID_TIMEOUT_VAL;

	SC_DBG(E_SC_DBGLV_INFO, "%s\n", __func__);

	ret_Result =
	    _SC_RawExchange(u8SCID, pu8SendData, u16SendDataLen, pu8ReadData,
			    u16ReadDataLen, u32Timeout);

	return ret_Result;
}

SC_Result MDrv_SC_RawExchangeTimeout(MS_U8 u8SCID, MS_U8 *pu8SendData,
				     MS_U16 *u16SendDataLen,
				     MS_U8 *pu8ReadData,
				     MS_U16 *u16ReadDataLen, MS_U32 u32TimeoutMs)
{
	SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] not support\n", __func__, __LINE__);
	return E_SC_FAIL;
}


MS_BOOL MDrv_SC_CardVoltage_Config(MS_U8 u8SCID, SC_VoltageCtrl eVoltage)
{
	return _MDrv_SC_CardVoltage_Config(u8SCID, eVoltage);
}

MS_BOOL _MDrv_SC_CardVoltage_Config(MS_U8 u8SCID, SC_VoltageCtrl eVoltage)
{
	HAL_SC_VCC_CTRL eHalVccCtrl;
	HAL_SC_VOLTAGE_CTRL eHalVoltageCtrl;
	MS_BOOL bRetVal;

	SC_DBG(E_SC_DBGLV_INFO, "%s u8SCID=%d eVoltage=%d\n", __func__, u8SCID, eVoltage);

	eHalVccCtrl = _SC_ConvHalVccCtrl(_scInfo[u8SCID].eVccCtrl);
	eHalVoltageCtrl = _SC_ConvHalVoltageCtrl(eVoltage);

	bRetVal = HAL_SC_CardVoltage_Config(u8SCID, eHalVoltageCtrl, eHalVccCtrl);
	if (bRetVal)
		_scInfoExt[u8SCID].eVoltage = eVoltage;
	else
		SC_DBG(E_SC_DBGLV_ERR_ONLY, "[%s][%d] not support\n", __func__, __LINE__);

	return bRetVal;
}

void MDrv_SC_EnableTimeout(MS_BOOL bTimeout)
{

}

MS_U32 MDrv_SC_SetPowerState(EN_POWER_MODE u16PowerState)
{
	return _MDrv_SC_SetPowerState(u16PowerState);
}


MS_U32 _MDrv_SC_SetPowerState(EN_POWER_MODE u16PowerState)
{
	MS_U32 u32Return = UTOPIA_STATUS_FAIL;
	MS_U8 u8SCID;

	OS_SC_STR_LOCK();

	if (u16PowerState == E_POWER_SUSPEND) {
		if (_gu16PreSCPowerState == E_POWER_MECHANICAL
		    || _gu16PreSCPowerState == E_POWER_RESUME) {
			SC_DBG(E_SC_DBGLV_INFO, "[%s][%d] EN_POWER_MODE:%d\n",
			       __func__, __LINE__, (int)u16PowerState);

			for (u8SCID = 0; u8SCID < SC_DEV_NUM; u8SCID++) {
				if (_scInfo[u8SCID].bInited == TRUE) {
					_gstStrCtrl[u8SCID].bIsResumeDrv = TRUE;
					_gstStrCtrl[u8SCID].pfNotify = _scInfo[u8SCID].pfNotify;
					_gstStrCtrl[u8SCID].stParam.u8Protocal =
					    _scInfo[u8SCID].u8Protocol;
					_gstStrCtrl[u8SCID].stParam.eCardClk =
					    _scInfo[u8SCID].eCardClk;
					_gstStrCtrl[u8SCID].stParam.u8UartMode =
					    _scInfo[u8SCID].u8UartMode;
					_gstStrCtrl[u8SCID].stParam.u16ClkDiv =
					    _scInfo[u8SCID].u16ClkDiv;
					_gstStrCtrl[u8SCID].stParam.eVccCtrl =
					    _scInfo[u8SCID].eVccCtrl;
					_gstStrCtrl[u8SCID].stParam.u16Bonding =
					    _scInfo[u8SCID].u16Bonding;
					_gstStrCtrl[u8SCID].stParam.pfOCPControl =
					    _scInfo[u8SCID].pfEn5V;
					_gstStrCtrl[u8SCID].stParam.u8Convention =
					    _scInfo[u8SCID].u8Convention;
					_gstStrCtrl[u8SCID].stParam.eVoltage =
					    _scInfoExt[u8SCID].eVoltage;
					_gstStrCtrl[u8SCID].stParam.u8CardDetInvert =
					    (SC_CardDetType) _scInfoExt[u8SCID].u8CardDetInvert;
					memcpy(&_gstStrCtrl[u8SCID].stParam.TimeoutCfg,
					       &_scInfo[u8SCID].TimeoutCfg,
					       sizeof(_scInfo[u8SCID].TimeoutCfg));

					_MDrv_SC_Close(u8SCID);
					_MDrv_SC_Exit(u8SCID);
				} else {
					_gstStrCtrl[u8SCID].bIsResumeDrv = FALSE;
				}
			}

			_gu16PreSCPowerState = u16PowerState;
		} else {
			SC_DBG(E_SC_DBGLV_INFO,
			       "[%s][%d] PreSCPowerState is not RESUME/MECHANICAL. We shouldn't suspend\n",
			       __func__, __LINE__);
			u32Return = UTOPIA_STATUS_SUCCESS;
		}

		u32Return = UTOPIA_STATUS_SUCCESS;	//SUSPEND_OK;
	} else if (u16PowerState == E_POWER_RESUME) {
		if (_gu16PreSCPowerState == E_POWER_SUSPEND) {
			SC_DBG(E_SC_DBGLV_INFO, "[%s][%d] EN_POWER_MODE:%d\n",
			       __func__, __LINE__, (int)u16PowerState);

			for (u8SCID = 0; u8SCID < SC_DEV_NUM; u8SCID++) {
				if (_gstStrCtrl[u8SCID].bIsResumeDrv == TRUE) {
					_MDrv_SC_Init(u8SCID);
					_MDrv_SC_Open(u8SCID,
						      _gstStrCtrl[u8SCID].stParam.u8Protocal,
						      &_gstStrCtrl[u8SCID].stParam,
						      _gstStrCtrl[u8SCID].pfNotify);
				}
				_gstStrCtrl[u8SCID].bIsResumeDrv = FALSE;
			}

			_gu16PreSCPowerState = u16PowerState;
			u32Return = UTOPIA_STATUS_SUCCESS;	//RESUME_OK;
		} else {
			SC_DBG(E_SC_DBGLV_INFO,
			       "[%s][%d] PreSCPowerState is not SUSPEND. We shouldn't resume\n",
			       __func__, __LINE__);
			u32Return = UTOPIA_STATUS_SUCCESS;	//SUSPEND_FAILED;
		}
	} else {
		SC_DBG(E_SC_DBGLV_INFO, "[%s][%d] EN_POWER_MODE:%d\n", __func__,
		       __LINE__, (int)u16PowerState);
		u32Return = UTOPIA_STATUS_FAIL;
	}

	OS_SC_STR_UNLOCK();

	return u32Return;	// for success
}

static HAL_SC_VOLTAGE_CTRL _SC_ConvHalVoltageCtrl(SC_VoltageCtrl eVoltage)
{
	HAL_SC_VOLTAGE_CTRL eHalVoltageCtrl;

	switch (eVoltage) {
	case E_SC_VOLTAGE_3V:
		eHalVoltageCtrl = E_HAL_SC_VOL_CTRL_3V;
		break;

	case E_SC_VOLTAGE_1P8V:
		eHalVoltageCtrl = E_HAL_SC_VOL_CTRL_1P8V;
		break;

	case E_SC_VOLTAGE_3_POINT_3V:
		eHalVoltageCtrl = E_HAL_SC_VOL_CTRL_3P3V;
		break;

	case E_SC_VOLTAGE_5V:
		eHalVoltageCtrl = E_HAL_SC_VOL_CTRL_5V;
		break;

	default:
		eHalVoltageCtrl = E_HAL_SC_VOL_CTRL_INVALID;
		break;
	}

	return eHalVoltageCtrl;
}

static HAL_SC_VCC_CTRL _SC_ConvHalVccCtrl(SC_VccCtrl eVccCtrl)
{
	HAL_SC_VCC_CTRL eHalVccCtrl;

	switch (eVccCtrl) {
	case E_SC_VCC_CTRL_8024_ON:
	default:
		eHalVccCtrl = E_HAL_SC_VCC_EXT_8024;
		break;

	case E_SC_VCC_CTRL_LOW:
		eHalVccCtrl = E_HAL_SC_VCC_LOW;
		break;

	case E_SC_VCC_CTRL_HIGH:
		eHalVccCtrl = E_HAL_SC_VCC_HIGH;
		break;

	case E_SC_OCP_VCC_HIGH:
		eHalVccCtrl = E_HAL_SC_VCC_OCP_HIGH;
		break;

	case E_SC_VCC_VCC_ONCHIP_8024:
		eHalVccCtrl = E_HAL_SC_VCC_INT_8024;
		break;
	}

	return eHalVccCtrl;
}

static HAL_SC_CLK_CTRL _SC_ConvHalClkCtrl(SC_ClkCtrl eClkCtrl)
{
	HAL_SC_CLK_CTRL eHalClkCtrl;

	switch (eClkCtrl) {
	case E_SC_CLK_3M:
		eHalClkCtrl = E_HAL_SC_CLK_3M;
		break;

	case E_SC_CLK_4P5M:
		eHalClkCtrl = E_HAL_SC_CLK_4P5M;
		break;

	case E_SC_CLK_4M:
		eHalClkCtrl = E_HAL_SC_CLK_4M;
		break;

	case E_SC_CLK_6M:
		eHalClkCtrl = E_HAL_SC_CLK_6M;
		break;

	case E_SC_CLK_13M:
		eHalClkCtrl = E_HAL_SC_CLK_13M;
		break;

	case E_SC_CLK_TSIO_MODE:
		eHalClkCtrl = E_HAL_SC_CLK_TSIO;
		break;
	default:
		eHalClkCtrl = E_HAL_SC_CLK_INVALID;
		break;
	}

	return eHalClkCtrl;
}

static HAL_SC_CARD_DET_TYPE _SC_ConvHalCardDetType(SC_CardDetType eCardDetType)
{
	HAL_SC_CARD_DET_TYPE eHalCardDetType;

	if (eCardDetType == E_SC_LOW_ACTIVE)
		eHalCardDetType = E_SC_CARD_DET_LOW_ACTIVE;
	else
		eHalCardDetType = E_SC_CARD_DET_HIGH_ACTIVE;

	return eHalCardDetType;
}

//void mdrv_sc_init_sw(void __iomem *riu_vaddr, void __iomem *ext_riu_vaddr)
void mdrv_sc_init_sw(void __iomem *riu_vaddr)
{

	pr_info("[%s][%d]\n", __func__, __LINE__);

	hal_sc_set_riu_base(riu_vaddr);
	//hal_sc_set_extriu_base(ext_riu_vaddr);

	pr_info("[%s][%d]pass\n", __func__, __LINE__);
}

void mdrv_sc_set_rf_pop_delay(int rf_pop_delay)
{

	pr_info("[%s][%d]\n", __func__, __LINE__);

	hal_sc_set_rf_pop_delay(0, rf_pop_delay);

	pr_info("[%s][%d]pass\n", __func__, __LINE__);
}
