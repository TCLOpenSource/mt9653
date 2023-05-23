// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * MediaTek	Inc. (C) 2020. All rights	reserved.
 */

#include <asm/io.h>
#include <linux/string.h>
#include <linux/mm_types.h>
#include <linux/timekeeping.h>
#include <linux/math64.h>

#include "demod_drv_dvbc.h"
#include "demod_hal_dvbc.h"

#define DMD_DVBC_TR_TIMEOUT     500
#define DMD_DVBC_LOCK_TIMEOUT  7000
#define DEFAULT_SYMBOL_RATE    6900


static unsigned long dvbc_fw_addr;
static u64 post_error_bit_count;
static u64 post_total_bit_count;

static u8 *dram_code_virt_addr;
static u8 fec_lock;
static u8 tr_lock;
static u8 tuner_spectrum_inversion;
static u16 ip_version;
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
		intp = div64_u64(intp,
			(logapproxtablex100[indx]-logapproxtablex100[indx-1]));
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
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;
	u8 divider = 0;
	u8 ts_enable = 0;
	u8 clk_src = 0;
	u8 reg = 0;

	reg = mtk_demod_read_byte(MB_REG_BASE + 0x15);
	clk_src = (reg >> 6);
	divider = reg & 0x1f;

	if (clk_src == 1)
		pr_info("CLK Source: 288 MHz\n");
	else
		pr_info("CLK Source: 172 MHz\n");
	pr_info(">>>[%s] = 0x%x<<<\n",  __func__, divider);

	/* disable TS module work */
	reg = readb(demod_prvi->virt_ts_base_addr + 0x04);
	writeb(reg | 0x01, demod_prvi->virt_ts_base_addr + 0x04);
	writew(divider, demod_prvi->virt_ts_base_addr);
	/* phase_tune setting */
	reg = readb(demod_prvi->virt_ts_base_addr + 0x01);
	reg = (reg&(~0x07)) | clk_src;
	writeb(reg | 0x40, demod_prvi->virt_ts_base_addr + 0x01);
	writeb((divider>>1), demod_prvi->virt_ts_base_addr + 0x11);
	/* enable TS module work */
	reg = readb(demod_prvi->virt_ts_base_addr + 0x04);
	writeb(reg & ~0x01, demod_prvi->virt_ts_base_addr + 0x04);

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

	post_error_bit_count = bit_err;
	post_total_bit_count = (u64)bit_err_period*192512;

	msf_denominator.data = 100000;
	msf_denominator.exp = 0;
	*msf_ber = mtk_demod_float_op(*msf_ber, msf_denominator, MULTIPLY);
	msf_denominator.data = 1000000;
	msf_denominator.exp = 0;
	*msf_ber = mtk_demod_float_op(*msf_ber, msf_denominator, MULTIPLY);
}

//static int intern_dvbc_get_snr(struct dvb_frontend *fe, u16 *snr)
static int intern_dvbc_get_snr(u16 *snr)
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

//static int intern_dvbc_ger_ber(struct dvb_frontend *fe, u32 *ber)
static int intern_dvbc_ger_ber(u32 *ber)
{
	struct ms_float_st msf_ber;
	struct ms_float_st msf_denominator;

	if (fec_lock == 0) {
		*ber = 1;
		post_error_bit_count = 1;
		post_total_bit_count = 1;
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

//static int intern_dvbc_get_sqi(struct dvb_frontend *fe, u16 *sqi)
static int intern_dvbc_get_sqi(u16 *sqi)

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

	//intern_dvbc_get_snr(fe, &cn_rec);
	intern_dvbc_get_snr(&cn_rec);

	mtk_demod_mbx_dvb_read(0x20d0, &QAM);
	cn_ref = dvbc_sqi_db_nordigp1_X10[QAM];

	cn_rel = cn_rec - cn_ref;

	if (cn_rel < -70) {
		*sqi = 0;
	} else if (cn_rel < 30) {
		/* (ber_sqi*((cn_rel - 3.0)/10.0 + 1.0)) */
		u64tmp = (u64)(ber_sqi*((cn_rel-30) + 100));
		*sqi = (u16)(div64_u64(u64tmp, 10000));
	} else {
		*sqi = 100;
	}


	return 0;
}

static int intern_dvbc_get_ssi(struct dvb_frontend *fe, u16 *ssi, int rf_power_dbm)
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

	return 0;
}

