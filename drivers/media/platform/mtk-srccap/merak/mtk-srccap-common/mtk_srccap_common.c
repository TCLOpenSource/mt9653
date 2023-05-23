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
#include <linux/delay.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <asm-generic/div64.h>

#include "linux/metadata_utility/m_srccap.h"
#include "mtk_srccap.h"
#include "mtk_srccap_common.h"
#include "hwreg_srccap_common_trigger_gen.h"
#include "hwreg_srccap_common.h"
#include "mapi_pq_if.h"
#include "mapi_pq_nts_reg_if.h"
#include "mapi_pq_cfd_if.h"

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
#include "mtk_srccap_atrace.h"
#endif

enum srccap_cfd_info_index {
	E_PQ_CFD_ANALOG_YUV_NTSC,   // 0
	E_PQ_CFD_ANALOG_YUV_PAL,    // 1
	E_PQ_CFD_ANALOG_YUV_SECAM,  // 2
	E_PQ_CFD_ANALOG_RGB_NTSC,   // 3
	E_PQ_CFD_ANALOG_RGB_PAL,    // 4
	E_PQ_CFD_YPBPR_SD,          // 5
	E_PQ_CFD_YPBPR_FHD,         // 6
	E_PQ_CFD_YPBPR_P2P,         // 7
	E_PQ_CFD_DVI,               // 8
	E_PQ_CFD_VGA,               // 9
	E_PQ_CFD_HDMI,              // 10
	E_PQ_CFD_MAX,               // 11
};

struct srccap_color_info_vui {
	u8 color_primaries;
	u8 transfer_characteristics;
	u8 matrix_coefficients;
};

static enum srccap_stub_mode srccapStubMode;
//mapping HDMI AVI color space to VUI
static struct srccap_color_info_vui color_metry_vui_map[11] = {
	{1, 11, 5},    //0:MS_HDMI_EXT_COLOR_XVYCC601
	{1, 11, 1},    //1:MS_HDMI_EXT_COLOR_XVYCC709
	{1, 13, 5},    //2:MS_HDMI_EXT_COLOR_SYCC601
	{0x80, 18, 5}, //3:MS_HDMI_EXT_COLOR_ADOBEYCC601
	{0x80, 18, 5}, //4:MS_HDMI_EXT_COLOR_ADOBERGB
	{9, 13, 10},   //5:MS_HDMI_EXT_COLOR_BT2020YcCbcCrc
	{9, 14, 9},    //6:MS_HDMI_EXT_COLOR_BT2020RGBYCbCr
	{12, 13, 10},  //7:MS_HDMI_EXT_COLOR_ADDITIONAL_DCI_P3_RGB_D65
	{11, 14, 9},   //8:MS_HDMI_EXT_COLOR_ADDITIONAL_DCI_P3_RGB_THEATER
	{6, 6, 6},     //9:MS_HDMI_EXT_COLORIMETRY_SMPTE_170M
	{1, 1, 1},     //10:MS_HDMI_EXT_COLORIMETRY_ITU_R_BT_709
};

//mapping src type to CFD info
static struct srccap_main_color_info src_cfd_map[E_PQ_CFD_MAX] = {
	{2, 0, 8, 6, 6, 6},  // 0 E_PQ_CFD_ANALOG_YUV_NTSC
	{2, 0, 8, 5, 6, 5},  // 1 E_PQ_CFD_ANALOG_YUV_PAL
	{2, 0, 8, 5, 6, 5},  // 2 E_PQ_CFD_ANALOG_YUV_SECAM
	{0, 0, 8, 6, 6, 6},  // 3 E_PQ_CFD_ANALOG_RGB_NTSC
	{0, 0, 8, 5, 6, 5},  // 4 E_PQ_CFD_ANALOG_RGB_PAL
	{2, 0, 8, 5, 6, 5},  // 5 E_PQ_CFD_YPBPR_SD
	{2, 0, 8, 1, 1, 1},  // 6 E_PQ_CFD_YPBPR_FHD
	{2, 0, 8, 2, 2, 2},  // 7 E_PQ_CFD_YPBPR_P2P
	{0, 1, 8, 1, 13, 5}, // 8 E_PQ_CFD_DVI
	{0, 1, 8, 1, 13, 5}, // 9 E_PQ_CFD_VGA
	{2, 0, 8, 2, 2, 2},  // 10 E_PQ_CFD_HDMI
};


// used to remap from enum m_hdmi_colorimetry_format to MS_HDMI_EXT_COLORIMETRY_FORMAT
#define HDMI_COLORI_METRY_REMAP_SIZE  19
static MS_HDMI_EXT_COLORIMETRY_FORMAT enHdmiColoriMetryRemap[HDMI_COLORI_METRY_REMAP_SIZE] = {
	MS_HDMI_EXT_COLOR_XVYCC601,  // 0x0 M_HDMI_COLORIMETRY_NO_DATA
	MS_HDMI_EXT_COLORIMETRY_SMPTE_170M,  // 0x1 M_HDMI_COLORIMETRY_SMPTE_170M
	MS_HDMI_EXT_COLORIMETRY_ITU_R_BT_709,  // 0x2 M_HDMI_COLORIMETRY_ITU_R_BT_709
	MS_HDMI_EXT_COLOR_UNKNOWN,  // 0x3 M_HDMI_COLORIMETRY_EXTENDED
	MS_HDMI_EXT_COLOR_UNKNOWN,  // 0x4 empty
	MS_HDMI_EXT_COLOR_UNKNOWN,  // 0x5 empty
	MS_HDMI_EXT_COLOR_UNKNOWN,  // 0x6 empty
	MS_HDMI_EXT_COLOR_UNKNOWN,  // 0x7 empty
	MS_HDMI_EXT_COLOR_XVYCC601,  // 0x8 M_HDMI_EXT_COLORIMETRY_XVYCC601
	MS_HDMI_EXT_COLOR_XVYCC709,  // 0x9 M_HDMI_EXT_COLORIMETRY_XVYCC709
	MS_HDMI_EXT_COLOR_SYCC601,  // 0xA M_HDMI_EXT_COLORIMETRY_SYCC601
	MS_HDMI_EXT_COLOR_ADOBEYCC601,  // 0xB M_HDMI_EXT_COLORIMETRY_ADOBEYCC601
	MS_HDMI_EXT_COLOR_ADOBERGB,  // 0xC M_HDMI_EXT_COLORIMETRY_ADOBERGB
	MS_HDMI_EXT_COLOR_BT2020YcCbcCrc,  // 0xD M_HDMI_EXT_COLORIMETRY_BT2020YcCbcCrc
	MS_HDMI_EXT_COLOR_BT2020RGBYCbCr,  // 0xE M_HDMI_EXT_COLORIMETRY_BT2020RGBYCbCr
	MS_HDMI_EXT_COLOR_BT2020RGBYCbCr,  // 0xF M_HDMI_EXT_COLORIMETRY_ADDITIONAL
	MS_HDMI_EXT_COLOR_ADDITIONAL_DCI_P3_RGB_D65,  // 0x10
				//	M_HDMI_EXT_COLORIMETRY_ADDITIONAL_DCI_P3_RGB_D65
	MS_HDMI_EXT_COLOR_ADDITIONAL_DCI_P3_RGB_THEATER,  // 0x11
				//	M_HDMI_EXT_COLORIMETRY_ADDITIONAL_DCI_P3_RGB_THEATER
	MS_HDMI_EXT_COLOR_UNKNOWN,  // 0xffff M_HDMI_COLORMETRY_UNKNOWN
};
#define HDMI_COLORMETRI_REMAP(x) enHdmiColoriMetryRemap[((x) >= \
		HDMI_COLORI_METRY_REMAP_SIZE ? HDMI_COLORI_METRY_REMAP_SIZE - 1 : (x))]

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
void common_vsync_isr(void *param)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	struct v4l2_event event;

	if (param == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	srccap_dev = (struct mtk_srccap_dev *)param;

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	SRCCAP_ATRACE_INT_FORMAT(srccap_dev->dev_id,
		"41-S: [srccap]send_event_vsync_%u", srccap_dev->dev_id);
#endif

	if (video_is_registered(srccap_dev->vdev)) {
		SRCCAP_MSG_DEBUG("send V4L2_EVENT_VSYNC to event queue\n");
		memset(&event, 0, sizeof(struct v4l2_event));
		event.type = V4L2_EVENT_VSYNC;
		event.id = 0;
		v4l2_event_queue(srccap_dev->vdev, &event);
	}
}

