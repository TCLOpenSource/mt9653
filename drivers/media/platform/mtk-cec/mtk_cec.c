// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/timer.h>
#include <media/cec.h>

#include "mtk_cec.h"
#include "mtk_cec_hw.h"


static int mtk_cec_adap_enable(struct cec_adapter *adap, bool enable)
{
	struct mtk_cec_dev *cec_dev = cec_get_drvdata(adap);

	if (enable) {
		cec_dev->adap->phys_addr = 0x0000;
		mtk_cec_enable(cec_dev);
	} else
		mtk_cec_disable(cec_dev);

	return 0;
}

static int mtk_cec_adap_log_addr(struct cec_adapter *adap, u8 addr)
{
	struct mtk_cec_dev *cec_dev = cec_get_drvdata(adap);

	mtk_cec_setlogicaladdress(cec_dev, addr);
	return 0;
}


static irqreturn_t mtk_cec_irq_handler(int irq, void *priv)
{
	struct mtk_cec_dev *cec_dev = priv;
	u8 status;

	status = mtk_cec_get_event_status(cec_dev);
	dev_dbg(cec_dev->dev, "[%s] IRQ status = %x\n", __func__, status);

	if (status & CEC_RECEIVE_SUCCESS) {
		if (cec_dev->rx != STATE_IDLE)
			dev_dbg(cec_dev->dev, "[%s] Buffer overrun\n", __func__);

		cec_dev->rx = STATE_BUSY;
		mtk_cec_get_receive_message(cec_dev);
		cec_dev->rx = STATE_DONE;
		mtk_cec_clear_event_status(cec_dev, (CEC_RECEIVE_SUCCESS<<8));

	}
	if (status & CEC_SNED_SUCCESS) {
		cec_dev->tx = STATE_DONE;
		mtk_cec_clear_event_status(cec_dev, (CEC_SNED_SUCCESS<<8));

	}
	if (status & CEC_RETRY_FAIL) {
		cec_dev->tx = STATE_NACK;
		mtk_cec_clear_event_status(cec_dev, (CEC_RETRY_FAIL<<8));

	}
	if (status & CEC_LOST_ARBITRRATION) {
		cec_dev->tx = STATE_ERROR;
		mtk_cec_clear_event_status(cec_dev, (CEC_LOST_ARBITRRATION<<8));

	}
	if (status & CEC_TRANSMIT_NACK) {
		cec_dev->tx = STATE_NACK;
		mtk_cec_clear_event_status(cec_dev, (CEC_TRANSMIT_NACK<<8));
	}

	return IRQ_WAKE_THREAD;
}

static irqreturn_t mtk_cec_irq_handler_thread(int irq, void *priv)
{
	struct mtk_cec_dev *cec_dev = priv;
	unsigned int len = 0, index = 0;

	switch (cec_dev->rx) {
	case STATE_DONE:
		{
			dev_dbg(cec_dev->dev, "[%s] cec_received_msg\n", __func__);
			len = cec_dev->msg.len;
			dev_dbg(cec_dev->dev, "[%s] len = %x\n", __func__, len);

			for (index = 0; index < len; index++) {
				dev_dbg(cec_dev->dev, "[%s] msg = %x\n", __func__,
					cec_dev->msg.msg[index]);
			}

		    cec_received_msg(cec_dev->adap, &cec_dev->msg);
		    cec_dev->rx = STATE_IDLE;
		    //dev_info(pstCECDev->dev, "[%s] Rx: STATE_DONE\n", __func__);
		}
		break;
	default:
		break;
	}

	switch (cec_dev->tx) {
	case STATE_DONE:
		{
			cec_transmit_attempt_done(cec_dev->adap, CEC_TX_STATUS_OK);
			cec_dev->tx = STATE_IDLE;
			dev_dbg(cec_dev->dev, "[%s] Tx: STATE_DONE\n", __func__);
		}
		break;

	case STATE_NACK:
		{
			cec_transmit_attempt_done(cec_dev->adap, CEC_TX_STATUS_NACK);
			cec_dev->tx = STATE_IDLE;
			dev_dbg(cec_dev->dev, "[%s] Tx: STATE_NACK\n", __func__);
		}
		break;

	case STATE_ERROR:
		{
			cec_transmit_attempt_done(cec_dev->adap, CEC_TX_STATUS_ARB_LOST);
			cec_dev->tx = STATE_IDLE;
			dev_dbg(cec_dev->dev, "[%s] Tx: STATE_ERROR\n", __func__);
		}
		break;
	default:
		break;
	}

	return IRQ_HANDLED;
}

