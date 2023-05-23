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

#include "mtk_srccap.h"
#include "mtk_srccap_common_irq.h"
#include "hwreg_srccap_common_irq.h"

#define MTK_SRCCAP_COMPATIBLE_STR "mediatek,srccap0"

static enum srccap_hwreg_irq_type get_irq_type_reg(enum srccap_common_irq_type type)
{
	enum srccap_hwreg_irq_type irq_type = srccap_hwreg_irq_type_max;

	switch (type) {
	case SRCCAP_COMMON_IRQTYPE_HK:
		irq_type = srccap_hwreg_irq_type_hk;
		break;
	case SRCCAP_COMMON_IRQTYPE_FRC:
		irq_type = srccap_hwreg_irq_type_frc;
		break;
	case SRCCAP_COMMON_IRQTYPE_PQ:
		irq_type = srccap_hwreg_irq_type_pq;
		break;
	default:
		break;
	}

	return irq_type;
}

static enum srccap_hwreg_irq_event get_irq_event_reg(enum srccap_common_irq_event event)
{
	enum srccap_hwreg_irq_event irq_event = srccap_hwreg_irq_event_max;

	switch (event) {
	case SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC0:
		irq_event = srccap_hwreg_irq_event_sw_irq_trig_src0;
		break;
	case SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC1:
		irq_event = srccap_hwreg_irq_event_sw_irq_trig_src1;
		break;
	case SRCCAP_COMMON_IRQEVENT_DV_HW5_IP_INT0:
		irq_event = srccap_hwreg_irq_event_dv_hw5_ip_int0;
		break;
	case SRCCAP_COMMON_IRQEVENT_DV_HW5_IP_INT1:
		irq_event = srccap_hwreg_irq_event_dv_hw5_ip_int1;
		break;
	default:
		break;
	}

	return irq_event;
}

int mtk_common_attach_irq(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	SRCCAP_COMMON_INTCB intcb,
	void *param)
{
	int ret = 0;
	int int_num = 0;

	struct platform_device *pdev = NULL;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_ERROR("pdev or srccap_dev failed\n");
		return -EINVAL;
	}

	pdev = to_platform_device(srccap_dev->dev);

	if ((type >= SRCCAP_COMMON_IRQTYPE_MAX) || (type < SRCCAP_COMMON_IRQTYPE_HK)) {
		SRCCAP_MSG_ERROR("failed to attach irq: %d, (max %d)\n", type,
		SRCCAP_COMMON_IRQTYPE_MAX);
		return -EINVAL;
	}

	if ((event >= SRCCAP_COMMON_IRQEVENT_MAX) ||
			(event < SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC0)) {
		SRCCAP_MSG_ERROR("failed to attach irq: %d, (max %d)\n", type,
		SRCCAP_COMMON_IRQEVENT_MAX);
		return -EINVAL;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_irq_info);

	//Add irq to irq info table
	for (int_num = 0; int_num < SRCCAP_COMMON_IRQ_MAX_NUM; int_num++) {
		if (srccap_dev->common_info.irq_info.isr_info
			[int_num][type][event].param == NULL) {
			srccap_dev->common_info.irq_info.isr_info
				[int_num][type][event].param = param;
			srccap_dev->common_info.irq_info.isr_info
				[int_num][type][event].isr = intcb;

			ret = mtk_common_set_irq_mask(type, event, UNMASKIRQ,
				srccap_dev->srccap_info.cap.u32IRQ_Version);
			if (ret) {
				SRCCAP_MSG_ERROR("mtk_common_set_irq_mask(%d,%d,%d) fail",
					type, event, UNMASKIRQ);
				mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_irq_info);
				return -EPERM;
			}
			break;
		}
	}

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_irq_info);

	return ret;
}

