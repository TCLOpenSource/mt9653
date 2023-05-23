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
#include "apiXC_ModeParse.h"
#include "apiXC_PCMonitor.h"

static struct v4l2_srccap_timing_mode_table _gstTimingModeTable[SRCCAP_DEV_NUM];

static int _detect_timing_buffer(int cid, int dev_id, void *para,
				enum srccap_access_type enAccesType)
{
	if (dev_id >= SRCCAP_DEV_NUM) {
		SRCCAP_MSG_DEBUG("dev_id is out-range\n");
		return -1;
	}

	switch (cid) {
	case V4L2_CID_TIMINGDETECT_MONITOR:
	{
		struct v4l2_srccap_pcm *pstPCM =
					(struct v4l2_srccap_pcm *)para;
		static enum v4l2_srccap_input_source
			_enSource[SRCCAP_DEV_NUM] = {
			V4L2_SRCCAP_INPUT_SOURCE_NONE};

		if (enAccesType == SRCCAP_SET)
			_enSource[dev_id] = pstPCM->enSrcType;
		else
			pstPCM->enSrcType = _enSource[dev_id];

		break;
	}
	case V4L2_CID_TIMINGDETECT_INVALID_TIMING_DETECT:
	{
		struct v4l2_srccap_pcm_invalid_timing_detect
			*pstInvalidTimingDetect =
			(struct v4l2_srccap_pcm_invalid_timing_detect *)para;
		static bool _bPollingOnly[SRCCAP_DEV_NUM] = {false};

		if (enAccesType == SRCCAP_SET) {
			_bPollingOnly[dev_id] =
					pstInvalidTimingDetect->bPollingOnly;
		} else {
			pstInvalidTimingDetect->bPollingOnly =
					_bPollingOnly[dev_id];
		}
		break;
	}
	default:
		break;
	}

	return 0;
}

int mtk_srccap_timingdetect_drv_init(struct mtk_srccap_dev *srccap_dev)
{
	int dev_id = srccap_dev->dev_id;

	//init variable
	_gstTimingModeTable[dev_id].p.stModeDB = NULL;
	_gstTimingModeTable[dev_id].u8NumberOfItems = 0;

	return 0;
}

int mtk_srccap_timingdetect_drv_exit(struct mtk_srccap_dev *srccap_dev)
{
	int dev_id = srccap_dev->dev_id;

	if (_gstTimingModeTable[dev_id].p.stModeDB != NULL) {
		kfree(_gstTimingModeTable[dev_id].p.stModeDB);
		_gstTimingModeTable[dev_id].p.stModeDB = NULL;
	}
	return 0;
}

int mtk_srccap_drv_get_hdmi_sync_mode(
			enum v4l2_srccap_hdmi_sync_mode *enSyncMode)
{
	*enSyncMode =
		(enum v4l2_srccap_hdmi_sync_mode)MApi_XC_GetHdmiSyncMode();

	return 0;
}

int mtk_srccap_drv_get_current_pcm_status(
			enum v4l2_srccap_pcm_status *enPCMStatus,
			struct mtk_srccap_dev *srccap_dev)
{
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;

	*enPCMStatus = (enum v4l2_srccap_pcm_status)
			MApi_XC_PCMonitor_GetCurrentState(enWindow);

	return 0;
}

int mtk_srccap_drv_detect_timing_pcm_init(void)
{
	MApi_XC_PCMonitor_Init(SRCCAP_DEV_NUM);

	return 0;
}

int mtk_srccap_drv_detect_timing_restart(struct mtk_srccap_dev *srccap_dev)
{
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;

	MApi_XC_PCMonitor_Restart(enWindow);

	return 0;
}

int mtk_srccap_drv_auto_no_signal(bool *bEnable,
	enum srccap_access_type enAccessType, struct mtk_srccap_dev *srccap_dev)
{
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;

	if (enAccessType == SRCCAP_SET)
		MApi_XC_EnableIPAutoNoSignal(*bEnable, enWindow);
	else
		*bEnable = MApi_XC_GetIPAutoNoSignal(enWindow);

	return 0;
}

int mtk_srccap_drv_set_detect_timing_counter(
			struct v4l2_srccap_pcm_timing_count *pstTimingCounter)
{
	MApi_XC_PCMonitor_SetTimingCountEx(
		(INPUT_SOURCE_TYPE_t)pstTimingCounter->enSrcType,
		pstTimingCounter->u16TimingStableCounter,
		pstTimingCounter->u16TimingNosyncCounter);

	return 0;
}

int mtk_srccap_drv_set_pcm(struct v4l2_srccap_pcm *pstPCM,
			struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = _detect_timing_buffer(V4L2_CID_TIMINGDETECT_MONITOR,
			srccap_dev->dev_id, pstPCM, SRCCAP_SET);

	return ret;
}

