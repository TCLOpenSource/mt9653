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
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/debugfs.h>
//#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <asm/siginfo.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/videodev2.h>
#include <linux/errno.h>
#include <linux/compat.h>
#include <linux/delay.h>

#include "linux/metadata_utility/m_srccap.h"

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-memops.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-common.h>
#include <media/v4l2-subdev.h>

#include "mtk_srccap.h"
#include "mtk_srccap_hdmirx.h"
#include "mtk-reserved-memory.h"

#include "hwreg_srccap_hdmirx_packetreceiver.h"
#include "hwreg_srccap_hdmirx_mux.h"
#include "hwreg_srccap_hdmirx.h"

#include "drv_HDMI.h"

static MS_HDMI_PACKET_STATE_t _mtk_v4l2src_packet_state_to_hdmirx(
	enum v4l2_ext_hdmi_packet_state enPacketType)
{
	MS_HDMI_PACKET_STATE_t eState = 0;

	switch (enPacketType) {
	case V4L2_EXT_HDMI_PKT_MPEG:
		eState = PKT_MPEG;
		break;
	case V4L2_EXT_HDMI_PKT_AUI:
		eState = PKT_AUI;
		break;
	case V4L2_EXT_HDMI_PKT_SPD:
		eState = PKT_SPD;
		break;
	case V4L2_EXT_HDMI_PKT_AVI:
		eState = PKT_AVI;
		break;
	case V4L2_EXT_HDMI_PKT_GC:
		eState = PKT_GC;
		break;
	case V4L2_EXT_HDMI_PKT_ASAMPLE:
		eState = PKT_ASAMPLE;
		break;
	case V4L2_EXT_HDMI_PKT_ACR:
		eState = PKT_ACR;
		break;
	case V4L2_EXT_HDMI_PKT_VS:
		eState = PKT_VS;
		break;
	case V4L2_EXT_HDMI_PKT_NULL:
		eState = PKT_NULL;
		break;
	case V4L2_EXT_HDMI_PKT_ISRC2:
		eState = PKT_ISRC2;
		break;
	case V4L2_EXT_HDMI_PKT_ISRC1:
		eState = PKT_ISRC1;
		break;
	case V4L2_EXT_HDMI_PKT_ACP:
		eState = PKT_ACP;
		break;
	case V4L2_EXT_HDMI_PKT_ONEBIT_AUD:
		eState = PKT_ONEBIT_AUD;
		break;
	case V4L2_EXT_HDMI_PKT_GM:
		eState = PKT_GM;
		break;
	case V4L2_EXT_HDMI_PKT_HBR:
		eState = PKT_HBR;
		break;
	case V4L2_EXT_HDMI_PKT_VBI:
		eState = PKT_VBI;
		break;
	case V4L2_EXT_HDMI_PKT_HDR:
		eState = PKT_HDR;
		break;
	case V4L2_EXT_HDMI_PKT_RSV:
		eState = PKT_RSV;
		break;
	case V4L2_EXT_HDMI_PKT_EDR:
		eState = PKT_EDR;
		break;
	case V4L2_EXT_HDMI_PKT_CHANNEL_STATUS:
		eState = PKT_CHANNEL_STATUS;
		break;
	case V4L2_EXT_HDMI_PKT_MULTI_VS:
		eState = PKT_MULTI_VS;
		break;
	case V4L2_EXT_HDMI_PKT_EMP_VENDOR:
		eState = PKT_EMP_VENDOR;
		break;
	case V4L2_EXT_HDMI_PKT_EMP_VTEM:
		eState = PKT_EMP_VTEM;
		break;
	case V4L2_EXT_HDMI_PKT_EMP_DSC:
		eState = PKT_EMP_DSC;
		break;
	case V4L2_EXT_HDMI_PKT_EMP_HDR:
		eState = PKT_EMP_HDR;
		break;
	default:
		eState = 0;
		break;
	}

	return eState;
}


static enum HDMI_STATUS_FLAG_TYPE
_mtk_v4l2src_packet_state_to_hdmi_status_flag_type(
	enum v4l2_ext_hdmi_packet_state enPacketType)
{
	enum HDMI_STATUS_FLAG_TYPE eState = 0;

