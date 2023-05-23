// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
 * Author Tsung-Hsien Chiang <Tsung-Hsien.Chiang@mediatek.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/sysfs.h>

#define MIC_MUTE_DRV_COMPATIBLE	"mediatek,mic-mute"

struct workqueue_struct *mwq;

/**
 * struct mic_mute_button - configuration parameters
 * @code:		input event code (KEY_*, SW_*)
 * @gpio:		%-1 if this key does not support gpio
 * @active_low:		%true indicates that button is considered
 *			depressed when gpio is low
 * @desc:		label that will be attached to button's gpio
 * @type:		input event type (%EV_KEY, %EV_SW, %EV_ABS)
 * @wakeup:		configure the button as a wake-up source
 * @wakeup_event_action:	event action to trigger wakeup
 * @debounce_interval:	debounce ticks interval in msecs
 * @can_disable:	%true indicates that userspace is allowed to
 *			disable button via sysfs
 * @value:		axis value for %EV_ABS
 * @irq:		Irq number in case of interrupt keys
 */
struct mic_mute_button {
	unsigned int code;
	int gpio;
	int active_low;
	const char *desc;
	unsigned int type;
	int wakeup;
	int wakeup_event_action;
	int debounce_interval;
	bool can_disable;
	int value;
	unsigned int irq;
};

/**
 * struct mic_mute_platform_data - platform data for mic_mute driver
 * @buttons:		pointer to array of &mic_mute_button structures
 *			describing buttons attached to the device
 * @nbuttons:		number of elements in @buttons array
 * @poll_interval:	polling interval in msecs - for polling driver only
 * @rep:		enable input subsystem auto repeat
 * @enable:		platform hook for enabling the device
 * @disable:		platform hook for disabling the device
 * @name:		input device name
 */
struct mic_mute_platform_data {
	const struct mic_mute_button *buttons;
	int nbuttons;
	unsigned int poll_interval;
	unsigned int rep:1;
	int (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	const char *name;
};

struct gpio_button_data {
	const struct mic_mute_button *button;
	struct input_dev *input;
	struct gpio_desc *gpiod;

	unsigned short *code;

	struct delayed_work work;

	spinlock_t lock;

};

struct mic_mute_drvdata {
	const struct mic_mute_platform_data *pdata;
	struct input_dev *input;
	struct mutex disable_lock;
	unsigned short *keymap;
	struct gpio_button_data data[0];
};


static void mic_mute_gpio_report_event(struct gpio_button_data *bdata)
{
	const struct mic_mute_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned int type = button->type ?: EV_KEY;
	int state;

	state = gpiod_get_value_cansleep(bdata->gpiod);
	if (state < 0) {
		dev_err(input->dev.parent,
			"failed to get gpio state: %d\n", state);
		return;
	}
	//printk(KERN_ALERT" mic_mute GPIO state =%d \n",state);//For test

	input_event(input, type, *bdata->code, state);
	input_sync(input);
}

static void mic_mute_gpio_work_func(struct work_struct *work)
{
	struct gpio_button_data *bdata =
		container_of(work, struct gpio_button_data, work.work);

	mic_mute_gpio_report_event(bdata);
	queue_delayed_work(mwq, &bdata->work, 200);
}

static int mic_mute_setup_key(struct platform_device *pdev,
				struct input_dev *input,
				struct mic_mute_drvdata *ddata,
				const struct mic_mute_button *button,
				int idx,
				struct fwnode_handle *child)
{
	const char *desc = button->desc ? button->desc : "mic_mute";
	struct device *dev = &pdev->dev;
	struct gpio_button_data *bdata = &ddata->data[idx];
	irq_handler_t isr;
	unsigned long irqflags;
	int irq;
	int error;

	bdata->input = input;
	bdata->button = button;
		spin_lock_init(&bdata->lock);

