/* SPDX-License-Identifier: GPL-2.0 */
/*
 * PWM LED driver data - see drivers/leds/leds-breath.c
 */
#ifndef __LINUX_LEDS_BREATH_H
#define __LINUX_LEDS_BREATH_H
#include <linux/mailbox_client.h>



#define MTK_LED_LOG_NONE	(-1)	/* set mtk loglevel to be useless */
#define MTK_LED_LOG_MUTE	(0)
#define MTK_LED_LOG_ERR		(3)
#define MTK_LED_LOG_WARN	(4)
#define MTK_LED_LOG_INFO	(6)
#define MTK_LED_LOG_DEBUG	(7)

#define LED_PROXY_MSG_SIZE (8-1)

#define rpmsg_log(rpmsg_dev, level, args...) \
	do { \
		if (!rpmsg_dev->led_dev) { \
			if (level == MTK_LED_LOG_DEBUG) \
				dev_dbg(rpmsg_dev->dev, ## args); \
			else if (level == MTK_LED_LOG_INFO) \
				dev_info(rpmsg_dev->dev, ## args); \
			else if (level == MTK_LED_LOG_WARN) \
				dev_warn(rpmsg_dev->dev, ## args); \
			else if (level == MTK_LED_LOG_ERR) \
				dev_err(rpmsg_dev->dev, ## args); \
		} else if (rpmsg_dev->led_dev->log_level < 0) { \
			if (level == MTK_LED_LOG_DEBUG) \
				dev_dbg(rpmsg_dev->led_dev->dev, ## args); \
			else if (level == MTK_LED_LOG_INFO) \
				dev_info(rpmsg_dev->led_dev->dev, ## args); \
			else if (level == MTK_LED_LOG_WARN) \
				dev_warn(rpmsg_dev->led_dev->dev, ## args); \
			else if (level == MTK_LED_LOG_ERR) \
				dev_err(rpmsg_dev->led_dev->dev, ## args); \
		} else if (rpmsg_dev->led_dev->log_level >= level) \
			dev_emerg(rpmsg_dev->led_dev->dev, ## args); \
	} while (0)

enum led_proxy_cmd {
	LED_CMD_SET_GPIO = 0,
	LED_CMD_SET_VALUE,
	LED_CMD_SET_BREATHLED,
	LED_CMD_SET_FADE_OUT,
	LED_CMD_MAX,
};


struct led_proxy_cmd_msg {
	uint8_t cmd;
	uint32_t payload[LED_PROXY_MSG_SIZE];
};

struct led_proxy_ack_msg {
	uint8_t cmd;
	int32_t val;
};


/**
 * struct mtk_rpmsg_led - rpmsg device driver private data
 *
 * @dev: platform device of rpmsg device
 * @rpdev: rpmsg device
 * @txn: rpmsg transaction callback
 * @txn_result: rpmsg transaction result
 * @receive_msg: received message of rpmsg transaction
 * @ack: completion of rpmsg transaction
 * @led_dev: led adapter of rpmsg device.
 */
struct mtk_rpmsg_led {
	struct device *dev;
	struct rpmsg_device *rpdev;
	int (*txn)(void *data, size_t len);
	int txn_result;
	struct led_proxy_ack_msg receive_msg;
	struct completion ack;
	struct mtk_rproc_led_bus_dev *led_dev;
	struct list_head list;
};

/**
 * struct mtk_rproc_led_bus_dev - led bus driver private data
 *
 * @dev: platform device of led bus
 * @gpio: led gpio number
 * @rpmsg_dev: rpmsg device for send cmd to rproc
 * @rpmsg_bind: completion when rpmsg and led device binded
 * @rproc_led_name: Required rproc led bus name
 * @log_level: override kernel log for mtk_dbg
 */
struct mtk_rproc_led_bus_dev {
	struct device *dev;
	int32_t gpio;
	struct mtk_rpmsg_led *rpmsg_dev;
	struct completion rpmsg_bind;
	const char *rproc_led_name;
	struct mbox_chan *mchan;
	int log_level;
	struct list_head list;
};
void mbox_cb(struct mbox_client *cl, void *mssg);

#endif
