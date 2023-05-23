// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>

#include "core.h"
#include "devicetree.h"
#include "pinconf.h"
#include "pinmux.h"

#define DRIVER_NAME			"pinctrl-mt5896"
#define MTK_PCS_OFF_DISABLED		~0U
#define MTK_PINCTRL_REG_SIZE		0x200

struct mtk_pcs_func_vals {
	void __iomem *reg;
	unsigned int val;
	unsigned int mask;
};

struct mtk_pcs_conf_vals {
	enum pin_config_param param;
	unsigned int val;
	unsigned int enable;
	unsigned int disable;
	unsigned int mask;
};

struct mtk_pcs_conf_type {
	const char *name;
	enum pin_config_param param;
};

struct mtk_pcs_function {
	const char *name;
	struct mtk_pcs_func_vals *vals;
	unsigned int nvals;
	const char **pgnames;
	int npgnames;
	struct mtk_pcs_conf_vals *conf;
	int nconfs;
	struct list_head node;
};

struct mtk_pcs_gpiofunc_range {
	unsigned int offset;
	unsigned int npins;
	unsigned int gpiofunc;
	struct list_head node;
};

struct mtk_pcs_data {
	struct pinctrl_pin_desc *pa;
	int cur;
};

struct mtk_pcs_soc_data {
	unsigned int flags;
};

struct mtk_pcs_device {
	struct resource *res;
	void __iomem *base;
	void *saved_vals;
	unsigned int size;
	struct device *dev;
	struct device_node *np;
	struct pinctrl_dev *pctl;
	unsigned int flags;
#define MTK_PCS_CONTEXT_LOSS_OFF	(1 << 3)
#define MTK_PCS_QUIRK_SHARED_IRQ	(1 << 2)
#define MTK_PCS_FEAT_IRQ		(1 << 1)
#define MTK_PCS_FEAT_PINCONF	(1 << 0)
	struct property *missing_nr_pinctrl_cells;
	struct mtk_pcs_soc_data socdata;
	raw_spinlock_t lock;
	struct mutex mutex;
	unsigned int width;
	unsigned int fmask;
	unsigned int fshift;
	unsigned int foff;
	unsigned int fmax;
	unsigned int npins;
	bool bits_per_mux;
	unsigned int bits_per_pin;
	struct mtk_pcs_data pins;
	struct list_head gpiofuncs;
	struct pinctrl_desc desc;
	u8 pinctrl_pm_reg_save[MTK_PINCTRL_REG_SIZE];
	unsigned int (*read)(void __iomem *reg);
	void (*write)(unsigned int val, void __iomem *reg);
	bool is_skip_str_restore;
};

#define MTK_PCS_QUIRK_HAS_SHARED_IRQ	(pcs->flags & MTK_PCS_QUIRK_SHARED_IRQ)
#define MTK_PCS_HAS_IRQ		(pcs->flags & MTK_PCS_FEAT_IRQ)
#define MTK_PCS_HAS_PINCONF		(pcs->flags & MTK_PCS_FEAT_PINCONF)

static int mtk_pcs_pinconf_get(struct pinctrl_dev *pctldev, unsigned int pin,
			   unsigned long *config);
static int mtk_pcs_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
			   unsigned long *configs, unsigned int num_configs);

static enum pin_config_param mtk_pcs_bias[] = {
	PIN_CONFIG_BIAS_PULL_DOWN,
	PIN_CONFIG_BIAS_PULL_UP,
};

static unsigned int __maybe_unused mtk_pcs_readb(void __iomem *reg)
{
	return readb(reg);
}

static unsigned int __maybe_unused mtk_pcs_readw(void __iomem *reg)
{
	return readw(reg);
}

static unsigned int __maybe_unused mtk_pcs_readl(void __iomem *reg)
{
	return readl(reg);
}

static void __maybe_unused mtk_pcs_writeb(unsigned int val, void __iomem *reg)
{
	writeb(val, reg);
}

static void __maybe_unused mtk_pcs_writew(unsigned int val, void __iomem *reg)
{
	writew(val, reg);
}

static void __maybe_unused mtk_pcs_writel(unsigned int val, void __iomem *reg)
{
	writel(val, reg);
}

static void mtk_pcs_pin_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s,
					unsigned int pin)
{
	struct mtk_pcs_device *pcs;
	unsigned int val, mux_bytes;
	unsigned long offset;
	size_t pa;

	pcs = pinctrl_dev_get_drvdata(pctldev);

	mux_bytes = pcs->width / BITS_PER_BYTE;

	if (pcs->bits_per_mux) {
		offset = pin * mux_bytes;
		offset -= (pin % 4);
		if ((pin % 4) >= 2)
			offset += 1;
	} else {
		offset = pin * mux_bytes;
	}

	val = pcs->read(pcs->base + offset);
	pa = pcs->res->start + offset;

	seq_printf(s, "%zx %08x %s ", pa, val, DRIVER_NAME);
}

static void mtk_pcs_dt_free_map(struct pinctrl_dev *pctldev,
				struct pinctrl_map *map, unsigned int num_maps)
{
	struct mtk_pcs_device *pcs;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	devm_kfree(pcs->dev, map);
}

static int mtk_pcs_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned int *num_maps);

static const struct pinctrl_ops mtk_pcs_pinctrl_ops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
	.pin_dbg_show = mtk_pcs_pin_dbg_show,
	.dt_node_to_map = mtk_pcs_dt_node_to_map,
	.dt_free_map = mtk_pcs_dt_free_map,
};

