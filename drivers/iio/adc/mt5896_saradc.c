// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Owen.Tseng <Owen.Tseng@mediatek.com>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/regulator/consumer.h>
#include <linux/iio/iio.h>
#include "saradc-mt5896-coda.h"
#include "saradc-mt5896-riu.h"


#define SARADC_TIMEOUT	msecs_to_jiffies(1)

#define SAR_CTRL_SINGLE_MODE		(1 << 4)
#define SAR_CTRL_DIG_FREERUN_MODE	(1 << 5)
#define SAR_CTRL_ATOP_FREERUN_MODE	(1 << 9)
#define SAR_CTRL_ALL_CHANNEL		(1 << 0xb)
#define SAR_CTRL_LOAD_EN			(1 << 0xe)

#define SAR_CTRL_DIG_PD				(1 << 6)
#define SAR_CTRL_ATOP_PD			(1 << 8)
#define SAR_CTRL_MEAN_EN			(1 << 10)

#define SAR_OFFSET		4
#define REF_BIT_MASK	0x3F
#define REG_0020_SAR	0x20


struct mtkdtv_saradc_data {
	int num_bits;
	const struct iio_chan_spec *channels;
	int num_channels;
};

struct mtkdtv_saradc {
	void __iomem  *regs;
	struct completion completion;
	struct reset_control *reset;
	const struct mtkdtv_saradc_data *data;
	u16   last_val;
	u32   ref_bitmap;
};

static void mtk_saradc_start(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct mtkdtv_saradc *info = iio_priv(indio_dev);
	unsigned int val;

	val = mtk_readw_relaxed(info->regs + REG_0000_SAR);
	val |= SAR_CTRL_DIG_FREERUN_MODE;
	val |= SAR_CTRL_ATOP_FREERUN_MODE;
	val &= ~SAR_CTRL_ALL_CHANNEL;
	val &= ~SAR_CTRL_DIG_PD;
	val &= ~SAR_CTRL_ATOP_PD;
	val &= ~SAR_CTRL_MEAN_EN;
	mtk_writew_relaxed(info->regs + REG_0000_SAR, val);
	mtk_writew_relaxed(info->regs + REG_006C_SAR, (info->ref_bitmap));
	mtk_writew_relaxed(info->regs + REG_0070_SAR, 0);
	mtk_writew_relaxed(info->regs + REG_0020_SAR, 1);
}

static int mtkdtv_saradc_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	struct mtkdtv_saradc *info = iio_priv(indio_dev);
	int ctrl, ctrl_l;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		mutex_lock(&indio_dev->mlock);
		ctrl_l = mtk_readw_relaxed(info->regs + REG_0040_SAR);
		ctrl_l |= (2<<(chan->channel));
		mtk_writew_relaxed(info->regs + REG_0040_SAR, ctrl_l);
		/* Select the channel to be used and trigger conversion */
		ctrl = mtk_readw_relaxed(info->regs + REG_0000_SAR);
		ctrl |= SAR_CTRL_LOAD_EN;
		mtk_writew_relaxed(info->regs + REG_0000_SAR, ctrl);
		usleep_range(500, 1000);
		info->last_val = mtk_readw_relaxed(info->regs + REG_0100_SAR +
			((chan->channel)*SAR_OFFSET));
		*val = info->last_val;
		mutex_unlock(&indio_dev->mlock);
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}


static const struct iio_info mtkdtv_saradc_iio_info = {
	.read_raw = mtkdtv_saradc_read_raw,
};

#define ADC_CHANNEL(_index, _id) {              \
	.type = IIO_VOLTAGE,                    \
	.indexed = 1,                       \
	.channel = _index,                  \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),       \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),   \
	.datasheet_name = _id,                  \
}

static const struct iio_chan_spec mtkdtv_saradc_iio_channels[] = {
	ADC_CHANNEL(0, "adc0"),
	ADC_CHANNEL(1, "adc1"),
	ADC_CHANNEL(2, "adc2"),
	ADC_CHANNEL(3, "adc3"),
	ADC_CHANNEL(4, "adc4"),
	ADC_CHANNEL(5, "adc5"),
	ADC_CHANNEL(6, "adc6"),
	ADC_CHANNEL(7, "adc7"),
	ADC_CHANNEL(8, "adc8"),
	ADC_CHANNEL(9, "adc9"),
	ADC_CHANNEL(10, "adc10"),
	ADC_CHANNEL(11, "adc11"),
	ADC_CHANNEL(12, "adc12"),
	ADC_CHANNEL(13, "adc13"),
	ADC_CHANNEL(14, "adc14"),
	ADC_CHANNEL(15, "adc15"),
	ADC_CHANNEL(16, "adc16"),
	ADC_CHANNEL(17, "adc17"),
	ADC_CHANNEL(18, "adc18"),
	ADC_CHANNEL(19, "adc19"),
	ADC_CHANNEL(20, "adc20"),
	ADC_CHANNEL(21, "adc21"),
	ADC_CHANNEL(22, "adc22"),
	ADC_CHANNEL(23, "adc23"),
	ADC_CHANNEL(24, "adc24"),
	ADC_CHANNEL(25, "adc25"),
	ADC_CHANNEL(26, "adc26"),
	ADC_CHANNEL(27, "adc27"),
	ADC_CHANNEL(28, "adc28"),
	ADC_CHANNEL(29, "adc29"),
	ADC_CHANNEL(30, "adc30"),
	ADC_CHANNEL(31, "adc31"),
};

