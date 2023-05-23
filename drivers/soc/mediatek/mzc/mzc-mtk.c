// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "mtk-mzc.h"
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/of.h>
#include <uapi/linux/psci.h>
#include <linux/clk.h>
#ifdef CONFIG_ARM
#include <linux/mm.h>
#include <linux/highmem.h>
#endif

/* The value is for zram to reduce wastage due to unusable space
 * left at end of each zspage
 */
#define MAX_SIZE_FOR_ZRAM	3248
int is_new_mzc;
EXPORT_SYMBOL(is_new_mzc);

int is_secure;
enum MZC_REGS_OFFSET {

	CMDQ_TOP_SETTING = 0,

	CMDQ_TOP_SETTING_002 = 0x02,

	RO_LDEC_OUT_INQ_LV1 = 0x06,

	RO_LENC_OUT_INQ_LV1 = 0x07,

	LDEC_INQ_IN_ST_ADR_LSB_LO = 0x08,

	LDEC_INQ_DATA = 0x0a,

	MZC_VERSION_SETTING = 0x0d,

	LDEC_OUTQ_INFO = 0x0e,

	LENC_INQ_DATA = 0x10,

	LENC_INQ_OUT_ST_ADR_LSB_LO = 0x12,

	LENC_CRC_OUT = 0x14,

	LENC_OUTQ_INFO_LO = 0x16,

	MZC_OUTQ_NUM_TH = 0x18,

	LDEC_OUTQ_TIMER_TH_LO = 0x19,

	LDEC_OUTQ_TIMER_TH_HI = 0x1a,

	LENC_OUTQ_TIMER_TH_LO = 0x1b,

	LENC_OUTQ_TIMER_TH_HI = 0x1c,

	LDEC_GENERAL_SETTING = 0x1d,

	LDEC_IN_ST_ADR_LSB_LO = 0x1e,

	LDEC_IN_ST_ADR_LSB_HI = 0x1f,

	LDEC_OUT_ST_ADR_LO = 0x20,

	LDEC_OUT_ST_ADR_HI = 0x21,

	LDEC_TIMEOUT_THR = 0x23,

	LDEC_TIMEOUT_EN = 0x24,

	RO_LDEC_CYC_INFO = 0x26,

	LDEC_IN_ST_ADR_4K = 0x2a,

	MZC_IRQ_MASK_HI = 0x31,

	MZC_IRQ_MASK_LO = 0x30,

	MZC_IRQ_STATUS_HI = 0x35,

	MZC_ST_IRQ_CPU_HI = 0x37,

	LENC_GENERAL_SETTING = 0x40,

	LENC_IN_ST_ADR = 0x41,

	LENC_OUT_ST_ADR_MSB = 0x42,

	LENC_OUT_ST_ADR_LSB_HI = 0x44,

	LENC_OUT_ST_ADR_LSB_LO = 0x43,

	LENC_OUTSIZE_THR = 0x45,

	LENC_TIMEOUT_THR = 0x46,

	LENC_TIMEOUT_EN = 0x47,

	RO_LENC_CYC_INFO = 0x4a,

	LENC_PAGE_SYNC = 0x50,

	RO_LENC_BYTE_CNT = 0x56,

	ACP_AR = 0x61,

	ACP_AW = 0x62,

	MZC_RSV7C = 0x7c,
};

struct mtk_dtv_mzc {
	struct device *dev;
	void __iomem *mzc_bank;
	void __iomem *acp_bank;
	void __iomem *freq_bank;
	void __iomem *mzc_clk_bank;
	void __iomem *acache_bank;
	void __iomem *l4_axi_bank;
	struct _mzc_reg comm_reg;
	struct dev_mzc empty_dev;
	unsigned long ldec_cmdq_info;
	unsigned long ldec_output_level;
	unsigned long lenc_cmdq_info;
	unsigned long lenc_output_level;
};

struct mtk_dtv_mzc *mzc_pdata;

//hal layer start


//extern ptrdiff_t mtk_pm_base;


static inline unsigned short MZC_REG_READ(unsigned short _offset)
{
	return readw(mzc_pdata->mzc_bank + ((_offset)*2));
}

static inline void MZC_REG_WRITE(unsigned short _offset, unsigned short val)
{
	writew(val, mzc_pdata->mzc_bank + ((_offset)*2));
}

static inline void MZC_REG_WRITE_32(unsigned short _offset, u32 val)
{
	writel(val, mzc_pdata->mzc_bank + ((_offset)*2));
}

static inline unsigned int MZC_REG_READ_32(unsigned short _offset)
{
	return readl(mzc_pdata->mzc_bank + ((_offset)*2));
}

static inline void ACP_REG_WRITE(unsigned short _offset, unsigned short val)
{
	writew(val, mzc_pdata->acp_bank + ((_offset)*4));
}

static inline unsigned short ACP_REG_READ(unsigned short _offset)
{
	return readw((mzc_pdata->acp_bank + ((_offset)*4)));
}

static inline void ACACHE_REG_WRITE(unsigned short _offset, unsigned short val)
{
	writew(val, (mzc_pdata->acache_bank + ((_offset)*4)));
}

static inline unsigned short ACACHE_REG_READ(unsigned short _offset)
{
	return readw((mzc_pdata->acache_bank + ((_offset)*4)));
}


void __mtk_dtv_acp_common_init(void)
{
	unsigned short orig_acp_info, acp_set, orig_acache_info, acache_set;

	orig_acp_info = ACP_REG_READ(0x10);
	acp_set = (orig_acp_info & 0xfffe);
	orig_acache_info = ACACHE_REG_READ(0x2b);
	acache_set = (orig_acache_info | 0xff);
	ACP_REG_WRITE(0x10, acp_set);
	ACACHE_REG_WRITE(0x2b, acache_set);
}

