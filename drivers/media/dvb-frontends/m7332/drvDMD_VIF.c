// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "drvDMD_VIF.h"
#include "halDMD_INTERN_VIF.h"
#include "regVIF.h"


//#define DRVVIFDBG(x)		  //x

//-----------------------------------------------
//  Local Variables
//-----------------------------------------------


struct vif_initial_in vif_init_inst;
struct vif_notch_a1a2 vif_n_a1a2;
struct vif_sos_1112 vif_s_1112;
enum vif_sound_sys g_vif_sound_sys;
struct vif_str_para vif_str_info;
u16 vif_coef[EQ_COEF_LEN] = {0};

bool is_atv;

const u16 vif_coef_default[EQ_COEF_LEN] = {
	#include "VIF_COEF_TABLE.dat"
};


//-----------------------------------------------
//  Local VIF functions
//-----------------------------------------------

void drv_vif_writebyte(u32 reg, u8 val)
{
	hal_vif_writebyte(reg, val);
}

u8 drv_vif_readbyte(u32 reg)
{
	return hal_vif_readbyte(reg);
}

s32 drv_vif_foe2hz(u8 foe, int tuner_quarter_step)
{
	int sign_i = 1;
	u8 tmp_foe = foe;
	s32 ret = 0;

	if ((foe & 0x80) != 0) {
		sign_i *= -1;
		tmp_foe = (~foe)+1;
	}

	ret = tmp_foe * tuner_quarter_step * sign_i;
	return ret;
}

u8 drv_vif_read_cr_foe(void)
{
	return hal_vif_read_foe();
}

u8 drv_vif_read_cr_lock_status(void)
{
	return hal_vif_read_lock_status();
}

u8 drv_vif_read_statics(u8 *agc_num)
{
	u8 tmp_value;

	mtk_demod_delay_ms(1);//prevent random unlock, ckang

	tmp_value = drv_vif_readbyte(DBB3_REG_BASE + 0x61);
	tmp_value = tmp_value ^ 0x80;
	*agc_num = tmp_value;
	return tmp_value;
}

