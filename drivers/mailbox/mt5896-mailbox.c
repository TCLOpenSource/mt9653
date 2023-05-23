// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
//#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <soc/mediatek/mtk-mailbox.h>
#include <linux/delay.h>

#include "mt5896-mailbox-riu.h"
#include "mt5896-mailbox-coda.h"

#define MTK_MBOX_LOG_NONE	(-1)	/* set mtk loglevel to be useless */
#define MTK_MBOX_LOG_MUTE	(0)
#define MTK_MBOX_LOG_ERR	(3)
#define MTK_MBOX_LOG_WARN	(4)
#define MTK_MBOX_LOG_INFO	(6)
#define MTK_MBOX_LOG_DEBUG	(7)

#define mbox_log(mbox_dev, level, args...) \
do { \
	if (mbox_dev->log_level < 0) { \
		if (level == MTK_MBOX_LOG_DEBUG) \
			dev_dbg(mbox_dev->dev, ## args); \
		else if (level == MTK_MBOX_LOG_INFO) \
			dev_info(mbox_dev->dev, ## args); \
		else if (level == MTK_MBOX_LOG_WARN) \
			dev_warn(mbox_dev->dev, ## args); \
		else if (level == MTK_MBOX_LOG_ERR) \
			dev_err(mbox_dev->dev, ## args); \
	} else if (mbox_dev->log_level >= level) \
		dev_emerg(mbox_dev->dev, ## args); \
} while (0)

#define MTK_MBOX_HDR_FREE_MSK		BIT(7)
#define MTK_MBOX_HDR_ID_MSK		GENMASK(6, 0)
#define MTK_MBOX_SIZE			(16)
#define MTK_MBOX_RSV_LEN		(1)
#define MTK_MBOX_MSG_LEN		(mbox->packet_size - 1 - MTK_MBOX_RSV_LEN)
#define MTK_MBOX_TXPOLL_PERIOD		(1)
#define MTK_MBOX_TX_DONE_LOOP_MAX	(100)
/* ns */
#define MTK_MBOX_TX_DONE_LOOP_DELAY	(10)
#define MTK_MBOX_CHAN_NUM		(128)
#define MTK_MBOX_IPI_FIRE_BIT_NUM_MAX	15
/* total 32 bytes for one channel, include tx(16 bytes) and rx(16 bytes) */
#define MTK_MBOX_TX_START_OFFSET	(0)
#define MTK_MBOX_RX_START_OFFSET	(mbox->packet_size)
#define MTK_MBOX_NOT_MEXICAN		(-1)

struct mt5896_mbox_chan_tx {
	int id;
	struct list_head list;
	u8 *msg;
	bool done;
};

struct mt5896_mbox {
	struct device *dev;
	void __iomem *regs;
	void __iomem *ctrl_reg;
	spinlock_t lock;
	struct mbox_controller controller;
	struct list_head tx_list;
	u8 *rx_msg;
	int log_level;
	int packet_size;
	u32 ipi_fire;
	int mexican_id;
};

static inline void __mt5896_fill_mbox(struct mt5896_mbox *mbox, u8 *data)
{
	int i;
	u16 val;
	u8 *ptr = data + sizeof(val);

	// fill mail box msg content first
	for (i = sizeof(val); i < mbox->packet_size; i += sizeof(val)) {
		val = (u16)(*ptr);
		if (i != mbox->packet_size - 1)
			val |= (u16)(*(ptr + 1)) << BITS_PER_BYTE;

		mtk_writew(mbox->regs + (MTK_MBOX_TX_START_OFFSET << 1) + ((i >> 1) * sizeof(u32)), val);
		mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "(w)mbox:(%d) 0x%04X\n", i, val);
		ptr += sizeof(val);
	}

	// fill mail box header
	val = (u16)(*data);
	val |= (u16)(*(data + 1)) << BITS_PER_BYTE;
	mtk_writew(mbox->regs + (MTK_MBOX_TX_START_OFFSET << 1), val);
}

static inline void __mt5896_get_mbox(struct mt5896_mbox *mbox, u8 *data)
{
	int i;
	u16 val;

	for (i = 0 ; i < mbox->packet_size ; i += sizeof(val)) {
		val = mtk_readw(mbox->regs + (MTK_MBOX_RX_START_OFFSET << 1) + ((i >> 1) * sizeof(u32)));
		mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "(r)mbox:(%d) 0x%04X\n", i, val);

		*data = val & U8_MAX;
		data++;

		if (i == mbox->packet_size - 1)
			break;

		*data = val >> BITS_PER_BYTE;
		data++;
	}
}

static irqreturn_t mt5896_mbox_irq(int irq, void *dev_id)
{
	struct mt5896_mbox *mbox = dev_id;
	struct mbox_chan *link;
	unsigned int rx_ch;

	/* get mailbox msg */
	__mt5896_get_mbox(mbox, mbox->rx_msg);
	/* retrieve channel num */
	rx_ch = mbox->rx_msg[0] & ~MTK_MBOX_HDR_FREE_MSK;
	if (rx_ch >= MTK_MBOX_CHAN_NUM)
		/* invalid rx channel */
		return IRQ_HANDLED;

	/* report msg */
	link = &mbox->controller.chans[rx_ch];
	/* message header include 1 byte channel id and 2 bytes reserved */
	if (link->cl)
		mbox_chan_received_data(link, &mbox->rx_msg[1 + MTK_MBOX_RSV_LEN]);
	/* clear mailbox dirty bit */
	mtk_writeb(mbox->regs + (MTK_MBOX_RX_START_OFFSET << 1), rx_ch);

	return IRQ_HANDLED;
}

static void _mt5896_send_start(struct mt5896_mbox *mbox, struct mt5896_mbox_chan_tx *tx_mail)
{
	u8 reg;

	mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "send ch %d\n", tx_mail->id);
	/* fill mailbox msg */
	__mt5896_fill_mbox(mbox, tx_mail->msg);
	/* trigger remote interrupt */
	reg = mtk_readb(mbox->ctrl_reg);
	mtk_writeb(mbox->ctrl_reg, reg | BIT(mbox->ipi_fire));
	mtk_writeb(mbox->ctrl_reg, reg & ~BIT(mbox->ipi_fire));
}

static int mt5896_send_data(struct mbox_chan *link, void *data)
{
	struct mt5896_mbox *mbox = dev_get_drvdata(link->mbox->dev);
	struct mt5896_mbox_chan_tx *tx_mail = link->con_priv;
	struct mt5896_mbox_chan_tx *checked_tx_mail = NULL, *n;
	bool mbox_chan_already_in_tx_list = false;

	mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "add ch %d to send list.\n", tx_mail->id);

	spin_lock(&mbox->lock);

	*tx_mail->msg = tx_mail->id | MTK_MBOX_HDR_FREE_MSK;
	memcpy(tx_mail->msg + 1 + MTK_MBOX_RSV_LEN, data, MTK_MBOX_MSG_LEN);
	tx_mail->done = false;

	// traverse tx list to get requested channel status
	list_for_each_entry_safe(checked_tx_mail, n, &mbox->tx_list, list) {
		if (checked_tx_mail->id == tx_mail->id) {
			mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "ch %d already in tx list\n",
				 checked_tx_mail->id);
			mbox_chan_already_in_tx_list = true;
			break;
		}
	}

	if (mbox_chan_already_in_tx_list == false) {
		// add node into list
		INIT_LIST_HEAD(&tx_mail->list);
		list_add_tail(&tx_mail->list, &mbox->tx_list);
	}

	if (list_is_first(&tx_mail->list, &mbox->tx_list))
		// no send in progress, start send
		_mt5896_send_start(mbox, tx_mail);

	spin_unlock(&mbox->lock);

	return 0;
}

