/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MDRV_CI_RESOURCE_H
#define MDRV_CI_RESOURCE_H

#include <linux/types.h>

struct mdrv_ci_regmaps {
	void __iomem *riu_vaddr;
	void __iomem *ext_riu_vaddr;
};

struct mdrv_ci_res {
	struct mdrv_ci_regmaps *regmaps;
};

int  mdrv_ci_res_init(struct platform_device *pdev, struct mdrv_ci_res *pres);

#endif /* MDRV_CI_RESOURCE_H */
