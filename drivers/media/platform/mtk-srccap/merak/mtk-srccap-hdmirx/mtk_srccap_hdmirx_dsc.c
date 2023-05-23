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

#include "hwreg_srccap_hdmirx_dsc.h"

static void getV4lFromST_HDMI_DSC_INFO(struct v4l2_ext_hdmi_dsc_info *p_info,
				       ST_HDMI_DSC_INFO *dsc_info_buf)
{
	p_info->u8_bpc = dsc_info_buf->u8_bpc;
	p_info->u8_linebuf_depth = dsc_info_buf->u8_linebuf_depth;
	p_info->u16_bpp = dsc_info_buf->u16_bpp;
	p_info->u16_pic_height = dsc_info_buf->u16_pic_height;
	p_info->u16_pic_width = dsc_info_buf->u16_pic_width;
	p_info->u16_slice_height = dsc_info_buf->u16_slice_height;
	p_info->u16_slice_width = dsc_info_buf->u16_slice_width;
	p_info->u16_chunkbytes = dsc_info_buf->u16_chunkbytes;
	p_info->u16_initial_dec_delay = dsc_info_buf->u16_initial_dec_delay;
	p_info->u8_native_422 = dsc_info_buf->u8_native_422;
	p_info->u8_native_420 = dsc_info_buf->u8_native_420;
	p_info->u16_hfront = dsc_info_buf->u16_hfront;
	p_info->u16_hsync = dsc_info_buf->u16_hsync;
	p_info->u16_hback = dsc_info_buf->u16_hback;
	p_info->u16_hactivebytes = dsc_info_buf->u16_hactivebytes;
	p_info->u8_slice_num = dsc_info_buf->u8_slice_num;
}

bool mtk_srccap_hdmirx_dsc_get_pps_info(
	enum v4l2_srccap_input_source src,
	struct v4l2_ext_hdmi_dsc_info *p_info)
{
	ST_HDMI_DSC_INFO dsc_info_buf;

	if (!p_info)
		return false;

	if (!drv_hwreg_srccap_hdmirx_get_dsc_pps_info(
		mtk_hdmirx_to_muxinputport(src),
		&dsc_info_buf))
		return false;

	//memcpy((void *)p_info, (void *)&dsc_info_buf,
	//	sizeof(struct v4l2_ext_hdmi_dsc_info));

	getV4lFromST_HDMI_DSC_INFO(p_info, &dsc_info_buf);

	return true;
}

