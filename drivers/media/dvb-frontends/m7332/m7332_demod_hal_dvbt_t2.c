// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <asm/io.h>
#include <linux/string.h>
#include <linux/mm_types.h>
#include <linux/timekeeping.h>
#include "m7332_demod_drv_dvbt_t2.h"
#include "m7332_demod_hal_dvbt_t2.h"

#define MIU_INTERVAL     0x80000000
#define DMD_DVBT_T2_LOCK_TIMEOUT  7000
#define L1_READY  0x10


static u32  dvbt_t2_eq_addr;
static u32  dvbt_t2_tdi_addr;
static u32  dvbt_t2_djb_addr;
static u32  dvbt_t2_fw_addr;

static u8 *dram_code_virt_addr;
static u8 lock_signal = UNKNOWN_SIGNAL;
static u8 fec_lock;
static u8 p1_tps_lock;
static u32 scan_start_time;
static u32 first_lock_time;
static u32 last_lock_time;

s64 ber_filtered = -100000000000;


u8 dvbt_t2_com_table[] = {
    #include "fw_dmd_dvbt_t2_com.dat"
};
u8 dvbt_t2_bk0_table[] = {
    #include "fw_dmd_dvbt_t2_bk0.dat"
};
u8 dvbt_t2_bk1_table[] = {
    #include "fw_dmd_dvbt_t2_bk1.dat"
};

static u16 dvbt_sqi_dbm_nordigp1_x10[][5] = {
	{ 51, 69, 79, 89, 97},
	{ 108, 131, 146, 156, 160},
	{ 165, 187, 202, 216, 225},
};

static s16 dvbt_ssi_dbm_nordigp1_x10[][5] = {
	{ -930, -910, -900, -890, -880},
	{ -870, -850, -840, -830, -820},
	{ -820, -800, -780, -770, -760},
};

static s16 dvbt2_sqi_db_nordigp1_x10[][6] = {
	{ 35, 47, 56, 66, 72, 77},
	{ 87, 101, 114, 125, 133, 138},
	{ 130, 148, 162, 177, 187, 194},
	{ 170, 194, 208, 229, 243, 251},
};

static s16 dvbt2_ssi_dbm_nordigp1_x10[][6] = {
	{ -957, -944, -936, -926, -920, -915},
	{ -908, -891, -879, -867, -858, -852},
	{ -869, -846, -832, -814, -803, -797},
	{ -835, -804, -786, -760, -744, -733},
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

static int intern_dvbt_t2_soft_stop(void)
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
			pr_err("@@@@@ DVBT/T2 SoftStop Fail!\n");
			return -ETIMEDOUT;
		}
	}

	/* MB_CNTL clear */
	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0x00);

	mtk_demod_delay_ms(1);

	return 0;
}

static int intern_dvbt_t2_get_ts_divider(struct dvb_frontend *fe,
								u8 *divider)
{
	/* struct dtv_frontend_properties *c = &fe->dtv_property_cache; */
	u8 reg;
	int ts_data_rate = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_3, &reg);
	ts_data_rate = reg;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_2, &reg);
	ts_data_rate = (ts_data_rate<<8) | reg;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_1, &reg);
	ts_data_rate = (ts_data_rate<<8) | reg;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_0, &reg);
	ts_data_rate = (ts_data_rate<<8) | reg;

	pr_info("[dvbt/t2] TS_DATA_RATE_total = %d (0x%x)\n",
		ts_data_rate, ts_data_rate);


	/* if(c->delivery_system = DVB_T) */
	if (lock_signal == T_SIGNAL) {
		/* set TS clock source div 5 */
		reg = mtk_demod_read_byte(0x103301);
		reg = reg&(~0x07);
		mtk_demod_write_byte(0x103301, reg);

		mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DIV_172, &reg);

		*divider = reg;

		pr_info("CLK Source: 172 MHz\n");
	} else {
		/* set TS clock source div 3 */
		reg = mtk_demod_read_byte(0x103301);
		reg = (reg&(~0x07)) | 0x01;
		mtk_demod_write_byte(0x103301, reg);

		mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DIV_288, &reg);
		*divider = reg;

		pr_info("CLK Source: 288 MHz\n");

		/* 36 MHz/8 = 4.5 MHz */
		if (*divider > 0x1f)
			*divider = 0x1f;

		/* 76.8 MHz/8 = 9.6 MHz */
		if (*divider < 0x0e)
			*divider = 0x0e;
	}

	pr_info(">>>[%s] = 0x%x<<<\n",  __func__, *divider);

	return 0;
}

static int intern_dvbt_t2_adaptive_ts_config(struct dvb_frontend *fe)
{
	/* struct dtv_frontend_properties *c = &fe->dtv_property_cache; */
	u8 divider = 0;
	u8 ts_enable = 0;
	u8 reg = 0;
	u8 wait_cnt = 0;

	mtk_demod_mbx_dvb_read(DVBTM_REG_BASE + 0x40, &ts_enable);

	/* check if TS DATA RATE change */
	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_CHANGE_IND,
		&reg);

	/* Since T2 reports FEC lock before caculating TS clock */
	/* ==> Saving channel scan time */
	if ((reg == 0) && ((ts_enable & 0x04) == 0x00)) {
		do {
			mtk_demod_delay_ms(1);

			/* check if TS DATA RATE change */
			mtk_demod_mbx_dvb_read_dsp(
			E_DMD_T_T2_TS_DATA_RATE_CHANGE_IND, &reg);

			if (wait_cnt++ >= 0xFF) {
				pr_err("@@@@@ DVBT/T2 TS DATA RATE change indcator Fail!\n");
				return -ETIMEDOUT;
			}
		} while (reg == 0);
	}

	if (reg == 1)  {
		intern_dvbt_t2_get_ts_divider(fe, &divider);
		pr_info("[%s][%d] TS_DATA_RATE_CHANGE Detected: TsClkDivNum = 0x%x\n",
			__func__, __LINE__, divider);

		/* disable TS module work */
		reg = mtk_demod_read_byte(0x103302);
		mtk_demod_write_byte(0x103302, reg | 0x01);
		mtk_demod_write_byte(0x103300, divider);
		/* phase_tun setting */
		reg = mtk_demod_read_byte(0x103301);
		mtk_demod_write_byte(0x103301, reg | 0x40);
		mtk_demod_write_byte(0x10330b, (divider>>1));
		/* enable TS module work */
		reg = mtk_demod_read_byte(0x103302);
		mtk_demod_write_byte(0x103302, reg & ~0x01);
		mtk_demod_mbx_dvb_write_dsp(
			(u16)E_DMD_T_T2_TS_DATA_RATE_CHANGE_IND, 0x00);

		/* Enable TS */
		ts_enable |= 0x04;
		mtk_demod_mbx_dvb_write(DVBTM_REG_BASE + 0x40, ts_enable);
	}

	return 0;
}

