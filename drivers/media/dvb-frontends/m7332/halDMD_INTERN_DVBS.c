// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <asm/io.h>
#include <linux/string.h>
#include <linux/mm_types.h>
#include <linux/timekeeping.h>
#include "drvDMD_DVBS.h"
#include "halDMD_INTERN_DVBS.h"
#include "m7332_demod_common.h"

#define HAL_DRIVER_VERSION_KEY 0xAA
#define HAL_DRIVER_VERSION  0x01
#define HAL_FW_VERSION  0x0A
#define INTERN_DVBS_DEMOD_WAIT_TIMEOUT	  6000

static u8 *DramCodeVAddr;

u8 u8DemodLockFlag;
u8	   modulation_order;
static  u8 _bDemodType;
static u32	   u32ChkScanTimeStartDVBS;
static  u8		_bSerialTS;
static u8			 pre_TS_CLK_DIV;
static u8			 cur_TS_CLK_DIV;
u8 INTERN_DVBS_table[] = {
#include "fwDMD_INTERN_DVBS.dat"
};

static u32  u32DMD_DVBS_FW_START_ADDR;
static u32 u32DVBS2_DJB_START_ADDR;
const u8 modulation_order_array[12] = {2, 3, 4, 5, 3, 4, 5, 6, 6, 6, 7, 8};
static u64  TS_Rate;
static enum DMD_DVBS_VCM_OPT u8VCM_Enabled_Opt = VCM_Disabled;
static const u16 _u16SignalLevel[47][2] = {
	{255, 20},
	{255, 900},
	{255, 880},
	{255, 860},
	{255, 840},
	{6076, 820},
	{29526, 800},
	{30032, 780},
	{30868, 760},
	{31418, 740},
	{32021, 720},
	{32413, 700},
	{32617, 680},
	{32733, 660},
	{33057, 640},
	{33431, 620},
	{34080, 600},
	{34418, 580},
	{34350, 560},
	{34757, 540},
	{34919, 520},
	{35389, 500},
	{36051, 480},
	{36575, 460},
	{36959, 440},
	{37396, 420},
	{37578, 400},
	{37850, 380},
	{38084, 360},
	{38317, 340},
	{38585, 320},
	{38881, 300},
	{39149, 280},
	{39433, 260},
	{39678, 240},
	{39927, 220},
	{40166, 200},
	{40426, 180},
	{40670, 160},
	{40911, 140},
	{41189, 120},
	{41465, 100},
	{41710, 80},
	{41982, 60},
	{42302, 40},
	{42504, 20},
	{42619, 0},

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

int intern_dvbs_soft_stop(void)
{
	u16	 u16WaitCnt = 0;

	pr_info("[%s] is called\n", __func__);

	if (mtk_demod_read_byte(MBRegBase + 0x00)) {
		pr_err(">> MB Busy!\n");
		return -ETIMEDOUT;
	}
	mtk_demod_write_byte(MBRegBase + 0x00, 0xA5);
	mtk_demod_write_byte(DMDMcuBase + 0x03,  0x02);
	mtk_demod_write_byte(DMDMcuBase + 0x03,  0x00);

	while (mtk_demod_read_byte(MBRegBase + 0x00) != 0x5A) {
		if (u16WaitCnt++ >= 0xFFF) {
			pr_err("DVBS SoftStop Fail!\n");
			return 0;
		}
	}
	mtk_demod_write_byte(MBRegBase + 0x00, 0x00);
	mtk_demod_delay_ms(1);

	return 0;
}

int intern_dvbs_reset(void)
{
	pr_info("[%s] is called\n", __func__);

	intern_dvbs_soft_stop();

	mtk_demod_write_byte(DMDMcuBase + 0x00, 0x01);
	mtk_demod_delay_ms(1);
	mtk_demod_write_byte(MBRegBase + 0x00, 0x00);
	mtk_demod_write_byte(DMDMcuBase + 0x00, 0x00);
	mtk_demod_delay_ms(5);
	mtk_demod_mbx_dvb_wait_handshake();
	mtk_demod_write_byte(MBRegBase + 0x00, 0x00);
	pr_info("[%s] is done\n", __func__);
	u32ChkScanTimeStartDVBS = mtk_demod_get_time();
	u8DemodLockFlag = 0;
	return 0;
}

int intern_dvbs_power_saving(void)
{
	mtk_demod_write_byte(0x101E83, 0x00);
	mtk_demod_write_byte(0x101E82, 0x00);
	mtk_demod_delay_ms(1);
	mtk_demod_write_byte(0x101E83, 0x00);
	mtk_demod_write_byte(0x101E82, 0x11);
	return 0;
}

int intern_dvbs_exit(void)
{
	u8 u8Data = 0;
	u8 u8Data_temp = 0;

	u8Data_temp = mtk_demod_read_byte(CHIPTOP_DMDTOP_DMD_SEL);
	mtk_demod_write_byte(CHIPTOP_DMDTOP_DMD_SEL, 0);

	u8Data = mtk_demod_read_byte(0x1128C0);
	u8Data &= ~(0x02);
	mtk_demod_write_byte(0x1128C0, u8Data);//revert IQ Swap status

	mtk_demod_write_byte(CHIPTOP_DMDTOP_DMD_SEL, u8Data_temp);

	intern_dvbs_soft_stop();

	intern_dvbs_power_saving();

	mtk_demod_write_byte(0x153D07, 0xff);
	mtk_demod_write_byte(0x153D06, 0xff);
	mtk_demod_write_byte(0x153D18,
		mtk_demod_read_byte(0x153D18) & (~0x04));
	return 0;
}

int intern_dvbs_load_code(void)
{
	int ret = 0;

	mtk_demod_write_byte(DMDMcuBase + 0x00,  0x01);
	mtk_demod_write_byte(DMDMcuBase + 0x01,  0x00);
	mtk_demod_write_byte(DMDMcuBase + 0x03,  0x50);
	mtk_demod_write_byte(DMDMcuBase + 0x03,  0x51);
	mtk_demod_write_byte(DMDMcuBase + 0x04,  0x00);
	mtk_demod_write_byte(DMDMcuBase + 0x05,  0x00);

	pr_info("[%s] is called\n", __func__);

	DramCodeVAddr = ioremap_wc(u32DMD_DVBS_FW_START_ADDR+0x20000000,
		0x10000);

	memcpy(DramCodeVAddr, &INTERN_DVBS_table[0], sizeof(INTERN_DVBS_table));
	memcpy(DramCodeVAddr+0x30, &u32DVBS2_DJB_START_ADDR, 4);

	mtk_demod_write_byte(DMDMcuBase + 0x03, 0x50);
	mtk_demod_write_byte(DMDMcuBase + 0x03, 0x00);
	mtk_demod_write_byte(DMDMcuBase + 0x00, 0x00);

	ret = intern_dvbs_reset();

	return ret;
}

void intern_dvbs_init_clk(struct dvb_frontend *fe)
{
	u16	u16_temp_val = 0;
	struct mtk_demod_dev *demod_priv = fe->demodulator_priv;

	u32DMD_DVBS_FW_START_ADDR = demod_priv->dram_base_addr;
	pr_notice("u32DMD_DVBS_FW_START_ADDR address = 0x%x\n",
		u32DMD_DVBS_FW_START_ADDR);
	u32DVBS2_DJB_START_ADDR = u32DMD_DVBS_FW_START_ADDR + 0x10000;
	pr_notice("u32DMD_DVBS_DJB_START_ADDR address = 0x%x\n",
		u32DVBS2_DJB_START_ADDR);
	pr_info("[%s] is called\n", __func__);
	mtk_demod_write_byte(0x101e39, 0x00);
	mtk_demod_write_byte(0x101e68, 0xfc);
	mtk_demod_write_byte(0x10331f, 0x00);
	mtk_demod_write_byte(0x10331e, 0x10);

	mtk_demod_write_byte(0x103301, 0x01);
	mtk_demod_write_byte(0x103300, 0x11);

	mtk_demod_write_byte(0x103309, 0x00);
	mtk_demod_write_byte(0x103308, 0x00);
	mtk_demod_write_byte(0x103321, 0x00);
	mtk_demod_write_byte(0x103320, 0x00);

	mtk_demod_write_byte(0x103302, 0x01);

	mtk_demod_write_byte(0x103302, 0x00);

	mtk_demod_write_byte(0x103314, 0x04);
	mtk_demod_write_byte(0x111f01, 0x00);
	mtk_demod_write_byte(0x111f00, 0x08);

	mtk_demod_write_byte(0x153D01, 0x00);
	mtk_demod_write_byte(0x153D00, 0x00);

	mtk_demod_write_byte(0x153D05, 0x00);
	mtk_demod_write_byte(0x153D04, 0x00);

	mtk_demod_write_byte(0x153D03, 0x00);
	mtk_demod_write_byte(0x153D02, 0x00);

	mtk_demod_write_byte(0x153D07, 0x00);
	mtk_demod_write_byte(0x153D06, 0x00);

	u16_temp_val = (u16)(u32DMD_DVBS_FW_START_ADDR>>16);
	mtk_demod_write_byte(0x153D1b, (u8)(u16_temp_val>>8));
	mtk_demod_write_byte(0x153D1a, (u8)(u16_temp_val));

	mtk_demod_write_byte(0x153D09, 0x00);
	mtk_demod_write_byte(0x153D08, 0x00);

	mtk_demod_write_byte(0x153D0d, 0x00);
	mtk_demod_write_byte(0x153D0c, 0x00);

	mtk_demod_write_byte(0x153D0b, 0x00);
	mtk_demod_write_byte(0x153D0a, 0x01);

	mtk_demod_write_byte(0x153D0f, 0xff);
	mtk_demod_write_byte(0x153D0e, 0xff);

	mtk_demod_write_byte(0x153d18, 0x04);

	mtk_demod_write_byte(0x153D1c, 0x01);

	mtk_demod_write_byte(0x112001, 0x20);
	mtk_demod_write_byte(0x112000, 0x08);

	mtk_demod_write_byte(0x101e39, 0x03);
}

int intern_dvbs_config(struct dvb_frontend *fe)
{
	u8 u8Data = 0;
	u8 u8Data1 = 0;
	u8 u8counter = 0;
	u32 u32CurrentSR;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	pr_info("[%s] is called\n", __func__);
	u32CurrentSR = c->symbol_rate;///1000;  //KHz

	intern_dvbs_reset();

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_9,
		HAL_DRIVER_VERSION_KEY);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_A,
		HAL_DRIVER_VERSION);
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FW_VERSION_L, &u8Data);
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FW_VERSION_H, &u8Data1);

	if (u8Data1 < HAL_FW_VERSION)
		pr_err("Demod FW .dat is TOO OLD! Please Update to Newest One!!!!!\n");

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_11, 0x01);
	u8DemodLockFlag = 0;

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_L,
		u32CurrentSR&0xff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_H,
		(u32CurrentSR>>8)&0xff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DMDTOP_DBG_5,
		(u32CurrentSR>>16)&0xff);
	pr_info(">>>SymbolRate %d<<<\n", u32CurrentSR);

	if (c->inversion) {
		mtk_demod_mbx_dvb_read(TOP_REG_BASE+0x60, &u8Data);
		u8Data |= (0x02);
		mtk_demod_mbx_dvb_write(TOP_REG_BASE+0x60, u8Data);
	}

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_TS_SERIAL, 0x00);
	u8Data = mtk_demod_read_byte(0x001E03);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DMDTOP_DBG_4, u8Data);

	mtk_demod_mbx_dvb_read((TOP_REG_BASE + 0x60*2), &u8Data);
	u8Data = (u8Data&0xF0)|0x01;
	mtk_demod_mbx_dvb_write((TOP_REG_BASE + 0x60*2), u8Data);
	mtk_demod_mbx_dvb_read((TOP_REG_BASE + 0x60*2), &u8Data);

	pr_info("INTERN_DVBS_config TOP_60=%d\n", u8Data);
	u8counter = 20;
	while (((u8Data&0x01) == 0x00) && (u8counter != 0)) {
		mtk_demod_delay_ms(1);
		u8Data |= 0x01;
		mtk_demod_mbx_dvb_write(TOP_REG_BASE + 0x60*2, u8Data);
		mtk_demod_mbx_dvb_read(TOP_REG_BASE + 0x60*2, &u8Data);
		u8counter--;
	}

	if ((u8Data & 0x01) == 0x00)
		return 0;

	mtk_demod_write_byte(MBRegBase + (0x0e)*2, 0x01);// FSM_EN
	u32ChkScanTimeStartDVBS = mtk_demod_get_time();
	return 0;
}


