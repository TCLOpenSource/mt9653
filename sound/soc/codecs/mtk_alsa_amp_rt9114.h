/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk_alsa_amp_rt9114.h  --  Mediatek DTV AMP header
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#ifndef _AMP_RT9114_HEADER
#define _AMP_RT9114_HEADER

#define MTK_AMP_RT9114_PARAM_COUNT	64

struct amp_rt9114_priv {
	struct device	*dev;
	struct i2c_client *client;
	struct snd_soc_component *component;
	struct gpio_desc *amp_mute;
	struct gpio_desc *amp_reset;
	unsigned int amp_mute_invertor;
	unsigned int amp_reset_invertor;
	struct regmap *regmap;
	bool suspended;
	int *param;
	const char *mtk_ini;
	u8 *init_table;
};

#endif /* _AMP_RT9114_HEADER */