static void intern_dvbt_t2_get_snr_ms_format(struct ms_float_st *msf_snr)
{
	u8 reg, reg_frz;
	u16 snr;
	u32 noise_power;

	struct ms_float_st msf_denominator;

	if (lock_signal == T_SIGNAL) {
		mtk_demod_mbx_dvb_read(FDP_REG_BASE+0xfe, &reg_frz);
		mtk_demod_mbx_dvb_write(FDP_REG_BASE+0xfe, reg_frz|0x01);

		mtk_demod_mbx_dvb_write(FDP_REG_BASE+0xff, 0x01);

		mtk_demod_mbx_dvb_read(ISDBT_FDPE_REG_BASE+0x5b, &reg);
		noise_power = reg&0x07;
		mtk_demod_mbx_dvb_read(ISDBT_FDPE_REG_BASE+0x5a, &reg);
		noise_power = (noise_power<<8)|reg;
		mtk_demod_mbx_dvb_read(ISDBT_FDPE_REG_BASE+0x59, &reg);
		noise_power = (noise_power<<8)|reg;
		mtk_demod_mbx_dvb_read(ISDBT_FDPE_REG_BASE+0x58, &reg);
		noise_power = (noise_power<<8)|reg;

		mtk_demod_mbx_dvb_write(FDP_REG_BASE+0xfe, reg_frz);

		mtk_demod_mbx_dvb_write(FDP_REG_BASE+0xff, 0x01);

		noise_power = noise_power/2;
		noise_power /= 1280;

		if (noise_power == 0)
			noise_power = 1;

		noise_power = noise_power*100;

		msf_snr->data = (s32)log10approx100((u64)noise_power);
		msf_snr->exp = 0;
		msf_denominator.data = 10;
		msf_denominator.exp = 0;
		*msf_snr = mtk_demod_float_op(*msf_snr,
			msf_denominator, DIVIDE);
	} else {
		mtk_demod_mbx_dvb_read(TOP_REG_BASE+0xef, &reg_frz);
		mtk_demod_mbx_dvb_write(TOP_REG_BASE+0xef, reg_frz|0x80);

		mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_SNR_H, &reg);
		snr = reg;
		mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_SNR_L, &reg);
		snr = (snr<<8)|reg;

		mtk_demod_mbx_dvb_write(TOP_REG_BASE+0xef, reg_frz);

		msf_snr->data = (u32)snr;
		msf_snr->exp = 0;
		msf_denominator.data = 10;
		msf_denominator.exp = 0;
		*msf_snr = mtk_demod_float_op(*msf_snr,
			msf_denominator, DIVIDE);
	}
}

static void intern_dvbt_t2_get_ber_ms_format(struct ms_float_st *msf_ber)
{
	u8 reg, reg_frz;
	u16 bit_err_period, fec_type;
	u32 bit_err, time_diff;

	struct ms_float_st msf_numerator;
	struct ms_float_st msf_denominator;

	time_diff = mtk_demod_get_time() - first_lock_time;
	if (time_diff < 300)
		mtk_demod_delay_ms(time_diff);

	if (lock_signal == T_SIGNAL) {
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

		if (bit_err > 2147483647) {
			msf_numerator.data = 1;
			msf_numerator.exp = -1;
		} else {
			msf_numerator.data = bit_err;
			msf_numerator.exp = 0;
		}

		*msf_ber = mtk_demod_float_op(msf_numerator,
			msf_denominator, DIVIDE);
	} else {
		mtk_demod_mbx_dvb_write(T2FEC_REG_BASE+0x04, 0x01);

		mtk_demod_mbx_dvb_read(T2FEC_REG_BASE+0xA9, &reg);
		bit_err_period = reg;
		mtk_demod_mbx_dvb_read(T2FEC_REG_BASE+0xA8, &reg);
		bit_err_period = (bit_err_period << 8) | reg;

		mtk_demod_mbx_dvb_read(T2FEC_REG_BASE+(0x34<<1)+3, &reg);
		bit_err = reg;
		mtk_demod_mbx_dvb_read(T2FEC_REG_BASE+(0x34<<1)+2, &reg);
		bit_err = (bit_err << 8) | reg;
		mtk_demod_mbx_dvb_read(T2FEC_REG_BASE+(0x34<<1)+1, &reg);
		bit_err = (bit_err << 8) | reg;
		mtk_demod_mbx_dvb_read(T2FEC_REG_BASE+(0x34<<1)+0, &reg);
		bit_err = (bit_err << 8) | reg;

		mtk_demod_mbx_dvb_write(T2FEC_REG_BASE+0x04, 0x00);

		mtk_demod_mbx_dvb_read(T2L1_REG_BASE+0x8f, &reg);
		fec_type = reg;
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE+0x8e, &reg);
		fec_type = (fec_type << 8) | reg;

		if (bit_err_period == 0)
			bit_err_period = 1;

		if (fec_type & 0x0180)
			msf_denominator.data = bit_err_period*64800;
		else
			msf_denominator.data = bit_err_period*16200;

		msf_denominator.exp = 0;
		msf_numerator.data = bit_err;
		msf_numerator.exp = 0;
		*msf_ber = mtk_demod_float_op(msf_numerator,
			msf_denominator, DIVIDE);
	}

	msf_denominator.data = 100000;
	msf_denominator.exp = 0;
	*msf_ber = mtk_demod_float_op(*msf_ber, msf_denominator, MULTIPLY);
	msf_denominator.data = 1000000;
	msf_denominator.exp = 0;
	*msf_ber = mtk_demod_float_op(*msf_ber, msf_denominator, MULTIPLY);
}

static void intern_dvbt_t2_ber_cali(struct ms_float_st *msf_ber)
{
	s64 s64ber;

	intern_dvbt_t2_get_ber_ms_format(msf_ber);

	/* if (c->delivery_system == SYS_DVBT) { */
	if (lock_signal == T_SIGNAL) {
		if (msf_ber->data == 0) {
			/* min ber = 4.329E-7 */
			s64ber = 43290;
		} else {
			s64ber = mtk_demod_float_to_s64(*msf_ber);
		}

		if ((s64ber > 1000) && (s64ber < 10000000000)) {
			if ((ber_filtered <= 0) ||
				((ber_filtered/s64ber > 30) ||
				(s64ber/ber_filtered > 33))) {
				ber_filtered = s64ber;
			} else {
				if ((ber_filtered/s64ber > 3) ||
					(s64ber/ber_filtered > 3)) {
					ber_filtered =
						(5*ber_filtered + 5*s64ber)/10;
				} else if ((ber_filtered/s64ber > 2) ||
					(s64ber/ber_filtered > 2)) {
					ber_filtered =
						(7*ber_filtered + 3*s64ber)/10;
				} else {
					ber_filtered =
						(9*ber_filtered + s64ber)/10;
				}
			}
		}

	} else {
		if (msf_ber->data == 0) {
			/* min ber = 3.014E-8 */
			s64ber = 3014;
		} else {
			s64ber = mtk_demod_float_to_s64(*msf_ber);
		}

		if ((ber_filtered <= 0) || ((ber_filtered/s64ber > 30) ||
				(s64ber/ber_filtered > 33))) {
			ber_filtered = s64ber;
		} else {
			ber_filtered = (9*ber_filtered + s64ber)/10;
		}
	}
}

