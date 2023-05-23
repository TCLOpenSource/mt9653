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
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/hwspinlock.h>

#define MTK_GPIO_OEN_OFFSET		(0x40)
#define MTK_GPIO_OEN_IN			0
#define MTK_GPIO_OEN_OUT		1
#define MTK_GPIO_OUT_OFFSET		(0x00)
#define MTK_GPIO_IN_OFFSET		(0x80)
#define MTK_GPIO_IRQC_FORCE_OFFSET	(0x00)
#define MTK_GPIO_IRQC_MASK_OFFSET	(0x10)
#define MTK_GPIO_IRQC_POLAR_OFFSET	(0x20)
#define MTK_GPIO_IRQC_STATUS_OFFSET	(0x30)
#define OFFSET_TO_BIT(bit)		(1UL << (bit))
#define MTK_GPIO_MAX_NUMBER		256
#define MTK_GPIO_REG_SIZE		0x200
#define MTK_GPIO_IRQ_REG_NO_STAT_SIZE	0x30
#define MTK_GPIO_IRQ_REG_PER_SUB_SIZE	0x40
/* hwspinlock timeout ms */
#define MTK_GPIO_HWSPINLOCK_TIMEOUT	10

struct mtk_dtv_gpio {
	void __iomem *reg_base;
	void __iomem *reg_base_irq;
	void __iomem *reg_base_irq_both_edge;
	struct device *dev;
	struct gpio_chip gc;
	struct irq_chip ic;
	raw_spinlock_t lock;
	int gpio_irq;
	int gpio_fiq;
	u8 gpio_pm_reg_save[MTK_GPIO_REG_SIZE];
	u8 gpio_irq_pm_reg_save[MTK_GPIO_REG_SIZE];
	u8 gpio_irq_both_edge_pm_reg_save[MTK_GPIO_REG_SIZE];
	bool is_gpio_as_fiq[MTK_GPIO_MAX_NUMBER];
	bool is_irq_both_edge;
	bool is_skip_str_restore;
	struct hwspinlock *hwlock;
};

static int mtk_dtv_gpio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	int ret;
	ulong val;
	u32 group, bit, reg_offset;

	if (offset >= gc->ngpio) {
		dev_err(pdata->dev,
			"set input: offset(%d) is bigger than ngpio(%d)\n",
			offset, gc->ngpio);
		return -EINVAL;
	}

	group = offset / 16;
	bit = offset % 16;

	reg_offset = MTK_GPIO_OEN_OFFSET + (group * 4);

	if (bit > 7)
		reg_offset += 1;

	raw_spin_lock_irqsave(&pdata->lock, flags);

	if (pdata->hwlock) {
		ret = hwspin_lock_timeout_in_atomic(pdata->hwlock, MTK_GPIO_HWSPINLOCK_TIMEOUT);

		if (ret) {
			dev_err(pdata->dev, "%s: cannot get hwspinlock: offset=%d\n",
				__func__, offset);
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return ret;
		}
	}

	val = readb_relaxed(pdata->reg_base + reg_offset);

	if (pdata->hwlock)
		hwspin_unlock_in_atomic(pdata->hwlock);

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	bit %= 8;

	dev_dbg(pdata->dev, "%s: reg=0x%lx\n",
		__func__, (unsigned long)(pdata->reg_base + reg_offset));

	if (test_bit(bit, &val)) {
		dev_dbg(pdata->dev, "%s: offset %d, bit=0x%x, val=0x%lx, out\n",
			__func__, offset, bit, val);
		ret = 0;
	} else {
		dev_dbg(pdata->dev, "%s: offset %d, bit=0x%x, val=0x%lx, in\n",
			__func__, offset, bit, val);
		ret = 1;
	}

	return ret;
}

