/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef	_HALDMD_INTERN_ATSC_H_
#define	_HALDMD_INTERN_ATSC_H_

#include "drvDMD_ATSC.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern u8/*MS_BOOL*/ hal_intern_atsc_iotcl_cmd(
DMD_ATSC_IOCTL_DATA * pdata, DMD_ATSC_HAL_COMMAND eCmd, void *pargs);

#ifdef __cplusplus
}
#endif

#endif //	_HALDMD_INTERN_ATSC_H_

