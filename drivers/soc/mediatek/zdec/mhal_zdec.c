// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if defined(MSOS_TYPE_LINUX)
#include <string.h>
#include "mdrv_types.h"
#elif defined(__KERNEL__)
#include <linux/string.h>
#elif defined(MSOS_TYPE_NOS)
#include <string.h>
#include "datatype.h"
#include "zlib_hwcfg.h"
#endif
#include <linux/io.h>
#include <linux/of.h>
#include "mhal_zdec.h"
#include "mhal_zdec_reg.h"

struct ZDEC_REGS g_regs;

#define TRACE(s, args...)	pr_debug("ZDEC: "s, ## args)

//-------------------------------------------------------------------------------------------------
// Local Functions
//-------------------------------------------------------------------------------------------------

void mhal_zdec_power_on(void)
{
	ZDEC_REG_WRITE(REG_ZDEC_GLOBAL_WOC, ZDEC_WOC_SET_CLOCK_ON);
}


void mhal_zdec_power_off(void)
{

}


void mhal_zdec_sw_reset(void)
{
	g_regs.reg_zdec_sw_rst = 0;

	ZDEC_REG_WRITE(0x00, g_regs.reg00);
	g_regs.reg_zdec_sw_rst = 1;

	ZDEC_REG_WRITE(0x00, g_regs.reg00);
	ZDEC_REG_WRITE(REG_ZDEC_GLOBAL_WOC, ZDEC_WOC_SET_CLOCK_ON);
	}


void mhal_zdec_set_hw_miu_selection(void)
{

}


void mhal_zdec_clear_all_interrupts(void)
{
	ZDEC_REG_WRITE(REG_ZDEC_IRQ_CLEAR, ZDEC_IRQ_ALL);
}


