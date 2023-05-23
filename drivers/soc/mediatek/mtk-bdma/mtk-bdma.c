// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author Owen.Tseng <Owen.Tseng@mediatek.com>
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/of_reserved_mem.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/refcount.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/types.h>
#include <linux/of_address.h>
#include "bdma-mt5896-coda.h"
#include "bdma-mt5896-riu.h"
#include <soc/mediatek/mtk-bdma.h>

#define BDMA_INT_EN					BIT(1)
#define BDMA_INT_STS				BIT(2)
#define BDMA_DONE_STS				BIT(3)
#define BDMA_RESULT_STS				BIT(4)
#define DOUBLE_WORD					32
#define HALF_DWORD					16
#define HALF_WORD					8
#define BDMA_TIMEOUT_MS				30000
#define BYTE_MSK					0xFF
#define WORD_MSK					0xFFFF
#define HALF_BYTE					4
#define TRIPLE_BYTE					12
#define SRC_EXT_BIT					10
#define DST_EXT_BIT					12
#define LEN_EXT_WIDTH				14
#define BDMA_CKGEN_SW_EN			BIT(1)
#define NOT_FOUND_ADDR				0xBABE
#define BDMA_VDEC_CKGEN_SW_EN		BIT(0)


enum _bdma_dw {
	E_BDMA_DW_1BYTE = 0x00,
	E_BDMA_DW_2BYTE = 0x10,
	E_BDMA_DW_4BYTE = 0x20,
	E_BDMA_DW_8BYTE = 0x30,
	E_BDMA_DW_16BYTE = 0x40,
	E_BDMA_DW_MAX = E_BDMA_DW_16BYTE
};



static LIST_HEAD(mtk_bdma_channel_list);
static LIST_HEAD(mtk_bdma_lookup_list);
static DEFINE_MUTEX(mtk_bdma_lookup_lock);

static struct mtk_bdma *mtk_bdma_find(const char *channel_name)
{
	struct mtk_bdma *bdma;

	list_for_each_entry(bdma, &mtk_bdma_lookup_list, list) {
		if (!strncmp(bdma->channel, channel_name, sizeof(bdma->channel)))
			return bdma;
	}
	return NULL;
}

static inline u16 dtvbdma_rd(struct mtk_bdma *bdma, u32 reg)
{
	return mtk_readw_relaxed((bdma->base + reg));
}

static inline u8 dtvbdma_rdl(struct mtk_bdma *bdma, u16 reg)
{
	return dtvbdma_rd(bdma, reg)&BYTE_MSK;
}

static inline void dtvbdma_wr(struct mtk_bdma *bdma, u16 reg, u32 val)
{
	mtk_writew_relaxed((bdma->base + reg), val);
}

static inline void dtvbdma_wrl(struct mtk_bdma *bdma, u16 reg, u8 val)
{
	u16 val16 = dtvbdma_rd(bdma, reg)&0xff00;

	val16 |= val;
	dtvbdma_wr(bdma, reg, val16);
}

static inline void mtk_hw_clear_done(struct mtk_bdma *bdma)
{
	u16 val16 = dtvbdma_rdl(bdma, REG_0004_BDMA_CH0);

	val16 |= BDMA_INT_STS;
	dtvbdma_wr(bdma, REG_0004_BDMA_CH0, val16);
}

static inline void mtk_hw_clear_result(struct mtk_bdma *bdma)
{
	u16 val16 = dtvbdma_rdl(bdma, REG_0004_BDMA_CH0);

	val16 |= BDMA_RESULT_STS;
	dtvbdma_wr(bdma, REG_0004_BDMA_CH0, val16);
}

static inline u8 bdma_dev_cfg(u16 devcfg)
{
	switch (devcfg&BYTE_MSK) {
	case E_BDMA_DEV_MIU0:
	case E_BDMA_DEV_MIU1:
	case E_BDMA_DEV_FLASH:
		return E_BDMA_DW_16BYTE;
	case E_BDMA_DEV_MEM_FILL:
	case E_BDMA_DEV_FCIC:
		return E_BDMA_DW_4BYTE;
	default:
		return E_BDMA_DW_1BYTE;
	}
}

