/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_H
#define _MTK_PQ_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <uapi/linux/mtk-v4l2-pq.h>
#include <uapi/linux/metadata_utility/m_pq.h>
#include <media/videobuf2-dma-contig.h>

#include "mtk_pq_common.h"
#include "mtk_pq_b2r.h"
#include "mtk_pq_xc.h"
#include "mtk_pq_enhance.h"
#include "mtk_pq_display.h"

#define PQ_WIN_MAX_NUM		16
#define PQ_NEVENTS		8

#define STI_PQ_LOG_LEVEL_ERROR		BIT(0)
#define STI_PQ_LOG_LEVEL_COMMON		BIT(1)
#define STI_PQ_LOG_LEVEL_ENHANCE		BIT(2)
#define STI_PQ_LOG_LEVEL_B2R		BIT(3)
#define STI_PQ_LOG_LEVEL_DISPLAY		BIT(4)
#define STI_PQ_LOG_LEVEL_XC		BIT(5)
#define STI_PQ_LOG_LEVEL_3D		BIT(6)
#define STI_PQ_LOG_LEVEL_BUFFER		BIT(7)
#define STI_PQ_LOG_LEVEL_SVP		BIT(8)
#define STI_PQ_LOG_LEVEL_ALL		(BIT(9) - 1)

extern __u32 u32DbgLevel;

#define STI_PQ_LOG(_dbgLevel_, _fmt, _args...) { \
	if ((_dbgLevel_ & u32DbgLevel) != 0) { \
		if (_dbgLevel_ & STI_PQ_LOG_LEVEL_ERROR) \
			pr_err("[STI PQ ERROR]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_COMMON) { \
			pr_err("[STI PQ COMMON]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_ENHANCE) { \
			pr_err("[STI PQ ENHANCE]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_B2R) { \
			pr_err("[STI PQ B2R]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_DISPLAY) { \
			pr_err("[STI PQ DISPLAY]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_XC) { \
			pr_err("[STI PQ XC]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_3D) { \
			pr_err("[STI PQ 3D]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_BUFFER) { \
			pr_err("[STI PQ BUFFER]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_SVP) {\
			pr_err("[STI PQ SVP]  [%s,%5d]"_fmt,\
			__func__, __LINE__, ##_args);\
		} \
	} \
}

#define PQ_LABEL    "[PQ] "
#define PQ_MSG_DEBUG(format, args...) \
	pr_debug(PQ_LABEL format, ##args)
#define PQ_MSG_INFO(format, args...) \
	pr_info(PQ_LABEL format, ##args)
#define PQ_MSG_WARNING(format, args...) \
	pr_warn(PQ_LABEL format, ##args)
#define PQ_MSG_ERROR(format, args...) \
	pr_err(PQ_LABEL format, ##args)
#define PQ_MSG_FATAL(format, args...) \
	pr_crit(PQ_LABEL format, ##args)
#define PQ_CAPABILITY_SHOW(PQItem, PQItemCap) { \
	if (ssize > u8Size) \
		PQ_MSG_ERROR("Failed to get" PQItem "\n"); \
	else \
		ssize += snprintf(buf + ssize, u8Size - ssize, "%d\n", \
		PQItemCap); \
}

struct v4l2_display_info {
	struct task_struct *mvop_task;
	bool bReleaseTaskCreated;
	bool bCallBackTaskCreated;
};

struct mtk_pq_ctx {
	struct v4l2_fh fh;
	struct mtk_pq_device *pq_dev;
	struct v4l2_m2m_ctx *m2m_ctx;
};

struct mtk_pq_m2m_dev {
	struct video_device *video_dev;
	struct v4l2_m2m_dev *m2m_dev;
	struct mtk_pq_ctx *ctx;
	int refcnt;
};

struct mtk_pq_caps {
	u32 u32TCH_color;
	u32 u32PreSharp;
	u32 u322D_Peaking;
	u32 u32LCE;
	u32 u323D_LUT;
	u32 u32TS_Input;
	u32 u32TS_Output;
	u32 u32DMA_SCL;
	u32 u32PQU;
	u32 u32Window_Num;
};

/*INFO OF PQ DD*/
struct mtk_pq_device {
	spinlock_t slock;
	bool bopened;
	struct mutex mutex;
	struct v4l2_device v4l2_dev;
	struct video_device video_dev;
	struct device *dev;
	struct mtk_pq_m2m_dev m2m;
	struct v4l2_subdev subdev_common;
	struct v4l2_subdev subdev_enhance;
	struct v4l2_subdev subdev_xc;
	struct v4l2_subdev subdev_b2r;
	struct v4l2_subdev subdev_display;

	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl_handler common_ctrl_handler;
	struct v4l2_ctrl_handler enhance_ctrl_handler;
	struct v4l2_ctrl_handler xc_ctrl_handler;
	struct v4l2_ctrl_handler b2r_ctrl_handler;
	struct v4l2_ctrl_handler display_ctrl_handler;

	__u16 dev_indx;    /*pq device ID or window number*/

	struct v4l2_common_info common_info;
	void *enhance_info;	//pointer to struct v4l2_enhance_info
	struct v4l2_b2r_info b2r_info;
	struct v4l2_xc_info xc_info;
	struct v4l2_display_info display_info;

	struct b2r_device b2r_dev;
};

struct mtk_pq_platform_device {
	struct mtk_pq_device *pq_devs[PQ_WIN_MAX_NUM];
	struct device *dev;

	struct b2r_device b2r_dev;
	struct mtk_pq_caps pqcaps;
	struct mtk_pq_enhance pq_enhance;
};

#endif
