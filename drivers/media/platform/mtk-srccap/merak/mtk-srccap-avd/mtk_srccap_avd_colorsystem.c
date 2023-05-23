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
#include "mtk_srccap_avd_colorsystem.h"
#include "mtk_srccap.h"
#include "show_param.h"
#include "avd-ex.h"

/* ============================================================================================== */
/* -------------------------------------- Global Type Definitions ------------------------------- */
/* ============================================================================================== */
v4l2_std_id u64TvSystem;
const struct std_descr standards[] = {
	{ V4L2_STD_NTSC,         "NTSC"      },
	{ V4L2_STD_NTSC_M,       "NTSC-M"    },
	{ V4L2_STD_NTSC_M_JP,    "NTSC-M-JP" },
	{ V4L2_STD_NTSC_M_KR,    "NTSC-M-KR" },
	{ V4L2_STD_NTSC_443,     "NTSC-443"  },
	{ V4L2_STD_PAL,          "PAL"       },
	{ V4L2_STD_PAL_BG,       "PAL-BG"    },
	{ V4L2_STD_PAL_B,        "PAL-B"     },
	{ V4L2_STD_PAL_B1,       "PAL-B1"    },
	{ V4L2_STD_PAL_G,        "PAL-G"     },
	{ V4L2_STD_PAL_H,        "PAL-H"     },
	{ V4L2_STD_PAL_I,        "PAL-I"     },
	{ V4L2_STD_PAL_DK,       "PAL-DK"    },
	{ V4L2_STD_PAL_D,        "PAL-D"     },
	{ V4L2_STD_PAL_D1,       "PAL-D1"    },
	{ V4L2_STD_PAL_K,        "PAL-K"     },
	{ V4L2_STD_PAL_M,        "PAL-M"     },
	{ V4L2_STD_PAL_N,        "PAL-N"     },
	{ V4L2_STD_PAL_Nc,       "PAL-Nc"    },
	{ V4L2_STD_PAL_60,       "PAL-60"    },
	{ V4L2_STD_SECAM,        "SECAM"     },
	{ V4L2_STD_SECAM_B,      "SECAM-B"   },
	{ V4L2_STD_SECAM_G,      "SECAM-G"   },
	{ V4L2_STD_SECAM_H,      "SECAM-H"   },
	{ V4L2_STD_SECAM_DK,     "SECAM-DK"  },
	{ V4L2_STD_SECAM_D,      "SECAM-D"   },
	{ V4L2_STD_SECAM_K,      "SECAM-K"   },
	{ V4L2_STD_SECAM_K1,     "SECAM-K1"  },
	{ V4L2_STD_SECAM_L,      "SECAM-L"   },
	{ V4L2_STD_SECAM_LC,     "SECAM-Lc"  },
	{ 0,                     "Unknown"   }
};
/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static enum v4l2_ext_avd_videostandardtype _MappingV4l2stdToVideoStandard(
			v4l2_std_id std_id);
static v4l2_std_id _MappingVideoStandardToV4l2std(
		enum v4l2_ext_avd_videostandardtype eVideoStandardType);
static bool _IsVifChLock(struct mtk_srccap_dev *srccap_dev);
static bool _IsVdScanDetect(struct mtk_srccap_dev *srccap_dev);
static enum V4L2_AVD_RESULT _mtk_avd_get_status(struct mtk_srccap_dev *srccap_dev,
	__u16 *u16Status);
static enum V4L2_AVD_RESULT _mtk_avd_set_debunce_count(struct mtk_srccap_dev *srccap_dev,
	struct stAvdCount *AvdCount);
static enum V4L2_AVD_RESULT _mtk_avd_get_notify_event(struct mtk_srccap_dev *srccap_dev,
	enum v4l2_timingdetect_status *pNotify);
static enum V4L2_AVD_RESULT _mtk_avd_set_timeout(struct mtk_srccap_dev *srccap_dev,
	struct stAvdCount *AvdCount);
static enum V4L2_AVD_RESULT _mtk_avd_set_signal_stage(struct mtk_srccap_dev *srccap_dev,
		enum V4L2_AVD_SIGNAL_STAGE enStg);
static enum V4L2_AVD_RESULT _mtk_avd_field_signal_patch(struct mtk_srccap_dev *srccap_dev,
	__u16 u16Status);
const char *InputSourceToString(
	enum v4l2_srccap_input_source eInputSource)
{
	char *src = NULL;

	switch (eInputSource) {
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		src = "Cvbs";
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		src = "Svideo";
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		src = "Atv";
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		src = "Scart";
		break;
	default:
		src = "Other";
		break;
	}

	return src;
}

const char *ScanTypeToString(u8 ScanType)
{
	char *Type = NULL;

	if (ScanType == SCAN_MODE)
		Type = "Scan";
	else if (ScanType == PLAY_MODE)
		Type = "Play";

	return Type;
}

const char *VideoStandardToString(v4l2_std_id id)
{
	v4l2_std_id myid = id;
	int i;

	for (i = 0 ; standards[i].std ; i++)
		if (myid == standards[i].std)
			break;
	return standards[i].descr;
}

const char *VdStandardToString(
			enum v4l2_ext_avd_videostandardtype eVideoStandardType)
{
	char *standard = NULL;

	switch (eVideoStandardType) {
	case V4L2_EXT_AVD_STD_PAL_BGHI:
		standard = "PAL_BGHI";
		break;
	case V4L2_EXT_AVD_STD_NTSC_M:
		standard = "NTSC_M";
		break;
	case V4L2_EXT_AVD_STD_SECAM:
		standard = "SECAM";
		break;
	case V4L2_EXT_AVD_STD_NTSC_44:
		standard = "NTSC_443";
		break;
	case V4L2_EXT_AVD_STD_PAL_M:
		standard = "PAL_M";
		break;
	case V4L2_EXT_AVD_STD_PAL_N:
		standard = "PAL_N";
		break;
	case V4L2_EXT_AVD_STD_PAL_60:
		standard = "PAL_60";
		break;
	case V4L2_EXT_AVD_STD_NOTSTANDARD:
		standard = "NOT_STANDARD";
		break;
	case V4L2_EXT_AVD_STD_AUTO:
		standard = "AUTO";
		break;
	default:
		standard = "NOT_STANDARD";
		break;
	}
	return standard;
}

const char *SignalStatusToString(
			enum v4l2_timingdetect_status enStatus)
{
	char *status = NULL;

