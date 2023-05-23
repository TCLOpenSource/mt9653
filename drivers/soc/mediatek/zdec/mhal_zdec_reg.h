/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef	__MHAL_ZDEC_REG_H__
#define	__MHAL_ZDEC_REG_H__

#ifndef	BIT
#define	BIT(x) (1UL	<< (x))
#endif






#define	SET_0x00 0x00
#define	SET_0x02 0x02
#define	SET_0x03 0x03
#define	SET_0x04 0x04
#define	SET_0x06 0x06
#define	SET_0x07 0x07
#define	SET_0x08 0x08
#define	SET_0x09 0x09
#define	SET_0x0a 0x0a
#define	SET_0x0b 0x0b
#define	SET_0x0c 0x0c
#define	SET_0x0e 0x0e
#define	SET_0x0f 0x0f
#define	SET_0x10 0x10
#define	SET_0x11 0x11
#define	SET_0x12 0x12
#define	SET_0x13 0x13
#define	SET_0x15 0x15
#define	SET_0x16 0x16
#define	SET_0x17 0x17
#define	SET_0x18 0x18
#define	SET_0x19 0x19
#define	SET_0x1a 0x1a
#define	SET_0x1b 0x1b
#define	SET_0x1c 0x1c
#define	SET_0x1d 0x1d
#define	SET_0x1e 0x1e
#define	SET_0x20 0x20
#define	SET_0x21 0x21
#define	SET_0x22 0x22
#define	SET_0x28 0x28
#define	SET_0x2a 0x2a
#define	SET_0x2f 0x2f
#define	SET_0x30 0x30
#define	SET_0x31 0x31
#define	SET_0x32 0x32
#define	SET_0x33 0x33
#define	SET_0x34 0x34
#define	SET_0x35 0x35
#define	SET_0x36 0x36
#define	SET_0x38 0x38
#define	SET_0x39 0x39
#define	SET_0x3f 0x3f
#define	SET_0x40 0x40
#define	SET_0x41 0x41
#define	SET_0x42 0x42
#define	SET_0x43 0x43
#define	SET_0x44 0x44
#define	SET_0x46 0x46
#define	SET_0x47 0x47
#define	SET_0x48 0x48
#define	SET_0x49 0x49
#define	SET_0x50 0x50
#define	SET_0x51 0x51
#define	SET_0x54 0x54
#define	SET_0x58 0x58
#define	SET_0x59 0x59
#define	SET_0x5a 0x5a
#define	SET_0x62 0x62
#define	SET_0x65 0x65
#define	SET_0x68 0x68
#define	SET_0x71 0x71
#define	SET_0x72 0x72
#define	SET_0x75 0x75
#define	SET_0x77 0x77
#define	SET_0x78 0x78
#define	SET_0x79 0x79
#define	SET_0x7a 0x7a
#define	SET_0x7b 0x7b
#define	SET_0x7c 0x7c
#define	SET_0x7d 0x7d
#define	SET_0x7e 0x7e
#define	SET_0x7f 0x7f
#define	SET_0xff 0xff
#define	SET_0x101 0x101
#define	SET_0x200 0x200
#define	SET_0xffff 0xffff
#define	SET_0xffffffff 0xffffffff //32 bit

#define	SET_2 2
#define	SET_9 9
#define	SET_16 16
#define	SET_32 32
#define	EMMC_SZIE 512

#define	NONPM_BASE 0x1c000000

#define	MIU_REG_BANK 0x1203UL // offset 0x41 =	0x2a00
							  //offset 0x42	= 0x2a00
							  //offset 0x3c	= 0x2a
#define	CKG_REG_BANK_00_0 0x1020UL // offset 0x46 =	0
#define	CKG_REG_BANK_00_1 0x102EUL // offset 0x65 =	0x101
#define	CKG_REG_BANK_01_0 0x103FUL // offset 0xF7 = 0x101

void __iomem *zdec_base;

#define	GET_ADDR(_bank,	_offset) (zdec_base	+ ((_offset) <<	2))

#define	ZDEC_REG(_offset) readw(zdec_base + ((_offset) << 2))

#define	ZDEC_REG_WRITE(_offset,	_val)		 writew(_val, zdec_base	+ ((_offset) <<	2))