static int _get_hdmi_port_by_src(enum v4l2_srccap_input_source src,
	int *hdmi_port)
{
	if ((src >= V4L2_SRCCAP_INPUT_SOURCE_DVI) && (src <= V4L2_SRCCAP_INPUT_SOURCE_DVI4))
		*hdmi_port = src - V4L2_SRCCAP_INPUT_SOURCE_DVI;
	else if ((src >= V4L2_SRCCAP_INPUT_SOURCE_HDMI) && (src <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4))
		*hdmi_port = src - V4L2_SRCCAP_INPUT_SOURCE_HDMI;
	else
		return -EINVAL;

	return 0;
}

static int _get_avd_source_type(bool b_scart_rgb, enum AVD_VideoStandardType e_standard,
	enum srccap_cfd_info_index *index)
{
	if (b_scart_rgb) {
		switch (e_standard) {
		case E_VIDEOSTANDARD_NTSC_44:            // 480
		case E_VIDEOSTANDARD_NTSC_M:             // 480
		case E_VIDEOSTANDARD_PAL_M:              // 480
		case E_VIDEOSTANDARD_PAL_60:             // 480
			*index = E_PQ_CFD_ANALOG_RGB_NTSC;  // for 480
			break;
		case E_VIDEOSTANDARD_PAL_N:              // 576
		case E_VIDEOSTANDARD_PAL_BGHI:           // 576
		case E_VIDEOSTANDARD_SECAM:              // 576
		default:
			*index = E_PQ_CFD_ANALOG_RGB_PAL;   // for 576
			break;
		}
	} else {
		switch (e_standard) {
		case E_VIDEOSTANDARD_PAL_M:
			*index = E_PQ_CFD_ANALOG_YUV_PAL;
			break;
		case E_VIDEOSTANDARD_PAL_N:
			*index = E_PQ_CFD_ANALOG_YUV_PAL;
			break;
		case E_VIDEOSTANDARD_NTSC_44:
			*index = E_PQ_CFD_ANALOG_YUV_NTSC;
			break;
		case E_VIDEOSTANDARD_PAL_60:
			*index = E_PQ_CFD_ANALOG_YUV_PAL;
			break;
		case E_VIDEOSTANDARD_NTSC_M:
			*index = E_PQ_CFD_ANALOG_YUV_NTSC;
			break;
		case E_VIDEOSTANDARD_SECAM:
			*index = E_PQ_CFD_ANALOG_YUV_SECAM;
			break;
		case E_VIDEOSTANDARD_PAL_BGHI:
		default:
			*index = E_PQ_CFD_ANALOG_YUV_PAL;
			break;
		}
	}

	return 0;
}

static int _parse_hdmi_main_color_info(enum v4l2_srccap_input_source src,
	struct srccap_main_color_info *color_info)
{
	MS_HDMI_EXT_COLORIMETRY_FORMAT enColormetry;
	struct meta_srccap_hdr_pkt hdrPkt;
	struct hdmi_avi_packet_info avi_info;
	int ret = 0;

	if (color_info == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&hdrPkt, 0, sizeof(hdrPkt));
	memset(&avi_info, 0, sizeof(avi_info));

	/*get pkt*/
	if (mtk_srccap_common_is_Cfd_stub_mode()) {
		ret = mtk_srccap_common_get_cfd_test_HDMI_pkt_HDRIF(&hdrPkt);
		if (ret == 0)
			SRCCAP_MSG_ERROR("mtk_srccap_hdmirx_pkt_get_HDRIF failed, set 0 instead\n");
		ret = mtk_srccap_common_get_cfd_test_HDMI_pkt_AVI(&avi_info);
		if (ret == 0)
			SRCCAP_MSG_ERROR("mtk_srccap_hdmirx_pkt_get_AVI failed, set 0 instead\n");
	} else {
		ret = mtk_srccap_hdmirx_pkt_get_HDRIF(src,
			((struct hdmi_hdr_packet_info *)&hdrPkt), sizeof(hdrPkt));
		if (ret == 0)
			SRCCAP_MSG_ERROR("mtk_srccap_hdmirx_pkt_get_HDRIF failed, set 0 instead\n");
		ret = mtk_srccap_hdmirx_pkt_get_AVI(src, &avi_info, sizeof(avi_info));
		if (ret == 0)
			SRCCAP_MSG_ERROR("mtk_srccap_hdmirx_pkt_get_AVI failed, set 0 instead\n");
	}


	SRCCAP_MSG_DEBUG("CFD get avi format = %d, range = %d, color space = %d\n",
		avi_info.enColorForamt,
		avi_info.enColorRange,
		avi_info.enColormetry);

	enColormetry = HDMI_COLORMETRI_REMAP(avi_info.enColormetry);
	if (enColormetry > MS_HDMI_EXT_COLORIMETRY_ITU_R_BT_709)
		enColormetry = MS_HDMI_EXT_COLOR_XVYCC601;

	/*ctrl point 1*/
	color_info->data_format = avi_info.enColorForamt;
	color_info->data_range = avi_info.enColorRange;
	color_info->bit_depth = 8; //hard code
	color_info->color_primaries =
		color_metry_vui_map[enColormetry].color_primaries;

	/*transfer_characteristics logic*/
	if (hdrPkt.u8Length != 0) {
		if (hdrPkt.u8EOTF == 2) { //HDR10
			//E_PQ_CFD_CFIO_TR_SMPTE2084
			color_info->transfer_characteristics = 16;
		} else if (hdrPkt.u8EOTF == 3) { //HLG
			//E_PQ_CFD_CFIO_TR_HLG
			color_info->transfer_characteristics = 18;
		} else {
			color_info->transfer_characteristics =
			color_metry_vui_map[enColormetry].transfer_characteristics;
		}
	} else {
		color_info->transfer_characteristics =
			color_metry_vui_map[enColormetry].transfer_characteristics;
	}