int intern_dvbs_get_ts_divnum(u16 *fTSDivNum)
{
	u8 u8Data = 0;
	u32	  u32SymbolRate = 0;
	u8 ISSY_EN = 0;
	u8 code_rate_idx = 0;
	u8 pilot_flag = 0;
	u8 fec_type_idx = 0;
	u8 mod_type_idx = 0;
	u16 k_bch_array[2][42] = {
		{
			16008, 21408, 25728, 32208, 38688, 43040, 48408, 51648,
			53840, 57472, 58192, 14208, 18528, 28968, 32208, 34368,
			35448, 35808, 37248, 37248, 38688, 40128, 41208, 41568,
			43008, 44448, 44808, 45888, 46608, 47328, 47328, 48408,
			50208, 50208, 55248, 0, 0, 0, 0, 0, 0, 0
		},
		{
			3072, 5232, 6312, 7032, 9552, 10632, 11712, 12432,
			13152, 14232, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3792, 4152, 4872,
			7392, 8472, 9192, 11352
		}
	};
	u16 n_ldpc_array[2] = {64800, 16200};
	u64 pilot_term = 0;
	u64 k_bch;
	u64 n_ldpc;
	u32 TS_div_100;
	u64 SR;
	u16 ts_div_num_offset = 100;
	u16 ts_div_num_margin_ratio = 101;

	intern_dvbs_get_symbol_rate(&u32SymbolRate);
	SR = (u64)u32SymbolRate;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &u8Data);//V

	if (!u8Data) {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE,
			&code_rate_idx);
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FEC_TYPE, &fec_type_idx);
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MOD_TYPE, &mod_type_idx);
		modulation_order = modulation_order_array[mod_type_idx];
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_PILOT_FLAG,
			&pilot_flag);

		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_ISSY_ACTIVE, &ISSY_EN);
		k_bch = (u64)k_bch_array[fec_type_idx][code_rate_idx];
		n_ldpc = n_ldpc_array[fec_type_idx];
		pilot_term = (u64)(n_ldpc*36/modulation_order/1440
			) * pilot_flag;
		TS_Rate = (((k_bch-80)*SR*1000)/((n_ldpc/modulation_order)+90+
			pilot_term));
		if (_bSerialTS) {
			TS_div_100 = (u32)((2880000000
			/((k_bch*SR*ts_div_num_margin_ratio)
			/(n_ldpc/modulation_order+90+pilot_term))) / 2
			)-ts_div_num_offset;
		} else {
			TS_div_100 = (u32)((2880000000/((k_bch*SR
			/(n_ldpc/modulation_order+90+pilot_term)/8)
			*ts_div_num_margin_ratio))/2)-ts_div_num_offset;
		}
		*fTSDivNum = (u16)(TS_div_100/100);
	} else {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE, &u8Data);
		switch (u8Data) {
		case 0x00: //CR 1/2
			TS_Rate = (SR*2*(1*188000)/(2*204));
			if (_bSerialTS) {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(1*188)/(2*204)))
				/ 2)-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(1*188)/(2*204)/8))
				/ 2)-ts_div_num_offset;
			}
			break;
		case 0x01: //CR 2/3
			TS_Rate = (SR*2*(2*188000)/(3*204));
			if (_bSerialTS) {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(2*188)/(3*204)))
				/ 2)-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(2*188)/(3*204)/8))
				/ 2)-ts_div_num_offset;
			}
			break;
		case 0x02: //CR 3/4
			TS_Rate = (SR*2*(3*188000)/(4*204));
			if (_bSerialTS) {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(3*188)/(4*204)))
				/ 2)-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(3*188)/(4*204)/8))
				/ 2)-ts_div_num_offset;
			}
			break;
		case 0x03: //CR 5/6
			TS_Rate = (SR*2*(5*188000)/(6*204));
			if (_bSerialTS) {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(5*188)/(6*204)))
				/ 2)-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(5*188)/(6*204)/8))
				/ 2)-ts_div_num_offset;
			}
			break;
		case 0x04: //CR 7/8
			TS_Rate = (SR*2*(7*188000)/(8*204));
			if (_bSerialTS) {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(7*188)/(8*204)))
				/ 2)-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(7*188)/(8*204)/8))
				/ 2)-ts_div_num_offset;
			}
			break;
		default:
			TS_Rate = (SR*2*(7*188000)/(8*204));
			if (_bSerialTS) {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(7*188)/(8*204)))
				/ 2)-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)((2880000000/(SR*
				ts_div_num_margin_ratio*2*(7*188)/(8*204)/8))
				/ 2)-ts_div_num_offset;
			}
			break;
		}

		*fTSDivNum = (u16)(TS_div_100/100);
	}
	if (*fTSDivNum > 0x1F)
		*fTSDivNum = 0x1F;

	if (*fTSDivNum < 1)
		*fTSDivNum = 1;

	return 0;
}

