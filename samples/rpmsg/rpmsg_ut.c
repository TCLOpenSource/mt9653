// SPDX-License-Identifier: GPL-2.0
/*
 * rpmsg_test.c  --  Mediatek DTV rpmsg test driver
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/delay.h>

#define MSG		"hello world!"
#define MSG_LIMIT	10000

struct instance_data {
	int rx_count;
};

static int rx_count;

static int rpmsg_sample_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	int ret = 0;

	dev_info(&rpdev->dev, "recv %d byte(s)\n", len);
	rx_count++;

	pr_info("RX Raw:\n");
	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, 16, 1,
		       data, len, true);

	/* samples should not live forever */
	if (rx_count >= MSG_LIMIT) {
		dev_err(&rpdev->dev, "goodbye!\n");
		ret = -EINVAL;
		goto out;
	}

	/* send the data back */
	ret = rpmsg_sendto(rpdev->ept, data, len, src);
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		goto out;
	}

	dev_info(&rpdev->dev, "send %d time(s)\n", rx_count);
out:
	return ret;
}

static int rpmsg_sample_probe(struct rpmsg_device *rpdev)
{
	int ret;
	struct instance_data *idata;

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n",
					rpdev->src, rpdev->dst);

	idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	dev_set_drvdata(&rpdev->dev, idata);

	/* send a message to our remote processor */
	ret = rpmsg_send(rpdev->ept, MSG, strlen(MSG));
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static void rpmsg_sample_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg sample client driver is removed\n");
}

static struct rpmsg_device_id rpmsg_driver_sample_id_table[] = {
	{ .name	= "rpmsg-ept"},
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_sample_id_table);

static struct rpmsg_driver rpmsg_sample_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_sample_id_table,
	.probe		= rpmsg_sample_probe,
	.callback	= rpmsg_sample_cb,
	.remove		= rpmsg_sample_remove,
};
module_rpmsg_driver(rpmsg_sample_client);

MODULE_DESCRIPTION("Remote processor messaging sample client driver");
MODULE_LICENSE("GPL v2");