static int mtk_cec_adap_transmit(struct cec_adapter *adap, u8 attempts,
							u32 signal_free_time, struct cec_msg *msg)
{
	// TODO: this function need to modify later: use rpmsg function to communicate

	struct mtk_cec_dev *cec_dev = cec_get_drvdata(adap);

	dev_dbg(cec_dev->dev, "[%s] triggered\n", __func__);
	if (!(mtk_cec_sendframe(cec_dev, msg->msg, msg->len)))
		return STATE_BUSY;

	return 0;
}

static const struct cec_adap_ops stCEC_adap_ops = {
	.adap_enable	=	mtk_cec_adap_enable,
	.adap_log_addr	=	mtk_cec_adap_log_addr,
	.adap_transmit	=	mtk_cec_adap_transmit,
};


static ssize_t cec_wakeup_command_store(struct device *dev,
							struct device_attribute *attr,
							const char *buf,
							size_t count)
{
	int i = 0;

	dev_dbg(dev, "[%s] function called\n", __func__);
	dev_dbg(dev, "[%s] count = %x", __func__, count);
	dev_dbg(dev, "[%s] buf = ", __func__);

	for (i = 0; i < count; i++)
		dev_dbg(dev, "[%s] buf[%d] =%X ", __func__, i, buf[i]);

	return count;
}
static ssize_t cec_wakeup_command_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16_size = 65535;
	int int_str_size = 0;

	//query HDMI number
	//sysfs_print(buf, &intStrSize, u16Size,6);

	return int_str_size;
}

static ssize_t cec_wakeup_config_store(struct device *dev,
							struct device_attribute *attr,
							const char *buf,
							size_t count)
{
	int i = 0;

	dev_dbg(dev, "[%s] function called\n", __func__);
	dev_dbg(dev, "[%s] count = %x", __func__, count);
	dev_dbg(dev, "[%s] buf = ", __func__);

	for (i = 0; i < count; i++)
		dev_dbg(dev, "[%s] buf[%d] =%X ", __func__, i, buf[i]);

	return count;
}
static ssize_t cec_wakeup_config_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16_size = 65535;
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
	int i = 0;

	dev_dbg(dev, "[%s] function called\n", __func__);
	dev_dbg(dev, "[%s] count = %x", __func__, count);

	dev_dbg(dev, "[%s] buf = ", __func__);

	for (i = 0; i < count; i++)
		dev_dbg(dev, "[%s] buf[%d] =%X ", __func__, i, buf[i]);

	return count;
}
static ssize_t cec_wakeup_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16_size = 65535;
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
static DEVICE_ATTR_RW(cec_wakeup_enable);
static DEVICE_ATTR_RW(cec_wakeup_config);




static struct attribute *mtk_dtv_cec_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	&dev_attr_cec_wakeup_command.attr,
	&dev_attr_cec_wakeup_enable.attr,
	&dev_attr_cec_wakeup_config.attr,
	NULL,
};
static const struct attribute_group mtk_dtv_cec_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_cec_attrs
};