static int intern_dvbt_t2_ger_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct ms_float_st msf_snr;
	struct ms_float_st msf_denominator;

	if (fec_lock == 0) {
		*snr = 0;
		return 0;
	}

	intern_dvbt_t2_get_snr_ms_format(&msf_snr);

	msf_denominator.data = 10;
	msf_denominator.exp = 0;

	if (lock_signal == T_SIGNAL) {
		msf_snr = mtk_demod_float_op(msf_snr,
			msf_denominator, MULTIPLY);
	}

	*snr = (u16)mtk_demod_float_to_s64(msf_snr);


	return 0;
}

static int intern_dvbt_t2_ger_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct ms_float_st msf_ber;
	struct ms_float_st msf_denominator;

	if (fec_lock == 0) {
		*ber = 1;
		return 0;
	}

	intern_dvbt_t2_get_ber_ms_format(&msf_ber);

	/* 10^9 */
	msf_denominator.data = 100;
	msf_denominator.exp = 0;
	msf_ber = mtk_demod_float_op(msf_ber,
			msf_denominator, DIVIDE);

	*ber = (u32)mtk_demod_float_to_s64(msf_ber);

	return 0;
}

static int intern_dvbt_t2_get_sqi(struct dvb_frontend *fe, u16 *sqi)
{
	u8 reg, QAM, CR;
	u16 cn_rec, ber_sqi;
	s16 cn_rel, cn_ref, s16tmp;
	s64 s64ber, s64tmp;
	u64 max_ber_th, u64tmp;
	/* u64 ber_th1[4] = {1E+7, 9E+6, 11E+6, 1E+7} */
	u64 ber_th1[4] = {10000000, 9000000, 11000000, 10000000};
	/* u64 ber_th2[4] = {3E+4, 3E+4, 27E+3, 33E+3} */
	u64 ber_th2[4] = {30000, 30000, 27000, 33000};
	static u8 SQI_State;

	struct ms_float_st msf_ber = {0, 0};
	struct ms_float_st msf_tmp = {0, 0};


	if (fec_lock == 0) {
		*sqi = 0;
		return 0;
	}

	if (lock_signal == T_SIGNAL) {
		intern_dvbt_t2_ber_cali(&msf_ber);

		if (ber_filtered <= 0) {
			intern_dvbt_t2_get_ber_ms_format(&msf_ber);

			if (msf_ber.data == 0) {
				/* min ber = 4.329-7 */
				s64ber = 43290;
			} else {
				s64ber = mtk_demod_float_to_s64(msf_ber);
			}
			ber_filtered = s64ber;
		} else {
			s64ber = ber_filtered;
		}

		mtk_demod_mbx_dvb_read(0x1f00+(0x41)*2, &reg);

		if ((reg&0x01) == 0x00)
			max_ber_th = 2000000000;
		else
			max_ber_th = 100000000;

		if (s64ber > max_ber_th) {
			ber_sqi = 0;
		} else if (s64ber > 10000) {
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
			u64tmp = u64tmp*20-2200;

			ber_sqi = (u16)u64tmp;
		} else {
			ber_sqi = 10000;
		}

		if (s64ber < 51900) /* g_min_ber = 5.19e-7 */
			ber_sqi = 10000;

		if (ber_sqi >= 10000)
			ber_sqi = 10000;
		else if (ber_sqi <= 0)
			ber_sqi = 0;

		intern_dvbt_t2_ger_snr(fe, &cn_rec);

		mtk_demod_mbx_dvb_read(FDP_REG_BASE + 0x24, &reg);
		QAM = reg&0x03;
		mtk_demod_mbx_dvb_read(FDP_REG_BASE + 0x25, &reg);
		CR = (reg>>4)&0x07;
		cn_ref = dvbt_sqi_dbm_nordigp1_x10[QAM][CR];

		cn_rel = cn_rec - cn_ref + 5;

		if (cn_rel < -70) {
			*sqi = 0;
		} else if (cn_rel < 30) {
			/* (ber_sqi*((cn_rel - 3.0)/10.0 + 1.0)) */
			u64tmp = (u64)(ber_sqi*((cn_rel-30) + 100));
			*sqi = (u16)(u64tmp/10000);
		} else {
			*sqi = (u16)(ber_sqi/100);
		}

	} else {
		/* DVBT2 Signal */
		if (ber_filtered < 0) {
			intern_dvbt_t2_get_ber_ms_format(&msf_ber);

			if (msf_ber.data == 0) {
				/* min ber = 3.014E-8 */
				s64ber = 3014;
			} else {
				s64ber = mtk_demod_float_to_s64(msf_ber);
			}
			ber_filtered = s64ber;
		} else {
			s64ber = ber_filtered;
		}

		if (s64ber > ber_th1[SQI_State]) {
			ber_sqi = 0;
			SQI_State = 1;
		} else if (s64ber >= ber_th2[SQI_State]) {
			ber_sqi = 1000/15;
			SQI_State = 2;
		} else {
			ber_sqi = 1000/6;
			SQI_State = 3;
		}

		intern_dvbt_t2_ger_snr(fe, &cn_rec);

		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x47 * 2), &reg);
		CR = reg&0x07;
		QAM = (reg>>3)&0x03;
		cn_ref = dvbt2_sqi_db_nordigp1_x10[QAM][CR];

		cn_rel = cn_rec - cn_ref + 7;

		if (cn_rel > 30) {
			*sqi = 100;
		} else if (cn_rel >= -30) {
			s16tmp = (cn_rel+30)*ber_sqi;
			if (s16tmp > 10000)
				*sqi = 100;
			else if (s16tmp < 0)
				*sqi = 0;
			else
				*sqi = (u16)(s16tmp/100);
		} else {
			*sqi = 0;
		}
	}


	return 0;
}

static int intern_dvbt_t2_get_ssi(struct dvb_frontend *fe, u16 *ssi,
			int rf_power_dbm)
{
	u8 QAM, CR, reg;
	s32 tmp_value = 0;
	s16 ch_power_db = 0;
	s16 ch_power_ref = 0;
	s16 ch_power_rel = 0;

	if (fec_lock == 0) {
		*ssi = 0;
		return 0;
	}

	if (lock_signal == T_SIGNAL) {
		mtk_demod_mbx_dvb_read(FDP_REG_BASE + 0x24, &reg);
		QAM = reg&0x03;
		mtk_demod_mbx_dvb_read(FDP_REG_BASE + 0x25, &reg);
		CR = reg&0x07;

		ch_power_ref = dvbt_ssi_dbm_nordigp1_x10[QAM][CR];
	} else {
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x47 * 2), &reg);
		CR = reg&0x07;
		QAM = (reg>>3)&0x03;

		ch_power_ref = dvbt2_ssi_dbm_nordigp1_x10[QAM][CR];
	}

	ch_power_db = (s16)rf_power_dbm * 10;
	ch_power_rel = ch_power_db - ch_power_ref;

	if (ch_power_rel < -150) {
		*ssi = 0;
	} else if (ch_power_rel < 0) {
		tmp_value = ch_power_rel;
		tmp_value = (2*(tmp_value+150));
		tmp_value = tmp_value/3;

		*ssi = tmp_value/10;
	} else if (ch_power_rel < 200) {
		tmp_value = ch_power_rel;
		tmp_value = 4*tmp_value+100;

		*ssi = tmp_value/10;
	} else if (ch_power_rel < 350) {
		tmp_value = ch_power_rel;
		tmp_value = 2*(tmp_value-200);
		tmp_value = tmp_value/3;
		tmp_value = tmp_value+900;

		*ssi = tmp_value/10;
	} else {
		*ssi = 100;
	}


	return 0;
}

