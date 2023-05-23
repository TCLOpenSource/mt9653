// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include "rtc-mt5896-coda.h"
#include "rtc-mt5896-riu.h"

/* registers */
#define REG_OFFSET_08_L(base) (base + 0x0020)
#define REG_OFFSET_22_L(base) (base + 0x0088)

#define MTK_DTV_RTC_DEFAULT_XTAL	12000000
#define MTK_DTV_RTC_DEFAULT_FREQ	1
#define MTK_DTV_RTC_DEFAULT_COUNT	946684800

#define MTK_RTC_LOG_NONE	-1	/* set mtk loglevel to be useless */
#define MTK_RTC_LOG_MUTE	0
#define MTK_RTC_LOG_ERR		3
#define MTK_RTC_LOG_INFO	6
#define MTK_RTC_LOG_DEBUG	7

struct mtk_dtv_rtc {
	struct rtc_device *rtc;
	struct device *dev;
	void __iomem *rtc_bank;
	int irq;
	unsigned int irq_wake;
	unsigned int irq_enabled;
	unsigned int xtal;
	unsigned int freq;
	timeu64_t default_cnt;
	int log_level; /* for mtk_dbg */
};

static int mtk_dtv_rtc_alarm_irq_enable(struct device *dev,
					unsigned int enable);

static timeu64_t mtk_dtv_rtc_get_alarm_counter(struct mtk_dtv_rtc *pdata)
{
	timeu64_t cnt, hw_cnt[4];

	hw_cnt[0] = (mtk_readw_relaxed((pdata->rtc_bank + PM_RTC_REG_001C_PM_RTC0)) &
		     0xFFFFULL);
	hw_cnt[1] = (mtk_readw_relaxed((pdata->rtc_bank + PM_RTC_REG_0020_PM_RTC0)) &
		     0xFFFFULL) << 16;
	hw_cnt[2] = (mtk_readw_relaxed((pdata->rtc_bank + PM_RTC_REG_0024_PM_RTC0)) &
		     0xFFFFULL) << 32;
	hw_cnt[3] = (mtk_readw_relaxed((pdata->rtc_bank + PM_RTC_REG_0028_PM_RTC0)) &
		     0xFFFFULL) << 48;
	cnt = hw_cnt[0] | hw_cnt[1] | hw_cnt[2] | hw_cnt[3];

	if (pdata->log_level >= MTK_RTC_LOG_DEBUG)
		dev_err(pdata->dev, "get hw alarm cnt = 0x%llx\n", cnt);

	return cnt;
}

static void mtk_dtv_rtc_set_alarm_counter(struct mtk_dtv_rtc *pdata,
					  timeu64_t cnt)
{
	u16 val;

	val = (cnt & 0xFFFFULL);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_001C_PM_RTC0), val);
	val = ((cnt & 0xFFFF0000ULL) >> 16);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_0020_PM_RTC0), val);
	val = ((cnt & 0xFFFF00000000ULL) >> 32);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_0024_PM_RTC0), val);
	val = ((cnt & 0xFFFF000000000000ULL) >> 48);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_0028_PM_RTC0), val);

	if (pdata->log_level >= MTK_RTC_LOG_DEBUG)
		dev_err(pdata->dev, "set hw alarm cnt = 0x%llx\n", cnt);
}

static void mtk_dtv_rtc_set_load_counter(struct mtk_dtv_rtc *pdata,
					 timeu64_t cnt)
{
	u16 val;

	val = (cnt & 0xFFFFULL);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_000C_PM_RTC0), val);
	val = ((cnt & 0xFFFF0000ULL) >> 16);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_0010_PM_RTC0), val);
	val = ((cnt & 0xFFFF00000000ULL) >> 32);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_0014_PM_RTC0), val);
	val = ((cnt & 0xFFFF000000000000ULL) >> 48);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_0018_PM_RTC0), val);

	if (pdata->log_level >= MTK_RTC_LOG_DEBUG)
		dev_err(pdata->dev, "set hw load cnt = 0x%llx\n", cnt);
}

