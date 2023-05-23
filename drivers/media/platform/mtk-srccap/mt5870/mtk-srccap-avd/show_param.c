// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/mtk-v4l2-srccap.h>
#include "avd-ex.h"

extern unsigned int dbglevel;

void show_v4l2_ext_avd_hsync_edge(unsigned char *u8HsyncEdge)
{
	AVD_PAM_MSG("\t u8HsyncEdge={0x%02x, }\n", *u8HsyncEdge);
}

void show_v4l2_ext_avd_g_flag(unsigned int *u32AVDFlag)
{
	AVD_PAM_MSG("\t u32AVDFlag={0x%08x, }\n", *u32AVDFlag);
}

void show_v4l2_ext_avd_noise_mag(unsigned char *u8NoiseMag)
{
	AVD_PAM_MSG("\t u8NoiseMag={0x%02x, }\n", *u8NoiseMag);
}

void show_v4l2_ext_avd_is_synclocked(unsigned char *u8lock)
{
	AVD_PAM_MSG("\t u8lock={0x%02x, }\n", *u8lock);
}

void show_v4l2_ext_avd_status(unsigned short *u16vdstatus)
{
	AVD_PAM_MSG("\t u8lock={0x%04x, }\n", *u16vdstatus);
}

void show_v4l2_ext_avd_timing(struct v4l2_timing *timing)
{
	AVD_PAM_MSG("\t timing={.h_total=%d, .v_total=%d,\n"
			".h_freq=%d, .v_freq=%d, }\n",
				timing->h_total,
				timing->v_total,
				timing->h_freq,
				timing->v_freq
				);
}
void show_v4l2_ext_avd_standard_detection(
			struct v4l2_ext_avd_standard_detection *detection)
{
	AVD_PAM_MSG("\t detection={.u16LatchStatus=0x%04x,\n"
			".u64VideoStandardType=0x%016llx, }\n",
				  detection->u16LatchStatus,
				  detection->u64VideoStandardType
				);
}

void show_v4l2_ext_avd_scan_hsyc_check(
			struct v4l2_ext_avd_scan_hsyc_check *hsync)
{
	AVD_PAM_MSG("\t hsync={.u8HtotalTolerance=%d,\n"
			".u16ScanHsyncCheck=0x%04x, }\n",
				  hsync->u8HtotalTolerance,
				  hsync->u16ScanHsyncCheck
				);
}

void show_v4l2_ext_avd_info(struct v4l2_ext_avd_info *info)
{
	AVD_PAM_MSG("\t info={\n"
			"\t .einputsourcetype=%d,\n"
			"\t .u64VideoSystem=0x%016llx,\n"
			"\t .u64LastStandard=0x%016llx,\n"
			"\t .u8AutoDetMode=%d,\n"
			"\t .u16CurVDStatus=0x%04x,\n"
			"\t .u8AutoTuningIsProgress=%d, }\n"
			"\t\n",
			info->einputsourcetype,
			info->u64VideoSystem,
			info->u64LastStandard,
			info->u8AutoDetMode,
			info->u16CurVDStatus,
			info->u8AutoTuningIsProgress
			);
}

void show_v4l2_ext_avd_sync_sensitivity(
			struct v4l2_ext_avd_sync_sensitivity *sensitivity)
{
	AVD_PAM_MSG("\t sensitivity={\n"
				"\t .u8DetectWinBeforeLock=0x%02x,\n"
				"\t .u8DetectWinAfterLock=0x%02x,\n"
				"\t .u8CNTRFailBeforeLock=0x%02x,\n"
				"\t .u8CNTRSyncBeforeLock=0x%02x,\n"
				"\t .u8CNTRSyncAfterLock=0x%02x, }\n"
				"\t\n",
				sensitivity->u8DetectWinBeforeLock,
				sensitivity->u8DetectWinAfterLock,
				sensitivity->u8CNTRFailBeforeLock,
				sensitivity->u8CNTRSyncBeforeLock,
				sensitivity->u8CNTRSyncAfterLock
				);
}

void show_v4l2_ext_avd_init_para(struct v4l2_ext_avd_init_data *init)
{
	AVD_PAM_MSG("\t init={\n"
			"\t .eLoadCodeType=%d,\n"
			"\t .eDemodType=%d,\n"
			"\t .u16VDDSPBinID=0x%04x,\n"
			"\t .bRFGainSel=%d,\n"
			"\t .bAVGainSel=%d,\n"
			"\t .u8RFGain=0x%02x,\n"
			"\t .u8AVGain=0x%02x,\n"
			"\t .u32VDPatchFlag=0x%08x,\n"
			"\t .u8ColorKillHighBound=0x%02x,\n"
			"\t .u8ColorKillLowBound=0x%02x,\n"
			"\t .u8SwingThreshold=0x%02x,\n"
			"\t .eVDHsyncSensitivityNormal =\n"
			"{0x%02x, 0x%02x, 0x%02x, 0x%02x ,0x%02x},\n"
			"\t .eVDHsyncSensitivityTuning =\n"
			"{0x%02x, 0x%02x, 0x%02x, 0x%02x ,0x%02x},\n"
			"\t .u32COMB_3D_ADR=0x%08llx,\n"
			"\t .u32COMB_3D_LEN=0x%08x,\n"
			"\t .u32CMA_HEAP_ID=%d,\n"
			"}\n",
			init->eLoadCodeType,
			init->eDemodType,
			init->u16VDDSPBinID,
			init->bRFGainSel,
			init->bAVGainSel,
			init->u8RFGain,
			init->u8AVGain,
			init->u32VDPatchFlag,
			init->u8ColorKillHighBound,
			init->u8ColorKillLowBound,
			init->u8SwingThreshold,
			init->eVDHsyncSensitivityNormal.u8DetectWinBeforeLock,
			init->eVDHsyncSensitivityNormal.u8DetectWinAfterLock,
			init->eVDHsyncSensitivityNormal.u8CNTRFailBeforeLock,
			init->eVDHsyncSensitivityNormal.u8CNTRSyncBeforeLock,
			init->eVDHsyncSensitivityNormal.u8CNTRSyncAfterLock,
			init->eVDHsyncSensitivityTuning.u8DetectWinBeforeLock,
			init->eVDHsyncSensitivityTuning.u8DetectWinAfterLock,
			init->eVDHsyncSensitivityTuning.u8CNTRFailBeforeLock,
			init->eVDHsyncSensitivityTuning.u8CNTRSyncBeforeLock,
			init->eVDHsyncSensitivityTuning.u8CNTRSyncAfterLock,
			init->u32COMB_3D_ADR,
			init->u32COMB_3D_LEN,
			init->u32CMA_HEAP_ID
			);
}

