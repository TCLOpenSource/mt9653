/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_VCODEC_INTR_H_
#define _MTK_VCODEC_INTR_H_

#include "mtk_vcodec_drv.h"

#define MTK_INST_IRQ_RECEIVED           0x1

struct mtk_vcodec_ctx;

/* timeout is ms */
int mtk_vcodec_wait_for_done_ctx(struct mtk_vcodec_ctx  *ctx,
	int core_id, int command, unsigned int timeout_ms);

int mtk_vcodec_dec_irq_setup(struct platform_device *pdev,
	struct mtk_vcodec_dev *dev);

int mtk_vcodec_enc_irq_setup(struct platform_device *pdev,
	struct mtk_vcodec_dev *dev);

#endif /* _MTK_VCODEC_INTR_H_ */