static timeu64_t mtk_dtv_rtc_get_now_counter(struct mtk_dtv_rtc *pdata)
{
	timeu64_t cnt, hw_cnt[4];
	u32 val;
	u32 retry = 0;

	/* trigger read */
	mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_READ_EN);

	/* wait for HW latch bits okay, or sometimes it reads wrong value */
	do {
		val = mtk_read_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, REG_C_READ_EN);
		retry++;
		/**
		 * HW bit clear cost is about 0.25us, and one while loop cost is about 0.1us.
		 * So, retry in 5 times should be enough for HW bit clear checking.
		 */
		if (retry > 5) {
			if (pdata->log_level < 0)
				dev_err(pdata->dev,
					"rtc read latch failed for %d times(0x%x)\n",
					retry, val);
			else if (pdata->log_level >= MTK_RTC_LOG_ERR)
				dev_err(pdata->dev,
					"rtc read latch failed for %d times(0x%x)\n",
					retry, val);
			break;
		}
	} while (val);

	hw_cnt[0] = (mtk_readw_relaxed((pdata->rtc_bank + PM_RTC_REG_002C_PM_RTC0)) &
		     0xFFFFULL);
	hw_cnt[1] = (mtk_readw_relaxed((pdata->rtc_bank + PM_RTC_REG_0030_PM_RTC0)) &
		     0xFFFFULL) << 16;
	hw_cnt[2] = (mtk_readw_relaxed((pdata->rtc_bank + PM_RTC_REG_0034_PM_RTC0)) &
		     0xFFFFULL) << 32;
	hw_cnt[3] = (mtk_readw_relaxed((pdata->rtc_bank + PM_RTC_REG_0038_PM_RTC0)) &
		     0xFFFFULL) << 48;
	cnt = hw_cnt[0] | hw_cnt[1] | hw_cnt[2] | hw_cnt[3];

	if (pdata->log_level >= MTK_RTC_LOG_DEBUG)
		dev_err(pdata->dev, "get hw now cnt = 0x%llx\n", cnt);

	return cnt;
}

static void mtk_dtv_rtc_set_now_counter(struct mtk_dtv_rtc *pdata,
					timeu64_t cnt)
{
	mtk_dtv_rtc_set_load_counter(pdata, cnt);

	/* set load counter to now */
	mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_LOAD_EN);

	if (pdata->log_level >= MTK_RTC_LOG_DEBUG)
		dev_err(pdata->dev, "set hw now cnt = 0x%llx\n", cnt);
}

static void mtk_dtv_rtc_hw_init(struct mtk_dtv_rtc *pdata)
{
	u16 val;
	u32 cw;
	timeu64_t cnt;

	/* reset rtc engine */
	/* 0x12 0x00 bit0 = 1 */
	mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_SOFT_RSTZ);

	/* clear rtc interrupt state */
	/* 0x12 0x00 bit7 = 7 */
	mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_INT_CLEAR);

	/* set freq control word */
	cw = pdata->xtal / pdata->freq;
	val = cw & 0xffffUL;
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_0004_PM_RTC0), val);
	val = ((cw & 0xffff0000UL) >> 16);
	mtk_writew_relaxed((pdata->rtc_bank + PM_RTC_REG_0008_PM_RTC0), val);

	cnt = mtk_dtv_rtc_get_now_counter(pdata);
	if (cnt <= pdata->default_cnt) {
		/* reset rtc engine */
		/* 0x12 0x00 bit0 = 0->1 */
		mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x0, REG_C_SOFT_RSTZ);
		mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_SOFT_RSTZ);

		/* set now counter */
		cnt = pdata->default_cnt;
		mtk_dtv_rtc_set_now_counter(pdata, cnt);
		if (pdata->log_level < 0)
			dev_info(pdata->dev, "reset rtc counter to 0x%llx\n",
				 cnt);
		else if (pdata->log_level >= MTK_RTC_LOG_INFO)
			dev_err(pdata->dev, "reset rtc counter to 0x%llx\n",
				cnt);
	}

	/* start rtc counter */
	/* 0x12 0x00 bit1 = 1 */
	mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_CNT_EN);
}

