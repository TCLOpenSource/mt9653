// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <asm/io.h>
#include <linux/string.h>
#include <linux/mm_types.h>
#include <linux/timekeeping.h>
#include <linux/math64.h>
#include "demod_drv_dvbs.h"
#include "demod_hal_dvbs.h"

#define HAL_DRIVER_VERSION_KEY 0xAA
#define HAL_DRIVER_VERSION  0x00
#define HAL_FW_VERSION  0x00
#define INTERN_DVBS_DEMOD_WAIT_TIMEOUT	  6000
#define auto_iq_swap_timeout	  600
#define state_reset_timeout	  100


#define DVBS2_DISEQC_EN               (DVBS2_REG_BASE*2+0x60*2)
#define DVBS2_DISEQC_MOD              (DVBS2_REG_BASE*2+0x61*2)
#define DVBS2_DISEQC_CTRL             (DVBS2_REG_BASE*2+0x61*2+1)
#define DVBS2_DISEQC_TX_RAM_ADDR      (DVBS2_REG_BASE*2+0x62*2)
#define DVBS2_DISEQC_TX_RAM_WDATA     (DVBS2_REG_BASE*2+0x62*2+1)
#define DVBS2_DISEQC_TX_LENGTH        (DVBS2_REG_BASE*2+0x63*2)
#define DVBS2_DISEQC_TX_DATA          (DVBS2_REG_BASE*2+0x63*2+1)
#define DVBS2_DISEQC_FCAR             (DVBS2_REG_BASE*2+0x66*2)
#define DVBS2_DISEQC_STATUS_CLEAR          (DVBS2_REG_BASE*2+0x66*2+1)
#define DVBS2_DISEQC_FIQ_CLEAR_FRONTEND0   (DVBS2_REG_BASE*2+0x36*2)
#define DVBS2_DISEQC_FIQ_CLEAR_FRONTEND1   (DVBS2_REG_BASE*2+0x36*2+1)
#define DVBS2_DISEQC_REPLY_TIMEOUT0        (DVBS2_REG_BASE*2+0x6A*2)
#define DVBS2_DISEQC_REPLY_TIMEOUT1        (DVBS2_REG_BASE*2+0x6A*2+1)
#define DVBS2_DISEQC_CARRY_THRESHOLD       (DVBS2_REG_BASE*2+0x69*2)
#define DVBS2_DISEQC_TX_NEW_MODE           (DVBS2_REG_BASE*2+0x6C*2)
#define DVBS2_DISEQC_FCAR2                 (DVBS2_REG_BASE*2+0x6C*2+1)

static u8 *dram_code_virt_addr;
static unsigned long dvbs_fw_addr;
static u16 ip_version;
u8 u8DemodLockFlag;
u8	   modulation_order;
static  u8 _bDemodType;
static u32	   u32ChkScanTimeStartDVBS;
static  u8		_bSerialTS;
static u8			pre_TS_CLK_DIV;
static u8			cur_TS_CLK_DIV;
static u8			toneBurstFlag;
static u8			tone22k_Flag;
u8 INTERN_DVBS_table[] = {
#include "fw_dmd_dvbs.dat"
};

static unsigned long dvbs2_djb_addr;
const u8 modulation_order_array[12] = {2, 3, 4, 5, 3, 4, 5, 6, 6, 6, 7, 8};
static u64  TS_Rate;
static enum DMD_DVBS_VCM_OPT u8VCM_Enabled_Opt = VCM_Disabled;
/*BEGIN: auto iq swap detection (1/7)*/
static bool  iq_swap_checking_flag;
static bool  iq_swap_done_flag;
static  u16  cf_iq_swap_check;//center frequency
static  u16  bw_iq_swap_check;
static s16 cfo_check_iq_swap;
/*END: auto iq swap detection (1/7)*/
static  u16  _u16BlindScanStartFreq;
static  u16  _u16BlindScanEndFreq;
static  u16  _u16TunerCenterFreq;
static  u16  _u16ChannelInfoIndex;
//Debug Only+
static  u16  _u16NextCenterFreq;
static  u16  _u16PreCenterFreq;
static  u16  _u16LockedSymbolRate;
static  u16  _u16LockedCenterFreq;
static  u16  _u16PreLockedHB;
static  u16  _u16PreLockedLB;
static  u16  _u16CurrentSymbolRate;
static  s16  _s16CurrentCFO;
static  u16  _u16CurrentStepSize;
//Debug Only-
static  u32  _u16ChannelInfoArray[Max_TP_info][Max_TP_number];

static u16 _u16SignalLevel[96][2] = {
    //SONY APH Tuner, SR=30M
	{64982,   950}, {64862,   940}, {64730,   930}, {64582,   920},
	{64432,   910}, {64245,   900}, {64035,   890}, {63828,   880},
	{63590,   870}, {63365,   860}, {63139,   850}, {62885,   840},
	{62636,   830}, {62422,   820}, {62147,   810}, {61926,   800},
	{61741,   790}, {61495,   780}, {61250,   770}, {61030,   760},
	{60887,   750}, {60720,   740}, {60553,   730}, {60370,   720},
	{60194,   710}, {60048,   700}, {59875,   690}, {59732,   680},
	{59602,   670}, {59480,   660}, {59372,   650}, {59267,   640},
	{59155,   630}, {59046,   620}, {58957,   610}, {58865,   600},
	{58770,   590}, {58672,   580}, {58580,   570}, {58495,   560},
	{58415,   550}, {58325,   540}, {58220,   530}, {58110,   520},
	{58006,   510}, {57874,   500}, {57724,   490}, {57591,   480},
	{57428,   470}, {57270,   460}, {57100,   450}, {56916,   440},
	{56695,   430}, {56455,   420}, {56267,   410}, {56090,   400},
	{55930,   390}, {55768,   380}, {55605,   370}, {55460,   360},
	{55300,   350}, {55130,   340}, {54937,   330}, {54686,   320},
	{54464,   310}, {54240,   300}, {54060,   290}, {53868,   280},
	{53635,   270}, {53415,   260}, {53162,   250}, {52883,   240},
	{52650,   230}, {52450,   220}, {52295,   210}, {52122,   200},
	{51964,   190}, {51780,   180}, {51580,   170}, {51340,   160},
	{51127,   150}, {50906,   140}, {50676,   130}, {50462,   120},
	{50260,   110}, {50060,   100}, {49860,    90}, {49688,    80},
	{49505,    70}, {49335,    60}, {49185,    50}, {49025,    40},
	{48866,    30}, {48740,    20}, {48618,    10}, {48508,     0}
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

static int intern_dvbs_soft_stop(void)
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
			pr_err("@@@@@ DVBS SoftStop Fail!\n");
			return -ETIMEDOUT;
		}
	}

	/* MB_CNTL clear */
	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0x00);

	mtk_demod_delay_ms(1);

	return 0;
}

static int intern_dvbs_reset(void)
{
	int ret = 0;

	pr_info("[%s] is called\n", __func__);
	ret = intern_dvbs_soft_stop();

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

	u32ChkScanTimeStartDVBS = mtk_demod_get_time();
	u8DemodLockFlag = 0;
	return ret;
}

int intern_dvbs_exit(struct dvb_frontend *fe)
{
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;
	int ret = 0;

	ret = intern_dvbs_soft_stop();

	/* [0]: reset demod 0 */
	/* [4]: reset demod 1 */
	writeb(0x00, demod_prvi->virt_reset_base_addr);
	mtk_demod_delay_ms(1);
	writeb(0x11, demod_prvi->virt_reset_base_addr);


	return ret;
}

int intern_dvbs_load_code(void)
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

	pr_info("[%s] is called\n", __func__);

	memcpy(dram_code_virt_addr, &INTERN_DVBS_table[0], sizeof(INTERN_DVBS_table));
	memcpy(dram_code_virt_addr+0x30, &dvbs2_djb_addr, sizeof(dvbs2_djb_addr));

	/* disable auto-increase */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03, 0x50);
	/* disable "vdmcu51_if" */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x03, 0x00);
	/* release VD_MCU */
	mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x00);

	if (mtk_demod_mbx_dvb_wait_handshake()) {
		pr_info("############# 1st\n");
		mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x01);
		mtk_demod_delay_ms(1);
		mtk_demod_write_byte(DMD51_REG_BASE + 0x00, 0x00);
		if (mtk_demod_mbx_dvb_wait_handshake()) {
			pr_info("############# 2nd\n");
			return -ETIMEDOUT;
		}
	}

	mtk_demod_write_byte(MB_REG_BASE + 0x00, 0x00);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_9,
		HAL_DRIVER_VERSION_KEY);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_A,
		HAL_DRIVER_VERSION);

	ret = intern_dvbs_reset();
	return ret;
}

void intern_dvbs_init_clk(struct dvb_frontend *fe)
{
	u8 reg;
	u16 u16tmp;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

	dvbs_fw_addr = demod_prvi->dram_base_addr;
	pr_notice("@@@@@@@@@@@ dvbs_fw_addr address = 0x%lx\n",
		dvbs_fw_addr);

	dvbs2_djb_addr = dvbs_fw_addr + 0x10000;
	pr_notice("@@@@@@@@@@@ dvbs2_djb_addr address = 0x%lx\n",
		dvbs2_djb_addr);

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
	reg = (reg&0xfe) | 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x1598);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x1740);
	reg = (reg&0xfe) | 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x1740);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x19c8);
	reg = (reg&0xfe) | 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x19c8);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x19cc);
	reg = (reg&0xfe) | 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x19cc);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x19d0);
	reg = (reg&0xfe) | 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x19d0);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x19d4);
	reg = (reg&0xfe) | 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x19d4);
	/* [9:8][4:0] */
	u16tmp = readw(demod_prvi->virt_clk_base_addr + 0x3040);
	u16tmp = (u16tmp&0xfce0) | 0x0004;
	writew(u16tmp, demod_prvi->virt_clk_base_addr + 0x3040);
	/* [9:8][4:0] */
	u16tmp = readw(demod_prvi->virt_clk_base_addr + 0x3110);
	u16tmp = (u16tmp&0xfce0);
	writew(u16tmp, demod_prvi->virt_clk_base_addr + 0x3110);
	/* [1:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x31c0);
	reg &= 0xfc;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x31c0);

	/* [5:5] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x3b4c);
	reg = (reg&0xdf) | 0x20;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x3b4c);
	/* [0:0] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x3b5c);
	reg = (reg&0xfe) | 0x01;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x3b5c);
	/* [2:2] */
	reg = readb(demod_prvi->virt_clk_base_addr + 0x3b74);
	reg = (reg&0xfb) | 0x04;
	writeb(reg, demod_prvi->virt_clk_base_addr + 0x3b74);

	ip_version = demod_prvi->ip_version;
	mtk_demod_write_byte(DEMOD_TOP_REG_BASE+(0x70*2), ((U8)ip_version)&0xff);
	mtk_demod_write_byte(DEMOD_TOP_REG_BASE+(0x70*2+1), ((U8)(ip_version>>8))&0xff);
	if (ip_version > 0x0100) {
		/* [0:0] */
		reg = readb(demod_prvi->virt_sram_base_addr);
		reg &= 0xfe;
		writeb(reg, demod_prvi->virt_sram_base_addr);

		/* [3:0] */
		reg = readb(demod_prvi->virt_pmux_base_addr);
		reg = (reg&0xf0) | 0x03;
		writeb(reg, demod_prvi->virt_pmux_base_addr);
		/* [11:8] */
		reg = readw(demod_prvi->virt_vagc_base_addr);
		reg = (reg&0xf0ff) | 0x0300;
		writew(reg, demod_prvi->virt_vagc_base_addr);
	} else {
		writew(0x00fc, demod_prvi->virt_sram_base_addr);
	}
	/* [9:8][4:0] */
	u16tmp = readw(demod_prvi->virt_clk_base_addr + 0x3110);
	u16tmp = (u16tmp&0xfcf0) | 0x0014;
	writew(u16tmp, demod_prvi->virt_clk_base_addr + 0x3110);

	writew(0x1105, demod_prvi->virt_ts_base_addr);

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

	u16tmp = (u16)(dvbs_fw_addr>>16);
	mtk_demod_write_byte(MCU2_REG_BASE+0x1b, (u8)(u16tmp>>8));
	mtk_demod_write_byte(MCU2_REG_BASE+0x1a, (u8)u16tmp);
	u16tmp = (u16)(dvbs_fw_addr>>32);
	mtk_demod_write_byte(MCU2_REG_BASE+0x10, (u8)u16tmp);

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

