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
 * this is the remote control that comes with the hisense smart tv
 * which based on STAOS standard.
 */

#define HISENSE_USE_BOX 0

static struct key_map_table hisense_tv[] = {
    // 1st IR controller.
    { 0x0D, KEY_POWER },
    { 0x0E, KEY_MUTE },
    { 0x1A, KEY_SLEEP },
    { 0x11, KEY_AUDIO },      // SOUND_MODE
    { 0x10, KEY_CAMERA },     // PICTURE_MODE
    { 0x13, KEY_ZOOM },       // ASPECT_RATIO
    { 0x0B, KEY_CHANNEL },    // CHANNEL_RETURN
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
    { 0x52, KEY_RED },
    { 0x53, KEY_GREEN },
    { 0x54, KEY_YELLOW },
    { 0x55, KEY_BLUE },
    { 0x14, KEY_MENU },
    { 0x48, KEY_BACK },
    { 0x1D, KEY_EPG },
    { 0x16, KEY_UP },
    { 0x17, KEY_DOWN },
    { 0x18, KEY_RIGHT },
    { 0x19, KEY_LEFT },
    { 0x15, KEY_ENTER },
    { 0x4A, KEY_CHANNELUP },
    { 0x4B, KEY_CHANNELDOWN },
    { 0x44, KEY_VOLUMEUP },
    { 0x43, KEY_VOLUMEDOWN },
    { 0x5C, KEY_BACK },
    { 0x94, KEY_HOME },
    { 0x1F, KEY_SUBTITLE },
    { 0x5E, KEY_RECORD },     // DVR
//    { 0x0C, KEY_INFO },
    { 0x95, KEY_INFO },
    { 0x1B, KEY_TV },
    { 0x5B, KEY_SEARCH },
    { 0x1E, KEY_FAVORITES },
    { 0x57, KEY_REWIND },
    { 0x56, KEY_FORWARD },
    { 0x58, KEY_PREVIOUSSONG },
    { 0x59, KEY_NEXTSONG },
    { 0x5A, KEY_STOP },
    { 0x4E, KEY_PLAYPAUSE },
    { 0x4C, KEY_FN_F1 },      // MTS
    { 0x0F, KEY_FN_F2 },      // FREEZE
    { 0x12, KEY_FN_F6 },      // TV_INPUT
    { 0x5F, KEY_FN_F7 },      // 3D_MODE

    // Hisense extended
    { 0x1C, KEY_F1 },         // HISENSE_SAVEMODE
    { 0x49, KEY_F2 },         // HISENSE_AUDIO_TRACK
    { 0x5D, KEY_F3 },         // HISENSE_BROADCAST


