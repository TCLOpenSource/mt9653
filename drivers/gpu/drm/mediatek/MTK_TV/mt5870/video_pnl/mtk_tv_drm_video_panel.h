/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_VIDEO_PANEL_H_
#define _MTK_TV_DRM_VIDEO_PANEL_H_

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <drm/mtk_tv_drm.h>

#include "../mtk_tv_drm_plane.h"
#include "../mtk_tv_drm_drv.h"
#include "../mtk_tv_drm_crtc.h"
#include "../mtk_tv_drm_connector.h"
#include "../mtk_tv_drm_video.h"
#include "../mtk_tv_drm_encoder.h"
#include "../mtk_tv_drm_kms.h"
#include "hwreg_render_video_pnl.h"

#include "chip_int.h"

/***********
 *MOD setting
 ************/
//define output mode
#define MOD_IN_IF_VIDEO (4)	//means 4p engine input
#define MOD_IN_IF_DeltaV (1)	//means 1p engine input
#define MOD_IN_IF_GRAPH (1)	//means 1p engine input
#define LVDS_MPLL_CLOCK_MHZ	(864)	// For crystal 24Mhz

#define W2BYTEMSK(addr, val, mask)
#define W4BYTE(args...)
#define W2BYTE(args...)

#define reg_ckg_xtal_24m_mod	(0x103534)
#define reg_ckg_lpll_syn_432	(0x103480)
#define reg_ckg_lpll_syn_864	(0x103484)
#define reg_ckg_mod_d_odclk	(0x10348C)

#define reg_sw_en_lpll_sync_4322fpll		(0x103C04)
#define reg_sw_en_lpll_sync_8642fpll		(0x103C06)
#define reg_sw_en_xtal_24m2fpll			(0x103C4C)
#define reg_sw_en_lpll_syn_864_pipe2fpll	(0x103BF8)
#define reg_sw_en_mod_d_odclk2mod_d		(0x103C0A)
#define reg_sw_en_mod_d_odclk2lvds		(0x103C0A)

#define reg_ckg_v1_odclk	(0x1034bc)	// 1p mod
#define reg_ckg_v1_odclk_stg1	(0x1034c0)	// 2p mod
#define reg_ckg_v1_odclk_stg2	(0x1034c4)	// 4p mod
#define reg_ckg_v1_odclk_stg3	(0x1034c8)	// 8p mft sram w
#define reg_ckg_v1_odclk_stg4	(0x1034cc)	// 8p mft sram r
#define reg_ckg_v1_odclk_stg5	(0x1034d0)	// 4p mft
#define reg_ckg_v1_odclk_stg6	(0x1034d4)	// 2p mft
#define reg_ckg_v1_odclk_stg7	(0x1034d8)	// 1p mft
#define reg_ckg_v1_odclk_stg8	(0x1034dc)	// vby1 16p
#define reg_ckg_v1_odclk_stg9	(0x10352c)	// vby1 32p

//sw en
#define reg_sw_en_v1_odclk2v1		(0x103C36)
#define reg_sw_en_v1_odclk_stg12v1	(0x103C26)
#define reg_sw_en_v1_odclk_stg22v1	(0x103C28)
#define reg_sw_en_v1_odclk_stg32v1	(0x103C2A)
#define reg_sw_en_v1_odclk_stg42v1	(0x103C2C)
#define reg_sw_en_v1_odclk_stg52v1	(0x103C2E)
#define reg_sw_en_v1_odclk_stg62v1	(0x103C30)
#define reg_sw_en_v1_odclk_stg72v1	(0x103C32)
#define reg_sw_en_v1_odclk_stg82v1	(0x103C34)
#define reg_sw_en_v1_odclk_stg92v1	(0x103C60)

