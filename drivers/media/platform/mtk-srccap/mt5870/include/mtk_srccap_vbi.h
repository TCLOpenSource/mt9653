/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_VBI_H
#define MTK_SRCCAP_VBI_H

struct srccap_vbi_info {
};


int mtk_srccap_register_vbi_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_vbi,
	struct v4l2_ctrl_handler *vbi_ctrl_handler);
void mtk_srccap_unregister_vbi_subdev(struct v4l2_subdev *subdev_vbi);

#endif  // MTK_SRCCAP_VBI_H

