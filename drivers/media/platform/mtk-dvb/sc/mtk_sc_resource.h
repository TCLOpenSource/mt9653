/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MDRV_SC_RESOURCE_H
#define MDRV_SC_RESOURCE_H

#include <linux/types.h>

struct mdrv_sc_regmaps {
	void __iomem *riu_vaddr;
	void __iomem *ext_riu_vaddr;
};

struct mdrv_sc_res {
	struct mdrv_sc_regmaps *regmaps;
};

int mdrv_sc_res_init(struct platform_device *pdev, struct mdrv_sc_res *pres);
int mdrv_sc_vcc_init(struct platform_device *pdev, int vcc_high_active);

#endif				/* MDRV_SC_RESOURCE_H */