	switch (enStatus) {
	case V4L2_TIMINGDETECT_NO_SIGNAL:
		status = "AVD_NO_SIGNAL";
		break;
	case V4L2_TIMINGDETECT_STABLE_SYNC:
		status = "AVD_STABLE_SYNC";
		break;
	case V4L2_TIMINGDETECT_UNSTABLE_SYNC:
		status = "AVD_UNSTABLE";
		break;
	default:
		status = "Out of status";
		break;
	}
	return status;
}

const char *SignalStageToString(
			int enStage)
{
	char *stage = NULL;

	switch ((enum V4L2_AVD_SIGNAL_STAGE)enStage) {
	case V4L2_SIGNAL_CHANGE_FALSE:
		stage = "V4L2_SIGNAL_CHANGE_FALSE";
		break;
	case V4L2_SIGNAL_CHANGE_TRUE:
		stage = "V4L2_SIGNAL_CHANGE_TRUE";
		break;
	case V4L2_SIGNAL_KEEP_TRUE:
		stage = "V4L2_SIGNAL_KEEP_TRUE";
		break;
	case V4L2_SIGNAL_KEEP_FALSE:
		stage = "V4L2_SIGNAL_KEEP_FALSE";
		break;
	default:
		stage = "Out of stage";
		break;
	}
	return stage;
}

static enum v4l2_ext_avd_videostandardtype _MappingV4l2stdToVideoStandard(
							v4l2_std_id std_id)
{
	enum v4l2_ext_avd_videostandardtype eVideoStandardType;

	switch (std_id) {
	case V4L2_STD_PAL:
		eVideoStandardType = V4L2_EXT_AVD_STD_PAL_BGHI;
		break;
	case V4L2_STD_NTSC:
		eVideoStandardType = V4L2_EXT_AVD_STD_NTSC_M;
		break;
	case V4L2_STD_SECAM:
		eVideoStandardType = V4L2_EXT_AVD_STD_SECAM;
		break;
	case V4L2_STD_NTSC_443:
		eVideoStandardType = V4L2_EXT_AVD_STD_NTSC_44;
		break;
	case V4L2_STD_PAL_M:
		eVideoStandardType = V4L2_EXT_AVD_STD_PAL_M;
		break;
	case V4L2_STD_PAL_N:
		eVideoStandardType = V4L2_EXT_AVD_STD_PAL_N;
		break;
	case V4L2_STD_PAL_60:
		eVideoStandardType = V4L2_EXT_AVD_STD_PAL_60;
		break;
	case V4L2_STD_UNKNOWN:
		eVideoStandardType = V4L2_EXT_AVD_STD_NOTSTANDARD;
		break;
	case V4L2_STD_ALL:
		eVideoStandardType = V4L2_EXT_AVD_STD_AUTO;
		break;
	default:
		eVideoStandardType = V4L2_EXT_AVD_STD_NOTSTANDARD;
		break;
	}
	return eVideoStandardType;
}

static v4l2_std_id _MappingVideoStandardToV4l2std(
			enum v4l2_ext_avd_videostandardtype eVideoStandardType)
{
	v4l2_std_id std_id;

	switch (eVideoStandardType) {
	case V4L2_EXT_AVD_STD_PAL_BGHI:
		std_id = V4L2_STD_PAL;
		break;
	case V4L2_EXT_AVD_STD_NTSC_M:
		std_id = V4L2_STD_NTSC;
		break;
	case V4L2_EXT_AVD_STD_SECAM:
		std_id = V4L2_STD_SECAM;
		break;
	case V4L2_EXT_AVD_STD_NTSC_44:
		std_id = V4L2_STD_NTSC_443;
		break;
	case V4L2_EXT_AVD_STD_PAL_M:
		std_id = V4L2_STD_PAL_M;
		break;
	case V4L2_EXT_AVD_STD_PAL_N:
		std_id = V4L2_STD_PAL_N;
		break;
	case V4L2_EXT_AVD_STD_PAL_60:
		std_id = V4L2_STD_PAL_60;
		break;
	case V4L2_EXT_AVD_STD_NOTSTANDARD:
		std_id = V4L2_STD_UNKNOWN;
		break;
	case V4L2_EXT_AVD_STD_AUTO:
		std_id = V4L2_STD_ALL;
		break;
	default:
		std_id = V4L2_STD_UNKNOWN;
		break;
	}
	return std_id;
}

static bool _IsVifChLock(struct mtk_srccap_dev *srccap_dev)
{
	bool bVifLock;

	bVifLock = srccap_dev->avd_info.bIsVifLock;

	if (bVifLock == TRUE)
		return TRUE;
	else
		return FALSE;
}

static bool _IsVdScanDetect(struct mtk_srccap_dev *srccap_dev)
{
	static bool bpreDetect = FALSE;
	bool bVdDetect;

	bVdDetect = srccap_dev->avd_info.bStrAtvVdDet;

	if (bpreDetect == bVdDetect)
		return FALSE;

	SRCCAP_AVD_MSG_INFO("[AVD][bpreDetect : %d][bVdDetect : %d]", bpreDetect, bVdDetect);

	bpreDetect = bVdDetect;

	if (bVdDetect == TRUE)
		return TRUE;
	else
		return FALSE;
}

static enum V4L2_AVD_RESULT _mtk_avd_get_status(struct mtk_srccap_dev *srccap_dev,
	__u16 *u16Status)
{
	struct v4l2_ext_avd_scan_hsyc_check stScanStatus;
	struct v4l2_ext_avd_standard_detection psStatus;
	bool ScanType;

	ScanType = (bool)srccap_dev->avd_info.bIsATVScanMode;

	if (ScanType == TRUE) {
		memset(&stScanStatus, 0, sizeof(struct v4l2_ext_avd_scan_hsyc_check));
		stScanStatus.u8HtotalTolerance = 20;
		mtk_avd_scan_hsync_check(&stScanStatus);
		*u16Status = stScanStatus.u16ScanHsyncCheck;
	} else if (ScanType == FALSE) {
		memset(&psStatus, 0, sizeof(struct v4l2_ext_avd_standard_detection));
		mtk_avd_standard_detetion(&psStatus);
		*u16Status = psStatus.u16LatchStatus;
	} else {
		SRCCAP_AVD_MSG_ERROR("[AVD]InValid Scan Type.\n");
		return V4L2_EXT_AVD_NOT_SUPPORT;
	}

	return V4L2_EXT_AVD_OK;
}

