// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/string.h>
#include <linux/namei.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/ctype.h>
#else
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#endif
//#include "MsCommon.h"
//#include "MsOS.h"
#include "drvLDM_sti.h"
#include <linux/vmalloc.h>
#include "mdrv_ldm_stdlib.h"
//#include "ULog.h"
//#include "mdrv_ldm_algorithm.h"


//#include "regLDM.h"
//#include "halLDM.h"
//#include "drvMMIO.h"
//#include "utopia.h"
//#include "sti_msos.h"


//#if defined(MSOS_TYPE_LINUX)
//#include <sys/ioctl.h>
//#endif

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Structurs
//-------------------------------------------------------------------------------------------------
static int LDM_fd;

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
//ST_DRV_LD_CUS_PATH stCusPath;
struct LD_CUS_PATH stCusPath;

struct file *filp;

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
static enum EN_LDM_DEBUG_LEVEL gLDMDbgLevel = E_LDM_DEBUG_LEVEL_ERROR;
//static bool gbLDMInitialized = FALSE;
//static u8 gu8LDMStatus;
//static u8 gLDMTemp;

//static bool bLdm_Inited = FALSE;
//static MS_S32 _gs32LDM_Mutex;
//LDM mutex wait time
//#define LDM_MUTEX_WAIT_TIME    3000


//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------
#define LDM_DBG_FUNC()	\
do {				\
	if (gLDMDbgLevel >= E_LDM_DEBUG_LEVEL_ALL)\
		printf("[LDM]: %s: %d:\n", __func__, __LINE__);\
} while (0)

#define LDM_DBG_INFO(msg...)	\
do {\
	if (gLDMDbgLevel >= E_LDM_DEBUG_LEVEL_ERROR/*E_LDM_DEBUG_LEVEL_INFO */)\
		printf("[LDM INFO]: %s: %d: ", __func__, __LINE__);\
		printf(msg);\
} while (0)

#define LDM_DBG_ERR(msg...)	\
do {\
	if (gLDMDbgLevel >= E_LDM_DEBUG_LEVEL_ERROR)\
		printf("[LDM ERROR]: %s: %d: ", __func__, __LINE__);\
		printf(msg);\
} while (0)

#define LDM_DBG_WARN(msg...)	\
do {\
	if (gLDMDbgLevel >= E_LDM_DEBUG_LEVEL_WARNING) \
		printf("[LDM WARNING]: %s: %d: ", __func__, __LINE__);\
		printf(msg); \
} while (0)



#define COMPENSATION_BIN    "Compensation.bin"
#define EDGE2D_BIN          "Edge2D.bin"
#define BACKLIGHTGAMMA_BIN  "BacklightGamma.bin"
#define LDM_INI             "ldm.ini"
#define MAX_PATH_LEN        256

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))
#endif

static char *ld_config_paths[] = {
	"/vendor/tvconfig/config/ldm/",
	"/tvconfig/config/ldm/",
	"/config/ldm/",
};


#define MAX_PATH_LEN	256
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))
#endif





////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_GetData - get local dimming status
/// @ingroup G_LDM_CONTROL
// @param: void
// @return: EN_LDM_STATUS
//----------------------------------------------------------------
//u8 MDrv_LDM_GetStatus(void)
//{
//	LDM_DBG_FUNC();
//	LDM_DBG_INFO("status: %d\n", gu8LDMStatus);
//
//	return gu8LDMStatus;
//}
////remove-end trunk ld need refactor


////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_Enable - start local dimming algorithm
/// @ingroup G_LDM_CONTROL
// @param: na
// @return: E_LDM_RET_SUCCESS is enable
//----------------------------------------------------------------
//u8 MDrv_LDM_Enable(void)
//{
//	u8 iResult = 0;
//
//	iResult = MDrv_LD_Enable(TRUE, 0);
//
//	if (iResult == E_LDM_RET_FAIL) {
//		LDM_DBG_INFO("LDM Enable Fail\n");
//
//		return E_LDM_RET_FAIL;
//	}
//	gu8LDMStatus = E_LDM_STATUS_ENABLE;
//
//	LDM_DBG_INFO("OK\n");
//
//	return E_LDM_RET_SUCCESS;
//}
////remove-end trunk ld need refactor