static void intern_dvbt_t2_get_freq_offset(struct dvb_frontend *fe, s16 *cfo)
{
	u8 bw, reg, reg_frz, fft_reg;
	u16 icfo_reg;
	u32 cfo_reg, td_cfo_reg, fd_cfo_reg;

	struct ms_float_st msf_bw;
	struct ms_float_st msf_cfo;
	struct ms_float_st msf_tmp;
	struct ms_float_st msf_numerator;
	struct ms_float_st msf_denominator;

	struct dtv_frontend_properties *c = &fe->dtv_property_cache;


	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_BW, &bw);
	switch (bw) {
	case 0x02:
		bw = 6;
		break;
	case 0x03:
		bw = 7;
		break;
	case 0x04:
	default:
		bw = 8;
		break;
	}

	if (lock_signal == T_SIGNAL) {
		/* TD CFO */
		mtk_demod_mbx_dvb_read(ISDBT_TDP_REG_BASE + 0x04, &reg_frz);
		mtk_demod_mbx_dvb_write(ISDBT_TDP_REG_BASE + 0x04,
								reg_frz|0x01);

		mtk_demod_mbx_dvb_read(ISDBT_TDP_REG_BASE + 0x8A + 3, &reg);
		td_cfo_reg = reg;
		mtk_demod_mbx_dvb_read(ISDBT_TDP_REG_BASE + 0x8A + 2, &reg);
		td_cfo_reg = (td_cfo_reg << 8)|reg;
		mtk_demod_mbx_dvb_read(ISDBT_TDP_REG_BASE + 0x8A + 1, &reg);
		td_cfo_reg = (td_cfo_reg << 8)|reg;
		mtk_demod_mbx_dvb_read(ISDBT_TDP_REG_BASE + 0x8A + 0, &reg);
		td_cfo_reg = (td_cfo_reg << 8)|reg;

		mtk_demod_mbx_dvb_read(ISDBT_TDP_REG_BASE + 0x04, &reg_frz);
		mtk_demod_mbx_dvb_write(ISDBT_TDP_REG_BASE + 0x04,
							reg_frz&(~0x01));

		/* FD CFO */
		mtk_demod_mbx_dvb_read(ISDBT_FDP_REG_BASE + 0xfe, &reg_frz);
		mtk_demod_mbx_dvb_write(ISDBT_FDP_REG_BASE + 0xfe,
								reg_frz|0x01);
		mtk_demod_mbx_dvb_write(ISDBT_FDP_REG_BASE + 0xff, 0x01);

		mtk_demod_mbx_dvb_read(ISDBT_FDP_REG_BASE + 0x5e + 3, &reg);
		fd_cfo_reg = reg;
		mtk_demod_mbx_dvb_read(ISDBT_FDP_REG_BASE + 0x5e + 2, &reg);
		fd_cfo_reg = (fd_cfo_reg << 8)|reg;
		mtk_demod_mbx_dvb_read(ISDBT_FDP_REG_BASE + 0x5e + 1, &reg);
		fd_cfo_reg = (fd_cfo_reg << 8)|reg;
		mtk_demod_mbx_dvb_read(ISDBT_FDP_REG_BASE + 0x5e + 0, &reg);
		fd_cfo_reg = (fd_cfo_reg << 8)|reg;

		mtk_demod_mbx_dvb_read(ISDBT_FDP_REG_BASE + 0xfe, &reg_frz);
		mtk_demod_mbx_dvb_write(ISDBT_FDP_REG_BASE + 0xfe,
							reg_frz&(~0x01));
		mtk_demod_mbx_dvb_write(ISDBT_FDP_REG_BASE + 0xff, 0x01);

		/* ICFO */
		mtk_demod_mbx_dvb_read(ISDBT_FDP_REG_BASE + 0x2e*2, &reg);
		icfo_reg = reg;
		mtk_demod_mbx_dvb_read(ISDBT_FDP_REG_BASE + 0x2e*2 + 1, &reg);
		icfo_reg = (((reg << 8)|icfo_reg) >> 4);

		mtk_demod_mbx_dvb_read(FDP_REG_BASE + 0x26, &reg);
		fft_reg = (reg>>4)&0x03;


		msf_numerator.data = 8*bw;
		msf_numerator.exp = 0;
		msf_denominator.data = 7;
		msf_denominator.exp = 0;
		msf_bw = mtk_demod_float_op(msf_numerator,
					msf_denominator, DIVIDE);

		msf_cfo.data = (s32)td_cfo_reg;
		msf_cfo.exp = 0;
		if (td_cfo_reg & 0x10000000) {
			msf_denominator.data = 0x20000000;
			msf_denominator.exp = 0;
			msf_cfo = mtk_demod_float_op(msf_cfo,
						msf_denominator, MINUS);
		}

		msf_cfo = mtk_demod_float_op(msf_cfo, msf_bw, MULTIPLY);

		msf_numerator.data = 367;
		msf_numerator.exp = 0;
		msf_denominator.data = 100000000;
		msf_denominator.exp = 0;
		msf_numerator = mtk_demod_float_op(msf_numerator,
						msf_denominator, DIVIDE);
		msf_cfo = mtk_demod_float_op(msf_cfo, msf_numerator, MULTIPLY);


		msf_tmp.data = (s32)fd_cfo_reg;
		msf_tmp.exp = 0;
		if (fd_cfo_reg & 0x1000000) {
			msf_denominator.data = (s32)0x2000000;
			msf_denominator.exp = 0;
			msf_tmp = mtk_demod_float_op(msf_tmp,
					msf_denominator, MINUS);
		}

		msf_tmp = mtk_demod_float_op(msf_tmp,
							msf_bw, MULTIPLY);

		msf_numerator.data = 75;
		msf_numerator.exp = 0;
		msf_denominator.data = 10000;
		msf_denominator.exp = 0;
		msf_numerator = mtk_demod_float_op(msf_numerator,
						msf_denominator, DIVIDE);
		msf_tmp = mtk_demod_float_op(msf_tmp,
						msf_numerator, MULTIPLY);
		msf_cfo = mtk_demod_float_op(msf_cfo, msf_tmp, ADD);


		msf_tmp.data = (s32)icfo_reg;
		msf_tmp.exp = 0;
		if (icfo_reg & 0x400) {
			msf_denominator.data = 0x800;
			msf_denominator.exp = 0;
			msf_tmp = mtk_demod_float_op(msf_tmp,
							msf_denominator, MINUS);
		}

		msf_tmp = mtk_demod_float_op(msf_tmp,
							msf_bw, MULTIPLY);

		msf_numerator.data = 1000;
		msf_numerator.exp = 0;
		msf_tmp = mtk_demod_float_op(msf_tmp,
						msf_numerator, MULTIPLY);

		switch (fft_reg) {
		case 0x00:
			msf_denominator.data = 2048;
			msf_denominator.exp = 0;
			break;
		case 0x2:
			msf_denominator.data = 4096;
			msf_denominator.exp = 0;
			break;
		case 0x1:
		default:
			msf_denominator.data = 8192;
			msf_denominator.exp = 0;
			break;
		}
		msf_tmp = mtk_demod_float_op(msf_tmp,
						msf_denominator, DIVIDE);

		msf_denominator.data = 1000;
		msf_denominator.exp = 0;
		msf_cfo = mtk_demod_float_op(msf_cfo, msf_denominator, DIVIDE);
		msf_cfo = mtk_demod_float_op(msf_cfo, msf_tmp, ADD);

		if (msf_cfo.exp < -32) {
			msf_cfo.data = 0;
			msf_cfo.exp = 0;
		}
	} else {
		mtk_demod_mbx_dvb_read(TDF_REG_BASE + (0x2f)*2, &reg);
		mtk_demod_mbx_dvb_write(TDF_REG_BASE + (0x2f)*2, reg|0x10);

		mtk_demod_mbx_dvb_write(TDF_REG_BASE + (0x02)*2 + 1, 0x80);

		mtk_demod_mbx_dvb_read(TDF_REG_BASE + (0x2c)*2, &reg);
		cfo_reg = (u32)reg;
		mtk_demod_mbx_dvb_read(TDF_REG_BASE + (0x2c)*2 + 1, &reg);
		cfo_reg |= (((u32)reg)<<8);
		mtk_demod_mbx_dvb_read(TDF_REG_BASE + (0x2d)*2, &reg);
		cfo_reg |= (((u32)reg)<<16);
		mtk_demod_mbx_dvb_read(TDF_REG_BASE + (0x2d)*2 + 1, &reg);
		cfo_reg |= (((u32)reg)<<24);

		mtk_demod_mbx_dvb_write(TDF_REG_BASE + (0x02)*2 + 1, 0x00);

		if (cfo_reg >= 0x4000000)
			msf_cfo.data = (s32)cfo_reg - 0x8000000;
		else
			msf_cfo.data = (s32)cfo_reg;
		msf_cfo.exp = 0;

		msf_denominator.data = 134217728;
		msf_denominator.exp = 0;
		msf_cfo = mtk_demod_float_op(msf_cfo, msf_denominator, DIVIDE);

		msf_bw.data = bw;
		msf_bw.exp = 0;
		msf_numerator.data = 8000;
		msf_numerator.exp = 0;
		msf_denominator.data = 7;
		msf_denominator.exp = 0;

		msf_cfo = mtk_demod_float_op(msf_cfo, msf_bw, MULTIPLY);
		msf_cfo = mtk_demod_float_op(msf_cfo, msf_numerator, MULTIPLY);
		msf_cfo = mtk_demod_float_op(msf_cfo, msf_denominator, DIVIDE);

		/* avoid underflow */
		if (msf_cfo.exp < -32) {
			msf_cfo.data = 0;
			msf_cfo.exp = 0;
		}
	}

	*cfo = (s16)mtk_demod_float_to_s64(msf_cfo);
}