static enum V4L2_AVD_RESULT _mtk_avd_set_debunce_count(struct mtk_srccap_dev *srccap_dev,
	struct stAvdCount *AvdCount)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (AvdCount == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	bool ScanType;
	enum v4l2_srccap_input_source eSrc;

	ScanType = (bool)srccap_dev->avd_info.bIsATVScanMode;
	eSrc = srccap_dev->srccap_info.src;

	switch (eSrc) {
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		if (ScanType == FALSE) {
			AvdCount[PLAY_MODE].u8StableCheckCount = _AVD_CVBS_PLAY_STABLE_CHECK_COUNT;
			AvdCount[PLAY_MODE].u8PollingTime = _AVD_CVBS_PLAY_HSYNC_POLLING_TIME;
			AvdCount[PLAY_MODE].u8CheckCount = _AVD_CVBS_PLAY_CHECK_COUNT;
			AvdCount[PLAY_MODE].u8Timeout = _AVD_CVBS_PLAY_TIME_OUT;
		} else {
			SRCCAP_AVD_MSG_ERROR("[AVD]InValid Scan Type.\n");
			return V4L2_EXT_AVD_NOT_SUPPORT;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		if (ScanType == TRUE) {
			AvdCount[SCAN_MODE].u8StableCheckCount = _AVD_ATV_SCAN_STABLE_CHECK_COUNT;
			AvdCount[SCAN_MODE].u8PollingTime = _AVD_ATV_SCAN_HSYNC_POLLING_TIME;
			AvdCount[SCAN_MODE].u8CheckCount = _AVD_ATV_SCAN_CHECK_COUNT;
			AvdCount[SCAN_MODE].u8Timeout = _AVD_ATV_SCAN_TIME_OUT;
		} else if (ScanType == FALSE) {
			AvdCount[PLAY_MODE].u8StableCheckCount = _AVD_ATV_PLAY_STABLE_CHECK_COUNT;
			AvdCount[PLAY_MODE].u8PollingTime = _AVD_ATV_PLAY_HSYNC_POLLING_TIME;
			AvdCount[PLAY_MODE].u8CheckCount = _AVD_ATV_PLAY_CHECK_COUNT;
			AvdCount[PLAY_MODE].u8Timeout = _AVD_ATV_PLAY_TIME_OUT;
		} else {
			SRCCAP_AVD_MSG_ERROR("[AVD]InValid Scan Type.\n");
			return V4L2_EXT_AVD_NOT_SUPPORT;
		}
		break;
	default:
		SRCCAP_AVD_MSG_ERROR("[AVD]InValid Source Type.\n");
		return V4L2_EXT_AVD_NOT_SUPPORT;
		break;
	}
	return V4L2_EXT_AVD_OK;
}

static enum V4L2_AVD_RESULT _mtk_avd_get_notify_event(struct mtk_srccap_dev *srccap_dev,
	enum v4l2_timingdetect_status *pNotify)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (pNotify == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	v4l2_std_id system = srccap_dev->avd_info.u64DetectTvSystem;
	enum V4L2_AVD_SIGNAL_STAGE Stage = srccap_dev->avd_info.enSignalStage;
	bool ScanMode = srccap_dev->avd_info.bIsATVScanMode;
	bool TimeOut = srccap_dev->avd_info.bIsTimeOut;
	enum v4l2_srccap_input_source eSrc = srccap_dev->srccap_info.src;
	bool bIsVifLock  = srccap_dev->avd_info.bIsVifLock;

	if (ScanMode == SCAN_MODE) {
		if (system != V4L2_STD_UNKNOWN)
			*pNotify = V4L2_TIMINGDETECT_STABLE_SYNC;
		else if (system == V4L2_STD_UNKNOWN)
			*pNotify = (TimeOut) ? V4L2_TIMINGDETECT_NO_SIGNAL :
			V4L2_TIMINGDETECT_UNSTABLE_SYNC;
		else
			SRCCAP_AVD_MSG_INFO("[AVD][Input : %s][Type : %s]:InValid Case.",
				InputSourceToString(eSrc),
				ScanTypeToString(ScanMode));

		//Vd detect finish at this time.
		if ((*pNotify != V4L2_TIMINGDETECT_UNSTABLE_SYNC) &&
			(bIsVifLock == true)) {
			srccap_dev->avd_info.bIsVifLock = FALSE;
			srccap_dev->avd_info.bStrAtvVdDet = FALSE;
			SRCCAP_AVD_MSG_INFO(
				"[AVD][Input:%s][Type:%s]bIsVifLock=(%d),bStrAtvVdDet=(%d)",
			InputSourceToString(eSrc),
			ScanTypeToString(ScanMode),
			srccap_dev->avd_info.bIsVifLock,
			srccap_dev->avd_info.bStrAtvVdDet);
		}

	} else if (ScanMode == PLAY_MODE) {
		switch (Stage) {
		case V4L2_SIGNAL_CHANGE_FALSE:
			srccap_dev->avd_info.u64DetectTvSystem = V4L2_STD_UNKNOWN;
			*pNotify = V4L2_TIMINGDETECT_UNSTABLE_SYNC;
			break;
		case V4L2_SIGNAL_CHANGE_TRUE:
			if (system != V4L2_STD_UNKNOWN)
				*pNotify = V4L2_TIMINGDETECT_STABLE_SYNC;
			break;
		case V4L2_SIGNAL_KEEP_TRUE:
			*pNotify = V4L2_TIMINGDETECT_STABLE_SYNC;
			break;
		case V4L2_SIGNAL_KEEP_FALSE:
			*pNotify = V4L2_TIMINGDETECT_NO_SIGNAL;
			break;
		default:
			SRCCAP_AVD_MSG_ERROR("[AVD][Input : %s][Type : %s]:InValid Case.",
				InputSourceToString(eSrc),
				ScanTypeToString(ScanMode));
			break;
		}
	} else
		SRCCAP_AVD_MSG_ERROR("[AVD][Input : %s][Type : %s]:InValid Type.",
			InputSourceToString(eSrc),
			ScanTypeToString(ScanMode));

	return V4L2_EXT_AVD_OK;
}

static enum V4L2_AVD_RESULT _mtk_avd_set_timeout(struct mtk_srccap_dev *srccap_dev,
	struct stAvdCount *AvdCount)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	bool ScanMode = srccap_dev->avd_info.bIsATVScanMode;
	bool TimeOut = srccap_dev->avd_info.bIsTimeOut;

	if (ScanMode == SCAN_MODE) {
		if (TimeOut == FALSE)
			SRCCAP_AVD_MSG_INFO("[AVD][Type:%s]Reach detect time(%d)\n",
			ScanTypeToString(ScanMode), AvdCount[ScanMode].u8Timeout);
		srccap_dev->avd_info.bIsTimeOut = TRUE;
	}

	return V4L2_EXT_AVD_OK;
}

