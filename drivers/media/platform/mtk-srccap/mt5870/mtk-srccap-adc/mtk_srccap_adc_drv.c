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
#include "apiXC_Adc.h"
#include "apiXC_Auto.h"

static enum v4l2_srccap_input_source _genInputSource[SRCCAP_DEV_NUM] = {
					V4L2_SRCCAP_INPUT_SOURCE_NONE};

static int _adc_buffer(int cid, int dev_id, void *para,
				enum srccap_access_type enAccesType)
{
	if (dev_id >= SRCCAP_DEV_NUM) {
		SRCCAP_MSG_DEBUG("dev_id is out-range\n");
		return -1;
	}

	switch (cid) {
	case V4L2_CID_ADC_DEFAULT_GAIN_OFFSET:
	{
		static enum v4l2_srccap_input_source
			_enInputSource[SRCCAP_DEV_NUM] = {
			V4L2_SRCCAP_INPUT_SOURCE_NONE};
		struct v4l2_srccap_default_gain_offset *pstDefaultGainOffset =
			(struct v4l2_srccap_default_gain_offset *)para;
		if (enAccesType == SRCCAP_SET) {
			_enInputSource[dev_id] =
				pstDefaultGainOffset->enSrcType;
		} else {
			pstDefaultGainOffset->enSrcType =
				_enInputSource[dev_id];
		}
		break;
	}
	case V4L2_CID_ADC_AUTO_HWFIXED_GAIN_OFFSET:
	{
		static enum v4l2_srccap_auto_tune_type
		enAutoTuneType[SRCCAP_DEV_NUM] = {V4L2_SRCCAP_AUTO_TUNE_NULL};
		struct v4l2_srccap_auto_hwfixed_gain_offset
			*pstHWFixedGainOffset =
			(struct v4l2_srccap_auto_hwfixed_gain_offset *)para;
		if (enAccesType == SRCCAP_SET) {
			enAutoTuneType[dev_id] =
				pstHWFixedGainOffset->enAutoTuneType;
		} else {
			pstHWFixedGainOffset->enAutoTuneType =
				enAutoTuneType[dev_id];
		}
		break;
	}
	case V4L2_CID_ADC_AUTO_SYNC_INFO:
	{
		static enum v4l2_srccap_input_source
			_enInputSource[SRCCAP_DEV_NUM] = {
			V4L2_SRCCAP_INPUT_SOURCE_NONE};
		struct v4l2_srccap_auto_sync_info *pstAutoSyncInfo =
			(struct v4l2_srccap_auto_sync_info *)para;
		if (enAccesType == SRCCAP_SET)
			_enInputSource[dev_id] = pstAutoSyncInfo->enSrcType;
		else
			pstAutoSyncInfo->enSrcType = _enInputSource[dev_id];
		break;
	}
	case V4L2_CID_ADC_IP_STATUS:
	{
		static __u32 _u32Version[SRCCAP_DEV_NUM] = {0},
			_u32Length[SRCCAP_DEV_NUM] = {0};
		static enum v4l2_srccap_input_value_type
			_enInputValueType[SRCCAP_DEV_NUM] = {
			V4L2_SRCCAP_INPUT_MIN_R_VALUE};
		struct v4l2_srccap_ip_status *pstIPStatus =
			(struct v4l2_srccap_ip_status *)para;
		if (enAccesType == SRCCAP_SET) {
			_u32Version[dev_id] = pstIPStatus->u32Version;
			_u32Length[dev_id] = pstIPStatus->u32Length;
			_enInputValueType[dev_id] =
				pstIPStatus->enInputValueType;
		} else {
			pstIPStatus->u32Version = _u32Version[dev_id];
			pstIPStatus->u32Length = _u32Length[dev_id];
			pstIPStatus->enInputValueType =
				_enInputValueType[dev_id];
		}
		break;
	}
	default:
		break;
	}

	return 0;
}

void mtk_srccap_drv_adc_set_source(
				enum v4l2_srccap_input_source enInputSource,
				struct mtk_srccap_dev *srccap_dev)
{
	_genInputSource[srccap_dev->dev_id] = enInputSource;
}

