/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

int mtk_srccap_timingdetect_drv_init(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_timingdetect_drv_exit(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_get_hdmi_sync_mode(
			enum v4l2_srccap_hdmi_sync_mode *enSyncMode);
int mtk_srccap_drv_get_current_pcm_status(
			enum v4l2_srccap_pcm_status *enPCMStatus,
			struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_detect_timing_pcm_init(void);
int mtk_srccap_drv_detect_timing_restart(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_set_detect_timing_counter(
			struct v4l2_srccap_pcm_timing_count *pstTimingCounter);
int mtk_srccap_drv_auto_no_signal(bool *bEnable,
	enum srccap_access_type enAccessType, struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_set_pcm(struct v4l2_srccap_pcm *pstPCM,
				struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_get_pcm(struct v4l2_srccap_pcm *pstPCM,
				struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_set_invalid_timing_detect(
	struct v4l2_srccap_pcm_invalid_timing_detect *pstInvalidTimingDetect,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_get_invalid_timing_detect(
	struct v4l2_srccap_pcm_invalid_timing_detect *pstInvalidTimingDetect,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_update_setting(
	struct v4l2_srccap_mp_input_info *pstSrccapUpdateSetting,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_mode_table(
	struct v4l2_srccap_timing_mode_table *pstTimingModeTable,
	struct mtk_srccap_dev *srccap_dev);

