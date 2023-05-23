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

#include <linux/mtk-v4l2-srccap.h>
#include "mtk_srccap.h"
#include "mtk_srccap_avd.h"
#include "mtk_srccap_avd_avd.h"
#include "mtk_srccap_avd_mux.h"
#include "mtk_srccap_avd_colorsystem.h"
#include "show_param.h"
#include "sti_msos.h"
#include "avd-ex.h"

/* ============================================================================================== */
/* -------------------------------------- Global Type Definitions ------------------------------- */
/* ============================================================================================== */
/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static struct v4l2_ext_avd_scan_hsyc_check pHsync;

static int _avd_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
				struct mtk_srccap_dev, srccap_ctrl_handler);
	struct v4l2_timing ptiming;
	int ret;
	bool bResult;
	struct v4l2_ext_avd_standard_detection pDetection;
	struct v4l2_ext_avd_info pinfo;
	__u32 u32AVDFlag;
	__u16 u16vdstatus;
	__u8 u8HsyncEdge, u8NoiseMag, u8lock;

	switch (ctrl->id) {
	case V4L2_CID_AVD_HSYNC_EDGE:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_HSYNC_EDGE\n",
							__func__, __LINE__);
		memset(&u8HsyncEdge, 0, sizeof(__u8));
		ret = mtk_avd_hsync_edge(&u8HsyncEdge);
		memcpy((void *)ctrl->p_new.p_u8, &u8HsyncEdge, sizeof(__u8));
		break;
	case V4L2_CID_AVD_FLAG:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_FLAG\n",
							__func__, __LINE__);
		memset(&u32AVDFlag, 0, sizeof(__u32));
		ret = mtk_avd_g_flag(&u32AVDFlag);
		memcpy((void *)ctrl->p_new.p_u8, &u32AVDFlag, sizeof(__u32));
		break;
	case V4L2_CID_AVD_V_TOTAL:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_V_TOTAL\n",
							__func__, __LINE__);
		memset(&ptiming, 0, sizeof(struct v4l2_timing));
		ret = mtk_avd_v_total(&ptiming);
		memcpy((void *)ctrl->p_new.p_u8, &ptiming,
					sizeof(struct v4l2_timing));
		break;
	case V4L2_CID_AVD_NOISE_MAG:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_NOISE_MAG\n",
							__func__, __LINE__);
		memset(&u8NoiseMag, 0, sizeof(__u8));
		ret = mtk_avd_noise_mag(&u8NoiseMag);
		memcpy((void *)ctrl->p_new.p_u8, &u8NoiseMag, sizeof(__u8));
		break;
	case V4L2_CID_AVD_IS_SYNCLOCKED:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_IS_SYNCLOCKED\n",
							__func__, __LINE__);
		memset(&u8lock, 0, sizeof(__u8));
		ret = mtk_avd_is_synclocked(&u8lock);
		memcpy((void *)ctrl->p_new.p_u8, &u8lock, sizeof(__u8));
		break;
	case V4L2_CID_AVD_STANDARD_DETETION:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_STANDARD_DETETION\n",
							__func__, __LINE__);
		memset(&pDetection, 0,
			sizeof(struct v4l2_ext_avd_standard_detection));
		ret = mtk_avd_standard_detetion(&pDetection);
		memcpy((void *)ctrl->p_new.p_u8, &pDetection,
			sizeof(struct v4l2_ext_avd_standard_detection));
		break;
	case V4L2_CID_AVD_STATUS:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_STATUS\n",
							__func__, __LINE__);
		memset(&u16vdstatus, 0, sizeof(__u16));
		ret = mtk_avd_status(&u16vdstatus);
		memcpy((void *)ctrl->p_new.p_u8, &u16vdstatus, sizeof(__u16));
		break;
	case V4L2_CID_AVD_SCAN_HSYNC_CHECK:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_SCAN_HSYNC_CHECK\n",
							__func__, __LINE__);
		ret = mtk_avd_scan_hsync_check(&pHsync);
		memcpy((void *)ctrl->p_new.p_u8, &pHsync,
				sizeof(struct v4l2_ext_avd_scan_hsyc_check));
		break;
	case V4L2_CID_AVD_VERTICAL_FREQ:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_VERTICAL_FREQ\n",
							__func__, __LINE__);
		memset(&ptiming, 0, sizeof(struct v4l2_timing));
		ret = mtk_avd_vertical_freq(&ptiming);
		memcpy((void *)ctrl->p_new.p_u8, &ptiming,
						sizeof(struct v4l2_timing));
		break;
	case V4L2_CID_AVD_INFO:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_INFO\n",
							__func__, __LINE__);
		memset(&pinfo, 0, sizeof(struct v4l2_ext_avd_info));
		ret = mtk_avd_info(&pinfo);
		memcpy((void *)ctrl->p_new.p_u8, &pinfo,
					sizeof(struct v4l2_ext_avd_info));
		break;
	case V4L2_CID_AVD_IS_LOCK_AUDIO_CARRIER:
		AVD_API_MSG(
		"[V4L2][%s][%d]V4L2_CID_AVD_IS_LOCK_AUDIO_CARRIER\n",
							__func__, __LINE__);
		memset(&bResult, 0, sizeof(bool));
		ret = 0;
		mtk_avd_is_lock_audio_carrier(&bResult);
		memcpy((void *)ctrl->p_new.p_u8, &bResult, sizeof(bool));

		break;
	case V4L2_CID_AVD_ALIVE_CHECK:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_ALIVE_CHECK\n",
							__func__, __LINE__);
		memset(&bResult, 0, sizeof(bool));
		ret = 0;
		mtk_avd_alive_check(&bResult);
		memcpy((void *)ctrl->p_new.p_u8, &bResult, sizeof(bool));
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _avd_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
			struct mtk_srccap_dev, srccap_ctrl_handler);
	int ret;
	struct v4l2_ext_avd_init_data pParam;
	v4l2_std_id u64Videostandardtype;
	struct v4L2_ext_avd_video_standard pVideoStandard;
	enum v4l2_ext_avd_freerun_freq pFreerunFreq;
	struct v4l2_ext_avd_still_image_param pImageParam;
	struct v4l2_ext_avd_factory_data pFactoryData;
	struct v4l2_ext_avd_3d_comb_speed pSpeed;
	bool bCombEanble, bTuningEanble;
	__u32 u32VDPatchFlag;
	__u16 u16Htt;
	__u8 u8ScartFB;

	switch (ctrl->id) {
	case V4L2_CID_AVD_INIT:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_INIT\n",
							__func__, __LINE__);
		memset(&pParam, 0, sizeof(struct v4l2_ext_avd_init_data));
		memcpy(&pParam, (void *)ctrl->p_new.p_u8,
					sizeof(struct v4l2_ext_avd_init_data));
		ret = mtk_avd_init(pParam);
		break;
	case V4L2_CID_AVD_EXIT:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_EXIT\n",
							__func__, __LINE__);
		ret = mtk_avd_exit();
		break;
	case V4L2_CID_AVD_INPUT:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_INPUT\n",
							__func__, __LINE__);
		memset(&u8ScartFB, 0, sizeof(__u8));
		memcpy(&u8ScartFB, (void *)ctrl->p_new.p_u8, sizeof(__u8));
		ret = mtk_avd_input(&u8ScartFB);
		break;
	case V4L2_CID_AVD_FLAG:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_FLAG\n",
							__func__, __LINE__);
		memset(&u32VDPatchFlag, 0, sizeof(__u32));
		memcpy(&u32VDPatchFlag, (void *)ctrl->p_new.p_u8,
							sizeof(__u32));
		ret = mtk_avd_s_flag(&u32VDPatchFlag);
		break;
	case V4L2_CID_AVD_FORCE_VIDEO_STANDARD:
		AVD_API_MSG(
			"[V4L2][%s][%d]V4L2_CID_AVD_FORCE_VIDEO_STANDARD\n",
							__func__, __LINE__);
		memset(&u64Videostandardtype, 0, sizeof(v4l2_std_id));
		memcpy(&u64Videostandardtype, (void *)ctrl->p_new.p_u8,
							sizeof(v4l2_std_id));
		ret = mtk_avd_force_video_standard(&u64Videostandardtype);
		break;
	case V4L2_CID_AVD_VIDEO_STANDARD:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_VIDEO_STANDARD\n",
							__func__, __LINE__);
		memset(&pVideoStandard, 0,
				sizeof(struct v4L2_ext_avd_video_standard));
		memcpy(&pVideoStandard, (void *)ctrl->p_new.p_u8,
				sizeof(struct v4L2_ext_avd_video_standard));
		ret = mtk_avd_video_standard(&pVideoStandard);
		break;
	case V4L2_CID_AVD_FREERUN_FREQ:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_FREERUN_FREQ\n",
							__func__, __LINE__);
		memset(&pFreerunFreq, 0,
			sizeof(enum v4l2_ext_avd_freerun_freq));
		memcpy(&pFreerunFreq, (void *)ctrl->p_new.p_u8,
			sizeof(enum v4l2_ext_avd_freerun_freq));
		ret = mtk_avd_freerun_freq(&pFreerunFreq);
		break;
	case V4L2_CID_AVD_STILL_IMAGE_PARAM:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_STILL_IMAGE_PARAM\n",
							__func__, __LINE__);
		memset(&pImageParam, 0,
			sizeof(struct v4l2_ext_avd_still_image_param));
		memcpy(&pImageParam, (void *)ctrl->p_new.p_u8,
			sizeof(struct v4l2_ext_avd_still_image_param));
		ret = mtk_avd_still_image_param(&pImageParam);
		break;
	case V4L2_CID_AVD_FACTORY_PARA:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_FACTORY_PARA\n",
							__func__, __LINE__);
		memset(&pFactoryData, 0,
			sizeof(struct v4l2_ext_avd_factory_data));
		memcpy(&pFactoryData, (void *)ctrl->p_new.p_u8,
			sizeof(struct v4l2_ext_avd_factory_data));
		ret = mtk_avd_factory_para(&pFactoryData);
		break;
	case V4L2_CID_AVD_3D_COMB_SPEED:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_3D_COMB_SPEED\n",
							__func__, __LINE__);
		memset(&pSpeed, 0,
			sizeof(struct v4l2_ext_avd_3d_comb_speed));
		memcpy(&pSpeed, (void *)ctrl->p_new.p_u8,
			sizeof(struct v4l2_ext_avd_3d_comb_speed));
		ret = mtk_avd_3d_comb_speed(&pSpeed);
		break;
	case V4L2_CID_AVD_3D_COMB:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_3D_COMB\n",
							__func__, __LINE__);
		memset(&bCombEanble, 0, sizeof(bool));
		memcpy(&bCombEanble, (void *)ctrl->p_new.p_u8, sizeof(bool));
		ret = mtk_avd_3d_comb(&bCombEanble);
		break;
	case V4L2_CID_AVD_REG_FROM_DSP:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_REG_FROM_DSP\n",
							__func__, __LINE__);
		ret = mtk_avd_reg_from_dsp();
		break;
	case V4L2_CID_AVD_HTT_USER_MD:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_HTT_USER_MD\n",
							__func__, __LINE__);
		memset(&u16Htt, 0, sizeof(__u16));
		memcpy(&u16Htt, (void *)ctrl->p_new.p_u8, sizeof(__u16));
		ret = mtk_avd_htt_user_md(&u16Htt);
		break;
	case V4L2_CID_AVD_HSYNC_DETECTION_FOR_TUNING:
		AVD_API_MSG(
		"[V4L2][%s][%d]V4L2_CID_AVD_HSYNC_DETECTION_FOR_TUNING\n",
							__func__, __LINE__);
		memset(&bTuningEanble, 0, sizeof(bool));
		memcpy(&bTuningEanble, (void *)ctrl->p_new.p_u8,
							sizeof(bool));
		ret = mtk_avd_hsync_detect_detetion_for_tuning(&bTuningEanble);
		break;
	case V4L2_CID_AVD_CHANNEL_CHANGE:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_CHANNEL_CHANGE\n",
							__func__, __LINE__);
		ret = mtk_avd_channel_change();
		break;
	case V4L2_CID_AVD_MCU_RESET:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_MCU_RESET\n",
							__func__, __LINE__);
		ret = mtk_avd_mcu_reset();
		break;
	case V4L2_CID_AVD_START_AUTO_STANDARD_DETECTION:
		AVD_API_MSG(
		"[V4L2][%s][%d]V4L2_CID_AVD_START_AUTO_STANDARD_DETECTION\n",
							__func__, __LINE__);
		ret = mtk_avd_start_auto_standard_detetion();
		break;
	case V4L2_CID_AVD_3D_COMB_SPEED_UP:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_3D_COMB_SPEED_UP\n",
							__func__, __LINE__);
		ret = mtk_avd_3d_comb_speed_up();
		break;
	case V4L2_CID_AVD_SCAN_HSYNC_CHECK:
		AVD_API_MSG("[V4L2][%s][%d]V4L2_CID_AVD_SCAN_HSYNC_CHECK\n",
							__func__, __LINE__);
		memset(&pHsync, 0,
			sizeof(struct v4l2_ext_avd_scan_hsyc_check));
		memcpy(&pHsync, (void *)ctrl->p_new.p_u8,
			sizeof(struct v4l2_ext_avd_scan_hsyc_check));
		show_v4l2_ext_avd_scan_hsyc_check(&pHsync);
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops avd_ctrl_ops = {
	.g_volatile_ctrl = _avd_g_ctrl,
	.s_ctrl = _avd_s_ctrl,
};