#define reg_ckg_v2_odclk	(0x1034E4)	//DeltaV 1p mod
#define reg_ckg_v2_odclk_stg1	(0x1034E8)	//DeltaV 2p mod
#define reg_ckg_v2_odclk_stg2	(0x1034EC)	// DeltaV 4p mod
#define reg_ckg_v2_odclk_stg3	(0x1034F0)	// DeltaV 8p mft sram w
#define reg_ckg_v2_odclk_stg4	(0x1034F4)	// DeltaV 8p mfr sram r
#define reg_ckg_v2_odclk_stg5	(0x1034F8)	// mft 4p mod
#define reg_ckg_v2_odclk_stg6	(0x1034FC)	// mft 2p mod
#define reg_ckg_v2_odclk_stg7	(0x103500)	// mft 1p

#define reg_sw_en_v2_odclk2v2	(0x103C48)
#define reg_sw_en_v2_odclk_stg12v2	(0x103C3A)
#define reg_sw_en_v2_odclk_stg22v2	(0x103C3C)
#define reg_sw_en_v2_odclk_stg32v2	(0x103C3E)
#define reg_sw_en_v2_odclk_stg42v2	(0x103C40)
#define reg_sw_en_v2_odclk_stg52v2	(0x103C42)
#define reg_sw_en_v2_odclk_stg62v2	(0x103C44)
#define reg_sw_en_v2_odclk_stg72v2	(0x103C46)

#define reg_ckg_o_odclk		(0x103498)	// osd 1p mod
#define reg_ckg_o_odclk_stg1	(0x13049C)	// osd 2p mod
#define reg_ckg_o_odclk_stg2	(0x1034A0)	// osd 4p mod
#define reg_ckg_o_odclk_stg3	(0x1034A4)	// osd 8p mft w
#define reg_ckg_o_odclk_stg4	(0x1034A8)	// osd 8p mft r
#define reg_ckg_o_odclk_stg5	(0x1034AC)	// osd mft 4p mod
#define reg_ckg_o_odclk_stg6	(0x1034B0)	// osd mft 2p mod

#define reg_sw_en_o_odclk2o		(0x103C20)
#define reg_sw_en_o_odclk_stg12o	(0x103C14)
#define reg_sw_en_o_odclk_stg22o	(0x103C16)
#define reg_sw_en_o_odclk_stg32o	(0x103C18)
#define reg_sw_en_o_odclk_stg42o	(0x103C1A)
#define reg_sw_en_o_odclk_stg52o	(0x103C1C)
#define reg_sw_en_o_odclk_stg62o	(0x103C1E)

//serial top clk
#define reg_ckg_mod_v1_sr_wclk  (0x103490)	// vb1 64p to serial top w
#define reg_ckg_mod_v2_sr_wclk  (0x103492)	// vby 16p to serial top
#define reg_ckg_v1_mod_sr_rclk	(0x1034B8)
#define reg_ckg_v1_mod_sr_rclk_pre0	(0x103514)
#define reg_ckg_v1_mod_sr_rclk_pre1	(0x103518)
#define reg_ckg_v1_mod_sr_rclk_final	(0x103510)
#define reg_ckg_v2_mod_sr_rclk	(0x1034E0)
#define reg_ckg_v2_mod_sr_rclk_pre0	(0x103520)
#define reg_ckg_v2_mod_sr_rclk_pre1	(0x103524)
#define reg_ckg_v2_mod_sr_rclk_final	(0x10351C)

#define reg_ckg_mod_o_sr_wclk	(0x10348E)	// osd mft 1p
#define reg_ckg_mod_o_sr_rclk	(0x103494)
#define reg_ckg_mod_o_sr_rclk_pre0	(0x103508)
#define reg_ckg_mod_o_sr_rclk_pre1	(0x10350C)
#define reg_ckg_mod_o_sr_rclk_final	(0x103504)

