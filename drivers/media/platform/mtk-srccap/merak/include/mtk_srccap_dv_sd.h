/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DV_SD_H
#define MTK_SRCCAP_DV_SD_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define SRCCAP_DV_UHD_SD_RATIO (4)
#define SRCCAP_DV_FHD_SD_RATIO (2)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------
struct srccap_dv_init_in;
struct srccap_dv_init_out;
struct srccap_dv_qbuf_in;
struct srccap_dv_qbuf_out;
struct srccap_dv_dqbuf_in;
struct srccap_dv_dqbuf_out;

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------
int mtk_dv_sd_init(
	struct srccap_dv_init_in *in,
	struct srccap_dv_init_out *out);

int mtk_dv_sd_qbuf(
	struct srccap_dv_qbuf_in *in,
	struct srccap_dv_qbuf_out *out);

int mtk_dv_sd_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out);

#endif  // MTK_SRCCAP_DV_SD_H
