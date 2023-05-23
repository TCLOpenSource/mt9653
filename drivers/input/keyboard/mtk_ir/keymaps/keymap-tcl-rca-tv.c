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
 * this is the remote control that comes with the tcl rca smart tv
 * which based on STAOS standard.
 */

static struct key_map_table tcl_rca_tv[] = {
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
    { 0xA6, KEY_UP },
    { 0xA7, KEY_DOWN },
    { 0xA8, KEY_RIGHT },
    { 0xA9, KEY_LEFT },
    { 0x0B, KEY_ENTER },
    { 0xD2, KEY_CHANNELUP },
    { 0xD3, KEY_CHANNELDOWN },
    { 0xD0, KEY_VOLUMEUP },
    { 0xD1, KEY_VOLUMEDOWN },
    { 0xF7, KEY_HOME },
    { 0xD8, KEY_BACK },
    { 0xC0, KEY_MUTE },

    // 2nd IR controller.
};

static struct key_map_list tcl_tv_rca_map = {
    .map = {
        .scan    = tcl_rca_tv,
        .size    = ARRAY_SIZE(tcl_rca_tv),
        .name    = NAME_KEYMAP_TCL_RCA_TV,
        .headcode     = NUM_KEYMAP_TCL_RCA_TV,
    }
};

int init_key_map_tcl_rca_tv(void)
{
    IR_PRINT("[IR Log] %s\n", __func__);
    return MIRC_Map_Register(&tcl_tv_rca_map);
}
EXPORT_SYMBOL(init_key_map_tcl_rca_tv);

void exit_key_map_tcl_rca_tv(void)
{
    MIRC_Map_UnRegister(&tcl_tv_rca_map);
}
EXPORT_SYMBOL(exit_key_map_tcl_rca_tv);