	color_info->matrix_coefficients =
		color_metry_vui_map[enColormetry].matrix_coefficients;

	return 0;
}

static int _parse_source_main_color_info(struct mtk_srccap_dev *srccap_dev,
	struct srccap_color_info *color_info)
{
	enum v4l2_srccap_input_source src;
	enum srccap_cfd_info_index index = E_PQ_CFD_MAX;
	int hdmi_port = 0;
	u16 h_size;
	u16 v_size;
#if PCMODE_UI_READY
	bool b_p2p;
#endif
	bool b_hdmi_mode;
	bool b_interlace;
	bool b_scart_rgb;
	enum AVD_VideoStandardType e_standard;

	if ((srccap_dev == NULL) || (color_info == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	src = srccap_dev->srccap_info.src;
	h_size = srccap_dev->timingdetect_info.data.h_de;
	v_size = srccap_dev->timingdetect_info.data.v_de;
#if PCMODE_UI_READY
	b_p2p = false; //TODO
#endif
	switch (src) {
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		index = E_PQ_CFD_VGA;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		if (mtk_srccap_common_is_Cfd_stub_mode())
			index = E_PQ_CFD_HDMI;
		else {
			b_hdmi_mode = mtk_srccap_hdmirx_isHDMIMode(src);
			if (!b_hdmi_mode)
				index = E_PQ_CFD_DVI;
			else
				index = E_PQ_CFD_HDMI;
		}
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		b_interlace = srccap_dev->timingdetect_info.data.interlaced;
#if PCMODE_UI_READY
		if ((!b_interlace) && b_p2p)
			index = E_PQ_CFD_YPBPR_P2P;
#endif
		if ((h_size >= SRCCAP_COMMON_FHD_W) && (v_size >= SRCCAP_COMMON_FHD_H))
			index = E_PQ_CFD_YPBPR_FHD;
		else
			index = E_PQ_CFD_YPBPR_SD;
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
		b_scart_rgb = (srccap_dev->adc_info.adc_src == SRCCAP_ADC_SOURCE_ONLY_SCART_RGB);
		e_standard = Api_AVD_GetLastDetectedStandard();
		_get_avd_source_type(b_scart_rgb, e_standard, &index);
		break;
	default:
		return -EINVAL;
	}

	if ((index >= E_PQ_CFD_ANALOG_YUV_NTSC) && (index < E_PQ_CFD_MAX)) {
		memcpy(&color_info->in,
			&src_cfd_map[index],
			sizeof(struct srccap_main_color_info));
		SRCCAP_MSG_DEBUG("Get Color info index: %d\n", index);
	} else {
		SRCCAP_MSG_ERROR("Color info index invalid: %d\n", index);
		return -EINVAL;
	}

	if (index == E_PQ_CFD_HDMI) {
		SRCCAP_MSG_DEBUG("Get Color info index HDMI, need paser HDMI info\n");
		_parse_hdmi_main_color_info(srccap_dev->srccap_info.src,
			&color_info->in);
	}

	SRCCAP_MSG_DEBUG("CFD get format = %u, range = %u, cp = %u, tc = %u, mc = %u\n",
		color_info->in.data_format,
		color_info->in.data_range,
		color_info->in.color_primaries,
		color_info->in.transfer_characteristics,
		color_info->in.matrix_coefficients);

	return 0;
}

/* ============================================================================================== */
/* --------------------------------------- v4l2_ctrl_ops ---------------------------------------- */
/* ============================================================================================== */
static int _common_g_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, common_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_SRCCAP_PQMAP_INFO:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_SRCCAP_PQMAP_INFO\n");
		ret = mtk_srccap_common_get_pqmap_info(
		(struct v4l2_srccap_pqmap_info *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _common_s_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, common_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_COMMON_FORCE_VSYNC:
		if (ctrl->p_new.p == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			return -EINVAL;
		}

		struct v4l2_common_force_vsync i =
			*(struct v4l2_common_force_vsync *)(ctrl->p_new.p);
		SRCCAP_MSG_INFO("force vsync:(%lu,%lu,%lu)\n",
			(i.type), (i.event), (i.set_force));
		ret = mtk_common_force_irq(i.type, i.event, i.set_force,
			srccap_dev->srccap_info.cap.u32IRQ_Version);
		break;
	case V4L2_CID_SRCCAP_PQMAP_INFO:
		SRCCAP_MSG_DEBUG("Set V4L2_CID_SRCCAP_PQMAP_INFO\n");
		ret = mtk_srccap_common_set_pqmap_info(
		(struct v4l2_srccap_pqmap_info *)ctrl->p_new.p_u8, srccap_dev);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops common_ctrl_ops = {
	.g_volatile_ctrl = _common_g_ctrl,
	.s_ctrl = _common_s_ctrl,
};

static const struct v4l2_ctrl_config common_ctrl[] = {
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_COMMON_FORCE_VSYNC,
		.name = "Set Force Vsync IRQ",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_common_force_vsync)},
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SRCCAP_PQMAP_INFO,
		.name = "Init Srccap pqmap",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_srccap_pqmap_info)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops common_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops common_sd_internal_ops = {
};

static int srccap_common_init_alg_ctx(struct mtk_srccap_dev *srccap_dev)
{
	void *alg_ctx = NULL;
	void *sw_reg = NULL;
	void *meta_data = NULL;
	void *hw_report = NULL;
	int init_ctx_result = 0;
	struct ST_PQ_CTX_SIZE_INFO ctx_size_info;
	struct ST_PQ_CTX_SIZE_STATUS ctx_size_status;
	struct ST_PQ_CTX_INFO ctx_info;
	struct ST_PQ_CTX_STATUS ctx_status;
	struct ST_PQ_FRAME_CTX_INFO frame_ctx_info;
	struct ST_PQ_FRAME_CTX_STATUS frame_ctx_status;
	enum EN_PQAPI_RESULT_CODES pq_ret = 0;

	memset(&ctx_size_info, 0, sizeof(struct ST_PQ_CTX_SIZE_INFO));
	memset(&ctx_size_status, 0, sizeof(struct ST_PQ_CTX_SIZE_STATUS));
	memset(&ctx_info, 0, sizeof(struct ST_PQ_CTX_INFO));
	memset(&ctx_status, 0, sizeof(struct ST_PQ_CTX_STATUS));
	memset(&frame_ctx_info, 0, sizeof(struct ST_PQ_FRAME_CTX_INFO));
	memset(&frame_ctx_status, 0, sizeof(struct ST_PQ_FRAME_CTX_STATUS));

	ctx_size_info.u32Version = 0;
	ctx_size_info.u32Length = sizeof(struct ST_PQ_CTX_SIZE_INFO);

	ctx_size_status.u32Version = 0;
	ctx_size_status.u32Length = sizeof(struct ST_PQ_CTX_SIZE_STATUS);
	ctx_size_status.u32PqCtxSize = 0;
	pq_ret = MApi_PQ_GetCtxSize(NULL, &ctx_size_info, &ctx_size_status);
	if (pq_ret == E_PQAPI_RC_SUCCESS) {
		if (ctx_size_status.u32PqCtxSize != 0) {
			alg_ctx = vmalloc(ctx_size_status.u32PqCtxSize);
			if (alg_ctx)
				memset(alg_ctx, 0, ctx_size_status.u32PqCtxSize);
			else
				SRCCAP_MSG_ERROR("alg_ctx vmalloc fail\n");
		} else {
			SRCCAP_MSG_ERROR("ctx_size_status.u32PqCtxSize is 0\n");
		}

		if (ctx_size_status.u32SwRegSize != 0) {
			sw_reg = vmalloc(ctx_size_status.u32SwRegSize);
			if (sw_reg)
				memset(sw_reg, 0, ctx_size_status.u32SwRegSize);
			else
				SRCCAP_MSG_ERROR("sw_reg vmalloc fail\n");
		} else {
			SRCCAP_MSG_ERROR("ctx_size_status.u32SwRegSize is 0\n");
		}

		if (ctx_size_status.u32MetadataSize != 0) {
			meta_data = vmalloc(ctx_size_status.u32MetadataSize);
			if (meta_data)
				memset(meta_data, 0, ctx_size_status.u32MetadataSize);
			else
				SRCCAP_MSG_ERROR("meta_data vmalloc fail\n");
		} else {
			SRCCAP_MSG_ERROR("ctx_size_status.u32MetadataSize is 0\n");
		}

		if (ctx_size_status.u32HwReportSize != 0) {
			hw_report = vmalloc(ctx_size_status.u32HwReportSize);
			if (hw_report)
				memset(hw_report, 0, ctx_size_status.u32HwReportSize);
			else
				SRCCAP_MSG_ERROR("hw_report vmalloc fail\n");
		} else {
			SRCCAP_MSG_ERROR("ctx_size_status.u32HwReportSize is 0\n");
		}
	} else {
		SRCCAP_MSG_ERROR("MApi_PQ_GetCtxSize failed! pq_ret = 0x%x\n", pq_ret);
	}

	frame_ctx_info.pSwReg = sw_reg;
	frame_ctx_info.pMetadata = meta_data;
	frame_ctx_info.pHwReport = hw_report;
	pq_ret = MApi_PQ_SetFrameCtx(alg_ctx, &frame_ctx_info, &frame_ctx_status);
	if (pq_ret != E_PQAPI_RC_SUCCESS) {
		SRCCAP_MSG_ERROR("MApi_PQ_SetFrameCtx failed! pq_ret = 0x%x\n", pq_ret);
		vfree(alg_ctx);
		return -1;
	}
	SRCCAP_MSG_INFO("MApi_PQ_SetFrameCtx success\n");

	ctx_info.eWinID = E_PQ_MAIN_WINDOW;
	init_ctx_result = MApi_PQ_InitCtx(alg_ctx, &ctx_info, &ctx_status);
	if (init_ctx_result > 0) {
		SRCCAP_MSG_ERROR("MApi_PQ_InitCtx fail, return:%d\n", init_ctx_result);
		vfree(alg_ctx);
		return -1;
	}
	srccap_dev->common_info.pqmap_info.alg_ctx = alg_ctx;
	SRCCAP_MSG_INFO("MApi_PQ_InitCtx success, u32PqCtxSize:%u\n", ctx_size_status.u32PqCtxSize);

	return 0;
}

static int srccap_common_free_alg_ctx(void *ctx)
{
	if (ctx)
		vfree(ctx);
	return 0;
}

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_common_updata_color_info(struct mtk_srccap_dev *srccap_dev)
{
	struct srccap_color_info *color_info;
	enum v4l2_srccap_input_source src;
	int hdmi_port;
	struct hdmi_avi_packet_info avi;
	int ret = 0;
	u16 dev_id = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	src = srccap_dev->srccap_info.src;
	color_info = &srccap_dev->common_info.color_info;
	dev_id = srccap_dev->dev_id;

	if (_parse_source_main_color_info(srccap_dev, color_info))
		SRCCAP_MSG_ERROR("Get Main color info failed!!!\n");

	if (_get_hdmi_port_by_src(src, &hdmi_port))
		SRCCAP_MSG_DEBUG("Not HDMI source!!!\n");

	color_info->out.data_range = color_info->in.data_range;
	color_info->out.color_primaries = color_info->in.color_primaries;
	color_info->out.transfer_characteristics = color_info->in.transfer_characteristics;
	color_info->out.data_format = color_info->in.data_format;

	//If control point 1 is RGB format
	//and control point 3 is YUV format, default BT.709.
	if ((color_info->in.data_format == MS_HDMI_COLOR_RGB) &&
	((color_info->out.data_format == MS_HDMI_COLOR_YUV_420) ||
	(color_info->out.data_format == MS_HDMI_COLOR_YUV_422) ||
	(color_info->out.data_format == MS_HDMI_COLOR_YUV_444))) {
		color_info->out.matrix_coefficients =
			0x1; //default BT.709  E_PQ_CFD_CFIO_MC_BT709_XVYCC709
	} else {
		color_info->out.matrix_coefficients = color_info->in.matrix_coefficients;
	}

	color_info->out.bit_depth = color_info->in.bit_depth;
	color_info->hdr_type = 0; //default SDR, no need to set this param here.

	SRCCAP_MSG_INFO("Srccap R2Y off\n");

	return 0;
}

int mtk_common_updata_frame_color_info(struct mtk_srccap_dev *srccap_dev)
{
	struct srccap_color_info *color_info;
	enum v4l2_srccap_input_source src;
	int hdmi_port;
	struct hdmi_avi_packet_info avi_info;
	struct meta_srccap_hdr_pkt hdrPkt;
	MS_HDMI_EXT_COLORIMETRY_FORMAT enColormetry = MS_HDMI_EXT_COLOR_UNKNOWN;
	int ret = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&avi_info, 0, sizeof(struct hdmi_avi_packet_info));
	memset(&hdrPkt, 0, sizeof(struct meta_srccap_hdr_pkt));

	src = srccap_dev->srccap_info.src;
	color_info = &srccap_dev->common_info.color_info;

	if (_get_hdmi_port_by_src(src, &hdmi_port)) {
		SRCCAP_MSG_DEBUG("Not HDMI source!!!\n");
	} else {

		/*get pkt*/
		if (mtk_srccap_common_is_Cfd_stub_mode()) {
			ret = mtk_srccap_common_get_cfd_test_HDMI_pkt_HDRIF(&hdrPkt);
			srccap_dev->dv_info.descrb_info.common.interface
			= SRCCAP_DV_DESCRB_INTERFACE_NONE;
			ret = mtk_srccap_common_get_cfd_test_HDMI_pkt_AVI(&avi_info);
		} else {
			ret = mtk_srccap_hdmirx_pkt_get_HDRIF(src,
				((struct hdmi_hdr_packet_info *)&hdrPkt), sizeof(hdrPkt));
			if (ret == 0)
				SRCCAP_MSG_ERROR(
				"mtk_srccap_hdmirx_pkt_get_HDRIF failed, set 0 instead\n");
			ret = mtk_srccap_hdmirx_pkt_get_AVI(src, &avi_info, sizeof(avi_info));
			if (ret == 0)
				SRCCAP_MSG_ERROR(
				"mtk_srccap_hdmirx_pkt_get_AVI failed, set 0 instead\n");
		}

		enColormetry = HDMI_COLORMETRI_REMAP(avi_info.enColormetry);
		if (enColormetry > MS_HDMI_EXT_COLORIMETRY_ITU_R_BT_709)
			enColormetry = MS_HDMI_EXT_COLOR_XVYCC601;

		/*set input data format*/
		color_info->in.data_format = avi_info.enColorForamt;
		color_info->in.data_range = avi_info.enColorRange;
		color_info->in.bit_depth = 8; //hard code
		color_info->in.color_primaries =
			color_metry_vui_map[enColormetry].color_primaries;

		if (hdrPkt.u8Length != 0) {
			if (hdrPkt.u8EOTF == 2) { //HDR10
				color_info->in.transfer_characteristics =
					0x10; //E_PQ_CFD_CFIO_TR_SMPTE2084;
			} else if (hdrPkt.u8EOTF == 3) { //HLG
				//E_PQ_CFD_CFIO_TR_HLG
				color_info->in.transfer_characteristics = 0x12;
			} else {
				color_info->in.transfer_characteristics =
				color_metry_vui_map[enColormetry].transfer_characteristics;
			}
		} else {
			color_info->in.transfer_characteristics =
			color_metry_vui_map[enColormetry].transfer_characteristics;
		}

		color_info->in.matrix_coefficients =
			color_metry_vui_map[enColormetry].matrix_coefficients;

		/*set output data format*/
		//color_info->out.data_format  //already set in set_fmt and no longer change
		color_info->out.data_range = color_info->in.data_range;
		color_info->out.color_primaries = color_info->in.color_primaries;
		color_info->out.transfer_characteristics = color_info->in.transfer_characteristics;
		color_info->out.data_format = color_info->in.data_format;

		if ((color_info->in.data_format == MS_HDMI_COLOR_RGB) &&
		((color_info->out.data_format == MS_HDMI_COLOR_YUV_420) ||
		(color_info->out.data_format == MS_HDMI_COLOR_YUV_422) ||
		(color_info->out.data_format == MS_HDMI_COLOR_YUV_444))) {
			color_info->out.matrix_coefficients =
			0x1; //E_PQ_CFD_CFIO_MC_BT709_XVYCC709; //default BT.709
		} else {
			color_info->out.matrix_coefficients = color_info->in.matrix_coefficients;
		}

		color_info->out.bit_depth = color_info->in.bit_depth;

		/*set dip data format*/
		color_info->dip_point.data_format = MS_HDMI_COLOR_YUV_444;

		color_info->dip_point.data_range = color_info->out.data_range;
		color_info->dip_point.color_primaries = color_info->out.color_primaries;
		color_info->dip_point.transfer_characteristics =
			color_info->out.transfer_characteristics;
		color_info->dip_point.matrix_coefficients = color_info->out.matrix_coefficients;
		color_info->dip_point.bit_depth = color_info->out.bit_depth;


		/*set common format*/
		if (srccap_dev->dv_info.descrb_info.common.interface
			!= SRCCAP_DV_DESCRB_INTERFACE_NONE) {
			color_info->hdr_type = 0x1; //E_PQ_CFD_HDR_MODE_DOLBY;
		} else {
			if (color_info->in.transfer_characteristics ==
				0x10) { //E_PQ_CFD_CFIO_TR_SMPTE2084
				color_info->hdr_type = 0x2; //E_PQ_CFD_HDR_MODE_HDR10;
			} else if (color_info->in.transfer_characteristics
				== 0x12) { //E_PQ_CFD_CFIO_TR_HLG
				color_info->hdr_type = 0x3; //E_PQ_CFD_HDR_MODE_HLG;
			} else {
				color_info->hdr_type = 0x0;//E_PQ_CFD_HDR_MODE_SDR;
			}
		}
	}

	return 0;
}

int mtk_common_get_cfd_metadata(struct mtk_srccap_dev *srccap_dev,
	struct meta_srccap_color_info *cfd_meta)
{
	struct srccap_color_info *color_info;

