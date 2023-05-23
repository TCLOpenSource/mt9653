// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include "mtk_pcmcia_hal_diff.h"

#include "mtk_pcmcia_resource.h"

#define MLP_CI_CORE_ENTRY(fmt, arg...)	pr_info("[mdbgin_CADD_CI]" fmt, ##arg)
#if (!PCMCIA_MSPI_SUPPORTED)
static int mdrv_ci_regmaps_init(struct platform_device *pdev,
				struct mdrv_ci_regmaps *pregmaps)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);	//RIU 1034-40
	if (!res) {
		pr_info("[%s][%d] failed to get ci address\n", __func__,
		       __LINE__);
		return -ENOENT;
	}

	pr_info("resource[0]: start = %llx, end = %llx\n", res->start, res->end);

	pregmaps->riu_vaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pregmaps->riu_vaddr)) {
		pr_info("[%s][%d] failed to ioremap RIU_vaddr\n", __func__,
		       __LINE__);
		return PTR_ERR(pregmaps->riu_vaddr);
	}
	pr_info("resource[0]pregmaps->RIU_vaddr= %llx\n", pregmaps->riu_vaddr);

	//pregmaps->ext_riu_vaddr = 0xffffff800c600000;
	res->start = 0x1f600000; //ext-RIU 3000

	pregmaps->ext_riu_vaddr = ioremap(res->start, 0x200*0x230);
	if (IS_ERR(pregmaps->ext_riu_vaddr))
		return PTR_ERR(pregmaps->ext_riu_vaddr);

	pr_info("resource[1]pregmaps->extRIU_vaddr= %llx\n",
	       pregmaps->ext_riu_vaddr);
	return 0;
}

int mdrv_ci_res_init(struct platform_device *pdev, struct mdrv_ci_res *pres)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	int ret = 0;

	pres->regmaps =
	    devm_kzalloc(&pdev->dev, sizeof(struct mdrv_ci_regmaps),
			 GFP_KERNEL);
	if (!pres->regmaps) {
		pr_info("[%s][%d] allocate regmaps struct fail\n", __func__,
		       __LINE__);
		return -ENOMEM;
	}
	ret = mdrv_ci_regmaps_init(pdev, pres->regmaps);
	if (ret < 0) {
		pr_info("[%s][%d] init regmaps struct fail\n", __func__,
		       __LINE__);
		return ret;
	}

	return 0;
}
#endif
