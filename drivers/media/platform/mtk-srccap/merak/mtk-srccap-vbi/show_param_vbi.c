// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/mtk-v4l2-srccap.h>
#include "vbi-ex.h"

extern unsigned int dbglevel;
void show_v4l2_ext_vbi_cc_getinfo(__u32 u32selector, __u32 u32Result)
{
	SRCCAP_VBI_MSG_DEBUG("\t cc_info={"
					".u32selector=%d, .u32Result=%d}\n",
					u32selector,
					u32Result
				);
}

