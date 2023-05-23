/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __SHOW_PARAM_H__
#define __SHOW_PARAM_H__

#include <linux/mtk-v4l2-srccap.h>
#include <linux/videodev2.h>

void show_v4l2_ext_avd_hsync_edge(unsigned char *u8HsyncEdge);
void show_v4l2_ext_avd_g_flag(unsigned int *u32AVDFlag);
void show_v4l2_ext_avd_noise_mag(unsigned char *u8NoiseMag);
void show_v4l2_ext_avd_is_synclocked(unsigned char *u8lock);
void show_v4l2_ext_avd_status(unsigned short *u16vdstatus);
void show_v4l2_ext_avd_timing(struct v4l2_timing *timing);
void show_v4l2_ext_avd_standard_detection(
			struct v4l2_ext_avd_standard_detection *detection);
void show_v4l2_ext_avd_scan_hsyc_check(
			struct v4l2_ext_avd_scan_hsyc_check *hsyc);
void show_v4l2_ext_avd_info(struct v4l2_ext_avd_info *info);
void show_v4l2_ext_avd_sync_sensitivity(
			struct v4l2_ext_avd_sync_sensitivity *sensitivity);
void show_v4l2_ext_avd_init_para(struct v4l2_ext_avd_init_data *init);
void show_v4l2_ext_avd_input(unsigned char *u8ScartFB);
void show_v4l2_ext_avd_s_flag(unsigned int *u32VDPatchFlag);
void show_v4l2_ext_avd_force_video_standard(v4l2_std_id *u64Videostandardtype);
void show_v4L2_ext_avd_video_standard(
			struct v4L2_ext_avd_video_standard *standard);
void show_v4L2_ext_avd_freerun_freq(
			enum v4l2_ext_avd_freerun_freq *pFreerunFreq);
void show_v4l2_ext_avd_still_image_param(
			struct v4l2_ext_avd_still_image_param *image);
void show_v4l2_ext_avd_factory_data(
			struct v4l2_ext_avd_factory_data *factory);
void show_v4l2_ext_avd_3d_comb(bool *bCombEanble);
void show_v4l2_ext_avd_htt_user_md(unsigned short *u16Htt);
void show_v4l2_ext_avd_hsync_detect_detetion_for_tuning(bool *bTuningEanble);
void show_v4l2_ext_avd_3d_comb_speed(
			struct v4l2_ext_avd_3d_comb_speed *speed);
void show_v4l2_ext_avd_is_lock_audio_carrier(bool *bLock);
void show_v4l2_ext_avd_alive_check(bool *bAlive);
#endif /*__SHOW_PARAM_H__*/
