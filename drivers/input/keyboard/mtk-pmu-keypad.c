// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author jefferry.yen <jefferry.yen@mediatek.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <linux/sysfs.h>

#include <asm/irq.h>
#include <linux/io.h>
#include "input_cli_svc_msg.h"

#define MTK_KEYPAD_DEVICE_NAME	"MTK PMU KEYPAD"

#define INPUTID_VENDOR		0x3697UL
#define INPUTID_PRODUCT		0x0002
#define INPUTID_VERSION		0x0002
#define IPCM_MAX_DATA_SIZE	64

#define MAX_KEY_EVENT_BUFFER_SIZE	(1<<8)
#define MAX_KEY_EVENT_BUFFER_MASK	(MAX_KEY_EVENT_BUFFER_SIZE-1)
#define DUMP_SIZE	16

#define MIC_MUTE_TEST       1
#define MTK_KEYPAD_TYPE_MASK 0xc0000000
#define MTK_KEYPAD_TYPE_SW   0x40000000

//------------------------------------------------------------------------------

struct mtk_keypad_dev {
	struct input_dev *inputdev;
	struct rpmsg_device *rpdev;
	struct task_struct *task;
	wait_queue_head_t kthread_waitq;

	struct completion ack;
	struct input_climsg reply;
	int readpos;
	int writepos;
	struct input_climsg_event keyevent_buffer[MAX_KEY_EVENT_BUFFER_SIZE];
	struct notifier_block pm_notifier;
};

/* MIC MUTE test code */
#if MIC_MUTE_TEST

static u32 _test_mic_mute;
static struct mtk_keypad_dev *__mtk_keypad;
static ssize_t mic_mute_sysfs_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *page)
{
	int rc;

	rc = scnprintf(page, PAGE_SIZE, "%d\n", _test_mic_mute);
	return rc;
}

static ssize_t mic_mute_sysfs_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t len)
{
	unsigned long mute;
	u32 keycode = SW_MUTE_DEVICE;
	int rc;

	rc = kstrtoul(buf, 0, &mute);
	if (rc)
		return rc;

	if (__mtk_keypad == NULL)
		return -ENXIO;
	input_set_capability(__mtk_keypad->inputdev, EV_SW, keycode);
	input_report_switch(__mtk_keypad->inputdev, keycode, mute);
	input_sync(__mtk_keypad->inputdev);

	return len;
}

static struct kobj_attribute kobj_attr_mic_mute =
	__ATTR(mic_mute_test, 0660, mic_mute_sysfs_show, mic_mute_sysfs_store);
#endif

static int mtk_keypad_callback(struct rpmsg_device *rpdev, void *data,
						   int len, void *priv, u32 src)
{
	struct mtk_keypad_dev *mtk_keypad;
	struct input_climsg *climsg;
	struct input_climsg_event *event;
	int readbytes;
	unsigned int r, w;
	bool dump_msg = false;

	// sanity check
	if (!rpdev) {
		return -EINVAL;
	}
	mtk_keypad = dev_get_drvdata(&rpdev->dev);
	if (!mtk_keypad) {
		return -EINVAL;
	}
	if (!data || len == 0) {
		return -EINVAL;
	}
	climsg = (struct input_climsg *)data;
	if (climsg->msg_id == INPUT_CLIMSG_ACK) {
		mtk_keypad->reply = *climsg;
		complete(&mtk_keypad->ack);
	} else {
		readbytes = 0;
		event = (struct input_climsg_event *)climsg;

		r = mtk_keypad->readpos;
		w = mtk_keypad->writepos;

		while (((r != (w+1)) & MAX_KEY_EVENT_BUFFER_MASK) && (readbytes < len)) {
			if (event->msg_id == INPUT_CLIMSG_KEY_EVENT) {
				mtk_keypad->keyevent_buffer[w] = *event;

			} else {
				dump_msg = true;
				pr_err("msg id(%d) incorrect@idx(%lu)\n",
					event->msg_id, (unsigned long)readbytes / sizeof(*event));
			}
			event++;
			w = (w+1) & MAX_KEY_EVENT_BUFFER_MASK;
			readbytes += sizeof(*event);
		}
		mtk_keypad->writepos = w;
		if ((r == (w+1)) & MAX_KEY_EVENT_BUFFER_MASK)
			pr_err("keyevent_buffer full!\n");
		if (readbytes != len) {
			dump_msg = true;
			pr_err("size of total event count(%d) != rpmsg rx length(%d)\n",
			readbytes, len);
		}
#ifdef DEBUG
		if (dump_msg)
			print_hex_dump(KERN_INFO, " ",
					DUMP_PREFIX_OFFSET, DUMP_SIZE, 1, data, len, true);
#endif
		wake_up_interruptible(&mtk_keypad->kthread_waitq);
	}
	return 0;
}