static void __mtk_dtv_mzc_common_init(int enc_out_size_thr)
{
	// clk & freq
	unsigned short value;
/*
	writew(MZC_FULL_SPEED,
		(mzc_pdata->freq_bank + ((MZC_FREQ_REGISTER)*4)));
	value = readw((mzc_pdata->mzc_clk_bank + ((MZC_FREQ_REGISTER)*4)));
	writew((value | MZC_CLCK_FIRE),
		(mzc_pdata->mzc_clk_bank + ((MZC_FREQ_REGISTER)*4)));
*/
	if (!is_secure) {
		mzc_pdata->comm_reg.reg61 = MZC_REG_READ(ACP_AR);
		mzc_pdata->comm_reg.reg_arprot = NON_SECURE_ACCESS;
		MZC_REG_WRITE(ACP_AR, mzc_pdata->comm_reg.reg61);
		mzc_pdata->comm_reg.reg62 = MZC_REG_READ(ACP_AW);
		mzc_pdata->comm_reg.reg_awprot = NON_SECURE_ACCESS;
		mzc_pdata->comm_reg.reg62 &= ACP_IDLE_SW_FORCE;
		MZC_REG_WRITE(ACP_AW, mzc_pdata->comm_reg.reg62);
	}	else {
		mzc_pdata->comm_reg.reg61 = MZC_REG_READ(ACP_AR);
		mzc_pdata->comm_reg.reg_arprot = SECURE_ACCESS;
		MZC_REG_WRITE(ACP_AR, mzc_pdata->comm_reg.reg61);
		mzc_pdata->comm_reg.reg62 = MZC_REG_READ(ACP_AW);
		mzc_pdata->comm_reg.reg_awprot = SECURE_ACCESS;
		mzc_pdata->comm_reg.reg62 &= ACP_IDLE_SW_FORCE;
		MZC_REG_WRITE(ACP_AW, mzc_pdata->comm_reg.reg62);
	}
	// top setting
	mzc_pdata->comm_reg.reg00 = MZC_REG_READ(CMDQ_TOP_SETTING);
	mzc_pdata->comm_reg.reg_mzc_soft_rstz = RESET; //set 0
	mzc_pdata->comm_reg.reg_ldec_soft_rstz = NOT_RESET;
	mzc_pdata->comm_reg.reg_lenc_soft_rstz = NOT_RESET;
	MZC_REG_WRITE(CMDQ_TOP_SETTING, mzc_pdata->comm_reg.reg00);
	mzc_pdata->comm_reg.reg_mzc_soft_rstz = NOT_RESET; //set 1
	mzc_pdata->comm_reg.reg_ldec_soft_rstz = NOT_RESET;
	mzc_pdata->comm_reg.reg_lenc_soft_rstz = NOT_RESET;
	MZC_REG_WRITE(CMDQ_TOP_SETTING, mzc_pdata->comm_reg.reg00);

	mzc_pdata->comm_reg.reg02 = MZC_REG_READ(CMDQ_TOP_SETTING_002);
	mzc_pdata->comm_reg.reg_ldec_clk_gate_en = 1;
	mzc_pdata->comm_reg.reg_lenc_clk_gate_en = 1;
	mzc_pdata->comm_reg.reg_mzc_v15_mode_en = 1;
	MZC_REG_WRITE(CMDQ_TOP_SETTING_002, mzc_pdata->comm_reg.reg02);

	mzc_pdata->comm_reg.reg45 = MZC_REG_READ(LENC_OUTSIZE_THR);
	mzc_pdata->comm_reg.reg_lenc_out_size_thr = enc_out_size_thr >> 4;
	MZC_REG_WRITE(LENC_OUTSIZE_THR, mzc_pdata->comm_reg.reg45);

	MZC_REG_WRITE(MZC_RSV7C, MZC_VERSION_SETTING);
}

static void __mtk_dtv_lenc_outputcount_read(int *outcount)
{
	*outcount = ((MZC_REG_READ(RO_LENC_OUT_INQ_LV1) >> 8) & 0x1f);
}


static void __mtk_dtv_ldec_outputcount_read(int *outcount)
{
	*outcount = ((MZC_REG_READ(RO_LDEC_OUT_INQ_LV1) >> 8) & 0x1f);
}


static void __mtk_dtv_lenc_cmdq_init(void)
{
	mzc_pdata->comm_reg.reg40 = MZC_REG_READ(LENC_GENERAL_SETTING);
	mzc_pdata->comm_reg.reg_lenc_op_mode = CMDQ_MODE;
	mzc_pdata->comm_reg.reg_lenc_crc_en
		= LENC_CRC_DEFAULT_ENABLE_STATE;  //set crc enable or disable
	mzc_pdata->comm_reg.reg_lenc_crc_mode
		= LENC_CRC_DEFAULT_MODE; //set crc mode
	mzc_pdata->comm_reg.reg_w1c_lenc_start = 1;
	MZC_REG_WRITE(LENC_GENERAL_SETTING, mzc_pdata->comm_reg.reg40);

	mzc_pdata->comm_reg.reg18 = MZC_REG_READ(MZC_OUTQ_NUM_TH);
	mzc_pdata->comm_reg.reg_lenc_outq_num_th
		= LENC_OUTPUT_THRESHOLD; //set output threshold
	MZC_REG_WRITE(MZC_OUTQ_NUM_TH, mzc_pdata->comm_reg.reg18);

	mzc_pdata->comm_reg.reg1b
		= MZC_REG_READ(LENC_OUTQ_TIMER_TH_LO);
	mzc_pdata->comm_reg.reg_lenc_outq_timer_th_lo
		=  LENC_CMDQ_TIMER_THR_LO; //set timer default
	MZC_REG_WRITE(LENC_OUTQ_TIMER_TH_LO, mzc_pdata->comm_reg.reg1b);


	mzc_pdata->comm_reg.reg1c
		= MZC_REG_READ(LENC_OUTQ_TIMER_TH_HI);
	mzc_pdata->comm_reg.reg_lenc_outq_timer_th_hi
		=  LENC_CMDQ_TIMER_THR_HI; //set timer default
	MZC_REG_WRITE(LENC_OUTQ_TIMER_TH_HI, mzc_pdata->comm_reg.reg1c);



	mzc_pdata->comm_reg.reg_lenc_timeout_thr_lo
		= LENC_TIMEOUT_THRESHOLD & 0xffff;
	MZC_REG_WRITE(LENC_TIMEOUT_THR, mzc_pdata->comm_reg.reg46);

	mzc_pdata->comm_reg.reg47
		= MZC_REG_READ(LENC_TIMEOUT_EN);
	mzc_pdata->comm_reg.reg_lenc_timeout_thr_hi
		= LENC_TIMEOUT_THRESHOLD >> 16;
	mzc_pdata->comm_reg.reg_lenc_cnt_sel
		= 0; // 0: wrap-around, 1: staurated
	MZC_REG_WRITE(LENC_TIMEOUT_EN, mzc_pdata->comm_reg.reg47);

	mzc_pdata->comm_reg.reg50
		= MZC_REG_READ(LENC_PAGE_SYNC);
	mzc_pdata->comm_reg.reg_sync_lenc_reps_full_search = 0;
	MZC_REG_WRITE(LENC_PAGE_SYNC, mzc_pdata->comm_reg.reg50);
}

