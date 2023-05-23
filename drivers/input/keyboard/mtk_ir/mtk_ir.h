/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#ifndef _MTK_IR_H_
#define _MTK_IR_H_
#include "ir_common.h"

//IR MTK TV Headcode
#define IR_DEFAULT_CUSTOMER_CODE0		  0x7F80UL	  // Custom 0
#define IR_DEFAULT_CUSTOMER_CODE1		  0x0000UL	  // Custom 1

// IR HW NEC Timing define
#define IR_HEADER_CODE_TIME		9000	// us
#define IR_OFF_CODE_TIME		4500	// us
#define IR_OFF_CODE_RP_TIME		2500	// us
#define IR_LOGI_01H_TIME		560		// us
#define IR_LOGI_0_TIME			1120	// us
#define IR_LOGI_1_TIME			2240	// us
#define IR_TIMEOUT_CYC			140000	// us

#define IR_RP_TIMEOUT			irGetCnt(IR_TIMEOUT_CYC)
#define IR_HDC_UPB				irGetMaxCnt(IR_HEADER_CODE_TIME, 0.2)
#define IR_HDC_LOB				irGetMinCnt(IR_HEADER_CODE_TIME, 0.2)
#define IR_OFC_UPB				irGetMaxCnt(IR_OFF_CODE_TIME, 0.2)
#define IR_OFC_LOB				irGetMinCnt(IR_OFF_CODE_TIME, 0.2)
#define IR_OFC_RP_UPB			irGetMaxCnt(IR_OFF_CODE_RP_TIME, 0.2)
#define IR_OFC_RP_LOB			irGetMinCnt(IR_OFF_CODE_RP_TIME, 0.2)
#define IR_LG01H_UPB			irGetMaxCnt(IR_LOGI_01H_TIME, 0.35)
#define IR_LG01H_LOB			irGetMinCnt(IR_LOGI_01H_TIME, 0.3)
#define IR_LG0_UPB				irGetMaxCnt(IR_LOGI_0_TIME, 0.2)
#define IR_LG0_LOB				irGetMinCnt(IR_LOGI_0_TIME, 0.2)
#define IR_LG1_UPB				irGetMaxCnt(IR_LOGI_1_TIME, 0.2)
#define IR_LG1_LOB				irGetMinCnt(IR_LOGI_1_TIME, 0.2)

//#######################################
//#		(1) ==> Operation on 4/3 MHz
//#######################################
#define IR_RC_CLK_DIV(ClkMhz)		(ClkMhz * 3 / 4-1)
#define IR_RC_LGPS_THR(UnitTime)	((UnitTime))
/*
 * (UnitTime)*(4/3)*(3/4)
 */
#define IR_RC_INTG_THR(UnitTime)	(((UnitTime) * 2 / 3 - 7) / 8)
/*
 * ((UnitTime)*(4/3)*(1/2)-7)/8
 */
#define IR_RC_WDOG_CNT(UnitTime)	((UnitTime) / 768)
/*
 *(UnitTime)*(4/3)/1024
 */
#define IR_RC_TMOUT_CNT(UnitTime)	((UnitTime) / 1536)
/*
 *(UnitTime)*(4/3)/2048
 */
#define IR_RC6_LDPS_THR(UnitTime)	((UnitTime) * 8 / 9 - 31)
/*
 *(UnitTime)*(4/3)*(2/3)-31
 */
/*
 *RC6 leading pulse threshold * 32
 */
#define IR_RC6_LGPS_MAR(UnitTime)	((UnitTime) * 2 / 3)
/*
 *(UnitTime)*(4/3)*(1/2)
 */
//HW RC5 Timming Define
#define IR_RC5_LONGPULSE_THR	 1778	 //us
#define IR_RC5_LONGPULSE_MAR	 192	 //us only for RC6????
#define IR_RC5_INTG_THR_TIME	 887	 //us
#define IR_RC5_WDOG_TIME		 24576	 // us ???
#define IR_RC5_TIMEOUT_TIME		 24576	 // us ???
#define IR_RC5_POWER_WAKEUP_KEY	 0xffUL
#define IR_RC5_BITS				 14

//HW RC6 Timming Define
#define IR_RC6_LONGPULSE_THR	 889	//us 2t
#define IR_RC6_LONGPULSE_MAR	 444	//us only for RC6????
#define IR_RC6_INTG_THR_TIME	 444	//us t
#define IR_RC6_WDOG_TIME		 25752	// us ??? IR_RC6_LONGPULSE_THR*29bit=58t   25781
#define IR_RC6_TIMEOUT_TIME		 25752	// us ??? 58t
#define IR_RC6_POWER_WAKEUP_KEY	 0xffUL
#define IR_RC6_BITS				21
#define IR_RC6_LEADPULSE		2667  //6t	RC6 leading pulse threshold * 32


#define IRFLAG_IRENABLE			0x00000001UL
#define IRFLAG_HWINITED			0x00000002UL

#endif
