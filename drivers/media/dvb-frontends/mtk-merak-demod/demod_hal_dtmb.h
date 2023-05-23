/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _HALDMD_INTERN_DTMB_H_
#define _HALDMD_INTERN_DTMB_H_

#include "demod_drv_dtmb.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern u8/*MS_BOOL*/ hal_intern_dtmb_ioctl_cmd(struct DTMB_IOCTL_DATA *pData,
enum DMD_DTMB_HAL_COMMAND eCmd, void *pArgs);

#ifdef __cplusplus
}
#endif


#endif//_HALDMD_INTERN_DTMB_H_