void mhal_zdec_set_all_regs(void)
{
	ZDEC_REG_WRITE(SET_0x00, g_regs.reg00);
	ZDEC_REG_WRITE(SET_0x02, g_regs.reg02);
	ZDEC_REG_WRITE(SET_0x06, g_regs.reg06);
	ZDEC_REG_WRITE(SET_0x07, g_regs.reg07);
	ZDEC_REG_WRITE(SET_0x08, g_regs.reg08);
	ZDEC_REG_WRITE(SET_0x09, g_regs.reg09);
	ZDEC_REG_WRITE(SET_0x0a, g_regs.reg0a);
	ZDEC_REG_WRITE(SET_0x0b, g_regs.reg0b);
	ZDEC_REG_WRITE(SET_0x0c, g_regs.reg0c);
	ZDEC_REG_WRITE(SET_0x0f, g_regs.reg0f);
	ZDEC_REG_WRITE(SET_0x10, g_regs.reg10);
	ZDEC_REG_WRITE(SET_0x11, g_regs.reg11);
	ZDEC_REG_WRITE(SET_0x12, g_regs.reg12);
	ZDEC_REG_WRITE(SET_0x13, g_regs.reg13);
	ZDEC_REG_WRITE(SET_0x15, g_regs.reg15);
	ZDEC_REG_WRITE(SET_0x16, g_regs.reg16);
	ZDEC_REG_WRITE(SET_0x17, g_regs.reg17);
	ZDEC_REG_WRITE(SET_0x18, g_regs.reg18);
	ZDEC_REG_WRITE(SET_0x19, g_regs.reg19);
	ZDEC_REG_WRITE(SET_0x1a, g_regs.reg1a);
	ZDEC_REG_WRITE(SET_0x1b, g_regs.reg1b);
	ZDEC_REG_WRITE(SET_0x1c, g_regs.reg1c);
	ZDEC_REG_WRITE(SET_0x1d, g_regs.reg1d);
	ZDEC_REG_WRITE(SET_0x1e, g_regs.reg1e);
	ZDEC_REG_WRITE(SET_0x20, g_regs.reg20);
	ZDEC_REG_WRITE(SET_0x21, g_regs.reg21);
	ZDEC_REG_WRITE(SET_0x22, g_regs.reg22);
	ZDEC_REG_WRITE(SET_0x28, g_regs.reg28);
	ZDEC_REG_WRITE(SET_0x2a, g_regs.reg2a);
	ZDEC_REG_WRITE(SET_0x2f, g_regs.reg2f);
	ZDEC_REG_WRITE(SET_0x30, g_regs.reg30);
	ZDEC_REG_WRITE(SET_0x31, g_regs.reg31);
	ZDEC_REG_WRITE(SET_0x32, g_regs.reg32);
	ZDEC_REG_WRITE(SET_0x33, g_regs.reg33);
	ZDEC_REG_WRITE(SET_0x34, g_regs.reg34);
	ZDEC_REG_WRITE(SET_0x35, g_regs.reg35);
	ZDEC_REG_WRITE(SET_0x36, g_regs.reg36);
	ZDEC_REG_WRITE(SET_0x38, g_regs.reg38);
	ZDEC_REG_WRITE(SET_0x39, g_regs.reg39);
	ZDEC_REG_WRITE(SET_0x40, g_regs.reg40);
	ZDEC_REG_WRITE(SET_0x41, g_regs.reg41);
	ZDEC_REG_WRITE(SET_0x42, g_regs.reg42);
	ZDEC_REG_WRITE(SET_0x43, g_regs.reg43);
	ZDEC_REG_WRITE(SET_0x44, g_regs.reg44);
	ZDEC_REG_WRITE(SET_0x46, g_regs.reg46);
	ZDEC_REG_WRITE(SET_0x47, g_regs.reg47);
	ZDEC_REG_WRITE(SET_0x48, g_regs.reg48);
	ZDEC_REG_WRITE(SET_0x49, g_regs.reg49);
	ZDEC_REG_WRITE(SET_0x50, g_regs.reg50);
	ZDEC_REG_WRITE(SET_0x51, g_regs.reg51);
	ZDEC_REG_WRITE(SET_0x54, g_regs.reg54);
	ZDEC_REG_WRITE(SET_0x58, g_regs.reg58);
	ZDEC_REG_WRITE(SET_0x59, g_regs.reg59);
	ZDEC_REG_WRITE(SET_0x5a, g_regs.reg5a);
#ifdef CONFIG_MTK_ACP_MODE
	ZDEC_REG_WRITE(SET_0x62, g_regs.reg62);
#endif
	ZDEC_REG_WRITE(SET_0x68, g_regs.reg68);
	ZDEC_REG_WRITE(SET_0x71, g_regs.reg71);
	ZDEC_REG_WRITE(SET_0x72, g_regs.reg72);
	ZDEC_REG_WRITE(SET_0x75, g_regs.reg75);
	ZDEC_REG_WRITE(SET_0x78, g_regs.reg78);
	ZDEC_REG_WRITE(SET_0x79, g_regs.reg79);
	ZDEC_REG_WRITE(SET_0x7a, g_regs.reg7a);
	ZDEC_REG_WRITE(SET_0x7b, g_regs.reg7b);
	ZDEC_REG_WRITE(SET_0x7c, g_regs.reg7c);
	ZDEC_REG_WRITE(SET_0x7d, g_regs.reg7d);
	ZDEC_REG_WRITE(SET_0x7e, g_regs.reg7e);
	ZDEC_REG_WRITE(SET_0x7f, g_regs.reg7f);
}


//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------

void MHal_ZDEC_Conf_Reset(void)
{
	memset(&g_regs, 0, sizeof(g_regs));

	//
	// power on defaults
	//
	g_regs.reg_zdec_sw_rst = 1;

	g_regs.reg_zdec_early_done_thd = SET_0x03;

	g_regs.reg_zdec_ibuf_burst_len = SET_0x03;

	g_regs.reg_zdec_ibuf_reverse_bit = 1;

	g_regs.reg_zdec_obuf_burst_length = SET_0x03;

	g_regs.reg_zdec_obuf_byte_write_en = 1;

	g_regs.reg_zdec_obuf_timeout = SET_0x08;

	g_regs.reg_zdec_obuf_cnt_no_wait_last_done_z = 1; //johnny

	g_regs.reg_zdec_zmem_burst_len = SET_0x04;

	g_regs.reg_zdec_mreq_sub_control = SET_0xffff; //0x38

	g_regs.reg_jobwptr_delta = 1; //johnny0x46

	g_regs.reg_arcache = SET_0x0b;//0x47

	g_regs.reg_arprot = SET_0x02;//0x47

	g_regs.reg_awcache = SET_0x07; //0x48

	g_regs.reg_awprot = SET_0x02;//0x48

	g_regs.reg_awuser = 0x1;//0x48

	g_regs.reg_rburst_len = SET_0x03; //0x49

	g_regs.reg_wburst_len = SET_0x03; //0x49

	//
	// same value for all scenerios
	//

	// OBUF
	g_regs.reg_zdec_obuf_datacnt_en = 1;

	g_regs.reg_zdec_obuf_burst_split = 1;

	g_regs.reg_zdec_irq_mask = SET_0x0e;

	// LZD write-out byte count

	g_regs.reg_zdec_dec_cnt_sel = 1;

	// CRC
	g_regs.reg_zdec_crc_en = 1;

	g_regs.reg_zdec_crc_mode = 1;

	g_regs.reg_zdec_crc_sel = 0;

	// ECO item
	g_regs.reg_zdec_spare00 = SET_0x03;

#ifdef CONFIG_MTK_ACP_MODE
	g_regs.reg_acpr_enable = 1;

	g_regs.reg_acpw_enable = 1;

#endif
}