static enum V4L2_AVD_RESULT _mtk_avd_field_signal_patch(struct mtk_srccap_dev *srccap_dev,
	__u16 u16Status)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if ((u64TvSystem == V4L2_STD_UNKNOWN) && IsAVDLOCK(u16Status)) {
		if (IsVSYNCTYPE(u16Status))
			u64TvSystem = V4L2_STD_PAL;
		else
			u64TvSystem = V4L2_STD_NTSC;
	}

	return V4L2_EXT_AVD_OK;
}

static enum V4L2_AVD_RESULT _mtk_avd_set_signal_stage(struct mtk_srccap_dev *srccap_dev,
		enum V4L2_AVD_SIGNAL_STAGE cntStg)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	enum V4L2_AVD_SIGNAL_STAGE preStg = srccap_dev->avd_info.enSignalStage;
	enum v4l2_srccap_input_source eSrc = srccap_dev->srccap_info.src;
	bool ScanMode = srccap_dev->avd_info.bIsATVScanMode;

	srccap_dev->avd_info.enSignalStage = cntStg;

	if (preStg != cntStg)
		switch (cntStg) {
		case V4L2_SIGNAL_CHANGE_FALSE:
		case V4L2_SIGNAL_CHANGE_TRUE:
		case V4L2_SIGNAL_KEEP_TRUE:
		case V4L2_SIGNAL_KEEP_FALSE:
			SRCCAP_AVD_MSG_INFO("[AVD]%s.\n", SignalStageToString((int)cntStg));
			break;
		default:
			SRCCAP_AVD_MSG_INFO("[AVD][Input : %s][Type : %s]:InValid Stage.",
				InputSourceToString(eSrc),
				ScanTypeToString(ScanMode));
			return V4L2_EXT_AVD_NOT_SUPPORT;
		}
	return V4L2_EXT_AVD_OK;
}

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
enum V4L2_AVD_RESULT mtk_avd_SetColorSystem(v4l2_std_id std)
{
	MS_U16 colorSystem = 0;
	MS_U8 u8Value;

	SRCCAP_AVD_MSG_INFO("in\n");
	//Set color system NTSC-M enable
	if ((std & V4L2_STD_NTSC) == V4L2_STD_NTSC) {
		colorSystem |= (BIT(0));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_NTSC]\n");
	}

	//Set color system PAL enable
	if ((std & V4L2_STD_PAL) == V4L2_STD_PAL) {
		colorSystem |= (BIT(1));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_PAL]\n");
	}

	//Set color system PAL-Nc enable
	if ((std & V4L2_STD_PAL_Nc) == V4L2_STD_PAL_Nc) {
		colorSystem |= (BIT(2));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_PAL_Nc]\n");
	}

	//Set color system PAL-M enable
	if ((std & V4L2_STD_PAL_M) == V4L2_STD_PAL_M) {
		colorSystem |= (BIT(3));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_PAL_M]\n");
	}

	//Set color system SECAM enable
	if ((std & V4L2_STD_SECAM) ==  V4L2_STD_SECAM) {
		colorSystem |= (BIT(4));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_SECAM]\n");
	}

	//Set color system NTSC443 enable
	if ((std & V4L2_STD_NTSC_443) == V4L2_STD_NTSC_443) {
		colorSystem |= (BIT(5));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_NTSC_443]\n");
	}

	//Set color system PAL60 enable
	if ((std & V4L2_STD_PAL_60) == V4L2_STD_PAL_60) {
		colorSystem |= (BIT(6));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_PAL_60]\n");
	}

	//Set a color system NTSC, PAL-M, NTSC-443, PAL-60 enable
	if ((std & V4L2_STD_525_60) == V4L2_STD_525_60) {
		colorSystem |= (BIT(0) | BIT(3) | BIT(5) | BIT(6));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_525_60]\n");
	}

	//Set a color system PAL,PAL-N, SECAM enable
	if ((std & V4L2_STD_625_50) == V4L2_STD_625_50) {
		colorSystem |= (BIT(1) | BIT(2) | BIT(4));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_625_50]\n");
	}

	//Set all color system enable
	if ((std & V4L2_STD_ALL) == V4L2_STD_ALL) {
		colorSystem |=
		(BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6));
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_ALL]\n");
	}

	if (std == V4L2_STD_UNKNOWN)
		SRCCAP_AVD_MSG_INFO("[V4L2_STD_UNKNOWN]\n");

	u8Value = Api_AVD_GetStandard_Detection_Flag();

	//all disable
	u8Value |=  (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(6));

	if (colorSystem&(BIT(0)))	//Set color system NTSC-M enable
		u8Value &= ~(BIT(2));

	if (colorSystem&(BIT(1)))	//Set color system PAL enable
		u8Value &= ~(BIT(0));

	if (colorSystem&(BIT(2)))	//Set color system PAL-Nc enable
		u8Value &= ~(BIT(6));

	if (colorSystem&(BIT(3)))	//Set color system PAL-M enable
		u8Value &= ~(BIT(4));

	if (colorSystem&(BIT(4)))	//Set color system SECAM enable
		u8Value &= ~(BIT(1));

	if (colorSystem&(BIT(5)))	//Set color system NTSC443 enable
		u8Value &= ~(BIT(3));

	if (colorSystem&(BIT(6)))	//Set color system PAL60 enable
		u8Value &= ~(BIT(0));

	// Reset dsp when we change color system
	u8Value |= BIT(7);
	Api_AVD_SetStandard_Detection_Flag(u8Value);

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_standard_detetion(
		struct v4l2_ext_avd_standard_detection *pDetection)
{

#ifdef DECOMPOSE
	enum v4l2_ext_avd_videostandardtype eVideoStandardType;
	unsigned short u16LatchStatus;
#endif

	SRCCAP_AVD_MSG_DEBUG("in\n");

#ifdef DECOMPOSE
	eVideoStandardType = Api_AVD_GetStandardDetection(&u16LatchStatus);
	pDetection->u64VideoStandardType =
		_MappingVideoStandardToV4l2std(eVideoStandardType);
	pDetection->u16LatchStatus = u16LatchStatus;
#endif

#if (TEST_MODE) //For test
	pDetection->u64VideoStandardType = V4L2_STD_NTSC;
	pDetection->u16LatchStatus = 0xE443;
#endif

