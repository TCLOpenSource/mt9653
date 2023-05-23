/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MDRV_DEMUX_TSP_H
#define MDRV_DEMUX_TSP_H

//#include "mdrv_demux_ctrl.h"
//#include "mdrv_demux_filter.h"
//#include "mdrv_demux_tsio.h"
//#include "mdrv_demux_pvr.h"

struct mdrv_ci_int {
	int irq;
};

int mdrv_ci_init(struct platform_device *pdev, struct mdrv_ci_int *pinterrupt);
//int mdrv_demux_tsp_exit(struct platform_device *pdev, struct mdrv_demux_tsp *ptsp);

#endif /* MDRV_DEMUX_TSP_H */