int intern_dvbs_config(struct dvb_frontend *fe)
{
	u8 u8Data = 0;
	u8 u8Data1 = 0;
	u8 u8counter = 0;
	u32 u32CurrentSR;
	int ret = 0;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

	pr_info("[%s] is called\n", __func__);
	u32CurrentSR = c->symbol_rate/1000;///1000;  //KHz

	ret = intern_dvbs_reset();

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_9,
		HAL_DRIVER_VERSION_KEY);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_A,
		HAL_DRIVER_VERSION);
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FW_VERSION_L, &u8Data);
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FW_VERSION_H, &u8Data1);

	if (u8Data1 < HAL_FW_VERSION) {
		pr_err("[mdbgin_merak_demod_dd][%s][%d]Demod FW too OLD\n",
			__func__, __LINE__);
		return -EINVAL;
	}
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_11, 0x01);
	u8DemodLockFlag = 0;

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_L,
		u32CurrentSR&0xff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_H,
		(u32CurrentSR>>8)&0xff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DMDTOP_DBG_5,
		(u32CurrentSR>>16)&0xff);
	pr_info(">>>SymbolRate %d<<<\n", u32CurrentSR);

	if (demod_prvi->if_gain == 1) {//for sony
		if (ip_version >= 0x0200) {
			/* [15:13] */
			mtk_demod_mbx_dvb_read(DMDANA_REG_BASE + 0x9d, &u8Data);
			u8Data = (u8Data&0x1f)|0x40;
			mtk_demod_mbx_dvb_write(DMDANA_REG_BASE + 0x9d, u8Data);
		} else {
			u8Data = 0x01;
			mtk_demod_mbx_dvb_write(DMDANA_REG_BASE + 0x16, u8Data);
			mtk_demod_mbx_dvb_write(DMDANA_REG_BASE + 0x17, u8Data);
		}
	}
	if (c->inversion) {
		mtk_demod_mbx_dvb_read(TOP_REG_BASE+0x60, &u8Data);
		u8Data |= (0x02);
		mtk_demod_mbx_dvb_write(TOP_REG_BASE+0x60, u8Data);
	}

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_TS_SERIAL, 0x00);

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
		return -ETIMEDOUT;

	/* Active state machine */
	mtk_demod_write_byte(MB_REG_BASE + (0x0e)*2, 0x01);
	u32ChkScanTimeStartDVBS = mtk_demod_get_time();
	_bDemodType = 0;
	return 0;
}

static int intern_dvbs_get_symbol_rate(u32 *u32SymbolRate)
{
	u8  tmp = 0;
	u16 u16SymbolRateTmp = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_H, &tmp);
	u16SymbolRateTmp = tmp;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_L, &tmp);
	u16SymbolRateTmp = (u16SymbolRateTmp<<8)|tmp;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_DMDTOP_DBG_5, &tmp);
	*u32SymbolRate = (tmp<<16)|u16SymbolRateTmp;

	pr_info("Symbol rate =%d\n", *u32SymbolRate);

	return 0;
}

static int intern_dvbs_get_ts_divnum(u16 *fTSDivNum)
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
		pilot_term = (u64)(div64_u64(div64_u64((n_ldpc*36), modulation_order), 1440))
			* pilot_flag;
		TS_Rate = (div64_u64(((k_bch-80)*SR*1000), ((div64_u64(n_ldpc, modulation_order))
			+90+pilot_term)));
		if (_bSerialTS) {
			TS_div_100 = (u32)(div64_u64((div64_u64(2880000000,
				(div64_u64((k_bch*SR*ts_div_num_margin_ratio),
				(div64_u64(n_ldpc, modulation_order)+90+pilot_term))))), 2))
				-ts_div_num_offset;
		} else {
			TS_div_100 = (u32)(div64_u64((div64_u64(2880000000,
				((div64_u64(div64_u64(k_bch*SR,
				(div64_u64(n_ldpc, modulation_order)+90+pilot_term)), 8))
				*ts_div_num_margin_ratio))), 2))-ts_div_num_offset;
		}
		*fTSDivNum = (u16)(TS_div_100/100);
	} else {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE, &u8Data);
		switch (u8Data) {
		case 0x00: //CR 1/2
			TS_Rate = (div64_u64(SR*2*(1*188000), (2*204)));
			if (_bSerialTS) {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(SR*
				ts_div_num_margin_ratio*2*(1*188), (2*204)))))
				, 2))-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(
				div64_u64(SR*ts_div_num_margin_ratio*2*(1*188), (2*204)), 8))))
				, 2))-ts_div_num_offset;
			}
			break;
		case 0x01: //CR 2/3
			TS_Rate = (div64_u64(SR*2*(2*188000), (3*204)));
			if (_bSerialTS) {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(SR*
				ts_div_num_margin_ratio*2*(2*188), (3*204)))))
				, 2))-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(
				div64_u64(SR*ts_div_num_margin_ratio*2*(2*188), (3*204)), 8))))
				, 2))-ts_div_num_offset;
			}
			break;
		case 0x02: //CR 3/4
			TS_Rate = (div64_u64(SR*2*(3*188000), (4*204)));
			if (_bSerialTS) {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(SR*
				ts_div_num_margin_ratio*2*(3*188), (4*204)))))
				, 2))-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(
				div64_u64(SR*ts_div_num_margin_ratio*2*(3*188), (4*204)), 8))))
				, 2))-ts_div_num_offset;
			}
			break;
		case 0x03: //CR 5/6
			TS_Rate = (div64_u64(SR*2*(5*188000), (6*204)));
			if (_bSerialTS) {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(SR*
				ts_div_num_margin_ratio*2*(5*188), (6*204)))))
				, 2))-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(
				div64_u64(SR*ts_div_num_margin_ratio*2*(5*188), (6*204)), 8))))
				, 2))-ts_div_num_offset;
			}
			break;
		case 0x04: //CR 7/8
			TS_Rate = (div64_u64(SR*2*(7*188000), (8*204)));
			if (_bSerialTS) {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(SR*
				ts_div_num_margin_ratio*2*(7*188), (8*204)))))
				, 2))-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(
				div64_u64(SR*ts_div_num_margin_ratio*2*(7*188), (8*204)), 8))))
				, 2))-ts_div_num_offset;
			}
			break;
		default:
			TS_Rate = (div64_u64(SR*2*(7*188000), (8*204)));
			if (_bSerialTS) {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(SR*
				ts_div_num_margin_ratio*2*(7*188), (8*204)))))
				, 2))-ts_div_num_offset;
			} else {
				TS_div_100 = (u32)(div64_u64((div64_u64(2880000000, (div64_u64(
				div64_u64(SR*ts_div_num_margin_ratio*2*(7*188), (8*204)), 8))))
				, 2))-ts_div_num_offset;
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

	pr_info("TS Rate =%llu\n", TS_Rate);
	return 0;
}

static int intern_dvbs_version(u16 *ver)
{
	u8 tmp = 0;
	u16 u16_intern_dvbs_Version;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FW_VERSION_L, &tmp);
	u16_intern_dvbs_Version = tmp;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FW_VERSION_H, &tmp);
	u16_intern_dvbs_Version = u16_intern_dvbs_Version<<8|tmp;
	*ver = u16_intern_dvbs_Version;

	pr_info("Demod version =%d\n", u16_intern_dvbs_Version);
	return 0;
}

static int intern_dvbs_get_snr(s16 *snr)
{
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

	return 0;
}

static int intern_dvbs_get_code_rate(struct dtv_frontend_properties *p)
{
	u8 u8Data = 0;

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
	pr_info("Code rate =%d\n", u8Data);
	return 0;
}

static int intern_dvbs_ts_enable(int enable)
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

static int intern_dvbs_check_tr_ever_lock(void)
{
	u8 tr_ever_lock = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_TR_LOCK_FLAG, &tr_ever_lock);

	if (tr_ever_lock)
		return 1;
	else
		return 0;
}

static int intern_dvbs_get_ifagc(u16 *ifagc)
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