static int mtk_dtv_gpio_get_value(struct gpio_chip *gc, unsigned int offset)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	int func, ret;
	ulong val;
	u32 group, bit, reg_offset;

	if (offset >= gc->ngpio) {
		dev_err(pdata->dev,
			"set input: offset(%d) is bigger than ngpio(%d)\n",
			offset, gc->ngpio);
		return -EINVAL;
	}

	func = mtk_dtv_gpio_get_direction(gc, offset);

	group = offset / 16;
	bit = offset % 16;

	if (func == 0)
		reg_offset = MTK_GPIO_OUT_OFFSET + (group * 4);
	else
		reg_offset = MTK_GPIO_IN_OFFSET + (group * 4);

	if (bit > 7)
		reg_offset += 1;

	raw_spin_lock_irqsave(&pdata->lock, flags);

	if (pdata->hwlock) {
		ret = hwspin_lock_timeout_in_atomic(pdata->hwlock, MTK_GPIO_HWSPINLOCK_TIMEOUT);

		if (ret) {
			dev_err(pdata->dev, "%s: cannot get hwspinlock: offset=%d\n",
				__func__, offset);
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return ret;
		}
	}

	val = readb_relaxed(pdata->reg_base + reg_offset);

	if (pdata->hwlock)
		hwspin_unlock_in_atomic(pdata->hwlock);

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	bit %= 8;

	if (test_bit(bit, &val))
		ret = 1;
	else
		ret = 0;

	dev_dbg(pdata->dev, "%s: reg=0x%lx\n",
		__func__, (unsigned long)(pdata->reg_base + reg_offset));
	dev_dbg(pdata->dev, "%s: offset %d, bit=0x%x, val=0x%lx, %d\n",
		__func__, offset, bit, val, ret);

	return ret;
}

static void mtk_dtv_gpio_set_value(struct gpio_chip *gc, unsigned int offset,
				   int value)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	ulong val, mask;
	u32 group, bit, reg_offset;
	int ret;

	if (offset >= gc->ngpio) {
		dev_err(pdata->dev,
			"set input: offset(%d) is bigger than ngpio(%d)\n",
			offset, gc->ngpio);
		return;
	}

	group = offset / 16;
	bit = offset % 16;

	reg_offset = MTK_GPIO_OUT_OFFSET + (group * 4);

	if (bit > 7)
		reg_offset += 1;

	bit %= 8;

	mask = OFFSET_TO_BIT(bit);

	raw_spin_lock_irqsave(&pdata->lock, flags);

	if (pdata->hwlock) {
		ret = hwspin_lock_timeout_in_atomic(pdata->hwlock, MTK_GPIO_HWSPINLOCK_TIMEOUT);

		if (ret) {
			dev_err(pdata->dev, "%s: cannot get hwspinlock: offset=%d\n",
				__func__, offset);
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return;
		}
	}

	val = readb_relaxed(pdata->reg_base + reg_offset);

	if (value)
		val |= mask;
	else
		val &= ~mask;

	writeb_relaxed((u8)val, (pdata->reg_base + reg_offset));

	if (pdata->hwlock)
		hwspin_unlock_in_atomic(pdata->hwlock);

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "%s: reg=0x%lx\n",
		__func__, (unsigned long)(pdata->reg_base + reg_offset));
	dev_dbg(pdata->dev, "%s: offset %d, bit=0x%x, val=0x%lx, value=0x%x\n",
		__func__, offset, bit, val, value);
}

static int mtk_dtv_gpio_direction_input(struct gpio_chip *gc,
					unsigned int offset)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	ulong val, mask;
	u32 group, bit, reg_offset;
	int ret;

	if (offset >= gc->ngpio) {
		dev_err(pdata->dev,
			"set input: offset(%d) is bigger than ngpio(%d)\n",
			offset, gc->ngpio);
		return -EINVAL;
	}

	group = offset / 16;
	bit = offset % 16;

	reg_offset = MTK_GPIO_OEN_OFFSET + (group * 4);

	if (bit > 7)
		reg_offset += 1;

	bit %= 8;

	mask = OFFSET_TO_BIT(bit);

	raw_spin_lock_irqsave(&pdata->lock, flags);

	if (pdata->hwlock) {
		ret = hwspin_lock_timeout_in_atomic(pdata->hwlock, MTK_GPIO_HWSPINLOCK_TIMEOUT);

		if (ret) {
			dev_err(pdata->dev, "%s: cannot get hwspinlock: offset=%d\n",
				__func__, offset);
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return ret;
		}
	}

	val = readb_relaxed(pdata->reg_base + reg_offset);

	if (MTK_GPIO_OEN_IN != 0)
		val |= mask;
	else
		val &= ~mask;

	writeb_relaxed((u8)val, (pdata->reg_base + reg_offset));

	if (pdata->hwlock)
		hwspin_unlock_in_atomic(pdata->hwlock);

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "%s: reg=0x%lx\n",
		__func__, (unsigned long)(pdata->reg_base + reg_offset));
	dev_dbg(pdata->dev, "%s: offset %d, bit=0x%x, val=0x%lx\n",
		__func__, offset, bit, val);

	return 0;
}

