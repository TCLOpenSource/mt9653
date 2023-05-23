/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Owen Tseng <Owen.Tseng@mediatek.com>
 */

#ifndef __MTK_KEYPAD_H
#define __MTK_KEYPAD_H

#include <linux/input/matrix_keypad.h>


/**
 * struct mtk_keypad_platdata - Platform device data for Samsung Keypad.
 * @wakeup: controls whether the device should be set up as wakeup source.
 *
 * Initialisation data specific to either the machine or the platform
 * for the device driver to use or call-back when configuring gpio.
 */
struct mtk_keypad_platdata {
	bool wakeup;
	unsigned int *threshold;
	unsigned int *sarkeycode;
	unsigned int  chanel;
	unsigned int  low_bound;
	unsigned int  high_bound;
	unsigned int  key_num;
	unsigned int  debounce_time;
	bool no_autorepeat;
	unsigned int  reset_timeout;
	unsigned int  reset_key;
};

#endif /* __MTK_KEYPAD_H */
