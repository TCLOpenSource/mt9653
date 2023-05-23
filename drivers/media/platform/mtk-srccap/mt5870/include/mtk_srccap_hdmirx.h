/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_HDMIRX_H
#define MTK_SRCCAP_HDMIRX_H

#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
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
struct srccap_hdmirx_info {
	struct task_struct *ctrl_packet_monitor_task;
	struct task_struct *ctrl_5v_detect_monitor_task;
	struct task_struct *ctrl_hdcp2x_irq_monitor_task;
	struct task_struct *ctrl_hdcp1x_irq_monitor_task;
	struct task_struct *ctrl_hdmi_crc_monitor_task;
	struct task_struct *ctrl_hdmi_mode_monitor_task;
	struct task_struct *ctrl_hdcp_state_monitor_task;
	void *ucSetBuffer;
	bool ubHDCP22ISREnable;
	bool ubHDCP14ISREnable;
	unsigned char u8ALLMMode;
	bool bCableDetect[(INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0)];
	bool bHDMIMode[(INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0)];
	bool bAVMute[(INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0)];
	unsigned char ucHDCPState[(INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0)];
	unsigned short usHDMICrc[(INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0)][3];
	stAVI_PARSING_INFO AVI[(INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0)];
	MS_HDMI_COLOR_FORMAT color_format;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_hdmirx_init(struct mtk_srccap_dev *srccap_dev);
int mtk_hdmirx_release(struct mtk_srccap_dev *srccap_dev);
void mtk_hdmirx_hdcp_isr(int irq, void *priv);
unsigned int mtk_hdmirx_STREventProc(EN_POWER_MODE usPowerState);
int mtk_hdmirx_set_current_port(unsigned int current_input);
int mtk_hdmirx_g_edid(struct v4l2_edid *edid);
int mtk_hdmirx_s_edid(struct v4l2_edid *edid);
int mtk_hdmirx_g_audio_channel_status(struct v4l2_audio *audio);
int mtk_hdmirx_g_dv_timings(struct v4l2_dv_timings *timings);
int mtk_hdmirx_g_fmt_vid_cap(struct v4l2_format *format);
int mtk_hdmirx_store_color_fmt(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_register_hdmirx_subdev(struct v4l2_device *srccap_dev,
			struct v4l2_subdev *hdmirx_subdev,
			struct v4l2_ctrl_handler *hdmirx_ctrl_handler);
void mtk_srccap_unregister_hdmirx_subdev(struct v4l2_subdev *hdmirx_subdev);
int mtk_hdmirx_subscribe_ctrl_event(struct mtk_srccap_dev *srccap_dev,
			const struct v4l2_event_subscription *event_sub);
int mtk_hdmirx_unsubscribe_ctrl_event(struct mtk_srccap_dev *srccap_dev,
			const struct v4l2_event_subscription *event_sub);

#endif  //#ifndef MTK_SRCCAP_HDMIRX_H
