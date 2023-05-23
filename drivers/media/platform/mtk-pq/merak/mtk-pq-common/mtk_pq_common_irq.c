// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#include "mtk_pq.h"
#include "mtk_pq_common_irq.h"
#include "hwreg_pq_common_irq.h"

#define MASKIRQ		(true)
#define UNMASKIRQ	(false)

#define MTK_PQ_COMPATIBLE_STR "mediatek,pq"

enum pq_hwreg_irq_type _mtk_pq_common_irq_get_irqType(enum PQIRQTYPE irqType)
{
	enum pq_hwreg_irq_type irq_type = pq_hwreg_irq_type_max;

	switch (irqType) {
	case PQ_IRQTYPE_HK:
		irq_type = pq_hwreg_irq_type_hk;
		break;
	case PQ_IRQTYPE_FRC:
		irq_type = pq_hwreg_irq_type_frc;
		break;
	case PQ_IRQTYPE_PQ:
		irq_type = pq_hwreg_irq_type_pq;
		break;
	default:
		break;
	}

	return irq_type;
}

enum pq_hwreg_irq_event _mtk_pq_common_irq_get_irqEvent(enum PQIRQEVENT irqEvent)
{
	enum pq_hwreg_irq_event irq_event = pq_hwreg_irq_event_max;

	switch (irqEvent) {
	case PQ_IRQEVENT_SW_IRQ_TRIG_IP:
		irq_event = pq_hwreg_irq_event_sw_irq_trig_ip;
		break;
	case PQ_IRQEVENT_SW_IRQ_TRIG_OP2:
		irq_event = pq_hwreg_irq_event_sw_irq_trig_op2;
		break;
	default:
		break;
	}

	return irq_event;
}

int _mtk_pq_common_irq_set_mask(
	enum PQIRQTYPE irqType,
	enum PQIRQEVENT irqEvent,
	bool mask,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[1];
	struct hwregPQIRQMaskIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregPQIRQMaskIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU      = 1;
	paramIn.irqType	 = _mtk_pq_common_irq_get_irqType(irqType);
	paramIn.irqEvent = _mtk_pq_common_irq_get_irqEvent(irqEvent);
	paramIn.mask     = mask;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "RIU:%d irqType:%d irqEvent:%d mask:%d\n",
		paramIn.RIU, paramIn.irqType, paramIn.irqEvent, paramIn.mask);

	ret = drv_hwreg_pq_common_irq_set_mask(paramIn, &paramOut, u32IRQ_Version);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "set mask fail\n");
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "regCount:%d\n",
		paramOut.regCount);

	return ret;
}

int _mtk_pq_common_irq_set_clr(
	enum PQIRQTYPE irqType,
	enum PQIRQEVENT irqEvent,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[1];
	struct hwregPQIRQClrIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregPQIRQClrIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU      = 1;
	paramIn.irqType	 = _mtk_pq_common_irq_get_irqType(irqType);
	paramIn.irqEvent = _mtk_pq_common_irq_get_irqEvent(irqEvent);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "RIU:%d irqType:%d irqEvent:%d\n",
		paramIn.RIU, paramIn.irqType, paramIn.irqEvent);

	ret = drv_hwreg_pq_common_irq_set_clr(paramIn, &paramOut, u32IRQ_Version);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "set clear fail\n");
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "regCount:%d\n",
		paramOut.regCount);

	return ret;
}

int _mtk_pq_common_irq_get_status(
	enum PQIRQTYPE irqType,
	enum PQIRQEVENT irqEvent,
	bool *IRQstatus,
	u32 u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[1];
	struct hwregPQIRQStatusIn paramIn;
	struct hwregOut paramOut;

	if (IRQstatus == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid input, IRQstatus=NULL\n");
		return -EINVAL;
	}

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregPQIRQStatusIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU      = 1;
	paramIn.irqType	 = _mtk_pq_common_irq_get_irqType(irqType);
	paramIn.irqEvent = _mtk_pq_common_irq_get_irqEvent(irqEvent);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "RIU:%d irqType:%d irqEvent:%d\n",
		paramIn.RIU, paramIn.irqType, paramIn.irqEvent);


	ret = drv_hwreg_pq_common_irq_get_status(paramIn, &paramOut, u32IRQ_Version);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get status fail\n");
		return -EINVAL;
	}

	*IRQstatus = paramOut.reg[0].val;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "regCount:%d\n",
		paramOut.regCount);

	return ret;
}

static int _mtk_pq_common_irq_event(
	struct mtk_pq_platform_device *pqdev,
	enum PQIRQTYPE irqType,
	enum PQIRQEVENT irqEvent)
{
	int ret = 0;
	bool IRQstatus = 0;

	if (pqdev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid input, pqdev=NULL\n");
		return -EINVAL;
	}


