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
#include "mtk_srccap_offlinedetect.h"

#include "apiXC.h"

#define OFFLINEDETECT_LABEL	"[OFFLINEDETECT] "
#define OFFLINEDETECT_MSG_DEBUG(format, args...) \
				pr_debug(OFFLINEDETECT_LABEL format, ##args)
#define OFFLINEDETECT_MSG_INFO(format, args...) \
				pr_info(OFFLINEDETECT_LABEL format, ##args)
#define OFFLINEDETECT_MSG_WARNING(format, args...) \
				pr_warn(OFFLINEDETECT_LABEL format, ##args)
#define OFFLINEDETECT_MSG_ERROR(format, args...) \
				pr_err(OFFLINEDETECT_LABEL format, ##args)
#define OFFLINEDETECT_MSG_FATAL(format, args...) \
				pr_crit(OFFLINEDETECT_LABEL format, ##args)

#define SYMBOL_WEAK __attribute__((weak))

static enum v4l2_srccap_input_source _genV4l2OfflineInputSource =
					V4L2_SRCCAP_INPUT_SOURCE_NONE;

static int _mtk_srccap_setOffLineDetection(
			enum v4l2_srccap_input_source enInputSource)
{
	MApi_XC_SetOffLineDetection((INPUT_SOURCE_TYPE_t)enInputSource);
	OFFLINEDETECT_MSG_INFO("set offlinedetection inputsource = %d\n",
							enInputSource);

	return 0;
}

static int _mtk_srccap_getOffLineDetection(
			enum v4l2_srccap_input_source enInputSource,
			__u32 *pu32OfflineStatus)
{
	if (pu32OfflineStatus == NULL) {
		OFFLINEDETECT_MSG_INFO("INVALID PARA.\n");
		return -EPERM;
	}

	*pu32OfflineStatus = MApi_XC_GetOffLineDetection(
					(INPUT_SOURCE_TYPE_t)enInputSource);
	OFFLINEDETECT_MSG_INFO("get offlinedetection status = %d\n",
					*pu32OfflineStatus);

	return 0;
}

static int _offlinedetect_g_ctrl(struct v4l2_ctrl *ctrl)
{
	//struct mtk_srccap_device *srccap_dev =
	//container_of(ctrl->handler, struct mtk_srccap_device, ctrl_handler);
	int ret = 0;

	OFFLINEDETECT_MSG_INFO("%s : %x\r\n", __func__, ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_OFFLINEDETECTION_STATUS:
		ret = _mtk_srccap_getOffLineDetection(
			_genV4l2OfflineInputSource, &(ctrl->val));
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _offlinedetect_s_ctrl(struct v4l2_ctrl *ctrl)
{
	//struct mtk_srccap_device *srccap_dev =
	//container_of(ctrl->handler, struct mtk_srccap_device, ctrl_handler);
	int ret = 0;

	OFFLINEDETECT_MSG_INFO("%s id = %d\n", __func__, ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_OFFLINEDETECTION:
		ret = _mtk_srccap_setOffLineDetection(ctrl->val);
		_genV4l2OfflineInputSource = ctrl->val;
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops offlinedetect_ctrl_ops = {
	.g_volatile_ctrl = _offlinedetect_g_ctrl,
	.s_ctrl = _offlinedetect_s_ctrl,
};

static const struct v4l2_ctrl_config offlinedetect_ctrl[] = {
	{
		.ops = &offlinedetect_ctrl_ops,
		.id = V4L2_CID_OFFLINEDETECTION,
		.name = "offline detection",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 0xff,
		.def = 0,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &offlinedetect_ctrl_ops,
		.id = V4L2_CID_OFFLINEDETECTION_STATUS,
		.name = "offline detection status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 0xff,
		.def = 0,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops offlinedetect_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops offlinedetect_sd_internal_ops = {
};

int mtk_srccap_register_offlinedetect_subdev(
			struct v4l2_device *srccap_dev,
			struct v4l2_subdev *subdev_offlinedetect,
			struct v4l2_ctrl_handler *offlinedetect_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(offlinedetect_ctrl)/sizeof(
						struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(offlinedetect_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(offlinedetect_ctrl_handler,
				&offlinedetect_ctrl[ctrl_count], NULL);
	}

	ret = offlinedetect_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create offlinedetect ctrl\n"
					"handler\n");
		goto exit;
	}
	subdev_offlinedetect->ctrl_handler = offlinedetect_ctrl_handler;

	v4l2_subdev_init(subdev_offlinedetect, &offlinedetect_sd_ops);
	subdev_offlinedetect->internal_ops = &offlinedetect_sd_internal_ops;
	strlcpy(subdev_offlinedetect->name, "mtk-offlinedetect",
					sizeof(subdev_offlinedetect->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_offlinedetect);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register offlinedetect\n"
					"subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(offlinedetect_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_offlinedetect_subdev(
			struct v4l2_subdev *subdev_offlinedetect)
{
	v4l2_ctrl_handler_free(subdev_offlinedetect->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_offlinedetect);
}