static int mtk_pcs_get_function(struct pinctrl_dev *pctldev, unsigned int pin,
			    struct mtk_pcs_function **func)
{
	struct mtk_pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct pin_desc *pdesc = pin_desc_get(pctldev, pin);
	const struct pinctrl_setting_mux *setting;
	struct function_desc *function;
	unsigned int fselector;

	/* If pin is not described in DTS & enabled, mux_setting is NULL. */
	if (!pdesc) {
		dev_err(pcs->dev, "get null pin%d desc\n", pin);
		return -EINVAL;
	}
	setting = pdesc->mux_setting;
	if (!setting)
		return -ENOTSUPP;
	fselector = setting->func;
	function = pinmux_generic_get_function(pctldev, fselector);
	if (!function)
		return -EINVAL;

	*func = function->data;
	if (!(*func)) {
		dev_err(pcs->dev, "%s could not find function%i\n",
			__func__, fselector);
		return -ENOTSUPP;
	}
	return 0;
}

static int mtk_pcs_set_mux(struct pinctrl_dev *pctldev, unsigned int fselector,
	unsigned int group)
{
	struct mtk_pcs_device *pcs;
	struct function_desc *function;
	struct mtk_pcs_function *func;
	int i;

	pcs = pinctrl_dev_get_drvdata(pctldev);
	/* If function mask is null, needn't enable it. */
	if (!pcs->fmask)
		return 0;
	function = pinmux_generic_get_function(pctldev, fselector);
	if (!function)
		return -EINVAL;

	func = function->data;
	if (!func)
		return -EINVAL;

	dev_dbg(pcs->dev, "enabling %s function%i\n",
		func->name, fselector);

	for (i = 0; i < func->nvals; i++) {
		struct mtk_pcs_func_vals *vals;
		unsigned long flags;
		unsigned int val, mask;

		vals = &func->vals[i];
		raw_spin_lock_irqsave(&pcs->lock, flags);
		val = pcs->read(vals->reg);

		if (pcs->bits_per_mux)
			mask = vals->mask;
		else
			mask = pcs->fmask;

		val &= ~mask;
		val |= (vals->val & mask);
		pcs->write(val, vals->reg);
		raw_spin_unlock_irqrestore(&pcs->lock, flags);
	}

	return 0;
}

static int mtk_pcs_request_gpio(struct pinctrl_dev *pctldev,
			    struct pinctrl_gpio_range *range, unsigned int pin)
{
	struct mtk_pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct mtk_pcs_gpiofunc_range *frange = NULL;
	struct list_head *pos, *tmp;
	int mux_bytes = 0;
	unsigned int data;

	/* If function mask is null, return directly. */
	if (!pcs->fmask)
		return -ENOTSUPP;

	list_for_each_safe(pos, tmp, &pcs->gpiofuncs) {
		frange = list_entry(pos, struct mtk_pcs_gpiofunc_range, node);
		if (pin >= frange->offset + frange->npins
			|| pin < frange->offset)
			continue;
		mux_bytes = pcs->width / BITS_PER_BYTE;

		if (pcs->bits_per_mux) {
			int offset, pin_shift;

			offset = pin * mux_bytes;
			offset -= (pin % 4);
			if ((pin % 4) >= 2)
				offset += 1;

			pin_shift = pin % (pcs->width / pcs->bits_per_pin) *
				    pcs->bits_per_pin;

			data = pcs->read(pcs->base + offset);
			data &= ~(pcs->fmask << pin_shift);
			data |= frange->gpiofunc << pin_shift;
			pcs->write(data, pcs->base + offset);
		} else {
			data = pcs->read(pcs->base + pin * mux_bytes);
			data &= ~pcs->fmask;
			data |= frange->gpiofunc;
			pcs->write(data, pcs->base + pin * mux_bytes);
		}
		break;
	}
	return 0;
}

static const struct pinmux_ops mtk_pcs_pinmux_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = mtk_pcs_set_mux,
	.gpio_request_enable = mtk_pcs_request_gpio,
};

/* Clear BIAS value */
static void mtk_pcs_pinconf_clear_bias(struct pinctrl_dev *pctldev, unsigned int pin)
{
	unsigned long config;
	int i;

	for (i = 0; i < ARRAY_SIZE(mtk_pcs_bias); i++) {
		config = pinconf_to_config_packed(mtk_pcs_bias[i], 0);
		mtk_pcs_pinconf_set(pctldev, pin, &config, 1);
	}
}

/*
 * Check whether PIN_CONFIG_BIAS_DISABLE is valid.
 * It's depend on that PULL_DOWN & PULL_UP configs are all invalid.
 */
static bool mtk_pcs_pinconf_bias_disable(struct pinctrl_dev *pctldev, unsigned int pin)
{
	unsigned long config;
	int i;

	for (i = 0; i < ARRAY_SIZE(mtk_pcs_bias); i++) {
		config = pinconf_to_config_packed(mtk_pcs_bias[i], 0);
		if (!mtk_pcs_pinconf_get(pctldev, pin, &config))
			goto out;
	}
	return true;
out:
	return false;
}

