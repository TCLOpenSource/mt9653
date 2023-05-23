// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#include <linux/gpio/driver.h>
//#include <linux/irqchip/chained_irq.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#define MASK_REG_BANK		0xFFFF00UL
#define PM_GPIO_BANK		0x001E00UL /* 0x0f << 1 */
#define DDC_BANK		0x000800UL /* 0x04 << 1 */
#define SAR_BANK		0x002800UL /* 0x14 << 1 */
#define PM_MISC_BANK		0x005C00UL /* 0x2e << 1 */
#define TOP_GPIO_BANK		0x645600UL /* 0x322b << 1 */
#define TOP_GPIO_TS_BANK	0x645800UL /* 0x322c << 1 */

struct mtk_dtv_gpio_control {
	u32 oen_offset;
	u8 oen_mask;
	u32 in_offset;
	u8 in_mask;
	u32 out_offset;
	u8 out_mask;
};

struct mtk_dtv_gpio {
	raw_spinlock_t lock;
	void __iomem *reg_base;
	struct gpio_chip gc;
	struct device *dev;
	const struct mtk_dtv_gpio_control *gpio_control;
};

const struct mtk_dtv_gpio_control gpio_control_pm_gpio0[] = {
	{0x28, BIT(0), 0x28, BIT(2), 0x28, BIT(1)},
	{0x2a, BIT(0), 0x2a, BIT(2), 0x2a, BIT(1)},
	{0x00, BIT(0), 0x00, BIT(2), 0x00, BIT(1)},
	{0x02, BIT(0), 0x02, BIT(2), 0x02, BIT(1)},
	{0x04, BIT(0), 0x04, BIT(2), 0x04, BIT(1)},
	{0x06, BIT(0), 0x06, BIT(2), 0x06, BIT(1)},
	{0x0a, BIT(0), 0x0a, BIT(2), 0x0a, BIT(1)},
	{0x0c, BIT(0), 0x0c, BIT(2), 0x0c, BIT(1)},
	{0x0e, BIT(0), 0x0e, BIT(2), 0x0e, BIT(1)},
	{0x10, BIT(0), 0x10, BIT(2), 0x10, BIT(1)},
	{0x12, BIT(0), 0x12, BIT(2), 0x12, BIT(1)},
	{0x14, BIT(0), 0x14, BIT(2), 0x14, BIT(1)},
	{0x16, BIT(0), 0x16, BIT(2), 0x16, BIT(1)},
	{0x18, BIT(0), 0x18, BIT(2), 0x18, BIT(1)},
	{0x1a, BIT(0), 0x1a, BIT(2), 0x1a, BIT(1)},
	{0x1c, BIT(0), 0x1c, BIT(2), 0x1c, BIT(1)},
	{0x1e, BIT(0), 0x1e, BIT(2), 0x1e, BIT(1)},
	{0x20, BIT(0), 0x20, BIT(2), 0x20, BIT(1)},
	{0x48, BIT(0), 0x48, BIT(2), 0x48, BIT(1)},
	{0x4a, BIT(0), 0x4a, BIT(2), 0x4a, BIT(1)},
	{0x4c, BIT(0), 0x4c, BIT(2), 0x4c, BIT(1)},
	{0x4e, BIT(0), 0x4e, BIT(2), 0x4e, BIT(1)},
	{0x50, BIT(0), 0x50, BIT(2), 0x50, BIT(1)},
	{0x52, BIT(0), 0x52, BIT(2), 0x52, BIT(1)},
	{0x54, BIT(0), 0x54, BIT(2), 0x54, BIT(1)},
	{0x56, BIT(0), 0x56, BIT(2), 0x56, BIT(1)},
	{0x58, BIT(0), 0x58, BIT(2), 0x58, BIT(1)},
	{0x5a, BIT(0), 0x5a, BIT(2), 0x5a, BIT(1)}
};

