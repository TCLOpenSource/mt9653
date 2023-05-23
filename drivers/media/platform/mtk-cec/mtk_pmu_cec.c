// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author daniel-cc.liu <daniel-cc.liu@mediatek.com>
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

#include <asm/irq.h>
#include <linux/io.h>
#include <media/cec.h>

#include "cec_cli_svc_msg.h"

#define MTK_CEC_NAME		"mediatek,cec"
#define DEFAULT_LOG_LEVEL	4

#define HEX_A_DEC			10
#define BIT_NUM_IN_NIBBLE	4
#ifndef BIT
#define BIT(_bit_)			(1 << (_bit_))
#endif

int char_to_int(char *buf, int idx_s, int size)
{
	int i = 0;
	int tmp = 0;
	int ret = 0;

	for (i = 0; i < size; i++) {
		tmp = (int)buf[idx_s + i];

		if (tmp >= '0' && tmp <= '9')
			tmp -= '0';
		else if (tmp >= 'A' && tmp <= 'F')
			tmp = tmp - 'A' + HEX_A_DEC;
		else if (tmp >= 'a' && tmp <= 'f')
			tmp = tmp - 'a' + HEX_A_DEC;
		else
			return -1;

		ret = (ret << BIT_NUM_IN_NIBBLE) | tmp;
	}

	return ret;
}

struct mtk_cec_event {
	struct cec_climsg_event event;
	struct list_head node;
};

struct mtk_cec_dev {
	struct cec_adapter    *adap;
	struct rpmsg_device *rpdev;
	struct cec_generic_msg climsg;
	struct completion ack;

	struct task_struct *task;
	wait_queue_head_t kthread_waitq;

	spinlock_t lock;
	struct list_head event_queue;
	uint32_t event_cnt;
	struct notifier_block pm_notifier;

	bool suspended;
	wait_queue_head_t suspend_waitq;
	unsigned long log_level;
};

static int mtk_pmu_cec_callback(struct rpmsg_device *rpdev, void *data,
						   int len, void *priv, u32 src)
{
	struct mtk_cec_dev *mtk_cec;
	struct cec_generic_msg *msg;

	mtk_cec = dev_get_drvdata(&rpdev->dev);
	if (!mtk_cec) {
		dev_err(&rpdev->dev, "private data is NULL\n");
		return -EINVAL;
	}
	if (!data || len == 0) {
		dev_err(&rpdev->dev, "invalid data or length from src:%d\n", src);
		return -EINVAL;
	}

	dev_dbg(&rpdev->dev, "[%s]\n", __func__);
	msg = (struct cec_generic_msg *)data;

	if (msg->msg_id == CEC_CLIMSG_ACK) {
		mtk_cec->climsg = *msg;
		complete(&mtk_cec->ack);

	} else {
		struct cec_climsg_event *event = (struct cec_climsg_event *)msg;
		struct mtk_cec_event *ev = kzalloc(sizeof(struct mtk_cec_event), GFP_KERNEL);

		if (ev) {
			ev->event = *(struct cec_climsg_event *)msg;
			spin_lock(&mtk_cec->lock);
			list_add_tail(&ev->node, &mtk_cec->event_queue);
			mtk_cec->event_cnt++;
			spin_unlock(&mtk_cec->lock);
			dev_dbg(&rpdev->dev, "receive event, cnt=%d\n", mtk_cec->event_cnt);
			wake_up_interruptible(&mtk_cec->kthread_waitq);
		} else {
			dev_err(&rpdev->dev, "kmalloc fail!\n");
		}
	}
	return 0;
}