//static void intern_dvbc_get_freq_offset(struct dvb_frontend *fe, s16 *cfo)
static void intern_dvbc_get_freq_offset(s16 *cfo)
{
	u8 reg;
	u16 if_reg;
	u32 fc_over_fs_reg;

	struct ms_float_st msf_cfo;
	struct ms_float_st msf_numerator;
	struct ms_float_st msf_denominator;

	//struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

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
	//if (demod_prvi->spectrum_inversion)
	if (tuner_spectrum_inversion)
		msf_cfo = mtk_demod_float_op(msf_denominator, msf_numerator, MINUS);
	else
		msf_cfo = mtk_demod_float_op(msf_numerator, msf_denominator, MINUS);

	*cfo = (s16)mtk_demod_float_to_s64(msf_cfo);
}

//static void intern_dvbc_get_pkt_err(struct dvb_frontend *fe, u16 *err)
static void intern_dvbc_get_pkt_err(u16 *err)
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

static void intern_dvbc_get_agc_gain(u8 *agc)
{
	mtk_demod_mbx_dvb_write(TDF_REG_BASE+(0x11)*2, 0x03);
	mtk_demod_mbx_dvb_write(TDF_REG_BASE+(0x02)*2+1, 0x80);
	mtk_demod_mbx_dvb_read(TDF_REG_BASE+(0x12)*2+1, agc);
	mtk_demod_mbx_dvb_write(TDF_REG_BASE+(0x02)*2+1, 0x00);
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

void dmd_hal_dvbc_init_clk(struct dvb_frontend *fe)
{
	u8 reg;
	u16 u16tmp;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

	dvbc_fw_addr = demod_prvi->dram_base_addr;
	pr_notice("@@@@@@@@@@@ dvbc_fw_addr address = 0x%lx\n",
		dvbc_fw_addr);
	dram_code_virt_addr = demod_prvi->virt_dram_base_addr;
	pr_notice("@@@@@@@@@@@ va = 0x%llx\n",
		dram_code_virt_addr);


	/* [9:9][6:6] */
	u16tmp = readw(demod_prvi->virt_clk_base_addr + 0x14e4);
	u16tmp |= 0x240;
	writew(u16tmp, demod_prvi->virt_clk_base_addr + 0x14e4);

	/* [9:8][2:0] */
	u16tmp = readw(demod_prvi->virt_clk_base_addr + 0x1d8);
	u16tmp = (u16tmp&0xfcf8) | 0x0004;
	writew(u16tmp, demod_prvi->virt_clk_base_addr + 0x1d8);
	/* [2:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x2b8);
	reg &= 0xf8;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x2b8);
	/* [1:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x2c0);
	reg &= 0xfc;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x2c0);
	/* [1:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x2c8);
	reg &= 0xfc;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x2c8);
	/* [1:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x2d8);
	reg &= 0xfc;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x2d8);
	/* [1:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x510);
	reg &= 0xfc;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x510);
	/* [1:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0xab8);
	reg &= 0xfc;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0xab8);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x1598);
	reg |= 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x1598);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x1740);
	reg |= 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x1740);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x19c8);
	reg |= 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x19c8);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x19cc);
	reg |= 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x19cc);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x19d0);
	reg |= 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x19d0);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x19d4);
	reg |= 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x19d4);
	/* [9:8][4:0] */
	u16tmp = readw(demod_prvi->virt_clk_base_addr + 0x3040);
	u16tmp = (u16tmp&0xfce0) | 0x0004;
	writew(u16tmp, demod_prvi->virt_clk_base_addr + 0x3040);
	/* [9:8][4:0] */
	u16tmp = readw(demod_prvi->virt_clk_base_addr + 0x3110);
	u16tmp &= 0xfce0;
	writew(u16tmp, demod_prvi->virt_clk_base_addr + 0x3110);
	/* [1:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x31c0);
	reg &= 0xfc;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x31c0);
	/* [5:5] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x3b4c);
	reg |= 0x20;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x3b4c);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x3b5c);
	reg |= 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x3b5c);
	/* [2:2] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x3b74);
	reg |= 0x04;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x3b74);

	ip_version = demod_prvi->ip_version;
	mtk_demod_write_byte(DEMOD_TOP_REG_BASE+0xea, (u8)ip_version);
	mtk_demod_write_byte(DEMOD_TOP_REG_BASE+0xeb, (u8)(ip_version>>8));
	if (ip_version > 0x0100) {
		/* [0:0] */
		writeb(0x00, demod_prvi->virt_sram_base_addr);

		/* [3:0] */
		reg = readb(demod_prvi->virt_pmux_base_addr);
		reg = (reg&0xf0) | 0x03;
		writeb(reg, demod_prvi->virt_pmux_base_addr);
		/* [11:8] */
		u16tmp = readw(demod_prvi->virt_vagc_base_addr);
		u16tmp = (u16tmp&0xf0ff) | 0x0300;
		writew(u16tmp, demod_prvi->virt_vagc_base_addr);
	} else {
		writew(0x00fc, demod_prvi->virt_sram_base_addr);
	}

	/* [9:8][4:0] */
	u16tmp = readw(demod_prvi->virt_clk_base_addr + 0x3110);
	u16tmp = (u16tmp&0xfce0) | 0x0014;
	writew(u16tmp, demod_prvi->virt_clk_base_addr + 0x3110);

	writew(0x1117, demod_prvi->virt_ts_base_addr);
	/* [3:0] */
	reg = readb(demod_prvi->virt_ts_base_addr + 0x08);
	reg &= 0xf0;
	writeb(reg, demod_prvi->virt_ts_base_addr + 0x08);
	/* [11:8] */
	u16tmp = readw(demod_prvi->virt_ts_base_addr + 0x0c);
	u16tmp &= 0xf0ff;
	writew(u16tmp, demod_prvi->virt_ts_base_addr + 0x0c);
	/* [0:0] */
	reg = readb(demod_prvi->virt_ts_base_addr + 0x04);
	reg |= 0x01;
	writeb(reg, demod_prvi->virt_ts_base_addr + 0x04);
	reg &= 0xfe;
	writeb(reg, demod_prvi->virt_ts_base_addr + 0x04);


	mtk_demod_write_byte(MCU2_REG_BASE+0xe0, 0x23);
	mtk_demod_write_byte(MCU2_REG_BASE+0xe1, 0x21);

	mtk_demod_write_byte(MCU2_REG_BASE+0xe4, 0x01);
	mtk_demod_write_byte(MCU2_REG_BASE+0xe6, 0x11);

	mtk_demod_write_byte(MCU2_REG_BASE+0x01, 0x00);
	mtk_demod_write_byte(MCU2_REG_BASE+0x00, 0x00);

	mtk_demod_write_byte(MCU2_REG_BASE+0x05, 0x00);
	mtk_demod_write_byte(MCU2_REG_BASE+0x04, 0x00);

	mtk_demod_write_byte(MCU2_REG_BASE+0x03, 0x00);
	mtk_demod_write_byte(MCU2_REG_BASE+0x02, 0x00);

	mtk_demod_write_byte(MCU2_REG_BASE+0x07, 0x00);
	mtk_demod_write_byte(MCU2_REG_BASE+0x06, 0x00);

	u16tmp = (u16)(dvbc_fw_addr>>16);
	mtk_demod_write_byte(MCU2_REG_BASE+0x1b, (u8)(u16tmp>>8));
	mtk_demod_write_byte(MCU2_REG_BASE+0x1a, (u8)u16tmp);
	u16tmp = (u16)(dvbc_fw_addr>>32);
	mtk_demod_write_byte(MCU2_REG_BASE+0x20, (u8)u16tmp);

	mtk_demod_write_byte(MCU2_REG_BASE+0x09, 0x00);
	mtk_demod_write_byte(MCU2_REG_BASE+0x08, 0x00);

	mtk_demod_write_byte(MCU2_REG_BASE+0x0d, 0x00);
	mtk_demod_write_byte(MCU2_REG_BASE+0x0c, 0x00);

	mtk_demod_write_byte(MCU2_REG_BASE+0x0b, 0x00);
	mtk_demod_write_byte(MCU2_REG_BASE+0x0a, 0x01);

	mtk_demod_write_byte(MCU2_REG_BASE+0x0f, 0xff);
	mtk_demod_write_byte(MCU2_REG_BASE+0x0e, 0xff);

	mtk_demod_write_byte(MCU2_REG_BASE+0x18, 0x04);

	reg = readb(demod_prvi->virt_riu_base_addr+0x4001);
	reg &= (~0x10);
	writeb(reg, demod_prvi->virt_riu_base_addr+0x4001);

	mtk_demod_write_byte(MCU2_REG_BASE+0x1c, 0x01);
}