static int mtk_pcs_pinconf_get(struct pinctrl_dev *pctldev,
				unsigned int pin, unsigned long *config)
{
	struct mtk_pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct mtk_pcs_function *func;
	enum pin_config_param param;
	unsigned int offset = 0, shift = 0, data = 0, i, j, ret, arg;

	ret = mtk_pcs_get_function(pctldev, pin, &func);
	if (ret)
		return ret;

	for (i = 0; i < func->nconfs; i++) {
		param = pinconf_to_config_param(*config);
		if (param == PIN_CONFIG_BIAS_DISABLE) {
			if (mtk_pcs_pinconf_bias_disable(pctldev, pin)) {
				*config = 0;
				return 0;
			} else {
				return -ENOTSUPP;
			}
		} else if (param != func->conf[i].param) {
			continue;
		}

		offset = pin * (pcs->width / BITS_PER_BYTE);
		data = pcs->read(pcs->base + offset) & func->conf[i].mask;

		switch (func->conf[i].param) {
		/* 4 parameters */
		case PIN_CONFIG_BIAS_PULL_DOWN:
		case PIN_CONFIG_BIAS_PULL_UP:
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			if ((data != func->conf[i].enable) ||
			    (data == func->conf[i].disable))
				return -ENOTSUPP;
			*config = 0;
			break;
		/* 2 parameters */
		case PIN_CONFIG_INPUT_SCHMITT:
			for (j = 0; j < func->nconfs; j++) {
				switch (func->conf[j].param) {
				case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
					if (data != func->conf[j].enable)
						return -ENOTSUPP;
					break;
				default:
					break;
				}
			}
			*config = data;
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
			shift = ffs(func->conf[i].mask) - 1;
			arg = (data >> shift);
			*config = pinconf_to_config_packed(func->conf[i].param, arg);
			break;
		case PIN_CONFIG_SLEW_RATE:
		case PIN_CONFIG_LOW_POWER_MODE:
		default:
			*config = data;
			break;
		}
		return 0;
	}
	return -ENOTSUPP;
}

static int mtk_pcs_pinconf_set(struct pinctrl_dev *pctldev,
				unsigned int pin, unsigned long *configs,
				unsigned int num_configs)
{
	struct mtk_pcs_device *pcs = pinctrl_dev_get_drvdata(pctldev);
	struct mtk_pcs_function *func;
	unsigned int offset = 0, shift = 0, i, data, ret;
	u32 arg;
	int j;

	ret = mtk_pcs_get_function(pctldev, pin, &func);
	if (ret)
		return ret;

	for (j = 0; j < num_configs; j++) {
		for (i = 0; i < func->nconfs; i++) {
			if (pinconf_to_config_param(configs[j])
				!= func->conf[i].param)
				continue;

			offset = pin * (pcs->width / BITS_PER_BYTE);
			data = pcs->read(pcs->base + offset);
			arg = pinconf_to_config_argument(configs[j]);
			switch (func->conf[i].param) {
			/* 2 parameters */
			case PIN_CONFIG_INPUT_SCHMITT:
			case PIN_CONFIG_DRIVE_STRENGTH:
			case PIN_CONFIG_SLEW_RATE:
			case PIN_CONFIG_LOW_POWER_MODE:
				shift = ffs(func->conf[i].mask) - 1;
				data &= ~func->conf[i].mask;
				data |= (arg << shift) & func->conf[i].mask;
				break;
			/* 4 parameters */
			case PIN_CONFIG_BIAS_DISABLE:
				mtk_pcs_pinconf_clear_bias(pctldev, pin);
				break;
			case PIN_CONFIG_BIAS_PULL_DOWN:
			case PIN_CONFIG_BIAS_PULL_UP:
				if (arg)
					mtk_pcs_pinconf_clear_bias(pctldev, pin);
				data &= ~func->conf[i].mask;
				if (arg)
					data |= func->conf[i].enable;
				else
					data |= func->conf[i].disable;
				break;
			case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
				data &= ~func->conf[i].mask;
				if (arg)
					data |= func->conf[i].enable;
				else
					data |= func->conf[i].disable;
				break;
			default:
				return -ENOTSUPP;
			}
			pcs->write(data, pcs->base + offset);

			break;
		}
		if (i >= func->nconfs)
			return -ENOTSUPP;
	} /* for each config */

	return 0;
}

static int mtk_pcs_pinconf_group_get(struct pinctrl_dev *pctldev,
				unsigned int group, unsigned long *config)
{
	const unsigned int *pins;
	unsigned int npins, old = 0;
	int i, ret;

	ret = pinctrl_generic_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < npins; i++) {
		if (mtk_pcs_pinconf_get(pctldev, pins[i], config))
			return -ENOTSUPP;
		/* configs do not match between two pins */
		if (i && (old != *config))
			return -ENOTSUPP;
		old = *config;
	}
	return 0;
}

static int mtk_pcs_pinconf_group_set(struct pinctrl_dev *pctldev,
				unsigned int group, unsigned long *configs,
				unsigned int num_configs)
{
	const unsigned int *pins;
	unsigned int npins;
	int i, ret;

	ret = pinctrl_generic_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < npins; i++) {
		if (mtk_pcs_pinconf_set(pctldev, pins[i], configs, num_configs))
			return -ENOTSUPP;
	}
	return 0;
}

static void mtk_pcs_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned int pin)
{
}

static void mtk_pcs_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned int selector)
{
}

static void mtk_pcs_pinconf_config_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s,
					unsigned long config)
{
	pinconf_generic_dump_config(pctldev, s, config);
}

static const struct pinconf_ops mtk_pcs_pinconf_ops = {
	.pin_config_get = mtk_pcs_pinconf_get,
	.pin_config_set = mtk_pcs_pinconf_set,
	.pin_config_group_get = mtk_pcs_pinconf_group_get,
	.pin_config_group_set = mtk_pcs_pinconf_group_set,
	.pin_config_dbg_show = mtk_pcs_pinconf_dbg_show,
	.pin_config_group_dbg_show = mtk_pcs_pinconf_group_dbg_show,
	.pin_config_config_dbg_show = mtk_pcs_pinconf_config_dbg_show,
	.is_generic = true,
};