static int mtk_pmu_cec_thread(void *priv)
{
	struct mtk_cec_dev *mtk_cec = priv;
	struct mtk_cec_event *entry;
	struct cec_climsg_event	*ev;
	struct list_head *node;
	int index = 0;

	dev_info(&mtk_cec->rpdev->dev, "enter %s\n", __func__);
	for (;;) {
		wait_event_interruptible(mtk_cec->kthread_waitq,
						kthread_should_stop() ||
						!list_empty(&mtk_cec->event_queue));

		if (kthread_should_stop())
			break;

		spin_lock(&mtk_cec->lock);
		node = mtk_cec->event_queue.next;
		list_del(node);
		mtk_cec->event_cnt--;
		spin_unlock(&mtk_cec->lock);

		entry = container_of(node, struct mtk_cec_event, node);
		ev = &entry->event;

		if (ev->msg_id & CEC_CLIMSG_TX_DONE) {
			cec_transmit_done(mtk_cec->adap, CEC_TX_STATUS_OK, 0, 0, 0, 0);
			dev_dbg(&mtk_cec->rpdev->dev, "[%s] Tx: STATE_DONE\n", __func__);
		} else if (ev->msg_id & CEC_CLIMSG_TX_NACK) {
			cec_transmit_done(mtk_cec->adap,
				CEC_TX_STATUS_MAX_RETRIES | CEC_TX_STATUS_NACK, 0, 1, 0, 0);
			dev_dbg(&mtk_cec->rpdev->dev, "[%s] Tx: STATE_NACK\n", __func__);
		} else if (ev->msg_id & CEC_CLIMSG_TX_ERROR) {
			cec_transmit_done(mtk_cec->adap,
				CEC_TX_STATUS_MAX_RETRIES | CEC_TX_STATUS_ERROR, 1, 0, 0, 1);
			dev_dbg(&mtk_cec->rpdev->dev, "[%s] Tx: STATE_ERROR\n", __func__);
		}

		if (ev->msg_id & CEC_CLIMSG_RX_DONE) {
			struct cec_msg cecmsg;

			memset(&cecmsg, 0, sizeof(cecmsg));
			if (ev->msg.length <= CEC_MAX_MSG_SIZE) {
				cecmsg.len = ev->msg.length;
				memcpy(cecmsg.msg, ev->msg.msg, ev->msg.length);
				cec_received_msg(mtk_cec->adap, &cecmsg);
/*
 *				for(index=0;index<CEC_MAX_MSG_SIZE;index++)
 *				{
 *					cecmsg.len[]
 *				}
 */
				dev_info(&mtk_cec->rpdev->dev, "received msg len=%d\n", cecmsg.len);
			} else {
				dev_err(&mtk_cec->rpdev->dev, "Invalid cec rx msg length:%d\n",
						ev->msg.length);
			}
		}

		kfree(ev);
	}
	return 0;
}

static struct rpmsg_device_id mtk_pmu_cec_id_table[] = {
	{.name = "cec"},	// this name must match with rtos
	{},
};

static int mtk_pmu_cec_adap_enable(struct cec_adapter *adap, bool enable)
{
	int ret;
	struct mtk_cec_dev *mtk_cec = cec_get_drvdata(adap);
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_enable *msg = (struct cec_svcmsg_enable *)&svcmsg;
	struct cec_climsg_enable *reply = (struct cec_climsg_enable *)&mtk_cec->climsg;


	dev_info(&mtk_cec->rpdev->dev, "enter [%s]\n", __func__);
	if (mtk_cec->suspended) {
		ret = wait_event_interruptible(mtk_cec->suspend_waitq, !mtk_cec->suspended);

		dev_info(&mtk_cec->rpdev->dev, "[%s] MTK CEC: wait_event_interruptible error\n",
				__func__);
		if (ret < 0)
			return ret;
	}

	msg->msg_id = CEC_SVCMSG_ENABLE;
	msg->enable = enable;
	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));

	if (ret < 0) {
		dev_info(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n", __func__);
		return ret;
	}
	ret = wait_for_completion_interruptible(&mtk_cec->ack);
	if (ret < 0) {
		dev_info(&mtk_cec->rpdev->dev, "[%s] MTK CEC: wait_for_completion_interruptible error %x\n",
				__func__, ret);
		return ret;
	}

	if (enable) {
		mtk_cec->adap->phys_addr = 0x0000;
		//dev_info(&mtk_cec->rpdev->dev, "[%s] enable = %x\n", __func__, enable);
	}

	return reply->ret;
}


static int mtk_pmu_cec_adap_log_addr(struct cec_adapter *adap, u8 addr)
{
	int ret;
	struct mtk_cec_dev *mtk_cec = cec_get_drvdata(adap);
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_log_addr *msg = (struct cec_svcmsg_log_addr *)&svcmsg;
	struct cec_climsg_log_addr *reply = (struct cec_climsg_log_addr *)&mtk_cec->climsg;

	dev_info(&mtk_cec->rpdev->dev, "enter [%s]\n", __func__);

	if (mtk_cec->suspended) {
		ret = wait_event_interruptible(mtk_cec->suspend_waitq, !mtk_cec->suspended);
		if (ret < 0)
			return ret;
	}

	msg->msg_id = CEC_SVCMSG_LOG_ADDR;
	msg->addr = addr;
	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));

	if (ret < 0) {
		dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n", __func__);
		return ret;
	}
	ret = wait_for_completion_interruptible(&mtk_cec->ack);
	if (ret < 0)
		return ret;

	return reply->ret;
}

