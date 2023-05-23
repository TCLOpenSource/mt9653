/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */

#ifndef MZC_DRV_H
#define MZC_DRV_H
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/io.h>
//#define ENABLE_SINGLE_MODE

#define _BIT(x)	(1<<(x))
/*bank and irq */

#define SINGLE_MODE 0

#define CMDQ_MODE 1

#define CMDQ_LENC_NORMAL_END 1

#define CMDQ_LDEC_NORMAL_END 1

#define RESET 0

#define NOT_RESET 1

#define NON_SECURE_ACCESS 0x3

#define SECURE_ACCESS 0x0

#define MASK_IRQ_ALL 0xffff
#define LDEC_CMDQ_IRQ_IN_NEAR_EMPTY     _BIT(0)
#define LDEC_CMDQ_IRQ_IN_NEAR_FULL      _BIT(1)
#define LDEC_CMDQ_IRQ_IN_OVERFLOW       _BIT(2)
#define LDEC_CMDQ_IRQ_OUT_NEAR_EMPTY    _BIT(3)
#define LDEC_CMDQ_IRQ_OUT_NEAR_FULL     _BIT(4)
#define LDEC_CMDQ_IRQ_OUT_UNDERFLOW     _BIT(5)
#define LENC_CMDQ_IRQ_IN_NEAR_EMPTY     _BIT(6)
#define LENC_CMDQ_IRQ_IN_NEAR_FULL      _BIT(7)
#define LENC_CMDQ_IRQ_IN_OVERFLOW       _BIT(8)
#define LENC_CMDQ_IRQ_OUT_NEAR_EMPTY    _BIT(9)
#define LENC_CMDQ_IRQ_OUT_NEAR_FULL     _BIT(10)
#define LENC_CMDQ_IRQ_OUT_UNDERFLOW     _BIT(11)
#define LDEC_CMDQ_IRQ_TIMER				_BIT(12)
#define LDEC_CMDQ_IRQ_NUMBER_THRESHOLD  _BIT(13)
#define LENC_CMDQ_IRQ_TIMER				_BIT(14)
#define LENC_CMDQ_IRQ_NUMBER_THRESHOLD  _BIT(15)
#define LDEC_IRQ_NORMAL_END             _BIT(16-16)
#define LDEC_IRQ_CABAC_ERROR            _BIT(17-16)
#define LDEC_IRQ_SIZE_ERROR             _BIT(18-16)
#define LDEC_IRQ_DICT_ERROR             _BIT(19-16)
#define LDEC_IRQ_TIMEOUT                _BIT(20-16)
#define LDEC_IRQ_PKT_MARKER_ERROR       _BIT(21-16)
#define LENC_IRQ_NORMAL_END             _BIT(22-16)
#define LENC_IRQ_SIZE_ERROR             _BIT(23-16)
#define LENC_IRQ_TIMEOUT                _BIT(24-16)
#define ACP_RRESP_ERROR					_BIT(25-16)
#define ACP_BRESP_ERROR					_BIT(26-16)
#define LDEC_WRITE_INVALID_ERROR		_BIT(27-16)
#define LENC_WRITE_INVALID_ERROR		_BIT(28-16)


#define PAGE_MASK 0xffffff000
#define BIT_MASK_32 0xffffffff
#define BIT_MASK_33_34 0xf00000000
#define BIT_MASK_24 0xffffff
#define SHIFT_4 4
#define SHIFT_8 8
#define SHITF_12 12
#define SHITF_24 24
#define SHITF_28 28
#define SHITF_35 35

#define SINGLE_MODE_LENC_IRQ  \
	(LENC_IRQ_NORMAL_END | LENC_IRQ_SIZE_ERROR | LENC_IRQ_TIMEOUT)

