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
#include "mtk_srccap_adc.h"
#include "mtk_srccap_adc_drv.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int _adc_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
				struct mtk_srccap_dev, adc_ctrl_handler);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_ADC_PCCLOCK:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_PCCLOCK\n");
		ret = mtk_srccap_drv_adc_pcclock((__u16 *)&(ctrl->val),
							SRCCAP_GET);
		break;
	}
	case V4L2_CID_ADC_PHASE_RANGE:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_PHASE_RANGE\n");
		ret = mtk_srccap_drv_adc_get_phase_range((__u16 *)&(ctrl->val));
		break;
	}
	case V4L2_CID_ADC_IS_SCARTRGB:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_IS_SCARTRGB\n");
		ret = mtk_srccap_drv_adc_is_ScartRGB((bool *)&(ctrl->val));
		break;
	}
	case V4L2_CID_ADC_MAX_GAIN:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_MAX_GAIN\n");
		ret = mtk_srccap_drv_adc_max_gain((__u16 *)&(ctrl->val));
		break;
	}
	case V4L2_CID_ADC_MAX_OFFSET:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_MAX_OFFSET\n");
		ret = mtk_srccap_drv_adc_max_offset((__u16 *)&(ctrl->val));
		break;
	}
	case V4L2_CID_ADC_CENTER_GAIN:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_CENTER_GAIN\n");
		ret = mtk_srccap_drv_adc_center_gain((__u16 *)&(ctrl->val));
		break;
	}
	case V4L2_CID_ADC_CENTER_OFFSET:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_CENTER_OFFSET\n");
		ret = mtk_srccap_drv_adc_center_offset(
						(__u16 *)&(ctrl->val));
		break;
	}
	case V4L2_CID_ADC_DEFAULT_GAIN_OFFSET:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_DEFAULT_GAIN_OFFSET\n");
		ret = mtk_srccap_drv_adc_get_default_gainoffset(
		(struct v4l2_srccap_default_gain_offset *)ctrl->p_new.p_u8,
							srccap_dev);
		break;
	}
	case V4L2_CID_ADC_AUTO_HWFIXED_GAIN_OFFSET:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_AUTO_HWFIXED_GAIN_OFFSET\n");
		ret = mtk_srccap_drv_adc_get_hwfixed_gainoffset(
		(struct v4l2_srccap_auto_hwfixed_gain_offset *)
		ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_AUTO_SYNC_INFO:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_AUTO_SYNC_INFO\n");
		ret = mtk_srccap_drv_adc_get_auto_sync_info(
		(struct v4l2_srccap_auto_sync_info *)ctrl->p_new.p_u8,
							srccap_dev);
		break;
	}
	case V4L2_CID_ADC_IP_STATUS:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_AUTO_SYNC_INFO\n");
		ret = mtk_srccap_drv_adc_get_slt_ip_status(
		(struct v4l2_srccap_ip_status *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_PHASE:
	{
		//Only support in Merak
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_PHASE\n");
		break;
	}
	case V4L2_CID_ADC_OFFSET:
	{
		//Only support in Merak
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_OFFSET\n");
		break;
	}
	case V4L2_CID_ADC_GAIN:
	{
		//Only support in Merak
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_GAIN\n");
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _adc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = container_of(ctrl->handler,
				struct mtk_srccap_dev, adc_ctrl_handler);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_ADC_PCCLOCK:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_PCCLOCK\n");
		ret = mtk_srccap_drv_adc_pcclock((__u16 *)&(ctrl->val),
							SRCCAP_SET);
		break;
	}
	case V4L2_CID_ADC_SOURCE_CALIBRATE:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_SOURCE_CALIBRATE\n");
		ret = mtk_srccap_drv_adc_source_calibration(srccap_dev);
		break;
	}
	case V4L2_CID_ADC_RGB_PIPEDELAY:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_RGB_PIPEDELAY\n");
		ret = mtk_srccap_drv_adc_rgb_pipedelay((__u8)ctrl->val);
		break;
	}
	case V4L2_CID_ADC_ENABLE_HW_CALIBRATION:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_ENABLE_HW_CALIBRATION\n");
		ret = mtk_srccap_drv_adc_enable_hw_calibration((bool)ctrl->val);
		break;
	}
	case V4L2_CID_ADC_AUTO_CALIBRATION_MODE:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_AUTO_CALIBRATION_MODE\n");
		ret = mtk_srccap_drv_adc_auto_calibration_mode(
		(enum v4l2_srccap_calibration_mode)ctrl->val);
		break;
	}
	case V4L2_CID_ADC_ADJUST_GAIN_OFFSET:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_ADJUST_GAIN_OFFSET\n");
		ret = mtk_srccap_drv_adc_adjust_gainoffset(
		(struct v4l2_srccap_gainoffset_setting *)ctrl->p_new.p_u8);
		break;
	}
	case V4L2_CID_ADC_AUTO_GEOMETRY:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_AUTO_GEOMETRY\n");
		ret = mtk_srccap_drv_adc_auto_geometry(
		(struct v4l2_srccap_auto_geometry *)ctrl->p_new.p_u8,
							srccap_dev);
		break;
	}
	case V4L2_CID_ADC_AUTO_GAIN_OFFSET:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_AUTO_GAIN_OFFSET\n");
		ret = mtk_srccap_drv_adc_auto_gainoffset(
		(struct v4l2_srccap_auto_gainoffset *)ctrl->p_new.p_u8,
							srccap_dev);
		break;
	}
	case V4L2_CID_ADC_AUTO_AUTO_OFFSET:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_AUTO_AUTO_OFFSET\n");
		ret = mtk_srccap_drv_adc_auto_offset(
		(struct v4l2_srccap_auto_offset *)ctrl->p_new.p_u8);
		break;
	}
	case V4L2_CID_ADC_DEFAULT_GAIN_OFFSET:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_DEFAULT_GAIN_OFFSET\n");
		ret = mtk_srccap_drv_adc_set_default_gainoffset(
		(struct v4l2_srccap_default_gain_offset *)ctrl->p_new.p_u8,
							srccap_dev);
		break;
	}
	case V4L2_CID_ADC_AUTO_HWFIXED_GAIN_OFFSET:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_AUTO_HWFIXED_GAIN_OFFSET\n");
		ret = mtk_srccap_drv_adc_set_hwfixed_gainoffset(
		(struct v4l2_srccap_auto_hwfixed_gain_offset *)
		ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_AUTO_SYNC_INFO:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_AUTO_SYNC_INFO\n");
		ret = mtk_srccap_drv_adc_set_auto_sync_info(
		(struct v4l2_srccap_auto_sync_info *)ctrl->p_new.p_u8,
							srccap_dev);
		break;
	}
	case V4L2_CID_ADC_IP_STATUS:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_IP_STATUS\n");
		ret = mtk_srccap_drv_adc_set_slt_ip_status(
		(struct v4l2_srccap_ip_status *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_SOURCE:
	{
		//Only support in Merak
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_SOURCE\n");
		break;
	}
	case V4L2_CID_ADC_PHASE:
	{
		//Only support in Merak
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_PHASE\n");
		break;
	}
	case V4L2_CID_ADC_OFFSET:
	{
		//Only support in Merak
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_OFFSET\n");
		break;
	}
	case V4L2_CID_ADC_GAIN:
	{
		//Only support in Merak
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_GAIN\n");
		break;
	}
	default:
		ret = -1;
		break;
	}
	return ret;
}