static int mtk_pmu_cec_adap_transmit(struct cec_adapter *adap, u8 attempts,
	u32 signal_free_time, struct cec_msg *cec_msg)
{
	int ret;
	struct mtk_cec_dev *mtk_cec = cec_get_drvdata(adap);
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_transmit *msg = (struct cec_svcmsg_transmit *)&svcmsg;
	struct cec_climsg_transmit *reply = (struct cec_climsg_transmit *)&mtk_cec->climsg;


	dev_dbg(&mtk_cec->rpdev->dev, "enter [%s]\n", __func__);

	if (mtk_cec->suspended) {
		ret = wait_event_interruptible(mtk_cec->suspend_waitq, !mtk_cec->suspended);
		if (ret < 0)
			return 0;//ret; return 0 to avoid STI abort command when transmit fail
	}

	msg->msg_id = CEC_SVCMSG_TRANSMIT;
	msg->attempts = attempts;
	msg->cec_msg.length = cec_msg->len;
	memcpy(msg->cec_msg.msg, cec_msg->msg, cec_msg->len);
	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));

	if (ret < 0) {
		dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n", __func__);
		return ret;
	}
	ret = wait_for_completion_interruptible(&mtk_cec->ack);

	if (ret < 0)
		return ret;

	return reply->ret;
}

static const struct cec_adap_ops mtk_cec_adap_ops = {
	.adap_enable	=	mtk_pmu_cec_adap_enable,
	.adap_log_addr	=	mtk_pmu_cec_adap_log_addr,
	.adap_transmit	=	mtk_pmu_cec_adap_transmit,
};


static ssize_t cec_factory_pin_enable_store(struct device *dev,
							struct device_attribute *attr,
							const char *buf,
							size_t count)
{

		int ret, i;
		struct mtk_cec_dev *mtk_cec = dev_get_drvdata(dev);
		struct cec_generic_msg svcmsg = { 0 };
		struct cec_svcmsg_factory_pin_enable *msg =
			(struct cec_svcmsg_factory_pin_enable *)&svcmsg;
		struct cec_climsg_factory_pin_enable *reply =
			(struct cec_climsg_factory_pin_enable *)&mtk_cec->climsg;

		dev_info(&mtk_cec->rpdev->dev, "enter [%s] buf[0] = %x\n", __func__, buf[0]);

		if (mtk_cec->suspended) {
			ret = wait_event_interruptible(mtk_cec->suspend_waitq, !mtk_cec->suspended);
			if (ret < 0)
				return ret;
		}

		msg->msg_id = CEC_SVCMSG_ENABLE_FACTORY_PIN;

		if (buf[0] == '1')// 0x31 = "1"
			msg->enable = 1;
		else
			msg->enable = 0;
		dev_info(&mtk_cec->rpdev->dev, "enable = %x\n", msg->enable);

		ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));

		if (ret < 0) {
			dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n", __func__);
			return count;
		}

		ret = wait_for_completion_interruptible(&mtk_cec->ack);
		if (ret < 0)
			return count;

		return count;

}
static ssize_t cec_factory_pin_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int int_str_size = 0;

	//query HDMI number
	//sysfs_print(buf, &intStrSize, u16Size,6);

	return int_str_size;
}

