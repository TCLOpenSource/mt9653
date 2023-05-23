/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_TV_LPLL_TBL_H_
#define _MTK_TV_LPLL_TBL_H_

#include <drm/mtk_tv_drm.h>

#define LPLL_REG_NUM    52

typedef enum {
	E_PNL_SUPPORTED_LPLL_LVDS_1ch_450to600MHz,	//0
	E_PNL_SUPPORTED_LPLL_LVDS_1ch_300to450MHz,	//1
	E_PNL_SUPPORTED_LPLL_LVDS_1ch_300to300MHz,	//2

	E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz,	//3
	E_PNL_SUPPORTED_LPLL_LVDS_2ch_300to450MHz,	//4
	E_PNL_SUPPORTED_LPLL_LVDS_2ch_300to300MHz,	//5

	E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_450to600MHz,	//6
	E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_300to450MHz,	//7
	E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_300to300MHz,	//8

	E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_450to600MHz,	//9
	E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_300to450MHz,	//10
	E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_300to300MHz,	//11

	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_3BYTE_400to600MHz,	//12
	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_3BYTE_300to400MHz,	//13
	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_3BYTE_300to300MHz,	//14

	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_4BYTE_400to600MHz,	//15
	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_4BYTE_300to400MHz,	//16
	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_4BYTE_300to300MHz,	//17

	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_5BYTE_400to600MHz,	//18
	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_5BYTE_300to400MHz,	//19
	E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_5BYTE_300to300MHz,	//20

	E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_400to600MHz,	//21
	E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_300to400MHz,	//22
	E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_300to300MHz,	//23

	E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_4BYTE_300to600MHz,	//24
	E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_4BYTE_300to300MHz,	//25

	E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz,	//26
	E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to300MHz,	//27

	E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_3BYTE_400to600MHz,	//28
	E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_3BYTE_300to400MHz,	//29
	E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_3BYTE_300to300MHz,	//30

	E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_4BYTE_300to600MHz,	//31
	E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_4BYTE_300to300MHz,	//32

	E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_5BYTE_300to600MHz,	//33
	E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_5BYTE_300to300MHz,	//34

	E_PNL_SUPPORTED_LPLL_MAX,	//35
} E_PNL_SUPPORTED_LPLL_TYPE;

typedef enum {
	reg_lpll1_ibias_ictrl,
	reg_lpll1_input_div_first,
	reg_lpll1_loop_div_first,
	reg_lpll1_loop_div_second,
	reg_lpll1_output_div_second,
	reg_lpll1_skew_div,
	reg_lpll1_fifo_div,
	reg_lpll1_fifo_div5_en,
	reg_lpll1_en_fix_clk,
	reg_lpll1_dual_lp_en,
	reg_lpll1_sdiv2p5_en,
	reg_lpll1_en_mini,
	reg_lpll1_en_fifo,
	reg_lpll1_test,
	reg_lpll1_en_vby1_hs,
	reg_lpll2_pd,
	reg_lpll2_ibias_ictrl,
	reg_lpll2_input_div_first,
	reg_lpll2_loop_div_first,
	reg_lpll2_loop_div_second,
	reg_lpll2_output_div_first,
	reg_lpll2_test,
	reg_lpll1_scalar2fifo_en,
	reg_lpll1_scalar2fifo_div2,
	reg_lpllo_ictrl,
	reg_lpllo_input_div_first,
	reg_lpllo_loop_div_first,
	reg_lpllo_loop_div_second,
	reg_lpllo_scalar_div_first,
	reg_lpllo_scalar_div_second,
	reg_lpllo_skew_div,
	reg_lpllo_fifo_div,
	reg_lpllo_fifo_div5_en,
	reg_lpllo_dual_lp_en,
	reg_lpllo_sdiv2p5_en,
	reg_lpllo_en_mini,
	reg_lpllo_en_fifo,
	reg_lpllo_test,
	reg_lpllo_pd,
	reg_lpll_2ndpll_clk_sel,
	reg_lpll1_en_scalar,
	reg_lpll1_test_en_sdiv,
} E_LPLL_REG_ADDR;

typedef struct {
	E_LPLL_REG_ADDR address;
	uint16_t value;
	uint16_t mask;
} TBLStruct, *pTBLStruct;



