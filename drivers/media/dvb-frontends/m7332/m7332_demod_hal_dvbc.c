// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <asm/io.h>
#include <linux/string.h>
#include <linux/mm_types.h>
#include <linux/timekeeping.h>
#include "m7332_demod_drv_dvbc.h"
#include "m7332_demod_hal_dvbc.h"

#define MIU_INTERVAL     0x80000000
#define DMD_DVBC_TR_TIMEOUT     500
#define DMD_DVBC_LOCK_TIMEOUT  7000
#define DEFAULT_SYMBOL_RATE    6900


static u32 dvbc_fw_addr;

static u8 *dram_code_virt_addr;
static u8 fec_lock;
static u8 tr_lock;
static u32 scan_start_time;
static u32 last_lock_time;


u8 dvbc_table[] = {
    #include "fw_dmd_dvbc.dat"
};

static s16 dvbc_sqi_db_nordigp1_X10[5] = {
	194, 224, 255, 284, 318
};

static s16 dvbc_ssi_dbm_nordigp1_X10[5] = {
	-820, -792, -761, -730, -696
};

static u64 logapproxtablex100[80] = {
	100UL, 130UL, 169UL, 220UL, 286UL, 371UL, 483UL, 627UL, 816UL, 1060UL,
	1379UL, 1792UL, 2330UL, 3029UL, 3937UL, 5119UL, 6654UL, 8650UL,
	11246UL, 14619UL, 19005UL, 24706UL, 32118UL, 41754UL, 54280UL, 70564UL,
	91733UL, 119253UL, 155029UL, 201538UL, 262000UL, 340599UL, 442779UL,
	575613UL, 748297UL, 972786UL, 1264622UL, 1644008UL, 2137211UL,
	2778374UL, 3611886UL, 4695452UL, 6104088UL, 7935315UL, 10315909UL,
	13410682UL, 17433886UL, 22664052, 29463268, 38302248, 49792922,
	64730799, 84150039, 109395050, 142213565UL, 184877635UL, 240340925UL,
	312443203UL, 406176164UL, 528029013UL, 686437717UL, 892369032UL,
	1160079742UL, 1508103665UL, 1960534764UL, 2548695194UL, 3313303752UL,
	4307294877UL, 5599483340UL, 7279328342UL, 9463126845UL, 12302064899UL,
	15992684368UL, 20790489679UL, 27027636582UL, 35135927557UL,
	45676705824UL, 59379717572UL, 77193632843UL, 100351722696UL
};

static u64 logapproxtabley100[80] = {
	0, 11, 23, 34, 46, 57, 68, 80, 91, 103, 114, 125,
	137, 148, 160, 171, 182, 194, 205, 216, 228, 239, 251, 262,
	273, 285, 296, 308, 319, 330, 342, 353, 365, 376, 387, 399,
	410, 422, 433, 444, 456, 467, 479, 490, 501, 513, 524, 536,
	547, 558, 570, 581, 593, 604, 615, 627, 638, 649, 661, 672,
	684, 695, 706, 718, 729, 741, 752, 763, 775, 786, 798, 809,
	820, 832, 843, 855, 866, 877, 889, 900
};


static u64 log10approx100(u64 flt_x)
{
	u8  indx = 0;
	u64 intp;

	do {
		if (flt_x < logapproxtablex100[indx])
			break;
		indx++;
	} while (indx < 79);

	if ((indx != 0) && (indx != 79)) {
		intp = (flt_x-logapproxtablex100[indx-1])*
			(logapproxtabley100[indx]-logapproxtabley100[indx-1]);
		intp = intp/
			(logapproxtablex100[indx]-logapproxtablex100[indx-1]);
		intp = logapproxtabley100[indx-1]+intp;

		return (u32)intp;
	}

	return logapproxtabley100[indx];
}

static int intern_dvbc_soft_stop(void)
{
	u16 wait_cnt = 0;

	if (mtk_demod_read_byte(MB_REG_BASE + 0x00)) {
		pr_err(">> MB Busy!\n");
		return -ETIMEDOUT;
	}

	pr_info("[%s] is called\n", __func__);

	/* MB_CNTL set read mode */
	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0xA5);

	/* assert interrupt to VD MCU51 */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03, 0x02);
	/* de-assert interrupt to VD MCU51 */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03, 0x00);

	/* wait MB_CNTL set done */
	while (mtk_demod_read_byte(MB_REG_BASE + 0x00) != 0x5A) {
		mtk_demod_delay_ms(1);

		if (wait_cnt++ >= 0xFF) {
			pr_err("@@@@@ DVBC SoftStop Fail!\n");
			return -ETIMEDOUT;
		}
	}

	/* MB_CNTL clear */
	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0x00);

	mtk_demod_delay_ms(1);

	return 0;
}

