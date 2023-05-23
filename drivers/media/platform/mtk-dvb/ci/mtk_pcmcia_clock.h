/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MDRV_CI_CLOCK_H
#define MDRV_CI_CLOCK_H

#include <linux/clk.h>
#include <linux/firmware.h>

struct mdrv_ci_clk {
struct clk *clk_pcm;
};

int mdrv_ci_clock_init(struct platform_device *pdev, struct mdrv_ci_clk *pclk);


#endif /* MDRV_CI_CLOCK_H */