	switch (enPacketType) {
	case V4L2_EXT_HDMI_PKT_MPEG:
		eState = HDMI_STATUS_MPEG_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_AUI:
		eState = HDMI_STATUS_AUDIO_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_SPD:
		eState = HDMI_STATUS_SPD_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_AVI:
		eState = HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_GC:
		eState = HDMI_STATUS_GCP_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_ASAMPLE:
		eState = HDMI_STATUS_AUDIO_SAMPLE_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_ACR:
		eState = HDMI_STATUS_ACR_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_VS:
		eState = HDMI_STATUS_VS_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_NULL:
		eState = HDMI_STATUS_NULL_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_ISRC2:
		eState = HDMI_STATUS_ISRC2_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_ISRC1:
		eState = HDMI_STATUS_ISRC1_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_ACP:
		eState = HDMI_STATUS_ACP_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_ONEBIT_AUD:
		eState = HDMI_STATUS_DSD_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_GM:
		eState = HDMI_STATUS_GM_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_HBR:
		eState = HDMI_STATUS_HBR_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_VBI:
		eState = HDMI_STATUS_VBI_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_HDR:
		eState = HDMI_STATUS_HDR_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_RSV:
		eState = HDMI_STATUS_RESERVED_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_EDR:
		eState = HDMI_STATUS_EDR_VALID_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_CHANNEL_STATUS:
		eState = HDMI_STATUS_AUDIO_SAMPLE_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_MULTI_VS:
		eState = HDMI_STATUS_VS_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_EMP_VENDOR:
		eState = HDMI_STATUS_EM_VENDOR_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_EMP_VTEM:
		eState = HDMI_STATUS_EM_VTEM_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_EMP_DSC:
		eState = HDMI_STATUS_EM_DSC_PACKET_RECEIVE_FLAG;
		break;
	case V4L2_EXT_HDMI_PKT_EMP_HDR:
		eState = HDMI_STATUS_EM_HDR_PACKET_RECEIVE_FLAG;
		break;
	default:
		eState = 0;
		break;
	}

	return eState;
}



static char *
_mtk_v4l2src_packet_state_to_string(
	enum v4l2_ext_hdmi_packet_state enPacketType)
{
	char *string = "WHO?";

	switch (enPacketType) {
	case V4L2_EXT_HDMI_PKT_MPEG:
		string = "MPEG";
		break;
	case V4L2_EXT_HDMI_PKT_AUI:
		string = "AUI";
		break;
	case V4L2_EXT_HDMI_PKT_SPD:
		string = "SPD";
		break;
	case V4L2_EXT_HDMI_PKT_AVI:
		string = "AVI";
		break;
	case V4L2_EXT_HDMI_PKT_GC:
		string = "GCP";
		break;
	case V4L2_EXT_HDMI_PKT_ASAMPLE:
		string = "ASP";
		break;
	case V4L2_EXT_HDMI_PKT_ACR:
		string = "ACR";
		break;
	case V4L2_EXT_HDMI_PKT_VS:
		string = "VSIF";
		break;
	case V4L2_EXT_HDMI_PKT_NULL:
		string = "NULL";
		break;
	case V4L2_EXT_HDMI_PKT_ISRC2:
		string = "ISRC2";
		break;
	case V4L2_EXT_HDMI_PKT_ISRC1:
		string = "ISRC1";
		break;
	case V4L2_EXT_HDMI_PKT_ACP:
		string = "ACP";
		break;
	case V4L2_EXT_HDMI_PKT_ONEBIT_AUD:
		string = "DSD";
		break;
	case V4L2_EXT_HDMI_PKT_GM:
		string = "GM";
		break;
	case V4L2_EXT_HDMI_PKT_HBR:
		string = "HBR";
		break;
	case V4L2_EXT_HDMI_PKT_VBI:
		string = "VBI";
		break;
	case V4L2_EXT_HDMI_PKT_HDR:
		string = "HDR";
		break;
	case V4L2_EXT_HDMI_PKT_RSV:
		string = "RSV";
		break;
	case V4L2_EXT_HDMI_PKT_EDR:
		string = "EDR";
		break;
	case V4L2_EXT_HDMI_PKT_CHANNEL_STATUS:
		string = "AUD_CHAN_STATUS";
		break;
	case V4L2_EXT_HDMI_PKT_MULTI_VS:
		string = "M_VSIF";
		break;
	case V4L2_EXT_HDMI_PKT_EMP_VENDOR:
		string = "EMP_VENDOR";
		break;
	case V4L2_EXT_HDMI_PKT_EMP_VTEM:
		string = "EMP_VTEM";
		break;
	case V4L2_EXT_HDMI_PKT_EMP_DSC:
		string = "EMP_DSC";
		break;
	case V4L2_EXT_HDMI_PKT_EMP_HDR:
		string = "EMP_HDR";
		break;
	default:
		break;
	}