static bool mt5896_last_tx_done(struct mbox_chan *link)
{
	struct mt5896_mbox *mbox = dev_get_drvdata(link->mbox->dev);
	struct mt5896_mbox_chan_tx *checked_tx_mail = link->con_priv, *n;
	unsigned int ch_in_check = checked_tx_mail->id;
	u8 mbox_reg_hdr = mtk_readb(mbox->regs);
	unsigned int ch_in_progress = mbox_reg_hdr & MTK_MBOX_HDR_ID_MSK;
	bool ret = false;
	int cnt = 0;

	mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "check ch %d tx done, progress header 0x%02X\n",
		 ch_in_check, mbox_reg_hdr);

	spin_lock(&mbox->lock);

	while (true) {
		if ((!(mbox_reg_hdr & MTK_MBOX_HDR_FREE_MSK)) || (cnt > MTK_MBOX_TX_DONE_LOOP_MAX))
			break;

		ndelay(MTK_MBOX_TX_DONE_LOOP_DELAY);
		mbox_reg_hdr = mtk_readb(mbox->regs);
		cnt++;
	};

	// check current channel tx status
	if (!(mbox_reg_hdr & MTK_MBOX_HDR_FREE_MSK)) {
		struct mt5896_mbox_chan_tx *progress_tx_mail;

		// current channel status is tx done
		progress_tx_mail = mbox->controller.chans[ch_in_progress].con_priv;
		progress_tx_mail->done = true;

		if (!list_is_last(&progress_tx_mail->list, &mbox->tx_list))
			// have remain element in list, send next tx mail in list
			_mt5896_send_start(mbox, list_next_entry(progress_tx_mail, list));
	}

	// traverse tx list to get requested channel status
	list_for_each_entry_safe(checked_tx_mail, n, &mbox->tx_list, list) {
		mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "ch %d in tx list\n", checked_tx_mail->id);
		if (checked_tx_mail->id == ch_in_check)
			// get requested channel, break loop
			break;
	}

	// get channel tx status
	ret = checked_tx_mail->done;
	if (ret)
		// will report tx done, remove tx node in tx list
		list_del(&checked_tx_mail->list);

	spin_unlock(&mbox->lock);

	return ret;
}