	if ((srccap_dev == NULL) || (cfd_meta == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	color_info = &srccap_dev->common_info.color_info;
	cfd_meta->u8DataFormat = color_info->out.data_format;
	cfd_meta->u8DataRange = color_info->out.data_range;
	cfd_meta->u8BitDepth = color_info->out.bit_depth;
	cfd_meta->u8ColorPrimaries = color_info->out.color_primaries;
	cfd_meta->u8TransferCharacteristics = color_info->out.transfer_characteristics;
	cfd_meta->u8MatrixCoefficients = color_info->out.matrix_coefficients;
	cfd_meta->u8HdrType = color_info->hdr_type;
	cfd_meta->u32SamplingMode = color_info->chroma_sampling_mode;

	return 0;
}

int mtk_common_set_cfd_setting(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct ST_PQ_CFD_420_CONTROL_PARAMS cfd_420_control_param;
	struct ST_PQ_CFD_420_CONTROL_INFO cfd_420_control_info;

	memset(&cfd_420_control_param, 0, sizeof(struct ST_PQ_CFD_420_CONTROL_PARAMS));
	memset(&cfd_420_control_info, 0, sizeof(struct ST_PQ_CFD_420_CONTROL_INFO));

	cfd_420_control_param.u32Version = 0;
	cfd_420_control_param.u32Length = sizeof(struct ST_PQ_CFD_420_CONTROL_PARAMS);
	cfd_420_control_param.u32Input_DataFormat =
		srccap_dev->common_info.color_info.in.data_format;
	cfd_420_control_param.u32IPDMA_DataFormat =
		srccap_dev->common_info.color_info.out.data_format;
	cfd_420_control_param.u32Path = srccap_dev->dev_id;

	MApi_PQ_CFD_SetChromaSampling(srccap_dev->common_info.pqmap_info.alg_ctx,
		 &cfd_420_control_param, &cfd_420_control_info);

	/*TODO: set CSC and chroma sampling (SrcInputCSC /InputDomain_420_control)*/

	srccap_dev->common_info.color_info.chroma_sampling_mode =
		cfd_420_control_info.u32ChromaStatus;

	return 0;
}

int mtk_common_subscribe_event_vsync(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	enum srccap_common_irq_event event = SRCCAP_COMMON_IRQEVENT_MAX;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_0)
		event = SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC0;
	else if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_1)
		event = SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC1;
	else {
		SRCCAP_MSG_ERROR("failed to subscribe event (type, event = %d,%d)\n",
			SRCCAP_COMMON_IRQTYPE_HK, event);
		return -EINVAL;
	}