	return string;
}

static void getV4lFromPKT_REPORT_S(struct v4l2_ext_hdmi_pkt_report_s *p_info,
				   struct hdmi_pkt_report_s *pkt_report_buf)
{
	p_info->time_info.u32_vsync_interval[E_INDEX_0] =
		pkt_report_buf->time_info.u32_vsync_interval[E_INDEX_0];
	p_info->time_info.u32_vsync_interval[E_INDEX_1] =
		pkt_report_buf->time_info.u32_vsync_interval[E_INDEX_1];
	p_info->time_info.u32_vsync_interval[E_INDEX_2] =
		pkt_report_buf->time_info.u32_vsync_interval[E_INDEX_2];
	p_info->time_info.u32_vsync_handler[E_INDEX_0] =
		pkt_report_buf->time_info.u32_vsync_handler[E_INDEX_0];
	p_info->time_info.u32_vsync_handler[E_INDEX_1] =
		pkt_report_buf->time_info.u32_vsync_handler[E_INDEX_1];
	p_info->time_info.u32_vsync_handler[E_INDEX_2] =
		pkt_report_buf->time_info.u32_vsync_handler[E_INDEX_2];
	p_info->time_info.u8_framecnt_interval[E_INDEX_0] =
		pkt_report_buf->time_info.u8_framecnt_interval[E_INDEX_0];
	p_info->time_info.u8_framecnt_interval[E_INDEX_1] =
		pkt_report_buf->time_info.u8_framecnt_interval[E_INDEX_1];
	p_info->time_info.u8_framecnt_interval[E_INDEX_2] =
		pkt_report_buf->time_info.u8_framecnt_interval[E_INDEX_2];
	p_info->u16_latency[E_INDEX_0] = pkt_report_buf->u16_latency[E_INDEX_0];
	p_info->u16_latency[E_INDEX_1] = pkt_report_buf->u16_latency[E_INDEX_1];
	p_info->u16_latency[E_INDEX_2] = pkt_report_buf->u16_latency[E_INDEX_2];
	p_info->u16_latency[E_INDEX_3] = pkt_report_buf->u16_latency[E_INDEX_3];
	p_info->u32_isr_cnt = pkt_report_buf->u32_isr_cnt;
	p_info->u16_frame_cnt_chg_at_handler =
		pkt_report_buf->u16_frame_cnt_chg_at_handler;
	p_info->u16_handler_late = pkt_report_buf->u16_handler_late;
	p_info->u16_pkt_overwrite = pkt_report_buf->u16_pkt_overwrite;
	p_info->u16_crc_mismatch = pkt_report_buf->u16_crc_mismatch;
}

static void _mtk_srccap_hdmirx_ParseAVIInfoFrame(
	const struct pkt_prototype_avi_if *p_pkt_avi,
	struct hdmi_avi_packet_info *p_dst_buf_avi)
{
	bool bExtendColormetryFlag = false;