int mtk_common_detach_irq(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	SRCCAP_COMMON_INTCB intcb,
	void *param)
{
	int ret = 0;
	int int_num = 0;

	struct platform_device *pdev = NULL;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_ERROR("pdev or srccap_dev failed\n");
		return -EINVAL;
	}

	pdev = to_platform_device(srccap_dev->dev);

	if ((type >= SRCCAP_COMMON_IRQTYPE_MAX) || (type < SRCCAP_COMMON_IRQTYPE_HK)) {
		SRCCAP_MSG_ERROR("failed to attach irq: %d, (max %d)\n", type,
			SRCCAP_COMMON_IRQTYPE_MAX);
		return -EINVAL;
	}

	if ((event >= SRCCAP_COMMON_IRQEVENT_MAX) ||
		(event < SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC0)) {
		SRCCAP_MSG_ERROR("failed to attach irq: %d, (max %d)\n", type,
			SRCCAP_COMMON_IRQEVENT_MAX);
		return -EINVAL;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_irq_info);

	//Clear irq info table
	for (int_num = 0; int_num < SRCCAP_COMMON_IRQ_MAX_NUM; int_num++) {
		if ((srccap_dev->common_info.irq_info.isr_info
			[int_num][type][event].param == param)
		&& (srccap_dev->common_info.irq_info.isr_info
			[int_num][type][event].isr == intcb)) {
			srccap_dev->common_info.irq_info.isr_info
				[int_num][type][event].param = NULL;
			srccap_dev->common_info.irq_info.isr_info
				[int_num][type][event].isr = NULL;
			break;
		}
	}
	//Check irq info table
	for (int_num = 0; int_num < SRCCAP_COMMON_IRQ_MAX_NUM; int_num++) {
		if ((srccap_dev->common_info.irq_info.isr_info
			[int_num][type][event].param != NULL)
		&& (srccap_dev->common_info.irq_info.isr_info
			[int_num][type][event].isr != NULL)) {
			break;
		}
	}
	if (int_num == SRCCAP_COMMON_IRQ_MAX_NUM)
		mtk_common_set_irq_mask(type, event, MASKIRQ,
			srccap_dev->srccap_info.cap.u32IRQ_Version);

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_irq_info);

	return ret;
}

int mtk_common_set_irq_mask(
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	bool mask,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[1];
	struct hwregSrcIRQMaskIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregSrcIRQMaskIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU      = 1;
	paramIn.irqType	 = get_irq_type_reg(type);
	paramIn.irqEvent = get_irq_event_reg(event);
	paramIn.mask     = mask;

	ret = drv_hwreg_srccap_common_irq_set_mask(paramIn, &paramOut, u32IRQ_Version);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to set mask = %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

int mtk_common_force_irq(
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	bool force,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[1];
	struct hwregSrcIRQForceIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregSrcIRQForceIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU      = 1;
	paramIn.irqType	 = get_irq_type_reg(type);
	paramIn.irqEvent = get_irq_event_reg(event);
	paramIn.force    = force;

	ret = drv_hwreg_srccap_common_irq_set_force(paramIn, &paramOut, u32IRQ_Version);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to force irq = %d\n", ret);
		return -EINVAL;
	}
	return ret;
}


int mtk_common_clear_irq(
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[1];
	struct hwregSrcIRQClrIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregSrcIRQClrIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU      = 1;
	paramIn.irqType	 = get_irq_type_reg(type);
	paramIn.irqEvent = get_irq_event_reg(event);

	ret = drv_hwreg_srccap_common_irq_set_clr(paramIn, &paramOut, u32IRQ_Version);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to clear irq = %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

int mtk_common_get_irq_status(
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	bool *status,
	uint32_t u32IRQ_Version)
{
	int ret = 0;
	struct reg_info reg[1];
	struct hwregSrcIRQStatusIn paramIn;
	struct hwregOut paramOut;

	if (status == NULL) {
		SRCCAP_MSG_ERROR("IRQstatus NULL\n");
		return -EINVAL;
	}

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregSrcIRQStatusIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU      = 1;
	paramIn.irqType	 = get_irq_type_reg(type);
	paramIn.irqEvent = get_irq_event_reg(event);

	ret = drv_hwreg_srccap_common_irq_get_status(paramIn, &paramOut, u32IRQ_Version);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to get irq status = %d\n", ret);
		return -EINVAL;
	}

	*status = paramOut.reg[0].val;

	return ret;
}

static int handle_irq_event(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event)
{
	int ret = 0;
	bool status = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_ERROR("srccap_dev failed\n");
		return -EINVAL;
	}

	ret = mtk_common_get_irq_status(
			type,
			event,
			&status,
			srccap_dev->srccap_info.cap.u32IRQ_Version);

	if (ret) {
		SRCCAP_MSG_ERROR("failed to get status = %d\n", ret);
		return -EINVAL;
	}

	if (status == true) {
		ret = mtk_common_clear_irq(
				type,
				event,
				srccap_dev->srccap_info.cap.u32IRQ_Version);
		if (ret) {
			SRCCAP_MSG_ERROR("failed to set clear = %d\n", ret);
			return -EINVAL;
		}

		srccap_dev->common_info.srccap_handled_top_irq[event][type] = 1;
	}

	return 0;
}

