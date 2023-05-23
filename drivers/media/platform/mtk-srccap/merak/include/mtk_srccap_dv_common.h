/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DV_COMMON_H
#define MTK_SRCCAP_DV_COMMON_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define DV_SRCCAP_HW_TAG_V1               (1)
#define DV_SRCCAP_HW_TAG_V2               (2)
#define DV_SRCCAP_HW_TAG_V3               (3)

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
int mtk_dv_common_init(
	struct srccap_dv_init_in *in,
	struct srccap_dv_init_out *out);

int mtk_dv_common_qbuf(
	struct srccap_dv_qbuf_in *in,
	struct srccap_dv_qbuf_out *out);

int mtk_dv_common_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out);

int mtk_dv_common_update_deq_setting(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out);

#endif  // MTK_SRCCAP_DV_COMMON_H
