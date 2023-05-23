/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_MEMOUT_H
#define MTK_SRCCAP_MEMOUT_H

#include "mtk_srccap_memout_svp.h"

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */
#define SRCCAP_MEMOUT_ALIGN(val, align) (((val) + ((align) - 1)) & (~((align) - 1)))

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum srccap_memout_metatag {
	SRCCAP_MEMOUT_METATAG_FRAME_INFO = 0,
	SRCCAP_MEMOUT_METATAG_SVP_INFO,
};

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct mtk_srccap_dev;

struct srccap_memout_info {
	enum v4l2_memout_buf_ctrl_mode buf_ctrl_mode;
#ifdef CONFIG_OPTEE
	struct srccap_memout_svp_info  secure_info;
#endif
	struct v4l2_pix_format_mplane fmt_mp;
	u8 num_planes;
	u8 num_bufs;
	u32 frame_pitch[3]; // unit: byte
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
int mtk_srccap_register_memout_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_memout,
	struct v4l2_ctrl_handler *memout_ctrl_handler);
void mtk_srccap_unregister_memout_subdev(
	struct v4l2_subdev *subdev_memout);

#endif  // MTK_SRCCAP_MEMOUT_H
