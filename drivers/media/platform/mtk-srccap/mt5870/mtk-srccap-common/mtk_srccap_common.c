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
#include <linux/delay.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include "mtk_srccap.h"
#include "mtk_srccap_common.h"
#include "mtk_srccap_common_drv.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int vsync_monitor_task(void *data)
{
	if (data == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;

	while (video_is_registered(srccap_dev->vdev)) {
		memset(&event, 0, sizeof(struct v4l2_event));
		event.type = V4L2_EVENT_VSYNC;
		event.id = 0;
		v4l2_event_queue(srccap_dev->vdev, &event);
		usleep_range(10000, 11000); // sleep 10ms

		if (kthread_should_stop()) {
			pr_err("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int _common_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
			struct mtk_srccap_dev, common_ctrl_handler);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_OUTPUT_EXT_INFO:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_OUTPUT_EXT_INFO\n");
		ret = mtk_srccap_drv_get_output_info(
		(struct v4l2_output_ext_info *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _common_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(
		ctrl->handler, struct mtk_srccap_dev, common_ctrl_handler);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_SRCCAP_TESTPATTERN:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_SRCCAP_TESTPATTERN\n");
		ret = mtk_srccap_drv_testpattern(
		(struct v4l2_srccap_testpattern *)
					ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_OUTPUT_EXT_INFO:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_OUTPUT_EXT_INFO\n");
		ret = mtk_srccap_drv_set_output_info(
		(struct v4l2_output_ext_info *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops common_ctrl_ops = {
	.g_volatile_ctrl = _common_g_ctrl,
	.s_ctrl = _common_s_ctrl,
};

static const struct v4l2_ctrl_config common_ctrl[] = {
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SRCCAP_TESTPATTERN,
		.name = "Srccap Test Pattern",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_testpattern)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_OUTPUT_EXT_INFO,
		.name = "Get Output Timing Info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_output_ext_info)},
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops common_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops common_sd_internal_ops = {
};

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_common_subscribe_event_vsync(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev->common_info.vsync_monitor_task == NULL) {
		srccap_dev->common_info.vsync_monitor_task = kthread_create(
			vsync_monitor_task,
			srccap_dev,
			"vsync_monitor_task");
		if (srccap_dev->common_info.vsync_monitor_task == ERR_PTR(-ENOMEM))
			return -ENOMEM;
		wake_up_process(srccap_dev->common_info.vsync_monitor_task);
	}

	return 0;
}

int mtk_common_unsubscribe_event_vsync(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev->common_info.vsync_monitor_task != NULL) {
		kthread_stop(srccap_dev->common_info.vsync_monitor_task);
		srccap_dev->common_info.vsync_monitor_task = NULL;
	}

	return 0;
}

int mtk_srccap_register_common_subdev(
	struct v4l2_device *srccap_dev, struct v4l2_subdev *subdev_common,
	struct v4l2_ctrl_handler *common_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(common_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(common_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(common_ctrl_handler,
					&common_ctrl[ctrl_count], NULL);
	}

	ret = common_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create common ctrl handler\n");
		goto exit;
	}
	subdev_common->ctrl_handler = common_ctrl_handler;

	v4l2_subdev_init(subdev_common, &common_sd_ops);
	subdev_common->internal_ops = &common_sd_internal_ops;
	strlcpy(subdev_common->name, "mtk-common",
					sizeof(subdev_common->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_common);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register common subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(common_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_common_subdev(struct v4l2_subdev *subdev_common)
{
	v4l2_ctrl_handler_free(subdev_common->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_common);
}