static int mtk_dtv_gpio_direction_output(struct gpio_chip *gc,
					 unsigned int offset, int value)
{
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long flags;
	ulong val, mask;
	u32 group, bit, reg_offset;
	int ret;

	if (offset >= gc->ngpio) {
		dev_err(pdata->dev,
			"set output: offset(%d) is bigger than ngpio(%d)\n",
			offset, gc->ngpio);
		return -EINVAL;
	}

	/* set value, then direction */
	mtk_dtv_gpio_set_value(gc, offset, value);

	group = offset / 16;
	bit = offset % 16;

	reg_offset = MTK_GPIO_OEN_OFFSET + (group * 4);

	if (bit > 7)
		reg_offset += 1;

	bit %= 8;

	mask = OFFSET_TO_BIT(bit);

	raw_spin_lock_irqsave(&pdata->lock, flags);

	if (pdata->hwlock) {
		ret = hwspin_lock_timeout_in_atomic(pdata->hwlock, MTK_GPIO_HWSPINLOCK_TIMEOUT);

		if (ret) {
			dev_err(pdata->dev, "%s: cannot get hwspinlock: offset=%d\n",
				__func__, offset);
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return ret;
		}
	}

	val = readb_relaxed(pdata->reg_base + reg_offset);

	if (MTK_GPIO_OEN_OUT != 0)
		val |= mask;
	else
		val &= ~mask;

	writeb_relaxed((u8)val, (pdata->reg_base + reg_offset));

	if (pdata->hwlock)
		hwspin_unlock_in_atomic(pdata->hwlock);

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "%s: reg=0x%lx\n",
		__func__, (unsigned long)(pdata->reg_base + reg_offset));
	dev_dbg(pdata->dev, "%s: offset %d, bit=0x%x, val=0x%lx\n",
		__func__, offset, bit, val);

	return 0;
}

static irqreturn_t mtk_dtv_gpio_irq_handler(int irq, void *data)
{
	struct gpio_chip *gc = data;
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	irqreturn_t ret = IRQ_NONE;
	int bit, cnt, i, pin;
	u32 map, reg_offset;
	ulong val_status;
	u16 mask;

	cnt = (MTK_GPIO_MAX_NUMBER / 16);
	for (i = 0; i < cnt; i++) {
		reg_offset = MTK_GPIO_IRQC_STATUS_OFFSET;
		reg_offset += ((i % 4) * 4) + ((i / 4) * 0x80) + 0x40;
		val_status = readw_relaxed(pdata->reg_base_irq + reg_offset);

		if (val_status) {
			for_each_set_bit(bit, &val_status, 16) {
				mask = OFFSET_TO_BIT(bit);
				pin = bit + (i * 16);
				map = irq_find_mapping(gc->irq.domain, pin);
				generic_handle_irq(map);

				/* clear status */
				writew_relaxed(mask,
					pdata->reg_base_irq + reg_offset);

				ret |= IRQ_HANDLED;
			}
		}
	}

	return ret;
}

