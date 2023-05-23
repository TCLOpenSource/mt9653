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

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include "mtk_pq.h"
#include "mtk_pq_hdr.h"
//#include "mtk_pq_tch.h"
//#include "mtk_pq_dolby.h"
#include "mtk_pq_common.h"
#include "mtk_pq_buffer.h"

extern bool bPquEnable;

static bool bDVBin_mmp;
static bool bIsDVBinDone;

// set ctrl
int mtk_hdr_SetHDRMode(struct st_hdr_info *hdr_info, __s32 mode)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "hdr enable mode = %d\n", mode);

	if (!hdr_info)
		return -EFAULT;

	return 0;
}

int mtk_hdr_SetDolbyViewMode(struct st_hdr_info *hdr_info,
	__s32 mode)
{
	if (!hdr_info)
		return -EFAULT;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "dolby view mode = %d\n", mode);

	// TODO by DV owner

	return 0;
}

int mtk_hdr_SetTmoLevel(struct st_hdr_info *hdr_info,
	__s32 level)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "tmo level = %d\n", level);

	if (!hdr_info)
		return -EFAULT;

	return 0;
}

// get ctrl
int mtk_hdr_GetHDRType(struct st_hdr_info *hdr_info,
	__s32 *hdr_type)
{
	if ((!hdr_info) || (!hdr_type))
		return -EFAULT;

	*hdr_type = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "HdrType = %d\n", *hdr_type);

	// TODO

	return 0;
}

// set ext ctrl
int mtk_hdr_SetDolbyBinMmpStatus(struct st_hdr_info *hdr_info, void *ctrl)
{
	int ret = 0;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	bDVBin_mmp = *((bool *)ctrl);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "[DV CFG] SetDolbyBinMmpStatus = %d\n", bDVBin_mmp);

	return ret;
}

int mtk_hdr_SetDVBinDone(struct st_hdr_info *hdr_info, void *ctrl,
	__u64 dolby_config_va_addr, __u64 dolby_config_bus_addr)
{
	int ret = 0;
	v4l2_PQ_dv_config_info_t *dv_config_info = NULL;
	struct msg_config_info dv_msg_config_info;
	bool is_pqu;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	bIsDVBinDone = *((bool *)ctrl);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"[DV CFG] SetDVBinDone = %d, bPquEnable = %d, kernel va = 0x%llX, pa = 0x%llX\n",
		bIsDVBinDone, bPquEnable, dolby_config_va_addr, dolby_config_bus_addr);

	memset(&dv_msg_config_info, 0, sizeof(struct msg_config_info));

	if (bIsDVBinDone) {
		dv_config_info = (v4l2_PQ_dv_config_info_t *)dolby_config_va_addr;
		dv_config_info->bin_info.pa = dolby_config_bus_addr + DV_CONFIG_BIN_OFFSET;
		dv_config_info->bin_info.va = dolby_config_va_addr + DV_CONFIG_BIN_OFFSET;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"[DV CFG] dolby bin: va=0x%llx, pa=0x%llx, size=%d\n",
			(__u64)dv_config_info->bin_info.va,
			(__u64)dv_config_info->bin_info.pa,
			dv_config_info->bin_info.size);

		if (bPquEnable) {
			dv_msg_config_info.dvconfig = dolby_config_bus_addr;
			is_pqu = true;
		} else {
			dv_msg_config_info.dvconfig = dolby_config_va_addr;
			is_pqu = false;
		}

		dv_msg_config_info.dvconfigsize = sizeof(v4l2_PQ_dv_config_info_t);
		dv_msg_config_info.dvconfig_en = true;
		mtk_pq_common_config(&dv_msg_config_info, is_pqu);
	}

	return ret;
}

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

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "Size = %u\n", dolby_3dlut.size);

	//todo by dolby owner
	kfree(pu8_3dlut);

	return 0;
}

int mtk_hdr_SetCUSColorRange(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_cus_color_range *cus_color_range;
	__u8 u8ColorRangeType;
	__u8 u8WhiteLevel;
	__u8 u8BlackLevel;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	cus_color_range = (struct v4l2_cus_color_range *)ctrl;
	u8ColorRangeType = cus_color_range->cus_type;
	u8WhiteLevel = cus_color_range->ultral_white_level;
	u8BlackLevel = cus_color_range->ultral_black_level;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "cus_type = %u\n", u8ColorRangeType);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "ultral_black_level = %u\n",
		u8WhiteLevel);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "ultral_white_level = %u\n",
		u8BlackLevel);

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
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "customer setting error\n");
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "CustomerIp = %u\n", cus_ip_setting.ip);

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

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "hdr_type = %u\n", hdr_attr_info->hdr_type);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_output_max_luminace = %u\n",
		hdr_attr_info->b_output_max_luminace);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "output_max_luminace = %u\n",
		hdr_attr_info->output_max_luminace);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_output_tr = %u\n", hdr_attr_info->b_output_tr);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "enOutputTR = %u\n", hdr_attr_info->output_tr);

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
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_hdr_changed = %u\n",
		seamless_status->b_hdr_changed);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_cfd_updated = %u\n",
		seamless_status->b_cfd_updated);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_mute_done = %u\n",
		seamless_status->b_mute_done);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_reset_flag = %u\n",
		seamless_status->b_reset_flag);

	// TODO

	return 0;
}

