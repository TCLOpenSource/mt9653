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
 * this is the remote control that comes with the gome smart tv
 * which based on STAOS standard.
 */

static struct key_map_table gome_tv[] = {
#if 0
    //yuanerlei 2017-7-21 start sanyo
    { 0x1C, KEY_POWER },
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
    { 0x49, KEY_RED },
    { 0x4A, KEY_GREEN },
    { 0x4B, KEY_YELLOW },
    { 0x4C, KEY_BLUE },
    { 0x36, KEY_UP },
    { 0x37, KEY_DOWN },
    { 0x39, KEY_LEFT },
    { 0x38, KEY_RIGHT },
    { 0x1F, KEY_ENTER },
    { 0x0C, KEY_CHANNELUP },
    { 0x0D, KEY_CHANNELDOWN },
    { 0x16, KEY_VOLUMEUP },
    { 0x17, KEY_VOLUMEDOWN },
    { 0x03, KEY_PAGEUP },
    { 0x05, KEY_PAGEDOWN },
    { 0x4D, KEY_HOME},
    { 0x1A, KEY_MENU },
    { 0xEA, KEY_BACK },
    { 0x15, KEY_MUTE },
    { 0x18, KEY_INFO },
    { 0x14, KEY_KP1 },        // TV_INPUT
    { 0x44, KEY_AUDIO },      // (C)SOUND_MODE
    { 0x56, KEY_CAMERA },     // (C)PICTURE_MODE
    { 0x4C, KEY_ZOOM },       // (C)ASPECT_RATIO
    { 0x5C, KEY_CHANNEL },    // (C)CHANNEL_RETURN
    { 0x45, KEY_SLEEP },      // (C)SLEEP
    { 0x0B, KEY_EPG },        // (C)EPG
    { 0x6A, KEY_SETUP},
   //yuanerlei 2017-7-21 end sanyo
#endif

#if 0
   //yuanerlei 2017-9-12 start guomei
    { 0x10, KEY_POWER },
    { 0x5A, KEY_UP },
    { 0x5B, KEY_DOWN },
    { 0x5D, KEY_LEFT },
    { 0x5E, KEY_RIGHT },
    { 0x5C, KEY_ENTER },
    { 0x1C, KEY_VOLUMEUP },
    { 0x1D, KEY_VOLUMEDOWN },
    { 0x17, KEY_HOME},
    { 0x0C, KEY_MENU},
    { 0x0B, KEY_BACK},
    { 0x20, KEY_KP1},     // TV_INPUT
    { 0x45, KEY_SETUP},
    { 0x05, KEY_MIR},
    { 0x3A, KEY_SOUND},
    { 0x18, KEY_GHOME},
    { 0xE0, KEY_AUTO_PAIRING},
    { 0x2D, KEY_FAC_P},//P
    { 0x3F, KEY_FAC_P},//P
    { 0x15, KEY_FAC_PW},//PW,INFO
    { 0x14, KEY_FAC_PW},//PW,INFO
    { 0x1D, KEY_FAC_WIFI},//WIFI
    { 0x2E, KEY_FAC_OOB},//OOB,SLEEP
    //yuanerlei 2017-9-12 end guomei
    //add by hl for ir
    //yuanerlei 2018-1-2 add start wangnengyaokongqi
     { 0x08, KEY_KP1},        // TV_INPUT
     { 0x00, KEY_POWER },
     { 0x04, KEY_LEFT},
     { 0x05, KEY_RIGHT},
     { 0x02, KEY_UP},
     { 0x03, KEY_DOWN},
     { 0x06, KEY_SELECT},
     { 0x07, KEY_MENU},
     { 0x05, KEY_VOLUMEUP },
     { 0x04, KEY_VOLUMEDOWN },
     { 0x02, KEY_CHANNELUP},
     { 0x03, KEY_CHANNELDOWN},
     { 0x01, KEY_MUTE},
    //yuanerlei 2018-1-2 add end
#endif
#if 0
    //MAWAN 2017-11-09 start tcl factory IR
    { 0x12, KEY_POWER },
    { 0x17, KEY_MUTE},
    { 0x16, KEY_MUTE},
    { 0x01, KEY_FAC_AV1},//AV1
    { 0x02, KEY_FAC_AV2},//AV2
    { 0x08, KEY_FAC_HDMI1},//HDMI1
    { 0x09, KEY_FAC_HDMI2},//HDMI2
    { 0x0A, KEY_FAC_HDMI3},//HDMI3
    { 0x0C, KEY_USB},
    { 0x1D, KEY_FAC_WIFI},//WIFI
    { 0x14, KEY_FAC_SN},//SN
    { 0x15, KEY_FAC_SN},//SN
    { 0x14, KEY_KP1},  // TV_INPUT
    { 0x52, KEY_BACK},
    { 0x2E, KEY_FAC_OOB},//OOB,SLEEP
    { 0x48, KEY_HOME},
    { 0x19, KEY_UP },
    { 0x1D, KEY_DOWN },
    { 0x42, KEY_LEFT },
    { 0x40, KEY_RIGHT },
    { 0x21, KEY_ENTER },
    { 0x5B, KEY_SETUP},
    { 0x43, KEY_BACK},
    { 0x1A, KEY_VOLUMEUP },
    { 0x1E, KEY_VOLUMEDOWN },
    { 0x1B, KEY_CHANNELUP },
    { 0x1F, KEY_CHANNELDOWN },
    { 0x2D, KEY_FAC_P},//P
    { 0x3F, KEY_FAC_P},//P
    { 0x15, KEY_FAC_PW},//PW,INFO
    { 0x14, KEY_FAC_PW},//PW,INFO
    { 0x0A, KEY_FAC_TV},//TV
    { 0x0D, KEY_FAC_CABLE},//CABLE
    { 0x12, KEY_FAC_KEYPADTEST},//KEYPADETEST
    { 0x13, KEY_FAC_KEYPADTEST},//KEYPADEYEST
    { 0x28, KEY_FAC_CTEMP},//CTEMPT
    { 0x3E, KEY_FAC_3D},//CTEMPT
    { 0x3D, KEY_FAC_3D},//CTEMPT
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
    { 0x4B, KEY_MENU},
    //MAWAN 2017-11-09 END tcl factory IR