static int intern_dvbc_adaptive_ts_config(struct dvb_frontend *fe)
{
	/* struct dtv_frontend_properties *c = &fe->dtv_property_cache; */
	u8 divider = 0;
	u8 ts_enable = 0;
	u8 clk_src = 0;
	u8 reg = 0;

	reg = mtk_demod_read_byte(0x112615);
	clk_src = (reg >> 6);
	divider = reg & 0x1f;

	if (clk_src == 1)
		pr_info("CLK Source: 288 MHz\n");
	else
		pr_info("CLK Source: 172 MHz\n");
	pr_info(">>>[%s] = 0x%x<<<\n",  __func__, divider);

	/* disable TS module work */
	reg = mtk_demod_read_byte(0x103302);
	mtk_demod_write_byte(0x103302, reg | 0x01);
	mtk_demod_write_byte(0x103300, divider);
	/* phase_tun setting */
	reg = mtk_demod_read_byte(0x103301);
	reg = (reg&(~0x07)) | clk_src;
	mtk_demod_write_byte(0x103301, reg | 0x40);
	mtk_demod_write_byte(0x10330b, (divider>>1));
	/* enable TS module work */
	reg = mtk_demod_read_byte(0x103302);
	mtk_demod_write_byte(0x103302, reg & ~0x01);

	/* extend TS FIFO */
	mtk_demod_mbx_dvb_write(BACKEND_REG_BASE + (0x16*2)+1, 0x15);

	/* Enable TS */
	mtk_demod_mbx_dvb_read(DVBTM_REG_BASE + 0x40, &ts_enable);
	ts_enable |= 0x04;
	mtk_demod_mbx_dvb_write(DVBTM_REG_BASE + 0x40, ts_enable);

	return 0;
}

static void intern_dvbc_get_snr_ms_format(struct ms_float_st *msf_snr)
{
	u8 reg;
	u16 snr;

	struct ms_float_st msf_denominator;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_DVBC_SNR100_H, &reg);
	snr = reg;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_DVBC_SNR100_L, &reg);
	snr = (snr<<8)|reg;

	msf_snr->data = (u32)snr;
	msf_snr->exp = 0;
	msf_denominator.data = 10;
	msf_denominator.exp = 0;
	*msf_snr = mtk_demod_float_op(*msf_snr,
		msf_denominator, DIVIDE);
}

static void intern_dvbc_get_ber_ms_format(struct ms_float_st *msf_ber)
{
	u8 reg, reg_frz;
	u16 bit_err_period;
	u32 bit_err;

	struct ms_float_st msf_numerator;
	struct ms_float_st msf_denominator;

	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x03, &reg_frz);
	mtk_demod_mbx_dvb_write(BACKEND_REG_BASE+0x03, reg_frz|0x03);

	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x47, &reg);
	bit_err_period = reg;
	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x46, &reg);
	bit_err_period = (bit_err_period<<8)|reg;

	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x6d, &reg);
	bit_err = reg;
	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x6c, &reg);
	bit_err = (bit_err<<8)|reg;
	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x6b, &reg);
	bit_err = (bit_err<<8)|reg;
	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x6a, &reg);
	bit_err = (bit_err<<8)|reg;

	mtk_demod_mbx_dvb_write(BACKEND_REG_BASE+0x03, reg_frz);

	if (bit_err_period == 0)
		bit_err_period = 1;

	msf_numerator.data = bit_err_period;
	msf_numerator.exp = 0;
	msf_denominator.data = 192512; /* 128*188*8 */
	msf_denominator.exp = 0;
	msf_denominator = mtk_demod_float_op(msf_numerator,
		msf_denominator, MULTIPLY);

	msf_numerator.data = bit_err;
	msf_numerator.exp = 0;
	*msf_ber = mtk_demod_float_op(msf_numerator,
		msf_denominator, DIVIDE);

	msf_denominator.data = 100000;
	msf_denominator.exp = 0;
	*msf_ber = mtk_demod_float_op(*msf_ber, msf_denominator, MULTIPLY);
	msf_denominator.data = 1000000;
	msf_denominator.exp = 0;
	*msf_ber = mtk_demod_float_op(*msf_ber, msf_denominator, MULTIPLY);
}

