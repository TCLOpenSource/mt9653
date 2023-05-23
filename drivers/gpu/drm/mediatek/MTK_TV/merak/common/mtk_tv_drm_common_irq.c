// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/of.h>

#include "mtk_tv_drm_common_irq.h"
#include "hwreg_render_common_irq.h"
#include "../mtk_tv_drm_log.h"

#define MTK_DRM_COMPATIBLE_STR "MTK-drm-tv-kms"

#define MTK_DRM_MODEL MTK_DRM_MODEL_COMMON
#define IRQ_REG_NUM 2

static enum drm_hwreg_irq_type get_irq_type_reg(enum IRQType type)
{
	enum drm_hwreg_irq_type irq_type = VIDEO_IRQTYPE_MAX;

	switch (type) {
	case MTK_VIDEO_IRQTYPE_HK:
		irq_type = VIDEO_IRQTYPE_HK;
		break;
	case MTK_VIDEO_IRQTYPE_FRC:
		irq_type = VIDEO_IRQTYPE_FRC;
		break;
	case MTK_VIDEO_IRQTYPE_PQ:
		irq_type = VIDEO_IRQTYPE_PQ;
		break;
	default:
		break;
	}

	return irq_type;
}

static enum drm_hwreg_irq_event get_irq_event_reg(enum IRQEVENT event)
{
	enum drm_hwreg_irq_event irq_event = VIDEO_IRQEVENT_MAX;

	switch (event) {
	case MTK_VIDEO_SW_IRQ_TRIG_DISP:
		irq_event = VIDEO_IRQEVENT_SW_IRQ_TRIG_DISP;
		break;
	case MTK_VIDEO_PQ_IRQ_TRIG_DISP:
		irq_event = VIDEO_IRQEVENT_PQ_IRQ_TRIG_DISP;
		break;
	default:
		break;
	}

	return irq_event;
}

int mtk_common_IRQ_set_mask(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[2];
	struct hwregIRQmaskIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregIRQmaskIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;
	paramIn.RIU = 1;
	paramIn.irqType = get_irq_type_reg(irqType);
	paramIn.irqEvent = get_irq_event_reg(irqEvent);
	paramIn.mask = 1;

	ret = drv_hwreg_render_common_irq_set_mask(paramIn, &paramOut, u32IRQ_Version);

	return ret;
}

int mtk_common_IRQ_set_unmask(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[2];
	struct hwregIRQmaskIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregIRQmaskIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;
	paramIn.RIU = 1;
	paramIn.irqType = get_irq_type_reg(irqType);
	paramIn.irqEvent = get_irq_event_reg(irqEvent);
	paramIn.mask = 0;

	ret = drv_hwreg_render_common_irq_set_mask(paramIn, &paramOut, u32IRQ_Version);

	return ret;
}

int mtk_common_IRQ_set_clr(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[2];
	struct hwregIRQClrIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregIRQClrIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;
	paramIn.RIU = 1;
	paramIn.irqType = get_irq_type_reg(irqType);
	paramIn.irqEvent = get_irq_event_reg(irqEvent);

	ret = drv_hwreg_render_common_irq_set_clr(paramIn, &paramOut, u32IRQ_Version);

	return ret;
}

int mtk_common_IRQ_get_status(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	bool *IRQstatus,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[2];
	struct hwregIRQstatusIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregIRQstatusIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;
	paramIn.RIU = 1;
	paramIn.irqType = get_irq_type_reg(irqType);
	paramIn.irqEvent = get_irq_event_reg(irqEvent);

	ret = drv_hwreg_render_common_irq_get_status(paramIn, &paramOut, u32IRQ_Version);
	*IRQstatus = paramOut.reg[0].val;
	return ret;
}

int mtk_common_IRQ_get_mask(
	struct mtk_tv_drm_crtc *crtc,
	enum IRQType irqType,
	enum IRQEVENT irqEvent,
	bool *IRQmask,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[IRQ_REG_NUM];
	struct hwregIRQmaskIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregIRQmaskIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;
	paramIn.RIU = 1;
	paramIn.irqType = get_irq_type_reg(irqType);
	paramIn.irqEvent = get_irq_event_reg(irqEvent);

	ret = drv_hwreg_render_common_irq_get_mask(paramIn, &paramOut, u32IRQ_Version);
	*IRQmask = paramOut.reg[0].val;
	return ret;
}

int mtk_common_irq_suspend(void)
{
	int irq_video = 0;
	int irq_graphic = 0;
	int ret = 0;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_COMPATIBLE_STR);
	if (np != NULL) {
		irq_video = of_irq_get(np, 0);
		if (irq_video < 0) {
			DRM_INFO("of_irq_get failed\n");
			return -EINVAL;
		}
		irq_graphic = of_irq_get(np, 1);
		if (irq_graphic < 0) {
			DRM_INFO("of_irq_get failed\n");
			return -EINVAL;
	}
	}
	disable_irq(irq_video);
	disable_irq(irq_graphic);
	return ret;
}
EXPORT_SYMBOL(mtk_common_irq_suspend);

int mtk_common_irq_resume(void)
{
	int irq_video = 0;
	int irq_graphic = 0;
	int ret = 0;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_COMPATIBLE_STR);
	if (np != NULL) {
		irq_video = of_irq_get(np, 0);
		if (irq_video < 0) {
			DRM_INFO("of_irq_get failed\n");
			return -EINVAL;
		}
		irq_graphic = of_irq_get(np, 1);
		if (irq_graphic < 0) {
			DRM_INFO("of_irq_get failed\n");
			return -EINVAL;
		}
	}
	enable_irq(irq_video);
	enable_irq(irq_graphic);
	return ret;
}
EXPORT_SYMBOL(mtk_common_irq_resume);