static int mtk_keypad_thread(void *priv)
{
	int r, w;
	struct input_climsg_event *event;
	struct mtk_keypad_dev *mtk_keypad = (struct mtk_keypad_dev *)priv;

	for (;;) {
		wait_event_interruptible(mtk_keypad->kthread_waitq,
					kthread_should_stop() ||
					(mtk_keypad->readpos != mtk_keypad->writepos));
		if (kthread_should_stop())
			break;

		r = mtk_keypad->readpos;
		w = mtk_keypad->writepos;

		while (r != w) {
			event = &mtk_keypad->keyevent_buffer[r];
			uint32_t keycode = event->keycode;
			uint32_t pressed = event->pressed;

			if ((keycode & MTK_KEYPAD_TYPE_MASK) == MTK_KEYPAD_TYPE_SW) {
				keycode = keycode & (~MTK_KEYPAD_TYPE_MASK);
				pr_info("[%s] received a switch event: 0x%X, 0x%X!\n",
						__func__, keycode, pressed);
				input_set_capability(mtk_keypad->inputdev, EV_SW, keycode);
				input_report_switch(mtk_keypad->inputdev, keycode, pressed);
			} else {
				pr_info("[%s] received_keyevent: 0x%X , 0x%X\n",
						__func__, keycode, pressed);
				input_set_capability(mtk_keypad->inputdev, EV_KEY,
						keycode);
				input_report_key(mtk_keypad->inputdev, keycode, pressed);
			}

			input_sync(mtk_keypad->inputdev);

			r = (r+1) & MAX_KEY_EVENT_BUFFER_MASK;
		}
		mtk_keypad->readpos = r;

	}

	return 0;
}
static int mtk_keypad_rpmsginit(struct mtk_keypad_dev *mtk_keypad)
{
	struct input_svcmsg svcmsg = { 0 };
	struct input_svcmsg_open *msg = (struct input_svcmsg_open *)&svcmsg;
	int ret;


	msg->msg_id = INPUT_SVCMSG_OPEN;
	ret = rpmsg_send(mtk_keypad->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret)
		pr_err("[%s]rpmsg send failed, ret=%d\n", __func__, ret);

	return ret;
}
static int mtk_keypad_suspend_prepare(struct mtk_keypad_dev *mtk_keypad)
{
	struct input_svcmsg svcmsg = { 0 };
	struct input_svcmsg_suspend *msg = (struct input_svcmsg_suspend *)&svcmsg;
	struct input_climsg_suspend *reply = (struct input_climsg_suspend *)&mtk_keypad->reply;
	int ret;

	msg->msg_id = INPUT_SVCMSG_SUSPEND;
	ret = rpmsg_send(mtk_keypad->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret)
		pr_err("[%s]rpmsg send failed, ret=%d\n", __func__, ret);

	return ret;
}

static int mtk_keypad_resume_complete(struct mtk_keypad_dev *mtk_keypad)
{
	struct input_svcmsg svcmsg = { 0 };
	struct input_svcmsg_resume *msg = (struct input_svcmsg_resume *)&svcmsg;
	struct input_climsg_resume *reply = (struct input_climsg_resume *)&mtk_keypad->reply;
	int ret;

	msg->msg_id = INPUT_SVCMSG_RESUME;
	ret = rpmsg_send(mtk_keypad->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret)
		pr_err("[%s]rpmsg send failed, ret=%d\n", __func__, ret);

	return ret;
}

static int mtk_keypad_pm_notifier_cb(struct notifier_block *nb,
			unsigned long event, void *ptr)
{
	struct mtk_keypad_dev *mtk_keypad = (struct mtk_keypad_dev *)container_of(nb,
							struct mtk_keypad_dev, pm_notifier);
	switch (event) {
	case PM_SUSPEND_PREPARE:
		mtk_keypad_suspend_prepare(mtk_keypad);
		break;
	case PM_POST_SUSPEND:
		mtk_keypad_resume_complete(mtk_keypad);
		break;
	default:
		break;
	}

	return 0;
}