TBLStruct LPLLSettingTBL[E_PNL_SUPPORTED_LPLL_MAX][LPLL_REG_NUM] = {
	{			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_450to600MHz    NO.0
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0000, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_300to450MHz    NO.1
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x1000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0100, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_300to300MHz    NO.2
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x1000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0100, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz    NO.3
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0000, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_300to450MHz    NO.4
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x1000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0100, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_300to300MHz    NO.5
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x1000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0100, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_450to600MHz    NO.6
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0000, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_300to450MHz    NO.7
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_300to300MHz    NO.8
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_450to600MHz    NO.9
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0000, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_300to450MHz    NO.10
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_300to300MHz    NO.11
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0700, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x4000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0007, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0004, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{
		//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_3BYTE_400to600MHz    NO.12
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0000, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000F, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_3BYTE_300to400MHz    NO.13
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000E, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_3BYTE_300to300MHz    NO.14
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000E, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_4BYTE_400to600MHz    NO.15
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0000, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000A, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_4BYTE_300to400MHz    NO.16
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000A, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_4BYTE_300to300MHz    NO.17
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000A, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_5BYTE_400to600MHz    NO.18
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0000, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0009, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0040, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_5BYTE_300to400MHz    NO.19
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0009, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0040, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_5BYTE_300to300MHz    NO.20
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0008, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0F00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x0000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0009, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0040, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_400to600MHz    NO.21
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0A00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0000, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000F, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0040, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_300to400MHz    NO.22
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0A00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0002, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000F, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_300to300MHz    NO.23
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0A00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0002, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000F, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_4BYTE_300to600MHz    NO.24
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0A00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000A, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_4BYTE_300to300MHz    NO.25
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0A00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000A, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz    NO.26
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0A00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0009, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0040, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to300MHz    NO.27
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0001, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0A00, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0009, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0040, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_3BYTE_400to600MHz    NO.28
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0900, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0001, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0000, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000F, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0040, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_3BYTE_300to400MHz    NO.29
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0900, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0001, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0002, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000F, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_3BYTE_300to300MHz    NO.30
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0900, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0001, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0002, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000F, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0000, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_4BYTE_300to600MHz    NO.31
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0900, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0001, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000A, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_4BYTE_300to300MHz    NO.32
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0900, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0001, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0001, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x1000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x000A, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x0000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_5BYTE_300to600MHz    NO.33
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0900, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0001, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0009, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

	{	//E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_5BYTE_300to300MHz    NO.34
	 //Address,Value,Mask
	 {reg_lpll1_ibias_ictrl, 0x0004, 0x001C},	//reg_lpll1_ibias_ictrl
	 {reg_lpll1_input_div_first, 0x0000, 0x0003},	//reg_lpll1_input_div_first
	 {reg_lpll1_loop_div_first, 0x0000, 0x0003},	//reg_lpll1_loop_div_first
	 {reg_lpll1_loop_div_second, 0x0900, 0x1F00},	//reg_lpll1_loop_div_second
	 {reg_lpll1_output_div_second, 0x3000, 0x3000},	//reg_lpll1_output_div_second
	 {reg_lpll1_output_div_second, 0x0000, 0x0F00},	//reg_lpll1_output_div_second
	 {reg_lpll1_skew_div, 0x7000, 0x7000},	//reg_lpll1_skew_div
	 {reg_lpll1_fifo_div, 0x0000, 0x0007},	//reg_lpll1_fifo_div
	 {reg_lpll1_fifo_div5_en, 0x0000, 0x0800},	//reg_lpll1_fifo_div5_en
	 {reg_lpll1_en_fix_clk, 0x1000, 0x1000},	//reg_lpll1_en_fix_clk
	 {reg_lpll1_dual_lp_en, 0x8000, 0x8000},	//reg_lpll1_dual_lp_en
	 {reg_lpll1_sdiv2p5_en, 0x0000, 0x0400},	//reg_lpll1_sdiv2p5_en
	 {reg_lpll1_en_mini, 0x0000, 0x4000},	//reg_lpll1_en_mini
	 {reg_lpll1_en_fifo, 0x0000, 0x0040},	//reg_lpll1_en_fifo
	 {reg_lpll1_test, 0x8000, 0x8000},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0001, 0x0001},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0040, 0x0040},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0002},	//reg_lpll1_test
	 {reg_lpll1_en_vby1_hs, 0x0000, 0x0001},	//reg_lpll1_en_vby1_hs
	 {reg_lpll2_pd, 0x0000, 0x0020},	//reg_lpll2_pd
	 {reg_lpll2_ibias_ictrl, 0x0000, 0x0004},	//reg_lpll2_ibias_ictrl
	 {reg_lpll2_input_div_first, 0x0000, 0x001F},	//reg_lpll2_input_div_first
	 {reg_lpll2_loop_div_first, 0x0001, 0x0003},	//reg_lpll2_loop_div_first
	 {reg_lpll2_loop_div_second, 0x0300, 0x1F00},	//reg_lpll2_loop_div_second
	 {reg_lpll2_output_div_first, 0x0000, 0x000F},	//reg_lpll2_output_div_first
	 {reg_lpll2_test, 0x00C0, 0x00E0},	//reg_lpll2_test
	 {reg_lpll1_scalar2fifo_en, 0x0000, 0x0200},	//reg_lpll1_scalar2fifo_en
	 {reg_lpll1_scalar2fifo_div2, 0x0000, 0x0100},	//reg_lpll1_scalar2fifo_div2
	 {reg_lpllo_ictrl, 0x0003, 0x0007},	//reg_lpllo_ictrl
	 {reg_lpllo_input_div_first, 0x0000, 0x0030},	//reg_lpllo_input_div_first
	 {reg_lpllo_loop_div_first, 0x0000, 0x3000},	//reg_lpllo_loop_div_first
	 {reg_lpllo_loop_div_second, 0x0009, 0x000F},	//reg_lpllo_loop_div_second
	 {reg_lpllo_scalar_div_first, 0x00C0, 0x00C0},	//reg_lpllo_scalar_div_first
	 {reg_lpllo_scalar_div_second, 0x0000, 0x0F00},	//reg_lpllo_scalar_div_second
	 {reg_lpllo_skew_div, 0x0700, 0x0700},	//reg_lpllo_skew_div
	 {reg_lpllo_fifo_div, 0x0000, 0x0070},	//reg_lpllo_fifo_div
	 {reg_lpllo_fifo_div5_en, 0x0000, 0x0020},	//reg_lpllo_fifo_div5_en
	 {reg_lpllo_dual_lp_en, 0x0010, 0x0010},	//reg_lpllo_dual_lp_en
	 {reg_lpllo_sdiv2p5_en, 0x0000, 0x0080},	//reg_lpllo_sdiv2p5_en
	 {reg_lpllo_en_mini, 0x0000, 0x0004},	//reg_lpllo_en_mini
	 {reg_lpllo_en_fifo, 0x0000, 0x0002},	//reg_lpllo_en_fifo
	 {reg_lpllo_test, 0x8000, 0x8000},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0001, 0x0001},	//reg_lpllo_test
	 {reg_lpllo_test, 0x0000, 0x0040},	//reg_lpllo_test
	 {reg_lpllo_pd, 0x2000, 0x2000},	//reg_lpllo_pd
	 {reg_lpll_2ndpll_clk_sel, 0x0020, 0x0020},	//reg_lpll_2ndpll_clk_sel
	 {reg_lpll1_en_scalar, 0x0000, 0x0010},	//reg_lpll1_en_scalar
	 {reg_lpll1_test_en_sdiv, 0x0000, 0x1000},	//reg_lpll1_test_en_sdiv
	 {reg_lpll1_test, 0x0000, 0x0800},	//reg_lpll1_test
	 {reg_lpll1_test, 0x0000, 0x0200},	//reg_lpll1_test
	 {reg_lpll2_test, 0x0000, 0x0001},	//reg_lpll2_test
	 {reg_lpll2_test, 0x0000, 0x1000},	//reg_lpll2_test
	 },

};