#define reg_sw_en_mod_v1_sr_wclk2vby1_v1	(0x103C0E)	//bit1
#define reg_sw_en_mod_v1_sr_wclk2v1		(0x103C0E)	//bit0
#define reg_sw_en_mod_v2_sr_wclk2vby1_v2	(0x103C10)	//bit1
#define reg_sw_en_mod_v2_sr_wclk2v2		(0x103C10)	//bit0

#define reg_sw_en_v1_mod_sr_rclk2v1		(0x103C24)
#define reg_sw_en_v1_mod_sr_rclk_pre02v1	(0x103C56)
#define reg_sw_en_v1_mod_sr_rclk_pre12v1	(0x103C58)
#define reg_sw_en_v1_mod_sr_rclk_final2v1	(0x103C54)
#define reg_sw_en_v2_mod_sr_rclk2v2		(0x103C38)
#define reg_sw_en_v2_mod_sr_rclk_pre02v2	(0x103C5C)
#define reg_sw_en_v2_mod_sr_rclk_pre12v2	(0x103C5E)
#define reg_sw_en_v2_mod_sr_rclk_final2v2	(0x103C5A)

#define reg_sw_en_mod_o_sr_wclk2vby1_o		(0x103C0C)
#define reg_sw_en_mod_o_sr_wclk2o		(0x103C0C)
#define reg_sw_en_mod_o_sr_rclk2o		(0x103C12)
#define reg_sw_en_mod_o_sr_rclk_pre02o		(0x103C50)
#define reg_sw_en_mod_o_sr_rclk_pre12o		(0x103C52)
#define reg_sw_en_mod_o_sr_rclk_final2o		(0x103C4E)

//mod setting
#define reg_mod_2pto4p_en	(0x2427A0)
#define reg_mod_4pto8p_en	(0x2427A0)
#define reg_yuv_422_mode	(0x2427A0)
#define reg_mft_mode		(0x242702)
#define reg_div_pix_mode	(0x242702)
#define reg_hsize		(0x242720)
#define reg_div_len		(0x242722)
#define reg_base0_addr		(0x242724)
#define reg_base1_addr		(0x242726)
#define reg_base2_addr		(0x242728)
#define reg_base3_addr		(0x24272A)
#define reg_vby1_8pto16p_en	(0x242802)
#define reg_vby1_16pto32p_en	(0x242802)
#define reg_vby1_hs_inv		(0x242802)
#define reg_vby1_vs_inv		(0x242802)
#define reg_format_map		(0x242804)
#define reg_vby1_16ch_cken_31_16	(0x242804)
#define reg_vby1_swap_ch1_ch0		(0x2428D0)
#define reg_vby1_swap_ch3_ch2		(0x2428D2)
#define reg_vby1_swap_ch5_ch4		(0x2428D4)
#define reg_vby1_swap_ch7_ch6		(0x2428D6)
#define reg_vby1_swap_ch9_ch8		(0x2428D8)
#define reg_vby1_swap_ch11_ch10		(0x2428DA)
#define reg_vby1_swap_ch13_ch12		(0x2428DC)
#define reg_vby1_swap_ch15_ch14		(0x2428DE)
#define reg_vby1_swap_ch17_ch16		(0x2428E0)
#define reg_vby1_swap_ch19_ch18		(0x2428E2)
#define reg_vby1_swap_ch21_ch20		(0x2428E4)
#define reg_vby1_swap_ch23_ch22		(0x2428E6)
#define reg_vby1_swap_ch25_ch24		(0x2428E8)
#define reg_vby1_swap_ch27_ch26		(0x2428EA)
#define reg_vby1_swap_ch29_ch28		(0x2428EC)
#define reg_vby1_swap_ch31_ch30		(0x2428EE)

#define reg_vby1_byte_mode	(0x2428C4)
#define reg_seri_fotmat_v1	(0x2422AE)
#define reg_gcr_gank_clk_src_sel	(0x24231C)
#define reg_output_conf_ch07_ch00	(0x242330)
#define reg_output_conf_ch15_ch08	(0x242332)
#define reg_output_conf_ch19_ch16	(0x242334)
#define reg_output_conf_ch27_ch20	(0x242430)
#define reg_output_conf_ch35_ch28	(0x242432)
#define reg_sram_ext_en		(0x242A02)