static int intern_dvbs_get_rf_level(u16 ifagc, u16 *rf_level)
{
	u8  u8Index = 0;
	u16  SSI_Size = 0;

	SSI_Size = ARRAY_SIZE(_u16SignalLevel);
	for (u8Index = 0; u8Index < SSI_Size; u8Index++) {
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
	if (ifagc < _u16SignalLevel[SSI_Size-1][0])
		*rf_level = 100;
//---------------------------------------------------
	if (*rf_level > 950)
		*rf_level = 950;

	pr_info("INTERN_DVBS GetTunrSignalLevel_PWR %d\n", *rf_level);

	return 0;
}

static int intern_dvbs_get_pkterr(u16 *pkterr)//V
{
	u8 u8Data = 0;
	u16 u16PktErr = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &u8Data);
	if (!u8Data) {
		mtk_demod_mbx_dvb_read(DVBT2FEC_REG_BASE+0x02*2, &u8Data);
		mtk_demod_mbx_dvb_write(DVBT2FEC_REG_BASE+0x02*2,
			u8Data|0x01);

		mtk_demod_mbx_dvb_read(DVBT2FEC_REG_BASE+0x26*2+1, &u8Data);
		u16PktErr = u8Data;
		mtk_demod_mbx_dvb_read(DVBT2FEC_REG_BASE+0x26*2, &u8Data);
		u16PktErr = (u16PktErr << 8)|u8Data;

		mtk_demod_mbx_dvb_read(DVBT2FEC_REG_BASE+0x02*2, &u8Data);
		mtk_demod_mbx_dvb_write(DVBT2FEC_REG_BASE+0x02*2,
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

static int intern_dvbs_get_signal_strength(u16 rssi, u16 *strength)
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

static int intern_dvbs_get_signal_quality(u16 *quality)
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
	u32 biterr_period, biterr;
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
					fber = div64_u64((u64)BitErrPeriod*64800*
						100, BitErr);
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
			intern_dvbs_get_ber_window(&biterr_period, &biterr);
			ber = biterr*BER_unit/(biterr_period*dvbs_err_window);

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
	} else {
		*quality = 0;
	}
	pr_info(">>>>>Signal Quility(SQI) = %d\n", *quality);
	return 0;
}

int intern_dvbs_get_ber_window(u32 *err_period, u32 *err)
{
	u8 reg = 0, reg_frz = 0;

	if (_bDemodType) {
		mtk_demod_mbx_dvb_read(DVBT2FEC_OUTER_FREEZE, &reg_frz);
		reg_frz |= 0x01;
		mtk_demod_mbx_dvb_write(DVBT2FEC_OUTER_FREEZE, reg_frz);

		mtk_demod_mbx_dvb_read(DVBT2FEC_BCH_EFLAG_CNT_NUM1, &reg);
		*err_period = reg;
		mtk_demod_mbx_dvb_read(DVBT2FEC_BCH_EFLAG_CNT_NUM0, &reg);
		*err_period = (*err_period << 8) | reg;

		mtk_demod_mbx_dvb_read(DVBT2FEC_BCH_EFLAG_SUM1, &reg);
		*err = reg;
		mtk_demod_mbx_dvb_read(DVBT2FEC_BCH_EFLAG_SUM0, &reg);
		*err = (*err << 8) | reg;

		mtk_demod_mbx_dvb_read(DVBT2FEC_OUTER_FREEZE, &reg_frz);
		reg_frz &= ~(0x01);
		mtk_demod_mbx_dvb_write(DVBT2FEC_OUTER_FREEZE, reg_frz);

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
	}
	if (*err_period == 0)
		*err_period = 1;

	pr_info("Biterr=%d, Biterr_period= %d\n", *err, *err_period);
	return 0;
}

static int intern_dvbs_get_freqoffset(s32 *cfo)
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


	center_Freq_Offset_total = (s32)((s64)(div64_s64(center_Freq_Offset_total*144000,
		4096)));
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

		Fine_CFO_Value = (s32)((s64)(div64_s64(Fine_CFO_Value*
			u32SR, 1048576)));
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

	PR_CFO_Value = (s32)((s64)(div64_s64(PR_CFO_Value*u32SR, 134217728)));
	pr_info(">>> pr cfo only = %d [KHz] <<<\n", PR_CFO_Value);
	center_Freq_Offset_total += PR_CFO_Value;
	pr_info(">>> Total CFO = %d [KHz] <<<\n", center_Freq_Offset_total);

	*cfo = center_Freq_Offset_total;
	return 0;
}

static int intern_dvbs_get_fec_type(u8 *fec_type)
{
	u8 u8Data;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FEC_TYPE, &u8Data);
	*fec_type = u8Data;
	return 0;
}

static int intern_dvbs2_set_isid(u8 isid)
{
	u8 VCM_OPT = 0, total_is_id_count = 0, i = 0, j = 0, u8Data = 0;
	u32 current_time = 0;

	// assign IS-ID
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_IS_ID, isid);
	// wait for VCM_OPT == 1 or time out
	current_time = mtk_demod_get_time();
	do {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_VCM_OPT, &VCM_OPT);
	} while (VCM_OPT != 1 && (mtk_demod_get_time() - current_time) < 100);

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

static void intern_dvbs2_get_isid_table(u8 *isid_table)
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

static int intern_dvbs2_vcm_check(void)
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

static int intern_dvbs2_vcm_mode(enum DMD_DVBS_VCM_OPT u8VCM_OPT)
{
	u8VCM_Enabled_Opt = u8VCM_OPT;
	// Change VCM mode
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_VCM_OPT, u8VCM_Enabled_Opt);

	return 0;
}

int intern_dvbs_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	u8 u8Data = 0; //MS_U8 u8Data2 =0;
	int bRet = 0;
	u16 fTSDivNum = 0;
	u8  phase_tuning_num = 0;
	u8  pre_phase_tuning_enable = 0;
	u16 lockingtime;
	u32 u32CurrScanTime = 0;
	u16 timeout = 0;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

	bRet |= mtk_demod_mbx_dvb_read((TOP_REG_BASE + 0x60*2),
		&u8Data);
	if ((u8Data&0x02) == 0x00) {
		bRet |= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG,
			&u8Data);
		pr_info(">>>INTERN_DVBS_GetLock MailBox state=%d<<<\n", u8Data);
		if (u8Data > 6)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;

		if ((u8Data == 15) || (u8Data == 16)) {
			if (u8Data == 15) {
				_bDemodType = 0;   //S
				pr_info(">>>Demod DVBS Lock<<<\n");
			} else if (u8Data == 16) {
				_bDemodType = 1;//S2
				pr_info(">>>Demod DVBS2 Lock<<<\n");
			}
			if (u8DemodLockFlag == 0) {
				u8DemodLockFlag = 1;
				mtk_demod_mbx_dvb_read(FRONTEND_INFO_02, &u8Data);
				lockingtime = u8Data;
				mtk_demod_mbx_dvb_read(FRONTEND_INFO_01, &u8Data);
				lockingtime = ((lockingtime<<u16_highbyte_to_u8)|u8Data);
				pr_info("Demod Locking time [%d]\n", lockingtime);

				intern_dvbs_get_ts_divnum(&fTSDivNum);

				if (fTSDivNum > 0x1F)
					fTSDivNum = 0x1F;
				else if (fTSDivNum < 0x01)
					fTSDivNum = 0x01;

				u8Data = (u8)fTSDivNum;
				pr_info(">>>fTSDivNum=%d<<<\n", u8Data);
				cur_TS_CLK_DIV = u8Data;
				u8Data = readb(demod_prvi->virt_ts_base_addr);
				pre_TS_CLK_DIV = u8Data;
				u8Data = readb(demod_prvi->virt_ts_base_addr + 0x01);
				pre_phase_tuning_enable = u8Data&0x40;
				if ((pre_TS_CLK_DIV != cur_TS_CLK_DIV) ||
					(pre_phase_tuning_enable != 0x40)) {
					u8Data = readb(demod_prvi->virt_ts_base_addr + 0x04);
					writeb(u8Data | 0x01, demod_prvi->virt_ts_base_addr + 0x04);
					phase_tuning_num = cur_TS_CLK_DIV>>1;
					writeb(phase_tuning_num,
						demod_prvi->virt_ts_base_addr + 0x15);
					u8Data = readb(demod_prvi->virt_ts_base_addr + 0x01);
					u8Data |= 0x40;//enable phase tuning
					u8Data &= ~(0x08);//disable prolarity
					writeb(u8Data, demod_prvi->virt_ts_base_addr + 0x01);
					writeb(cur_TS_CLK_DIV, demod_prvi->virt_ts_base_addr);
					u8Data = readb(demod_prvi->virt_ts_base_addr);
					pre_TS_CLK_DIV = u8Data;
					u8Data = readb(demod_prvi->virt_ts_base_addr + 0x04);
					u8Data = (u8Data & ~(0x01));
					writeb(u8Data, demod_prvi->virt_ts_base_addr + 0x04);
				}
			}
			intern_dvbs_ts_enable(1);

			*status |= FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
			bRet = 0;
		} else {
			if (intern_dvbs_check_tr_ever_lock()) {
				*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
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
		bRet = -EINVAL;

	return bRet;
}

int intern_dvbs_get_frontend(struct dvb_frontend *fe, struct dtv_frontend_properties *p)
{
	u8  reg = 0;
	u8  reg2 = 0;
	u8  fec = 0;
	int ret = 0;
	u32 Symbol_Rate = 0;
	s32 cfo = 0;
	u16 ifagc = 0;
	u16 sqi = 0;
	s16 snr = 0;
	u32 post_ber = 0;
	u16 pkt_err = 0;
	u16 rssi = 0;
	u16 ssi = 0;
	u32 biterr_period = 0;
	u32 biterr = 0;
	u16 k_bch;
	u8 fec_type_idx = 0;
	u8 code_rate_idx = 0;
	u16 bch_array[2][42] = {
		{
			16200, 21600, 25920, 32400, 38880, 43200, 48600,
			51840, 54000, 57600, 58320, //S2 code rate
			14400, 18720, 29160, 32400, 34560, 35640, 36000,
			37440, 37440, 38880, 40320, 41400, 41760, 43200,
			44640, 45000, 46080, 46800, 47520, 47520, 48600,
			50400, 50400, 55440, 0, 0, 0, 0, 0, 0, 0
		},//S2X short FEC codr rate
		{
			3240, 5400, 6480, 7200, 9720, 10800, 11880, 12600,
			13320, 14400, 0,  //S2 code rate
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 3960, 4320, 5040, 7560, 8640, 9360, 11520
		} //S2X short FEC codr rate
	};
	struct dtv_fe_stats *stats;

	if (mtk_demod_read_byte(MB_REG_BASE + (0x0e)*2) != 0x01) {
		pr_err("[mdbgin_merak_demod_dd][%s][%d]Demod inactive !\n",
			__func__, __LINE__);
		return 0;
	}
	//Symbol rate
	intern_dvbs_get_symbol_rate(&Symbol_Rate);
	p->symbol_rate = Symbol_Rate;

	//Frequency offset
	intern_dvbs_get_freqoffset(&cfo);
	p->frequency_offset = cfo;

	//IFAGC
	intern_dvbs_get_ifagc(&ifagc);
	p->agc_val = (u8)(ifagc>>u16_highbyte_to_u8);

	//RSSI
	intern_dvbs_get_rf_level(ifagc, &rssi);

	//SSI
	intern_dvbs_get_signal_strength(rssi, &ssi);
	stats = &p->strength;
	stats->len = 1;
	stats->stat[0].uvalue = ssi;
	stats->stat[0].scale = FE_SCALE_RELATIVE;

	//SNR
	intern_dvbs_get_snr(&snr);
	stats = &p->cnr;
	stats->len = 1;
	stats->stat[0].svalue = (transfer_unit*(snr+half_CNR_unit)/CNR_unit);
	stats->stat[0].scale = FE_SCALE_DECIBEL;
	//error count
	intern_dvbs_get_pkterr(&pkt_err);
	stats = &p->block_error;
	stats->len = 1;
	stats->stat[0].uvalue = pkt_err;
	stats->stat[0].scale = FE_SCALE_COUNTER;
	//SQI
	intern_dvbs_get_signal_quality(&sqi);
	stats = &p->quality;
	stats->len = 1;
	stats->stat[0].uvalue = sqi;
	stats->stat[0].scale = FE_SCALE_RELATIVE;
	//System
	ret |= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &reg);
	pr_info("E_DMD_S2_SYSTEM_TYPE=%d\n", reg);
	if (!reg)
		p->delivery_system = SYS_DVBS2;
	else
		p->delivery_system = SYS_DVBS;

	//Modulation
	if (p->delivery_system == SYS_DVBS2) {
		ret |= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MOD_TYPE, &reg);
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
		intern_dvbs_get_code_rate(p);

		//Pilot
		ret |= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_PILOT_FLAG, &reg);
		if (reg == 1)
			p->pilot = PILOT_ON;
		else
			p->pilot = PILOT_OFF;

		pr_info("Pilot = %d\n", reg);
		//FEC Type
		ret |= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FEC_TYPE, &fec);
		if (fec == 1)
			p->fec_type = FEC_SHORT;
		else
			p->fec_type = FEC_LONG;

		//Rolloff
		ret |= mtk_demod_mbx_dvb_read(DVBS2OPPRO_ROLLOFF_DET_DONE, &reg);
		pr_info("DVBS2OPPRO_ROLLOFF_DET_DONE = %d\n", reg);
		if (reg) {
			ret |= mtk_demod_mbx_dvb_read(DVBS2OPPRO_ROLLOFF_DET_VALUE,
				&reg2);
			reg2 = (reg2 & 0x70) >> 4;
			switch (reg2) {
			case 0:
				p->rolloff = ROLLOFF_35;
				pr_info("ROLLOFF_35\n");
				break;
			case 1:
				p->rolloff = ROLLOFF_25;
				pr_info("ROLLOFF_25\n");
				break;
			case 2:
				p->rolloff = ROLLOFF_20;
				pr_info("ROLLOFF_20\n");
				break;
			case 3:
				p->rolloff = ROLLOFF_15;
				pr_info("ROLLOFF_15\n");
				break;
			case 4:
				p->rolloff = ROLLOFF_10;
				pr_info("ROLLOFF_10\n");
				break;
			case 5:
				p->rolloff = ROLLOFF_5;
				pr_info("ROLLOFF_5\n");
				break;
			}
		}
		//BER
		intern_dvbs_get_ber_window(&biterr_period, &biterr);
		ret |= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE, &code_rate_idx);
		ret |= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FEC_TYPE, &fec_type_idx);
		if (code_rate_idx > 41)
			code_rate_idx = 41;
		if (fec_type_idx > 1)
			fec_type_idx = 1;

		k_bch = bch_array[fec_type_idx][code_rate_idx];
		stats = &p->post_bit_error;
		stats->len = 1;
		stats->stat[0].uvalue = biterr;
		stats->stat[0].scale = FE_SCALE_COUNTER;
		stats = &p->post_bit_count;
		stats->len = 1;
		stats->stat[0].uvalue = (biterr_period*k_bch);
		stats->stat[0].scale = FE_SCALE_COUNTER;
		p->post_ber = biterr*BER_unit/(biterr_period*k_bch);

		if (intern_dvbs2_vcm_check()) {
			pr_info("VCM ENABLE\n");
			p->vcm_enable = 1;
			ret |= mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_IS_ID, &reg);
			// DVBS2 ISID
			p->stream_id = reg;

			// DVBS2 ISID array
			intern_dvbs2_get_isid_table(&(p->isid_array[0]));
		} else {
			pr_info("VCM DISABLE\n");
			p->vcm_enable = 0;
			p->stream_id = 0;
		}
	} else {
		p->modulation = QPSK;  //modulation
		p->pilot = PILOT_OFF;  //Pilot
		intern_dvbs_get_code_rate(p);
		p->rolloff = ROLLOFF_35;  //Rolloff

		//BER
		intern_dvbs_get_ber_window(&biterr_period, &biterr);
		stats = &p->post_bit_error;
		stats->len = 1;
		stats->stat[0].uvalue = biterr;
		stats->stat[0].scale = FE_SCALE_COUNTER;
		stats = &p->post_bit_count;
		stats->len = 1;
		stats->stat[0].uvalue = (biterr_period*dvbs_err_window);
		stats->stat[0].scale = FE_SCALE_COUNTER;

		p->post_ber = biterr*BER_unit/(biterr_period*dvbs_err_window);
	}
	return ret;
}

