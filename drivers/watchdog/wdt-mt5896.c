// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Owen.Tseng <Owen.Tseng@mediatek.com>
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
#include "wdt-mt5896-coda.h"
#include "wdt-mt5896-riu.h"
#include <linux/jiffies.h>
#include <linux/timer.h>

#define WDT_MAX_TIMEOUT		30
#define WDT_MIN_TIMEOUT		1

#define WDT_CLOCK		12000000
#define WDT_CLOCK_MS	12000
#define REG_0024_PM_POR_STATUS(base)	(base + 0x024)

#define DRV_NAME		"mtktv-wdt"
#define DRV_VERSION		"1.0"
#define WDT_HEART_BEATS_HZ	(HZ/2)
#define STR_LENSIZE		32

static bool nowayout = WATCHDOG_NOWAYOUT;
static unsigned int timeout = WDT_MAX_TIMEOUT;
bool bwdtenable = true;

struct mtktv_wdt_dev {
	struct watchdog_device wdt_dev;
	void __iomem *wdt_base;
	void __iomem *pm_port_base;
	int irq_num;
	struct timer_list timer;	/* The timer that pings the watchdog */
	u32 wdt_ping_self;
	u32 wdt_timeout_ms;
	struct kobject *wdt_kobj;
};

static int mtktv_wdt_restart(struct watchdog_device *wdt_dev,
			     unsigned long action, void *data)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);

	while (1) {
		mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0190_L4_MAIN), 1);
		mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0194_L4_MAIN), 0);
		mdelay(5);
	}

	return 0;
}

static int mtktv_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);

	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0180_L4_MAIN), 1);

	return 0;
}

static int mtktv_wdt_set_timeout(struct watchdog_device *wdt_dev,
				  unsigned int timeout)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	unsigned int wdt_cnt = timeout*WDT_CLOCK;

	wdt_dev->timeout = timeout;

	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0190_L4_MAIN), wdt_cnt);
	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0194_L4_MAIN), (wdt_cnt >> 16));

	mtk_write_fld((mtktv_wdt->wdt_base + REG_0198_L4_MAIN), 0x1, REG_WDT_ENABLE_1ST);
	mtk_write_fld((mtktv_wdt->wdt_base + REG_0198_L4_MAIN), 0x1, REG_WDT_ENABLE_2ND);
	mtk_write_fld((mtktv_wdt->wdt_base + REG_0198_L4_MAIN), 0x0, REG_WDT_2ND_MAX);

	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0180_L4_MAIN), 1);

	return 0;
}

static int mtktv_wdt_set_timeout_ms(struct watchdog_device *wdt_dev,
				  unsigned int timeout)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	unsigned int wdt_cnt = timeout*WDT_CLOCK_MS;

	mtktv_wdt->wdt_timeout_ms = timeout;
	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0190_L4_MAIN), wdt_cnt);
	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0194_L4_MAIN), (wdt_cnt >> 16));

	mtk_write_fld((mtktv_wdt->wdt_base + REG_0198_L4_MAIN), 0x1, REG_WDT_ENABLE_1ST);
	mtk_write_fld((mtktv_wdt->wdt_base + REG_0198_L4_MAIN), 0x1, REG_WDT_ENABLE_2ND);
	mtk_write_fld((mtktv_wdt->wdt_base + REG_0198_L4_MAIN), 0x0, REG_WDT_2ND_MAX);

	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0180_L4_MAIN), 1);

	return 0;
}

static int mtktv_wdt_set_pretimeout(struct watchdog_device *wdt_dev,
				    unsigned int timeout)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);
	unsigned int wdt_cnt = timeout*WDT_CLOCK;

	wdt_dev->pretimeout = timeout;

	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_018C_L4_MAIN), (wdt_cnt >> 16));

	return 0;
}

