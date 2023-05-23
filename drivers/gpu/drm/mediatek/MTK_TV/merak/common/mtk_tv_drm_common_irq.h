/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_COMMON_IRQ_H_
#define _MTK_TV_DRM_COMMON_IRQ_H_
//------------------------------------------------------------------------------
//  Enum
//------------------------------------------------------------------------------
enum IRQType {
	MTK_VIDEO_IRQTYPE_HK = 0,
	MTK_VIDEO_IRQTYPE_FRC,
	MTK_VIDEO_IRQTYPE_PQ,
	MTK_VIDEO_IRQTYPE_MAX,
};
enum IRQEVENT {
	MTK_VIDEO_SW_IRQ_TRIG_DISP = 0,
	MTK_VIDEO_PQ_IRQ_TRIG_DISP,
	MTK_VIDEO_IRQEVENT_MAX,
};
//------------------------------------------------------------------------------
//  Structures
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Global Functions
//------------------------------------------------------------------------------
struct mtk_tv_drm_crtc;
int mtk_common_IRQ_set_mask(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	uint32_t u32IRQ_Version);
int mtk_common_IRQ_set_unmask(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	uint32_t u32IRQ_Version);
int mtk_common_IRQ_set_clr(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	uint32_t u32IRQ_Version);
int mtk_common_IRQ_get_status(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	bool *IRQstatus,
	uint32_t u32IRQ_Version);
int mtk_common_IRQ_get_mask(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	bool *IRQmask,
	uint32_t u32IRQ_Version);

int mtk_common_irq_suspend(void);
int mtk_common_irq_resume(void);

#endif