static void __mtk_dtv_ldec_cmdq_init(void)
{
	mzc_pdata->comm_reg.reg1d  = MZC_REG_READ(LDEC_GENERAL_SETTING);
	mzc_pdata->comm_reg.reg_ldec_op_mode = CMDQ_MODE;
	mzc_pdata->comm_reg.reg_w1c_ldec_start = 1;
	mzc_pdata->comm_reg.reg_ldec_crc_en
		= LDEC_CRC_DEFAULT_ENABLE_STATE;  //set crc enable or disable
	mzc_pdata->comm_reg.reg_ldec_crc_mode
		= LDEC_CRC_DEFAULT_MODE; //set crc mode
	MZC_REG_WRITE(LDEC_GENERAL_SETTING, mzc_pdata->comm_reg.reg1d);

	mzc_pdata->comm_reg.reg18 = MZC_REG_READ(MZC_OUTQ_NUM_TH);
	mzc_pdata->comm_reg.reg_ldec_outq_num_th
		= LDEC_OUTPUT_THRESHOLD; //set output threshold
	MZC_REG_WRITE(MZC_OUTQ_NUM_TH, mzc_pdata->comm_reg.reg18);

	mzc_pdata->comm_reg.reg19 = MZC_REG_READ(LDEC_OUTQ_TIMER_TH_LO);
	mzc_pdata->comm_reg.reg_ldec_outq_timer_th_lo
		=  LDEC_CMDQ_TIMER_THR_LO; //set timer default
	MZC_REG_WRITE(LDEC_OUTQ_TIMER_TH_LO, mzc_pdata->comm_reg.reg19);

	mzc_pdata->comm_reg.reg1a
		= MZC_REG_READ(LDEC_OUTQ_TIMER_TH_HI);
	mzc_pdata->comm_reg.reg_ldec_outq_timer_th_hi
		=  LDEC_CMDQ_TIMER_THR_HI;
	MZC_REG_WRITE(LDEC_OUTQ_TIMER_TH_HI, mzc_pdata->comm_reg.reg1a);


	mzc_pdata->comm_reg.reg_ldec_timeout_thr_lo
		= LDEC_TIMEOUT_THRESHOLD & 0xffff;
	MZC_REG_WRITE(LDEC_TIMEOUT_THR, mzc_pdata->comm_reg.reg23);

	mzc_pdata->comm_reg.reg24
		= MZC_REG_READ(LDEC_TIMEOUT_EN);
	mzc_pdata->comm_reg.reg_ldec_timeout_thr_hi
		= LDEC_TIMEOUT_THRESHOLD >> 16;
	mzc_pdata->comm_reg.reg_ldec_cnt_sel
		= 0; // 0: wrap-around, 1: staurated
	MZC_REG_WRITE(LDEC_TIMEOUT_EN, mzc_pdata->comm_reg.reg24);
}

void __mtk_dtv_mzc_cmdq_init(int enc_out_size_thr)
{
	__mtk_dtv_mzc_common_init(enc_out_size_thr);
	__mtk_dtv_lenc_cmdq_init();
	__mtk_dtv_ldec_cmdq_init();
}

void __mtk_dtv_mzc_irq_mask_all(void)
{
	MZC_REG_WRITE(MZC_IRQ_MASK_LO, MASK_IRQ_ALL);
	MZC_REG_WRITE(MZC_IRQ_MASK_HI, MASK_IRQ_ALL);
}

void __mtk_dtv_mzc_irq_mask_all_except_timeout(void)
{
	MZC_REG_WRITE(MZC_IRQ_MASK_LO, MASK_IRQ_ALL_EXCEPT_TIMER_AND_NUMBER);
	MZC_REG_WRITE(MZC_IRQ_MASK_HI, MASK_IRQ_ALL);
}

int __mtk_dtv_ldec_cmdq_set(phys_addr_t in_addr,
		phys_addr_t out_addr, unsigned int process_id)
{
	mzc_pdata->ldec_cmdq_info = 0;
	mzc_pdata->ldec_output_level = 0;

	phys_addr_t in_addr2 = ((in_addr & PAGE_MASK) + PAGE_SIZE);

	MZC_REG_WRITE_32(LDEC_INQ_IN_ST_ADR_LSB_LO, (in_addr & BIT_MASK_32));
	MZC_REG_WRITE_32(LDEC_INQ_DATA,
						((out_addr >> SHITF_12) << SHIFT_8)
						| ((in_addr & BIT_MASK_33_34) >> SHITF_28)
						| process_id);
	MZC_REG_WRITE_32(LDEC_IN_ST_ADR_4K,
					((in_addr2 >> SHITF_12) & BIT_MASK_24)
					| (1 << SHITF_24));

	return 0;
}

int __mtk_dtv_ldec_cmdq_set_split(phys_addr_t in_addr1,
	phys_addr_t in_addr2, phys_addr_t out_addr,
	unsigned int process_id)
{
	mzc_pdata->ldec_cmdq_info = 0;
	mzc_pdata->ldec_output_level = 0;
	const int enable = 1;
	MZC_REG_WRITE_32(LDEC_INQ_IN_ST_ADR_LSB_LO,
		(in_addr1 & 0xffffffff));
	if (in_addr1 > BIT_MASK_32)
		MZC_REG_WRITE_32(LDEC_INQ_DATA,
			((out_addr >> SHITF_12) << SHIFT_8)
			| ((in_addr1 & BIT_MASK_33_34) >> SHITF_28)
			| process_id);
	else
		MZC_REG_WRITE_32(LDEC_INQ_DATA,
			((out_addr >> SHITF_12) << SHIFT_8)
			| ((0) >> SHITF_28)
			| process_id);
	MZC_REG_WRITE_32(LDEC_IN_ST_ADR_4K,
		((in_addr2 >> 12) & 0xffffff) | (enable << 24));
	return 0;
}

