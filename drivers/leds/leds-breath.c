// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/leds_breath.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/completion.h>
#include <linux/mailbox_client.h>


struct gpio_led_data {
	struct led_classdev cdev;
	struct gpio_desc *gpiod;
};


/* A list of pmpwm_chip_dev */
static LIST_HEAD(led_chip_dev_list);
/* A list of rpmsg_dev */
static LIST_HEAD(rpmsg_dev_list);

static DEFINE_MUTEX(rpmsg_led_bind_lock);

/**
 * LED bus device and rpmsg device binder
 */
static void _mtk_bind_rpmsg_led(struct mtk_rpmsg_led *rpmsg_dev,
				struct mtk_rproc_led_bus_dev *led_dev)
{
	mutex_lock(&rpmsg_led_bind_lock);

	if (rpmsg_dev) {
		struct mtk_rproc_led_bus_dev *led_device, *n;

		list_for_each_entry_safe(led_device, n, &led_chip_dev_list,
					 list) {
			if (led_device->rpmsg_dev == NULL) {
				led_device->rpmsg_dev = rpmsg_dev;
				rpmsg_dev->led_dev = led_device;
				complete(&led_device->rpmsg_bind);
				break;
			}
		}
	} else {
		struct mtk_rpmsg_led *rpmsg_device, *n;

		list_for_each_entry_safe(rpmsg_device, n, &rpmsg_dev_list, list) {
			if (rpmsg_device->led_dev == NULL) {
				rpmsg_device->led_dev = led_dev;
				led_dev->rpmsg_dev = rpmsg_device;
				complete(&led_dev->rpmsg_bind);
				break;
			}
		}
	}

	mutex_unlock(&rpmsg_led_bind_lock);
}


static inline struct gpio_led_data *
			cdev_to_gpio_led_data(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct gpio_led_data, cdev);
}

static int create_gpio_led(const struct gpio_led *template,
	struct gpio_led_data *led_dat, struct device *parent,
	struct fwnode_handle *fwnode, gpio_blink_set_t blink_set)
{
	struct led_init_data init_data = {};
	int ret, state;

	led_dat->cdev.default_trigger = template->default_trigger;

	state = (template->default_state == LEDS_GPIO_DEFSTATE_ON);
	led_dat->cdev.brightness = state ? LED_FULL : LED_OFF;
	if (!template->retain_state_suspended)
		led_dat->cdev.flags |= LED_CORE_SUSPENDRESUME;
	if (template->panic_indicator)
		led_dat->cdev.flags |= LED_PANIC_INDICATOR;
	if (template->retain_state_shutdown)
		led_dat->cdev.flags |= LED_RETAIN_AT_SHUTDOWN;

	if (template->name) {
		led_dat->cdev.name = template->name;
		ret = devm_led_classdev_register(parent, &led_dat->cdev);
	} else {
		init_data.fwnode = fwnode;
		ret = devm_led_classdev_register_ext(parent, &led_dat->cdev,
								&init_data);
	}

	return ret;
}

struct gpio_leds_priv {
	struct mbox_client mbox_client;
	struct mbox_chan *mchan;
	int num_leds;
	struct gpio_led_data leds[];
};

static inline int sizeof_gpio_leds_priv(int num_leds)
{
	return sizeof(struct gpio_leds_priv) +
		(sizeof(struct gpio_led_data) * num_leds);
}

