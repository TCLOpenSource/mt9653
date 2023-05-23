/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#ifndef _IR_CONFIG_H_
#define _IR_CONFIG_H_
#include "mtk_ir.h"
#include "ir_core.h"
#include "ir_common.h"

extern struct list_head stage_List;
extern unsigned long ts_IsEnable;
extern struct IRModHandle IRDev;
extern u32 u32IRHeaderCode[2];

/*******************  extern function Start ******************/
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
#ifdef CONFIG_MTK_TV_IR_KEYMAP_MTK_NEC
int init_key_map_mtk_tv(void);
void exit_key_map_mtk_tv(void);
#endif
#ifdef CONFIG_MTK_TV_IR_KEYMAP_SONY
int init_key_map_sony_tv(void);
void exit_key_map_sony_tv(void);
#endif
#ifdef CONFIG_IR_KEYMAP_TCL_RCA
int init_key_map_tcl_rca_tv(void);
void exit_key_map_tcl_rca_tv(void);
#endif
#ifdef CONFIG_IR_KEYMAP_TCL
int init_key_map_tcl_tv(void);
void exit_key_map_tcl_tv(void);
#endif
#ifdef CONFIG_IR_KEYMAP_KONKA
int init_key_map_konka_tv(void);
void exit_key_map_konka_tv(void);
#endif


#ifdef CONFIG_IR_KEYMAP_KATHREIN
int init_key_map_rc6_kathrein(void);
void exit_key_map_rc6_kathrein(void);
#endif
#ifdef CONFIG_IR_KEYMAP_RC5
int init_key_map_rc5_tv(void);
void exit_key_map_rc5_tv(void);
#endif

#ifdef CONFIG_IR_KEYMAP_SAMPO
int init_key_map_rc311_samp(void);
void exit_key_map_rc311_samp(void);
#endif

#endif

/****************** For Customer Config	 Start [can modify by customers] ********************/

//Number of IR should this chip supported
#define IR_SUPPORT_NUM 1

//Add & Modify Customer IR with Differ Headcode Here
static struct IR_Profile_s ir_config[IR_SUPPORT_NUM] = {
//	 protocol_type, Headcode, IRSpeed ,enable
	{IR_TYPE_NEC, NUM_KEYMAP_MTK_TV, 0, 1},	   // mtk IR customer code
	//{IR_TYPE_METZ,NUM_KEYMAP_METZ_RM18_TV,0,1}, // metz rm18 IR customer code
	//{IR_TYPE_METZ,NUM_KEYMAP_METZ_RM19_TV,0,1}, // metz rm19 IR customer code
	//{IR_TYPE_TOSHIBA,NUM_KEYMAP_SKYWORTH_TV,0,1}, //skyworth toshiba ir
	//{IR_TYPE_NEC,NUM_KEYMAP_CHANGHONG_TV,0,1}, // changhong_RL78B
	//{IR_TYPE_NEC,NUM_KEYMAP_HISENSE_TV,0,1},	 // Hisense IR customer code
	//{IR_TYPE_RCA,NUM_KEYMAP_TCL_RCA_TV,0,1},	// TCL RCA	customer code
	//{IR_TYPE_P7051,NUM_KEYMAP_P7051_STB,0,1}, // Panasonic 7051 IR customer code
	//{IR_TYPE_RC5,NUM_KEYMAP_RC5_TV,0,1},		// RC5 customer code
	//{IR_TYPE_RC6,NUM_KEYMAP_KATHREIN_TV,0,1}, //Kathrein RC6 customer code
	//{IR_TYPE_KONKA,NUM_KEYMAP_KONKA_TV,2,1},
	//{IR_TYPE_BEKO_RC5,NUM_KEYMAP_BEKO_RC5_TV,0,1},//Beko RC5 customer code
	//{IR_TYPE_SHARP, NUM_KEYMAP_SHARP_TV, 0, 1},	//Sharp IR customer code1
	//{IR_TYPE_SONY, NUM_KEYMAP_SONY_TV1, 0, 1},	//Sony IR customer code1
	//{IR_TYPE_SONY, NUM_KEYMAP_SONY_TV2, 0, 1},	//Sony IR customer code2
	//{IR_TYPE_SONY, NUM_KEYMAP_SONY_TV3, 0, 1},	//Sony IR customer code3
	//{IR_TYPE_SONY, NUM_KEYMAP_SONY_TV4, 0, 1},	//Sony IR customer code4
	//{IR_TYPE_SONY, NUM_KEYMAP_SONY_TV5, 0, 1},	//Sony IR customer code5
	//{IR_TYPE_SONY, NUM_KEYMAP_SONY_TV6, 0, 1},	//Sony IR customer code6
};

/****************** For Customer Config	 End [can modify by customers] ********************/

#endif
