// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>

#include "mtk_sc_clock.h"

#include "mtk_sc_drv.h"
#include "mtk_sc_hal.h"

int mdrv_sc_clock_init(struct platform_device *pdev, struct mdrv_sc_clk *pclk)
{
	pclk->smart_ck = devm_clk_get(&pdev->dev, "smart_ck");
	if (IS_ERR(pclk->smart_ck)) {
		dev_info(&pdev->dev, "[%s][%d]failed to get smart_ck handle\n", __func__,
		       __LINE__);
		return -ENODEV;
	}
	pclk->smart_synth_432_ck = devm_clk_get(&pdev->dev, "smart_synth_432_ck");
	if (IS_ERR(pclk->smart_synth_432_ck)) {
		dev_info(&pdev->dev, "[%s][%d]failed to get smart_synth_432_ck handle\n", __func__,
		       __LINE__);
		return -ENODEV;
	}
	pclk->smart_synth_27_ck = devm_clk_get(&pdev->dev, "smart_synth_27_ck");
	if (IS_ERR(pclk->smart_synth_27_ck)) {
		dev_info(&pdev->dev, "[%s][%d]failed to get smart_synth_27_ck handle\n", __func__,
		       __LINE__);
		return -ENODEV;
	}
	pclk->smart2piu = devm_clk_get(&pdev->dev, "smart2piu");
	if (IS_ERR(pclk->smart2piu)) {
		dev_info(&pdev->dev, "[%s][%d]failed to get smart2piu handle\n", __func__,
		       __LINE__);
		return -ENODEV;
	}
	pclk->smart_synth_272piu = devm_clk_get(&pdev->dev, "smart_synth_272piu");
	if (IS_ERR(pclk->smart_synth_272piu)) {
		dev_info(&pdev->dev, "[%s][%d]failed to get smart_synth_272piu handle\n", __func__,
		       __LINE__);
		return -ENODEV;
	}
	pclk->smart_synth_4322piu = devm_clk_get(&pdev->dev, "smart_synth_4322piu");
	if (IS_ERR(pclk->smart_synth_4322piu)) {
		dev_info(&pdev->dev, "[%s][%d]failed to get smart_synth_4322piu handle\n", __func__,
		       __LINE__);
		return -ENODEV;
	}
	pclk->mcu_nonpm2smart0 = devm_clk_get(&pdev->dev, "mcu_nonpm2smart0");
	if (IS_ERR(pclk->mcu_nonpm2smart0)) {
		dev_info(&pdev->dev, "[%s][%d]failed to get mcu_nonpm2smart0 handle\n", __func__,
		       __LINE__);
		return -ENODEV;
	}

	hal_sc_set_clk_base(pclk);
	return 0;
}
