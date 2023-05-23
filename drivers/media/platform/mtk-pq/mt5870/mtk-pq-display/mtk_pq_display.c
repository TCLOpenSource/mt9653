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
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <uapi/linux/mtk-v4l2-pq.h>

#include "mtk_pq.h"
#include "mtk_pq_display_manager.h"


//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------
static int _display_g_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	struct mtk_pq_device *pq_dev = container_of(ctrl->handler,
	    struct mtk_pq_device, display_ctrl_handler);

	if (pq_dev->dev_indx >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", pq_dev->dev_indx);
		return -EINVAL;
	}

	int ret;

	switch (ctrl->id) {
	case V4L2_CID_GET_DISPLAY_DATA:
		ret = mtk_display_get_data((__u32) pq_dev->dev_indx,
		    (void *)ctrl->p_new.p_u8);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int _display_s_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	struct mtk_pq_device *pq_dev =
	    container_of(ctrl->handler, struct mtk_pq_device,
	    display_ctrl_handler);

	if (pq_dev->dev_indx >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", pq_dev->dev_indx);
		return -EINVAL;
	}

	int ret;

	switch (ctrl->id) {
	case V4L2_CID_SET_DISPLAY_DATA:
		ret = mtk_display_set_data((__u32) pq_dev->dev_indx,
		    (void *)ctrl->p_new.p_u8);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops display_ctrl_ops = {
	.g_volatile_ctrl = _display_g_ctrl,
	.s_ctrl = _display_s_ctrl,
};

static const struct v4l2_ctrl_config display_ctrl[] = {
	{
	 .ops = &display_ctrl_ops,
	 .id = V4L2_CID_SET_DISPLAY_DATA,
	 .name = "set display manager data",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(struct mtk_pq_display_s_data)},
	 },
	{
	 .ops = &display_ctrl_ops,
	 .id = V4L2_CID_GET_DISPLAY_DATA,
	 .name = "get display manager data",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(struct mtk_pq_display_g_data)},
	 },
};

/* subdev operations */
static const struct v4l2_subdev_ops display_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops display_sd_internal_ops = {
};


//-----------------------------------------------------------------------------
//  FUNCTION
//-----------------------------------------------------------------------------
int mtk_pq_register_display_subdev(struct v4l2_device *pq_dev,
			struct v4l2_subdev *subdev_display,
			struct v4l2_ctrl_handler *display_ctrl_handler)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

	int ret = 0;
	__u32 ctrl_count;
	__u32 ctrl_num = sizeof(display_ctrl) / sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(display_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++)
		v4l2_ctrl_new_custom(display_ctrl_handler,
		    &display_ctrl[ctrl_count], NULL);

	ret = display_ctrl_handler->error;
	if (ret) {
		v4l2_err(pq_dev, "failed to create display ctrl handler\n");
		goto exit;
	}
	subdev_display->ctrl_handler = display_ctrl_handler;

	v4l2_subdev_init(subdev_display, &display_sd_ops);
	subdev_display->internal_ops = &display_sd_internal_ops;
	strlcpy(subdev_display->name, "mtk-display",
	    sizeof(subdev_display->name));

	ret = v4l2_device_register_subdev(pq_dev, subdev_display);
	if (ret) {
		v4l2_err(pq_dev, "failed to register display subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(display_ctrl_handler);
	return ret;
}

void mtk_pq_unregister_display_subdev(struct v4l2_subdev *subdev_display)
{
	v4l2_ctrl_handler_free(subdev_display->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_display);
}
