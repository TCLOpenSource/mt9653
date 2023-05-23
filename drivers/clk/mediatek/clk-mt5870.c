// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/slab.h>

#include "clk-mtk.h"
#include "clk-gate.h"
#include "clk-dtv.h"

static DEFINE_SPINLOCK(mt5870_clk_lock);
static LIST_HEAD(clock_reg_storage);

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

	pr_crit("[%s] : %s\n", __func__, node->name);

	ret = of_property_read_u32(node, "num", &size);

	if (size > 0)
		fixed_clks = kcalloc(size, sizeof(*fixed_clks), GFP_KERNEL);
	else
		return;

	clk_data = mtk_alloc_clk_data(size);

	for_each_child_of_node(node, child) {
		char *name;

		of_property_read_u32(child, "id", &index);
		if (index >= size) {
			pr_err("[%s]%s index error: %d, size: %d\n", __func__,
					node->name, index, size);
			continue;
		}
		/* Import clk freq */
		of_property_read_u32(child, "clock-frequency", &freq);
		if (freq > MAX_FREQ_INPUT) {
			pr_err("[%s]%s wrong freq input:%u\n", __func__, node->name, freq);
			continue;
		}
		fixed_clks[index].rate = freq*MHZ;
		/* Import clk name */
		fixed_clks[index].name = child->name;
		fixed_clks[index].id = index;
		pr_warn("[%d] %u %s\n", index,
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

	pr_crit("[%s] : %s\n", __func__, node->name);

	ret = of_property_read_u32(node, "num", &size);

	if (size > 0)
		fixed_divs = kcalloc(size, sizeof(*fixed_divs), GFP_KERNEL);
	else
		return;

	clk_data = mtk_alloc_clk_data(size);

	for_each_child_of_node(node, child) {
		for_each_child_of_node(child, gChild) {
			char *name, *parent_name;
			u32 div_factor[2] = {0};

			of_property_read_u32(gChild, "id", &index);
			if (index >= size) {
				pr_err("[%s]%s index error: %d, size: %d\n", __func__,
						node->name, index, size);
				continue;
			}
			fixed_divs[index].id = index;

			/* Import clk and parent name : the sub-node and node name */
			fixed_divs[index].name = gChild->name;
			fixed_divs[index].parent_name = child->name;

			/* Import DIV factor info */
			ret = of_property_read_u32_array(gChild, "div_factor", div_factor, 2);
			fixed_divs[index].mult = div_factor[0];
			fixed_divs[index].div = div_factor[1];

			pr_info("[%d] %s , p=%s mult:%u, div:%u\n",
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

static void mux_import_parents_info(struct device_node *node, struct mtk_composite *mg)
{

	struct property *p;
	const char *parents;
	char **tmp;
	int i = 0;

	mg->num_parents = of_property_count_strings(node, "parents");
	mg->parent_names = kcalloc(mg->num_parents, sizeof(const char *), GFP_KERNEL);
	tmp = (char **)mg->parent_names;
	of_property_for_each_string(node, "parents", p, parents) {
		tmp[i] = (char *) parents;
		//pr_info("\033[1;45m(%s) %s \033[m\n", node->name, tmp[i]);
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
	void __iomem *clk_pm_base;
	u32 tmp[2];

	pr_crit("[%s] : %s\n", __func__, node->name);

	clk_pm_base = of_iomap(node, 0);
	if (clk_pm_base) {
		of_property_read_u32_array(node, "reg", tmp, 2);
		pr_info("[%s]%s reg base : 0x%x -> 0x%lx\n",
				__func__, node->name, tmp[1], clk_pm_base);
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

		char *name;
		u32 _reg[2];
		u32 _bit[2];

		of_property_read_u32(child, "id", &index);
		if (index >= size) {
			pr_err("[%s]%s index error: %d, size: %d\n", __func__,
					node->name, index, size);
			continue;
		}
		mg[index].id = index;

		/* Import name info : the node name */
		mg[index].name = child->name;

		/* Import bank info :  */
		of_property_read_u32_array(child, "reg_bank_offset", _reg, 2);
		mg[index].mux_reg = mg[index].gate_reg = _reg[0]*2 + _reg[1]*4;

		/* BIT is the control bit range */
		of_property_read_u32_array(child, "BIT", _bit, 2);

		/* mux_shift > 15 means it doesn't support mux control */
		of_property_read_u32(child, "mux_shift", &val);
		if (val > 15) {
			mg[index].mux_shift = -1;
			of_property_read_string(child, "parents", &mg[index].parent);
		} else {
			mg[index].mux_shift = val+_bit[0];
			of_property_read_u32(child, "mux_width", &val);
			mg[index].mux_width = val;
			mux_import_parents_info(child, &mg[index]);
		}

		/* gate_shift > 15 means it doesn't support gate control */
		of_property_read_u32(child, "gate_shift", &val);
		if (val > 15)
			mg[index].gate_shift = -1;
		else
			mg[index].gate_shift = val+_bit[0];

		/* divider_shift = -1 since we don't support dynamic DIV here */
		mg[index].divider_shift = -1;

		mg[index].flags = CLK_SET_RATE_PARENT;

		if (strstr(node->name, "topgen")) {
			store = kzalloc(sizeof(*store), GFP_KERNEL);
			store->info = kzalloc(sizeof(*store->info), GFP_KERNEL);
			store->id = index;
			store->regOffset = clk_pm_base + mg[index-1].gate_reg;
			store->info->mask = (u16)((1<<(_bit[1]+1))-(1<<(_bit[0])));

			list_add_tail(&store->node, &clock_reg_storage);
		}
	}

	/* TODO: tmp solution on T32, with new scheme on M6 corresponding to the new HW */
	if (strstr(node->name, "topgen")) {
		/* The latest clk (i=mg_num) is the test clk and no need to take care of it */
		for (i = 0; i < size-1; i++) {
			/* Sync gate setting status from bootloader */
			if (mg[i].id &&
			!(readw(clk_pm_base + mg[i].gate_reg)&(1<<mg[i].gate_shift))) {
				mg[i].flags |= CLK_IS_CRITICAL;
			}
		}
	}

	mtk_clk_register_composites(mg, size, clk_pm_base,
		&mt5870_clk_lock, clk_data);

	of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	for (i = 0; i < size; i++)
		kfree(mg[i].parent_names);

	kfree(mg);

}

static void __init of_mtk_dtv_gate_en(struct device_node *node)
{
	int i, ret, index;
	u32 size = 0;
	u32 gate_shift;
	struct device_node *child = NULL;
	struct clk_onecell_data *clk_data;
	void __iomem *clk_pm_base;
	u32 tmp[2];

	pr_crit("[%s] : %s\n", __func__, node->name);

	clk_pm_base = of_iomap(node, 0);
	if (clk_pm_base) {
		of_property_read_u32_array(node, "reg", tmp, 2);
		pr_info("[%s]%s reg base : 0x%x -> 0x%lx\n",
				__func__, node->name, tmp[1], clk_pm_base);
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
		u32 _reg[2];
		const char *parent_name = NULL;

		of_property_read_u32(child, "id", &index);
		if (index >= size) {
			pr_err("[%s]%s index error: %d, size: %d\n", __func__,
					node->name, index, size);
			continue;
		}
		of_property_read_string(child, "parent", &parent_name);
		of_property_read_u32(child, "gate_shift", &gate_shift);
		/* bad clk gate bit setting */
		if (gate_shift > 15)
			continue;

		/* Import bank info :  */
		of_property_read_u32_array(child, "reg_bank_offset", _reg, 2);

		clk_data->clks[index] = clk_register_gate(NULL, child->name,
			parent_name, 0,
			clk_pm_base + _reg[0]*2 + _reg[1]*4, (u8)gate_shift,
			NULL, &mt5870_clk_lock
		);
	}

	of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

}

static int clk_mt5870_suspend(void)
{

	struct clock_reg_save *store;

	pr_warn("(%s)\n", __func__);
	list_for_each_entry(store, &clock_reg_storage, node) {
		pr_info("[%d] 0x%x 0x%x\n", store->id,
			clk_reg_r(store->regOffset), store->info->mask);
		store->info->value = clk_reg_r(store->regOffset);
	}

	return 0;
}
static void clk_mt5870_resume(void)
{
	u16 store_value, tmp;
	struct clock_reg_save *store;

	pr_warn("(%s)\n", __func__);
	list_for_each_entry(store, &clock_reg_storage, node) {
		store_value = store->info->value & store->info->mask;// store info
		tmp = clk_reg_r(store->regOffset) & ~(store->info->mask);
		pr_info("[%d] 0x%x 0x%x mask:0x%x\n", store->id,
				store_value, tmp, store->info->mask);
		clk_reg_w(store_value|tmp, store->regOffset);
	}
}

static struct syscore_ops mt5870_clk_syscore_ops = {
	.suspend	= clk_mt5870_suspend,
	.resume         = clk_mt5870_resume,
};

static int __init mt5870_clk_pm_init_ops(void)
{
	register_syscore_ops(&mt5870_clk_syscore_ops);
	return 0;
}

#ifdef MODULE
static const struct of_device_id mt5870_clk_dev[] = {
	{.compatible = "mediatek,dtv_clk_fix",
	 .data = of_mtk_dtv_fix },
	{.compatible = "mediatek,dtv_clk_fix_factor",
	 .data = of_mtk_dtv_fix_factor },
	{.compatible = "mediatek,dtv_clk_mux_gate",
	 .data = of_mtk_dtv_mg },
	{.compatible = "mediatek,mtk_dtv_clk_gate_en",
	 .data = of_mtk_dtv_gate_en },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mt5870_clk_dev);

static int mt5870_clk_probe(struct platform_device *pdev)
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

static struct platform_driver mt5870_clk_drv = {
	.probe  = mt5870_clk_probe,
	.driver = {
		.name = "mt5870_clk_driver",
		.of_match_table = of_match_ptr(mt5870_clk_dev),
	},
};

static int __init mt5870_clk_init(void)
{
	int err;

	pr_warn("(%s)\n", __func__);
	err = platform_driver_register(&mt5870_clk_drv);

	pr_warn("(%s) pm init\n");
	mt5870_clk_pm_init_ops();

	return err;
}

module_init(mt5870_clk_init);
MODULE_DESCRIPTION("Driver for mt5870");
MODULE_LICENSE("GPL v2");
#else /*ifdef MODULE*/
CLK_OF_DECLARE(mtk_dtv_fix, "mediatek,dtv_clk_fix", of_mtk_dtv_fix);
CLK_OF_DECLARE(mtk_topgen_fix_factor, "mediatek,dtv_clk_fix_factor",
			of_mtk_dtv_fix_factor);
CLK_OF_DECLARE(mtk_topgen_mg, "mediatek,dtv_clk_mux_gate", of_mtk_dtv_mg);
CLK_OF_DECLARE(mtk_dtv_gate_en, "mediatek,mtk_dtv_clk_gate_en",
			of_mtk_dtv_gate_en);
device_initcall(mt5870_clk_pm_init_ops);
#endif /*ifdef MODULE*/