int dmd_hal_dvbc_load_code(void)
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


	/* load code to SDRAM */
	memcpy(dram_code_virt_addr, &dvbc_table[0],
			sizeof(dvbc_table));

	/* disable auto-increase */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03, 0x50);
	/* disable "vdmcu51_if" */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03, 0x00);
	/* release VD_MCU */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x00);

	if (mtk_demod_mbx_dvb_wait_handshake()) {
		pr_err("############# 1st\n");
		mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x01);
		mtk_demod_delay_ms(1);
		mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x00);
		if (mtk_demod_mbx_dvb_wait_handshake()) {
			pr_err("############# 2nd\n");
			return -ETIMEDOUT;
		}
	}

	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0x00);


	return ret;
}

int dmd_hal_dvbc_config(struct dvb_frontend *fe, u32 if_freq)
{
	u8 reg_autosym, reg_symrate_l, reg_symrate_h, reg;
	u32 ret = 0;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

	pr_info("[%s] is called\n", __func__);

	/* Reset Demod */
	ret = intern_dvbc_reset();

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

	/* AGC polarity */
	mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBC_IF_INV_PWM_OUT_EN, demod_prvi->agc_polarity);

	tuner_spectrum_inversion = demod_prvi->spectrum_inversion;

	if (demod_prvi->if_gain == 1) {
		if (ip_version >= 0x0200) {
			/* [15:13] */
			mtk_demod_mbx_dvb_read(DMDANA_REG_BASE + 0x9d, &reg);
			reg = reg&0x1f;
			mtk_demod_mbx_dvb_write(DMDANA_REG_BASE + 0x9d, reg);
		} else {
			/* [14:8] */
			mtk_demod_mbx_dvb_read(DMDANA_REG_BASE + 0xeb, &reg);
			reg = reg&0x80;
			mtk_demod_mbx_dvb_write(DMDANA_REG_BASE + 0xeb, reg);
			/* [2:2] */
			mtk_demod_mbx_dvb_read(DMDANA_REG_BASE + 0x52, &reg);
			reg = reg | 0x04;
			mtk_demod_mbx_dvb_write(DMDANA_REG_BASE + 0x52, reg);
			/* [4:0] */
			mtk_demod_mbx_dvb_read(DMDANA_REG_BASE + 0x16, &reg);
			reg = (reg&0xE0) | 0x03;
			mtk_demod_mbx_dvb_write(DMDANA_REG_BASE + 0x16, reg);
		}
	}

	fec_lock = 0;
	tr_lock = 0;
	post_error_bit_count = 1;
	post_total_bit_count = 1;

	/* Active state machine */
	mtk_demod_write_byte(MB_REG_BASE + (0x0e)*2, 0x01);

	scan_start_time = mtk_demod_get_time();

	return ret;
}