static irqreturn_t mtk_dtv_rtc_alarmirq(int irq, void *id)
{
	struct mtk_dtv_rtc *pdata = (struct mtk_dtv_rtc *)id;

	if (mtk_read_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, REG_RTC_INT)) {
		/* clear irq state */
		mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_INT_CLEAR);
		rtc_update_irq(pdata->rtc, 1, RTC_IRQF | RTC_AF);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int mtk_dtv_rtc_gettime(struct device *dev, struct rtc_time *tm)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev);
	timeu64_t cnt;

	cnt = mtk_dtv_rtc_get_now_counter(pdata);

	rtc_time64_to_tm(cnt, tm);

	if (pdata->log_level < 0)
		dev_dbg(dev, "rtc get time %5d:%02d:%02d-%02d:%02d:%02d, cnt=0x%llx\n",
			tm->tm_year+1900, tm->tm_mon+1,
			tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, cnt);
	else if (pdata->log_level >= MTK_RTC_LOG_DEBUG)
		dev_err(dev, "rtc get time %5d:%02d:%02d-%02d:%02d:%02d, cnt=0x%llx\n",
			tm->tm_year+1900, tm->tm_mon+1,
			tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, cnt);

	return rtc_valid_tm(tm);
}

static int mtk_dtv_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev);
	timeu64_t cnt;

	cnt = rtc_tm_to_time64(tm);

	if (pdata->log_level < 0)
		dev_dbg(dev, "rtc set time %5d:%02d:%02d-%02d:%02d:%02d, cnt=0x%llx\n",
			tm->tm_year+1900, tm->tm_mon+1,
			tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, cnt);
	else if (pdata->log_level >= MTK_RTC_LOG_DEBUG)
		dev_err(dev, "rtc set time %5d:%02d:%02d-%02d:%02d:%02d, cnt=0x%llx\n",
			tm->tm_year+1900, tm->tm_mon+1,
			tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, cnt);

	mtk_dtv_rtc_set_now_counter(pdata, cnt);

	return 0;
}

static int mtk_dtv_rtc_getalarm(struct device *dev, struct rtc_wkalrm *wkalrm)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev);
	timeu64_t cnt;

	cnt = mtk_dtv_rtc_get_alarm_counter(pdata);

	if (cnt == 0xFFFFFFFFFFFFFFFFULL) {
		/* it's default value, no one set alarm, mask irq for savety */
		mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_INT_MASK);
		wkalrm->enabled = 0;
	} else {
		rtc_time64_to_tm(cnt, &wkalrm->time);
		if (mtk_read_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, REG_C_INT_MASK))
			wkalrm->enabled = 0;
		else
			wkalrm->enabled = 1;
	}
	wkalrm->pending = !!(mtk_read_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, REG_RTC_INT));

	if (pdata->log_level < 0)
		dev_dbg(dev, "rtc get alarm cnt=0x%llx, enabled=0x%x, pending=0x%x\n",
			cnt, wkalrm->enabled, wkalrm->pending);
	else if (pdata->log_level >= MTK_RTC_LOG_DEBUG)
		dev_err(dev, "rtc get alarm cnt=0x%llx, enabled=0x%x, pending=0x%x\n",
			cnt, wkalrm->enabled, wkalrm->pending);

	return 0;
}

static int mtk_dtv_rtc_setalarm(struct device *dev, struct rtc_wkalrm *wkalrm)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev);
	timeu64_t cnt;

	cnt = rtc_tm_to_time64(&wkalrm->time);

	if (pdata->log_level < 0)
		dev_info(dev,
			 "rtc set alarm %5d:%02d:%02d-%02d:%02d:%02d, cnt=0x%llx\n",
			 wkalrm->time.tm_year+1900, wkalrm->time.tm_mon+1,
			 wkalrm->time.tm_mday, wkalrm->time.tm_hour,
			 wkalrm->time.tm_min, wkalrm->time.tm_sec,
			 cnt);
	else if (pdata->log_level >= MTK_RTC_LOG_INFO)
		dev_err(dev,
			"rtc set alarm %5d:%02d:%02d-%02d:%02d:%02d, cnt=0x%llx\n",
			wkalrm->time.tm_year+1900, wkalrm->time.tm_mon+1,
			wkalrm->time.tm_mday, wkalrm->time.tm_hour,
			wkalrm->time.tm_min, wkalrm->time.tm_sec,
			cnt);


	/* set irq mask to disable interrupt before setting a new one */
	mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_INT_MASK);

	synchronize_irq(pdata->irq);

	mtk_dtv_rtc_set_alarm_counter(pdata, cnt);

	if (wkalrm->enabled)
		mtk_dtv_rtc_alarm_irq_enable(dev, 1);
	else
		mtk_dtv_rtc_alarm_irq_enable(dev, 0);

	return 0;
}

