/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DV_META_H
#define MTK_SRCCAP_DV_META_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------
enum srccap_dv_meta_tag {
	SRCCAP_DV_META_TAG_DV_INFO = 0,
	SRCCAP_DV_META_TAG_MAX
};

struct srccap_dv_dqbuf_in;
struct srccap_dv_dqbuf_out;

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------
int mtk_dv_meta_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out);

int mtk_dv_meta_read(
	int fd,
	enum srccap_dv_meta_tag tag,
	void *content);

int mtk_dv_meta_write(
	int fd,
	enum srccap_dv_meta_tag tag,
	void *content);

int mtk_dv_meta_remove(
	int fd,
	enum srccap_dv_meta_tag tag);

#endif  // MTK_SRCCAP_DV_META_H
