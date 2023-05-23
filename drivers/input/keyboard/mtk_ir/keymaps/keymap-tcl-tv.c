// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

/*
 * this is the remote control that comes with the tcl smart tv
 * which based on STAOS standard.
 */

static struct key_map_table tcl_tv[] = {
    // 1st IR controller.
    { 0xD5, KEY_POWER },
    { 0xCF, KEY_0 },
    { 0xCE, KEY_1 },
    { 0xCD, KEY_2 },
    { 0xCC, KEY_3 },
    { 0xCB, KEY_4 },
    { 0xCA, KEY_5 },
    { 0xC9, KEY_6 },
    { 0xC8, KEY_7 },
    { 0xC7, KEY_8 },
    { 0xC6, KEY_9 },
    { 0xFF, KEY_RED },
    { 0x17, KEY_GREEN },
    { 0x1B, KEY_YELLOW },
    { 0x27, KEY_BLUE },
    { 0xA6, KEY_UP },
    { 0xA7, KEY_DOWN },
    { 0xA9, KEY_LEFT },
    { 0xA8, KEY_RIGHT },
    { 0x0B, KEY_ENTER },
    { 0xD2, KEY_CHANNELUP },
    { 0xD3, KEY_CHANNELDOWN },
    { 0xD0, KEY_VOLUMEUP },
    { 0xD1, KEY_VOLUMEDOWN },
    { 0xF7, KEY_HOME },
    { 0xF9, KEY_BACK },
    { 0xC0, KEY_MUTE },
    { 0xC3, KEY_INFO },
    { 0x5C, KEY_KP0 },        // TV_INPUT
    { 0x67, KEY_KP1 },        // 3D_MODE
    { 0xA5, KEY_AUDIO },      // (C)SOUND_MODE
    { 0xED, KEY_CAMERA },     // (C)PICTURE_MODE
    { 0x6F, KEY_ZOOM },       // (C)ASPECT RATIO
    { 0xD8, KEY_CHANNEL },    // (C)CHANNEL_RETURN
    { 0xF5, KEY_EPG },        // (C)EPG
    { 0xF3, KEY_FN_F1 },      // (C)FREEZE
    { 0xFD, KEY_FN_F2 },      // (C)USB
    { 0x9E, KEY_F1 },         // TCL_MITV
    { 0x62, KEY_F2 },         // TCL_USB_MENU
    { 0x55, KEY_F3 },         // TCL_SWING_R1
    { 0x56, KEY_F4 },         // TCL_SWING_R2
    { 0x57, KEY_F5 },         // TCL_SWING_R3
    { 0x58, KEY_F6 },         // TCL_SWING_R4
    { 0xFA, KEY_F7 },         // TCL_SWING_L1
    { 0x34, KEY_F8 },         // TCL_SWING_L2
    { 0x18, KEY_F9 },         // TCL_SWING_L3
    { 0x87, KEY_F10 },        // TCL_SWING_L4

    // 2nd IR controller.
};

static struct key_map_list tcl_tv_map = {
	.map = {
		.scan    = tcl_tv,
		.size    = ARRAY_SIZE(tcl_tv),
		.name    = NAME_KEYMAP_TCL_TV,
		.headcode     = NUM_KEYMAP_TCL_TV,
                //.protocol     = (1<<IR_TYPE_NEC),
	}
};

int init_key_map_tcl_tv(void)
{
    IR_PRINT("[IR Log] %s\n", __func__);
	return MIRC_Map_Register(&tcl_tv_map);
}
EXPORT_SYMBOL(init_key_map_tcl_tv);

void exit_key_map_tcl_tv(void)
{
	MIRC_Map_UnRegister(&tcl_tv_map);
}
EXPORT_SYMBOL(exit_key_map_tcl_tv);