int intern_dvbs_BlindScan_Start(struct DMD_DVBS_BlindScan_Start_param *Start_param)
{
	u8 u8Data = 0;

	//DBG_intern_dvbs(printf("MDrv_Demod_BlindScan_Start+\n"));
	/*BEGIN: auto iq swap detection (2/7)*/
	iq_swap_checking_flag = FALSE;
	iq_swap_done_flag = FALSE;
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_6, 0);//disable force coarsecfo mechenism
	/*END: auto iq swap detection (2/7)*/
	_u16BlindScanStartFreq = Start_param->StartFreq;
	_u16BlindScanEndFreq = Start_param->EndFreq;
	_u16TunerCenterFreq = 0;
	_u16ChannelInfoIndex = 0;
	_u16LockedCenterFreq = 0;
	_u16PreCenterFreq = 0;
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_9,
		HAL_DRIVER_VERSION_KEY);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_A,
		HAL_DRIVER_VERSION);
	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	u8Data &= (BIT7|BIT6|BIT4);
	mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_TOP_WR_DBG_92, (u8)_u16BlindScanStartFreq&0x00ff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_TOP_WR_DBG_93,
		(u8)(_u16BlindScanStartFreq>>u16_highbyte_to_u8)&0x00ff);

	pr_info("Demod_BlindScan_Start _u16BlindScanStartFreq %d u16StartFreq %d u16EndFreq %d\n",
		_u16BlindScanStartFreq, _u16BlindScanStartFreq, _u16BlindScanEndFreq);
	return 0;
}

int intern_dvbs_BlindScan_NextFreq(struct DMD_DVBS_BlindScan_NextFreq_param *NextFreq_param)
{
	u8   u8Data = 0;

	pr_info("MDrv_Demod_BlindScan_NextFreq+\n");

	NextFreq_param->bBlindScanEnd = 0;

	/*BEGIN: auto iq swap detection (6/7)*/
	if ((iq_swap_checking_flag == TRUE) && (iq_swap_done_flag == FALSE)) {
		mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
		u8Data &= ~(BIT1);
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
		u8Data &= ~(BIT5|BIT3);
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
		return 0;
	}
	/*END: auto iq swap detection (6/7)*/

	if (_u16TunerCenterFreq >= _u16BlindScanEndFreq) {
		NextFreq_param->bBlindScanEnd = 1;
		return 0;
	}
	//Set Tuner Frequency
	mtk_demod_delay_ms(tuner_stable_delay);

	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	if ((u8Data & BIT1) == 0x00) {
		u8Data &= ~(BIT5|BIT3);
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
		u8Data |= BIT1;
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
		u8Data |= BIT0;
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
	} else {
		u8Data &= ~(BIT5|BIT3);
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
	}
	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	pr_info("TOP_REG_BASE = %d\n", u8Data);
	return 0;
}

int intern_dvbs_BlindScan_GetTunerFreq(
	struct DMD_DVBS_BlindScan_GetTunerFreq_param *GetTunerFreq_param)
{
	u8   u8Data = 0;
	u16  u16WaitCount;
	u16  u16TunerCutOff;

	//DBG_intern_dvbs(printf("MDrv_Demod_BlindScan_GetTunerFreq+\n"));

	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	if ((u8Data & BIT1) == BIT1) {
		u8Data |= BIT3;
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
		u16WaitCount = 0;
		do {
			mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG, &u8Data);//SubState
			u16WaitCount++;
			pr_info("MDrv_Demod_BlindScan_NextFreq u8Data:0x%x u16WaitCount:%d\n",
				u8Data, u16WaitCount);
			mtk_demod_delay_ms(1);
		} while ((u8Data != 0x01) && (u16WaitCount < INTERN_DVBS_DEMOD_WAIT_TIMEOUT));
	} else if ((u8Data & BIT0) == BIT0) {
		u8Data |= BIT5;
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
		u16WaitCount = 0;
		do {
			mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG, &u8Data);//SubState
			u16WaitCount++;
			mtk_demod_delay_ms(1);
		} while ((u8Data != 0x01) && (u16WaitCount < INTERN_DVBS_DEMOD_WAIT_TIMEOUT));
		mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
		u8Data |= BIT1;
		mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
	}

	_u16TunerCenterFreq = 0;
	/*START: auto iq swap detection (4/7)*/
	if ((iq_swap_checking_flag == TRUE) && (iq_swap_done_flag == FALSE))
		_u16TunerCenterFreq = cf_iq_swap_check;
	else {/*END: auto iq swap detection (4/7)*/
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_TOP_WR_DBG_93, &u8Data);
		_u16TunerCenterFreq = u8Data;
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_TOP_WR_DBG_92, &u8Data);
		_u16TunerCenterFreq = (_u16TunerCenterFreq<<u16_highbyte_to_u8)|u8Data;
	}

	GetTunerFreq_param->TunerCenterFreq = _u16TunerCenterFreq;

	/*START: auto iq swap detection (5/7)*/
	if ((iq_swap_checking_flag == TRUE) && (iq_swap_done_flag == FALSE))
		u16TunerCutOff = bw_iq_swap_check;

	else /*END: auto iq swap detection (5/7)*/
		u16TunerCutOff = blindscan_tuner_cutoff;

	GetTunerFreq_param->TunerCutOffFreq = u16TunerCutOff;

	pr_info("MDrv_Demod_BlindScan_GetTunerFreq- _u16TunerCenterFreq:%d  _u16TunerCutOff:%d\n",
		_u16TunerCenterFreq, u16TunerCutOff);
	return 0;
}