void _get_init_setting(void)
{
	pr_info("[%s] is called\n", __func__);


	vif_init_inst.vif_top = 5;
	vif_init_inst.vif_if_base_freq = 0x02; // 38.9MHz
	vif_init_inst.vif_tuner_step_size = 0x02; // 62.5KHz
	vif_init_inst.vif_saw_arch = 2; // Silicon tuner
	vif_init_inst.vif_vga_max = 0x7000;
	vif_init_inst.vif_vga_min = 0x8000;

	//threshold was 0x6A00;
	vif_init_inst.gain_distribute_thr = (0x4000 - 0x600);
	vif_init_inst.vif_agc_vga_base = (0x4000 - 0x600)>>7; //0xD4;

	vif_init_inst.vif_agc_vga_offs = 0x40;
	vif_init_inst.vif_agc_ref_negative = 0x60;
	vif_init_inst.vif_agc_ref_positive = 0x19;

	vif_init_inst.vif_dagc1_ref = 0x26;
	vif_init_inst.vif_dagc2_ref = 0x26;
	vif_init_inst.vif_dagc1_gain_ov = 0x1000; //was 0x1800;
	vif_init_inst.vif_dagc2_gain_ov = 0x1000; //was 0x1800;

	vif_init_inst.vif_cr_kf1 = 0x03;
	vif_init_inst.vif_cr_kp1 = 0x03;
	vif_init_inst.vif_cr_ki1 = 0x03;
	vif_init_inst.vif_cr_kp2 = 0x06;
	vif_init_inst.vif_cr_ki2 = 0x05;
	vif_init_inst.vif_cr_kp = 0x06;
	vif_init_inst.vif_cr_ki = 0x09;
	vif_init_inst.vif_cr_lpf_sel = 1;
	vif_init_inst.vif_cr_pd_mode_sel = 0;
	vif_init_inst.vif_cr_kpki_adjust = 1;
	vif_init_inst.vif_cr_kpki_adjust_gear = 2;
	vif_init_inst.vif_cr_kpki_adjust_thr1 = 0x05;
	vif_init_inst.vif_cr_kpki_adjust_thr2 = 0x10;
	vif_init_inst.vif_cr_kpki_adjust_thr3 = 0x03;

	vif_init_inst.vif_cr_lock_thr = 0x0020;
	vif_init_inst.vif_cr_thr = 0x0500;
	vif_init_inst.vif_cr_lock_num = 0x00008000;
	vif_init_inst.vif_cr_unlock_num = 0x00004000;
	vif_init_inst.vif_cr_pd_err_max = 0x3FFF;
	vif_init_inst.vif_cr_lock_leaky_sel = 1;
	vif_init_inst.vif_cr_pd_x2 = 1;

	vif_init_inst.vif_dynamic_top_adjust = 0;
	vif_init_inst.vif_dynamic_top_min = 0;
	vif_init_inst.vif_am_hum_detection = 0;
	vif_init_inst.clampgain_clamp_sel = 1;
	vif_init_inst.clampgain_syncbott_ref = 0x78;
	vif_init_inst.clampgain_syncheight_ref = 0x68;
	vif_init_inst.clampgain_kc = 0x07;
	vif_init_inst.clampgain_kg = 0x07;
	vif_init_inst.clampgain_clamp_oren = 1;
	vif_init_inst.clampgain_gain_oren = 1;

	vif_init_inst.clampgain_clamp_ov_negative = 0x750;
	vif_init_inst.clampgain_gain_ov_negative = 0x5F0; //was 0x300;
	vif_init_inst.clampgain_clamp_ov_positive = 0x5F0;
	vif_init_inst.clampgain_gain_ov_positive = 0x4F0; //was 0x300;

	vif_init_inst.clampgain_clamp_min = 0x80;
	vif_init_inst.clampgain_clamp_max = 0x00;
	vif_init_inst.clampgain_gain_min = 0x40;
	vif_init_inst.clampgain_gain_max = 0xFF;
	vif_init_inst.clampgain_porch_cnt = 0x00C8;

	vif_init_inst.china_descramble_box = 0;
	vif_init_inst.vif_delay_reduce = 0;
	vif_init_inst.vif_overmodulation = 0;
	vif_init_inst.vif_overmodulation_detect = 0;
	vif_init_inst.vif_aci_detect = 0;
	vif_init_inst.vif_aci_agc_ref = 0x30;
	vif_init_inst.vif_adc_overflow_agc_ref = 0x20;
	vif_init_inst.vif_channel_scan_agc_ref = 0x20; //was 0x60;
	vif_init_inst.vif_serious_aci_detect = 0;
	vif_init_inst.vif_aci_det_thr1 = 0x16;
	vif_init_inst.vif_aci_det_thr2 = 0x0A;
	vif_init_inst.vif_aci_det_thr3 = 0x08;
	vif_init_inst.vif_aci_det_thr4 = 0x03;

	// should be changed when setting tuner freq
	vif_init_inst.vif_rf_band = 0; // FREQ_VHF_L
	// Tuner_type
	// 0: (outdated, don't use it), 1:Silicon tuner, 2: FM radio
	vif_init_inst.vif_tuner_type = 1;

	// TODO: Get tuner IF Video carrier position from tuner side
	// mtkTuner_MXL661.h
	{
		// B = 5+2.25M
		vif_init_inst.vif_cr_rate_b = 0x000ABDA0;
		vif_init_inst.vif_cr_inv_b = 0;
		// GH = 5+2.75M
		vif_init_inst.vif_cr_rate_gh = 0x000B7B42;
		vif_init_inst.vif_cr_inv_gh = 0;
		// DK = 5+2.75M
		vif_init_inst.vif_cr_rate_dk = 0x000B7B42;
		vif_init_inst.vif_cr_inv_dk = 0;
		// I = 5+2.75M
		vif_init_inst.vif_cr_rate_i = 0x000B7B42;
		vif_init_inst.vif_cr_inv_i = 0;
		// L = 5+2.75M
		vif_init_inst.vif_cr_rate_l = 0x000B7B42;
		vif_init_inst.vif_cr_inv_l = 0;
		// LP = 5+2.75M
		vif_init_inst.vif_cr_rate_ll = 0x000B7B42;
		vif_init_inst.vif_cr_inv_ll = 0;
		// MN = 5+1.75M
		vif_init_inst.vif_cr_rate_mn = 0x000A0000;
		vif_init_inst.vif_cr_inv_mn = 0;
	}

	vif_init_inst.vif_reserve = 0;

	memcpy(vif_coef, vif_coef_default, EQ_COEF_LEN);
}

