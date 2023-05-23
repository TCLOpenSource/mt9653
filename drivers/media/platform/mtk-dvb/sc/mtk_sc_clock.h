/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MDRV_SC_CLOCK_H
#define MDRV_SC_CLOCK_H

#include <linux/clk.h>
#include <linux/firmware.h>

struct mdrv_sc_clk {
	struct clk *smart_ck;
	struct clk *smart_synth_432_ck;
	struct clk *smart_synth_27_ck;
	struct clk *smart2piu;
	struct clk *smart_synth_272piu;
	struct clk *smart_synth_4322piu;
	struct clk *mcu_nonpm2smart0;
};

int mdrv_sc_clock_init(struct platform_device *pdev, struct mdrv_sc_clk *pclk);


#endif /* MDRV_SC_CLOCK_H */
