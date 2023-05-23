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

#include "linux/metadata_utility/m_srccap.h"

#include "hwreg_srccap_hdmirx_pkt_hdr.h"
/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
#define MAX_INT_N 12
extern int hdmirxloglevel;

#define HDMIRX_MSG_ERROR(fmt, args...)	do { \
	if (hdmirxloglevel >= LOG_LEVEL_ERROR)	\
		pr_err("[SRCCAP_HDMIRX_ERR][%s][%d] " fmt,	\
			__func__, __LINE__, ## args);	\
} while (0)
#define HDMIRX_MSG_INFO(fmt, args...)	do { \
	if (hdmirxloglevel >= LOG_LEVEL_INFO)	\
		pr_err("[SRCCAP_HDMIRX_INFO][%s][%d] " fmt,	\
			__func__, __LINE__, ## args);	\
} while (0)
#define HDMIRX_MSG_DBG(fmt, args...)	do { \
	if (hdmirxloglevel >= LOG_LEVEL_DEBUG)	\
		pr_err("[SRCCAP_HDMIRX_DBG][%s][%d] " fmt,	\
			__func__, __LINE__, ## args);	\
} while (0)

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */
typedef irqreturn_t (*hdmi_isr_func)(int irq, void *priv);
/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */

typedef enum {
	E_INDEX_0,
	E_INDEX_1,
	E_INDEX_2,
	E_INDEX_3,
	E_INDEX_4,
	E_INDEX_5,
	E_INDEX_6,
	E_INDEX_7,
	E_INDEX_8,
	E_INDEX_9,
	E_INDEX_10,
	E_INDEX_11,
} INDEX_E;

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */

struct srccap_hdmirx_interrupt {
	int int_id[SRCCAP_HDMI_IRQ_END-SRCCAP_HDMI_IRQ_START];
	hdmi_isr_func int_func[SRCCAP_HDMI_IRQ_END-SRCCAP_HDMI_IRQ_START];
};

struct srccap_hdmirx_info {
	struct task_struct *ctrl_packet_monitor_task;
	struct task_struct *ctrl_5v_detect_monitor_task;
	struct task_struct *ctrl_hdcp2x_irq_monitor_task;
	struct task_struct *ctrl_hdcp1x_irq_monitor_task;
	struct task_struct *ctrl_hdmi_crc_monitor_task;
	struct task_struct *ctrl_hdmi_mode_monitor_task;
	struct task_struct *ctrl_hdcp_state_monitor_task;
	struct task_struct *ctrl_dsc_state_monitor_task;
	void *ucSetBuffer;
	bool ubHDCP22ISREnable;
	bool ubHDCP14ISREnable;
	unsigned char u8ALLMMode;
	bool bCableDetect[HDMI_PORT_MAX_NUM];
	bool bHDMIMode[HDMI_PORT_MAX_NUM];
	bool bAVMute[HDMI_PORT_MAX_NUM];
	unsigned char ucHDCPState[HDMI_PORT_MAX_NUM];
	unsigned short usHDMICrc[HDMI_PORT_MAX_NUM][3];
	stAVI_PARSING_INFO AVI[HDMI_PORT_MAX_NUM];
	MS_HDMI_COLOR_FORMAT color_format;
	unsigned char ucDSCState[HDMI_PORT_MAX_NUM];
	struct clk	*hdmi_mcu_ck;
	struct clk	*hdmi_en_clk_xtal;
	struct clk	*hdmi_en_smi2mac;

	//R2 clock
	struct clk	*hdmi_en_clk_r2_hdmi2mac;
	struct clk	*hdmi_r2_ck;

	// interrupt id from platform_get_irq, and isr
	struct srccap_hdmirx_interrupt hdmi_int_s;

	struct kobject hdmirx_kobj;
	struct kobject hdmirx_p_kobj[HDMI_PORT_MAX_NUM];
	int port_count;
	unsigned int u32Emp_vtem_info_subscribe_bitmap;
	struct s_pkt_emp_vtem emp_vtem_info[HDMI_PORT_MAX_NUM];

};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_hdmirx_init(struct mtk_srccap_dev *srccap_dev,
		    struct platform_device *pdev, int logLevel);
int mtk_hdmirx_deinit(struct mtk_srccap_dev *srccap_dev);
int mtk_hdmirx_set_current_port(struct mtk_srccap_dev *srccap_dev);
int mtk_hdmirx_g_edid(struct v4l2_edid *edid);
int mtk_hdmirx_s_edid(struct v4l2_edid *edid);
int mtk_hdmirx_g_audio_channel_status(struct mtk_srccap_dev *srccap_dev, struct v4l2_audio *audio);
int mtk_hdmirx_g_dv_timings(struct mtk_srccap_dev *srccap_dev, struct v4l2_dv_timings *timings);
//int mtk_hdmirx_g_fmt_vid_cap(struct mtk_srccap_dev *srccap_dev, struct v4l2_format *format);
int mtk_hdmirx_store_color_fmt(struct mtk_srccap_dev *srccap_dev);
int mtk_hdmirx_SetPowerState(struct mtk_srccap_dev *srccap_dev, EN_POWER_MODE ePowerState);
int mtk_srccap_register_hdmirx_subdev(struct v4l2_device *srccap_dev,
			struct v4l2_subdev *hdmirx_subdev,
			struct v4l2_ctrl_handler *hdmirx_ctrl_handler);
