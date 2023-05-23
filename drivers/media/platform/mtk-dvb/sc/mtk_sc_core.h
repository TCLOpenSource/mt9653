/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SC_H
#define MTK_SC_H
#include <media/dvb_ca_en50221.h>
#include "mtk_sc_resource.h"
#include "mtk_sc_interrupt.h"
#include "mtk_sc_clock.h"

#define DRV_NAME	"mediatek,sc"

struct mtk_dvb_sc {
	struct platform_device *pdev;
	struct mdrv_sc_res resource;
	struct mdrv_sc_int interrupt;
	struct mdrv_sc_clk clock;
	struct dvb_ca_en50221 ca;
	struct dvb_adapter *adapter;
};


#endif /* MTK_PCMCIA_H */
