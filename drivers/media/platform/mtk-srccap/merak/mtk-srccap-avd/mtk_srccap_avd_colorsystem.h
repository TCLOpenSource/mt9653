/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_AVD_COLORSYSTEM_H
#define MTK_SRCCAP_AVD_COLORSYSTEM_H

#include <linux/videodev2.h>
#include <linux/types.h>
#include "mtk_srccap.h"
/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
const char *InputSourceToString(
	enum v4l2_srccap_input_source eInputSource);
const char *VideoStandardToString(v4l2_std_id id);
const char *ScanTypeToString(u8 ScanType);
const char *VdStandardToString(
	enum v4l2_ext_avd_videostandardtype eVideoStandardType);
const char *SignalStatusToString(
	enum v4l2_timingdetect_status enStatus);
const char *SignalStageToString(
	int enStage);
enum V4L2_AVD_RESULT mtk_avd_SetColorSystem(v4l2_std_id std);
enum V4L2_AVD_RESULT mtk_avd_standard_detetion(
		struct v4l2_ext_avd_standard_detection *pDetection);
enum V4L2_AVD_RESULT mtk_avd_status(unsigned short *u16vdstatus);
enum V4L2_AVD_RESULT mtk_avd_force_video_standard(
			v4l2_std_id *u64Videostandardtype);
enum V4L2_AVD_RESULT mtk_avd_video_standard(
			struct v4L2_ext_avd_video_standard *pVideoStandard);
enum V4L2_AVD_RESULT mtk_avd_start_auto_standard_detetion(void);
enum V4L2_AVD_RESULT mtk_avd_scan_hsync_check(
		struct v4l2_ext_avd_scan_hsyc_check *pHsync);
enum V4L2_AVD_RESULT mtk_avd_reg_from_dsp(void);
enum V4L2_AVD_RESULT mtk_avd_SourceSignalMonitor(enum v4l2_timingdetect_status *pAvdNotify,
			struct mtk_srccap_dev *srccap_dev);
enum V4L2_AVD_RESULT mtk_avd_AtvTimingMonitor(enum v4l2_timingdetect_status *pAvdNotify,
	struct mtk_srccap_dev *srccap_dev);
enum V4L2_AVD_RESULT mtk_avd_CvbsTimingMonitor(enum v4l2_timingdetect_status *pAvdNotify,
	struct mtk_srccap_dev *srccap_dev);
enum V4L2_AVD_RESULT mtk_avd_SetSamplingByHtt(struct mtk_srccap_dev *srccap_dev,
	v4l2_std_id *u64Videostandardtype);
enum V4L2_AVD_RESULT mtk_avd_SetCurForceSystem(struct mtk_srccap_dev *srccap_dev,
	v4l2_std_id *u64Videostandardtype);
enum V4L2_AVD_RESULT mtk_avd_GetCurForceSystem(struct mtk_srccap_dev *srccap_dev,
	v4l2_std_id *u64Videostandardtype);
enum V4L2_AVD_RESULT mtk_avd_CheckForceSystem(struct mtk_srccap_dev *srccap_dev,
	bool *bForce);
enum V4L2_AVD_RESULT mtk_avd_match_vd_sampling(enum v4l2_ext_avd_videostandardtype videotype,
	struct srccap_avd_vd_sampling *vdsampling_table,
	u16 table_entry_cnt,
	u16 *vdsampling_table_index);
#endif  // MTK_SRCCAP_AVD_COLORSYSTEM_H