#define MASK_IRQ_ALL_EXCEPT_TIMER_AND_NUMBER \
	(MASK_IRQ_ALL & \
	~(LDEC_CMDQ_IRQ_TIMER \
	| LDEC_CMDQ_IRQ_NUMBER_THRESHOLD \
	| LENC_CMDQ_IRQ_TIMER \
	| LENC_CMDQ_IRQ_NUMBER_THRESHOLD))

#define SINGLE_MODE_LDEC_IRQ \
	(LDEC_IRQ_NORMAL_END \
	| LDEC_IRQ_CABAC_ERROR \
	| LDEC_IRQ_SIZE_ERROR \
	| LDEC_IRQ_DICT_ERROR \
	| LDEC_IRQ_TIMEOUT \
	| LDEC_IRQ_PKT_MARKER_ERROR)

#define MASK_SINGLE_MODE_IRQ \
	(~(SINGLE_MODE_LENC_IRQ | SINGLE_MODE_LDEC_IRQ))

#define CLEAR_IRQ(irq)              (irq)

#define LENC_CMDQ_TIMER_THR_LO 0xffff

#define LENC_CMDQ_TIMER_THR_HI 0xffff

#define LDEC_CMDQ_TIMER_THR_LO 0xffff

#define LDEC_CMDQ_TIMER_THR_HI 0xffff

#define LENC_TIMEOUT_THRESHOLD (0xfffff)

#define LDEC_TIMEOUT_THRESHOLD (0xfffff)

#define LENC_CRC_DEFAULT_MODE 1

#define LENC_CRC_DEFAULT_ENABLE_STATE 1

#define LENC_OUTPUT_THRESHOLD 0

#define LDEC_CRC_DEFAULT_MODE 0	// 0: input, 1: output

#define LDEC_CRC_DEFAULT_ENABLE_STATE 0

#define LDEC_OUTPUT_THRESHOLD 0

#define CMDQ_LENC_SIZE_ERROR 0x2

#define CMDQ_LDEC_ERROR_TYPE_BIT_POS \
	((LDEC_IRQ_CABAC_ERROR) \
	| (LDEC_IRQ_SIZE_ERROR) \
	| (LDEC_IRQ_DICT_ERROR) \
	| (LDEC_IRQ_TIMEOUT) \
	| (LDEC_IRQ_PKT_MARKER_ERROR))

#define LENC_TIMEOUT (LENC_IRQ_TIMEOUT)

#define LDEC_TIMEOUT (LDEC_IRQ_TIMEOUT)

#define ACP_IDLE_SW_FORCE 0xdfff

#define MZC_FREQ_REGISTER  0x19

#define MZC_FULL_SPEED 0x0

#define MZC_CLCK_FIRE 0x80

#define DFS_ENABLE_ADDR 0x6a

//above is register operation

struct _mzc_reg {
	union {
		union {
			struct {
				unsigned short reg_w1c_mzc_start:1;
				unsigned short reg_mzc_soft_rstz:1;
				unsigned short reg_ldec_soft_rstz:1;
				unsigned short reg_lenc_soft_rstz:1;
				unsigned short reg_mzc_ver_rid:12;
			};
			unsigned short reg00;
		};

		union {
			struct {
				unsigned short reg_mzc_pkt_start_marker:8;
				unsigned short reg_mzc_core_rstz_cnt:5;
				unsigned short reg_mzc_acp_rstz_cnt:3;
			};
			unsigned short reg01;
		};

		union {
			struct {
				unsigned short reg_ldec_clk_gate_en:1;
				unsigned short reg_lenc_clk_gate_en:1;
				unsigned short reg_mzc_reset_fsm_idle:1;
				unsigned short reg_mzc_fullspeed_mode:1;
				unsigned short reg_mzc_fullspeed_sw_en:1;
			unsigned short reg_mzc_v15_mode_en:1};
			unsigned short reg02;
		};

		union {
			struct {
				unsigned short reg_mzc_ext_addr:4;
			};
			unsigned short reg03;
		};

