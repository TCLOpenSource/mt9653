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
#include <uapi/linux/mtk-v4l2-pq.h>
#include <linux/videodev2.h>
#include <linux/uaccess.h>


#include "mtk_pq_hdr.h"
//#include "mtk_pq_tch.h"
//#include "mtk_pq_dolby.h"

#define STI_HDR_LOG(format, args...) pr_err(format, ##args)

// set ctrl
int mtk_hdr_SetHDRMode(struct st_hdr_info *hdr_info, __s32 mode)
{
	struct st_cfd_info *pstCFDInfo;
	__u8 hdr_enable_mode;

	STI_HDR_LOG("hdr enable mode = %d\n", mode);

	if (!hdr_info)
		return -EFAULT;

	pstCFDInfo = &hdr_info->stCFDInfo;
	hdr_enable_mode = (__u8)mode;

	mtk_cfd_SetUIHDR(pstCFDInfo, hdr_enable_mode);

	return 0;
}

int mtk_hdr_SetDolbyViewMode(struct st_hdr_info *hdr_info,
	__s32 mode)
{
	if (!hdr_info)
		return -EFAULT;

	STI_HDR_LOG("dolby view mode = %d\n", mode);

	// TODO by DV owner

	return 0;
}

int mtk_hdr_SetTmoLevel(struct st_hdr_info *hdr_info,
	__s32 level)
{
	struct st_cfd_info *cfd_info;
	__u8 tmo_level;

	STI_HDR_LOG("tmo level = %d\n", level);

	if (!hdr_info)
		return -EFAULT;

	cfd_info = &hdr_info->stCFDInfo;
	tmo_level = (__u8)level;

	mtk_cfd_SetUITMOLevel(cfd_info, tmo_level);

	return 0;

}

// get ctrl
int mtk_hdr_GetHDRType(struct st_hdr_info *hdr_info,
	__s32 *hdr_type)
{
	struct st_cfd_info *cfd_info;

	if ((!hdr_info) || (!hdr_type))
		return -EFAULT;

	cfd_info = &hdr_info->stCFDInfo;
	*hdr_type = (__s32)cfd_info->stCFD.u8HdrStatus;

	STI_HDR_LOG("HdrType = %d\n", *hdr_type);

	// TODO

	return 0;
}

// set ext ctrl
int mtk_hdr_SetDolby3DLut(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_dolby_3dlut dolby_3dlut;
	__u8 *pu8_3dlut = NULL;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	memset(&dolby_3dlut, 0, sizeof(struct v4l2_dolby_3dlut));
	memcpy(&dolby_3dlut, ctrl, sizeof(struct v4l2_dolby_3dlut));
	pu8_3dlut = kzalloc(dolby_3dlut.size, GFP_KERNEL);
	if (pu8_3dlut == NULL)
		return -EFAULT;

	if (copy_from_user((void *)pu8_3dlut,
		(__u8 __user *)dolby_3dlut.p.pu8_data, dolby_3dlut.size)) {
		kfree(pu8_3dlut);
		return -EFAULT;
	}

	STI_HDR_LOG("Size = %u\n", dolby_3dlut.size);

	//todo by dolby owner
	kfree(pu8_3dlut);

	return 0;
}

int mtk_hdr_SetCUSColorRange(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct st_cfd_info *cfd_info;
	struct v4l2_cus_color_range *cus_color_range;
	__u8 u8ColorRangeType;
	__u8 u8WhiteLevel;
	__u8 u8BlackLevel;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	cfd_info = &hdr_info->stCFDInfo;
	cus_color_range = (struct v4l2_cus_color_range *)ctrl;
	u8ColorRangeType = cus_color_range->cus_type;
	u8WhiteLevel = cus_color_range->ultral_white_level;
	u8BlackLevel = cus_color_range->ultral_black_level;

	STI_HDR_LOG("cus_type = %u\n", u8ColorRangeType);
	STI_HDR_LOG("ultral_black_level = %u\n",
		u8WhiteLevel);
	STI_HDR_LOG("ultral_white_level = %u\n",
		u8BlackLevel);

	mtk_cfd_SetUIColorRange(cfd_info, u8ColorRangeType,
		u8WhiteLevel, u8BlackLevel);

	return 0;
}

