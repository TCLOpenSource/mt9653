// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/extcon-provider.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/gpio.h>

#include <linux/delay.h>
#include <linux/kthread.h>

static struct task_struct *gpio_poll_tsk;
static const int POLL_SLEEP_MS = 100; // poll interval = 100 ms;

/**
 * struct gpio_extcon_data - A simple GPIO-controlled extcon device state container.
 * @edev:		Extcon device.
 * @work:		Work fired by the interrupt.
 * @debounce_jiffies:	Number of jiffies to wait for the GPIO to stabilize, from the debounce
 *			value.
 * @gpiod:		GPIO descriptor for this external connector.
 * @extcon_id:		The unique id of specific external connector.
 * @state:		Headset state
 * @hs_test_state: Headset test state for Vts.
 * @enable_debug_log: Enable log dynamically.
 * @dev:		device.
 */

struct gpio_extcon_data {
	struct extcon_dev *edev;
	struct delayed_work work;
	unsigned long debounce_jiffies;
	struct gpio_desc *gpiod;
	unsigned int extcon_id;
	int state;
	int hs_test_state;
	bool hs_debug_test;
	bool enable_debug_log;
	struct device *dev;
};

static void gpio_extcon_work(struct work_struct *work)
{
	int state;
	struct gpio_extcon_data	*data =
		container_of(to_delayed_work(work), struct gpio_extcon_data,
			     work);

	if (!data->hs_debug_test) {
		state = gpiod_get_value_cansleep(data->gpiod);
		data->state = state;
		if (data->enable_debug_log) {
			dev_info(data->dev, "%s(): state = %d\n",
						__func__, state);
		}
		extcon_set_state_sync(data->edev, data->extcon_id, state);
	} else {
		if (data->enable_debug_log) {
			dev_info(data->dev, "%s(): hs test state = %d\n",
						__func__, data->hs_test_state);
		}
		extcon_set_state_sync(data->edev, data->extcon_id, data->hs_test_state);
		data->hs_debug_test = false;
	}
}

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	struct gpio_extcon_data *data = dev_id;

	if (data->enable_debug_log)
		dev_info(data->dev, "%s()\n", __func__);

	queue_delayed_work(system_power_efficient_wq, &data->work,
			      data->debounce_jiffies);
	return IRQ_HANDLED;
}

static int gpio_poll(void *arg)
{
	struct gpio_extcon_data *extcon_data =
							(struct gpio_extcon_data *)arg;
	int state;

	while (true) {
		if (kthread_should_stop()) { // if kthread_stop() is called
			break;
		}

		msleep(POLL_SLEEP_MS);

		state = gpiod_get_value_cansleep(extcon_data->gpiod);

		if (extcon_data->state != state) {
			if (extcon_data->enable_debug_log) {
				dev_info(extcon_data->dev,
					"%s(): state = %d, extcon state %d\n",
					__func__, state, extcon_data->state);
			}
			queue_delayed_work(system_power_efficient_wq,
								&extcon_data->work,
								extcon_data->debounce_jiffies);
		}
	}

	return 0;
}

static ssize_t help_show(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	int ret = 0;

	if (dev == NULL) {
		goto exit;
	}

	if (attr == NULL) {
		dev_err(dev, "%s(%d): attr is null\n", __func__, __LINE__);
		goto exit;
	}

	if (buf == NULL) {
		dev_err(dev, "%s(%d): buf is null\n", __func__, __LINE__);
		goto exit;
	}

	return snprintf(buf, PAGE_SIZE, "Debug Help:\n"
		"- hs_test\n"
		"	<W>[state]: Set headset state.\n"
		"	(0:set headset unplug, 1:set headset plug)\n"
		"	<R>: Read headset test state.\n"
		"- hs_state\n"
		"	<R>: Read headset real state via gpio.\n"
		"- hs_enalbe_log\n"
		"	<W>[enable]: enable headset driver log.\n"
		"	(0:disable log, 1:enable log)\n"
		"	<R>: Read enable headset log or not.\n");
exit:
	return ret;
}

static ssize_t hs_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gpio_extcon_data *data = NULL;
	int ret = 0, state = 0;

	if (dev == NULL) {
		goto exit;
	}

	if (attr == NULL) {
		dev_err(dev, "%s(%d): attr is null\n", __func__, __LINE__);
		goto exit;
	}

	if (buf == NULL) {
		dev_err(dev, "%s(%d): buf is null\n", __func__, __LINE__);
		goto exit;
	}

	data = dev_get_drvdata(dev);
	if (data == NULL) {
		dev_err(dev, "%s: data is null\n", __func__);
		goto exit;
	}

	state = gpiod_get_value_cansleep(data->gpiod);
	if (data->enable_debug_log) {
		dev_info(dev, "%s(): state %d\n",
					__func__, state);
	}

	return snprintf(buf, PAGE_SIZE, "Headset Real State(from GPIO): %s\n",
					(state == 0) ? "Un-plug" : "Plug");