//void drv_vif_init(vif_initial_in * pVIF_InitData, u32 u32InitDataLen)
void drv_vif_init(void)
{

	bool tmp_b = FALSE;

	pr_debug("%s()\n", __func__);


	// !! any register access should be after this function
	hal_vif_reg_init(&vif_init_inst, &vif_n_a1a2,
			&vif_s_1112, vif_coef);

	_get_init_setting();//TODO, use default init value for now
	is_atv = TRUE;



	hal_vif_adc_init();

	tmp_b = hal_vif_download();

	if (tmp_b == FALSE)
		pr_info("\n ===== DSP Load Code Fail =====\n");

	if (vif_init_inst.vif_if_base_freq == IF_FREQ_45_75)
		hal_vif_set_if_freq(IF_FREQ_4575);
	else if (vif_init_inst.vif_if_base_freq == IF_FREQ_38_00)
		hal_vif_set_if_freq(IF_FREQ_3800);
	else
		hal_vif_set_if_freq(IF_FREQ_3890);

	/* init default DK */
	hal_vif_set_sound_sys(VIF_SOUND_DK2, 0);
	hal_vif_top_adjust();

	pr_debug("coef_default[0] = %x\n", vif_coef_default[0]);

	drv_vif_set_filter_coef(vif_coef_default,
		sizeof(vif_coef_default));
}

void drv_vif_exit(void)
{
	hal_vif_exit();
	is_atv = FALSE;

	//bParaDownload = FALSE;
	//bFilterDownload = FALSE;
}

void drv_vif_reset(void)
{
	bool tmp_b = FALSE;

	tmp_b = hal_vif_download();

	if (tmp_b == FALSE)
		pr_info("\n ===== DSP Load Code Fail =====\n");
}

void drv_vif_set_scan(u8 bAutoScan)
{
	hal_vif_set_scan(bAutoScan);
	//bChannelScan = bAutoScan;
}

void drv_vif_set_sound_sys(enum vif_sound_sys sound_sys, u32 if_freq)
{
	pr_debug("%s sound_sys=%d\n", __func__, sound_sys);
	g_vif_sound_sys = sound_sys;
	hal_vif_set_sound_sys(sound_sys, if_freq);
}

void drv_vif_set_freqband(enum vif_freq_band rf_band)
{
	u8 u8tmp;

	pr_debug("%s() rf_band=%d\n", __func__, rf_band);
	vif_init_inst.vif_rf_band = rf_band;

	u8tmp = drv_vif_readbyte((RF_REG_BASE+0xD4));

	u8tmp = (u8tmp & (~0x0F)) | rf_band;

	drv_vif_writebyte((RF_REG_BASE+0xD4), u8tmp);

}