int intern_dvbs_get_ts_rate(u64 *clock_rate)
{
	if (u8DemodLockFlag)
		*clock_rate = TS_Rate;

	return 0;
}

int intern_dvbs_version(u16 *ver)
{
	u8 tmp = 0;
	u16 u16_intern_dvbs_Version;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FW_VERSION_L, &tmp);
	u16_intern_dvbs_Version = tmp;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FW_VERSION_H, &tmp);
	u16_intern_dvbs_Version = u16_intern_dvbs_Version<<8|tmp;
	*ver = u16_intern_dvbs_Version;

	return 0;
}

int intern_dvbs_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	u8 u8Data = 0; //MS_U8 u8Data2 =0;
	u8 bRet = 1;
	u16 fTSDivNum = 0;
	u8  phase_turning_num = 0;
	u32 u32CurrScanTime = 0;
	u16 timeout = 0;

	bRet &= mtk_demod_mbx_dvb_read((TOP_REG_BASE + 0x60*2),
		&u8Data);
	if ((u8Data&0x02) == 0x00) {
		bRet &= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG,
			&u8Data);
		pr_info(">>>INTERN_DVBS_GetLock MailBox state=%d<<<\n", u8Data);
		if ((u8Data == 15) || (u8Data == 16)) {
			if (u8Data == 15) {
				_bDemodType = 0;   //S
				pr_info(">>>INTERN_DVBS_Demod DVBS Lock<<<\n");
			} else if (u8Data == 16) {
				_bDemodType = 1;//S2
				pr_info(">>>INTERN_DVBS_Demod DVBS2 Lock<<<\n");
			}
			if (u8DemodLockFlag == 0) {
				u8DemodLockFlag = 1;

				intern_dvbs_get_ts_divnum(&fTSDivNum);

				if (fTSDivNum > 0x1F)
					fTSDivNum = 0x1F;
				else if (fTSDivNum < 0x01)
					fTSDivNum = 0x01;

				u8Data = (u8)fTSDivNum;
				cur_TS_CLK_DIV = u8Data;
				u8Data = mtk_demod_read_byte(0x103300);

				if (pre_TS_CLK_DIV != cur_TS_CLK_DIV) {
					u8Data = mtk_demod_read_byte(
						0x103302);
					mtk_demod_write_byte(0x103302,
						u8Data | 0x01);
					phase_turning_num = cur_TS_CLK_DIV>>1;
					mtk_demod_write_byte(0x10330b,
						phase_turning_num);
					u8Data = mtk_demod_read_byte(
						0x103301);
					u8Data |= 0x40;//enable phase tuning
					u8Data &= ~(0x08);//disable prolarity
					mtk_demod_write_byte(0x103301,
						u8Data);
					mtk_demod_write_byte(0x103300,
						cur_TS_CLK_DIV);
					u8Data = mtk_demod_read_byte(
						0x103300);
					pre_TS_CLK_DIV = u8Data;

					u8Data = mtk_demod_read_byte(
						0x103302);
					u8Data = (u8Data & ~(0x01));
					mtk_demod_write_byte(0x103302,
						u8Data);
				}
			}
			intern_dvbs_ts_enable(1);

			*status = FE_HAS_LOCK;
			bRet = 1;
		} else {
			if (intern_dvbs_check_tr_ever_lock()) {
				*status = FE_HAS_SIGNAL;
				timeout = 3000;
			} else
				timeout = 1500;

			u32CurrScanTime = mtk_demod_get_time();
			if (u32CurrScanTime - u32ChkScanTimeStartDVBS > timeout)
				*status = FE_TIMEDOUT;
			if (u8DemodLockFlag == 1)
				u8DemodLockFlag = 0;

			bRet = 0;
		}
	} else
		bRet = 1;

	return bRet;
}

int intern_dvbs_get_snr(s16 *snr)
{
	int status = 0;
	u8  u8Data = 0;
	//NDA SNR
	u16 NDA_SNR_A = 0;
	u16 NDA_SNR_AB = 0;

	mtk_demod_mbx_dvb_read(FRONTEND_INFO_07, &u8Data);
	u8Data |= 0x01;
	mtk_demod_mbx_dvb_write(FRONTEND_INFO_07, u8Data);

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_DMDTOP_DBG_7,
		&u8Data);
	NDA_SNR_A = u8Data;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_DMDTOP_DBG_6,
		&u8Data);
	NDA_SNR_AB = u8Data;

	mtk_demod_mbx_dvb_read(FRONTEND_INFO_07, &u8Data);
	u8Data &= (~0x01);
	mtk_demod_mbx_dvb_write(FRONTEND_INFO_07, u8Data);

	*snr = (NDA_SNR_A*256)+NDA_SNR_AB;
	pr_info("[DVBS]: NDA_SNR: %d\n",
		(NDA_SNR_A*256)+NDA_SNR_AB);

	return status;
}

int intern_dvbs_get_code_rate(struct dtv_frontend_properties *p)
{
	u8 u8Data = 0;
	int status = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &u8Data);
	if (!u8Data) {
		//d0: 1/4, d1: 1/3, d2: 2/5, d3: 1/2, d4: 3/5, d5: 2/3,
		//d6: 3/4, d7: 4/5, d8: 5/6, d9: 8/9, d10: 9/10, d11: 2/9,
		//d12: 13/45, d13: 9/20, d14: 90/180, d15: 96/180,
		//d16: 11/20, d17: 100/180, d18: 104/180, d19: 26/45
		//d20: 18/30, d21: 28/45, d22: 23/36, d23: 116/180,
		//d24: 20/30, d25: 124/180, d26: 25/36, d27: 128/180,
		//d28: 13/18, d29: 132/180, d30: 22/30, d31: 135/180,
		//d32: 140/180, d33: 7/9, d34: 154/180, d35: 11/45,
		//d36: 4/15, d37: 14/45, d38: 7/15, d39: 8/15, d40: 26/45,
		//d41: 32/45

		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE, &u8Data);
		switch (u8Data) {
		case 0x00: {
			p->fec_inner = FEC_1_4;
			break;
		}
		case 0x01: {
			p->fec_inner = FEC_1_3;
			break;
		}
		case 0x02: {
			p->fec_inner = FEC_2_5;
			break;
		}

		case 0x03: {
			p->fec_inner = FEC_1_2;
			break;
		}

		case 0x04: {
			p->fec_inner = FEC_3_5;
			break;
		}

		case 0x05: {
			p->fec_inner = FEC_2_3;
			break;
		}

		case 0x06: {
			p->fec_inner = FEC_3_4;
			break;
		}

		case 0x07: {
			p->fec_inner = FEC_4_5;
			break;
		}

		case 0x08: {
			p->fec_inner = FEC_5_6;
			break;
		}

		case 0x09: {
			p->fec_inner = FEC_8_9;
			break;
		}

		case 0x0A: {
			p->fec_inner = FEC_9_10;
			break;
		}

		case 0x0B: {
			p->fec_inner = FEC_2_9;
			break;
		}

		case 0x0C: {
			p->fec_inner = FEC_13_45;
			break;
		}

		case 0x0D: {
			p->fec_inner = FEC_9_20;
			break;
		}

		case 0x0E: {
			p->fec_inner = FEC_90_180;
			break;
		}

		case 0x0F: {
			p->fec_inner = FEC_96_180;
			break;
		}

		case 0x10: {
			p->fec_inner = FEC_11_20;
			break;
		}

		case 0x11: {
			p->fec_inner = FEC_100_180;
			break;
		}

		case 0x12: {
			p->fec_inner = FEC_104_180;
			break;
		}

		case 0x13: {
			p->fec_inner = FEC_26_45_L;
			break;
		}

		case 0x14: {
			p->fec_inner = FEC_18_30;
			break;
		}

		case 0x15: {
			p->fec_inner = FEC_28_45;
			break;
		}

		case 0x16: {
			p->fec_inner = FEC_23_36;
			break;
		}

		case 0x17: {
			p->fec_inner = FEC_116_180;
			break;
		}

		case 0x18: {
			p->fec_inner = FEC_20_30;
			break;
		}

		case 0x19: {
			p->fec_inner = FEC_124_180;
			break;
		}

		case 0x1A: {
			p->fec_inner = FEC_25_36;
			break;
		}

		case 0x1B: {
			p->fec_inner = FEC_128_180;
			break;
		}

		case 0x1C: {
			p->fec_inner = FEC_13_18;
			break;
		}

		case 0x1D: {
			p->fec_inner = FEC_132_180;
			break;
		}

		case 0x1E: {
			p->fec_inner = FEC_22_30;
			break;
		}

		case 0x1F: {
			p->fec_inner = FEC_135_180;
			break;
		}

		case 0x20: {
			p->fec_inner = FEC_140_180;
			break;
		}

		case 0x21: {
			p->fec_inner = FEC_7_9;
			break;
		}

		case 0x22: {
			p->fec_inner = FEC_154_180;
			break;
		}

		case 0x23: {
			p->fec_inner = FEC_11_45;
			break;
		}

		case 0x24: {
			p->fec_inner = FEC_4_15;
			break;
		}

		case 0x25: {
			p->fec_inner = FEC_14_45;
			break;
		}

		case 0x26: {
			p->fec_inner = FEC_7_15;
			break;
		}

		case 0x27: {
			p->fec_inner = FEC_8_15;
			break;
		}

		case 0x28: {
			p->fec_inner = FEC_26_45_S;
			break;
		}

		case 0x29: {
			p->fec_inner = FEC_32_45;
			break;
		}

		default:
			p->fec_inner = FEC_1_2;
			break;
		}
	} else {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE, &u8Data);
		switch (u8Data) {
		case 0x00:
			p->fec_inner = FEC_1_2;
			break;
		case 0x01:
			p->fec_inner = FEC_2_3;
			break;
		case 0x02:
			p->fec_inner = FEC_3_4;
			break;
		case 0x03:
			p->fec_inner = FEC_5_6;
			break;
		case 0x04:
			p->fec_inner = FEC_7_8;
			break;
		default:
			p->fec_inner = FEC_7_8;
			break;
		}
	}
	return status;
}