int mtk_hdr_SetCusColorMetry(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_cus_colori_metry *cus_colori_metry;
	bool bCusCP;
	__u16 u16WhitePointX;
	__u16 u16WhitePointY;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	cus_colori_metry = (struct v4l2_cus_colori_metry *)ctrl;
	bCusCP = cus_colori_metry->b_customer_color_primaries;
	u16WhitePointX = cus_colori_metry->white_point_x;
	u16WhitePointY = cus_colori_metry->white_point_y;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_customer_CP = %u\n", bCusCP);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "white_point_x = %u\n", u16WhitePointX);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "white_point_y = %u\n", u16WhitePointY);

	return 0;
}

int mtk_hdr_SetDolbyGDInfo(struct st_hdr_info *hdr_info,
	void *ctrl)
{
	struct v4l2_dolby_gd_info *dolby_gd_info;

	if ((!hdr_info) || (!ctrl))
		return -EFAULT;

	dolby_gd_info = (struct v4l2_dolby_gd_info *)ctrl;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_global_dimming = %u\n",
		dolby_gd_info->b_global_dimming);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "mm_delay_frame = %d\n",
		dolby_gd_info->mm_delay_frame);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "hdmi_delay_frame = %d\n",
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

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_hdr_changed = %u\n",
		seamless_status->b_hdr_changed);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_cfd_updated = %u\n",
		seamless_status->b_cfd_updated);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_mute_done = %u\n",
		seamless_status->b_mute_done);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "b_reset_flag = %u\n",
		seamless_status->b_reset_flag);

	// TODO

	return 0;
}


// set input
int mtk_hdr_SetInputSource(struct st_hdr_info *hdr_info,
	__u8 u8Input)
{
	if (!hdr_info)
		return -EFAULT;

	return 0;
}

// set format
int mtk_hdr_SetPixFmtIn(struct st_hdr_info *hdr_info,
	struct v4l2_pix_format_mplane *pix)
{
	if ((!pix) || (!hdr_info))
		return -EFAULT;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "pixel format = %u\n", pix->pixelformat);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "color space = %u\n", pix->colorspace);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "quantization = %u\n", pix->quantization);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "transfer function = %u\n", pix->xfer_func);

	return 0;
}


int mtk_hdr_SetPixFmtOut(struct st_hdr_info *hdr_info,
	struct v4l2_pix_format_mplane *pix)
{
	if ((!pix) || (!hdr_info))
		return -EFAULT;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "pixel format = %u\n", pix->pixelformat);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "color space = %u\n", pix->colorspace);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "quantization = %u\n", pix->quantization);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "transfer function = %u\n", pix->xfer_func);

	return 0;
}

// stream on
int mtk_hdr_StreamOn(struct st_hdr_info *hdr_info)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "HDR stream on\n");

	return 0;
}

// stream off
int mtk_hdr_StreamOff(struct st_hdr_info *hdr_info)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "HDR stream off\n");

	return 0;
}

int mtk_hdr_Open(struct st_hdr_info *hdr_info)
{
	if (!hdr_info)
		return -EFAULT;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "HDR open\n");

	return 0;
}

int mtk_hdr_Close(struct st_hdr_info *hdr_info)
{
	if (!hdr_info)
		return -EFAULT;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "HDR close\n");
	//todo
	return 0;
}

int mtk_hdr_SubscribeEvent(struct st_hdr_info *hdr_info,
	__u32 event_type)
{
	if (!hdr_info)
		return -EFAULT;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "subscribe event = %u\n", event_type);
	//todo
	return 0;
}

int mtk_hdr_UnsubscribeEvent(struct st_hdr_info *hdr_info,
	__u32 event_type)
{
	if (!hdr_info)
		return -EFAULT;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "unsubscribe event = %u\n", event_type);
	//todo
	return 0;
}

bool mtk_hdr_GetDVBinMmpStatus(void)
{
	return bDVBin_mmp;
}

bool mtk_hdr_GetDVBinDone(void)
{
	return bIsDVBinDone;
}