const struct mtk_dtv_gpio_control gpio_control_ddc0[] = {
	{0x96, BIT(1), 0x96, BIT(0), 0x96, BIT(2)},
	{0x96, BIT(5), 0x96, BIT(4), 0x96, BIT(6)},
	{0x97, BIT(1), 0x97, BIT(0), 0x97, BIT(2)},
	{0x97, BIT(5), 0x97, BIT(4), 0x97, BIT(6)},
	{0x98, BIT(1), 0x98, BIT(0), 0x98, BIT(2)},
	{0x98, BIT(5), 0x98, BIT(4), 0x98, BIT(6)},
	{0x99, BIT(1), 0x99, BIT(0), 0x99, BIT(2)},
	{0x99, BIT(5), 0x99, BIT(4), 0x99, BIT(6)}
};

const struct mtk_dtv_gpio_control gpio_control_sar0[] = {
	{0x23, BIT(0), 0x25, BIT(0), 0x24, BIT(0)},
	{0x23, BIT(1), 0x25, BIT(1), 0x24, BIT(1)},
	{0x23, BIT(2), 0x25, BIT(2), 0x24, BIT(2)},
	{0x23, BIT(3), 0x25, BIT(3), 0x24, BIT(3)},
	{0x23, BIT(4), 0x25, BIT(4), 0x24, BIT(4)},
	{0x23, BIT(5), 0x25, BIT(5), 0x24, BIT(5)}
};

const struct mtk_dtv_gpio_control gpio_control_pm_misc0[] = {
	{0x84, BIT(1), 0x84, BIT(2), 0x84, BIT(0)},
	{0x85, BIT(1), 0x85, BIT(2), 0x85, BIT(0)}
};

const struct mtk_dtv_gpio_control gpio_control_top_gpio0[] = {
	{0x80, BIT(1), 0x80, BIT(2), 0x80, BIT(0)},
	{0x82, BIT(1), 0x82, BIT(2), 0x82, BIT(0)},
	{0x84, BIT(1), 0x84, BIT(2), 0x84, BIT(0)},
	{0x86, BIT(1), 0x86, BIT(2), 0x86, BIT(0)}
};

const struct mtk_dtv_gpio_control gpio_control_top_gpio_ts0[] = {
	{0x10, BIT(1), 0x10, BIT(2), 0x10, BIT(0)},
	{0x00, BIT(1), 0x00, BIT(2), 0x00, BIT(0)},
	{0x02, BIT(1), 0x02, BIT(2), 0x02, BIT(0)},
	{0x04, BIT(1), 0x04, BIT(2), 0x04, BIT(0)},
	{0x06, BIT(1), 0x06, BIT(2), 0x06, BIT(0)},
	{0x08, BIT(1), 0x08, BIT(2), 0x08, BIT(0)},
	{0x0a, BIT(1), 0x0a, BIT(2), 0x0a, BIT(0)},
	{0x0c, BIT(1), 0x0c, BIT(2), 0x0c, BIT(0)},
	{0x0e, BIT(1), 0x0e, BIT(2), 0x0e, BIT(0)},
	{0x12, BIT(1), 0x12, BIT(2), 0x12, BIT(0)},
	{0x14, BIT(1), 0x14, BIT(2), 0x14, BIT(0)},
	{0x30, BIT(1), 0x30, BIT(2), 0x30, BIT(0)},
	{0x20, BIT(1), 0x20, BIT(2), 0x20, BIT(0)},
	{0x22, BIT(1), 0x22, BIT(2), 0x22, BIT(0)},
	{0x24, BIT(1), 0x24, BIT(2), 0x24, BIT(0)},
	{0x26, BIT(1), 0x26, BIT(2), 0x26, BIT(0)},
	{0x28, BIT(1), 0x28, BIT(2), 0x28, BIT(0)},
	{0x2a, BIT(1), 0x2a, BIT(2), 0x2a, BIT(0)},
	{0x2c, BIT(1), 0x2c, BIT(2), 0x2c, BIT(0)},
	{0x2e, BIT(1), 0x2e, BIT(2), 0x2e, BIT(0)},
	{0x32, BIT(1), 0x32, BIT(2), 0x32, BIT(0)},
	{0x34, BIT(1), 0x34, BIT(2), 0x34, BIT(0)},
	{0x50, BIT(1), 0x50, BIT(2), 0x50, BIT(0)},
	{0x40, BIT(1), 0x40, BIT(2), 0x40, BIT(0)},
	{0x42, BIT(1), 0x42, BIT(2), 0x42, BIT(0)},
	{0x44, BIT(1), 0x44, BIT(2), 0x44, BIT(0)},
	{0x46, BIT(1), 0x46, BIT(2), 0x46, BIT(0)},
	{0x48, BIT(1), 0x48, BIT(2), 0x48, BIT(0)},
	{0x4a, BIT(1), 0x4a, BIT(2), 0x4a, BIT(0)},
	{0x4c, BIT(1), 0x4c, BIT(2), 0x4c, BIT(0)},
	{0x4e, BIT(1), 0x4e, BIT(2), 0x4e, BIT(0)},
	{0x52, BIT(1), 0x52, BIT(2), 0x52, BIT(0)},
	{0x54, BIT(1), 0x54, BIT(2), 0x54, BIT(0)}
};