static int mtk_dtv_rtc_alarm_irq_enable(struct device *dev, unsigned int enable)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev);
	int ret = 0;

	if (enable) {
		/* clear irq state */
		mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_INT_CLEAR);

		/* unmask irq */
		mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x0, REG_C_INT_MASK);
	} else {
		/* mask irq */
		mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_INT_MASK);

		/* clear irq state */
		mtk_write_fld(pdata->rtc_bank + PM_RTC_REG_0000_PM_RTC0, 0x1, REG_C_INT_CLEAR);
	}

	if (pdata->log_level < 0)
		dev_info(dev, "rtc alarm irq enable=0x%x\n", enable);
	else if (pdata->log_level >= MTK_RTC_LOG_INFO)
		dev_err(dev, "rtc alarm irq enable=0x%x\n", enable);

	return 0;
}
static const struct rtc_class_ops mtk_dtv_rtc_ops = {
	.read_time		= mtk_dtv_rtc_gettime,
	.set_time		= mtk_dtv_rtc_settime,
	.read_alarm		= mtk_dtv_rtc_getalarm,
	.set_alarm		= mtk_dtv_rtc_setalarm,
	.alarm_irq_enable = mtk_dtv_rtc_alarm_irq_enable,
};

static const struct of_device_id mtk_dtv_rtc_match[] = {
	{ .compatible = "mediatek,mt5896-rtc" },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_dtv_rtc_match);

/* rtc sysfs debug tools */
static ssize_t log_level_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev->parent);
	ssize_t retval;
	int level;

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < MTK_RTC_LOG_NONE) || (level > MTK_RTC_LOG_DEBUG))
			retval = -EINVAL;
		else
			pdata->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t log_level_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev->parent);

	return snprintf(buf, PAGE_SIZE, "none:\t\t%d\nmute:\t\t%d\nerror:\t\t%d\n"
		       "info:\t\t%d\ndebug:\t\t%d\ncurrent:\t%d\n",
		       MTK_RTC_LOG_NONE, MTK_RTC_LOG_MUTE, MTK_RTC_LOG_ERR,
		       MTK_RTC_LOG_INFO, MTK_RTC_LOG_DEBUG, pdata->log_level);
}

static DEVICE_ATTR_RW(log_level);

static ssize_t hw_alarm_count_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev->parent);
	struct rtc_time time;
	timeu64_t cnt;

	cnt = mtk_dtv_rtc_get_alarm_counter(pdata);
	rtc_time64_to_tm(cnt, &time);

	return snprintf(buf, PAGE_SIZE, "cnt:\t0x%llx\n%ptR\n", cnt, &time);
}

static DEVICE_ATTR_RO(hw_alarm_count);

static ssize_t hw_now_count_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct mtk_dtv_rtc *pdata = dev_get_drvdata(dev->parent);
	struct rtc_time time;
	timeu64_t cnt;

	cnt = mtk_dtv_rtc_get_now_counter(pdata);
	rtc_time64_to_tm(cnt, &time);

	return snprintf(buf, PAGE_SIZE, "cnt:\t0x%llx\n%ptR\n", cnt, &time);
}

static DEVICE_ATTR_RO(hw_now_count);

static ssize_t help_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	return snprintf(buf, PAGE_SIZE, "Debug Help:\n"
		       "- log_level <RW>: To control debug log level.\n"
		       "                  Read log_level for the definition of log level number.\n"
		       "- hw_now_count <RO>: Read RTC time and hardware count.\n"
		       "- hw_alarm_count <RO>: Read RTC alarm time and alarm hardware count.\n");
}

static DEVICE_ATTR_RO(help);

static struct attribute *mtk_rtc_hw_attrs[] = {
	&dev_attr_log_level.attr,
	&dev_attr_hw_alarm_count.attr,
	&dev_attr_hw_now_count.attr,
	&dev_attr_help.attr,
	NULL,
};

static const struct attribute_group mtk_rtc_hw_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_rtc_hw_attrs,
};


