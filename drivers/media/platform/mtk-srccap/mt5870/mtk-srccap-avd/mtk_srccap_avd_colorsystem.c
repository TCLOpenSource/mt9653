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
static const char *VideoStandardToString(v4l2_std_id id);
static v4l2_std_id _MappingVideoStandardToV4l2std(
		enum v4l2_ext_avd_videostandardtype eVideoStandardType);
static enum v4l2_ext_avd_videostandardtype _MappingV4l2stdToVideoStandard(
			v4l2_std_id std_id);

static const char *VideoStandardToString(v4l2_std_id id)
{
	v4l2_std_id myid = id;
	int i;

	for (i = 0 ; standards[i].std ; i++)
		if (myid == standards[i].std)
			break;
	return standards[i].descr;
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
/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
enum V4L2_AVD_RESULT mtk_avd_SetColorSystem(v4l2_std_id std)
{
	MS_U16 colorSystem = 0;
	MS_U8 u8Value;

	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);
	//Set color system NTSC-M enable
	if ((std & V4L2_STD_NTSC) == V4L2_STD_NTSC) {
		colorSystem |= (BIT(0));
		AVD_PAM_MSG("[V4L2_STD_NTSC]\n");
	}

	//Set color system PAL enable
	if ((std & V4L2_STD_PAL) == V4L2_STD_PAL) {
		colorSystem |= (BIT(1));
		AVD_PAM_MSG("[V4L2_STD_PAL]\n");
	}

	//Set color system PAL-Nc enable
	if ((std & V4L2_STD_PAL_Nc) == V4L2_STD_PAL_Nc) {
		colorSystem |= (BIT(2));
		AVD_PAM_MSG("[V4L2_STD_PAL_Nc]\n");
	}

	//Set color system PAL-M enable
	if ((std & V4L2_STD_PAL_M) == V4L2_STD_PAL_M) {
		colorSystem |= (BIT(3));
		AVD_PAM_MSG("[V4L2_STD_PAL_M]\n");
	}

	//Set color system SECAM enable
	if ((std & V4L2_STD_SECAM) ==  V4L2_STD_SECAM) {
		colorSystem |= (BIT(4));
		AVD_PAM_MSG("[V4L2_STD_SECAM]\n");
	}

	//Set color system NTSC443 enable
	if ((std & V4L2_STD_NTSC_443) == V4L2_STD_NTSC_443) {
		colorSystem |= (BIT(5));
		AVD_PAM_MSG("[V4L2_STD_NTSC_443]\n");
	}

	//Set color system PAL60 enable
	if ((std & V4L2_STD_PAL_60) == V4L2_STD_PAL_60) {
		colorSystem |= (BIT(6));
		AVD_PAM_MSG("[V4L2_STD_PAL_60]\n");
	}

	//Set a color system NTSC, PAL-M, NTSC-443, PAL-60 enable
	if ((std & V4L2_STD_525_60) == V4L2_STD_525_60) {
		colorSystem |= (BIT(0) | BIT(3) | BIT(5) | BIT(6));
		AVD_PAM_MSG("[V4L2_STD_525_60]\n");
	}

	//Set a color system PAL,PAL-N, SECAM enable
	if ((std & V4L2_STD_625_50) == V4L2_STD_625_50) {
		colorSystem |= (BIT(1) | BIT(2) | BIT(4));
		AVD_PAM_MSG("[V4L2_STD_625_50]\n");
	}

	//Set all color system enable
	if ((std & V4L2_STD_ALL) == V4L2_STD_ALL) {
		colorSystem |=
		(BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6));
		AVD_PAM_MSG("[V4L2_STD_ALL]\n");
	}

	if (std == V4L2_STD_UNKNOWN)
		AVD_PAM_MSG("[V4L2_STD_UNKNOWN]\n");

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

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_standard_detetion(
		struct v4l2_ext_avd_standard_detection *pDetection)
{

#ifdef DECOMPOSE
	enum v4l2_ext_avd_videostandardtype eVideoStandardType;
	unsigned short u16LatchStatus;
#endif

	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

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

	AVD_PAM_MSG("\t Videostandardtype={%s}\n",
	VideoStandardToString(pDetection->u64VideoStandardType));
	show_v4l2_ext_avd_standard_detection(pDetection);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_status(unsigned short *u16vdstatus)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	*u16vdstatus = Api_AVD_GetStatus();
#endif

#if (TEST_MODE) //For test
	*u16vdstatus = 0xE443;
#endif