static int mtk_dtv_gpio_direction_input(struct gpio_chip *gc,
					    unsigned offset)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	u8 val;
	u32 reg_offset;

	if (offset >= gc->ngpio) {
		dev_err(pdata->dev,
			"set input: offset(%d) is bigger than ngpio(%d)\n",
			offset, gc->ngpio);
		return -EINVAL;
	}

	reg_offset = (pdata->gpio_control[offset].oen_offset & BIT(0)) ?
			((pdata->gpio_control[offset].oen_offset << 1) - 1) :
			(pdata->gpio_control[offset].oen_offset << 1);

	raw_spin_lock_irqsave(&pdata->lock, flags);
	val = readb_relaxed(pdata->reg_base + reg_offset);
	val |= pdata->gpio_control[offset].oen_mask;
	writeb_relaxed(val, (pdata->reg_base + reg_offset));
	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "set input: reg=0x%lx, offset=%d, val=0x%x\n",
			(unsigned long)(pdata->reg_base + reg_offset),
			offset, val);

	return 0;
}

static int mtk_dtv_gpio_direction_output(struct gpio_chip *gc,
					     unsigned offset, int value)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	u8 val;
	u32 reg_oen_offset, reg_out_offset;

	if (offset >= gc->ngpio) {
		dev_err(pdata->dev,
			"set output: offset(%d) is bigger than ngpio(%d)\n",
			offset, gc->ngpio);
		return -EINVAL;
	}

	reg_oen_offset = (pdata->gpio_control[offset].oen_offset & BIT(0)) ?
			((pdata->gpio_control[offset].oen_offset << 1) - 1) :
			(pdata->gpio_control[offset].oen_offset << 1);

	reg_out_offset = (pdata->gpio_control[offset].out_offset & BIT(0)) ?
			((pdata->gpio_control[offset].out_offset << 1) - 1) :
			(pdata->gpio_control[offset].out_offset << 1);

	raw_spin_lock_irqsave(&pdata->lock, flags);
	val = readb_relaxed(pdata->reg_base + reg_oen_offset);
	val &= ~(pdata->gpio_control[offset].oen_mask);
	writeb_relaxed(val, (pdata->reg_base + reg_oen_offset));

	val = readb_relaxed(pdata->reg_base + reg_out_offset);
	if (value)
		val |= pdata->gpio_control[offset].out_mask;
	else
		val &= ~(pdata->gpio_control[offset].out_mask);
	writeb_relaxed(val, (pdata->reg_base + reg_out_offset));

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "set output: reg oen=0x%lx, out=0x%lx\n",
			(unsigned long)(pdata->reg_base + reg_oen_offset),
			(unsigned long)(pdata->reg_base + reg_out_offset));
	dev_dbg(pdata->dev, "set output: offset=%d, val=0x%x\n",
			offset, val);
	return 0;
}