static const struct v4l2_ctrl_ops adc_ctrl_ops = {
	.g_volatile_ctrl = _adc_g_ctrl,
	.s_ctrl = _adc_s_ctrl,
};

static const struct v4l2_ctrl_config adc_ctrl[] = {
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_PCCLOCK,
		.name = "Srccap ADC PCClock",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_SOURCE_CALIBRATE,
		.name = "Srccap ADC Source Calibration",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_RGB_PIPEDELAY,
		.name = "Srccap ADC RGB Pipedelay",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_ENABLE_HW_CALIBRATION,
		.name = "Srccap ADC Enable HW Calibration",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = false,
		.max = true,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_AUTO_CALIBRATION_MODE,
		.name = "Srccap ADC Enable HW Calibration",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_PHASE_RANGE,
		.name = "Srccap ADC Get Phase Range",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_IS_SCARTRGB,
		.name = "Srccap ADC Is ScartRGB",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = false,
		.max = true,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_MAX_GAIN,
		.name = "Srccap ADC Get Max Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_MAX_OFFSET,
		.name = "Srccap ADC Get Max Offset",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_CENTER_GAIN,
		.name = "Srccap ADC Get Center Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_CENTER_OFFSET,
		.name = "Srccap ADC Get Center Offset",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_ADJUST_GAIN_OFFSET,
		.name = "Srccap ADC Adjust Gain Offset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_gainoffset_setting)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_AUTO_GEOMETRY,
		.name = "Srccap ADC Auto Geometry",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_auto_geometry)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_AUTO_GAIN_OFFSET,
		.name = "Srccap ADC Auto Gain Offset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_auto_gainoffset)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_AUTO_AUTO_OFFSET,
		.name = "Srccap ADC Auto Offset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_auto_offset)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_DEFAULT_GAIN_OFFSET,
		.name = "Srccap ADC Get Default Gain Offset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_default_gain_offset)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_AUTO_HWFIXED_GAIN_OFFSET,
		.name = "Srccap ADC Get HW Fixed Gain Offset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_auto_hwfixed_gain_offset)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_AUTO_SYNC_INFO,
		.name = "Srccap ADC Get Auto Sync Info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_auto_sync_info)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_IP_STATUS,
		.name = "Srccap ADC Get IP Status",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_ip_status)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_SOURCE,
		.name = "Srccap ADC Set Source",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_adc_source)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_PHASE,
		.name = "Srccap ADC Phase",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_OFFSET,
		.name = "Srccap ADC Offset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.dims = {sizeof(struct v4l2_adc_offset)},
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_GAIN,
		.name = "Srccap ADC Gain",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.dims = {sizeof(struct v4l2_adc_gain)},
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops adc_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops adc_sd_internal_ops = {
};

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_adc_set_port(struct mtk_srccap_dev *srccap_dev)
{
	int i = 0;
	u32 hdmi_cnt = srccap_dev->srccap_info.srccap_cap.hdmi_cnt;
	u32 cvbs_cnt = srccap_dev->srccap_info.srccap_cap.cvbs_cnt;
	u32 svideo_cnt = srccap_dev->srccap_info.srccap_cap.svideo_cnt;
	u32 ypbpr_cnt = srccap_dev->srccap_info.srccap_cap.ypbpr_cnt;
	u32 vga_cnt = srccap_dev->srccap_info.srccap_cap.vga_cnt;
	int ret = 0;

	if (srccap_dev->srccap_info.srccap_cap.hdmi_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
			i < (V4L2_SRCCAP_INPUT_SOURCE_HDMI + hdmi_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}
	if (srccap_dev->srccap_info.srccap_cap.cvbs_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
			i < (V4L2_SRCCAP_INPUT_SOURCE_CVBS + cvbs_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}
	if (srccap_dev->srccap_info.srccap_cap.svideo_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO;
			i < (V4L2_SRCCAP_INPUT_SOURCE_SVIDEO + svideo_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}
	if (srccap_dev->srccap_info.srccap_cap.ypbpr_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
			i < (V4L2_SRCCAP_INPUT_SOURCE_YPBPR + ypbpr_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}
	if (srccap_dev->srccap_info.srccap_cap.vga_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_VGA;
			i < (V4L2_SRCCAP_INPUT_SOURCE_VGA + vga_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}

	return ret;
}

int mtk_srccap_register_adc_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_adc,
	struct v4l2_ctrl_handler *adc_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(adc_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(adc_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(adc_ctrl_handler, &adc_ctrl[ctrl_count],
									NULL);
	}

	ret = adc_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create adc ctrl handler\n");
		goto exit;
	}
	subdev_adc->ctrl_handler = adc_ctrl_handler;

	v4l2_subdev_init(subdev_adc, &adc_sd_ops);
	subdev_adc->internal_ops = &adc_sd_internal_ops;
	strlcpy(subdev_adc->name, "mtk-adc", sizeof(subdev_adc->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_adc);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register adc subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(adc_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_adc_subdev(struct v4l2_subdev *subdev_adc)
{
	v4l2_ctrl_handler_free(subdev_adc->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_adc);
}


