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
#include <linux/clk-provider.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/delay.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <asm-generic/div64.h>

#include "mtk_srccap.h"
#include "mtk_srccap_timingdetect.h"
#include "hwreg_srccap_timingdetect.h"
#include "pixelmonitor.h"
#include "mtk_srccap_common_ca.h"
#include "utpa2_XC.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int timingdetect_get_clk(
	struct device *dev,
	char *s,
	struct clk **clk)
{
	if (dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (s == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (clk == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	*clk = devm_clk_get(dev, s);
	if (IS_ERR(*clk)) {
		SRCCAP_MSG_FATAL("unable to retrieve %s\n", s);
		return PTR_ERR(clk);
	}

	return 0;
}

static void timingdetect_put_clk(
	struct device *dev,
	struct clk *clk)
{
	if (dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	devm_clk_put(dev, clk);
}

static int timingdetect_enable_parent(
	struct mtk_srccap_dev *srccap_dev,
	struct clk *clk,
	char *s)
{
	int ret = 0;
	struct clk *parent = NULL;

	ret = timingdetect_get_clk(srccap_dev->dev, s, &parent);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	ret = clk_set_parent(clk, parent);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	ret = clk_prepare_enable(clk);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	timingdetect_put_clk(srccap_dev->dev, parent);

	return 0;
}

static int timingdetect_map_detblock_reg(
	enum v4l2_srccap_input_source src,
	enum reg_srccap_timingdetect_source *detblock)
{
	if (detblock == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (src) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI4;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_VD;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_VD;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_VD;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_VD;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		*detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI4;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


static int timingdetect_set_source_hwreg(
	enum reg_srccap_timingdetect_source detblock,
	enum reg_srccap_timingdetect_sync sync,
	enum reg_srccap_timingdetect_sync_detection sync_detection,
	enum reg_srccap_timingdetect_video_port video_port,
	enum reg_srccap_timingdetect_video_type video_type,
	bool de_only,
	bool de_bypass,
	bool dot_based_hsync,
	bool component,
	bool coast_pol,
	bool hs_ref_edge,
	bool vs_delay_md,
	bool direct_de_md,
	bool vdovs_ref_edge,
	bool in_field_inv,
	bool ext_field,
	bool out_field_inv,
	u8 front_coast,
	u8 back_coast)
{
	int ret = 0;

	ret = drv_hwreg_srccap_timingdetect_set_source(detblock, video_type, TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_sync(detblock, sync, TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_sync_detection(detblock, sync_detection,
		TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_video_port(detblock, video_port,
		TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_de_only_mode(detblock, de_only, TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_de_bypass_mode(detblock, de_bypass,
		TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_dot_based_hsync(detblock, dot_based_hsync,
		TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_adc_source_sel(detblock, component,
		TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_coast_polarity(detblock, coast_pol,
		TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_front_coast(detblock, front_coast,
		TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_back_coast(detblock, back_coast,
		TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_hsync_ref_edge(detblock, hs_ref_edge,
		TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_vsync_delay_mode(detblock, vs_delay_md,
		TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_direct_de_mode(detblock, direct_de_md,
		TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_vdo_vsync_ref_edge(detblock, vdovs_ref_edge,
		TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_input_field_invert(detblock, in_field_inv,
		TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_external_field_en(detblock, ext_field,
		TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_output_field_invert(detblock, out_field_inv,
		TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

static int timingdetect_set_source(
	struct mtk_srccap_dev *srccap_dev,
	enum reg_srccap_timingdetect_source detblock,
	enum v4l2_srccap_input_source src)
{
	int ret = 0;
	enum reg_srccap_timingdetect_video_type video_type =
		REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_ANALOG1;
	enum reg_srccap_timingdetect_sync sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
	enum reg_srccap_timingdetect_sync_detection sync_detection =
		REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
	enum reg_srccap_timingdetect_video_port video_port =
		REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;

	bool de_only = FALSE, de_bypass = FALSE, dot_based_hsync = FALSE, component = FALSE;
	bool coast_pol = FALSE, hs_ref_edge = FALSE, vs_delay_md = FALSE, direct_de_md = FALSE;
	bool vdovs_ref_edge = FALSE, in_field_inv = FALSE, ext_field = FALSE, out_field_inv = FALSE;
	u8 front_coast = 0;
	u8 back_coast = 0;
	struct clk *clk = NULL;

	switch (src) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_HDMI;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = TRUE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		clk = srccap_dev->srccap_info.clk.hdmi_idclk_ck;
		ret = timingdetect_enable_parent(srccap_dev, clk,
			"srccap_hdmi_idclk_ck_ip1_xtal_ck");
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_HDMI;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = TRUE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		clk = srccap_dev->srccap_info.clk.hdmi2_idclk_ck;
		ret = timingdetect_enable_parent(srccap_dev, clk,
			"srccap_hdmi2_idclk_ck_ip1_xtal_ck");
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_HDMI;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = TRUE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		clk = srccap_dev->srccap_info.clk.hdmi3_idclk_ck;
		ret = timingdetect_enable_parent(srccap_dev, clk,
			"srccap_hdmi3_idclk_ck_ip1_xtal_ck");
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_HDMI;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = TRUE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		clk = srccap_dev->srccap_info.clk.hdmi4_idclk_ck;
		ret = timingdetect_enable_parent(srccap_dev, clk,
			"srccap_hdmi4_idclk_ck_ip1_xtal_ck");
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_VIDEO;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_INT_VD_A;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = FALSE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = FALSE;
		direct_de_md = TRUE;
		vdovs_ref_edge = TRUE;
		in_field_inv = TRUE;
		ext_field = TRUE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_VIDEO;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_INT_VD_A;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = FALSE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = FALSE;
		direct_de_md = TRUE;
		vdovs_ref_edge = TRUE;
		in_field_inv = TRUE;
		ext_field = TRUE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_ANALOG1;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_SOG;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_SOG;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = FALSE;
		component = TRUE;
		coast_pol = TRUE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = TRUE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0xC;
		back_coast = 0xC;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_ANALOG1;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = FALSE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = FALSE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_VIDEO;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_INT_VD_A;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = FALSE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = FALSE;
		direct_de_md = TRUE;
		vdovs_ref_edge = TRUE;
		in_field_inv = TRUE;
		ext_field = TRUE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_VIDEO;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_INT_VD_A;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = FALSE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = FALSE;
		direct_de_md = TRUE;
		vdovs_ref_edge = TRUE;
		in_field_inv = TRUE;
		ext_field = TRUE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_DVI;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = TRUE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		clk = srccap_dev->srccap_info.clk.hdmi_idclk_ck;
		ret = timingdetect_enable_parent(srccap_dev, clk,
			"srccap_hdmi_idclk_ck_ip1_xtal_ck");
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_DVI;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = TRUE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		clk = srccap_dev->srccap_info.clk.hdmi2_idclk_ck;
		ret = timingdetect_enable_parent(srccap_dev, clk,
			"srccap_hdmi2_idclk_ck_ip1_xtal_ck");
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_DVI;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = FALSE;
		dot_based_hsync = TRUE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		clk = srccap_dev->srccap_info.clk.hdmi3_idclk_ck;
		ret = timingdetect_enable_parent(srccap_dev, clk,
			"srccap_hdmi3_idclk_ck_ip1_xtal_ck");
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		video_type = REG_SRCCAP_TIMINGDETECT_VIDEO_TYPE_DVI;
		sync = REG_SRCCAP_TIMINGDETECT_SYNC_CSYNC;
		sync_detection = REG_SRCCAP_TIMINGDETECT_SYNC_DETECTION_AUTO;
		video_port = REG_SRCCAP_TIMINGDETECT_PORT_EXT_8_10_BIT;
		de_only = FALSE;
		de_bypass = TRUE;
		dot_based_hsync = TRUE;
		component = FALSE;
		coast_pol = FALSE;
		hs_ref_edge = TRUE;
		vs_delay_md = TRUE;
		direct_de_md = TRUE;
		vdovs_ref_edge = FALSE;
		in_field_inv = TRUE;
		ext_field = FALSE;
		out_field_inv = TRUE;
		front_coast = 0;
		back_coast = 0;
		clk = srccap_dev->srccap_info.clk.hdmi4_idclk_ck;
		ret = timingdetect_enable_parent(srccap_dev, clk,
			"srccap_hdmi4_idclk_ck_ip1_xtal_ck");
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}

	ret = timingdetect_set_source_hwreg(detblock, sync, sync_detection,
						video_port, video_type,
						de_only, de_bypass, dot_based_hsync,
						component, coast_pol, hs_ref_edge, vs_delay_md,
						direct_de_md, vdovs_ref_edge, in_field_inv,
						ext_field, out_field_inv, front_coast, back_coast);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

static int timingdetect_check_polarity(
	u16 status,
	bool hpol,
	bool vpol,
	bool *same_pol)
{
	if (same_pol == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	*same_pol = TRUE;

	if (status & SRCCAP_TIMINGDETECT_FLAG_POL_HPVP) {
		if (!(hpol && vpol))
			*same_pol = FALSE;
	}
	if (status & SRCCAP_TIMINGDETECT_FLAG_POL_HPVN) {
		if (!(hpol && !vpol))
			*same_pol = FALSE;
	}
	if (status & SRCCAP_TIMINGDETECT_FLAG_POL_HNVP) {
		if (!(!hpol && vpol))
			*same_pol = FALSE;
	}
	if (status & SRCCAP_TIMINGDETECT_FLAG_POL_HNVN) {
		if (!(!hpol && !vpol))
			*same_pol = FALSE;
	}

	return 0;
}

static int timingdetect_match_vd_sampling(
	enum v4l2_ext_avd_videostandardtype videotype,
	struct srccap_timingdetect_vdsampling *vdsampling_table,
	u16 table_entry_cnt,
	u16 *table_index)
{
	if (vdsampling_table == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (table_index == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	SRCCAP_MSG_INFO("%s:%u, %s:%d\n",
		"video type", videotype,
		"vdtable_entry_count", table_entry_cnt);

	for (*table_index = 0; (*table_index) < table_entry_cnt; (*table_index)++) {
		SRCCAP_MSG_INFO("[%u], %s:%d\n",
			*table_index, "status", vdsampling_table[*table_index].videotype);

		if (videotype == vdsampling_table[*table_index].videotype)
			break;
	}

	if (*table_index == table_entry_cnt) {
		SRCCAP_MSG_ERROR("video type not supported\n");
		return -ERANGE;
	}

	return 0;
}

static int timingdetect_check_source(
	enum v4l2_srccap_input_source src,
	u16 status,
	bool *same_source)
{
	if (same_source == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (src) {

	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		if (status & SRCCAP_TIMINGDETECT_FLAG_HDMI)
			*same_source = true;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		if (status & SRCCAP_TIMINGDETECT_FLAG_DVI)
			*same_source = true;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		if (status & SRCCAP_TIMINGDETECT_FLAG_YPBPR)
			*same_source = true;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		if (status & SRCCAP_TIMINGDETECT_FLAG_VGA)
			*same_source = true;
		break;
	default:
		SRCCAP_MSG_ERROR("unknown src type\n");
		break;
	}

	return 0;
}

static int timingdetect_match_timing(
	bool hpol,
	bool vpol,
	bool interlaced,
	u32 hfreqx10,
	u64 vfreqx1000,
	u32 vtt,
	struct srccap_timingdetect_timing *timing_table,
	u16 table_entry_count,
	enum v4l2_srccap_input_source src,
	u16 *table_index)
{
	if (timing_table == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (table_index == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	u16 hfreq_tolerance = 0, vfreq_tolerance = 0;
	bool same_pol = FALSE;
	bool same_source = FALSE;

	SRCCAP_MSG_INFO("%s:%u, %s:%u, %s:%u, %s:%u, %s:%u, %s:%u, %s:%u\n",
		"table_entry_count", table_entry_count,
		"hpol", hpol,
		"vpol", vpol,
		"interlaced", interlaced,
		"hfreqx10", hfreqx10,
		"vfreqx1000", vfreqx1000,
		"vtt", vtt);

	/* find matching timing from index 0 to number of table entries */
	for (*table_index = 0; *table_index < table_entry_count; (*table_index)++) {
		SRCCAP_MSG_INFO("[%u], %s:0x%x, %s:%u, %s:%u, %s:%u, %s:%u,\n",
			*table_index,
			"status", timing_table[*table_index].status,
			"hfreqx10", timing_table[*table_index].hfreqx10,
			"vfreqx1000", timing_table[*table_index].vfreqx1000,
			"vtt", timing_table[*table_index].vtt,
			"src", src);
		/* check hpol and vpol only if polarity is defined in the specific timing */
		ret = timingdetect_check_polarity(timing_table[*table_index].status,
			hpol, vpol, &same_pol);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		if (same_pol == FALSE)
			continue;

		/* check source */
		ret = timingdetect_check_source(src, timing_table[*table_index].status,
			&same_source);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		if (same_source == FALSE)
			continue;

		/* check interlaced */
		if ((timing_table[*table_index].status & SRCCAP_TIMINGDETECT_FLAG_INTERLACED)
			!= (interlaced << SRCCAP_TIMINGDETECT_INTERLACED_BIT))
			continue;

		/* decrease vfreq tolerance if vfreqx1000 is 24000 or 25000 */
		if ((abs(timing_table[*table_index].vfreqx1000 - 25000)
				< SRCCAP_TIMINGDETECT_VFREQ_TOLERANCE_LOW)
			|| (abs(timing_table[*table_index].vfreqx1000 - 24000)
				< SRCCAP_TIMINGDETECT_VFREQ_TOLERANCE_LOW))
			vfreq_tolerance = SRCCAP_TIMINGDETECT_VFREQ_TOLERANCE_LOW;
		else
			vfreq_tolerance = SRCCAP_TIMINGDETECT_VFREQ_TOLERANCE;

		/* increase hfreq tolerance if hfreqx10 is greater than 1000 */
		if (timing_table[*table_index].hfreqx10 > 1000)
			hfreq_tolerance = SRCCAP_TIMINGDETECT_HFREQ_TOLERANCE_HIGH;
		else
			hfreq_tolerance = SRCCAP_TIMINGDETECT_HFREQ_TOLERANCE;

		/* check hfreq */
		if (abs(timing_table[*table_index].hfreqx10 - hfreqx10) >= hfreq_tolerance)
			continue;

		/* check vfreq */
		if (abs(timing_table[*table_index].vfreqx1000 - vfreqx1000) >= vfreq_tolerance)
			continue;

		/* check vtt */
		if (abs(timing_table[*table_index].vtt - vtt) >= SRCCAP_TIMINGDETECT_VTT_TOLERANCE)
			continue;

		SRCCAP_MSG_INFO("table index:%u\n", *table_index);
		SRCCAP_MSG_INFO("%s:%u, %s:%u, %s:%u, %s:%u, %s:%u, %s:%u, %s:%u, %s:%u, %s:%x\n",
			"hfreqx10", timing_table[*table_index].hfreqx10,
			"vfreqx1000", timing_table[*table_index].vfreqx1000,
			"hstart", timing_table[*table_index].hstart,
			"vstart", timing_table[*table_index].vstart,
			"hde", timing_table[*table_index].hde,
			"vde", timing_table[*table_index].vde,
			"htt", timing_table[*table_index].htt,
			"vtt", timing_table[*table_index].vtt,
			"adcphase", timing_table[*table_index].adcphase,
			"status", timing_table[*table_index].status);

		break;
	}

	if (*table_index == table_entry_count) {
		SRCCAP_MSG_ERROR("timing not supported\n");
		return -ERANGE;
	}

	return 0;
}


static bool timingdetect_check_null_pointer(
	u32 *hpd,
	u32 *vtt,
	u32 *vpd,
	bool *interlaced,
	bool *hpol,
	bool *vpol,
	u16 *hstart,
	u16 *vstart,
	u16 *hend,
	u16 *vend,
	bool *yuv420,
	bool *frl,
	u32 *htt)
{
	bool ret = FALSE;

	if ((hpd == NULL) || (vtt == NULL) || (vpd == NULL) || (interlaced == NULL)
		|| (hpol == NULL) || (vpol == NULL) || (hstart == NULL) || (vstart == NULL)
		|| (hend == NULL) || (vend == NULL) || (yuv420 == NULL) || (frl == NULL)
		|| (htt == NULL))
		ret = TRUE;

	return ret;
}

static int timingdetect_get_sync_status(
	enum v4l2_srccap_input_source src,
	bool adc_offline_status,
	u32 *hpd,
	u32 *vtt)
{
	int ret = 0;
	enum reg_srccap_timingdetect_source detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCB;
	u16 hpd_u16 = 0, vtt_u16 = 0;

	if ((hpd == NULL) || (vtt == NULL))
		return -ENXIO;

	if (adc_offline_status == FALSE) {
		ret = timingdetect_map_detblock_reg(src, &detblock);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	ret = drv_hwreg_srccap_timingdetect_get_hperiod(detblock, &hpd_u16);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	*hpd = (u32)hpd_u16;

	ret = drv_hwreg_srccap_timingdetect_get_vtotal(detblock, &vtt_u16);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	*vtt = (u32)vtt_u16;

	return 0;
}


static int timingdetect_get_timing_info(
	enum v4l2_srccap_input_source src,
	u32 *hpd,
	u32 *vtt,
	u32 *vpd,
	bool *interlaced,
	bool *hpol,
	bool *vpol,
	u16 *hstart,
	u16 *vstart,
	u16 *hend,
	u16 *vend,
	bool *yuv420,
	bool *frl,
	u32 *htt)
{
	if (timingdetect_check_null_pointer(hpd, vtt, vpd, interlaced, hpol, vpol, hstart, vstart,
			hend, vend, yuv420, frl, htt)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	enum reg_srccap_timingdetect_source detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;
	u16 hpd_u16 = 0, vtt_u16 = 0, htt_u16 = 0;
	struct reg_srccap_timingdetect_de_info info;

	memset(&info, 0, sizeof(struct reg_srccap_timingdetect_de_info));

	ret = timingdetect_map_detblock_reg(src, &detblock);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_get_hperiod(detblock, &hpd_u16);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	*hpd = (u32)hpd_u16;

	ret = drv_hwreg_srccap_timingdetect_get_vtotal(detblock, &vtt_u16);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	*vtt = (u32)vtt_u16;

	ret = drv_hwreg_srccap_timingdetect_get_vsync2vsync_pix_cnt(detblock, vpd);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_get_interlace_status(detblock, interlaced);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_get_hsync_polarity(detblock, hpol);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_get_vsync_polarity(detblock, vpol);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_get_de_info(detblock, &info);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_get_htt(detblock, &htt_u16);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	*htt = (u32)htt_u16;

	ret = drv_hwreg_srccap_timingdetect_get_yuv420(detblock, yuv420);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_get_frl_mode(detblock, frl);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	*hstart = info.hstart;
	*vstart = info.vstart;
	*hend = info.hend;
	*vend = info.vend;

	return 0;
}

static int timingdetect_check_signal_sync(u32 hpd, u32 vtt, bool *has_sync)
{
	if (has_sync == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (((hpd > SRCCAP_TIMINGDETECT_HPD_MIN) && (hpd != SRCCAP_TIMINGDETECT_HPD_MASK))
		&& ((vtt > SRCCAP_TIMINGDETECT_VTT_MIN) && (vtt != SRCCAP_TIMINGDETECT_VTT_MASK)))
		*has_sync = TRUE;
	else
		*has_sync = FALSE;

	return 0;
}

static bool timingdetect_check_last_signal(
	u32 hpd,
	u32 old_hpd,
	u32 vtt,
	u32 old_vtt,
	bool interlaced,
	bool old_interlaced,
	bool hpol,
	bool old_hpol,
	bool vpol,
	bool old_vpol,
	bool bypass_v)
{
	int ret = FALSE;

	if ((hpd < SRCCAP_TIMINGDETECT_HPD_MIN
		|| hpd == SRCCAP_TIMINGDETECT_HPD_MASK)
		&& (old_hpd < SRCCAP_TIMINGDETECT_HPD_MIN
		|| old_hpd == SRCCAP_TIMINGDETECT_HPD_MASK)
		&& ((vtt < SRCCAP_TIMINGDETECT_VTT_MIN)
		|| (vtt == SRCCAP_TIMINGDETECT_VTT_MASK) || (bypass_v == TRUE))
		&& ((old_vtt < SRCCAP_TIMINGDETECT_VTT_MIN)
		|| (old_vtt == SRCCAP_TIMINGDETECT_VTT_MASK) || (bypass_v == TRUE)))
		ret = TRUE;

	return ret;
}

static bool timingdetect_check_last_timing(
	u32 hpd,
	u32 old_hpd,
	u32 vtt,
	u32 old_vtt,
	bool interlaced,
	bool old_interlaced,
	bool hpol,
	bool old_hpol,
	bool vpol,
	bool old_vpol,
	u16 hstart,
	u16 old_hstart,
	u16 vstart,
	u16 old_vstart,
	u16 hend,
	u16 old_hend,
	u16 vend,
	u16 old_vend,
	bool bypass_v)
{
	int ret = FALSE;

	if ((abs((s64)hpd - (s64)old_hpd) < SRCCAP_TIMINGDETECT_HPD_TOL)
		&& ((abs((s64)vtt - (s64)old_vtt) < SRCCAP_TIMINGDETECT_VTT_TOL)
			|| (bypass_v == TRUE))
		&& ((interlaced == old_interlaced)
			|| (bypass_v == TRUE))
		&& (hpol == old_hpol)
		&& (vpol == old_vpol)
		&& (hstart == old_hstart)
		&& (vstart == old_vstart)
		&& (hend == old_hend)
		&& (vend == old_vend))
		ret = TRUE;

	return ret;
}

static int timingdetect_compare_timing(
	u32 hpd,
	u32 old_hpd,
	u32 vtt,
	u32 old_vtt,
	bool interlaced,
	bool old_interlaced,
	bool hpol,
	bool old_hpol,
	bool vpol,
	bool old_vpol,
	u16 hstart,
	u16 old_hstart,
	u16 vstart,
	u16 old_vstart,
	u16 hend,
	u16 old_hend,
	u16 vend,
	u16 old_vend,
	enum srccap_timingdetect_vrr_type vrr_type,
	bool vrr_enforcement_en,
	bool hdmi_free_sync_status,
	bool *same_timing)
{
	int same_stable_sync = 0, same_no_signal = 0;
	bool bypass_v = FALSE;

	if (same_timing == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if ((vrr_type == SRCCAP_TIMINGDETECT_VRR_TYPE_VRR)
		|| (vrr_type == SRCCAP_TIMINGDETECT_VRR_TYPE_CVRR)
		|| vrr_enforcement_en == TRUE
		|| hdmi_free_sync_status == TRUE)
		bypass_v = TRUE;

	/* check if all timing info are the same as last time */
	same_stable_sync = timingdetect_check_last_timing(
		hpd, old_hpd,
		vtt, old_vtt,
		interlaced, old_interlaced,
		hpol, old_hpol,
		vpol, old_vpol,
		hstart, old_hstart,
		vstart, old_vstart,
		hend, old_hend,
		vend, old_vend,
		bypass_v);

	/* check if there is no signal like last time */
	same_no_signal = timingdetect_check_last_signal(
		hpd, old_hpd,
		vtt, old_vtt,
		interlaced, old_interlaced,
		hpol, old_hpol,
		vpol, old_vpol,
		bypass_v);

	if (same_stable_sync)
		*same_timing = TRUE;
	else if (same_no_signal)
		*same_timing = TRUE;
	else
		*same_timing = FALSE;

	return 0;
}

static void timingdetect_set_pixel_monitor_size(
	u16 dev_id,
	struct reg_srccap_timingdetect_window win)
{
	enum pxm_return ret = E_PXM_RET_FAIL;
	struct pxm_size_info info;

	memset(&info, 0, sizeof(struct pxm_size_info));

	if (dev_id == 0)
		info.point = E_PXM_POINT_PRE_IP2_0_INPUT;
	else if (dev_id == 1)
		info.point = E_PXM_POINT_PRE_IP2_1_INPUT;
	else {
		SRCCAP_MSG_ERROR("dev_id(%u) is invalid!\n", dev_id);
		return;
	}
	info.win_id = 0;  // srccap not support time sharing, win id set 0
	info.size.h_size = (__u16)win.w;
	info.size.v_size = (__u16)win.h;

	ret = pxm_set_size(&info);
	if (ret != E_PXM_RET_OK)
		SRCCAP_MSG_ERROR("srccap set pxm size fail(%d)!\n", ret);
}

static enum srccap_timingdetect_vrr_type timingdetect_get_vrr_status(
	enum v4l2_srccap_input_source src)
{
	if ((src < V4L2_SRCCAP_INPUT_SOURCE_HDMI)
		|| (src > V4L2_SRCCAP_INPUT_SOURCE_HDMI4))
		return SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR;

	int ret = 0;
	enum reg_srccap_timingdetect_vrr_type vrr = 0;
	enum reg_srccap_timingdetect_source detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;

	ret = timingdetect_map_detblock_reg(src, &detblock);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR;
	}

	ret = drv_hwreg_srccap_timingdetect_get_vrr(detblock, &vrr);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR;
	}

	if (vrr == REG_SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR)
		return SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR;
	else if (vrr == REG_SRCCAP_TIMINGDETECT_VRR_TYPE_VRR)
		return SRCCAP_TIMINGDETECT_VRR_TYPE_VRR;
	else if (vrr == REG_SRCCAP_TIMINGDETECT_VRR_TYPE_CVRR)
		return SRCCAP_TIMINGDETECT_VRR_TYPE_CVRR;
	else
		return SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR;
}

static enum srccap_timingdetect_vrr_type timingdetect_get_debounce_vrr_status(
	enum srccap_timingdetect_vrr_type *prev_vrr_type,
	enum v4l2_srccap_input_source src,
	bool hdmi_free_sync_status,
	u8 *debounce_cnt)
{
	enum srccap_timingdetect_vrr_type ret_vrr_type = SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR;
	//     NVRR  VRR   VVRR
	//NVRR x     x     x
	//VRR  v     x     x
	//CVRR x     x     x
	bool debounce_table[SRCCAP_TIMINGDETECT_VRR_TYPE_CNT][SRCCAP_TIMINGDETECT_VRR_TYPE_CNT] = {
									{FALSE, FALSE, FALSE},
									{TRUE, FALSE, FALSE},
									{FALSE, FALSE, FALSE}};
	if (prev_vrr_type == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR;
	}

	if (debounce_cnt == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR;
	}

	if (hdmi_free_sync_status == TRUE) {
		*debounce_cnt = 0;
		*prev_vrr_type = ret_vrr_type = SRCCAP_TIMINGDETECT_VRR_TYPE_VRR;
		return ret_vrr_type;
	}

	ret_vrr_type = timingdetect_get_vrr_status(src);

//	pr_err("[SRCCAP][%s][%d]debounce_cnt=%d current_vrr_type=%d prev_vrr_type=%d !",
//				__func__, __LINE__, *debounce_cnt, ret_vrr_type, *prev_vrr_type);

	if (debounce_table[*prev_vrr_type][ret_vrr_type]) {
		if (*debounce_cnt < SRCCAP_TIMINGDETECT_VRR_DEBOUNCE_CNT) {
			(*debounce_cnt)++;
			ret_vrr_type = *prev_vrr_type;
		} else {
			*debounce_cnt = 0;
			*prev_vrr_type = ret_vrr_type;
		}
	} else {
		*debounce_cnt = 0;
		*prev_vrr_type = ret_vrr_type;
	}

//	pr_err("[SRCCAP][%s][%d]ret_vrr_type=%d !", __func__, __LINE__, ret_vrr_type);

	return ret_vrr_type;

}

static void timingdetect_calculate_freq(
	struct mtk_srccap_dev *srccap_dev,
	u32 hpd, u32 vpd, u32 xtal_clk,
	u32 *hfreqx10, u64 *vfreqx1000)
{
	u64 quotient = 0;
	u64 half_hpd = 0, half_vpd = 0;

	half_hpd = (u64)hpd;
	do_div(half_hpd, SRCCAP_TIMINGDETECT_DIVIDE_BASE_2);
	half_vpd = (u64)vpd;
	do_div(half_vpd, SRCCAP_TIMINGDETECT_DIVIDE_BASE_2);

	if ((hpd != 0) && (hfreqx10 != NULL)) {
		quotient = (xtal_clk * 10UL) + half_hpd; // round hfreqx10
		do_div(quotient, hpd);
		do_div(quotient, 1000UL);
		*hfreqx10 = (u32)quotient;
	}

	if ((vpd != 0) && (vfreqx1000 != NULL) && (srccap_dev != NULL)) {
		if (srccap_dev->srccap_info.cap.hw_version == 1) {
			quotient = (24000000UL * 1000UL) + half_vpd; // round vfreqx1000
			do_div(quotient, vpd);
			*vfreqx1000 = quotient;
		} else {
			quotient = ((u64)xtal_clk * 1000UL) + half_vpd; // round vfreqx1000
			do_div(quotient, vpd);
			*vfreqx1000 = quotient;
		}
	}
}


static int timingdetect_calculate_refreshrate(u16 v_de, u16 h_period, u32 xtal_clk)
{
	u16 v_total = 0;

	v_total = v_de * SRCCAP_TIMINGDETECT_TIMING_HDMI21_VTT_THRESHOLD /
		SRCCAP_TIMINGDETECT_TIMING_HDMI21_VDE_THRESHOLD;

	if (h_period > 0 && v_total > 0)
		return ((u16)(xtal_clk*10/h_period/v_total));
	else
		return 0;
}

static void timingdetect_disable_clk(struct clk *clk)
{
	if (__clk_is_enabled(clk))
		clk_disable_unprepare(clk);
}

static int timingdetect_set_auto_nosignal(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	bool enable = FALSE;
	enum reg_srccap_timingdetect_source detsrc = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;

	ret = timingdetect_map_detblock_reg(srccap_dev->srccap_info.src, &detsrc);
	if (ret)
		goto exit;

	if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_BYPASSMODE)
		enable = TRUE;

	ret = drv_hwreg_srccap_timingdetect_set_main_det_mute_en(detsrc, enable, TRUE, NULL, NULL);
	if (ret < 0)
		goto exit;

	ret = drv_hwreg_srccap_timingdetect_set_hdmi_det_mute_en(detsrc, enable, TRUE, NULL, NULL);
	if (ret < 0)
		goto exit;

	ret = drv_hwreg_srccap_timingdetect_set_hdmi_vmute_det_en(detsrc, enable, TRUE, NULL, NULL);
	if (ret < 0)
		goto exit;

	ret = drv_hwreg_srccap_timingdetect_set_ans_function(detsrc, enable, TRUE, NULL, NULL);
	if (ret < 0)
		goto exit;

	return ret;
exit:
	SRCCAP_MSG_RETURN_CHECK(ret);
	return ret;
}

static int timingdetect_reset(enum reg_srccap_timingdetect_source detblock)
{
	int ret = 0;

	ret = drv_hwreg_srccap_timingdetect_reset(detblock, TRUE, TRUE, NULL, NULL);
	if (ret < 0)
		goto exit;

	usleep_range(SRCCAP_TIMINGDETECT_RESET_DELAY_1000, SRCCAP_TIMINGDETECT_RESET_DELAY_1100);

	ret = drv_hwreg_srccap_timingdetect_reset(detblock, FALSE, TRUE, NULL, NULL);
	if (ret < 0)
		goto exit;

	usleep_range(SRCCAP_TIMINGDETECT_RESET_DELAY_1000, SRCCAP_TIMINGDETECT_RESET_DELAY_1100);

	return ret;
exit:
	SRCCAP_MSG_RETURN_CHECK(ret);
	return ret;
}

static int timingdetect_obtain_timing_parameter(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_timingdetect_data *data,
	enum srccap_timingdetect_vrr_type vrr_type,
	u16 hend,
	u16 vend,
	u16 index,
	u16 hstart,
	u16 vstart,
	u32 hpd,
	u32 htt,
	u32 vtt,
	u32 hfreqx10,
	u64 vfreqx1000,
	bool frl,
	bool hpol,
	bool vpol,
	bool yuv420,
	bool interlaced,
	bool non_ce_timing,
	bool hdmi_free_sync_status)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (data == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct srccap_timingdetect_timing *timing_table;
	struct srccap_timingdetect_vdsampling *vdsampling_table;

	switch (srccap_dev->srccap_info.src) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		timing_table = srccap_dev->timingdetect_info.timing_table;

		if (yuv420)
			data->h_de = (hend - hstart + 1) * SRCCAP_TIMINGDETECT_YUV420_MULTIPLIER;
		else
			data->h_de = hend - hstart + 1;

		if (frl)
			data->h_de = (data->h_de) * SRCCAP_TIMINGDETECT_FRL_8P_MULTIPLIER;

		if (!non_ce_timing) {
			data->v_freqx1000 = timing_table[index].vfreqx1000;
			data->h_freqx10 = timing_table[index].hfreqx10;
		} else {
			data->v_freqx1000 = vfreqx1000;
			data->h_freqx10 = hfreqx10;
		}

		if (vrr_type == SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR || hdmi_free_sync_status != TRUE)
			data->interlaced = interlaced;
		else
			data->interlaced = 0;

		if (data->interlaced)
			data->v_de = vend - vstart + SRCCAP_TIMINGDETECT_INTERLACE_ADDEND;
		else
			data->v_de = vend - vstart + 1;

		if (non_ce_timing == TRUE) {
			data->adc_phase = 0;
			data->ce_timing = FALSE;
		} else {
			data->adc_phase = timing_table[index].adcphase;
			data->ce_timing = ((timing_table[index].status
				& SRCCAP_TIMINGDETECT_FLAG_CE_TIMING) >>
				SRCCAP_TIMINGDETECT_CE_TIMING_BIT);
		}

		data->h_start = hstart;
		data->v_start = vstart;
		data->vrr_type = vrr_type;
		data->h_period = hpd;
		data->h_total = htt;
		data->v_total = vtt;
		data->h_pol = hpol;
		data->v_pol = vpol;
		data->yuv420 = yuv420;
		data->frl = frl;
		data->refresh_rate = timingdetect_calculate_refreshrate(
			data->v_de, data->h_period, srccap_dev->srccap_info.cap.xtal_clk);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		timing_table = srccap_dev->timingdetect_info.timing_table;

		data->h_start = timing_table[index].hstart;
		data->v_start = timing_table[index].vstart;
		data->h_de = timing_table[index].hde;
		data->v_de = timing_table[index].vde;
		data->h_freqx10 = timing_table[index].hfreqx10;
		data->v_freqx1000 = timing_table[index].vfreqx1000;
		data->interlaced = interlaced;
		data->h_period = hpd;
		data->h_total = timing_table[index].htt;
		data->v_total = timing_table[index].vtt;
		data->h_pol = hpol;
		data->v_pol = vpol;
		data->adc_phase = timing_table[index].adcphase;
		data->yuv420 = yuv420;
		data->frl = frl;
		data->ce_timing = ((timing_table[index].status
			& SRCCAP_TIMINGDETECT_FLAG_CE_TIMING) >> SRCCAP_TIMINGDETECT_CE_TIMING_BIT);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		vdsampling_table = srccap_dev->timingdetect_info.vdsampling_table;

		data->h_start = vdsampling_table[index].h_start;
		data->v_start = vdsampling_table[index].v_start;
		data->h_de = vdsampling_table[index].h_de;
		data->v_de = vdsampling_table[index].v_de;
		data->v_freqx1000 = vdsampling_table[index].v_freqx1000;
		data->h_freqx10 = hfreqx10;
		data->interlaced = true;
		data->h_total = vdsampling_table[index].h_total;
		data->h_period = hpd;
		data->v_total = vtt;
		data->h_pol = hpol;
		data->v_pol = vpol;
		data->yuv420 = yuv420;
		data->frl = frl;
		break;
	default:
		break;
	}
	return 0;
}

static int timingdetect_set_hw_version(
	u32 hw_version,
	bool stub)
{
	int ret = 0;

	ret = drv_hwreg_srccap_timingdetect_set_hw_version(hw_version, stub);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

/* ============================================================================================== */
/* --------------------------------------- v4l2_ctrl_ops ---------------------------------------- */
/* ============================================================================================== */
static int mtk_timingdetect_get_status(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_timingdetect_status *status)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (status == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	*status = srccap_dev->timingdetect_info.status;

	return 0;
}

static int mtk_timingdetect_get_timing(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_timingdetect_timing *timing)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (timing == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	timing->vfreqx1000 = srccap_dev->timingdetect_info.data.v_freqx1000;
	timing->hstart = srccap_dev->timingdetect_info.data.h_start;
	timing->vstart = srccap_dev->timingdetect_info.data.v_start;
	timing->hde = srccap_dev->timingdetect_info.data.h_de;
	timing->vde = srccap_dev->timingdetect_info.data.v_de;
	timing->htt = srccap_dev->timingdetect_info.data.h_total;
	timing->vtt = srccap_dev->timingdetect_info.data.v_total;
	timing->adcphase = srccap_dev->timingdetect_info.data.adc_phase;
	timing->interlaced = srccap_dev->timingdetect_info.data.interlaced;
	timing->ce_timing = srccap_dev->timingdetect_info.data.ce_timing;

	SRCCAP_MSG_INFO(
		"[CTRL_FLOW]g_timing(%u): %s=%u %s=%u %s=%u %s=%u %s=%u %s=%u %s=%u\n",
		srccap_dev->dev_id,
		"vfreq1000", timing->vfreqx1000,
		"hstart", timing->hstart,
		"vstart", timing->vstart,
		"hde", timing->hde,
		"vde", timing->vde,
		"htt", timing->htt,
		"vtt", timing->vtt);
	SRCCAP_MSG_INFO(
		"%s=%u %s=%d %s=%d\n",
		"adcphase", timing->adcphase,
		"interlaced", timing->interlaced,
		"ce", timing->ce_timing);

	return 0;
}

static int mtk_timingdetect_force_vrr(struct mtk_srccap_dev *srccap_dev, bool vrr)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;

	srccap_dev->timingdetect_info.data.vrr_enforcement_en = vrr;

	ret = mtk_timingdetect_set_ans_vtt_det_en(srccap_dev, vrr);
	if (ret < 0)
		goto exit;

	if (vrr) {
		ret = mtk_timingdetect_set_auto_no_signal_mute(srccap_dev, FALSE);
		if (ret < 0)
			goto exit;
	}

	return ret;
exit:
	SRCCAP_MSG_RETURN_CHECK(ret);
	return ret;
}

static int _timingdetect_g_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, timingdetect_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_TIMINGDETECT_STATUS:
	{
		enum v4l2_timingdetect_status status = V4L2_TIMINGDETECT_NO_SIGNAL;

		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_STATUS\n");

		ret = mtk_timingdetect_get_status(srccap_dev, &status);
		ctrl->val = (s32)status;
		break;
	}
	case V4L2_CID_TIMINGDETECT_TIMING:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_TIMING\n");

		ret = mtk_timingdetect_get_timing(srccap_dev,
			(struct v4l2_timingdetect_timing *)(ctrl->p_new.p_u8));
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _timingdetect_s_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, timingdetect_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_TIMINGDETECT_VRR_ENFORCEMENT:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_TIMINGDETECT_VRR_ENFORCEMENT\n");

		bool vrr = (bool)(ctrl->val);

		ret = mtk_timingdetect_force_vrr(srccap_dev, vrr);
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops timingdetect_ctrl_ops = {
	.g_volatile_ctrl = _timingdetect_g_ctrl,
	.s_ctrl = _timingdetect_s_ctrl,
};

static const struct v4l2_ctrl_config timingdetect_ctrl[] = {
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_STATUS,
		.name = "Srccap TimingDetect Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_TIMING,
		.name = "Srccap TimingDetect Timing",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_timingdetect_timing)},
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &timingdetect_ctrl_ops,
		.id = V4L2_CID_TIMINGDETECT_VRR_ENFORCEMENT,
		.name = "Srccap TimingDetect VRR Enforce",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = 0,
		.max = 1,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			|V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops timingdetect_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops timingdetect_sd_internal_ops = {
};

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_timingdetect_init(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	enum reg_srccap_timingdetect_source detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;

	ret = timingdetect_set_hw_version(srccap_dev->srccap_info.cap.hw_version, FALSE);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* set a default source for each timingdetect hardware block */
	ret = timingdetect_set_source(srccap_dev, REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA,
		V4L2_SRCCAP_INPUT_SOURCE_YPBPR);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = timingdetect_set_source(srccap_dev, REG_SRCCAP_TIMINGDETECT_SOURCE_ADCB,
		V4L2_SRCCAP_INPUT_SOURCE_YPBPR);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = timingdetect_set_source(srccap_dev, REG_SRCCAP_TIMINGDETECT_SOURCE_VD,
		V4L2_SRCCAP_INPUT_SOURCE_CVBS);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = timingdetect_set_source(srccap_dev, REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI,
		V4L2_SRCCAP_INPUT_SOURCE_HDMI);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = timingdetect_set_source(srccap_dev, REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI2,
		V4L2_SRCCAP_INPUT_SOURCE_HDMI2);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = timingdetect_set_source(srccap_dev, REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI3,
		V4L2_SRCCAP_INPUT_SOURCE_HDMI3);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = timingdetect_set_source(srccap_dev, REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI4,
		V4L2_SRCCAP_INPUT_SOURCE_HDMI4);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* disable auto no signal mute for all timingdetect hardware blocks */
	for (detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;
		detblock <= REG_SRCCAP_TIMINGDETECT_SOURCE_HDMI4;
		detblock++) {
		ret = drv_hwreg_srccap_timingdetect_set_ans_mute(detblock, FALSE, TRUE, NULL, NULL);
		if (ret) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	return 0;
}

int mtk_timingdetect_init_offline(
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	enum reg_srccap_timingdetect_source detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCB;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = timingdetect_reset(detblock);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

int mtk_timingdetect_init_clock(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	struct clk *clk = NULL, *parent = NULL;

	/* set timingdetect xtal clock */
	clk = srccap_dev->srccap_info.clk.ip1_xtal_ck;
	if (srccap_dev->srccap_info.cap.xtal_clk == 12000000)
		ret = timingdetect_enable_parent(srccap_dev, clk, "srccap_ip1_xtal_ck_xtal_12m_ck");
	else if (srccap_dev->srccap_info.cap.xtal_clk == 24000000)
		ret = timingdetect_enable_parent(srccap_dev, clk, "srccap_ip1_xtal_ck_xtal_24m_ck");
	else
		ret = timingdetect_enable_parent(srccap_dev, clk, "srccap_ip1_xtal_ck_xtal_48m_ck");
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	/* set adc timingdetect clock */
	clk = srccap_dev->srccap_info.clk.adc_ck;
	ret = timingdetect_enable_parent(srccap_dev, clk, "srccap_adc_ck_adc_ck");
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	/* set hdmi timingdetect clock */
	clk = srccap_dev->srccap_info.clk.hdmi_ck;
	ret = timingdetect_enable_parent(srccap_dev, clk, "srccap_hdmi_ck_hdmi_ck");
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	/* set hdmi2 timingdetect clock */
	clk = srccap_dev->srccap_info.clk.hdmi2_ck;
	ret = timingdetect_enable_parent(srccap_dev, clk, "srccap_hdmi2_ck_hdmi2_ck");
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	/* set hdmi3 timingdetect clock */
	clk = srccap_dev->srccap_info.clk.hdmi3_ck;
	ret = timingdetect_enable_parent(srccap_dev, clk, "srccap_hdmi3_ck_hdmi3_ck");
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	/* set hdmi4 timingdetect clock */
	clk = srccap_dev->srccap_info.clk.hdmi4_ck;
	ret = timingdetect_enable_parent(srccap_dev, clk, "srccap_hdmi4_ck_hdmi4_ck");
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	return 0;
}

int mtk_timingdetect_deinit(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct clk *clk = NULL;

	if (srccap_dev->dev_id == 0) {
		clk = srccap_dev->avd_info.stclock.cmbai2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.cmbao2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.cmbbi2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.cmbbo2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.mcu_mail02vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.mcu_mail12vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.smi2mcu_m2mcu;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.smi2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.vd2x2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.vd_32fsc2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.vd2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.xtal_12m2vd;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.xtal_12m2mcu_m2riu;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.xtal_12m2mcu_m2mcu;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.clk_vd_atv_input;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.clk_vd_cvbs_input;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->avd_info.stclock.clk_buf_sel;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.ip1_xtal_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.hdmi_idclk_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.hdmi2_idclk_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.hdmi3_idclk_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.hdmi4_idclk_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.adc_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.hdmi_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.hdmi2_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.hdmi3_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.hdmi4_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
		clk = srccap_dev->srccap_info.clk.ipdma0_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
	} else if (srccap_dev->dev_id == 1) {
		clk = srccap_dev->srccap_info.clk.ipdma1_ck;
		timingdetect_put_clk(srccap_dev->dev, clk);
	}

	return 0;
}

int mtk_timingdetect_deinit_clock(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct clk *clk = NULL;

	clk = srccap_dev->srccap_info.clk.ip1_xtal_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.adc_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.hdmi_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.hdmi2_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.hdmi3_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.hdmi4_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.hdmi_idclk_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.hdmi2_idclk_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.hdmi3_idclk_ck;
	timingdetect_disable_clk(clk);
	clk = srccap_dev->srccap_info.clk.hdmi4_idclk_ck;
	timingdetect_disable_clk(clk);

	return 0;
}

int mtk_timingdetect_set_capture_win(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_rect cap_win)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	enum reg_srccap_timingdetect_source src = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;
	struct reg_srccap_timingdetect_window win;

	memset(&win, 0, sizeof(struct reg_srccap_timingdetect_window));

	ret = timingdetect_map_detblock_reg(srccap_dev->srccap_info.src, &src);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	win.x = cap_win.left;
	win.y = cap_win.top;
	win.w = cap_win.width;
	win.h = cap_win.height;

	if (srccap_dev->srccap_info.stub_mode)
		ret = drv_hwreg_srccap_timingdetect_set_capture_win(src, win, TRUE, NULL, NULL);
	else {
#if IS_ENABLED(CONFIG_OPTEE)
		TEEC_Session *pstSession = srccap_dev->memout_info.secure_info.pstSession;
		TEEC_Operation op = {0};
		u32 error = 0;

		op.params[SRCCAP_CA_SMC_PARAM_IDX_0].tmpref.buffer = (void *)&win;
		op.params[SRCCAP_CA_SMC_PARAM_IDX_0].tmpref.size =
				sizeof(struct reg_srccap_timingdetect_window);
		op.params[SRCCAP_CA_SMC_PARAM_IDX_1].value.a = (u32)src;
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
				TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE);
		ret = mtk_common_ca_teec_invoke_cmd(srccap_dev, pstSession,
				E_XC_OPTEE_SET_IPCAP_WIN, &op, &error);
#else
		ret = drv_hwreg_srccap_timingdetect_set_capture_win(src, win, TRUE, NULL, NULL);
#endif
	}
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	timingdetect_set_pixel_monitor_size(srccap_dev->dev_id, win);

	return 0;
}

int mtk_timingdetect_force_progressive(struct mtk_srccap_dev *srccap_dev, bool force_en)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	enum reg_srccap_timingdetect_source detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;
	struct reg_srccap_timingdetect_user_scantype scantype;

	memset(&scantype, 0, sizeof(struct reg_srccap_timingdetect_user_scantype));

	scantype.en = force_en;
	scantype.interlaced = 0;
	ret = timingdetect_map_detblock_reg(srccap_dev->srccap_info.src, &detblock);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_set_user_scantype(
		detblock, scantype, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

int mtk_timingdetect_retrieve_timing(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_timingdetect_vrr_type vrr_type,
	bool hdmi_free_sync_status,
	struct srccap_timingdetect_data *data)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (data == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	u16 table_entry_count = 0, table_index = 0;
	struct srccap_timingdetect_timing *timing_table;
	struct srccap_timingdetect_vdsampling *vdsampling_table;
	u32 xtal_clk = srccap_dev->srccap_info.cap.xtal_clk;
	u32 hfreqx10 = 0, hpd = 0, vtt = 0, vpd = 0, htt = 0;
	u64 vfreqx1000 = 0, refresh_rate = 0;
	u16 hstart = 0, vstart = 0, hend = 0, vend = 0;
	bool interlaced = 0, hpol = 0, vpol = 0, yuv420 = 0, frl = 0, non_ce_timing = 0;
	enum v4l2_ext_avd_videostandardtype videotype = V4L2_EXT_AVD_STD_PAL_BGHI;

	/* get hpd, vtt, vpd, interlaced, hpol, vpol, hstart, vstart, hend, and vend */
	ret = timingdetect_get_timing_info(srccap_dev->srccap_info.src,
		&hpd, &vtt, &vpd, &interlaced, &hpol, &vpol, &hstart, &vstart, &hend, &vend,
		&yuv420, &frl, &htt);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	switch (srccap_dev->srccap_info.src) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		/* match timing from timing table */
		table_entry_count = srccap_dev->timingdetect_info.table_entry_count;
		timing_table = srccap_dev->timingdetect_info.timing_table;

		timingdetect_calculate_freq(srccap_dev, hpd, vpd, xtal_clk, &hfreqx10, &vfreqx1000);

		if ((vrr_type != SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR
			|| (srccap_dev->timingdetect_info.data.vrr_enforcement_en == TRUE)
			|| (hdmi_free_sync_status == TRUE))
			&& (srccap_dev->srccap_info.src <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4)
			&& (srccap_dev->srccap_info.src >= V4L2_SRCCAP_INPUT_SOURCE_HDMI))
			non_ce_timing = TRUE;

		if (!non_ce_timing) {
			ret = timingdetect_match_timing(hpol, vpol, interlaced,
				hfreqx10, (u32)vfreqx1000, vtt,
				timing_table, table_entry_count,
				srccap_dev->srccap_info.src,
				&table_index);
			if (ret < 0) {
				if (ret == -ERANGE) {
					SRCCAP_MSG_ERROR("Not CE timing\n");
					non_ce_timing = TRUE;
					ret = 0;
				} else {
					SRCCAP_MSG_RETURN_CHECK(ret);
					return ret;
				}
			}
		}

		/* save data */
		ret = timingdetect_obtain_timing_parameter(srccap_dev, data, vrr_type, hend, vend,
			table_index, hstart, vstart, hpd, htt, vtt, hfreqx10, vfreqx1000, frl, hpol,
			vpol, yuv420, interlaced, non_ce_timing, hdmi_free_sync_status);

		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		/* match timing from timing table */
		table_entry_count = srccap_dev->timingdetect_info.table_entry_count;
		timing_table = srccap_dev->timingdetect_info.timing_table;

		timingdetect_calculate_freq(srccap_dev, hpd, vpd, xtal_clk, &hfreqx10, &vfreqx1000);

		ret = timingdetect_match_timing(hpol, vpol, interlaced, hfreqx10, (u32)vfreqx1000,
			vtt, timing_table, table_entry_count,
			srccap_dev->srccap_info.src,
			&table_index);
		if (ret < 0) {
			if (ret == -ERANGE)
				SRCCAP_MSG_ERROR("timing not found\n");
			else
				SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		/* save data */
		ret = timingdetect_obtain_timing_parameter(srccap_dev, data, vrr_type, hend, vend,
			table_index, hstart, vstart, hpd, htt, vtt, hfreqx10, vfreqx1000, frl, hpol,
			vpol, yuv420, interlaced, non_ce_timing, hdmi_free_sync_status);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		// get vd sampling table
		table_entry_count = srccap_dev->timingdetect_info.vdsampling_table_entry_count;
		vdsampling_table = srccap_dev->timingdetect_info.vdsampling_table;
		videotype = srccap_dev->avd_info.eStableTvsystem;

		if (hpd != 0)
			hfreqx10 = ((xtal_clk * 10) + (hpd / 2)) / hpd / 1000; // round hfreqx10

		ret = timingdetect_match_vd_sampling(videotype, vdsampling_table,
			table_entry_count, &table_index);
		if (ret < 0) {
			if (ret == -ERANGE)
				SRCCAP_MSG_ERROR("vd sampling not found\n");
			else
				SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}


		/* save data */
		ret = timingdetect_obtain_timing_parameter(srccap_dev, data, vrr_type, hend, vend,
			table_index, hstart, vstart, hpd, htt, vtt, hfreqx10, vfreqx1000, frl, hpol,
			vpol, yuv420, interlaced, non_ce_timing, hdmi_free_sync_status);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}
#define PATCH_ALIGN_4 (4)
int mtk_timingdetect_store_timing_info(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_timingdetect_data data)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	srccap_dev->timingdetect_info.data.h_start = data.h_start;
	srccap_dev->timingdetect_info.data.v_start = data.v_start;
	srccap_dev->timingdetect_info.data.h_de = data.h_de;
	srccap_dev->timingdetect_info.data.v_de = data.v_de;
	srccap_dev->timingdetect_info.data.h_freqx10 = data.h_freqx10;
	srccap_dev->timingdetect_info.data.v_freqx1000 = data.v_freqx1000;
	srccap_dev->timingdetect_info.data.interlaced = data.interlaced;
	srccap_dev->timingdetect_info.data.h_period = data.h_period;
	srccap_dev->timingdetect_info.data.h_total = data.h_total;
	srccap_dev->timingdetect_info.data.v_total = data.v_total;
	srccap_dev->timingdetect_info.data.h_pol = data.h_pol;
	srccap_dev->timingdetect_info.data.v_pol = data.v_pol;
	srccap_dev->timingdetect_info.data.adc_phase = data.adc_phase;
	srccap_dev->timingdetect_info.data.ce_timing = data.ce_timing;
	srccap_dev->timingdetect_info.data.yuv420 = data.yuv420;
	srccap_dev->timingdetect_info.data.frl = data.frl;
	srccap_dev->timingdetect_info.data.colorformat = data.colorformat;
	srccap_dev->timingdetect_info.data.vrr_type = data.vrr_type;
	srccap_dev->timingdetect_info.data.refresh_rate = data.refresh_rate;

	/* patch starts */
	if (srccap_dev->timingdetect_info.data.interlaced)
		srccap_dev->timingdetect_info.data.v_de =
			srccap_dev->timingdetect_info.data.v_de
			/ PATCH_ALIGN_4 * PATCH_ALIGN_4;
	/* patch ends */

	SRCCAP_MSG_INFO(
		"[CTRL_FLOW]s_timing(%u): %s=%u %s=%u %s=%u %s=%u %s=%u %s=%u %s=%d %s=%u\n",
		srccap_dev->dev_id,
		"hstart", srccap_dev->timingdetect_info.data.h_start,
		"vstart", srccap_dev->timingdetect_info.data.v_start,
		"hde", srccap_dev->timingdetect_info.data.h_de,
		"vde", srccap_dev->timingdetect_info.data.v_de,
		"hfreq10", srccap_dev->timingdetect_info.data.h_freqx10,
		"vfreq1000", srccap_dev->timingdetect_info.data.v_freqx1000,
		"interlaced", srccap_dev->timingdetect_info.data.interlaced,
		"hpd", srccap_dev->timingdetect_info.data.h_period);
	SRCCAP_MSG_INFO(
		"%s=%u %s=%u %s=%d %s=%d %s=%u %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d\n",
		"htt", srccap_dev->timingdetect_info.data.h_total,
		"vtt", srccap_dev->timingdetect_info.data.v_total,
		"hpol", srccap_dev->timingdetect_info.data.h_pol,
		"vpol", srccap_dev->timingdetect_info.data.v_pol,
		"adcphase", srccap_dev->timingdetect_info.data.adc_phase,
		"ce", srccap_dev->timingdetect_info.data.ce_timing,
		"yuv420", srccap_dev->timingdetect_info.data.yuv420,
		"frl", srccap_dev->timingdetect_info.data.frl,
		"colorfmt", srccap_dev->timingdetect_info.data.colorformat,
		"vrrtype", srccap_dev->timingdetect_info.data.vrr_type,
		"refreshrate", srccap_dev->timingdetect_info.data.refresh_rate);

	return 0;
}

int mtk_timingdetect_get_signal_status(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_timingdetect_vrr_type *vrr_type,
	bool hdmi_free_sync_status,
	enum v4l2_timingdetect_status *status)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (status == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (vrr_type == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u16 dev_id = srccap_dev->dev_id;
	static u64 count[SRCCAP_DEV_NUM];
	static enum v4l2_timingdetect_status old_status[SRCCAP_DEV_NUM];
	static u32 old_hpd[SRCCAP_DEV_NUM], old_vtt[SRCCAP_DEV_NUM];
	static bool old_interlaced[SRCCAP_DEV_NUM];
	static bool old_hpol[SRCCAP_DEV_NUM], old_vpol[SRCCAP_DEV_NUM];
	static u16 old_hstart[SRCCAP_DEV_NUM], old_vstart[SRCCAP_DEV_NUM];
	static u16 old_hend[SRCCAP_DEV_NUM], old_vend[SRCCAP_DEV_NUM];
	u32 hpd = 0, vtt = 0, vpd = 0, htt = 0;
	bool interlaced = 0, hpol = 0, vpol = 0, yuv420 = 0, frl = 0;
	u16 hstart = 0, vstart = 0, hend = 0, vend = 0;
	bool has_sync = FALSE, same_timing = FALSE;
	static u8 vrr_debounce_count[SRCCAP_DEV_NUM];
	static enum srccap_timingdetect_vrr_type last_vrr_type[SRCCAP_DEV_NUM];

	/* check if signal is vrr */
	*vrr_type = timingdetect_get_debounce_vrr_status(&last_vrr_type[dev_id],
							srccap_dev->srccap_info.src,
							hdmi_free_sync_status,
							&vrr_debounce_count[dev_id]);

	/* get hpd, vtt, vpd, interlaced, hpol, vpol, hstart, vstart, hend, and vend */
	ret = timingdetect_get_timing_info(srccap_dev->srccap_info.src,
		&hpd, &vtt, &vpd, &interlaced, &hpol, &vpol, &hstart, &vstart, &hend, &vend,
		&yuv420, &frl, &htt);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* check if signal has sync */
	ret = timingdetect_check_signal_sync(hpd, vtt, &has_sync);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* check if timing is the same */
	ret = timingdetect_compare_timing(hpd, old_hpd[dev_id], vtt, old_vtt[dev_id],
		interlaced, old_interlaced[dev_id],
		hpol, old_hpol[dev_id], vpol, old_vpol[dev_id],
		hstart, old_hstart[dev_id], vstart, old_vstart[dev_id],
		hend, old_hend[dev_id], vend, old_vend[dev_id],
		*vrr_type, srccap_dev->timingdetect_info.data.vrr_enforcement_en,
		hdmi_free_sync_status, &same_timing);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	switch (old_status[dev_id]) {
	case V4L2_TIMINGDETECT_NO_SIGNAL:
		/* if there's signal, enter unstable sync */
		if (has_sync) {
			count[dev_id] = 0;
			*status = V4L2_TIMINGDETECT_UNSTABLE_SYNC;
		/* else do nothing */
		} else
			*status = V4L2_TIMINGDETECT_NO_SIGNAL;
		break;
	case V4L2_TIMINGDETECT_UNSTABLE_SYNC:
		/* if signal is the same, add count */
		if (same_timing) {
			count[dev_id]++;
			/* if count reaches stable count, enter no signal or stable state */
			if (count[dev_id] >= SRCCAP_TIMINGDETECT_STABLE_CNT) {
				if (has_sync) {
					count[dev_id] = 0;
					*status = V4L2_TIMINGDETECT_STABLE_SYNC;
				} else {
					count[dev_id] = 0;
					*status = V4L2_TIMINGDETECT_NO_SIGNAL;
				}
			/* else do nothing */
			} else
				*status = V4L2_TIMINGDETECT_UNSTABLE_SYNC;
		/* else reset count */
		} else {
			count[dev_id] = 0;
			*status = V4L2_TIMINGDETECT_UNSTABLE_SYNC;
		}
		break;
	case V4L2_TIMINGDETECT_STABLE_SYNC:
		/* if signal is the same do nothing */
		if (has_sync && same_timing)
			*status = V4L2_TIMINGDETECT_STABLE_SYNC;
		/* else enter unstable state */
		else {
			count[dev_id] = 0;
			*status = V4L2_TIMINGDETECT_UNSTABLE_SYNC;
		}
		break;
	default:
		SRCCAP_MSG_ERROR("undefined status\n");
		break;
	}

	SRCCAP_MSG_DEBUG("%s:%u->%u, %s:%u->%u, %s:%u->%u, %s:%u->%u, %s:%u->%u\n",
		"hpd", old_hpd[dev_id], hpd,
		"vtt", old_vtt[dev_id], vtt,
		"interlaced", old_interlaced[dev_id], interlaced,
		"hpol", old_hpol[dev_id], hpol,
		"vpol", old_vpol[dev_id], vpol);
	SRCCAP_MSG_DEBUG("%s:%u->%u, %s:%u->%u, %s:%u->%u, %s:%u->%u\n",
		"hstart", old_hstart[dev_id], hstart,
		"vstart", old_vstart[dev_id], vstart,
		"hend", old_hend[dev_id], hend,
		"vend", old_vend[dev_id], vend);
	SRCCAP_MSG_DEBUG("%s:%u, %s:%u, %s:%u, %s:%lu, %s:%d->%d, %s:%d, %s:%d\n",
		"dev", dev_id,
		"same_timing", same_timing,
		"has_sync", has_sync,
		"count", count[dev_id],
		"status", old_status[dev_id], *status,
		"vrr_type", *vrr_type,
		"vrr_debounce_count", vrr_debounce_count[dev_id]);

	/* save timing info */
	old_hpd[dev_id] = hpd;
	old_vtt[dev_id] = vtt;
	old_interlaced[dev_id] = interlaced;
	old_hpol[dev_id] = hpol;
	old_vpol[dev_id] = vpol;
	old_hstart[dev_id] = hstart;
	old_vstart[dev_id] = vstart;
	old_hend[dev_id] = hend;
	old_vend[dev_id] = vend;
	old_status[dev_id] = *status;

	return 0;
}

int mtk_timingdetect_streamoff(
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	enum reg_srccap_timingdetect_source src = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;
	struct reg_srccap_timingdetect_window win;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	memset(&win, 0, sizeof(struct reg_srccap_timingdetect_window));

	ret = timingdetect_map_detblock_reg(srccap_dev->srccap_info.src, &src);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	win.x = 0;
	win.y = 0;
	win.w = 0;
	win.h = 0;

	ret = drv_hwreg_srccap_timingdetect_set_capture_win(src, win, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	memset(&srccap_dev->timingdetect_info.cap_win, 0, sizeof(struct v4l2_rect));

	return 0;
}

int mtk_timingdetect_check_sync(enum v4l2_srccap_input_source src,
	bool adc_offline_status, bool *has_sync)
{
	u32 hpd = 0;
	u32 vtt = 0;
	int ret = 0;

	if (src == V4L2_SRCCAP_INPUT_SOURCE_NONE)
		return -EINVAL;

	ret = timingdetect_get_sync_status(src, adc_offline_status, &hpd, &vtt);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = timingdetect_check_signal_sync(hpd, vtt, has_sync);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

int mtk_timingdetect_get_field(
	struct mtk_srccap_dev *srccap_dev,
	u8 *field)
{
	int ret = 0;
	enum reg_srccap_timingdetect_source detblock = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = timingdetect_map_detblock_reg(srccap_dev->srccap_info.src, &detblock);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_timingdetect_get_field_type(detblock, field);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

int mtk_timingdetect_set_auto_no_signal_mute(
	struct mtk_srccap_dev *srccap_dev,
	bool enable)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	enum reg_srccap_timingdetect_source detsrc = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;

	ret = timingdetect_map_detblock_reg(srccap_dev->srccap_info.src, &detsrc);
	if (ret)
		goto exit;


	ret = drv_hwreg_srccap_timingdetect_set_ans_mute(detsrc, enable, TRUE, NULL, NULL);

exit:
	SRCCAP_MSG_RETURN_CHECK(ret);
	return ret;
}

int mtk_timingdetect_set_ans_vtt_det_en(
	struct mtk_srccap_dev *srccap_dev,
	bool disable)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	enum reg_srccap_timingdetect_source detsrc = REG_SRCCAP_TIMINGDETECT_SOURCE_ADCA;

	ret = timingdetect_map_detblock_reg(srccap_dev->srccap_info.src, &detsrc);
	if (ret < 0)
		goto exit;

	ret = drv_hwreg_srccap_timingdetect_set_ans_vtt_det(detsrc, disable, TRUE, NULL, NULL);
	if (ret < 0)
		goto exit;

	return ret;
exit:
	SRCCAP_MSG_RETURN_CHECK(ret);
	return ret;
}

int mtk_timingdetect_steamon(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;

	ret = timingdetect_set_auto_nosignal(srccap_dev);

	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

	return ret;
}

int mtk_timingdetect_set_8p(
	struct mtk_srccap_dev *srccap_dev,
	bool frl)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	bool enable_8p = 0;
	enum reg_srccap_timingdetect_source src = 0;

	if ((srccap_dev->srccap_info.src >= V4L2_SRCCAP_INPUT_SOURCE_HDMI) &&
		(srccap_dev->srccap_info.src <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4)) {

		ret = timingdetect_map_detblock_reg(srccap_dev->srccap_info.src, &src);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		enable_8p = frl;
		SRCCAP_MSG_INFO("frl mode:%s, enable_8p:%s\n", frl?"Yes":"No", enable_8p?"1p":"8p");

		ret = drv_hwreg_srccap_timingdetect_set_8p(enable_8p, src, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}
	return 0;
}

int mtk_timingdetect_set_hw_version(
	struct mtk_srccap_dev *srccap_dev,
	bool stub)
{
	u32 hw_ver = 0;
	int ret = 0;

	hw_ver = srccap_dev->srccap_info.cap.hw_version;

	ret = timingdetect_set_hw_version(hw_ver, stub);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

	return ret;
}

int mtk_srccap_register_timingdetect_subdev(struct v4l2_device *v4l2_dev,
	struct v4l2_subdev *subdev_timingdetect,
	struct v4l2_ctrl_handler *timingdetect_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num =
		sizeof(timingdetect_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(timingdetect_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(timingdetect_ctrl_handler,
					&timingdetect_ctrl[ctrl_count], NULL);
	}

	ret = timingdetect_ctrl_handler->error;
	if (ret) {
		SRCCAP_MSG_ERROR("failed to create timingdetect ctrl\n"
					"handler\n");
		goto exit;
	}
	subdev_timingdetect->ctrl_handler = timingdetect_ctrl_handler;

	v4l2_subdev_init(subdev_timingdetect, &timingdetect_sd_ops);
	subdev_timingdetect->internal_ops = &timingdetect_sd_internal_ops;
	strlcpy(subdev_timingdetect->name, "mtk-timingdetect",
					sizeof(subdev_timingdetect->name));

	ret = v4l2_device_register_subdev(v4l2_dev, subdev_timingdetect);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to register timingdetect\n"
					"subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(timingdetect_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_timingdetect_subdev(
			struct v4l2_subdev *subdev_timingdetect)
{
	v4l2_ctrl_handler_free(subdev_timingdetect->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_timingdetect);
}