static int mtk_dtv_gpio_get_value(struct gpio_chip *gc, unsigned offset)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	u8 val;
	u32 reg_offset;

	reg_offset = (pdata->gpio_control[offset].in_offset & BIT(0)) ?
			((pdata->gpio_control[offset].in_offset << 1) - 1) :
			(pdata->gpio_control[offset].in_offset << 1);

	raw_spin_lock_irqsave(&pdata->lock, flags);
	val = readb_relaxed(pdata->reg_base + reg_offset);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "get value: reg=0x%lx, offset=%d\n",
			(unsigned long)(pdata->reg_base + reg_offset),
			offset);
	dev_dbg(pdata->dev, "get value: val=0x%x, in_mask=0x%x\n",
			val, (pdata->gpio_control[offset].in_mask));

	return !!(val & (pdata->gpio_control[offset].in_mask));
}

static void mtk_dtv_gpio_set_value(struct gpio_chip *gc, unsigned offset,
				   int value)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	u8 val;
	u32 reg_offset;

	reg_offset = (pdata->gpio_control[offset].out_offset & BIT(0)) ?
			((pdata->gpio_control[offset].out_offset << 1) - 1) :
			(pdata->gpio_control[offset].out_offset << 1);

	raw_spin_lock_irqsave(&pdata->lock, flags);
	val = readb_relaxed(pdata->reg_base + reg_offset);
	if (value)
		val |= pdata->gpio_control[offset].out_mask;
	else
		val &= ~(pdata->gpio_control[offset].out_mask);
	writeb_relaxed(val, (pdata->reg_base + reg_offset));
	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "set value: reg=0x%lx, offset=%d\n",
			(unsigned long)(pdata->reg_base + reg_offset),
			offset);
	dev_dbg(pdata->dev, "set value: val=0x%x, out_mask=0x%x, value=0x%x\n",
			val, (pdata->gpio_control[offset].out_mask), value);
}

static int mtk_dtv_gpio_irq_type(struct irq_data *d, unsigned trigger)
{
#if 0
//TODO
#endif
	return 0;
}

static void mtk_dtv_gpio_irq_handler(struct irq_desc *desc)
{
#if 0
//TODO
#endif
}

static void mtk_dtv_gpio_irq_mask(struct irq_data *d)
{
#if 0
//TODO
#endif
}

static void mtk_dtv_gpio_irq_unmask(struct irq_data *d)
{
#if 0
//TODO
#endif
}

static struct irq_chip mtk_dtv_gpio_irqchip = {
	.name		= "mtk-dtv-gpio",
	.irq_mask	= mtk_dtv_gpio_irq_mask,
	.irq_unmask	= mtk_dtv_gpio_irq_unmask,
	.irq_set_type	= mtk_dtv_gpio_irq_type,
};

static int mtk_dtv_gpio_control_select(struct mtk_dtv_gpio *pdata, struct resource *res, int sub_index)
{
	switch (res->start & MASK_REG_BANK) {
	case PM_GPIO_BANK:
		if (sub_index == 0)
			pdata->gpio_control = gpio_control_pm_gpio0;
		else {
			dev_err(pdata->dev, "wrong sub-index(%d) in dts node\n", sub_index);
			return -EINVAL;
		}
		break;
	case DDC_BANK:
		if (sub_index == 0)
			pdata->gpio_control = gpio_control_ddc0;
		else {
			dev_err(pdata->dev, "wrong sub-index(%d) in dts node\n", sub_index);
			return -EINVAL;
		}
		break;
	case SAR_BANK:
		if (sub_index == 0)
			pdata->gpio_control = gpio_control_sar0;
		else {
			dev_err(pdata->dev, "wrong sub-index(%d) in dts node\n", sub_index);
			return -EINVAL;
		}
		break;
	case PM_MISC_BANK:
		if (sub_index == 0)
			pdata->gpio_control = gpio_control_pm_misc0;
		else {
			dev_err(pdata->dev, "wrong sub-index(%d) in dts node\n", sub_index);
			return -EINVAL;
		}
		break;
	case TOP_GPIO_BANK:
		if (sub_index == 0)
			pdata->gpio_control = gpio_control_top_gpio0;
		else {
			dev_err(pdata->dev, "wrong sub-index(%d) in dts node\n", sub_index);
			return -EINVAL;
		}
		break;
	case TOP_GPIO_TS_BANK:
		if (sub_index == 0)
			pdata->gpio_control = gpio_control_top_gpio_ts0;
		else {
			dev_err(pdata->dev, "wrong sub-index(%d) in dts node\n", sub_index);
			return -EINVAL;
		}
		break;
	default:
		dev_err(pdata->dev, "wrong reg bank(0x%lx) in dts node\n", (unsigned long)res->start);
		return -EINVAL;
	}

	return 0;
}