int mtk_hdr_SetCustomerSetting(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_cus_ip_setting cus_ip_setting;
	__u8 *pu8_setting = NULL;

	memset(&cus_ip_setting, 0, sizeof(struct v4l2_cus_ip_setting));
	memcpy(&cus_ip_setting, ctrl, sizeof(struct v4l2_cus_ip_setting));

	if (cus_ip_setting.ip >= V4L2_CUS_IP_MAX) {
		STI_HDR_LOG("customer setting error\n");
		return -EINVAL;
	}

	STI_HDR_LOG("CustomerIp = %u\n", cus_ip_setting.ip);

	pu8_setting = kzalloc(cus_ip_setting.param_size, GFP_KERNEL);
	if (pu8_setting == NULL)
		return -EFAULT;

	if (copy_from_user((void *)pu8_setting,
		(__u8 __user *)cus_ip_setting.p.pu8_param,
		cus_ip_setting.param_size)) {
		kfree(pu8_setting);
		return -EFAULT;
	}

	// TODO
	kfree(pu8_setting);

	return 0;
}

int mtk_hdr_SetHDRAttr(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_hdr_attr_info *hdr_attr_info;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	hdr_attr_info = (struct v4l2_hdr_attr_info *)ctrl;

	STI_HDR_LOG("hdr_type = %u\n", hdr_attr_info->hdr_type);
	STI_HDR_LOG("b_output_max_luminace = %u\n",
		hdr_attr_info->b_output_max_luminace);
	STI_HDR_LOG("output_max_luminace = %u\n",
		hdr_attr_info->output_max_luminace);
	STI_HDR_LOG("b_output_tr = %u\n", hdr_attr_info->b_output_tr);
	STI_HDR_LOG("enOutputTR = %u\n", hdr_attr_info->output_tr);

	// TODO by TCH owner

	return 0;
}

int mtk_hdr_SetSeamlessStatus(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_seamless_status *seamless_status;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	seamless_status = (struct v4l2_seamless_status *)ctrl;
	STI_HDR_LOG("b_hdr_changed = %u\n", seamless_status->b_hdr_changed);
	STI_HDR_LOG("b_cfd_updated = %u\n", seamless_status->b_cfd_updated);
	STI_HDR_LOG("b_mute_done = %u\n", seamless_status->b_mute_done);
	STI_HDR_LOG("b_reset_flag = %u\n", seamless_status->b_reset_flag);

	// TODO

	return 0;
}

int mtk_hdr_SetCusColorMetry(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct st_cfd_info *cfd_info;
	struct v4l2_cus_colori_metry *cus_colori_metry;
	bool bCusCP;
	__u16 u16WhitePointX;
	__u16 u16WhitePointY;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	cfd_info = &hdr_info->stCFDInfo;
	cus_colori_metry = (struct v4l2_cus_colori_metry *)ctrl;
	bCusCP = cus_colori_metry->b_customer_color_primaries;
	u16WhitePointX = cus_colori_metry->white_point_x;
	u16WhitePointY = cus_colori_metry->white_point_y;

	STI_HDR_LOG("b_customer_CP = %u\n", bCusCP);
	STI_HDR_LOG("white_point_x = %u\n", u16WhitePointX);
	STI_HDR_LOG("white_point_y = %u\n", u16WhitePointY);

	mtk_cfd_SetOutputCusCP(cfd_info, bCusCP, u16WhitePointX,
		u16WhitePointY);

	return 0;
}

int mtk_hdr_SetDolbyGDInfo(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_dolby_gd_info *dolby_gd_info;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	dolby_gd_info = (struct v4l2_dolby_gd_info *)ctrl;

	STI_HDR_LOG("b_global_dimming = %u\n",
		dolby_gd_info->b_global_dimming);
	STI_HDR_LOG("mm_delay_frame = %d\n",
		dolby_gd_info->mm_delay_frame);
	STI_HDR_LOG("hdmi_delay_frame = %d\n",
		dolby_gd_info->hdmi_delay_frame);

	// TODO by DV owner

	return 0;
}

// get ext ctrl
int mtk_hdr_GetSeamlessStatus(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_seamless_status *seamless_status;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	seamless_status = (struct v4l2_seamless_status *)ctrl;
	seamless_status->b_hdr_changed = false;
	seamless_status->b_cfd_updated = false;
	seamless_status->b_mute_done = false;
	seamless_status->b_reset_flag = false;

	STI_HDR_LOG("b_hdr_changed = %u\n", seamless_status->b_hdr_changed);
	STI_HDR_LOG("b_cfd_updated = %u\n", seamless_status->b_cfd_updated);
	STI_HDR_LOG("b_mute_done = %u\n", seamless_status->b_mute_done);
	STI_HDR_LOG("b_reset_flag = %u\n", seamless_status->b_reset_flag);

	// TODO

	return 0;
}