#define reg_v1_sw_force_fix_trig	(0x2422A8)
#define reg_mft_read_hde_st		(0x24271E)
#define reg_mft_vld_wrap_fix_htt	(0x242706)

//vby1 setting for Deltav
#define reg_v2_sel		(0x242200)
#define reg_v2_mod_1pto2p_en	(0x242AA0)
#define reg_v2_mod_2pto4p_en	(0x242AA0)
#define reg_v2_mod_4pto8p_en	(0x242AA0)
#define reg_v2_mft_mode		(0x242A02)
#define reg_v2_div_pix_mode	(0x242A02)
#define reg_v2_hsize		(0x242A20)
#define reg_v2_div_len		(0x242A22)
#define reg_v2_base0_addr	(0x242A24)
#define reg_v2_base1_addr	(0x242A26)
#define reg_v2_vby1_8pto16p_en	(0x242B02)
#define reg_v2_vby1_hs_inv	(0x242B02)
#define reg_v2_vby1_vs_inv	(0x242B02)
#define reg_v2_format_map	(0x242B04)
#define reg_v2_vby1_swap_ch1_ch0		(0x242BD0)
#define reg_v2_vby1_swap_ch3_ch2		(0x242BD2)
#define reg_v2_vby1_swap_ch5_ch4		(0x242BD4)
#define reg_v2_vby1_swap_ch7_ch6		(0x242BD6)
#define reg_v2_vby1_swap_ch9_ch8		(0x242BD8)
#define reg_v2_vby1_swap_ch11_ch10		(0x242BDA)
#define reg_v2_vby1_swap_ch13_ch12		(0x242BDC)
#define reg_v2_vby1_swap_ch15_ch14		(0x242BDE)
#define reg_v2_vby1_byte_mode	(0x242BC4)
#define reg_seri_format_v2	(0x2422AE)
#define reg_seri_v2_mode	(0x2422A2)
#define reg_v2_gcr_bank_clk_src_sel	(0x24231C)
#define reg_v2_output_conf_ch19_ch16	(0x242334)
#define reg_v2_output_conf_ch27_ch20	(0x242430)
#define reg_v2_output_conf_ch35_ch28	(0x242432)

#define reg_v2_sw_force_fix_trig	(0x2422AA)
#define reg_v2_mft_read_hde_st		(0x242A1E)

#define reg_o_mod_1pto2p_en	(0x242CA0)
#define reg_o_mod_2pto4p_en	(0x242CA0)
#define reg_o_mod_4pto8p_en	(0x242CA0)
#define reg_o_mft_mode		(0x242C02)
#define reg_o_div_pix_mode	(0x242C02)
#define reg_o_hize		(0x242C20)
#define reg_o_div_len		(0x242C22)
#define reg_o_base0_addr	(0x242C24)
#define reg_o_vby1_hs_inv	(0x242D02)
#define reg_o_vby1_vs_inv	(0x242D02)
#define reg_o_format_map	(0x242D04)
#define reg_o_vby1_byte_mode	(0x242DC4)
#define reg_seri_format_osd	(0x2422AE)
#define reg_seri_osd_mode	(0x2422A2)
#define reg_o_gcr_bank_clk_src_sel	(0x24231C)
#define reg_o_output_conf_ch35_ch28	(0x242432)
#define reg_o_output_conf_ch39_ch36	(0x242434)

#define reg_osd_sw_force_fix_trig	(0x2422AC)
#define reg_o_mft_read_hde_st		(0x242C1E)