int dmd_hal_dvbc_exit(struct dvb_frontend *fe)
{
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;
	int ret = 0;

	ret = intern_dvbc_soft_stop();

	/* [0]: reset demod 0 */
	/* [4]: reset demod 1 */
	writeb(0x00, demod_prvi->virt_reset_base_addr);
	mtk_demod_delay_ms(1);
	writeb(0x11, demod_prvi->virt_reset_base_addr);


	return ret;
}

int dmd_hal_dvbc_get_frontend(struct dvb_frontend *fe, struct dtv_frontend_properties *p)
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
	//intern_dvbc_get_snr(fe, &snr);
	intern_dvbc_get_snr(&snr);
	stats = &p->cnr;
	stats->len = 1;
	stats->stat[0].svalue = snr*100;
	stats->stat[0].scale = FE_SCALE_DECIBEL;

	/* Packet Error */
	//intern_dvbc_get_pkt_err(fe, &err);
	intern_dvbc_get_pkt_err(&err);
	stats = &p->block_error;
	stats->len = 1;
	stats->stat[0].uvalue = err;
	stats->stat[0].scale = FE_SCALE_COUNTER;

	/* cfo */
	//intern_dvbc_get_freq_offset(fe, &cfo);
	intern_dvbc_get_freq_offset(&cfo);
	p->frequency_offset = cfo*1000;  /* KHz to Hz */

	/* SQI */
	//intern_dvbc_get_sqi(fe, &sqi);
	intern_dvbc_get_sqi(&sqi);
	stats = &p->quality;
	stats->len = 1;
	stats->stat[0].uvalue = sqi;
	stats->stat[0].scale = FE_SCALE_RELATIVE;

	/*BER */
	//intern_dvbc_ger_ber(fe, &ber);
	intern_dvbc_ger_ber(&ber);
	p->post_ber = ber;
	/* numerator */
	stats = &p->post_bit_error;
	stats->len = 1;
	stats->stat[0].uvalue = post_error_bit_count;
	stats->stat[0].scale = FE_SCALE_COUNTER;
	/* denominator */
	stats = &p->post_bit_count;
	stats->len = 1;
	stats->stat[0].uvalue = post_total_bit_count;
	stats->stat[0].scale = FE_SCALE_COUNTER;

	/* AGC */
	intern_dvbc_get_agc_gain(&reg);
	p->agc_val = reg;

	ret |= mtk_demod_mbx_dvb_read(0x20d0, &reg);
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
	ret |= mtk_demod_mbx_dvb_read(0x20d1, &reg);
	p->symbol_rate = reg;
	ret |= mtk_demod_mbx_dvb_read(0x20d2, &reg);
	p->symbol_rate |= (reg << 8);

	if (abs(p->symbol_rate - 6900) < 2)
		p->symbol_rate = 6900;
	if (abs(p->symbol_rate - 6875) < 2)
		p->symbol_rate = 6875;

	p->symbol_rate = p->symbol_rate * 1000;

	return ret;
}

