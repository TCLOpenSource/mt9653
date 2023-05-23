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

enum srccap_stub_mode stub_srccap_dv;


//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global Functions
//-----------------------------------------------------------------------------
int mtk_dv_init(
	struct srccap_dv_init_in *in,
	struct srccap_dv_init_out *out)
{
	int ret = 0;

	g_dv_debug_level = SRCCAP_DV_DBG_LEVEL_ERR;
	g_dv_pr_level = SRCCAP_DV_PR_LEVEL_DEFAULT;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "init\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	ret = mtk_dv_common_init(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_descrb_init(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_dma_init(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_sd_init(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "init done\n");

exit:
	return ret;
}

void mtk_dv_resume(
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	struct srccap_dv_init_in dv_in;
	struct srccap_dv_init_out dv_out;

	memset(&dv_in, 0, sizeof(struct srccap_dv_init_in));
	memset(&dv_out, 0, sizeof(struct srccap_dv_init_out));
	dv_in.dev = srccap_dev;

	ret = mtk_dv_init(&dv_in, &dv_out);
	if (ret < 0)
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
}


int mtk_dv_set_fmt_mplane(
	struct srccap_dv_set_fmt_mplane_in *in,
	struct srccap_dv_set_fmt_mplane_out *out)
{
	int ret = 0;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "set format\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	ret = mtk_dv_dma_set_fmt_mplane(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "set format done\n");

exit:
	return ret;
}

int mtk_dv_streamon(
	struct srccap_dv_streamon_in *in,
	struct srccap_dv_streamon_out *out)
{
	int ret = 0;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "stream on\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	ret = mtk_dv_dma_streamon(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_descrb_streamon(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "stream on done\n");

exit:
	return ret;
}

int mtk_dv_streamoff(
	struct srccap_dv_streamoff_in *in,
	struct srccap_dv_streamoff_out *out)
{
	int ret = 0;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "stream off\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	ret = mtk_dv_descrb_streamoff(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_dma_streamoff(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "stream off done\n");

exit:
	return ret;
}

int mtk_dv_qbuf(
	struct srccap_dv_qbuf_in *in,
	struct srccap_dv_qbuf_out *out)
{
	int ret = 0;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "queue buffer\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "queue buffer done\n");

exit:
	return ret;
}

int mtk_dv_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out)
{
	int ret = 0;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "dequeue buffer\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	/* get Dolby HDMI interface */
	ret = mtk_dv_descrb_dqbuf(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_dma_dqbuf(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_common_dqbuf(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_sd_dqbuf(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_meta_dqbuf(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	ret = mtk_dv_common_update_deq_setting(in, out);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "dequeue buffer done\n");

exit:
	return ret;
}

void mtk_dv_handle_irq(
	void *param)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "handle IRQ\n");

	if (param == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	srccap_dev = (struct mtk_srccap_dev *)param;

	mtk_dv_descrb_handle_irq(param);

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "handle IRQ done\n");

exit:
	return;
}

//-----------------------------------------------------------------------------
// Debug Functions
//-----------------------------------------------------------------------------
int mtk_dv_show(
	struct device *dev,
	const char *buf)
{
	return mtk_dv_debug_show(dev, buf);
}

int mtk_dv_store(
	struct device *dev,
	const char *buf)
{
	return mtk_dv_debug_store(dev, buf);
}

int mtk_dv_set_stub(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_stub_mode stub)
{
	int ret = 0;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "received stub mode = %d\n", stub);

	if (srccap_dev == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (stub == SRCCAP_STUB_MODE_DV_FORCE_IRQ) {
		mtk_dv_handle_irq(srccap_dev);
		goto exit;
	}

	if (stub < SRCCAP_STUB_MODE_DV || stub >= SRCCAP_STUB_MODE_DV_MAX)
		stub = SRCCAP_STUB_MODE_OFF;

	stub_srccap_dv = stub;

	ret = mtk_dv_descrb_set_stub(stub);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

exit:
	return ret;
}