	if (p_dst_buf_avi != NULL) {

		p_dst_buf_avi->enColorForamt = M_HDMI_COLOR_RGB;
		p_dst_buf_avi->enColorRange = M_HDMI_COLOR_RANGE_DEFAULT;
		p_dst_buf_avi->enColormetry = M_HDMI_COLORIMETRY_NO_DATA;
		p_dst_buf_avi->enPixelRepetition = M_HDMI_PIXEL_REPETITION_1X;
		p_dst_buf_avi->enAspectRatio = M_HDMI_Pic_AR_NODATA;
		p_dst_buf_avi->enActiveFormatAspectRatio = M_HDMI_AF_AR_SAME;

		p_dst_buf_avi->u16StructVersion = 0;
		p_dst_buf_avi->u16StructSize = 0;
		p_dst_buf_avi->u8Version = p_pkt_avi->hdr_if.version;
		p_dst_buf_avi->u8Length = p_pkt_avi->hdr_if.length;
		p_dst_buf_avi->u8S10Value = p_pkt_avi->payld_avi_if.S1_0;
		p_dst_buf_avi->u8B10Value = p_pkt_avi->payld_avi_if.B1_0;
		p_dst_buf_avi->u8A0Value = p_pkt_avi->payld_avi_if.A0;
		p_dst_buf_avi->u8Y210Value = p_pkt_avi->payld_avi_if.Y2_0;
		p_dst_buf_avi->u8R3210Value = p_pkt_avi->payld_avi_if.R3_0;
		p_dst_buf_avi->u8M10Value = p_pkt_avi->payld_avi_if.M1_0;
		p_dst_buf_avi->u8C10Value = p_pkt_avi->payld_avi_if.C1_0;
		p_dst_buf_avi->u8SC10Value = p_pkt_avi->payld_avi_if.SC1_0;
		p_dst_buf_avi->u8Q10Value = p_pkt_avi->payld_avi_if.Q1_0;
		p_dst_buf_avi->u8EC210Value = p_pkt_avi->payld_avi_if.EC2_0;
		p_dst_buf_avi->u8ITCValue = p_pkt_avi->payld_avi_if.ITC;
		p_dst_buf_avi->u8VICValue = p_pkt_avi->payld_avi_if.VIC;
		p_dst_buf_avi->u8PR3210Value = p_pkt_avi->payld_avi_if.PR;
		p_dst_buf_avi->u8CN10Value = p_pkt_avi->payld_avi_if.CN;
		p_dst_buf_avi->u8YQ10Value = p_pkt_avi->payld_avi_if.YQ;
		p_dst_buf_avi->u8ACE3210Value = p_pkt_avi->payld_avi_if.ACE3_0;


		switch (p_dst_buf_avi->u8Y210Value) {
		case 0x0:
			p_dst_buf_avi->enColorForamt = M_HDMI_COLOR_RGB;
			break;

		case 0x2:
			p_dst_buf_avi->enColorForamt = M_HDMI_COLOR_YUV_444;
			break;

		case 0x1:
			p_dst_buf_avi->enColorForamt = M_HDMI_COLOR_YUV_422;
			break;

		case 0x3:
			p_dst_buf_avi->enColorForamt = M_HDMI_COLOR_YUV_420;
			break;

		default:
			break;
		};

		if (p_dst_buf_avi->enColorForamt == M_HDMI_COLOR_RGB) {
			switch (p_dst_buf_avi->u8Q10Value) {
			case 0x0:
			//CEA spec VIC=1 640*480, is the only timing of table 1,
			//the default color space shall be RGB using Full Range
				if (p_dst_buf_avi->u8VICValue == 0x01)
					p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_FULL;
				else if (p_dst_buf_avi->u8VICValue == 0x00)
					p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_FULL;
				else
					p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_LIMIT;
				break;

			case 0x1:
				p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_LIMIT;
				break;

			case 0x2:
				p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_FULL;
				break;

			case 0x3:
				p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_RESERVED;
				break;

			default:
				break;
			};
		} else {
			switch (p_dst_buf_avi->u8YQ10Value) {
			case 0x0:
				p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_LIMIT;
				break;

			case 0x1:
				p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_FULL;
				break;

			case 0x2:
				p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_RESERVED;
				break;

			case 0x3:
				p_dst_buf_avi->enColorRange
					= M_HDMI_COLOR_RANGE_RESERVED;
				break;

			default:
				break;
			};
		}

		switch (p_dst_buf_avi->u8C10Value) {
		case 0:
			if (p_dst_buf_avi->enColorForamt
				!= M_HDMI_COLOR_RGB)
				p_dst_buf_avi->enColormetry
				= M_HDMI_COLORMETRY_UNKNOWN;
			break;

		case 0x1:
			if (p_dst_buf_avi->enColorForamt
				!= M_HDMI_COLOR_RGB)
				p_dst_buf_avi->enColormetry
				= M_HDMI_COLORIMETRY_SMPTE_170M;
			break;

		case 0x2:
			if (p_dst_buf_avi->enColorForamt
				!= M_HDMI_COLOR_RGB)
				p_dst_buf_avi->enColormetry
				= M_HDMI_COLORIMETRY_ITU_R_BT_709;
			break;

		case 0x3:
			bExtendColormetryFlag = true;
			break;

		default:
			p_dst_buf_avi->enColormetry
				= M_HDMI_COLORMETRY_UNKNOWN;
			break;
		};

		if (bExtendColormetryFlag) {
			switch (p_dst_buf_avi->u8EC210Value) {
			case 0:
				p_dst_buf_avi->enColormetry
				= M_HDMI_EXT_COLORIMETRY_XVYCC601;
				break;

			case 0x1:
				p_dst_buf_avi->enColormetry
				= M_HDMI_EXT_COLORIMETRY_XVYCC709;
				break;

			case 0x2:
				p_dst_buf_avi->enColormetry
				= M_HDMI_EXT_COLORIMETRY_SYCC601;
				break;

			case 0x3:
				p_dst_buf_avi->enColormetry
				= M_HDMI_EXT_COLORIMETRY_ADOBEYCC601;
				break;

			case 0x4:
				p_dst_buf_avi->enColormetry
				= M_HDMI_EXT_COLORIMETRY_ADOBERGB;
				break;

			case 0x5:
				p_dst_buf_avi->enColormetry
				= M_HDMI_EXT_COLORIMETRY_BT2020YcCbcCrc;
				break;

			case 0x6:
				p_dst_buf_avi->enColormetry
				= M_HDMI_EXT_COLORIMETRY_BT2020RGBYCbCr;
				break;

			case 0x7:
				if (p_dst_buf_avi->u8ACE3210Value == 0x00) {
					p_dst_buf_avi->enColormetry =
			M_HDMI_EXT_COLORIMETRY_ADDITIONAL_DCI_P3_RGB_D65;
				} else if (p_dst_buf_avi->u8ACE3210Value
					== 0x01) {
					p_dst_buf_avi->enColormetry =
			M_HDMI_EXT_COLORIMETRY_ADDITIONAL_DCI_P3_RGB_THEATER;
				}
				break;

			default:
				p_dst_buf_avi->enColormetry
					= M_HDMI_COLORMETRY_UNKNOWN;
				break;
			};
		}

		p_dst_buf_avi->enPixelRepetition =
		(enum m_hdmi_pixel_repetition) p_dst_buf_avi->u8PR3210Value;

		switch (p_dst_buf_avi->u8M10Value) {
		case 0x0:
			p_dst_buf_avi->enAspectRatio = M_HDMI_Pic_AR_NODATA;
			break;

		case 0x1:
			p_dst_buf_avi->enAspectRatio = M_HDMI_Pic_AR_4x3;
			break;

		case 0x2:
			p_dst_buf_avi->enAspectRatio = M_HDMI_Pic_AR_16x9;
			break;

		case 0x3:
		default:
			p_dst_buf_avi->enAspectRatio = M_HDMI_Pic_AR_RSV;
			break;
		};

		switch (p_dst_buf_avi->u8R3210Value) {
		case 0x2:
			p_dst_buf_avi->enActiveFormatAspectRatio
				= M_HDMI_AF_AR_16x9_Top;
			break;

		case 0x3:
			p_dst_buf_avi->enActiveFormatAspectRatio
				= M_HDMI_AF_AR_14x9_Top;
			break;

		case 0x4:
			p_dst_buf_avi->enActiveFormatAspectRatio
				= M_HDMI_AF_AR_GT_16x9;
			break;

		case 0x8:
			p_dst_buf_avi->enActiveFormatAspectRatio
				= M_HDMI_AF_AR_SAME;
			break;

		case 0x9:
			p_dst_buf_avi->enActiveFormatAspectRatio
				= M_HDMI_AF_AR_4x3_C;
			break;

		case 0xA:
			p_dst_buf_avi->enActiveFormatAspectRatio
				= M_HDMI_AF_AR_16x9_C;
			break;

		case 0xB:
			p_dst_buf_avi->enActiveFormatAspectRatio
				= M_HDMI_AF_AR_14x9_C;
			break;

		case 0xD:
			p_dst_buf_avi->enActiveFormatAspectRatio
			= M_HDMI_AF_AR_4x3_with_14x9_C;
			break;

		case 0xE:
			p_dst_buf_avi->enActiveFormatAspectRatio
			= M_HDMI_AF_AR_16x9_with_14x9_C;
			break;

		case 0xF:
			p_dst_buf_avi->enActiveFormatAspectRatio
			= M_HDMI_AF_AR_16x9_with_4x3_C;
			break;

		default:
			p_dst_buf_avi->enActiveFormatAspectRatio
				= M_HDMI_AF_AR_SAME;
			break;
		}
	}
}



