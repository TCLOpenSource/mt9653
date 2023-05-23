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
#include <stdbool.h>

#include "mtk_srccap.h"
#include "mtk_srccap_mux.h"
#include "mtk_srccap_mux_drv.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int _mux_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
				struct mtk_srccap_dev, mux_ctrl_handler);
	int ret;

	switch (ctrl->id) {
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _mux_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
				struct mtk_srccap_dev, mux_ctrl_handler);
	int ret;

	switch (ctrl->id) {
	case V4L2_CID_MUX_SKIP_SWRESET:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_MUX_SKIP_SWRESET\n");
		ret = mtk_srccap_drv_mux_skip_swreset((bool)ctrl->val);
		break;
	}
	case V4L2_CID_MUX_DISABLE_INPUT:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_MUX_DISABLE_INPUT\n");
		ret = mtk_srccap_drv_mux_disable_inputsource(
					(bool)ctrl->val, srccap_dev);
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops mux_ctrl_ops = {
	.g_volatile_ctrl = _mux_g_ctrl,
	.s_ctrl = _mux_s_ctrl,
};

static const struct v4l2_ctrl_config mux_ctrl[] = {
	{
		.ops = &mux_ctrl_ops,
		.id = V4L2_CID_MUX_SKIP_SWRESET,
		.name = "Srccap Mux Skip Sw Reset",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = false,
		.max = true,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &mux_ctrl_ops,
		.id = V4L2_CID_MUX_DISABLE_INPUT,
		.name = "Srccap Mux Disable Input Source",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = false,
		.max = true,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops mux_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops mux_sd_internal_ops = {
};

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_srccap_register_mux_subdev(struct v4l2_device *srccap_dev,
			struct v4l2_subdev *subdev_mux,
			struct v4l2_ctrl_handler *mux_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(mux_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(mux_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(mux_ctrl_handler, &mux_ctrl[ctrl_count],
									NULL);
	}

	ret = mux_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create mux ctrl handler\n");
		goto exit;
	}
	subdev_mux->ctrl_handler = mux_ctrl_handler;

	v4l2_subdev_init(subdev_mux, &mux_sd_ops);
	subdev_mux->internal_ops = &mux_sd_internal_ops;
	strlcpy(subdev_mux->name, "mtk-mux", sizeof(subdev_mux->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_mux);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register mux subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(mux_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_mux_subdev(struct v4l2_subdev *subdev_mux)
{
	v4l2_ctrl_handler_free(subdev_mux->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_mux);
}