	ret = _mtk_pq_common_irq_get_status(
				irqType,
				irqEvent,
				&IRQstatus,
				pqdev->pqcaps.u32IRQ_Version);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get status fail\n");
		return -EINVAL;
	}

	if (IRQstatus == true) {
		ret = _mtk_pq_common_irq_set_clr(
					irqType,
					irqEvent,
					pqdev->pqcaps.u32IRQ_Version);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "set clear fail\n");
			return -EINVAL;
		}

		pqdev->pq_irq[irqEvent] = 1;
	}

	return 0;
}

static irqreturn_t _mtk_pq_common_irq_top_handler(
	int irq,
	void *dev_id)
{
	int ret = 0;
	struct mtk_pq_platform_device *pqdev = (struct mtk_pq_platform_device *)dev_id;

	if ((dev_id == NULL) || (pqdev == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dev_id NULL\n");
		goto NONE;
	}

	ret = _mtk_pq_common_irq_event(pqdev, PQ_IRQTYPE_HK, PQ_IRQEVENT_SW_IRQ_TRIG_IP);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "check event fail\n");
		goto NONE;
	}

	if (pqdev->pq_irq[PQ_IRQEVENT_SW_IRQ_TRIG_IP])
		return IRQ_WAKE_THREAD;

	ret = _mtk_pq_common_irq_event(pqdev, PQ_IRQTYPE_HK, PQ_IRQEVENT_SW_IRQ_TRIG_OP2);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "check event fail\n");
		goto NONE;
	}

	if (pqdev->pq_irq[PQ_IRQEVENT_SW_IRQ_TRIG_OP2])
		return IRQ_WAKE_THREAD;

NONE:
	return IRQ_NONE;
}

static irqreturn_t _mtk_pq_common_irq_bottom_handler(
	int irq,
	void *dev_id)
{
	struct mtk_pq_platform_device *pqdev = (struct mtk_pq_platform_device *)dev_id;

	if ((dev_id == NULL) || (pqdev == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dev_id NULL\n");

		return IRQ_HANDLED;
	}

	if (pqdev->pq_irq[PQ_IRQEVENT_SW_IRQ_TRIG_IP] == 1) {
		//do something
		pqdev->pq_irq[PQ_IRQEVENT_SW_IRQ_TRIG_IP] = 0;
	}

	if (pqdev->pq_irq[PQ_IRQEVENT_SW_IRQ_TRIG_OP2] == 1) {
		//do something
		pqdev->pq_irq[PQ_IRQEVENT_SW_IRQ_TRIG_OP2] = 0;
	}

	return IRQ_HANDLED;
}

int mtk_pq_common_irq_init(
	struct mtk_pq_platform_device *pqdev)
{
	int irq = 0;
	int ret = 0;
	struct device_node *np;
	struct platform_device *pdev = to_platform_device(pqdev->dev);

	if ((pqdev == NULL) || (pdev == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pqdev NULL\n");
		return IRQ_HANDLED;
	}

	np = of_find_compatible_node(NULL, NULL, MTK_PQ_COMPATIBLE_STR);

	if (np != NULL) {
		irq = of_irq_get(np, 1);
		if (irq < 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "of_irq_get failed\n");
			return -EINVAL;
		}
	}

	ret = request_threaded_irq(
		irq,
		_mtk_pq_common_irq_top_handler,
		_mtk_pq_common_irq_bottom_handler,
		IRQF_SHARED | IRQF_ONESHOT,
		pdev->name,
		pqdev);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "failed to install IRQ\n");
		return -EINVAL;
	}

	return ret;
}

int mtk_pq_common_irq_suspend(bool stub)
{
	int irq = 0;
	int ret = 0;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, MTK_PQ_COMPATIBLE_STR);

	if (np != NULL) {
		irq = of_irq_get(np, 1);
		if (irq < 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "of_irq_get failed\n");
			return -EINVAL;
		}
	}

	if (!stub) {
		//Rendor and other module  is using irq when stub mode
		disable_irq(irq);
	}

	return ret;
}
EXPORT_SYMBOL(mtk_pq_common_irq_suspend);

int mtk_pq_common_irq_resume(bool stub, uint32_t u32IRQ_Version)
{
	int irq = 0;
	int ret = 0;
	struct device_node *np;
	struct reg_info reg[40];
	struct hwregOut paramOut;

	// Set pq irq patch [remove at ES2]
	memset(reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	ret = drv_hwreg_pq_common_irq_set_patch(&paramOut, u32IRQ_Version);
	if (ret) {
		pr_err("[PQ_IRQ][%s,%d] fail\n", __func__, __LINE__);
		return ret;
	}

	np = of_find_compatible_node(NULL, NULL, MTK_PQ_COMPATIBLE_STR);

	if (np != NULL) {
		irq = of_irq_get(np, 1);
		if (irq < 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_IRQ, "of_irq_get failed\n");
			return -EINVAL;
		}
	}

	if (!stub) {
		//Rendor and other module  is using irq when stub mode
		enable_irq(irq);
	}
	return ret;
}
EXPORT_SYMBOL(mtk_pq_common_irq_resume);