static MS_U8 _mtk_srccap_hdmirx_pkt_get_raw_data(
	E_MUX_INPUTPORT enInputPortType,
	MS_HDMI_PACKET_STATE_t enPacketType,
	void *buf_ptr,
	MS_U32 u32_buf_len)
{
	return (MS_U8)drv_hwreg_srccap_hdmirx_pkt_get_gnl(
			enInputPortType,
			enPacketType,
			(struct pkt_prototype_gnl *)buf_ptr,
			u32_buf_len);
}


static MS_BOOL _mtk_srccap_hdmirx_pkt_valid(
	enum v4l2_srccap_input_source src,
	enum v4l2_ext_hdmi_packet_state enPacketType)
{

	MS_U32 u32_pkt_recv_flag =
		MDrv_HDMI_Get_PacketStatus(
		mtk_hdmirx_to_muxinputport(src));

	return (u32_pkt_recv_flag &
_mtk_v4l2src_packet_state_to_hdmi_status_flag_type(enPacketType)) ? TRUE:FALSE;
}


MS_U8 mtk_srccap_hdmirx_pkt_get_gnl(
	enum v4l2_srccap_input_source src,
	enum v4l2_ext_hdmi_packet_state enPacketType,
	struct hdmi_packet_info *gnl_ptr,
	MS_U32 u32_buf_len)
{
	if (!_mtk_srccap_hdmirx_pkt_valid(src, enPacketType))
		return 0;

	return (MS_U8)drv_hwreg_srccap_hdmirx_pkt_get_gnl(
			mtk_hdmirx_to_muxinputport(src),
			_mtk_v4l2src_packet_state_to_hdmirx(enPacketType),
			(struct pkt_prototype_gnl *)gnl_ptr,
			u32_buf_len);
}

