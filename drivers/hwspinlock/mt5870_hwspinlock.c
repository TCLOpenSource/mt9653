// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/hwspinlock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "hwspinlock_internal.h"
struct mtk_dtv_hwspinlock {
	void __iomem *io_base;
	struct hwspinlock_device bank;
};
/* ID of this processor */
#define HW_SPINLOCK_PROCESSOR_ID 1
/* Number of Hardware Spinlocks*/
#define HW_SPINLOCK_NUMBER 16
/* Hardware spinlock register offsets */
#define HW_SPINLOCK_OFFSET(x) (0x4 * (x))
/* returns 0 on failure and true on success */
static int mtk_dtv_hwspinlock_trylock(struct hwspinlock *lock)
{
	void __iomem *addr = lock->priv;
	u16 val;

	writew_relaxed(HW_SPINLOCK_PROCESSOR_ID, addr);
	val = readw_relaxed(addr);
	dev_dbg(lock->bank->dev,
		"hwspinlock trylock: addr=0x%lx, val=0x%x, id=0x%x\n",
		(unsigned long)addr, val, HW_SPINLOCK_PROCESSOR_ID);
	if (val != HW_SPINLOCK_PROCESSOR_ID)
		return 0;
	else
		return 1;
}
static void mtk_dtv_hwspinlock_unlock(struct hwspinlock *lock)
{
	void __iomem *addr = lock->priv;
	u16 val;

	val = readw_relaxed(addr);
	if ((val != 0) && (val != HW_SPINLOCK_PROCESSOR_ID)) {
		dev_warn(lock->bank->dev,
			 "hwspinlock unlock: lock not owned by us\n");
		dev_warn(lock->bank->dev,
			 "hwspinlock unlock: addr=0x%lx, val=0x%x, id=0x%x\n",
			 (unsigned long)addr, val, HW_SPINLOCK_PROCESSOR_ID);
	}
	dev_dbg(lock->bank->dev,
		"hwspinlock unlock: addr=0x%lx, val=0x%x, id=0x%x\n",
		(unsigned long)addr, val, HW_SPINLOCK_PROCESSOR_ID);
	writew_relaxed(0, addr);
}
static const struct hwspinlock_ops mtk_dtv_hwspinlock_ops = {
	.trylock = mtk_dtv_hwspinlock_trylock,
	.unlock = mtk_dtv_hwspinlock_unlock,
};
static int mtk_dtv_hwspinlock_probe(struct platform_device *pdev)
{
	struct mtk_dtv_hwspinlock *pdata;
	struct resource *res;
	struct hwspinlock *hwlock;
	int idx, ret;

	if (!pdev->dev.of_node)
		return -ENODEV;
	pdata = devm_kzalloc(&pdev->dev,
			      struct_size(pdata, bank.lock,
					  HW_SPINLOCK_NUMBER),
			      GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pdata->io_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->io_base)) {
		dev_err(&pdev->dev, "err io_base=%#lx\n",
			(unsigned long)pdata->io_base);
		return PTR_ERR(pdata->io_base);
	}
	dev_info(&pdev->dev, "io_base=%#lx\n",
		 (unsigned long)pdata->io_base);
	for (idx = 0; idx < HW_SPINLOCK_NUMBER; idx++) {
		hwlock = &pdata->bank.lock[idx];
		hwlock->priv = pdata->io_base + HW_SPINLOCK_OFFSET(idx);
	}
	platform_set_drvdata(pdev, pdata);
	pm_runtime_enable(&pdev->dev);
	ret = devm_hwspin_lock_register(&pdev->dev, &pdata->bank,
					&mtk_dtv_hwspinlock_ops, 0,
					HW_SPINLOCK_NUMBER);
	if (ret)
		goto reg_failed;
	dev_info(&pdev->dev, "hwspinlock probe done\n");
	return 0;
reg_failed:
	pm_runtime_disable(&pdev->dev);
	dev_err(&pdev->dev, "hwspinlock probe error: ret=%d\n", ret);
	return ret;
}
static int mtk_dtv_hwspinlock_remove(struct platform_device *pdev)
{
	struct mtk_dtv_hwspinlock *pdata = platform_get_drvdata(pdev);
	int ret;

	ret = devm_hwspin_lock_unregister(&pdev->dev, &pdata->bank);
	if (ret) {
		dev_err(&pdev->dev, "%s failed: %d\n", __func__, ret);
		return ret;
	}
	pm_runtime_disable(&pdev->dev);
	return 0;
}
static const struct of_device_id mtk_dtv_hwpinlock_ids[] = {
	{ .compatible = "mediatek,mt5870-hwspinlock", },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_dtv_hwpinlock_ids);
static struct platform_driver mtk_dtv_hwspinlock_driver = {
	.probe = mtk_dtv_hwspinlock_probe,
	.remove = mtk_dtv_hwspinlock_remove,
	.driver = {
		.name = "mt5870-hwspinlock",
		.of_match_table = of_match_ptr(mtk_dtv_hwpinlock_ids),
	},
};
module_platform_driver(mtk_dtv_hwspinlock_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mediatek MT5870 Hardware spinlock driver");
MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");