static int mtk_pcs_add_pin(struct mtk_pcs_device *pcs, unsigned int offset,
		unsigned int pin_pos)
{
	struct pinctrl_pin_desc *pin;
	int i;

	i = pcs->pins.cur;
	if (i >= pcs->desc.npins) {
		dev_err(pcs->dev, "too many pins, max %i\n",
			pcs->desc.npins);
		return -ENOMEM;
	}

	pin = &pcs->pins.pa[i];
	pin->number = i;
	pcs->pins.cur++;

	return i;
}

static int mtk_pcs_allocate_pin_table(struct mtk_pcs_device *pcs)
{
	int mux_bytes, nr_pins, i;
	int num_pins_in_register = 0;

	mux_bytes = pcs->width / BITS_PER_BYTE;

	if (pcs->bits_per_mux) {
		pcs->bits_per_pin = fls(pcs->fmask);
		if (!pcs->bits_per_pin) {
			dev_err(pcs->dev, "zero bits_per_pin\n");
			return -EINVAL;
		}
		if (pcs->npins == 0)
			nr_pins = (pcs->size * BITS_PER_BYTE) / pcs->bits_per_pin / 2;
		else
			nr_pins = pcs->npins;

		num_pins_in_register = pcs->width / pcs->bits_per_pin;
	} else {
		if (pcs->npins == 0)
			nr_pins = pcs->size / mux_bytes;
		else
			nr_pins = pcs->npins;
	}

	dev_dbg(pcs->dev, "allocating %i pins\n", nr_pins);
	pcs->pins.pa = devm_kcalloc(pcs->dev,
				nr_pins, sizeof(*pcs->pins.pa),
				GFP_KERNEL);
	if (!pcs->pins.pa)
		return -ENOMEM;

	pcs->desc.pins = pcs->pins.pa;
	pcs->desc.npins = nr_pins;

	for (i = 0; i < pcs->desc.npins; i++) {
		unsigned int offset;
		int res;
		int byte_num;
		int pin_pos = 0;

		if (pcs->bits_per_mux) {
			byte_num = (pcs->bits_per_pin * i) / BITS_PER_BYTE;
			offset = (byte_num / mux_bytes) * mux_bytes;
			pin_pos = i % num_pins_in_register;
		} else {
			offset = i * mux_bytes;
		}
		res = mtk_pcs_add_pin(pcs, offset, pin_pos);
		if (res < 0) {
			dev_err(pcs->dev, "error adding pins: %i\n", res);
			return res;
		}
	}

	return 0;
}

static int mtk_pcs_add_function(struct mtk_pcs_device *pcs,
			    struct mtk_pcs_function **fcn,
			    const char *name,
			    struct mtk_pcs_func_vals *vals,
			    unsigned int nvals,
			    const char **pgnames,
			    unsigned int npgnames)
{
	struct mtk_pcs_function *function;
	int selector;

	function = devm_kzalloc(pcs->dev, sizeof(*function), GFP_KERNEL);
	if (!function)
		return -ENOMEM;

	function->vals = vals;
	function->nvals = nvals;

	selector = pinmux_generic_add_function(pcs->pctl, name,
					       pgnames, npgnames,
					       function);
	if (selector < 0) {
		devm_kfree(pcs->dev, function);
		*fcn = NULL;
	} else {
		*fcn = function;
	}

	return selector;
}

static int mtk_pcs_get_pin_by_offset(struct mtk_pcs_device *pcs, unsigned int offset)
{
	unsigned int index;

	if (offset >= pcs->size) {
		dev_err(pcs->dev, "mux offset out of range: 0x%x (0x%x)\n",
			offset, pcs->size);
		return -EINVAL;
	}

	if (pcs->bits_per_mux) {
		index = offset;
		index -= (offset % 2);
		if ((offset % 2) != 0)
			index += 2;
	} else
		index = offset / (pcs->width / BITS_PER_BYTE);

	return index;
}

static int mtk_pcs_config_match(unsigned int data, unsigned int enable, unsigned int disable)
{
	int ret = -EINVAL;

	if (data == enable)
		ret = 1;
	else if (data == disable)
		ret = 0;
	return ret;
}

static void add_config(struct mtk_pcs_conf_vals **conf, enum pin_config_param param,
		       unsigned int value, unsigned int enable, unsigned int disable,
		       unsigned int mask)
{
	(*conf)->param = param;
	(*conf)->val = value;
	(*conf)->enable = enable;
	(*conf)->disable = disable;
	(*conf)->mask = mask;
	(*conf)++;
}

static void add_setting(unsigned long **setting, enum pin_config_param param,
			unsigned int arg)
{
	**setting = pinconf_to_config_packed(param, arg);
	(*setting)++;
}

/* add pinconf setting with 2 parameters */
static void mtk_pcs_add_conf2(struct mtk_pcs_device *pcs, struct device_node *np,
			  const char *name, enum pin_config_param param,
			  struct mtk_pcs_conf_vals **conf, unsigned long **settings)
{
	unsigned int value[2], shift;
	int ret;

	ret = of_property_read_u32_array(np, name, value, 2);
	if (ret)
		return;
	/* set value & mask */
	value[1] &= value[0];
	shift = ffs(value[0]) - 1;
	/* skip enable & disable */
	add_config(conf, param, value[1], 0, 0, value[0]);
	add_setting(settings, param, value[1] >> shift);
}

/* add pinconf setting with 4 parameters */
static void mtk_pcs_add_conf4(struct mtk_pcs_device *pcs, struct device_node *np,
			  const char *name, enum pin_config_param param,
			  struct mtk_pcs_conf_vals **conf, unsigned long **settings)
{
	unsigned int value[4];
	int ret;

