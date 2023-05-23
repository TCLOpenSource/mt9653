/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_AVD_H
#define MTK_SRCCAP_AVD_H

#include <linux/videodev2.h>
#include <linux/types.h>
#include "mtk_srccap.h"

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
//for avd decompose
#define DECOMPOSE

#ifdef DECOMPOSE
#include "drvAVD.h"
#endif

#define TEST_MODE         0   // 1: for TEST

//check avd status
#define _AVD_DATA_VALID            0x0001UL
#define _AVD_VSYNC_TYPE_PAL        0x1000UL
#define _AVD_IS_HSYNC_LOCK         0x4000UL
#define AVD_STATUS_LOCK (_AVD_DATA_VALID | _AVD_IS_HSYNC_LOCK)

// change TV System
#define _AVD_STABLE_CHECK_COUNT  (20)
#define _AVD_HSYNC_POLLING_TIME  (20)
#define _AVD_CHECK_COUNT         (5)

// check signal status
#define _AVD_TYPE_CHECK_COUNT    (12)
#define _AVD_TYPE_CHECK_TIME     (30)
#define _AVD_STD_STABLE_COUNT     (1)
/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum V4L2_AVD_RESULT {
	V4L2_EXT_AVD_FAIL = -1,
	V4L2_EXT_AVD_OK = 0,
	V4L2_EXT_AVD_NOT_SUPPORT,
	V4L2_EXT_AVD_NOT_IMPLEMENT,
};

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct std_descr {
	v4l2_std_id std;
	const char *descr;
};

struct srccap_avd_info {
	enum v4l2_srccap_pcm_status enVdSignalStatus;
	struct v4l2_timing stVDtiming;
	v4l2_std_id u64DetectTvSystem;
	v4l2_std_id region_type;
	u32 Comb3dBufAddr;
	u32 Comb3dBufSize;
	u32 Comb3dBufHeapId;
	u32 u32Comb3dAliment;
	u32 u32Comb3dSize;
	u8 *pu8RiuBaseAddr;
	u32 u32DramBaseAddr;
	u64 *pIoremap;
	u32 regCount;
	u8 reg_info_offset;
	u8 reg_pa_idx;
	u8 reg_pa_size_idx;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_srccap_register_avd_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_avd,
	struct v4l2_ctrl_handler *avd_ctrl_handler);
void mtk_srccap_unregister_avd_subdev(
	struct v4l2_subdev *subdev_avd);
#endif  // MTK_SRCCAP_AVD_H