void show_v4l2_ext_avd_input(unsigned char *u8ScartFB)
{
	AVD_PAM_MSG("\t u8ScartFB={0x%02x, }\n", *u8ScartFB);
}

void show_v4l2_ext_avd_s_flag(unsigned int *u32VDPatchFlag)
{
	AVD_PAM_MSG("\t u32VDPatchFlag={0x%08x, }\n", *u32VDPatchFlag);
}

void show_v4l2_ext_avd_force_video_standard(v4l2_std_id *u64Videostandardtype)
{
	AVD_PAM_MSG("\t u64Videostandardtype={0x%016llx, }\n",
						*u64Videostandardtype);
}

void show_v4L2_ext_avd_video_standard(
			struct v4L2_ext_avd_video_standard *standard)
{
	AVD_PAM_MSG("\t standard={\n"
			".u64Videostandardtype=0x%016llx,\n"
			".bIsInAutoTuning=%d,\n"
			"}\n",
			standard->u64Videostandardtype,
			standard->bIsInAutoTuning
			);
}
void show_v4L2_ext_avd_freerun_freq(
			enum v4l2_ext_avd_freerun_freq *pFreerunFreq)
{
	AVD_PAM_MSG("\t pFreerunFreq={0x%02x, }\n", *pFreerunFreq);
}

void show_v4l2_ext_avd_still_image_param(
				struct v4l2_ext_avd_still_image_param *image)
{
	AVD_PAM_MSG("\t image={\n"
			"\t .u8Threshold1=0x%02x,\n"
			"\t .u8Threshold2=0x%02x,\n"
			"\t .u8Threshold3=0x%02x,\n"
			"\t .u8Threshold4=0x%02x,\n"
			"\t .u8Str1_COMB37=0x%02x,\n"
			"\t .u8Str1_COMB38=0x%02x,\n"
			"\t .u8Str1_COMB7C=0x%02x,\n"
			"\t .u8Str1_COMBED=0x%02x,\n"
			"\t .u8Str2_COMB37=0x%02x,\n"
			"\t .u8Str2_COMB38=0x%02x,\n"
			"\t .u8Str2_COMB7C=0x%02x,\n"
			"\t .u8Str2_COMBED=0x%02x,\n"
			"\t .u8Str3_COMB37=0x%02x,\n"
			"\t .u8Str3_COMB38=0x%02x,\n"
			"\t .u8Str3_COMB7C=0x%02x,\n"
			"\t .u8Str3_COMBED=0x%02x,\n"
			"\t .bMessageOn=%x,\n"
			"}\n",
			image->u8Threshold1,
			image->u8Threshold2,
			image->u8Threshold3,
			image->u8Threshold4,
			image->u8Str1_COMB37,
			image->u8Str1_COMB38,
			image->u8Str1_COMB7C,
			image->u8Str1_COMBED,
			image->u8Str2_COMB37,
			image->u8Str2_COMB38,
			image->u8Str2_COMB7C,
			image->u8Str2_COMBED,
			image->u8Str3_COMB37,
			image->u8Str3_COMB38,
			image->u8Str3_COMB7C,
			image->u8Str3_COMBED,
			image->bMessageOn
			);
}

void show_v4l2_ext_avd_factory_data(
			struct v4l2_ext_avd_factory_data *factory)
{
	AVD_PAM_MSG("\t factory={\n"
			".eFactoryPara=%d,\n"
			".u8Value=0x%02x,\n"
			" }\n",
			factory->eFactoryPara,
			factory->u8Value
			);
}
void show_v4l2_ext_avd_3d_comb(bool *bCombEanble)
{
	AVD_PAM_MSG("\t bCombEanble={%d, }\n", *bCombEanble);
}

void show_v4l2_ext_avd_htt_user_md(unsigned short *u16Htt)
{
	AVD_PAM_MSG("\t u16Htt={0x%04x, }\n", *u16Htt);
}

void show_v4l2_ext_avd_hsync_detect_detetion_for_tuning(bool *bTuningEanble)
{
	AVD_PAM_MSG("\t bTuningEanble={%d, }\n", *bTuningEanble);
}

void show_v4l2_ext_avd_3d_comb_speed(
			struct v4l2_ext_avd_3d_comb_speed *speed)
{
	AVD_PAM_MSG("\t speed={\n"
			".u8COMB57=0x%02x,\n"
			".u8COMB58=0x%02x,\n"
			"}\n",
			speed->u8COMB57,
			speed->u8COMB58
			);
}

void show_v4l2_ext_avd_is_lock_audio_carrier(bool *bLock)
{
	AVD_PAM_MSG("\t bLock={%d, }\n", *bLock);
}

void show_v4l2_ext_avd_alive_check(bool *bAlive)
{
	AVD_PAM_MSG("\t bAlive={%d, }\n", *bAlive);
}

