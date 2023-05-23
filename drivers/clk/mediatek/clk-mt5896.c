// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Joy Huang <joy-mi.huang@mediatek.com>
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/slab.h>
#include <linux/kallsyms.h>

#include "clk-mtk.h"
#include "clk-gate.h"
#include "clk-dtv.h"

static DEFINE_SPINLOCK(mt5896_clk_lock);
static LIST_HEAD(clock_reg_storage);

static int clk_str_log;
static int clk_init_log;
static int clk_init_dbg;
static int clk_gnl_rw_log;

module_param(clk_init_log, int, 0644);
module_param(clk_init_dbg, int, 0644);
module_param(clk_str_log, int, 0644);
module_param(clk_gnl_rw_log, int, 0644);
MODULE_PARM_DESC(clk_init_log, "clk boot info switch\n");
MODULE_PARM_DESC(clk_init_dbg, "clk boot dbg switch\n");
MODULE_PARM_DESC(clk_str_log, "clk str info switch\n");
MODULE_PARM_DESC(clk_gnl_rw_log, "clk gnl w info switch\n");

#define CLK_STR_INFO(fmt, args...)	do { \
	if (clk_str_log > 0)  \
		pr_crit("[%s] " fmt,  \
			__func__, ## args);    \
} while (0)

#define CLK_BOOT_INFO(fmt, args...)	do { \
	if (clk_init_log > 0)  \
		pr_crit("[%s] " fmt,  \
			__func__, ## args);    \
} while (0)

#define CLK_BOOT_DBG(fmt, args...)	do { \
	if (clk_init_dbg > 0)  \
		pr_crit("[%s] " fmt,  \
			__func__, ## args);    \
} while (0)

#define CLK_GNL_RW_INFO(fmt, args...)	do { \
	if (clk_gnl_rw_log > 0)  \
		pr_crit("[%s] " fmt,  \
			__func__, ## args);    \
} while (0)

struct dtv_clk_reg_access *clk_pm;
struct dtv_clk_reg_access *clk_nonpm;

#define MAX_FREQ_INPUT	4294 /* ULONG_MAX/MHZ */
static void __init of_mtk_dtv_fix(struct device_node *node)
{

	int ret;
	u32 size = 0, index;
	u32 freq;
	struct device_node *child = NULL;
	struct mtk_fixed_clk *fixed_clks;
	struct clk_onecell_data *clk_data;
	int i;

	CLK_BOOT_INFO("[%s] : %s\n", __func__, node->name);

	ret = of_property_read_u32(node, "num", &size);
	if (size > 0)
		fixed_clks = kcalloc(size, sizeof(*fixed_clks), GFP_KERNEL);
	else
		return;

	clk_data = mtk_alloc_clk_data(size);

	for_each_child_of_node(node, child) {
		ret = 0;
		freq = 0;

		if (!of_device_is_available(child)) {
			CLK_BOOT_INFO("%s disabled\n", child->name);
			continue;
		}

		ret = of_property_read_u32(child, "id", &index);
		if (index >= size || ret < 0) {
			pr_err("[%s] %s bad index:%d, size: %d ret:%d\n",
					node->name, child->name, index, size, ret);
			continue;
		}

		if (fixed_clks[index].id) {
			pr_err("fatal error: duplicated ID[%d]%s %d\n",
					index, child->name, fixed_clks[index].id);
			continue;
		}

		/* Import clk freq */
		ret = of_property_read_u32(child, "clock-frequency", &freq);
		if (freq > MAX_FREQ_INPUT || ret < 0) {
			pr_err("[%s]%s bad freq:%u ret:%d\n",
					node->name, child->name, freq, ret);
			continue;
		}
		fixed_clks[index].rate = freq*MHZ;
		/* Import clk name */
		fixed_clks[index].name = child->name;
		fixed_clks[index].id = index;
		fixed_clks[index].parent = NULL;
		CLK_BOOT_INFO("[%d] %lu %s\n", index,
					fixed_clks[index].rate, child->name);
	}

	mtk_clk_register_fixed_clks(fixed_clks, size, clk_data);

	if (strstr(node->name, "topgen")) {
		for (i = 0; i < size; i++) {
			if (!IS_ERR_OR_NULL(clk_data->clks[i]))
				clk_prepare_enable(clk_data->clks[i]);
		}
	}
	of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	kfree(fixed_clks);

}