	/* value to set, enable, disable, mask */
	ret = of_property_read_u32_array(np, name, value, 4);
	if (ret)
		return;
	if (!value[3]) {
		dev_err(pcs->dev, "mask field of the property can't be 0\n");
		return;
	}
	value[0] &= value[3];
	value[1] &= value[3];
	value[2] &= value[3];
	ret = mtk_pcs_config_match(value[0], value[1], value[2]);
	if (ret < 0)
		dev_err(pcs->dev, "failed to match enable or disable bits\n");
	add_config(conf, param, value[0], value[1], value[2], value[3]);
	add_setting(settings, param, ret);
}

static int mtk_pcs_parse_pinconf(struct mtk_pcs_device *pcs, struct device_node *np,
			     struct mtk_pcs_function *func,
			     struct pinctrl_map **map)

{
	struct pinctrl_map *m = *map;
	int i = 0, nconfs = 0;
	unsigned long *settings = NULL, *s = NULL;
	struct mtk_pcs_conf_vals *conf = NULL;
	static const struct mtk_pcs_conf_type prop2[] = {
		{ "pinctrl-mt5896,drive-strength", PIN_CONFIG_DRIVE_STRENGTH, },
		{ "pinctrl-mt5896,slew-rate", PIN_CONFIG_SLEW_RATE, },
		{ "pinctrl-mt5896,input-schmitt", PIN_CONFIG_INPUT_SCHMITT, },
		{ "pinctrl-mt5896,low-power-mode", PIN_CONFIG_LOW_POWER_MODE, },
	};
	static const struct mtk_pcs_conf_type prop4[] = {
		{ "pinctrl-mt5896,bias-pullup", PIN_CONFIG_BIAS_PULL_UP, },
		{ "pinctrl-mt5896,bias-pulldown", PIN_CONFIG_BIAS_PULL_DOWN, },
		{ "pinctrl-mt5896,input-schmitt-enable",
			PIN_CONFIG_INPUT_SCHMITT_ENABLE, },
	};

	/* If pinconf isn't supported, don't parse properties in below. */
	if (!MTK_PCS_HAS_PINCONF)
		return 0;

	/* cacluate how much properties are supported in current node */
	for (i = 0; i < ARRAY_SIZE(prop2); i++) {
		if (of_find_property(np, prop2[i].name, NULL))
			nconfs++;
	}
	for (i = 0; i < ARRAY_SIZE(prop4); i++) {
		if (of_find_property(np, prop4[i].name, NULL))
			nconfs++;
	}
	if (!nconfs)
		return 0;

	func->conf = devm_kcalloc(pcs->dev,
				  nconfs, sizeof(struct mtk_pcs_conf_vals),
				  GFP_KERNEL);
	if (!func->conf)
		return -ENOMEM;
	func->nconfs = nconfs;
	conf = &(func->conf[0]);
	m++;
	settings = devm_kcalloc(pcs->dev, nconfs, sizeof(unsigned long),
				GFP_KERNEL);
	if (!settings)
		return -ENOMEM;
	s = &settings[0];

	for (i = 0; i < ARRAY_SIZE(prop2); i++)
		mtk_pcs_add_conf2(pcs, np, prop2[i].name, prop2[i].param,
			      &conf, &s);
	for (i = 0; i < ARRAY_SIZE(prop4); i++)
		mtk_pcs_add_conf4(pcs, np, prop4[i].name, prop4[i].param,
			      &conf, &s);
	m->type = PIN_MAP_TYPE_CONFIGS_GROUP;
	m->data.configs.group_or_pin = np->name;
	m->data.configs.configs = settings;
	m->data.configs.num_configs = nconfs;
	return 0;
}

static int mtk_pcs_parse_one_pinctrl_entry(struct mtk_pcs_device *pcs,
						struct device_node *np,
						struct pinctrl_map **map,
						unsigned int *num_maps,
						const char **pgnames)
{
	const char *name = "pinctrl-mt5896,pins";
	struct mtk_pcs_func_vals *vals;
	int rows, *pins, found = 0, res = -ENOMEM, i, fsel, gsel;
	struct mtk_pcs_function *function = NULL;

	rows = pinctrl_count_index_with_args(np, name);
	if (rows <= 0) {
		dev_err(pcs->dev, "Invalid number of rows: %d\n", rows);
		return -EINVAL;
	}

	vals = devm_kcalloc(pcs->dev, rows, sizeof(*vals), GFP_KERNEL);
	if (!vals)
		return -ENOMEM;

	pins = devm_kcalloc(pcs->dev, rows, sizeof(*pins), GFP_KERNEL);
	if (!pins)
		goto free_vals;

	for (i = 0; i < rows; i++) {
		struct of_phandle_args pinctrl_spec;
		unsigned int offset;
		int pin;

		res = pinctrl_parse_index_with_args(np, name, i, &pinctrl_spec);
		if (res)
			return res;

		if (pinctrl_spec.args_count < 2) {
			dev_err(pcs->dev, "invalid args_count for spec: %i\n",
				pinctrl_spec.args_count);
			break;
		}

		/* Index plus one value cell */
		offset = pinctrl_spec.args[0];
		vals[found].reg = pcs->base + offset;
		vals[found].val = pinctrl_spec.args[1];

		dev_dbg(pcs->dev, "%pOFn index: 0x%x value: 0x%x\n",
			pinctrl_spec.np, offset, pinctrl_spec.args[1]);

		pin = mtk_pcs_get_pin_by_offset(pcs, offset);
		if (pin < 0) {
			dev_err(pcs->dev,
				"could not add functions for %pOFn %ux\n",
				np, offset);
			break;
		}
		pins[found++] = pin;
	}

	pgnames[0] = np->name;
	mutex_lock(&pcs->mutex);
	fsel = mtk_pcs_add_function(pcs, &function, np->name, vals, found,
				pgnames, 1);
	if (fsel < 0) {
		res = fsel;
		goto free_pins;
	}

	gsel = pinctrl_generic_add_group(pcs->pctl, np->name, pins, found, pcs);
	if (gsel < 0) {
		res = gsel;
		goto free_function;
	}

	(*map)->type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)->data.mux.group = np->name;
	(*map)->data.mux.function = np->name;

	if (MTK_PCS_HAS_PINCONF && function) {
		res = mtk_pcs_parse_pinconf(pcs, np, function, map);
		if (res)
			goto free_pingroups;
		*num_maps = 2;
	} else {
		*num_maps = 1;
	}
	mutex_unlock(&pcs->mutex);

	return 0;