static void intern_dvbt_t2_get_pkt_err(struct dvb_frontend *fe, u16 *err)
{
	u8 reg, reg_frz;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	if (lock_signal == T_SIGNAL) {
		mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x03, &reg_frz);
		mtk_demod_mbx_dvb_write(BACKEND_REG_BASE+0x03, reg_frz|0x03);

		mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x67, &reg);
		*err = reg;
		mtk_demod_mbx_dvb_read(BACKEND_REG_BASE+0x66, &reg);
		*err = (*err << 8) | reg;

		reg_frz = reg_frz&(~0x03);
		mtk_demod_mbx_dvb_write(BACKEND_REG_BASE+0x03, reg_frz);
	} else {
		mtk_demod_mbx_dvb_write(T2FEC_REG_BASE+0x04, 0x01);

		mtk_demod_mbx_dvb_read(T2FEC_REG_BASE+0x5B, &reg);
		*err = reg;
		mtk_demod_mbx_dvb_read(T2FEC_REG_BASE+0x5A, &reg);
		*err = (*err << 8) | reg;

		mtk_demod_mbx_dvb_write(T2FEC_REG_BASE+0x04, 0x00);
	}
}

static void intern_dvbt_t2_get_plp_array(u8 *plp_array)
{
	u8 reg, index = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_L1_FLAG, &reg);
	if (reg != L1_READY) {
		pr_err("[%s] L1 is not ready\n", __func__);
		return;
	}

	while (index < NUM_PLP_ARRAY) {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_PLP_ID_ARR + index, &reg);
		plp_array[index] = reg;
		//pr_info("[0x%x]", p->plp_array[index]);
		index++;
	}
}