static irqreturn_t mtk_dtv_gpio_fiq_handler(int irq, void *data)
{
	struct gpio_chip *gc = data;
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	irqreturn_t ret = IRQ_NONE;
	int bit, cnt, i, pin;
	u32 map, reg_offset;
	ulong val_status;
	u16 mask;

	cnt = (MTK_GPIO_MAX_NUMBER / 16);
	for (i = 0; i < cnt; i++) {
		reg_offset = MTK_GPIO_IRQC_STATUS_OFFSET;
		reg_offset += ((i % 4) * 4) + ((i / 4) * 0x80);
		val_status = readw_relaxed(pdata->reg_base_irq + reg_offset);

		if (val_status) {
			for_each_set_bit(bit, &val_status, 16) {
				mask = OFFSET_TO_BIT(bit);
				pin = bit + (i * 16);
				map = irq_find_mapping(gc->irq.domain, pin);

				generic_handle_irq(map);

				/* clear status */
				writew_relaxed(mask,
					pdata->reg_base_irq + reg_offset);

				ret |= IRQ_HANDLED;
			}
		}
	}

	return ret;

}

static void _mtk_dtv_gpio_irq_set_polar(struct mtk_dtv_gpio *pdata,
					unsigned long pin,
					bool is_fiq, bool is_polar)
{
	unsigned long flags;
	u8 val, mask;
	u32 group, bit, reg_offset;
	int ret;

	if (is_fiq == true)
		reg_offset = (pin / 64) * 0x80;
	else
		reg_offset = ((pin / 64) * 0x80) + 0x40;

	reg_offset += MTK_GPIO_IRQC_POLAR_OFFSET;

	group = (pin % 64) / 16;
	bit = (pin % 64) % 16;

	reg_offset += (group * 4);

	if (bit > 7)
		reg_offset += 1;

	bit %= 8;

	mask = OFFSET_TO_BIT(bit);

	raw_spin_lock_irqsave(&pdata->lock, flags);

	if (pdata->hwlock) {
		ret = hwspin_lock_timeout_in_atomic(pdata->hwlock, MTK_GPIO_HWSPINLOCK_TIMEOUT);

		if (ret) {
			dev_err(pdata->dev, "%s: cannot get hwspinlock: pin=%ld\n",
				__func__, pin);
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return;
		}
	}

	val = readb_relaxed(pdata->reg_base_irq + reg_offset);

	if (is_polar == true)
		val |= mask;
	else
		val &= ~mask;

	writeb_relaxed(val, (pdata->reg_base_irq + reg_offset));

	if (pdata->hwlock)
		hwspin_unlock_in_atomic(pdata->hwlock);

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "%s: reg=0x%lx\n",
		__func__, (unsigned long)(pdata->reg_base_irq + reg_offset));
	dev_dbg(pdata->dev, "%s: pin %ld, bit=0x%x, val=0x%x, mask=x0%x\n",
		__func__, pin, bit, val, mask);
}

static void _mtk_dtv_gpio_irq_set_both_edge(struct mtk_dtv_gpio *pdata,
					    unsigned long pin,
					    bool is_both)
{
	unsigned long flags;
	u8 val, mask;
	u32 group, bit, reg_offset;
	int ret;

	reg_offset = 0;

	group = pin / 16;
	bit = pin % 16;

	reg_offset += (group * 4);

	if (bit > 7)
		reg_offset += 1;

	bit %= 8;

	mask = OFFSET_TO_BIT(bit);

	raw_spin_lock_irqsave(&pdata->lock, flags);

	if (pdata->hwlock) {
		ret = hwspin_lock_timeout_in_atomic(pdata->hwlock, MTK_GPIO_HWSPINLOCK_TIMEOUT);

		if (ret) {
			dev_err(pdata->dev, "%s: cannot get hwspinlock: pin=%ld\n",
				__func__, pin);
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return;
		}
	}

	val = readb_relaxed(pdata->reg_base_irq_both_edge + reg_offset);

	if (is_both == true)
		val |= mask;
	else
		val &= ~mask;

	writeb_relaxed(val, (pdata->reg_base_irq_both_edge + reg_offset));

	if (pdata->hwlock)
		hwspin_unlock_in_atomic(pdata->hwlock);

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "%s: reg=0x%lx\n",
		__func__, (unsigned long)(pdata->reg_base_irq_both_edge + reg_offset));
	dev_dbg(pdata->dev, "%s: pin %ld, bit=0x%x, val=0x%x, mask=x0%x\n",
		__func__, pin, bit, val, mask);
}