void MHal_ZDEC_Conf_Zmem(unsigned char miu, unsigned long addr, unsigned int size)
{
	g_regs.reg_zdec_zmem_miu_sel  = miu;

	g_regs.reg_zdec_zmem_saddr_lo = GET_LOWORD(addr);

	g_regs.reg_zdec_zmem_saddr_hi = GET_HIWORD(addr);

	g_regs.reg_zdec_zmem_eaddr_lo = GET_LOWORD(addr + size - 1);

	g_regs.reg_zdec_zmem_eaddr_hi = GET_HIWORD(addr + size - 1);

	if (addr > SET_0xffffffff) {
		g_regs.reg_zdec_zmem_saddr_msb = (addr >> SET_32) & SET_0x03;

		g_regs.reg_zdec_zmem_eaddr_msb = ((addr + size) >> SET_32) & SET_0x03;
	}
}


void MHal_ZDEC_Conf_FCIE_Handshake(unsigned char enable)
{
	g_regs.reg_zdec_fcie_hw_talk = (enable == 0) ? 0 : 1;
}


void MHal_ZDEC_Conf_Input_Shift(unsigned int skip_words_cnt,
			unsigned char shift_byte, unsigned char shift_bit)
{
	g_regs.reg_zdec_ibuf_skip_words_cnt_lo = GET_LOWORD(skip_words_cnt);

	g_regs.reg_zdec_ibuf_skip_words_cnt_hi = GET_HIWORD(skip_words_cnt);

	g_regs.reg_zdec_ibuf_shift_byte = shift_byte;

	g_regs.reg_zdec_ibuf_shift_bit = shift_bit;

}


void MHal_ZDEC_Conf_Preset_Dictionary(unsigned int size)
{
	g_regs.reg_zdec_preset_dic_bytes = ALIGN16(size);
}

void MHal_ZDEC_Conf_Scatter_Mode(unsigned char dst_tbl_miu,
								unsigned long dst_tbl_addr,
								unsigned char table_format,
								unsigned char in_nand_page_size,
								unsigned char out_nand_page_size)
{
	g_regs.reg_zdec_iobuf_mode = SCATTER_MODE;

	g_regs.reg_zdec_dst_tbl_miu_sel = dst_tbl_miu;

	g_regs.reg_zdec_dst_tbl_addr_lo = GET_LOWORD(dst_tbl_addr);

	g_regs.reg_zdec_dst_tbl_addr_hi = GET_HIWORD(dst_tbl_addr);

	if (dst_tbl_addr > SET_0xffffffff) //over 32bits
		g_regs.reg_zdec_dst_tbl_addr_msb = (dst_tbl_addr >> SET_32) & SET_0x03;
	else
		g_regs.reg_zdec_dst_tbl_addr_msb = 0;
	if (table_format == EMMC_TABLE) {
		g_regs.reg_zdec_tbl_format = EMMC_TABLE;
	} else {
		g_regs.reg_zdec_tbl_format = NAND_TABLE;

		g_regs.reg_zdec_page_size = in_nand_page_size;

		g_regs.reg_zdec_out_page_size = out_nand_page_size;

	}
}


