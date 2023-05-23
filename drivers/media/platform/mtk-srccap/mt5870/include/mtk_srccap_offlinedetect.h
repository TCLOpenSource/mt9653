/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_OFFLINEDETECT_H
#define MTK_SRCCAP_OFFLINEDETECT_H

struct srccap_offlinedetect_info {
};

int mtk_srccap_register_offlinedetect_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_offlinedetect,
	struct v4l2_ctrl_handler *offlinedetect_ctrl_handler);
void mtk_srccap_unregister_offlinedetect_subdev(
	struct v4l2_subdev *subdev_offlinedetect);

#endif  // MTK_SRCCAP_OFFLINEDETECT_H

