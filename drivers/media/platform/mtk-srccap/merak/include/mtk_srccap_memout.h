/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_MEMOUT_H
#define MTK_SRCCAP_MEMOUT_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/mtk-v4l2-srccap.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

#include "mtk_srccap_memout_svp.h"

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
#define SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM (3)
#define SRCCAP_MEMOUT_FRAME_NUM (10)
#define SRCCAP_MEMOUT_RW_DIFF (2)
#define SRCCAP_MEMOUT_ONE_INDEX (1)
#define SRCCAP_MEMOUT_TWO_INDEX (2)
#define SRCCAP_MEMOUT_24FPS_FRAME_TIME (42)
#define SRCCAP_MEMOUT_MAX_W_INDEX (31)
#define SRCCAP_MEMOUT_TOP_FIELD (1)
#define SRCCAP_MEMOUT_BOTTOM_FIELD (0)
#define SRCCAP_MEMOUT_MIN_BUF_FOR_CAPTURING_FIELDS (2)
#define SRCCAP_MEMOUT_MAX_THREAD_PROCESSING_TIME (8000)
#define SRCCAP_MEMOUT_REG_NUM (10)

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */
#define SRCCAP_MEMOUT_ALIGN(val, align) (((val) + ((align) - 1)) & (~((align) - 1)))

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum srccap_memout_buffer_stage {
	SRCCAP_MEMOUT_BUFFER_STAGE_NEW = 0,
	SRCCAP_MEMOUT_BUFFER_STAGE_CAPTURING,
	SRCCAP_MEMOUT_BUFFER_STAGE_DONE,
};

enum srccap_memout_metatag {
	SRCCAP_MEMOUT_METATAG_FRAME_INFO = 0,
	SRCCAP_MEMOUT_METATAG_SVP_INFO,
	SRCCAP_MEMOUT_METATAG_COLOR_INFO,
	SRCCAP_MEMOUT_METATAG_HDR_PKT,
	SRCCAP_MEMOUT_METATAG_HDR_VSIF_PKT,
	SRCCAP_MEMOUT_METATAG_HDR_EMP_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_AVI_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_GCP_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_SPD_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_VSIF_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_HDR_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_AUI_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_MPG_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_VS_EMP_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_DSC_EMP_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_VRR_EMP_PKT,
	SRCCAP_MEMOUT_METATAG_HDMIRX_DHDR_EMP_PKT,
	SRCCAP_MEMOUT_METATAG_DIP_POINT_INFO,
};

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct mtk_srccap_dev;

struct srccap_memout_buffer_entry {
	struct vb2_buffer *vb;
	enum srccap_memout_buffer_stage stage;
	u64 addr[SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM];
	u8 w_index;
	u8 field;
	struct list_head list;
};

struct srccap_memout_frame_queue {
	struct list_head inputq_head;
	struct list_head outputq_head;
	struct list_head processingq_head;
};

struct srccap_memout_info {
	enum v4l2_memout_buf_ctrl_mode buf_ctrl_mode;
#if IS_ENABLED(CONFIG_OPTEE)
	struct srccap_memout_svp_info  secure_info;
#endif
	struct v4l2_pix_format_mplane fmt_mp;
	u8 num_planes;
	u8 num_bufs;
	u32 frame_pitch[SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM]; // unit: byte
	u8 access_mode;
	struct srccap_memout_frame_queue frame_q;
	u32 frame_id;
	bool first_frame;
	u32 hoffset;
	u8 field_info;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_memout_read_metadata(
	int meta_fd,
	enum srccap_memout_metatag meta_tag,
	void *meta_content);
int mtk_memout_write_metadata(
	int meta_fd,
	enum srccap_memout_metatag meta_tag,
	void *meta_content);
int mtk_memout_remove_metadata(
	int meta_fd,
	enum srccap_memout_metatag meta_tag);
void mtk_memout_process_buffer(
	struct mtk_srccap_dev *srccap_dev,
	u8 src_field);
int mtk_memout_s_fmt_vid_cap(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_format *format);
int mtk_memout_s_fmt_vid_cap_mplane(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_format *format);
int mtk_memout_g_fmt_vid_cap(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_format *format);
int mtk_memout_g_fmt_vid_cap_mplane(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_format *format);
int mtk_memout_reqbufs(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_requestbuffers *req_buf);
int mtk_memout_qbuf(
	struct mtk_srccap_dev *srccap_dev,
	struct vb2_buffer *vb);
int mtk_memout_dqbuf(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_buffer *buf);
int mtk_memout_streamon(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_buf_type buf_type);
int mtk_memout_streamoff(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_buf_type buf_type);
void mtk_memout_vsync_isr(
	void *param);
int mtk_srccap_register_memout_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_memout,
	struct v4l2_ctrl_handler *memout_ctrl_handler);
void mtk_srccap_unregister_memout_subdev(
	struct v4l2_subdev *subdev_memout);

void mtk_memout_set_capture_size(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_ml_script_info *ml_script_info,
	struct v4l2_area_size *capture_size);

#endif  // MTK_SRCCAP_MEMOUT_H
