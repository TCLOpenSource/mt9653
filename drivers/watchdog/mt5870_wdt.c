// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#define WDT_MAX_TIMEOUT		30
#define WDT_MIN_TIMEOUT		1

#define WDT_CLOCK		12000000

#define WDT_CLR			(0x0 << 1)
#define WDT_INT_CNT		(0x6 << 1)
#define WDT_MAX_LCNT		(0x8 << 1)
#define WDT_MAX_HCNT		(0xa << 1)

#define DRV_NAME		"mt5870-wdt"
#define DRV_VERSION		"1.0"

static bool nowayout = WATCHDOG_NOWAYOUT;
static unsigned int timeout = WDT_MAX_TIMEOUT;

struct mtktv_wdt_dev {
	struct watchdog_device wdt_dev;
	void __iomem *wdt_base;
	int irq_num;
};

static int mtktv_wdt_restart(struct watchdog_device *wdt_dev,
			     unsigned long action, void *data)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtktv_wdt->wdt_base;

	while (1) {
		writel(1, wdt_base + WDT_MAX_LCNT);
		writel(0, wdt_base + WDT_MAX_HCNT);
		mdelay(5);
	}

	return 0;
}

static int mtktv_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtktv_wdt->wdt_base;

	writel(0x1, wdt_base + WDT_CLR);

	return 0;
}

static int mtktv_wdt_set_timeout(struct watchdog_device *wdt_dev,
				  unsigned int timeout)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtktv_wdt->wdt_base;
	unsigned int wdt_cnt = timeout*WDT_CLOCK;

	wdt_dev->timeout = timeout;

	writel(wdt_cnt, wdt_base + WDT_MAX_LCNT);
	writel((wdt_cnt >> 16), wdt_base + WDT_MAX_HCNT);

	writel(0x1, wdt_base + WDT_CLR);

	return 0;
}

static int mtktv_wdt_set_pretimeout(struct watchdog_device *wdt_dev,
				    unsigned int timeout)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtktv_wdt->wdt_base;
	unsigned int wdt_cnt = timeout*WDT_CLOCK;

	wdt_dev->pretimeout = timeout;

	writel((wdt_cnt >> 16), wdt_base + WDT_INT_CNT);

	return 0;
}

static int mtktv_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtktv_wdt->wdt_base;

	writel(0, wdt_base + WDT_MAX_LCNT);
	writel(0, wdt_base + WDT_MAX_HCNT);
	writel(0, wdt_base + WDT_INT_CNT);

	return 0;
}

static int mtktv_wdt_start(struct watchdog_device *wdt_dev)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = mtktv_wdt->wdt_base;

	return mtktv_wdt_set_timeout(wdt_dev, wdt_dev->timeout);
}

static irqreturn_t mtktv_wdt_isr(int irq, void *wdt_arg)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_arg);

	watchdog_notify_pretimeout(wdt_arg);

	return IRQ_HANDLED;
}

static const struct watchdog_info mtktv_wdt_info = {
	.identity	= DRV_NAME,
	.options	= WDIOF_SETTIMEOUT |
			  WDIOF_KEEPALIVEPING |
			  WDIOF_MAGICCLOSE,
};

static const struct watchdog_info mtktv_wdt_pretimeout_info = {
	.identity	= DRV_NAME,
	.options	= WDIOF_SETTIMEOUT |
			  WDIOF_KEEPALIVEPING |
			  WDIOF_MAGICCLOSE |
			  WDIOF_PRETIMEOUT,
};

static const struct watchdog_ops mtktv_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= mtktv_wdt_start,
	.stop		= mtktv_wdt_stop,
	.ping		= mtktv_wdt_ping,
	.set_timeout	= mtktv_wdt_set_timeout,
	.set_pretimeout	= mtktv_wdt_set_pretimeout,
	.restart	= mtktv_wdt_restart,
};

