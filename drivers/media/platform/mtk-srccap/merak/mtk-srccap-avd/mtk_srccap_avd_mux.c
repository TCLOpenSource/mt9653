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
#include "mtk_srccap_avd_mux.h"
#include "mtk_srccap.h"
#include "show_param.h"
#include "avd-ex.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static enum v4l2_ext_avd_Inputsourcetype _MappingAvdInputSourceType(
	enum v4l2_srccap_input_source esource);

static enum v4l2_ext_avd_Inputsourcetype _MappingAvdInputSourceType(
	enum v4l2_srccap_input_source esource)
{
	enum v4l2_ext_avd_Inputsourcetype eVideoSourceType;

	switch (esource) {
	case V4L2_SRCCAP_INPUT_SOURCE_NONE:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_INVVAILD;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_ATV;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_CVBS1;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_CVBS2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_SVIDEO1;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_SVIDEO2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_SCART1;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_SCAER2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_YPbPr;
		break;
	default:
		eVideoSourceType = V4L2_EXT_AVD_SOURCE_INVVAILD;
		break;
	}

	return eVideoSourceType;
}
/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
enum V4L2_AVD_RESULT mtk_avd_input(unsigned char *u8ScartFB, struct mtk_srccap_dev *srccap_dev)
{
#ifdef DECOMPOSE
	enum v4l2_ext_avd_Inputsourcetype eVideoSourceType;
#endif
	SRCCAP_AVD_MSG_INFO("in\n");
	show_v4l2_ext_avd_input(u8ScartFB);

#ifdef DECOMPOSE
	eVideoSourceType = _MappingAvdInputSourceType(srccap_dev->avd_info.eInputSource);

	if (eVideoSourceType > V4L2_EXT_AVD_SOURCE_YPbPr)
		SRCCAP_AVD_MSG_ERROR("Out of input source(%d).\n", eVideoSourceType);

	if (Api_AVD_SetInput(eVideoSourceType, *u8ScartFB) == 0) {
		SRCCAP_AVD_MSG_ERROR("Fail(-1)\n");
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