static int mtk_dtv_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long pin = d->hwirq;

	if (pin >= gc->ngpio) {
		dev_err(pdata->dev,
			"irq set type: pin(%ld) is bigger than ngpio(%d)\n",
			pin, gc->ngpio);
		return -EINVAL;
	}

	/* IRQ_TYPE_PROBE is not supported. */
	if (type & ~IRQ_TYPE_SENSE_MASK) {
		dev_err(pdata->dev,
			"irq set type: pin(%ld) type(%d) is not supported\n",
			pin, type);
		return -EINVAL;
	}

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_BOTH:
		if (pdata->is_irq_both_edge) {
			_mtk_dtv_gpio_irq_set_both_edge(pdata, pin, true);
			/* clear polar setting */
			_mtk_dtv_gpio_irq_set_polar(pdata, pin, true, false);
			pdata->is_gpio_as_fiq[pin] = true;
			break;
		}
		/* else, edge both is not supported, use edge rising */
	case IRQ_TYPE_EDGE_RISING:
		/* GPIO FIQ, polarity = 0 */
		if (pdata->is_irq_both_edge) {
			/* clear both edge setting */
			_mtk_dtv_gpio_irq_set_both_edge(pdata, pin, false);
		}
		_mtk_dtv_gpio_irq_set_polar(pdata, pin, true, false);
		pdata->is_gpio_as_fiq[pin] = true;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		/* GPIO FIQ, polarity = 1 */
		if (pdata->is_irq_both_edge) {
			/* clear both edge setting */
			_mtk_dtv_gpio_irq_set_both_edge(pdata, pin, false);
		}
		_mtk_dtv_gpio_irq_set_polar(pdata, pin, true, true);
		pdata->is_gpio_as_fiq[pin] = true;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		/* GPIO IRQ, polarity = 0 */
		_mtk_dtv_gpio_irq_set_polar(pdata, pin, false, false);
		pdata->is_gpio_as_fiq[pin] = false;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		/* GPIO IRQ, polarity = 1 */
		_mtk_dtv_gpio_irq_set_polar(pdata, pin, false, true);
		pdata->is_gpio_as_fiq[pin] = false;
		break;
	}

	return 0;
}

static void _mtk_dtv_gpio_irq_set_mask(struct mtk_dtv_gpio *pdata,
					unsigned long pin,
					bool is_fiq, bool is_mask)
{
	unsigned long flags;
	u8 val, mask;
	u32 group, bit, reg_offset;
	int ret;

	if (is_fiq == true)
		reg_offset = (pin / 64) * 0x80;
	else
		reg_offset = ((pin / 64) * 0x80) + 0x40;

	reg_offset += MTK_GPIO_IRQC_MASK_OFFSET;

	group = (pin % 64) / 16;
	bit = (pin % 64) % 16;

	reg_offset += (group * 4);

	if (bit > 7)
		reg_offset += 1;

	bit %= 8;

	mask = OFFSET_TO_BIT(bit);

	raw_spin_lock_irqsave(&pdata->lock, flags);

	if (pdata->hwlock) {
		ret = hwspin_lock_timeout_in_atomic(pdata->hwlock, MTK_GPIO_HWSPINLOCK_TIMEOUT);

		if (ret) {
			dev_err(pdata->dev, "%s: cannot get hwspinlock: pin=%ld\n",
				__func__, pin);
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return;
		}
	}

	val = readb_relaxed(pdata->reg_base_irq + reg_offset);

	if (is_mask == true)
		val |= mask;
	else
		val &= ~mask;

	writeb_relaxed(val, (pdata->reg_base_irq + reg_offset));

	if (pdata->hwlock)
		hwspin_unlock_in_atomic(pdata->hwlock);

	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	dev_dbg(pdata->dev, "%s: reg=0x%lx\n",
		__func__, (unsigned long)(pdata->reg_base_irq + reg_offset));
	dev_dbg(pdata->dev, "%s: pin %ld, bit=0x%x, val=0x%x, mask=x0%x\n",
		__func__, pin, bit, val, mask);

	/* clear status, after unmask. write 1 to clear */
	if (is_mask == false) {
		reg_offset -= MTK_GPIO_IRQC_MASK_OFFSET;
		reg_offset += MTK_GPIO_IRQC_STATUS_OFFSET;

		raw_spin_lock_irqsave(&pdata->lock, flags);

		if (pdata->hwlock) {
			ret = hwspin_lock_timeout_in_atomic(pdata->hwlock,
							    MTK_GPIO_HWSPINLOCK_TIMEOUT);

			if (ret) {
				dev_err(pdata->dev, "%s: cannot get hwspinlock: pin=%ld\n",
					__func__, pin);
				raw_spin_unlock_irqrestore(&pdata->lock, flags);
				return;
			}
		}

		writeb_relaxed(mask, (pdata->reg_base_irq + reg_offset));

		if (pdata->hwlock)
			hwspin_unlock_in_atomic(pdata->hwlock);

		raw_spin_unlock_irqrestore(&pdata->lock, flags);

		dev_dbg(pdata->dev, "%s: clear status reg=0x%lx\n",
			__func__,
			(unsigned long)(pdata->reg_base_irq + reg_offset));
	}
}