int intern_dvbs_get_symbol_rate(u32 *u32SymbolRate)
{
	u8  tmp = 0;
	u16 u16SymbolRateTmp = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_H, &tmp);
	u16SymbolRateTmp = tmp;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_L, &tmp);
	u16SymbolRateTmp = (u16SymbolRateTmp<<8)|tmp;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_DMDTOP_DBG_5, &tmp);
	*u32SymbolRate = (tmp<<16)|u16SymbolRateTmp;

	return 0;
}
int intern_dvbs_ts_enable(int enable)
{
	u8 u8Data = 0;

	mtk_demod_mbx_dvb_read(DVBTM_REG_BASE+0x20*2, &u8Data);
	if (enable == 1)
		u8Data |= 0x04;
	else
		u8Data &= (~0x04);

	mtk_demod_mbx_dvb_write(DVBTM_REG_BASE+0x20*2, u8Data);
	return 0;
}
int intern_dvbs_get_frontend(struct dvb_frontend *fe,
				struct dtv_frontend_properties *p)
{
	u8  reg;
	u32 ret = 0;
	u32 Symbol_Rate;
	s32 cfo;
	u16 ifagc;
	u16 sqi;
	s16 snr;
	u32 post_ber;
	u32 pkt_err;

	if (mtk_demod_read_byte(MBRegBase + (0x0e)*2) != 0x01) {
		pr_err("[%s] Demod is inavtive !\n", __func__);
		return 0;
	}
	//Symbol rate
	ret = intern_dvbs_get_symbol_rate(&Symbol_Rate);
	p->symbol_rate = Symbol_Rate;

	//Frequency offset
	ret = intern_dvbs_get_freqoffset(&cfo);
	p->frequency_offset = cfo;

	//IFAGC
	ret = intern_dvbs_get_ifagc(&ifagc);
	p->agc_val = ifagc;

	//SNR
	ret = intern_dvbs_get_snr(&snr);
	p->cnr.stat[0].scale = FE_SCALE_DECIBEL;
	p->cnr.stat[0].uvalue = ((snr+128)/256);

	//BER
	ret = intern_dvbs_get_ber(&post_ber);
	p->post_ber = post_ber;

	//error count
	ret = intern_dvbs_get_pkterr(&pkt_err);
	p->block_error.stat[0].scale = FE_SCALE_COUNTER;
	p->block_error.stat[0].uvalue = pkt_err;

	//SQI
	ret = intern_dvbs_get_signal_quality(&sqi);
	p->quality.stat[0].scale = FE_SCALE_RELATIVE;
	p->quality.stat[0].uvalue = sqi;

	//System
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &reg);
	if (!reg)
		p->delivery_system = SYS_DVBS2;
	else
		p->delivery_system = SYS_DVBS;

	//Modulation
	if (p->delivery_system == SYS_DVBS2) {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MOD_TYPE, &reg);
		switch (reg) {
		case 0:
			p->modulation = QPSK;
			break;
		case 1:
			p->modulation = PSK_8;
			break;
		case 2:
			p->modulation = APSK_16;
			break;
		case 3:
			p->modulation = APSK_32;
			break;
		case 4:
			p->modulation = APSK_8;
			break;
		case 5:
			p->modulation = APSK_8_8;
			break;
		case 6:
			p->modulation = APSK_4_8_4_16;
			break;
		}
		// Code rate
		ret = intern_dvbs_get_code_rate(p);

		//Pilot
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_PILOT_FLAG, &reg);
		if (reg == 1)
			p->pilot = PILOT_ON;
		else
			p->pilot = PILOT_OFF;

		//FEC Type
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FEC_TYPE, &reg);
		if (reg == 1)
			p->fec_type = FEC_SHORT;
		else
			p->fec_type = FEC_LONG;

		//Rolloff
		mtk_demod_mbx_dvb_read(DVBS2OPPRO_ROLLOFF_DET_DONE, &reg);
		if (reg) {
			mtk_demod_mbx_dvb_read(DVBS2OPPRO_ROLLOFF_DET_VALUE,
				&reg);
			reg = (reg & 0x70) >> 4;
			switch (reg) {
			case 0:
				p->rolloff = ROLLOFF_35;
				break;
			case 1:
				p->rolloff = ROLLOFF_25;
				break;
			case 2:
				p->rolloff = ROLLOFF_20;
				break;
			case 3:
				p->rolloff = ROLLOFF_15;
				break;
			case 4:
				p->rolloff = ROLLOFF_10;
				break;
			case 5:
				p->rolloff = ROLLOFF_5;
				break;
			}
		}
		if (intern_dvbs2_vcm_check()) {
			p->vcm_enable = 1;
			mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_IS_ID, &reg);
			/* DVBS2 ISID */
			p->stream_id = reg;

			/* DVBS2 ISID array */
			intern_dvbs2_get_isid_table(&(p->isid_array[0]));
		} else {
			p->vcm_enable = 0;
			p->stream_id = 0;
		}
	} else {
		p->modulation = QPSK;  //modulation
		p->pilot = PILOT_OFF;  //Pilot
		ret = intern_dvbs_get_code_rate(p);
		p->rolloff = ROLLOFF_35;  //Rolloff
	}
	return ret;
}

int intern_dvbs_check_tr_ever_lock(void)
{
	u8 tr_ever_lock = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_TR_LOCK_FLAG, &tr_ever_lock);

	if (tr_ever_lock)
		return 1;
	else
		return 0;
}

int intern_dvbs_get_ifagc(u16 *ifagc)
{
	u16 u16Data = 0;
	u8  u8Data = 0;

	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE+0x11*2, &u8Data);
	u8Data = (u8Data&0xF0)|0x03;
	mtk_demod_mbx_dvb_write(FRONTEND_REG_BASE+0x11*2, u8Data);

	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE+0x02*2+1, &u8Data);
	u8Data |= 0x80;
	mtk_demod_mbx_dvb_write(FRONTEND_REG_BASE+0x02*2+1, u8Data);

	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE+0x12*2+1, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE+0x12*2, &u8Data);
	u16Data = (u16Data<<8)|u8Data;
	*ifagc = u16Data;

	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE+0x02*2+1, &u8Data);
	u8Data &= ~(0x80);
	mtk_demod_mbx_dvb_write(FRONTEND_REG_BASE+0x02*2+1, u8Data);

	pr_info("Tuner IFAGC = %d\n", u16Data);
	return 0;
}