		union {
			struct {
				unsigned short reg_ldec_inq_near_empty_th:4;
				unsigned short reg_ldec_inq_near_full_th:4;
				unsigned short reg_ldec_outq_near_empty_th:4;
				unsigned short reg_ldec_outq_near_full_th:4;
			};
			unsigned short reg04;
		};

		union {
			struct {
				unsigned short reg_lenc_inq_near_empty_th:4;
				unsigned short reg_lenc_inq_near_full_th:4;
				unsigned short reg_lenc_outq_near_empty_th:4;
				unsigned short reg_lenc_outq_near_full_th:4;
			};
			unsigned short reg05;
		};

		union {
			struct {
				unsigned short reg_ro_ldec_inq_lvl:5;
				unsigned short reg_reg06_reserved:3;
				unsigned short reg_ro_ldec_outq_lvl:5;
			};
			unsigned short reg06;
		};

		union {
			struct {
				unsigned short reg_ro_lenc_inq_lvl:5;
				unsigned short reg_reg07_reserved:3;
				unsigned short reg_ro_lenc_outq_lvl:5;
			};
			unsigned short reg07;
		};

		union {
			struct {
				unsigned short reg_ldec_inq_in_st_adr_lsb_lo:16;
			};
			unsigned short reg08;
		};

		union {
			struct {
				unsigned short reg_ldec_inq_in_st_adr_lsb_hi:16;
			};
			unsigned short reg09;
		};

		union {
			struct {
				unsigned short reg_ldec_inq_id:4;
				unsigned short reg_ldec_inq_in_st_adr_msb:4;
				unsigned short reg_ldec_inq_out_st_adr_lo:8;
			};
			unsigned short reg0a;
		};
		union {
			struct {
				unsigned short reg_ldec_inq_out_st_adr_hi:16;
			};
			unsigned short reg0b;
		};

		union {
			struct {
				unsigned short reg_ldec_outq_crc_lo:16;
			};
			unsigned short reg0c;
		};

		union {
			struct {
				unsigned short reg_ldec_outq_crc_hi:16;
			};
			unsigned short reg0d;
		};

		union {
			struct {
				unsigned short reg_ldec_outq_info:10;
			};
			unsigned short reg0e;
		};

		union {
			struct {
				unsigned short reg_lenc_inq_id:4;
				unsigned short reg_lenc_inq_out_st_adr_msb:1;
				unsigned short reg_reg10_reserved:3;
				unsigned short reg_lenc_inq_in_st_adr_lo:8;
			};
			unsigned short reg10;
		};

		union {
			struct {
				unsigned short reg_lenc_inq_in_st_adr_hi:16;
			};
			unsigned short reg11;
		};
		union {
			struct {
				unsigned short
					reg_lenc_inq_out_st_adr_lsb_lo:16;
			};
			unsigned short reg12;
		};
		union {
			struct {
				unsigned short
					reg_lenc_inq_out_st_adr_lsb_hi:16;
			};
			unsigned short reg13;
		};
		union {
			struct {
				unsigned short reg_lenc_outq_crc_lo:16;
			};
			unsigned short reg14;
		};

		union {
			struct {
				unsigned short reg_lenc_outq_crc_hi:16;
			};
			unsigned short reg15;
		};

