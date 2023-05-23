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

#include "mtk_srccap.h"
#include "mtk_srccap_dv.h"
#include "mtk_srccap_dv_common.h"

#include "hwreg_srccap_dv_common.h"

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global Functions
//-----------------------------------------------------------------------------
int mtk_dv_common_init(
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

int mtk_dv_common_qbuf(
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

int mtk_dv_common_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out)
{
	int ret = 0;
	u8 dev_id = 0;
	bool en = FALSE;
	bool riu = FALSE;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "common dequeue buffer\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	dev_id = in->dev->dev_id;

	/* enable 3way if dma is enabled. MUST check after dma status update */
	if (in->dev->dv_info.dma_info.dma_status == SRCCAP_DV_DMA_STATUS_ENABLE_FB)
		en = FALSE;
	else
		en = TRUE;

	if (g_dv_pr_level == SRCCAP_DV_PR_LEVEL_DISABLE)
		en = TRUE;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_PR,
		"pre-ip2 dv path disable: %d\n", en);

	riu = TRUE;
	ret = drv_hwreg_srccap_dv_set_pre_ip2_dvpath_disable(
			dev_id,
			en,
			riu,
			NULL,
			NULL);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "common dequeue buffer done\n");

exit:
	return ret;
}

int mtk_dv_common_update_deq_setting(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out)
{
	int ret = 0;
	int hw_ver = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "common update dequeue setting\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	srccap_dev = in->dev;
	hw_ver = srccap_dev->srccap_info.cap.u32DV_Srccap_HWVersion;

	if (hw_ver >= DV_SRCCAP_HW_TAG_V2)
		mtk_srccap_common_set_hdmi422pack(srccap_dev);

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "common update dequeue setting done\n");

exit:
	return ret;
}


//-----------------------------------------------------------------------------
// Debug Functions
//-----------------------------------------------------------------------------