static int mtk_dtv_rtc_probe(struct platform_device *pdev)
{
	struct mtk_dtv_rtc *pdata;
	struct resource *res;
	struct device_node *dn;
	int ret;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	platform_set_drvdata(pdev, pdata);
	pdata->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pdata->rtc_bank = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->rtc_bank)) {
		dev_err(&pdev->dev, "err rtc_bank=%#lx\n",
			(unsigned long)pdata->rtc_bank);
		return PTR_ERR(pdata->rtc_bank);
	}
	dev_dbg(&pdev->dev, "rtc_bank=%#lx\n",
		(unsigned long)pdata->rtc_bank);

	dn = pdev->dev.of_node;

	ret = of_property_read_u32(dn, "xtal", &pdata->xtal);
	if ((ret < 0) || (pdata->xtal <= 0)) {
		dev_info(&pdev->dev,
			 "bad xtal(%d), set it to default value(%d)\n",
			 pdata->xtal, MTK_DTV_RTC_DEFAULT_XTAL);
		pdata->xtal = MTK_DTV_RTC_DEFAULT_XTAL;
	}

	ret = of_property_read_u32(dn, "freq", &pdata->freq);
	if ((ret < 0) || (pdata->freq <= 0)) {
		dev_info(&pdev->dev,
			 "bad freq(%d), set it to default value(%d)\n",
			 pdata->freq, MTK_DTV_RTC_DEFAULT_FREQ);
		pdata->freq = MTK_DTV_RTC_DEFAULT_FREQ;
	}

	ret = of_property_read_u64(dn, "default-cnt", &pdata->default_cnt);
	if ((ret < 0) || (pdata->default_cnt <= 0)) {
		dev_info(&pdev->dev,
			 "bad def cnt(%lld), set to default(%d)\n",
			 pdata->default_cnt,
			 MTK_DTV_RTC_DEFAULT_COUNT);
		pdata->default_cnt = MTK_DTV_RTC_DEFAULT_COUNT;
	}

	dev_dbg(&pdev->dev, "xtal(%d), freq(%d), default_cnt(%lld)\n",
		pdata->xtal, pdata->freq, pdata->default_cnt);

	pdata->irq = platform_get_irq(pdev, 0);
	if (pdata->irq < 0) {
		dev_err(&pdev->dev, "no IRQ resource\n");
		ret = pdata->irq;
		goto err;
	}

	dev_dbg(&pdev->dev, "irq=%d\n", pdata->irq);

	ret = devm_request_irq(&pdev->dev, pdata->irq, mtk_dtv_rtc_alarmirq,
			       IRQF_TRIGGER_HIGH | IRQF_SHARED,
			       dev_name(&pdev->dev), pdata);
	if (ret) {
		dev_err(&pdev->dev, "can't request IRQ\n");
		goto err;
	}

	mtk_dtv_rtc_hw_init(pdata);

	device_init_wakeup(&pdev->dev, true);

	/* separate allocate rtc dev and register rtc dev, due to rtc add sysfs group */
	pdata->rtc = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(pdata->rtc)) {
		ret = PTR_ERR(pdata->rtc);
		dev_err(&pdev->dev, "unable to allocate rtc device\n");
		goto err;
	}

	/* sysfs debug */
	ret = rtc_add_group(pdata->rtc, &mtk_rtc_hw_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "add sysfs group failed\n");
		goto err;
	}

	/* mtk loglevel */
	pdata->log_level = MTK_RTC_LOG_NONE;

	pdata->rtc->ops = &mtk_dtv_rtc_ops;

	ret = rtc_register_device(pdata->rtc);
	if (ret) {
		dev_err(&pdev->dev, "unable to register rtc device\n");
		goto err;
	}

	dev_info(&pdev->dev, "rtc probe done\n");

	return 0;
err:
	dev_err(&pdev->dev, "rtc probe error: ret=%d\n", ret);

	return ret;
}

static int mtk_dtv_rtc_remove(struct platform_device *pdev)
{
	mtk_dtv_rtc_alarm_irq_enable(&pdev->dev, 0);
	device_init_wakeup(&pdev->dev, 0);
	dev_info(&pdev->dev, "rtc remove done\n");

	return 0;
}

static struct platform_driver mtk_dtv_rtc_driver = {
	.probe	= mtk_dtv_rtc_probe,
	.remove	= mtk_dtv_rtc_remove,
	.driver = {
		.name = "mt5896-rtc",
		.of_match_table = mtk_dtv_rtc_match,
	},
};

module_platform_driver(mtk_dtv_rtc_driver);

MODULE_DESCRIPTION("MediaTek DTV SoC based RTC Driver");
MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");
MODULE_LICENSE("GPL");