void mtk_srccap_unregister_hdmirx_subdev(struct v4l2_subdev *hdmirx_subdev);
int mtk_hdmirx_subscribe_ctrl_event(struct mtk_srccap_dev *srccap_dev,
			const struct v4l2_event_subscription *event_sub);
int mtk_hdmirx_unsubscribe_ctrl_event(struct mtk_srccap_dev *srccap_dev,
			const struct v4l2_event_subscription *event_sub);
int mtk_srccap_hdmirx_parse_dts_reg(
	struct mtk_srccap_dev *srccap_dev,
	struct device *property_dev);

int mtk_srccap_hdmirx_init(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_hdmirx_release(struct mtk_srccap_dev *srccap_dev);

extern E_MUX_INPUTPORT mtk_hdmirx_to_muxinputport(
	enum v4l2_srccap_input_source src);
extern enum v4l2_srccap_input_source mtk_hdmirx_muxinput_to_v4l2src(
	E_MUX_INPUTPORT src);
extern bool mtk_srccap_hdmirx_isHDMIMode(
	enum v4l2_srccap_input_source src);

extern bool mtk_srccap_hdmirx_get_pwr_saving_onoff_hwdef(
	enum v4l2_srccap_input_source src);

extern bool mtk_srccap_hdmirx_get_pwr_saving_onoff(
	enum v4l2_srccap_input_source src);

extern bool mtk_srccap_hdmirx_set_pwr_saving_onoff(
	enum v4l2_srccap_input_source src,
	bool b_on);
extern bool mtk_srccap_hdmirx_get_HDMIStatus(
	enum v4l2_srccap_input_source src
	);
extern bool mtk_srccap_hdmirx_get_freesync_info(
	enum v4l2_srccap_input_source src,
	struct v4l2_ext_hdmi_get_freesync_info *p_info
	);

// dsc
extern bool mtk_srccap_hdmirx_dsc_get_pps_info(
	enum v4l2_srccap_input_source src,
	struct v4l2_ext_hdmi_dsc_info *p_info);

// mux
extern bool mtk_srccap_hdmirx_mux_sel_dsc(
	enum v4l2_srccap_input_source src,
	enum v4l2_ext_hdmi_dsc_decoder_mux_n dsc_mux_n);

extern enum v4l2_srccap_input_source mtk_srccap_hdmirx_mux_query_dsc(
	enum v4l2_ext_hdmi_dsc_decoder_mux_n dsc_mux_n);

// pkt
extern unsigned char mtk_srccap_hdmirx_pkt_get_AVI(
	enum v4l2_srccap_input_source src,
	struct hdmi_avi_packet_info *ptr,
	unsigned int u32_buf_len);


extern unsigned char mtk_srccap_hdmirx_pkt_get_VSIF(
	enum v4l2_srccap_input_source src,
	struct hdmi_vsif_packet_info *ptr,
	unsigned int u32_buf_len);

extern unsigned char mtk_srccap_hdmirx_pkt_get_GCP(
	enum v4l2_srccap_input_source src,
	struct hdmi_gc_packet_info *ptr,
	unsigned int u32_buf_len);

extern unsigned char mtk_srccap_hdmirx_pkt_get_HDRIF(
	enum v4l2_srccap_input_source src,
	struct hdmi_hdr_packet_info *ptr,
	unsigned int u32_buf_len);

extern unsigned char mtk_srccap_hdmirx_pkt_get_VS_EMP(
	enum v4l2_srccap_input_source src,
	struct hdmi_emp_packet_info *ptr,
	unsigned int u32_buf_len);

extern unsigned char mtk_srccap_hdmirx_pkt_get_DSC_EMP(
	enum v4l2_srccap_input_source src,
	struct hdmi_emp_packet_info *ptr,
	unsigned int u32_buf_len);

extern unsigned char mtk_srccap_hdmirx_pkt_get_Dynamic_HDR_EMP(
	enum v4l2_srccap_input_source src,
	struct hdmi_emp_packet_info *ptr,
	unsigned int u32_buf_len);

extern unsigned char mtk_srccap_hdmirx_pkt_get_VRR_EMP(
	enum v4l2_srccap_input_source src,
	struct hdmi_emp_packet_info *ptr,
	unsigned int u32_buf_len);

extern bool mtk_srccap_hdmirx_pkt_get_report(
	enum v4l2_srccap_input_source src,
	enum v4l2_ext_hdmi_packet_state enPacketType,
	struct v4l2_ext_hdmi_pkt_report_s *buf_ptr);

extern bool mtk_srccap_hdmirx_pkt_is_dsc(
	enum v4l2_srccap_input_source src);

extern bool mtk_srccap_hdmirx_pkt_is_vrr(
	enum v4l2_srccap_input_source src);

extern unsigned char mtk_srccap_hdmirx_pkt_get_gnl(
	enum v4l2_srccap_input_source src,
	enum v4l2_ext_hdmi_packet_state enPacketType,
	struct hdmi_packet_info *gnl_ptr,
	unsigned int u32_buf_len);

extern bool
mtk_srccap_hdmirx_pkt_audio_mute_en(enum v4l2_srccap_input_source src,
				    bool mute_en);

extern bool
mtk_srccap_hdmirx_pkt_audio_status_clr(enum v4l2_srccap_input_source src);

extern MS_U32
mtk_srccap_hdmirx_pkt_get_audio_fs_hz(
	enum v4l2_srccap_input_source src);

#endif  //#ifndef MTK_SRCCAP_HDMIRX_H
