// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
 *         Tiffany Lin <tiffany.lin@mediatek.com>
 */

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "venc_drv_base.h"
#include "venc_drv_if.h"

#include "mtk_vcodec_enc.h"
#include "mtk_vcodec_enc_pm.h"

#if IS_ENABLED(CONFIG_VIDEO_MEDIATEK_VCU)
#include "mtk_vcu.h"
const struct venc_common_if *get_enc_common_if(void);
#endif

#ifdef CONFIG_VIDEO_MEDIATEK_VPU
#include "mtk_vpu.h"
const struct venc_common_if *get_h264_enc_comm_if(void);
const struct venc_common_if *get_vp8_enc_comm_if(void);
#endif

#ifdef TV_ENCODE_INTEGRATION
const struct venc_common_if *get_h264_enc_utopia_comm_if(void);
#endif

int venc_if_init(struct mtk_vcodec_ctx *ctx, unsigned int fourcc)
{
	int ret = 0;

	ctx->oal_vcodec = 0;
	mtk_venc_init_ctx_pm(ctx);

#if IS_ENABLED(CONFIG_VIDEO_MEDIATEK_VCU)
	switch (fourcc) {
	case V4L2_PIX_FMT_H264:
#ifndef TV_ENCODE_INTEGRATION
	case V4L2_PIX_FMT_H265:
	case V4L2_PIX_FMT_HEIF:
	case V4L2_PIX_FMT_MPEG4:
	case V4L2_PIX_FMT_H263:
#else
	case V4L2_PIX_FMT_VP8:
#endif
		ctx->enc_if = get_enc_common_if();
		ctx->oal_vcodec = 0;
		break;
	default:
		return -EINVAL;
	}
#endif
#ifdef CONFIG_VIDEO_MEDIATEK_VPU
	switch (fourcc) {
	case V4L2_PIX_FMT_VP8:
		ctx->enc_if = get_vp8_enc_comm_if();
		break;
	case V4L2_PIX_FMT_H264:
		ctx->enc_if = get_h264_enc_comm_if();
		break;
	default:
		return -EINVAL;
	}
#endif
#ifdef TV_ENCODE_INTEGRATION_UTOPIA
	ctx->enc_if = get_h264_enc_utopia_comm_if();
#endif
	ret = ctx->enc_if->init(ctx, (unsigned long *)&ctx->drv_handle);

	return ret;
}

// TODO: Change to original flow in M6.
#if IS_ENABLED(CONFIG_VIDEO_MEDIATEK_VCU)
int venc_if_get_param(struct mtk_vcodec_ctx *ctx, enum venc_get_param_type type,
					  void *out)
{
	struct venc_inst *inst = NULL;
	int ret = 0;
	int drv_handle_exist = 1;

	if (!ctx->drv_handle) {
		inst = kzalloc(sizeof(struct venc_inst), GFP_KERNEL);
		inst->ctx = ctx;
		ctx->drv_handle = (unsigned long)(inst);
		ctx->enc_if = get_enc_common_if();
		drv_handle_exist = 0;
		mtk_v4l2_debug(0, "%s init drv_handle = 0x%lx",
			__func__, ctx->drv_handle);
	}

	ret = ctx->enc_if->get_param(ctx->drv_handle, type, out);

	if (!drv_handle_exist) {
		kfree(inst);
		ctx->drv_handle = 0;
		ctx->enc_if = NULL;
	}

	return ret;
}
#endif

int venc_if_set_param(struct mtk_vcodec_ctx *ctx,
	enum venc_set_param_type type, struct venc_enc_param *in)
{
	int ret = 0;

	ret = ctx->enc_if->set_param(ctx->drv_handle, type, in);

	return ret;
}

void venc_encode_prepare(void *ctx_prepare, int core_id, unsigned long *flags)
{
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)ctx_prepare;

	if (ctx == NULL)
		return;

	mtk_venc_pmqos_prelock(ctx, core_id);
	mtk_venc_lock(ctx, core_id);
	mtk_venc_pmqos_begin_frame(ctx, core_id);
	spin_lock_irqsave(&ctx->dev->irqlock, *flags);
	ctx->dev->curr_enc_ctx[0] = ctx;
	spin_unlock_irqrestore(&ctx->dev->irqlock, *flags);
	mtk_vcodec_enc_clock_on(ctx, core_id);
}
EXPORT_SYMBOL_GPL(venc_encode_prepare);

void venc_encode_unprepare(void *ctx_unprepare,
	int core_id, unsigned long *flags)
{
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)ctx_unprepare;

	if (ctx == NULL)
		return;

	if (core_id < 0 || core_id >= MTK_VENC_HW_NUM)
		return;

	if (ctx->dev->enc_sem[core_id].count != 0) {
		mtk_v4l2_err("HW not prepared, enc_sem[%d].count = %d",
			core_id, ctx->dev->enc_sem[core_id].count);
		return;
	}

	mtk_vcodec_enc_clock_off(ctx, core_id);
	spin_lock_irqsave(&ctx->dev->irqlock, *flags);
	ctx->dev->curr_enc_ctx[0] = NULL;
	spin_unlock_irqrestore(&ctx->dev->irqlock, *flags);
	mtk_venc_pmqos_end_frame(ctx, core_id);
	mtk_venc_unlock(ctx, core_id);
}
EXPORT_SYMBOL_GPL(venc_encode_unprepare);

#ifndef TV_ENCODE_INTEGRATION
void venc_encode_pmqos_gce_begin(void *ctx_begin, int core_id, int job_cnt)
{
	mtk_venc_pmqos_gce_flush(ctx_begin, core_id, job_cnt);
}
EXPORT_SYMBOL_GPL(venc_encode_pmqos_gce_begin);

void venc_encode_pmqos_gce_end(void *ctx_end, int core_id, int job_cnt)
{
	mtk_venc_pmqos_gce_done(ctx_end, core_id, job_cnt);
}
EXPORT_SYMBOL_GPL(venc_encode_pmqos_gce_end);
#endif

int venc_if_encode(struct mtk_vcodec_ctx *ctx,
	enum venc_start_opt opt, struct venc_frm_buf *frm_buf,
	struct mtk_vcodec_mem *bs_buf,
	struct venc_done_result *result)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return 0;

	ret = ctx->enc_if->encode(ctx->drv_handle, opt, frm_buf,
							  bs_buf, result);

	return ret;
}

int venc_if_deinit(struct mtk_vcodec_ctx *ctx)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return 0;

	ret = ctx->enc_if->deinit(ctx->drv_handle);

	ctx->drv_handle = 0;

	mtk_venc_deinit_ctx_pm(ctx);

	return ret;
}
