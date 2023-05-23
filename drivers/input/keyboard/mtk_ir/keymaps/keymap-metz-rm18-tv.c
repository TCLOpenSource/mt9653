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
 * this is the remote control that comes with the metz rm18 smart tv
 * which based on STAOS standard.
 */

static struct key_map_table metz_rm18[] = {
    { 0x00, KEY_0 },
    { 0x01, KEY_1 },
    { 0x02, KEY_2 },
    { 0x03, KEY_3 },
    { 0x04, KEY_4 },
    { 0x05, KEY_5 },
    { 0x06, KEY_6 },
    { 0x07, KEY_7 },
    { 0x08, KEY_8 },
    { 0x09, KEY_9 },
    { 0x0A, KEY_POWER },
    { 0x11, KEY_BLUE },
    { 0x12, KEY_YELLOW },
    //{ 0x13, KEY_WHITE},
    { 0x14, KEY_GREEN },
    { 0x15, KEY_RED },
    { 0x16, KEY_UP },
    { 0x17, KEY_DOWN },
    { 0x18, KEY_LEFT },
    { 0x19, KEY_RIGHT },
    { 0x1A, KEY_VOLUMEUP },
    { 0x1B, KEY_VOLUMEDOWN },
    { 0x20, KEY_BACK },
    { 0x22, KEY_MUTE },
    { 0x24, KEY_EPG },        // (C)EPG
    { 0x26, KEY_OK },         // KEY_OK
    { 0x27, KEY_EXIT },
    { 0x28, KEY_REWIND },
    { 0x2A, KEY_PLAY },
    { 0x2B, KEY_STOP },
    { 0x2C, KEY_RECORD },     // DVR
    { 0x2D, KEY_PAGEUP },
    { 0x2E, KEY_PAGEDOWN },
    { 0x2F, KEY_HOME},
    // 2nd IR controller.
};

static struct key_map_list metz_rm18_map = {
    .map = {
        .scan    = metz_rm18,
        .size    = ARRAY_SIZE(metz_rm18),
        .name    = NAME_KEYMAP_METZ_RM18_TV,
        .headcode     = NUM_KEYMAP_METZ_RM18_TV,
        //.protocol     = (1<<IR_TYPE_METZ),
    }
};

int init_key_map_metz_rm18(void)
{
    IR_PRINT("[IR Log] %s\n", __func__);
    return MIRC_Map_Register(&metz_rm18_map);
}
EXPORT_SYMBOL(init_key_map_metz_rm18);

void exit_key_map_metz_rm18(void)
{
    MIRC_Map_UnRegister(&metz_rm18_map);
}
EXPORT_SYMBOL(exit_key_map_metz_rm18);
