/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_BUFFER_H
#define _MTK_PQ_BUFFER_H

#include <linux/types.h>
#include <linux/videodev2.h>

#define free(buf) kfree(buf)
#define malloc(size) kzalloc((size), GFP_KERNEL)

enum pq_buffer_attribute {
	PQ_BUF_ATTR_NS = 0, // non secure buffer
	PQ_BUF_ATTR_S, // secure buffer
	PQ_BUF_ATTR_MAX,
};

enum pq_buffer_type {
	PQ_BUF_SCMI = 0,
	PQ_BUF_ZNR,
	PQ_BUF_UCM,
	PQ_BUF_ABF,
	PQ_BUF_MAX,
};

struct pq_buffer {
	bool valid;
	__u32 size;
	__u64 addr[PQ_BUF_ATTR_MAX]; // by HW
	__u64 va[PQ_BUF_ATTR_MAX]; // by CPU
};

struct mtk_pq_device;
struct v4l2_xc_info;

/* function */
int mtk_pq_buf_get_iommu_idx(struct mtk_pq_device *pq_dev,
	enum pq_buffer_type buf_type);
int mtk_pq_buffer_win_init(struct v4l2_xc_info *xc_info);
void mtk_pq_buffer_win_exit(struct v4l2_xc_info *xc_info);
int mtk_pq_buffer_buf_init(struct mtk_pq_device *pq_dev);
void mtk_pq_buffer_buf_exit(struct mtk_pq_device *pq_dev);
int mtk_pq_buffer_allocate(struct mtk_pq_device *pq_dev, __s32 BufEn);
int mtk_get_pq_buffer(struct mtk_pq_device *pq_dev,
	enum pq_buffer_type buftype, struct pq_buffer *pq_buf);
int mtk_xc_get_buf_info(
	struct v4l2_pq_buf buf,
	struct pq_buffer *fbuf,
	struct pq_buffer *mbuf);
#endif
