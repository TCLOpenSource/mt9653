// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/errno.h>
#include <linux/wait.h>

#include "mtk_vcodec_intr.h"
#include "mtk_vcodec_util.h"

int mtk_vcodec_wait_for_done_ctx(struct mtk_vcodec_ctx  *ctx,
	int core_id, int command, unsigned int timeout_ms)
{
	int status = 0;
	wait_queue_head_t *waitqueue;
	long timeout_jiff, ret;

	if (core_id >= MTK_VDEC_HW_NUM ||
		core_id < 0) {
		mtk_v4l2_err("ctx %d, invalid core_id=%d", ctx->id, core_id);
		return -1;
	}

	waitqueue = (wait_queue_head_t *)&ctx->queue[core_id];
	timeout_jiff = msecs_to_jiffies(timeout_ms);

	ret = wait_event_interruptible_timeout(*waitqueue,
		ctx->int_cond[core_id],
		timeout_jiff);

	if (!ret) {
		status = -1;    /* timeout */
		mtk_v4l2_err("[%d] cmd=%d, ctx->type=%d, core_id %d timeout time=%ums out %d %d!",
			ctx->id, ctx->type, core_id, command, timeout_ms,
			ctx->int_cond[core_id], ctx->int_type);
	} else if (-ERESTARTSYS == ret) {
		mtk_v4l2_err("[%d] cmd=%d, ctx->type=%d, core_id %d timeout interrupted by a signal %d %d",
			ctx->id, ctx->type, core_id,
			command, ctx->int_cond[core_id],
			ctx->int_type);
		status = -2;
	}

	ctx->int_cond[core_id] = 0;
	ctx->int_type = 0;

	return status;
}
EXPORT_SYMBOL(mtk_vcodec_wait_for_done_ctx);

int mtk_vcodec_dec_irq_setup(struct platform_device *pdev,
	struct mtk_vcodec_dev *dev)
{
	return 0;
}
EXPORT_SYMBOL(mtk_vcodec_dec_irq_setup);

int mtk_vcodec_enc_irq_setup(struct platform_device *pdev,
	struct mtk_vcodec_dev *dev)
{
	return 0;
}
EXPORT_SYMBOL(mtk_vcodec_enc_irq_setup);