static void __init of_mtk_dtv_fix_factor(struct device_node *node)
{

	int ret;
	u32 size = 0, index;
	struct mtk_fixed_factor *fixed_divs;
	struct device_node *child = NULL;
	struct device_node *gChild = NULL;
	struct clk_onecell_data *clk_data;
	int i;

	CLK_BOOT_INFO("[%s] : %s\n", __func__, node->name);

	ret = of_property_read_u32(node, "num", &size);

	if (size > 0)
		fixed_divs = kcalloc(size, sizeof(*fixed_divs), GFP_KERNEL);
	else
		return;

	clk_data = mtk_alloc_clk_data(size);

	for_each_child_of_node(node, child) {
		for_each_child_of_node(child, gChild) {
			u32 div_factor[2] = {0};
			ret = 0;

			if (!of_device_is_available(gChild)) {
				CLK_BOOT_INFO("%s disabled\n", gChild->name);
				continue;
			}

			ret = of_property_read_u32(gChild, "id", &index);
			if (index >= size || ret < 0) {
				pr_err("[%s]%s bad index:%d, size:%d ret:%d\n",
					node->name, gChild->name, index, size, ret);
				continue;
			}
			if (fixed_divs[index].id) {
				pr_err("fatal error: duplicated ID[%d]%s %d\n",
						index, gChild->name, fixed_divs[index].id);
				continue;
			}
			fixed_divs[index].id = index;

			/* Import DIV factor info */
			ret = of_property_read_u32_array(gChild, "div_factor", div_factor, 2);
			if (ret < 0) {
				pr_err("[%s]%s bad div_factor ret:%d\n",
						node->name, gChild->name, ret);
				continue;
			}
			fixed_divs[index].mult = div_factor[0];
			fixed_divs[index].div = div_factor[1];

			/* Import clk and parent name : the sub-node and node name */
			fixed_divs[index].name = gChild->name;
			fixed_divs[index].parent_name = child->name;

			CLK_BOOT_INFO("[%d] %s , p=%s mult:%u, div:%u\n",
					fixed_divs[index].id,
					fixed_divs[index].name, fixed_divs[index].parent_name,
					fixed_divs[index].mult, fixed_divs[index].div
				);

		}
	}

	mtk_clk_register_factors(fixed_divs, size, clk_data);

	if (strstr(node->name, "topgen")) {
		for (i = 0; i < size; i++) {
			if (!IS_ERR_OR_NULL(clk_data->clks[i]))
				clk_prepare_enable(clk_data->clks[i]);
		}
	}

	of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	kfree(fixed_divs);

}

static inline void get_gnl_base(struct device_node *node, void __iomem **base)
{
	if (of_property_read_bool(node, "NONPM") && clk_nonpm)
		*base = clk_nonpm->base;
	else if (of_property_read_bool(node, "PM") && clk_pm)
		*base = clk_pm->base;
	else
		*base = NULL;
}

static void mux_import_parents_info(struct device_node *node, struct mtk_composite *mg)
{

	struct property *p;
	const char *parents;
	char **tmp;
	int i = 0;
	int ret;

	ret = of_property_count_strings(node, "parents");
	if (ret < 0)
		return;

	mg->num_parents = ret;
	mg->parent_names = kcalloc(mg->num_parents, sizeof(const char *), GFP_KERNEL);
	tmp = (char **)mg->parent_names;
	of_property_for_each_string(node, "parents", p, parents) {
		tmp[i] = (char *) parents;
		//printk ("\033[1;45m(%s) %s \033[m\n", node->name, tmp[i]);
		i++;
	}
}

