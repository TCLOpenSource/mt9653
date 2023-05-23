/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_ADC_H
#define MTK_SRCCAP_ADC_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/mtk-v4l2-srccap.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
#define ADCTBL_CLK_SET_NUM (50)
#define ADCTBL_CLK_SWEN_NUM (50)

// hardware fixed gain/offset/phase
#define ADC_VGA_FIXED_GAIN_R        (0x1B4A)
#define ADC_VGA_FIXED_GAIN_G        (0x1B4A)
#define ADC_VGA_FIXED_GAIN_B        (0x1B4A)
#define ADC_VGA_FIXED_OFFSET_R      (0x0000)
#define ADC_VGA_FIXED_OFFSET_G      (0x0000)
#define ADC_VGA_FIXED_OFFSET_B      (0x0000)
#define ADC_YPBPR_FIXED_GAIN_R      (0x17F8)
#define ADC_YPBPR_FIXED_GAIN_G      (0x176F)
#define ADC_YPBPR_FIXED_GAIN_B      (0x17F8)
#define ADC_YPBPR_FIXED_OFFSET_R    (0x0800)
#define ADC_YPBPR_FIXED_OFFSET_G    (0x0100)
#define ADC_YPBPR_FIXED_OFFSET_B    (0x0800)
#define ADC_YPBPR_FIXED_PHASE       (0x0005)

#define SRCCAP_ADC_MAX_CLK (3500)
#define SRCCAP_ADC_UDELAY_500 (500)
#define SRCCAP_ADC_UDELAY_510 (510)
#define SRCCAP_ADC_UDELAY_1000 (1000)
#define SRCCAP_ADC_UDELAY_1100 (1100)

#define SRCCAP_ADC_DOUBLE_SAMPLE_RESOLUTION  (720)

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum srccap_input_port;
enum srccap_access_type;

enum srccap_adc_source_info {
	// None ADC source
	SRCCAP_ADC_SOURCE_NONADC = 0,

	// Single ADC source
	SRCCAP_ADC_SOURCE_ONLY_CVBS,
	SRCCAP_ADC_SOURCE_ONLY_SVIDEO,
	SRCCAP_ADC_SOURCE_ONLY_YPBPR,
	SRCCAP_ADC_SOURCE_ONLY_RGB,
	SRCCAP_ADC_SOURCE_ONLY_INT_DMD_ATV,
	SRCCAP_ADC_SOURCE_ONLY_EXT_DMD_ATV,
	SRCCAP_ADC_SOURCE_ONLY_SCART_RGB,
	SRCCAP_ADC_SOURCE_ONLY_SCART_Y,

	SRCCAP_ADC_SOURCE_SINGLE_NUMS,

	// Multi ADC source
	SRCCAP_ADC_SOURCE_MULTI_RGB_ATV,
	SRCCAP_ADC_SOURCE_MULTI_RGB_SCARTY,
	SRCCAP_ADC_SOURCE_MULTI_RGB_CVBS,
	SRCCAP_ADC_SOURCE_MULTI_YPBPR_ATV,
	SRCCAP_ADC_SOURCE_MULTI_YPBPR_SCARTY,
	SRCCAP_ADC_SOURCE_MULTI_YPBPR_CVBS,

	SRCCAP_ADC_SOURCE_ALL_NUMS,
};

enum srccap_adc_isog_detect_mode {
	SRCCAP_ADC_ISOG_NORMAL_MODE = 0,
	SRCCAP_ADC_ISOG_STANDBY_MODE,
	SRCCAP_ADC_ISOG_MODE_NUMS,
};

enum srccap_adc_sog_online_mux {
	SRCCAP_SOG_ONLINE_PADA_GIN0P,
	SRCCAP_SOG_ONLINE_PADA_GIN1P,
	SRCCAP_SOG_ONLINE_PADA_GIN2P,
	SRCCAP_SOG_ONLINE_PADA_GND,
};

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct mtk_srccap_dev;

struct srccap_adc_info {
	enum srccap_adc_source_info adc_src;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_adc_enable_offline_sog(
	struct mtk_srccap_dev *srccap_dev,
	bool enable);
int mtk_adc_set_offline_mux(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_srccap_input_source src);
int mtk_adc_check_offline_source(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_srccap_input_source src,
	bool *offline_status);
int mtk_adc_set_freerun(
	struct mtk_srccap_dev *srccap_dev,
	bool enable);
int mtk_adc_set_mode(
	struct mtk_srccap_dev *srccap_dev);
int mtk_adc_load_cal_tbl(
	struct mtk_srccap_dev *srccap_dev);
int mtk_adc_set_source(
	struct v4l2_adc_source *source,
	struct mtk_srccap_dev *srccap_dev);
int mtk_adc_set_isog_detect_mode(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_adc_isog_detect_mode mode);
int mtk_adc_set_iclamp_rgb_powerdown(
	struct mtk_srccap_dev *srccap_dev,
	bool powerdown);
int mtk_adc_init(
	struct mtk_srccap_dev *srccap_dev);
int mtk_adc_deinit(
	struct mtk_srccap_dev *srccap_dev);
int mtk_adc_set_port(
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_register_adc_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_adc,
	struct v4l2_ctrl_handler *adc_ctrl_handler);
void mtk_srccap_unregister_adc_subdev(
	struct v4l2_subdev *subdev_adc);

#endif  // MTK_SRCCAP_ADC_H
