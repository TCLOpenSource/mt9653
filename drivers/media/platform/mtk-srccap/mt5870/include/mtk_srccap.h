/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_H
#define MTK_SRCCAP_H

#include <linux/platform_device.h>
#include <linux/mtk-v4l2-srccap.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include "drvXC_HDMI_if.h"
#include "mtk_srccap_common.h"
#include "mtk_srccap_hdmirx.h"
#include "mtk_srccap_adc.h"
#include "mtk_srccap_avd.h"
#include "mtk_srccap_mux.h"
#include "mtk_srccap_offlinedetect.h"
#include "mtk_srccap_timingdetect.h"
#include "mtk_srccap_dscl.h"
#include "mtk_srccap_memout.h"
#include "mtk_srccap_vbi.h"
#include "hwreg_srccap_memout.h"

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
#define SYMBOL_WEAK __attribute__((weak))
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#define MTK_SRCCAP_NAME "mtk_srccap"
#define SRCCAP_LABEL "[SRCCAP] "
#define SRCCAP_DEV_NUM (2)
#define SRCCAP_VIDEO_DEV_NUM_BASE (3)
#define SRCCAP_EVENTQ_SIZE (8)
#define SRCCAP_MAX_PLANE_NUM (3)
#define SRCCAP_NSEC_PER_USEC (1000)
#define SRCCAP_BYTE_SIZE (8)