static int intern_dvbt_t2_reset(void)
{
	int ret = 0;

	ret = intern_dvbt_t2_soft_stop();

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

void m7332_dmd_hal_dvbt_t2_init_clk(struct dvb_frontend *fe)
{
	u8 reg;
	u16 u16tmp;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

	dvbt_t2_tdi_addr = demod_prvi->dram_base_addr;
	pr_notice("@@@@@@@@@@@ dvbt_t2_tdi_addr address = 0x%x\n",
		dvbt_t2_tdi_addr);
	dvbt_t2_fw_addr = dvbt_t2_tdi_addr + 0x320000;
	pr_notice("@@@@@@@@@@@ dvbt_t2_fw_addr address = 0x%x\n",
		dvbt_t2_fw_addr);
	dvbt_t2_eq_addr = dvbt_t2_fw_addr + 0x20000;
	pr_notice("@@@@@@@@@@@ dvbt_t2_eq_addr address = 0x%x\n",
		dvbt_t2_eq_addr);
	dvbt_t2_djb_addr = dvbt_t2_eq_addr + 0x96000;
	pr_notice("@@@@@@@@@@@ dvbt_t2_djb_addr address = 0x%x\n",
		dvbt_t2_djb_addr);


	mtk_demod_write_byte(0x101e39, 0x00);

	mtk_demod_write_byte(0x101e68,
		(mtk_demod_read_byte(0x101e68)&(~0x03)));

	mtk_demod_write_byte(0x10331f, 0x00);
	mtk_demod_write_byte(0x10331e, 0x10);

	mtk_demod_write_byte(0x103301, 0x11);
	mtk_demod_write_byte(0x103300, 0x17);

	mtk_demod_write_byte(0x103309, 0x00);
	mtk_demod_write_byte(0x103308, 0x00);

	mtk_demod_write_byte(0x103314, 0x04);

	mtk_demod_write_byte(0x103321, 0x00);
	mtk_demod_write_byte(0x103320, 0x00);

	mtk_demod_write_byte(0x103302, 0x01);
	mtk_demod_write_byte(0x103302, 0x00);

	mtk_demod_write_byte(0x111f01, 0x00);
	mtk_demod_write_byte(0x111f00, 0x0a);

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

	u16tmp = (u16)(dvbt_t2_fw_addr>>16);
	mtk_demod_write_byte(0x153d1b, (u8)(u16tmp>>8));
	mtk_demod_write_byte(0x153d1a, (u8)u16tmp);

	mtk_demod_write_byte(0x153d09, 0x00);
	mtk_demod_write_byte(0x153D08, 0x00);

	mtk_demod_write_byte(0x153d0d, 0x00);
	mtk_demod_write_byte(0x153d0c, 0x00);

	mtk_demod_write_byte(0x153d0b, 0x00);
	mtk_demod_write_byte(0x153d0a, 0x01);

	mtk_demod_write_byte(0x153d0f, 0xff);
	mtk_demod_write_byte(0x153d0e, 0xff);

	mtk_demod_write_byte(0x153d18, 0x04);

	reg = mtk_demod_read_byte(0x112001);
	reg &= (~0x10);
	mtk_demod_write_byte(0x112001, reg);

	mtk_demod_write_byte(0x153d1c, 0x01);

/*
 *	if(u32DMD_DVBT_FW_START_ADDR&MIU_INTERVAL)
 *	{
 *		mtk_demod_write_byte(0x1127ae,0x01);
 *		pr_info("[DVBT/T2][MIU 1] Select ADDR for miu 1 !!\n");
 *	}
 *	else
 *	{
 *		mtk_demod_write_byte(0x1127ae,0x00);
 *		pr_info("[DVBT/T2][MIU 0] Select ADDR for miu 0 !!\n");
 *	}
 */

	mtk_demod_write_byte(0x101e39, 0x03);
	mtk_demod_write_byte(0x103c0e, 0x01);
}

int m7332_dmd_hal_dvbt_t2_load_code(void)
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

	dram_code_virt_addr = ioremap_wc(dvbt_t2_fw_addr+0x20000000,
								0x20000);
	/* load code to SDRAM (common) */
	memcpy(dram_code_virt_addr, &dvbt_t2_com_table[0],
			sizeof(dvbt_t2_com_table));
	/* load code to SDRAM (bk0) */
	memcpy(dram_code_virt_addr+0x3000, &dvbt_t2_bk0_table[0],
			sizeof(dvbt_t2_bk0_table));
	/* load code to SDRAM (common) */
	memcpy(dram_code_virt_addr+0x10000, &dvbt_t2_com_table[0],
			sizeof(dvbt_t2_com_table));
	/* load code to SDRAM (bk1) */
	memcpy(dram_code_virt_addr+0x13000, &dvbt_t2_bk1_table[0],
			sizeof(dvbt_t2_bk1_table));

	memcpy(dram_code_virt_addr+0x30, &dvbt_t2_eq_addr, 4);
	memcpy(dram_code_virt_addr+0x34, &dvbt_t2_tdi_addr, 4);
	memcpy(dram_code_virt_addr+0x38, &dvbt_t2_djb_addr, 4);
	memcpy(dram_code_virt_addr+0x3c, &dvbt_t2_fw_addr, 4);

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

int m7332_dmd_hal_dvbt_t2_config(struct dvb_frontend *fe, u32 if_freq)
{
	u8 bw;
	u32 ret = 0;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	pr_info("[%s] is called\n", __func__);

	/* Bandwidth */
	switch (c->bandwidth_hz) {
	case 1700000:
		 bw = 0;
		break;
	case 5000000:
		bw = 1;
		break;
	case 6000000:
		bw = 2;
		break;
	case 7000000:
		bw = 3;
		break;
	case 8000000:
		bw = 4;
		break;
	case 10000000:
		bw = 5;
		break;
	default:
		bw = 4;
		break;
	}
	mtk_demod_mbx_dvb_write_dsp(E_DMD_T_T2_BW, bw);

	/* IF */
	mtk_demod_mbx_dvb_write_dsp(E_DMD_T_T2_FC_L, (if_freq)&0xff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_T_T2_FC_H, (if_freq>>8)&0xff);

	/* AGC ref */
	mtk_demod_mbx_dvb_write_dsp(E_DMD_T_T2_AGC_REF, 0x40);

	/* Channel Switch Indicator */
	mtk_demod_mbx_dvb_write_dsp(E_DMD_T_T2_CHANNEL_SWITCH, 0xF0);

	/* Hierarchy Mode */
	if (c->stream_id)
		mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBT_N_CFG_LP_SEL, 0x00);
	else
		mtk_demod_mbx_dvb_write_dsp(E_DMD_DVBT_N_CFG_LP_SEL, 0x01);

	/* PLP_ID */
	mtk_demod_mbx_dvb_write_dsp(E_DMD_T_T2_PLP_ID, c->stream_id);

	/* Reset Demod */
	ret = intern_dvbt_t2_reset();

	fec_lock = 0;
	p1_tps_lock = 0;
	lock_signal = UNKNOWN_SIGNAL;

	/* Active state machine */
	mtk_demod_write_byte(MB_REG_BASE + (0x0e)*2, 0x01);

	if (c->delivery_system == SYS_DVBT)
		ber_filtered = -100000000000;

	scan_start_time = mtk_demod_get_time();

	return ret;
}