static void mtk_dtv_gpio_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long pin = d->hwirq;

	/* set mask */
	_mtk_dtv_gpio_irq_set_mask(pdata, pin, pdata->is_gpio_as_fiq[pin], true);
}

static void mtk_dtv_gpio_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct mtk_dtv_gpio *pdata = gpiochip_get_data(gc);
	unsigned long pin = d->hwirq;

	/* set unmask and clear status */
	_mtk_dtv_gpio_irq_set_mask(pdata, pin, pdata->is_gpio_as_fiq[pin], false);
}

static void _mtk_dtv_gpio_of_get_misc(struct mtk_dtv_gpio *pdata, struct device_node *dn)
{
	int hwlock_id;

	/* hwspinlock for multi processors reg access atmoic protect */
	hwlock_id = of_hwspin_lock_get_id(dn, 0);
	if (hwlock_id < 0) {
		if (hwlock_id == -EPROBE_DEFER)
			dev_err(pdata->dev, "hwlock is -EPROBE_DEFER\n");

		pdata->hwlock = NULL;
	} else {
		dev_dbg(pdata->dev, "hwlock_id=%d\n", hwlock_id);
		pdata->hwlock = hwspin_lock_request_specific(hwlock_id);
	}
}

static int mtk_dtv_gpio_probe(struct platform_device *pdev)
{
	struct mtk_dtv_gpio *pdata;
	struct resource *res;
	struct device_node *dn;
	struct gpio_irq_chip *g_irqc;
	int ret, of_base, of_ngpio, of_val;
	bool is_request_irq = false;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	platform_set_drvdata(pdev, pdata);
	pdata->dev = &pdev->dev;

	/* gpio reg */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pdata->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->reg_base)) {
		dev_err(&pdev->dev, "err reg_base=0x%lx\n",
			(unsigned long)pdata->reg_base);
		return PTR_ERR(pdata->reg_base);
	}

	/* gpio interrupt reg */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	pdata->reg_base_irq = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->reg_base_irq)) {
		dev_err(&pdev->dev, "err reg_base_irq=0x%lx\n",
			(unsigned long)pdata->reg_base_irq);
		return PTR_ERR(pdata->reg_base_irq);
	}

	dev_dbg(&pdev->dev, "reg_base=0x%lx, reg_base_irq=0x%lx\n",
		 (unsigned long)pdata->reg_base,
		 (unsigned long)pdata->reg_base_irq);

	dn = pdev->dev.of_node;

	_mtk_dtv_gpio_of_get_misc(pdata, dn);

	raw_spin_lock_init(&pdata->lock);

	if (of_property_read_bool(dn, "gpio-ranges")) {
		pdata->gc.request = gpiochip_generic_request;
		pdata->gc.free = gpiochip_generic_free;
	}

	pdata->gc.get_direction = mtk_dtv_gpio_get_direction;
	pdata->gc.direction_input = mtk_dtv_gpio_direction_input;
	pdata->gc.direction_output = mtk_dtv_gpio_direction_output;
	pdata->gc.get = mtk_dtv_gpio_get_value;
	pdata->gc.set = mtk_dtv_gpio_set_value;

	pdata->gc.label = devm_kasprintf(&pdev->dev, GFP_KERNEL, "%s",
					 dev_name(&pdev->dev));
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

	/* get irq both edge supported flag */
	if (of_property_read_u32(dn, "irq-both-edge", &of_val)) {
		pdata->is_irq_both_edge = false;
		dev_info(&pdev->dev, "set irq-both-edge to default false\n");
	} else {
		if (of_val < 0) {
			dev_info(&pdev->dev, "bad irq-both-edge(%d), set it to default false\n",
				 of_val);
			pdata->is_irq_both_edge = false;
		} else {
			if (of_val == 0) {
				dev_dbg(&pdev->dev, "irq-both-edge not support\n");
				pdata->is_irq_both_edge = false;
			} else if (of_val == 1) {
				dev_dbg(&pdev->dev, "irq-both-edge support\n");
				pdata->is_irq_both_edge = true;
			} else {
				dev_info(&pdev->dev,
					 "unknown irq-both-edge(%d), set it to default false\n",
					 of_val);
				pdata->is_irq_both_edge = false;
			}
		}
	}

	/* get irq both edge reg */
	if (pdata->is_irq_both_edge) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		pdata->reg_base_irq_both_edge = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(pdata->reg_base_irq_both_edge)) {
			dev_err(&pdev->dev, "err reg_base_irq_both_edge=0x%lx\n",
				(unsigned long)pdata->reg_base_irq_both_edge);
			return PTR_ERR(pdata->reg_base_irq_both_edge);
		}

		dev_dbg(&pdev->dev, "reg_base_irq_both_edge=0x%lx\n",
			(unsigned long)pdata->reg_base_irq_both_edge);
	}

	dev_dbg(&pdev->dev, "base(%d), ngpios(%d)\n",
		pdata->gc.base, pdata->gc.ngpio);

	pdata->gpio_irq = irq_of_parse_and_map(dn, 0);
	pdata->gpio_fiq = irq_of_parse_and_map(dn, 1);
	if (pdata->gpio_irq > 0) {
		ret = devm_request_irq(&pdev->dev, pdata->gpio_irq,
				       mtk_dtv_gpio_irq_handler, IRQF_SHARED,
				       pdata->gc.label, &pdata->gc);

		if (ret) {
			dev_err(&pdev->dev, "Error requesting IRQ %d: %d\n",
				pdata->gpio_irq, ret);
			return ret;
		}
		dev_dbg(&pdev->dev, "request gpio irq(%d)\n", pdata->gpio_irq);
		is_request_irq = true;
	}

	if (pdata->gpio_fiq > 0) {
		ret = devm_request_irq(&pdev->dev, pdata->gpio_fiq,
				       mtk_dtv_gpio_fiq_handler, IRQF_SHARED,
				       pdata->gc.label, &pdata->gc);

		if (ret) {
			dev_err(&pdev->dev, "Error requesting IRQ %d: %d\n",
				pdata->gpio_fiq, ret);
			return ret;
		}
		dev_dbg(&pdev->dev, "request gpio fiq(%d)\n", pdata->gpio_fiq);
		is_request_irq = true;
	}

	if (is_request_irq == true) {
		pdata->ic.name = dev_name(&pdev->dev);
		//pdata->ic.parent_device = &pdev->dev;
		pdata->ic.irq_unmask = mtk_dtv_gpio_irq_unmask;
		pdata->ic.irq_mask = mtk_dtv_gpio_irq_mask;
		pdata->ic.irq_mask_ack = mtk_dtv_gpio_irq_mask;
		pdata->ic.irq_set_type = mtk_dtv_gpio_irq_set_type;

		g_irqc = &pdata->gc.irq;
		g_irqc->chip = &pdata->ic;
		/* This will let us handle the parent IRQ in the driver */
		g_irqc->parent_handler = NULL;
		g_irqc->num_parents = 0;
		g_irqc->parents = NULL;
		g_irqc->default_type = IRQ_TYPE_NONE;
		g_irqc->handler = handle_simple_irq;
	}

	ret = devm_gpiochip_add_data(&pdev->dev, &pdata->gc, pdata);
	if (ret) {
		dev_err(&pdev->dev, "probe add data error: ret=%d\n", ret);
		return ret;
	}

	/* if skip-str-restore exist, do not restore hw reg during resume */
	if (of_property_read_bool(dn, "skip-str-restore") == true) {
		dev_info(&pdev->dev, "skip str restore is true\n");
		pdata->is_skip_str_restore = true;
	} else {
		pdata->is_skip_str_restore = false;
	}

	dev_info(&pdev->dev, "gpio probe done\n");

	return 0;
}

