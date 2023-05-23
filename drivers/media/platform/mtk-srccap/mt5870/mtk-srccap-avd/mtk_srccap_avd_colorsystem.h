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
enum V4L2_AVD_RESULT mtk_avd_SetColorSystem(v4l2_std_id std);
enum V4L2_AVD_RESULT mtk_avd_standard_detetion(
		struct v4l2_ext_avd_standard_detection *pDetection);
enum V4L2_AVD_RESULT mtk_avd_status(unsigned short *u16vdstatus);
enum V4L2_AVD_RESULT mtk_avd_force_video_standard(
			v4l2_std_id *u64Videostandardtype);
enum V4L2_AVD_RESULT mtk_avd_video_standard(
			struct v4L2_ext_avd_video_standard *pVideoStandard);
enum V4L2_AVD_RESULT mtk_avd_start_auto_standard_detetion(void);
enum V4L2_AVD_RESULT mtk_avd_reg_from_dsp(void);
enum V4L2_AVD_RESULT mtk_avd_TimingMonitor(struct v4l2_srccap_pcm *pstPCM,
	struct mtk_srccap_dev *srccap_dev);
enum V4L2_AVD_RESULT mtk_avd_TimingVideoInfo(struct v4l2_timing *stVideoInfo,
	struct mtk_srccap_dev *srccap_dev);
#endif  // MTK_SRCCAP_AVD_COLORSYSTEM_H