exit:
	return ret;
}

static ssize_t hs_test_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	struct gpio_extcon_data *data = NULL;
	int ret = 0, state = 0;

	if (dev == NULL) {
		goto exit;
	}

	if (attr == NULL) {
		dev_err(dev, "%s(%d): attr is null\n", __func__, __LINE__);
		goto exit;
	}

	if (buf == NULL) {
		dev_err(dev, "%s(%d): buf is null\n", __func__, __LINE__);
		goto exit;
	}

	data = dev_get_drvdata(dev);
	if (data == NULL) {
		dev_err(dev, "%s(%d): data is null\n", __func__, __LINE__);
		goto exit;
	}

	ret = kstrtoint(buf, 10, &state);
	if (data->enable_debug_log) {
		dev_info(dev, "%s(): ret %d, test state = %d\n",
					__func__, ret, state);
	}

	if (ret == 0) {
		if ((state != 0) && (state != 1)) {
			dev_err(dev, "%s(%d): You should enter 0 or 1 !!!\n",
						__func__, __LINE__);
		} else {
			if (data->enable_debug_log) {
				dev_info(dev, "%s(): hs test state = %d -> %d\n",
						__func__, data->hs_test_state, state);
			}
			data->hs_test_state = state;
			data->hs_debug_test = true;
			queue_delayed_work(system_power_efficient_wq, &data->work,
								data->debounce_jiffies);
			ret = count;
		}
	} else {
		dev_err(dev, "%s(%d): kstrtoint failed, ret = %d\n",
					__func__, __LINE__, ret);
	}

exit:
	return ret;
}

static ssize_t hs_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gpio_extcon_data *data = NULL;
	int ret = 0, state = 0;

	if (dev == NULL) {
		goto exit;
	}

	if (attr == NULL) {
		dev_err(dev, "%s(%d): attr is null\n", __func__, __LINE__);
		goto exit;
	}

	if (buf == NULL) {
		dev_err(dev, "%s(%d): buf is null\n", __func__, __LINE__);
		goto exit;
	}

	data = dev_get_drvdata(dev);
	if (data == NULL) {
		dev_err(dev, "%s: data is null\n", __func__);
		goto exit;
	}

	state = data->hs_test_state;

	return snprintf(buf, PAGE_SIZE, "Headset Test state = %s\n",
		(state == 0) ? "Un-plug" : "Plug");
exit:
	return ret;
}

static ssize_t hs_enable_log_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	struct gpio_extcon_data *data = NULL;
	int ret = 0, enable = 0;

	if (dev == NULL) {
		goto exit;
	}

	if (attr == NULL) {
		dev_err(dev, "%s(%d): attr is null\n", __func__, __LINE__);
		goto exit;
	}

	if (buf == NULL) {
		dev_err(dev, "%s(%d): buf is null\n", __func__, __LINE__);
		goto exit;
	}

	data = dev_get_drvdata(dev);
	if (data == NULL) {
		dev_err(dev, "%s(%d): data is null\n", __func__, __LINE__);
		goto exit;
	}

	ret = kstrtoint(buf, 10, &enable);
	if (ret == 0) {
		if ((enable != 0) && (enable != 1)) {
			dev_err(dev, "%s: enable(%d) is ng. Pls enter 0 or 1\n",
						__func__, enable);
		} else {
			data->enable_debug_log = enable;
			if (data->enable_debug_log) {
				dev_info(dev, "%s(): enable = %d\n",
					__func__, enable);
			}
			ret = count;
		}
	} else {
		dev_err(dev, "%s(%d): kstrtoint failed, ret = %d\n",
				__func__, __LINE__, ret);
	}
exit:
	return ret;
}

static ssize_t hs_enable_log_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gpio_extcon_data *data = NULL;
	int ret = 0, enable = 0;

	if (dev == NULL) {
		goto exit;
	}

	if (attr == NULL) {
		dev_err(dev, "%s(%d): attr is null\n", __func__, __LINE__);
		goto exit;
	}

	if (buf == NULL) {
		dev_err(dev, "%s(%d): buf is null\n", __func__, __LINE__);
		goto exit;
	}

	data = dev_get_drvdata(dev);
	if (data == NULL) {
		dev_err(dev, "%s: data is null\n", __func__);
		goto exit;
	}

	enable = data->enable_debug_log;

	return snprintf(buf, PAGE_SIZE, "Headset enable log = %d\n",
								enable);
exit:
	return ret;
}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RO(hs_state);
static DEVICE_ATTR_RW(hs_test);
static DEVICE_ATTR_RW(hs_enable_log);

static struct attribute *headset_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_hs_state.attr,
	&dev_attr_hs_test.attr,
	&dev_attr_hs_enable_log.attr,
	NULL,
};