	SRCCAP_AVD_MSG_DEBUG("\t Videostandardtype={%s}\n",
	VideoStandardToString(pDetection->u64VideoStandardType));
	show_v4l2_ext_avd_standard_detection(pDetection);

	SRCCAP_AVD_MSG_DEBUG("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_status(unsigned short *u16vdstatus)
{
	SRCCAP_AVD_MSG_DEBUG("in\n");

#ifdef DECOMPOSE
	if (u16vdstatus == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	*u16vdstatus = Api_AVD_GetStatus();
#endif

#if (TEST_MODE) //For test
	*u16vdstatus = 0xE443;
#endif

	show_v4l2_ext_avd_status(u16vdstatus);

	SRCCAP_AVD_MSG_DEBUG("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_force_video_standard(
				v4l2_std_id *u64Videostandardtype)
{
#ifdef DECOMPOSE
	enum v4l2_ext_avd_videostandardtype eVideoStandardType;
#endif

	SRCCAP_AVD_MSG_INFO("in\n");
	SRCCAP_AVD_MSG_INFO("\t Videostandardtype={%s(0x%02x)}\n",
		VideoStandardToString(*u64Videostandardtype),
		_MappingV4l2stdToVideoStandard(*u64Videostandardtype));
	show_v4l2_ext_avd_force_video_standard(u64Videostandardtype);

#ifdef DECOMPOSE
	eVideoStandardType =
		_MappingV4l2stdToVideoStandard(*u64Videostandardtype);

	if (eVideoStandardType > V4L2_EXT_AVD_STD_AUTO)
		SRCCAP_AVD_MSG_ERROR("Out of video standard(%d).\n", eVideoStandardType);

	if (Api_AVD_ForceVideoStandard(eVideoStandardType) == 0) {
		SRCCAP_AVD_MSG_ERROR("\t Videostandardtype={0x%02x, }\n",
			_MappingV4l2stdToVideoStandard(*u64Videostandardtype));
		show_v4l2_ext_avd_force_video_standard(u64Videostandardtype);
		SRCCAP_AVD_MSG_ERROR("Fail(-1)\n");
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_video_standard(
		struct v4L2_ext_avd_video_standard *pVideoStandard)
{
#ifdef DECOMPOSE
	enum v4l2_ext_avd_videostandardtype eVideoStandardType;
#endif

	SRCCAP_AVD_MSG_INFO("in\n");

	SRCCAP_AVD_MSG_INFO("\t Videostandardtype={%s(0x%02x) }\n",
		VideoStandardToString(pVideoStandard->u64Videostandardtype),
		_MappingV4l2stdToVideoStandard(pVideoStandard->u64Videostandardtype));
	show_v4L2_ext_avd_video_standard(pVideoStandard);

#ifdef DECOMPOSE
	eVideoStandardType =
	_MappingV4l2stdToVideoStandard(pVideoStandard->u64Videostandardtype);

	if (eVideoStandardType > V4L2_EXT_AVD_STD_AUTO)
		SRCCAP_AVD_MSG_ERROR("Out of video standard(%d).\n", eVideoStandardType);

	if (Api_AVD_SetVideoStandard(eVideoStandardType,
				pVideoStandard->bIsInAutoTuning) == 0) {
		SRCCAP_AVD_MSG_ERROR("\t Videostandardtype={0x%02x, }\n",
			_MappingV4l2stdToVideoStandard(pVideoStandard->u64Videostandardtype));
		show_v4L2_ext_avd_video_standard(pVideoStandard);
		SRCCAP_AVD_MSG_ERROR("Fail(-1)\n");
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_start_auto_standard_detetion(void)
{
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	Api_AVD_StartAutoStandardDetection();
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_reg_from_dsp(void)
{
	SRCCAP_AVD_MSG_DEBUG("in\n");

#ifdef DECOMPOSE
	Api_AVD_SetRegFromDSP();
#endif

	SRCCAP_AVD_MSG_DEBUG("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_scan_hsync_check(
			struct v4l2_ext_avd_scan_hsyc_check *pHsync)
{
	SRCCAP_AVD_MSG_DEBUG("in\n");

#ifdef DECOMPOSE
	if (pHsync == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	pHsync->u16ScanHsyncCheck =
	Api_AVD_Scan_HsyncCheck(pHsync->u8HtotalTolerance);
#endif

#if (TEST_MODE) //For test
	pHsync->u16ScanHsyncCheck = 0xE443;
#endif

	show_v4l2_ext_avd_scan_hsyc_check(pHsync);

	SRCCAP_AVD_MSG_DEBUG("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_SourceSignalMonitor(enum v4l2_timingdetect_status *pAvdNotify,
			struct mtk_srccap_dev *srccap_dev)
{
	if (pAvdNotify == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (srccap_dev->srccap_info.src) {
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		return mtk_avd_CvbsTimingMonitor(pAvdNotify, srccap_dev);
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		return mtk_avd_AtvTimingMonitor(pAvdNotify, srccap_dev);
	default:
		break;
	}

	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_AtvTimingMonitor(enum v4l2_timingdetect_status *pAvdNotify,
			struct mtk_srccap_dev *srccap_dev)
{
	struct v4l2_ext_avd_standard_detection psStatus;
	struct stAvdCount _AvdCount[2];
	enum v4l2_srccap_input_source eSrc;
	enum V4L2_AVD_SIGNAL_STAGE enStg;
	enum V4L2_AVD_RESULT ret;
	static bool	bPrebLock = FALSE;
	static __u16 u16preAvdStatus;
	static __u32 u32preTime;
	__u16 u16curAvdStatus = 0;
	__u32 u32DiffTime = 0;
	bool bVifFstLock, bVdScanDet, bLock = FALSE, bForce = FALSE;
	__u8 u8CheckCnt = 0, u8AtvScan, u8StableCheckCount = 0;
	v4l2_std_id u64PreTvSystem = V4L2_STD_UNKNOWN, u64ForceVideostandard;
	struct v4L2_ext_avd_video_standard VideoStandard;

	eSrc = srccap_dev->srccap_info.src;
	u8AtvScan = srccap_dev->avd_info.bIsATVScanMode;
	bVifFstLock = srccap_dev->avd_info.bIsVifLock;

	//Set dedunce count
	_mtk_avd_set_debunce_count(srccap_dev, _AvdCount);

	//Check Vd detect flag enable or not.
	bVdScanDet = _IsVdScanDetect(srccap_dev);

	//Return if vif not ready during scan time.
	if ((u8AtvScan == TRUE) && (_IsVifChLock(srccap_dev) == FALSE)) {
		*pAvdNotify = V4L2_TIMINGDETECT_NO_SIGNAL;
		srccap_dev->avd_info.u64DetectTvSystem = V4L2_STD_UNKNOWN;
		//Avoid to notify Interval is too short.
		MsOS_DelayTask(_AvdCount[u8AtvScan].u8PollingTime);
		return V4L2_EXT_AVD_OK;
	}

	//Request Vd detect during scan time.
	if ((u8AtvScan == TRUE) && (bVdScanDet == TRUE)) {
		u32preTime = MsOS_GetSystemTime();
		srccap_dev->avd_info.bIsTimeOut = FALSE;
		SRCCAP_AVD_MSG_INFO("[AVD][Input : %s][Type : %s][Start Time : %d]:VIF Lock\n",
			InputSourceToString(eSrc),
			ScanTypeToString(u8AtvScan), u32preTime);
	    *pAvdNotify = V4L2_TIMINGDETECT_UNSTABLE_SYNC;
		return V4L2_EXT_AVD_OK;
	}

	//Get VD status
	_mtk_avd_get_status(srccap_dev, &u16curAvdStatus);

	//Check VD status
	if (IsAVDLOCK(u16curAvdStatus) && (u16curAvdStatus == u16preAvdStatus))
		bLock = TRUE;
	else
		bLock = FALSE;

	//Get Signal stage
	enStg = FIND_STAGE_BY_KEY(bPrebLock, bLock);

	//Set Signal stage
	_mtk_avd_set_signal_stage(srccap_dev, enStg);

	switch (enStg) {
	case V4L2_SIGNAL_CHANGE_FALSE:
		bPrebLock = bLock;
		SRCCAP_AVD_MSG_INFO("[AVD][preStatus = 0x%04x][Status = 0x%04x]:Lock->UnLock.\n",
			u16preAvdStatus, u16curAvdStatus);
		_mtk_avd_get_notify_event(srccap_dev, pAvdNotify);
		break;
	case V4L2_SIGNAL_CHANGE_TRUE:
		//Unlock->Lock
		SRCCAP_AVD_MSG_INFO("[AVD][Status = 0x%04x]:Unlock->Lock.\n", u16curAvdStatus);
		// Check signal stable
		u8StableCheckCount = _AvdCount[u8AtvScan].u8StableCheckCount;
		u8CheckCnt = _AvdCount[u8AtvScan].u8CheckCount;
		do {
			mtk_avd_standard_detetion(&psStatus);
			u64TvSystem = psStatus.u64VideoStandardType;
			u16curAvdStatus = psStatus.u16LatchStatus;

			if ((u64PreTvSystem == u64TvSystem) && (u64TvSystem != V4L2_STD_UNKNOWN)) {
				u8CheckCnt--;
				if (u8CheckCnt == 0) {
					SRCCAP_AVD_MSG_INFO(
						"[AVD][Input : %s][Type : %s][Time : %d][Status = 0x%04x]:Break(%d)\n",
						InputSourceToString(eSrc),
						ScanTypeToString(u8AtvScan),
						MsOS_GetSystemTime(),
						u16curAvdStatus, u8CheckCnt);
					break;
				}
			} else {
				u8CheckCnt = _AvdCount[u8AtvScan].u8CheckCount;
			}

			SRCCAP_AVD_MSG_INFO(
				"[AVD][Input : %s][Type : %s][Time : %d][Cnt = %d][Status = 0x%04x][TvSystem = 0x%04x]",
				InputSourceToString(eSrc), ScanTypeToString(u8AtvScan),
				MsOS_GetSystemTime(), u8CheckCnt, u16curAvdStatus,
				_MappingV4l2stdToVideoStandard(u64TvSystem));
			u64PreTvSystem = u64TvSystem;
			MsOS_DelayTask(_AvdCount[u8AtvScan].u8PollingTime);
			u8StableCheckCount--;

		} while (u8StableCheckCount > 0);

		_mtk_avd_field_signal_patch(srccap_dev, u16curAvdStatus);
		//Check force Tv system or not.
		mtk_avd_CheckForceSystem(srccap_dev, &bForce);

		if (bForce == TRUE) {
			mtk_avd_GetCurForceSystem(srccap_dev, &u64ForceVideostandard);
			//The result of Tv system is from Upper layer.
			srccap_dev->avd_info.u64DetectTvSystem = u64ForceVideostandard;
		} else {
			//The result of Tv system is from driver detect..
			srccap_dev->avd_info.u64DetectTvSystem = u64TvSystem;
			//Set VD Htt.
			mtk_avd_SetSamplingByHtt
			(srccap_dev, &srccap_dev->avd_info.u64DetectTvSystem);
			//Set VD standard .
			memset(&VideoStandard, 0, sizeof(struct v4L2_ext_avd_video_standard));
			VideoStandard.u64Videostandardtype = srccap_dev->avd_info.u64DetectTvSystem;
			mtk_avd_video_standard(&VideoStandard);
		}

		//Store Tv system for XC getting capture win/format.
		srccap_dev->avd_info.eStableTvsystem =
			_MappingV4l2stdToVideoStandard(srccap_dev->avd_info.u64DetectTvSystem);

		SRCCAP_AVD_MSG_INFO(
			"[AVD][Input : %s][Type : %s][Time : %d][Force:%d][Status = 0x%04x][Standard : %s]\n",
			InputSourceToString(eSrc),
			ScanTypeToString(u8AtvScan),
			MsOS_GetSystemTime(),
			bForce, u16curAvdStatus,
			VdStandardToString(srccap_dev->avd_info.eStableTvsystem));

		bPrebLock = bLock;
		_mtk_avd_get_notify_event(srccap_dev, pAvdNotify);
		break;
	case V4L2_SIGNAL_KEEP_TRUE:
		_mtk_avd_get_notify_event(srccap_dev, pAvdNotify);
		break;
	case V4L2_SIGNAL_KEEP_FALSE:
		_mtk_avd_get_notify_event(srccap_dev, pAvdNotify);
		break;
	default:
		SRCCAP_AVD_MSG_ERROR("[AVD][Input : %s][Type : %s]:InValid Stage.",
			InputSourceToString(eSrc),
			ScanTypeToString(u8AtvScan));
		break;
	}

	if (u16preAvdStatus != u16curAvdStatus)
		u16preAvdStatus = u16curAvdStatus;

	//Check time out or not.
	u32DiffTime =  MsOS_GetSystemTime() - u32preTime;
	if (u32DiffTime > _AvdCount[u8AtvScan].u8Timeout)
		_mtk_avd_set_timeout(srccap_dev, _AvdCount);

	mtk_avd_reg_from_dsp();
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_CvbsTimingMonitor(enum v4l2_timingdetect_status *pAvdNotify,
	struct mtk_srccap_dev *srccap_dev)
{
	struct v4l2_ext_avd_standard_detection psStatus;
	struct stAvdCount _AvdCount[2];
	enum v4l2_srccap_input_source eSrc;
	enum V4L2_AVD_SIGNAL_STAGE enStg;
	static bool	bPrebLock = FALSE;
	static __u16 u16preAvdStatus;
	bool bLock = FALSE, bForce = FALSE;
	__u16 u16curAvdStatus = 0;
	__u8 u8CheckCnt = 0, u8StableCheckCount = 0;
	v4l2_std_id u64PreTvSystem = V4L2_STD_UNKNOWN, u64ForceVideostandard;
	struct v4L2_ext_avd_video_standard VideoStandard;

	eSrc = srccap_dev->srccap_info.src;

	//Set dedunce count
	_mtk_avd_set_debunce_count(srccap_dev, _AvdCount);

	//Get VD status
	_mtk_avd_get_status(srccap_dev, &u16curAvdStatus);

	//Check VD status
	if (IsAVDLOCK(u16curAvdStatus) && (u16curAvdStatus == u16preAvdStatus))
		bLock = TRUE;
	else
		bLock = FALSE;

	//Get Signal stage
	enStg = FIND_STAGE_BY_KEY(bPrebLock, bLock);

	//Set Signal stage
	_mtk_avd_set_signal_stage(srccap_dev, enStg);

	switch (enStg) {
	case V4L2_SIGNAL_CHANGE_FALSE:
		 // Lock -> UnLock
		bPrebLock = bLock;
		SRCCAP_AVD_MSG_INFO("[AVD][preStatus = 0x%04x][Status = 0x%04x]:Lock->UnLock.\n",
			u16preAvdStatus, u16curAvdStatus);
		_mtk_avd_get_notify_event(srccap_dev, pAvdNotify);
		break;
	case V4L2_SIGNAL_CHANGE_TRUE:
		 //Unlock->Lock
		SRCCAP_AVD_MSG_INFO("[AVD][Status = 0x%04x]:Unlock->Lock.\n", u16curAvdStatus);

		// Check signal stable
		u8StableCheckCount = _AvdCount[PLAY_MODE].u8StableCheckCount;
		u8CheckCnt = _AvdCount[PLAY_MODE].u8CheckCount;
		do {
			mtk_avd_standard_detetion(&psStatus);
			u64TvSystem = psStatus.u64VideoStandardType;
			u16curAvdStatus = psStatus.u16LatchStatus;

			if ((u64PreTvSystem == u64TvSystem) && (u64TvSystem != V4L2_STD_UNKNOWN)) {
				u8CheckCnt--;
				if (u8CheckCnt == 0) {
					SRCCAP_AVD_MSG_INFO(
						"[AVD][Input : %s][Type : %s][Status = 0x%04x]:Break(%d)\n",
						InputSourceToString(eSrc),
						ScanTypeToString(PLAY_MODE),
						u16curAvdStatus, u8CheckCnt);
					break;
				}
			} else {
				u8CheckCnt = _AvdCount[PLAY_MODE].u8CheckCount;
			}

			SRCCAP_AVD_MSG_INFO(
				"[AVD][Input : %s][Type : %s][Cnt = %d][Status = 0x%04x][TvSystem = 0x%04x]",
				InputSourceToString(eSrc), ScanTypeToString(PLAY_MODE),
				u8CheckCnt, u16curAvdStatus,
				_MappingV4l2stdToVideoStandard(u64TvSystem));
			u64PreTvSystem = u64TvSystem;
			MsOS_DelayTask(_AvdCount[PLAY_MODE].u8PollingTime);
			u8StableCheckCount--;

		} while (u8StableCheckCount > 0);

		_mtk_avd_field_signal_patch(srccap_dev, u16curAvdStatus);
		//Check force Tv system or not.
		mtk_avd_CheckForceSystem(srccap_dev, &bForce);

		if (bForce == TRUE) {
			mtk_avd_GetCurForceSystem(srccap_dev, &u64ForceVideostandard);
			//The result of Tv system is from Upper layer.
			srccap_dev->avd_info.u64DetectTvSystem = u64ForceVideostandard;
		} else {
			//The result of Tv system is from driver detect.
			srccap_dev->avd_info.u64DetectTvSystem = u64TvSystem;
			//Set VD Htt.
			mtk_avd_SetSamplingByHtt
			(srccap_dev, &srccap_dev->avd_info.u64DetectTvSystem);
			//Set VD standard .
			memset(&VideoStandard, 0, sizeof(struct v4L2_ext_avd_video_standard));
			VideoStandard.u64Videostandardtype = srccap_dev->avd_info.u64DetectTvSystem;
			mtk_avd_video_standard(&VideoStandard);
		}

		//Store Tv system for XC getting capture win/format.
		srccap_dev->avd_info.eStableTvsystem =
			_MappingV4l2stdToVideoStandard(srccap_dev->avd_info.u64DetectTvSystem);

		SRCCAP_AVD_MSG_INFO(
			"[AVD][Input : %s][Type : %s][Force:%d][Status = 0x%04x][Standard : %s]\n",
			InputSourceToString(eSrc),
			ScanTypeToString(PLAY_MODE), bForce,
			u16curAvdStatus,
			VdStandardToString(srccap_dev->avd_info.eStableTvsystem));

		_mtk_avd_get_notify_event(srccap_dev, pAvdNotify);
		bPrebLock = bLock;
		break;
	case V4L2_SIGNAL_KEEP_TRUE:
		_mtk_avd_get_notify_event(srccap_dev, pAvdNotify);
		break;
	case V4L2_SIGNAL_KEEP_FALSE:
		_mtk_avd_get_notify_event(srccap_dev, pAvdNotify);
		break;
	default:
		SRCCAP_AVD_MSG_ERROR("[AVD][Input : %s][Type : %s]:InValid Stage.",
			InputSourceToString(eSrc),
			ScanTypeToString(PLAY_MODE));
		break;
	}

	if (u16preAvdStatus != u16curAvdStatus)
		u16preAvdStatus = u16curAvdStatus;

	mtk_avd_reg_from_dsp();
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_SetSamplingByHtt(struct mtk_srccap_dev *srccap_dev,
		v4l2_std_id *u64Videostandardtype)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (u64Videostandardtype == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret;
	__u32 u32VDPatchFlag;
	__u16 u16Htt, vdsampling_entry_cnt = 0, vdsampling_index = 0;
	struct srccap_avd_vd_sampling *vdsapmling_table;
	enum v4l2_ext_avd_videostandardtype videotype = V4L2_EXT_AVD_STD_NOTSTANDARD;

	// get vd sampling table
	vdsampling_entry_cnt = srccap_dev->avd_info.vdsampling_table_entry_count;
	vdsapmling_table = srccap_dev->avd_info.vdsampling_table;
	videotype = _MappingV4l2stdToVideoStandard(*u64Videostandardtype);

	ret = mtk_avd_match_vd_sampling
		(videotype, vdsapmling_table, vdsampling_entry_cnt, &vdsampling_index);

	if (ret == -ERANGE) {
		SRCCAP_MSG_ERROR("vd sampling not found\n");
		return ret;
	}

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	SRCCAP_AVD_MSG_INFO
		("[vst:0x%4X][hst:0x%4X][hde:0x%4X][vde:0x%4X][htt:0x%4X][vfreqx1000:%d]\n",
		vdsapmling_table[vdsampling_index].v_start,
		vdsapmling_table[vdsampling_index].h_start,
		vdsapmling_table[vdsampling_index].h_de,
		vdsapmling_table[vdsampling_index].v_de,
		vdsapmling_table[vdsampling_index].h_total,
		vdsapmling_table[vdsampling_index].v_freqx1000);

	memset(&u16Htt, 0, sizeof(__u16));
	u16Htt = vdsapmling_table[vdsampling_index].h_total;
	ret = mtk_avd_htt_user_md(&u16Htt);
	if (ret != V4L2_EXT_AVD_OK)
		return V4L2_EXT_AVD_FAIL;

	memset(&u32VDPatchFlag, 0, sizeof(__u32));
	ret = mtk_avd_g_flag(&u32VDPatchFlag);
	if (ret != V4L2_EXT_AVD_OK)
		return V4L2_EXT_AVD_FAIL;

	u32VDPatchFlag = ((u32VDPatchFlag & ~(AVD_PATCH_HTOTAL_MASK)) | AVD_PATCH_HTOTAL_USER);
	ret = mtk_avd_s_flag(&u32VDPatchFlag);
	if (ret != V4L2_EXT_AVD_OK)
		return V4L2_EXT_AVD_FAIL;

	SRCCAP_AVD_MSG_INFO("[ForceTvSystem = %s][PatchFlag = 0x%08x],[Htt = 0x%04x]\n",
		VideoStandardToString(*u64Videostandardtype),
		u32VDPatchFlag, u16Htt);

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_SetCurForceSystem(struct mtk_srccap_dev *srccap_dev,
	v4l2_std_id *u64Videostandardtype)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (u64Videostandardtype == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	enum v4l2_srccap_input_source eSrc;
	__u8 u8AtvScan;

	eSrc = srccap_dev->srccap_info.src;
	u8AtvScan = srccap_dev->avd_info.bIsATVScanMode;

	srccap_dev->avd_info.u64ForceTvSystem = *u64Videostandardtype;

	SRCCAP_AVD_MSG_INFO(
		"[AVD][Input : %s][Type : %s][Time : %d][Force Standard : %s(%d)]\n",
		InputSourceToString(eSrc), ScanTypeToString(u8AtvScan),
		MsOS_GetSystemTime(),
		VideoStandardToString(srccap_dev->avd_info.u64ForceTvSystem),
		_MappingV4l2stdToVideoStandard(srccap_dev->avd_info.u64ForceTvSystem));

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_GetCurForceSystem(struct mtk_srccap_dev *srccap_dev,
	v4l2_std_id *u64Videostandardtype)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (u64Videostandardtype == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	*u64Videostandardtype = srccap_dev->avd_info.u64ForceTvSystem;

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_CheckForceSystem(struct mtk_srccap_dev *srccap_dev,
	bool *bForce)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (bForce == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	enum v4l2_srccap_input_source eSrc;
	bool u8AtvScan;
	v4l2_std_id u64ForceVideostandard = V4L2_STD_UNKNOWN;

	eSrc = srccap_dev->srccap_info.src;
	u8AtvScan = srccap_dev->avd_info.bIsATVScanMode;
	mtk_avd_GetCurForceSystem(srccap_dev, &u64ForceVideostandard);

	if ((u8AtvScan == FALSE) && (u64ForceVideostandard != V4L2_STD_UNKNOWN))
		*bForce = TRUE;
	else
		*bForce = FALSE;

	srccap_dev->avd_info.bForce = *bForce;
	SRCCAP_AVD_MSG_INFO(
		"[AVD][Input : %s][Type : %s][Time : %d][Force Eanble : %d][Force Standard : %s]\n",
		InputSourceToString(eSrc), ScanTypeToString(u8AtvScan),	MsOS_GetSystemTime(),
		*bForce, VideoStandardToString(u64ForceVideostandard));

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_match_vd_sampling(enum v4l2_ext_avd_videostandardtype videotype,
	struct srccap_avd_vd_sampling *vdsampling_table,
	u16 table_entry_cnt,
	u16 *vdsampling_table_index)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	if (vdsampling_table == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (vdsampling_table_index == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	SRCCAP_AVD_MSG_INFO("%s:%u, %s:%d\n",
		"video type", videotype,
		"vdtable_entry_count", table_entry_cnt);

	for (*vdsampling_table_index = 0; (*vdsampling_table_index) < table_entry_cnt;
		(*vdsampling_table_index)++) {
		SRCCAP_AVD_MSG_INFO("[%u], %s:%d\n",
			*vdsampling_table_index,
			"status", vdsampling_table[*vdsampling_table_index].videotype);

		if (videotype == vdsampling_table[*vdsampling_table_index].videotype)
			break;
	}

	if (*vdsampling_table_index == table_entry_cnt) {
		SRCCAP_MSG_ERROR("video type not supported\n");
		return -ERANGE;
	}

	SRCCAP_AVD_MSG_INFO("out\n");
	return 0;
}