static struct gpio_leds_priv *gpio_leds_create(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;
	struct gpio_leds_priv *priv;
	int count, ret;

	count = device_get_child_node_count(dev);
	if (!count)
		return ERR_PTR(-ENODEV);

	priv = devm_kzalloc(dev, sizeof_gpio_leds_priv(count), GFP_KERNEL);
	if (!priv)
		return ERR_PTR(-ENOMEM);

	device_for_each_child_node(dev, child) {
		struct gpio_led_data *led_dat = &priv->leds[priv->num_leds];
		struct gpio_led led = {};
		const char *state = NULL;

		/*
		 * Acquire gpiod from DT with uninitialized label, which
		 * will be updated after LED class device is registered,
		 * Only then the final LED name is known.
		 */
		led.gpiod = devm_fwnode_get_gpiod_from_child(dev, NULL, child,
					GPIOD_ASIS, NULL);
		if (IS_ERR(led.gpiod)) {
			fwnode_handle_put(child);
			return ERR_CAST(led.gpiod);
		}

		led_dat->gpiod = led.gpiod;

		fwnode_property_read_string(child,
					"linux,default-trigger",
					&led.default_trigger);

		if (!fwnode_property_read_string(child,
					"default-state", &state)) {
			if (!strcmp(state, "keep"))
				led.default_state = LEDS_GPIO_DEFSTATE_KEEP;
			else if (!strcmp(state, "on"))
				led.default_state = LEDS_GPIO_DEFSTATE_ON;
			else
				led.default_state = LEDS_GPIO_DEFSTATE_OFF;
		}

		if (fwnode_property_present(child, "retain-state-suspended"))
			led.retain_state_suspended = 1;
		if (fwnode_property_present(child, "retain-state-shutdown"))
			led.retain_state_shutdown = 1;
		if (fwnode_property_present(child, "panic-indicator"))
			led.panic_indicator = 1;
		ret = create_gpio_led(&led, led_dat, dev, child, NULL);
		if (ret < 0) {
			fwnode_handle_put(child);
			return ERR_PTR(ret);
		}
		/* Set gpiod label to match the corresponding LED name. */
		gpiod_set_consumer_name(led_dat->gpiod,
		led_dat->cdev.dev->kobj.name);
		priv->num_leds++;
	}

	return priv;
}

static const struct of_device_id of_breath_leds_match[] = {
	{ .compatible = "breath-leds", },
	{},
};

MODULE_DEVICE_TABLE(of, of_breath_leds_match);

static int breath_led_probe(struct platform_device *pdev)
{
	//struct gpio_led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct gpio_leds_priv *priv;
	struct mtk_rproc_led_bus_dev *led_bus_dev;

	led_bus_dev = devm_kzalloc(&pdev->dev,
		sizeof(struct mtk_rproc_led_bus_dev), GFP_KERNEL);
	if (led_bus_dev == NULL)
		return -ENOMEM;
	priv = gpio_leds_create(pdev);
	if (IS_ERR(priv))
		return PTR_ERR(priv);
	priv->mbox_client.dev = &pdev->dev;
	priv->mbox_client.tx_block = true;
	priv->mbox_client.tx_tout = 5000;
	priv->mbox_client.knows_txdone = false;
	priv->mbox_client.rx_callback = mbox_cb;
	priv->mbox_client.tx_prepare = NULL;
	priv->mbox_client.tx_done = NULL;
	priv->mchan = mbox_request_channel(&priv->mbox_client, 0);
	if (IS_ERR(priv->mchan)) {
		dev_err(&pdev->dev, "IPC Request for ARM to CM4 Channel failed! %ld\n",
			PTR_ERR(priv->mchan));
		return PTR_ERR(priv->mchan);
	}
	platform_set_drvdata(pdev, priv);
	led_bus_dev->gpio = desc_to_gpio(priv->leds[0].gpiod);
    led_bus_dev->mchan = priv->mchan;

	// add to led_bus_dev list
	INIT_LIST_HEAD(&led_bus_dev->list);
	list_add_tail(&led_bus_dev->list, &led_chip_dev_list);

	init_completion(&led_bus_dev->rpmsg_bind);
	_mtk_bind_rpmsg_led(NULL, led_bus_dev);

	return 0;
}


void mbox_cb(struct mbox_client *cl, void *mssg)
{
	;
}


/**
 * RPMSG functions
 * todo:add led device select
 * only support one breath led now
 */