	ret = mtk_common_attach_irq(srccap_dev, SRCCAP_COMMON_IRQTYPE_HK, event,
		common_vsync_isr,
		(void *)srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to subscribe event (type,event = %d,%d)\n",
			SRCCAP_COMMON_IRQTYPE_HK, event);
		return -EPERM;
	}

	return 0;
}

int mtk_common_unsubscribe_event_vsync(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	enum srccap_common_irq_event event = SRCCAP_COMMON_IRQEVENT_MAX;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_0) {
		event = SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC0;
	} else if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_1) {
		event = SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC1;
	} else {
		SRCCAP_MSG_ERROR(
		"failed to subscribe event (type,event = %d,%d)\n",
		SRCCAP_COMMON_IRQTYPE_HK,
		event);
		return -EPERM;
	}

	ret = mtk_common_detach_irq(srccap_dev, SRCCAP_COMMON_IRQTYPE_HK, event,
		common_vsync_isr,
		(void *)srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to unsubscribe event %d\n", event);
		return -EPERM;
	}

	return 0;
}

int mtk_common_init_triggergen(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	struct reg_srccap_common_triggen_input_src_sel src_sel;
	struct reg_srccap_common_triggen_sw_irq_dly sw_irq_dly;
	struct reg_srccap_common_triggen_pq_irq_dly pq_irq_dly;

	memset(&src_sel, 0, sizeof(struct reg_srccap_common_triggen_input_src_sel));
	memset(&sw_irq_dly, 0, sizeof(struct reg_srccap_common_triggen_sw_irq_dly));
	memset(&pq_irq_dly, 0, sizeof(struct reg_srccap_common_triggen_pq_irq_dly));

	if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_0) {
		src_sel.domain = SRCCAP_COMMON_TRIGEN_DOMAIN_SRC0;
		src_sel.src = SRCCAP_COMMON_TRIGEN_INPUT_IP0;
		ret = drv_hwreg_srccap_common_triggen_set_inputSrcSel(src_sel, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		sw_irq_dly.domain = SRCCAP_COMMON_TRIGEN_DOMAIN_SRC0;
		sw_irq_dly.sw_irq_dly_v = SRCCAP_COMMON_SW_IRQ_DLY;
		sw_irq_dly.swirq_trig_sel = 0;
		ret = drv_hwreg_srccap_common_triggen_set_swIrqTrig(sw_irq_dly, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		pq_irq_dly.domain = SRCCAP_COMMON_TRIGEN_DOMAIN_SRC0;
		pq_irq_dly.pq_irq_dly_v = SRCCAP_COMMON_PQ_IRQ_DLY;
		pq_irq_dly.pqirq_trig_sel = 0;
		ret = drv_hwreg_srccap_common_triggen_set_pqIrqTrig(pq_irq_dly, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	} else if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_1) {
		src_sel.domain = SRCCAP_COMMON_TRIGEN_DOMAIN_SRC1;
		src_sel.src = SRCCAP_COMMON_TRIGEN_INPUT_IP1;
		ret = drv_hwreg_srccap_common_triggen_set_inputSrcSel(
			src_sel, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		sw_irq_dly.domain = SRCCAP_COMMON_TRIGEN_DOMAIN_SRC1;
		sw_irq_dly.sw_irq_dly_v = SRCCAP_COMMON_SW_IRQ_DLY;
		sw_irq_dly.swirq_trig_sel = 0;
		ret = drv_hwreg_srccap_common_triggen_set_swIrqTrig(sw_irq_dly, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		pq_irq_dly.domain = SRCCAP_COMMON_TRIGEN_DOMAIN_SRC1;
		pq_irq_dly.pq_irq_dly_v = SRCCAP_COMMON_PQ_IRQ_DLY;
		pq_irq_dly.pqirq_trig_sel = 0;
		ret = drv_hwreg_srccap_common_triggen_set_pqIrqTrig(pq_irq_dly, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	} else
		SRCCAP_MSG_ERROR("(%s)device id not found\n", srccap_dev->v4l2_dev.name);

	return 0;
}

int mtk_srccap_register_common_subdev(
	struct v4l2_device *v4l2_dev, struct v4l2_subdev *subdev_common,
	struct v4l2_ctrl_handler *common_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(common_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(common_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(common_ctrl_handler,
					&common_ctrl[ctrl_count], NULL);
	}

	ret = common_ctrl_handler->error;
	if (ret) {
		SRCCAP_MSG_ERROR("failed to create common ctrl handler\n");
		goto exit;
	}
	subdev_common->ctrl_handler = common_ctrl_handler;

	v4l2_subdev_init(subdev_common, &common_sd_ops);
	subdev_common->internal_ops = &common_sd_internal_ops;
	strlcpy(subdev_common->name, "mtk-common",
					sizeof(subdev_common->name));

	ret = v4l2_device_register_subdev(v4l2_dev, subdev_common);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to register common subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(common_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_common_subdev(struct v4l2_subdev *subdev_common)
{
	v4l2_ctrl_handler_free(subdev_common->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_common);
}

void mtk_srccap_common_set_hdmi_420to444(struct mtk_srccap_dev *srccap_dev)
{
	bool enable = false;
	u16 dev_id = srccap_dev->dev_id;
	int ret = 0;

	if (srccap_dev->timingdetect_info.data.yuv420 == true)
		enable = true;
	SRCCAP_MSG_DEBUG("yuv420:%d, enable:%d\n",
		srccap_dev->timingdetect_info.data.yuv420, enable);
	ret = drv_hwreg_srccap_common_set_hdmi_420to444(dev_id, enable, true, NULL, NULL);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

}

void mtk_srccap_common_set_hdmi422pack(struct mtk_srccap_dev *srccap_dev)
{
	bool enable = false;
	u16 dev_id = srccap_dev->dev_id;
	int ret = 0;

	//Just enable when DV STD tunnel mode.
	if (srccap_dev->dv_info.descrb_info.common.hdmi_422_pack_en == true)
		enable = true;
	SRCCAP_MSG_DEBUG("HDMI422pack enable:%d\n", enable);
	ret = drv_hwreg_srccap_common_set_hdmi422pack(dev_id, enable, true, NULL, NULL);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

}

int mtk_srccap_common_streamon(struct mtk_srccap_dev *srccap_dev)
{
	return srccap_common_init_alg_ctx(srccap_dev);
}

int mtk_srccap_common_stream_off(struct mtk_srccap_dev *srccap_dev)
{
	return srccap_common_free_alg_ctx(srccap_dev->common_info.pqmap_info.alg_ctx);
}

int mtk_srccap_common_pq_hwreg_init(struct mtk_srccap_dev *srccap_dev)
{
	void *pCtx = NULL;
	enum EN_PQAPI_RESULT_CODES pq_ret = E_PQAPI_RC_SUCCESS;
	struct ST_PQ_NTS_CAPABILITY_INFO capability_info;
	struct ST_PQ_NTS_CAPABILITY_STATUS capability_status;
	struct ST_PQ_NTS_GETHWREG_INFO get_hwreg_info;
	struct ST_PQ_NTS_GETHWREG_STATUS get_hwreg_status;
	struct ST_PQ_NTS_INITHWREG_INFO init_hwreg_info;
	struct ST_PQ_NTS_INITHWREG_STATUS init_hwreg_status;

	memset(&capability_info, 0, sizeof(struct ST_PQ_NTS_CAPABILITY_INFO));
	memset(&capability_status, 0, sizeof(struct ST_PQ_NTS_CAPABILITY_STATUS));
	memset(&get_hwreg_info, 0, sizeof(struct ST_PQ_NTS_GETHWREG_INFO));
	memset(&get_hwreg_status, 0, sizeof(struct ST_PQ_NTS_GETHWREG_STATUS));
	memset(&init_hwreg_info, 0, sizeof(struct ST_PQ_NTS_INITHWREG_INFO));
	memset(&init_hwreg_status, 0, sizeof(struct ST_PQ_NTS_INITHWREG_STATUS));

	capability_info.u16CapabilityModuleIdx = 0;
	capability_info.pu16Capability[0] = 0;
	capability_info.eDomain = E_PQ_DOMAIN_SOURCE;
	capability_info.u32Length = 0;
	capability_info.u32Version = 0;

	capability_status.u32Length = 0;
	capability_status.u32Version = 0;
	pq_ret = MApi_PQ_SetCapability_NonTs(pCtx, &capability_info, &capability_status);
	if (pq_ret != E_PQAPI_RC_SUCCESS) {
		SRCCAP_MSG_ERROR("MApi_PQ_SetCapability_NonTs failed! pq_ret: %d\n", pq_ret);
		return -1;
	}

	get_hwreg_info.u32Length = 0;
	get_hwreg_info.u32Version = 0;
	get_hwreg_info.eDomain = E_PQ_DOMAIN_SOURCE;

	get_hwreg_status.u32Length = 0;
	get_hwreg_status.u32Version = 0;

	pq_ret = MApi_PQ_GetHWRegInfo_NonTs(pCtx, &get_hwreg_info, &get_hwreg_status);
	if (pq_ret != E_PQAPI_RC_SUCCESS) {
		SRCCAP_MSG_ERROR("MApi_PQ_GetHWRegInfo_NonTs failed! pq_ret: %d\n", pq_ret);
		return -1;
	}

	init_hwreg_info.u32FuncTableSize = get_hwreg_status.u32FuncTableSize;
	init_hwreg_info.pFuncTableAddr = vmalloc(init_hwreg_info.u32FuncTableSize);
	if (init_hwreg_info.pFuncTableAddr == NULL) {
		SRCCAP_MSG_ERROR("[%s] xcalg_vmalloc failed!\n", __func__);
		return -1;
	}

	init_hwreg_info.u32Length = 0;
	init_hwreg_info.u32Version = 0;
	init_hwreg_info.eDomain = E_PQ_DOMAIN_SOURCE;

	init_hwreg_status.u32Length = 0;
	init_hwreg_status.u32Version = 0;

	pq_ret =  MApi_PQ_InitHWRegTable_NonTs(pCtx, &init_hwreg_info, &init_hwreg_status);
	if (pq_ret != E_PQAPI_RC_SUCCESS) {
		vfree(init_hwreg_info.pFuncTableAddr);
		SRCCAP_MSG_ERROR("MApi_PQ_InitHWRegTable_NonTs failed! pq_ret: %d\n", pq_ret);
		return -1;
	}
	srccap_dev->common_info.pqmap_info.hwreg_func_table_addr = init_hwreg_info.pFuncTableAddr;

	return 0;
}

int mtk_srccap_common_pq_hwreg_deinit(struct mtk_srccap_dev *srccap_dev)
{
	void *pCtx = NULL;
	enum EN_PQAPI_RESULT_CODES pq_ret = E_PQAPI_RC_SUCCESS;
	struct ST_PQ_NTS_DEINITHWREG_INFO deinit_hwreg_info;
	struct ST_PQ_NTS_DEINITHWREG_STATUS deinit_hwreg_status;

	memset(&deinit_hwreg_info, 0, sizeof(struct ST_PQ_NTS_DEINITHWREG_INFO));
	memset(&deinit_hwreg_status, 0, sizeof(struct ST_PQ_NTS_DEINITHWREG_STATUS));

	deinit_hwreg_info.u32Length = 0;
	deinit_hwreg_info.u32Version = 0;
	deinit_hwreg_info.eDomain = E_PQ_DOMAIN_SOURCE;

	deinit_hwreg_status.u32Length = 0;
	deinit_hwreg_status.u32Version = 0;
	if (srccap_dev->common_info.pqmap_info.hwreg_func_table_addr) {
		pq_ret =  MApi_PQ_DeInitHWRegTable_NonTs(pCtx,
					&deinit_hwreg_info, &deinit_hwreg_status);
		if (pq_ret != E_PQAPI_RC_SUCCESS) {
			SRCCAP_MSG_ERROR("MApi_PQ_DeInitHWRegTable_NonTs failed!%d\n", pq_ret);
			return -1;
		}
		vfree(srccap_dev->common_info.pqmap_info.hwreg_func_table_addr);
	}

	return 0;
}

int mtk_srccap_common_vd_pq_hwreg_init(struct mtk_srccap_dev *srccap_dev)
{
	void *pCtx = NULL;
	enum EN_PQAPI_RESULT_CODES pq_ret = E_PQAPI_RC_SUCCESS;
	struct ST_PQ_NTS_CAPABILITY_INFO capability_info;
	struct ST_PQ_NTS_CAPABILITY_STATUS capability_status;
	struct ST_PQ_NTS_GETHWREG_INFO get_hwreg_info;
	struct ST_PQ_NTS_GETHWREG_STATUS get_hwreg_status;
	struct ST_PQ_NTS_INITHWREG_INFO init_hwreg_info;
	struct ST_PQ_NTS_INITHWREG_STATUS init_hwreg_status;

	memset(&capability_info, 0, sizeof(struct ST_PQ_NTS_CAPABILITY_INFO));
	memset(&capability_status, 0, sizeof(struct ST_PQ_NTS_CAPABILITY_STATUS));
	memset(&get_hwreg_info, 0, sizeof(struct ST_PQ_NTS_GETHWREG_INFO));
	memset(&get_hwreg_status, 0, sizeof(struct ST_PQ_NTS_GETHWREG_STATUS));
	memset(&init_hwreg_info, 0, sizeof(struct ST_PQ_NTS_INITHWREG_INFO));
	memset(&init_hwreg_status, 0, sizeof(struct ST_PQ_NTS_INITHWREG_STATUS));

	capability_info.u16CapabilityModuleIdx = 0;
	capability_info.pu16Capability[0] = 0;
	capability_info.eDomain = E_PQ_DOMAIN_VD;
	capability_info.u32Length = 0;
	capability_info.u32Version = 0;

	capability_status.u32Length = 0;
	capability_status.u32Version = 0;
	pq_ret = MApi_PQ_SetCapability_NonTs(pCtx, &capability_info, &capability_status);
	if (pq_ret != E_PQAPI_RC_SUCCESS) {
		SRCCAP_MSG_ERROR("MApi_PQ_SetCapability_NonTs failed! pq_ret: %d\n", pq_ret);
		return -1;
	}

	get_hwreg_info.u32Length = 0;
	get_hwreg_info.u32Version = 0;
	get_hwreg_info.eDomain = E_PQ_DOMAIN_VD;

	get_hwreg_status.u32Length = 0;
	get_hwreg_status.u32Version = 0;

	pq_ret = MApi_PQ_GetHWRegInfo_NonTs(pCtx, &get_hwreg_info, &get_hwreg_status);
	if (pq_ret != E_PQAPI_RC_SUCCESS) {
		SRCCAP_MSG_ERROR("MApi_PQ_GetHWRegInfo_NonTs failed! pq_ret: %d\n", pq_ret);
		return -1;
	}

	init_hwreg_info.u32FuncTableSize = get_hwreg_status.u32FuncTableSize;
	init_hwreg_info.pFuncTableAddr = vmalloc(init_hwreg_info.u32FuncTableSize);
	if (init_hwreg_info.pFuncTableAddr == NULL) {
		SRCCAP_MSG_ERROR("[%s] xcalg_vmalloc failed!\n", __func__);
		return -1;
	}

	init_hwreg_info.u32Length = 0;
	init_hwreg_info.u32Version = 0;
	init_hwreg_info.eDomain = E_PQ_DOMAIN_VD;

	init_hwreg_status.u32Length = 0;
	init_hwreg_status.u32Version = 0;

	pq_ret =  MApi_PQ_InitHWRegTable_NonTs(pCtx, &init_hwreg_info, &init_hwreg_status);
	if (pq_ret != E_PQAPI_RC_SUCCESS) {
		vfree(init_hwreg_info.pFuncTableAddr);
		SRCCAP_MSG_ERROR("MApi_PQ_InitHWRegTable_NonTs failed! pq_ret: %d\n", pq_ret);
		return -1;
	}
	srccap_dev->common_info.vdpqmap_info.hwreg_func_table_addr = init_hwreg_info.pFuncTableAddr;

	return 0;
}

int mtk_srccap_common_vd_pq_hwreg_deinit(struct mtk_srccap_dev *srccap_dev)
{
	void *pCtx = NULL;
	enum EN_PQAPI_RESULT_CODES pq_ret = E_PQAPI_RC_SUCCESS;
	struct ST_PQ_NTS_DEINITHWREG_INFO deinit_hwreg_info;
	struct ST_PQ_NTS_DEINITHWREG_STATUS deinit_hwreg_status;

	memset(&deinit_hwreg_info, 0, sizeof(struct ST_PQ_NTS_DEINITHWREG_INFO));
	memset(&deinit_hwreg_status, 0, sizeof(struct ST_PQ_NTS_DEINITHWREG_STATUS));

	deinit_hwreg_info.u32Length = 0;
	deinit_hwreg_info.u32Version = 0;
	deinit_hwreg_info.eDomain = E_PQ_DOMAIN_VD;

	deinit_hwreg_status.u32Length = 0;
	deinit_hwreg_status.u32Version = 0;
	if (srccap_dev->common_info.pqmap_info.hwreg_func_table_addr) {
		pq_ret =  MApi_PQ_DeInitHWRegTable_NonTs(pCtx,
					&deinit_hwreg_info, &deinit_hwreg_status);
		if (pq_ret != E_PQAPI_RC_SUCCESS) {
			SRCCAP_MSG_ERROR("MApi_PQ_DeInitHWRegTable_NonTs failed!%d\n", pq_ret);
			return -1;
		}
		vfree(srccap_dev->common_info.vdpqmap_info.hwreg_func_table_addr);
	}

	return 0;
}

void mtk_srccap_common_set_stub_mode(u8 stub)
{
	srccapStubMode = stub;
}

bool mtk_srccap_common_is_Cfd_stub_mode(void)
{
	if (srccapStubMode >= SRCCAP_STUB_MODE_CFD_SDR_RGB_RGB
			&& srccapStubMode <= SRCCAP_STUB_MODE_CFD_HLG_YUV_RGB) {
		return true;
	}
	return false;
}

int mtk_srccap_common_get_cfd_test_HDMI_pkt_HDRIF(struct meta_srccap_hdr_pkt *phdr_pkt)
{
	#define EOTF_HDR 2
	#define EOTF_HLG 3
	phdr_pkt->u8Length = sizeof(struct meta_srccap_hdr_pkt);
	phdr_pkt->u8EOTF = 0;
	if (srccapStubMode == SRCCAP_STUB_MODE_CFD_HDR_RGB_YUV)
		phdr_pkt->u8EOTF = EOTF_HDR;
	else if (srccapStubMode == SRCCAP_STUB_MODE_CFD_HLG_YUV_RGB)
		phdr_pkt->u8EOTF = EOTF_HLG;


	return 1;
}

int mtk_srccap_common_get_cfd_test_HDMI_pkt_AVI(struct hdmi_avi_packet_info *pavi_pkt)
{
	pavi_pkt->enColorRange = M_HDMI_COLOR_RANGE_DEFAULT;
	if (srccapStubMode == SRCCAP_STUB_MODE_CFD_SDR_RGB_RGB
		|| srccapStubMode == SRCCAP_STUB_MODE_CFD_HDR_RGB_YUV) {
		pavi_pkt->enColorForamt = M_HDMI_COLOR_RGB;
		pavi_pkt->enColormetry = M_HDMI_EXT_COLORIMETRY_BT2020RGBYCbCr;
	} else {
		pavi_pkt->enColorForamt = M_HDMI_COLOR_YUV_420;
		pavi_pkt->enColormetry = M_HDMI_EXT_COLORIMETRY_BT2020YcCbcCrc;
	}

	return 1;
}

int mtk_srccap_common_get_cfd_test_HDMI_pkt_VSIF(struct hdmi_vsif_packet_info *vsif_pkt)
{
	#define MAGIC_VSIF_IEEE_HRD10P_LENG 3
	static const u8 MAGIC_VSIF_IEEE_HRD10P[MAGIC_VSIF_IEEE_HRD10P_LENG] = {0x8B, 0x84, 0x90};

	int i = 0;

	if (srccapStubMode != SRCCAP_STUB_MODE_CFD_HDR_RGB_YUV) {
		for (i = 0; i < MAGIC_VSIF_IEEE_HRD10P_LENG; i++)
			vsif_pkt->au8ieee[i] = 0;
	} else {
		for (i = 0; i < MAGIC_VSIF_IEEE_HRD10P_LENG; i++)
			vsif_pkt->au8ieee[i] = MAGIC_VSIF_IEEE_HRD10P[i];
	}
	return 1;
}