		union {
			struct {
				unsigned short reg_lenc_outq_info_lo:16;
			};
			unsigned short reg16;
		};
		union {
			struct {
				unsigned short reg_lenc_outq_info_hi:4;
			};
			unsigned short reg17;
		};
		union {
			struct {
				unsigned short reg_ldec_outq_num_th:4;
				unsigned short reg_lenc_outq_num_th:4;
			};
			unsigned short reg18;
		};
		union {
			struct {
				unsigned short reg_ldec_outq_timer_th_lo:16;
			};
			unsigned short reg19;
		};
		union {
			struct {
				unsigned short reg_ldec_outq_timer_th_hi:16;
			};
			unsigned short reg1a;
		};
		union {
			struct {
				unsigned short reg_lenc_outq_timer_th_lo:16;
			};
			unsigned short reg1b;
		};
		union {
			struct {
				unsigned short reg_lenc_outq_timer_th_hi:16;
			};
			unsigned short reg1c;
		};
		union {
			struct {
				unsigned short reg_ldec_op_mode:1;
				unsigned short reg_reg1d_reserved:6;
				unsigned short reg_w1c_ldec_start:1;
				unsigned short reg_ldec_bdma_burst_sel:2;
				unsigned short reg_ldec_odma_burst_sel:2;
				unsigned short reg_ldec_crc_en:1;
				unsigned short reg_ldec_crc_mode:1;
				unsigned short reg_ldec_crc_round_en:1;
				unsigned short reg_ldec_crc_per_byte:1;
			};
			unsigned short reg1d;
		};
		union {
			struct {
				unsigned short reg_ldec_in_st_adr_lsb_lo:16;
			};
			unsigned short reg1e;
		};
		union {
			struct {
				unsigned short reg_ldec_in_st_adr_lsb_hi:16;
			};
			unsigned short reg1f;
		};
		union {
			struct {
				unsigned short reg_ldec_out_st_adr_lo:16;
			};
			unsigned short reg20;
		};
		union {
			struct {
				unsigned short reg_ldec_out_st_adr_hi:8;
				unsigned short reg_ldec_in_st_adr_msb:4;
			};
			unsigned short reg21;
		};
		union {
			struct {
				unsigned short reg_ldec_out_size_thr:13;
			};
			unsigned short reg22;
		};
		union {
			struct {
				unsigned short reg_ldec_timeout_thr_lo:16;
			};
			unsigned short reg23;
		};
		union {
			struct {
				unsigned short reg_ldec_timeout_thr_hi:4;
				unsigned short reg_ldec_timeout_en:1;
				unsigned short reg_ldec_cnt_sel:1;
				unsigned short reg_ldec_cnt_sel_1:3;
			};
			unsigned short reg24;
		};
		union {
			struct {
				unsigned short reg_ldec_crc_round_cnt:12;
			};
			unsigned short reg25;
		};
		union {
			struct {
				unsigned short reg_ro_ldec_debug_cyc_lo:16;
			};
			unsigned short reg26;
		};
		union {
			struct {
				unsigned short reg_ro_ldec_debug_cyc_hi:16;
			};
			unsigned short reg27;
		};
		union {
			struct {
				unsigned short reg_ro_ldec_crc_out_lo:16;
			};
			unsigned short reg28;
		};
		union {
			struct {
				unsigned short reg_ro_ldec_crc_out_hi:16;
			};
			unsigned short reg29;
		};
		union {
			struct {
				unsigned short
					reg_ldec_inq_in_st_adr2_4k_lsb:16;
			};
			unsigned short reg2a;
		};
		union {
			struct {
				unsigned short reg_ldec_inq_in_st_adr2_4k_msb:8;
			unsigned short reg_ldec_inq_in_st_adr2_en:1};
			unsigned short reg2b;
		};
		union {
			struct {
				unsigned short reg_ro_ldec_single_cyc_hi:4;
			};
			unsigned short reg2c;
		};
		union {
			struct {
				unsigned short reg_ro_ldec_inq_cur_ptr:4;
				unsigned short reg_ldec_inq_read_ptr:4;
				unsigned short reg_ldec_inq_read_sel:2;
			};
			unsigned short reg2d;
		};
		union {
			struct {
				unsigned short reg_ro_ldec_inq_debug_lo:16;
			};
			unsigned short reg2e;
		};
		union {
			struct {
				unsigned short reg_ro_ldec_inq_debug_hi:16;
			};
			unsigned short reg2f;
		};
		union {
			struct {
				unsigned short reg_mzc_irq_mask_lo:16;
			};
			unsigned short reg30;
		};
		union {
			struct {
				unsigned short reg_mzc_irq_mask_hi:16;
			};
			unsigned short reg31;
		};
		union {
			struct {
				unsigned short reg_mzc_irq_force_lo:16;
			};
			unsigned short reg32;
		};
		union {
			struct {
				unsigned short reg_mzc_irq_force_hi:16;
			};
			unsigned short reg33;
		};
		union {
			struct {
				unsigned short reg_mzc_irq_clr0:1;
				unsigned short reg_mzc_irq_clr1:1;
				unsigned short reg_mzc_irq_clr2:1;
				unsigned short reg_mzc_irq_clr3:1;
				unsigned short reg_mzc_irq_clr4:1;
				unsigned short reg_mzc_irq_clr5:1;
				unsigned short reg_mzc_irq_clr6:1;
				unsigned short reg_mzc_irq_clr7:1;
				unsigned short reg_mzc_irq_clr8:1;
				unsigned short reg_mzc_irq_clr9:1;
				unsigned short reg_mzc_irq_clr10:1;
				unsigned short reg_mzc_irq_clr11:1;
				unsigned short reg_mzc_irq_clr12:1;
				unsigned short reg_mzc_irq_clr13:1;
				unsigned short reg_mzc_irq_clr14:1;
				unsigned short reg_mzc_irq_clr15:1;
			};
			unsigned short reg34;
		};
		union {
			struct {
				unsigned short reg_mzc_irq_clr16:1;
				unsigned short reg_mzc_irq_clr17:1;
				unsigned short reg_mzc_irq_clr18:1;
				unsigned short reg_mzc_irq_clr19:1;
				unsigned short reg_mzc_irq_clr20:1;
				unsigned short reg_mzc_irq_clr21:1;
				unsigned short reg_mzc_irq_clr22:1;
				unsigned short reg_mzc_irq_clr23:1;
				unsigned short reg_mzc_irq_clr24:1;
				unsigned short reg_mzc_irq_clr25:1;
				unsigned short reg_mzc_irq_clr26:1;
				unsigned short reg_mzc_irq_clr27:1;
				unsigned short reg_mzc_irq_clr28:1;
				unsigned short reg_mzc_irq_clr29:1;
				unsigned short reg_mzc_irq_clr30:1;
				unsigned short reg_mzc_irq_clr31:1;
			};
			unsigned short reg35;
		};
		union {
			struct {
				unsigned short reg_mzc_st_irq_cpu_lo:16;
			};
			unsigned short reg36;
		};
		union {
			struct {
				unsigned short reg_mzc_st_irq_cpu_hi:16;
			};
			unsigned short reg37;
		};
		union {
			struct {
				unsigned short reg_mzc_st_irq_ip_lo:16;
			};
			unsigned short reg38;
		};
		union {
			struct {
				unsigned short reg_mzc_st_irq_ip_hi:16;
			};
			unsigned short reg39;
		};
		union {
			struct {
				unsigned short
					reg_ldec_out_lower_bound_adr_lo:16;
			};
			unsigned short reg3a;
		};
		union {
			struct {
				unsigned short
					reg_ldec_out_lower_bound_adr_hi:16;
			};
			unsigned short reg3b;
		};
		union {
			struct {
				unsigned short
					reg_ldec_out_upper_bound_adr_lo:16;
			};
			unsigned short reg3c;
		};
		union {
			struct {
				unsigned short
					reg_ldec_out_upper_bound_adr_hi:16;
			};
			unsigned short reg3d;
		};
		union {
			struct {
				unsigned short reg_lenc_op_mode:1;
				unsigned short reg_lenc_auto_rstz_cnt:6;
				unsigned short reg_w1c_lenc_start:1;
				unsigned short reg_lenc_rd_burst_sel:2;
				unsigned short reg_lenc_wr_burst_sel:2;
				unsigned short reg_lenc_crc_en:1;
				unsigned short reg_lenc_crc_mode:1;
				unsigned short reg_lenc_crc_round_en:1;
			};
			unsigned short reg40;
		};
		union {
			struct {
				unsigned short reg_lenc_in_st_adr_lo:16;
			};
			unsigned short reg41;
		};
		union {
			struct {
				unsigned short reg_lenc_in_st_adr_hi:8;
				unsigned short reg_lenc_out_st_adr_msb:1;
			};
			unsigned short reg42;
		};
		union {
			struct {
				unsigned short reg_lenc_out_st_adr_lsb_lo:16;
			};
			unsigned short reg43;
		};
		union {
			struct {
				unsigned short reg_lenc_out_st_adr_lsb_hi:16;
			};
			unsigned short reg44;
		};
		union {
			struct {
				unsigned short reg_reg45_reserved:4;
				unsigned short reg_lenc_out_size_thr:9;
			};
			unsigned short reg45;
		};
		union {
			struct {
				unsigned short reg_lenc_timeout_thr_lo:16;
			};
			unsigned short reg46;
		};
		union {
			struct {
				unsigned short reg_lenc_timeout_thr_hi:4;
				unsigned short reg_lenc_timeout_en:1;
				unsigned short reg_lenc_cnt_sel:1;
				unsigned short reg_lenc_cnt_sel_1:3;
			};
			unsigned short reg47;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_crc_round_cnt:12;
				unsigned short reg_lenc_dic_cache_disable:1;
				unsigned short reg_lenc_chain_cache_disable:1;
				unsigned short reg_lenc_only_park_inout_stage:1;
			};
			unsigned short reg48;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_active_cyc_hi:16;
			};
			unsigned short reg49;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_debug_cyc_lo:16;
			};
			unsigned short reg4a;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_debug_cyc_hi:16;
			};
			unsigned short reg4b;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_crc_out_lo:16;
			};
			unsigned short reg4c;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_crc_out_hi:16;
			};
			unsigned short reg4d;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_single_cyc_hi:4;
			};
			unsigned short reg4e;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_crc_out:16;
			};
			unsigned short reg4f;
		};
		union {
			struct {
				unsigned short reg_sync_lenc_hash_early_cnt:8;
				unsigned short reg_sync_lenc_hash_search_num:3;
				unsigned short reg_sync_lenc_reps_full_search:1;
				unsigned short reg_sync_lenc_lit_bypass_bins:2;
			};
			unsigned short reg50;
		};
		union {
			struct {
				unsigned short reg_lenc_hash_prime_val:16;
			};
			unsigned short reg51;
		};
		union {
			struct {
				unsigned short reg_lenc_lower_addr_lo:16;
			};
			unsigned short reg52;
		};
		union {
			struct {
				unsigned short reg_lenc_lower_addr_hi:16;
			};
			unsigned short reg53;
		};
		union {
			struct {
				unsigned short reg_lenc_upper_addr_lo:16;
			};
			unsigned short reg54;
		};
		union {
			struct {
				unsigned short reg_lenc_upper_addr_hi:16;
			};
			unsigned short reg55;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_byte_cnt:13;
			};
			unsigned short reg56;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_inq_cur_ptr:4;
				unsigned short reg_lenc_inq_read_ptr:4;
				unsigned short reg_lenc_inq_read_sel:1;
			};
			unsigned short reg57;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_inq_debug_lo:16;
			};
			unsigned short reg58;
		};
		union {
			struct {
				unsigned short reg_ro_lenc_inq_debug_hi:16;
			};
			unsigned short reg59;
		};
		union {
			struct {
				unsigned short reg_acp_outstand:1;
			};
			unsigned short reg60;
		};
		union {
			struct {
				unsigned short reg_arcache:4;
				unsigned short reg_arprot:3;
				unsigned short reg_aruser:2;
			};
			unsigned short reg61;
		};
		union {
			struct {
				unsigned short reg_awcache:4;
				unsigned short reg_awprot:3;
				unsigned short reg_awuser:2;
			};
			unsigned short reg62;
		};
		union {
			struct {
				unsigned short reg_acp_eff_utilize_bl_en:1;
			};
			unsigned short reg63;
		};
		union {
			struct {
				unsigned short reg_acp_arc_utilization_lo:16;
			};
			unsigned short reg64;
		};
		union {
			struct {
				unsigned short reg_acp_arc_utilization_hi:16;
			};
			unsigned short reg65;
		};
		union {
			struct {
				unsigned short reg_acp_arc_efficiency_lo:16;
			};
			unsigned short reg66;
		};
		union {
			struct {
				unsigned short reg_acp_arc_efficiency_hi:16;
			};
			unsigned short reg67;
		};
		union {
			struct {
				unsigned short reg_acp_rc_utilization_lo:16;
			};
			unsigned short reg68;
		};
		union {
			struct {
				unsigned short reg_acp_rc_utilization_hi:16;
			};
			unsigned short reg69;
		};
		union {
			struct {
				unsigned short reg_acp_rc_efficiency_lo:16;
			};
			unsigned short reg6a;
		};
		union {
			struct {
				unsigned short reg_acp_rc_efficiency_hi:16;
			};
			unsigned short reg6b;
		};
		union {
			struct {
				unsigned short reg_acp_awc_utilization_lo:16;
			};
			unsigned short reg6c;
		};
		union {
			struct {
				unsigned short reg_acp_awc_utilization_hi:16;
			};
			unsigned short reg6d;
		};
		union {
			struct {
				unsigned short reg_acp_awc_efficiency_lo:16;
			};
			unsigned short reg6e;
		};
		union {
			struct {
				unsigned short reg_acp_awc_efficiency_hi:16;
			};
			unsigned short reg6f;
		};
		union {
			struct {
				unsigned short reg_acp_wc_utilization_lo:16;
			};
			unsigned short reg70;
		};
		union {
			struct {
				unsigned short reg_acp_wc_utilization_hi:16;
			};
			unsigned short reg71;
		};
		union {
			struct {
				unsigned short reg_acp_wc_efficiency_lo:16;
			};
			unsigned short reg72;
		};
		union {
			struct {
				unsigned short reg_acp_wc_efficiency_hi:16;
			};
			unsigned short reg73;
		};
		union {
			struct {
				unsigned short reg_mzc_dbg_sel:8;
			};
			unsigned short reg78;
		};
		union {
			struct {
				unsigned short reg_mzc_dbg_bus_out_lo:16;
			};
			unsigned short reg79;
		};
		union {
			struct {
				unsigned short reg_mzc_dbg_bus_out_hi:16;
			};
			unsigned short reg7a;
		};
		union {
			struct {
				unsigned short reg_mzc_bist_fail:16;
			};
			unsigned short reg7b;
		};
		union {
			struct {
				unsigned short reg_mzc_rsv7c_reserved1:2;
				unsigned short reg_mzc_rsv7c_reserved2:1;
				unsigned short reg_mzc_rsv7c_reserved3:13;
			};
			unsigned short reg7c;
		};
		union {
			struct {
				unsigned short reg_mzc_rsv7d:16;
			};
			unsigned short reg7d;
		};
	};
};