static const struct attribute_group headset_attr_group = {
	.name = "mtk_dbg",
	.attrs = headset_attrs,
};

static const struct attribute_group *headset_attr_groups[] = {
	&headset_attr_group,
	NULL,
};

static int mtk_headset_extcon_probe(struct platform_device *pdev)
{
	struct gpio_extcon_data *data;
	struct device *dev = &pdev->dev;
	unsigned long irq_flags;
	int irq;
	int ret;
	struct device_node *dn;
	u32 dn_extcon_id;

	data = devm_kzalloc(dev, sizeof(struct gpio_extcon_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/*
	 * FIXME: extcon_id represents the unique identifier of external
	 * connectors such as EXTCON_USB, EXTCON_DISP_HDMI and so on. extcon_id
	 * is necessary to register the extcon device. But, it's not yet
	 * developed to get the extcon id from device-tree or others.
	 * On later, it have to be solved.
	 */
	if (data->extcon_id > EXTCON_NONE)
		return -EINVAL;

	dn = pdev->dev.of_node;
	if (dn) {
		if (of_property_read_u32(dn, "extcon-id", &dn_extcon_id) != 0) {
			dev_err(dev, "%s: can not read extcon-id\n", __func__);
			return -EINVAL;
		}
		data->extcon_id = dn_extcon_id;
	} else {
		dev_err(dev, "%s: dn is null\n", __func__);
		return -EINVAL;
	}

	data->gpiod = devm_gpiod_get(dev, "extcon", GPIOD_IN);
	if (IS_ERR(data->gpiod))
		return PTR_ERR(data->gpiod);

	irq = gpiod_to_irq(data->gpiod);
	if (irq <= 0)
		return irq;

	/*
	 * It is unlikely that this is an acknowledged interrupt that goes
	 * away after handling, what we are looking for are falling edges
	 * if the signal is active low, and rising edges if the signal is
	 * active high.
	 */
	if (gpiod_is_active_low(data->gpiod))
		irq_flags = IRQF_TRIGGER_FALLING;
	else
		irq_flags = IRQF_TRIGGER_RISING;

	/* Allocate the memory of extcon devie and register extcon device */
	data->edev = devm_extcon_dev_allocate(dev, &data->extcon_id);
	if (IS_ERR(data->edev)) {
		dev_err(dev, "failed to allocate extcon device\n");
		return -ENOMEM;
	}

	ret = devm_extcon_dev_register(dev, data->edev);
	if (ret < 0)
		return ret;

	INIT_DELAYED_WORK(&data->work, gpio_extcon_work);

	/*
	 * Request the interrupt of gpio to detect whether external connector
	 * is attached or detached.
	 */
	ret = devm_request_any_context_irq(dev, irq,
					gpio_irq_handler, irq_flags,
					pdev->name, data);
	if (ret < 0)
		return ret;

	data->hs_test_state = 0;
	data->hs_debug_test = false;
	data->enable_debug_log = false;
	data->dev = dev;

	platform_set_drvdata(pdev, data);
	/* Perform initial detection */
	gpio_extcon_work(&data->work.work);

	if (gpio_poll_tsk == NULL) {
		gpio_poll_tsk = kthread_create(gpio_poll, data, "GPIO poll Task");
		if (IS_ERR(gpio_poll_tsk)) {
			dev_err(dev, "%s: create kthread for GPIO poll Task fail\n", __func__);
			return -ENOMEM;
		}
		wake_up_process(gpio_poll_tsk);
	} else {
		dev_err(dev, "%s: gpio_poll_tsk is already created\n", __func__);
		return 0;
	}

	/* Create device attribute files */
	ret = sysfs_create_groups(&dev->kobj, headset_attr_groups);
	if (ret)
		dev_err(dev, "%s: Audio Headset detection Sysfs init fail\n", __func__);

	return 0;
}

static int mtk_headset_extcon_remove(struct platform_device *pdev)
{
	struct gpio_extcon_data *data = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	kthread_stop(gpio_poll_tsk);
	gpio_poll_tsk = NULL;

	cancel_delayed_work_sync(&data->work);

	if (dev != NULL)
		sysfs_remove_groups(&dev->kobj, headset_attr_groups);

	return 0;
}

static const struct of_device_id mtk_headset_extcon_of_match[] = {
	{ .compatible = "mediatek,extcon-headset", },
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_headset_extcon_of_match);

static struct platform_driver mtk_headset_extcon_driver = {
	.probe		= mtk_headset_extcon_probe,
	.remove		= mtk_headset_extcon_remove,
	.driver		= {
		.name	= "mtk-extcon-headset",
		.of_match_table = mtk_headset_extcon_of_match,
	},
};
module_platform_driver(mtk_headset_extcon_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Extcon HEADSET Driver");