int MHal_ZDEC_Start_Operation(unsigned char op_mode)
{
	mhal_zdec_power_on();
	mhal_zdec_sw_reset();
	mhal_zdec_clear_all_interrupts();
	mhal_zdec_set_all_regs();

	if (op_mode == DECODING)
		ZDEC_REG_WRITE(REG_ZDEC_GLOBAL_WOC, ZDEC_WOC_FRAME_ST);
	else if (op_mode == LOADING_PRESET_DICT_MIU)
		ZDEC_REG_WRITE(REG_ZDEC_GLOBAL_WOC, ZDEC_WOC_LOAD_DIC);

	return 0;
}


void MHal_ZDEC_Feed_Data(unsigned char last,
		unsigned char miu, unsigned long sadr, unsigned int size)
{
	if (g_regs.reg_zdec_iobuf_mode == SCATTER_MODE) {
		g_regs.reg_zdec_src_tbl_miu_sel = miu;

		g_regs.reg_zdec_src_tbl_last = (last == 0) ? 0 : 1;

		g_regs.reg_zdec_src_tbl_addr_lo = GET_LOWORD(sadr);

		g_regs.reg_zdec_src_tbl_addr_hi = GET_HIWORD(sadr);

		if (sadr > SET_0xffffffff) //over 32bits
			g_regs.reg_zdec_src_tbl_addr_msb = (sadr >> SET_32) & SET_0x03;
		else
			g_regs.reg_zdec_src_tbl_addr_msb = 0;

		ZDEC_REG_WRITE(SET_0x40, g_regs.reg40);
		ZDEC_REG_WRITE(SET_0x41, g_regs.reg41);
		ZDEC_REG_WRITE(SET_0x42, g_regs.reg42);
		ZDEC_REG_WRITE(SET_0x3f, g_regs.reg3f);
		ZDEC_REG_WRITE(REG_ZDEC_WOC_NXT_SRC_TBL_VLD, ZDEC_WOC_NXT_SRC_TBL_VLD);
	}
}


int MHal_ZDEC_Check_Internal_Buffer(void)
{
	unsigned short reg_val = ZDEC_REG(REG_ZDEC_IBUF_R_CMD);

	if ((0 == (reg_val & ZDEC_IBUF_R_CMD_FULL))
	    && (((reg_val & ((unsigned short)(SET_0xff) << SET_0x04)) >> SET_0x04) < SET_0x02)) {
		return 0;
	} else {
		return -1;
	}
}


int MHal_ZDEC_Check_ADMA_Table_Done(void)
{
	unsigned short reg_val = ZDEC_REG(REG_ZDEC_IRQ_FINAL_S);
	int ret;

	if (0 == (reg_val & ZDEC_IRQ_ADMA_TABLE_DONE)) {
		ret = -1;
	} else {
		ZDEC_REG_WRITE(REG_ZDEC_IRQ_CLEAR, ZDEC_IRQ_ADMA_TABLE_DONE);
		ret = 0;
	}
	return ret;
}


int MHal_ZDEC_Check_Last_ADMA_Table_Done(void)
{
	unsigned short reg_val = ZDEC_REG(REG_ZDEC_IRQ_FINAL_S);
	int ret;

	if (0 == (reg_val & ZDEC_IRQ_LAST_ADMA_TABLE_DONE)) {
		ret = -1;
	} else {
		ZDEC_REG_WRITE(REG_ZDEC_IRQ_CLEAR, ZDEC_IRQ_LAST_ADMA_TABLE_DONE);
		ret = 0;
	}
	return ret;
}


int MHal_ZDEC_Check_MIU_Load_Dict_Done(void)
{
	unsigned short reg_val = ZDEC_REG(REG_ZDEC_IRQ_FINAL_S);
	int ret;

	if (0 == (reg_val & ZDEC_IRQ_PRESET_DICTIONARY_LOAD_DONE)) {
		ret = -1;
	} else {
		ZDEC_REG_WRITE(REG_ZDEC_IRQ_CLEAR, ZDEC_IRQ_PRESET_DICTIONARY_LOAD_DONE);
		ret = 0;
	}
	return ret;
}


