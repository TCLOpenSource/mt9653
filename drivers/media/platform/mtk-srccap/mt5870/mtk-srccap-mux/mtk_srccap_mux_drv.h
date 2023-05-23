/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

void mtk_srccap_drv_mux_set_source(
			enum v4l2_srccap_input_source enInputSource,
			struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_drv_mux_skip_swreset(bool bSkip);
int mtk_srccap_drv_mux_disable_inputsource(bool bDisable,
			struct mtk_srccap_dev *srccap_dev);

