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

#include "hwreg_srccap_hdmirx_mux.h"
#include "hwreg_srccap_hdmirx_dsc.h"

bool mtk_srccap_hdmirx_mux_sel_dsc(
	enum v4l2_srccap_input_source src,
	enum v4l2_ext_hdmi_dsc_decoder_mux_n dsc_mux_n)
{
	return drv_hwreg_srccap_hdmirx_mux_sel_dsc(
		mtk_hdmirx_to_muxinputport(src),
		(enum e_dsc_decoder_mux_n)dsc_mux_n);
}

enum v4l2_srccap_input_source mtk_srccap_hdmirx_mux_query_dsc(
	enum v4l2_ext_hdmi_dsc_decoder_mux_n dsc_mux_n)
{
	return mtk_hdmirx_muxinput_to_v4l2src(
		drv_hwreg_srccap_hdmirx_mux_query_dsc(
		(enum e_dsc_decoder_mux_n)dsc_mux_n)
		);
}