static void __init of_mtk_dtv_mg(struct device_node *node)
{

	int i, ret, index;
	u32 size = 0;
	u32 val;
	struct device_node *child = NULL;
	struct clock_reg_save *store;
	struct mtk_composite *mg;
	struct clk_onecell_data *clk_data;
	void __iomem *base = NULL;
	u32 tmp[2];

	CLK_BOOT_INFO("[%s] : %s\n", __func__, node->name);

	get_gnl_base(node, &base);
	if (base == NULL)
		base = of_iomap(node, 0);

	if (base) {
		of_property_read_u32_array(node, "reg", tmp, 2);
		CLK_BOOT_DBG("[%s]%s reg base : 0x%x -> 0x%px\n",
				__func__, node->name, tmp[1], base);
	} else {
		pr_err("[%s]%s reg base error\n", __func__, node->name);
		return;
	}

	ret = of_property_read_u32(node, "num", &size);
	if (size > 0) {
		mg = kcalloc(size, sizeof(*mg), GFP_KERNEL);
	} else {
		pr_err("Bad clock number: %s\n", node->name);
		return;
	}

	clk_data = mtk_alloc_clk_data(size);

	for_each_child_of_node(node, child) {
		u32 _reg = 0, mux_width = 0;
		u32 _bit[2] = {0};
		ret = 0;
		index = 0;
		val = 0;

		if (!of_device_is_available(child)) {
			CLK_BOOT_INFO("%s disabled\n", child->name);
			continue;
		}

		ret = of_property_read_u32(child, "id", &index);
		if (index >= size || ret < 0) {
			pr_err("[%s]%s bad index:%d size:%d ret:%d\n",
					node->name, child->name, index, size, ret);
			continue;
		}
		if (mg[index].id) {
			pr_err("fatal error: duplicated ID[%d]%s %d\n",
						index, child->name, mg[index].id);
			continue;
		}
		mg[index].id = index;

		/* Import reg info :*/
		ret = of_property_read_u32(child, "ctrl_reg", &_reg);
		if (ret < 0) {
			CLK_BOOT_DBG("[%s]%s no reg info ret:%d\n",
					node->name, child->name, ret);
			mg[index].gate_shift = -1;
			mg[index].mux_shift = -1;
			of_property_read_string(child, "parents", &mg[index].parent);
			goto import_finish;
		}

		/* BIT is the control bit range */
		ret = of_property_read_u32_array(child, "BIT", _bit, 2);
		if (ret < 0) {
			pr_err("[%s]%s bad BIT ret:%d\n",
					node->name, child->name, ret);
			mg[index].gate_shift = -1;
			mg[index].mux_shift = -1;
			of_property_read_string(child, "parents", &mg[index].parent);
			goto import_finish;
		}

		/* mux_shift > 15 means it doesn't support mux control */
		ret = of_property_read_u32(child, "mux_shift", &val);
		if (ret < 0) {
			pr_err("[%s]%s bad mux_shift ret:%d\n",
					node->name, child->name, ret);
			/* if mux_shift import fails, suggest mux degration */
			val = 17;
		}

		/* get mux_width info */
		ret = of_property_read_u32(child, "mux_width", &mux_width);
		if (ret < 0) {
			pr_err("[%s]%s bad mux_width ret:%d\n",
				node->name, child->name, ret);
			/* if mux_width import fails, suggest mux degration */
			val = 17;
		}

		mux_import_parents_info(child, &mg[index]);
		if (mg[index].num_parents <= 0) {
			pr_err("[%s]%s bad parents\n", node->name, child->name);
			/* if parents import fails, suggest mux degration */
			val = 17;
		}

		/* setup mux clock */
		if (val > 15) {
			mg[index].mux_shift = -1;
			of_property_read_string(child, "parents", &mg[index].parent);
		} else {
			/* BIT is the control bit range */
			mg[index].mux_shift = val+_bit[0];
			mg[index].mux_reg = _reg;
			mg[index].mux_width = (signed char)mux_width;
		}

		/* setup gate clock: gate_shift > 15 means it doesn't support gate control */
		ret = of_property_read_u32(child, "gate_shift", &val);
		if (ret < 0) {
			pr_err("[%s]%s bad gate_shift ret:%d\n",
					node->name, child->name, ret);
			val = 17;
		}
		if (val > 15)
			mg[index].gate_shift = -1;
		else {
			mg[index].gate_shift = val+_bit[0];
			mg[index].gate_reg = _reg;
		}

import_finish:
		mg[index].flags = CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE;
		/* divider_shift = -1 since we don't support dynamic DIV here */
		mg[index].divider_shift = -1;
		/* Import name info : the node name */
		mg[index].name = child->name;


	}

	mtk_clk_register_composites(mg, size, base, &mt5896_clk_lock, clk_data);

	of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	for (i = 0; i < size; i++) {
		if ((mg[i].mux_width > 0) && !(IS_ERR_OR_NULL(clk_data->clks[i]))) {
			store = kzalloc(sizeof(*store), GFP_KERNEL);
			store->c_hw = __clk_get_hw(clk_data->clks[i]);

			list_add_tail(&store->node, &clock_reg_storage);
		}

		kfree(mg[i].parent_names);
	}

	kfree(mg);
}

