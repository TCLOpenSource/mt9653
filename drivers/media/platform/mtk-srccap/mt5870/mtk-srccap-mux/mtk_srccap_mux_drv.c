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

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include "mtk_srccap.h"
#include "apiXC.h"

static int _mux_buffer(int cid, int dev_id, void *para,
				enum srccap_access_type enAccesType)
{
	if (dev_id >=  SRCCAP_DEV_NUM) {
		SRCCAP_MSG_DEBUG("dev_id is out-range\n");
		return -1;
	}

	switch (cid) {
	default:
		break;
	}

	return 0;
}

void mtk_srccap_drv_mux_set_source(
			enum v4l2_srccap_input_source enInputSource,
			struct mtk_srccap_dev *srccap_dev)
{
	static enum v4l2_srccap_input_source _enInputSource[SRCCAP_DEV_NUM] = {
						V4L2_SRCCAP_INPUT_SOURCE_NONE};
	SCALER_WIN enWin = (SCALER_WIN)srccap_dev->dev_id;
	E_DEST_TYPE enDstType = OUTPUT_NONE;
	MS_S16 s16PathID = 0;

	if (enWin == MAIN_WINDOW) {
		//need fix, elvis
		//type should come from dts
		enDstType = OUTPUT_SCALER_MAIN_WINDOW;
	} else {
		//need fix, elvis
		//type should come from dts
		enDstType = OUTPUT_SCALER2_MAIN_WINDOW;
	}

	if (enInputSource == V4L2_SRCCAP_INPUT_SOURCE_NONE) {
		enInputSource = _enInputSource[srccap_dev->dev_id];
		MApi_XC_Mux_DeletePath((INPUT_SOURCE_TYPE_t)enInputSource,
								enDstType);
		_enInputSource[srccap_dev->dev_id] =
					V4L2_SRCCAP_INPUT_SOURCE_NONE;
	} else {
		XC_MUX_PATH_INFO stMuxInfo;
		MS_U32 u32DataLength = sizeof(XC_MUX_PATH_INFO);

		memset(&stMuxInfo, 0, u32DataLength);
		stMuxInfo.Path_Type = PATH_TYPE_SYNCHRONOUS;
		stMuxInfo.src = (INPUT_SOURCE_TYPE_t)enInputSource;
		stMuxInfo.dest = enDstType;
		stMuxInfo.path_thread = NULL;
		stMuxInfo.SyncEventHandler = NULL;
		stMuxInfo.DestOnOff_Event_Handler = NULL;
		stMuxInfo.dest_periodic_handler = NULL;

		_enInputSource[srccap_dev->dev_id] = enInputSource;

		s16PathID = MApi_XC_Mux_CreatePath(&stMuxInfo, u32DataLength);
		if (s16PathID > 0)
			MApi_XC_Mux_EnablePath((MS_U16)s16PathID);

		MApi_XC_SetInputSource((INPUT_SOURCE_TYPE_t)enInputSource,
								enWin);
	}
}

int mtk_srccap_drv_mux_skip_swreset(bool bSkip)
{
	MApi_XC_SkipSWReset(bSkip);

	return 0;
}

int mtk_srccap_drv_mux_disable_inputsource(bool bDisable,
			struct mtk_srccap_dev *srccap_dev)
{
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;

	MApi_XC_DisableInputSource(bDisable, enWindow);

	return 0;
}