free_pingroups:
	pinctrl_generic_remove_group(pcs->pctl, gsel);
	*num_maps = 1;
free_function:
	pinmux_generic_remove_function(pcs->pctl, fsel);
free_pins:
	mutex_unlock(&pcs->mutex);
	devm_kfree(pcs->dev, pins);

free_vals:
	devm_kfree(pcs->dev, vals);

	return res;
}

static int mtk_pcs_parse_bits_in_pinctrl_entry(struct mtk_pcs_device *pcs,
						struct device_node *np,
						struct pinctrl_map **map,
						unsigned int *num_maps,
						const char **pgnames)
{
	const char *name = "pinctrl-mt5896,bits";
	struct mtk_pcs_func_vals *vals;
	int rows, *pins, found = 0, res = -ENOMEM, i, fsel, gsel;
	int npins_in_row;
	struct mtk_pcs_function *function = NULL;

	rows = pinctrl_count_index_with_args(np, name);
	if (rows <= 0) {
		dev_err(pcs->dev, "Invalid number of rows: %d\n", rows);
		return -EINVAL;
	}

	npins_in_row = pcs->width / pcs->bits_per_pin;

	vals = devm_kzalloc(pcs->dev,
			    array3_size(rows, npins_in_row, sizeof(*vals)),
			    GFP_KERNEL);
	if (!vals)
		return -ENOMEM;

	pins = devm_kzalloc(pcs->dev,
			    array3_size(rows, npins_in_row, sizeof(*pins)),
			    GFP_KERNEL);
	if (!pins)
		goto free_vals;

	for (i = 0; i < rows; i++) {
		struct of_phandle_args pinctrl_spec;
		unsigned int offset, val;
		unsigned int mask, bit_pos, val_pos, mask_pos, submask;
		unsigned int pin_num_from_lsb;
		int pin;

		res = pinctrl_parse_index_with_args(np, name, i, &pinctrl_spec);
		if (res)
			return res;

		if (pinctrl_spec.args_count < 3) {
			dev_err(pcs->dev, "invalid args_count for spec: %i\n",
				pinctrl_spec.args_count);
			break;
		}

		/* Index plus two value cells */
		offset = pinctrl_spec.args[0];
		mask = pinctrl_spec.args[1];
		val = pinctrl_spec.args[2];
		if (mask == 0xf0)
			val <<= 4;

		dev_dbg(pcs->dev, "%pOFn index: 0x%x value: 0x%x mask: 0x%x\n",
			pinctrl_spec.np, offset, val, mask);

		/* Parse pins in each row from LSB */
		while (mask) {
			bit_pos = __ffs(mask);
			pin_num_from_lsb = bit_pos / pcs->bits_per_pin;
			mask_pos = ((pcs->fmask) << bit_pos);
			val_pos = val & mask_pos;
			submask = mask & mask_pos;

			if ((mask & mask_pos) == 0) {
				dev_err(pcs->dev,
					"Invalid mask for %pOFn at 0x%x\n",
					np, offset);
				break;
			}

			mask &= ~mask_pos;

			if (submask != mask_pos) {
				dev_warn(pcs->dev,
						"Invalid submask 0x%x for %pOFn at 0x%x\n",
						submask, np, offset);
				continue;
			}

			vals[found].mask = submask;
			vals[found].reg = pcs->base + offset;
			vals[found].val = val_pos;

			pin = mtk_pcs_get_pin_by_offset(pcs, offset);
			if (pin < 0) {
				dev_err(pcs->dev,
					"could not add functions for %pOFn %ux\n",
					np, offset);
				break;
			}
			pins[found++] = pin + pin_num_from_lsb;
		}
	}

	pgnames[0] = np->name;
	mutex_lock(&pcs->mutex);
	fsel = mtk_pcs_add_function(pcs, &function, np->name, vals, found,
				pgnames, 1);
	if (fsel < 0) {
		res = fsel;
		goto free_pins;
	}

	gsel = pinctrl_generic_add_group(pcs->pctl, np->name, pins, found, pcs);
	if (gsel < 0) {
		res = gsel;
		goto free_function;
	}

	(*map)->type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)->data.mux.group = np->name;
	(*map)->data.mux.function = np->name;

	if (MTK_PCS_HAS_PINCONF) {
		dev_err(pcs->dev, "pinconf not supported\n");
		goto free_pingroups;
	}

	*num_maps = 1;
	mutex_unlock(&pcs->mutex);

	return 0;

free_pingroups:
	pinctrl_generic_remove_group(pcs->pctl, gsel);
	*num_maps = 1;
