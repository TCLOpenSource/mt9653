/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_AVD_AVD_H
#define MTK_SRCCAP_AVD_AVD_H

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
enum V4L2_AVD_RESULT mtk_srccap_avd_init(struct mtk_srccap_dev *srccap_dev);
enum V4L2_AVD_RESULT mtk_avd_InitHWinfo(struct mtk_srccap_dev *dev);

//VIDIOC_G_EXT_CTRLS ioctl API
enum V4L2_AVD_RESULT mtk_avd_hsync_edge(unsigned char *u8HsyncEdge);
enum V4L2_AVD_RESULT mtk_avd_g_flag(unsigned int *u32AVDFlag);
enum V4L2_AVD_RESULT mtk_avd_v_total(struct v4l2_timing *ptiming);
enum V4L2_AVD_RESULT mtk_avd_noise_mag(unsigned char *u8NoiseMag);
enum V4L2_AVD_RESULT mtk_avd_is_synclocked(unsigned char *u8lock);
enum V4L2_AVD_RESULT mtk_avd_scan_hsync_check(
		struct v4l2_ext_avd_scan_hsyc_check *pHsync);
enum V4L2_AVD_RESULT mtk_avd_vertical_freq(struct v4l2_timing *ptiming);
enum V4L2_AVD_RESULT mtk_avd_info(struct v4l2_ext_avd_info *pinfo);
enum V4L2_AVD_RESULT mtk_avd_is_lock_audio_carrier(bool *bLock);
enum V4L2_AVD_RESULT mtk_avd_alive_check(bool *bAlive);
//VIDIOC_S_EXT_CTRLS ioctl API
enum V4L2_AVD_RESULT mtk_avd_init(struct v4l2_ext_avd_init_data pParam);
enum V4L2_AVD_RESULT mtk_avd_exit(void);
enum V4L2_AVD_RESULT mtk_avd_s_flag(unsigned int *u32VDPatchFlag);
enum V4L2_AVD_RESULT mtk_avd_freerun_freq(
			enum v4l2_ext_avd_freerun_freq *pFreerunFreq);
enum V4L2_AVD_RESULT mtk_avd_still_image_param(
			struct v4l2_ext_avd_still_image_param *pImageParam);
enum V4L2_AVD_RESULT mtk_avd_factory_para(
			struct v4l2_ext_avd_factory_data *pFactoryData);
enum V4L2_AVD_RESULT mtk_avd_3d_comb_speed(
			struct v4l2_ext_avd_3d_comb_speed *pspeed);
enum V4L2_AVD_RESULT mtk_avd_3d_comb(bool *bCombEanble);
enum V4L2_AVD_RESULT mtk_avd_htt_user_md(unsigned short *u16Htt);
enum V4L2_AVD_RESULT mtk_avd_hsync_detect_detetion_for_tuning(
			bool *bTuningEanble);
enum V4L2_AVD_RESULT mtk_avd_channel_change(void);
enum V4L2_AVD_RESULT mtk_avd_mcu_reset(void);
enum V4L2_AVD_RESULT mtk_avd_3d_comb_speed_up(void);

#endif  // MTK_SRCCAP_AVD_AVD_H