int intern_dvbs_BlindScan_WaitCurFreqFinished(
	struct DMD_DVBS_BlindScan_WaitCurFreqFinished_param *WaitCurFreqFinished_param)
{
	int status = 0;
	u32  u32Data = 0;
	u16  u16Data = 0;
	u8   u8Data = 0, u8Data2 = 0;
	u16  u16WaitCount;
	u8  state = 0;
	static s16 cfo_check_iq_swap;
	s32 freq_offset = 0;
	u8   CsdSel = 0, ACISel = 0;
	//DBG_intern_dvbs(printf("MDrv_Demod_BlindScan_WaitCurFreqFinished+\n"));
	u16WaitCount = 0;
	WaitCurFreqFinished_param->u8FindNum = 0;
	WaitCurFreqFinished_param->u8Progress = 0;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_DUMMY_REG_B, &u8Data);
	pr_info("E_DMD_S2_MB_DMDTOP_DBG_B[%d]\n", u8Data);
	/*BEGIN: auto iq swap detection (7/7)*/
	if ((iq_swap_checking_flag == TRUE) && (iq_swap_done_flag == FALSE)) {
		intern_dvbs_BlindScan_iq_swap_check(WaitCurFreqFinished_param);
		return 0; /*END: auto iq swap detection (7/7)*/
	}
	do {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG, &u8Data);
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_BLINDSCAN_CHECK, &u8Data2);
		u16WaitCount++;
		pr_info("WaitCurFreqFinished State: 0x%x Status: 0x%x u16WaitCount:%d\n",
			u8Data, u8Data2, u16WaitCount);
		mtk_demod_delay_ms(1);
	} while (((u8Data != DVBS_BLIND_SCAN) || (u8Data2 != Max_u8)) && (
		u16WaitCount < INTERN_DVBS_DEMOD_WAIT_TIMEOUT)); //E_DMD_S2_STATE_FLAG

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_DUMMY_REG_2, &u8Data);
	u16Data = u8Data;
	//printf("BlindScan_WaitCurFreqFinished OuterCheckStatus:0x%x\n", u16Data);
	if (u16WaitCount >= INTERN_DVBS_DEMOD_WAIT_TIMEOUT) {
		status = -1;
		pr_err("Debug blind scan wait finished time out!!!!\n");
	} else {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SUBSTATE_FLAG, &u8Data);//SubState
		if (u8Data == 0) {
			intern_dvbs_BlindScan_TP_lock();
			//switch to disable auto iq mechanism
			/*START: auto iq swap detection (3/7)*/
			if ((_u16ChannelInfoIndex == 0) && (iq_swap_checking_flag == FALSE)
				&& (iq_swap_done_flag == FALSE)) {
				//auto iq swap mechnism would run in first found TP
				iq_swap_checking_flag = TRUE;
				if (_s16CurrentCFO == 0) {
					if (_u16LockedSymbolRate >= auto_swap_force_cfo_criterion)
						cfo_check_iq_swap = auto_swap_force_cfo_large;
					else
						cfo_check_iq_swap = auto_swap_force_cfo_small;

					mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_6, 1);
					//enable force coarse cfo mechenism
					//configure force cfo value
					mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_7,
						(U16)((-1)*cfo_check_iq_swap)&0xFF);
					mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_8,
						(U16)(((-1)*cfo_check_iq_swap)>>u16_highbyte_to_u8
							)&0xFF);
					cf_iq_swap_check = (_u16LockedCenterFreq*transfer_unit
						+cfo_check_iq_swap)/transfer_unit;
				} else {
					mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_6, 1);
					//configure force cfo value
					mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_7, 0);
					mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_8, 0);
					cfo_check_iq_swap = _s16CurrentCFO;
					cf_iq_swap_check = _u16LockedCenterFreq;
				}
				bw_iq_swap_check = _u16LockedSymbolRate;
				u8Data = (bw_iq_swap_check&0xFF);
				mtk_demod_mbx_dvb_write_dsp(
					E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_L, u8Data);
				u8Data = ((bw_iq_swap_check>>u16_highbyte_to_u8)&0xFF);
				mtk_demod_mbx_dvb_write_dsp(
					E_DMD_S2_MANUAL_TUNE_SYMBOLRATE_H, u8Data);
				mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DMDTOP_DBG_5, 0);
				WaitCurFreqFinished_param->u8FindNum = 0;
				pr_info("cf_iq_swap_check[%d] cfo_check_iq_swap[%d]\n",
					cf_iq_swap_check, cfo_check_iq_swap);
				return 0;
			}
			pr_info(
				"Current Locked CF:%d BW:%d BWH:%d BWL:%d CFO(KHz):%d Step:%d\n",
				_u16LockedCenterFreq, _u16LockedSymbolRate, _u16PreLockedHB,
				_u16PreLockedLB, _s16CurrentCFO, _u16CurrentStepSize);
			_u16ChannelInfoIndex++;
			WaitCurFreqFinished_param->u8FindNum = _u16ChannelInfoIndex;
			/*END: auto iq swap detection (3/7)*/
		} else if (u8Data == 1) {
			status = intern_dvbs_BlindScan_TP_unlock();
			mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE16H, &u8Data);
			ACISel = u8Data;
			mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE16L, &u8Data);
			CsdSel = u8Data;

			WaitCurFreqFinished_param->u8FindNum = _u16ChannelInfoIndex;
			if (_u16NextCenterFreq > _u16TunerCenterFreq)
				_u16PreCenterFreq = _u16TunerCenterFreq;

			pr_info(
				"PreLocked CF:%d BW:%d NextCF:%d BW:%d CFO(KHz):%d CSD:%d ACI:%d\n",
				_u16LockedCenterFreq, _u16LockedSymbolRate,
				_u16NextCenterFreq, _u16CurrentSymbolRate, _s16CurrentCFO,
				CsdSel, ACISel);
		}
	}
	WaitCurFreqFinished_param->u8Progress = progress*(
		_u16TunerCenterFreq-_u16BlindScanStartFreq)/(
		_u16BlindScanEndFreq-_u16BlindScanStartFreq);
	if (WaitCurFreqFinished_param->u8Progress > progress)
		WaitCurFreqFinished_param->u8Progress = progress;

	//mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_DMDTOP_SWUSE00L, &u8Data);
	//DBG_intern_dvbs(printf("E_DMD_S2_MB_DMDTOP_SWUSE00L: %d\n", u8Data));
	//status &= MDrv_SYS_DMD_VD_MBX_ReadReg(DMDANA_REG_BASE+0xC0, &u8Data);
	mtk_demod_mbx_dvb_read(DMDTOP_ADC_IQ_SWAP, &u8Data);//Demod\reg_dmdana.xls
	pr_info("NOW IQ SWAP setting: %d\n", u8Data);
	return status;
}

int intern_dvbs_BlindScan_Cancel(void)
{
	u8   u8Data = 0;
	u16  u16Data;

	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	u8Data &= (BIT7|BIT6|BIT5|BIT4);
	mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);

	u16Data = 0x0000;
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_TOP_WR_DBG_92, (u8)u16Data&0x00ff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_TOP_WR_DBG_93,
		(u8)(u16Data>>u16_highbyte_to_u8)&0x00ff);

	_u16TunerCenterFreq = 0;
	_u16ChannelInfoIndex = 0;

	pr_info("MDrv_Demod_BlindScan_Cancel-\n");
	return 0;
}

int intern_dvbs_BlindScan_End(void)
{
	u8   u8Data = 0;
	u16  u16Data;

	pr_info("MDrv_Demod_BlindScan_End+\n");

	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	u8Data &= (BIT7|BIT6|BIT5|BIT4);
	mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
	u16Data = 0x0000;
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_TOP_WR_DBG_92, (u8)u16Data&0x00ff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_TOP_WR_DBG_93,
		(u8)(u16Data>>u16_highbyte_to_u8)&0x00ff);

	_u16TunerCenterFreq = 0;
	_u16ChannelInfoIndex = 0;

	return 0;
}

int intern_dvbs_BlindScan_GetChannel(struct DMD_DVBS_BlindScan_GetChannel_param *GetChannel_param)
{
	u16  u16TableIndex;

	if (_u16ChannelInfoIndex < (GetChannel_param->ReadStart)) {
		pr_info("BlindScan_GetChannel u16ReadStart is not correct!!!!\n");
		return FALSE;
	}

	(GetChannel_param->TPNum) = _u16ChannelInfoIndex-(GetChannel_param->ReadStart);
	pr_info("BlindScan_GetChannel TPNum: %d\n", GetChannel_param->TPNum);
	pr_info("BlindScan_GetChannel _u16ChannelInfoIndex: %d\n", _u16ChannelInfoIndex);

	for (u16TableIndex = 0; u16TableIndex < GetChannel_param->TPNum; u16TableIndex++) {
		pr_info("BlindScan_GetChannel Frequency: %d\n",
			GetChannel_param->pTable[u16TableIndex].Frequency);
		GetChannel_param->pTable[u16TableIndex].Frequency = (
			_u16ChannelInfoArray[0][_u16ChannelInfoIndex-1]);
		GetChannel_param->pTable[u16TableIndex].SymbolRate = (
			_u16ChannelInfoArray[1][_u16ChannelInfoIndex-1]);
		pr_info("BlindScan_GetChannel Freq: %lu SymbolRate: %lu\n",
			GetChannel_param->pTable[u16TableIndex].Frequency,
			GetChannel_param->pTable[u16TableIndex].SymbolRate);
	}
	return 0;
}

int intern_dvbs_blindscan_config(struct dvb_frontend *fe)
{
	u8 u8Data = 0;
	u8 u8Data1 = 0;
	u16 u16counter = 0;
	u32 u32CurrentSR;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	pr_info("[%s] is called\n", __func__);
	u32CurrentSR = c->symbol_rate/transfer_unit;///1000;  //KHz

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
		(u32CurrentSR>>u16_highbyte_to_u8)&0xff);
	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DMDTOP_DBG_5,
		(u32CurrentSR>>u32_lowbyte_to_u8)&0xff);
	pr_info(">>>SymbolRate %d<<<\n", u32CurrentSR);

	if (c->inversion) {
		mtk_demod_mbx_dvb_read(DMDTOP_ADC_IQ_SWAP, &u8Data);
		u8Data |= (BIT1);
		mtk_demod_mbx_dvb_write(DMDTOP_ADC_IQ_SWAP, u8Data);
	}

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_TS_SERIAL, 0x00);

	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	u8Data = u8Data|BIT3;
	mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);

	pr_info("INTERN_DVBS_config TOP_60=%d\n", u8Data);
	u16counter = INTERN_DVBS_DEMOD_WAIT_TIMEOUT;
	while (((u8Data&0x01) == 0x00) && (u16counter != 0)) {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG, &u8Data);
		u16counter--;
		mtk_demod_delay_ms(1);
	}

	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	u8Data &= ~(BIT1);
	mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	u8Data &= ~(BIT5|BIT3);
	mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);

	u32ChkScanTimeStartDVBS = mtk_demod_get_time();
	return 0;
}