static irqreturn_t irq_handler_top_half(
	int irq,
	void *data)
{
	int ret = 0;
	int int_num = 0, irq_type = 0, event = 0;
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	irqreturn_t top_ret = IRQ_NONE;
	bool bottom_trigger = FALSE;

	if ((data == NULL) || (srccap_dev == NULL))
		return IRQ_NONE;

	//SRCCAP_MSG_DEBUG("%s(%d,%u)\n", __func__, irq, srccap_dev->dev_id);

	for (irq_type = 0; irq_type < SRCCAP_COMMON_IRQTYPE_MAX; irq_type++) {
		for (event = 0; event < SRCCAP_COMMON_IRQEVENT_MAX; event++) {
			// Check all functions attached to this event
			for (int_num = 0; int_num < SRCCAP_COMMON_IRQ_MAX_NUM; int_num++) {
				if (srccap_dev->common_info.irq_info.isr_info
					[int_num][irq_type][event].isr != NULL) {
					ret = handle_irq_event(
						srccap_dev,
						(enum srccap_common_irq_type) irq_type,
						(enum srccap_common_irq_event) event);
					if (ret < 0) {
						SRCCAP_MSG_ERROR(
							"failed to get irq event %x (num=%x, type=%x, event=%x)\n",
							ret, int_num, irq_type, event);
						return IRQ_NONE;
					}

					break;
				}
			}

			if (srccap_dev->common_info.srccap_handled_top_irq[event][irq_type] == 1)
				bottom_trigger = TRUE;
		}
	}

	if (bottom_trigger)
		top_ret = IRQ_WAKE_THREAD;
	else
		top_ret = IRQ_NONE;

	return top_ret;
}

static irqreturn_t irq_handler_bottom_half(
	int irq,
	void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	int int_num = 0, irq_type = 0, event = 0;

	if ((data == NULL) || (srccap_dev == NULL))
		return IRQ_HANDLED;

	//SRCCAP_MSG_DEBUG("%s(%d,%u)\n", __func__, irq, srccap_dev->dev_id);

	for (irq_type = 0; irq_type < SRCCAP_COMMON_IRQTYPE_MAX; irq_type++) {
		for (event = 0; event < SRCCAP_COMMON_IRQEVENT_MAX; event++) {
			if (srccap_dev->common_info.srccap_handled_top_irq
				[event][irq_type] == 0)
				continue;

			for (int_num = 0; int_num < SRCCAP_COMMON_IRQ_MAX_NUM;
				int_num++) {
				SRCCAP_COMMON_INTCB isr =
					srccap_dev->common_info.irq_info.isr_info
					[int_num][irq_type][event].isr;
				void *param =
					srccap_dev->common_info.irq_info.isr_info
					[int_num][irq_type][event].param;
				if (isr != NULL)
					isr(param);
			}

			srccap_dev->common_info.srccap_handled_top_irq[event][irq_type] = 0;
		}
	}

	return IRQ_HANDLED;
}

int mtk_common_init_irq(
	struct mtk_srccap_dev *srccap_dev)
{
	int irq = 0;
	int ret = 0;
	struct device_node *np;
	struct platform_device *pdev;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	pdev = to_platform_device(srccap_dev->dev);
	if (pdev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	np = of_find_compatible_node(NULL, NULL, MTK_SRCCAP_COMPATIBLE_STR);
	if (np != NULL) {
		irq = of_irq_get(np, SRCCAP_XC_IRQ);

		if (irq < 0) {
			SRCCAP_MSG_ERROR("of_irq_get failed\n");
			return -EINVAL;
		}
	}

	ret = request_threaded_irq(
		irq,
		irq_handler_top_half,
		irq_handler_bottom_half,
		IRQF_SHARED | IRQF_ONESHOT,
		pdev->name,
		srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to install IRQ\n");
		return -EINVAL;
	}

	return ret;
}

int mtk_srccap_common_irq_suspend(void)
{
	int irq = 0;
	int ret = 0;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, MTK_SRCCAP_COMPATIBLE_STR);

	if (np != NULL) {
		irq = of_irq_get(np, SRCCAP_XC_IRQ);

		if (irq < 0) {
			SRCCAP_MSG_ERROR("of_irq_get failed\n");
			return -EINVAL;
		}
	}

	disable_irq(irq);

	return ret;
}
EXPORT_SYMBOL(mtk_srccap_common_irq_suspend);

int mtk_srccap_common_irq_resume(void)
{
	int irq = 0;
	int ret = 0;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, MTK_SRCCAP_COMPATIBLE_STR);

	if (np != NULL) {
		irq = of_irq_get(np, SRCCAP_XC_IRQ);

		if (irq < 0) {
			SRCCAP_MSG_ERROR("of_irq_get failed\n");
			return -EINVAL;
		}
	}

	enable_irq(irq);

	return ret;
}
EXPORT_SYMBOL(mtk_srccap_common_irq_resume);