free_function:
	pinmux_generic_remove_function(pcs->pctl, fsel);
free_pins:
	mutex_unlock(&pcs->mutex);
	devm_kfree(pcs->dev, pins);

free_vals:
	devm_kfree(pcs->dev, vals);

	return res;
}

static int mtk_pcs_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned int *num_maps)
{
	struct mtk_pcs_device *pcs;
	const char **pgnames;
	int ret;

	pcs = pinctrl_dev_get_drvdata(pctldev);

	/* create 2 maps. One is for pinmux, and the other is for pinconf. */
	*map = devm_kcalloc(pcs->dev, 2, sizeof(**map), GFP_KERNEL);
	if (!*map)
		return -ENOMEM;

	*num_maps = 0;

	pgnames = devm_kzalloc(pcs->dev, sizeof(*pgnames), GFP_KERNEL);
	if (!pgnames) {
		ret = -ENOMEM;
		goto free_map;
	}

	if (pcs->bits_per_mux) {
		ret = mtk_pcs_parse_bits_in_pinctrl_entry(pcs, np_config, map,
				num_maps, pgnames);
		if (ret < 0) {
			dev_err(pcs->dev, "no pins entries for %pOFn\n",
				np_config);
			goto free_pgnames;
		}
	} else {
		ret = mtk_pcs_parse_one_pinctrl_entry(pcs, np_config, map,
				num_maps, pgnames);
		if (ret < 0) {
			dev_err(pcs->dev, "no pins entries for %pOFn\n",
				np_config);
			goto free_pgnames;
		}
	}

	return 0;

free_pgnames:
	devm_kfree(pcs->dev, pgnames);
free_map:
	devm_kfree(pcs->dev, *map);

	return ret;
}

static void mtk_pcs_free_resources(struct mtk_pcs_device *pcs)
{
	pinctrl_unregister(pcs->pctl);
}

static int mtk_pcs_add_gpio_func(struct device_node *node, struct mtk_pcs_device *pcs)
{
	const char *propname = "pinctrl-mt5896,gpio-range";
	const char *cellname = "#pinctrl-mt5896,gpio-range-cells";
	struct of_phandle_args gpiospec;
	struct mtk_pcs_gpiofunc_range *range;
	int ret, i;

	for (i = 0; ; i++) {
		ret = of_parse_phandle_with_args(node, propname, cellname,
						 i, &gpiospec);
		/* Do not treat it as error. Only treat it as end condition. */
		if (ret) {
			ret = 0;
			break;
		}
		range = devm_kzalloc(pcs->dev, sizeof(*range), GFP_KERNEL);
		if (!range) {
			ret = -ENOMEM;
			break;
		}
		range->offset = gpiospec.args[0];
		range->npins = gpiospec.args[1];
		range->gpiofunc = gpiospec.args[2];
		mutex_lock(&pcs->mutex);
		list_add_tail(&range->node, &pcs->gpiofuncs);
		mutex_unlock(&pcs->mutex);
	}
	return ret;
}

static int pinctrl_mt5896_suspend(struct device *dev)
{
	struct mtk_pcs_device *pcs;

	pcs = dev_get_drvdata(dev);
	if (!pcs)
		return -EINVAL;

	if (pcs->is_skip_str_restore)
		dev_info(pcs->dev, "skip str store in suspend\n");
	else
		memcpy_fromio(pcs->pinctrl_pm_reg_save, pcs->base, MTK_PINCTRL_REG_SIZE);

	return pinctrl_force_sleep(pcs->pctl);
}

static int pinctrl_mt5896_resume(struct device *dev)
{
	struct mtk_pcs_device *pcs;

	pcs = dev_get_drvdata(dev);
	if (!pcs)
		return -EINVAL;

	if (pcs->is_skip_str_restore)
		dev_info(pcs->dev, "skip str restore in resume\n");
	else
		memcpy_toio(pcs->base, pcs->pinctrl_pm_reg_save, MTK_PINCTRL_REG_SIZE);

	return pinctrl_force_default(pcs->pctl);
}

