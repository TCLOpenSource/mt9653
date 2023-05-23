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
#include "mtk_srccap_dscl.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int mtk_dscl_setting(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_dscl_size size)
{
	return 0;
}

static int _dscl_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler,
		struct mtk_srccap_dev, dscl_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _dscl_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler,
		struct mtk_srccap_dev, dscl_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_DSCL_SETTING:
	{
		if (ctrl->p_new.p == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			return -EINVAL;
		}

		struct v4l2_dscl_size size =
			*(struct v4l2_dscl_size *)(ctrl->p_new.p);
		pr_err("[SRCCAP][%s]dscl:(%lu,%lu,%lu,%lu)\n",
			__func__,
			(size.input.width),
			(size.input.height),
			(size.output.width),
			(size.output.height));
		SRCCAP_MSG_DEBUG("V4L2_CID_DSCL_SETTING\n");

		ret = mtk_dscl_setting(srccap_dev, size);
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops dscl_ctrl_ops = {
	.g_volatile_ctrl = _dscl_g_ctrl,
	.s_ctrl = _dscl_s_ctrl,
};

static const struct v4l2_ctrl_config dscl_ctrl[] = {
	{
		.ops = &dscl_ctrl_ops,
		.id = V4L2_CID_DSCL_SETTING,
		.name = "Srccap DSCL Setting",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_dscl_size)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops dscl_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops dscl_sd_internal_ops = {
};

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_srccap_register_dscl_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_dscl,
	struct v4l2_ctrl_handler *dscl_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(dscl_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(dscl_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(dscl_ctrl_handler,
			&dscl_ctrl[ctrl_count], NULL);
	}

	ret = dscl_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create dscl ctrl handler\n");
		goto exit;
	}
	subdev_dscl->ctrl_handler = dscl_ctrl_handler;

	v4l2_subdev_init(subdev_dscl, &dscl_sd_ops);
	subdev_dscl->internal_ops = &dscl_sd_internal_ops;
	strlcpy(subdev_dscl->name, "mtk-dscl", sizeof(subdev_dscl->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_dscl);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register dscl subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(dscl_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_dscl_subdev(struct v4l2_subdev *subdev_dscl)
{
	v4l2_ctrl_handler_free(subdev_dscl->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_dscl);
}