static void __init of_mtk_dtv_gate_en(struct device_node *node)
{
	int ret, index;
	u32 size = 0, _reg = 0;
	u32 gate_shift;
	struct device_node *child = NULL;
	struct clk_onecell_data *clk_data;
	void __iomem *base = NULL;
	u32 tmp[2];
	const char *parent_name = NULL;
	u8 flags = 0;

	CLK_BOOT_INFO("[%s] : %s\n", __func__, node->name);

	get_gnl_base(node, &base);
	if (base == NULL)
		base = of_iomap(node, 0);

	if (base) {
		of_property_read_u32_array(node, "reg", tmp, 2);
		CLK_BOOT_DBG("[%s]%s reg base : 0x%x -> %px\n",
				__func__, node->name, tmp[1], base);
	} else {
		pr_err("[%s]%s reg base error\n", __func__, node->name);
		return;
	}

	ret = of_property_read_u32(node, "num", &size);
	if (size > 0) {
		clk_data = mtk_alloc_clk_data(size);
	} else {
		pr_err("Bad clock number: %s\n", node->name);
		return;
	}

	for_each_child_of_node(node, child) {
		_reg = 0;
		parent_name = NULL;
		ret = 0;
		flags = 0;

		if (!of_device_is_available(child)) {
			CLK_BOOT_INFO("%s disabled\n", child->name);
			continue;
		}

		ret = of_property_read_u32(child, "id", &index);
		if (index >= size || ret < 0) {
			pr_err("[%s]%s bad index:%d size:%d ret:%d\n",
					node->name, child->name, index, size, ret);
			continue;
		}
		if (!IS_ERR_OR_NULL(clk_data->clks[index])) {
			pr_err("fatal error: duplicated ID[%d]%s\n",
						index, child->name);
			continue;
		}

		ret = of_property_read_string(child, "parent", &parent_name);

		of_property_read_u32(child, "gate_shift", &gate_shift);
		if (ret < 0) {
			pr_err("[%s]%s bad gate_shift ret:%d\n",
					node->name, child->name, ret);
			continue;
		}
		/* bad clk gate bit setting */
		if (gate_shift > 15)
			continue;

		/* Import bank info :  */
		of_property_read_u32(child, "ctrl_reg", &_reg);
		if (ret < 0) {
			pr_err("[%s]%s bad ctrl_reg:%d\n",
					node->name, child->name, ret);
			continue;
		}

		/* Import HW characteristics */
		if (of_property_read_bool(child, "ignore-unused"))
			flags |= CLK_IGNORE_UNUSED;

		clk_data->clks[index] = clk_register_gate(NULL, child->name,
			parent_name, flags,
			base + _reg, (u8)gate_shift,
			0, &mt5896_clk_lock
		);
	}

	of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

}

/*
 * bank_type : CLK_PM, CLK_NONPM
 * reg_addr : clock address
 * start : clock start bit
 * end : clock end bit
 * ret : clock reg value or negtive if error
 */
