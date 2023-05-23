/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_VCODEC_DEC_PM_H_
#define _MTK_VCODEC_DEC_PM_H_

#include "mtk_vcodec_drv.h"

void mtk_dec_init_ctx_pm(struct mtk_vcodec_ctx *ctx);
int mtk_vcodec_init_dec_pm(struct mtk_vcodec_dev *dev);
void mtk_vcodec_release_dec_pm(struct mtk_vcodec_dev *dev);
void mtk_vcodec_resume_dec_pm(struct mtk_vcodec_dev *mtkdev);

void mtk_vcodec_dec_pw_on(struct mtk_vcodec_pm *pm, int hw_id);
void mtk_vcodec_dec_pw_off(struct mtk_vcodec_pm *pm, int hw_id);
void mtk_vcodec_dec_clock_on(struct mtk_vcodec_pm *pm, int hw_id);
void mtk_vcodec_dec_clock_off(struct mtk_vcodec_pm *pm, int hw_id);

void mtk_prepare_vdec_dvfs(void);
void mtk_unprepare_vdec_dvfs(void);
void mtk_prepare_vdec_emi_bw(void);
void mtk_unprepare_vdec_emi_bw(void);

void mtk_vdec_pmqos_prelock(struct mtk_vcodec_ctx *ctx, int hw_id);
void mtk_vdec_pmqos_begin_frame(struct mtk_vcodec_ctx *ctx, int hw_id);
void mtk_vdec_pmqos_end_frame(struct mtk_vcodec_ctx *ctx, int hw_id);

#endif /* _MTK_VCODEC_DEC_PM_H_ */
