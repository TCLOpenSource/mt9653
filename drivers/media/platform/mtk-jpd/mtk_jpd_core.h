/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _MTK_JPD_CORE_H
#define _MTK_JPD_CORE_H

#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>

#define MTK_JPD_NAME "mtk-jpd"
#define MTK_MAX_CTRLS_HINT      100

enum mtk_jpd_ctx_state {
	MTK_JPD_STATE_FREE = 0,
	MTK_JPD_STATE_INIT = 1,
	MTK_JPD_STATE_HEADER = 2,
	MTK_JPD_STATE_FLUSH = 3,
	MTK_JPD_STATE_ABORT = 4,
};

struct mtk_jpd_dev {
	// device related
	struct v4l2_device v4l2_dev;
	struct video_device *video_dev;
	struct device *dev;

	// v4l2 internal
	struct v4l2_m2m_dev *m2m_dev;
	struct mutex dev_mutex;

	// hardware related
	void __iomem *jpd_reg_base;
	void __iomem *jpd_ext_reg_base;
	struct clk *clk_njpd;
	struct clk *clk_smi2jpd;
	struct clk *clk_njpd2jpd;
	int jpd_irq;

	unsigned long id_counter;
};

struct mtk_jpd_q_data {
	u32 fourcc;
	u32 width;
	u32 height;
	u32 sizeimage;
	struct v4l2_rect crop;
};

struct mtk_jpd_buf {
	struct vb2_v4l2_buffer vvb;
	struct list_head list;

	void *driver_priv;
	struct mtk_jpd_q_data decoded_q;
	struct mtk_jpd_q_data display_q;
	u32 flags;
	bool lastframe;
};

struct mtk_jpd_ctx {
	struct mtk_jpd_dev *jpd_dev;
	struct v4l2_fh fh;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct v4l2_ctrl_handler ctrl_handler;

	bool active;
	int id;
	int platform_handle;
	enum mtk_jpd_ctx_state state;
	struct mtk_jpd_q_data out_q;
	struct mtk_jpd_q_data cap_q;
	struct mtk_jpd_q_data last_decoded_q;
	struct mtk_jpd_q_data last_display_q;
	spinlock_t q_data_lock;
	struct mtk_jpd_buf *fake_output_buf;

	wait_queue_head_t drain_wq;
	bool draining;
};

#endif				/* _MTK_JPD_CORE_H */
