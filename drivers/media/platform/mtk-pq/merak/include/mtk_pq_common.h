/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_COMMON_H
#define _MTK_PQ_COMMON_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <uapi/linux/mtk-v4l2-pq.h>
#include "metadata_utility.h"
#include "pqu_msg.h"
#include "mtk_iommu_dtv_api.h"
#include "hwreg_common_bin.h"

#define PQ_CHECK_NULLPTR() \
	do { \
		pr_err("[%s,%5d] NULL POINTER\n", __func__, __LINE__); \
		dump_stack(); \
	} while (0)

enum mtk_pq_common_metatag {
	EN_PQ_METATAG_VDEC_SVP_INFO = 0,
	EN_PQ_METATAG_SRCCAP_SVP_INFO,
	EN_PQ_METATAG_SVP_INFO,
	EN_PQ_METATAG_SH_FRM_INFO,
	EN_PQ_METATAG_DISP_MDW_CTRL,
	EN_PQ_METATAG_DISP_IDR_CTRL,
	EN_PQ_METATAG_DISP_B2R_CTRL,
	EN_PQ_METATAG_PQU_DISP_FLOW_INFO,
	EN_PQ_METATAG_OUTPUT_FRAME_INFO,
	EN_PQ_METATAG_SRCCAP_FRAME_INFO,
	EN_PQ_METATAG_QBUF_INTERNAL_INFO,
	EN_PQ_METATAG_STREAM_INTERNAL_INFO,
	EN_PQ_METATAG_PQU_DEBUG_INFO,
	EN_PQ_METATAG_PQU_STREAM_INFO,
	EN_PQ_METATAG_PQU_BUFFER_INFO,
	EN_PQ_METATAG_PQU_PATTERN_INFO,
	EN_PQ_METATAG_PQU_PATTERN_SIZE_INFO,
	EN_PQ_METATAG_BBD_INFO,
	EN_PQ_METATAG_PQU_BBD_INFO,
	EN_PQ_METATAG_STREAM_INFO,
	EN_PQ_METATAG_DISPLAY_FLOW_INFO,
	EN_PQ_METATAG_INPUT_QUEUE_EXT_INFO,
	EN_PQ_METATAG_OUTPUT_QUEUE_EXT_INFO,
	EN_PQ_METATAG_PQU_QUEUE_EXT_INFO,
	EN_PQ_METATAG_OUTPUT_DISP_IDR_CTRL,
	EN_PQ_METATAG_DV_DEBUG,
	EN_PQ_METATAG_PQPARAM_TAG,
	EN_PQ_METATAG_PQU_PQPARAM_TAG,
	EN_PQ_METATAG_DISPLAY_WM_INFO,
	EN_PQ_METATAG_PQU_DISP_WM_INFO,
	EN_PQ_METATAG_DV_INFO,
	EN_PQ_METATAG_SRCCAP_DV_HDMI_INFO,
	EN_PQ_METATAG_VDEC_DV_DESCRB_INFO,
	EN_PQ_METATAG_PQU_THERMAL_INFO,
	EN_PQ_METATAG_MAX,
};

struct v4l2_common_info {
	__u32 input_source;
	__u32 op2_diff;
	__u32 diff_count;
	enum mtk_pq_input_mode input_mode;
	enum mtk_pq_output_mode output_mode;
	struct v4l2_timing timing_in;
	struct v4l2_format format_cap;
	struct v4l2_format format_out;
	bool v_flip;
	/*
	 * note: following variable is used to identify T32 or M6 flow.
	 * Should be remove in future.
	 */
	bool temp_M6;

};
struct mtk_pq_dma_buf;
struct mtk_pq_buffer;
struct mtk_pq_device;
struct mtk_pq_platform_device;

extern bool bPquEnable;

/* function*/
int mtk_pq_common_trigger_gen_init(bool stub);
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
int mtk_pq_register_common_subdev(struct v4l2_device *pq_dev,
		struct v4l2_subdev *subdev_common,
		struct v4l2_ctrl_handler *common_ctrl_handler);
void mtk_pq_unregister_common_subdev(struct v4l2_subdev *subdev_common);
int mtk_pq_common_read_metadata(int meta_fd,
	enum mtk_pq_common_metatag meta_tag, void *meta_content);
int mtk_pq_common_read_metadata_db(struct dma_buf *meta_db,
	enum mtk_pq_common_metatag meta_tag, void *meta_content);
int mtk_pq_common_read_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_pq_common_metatag meta_tag, void *meta_content);
int mtk_pq_common_write_metadata_db(struct dma_buf *meta_db,
	enum mtk_pq_common_metatag meta_tag, void *meta_content);
int mtk_pq_common_write_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_pq_common_metatag meta_tag, void *meta_content);
int mtk_pq_common_delete_metadata(int meta_fd,
	enum mtk_pq_common_metatag meta_tag);
int mtk_pq_common_delete_metadata_db(struct dma_buf *meta_db,
	enum mtk_pq_common_metatag meta_tag);
int mtk_pq_common_delete_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_pq_common_metatag meta_tag);
int mtk_pq_common_get_dma_buf(struct mtk_pq_device *pq_dev,
	struct mtk_pq_dma_buf *buf);
int mtk_pq_common_put_dma_buf(struct mtk_pq_dma_buf *buf);
int mtk_pq_common_open(struct mtk_pq_device *pq_dev,
	struct msg_new_win_info *msg_info);
int mtk_pq_common_stream_on(struct file *file, int type,
	struct mtk_pq_device *pq_dev,
	struct msg_stream_on_info *msg_info);
int mtk_pq_common_stream_off(struct mtk_pq_device *pq_dev, int type,
	struct msg_stream_off_info *msg_info);
int mtk_pq_common_qbuf(struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf,
	struct msg_queue_info *msg_info);
int mtk_pq_common_close(struct mtk_pq_device *pq_dev,
	struct msg_destroy_win_info *msg_info);
int mtk_pq_common_config(struct msg_config_info *msg_info, bool is_pqu);
int mtk_pq_common_suspend(bool stub);
int mtk_pq_common_resume(bool stub, uint32_t u32IRQ_Version);

#endif
