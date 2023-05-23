/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_ENHANCE_H
#define _MTK_PQ_ENHANCE_H

struct mtk_pqd_settings {
	char  PqBinPath[V4L2_PQ_FILE_PATH_LENGTH];
	bool  IsDDR2;
	__u32 DDR_Freq;
	__u8  Bus_Width;
	__u64 PQVR_Addr;
	__u32 PQVR_Len;
};

struct mtk_pq_enhance {
	struct mtk_pqd_settings pqd_settings;
};

enum EN_PQ_ENHANCE_INPUT_SOURCE {
	/// VGA
	E_PQ_ENHANCE_INPUT_SOURCE_VGA = 0x00,
	/// ATV
	E_PQ_ENHANCE_INPUT_SOURCE_TV = 0x01,
	/// CVBS
	E_PQ_ENHANCE_INPUT_SOURCE_CVBS = 0x02,
	/// S-video
	E_PQ_ENHANCE_INPUT_SOURCE_SVIDEO = 0x03,
	/// Component
	E_PQ_ENHANCE_INPUT_SOURCE_YPBPR = 0x04,
	/// Scart
	E_PQ_ENHANCE_INPUT_SOURCE_SCART = 0x05,
	/// HDMI
	E_PQ_ENHANCE_INPUT_SOURCE_HDMI = 0x06,
	/// DTV
	E_PQ_ENHANCE_INPUT_SOURCE_DTV = 0x07,
	/// DVI
	E_PQ_ENHANCE_INPUT_SOURCE_DVI = 0x08,
	// Application source
	/// Storage
	E_PQ_ENHANCE_INPUT_SOURCE_STORAGE = 0x09,
	/// KTV
	E_PQ_ENHANCE_INPUT_SOURCE_KTV = 0x0A,
	/// JPEG
	E_PQ_ENHANCE_INPUT_SOURCE_JPEG = 0x0B,
	//RX for ulsa
	E_PQ_ENHANCE_INPUT_SOURCE_RX = 0x0C,
	/// The max support number of PQ input source
	E_PQ_ENHANCE_INPUT_SOURCE_RESERVED_START,
	/// None
	E_PQ_ENHANCE_INPUT_SOURCE_NONE =
		E_PQ_ENHANCE_INPUT_SOURCE_RESERVED_START,
	//For general usage, Internal Use
	E_PQ_ENHANCE_INPUT_SOURCE_GENERAL = 0xFF,
};


// function
int mtk_pq_register_enhance_subdev(
	struct v4l2_device *pq_dev,
	struct v4l2_subdev *subdev_enhance,
	struct v4l2_ctrl_handler *enhance_ctrl_handler);
void mtk_pq_unregister_enhance_subdev(
	struct v4l2_subdev *subdev_enhance);
int mtk_pq_enhance_open(struct mtk_pq_device *pq_dev);
int mtk_pq_enhance_close(struct mtk_pq_device *pq_dev);
int mtk_pq_enhance_set_input_source(
	struct mtk_pq_device *pq_dev, __u32 input);
int mtk_pq_enhance_set_pix_format_in_mplane(
	struct mtk_pq_device *pq_dev, struct v4l2_pix_format_mplane *pix);
int mtk_pq_enhance_set_pix_format_out_mplane(
	struct mtk_pq_device *pq_dev, struct v4l2_pix_format_mplane *pix);
int mtk_pq_enhance_stream_on(struct mtk_pq_device *pq_dev);
int mtk_pq_enhance_stream_off(struct mtk_pq_device *pq_dev);
int mtk_pq_enhance_subscribe_event(
	struct mtk_pq_device *pq_dev, __u32 event_type);
int mtk_pq_enhance_unsubscribe_event(
	struct mtk_pq_device *pq_dev, __u32 event_type);
int mtk_pq_enhance_event_queue(struct mtk_pq_device *pq_dev,
	__u32 event_type);
int mtk_pq_enhance_parse_dts(struct mtk_pq_enhance *pq_enhance,
	struct platform_device *pdev);
int mtk_pq_enhance_set_atv_ext_info(
	struct mtk_pq_device *pq_dev, struct v4l2_atv_ext_info *atv_info);
int mtk_pq_enhance_set_hdmi_ext_info(
	struct mtk_pq_device *pq_dev, struct v4l2_hdmi_ext_info *hdmi_info);

int mtk_pq_enhance_set_dtv_ext_info(
	struct mtk_pq_device *pq_dev, struct v4l2_dtv_ext_info *dtv_info);

int mtk_pq_enhance_set_av_ext_info(
	struct mtk_pq_device *pq_dev, struct v4l2_av_ext_info *av_info);

int mtk_pq_enhance_set_sv_ext_info(
	struct mtk_pq_device *pq_dev, struct v4l2_sv_ext_info *sv_info);

int mtk_pq_enhance_set_scart_ext_info(
	struct mtk_pq_device *pq_dev, struct v4l2_scart_ext_info *scart_info);

#endif
