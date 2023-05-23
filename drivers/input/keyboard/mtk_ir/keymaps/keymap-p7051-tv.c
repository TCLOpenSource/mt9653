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
 * this is the remote control that comes with the panasonic  smart tv
 * which based on panasonic  standard.
 */

static struct key_map_table p7051_stb[] = {
    { 0x5057, KEY_POWER },

    { 0x3030, KEY_0 },
    { 0x3031, KEY_1 },
    { 0x3032, KEY_2 },
    { 0x3033, KEY_3 },
    { 0x3034, KEY_4 },
    { 0x3035, KEY_5 },
    { 0x3036, KEY_6 },
    { 0x3037, KEY_7 },
    { 0x3038, KEY_8 },
    { 0x3039, KEY_9 },

    { 0x5550, KEY_UP },
    { 0x444E, KEY_DOWN },
    { 0x4C45, KEY_LEFT },
    { 0x5249, KEY_RIGHT },
    { 0x454E, KEY_ENTER },
    //{ 0x454E, KEY_OK },         // KEY_OK

    { 0x562B, KEY_VOLUMEUP },
    { 0x562D, KEY_VOLUMEDOWN },

    { 0x4D45, KEY_HOME},
    { 0x5250, KEY_MENU },
    { 0x5254, KEY_BACK },
    { 0x4633, KEY_KP1 },        // TV_INPUT
};

static struct key_map_list p7051_stb_map = {
    .map = {
        .scan    = p7051_stb,
        .size    = ARRAY_SIZE(p7051_stb),
        .name    = NAME_KEYMAP_P7051_STB,
        .headcode     = NUM_KEYMAP_P7051_STB,
        //.protocol     = (1<<IR_TYPE_P7051),
    }
};

int init_key_map_p7051_stb(void)
{
    IR_PRINT("[IR Log] %s\n", __func__);
    return MIRC_Map_Register(&p7051_stb_map);
}
EXPORT_SYMBOL(init_key_map_p7051_stb);

void exit_key_map_p7051_stb(void)
{
    MIRC_Map_UnRegister(&p7051_stb_map);
}
EXPORT_SYMBOL(exit_key_map_p7051_stb);
