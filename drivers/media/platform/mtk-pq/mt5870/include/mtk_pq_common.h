/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_COMMON_H
#define _MTK_PQ_COMMON_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <uapi/linux/mtk-v4l2-pq.h>

enum mtk_pq_common_metatag {
	EN_PQ_METATAG_VDEC_SVP_INFO = 0,
	EN_PQ_METATAG_SRCCAP_SVP_INFO,
	EN_PQ_METATAG_PQ_SVP_INFO,
	EN_PQ_METATAG_SH_FRM_INFO,
	EN_PQ_METATAG_MAX,
};

struct v4l2_common_info {
	__u32 input_source;
	struct v4l2_timing timing_in;
	struct v4l2_format format_cap;
	struct v4l2_format format_out;
};

struct mtk_pq_device;

/* function*/
int mtk_pq_common_set_input(struct mtk_pq_device *pq_dev,
		unsigned int input_index);
int mtk_pq_common_get_input(struct mtk_pq_device *pq_dev,
		unsigned int *p_input_index);
int mtk_pq_common_set_fmt_cap_mplane(
		struct mtk_pq_device *pq_dev,
		struct v4l2_format *fmt);
int mtk_pq_common_set_fmt_out_mplane(
		struct mtk_pq_device *pq_dev,
		struct v4l2_format *fmt);
int mtk_pq_common_get_fmt_cap_mplane(
		struct mtk_pq_device *pq_dev,
		struct v4l2_format *fmt);
int mtk_pq_common_get_fmt_out_mplane(
		struct mtk_pq_device *pq_dev,
		struct v4l2_format *fmt);
int mtk_pq_common_stream_off(struct mtk_pq_device *pq_dev);
int mtk_pq_register_common_subdev(struct v4l2_device *pq_dev,
		struct v4l2_subdev *subdev_common,
		struct v4l2_ctrl_handler *common_ctrl_handler);
void mtk_pq_unregister_common_subdev(struct v4l2_subdev *subdev_common);
int mtk_pq_common_read_metadata(int meta_fd,
	enum mtk_pq_common_metatag meta_tag, void *meta_content);
int mtk_pq_common_write_metadata(int meta_fd,
	enum mtk_pq_common_metatag meta_tag, void *meta_content);
int mtk_pq_common_delete_metadata(int meta_fd,
	enum mtk_pq_common_metatag meta_tag);
#endif
