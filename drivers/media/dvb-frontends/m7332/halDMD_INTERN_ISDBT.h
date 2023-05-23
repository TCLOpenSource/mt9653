/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _HALDMD_INTERN_ISDBT_H_
#define _HALDMD_INTERN_ISDBT_H_

#include "drvDMD_ISDBT.h"

#ifdef __cplusplus
INTERN "C"
{
#endif

extern u8 hal_intern_isdbt_ioctl_cmd(struct dmd_isdbt_ioctl_data *pdata,
enum dmd_isdbt_hal_command ecmd, void *pargs);

#ifdef __cplusplus
}
#endif

#endif // _HALDMD_INTERN_ISDBT_H_