//
// MIU registers
//

#define	REG_MIU_SEL_ZDEC					 0x7bUL
#define	MIU_SEL_ZDEC_0						  BIT(4)
#define	MIU_SEL_ZDEC_1						  BIT(5)


//
// CLKGEN registers	for	ZDEC
//

#define	REG_CKG_ZDEC						 0x70UL
#define	CKG_ZDEC_VLD_GATE					  BIT(0)
#define	CKG_ZDEC_LZD_GATE					  BIT(4)


//
// ZDEC	read-only registers
//

#define	REG_ZDEC_IBUF_R_CMD					 0x0dUL
#define	ZDEC_IBUF_R_CMD_FULL				 BIT(0) // reg_zdec_ibuf_r_cmd_full
#define	REG_ZDEC_R_DEC_CNT_LO				 0x25UL // reg_zdec_r_dec_cnt[15:0]
#define	REG_ZDEC_R_DEC_CNT_HI				 0x26UL // reg_zdec_r_dec_cnt[31:16]
#define	REG_ZDEC_DIC_RIU_RD					 0x5bUL
#define	ZDEC_DIC_RIU_RDY					 BIT(8) // reg_zdec_dic_riu_rdy
#define	REG_ZDEC_CRC_R_RESULT				 0x69UL


//
// ZDEC	IRQ	registers
//

#define	REG_ZDEC_IRQ_CLEAR 0x70UL		  // reg_zdec_irq_clear	(woc)
#define	REG_ZDEC_IRQ_FINAL_S 0x73UL		  // reg_zdec_irq_final_s
#define	ZDEC_IRQ_DECODE_DONE BIT(0)
#define	ZDEC_IRQ_ADMA_TABLE_DONE BIT(2)
#define	ZDEC_IRQ_LAST_ADMA_TABLE_DONE BIT(3)
#define	ZDEC_IRQ_PRESET_DICTIONARY_LOAD_DONE  BIT(4)
#define	ZDEC_IRQ_ZDEC_EXCEPTION BIT(5)
#define	ZDEC_IRQ_ALL 0xEEFUL

//
// ZDEC	woc	registers
//

#define	REG_ZDEC_GLOBAL_WOC	0x01UL
#define	ZDEC_WOC_STOP BIT(0)  // reg_zdec_woc_stop
#define	ZDEC_WOC_FRAME_ST BIT(1)  // reg_zdec_woc_frame_st
#define	ZDEC_WOC_PARK_MIU BIT(2)  // reg_zdec_woc_park_miu
#define	ZDEC_VHT_SW_CTRL_WOC_EN BIT(3)  // reg_zdec_vht_sw_ctrl_woc_en
#define	ZDEC_VHT_SW_CTRL_WOC_DONE_CLR BIT(4)  // reg_zdec_vht_sw_ctrl_woc_done_clr
#define	ZDEC_OBUF_DATA_CNT_GET_WOC BIT(5)  // reg_zdec_obuf_data_cnt_get_woc
#define	ZDEC_DEBUG_WOC_TRIG BIT(6)  // reg_zdec_debug_woc_trig
#define	ZDEC_DIC_RIU_WOC BIT(7)  // reg_zdec_dic_riu_woc
#define	ZDEC_WOC_LOAD_DIC BIT(8)  // reg_zdec_woc_load_dic
#define	ZDEC_WOC_NXT_DST_TBL_VLD BIT(9)  // reg_zdec_woc_nxt_dst_tbl_vld
#define	ZDEC_WOC_SET_FCIE_SWPTR BIT(10) // reg_zdec_woc_set_fcie_swptr
#define	ZDEC_WOC_SET_CLOCK_ON BIT(11) // reg_zdec_woc_set_clock_on
#define	REG_ZDEC_IBUF_WOC_CMD_WRITE 0x0eUL
#define	ZDEC_IBUF_WOC_CMD_WRITE BIT(0)  // reg_zdec_ibuf_woc_cmd_write
#define	REG_ZDEC_WOC_NXT_SRC_TBL_VLD 0x45UL
#define	ZDEC_WOC_NXT_SRC_TBL_VLD BIT(0)  // reg_zdec_woc_nxt_src_tbl_vld


//
// ZDEC	writable registers
//