void __mtk_dtv_mzc_cmdq_init(int enc_out_size_thr);

void __mtk_dtv_mzc_cmdq_exit(void);

int __mtk_dtv_lenc_cmdq_set(phys_addr_t in_addr,
	phys_addr_t out_addr, unsigned int process_id);

int __mtk_dtv_ldec_cmdq_set(phys_addr_t in_addr,
	phys_addr_t out_addr, unsigned int process_id);

int __mtk_dtv_ldec_cmdq_set_split(phys_addr_t in_addr1,
				phys_addr_t in_addr2,
				phys_addr_t out_addr,
				unsigned int process_id);

void __mtk_dtv_mzc_single_init(int enc_out_size_thr);

void __mtk_dtv_mzc_single_exit(void);

void __mtk_dtv_mzc_irq_mask_all_except_timeout(void);

void __mtk_dtv_mzc_irq_mask_all(void);

int __mtk_dtv_lenc_single_set(unsigned long in_addr,
				unsigned long out_addr,
				unsigned int process_id);

int __mtk_dtv_ldec_single_set(unsigned long in_addr,
				unsigned long out_addr,
				unsigned int process_id);

void __mtk_dtv_ldec_cycle_info(int *active_cycle,
		int *free_cycle, int *page_count);

void __mtk_dtv_lenc_cycle_info(int *active_cycle,
		int *free_cycle, int *page_count);