// set input
int mtk_hdr_SetInputSource(struct st_hdr_info *hdr_info,
	__u8 u8Input)
{
	if (!hdr_info)
		return -EFAULT;

	mtk_cfd_SetInputSource(&hdr_info->stCFDInfo, u8Input);

	return 0;
}

// set format
int mtk_hdr_SetPixFmtIn(struct st_hdr_info *hdr_info,
	struct v4l2_pix_format_mplane *pix)
{
	struct st_cfd_info *cfd_info;
	struct st_main_color_fmt main_fmt;

	if ((!pix) || (!hdr_info))
		return -EFAULT;

	STI_HDR_LOG("pixel format = %u\n", pix->pixelformat);
	STI_HDR_LOG("color space = %u\n", pix->colorspace);
	STI_HDR_LOG("quantization = %u\n", pix->quantization);
	STI_HDR_LOG("transfer function = %u\n", pix->xfer_func);

	cfd_info = &hdr_info->stCFDInfo;

	main_fmt.u8Fmt = 7;
	main_fmt.u8DataFmt = pix->pixelformat;
	main_fmt.u8CP = pix->colorspace;
	main_fmt.u8TC = pix->xfer_func;
	main_fmt.u8MC = pix->colorspace;
	main_fmt.u8BitDepth = 8;
	main_fmt.bFullRange = (pix->quantization == 1);

	mtk_cfd_SetInputMainFmt(cfd_info, &main_fmt);

	return 0;
}


int mtk_hdr_SetPixFmtOut(struct st_hdr_info *hdr_info,
	struct v4l2_pix_format_mplane *pix)
{
	struct st_cfd_info *cfd_info;
	struct st_main_color_fmt main_fmt;

	if ((!pix) || (!hdr_info))
		return -EFAULT;

	STI_HDR_LOG("pixel format = %u\n", pix->pixelformat);
	STI_HDR_LOG("color space = %u\n", pix->colorspace);
	STI_HDR_LOG("quantization = %u\n", pix->quantization);
	STI_HDR_LOG("transfer function = %u\n", pix->xfer_func);

	cfd_info = &hdr_info->stCFDInfo;

	main_fmt.u8Fmt = 3;
	main_fmt.u8DataFmt = pix->pixelformat;
	main_fmt.u8CP = pix->colorspace;
	main_fmt.u8TC = pix->xfer_func;
	main_fmt.u8MC = pix->colorspace;
	main_fmt.u8BitDepth = 8;
	main_fmt.bFullRange = (pix->quantization == 1);

	mtk_cfd_SetOutputMainFmt(cfd_info, &main_fmt);

	return 0;
}

// stream on
int mtk_hdr_StreamOn(struct st_hdr_info *hdr_info)
{
	struct st_cfd_info *cfd_info;

	STI_HDR_LOG("HDR stream on\n");

	cfd_info = &hdr_info->stCFDInfo;

	mtk_cfd_SetFire(cfd_info, true);

	return 0;
}

// stream off
int mtk_hdr_StreamOff(struct st_hdr_info *hdr_info)
{
	struct st_cfd_info *cfd_info;
	__u32 input = MTK_PQ_INPUTSRC_NONE;

	STI_HDR_LOG("HDR stream off\n");

	cfd_info = &hdr_info->stCFDInfo;

	mtk_cfd_SetInputSource(cfd_info, input);

	return 0;
}

int mtk_hdr_Open(struct st_hdr_info *hdr_info)
{
	if (!hdr_info)
		return -EFAULT;

	STI_HDR_LOG("HDR open\n");

	mtk_cfd_ResetCfdInfo(&hdr_info->stCFDInfo);

	return 0;
}

int mtk_hdr_Close(struct st_hdr_info *hdr_info)
{
	if (!hdr_info)
		return -EFAULT;

	STI_HDR_LOG("HDR close\n");
	//todo
	return 0;
}

int mtk_hdr_SubscribeEvent(struct st_hdr_info *hdr_info,
	__u32 event_type)
{
	if (!hdr_info)
		return -EFAULT;

	STI_HDR_LOG("subscribe event = %u\n", event_type);
	//todo
	return 0;
}

int mtk_hdr_UnsubscribeEvent(struct st_hdr_info *hdr_info,
	__u32 event_type)
{
	if (!hdr_info)
		return -EFAULT;

	STI_HDR_LOG("unsubscribe event = %u\n", event_type);
	//todo
	return 0;
}

