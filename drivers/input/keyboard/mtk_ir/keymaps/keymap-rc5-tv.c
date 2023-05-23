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
 * this is the remote control that comes with the rc5 smart tv
 * which based on STAOS standard.
 */

static struct key_map_table rc5_tv[] = {
    { 0x00080C, KEY_POWER },
    { 0x000800, KEY_0 },
    { 0x000801, KEY_1 },
    { 0x000802, KEY_2 },
    { 0x000803, KEY_3 },
    { 0x000804, KEY_4 },
    { 0x000805, KEY_5 },
    { 0x000806, KEY_6 },
    { 0x000807, KEY_7 },
    { 0x000808, KEY_8 },
    { 0x000809, KEY_9 },
    { 0x000837, KEY_RED },
    { 0x000836, KEY_GREEN },
    { 0x000832, KEY_YELLOW },
    { 0x000834, KEY_BLUE },
    { 0x000812, KEY_UP },
    { 0x000813, KEY_DOWN },
    { 0x000815, KEY_LEFT },
    { 0x000816, KEY_RIGHT },
    { 0x000814, KEY_ENTER },
    { 0x000820, KEY_CHANNELUP },
    { 0x000821, KEY_CHANNELDOWN },
    { 0x000810, KEY_VOLUMEUP },
    { 0x000811, KEY_VOLUMEDOWN },
    //{ 0x000803, KEY_PAGEUP },
    //{ 0x000805, KEY_PAGEDOWN },
    //{ 0x000817, KEY_HOME},
    { 0x000835, KEY_MENU },
    { 0x000833, KEY_BACK },
    { 0x00080d, KEY_MUTE },
    { 0x00080D, KEY_RECORD },     // DVR
    { 0x000842, KEY_HELP },       // GUIDE
    { 0x000814, KEY_INFO },
    { 0x000840, KEY_KP0 },        // WINDOW
    { 0x000804, KEY_KP1 },        // TV_INPUT
    { 0x00080E, KEY_REWIND },
    { 0x000812, KEY_FORWARD },
    { 0x000802, KEY_PREVIOUSSONG },
    { 0x00081E, KEY_NEXTSONG },
    { 0x00082e, KEY_PLAY },
    { 0x000830, KEY_PAUSE },
    { 0x00082f, KEY_STOP },
    { 0x000844, KEY_AUDIO },      // (C)SOUND_MODE
    { 0x000856, KEY_CAMERA },     // (C)PICTURE_MODE
    { 0x00084C, KEY_ZOOM },       // (C)ASPECT_RATIO
    { 0x00085C, KEY_CHANNEL },    // (C)CHANNEL_RETURN
    { 0x000845, KEY_SLEEP },      // (C)SLEEP
    { 0x00084A, KEY_EPG },        // (C)EPG
    { 0x000810, KEY_LIST },       // (C)LIST
    { 0x000853, KEY_SUBTITLE },   // (C)SUBTITLE
    { 0x000841, KEY_FN_F1 },      // (C)MTS
    { 0x00084E, KEY_FN_F2 },      // (C)FREEZE
    { 0x00080A, KEY_FN_F3 },      // (C)TTX
    { 0x000809, KEY_FN_F4 },      // (C)CC
    { 0x00081C, KEY_FN_F5 },      // (C)TV_SETTING
    { 0x000808, KEY_FN_F6 },      // (C)SCREENSHOT
    { 0x00080B, KEY_F1 },         // MSTAR_BALANCE
    { 0x000818, KEY_F2 },         // MSTAR_INDEX
    { 0x000800, KEY_F3 },         // MSTAR_HOLD
    { 0x00080C, KEY_F4 },         // MSTAR_UPDATE
    { 0x00084F, KEY_F5 },         // MSTAR_REVEAL
    { 0x00085E, KEY_F6 },         // MSTAR_SUBCODE
    { 0x000843, KEY_F7 },         // MSTAR_SIZE
    { 0x00085F, KEY_F8 },         // MSTAR_CLOCK
    { 0x0008FE, KEY_POWER2 },     // FAKE_POWER
    { 0x0008FF, KEY_OK },         // KEY_OK

    // 2nd IR controller.
};

static struct key_map_list mstar_rc5_map = {
    .map = {
        .scan    = rc5_tv,
        .size    = ARRAY_SIZE(rc5_tv),
        .name    = NAME_KEYMAP_RC5_TV,
        .headcode    = NUM_KEYMAP_RC5_TV,
        //.protocol     = (1<<IR_TYPE_RC5),
    }
};

int init_key_map_rc5_tv(void)
{
    IR_PRINT("[IR Log] %s\n", __func__);
    return MIRC_Map_Register(&mstar_rc5_map);
}
EXPORT_SYMBOL(init_key_map_rc5_tv);

void exit_key_map_rc5_tv(void)
{
    MIRC_Map_UnRegister(&mstar_rc5_map);
}
EXPORT_SYMBOL(exit_key_map_rc5_tv);