int intern_dvbs_get_rf_level(u16 ifagc, u16 *rf_level)
{
	u8  u8Index = 0;


	for (u8Index = 0; u8Index < 47; u8Index++) {
		if (ifagc >= _u16SignalLevel[u8Index][0]) {
			if (u8Index >= 1) {
				*rf_level = (_u16SignalLevel[u8Index][1])+(
					(_u16SignalLevel[u8Index-1][1] -
					_u16SignalLevel[u8Index][1])
				*(ifagc-_u16SignalLevel[u8Index][0]) /
				(_u16SignalLevel[u8Index-1][0] -
				_u16SignalLevel[u8Index][0]));
			} else {
				*rf_level = _u16SignalLevel[u8Index][1];
			}
			break;
		}
	}
//---------------------------------------------------
	if (*rf_level > 950)
		*rf_level = 950;

	pr_info("INTERN_DVBS GetTunrSignalLevel_PWR %d\n", *rf_level);

	return 0;
}

int intern_dvbs_get_pkterr(u32 *pkterr)//V
{
	u8 u8Data = 0;
	u16 u16PktErr = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &u8Data);
	if (!u8Data) {
		mtk_demod_mbx_dvb_read(DVBS2FEC_REG_BASE+0x02*2, &u8Data);
		mtk_demod_mbx_dvb_write(DVBS2FEC_REG_BASE+0x02*2,
			u8Data|0x01);

		mtk_demod_mbx_dvb_read(DVBS2FEC_REG_BASE+0x26*2+1, &u8Data);
		u16PktErr = u8Data;
		mtk_demod_mbx_dvb_read(DVBS2FEC_REG_BASE+0x26*2, &u8Data);
		u16PktErr = (u16PktErr << 8)|u8Data;

		mtk_demod_mbx_dvb_read(DVBS2FEC_REG_BASE+0x02*2, &u8Data);
		mtk_demod_mbx_dvb_write(DVBS2FEC_REG_BASE+0x02*2,
			u8Data&(~0x01));
	} else {
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x33*2+1, &u8Data);
		u16PktErr = u8Data;
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x33*2, &u8Data);
		u16PktErr = (u16PktErr << 8)|u8Data;
	}
	*pkterr = u16PktErr;
	pr_info("[DVBS]: pktErr: %d\n", u16PktErr);
	return 0;
}

int intern_dvbs_get_signal_strength(s32 rssi, u16 *strength)
{
	if (rssi <= 400)
		*strength = 100;
	else if (rssi <= 510)
		*strength = 100-((rssi-400)*2/11);
	else if (rssi <= 780)
		*strength = 80-((rssi-510)*2/9);
	else if (rssi <= 800)
		*strength = 800-rssi;
	else
		*strength = 0;

	pr_info(">>>>>Signal Strength(SSI) = %d\n", *strength);
	return 0;
}

int intern_dvbs_get_signal_quality(u16 *quality)
{
	u32 BitErrPeriod, BitErr;
	u64 fber;
	s32 log_ber;
	s16 NDA_SNR;
	u8 Mod_Type_idx = 0;
	u8 CR_Type_idx = 0;
	s16 CN;
	s16 CN_rel;
	u32 ber;
	s16 critical_cn[2][12] = {
	// 1/4  1/3  2/5  1/2  3/5  2/3  3/4   4/5   5/6  7/8 8/9  9/10
	{-436, -333, -26, 282, 640, 871, 1127, 1280, 1408, 0, 1690, 1767},
	{  0,    0,   0,   0,  1511, 1869, 2228, 0, 2688, 0, 2919, 2996}
	};

	if (u8DemodLockFlag) {
		if (_bDemodType) {
			intern_dvbs_get_snr(&NDA_SNR);
			mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MOD_TYPE,
				&Mod_Type_idx);
			mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE,
				&CR_Type_idx);
			//8PSK 9/10
			if ((Mod_Type_idx <= 1) && (CR_Type_idx <= 11)) {
				CN = critical_cn[Mod_Type_idx][CR_Type_idx];
				CN_rel = NDA_SNR-CN;
				if (Mod_Type_idx == 0 /*QPSK*/) {
					if (CN_rel <= 0)//SNR<=critical CN
						*quality = (u16)(20*NDA_SNR)/CN;
					else if (CN_rel <= 512) {
						*quality = (u16)(20+(10*CN_rel)/
							256);//CN<SNR<CN+2
					} else if (CN_rel <= 1024) {
						*quality = (u16)40+((10*
						(CN_rel-512))/128);
						//CN+2<SNR<CN+4
					} else if (CN_rel <= 1536) {
						//CN+4<SNR<CN+6
						*quality = (u16)80+((10*
						(CN_rel-1024))/256);
					} else
						*quality = 100;
				} else if ((Mod_Type_idx == 1) && (CR_Type_idx
					== 4)) {
					if (CN_rel <= 0)//NDA_SNR<=critical CN
						*quality = (u16)(20*NDA_SNR)/CN;
					else if (CN_rel <= 180) {
						//CN<NDA_SNR<CN+0.7
						*quality = (u16)(20+(10*
							CN_rel)/180);
					} else if (CN_rel <= 538) {
						//CN+0.7<NDA_SNR<CN+2.1
						*quality = (u16)(30+((10*
							(CN_rel-180))/359));
					} else if (CN_rel <= 794) {
						//CN+2.1<NDA_SNR<CN+3.1
						*quality = (u16)(40+((10*
							(CN_rel-538))/128));
					} else if (CN_rel <= 1050) {
						//CN+3.1<NDA_SNR<CN+4.1
						*quality = (u16)(60+(10*
							(CN_rel-794))/256);
					} else if (CN_rel <= 1434) {
						//CN+4.1<NDA_SNR<CN+5.6
						*quality = (u16)(70+((10*
							(CN_rel-1050))/384));
					} else if (CN_rel <= 1946) {
						//CN+4<NDA_SNR<CN+6
						*quality = (u16)(80+((10*
							(CN_rel-1434))/256));
					} else
						*quality = 100;
				} else if ((Mod_Type_idx == 1) && (CR_Type_idx
					== 5)) {
					if (CN_rel <= 0) {
						//NDA_SNR<=critical CN
						*quality = (u16)(20*NDA_SNR)/CN;
					} else if (CN_rel <= 180) {
						//critical CN<NDA_SNR<CN+0.7
						*quality = (u16)(20+(10*
							CN_rel)/180);
					} else if (CN_rel <= 410) {
						//CN+0.7<NDA_SNR<CN+1.6
						*quality = (u16)(30+((10*
							(CN_rel-180))/231));
					} else if (CN_rel <= 922) {
						//CN+1.6<NDA_SNR<CN+3.6
						*quality = (u16)(40+((10*
							(CN_rel-410))/256));
					} else if (CN_rel <= 1178) {
						//CN+3.6<NDA_SNR<CN+4.6
						*quality = (u16)(60+(10*
							(CN_rel-922))/128);
					} else if (CN_rel <= 1690) {
						//CN+4.6<NDA_SNR<CN+6.6
						*quality = (u16)(80+((10*
							(CN_rel-1178))/256));
					} else
						*quality = 100;
				} else if ((Mod_Type_idx == 1) && (CR_Type_idx
					== 6)) {
					if (CN_rel <= 0) {
						//NDA_SNR<=critical CN
						*quality = (u16)(20*NDA_SNR)/CN;
					} else if (CN_rel <= 180) {
						//critical CN<NDA_SNR<CN+0.7
						*quality = (u16)(20+(10*
							CN_rel)/180);
					} else if (CN_rel <= 538) {
						//CN+0.7<NDA_SNR<CN+2.1
						*quality = (u16)(30+((10*
							(CN_rel-180))/359));
					} else if (CN_rel <= 717) {
						//CN+2.1<NDA_SNR<CN+2.8
						*quality = (u16)(40+((10*
							(CN_rel-538))/180));
					} else if (CN_rel <= 1101) {
						//CN+2.8<NDA_SNR<CN+4.3
						*quality = (u16)(50+(10*
							(CN_rel-717))/128);
					} else if (CN_rel <= 1613) {
						//CN+4.3<NDA_SNR<CN+6.3
						*quality = (u16)(80+((10*
							(CN_rel-1101))/256));
					} else
						*quality = 100;
				} else if ((Mod_Type_idx == 1) && (CR_Type_idx
					== 8)) {
					if (CN_rel <= 0) {
						//NDA_SNR<=critical CN
						*quality = (u16)(20*NDA_SNR)/CN;
					} else if (CN_rel <= 128) {
						//critical CN<NDA_SNR<CN+0.5
						*quality = (u16)(20+(10*
							CN_rel)/128);
					} else if (CN_rel <= 384) {
						//CN+0.5<NDA_SNR<CN+1.5
						*quality = (u16)(30+((10*
							(CN_rel-128))/256));
					} else if (CN_rel <= 896) {
						//CN+1.5<NDA_SNR<CN+3.5
						*quality = (u16)(40+((10*
							(CN_rel-384))/128));
					} else if (CN_rel <= 1408) {
						//CN+3.5<NDA_SNR<CN+5.5
						*quality = (u16)(80+((10*
							(CN_rel-1101))/256));
					} else
						*quality = 100;
				} else if ((Mod_Type_idx == 1) && (CR_Type_idx
					== 10)) {
					if (CN_rel <= 0) {
						//NDA_SNR<=critical CN
						*quality = (u16)(20*NDA_SNR)/CN;
					} else if (CN_rel <= 540) {
						//critical CN<NDA_SNR<CN+2.1
						*quality = (u16)(20+(10*
							CN_rel)/180);
					} else if (CN_rel <= 924) {
						//CN+2.1<NDA_SNR<CN+3.6
						*quality = (u16)(50+((10*
							(CN_rel-540))/128));
					} else if (CN_rel <= 1436) {
						//CN+3.6<NDA_SNR<CN+5.6
						*quality = (u16)(80+((10*
							(CN_rel-924))/256));
					} else
						*quality = 100;
				} else if ((Mod_Type_idx == 1) && (CR_Type_idx
					== 11)) {
					if (CN_rel <= 0) {
						//NDA_SNR<=critical CN
						*quality = (u16)(20*NDA_SNR)/CN;
					} else if (CN_rel <= 103) {
						//critical CN<NDA_SNR<CN+0.4
						*quality = (u16)(20+(10*
							CN_rel)/103);
					} else if (CN_rel <= 461) {
						//CN+0.4<NDA_SNR<CN+1.8
						*quality = (u16)(30+((10*
							(CN_rel-103))/180));
					} else if (CN_rel <= 845) {
						//CN+1.8<NDA_SNR<CN+3.3
						*quality = (u16)(50+((10*
							(CN_rel-461))/128));
					} else if (CN_rel <= 1357) {
						//CN+3.3<NDA_SNR<CN+5.3
						*quality = (u16)(80+((10*
						(CN_rel-845))/256));
					} else
						*quality = 100;
				}
			} else {
				intern_dvbs_get_ber_window(&BitErrPeriod,
					&BitErr);
				if (BitErrPeriod == 0)
					BitErrPeriod = 1;

				if (BitErr <= 0) {
					//fber = 0.5/(BitErrPeriod*64800);
					fber = (u64)BitErrPeriod*64800*2*100;
				} else {
					//fber = BitErr/(BitErrPeriod*64800);
					fber = (u64)BitErrPeriod*64800*
						100/BitErr;
				}
				//log_ber = (-1)*Log10Approx(1/fber);
				log_ber = (-1) * log10approx100(fber);
				//if (log_ber <= ( - 7.0))
				if (log_ber <= (-700))
					*quality = 100;
				else if (log_ber < (-370))
					*quality = (80+(((-370) - log_ber)*
						(100-80) / ((-370) - (-700))));
				else if (log_ber < (-170))
					*quality = (40+(((-170) - log_ber)*
						(80-40) / ((-170) - (-370))));
				else if (log_ber < (-70))
					*quality = (10+(((-70) - log_ber)*
						(40-10) / ((-70) - (-170))));
				else
					*quality = 5;
			}
		} else {
			intern_dvbs_get_ber(&ber);
			if (ber >= 70000)//ber>7*10^-3
				*quality = 0;
			else if (ber >= 40000)//  4*10^-3<ber<7*10^-3
				*quality = (u16)(30-((ber-40000)/1000));
			else if (ber >= 36000)//  3.6*10^-3<ber<4*10^-3
				*quality = (u16)(40-((ber-36000)/400));
			else if (ber >= 10000)//  10^-3<ber<3.6*10^-3
				*quality = (u16)(50-((ber-10000)/2600));
			else if (ber >= 6000)//  6*10^-4<ber<10^-3
				*quality = (u16)(60-((ber-6000)/400));
			else if (ber >= 2000)//  2*10^-4<ber<6*10^-4
				*quality = (u16)(80-((ber-2000)/200));
			else if (ber >= 800)//  8*10^-5<ber<2*10^-4
				*quality = (u16)(90-((ber-800)/100));
			else if (ber >= 300)//  3*10^-5<ber<8*10^-5
				*quality = (u16)(100-((ber-300)/50));
			else//  ber<3*10^-5
				*quality = 100;

		}
		pr_info(">>>>>Signal Quility(SQI) = %d\n", *quality);
	} else {
		*quality = 0;
	}
	return 0;
}

