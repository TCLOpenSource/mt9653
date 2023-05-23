/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _MTK_LIGHT_H
#define _MTK_LIGHT_H

struct mtk_light_range {
	int min;
	int max;
};

int mtk_light_get_lux_range(struct mtk_light_range *range);
int mtk_light_get_lux(int *lux);

#endif