int intern_dvbs_BlindScan_iq_swap_check(
	struct DMD_DVBS_BlindScan_WaitCurFreqFinished_param *WaitCurFreqFinished_param)
{
	u16 u16Data = 0;
	u32 u32Data = 0;
	u8  u8Data = 0;
	u16  u16WaitCount = 0;
	u8  state = 0;
	s32 freq_offset = 0;

	iq_swap_checking_flag = FALSE;
	iq_swap_done_flag = TRUE;
	do {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG, &state);//State=BlindScan
		u16WaitCount++;
		mtk_demod_delay_ms(1);//udelay(1*1000);//MsOS_DelayTask(1);
	} while (((_bDemodType && (state < DVBS2_EQ_STATE)) || (
		(_bDemodType == FALSE) && (state != DVBS_IDLE_STATE))) && (
		u16WaitCount < auto_iq_swap_timeout));

	mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_DUMMY_REG_6, 0); //disable force coarse cfo

	if ((u16WaitCount >= auto_iq_swap_timeout) /*Time Out*/
		|| (((_s16CurrentCFO != 0) && (abs(freq_offset) > abs(cfo_check_iq_swap)))
		|| ((_s16CurrentCFO == 0) && (freq_offset < 0)))
		|| ((cf_iq_swap_check < _u16PreCenterFreq) && (_u16PreCenterFreq != 0))) {
		//need to do IQ SWAP
		mtk_demod_mbx_dvb_read(DMDTOP_ADC_IQ_SWAP, &u8Data);
		if ((u8Data&BIT1) == BIT1)
			u8Data &= ~(BIT1);
		else
			u8Data |= (BIT1);

		mtk_demod_mbx_dvb_write(DMDTOP_ADC_IQ_SWAP, u8Data);
		//DBG_intern_dvbs(printf("Do IQ swap setting= 0x%x\n",u8Data));

		mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_TOP_WR_DBG_92,
			(U8)_u16BlindScanStartFreq&0x00ff);
		mtk_demod_mbx_dvb_write_dsp(E_DMD_S2_MB_TOP_WR_DBG_93,
			(U8)(_u16BlindScanStartFreq>>u16_highbyte_to_u8)&0x00ff);
		_u16ChannelInfoIndex = 0;
		_u16LockedCenterFreq = 0;
		_u16LockedSymbolRate = 0;
		//printf("Do IQ SWAP, next frequency is from start freq);
	} else {
		//1. no need to do iq swap
		//2. update *u8FindNum and _u16ChannelInfoIndex to report 1st tp is correct
		_u16ChannelInfoIndex++;
		WaitCurFreqFinished_param->u8FindNum = _u16ChannelInfoIndex;
	}
	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	u8Data |= BIT5;//Make state machine stay at AGC()
	mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
	u16WaitCount = 0;
	do {
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG, &state);//State=BlindScan
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SUBSTATE_FLAG, &u8Data);//SubState
		u16WaitCount++;
		pr_info("state[%d] u16WaitCount[%d]\n", state, u16WaitCount);
		mtk_demod_delay_ms(1);//udelay(1*1000);//MsOS_DelayTask(1);
	} while ((state != 1) && (u8Data != 0) && (u16WaitCount < state_reset_timeout));
	mtk_demod_mbx_dvb_read(TOP_WR_DBG_90, &u8Data);
	u8Data |= BIT1;//for blindscan flow
	mtk_demod_mbx_dvb_write(TOP_WR_DBG_90, u8Data);
	mtk_demod_delay_ms(1);//udelay(1*1000);//MsOS_DelayTask(1);
	return 0;
}

int intern_dvbs_BlindScan_TP_lock(void)
{
	u16 u16Data = 0;
	u32 u32Data = 0;
	u8  u8Data = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE13L, &u8Data);
	u32Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE12H, &u8Data);
	u32Data = (u32Data<<u16_highbyte_to_u8)|u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE12L, &u8Data);
	u32Data = (u32Data<<u16_highbyte_to_u8)|u8Data;
	_u16ChannelInfoArray[0][_u16ChannelInfoIndex] = ((u32Data+half_transfer_unit)
								/transfer_unit);
	_u16LockedCenterFreq = ((u32Data+half_transfer_unit)/transfer_unit);//Center Freq
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE14H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE14L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;
	_u16ChannelInfoArray[1][_u16ChannelInfoIndex] = (u16Data);//SR
	_u16LockedSymbolRate = u16Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE15H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE15L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;//Center_Freq_Offset_Locked
	if (u16Data >= u16_sign_criteria)
		_s16CurrentCFO = (-1)*(u16Data&0x7fff);
	else
		_s16CurrentCFO = u16Data;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE16H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE16L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;
	_u16CurrentStepSize = u16Data;            //Tuner_Frequency_Step

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE18H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE18L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;
	_u16PreLockedHB = u16Data;                //Pre_Scanned_HB

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE19H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE19L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;
	_u16PreLockedLB = u16Data;                //Pre_Scanned_LB

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &u8Data);
	if (u8Data == 0) // DVBS2
		_bDemodType = TRUE;
	else
		_bDemodType = FALSE;

	return 0;
}

int intern_dvbs_BlindScan_TP_unlock(void)
{
	u16 u16Data = 0;
	u32 u32Data = 0;
	u8  u8Data = 0;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_TOP_WR_DBG_93, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_TOP_WR_DBG_92, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;
	_u16NextCenterFreq = u16Data;

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE12H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE12L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;
	_u16PreLockedHB = u16Data;            //Pre_Scanned_HB

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE13H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE13L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;
	_u16PreLockedLB = u16Data;            //Pre_Scanned_LB

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE14H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE14L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;
	_u16CurrentSymbolRate = u16Data;        //Fine_Symbol_Rate

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE15H, &u8Data);
	u16Data = u8Data;
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MB_SWUSE15L, &u8Data);
	u16Data = (u16Data<<u16_highbyte_to_u8)|u8Data;        //Center_Freq_Offset
	if (u16Data >= u16_sign_criteria)
		_s16CurrentCFO = (-1)*(u16Data&0x7fff);
	else
		_s16CurrentCFO = u16Data;

	return 0;
}

int intern_dvbs_diseqc_set_tone(enum fe_sec_mini_cmd minicmd)
{
	u8   u8Data = 0;
	u16  u16WaitCount = 0;
	u8   mask = 0;
	u32  address = 0;

	address = DVBS2_DISEQC_TX_RAM_ADDR;
	mtk_demod_write_byte(address, BIT0);
	address = DVBS2_DISEQC_EN;
	mtk_demod_write_byte(address, (BIT6|BIT3|BIT2|BIT1));
	address = DVBS2_DISEQC_FCAR;
	mtk_demod_write_byte(address, (BIT7|BIT3));
	address = DVBS2_DISEQC_FIQ_CLEAR_FRONTEND1;
	mtk_demod_write_byte(address, (BIT7|BIT3));
	address = DVBS2_DISEQC_MOD;
	if (minicmd == SEC_MINI_B) {
		u8Data = (BIT4|BIT3|BIT0);
		toneBurstFlag = 1;
	} else {
		address = DVBS2_DISEQC_MOD;
		u8Data = (BIT4|BIT0);
		toneBurstFlag = 0;
	}
	mtk_demod_write_byte(address, u8Data);

	pr_info("[%s] toneBurst %d\n", __func__, toneBurstFlag);

	address = DVBS2_DISEQC_STATUS_CLEAR;
	mask = (BIT5|BIT4|BIT3|BIT2|BIT1);
	u8Data = mtk_demod_read_byte(address);
	u8Data |= mask;
	mtk_demod_write_byte(address, u8Data);
	mtk_demod_delay_ms(1);
	u8Data = mtk_demod_read_byte(address);
	u8Data &= ~(mask);
	mtk_demod_write_byte(address, u8Data);

	address = DVBS2_DISEQC_FIQ_CLEAR_FRONTEND0;
	u8Data = mtk_demod_read_byte(address);
	u8Data |= BIT7;
	mtk_demod_write_byte(address, u8Data);
	address = DVBS2_DISEQC_STATUS_CLEAR;
	u8Data = mtk_demod_read_byte(address);
	u8Data |= BIT0;
	mtk_demod_write_byte(address, u8Data);

	do {
		address = DVBS2_DISEQC_STATUS_CLEAR;
		u8Data = mtk_demod_read_byte(address);
		mtk_demod_delay_ms(1);
		u16WaitCount++;
	} while (((u8Data&BIT0) == BIT0) &&
		(u16WaitCount < INTERN_DVBS_DEMOD_WAIT_TIMEOUT));

	return 0;
}

int intern_dvbs_diseqc_set_22k(enum fe_sec_tone_mode tone)
{
	u8   u8Data = 0;
	u8   mask = 0;
	u32  address = 0;

	address = DVBS2_DISEQC_MOD;
	mask = (BIT5|BIT4|BIT3);
	u8Data = mtk_demod_read_byte(address);
	u8Data &= (~mask);
	if (tone == SEC_TONE_ON) {
		u8Data |= BIT3;
		tone22k_Flag = 1;
	} else {
		tone22k_Flag = 0;
	}
	pr_info("[%s] tone22k_Flag %d\n", __func__, tone22k_Flag);

	mtk_demod_write_byte(address, u8Data);
	return 0;
}


