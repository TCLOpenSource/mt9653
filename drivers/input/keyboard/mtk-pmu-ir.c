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
#include <asm/irq.h>
#include <linux/io.h>
#include "input_cli_svc_msg.h"

#define MTK_IR_DEVICE_NAME	"MTK PMU IR"
#define INPUTID_VENDOR		0x3697UL
#define INPUTID_PRODUCT		0x0001
#define INPUTID_VERSION		0x0001
#define IPCM_MAX_DATA_SIZE	64

#define MAX_KEY_EVENT_BUFFER_SIZE	(1<<8)
#define MAX_KEY_EVENT_BUFFER_MASK	(MAX_KEY_EVENT_BUFFER_SIZE-1)
#define DUMP_SIZE	16
#define WAIT_RESULT_TIMEOUT msecs_to_jiffies(1000)
//------------------------------------------------------------------------------

struct mtk_ir_dev {
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
/*
 * TODO: Remove save wkaeup key to dummy register when PMU always live.
 */
static int _ir_wakeup_key;
module_param_named(ir_wakeup_key, _ir_wakeup_key, int, 0644);

static int mtk_ir_get_wakeup_key(struct mtk_ir_dev *mtk_ir)
{
	struct input_svcmsg svcmsg = { 0 };
	struct input_svcmsg_open *msg = (struct input_svcmsg_open *)&svcmsg;
	int ret;
	struct input_climsg_get_wakeupkey *reply =
					(struct input_climsg_get_wakeupkey *)&mtk_ir->reply;

	msg->msg_id = INPUT_SVCMSG_GET_WAKEUPKEY;
	ret = rpmsg_send(mtk_ir->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret)
		pr_err("[%s]rpmsg send failed, ret=%d\n", __func__, ret);

	ret = wait_for_completion_interruptible_timeout
						(&mtk_ir->ack, WAIT_RESULT_TIMEOUT);
	if (!reply->ret)
		_ir_wakeup_key = reply->keycode;

	pr_err("[%s](ret, reply->ret)= (%d, %d)\n", __func__, ret, reply->ret);

	return ret;
}


static int mtk_ir_callback(struct rpmsg_device *rpdev, void *data,
						   int len, void *priv, u32 src)
{
	struct mtk_ir_dev *mtk_ir;
	struct input_climsg *climsg;
	struct input_climsg_event *event;
	int readbytes;
	unsigned int r, w;
	bool dump_msg = false;

	// sanity check
	if (!rpdev) {
		return -EINVAL;
	}
	mtk_ir = dev_get_drvdata(&rpdev->dev);
	if (!mtk_ir) {
		return -EINVAL;
	}
	if (!data || len == 0) {
		return -EINVAL;
	}
	climsg = (struct input_climsg *)data;

	if (climsg->msg_id == INPUT_CLIMSG_ACK) {
		mtk_ir->reply = *climsg;
		complete(&mtk_ir->ack);
	} else {
		readbytes = 0;
		event = (struct input_climsg_event *)climsg;

		r = mtk_ir->readpos;
		w = mtk_ir->writepos;

		while (((r != (w+1)) & MAX_KEY_EVENT_BUFFER_MASK) && (readbytes < len)) {
			if (event->msg_id == INPUT_CLIMSG_KEY_EVENT) {
				mtk_ir->keyevent_buffer[w] = *event;

			} else {
				dump_msg = true;
				pr_err("msg id(%d) incorrect@idx(%lu)\n",
					event->msg_id, (unsigned long)readbytes / sizeof(*event));
			}
			event++;
			w = (w+1) & MAX_KEY_EVENT_BUFFER_MASK;
			readbytes += sizeof(*event);
		}
		mtk_ir->writepos = w;
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
		wake_up_interruptible(&mtk_ir->kthread_waitq);
	}
	return 0;
}

static int mtk_ir_thread(void *priv)
{
	int r, w;
	struct input_climsg_event *event;
	struct mtk_ir_dev *mtk_ir = (struct mtk_ir_dev *)priv;

	for (;;) {
		wait_event_interruptible(mtk_ir->kthread_waitq,
					kthread_should_stop() ||
					(mtk_ir->readpos != mtk_ir->writepos));
		if (kthread_should_stop())
			break;

		r = mtk_ir->readpos;
		w = mtk_ir->writepos;

		while (r != w) {
			event = &mtk_ir->keyevent_buffer[r];
			pr_info("[%s] received_keyevent: 0x%X , 0x%X\n",
			__func__,
			event->keycode,
			event->pressed);
			input_report_key(mtk_ir->inputdev,
					event->keycode,
					event->pressed);

			input_sync(mtk_ir->inputdev);

			r = (r+1) & MAX_KEY_EVENT_BUFFER_MASK;
		}
		mtk_ir->readpos = r;

	}
	return 0;
}
static int mtk_ir_rpmsginit(struct mtk_ir_dev *mtk_ir)
{
	struct input_svcmsg svcmsg = { 0 };
	struct input_svcmsg_open *msg = (struct input_svcmsg_open *)&svcmsg;
	int ret;


	msg->msg_id = INPUT_SVCMSG_OPEN;
	ret = rpmsg_send(mtk_ir->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret)
		pr_err("[%s]rpmsg send failed, ret=%d\n", __func__, ret);

	return ret;
}
static int mtk_ir_suspend_prepare(struct mtk_ir_dev *mtk_ir)
{
	struct input_svcmsg svcmsg = { 0 };
	struct input_svcmsg_suspend *msg = (struct input_svcmsg_suspend *)&svcmsg;
	struct input_climsg_suspend *reply = (struct input_climsg_suspend *)&mtk_ir->reply;
	int ret;

	msg->msg_id = INPUT_SVCMSG_SUSPEND;
	ret = rpmsg_send(mtk_ir->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret)
		pr_err("[%s]rpmsg send failed, ret=%d\n", __func__, ret);

	return ret;
}

static int mtk_ir_resume_complete(struct mtk_ir_dev *mtk_ir)
{
	struct input_svcmsg svcmsg = { 0 };
	struct input_svcmsg_resume *msg = (struct input_svcmsg_resume *)&svcmsg;
	struct input_climsg_resume *reply = (struct input_climsg_resume *)&mtk_ir->reply;
	int ret;

	msg->msg_id = INPUT_SVCMSG_RESUME;
	ret = rpmsg_send(mtk_ir->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret)
		pr_err("[%s]rpmsg send failed, ret=%d\n", __func__, ret);

	return ret;
}

static int mtk_ir_pm_notifier_cb(struct notifier_block *nb,
			unsigned long event, void *ptr)
{
	struct mtk_ir_dev *mtk_ir = (struct mtk_ir_dev *)container_of(nb,
								   struct mtk_ir_dev, pm_notifier);
	switch (event) {
	case PM_SUSPEND_PREPARE:
		mtk_ir_suspend_prepare(mtk_ir);
		pr_info("[%s]PM_SUSPEND_PREPARE\n", __func__);
		_ir_wakeup_key = 0;//Clean
		break;
	case PM_POST_SUSPEND:
		mtk_ir_resume_complete(mtk_ir);
		pr_info("[%s]PM_POST_SUSPEND\n", __func__);
		mtk_ir_get_wakeup_key(mtk_ir);
		break;
	default:
		break;
	}
	return 0;
}

static int mtk_ir_probe(struct rpmsg_device *rpdev)
{
	int i, ret = 0;
	struct mtk_ir_dev *mtk_ir = NULL;
	struct input_dev *inputdev = NULL;

	mtk_ir = kzalloc(sizeof(struct mtk_ir_dev), GFP_KERNEL);
	inputdev = input_allocate_device();
	if (!mtk_ir || !inputdev) {
		pr_err("%s: Not enough memory\n", __FILE__);
		ret = -ENOMEM;
		goto err_allocate_dev;
	}

	mtk_ir->rpdev = rpdev;
	init_completion(&mtk_ir->ack);

	inputdev->id.bustype = BUS_HOST;
	inputdev->id.vendor = INPUTID_VENDOR;
	inputdev->id.product = INPUTID_PRODUCT;
	inputdev->id.version = INPUTID_VERSION;
	set_bit(EV_KEY, inputdev->evbit);
	set_bit(EV_REP, inputdev->evbit);
	set_bit(EV_MSC, inputdev->evbit);
	set_bit(MSC_SCAN, inputdev->mscbit);
	inputdev->dev.parent = &rpdev->dev;
	inputdev->phys = "/dev/ir";
	inputdev->name = MTK_IR_DEVICE_NAME;

	for (i = 0; i < KEY_CNT; i++)
		__set_bit(i, inputdev->keybit);

	__clear_bit(BTN_TOUCH, inputdev->keybit);

	ret = input_register_device(inputdev);
	if (ret) {
		pr_err("%s: failed to register device\n", __FILE__);

		goto err_register_input_dev;
	}

	mtk_ir->inputdev = inputdev;


	mtk_ir->pm_notifier.notifier_call = mtk_ir_pm_notifier_cb;
	ret = register_pm_notifier(&mtk_ir->pm_notifier);
	if (ret) {
		pr_err("%s: couldn't register pm notifier\n", __func__);

		goto err_register_pm_notifier;
	}
	dev_set_drvdata(&rpdev->dev, mtk_ir);
	init_waitqueue_head(&mtk_ir->kthread_waitq);

	mtk_ir->task = kthread_run(mtk_ir_thread, (void *)mtk_ir, "mtk_ir_thread");
	if (IS_ERR(mtk_ir->task)) {
		pr_err("create thread for add spi failed!\n");
		ret = IS_ERR(mtk_ir->task);
		goto err_kthread_run;
	}
	/*
	 * TODO: Remove the mtk_ir_rpmsginit() & INPUT_SVCMSG_OPEN.
	 * After create a RPMsg endpoint in pmu, initialize it with a name,
	 * source address,remoteproc address.
	 * But Can't get remoteproc address in PMU until the first command is received.
	 */
	//mtk_ir_rpmsginit(mtk_ir);
	mtk_ir_get_wakeup_key(mtk_ir);
	return 0;

err_kthread_run:
	unregister_pm_notifier(&mtk_ir->pm_notifier);

err_register_pm_notifier:
	input_unregister_device(inputdev);
err_register_input_dev:
err_allocate_dev:
	if (inputdev)
		input_free_device(inputdev);

	kfree(mtk_ir);
	return ret;
}

void mtk_ir_remove(struct rpmsg_device *rpdev)
{
	struct mtk_ir_dev *mtk_ir = dev_get_drvdata(&rpdev->dev);

	kthread_stop(mtk_ir->task);
	input_unregister_device(mtk_ir->inputdev);
	unregister_pm_notifier(&mtk_ir->pm_notifier);
	input_free_device(mtk_ir->inputdev);
	kfree(mtk_ir);
}

static struct rpmsg_device_id mtk_ir_id_table[] = {
	{.name = "ir0"},	// this name must match with rtos
	{},
};
MODULE_DEVICE_TABLE(rpmsg, mtk_ir_id_table);

static struct rpmsg_driver mtk_ir_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= mtk_ir_id_table,
	.probe		= mtk_ir_probe,
	.callback	= mtk_ir_callback,
	.remove		= mtk_ir_remove,
};

module_rpmsg_driver(mtk_ir_driver);

MODULE_AUTHOR("jefferry.yen <jefferry.yen@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek dtv ir receiver controller driver");