#define reg_datax_sel			(0x242260)
#define reg_gcr_en			(0x242302)
#define reg_mpll_pd			(0x2423D8)
#define reg_mpll_clk_adc_vco_div2_pd	(0x2423D8)
#define reg_lpll1_pd			(0x243006)
//#define reg_lpll2_pd                  (0x243066)

#define reg_lpll_set			(0x24301E)

//tgen
#define reg_tgen_ts_md		(0x13A003)
#define reg_htt			(0x13A026)
#define reg_hsync_st		(0x13A028)
#define reg_hsync_end		(0x13A02A)
#define reg_hde_frame_st	(0x13A02C)
#define reg_hde_frame_end	(0x13A02E)
#define reg_hde_frame_v_st	(0x13A030)
#define reg_hde_frame_v_end	(0x13A032)
#define reg_hde_frame_g_st	(0x13A034)
#define reg_hde_frame_g_end	(0x13A036)

#define reg_vtt			(0x13A046)
#define reg_vsync_st		(0x13A048)
#define reg_vsync_end		(0x13A04A)
#define reg_vde_frame_st	(0x13A04C)
#define reg_vde_frame_end	(0x13A04E)
#define reg_vde_frame_v_st	(0x13A050)
#define reg_vde_frame_v_end	(0x13A052)

#define reg_vde_frame_g_st	(0x13A054)
#define reg_vde_frame_g_end	(0x13A056)
#define reg_m_delta		(0x13A0FA)
#define reg_protect_v_end	(0x13A232)
#define reg_panel_lower_bound	(0x13A0FC)
#define reg_panel_upper_bound	(0x13A0FE)
#define reg_even_vtt_vcnt_en	(0x13A042)
#define reg_tgen_sw_rst		(0x13A003)
#define reg_tgen_video_en	(0x13A004)

#define reg_hde_video0_st_1p	(0x13A060)
#define reg_hde_video0_end_1p	(0x13A062)
#define reg_vde_video0_st_1p	(0x13A080)
#define reg_vde_video0_end_1p	(0x13A082)

/***********
 *REG setting
 ************/
#define REG_MAX_INDEX (50)

/***********
 *FRAMESYNC setting
 ************/
#define FRAMESYNC_VTTV_NORMAL_PHASE_DIFF (500)
#define FRAMESYNC_VTTV_LOW_LATENCY_PHASE_DIFF (55)
#define FRAMESYNC_VTTV_FRAME_COUNT (3)
#define FRAMESYNC_VTTV_THRESHOLD (3)
#define FRAMESYNC_VTTV_DIFF_LIMIT (5)
#define FRAMESYNC_VTTV_LOCKED_LIMIT (9)
#define FRAMESYNC_VRR_DEPOSIT_COUNT (1)
#define FRAMESYNC_VRR_STATE_SW_MODE (4)
#define FRAMESYNC_VRR_CHASE_TOTAL_DIFF_RANGE (2)
#define FRAMESYNC_VRR_CHASE_PHASE_TARGET (2)
#define FRAMESYNC_VRR_CHASE_PHASE_ALMOST (2)
#define FRAMESYNC_VRR_PROTECT_DIV (8)

void mtk_render_output_en(
	struct mtk_panel_context *pctx, bool bEn);
bool mtk_render_cfg_connector(
	struct mtk_panel_context *pctx);
bool mtk_render_cfg_crtc(
	struct mtk_panel_context *pctx);
int _mtk_video_set_customize_frc_table(
	struct mtk_panel_context *pctx_pnl,
	struct drm_st_pnl_frc_table_info *customizeFRCTableInfo);
int _mtk_video_get_customize_frc_table(
	struct mtk_panel_context *pctx_pnl,
	void *CurrentFRCTableInfo);
int _mtk_video_set_framesync_mode(
	struct mtk_tv_kms_context *pctx);
int _mtk_video_set_low_latency_mode(
	struct mtk_tv_kms_context *pctx,
	uint64_t bLowLatencyEn);

#endif