static const struct mbox_chan_ops mt5896_mbox_chan_ops = {
	.send_data	= mt5896_send_data,
	.last_tx_done	= mt5896_last_tx_done
};

/**
 * mt5896_mbox_set_chan_id() - set channel ID for requesting
 * @cntlr_dev: mbox controller device
 * @id: ID for requesting
 *
 * Return 0 if success. Or negative errno on failure.
 */
int mt5896_mbox_set_chan_id(struct device *cntlr_dev, int id)
{
	struct mt5896_mbox *mbox = dev_get_drvdata(cntlr_dev);

	if ((id >= MTK_MBOX_CHAN_NUM) || (id < 0))
		return -EINVAL;

	if (!mbox)
		return -ENODEV;

	if (mbox->mexican_id != MTK_MBOX_NOT_MEXICAN)
		return -EBUSY;

	mbox->mexican_id = id;
	dev_info(cntlr_dev, "%s: mexican_id = %d\n", __func__, mbox->mexican_id);

	return 0;
}
EXPORT_SYMBOL(mt5896_mbox_set_chan_id);

static struct mbox_chan *mt5896_mbox_index_xlate(struct mbox_controller *con,
		    const struct of_phandle_args *sp)
{
	struct mt5896_mbox *mbox = dev_get_drvdata(con->dev);
	int id;

	if (sp->args_count != 1)
		return ERR_PTR(-EINVAL);

	if (sp->args[0] >= MTK_MBOX_CHAN_NUM)
		return ERR_PTR(-EINVAL);

	if (!mbox)
		return ERR_PTR(-EINVAL);

	if ((mbox->mexican_id >= 0) && (mbox->mexican_id < MTK_MBOX_CHAN_NUM)) {
		id = mbox->mexican_id;
		dev_info(mbox->dev, "%s: set from mexican_id = %d\n", __func__, mbox->mexican_id);
		mbox->mexican_id = MTK_MBOX_NOT_MEXICAN;
	} else {
		id = sp->args[0];
	}

	return &con->chans[id];
}

static ssize_t txdone_poll_period_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t n)
{
	struct mt5896_mbox *mbox = dev_get_drvdata(dev);

	if (buf) {
		unsigned long value;

		if (kstrtoul(buf, 10, &value))
			return -EINVAL;

		spin_lock(&mbox->lock);

		mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "tx done polling period %ld\n", value);
		mbox->controller.txpoll_period = value;

		spin_unlock(&mbox->lock);

		return n;
	}

	return -EINVAL;
}