////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_Disable - stop local dimming algorithm, send constant luminance  to led
/// @ingroup G_LDM_CONTROL
// @param: u8Lumma: constant luminance range from 00 to 255
// @return: E_LDM_RET_SUCCESS is disable
//----------------------------------------------------------------
//u8 MDrv_LDM_Disable(u8 u8Lumma)
//{
//	u8 iResult = 0;
//
//	iResult = MDrv_LD_Enable(FALSE, u8Lumma);
//
//	if (iResult == E_LDM_RET_FAIL) {
//		LDM_DBG_INFO("LDM Disable Fail\n");
//		return E_LDM_RET_FAIL;
//	}
//	gu8LDMStatus = E_LDM_STATUS_DISNABLE;
//
//	LDM_DBG_INFO("OK\n");
//
//	return E_LDM_RET_SUCCESS;
//
//}
////remove-end trunk ld need refactor


////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_Init - Set  mmap address to register base
/// @ingroup G_LDM_CONTROL
// @param: phyAddr: local dimming mmap address in mmap.ini
// @return: E_LDM_RET_SUCCESS is initial
//----------------------------------------------------------------
//u8 MDrv_LDM_Init(u64 phyAddr)
//{
//	//int iResult = 0;
//	int ret = 0;
//	u64 phyMMAPAddr = 0;
//
//	phyMMAPAddr = phyAddr;
//
//
//	LDM_DBG_INFO("init mmap address: 0x%llx\n", phyAddr);
//
//	if (gbLDMInitialized == TRUE) {
//		LDM_DBG_WARN(" reenter\n");
//		return E_LDM_RET_SUCCESS;
//	}
//
//	if (MDrv_LD_PrepareConfigFile(&stCusPath) == false) {
//		LDM_DBG_ERR("MDrv_LD_PrepareConfigFile fail\n");
//		return -1;
//	}
//
//	ret = MDrv_LD_Init(phyMMAPAddr, &stLdConfig[0]);
//	if (ret != E_LDM_RET_SUCCESS) {
//		LDM_DBG_ERR(" MDrv_LD_Init FAIL err:%d\n ", ret);
//		return -1;
//	}
//
//	ret = MDrv_LD_Setup();
//
//	if (ret != E_LDM_RET_SUCCESS) {
//		LDM_DBG_ERR(" MDrv_LD_Setup FAIL err:%d\n ", ret);
//		return -1;
//	}
//
//	gbLDMInitialized = TRUE;
//	gu8LDMStatus = E_LDM_STATUS_INIT;
//	LDM_DBG_INFO("OK\n");
//	return E_LDM_RET_SUCCESS;
//}
////remove-end trunk ld need refactor



////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_SetStrength - Set back light percent
/// @ingroup G_LDM_CONTROL
// @param: u8Percent: the percent ranged from 0 to 100
// @return: E_LDM_RET_SUCCESS is setted
//----------------------------------------------------------------
//u8 MDrv_LDM_SetStrength(u8 u8Percent)
//{
//	u8 iResult = 0;
//
//	iResult = MDrv_LD_SetGlobalStrength(u8Percent);
//
//	if (iResult == E_LDM_RET_FAIL) {
//		LDM_DBG_INFO("LDM setStrength Fail\n");
//		return E_LDM_RET_FAIL;
//	}
//
//	LDM_DBG_INFO("OK\n");
//	return E_LDM_RET_SUCCESS;
//
//}
////remove-end trunk ld need refactor



////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_DemoPattern - demo pattern from customer
/// @ingroup G_LDM_CONTROL
// @param: stPattern: demo type: turn on led, left-right half show
// @return: E_LDM_RET_SUCCESS is demonstrative
//----------------------------------------------------------------
//u8 MDrv_LDM_DemoPattern(ST_LDM_DEMO_PATTERN stPattern)
//{
//
//	MDrv_LD_SetDemoPattern(
//		stPattern.enDemoPattern,
//		stPattern.bOn,
//		stPattern.u16LEDNum
//		);
//
//	LDM_DBG_INFO("Demo OK\n");
//
//	return E_LDM_RET_SUCCESS;
//
//}
////remove-end trunk ld need refactor



////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_GetData - get LDF/LDB/SPI data pre frame in buffer
/// @ingroup G_LDM_CONTROL
// @param: stData:  data type and mmap address filled with the requied type
// @return: E_LDM_RET_SUCCESS is getted
//----------------------------------------------------------------
//u8 MDrv_LDM_GetData(ST_LDM_GET_DATA *stData)
//{
//	stData->phyAddr = MDrv_LD_GetDataAddr(stData->enDataType);
//	LDM_DBG_INFO("OK\n");
//	return E_LDM_RET_SUCCESS;
//
//}
////remove-end trunk ld need refactor