int clock_read(enum merak_regbank_type bank_type, unsigned int reg_addr,
	unsigned int start, unsigned int end)
{
	struct dtv_clk_reg_access *target = NULL;
	u16 value, mask;
	unsigned long flags;
	const int bit_w_of_ckgen = 15;

	if (end < start || end > bit_w_of_ckgen)
		return -EINVAL;

	mask = (1<<(end+1)) - (1<<start);

	if (bank_type >= CLK_BANK_TYPE_MAX)
		return -EINVAL;

	target = bank_type == CLK_PM ? clk_pm : clk_nonpm;
	if (!target)
		return -ENODEV;

	if (reg_addr > target->size)
		return -EINVAL;

	spin_lock_irqsave(&mt5896_clk_lock, flags);

	value = clk_reg_r(target->base + reg_addr);

	spin_unlock_irqrestore(&mt5896_clk_lock, flags);

	CLK_GNL_RW_INFO("0x%x, mask:%x value: %x\n", reg_addr, mask, value);

	return (value & mask) >> start;
}
EXPORT_SYMBOL(clock_read);

/*
 * bank_type : CLK_PM, CLK_NONPM
 * reg_addr : clock address
 * value : write value
 * start : clock start bit
 * end : clock end bit
 * ret : clock write value or negtive if error
 */
int clock_write(enum merak_regbank_type bank_type, unsigned int reg_addr, u16 value,
		unsigned int start, unsigned int end)
{
	struct dtv_clk_reg_access *target = NULL;
	unsigned long flags;
	u16 tmp, mask, offset;
	u16 _value;
	const int bit_w_of_ckgen = 15;

	if (end < start || end > bit_w_of_ckgen)
		return -EINVAL;

	mask = (1<<(end+1)) - (1<<start);

	if (bank_type >= CLK_BANK_TYPE_MAX)
		return -EINVAL;

	target = bank_type == CLK_PM ? clk_pm:clk_nonpm;
	if (!target)
		return -ENODEV;

	offset = reg_addr;
	if (offset > target->size)
		return -EINVAL;

	_value = (value<<start) & mask;
	spin_lock_irqsave(&mt5896_clk_lock, flags);

	tmp = clk_reg_r(target->base + offset);

	clk_reg_w(_value | (tmp & ~mask), target->base + offset);

	spin_unlock_irqrestore(&mt5896_clk_lock, flags);

	CLK_GNL_RW_INFO("%pS: mask:%x value: %x reg_addr: %x\n",
			__builtin_return_address(0), mask, _value, reg_addr);

	return _value | (tmp & ~mask);
}
EXPORT_SYMBOL(clock_write);

static void __init of_mtk_dtv_clk_general(struct device_node *node)
{
	int i;
	int size;
	struct resource r;
	const int reg_unit_size = 4;

	CLK_BOOT_INFO("[%s] : %s\n", __func__, node->name);

	size = of_property_count_elems_of_size(node, "reg", sizeof(u32)*reg_unit_size);
	if (size < 0)
		return;

	for (i = 0; i < size; i++) {
		if (of_address_to_resource(node, i, &r))
			continue;

		pr_warn("we have : %s\n", r.name);
		if (!strcmp(r.name, "PM") && !clk_pm) {
			clk_pm = kmalloc(sizeof(*clk_pm), GFP_KERNEL);
			if (!clk_pm)
				continue;

			clk_pm->base = of_iomap(node, i);
			if (!clk_pm->base) {
				kfree(clk_pm);
				continue;
			}
			clk_pm->bank_type = CLK_PM;
			clk_pm->size = resource_size(&r);

		} else if (!strcmp(r.name, "NONPM") && !clk_nonpm) {
			clk_nonpm = kmalloc(sizeof(*clk_nonpm), GFP_KERNEL);
			if (!clk_nonpm)
				continue;

			clk_nonpm->base = of_iomap(node, i);
			if (!clk_nonpm->base) {
				kfree(clk_nonpm);
				continue;
			}
			clk_nonpm->bank_type = CLK_NONPM;
			clk_nonpm->size = resource_size(&r);
		} else
			continue;

	}

	return;

}

static int clk_mt5896_suspend(void)
{
	pr_warn("(%s)\n", __func__);

	return 0;
}