void _vif_get_str_para(void)
{

	vif_str_info.vif_clamp_l = drv_vif_readbyte(RF_REG_BASE+0x46);
	vif_str_info.vif_clamp_h = drv_vif_readbyte(RF_REG_BASE+0x47);
	vif_str_info.vif_clamp_gain_l = drv_vif_readbyte(RF_REG_BASE+0x48);
	vif_str_info.vif_clamp_gain_h = drv_vif_readbyte(RF_REG_BASE+0x49);

	vif_str_info.vif_cr_rate_1 = drv_vif_readbyte(CR_RATE);
	vif_str_info.vif_cr_rate_2 = drv_vif_readbyte(CR_RATE+1);
	vif_str_info.vif_cr_rate_3 = drv_vif_readbyte(CR_RATE+2);
	vif_str_info.vif_cr_rate_inv = drv_vif_readbyte(CR_RATE_INV);

	vif_str_info.vif_n_a1_c0_l = drv_vif_readbyte(N_A1_C0_L);
	vif_str_info.vif_n_a1_c0_h = drv_vif_readbyte(N_A1_C0_H);
	vif_str_info.vif_n_a1_c1_l = drv_vif_readbyte(N_A1_C1_L);
	vif_str_info.vif_n_a1_c1_h = drv_vif_readbyte(N_A1_C1_H);
	vif_str_info.vif_n_a1_c2_l = drv_vif_readbyte(N_A1_C2_L);
	vif_str_info.vif_n_a1_c2_h = drv_vif_readbyte(N_A1_C2_H);

	vif_str_info.vif_n_a2_c0_l = drv_vif_readbyte(N_A2_C0_L);
	vif_str_info.vif_n_a2_c0_h = drv_vif_readbyte(N_A2_C0_H);
	vif_str_info.vif_n_a2_c1_l = drv_vif_readbyte(N_A2_C1_L);
	vif_str_info.vif_n_a2_c1_h = drv_vif_readbyte(N_A2_C1_H);
	vif_str_info.vif_n_a2_c2_l = drv_vif_readbyte(N_A2_C2_L);
	vif_str_info.vif_n_a2_c2_h = drv_vif_readbyte(N_A2_C2_H);

	vif_str_info.vif_sos_11_c0_l = drv_vif_readbyte(SOS11_C0_L);
	vif_str_info.vif_sos_11_c0_h = drv_vif_readbyte(SOS11_C0_H);
	vif_str_info.vif_sos_11_c1_l = drv_vif_readbyte(SOS11_C1_L);
	vif_str_info.vif_sos_11_c1_h = drv_vif_readbyte(SOS11_C1_H);
	vif_str_info.vif_sos_11_c2_l = drv_vif_readbyte(SOS11_C2_L);
	vif_str_info.vif_sos_11_c2_h = drv_vif_readbyte(SOS11_C2_H);
	vif_str_info.vif_sos_11_c3_l = drv_vif_readbyte(SOS11_C3_L);
	vif_str_info.vif_sos_11_c3_h = drv_vif_readbyte(SOS11_C3_H);
	vif_str_info.vif_sos_11_c4_l = drv_vif_readbyte(SOS11_C4_L);
	vif_str_info.vif_sos_11_c4_h = drv_vif_readbyte(SOS11_C4_H);

	vif_str_info.vif_sos_12_c0_l = drv_vif_readbyte(SOS12_C0_L);
	vif_str_info.vif_sos_12_c0_h = drv_vif_readbyte(SOS12_C0_H);
	vif_str_info.vif_sos_12_c1_l = drv_vif_readbyte(SOS12_C1_L);
	vif_str_info.vif_sos_12_c1_h = drv_vif_readbyte(SOS12_C1_H);
	vif_str_info.vif_sos_12_c2_l = drv_vif_readbyte(SOS12_C2_L);
	vif_str_info.vif_sos_12_c2_h = drv_vif_readbyte(SOS12_C2_H);
	vif_str_info.vif_sos_12_c3_l = drv_vif_readbyte(SOS12_C3_L);
	vif_str_info.vif_sos_12_c3_h = drv_vif_readbyte(SOS12_C3_H);
	vif_str_info.vif_sos_12_c4_l = drv_vif_readbyte(SOS12_C4_L);
	vif_str_info.vif_sos_12_c4_h = drv_vif_readbyte(SOS12_C4_H);

	vif_str_info.vif_sos_21_c0_l = drv_vif_readbyte(SOS21_C0_L);
	vif_str_info.vif_sos_21_c0_h = drv_vif_readbyte(SOS21_C0_H);
	vif_str_info.vif_sos_21_c1_l = drv_vif_readbyte(SOS21_C1_L);
	vif_str_info.vif_sos_21_c1_h = drv_vif_readbyte(SOS21_C1_H);
	vif_str_info.vif_sos_21_c2_l = drv_vif_readbyte(SOS21_C2_L);
	vif_str_info.vif_sos_21_c2_h = drv_vif_readbyte(SOS21_C2_H);
	vif_str_info.vif_sos_21_c3_l = drv_vif_readbyte(SOS21_C3_L);
	vif_str_info.vif_sos_21_c3_h = drv_vif_readbyte(SOS21_C3_H);
	vif_str_info.vif_sos_21_c4_l = drv_vif_readbyte(SOS21_C4_L);
	vif_str_info.vif_sos_21_c4_h = drv_vif_readbyte(SOS21_C4_H);

	vif_str_info.vif_sos_22_c0_l = drv_vif_readbyte(SOS22_C0_L);
	vif_str_info.vif_sos_22_c0_h = drv_vif_readbyte(SOS22_C0_H);
	vif_str_info.vif_sos_22_c1_l = drv_vif_readbyte(SOS22_C1_L);
	vif_str_info.vif_sos_22_c1_h = drv_vif_readbyte(SOS22_C1_H);
	vif_str_info.vif_sos_22_c2_l = drv_vif_readbyte(SOS22_C2_L);
	vif_str_info.vif_sos_22_c2_h = drv_vif_readbyte(SOS22_C2_H);
	vif_str_info.vif_sos_22_c3_l = drv_vif_readbyte(SOS22_C3_L);
	vif_str_info.vif_sos_22_c3_h = drv_vif_readbyte(SOS22_C3_H);
	vif_str_info.vif_sos_22_c4_l = drv_vif_readbyte(SOS22_C4_L);
	vif_str_info.vif_sos_22_c4_h = drv_vif_readbyte(SOS22_C4_H);

	vif_str_info.vif_sos_31_c0_l = drv_vif_readbyte(SOS31_C0_L);
	vif_str_info.vif_sos_31_c0_h = drv_vif_readbyte(SOS31_C0_H);
	vif_str_info.vif_sos_31_c1_l = drv_vif_readbyte(SOS31_C1_L);
	vif_str_info.vif_sos_31_c1_h = drv_vif_readbyte(SOS31_C1_H);
	vif_str_info.vif_sos_31_c2_l = drv_vif_readbyte(SOS31_C2_L);
	vif_str_info.vif_sos_31_c2_h = drv_vif_readbyte(SOS31_C2_H);
	vif_str_info.vif_sos_31_c3_l = drv_vif_readbyte(SOS31_C3_L);
	vif_str_info.vif_sos_31_c3_h = drv_vif_readbyte(SOS31_C3_H);
	vif_str_info.vif_sos_31_c4_l = drv_vif_readbyte(SOS31_C4_L);
	vif_str_info.vif_sos_31_c4_h = drv_vif_readbyte(SOS31_C4_H);

	vif_str_info.vif_sos_32_c0_l = drv_vif_readbyte(SOS32_C0_L);
	vif_str_info.vif_sos_32_c0_h = drv_vif_readbyte(SOS32_C0_H);
	vif_str_info.vif_sos_32_c1_l = drv_vif_readbyte(SOS32_C1_L);
	vif_str_info.vif_sos_32_c1_h = drv_vif_readbyte(SOS32_C1_H);
	vif_str_info.vif_sos_32_c2_l = drv_vif_readbyte(SOS32_C2_L);
	vif_str_info.vif_sos_32_c2_h = drv_vif_readbyte(SOS32_C2_H);
	vif_str_info.vif_sos_32_c3_l = drv_vif_readbyte(SOS32_C3_L);
	vif_str_info.vif_sos_32_c3_h = drv_vif_readbyte(SOS32_C3_H);
	vif_str_info.vif_sos_32_c4_l = drv_vif_readbyte(SOS32_C4_L);
	vif_str_info.vif_sos_32_c4_h = drv_vif_readbyte(SOS32_C4_H);

	vif_str_info.vif_agc_ref = drv_vif_readbyte(AGC_REF);
	vif_str_info.vif_cr_k_sel = drv_vif_readbyte(CR_K_SEL);
	vif_str_info.vif_cr_kpki = drv_vif_readbyte(CR_KP_SW);
}

