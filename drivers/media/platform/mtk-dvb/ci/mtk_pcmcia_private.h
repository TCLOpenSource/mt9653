/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __PCMCIA_PRIVATE__
#define __PCMCIA_PRIVATE__
#include "mtk_pcmcia_drv.h"


typedef enum {
	E_PCMCIA_RESOURCE,
} ePcmResourceId;

//typedef struct DLL_PACKED _PCMCIA_RESOURCE_PRIVATE
typedef struct {
//    signed char         bPCMCIA_Irq[E_PCMCIA_MODULE_MAX];
//    signed char         bPCMCIA_IrqStatus[E_PCMCIA_MODULE_MAX];
//    unsigned char           u8PCMCIA_Command[E_PCMCIA_MODULE_MAX];
//    unsigned char           u8HW_ResetDuration;

//    signed char         bHighActive;
//    signed char         bCardInside[E_PCMCIA_MODULE_MAX];
//    signed char         bPCMCIA_Detect_Enable;
//    unsigned int          u32PCMCIA_CD_To_HWRST_Timer[E_PCMCIA_MODULE_MAX];
	unsigned char u8PCMCIACurModule;

	// APP callback function
//    IsrCallback     fnIsrCallback[E_PCMCIA_MODULE_MAX];

	signed char bInited_Drv;
	unsigned int u32Magic;
	signed char bCD_Reverse;
	signed int _s32EventId;
} PCMCIA_RESOURCE_PRIVATE;

typedef struct _PCMCIA_INSTANT_PRIVATE {
} PCMCIA_INSTANT_PRIVATE;

void _mdrv_pcmcia_InitHW(unsigned char bResume);
unsigned char _mdrv_pcmcia_DetectV2(PCMCIA_MODULE eModule);

void _mdrv_pcmcia_Enable_InterruptV2(PCMCIA_MODULE eModule, unsigned char bEnable);
void _mdrv_pcmcia_Set_InterruptStatusV2(PCMCIA_MODULE eModule, unsigned char Status);
unsigned char _mdrv_pcmcia_Get_InterruptStatusV2(PCMCIA_MODULE eModule);
void _mdrv_pcmcia_InstarllIsrCallbackV2(PCMCIA_MODULE eModule,
					IsrCallback fnIsrCallback);
void _mdrv_pcmcia_Exit(unsigned char bSuspend);
#if defined(MSOS_TYPE_LINUX_KERNEL)
typedef enum {
	E_PCMCIA_Module_A_CardInsert = 0,
	E_PCMCIA_Module_A_CardRemove = 1,
	E_PCMCIA_Module_A_CardData = 2,
	E_PCMCIA_Module_B_CardInsert = 3,
	E_PCMCIA_Module_B_CardRemove = 4,
	E_PCMCIA_Module_B_CardData = 5
} E_PCMCIA_EVENT_ISR;

signed char _mdrv_pcmcia_IsrStatus(PCMCIA_MODULE *eModule, PCMCIA_ISR *pStIsr);
unsigned int PCMCIAStr(unsigned int u32PowerState, void *pModule);
#endif				//MSOS_TYPE_LINUX_KERNEL
#endif				//__PCMCIA_PRIVATE__