static inline void mtk_bdma_path_config(struct mtk_bdma *bdma)
{
	u16 src_path;
	u16 dst_path;
	u16 dw_cfg;
	unsigned long val;
	u16 ctl_val;

	val = mtk_readw_relaxed(bdma->ckgen);
	if (!strncmp(bdma->channel, "bdma_ch3", sizeof(bdma->channel)))
		val |= BDMA_VDEC_CKGEN_SW_EN;
	else
		val |= BDMA_CKGEN_SW_EN;
	mtk_writew_relaxed(bdma->ckgen, val);

	dw_cfg = bdma_dev_cfg((bdma->data.bdmapath)&BYTE_MSK);
	src_path = ((bdma->data.bdmapath)&BYTE_MSK) | dw_cfg;
	bdma->data.bdmapath |= src_path;
	dw_cfg = bdma_dev_cfg(_RSHIFT((bdma->data.bdmapath), HALF_WORD));
	bdma->data.bdmapath |= _LSHIFT(dw_cfg, HALF_WORD);
	dtvbdma_wr(bdma, REG_0008_BDMA_CH0, (bdma->data.bdmapath));
	dst_path = _RSHIFT((bdma->data.bdmapath), HALF_WORD);
	if (dst_path&E_BDMA_DEV_SEARCH) {
		val = _RSHIFT((bdma->data.pattern), HALF_DWORD);
		dtvbdma_wr(bdma, REG_002C_BDMA_CH0, val);
		dtvbdma_wr(bdma, REG_0028_BDMA_CH0, ((bdma->data.pattern)));
	}
	if (bdma->data.exclubit) {
		val = _RSHIFT((bdma->data.exclubit), HALF_DWORD);
		dtvbdma_wr(bdma, REG_0034_BDMA_CH0, val);
		dtvbdma_wr(bdma, REG_0030_BDMA_CH0, ((bdma->data.exclubit)));
	}
	/*if DRAM Address is over 4G rang ,another HW setting should be set*/
	val = _RSHIFT((bdma->data.srcaddr), DOUBLE_WORD);
	if (val) {
		val = _LSHIFT(val, SRC_EXT_BIT);
		ctl_val = dtvbdma_rd(bdma, REG_0000_BDMA_CH0);
		ctl_val |= val;
		dtvbdma_wr(bdma, REG_0000_BDMA_CH0, ctl_val);
	}
	val = _RSHIFT((bdma->data.dstaddr), DOUBLE_WORD);
	if (val) {
		val = _LSHIFT(val, DST_EXT_BIT);
		ctl_val = dtvbdma_rd(bdma, REG_0000_BDMA_CH0);
		ctl_val |= val;
		dtvbdma_wr(bdma, REG_0000_BDMA_CH0, ctl_val);
	}
	/*if DRAM Address is over 4G rang ,another HW setting should be set*/
	dtvbdma_wr(bdma, REG_0010_BDMA_CH0, ((bdma->data.srcaddr)&WORD_MSK));
	dtvbdma_wr(bdma, REG_0014_BDMA_CH0, _RSHIFT((bdma->data.srcaddr), HALF_DWORD));
	dtvbdma_wr(bdma, REG_0018_BDMA_CH0, ((bdma->data.dstaddr)&WORD_MSK));
	dtvbdma_wr(bdma, REG_001C_BDMA_CH0, _RSHIFT((bdma->data.dstaddr), HALF_DWORD));
	dtvbdma_wr(bdma, REG_0020_BDMA_CH0, ((bdma->data.len)&WORD_MSK));
	dtvbdma_wr(bdma, REG_0024_BDMA_CH0, _RSHIFT((bdma->data.len), HALF_DWORD));
	ctl_val = dtvbdma_rd(bdma, REG_000C_BDMA_CH0);
	ctl_val |= BDMA_INT_EN;
	dtvbdma_wr(bdma, REG_000C_BDMA_CH0, ctl_val);
	/*BDMA Trigger*/
	dtvbdma_wrl(bdma, REG_0000_BDMA_CH0, 1);
}

static inline int mtk_bdma_handle_event(struct mtk_bdma *bdma)
{
	u32 search_offset;
	int ret = 0;
	u16 path = 0;

	path = _RSHIFT((bdma->data.bdmapath), HALF_WORD);
	switch (path&BYTE_MSK) {
	case E_BDMA_DEV_SEARCH:
		if (bdma->addr_match == 1) {
			search_offset = dtvbdma_rd(bdma, REG_0010_BDMA_CH0);
			search_offset |=
				(dtvbdma_rd(bdma, REG_0014_BDMA_CH0) << HALF_DWORD);
			mtk_hw_clear_result(bdma);
			if (search_offset > bdma->data.len) {
				ret = -ETIMEDOUT;
				bdma->data.searchoffset = NOT_FOUND_ADDR;
			} else
				bdma->data.searchoffset = search_offset - 1;
		} else {
			dev_err(bdma->dev, "dma search mimatch\n");
			ret = -ETIMEDOUT;
		}
	break;
	default:
		dev_err(bdma->dev, "@@@bdma path no need to handle event\n");
	break;
	}
	return ret;
}