#define T32_HDMI_SIMULATION 1
#define T32_AVD_SIMULATION (1)

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */
#define SRCCAP_MSG_DEBUG(format, args...) \
			pr_debug(SRCCAP_LABEL "[%s,%d]" format, __func__, __LINE__, ##args)
#define SRCCAP_MSG_INFO(format, args...) \
			pr_info(SRCCAP_LABEL "[%s,%d]" format, __func__, __LINE__, ##args)
#define SRCCAP_MSG_WARNING(format, args...) \
			pr_warn(SRCCAP_LABEL "[%s,%d]" format, __func__, __LINE__, ##args)
#define SRCCAP_MSG_ERROR(format, args...) \
			pr_err(SRCCAP_LABEL "[%s,%d]" format, __func__, __LINE__, ##args)
#define SRCCAP_MSG_FATAL(format, args...) \
			pr_crit(SRCCAP_LABEL "[%s,%d]" format, __func__, __LINE__, ##args)

#define SRCCAP_MSG_POINTER_CHECK() \
		do { \
			pr_err(SRCCAP_LABEL"[%s,%d] pointer is NULL\n", __func__, __LINE__); \
			dump_stack(); \
		} while (0)

#define SRCCAP_MSG_RETURN_CHECK(result) \
			pr_err(SRCCAP_LABEL"[%s,%d] return is %d\n", __func__, __LINE__, (result))

#define SRCCAP_GET_TIMESTAMP(tv) \
		do { \
			struct timespec ts; \
			ktime_get_ts(&ts); \
			((struct timeval *)tv)->tv_sec = ts.tv_sec; \
			((struct timeval *)tv)->tv_usec = ts.tv_nsec / SRCCAP_NSEC_PER_USEC; \
		} while (0)

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum srccap_access_type {
	SRCCAP_SET = 0,
	SRCCAP_GET,
};

enum srccap_input_port {
	SRCCAP_INPUT_PORT_NONE = 0,
	SRCCAP_INPUT_PORT_RGB0_DATA = 10,
	SRCCAP_INPUT_PORT_RGB1_DATA,
	SRCCAP_INPUT_PORT_RGB2_DATA,
	SRCCAP_INPUT_PORT_RGB3_DATA,
	SRCCAP_INPUT_PORT_RGB4_DATA,
	SRCCAP_INPUT_PORT_RGB0_SYNC = 30,
	SRCCAP_INPUT_PORT_RGB1_SYNC,
	SRCCAP_INPUT_PORT_RGB2_SYNC,
	SRCCAP_INPUT_PORT_RGB3_SYNC,
	SRCCAP_INPUT_PORT_RGB4_SYNC,
	SRCCAP_INPUT_PORT_YCVBS0 = 50,
	SRCCAP_INPUT_PORT_YCVBS1,
	SRCCAP_INPUT_PORT_YCVBS2,
	SRCCAP_INPUT_PORT_YCVBS3,
	SRCCAP_INPUT_PORT_YCVBS4,
	SRCCAP_INPUT_PORT_YCVBS5,
	SRCCAP_INPUT_PORT_YCVBS6,
	SRCCAP_INPUT_PORT_YCVBS7,
	SRCCAP_INPUT_PORT_YG0 = 70,
	SRCCAP_INPUT_PORT_YG1,
	SRCCAP_INPUT_PORT_YG2,
	SRCCAP_INPUT_PORT_YR0,
	SRCCAP_INPUT_PORT_YR1,
	SRCCAP_INPUT_PORT_YR2,
	SRCCAP_INPUT_PORT_YB0,
	SRCCAP_INPUT_PORT_YB1,
	SRCCAP_INPUT_PORT_YB2,
	SRCCAP_INPUT_PORT_CCVBS0 = 90,
	SRCCAP_INPUT_PORT_CCVBS1,
	SRCCAP_INPUT_PORT_CCVBS2,
	SRCCAP_INPUT_PORT_CCVBS3,
	SRCCAP_INPUT_PORT_CCVBS4,
	SRCCAP_INPUT_PORT_CCVBS5,
	SRCCAP_INPUT_PORT_CCVBS6,
	SRCCAP_INPUT_PORT_CCVBS7,
	SRCCAP_INPUT_PORT_CR0 = 110,
	SRCCAP_INPUT_PORT_CR1,
	SRCCAP_INPUT_PORT_CR2,
	SRCCAP_INPUT_PORT_CG0,
	SRCCAP_INPUT_PORT_CG1,
	SRCCAP_INPUT_PORT_CG2,
	SRCCAP_INPUT_PORT_CB0,
	SRCCAP_INPUT_PORT_CB1,
	SRCCAP_INPUT_PORT_CB2,
	SRCCAP_INPUT_PORT_DVI0 = 130,
	SRCCAP_INPUT_PORT_DVI1,
	SRCCAP_INPUT_PORT_DVI2,
	SRCCAP_INPUT_PORT_DVI3,
	SRCCAP_INPUT_PORT_NUM,
};

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct srccap_capability {
	/* external use */
	u8 ipdma_cnt;
	u8 hdmi_cnt;
	u8 cvbs_cnt;
	u8 svideo_cnt;
	u8 ypbpr_cnt;
	u8 vga_cnt;
	u8 atv_cnt;
	/* internal use */
	u32 bpw;
};

struct srccap_port_map {
	enum srccap_input_port data_port;
	enum srccap_input_port sync_port;
};

struct srccap_info {
	enum v4l2_srccap_input_source src;
	struct srccap_capability srccap_cap;
	struct srccap_port_map map[V4L2_SRCCAP_INPUT_SOURCE_NUM];
	struct task_struct *timing_monitor_task;
};

struct mtk_srccap_dev {
	struct device *dev;
	struct v4l2_device v4l2_dev;
	struct v4l2_m2m_dev *mdev;
	struct video_device *vdev;
	u16 dev_id;

	struct v4l2_subdev subdev_common;
	struct v4l2_subdev subdev_hdmirx;
	struct v4l2_subdev subdev_adc;
	struct v4l2_subdev subdev_avd;
	struct v4l2_subdev subdev_mux;
	struct v4l2_subdev subdev_offlinedetect;
	struct v4l2_subdev subdev_timingdetect;
	struct v4l2_subdev subdev_dscl;
	struct v4l2_subdev subdev_memout;
	struct v4l2_subdev subdev_vbi;

	struct v4l2_ctrl_handler srccap_ctrl_handler;
	struct v4l2_ctrl_handler common_ctrl_handler;
	struct v4l2_ctrl_handler hdmirx_ctrl_handler;
	struct v4l2_ctrl_handler adc_ctrl_handler;
	struct v4l2_ctrl_handler avd_ctrl_handler;
	struct v4l2_ctrl_handler mux_ctrl_handler;
	struct v4l2_ctrl_handler offlinedetect_ctrl_handler;
	struct v4l2_ctrl_handler timingdetect_ctrl_handler;
	struct v4l2_ctrl_handler dscl_ctrl_handler;
	struct v4l2_ctrl_handler memout_ctrl_handler;
	struct v4l2_ctrl_handler vbi_ctrl_handler;

	struct srccap_info srccap_info;
	struct srccap_common_info common_info;
	struct srccap_hdmirx_info hdmirx_info;
	struct srccap_adc_info adc_info;
	struct srccap_avd_info avd_info;
	struct srccap_mux_info mux_info;
	struct srccap_offlinedetect_info offlinedetect_info;
	struct srccap_timingdetect_info timingdetect_info;
	struct srccap_dscl_info dscl_info;
	struct srccap_memout_info memout_info;
	struct srccap_vbi_info vbi_info;

	struct mutex mutex;
	struct clk *clk;
	spinlock_t lock;

	int irq_hdcp;
	int irq_scdc;
	int irq_vbi;
};

struct mtk_srccap_ctx {
	struct v4l2_fh fh;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct mtk_srccap_dev *srccap_dev;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */

#endif  // MTK_SRCCAP_H