static int mtk_dtv_gpio_suspend(struct device *dev)
{
	struct mtk_dtv_gpio *pdata = dev_get_drvdata(dev);
	unsigned char *dst, *src;
	int i;

	if (!pdata)
		return -EINVAL;

	if (pdata->is_skip_str_restore) {
		dev_info(pdata->dev, "skip str store in suspend\n");
	} else {
		/* gpio control reg */
		memcpy_fromio(pdata->gpio_pm_reg_save, pdata->reg_base, MTK_GPIO_REG_SIZE);

		/* gpio irq reg, skip status reg */
		for (i = 0; i < MTK_GPIO_REG_SIZE / MTK_GPIO_IRQ_REG_PER_SUB_SIZE; i++) {
			dst = pdata->gpio_irq_pm_reg_save + (i * MTK_GPIO_IRQ_REG_PER_SUB_SIZE);
			src = pdata->reg_base_irq + (i * MTK_GPIO_IRQ_REG_PER_SUB_SIZE);
			memcpy_fromio(dst, src, MTK_GPIO_IRQ_REG_NO_STAT_SIZE);
		}

		/* gpio irq both edge */
		if (pdata->is_irq_both_edge)
			memcpy_fromio(pdata->gpio_irq_both_edge_pm_reg_save,
				      pdata->reg_base_irq_both_edge, MTK_GPIO_REG_SIZE);
	}

	return 0;
}