    // 2nd IR controller.
    //mstar ir
    { 0x807F46, KEY_POWER },
    { 0x807F50, KEY_0 },
    { 0x807F49, KEY_1 },
    { 0x807F55, KEY_2 },
    { 0x807F59, KEY_3 },
    { 0x807F4D, KEY_4 },
    { 0x807F51, KEY_5 },
    { 0x807F5D, KEY_6 },
    { 0x807F48, KEY_7 },
    { 0x807F54, KEY_8 },
    { 0x807F58, KEY_9 },
    { 0x807F47, KEY_RED },
    { 0x807F4B, KEY_GREEN },
    { 0x807F57, KEY_YELLOW },
    { 0x807F5B, KEY_BLUE },
    { 0x807F52, KEY_UP },
    { 0x807F13, KEY_DOWN },
    { 0x807F06, KEY_LEFT },
    { 0x807F1A, KEY_RIGHT },
    { 0x807F0F, KEY_ENTER },
    { 0x807F1F, KEY_CHANNELUP },
    { 0x807F19, KEY_CHANNELDOWN },
    { 0x807F16, KEY_VOLUMEUP },
    { 0x807F15, KEY_VOLUMEDOWN },
    { 0x807F03, KEY_PAGEUP },
    { 0x807F05, KEY_PAGEDOWN },
    { 0x807F17, KEY_HOME},
    { 0x807F07, KEY_MENU },
    { 0x807F1B, KEY_BACK },
    { 0x807F5A, KEY_MUTE },
    { 0x807F0D, KEY_RECORD },     // DVR
    { 0x807F42, KEY_HELP },       // GUIDE
    { 0x807F14, KEY_INFO },
    { 0x807F40, KEY_KP0 },        // WINDOW
    { 0x807F04, KEY_KP1 },        // TV_INPUT
    { 0x807F0E, KEY_REWIND },
    { 0x807F12, KEY_FORWARD },
    { 0x807F02, KEY_PREVIOUSSONG },
    { 0x807F1E, KEY_NEXTSONG },
    { 0x807F01, KEY_PLAY },
    { 0x807F1D, KEY_PAUSE },
    { 0x807F11, KEY_STOP },
    { 0x807F44, KEY_AUDIO },      // (C)SOUND_MODE
    { 0x807F56, KEY_CAMERA },     // (C)PICTURE_MODE
    { 0x807F4C, KEY_ZOOM },       // (C)ASPECT_RATIO
    { 0x807F5C, KEY_CHANNEL },    // (C)CHANNEL_RETURN
    { 0x807F45, KEY_SLEEP },      // (C)SLEEP
    { 0x807F4A, KEY_EPG },        // (C)EPG
    { 0x807F10, KEY_LIST },       // (C)LIST
    { 0x807F53, KEY_SUBTITLE },   // (C)SUBTITLE
    { 0x807F41, KEY_FN_F1 },      // (C)MTS
    { 0x807F4E, KEY_FN_F2 },      // (C)FREEZE
    { 0x807F0A, KEY_FN_F3 },      // (C)TTX
    { 0x807F09, KEY_FN_F4 },      // (C)CC
    { 0x807F1C, KEY_FN_F5 },      // (C)TV_SETTING
    { 0x807F08, KEY_FN_F6 },      // (C)SCREENSHOT
    { 0x807F0B, KEY_F1 },         // MSTAR_BALANCE
    { 0x807F18, KEY_F2 },         // MSTAR_INDEX
    { 0x807F00, KEY_F3 },         // MSTAR_HOLD
    { 0x807F0C, KEY_F4 },         // MSTAR_UPDATE
    { 0x807F4F, KEY_F5 },         // MSTAR_REVEAL
    { 0x807F5E, KEY_F6 },         // MSTAR_SUBCODE
    { 0x807F43, KEY_F7 },         // MSTAR_SIZE
    { 0x807F5F, KEY_F8 },         // MSTAR_CLOCK
    { 0x807FFE, KEY_POWER2 },     // FAKE_POWER
    { 0x807FFF, KEY_OK },         // KEY_OK
#endif
    // 2nd IR controller.
};

static struct key_map_list gome_tv_map = {
    .map = {
        .scan      = gome_tv,
        .size      = ARRAY_SIZE(gome_tv),
        .name      = NAME_KEYMAP_GOME_TV,
        .headcode  = NUM_KEYMAP_GOME_TV0,
    }
};

static int init_key_map_gome_tv(void)
{
    IR_PRINT("[IR Log] %s\n", __func__);
    return MIRC_Map_Register(&gome_tv_map);
}
EXPORT_SYMBOL(init_key_map_gome_tv);

static void exit_key_map_gome_tv(void)
{
    MIRC_Map_UnRegister(&gome_tv_map);
}
EXPORT_SYMBOL(exit_key_map_gome_tv);