int __mtk_dtv_lenc_cmdq_set(phys_addr_t in_addr,
	phys_addr_t out_addr,
	unsigned int process_id)
{
	mzc_pdata->lenc_cmdq_info = 0;
	mzc_pdata->lenc_output_level = 0;
	if (out_addr > BIT_MASK_32)
		MZC_REG_WRITE_32(LENC_INQ_DATA,
			((in_addr>>SHITF_12)  << SHIFT_8)
			| (out_addr >> SHITF_35) << SHIFT_4 | process_id);
	else
		MZC_REG_WRITE_32(LENC_INQ_DATA,
			((in_addr>>SHITF_12)  << SHIFT_8) | (0) << SHIFT_4 | process_id);
	MZC_REG_WRITE_32(LENC_INQ_OUT_ST_ADR_LSB_LO,
		((out_addr >> 3) & 0xffffffff)); //bit 3 ~ bit34
	return 0;
}


int __mtk_dtv_lenc_cmdq_done(int *target_id)
{
	if (((MZC_REG_READ(RO_LENC_OUT_INQ_LV1) >> 8) & 0x1f) == 0
		&& mzc_pdata->lenc_output_level == 0) {
		return 0;
	} else if (mzc_pdata->lenc_output_level == 0)  {
		unsigned int cmdq_out_data;

		unsigned short cmdq_out_data_hi;

		unsigned short cmdq_out_data_lo;

		unsigned short process_id;

		cmdq_out_data = MZC_REG_READ_32(LENC_OUTQ_INFO_LO);
		cmdq_out_data_hi = (cmdq_out_data >> 16); //take last 16 bits
		cmdq_out_data_lo = (cmdq_out_data & 0xffff);
		process_id = (cmdq_out_data_hi & 0xf); //take first 4 bits
		*target_id = process_id;
		if ((cmdq_out_data_lo & CMDQ_LENC_NORMAL_END)
			== CMDQ_LENC_NORMAL_END) {
			unsigned short outlen
				= ((cmdq_out_data_lo >> 3) & 0x1fff);

			return outlen;
		} else if ((cmdq_out_data_lo & CMDQ_LENC_SIZE_ERROR)
			== CMDQ_LENC_SIZE_ERROR) {
			return PAGE_SIZE;
		} else {
			return -1; //should never happen
		}
	} else {
		unsigned int cmdq_out_data = mzc_pdata->lenc_cmdq_info;

		unsigned short cmdq_out_data_hi;

		unsigned short cmdq_out_data_lo;

		unsigned short process_id;

		cmdq_out_data_hi = (cmdq_out_data >> 16); //take last 16 bits
		cmdq_out_data_lo = (cmdq_out_data & 0xffff);
		process_id = (cmdq_out_data_hi & 0xf); //take first 4 bits
		*target_id = process_id;
		if ((cmdq_out_data_lo & CMDQ_LENC_NORMAL_END)
			== CMDQ_LENC_NORMAL_END) { //first bit for normal_end
			unsigned short outlen
				= ((cmdq_out_data_lo >> 3) & 0x1fff);
			// take bit 3~15
			return outlen;
		} else if ((cmdq_out_data_lo & CMDQ_LENC_SIZE_ERROR)
			== CMDQ_LENC_SIZE_ERROR)   {
			return PAGE_SIZE;
		} else {
			return -1; //should never happen
		}
	}
}

int __mtk_dtv_ldec_cmdq_done(int *target_id)
{
	if (((MZC_REG_READ(RO_LDEC_OUT_INQ_LV1) >> 8) & 0x1f) == 0
		&& mzc_pdata->ldec_output_level == 0) {
		return 0;
	} else if (mzc_pdata->ldec_output_level == 0) {
		unsigned short cmdq_out_data;

		unsigned short process_id;

		cmdq_out_data = MZC_REG_READ(LDEC_OUTQ_INFO);
		process_id = cmdq_out_data >> 6; //take last 4 bits
		*target_id = process_id;
		if ((cmdq_out_data & CMDQ_LDEC_NORMAL_END)
			== CMDQ_LDEC_NORMAL_END)
			return PAGE_SIZE;
		else
			return (cmdq_out_data & CMDQ_LDEC_ERROR_TYPE_BIT_POS);
	} else {
		unsigned short cmdq_out_data = mzc_pdata->ldec_cmdq_info;

		unsigned short process_id;

		process_id = cmdq_out_data >> 6; //take last 4 bits
		*target_id = process_id;
		if ((cmdq_out_data & CMDQ_LDEC_NORMAL_END)
			== CMDQ_LDEC_NORMAL_END)
			return PAGE_SIZE;
		else
			return (cmdq_out_data & CMDQ_LDEC_ERROR_TYPE_BIT_POS);
	}
}

void __mtk_dtv_ldec_cycle_info(int *active_cycle,
	int *free_cycle, int *page_count)
{
	mzc_pdata->comm_reg.reg24 = MZC_REG_READ(LDEC_TIMEOUT_EN);
	mzc_pdata->comm_reg.reg_ldec_cnt_sel_1 = 0;
	MZC_REG_WRITE(LDEC_TIMEOUT_EN, mzc_pdata->comm_reg.reg24);

	u32 orig_active_cycle = MZC_REG_READ_32(RO_LDEC_CYC_INFO);

	*active_cycle = orig_active_cycle;

	mzc_pdata->comm_reg.reg24 = MZC_REG_READ(LDEC_TIMEOUT_EN);
	mzc_pdata->comm_reg.reg_ldec_cnt_sel_1 = 1;
	MZC_REG_WRITE(LDEC_TIMEOUT_EN, mzc_pdata->comm_reg.reg24);


	u32 orig_free_cycle = MZC_REG_READ_32(RO_LDEC_CYC_INFO);

	*free_cycle = orig_free_cycle;

	mzc_pdata->comm_reg.reg24 = MZC_REG_READ(LDEC_TIMEOUT_EN);
	mzc_pdata->comm_reg.reg_ldec_cnt_sel_1 = 2;
	MZC_REG_WRITE(LDEC_TIMEOUT_EN, mzc_pdata->comm_reg.reg24);

	u32 orig_page_count = MZC_REG_READ_32(RO_LDEC_CYC_INFO);

	*page_count = orig_page_count;
}


