/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_VBI_H
#define MTK_SRCCAP_VBI_H

struct srccap_vbi_info {
	unsigned long VBICCvirAddress;
	u64 vbiBufAddr;
	u32 vbiBufSize;
	u32 vbiBufHeapId;
	u32 u32vbiAliment;
	u32 u32vbiSize;
	u8 *pu8RiuBaseAddr;
	u32 u32DramBaseAddr;
	u64 *pIoremap;
	u32 regCount;
	u8 reg_info_offset;
	u8 reg_pa_idx;
	u8 reg_pa_size_idx;
};

int mtk_srccap_register_vbi_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_vbi,
	struct v4l2_ctrl_handler *vbi_ctrl_handler);
void mtk_srccap_unregister_vbi_subdev(struct v4l2_subdev *subdev_vbi);
void mtk_vbi_SetBufferAddr(void *Kernelvaddr, struct srccap_vbi_info vbi_info);

#endif  // MTK_SRCCAP_VBI_H