int mtk_rpmsg_led_txn(void *data, size_t len)
{
	struct mtk_rproc_led_bus_dev *led_bus_dev = NULL, *n;

	list_for_each_entry_safe(led_bus_dev, n, &led_chip_dev_list, list) {
		if (led_bus_dev->mchan != NULL) { //find the first led device
			break;
		}
	}

	if (!led_bus_dev->mchan) {
		pr_err("%s led dev is NULL\n", __func__);
		return -EINVAL;
	}

	pr_err("led_txn 0x%x\n",mbox_send_message(led_bus_dev->mchan, data));
	return 0;
}
EXPORT_SYMBOL(mtk_rpmsg_led_txn);
static int mtk_rpmsg_led_callback(struct rpmsg_device *rpdev,
	void *data, int len, void *priv, u32 src)
{
	struct mtk_rpmsg_led *rpmsg_led;

	if (!rpdev) {
		pr_err("%s rpdev is NULL\n", __func__);
		return -EINVAL;
	}

	rpmsg_led = dev_get_drvdata(&rpdev->dev);
	if (!rpmsg_led) {
		pr_err("%s private data is NULL\n", __func__);
		return -EINVAL;
	}

	if (!data || len == 0) {
		rpmsg_log(rpmsg_led, MTK_LED_LOG_ERR,
			"Invalid data or length from src:%d\n", src);
		return -EINVAL;
	}
#ifdef DEBUG
	pr_info("%s:\n", __func__);
	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET,
		DUMP_ROW_BYTES, 1, data, len, true);
#endif
	if (len > sizeof(struct led_proxy_ack_msg)) {
		rpmsg_log(rpmsg_led, MTK_LED_LOG_ERR,
			"Receive data length %d over size\n", len);
		rpmsg_led->txn_result = -E2BIG;
	} else {
		rpmsg_log(rpmsg_led, MTK_LED_LOG_DEBUG,
			"Got ack for cmd %d\n", *((u8 *)data));

		// store received data for transaction checking
		memcpy(&rpmsg_led->receive_msg, data,
			sizeof(struct led_proxy_ack_msg));
		rpmsg_led->txn_result = 0;
	}

	complete(&rpmsg_led->ack);
	dev_info(rpmsg_led->dev, "%s return\n", __func__);
	return 0;
}

static int mtk_rpmsg_led_probe(struct rpmsg_device *rpdev)
{
	struct mtk_rpmsg_led *rpmsg_led;
	//struct led_proxy_cmd_msg gpio_msg;

	dev_info(&rpdev->dev, "%s, id name %s\n", __func__, rpdev->id.name);

	rpmsg_led = devm_kzalloc(&rpdev->dev,
		sizeof(struct mtk_rpmsg_led), GFP_KERNEL);
	if (!rpmsg_led)
		return -ENOMEM;

	rpmsg_led->rpdev = rpdev;
	rpmsg_led->dev = &rpdev->dev;
	// add to pmpwm_chip_dev list
	INIT_LIST_HEAD(&rpmsg_led->list);
	list_add_tail(&rpmsg_led->list, &rpmsg_dev_list);
	init_completion(&rpmsg_led->ack);
	rpmsg_led->txn = mtk_rpmsg_led_txn;

	dev_set_drvdata(&rpdev->dev, rpmsg_led);

	_mtk_bind_rpmsg_led(rpmsg_led, NULL);
	//gpio_msg.cmd = LED_CMD_SET_GPIO;
	//gpio_msg.payload[0] = rpmsg_led->led_dev->gpio;
	//rpmsg_led->txn(&gpio_msg, sizeof(gpio_msg));
	return 0;
}

static void mtk_rpmsg_led_remove(struct rpmsg_device *rpdev)
{
	struct mtk_rpmsg_led *rpmsg_led = dev_get_drvdata(&rpdev->dev);

	rpmsg_log(rpmsg_led, MTK_LED_LOG_INFO, "%s dev %s\n",
			__func__, rpmsg_led->rpdev->id.name);

	// unbind led adapter and rpmsg device
	if (rpmsg_led->led_dev == NULL)
		rpmsg_log(rpmsg_led, MTK_LED_LOG_INFO,
			"led device already unbinded\n");
	else {
		rpmsg_led->led_dev->rpmsg_dev = NULL;
		rpmsg_led->led_dev = NULL;
	}

	dev_set_drvdata(&rpdev->dev, NULL);
}