static int mtktv_wdt_stop(struct watchdog_device *wdt_dev)
{
	u16 val;
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);

	val = mtk_readw_relaxed(REG_0024_PM_POR_STATUS(mtktv_wdt->pm_port_base));
	val |= (BIT(0));
	mtk_writew_relaxed(REG_0024_PM_POR_STATUS(mtktv_wdt->pm_port_base), val);
	mtk_write_fld((mtktv_wdt->wdt_base + REG_0198_L4_MAIN), 0x0, REG_WDT_ENABLE_1ST);
	mtk_write_fld((mtktv_wdt->wdt_base + REG_0198_L4_MAIN), 0x0, REG_WDT_ENABLE_2ND);
	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0190_L4_MAIN), 0);
	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0194_L4_MAIN), 0);
	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_018C_L4_MAIN), 0);

	return 0;
}

static int mtktv_wdt_start(struct watchdog_device *wdt_dev)
{
	u16 val;
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_dev);

	val = mtk_readw_relaxed(REG_0024_PM_POR_STATUS(mtktv_wdt->pm_port_base));
	if (bwdtenable)
		val &= ~(BIT(0));
	mtk_writew_relaxed(REG_0024_PM_POR_STATUS(mtktv_wdt->pm_port_base), val);

	return 0;
}

static irqreturn_t mtktv_wdt_isr(int irq, void *wdt_arg)
{
	struct mtktv_wdt_dev *mtktv_wdt = watchdog_get_drvdata(wdt_arg);
	struct device *dev = mtktv_wdt->wdt_dev.parent;

	sysfs_notify(&dev->kobj, "wdt_extend", "wdtwake");
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

static void wdtdev_inter_ping(struct timer_list *t)
{
	struct mtktv_wdt_dev *mtktv_wdt = from_timer(mtktv_wdt, t, timer);

	mtk_writew_relaxed((mtktv_wdt->wdt_base + REG_0180_L4_MAIN), 1);
	mod_timer(&mtktv_wdt->timer, jiffies + WDT_HEART_BEATS_HZ);
}

/* wdt sysfs for wakeup reason poll */
static ssize_t wdtwake_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct mtktv_wdt_dev *mtktv_wdt = dev_get_drvdata(dev);

	return scnprintf(buf, STR_LENSIZE, "%d\n", mtktv_wdt->wdt_dev.timeout);
}

static DEVICE_ATTR_RO(wdtwake);

/* wdt timeout path */
static ssize_t wdt_ms_timeout_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct mtktv_wdt_dev *mtktv_wdt = dev_get_drvdata(dev);

	return scnprintf(buf, STR_LENSIZE, "%d\n", mtktv_wdt->wdt_timeout_ms);
}

static ssize_t wdt_ms_timeout_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtktv_wdt_dev *mtktv_wdt = dev_get_drvdata(dev);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	mtktv_wdt_start(&mtktv_wdt->wdt_dev);
	mtktv_wdt_set_timeout_ms(&mtktv_wdt->wdt_dev, val);

	return ret ? : size;
}

static DEVICE_ATTR_RW(wdt_ms_timeout);

/* wdt off path */
static ssize_t wdtpinself_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct mtktv_wdt_dev *mtktv_wdt = dev_get_drvdata(dev);

	return scnprintf(buf, STR_LENSIZE, "%d\n", mtktv_wdt->wdt_dev.timeout);
}

static ssize_t wdtpinself_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct mtktv_wdt_dev *mtktv_wdt = dev_get_drvdata(dev);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	if (val) {
		timer_setup(&mtktv_wdt->timer, wdtdev_inter_ping, 0);
		mod_timer(&mtktv_wdt->timer, jiffies + WDT_HEART_BEATS_HZ);
	} else
		mtktv_wdt_stop(&mtktv_wdt->wdt_dev);

	return ret ? : size;
}

static DEVICE_ATTR_RW(wdtpinself);

static struct attribute *wdt_extend_attrs[] = {
	&dev_attr_wdtwake.attr,
	&dev_attr_wdtpinself.attr,
	&dev_attr_wdt_ms_timeout.attr,
	NULL,
};
static const struct attribute_group wdt_extend_attr_group = {
	.name = "wdt_extend",
	.attrs = wdt_extend_attrs
};