static int intern_dvbc_get_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct ms_float_st msf_snr;

	if (fec_lock == 0) {
		*snr = 0;
		return 0;
	}

	intern_dvbc_get_snr_ms_format(&msf_snr);

	*snr = (u16)mtk_demod_float_to_s64(msf_snr);


	return 0;
}

static int intern_dvbc_ger_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct ms_float_st msf_ber;
	struct ms_float_st msf_denominator;

	if (fec_lock == 0) {
		*ber = 1;
		return 0;
	}

	intern_dvbc_get_ber_ms_format(&msf_ber);

	/* 10^9 */
	msf_denominator.data = 100;
	msf_denominator.exp = 0;
	msf_ber = mtk_demod_float_op(msf_ber,
			msf_denominator, DIVIDE);

	*ber = (u32)mtk_demod_float_to_s64(msf_ber);

	return 0;
}

static int intern_dvbc_get_sqi(struct dvb_frontend *fe, u16 *sqi)
{
	u8 QAM;
	u16 cn_rec, ber_sqi;
	s16 cn_rel, cn_ref;
	s64 s64ber, s64tmp;
	u64 u64tmp;

	struct ms_float_st msf_ber = {0, 0};
	struct ms_float_st msf_tmp = {0, 0};

	if (fec_lock == 0) {
		*sqi = 0;
		return 0;
	}

	intern_dvbc_get_ber_ms_format(&msf_ber);

	if (msf_ber.data == 0) {
		/* min ber = 4.329-7 */
		s64ber = 43290;
	} else {
		s64ber = mtk_demod_float_to_s64(msf_ber);
	}

	if (s64ber > 100000000) {
		ber_sqi = 0;
	} else if (s64ber < 10000) {
		msf_ber.data = (s32)s64ber;
		msf_ber.exp = 0;
		msf_tmp.data = 10000;
		msf_tmp.exp = 0;
		msf_ber = mtk_demod_float_op(msf_ber, msf_tmp, DIVIDE);
		msf_tmp.data = 100000;
		msf_tmp.exp = 0;
		msf_ber = mtk_demod_float_op(msf_ber, msf_tmp, DIVIDE);
		msf_tmp.data = 1;
		msf_tmp.exp = 0;
		msf_ber = mtk_demod_float_op(msf_tmp, msf_ber, DIVIDE);

		s64tmp = mtk_demod_float_to_s64(msf_ber);

		u64tmp = log10approx100((u64)s64tmp);
		u64tmp = u64tmp*20-4000;

		ber_sqi = (u16)u64tmp;
	} else {
		ber_sqi = 10000;
	}

	intern_dvbc_get_snr(fe, &cn_rec);

	mtk_demod_mbx_dvb_read(0x20d0, &QAM);
	cn_ref = dvbc_sqi_db_nordigp1_X10[QAM];

	cn_rel = cn_rec - cn_ref;

	if (cn_rel < -70) {
		*sqi = 0;
	} else if (cn_rel < 30) {
		/* (ber_sqi*((cn_rel - 3.0)/10.0 + 1.0)) */
		u64tmp = (u64)(ber_sqi*((cn_rel-30) + 100));
		*sqi = (u16)(u64tmp/10000);
	} else {
		*sqi = 100;
	}


	return 0;
}

static int intern_dvbc_get_ssi(struct dvb_frontend *fe, u16 *ssi,
			int rf_power_dbm)
{
	u8 QAM;
	s32 s32tmp = 0;
	s16 ch_power_db = 0;
	s16 ch_power_ref = 0;
	s16 ch_power_db_rel = 0;

	if (fec_lock == 0) {
		*ssi = 0;
		return 0;
	}

	mtk_demod_mbx_dvb_read(0x20d0, &QAM);

	ch_power_ref = dvbc_ssi_dbm_nordigp1_X10[QAM];

	ch_power_db = rf_power_dbm*10;
	ch_power_db_rel = ch_power_db - ch_power_ref;

	if (ch_power_db_rel < -150) {
		*ssi = 0;
	} else if (ch_power_db_rel < 0) {
		s32tmp = ch_power_db_rel;
		s32tmp = (2*(s32tmp+150));
		s32tmp = s32tmp/3;

		*ssi = s32tmp/10;
	} else if (ch_power_db_rel < 200) {
		s32tmp = ch_power_db_rel;
		s32tmp = 4*s32tmp+100;

		*ssi = s32tmp/10;
	} else if (ch_power_db_rel < 350) {
		s32tmp = ch_power_db_rel;
		s32tmp = 2*(s32tmp-200);
		s32tmp = s32tmp/3;
		s32tmp = s32tmp+900;

		*ssi = s32tmp/10;
	} else {
		*ssi = 100;
	}
}