static ssize_t txdone_poll_period_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mt5896_mbox *mbox = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	spin_lock(&mbox->lock);

	mbox_log(mbox, MTK_MBOX_LOG_DEBUG, "tx done polling period %d\n",
		 mbox->controller.txpoll_period);
	str += scnprintf(str, end - str, "%d\n", mbox->controller.txpoll_period);

	spin_unlock(&mbox->lock);

	return (str - buf);
}

DEVICE_ATTR_RW(txdone_poll_period);

/* mtk_dbg sysfs group */
static ssize_t help_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Debug Help:\n"
		       "- log_level <RW>: To control debug log level.\n"
		       "                  Read log_level for the definition of log level number.\n");
}

static DEVICE_ATTR_RO(help);

static ssize_t log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mt5896_mbox *mbox = dev_get_drvdata(dev);

	return sprintf(buf, "none:\t\t%d\nmute:\t\t%d\nerror:\t\t%d\n"
		       "warning:\t\t%d\ninfo:\t\t%d\ndebug:\t\t%d\n"
		       "current:\t%d\n",
		       MTK_MBOX_LOG_NONE, MTK_MBOX_LOG_MUTE, MTK_MBOX_LOG_ERR,
		       MTK_MBOX_LOG_WARN, MTK_MBOX_LOG_INFO, MTK_MBOX_LOG_DEBUG,
		       mbox->log_level);
}

static ssize_t log_level_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct mt5896_mbox *mbox = dev_get_drvdata(dev);
	ssize_t retval;
	int level;

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < MTK_MBOX_LOG_NONE) || (level > MTK_MBOX_LOG_DEBUG))
			retval = -EINVAL;
		else
			mbox->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static DEVICE_ATTR_RW(log_level);

static struct attribute *mt5896_mbox_dbg_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	NULL,
};

static const struct attribute_group mt5896_mbox_attr_group = {
	.name = "mtk_dbg",
	.attrs = mt5896_mbox_dbg_attrs,
};