static int mtktv_wdt_probe(struct platform_device *pdev)
{
	struct mtktv_wdt_dev *mtktv_wdt;
	struct resource *res;
	int err;
	int reset_sts;

	mtktv_wdt = devm_kzalloc(&pdev->dev, sizeof(*mtktv_wdt), GFP_KERNEL);
	if (!mtktv_wdt)
		return -ENOMEM;

	platform_set_drvdata(pdev, mtktv_wdt);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mtktv_wdt->wdt_base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(mtktv_wdt->wdt_base))
		return PTR_ERR(mtktv_wdt->wdt_base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	mtktv_wdt->pm_port_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mtktv_wdt->pm_port_base)) {
		dev_err(&pdev->dev, "err pm_port_base=%#lx\n",
			(unsigned long)mtktv_wdt->pm_port_base);
		return PTR_ERR(mtktv_wdt->pm_port_base);
	}
	dev_info(&pdev->dev, "pm_port_base=%#lx\n",
		 (unsigned long)mtktv_wdt->pm_port_base);

	err = of_property_read_u32(pdev->dev.of_node,
				"wdt-ping-self", &mtktv_wdt->wdt_ping_self);
	if (err) {
		dev_info(&pdev->dev, "could not get pingself resource\n");
		mtktv_wdt->wdt_ping_self = 0;
	}

	mtktv_wdt->wdt_dev.info = &mtktv_wdt_pretimeout_info;

	mtktv_wdt->irq_num = platform_get_irq(pdev, 0);
	if (res > 0)
		if (!devm_request_threaded_irq(&pdev->dev, mtktv_wdt->irq_num,
					NULL,
					mtktv_wdt_isr,
					IRQF_ONESHOT, dev_name(&pdev->dev), &mtktv_wdt->wdt_dev))
			mtktv_wdt->wdt_dev.info = &mtktv_wdt_pretimeout_info;

	mtktv_wdt->wdt_dev.ops = &mtktv_wdt_ops;
	mtktv_wdt->wdt_dev.timeout = WDT_MAX_TIMEOUT;
	mtktv_wdt->wdt_dev.max_timeout = WDT_MAX_TIMEOUT;
	mtktv_wdt->wdt_dev.min_timeout = WDT_MIN_TIMEOUT;
	mtktv_wdt->wdt_dev.parent = &pdev->dev;

	watchdog_init_timeout(&mtktv_wdt->wdt_dev, timeout, &pdev->dev);
	watchdog_set_nowayout(&mtktv_wdt->wdt_dev, nowayout);
	watchdog_set_restart_priority(&mtktv_wdt->wdt_dev, 128);

	reset_sts = mtk_readw_relaxed((mtktv_wdt->wdt_base + REG_0198_L4_MAIN));
	if ((reset_sts) & 1)
		mtktv_wdt->wdt_dev.bootstatus = WDIOF_CARDRESET;

	watchdog_set_drvdata(&mtktv_wdt->wdt_dev, mtktv_wdt);
	if (mtktv_wdt->wdt_ping_self == 1) {
		timer_setup(&mtktv_wdt->timer, wdtdev_inter_ping, 0);
		mod_timer(&mtktv_wdt->timer, jiffies + WDT_HEART_BEATS_HZ);
		mtktv_wdt_start(&mtktv_wdt->wdt_dev);
	}

	err = watchdog_register_device(&mtktv_wdt->wdt_dev);
	if (unlikely(err))
		return err;

	err = sysfs_create_group(&pdev->dev.kobj, &wdt_extend_attr_group);
	if (err < 0)
		dev_err(&pdev->dev, "wdt sysfs_create_group failed: %d\n", err);

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
	{ .compatible = "mediatek,mt5896-wdt" },
	{},
};
MODULE_DEVICE_TABLE(of, mtktv_wdt_dt_ids);

static const struct dev_pm_ops mtktv_wdt_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtktv_wdt_suspend,
				mtktv_wdt_resume)
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

module_param_named(wdt_enable, bwdtenable, bool, 0444);
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
			__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Owen Tseng");
MODULE_DESCRIPTION("Mediatek WatchDog Timer Driver");
MODULE_VERSION(DRV_VERSION);