static void intern_dvbc_get_freq_offset(struct dvb_frontend *fe, s16 *cfo)
{
	u8 reg;
	u16 if_reg;
	u32 fc_over_fs_reg;

	struct ms_float_st msf_cfo;
	struct ms_float_st msf_numerator;
	struct ms_float_st msf_denominator;


	mtk_demod_mbx_dvb_read(TDF_REG_BASE + 0x57, &reg);
	fc_over_fs_reg = reg;
	mtk_demod_mbx_dvb_read(TDF_REG_BASE + 0x56, &reg);
	fc_over_fs_reg = (fc_over_fs_reg<<8)|reg;
	mtk_demod_mbx_dvb_read(TDF_REG_BASE + 0x55, &reg);
	fc_over_fs_reg = (fc_over_fs_reg<<8)|reg;
	mtk_demod_mbx_dvb_read(TDF_REG_BASE + 0x54, &reg);
	fc_over_fs_reg = (fc_over_fs_reg<<8)|reg;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_DVBC_CFG_FIF_H, &reg);
	if_reg = reg;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_DVBC_CFG_FIF_L, &reg);
	if_reg = (if_reg<<8)|reg;

	msf_numerator.data = fc_over_fs_reg;
	msf_numerator.exp = 0;
	msf_denominator.data = 48000;
	msf_denominator.exp = 0;
	msf_numerator = mtk_demod_float_op(msf_numerator,
						msf_denominator, MULTIPLY);

	msf_denominator.data = 134217728;
	msf_denominator.exp = 0;
	msf_numerator = mtk_demod_float_op(msf_numerator,
						msf_denominator, DIVIDE);

	msf_denominator.data = if_reg;
	msf_denominator.exp = 0;
	msf_cfo = mtk_demod_float_op(msf_numerator, msf_denominator, MINUS);

	*cfo = (s16)mtk_demod_float_to_s64(msf_cfo);
}

static void intern_dvbc_get_pkt_err(struct dvb_frontend *fe, u16 *err)
{
	u8 reg, reg_frz;

	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x03, &reg_frz);
	mtk_demod_mbx_dvb_write(BACKEND_REG_BASE+0x03, reg_frz|0x03);

	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x67, &reg);
	*err = reg;
	mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x66, &reg);
	*err = (*err << 8) | reg;

	reg_frz = reg_frz&(~0x03);
	mtk_demod_mbx_dvb_write(BACKEND_REG_BASE+0x03, reg_frz);
}

static int intern_dvbc_reset(void)
{
	int ret = 0;

	ret = intern_dvbc_soft_stop();

    /* reset DMD_MCU */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x01);
	mtk_demod_delay_ms(5);
	/* clear MB_CNTL */
	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0x00);

	mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x00);
	mtk_demod_delay_ms(5);

	if (mtk_demod_mbx_dvb_wait_handshake())
		return -ETIMEDOUT;
	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0x00);

	pr_info("[%s] is called\n", __func__);

	return ret;
}