MS_U8 mtk_srccap_hdmirx_pkt_get_AVI(
	enum v4l2_srccap_input_source src,
	struct hdmi_avi_packet_info *ptr,
	MS_U32 u32_buf_len)
{
	struct pkt_prototype_avi_if pkt_avi;

	if (!ptr)
		return 0;

	if (u32_buf_len < sizeof(struct hdmi_avi_packet_info))
		return 0;

	if (!_mtk_srccap_hdmirx_pkt_valid(src, V4L2_EXT_HDMI_PKT_AVI))
		return 0;

	if (drv_hwreg_srccap_hdmirx_pkt_get_avi(
		mtk_hdmirx_to_muxinputport(src),
		&pkt_avi, 0)) {
		_mtk_srccap_hdmirx_ParseAVIInfoFrame(&pkt_avi, ptr);
		return 1;
	}
	return 0;
}

MS_U8 mtk_srccap_hdmirx_pkt_get_VSIF(
	enum v4l2_srccap_input_source src,
	struct hdmi_vsif_packet_info *ptr,
	MS_U32 u32_buf_len)
{
	if (!ptr)
		return 0;

	if (u32_buf_len < sizeof(struct hdmi_vsif_packet_info))
		return 0;

	if (!_mtk_srccap_hdmirx_pkt_valid(src, V4L2_EXT_HDMI_PKT_VS))
		return 0;

	return _mtk_srccap_hdmirx_pkt_get_raw_data(
		mtk_hdmirx_to_muxinputport(src),
		PKT_VS, (void *)ptr, u32_buf_len);
}

MS_U8 mtk_srccap_hdmirx_pkt_get_GCP(
	enum v4l2_srccap_input_source src,
	struct hdmi_gc_packet_info *ptr,
	MS_U32 u32_buf_len)
{
	struct pkt_prototype_gcp pkt_buf;

	if (!ptr)
		return 0;

	if (u32_buf_len < sizeof(struct hdmi_gc_packet_info))
		return 0;

	if (!_mtk_srccap_hdmirx_pkt_valid(src, V4L2_EXT_HDMI_PKT_GC))
		return 0;

	if (drv_hwreg_srccap_hdmirx_pkt_get_gcp(
		mtk_hdmirx_to_muxinputport(src),
		&pkt_buf, 0)) {

		if (pkt_buf.payld_gcp.Set_AVMUTE)
			ptr->u8ControlAVMute = 1;

		if (pkt_buf.payld_gcp.Clear_AVMUTE)
			ptr->u8ControlAVMute = 0;

		ptr->u8DefaultPhase = pkt_buf.payld_gcp.Default_Phase;
		ptr->u8CDValue = pkt_buf.payld_gcp.CD3_0;
		ptr->u8PPValue = pkt_buf.payld_gcp.PP3_0;
		ptr->u8LastPPValue = pkt_buf.payld_gcp.PP3_0;
		ptr->u8PreLastPPValue = pkt_buf.payld_gcp.PP3_0;
		switch (pkt_buf.payld_gcp.CD3_0) {
		case 0:
		case 4:
			ptr->enColorDepth = M_HDMI_COLOR_DEPTH_8_BIT;
			break;
		case 5:
			ptr->enColorDepth = M_HDMI_COLOR_DEPTH_10_BIT;
			break;
		case 6:
			ptr->enColorDepth = M_HDMI_COLOR_DEPTH_12_BIT;
			break;
		case 7:
			ptr->enColorDepth = M_HDMI_COLOR_DEPTH_16_BIT;
			break;
		default:
			ptr->enColorDepth = M_HDMI_COLOR_DEPTH_UNKNOWN;
			break;
		}
		return 1;
	}
	return 0;
}

