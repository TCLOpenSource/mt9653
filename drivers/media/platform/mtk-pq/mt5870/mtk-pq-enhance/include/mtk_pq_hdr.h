/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_HDR_H
#define _MTK_PQ_HDR_H

#include "mtk_pq_cfd.h"

//INFO OF HDR SUB DEV
struct st_hdr_info {
	struct st_cfd_info stCFDInfo;
	//struct st_tch_info stTCHInfo;
	//struct st_dv_info stDVInfo;
};

enum hdr_event_type {
	HDR_EVENT_TYPE_SEAMLESS_MUTE,
	HDR_EVENT_TYPE_SEAMLESS_UNMUTE,
	HDR_EVENT_TYPE_MAX,
};


// function
// set ctrl
int mtk_hdr_SetHDRMode(struct st_hdr_info *hdr_info, __s32 mode);
int mtk_hdr_SetDolbyViewMode(struct st_hdr_info *hdr_info,
	__s32 mode);
int mtk_hdr_SetTmoLevel(struct st_hdr_info *hdr_info,
	__s32 level);
// get ctrl
int mtk_hdr_GetHDRType(struct st_hdr_info *hdr_info,
	__s32 *hdr_type);
// set ext ctrl
int mtk_hdr_SetDolby3DLut(struct st_hdr_info *hdr_info,
	void *ctrl);
int mtk_hdr_SetCUSColorRange(struct st_hdr_info *hdr_info,
	void *ctrl);
int mtk_hdr_SetCustomerSetting(struct st_hdr_info *hdr_info,
	void *ctrl);
int mtk_hdr_SetHDRAttr(struct st_hdr_info *hdr_info,
	void *ctrl);
int mtk_hdr_SetSeamlessStatus(struct st_hdr_info *hdr_info,
	void *ctrl);
int mtk_hdr_SetCusColorMetry(struct st_hdr_info *hdr_info,
	void *ctrl);
int mtk_hdr_SetDolbyGDInfo(struct st_hdr_info *hdr_info,
	void *ctrl);
// get ext ctrl
int mtk_hdr_GetSeamlessStatus(struct st_hdr_info *hdr_info,
	void *ctrl);

int mtk_hdr_Open(struct st_hdr_info *hdr_info);
int mtk_hdr_Close(struct st_hdr_info *hdr_info);
int mtk_hdr_SetInputSource(struct st_hdr_info *hdr_info,
	__u8 u8Input);
int mtk_hdr_SetPixFmtIn(struct st_hdr_info *hdr_info,
	struct v4l2_pix_format_mplane *pix);
int mtk_hdr_SetPixFmtOut(struct st_hdr_info *hdr_info,
	struct v4l2_pix_format_mplane *pix);
int mtk_hdr_StreamOn(struct st_hdr_info *hdr_info);
int mtk_hdr_StreamOff(struct st_hdr_info *hdr_info);
int mtk_hdr_SubscribeEvent(struct st_hdr_info *hdr_info,
	__u32 event_type);
int mtk_hdr_UnsubscribeEvent(struct st_hdr_info *hdr_info,
	__u32 event_type);

#endif