int intern_dvbs_get_ber(u32 *postber)//POST BER //V
{
	u8 reg = 0, reg_frz = 0;
	u16 BitErrPeriod;
	u32 BitErr;
	u32 current_time = 0;

	if (_bDemodType) {
		current_time = mtk_demod_get_time();
		do {
			mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT3, &reg);
		} while (reg == 0xFF &&
			(mtk_demod_get_time() - current_time) < 500);

		mtk_demod_mbx_dvb_read(DVBS2FEC_OUTER_FREEZE, &reg);
		reg |= 0x01;
		mtk_demod_mbx_dvb_write(DVBS2FEC_OUTER_FREEZE, reg);

		// Get LDPC error window
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_ERROR_WINDOW1, &reg);
		BitErrPeriod = reg;
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_ERROR_WINDOW0, &reg);
		BitErrPeriod = (BitErrPeriod << 8) | reg;

		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT3, &reg);
		BitErr = reg;
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT2, &reg);
		BitErr = (BitErr << 8) | reg;
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT1, &reg);
		BitErr = (BitErr << 8) | reg;
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT0, &reg);
		BitErr = (BitErr << 8) | reg;

		// unfreeze outer
		mtk_demod_mbx_dvb_read(DVBS2FEC_OUTER_FREEZE, &reg);
		reg &= ~(0x01);
		mtk_demod_mbx_dvb_write(DVBS2FEC_OUTER_FREEZE, reg);

		//postber*10^7
		*postber = BitErr*10000000/((u32)BitErrPeriod*64800);
	} else {

		mtk_demod_mbx_dvb_read(REG_BACKEND+0x1*2+1, &reg_frz);
		mtk_demod_mbx_dvb_write(REG_BACKEND+0x1*2+1, reg_frz|0x01);

		mtk_demod_mbx_dvb_read(REG_BACKEND+0x23*2+1, &reg);
		BitErrPeriod = reg;

		mtk_demod_mbx_dvb_read(REG_BACKEND+0x23*2, &reg);
		BitErrPeriod = (BitErrPeriod << 8)|reg;

		mtk_demod_mbx_dvb_read(REG_BACKEND+0x36*2+1, &reg);
		BitErr = reg;
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x36*2, &reg);
		BitErr = (BitErr << 8)|reg;
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x35*2+1, &reg);
		BitErr = (BitErr << 8)|reg;
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x35*2, &reg);
		BitErr = (BitErr << 8)|reg;

		reg_frz = reg_frz&(~0x01);
		mtk_demod_mbx_dvb_write(REG_BACKEND+0x1*2+1, reg_frz);

		if (BitErrPeriod == 0)
			BitErrPeriod = 1;

		if (BitErr <= 0) //postber*10^7
			*postber = 5000000 / ((u32)BitErrPeriod*128*188*8);
		else //postber*10^7
			*postber = BitErr*10000000 / ((u32)BitErrPeriod*
				128*188*8);

		if (*postber == 0)
			*postber = 1;

	}
	pr_info("INTERN_DVBS PostVitBER = %d\n", *postber);

	return 0;
}

int intern_dvbs_get_ber_window(u32 *err_period, u32 *err)
{
	u8 reg = 0, reg_frz = 0;
	u32 current_time = 0;

	if (_bDemodType) {
		current_time = mtk_demod_get_time();
		do {
			mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT3, &reg);
		} while (reg == 0xFF && (mtk_demod_get_time() - current_time
			) < 500);

		mtk_demod_mbx_dvb_read(DVBS2FEC_OUTER_FREEZE, &reg);
		reg |= 0x01;
		mtk_demod_mbx_dvb_write(DVBS2FEC_OUTER_FREEZE, reg);

		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_ERROR_WINDOW1, &reg);
		*err_period = reg;
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_ERROR_WINDOW0, &reg);
		*err_period = (*err_period << 8) | reg;

		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT3, &reg);
		*err = reg;
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT2, &reg);
		*err = (*err << 8) | reg;
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT1, &reg);
		*err = (*err << 8) | reg;
		mtk_demod_mbx_dvb_read(DVBS2FEC_LDPC_BER_COUNT0, &reg);
		*err = (*err << 8) | reg;

		mtk_demod_mbx_dvb_read(DVBS2FEC_OUTER_FREEZE, &reg);
		reg &= ~(0x01);
		mtk_demod_mbx_dvb_write(DVBS2FEC_OUTER_FREEZE, reg);

	} else {
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x1*2+1, &reg_frz);
		mtk_demod_mbx_dvb_write(REG_BACKEND+0x1*2+1, reg_frz|0x01);

		mtk_demod_mbx_dvb_read(REG_BACKEND+0x23*2+1, &reg);
		*err_period = reg;

		mtk_demod_mbx_dvb_read(REG_BACKEND+0x23*2, &reg);
		*err_period = (*err_period << 8)|reg;

		mtk_demod_mbx_dvb_read(REG_BACKEND+0x36*2+1, &reg);
		*err = reg;
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x36*2, &reg);
		*err = (*err << 8)|reg;
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x35*2+1, &reg);
		*err = (*err << 8)|reg;
		mtk_demod_mbx_dvb_read(REG_BACKEND+0x35*2, &reg);
		*err = (*err << 8)|reg;

		reg_frz = reg_frz&(~0x01);
		mtk_demod_mbx_dvb_write(REG_BACKEND+0x1*2+1, reg_frz);

		if (*err_period == 0)
			*err_period = 1;

	}
	return 0;
}