struct ZDEC_REGS {
	union {
		struct {
			unsigned long reg_zdec_sw_rst:1;
			unsigned long reg_zdec_sw_htg_en:1;
			unsigned long reg_zdec_vld_bypass_en:1;
			unsigned long reg_zdec_sw_block_type:2;
			unsigned long reg_zdec_arb_wait_rw_lat:1;
			unsigned long reg_zdec_w_priority:1;
			unsigned long reg_zdec_r_priority:1;
			unsigned long reg_zdec_zmem_priority:1;
			unsigned long reg_zdec_sram_sd_en:1;
			unsigned long reg_zdec_00_dummy:6;
		};
		unsigned long reg00;
	};

	union {
		struct {
			unsigned long reg_zdec_vld_st_bit_cnt:3;
			unsigned long reg_zdec_fcie_hw_talk:1;
			unsigned long reg_zdec_fcie_test_mode:1;
			unsigned long reg_zdec_fcie_test_adma_done:1;
			unsigned long reg_zdec_01_dummy:10;
		};
		unsigned long reg02;
	};

	union {
		struct {
			unsigned long reg_zdec_fcie_test_job_cnt:16;
		};
		unsigned long reg06;
	};

	union {
		struct {
			unsigned long reg_zdec_fcie_test_seg_cnt:16;
		};
		unsigned long reg07;
	};

	union {
		struct {
			unsigned long reg_zdec_ibuf_cmd_sadr_lo:16; // reg_zdec_ibuf_cmd_sadr[15:0]
		};
		unsigned long reg08;
	};

	union {
		struct {
			unsigned long reg_zdec_ibuf_cmd_sadr_hi:16; // reg_zdec_ibuf_cmd_sadr[31:16]
		};
		unsigned long reg09;
	};

	union {
		struct {
			unsigned long reg_zdec_ibuf_cmd_size_lo:16; // reg_zdec_ibuf_cmd_size[15:0]
		};
		unsigned long reg0a;
	};

	union {
		struct {
			unsigned long reg_zdec_ibuf_cmd_size_hi:16; // reg_zdec_ibuf_cmd_size[31:16]
		};
		unsigned long reg0b;
	};

	union {
		struct {
			unsigned long reg_zdec_ibuf_cmd_last:1;
			unsigned long reg_zdec_ibuf_cmd_miu_sel:2;
			unsigned long reg_zdec_ibuf_cmd_sadr_msb:2;
			unsigned long reg_zdec_0c_dummy:11;
		};
		unsigned long reg0c;
	};

	union {
		struct {
			unsigned long reg_zdec_early_done_thd:16;
		};
		unsigned long reg0f;
	};

	union {
		struct {
			unsigned long reg_zdec_ibuf_shift_bit:3;
			unsigned long reg_zdec_10_dummy0:1;
			unsigned long reg_zdec_ibuf_shift_byte:4;
			unsigned long reg_zdec_10_dummy1:8;
		};
		unsigned long reg10;
	};

	union {
		struct {
			unsigned long reg_zdec_ibuf_skip_words_cnt_lo:16;
			// reg_zdec_ibuf_skip_words_cnt[15:0]
		};
		unsigned long reg11;
	};

	union {
		struct {
			unsigned long reg_zdec_ibuf_skip_words_cnt_hi:16;
			// reg_zdec_ibuf_skip_words_cnt[31:16]
		};
		unsigned long reg12;
	};