void __mtk_dtv_lenc_cycle_info(int *active_cycle,
	int *free_cycle, int *page_count)
{
	mzc_pdata->comm_reg.reg47 = MZC_REG_READ(LENC_TIMEOUT_EN);
	mzc_pdata->comm_reg.reg_lenc_cnt_sel_1 = 0;
	MZC_REG_WRITE(LENC_TIMEOUT_EN, mzc_pdata->comm_reg.reg47);

	u32 orig_active_cycle = MZC_REG_READ_32(RO_LENC_CYC_INFO);
	*active_cycle = orig_active_cycle;

	mzc_pdata->comm_reg.reg47 = MZC_REG_READ(LENC_TIMEOUT_EN);
	mzc_pdata->comm_reg.reg_lenc_cnt_sel_1 = 1;
	MZC_REG_WRITE(LENC_TIMEOUT_EN, mzc_pdata->comm_reg.reg47);

	u32 orig_free_cycle = MZC_REG_READ_32(RO_LENC_CYC_INFO);
	*free_cycle = orig_free_cycle;

	mzc_pdata->comm_reg.reg47 = MZC_REG_READ(LENC_TIMEOUT_EN);
	mzc_pdata->comm_reg.reg_lenc_cnt_sel_1 = 2;
	MZC_REG_WRITE(LENC_TIMEOUT_EN, mzc_pdata->comm_reg.reg47);

	u32 orig_page_count = MZC_REG_READ_32(RO_LENC_CYC_INFO);
	*page_count = orig_page_count;
}

void __mtk_dtv_mzc_single_init(int enc_out_size_thr)
{
	__mtk_dtv_mzc_common_init(enc_out_size_thr);

	mzc_pdata->comm_reg.reg1d
		= MZC_REG_READ(LDEC_GENERAL_SETTING);
	mzc_pdata->comm_reg.reg_ldec_crc_en
		= LDEC_CRC_DEFAULT_ENABLE_STATE;  //set crc enable or disable
	mzc_pdata->comm_reg.reg_ldec_crc_mode
		= LDEC_CRC_DEFAULT_MODE; //set crc mode
	MZC_REG_WRITE(LDEC_GENERAL_SETTING, mzc_pdata->comm_reg.reg1d);

	mzc_pdata->comm_reg.reg40
		= MZC_REG_READ(LENC_GENERAL_SETTING);
	mzc_pdata->comm_reg.reg_lenc_crc_en
		= LENC_CRC_DEFAULT_ENABLE_STATE;  //set crc enable or disable
	mzc_pdata->comm_reg.reg_lenc_crc_mode
		= LENC_CRC_DEFAULT_MODE; //set crc mode
	MZC_REG_WRITE(LENC_GENERAL_SETTING, mzc_pdata->comm_reg.reg40);

	// mask IRQs
	MZC_REG_WRITE(MZC_IRQ_MASK_LO, MASK_IRQ_ALL);
	MZC_REG_WRITE(MZC_IRQ_MASK_HI, MASK_SINGLE_MODE_IRQ);
}

int __mtk_dtv_lenc_single_set(unsigned long in_addr,
	unsigned long out_addr, unsigned int process_id)
{
	//mzc_pdata->comm_reg.reg00 = MZC_REG_READ(CMDQ_TOP_SETTING);
	mzc_pdata->comm_reg.reg40 = MZC_REG_READ(LENC_GENERAL_SETTING);
	mzc_pdata->comm_reg.reg_w1c_lenc_start = 1;
	mzc_pdata->comm_reg.reg_lenc_op_mode = SINGLE_MODE;
	MZC_REG_WRITE(LENC_GENERAL_SETTING, mzc_pdata->comm_reg.reg40);

	mzc_pdata->comm_reg.reg41 = MZC_REG_READ(LENC_IN_ST_ADR);
	mzc_pdata->comm_reg.reg_lenc_in_st_adr_lo = ((in_addr >> 12) & 0xffff);
	MZC_REG_WRITE(LENC_IN_ST_ADR, mzc_pdata->comm_reg.reg41);

	mzc_pdata->comm_reg.reg42 = MZC_REG_READ(LENC_OUT_ST_ADR_MSB);
	mzc_pdata->comm_reg.reg_lenc_in_st_adr_hi = (in_addr >> 28);
#ifdef CONFIG_ARM
	mzc_pdata->comm_reg.reg_lenc_out_st_adr_msb = 0;
#else
	mzc_pdata->comm_reg.reg_lenc_out_st_adr_msb = (out_addr >> 35);
#endif
	MZC_REG_WRITE(LENC_OUT_ST_ADR_MSB, mzc_pdata->comm_reg.reg42);

	mzc_pdata->comm_reg.reg43
		= MZC_REG_READ(LENC_OUT_ST_ADR_LSB_LO);
	mzc_pdata->comm_reg.reg_lenc_out_st_adr_lsb_lo
		= ((out_addr >> 3) & 0xffff);
	MZC_REG_WRITE(LENC_OUT_ST_ADR_LSB_LO, mzc_pdata->comm_reg.reg43);

	mzc_pdata->comm_reg.reg44
		= MZC_REG_READ(LENC_OUT_ST_ADR_LSB_HI);
	mzc_pdata->comm_reg.reg_lenc_out_st_adr_lsb_hi
		= (out_addr >> 19);
	MZC_REG_WRITE(LENC_OUT_ST_ADR_LSB_HI, mzc_pdata->comm_reg.reg44);

	return 0;
}

int __mtk_dtv_ldec_single_set(unsigned long in_addr,
	unsigned long out_addr, unsigned int process_id)
{

	//mzc_pdata->comm_reg.reg00 = MZC_REG_READ(CMDQ_TOP_SETTING);
	mzc_pdata->comm_reg.reg1d = MZC_REG_READ(LDEC_GENERAL_SETTING);
	mzc_pdata->comm_reg.reg_w1c_ldec_start = 1;
	mzc_pdata->comm_reg.reg_ldec_op_mode = SINGLE_MODE;
	MZC_REG_WRITE(LDEC_GENERAL_SETTING, mzc_pdata->comm_reg.reg1d);




	mzc_pdata->comm_reg.reg1e = MZC_REG_READ(LDEC_IN_ST_ADR_LSB_LO);
	mzc_pdata->comm_reg.reg_ldec_in_st_adr_lsb_lo = in_addr & 0xffff;
	MZC_REG_WRITE(LDEC_IN_ST_ADR_LSB_LO, mzc_pdata->comm_reg.reg1e);

	mzc_pdata->comm_reg.reg1f = MZC_REG_READ(LDEC_IN_ST_ADR_LSB_HI);
	mzc_pdata->comm_reg.reg_ldec_in_st_adr_lsb_hi = in_addr >> 16;
	MZC_REG_WRITE(LDEC_IN_ST_ADR_LSB_HI, mzc_pdata->comm_reg.reg1f);

	mzc_pdata->comm_reg.reg20
		= MZC_REG_READ(LDEC_OUT_ST_ADR_LO);
	mzc_pdata->comm_reg.reg_ldec_out_st_adr_lo
		= ((out_addr >> 12) & 0xffff);
	MZC_REG_WRITE(LDEC_OUT_ST_ADR_LO, mzc_pdata->comm_reg.reg20);


	mzc_pdata->comm_reg.reg21 = MZC_REG_READ(LDEC_OUT_ST_ADR_HI);
	mzc_pdata->comm_reg.reg_ldec_out_st_adr_hi = (out_addr >> 28);
	//mzc_pdata->comm_reg.reg_ldec_soft_rstz = RESET;
	//MZC_REG_WRITE(CMDQ_TOP_SETTING,mzc_pdata->comm_reg.reg00);
	//mzc_pdata->comm_reg.reg_ldec_soft_rstz = NOT_RESET;
	//MZC_REG_WRITE(CMDQ_TOP_SETTING,mzc_pdata->comm_reg.reg00);
#ifdef CONFIG_ARM
	mzc_pdata->comm_reg.reg_ldec_in_st_adr_msb = 0;
#else
	mzc_pdata->comm_reg.reg_ldec_in_st_adr_msb = (in_addr >> 32);
#endif
	MZC_REG_WRITE(LDEC_OUT_ST_ADR_HI, mzc_pdata->comm_reg.reg21);
	return 0;
}