int mtk_srccap_drv_adc_pcclock(__u16 *u16PCClk,
				enum srccap_access_type enAccessType)
{
	if (enAccessType == SRCCAP_SET)
		MApi_XC_ADC_SetPcClock(*u16PCClk);
	else
		*u16PCClk = MApi_XC_ADC_GetPcClock();

	return 0;
}

int mtk_srccap_drv_adc_phase(__u16 *u16Phase,
				enum srccap_access_type enAccessType)
{
	if (enAccessType == SRCCAP_SET)
		MApi_XC_ADC_SetPhaseEx(*u16Phase);
	else
		*u16Phase = MApi_XC_ADC_GetPhaseEx();

	return 0;
}

int mtk_srccap_drv_adc_source_calibration(struct mtk_srccap_dev *srccap_dev)
{
	MApi_XC_ADC_Source_Calibrate(
		(INPUT_SOURCE_TYPE_t)_genInputSource[srccap_dev->dev_id]);

	return 0;
}

int mtk_srccap_drv_adc_get_phase_range(__u16 *u16Range)
{
	*u16Range = MApi_XC_ADC_GetPhaseRange();

	return 0;
}

int mtk_srccap_drv_adc_is_ScartRGB(bool *bIsScartRGB)
{
	*bIsScartRGB = MApi_XC_ADC_IsScartRGB();

	return 0;
}

int mtk_srccap_drv_adc_set_default_gainoffset(
	struct v4l2_srccap_default_gain_offset *pstDefaultGainOffset,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = _adc_buffer(V4L2_CID_ADC_DEFAULT_GAIN_OFFSET,
	srccap_dev->dev_id, (void *)pstDefaultGainOffset, SRCCAP_SET);

	return ret;
}

int mtk_srccap_drv_adc_get_default_gainoffset(
	struct v4l2_srccap_default_gain_offset *pstDefaultGainOffset,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = _adc_buffer(V4L2_CID_ADC_DEFAULT_GAIN_OFFSET,
	srccap_dev->dev_id, (void *)pstDefaultGainOffset, SRCCAP_GET);

	APIXC_AdcGainOffsetSetting stADCGainOffset;

	memset(&stADCGainOffset, 0, sizeof(APIXC_AdcGainOffsetSetting));

	MApi_XC_ADC_GetDefaultGainOffset(
		(INPUT_SOURCE_TYPE_t)pstDefaultGainOffset->enSrcType,
		&stADCGainOffset);

	memcpy(&(pstDefaultGainOffset->stADCSetting), &stADCGainOffset,
		sizeof(APIXC_AdcGainOffsetSetting));

	return ret;
}

int mtk_srccap_drv_adc_max_offset(__u16 *u16MaxOffset)
{
	*u16MaxOffset = MApi_XC_ADC_GetMaximalOffsetValue();

	return 0;
}

int mtk_srccap_drv_adc_max_gain(__u16 *u16MaxGain)
{
	*u16MaxGain = MApi_XC_ADC_GetMaximalGainValue();

	return 0;
}

int mtk_srccap_drv_adc_center_gain(__u16 *u16CenterGain)
{
	*u16CenterGain = MApi_XC_ADC_GetCenterGain();

	return 0;
}

int mtk_srccap_drv_adc_center_offset(__u16 *u16CenterOffset)
{
	*u16CenterOffset = MApi_XC_ADC_GetCenterOffset();

	return 0;
}

int mtk_srccap_drv_adc_adjust_gainoffset(
	struct v4l2_srccap_gainoffset_setting *pstGainOffsetSetting)
{
	APIXC_AdcGainOffsetSetting stADCSetting;

	stADCSetting.u16RedGain = pstGainOffsetSetting->u16RedGain;
	stADCSetting.u16GreenGain = pstGainOffsetSetting->u16GreenGain;
	stADCSetting.u16BlueGain = pstGainOffsetSetting->u16BlueGain;
	stADCSetting.u16RedOffset = pstGainOffsetSetting->u16RedOffset;
	stADCSetting.u16GreenOffset = pstGainOffsetSetting->u16GreenOffset;
	stADCSetting.u16BlueOffset = pstGainOffsetSetting->u16BlueOffset;