static int mtk_cec_probe(struct platform_device *pdev)
{
	struct mtk_cec_dev *pdata;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *dn;
	int ret = 0;

	dev_info(&pdev->dev, "[%s] CEC probe start\n", __func__);

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
/*
	if (!pdata) {
		dev_err(&pdev->dev, "[%s] Get resource fail\n", __func__);
		return -ENOMEM;
	}
*/
	platform_set_drvdata(pdev, pdata);
	pdata->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pdata->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->reg_base)) {
		dev_err(&pdev->dev, "err reg_base=%#lx\n", (unsigned long)pdata->reg_base);
		return PTR_ERR(pdata->reg_base);
	}
	dev_info(&pdev->dev, "reg_base=%#lx\n", (unsigned long)pdata->reg_base);

	dn = pdev->dev.of_node;

	pdata->adap = cec_allocate_adapter(&stCEC_adap_ops, pdata, "mediatek,cec",
							CEC_CAP_DEFAULTS, CEC_MAX_LOG_ADDRS);
	ret = PTR_ERR_OR_ZERO(pdata->adap);
	if (ret) {
		dev_err(&pdev->dev, "[%s] Couldn't create cec adapter\n", __func__);
		return ret;
	}

	ret = cec_register_adapter(pdata->adap, &pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "[%s] Couldn't register device\n", __func__);
		goto err_delete_adapter;
	}

	pdata->irq = platform_get_irq(pdev, 0);
	if (pdata->irq <= 0) {
		dev_err(&pdev->dev, "[%s] no IRQ resource\n", __func__);
		return pdata->irq;
	}

	ret = devm_request_threaded_irq(dev, pdata->irq, mtk_cec_irq_handler,
		mtk_cec_irq_handler_thread, 0, pdev->name, pdata);
	if (ret) {
		dev_err(&pdev->dev, "[%s] Fail to request thread irq\n", __func__);
		return ret;
	}

	pdata->log_level = 4;

	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_dtv_cec_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "[%s] Couldn't create sysfs\n", __func__);
		goto err_attr;
	}

	mtk_cec_enable(pdata);

	dev_info(&pdev->dev, "[%s] CEC successfully probe 0115\n", __func__);

	return 0;

err_attr:
	dev_info(&pdev->dev, "Remove device attribute files\n");
	sysfs_remove_group(&pdev->dev.kobj, &mtk_dtv_cec_attr_group);

err_delete_adapter:
	dev_err(&pdev->dev, "err_delete_adapter FAIL\n");
	cec_delete_adapter(pdata->adap);
	return ret;

}

static int mtk_cec_remove(struct platform_device *pdev)
{
	struct mtk_cec_dev *cec_dev = dev_get_drvdata(&pdev->dev);

	cec_unregister_adapter(cec_dev->adap);
	pm_runtime_disable(cec_dev->dev);

	return 0;
}

static int mtk_cec_suspend(struct device *dev)
{
	struct mtk_cec_dev *cec = dev_get_drvdata(dev);

	mtk_cec_config_wakeup(cec);
	dev_info(dev, "CEC suspend done\n");
	return 0;
}

static int mtk_cec_resume(struct device *dev)
{
	struct mtk_cec_dev *cec = dev_get_drvdata(dev);

	mtk_cec_enable(cec);
	dev_info(dev, "CEC resume done\n");
	return 0;
}

static const struct dev_pm_ops mtk_cec_pm_ops = {
	.suspend = mtk_cec_suspend,
	.resume = mtk_cec_resume,
};

static const struct of_device_id mtk_cec_match[] = {
	{
		.compatible = "mediatek,cec",
	},
	{},
};
MODULE_DEVICE_TABLE(of, mtk_cec_match);

static struct platform_driver mtk_cec_pdrv = {
	.probe	=	mtk_cec_probe,
	.remove	=	mtk_cec_remove,
	.driver	=	{
		.name	=	"mediatek,cec",
		.of_match_table	=	mtk_cec_match,
		.pm = &mtk_cec_pm_ops,
	},
};


module_platform_driver(mtk_cec_pdrv);

MODULE_AUTHOR("Mediatek TV");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mediatek CEC driver");
