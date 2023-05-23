// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>

#include "mtk_sc_resource.h"
#include "mtk_sc_drv.h"
#include "mtk_sc_hal.h"

static int mdrv_sc_regmaps_init(struct platform_device *pdev, struct mdrv_sc_regmaps *pregmaps)
{
	struct resource *res;
	void __iomem *clk_riu_vaddr;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);	//RIU 1029
	if (!res) {
		pr_info("[%s][%d] failed to get ci address\n", __func__, __LINE__);
		return -ENOENT;
	}

	pr_info("resource[0]: start = %llx, end = %llx\n", res->start, res->end);

	pregmaps->riu_vaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pregmaps->riu_vaddr)) {
		pr_info("[%s][%d] failed to ioremap RIU_vaddr\n", __func__, __LINE__);
		return PTR_ERR(pregmaps->riu_vaddr);
	}
	pr_info("resource[0]pregmaps->RIU_vaddr= %llx\n", pregmaps->riu_vaddr);

	return 0;
}

int mdrv_sc_res_init(struct platform_device *pdev, struct mdrv_sc_res *pres)
{
	int ret = 0;

	pres->regmaps = devm_kzalloc(&pdev->dev, sizeof(struct mdrv_sc_regmaps), GFP_KERNEL);
	if (!pres->regmaps)
		return -ENOMEM;

	ret = mdrv_sc_regmaps_init(pdev, pres->regmaps);
	if (ret < 0)
		return ret;


	return 0;
}

int mdrv_sc_vcc_init(struct platform_device *pdev, int vcc_high_active)
{
	hal_sc_set_vcc_init(vcc_high_active);

	return 0;
}