static const struct v4l2_ctrl_config avd_ctrl[] = {
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_INIT,
		.name = "AVD Init",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(struct v4l2_ext_avd_init_data)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_EXIT,
		.name = "AVD exit",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_INPUT,
		.name = "AVD_INPUT",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(unsigned char)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_FLAG,
		.name = "AVD_FLAG",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(unsigned int)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_SCAN_HSYNC_CHECK,
		.name = "AVD_SCAN_HSYNC_CHECK",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(struct v4l2_ext_avd_scan_hsyc_check)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_FORCE_VIDEO_STANDARD,
		.name = "AVD_FORCE_VIDEO_STANDARD",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(v4l2_std_id)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_VIDEO_STANDARD,
		.name = "AVD_VIDEO_STANDARD",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(struct v4L2_ext_avd_video_standard)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_FREERUN_FREQ,
		.name = "AVD_FREERUN_FREQ",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(enum v4l2_ext_avd_freerun_freq)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_STILL_IMAGE_PARAM,
		.name = "AVD_STILL_IMAGE_PARAM",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(struct v4l2_ext_avd_still_image_param)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_FACTORY_PARA,
		.name = "AVD_FACTORY_PARA",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(struct v4l2_ext_avd_factory_data)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_3D_COMB_SPEED,
		.name = "AVD_3D_COMB_SPEED",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(struct v4l2_ext_avd_3d_comb_speed)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_3D_COMB,
		.name = "AVD_3D_COMB",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(bool)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_REG_FROM_DSP,
		.name = "AVD_REG_FROM_DSP",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_HTT_USER_MD,
		.name = "AVD_HTT_USER_MD",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(unsigned short)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_HSYNC_DETECTION_FOR_TUNING,
		.name = "AVD_HSYNC_DETECTION_FOR_TUNING",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(bool)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_CHANNEL_CHANGE,
		.name = "AVD_CHANNEL_CHANGE",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_MCU_RESET,
		.name = "AVD_MCU_RESET",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_START_AUTO_STANDARD_DETECTION,
		.name = "AVD_START_AUTO_STANDARD_DETECTION",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_3D_COMB_SPEED_UP,
		.name = "AVD_3D_COMB_SPEED_UP",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_HSYNC_EDGE,
		.name = "AVD_HSYNC_EDGE",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(unsigned char)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_FLAG,
		.name = "AVD_FLAG",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(unsigned int)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_V_TOTAL,
		.name = "AVD_V_TOTAL",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(struct v4l2_timing)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_NOISE_MAG,
		.name = "AVD_NOISE_MAG",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(unsigned char)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_IS_SYNCLOCKED,
		.name = "AVD_NOISE_MAG",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(unsigned char)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_STANDARD_DETETION,
		.name = "AVD_STANDARD_DETETION",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(struct v4l2_ext_avd_standard_detection)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_STATUS,
		.name = "AVD_STATUS",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(unsigned short)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_SCAN_HSYNC_CHECK,
		.name = "AVD_SCAN_HSYNC_CHECK",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(struct v4l2_ext_avd_scan_hsyc_check)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_VERTICAL_FREQ,
		.name = "AVD_VERTICAL_FREQ",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(struct v4l2_timing)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_INFO,
		.name = "AVD_INFO",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(struct v4l2_ext_avd_info)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_IS_LOCK_AUDIO_CARRIER,
		.name = "AVD_IS_LOCK_AUDIO_CARRIER",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(bool)},
	},
	{
		.ops = &avd_ctrl_ops,
		.id = V4L2_CID_AVD_ALIVE_CHECK,
		.name = "AVD_ALIVE_CHECK",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.dims = {sizeof(bool)},
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops avd_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops avd_sd_internal_ops = {
};
/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_srccap_register_avd_subdev(
	struct v4l2_device *srccap_dev, struct v4l2_subdev *subdev_avd,
	struct v4l2_ctrl_handler *avd_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(avd_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(avd_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(avd_ctrl_handler, &avd_ctrl[ctrl_count],
									NULL);
	}

	ret = avd_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create avd ctrl handler\n");
		goto exit;
	}
	subdev_avd->ctrl_handler = avd_ctrl_handler;

	v4l2_subdev_init(subdev_avd, &avd_sd_ops);
	subdev_avd->internal_ops = &avd_sd_internal_ops;
	strlcpy(subdev_avd->name, "mtk-avd", sizeof(subdev_avd->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_avd);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register avd subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(avd_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_avd_subdev(struct v4l2_subdev *subdev_avd)
{
	v4l2_ctrl_handler_free(subdev_avd->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_avd);
}