void _vif_set_str_para(void)
{
	drv_vif_writebyte((RF_REG_BASE+0x46), vif_str_info.vif_clamp_l);
	drv_vif_writebyte((RF_REG_BASE+0x47), vif_str_info.vif_clamp_h);
	drv_vif_writebyte((RF_REG_BASE+0x48), vif_str_info.vif_clamp_gain_l);
	drv_vif_writebyte((RF_REG_BASE+0x49), vif_str_info.vif_clamp_gain_h);

	drv_vif_writebyte(CR_RATE, vif_str_info.vif_cr_rate_1);
	drv_vif_writebyte(CR_RATE+1, vif_str_info.vif_cr_rate_2);
	drv_vif_writebyte(CR_RATE+2, vif_str_info.vif_cr_rate_3);
	drv_vif_writebyte(CR_RATE_INV, vif_str_info.vif_cr_rate_inv);

	drv_vif_writebyte(N_A1_C0_L, vif_str_info.vif_n_a1_c0_l);
	drv_vif_writebyte(N_A1_C0_H, vif_str_info.vif_n_a1_c0_h);
	drv_vif_writebyte(N_A1_C1_L, vif_str_info.vif_n_a1_c1_l);
	drv_vif_writebyte(N_A1_C1_H, vif_str_info.vif_n_a1_c1_h);
	drv_vif_writebyte(N_A1_C2_L, vif_str_info.vif_n_a1_c2_l);
	drv_vif_writebyte(N_A1_C2_H, vif_str_info.vif_n_a1_c2_h);

	drv_vif_writebyte(N_A2_C0_L, vif_str_info.vif_n_a2_c0_l);
	drv_vif_writebyte(N_A2_C0_H, vif_str_info.vif_n_a2_c0_h);
	drv_vif_writebyte(N_A2_C1_L, vif_str_info.vif_n_a2_c1_l);
	drv_vif_writebyte(N_A2_C1_H, vif_str_info.vif_n_a2_c1_h);
	drv_vif_writebyte(N_A2_C2_L, vif_str_info.vif_n_a2_c2_l);
	drv_vif_writebyte(N_A2_C2_H, vif_str_info.vif_n_a2_c2_h);

	drv_vif_writebyte(SOS11_C0_L, vif_str_info.vif_sos_11_c0_l);
	drv_vif_writebyte(SOS11_C0_H, vif_str_info.vif_sos_11_c0_h);
	drv_vif_writebyte(SOS11_C1_L, vif_str_info.vif_sos_11_c1_l);
	drv_vif_writebyte(SOS11_C1_H, vif_str_info.vif_sos_11_c1_h);
	drv_vif_writebyte(SOS11_C2_L, vif_str_info.vif_sos_11_c2_l);
	drv_vif_writebyte(SOS11_C2_H, vif_str_info.vif_sos_11_c2_h);
	drv_vif_writebyte(SOS11_C3_L, vif_str_info.vif_sos_11_c3_l);
	drv_vif_writebyte(SOS11_C3_H, vif_str_info.vif_sos_11_c3_h);
	drv_vif_writebyte(SOS11_C4_L, vif_str_info.vif_sos_11_c4_l);
	drv_vif_writebyte(SOS11_C4_H, vif_str_info.vif_sos_11_c4_h);

	drv_vif_writebyte(SOS12_C0_L, vif_str_info.vif_sos_12_c0_l);
	drv_vif_writebyte(SOS12_C0_H, vif_str_info.vif_sos_12_c0_h);
	drv_vif_writebyte(SOS12_C1_L, vif_str_info.vif_sos_12_c1_l);
	drv_vif_writebyte(SOS12_C1_H, vif_str_info.vif_sos_12_c1_h);
	drv_vif_writebyte(SOS12_C2_L, vif_str_info.vif_sos_12_c2_l);
	drv_vif_writebyte(SOS12_C2_H, vif_str_info.vif_sos_12_c2_h);
	drv_vif_writebyte(SOS12_C3_L, vif_str_info.vif_sos_12_c3_l);
	drv_vif_writebyte(SOS12_C3_H, vif_str_info.vif_sos_12_c3_h);
	drv_vif_writebyte(SOS12_C4_L, vif_str_info.vif_sos_12_c4_l);
	drv_vif_writebyte(SOS12_C4_H, vif_str_info.vif_sos_12_c4_h);

	drv_vif_writebyte(SOS21_C0_L, vif_str_info.vif_sos_21_c0_l);
	drv_vif_writebyte(SOS21_C0_H, vif_str_info.vif_sos_21_c0_h);
	drv_vif_writebyte(SOS21_C1_L, vif_str_info.vif_sos_21_c1_l);
	drv_vif_writebyte(SOS21_C1_H, vif_str_info.vif_sos_21_c1_h);
	drv_vif_writebyte(SOS21_C2_L, vif_str_info.vif_sos_21_c2_l);
	drv_vif_writebyte(SOS21_C2_H, vif_str_info.vif_sos_21_c2_h);
	drv_vif_writebyte(SOS21_C3_L, vif_str_info.vif_sos_21_c3_l);
	drv_vif_writebyte(SOS21_C3_H, vif_str_info.vif_sos_21_c3_h);
	drv_vif_writebyte(SOS21_C4_L, vif_str_info.vif_sos_21_c4_l);
	drv_vif_writebyte(SOS21_C4_H, vif_str_info.vif_sos_21_c4_h);

	drv_vif_writebyte(SOS22_C0_L, vif_str_info.vif_sos_22_c0_l);
	drv_vif_writebyte(SOS22_C0_H, vif_str_info.vif_sos_22_c0_h);
	drv_vif_writebyte(SOS22_C1_L, vif_str_info.vif_sos_22_c1_l);
	drv_vif_writebyte(SOS22_C1_H, vif_str_info.vif_sos_22_c1_h);
	drv_vif_writebyte(SOS22_C2_L, vif_str_info.vif_sos_22_c2_l);
	drv_vif_writebyte(SOS22_C2_H, vif_str_info.vif_sos_22_c2_h);
	drv_vif_writebyte(SOS22_C3_L, vif_str_info.vif_sos_22_c3_l);
	drv_vif_writebyte(SOS22_C3_H, vif_str_info.vif_sos_22_c3_h);
	drv_vif_writebyte(SOS22_C4_L, vif_str_info.vif_sos_22_c4_l);
	drv_vif_writebyte(SOS22_C4_H, vif_str_info.vif_sos_22_c4_h);

	drv_vif_writebyte(SOS31_C0_L, vif_str_info.vif_sos_31_c0_l);
	drv_vif_writebyte(SOS31_C0_H, vif_str_info.vif_sos_31_c0_h);
	drv_vif_writebyte(SOS31_C1_L, vif_str_info.vif_sos_31_c1_l);
	drv_vif_writebyte(SOS31_C1_H, vif_str_info.vif_sos_31_c1_h);
	drv_vif_writebyte(SOS31_C2_L, vif_str_info.vif_sos_31_c2_l);
	drv_vif_writebyte(SOS31_C2_H, vif_str_info.vif_sos_31_c2_h);
	drv_vif_writebyte(SOS31_C3_L, vif_str_info.vif_sos_31_c3_l);
	drv_vif_writebyte(SOS31_C3_H, vif_str_info.vif_sos_31_c3_h);
	drv_vif_writebyte(SOS31_C4_L, vif_str_info.vif_sos_31_c4_l);
	drv_vif_writebyte(SOS31_C4_H, vif_str_info.vif_sos_31_c4_h);

	drv_vif_writebyte(SOS32_C0_L, vif_str_info.vif_sos_32_c0_l);
	drv_vif_writebyte(SOS32_C0_H, vif_str_info.vif_sos_32_c0_h);
	drv_vif_writebyte(SOS32_C1_L, vif_str_info.vif_sos_32_c1_l);
	drv_vif_writebyte(SOS32_C1_H, vif_str_info.vif_sos_32_c1_h);
	drv_vif_writebyte(SOS32_C2_L, vif_str_info.vif_sos_32_c2_l);
	drv_vif_writebyte(SOS32_C2_H, vif_str_info.vif_sos_32_c2_h);
	drv_vif_writebyte(SOS32_C3_L, vif_str_info.vif_sos_32_c3_l);
	drv_vif_writebyte(SOS32_C3_H, vif_str_info.vif_sos_32_c3_h);
	drv_vif_writebyte(SOS32_C4_L, vif_str_info.vif_sos_32_c4_l);
	drv_vif_writebyte(SOS32_C4_H, vif_str_info.vif_sos_32_c4_h);

	drv_vif_writebyte(AGC_REF, vif_str_info.vif_agc_ref);
	drv_vif_writebyte(CR_K_SEL, vif_str_info.vif_cr_k_sel);
	drv_vif_writebyte(CR_KP_SW, vif_str_info.vif_cr_kpki);
}