int intern_dvbs_diseqc_send_cmd(struct dvb_diseqc_master_cmd *cmd)
{
	u8   u8Data = 0, mask = 0;
	u8   u8Index = 0;
	u32  address = 0;
	u16  u16WaitCount = 0;
	u8 DiSEqC_Init_Mode = 0;
	static u8 Tx_Len;

	//Write_Reg(DVBS2_DISEQC_EN,0x01, 6, 6);
	address = DVBS2_DISEQC_EN;
	u8Data = mtk_demod_read_byte(address);
	u8Data |= BIT6;
	mtk_demod_write_byte(address, u8Data);

	//Write_Reg(DVBS2_DISEQC_EN,0x01, 3, 3);
	address = DVBS2_DISEQC_EN;
	u8Data = mtk_demod_read_byte(address);
	u8Data |= BIT3;
	mtk_demod_write_byte(address, u8Data);


	//REG_BASE[DVBS2_DISEQC_REPLY_TIMEOUT0]=0x00;
	address = DVBS2_DISEQC_REPLY_TIMEOUT0;
	u8Data = 0;
	mtk_demod_write_byte(address, u8Data);

	//REG_BASE[DVBS2_DISEQC_REPLY_TIMEOUT1]=0x02;
	address = DVBS2_DISEQC_REPLY_TIMEOUT0;
	u8Data = BIT1;
	mtk_demod_write_byte(address, u8Data);

	//DiSEqC_Init_Mode=REG_BASE[DVBS2_DISEQC_MOD];
	address = DVBS2_DISEQC_MOD;
	u8Data = mtk_demod_read_byte(address);
	DiSEqC_Init_Mode = u8Data;

	//Write_Reg(DVBS2_DISEQC_MOD,0x00, 7, 7);//Rx disable
	address = DVBS2_DISEQC_MOD;
	u8Data = mtk_demod_read_byte(address);
	u8Data &= ~((u8)BIT7);
	mtk_demod_write_byte(address, u8Data);

	address = DVBS2_DISEQC_MOD;
	//Write_Reg(DVBS2_DISEQC_MOD,0x04, 5, 3);//Data mode
	u8Data = mtk_demod_read_byte(address);
	mask = (BIT5|BIT4|BIT4);
	u8Data &= (~mask);
	u8Data |= BIT5;
	mtk_demod_write_byte(address, u8Data);


	//Odd_Enable_Mode=(gDvbsParam[E_S2_MB_DMDTOP_DBG_8]&0x40);
	//Write_Reg(DVBS2_DISEQC_EN,0x01, 2, 2);
	address = DVBS2_DISEQC_EN;
	u8Data = mtk_demod_read_byte(address);
	u8Data |= BIT2;
	mtk_demod_write_byte(address, u8Data);

	//Write_Reg(DVBS2_DISEQC_STATUS_CLEAR, 0x1f, 5, 1);
	//TimeDelayms(1);
	//Write_Reg(DVBS2_DISEQC_STATUS_CLEAR, 0x00, 5, 1);

	address = DVBS2_DISEQC_STATUS_CLEAR;
	u8Data = mtk_demod_read_byte(address);
	mask = (BIT5|BIT4|BIT3|BIT2|BIT1);
	u8Data |= mask;
	mtk_demod_write_byte(address, u8Data);
	//TimeDelayms(1);
	mtk_demod_delay_ms(1);
	address = DVBS2_DISEQC_STATUS_CLEAR;
	u8Data = mtk_demod_read_byte(address);
	u8Data &= (~mask);
	mtk_demod_write_byte(address, u8Data);


	//Write_Reg(DVBS2_DISEQC_MOD, 0x00, 5, 3);
	address = DVBS2_DISEQC_MOD;
	u8Data = mtk_demod_read_byte(address);
	mask = (BIT5|BIT4|BIT3);
	u8Data &= (~mask);
	mtk_demod_write_byte(address, u8Data);


	//Write_Reg(DVBS2_DISEQC_CARRY_THRESHOLD, 0x62, 7, 0);
	address = DVBS2_DISEQC_CARRY_THRESHOLD;
	u8Data = (BIT6|BIT5|BIT1);
	mtk_demod_write_byte(address, u8Data);


	//Write_Reg(DVBS2_DISEQC_FCAR, 0x88, 7, 0);  // set fcar
	address = DVBS2_DISEQC_FCAR;
	u8Data = (BIT7|BIT3);
	mtk_demod_write_byte(address, u8Data);


	//Write_Reg(DVBS2_DISEQC_TX_NEW_MODE, 0x01, 7, 7); //set tx new mode
	address = DVBS2_DISEQC_TX_NEW_MODE;
	u8Data = mtk_demod_read_byte(address);
	u8Data |= BIT7;
	mtk_demod_write_byte(address, u8Data);


	//Write_Reg(DVBS2_DISEQC_FCAR2, 0x88, 7, 0);  // set fcar
	address = DVBS2_DISEQC_FCAR2;
	u8Data = (BIT7|BIT3);
	mtk_demod_write_byte(address, u8Data);

	//Tx_Len=(gDvbsParam[E_S2_MB_DMDTOP_DBG_8]&0x07);
	pr_info("msg_len %d\n", cmd->msg_len);
	Tx_Len = ((cmd->msg_len-1) & ~((u8)BIT7));
	//Write_Reg(DVBS2_DISEQC_TX_LENGTH, Tx_Len, 6, 0); //Set tx len
	address = DVBS2_DISEQC_TX_LENGTH;
	u8Data = mtk_demod_read_byte(address);
	mask = (u8)(~BIT7);
	u8Data &= (~mask);
	u8Data |= Tx_Len;
	mtk_demod_write_byte(address, u8Data);


	for (u8Index = 0; u8Index < cmd->msg_len; u8Index++) {
		pr_info("DiseqcData[%d] 0x%x ", u8Index, cmd->msg[u8Index]);
		//Write_Reg(DVBS2_DISEQC_TX_RAM_ADDR,0x00+i*0x01,6,0);
		address = DVBS2_DISEQC_TX_RAM_ADDR;
		u8Data = mtk_demod_read_byte(address);
		mask = (u8)(~BIT7);
		u8Data &= (~mask);
		u8Data |= u8Index;
		mtk_demod_write_byte(address, u8Data);


		//REG_BASE[DVBS2_DISEQC_TX_RAM_WDATA]=0x00+i*0x01;
		address = DVBS2_DISEQC_TX_RAM_WDATA;
		u8Data = cmd->msg[u8Index];
		mtk_demod_write_byte(address, u8Data);
		//Write_Reg(DVBS2_DISEQC_TX_RAM_WE, 0x01, 7, 7);
		//Write_Reg(DVBS2_DISEQC_TX_RAM_WE, 0x00, 7, 7);
		address = DVBS2_DISEQC_TX_RAM_ADDR;
		u8Data = mtk_demod_read_byte(address);
		u8Data |= BIT7;
		mtk_demod_write_byte(address, u8Data);
		u8Data &= (~BIT7);
		mtk_demod_write_byte(address, u8Data);

		//read cmd data for debug
		//address = DVBS2_DISEQC_TX_LENGTH;
		//u8Data = mtk_demod_read_byte(address);
		//u8Data |= BIT7;
		//mtk_demod_write_byte(address, u8Data);
		//u8Data &= (~BIT7);
		//mtk_demod_write_byte(address, u8Data);
		//address = DVBS2_DISEQC_TX_DATA;
		//u8Data = mtk_demod_read_byte(address);
		//pr_err("check__data[%d] 0x%x ", u8Index, u8Data);

	}

	address = DVBS2_DISEQC_CTRL;
	u8Data = 0; //0x11;  let glue layer to control
	mtk_demod_write_byte(address, u8Data);


	//Write_Reg(DVBS2_DISEQC_MOD, 0x04, 5, 3);//Data mode
	address = DVBS2_DISEQC_MOD;
	u8Data = mtk_demod_read_byte(address);
	u8Data |= BIT5;
	mtk_demod_write_byte(address, u8Data);


	address = DVBS2_DISEQC_STATUS_CLEAR;
	u8Data = mtk_demod_read_byte(address);
	mask = (BIT5|BIT4|BIT3|BIT2|BIT1);
	u8Data |= mask;
	mtk_demod_write_byte(address, u8Data);
	//TimeDelayms(1);
	mtk_demod_delay_ms(1);
	address = DVBS2_DISEQC_STATUS_CLEAR;
	u8Data = mtk_demod_read_byte(address);
	u8Data &= (~mask);
	mtk_demod_write_byte(address, u8Data);


	//Write_Reg(DVBS2_DISEQC_TX_EN,0x01, 0, 0);//Tx enable
	address = DVBS2_DISEQC_STATUS_CLEAR;
	u8Data = mtk_demod_read_byte(address);
	u8Data |= BIT0;
	mtk_demod_write_byte(address, u8Data);


	u16WaitCount = 0;
	do {
		address = DVBS2_DISEQC_STATUS_CLEAR;
		u8Data = mtk_demod_read_byte(address);
		mtk_demod_delay_ms(1);
		//pr_info("status 0x%x\n", u8Data);
		u16WaitCount++;
	} while (((u8Data&BIT0) == BIT0) &&
		(u16WaitCount < INTERN_DVBS_DEMOD_WAIT_TIMEOUT));

	//Write_Reg(DVBS2_DISEQC_TX_EN,0x00, 0, 0);//Tx disable
	address = DVBS2_DISEQC_STATUS_CLEAR;
	u8Data = mtk_demod_read_byte(address);
	u8Data &= (~BIT0);
	mtk_demod_write_byte(address, u8Data);

	//Write_Reg(DVBS2_DISEQC_MOD,0x00, 7, 7);//Rx disable
	//REG_BASE[DVBS2_DISEQC_MOD]=DiSEqC_Init_Mode;
	address = DVBS2_DISEQC_MOD;
	mtk_demod_write_byte(address, DiSEqC_Init_Mode);

	if (u16WaitCount >= INTERN_DVBS_DEMOD_WAIT_TIMEOUT) {
		pr_err("INTERN_DVBS_Customized_DiSEqC_SendCmd Busy!!!\n");
		return -ETIMEDOUT;
	}

	return 0;
}