int dmd_hal_dvbc_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	u8  reg;
	u32 ret = 0;

	if (mtk_demod_read_byte(MB_REG_BASE + (0x0e)*2) != 0x01) {
		pr_info("[%s][%d] Demod is inavtive !\n", __func__, __LINE__);
		return 0;
	}

	ret |= mtk_demod_mbx_dvb_read(FEC_REG_BASE + 0xE0, &reg);

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

ssize_t demod_hal_dvbc_get_information(struct device_driver *driver, char *buf)
{
	u8 reg = 0, agc_val = 0, modulation = 0;
	u16 demod_rev = 0, cnr = 0, block_error = 0, quality = 0;
	s16 frequency_offset = 0;
	u32 ts_rate = 0, post_ber = 0, symbol_rate = 0;
	int ret = 0;

	pr_info("[%s] is called\n", __func__);

	/* Demod Rev. */
	mtk_demod_mbx_dvb_read(0x20c1, &reg);
	demod_rev = reg;
	mtk_demod_mbx_dvb_read(0x20c2, &reg);
	demod_rev = (demod_rev<<8) | reg;

	/* Freq. Offset */
	intern_dvbc_get_freq_offset(&frequency_offset);
	frequency_offset = frequency_offset*1000;  /* KHz to Hz */

	/* AGC */
	intern_dvbc_get_agc_gain(&agc_val);

	/* Symbol Rate */
	ret |= mtk_demod_mbx_dvb_read(0x20d1, &reg);
	symbol_rate = reg;
	ret |= mtk_demod_mbx_dvb_read(0x20d2, &reg);
	symbol_rate |= (reg << 8);

	if (abs(symbol_rate - 6900) < 2)
		symbol_rate = 6900;
	if (abs(symbol_rate - 6875) < 2)
		symbol_rate = 6875;

	symbol_rate = symbol_rate * 1000;

	/* Modulation Type */
	ret |= mtk_demod_mbx_dvb_read(0x20d0, &reg);
	switch (reg) {
	case 0:
		modulation = QAM_16;
		break;
	case 1:
		modulation = QAM_32;
		break;
	case 2:
		modulation = QAM_64;
		break;
	case 3:
		modulation = QAM_128;
		break;
	case 4:
		modulation = QAM_256;
		break;
	}

	if (fec_lock) {
		/* CNR */
		intern_dvbc_get_snr(&cnr);

		/* BER */
		intern_dvbc_ger_ber(&post_ber);

		/* UEC */
		intern_dvbc_get_pkt_err(&block_error);

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

		/* Signal quality */
		intern_dvbc_get_sqi(&quality);
	}

	// total 15 items (SM 11, SD 7, Common 3)
	pr_info("Demod Rev. = 0x%x\n", demod_rev);
	pr_info("Freq. Offset = %d\n", frequency_offset); // Hz
	pr_info("AGC = %d\n", agc_val);
	pr_info("Demod Lock = %d\n", fec_lock);
	pr_info("CNR = %d\n", cnr);
	pr_info("Symbol Rate = %d\n", symbol_rate); // Hz
	pr_info("BER = %d\n", post_ber);
	pr_info("UEC = %d\n", block_error);
	pr_info("Modulation Type = %d\n", modulation);
	pr_info("TS Rate = %d\n", ts_rate);
	pr_info("Signal quality = %d\n", quality);

	return scnprintf(buf, PAGE_SIZE,
		"%s%s%x%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s",
		SM_CHIP_REV, SM_COMMA, demod_rev, SM_END,
		SM_FREQ_OFFSET, SM_COMMA, frequency_offset, SM_END,
		SM_AGC, SM_COMMA, agc_val, SM_END, // SD_SM
		SM_DEMOD_LOCK, SM_COMMA, fec_lock, SM_END,
		SM_TS_LOCK, SM_COMMA, fec_lock, SM_END,
		SM_CNR, SM_COMMA, cnr, SM_END,
		SM_C_N, SM_COMMA, cnr, SM_END, // SD_SM
		SM_SYMBOL_RATE, SM_COMMA, symbol_rate, SM_END, // SD_SM
		SM_BER_BEFORE_RS, SM_COMMA, post_ber, SM_END,
		SD_PRE_RS, SM_COMMA, post_ber, SM_END,
		SM_UEC, SM_COMMA, block_error, SM_END, // SD_SM
		SM_MODULATION_TYPE, SM_COMMA, modulation, SM_END,
		SM_MODULATION, SM_COMMA, modulation, SM_END, //SD
		SM_TS_RATE, SM_COMMA, ts_rate, SM_END,
		SD_SIGNAL_QAULITY, SM_COMMA, quality, SM_END);
}