MS_U8 mtk_srccap_hdmirx_pkt_get_HDRIF(
	enum v4l2_srccap_input_source src,
	struct hdmi_hdr_packet_info *ptr,
	MS_U32 u32_buf_len)
{
	struct pkt_prototype_hdr_if pkt_buf;

	if (!ptr)
		return 0;

	if (u32_buf_len < sizeof(struct hdmi_hdr_packet_info))
		return 0;

	if (!_mtk_srccap_hdmirx_pkt_valid(src, V4L2_EXT_HDMI_PKT_HDR))
		return 0;

	if (drv_hwreg_srccap_hdmirx_pkt_get_hdrif(
		mtk_hdmirx_to_muxinputport(src),
		&pkt_buf, 0)) {
		ptr->u8EOTF = pkt_buf.payld_hdr_if.EOTF;
		ptr->u8Static_Metadata_ID
			= pkt_buf.payld_hdr_if.metadata_desc_id;
		ptr->u16Display_Primaries_X[E_INDEX_0]
			= pkt_buf.payld_hdr_if.dis_primaries_x0;
		ptr->u16Display_Primaries_X[E_INDEX_1]
			= pkt_buf.payld_hdr_if.dis_primaries_x1;
		ptr->u16Display_Primaries_X[E_INDEX_2]
			= pkt_buf.payld_hdr_if.dis_primaries_x2;
		ptr->u16Display_Primaries_Y[E_INDEX_0]
			= pkt_buf.payld_hdr_if.dis_primaries_y0;
		ptr->u16Display_Primaries_Y[E_INDEX_1]
			= pkt_buf.payld_hdr_if.dis_primaries_y1;
		ptr->u16Display_Primaries_Y[E_INDEX_2]
			= pkt_buf.payld_hdr_if.dis_primaries_y2;
		ptr->u16White_Point_X = pkt_buf.payld_hdr_if.white_point_x;
		ptr->u16White_Point_Y = pkt_buf.payld_hdr_if.white_point_y;
		ptr->u16Max_Display_Mastering_Luminance
			= pkt_buf.payld_hdr_if.max_dis_mastering_luminance;
		ptr->u16Min_Display_Mastering_Luminance
			= pkt_buf.payld_hdr_if.min_dis_mastering_luminance;
		ptr->u16Maximum_Content_Light_Level
			= pkt_buf.payld_hdr_if.max_content_light_level;
		ptr->u16Maximum_Frame_Average_Light_Level
			= pkt_buf.payld_hdr_if.max_frame_avg_light_level;
		ptr->u8Version = pkt_buf.hdr_if.version;
		ptr->u8Length = pkt_buf.hdr_if.length;

		return 1;
	}
	return 0;
}


MS_U8 mtk_srccap_hdmirx_pkt_get_VS_EMP(
	enum v4l2_srccap_input_source src,
	struct hdmi_emp_packet_info *ptr,
	MS_U32 u32_buf_len)
{
	if (!ptr)
		return 0;

	if (u32_buf_len < sizeof(struct hdmi_emp_packet_info))
		return 0;

	if (!_mtk_srccap_hdmirx_pkt_valid(src, V4L2_EXT_HDMI_PKT_EMP_VENDOR))
		return 0;

	return _mtk_srccap_hdmirx_pkt_get_raw_data(
		mtk_hdmirx_to_muxinputport(src),
		PKT_EMP_VENDOR, (void *)ptr, u32_buf_len);
}

MS_U8 mtk_srccap_hdmirx_pkt_get_DSC_EMP(
	enum v4l2_srccap_input_source src,
	struct hdmi_emp_packet_info *ptr,
	MS_U32 u32_buf_len)
{
	if (!ptr)
		return 0;

	if (u32_buf_len < sizeof(struct hdmi_emp_packet_info))
		return 0;

	if (!_mtk_srccap_hdmirx_pkt_valid(src, V4L2_EXT_HDMI_PKT_EMP_DSC))
		return 0;

	return _mtk_srccap_hdmirx_pkt_get_raw_data(
		mtk_hdmirx_to_muxinputport(src),
		PKT_EMP_DSC, (void *)ptr, u32_buf_len);
}