uint16_t u16LoopGain[E_PNL_SUPPORTED_LPLL_MAX] = {
	6,			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_450to600MHz    NO.0
	6,			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_300to450MHz    NO.1
	6,			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_300to300MHz    NO.2
	6,			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz    NO.3
	6,			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_300to450MHz    NO.4
	6,			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_300to300MHz    NO.5
	6,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_450to600MHz    NO.6
	6,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_300to450MHz    NO.7
	6,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_300to300MHz    NO.8
	6,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_450to600MHz    NO.9
	6,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_300to450MHz    NO.10
	6,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_300to300MHz    NO.11
	6,			//Vx1_Video_3BYTE_OSD_3BYTE_400to600MHz    NO.12
	6,			//Vx1_Video_3BYTE_OSD_3BYTE_300to400MHz    NO.13
	6,			//Vx1_Video_3BYTE_OSD_3BYTE_300to300MHz    NO.14
	6,			//Vx1_Video_3BYTE_OSD_4BYTE_400to600MHz    NO.15
	6,			//Vx1_Video_3BYTE_OSD_4BYTE_300to400MHz    NO.16
	6,			//Vx1_Video_3BYTE_OSD_4BYTE_300to300MHz    NO.17
	6,			//Vx1_Video_3BYTE_OSD_5BYTE_400to600MHz    NO.18
	6,			//Vx1_Video_3BYTE_OSD_5BYTE_300to400MHz    NO.19
	6,			//Vx1_Video_3BYTE_OSD_5BYTE_300to300MHz    NO.20
	6,			//Vx1_Video_4BYTE_OSD_3BYTE_400to600MHz    NO.21
	6,			//Vx1_Video_4BYTE_OSD_3BYTE_300to400MHz    NO.22
	6,			//Vx1_Video_4BYTE_OSD_3BYTE_300to300MHz    NO.23
	6,			//Vx1_Video_4BYTE_OSD_4BYTE_300to600MHz    NO.24
	6,			//Vx1_Video_4BYTE_OSD_4BYTE_300to300MHz    NO.25
	6,			//Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz    NO.26
	6,			//Vx1_Video_4BYTE_OSD_5BYTE_300to300MHz    NO.27
	6,			//Vx1_Video_5BYTE_OSD_3BYTE_400to600MHz    NO.28
	6,			//Vx1_Video_5BYTE_OSD_3BYTE_300to400MHz    NO.29
	6,			//Vx1_Video_5BYTE_OSD_3BYTE_300to300MHz    NO.30
	6,			//Vx1_Video_5BYTE_OSD_4BYTE_300to600MHz    NO.31
	6,			//Vx1_Video_5BYTE_OSD_4BYTE_300to300MHz    NO.32
	6,			//Vx1_Video_5BYTE_OSD_5BYTE_300to600MHz    NO.33
	6,			//Vx1_Video_5BYTE_OSD_5BYTE_300to300MHz    NO.34
};