int mtk_srccap_drv_get_pcm(struct v4l2_srccap_pcm *pstPCM,
			struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;

	ret = _detect_timing_buffer(V4L2_CID_TIMINGDETECT_MONITOR,
			srccap_dev->dev_id, pstPCM, SRCCAP_GET);
	if (ret != 0)
		return -1;

	pstPCM->enPCMStatus = (enum v4l2_srccap_pcm_status)MApi_XC_PCMonitor(
			(INPUT_SOURCE_TYPE_t)pstPCM->enSrcType, enWindow);

	return 0;
}

int mtk_srccap_drv_set_invalid_timing_detect(
	struct v4l2_srccap_pcm_invalid_timing_detect *pstInvalidTimingDetect,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = _detect_timing_buffer(
		V4L2_CID_TIMINGDETECT_INVALID_TIMING_DETECT,
		srccap_dev->dev_id, pstInvalidTimingDetect, SRCCAP_SET);

	return ret;
}

int mtk_srccap_drv_get_invalid_timing_detect(
	struct v4l2_srccap_pcm_invalid_timing_detect *pstInvalidTimingDetect,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;

	ret = _detect_timing_buffer(
		V4L2_CID_TIMINGDETECT_INVALID_TIMING_DETECT,
		srccap_dev->dev_id, pstInvalidTimingDetect, SRCCAP_GET);
	if (ret != 0)
		return -1;

	pstInvalidTimingDetect->bInvalid =
		MApi_XC_PCMonitor_InvalidTimingDetect(
		pstInvalidTimingDetect->bPollingOnly, enWindow);

	return 0;
}

int mtk_srccap_drv_mode_table(
		struct v4l2_srccap_timing_mode_table *pstTimingModeTable,
		struct mtk_srccap_dev *srccap_dev)
{
	int NumOfItems = 0, dev_id = 0;

	dev_id = srccap_dev->dev_id;
	NumOfItems = pstTimingModeTable->u8NumberOfItems;
	_gstTimingModeTable[dev_id].u8NumberOfItems = NumOfItems;

	if (_gstTimingModeTable[dev_id].p.stModeDB != NULL) {
		kfree(_gstTimingModeTable[dev_id].p.stModeDB);
		_gstTimingModeTable[dev_id].p.stModeDB = NULL;
	}

	_gstTimingModeTable[dev_id].p.stModeDB =
		kmalloc(
		sizeof(struct v4l2_srccap_mp_pcadc_modetable_type)*NumOfItems,
		GFP_KERNEL);
//		(struct v4l2_srccap_mp_pcadc_modetable_type *)kmalloc(
//		sizeof(struct v4l2_srccap_mp_pcadc_modetable_type)*NumOfItems,
//		GFP_KERNEL);

	if (_gstTimingModeTable[dev_id].p.stModeDB == NULL)
		return -ENOMEM;

	int ret = 0;

	ret = copy_from_user(_gstTimingModeTable[dev_id].p.stModeDB,
		compat_ptr(pstTimingModeTable->p.stModeDB),
		sizeof(struct v4l2_srccap_mp_pcadc_modetable_type)*NumOfItems);
	if (ret != 0) {
		SRCCAP_MSG_ERROR("V4L2_CID_TIMINGDETECT_TIMING_TABLE copy\n"
					"from user fail\n");
		return -EFAULT;
	}

	return 0;
}

int mtk_srccap_drv_update_setting(
		struct v4l2_srccap_mp_input_info *pstSrccapUpdateSetting,
		struct mtk_srccap_dev *srccap_dev)
{
	int dev_id = srccap_dev->dev_id;
	MS_PCADC_MODETABLE_TYPE_EX *pDrvModeDB = NULL;
	MS_U8 u8NumberOfItems = 0;
	XC_MODEPARSE_INPUT_INFO sInputInfo;
	XC_MODEPARSE_RESULT enModeParseRet = XC_MODEPARSE_NOT_PARSED;
	int ret = 0;

	u8NumberOfItems = _gstTimingModeTable[dev_id].u8NumberOfItems;
	pDrvModeDB = kmalloc_array(
		u8NumberOfItems, sizeof(MS_PCADC_MODETABLE_TYPE_EX),
		GFP_KERNEL);
	if (pDrvModeDB == NULL)
		return -ENOMEM;

	memset(&sInputInfo, 0, sizeof(XC_MODEPARSE_INPUT_INFO));
	memset(pDrvModeDB, 0,
		sizeof(MS_PCADC_MODETABLE_TYPE_EX)*u8NumberOfItems);
	memcpy(pDrvModeDB, _gstTimingModeTable[dev_id].p.stModeDB,
		sizeof(MS_PCADC_MODETABLE_TYPE_EX)*u8NumberOfItems);

	enModeParseRet = MApi_XC_ModeParse_MatchModeEx(pDrvModeDB,
				u8NumberOfItems, &sInputInfo);

	memcpy(pstSrccapUpdateSetting, &sInputInfo,
				sizeof(struct v4l2_srccap_mp_input_info));
	pstSrccapUpdateSetting->enMatchMode = enModeParseRet;

	kfree(pDrvModeDB);

	return ret;
}