static int mt5896_mbox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = NULL;
	int ret = 0;
	struct resource *iomem;
	struct mt5896_mbox *mbox;
	unsigned int i;

	np = pdev->dev.of_node;
	if (!np) {
		dev_err(&pdev->dev, "fail to find node\n");
		return -EINVAL;
	}

	mbox = devm_kzalloc(dev, sizeof(*mbox), GFP_KERNEL);
	if (mbox == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, mbox);
	spin_lock_init(&mbox->lock);
	mbox->dev = dev;
	INIT_LIST_HEAD(&mbox->tx_list);

	/* mailbox content resource */
	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mbox->regs = devm_ioremap_resource(&pdev->dev, iomem);
	if (IS_ERR(mbox->regs)) {
		ret = PTR_ERR(mbox->regs);
		dev_err(&pdev->dev, "Failed to remap mailbox regs: %d\n", ret);
		return ret;
	}

	/* trigger rproc intr resource, maybe share io with other driver */
	mbox->ctrl_reg = of_iomap(pdev->dev.of_node, 1);
	if (IS_ERR(mbox->ctrl_reg)) {
		ret = PTR_ERR(mbox->ctrl_reg);
		dev_err(&pdev->dev, "Failed to remap mailbox control regs: %d\n", ret);
		return ret;
	}

	/* host to host interrupt trigger bit */
	ret = of_property_read_u32(np, "ipi-fire", &mbox->ipi_fire);
	if (ret) {
		dev_err(dev, "cannot get ipi-fire\n");
		return -EINVAL;
	}

	if (mbox->ipi_fire > MTK_MBOX_IPI_FIRE_BIT_NUM_MAX) {
		dev_err(dev, "invalid ipi-fire: %d\n", mbox->ipi_fire);
		return -EINVAL;
	}

	/* mbox packet size */
	ret = of_property_read_u32(np, "packet-size", &mbox->packet_size);
	if (ret) {
		mbox->packet_size = MTK_MBOX_SIZE;
		dev_warn(dev, "cannot get packet-size, use default: %d\n", mbox->packet_size);
	}

	/* init mexican flag */
	mbox->mexican_id = MTK_MBOX_NOT_MEXICAN;

	/* allocate rx msg buffer */
	mbox->rx_msg = devm_kzalloc(dev, mbox->packet_size, GFP_KERNEL);
	if (!mbox->rx_msg)
		return -ENOMEM;

	memset(mbox->rx_msg, 0, mbox->packet_size);

	mbox->controller.txdone_irq = false;
	mbox->controller.txdone_poll = true;
	mbox->controller.txpoll_period = MTK_MBOX_TXPOLL_PERIOD;
	mbox->controller.ops = &mt5896_mbox_chan_ops;
	mbox->controller.of_xlate = &mt5896_mbox_index_xlate;
	mbox->controller.dev = dev;
	mbox->controller.num_chans = MTK_MBOX_CHAN_NUM;
	mbox->controller.chans =
	    devm_kzalloc(dev, sizeof(*mbox->controller.chans) * mbox->controller.num_chans,
			 GFP_KERNEL);
	if (!mbox->controller.chans)
		return -ENOMEM;

	for (i = 0; i < mbox->controller.num_chans; i++) {
		struct mt5896_mbox_chan_tx *chan_tx;

		chan_tx = devm_kzalloc(dev, sizeof(*chan_tx), GFP_KERNEL);
		if (!chan_tx)
			return -ENOMEM;

		chan_tx->msg = devm_kzalloc(dev, mbox->packet_size, GFP_KERNEL);
		if (!chan_tx->msg)
			return -ENOMEM;

		memset(chan_tx->msg, 0, mbox->packet_size);

		chan_tx->id = i;
		INIT_LIST_HEAD(&chan_tx->list);
		chan_tx->done = true;
		mbox->controller.chans[i].con_priv = (void *)chan_tx;
	}

	ret = devm_mbox_controller_register(dev, &mbox->controller);
	if (ret)
		return ret;

	/*
	 * create sysfs file for adjust txdone polling period, if sysfs file creation failed,
	 * txdone polling period can not adjust but mailbox still works with default value.
	 */
	ret = device_create_file(dev, &dev_attr_txdone_poll_period);
	if (ret)
		dev_err(dev, "failed to create file node for adjust txdone polling period (%d)\n",
			ret);

	/* mtk loglevel */
	mbox->log_level = MTK_MBOX_LOG_NONE;

	/*
	 * create mtk_dbg sysfs group, if sysfs group creation failed, mtk_dbg function can not work
	 * but MBOX still works.
	 */
	sysfs_create_group(&pdev->dev.kobj, &mt5896_mbox_attr_group);

	platform_set_drvdata(pdev, mbox);

	/* irq resource */
	ret = devm_request_irq(dev, irq_of_parse_and_map(dev->of_node, 0), mt5896_mbox_irq, 0,
			       dev_name(dev), mbox);
	if (ret) {
		dev_err(dev, "Failed to register a mailbox IRQ handler: %d\n", ret);
		return -ENODEV;
	}

	dev_info(dev, "mailbox enabled\n");

	return ret;
}

static int mt5896_mbox_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_txdone_poll_period);
	sysfs_remove_group(&pdev->dev.kobj, &mt5896_mbox_attr_group);

	return 0;
}

static const struct of_device_id mt5896_mbox_of_match[] = {
	{ .compatible = "mediatek,mt5896-mbox", },
	{},
};
MODULE_DEVICE_TABLE(of, mt5896_mbox_of_match);

static struct platform_driver mt5896_mbox_driver = {
	.driver = {
		.name = "mt5896-mbox",
		.of_match_table = mt5896_mbox_of_match,
#ifndef MODULE
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
#endif
	},
	.probe = mt5896_mbox_probe,
	.remove = mt5896_mbox_remove,
};
module_platform_driver(mt5896_mbox_driver);

MODULE_AUTHOR("Benjamin Lin <benjamin.lin@mediatek.com>");
MODULE_DESCRIPTION("MediaTek MT5896 mailbox Bus Driver");
MODULE_LICENSE("GPL v2");