struct mtk_bdma_channel *mtk_bdma_request_channel
		(const char *channel_name, const char *label)
{
	struct mtk_bdma *bdma = mtk_bdma_find(channel_name);
	struct mtk_bdma_channel *ch;

	if ((!label) || (!bdma))
		return NULL;

	if (bdma) {
		ch = kmalloc(sizeof(*ch), GFP_KERNEL);
		if (ch) {
			ch->bdma = bdma;
			strlcpy(ch->label, label, MAX_CHANNEL_LABLE);
			list_add_tail(&ch->link, &mtk_bdma_channel_list);
			return ch;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(mtk_bdma_request_channel);

void mtk_bdma_release_channel(struct mtk_bdma_channel *ch)
{
	if (ch) {
		list_del(&ch->link);
		kfree(ch);
	}
}
EXPORT_SYMBOL(mtk_bdma_release_channel);


int mtk_bdma_transact(struct mtk_bdma_channel *ch, struct mtk_bdma_dat *data)
{
	struct mtk_bdma *bdma;
	unsigned int timeout;
	int ret = 0;

	if (!ch || !data)
		return -EINVAL;


	list_for_each_entry(bdma, &mtk_bdma_lookup_list, list) {
		if (!strncmp(bdma->channel, ch->bdma->channel, sizeof(bdma->channel))) {
			mutex_lock(&bdma->bus_mutex);
			bdma->data.bdmapath = data->bdmapath;
			bdma->data.srcaddr = data->srcaddr;
			bdma->data.dstaddr = data->dstaddr;
			bdma->data.len = data->len;
			bdma->data.pattern = data->pattern;
			bdma->data.exclubit = data->exclubit;
			mtk_bdma_path_config(bdma);
			timeout = wait_for_completion_timeout(&bdma->done,
					msecs_to_jiffies(BDMA_TIMEOUT_MS));

			if (!timeout) {
				dev_err(bdma->dev, "@@@bdma timeout\n");
				mutex_unlock(&bdma->bus_mutex);
				return -ETIMEDOUT;
			}
			ret = mtk_bdma_handle_event(bdma);
			data->searchoffset = bdma->data.searchoffset;
			mutex_unlock(&bdma->bus_mutex);
			return ret;
		}
	}
	return -ENODEV;
}
EXPORT_SYMBOL(mtk_bdma_transact);

static irqreturn_t dtv_bdma_interrupt(int irq, void *dev_id)
{
	struct mtk_bdma *bdma = dev_id;
	u16 sts = 0;

	sts = dtvbdma_rd(bdma, REG_0004_BDMA_CH0);
	if (sts&BDMA_INT_STS) {
		if ((sts&BDMA_RESULT_STS) &&
		(_RSHIFT((bdma->data.bdmapath), HALF_WORD)&E_BDMA_DEV_SEARCH))
			bdma->addr_match = 1;
		mtk_hw_clear_done(bdma);
		complete(&bdma->done);
		return IRQ_HANDLED;
	}
	dev_err(bdma->dev, "Error:bdma irq num!\n");
	mtk_hw_clear_done(bdma);
	return IRQ_NONE;
}

static int mtk_bdma_probe(struct platform_device *pdev)
{
	struct mtk_bdma *bdma;
	int err = -ENODEV;
	const char *channel;
	struct device_node *np = NULL;

	np = pdev->dev.of_node;
	if (!np) {
		dev_err(&pdev->dev, "fail to find node\n");
		return -EINVAL;
	}

	bdma = devm_kzalloc(&pdev->dev, sizeof(*bdma), GFP_KERNEL);
	if (!bdma)
		return -ENOMEM;

	mutex_init(&bdma->bus_mutex);
	init_completion(&bdma->done);
	bdma->base = of_iomap(np, 0);
	bdma->ckgen = of_iomap(np, 1);
	bdma->dev = &pdev->dev;

	if (!bdma->base) {
		kfree(bdma);
		return -EINVAL;
	}
	bdma->irq = platform_get_irq(pdev, 0);
	if (bdma->irq < 0) {
		dev_err(&pdev->dev, "could not get irq resource\n");
		return -EINVAL;
	}

	/* Request a thread to handle bdma events */
	err = devm_request_irq(&pdev->dev, bdma->irq, dtv_bdma_interrupt,
					IRQF_ONESHOT, dev_name(&pdev->dev), bdma);
	if (err) {
		dev_err(&pdev->dev,
				"could not request IRQ: %d:%d\n", bdma->irq, err);
		return err;
	}
	err = of_property_read_string(pdev->dev.of_node, "mediatek,channel", &channel);
	if (!err)
		strlcpy(bdma->channel, channel, CONID_STRING_SIZE);

	mutex_lock(&mtk_bdma_lookup_lock);
	list_add_tail(&bdma->list, &mtk_bdma_lookup_list);
	mutex_unlock(&mtk_bdma_lookup_lock);
	platform_set_drvdata(pdev, bdma);
	return 0;
}


#if defined(CONFIG_OF)
static const struct of_device_id mtkbdma_of_device_ids[] = {
		{.compatible = "mediatek,mt5896-bdma" },
		{},
};
#endif

static struct platform_driver mtk_bdma_driver = {
	.probe		= mtk_bdma_probe,
	.driver		= {
#if defined(CONFIG_OF)
	.of_match_table = mtkbdma_of_device_ids,
#endif
	.name   = "mediatek,mt5896-bdma",
	.owner  = THIS_MODULE,
}
};

module_platform_driver(mtk_bdma_driver);

MODULE_AUTHOR("Owen.Tseng@mediatek.com");
MODULE_DESCRIPTION("bdma driver");
MODULE_LICENSE("GPL");

