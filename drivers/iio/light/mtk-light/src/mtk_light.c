// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/export.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/iio/consumer.h>
#include <linux/iio/types.h>
#include <linux/spinlock.h>
#include <linux/device.h>

#include "mtk_light.h"

#define SAMPLE_RATE_HZ     50
#define SAMPLE_INTERVAL_MS (1000 / (SAMPLE_RATE_HZ))
#define SAMPLE_TIMEOUT     SAMPLE_RATE_HZ

struct mtk_light_info {
	struct iio_channel *channel;
	struct mtk_light_range range;
	char *channel_name;
	int lux;
};

static struct task_struct *_mtk_light_sample_task;
static struct mtk_light_info *_light_info;
static spinlock_t _light_lock;
static bool _sample_trigger;
DECLARE_WAIT_QUEUE_HEAD(_sample_wait);

static int _mtk_light_change_channel(struct device *dev, char *channel_name);

static ssize_t iio_channel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;

	spin_lock(&_light_lock);

	if (!_light_info) {
		ret = -1;
		goto unlock;
	}

	ret = snprintf(buf, PAGE_SIZE, "channel = %s\n", _light_info->channel_name);
	if (ret)
		pr_err("[mtk-light] snprintf fail : %d\n", ret);

unlock:
	spin_unlock(&_light_lock);

	return ret;
}

static ssize_t iio_channel_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	_mtk_light_change_channel(dev, (char *)buf);
	return count;
}
static DEVICE_ATTR_RW(iio_channel);

static ssize_t lux_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int lux = 0;

	if (mtk_light_get_lux(&lux) == 0)
		ret = snprintf(buf, PAGE_SIZE, "%d\n", lux);
	else
		ret = snprintf(buf, PAGE_SIZE, "Get lux fail\n");

	if (ret)
		pr_err("[mtk-light] snprintf fail : %d\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(lux);

static ssize_t lux_max_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;

	spin_lock(&_light_lock);

	if (_light_info == NULL) {
		ret = -1;
		goto unlock;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n", _light_info->range.max);
	if (ret)
		pr_err("[mtk-light] snprintf fail : %d\n", ret);

unlock:
	spin_unlock(&_light_lock);

	return ret;
}
static DEVICE_ATTR_RO(lux_max);

static ssize_t lux_min_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;

	spin_lock(&_light_lock);

	if (_light_info == NULL) {
		ret = -1;
		goto unlock;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n", _light_info->range.min);
	if (ret)
		pr_err("[mtk-light] snprintf fail : %d\n", ret);

unlock:
	spin_unlock(&_light_lock);

	return ret;
}
static DEVICE_ATTR_RO(lux_min);

static struct attribute *mtk_light_attrs[] = {
	&dev_attr_iio_channel.attr,
	&dev_attr_lux.attr,
	&dev_attr_lux_max.attr,
	&dev_attr_lux_min.attr,
	NULL,
};

static const struct attribute_group mtk_light_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_light_attrs
};

int mtk_light_get_lux_range(struct mtk_light_range *range)
{
	if (!_light_info || !range)
		return -EINVAL;

	memcpy(range, &_light_info->range, sizeof(struct mtk_light_range));

	return 0;
}
EXPORT_SYMBOL(mtk_light_get_lux_range);

int mtk_light_get_lux(int *lux)
{
	int ret = 0;

	spin_lock(&_light_lock);

	if (!_light_info || !lux) {
		ret = -EINVAL;
		goto unlock;
	}

	*lux = _light_info->lux;

unlock:
	spin_unlock(&_light_lock);
	return ret;
}
EXPORT_SYMBOL(mtk_light_get_lux);

static int mtk_light_get_iio_lux(int *lux)
{
	int ret;
	struct iio_channel *channel = NULL;

	spin_lock(&_light_lock);
	if (!_light_info || !_light_info->channel || !lux) {
		ret = -EINVAL;
		spin_unlock(&_light_lock);
		return ret;
	}
	channel = _light_info->channel;
	spin_unlock(&_light_lock);

	ret = iio_read_channel_processed(channel, lux);
	if (ret < 0)
		pr_err("[mtk-light] iio_read_channel_processed fail : %d\n", ret);

	return ret;
}

static void mtk_light_set_lux(int lux)
{
	spin_lock(&_light_lock);

	if (!_light_info)
		goto unlock;

	_light_info->lux = lux;

unlock:
	spin_unlock(&_light_lock);
}