//u32 drv_vif_set_power_state(enum EN_POWER_MODE power_state)
s32 drv_vif_set_power_state(enum EN_POWER_MODE power_state)
{
	u32 ret = 1;

	if (is_atv == TRUE) {
		if (power_state == E_POWER_RESUME) {

			drv_vif_init();

			drv_vif_set_filter_coef(vif_coef_default,
				sizeof(vif_coef_default));

			if ((g_vif_sound_sys == VIF_SOUND_L) ||
					(g_vif_sound_sys == VIF_SOUND_LL)) {
				drv_vif_set_sound_sys(g_vif_sound_sys, 0);
				drv_vif_reset();
			}

			drv_vif_set_freqband(vif_init_inst.vif_rf_band);

			drv_vif_set_sound_sys(g_vif_sound_sys, 0);

			_vif_set_str_para();

			ret = 0;
		} else if (power_state == E_POWER_SUSPEND) {
			_vif_get_str_para();

			drv_vif_exit();

			ret = 0;
		} else {
			pr_err("=== [%s] Unknown Power State ===\n", __func__);
			ret = 1;
		}
	} else {
		pr_info("\n ==== VIF don't Suspend/Resume at Non-ATV mode ====\n");
		ret = 0;
	}

	return ret;
}

//bool drv_vif_set_filter_coef(struct vif_filter_coef *p_vif_coef,
//	u32 data_len)
bool drv_vif_set_filter_coef(const u16 p_vif_coef[], u32 data_len)
{
	if (data_len == sizeof(vif_coef_default)) {
		memcpy(vif_coef, p_vif_coef, data_len);

		//pr_debug("coef_default[0] = %x\n", vif_coef_default[0]);
		//pr_debug("vif_coef[0] = %x\n", vif_coef[0]);

		return hal_vif_filter_coef_download();
	}
	// else {
		pr_info("[%s] ERROR!! Wrong size, copy fail", __func__);
		return FALSE;
	//}
/*
 *	if (bChannelScan == FALSE) {
 *		if (bFilterDownload == FALSE) {
 *			bFilterDownload = TRUE;
 *			return hal_vif_filter_coef_download();
 *		}
 *	}
 */
}

