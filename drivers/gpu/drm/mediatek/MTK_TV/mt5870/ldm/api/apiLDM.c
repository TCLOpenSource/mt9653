// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#if !defined(MSOS_TYPE_LINUX_KERNEL)
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#else
#include <linux/string.h>
#include <linux/slab.h>
#endif


#include "apiLDM.h"



//-------------------------------------------------------------------------------------------------
//  Local Compiler Options
//-------------------------------------------------------------------------------------------------
////remove-start trunk ld need refactor
//int MApi_LDM_Enable(void)
//{
//	int ret = MDrv_LDM_Enable();
//
//	return ret;
//}
//int MApi_LDM_Disable(__u8 u8luma)
//{
//	int ret = MDrv_LDM_Disable(u8luma);
//
//	return ret;
//}
//
//int MApi_ld_Init(__u64 phyAddr)
//{
//	int ret = MDrv_LDM_Init(phyAddr);
//
//	return ret;
//}
////remove-end trunk ld need refactor


////remove-start trunk ld need refactor
//int MApi_LDM_SetStrength(__u8 u8Percent)
//{
//	int ret = MDrv_LDM_SetStrength(u8Percent);
//
//	return ret;
//}
//uint8_t MApi_LDM_GetStatus(void)
//{
//	uint8_t ret = MDrv_LDM_GetStatus();
//
//	return ret;
//}
//
//int MApi_LDM_SetDemoPattern(__u32 u32demopattern)
//{
//
//	ST_LDM_DEMO_PATTERN stDrvPattern = {0};
//	__u16 u16DemoPatternCtrl = (u32demopattern & 0xFFFF0000)>>16;
//	__u16 u16DemoLEDNum = (u32demopattern & 0x0000FFFF);
//	__u8 ret = 0;
//
//	switch (u16DemoPatternCtrl) {
//	default:
//	case E_LDM_DEMO_PATTERN_CTRL_OFF:
//	{
//		stDrvPattern.bOn = false;
//		stDrvPattern.enDemoPattern = E_LDM_DEMO_PATTERN_SWITCH_SINGLE_LED;
//		stDrvPattern.u16LEDNum = u16DemoLEDNum;
//
//		break;
//	}
//	case E_LDM_DEMO_PATTERN_CTRL_SINGLE:
//	{
//		stDrvPattern.bOn = true;
//		stDrvPattern.enDemoPattern = E_LDM_DEMO_PATTERN_SWITCH_SINGLE_LED;
//		stDrvPattern.u16LEDNum = u16DemoLEDNum;
//		break;
//	}
//	case E_LDM_DEMO_PATTERN_CTRL_MARQUEE:
//	{
//		stDrvPattern.bOn = true;
//		stDrvPattern.enDemoPattern = E_LDM_DEMO_PATTERN_MARQUEE;
//		stDrvPattern.u16LEDNum = u16DemoLEDNum;
//		break;
//	}
//	}
//
//	ret = MDrv_LDM_DemoPattern(stDrvPattern);
//
//	return ret;
//}
//
//int MApi_LDM_GetData(ST_LDM_GET_DATA *pstDrvData)
//{
//	int ret = MDrv_LDM_GetData(pstDrvData);
//
//	return ret;
//}
////remove-end trunk ld need refactor