    // 2nd IR cotroller.
    { 0x000F18, 530 },            // A
    { 0x000F19, 531 },
    { 0x000F1A, 532 },
    { 0x000F1B, 533 },
    { 0x000F1C, 534 },
    { 0x000F0A, 535 },
    { 0x000F2A, 536 },
    { 0x000F2B, 537 },
    { 0x000F2C, 538 },
    { 0x000F2D, 539 },
    { 0x000F2E, 540 },
    { 0x000F2F, 541 },
    { 0x000F31, 542 },
    { 0x000F32, 543 },
    { 0x000F33, 544 },
    { 0x000F34, 545 },
    { 0x000F35, 546 },
    { 0x000F36, 547 },
    { 0x000F37, 548 },
    { 0x000F38, 549 },
    { 0x000F39, 550 },
    { 0x000F3A, 551 },
    { 0x000F3B, 552 },
    { 0x000F3C, 553 },
    { 0x000F3D, 554 },
    { 0x000F3E, 555 },             // Z
    { 0x000F20, 556 },             // NUMPAD_0
    { 0x000F21, 557 },
    { 0x000F22, 558 },
    { 0x000F23, 559 },
    { 0x000F24, 560 },
    { 0x000F25, 561 },
    { 0x000F26, 562 },
    { 0x000F27, 563 },
    { 0x000F28, 564 },
    { 0x000F29, 565 },             // NUMPAD_9
    { 0x000F1F, 566 },             // HISENSE_PRODUCT_SCAN_START
    { 0x000F30, 567 },             // HISENSE_PRODUCT_SCAN_OVER
    { 0x000F00, 591 },             // HISENSE_TEST_BROAD_TV
    { 0x000F01, 592 },             // HISENSE_TEST_BROAD_DTV
    { 0x000F02, 593 },             // HISENSE_TEST_BROAD_AV1
    { 0x000F03, 594 },             // HISENSE_TEST_BROAD_AV2
    { 0x000F04, 595 },             // HISENSE_TEST_BROAD_AV3
    { 0x000F05, 596 },             // HISENSE_TEST_BROAD_SVIDEO1
    { 0x000F06, 597 },             // HISENSE_TEST_BROAD_SVIDEO2
    { 0x000F07, 598 },             // HISENSE_TEST_BROAD_SVIDEO3
    { 0x000F08, 599 },             // HISENSE_TEST_BROAD_SCART1
    { 0x000F09, 600 },             // HISENSE_TEST_BROAD_SCART2
    { 0x000F0A, 601 },             // HISENSE_TEST_BROAD_SCART3
    { 0x000F0B, 602 },             // HISENSE_TEST_BOARD_YPbPr1
    { 0x000F0C, 603 },             // HISENSE_TEST_BOARD_YPbPr2
    { 0x000F0D, 604 },             // HISENSE_TEST_BOARD_YPbPr3
    { 0x000F0E, 605 },             // HISENSE_TEST_BOARD_VGA
    { 0x000F0F, 606 },             // HISENSE_TEST_BOARD_HDMI1
    { 0x000F10, 607 },             // HISENSE_TEST_BOARD_HDMI2
    { 0x000F11, 608 },             // HISENSE_TEST_BOARD_HDMI3
    { 0x000F12, 609 },             // HISENSE_TEST_BOARD_HDMI4
    { 0x000F13, 610 },             // HISENSE_TEST_BOARD_HDMI5
    { 0x000F14, 611 },             // HISENSE_TEST_BOARD_DMP
    { 0x000F15, 612 },             // HISENSE_TEST_BOARD_EMP
    { 0x000F16, 613 },             // HISENSE_TEST_BOARD_AUTOCOLOR
    { 0x000F17, 614 },             // HISENSE_TEST_BOARD_SAVE
    { 0x000F18, 615 },             // HISENSE_TEST_BOARD_TELETEXT
    { 0x000F19, 616 },             // HISENSE_TEST_BOARD_SAPL
    { 0x000F1A, 617 },             // HISENSE_TEST_BOARD_VCHIP
    { 0x000F1B, 618 },             // HISENSE_TEST_BOARD_CCD
    { 0x000F1C, 619 },             // HISENSE_TEST_BOARD_BTSC
    { 0x000F1D, 620 },             // HISENSE_TEST_BOARD_FAC_OK

	{ 0x00FC50, 621 },//KEY_FN_RDRV_PLUS }, 
	{ 0x00FC46, 622 },//KEY_FN_RDRV_INC }, 
	{ 0x00FC4C, 623 },//KEY_FN_GDRV_PLUS }, 
	{ 0x00FC5A, 624 },//KEY_FN_GDRV_INC }, 
	{ 0x00FC49, 625 },//KEY_FN_BDRV_PLUS }, 
	{ 0x00FC4A, 626 },//KEY_FN_BDRV_INC }, 
	{ 0x00FC44, 627 },//KEY_FN_RCUT_PLUS }, 
	{ 0x00FC41, 628 },//KEY_FN_RCUT_INC }, 
	{ 0x00FC4B, 629 },//KEY_FN_GCUT_PLUS }, 
	{ 0x00FC51, 630 },//KEY_FN_GCUT_INC }, 
	{ 0x00FC08, 631 },//KEY_FN_BCUT_PLUS }, 
	{ 0x00FC45, 641 },//KEY_FN_BCUT_INC }, 

    { 0x4D, 642 },            //  KEYCODE_MEDIA_SONG_SYSTEM },
    { 0x4E, 643 },            //KEYCODE_MEDIA_PLAY_PAUSE },
    { 0x5A, 644 },            //KEYCODE_MEDIA_STOP },

