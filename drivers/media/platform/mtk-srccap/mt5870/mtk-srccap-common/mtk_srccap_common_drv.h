/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

int mtk_srccap_drv_testpattern(
		struct v4l2_srccap_testpattern *stSrccapTestPattern,
		struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_get_output_info(
		struct v4l2_output_ext_info *stOutputExtInfo,
		struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_set_output_info(
		struct v4l2_output_ext_info *pstOutputExtInfo,
		struct mtk_srccap_dev *srccap_dev);
