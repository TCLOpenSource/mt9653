// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h>
#include <linux/iopoll.h>

#include "apusys_power_user.h"

/* flag use to decide whether emulate apusys power on flow */
static int emulate_pwr;	/* 0: bypass pwer initial flow */
emulate_pwr = 0;	/* 0: bypass pwer initial flow */
module_param(emulate_pwr, int, 0444);
MODULE_PARM_DESC(emulate_pwr, "emulate apusys power on sequence");


int sim_power_on(struct device *dev, void __iomem *rpc)
{
	int err = 0;
	u32 tmp = 0;

	/* starting power on simulator */
	writel_relaxed(0x12, (rpc + RPC_FIFO_WE));
	writel_relaxed(0x13, (rpc + RPC_FIFO_WE));
	writel_relaxed(0x14, (rpc + RPC_FIFO_WE));
	writel_relaxed(0x15, (rpc + RPC_FIFO_WE));
	writel_relaxed(0x16, (rpc + RPC_FIFO_WE));

	/* check wither power on ready with max timeout 1000us */
	err = readl_poll_timeout_atomic((rpc + RPC_PWR_RDY), tmp,
					(tmp & 0x4) == 0x4, 10, 1000);
	if (err) {
		dev_err(dev, "%s RPC_PWR_RDY = 0x%x (expect 0x%x)\n",
				__func__, (u32)(tmp & 0x4), 0x4);
		goto out;
	}
	err = readl_poll_timeout_atomic((rpc + RPC_PWR_RDY), tmp,
					(tmp & 0x8) == 0x8, 10, 1000);
	if (err) {
		dev_err(dev, "%s RPC_PWR_RDY = 0x%x (expect 0x%x)\n",
				__func__, (u32)(tmp & 0x8), 0x8);
		goto out;
	}

	err = readl_poll_timeout_atomic((rpc + RPC_PWR_RDY), tmp,
					(tmp & 0x10) == 0x10, 10, 1000);
	if (err) {
		dev_err(dev, "%s 0x%x = 0x%x (expect 0x%x)\n",
				__func__, (u32)(tmp & 0x10), 0x10);
		goto out;
	}
	err = readl_poll_timeout_atomic((rpc + RPC_PWR_RDY), tmp,
					(tmp & 0x40) == 0x40, 10, 1000);
	if (err) {
		dev_err(dev, "%s 0x%x = 0x%x (expect 0x%x)\n",
				__func__, (u32)(tmp & 0x40), 0x40);
		goto out;
	}

out:
	return err;
}
static int apu_power_probe(struct platform_device *pdev)
{
	int err = 0;
	struct resource *rpc_res = NULL;
	struct resource *mdla_res = NULL;
	struct resource *vpu_res = NULL;

	void __iomem *va_rpc = NULL;
	void __iomem *va_mdla = NULL;
	void __iomem *va_vpu = NULL;
	struct device *dev = &pdev->dev;

	/* bypass pwr on emulation */
	if (!emulate_pwr)
		goto out;

	rpc_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					       "apusys_rpc");
	mdla_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"apusys_mdla");
	vpu_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					       "apusys_vpu");

	va_rpc = devm_ioremap_resource(dev, rpc_res);
	va_mdla = devm_ioremap_resource(dev, mdla_res);
	va_vpu = devm_ioremap_resource(dev, vpu_res);
	if (!va_rpc || !va_mdla || !va_vpu) {
		err = -ENOMEM;
		goto out;
	}

	/* start power on simulator */
	err = sim_power_on(dev, va_rpc);
	if (err)
		goto free_iomem;

	/* clear CGs */
	writel_relaxed(0xFFFFFFFF, (va_mdla + 0x9008));
	writel_relaxed(0xFFFFFFFF, (va_mdla + 0x0008));
	writel_relaxed(0xFFFFFFFF, (va_vpu + 0x0108));
	writel_relaxed(0xFFFFFFFF, (va_vpu + 0x1108));
	writel_relaxed(0xFFFFFFFF, (va_vpu + 0x2108));

free_iomem:
	devm_iounmap(dev, va_rpc);
	devm_iounmap(dev, va_mdla);
	devm_iounmap(dev, va_vpu);

out:
	return err;
}

static int apu_power_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id apu_power_of_match[] = {
	{ .compatible = "mediatek,apusys_power" },
	{ /* end of list */},
};

static struct platform_driver apu_power_driver = {
	.probe	= apu_power_probe,
	.remove	= apu_power_remove,
	.driver = {
		.name = "apusys_power",
		.of_match_table = apu_power_of_match,
	},
};

static int __init apu_power_drv_init(void)
{
	return platform_driver_register(&apu_power_driver);
}

module_init(apu_power_drv_init)

static void __exit apu_power_drv_exit(void)
{
	platform_driver_unregister(&apu_power_driver);
}
module_exit(apu_power_drv_exit)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("apu power driver");