    // 3th IR cotroller.
    { 0x00FC5D, 568 },             // HISENSE_FAC_NEC_F1
    { 0x00FC42, 569 },             // HISENSE_FAC_NEC_F2
    { 0x00FC56, 570 },             // HISENSE_FAC_NEC_F3
    { 0x00FC48, 571 },             // HISENSE_FAC_NEC_F4
    { 0x00FC53, 572 },             // HISENSE_FAC_NEC_F5
    { 0x00FC1D, 573 },             // HISENSE_FAC_NEC_F6
    { 0x00FC4F, 574 },             // HISENSE_FAC_NEC_F7
    { 0x00FC47, 575 },             // HISENSE_FAC_NEC_OK
    { 0x00FC0A, 576 },             // HISENSE_FAC_NEC_MAC
    { 0x00FC4E, 577 },             // HISENSE_FAC_NEC_IP
    { 0x00FC00, 578 },             // HISENSE_FAC_NEC_M
    { 0x00FC05, 579 },             // HISENSE_FAC_NEC_AV
    { 0x00FC18, 580 },             // HISENSE_FAC_NEC_PC
    { 0x00FC52, 581 },             // HISENSE_FAC_NEC_HDMI
    { 0x00FC01, 582 },             // HISENSE_FAC_NEC_YPBPR
    { 0x00FC06, 586 },             // HISENSE_FAC_NEC_BALANCE
    { 0x00FC0D, 588 },             // HISENSE_FAC_NEC_LOGO
    { 0x00FC19, 590 },             // HISNESE_FAC_NEC_PANEL
#if (HISENSE_USE_BOX==0)
	{ 0x00FC09, 583 },             // HISENSE_FAC_NEC_SAVE
    { 0x00FC58, 584 },             // HISENSE_FAC_NEC_PATTERN
    { 0x00FC15, 585 },             // HISENSE_FAC_NEC_AGING
	{ 0x00FC40, 587 },             // HISENSE_FAC_NEC_ADC
	{ 0x00FC1C, 589 },             // HISENSE_FAC_NEC_3D
#else
	{ 0x00FC09, KEY_LEFT },     //box factory ir  
    { 0x00FC58, KEY_UP },            
    { 0x00FC15, KEY_RIGHT },           
    { 0x00FC40, KEY_DOWN},             
    { 0x00FC1C, KEY_ENTER},            
#endif
	// box ir
	{ 0xBC0D, KEY_POWER },
    { 0xBC0E, KEY_MUTE },
    { 0xBC11, KEY_AUDIO },      // SOUND_MODE
    { 0xBC13, KEY_ZOOM },       // ASPECT_RATIO
    { 0xBC0B, KEY_CHANNEL },    // CHANNEL_RETURN
    { 0xBC00, KEY_0 },
    { 0xBC01, KEY_1 },
    { 0xBC02, KEY_2 },
    { 0xBC03, KEY_3 },
    { 0xBC04, KEY_4 },
    { 0xBC05, KEY_5 },
    { 0xBC06, KEY_6 },
    { 0xBC07, KEY_7 },
    { 0xBC08, KEY_8 },
    { 0xBC09, KEY_9 },
    { 0xBC14, KEY_MENU },
    { 0xBC48, KEY_BACK },
    { 0xBC16, KEY_UP },
    { 0xBC17, KEY_DOWN },
    { 0xBC18, KEY_RIGHT },
    { 0xBC19, KEY_LEFT },
    { 0xBC15, KEY_ENTER },
    { 0xBC4A, KEY_CHANNELUP },
    { 0xBC4B, KEY_CHANNELDOWN },
    { 0xBC44, KEY_VOLUMEUP },
    { 0xBC43, KEY_VOLUMEDOWN },
    { 0xBC5C, KEY_HOME},
    { 0xBC0C, KEY_INFO },
    { 0xBC1B, KEY_TV },
    { 0xBC5B, KEY_SEARCH },
    { 0xBC1E, KEY_FAVORITES },
    { 0xBC57, KEY_REWIND },
    { 0xBC56, KEY_FORWARD },
    { 0xBC58, KEY_PREVIOUSSONG },
    { 0xBC59, KEY_NEXTSONG },
    { 0xBC5A, KEY_STOP },
    { 0xBC4E, KEY_PLAYPAUSE },
    { 0xBC4C, KEY_FN_F1 },      // MTS
    { 0xBC0F, KEY_FN_F2 },      // FREEZE
    { 0xBC12, KEY_FN_F6 },      // TV_INPUT
    { 0xBC5F, KEY_FN_F7 },      // 3D_MODE
};

static struct key_map_list hisense_tv_map = {
	.map = {
		.scan    = hisense_tv,
		.size    = ARRAY_SIZE(hisense_tv),
		.name    = NAME_KEYMAP_HISENSE_TV,
        .headcode     = NUM_KEYMAP_HISENSE_TV,
        //.protocol     = (1<<IR_TYPE_NEC),
	}
};

int init_key_map_hisense_tv(void)
{
    IR_PRINT("[IR Log] %s\n", __func__);
	return MIRC_Map_Register(&hisense_tv_map);
}
EXPORT_SYMBOL(init_key_map_hisense_tv);

void exit_key_map_hisense_tv(void)
{
	MIRC_Map_UnRegister(&hisense_tv_map);
}
EXPORT_SYMBOL(exit_key_map_hisense_tv);
