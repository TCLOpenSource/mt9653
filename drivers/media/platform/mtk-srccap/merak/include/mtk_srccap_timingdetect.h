/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_TIMINGDETECT_H
#define MTK_SRCCAP_TIMINGDETECT_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/mtk-v4l2-srccap.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
/* signal status */
#define SRCCAP_TIMINGDETECT_HPD_MASK (0x3FFF)
#define SRCCAP_TIMINGDETECT_VTT_MASK (0x1FFF)
#define SRCCAP_TIMINGDETECT_HPD_MIN (0)
#define SRCCAP_TIMINGDETECT_VTT_MIN (0)
#define SRCCAP_TIMINGDETECT_HPD_TOL (5)
#define SRCCAP_TIMINGDETECT_VTT_TOL (9)
#define SRCCAP_TIMINGDETECT_STABLE_CNT (50)
#define SRCCAP_TIMINGDETECT_FRL_8P_MULTIPLIER (8)
#define SRCCAP_TIMINGDETECT_YUV420_MULTIPLIER (2)
#define SRCCAP_TIMINGDETECT_INTERLACE_ADDEND (2)
#define SRCCAP_TIMINGDETECT_DIVIDE_BASE_2 (2)
#define SRCCAP_TIMINGDETECT_VRR_DEBOUNCE_CNT (10)
#define SRCCAP_TIMINGDETECT_VRR_TYPE_CNT (3)


/* timing status */
#define SRCCAP_TIMINGDETECT_FLAG_NULL (0x00)

#define SRCCAP_TIMINGDETECT_POL_HPVP_BIT (0)
#define SRCCAP_TIMINGDETECT_FLAG_POL_HPVP (1 << SRCCAP_TIMINGDETECT_POL_HPVP_BIT)

#define SRCCAP_TIMINGDETECT_POL_HPVN_BIT (1)
#define SRCCAP_TIMINGDETECT_FLAG_POL_HPVN (1 << SRCCAP_TIMINGDETECT_POL_HPVN_BIT)

#define SRCCAP_TIMINGDETECT_POL_HNVP_BIT (2)
#define SRCCAP_TIMINGDETECT_FLAG_POL_HNVP (1 << SRCCAP_TIMINGDETECT_POL_HNVP_BIT)

#define SRCCAP_TIMINGDETECT_POL_HNVN_BIT (3)
#define SRCCAP_TIMINGDETECT_FLAG_POL_HNVN (1 << SRCCAP_TIMINGDETECT_POL_HNVN_BIT)

#define SRCCAP_TIMINGDETECT_INTERLACED_BIT (4)
#define SRCCAP_TIMINGDETECT_FLAG_INTERLACED (1 << SRCCAP_TIMINGDETECT_INTERLACED_BIT)

#define SRCCAP_TIMINGDETECT_CE_TIMING_BIT (5)
#define SRCCAP_TIMINGDETECT_FLAG_CE_TIMING (1 << SRCCAP_TIMINGDETECT_CE_TIMING_BIT)

#define SRCCAP_TIMINGDETECT_YPBPR_BIT (6)
#define SRCCAP_TIMINGDETECT_FLAG_YPBPR (1 << SRCCAP_TIMINGDETECT_YPBPR_BIT)

#define SRCCAP_TIMINGDETECT_HDMI_BIT (7)
#define SRCCAP_TIMINGDETECT_FLAG_HDMI (1 << SRCCAP_TIMINGDETECT_HDMI_BIT)

#define SRCCAP_TIMINGDETECT_DVI_BIT (8)
#define SRCCAP_TIMINGDETECT_FLAG_DVI (1 << SRCCAP_TIMINGDETECT_DVI_BIT)

#define SRCCAP_TIMINGDETECT_VGA_BIT (9)
#define SRCCAP_TIMINGDETECT_FLAG_VGA (1 << SRCCAP_TIMINGDETECT_VGA_BIT)

#define SRCCAP_TIMINGDETECT_HFREQ_TOLERANCE (15)
#define SRCCAP_TIMINGDETECT_HFREQ_TOLERANCE_HIGH (200)
#define SRCCAP_TIMINGDETECT_VFREQ_TOLERANCE (1500)
#define SRCCAP_TIMINGDETECT_VFREQ_TOLERANCE_LOW (500)
#define SRCCAP_TIMINGDETECT_VTT_TOLERANCE (10)

#define SRCCAP_TIMINGDETECT_TIMING_INFO_NUM (10)
#define SRCCAP_VD_SAMPLING_INFO_NUM (7)

#define SRCCAP_TIMINGDETECT_TIMING_HDMI21_VTT_THRESHOLD	(225)
#define SRCCAP_TIMINGDETECT_TIMING_HDMI21_VDE_THRESHOLD	(216)

