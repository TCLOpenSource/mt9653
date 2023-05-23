// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include <linux/mtk-v4l2-srccap.h>
#include "sti_msos.h"
#include "mtk_drv_vbi.h"
#include "mtk_srccap.h"
#include "show_param_vbi.h"
#include "vbi-ex.h"

#define UTOPIA_DECOMPOSE  //for vbi decompose

#ifdef UTOPIA_DECOMPOSE
#include "drvVBI.h"
#endif

#define TEST_MODE         0   // 1: for TEST

/******************************************************************************/
/* Global Type Definitions                                                    */
/******************************************************************************/

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
void mdrv_vbi_isr(int eIntNum)
{
	MDrv_VBI_ISR(eIntNum);
}