static struct rpmsg_device_id mtk_rpmsg_led_id_table[] = {
	// all aupported LED device names in dts
	{.name = "breath_led_0"},
	{},
};

MODULE_DEVICE_TABLE(rpmsg, mtk_rpmsg_led_id_table);

static struct rpmsg_driver mtk_rpmsg_led_driver = {
	.drv.name = KBUILD_MODNAME,
	.drv.owner = THIS_MODULE,
	.id_table = mtk_rpmsg_led_id_table,
	.probe = mtk_rpmsg_led_probe,
	.callback = mtk_rpmsg_led_callback,
	.remove = mtk_rpmsg_led_remove,
};

static struct platform_driver breath_led_driver = {
	.probe = breath_led_probe,
	.shutdown = NULL, // should no control led on arm
	.driver		= {
		.name = "leds-breath",
		.of_match_table = of_breath_leds_match,
	},
};
static int __init mtk_breath_led_init_driver(void)
{
	int ret = 0;

	ret = platform_driver_register(&breath_led_driver);
	if (ret) {
		pr_err("breath led driver register failed (%d)\n", ret);
		return ret;
	}
	ret = register_rpmsg_driver(&mtk_rpmsg_led_driver);
	if (ret) {
		pr_err("rpmsg driver register failed (%d)\n", ret);
		platform_driver_unregister(&breath_led_driver);
		return ret;
	}
	return ret;
}
module_init(mtk_breath_led_init_driver);
static void __exit mtk_breath_led_exit_driver(void)
{
	unregister_rpmsg_driver(&mtk_rpmsg_led_driver);
	platform_driver_unregister(&breath_led_driver);
}
module_exit(mtk_breath_led_exit_driver);

#define NODE_NAME rpmsg_txn
#define PREPH_PARAM_COUNT 10
#define PREPH_PARAM_LEN 50


// echo [cmd],[value]...
static int mt_preph_param_set(const char *val,
					const struct kernel_param *kp)
{
	char buf[PREPH_PARAM_LEN] = {'\0'};
	char *pbuf = &buf[0];
	char *tokens[PREPH_PARAM_COUNT] = {0};
	int i, ret = 0;
	size_t len = 0;
	u8 send_box[14] = {0};

	pr_err("%s\n", __func__);
	if (!val)
		return -EINVAL;

	len = strnlen(val, PREPH_PARAM_LEN);
	if (len == 0) {
		pr_warn("[LED]: empty parameter - ignored\n");
		return 0;
	}

	if (len == PREPH_PARAM_LEN) {
		pr_err("[LED]: parameter \"%s\" is too long, max. is %d\n",
							val, PREPH_PARAM_LEN);
		return -EINVAL;
	}
	strncpy(buf, val, len);
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	for (i = 0; i < PREPH_PARAM_COUNT; i++) {
		tokens[i] = strsep(&pbuf, ",");
		if (tokens[i] == NULL)
			break;
	}

	for (i = 0; i < PREPH_PARAM_COUNT; i++) {
		if (tokens[i] != NULL) {
				ret = kstrtoint(tokens[i], 0,
							(int *)&send_box[i]);
                pr_err("send_box[%d]:%d\n", i, (int )send_box[i]);
			if (ret < 0) {
				pr_err("ret < 0  tokens %d:%s\n", i, tokens[i]);
				return -EINVAL;
			}
		} else
			break;
	}
	mtk_rpmsg_led_txn(send_box, sizeof(send_box));

	return 0;
}

static const struct kernel_param_ops mt_preph_ops = {
	.set = mt_preph_param_set,
};
module_param_cb(NODE_NAME, &mt_preph_ops, NULL, 0644);



MODULE_AUTHOR("xxx <xxx@mediatek.com>");
MODULE_DESCRIPTION("BREATH LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:leds-breath");