#define SRCCAP_TIMINGDETECT_CAPWIN_PARA_NUM (2)

#define SRCCAP_TIMINGDETECT_RESET_DELAY_1000 (1000)
#define SRCCAP_TIMINGDETECT_RESET_DELAY_1100 (1100)

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum srccap_timingdetect_vrr_type {
	SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR = 0,
	SRCCAP_TIMINGDETECT_VRR_TYPE_VRR,
	SRCCAP_TIMINGDETECT_VRR_TYPE_CVRR
};

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct mtk_srccap_dev;

struct srccap_timingdetect_timing {
	u32 hfreqx10; // unit: kHz
	u32 vfreqx1000; // unit: Hz
	u16 hstart;
	u16 vstart;
	u16 hde;
	u16 vde;
	u16 htt;
	u16 vtt;
	u16 adcphase;
	u16 status;
};

struct srccap_timingdetect_data {
	u16 h_start;
	u16 v_start;
	u16 h_de;		/* width */
	u16 v_de;		/* height */
	u32 h_freqx10;
	u32 v_freqx1000;	/* frameratex1000 */
	bool interlaced;
	u16 h_period;
	u16 h_total;
	u16 v_total;
	bool h_pol;		/* hsync polarity */
	bool v_pol;		/* vsync polarity */
	u16 adc_phase;
	bool ce_timing;
	bool yuv420;
	bool frl;
	enum srccap_timingdetect_vrr_type vrr_type;
	bool vrr_enforcement_en;
	u32 refresh_rate;
	u8 colorformat;
};

struct srccap_timingdetect_vdsampling {
	enum v4l2_ext_avd_videostandardtype videotype;
	u16 v_start;
	u16 h_start;
	u16 h_de;
	u16 v_de;
	u16 h_total;
	u32 v_freqx1000;
};

struct srccap_timingdetect_info {
	enum v4l2_timingdetect_status status;
	u16 table_entry_count;
	struct srccap_timingdetect_timing *timing_table;
	struct v4l2_rect cap_win;
	struct srccap_timingdetect_data data;
	struct srccap_timingdetect_vdsampling *vdsampling_table;
	u16 vdsampling_table_entry_count;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_timingdetect_init(
	struct mtk_srccap_dev *srccap_dev);
int mtk_timingdetect_init_offline(
	struct mtk_srccap_dev *srccap_dev);
int mtk_timingdetect_init_clock(
	struct mtk_srccap_dev *srccap_dev);
int mtk_timingdetect_deinit(
	struct mtk_srccap_dev *srccap_dev);
int mtk_timingdetect_deinit_clock(
	struct mtk_srccap_dev *srccap_dev);
int mtk_timingdetect_set_capture_win(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_rect cap_win);
int mtk_timingdetect_force_progressive(
	struct mtk_srccap_dev *srccap_dev,
	bool force_en);
int mtk_timingdetect_retrieve_timing(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_timingdetect_vrr_type vrr_type,
	bool hdmi_free_sync_status,
	struct srccap_timingdetect_data *data);
int mtk_timingdetect_store_timing_info(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_timingdetect_data data);
int mtk_timingdetect_get_signal_status(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_timingdetect_vrr_type *vrr_type,
	bool hdmi_free_sync_status,
	enum v4l2_timingdetect_status *status);
int mtk_timingdetect_streamoff(
	struct mtk_srccap_dev *srccap_dev);
int mtk_timingdetect_check_sync(
	enum v4l2_srccap_input_source src, bool adc_offline_status, bool *has_sync);
int mtk_timingdetect_get_field(
	struct mtk_srccap_dev *srccap_dev,
	u8 *field);
int mtk_timingdetect_set_auto_no_signal_mute(
	struct mtk_srccap_dev *srccap_dev,
	bool enable);
int mtk_timingdetect_set_ans_vtt_det_en(
	struct mtk_srccap_dev *srccap_dev,
	bool disable);
int mtk_timingdetect_steamon(
	struct mtk_srccap_dev *srccap_dev);
int mtk_timingdetect_set_8p(
	struct mtk_srccap_dev *srccap_dev,
	bool frl);
int mtk_timingdetect_set_hw_version(
	struct mtk_srccap_dev *srccap_dev,
	bool stub);
int mtk_srccap_register_timingdetect_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_timingdetect,
	struct v4l2_ctrl_handler *timingdetect_ctrl_handler);
void mtk_srccap_unregister_timingdetect_subdev(
	struct v4l2_subdev *subdev_timingdetect);
#endif  // MTK_SRCCAP_TIMINGDETECT_H