static const struct mtkdtv_saradc_data saradc_data = {
	.num_bits = 10,
	.channels = mtkdtv_saradc_iio_channels,
	.num_channels = ARRAY_SIZE(mtkdtv_saradc_iio_channels),
};


static const struct of_device_id mtkdtv_saradc_match[] = {
	{
		.compatible = "mt5896,saradc",
		.data = &saradc_data,
	},
	{},
};
MODULE_DEVICE_TABLE(of, mtkdtv_saradc_match);

static int mtkdtv_saradc_probe(struct platform_device *pdev)
{
	struct mtkdtv_saradc *info = NULL;
	struct device_node *np = pdev->dev.of_node;
	struct iio_dev *indio_dev = NULL;
	struct resource *mem;
	const struct of_device_id *match;
	int ret;
	uint32_t u32sar2gpios = 0x0;
	uint32_t ctrl_l = 0x0;

	if (!np)
		return -ENODEV;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*info));
	if (!indio_dev)
		return -ENOMEM;
	info = iio_priv(indio_dev);
	match = of_match_device(mtkdtv_saradc_match, &pdev->dev);
	if (!match)
		return -ENOMEM;
	info->data = match->data;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	//info->regs = devm_ioremap_resource(&pdev->dev, mem);
	info->regs = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (IS_ERR(info->regs))
		return PTR_ERR(info->regs);

	ret = of_property_read_u32(pdev->dev.of_node,
				"ref-bitmap", &info->ref_bitmap);
	if (ret) {
		dev_info(&pdev->dev, "could not find bitmap\n");
		info->ref_bitmap = REF_BIT_MASK;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "sar2gpios", &u32sar2gpios);
	if (ret) {
		dev_info(&pdev->dev, "could not find sar2gpios\n");
	} else {
		ctrl_l = mtk_readw_relaxed(info->regs + REG_0040_SAR);
		ctrl_l = ((ctrl_l & u32sar2gpios) & 0xFFFF);
		mtk_writew_relaxed(info->regs + REG_0040_SAR, ctrl_l);
		dev_info(&pdev->dev, "find (sar2gpios,ctrl_l)=(0x%x,0x%x)\n",u32sar2gpios, ctrl_l);
	}


	platform_set_drvdata(pdev, indio_dev);
	indio_dev->name = dev_name(&pdev->dev);
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->info = &mtkdtv_saradc_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = info->data->channels;
	indio_dev->num_channels = info->data->num_channels;

	mtk_saradc_start(&pdev->dev);
	ret = iio_device_register(indio_dev);

	if (ret)
		goto err_clk;

	return 0;

err_clk:
	return ret;
}

static int mtkdtv_saradc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);

	iio_device_unregister(indio_dev);
	return 0;
}

static void mtktv_saradc_shutdown(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct mtkdtv_saradc *info = iio_priv(indio_dev);

	mtk_writew_relaxed(info->regs + REG_0020_SAR, 0);
}

#ifdef CONFIG_PM_SLEEP
static int mtkdtv_saradc_suspend(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct mtkdtv_saradc *info = iio_priv(indio_dev);

	mtk_writew_relaxed(info->regs + REG_0020_SAR, 0);
	return 0;
}

static int mtkdtv_saradc_resume(struct device *dev)
{
	mtk_saradc_start(dev);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mtkdtv_saradc_pm_ops,
		mtkdtv_saradc_suspend, mtkdtv_saradc_resume);

static struct platform_driver mtkdtv_saradc_driver = {
	.probe      = mtkdtv_saradc_probe,
	.remove     = mtkdtv_saradc_remove,
	.shutdown	= mtktv_saradc_shutdown,
	.driver     = {
		.name   = "mt5896-saradc",
		.of_match_table = mtkdtv_saradc_match,
		.pm = &mtkdtv_saradc_pm_ops,
	},
};

module_platform_driver(mtkdtv_saradc_driver);

MODULE_AUTHOR("Owen.Tseng@mediatek.com");
MODULE_DESCRIPTION("mtk SARADC driver");
MODULE_LICENSE("GPL v2");
