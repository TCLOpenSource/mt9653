/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DSCL_H
#define MTK_SRCCAP_DSCL_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/mtk-v4l2-srccap.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
#define SRCCAP_DSCL_AVERAGE (2)
#define SRCCAP_DSCL_REG_NUM (10)
/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct mtk_srccap_dev;
struct srccap_ml_script_info;

struct srccap_dscl_info {
	bool h_scalingmode;//0:CB; 1: Filter
	bool v_scalingmode;//0:CB; 1:Bilinear
	u64 h_scalingratio;
	u64 v_scalingratio;
	struct v4l2_dscl_size dscl_size;
	struct v4l2_dscl_size user_dscl_size;
	struct v4l2_srccap_pqmap_rm_info rm_info;
	struct v4l2_area_size scaled_size_meta;
	struct v4l2_area_size scaled_size_ml;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_srccap_dscl_set_source(
	struct mtk_srccap_dev *srccap_dev,
	bool frl);
int mtk_srccap_register_dscl_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_dscl,
	struct v4l2_ctrl_handler *dscl_ctrl_handler);
void mtk_srccap_unregister_dscl_subdev(
	struct v4l2_subdev *subdev_dscl);
void mtk_dscl_set_scaling(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_ml_script_info *ml_script_info,
	struct v4l2_dscl_size *dscl_size);
void mtk_dscl_get_scaling_mode(
	struct mtk_srccap_dev *srccap_dev,
	bool *h_filter_mode
	, bool *v_linear_mode,
	struct v4l2_dscl_size *dscl_size);
u32 mtk_dscl_set_ratio_v(u16 src_size, u16 dst_size, bool linear_mode);
u32 mtk_dscl_set_ratio_h(u16 src_size, u16 dst_size, bool filter_mode);
int mtk_dscl_streamoff(struct mtk_srccap_dev *srccap_dev);

#endif  // MTK_SRCCAP_DSCL_H
