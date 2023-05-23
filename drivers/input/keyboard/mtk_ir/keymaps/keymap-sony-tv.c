// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author jefferry.yen <jefferry.yen@mediatek.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

/*
 * Jimmy Hsu <jimmy.hsu@mediatek.com>
 * this is the remote control that comes with the mtk smart tv
 * which based on STAOS standard.
 */

// 5Addr(0x01)+7Cmd(Data) = 12bits
// Data mapping see the follow table
static struct key_map_table sony_tv1[] = {
	{ 0x15, KEY_POWER },
	{ 0x00, KEY_1 },
	{ 0x01, KEY_2 },
	{ 0x02, KEY_3 },
	{ 0x03, KEY_4 },
	{ 0x04, KEY_5 },
	{ 0x05, KEY_6 },
	{ 0x06, KEY_7 },
	{ 0x07, KEY_8 },
	{ 0x08, KEY_9 },
	{ 0x09, KEY_0 },
	#if 0
	{ 0x52, KEY_UP },
	{ 0x13, KEY_DOWN },
	{ 0x34, KEY_LEFT },
	{ 0x1A, KEY_RIGHT },
	{ 0x0F, KEY_ENTER },
	#endif
	{ 0x10, KEY_CHANNELUP },
	{ 0x11, KEY_CHANNELDOWN },
	{ 0x12, KEY_VOLUMEUP },
	{ 0x13, KEY_VOLUMEDOWN },
	{ 0x14, KEY_MUTE },
	//{ 0x17, KEY_HOME},
	//{ 0x07, KEY_MENU },
	//{ 0x1B, KEY_BACK },
	{ 0x17, KEY_AUDIO },	  // (C)SOUND_MODE
};

// 13Addr(0x081f)+7Cmd(Data) = 20bits
static struct key_map_table sony_tv2[] = {
	{ 0x08, KEY_F1 },	// youtube music
	{ 0x04, KEY_F2 },	// prime video
};

// 8Addr(0x97)+7Cmd(Data) = 15bits
static struct key_map_table sony_tv3[] = {
	{ 0x1D, KEY_DOT },	// DOT
	{ 0x23, KEY_BACK },	// BACK
};

// 8Addr(0xC4)+7Cmd(Data) = 15bits
static struct key_map_table sony_tv4[] = {
	{ 0x47, KEY_F3 },			// youtube
	{ 0x6F, KEY_F4 },			// MIC
	{ 0x2A, KEY_APPSELECT },	// apps
};

// 8Addr(0x1A)+7Cmd(Data) = 15bits
static struct key_map_table sony_tv5[] = {
	{ 0x7C, KEY_F5 }, // netflix
};

// 8Addr(0xA4)+7Cmd(Data) = 15bits
static struct key_map_table sony_tv6[] = {
	{ 0x5B, KEY_PROGRAM }, // GUIDE
};

static struct key_map_list sony_tv_map1 = {
	.map = {
		.scan	 = sony_tv1,
		.size	 = ARRAY_SIZE(sony_tv1),
		.name	 = NAME_KEYMAP_SONY_TV,
		.headcode	  = NUM_KEYMAP_SONY_TV1,
	}
};
static struct key_map_list sony_tv_map2 = {
	.map = {
		.scan	 = sony_tv2,
		.size	 = ARRAY_SIZE(sony_tv2),
		.name	 = NAME_KEYMAP_SONY_TV,
		.headcode	  = NUM_KEYMAP_SONY_TV2,
	}
};
static struct key_map_list sony_tv_map3 = {
	.map = {
		.scan	 = sony_tv3,
		.size	 = ARRAY_SIZE(sony_tv3),
		.name	 = NAME_KEYMAP_SONY_TV,
		.headcode	  = NUM_KEYMAP_SONY_TV3,
	}
};
static struct key_map_list sony_tv_map4 = {
	.map = {
		.scan	 = sony_tv4,
		.size	 = ARRAY_SIZE(sony_tv4),
		.name	 = NAME_KEYMAP_SONY_TV,
		.headcode	  = NUM_KEYMAP_SONY_TV4,
	}
};
static struct key_map_list sony_tv_map5 = {
	.map = {
		.scan	 = sony_tv5,
		.size	 = ARRAY_SIZE(sony_tv5),
		.name	 = NAME_KEYMAP_SONY_TV,
		.headcode	  = NUM_KEYMAP_SONY_TV5,
	}
};
static struct key_map_list sony_tv_map6 = {
	.map = {
		.scan	 = sony_tv6,
		.size	 = ARRAY_SIZE(sony_tv6),
		.name	 = NAME_KEYMAP_SONY_TV,
		.headcode	  = NUM_KEYMAP_SONY_TV6,
	}
};

int init_key_map_sony_tv(void)
{
	pr_emerg("[IR]init_key_map_sony_tv"); // IR_PRINT
	MIRC_Map_Register(&sony_tv_map1);
	MIRC_Map_Register(&sony_tv_map2);
	MIRC_Map_Register(&sony_tv_map3);
	MIRC_Map_Register(&sony_tv_map4);
	MIRC_Map_Register(&sony_tv_map5);
	MIRC_Map_Register(&sony_tv_map6);

	return 0;
}
EXPORT_SYMBOL(init_key_map_sony_tv);

void exit_key_map_sony_tv(void)
{
	MIRC_Map_UnRegister(&sony_tv_map1);
	MIRC_Map_UnRegister(&sony_tv_map2);
	MIRC_Map_UnRegister(&sony_tv_map3);
	MIRC_Map_UnRegister(&sony_tv_map4);
	MIRC_Map_UnRegister(&sony_tv_map5);
	MIRC_Map_UnRegister(&sony_tv_map6);
}
EXPORT_SYMBOL(exit_key_map_sony_tv);