ssize_t demod_hal_dvbs_get_information(struct device_driver *driver, char *buf)
{
	u8 sys_type = 0, reg = 0, pilot = 0, lock_status = 0, fec = 0, iq_inv = 0;
	enum fe_modulation  mod_type = QPSK;
	enum fe_code_rate  cr_type = FEC_1_2;
	enum fe_rolloff  ro_type = ROLLOFF_35;
	u16 demod_rev = 0, ifagc = 0, rssi = 0, ssi = 0, sqi = 0, pkt_err = 0;
	s16 snr = 0;
	u32 symbol_rate = 0, post_ber = 0;
	s32 cfo = 0;
	u64 ts_rate = 0;
	u32 biterr_period = 0, biterr = 0;
	u16 k_bch;
	u8 fec_type_idx = 0;
	u8 code_rate_idx = 0;
	u16 bch_array[2][42] = {
		{
			16200, 21600, 25920, 32400, 38880, 43200, 48600,
			51840, 54000, 57600, 58320, /*S2 code rate*/
			14400, 18720, 29160, 32400, 34560, 35640, 36000,
			37440, 37440, 38880, 40320, 41400, 41760, 43200,
			44640, 45000, 46080, 46800, 47520, 47520, 48600,
			50400, 50400, 55440, 0, 0, 0, 0, 0, 0, 0
		},/*S2X short FEC codr rate*/
		{
			3240, 5400, 6480, 7200, 9720, 10800, 11880, 12600,
			13320, 14400, 0,  /*S2 code rate*/
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 3960, 4320, 5040, 7560, 8640, 9360, 11520
		} /*S2X short FEC codr rate*/
	};

	int ret = 0;

	pr_info("[%s] is called\n", __func__);

	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_STATE_FLAG,
			&reg);
	if ((reg == 15) || (reg == 16))
		lock_status = 1;

	/* Demod Rev. */
	ret = intern_dvbs_version(&demod_rev);
	/* TS Rate */
	ret = intern_dvbs_get_ts_rate(&ts_rate);

	//Symbol rate
	ret = intern_dvbs_get_symbol_rate(&symbol_rate);

	//Frequency offset
	ret = intern_dvbs_get_freqoffset(&cfo);

	//IFAGC
	ret = intern_dvbs_get_ifagc(&ifagc);
	ifagc = (ifagc>>u16_highbyte_to_u8);
	//RSSI
	ret = intern_dvbs_get_rf_level(ifagc, &rssi);

	//SSI
	ret = intern_dvbs_get_signal_strength(rssi, &ssi);

	//SNR
	ret = intern_dvbs_get_snr(&snr);
	snr = (transfer_unit*(snr + half_CNR_unit)/CNR_unit);

	//error count
	ret = intern_dvbs_get_pkterr(&pkt_err);

	//SQI
	ret = intern_dvbs_get_signal_quality(&sqi);

	//System
	mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_SYSTEM_TYPE, &sys_type);
	//DVBS2
	if (sys_type == 0) {
	//Modulation
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_MOD_TYPE, &reg);
		switch (reg) {
		case 0:
			mod_type = QPSK;
			break;
		case 1:
			mod_type = PSK_8;
			break;
		case 2:
			mod_type = APSK_16;
			break;
		case 3:
			mod_type = APSK_32;
			break;
		case 4:
			mod_type = APSK_8;
			break;
		case 5:
			mod_type = APSK_8_8;
			break;
		case 6:
			mod_type = APSK_4_8_4_16;
			break;
		}
		// Code rate
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE, &reg);
		switch (reg) {
		case 0x00: {
			cr_type = FEC_1_4;
			break;
		}
		case 0x01: {
			cr_type = FEC_1_3;
			break;
		}
		case 0x02: {
			cr_type = FEC_2_5;
			break;
		}

		case 0x03: {
			cr_type = FEC_1_2;
			break;
		}

		case 0x04: {
			cr_type = FEC_3_5;
			break;
		}

		case 0x05: {
			cr_type = FEC_2_3;
			break;
		}

		case 0x06: {
			cr_type = FEC_3_4;
			break;
		}

		case 0x07: {
			cr_type = FEC_4_5;
			break;
		}

		case 0x08: {
			cr_type = FEC_5_6;
			break;
		}

		case 0x09: {
			cr_type = FEC_8_9;
			break;
		}

		case 0x0A: {
			cr_type = FEC_9_10;
			break;
		}

		case 0x0B: {
			cr_type = FEC_2_9;
			break;
		}

		case 0x0C: {
			cr_type = FEC_13_45;
			break;
		}

		case 0x0D: {
			cr_type = FEC_9_20;
			break;
		}

		case 0x0E: {
			cr_type = FEC_90_180;
			break;
		}

		case 0x0F: {
			cr_type = FEC_96_180;
			break;
		}

		case 0x10: {
			cr_type = FEC_11_20;
			break;
		}

		case 0x11: {
			cr_type = FEC_100_180;
			break;
		}

		case 0x12: {
			cr_type = FEC_104_180;
			break;
		}

		case 0x13: {
			cr_type = FEC_26_45_L;
			break;
		}

		case 0x14: {
			cr_type = FEC_18_30;
			break;
		}

		case 0x15: {
			cr_type = FEC_28_45;
			break;
		}

		case 0x16: {
			cr_type = FEC_23_36;
			break;
		}

		case 0x17: {
			cr_type = FEC_116_180;
			break;
		}

		case 0x18: {
			cr_type = FEC_20_30;
			break;
		}

		case 0x19: {
			cr_type = FEC_124_180;
			break;
		}

		case 0x1A: {
			cr_type = FEC_25_36;
			break;
		}

		case 0x1B: {
			cr_type = FEC_128_180;
			break;
		}

		case 0x1C: {
			cr_type = FEC_13_18;
			break;
		}

		case 0x1D: {
			cr_type = FEC_132_180;
			break;
		}

		case 0x1E: {
			cr_type = FEC_22_30;
			break;
		}

		case 0x1F: {
			cr_type = FEC_135_180;
			break;
		}

		case 0x20: {
			cr_type = FEC_140_180;
			break;
		}

		case 0x21: {
			cr_type = FEC_7_9;
			break;
		}

		case 0x22: {
			cr_type = FEC_154_180;
			break;
		}

		case 0x23: {
			cr_type = FEC_11_45;
			break;
		}

		case 0x24: {
			cr_type = FEC_4_15;
			break;
		}

		case 0x25: {
			cr_type = FEC_14_45;
			break;
		}

		case 0x26: {
			cr_type = FEC_7_15;
			break;
		}

		case 0x27: {
			cr_type = FEC_8_15;
			break;
		}

		case 0x28: {
			cr_type = FEC_26_45_S;
			break;
		}

		case 0x29: {
			cr_type = FEC_32_45;
			break;
		}

		default:
			cr_type = FEC_1_2;
			break;
		}

		//Pilot
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_PILOT_FLAG, &pilot);

		//FEC Type
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FEC_TYPE, &fec);

		//Rolloff
		mtk_demod_mbx_dvb_read(DVBS2OPPRO_ROLLOFF_DET_DONE, &reg);
		if (reg) {
			mtk_demod_mbx_dvb_read(DVBS2OPPRO_ROLLOFF_DET_VALUE,
				&reg);
			reg = (reg & 0x70) >> 4;
			switch (reg) {
			case 0:
				ro_type = ROLLOFF_35;
				break;
			case 1:
				ro_type = ROLLOFF_25;
				break;
			case 2:
				ro_type = ROLLOFF_20;
				break;
			case 3:
				ro_type = ROLLOFF_15;
				break;
			case 4:
				ro_type = ROLLOFF_10;
				break;
			case 5:
				ro_type = ROLLOFF_5;
				break;
			}
		}
		//BER
		ret = intern_dvbs_get_ber_window(&biterr_period, &biterr);
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE, &code_rate_idx);
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_FEC_TYPE, &fec_type_idx);
		k_bch = bch_array[fec_type_idx][code_rate_idx];
		post_ber = biterr*BER_unit/(biterr_period*k_bch);

		//IQ invert
		mtk_demod_mbx_dvb_read(FRONTEND_MIXER_IQ_SWAP_OUT, &reg);
		if ((reg&0x02) == 0x02)
			iq_inv = 1;
		else
			iq_inv = 0;
	} else {
	//DVBS
		mod_type = QPSK;  //modulation
		pilot = 0;  //Pilot
		fec = 0;  //Fec
		// Code rate
		mtk_demod_mbx_dvb_read_dsp(E_DMD_S2_CODE_RATE, &reg);
		switch (reg) {
		case 0x00:
			cr_type = FEC_1_2;
			break;
		case 0x01:
			cr_type = FEC_2_3;
			break;
		case 0x02:
			cr_type = FEC_3_4;
			break;
		case 0x03:
			cr_type = FEC_5_6;
			break;
		case 0x04:
			cr_type = FEC_7_8;
			break;
		default:
			cr_type = FEC_7_8;
			break;
		}
		ro_type = ROLLOFF_35;  //Rolloff

		//BER
		ret = intern_dvbs_get_ber_window(&biterr_period, &biterr);
		post_ber = biterr*BER_unit/(biterr_period*dvbs_err_window);

		//IQ invert
		mtk_demod_mbx_dvb_read(DVBSFEC_IQ_SWAP_FLAG, &reg);
		if ((reg&0x02) == 0x02)
			iq_inv = 1;
		else
			iq_inv = 0;
	}

	pr_info("Demod Type = %d\n", (u8)sys_type);
	pr_info("Modulation Type = %d\n", (u8)mod_type);
	pr_info("Code rate = %d\n", (u8)cr_type);
	pr_info("Pilot = %d\n", pilot);
	pr_info("FEC = %d\n", fec);
	pr_info("Rolloff = %d\n", (u8)ro_type);
	pr_info("IQ inverse= %d\n", iq_inv);
	pr_info("Tone burst = %d\n", toneBurstFlag);
	pr_info("22K tone = %d\n", tone22k_Flag);

	return scnprintf(buf, PAGE_SIZE,
		"%s%s%x%s %s%s%llu%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s %s%s%d%s",
		SM_CHIP_REV, SM_COMMA, demod_rev, SM_END,
		SM_TS_RATE, SM_COMMA, ts_rate, SM_END,
		SM_SYSTEM, SM_COMMA, sys_type, SM_END,
		SM_MODULATION_TYPE, SM_COMMA, (u8)mod_type, SM_END,
		SM_SYMBOL_RATE, SM_COMMA, symbol_rate, SM_END,
		SM_FREQ_OFFSET, SM_COMMA, cfo, SM_END,
		SM_DEMOD_LOCK, SM_COMMA, lock_status, SM_END,
		SM_TS_LOCK, SM_COMMA, lock_status, SM_END,
		SM_CNR, SM_COMMA, snr, SM_END,
		SM_C_N, SM_COMMA, snr, SM_END,
		SM_BER_BEFORE_RS_BCH_5S, SM_COMMA, post_ber, SM_END,
		SM_BER_BEFORE_RS_BCH, SM_COMMA, post_ber, SM_END,
		SD_POST_LDPC, SM_COMMA, post_ber, SM_END,
		SD_POST_VITERBI, SM_COMMA, post_ber, SM_END,
		SM_UEC, SM_COMMA, pkt_err, SM_END,
		SM_CODE_RATE, SM_COMMA, (u8)cr_type, SM_END,
		SM_PILOT, SM_COMMA, pilot, SM_END,
		SM_22K, SM_COMMA, tone22k_Flag, SM_END,
		SM_SELECTED_DISEQC, SM_COMMA, toneBurstFlag, SM_END,
		SD_SIGNAL_QAULITY, SM_COMMA, sqi, SM_END,
		SM_RSSI, SM_COMMA, rssi, SM_END,
		SD_SIGNAL_LEVEL, SM_COMMA, ssi, SM_END,
		SM_AGC, SM_COMMA, ifagc, SM_END,
		SM_SPECTRUM_INVERT_FLAG, SM_COMMA, iq_inv, SM_END);
}