int m7332_dmd_hal_dvbt_t2_exit(void)
{
	int ret = 0;

	ret = intern_dvbt_t2_soft_stop();

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

int m7332_dmd_hal_dvbt_t2_get_frontend(struct dvb_frontend *fe,
				struct dtv_frontend_properties *p)
{
	u8  reg, tmp;
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
	intern_dvbt_t2_ger_snr(fe, &snr);
	stats = &p->cnr;
	stats->len = 1;
	stats->stat[0].svalue = snr;
	stats->stat[0].scale = FE_SCALE_DECIBEL;

	/* Packet Error */
	intern_dvbt_t2_get_pkt_err(fe, &err);
	stats = &p->block_error;
	stats->len = 1;
	stats->stat[0].uvalue = err;
	stats->stat[0].scale = FE_SCALE_COUNTER;

	/* cfo */
	intern_dvbt_t2_get_freq_offset(fe, &cfo);
	p->frequency_offset = cfo*1000;  /* KHz to Hz */

	/* SQI */
	intern_dvbt_t2_get_sqi(fe, &sqi);
	stats = &p->quality;
	stats->len = 1;
	stats->stat[0].uvalue = sqi;
	stats->stat[0].scale = FE_SCALE_RELATIVE;


	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_BW, &reg);
	/* Bandwidth */
	switch (reg) {
	case 0:
		p->bandwidth_hz = 1700000;
		break;
	case 1:
		p->bandwidth_hz = 5000000;
		break;
	case 2:
		p->bandwidth_hz = 6000000;
		break;
	case 3:
		p->bandwidth_hz = 7000000;
		break;
	case 4:
		p->bandwidth_hz = 8000000;
		break;
	case 5:
		p->bandwidth_hz = 10000000;
		break;
	}

	/* if(fe->dtv_property_cache->delivery_system == SYS_DVBT) */
	if (lock_signal == T_SIGNAL) {
		mtk_demod_mbx_dvb_read(FDP_REG_BASE + 0x24, &reg);
		/* Modulation */
		switch (reg&0x03) {
		case 0:
			p->modulation = QPSK;
			break;
		case 1:
			p->modulation = QAM_16;
			break;
		case 2:
			p->modulation = QAM_64;
			break;
		}
		/* Hierarchy */
		switch ((reg>>4)&0x07) {
		case 0:
			p->hierarchy = HIERARCHY_NON_NATIVE;
			break;
		case 1:
			p->hierarchy = HIERARCHY_1_NATIVE;
			break;
		case 2:
			p->hierarchy = HIERARCHY_2_NATIVE;
			break;
		case 3:
			p->hierarchy = HIERARCHY_4_NATIVE;
			break;
		case 4:
			p->hierarchy = HIERARCHY_NON_INDEPTH;
			break;
		case 5:
			p->hierarchy = HIERARCHY_1_INDEPTH;
			break;
		case 6:
			p->hierarchy = HIERARCHY_2_INDEPTH;
			break;
		case 7:
			p->hierarchy = HIERARCHY_4_INDEPTH;
			break;
		}

		mtk_demod_mbx_dvb_read(FDP_REG_BASE + 0x25, &reg);
		/* LP code rate */
		switch (reg&0x07) {
		case 0:
			p->code_rate_LP = FEC_1_2;
			break;
		case 1:
			p->code_rate_LP = FEC_2_3;
			break;
		case 2:
			p->code_rate_LP = FEC_3_4;
			break;
		case 3:
			p->code_rate_LP = FEC_5_6;
			break;
		case 4:
			p->code_rate_LP = FEC_7_8;
			break;
		}
		/* HP code rate */
		switch ((reg>>4)&0x07) {
		case 0:
			p->code_rate_HP = FEC_1_2;
			break;
		case 1:
			p->code_rate_HP = FEC_2_3;
			break;
		case 2:
			p->code_rate_HP = FEC_3_4;
			break;
		case 3:
			p->code_rate_HP = FEC_5_6;
			break;
		case 4:
			p->code_rate_HP = FEC_7_8;
			break;
		}

		mtk_demod_mbx_dvb_read(FDP_REG_BASE + 0x26, &reg);
		/* Guard Interval */
		switch (reg&0x03) {
		case 0:
			p->guard_interval = GUARD_INTERVAL_1_32;
			break;
		case 1:
			p->guard_interval = GUARD_INTERVAL_1_16;
			break;
		case 2:
			p->guard_interval = GUARD_INTERVAL_1_8;
			break;
		case 3:
			p->guard_interval = GUARD_INTERVAL_1_4;
			break;
		}
		/* Transmission mode */
		switch ((reg>>4)&0x03) {
		case 0:
			p->transmission_mode = TRANSMISSION_MODE_2K;
			break;
		case 1:
			p->transmission_mode = TRANSMISSION_MODE_8K;
			break;
		}

		mtk_demod_mbx_dvb_read(FEC_REG_BASE + 0x0C, &reg);
		/* LP/HP */
		switch ((reg>>3)&0x01) {
		case 0:
			/* PRIORITY_LOW */
			p->stream_id = 0;
			break;
		case 1:
			/* PRIORITY_HIGH */
			p->stream_id = 1;
			break;
		}

		/* SSI  SNR  BER  UEC */
	} else {
		/* DVBT2 signal */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x47 * 2), &reg);
		/* Code rate */
		switch (reg&0x07) {
		case 0:
			p->fec_inner = FEC_1_2;
			break;
		case 1:
			p->fec_inner = FEC_3_5;
			break;
		case 2:
			p->fec_inner = FEC_2_3;
			break;
		case 3:
			p->fec_inner = FEC_3_4;
			break;
		case 4:
			p->fec_inner = FEC_4_5;
			break;
		case 5:
			p->fec_inner = FEC_5_6;
			break;
		}
	    /* Modulation */
/**
 *		if ((reg>>6)&0x01) {
 *			switch ((reg>>3)&0x03) {
 *			case 0:
 *				p->modulation = QPSK_R;
 *				break;
 *			case 1:
 *				p->modulation = QAM_16_R;
 *				break;
 *			case 2:
 *				p->modulation = QAM_64_R;
 *				break;
 *			case 3:
 *				p->modulation = QAM_256_R;
 *				break;
 *			}
 *		} else
 */
		{
			switch ((reg>>3)&0x03) {
			case 0:
				p->modulation = QPSK;
				break;
			case 1:
				p->modulation = QAM_16;
				break;
			case 2:
				p->modulation = QAM_64;
				break;
			case 3:
				p->modulation = QAM_256;
				break;
			}
		}
/*
 *		tmp = (reg>>7)&0x01;
 *		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x47*2)+1, &reg);
 *		tmp |= ((reg&0x01)<<1);
 *		// FEC
 *		switch (tmp) {
 *		case 0:
 *			p->fec_type = SHORT;
 *			break;
 *		case 1:
 *			p->fec_type = NORMAL;
 *			break;
 *		}
 */

		mtk_demod_mbx_dvb_read(DVBTM_REG_BASE + (0x0b * 2), &reg);
		/* Guard Interval */
		switch ((reg>>4)&0x07) {
		case 0:
			p->guard_interval = GUARD_INTERVAL_1_32;
			break;
		case 1:
			p->guard_interval = GUARD_INTERVAL_1_16;
			break;
		case 2:
			p->guard_interval = GUARD_INTERVAL_1_8;
			break;
		case 3:
			p->guard_interval = GUARD_INTERVAL_1_4;
			break;
		case 4:
			p->guard_interval = GUARD_INTERVAL_1_128;
			break;
		case 5:
			p->guard_interval = GUARD_INTERVAL_19_128;
			break;
		case 6:
			p->guard_interval = GUARD_INTERVAL_19_256;
			break;
		}

		mtk_demod_mbx_dvb_read(T2TDP_REG_BASE + (0x40*2)+1, &reg);
		/* Transmission mode */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x30*2)+1, &reg);
		mtk_demod_mbx_dvb_read(T2TDP_REG_BASE + (0x40*2)+1, &tmp);
/*
 *		if (reg&0x01) {
 *			switch (tmp&0x07) {
 *			case 0:
 *				p->transmission_mode = TRANSMISSION_MODE_2K_E;
 *				break;
 *			case 1:
 *				p->transmission_mode = TRANSMISSION_MODE_8K_E;
 *				break;
 *			case 2:
 *				p->transmission_mode = TRANSMISSION_MODE_4K_E;
 *				break;
 *			case 3:
 *				p->transmission_mode = TRANSMISSION_MODE_1K_E;
 *				break;
 *			case 4:
 *				p->transmission_mode = TRANSMISSION_MODE_16K_E;
 *				break;
 *			case 5:
 *				p->transmission_mode = TRANSMISSION_MODE_32K_E;
 *				break;
 *			case 6:
 *				p->transmission_mode = TRANSMISSION_MODE_8K_E;
 *				break;
 *			case 7:
 *				p->transmission_mode = TRANSMISSION_MODE_32K_E;
 *				break;
 *			}
 *		} else
 */
		{
			switch (tmp&0x07) {
			case 0:
				p->transmission_mode = TRANSMISSION_MODE_2K;
				break;
			case 1:
				p->transmission_mode = TRANSMISSION_MODE_8K;
				break;
			case 2:
				p->transmission_mode = TRANSMISSION_MODE_4K;
				break;
			case 3:
				p->transmission_mode = TRANSMISSION_MODE_1K;
				break;
			case 4:
				p->transmission_mode = TRANSMISSION_MODE_16K;
				break;
			case 5:
				p->transmission_mode = TRANSMISSION_MODE_32K;
				break;
			case 6:
				p->transmission_mode = TRANSMISSION_MODE_8K;
				break;
			case 7:
				p->transmission_mode = TRANSMISSION_MODE_32K;
				break;
			}
		}