int MHal_ZDEC_Check_Decode_Done(void)
{
	unsigned short reg_val = ZDEC_REG(REG_ZDEC_IRQ_FINAL_S);

	int ret;

	if (0 == (reg_val & ZDEC_IRQ_DECODE_DONE)) {
		ret = -1;
	} else {
		mhal_zdec_clear_all_interrupts();
		mhal_zdec_power_off();

		// The value stored in reg_zdec_r_dec_cnt is always short by 1 byte.
		ret = (((int)(ZDEC_REG(REG_ZDEC_R_DEC_CNT_HI)) << SET_16) |
					ZDEC_REG(REG_ZDEC_R_DEC_CNT_LO)) + 1;
	}
	return ret;
}


int MHal_ZDEC_RIU_Load_Preset_Dictionary(unsigned char *dict)
{
	int i = 0;

	unsigned short reg_val;

	// reg_zdec_dic_riu_sel[0] => 1; switch on preset dictionary RIU mode
	g_regs.reg_zdec_dic_riu_sel = 1;

	// reg_zdec_dic_riu_rw[8]  => 1; writing mode (DRAM => SRAM)
	g_regs.reg_zdec_dic_riu_rw = 1;

	ZDEC_REG_WRITE(SET_0x5a, g_regs.reg5a);
	ZDEC_REG_WRITE(SET_0x58, g_regs.reg58);

	for (i = 0; i < g_regs.reg_zdec_preset_dic_bytes; ++i) {
		g_regs.reg_zdec_dic_riu_addr = i;

		g_regs.reg_zdec_dic_riu_wd = dict[i];

		ZDEC_REG_WRITE(SET_0x59, g_regs.reg59);
		ZDEC_REG_WRITE(SET_0x58, g_regs.reg58);
		ZDEC_REG_WRITE(REG_ZDEC_GLOBAL_WOC, ZDEC_DIC_RIU_WOC);

		do {
			reg_val = ZDEC_REG(REG_ZDEC_DIC_RIU_RD);

		} while (0 == (reg_val & ZDEC_DIC_RIU_RDY));
	}

	g_regs.reg_zdec_dic_riu_sel = 0;

	ZDEC_REG_WRITE(SET_0x5a, g_regs.reg5a);
	return 0;
}

int MHal_ZDEC_handler(int *outlen)
{
	unsigned short reg_val = ZDEC_REG(REG_ZDEC_IRQ_FINAL_S);

	int ret = 0;

	if (reg_val & ZDEC_IRQ_DECODE_DONE)
		*outlen = (((int)(ZDEC_REG(REG_ZDEC_R_DEC_CNT_HI)) << SET_16)
					| ZDEC_REG(REG_ZDEC_R_DEC_CNT_LO)) + 1;
	else if (reg_val & ZDEC_IRQ_ZDEC_EXCEPTION) {
		*outlen = -1;
		ret = -1;
		dump_all_val();
	} else {
		*outlen = -1;
		ret = -SET_2;
		dump_all_val();
	}
	mhal_zdec_clear_all_interrupts();
	return ret;
}

void MHal_ZDEC_RIU_Bank_Init(void)
{
	// offset 0x41 = 0x2a00	offset 0x42 = 0x2a00	offset 0x3c = 0x2a
	// offset 0x46 = 0
	void __iomem *clk_base_0_0 =
			ioremap(NONPM_BASE + ((CKG_REG_BANK_00_0) << SET_9), SET_0x200);

	writew(0x0, clk_base_0_0 + ((SET_0x46) << SET_2));
	iounmap(clk_base_0_0);
	// offset 0x65 = 0x101
	void __iomem *clk_base_0_1 =
			ioremap(NONPM_BASE + ((CKG_REG_BANK_00_1) << SET_9), SET_0x200);

	writew(SET_0x101, clk_base_0_1 + ((SET_0x65) << SET_2));
	iounmap(clk_base_0_1);
	// offset 0xF7 = 0x101
	void __iomem *clk_base_1_0 =
			ioremap(NONPM_BASE + ((CKG_REG_BANK_01_0) << SET_9), SET_0x200);

	writew(SET_0x101, clk_base_1_0 + ((SET_0x77) << SET_2));
	iounmap(clk_base_1_0);
}

void dump_all_val(void)
{
	int i;

	for (i = 0; i <= SET_0x7f; i++)
		TRACE("offset %lx value %lx\n", i, ZDEC_REG(i));
}

