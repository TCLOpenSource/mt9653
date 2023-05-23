/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020-2021 MediaTek Inc.
 */

#ifndef __UAPI_MTK_M_SBUHAL_H
#define __UAPI_MTK_M_SBUHAL_H

#include <linux/types.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#endif

/* ============================================================== */
/* ------------------------ Metadata Tag ------------------------ */
/* ============================================================== */
#define META_SUBHAL_INPUTSRC_INFO  "meta_subhal_inputsrc_info"
#define META_SUBHAL_VDEC_INFO      "meta_subhal_vdec_info"
#define META_SUBHAL_RENDER_INFO    "meta_subhal_render_info"

/* ============================================================== */
/* -------------------- Metadata Tag Version -------------------- */
/* ============================================================== */
#define META_SUBHAL_INPUTSRC_INFO_VERSION (1)
#define META_SUBHAL_VDEC_INFO_VERSION (1)
#define META_SUBHAL_RENDER_INFO_VERSION (1)

/* ============================================================== */
/* ---------------------- Metadata Content ---------------------- */
/* ============================================================== */

enum meta_mute_action {
	no_mute_action = 0,
	freeze = 1,
	mute = 2,
	panel_mute = 3,
	backlight_mute = 4,
	tx_mute = 5,
};

struct meta_inputsrc_info {
	enum meta_mute_action mute_action;
	bool hdmi_mode;
};

struct meta_vdec_info {
	enum meta_mute_action mute_action;
};

struct meta_subhal_render_info {
	enum meta_mute_action mute_action;
};
#endif //__UAPI_MTK_M_SBUHAL_H