	if (child) {
		bdata->gpiod = devm_fwnode_get_gpiod_from_child(dev, NULL,
								child,
								GPIOD_IN,
								desc);
		if (IS_ERR(bdata->gpiod)) {
			error = PTR_ERR(bdata->gpiod);
			if (error == -ENOENT) {
				/*
				 * GPIO is optional, we may be dealing with
				 * purely interrupt-driven setup.
				 */
				bdata->gpiod = NULL;
			} else {
				if (error != -EPROBE_DEFER)
					dev_err(dev, "failed to get gpio: %d\n",
						error);
				return error;
			}
		}
	}
	mwq = create_workqueue("mic_mute workqueue");
	if (!mwq) {
		dev_err(dev, "Create workqueue failed!\n");
		return 1;
	}
	bdata->code = &ddata->keymap[idx];
	*bdata->code = button->code;
	input_set_capability(input, button->type ?: EV_KEY, *bdata->code);
		INIT_DELAYED_WORK(&bdata->work, mic_mute_gpio_work_func);

	queue_delayed_work(mwq, &bdata->work, 200);
	return 0;

}

static void mic_mute_report_state(struct mic_mute_drvdata *ddata)
{
	struct input_dev *input = ddata->input;
	int i;

	for (i = 0; i < ddata->pdata->nbuttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];
		if (bdata->gpiod)
			mic_mute_gpio_report_event(bdata);
	}
	input_sync(input);
}

static int mic_mute_open(struct input_dev *input)
{
	struct mic_mute_drvdata *ddata = input_get_drvdata(input);
	const struct mic_mute_platform_data *pdata = ddata->pdata;
	int error;

	if (pdata->enable) {
		error = pdata->enable(input->dev.parent);
		if (error)
			return error;
	}

	/* Report current state of buttons that are connected to GPIOs */
	mic_mute_report_state(ddata);

	return 0;
}

static void mic_mute_close(struct input_dev *input)
{
	struct mic_mute_drvdata *ddata = input_get_drvdata(input);
	const struct mic_mute_platform_data *pdata = ddata->pdata;

	if (pdata->disable)
		pdata->disable(input->dev.parent);
}

/*
 * Translate properties into platform_data
 */