void m7332_dmd_hal_dvbc_init_clk(struct dvb_frontend *fe)
{
	u8 reg;
	u16 u16tmp;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

	dvbc_fw_addr = demod_prvi->dram_base_addr;
	pr_notice("@@@@@@@@@@@ dvbc_fw_addr address = 0x%x\n",
		dvbc_fw_addr);


	mtk_demod_write_byte(0x103c0e, 0x00);
	mtk_demod_write_byte(0x101e39, 0x00);

	mtk_demod_write_byte(0x101e68,
		(mtk_demod_read_byte(0x101e68)&(~0x03)));

	mtk_demod_write_byte(0x112003,
		(mtk_demod_read_byte(0x112003)&(~0x20)));

	mtk_demod_write_byte(0x10331e, 0x10);

	mtk_demod_write_byte(0x103301, 0x11);
	mtk_demod_write_byte(0x103300, 0x13);

	mtk_demod_write_byte(0x103309, 0x00);
	mtk_demod_write_byte(0x103308, 0x00);

	mtk_demod_write_byte(0x103302, 0x01);
	mtk_demod_write_byte(0x103302, 0x00);

	mtk_demod_write_byte(0x103321, 0x00);
	mtk_demod_write_byte(0x103320, 0x00);

	mtk_demod_write_byte(0x103314, 0x04);

	mtk_demod_write_byte(0x111f01, 0x00);
	mtk_demod_write_byte(0x111f00, 0x0d);

	mtk_demod_write_byte(0x153de0, 0x23);
	mtk_demod_write_byte(0x153de1, 0x21);

	mtk_demod_write_byte(0x153de4, 0x01);
	mtk_demod_write_byte(0x153de6, 0x11);

	mtk_demod_write_byte(0x153d01, 0x00);
	mtk_demod_write_byte(0x153d00, 0x00);

	mtk_demod_write_byte(0x153d05, 0x00);
	mtk_demod_write_byte(0x153d04, 0x00);

	mtk_demod_write_byte(0x153d03, 0x00);
	mtk_demod_write_byte(0x153d02, 0x00);

	mtk_demod_write_byte(0x153d07, 0x00);
	mtk_demod_write_byte(0x153d06, 0x00);

	u16tmp = (u16)(dvbc_fw_addr>>16);
	mtk_demod_write_byte(0x153d1b, (u8)(u16tmp>>8));
	mtk_demod_write_byte(0x153d1a, (u8)u16tmp);

	mtk_demod_write_byte(0x153d09, 0x00);
	mtk_demod_write_byte(0x153d08, 0x00);

	mtk_demod_write_byte(0x153d0d, 0x00);
	mtk_demod_write_byte(0x153d0c, 0x00);

	mtk_demod_write_byte(0x153d0b, 0x00);
	mtk_demod_write_byte(0x153d0a, 0x00);

	mtk_demod_write_byte(0x153d0f, 0x7f);
	mtk_demod_write_byte(0x153d0e, 0xff);

	mtk_demod_write_byte(0x153d18, 0x04);

	reg = mtk_demod_read_byte(0x112001);
	reg &= (~0x10);
	mtk_demod_write_byte(0x112001, reg);

	mtk_demod_write_byte(0x153d1c, 0x01);
/*
 *	if(dvbc_fw_addr&MIU_INTERVAL)
 *	{
 *		mtk_demod_write_byte(0x1127ae,0x01);
 *		pr_info("[DVBC][MIU 1] Select ADDR for miu 1 !!\n");
 *	}
 *	else
 *	{
 *		mtk_demod_write_byte(0x1127ae,0x00);
 *		pr_info("[DVBC][MIU 0] Select ADDR for miu 0 !!\n");
 *	}
 */

	mtk_demod_write_byte(0x101e39, 0x03);
	mtk_demod_write_byte(0x103c0e, 0x01);
}

int m7332_dmd_hal_dvbc_load_code(void)
{
	int ret = 0;

	/* load code to DRAM */
	/* reset VD_MCU */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x00,  0x01);
	/* disable SRAM */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x01,  0x00);
	/* enable "vdmcu51_if" */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03,  0x50);
	/* enable auto-increase */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03,  0x51);
	/* sram address low byte */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x04,  0x00);
	/* sram address high byte */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x05,  0x00);

	dram_code_virt_addr = ioremap_wc(dvbc_fw_addr+0x20000000,
								0x10000);
	/* load code to SDRAM */
	memcpy(dram_code_virt_addr, &dvbc_table[0],
			sizeof(dvbc_table));

	iounmap(dram_code_virt_addr);

	/* disable auto-increase */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03, 0x50);
	/* disable "vdmcu51_if" */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03, 0x00);
	/* release VD_MCU */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x00);

	if (mtk_demod_mbx_dvb_wait_handshake())
		return -ETIMEDOUT;
	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0x00);


	return ret;
}

