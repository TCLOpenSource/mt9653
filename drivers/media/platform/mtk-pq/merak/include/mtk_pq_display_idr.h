/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_DISPLAY_IDR_H
#define _MTK_PQ_DISPLAY_IDR_H

#include <linux/types.h>

#include "m_pqu_pq.h"

//-----------------------------------------------------------------------------
//  Driver Capability
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Macro and Define
//-----------------------------------------------------------------------------
#define MAX_PLANE_NUM	(4)
#define MAX_FPLANE_NUM	(MAX_PLANE_NUM - 1)	// exclude metadata plane

//-----------------------------------------------------------------------------
//  Enum
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Structure
//-----------------------------------------------------------------------------
struct pq_display_idr_info {
	bool first_frame;
	struct buffer_info fbuf[MAX_FPLANE_NUM];
	enum mtk_pq_input_mode input_mode;
	uint16_t index;	/* force read index */
	int32_t frame_fd;
	struct v4l2_rect crop;
	uint32_t mem_fmt;
	uint32_t width;
	uint32_t height;
	uint8_t plane_num;
	uint8_t buffer_num;
	uint16_t inputHTT;
	uint16_t inputVTT;
	uint8_t field; // 1:top 2:bottom
};


//-----------------------------------------------------------------------------
//  Function and Variable
//-----------------------------------------------------------------------------
int mtk_pq_display_idr_queue_setup(struct vb2_queue *vq,
	unsigned int *num_buffers, unsigned int *num_planes,
	unsigned int sizes[], struct device *alloc_devs[]);
int mtk_pq_display_idr_queue_setup_info(struct mtk_pq_device *pq_dev,
	void *bufferctrl);
int mtk_pq_display_idr_set_pix_format(
	struct v4l2_format *fmt,
	struct pq_display_idr_info *idr_info);
int mtk_pq_display_idr_set_pix_format_mplane(
	struct v4l2_format *fmt,
	struct mtk_pq_device *pq_dev);
int mtk_pq_display_idr_set_pix_format_mplane_info(
	struct mtk_pq_device *pq_dev,
	void *bufferctrl);
int mtk_pq_display_idr_streamon(struct pq_display_idr_info *idr_info);
int mtk_pq_display_idr_streamoff(struct pq_display_idr_info *idr_info);
int mtk_pq_display_idr_s_crop(struct pq_display_idr_info *idr_info, const struct v4l2_selection *s);
int mtk_pq_display_idr_g_crop(struct pq_display_idr_info *idr_info, struct v4l2_selection *s);
int mtk_pq_display_idr_qbuf(
	struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf);
void mtk_pq_display_idr_qbuf_done(u8 win_id, __u64 meta_db_ptr);

#endif
