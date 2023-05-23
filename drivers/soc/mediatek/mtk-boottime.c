// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <linux/trace.h>

#define DUMMY_BASE	0x1c000000
#define TIMESTAMP_BANK  0x2009

#define NR_REG		2
#define DUMMY_OFFSET0	0x22
#define DUMMY_OFFSET1	0x24
#define DUMMY_OFFSET2	0x26
#define DUMMY_OFFSET3	0x28
#define DUMMY_OFFSET4	0x2a

#undef pr_fmt
#define pr_fmt(fmt) "BOOTTIME: " fmt

#define BANK_TO_ADDR(_bank) ((_bank) << 9)
#define timestamp_readl(_base, _offset) readl((_base) + ((_offset) << 2))

struct uboot_timestamp {
	char *desp;
	unsigned int offset;
};

static struct trace_array *instance;

int boottime_print(const char *s)
{
	if (!instance)
		return 0;

	return trace_array_printk(instance, 0, s);
}
EXPORT_SYMBOL(boottime_print);

static __init int _get_riu_base(phys_addr_t *base)
{
	int i, ret;
	u32 reg[NR_REG];
	struct device_node *np;

	np = of_find_node_by_name(NULL, "riu-base");
	if (!np) {
		pr_err("cannot find riu-base on fdt, use pre-defined as 0x%x\n", DUMMY_BASE);
		*base = DUMMY_BASE;
		return 0;
	}

	ret = of_property_read_u32_array(np, "reg", reg, ARRAY_SIZE(reg));
	if (ret) {
		pr_err("%pOF has no valid 'reg' property (%d)", np, ret);
		of_node_put(np);
		return -EINVAL;
	}

	of_node_put(np);

	for (i = 0; i < ARRAY_SIZE(reg); ++i) {
		if (reg[i])
			break;
	}

	if (i == ARRAY_SIZE(reg)) {
		pr_err("invalid riu base\n");
		return -ENOMEM;
	}

	*base = reg[i];

	return 0;
}

static __init int mtk_boottime_init(void)
{
	int i;
	int ret;
	phys_addr_t riu_base, bank_base;
	void __iomem *va;
	struct uboot_timestamp stamp[] = {
		// how about sboot ?
		{
			.desp	= "show logo start",
			.offset = DUMMY_OFFSET0,
		},
		{
			.desp	= "show logo end",
			.offset = DUMMY_OFFSET1,
		},
		{
			.desp	= "show logo start",
			.offset = DUMMY_OFFSET2,
		},
		{
			.desp	= "show logo end",
			.offset = DUMMY_OFFSET3,
		},
		{
			.desp	= "start kernel",
			.offset = DUMMY_OFFSET4,
		},
	};

	instance = trace_array_create("boottime");
	if (IS_ERR_OR_NULL(instance)) {
		pr_err("create boottime instance failed\n");
		return -ENODEV;
	}

	ret = trace_array_init_printk(instance);
	if (ret) {
		pr_err("init boottime instance failed with %d\n", ret);
		goto out;
	}

	pr_info("create boottime instance success\n");

	ret = _get_riu_base(&riu_base);
	if (ret)
		goto out;

	// just map timstamp bank
	bank_base = riu_base + BANK_TO_ADDR(TIMESTAMP_BANK);
	va = ioremap_wc(bank_base, SZ_1K);
	if (!va) {
		pr_err("ioremap %pa failed\n", &bank_base);
		ret = -ENOMEM;
		goto out;
	}

	pr_info("map pa:%pa+%x to va:0x%lx\n", &bank_base, SZ_1K,
			(unsigned long)va);

	for (i = 0; i < ARRAY_SIZE(stamp); ++i) {
		uint32_t tmp = timestamp_readl(va, stamp[i].offset);

		pr_debug("%s at [%d]\n", stamp[i].desp, tmp);
		trace_array_printk(instance, 0, "%s at [%d ms]\n",
				stamp[i].desp, tmp);
	}

	iounmap(va);

	return 0;
out:
	trace_array_destroy(instance);
	return ret;
}

static __exit void mtk_boottime_exit(void)
{
	trace_array_destroy(instance);
}

module_init(mtk_boottime_init);
module_exit(mtk_boottime_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MTK boottime record instance");
