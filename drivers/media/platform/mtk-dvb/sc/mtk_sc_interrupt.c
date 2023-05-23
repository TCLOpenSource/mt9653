// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>


#include "mtk_sc_interrupt.h"
#include "mtk_sc_core.h"
#include "mtk_sc_mdrv.h"
int sc_interrupt_irq;
static irqreturn_t sc_irq_handler(int irq, void *dev_id)
{
	//pr_info("[%s][%d]\n", __func__, __LINE__);

	if (!MDrv_SC_ISR_Proc(0))
		pr_info("ISR proc is failed\n");


	return IRQ_HANDLED;
}

int mdrv_sc_init(struct platform_device *pdev, struct mdrv_sc_int *pinterrupt)
{
	int ret = 0;

	pinterrupt->irq = platform_get_irq(pdev, 0);
	if (pinterrupt->irq < 0) {
		pr_info("[%s][%d] failed to get irq\n", __func__, __LINE__);
		return -ENOENT;
	}

	pr_info("sc irq = %x\n", pinterrupt->irq);
	sc_interrupt_irq = pinterrupt->irq;

	ret =
	    devm_request_irq(&pdev->dev, pinterrupt->irq, sc_irq_handler, 0, DRV_NAME, pinterrupt);
	if (ret < 0) {
		pr_info("[%s][%d] failed to request irq\n", __func__, __LINE__);
		return -ENODEV;
	}

	return 0;
}
