// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <soc/mediatek/mtk-mailbox.h>

/* be the same defined in dt-bindings mt5896_mbox_chan_id_maps.h */
#define MTK_MBOX_REMOTE_PM51	1
#define MTK_MBOX_REMOTE_CM4	2
#define MTK_MBOX_REMOTE_FRCR2	3
#define MTK_MBOX_REMOTE_SECR2	4

static LIST_HEAD(mbox_mexicans);
static DEFINE_MUTEX(mexican_mutex);

struct mexico_dealer {
	struct list_head node;
	struct mbox_chan *mchan;
	struct mbox_client mclient;
	int chan_id;
};

struct mbox_mexico {
	struct device *dev;
	struct device *cntlr_dev;
	struct list_head node;
	struct list_head dealers;
	int to_remote;
};

/**
 * mtk_mbox_request_channel_by_id() - request mbox channel handle by ID
 * @to_remote: remote co-processor to communicate
 * @id: ID for requesting
 * @mbox_cb: callback function, be executed when mbox message received
 *
 * Return NULL on failure, else success.
 */
struct mbox_chan *mtk_mbox_request_channel_by_id(int to_remote, int id, void *mbox_cb)
{
	struct mbox_mexico *mexican;
	struct mexico_dealer *dealer;
	int ret = -ENODEV;
	bool dealer_exist = false;

	pr_debug("%s: tx block tout = 5000, to_remote = %d, id = %d\n", __func__, to_remote, id);

	mutex_lock(&mexican_mutex);

	list_for_each_entry(mexican, &mbox_mexicans, node)
		if (mexican->to_remote == to_remote) {
			/* request channel */
			list_for_each_entry(dealer, &mexican->dealers, node)
				if (dealer->chan_id == id) {
					dealer_exist = true;
					dev_dbg(mexican->dev,
						"%s: dealer with id(%d) existed\n",
						__func__, id);
					break;
				}

			if (!dealer_exist) {
				ret = mt5896_mbox_set_chan_id(mexican->cntlr_dev, id);
				if (ret) {
					dev_err(mexican->dev,
						"%s: set chan id failed(%d) to_remote = %d, id = %d\n",
						__func__, ret, to_remote, id);
					mutex_unlock(&mexican_mutex);
					return NULL;
				}

				dealer = devm_kzalloc(mexican->dev, sizeof(*dealer), GFP_KERNEL);
				dev_info(mexican->dev,
					 "%s: create dealer with id(%d)\n",
					 __func__, id);

				dealer->mclient.dev = mexican->dev;
				dealer->mclient.tx_block = true;
				dealer->mclient.tx_tout = 5000;
				dealer->mclient.knows_txdone = false;
				dealer->mclient.rx_callback = mbox_cb,
				dealer->mclient.tx_prepare = NULL;
				dealer->mclient.tx_done = NULL;
				dealer->chan_id = id;
				dealer->mchan = mbox_request_channel(&dealer->mclient, 0);
			}

			if (IS_ERR(dealer->mchan)) {
				dev_err(mexican->dev,
					"%s: request channel failed(%ld) to_remote = %d, id = %d\n",
					__func__, PTR_ERR(dealer->mchan), to_remote, id);
				devm_kfree(mexican->dev, dealer);
				mutex_unlock(&mexican_mutex);
				return NULL;
			}

			/* request channel pass, add in dealers list */
			if (!dealer_exist) {
				list_add_tail(&dealer->node, &mexican->dealers);
				dev_info(mexican->dev,
					 "%s: add list dealer with id(%d)\n",
					 __func__, id);
			}

			mutex_unlock(&mexican_mutex);

			return dealer->mchan;
		}

	pr_err("%s: cannot get match mexican to_remote = %d, id = %d\n",
	       __func__, to_remote, id);

	mutex_unlock(&mexican_mutex);

	return NULL;
}
EXPORT_SYMBOL(mtk_mbox_request_channel_by_id);

static int mt5896_mbox_mexico_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = NULL;
	struct device_node *cntlr_np = NULL;
	struct platform_device *cntlr_pdev = NULL;
	struct device *cntlr_dev = NULL;
	struct mbox_mexico *mexican;
	struct of_phandle_args spec;

	np = pdev->dev.of_node;
	if (!np) {
		dev_err(&pdev->dev, "fail to find node\n");
		return -EINVAL;
	}

	mexican = devm_kzalloc(dev, sizeof(*mexican), GFP_KERNEL);
	if (mexican == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, mexican);
	mexican->dev = dev;

	mutex_lock(&mexican_mutex);

	/* get mbox controller handle */
	if (of_parse_phandle_with_args(dev->of_node, "mboxes",
				       "#mbox-cells", 0, &spec)) {
		dev_err(dev, "%s: can't parse \"mboxes\" property\n", __func__);
		mutex_unlock(&mexican_mutex);
		return -ENODEV;
	}

	/* get controller dev */
	cntlr_np = spec.np;
	cntlr_pdev = of_find_device_by_node(cntlr_np);
	if (!cntlr_pdev) {
		dev_err(dev, "%s: can't find controller platform device\n", __func__);
		mutex_unlock(&mexican_mutex);
		return -ENODEV;
	}

	cntlr_dev = &cntlr_pdev->dev;
	if (!cntlr_dev) {
		dev_err(dev, "%s: can't get controller device\n", __func__);
		mutex_unlock(&mexican_mutex);
		return -ENODEV;
	}

	mexican->cntlr_dev = cntlr_dev;
	mexican->to_remote = spec.args[0];
	INIT_LIST_HEAD(&mexican->dealers);

	dev_info(dev, "%s: get remote target(%d)\n", __func__, mexican->to_remote);

	list_add_tail(&mexican->node, &mbox_mexicans);

	mutex_unlock(&mexican_mutex);

	dev_info(dev, "%s done\n", __func__);

	return 0;
}

static int mt5896_mbox_mexico_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mt5896_mbox_mexico_of_match[] = {
	{ .compatible = "mediatek,mt5896-mbox-mexico", },
	{},
};
MODULE_DEVICE_TABLE(of, mt5896_mbox_mexico_of_match);

static struct platform_driver mt5896_mbox_mexico_driver = {
	.driver = {
		.name = "mt5896-mbox-mexico",
		.of_match_table = mt5896_mbox_mexico_of_match,
#ifndef MODULE
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
#endif
	},
	.probe = mt5896_mbox_mexico_probe,
	.remove = mt5896_mbox_mexico_remove,
};
module_platform_driver(mt5896_mbox_mexico_driver);

MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");
MODULE_DESCRIPTION("MediaTek MT5896 mailbox Wrapper Driver");
MODULE_LICENSE("GPL v2");
