// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include "mtk_pcmcia_hal_diff.h"

#include "mtk_pcmcia_clock.h"

#define MLP_CI_CORE_ENTRY(fmt, arg...)	pr_info("[mdbgin_CADD_CI_CLOCK]" fmt, ##arg)

struct clk *ci_clk;
#if (!PCMCIA_MSPI_SUPPORTED)
int mdrv_ci_clock_init(struct platform_device *pdev, struct mdrv_ci_clk *pclk)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	pclk->clk_pcm = devm_clk_get(&pdev->dev, "clk_pcm");
	if (IS_ERR(pclk->clk_pcm)) {
		pr_info("[%s][%d] failed to get clk_tsp handle\n", __func__,
		       __LINE__);
		return -ENODEV;
	}
	ci_clk = pclk->clk_pcm;
	return 0;
}
#endif
