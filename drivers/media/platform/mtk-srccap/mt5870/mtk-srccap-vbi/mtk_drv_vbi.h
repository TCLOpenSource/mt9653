/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_DRV_VBI_H__
#define __MTK_DRV_VBI_H__

#include <linux/videodev2.h>
#include <linux/types.h>
#include "mtk_srccap.h"
#include "drvVBI.h"

typedef enum {
	V4L2_EXT_VBI_FAIL = -1,
	V4L2_EXT_VBI_OK = 0,
	V4L2_EXT_VBI_NOT_SUPPORT,
	V4L2_EXT_VBI_NOT_IMPLEMENT,
} V4L2_VBI_RESULT;
void mdrv_vbi_isr(int eIntNum);


#endif /*__MTK_DRV_VBI_H__*/