int __mtk_dtv_lenc_cmdq_done(int *target_id);

int __mtk_dtv_ldec_cmdq_done(int *target_id);

int __mtk_dtv_lenc_single_done(void);

int __mtk_dtv_ldec_single_done(void);

unsigned int __mtk_dtv_mzc_cmdq_handler(int *w_outputcount,
			int *r_outputcount,
			int *is_write,
			int *is_read);

void __mtk_dtv_mzc_cmdq_clear_irq(unsigned int irq_status);

void __mtk_dtv_lenc_cmdq_handler(int *outlen, int *process_id);

void __mtk_dtv_ldec_cmdq_handler(int *outlen, int *process_id);

void __mtk_dtv_mzc_single_handler(int *w_outlen,
		int *r_outlen, int *is_write, int *is_read);

void __mtk_dtv_regptr_set(void);

void __mtk_dtv_regptr_unset(void);

void __mtk_dtv_acp_common_init(void);

void __mtk_dtv_ldec_state_save(int wait);

void __mtk_dtv_lenc_state_save(int wait);

int __mtk_dtv_lenc_state_check(void);

int __mtk_dtv_ldec_state_check(void);

unsigned long __mtk_dtv_lenc_crc_get(void);

void __mtk_dtv_ldec_literal_bypass_set(int value);


