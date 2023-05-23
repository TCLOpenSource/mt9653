// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/math64.h>
#include <linux/export.h>

unsigned long long __aeabi_uldivmod(unsigned long long n, unsigned long long d)
{
	return div64_u64(n, d);
}

long long __aeabi_ldivmod(long long n, long long d)
{
	return div64_s64(n, d);
}