static struct mic_mute_platform_data *
mic_mute_get_devtree_pdata(struct device *dev)
{
	struct mic_mute_platform_data *pdata;
	struct mic_mute_button *button;
	struct fwnode_handle *child;
	int nbuttons;

	nbuttons = device_get_child_node_count(dev);
	if (nbuttons == 0)
		return ERR_PTR(-ENODEV);

	pdata = devm_kzalloc(dev,
			     sizeof(*pdata) + nbuttons * sizeof(*button),
			     GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	button = (struct mic_mute_button *)(pdata + 1);

	pdata->buttons = button;
	pdata->nbuttons = nbuttons;

	pdata->rep = device_property_read_bool(dev, "autorepeat");

	device_property_read_string(dev, "label", &pdata->name);

	device_for_each_child_node(dev, child) {
		if (is_of_node(child))
			button->irq =
				irq_of_parse_and_map(to_of_node(child), 0);

		if (fwnode_property_read_u32(child, "linux,code",
					     &button->code)) {
			dev_err(dev, "Button without keycode\n");
			fwnode_handle_put(child);
			return ERR_PTR(-EINVAL);
		}

		fwnode_property_read_string(child, "label", &button->desc);

		if (fwnode_property_read_u32(child, "linux,input-type",
					     &button->type))
			button->type = EV_KEY;

		button->wakeup =
			fwnode_property_read_bool(child, "wakeup-source") ||
			/* legacy name */
			fwnode_property_read_bool(child, "gpio-key,wakeup");

		fwnode_property_read_u32(child, "wakeup-event-action",
					 &button->wakeup_event_action);

		button->can_disable =
			fwnode_property_read_bool(child, "linux,can-disable");

		if (fwnode_property_read_u32(child, "debounce-interval",
					 &button->debounce_interval))
			button->debounce_interval = 5;

		button++;
	}

	return pdata;
}

static const struct of_device_id mic_mute_of_match[] = {
	{ .compatible = MIC_MUTE_DRV_COMPATIBLE, },
	{ },
};
MODULE_DEVICE_TABLE(of, mic_mute_of_match);

static int mic_mute_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct mic_mute_platform_data *pdata = dev_get_platdata(dev);
	struct fwnode_handle *child = NULL;
	struct mic_mute_drvdata *ddata;
	struct input_dev *input;
	int i, error;
	int wakeup = 0;

	if (!pdata) {
		pdata = mic_mute_get_devtree_pdata(dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	}

	ddata = devm_kzalloc(dev, struct_size(ddata, data, pdata->nbuttons),
			     GFP_KERNEL);
	if (!ddata) {
		printk(KERN_ALERT "failed to allocate state\n");
		return -ENOMEM;
	}

	ddata->keymap = devm_kcalloc(dev,
				     pdata->nbuttons, sizeof(ddata->keymap[0]),
				     GFP_KERNEL);
	if (!ddata->keymap)
		return -ENOMEM;

	input = devm_input_allocate_device(dev);
	if (!input) {
		dev_err(dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	ddata->pdata = pdata;
	ddata->input = input;
	mutex_init(&ddata->disable_lock);

	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = pdata->name ? : pdev->name;
	input->phys = "mic_mute/input0";
	input->dev.parent = dev;
	input->open = mic_mute_open;
	input->close = mic_mute_close;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	input->keycode = ddata->keymap;
	input->keycodesize = sizeof(ddata->keymap[0]);
	input->keycodemax = pdata->nbuttons;

	/* Enable auto repeat feature of Linux input subsystem */
	if (pdata->rep)
		__set_bit(EV_REP, input->evbit);

	for (i = 0; i < pdata->nbuttons; i++) {
		const struct mic_mute_button *button = &pdata->buttons[i];

		if (!dev_get_platdata(dev)) {
			child = device_get_next_child_node(dev, child);
			if (!child) {
				dev_err(dev,
					"missing child device node for entry %d\n",
					i);
				return -EINVAL;
			}
		}

		error = mic_mute_setup_key(pdev, input, ddata,
					    button, i, child);
		if (error) {
			fwnode_handle_put(child);
			return error;
		}

		if (button->wakeup)
			wakeup = 1;
	}

	fwnode_handle_put(child);

	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		return error;
	}

	device_init_wakeup(dev, wakeup);

	return 0;
}

#ifdef CONFIG_PM
static int mic_mute_suspend(struct device *dev)
{
	struct mic_mute_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;
	int ret = 0;

	//pm_set_wakeup_config("mute-gpio", true);
	dev_info(dev, "##### MIC_MUTE Driver Suspend #####\n");

	return 0;
}

static int mic_mute_resume(struct device *dev)
{
	struct mic_mute_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;
	int ret = 0;

	dev_info(dev, "##### MIC_MUTE Driver Resume #####\n");

	return 0;
}

static const struct dev_pm_ops mtk_mic_mute_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mic_mute_suspend, mic_mute_resume)
};

#endif

static int mic_mute_remove(struct platform_device *pdev)
{
	int ret = 0;

	return ret;
}

static struct platform_driver mic_mute_device_driver = {
	.probe		= mic_mute_probe,
	.remove		= mic_mute_remove,
	.driver		= {
		.name	= "mic_mute",
		.pm	= &mtk_mic_mute_pm_ops,
		.of_match_table = mic_mute_of_match,
	}
};

static int __init mic_mute_init(void)
{
	return platform_driver_register(&mic_mute_device_driver);
}

static void __exit mic_mute_exit(void)
{
	platform_driver_unregister(&mic_mute_device_driver);
}

late_initcall(mic_mute_init);
module_exit(mic_mute_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tsung-Hsien.Chiang@mediatek.com");
MODULE_DESCRIPTION("Mic mute driver for GPIOs");
MODULE_ALIAS("platform:mic_mute");
