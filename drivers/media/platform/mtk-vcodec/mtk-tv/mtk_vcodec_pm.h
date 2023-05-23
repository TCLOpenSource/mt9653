/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_VCODEC_PM_H_
#define _MTK_VCODEC_PM_H_

#define MTK_PLATFORM_STR        "platform:mt6779"

#define E_VDEC_CLK_VDEC_NUM		16
/**
 * struct mtk_vcodec_pm - Power management data structure
 */
struct mtk_vcodec_pm {
	u32             vdec_clk_count;
	struct clk      *vdec_clk[E_VDEC_CLK_VDEC_NUM];
	struct clk      *venc_clk;
	struct clk      *venc_ip;
	const char      *vdec_clkname[E_VDEC_CLK_VDEC_NUM];
};

enum mtk_dec_dtsi_reg_idx {
	VDEC_SYS,
	VDEC_MISC,
	NUM_MAX_VDEC_REG_BASE = VDEC_SYS, // means no available REG
};

enum mtk_enc_dtsi_reg_idx {
	VENC_LT_SYS,
	VENC_SYS,
	NUM_MAX_VENC_REG_BASE
};

#endif /* _MTK_VCODEC_PM_H_ */