	MApi_XC_ADC_AdjustGainOffset(&stADCSetting);

	return 0;
}

int mtk_srccap_drv_adc_rgb_pipedelay(__u8 u8Pipedelay)
{
	MApi_XC_ADC_SetRGB_PIPE_Delay(u8Pipedelay);

	return 0;
}

int mtk_srccap_drv_adc_enable_hw_calibration(bool bEnable)
{
	MApi_XC_ADC_EnableHWCalibration(bEnable);

	return 0;
}

int mtk_srccap_drv_adc_auto_geometry(
	struct v4l2_srccap_auto_geometry *pstAutoGeometry,
	struct mtk_srccap_dev *srccap_dev)
{
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;
	XC_Auto_Signal_Info_Ex stActiveInfo, stStandardInfo;

	memset(&stActiveInfo, 0, sizeof(XC_Auto_Signal_Info_Ex));
	memset(&stStandardInfo, 0, sizeof(XC_Auto_Signal_Info_Ex));
	memcpy(&stActiveInfo, &(pstAutoGeometry->stActiveInfo),
		sizeof(struct v4l2_srccap_signal_info));
	memcpy(&stStandardInfo, &(pstAutoGeometry->stStandardInfo),
		sizeof(struct v4l2_srccap_signal_info));

	pstAutoGeometry->bTuneSuccess = MApi_XC_Auto_Geometry_Ex(
		(XC_Auto_TuneType)pstAutoGeometry->enAutoTuneType,
		&stActiveInfo, &stStandardInfo, enWindow);

	memcpy(&(pstAutoGeometry->stActiveInfo), &stActiveInfo,
		sizeof(struct v4l2_srccap_signal_info));

	return 0;
}

int mtk_srccap_drv_adc_auto_gainoffset(
	struct v4l2_srccap_auto_gainoffset *pstAutoGainOffset,
	struct mtk_srccap_dev *srccap_dev)
{
	SCALER_WIN enWindow = (SCALER_WIN)srccap_dev->dev_id;
	APIXC_AdcGainOffsetSetting stGainOffsetSetting;

	memset(&stGainOffsetSetting, 0, sizeof(APIXC_AdcGainOffsetSetting));
	memcpy(&stGainOffsetSetting, &(pstAutoGainOffset->stADCSetting),
		sizeof(struct v4l2_srccap_gainoffset_setting));

	//pstAutoGainOffset->bTuneSuccess = MApi_XC_Auto_GainOffset(
	//(XC_Auto_CalibrationType)pstAutoGainOffset->enAutoCaliType,
	//(XC_Auto_TuneType)pstAutoGainOffset->enAutoTuneType,
	//&stGainOffsetSetting, enWindow);

	memcpy(&(pstAutoGainOffset->stADCSetting), &stGainOffsetSetting,
	sizeof(struct v4l2_srccap_gainoffset_setting));

	return 0;
}

int mtk_srccap_drv_adc_set_hwfixed_gainoffset(
	struct v4l2_srccap_auto_hwfixed_gain_offset *pstHWFixedGainOffset,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = _adc_buffer(V4L2_CID_ADC_AUTO_HWFIXED_GAIN_OFFSET,
		srccap_dev->dev_id, (void *)pstHWFixedGainOffset,
		SRCCAP_SET);

	return ret;
}


int mtk_srccap_drv_adc_get_hwfixed_gainoffset(
	struct v4l2_srccap_auto_hwfixed_gain_offset *pstHWFixedGainOffset,
	struct mtk_srccap_dev *srccap_dev)
{
	bool bRet = false;
	APIXC_AdcGainOffsetSetting stGainOffsetSetting;

	memset(&stGainOffsetSetting, 0, sizeof(APIXC_AdcGainOffsetSetting));
	memcpy(&stGainOffsetSetting, &(pstHWFixedGainOffset->stADCSetting),
		sizeof(struct v4l2_srccap_gainoffset_setting));