uint16_t u16LoopDiv[E_PNL_SUPPORTED_LPLL_MAX] = {
	2,			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_450to600MHz    NO.0
	2,			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_300to450MHz    NO.1
	2,			//E_PNL_SUPPORTED_LPLL_LVDS_1ch_300to300MHz    NO.2
	2,			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz    NO.3
	2,			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_300to450MHz    NO.4
	2,			//E_PNL_SUPPORTED_LPLL_LVDS_2ch_300to300MHz    NO.5
	2,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_450to600MHz    NO.6
	2,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_300to450MHz    NO.7
	2,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_1ch_300to300MHz    NO.8
	2,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_450to600MHz    NO.9
	2,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_300to450MHz    NO.10
	2,			//E_PNL_SUPPORTED_LPLL_HS_LVDS_2ch_300to300MHz    NO.11
	2,			//Vx1_Video_3BYTE_OSD_3BYTE_400to600MHz    NO.12
	2,			//Vx1_Video_3BYTE_OSD_3BYTE_300to400MHz    NO.13
	2,			//Vx1_Video_3BYTE_OSD_3BYTE_300to300MHz    NO.14
	2,			//Vx1_Video_3BYTE_OSD_4BYTE_400to600MHz    NO.15
	2,			//Vx1_Video_3BYTE_OSD_4BYTE_300to400MHz    NO.16
	2,			//Vx1_Video_3BYTE_OSD_4BYTE_300to300MHz    NO.17
	2,			//Vx1_Video_3BYTE_OSD_5BYTE_400to600MHz    NO.18
	2,			//Vx1_Video_3BYTE_OSD_5BYTE_300to400MHz    NO.19
	2,			//Vx1_Video_3BYTE_OSD_5BYTE_300to300MHz    NO.20
	2,			//Vx1_Video_4BYTE_OSD_3BYTE_400to600MHz    NO.21
	2,			//Vx1_Video_4BYTE_OSD_3BYTE_300to400MHz    NO.22
	2,			//Vx1_Video_4BYTE_OSD_3BYTE_300to300MHz    NO.23
	2,			//Vx1_Video_4BYTE_OSD_4BYTE_300to600MHz    NO.24
	2,			//Vx1_Video_4BYTE_OSD_4BYTE_300to300MHz    NO.25
	2,			//Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz    NO.26
	2,			//Vx1_Video_4BYTE_OSD_5BYTE_300to300MHz    NO.27
	2,			//Vx1_Video_5BYTE_OSD_3BYTE_400to600MHz    NO.28
	2,			//Vx1_Video_5BYTE_OSD_3BYTE_300to400MHz    NO.29
	2,			//Vx1_Video_5BYTE_OSD_3BYTE_300to300MHz    NO.30
	2,			//Vx1_Video_5BYTE_OSD_4BYTE_300to600MHz    NO.31
	2,			//Vx1_Video_5BYTE_OSD_4BYTE_300to300MHz    NO.32
	2,			//Vx1_Video_5BYTE_OSD_5BYTE_300to600MHz    NO.33
	2,			//Vx1_Video_5BYTE_OSD_5BYTE_300to300MHz    NO.34
};

#endif				//_LPLL_TBL_H_
