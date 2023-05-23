// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>

static const struct of_device_id bring_up_id_table[] = {
	{ .compatible = "mediatek,clk-bring-up",},
	{ },
};
MODULE_DEVICE_TABLE(of, bring_up_id_table);

static int bring_up_probe(struct platform_device *pdev)
{
	struct clk *clk;
	int clk_con, i;

	clk_con = of_count_phandle_with_args(pdev->dev.of_node, "clocks",
			"#clock-cells");

	for (i = 0; i < clk_con; i++) {
		clk = of_clk_get(pdev->dev.of_node, i);
		if (IS_ERR(clk)) {
			long ret = PTR_ERR(clk);

			if (ret == -EPROBE_DEFER)
				pr_notice("clk %d is not ready\n", i);
			else
				pr_notice("get clk %d fail, ret=%d, clk_con=%d\n",
				       i, (int)ret, clk_con);
		} else {
			pr_notice("get clk [%d]: %s ok\n", i,
					__clk_get_name(clk));
			clk_prepare_enable(clk);
		}
	}

	return 0;
}

static int bring_up_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver bring_up = {
	.probe		= bring_up_probe,
	.remove		= bring_up_remove,
	.driver		= {
		.name	= "bring_up",
		.owner	= THIS_MODULE,
		.of_match_table = bring_up_id_table,
	},
};

module_platform_driver(bring_up);
MODULE_LICENSE("GPL");
