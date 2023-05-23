/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

/////////////////////////////////////////////////
///
/// file	ir_dynamic_config.h
/// @brief	Load IR config from INI config file
/////////////////////////////////////////////////

#ifndef __IR_DYNAMIC_CONFIG__
#define __IR_DYNAMIC_CONFIG__

#include <linux/input.h>

#define IR_CONFIG_PATH_LEN 256

extern void MIRC_Map_free(void);
extern void MIRC_Decoder_free(void);
extern void MIRC_Dis_support_ir(void);
extern char *saved_command_line;
/************************************************/
//&step2:  IR  protocols Type enum
enum IR_Type_e {
	IR_TYPE_NEC = 0,		/* NEC protocol */
	IR_TYPE_RC5,			/* RC5 protocol*/
	IR_TYPE_RC6,			/* RC6 protocol*/
	IR_TYPE_RCMM,			/* RCMM protocol*/
	IR_TYPE_KONKA,			/* Konka protocol*/
	IR_TYPE_HAIER,			/* Haier protocol*/
	IR_TYPE_RCA,			/* TCL RCA protocol**/
	IR_TYPE_P7051,			/* Panasonic 7051 protocol**/
	IR_TYPE_TOSHIBA,		/* Toshiba protocol*/
	IR_TYPE_RC5X,			/* RC5 ext protocol*/
	IR_TYPE_RC6_MODE0,		/* RC6	mode0 protocol*/
	IR_TYPE_METZ,			/* Metz remote control unit protocol*/
	IR_TYPE_SKY_PANASONIC,	/* Skyworth Panasonic protocol*/
	IR_TYPE_SONY,			/* Sony protocol*/ // 0xe(14)
	IR_TYPE_BEKO_RC5,		/* Beko protocal(customized RC5) */
	IR_TYPE_SHARP,			/* Sharp protocol*/ // 0x10(16)
	IR_TYPE_RC311,			/* RC311(RC-328ST) remote control */
	IR_TYPE_PANASONIC,		/* PANASONIC protocol */
	IR_TYPE_MAX,
};

enum DECODE_Type_e {
	HW_DECODE = 0,
	SW_DECODE,
};


//Description  of IR
struct IR_Profile_s {
	enum IR_Type_e eIRType;
	u32 u32HeadCode;
	u32 u32IRSpeed;
	u8 u8Enable;
};

//struct key_map_table {
//	u32 scancode;
//	u32 keycode;
//};
//
//struct key_map {
//	struct key_map_table	*scan;
//	unsigned int		size;	/* Max number of entries */
//	const char		*name;
//	u32				 headcode;
//};
//
//struct key_map_list {
//	struct list_head	 list;
//	struct key_map map;
//};

#define KEYMAP_PAIR(x)		{#x, x}
#define IRPROTOCOL_PAIR(x)	{#x, IR_TYPE_##x}
struct key_value_s {
	char *key;
	u32 value;
};

//load ir_config_ini for pm51
#if defined(CONFIG_MTK_PM)
#define IR_HEAD16_KEY_PM_CFG_OFFSET	  5
#define IR_HEAD32_KEY_PM_CFG_OFFSET	  25
#define IR_SUPPORT_PM_NUM_MAX		  5

struct IR_PM51_Profile_s {
	enum IR_Type_e eIRType;
	u32 u32Header;
	u32 u32PowerKey;
};




extern ssize_t MDrv_PM_Set_IRCfg(const u8 *buf, size_t size);
#endif
struct by_pass_kernel_keymap_Profile_s {
	enum DECODE_Type_e eDECType;
};


//int MIRC_Map_Register(struct key_map_list *map);
//void MIRC_Map_UnRegister(struct key_map_list *map);
int MIRC_IRCustomer_Add(struct IR_Profile_s *stIr);
int MIRC_IRCustomer_Remove(struct IR_Profile_s *stIr);
int MIRC_IRCustomer_Init(void);
void mtk_ir_reinit(void);
u32 MIRC_Get_Keycode(u32 keymapnum, u32 scancode);
unsigned int get_ir_keycode(unsigned int scancode);

#endif //__IR_DYNAMIC_CONFIG__
