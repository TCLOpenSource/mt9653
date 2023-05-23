/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_DISPLAY_H
#define _MTK_PQ_DISPLAY_H
#include "mtk_pq_display_manager.h"

// function
int mtk_pq_register_display_subdev(
	struct v4l2_device *pq_dev,
	struct v4l2_subdev *subdev_display,
	struct v4l2_ctrl_handler *display_ctrl_handler);
void mtk_pq_unregister_display_subdev(
	struct v4l2_subdev *subdev_display);

#endif
