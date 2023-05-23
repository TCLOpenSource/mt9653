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
#include "apiXC_PCMonitor.h"

static int _common_buffer(int cid, int dev_id, void *para,
				enum srccap_access_type enAccesType)
{
	if (dev_id >=  SRCCAP_DEV_NUM) {
		SRCCAP_MSG_DEBUG("dev_id is out-range\n");
		return -1;
	}

	switch (cid) {
	case V4L2_CID_OUTPUT_EXT_INFO:
	{
		static enum v4l2_ext_info_type
				_enExtInfoType[SRCCAP_DEV_NUM] = {0};
		struct v4l2_output_ext_info *pstOutputExtInfo =
				(struct v4l2_output_ext_info *)para;
		if (enAccesType == SRCCAP_SET)
			_enExtInfoType[dev_id] = pstOutputExtInfo->type;
		else
			pstOutputExtInfo->type = _enExtInfoType[dev_id];
		break;
	}
	default:
		break;
	}

	return 0;
}

int mtk_srccap_drv_testpattern(
			struct v4l2_srccap_testpattern *pstSrccapTestPattern,
			struct mtk_srccap_dev *srccap_dev)
{
	switch (pstSrccapTestPattern->enPatternType) {
	case V4L2_SRCCAP_PATTERN_ADC:
	{
		XC_SET_ADC_TESTPATTERN_t stADCTestPattern;

		SRCCAP_MSG_DEBUG("V4L2_CID_SRCCAP_TESTPATTERN :\n"
					"adc test pattern\n");

		memset(&stADCTestPattern, 0, sizeof(XC_SET_ADC_TESTPATTERN_t));
		stADCTestPattern.u8EnableADCType =
			pstSrccapTestPattern->stADCPattern.u8EnableADCType;
		stADCTestPattern.u16Ramp =
			pstSrccapTestPattern->stADCPattern.u16Ramp;
		MApi_XC_GenerateTestPattern(E_XC_ADC_PATTERN_MODE,
			&stADCTestPattern, sizeof(XC_SET_ADC_TESTPATTERN_t));
		break;
	}
	case V4L2_SRCCAP_PATTERN_MUX:
	{
		XC_SET_IPMUX_TESTPATTERN_t stMuxTestPattern;

		SRCCAP_MSG_DEBUG("V4L2_CID_SRCCAP_TESTPATTERN :\n"
					"ipmux test pattern\n");

		memset(&stMuxTestPattern, 0,
					sizeof(XC_SET_IPMUX_TESTPATTERN_t));
		stMuxTestPattern.bEnable =
			pstSrccapTestPattern->stMuxPattern.bEnable;
		stMuxTestPattern.u16R_CR_Data =
			pstSrccapTestPattern->stMuxPattern.u16R_CR_Data;
		stMuxTestPattern.u16G_Y_Data =
			pstSrccapTestPattern->stMuxPattern.u16G_Y_Data;
		stMuxTestPattern.u16B_CB_Data =
			pstSrccapTestPattern->stMuxPattern.u16B_CB_Data;
		MApi_XC_GenerateTestPattern(E_XC_IPMUX_PATTERN_MODE,
			&stMuxTestPattern, sizeof(XC_SET_IPMUX_TESTPATTERN_t));
		break;
	}
	default:
		break;
	}

	return 0;
}

int mtk_srccap_drv_set_output_info(
			struct v4l2_output_ext_info *pstOutputExtInfo,
			struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = _common_buffer(V4L2_CID_OUTPUT_EXT_INFO, srccap_dev->dev_id,
				(void *)pstOutputExtInfo, SRCCAP_SET);

	return ret;
}

int mtk_srccap_drv_get_output_info(
			struct v4l2_output_ext_info *pstOutputExtInfo,
			struct mtk_srccap_dev *srccap_dev)
{
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;
	int ret = 0;

	ret = _common_buffer(V4L2_CID_OUTPUT_EXT_INFO, srccap_dev->dev_id,
				(void *)pstOutputExtInfo, SRCCAP_GET);
	if (ret != 0)
		return -1;

	switch (pstOutputExtInfo->type) {
	case V4L2_EXT_INFO_TYPE_TIMING:
	{
		MS_WINDOW_TYPE stWin;
		__u8 u8SyncStatus = 0;

		SRCCAP_MSG_DEBUG("V4L2_CID_OUTPUT_EXT_INFO :\n"
					"get ext timing info\n");

		memset(&stWin, 0, sizeof(MS_WINDOW_TYPE));
		pstOutputExtInfo->v4l2Timing.v_total =
			MApi_XC_PCMonitor_Get_Vtotal(enWindow);
		pstOutputExtInfo->v4l2Timing.h_freq =
			MApi_XC_PCMonitor_Get_HFreqx1K(enWindow);
		pstOutputExtInfo->v4l2Timing.v_freq =
			MApi_XC_PCMonitor_Get_VFreqx1KNoDelay(enWindow,
			E_XC_PCMONITOR_VFREQ_CHECKING_NODELAY);
		MApi_XC_PCMonitor_Get_Dvi_Hdmi_De_Info(enWindow, &stWin);
		pstOutputExtInfo->v4l2Timing.de_x = stWin.x;
		pstOutputExtInfo->v4l2Timing.de_y = stWin.y;
		pstOutputExtInfo->v4l2Timing.de_width = stWin.width;
		pstOutputExtInfo->v4l2Timing.de_height = stWin.height;
		u8SyncStatus = MApi_XC_PCMonitor_GetSyncStatus(enWindow);
		pstOutputExtInfo->v4l2Timing.interlance =
			(u8SyncStatus & XC_MD_INTERLACE_BIT)?1:0;
		pstOutputExtInfo->v4l2Timing.sync_status = u8SyncStatus;
		break;
	}
	default:
		break;
	}

	return 0;
}

