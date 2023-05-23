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

#define MSG "hello world!"

#define DECLARE_RPMSG_TEST(t)	\
static struct rpmsg_device_id st_table_##t[] = { \
	{ .name = #t },				\
	{ },					\
}; \
MODULE_DEVICE_TABLE(rpmsg, st_table_##t); \
static struct rpmsg_driver t = {		\
	.drv.name	= #t,			\
	.id_table	= st_table_##t,	\
	.probe		= rpmsg_st_probe,	\
	.callback	= rpmsg_st_cb,		\
	.remove		= rpmsg_st_remove,	\
}

static int nr_ept;

static int rpmsg_st_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	int ret;

	pr_info("%s: recv %d byte with 0x%x, send back\n",
			rpdev->id.name, len, *(uint8_t *)data);

#ifdef DEBUG
	pr_info("RX Raw:\n");
	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, 16, 1,
		       data, len, true);
#endif
	/* send the data back */
	ret = rpmsg_sendto(rpdev->ept, data, len, src);
	if (ret)
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);

	return ret;
}

static int rpmsg_st_probe(struct rpmsg_device *rpdev)
{
	int ret;

	dev_info(&rpdev->dev, "%s: new channel: 0x%x -> 0x%x!\n",
				rpdev->id.name, rpdev->src, rpdev->dst);

	/* send a message to our remote processor */
	ret = rpmsg_send(rpdev->ept, MSG, strlen(MSG));
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static void rpmsg_st_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg sample client driver is removed\n");
}

DECLARE_RPMSG_TEST(rpmsg_st0);
DECLARE_RPMSG_TEST(rpmsg_st1);
DECLARE_RPMSG_TEST(rpmsg_st2);
DECLARE_RPMSG_TEST(rpmsg_st3);
DECLARE_RPMSG_TEST(rpmsg_st4);
DECLARE_RPMSG_TEST(rpsmg_st5);
DECLARE_RPMSG_TEST(rpmsg_st6);
DECLARE_RPMSG_TEST(rpmsg_st7);
DECLARE_RPMSG_TEST(rpmsg_st8);
DECLARE_RPMSG_TEST(rpmsg_st9);
DECLARE_RPMSG_TEST(rpmsg_st10);
DECLARE_RPMSG_TEST(rpmsg_st11);
DECLARE_RPMSG_TEST(rpmsg_st12);
DECLARE_RPMSG_TEST(rpmsg_st13);
DECLARE_RPMSG_TEST(rpmsg_st14);
DECLARE_RPMSG_TEST(rpmsg_st15);

static struct rpmsg_driver *drv[] = {
	&rpmsg_st0,
	&rpmsg_st1,
	&rpmsg_st2,
	&rpmsg_st3,
	&rpmsg_st4,
	&rpsmg_st5,
	&rpmsg_st6,
	&rpmsg_st7,
	&rpmsg_st8,
	&rpmsg_st9,
	&rpmsg_st10,
	&rpmsg_st11,
	&rpmsg_st12,
	&rpmsg_st13,
	&rpmsg_st14,
	&rpmsg_st15,
};

static int __init rpmsg_st_init(void)
{
	int i;

	if (nr_ept > ARRAY_SIZE(drv) || nr_ept < 1) {
		pr_err("Invalid nr_ept (%d)(nr_ept > %d or <= 0)\n",
			nr_ept, (int)ARRAY_SIZE(drv));
		return -EINVAL;
	}

	pr_info("Create %d rpmsg ept\n", nr_ept);

	for (i = 0; i < nr_ept; ++i)
		register_rpmsg_driver(drv[i]);

	return 0;
}

static void __exit rpmsg_st_exit(void)
{
	int i;

	for (i = 0; i < nr_ept; ++i)
		unregister_rpmsg_driver(drv[i]);
}

module_init(rpmsg_st_init);
module_exit(rpmsg_st_exit);
module_param(nr_ept, int, 0444);

MODULE_DESCRIPTION("Remote processor messaging sample client driver");
MODULE_LICENSE("GPL v2");