int m7332_dmd_hal_dvbc_config(struct dvb_frontend *fe, u32 if_freq)
{
	u8 reg_autosym, reg_symrate_l, reg_symrate_h;
	u32 ret = 0;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	pr_info("[%s] is called\n", __func__);

	/* QAM mode */
	if (c->modulation == QAM_AUTO) {
		mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_OP_AUTO_SCAN_QAM, 1);
	} else {
		mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_OP_AUTO_SCAN_QAM, 0);
		mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_CFG_QAM,
			(c->modulation)-1);
	}

	/* Symbol Rate */
	if (c->symbol_rate == 0) {
		reg_autosym = 1;
		reg_symrate_l = DEFAULT_SYMBOL_RATE & 0xff;
		reg_symrate_h = (DEFAULT_SYMBOL_RATE>>8) & 0xff;
	} else {
		reg_autosym = 0;
		reg_symrate_l = (u8)(((c->symbol_rate)/1000) & 0xff);
		reg_symrate_h = (u8)((((c->symbol_rate)/1000)>>8) & 0xff);
	}
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_OP_AUTO_SCAN_SYM_RATE,
		reg_autosym);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_CFG_BW0_L, reg_symrate_l);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_CFG_BW0_H, reg_symrate_h);

	/* IF */
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_CFG_FIF_L,
		(if_freq)&0xff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_CFG_FIF_H,
		(if_freq>>8)&0xff);

	/* FC */
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_CFG_FC_L,
		(48000-if_freq)&0xff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_CFG_FC_H,
		((48000-if_freq)>>8)&0xff);

	/* AGC ref */
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_AGC_REF_L, 0x00);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_AGC_REF_H, 0x06);

	/* Reset Demod */
	ret = intern_dvbc_reset();

	fec_lock = 0;
	tr_lock = 0;

	/* Active state machine */
	mtk_demod_write_byte(MB_REG_BASE + (0x0e)*2, 0x01);

	scan_start_time = mtk_demod_get_time();

	return ret;
}

int m7332_dmd_hal_dvbc_exit(void)
{
	int ret = 0;

	ret = intern_dvbc_soft_stop();

	/* [0]: reset demod 0 */
	/* [1]: reset demod 1 */
	mtk_demod_write_byte(0x101e82, 0x00);
	mtk_demod_delay_ms(1);
	mtk_demod_write_byte(0x101e82, 0x11);

	/* SRAM End Address */
	mtk_demod_write_byte(0x153d07, 0xff);
	mtk_demod_write_byte(0x153d06, 0xff);
	/* DRAM Disable */
	mtk_demod_write_byte(0x153d18,
		mtk_demod_read_byte(0x153d18)&(~0x04));

	return ret;
}

int m7332_dmd_hal_dvbc_get_frontend(struct dvb_frontend *fe,
				struct dtv_frontend_properties *p)
{
	u8  reg;
	u16 snr, err, sqi;
	s16 cfo;
	u32 ber;
	int ret = 0;
	struct dtv_fe_stats *stats;

	if (mtk_demod_read_byte(MB_REG_BASE + (0x0e)*2) != 0x01) {
		pr_info("[%s][%d] Demod is inavtive !\n", __func__, __LINE__);
		return 0;
	}

	/* RF power level (dBm) */
/*	if (fe->ops.tuner_ops.get_rf_strength) {
 *		ret = fe->ops.tuner_ops.get_rf_strength(fe, &rf_power_level);
 *		stats = &p->strength;
 *		stats->len = 1;
 *		stats->stat[0].svalue = rf_power_level;
 *		stats->stat[0].scale = FE_SCALE_DECIBEL;
 *	} else {
 *		pr_err("Failed to get tuner_ops.get_rf_strength\r\n");
 *		return -ENONET;
 *	}
 */

	/* SNR */
	intern_dvbc_get_snr(fe, &snr);
	stats = &p->cnr;
	stats->len = 1;
	stats->stat[0].svalue = snr;
	stats->stat[0].scale = FE_SCALE_DECIBEL;

	/* Packet Error */
	intern_dvbc_get_pkt_err(fe, &err);
	stats = &p->block_error;
	stats->len = 1;
	stats->stat[0].uvalue = err;
	stats->stat[0].scale = FE_SCALE_COUNTER;

	/* cfo */
	intern_dvbc_get_freq_offset(fe, &cfo);
	p->frequency_offset = cfo*1000;  /* KHz to Hz */

	/* SQI */
	intern_dvbc_get_sqi(fe, &sqi);
	stats = &p->quality;
	stats->len = 1;
	stats->stat[0].uvalue = sqi;
	stats->stat[0].scale = FE_SCALE_RELATIVE;