int intern_dvbs_get_freqoffset(s32 *cfo)
{
	u8 u8Data, Deci_factor;
	u16 u16Data;
	s32 center_Freq_Offset_total, Fsync_CFO_Value = 0;
	s32 PR_CFO_Value, Fine_CFO_Value;
	u32 u32Data = 0, u32SR = 0;

	// coarse CFO
	mtk_demod_mbx_dvb_write(FRONTENDEXT2_REG_BASE + 0x1C*2+1, 0x08);

	mtk_demod_mbx_dvb_read(FRONTENDEXT2_REG_BASE + 0x22*2+1, &u8Data);
	u32Data = (u8Data&0xF);
	mtk_demod_mbx_dvb_read(FRONTENDEXT2_REG_BASE + 0x22*2, &u8Data);
	u32Data = (u32Data<<8) | u8Data;
	if (u32Data > 2048)
		center_Freq_Offset_total = (-1)*(4096-u32Data);
	else
		center_Freq_Offset_total = u32Data;


	center_Freq_Offset_total = (s32)((s64)(center_Freq_Offset_total)*144000
		/4096);
	pr_info(">>> coarse cfo only = %d [KHz] <<<\n",
		center_Freq_Offset_total);

	// PART II. Mixer CFO
	// One shot cfo at frame sync module for DVBS2 signal
	// POW4 cfo for DVBS signal

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_DECIMATE_FORCED, &Deci_factor);
	Deci_factor = (1<<Deci_factor);

	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE + 0x34*2 + 1, &u8Data);
	u32Data = (u8Data&0x07);
	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE + 0x34*2, &u8Data);
	u32Data = (u32Data << 8) | u8Data;
	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE + 0x33*2 + 1, &u8Data);
	u32Data = (u32Data << 8) | u8Data;
	mtk_demod_mbx_dvb_read(FRONTEND_REG_BASE + 0x33*2, &u8Data);
	u32Data = (u32Data << 8) | u8Data;

	Fsync_CFO_Value = u32Data;

	if (u32Data > 67108864)
		Fsync_CFO_Value = Fsync_CFO_Value-134217728;


	Fsync_CFO_Value = (s32)((Fsync_CFO_Value/8192)/Deci_factor)*1125
		/(1048576/8192);
	center_Freq_Offset_total += Fsync_CFO_Value;

	pr_info(">>> Mixer cfo value =%d  [KHz] <<<\n", Fsync_CFO_Value);

	// Read SR
	mtk_demod_mbx_dvb_read(TOP_REG_BASE + 0x60*2, &u8Data);
	if ((u8Data&0x02) == 0x02) {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE14H, &u8Data);
		u16Data = u8Data;
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE14L, &u8Data);
		u16Data = (u16Data<<8)|u8Data;
		u32SR = u16Data;
	} else {
		intern_dvbs_get_symbol_rate(&u32SR);
	}

	// PART III. Fine CFO (DVBS2 Only)
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &u8Data);
	if (u8Data == 0) {
		mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x04*2 + 1,
			&u8Data);
		u8Data = ((u8Data&0xC0) | 0x02);
		mtk_demod_mbx_dvb_write(DVBS2_INNER_REG_BASE + 0x04*2 + 1,
			u8Data);

		mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x04*2,
			&u8Data);
		u8Data |= 0x10;
		mtk_demod_mbx_dvb_write(DVBS2_INNER_REG_BASE + 0x04*2,
			u8Data);

		mtk_demod_mbx_dvb_read(DVBS2_INNER_EXT_REG_BASE + 0x29*2,
			&u8Data);
		u32Data = u8Data;
		mtk_demod_mbx_dvb_read(DVBS2_INNER_EXT_REG_BASE + 0x28*2 + 1,
			&u8Data);
		u32Data = (u32Data<<8) | u8Data;
		mtk_demod_mbx_dvb_read(DVBS2_INNER_EXT_REG_BASE + 0x28*2,
			&u8Data);
		u32Data = (u32Data<<8) | u8Data;

		mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x04*2,
			&u8Data);
		u8Data &= ~(0x10);
		mtk_demod_mbx_dvb_write(DVBS2_INNER_REG_BASE + 0x04*2,
			u8Data);

		Fine_CFO_Value = u32Data;
		if (Fine_CFO_Value > 8388608)
			Fine_CFO_Value = Fine_CFO_Value - 16777216;

		Fine_CFO_Value = (s32)((s64)(Fine_CFO_Value)*
			u32SR/1048576);
		pr_info(">>> Fine cfo only = %d [KHz] <<<\n", Fine_CFO_Value);
		center_Freq_Offset_total += Fine_CFO_Value;
	}
	//PR CFO
	mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x04*2, &u8Data);
	u8Data |= 0x10;
	mtk_demod_mbx_dvb_write(DVBS2_INNER_REG_BASE + 0x04*2, u8Data);

	mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x79*2+1, &u8Data);
	u32Data = (u8Data&0x1);
	mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x79*2, &u8Data);
	u32Data = (u32Data<<8) | u8Data;
	mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x78*2+1, &u8Data);
	u32Data = (u32Data<<8) | u8Data;
	mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x78*2, &u8Data);
	u32Data = (u32Data<<8) | u8Data;

	mtk_demod_mbx_dvb_read(DVBS2_INNER_REG_BASE + 0x04*2, &u8Data);
	u8Data &= ~(0x10);
	mtk_demod_mbx_dvb_write(DVBS2_INNER_REG_BASE + 0x04*2, u8Data);
	PR_CFO_Value = u32Data;
	if (u32Data >= 0x1000000)
		PR_CFO_Value = PR_CFO_Value - 0x2000000;

	PR_CFO_Value = (s32)((s64)(PR_CFO_Value)*u32SR/134217728);
	pr_info(">>> pr cfo only = %d [KHz] <<<\n", PR_CFO_Value);
	center_Freq_Offset_total += PR_CFO_Value;
	pr_info(">>> Total CFO = %d [KHz] <<<\n", center_Freq_Offset_total);

	*cfo = center_Freq_Offset_total;
	return 0;
}

int intern_dvbs_get_fec_type(u8 *fec_type)
{
	u8 u8Data;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FEC_TYPE, &u8Data);
	*fec_type = u8Data;
	return 0;
}

int intern_dvbs2_set_isid(u8 isid)
{
	u8 VCM_OPT = 0, total_is_id_count = 0, i = 0, j = 0, u8Data = 0;
	u32 current_time = 0;

	// assign IS-ID
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_IS_ID, isid);
	// wait for VCM_OPT == 1 or time out
	current_time = mtk_demod_get_time();
	do {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_VCM_OPT, &VCM_OPT);
	} while (VCM_OPT != 1 && (mtk_demod_get_time() - current_time
		) < 100);

	if (VCM_OPT != 2) {
		// show IS-ID table
		for (i = 0; i < 32; i++) {
			mtk_demod_mbx_dvb_read(DVBS2OPPRO_REG_BASE + 0x30*2+i,
				&u8Data);
			for (j = 0; j < 8; j++) {
				if ((u8Data>>j) & 0x01)
					total_is_id_count++;
			}
		}
	}
	return 0;
}

