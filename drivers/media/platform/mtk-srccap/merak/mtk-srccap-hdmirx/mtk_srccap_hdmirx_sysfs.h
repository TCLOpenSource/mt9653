/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_HDMIRX_SYSFS_H
#define MTK_SRCCAP_HDMIRX_SYSFS_H

#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
#define MAX_HDMIRX_PORT_NUM		4
#define STR_UNIT_LENGTH			60
#define INTERNATIONAL_UNIT_KILO	1000
#define INVALID_DATA			0xFFFF

#define AVI_INFO_Y20_START_BIT		5
#define AVI_INFO_C10_START_BIT		6
#define AVI_INFO_EC20_START_BIT		4
#define AVI_INFO_ACE20_START_BIT	4
#define GCP_COLOR_DEPTH_START_BIT	8
#define VALID_4_BIT_MASK	0x0F
#define VALID_3_BIT_MASK	0x07
#define VALID_2_BIT_MASK	0x03
#define BIT_SHIFT_VAL_8_TIMES		3
#define COLOR_DEPTH_VAL_8			8
#define COLOR_DEPTH_VAL_10			10
#define COLOR_DEPTH_VAL_12			12
#define COLOR_DEPTH_VAL_16			16

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
typedef enum {
	E_Y20_RGB = 0,
	E_Y20_YCC422,
	E_Y20_YCC444,
	E_Y20_YCC420,
} E_Y20_ITEM;

typedef enum {
	E_DSC_FSM_NONE_SYS,
	E_DSC_FSM_HV_REGEN_SYS,
	E_DSC_FSM_DSC_CONFIG_SYS,
	E_DSC_FSM_REGEN_SYS,
} E_HDMI_DSC_FSM;

typedef enum {
	E_REGEN_INIT_SYS,
	E_REGEN_CONFIG_SYS,
	E_REGEN_DONE_SYS,
} E_HDMI_REGEN_FSM;

typedef enum {
	//ACE3,ACE2,ACE1,ACE0,EC2,EC1,EC0,C1,C0
	E_COLORIMETRY_SMPTE_170M = 0x01,
	E_COLORIMETRY_BT_709 = 0x02,

	E_COLORIMETRY_xvYCC601 = 0x03,
	E_COLORIMETRY_xvYCC709 = 0x07,
	E_COLORIMETRY_sYCC601 = 0x0B,
	E_COLORIMETRY_Adobe_YCC601 = 0x0F,
	E_COLORIMETRY_Adobe_RGB = 0x13,
	E_COLORIMETRY_BT_2020_EC5 = 0x17,
	E_COLORIMETRY_BT_2020_EC6 = 0x1B,

	E_COLORIMETRY_DCI_P3_D65 = 0x1F,
	E_COLORIMETRY_DCI_P3_Theater = 0x3F,
} E_COLORIMETRY_ITEM;

typedef enum {
	E_COLORDEPTH_ZERO = 0,
	E_COLORDEPTH_24BITS_PER_PIXEL = 4,
	E_COLORDEPTH_30BITS_PER_PIXEL,
	E_COLORDEPTH_36BITS_PER_PIXEL,
	E_COLORDEPTH_48BITS_PER_PIXEL,
} E_COLORDEPTH_ITEM;

typedef enum {
	E_VIDEO_STATUS_5V,
	E_VIDEO_STATUS_CLOCK_DETECT,
	E_VIDEO_STATUS_DE_STABLE,
	E_VIDEO_STATUS_SOURCE_VERSION,
	E_VIDEO_STATUS_WIDTH,
	E_VIDEO_STATUS_HEIGHT,
	E_VIDEO_STATUS_HTOTAL,
	E_VIDEO_STATUS_VTOTAL,
	E_VIDEO_STATUS_HPolarity,
	E_VIDEO_STATUS_HSync,
	E_VIDEO_STATUS_HFPorch,
	E_VIDEO_STATUS_HBPorch,
	E_VIDEO_STATUS_VPolarity,
	E_VIDEO_STATUS_VSync,
	E_VIDEO_STATUS_VFPorch,
	E_VIDEO_STATUS_VBPorch,
	E_VIDEO_STATUS_PIXEL_FREQUENCY,
	E_VIDEO_STATUS_REFRESH_RATE,
	E_VIDEO_STATUS_INTERLACE,
	E_VIDEO_STATUS_HDMI_DVI,
	E_VIDEO_STATUS_COLOR_SPACE_1,
	E_VIDEO_STATUS_COLOR_SPACE_2,
	E_VIDEO_STATUS_COLOR_DEPTH,
	E_VIDEO_STATUS_SCRAMBLING_STATUS,
	E_VIDEO_STATUS_TMDS_CLOCK_RATIO,
	E_VIDEO_STATUS_TMDS_SAMPLE_CLOCK,
	E_VIDEO_STATUS_AV_MUTE,
	E_VIDEO_STATUS_BCH_ERROR,
	E_VIDEO_STATUS_PACKET_RECEIVE,
	E_VIDEO_STATUS_FRL_RATE,
	E_VIDEO_STATUS_LANE_CNT,
	E_VIDEO_STATUS_RATION_DET,
	E_VIDEO_STATUS_HDMI20_Flag,
	E_VIDEO_STATUS_YUV420_Flag,
	E_VIDEO_STATUS_HDMI_Mode,
	E_VIDEO_STATUS_DSC_FSM,
	E_VIDEO_STATUS_Regen_Fsm,
	E_VIDEO_STATUS_HDCP_STATUS,
} E_HDMI_VIDEO_STATUS_ITEM;

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
typedef struct {
	MS_U8 u8IorP[15];
	MS_U8 u8HDMIorDVI[15];
	MS_U8 u8ColorSpace1[15];
	MS_U8 u8ColorSpace2[15];
	MS_U8 u8ClockRatio[15];
	MS_U8 u8SourceVer[15];
	MS_U8 u8FrlRate[15];
	MS_U8 u8RatDet[15];
	MS_U8 u8DscFSM[15];
	MS_U8 u8RegFSM[15];
	MS_U8 u8Hdcp[15];
} HDMI_STATUS_STRING;

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */

int mtk_srccap_hdmirx_sysfs_init(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_hdmirx_sysfs_deinit(struct mtk_srccap_dev *srccap_dev);

#endif  //#ifndef MTK_SRCCAP_HDMIRX_SYSFS_H