/*
 *		if (mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x36 * 2), &reg))
 *			return -ETIMEDOUT;
 *		// Pilot pattern
 *		switch (reg&0x0f) {
 *		case 0:
 *			p->pilot = GUARD_INTERVAL_1_32;
 *			break;
 *		case 1:
 *			p->pilot = GUARD_INTERVAL_1_16;
 *			break;
 *		case 2:
 *			p->pilot = GUARD_INTERVAL_1_8;
 *			break;
 *		case 3:
 *			p->pilot = GUARD_INTERVAL_1_4;
 *			break;
 *		case 4:
 *			p->pilot = GUARD_INTERVAL_1_128;
 *			break;
 *		case 5:
 *			p->pilot = GUARD_INTERVAL_19_128;
 *			break;
 *		case 6:
 *			p->pilot = GUARD_INTERVAL_19_256;
 *			break;
 *		}
 */

		mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_PLP_ID, &reg);
		/* DVBT2 PLP ID */
		p->stream_id = reg;

		/* DVBT2 PLP array */
		intern_dvbt_t2_get_plp_array(&(p->plp_array[0]));
	}

	return ret;
}

int m7332_dmd_hal_dvbt_t2_read_status(struct dvb_frontend *fe,
				enum fe_status *status)
{
	u8  reg;
	struct ms_float_st msf_ber = {0, 0};
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	if (mtk_demod_read_byte(MB_REG_BASE + (0x0e)*2) != 0x01) {
		pr_info("[%s][%d] Demod is inavtive !\n", __func__, __LINE__);
		return 0;
	}

	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_JOINT_DETECTION_FLAG, &reg);

	if (reg & 0x40) {
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
		if (p1_tps_lock == 0) {
			pr_notice("\n\n>>>>>>>> TPS lock !!!\n\n");
			p1_tps_lock = 1;
		}
		if (mtk_demod_mbx_dvb_read(0x20C4, &reg))
			return -ETIMEDOUT;
		if (reg == 0x0B) {
			*status |= FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
			lock_signal = T_SIGNAL;
			intern_dvbt_t2_adaptive_ts_config(fe);
			if (fec_lock == 0) {
				c->delivery_system = SYS_DVBT;
				first_lock_time = mtk_demod_get_time();
				pr_notice("\n\n>>>>>>>> DVBT lock !!!\n\n");
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
	} else if (reg & 0x08) {
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
		if (p1_tps_lock == 0) {
			pr_notice("\n\n>>>>>>>> P1 lock !!!\n\n");
			p1_tps_lock = 1;
		}
		if (mtk_demod_mbx_dvb_read_dsp(0xF0, &reg))
			return -ETIMEDOUT;
		if (reg & 0x80) {
			*status |= FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
			lock_signal = T2_SIGNAL;
			intern_dvbt_t2_adaptive_ts_config(fe);
			if (fec_lock == 0) {
				c->delivery_system = SYS_DVBT2;
				first_lock_time = mtk_demod_get_time();
				pr_notice("\n\n>>>>>>>> DVBT2 lock !!!\n\n");
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

		if (fec_lock == 1)
			intern_dvbt_t2_ber_cali(&msf_ber);
	} else if (reg & 0x80) {
		*status = FE_TIMEDOUT;
		pr_notice("\n\n>>>>>>>> DVBT/T2 no channel !!!\n\n");
	} else {
		*status = 0;
		p1_tps_lock = 0;
		fec_lock = 0;
	}

	if ((p1_tps_lock == 0) || (fec_lock == 0)) {
		if ((mtk_demod_get_time() - scan_start_time)
			> DMD_DVBT_T2_LOCK_TIMEOUT) {
			*status = FE_TIMEDOUT;
			pr_notice("\n\n>>>>>>>> DVBT/T2 lock timeout !!!\n\n");
		}
	}


	return 0;
}

ssize_t m7332_demod_hal_dvbt_t2_get_information(struct device_driver *driver,
			char *buf)
{
	u8 reg, xixo, num_frame, papr, fef, num_plp, l1_mode, pp, fec_type;
	u16 u16tmp, demod_rev, data_symbol;
	u32 ts_rate;

	pr_info("[%s] is called\n", __func__);

	/* Demod Rev. */
	mtk_demod_mbx_dvb_read(0x20c1, &reg);
	demod_rev = reg;
	mtk_demod_mbx_dvb_read(0x20c2, &reg);
	demod_rev = (demod_rev<<8) | reg;
	/* TS Rate */
	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_3, &reg);
	ts_rate = reg;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_2, &reg);
	ts_rate = (ts_rate<<8) | reg;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_1, &reg);
	ts_rate = (ts_rate<<8) | reg;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_T_T2_TS_DATA_RATE_0, &reg);
	ts_rate = (ts_rate<<8) | reg;
	ts_rate = ts_rate/1010;


	if (lock_signal == T2_SIGNAL) {
		/* XIXO */
		mtk_demod_mbx_dvb_read(T2TDP_REG_BASE + (0x40*2), &reg);
		if (((reg&0x07) == 0x00) || ((reg&0x03) == 0x00))
			xixo = 0;
		else
			xixo = 1;
		/* Number of T2 frame */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x3b*2), &num_frame);
		num_frame = reg;
		/* PAPR */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x31*2), &reg);
		papr = reg&0x0f;
		/* FEF */
		mtk_demod_mbx_dvb_read_dsp(0xf1, &reg);
		fef = reg&0x01;
		/* Number of PLPs */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x31*2), &reg);
		num_plp = reg;
		/* L1 modulation */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x31*2)+1, &reg);
		l1_mode = reg&0x0f;
		/* Pilot Pattern */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x36*2), &reg);
		pp = reg&0x0f;
		/* Data Symbols */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x3c*2), &reg);
		data_symbol = (u16)reg;
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + (0x3c*2)+1, &reg);
		data_symbol |= (((u16)reg)&0x0f)<<8;
		/* FEC type */
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + 0x8e, &reg);
		u16tmp = (u16)reg;
		mtk_demod_mbx_dvb_read(T2L1_REG_BASE + 0x8f, &reg);
		u16tmp |= ((u16)reg)<<8;
		fec_type = (u8)((u16tmp & 0x0180)>>7);
	}

	pr_info("Demod Rev. = 0x%x\n", demod_rev);
	pr_info("XIXO = %d\n", ts_rate);
	pr_info("Number of T2 frame = %d\n", xixo);
	pr_info("PAPR = %d\n", num_frame);
	pr_info("FEF = %d\n", papr);
	pr_info("Number of PLPs = %d\n", fef);
	pr_info("L1 modulation = %d\n", num_plp);
	pr_info("Pilot Pattern = %d\n", l1_mode);
	pr_info("Data Symbols = %d\n", pp);
	pr_info("FEC Type = %d\n", data_symbol);
	pr_info("TS Rate = %d\n", fec_type);


	return scnprintf(buf, PAGE_SIZE, "%x %d %d %d %d %d %d %d %d %d %d\n",
		demod_rev, ts_rate, xixo, num_frame, papr, fef, num_plp,
		l1_mode, pp, data_symbol, fec_type);
}
