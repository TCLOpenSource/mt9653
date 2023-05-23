/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

void mtk_srccap_drv_adc_set_source(enum v4l2_srccap_input_source enInputSource,
				struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_pcclock(__u16 *u16PCClk,
				enum srccap_access_type enAccessType);
int mtk_srccap_drv_adc_phase(__u16 *u16Phase,
				enum srccap_access_type enAccessType);
int mtk_srccap_drv_adc_source_calibration(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_get_phase_range(__u16 *u16Range);
int mtk_srccap_drv_adc_is_ScartRGB(bool *bIsScartRGB);
int mtk_srccap_drv_adc_set_default_gainoffset(
		struct v4l2_srccap_default_gain_offset *pstDefaultGainOffset,
		struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_get_default_gainoffset(
	struct v4l2_srccap_default_gain_offset *pstDefaultGainOffset,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_max_offset(__u16 *u16MaxOffset);
int mtk_srccap_drv_adc_max_gain(__u16 *u16MaxGain);
int mtk_srccap_drv_adc_center_gain(__u16 *u16CenterGain);
int mtk_srccap_drv_adc_center_offset(__u16 *u16CenterOffset);
int mtk_srccap_drv_adc_adjust_gainoffset(
	struct v4l2_srccap_gainoffset_setting *pstGainOffsetSetting);
int mtk_srccap_drv_adc_rgb_pipedelay(__u8 u8Pipedelay);
int mtk_srccap_drv_adc_enable_hw_calibration(bool bEnable);
int mtk_srccap_drv_adc_auto_geometry(
	struct v4l2_srccap_auto_geometry *pstAutoGeometry,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_auto_gainoffset(
	struct v4l2_srccap_auto_gainoffset *pstAutoGainOffset,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_set_hwfixed_gainoffset(
	struct v4l2_srccap_auto_hwfixed_gain_offset *pstHWFixedGainOffset,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_get_hwfixed_gainoffset(
	struct v4l2_srccap_auto_hwfixed_gain_offset *pstHWFixedGainOffset,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_auto_offset(
	struct v4l2_srccap_auto_offset *pstAutoOffset);
int mtk_srccap_drv_adc_auto_calibration_mode(
	enum v4l2_srccap_calibration_mode enMode);
int mtk_srccap_drv_adc_set_auto_sync_info(
	struct v4l2_srccap_auto_sync_info *pstAutoSyncInfo,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_get_auto_sync_info(
	struct v4l2_srccap_auto_sync_info *pstAutoSyncInfo,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_set_slt_ip_status(
	struct v4l2_srccap_ip_status *pstIPStatus,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_adc_get_slt_ip_status(
	struct v4l2_srccap_ip_status *pstIPStatus,
	struct mtk_srccap_dev *srccap_dev);