int __mtk_dtv_lenc_single_done(void)
{

	unsigned short irq_type_hi = MZC_REG_READ(MZC_ST_IRQ_CPU_HI);

	unsigned short outlen;

	if ((irq_type_hi & LENC_IRQ_NORMAL_END) == LENC_IRQ_NORMAL_END) {
		MZC_REG_WRITE(MZC_IRQ_STATUS_HI,
			CLEAR_IRQ(LENC_IRQ_NORMAL_END));
		outlen = MZC_REG_READ(RO_LENC_BYTE_CNT) & 0x1fff;
	} else if ((irq_type_hi & LENC_IRQ_SIZE_ERROR) == LENC_IRQ_SIZE_ERROR) {
		MZC_REG_WRITE(MZC_IRQ_STATUS_HI,
			CLEAR_IRQ(LENC_IRQ_SIZE_ERROR));
		outlen = PAGE_SIZE;
	} else if ((irq_type_hi & LENC_IRQ_TIMEOUT) == LENC_IRQ_TIMEOUT) {
		MZC_REG_WRITE(MZC_IRQ_STATUS_HI, CLEAR_IRQ(LENC_IRQ_TIMEOUT));
		outlen = -1;
	} else {
		outlen = 0;
	}

	return outlen;
}

int __mtk_dtv_ldec_single_done(void)
{
	unsigned short irq_type_hi = MZC_REG_READ(MZC_ST_IRQ_CPU_HI);

	unsigned short outlen;

	if ((irq_type_hi & LDEC_IRQ_NORMAL_END) == LDEC_IRQ_NORMAL_END) {
		MZC_REG_WRITE(MZC_IRQ_STATUS_HI,
		CLEAR_IRQ(LDEC_IRQ_NORMAL_END));
		outlen = PAGE_SIZE;
	} else {
		outlen = irq_type_hi & SINGLE_MODE_LDEC_IRQ;
	}

	return outlen;
}


void __mtk_dtv_ldec_state_save(int wait)
{
	if (wait) {
		while (((MZC_REG_READ(RO_LDEC_OUT_INQ_LV1) >> 8) & 0x1f) == 0)
			;
	}
	mzc_pdata->ldec_cmdq_info = MZC_REG_READ(LDEC_OUTQ_INFO);
	mzc_pdata->ldec_output_level = 1;

}

void __mtk_dtv_lenc_state_save(int wait)
{
	if (wait) {
		while (((MZC_REG_READ(RO_LENC_OUT_INQ_LV1) >> 8) & 0x1f) == 0)
			;
	}
	mzc_pdata->lenc_cmdq_info = MZC_REG_READ_32(LENC_OUTQ_INFO_LO);
	mzc_pdata->lenc_output_level = 1;
}

int __mtk_dtv_lenc_state_check(void)
{
	if (((MZC_REG_READ(RO_LENC_OUT_INQ_LV1) >> 8) & 0x1f) != 0)
		return 1;
	if (((MZC_REG_READ(RO_LENC_OUT_INQ_LV1)) & 0x1f) != 0)
		return 2;
	else
		return 0;
}


int __mtk_dtv_ldec_state_check(void)
{
	if (((MZC_REG_READ(RO_LDEC_OUT_INQ_LV1) >> 8) & 0x1f) != 0)
		return 1;
	if (((MZC_REG_READ(RO_LDEC_OUT_INQ_LV1)) & 0x1f) != 0)
		return 2;
	else
		return 0;
}

unsigned long __mtk_dtv_lenc_crc_get(void)
{
	return MZC_REG_READ_32(LENC_CRC_OUT);
}


void __mtk_dtv_ldec_literal_bypass_set(int value)
{
	if (value < 0 || value > 2) {
		WARN_ON(1);
	} else {
		mzc_pdata->comm_reg.reg50 = MZC_REG_READ(LENC_PAGE_SYNC);
		mzc_pdata->comm_reg.reg_sync_lenc_lit_bypass_bins = value;
		MZC_REG_WRITE(LENC_PAGE_SYNC, mzc_pdata->comm_reg.reg50);
	}
}


//hal layer end


static int enc_out_size_thr = MAX_SIZE_FOR_ZRAM;

#ifdef CONFIG_MP_ZRAM_PERFORMANCE
static inline unsigned long long timediff
	(struct timeval begin, struct timeval end)
{
	return ((end.tv_sec - begin.tv_sec)
			* 1000000
			+ (end.tv_usec - begin.tv_usec));
}

unsigned long long duration_enc = 0, duration_dec = 0;

#endif

static inline unsigned long VA_to_PA(unsigned long VA)
{
	return virt_to_phys((const volatile void*)(VA));
}


static int mtk_dtv_lenc_cmdq_done(struct cmdq_node *zram_write_cmdq)
{
	int target_id;

	int outlen;

	outlen = __mtk_dtv_lenc_cmdq_done(&target_id);
	if (outlen != 0) {
		zram_write_cmdq[target_id].out_len = outlen;
		return 1;
	} else {
		return 0;
	}
}

static int mtk_dtv_ldec_cmdq_done(struct cmdq_node *zram_read_cmdq)
{
	int target_id;

	int outlen;

	outlen = __mtk_dtv_ldec_cmdq_done(&target_id);
	if (outlen != 0) {
		zram_read_cmdq[target_id].out_len = outlen;
		return 1;
	} else {
		return 0;
	}
}