static void clk_mt5896_resume(void)
{
	u16 val, reg;
	struct clock_reg_save *store;
	struct clk_hw *p_from_reg;
	struct clk_composite *composite;
	struct clk_mux *mux;
	unsigned long flags = 0;

	pr_warn("(%s)\n", __func__);

	list_for_each_entry(store, &clock_reg_storage, node) {

		composite = to_clk_composite(store->c_hw);
		mux = to_clk_mux(composite->mux_hw);

		spin_lock_irqsave(&mt5896_clk_lock, flags);
		reg = clk_reg_r(mux->reg) >> mux->shift;
		spin_unlock_irqrestore(&mt5896_clk_lock, flags);
		val = reg & (mux->mask);

		p_from_reg = clk_hw_get_parent_by_index(store->c_hw, val);

		if (!IS_ERR_OR_NULL(p_from_reg))
			clk_hw_set_parent(store->c_hw, p_from_reg);
		else {
			pr_err("%s resume sync fail:%p 0x%x 0x%x\n",
					clk_hw_get_name(store->c_hw), p_from_reg, val, reg);
			continue;
		}

		spin_lock_irqsave(&mt5896_clk_lock, flags);
		reg = clk_reg_r(mux->reg) & (mux->mask);
		spin_unlock_irqrestore(&mt5896_clk_lock, flags);

		CLK_STR_INFO("[%s] addr:%p, 0x%x, mask:0x%x, %d\n", clk_hw_get_name(store->c_hw),
				mux->reg, reg, mux->mask, mux->shift);

	}

}

static struct syscore_ops mt5896_clk_syscore_ops = {
	.suspend	= clk_mt5896_suspend,
	.resume         = clk_mt5896_resume,
};

static int __init mt5896_clk_pm_init_ops(void)
{

	CLK_BOOT_INFO("(%s)\n", __func__);
	register_syscore_ops(&mt5896_clk_syscore_ops);
	return 0;
}

#ifdef MODULE
static const struct of_device_id mt5896_clk_dev[] = {
	{.compatible = "mediatek,dtv_clk_fix",
	 .data = of_mtk_dtv_fix },
	{.compatible = "mediatek,dtv_clk_fix_factor",
	 .data = of_mtk_dtv_fix_factor },
	{.compatible = "mediatek,dtv_clk_mux_gate",
	 .data = of_mtk_dtv_mg },
	{.compatible = "mediatek,dtv_clk_gate_en",
	 .data = of_mtk_dtv_gate_en },
	{.compatible = "mediatek,mtk_dtv_clk_general",
	 .data = of_mtk_dtv_clk_general },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mt5896_clk_dev);

static int mt5896_clk_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	void (*dtv_clk_init_func)(struct device_node *np);

	dev_warn(dev, "probe ...\n");

	dtv_clk_init_func = of_device_get_match_data(dev);
	if (!dtv_clk_init_func)
		return -ENODEV;

	dtv_clk_init_func(np);

	return 0;
}

static struct platform_driver mt5896_clk_drv = {
	.probe  = mt5896_clk_probe,
	.driver = {
		.name = "mt5896_clk_driver",
		.of_match_table = of_match_ptr(mt5896_clk_dev),
	},
};

static int __init mt5896_clk_init(void)
{
	int err;

	pr_warn("(%s)\n", __func__);
	err = platform_driver_register(&mt5896_clk_drv);

	pr_warn("(%s) pm init\n", __func__);
	mt5896_clk_pm_init_ops();

	return err;
}

module_init(mt5896_clk_init);
MODULE_DESCRIPTION("Driver for mt5896");
MODULE_LICENSE("GPL v2");
#else /*ifdef MODULE*/
CLK_OF_DECLARE(mtk_dtv_fix, "mediatek,dtv_clk_fix", of_mtk_dtv_fix);
CLK_OF_DECLARE(mtk_topgen_fix_factor, "mediatek,dtv_clk_fix_factor",
			of_mtk_dtv_fix_factor);
CLK_OF_DECLARE(mtk_topgen_mg, "mediatek,dtv_clk_mux_gate", of_mtk_dtv_mg);
CLK_OF_DECLARE(mtk_dtv_gate_en, "mediatek,dtv_clk_gate_en",
			of_mtk_dtv_gate_en);
CLK_OF_DECLARE(mtk_clk_general, "mediatek,mtk_dtv_clk_general",
			of_mtk_dtv_clk_general);

device_initcall(mt5896_clk_pm_init_ops);
#endif /*ifdef MODULE*/