static int mtk_dtv_gpio_resume(struct device *dev)
{
	struct mtk_dtv_gpio *pdata = dev_get_drvdata(dev);
	unsigned char *dst, *src;
	int i;

	if (!pdata)
		return -EINVAL;

	if (pdata->is_skip_str_restore) {
		dev_info(pdata->dev, "skip str restore in resume\n");
	} else {
		/* gpio control reg */
		memcpy_toio(pdata->reg_base, pdata->gpio_pm_reg_save, MTK_GPIO_REG_SIZE);

		/* gpio irq reg, skip status reg */
		for (i = 0; i < MTK_GPIO_REG_SIZE / MTK_GPIO_IRQ_REG_PER_SUB_SIZE; i++) {
			src = pdata->gpio_irq_pm_reg_save + (i * MTK_GPIO_IRQ_REG_PER_SUB_SIZE);
			dst = pdata->reg_base_irq + (i * MTK_GPIO_IRQ_REG_PER_SUB_SIZE);
			memcpy_toio(dst, src, MTK_GPIO_IRQ_REG_NO_STAT_SIZE);
		}

		/* gpio irq both edge */
		if (pdata->is_irq_both_edge)
			memcpy_toio(pdata->reg_base_irq_both_edge,
				    pdata->gpio_irq_both_edge_pm_reg_save, MTK_GPIO_REG_SIZE);
	}

	return 0;
}

static const struct dev_pm_ops mtk_dtv_gpio_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(mtk_dtv_gpio_suspend, mtk_dtv_gpio_resume)
};

static const struct of_device_id mtk_dtv_gpio_match[] = {
	{
		.compatible = "mediatek,mt5896-gpio",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_dtv_gpio_match);

static struct platform_driver mtk_dtv_gpio_driver = {
	.probe		= mtk_dtv_gpio_probe,
	.driver = {
		.name	= "mt5896-gpio",
		.of_match_table = of_match_ptr(mtk_dtv_gpio_match),
		.pm = &mtk_dtv_gpio_pm_ops,
	},
};

module_platform_driver(mtk_dtv_gpio_driver);

MODULE_DESCRIPTION("MediaTek mt5896 SoC based GPIO Driver");
MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");
MODULE_LICENSE("GPL");