	show_v4l2_ext_avd_status(u16vdstatus);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_force_video_standard(
				v4l2_std_id *u64Videostandardtype)
{
#ifdef DECOMPOSE
	enum v4l2_ext_avd_videostandardtype eVideoStandardType;
#endif

	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);
	AVD_PAM_MSG("\t Videostandardtype={0x%02x, }\n",
		_MappingV4l2stdToVideoStandard(*u64Videostandardtype));
	show_v4l2_ext_avd_force_video_standard(u64Videostandardtype);

#ifdef DECOMPOSE
	eVideoStandardType =
		_MappingV4l2stdToVideoStandard(*u64Videostandardtype);
	if (Api_AVD_ForceVideoStandard(eVideoStandardType) == 0) {
		AVD_ERR_MSG("[%s][%d]Fail\n", __func__, __LINE__);
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_video_standard(
		struct v4L2_ext_avd_video_standard *pVideoStandard)
{
#ifdef DECOMPOSE
	enum v4l2_ext_avd_videostandardtype eVideoStandardType;
#endif

	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	AVD_PAM_MSG("\t Videostandardtype={0x%02x, }\n",
	_MappingV4l2stdToVideoStandard(pVideoStandard->u64Videostandardtype));
	show_v4L2_ext_avd_video_standard(pVideoStandard);

#ifdef DECOMPOSE
	eVideoStandardType =
	_MappingV4l2stdToVideoStandard(pVideoStandard->u64Videostandardtype);
	if (Api_AVD_SetVideoStandard(eVideoStandardType,
				pVideoStandard->bIsInAutoTuning) == 0) {
		AVD_ERR_MSG("[%s][%d]Fail\n", __func__, __LINE__);
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_start_auto_standard_detetion(void)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	Api_AVD_StartAutoStandardDetection();
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_reg_from_dsp(void)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	Api_AVD_SetRegFromDSP();
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_TimingMonitor(struct v4l2_srccap_pcm *pstPCM,
			struct mtk_srccap_dev *srccap_dev)
{
	struct v4l2_ext_avd_standard_detection psStatus;
	static bool	bPrebLock = FALSE;
	bool bLock = FALSE;
	__u32 u32TestStableCnt = 0;
	__u8 u8CheckCnt = 0;
	v4l2_std_id u64PreTvSystem = V4L2_STD_UNKNOWN;

	memset(&psStatus, 0, sizeof(struct v4l2_ext_avd_standard_detection));
	mtk_avd_standard_detetion(&psStatus);

	if ((psStatus.u16LatchStatus & AVD_STATUS_LOCK) == AVD_STATUS_LOCK)
		bLock = TRUE;
	else
		bLock = FALSE;

    // Lock -> UnLock
	if ((bLock == FALSE) && (bPrebLock != bLock)) {
		printf("[AVD][%s:%d]:Lock->UnLock\n", __func__, __LINE__);
		u64TvSystem = V4L2_STD_UNKNOWN;
		pstPCM->enPCMStatus = V4L2_SRCCAP_PCM_STABLE_NOSYNC;
		srccap_dev->avd_info.u64DetectTvSystem = u64TvSystem;
	} else if ((bLock == TRUE) && (bPrebLock != bLock)) {
    //Unlock->Lock
		printf("[AVD][%s:%d]:Unlock->Lock\n", __func__, __LINE__);
	    pstPCM->enPCMStatus = V4L2_SRCCAP_PCM_UNSTABLE;

		// Check signal stable
		u32TestStableCnt = _AVD_STABLE_CHECK_COUNT;
		u8CheckCnt = _AVD_CHECK_COUNT;
		do {
			mtk_avd_standard_detetion(&psStatus);
			u64TvSystem = psStatus.u64VideoStandardType;

			if ((u64PreTvSystem == u64TvSystem) && (u64TvSystem != V4L2_STD_UNKNOWN)) {
				u8CheckCnt--;
				if (u8CheckCnt == 0) {
					printf("[AVD][%s:%d]Break(%d)\n", __func__, __LINE__,
					u8CheckCnt);
					break;
				}
			} else {
				u8CheckCnt = _AVD_CHECK_COUNT;
			}

			printf("[AVD][%s:%d][Cnt = %d][Status = 0x%04x][TvSystem = 0x%04x]",
				__func__, __LINE__, u8CheckCnt, psStatus.u16LatchStatus,
				_MappingV4l2stdToVideoStandard(u64TvSystem));
			u64PreTvSystem = u64TvSystem;
			MsOS_DelayTask(_AVD_HSYNC_POLLING_TIME);
			u32TestStableCnt--;

		} while (u32TestStableCnt > 0);

		if (u64TvSystem == V4L2_STD_UNKNOWN) {
			if ((psStatus.u16LatchStatus & AVD_STATUS_LOCK) == AVD_STATUS_LOCK) {
				if ((psStatus.u16LatchStatus & _AVD_VSYNC_TYPE_PAL) ==
					(_AVD_VSYNC_TYPE_PAL)) {
					u64TvSystem = V4L2_STD_PAL;
				} else {
					u64TvSystem = V4L2_STD_NTSC;
				}
			}
		}
		pstPCM->enPCMStatus = V4L2_SRCCAP_PCM_STABLE_SYNC;
		srccap_dev->avd_info.u64DetectTvSystem = u64TvSystem;
		printf("[AVD][%s:%d][Vidoe Standard  : %s]\n", __func__, __LINE__,
			VideoStandardToString(u64TvSystem));
	}

	if (bPrebLock != bLock)
		bPrebLock = bLock;
	else if ((bPrebLock == bLock) && (bLock == TRUE))
		pstPCM->enPCMStatus = V4L2_SRCCAP_PCM_STABLE_SYNC;
	else if ((bPrebLock == bLock) && (bLock == FALSE))
		pstPCM->enPCMStatus = V4L2_SRCCAP_PCM_STABLE_NOSYNC;

	mtk_avd_reg_from_dsp();
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_TimingVideoInfo(
	struct v4l2_timing *stVideoInfo,
	struct mtk_srccap_dev *srccap_dev)
{
	printf("[AVD][%s:%d][Vidoe Standard  : %s]\n", __func__, __LINE__,
		VideoStandardToString(srccap_dev->avd_info.u64DetectTvSystem));

	switch (srccap_dev->avd_info.u64DetectTvSystem) {
	case V4L2_STD_NTSC:
		stVideoInfo->interlance = TRUE;
		stVideoInfo->de_width  = 720;
		stVideoInfo->de_height = 480;
		stVideoInfo->v_freq = 60000;
		break;
	case V4L2_STD_NTSC_443:
		stVideoInfo->interlance = TRUE;
		stVideoInfo->de_width  = 720;
		stVideoInfo->de_height = 480;
		stVideoInfo->v_freq = 60000;
		break;
	case V4L2_STD_PAL_M:
		stVideoInfo->interlance = TRUE;
		stVideoInfo->de_width  = 720;
		stVideoInfo->de_height = 480;
		stVideoInfo->v_freq = 60000;
		break;
	case V4L2_STD_PAL_N:
		stVideoInfo->interlance = TRUE;
		stVideoInfo->de_width  = 720;
		stVideoInfo->de_height = 576;
		stVideoInfo->v_freq = 50000;
		break;
	case V4L2_STD_PAL:
		stVideoInfo->interlance = TRUE;
		stVideoInfo->de_width  = 720;
		stVideoInfo->de_height = 576;
		stVideoInfo->v_freq = 50000;
		break;
	case V4L2_STD_PAL_60:
		stVideoInfo->interlance = TRUE;
		stVideoInfo->de_width  = 720;
		stVideoInfo->de_height = 480;
		stVideoInfo->v_freq = 60000;
		break;
	case V4L2_STD_SECAM:
		stVideoInfo->interlance = TRUE;
		stVideoInfo->de_width  = 720;
		stVideoInfo->de_height = 576;
		stVideoInfo->v_freq = 50000;
		break;
	default:
		stVideoInfo->interlance = FALSE;
		stVideoInfo->de_width  = 0;
		stVideoInfo->de_height = 0;
		stVideoInfo->v_freq = 0;
		break;
	}

	srccap_dev->avd_info.stVDtiming.interlance = stVideoInfo->interlance;
	srccap_dev->avd_info.stVDtiming.de_width = stVideoInfo->de_width;
	srccap_dev->avd_info.stVDtiming.de_height = stVideoInfo->de_height;
	srccap_dev->avd_info.stVDtiming.v_freq = stVideoInfo->v_freq;

	return V4L2_EXT_AVD_OK;
}