static int mtk_keypad_probe(struct rpmsg_device *rpdev)
{
	int i, ret = 0;
	struct mtk_keypad_dev *mtk_keypad = NULL;
	struct input_dev *inputdev = NULL;


	mtk_keypad = kzalloc(sizeof(struct mtk_keypad_dev), GFP_KERNEL);
	inputdev = input_allocate_device();
	if (!mtk_keypad || !inputdev) {
		pr_err("%s: Not enough memory\n", __FILE__);
		ret = -ENOMEM;
		goto err_allocate_dev;
	}

	mtk_keypad->rpdev = rpdev;
	init_completion(&mtk_keypad->ack);

	inputdev->id.bustype = BUS_HOST;
	inputdev->id.vendor = INPUTID_VENDOR;
	inputdev->id.product = INPUTID_PRODUCT;
	inputdev->id.version = INPUTID_VERSION;
	set_bit(EV_KEY, inputdev->evbit);

	inputdev->dev.parent = &rpdev->dev;
	inputdev->phys = "/dev/keypad";
	inputdev->name = MTK_KEYPAD_DEVICE_NAME;

	for (i = 0; i < KEY_CNT; i++)
		__set_bit(i, inputdev->keybit);
	__clear_bit(BTN_TOUCH, inputdev->keybit);

	dev_info(&rpdev->dev, "enabling EVSW(SW_MUTE_DEVICE) capability!\n");
	set_bit(EV_SW, inputdev->evbit);
	input_set_capability(inputdev, EV_SW, SW_MUTE_DEVICE);

	ret = input_register_device(inputdev);
	if (ret) {
		pr_err("%s: failed to register device\n", __FILE__);

		goto err_register_input_dev;
	}
	mtk_keypad->inputdev = inputdev;
	mtk_keypad->pm_notifier.notifier_call = mtk_keypad_pm_notifier_cb;
	ret = register_pm_notifier(&mtk_keypad->pm_notifier);
	if (ret) {
		pr_err("%s: couldn't register pm notifier\n", __func__);

		goto err_register_pm_notifier;
	}
	dev_set_drvdata(&rpdev->dev, mtk_keypad);
	init_waitqueue_head(&mtk_keypad->kthread_waitq);

	mtk_keypad->task = kthread_run(mtk_keypad_thread, (void *)mtk_keypad, "mtk_keypad_thread");
	if (IS_ERR(mtk_keypad->task)) {
		pr_err("create thread for add spi failed!\n");
		ret = IS_ERR(mtk_keypad->task);
		goto err_kthread_run;
	}

	/*
	 * TODO: Remove the mtk_ir_rpmsginit() & INPUT_SVCMSG_OPEN.
	 * After create a RPMsg endpoint in pmu, initialize it with a name,
	 * source address,remoteproc address.
	 * But Can't get remoteproc address in PMU until the first command is received.
	 */
	mtk_keypad_rpmsginit(mtk_keypad);

#ifdef MIC_MUTE_TEST
	__mtk_keypad = mtk_keypad;
	if (sysfs_create_file(kernel_kobj, &kobj_attr_mic_mute.attr) < 0)
		pr_err("cannot register mic_mute_test sysfs!\n");
#endif
	return 0;

err_kthread_run:
	unregister_pm_notifier(&mtk_keypad->pm_notifier);

err_register_pm_notifier:
	input_unregister_device(inputdev);
err_register_input_dev:
err_allocate_dev:
	if (inputdev)
		input_free_device(inputdev);

	kfree(mtk_keypad);
	return ret;
}

void mtk_keypad_remove(struct rpmsg_device *rpdev)
{
	struct mtk_keypad_dev *mtk_keypad = dev_get_drvdata(&rpdev->dev);

	kthread_stop(mtk_keypad->task);
	input_unregister_device(mtk_keypad->inputdev);
	unregister_pm_notifier(&mtk_keypad->pm_notifier);
	input_free_device(mtk_keypad->inputdev);
	kfree(mtk_keypad);
}

static struct rpmsg_device_id mtk_keypad_id_table[] = {
	{.name = "keypad0"},	// this name must match with rtos
	{},
};
MODULE_DEVICE_TABLE(rpmsg, mtk_keypad_id_table);

static struct rpmsg_driver mtk_keypad_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= mtk_keypad_id_table,
	.probe		= mtk_keypad_probe,
	.callback	= mtk_keypad_callback,
	.remove		= mtk_keypad_remove,
};
module_rpmsg_driver(mtk_keypad_driver);


MODULE_AUTHOR("jefferry.yen <jefferry.yen@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek dtv keypad receiver controller driver");
