/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _APIGOP_PRIV_H_
#define _APIGOP_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sti_msos.h"

typedef struct {
	GFX_FireInfo *pFireInfo;
	GFX_OvalFillInfo *pDrawOvalInfo;
} GFX_Set_DrawOvalInfo;


//void            GFXRegisterToUtopia                 (FUtopiaOpen ModuleType);
MS_U32 GFXOpen(void **ppInstance, const void *const pAttribute);
MS_U32 GFXClose(void *pInstance);
MS_U32 GFXIoctl(void *pInstance, MS_U32 u32Cmd, void *pArgs);
#ifdef CONFIG_GFX_UTOPIA10
MS_U16 Ioctl_GFX_Init(void *pInstance, void *pArgs);
MS_U16 Ioctl_GFX_GetInfo(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_LineDraw(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_RectFill(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_TriFill(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_SpanFill(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_BitBlt(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_SetABL(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_SetConfig(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_MISC(void *pInstance, void *pArgs);
MS_S32 Ioctl_GFX_DrawOval(void *pInstance, void *pArgs);
#endif

#ifdef __cplusplus
}
#endif
#endif