static ssize_t cec_wakeup_command_store(struct device *dev,
							struct device_attribute *attr,
							const char *buf,
							size_t count)
{
	int ret = 0;
	int idx = -1, idx_s = -1, idx_e = 0;
	int tmp;
	unsigned int msg_idx = 0;
	int char_num_per_cmd = 0;

	struct mtk_cec_dev *mtk_cec = dev_get_drvdata(dev);
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_set_poweron_msg *msg =
		(struct cec_svcmsg_set_poweron_msg *)&svcmsg;
	struct cec_climsg_set_poweron_msg *reply =
		(struct cec_climsg_set_poweron_msg *)&mtk_cec->climsg;

	if (mtk_cec->suspended) {
		ret = wait_event_interruptible(mtk_cec->suspend_waitq, !mtk_cec->suspended);
		if (ret < 0)
			return ret;
	}

	msg->msg_id = CEC_SVCMSG_SET_POWERON_MSG; //dont touch this line

	while (idx < (count - 1) || idx < 0) {
		idx++;
		if (buf[idx] == ' ' || buf[idx] == '\n') {
			char_num_per_cmd = idx_e - idx_s + 1;

			if (idx_s != -1 && char_num_per_cmd <= BIT(1)) {
				tmp = char_to_int((char *)buf, idx_s, char_num_per_cmd);
				if (tmp >= 0) {
					msg->cec_msg.msg[msg_idx++] = tmp;
					dev_info(&mtk_cec->rpdev->dev, "msg = %x\n", tmp);
				}
			}

			idx_s = -1;
			continue;
		} else {
			if (idx_s < 0)
				idx_s = idx;
			idx_e = idx;
		}

		if (msg_idx == CEC_MAX_MSG_SIZE)
			break;
	}

	msg->cec_msg.length = msg_idx;

	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret < 0) {
		dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n", __func__);
		return count;
	}

	ret = wait_for_completion_interruptible(&mtk_cec->ack);
	if (ret < 0)
		return count;

	return count;
}
static ssize_t cec_wakeup_command_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int int_str_size = 0;

	//query HDMI number
	//sysfs_print(buf, &intStrSize, u16Size,6);

	return int_str_size;
}

static ssize_t cec_wakeup_command_enable_store(struct device *dev,
							struct device_attribute *attr,
							const char *buf,
							size_t count)
{
	int i = 0, ret = 0;
	struct mtk_cec_dev *mtk_cec = dev_get_drvdata(dev);
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_enable_poweron_msg *msg =
		(struct cec_svcmsg_enable_poweron_msg *)&svcmsg;
	struct cec_climsg_enable_poweron_msg *reply =
		(struct cec_climsg_enable_poweron_msg *)&mtk_cec->climsg;

	if (mtk_cec->suspended) {
		ret = wait_event_interruptible(mtk_cec->suspend_waitq, !mtk_cec->suspended);
		if (ret < 0)
			return ret;
	}

	msg->msg_id = CEC_SVCMSG_ENABLE_POWERON_MSG; //dont touch this line

	if (buf[0] == '1') // 0x31 = "1"
		msg->enable = 1;
	else
		msg->enable = 0;

	dev_info(&mtk_cec->rpdev->dev, "enable = %x\n", msg->enable);

	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret < 0) {
		dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n", __func__);
		return count;
	}

	ret = wait_for_completion_interruptible(&mtk_cec->ack);
	if (ret < 0)
		return count;

	return count;

}
static ssize_t cec_wakeup_command_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int int_str_size = 0;

	//query HDMI number
	//sysfs_print(buf, &intStrSize, u16Size,6);

	return int_str_size;
}

static ssize_t cec_wakeup_enable_store(struct device *dev,
							struct device_attribute *attr,
							const char *buf,
							size_t count)
{
	int i = 0, ret = 0;
	struct mtk_cec_dev *mtk_cec = dev_get_drvdata(dev);
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_config_wakeup *msg =
		(struct cec_svcmsg_config_wakeup *)&svcmsg;
	struct cec_climsg_config_wakeup *reply =
		(struct cec_climsg_config_wakeup *)&mtk_cec->climsg;

	if (mtk_cec->suspended) {
		ret = wait_event_interruptible(mtk_cec->suspend_waitq, !mtk_cec->suspended);
		if (ret < 0)
			return ret;
	}

	msg->msg_id = CEC_SVCMSG_CONFIG_WAKEUP; //dont touch this line

	if (buf[0] == '1')
		msg->enable = 1;
	else
		msg->enable = 0;

	dev_info(&mtk_cec->rpdev->dev, "enable = %x\n", msg->enable);

	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret < 0) {
		dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n", __func__);
		return count;
	}

	ret = wait_for_completion_interruptible(&mtk_cec->ack);
	if (ret < 0)
		return count;

	return count;

}
static ssize_t cec_wakeup_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int int_str_size = 0;

	//query HDMI number
	//sysfs_print(buf, &intStrSize, u16Size,6);

	return int_str_size;
}

/* ========================== Attribute Functions ========================== */
static ssize_t help_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "Debug Help:\n"
				"- log_level <RW>: To control debug log level.\n"
				"                  Read log_level value.\n");
}

static ssize_t log_level_show(struct device *dev,
							struct device_attribute *attr, char *buf)
{
	struct mtk_cec_dev *mtk_cec = dev_get_drvdata(dev);
	return sprintf(buf, "log_level = %lu\n", mtk_cec->log_level);
}

