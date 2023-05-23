/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRVDMD_VIF_H_
#define _DRVDMD_VIF_H_

#ifdef __cplusplus
extern "C"
{
#endif

//TODO, to solve re-define enum
#include "m7332_demod_common.h"

//-----------------------------------------------
// Public attributes.
//-----------------------------------------------

#ifndef DLL_PACKED
#define DLL_PACKED
#endif

#define EQ_COEF_LEN (131*7)

enum vif_sound_sys {
	VIF_SOUND_B,				// B_STEREO_A2
	VIF_SOUND_B_NICAM,		  // B_MONO_NICAM
	VIF_SOUND_GH,			   // GH_STEREO_A2
	VIF_SOUND_GH_NICAM,		 // GH_MONO_NICAM
	VIF_SOUND_I,
	VIF_SOUND_DK1,			  // DK1_STEREO_A2
	VIF_SOUND_DK2,			  // DK2_STEREO_A2
	VIF_SOUND_DK3,			  // DK3_STEREO_A2
	VIF_SOUND_DK_NICAM,		 // DK_MONO_NICAM
	VIF_SOUND_L,
	VIF_SOUND_LL,
	VIF_SOUND_MN,
	VIF_SOUND_NUMS
};

enum if_freq_type {
	IF_FREQ_3395, // SECAM-L'
	IF_FREQ_3800, // PAL
	IF_FREQ_3890, // PAL
	IF_FREQ_3950, // only for PAL-I
	IF_FREQ_4575, // NTSC-M/N
	IF_FREQ_5875, // NTSC-M/N
	IF_FREQ_NUMS
};

enum vif_freq_band {
	FREQ_VHF_L,
	FREQ_VHF_H,
	FREQ_UHF,
	FREQ_RANGE_NUMS
};

struct DLL_PACKED vif_initial_in {
	u8 vif_top;
	u8 vif_if_base_freq;
	u8 vif_tuner_step_size;
	u8 vif_saw_arch;
	u16 vif_vga_max;
	u16 vif_vga_min;
	u16 gain_distribute_thr;
	u8 vif_agc_vga_base;
	u8 vif_agc_vga_offs;
	u8 vif_agc_ref_negative;
	u8 vif_agc_ref_positive;
	u8 vif_dagc1_ref;
	u8 vif_dagc2_ref;
	u16 vif_dagc1_gain_ov;
	u16 vif_dagc2_gain_ov;
	u8 vif_cr_kf1;
	u8 vif_cr_kp1;
	u8 vif_cr_ki1;
	u8 vif_cr_kp2;
	u8 vif_cr_ki2;
	u8 vif_cr_kp;
	u8 vif_cr_ki;
	u16 vif_cr_lock_thr;
	u16 vif_cr_thr;
	u32 vif_cr_lock_num;
	u32 vif_cr_unlock_num;
	u16 vif_cr_pd_err_max;
	u8 vif_cr_lock_leaky_sel;
	u8 vif_cr_pd_x2;
	u8 vif_cr_lpf_sel;
	u8 vif_cr_pd_mode_sel;
	u8 vif_cr_kpki_adjust;
	u8 vif_cr_kpki_adjust_gear;
	u8 vif_cr_kpki_adjust_thr1;
	u8 vif_cr_kpki_adjust_thr2;
	u8 vif_cr_kpki_adjust_thr3;
	u8 vif_dynamic_top_adjust;
	u8 vif_dynamic_top_min;
	u8 vif_am_hum_detection;
	u8 clampgain_clamp_sel;
	u8 clampgain_syncbott_ref;
	u8 clampgain_syncheight_ref;
	u8 clampgain_kc;
	u8 clampgain_kg;
	u8 clampgain_clamp_oren;
	u8 clampgain_gain_oren;
	u16 clampgain_clamp_ov_negative;
	u16 clampgain_gain_ov_negative;
	u16 clampgain_clamp_ov_positive;
	u16 clampgain_gain_ov_positive;
	u8 clampgain_clamp_min;
	u8 clampgain_clamp_max;
	u8 clampgain_gain_min;
	u8 clampgain_gain_max;
	u16 clampgain_porch_cnt;

	u8 china_descramble_box;
	u8 vif_delay_reduce;
	u8 vif_overmodulation;
	u8 vif_overmodulation_detect;
	u8 vif_aci_detect;
	u8 vif_serious_aci_detect;
	u8 vif_aci_agc_ref;
	u8 vif_adc_overflow_agc_ref;
	u8 vif_channel_scan_agc_ref;
	u8 vif_aci_det_thr1;
	u8 vif_aci_det_thr2;
	u8 vif_aci_det_thr3;
	u8 vif_aci_det_thr4;
	u8 vif_rf_band;

	u8 vif_tuner_type;
	u32 vif_cr_rate_b;
	u8 vif_cr_inv_b;
	u32 vif_cr_rate_gh;
	u8 vif_cr_inv_gh;
	u32 vif_cr_rate_dk;
	u8 vif_cr_inv_dk;
	u32 vif_cr_rate_i;
	u8 vif_cr_inv_i;
	u32 vif_cr_rate_l;
	u8 vif_cr_inv_l;
	u32 vif_cr_rate_ll;
	u8 vif_cr_inv_ll;
	u32 vif_cr_rate_mn;
	u8 vif_cr_inv_mn;
	u8 vif_reserve;
};



enum VIF_USER_FILTER_SELECT {
	PK_START = 1,
	PK_B_VHF_L = PK_START,  //PeakingFilterB_VHF_L,
	PK_GH_VHF_L, //PeakingFilterGH_VHF_L,
	PK_DK_VHF_L, //PeakingFilterDK_VHF_L,
	PK_I_VHF_L,  //PeakingFilterI_VHF_L,
	PK_L_VHF_L,  //PeakingFilterL_VHF_L,
	PK_LL_VHF_L, //PeakingFilterLL_VHF_L,
	PK_MN_VHF_L, //PeakingFilterMN_VHF_L,
	PK_B_VHF_H,  //PeakingFilterB_VHF_H,
	PK_GH_VHF_H, //PeakingFilterGH_VHF_H,
	PK_DK_VHF_H, //PeakingFilterDK_VHF_H,
	PK_I_VHF_H,  //PeakingFilterI_VHF_H,
	PK_L_VHF_H,  //PeakingFilterL_VHF_H,
	PK_LL_VHF_H, //PeakingFilterLL_VHF_H,
	PK_MN_VHF_H, //PeakingFilterMN_VHF_H,
	PK_B_UHF,  //PeakingFilterB_UHF,
	PK_GH_UHF, //PeakingFilterGH_UHF,
	PK_DK_UHF, //PeakingFilterDK_UHF,
	PK_I_UHF,  //PeakingFilterI_UHF,
	PK_L_UHF,  //PeakingFilterL_UHF,
	PK_LL_UHF, //PeakingFilterLL_UHF,
	PK_MN_UHF, //PeakingFilterMN_UHF,
	PK_END = PK_MN_UHF,

	YC_START,
	YC_B_VHF_L = YC_START,  //YcDelayFilterB_VHF_L,
	YC_GH_VHF_L, //YcDelayFilterGH_VHF_L,
	YC_DK_VHF_L, //YcDelayFilterDK_VHF_L,
	YC_I_VHF_L,  //YcDelayFilterI_VHF_L,
	YC_L_VHF_L,  //YcDelayFilterL_VHF_L,
	YC_LL_VHF_L, //YcDelayFilterLL_VHF_L,
	YC_MN_VHF_L, //YcDelayFilterMN_VHF_L,
	YC_B_VHF_H,  //YcDelayFilterB_VHF_H,
	YC_GH_VHF_H, //YcDelayFilterGH_VHF_H,
	YC_DK_VHF_H, //YcDelayFilterDK_VHF_H,
	YC_I_VHF_H,  //YcDelayFilterI_VHF_H,
	YC_L_VHF_H,  //YcDelayFilterL_VHF_H,
	YC_LL_VHF_H, //YcDelayFilterLL_VHF_H,
	YC_MN_VHF_H, //YcDelayFilterMN_VHF_H,
	YC_B_UHF,  //YcDelayFilterB_UHF,
	YC_GH_UHF, //YcDelayFilterGH_UHF,
	YC_DK_UHF, //YcDelayFilterDK_UHF,
	YC_I_UHF,  //YcDelayFilterI_UHF,
	YC_L_UHF,  //YcDelayFilterL_UHF,
	YC_LL_UHF, //YcDelayFilterLL_UHF,
	YC_MN_UHF, //YcDelayFilterMN_UHF,
	YC_END = YC_MN_UHF,

	GP_START,
	GP_B_VHF_L = GP_START,  //GroupDelayFilterB_VHF_L,
	GP_GH_VHF_L, //GroupDelayFilterGH_VHF_L,
	GP_DK_VHF_L, //GroupDelayFilterDK_VHF_L,
	GP_I_VHF_L,  //GroupDelayFilterI_VHF_L,
	GP_L_VHF_L,  //GroupDelayFilterL_VHF_L,
	GP_LL_VHF_L, //GroupDelayFilterLL_VHF_L,
	GP_MN_VHF_L, //GroupDelayFilterMN_VHF_L,
	GP_B_VHF_H,  //GroupDelayFilterB_VHF_H,
	GP_GH_VHF_H, //GroupDelayFilterGH_VHF_H,
	GP_DK_VHF_H, //GroupDelayFilterDK_VHF_H,
	GP_I_VHF_H,  //GroupDelayFilterI_VHF_H,
	GP_L_VHF_H,  //GroupDelayFilterL_VHF_H,
	GP_LL_VHF_H, //GroupDelayFilterLL_VHF_H,
	GP_MN_VHF_H, //GroupDelayFilterMN_VHF_H,
	GP_B_UHF,  //GroupDelayFilterB_UHF,
	GP_GH_UHF, //GroupDelayFilterGH_UHF,
	GP_DK_UHF, //GroupDelayFilterDK_UHF,
	GP_I_UHF,  //GroupDelayFilterI_UHF,
	GP_L_UHF,  //GroupDelayFilterL_UHF,
	GP_LL_UHF, //GroupDelayFilterLL_UHF,
	GP_MN_UHF, //GroupDelayFilterMN_UHF,
	GP_END = GP_MN_UHF,

	VIF_USER_FILTER_SELECT_NUMS
};
/*
 *struct DLL_PACKED VIFUserFilter {};
 */
struct DLL_PACKED vif_notch_a1a2 {
	u16 vif_n_a1_c0;
	u16 vif_n_a1_c1;
	u16 vif_n_a1_c2;
	u16 vif_n_a2_c0;
	u16 vif_n_a2_c1;
	u16 vif_n_a2_c2;
};

struct DLL_PACKED vif_sos_1112 {
	u16 vif_sos_11_c0;
	u16 vif_sos_11_c1;
	u16 vif_sos_11_c2;
	u16 vif_sos_11_c3;
	u16 vif_sos_11_c4;
	u16 vif_sos_12_c0;
	u16 vif_sos_12_c1;
	u16 vif_sos_12_c2;
	u16 vif_sos_12_c3;
	u16 vif_sos_12_c4;
};
/*
 *struct DLL_PACKED VIFSOS33 {
 *	u16 Vif_SOS_33_C0;
 *	u16 Vif_SOS_33_C1;
 *	u16 Vif_SOS_33_C2;
 *	u16 Vif_SOS_33_C3;
 *	u16 Vif_SOS_33_C4;
 *};
 **/
/*
 *	vif_filter_coef is a large table of filter coefficients
 *	total: (56 + 3band * 5sos * 5) * 7standard
 *		= 131 * 7 [u16]
 *		= 917 [u16]
 *
 *	0~55:  EQ_C0 to C55
 *	56~80: band VHFL {
 *		56~60: SOS_21_C0 to C4
 *		61~65: SOS_22_C0 to C4
 *		66~70: SOS_31_C0 to C4
 *		71~75: SOS_32_C0 to C4
 *		76~80: SOS_33_C0 to C4
 *	}
 *	81~105: band VHFH {
 *		56~60: SOS_21_C0 to C4
 *		61~65: SOS_22_C0 to C4
 *		66~70: SOS_31_C0 to C4
 *		71~75: SOS_32_C0 to C4
 *		76~80: SOS_33_C0 to C4
 *	}
 *	106~130: band UHF {
 *		56~60: SOS_21_C0 to C4
 *		61~65: SOS_22_C0 to C4
 *		66~70: SOS_31_C0 to C4
 *		71~75: SOS_32_C0 to C4
 *		76~80: SOS_33_C0 to C4
 *	}
 *
 *	standards:
 *	B:  +0,
 *	GH: +131*1,
 *	DK: +131*2,
 *	I:  +131*3,
 *	L:  +131*4,
 *	LP: +131*5,
 *	MN: +131*6,
 */
/*
 *struct DLL_PACKED vif_filter_coef {};
 **/
struct DLL_PACKED vif_str_para
{
	u8 vif_clamp_l;
	u8 vif_clamp_h;
	u8 vif_clamp_gain_l;
	u8 vif_clamp_gain_h;

	u8 vif_cr_rate_1;
	u8 vif_cr_rate_2;
	u8 vif_cr_rate_3;
	u8 vif_cr_rate_inv;

	u8 vif_n_a1_c0_l;
	u8 vif_n_a1_c0_h;
	u8 vif_n_a1_c1_l;
	u8 vif_n_a1_c1_h;
	u8 vif_n_a1_c2_l;
	u8 vif_n_a1_c2_h;
	u8 vif_n_a2_c0_l;
	u8 vif_n_a2_c0_h;
	u8 vif_n_a2_c1_l;
	u8 vif_n_a2_c1_h;
	u8 vif_n_a2_c2_l;
	u8 vif_n_a2_c2_h;

	u8 vif_sos_11_c0_l;
	u8 vif_sos_11_c0_h;
	u8 vif_sos_11_c1_l;
	u8 vif_sos_11_c1_h;
	u8 vif_sos_11_c2_l;
	u8 vif_sos_11_c2_h;
	u8 vif_sos_11_c3_l;
	u8 vif_sos_11_c3_h;
	u8 vif_sos_11_c4_l;
	u8 vif_sos_11_c4_h;
	u8 vif_sos_12_c0_l;
	u8 vif_sos_12_c0_h;
	u8 vif_sos_12_c1_l;
	u8 vif_sos_12_c1_h;
	u8 vif_sos_12_c2_l;
	u8 vif_sos_12_c2_h;
	u8 vif_sos_12_c3_l;
	u8 vif_sos_12_c3_h;
	u8 vif_sos_12_c4_l;
	u8 vif_sos_12_c4_h;

	u8 vif_sos_21_c0_l;
	u8 vif_sos_21_c0_h;
	u8 vif_sos_21_c1_l;
	u8 vif_sos_21_c1_h;
	u8 vif_sos_21_c2_l;
	u8 vif_sos_21_c2_h;
	u8 vif_sos_21_c3_l;
	u8 vif_sos_21_c3_h;
	u8 vif_sos_21_c4_l;
	u8 vif_sos_21_c4_h;
	u8 vif_sos_22_c0_l;
	u8 vif_sos_22_c0_h;
	u8 vif_sos_22_c1_l;
	u8 vif_sos_22_c1_h;
	u8 vif_sos_22_c2_l;
	u8 vif_sos_22_c2_h;
	u8 vif_sos_22_c3_l;
	u8 vif_sos_22_c3_h;
	u8 vif_sos_22_c4_l;
	u8 vif_sos_22_c4_h;
	u8 vif_sos_31_c0_l;
	u8 vif_sos_31_c0_h;
	u8 vif_sos_31_c1_l;
	u8 vif_sos_31_c1_h;
	u8 vif_sos_31_c2_l;
	u8 vif_sos_31_c2_h;
	u8 vif_sos_31_c3_l;
	u8 vif_sos_31_c3_h;
	u8 vif_sos_31_c4_l;
	u8 vif_sos_31_c4_h;
	u8 vif_sos_32_c0_l;
	u8 vif_sos_32_c0_h;
	u8 vif_sos_32_c1_l;
	u8 vif_sos_32_c1_h;
	u8 vif_sos_32_c2_l;
	u8 vif_sos_32_c2_h;
	u8 vif_sos_32_c3_l;
	u8 vif_sos_32_c3_h;
	u8 vif_sos_32_c4_l;
	u8 vif_sos_32_c4_h;

	u8 vif_agc_ref;
	u8 vif_cr_k_sel;
	u8 vif_cr_kpki;
};

/*
 *  E_POWER_SUSPEND:    State is backup in DRAM, components power off.
 *  E_POWER_RESUME:     Resume from Suspend or Hibernate mode
 *  E_POWER_MECHANICAL: Full power off mode. System return to working state
 *      only after full reboot
 *  E_POWER_SOFT_OFF:   The system appears to be off, but some components
 *      remain powered for trigging boot-up.
 */
/*
 *enum EN_POWER_MODE {
 *	E_POWER_SUSPEND     = 1,
 *	E_POWER_RESUME      = 2,
 *	E_POWER_MECHANICAL  = 3,
 *	E_POWER_SOFT_OFF    = 4,
 *};
 **/

//-----------------------------------------------
// Public functions.
//-----------------------------------------------
//-----------------------------------------------
/// Check VIF version
/// @ingroup VIF_BASIC
/// @return TRUE : succeed
/// @return FALSE : fail
//-----------------------------------------------
//extern void DRV_VIF_Version(void);
//-----------------------------------------------
/// VIF Set Clock
/// @ingroup VIF_BASIC
/// @param  enable_b   \b IN: 0:means enable
/// @return TRUE : succeed
/// @return FALSE : fail
//-----------------------------------------------
//extern void DRV_VIF_SetClock(bool enable_b);
//-----------------------------------------------
/// Initialize VIF setting (any register access should be after this function)
/// @ingroup VIF_BASIC
/// @param  pVIF_InitData	 \b IN: init data
/// @param  u32InitDataLen  \b IN: init data size
/// @return TRUE : succeed
/// @return FALSE : fail
//-----------------------------------------------
//extern void drv_vif_init(vif_initial_in * pVIF_InitData, u32 u32InitDataLen);
extern void drv_vif_init(void);
//-----------------------------------------------
/// VIF Software Reset
/// @ingroup VIF_Basic
/// @return TRUE : succeed
/// @return FALSE : fail
//-----------------------------------------------
extern void drv_vif_reset(void);
//-----------------------------------------------
/// VIF Software Exit
/// @ingroup VIF_Basic
/// @return TRUE : succeed
/// @return FALSE : fail
//-----------------------------------------------
extern void drv_vif_exit(void);
//-----------------------------------------------
/// VIF Handler (monitor all VIF functions)
/// @ingroup VIF_Basic
/// @param  bAutoScan	 \b IN: init data
/// @return TRUE : succeed
/// @return FALSE : fail
//-----------------------------------------------
extern void drv_vif_set_scan(u8 bAutoScan);
//-----------------------------------------------
/// VIF Set Sound System
/// @ingroup VIF_Basic
/// @param  sound_sys	 \b IN: VIFSoundSystems
/// @return TRUE : succeed
/// @return FALSE : fail
//-----------------------------------------------
extern void drv_vif_set_sound_sys(enum vif_sound_sys sound_sys, u32 if_freq);
//-----------------------------------------------
/// VIF Set IF Frequnecy
/// @ingroup VIF_Basic
/// @param  if_freq	 \b IN: IF Frequency
/// @return TRUE : succeed
/// @return FALSE : fail
//-----------------------------------------------
//extern void DRV_VIF_SetIfFreq(enum if_freq_type if_freq);
//-----------------------------------------------
/// VIF Read CR FOE
/// @ingroup VIF_Basic
/// @return TRUE : CR_FOE
/// @return FALSE : 0, fail
//-----------------------------------------------
extern u8 drv_vif_read_cr_foe(void);
//-----------------------------------------------
/// VIF Read CR Lock Status
/// @ingroup VIF_Basic
/// @return TRUE : CR_LOCK_STATUS
/// @return FALSE : 0, fail
//-----------------------------------------------
extern u8 drv_vif_read_cr_lock_status(void);
//-----------------------------------------------
/// VIF Bypass DBB Audio Filter (A_DAGC_SEL)
/// @ingroup VIF_Task
/// @param  enable_b	 \b IN: enable_b
/// @return TRUE : 1, input from a_lpf_up
/// @return FALSE : 0, input from a_sos
//-----------------------------------------------
//extern void DRV_VIF_BypassDBBAudioFilter(bool enable_b);
//-----------------------------------------------
/// VIF Set Frquency Band
/// @ingroup VIF_Basic
/// @param  rf_band	 \b IN: frequency band
/// @return TRUE : vif_rf_band
/// @return FALSE : 0
//-----------------------------------------------
extern void drv_vif_set_freqband(enum vif_freq_band rf_band);
//-----------------------------------------------
/// VIF Get Input Level Indicator
/// @ingroup VIF_Task
/// @return TRUE : succeed
/// @return FALSE :  fail
//-----------------------------------------------
//extern bool DRV_VIF_GetInputLevelIndicator(void);
//-----------------------------------------------
/// VIF Set Parameters
/// @ingroup VIF_Task
/// @param  paraGroup	 \b IN: Parameters Group
/// @param  pVIF_Para	 \b IN: Parameters
/// @param  u32DataLen	 \b IN: Data Length
/// @return TRUE : succeed
/// @return FALSE :  fail
//-----------------------------------------------
//extern bool DRV_VIF_SetParameter(VIF_PARA_GROUP paraGroup,
//	void * pVIF_Para, u32 u32DataLen);
//-----------------------------------------------
/// VIF Set Shift Clock
/// @ingroup VIF_Task
/// @param  vif_shift_clk	 \b IN: 0 (42MHz, 140MHz),
///                             1(44.4MHz, 148MHz), 2(43.2MHz, 142MHz)
/// @return TRUE : succeed
/// @return FALSE :  fail
//-----------------------------------------------
//extern void DRV_VIF_ShiftClk(u8 vif_shift_clk);
//-----------------------------------------------
/// VIF Set Power State
/// @ingroup VIF_Task
/// @param  power_state	 \b IN: Power State
/// @return TRUE : succeed
/// @return FALSE :  fail
//-----------------------------------------------
//extern u32 drv_vif_set_power_state(enum EN_POWER_MODE power_state);
extern s32 drv_vif_set_power_state(enum EN_POWER_MODE power_state);
//-----------------------------------------------
/// VIF Write Byte
/// @ingroup VIF_Basic
/// @param  reg	 \b IN: Register address
/// @param  u8Val	 \b IN: Value
/// @return TRUE : succeed
/// @return FALSE :  fail
//-----------------------------------------------
extern void drv_vif_writebyte(u32 reg, u8 u8Val);
//-----------------------------------------------
/// VIF Read Byte
/// @ingroup VIF_Basic
/// @param  reg	 \b IN: Register address
/// @return TRUE : succeed
/// @return FALSE :  fail
//-----------------------------------------------
extern u8 drv_vif_readbyte(u32 reg);
//-----------------------------------------------
/// VIF Set FilterCoefficients
/// @ingroup VIF_Task
/// @param  p_vif_coef	 \b IN: Filter coefficients
/// @param  data_len	\b IN: Data Length
/// @return TRUE : succeed
/// @return FALSE :  fail
//-----------------------------------------------
//extern bool drv_vif_set_filter_coef(struct vif_filter_coef *p_vif_coef,
extern bool drv_vif_set_filter_coef(const u16 p_vif_coef[],
	u32 data_len);

extern s32 drv_vif_foe2hz(u8 foe, int tuner_quarter_step);
extern u8 drv_vif_read_statics(u8 *agc_num);

#ifdef __cplusplus
}
#endif

#endif
