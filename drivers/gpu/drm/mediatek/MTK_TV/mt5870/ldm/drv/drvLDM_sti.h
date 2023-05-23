/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRV_LDM_STI_H_
#define _DRV_LDM_STI_H_






//#include "sti_msos.h"
//#include "sti_msos_ldm.h"

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/slab.h>





#ifdef __cplusplus
extern "C"
{
#endif



#ifdef MDRV_LDM_C
#define MDRV_INTERFACE
#else
#define MDRV_INTERFACE extern
#endif

#undef printf
#define printf printk


enum EN_LDM_SUPPORT {
	E_LDM_UNSUPPORT = 0,
	E_LDM_SUPPORT = 1,
};

enum EN_LDM_STATUS {
	E_LDM_STATUS_INIT = 1,
	E_LDM_STATUS_ENABLE = 2,
	E_LDM_STATUS_DISNABLE,
	E_LDM_STATUS_SUSPEND,
	E_LDM_STATUS_RESUME,
	E_LDM_STATUS_DEINIT,
};

enum EN_LDM_RETURN {
	E_LDM_RET_SUCCESS = 0,
	E_LDM_RET_FAIL = 1,
	E_LDM_RET_NOT_SUPPORTED,
	E_LDM_RET_PARAMETER_ERROR,
	E_LDM_RET_OBTAIN_MUTEX_FAIL,
};


enum EN_LDM_DEBUG_LEVEL {
	E_LDM_DEBUG_LEVEL_ERROR = 0x01,
	E_LDM_DEBUG_LEVEL_WARNING = 0x02,
	E_LDM_DEBUG_LEVEL_INFO = 0x04,
	E_LDM_DEBUG_LEVEL_ALL = 0x07,
	E_LDM_DEBUG_LEVEL_MAX,
};

struct LD_CUS_PATH {
	char aCusPath[64];
	char aCusPathU[64];
};

////remove-start trunk ld need refactor
//typedef enum {
//	E_LDM_DATA_TYPE_LDF = 0x01,
//	E_LDM_DATA_TYPE_LDB = 0x02,
//	E_LDM_DATA_TYPE_SPI = 0x03,
//	E_LDM_DATA_TYPE_MAX
//} EN_LDM_GET_DATA_TYPE;
//
//typedef enum {
//	E_LDM_DEMO_PATTERN_SWITCH_SINGLE_LED = 0x00,
//	E_LDM_DEMO_PATTERN_LEFT_RIGHT_HALF = 0x01,
//	E_LDM_DEMO_PATTERN_MARQUEE = 0x02,
//	E_LDM_DEMO_PATTERN_LEFT_RIGHT_COLOR_SHELTER = 0x03,
//	E_LDM_DEMO_PATTERN_MAX
//} EN_LDM_DEMO_PATTERN;
//
//
//typedef struct {
//	EN_LDM_GET_DATA_TYPE enDataType;
//	u64 phyAddr;
//} ST_LDM_GET_DATA;
//
//typedef struct {
//	bool bOn;
//	EN_LDM_DEMO_PATTERN enDemoPattern;
//	u16 u16LEDNum;
//} ST_LDM_DEMO_PATTERN;
////remove-end trunk ld need refactor


//----------------------------------------------------------------
// MDrv_LDM_Enable - start local dimming algorithm
/// @ingroup G_LDM_CONTROL
// @param: na
// @return: E_LDM_RET_SUCCESS is enable
//----------------------------------------------------------------
//u8 MDrv_LDM_GetStatus(void);
//u8 MDrv_LDM_Enable(void);
//u8 MDrv_LDM_Disable(u8 u8Lumma);
//u8 MDrv_LDM_Init(u64 phyAddr);



//u8 MDrv_LDM_SetStrength(u8 u8Percent);
//u8 MDrv_LDM_DemoPattern(ST_LDM_DEMO_PATTERN stPattern);
//u8 MDrv_LDM_GetData(ST_LDM_GET_DATA *stData);


#ifdef __cplusplus
}
#endif


//void MDrv_LDM_Init(void);

//__u8 MDrv_LDM_Init_test(__u64 phyAddr);

#endif // _DRV_LDM_H_