static ssize_t log_level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int err;
	struct mtk_cec_dev *mtk_cec = dev_get_drvdata(dev);

	err = kstrtoul(buf, 10, &val);
	if (err)
		return err;
	if (val > 255)
		return -EINVAL;

	mtk_cec->log_level = val;
	return count;
}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RW(log_level);
static DEVICE_ATTR_RW(cec_wakeup_command);
static DEVICE_ATTR_RW(cec_wakeup_command_enable);
static DEVICE_ATTR_RW(cec_wakeup_enable);
static DEVICE_ATTR_RW(cec_factory_pin_enable);

static struct attribute *mtk_dtv_cec_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	&dev_attr_cec_wakeup_command.attr,
	&dev_attr_cec_wakeup_command_enable.attr,
	&dev_attr_cec_wakeup_enable.attr,
	&dev_attr_cec_factory_pin_enable.attr,
	NULL,
};
static const struct attribute_group mtk_dtv_cec_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_cec_attrs
};

static int _cec_suspend_prepare(struct mtk_cec_dev *mtk_cec)
{
	int ret;
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_suspend *msg = (struct cec_svcmsg_suspend *)&svcmsg;
	struct cec_climsg_suspend *reply = (struct cec_climsg_suspend *)&mtk_cec->climsg;

	msg->msg_id = CEC_SVCMSG_SUSPEND;
	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret < 0) {
		dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n",  __func__);
		return ret;
	}
	ret = wait_for_completion_interruptible(&mtk_cec->ack);
	if (ret < 0)
		return ret;

	mtk_cec->suspended = true;
	return reply->ret;
}

static int _cec_resume_complete(struct mtk_cec_dev *mtk_cec)
{
	int ret;
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_resume *msg = (struct cec_svcmsg_resume *)&svcmsg;
	struct cec_climsg_resume *reply = (struct cec_climsg_resume *)&mtk_cec->climsg;

	msg->msg_id = CEC_SVCMSG_RESUME;
	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret < 0) {
		dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n",  __func__);
		return ret;
	}
	ret = wait_for_completion_interruptible(&mtk_cec->ack);
	if (ret < 0)
		return ret;

	mtk_cec->suspended = false;
	wake_up_interruptible(&mtk_cec->suspend_waitq);
	return reply->ret;
}

static int _cec_pm_notifier_cb(struct notifier_block *nb,
			unsigned long event, void *ptr)
{
	struct mtk_cec_dev *mtk_cec = (struct mtk_cec_dev *)container_of(nb,
								   struct mtk_cec_dev, pm_notifier);

	switch (event) {
	case PM_SUSPEND_PREPARE:
		_cec_suspend_prepare(mtk_cec);
		break;
	case PM_POST_SUSPEND:
		_cec_resume_complete(mtk_cec);
		break;
	default:
		break;
	}
	return 0;
}

