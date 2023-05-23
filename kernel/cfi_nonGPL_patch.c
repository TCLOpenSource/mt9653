// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Joy Huang <joy-mi.huang@mediatek.com>
 *
 */
#include <linux/module.h>
#include <linux/printk.h>

void dummy_cfi_check(void *data, void *ptr, void *vtable)
{
}
EXPORT_SYMBOL(dummy_cfi_check);

MODULE_LICENSE("GPL v2");
