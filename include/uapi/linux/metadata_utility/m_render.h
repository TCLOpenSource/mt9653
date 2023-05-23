/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __UAPI_MTK_M_RENDER_H
#define __UAPI_MTK_M_RENDER_H

#include <linux/types.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#endif

/* ============================================================== */
/* ------------------------ Metadata Tag ------------------------- */
/* ============================================================== */
//#define RENDER_FRAME_INFO_TAG			"DISP_FRM_INFO"
#define RENDER_GOP_INFO_TAG			"RENDER_GOP_INFO"

/* ============================================================== */
/* -------------------- Metadata Tag Version --------------------- */
/* ============================================================== */
#define META_RENDER_FRAME_INFO_VERSION (1)

/* ============================================================== */
/* ---------------------- Metadata Content ----------------------- */
/* ============================================================== */

enum meta_render_gop_out_fmt {
	meta_render_gop_out_yuv444 = 0,
	meta_render_gop_out_yuv422,
	meta_render_gop_out_yuv420,
	meta_render_gop_out_rgb,
};

struct meta_render_gop_info {
	uint8_t gop_out_multi_pixel;
	uint32_t gop_out_width;
	uint32_t gop_out_height;
	enum meta_render_gop_out_fmt gop_out_fmt;
};
#endif