static int mtk_pcs_quirk_missing_pinctrl_cells(struct mtk_pcs_device *pcs,
					   struct device_node *np,
					   int cells)
{
	struct property *p;
	const char *name = "#pinctrl-cells";
	int error;
	u32 val;

	error = of_property_read_u32(np, name, &val);
	if (!error)
		return 0;

	dev_warn(pcs->dev, "please update dts to use %s = <%i>\n",
		 name, cells);

	p = devm_kzalloc(pcs->dev, sizeof(*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	p->length = sizeof(__be32);
	p->value = devm_kzalloc(pcs->dev, sizeof(__be32), GFP_KERNEL);
	if (!p->value)
		return -ENOMEM;
	*(__be32 *)p->value = cpu_to_be32(cells);

	p->name = devm_kstrdup(pcs->dev, name, GFP_KERNEL);
	if (!p->name)
		return -ENOMEM;

	pcs->missing_nr_pinctrl_cells = p;

	return error;
}

static int mtk_pcs_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct mtk_pcs_device *pcs;
	const struct mtk_pcs_soc_data *soc;
	int ret;

	soc = of_device_get_match_data(&pdev->dev);
	if (WARN_ON(!soc))
		return -EINVAL;

	pcs = devm_kzalloc(&pdev->dev, sizeof(*pcs), GFP_KERNEL);
	if (!pcs)
		return -ENOMEM;

	pcs->dev = &pdev->dev;
	pcs->np = np;
	raw_spin_lock_init(&pcs->lock);
	mutex_init(&pcs->mutex);
	INIT_LIST_HEAD(&pcs->gpiofuncs);
	pcs->flags = soc->flags;
	memcpy(&pcs->socdata, soc, sizeof(*soc));

	ret = of_property_read_u32(np, "pinctrl-mt5896,register-width",
				   &pcs->width);
	if (ret) {
		dev_err(pcs->dev, "register width not specified\n");

		return ret;
	}

	ret = of_property_read_u32(np, "pinctrl-mt5896,function-mask",
				   &pcs->fmask);
	if (!ret) {
		pcs->fshift = __ffs(pcs->fmask);
		pcs->fmax = pcs->fmask >> pcs->fshift;
	} else {
		/* If mask property doesn't exist, function mux is invalid. */
		pcs->fmask = 0;
		pcs->fshift = 0;
		pcs->fmax = 0;
	}

	ret = of_property_read_u32(np, "pinctrl-mt5896,function-off",
					&pcs->foff);
	if (ret)
		pcs->foff = MTK_PCS_OFF_DISABLED;

	ret = of_property_read_u32(np, "pinctrl-mt5896,npins",
				   &pcs->npins);
	if (ret)
		pcs->npins = 0;

	pcs->bits_per_mux = of_property_read_bool(np,
						  "pinctrl-mt5896,bit-per-mux");
	ret = mtk_pcs_quirk_missing_pinctrl_cells(pcs, np,
					      pcs->bits_per_mux ? 2 : 1);
	if (ret) {
		dev_err(&pdev->dev, "unable to patch #pinctrl-cells\n");

		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(pcs->dev, "could not get resource\n");
		return -ENODEV;
	}

	pcs->res = devm_request_mem_region(pcs->dev, res->start,
			resource_size(res), DRIVER_NAME);
	if (!pcs->res) {
		dev_err(pcs->dev, "could not get mem_region\n");
		return -EBUSY;
	}

	pcs->size = resource_size(pcs->res);
	pcs->base = devm_ioremap(pcs->dev, pcs->res->start, pcs->size);
	if (!pcs->base) {
		dev_err(pcs->dev, "could not ioremap\n");
		return -ENODEV;
	}

	platform_set_drvdata(pdev, pcs);

	switch (pcs->width) {
	case 8:
		pcs->read = mtk_pcs_readb;
		pcs->write = mtk_pcs_writeb;
		break;
	case 16:
		pcs->read = mtk_pcs_readw;
		pcs->write = mtk_pcs_writew;
		break;
	case 32:
	case 64:
		pcs->read = mtk_pcs_readl;
		pcs->write = mtk_pcs_writel;
		break;
	default:
		break;
	}

	pcs->desc.name = DRIVER_NAME;
	pcs->desc.pctlops = &mtk_pcs_pinctrl_ops;
	pcs->desc.pmxops = &mtk_pcs_pinmux_ops;
	if (MTK_PCS_HAS_PINCONF)
		pcs->desc.confops = &mtk_pcs_pinconf_ops;
	pcs->desc.owner = THIS_MODULE;

	ret = mtk_pcs_allocate_pin_table(pcs);
	if (ret < 0)
		goto free;

	ret = pinctrl_register_and_init(&pcs->desc, pcs->dev, pcs, &pcs->pctl);
	if (ret) {
		dev_err(pcs->dev, "could not register mt5896 pinctrl driver\n");
		goto free;
	}

	ret = mtk_pcs_add_gpio_func(np, pcs);
	if (ret < 0)
		goto free;

	/* if skip-str-restore exist, do not restore hw reg during resume */
	if (of_property_read_bool(np, "skip-str-restore") == true) {
		dev_info(pcs->dev, "skip str restore is true\n");
		pcs->is_skip_str_restore = true;
	} else {
		pcs->is_skip_str_restore = false;
	}

	dev_info(pcs->dev, "%i pins, size %u\n", pcs->desc.npins, pcs->size);

	return pinctrl_enable(pcs->pctl);

free:
	mtk_pcs_free_resources(pcs);

	return ret;
}

static int mtk_pcs_remove(struct platform_device *pdev)
{
	struct mtk_pcs_device *pcs = platform_get_drvdata(pdev);

	if (!pcs)
		return 0;

	mtk_pcs_free_resources(pcs);

	return 0;
}

static const struct mtk_pcs_soc_data pinctrl_mt5896 = {
};

static const struct mtk_pcs_soc_data pinconf_mt5896 = {
	.flags = MTK_PCS_FEAT_PINCONF,
};

static const struct dev_pm_ops mtk_pcs_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(pinctrl_mt5896_suspend, pinctrl_mt5896_resume)
};

static const struct of_device_id mtk_pcs_of_match[] = {
	{ .compatible = "mediatek,mt5896-pinctrl", .data = &pinctrl_mt5896 },
	{ .compatible = "mediatek,mt5896-pinconf", .data = &pinconf_mt5896 },
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_pcs_of_match);

static struct platform_driver mtk_pcs_driver = {
	.probe		= mtk_pcs_probe,
	.remove		= mtk_pcs_remove,
	.driver = {
		.name		= DRIVER_NAME,
		.of_match_table	= mtk_pcs_of_match,
		.pm		= &mtk_pcs_pm_ops,
#ifndef MODULE
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
#endif
	},
};

static int __init mtk_pcs_init(void)
{
	return platform_driver_register(&mtk_pcs_driver);
}
arch_initcall(mtk_pcs_init);

MODULE_DESCRIPTION("MediaTek mt5896 SoC based Pinctrl Driver");
MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");
MODULE_LICENSE("GPL");