static int mtktv_wdt_probe(struct platform_device *pdev)
{
	struct mtktv_wdt_dev *mtktv_wdt;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	int err;

	mtktv_wdt = devm_kzalloc(&pdev->dev, sizeof(*mtktv_wdt), GFP_KERNEL);
	if (!mtktv_wdt)
		return -ENOMEM;

	platform_set_drvdata(pdev, mtktv_wdt);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mtktv_wdt->wdt_base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(mtktv_wdt->wdt_base))
		return PTR_ERR(mtktv_wdt->wdt_base);

	mtktv_wdt->wdt_dev.info = &mtktv_wdt_pretimeout_info;

	mtktv_wdt->irq_num = platform_get_irq(pdev, 0);
	if (res > 0)
		if (!devm_request_irq(&pdev->dev, mtktv_wdt->irq_num,
				      mtktv_wdt_isr, 0, dev_name(&pdev->dev),
				      &mtktv_wdt->wdt_dev))
			mtktv_wdt->wdt_dev.info = &mtktv_wdt_pretimeout_info;

	mtktv_wdt->wdt_dev.ops = &mtktv_wdt_ops;
	mtktv_wdt->wdt_dev.timeout = WDT_MAX_TIMEOUT;
	mtktv_wdt->wdt_dev.max_timeout = WDT_MAX_TIMEOUT;
	mtktv_wdt->wdt_dev.min_timeout = WDT_MIN_TIMEOUT;
	mtktv_wdt->wdt_dev.parent = &pdev->dev;

	watchdog_init_timeout(&mtktv_wdt->wdt_dev, timeout, &pdev->dev);
	watchdog_set_nowayout(&mtktv_wdt->wdt_dev, nowayout);
	watchdog_set_restart_priority(&mtktv_wdt->wdt_dev, 128);

	watchdog_set_drvdata(&mtktv_wdt->wdt_dev, mtktv_wdt);

	mtktv_wdt_stop(&mtktv_wdt->wdt_dev);

	err = watchdog_register_device(&mtktv_wdt->wdt_dev);
	if (unlikely(err))
		return err;

	dev_info(&pdev->dev, "Watchdog enabled (timeout=%d sec, nowayout=%d)\n",
		 mtktv_wdt->wdt_dev.timeout, nowayout);

	return 0;
}

static void mtktv_wdt_shutdown(struct platform_device *pdev)
{
	struct mtktv_wdt_dev *mtktv_wdt = platform_get_drvdata(pdev);

	if (watchdog_active(&mtktv_wdt->wdt_dev))
		mtktv_wdt_stop(&mtktv_wdt->wdt_dev);
}

static int mtktv_wdt_remove(struct platform_device *pdev)
{
	struct mtktv_wdt_dev *mtktv_wdt = platform_get_drvdata(pdev);

	watchdog_unregister_device(&mtktv_wdt->wdt_dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mtktv_wdt_suspend(struct device *dev)
{
	struct mtktv_wdt_dev *mtktv_wdt = dev_get_drvdata(dev);

	if (watchdog_active(&mtktv_wdt->wdt_dev))
		mtktv_wdt_stop(&mtktv_wdt->wdt_dev);

	return 0;
}

static int mtktv_wdt_resume(struct device *dev)
{
	struct mtktv_wdt_dev *mtktv_wdt = dev_get_drvdata(dev);

	if (watchdog_active(&mtktv_wdt->wdt_dev)) {
		mtktv_wdt_start(&mtktv_wdt->wdt_dev);
		mtktv_wdt_ping(&mtktv_wdt->wdt_dev);
	}

	return 0;
}
#endif

static const struct of_device_id mtktv_wdt_dt_ids[] = {
	{ .compatible = "mediatek,mt5870-wdt" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mtktv_wdt_dt_ids);

static const struct dev_pm_ops mtktv_wdt_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtktv_wdt_suspend, mtktv_wdt_resume)
};

static struct platform_driver mtktv_wdt_driver = {
	.probe		= mtktv_wdt_probe,
	.remove		= mtktv_wdt_remove,
	.shutdown	= mtktv_wdt_shutdown,
	.driver		= {
		.name		= DRV_NAME,
		.pm		= &mtktv_wdt_pm_ops,
		.of_match_table	= mtktv_wdt_dt_ids,
	},
};

module_platform_driver(mtktv_wdt_driver);

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
		 __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MTKTV");
MODULE_DESCRIPTION("Mediatek WatchDog Timer Driver");
MODULE_VERSION(DRV_VERSION);