static int _mtk_light_sample_task_func(void *param)
{
	int ret = 0;
	int val = 0;
	int timeout = 0;

	while (!kthread_should_stop()) {
		ret = mtk_light_get_iio_lux(&val);
		if (ret < 0) {
			pr_err("[mtk-light] mtk_light_get_iio_lux fail\n");
			wait_event_interruptible_timeout(_sample_wait,
				_sample_trigger,
				msecs_to_jiffies(SAMPLE_INTERVAL_MS));
			timeout++;
			if (timeout < SAMPLE_TIMEOUT)
				continue;
			else
				break;
		}

		mtk_light_set_lux(val);
		wait_event_interruptible_timeout(_sample_wait,
			_sample_trigger,
			msecs_to_jiffies(SAMPLE_INTERVAL_MS));
	}

	return 0;
}

static int _mtk_light_change_channel(struct device *dev, char *channel_name)
{
	struct iio_channel *channel = NULL;
	int max = 0, min = 0;
	int ret = 0;

	if (!channel_name)
		return -1;

	channel = devm_iio_channel_get(dev, channel_name);
	if (IS_ERR(channel)) {
		pr_err("[mtk-light] channel %s is not found! (%ld)\n",
			channel_name, PTR_ERR(channel));
		if (PTR_ERR(channel) == -ENODEV) {
			ret = -EPROBE_DEFER;
			return ret;
		}
		ret = PTR_ERR(channel);
		return ret;
	}

	if (iio_read_channel_scale(channel, &max, &min))
		pr_warn("[mtk-light] iio_read_channel_scale fail\n");

	spin_lock(&_light_lock);

	if (_light_info == NULL) {
		ret = -ENOMEM;
		goto unlock;
	}

	_light_info->channel = channel;
	_light_info->channel_name = channel_name;
	_light_info->range.min = min;
	_light_info->range.max = max;

	pr_info("min = %d, max = %d\n", _light_info->range.min, _light_info->range.max);

unlock:
	spin_unlock(&_light_lock);
	return 0;
}

static int mtk_light_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = NULL;
	const char *default_channel = NULL;
	int ret;

	spin_lock_init(&_light_lock);

	if (dev == NULL)
		return -EINVAL;

	_light_info = devm_kzalloc(dev, sizeof(struct mtk_light_info), GFP_KERNEL);
	if (_light_info == NULL)
		return -ENOMEM;

	node = dev->of_node;
	of_property_read_string(node, "default-channel", &default_channel);

	ret = _mtk_light_change_channel(dev, (char *)default_channel);
	if (ret < 0)
		return ret;

	_mtk_light_sample_task = kthread_run(_mtk_light_sample_task_func, NULL, "mtk_light_task");
	if (IS_ERR(_mtk_light_sample_task)) {
		pr_err("[mtk-light] Create task fail\n");
		return PTR_ERR(_mtk_light_sample_task);
	}

	ret = sysfs_create_group(&dev->kobj, &mtk_light_attr_group);
	if (ret)
		pr_warn("[mtk-light] Fail to create sysfs files: %d\n", ret);

	return 0;
}

static int mtk_light_remove(struct platform_device *pdev)
{
	if (!IS_ERR(_mtk_light_sample_task))
		kthread_stop(_mtk_light_sample_task);
	return 0;
}

static const struct of_device_id mtk_light_of_match[] = {
	{ .compatible = "mediatek,light", },
	{ }
};
MODULE_DEVICE_TABLE(of, mtk_light_of_match);

static struct platform_driver mtk_light_driver = {
	.driver = {
		.name = "mtk-light",
		.of_match_table = mtk_light_of_match,
	},
	.probe = mtk_light_probe,
	.remove = mtk_light_remove,
};

static int mtk_light_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_light_driver);
	if (ret < 0) {
		pr_err("Failed to register %s driver: %d\n",
		mtk_light_driver.driver.name, ret);
		goto err;
	}

	return 0;

err:
	platform_driver_unregister(&mtk_light_driver);
	return ret;
}

static void mtk_light_deinit(void)
{
	platform_driver_unregister(&mtk_light_driver);
}

module_init(mtk_light_init);
module_exit(mtk_light_deinit);

MODULE_AUTHOR("Steve Chen <steve-jt.chen@mediatek.com>");
MODULE_DESCRIPTION("Mediatek light sensor");
MODULE_LICENSE("GPL v2");
