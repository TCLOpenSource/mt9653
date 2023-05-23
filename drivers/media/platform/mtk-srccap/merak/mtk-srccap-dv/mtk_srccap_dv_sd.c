// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
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
#include <media/v4l2-event.h>
#include <asm-generic/div64.h>

#include "mtk_srccap.h"
#include "mtk_srccap_dv.h"
#include "mtk_srccap_dv_sd.h"

#include "hwreg_srccap_dv_sd.h"

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define DV_SD_FORMULA_UNIT_2 (2)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------
struct dv_sd_info {
	u16 input_w;
	u16 input_h;
	u16 output_w;
	u16 output_h;
	bool h_filter_mode;
	bool v_filter_mode;
	bool h_scaling_en;
	bool v_scaling_en;
	u32 h_scaling_ratio;
	u32 v_scaling_ratio;
};

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------
int dv_get_scaling_params(struct dv_sd_info *sd_info)
{
	int ret = 0;
	u64 ratio = 0;

	if (sd_info == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if ((sd_info->input_w  <= 1) ||
		(sd_info->input_h  <= 1) ||
		(sd_info->output_w <= 1) ||
		(sd_info->output_h <= 1)) {
		ret = -EINVAL;
		goto exit;
	}

	if (sd_info->input_w > sd_info->output_w) {
		sd_info->h_scaling_en = TRUE;

		if (sd_info->h_filter_mode) {
			ratio = ((u64)(sd_info->input_w - 1)) * 2097152ul;
			do_div(ratio, (sd_info->output_w - 1));
			ratio = ratio + 1;
			do_div(ratio, DV_SD_FORMULA_UNIT_2);
		} else {
			ratio = ((u64)(sd_info->output_w)) * 2097152ul;
			do_div(ratio, sd_info->input_w);
			ratio = ratio + 1;
			do_div(ratio, DV_SD_FORMULA_UNIT_2);
		}

		sd_info->h_scaling_ratio = (u32)ratio;
	}

	if (sd_info->input_h > sd_info->output_h) {
		sd_info->v_scaling_en = TRUE;

		if (sd_info->v_filter_mode) {
			ratio = ((u64)(sd_info->input_h - 1)) * 1048576ul;
			do_div(ratio, (sd_info->output_h - 1));
		} else {
			ratio = ((u64)sd_info->output_h) * 1048576ul;
			do_div(ratio, (sd_info->input_h + 1));
		}

		sd_info->v_scaling_ratio = (u32)ratio;
	}

exit:
	return ret;
}

//-----------------------------------------------------------------------------
// Global Functions
//-----------------------------------------------------------------------------
int mtk_dv_sd_init(
	struct srccap_dv_init_in *in,
	struct srccap_dv_init_out *out)
{
	int ret = 0;

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

exit:
	return ret;
}

int mtk_dv_sd_qbuf(
	struct srccap_dv_qbuf_in *in,
	struct srccap_dv_qbuf_out *out)
{
	int ret = 0;

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

exit:
	return ret;
}

int mtk_dv_sd_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out)
{
	int ret = 0;
	u8 dev_id = 0;
	struct dv_sd_info sd_info;
	struct reg_srccap_dv_sd_info reg_sd_info;
	bool riu = FALSE;

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	dev_id = in->dev->dev_id;

	memset(&sd_info, 0, sizeof(struct dv_sd_info));
	memset(&reg_sd_info, 0, sizeof(struct reg_srccap_dv_sd_info));

	/* enable sd if dma is enabled. MUST check after dma status update */
	if (in->dev->dv_info.dma_info.dma_status == SRCCAP_DV_DMA_STATUS_ENABLE_FB) {
		/* get width and height of input */
		sd_info.input_w = in->dev->dv_info.dma_info.mem_fmt.src_width;
		sd_info.input_h = in->dev->dv_info.dma_info.mem_fmt.src_height;

		/* get width and height of output */
		sd_info.output_w = in->dev->dv_info.dma_info.mem_fmt.dma_width;
		sd_info.output_h = in->dev->dv_info.dma_info.mem_fmt.dma_height;

		/* get filter mode */
		sd_info.h_filter_mode = FALSE;
		sd_info.v_filter_mode = FALSE;

		/* get scaling parameters */
		ret = dv_get_scaling_params(&sd_info);
		if (ret < 0) {
			SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
			goto exit;
		}
	} else {
		sd_info.h_scaling_en = FALSE;
		sd_info.v_scaling_en = FALSE;
	}

	reg_sd_info.h_scaling_en = sd_info.h_scaling_en;
	reg_sd_info.h_scaling_ratio = sd_info.h_scaling_ratio;
	reg_sd_info.h_filter_mode = sd_info.h_filter_mode;
	reg_sd_info.v_scaling_en = sd_info.v_scaling_en;
	reg_sd_info.v_scaling_ratio = sd_info.v_scaling_ratio;
	reg_sd_info.v_filter_mode = sd_info.v_filter_mode;
	reg_sd_info.vsd_ipm_fetch = sd_info.output_h;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_PR,
		"input width, height: (%u, %u), output width, height: (%u, %u), ",
		sd_info.input_w, sd_info.input_h,
		sd_info.output_w, sd_info.output_h);

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_PR,
		"enable, ratio, mode of hsd: (%d, %u, %u), vsd: (%d, %u, %u)\n",
		reg_sd_info.h_scaling_en,
		reg_sd_info.h_scaling_ratio,
		reg_sd_info.h_filter_mode,
		reg_sd_info.v_scaling_en,
		reg_sd_info.v_scaling_ratio,
		reg_sd_info.v_filter_mode);

	riu = TRUE;
	ret = drv_hwreg_srccap_dv_sd_set_scaling(
			dev_id,
			riu,
			NULL,
			NULL,
			&reg_sd_info);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

exit:
	return ret;
}

//-----------------------------------------------------------------------------
// Debug Functions
//-----------------------------------------------------------------------------
