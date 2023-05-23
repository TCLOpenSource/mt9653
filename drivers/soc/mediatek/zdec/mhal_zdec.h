/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MHAL_ZDEC_H__
#define __MHAL_ZDEC_H__

#ifndef GET_LOWORD
#define GET_LOWORD(value) ((unsigned short)(((unsigned int)(value)) & 0xffff))
#endif
#ifndef GET_HIWORD
#define GET_HIWORD(value) ((unsigned short)((((unsigned int)(value)) >> 16) & 0xffff))
#endif
#ifndef ALIGN16
#define ALIGN16(value)       ((((value) + 15) >> 4) << 4)
#endif
#ifndef BITS2BYTE
#define BITS2BYTE(x)         ((x) >> 3)
#endif

#define DECODING                 0UL
#define LOADING_PRESET_DICT_MIU  1UL
#define LOADING_PRESET_DICT_RIU  2UL

#define CONTIGUOUS_MODE          0UL
#define SCATTER_MODE             1UL

#define EMMC_TABLE               0UL
#define NAND_TABLE               1UL

// These functions configure a copy of ZDEC reg table in memory.
// The configuration will not take effect until 'MHal_ZDEC_Start_Operation' is called.
void MHal_ZDEC_Conf_Reset(void);
void MHal_ZDEC_Conf_Zmem(unsigned char miu, unsigned long addr, unsigned int size);
void MHal_ZDEC_Conf_FCIE_Handshake(unsigned char enable);
void MHal_ZDEC_Conf_Input_Shift(unsigned int skip_words_cnt,
	unsigned char shift_byte, unsigned char shift_bit);
void MHal_ZDEC_Conf_Preset_Dictionary(unsigned int size);
void MHal_ZDEC_Conf_Contiguous_Mode
	(unsigned char obuf_miu,
	unsigned int obuf_addr, unsigned int obuf_size);
void MHal_ZDEC_Conf_Scatter_Mode
	(unsigned char dst_tbl_miu, unsigned long dst_tbl_addr,
	unsigned char nand_table, unsigned char in_nand_page_size,
	unsigned char out_nand_page_size);


// Applies configuration stored in memory reg table and starts ZDEC operation.
// After the call, ZDEC will be ready to receive input data.
int MHal_ZDEC_Start_Operation(unsigned char op_mode);


void MHal_ZDEC_Feed_Data
	(unsigned char last, unsigned char miu,
	unsigned long sadr, unsigned int size);
int MHal_ZDEC_Check_Internal_Buffer(void);
int MHal_ZDEC_Check_ADMA_Table_Done(void);
int MHal_ZDEC_Check_Last_ADMA_Table_Done(void);
int MHal_ZDEC_Check_MIU_Load_Dict_Done(void);
int MHal_ZDEC_Check_Decode_Done(void);
void MHal_ZDEC_RIU_Bank_Init(void);
int MHal_ZDEC_RIU_Load_Preset_Dictionary(unsigned char *dict);
void dump_all_val(void);
int MHal_ZDEC_handler(int *outlen);

#endif