	mtk_demod_mbx_dvb_read(0x20d0, &reg);
	/* QAM mode */
	switch (reg) {
	case 0:
		p->modulation = QAM_16;
		break;
	case 1:
		p->modulation = QAM_32;
		break;
	case 2:
		p->modulation = QAM_64;
		break;
	case 3:
		p->modulation = QAM_128;
		break;
	case 4:
		p->modulation = QAM_256;
		break;
	}

	/* Symbol Rate */
	mtk_demod_mbx_dvb_read(0x20d1, &reg);
	p->symbol_rate = reg;
	mtk_demod_mbx_dvb_read(0x20d2, &reg);
	p->symbol_rate |= (reg << 8);

	if (abs(p->symbol_rate - 6900) < 2)
		p->symbol_rate = 6900;
	if (abs(p->symbol_rate - 6875) < 2)
		p->symbol_rate = 6875;

	p->symbol_rate = p->symbol_rate * 1000;

	return ret;
}

int m7332_dmd_hal_dvbc_read_status(struct dvb_frontend *fe,
			enum fe_status *status)
{
	u8  reg;
	u32 ret = 0;
	/* struct dtv_frontend_properties *c = &fe->dtv_property_cache; */

	if (mtk_demod_read_byte(MB_REG_BASE + (0x0e)*2) != 0x01) {
		pr_info("[%s][%d] Demod is inavtive !\n", __func__, __LINE__);
		return 0;
	}

	mtk_demod_mbx_dvb_read(FEC_REG_BASE + 0xE0, &reg);

	if (reg > 0x05) {
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
		if (tr_lock == 0) {
			pr_notice("\n\n>>>>>>>> TR lock !!!\n\n");
			tr_lock = 1;
		}
		if (reg == 0x0C) {
			*status |= FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
			if (fec_lock == 0) {
				intern_dvbc_adaptive_ts_config(fe);
				pr_notice("\n\n>>>>>>>> DVBC lock !!!\n\n");
			}
			last_lock_time = mtk_demod_get_time();
			fec_lock = 1;
		} else {
			if (fec_lock == 1) {
				if ((mtk_demod_get_time()
					- last_lock_time) < 1000)
					*status |= FE_HAS_VITERBI | FE_HAS_SYNC
							| FE_HAS_LOCK;
				else
					fec_lock = 0;
			}
		}
	} else {
		if ((mtk_demod_get_time() - scan_start_time)
			> DMD_DVBC_TR_TIMEOUT) {
			*status = FE_TIMEDOUT;
			pr_notice("\n\n>>>>>>>> DVBC no channel !!!\n\n");
		} else {
			*status = 0;
			tr_lock = 0;
			fec_lock = 0;
		}
	}

	if ((tr_lock == 0) || (fec_lock == 0)) {
		if ((mtk_demod_get_time() - scan_start_time)
			> DMD_DVBC_LOCK_TIMEOUT) {
			*status = FE_TIMEDOUT;
			pr_notice("\n\n>>>>>>>> DVBC lock timeout !!!\n\n");
		}
	}


	return ret;
}

ssize_t m7332_demod_hal_dvbc_get_information(struct device_driver *driver,
			char *buf)
{
	u8 reg;
	u16 demod_rev;
	u32 ts_rate;

	pr_info("[%s] is called\n", __func__);

	/* Demod Rev. */
	mtk_demod_mbx_dvb_read(0x20c1, &reg);
	demod_rev = reg;
	mtk_demod_mbx_dvb_read(0x20c2, &reg);
	demod_rev = (demod_rev<<8) | reg;

	if (fec_lock) {
		/* TS Rate */
		mtk_demod_mbx_dvb_read_dsp(E_DMD_DVBC_TS_DATA_RATE_3, &reg);
		ts_rate = reg;
		mtk_demod_mbx_dvb_read_dsp(E_DMD_DVBC_TS_DATA_RATE_2, &reg);
		ts_rate = (ts_rate<<8) | reg;
		mtk_demod_mbx_dvb_read_dsp(E_DMD_DVBC_TS_DATA_RATE_1, &reg);
		ts_rate = (ts_rate<<8) | reg;
		mtk_demod_mbx_dvb_read_dsp(E_DMD_DVBC_TS_DATA_RATE_0, &reg);
		ts_rate = (ts_rate<<8) | reg;
		ts_rate = ts_rate/1010;

	}

	pr_info("Demod Rev. = 0x%x\n", demod_rev);
	pr_info("TS Rate = %d\n", ts_rate);


	return scnprintf(buf, PAGE_SIZE, "%x %d\n", demod_rev, ts_rate);
}