void intern_dvbs2_get_isid_table(u8 *isid_table)
{
	u8 iter, i, j;
	// get IS-ID
	if (_bDemodType) {
		// get IS-ID table
		for (iter = 0; iter <= 0x0F; ++iter) {
			mtk_demod_mbx_dvb_read(DVBS2OPPRO_REG_BASE + 0x30*2+
			2*iter, &isid_table[2*iter]);
			mtk_demod_mbx_dvb_read(DVBS2OPPRO_REG_BASE + 0x30*2+
			2*iter + 1, &isid_table[2*iter + 1]);
		}

		for (i = 0; i < 32; i++) {
			for (j = 0; j < 8; j++) {
				if ((isid_table[i] >> j)&0x01)
					pr_info("VCM Get IS ID = [%d]\n",
						i*8+j);
			}
		}
	}
}

int intern_dvbs2_vcm_check(void)
{
	u8 VCM_Check = 0;
	int status = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SIS_EN, &VCM_Check);

	if ((VCM_Check == 0x00) && _bDemodType) // VCM signal
		status = 1;
	else // CCM signal
		status = 0;

	return status;
}

int intern_dvbs2_vcm_mode(enum DMD_DVBS_VCM_OPT u8VCM_OPT)
{
	u8VCM_Enabled_Opt = u8VCM_OPT;
	// Change VCM mode
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_VCM_OPT, u8VCM_Enabled_Opt);

	return 0;
}

int intern_dvbs_diseqc_set_tone(enum fe_sec_mini_cmd minicmd)
{
	u8 u8Data = 0;
	u16  u16WaitCount = 0;

	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xC4, 0x01);
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xC0, 0x4E);
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xCC, 0x88);
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0x6C*2+1,
		0x88);

	if (minicmd == SEC_MINI_B) {
		mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xC2,
			0x19);
	} else {
		mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xC2,
			0x11);
	}

	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE + 0xCD, &u8Data);
	u8Data = u8Data|0x3E;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xCD, u8Data);
	mtk_demod_delay_ms(1);
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE + 0xCD, &u8Data);
	u8Data = u8Data&~(0x3E);
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xCD, u8Data);

	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE + 0x6C*2, &u8Data);
	u8Data = u8Data|0x80;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0x6C*2, u8Data);
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE + 0xCD, &u8Data);
	u8Data = u8Data|0x01;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xCD, u8Data);

	do {
		mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x66*2)+1,
			&u8Data);
		mtk_demod_delay_ms(1);
		u16WaitCount++;
	} while (((u8Data&0x01) == 0x01) &&
			(u16WaitCount < INTERN_DVBS_DEMOD_WAIT_TIMEOUT));

	return 1;
}

int intern_dvbs_diseqc_set_22k(enum fe_sec_tone_mode tone)
{
	u8   u8Data = 0;

	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE + 0xC2, &u8Data);
	if (tone == SEC_TONE_ON) {
		u8Data = (u8Data & 0xc7);
		u8Data = (u8Data | 0x08);
	} else
		u8Data = (u8Data&0xc7);

	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xC2, u8Data);
	return 1;
}

int intern_dvbs_diseqc_send_cmd(struct dvb_diseqc_master_cmd *cmd)
{
	u8   u8Data = 0;
	u8   u8Index = 0;
	u16  u16WaitCount = 0;
	u8 DiSEqC_Init_Mode = 0;
	static u8 Tx_Len;

	for (u8Index = 0; u8Index < cmd->msg_len; u8Index++) {
		u8Data = cmd->msg[u8Index];
		mtk_demod_mbx_dvb_write(DVBS2_REG_BASE + 0xC4
			+ u8Index, u8Data);
	}

	//Write_Reg(DVBS2_DISEQC_EN,0x01, 6, 6);
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x60*2),
		&u8Data);
	u8Data |= 0x40;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x60*2),
		u8Data);

	//Write_Reg(DVBS2_DISEQC_EN,0x01, 3, 3);
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x60*2),
		&u8Data);
	u8Data |= 0x08;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x60*2),
		u8Data);

	//REG_BASE[DVBS2_DISEQC_REPLY_TIMEOUT0]=0x00;
	u8Data = 0x00;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x6A*2),
		u8Data);

	//REG_BASE[DVBS2_DISEQC_REPLY_TIMEOUT1]=0x02;
	u8Data = 0x02;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x6A*2)+1,
		u8Data);

	//DiSEqC_Init_Mode=REG_BASE[DVBS2_DISEQC_MOD];
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x61*2),
		&u8Data);
	DiSEqC_Init_Mode = u8Data;

	//Write_Reg(DVBS2_DISEQC_MOD,0x00, 7, 7);//Rx disable
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x61*2),
		&u8Data);
	u8Data &= 0x7F;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x61*2),
		u8Data);

	//Write_Reg(DVBS2_DISEQC_MOD,0x04, 5, 3);//Data mode
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x61*2),
		&u8Data);
	u8Data &= 0xC7;
	u8Data |= 0x20;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x61*2),
		u8Data);

	//Tx_Len=(gDvbsParam[E_S2_MB_DMDTOP_DBG_8]&0x07);
	Tx_Len = ((cmd->msg_len - 1)&0x07);

	//Odd_Enable_Mode=(gDvbsParam[E_S2_MB_DMDTOP_DBG_8]&0x40);
	//Write_Reg(DVBS2_DISEQC_EN,0x01, 2, 2);
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x60*2), &u8Data);
	u8Data |= 0x04;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x60*2), u8Data);

	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x66*2)+1, &u8Data);
	u8Data |= 0x3E;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x66*2)+1, u8Data);

	//TimeDelayms(1);
	mtk_demod_delay_ms(1);

	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x66*2)+1, &u8Data);
	u8Data &= 0xC1;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x66*2)+1, u8Data);

	//Write_Reg(DVBS2_DISEQC_MOD,0x00, 5, 3);
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x61*2), &u8Data);
	u8Data &= 0xC7;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x61*2), u8Data);

	//Write_Reg(DVBS2_DISEQC_CARRY_THRESHOLD,0x62, 7, 0);
	u8Data = 0x62;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x69*2), u8Data);

	//Write_Reg(DVBS2_DISEQC_FCAR,0x88,7,0);  // set fcar
	u8Data = 0x88;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x66*2), u8Data);

	//Write_Reg(DVBS2_DISEQC_TX_NEW_MODE,0x01,7,7); //set tx new mode
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x6C*2), &u8Data);
	u8Data |= 0x80;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x6C*2), u8Data);

	//Write_Reg(DVBS2_DISEQC_FCAR2,0x88,7,0);  // set fcar
	u8Data = 0x88;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x6C*2)+1, u8Data);

	//Write_Reg(DVBS2_DISEQC_MOD,Tx_Len, 2, 0); //Set tx len
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x61*2), &u8Data);
	u8Data &= 0xF8;
	u8Data |= Tx_Len;

	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x61*2), u8Data);

	u8Data = 0x00; //0x11;  let glue layer to control
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x61*2)+1, u8Data);

	//Write_Reg(DVBS2_DISEQC_MOD,0x04, 5, 3);//Data mode
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x61*2), &u8Data);
	u8Data |= 0x20;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x61*2), u8Data);

	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x66*2)+1, &u8Data);
	u8Data |= 0x3E;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x66*2)+1, u8Data);

	//TimeDelayms(1);
	mtk_demod_delay_ms(1);

	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x66*2)+1, &u8Data);
	u8Data &= 0xC1;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x66*2)+1, u8Data);

	//Write_Reg(DVBS2_DISEQC_TX_EN,0x01, 0, 0);//Tx enable
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x66*2)+1, &u8Data);
	u8Data |= 0x01;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x66*2)+1, u8Data);

	u16WaitCount = 0;
	do {
		mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x66*2)+1, &u8Data);
		mtk_demod_delay_ms(1);
		u16WaitCount++;
	} while (((u8Data&0x01) == 0x01) &&
		(u16WaitCount < INTERN_DVBS_DEMOD_WAIT_TIMEOUT));

	//Write_Reg(DVBS2_DISEQC_TX_EN,0x00, 0, 0);//Tx disable
	mtk_demod_mbx_dvb_read(DVBS2_REG_BASE+(0x66*2)+1, &u8Data);
	u8Data &= 0xFE;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x66*2)+1, u8Data);
	//Write_Reg(DVBS2_DISEQC_MOD,0x00, 7, 7);//Rx disable

	//REG_BASE[DVBS2_DISEQC_MOD]=DiSEqC_Init_Mode;
	mtk_demod_mbx_dvb_write(DVBS2_REG_BASE+(0x61*2), DiSEqC_Init_Mode);

	if (u16WaitCount >= INTERN_DVBS_DEMOD_WAIT_TIMEOUT) {
		pr_err("INTERN_DVBS_Customized_DiSEqC_SendCmd Busy!!!\n");
		return 0;
	}

	return 1;
}