	union {
		struct {
			unsigned long reg_zdec_13_dummy0:3;
			unsigned long reg_zdec_ibuf_burst_len:3;
			unsigned long reg_zdec_ibuf_reverse_bit:1;
			unsigned long reg_zdec_ibuf_reverse_bit_in_byte:1;
			unsigned long reg_zdec_ibuf_byte_swap_64:1;
			unsigned long reg_zdec_ibuf_byte_swap_32:1;
			unsigned long reg_zdec_ibuf_byte_swap_16:1;
			unsigned long reg_zdec_ibuf_byte_swap_8:1;
			unsigned long reg_zdec_ibuf_64to32_lsb_first:1;
			unsigned long reg_zdec_13_dummy1:3;
		};
		unsigned long reg13;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_write_upperbound_en:1;
			unsigned long reg_zdec_obuf_write_lowerbound_en:1;
			unsigned long reg_zdec_obuf_timeout_en:1;
			unsigned long reg_zdec_obuf_datacnt_en:1;
			unsigned long reg_zdec_obuf_burst_split:1;
			unsigned long reg_zdec_obuf_byte_swap_64:1;
			unsigned long reg_zdec_obuf_byte_swap_32:1;
			unsigned long reg_zdec_obuf_byte_swap_16:1;
			unsigned long reg_zdec_obuf_byte_swap_8:1;
			unsigned long reg_zdec_obuf_miu_sel:2;
			unsigned long reg_zdec_last_done_z_mode:1;
			unsigned long reg_zdec_last_done_z_force:1;
			unsigned long reg_zdec_obuf_burst_length:2;
			unsigned long reg_zdec_obuf_byte_write_en:1;
		};
		unsigned long reg15;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_write_upperbound_lo:16;
			// reg_zdec_obuf_write_upperbound[15:0]
		};
		unsigned long reg16;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_write_upperbound_hi:16;
			// reg_zdec_obuf_write_upperbound[31:16]
		};
		unsigned long reg17;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_write_lowerbound_lo:16;
			// reg_zdec_obuf_write_lowerbound[15:0]
		};
		unsigned long reg18;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_write_lowerbound_hi:16;
			// reg_zdec_obuf_write_lowerbound[31:16]
		};
		unsigned long reg19;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_sadr_lo:16;
			// reg_zdec_obuf_sadr[15:0]
		};
		unsigned long reg1a;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_sadr_hi:16;
			// reg_zdec_obuf_sadr[31:16]
		};
		unsigned long reg1b;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_len_lo:16;
			//	reg_zdec_obuf_len[15:0]
		};
		unsigned long reg1c;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_len_hi:16;
			//	reg_zdec_obuf_len[31:16]
		};
		unsigned long reg1d;
	};

	union {
		struct {
			unsigned long reg_zdec_obuf_timeout:5;
			unsigned long reg_zdec_obuf_cnt_no_wait_last_done_z:1;
			unsigned long reg_zdec_obuf_sadr_msb:2;
			unsigned long reg_zdec_1e_dummy:8;
		};
		unsigned long reg1e;
	};

	union {
		struct {
			unsigned long reg_zdec_vht_sw_ctrl_en:1;
			unsigned long reg_zdec_vht_sw_ctrl_htg_or_tbl_sel:1;
			unsigned long reg_zdec_vht_sw_ctrl_rw:1;
			unsigned long reg_zdec_vht_sw_ctrl_mode_sel:2;
			unsigned long reg_zdec_vht_sw_ctrl_min_code_sel:2;
			unsigned long reg_zdec_20_dummy0:1;
			// [read only] reg_zdec_vht_sw_ctrl_htg_done
			unsigned long reg_zdec_dec_cnt_sel:1;
			unsigned long reg_zdec_20_dummy1:7;
		};
		unsigned long reg20;
	};

	union {
		struct {
			unsigned long reg_zdec_vht_sw_ctrl_adr:9;
			unsigned long reg_zdec_21_dummy:7;
		};
		unsigned long reg21;
	};

	union {
		struct {
			unsigned long reg_zdec_vht_sw_ctrl_wd:16;
		};
		unsigned long reg22;
	};

	union {
		struct {
			unsigned long reg_zdec_block_header_error_stop:1;
			unsigned long reg_zdec_mini_code_error_stop:1;
			unsigned long reg_zdec_uncompressed_len_error_stop:1;
			unsigned long reg_zdec_distance_error_stop:1;
			unsigned long reg_zdec_literal_error_stop:1;
			unsigned long reg_zdec_table_error_stop:1;
			unsigned long reg_zdec_28_dummy:10;
		};
		unsigned long reg28;
	};

	union {
		struct {
			unsigned long reg_zdec_exception_mask:16;
		};
		unsigned long reg2a;
	};

	union {
		struct {
			unsigned long reg_zdec_zmem_saddr_msb:2;
			unsigned long reg_zdec_zmem_eaddr_msb:2;
			unsigned long reg_zdec_2f_dummy:12;
		};
		unsigned long reg2f;
	};

	union {
		struct {
			unsigned long reg_zdec_lzd_bypass_en:1;
			unsigned long reg_zdec_frame_done_sel:1;
			unsigned long reg_zdec_zsram_size:2;
			unsigned long reg_zdec_zmem_miu_sel:2;
			unsigned long reg_zdec_zmem_last_done_z_mode:1;
			unsigned long reg_zdec_zmem_last_done_z_force:1;
			unsigned long reg_zdec_zsram_size_en:1;
			unsigned long reg_zdec_frame_done_mux:2;
			unsigned long reg_zdec_zmem_off:1;
			unsigned long reg_zdec_zmem_burst_len:3;
			unsigned long reg_zdec_30_dummy:1;
		};
		unsigned long reg30;
	};

	union {
		struct {
			unsigned long reg_zdec_tot_bytes_lo:16; // reg_zdec_tot_bytes[15:0]
		};
		unsigned long reg31;
	};

	union {
		struct {
			unsigned long reg_zdec_tot_bytes_hi:16; // reg_zdec_tot_bytes[31:16]
		};
		unsigned long reg32;
	};

	union {
		struct {
			unsigned long reg_zdec_zmem_saddr_lo:16; // reg_zdec_zmem_saddr[15:0]
		};
		unsigned long reg33;
	};

	union {
		struct {
			unsigned long reg_zdec_zmem_saddr_hi:16; // reg_zdec_zmem_saddr[31:16]
		};
		unsigned long reg34;
	};

	union {
		struct {
			unsigned long reg_zdec_zmem_eaddr_lo:16; // reg_zdec_zmem_eaddr[15:0]
		};
		unsigned long reg35;
	};

	union {
		struct {
			unsigned long reg_zdec_zmem_eaddr_hi:16; // reg_zdec_zmem_eaddr[31:16]
		};
		unsigned long reg36;
	};

	union {
		struct {
			unsigned long reg_zdec_mreq_sub_control:16; // reg_zdec_zmem_eaddr[31:16]
		};
		unsigned long reg38;
	};

	union {
		struct {
			unsigned long reg_zdec_preset_dic_bytes:16;
		};
		unsigned long reg39;
	};

	union {
		struct {
			unsigned long reg_zdec_src_tbl_addr_msb:2;
			unsigned long reg_zdec_dst_tbl_addr_msb:2;
			unsigned long reg_zdec_3f_dummy:12;
		};
		unsigned long reg3f;
	};

	union {
		struct {
			unsigned long reg_zdec_iobuf_mode:3;
			unsigned long reg_zdec_tbl_format:1;
			unsigned long reg_zdec_src_tbl_miu_sel:2;
			unsigned long reg_zdec_dst_tbl_miu_sel:2;
			unsigned long reg_zdec_src_tbl_last:1;
			unsigned long reg_zdec_40_dummy:7;
		};
		unsigned long reg40;
	};

	union {
		struct {
			unsigned long reg_zdec_src_tbl_addr_lo:16; //	reg_zdec_src_tbl_addr[15:0]
		};
		unsigned long reg41;
	};

	union {
		struct {
			unsigned long reg_zdec_src_tbl_addr_hi:16; //	reg_zdec_src_tbl_addr[31:16]
		};
		unsigned long reg42;
	};

	union {
		struct {
			unsigned long reg_zdec_dst_tbl_addr_lo:16; //	reg_zdec_dst_tbl_addr[15:0]
		};
		unsigned long reg43;
	};

	union {
		struct {
			unsigned long reg_zdec_dst_tbl_addr_hi:16; //	reg_zdec_dst_tbl_addr[31:16]
		};
		unsigned long reg44;
	};

	union {
		struct {
			unsigned long reg_zdec_page_size:4;
			unsigned long reg_zdec_out_page_size:4;
			unsigned long reg_segwptr_delta:4;
			unsigned long reg_jobwptr_delta:4;
		};
		unsigned long reg46;
	};

	union {
		struct {
			unsigned long reg_acp_awid_strgy:1;
			unsigned long reg_acp_eff_utilize_bl_en:1;
			unsigned long reg_acp_outstand_r:1;
			unsigned long reg_acp_outstand_w:1;
			unsigned long reg_arcache:4;
			unsigned long reg_arprot:3;
			unsigned long reg_aruser:2;
			unsigned long reg_zdec_47_dummy:3;

		};
		unsigned long reg47;
	};

	union {
		struct {
			unsigned long reg_awcache:4;
			unsigned long reg_awprot:3;
			unsigned long reg_awuser:8;
			unsigned long reg_axprot_lock:1;
		};
		unsigned long reg48;
	};

	union {
		struct {
			unsigned long reg_rburst_len:4;
			unsigned long reg_wburst_len:4;
			unsigned long reg_zdec_49_dummy:8;
		};
		unsigned long reg49;
	};

	union {
		struct {
			unsigned long reg_zdec_debug_en:1;
			unsigned long reg_zdec_debug_clk_sel:2;
			unsigned long reg_zdec_debug_trig_mode:4;
			unsigned long reg_zdec_50_dummy:9;
		};
		unsigned long reg50;
	};

	union {
		struct {
			unsigned long reg_zdec_debug_bus_sel:8;
			unsigned long reg_zdec_dbg_cnt_sel:4;
			unsigned long reg_zdec_51_dummy:4;
		};
		unsigned long reg51;
	};

	union {
		struct {
			unsigned long reg_zdec_debug_trig_cnt:16;
		};
		unsigned long reg54;
	};

	union {
		struct {
			unsigned long reg_zdec_dic_riu_wd:8;
			unsigned long reg_zdec_dic_riu_rw:1;
			unsigned long reg_zdec_58_dummy:7;
		};
		unsigned long reg58;
	};

	union {
		struct {
			unsigned long reg_zdec_dic_riu_addr:15;
			unsigned long reg_zdec_59_dummy:1;
		};
		unsigned long reg59;
	};

	union {
		struct {
			unsigned long reg_zdec_dic_riu_sel:1;
			unsigned long reg_zdec_5a_dummy:15;
		};
		unsigned long reg5a;
	};

	union {
		struct {
			unsigned long reg_riu2zdecdic_ns:6;
			unsigned long reg_riu2zdecibufr_ns:6;
			unsigned long reg_zdec_61_dummy:4;
		};
		unsigned long reg61;
	};

	union {
		struct {
			unsigned long reg_acpr_enable:1;
			unsigned long reg_acpw_enable:1;
			unsigned long reg_acp_uitilization_sel:2;
			unsigned long reg_acp_efficiency_sel:2;
			unsigned long reg_zdec_62_dummy:10;
		};
		unsigned long reg62;
	};

	union {
		struct {
			unsigned long reg_zdec_crc_en:1;
			unsigned long reg_zdec_crc_mode:2;
			unsigned long reg_zdec_crc_sel:1;
			unsigned long reg_zdec_68_dummy:12;
		};
		unsigned long reg68;
	};

	union {
		struct {
			unsigned long reg_zdec_irq_force:12;
			unsigned long reg_zdec_71_dummy:4;
		};
		unsigned long reg71;
	};

	union {
		struct {
			unsigned long reg_zdec_irq_mask:12;
			unsigned long reg_zdec_72_dummy:4;
		};
		unsigned long reg72;
	};

	union {
		struct {
			unsigned long  reg_zdec_irq_status_sel:1;
			unsigned long  reg_zdec_75_dummy:15;
		};
		unsigned long reg75;
	};

	union {
		struct {
			unsigned long reg_zdec_spare00:16;
		};
		unsigned long reg78;
	};

	union {
		struct {
			unsigned long reg_zdec_spare01:16;
		};
		unsigned long reg79;
	};

	union {
		struct {
			unsigned long reg_zdec_spare02:16;
		};
		unsigned long reg7a;
	};

	union {
		struct {
			unsigned long reg_zdec_spare03:16;
		};
		unsigned long reg7b;
	};

	union {
		struct {
			unsigned long reg_zdec_spare04:16;
		};
		unsigned long reg7c;
	};

	union {
		struct {
			unsigned long reg_zdec_spare05:16;
		};
		unsigned long reg7d;
	};

	union {
		struct {
			unsigned long reg_zdec_spare06:16;
		};
		unsigned long reg7e;
	};

	union {
		struct {
			unsigned long reg_zdec_spare07:16;
		};
		unsigned long reg7f;
	};
};


#endif

