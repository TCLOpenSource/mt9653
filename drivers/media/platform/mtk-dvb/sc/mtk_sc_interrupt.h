/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef MDRV_SC_INTERRUPT_H
#define MDRV_SC_INTERRUPT_H

struct mdrv_sc_int {
	int irq;
};

int mdrv_sc_init(struct platform_device *pdev, struct mdrv_sc_int *pinterrupt);

#endif				/* MDRV_SC_INTERRUPT_H */