static int mtk_dtv_gpio_probe(struct platform_device *pdev)
{
	struct mtk_dtv_gpio *pdata;
	struct resource *res;
	struct device_node *dn;
	int ret, of_base, of_ngpio, of_subindex;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	platform_set_drvdata(pdev, pdata);
	pdata->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pdata->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->reg_base)) {
		dev_err(&pdev->dev, "err reg_base=0x%lx\n",
			(unsigned long)pdata->reg_base);
		return PTR_ERR(pdata->reg_base);
	}

	dev_info(&pdev->dev, "reg_base=0x%lx\n",
		(unsigned long)pdata->reg_base);

	dn = pdev->dev.of_node;

	raw_spin_lock_init(&pdata->lock);

	if (of_property_read_bool(dn, "gpio-ranges")) {
		pdata->gc.request = gpiochip_generic_request;
		pdata->gc.free = gpiochip_generic_free;
	}

	pdata->gc.direction_input = mtk_dtv_gpio_direction_input;
	pdata->gc.direction_output = mtk_dtv_gpio_direction_output;
	pdata->gc.get = mtk_dtv_gpio_get_value;
	pdata->gc.set = mtk_dtv_gpio_set_value;

	if (of_property_read_u32(dn, "sub-index", &of_subindex)) {
		dev_err(&pdev->dev, "can't find sub-index in dts node\n");
		return -EINVAL;
	}
	pdata->gc.label = devm_kasprintf(&pdev->dev, GFP_KERNEL, "%s-%d",
						dev_name(&pdev->dev), of_subindex);
	pdata->gc.parent = &pdev->dev;
	pdata->gc.owner = THIS_MODULE;
	if (of_property_read_u32(dn, "base", &of_base)) {
		/* let gpio framework control the base */
		pdata->gc.base = -1;
		dev_info(&pdev->dev, "set base to default(-1)\n");
	} else {
		pdata->gc.base = of_base;
	}

	if (of_property_read_u32(dn, "ngpios", &of_ngpio)) {
		pdata->gc.ngpio = 32;
		dev_info(&pdev->dev, "set ngpios to default(32)\n");
	} else {
		pdata->gc.ngpio = of_ngpio;
	}

	dev_info(&pdev->dev, "base(%d), ngpios(%d), sub-index(%d)\n",
		 pdata->gc.base, pdata->gc.ngpio, of_subindex);

	ret = mtk_dtv_gpio_control_select(pdata, res, of_subindex);
	if (ret) {
		dev_err(&pdev->dev, "gpio control select error: ret=%d\n", ret);
		return ret;
	}

	ret = devm_gpiochip_add_data(&pdev->dev, &pdata->gc, pdata);
	if (ret) {
		dev_err(&pdev->dev, "probe add data error: ret=%d\n", ret);
		return ret;
	}
#if 0
	/*
	 * irq_chip support
	 */
//TODO
#endif

	dev_info(&pdev->dev, "gpio probe done\n");

	return 0;
}

static const struct of_device_id mtk_dtv_gpio_match[] = {
	{
		.compatible = "mediatek,mt5870-gpio",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_dtv_gpio_match);

static struct platform_driver mtk_dtv_gpio_driver = {
	.probe		= mtk_dtv_gpio_probe,
	.driver = {
		.name	= "mt5870-gpio",
		.of_match_table = of_match_ptr(mtk_dtv_gpio_match),
	},
};

module_platform_driver(mtk_dtv_gpio_driver);

MODULE_DESCRIPTION("MediaTek MT5870 SoC based GPIO Driver");
MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");
MODULE_LICENSE("GPL");