MS_U8 mtk_srccap_hdmirx_pkt_get_Dynamic_HDR_EMP(
	enum v4l2_srccap_input_source src,
	struct hdmi_emp_packet_info *ptr,
	MS_U32 u32_buf_len)
{
	if (!ptr)
		return 0;

	if (u32_buf_len < sizeof(struct hdmi_emp_packet_info))
		return 0;

	if (!_mtk_srccap_hdmirx_pkt_valid(src, V4L2_EXT_HDMI_PKT_EMP_HDR))
		return 0;

	return _mtk_srccap_hdmirx_pkt_get_raw_data(
		mtk_hdmirx_to_muxinputport(src),
		PKT_EMP_HDR, (void *)ptr, u32_buf_len);
}

MS_U8 mtk_srccap_hdmirx_pkt_get_VRR_EMP(
	enum v4l2_srccap_input_source src,
	struct hdmi_emp_packet_info *ptr,
	MS_U32 u32_buf_len)
{
	if (!ptr)
		return 0;

	if (u32_buf_len < sizeof(struct hdmi_emp_packet_info))
		return 0;

	if (!_mtk_srccap_hdmirx_pkt_valid(src, V4L2_EXT_HDMI_PKT_EMP_VTEM))
		return 0;

	return _mtk_srccap_hdmirx_pkt_get_raw_data(
		mtk_hdmirx_to_muxinputport(src),
		PKT_EMP_VTEM, (void *)ptr, u32_buf_len);
}

bool mtk_srccap_hdmirx_pkt_get_report(
	enum v4l2_srccap_input_source src,
	enum v4l2_ext_hdmi_packet_state enPacketType,
	struct v4l2_ext_hdmi_pkt_report_s *buf_ptr)
{
	struct hdmi_pkt_report_s my_report_buf;

	if (!buf_ptr)
		return false;

	if (!drv_hwreg_srccap_hdmirx_pkt_get_report(
		mtk_hdmirx_to_muxinputport(src),
		_mtk_v4l2src_packet_state_to_hdmirx(enPacketType),
		&my_report_buf))
		return false;

	//memcpy((void *)buf_ptr, (void *)&my_report_buf,
	//	sizeof(struct v4l2_ext_hdmi_pkt_report_s));
	getV4lFromPKT_REPORT_S(buf_ptr, &my_report_buf);

	return true;
}

bool mtk_srccap_hdmirx_pkt_is_dsc(
	enum v4l2_srccap_input_source src)
{
	return drv_hwreg_srccap_hdmirx_pkt_is_dsc(
		mtk_hdmirx_to_muxinputport(src));
}

bool mtk_srccap_hdmirx_pkt_is_vrr(
	enum v4l2_srccap_input_source src)
{
	return drv_hwreg_srccap_hdmirx_pkt_is_vrr(
		mtk_hdmirx_to_muxinputport(src));
}

bool mtk_srccap_hdmirx_pkt_audio_mute_en(
	enum v4l2_srccap_input_source src,
	bool mute_en)
{
	E_MUX_INPUTPORT InputPort = mtk_hdmirx_to_muxinputport(src);
	enum HDMI_AUDIO_PATH_TYPE audio_mux_n  =
		drv_hwreg_srccap_hdmirx_mux_query_audio_inverse(InputPort);

	if (audio_mux_n != HDMI_AUDIO_PATH_NONE) {
		return drv_hwreg_srccap_hdmirx_pkt_audio_mute_en(
			audio_mux_n, mute_en?HDMI_BYTE_FUL_MSK:_BYTE_0,
			HDMI_BYTE_FUL_MSK);
	}
	return false;
}

bool mtk_srccap_hdmirx_pkt_audio_status_clr(
	enum v4l2_srccap_input_source src)
{
	return drv_hwreg_srccap_hdmirx_pkt_audio_mute_status_clr(
		mtk_hdmirx_to_muxinputport(src));
}

MS_U32 mtk_srccap_hdmirx_pkt_get_audio_fs_hz(
	enum v4l2_srccap_input_source src)
{
	E_MUX_INPUTPORT enInputPortType
		= mtk_hdmirx_to_muxinputport(src);
	MS_U8 u8_active_src
		= drv_hwreg_srccap_hdmirx_mux_query_video_inverse(
			enInputPortType);

	MS_U32 u32_audio_fs_hz = 0;

	if (u8_active_src >= SRCINPUT_NUM_VAL)
		return 0;

	u32_audio_fs_hz = drv_hwreg_srccap_hdmirx_pkt_get_audio_fs_hz(
		mtk_hdmirx_to_muxinputport(src));

	if (u32_audio_fs_hz > HDMI_AUDIO_MAX_FS_HZ
		|| u32_audio_fs_hz < HDMI_AUDIO_MIN_FS_HZ)
		return 0;

	return u32_audio_fs_hz;
}