	_adc_buffer(V4L2_CID_ADC_AUTO_HWFIXED_GAIN_OFFSET, srccap_dev->dev_id,
		(void *)pstHWFixedGainOffset, SRCCAP_GET);

	bRet = MApi_XC_Auto_GetHWFixedGainOffset(
		(XC_Auto_TuneType)pstHWFixedGainOffset->enAutoTuneType,
		&stGainOffsetSetting);

	memcpy(&(pstHWFixedGainOffset->stADCSetting), &stGainOffsetSetting,
		sizeof(struct v4l2_srccap_gainoffset_setting));

	if (bRet == false)
		return -1;
	else
		return 0;

}

int mtk_srccap_drv_adc_auto_offset(
	struct v4l2_srccap_auto_offset *pstAutoOffset)
{
	MApi_XC_Auto_AutoOffset(pstAutoOffset->bEnable,
		pstAutoOffset->bYpbprFlag);

	return 0;
}

int mtk_srccap_drv_adc_auto_calibration_mode(
	enum v4l2_srccap_calibration_mode enMode)
{
	MApi_XC_Auto_SetCalibrationMode((XC_Auto_CalibrationMode)enMode);

	return 0;
}

int mtk_srccap_drv_adc_set_auto_sync_info(
	struct v4l2_srccap_auto_sync_info *pstAutoSyncInfo,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = _adc_buffer(V4L2_CID_ADC_AUTO_SYNC_INFO, srccap_dev->dev_id,
		(void *)pstAutoSyncInfo, SRCCAP_SET);

	return ret;
}

int mtk_srccap_drv_adc_get_auto_sync_info(
	struct v4l2_srccap_auto_sync_info *pstAutoSyncInfo,
	struct mtk_srccap_dev *srccap_dev)
{
	XC_AUTO_SYNC_INFO stSyncInfo;

	memset(&stSyncInfo, 0, sizeof(XC_AUTO_SYNC_INFO));
	_adc_buffer(V4L2_CID_ADC_AUTO_SYNC_INFO, srccap_dev->dev_id,
		(void *)pstAutoSyncInfo, SRCCAP_GET);
	stSyncInfo.eWindow = (SCALER_WIN)srccap_dev->dev_id;

	MApi_XC_AUTO_GetSyncInfo(&stSyncInfo);

	pstAutoSyncInfo->enSyncPolarity =
		(enum v4l2_srccap_auto_sync_por)stSyncInfo.eSyncPolarity;
	pstAutoSyncInfo->u32HSyncStart = stSyncInfo.u32HSyncStart;
	pstAutoSyncInfo->u32HSyncEnd = stSyncInfo.u32HSyncEnd;

	return 0;
}

int mtk_srccap_drv_adc_set_slt_ip_status(
	struct v4l2_srccap_ip_status *pstIPStatus,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = _adc_buffer(V4L2_CID_ADC_IP_STATUS, srccap_dev->dev_id,
		(void *)pstIPStatus, SRCCAP_SET);

	return ret;
}

int mtk_srccap_drv_adc_get_slt_ip_status(
	struct v4l2_srccap_ip_status *pstIPStatus,
	struct mtk_srccap_dev *srccap_dev)
{
	ST_XC_IP_STATUS stIPStatus;
	int ret = 0;
	MS_U16 u16InputValue = 0;

	memset(&stIPStatus, 0, sizeof(ST_XC_IP_STATUS));

	ret = _adc_buffer(V4L2_CID_ADC_IP_STATUS, srccap_dev->dev_id,
		(void *)pstIPStatus, SRCCAP_GET);

	stIPStatus.u32Version = pstIPStatus->u32Version;
	stIPStatus.u32Length = sizeof(ST_XC_IP_STATUS);
	stIPStatus.u8Win = (MS_U8)srccap_dev->dev_id;
	stIPStatus.enInputValueType =
		(EN_XC_INPUT_VALUE_TYPE)pstIPStatus->enInputValueType;
	stIPStatus.pu16InputValue = &(pstIPStatus->u16InputValue);

	MApi_XC_GetIPStatus(&stIPStatus);

	return ret;
}

