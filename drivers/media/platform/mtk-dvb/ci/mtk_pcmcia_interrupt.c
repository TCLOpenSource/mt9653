// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include "mtk_pcmcia_hal_diff.h"

#include "mtk_pcmcia_interrupt.h"
#include "mtk_pcmcia_drv.h"
#include "mtk_pcmcia_core.h"

#define MLP_CI_CORE_ENTRY(fmt, arg...)	pr_info("[mdbgin_CADD_CI]" fmt, ##arg)

int ci_interrupt_irq;
#if (!PCMCIA_MSPI_SUPPORTED)
static irqreturn_t ci_irq_handler(int irq, void *dev_id)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	pr_info("[%s][%d]\n", __func__, __LINE__);
	mdrv_pcmcia_isr();

	return IRQ_HANDLED;
}

int mdrv_ci_init(struct platform_device *pdev, struct mdrv_ci_int *pinterrupt)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	int ret = 0;

	pinterrupt->irq = platform_get_irq(pdev, 0);
	if (pinterrupt->irq  < 0) {
		pr_info("[%s][%d] failed to get irq\n", __func__, __LINE__);
		return -ENOENT;
	}

	pr_info("ci irq = %x\n", pinterrupt->irq);
	ci_interrupt_irq = pinterrupt->irq;

	ret =
	    devm_request_irq(&pdev->dev, pinterrupt->irq, ci_irq_handler, 0,
			     DRV_NAME, pinterrupt);
	if (ret < 0) {
		pr_info("[%s][%d] failed to request irq\n", __func__,
		       __LINE__);
		return -ENODEV;
	}

	return 0;
}
#endif