static int mtk_pmu_cec_probe(struct rpmsg_device *rpdev)
{
	struct mtk_cec_dev *mtk_cec;
	int ret = 0;

	dev_info(&rpdev->dev, "[%s] MTK CEC probe start\n", __func__);
	mtk_cec = devm_kzalloc(&rpdev->dev, sizeof(*mtk_cec), GFP_KERNEL);
	if (!mtk_cec)
		return -ENOMEM;

	mtk_cec->rpdev = rpdev;
	init_completion(&mtk_cec->ack);

	spin_lock_init(&mtk_cec->lock);
	INIT_LIST_HEAD(&mtk_cec->event_queue);
	mtk_cec->event_cnt = 0;

	mtk_cec->suspended = false;
	init_waitqueue_head(&mtk_cec->suspend_waitq);

	mtk_cec->adap = cec_allocate_adapter(&mtk_cec_adap_ops, mtk_cec,
			MTK_CEC_NAME, CEC_CAP_DEFAULTS, CEC_MAX_LOG_ADDRS);
	ret = PTR_ERR_OR_ZERO(mtk_cec->adap);
	if (ret) {
		dev_err(&rpdev->dev, "[%s] couldn't create cec adapter\n", __func__);
		goto err_allocate_adapter;
	}

	mtk_cec->pm_notifier.notifier_call = _cec_pm_notifier_cb;
	ret = register_pm_notifier(&mtk_cec->pm_notifier);
	if (ret) {
		dev_err(&rpdev->dev, "[%s] couldn't register pm notifier\n", __func__);
		goto err_register_pm_notifier;
	}
	ret = cec_register_adapter(mtk_cec->adap, &rpdev->dev);
	if (ret) {
		dev_err(&rpdev->dev, "[%s] couldn't register cec device\n", __func__);
		goto err_register_adapter;
	}

	mtk_cec->log_level = DEFAULT_LOG_LEVEL;
	ret = sysfs_create_group(&rpdev->dev.kobj, &mtk_dtv_cec_attr_group);
	if (ret) {
		dev_err(&rpdev->dev, "[%s] Couldn't create sysfs\n", __func__);
		goto err_attr;
	}

	dev_set_drvdata(&rpdev->dev, mtk_cec);
	init_waitqueue_head(&mtk_cec->kthread_waitq);

	mtk_cec->task = kthread_run(mtk_pmu_cec_thread, (void *)mtk_cec, "cec_thread");
	if (IS_ERR(mtk_cec->task)) {
		dev_err(&rpdev->dev, "create thread for add spi failed!\n");
		ret = IS_ERR(mtk_cec->task);
		goto err_kthread_run;
	}
	dev_info(&rpdev->dev, "[%s] MTK CEC successfully probe 0316\n", __func__);

	return 0;

err_kthread_run:
	sysfs_remove_group(&rpdev->dev.kobj, &mtk_dtv_cec_attr_group);

err_attr:
	cec_unregister_adapter(mtk_cec->adap);

err_register_adapter:
	unregister_pm_notifier(&mtk_cec->pm_notifier);

err_register_pm_notifier:
	cec_delete_adapter(mtk_cec->adap);

err_allocate_adapter:
	kfree(mtk_cec);

	return ret;
}

static void mtk_pmu_cec_remove(struct rpmsg_device *rpdev)
{
	struct mtk_cec_dev *mtk_cec = dev_get_drvdata(&rpdev->dev);

	kthread_stop(mtk_cec->task);
	sysfs_remove_group(&rpdev->dev.kobj, &mtk_dtv_cec_attr_group);
	cec_unregister_adapter(mtk_cec->adap);
	unregister_pm_notifier(&mtk_cec->pm_notifier);
	cec_delete_adapter(mtk_cec->adap);
	kfree(mtk_cec);

	dev_info(&rpdev->dev, "%s\n", __func__);
}


static void mtk_cec_shutdown(struct device *dev)
{
	struct mtk_cec_dev *mtk_cec = dev_get_drvdata(dev);
	int ret;
	struct cec_generic_msg svcmsg = { 0 };
	struct cec_svcmsg_suspend *msg = (struct cec_svcmsg_suspend *)&svcmsg;
	struct cec_climsg_suspend *reply = (struct cec_climsg_suspend *)&mtk_cec->climsg;

	pr_info("%s\n", __func__);
	msg->msg_id = CEC_SVCMSG_SUSPEND;
	ret = rpmsg_send(mtk_cec->rpdev->ept, (void *)&svcmsg, sizeof(svcmsg));
	if (ret < 0)
		dev_err(&mtk_cec->rpdev->dev, "%s:rpmsg_send fail!\n",  __func__);

	ret = wait_for_completion_interruptible(&mtk_cec->ack);
	pr_info("%s ret = %x\n", __func__, ret);
}

static struct rpmsg_driver mtk_pmu_cec_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.drv.shutdown	= mtk_cec_shutdown,
	.id_table	= mtk_pmu_cec_id_table,
	.probe		= mtk_pmu_cec_probe,
	.callback	= mtk_pmu_cec_callback, //triggered when received rpmsg.
	.remove		= mtk_pmu_cec_remove,
};

static int __init mtk_pmu_cec_init(void)
{
	int ret = 0;

	ret = register_rpmsg_driver(&mtk_pmu_cec_driver);

	if (ret) {
		pr_err("rpmsg bus driver register failed (%d)\n", ret);
		return ret;
	}
	pr_info("rpmsg bus driver register done (%d)\n", ret);
	return ret;
}

static void __exit mtk_pmu_cec_exit(void)
{
	unregister_rpmsg_driver(&mtk_pmu_cec_driver);
}

module_init(mtk_pmu_cec_init);
module_exit(mtk_pmu_cec_exit);

MODULE_AUTHOR("daniel-cc.liu@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek dtv cec pmu driver");
