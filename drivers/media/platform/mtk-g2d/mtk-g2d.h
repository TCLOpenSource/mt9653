/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>

#define G2D_NAME "mtk-g2d"
#define G2D_TIMEOUT		500
#define SYS_SIZE		255
#define DEV_NUM_NONE		(0xff)

struct g2d_dev {
	struct v4l2_device v4l2_dev;
	struct v4l2_m2m_dev *m2m_dev;
	struct video_device *vfd;
	struct mutex mutex;
	spinlock_t ctrl_lock;
	atomic_t num_inst;
	void __iomem *regs;
	struct clk *clk;
	struct clk *gate;
	struct g2d_ctx *curr;
	int irq;
	wait_queue_head_t irq_queue;
};

struct g2d_frame {
	u32 width;
	u32 height;
	u32 fmt;
	u32 size;
	u32 pitch;
	u64 addr;
};

struct g2d_ctx {
	struct v4l2_fh fh;
	struct g2d_dev *dev;
	struct g2d_frame in;
	struct g2d_frame out;
	struct v4l2_ctrl *ctrl_hflip;
	struct v4l2_ctrl *ctrl_vflip;
	struct v4l2_ctrl *ctrl_rotate;
	struct v4l2_ctrl *ctrl_alpha;
	struct v4l2_ctrl_handler ctrl_handler;
	u32 HMirror;
	u32 VMirror;
};