struct cmdq_node {
	unsigned long out_len;
};

int mtk_dtv_mzc_cmdq_init(void);

void mtk_dtv_mzc_cmdq_exit(void);

int mtk_dtv_lenc_cmdq_run(phys_addr_t in_addr,
	phys_addr_t out_addr, unsigned int *dst_len);


int mtk_dtv_ldec_cmdq_run(phys_addr_t in_addr, phys_addr_t out_addr);

void mtk_dtv_ldec_literal_bypass_set(int value);

int mtk_dtv_ldec_cmdq_run_split(phys_addr_t in_addr1,
			phys_addr_t in_addr2,
			phys_addr_t out_addr);

int mtk_dtv_mzc_single_init(void);

void mtk_dtv_mzc_single_exit(void);


int mtk_dtv_lenc_single_run(unsigned long in_addr,
	unsigned long out_addr, unsigned int *dst_len);


int mtk_dtv_ldec_single_run(unsigned long in_addr, unsigned long out_addr);

void mtk_dtv_ldec_cycle_info(void);

void mtk_dtv_lenc_cycle_info(void);


u32 mtk_dtv_lenc_crc32_get(unsigned char *cmem, unsigned int clen);



int mtk_dtv_lenc_hybrid_cmdq_run(phys_addr_t in_addr, phys_addr_t out_addr,
				 unsigned int *dst_len);



#define CMDQ_ELEMENT_NUM	(1)


struct dev_mzc {
	unsigned int ldec_single_id;
	unsigned int lenc_single_id;
	struct cmdq_node write_cmdq[CMDQ_ELEMENT_NUM];
	struct cmdq_node read_cmdq[CMDQ_ELEMENT_NUM];
	spinlock_t r_lock;
	spinlock_t w_lock;
};

enum {
	MZC_ERR_NO_ENC_RESOURCE = -1000,
};

#define INITIAL_VALUE -1
enum {
	LZO = 0,
	MZC,
	NR_COMP_ALGO,
};

#endif
