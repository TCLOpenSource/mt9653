/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_PCMCIA_H
#define MTK_PCMCIA_H
#include <media/dvb_ca_en50221.h>
#include "mtk_pcmcia_resource.h"
#include "mtk_pcmcia_interrupt.h"
#include "mtk_pcmcia_clock.h"



#define DRV_NAME    "mediatek,ci"

struct mtk_dvb_ci {
struct spi_device *pdev;
struct mdrv_ci_res resource;
struct mdrv_ci_int interrupt;
struct mdrv_ci_clk clock;
struct dvb_ca_en50221 ca;
struct dvb_adapter *adapter;
struct gpio_desc *ci_power_en;
struct gpio_desc *ci_reset;
};

ssize_t mtk_pcmcia_to_spi_read(const char *txbuf, size_t txlen, char *buf, size_t len);
ssize_t mtk_pcmcia_to_spi_write(const char *buf, size_t n);
void mtk_pcmcia_set_spi(u32	max_speed_hz);
static int mtk_pcmcia_reset_companion_chip(struct gpio_desc *ci_reset);

#endif /* MTK_PCMCIA_H */
