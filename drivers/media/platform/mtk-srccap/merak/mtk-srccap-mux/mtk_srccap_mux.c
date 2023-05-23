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
#include <linux/clk-provider.h>
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
#include <asm-generic/div64.h>

#include "mtk_srccap.h"
#include "mtk_srccap_mux.h"
#include "hwreg_srccap_mux.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* --------------------------------------- v4l2_ctrl_ops ---------------------------------------- */
/* ============================================================================================== */
static int _mux_g_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, mux_ctrl_handler);
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

static int _mux_s_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, mux_ctrl_handler);
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

static const struct v4l2_ctrl_ops mux_ctrl_ops = {
	.g_volatile_ctrl = _mux_g_ctrl,
	.s_ctrl = _mux_s_ctrl,
};

static const struct v4l2_ctrl_config mux_ctrl[] = {
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
int mtk_srccap_mux_set_source(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	enum reg_srccap_mux_source src;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (srccap_dev->srccap_info.src) {
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		src = REG_SRCCAP_MUX_SOURCE_ADCA;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		src = REG_SRCCAP_MUX_SOURCE_VD;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
		src = REG_SRCCAP_MUX_SOURCE_HDMI;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
		src = REG_SRCCAP_MUX_SOURCE_HDMI2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
		src = REG_SRCCAP_MUX_SOURCE_HDMI3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		src = REG_SRCCAP_MUX_SOURCE_HDMI4;
		break;
	default:
		SRCCAP_MSG_ERROR("Set source fail, not support source: (%d)\n",
			srccap_dev->srccap_info.src);
		return -EINVAL;
	}

	ret = drv_hwreg_srccap_mux_set_source(srccap_dev->dev_id, src, true, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

int mtk_srccap_register_mux_subdev(struct v4l2_device *v4l2_dev,
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
		SRCCAP_MSG_ERROR("failed to create mux ctrl handler\n");
		goto exit;
	}
	subdev_mux->ctrl_handler = mux_ctrl_handler;

	v4l2_subdev_init(subdev_mux, &mux_sd_ops);
	subdev_mux->internal_ops = &mux_sd_internal_ops;
	strlcpy(subdev_mux->name, "mtk-mux", sizeof(subdev_mux->name));

	ret = v4l2_device_register_subdev(v4l2_dev, subdev_mux);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to register mux subdev\n");
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

