// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include "mtk_srccap.h"
#include "mtk_srccap_timingdetect.h"
#include "mtk_srccap_timingdetect_drv.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int _timingdetect_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
			struct mtk_srccap_dev, timingdetect_ctrl_handler);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_TIMINGDETECT_AUTO_NOSIGNAL:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_TIMINGDETECT_AUTO_NOSIGNAL\n");
		ret = mtk_srccap_drv_auto_no_signal((bool *)&(ctrl->val),
						SRCCAP_GET, srccap_dev);
		break;
	}
	case V4L2_CID_TIMINGDETECT_CURRENT_STATE:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_CURRENT_STATE\n");
		ret = mtk_srccap_drv_get_current_pcm_status(
			(enum v4l2_srccap_pcm_status *)&(ctrl->val),
							srccap_dev);
		break;
	}
	case V4L2_CID_TIMINGDETECT_HDMI_SYNC_MODE:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_HDMI_SYNC_MODE\n");
		ret = mtk_srccap_drv_get_hdmi_sync_mode(
			(enum v4l2_srccap_hdmi_sync_mode *)&(ctrl->val));
		break;
	}
	case V4L2_CID_TIMINGDETECT_MONITOR:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_TIMINGDETECT_MONITOR\n");
		ret = mtk_srccap_drv_get_pcm(
			(struct v4l2_srccap_pcm *)ctrl->p_new.p_u8,
			srccap_dev);
		break;
	}
	case V4L2_CID_TIMINGDETECT_INVALID_TIMING_DETECT:
	{
		SRCCAP_MSG_DEBUG("Get\n"
			"V4L2_CID_TIMINGDETECT_INVALID_TIMING_DETECT\n");
		ret = mtk_srccap_drv_get_invalid_timing_detect(
		(struct v4l2_srccap_pcm_invalid_timing_detect *)ctrl
							->p_new.p_u8,
		srccap_dev);
		break;
	}
	case V4L2_CID_TIMINGDETECT_UPDATE_SETTING:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_UPDATE_SETTING\n");
		ret = mtk_srccap_drv_update_setting(
			(struct v4l2_srccap_mp_input_info *)ctrl->p_new.p_u8,
			srccap_dev);
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _timingdetect_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
			struct mtk_srccap_dev, timingdetect_ctrl_handler);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_TIMINGDETECT_INIT:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_INIT\n");
		ret = mtk_srccap_drv_detect_timing_pcm_init();
		break;
	}
	case V4L2_CID_TIMINGDETECT_RESTART:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_RESTART\n");
		ret = mtk_srccap_drv_detect_timing_restart(srccap_dev);
		break;
	}
	case V4L2_CID_TIMINGDETECT_AUTO_NOSIGNAL:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_TIMINGDETECT_AUTO_NOSIGNAL\n"
					": %d\n", ctrl->val);
		ret = mtk_srccap_drv_auto_no_signal(
			(bool *)&(ctrl->val), SRCCAP_SET, srccap_dev);
		break;
	}
	case V4L2_CID_TIMINGDETECT_COUNT:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_COUNT\n");
		ret = mtk_srccap_drv_set_detect_timing_counter(
		(struct v4l2_srccap_pcm_timing_count *)ctrl->p_new.p_u8);
		break;
	}
	case V4L2_CID_TIMINGDETECT_MONITOR:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_TIMINGDETECT_MONITOR\n");
		ret = mtk_srccap_drv_set_pcm(
			(struct v4l2_srccap_pcm *)ctrl->p_new.p_u8,
			srccap_dev);
		break;
	}
	case V4L2_CID_TIMINGDETECT_INVALID_TIMING_DETECT:
	{
		SRCCAP_MSG_DEBUG("Set\n"
			"V4L2_CID_TIMINGDETECT_INVALID_TIMING_DETECT\n");
		ret = mtk_srccap_drv_set_invalid_timing_detect(
		(struct v4l2_srccap_pcm_invalid_timing_detect *)ctrl
							->p_new.p_u8,
		srccap_dev);
		break;
	}
	case V4L2_CID_TIMINGDETECT_TIMING_TABLE:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_TIMING_TABLE\n");
		ret = mtk_srccap_drv_mode_table(
			(struct v4l2_srccap_timing_mode_table *)ctrl
							->p_new.p_u8,
			srccap_dev);
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops timingdetect_ctrl_ops = {
	.g_volatile_ctrl = _timingdetect_g_ctrl,
	.s_ctrl = _timingdetect_s_ctrl,
};

static const struct v4l2_ctrl_config timingdetect_ctrl[] = {
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_INIT,
		.name = "Srccap TimingDetect Init",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_RESTART,
		.name = "Srccap TimingDetect Restart",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_AUTO_NOSIGNAL,
		.name = "Srccap TimingDetect AutoNoSignal",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_CURRENT_STATE,
		.name = "Srccap TimingDetect Current State",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_HDMI_SYNC_MODE,
		.name = "Srccap TimingDetect HDMI Sync Mode",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_COUNT,
		.name = "Srccap TimingDetect Counter",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_pcm_timing_count)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_MONITOR,
		.name = "Srccap TimingDetect Monitor",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_pcm)},
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_INVALID_TIMING_DETECT,
		.name = "Srccap TimingDetect Invalid Timing Detect",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_pcm_invalid_timing_detect)},
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_UPDATE_SETTING,
		.name = "Srccap TimingDetect Update Setting",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_mp_input_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_TIMING_TABLE,
		.name = "Srccap TimingDetect Timing Mode Table",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_timing_mode_table)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops timingdetect_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops timingdetect_sd_internal_ops = {
};

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_srccap_register_timingdetect_subdev(struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_timingdetect,
	struct v4l2_ctrl_handler *timingdetect_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num =
		sizeof(timingdetect_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(timingdetect_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(timingdetect_ctrl_handler,
					&timingdetect_ctrl[ctrl_count], NULL);
	}

	ret = timingdetect_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create timingdetect ctrl\n"
					"handler\n");
		goto exit;
	}
	subdev_timingdetect->ctrl_handler = timingdetect_ctrl_handler;

	v4l2_subdev_init(subdev_timingdetect, &timingdetect_sd_ops);
	subdev_timingdetect->internal_ops = &timingdetect_sd_internal_ops;
	strlcpy(subdev_timingdetect->name, "mtk-timingdetect",
					sizeof(subdev_timingdetect->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_timingdetect);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register timingdetect\n"
					"subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(timingdetect_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_timingdetect_subdev(
			struct v4l2_subdev *subdev_timingdetect)
{
	v4l2_ctrl_handler_free(subdev_timingdetect->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_timingdetect);
}