int mtk_dtv_mzc_cmdq_init(void)
{
	int i;

	//__mtk_dtv_acp_common_init();
	__mtk_dtv_mzc_cmdq_init(enc_out_size_thr);
	spin_lock_init(&mzc_pdata->empty_dev.r_lock);
	spin_lock_init(&mzc_pdata->empty_dev.w_lock);
	__mtk_dtv_mzc_irq_mask_all();
	return 0;
}

static int mtk_dtv_lenc_cmdq_set(phys_addr_t in_addr,
	phys_addr_t out_addr,
	unsigned int process_id,
	struct cmdq_node *zram_write_cmdq)
{
	zram_write_cmdq[process_id].out_len = 0;
	return __mtk_dtv_lenc_cmdq_set
		((in_addr),
		(out_addr),
		process_id);
}

static int mtk_dtv_ldec_cmdq_set(phys_addr_t in_addr,
	phys_addr_t out_addr,
	unsigned int process_id,
	struct cmdq_node *zram_read_cmdq)
{
	zram_read_cmdq[process_id].out_len = 0;
	return __mtk_dtv_ldec_cmdq_set(
		(in_addr),
		(out_addr),
		process_id);
}

void mtk_dtv_ldec_literal_bypass_set(int value)
{
	__mtk_dtv_ldec_literal_bypass_set(value);
}

static int mtk_dtv_ldec_cmdq_set_split(phys_addr_t in_addr1,
	phys_addr_t in_addr2,
	phys_addr_t out_addr,
	unsigned int process_id,
	struct cmdq_node *zram_read_cmdq)
{
	zram_read_cmdq[process_id].out_len = 0;
	return __mtk_dtv_ldec_cmdq_set_split((in_addr1),
		(in_addr2),
		(out_addr),
		process_id);
}

int mtk_dtv_lenc_cmdq_run(phys_addr_t in_addr,
	phys_addr_t out_addr,
	unsigned int *dst_len)
{
	struct cmdq_node *pCmdq = mzc_pdata->empty_dev.write_cmdq;
	int ret = 0;
	const int process_id = 0;

	spin_lock(&mzc_pdata->empty_dev.w_lock);
	mtk_dtv_lenc_cmdq_set(in_addr,
		out_addr,
		process_id,
		pCmdq);
	while (!mtk_dtv_lenc_cmdq_done(pCmdq))
		;
	if (pCmdq[process_id].out_len == -1)
		ret = -1;
	*dst_len = pCmdq[process_id].out_len;
	spin_unlock(&mzc_pdata->empty_dev.w_lock);

	return ret;
}
EXPORT_SYMBOL(mtk_dtv_lenc_cmdq_run);

int mtk_dtv_ldec_cmdq_run(phys_addr_t in_addr, phys_addr_t out_addr)
{
	struct cmdq_node *pCmdq = mzc_pdata->empty_dev.read_cmdq;
	int ret = 0;

	int retry_count = 3;

	const int process_id = 0;

	spin_lock(&mzc_pdata->empty_dev.r_lock);
DEC_AGAIN:
	mtk_dtv_ldec_cmdq_set(in_addr, out_addr, process_id, pCmdq);
	while (!mtk_dtv_ldec_cmdq_done(pCmdq))
		;
	if (pCmdq[process_id].out_len != PAGE_SIZE) {
		ret = pCmdq[process_id].out_len;
		if (retry_count) {
			retry_count--;
			goto DEC_AGAIN;
		}
	} else {
		ret = 0;
	}
	spin_unlock(&mzc_pdata->empty_dev.r_lock);
	return ret;
}
EXPORT_SYMBOL(mtk_dtv_ldec_cmdq_run);

int mtk_dtv_ldec_cmdq_run_split(phys_addr_t in_addr1,
	phys_addr_t in_addr2,
	phys_addr_t out_addr)
{
	struct cmdq_node *pCmdq = mzc_pdata->empty_dev.read_cmdq;
	int ret = 0;

	int retry_count = 3;

	const int process_id = 0;

	spin_lock(&mzc_pdata->empty_dev.r_lock);

DEC_AGAIN:
	mtk_dtv_ldec_cmdq_set_split(in_addr1,
		in_addr2,
		out_addr,
		process_id,
		pCmdq);
	while (!mtk_dtv_ldec_cmdq_done(pCmdq))
		;
	if (pCmdq[process_id].out_len != PAGE_SIZE) {
		ret = pCmdq[process_id].out_len;
		if (retry_count) {
			retry_count--;
			goto DEC_AGAIN;
		}
	} else {
		ret = 0;
	}


	spin_unlock(&mzc_pdata->empty_dev.r_lock);

	return ret;
}
EXPORT_SYMBOL(mtk_dtv_ldec_cmdq_run_split);

void mtk_dtv_ldec_cycle_info(void)
{
	int active_cycle;

	int free_cycle;

	int page_count;

	__mtk_dtv_ldec_cycle_info(&active_cycle,
		&free_cycle,
		&page_count);
}

void mtk_dtv_lenc_cycle_info(void)
{
	int active_cycle;

	int free_cycle;

	int page_count;

	__mtk_dtv_lenc_cycle_info(&active_cycle,
		&free_cycle,
		&page_count);
}

static void mtk_dtv_ldec_state_save(int wait)
{
	__mtk_dtv_ldec_state_save(wait);
}

static void mtk_dtv_lenc_state_save(int wait)
{
	__mtk_dtv_lenc_state_save(wait);
}

static int mtk_dtv_ldec_state_check(void)
{
	return __mtk_dtv_ldec_state_check();
}

static int mtk_dtv_lenc_state_check(void)
{
	return __mtk_dtv_lenc_state_check();
}


u32 mtk_dtv_lenc_crc32_get(unsigned char *cmem, unsigned int comp_len)
{

	u32 crc32 = __mtk_dtv_lenc_crc_get();

	unsigned int crc16 = __mtk_dtv_lenc_crc_get();

	return crc32;
}


int mtk_dtv_lenc_hybrid_cmdq_run(phys_addr_t in_addr,
	phys_addr_t out_addr,
	unsigned int *dst_len)
{
	struct cmdq_node *pCmdq = mzc_pdata->empty_dev.write_cmdq;

	int ret = 0;

	const int process_id = 0;

	if (spin_trylock(&mzc_pdata->empty_dev.w_lock)) {
		mtk_dtv_lenc_cmdq_set(in_addr, out_addr, process_id, pCmdq);
		while (!mtk_dtv_lenc_cmdq_done(pCmdq))
			;
		if (pCmdq[process_id].out_len == -1)
			ret = -1;
		*dst_len = pCmdq[process_id].out_len;
		spin_unlock(&mzc_pdata->empty_dev.w_lock);
		return ret;
	} else {
		return MZC_ERR_NO_ENC_RESOURCE;
	}
}
EXPORT_SYMBOL(mtk_dtv_lenc_hybrid_cmdq_run);

static s32 mtk_mzc_suspend(struct platform_device *pDev_st,
	pm_message_t state)
{
	int ldec_state, lenc_state;

	mtk_dtv_ldec_state_check();
	mtk_dtv_lenc_state_check();
	ldec_state = mtk_dtv_ldec_state_check();
	lenc_state = mtk_dtv_lenc_state_check();
	if (ldec_state == 1)
		mtk_dtv_ldec_state_save(0);
	else if (ldec_state == 2)
		mtk_dtv_ldec_state_save(1); //have to wait for HW decode done
	if (lenc_state == 1)
		mtk_dtv_lenc_state_save(0);
	else if (lenc_state == 2)
		mtk_dtv_lenc_state_save(1); //have to wait for HW decode done
	return 0;
}

static struct clk *mzc_mux_gate;

static s32 mtk_mzc_resume(struct platform_device *pdev)
{
	mzc_mux_gate = devm_clk_get(&pdev->dev, "MZC-CLK");
	if (IS_ERR_OR_NULL(mzc_mux_gate)) {
		if (IS_ERR(mzc_mux_gate))
			dev_err(&pdev->dev, "err mzc_mux_gate=%#lx\n",
				(unsigned long)mzc_mux_gate);
		else
			dev_err(&pdev->dev, "err mzc_mux_gate=NULL\n");
		return -EINVAL;
	}
	int res = clk_prepare_enable(mzc_mux_gate);

	if (res) {
		dev_err(&pdev->dev, "Failed to enable MZC clock: %d\n", res);
		return -EINVAL;
	}
	mtk_dtv_mzc_cmdq_init();
	return 0;
}

static s32 mtk_mzc_probe(struct platform_device *pdev)
{
	mzc_pdata = devm_kzalloc(&pdev->dev, sizeof(*mzc_pdata), GFP_KERNEL);
	platform_set_drvdata(pdev, mzc_pdata);
	mzc_pdata->dev = &pdev->dev;
	struct device_node *dn;
	struct resource *res;
	int ret;

	mzc_mux_gate = devm_clk_get(&pdev->dev, "MZC-CLK");
	if (IS_ERR_OR_NULL(mzc_mux_gate)) {
		if (IS_ERR(mzc_mux_gate))
			dev_err(&pdev->dev, "err mzc_mux_gate=%#lx\n",
				(unsigned long)mzc_mux_gate);
		else
			dev_err(&pdev->dev, "err mzc_mux_gate=NULL\n");
		return -EINVAL;
	}
	ret = clk_prepare_enable(mzc_mux_gate);
	if (ret) {
		dev_err(&pdev->dev, "Failed to enable MZC clock: %d\n", ret);
		return -EINVAL;
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;
	mzc_pdata->mzc_bank = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mzc_pdata->mzc_bank)) {
		dev_err(&pdev->dev, "err mzc_bank=%#lx\n",
			(unsigned long)mzc_pdata->mzc_bank);
		return PTR_ERR(mzc_pdata->mzc_bank);
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -EINVAL;
	mzc_pdata->acp_bank = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mzc_pdata->acp_bank)) {
		dev_err(&pdev->dev, "err acp_bank=%#lx\n",
			(unsigned long)mzc_pdata->acp_bank);
		return PTR_ERR(mzc_pdata->acp_bank);
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res)
		return -EINVAL;
	mzc_pdata->freq_bank = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mzc_pdata->freq_bank)) {
		dev_err(&pdev->dev, "err freq_bank=%#lx\n",
			(unsigned long)mzc_pdata->freq_bank);
		return PTR_ERR(mzc_pdata->freq_bank);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!res)
		return -EINVAL;
	mzc_pdata->mzc_clk_bank = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mzc_pdata->mzc_clk_bank)) {
		dev_err(&pdev->dev, "err mzc_clk_bank=%#lx\n",
			(unsigned long)mzc_pdata->mzc_clk_bank);
		return PTR_ERR(mzc_pdata->mzc_clk_bank);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	if (!res)
		return -EINVAL;
	mzc_pdata->acache_bank = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mzc_pdata->acache_bank)) {
		dev_err(&pdev->dev, "err acache_bank=%#lx\n",
			(unsigned long)mzc_pdata->acache_bank);
		return PTR_ERR(mzc_pdata->acache_bank);
	}

	dn = pdev->dev.of_node;

	ret = of_property_read_u32(dn, "is_new_mzc", &is_new_mzc);
	if (ret)
		return -EINVAL;
	ret = of_property_read_u32(dn, "is_secure", &is_secure);
	if (ret)
		return -EINVAL;
	mtk_dtv_mzc_cmdq_init();



	return 0;
}

static s32 mtk_mzc_remove(struct platform_device *pdev)
{
	clk_disable_unprepare(mzc_mux_gate);
	devm_clk_put(&pdev->dev, mzc_mux_gate);
	return 0;
}

static const struct of_device_id mtk_dtv_mzc_match[] = {
	{ .compatible = "mtk,mtk-dtv-MZC" },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_dtv_mzc_match);

static struct platform_driver mzc_driver = {

	.probe	= mtk_mzc_probe,
	.remove	= mtk_mzc_remove,
#ifdef CONFIG_PM
	.suspend = mtk_mzc_suspend,
	.resume = mtk_mzc_resume,
#endif

	.driver = {
		.name = "mtk-dtv-MZC",
		.owner = THIS_MODULE,
		.of_match_table = mtk_dtv_mzc_match
	},
};


static int __init MZC_driver_init(void)
{
	return platform_driver_register(&mzc_driver);
}

static void __exit MZC_driver_fini(void)
{
	platform_driver_unregister(&mzc_driver);
}

module_init(MZC_driver_init);
module_exit(MZC_driver_fini);



module_param(enc_out_size_thr, int, 0644);
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("MZC");
MODULE_LICENSE("GPL");

