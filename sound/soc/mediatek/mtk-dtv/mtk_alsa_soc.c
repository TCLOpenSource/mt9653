// SPDX-License-Identifier: GPL-2.0
/*
 * mtk_alsa_soc.c  --  Mediatek DTV mtk_alsa_soc function
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <sound/soc.h>
#include <linux/timer.h>
#include <linux/firmware.h>

MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

#include "mtk_alsa_chip.h"
#include "mtk_alsa_pcm_platform.h"

#define END_OF_TABLE	0xFFFFFF

struct timer_list timer_charge;

//--------------------------------------------------
// GATE CLOCK (IP_version 1)
//--------------------------------------------------
enum {
	GATE_XTAL_12M,
	GATE_XTAL_24M,
	GATE_MCU_NONPM,
	GATE_AU_UART0,
	GATE_SMI,
	GATE_RIU,
	GATE_AU_RIU,
	GATE_HDMI_RX_V2A_TIME_STAMP,
	GATE_HDMI_RX2_V2A_TIME_STAMP,
	GATE_BCLK_I2S_ENCODER_TO_PAD,
	GATE_BCLK_I2S_ENCODER2_TO_PAD,
	GATE_INVERSE_OF_I2S_TDM_6X16TX32B_BCLK,
	GATE_INVERSE_OF_I2S_TDM_1X16TX32B_BCLK,
	GATE_R2,
	GATE_DAP,
	GATE_DAP_TCK,
	GATE_CLK_NUM
};

static const char *gate_clk[GATE_CLK_NUM] = {
	[GATE_XTAL_12M] = "viva_xtal_12m_int_ck",
	[GATE_XTAL_24M] = "viva_xtal_24m_int_ck",
	[GATE_MCU_NONPM] = "mcu_nonpm_int_ck",
	[GATE_AU_UART0] = "au_uart0_int_ck",
	[GATE_SMI] = "viva_smi_int_ck",
	[GATE_RIU] = "viva_riu_int_ck",
	[GATE_AU_RIU] = "viva_au_riu_int_ck",
	[GATE_HDMI_RX_V2A_TIME_STAMP] = "viva_hdmi_rx_v2a_time_stamp_int_ck",
	[GATE_HDMI_RX2_V2A_TIME_STAMP] = "viva_hdmi_rx2_v2a_time_stamp_int_ck",
	[GATE_BCLK_I2S_ENCODER_TO_PAD] = "bclk_i2s_encoder_to_pad_int_ck",
	[GATE_BCLK_I2S_ENCODER2_TO_PAD] = "bclk_i2s_encoder2_to_pad_int_ck",
	[GATE_INVERSE_OF_I2S_TDM_6X16TX32B_BCLK] = "inverse_of_viva_i2s_tdm_6x16tx32b_bclk_int_ck",
	[GATE_INVERSE_OF_I2S_TDM_1X16TX32B_BCLK] = "inverse_of_viva_i2s_tdm_1x16tx32b_bclk_int_ck",
	[GATE_R2] = "r2_int_ck",
	[GATE_DAP] = "dap_int_ck",
	[GATE_DAP_TCK] = "dap_tck_int_ck",
};

//--------------------------------------------------
// MUX CLOCK (IP_version 1)
//--------------------------------------------------
enum {
	MUX_RIU_BRG_R2,
	MUX_RIU_R2,
	MUX_AU_MCU,
	MUX_AU_MCU_BUS,
	MUX_SYNTH_CODEC,
	MUX_SYNTH_2ND_ORDER,
	MUX_AUPLL_IN,
	MUX_DSP_INT,
	MUX_SIF_ADC_R2B_FIFO_IN,
	MUX_SIF_ADC_R2B,
	MUX_SIF_ADC_CIC,
	MUX_FM_DEMODULATOR,
	MUX_SIF_SYNTH,
	MUX_SIF_CORDIC,
	MUX_PARSER_INT,
	MUX_GPA_480FSO,
	MUX_GPA_240FSO,
	MUX_GPA_256FSO,
	MUX_ADC_512FS,
	MUX_DAC_512FS,
	MUX_AB_MAC,
	MUX_AB3_MAC,
	MUX_SH,
	MUX_CODEC_SH,
	MUX_PCM_I2S_256FS_MUX,
	MUX_CH_M1_256FSI_MUX1,
	MUX_CH_M1_256FSI_MUX0,
	MUX_CH_M1_256FSI_MUX,
	MUX_CH_M1_256FSI,
	MUX_CH_M2_256FSI_MUX1,
	MUX_CH_M2_256FSI_MUX0,
	MUX_CH_M2_256FSI_MUX,
	MUX_CH_M2_256FSI,
	MUX_CH_M3_256FSI_MUX1,
	MUX_CH_M3_256FSI_MUX0,
	MUX_CH_M3_256FSI_MUX,
	MUX_CH_M3_256FSI,
	MUX_CH_M4_256FSI_MUX1,
	MUX_CH_M4_256FSI_MUX0,
	MUX_CH_M4_256FSI_MUX,
	MUX_CH_M4_256FSI,
	MUX_CH_M5_256FSI_MUX1,
	MUX_CH_M5_256FSI_MUX0,
	MUX_CH_M5_256FSI_MUX,
	MUX_CH_M5_256FSI,
	MUX_CH_M6_256FSI_MUX1,
	MUX_CH_M6_256FSI_MUX0,
	MUX_CH_M6_256FSI_MUX,
	MUX_CH_M6_256FSI,
	MUX_CH_M7_256FSI_MUX1,
	MUX_CH_M7_256FSI_MUX0,
	MUX_CH_M7_256FSI_MUX,
	MUX_CH_M7_256FSI,
	MUX_CH_M8_256FSI_MUX1,
	MUX_CH_M8_256FSI_MUX0,
	MUX_CH_M8_256FSI_MUX,
	MUX_CH_M8_256FSI,
	MUX_CH_M9_256FSI_MUX1,
	MUX_CH_M9_256FSI_MUX0,
	MUX_CH_M9_256FSI_MUX,
	MUX_CH_M9_256FSI,
	MUX_CH_M10_256FSI_MUX1,
	MUX_CH_M10_256FSI_MUX0,
	MUX_CH_M10_256FSI_MUX,
	MUX_CH_M10_256FSI,
	MUX_CH_M11_256FSI_MUX1,
	MUX_CH_M11_256FSI_MUX0,
	MUX_CH_M11_256FSI_MUX,
	MUX_CH_M11_256FSI,
	MUX_CH_M12_256FSI_MUX1,
	MUX_CH_M12_256FSI_MUX0,
	MUX_CH_M12_256FSI_MUX,
	MUX_CH_M12_256FSI,
	MUX_CH_M13_256FSI_MUX1,
	MUX_CH_M13_256FSI_MUX0,
	MUX_CH_M13_256FSI_MUX,
	MUX_CH_M13_256FSI,
	MUX_CH_M14_256FSI_MUX1,
	MUX_CH_M14_256FSI_MUX0,
	MUX_CH_M14_256FSI_MUX,
	MUX_CH_M14_256FSI,
	MUX_CH_M15_256FSI_MUX1,
	MUX_CH_M15_256FSI_MUX0,
	MUX_CH_M15_256FSI_MUX,
	MUX_CH_M15_256FSI,
	MUX_CH_M16_256FSI_MUX1,
	MUX_CH_M16_256FSI_MUX0,
	MUX_CH_M16_256FSI_MUX,
	MUX_CH_M16_256FSI,
	MUX_CH5_256FSI_MUX2,
	MUX_CH5_256FSI_MUX1,
	MUX_CH5_256FSI_MUX0,
	MUX_CH5_256FSI_MUX,
	MUX_CH5_256FSI,
	MUX_CH6_256FSI_MUX2,
	MUX_CH6_256FSI_MUX1,
	MUX_CH6_256FSI_MUX0,
	MUX_CH6_256FSI_MUX,
	MUX_CH6_256FSI,
	MUX_CH7_256FSI_MUX2,
	MUX_CH7_256FSI_MUX1,
	MUX_CH7_256FSI_MUX0,
	MUX_CH7_256FSI_MUX,
	MUX_CH7_256FSI,
	MUX_CH8_256FSI_MUX2,
	MUX_CH8_256FSI_MUX1,
	MUX_CH8_256FSI_MUX0,
	MUX_CH8_256FSI_MUX,
	MUX_CH8_256FSI,
	MUX_CH9_256FSI_MUX2,
	MUX_CH9_256FSI_MUX1,
	MUX_CH9_256FSI_MUX0,
	MUX_CH9_256FSI_MUX,
	MUX_CH9_256FSI,
	MUX_CH10_256FSI_MUX2,
	MUX_CH10_256FSI_MUX1,
	MUX_CH10_256FSI_MUX0,
	MUX_CH10_256FSI_MUX,
	MUX_CH10_256FSI,
	MUX_DVB1_SYNTH_REF,
	MUX_DECODER1_SYNTH_HDMI_CTSN_256FS,
	MUX_DECODER1_SYNTH_HDMI_CTSN_128FS,
	MUX_DECODER1_SYNTH_HDMI2_CTSN_256FS,
	MUX_DECODER1_SYNTH_HDMI2_CTSN_128FS,
	MUX_DECODER1_SYNTH_SPDIF_CDR_256FS,
	MUX_DECODER1_SYNTH_SPDIF_CDR_128FS,
	MUX_DECODER1_256FS_MUX0,
	MUX_DECODER1_256FS_MUX1,
	MUX_DECODER1_256FS_MUX,
	MUX_DVB1_TIMING_GEN_256FS,
	MUX_DECODER1_128FS_MUX0,
	MUX_DECODER1_128FS_MUX1,
	MUX_DECODER1_128FS_MUX,
	MUX_DVB2_SYNTH_REF,
	MUX_DECODER2_SYNTH_HDMI_CTSN_256FS,
	MUX_DECODER2_SYNTH_HDMI_CTSN_128FS,
	MUX_DECODER2_SYNTH_HDMI2_CTSN_256FS,
	MUX_DECODER2_SYNTH_HDMI2_CTSN_128FS,
	MUX_DECODER2_SYNTH_SPDIF_CDR_256FS,
	MUX_DECODER2_SYNTH_SPDIF_CDR_128FS,
	MUX_DECODER2_256FS_MUX0,
	MUX_DECODER2_256FS_MUX1,
	MUX_DECODER2_256FS_MUX,
	MUX_DVB2_TIMING_GEN_256FS,
	MUX_DECODER2_128FS_MUX0,
	MUX_DECODER2_128FS_MUX1,
	MUX_DECODER2_128FS_MUX,
	MUX_DVB3_SYNTH_REF,
	MUX_DECODER3_SYNTH_HDMI_CTSN_256FS,
	MUX_DECODER3_SYNTH_HDMI_CTSN_128FS,
	MUX_DECODER3_SYNTH_HDMI2_CTSN_256FS,
	MUX_DECODER3_SYNTH_HDMI2_CTSN_128FS,
	MUX_DECODER3_SYNTH_SPDIF_CDR_256FS,
	MUX_DECODER3_SYNTH_SPDIF_CDR_128FS,
	MUX_DECODER3_256FS_MUX0,
	MUX_DECODER3_256FS_MUX1,
	MUX_DECODER3_256FS_MUX,
	MUX_DVB3_TIMING_GEN_256FS,
	MUX_DECODER3_128FS_MUX0,
	MUX_DECODER3_128FS_MUX1,
	MUX_DECODER3_128FS_MUX,
	MUX_DVB4_SYNTH_REF,
	MUX_DECODER4_SYNTH_HDMI_CTSN_256FS,
	MUX_DECODER4_SYNTH_HDMI_CTSN_128FS,
	MUX_DECODER4_SYNTH_HDMI2_CTSN_256FS,
	MUX_DECODER4_SYNTH_HDMI2_CTSN_128FS,
	MUX_DECODER4_SYNTH_SPDIF_CDR_256FS,
	MUX_DECODER4_SYNTH_SPDIF_CDR_128FS,
	MUX_DECODER4_256FS_MUX0,
	MUX_DECODER4_256FS_MUX1,
	MUX_DECODER4_256FS_MUX,
	MUX_DVB4_TIMING_GEN_256FS,
	MUX_DECODER4_128FS_MUX0,
	MUX_DECODER4_128FS_MUX1,
	MUX_DECODER4_128FS_MUX,
	MUX_DVB5_TIMING_GEN_256FS,
	MUX_DVB6_TIMING_GEN_256FS,
	MUX_AUDMA_V2_R1_SYNTH_REF,
	MUX_AUDMA_V2_R1_256FS_MUX,
	MUX_AUDMA_V2_R1_256FS,
	MUX_AUDMA_V2_R2_SYNTH_REF,
	MUX_AUDMA_V2_R2_256FS,
	MUX_SPDIF_RX_128FS,
	MUX_SPDIF_RX_DG,
	MUX_SPDIF_RX_SYNTH_REF,
	MUX_HDMI_RX_SYNTH_REF,
	MUX_HDMI_RX_S2P_256FS_MUX,
	MUX_HDMI_RX_S2P_256FS,
	MUX_HDMI_RX_S2P_128FS_MUX,
	MUX_HDMI_RX_S2P_128FS,
	MUX_HDMI_RX_MPLL_432,
	MUX_HDMI_RX2_SYNTH_REF,
	MUX_HDMI_RX2_S2P_256FS_MUX,
	MUX_HDMI_RX2_S2P_256FS,
	MUX_HDMI_RX2_S2P_128FS_MUX,
	MUX_HDMI_RX2_S2P_128FS,
	MUX_HDMI_RX2_MPLL_432,
	MUX_BCLK_I2S_DECODER,
	MUX_MCLK_I2S_DECODER,
	MUX_I2S_BCLK_DG,
	MUX_I2S_SYNTH_REF,
	MUX_BCLK_I2S_DECODER2,
	MUX_MCLK_I2S_DECODER2,
	MUX_I2S_BCLK_DG2,
	MUX_BI2S_SYNTH_REF2,
	MUX_HDMI_TX_TO_I2S_TX_256FS_MUX,
	MUX_HDMI_TX_TO_I2S_TX_128FS_MUX,
	MUX_HDMI_TX_TO_I2S_TX_64FS_MUX,
	MUX_HDMI_TX_TO_I2S_TX_32FS_MUX,
	MUX_HDMI_RX_TO_I2S_TX_256FS_MUX,
	MUX_HDMI_RX_TO_I2S_TX_128FS_MUX,
	MUX_HDMI_RX_TO_I2S_TX_64FS_MUX,
	MUX_HDMI_RX_TO_I2S_TX_32FS_MUX,
	MUX_I2S_TX_FIFO_256FS,
	MUX_I2S_MCLK_SYNTH_REF,
	MUX_MCLK_I2S_ENCODER,
	MUX_I2S_BCLK_SYNTH_REF_MUX,
	MUX_I2S_BCLK_SYNTH_REF,
	MUX_BCLK_I2S_ENCODER_128FS_SSC_MUX,
	MUX_BCLK_I2S_ENCODER_64FS_SSC_MUX,
	MUX_BCLK_I2S_ENCODER_32FS_SSC_MUX,
	MUX_BCLK_I2S_ENCODER_MUX,
	MUX_BCLK_I2S_ENCODER,
	MUX_I2S_TX_2_FIFO_256FS,
	MUX_MCLK_I2S_ENCODER2,
	MUX_I2S_2_BCLK_SYNTH_REF_MUX,
	MUX_I2S_2_BCLK_SYNTH_REF,
	MUX_BCLK_I2S_ENCODER2_128FS_SSC_MUX,
	MUX_BCLK_I2S_ENCODER2_64FS_SSC_MUX,
	MUX_BCLK_I2S_ENCODER2_32FS_SSC_MUX,
	MUX_BCLK_I2S_ENCODER2_MUX,
	MUX_BCLK_I2S_ENCODER2,
	MUX_I2S_TRX_SYNTH_REF,
	MUX_I2S_TRX_BCLK_TIMING_GEN,
	MUX_PCM_CLK_216,
	MUX_PCM_CLK_AUPLL,
	MUX_SPDIF_TX_GPA_SYNTH_256FS_MUX,
	MUX_SPDIF_TX_256FS_MUX,
	MUX_SPDIF_TX_256FS,
	MUX_SPDIF_TX_GPA_128FS_MUX,
	MUX_SPDIF_TX_128FS_MUX,
	MUX_SPDIF_TX_128FS,
	MUX_SPDIF_TX_SYNTH_REF,
	MUX_SPDIF_TX2_GPA_SYNTH_256FS_MUX,
	MUX_SPDIF_TX2_256FS_MUX,
	MUX_SPDIF_TX2_256FS,
	MUX_SPDIF_TX2_GPA_128FS_MUX,
	MUX_SPDIF_TX2_128FS_MUX,
	MUX_SPDIF_TX2_128FS,
	MUX_NPCM_SYNTH_REF,
	MUX_NPCM2_SYNTH_REF,
	MUX_NPCM3_SYNTH_REF,
	MUX_NPCM4_SYNTH_REF,
	MUX_NPCM4_256FS_TIMING_GEN,
	MUX_NPCM5_SYNTH_REF,
	MUX_NPCM5_256FS_TIMING_GEN,
	MUX_HDMI_TX_256FS_MUX1,
	MUX_HDMI_TX_256FS_MUX0,
	MUX_HDMI_TX_256FS_MUX,
	MUX_HDMI_TX_256FS,
	MUX_HDMI_TX_128FS,
	MUX_SPDIF_EXTEARC_256FS_MUX,
	MUX_SPDIF_EXTEARC_128FS_MUX,
	MUX_SPDIF_EXTEARC_MUX0,
	MUX_SPDIF_EXTEARC_MUX,
	MUX_SPDIF_EXTEARC,
	MUX_EARC_TX_SYNTH_REF,
	MUX_EARC_TX_GPA_SYNTH_REF,
	MUX_EARC_TX_SYNTH_IIR_REF,
	MUX_EARC_TX_GPA_256FS_MUX,
	MUX_EARC_TX_PLL_256FS_REF_MUX,
	MUX_EARC_TX_PLL_256FS_REF,
	MUX_EARC_TX_PLL,
	MUX_EARC_TX_64XCHXFS,
	MUX_EARC_TX_32XCHXFS,
	MUX_EARC_TX_32XFS,
	MUX_EARC_TX_SOURCE_256FS_MUX,
	MUX_EARC_TX_SOURCE_256FS,
	MUX_SIN_GEN,
	MUX_TEST_BUS,
	MUX_SRC_MAC_BSPLINE,
	MUX_I2S_TDM_6X16TX32B_BCLK,
	MUX_I2S_TDM_6X16TX32B_FIFO,
	MUX_I2S_TDM_6X16TX32B_BCLK_TO_PAD,
	MUX_I2S_TDM_1X16TX32B_BCLK,
	MUX_I2S_TDM_1X16TX32B_FIFO,
	MUX_I2S_TDM_1X16TX32B_BCLK_TO_PAD,
	MUX_CLK_NUM,
};

static const char *mux_clk[MUX_CLK_NUM] = {
	[MUX_RIU_BRG_R2] = "viva_riu_brg_r2_int_ck",
	[MUX_RIU_R2] = "viva_riu_r2_int_ck",
	[MUX_AU_MCU] = "viva_au_mcu_int_ck",
	[MUX_AU_MCU_BUS] = "au_mcu_bus_int_ck",
	[MUX_SYNTH_CODEC] = "viva_synth_codec_int_ck",
	[MUX_SYNTH_2ND_ORDER] = "viva_synth_2nd_order_int_ck",
	[MUX_AUPLL_IN] = "viva_aupll_in_int_ck",
	[MUX_DSP_INT] = "viva_dsp_int_ck",
	[MUX_SIF_ADC_R2B_FIFO_IN] = "viva_sif_adc_r2b_fifo_in_int_ck",
	[MUX_SIF_ADC_R2B] = "viva_sif_adc_r2b_int_ck",
	[MUX_SIF_ADC_CIC] = "viva_sif_adc_cic_int_ck",
	[MUX_FM_DEMODULATOR] = "viva_fm_demodulator_int_ck",
	[MUX_SIF_SYNTH] = "viva_sif_synth_int_ck",
	[MUX_SIF_CORDIC] = "viva_sif_cordic_int_ck",
	[MUX_PARSER_INT] = "viva_parser_int_ck",
	[MUX_GPA_480FSO] = "viva_gpa_480fso_int_ck",
	[MUX_GPA_240FSO] = "viva_gpa_240fso_int_ck",
	[MUX_GPA_256FSO] = "viva_gpa_256fso_int_ck",
	[MUX_ADC_512FS] = "viva_adc_512fs_int_ck",
	[MUX_DAC_512FS] = "viva_dac_512fs_int_ck",
	[MUX_AB_MAC] = "viva_ab_mac_int_ck",
	[MUX_AB3_MAC] = "viva_ab3_mac_int_ck",
	[MUX_SH] = "viva_sh_int_ck",
	[MUX_CODEC_SH] = "viva_codec_sh_int_ck",
	[MUX_PCM_I2S_256FS_MUX] = "pcm_i2s_256fs_mux_int_ck",
	[MUX_CH_M1_256FSI_MUX1] = "ch_m1_256fsi_mux1_int_ck",
	[MUX_CH_M1_256FSI_MUX0] = "ch_m1_256fsi_mux0_int_ck",
	[MUX_CH_M1_256FSI_MUX] = "ch_m1_256fsi_mux_int_ck",
	[MUX_CH_M1_256FSI] = "viva_ch_m1_256fsi_int_ck",
	[MUX_CH_M2_256FSI_MUX1] = "ch_m2_256fsi_mux1_int_ck",
	[MUX_CH_M2_256FSI_MUX0] = "ch_m2_256fsi_mux0_int_ck",
	[MUX_CH_M2_256FSI_MUX] = "ch_m2_256fsi_mux_int_ck",
	[MUX_CH_M2_256FSI] = "viva_ch_m2_256fsi_int_ck",
	[MUX_CH_M3_256FSI_MUX1] = "ch_m3_256fsi_mux1_int_ck",
	[MUX_CH_M3_256FSI_MUX0] = "ch_m3_256fsi_mux0_int_ck",
	[MUX_CH_M3_256FSI_MUX] = "ch_m3_256fsi_mux_int_ck",
	[MUX_CH_M3_256FSI] = "viva_ch_m3_256fsi_int_ck",
	[MUX_CH_M4_256FSI_MUX1] = "ch_m4_256fsi_mux1_int_ck",
	[MUX_CH_M4_256FSI_MUX0] = "ch_m4_256fsi_mux0_int_ck",
	[MUX_CH_M4_256FSI_MUX] = "ch_m4_256fsi_mux_int_ck",
	[MUX_CH_M4_256FSI] = "viva_ch_m4_256fsi_int_ck",
	[MUX_CH_M5_256FSI_MUX1] = "ch_m5_256fsi_mux1_int_ck",
	[MUX_CH_M5_256FSI_MUX0] = "ch_m5_256fsi_mux0_int_ck",
	[MUX_CH_M5_256FSI_MUX] = "ch_m5_256fsi_mux_int_ck",
	[MUX_CH_M5_256FSI] = "viva_ch_m5_256fsi_int_ck",
	[MUX_CH_M6_256FSI_MUX1] = "ch_m6_256fsi_mux1_int_ck",
	[MUX_CH_M6_256FSI_MUX0] = "ch_m6_256fsi_mux0_int_ck",
	[MUX_CH_M6_256FSI_MUX] = "ch_m6_256fsi_mux_int_ck",
	[MUX_CH_M6_256FSI] = "viva_ch_m6_256fsi_int_ck",
	[MUX_CH_M7_256FSI_MUX1] = "ch_m7_256fsi_mux1_int_ck",
	[MUX_CH_M7_256FSI_MUX0] = "ch_m7_256fsi_mux0_int_ck",
	[MUX_CH_M7_256FSI_MUX] = "ch_m7_256fsi_mux_int_ck",
	[MUX_CH_M7_256FSI] = "viva_ch_m7_256fsi_int_ck",
	[MUX_CH_M8_256FSI_MUX1] = "ch_m8_256fsi_mux1_int_ck",
	[MUX_CH_M8_256FSI_MUX0] = "ch_m8_256fsi_mux0_int_ck",
	[MUX_CH_M8_256FSI_MUX] = "ch_m8_256fsi_mux_int_ck",
	[MUX_CH_M8_256FSI] = "viva_ch_m8_256fsi_int_ck",
	[MUX_CH_M9_256FSI_MUX1] = "ch_m9_256fsi_mux1_int_ck",
	[MUX_CH_M9_256FSI_MUX0] = "ch_m9_256fsi_mux0_int_ck",
	[MUX_CH_M9_256FSI_MUX] = "ch_m9_256fsi_mux_int_ck",
	[MUX_CH_M9_256FSI] = "viva_ch_m9_256fsi_int_ck",
	[MUX_CH_M10_256FSI_MUX1] = "ch_m10_256fsi_mux1_int_ck",
	[MUX_CH_M10_256FSI_MUX0] = "ch_m10_256fsi_mux0_int_ck",
	[MUX_CH_M10_256FSI_MUX] = "ch_m10_256fsi_mux_int_ck",
	[MUX_CH_M10_256FSI] = "viva_ch_m10_256fsi_int_ck",
	[MUX_CH_M11_256FSI_MUX1] = "ch_m11_256fsi_mux1_int_ck",
	[MUX_CH_M11_256FSI_MUX0] = "ch_m11_256fsi_mux0_int_ck",
	[MUX_CH_M11_256FSI_MUX] = "ch_m11_256fsi_mux_int_ck",
	[MUX_CH_M11_256FSI] = "viva_ch_m11_256fsi_int_ck",
	[MUX_CH_M12_256FSI_MUX1] = "ch_m12_256fsi_mux1_int_ck",
	[MUX_CH_M12_256FSI_MUX0] = "ch_m12_256fsi_mux0_int_ck",
	[MUX_CH_M12_256FSI_MUX] = "ch_m12_256fsi_mux_int_ck",
	[MUX_CH_M12_256FSI] = "viva_ch_m12_256fsi_int_ck",
	[MUX_CH_M13_256FSI_MUX1] = "ch_m13_256fsi_mux1_int_ck",
	[MUX_CH_M13_256FSI_MUX0] = "ch_m13_256fsi_mux0_int_ck",
	[MUX_CH_M13_256FSI_MUX] = "ch_m13_256fsi_mux_int_ck",
	[MUX_CH_M13_256FSI] = "viva_ch_m13_256fsi_int_ck",
	[MUX_CH_M14_256FSI_MUX1] = "ch_m14_256fsi_mux1_int_ck",
	[MUX_CH_M14_256FSI_MUX0] = "ch_m14_256fsi_mux0_int_ck",
	[MUX_CH_M14_256FSI_MUX] = "ch_m14_256fsi_mux_int_ck",
	[MUX_CH_M14_256FSI] = "viva_ch_m14_256fsi_int_ck",
	[MUX_CH_M15_256FSI_MUX1] = "ch_m15_256fsi_mux1_int_ck",
	[MUX_CH_M15_256FSI_MUX0] = "ch_m15_256fsi_mux0_int_ck",
	[MUX_CH_M15_256FSI_MUX] = "ch_m15_256fsi_mux_int_ck",
	[MUX_CH_M15_256FSI] = "viva_ch_m15_256fsi_int_ck",
	[MUX_CH_M16_256FSI_MUX1] = "ch_m16_256fsi_mux1_int_ck",
	[MUX_CH_M16_256FSI_MUX0] = "ch_m16_256fsi_mux0_int_ck",
	[MUX_CH_M16_256FSI_MUX] = "ch_m16_256fsi_mux_int_ck",
	[MUX_CH_M16_256FSI] = "viva_ch_m16_256fsi_int_ck",
	[MUX_CH5_256FSI_MUX2] = "ch5_256fsi_mux2_int_ck",
	[MUX_CH5_256FSI_MUX1] = "ch5_256fsi_mux1_int_ck",
	[MUX_CH5_256FSI_MUX0] = "ch5_256fsi_mux0_int_ck",
	[MUX_CH5_256FSI_MUX] = "ch5_256fsi_mux_int_ck",
	[MUX_CH5_256FSI] = "viva_ch5_256fsi_int_ck",
	[MUX_CH6_256FSI_MUX2] = "ch6_256fsi_mux2_int_ck",
	[MUX_CH6_256FSI_MUX1] = "ch6_256fsi_mux1_int_ck",
	[MUX_CH6_256FSI_MUX0] = "ch6_256fsi_mux0_int_ck",
	[MUX_CH6_256FSI_MUX] = "ch6_256fsi_mux_int_ck",
	[MUX_CH6_256FSI] = "viva_ch6_256fsi_int_ck",
	[MUX_CH7_256FSI_MUX2] = "ch7_256fsi_mux2_int_ck",
	[MUX_CH7_256FSI_MUX1] = "ch7_256fsi_mux1_int_ck",
	[MUX_CH7_256FSI_MUX0] = "ch7_256fsi_mux0_int_ck",
	[MUX_CH7_256FSI_MUX] = "ch7_256fsi_mux_int_ck",
	[MUX_CH7_256FSI] = "viva_ch7_256fsi_int_ck",
	[MUX_CH8_256FSI_MUX2] = "ch8_256fsi_mux2_int_ck",
	[MUX_CH8_256FSI_MUX1] = "ch8_256fsi_mux1_int_ck",
	[MUX_CH8_256FSI_MUX0] = "ch8_256fsi_mux0_int_ck",
	[MUX_CH8_256FSI_MUX] = "ch8_256fsi_mux_int_ck",
	[MUX_CH8_256FSI] = "viva_ch8_256fsi_int_ck",
	[MUX_CH9_256FSI_MUX2] = "ch9_256fsi_mux2_int_ck",
	[MUX_CH9_256FSI_MUX1] = "ch9_256fsi_mux1_int_ck",
	[MUX_CH9_256FSI_MUX0] = "ch9_256fsi_mux0_int_ck",
	[MUX_CH9_256FSI_MUX] = "ch9_256fsi_mux_int_ck",
	[MUX_CH9_256FSI] = "viva_ch9_256fsi_int_ck",
	[MUX_CH10_256FSI_MUX2] = "ch10_256fsi_mux2_int_ck",
	[MUX_CH10_256FSI_MUX1] = "ch10_256fsi_mux1_int_ck",
	[MUX_CH10_256FSI_MUX0] = "ch10_256fsi_mux0_int_ck",
	[MUX_CH10_256FSI_MUX] = "ch10_256fsi_mux_int_ck",
	[MUX_CH10_256FSI] = "viva_ch10_256fsi_int_ck",
	[MUX_DVB1_SYNTH_REF] = "viva_dvb1_synth_ref_int_ck",
	[MUX_DECODER1_SYNTH_HDMI_CTSN_256FS] = "decoder1_synth_hdmi_ctsn_256fs_div_mux_int_ck",
	[MUX_DECODER1_SYNTH_HDMI_CTSN_128FS] = "decoder1_synth_hdmi_ctsn_128fs_div_mux_int_ck",
	[MUX_DECODER1_SYNTH_HDMI2_CTSN_256FS] = "decoder1_synth_hdmi2_ctsn_256fs_div_mux_int_ck",
	[MUX_DECODER1_SYNTH_HDMI2_CTSN_128FS] = "decoder1_synth_hdmi2_ctsn_128fs_div_mux_int_ck",
	[MUX_DECODER1_SYNTH_SPDIF_CDR_256FS] = "decoder1_synth_spdif_cdr_256fs_div_mux_int_ck",
	[MUX_DECODER1_SYNTH_SPDIF_CDR_128FS] = "decoder1_synth_spdif_cdr_128fs_div_mux_int_ck",
	[MUX_DECODER1_256FS_MUX0] = "decoder1_256fs_mux0_int_ck",
	[MUX_DECODER1_256FS_MUX1] = "decoder1_256fs_mux1_int_ck",
	[MUX_DECODER1_256FS_MUX] = "decoder1_256fs_mux_int_ck",
	[MUX_DVB1_TIMING_GEN_256FS] = "viva_dvb1_timing_gen_256fs_int_ck",
	[MUX_DECODER1_128FS_MUX0] = "decoder1_128fs_mux0_int_ck",
	[MUX_DECODER1_128FS_MUX1] = "decoder1_128fs_mux1_int_ck",
	[MUX_DECODER1_128FS_MUX] = "decoder1_128fs_mux_int_ck",
	[MUX_DVB2_SYNTH_REF] = "viva_dvb2_synth_ref_int_ck",
	[MUX_DECODER2_SYNTH_HDMI_CTSN_256FS] = "decoder2_synth_hdmi_ctsn_256fs_div_mux_int_ck",
	[MUX_DECODER2_SYNTH_HDMI_CTSN_128FS] = "decoder2_synth_hdmi_ctsn_128fs_div_mux_int_ck",
	[MUX_DECODER2_SYNTH_HDMI2_CTSN_256FS] = "decoder2_synth_hdmi2_ctsn_256fs_div_mux_int_ck",
	[MUX_DECODER2_SYNTH_HDMI2_CTSN_128FS] = "decoder2_synth_hdmi2_ctsn_128fs_div_mux_int_ck",
	[MUX_DECODER2_SYNTH_SPDIF_CDR_256FS] = "decoder2_synth_spdif_cdr_256fs_div_mux_int_ck",
	[MUX_DECODER2_SYNTH_SPDIF_CDR_128FS] = "decoder2_synth_spdif_cdr_128fs_div_mux_int_ck",
	[MUX_DECODER2_256FS_MUX0] = "decoder2_256fs_mux0_int_ck",
	[MUX_DECODER2_256FS_MUX1] = "decoder2_256fs_mux1_int_ck",
	[MUX_DECODER2_256FS_MUX] = "decoder2_256fs_mux_int_ck",
	[MUX_DVB2_TIMING_GEN_256FS] = "viva_dvb2_timing_gen_256fs_int_ck",
	[MUX_DECODER2_128FS_MUX0] = "decoder2_128fs_mux0_int_ck",
	[MUX_DECODER2_128FS_MUX1] = "decoder2_128fs_mux1_int_ck",
	[MUX_DECODER2_128FS_MUX] = "decoder2_128fs_mux_int_ck",
	[MUX_DVB3_SYNTH_REF] = "viva_dvb3_synth_ref_int_ck",
	[MUX_DECODER3_SYNTH_HDMI_CTSN_256FS] = "decoder3_synth_hdmi_ctsn_256fs_div_mux_int_ck",
	[MUX_DECODER3_SYNTH_HDMI_CTSN_128FS] = "decoder3_synth_hdmi_ctsn_128fs_div_mux_int_ck",
	[MUX_DECODER3_SYNTH_HDMI2_CTSN_256FS] = "decoder3_synth_hdmi2_ctsn_256fs_div_mux_int_ck",
	[MUX_DECODER3_SYNTH_HDMI2_CTSN_128FS] = "decoder3_synth_hdmi2_ctsn_128fs_div_mux_int_ck",
	[MUX_DECODER3_SYNTH_SPDIF_CDR_256FS] = "decoder3_synth_spdif_cdr_256fs_div_mux_int_ck",
	[MUX_DECODER3_SYNTH_SPDIF_CDR_128FS] = "decoder3_synth_spdif_cdr_128fs_div_mux_int_ck",
	[MUX_DECODER3_256FS_MUX0] = "decoder3_256fs_mux0_int_ck",
	[MUX_DECODER3_256FS_MUX1] = "decoder3_256fs_mux1_int_ck",
	[MUX_DECODER3_256FS_MUX] = "decoder3_256fs_mux_int_ck",
	[MUX_DVB3_TIMING_GEN_256FS] = "viva_dvb3_timing_gen_256fs_int_ck",
	[MUX_DECODER3_128FS_MUX0] = "decoder3_128fs_mux0_int_ck",
	[MUX_DECODER3_128FS_MUX1] = "decoder3_128fs_mux1_int_ck",
	[MUX_DECODER3_128FS_MUX] = "decoder3_128fs_mux_int_ck",
	[MUX_DVB4_SYNTH_REF] = "viva_dvb4_synth_ref_int_ck",
	[MUX_DECODER4_SYNTH_HDMI_CTSN_256FS] = "decoder4_synth_hdmi_ctsn_256fs_div_mux_int_ck",
	[MUX_DECODER4_SYNTH_HDMI_CTSN_128FS] = "decoder4_synth_hdmi_ctsn_128fs_div_mux_int_ck",
	[MUX_DECODER4_SYNTH_HDMI2_CTSN_256FS] = "decoder4_synth_hdmi2_ctsn_256fs_div_mux_int_ck",
	[MUX_DECODER4_SYNTH_HDMI2_CTSN_128FS] = "decoder4_synth_hdmi2_ctsn_128fs_div_mux_int_ck",
	[MUX_DECODER4_SYNTH_SPDIF_CDR_256FS] = "decoder4_synth_spdif_cdr_256fs_div_mux_int_ck",
	[MUX_DECODER4_SYNTH_SPDIF_CDR_128FS] = "decoder4_synth_spdif_cdr_128fs_div_mux_int_ck",
	[MUX_DECODER4_256FS_MUX0] = "decoder4_256fs_mux0_int_ck",
	[MUX_DECODER4_256FS_MUX1] = "decoder4_256fs_mux1_int_ck",
	[MUX_DECODER4_256FS_MUX] = "decoder4_256fs_mux_int_ck",
	[MUX_DVB4_TIMING_GEN_256FS] = "viva_dvb4_timing_gen_256fs_int_ck",
	[MUX_DECODER4_128FS_MUX0] = "decoder4_128fs_mux0_int_ck",
	[MUX_DECODER4_128FS_MUX1] = "decoder4_128fs_mux1_int_ck",
	[MUX_DECODER4_128FS_MUX] = "decoder4_128fs_mux_int_ck",
	[MUX_DVB5_TIMING_GEN_256FS] = "viva_dvb5_timing_gen_256fs_int_ck",
	[MUX_DVB6_TIMING_GEN_256FS] = "viva_dvb6_timing_gen_256fs_int_ck",
	[MUX_AUDMA_V2_R1_SYNTH_REF] = "viva_audma_v2_r1_synth_ref_int_ck",
	[MUX_AUDMA_V2_R1_256FS_MUX] = "viva_audma_v2_r1_256fs_mux_int_ck",
	[MUX_AUDMA_V2_R1_256FS] = "viva_audma_v2_r1_256fs_int_ck",
	[MUX_AUDMA_V2_R2_SYNTH_REF] = "viva_audma_v2_r2_synth_ref_int_ck",
	[MUX_AUDMA_V2_R2_256FS] = "viva_audma_v2_r2_256fs_int_ck",
	[MUX_SPDIF_RX_128FS] = "viva_spdif_rx_128fs_int_ck",
	[MUX_SPDIF_RX_DG] = "viva_spdif_rx_dg_int_ck",
	[MUX_SPDIF_RX_SYNTH_REF] = "viva_spdif_rx_synth_ref_int_ck",
	[MUX_HDMI_RX_SYNTH_REF] = "viva_hdmi_rx_synth_ref_int_ck",
	[MUX_HDMI_RX_S2P_256FS_MUX] = "hdmi_rx_s2p_256fs_mux_int_ck",
	[MUX_HDMI_RX_S2P_256FS] = "viva_hdmi_rx_s2p_256fs_int_ck",
	[MUX_HDMI_RX_S2P_128FS_MUX] = "hdmi_rx_s2p_128fs_mux_int_ck",
	[MUX_HDMI_RX_S2P_128FS] = "viva_hdmi_rx_s2p_128fs_int_ck",
	[MUX_HDMI_RX_MPLL_432] = "viva_hdmi_rx_mpll_432_int_ck",
	[MUX_HDMI_RX2_SYNTH_REF] = "viva_hdmi_rx2_synth_ref_int_ck",
	[MUX_HDMI_RX2_S2P_256FS_MUX] = "hdmi_rx2_s2p_256fs_mux_int_ck",
	[MUX_HDMI_RX2_S2P_256FS] = "viva_hdmi_rx2_s2p_256fs_int_ck",
	[MUX_HDMI_RX2_S2P_128FS_MUX] = "hdmi_rx2_s2p_128fs_mux_int_ck",
	[MUX_HDMI_RX2_S2P_128FS] = "viva_hdmi_rx2_s2p_128fs_int_ck",
	[MUX_HDMI_RX2_MPLL_432] = "viva_hdmi_rx2_mpll_432_int_ck",
	[MUX_BCLK_I2S_DECODER] = "viva_bclk_i2s_decoder_int_ck",
	[MUX_MCLK_I2S_DECODER] = "viva_mclk_i2s_decoder_int_ck",
	[MUX_I2S_BCLK_DG] = "viva_i2s_bclk_dg_int_ck",
	[MUX_I2S_SYNTH_REF] = "viva_i2s_synth_ref_int_ck",
	[MUX_BCLK_I2S_DECODER2] = "viva_bclk_i2s_decoder2_int_ck",
	[MUX_MCLK_I2S_DECODER2] = "viva_mclk_i2s_decoder2_int_ck",
	[MUX_I2S_BCLK_DG2] = "viva_i2s_bclk_dg2_int_ck",
	[MUX_BI2S_SYNTH_REF2] = "viva_bi2s_synth_ref2_int_ck",
	[MUX_HDMI_TX_TO_I2S_TX_256FS_MUX] = "hdmi_tx_to_i2s_tx_256fs_mux_int_ck",
	[MUX_HDMI_TX_TO_I2S_TX_128FS_MUX] = "hdmi_tx_to_i2s_tx_128fs_mux_int_ck",
	[MUX_HDMI_TX_TO_I2S_TX_64FS_MUX] = "hdmi_tx_to_i2s_tx_64fs_mux_int_ck",
	[MUX_HDMI_TX_TO_I2S_TX_32FS_MUX] = "hdmi_tx_to_i2s_tx_32fs_mux_int_ck",
	[MUX_HDMI_RX_TO_I2S_TX_256FS_MUX] = "hdmi_rx_to_i2s_tx_256fs_mux_int_ck",
	[MUX_HDMI_RX_TO_I2S_TX_128FS_MUX] = "hdmi_rx_to_i2s_tx_128fs_mux_int_ck",
	[MUX_HDMI_RX_TO_I2S_TX_64FS_MUX] = "hdmi_rx_to_i2s_tx_64fs_mux_int_ck",
	[MUX_HDMI_RX_TO_I2S_TX_32FS_MUX] = "hdmi_rx_to_i2s_tx_32fs_mux_int_ck",
	[MUX_I2S_TX_FIFO_256FS] = "viva_i2s_tx_fifo_256fs_int_ck",
	[MUX_I2S_MCLK_SYNTH_REF] = "viva_i2s_mclk_synth_ref_int_ck",
	[MUX_MCLK_I2S_ENCODER] = "viva_mclk_i2s_encoder_int_ck",
	[MUX_I2S_BCLK_SYNTH_REF_MUX] = "i2s_bclk_synth_ref_mux_int_ck",
	[MUX_I2S_BCLK_SYNTH_REF] = "viva_i2s_bclk_synth_ref_int_ck",
	[MUX_BCLK_I2S_ENCODER_128FS_SSC_MUX] = "bclk_i2s_encoder_128fs_ssc_mux_int_ck",
	[MUX_BCLK_I2S_ENCODER_64FS_SSC_MUX] = "bclk_i2s_encoder_64fs_ssc_mux_int_ck",
	[MUX_BCLK_I2S_ENCODER_32FS_SSC_MUX] = "bclk_i2s_encoder_32fs_ssc_mux_int_ck",
	[MUX_BCLK_I2S_ENCODER_MUX] = "bclk_i2s_encoder_mux_int_ck",
	[MUX_BCLK_I2S_ENCODER] = "bclk_i2s_encoder_int_ck",
	[MUX_I2S_TX_2_FIFO_256FS] = "viva_i2s_tx_2_fifo_256fs_int_ck",
	[MUX_MCLK_I2S_ENCODER2] = "viva_mclk_i2s_encoder2_int_ck",
	[MUX_I2S_2_BCLK_SYNTH_REF_MUX] = "i2s_2_bclk_synth_ref_mux_int_ck",
	[MUX_I2S_2_BCLK_SYNTH_REF] = "viva_i2s_2_bclk_synth_ref_int_ck",
	[MUX_BCLK_I2S_ENCODER2_128FS_SSC_MUX] = "bclk_i2s_encoder2_128fs_ssc_mux_int_ck",
	[MUX_BCLK_I2S_ENCODER2_64FS_SSC_MUX] = "bclk_i2s_encoder2_64fs_ssc_mux_int_ck",
	[MUX_BCLK_I2S_ENCODER2_32FS_SSC_MUX] = "bclk_i2s_encoder2_32fs_ssc_mux_int_ck",
	[MUX_BCLK_I2S_ENCODER2_MUX] = "bclk_i2s_encoder2_mux_int_ck",
	[MUX_BCLK_I2S_ENCODER2] = "bclk_i2s_encoder2_int_ck",
	[MUX_I2S_TRX_SYNTH_REF] = "viva_i2s_trx_synth_ref_int_ck",
	[MUX_I2S_TRX_BCLK_TIMING_GEN] = "viva_i2s_trx_bclk_timing_gen_int_ck",
	[MUX_PCM_CLK_216] = "viva_pcm_clk_216_int_ck",
	[MUX_PCM_CLK_AUPLL] = "viva_pcm_clk_aupll_int_ck",
	[MUX_SPDIF_TX_GPA_SYNTH_256FS_MUX] = "spdif_tx_gpa_synth_256fs_mux_int_ck",
	[MUX_SPDIF_TX_256FS_MUX] = "spdif_tx_256fs_mux_int_ck",
	[MUX_SPDIF_TX_256FS] = "viva_spdif_tx_256fs_int_ck",
	[MUX_SPDIF_TX_GPA_128FS_MUX] = "spdif_tx_gpa_128fs_mux_int_ck",
	[MUX_SPDIF_TX_128FS_MUX] = "spdif_tx_128fs_mux_int_ck",
	[MUX_SPDIF_TX_128FS] = "viva_spdif_tx_128fs_int_ck",
	[MUX_SPDIF_TX_SYNTH_REF] = "viva_spdif_tx_synth_ref_int_ck",
	[MUX_SPDIF_TX2_GPA_SYNTH_256FS_MUX] = "spdif_tx2_gpa_synth_256fs_mux_int_ck",
	[MUX_SPDIF_TX2_256FS_MUX] = "spdif_tx2_256fs_mux_int_ck",
	[MUX_SPDIF_TX2_256FS] = "viva_spdif_tx2_256fs_int_ck",
	[MUX_SPDIF_TX2_GPA_128FS_MUX] = "spdif_tx2_gpa_128fs_mux_int_ck",
	[MUX_SPDIF_TX2_128FS_MUX] = "spdif_tx2_128fs_mux_int_ck",
	[MUX_SPDIF_TX2_128FS] = "viva_spdif_tx2_128fs_int_ck",
	[MUX_NPCM_SYNTH_REF] = "viva_npcm_synth_ref_int_ck",
	[MUX_NPCM2_SYNTH_REF] = "viva_npcm2_synth_ref_int_ck",
	[MUX_NPCM3_SYNTH_REF] = "viva_npcm3_synth_ref_int_ck",
	[MUX_NPCM4_SYNTH_REF] = "viva_npcm4_synth_ref_int_ck",
	[MUX_NPCM4_256FS_TIMING_GEN] = "viva_npcm4_256fs_timing_gen_int_ck",
	[MUX_NPCM5_SYNTH_REF] = "viva_npcm5_synth_ref_int_ck",
	[MUX_NPCM5_256FS_TIMING_GEN] = "viva_npcm5_256fs_timing_gen_int_ck",
	[MUX_HDMI_TX_256FS_MUX1] = "hdmi_tx_256fs_mux1_int_ck",
	[MUX_HDMI_TX_256FS_MUX0] = "hdmi_tx_256fs_mux0_int_ck",
	[MUX_HDMI_TX_256FS_MUX] = "hdmi_tx_256fs_mux_int_ck",
	[MUX_HDMI_TX_256FS] = "viva_hdmi_tx_256fs_int_ck",
	[MUX_HDMI_TX_128FS] = "viva_hdmi_tx_128fs_int_ck",
	[MUX_SPDIF_EXTEARC_256FS_MUX] = "spdif_extearc_256fs_mux_int_ck",
	[MUX_SPDIF_EXTEARC_128FS_MUX] = "spdif_extearc_128fs_mux_int_ck",
	[MUX_SPDIF_EXTEARC_MUX0] = "spdif_extearc_mux0_int_ck",
	[MUX_SPDIF_EXTEARC_MUX] = "spdif_extearc_mux_int_ck",
	[MUX_SPDIF_EXTEARC] = "viva_spdif_extearc_int_ck",
	[MUX_EARC_TX_SYNTH_REF] = "viva_earc_tx_synth_ref_int_ck",
	[MUX_EARC_TX_GPA_SYNTH_REF] = "viva_earc_tx_gpa_synth_ref_int_ck",
	[MUX_EARC_TX_SYNTH_IIR_REF] = "viva_earc_tx_synth_iir_ref_int_ck",
	[MUX_EARC_TX_GPA_256FS_MUX] = "earc_tx_gpa_256fs_mux_int_ck",
	[MUX_EARC_TX_PLL_256FS_REF_MUX] = "earc_tx_pll_256fs_ref_mux_int_ck",
	[MUX_EARC_TX_PLL_256FS_REF] = "earc_tx_pll_256fs_ref_int_ck",
	[MUX_EARC_TX_PLL] = "viva_earc_tx_pll_int_ck",
	[MUX_EARC_TX_64XCHXFS] = "viva_earc_tx_64xchxfs_int_ck",
	[MUX_EARC_TX_32XCHXFS] = "viva_earc_tx_32xchxfs_int_ck",
	[MUX_EARC_TX_32XFS] = "viva_earc_tx_32xfs_int_ck",
	[MUX_EARC_TX_SOURCE_256FS_MUX] = "earc_tx_source_256fs_mux_int_ck",
	[MUX_EARC_TX_SOURCE_256FS] = "viva_earc_tx_source_256fs_int_ck",
	[MUX_SIN_GEN] = "viva_sin_gen_int_ck",
	[MUX_TEST_BUS] = "viva_test_bus_int_ck",
	[MUX_SRC_MAC_BSPLINE] = "viva_src_mac_bspline_int_ck",
	[MUX_I2S_TDM_6X16TX32B_BCLK] = "viva_i2s_tdm_6x16tx32b_bclk_int_ck",
	[MUX_I2S_TDM_6X16TX32B_FIFO] = "viva_i2s_tdm_6x16tx32b_fifo_int_ck",
	[MUX_I2S_TDM_6X16TX32B_BCLK_TO_PAD] = "viva_i2s_tdm_6x16tx32b_bclk_to_pad_int_ck",
	[MUX_I2S_TDM_1X16TX32B_BCLK] = "viva_i2s_tdm_1x16tx32b_bclk_int_ck",
	[MUX_I2S_TDM_1X16TX32B_FIFO] = "viva_i2s_tdm_1x16tx32b_fifo_int_ck",
	[MUX_I2S_TDM_1X16TX32B_BCLK_TO_PAD] = "viva_i2s_tdm_1x16tx32b_bclk_to_pad_int_ck",
};

//--------------------------------------------------
// SW CLOCK (IP_version 1)
//--------------------------------------------------
enum {
	CLK_BCLK_I2S_ENCODER2 = 0,
	CLK_BCLK_I2S_ENCODER,
	CLK_AB_MAC,
	CLK_AB3_MAC,
	CLK_ADC_512FS,
	CLK_AU_MCU,
	CLK_AUDMA_V2_R1_256FS,
	CLK_AUDMA_V2_R1_TIMING_GEN,
	CLK_AUDMA_V2_R1_SYNTH_REF,
	CLK_AUDMA_V2_R2_256FS,
	CLK_AUDMA_V2_R2_TIMING_GEN,
	CLK_AUDMA_V2_R2_SYNTH_REF,
	CLK_BCLK_I2S_DECODER2,
	CLK_BCLK_I2S_DECODER,
	CLK_I2S_SYNTH_REF2,
	CLK_CH1_256FSI,
	CLK_CH2_256FSI,
	CLK_CH3_256FSI,
	CLK_CH4_256FSI,
	CLK_CH5_256FSI,
	CLK_CH6_256FSI,
	CLK_CH7_256FSI,
	CLK_CH8_256FSI,
	CLK_CH9_256FSI,
	CLK_CODEC_SH,
	CLK_DAC_512_FS,
	CLK_DSP_230,
	CLK_DSP_230_1,
	CLK_DSP_230_2,
	CLK_DSP_230_3,
	CLK_DSP_230_DFS_REF,
	CLK_DSP_230_TM,
	CLK_DVB1_SYNTH_REF,
	CLK_DVB1_TIMING_GEN_256FS,
	CLK_DVB2_SYNTH_REF,
	CLK_DVB2_TIMING_GEN_256FS,
	CLK_DVB3_SYNTH_REF,
	CLK_DVB3_TIMING_GEN_256FS,
	CLK_DVB4_SYNTH_REF,
	CLK_DVB4_TIMING_GEN_256FS,
	CLK_DVB5_TIMING_GEN_256FS,
	CLK_DVB6_TIMING_GEN_256FS,
	CLK_EARC_TX_32XCHXFS,
	CLK_EARC_TX_32XFS,
	CLK_EARC_TX_64XCHXFS,
	CLK_EARC_TX_GPA_SYNTH_REF,
	CLK_EARC_TX_PLL,
	CLK_EARC_TX_SOURCE_256FS,
	CLK_EARC_TX_SYNTH_IIR_REF,
	CLK_EARC_TX_SYNTH_REF,
	CLK_FM_DEMODULATOR,
	CLK_GPA_240FSO,
	CLK_GPA_256FSO,
	CLK_GPA_480FSO,
	CLK_HDMI_RX_MPLL_432,
	CLK_HDMI_RX_S2P_128FS,
	CLK_HDMI_RX_S2P_256FS,
	CLK_HDMI_RX_SYNTH_REF,
	CLK_HDMI_RX_V2A_TIME_STAMP,
	CLK_HDMI_RX2_MPLL_432,
	CLK_HDMI_RX2_S2P_128FS,
	CLK_HDMI_RX2_S2P_256FS,
	CLK_HDMI_RX2_SYNTH_REF,
	CLK_HDMI_RX2_V2A_TIME_STAMP,
	CLK_HDMI_TX_128FS,
	CLK_HDMI_TX_256FS,
	CLK_I2S_2_BCLK_SYNTH_REF,
	CLK_I2S_BCLK_DG2,
	CLK_I2S_BCLK_DG,
	CLK_I2S_BCLK_SYNTH_REF,
	CLK_I2S_MCLK_SYNTH_REF,
	CLK_I2S_SYNTH_REF,
	CLK_I2S_TRX_BCLK_TIMING_GEN,
	CLK_I2S_TRX_SYNTH_REF,
	CLK_I2S_TX_FIFO_256FS,
	CLK_MCLK_I2S_DECODER2,
	CLK_MCLK_I2S_DECODER,
	CLK_NPCM_SYNTH_REF,
	CLK_NPCM2_SYNTH_REF,
	CLK_NPCM3_SYNTH_REF,
	CLK_NPCM4_256FS_TIMING_GEN,
	CLK_NPCM4_SYNTH_REF,
	CLK_NPCM5_256FS_TIMING_GEN,
	CLK_NPCM5_SYNTH_REF,
	CLK_PCM_CLK_216,
	CLK_PCM_CLK_AUPLL,
	CLK_R2_WB,
	CLK_R2_RIU_BRG,
	CLK_RIU_R2,
	CLK_CH1_SH,
	CLK_CH2_SH,
	CLK_CH3_SH,
	CLK_CH4_SH,
	CLK_CH5_SH,
	CLK_CH6_SH,
	CLK_CH7_SH,
	CLK_CH8_SH,
	CLK_CH9_SH,
	CLK_SIF_ADC_CIC,
	CLK_SIF_ADC_R2B_FIFO_IN,
	CLK_SIF_ADC_R2B,
	CLK_SIF_CORDIC,
	CLK_SIF_SYNTH,
	CLK_SIN_GEN,
	CLK_SPDIF_RX_128FS,
	CLK_SPDIF_RX_DG,
	CLK_SPDIF_RX_SYNTH_REF,
	CLK_SPDIF_TX_128FS,
	CLK_SPDIF_TX_256FS,
	CLK_SPDIF_TX_SYNTH_REF,
	CLK_SPDIF_TX2_128FS,
	CLK_SPDIF_TX2_256FS,
	CLK_SYNTH_2ND_ORDER,
	CLK_SYNTH_CODEC,
	CLK_TEST_BUS,
	CLK_XTAL_12M,
	CLK_XTAL_24M,
	CLK_I2S_TX_2_FIFO_256FS,
	CLK_CH10_256FSI,
	CLK_CH_M1_256FSI,
	CLK_CH_M2_256FSI,
	CLK_CH_M3_256FSI,
	CLK_CH_M4_256FSI,
	CLK_CH_M5_256FSI,
	CLK_CH_M6_256FSI,
	CLK_CH_M7_256FSI,
	CLK_CH_M8_256FSI,
	CLK_CH_M9_256FSI,
	CLK_CH_M10_256FSI,
	CLK_CH_M11_256FSI,
	CLK_CH_M12_256FSI,
	CLK_CH_M13_256FSI,
	CLK_CH_M14_256FSI,
	CLK_CH_M15_256FSI,
	CLK_CH_M16_256FSI,
	CLK_I2S_TDM_1X16TX32B_BCLK,
	CLK_I2S_TDM_1X16TX32B_FIFO,
	CLK_I2S_TDM_6X16TX32B_BCLK,
	CLK_I2S_TDM_6X16TX32B_FIFO,
	CLK_CH10_SH,
	CLK_CH_M1_SH,
	CLK_CH_M2_SH,
	CLK_CH_M3_SH,
	CLK_CH_M4_SH,
	CLK_CH_M5_SH,
	CLK_CH_M6_SH,
	CLK_CH_M7_SH,
	CLK_CH_M8_SH,
	CLK_CH_M9_SH,
	CLK_CH_M10_SH,
	CLK_CH_M11_SH,
	CLK_CH_M12_SH,
	CLK_CH_M13_SH,
	CLK_CH_M14_SH,
	CLK_CH_M15_SH,
	CLK_CH_M16_SH,
	CLK_SRC_MAC_BSPLINE,
	CLK_NUM
};

static const char *sw_clk[CLK_NUM] = {
	[CLK_BCLK_I2S_ENCODER2] = "bclk_i2s_encoder22bclk_i2s_encoder2",
	[CLK_BCLK_I2S_ENCODER] = "bclk_i2s_encoder2bclk_i2s_encoder",
	[CLK_AB_MAC] = "viva_ab_mac2ab_mac",
	[CLK_AB3_MAC] = "viva_ab3_mac2ab3_mac",
	[CLK_ADC_512FS] = "viva_adc_512fs2adc_512_fs",
	[CLK_AU_MCU] = "viva_au_mcu2au_mcu",
	[CLK_AUDMA_V2_R1_256FS] = "viva_audma_v2_r1_256fs2audma_v2_r1_256fs",
	[CLK_AUDMA_V2_R1_TIMING_GEN] = "viva_audma_v2_r1_256fs2audma_v2_r1_timing_gen",
	[CLK_AUDMA_V2_R1_SYNTH_REF] = "viva_audma_v2_r1_synth_ref2audma_v2_r1_synth_ref",
	[CLK_AUDMA_V2_R2_256FS] = "viva_audma_v2_r2_256fs2audma_v2_r2_256fs",
	[CLK_AUDMA_V2_R2_TIMING_GEN] = "viva_audma_v2_r2_256fs2audma_v2_r2_timing_gen",
	[CLK_AUDMA_V2_R2_SYNTH_REF] = "viva_audma_v2_r2_synth_ref2audma_v2_r2_synth_ref",
	[CLK_BCLK_I2S_DECODER2] = "viva_bclk_i2s_decoder22bclk_i2s_decoder2",
	[CLK_BCLK_I2S_DECODER] = "viva_bclk_i2s_decoder2bclk_i2s_decoder",
	[CLK_I2S_SYNTH_REF2] = "viva_bi2s_synth_ref22i2s_synth_ref2",
	[CLK_CH1_256FSI] = "viva_ch1_256fsi2ch1_256fsi",
	[CLK_CH2_256FSI] = "viva_ch2_256fsi2ch2_256fsi",
	[CLK_CH3_256FSI] = "viva_ch3_256fsi2ch3_256fsi",
	[CLK_CH4_256FSI] = "viva_ch4_256fsi2ch4_256fsi",
	[CLK_CH5_256FSI] = "viva_ch5_256fsi2ch5_256fsi",
	[CLK_CH6_256FSI] = "viva_ch6_256fsi2ch6_256fsi",
	[CLK_CH7_256FSI] = "viva_ch7_256fsi2ch7_256fsi",
	[CLK_CH8_256FSI] = "viva_ch8_256fsi2ch8_256fsi",
	[CLK_CH9_256FSI] = "viva_ch9_256fsi2ch9_256fsi",
	[CLK_CODEC_SH] = "viva_codec_sh2codec_sh",
	[CLK_DAC_512_FS] = "viva_dac_512fs2dac_512_fs",
	[CLK_DSP_230] = "viva_dsp2dsp_230",
	[CLK_DSP_230_1] = "viva_dsp2dsp_230_1",
	[CLK_DSP_230_2] = "viva_dsp2dsp_230_2",
	[CLK_DSP_230_3] = "viva_dsp2dsp_230_3",
	[CLK_DSP_230_DFS_REF] = "viva_dsp2dsp_230_dfs_ref",
	[CLK_DSP_230_TM] = "viva_dsp2dsp_230_tm",
	[CLK_DVB1_SYNTH_REF] = "viva_dvb1_synth_ref2dvb1_synth_ref",
	[CLK_DVB1_TIMING_GEN_256FS] = "viva_dvb1_timing_gen_256fs2dvb1_timing_gen_256fs",
	[CLK_DVB2_SYNTH_REF] = "viva_dvb2_synth_ref2dvb2_synth_ref",
	[CLK_DVB2_TIMING_GEN_256FS] = "viva_dvb2_timing_gen_256fs2dvb2_timing_gen_256fs",
	[CLK_DVB3_SYNTH_REF] = "viva_dvb3_synth_ref2dvb3_synth_ref",
	[CLK_DVB3_TIMING_GEN_256FS] = "viva_dvb3_timing_gen_256fs2dvb3_timing_gen_256fs",
	[CLK_DVB4_SYNTH_REF] = "viva_dvb4_synth_ref2dvb4_synth_ref",
	[CLK_DVB4_TIMING_GEN_256FS] = "viva_dvb4_timing_gen_256fs2dvb4_timing_gen_256fs",
	[CLK_DVB5_TIMING_GEN_256FS] = "viva_dvb5_timing_gen_256fs2dvb5_timing_gen_256fs",
	[CLK_DVB6_TIMING_GEN_256FS] = "viva_dvb6_timing_gen_256fs2dvb6_timing_gen_256fs",
	[CLK_EARC_TX_32XCHXFS] = "viva_earc_tx_32xchxfs2earc_tx_32xchxfs",
	[CLK_EARC_TX_32XFS] = "viva_earc_tx_32xfs2earc_tx_32xfs",
	[CLK_EARC_TX_64XCHXFS] = "viva_earc_tx_64xchxfs2earc_tx_64xchxfs",
	[CLK_EARC_TX_GPA_SYNTH_REF] = "viva_earc_tx_gpa_synth_ref2earc_tx_gpa_synth_ref",
	[CLK_EARC_TX_PLL] = "viva_earc_tx_pll2earc_tx_pll",
	[CLK_EARC_TX_SOURCE_256FS] = "viva_earc_tx_source_256fs2earc_tx_source_256fs",
	[CLK_EARC_TX_SYNTH_IIR_REF] = "viva_earc_tx_synth_iir_ref2earc_tx_synth_iir_ref",
	[CLK_EARC_TX_SYNTH_REF] = "viva_earc_tx_synth_ref2earc_tx_synth_ref",
	[CLK_FM_DEMODULATOR] = "viva_fm_demodulator2fm_demodulator",
	[CLK_GPA_240FSO] = "viva_gpa_240fso2gpa_240fso",
	[CLK_GPA_256FSO] = "viva_gpa_256fso2gpa_256fso",
	[CLK_GPA_480FSO] = "viva_gpa_480fso2gpa_480fso",
	[CLK_HDMI_RX_MPLL_432] = "viva_hdmi_rx_mpll_4322hdmi_rx_mpll_432",
	[CLK_HDMI_RX_S2P_128FS] = "viva_hdmi_rx_s2p_128fs2hdmi_rx_s2p_128fs",
	[CLK_HDMI_RX_S2P_256FS] = "viva_hdmi_rx_s2p_256fs2hdmi_rx_s2p_256fs",
	[CLK_HDMI_RX_SYNTH_REF] = "viva_hdmi_rx_synth_ref2hdmi_rx_synth_ref",
	[CLK_HDMI_RX_V2A_TIME_STAMP] = "viva_hdmi_rx_v2a_time_stamp2hdmi_rx_v2a_time_stamp",
	[CLK_HDMI_RX2_MPLL_432] = "viva_hdmi_rx2_mpll_4322hdmi_rx2_mpll_432",
	[CLK_HDMI_RX2_S2P_128FS] = "viva_hdmi_rx2_s2p_128fs2hdmi_rx2_s2p_128fs",
	[CLK_HDMI_RX2_S2P_256FS] = "viva_hdmi_rx2_s2p_256fs2hdmi_rx2_s2p_256fs",
	[CLK_HDMI_RX2_SYNTH_REF] = "viva_hdmi_rx2_synth_ref2hdmi_rx2_synth_ref",
	[CLK_HDMI_RX2_V2A_TIME_STAMP] = "viva_hdmi_rx2_v2a_time_stamp2hdmi_rx2_v2a_time_stamp",
	[CLK_HDMI_TX_128FS] = "viva_hdmi_tx_128fs2hdmi_tx_128fs",
	[CLK_HDMI_TX_256FS] = "viva_hdmi_tx_256fs2hdmi_tx_256fs",
	[CLK_I2S_2_BCLK_SYNTH_REF] = "viva_i2s_2_bclk_synth_ref2i2s_2_bclk_synth_ref",
	[CLK_I2S_BCLK_DG2] = "viva_i2s_bclk_dg22i2s_bclk_dg2",
	[CLK_I2S_BCLK_DG] = "viva_i2s_bclk_dg2i2s_bclk_dg",
	[CLK_I2S_BCLK_SYNTH_REF] = "viva_i2s_bclk_synth_ref2i2s_bclk_synth_ref",
	[CLK_I2S_MCLK_SYNTH_REF] = "viva_i2s_mclk_synth_ref2i2s_mclk_synth_ref",
	[CLK_I2S_SYNTH_REF] = "viva_i2s_synth_ref2i2s_synth_ref",
	[CLK_I2S_TRX_BCLK_TIMING_GEN] = "viva_i2s_trx_bclk_timing_gen2i2s_trx_bclk_timing_gen",
	[CLK_I2S_TRX_SYNTH_REF] = "viva_i2s_trx_synth_ref2i2s_trx_synth_ref",
	[CLK_I2S_TX_FIFO_256FS] = "viva_i2s_tx_fifo_256fs2i2s_tx_fifo_256fs",
	[CLK_MCLK_I2S_DECODER2] = "viva_mclk_i2s_decoder22mclk_i2s_decoder2",
	[CLK_MCLK_I2S_DECODER] = "viva_mclk_i2s_decoder2mclk_i2s_decoder",
	[CLK_NPCM_SYNTH_REF] = "viva_npcm_synth_ref2npcm_synth_ref",
	[CLK_NPCM2_SYNTH_REF] = "viva_npcm2_synth_ref2npcm2_synth_ref",
	[CLK_NPCM3_SYNTH_REF] = "viva_npcm3_synth_ref2npcm3_synth_ref",
	[CLK_NPCM4_256FS_TIMING_GEN] = "viva_npcm4_256fs_timing_gen2npcm4_256fs_timing_gen",
	[CLK_NPCM4_SYNTH_REF] = "viva_npcm4_synth_ref2npcm4_synth_ref",
	[CLK_NPCM5_256FS_TIMING_GEN] = "viva_npcm5_256fs_timing_gen2npcm5_256fs_timing_gen",
	[CLK_NPCM5_SYNTH_REF] = "viva_npcm5_synth_ref2npcm5_synth_ref",
	[CLK_PCM_CLK_216] = "viva_pcm_clk_2162pcm_clk_216",
	[CLK_PCM_CLK_AUPLL] = "viva_pcm_clk_aupll2pcm_clk_aupll",
	[CLK_R2_WB] = "viva_r2_wb2r2_wb",
	[CLK_R2_RIU_BRG] = "viva_riu_brg_r22r2_riu_brg",
	[CLK_RIU_R2] = "viva_riu_r22riu_r2",
	[CLK_CH1_SH] = "viva_sh2ch1_sh",
	[CLK_CH2_SH] = "viva_sh2ch2_sh",
	[CLK_CH3_SH] = "viva_sh2ch3_sh",
	[CLK_CH4_SH] = "viva_sh2ch4_sh",
	[CLK_CH5_SH] = "viva_sh2ch5_sh",
	[CLK_CH6_SH] = "viva_sh2ch6_sh",
	[CLK_CH7_SH] = "viva_sh2ch7_sh",
	[CLK_CH8_SH] = "viva_sh2ch8_sh",
	[CLK_CH9_SH] = "viva_sh2ch9_sh",
	[CLK_SIF_ADC_CIC] = "viva_sif_adc_cic2sif_adc_cic",
	[CLK_SIF_ADC_R2B_FIFO_IN] = "viva_sif_adc_r2b_fifo_in2sif_adc_r2b_fifo_in",
	[CLK_SIF_ADC_R2B] = "viva_sif_adc_r2b2sif_adc_r2b",
	[CLK_SIF_CORDIC] = "viva_sif_cordic2sif_cordic",
	[CLK_SIF_SYNTH] = "viva_sif_synth2sif_synth",
	[CLK_SIN_GEN] = "viva_sin_gen2sin_gen",
	[CLK_SPDIF_RX_128FS] = "viva_spdif_rx_128fs2spdif_rx_128fs",
	[CLK_SPDIF_RX_DG] = "viva_spdif_rx_dg2spdif_rx_dg",
	[CLK_SPDIF_RX_SYNTH_REF] = "viva_spdif_rx_synth_ref2spdif_rx_synth_ref",
	[CLK_SPDIF_TX_128FS] = "viva_spdif_tx_128fs2spdif_tx_128fs",
	[CLK_SPDIF_TX_256FS] = "viva_spdif_tx_256fs2spdif_tx_256fs",
	[CLK_SPDIF_TX_SYNTH_REF] = "viva_spdif_tx_synth_ref2spdif_tx_synth_ref",
	[CLK_SPDIF_TX2_128FS] = "viva_spdif_tx2_128fs2spdif_tx2_128fs",
	[CLK_SPDIF_TX2_256FS] = "viva_spdif_tx2_256fs2spdif_tx2_256fs",
	[CLK_SYNTH_2ND_ORDER] = "viva_synth_2nd_order2synth_2nd_order",
	[CLK_SYNTH_CODEC] = "viva_synth_codec2synth_codec",
	[CLK_TEST_BUS] = "viva_test_bus2test_bus",
	[CLK_XTAL_12M] = "viva_xtal_12m2xtal",
	[CLK_XTAL_24M] = "viva_xtal_24m2xtal",
	[CLK_I2S_TX_2_FIFO_256FS] = "viva_i2s_tx_2_fifo_256fs2i2s_tx_2_fifo_256fs",
	[CLK_CH10_256FSI] = "viva_ch10_256fsi2ch10_256fsi",
	[CLK_CH_M1_256FSI] = "viva_ch_m1_256fsi2ch_m1_256fsi",
	[CLK_CH_M2_256FSI] = "viva_ch_m2_256fsi2ch_m2_256fsi",
	[CLK_CH_M3_256FSI] = "viva_ch_m3_256fsi2ch_m3_256fsi",
	[CLK_CH_M4_256FSI] = "viva_ch_m4_256fsi2ch_m4_256fsi",
	[CLK_CH_M5_256FSI] = "viva_ch_m5_256fsi2ch_m5_256fsi",
	[CLK_CH_M6_256FSI] = "viva_ch_m6_256fsi2ch_m6_256fsi",
	[CLK_CH_M7_256FSI] = "viva_ch_m7_256fsi2ch_m7_256fsi",
	[CLK_CH_M8_256FSI] = "viva_ch_m8_256fsi2ch_m8_256fsi",
	[CLK_CH_M9_256FSI] = "viva_ch_m9_256fsi2ch_m9_256fsi",
	[CLK_CH_M10_256FSI] = "viva_ch_m10_256fsi2ch_m10_256fsi",
	[CLK_CH_M11_256FSI] = "viva_ch_m11_256fsi2ch_m11_256fsi",
	[CLK_CH_M12_256FSI] = "viva_ch_m12_256fsi2ch_m12_256fsi",
	[CLK_CH_M13_256FSI] = "viva_ch_m13_256fsi2ch_m13_256fsi",
	[CLK_CH_M14_256FSI] = "viva_ch_m14_256fsi2ch_m14_256fsi",
	[CLK_CH_M15_256FSI] = "viva_ch_m15_256fsi2ch_m15_256fsi",
	[CLK_CH_M16_256FSI] = "viva_ch_m16_256fsi2ch_m16_256fsi",
	[CLK_I2S_TDM_1X16TX32B_BCLK] = "viva_i2s_tdm_1x16tx32b_bclk2viva_i2s_tdm_1x16tx32b_bclk",
	[CLK_I2S_TDM_1X16TX32B_FIFO] = "viva_i2s_tdm_1x16tx32b_fifo2viva_i2s_tdm_1x16tx32b_fifo",
	[CLK_I2S_TDM_6X16TX32B_BCLK] = "viva_i2s_tdm_6x16tx32b_bclk2i2s_tdm_6x16tx32b_bclk",
	[CLK_I2S_TDM_6X16TX32B_FIFO] = "viva_i2s_tdm_6x16tx32b_fifo2viva_i2s_tdm_6x16tx32b_fifo",
	[CLK_CH10_SH] = "viva_sh2ch10_sh",
	[CLK_CH_M1_SH] = "viva_sh2ch_m1_sh",
	[CLK_CH_M2_SH] = "viva_sh2ch_m2_sh",
	[CLK_CH_M3_SH] = "viva_sh2ch_m3_sh",
	[CLK_CH_M4_SH] = "viva_sh2ch_m4_sh",
	[CLK_CH_M5_SH] = "viva_sh2ch_m5_sh",
	[CLK_CH_M6_SH] = "viva_sh2ch_m6_sh",
	[CLK_CH_M7_SH] = "viva_sh2ch_m7_sh",
	[CLK_CH_M8_SH] = "viva_sh2ch_m8_sh",
	[CLK_CH_M9_SH] = "viva_sh2ch_m9_sh",
	[CLK_CH_M10_SH] = "viva_sh2ch_m10_sh",
	[CLK_CH_M11_SH] = "viva_sh2ch_m11_sh",
	[CLK_CH_M12_SH] = "viva_sh2ch_m12_sh",
	[CLK_CH_M13_SH] = "viva_sh2ch_m13_sh",
	[CLK_CH_M14_SH] = "viva_sh2ch_m14_sh",
	[CLK_CH_M15_SH] = "viva_sh2ch_m15_sh",
	[CLK_CH_M16_SH] = "viva_sh2ch_m16_sh",
	[CLK_SRC_MAC_BSPLINE] = "viva_src_mac_bspline2src_mac_bspline",
};

//--------------------------------------------------
// GATE CLOCK (IP_version 2)
//--------------------------------------------------
enum {
	IP2_GATE_XTAL_12M,
	IP2_GATE_XTAL_24M,
	IP2_GATE_SMI,
	IP2_GATE_RIU,
	IP2_GATE_AU_RIU,
	IP2_GATE_HDMI_RX_V2A_TIME_STAMP,
	IP2_GATE_BCLK_I2S_ENCODER_TO_PAD,
	IP2_GATE_CLK_NUM
};

static const char *ip2_gate_clk[IP2_GATE_CLK_NUM] = {
	[IP2_GATE_XTAL_12M] = "viva_xtal_12m_int_ck",
	[IP2_GATE_XTAL_24M] = "viva_xtal_24m_int_ck",
	[IP2_GATE_SMI] = "viva_smi_int_ck",
	[IP2_GATE_RIU] = "viva_riu_int_ck",
	[IP2_GATE_AU_RIU] = "viva_au_riu_int_ck",
	[IP2_GATE_HDMI_RX_V2A_TIME_STAMP] = "viva_hdmi_rx_v2a_time_stamp_int_ck",
	[IP2_GATE_BCLK_I2S_ENCODER_TO_PAD] = "bclk_i2s_encoder_to_pad_int_ck",
};

//--------------------------------------------------
// MUX CLOCK (IP_version 2)
//--------------------------------------------------
enum {
	IP2_MUX_RIU_BRG_R2,
	IP2_MUX_AU_MCU,
	IP2_MUX_AU_MCU_BUS,
	IP2_MUX_SYNTH_CODEC,
	IP2_MUX_SYNTH_2ND_ORDER,
	IP2_MUX_AUPLL_IN,
	IP2_MUX_DSP_INT,
	IP2_MUX_SIF_ADC_R2B_FIFO_IN,
	IP2_MUX_SIF_ADC_R2B,
	IP2_MUX_SIF_ADC_CIC,
	IP2_MUX_FM_DEMODULATOR,
	IP2_MUX_SIF_SYNTH,
	IP2_MUX_SIF_CORDIC,
	IP2_MUX_PARSER_INT,
	IP2_MUX_GPA_480FSO,
	IP2_MUX_GPA_240FSO,
	IP2_MUX_GPA_256FSO,
	IP2_MUX_ADC_512FS,
	IP2_MUX_DAC_512FS,
	IP2_MUX_AB_MAC,
	IP2_MUX_AB_MAC_10CH,
	IP2_MUX_AB3_MAC,
	IP2_MUX_SH,
	IP2_MUX_CODEC_SH,
	IP2_MUX_PCM_I2S_256FS_MUX,
	IP2_MUX_CH_M1_256FSI_MUX2,
	IP2_MUX_CH_M1_256FSI_MUX1,
	IP2_MUX_CH_M1_256FSI_MUX0,
	IP2_MUX_CH_M1_256FSI_MUX,
	IP2_MUX_CH_M1_256FSI,
	IP2_MUX_CH_M2_256FSI_MUX2,
	IP2_MUX_CH_M2_256FSI_MUX1,
	IP2_MUX_CH_M2_256FSI_MUX0,
	IP2_MUX_CH_M2_256FSI_MUX,
	IP2_MUX_CH_M2_256FSI,
	IP2_MUX_CH_M3_256FSI_MUX2,
	IP2_MUX_CH_M3_256FSI_MUX1,
	IP2_MUX_CH_M3_256FSI_MUX0,
	IP2_MUX_CH_M3_256FSI_MUX,
	IP2_MUX_CH_M3_256FSI,
	IP2_MUX_CH_M4_256FSI_MUX2,
	IP2_MUX_CH_M4_256FSI_MUX1,
	IP2_MUX_CH_M4_256FSI_MUX0,
	IP2_MUX_CH_M4_256FSI_MUX,
	IP2_MUX_CH_M4_256FSI,
	IP2_MUX_CH_M5_256FSI_MUX2,
	IP2_MUX_CH_M5_256FSI_MUX1,
	IP2_MUX_CH_M5_256FSI_MUX0,
	IP2_MUX_CH_M5_256FSI_MUX,
	IP2_MUX_CH_M5_256FSI,
	IP2_MUX_CH6_256FSI_MUX2,
	IP2_MUX_CH6_256FSI_MUX1,
	IP2_MUX_CH6_256FSI_MUX0,
	IP2_MUX_CH6_256FSI_MUX,
	IP2_MUX_CH6_256FSI,
	IP2_MUX_CH7_256FSI_MUX2,
	IP2_MUX_CH7_256FSI_MUX1,
	IP2_MUX_CH7_256FSI_MUX0,
	IP2_MUX_CH7_256FSI_MUX,
	IP2_MUX_CH7_256FSI,
	IP2_MUX_CH8_256FSI_MUX2,
	IP2_MUX_CH8_256FSI_MUX1,
	IP2_MUX_CH8_256FSI_MUX0,
	IP2_MUX_CH8_256FSI_MUX,
	IP2_MUX_CH8_256FSI,
	IP2_MUX_CH9_256FSI_MUX2,
	IP2_MUX_CH9_256FSI_MUX1,
	IP2_MUX_CH9_256FSI_MUX0,
	IP2_MUX_CH9_256FSI_MUX,
	IP2_MUX_CH9_256FSI,
	IP2_MUX_DVB1_SYNTH_REF,
	IP2_MUX_DECODER1_SYNTH_HDMI_CTSN_256FS,
	IP2_MUX_DECODER1_SYNTH_HDMI_CTSN_128FS,
	IP2_MUX_DECODER1_SYNTH_HDMI2_CTSN_256FS,
	IP2_MUX_DECODER1_SYNTH_HDMI2_CTSN_128FS,
	IP2_MUX_DECODER1_SYNTH_SPDIF_CDR_256FS,
	IP2_MUX_DECODER1_SYNTH_SPDIF_CDR_128FS,
	IP2_MUX_DECODER1_256FS_MUX0,
	IP2_MUX_DECODER1_256FS_MUX1,
	IP2_MUX_DECODER1_256FS_MUX,
	IP2_MUX_DVB1_TIMING_GEN_256FS,
	IP2_MUX_DECODER1_128FS_MUX0,
	IP2_MUX_DECODER1_128FS_MUX1,
	IP2_MUX_DECODER1_128FS_MUX,
	IP2_MUX_DVB2_SYNTH_REF,
	IP2_MUX_DECODER2_SYNTH_HDMI_CTSN_256FS,
	IP2_MUX_DECODER2_SYNTH_HDMI_CTSN_128FS,
	IP2_MUX_DECODER2_SYNTH_HDMI2_CTSN_256FS,
	IP2_MUX_DECODER2_SYNTH_HDMI2_CTSN_128FS,
	IP2_MUX_DECODER2_SYNTH_SPDIF_CDR_256FS,
	IP2_MUX_DECODER2_SYNTH_SPDIF_CDR_128FS,
	IP2_MUX_DECODER2_256FS_MUX0,
	IP2_MUX_DECODER2_256FS_MUX1,
	IP2_MUX_DECODER2_256FS_MUX,
	IP2_MUX_DVB2_TIMING_GEN_256FS,
	IP2_MUX_DECODER2_128FS_MUX0,
	IP2_MUX_DECODER2_128FS_MUX1,
	IP2_MUX_DECODER2_128FS_MUX,
	IP2_MUX_DVB3_SYNTH_REF,
	IP2_MUX_DECODER3_SYNTH_HDMI_CTSN_256FS,
	IP2_MUX_DECODER3_SYNTH_HDMI_CTSN_128FS,
	IP2_MUX_DECODER3_SYNTH_HDMI2_CTSN_256FS,
	IP2_MUX_DECODER3_SYNTH_HDMI2_CTSN_128FS,
	IP2_MUX_DECODER3_SYNTH_SPDIF_CDR_256FS,
	IP2_MUX_DECODER3_SYNTH_SPDIF_CDR_128FS,
	IP2_MUX_DECODER3_256FS_MUX0,
	IP2_MUX_DECODER3_256FS_MUX1,
	IP2_MUX_DECODER3_256FS_MUX,
	IP2_MUX_DVB3_TIMING_GEN_256FS,
	IP2_MUX_DECODER3_128FS_MUX0,
	IP2_MUX_DECODER3_128FS_MUX1,
	IP2_MUX_DECODER3_128FS_MUX,
	IP2_MUX_DVB4_SYNTH_REF,
	IP2_MUX_DECODER4_SYNTH_HDMI_CTSN_256FS,
	IP2_MUX_DECODER4_SYNTH_HDMI_CTSN_128FS,
	IP2_MUX_DECODER4_SYNTH_HDMI2_CTSN_256FS,
	IP2_MUX_DECODER4_SYNTH_HDMI2_CTSN_128FS,
	IP2_MUX_DECODER4_SYNTH_SPDIF_CDR_256FS,
	IP2_MUX_DECODER4_SYNTH_SPDIF_CDR_128FS,
	IP2_MUX_DECODER4_256FS_MUX0,
	IP2_MUX_DECODER4_256FS_MUX1,
	IP2_MUX_DECODER4_256FS_MUX,
	IP2_MUX_DVB4_TIMING_GEN_256FS,
	IP2_MUX_DECODER4_128FS_MUX0,
	IP2_MUX_DECODER4_128FS_MUX1,
	IP2_MUX_DECODER4_128FS_MUX,
	IP2_MUX_DVB5_TIMING_GEN_256FS,
	IP2_MUX_DVB6_TIMING_GEN_256FS,
	IP2_MUX_AUDMA_V2_R1_SYNTH_REF,
	IP2_MUX_AUDMA_V2_R1_256FS_MUX,
	IP2_MUX_AUDMA_V2_R1_256FS,
	IP2_MUX_AUDMA_V2_R2_SYNTH_REF,
	IP2_MUX_AUDMA_V2_R2_256FS,
	IP2_MUX_SPDIF_RX_128FS,
	IP2_MUX_SPDIF_RX_DG,
	IP2_MUX_SPDIF_RX_SYNTH_REF,
	IP2_MUX_HDMI_RX_SYNTH_REF,
	IP2_MUX_HDMI_RX_S2P_256FS_MUX,
	IP2_MUX_HDMI_RX_S2P_256FS,
	IP2_MUX_HDMI_RX_S2P_128FS_MUX,
	IP2_MUX_HDMI_RX_S2P_128FS,
	IP2_MUX_HDMI_RX_MPLL_432,
	IP2_MUX_BCLK_I2S_DECODER,
	IP2_MUX_MCLK_I2S_DECODER,
	IP2_MUX_I2S_BCLK_DG,
	IP2_MUX_I2S_SYNTH_REF,
	IP2_MUX_HDMI_TX_TO_I2S_TX_256FS_MUX,
	IP2_MUX_HDMI_TX_TO_I2S_TX_128FS_MUX,
	IP2_MUX_HDMI_TX_TO_I2S_TX_64FS_MUX,
	IP2_MUX_HDMI_TX_TO_I2S_TX_32FS_MUX,
	IP2_MUX_HDMI_RX_TO_I2S_TX_256FS_MUX,
	IP2_MUX_HDMI_RX_TO_I2S_TX_128FS_MUX,
	IP2_MUX_HDMI_RX_TO_I2S_TX_64FS_MUX,
	IP2_MUX_HDMI_RX_TO_I2S_TX_32FS_MUX,
	IP2_MUX_I2S_TX_FIFO_256FS,
	IP2_MUX_I2S_MCLK_SYNTH_REF,
	IP2_MUX_MCLK_I2S_ENCODER,
	IP2_MUX_I2S_BCLK_SYNTH_REF_MUX,
	IP2_MUX_I2S_BCLK_SYNTH_REF,
	IP2_MUX_BCLK_I2S_ENCODER_128FS_SSC_MUX,
	IP2_MUX_BCLK_I2S_ENCODER_64FS_SSC_MUX,
	IP2_MUX_BCLK_I2S_ENCODER_32FS_SSC_MUX,
	IP2_MUX_BCLK_I2S_ENCODER_MUX,
	IP2_MUX_BCLK_I2S_ENCODER,
	IP2_MUX_I2S_TRX_SYNTH_REF,
	IP2_MUX_I2S_TRX_BCLK_TIMING_GEN,
	IP2_MUX_PCM_CLK_216,
	IP2_MUX_PCM_CLK_AUPLL,
	IP2_MUX_SPDIF_TX_GPA_SYNTH_256FS_MUX,
	IP2_MUX_SPDIF_TX_256FS_MUX,
	IP2_MUX_SPDIF_TX_256FS,
	IP2_MUX_SPDIF_TX_GPA_128FS_MUX,
	IP2_MUX_SPDIF_TX_128FS_MUX,
	IP2_MUX_SPDIF_TX_128FS,
	IP2_MUX_SPDIF_TX_SYNTH_REF,
	IP2_MUX_SPDIF_TX2_GPA_SYNTH_256FS_MUX,
	IP2_MUX_SPDIF_TX2_256FS_MUX,
	IP2_MUX_SPDIF_TX2_256FS,
	IP2_MUX_SPDIF_TX2_GPA_128FS_MUX,
	IP2_MUX_SPDIF_TX2_128FS_MUX,
	IP2_MUX_SPDIF_TX2_128FS,
	IP2_MUX_NPCM_SYNTH_REF,
	IP2_MUX_NPCM2_SYNTH_REF,
	IP2_MUX_NPCM3_SYNTH_REF,
	IP2_MUX_NPCM4_SYNTH_REF,
	IP2_MUX_NPCM4_256FS_TIMING_GEN,
	IP2_MUX_NPCM5_SYNTH_REF,
	IP2_MUX_NPCM5_256FS_TIMING_GEN,
	IP2_MUX_HDMI_TX_256FS_MUX1,
	IP2_MUX_HDMI_TX_256FS_MUX0,
	IP2_MUX_HDMI_TX_256FS_MUX,
	IP2_MUX_HDMI_TX_256FS,
	IP2_MUX_HDMI_TX_128FS,
	IP2_MUX_SPDIF_EXTEARC_256FS_MUX,
	IP2_MUX_SPDIF_EXTEARC_128FS_MUX,
	IP2_MUX_SPDIF_EXTEARC_MUX0,
	IP2_MUX_SPDIF_EXTEARC_MUX,
	IP2_MUX_SPDIF_EXTEARC,
	IP2_MUX_EARC_TX_SYNTH_REF,
	IP2_MUX_EARC_TX_GPA_SYNTH_REF,
	IP2_MUX_EARC_TX_SYNTH_IIR_REF,
	IP2_MUX_EARC_TX_GPA_256FS_MUX,
	IP2_MUX_EARC_TX_PLL_256FS_REF_MUX,
	IP2_MUX_EARC_TX_PLL_256FS_REF,
	IP2_MUX_EARC_TX_PLL,
	IP2_MUX_EARC_TX_64XCHXFS,
	IP2_MUX_EARC_TX_32XCHXFS,
	IP2_MUX_EARC_TX_32XFS,
	IP2_MUX_EARC_TX_SOURCE_256FS_MUX,
	IP2_MUX_EARC_TX_SOURCE_256FS,
	IP2_MUX_SIN_GEN,
	IP2_MUX_TEST_BUS,
	IP2_MUX_SRC_MAC_BSPLINE,
	IP2_MUX_SYNTH_PCM_TRX_256FS,
	IP2_MUX_CLK_NUM,
};

static const char *ip2_mux_clk[IP2_MUX_CLK_NUM] = {
	[IP2_MUX_RIU_BRG_R2] = "viva_riu_brg_r2_int_ck",
	[IP2_MUX_AU_MCU] = "viva_au_mcu_int_ck",
	[IP2_MUX_AU_MCU_BUS] = "au_mcu_bus_int_ck",
	[IP2_MUX_SYNTH_CODEC] = "viva_synth_codec_int_ck",
	[IP2_MUX_SYNTH_2ND_ORDER] = "viva_synth_2nd_order_int_ck",
	[IP2_MUX_AUPLL_IN] = "viva_aupll_in_int_ck",
	[IP2_MUX_DSP_INT] = "viva_dsp_int_ck",
	[IP2_MUX_SIF_ADC_R2B_FIFO_IN] = "viva_sif_adc_r2b_fifo_in_int_ck",
	[IP2_MUX_SIF_ADC_R2B] = "viva_sif_adc_r2b_int_ck",
	[IP2_MUX_SIF_ADC_CIC] = "viva_sif_adc_cic_int_ck",
	[IP2_MUX_FM_DEMODULATOR] = "viva_fm_demodulator_int_ck",
	[IP2_MUX_SIF_SYNTH] = "viva_sif_synth_int_ck",
	[IP2_MUX_SIF_CORDIC] = "viva_sif_cordic_int_ck",
	[IP2_MUX_PARSER_INT] = "viva_parser_int_ck",
	[IP2_MUX_GPA_480FSO] = "viva_gpa_480fso_int_ck",
	[IP2_MUX_GPA_240FSO] = "viva_gpa_240fso_int_ck",
	[IP2_MUX_GPA_256FSO] = "viva_gpa_256fso_int_ck",
	[IP2_MUX_ADC_512FS] = "viva_adc_512fs_int_ck",
	[IP2_MUX_DAC_512FS] = "viva_dac_512fs_int_ck",
	[IP2_MUX_AB_MAC] = "viva_ab_mac_int_ck",
	[IP2_MUX_AB_MAC_10CH] = "viva_ab_mac_10ch_int_ck",
	[IP2_MUX_AB3_MAC] = "viva_ab3_mac_int_ck",
	[IP2_MUX_SH] = "viva_sh_int_ck",
	[IP2_MUX_CODEC_SH] = "viva_codec_sh_int_ck",
	[IP2_MUX_PCM_I2S_256FS_MUX] = "pcm_i2s_256fs_mux_int_ck",
	[IP2_MUX_CH_M1_256FSI_MUX2] = "ch_m1_256fsi_mux2_int_ck",
	[IP2_MUX_CH_M1_256FSI_MUX1] = "ch_m1_256fsi_mux1_int_ck",
	[IP2_MUX_CH_M1_256FSI_MUX0] = "ch_m1_256fsi_mux0_int_ck",
	[IP2_MUX_CH_M1_256FSI_MUX] = "ch_m1_256fsi_mux_int_ck",
	[IP2_MUX_CH_M1_256FSI] = "viva_ch_m1_256fsi_int_ck",
	[IP2_MUX_CH_M2_256FSI_MUX2] = "ch_m2_256fsi_mux2_int_ck",
	[IP2_MUX_CH_M2_256FSI_MUX1] = "ch_m2_256fsi_mux1_int_ck",
	[IP2_MUX_CH_M2_256FSI_MUX0] = "ch_m2_256fsi_mux0_int_ck",
	[IP2_MUX_CH_M2_256FSI_MUX] = "ch_m2_256fsi_mux_int_ck",
	[IP2_MUX_CH_M2_256FSI] = "viva_ch_m2_256fsi_int_ck",
	[IP2_MUX_CH_M3_256FSI_MUX2] = "ch_m3_256fsi_mux2_int_ck",
	[IP2_MUX_CH_M3_256FSI_MUX1] = "ch_m3_256fsi_mux1_int_ck",
	[IP2_MUX_CH_M3_256FSI_MUX0] = "ch_m3_256fsi_mux0_int_ck",
	[IP2_MUX_CH_M3_256FSI_MUX] = "ch_m3_256fsi_mux_int_ck",
	[IP2_MUX_CH_M3_256FSI] = "viva_ch_m3_256fsi_int_ck",
	[IP2_MUX_CH_M4_256FSI_MUX2] = "ch_m4_256fsi_mux2_int_ck",
	[IP2_MUX_CH_M4_256FSI_MUX1] = "ch_m4_256fsi_mux1_int_ck",
	[IP2_MUX_CH_M4_256FSI_MUX0] = "ch_m4_256fsi_mux0_int_ck",
	[IP2_MUX_CH_M4_256FSI_MUX] = "ch_m4_256fsi_mux_int_ck",
	[IP2_MUX_CH_M4_256FSI] = "viva_ch_m4_256fsi_int_ck",
	[IP2_MUX_CH_M5_256FSI_MUX2] = "ch_m5_256fsi_mux2_int_ck",
	[IP2_MUX_CH_M5_256FSI_MUX1] = "ch_m5_256fsi_mux1_int_ck",
	[IP2_MUX_CH_M5_256FSI_MUX0] = "ch_m5_256fsi_mux0_int_ck",
	[IP2_MUX_CH_M5_256FSI_MUX] = "ch_m5_256fsi_mux_int_ck",
	[IP2_MUX_CH_M5_256FSI] = "viva_ch_m5_256fsi_int_ck",
	[IP2_MUX_CH6_256FSI_MUX2] = "ch6_256fsi_mux2_int_ck",
	[IP2_MUX_CH6_256FSI_MUX1] = "ch6_256fsi_mux1_int_ck",
	[IP2_MUX_CH6_256FSI_MUX0] = "ch6_256fsi_mux0_int_ck",
	[IP2_MUX_CH6_256FSI_MUX] = "ch6_256fsi_mux_int_ck",
	[IP2_MUX_CH6_256FSI] = "viva_ch6_256fsi_int_ck",
	[IP2_MUX_CH7_256FSI_MUX2] = "ch7_256fsi_mux2_int_ck",
	[IP2_MUX_CH7_256FSI_MUX1] = "ch7_256fsi_mux1_int_ck",
	[IP2_MUX_CH7_256FSI_MUX0] = "ch7_256fsi_mux0_int_ck",
	[IP2_MUX_CH7_256FSI_MUX] = "ch7_256fsi_mux_int_ck",
	[IP2_MUX_CH7_256FSI] = "viva_ch7_256fsi_int_ck",
	[IP2_MUX_CH8_256FSI_MUX2] = "ch8_256fsi_mux2_int_ck",
	[IP2_MUX_CH8_256FSI_MUX1] = "ch8_256fsi_mux1_int_ck",
	[IP2_MUX_CH8_256FSI_MUX0] = "ch8_256fsi_mux0_int_ck",
	[IP2_MUX_CH8_256FSI_MUX] = "ch8_256fsi_mux_int_ck",
	[IP2_MUX_CH8_256FSI] = "viva_ch8_256fsi_int_ck",
	[IP2_MUX_CH9_256FSI_MUX2] = "ch9_256fsi_mux2_int_ck",
	[IP2_MUX_CH9_256FSI_MUX1] = "ch9_256fsi_mux1_int_ck",
	[IP2_MUX_CH9_256FSI_MUX0] = "ch9_256fsi_mux0_int_ck",
	[IP2_MUX_CH9_256FSI_MUX] = "ch9_256fsi_mux_int_ck",
	[IP2_MUX_CH9_256FSI] = "viva_ch9_256fsi_int_ck",
	[IP2_MUX_DVB1_SYNTH_REF] = "viva_dvb1_synth_ref_int_ck",
	[IP2_MUX_DECODER1_SYNTH_HDMI_CTSN_256FS] = "decoder1_synth_hdmi_ctsn_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER1_SYNTH_HDMI_CTSN_128FS] = "decoder1_synth_hdmi_ctsn_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER1_SYNTH_HDMI2_CTSN_256FS] =
					"decoder1_synth_hdmi2_ctsn_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER1_SYNTH_HDMI2_CTSN_128FS] =
					"decoder1_synth_hdmi2_ctsn_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER1_SYNTH_SPDIF_CDR_256FS] = "decoder1_synth_spdif_cdr_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER1_SYNTH_SPDIF_CDR_128FS] = "decoder1_synth_spdif_cdr_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER1_256FS_MUX0] = "decoder1_256fs_mux0_int_ck",
	[IP2_MUX_DECODER1_256FS_MUX1] = "decoder1_256fs_mux1_int_ck",
	[IP2_MUX_DECODER1_256FS_MUX] = "decoder1_256fs_mux_int_ck",
	[IP2_MUX_DVB1_TIMING_GEN_256FS] = "viva_dvb1_timing_gen_256fs_int_ck",
	[IP2_MUX_DECODER1_128FS_MUX0] = "decoder1_128fs_mux0_int_ck",
	[IP2_MUX_DECODER1_128FS_MUX1] = "decoder1_128fs_mux1_int_ck",
	[IP2_MUX_DECODER1_128FS_MUX] = "decoder1_128fs_mux_int_ck",
	[IP2_MUX_DVB2_SYNTH_REF] = "viva_dvb2_synth_ref_int_ck",
	[IP2_MUX_DECODER2_SYNTH_HDMI_CTSN_256FS] = "decoder2_synth_hdmi_ctsn_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER2_SYNTH_HDMI_CTSN_128FS] = "decoder2_synth_hdmi_ctsn_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER2_SYNTH_HDMI2_CTSN_256FS] =
					"decoder2_synth_hdmi2_ctsn_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER2_SYNTH_HDMI2_CTSN_128FS] =
					"decoder2_synth_hdmi2_ctsn_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER2_SYNTH_SPDIF_CDR_256FS] = "decoder2_synth_spdif_cdr_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER2_SYNTH_SPDIF_CDR_128FS] = "decoder2_synth_spdif_cdr_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER2_256FS_MUX0] = "decoder2_256fs_mux0_int_ck",
	[IP2_MUX_DECODER2_256FS_MUX1] = "decoder2_256fs_mux1_int_ck",
	[IP2_MUX_DECODER2_256FS_MUX] = "decoder2_256fs_mux_int_ck",
	[IP2_MUX_DVB2_TIMING_GEN_256FS] = "viva_dvb2_timing_gen_256fs_int_ck",
	[IP2_MUX_DECODER2_128FS_MUX0] = "decoder2_128fs_mux0_int_ck",
	[IP2_MUX_DECODER2_128FS_MUX1] = "decoder2_128fs_mux1_int_ck",
	[IP2_MUX_DECODER2_128FS_MUX] = "decoder2_128fs_mux_int_ck",
	[IP2_MUX_DVB3_SYNTH_REF] = "viva_dvb3_synth_ref_int_ck",
	[IP2_MUX_DECODER3_SYNTH_HDMI_CTSN_256FS] = "decoder3_synth_hdmi_ctsn_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER3_SYNTH_HDMI_CTSN_128FS] = "decoder3_synth_hdmi_ctsn_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER3_SYNTH_HDMI2_CTSN_256FS] =
					"decoder3_synth_hdmi2_ctsn_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER3_SYNTH_HDMI2_CTSN_128FS] =
					"decoder3_synth_hdmi2_ctsn_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER3_SYNTH_SPDIF_CDR_256FS] = "decoder3_synth_spdif_cdr_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER3_SYNTH_SPDIF_CDR_128FS] = "decoder3_synth_spdif_cdr_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER3_256FS_MUX0] = "decoder3_256fs_mux0_int_ck",
	[IP2_MUX_DECODER3_256FS_MUX1] = "decoder3_256fs_mux1_int_ck",
	[IP2_MUX_DECODER3_256FS_MUX] = "decoder3_256fs_mux_int_ck",
	[IP2_MUX_DVB3_TIMING_GEN_256FS] = "viva_dvb3_timing_gen_256fs_int_ck",
	[IP2_MUX_DECODER3_128FS_MUX0] = "decoder3_128fs_mux0_int_ck",
	[IP2_MUX_DECODER3_128FS_MUX1] = "decoder3_128fs_mux1_int_ck",
	[IP2_MUX_DECODER3_128FS_MUX] = "decoder3_128fs_mux_int_ck",
	[IP2_MUX_DVB4_SYNTH_REF] = "viva_dvb4_synth_ref_int_ck",
	[IP2_MUX_DECODER4_SYNTH_HDMI_CTSN_256FS] = "decoder4_synth_hdmi_ctsn_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER4_SYNTH_HDMI_CTSN_128FS] = "decoder4_synth_hdmi_ctsn_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER4_SYNTH_HDMI2_CTSN_256FS] =
					"decoder4_synth_hdmi2_ctsn_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER4_SYNTH_HDMI2_CTSN_128FS] =
					"decoder4_synth_hdmi2_ctsn_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER4_SYNTH_SPDIF_CDR_256FS] = "decoder4_synth_spdif_cdr_256fs_div_mux_int_ck",
	[IP2_MUX_DECODER4_SYNTH_SPDIF_CDR_128FS] = "decoder4_synth_spdif_cdr_128fs_div_mux_int_ck",
	[IP2_MUX_DECODER4_256FS_MUX0] = "decoder4_256fs_mux0_int_ck",
	[IP2_MUX_DECODER4_256FS_MUX1] = "decoder4_256fs_mux1_int_ck",
	[IP2_MUX_DECODER4_256FS_MUX] = "decoder4_256fs_mux_int_ck",
	[IP2_MUX_DVB4_TIMING_GEN_256FS] = "viva_dvb4_timing_gen_256fs_int_ck",
	[IP2_MUX_DECODER4_128FS_MUX0] = "decoder4_128fs_mux0_int_ck",
	[IP2_MUX_DECODER4_128FS_MUX1] = "decoder4_128fs_mux1_int_ck",
	[IP2_MUX_DECODER4_128FS_MUX] = "decoder4_128fs_mux_int_ck",
	[IP2_MUX_DVB5_TIMING_GEN_256FS] = "viva_dvb5_timing_gen_256fs_int_ck",
	[IP2_MUX_DVB6_TIMING_GEN_256FS] = "viva_dvb6_timing_gen_256fs_int_ck",
	[IP2_MUX_AUDMA_V2_R1_SYNTH_REF] = "viva_audma_v2_r1_synth_ref_int_ck",
	[IP2_MUX_AUDMA_V2_R1_256FS_MUX] = "viva_audma_v2_r1_256fs_mux_int_ck",
	[IP2_MUX_AUDMA_V2_R1_256FS] = "viva_audma_v2_r1_256fs_int_ck",
	[IP2_MUX_AUDMA_V2_R2_SYNTH_REF] = "viva_audma_v2_r2_synth_ref_int_ck",
	[IP2_MUX_AUDMA_V2_R2_256FS] = "viva_audma_v2_r2_256fs_int_ck",
	[IP2_MUX_SPDIF_RX_128FS] = "viva_spdif_rx_128fs_int_ck",
	[IP2_MUX_SPDIF_RX_DG] = "viva_spdif_rx_dg_int_ck",
	[IP2_MUX_SPDIF_RX_SYNTH_REF] = "viva_spdif_rx_synth_ref_int_ck",
	[IP2_MUX_HDMI_RX_SYNTH_REF] = "viva_hdmi_rx_synth_ref_int_ck",
	[IP2_MUX_HDMI_RX_S2P_256FS_MUX] = "hdmi_rx_s2p_256fs_mux_int_ck",
	[IP2_MUX_HDMI_RX_S2P_256FS] = "viva_hdmi_rx_s2p_256fs_int_ck",
	[IP2_MUX_HDMI_RX_S2P_128FS_MUX] = "hdmi_rx_s2p_128fs_mux_int_ck",
	[IP2_MUX_HDMI_RX_S2P_128FS] = "viva_hdmi_rx_s2p_128fs_int_ck",
	[IP2_MUX_HDMI_RX_MPLL_432] = "viva_hdmi_rx_mpll_432_int_ck",
	[IP2_MUX_BCLK_I2S_DECODER] = "viva_bclk_i2s_decoder_int_ck",
	[IP2_MUX_MCLK_I2S_DECODER] = "viva_mclk_i2s_decoder_int_ck",
	[IP2_MUX_I2S_BCLK_DG] = "viva_i2s_bclk_dg_int_ck",
	[IP2_MUX_I2S_SYNTH_REF] = "viva_i2s_synth_ref_int_ck",
	[IP2_MUX_HDMI_TX_TO_I2S_TX_256FS_MUX] = "hdmi_tx_to_i2s_tx_256fs_mux_int_ck",
	[IP2_MUX_HDMI_TX_TO_I2S_TX_128FS_MUX] = "hdmi_tx_to_i2s_tx_128fs_mux_int_ck",
	[IP2_MUX_HDMI_TX_TO_I2S_TX_64FS_MUX] = "hdmi_tx_to_i2s_tx_64fs_mux_int_ck",
	[IP2_MUX_HDMI_TX_TO_I2S_TX_32FS_MUX] = "hdmi_tx_to_i2s_tx_32fs_mux_int_ck",
	[IP2_MUX_HDMI_RX_TO_I2S_TX_256FS_MUX] = "hdmi_rx_to_i2s_tx_256fs_mux_int_ck",
	[IP2_MUX_HDMI_RX_TO_I2S_TX_128FS_MUX] = "hdmi_rx_to_i2s_tx_128fs_mux_int_ck",
	[IP2_MUX_HDMI_RX_TO_I2S_TX_64FS_MUX] = "hdmi_rx_to_i2s_tx_64fs_mux_int_ck",
	[IP2_MUX_HDMI_RX_TO_I2S_TX_32FS_MUX] = "hdmi_rx_to_i2s_tx_32fs_mux_int_ck",
	[IP2_MUX_I2S_TX_FIFO_256FS] = "viva_i2s_tx_fifo_256fs_int_ck",
	[IP2_MUX_I2S_MCLK_SYNTH_REF] = "viva_i2s_mclk_synth_ref_int_ck",
	[IP2_MUX_MCLK_I2S_ENCODER] = "viva_mclk_i2s_encoder_int_ck",
	[IP2_MUX_I2S_BCLK_SYNTH_REF_MUX] = "i2s_bclk_synth_ref_mux_int_ck",
	[IP2_MUX_I2S_BCLK_SYNTH_REF] = "viva_i2s_bclk_synth_ref_int_ck",
	[IP2_MUX_BCLK_I2S_ENCODER_128FS_SSC_MUX] = "bclk_i2s_encoder_128fs_ssc_mux_int_ck",
	[IP2_MUX_BCLK_I2S_ENCODER_64FS_SSC_MUX] = "bclk_i2s_encoder_64fs_ssc_mux_int_ck",
	[IP2_MUX_BCLK_I2S_ENCODER_32FS_SSC_MUX] = "bclk_i2s_encoder_32fs_ssc_mux_int_ck",
	[IP2_MUX_BCLK_I2S_ENCODER_MUX] = "bclk_i2s_encoder_mux_int_ck",
	[IP2_MUX_BCLK_I2S_ENCODER] = "bclk_i2s_encoder_int_ck",
	[IP2_MUX_I2S_TRX_SYNTH_REF] = "viva_i2s_trx_synth_ref_int_ck",
	[IP2_MUX_I2S_TRX_BCLK_TIMING_GEN] = "viva_i2s_trx_bclk_timing_gen_int_ck",
	[IP2_MUX_PCM_CLK_216] = "viva_pcm_clk_216_int_ck",
	[IP2_MUX_PCM_CLK_AUPLL] = "viva_pcm_clk_aupll_int_ck",
	[IP2_MUX_SPDIF_TX_GPA_SYNTH_256FS_MUX] = "spdif_tx_gpa_synth_256fs_mux_int_ck",
	[IP2_MUX_SPDIF_TX_256FS_MUX] = "spdif_tx_256fs_mux_int_ck",
	[IP2_MUX_SPDIF_TX_256FS] = "viva_spdif_tx_256fs_int_ck",
	[IP2_MUX_SPDIF_TX_GPA_128FS_MUX] = "spdif_tx_gpa_128fs_mux_int_ck",
	[IP2_MUX_SPDIF_TX_128FS_MUX] = "spdif_tx_128fs_mux_int_ck",
	[IP2_MUX_SPDIF_TX_128FS] = "viva_spdif_tx_128fs_int_ck",
	[IP2_MUX_SPDIF_TX_SYNTH_REF] = "viva_spdif_tx_synth_ref_int_ck",
	[IP2_MUX_SPDIF_TX2_GPA_SYNTH_256FS_MUX] = "spdif_tx2_gpa_synth_256fs_mux_int_ck",
	[IP2_MUX_SPDIF_TX2_256FS_MUX] = "spdif_tx2_256fs_mux_int_ck",
	[IP2_MUX_SPDIF_TX2_256FS] = "viva_spdif_tx2_256fs_int_ck",
	[IP2_MUX_SPDIF_TX2_GPA_128FS_MUX] = "spdif_tx2_gpa_128fs_mux_int_ck",
	[IP2_MUX_SPDIF_TX2_128FS_MUX] = "spdif_tx2_128fs_mux_int_ck",
	[IP2_MUX_SPDIF_TX2_128FS] = "viva_spdif_tx2_128fs_int_ck",
	[IP2_MUX_NPCM_SYNTH_REF] = "viva_npcm_synth_ref_int_ck",
	[IP2_MUX_NPCM2_SYNTH_REF] = "viva_npcm2_synth_ref_int_ck",
	[IP2_MUX_NPCM3_SYNTH_REF] = "viva_npcm3_synth_ref_int_ck",
	[IP2_MUX_NPCM4_SYNTH_REF] = "viva_npcm4_synth_ref_int_ck",
	[IP2_MUX_NPCM4_256FS_TIMING_GEN] = "viva_npcm4_256fs_timing_gen_int_ck",
	[IP2_MUX_NPCM5_SYNTH_REF] = "viva_npcm5_synth_ref_int_ck",
	[IP2_MUX_NPCM5_256FS_TIMING_GEN] = "viva_npcm5_256fs_timing_gen_int_ck",
	[IP2_MUX_HDMI_TX_256FS_MUX1] = "hdmi_tx_256fs_mux1_int_ck",
	[IP2_MUX_HDMI_TX_256FS_MUX0] = "hdmi_tx_256fs_mux0_int_ck",
	[IP2_MUX_HDMI_TX_256FS_MUX] = "hdmi_tx_256fs_mux_int_ck",
	[IP2_MUX_HDMI_TX_256FS] = "viva_hdmi_tx_256fs_int_ck",
	[IP2_MUX_HDMI_TX_128FS] = "viva_hdmi_tx_128fs_int_ck",
	[IP2_MUX_SPDIF_EXTEARC_256FS_MUX] = "spdif_extearc_256fs_mux_int_ck",
	[IP2_MUX_SPDIF_EXTEARC_128FS_MUX] = "spdif_extearc_128fs_mux_int_ck",
	[IP2_MUX_SPDIF_EXTEARC_MUX0] = "spdif_extearc_mux0_int_ck",
	[IP2_MUX_SPDIF_EXTEARC_MUX] = "spdif_extearc_mux_int_ck",
	[IP2_MUX_SPDIF_EXTEARC] = "viva_spdif_extearc_int_ck",
	[IP2_MUX_EARC_TX_SYNTH_REF] = "viva_earc_tx_synth_ref_int_ck",
	[IP2_MUX_EARC_TX_GPA_SYNTH_REF] = "viva_earc_tx_gpa_synth_ref_int_ck",
	[IP2_MUX_EARC_TX_SYNTH_IIR_REF] = "viva_earc_tx_synth_iir_ref_int_ck",
	[IP2_MUX_EARC_TX_GPA_256FS_MUX] = "earc_tx_gpa_256fs_mux_int_ck",
	[IP2_MUX_EARC_TX_PLL_256FS_REF_MUX] = "earc_tx_pll_256fs_ref_mux_int_ck",
	[IP2_MUX_EARC_TX_PLL_256FS_REF] = "earc_tx_pll_256fs_ref_int_ck",
	[IP2_MUX_EARC_TX_PLL] = "viva_earc_tx_pll_int_ck",
	[IP2_MUX_EARC_TX_64XCHXFS] = "viva_earc_tx_64xchxfs_int_ck",
	[IP2_MUX_EARC_TX_32XCHXFS] = "viva_earc_tx_32xchxfs_int_ck",
	[IP2_MUX_EARC_TX_32XFS] = "viva_earc_tx_32xfs_int_ck",
	[IP2_MUX_EARC_TX_SOURCE_256FS_MUX] = "earc_tx_source_256fs_mux_int_ck",
	[IP2_MUX_EARC_TX_SOURCE_256FS] = "viva_earc_tx_source_256fs_int_ck",
	[IP2_MUX_SIN_GEN] = "viva_sin_gen_int_ck",
	[IP2_MUX_TEST_BUS] = "viva_test_bus_int_ck",
	[IP2_MUX_SRC_MAC_BSPLINE] = "viva_src_mac_bspline_int_ck",
	[IP2_MUX_SYNTH_PCM_TRX_256FS] = "synth_pcm_trx_256fs_int_ck",
};

//--------------------------------------------------
// SW CLOCK (IP_version 2)
//--------------------------------------------------
enum {
	IP2_CLK_AU_MCU_BUS2AU_MCU,
	IP2_CLK_AU_UART02AU_UART0,
	IP2_CLK_AU_UART12AU_UART1,
	IP2_CLK_MCU_NONPM2VIVA_VBDMAU,
	IP2_CLK_SYNTH_PCM_TRX_256FS2SYNTH_PCM_TRX_256FS,
	IP2_CLK_VIVA_AB_MAC_10CH2AB_MAC_10CH,
	IP2_CLK_VIVA_RIU2RIU_DFT_VIVALDI9,
	IP2_CLK_TSP2VIV_TSP_BRIDGE,
	IP2_CLK_BCLK_I2S_ENCODER,
	IP2_CLK_AB_MAC,
	IP2_CLK_AB3_MAC,
	IP2_CLK_ADC_512FS,
	IP2_CLK_AU_MCU,
	IP2_CLK_AUDMA_V2_R1_256FS,
	IP2_CLK_AUDMA_V2_R1_TIMING_GEN,
	IP2_CLK_AUDMA_V2_R1_SYNTH_REF,
	IP2_CLK_AUDMA_V2_R2_256FS,
	IP2_CLK_AUDMA_V2_R2_TIMING_GEN,
	IP2_CLK_AUDMA_V2_R2_SYNTH_REF,
	IP2_CLK_BCLK_I2S_DECODER,
	IP2_CLK_CH6_256FSI,
	IP2_CLK_CH7_256FSI,
	IP2_CLK_CH8_256FSI,
	IP2_CLK_CH9_256FSI,
	IP2_CLK_CODEC_SH,
	IP2_CLK_DAC_512_FS,
	IP2_CLK_DSP_230,
	IP2_CLK_DSP_230_1,
	IP2_CLK_DSP_230_2,
	IP2_CLK_DSP_230_3,
	IP2_CLK_DSP_230_DFS_REF,
	IP2_CLK_DSP_230_TM,
	IP2_CLK_DVB1_SYNTH_REF,
	IP2_CLK_DVB1_TIMING_GEN_256FS,
	IP2_CLK_DVB2_SYNTH_REF,
	IP2_CLK_DVB2_TIMING_GEN_256FS,
	IP2_CLK_DVB3_SYNTH_REF,
	IP2_CLK_DVB3_TIMING_GEN_256FS,
	IP2_CLK_DVB4_SYNTH_REF,
	IP2_CLK_DVB4_TIMING_GEN_256FS,
	IP2_CLK_DVB5_TIMING_GEN_256FS,
	IP2_CLK_DVB6_TIMING_GEN_256FS,
	IP2_CLK_EARC_TX_32XCHXFS,
	IP2_CLK_EARC_TX_32XFS,
	IP2_CLK_EARC_TX_64XCHXFS,
	IP2_CLK_EARC_TX_GPA_SYNTH_REF,
	IP2_CLK_EARC_TX_PLL,
	IP2_CLK_EARC_TX_SOURCE_256FS,
	IP2_CLK_EARC_TX_SYNTH_IIR_REF,
	IP2_CLK_EARC_TX_SYNTH_REF,
	IP2_CLK_FM_DEMODULATOR,
	IP2_CLK_GPA_240FSO,
	IP2_CLK_GPA_256FSO,
	IP2_CLK_GPA_480FSO,
	IP2_CLK_HDMI_RX_MPLL_432,
	IP2_CLK_HDMI_RX_S2P_128FS,
	IP2_CLK_HDMI_RX_S2P_256FS,
	IP2_CLK_HDMI_RX_SYNTH_REF,
	IP2_CLK_HDMI_RX_V2A_TIME_STAMP,
	IP2_CLK_HDMI_TX_128FS,
	IP2_CLK_HDMI_TX_256FS,
	IP2_CLK_I2S_BCLK_DG,
	IP2_CLK_I2S_BCLK_SYNTH_REF,
	IP2_CLK_I2S_MCLK_SYNTH_REF,
	IP2_CLK_I2S_SYNTH_REF,
	IP2_CLK_I2S_TRX_BCLK_TIMING_GEN,
	IP2_CLK_I2S_TRX_SYNTH_REF,
	IP2_CLK_I2S_TX_FIFO_256FS,
	IP2_CLK_MCLK_I2S_DECODER,
	IP2_CLK_NPCM_SYNTH_REF,
	IP2_CLK_NPCM2_SYNTH_REF,
	IP2_CLK_NPCM3_SYNTH_REF,
	IP2_CLK_NPCM4_256FS_TIMING_GEN,
	IP2_CLK_NPCM4_SYNTH_REF,
	IP2_CLK_NPCM5_256FS_TIMING_GEN,
	IP2_CLK_NPCM5_SYNTH_REF,
	IP2_CLK_PCM_CLK_216,
	IP2_CLK_PCM_CLK_AUPLL,
	IP2_CLK_R2_WB,
	IP2_CLK_R2_RIU_BRG,
	IP2_CLK_CH6_SH,
	IP2_CLK_CH7_SH,
	IP2_CLK_CH8_SH,
	IP2_CLK_CH9_SH,
	IP2_CLK_SIF_ADC_CIC,
	IP2_CLK_SIF_ADC_R2B_FIFO_IN,
	IP2_CLK_SIF_ADC_R2B,
	IP2_CLK_SIF_CORDIC,
	IP2_CLK_SIF_SYNTH,
	IP2_CLK_SIN_GEN,
	IP2_CLK_SPDIF_RX_128FS,
	IP2_CLK_SPDIF_RX_DG,
	IP2_CLK_SPDIF_RX_SYNTH_REF,
	IP2_CLK_SPDIF_TX_128FS,
	IP2_CLK_SPDIF_TX_256FS,
	IP2_CLK_SPDIF_TX_SYNTH_REF,
	IP2_CLK_SPDIF_TX2_128FS,
	IP2_CLK_SPDIF_TX2_256FS,
	IP2_CLK_SYNTH_2ND_ORDER,
	IP2_CLK_SYNTH_CODEC,
	IP2_CLK_TEST_BUS,
	IP2_CLK_XTAL_12M,
	IP2_CLK_XTAL_24M,
	IP2_CLK_CH_M1_256FSI,
	IP2_CLK_CH_M2_256FSI,
	IP2_CLK_CH_M3_256FSI,
	IP2_CLK_CH_M4_256FSI,
	IP2_CLK_CH_M5_256FSI,
	IP2_CLK_CH10_SH,
	IP2_CLK_CH_M1_SH,
	IP2_CLK_CH_M2_SH,
	IP2_CLK_CH_M3_SH,
	IP2_CLK_CH_M4_SH,
	IP2_CLK_CH_M5_SH,
	IP2_CLK_SRC_MAC_BSPLINE,
	IP2_CLK_NUM
};

static const char *ip2_sw_clk[IP2_CLK_NUM] = {
	[IP2_CLK_AU_MCU_BUS2AU_MCU] = "au_mcu_bus2au_mcu",
	[IP2_CLK_AU_UART02AU_UART0] = "au_uart02au_uart0",
	[IP2_CLK_AU_UART12AU_UART1] = "au_uart12au_uart1",
	[IP2_CLK_MCU_NONPM2VIVA_VBDMAU] = "mcu_nonpm2viva_vbdma",
	[IP2_CLK_SYNTH_PCM_TRX_256FS2SYNTH_PCM_TRX_256FS] =
					"synth_pcm_trx_256fs2synth_pcm_trx_256fs",
	[IP2_CLK_VIVA_AB_MAC_10CH2AB_MAC_10CH] = "viva_ab_mac_10ch2ab_mac_10ch",
	[IP2_CLK_VIVA_RIU2RIU_DFT_VIVALDI9] = "viva_riu2riu_dft_vivaldi9",
	[IP2_CLK_TSP2VIV_TSP_BRIDGE] = "tsp2viv_tsp_bridge",
	[IP2_CLK_BCLK_I2S_ENCODER] = "bclk_i2s_encoder2bclk_i2s_encoder",
	[IP2_CLK_AB_MAC] = "viva_ab_mac2ab_mac",
	[IP2_CLK_AB3_MAC] = "viva_ab3_mac2ab3_mac",
	[IP2_CLK_ADC_512FS] = "viva_adc_512fs2adc_512_fs",
	[IP2_CLK_AU_MCU] = "viva_au_mcu2au_mcu",
	[IP2_CLK_AUDMA_V2_R1_256FS] = "viva_audma_v2_r1_256fs2audma_v2_r1_256fs",
	[IP2_CLK_AUDMA_V2_R1_TIMING_GEN] = "viva_audma_v2_r1_256fs2audma_v2_r1_timing_gen",
	[IP2_CLK_AUDMA_V2_R1_SYNTH_REF] = "viva_audma_v2_r1_synth_ref2audma_v2_r1_synth_ref",
	[IP2_CLK_AUDMA_V2_R2_256FS] = "viva_audma_v2_r2_256fs2audma_v2_r2_256fs",
	[IP2_CLK_AUDMA_V2_R2_TIMING_GEN] = "viva_audma_v2_r2_256fs2audma_v2_r2_timing_gen",
	[IP2_CLK_AUDMA_V2_R2_SYNTH_REF] = "viva_audma_v2_r2_synth_ref2audma_v2_r2_synth_ref",
	[IP2_CLK_BCLK_I2S_DECODER] = "viva_bclk_i2s_decoder2bclk_i2s_decoder",
	[IP2_CLK_CH6_256FSI] = "viva_ch6_256fsi2ch6_256fsi",
	[IP2_CLK_CH7_256FSI] = "viva_ch7_256fsi2ch7_256fsi",
	[IP2_CLK_CH8_256FSI] = "viva_ch8_256fsi2ch8_256fsi",
	[IP2_CLK_CH9_256FSI] = "viva_ch9_256fsi2ch9_256fsi",
	[IP2_CLK_CODEC_SH] = "viva_codec_sh2codec_sh",
	[IP2_CLK_DAC_512_FS] = "viva_dac_512fs2dac_512_fs",
	[IP2_CLK_DSP_230] = "viva_dsp2dsp_230",
	[IP2_CLK_DSP_230_1] = "viva_dsp2dsp_230_1",
	[IP2_CLK_DSP_230_2] = "viva_dsp2dsp_230_2",
	[IP2_CLK_DSP_230_3] = "viva_dsp2dsp_230_3",
	[IP2_CLK_DSP_230_DFS_REF] = "viva_dsp2dsp_230_dfs_ref",
	[IP2_CLK_DSP_230_TM] = "viva_dsp2dsp_230_tm",
	[IP2_CLK_DVB1_SYNTH_REF] = "viva_dvb1_synth_ref2dvb1_synth_ref",
	[IP2_CLK_DVB1_TIMING_GEN_256FS] = "viva_dvb1_timing_gen_256fs2dvb1_timing_gen_256fs",
	[IP2_CLK_DVB2_SYNTH_REF] = "viva_dvb2_synth_ref2dvb2_synth_ref",
	[IP2_CLK_DVB2_TIMING_GEN_256FS] = "viva_dvb2_timing_gen_256fs2dvb2_timing_gen_256fs",
	[IP2_CLK_DVB3_SYNTH_REF] = "viva_dvb3_synth_ref2dvb3_synth_ref",
	[IP2_CLK_DVB3_TIMING_GEN_256FS] = "viva_dvb3_timing_gen_256fs2dvb3_timing_gen_256fs",
	[IP2_CLK_DVB4_SYNTH_REF] = "viva_dvb4_synth_ref2dvb4_synth_ref",
	[IP2_CLK_DVB4_TIMING_GEN_256FS] = "viva_dvb4_timing_gen_256fs2dvb4_timing_gen_256fs",
	[IP2_CLK_DVB5_TIMING_GEN_256FS] = "viva_dvb5_timing_gen_256fs2dvb5_timing_gen_256fs",
	[IP2_CLK_DVB6_TIMING_GEN_256FS] = "viva_dvb6_timing_gen_256fs2dvb6_timing_gen_256fs",
	[IP2_CLK_EARC_TX_32XCHXFS] = "viva_earc_tx_32xchxfs2earc_tx_32xchxfs",
	[IP2_CLK_EARC_TX_32XFS] = "viva_earc_tx_32xfs2earc_tx_32xfs",
	[IP2_CLK_EARC_TX_64XCHXFS] = "viva_earc_tx_64xchxfs2earc_tx_64xchxfs",
	[IP2_CLK_EARC_TX_GPA_SYNTH_REF] = "viva_earc_tx_gpa_synth_ref2earc_tx_gpa_synth_ref",
	[IP2_CLK_EARC_TX_PLL] = "viva_earc_tx_pll2earc_tx_pll",
	[IP2_CLK_EARC_TX_SOURCE_256FS] = "viva_earc_tx_source_256fs2earc_tx_source_256fs",
	[IP2_CLK_EARC_TX_SYNTH_IIR_REF] = "viva_earc_tx_synth_iir_ref2earc_tx_synth_iir_ref",
	[IP2_CLK_EARC_TX_SYNTH_REF] = "viva_earc_tx_synth_ref2earc_tx_synth_ref",
	[IP2_CLK_FM_DEMODULATOR] = "viva_fm_demodulator2fm_demodulator",
	[IP2_CLK_GPA_240FSO] = "viva_gpa_240fso2gpa_240fso",
	[IP2_CLK_GPA_256FSO] = "viva_gpa_256fso2gpa_256fso",
	[IP2_CLK_GPA_480FSO] = "viva_gpa_480fso2gpa_480fso",
	[IP2_CLK_HDMI_RX_MPLL_432] = "viva_hdmi_rx_mpll_4322hdmi_rx_mpll_432",
	[IP2_CLK_HDMI_RX_S2P_128FS] = "viva_hdmi_rx_s2p_128fs2hdmi_rx_s2p_128fs",
	[IP2_CLK_HDMI_RX_S2P_256FS] = "viva_hdmi_rx_s2p_256fs2hdmi_rx_s2p_256fs",
	[IP2_CLK_HDMI_RX_SYNTH_REF] = "viva_hdmi_rx_synth_ref2hdmi_rx_synth_ref",
	[IP2_CLK_HDMI_RX_V2A_TIME_STAMP] = "viva_hdmi_rx_v2a_time_stamp2hdmi_rx_v2a_time_stamp",
	[IP2_CLK_HDMI_TX_128FS] = "viva_hdmi_tx_128fs2hdmi_tx_128fs",
	[IP2_CLK_HDMI_TX_256FS] = "viva_hdmi_tx_256fs2hdmi_tx_256fs",
	[IP2_CLK_I2S_BCLK_DG] = "viva_i2s_bclk_dg2i2s_bclk_dg",
	[IP2_CLK_I2S_BCLK_SYNTH_REF] = "viva_i2s_bclk_synth_ref2i2s_bclk_synth_ref",
	[IP2_CLK_I2S_MCLK_SYNTH_REF] = "viva_i2s_mclk_synth_ref2i2s_mclk_synth_ref",
	[IP2_CLK_I2S_SYNTH_REF] = "viva_i2s_synth_ref2i2s_synth_ref",
	[IP2_CLK_I2S_TRX_BCLK_TIMING_GEN] = "viva_i2s_trx_bclk_timing_gen2i2s_trx_bclk_timing_gen",
	[IP2_CLK_I2S_TRX_SYNTH_REF] = "viva_i2s_trx_synth_ref2i2s_trx_synth_ref",
	[IP2_CLK_I2S_TX_FIFO_256FS] = "viva_i2s_tx_fifo_256fs2i2s_tx_fifo_256fs",
	[IP2_CLK_MCLK_I2S_DECODER] = "viva_mclk_i2s_decoder2mclk_i2s_decoder",
	[IP2_CLK_NPCM_SYNTH_REF] = "viva_npcm_synth_ref2npcm_synth_ref",
	[IP2_CLK_NPCM2_SYNTH_REF] = "viva_npcm2_synth_ref2npcm2_synth_ref",
	[IP2_CLK_NPCM3_SYNTH_REF] = "viva_npcm3_synth_ref2npcm3_synth_ref",
	[IP2_CLK_NPCM4_256FS_TIMING_GEN] = "viva_npcm4_256fs_timing_gen2npcm4_256fs_timing_gen",
	[IP2_CLK_NPCM4_SYNTH_REF] = "viva_npcm4_synth_ref2npcm4_synth_ref",
	[IP2_CLK_NPCM5_256FS_TIMING_GEN] = "viva_npcm5_256fs_timing_gen2npcm5_256fs_timing_gen",
	[IP2_CLK_NPCM5_SYNTH_REF] = "viva_npcm5_synth_ref2npcm5_synth_ref",
	[IP2_CLK_PCM_CLK_216] = "viva_pcm_clk_2162pcm_clk_216",
	[IP2_CLK_PCM_CLK_AUPLL] = "viva_pcm_clk_aupll2pcm_clk_aupll",
	[IP2_CLK_R2_WB] = "viva_r2_wb2r2_wb",
	[IP2_CLK_R2_RIU_BRG] = "viva_riu_brg_r22r2_riu_brg",
	[IP2_CLK_CH6_SH] = "viva_sh2ch6_sh",
	[IP2_CLK_CH7_SH] = "viva_sh2ch7_sh",
	[IP2_CLK_CH8_SH] = "viva_sh2ch8_sh",
	[IP2_CLK_CH9_SH] = "viva_sh2ch9_sh",
	[IP2_CLK_SIF_ADC_CIC] = "viva_sif_adc_cic2sif_adc_cic",
	[IP2_CLK_SIF_ADC_R2B_FIFO_IN] = "viva_sif_adc_r2b_fifo_in2sif_adc_r2b_fifo_in",
	[IP2_CLK_SIF_ADC_R2B] = "viva_sif_adc_r2b2sif_adc_r2b",
	[IP2_CLK_SIF_CORDIC] = "viva_sif_cordic2sif_cordic",
	[IP2_CLK_SIF_SYNTH] = "viva_sif_synth2sif_synth",
	[IP2_CLK_SIN_GEN] = "viva_sin_gen2sin_gen",
	[IP2_CLK_SPDIF_RX_128FS] = "viva_spdif_rx_128fs2spdif_rx_128fs",
	[IP2_CLK_SPDIF_RX_DG] = "viva_spdif_rx_dg2spdif_rx_dg",
	[IP2_CLK_SPDIF_RX_SYNTH_REF] = "viva_spdif_rx_synth_ref2spdif_rx_synth_ref",
	[IP2_CLK_SPDIF_TX_128FS] = "viva_spdif_tx_128fs2spdif_tx_128fs",
	[IP2_CLK_SPDIF_TX_256FS] = "viva_spdif_tx_256fs2spdif_tx_256fs",
	[IP2_CLK_SPDIF_TX_SYNTH_REF] = "viva_spdif_tx_synth_ref2spdif_tx_synth_ref",
	[IP2_CLK_SPDIF_TX2_128FS] = "viva_spdif_tx2_128fs2spdif_tx2_128fs",
	[IP2_CLK_SPDIF_TX2_256FS] = "viva_spdif_tx2_256fs2spdif_tx2_256fs",
	[IP2_CLK_SYNTH_2ND_ORDER] = "viva_synth_2nd_order2synth_2nd_order",
	[IP2_CLK_SYNTH_CODEC] = "viva_synth_codec2synth_codec",
	[IP2_CLK_TEST_BUS] = "viva_test_bus2test_bus",
	[IP2_CLK_XTAL_12M] = "viva_xtal_12m2xtal",
	[IP2_CLK_XTAL_24M] = "viva_xtal_24m2xtal",
	[IP2_CLK_CH_M1_256FSI] = "viva_ch_m1_256fsi2ch_m1_256fsi",
	[IP2_CLK_CH_M2_256FSI] = "viva_ch_m2_256fsi2ch_m2_256fsi",
	[IP2_CLK_CH_M3_256FSI] = "viva_ch_m3_256fsi2ch_m3_256fsi",
	[IP2_CLK_CH_M4_256FSI] = "viva_ch_m4_256fsi2ch_m4_256fsi",
	[IP2_CLK_CH_M5_256FSI] = "viva_ch_m5_256fsi2ch_m5_256fsi",
	[IP2_CLK_CH10_SH] = "viva_sh2ch10_sh",
	[IP2_CLK_CH_M1_SH] = "viva_sh2ch_m1_sh",
	[IP2_CLK_CH_M2_SH] = "viva_sh2ch_m2_sh",
	[IP2_CLK_CH_M3_SH] = "viva_sh2ch_m3_sh",
	[IP2_CLK_CH_M4_SH] = "viva_sh2ch_m4_sh",
	[IP2_CLK_CH_M5_SH] = "viva_sh2ch_m5_sh",
	[IP2_CLK_SRC_MAC_BSPLINE] = "viva_src_mac_bspline2src_mac_bspline",
};

struct clk_type {
	unsigned int type;
	unsigned int value;
};

// default mux clock table
static const struct clk_type mux_clk_table[] = {
	/* set mux clk : viva_riu_brg_r2_int_ck from dtsi to 1 */
	{MUX_RIU_BRG_R2, 1},
	/* set mux clk : viva_riu_r2_int_ck from dtsi to 0 */
	{MUX_RIU_R2, 0},
	/* set mux clk : viva_au_mcu_int_ck from dtsi to 1 */
	{MUX_AU_MCU, 1},
	/* set mux clk : au_mcu_bus_int_ck from dtsi to 1 */
	{MUX_AU_MCU_BUS, 1},
	/* set mux clk : viva_synth_codec_int_ck from dtsi to 1 */
	{MUX_SYNTH_CODEC, 1},
	/* set mux clk : viva_synth_2nd_order_int_ck from dtsi to 1 */
	{MUX_SYNTH_2ND_ORDER, 1},
	/* set mux clk : viva_aupll_in_int_ck from dtsi to 2 */
	{MUX_AUPLL_IN, 2},
	/* set mux clk : viva_dsp_int_ck from dtsi to 5 */
	{MUX_DSP_INT, 5},
	/* set mux clk : viva_sif_adc_r2b_fifo_in_int_ck from dtsi to 1 */
	{MUX_SIF_ADC_R2B_FIFO_IN, 1},
	/* set mux clk : viva_sif_adc_r2b_int_ck from dtsi to 1 */
	{MUX_SIF_ADC_R2B, 1},
	/* set mux clk : viva_sif_adc_cic_int_ck from dtsi to 1 */
	{MUX_SIF_ADC_CIC, 1},
	/* set mux clk : viva_fm_demodulator_int_ck from dtsi to 2 */
	{MUX_FM_DEMODULATOR, 2},
	/* set mux clk : viva_sif_synth_int_ck from dtsi to 1 */
	{MUX_SIF_SYNTH, 1},
	/* set mux clk : viva_sif_cordic_int_ck from dtsi to 4 */
	{MUX_SIF_CORDIC, 4},
	/* set mux clk : viva_parser_int_ck from dtsi to 1 */
	{MUX_PARSER_INT, 1},
	/* set mux clk : viva_gpa_480fso_int_ck from dtsi to 1 */
	{MUX_GPA_480FSO, 1},
	/* set mux clk : viva_gpa_240fso_int_ck from dtsi to 1 */
	{MUX_GPA_240FSO, 1},
	/* set mux clk : viva_gpa_256fso_int_ck from dtsi to 1 */
	{MUX_GPA_256FSO, 1},
	/* set mux clk : viva_adc_512fs_int_ck from dtsi to 1 */
	{MUX_ADC_512FS, 1},
	/* set mux clk : viva_dac_512fs_int_ck from dtsi to 1 */
	{MUX_DAC_512FS, 1},
	/* set mux clk : viva_ab_mac_int_ck from dtsi to 1 */
	{MUX_AB_MAC, 1},
	/* set mux clk : viva_ab3_mac_int_ck from dtsi to 4 */
	{MUX_AB3_MAC, 4},
	/* set mux clk : viva_sh_int_ck from dtsi to 1 */
	{MUX_SH, 1},
	/* set mux clk : viva_codec_sh_int_ck from dtsi to 1 */
	{MUX_CODEC_SH, 1},
	/* set mux clk : pcm_i2s_256fs_mux_int_ck from dtsi to 0 */
	{MUX_PCM_I2S_256FS_MUX, 0},
	/* set mux clk : ch_m1_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M1_256FSI_MUX1, 0},
	/* set mux clk : ch_m1_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M1_256FSI_MUX0, 0},
	/* set mux clk : ch_m1_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M1_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m1_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M1_256FSI, 1},
	/* set mux clk : ch_m2_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M2_256FSI_MUX1, 0},
	/* set mux clk : ch_m2_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M2_256FSI_MUX0, 0},
	/* set mux clk : ch_m2_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M2_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m2_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M2_256FSI, 1},
	/* set mux clk : ch_m3_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M3_256FSI_MUX1, 0},
	/* set mux clk : ch_m3_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M3_256FSI_MUX0, 0},
	/* set mux clk : ch_m3_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M3_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m3_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M3_256FSI, 1},
	/* set mux clk : ch_m4_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M4_256FSI_MUX1, 0},
	/* set mux clk : ch_m4_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M4_256FSI_MUX0, 0},
	/* set mux clk : ch_m4_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M4_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m4_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M4_256FSI, 1},
	/* set mux clk : ch_m5_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M5_256FSI_MUX1, 0},
	/* set mux clk : ch_m5_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M5_256FSI_MUX0, 0},
	/* set mux clk : ch_m5_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M5_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m5_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M5_256FSI, 1},
	/* set mux clk : ch_m6_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M6_256FSI_MUX1, 0},
	/* set mux clk : ch_m6_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M6_256FSI_MUX0, 0},
	/* set mux clk : ch_m6_256fsi_mux_int_ck from dtsi to 0*/
	{MUX_CH_M6_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m6_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M6_256FSI, 1},
	/* set mux clk : ch_m7_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M7_256FSI_MUX1, 0},
	/* set mux clk : ch_m7_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M7_256FSI_MUX0, 0},
	/* set mux clk : ch_m7_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M7_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m7_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M7_256FSI, 1},
	/* set mux clk : ch_m8_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M8_256FSI_MUX1, 0},
	/* set mux clk : ch_m8_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M8_256FSI_MUX0, 0},
	/* set mux clk : ch_m8_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M8_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m8_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M8_256FSI, 1},
	/* set mux clk : ch_m9_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M9_256FSI_MUX1, 0},
	/* set mux clk : ch_m9_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M9_256FSI_MUX0, 2},
	/* set mux clk : ch_m9_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M9_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m9_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M9_256FSI, 1},
	/* set mux clk : ch_m10_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M10_256FSI_MUX1, 0},
	/* set mux clk : ch_m10_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M10_256FSI_MUX0, 2},
	/* set mux clk : ch_m10_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M10_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m10_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M10_256FSI, 1},
	/* set mux clk : ch_m11_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M11_256FSI_MUX1, 0},
	/* set mux clk : ch_m11_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M11_256FSI_MUX0, 2},
	/* set mux clk : ch_m11_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M11_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m11_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M11_256FSI, 1},
	/* set mux clk : ch_m12_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M12_256FSI_MUX1, 0},
	/* set mux clk : ch_m12_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M12_256FSI_MUX0, 2},
	/* set mux clk : ch_m12_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M12_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m12_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M12_256FSI, 1},
	/* set mux clk : ch_m13_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M13_256FSI_MUX1, 0},
	/* set mux clk : ch_m13_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M13_256FSI_MUX0, 2},
	/* set mux clk : ch_m13_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M13_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m13_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M13_256FSI, 1},
	/* set mux clk : ch_m14_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M14_256FSI_MUX1, 0},
	/* set mux clk : ch_m14_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M14_256FSI_MUX0, 2},
	/* set mux clk : ch_m14_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M14_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m14_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M14_256FSI, 1},
	/* set mux clk : ch_m15_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M15_256FSI_MUX1, 0},
	/* set mux clk : ch_m15_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M15_256FSI_MUX0, 0},
	/* set mux clk : ch_m15_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M15_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m15_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M15_256FSI, 1},
	/* set mux clk : ch_m16_256fsi_mux1_int_ck from dtsi to 0 */
	{MUX_CH_M16_256FSI_MUX1, 0},
	/* set mux clk : ch_m16_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M16_256FSI_MUX0, 0},
	/* set mux clk : ch_m16_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M16_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m16_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M16_256FSI, 1},
	/* set mux clk : ch5_256fsi_mux2_int_ck from dtsi to 0 */
	{MUX_CH5_256FSI_MUX2, 0},
	/* set mux clk : ch5_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH5_256FSI_MUX1, 4},
	/* set mux clk : ch5_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH5_256FSI_MUX0, 0},
	/* set mux clk : ch5_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH5_256FSI_MUX, 1},
	/* set mux clk : viva_ch5_256fsi_int_ck from dtsi to 1 */
	{MUX_CH5_256FSI, 1},
	/* set mux clk : ch6_256fsi_mux2_int_ck from dtsi to 0 */
	{MUX_CH6_256FSI_MUX2, 0},
	/* set mux clk : ch6_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH6_256FSI_MUX1, 4},
	/* set mux clk : ch6_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH6_256FSI_MUX0, 0},
	/* set mux clk : ch6_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH6_256FSI_MUX, 1},
	/* set mux clk : viva_ch6_256fsi_int_ck from dtsi to 1 */
	{MUX_CH6_256FSI, 1},
	/* set mux clk : ch7_256fsi_mux2_int_ck from dtsi to 0 */
	{MUX_CH7_256FSI_MUX2, 0},
	/* set mux clk : ch7_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH7_256FSI_MUX1, 4},
	/* set mux clk : ch7_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH7_256FSI_MUX0, 0},
	/* set mux clk : ch7_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH7_256FSI_MUX, 1},
	/* set mux clk : viva_ch7_256fsi_int_ck from dtsi to 1 */
	{MUX_CH7_256FSI, 1},
	/* set mux clk : ch8_256fsi_mux2_int_ck from dtsi to 0 */
	{MUX_CH8_256FSI_MUX2, 0},
	/* set mux clk : ch8_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH8_256FSI_MUX1, 4},
	/* set mux clk : ch8_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH8_256FSI_MUX0, 0},
	/* set mux clk : ch8_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH8_256FSI_MUX, 1},
	/* set mux clk : viva_ch8_256fsi_int_ck from dtsi to 1 */
	{MUX_CH8_256FSI, 1},
	/* set mux clk : ch9_256fsi_mux2_int_ck from dtsi to 0 */
	{MUX_CH9_256FSI_MUX2, 0},
	/* set mux clk : ch9_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH9_256FSI_MUX1, 4},
	/* set mux clk : ch9_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH9_256FSI_MUX0, 0},
	/* set mux clk : ch9_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH9_256FSI_MUX, 1},
	/* set mux clk : viva_ch9_256fsi_int_ck from dtsi to 1 */
	{MUX_CH9_256FSI, 1},
	/* set mux clk : ch10_256fsi_mux2_int_ck from dtsi to 0 */
	{MUX_CH10_256FSI_MUX2, 0},
	/* set mux clk : ch10_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH10_256FSI_MUX1, 4},
	/* set mux clk : ch10_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH10_256FSI_MUX0, 0},
	/* set mux clk : ch10_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH10_256FSI_MUX, 1},
	/* set mux clk : viva_ch10_256fsi_int_ck from dtsi to 1 */
	{MUX_CH10_256FSI, 1},
	/* set mux clk : viva_dvb1_synth_ref_int_ck from dtsi to 1 */
	{MUX_DVB1_SYNTH_REF, 1},
	/* set mux clk : decoder1_synth_hdmi_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER1_SYNTH_HDMI_CTSN_256FS, 0},
	/* set mux clk : decoder1_synth_hdmi_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER1_SYNTH_HDMI_CTSN_128FS, 0},
	/* set mux clk : decoder1_synth_hdmi2_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER1_SYNTH_HDMI2_CTSN_256FS, 0},
	/* set mux clk : decoder1_synth_hdmi2_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER1_SYNTH_HDMI2_CTSN_128FS, 0},
	/* set mux clk : decoder1_synth_spdif_cdr_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER1_SYNTH_SPDIF_CDR_256FS, 0},
	/* set mux clk : decoder1_synth_spdif_cdr_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER1_SYNTH_SPDIF_CDR_128FS, 0},
	/* set mux clk : decoder1_256fs_mux0_int_ck from dtsi to 0 */
	{MUX_DECODER1_256FS_MUX0, 0},
	/* set mux clk : decoder1_256fs_mux1_int_ck from dtsi to 0 */
	{MUX_DECODER1_256FS_MUX1, 0},
	/* set mux clk : decoder1_256fs_mux_int_ck from dtsi to 0 */
	{MUX_DECODER1_256FS_MUX, 0},
	/* set mux clk : viva_dvb1_timing_gen_256fs_int_ck from dtsi to 1 */
	{MUX_DVB1_TIMING_GEN_256FS, 1},
	/* set mux clk : decoder1_128fs_mux0_int_ck from dtsi to 0 */
	{MUX_DECODER1_128FS_MUX0, 0},
	/* set mux clk : decoder1_128fs_mux1_int_ck from dtsi to 0 */
	{MUX_DECODER1_128FS_MUX1, 0},
	/* set mux clk : decoder1_128fs_mux_int_ck from dtsi to 0 */
	{MUX_DECODER1_128FS_MUX, 0},
	/* set mux clk : viva_dvb2_synth_ref_int_ck from dtsi to 1 */
	{MUX_DVB2_SYNTH_REF, 1},
	/* set mux clk : decoder2_synth_hdmi_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER2_SYNTH_HDMI_CTSN_256FS, 0},
	/* set mux clk : decoder2_synth_hdmi_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER2_SYNTH_HDMI_CTSN_128FS, 0},
	/* set mux clk : decoder2_synth_hdmi2_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER2_SYNTH_HDMI2_CTSN_256FS, 0},
	/* set mux clk : decoder2_synth_hdmi2_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER2_SYNTH_HDMI2_CTSN_128FS, 0},
	/* set mux clk : decoder2_synth_spdif_cdr_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER2_SYNTH_SPDIF_CDR_256FS, 0},
	/* set mux clk : decoder2_synth_spdif_cdr_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER2_SYNTH_SPDIF_CDR_128FS, 0},
	/* set mux clk : decoder2_256fs_mux0_int_ck from dtsi to 0 */
	{MUX_DECODER2_256FS_MUX0, 0},
	/* set mux clk : decoder2_256fs_mux1_int_ck from dtsi to 0 */
	{MUX_DECODER2_256FS_MUX1, 0},
	/* set mux clk : decoder2_256fs_mux_int_ck from dtsi to 0 */
	{MUX_DECODER2_256FS_MUX, 0},
	/* set mux clk : viva_dvb2_timing_gen_256fs_int_ck from dtsi to 1 */
	{MUX_DVB2_TIMING_GEN_256FS, 1},
	/* set mux clk : decoder2_128fs_mux0_int_ck from dtsi to 0 */
	{MUX_DECODER2_128FS_MUX0, 0},
	/* set mux clk : decoder2_128fs_mux1_int_ck from dtsi to 0 */
	{MUX_DECODER2_128FS_MUX1, 0},
	/* set mux clk : decoder2_128fs_mux_int_ck from dtsi to 0 */
	{MUX_DECODER2_128FS_MUX, 0},
	/* set mux clk : viva_dvb3_synth_ref_int_ck from dtsi to 1 */
	{MUX_DVB3_SYNTH_REF, 1},
	/* set mux clk : decoder3_synth_hdmi_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER3_SYNTH_HDMI_CTSN_256FS, 0},
	/* set mux clk : decoder3_synth_hdmi_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER3_SYNTH_HDMI_CTSN_128FS, 0},
	/* set mux clk : decoder3_synth_hdmi2_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER3_SYNTH_HDMI2_CTSN_256FS, 0},
	/* set mux clk : decoder3_synth_hdmi2_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER3_SYNTH_HDMI2_CTSN_128FS, 0},
	/* set mux clk : decoder3_synth_spdif_cdr_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER3_SYNTH_SPDIF_CDR_256FS, 0},
	/* set mux clk : decoder3_synth_spdif_cdr_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER3_SYNTH_SPDIF_CDR_128FS, 0},
	/* set mux clk : decoder3_256fs_mux0_int_ck from dtsi to 0 */
	{MUX_DECODER3_256FS_MUX0, 0},
	/* set mux clk : decoder3_256fs_mux1_int_ck from dtsi to 0 */
	{MUX_DECODER3_256FS_MUX1, 0},
	/* set mux clk : decoder3_256fs_mux_int_ck from dtsi to 0 */
	{MUX_DECODER3_256FS_MUX, 0},
	/* set mux clk : viva_dvb3_timing_gen_256fs_int_ck from dtsi to 1 */
	{MUX_DVB3_TIMING_GEN_256FS, 1},
	/* set mux clk : decoder3_128fs_mux0_int_ck from dtsi to 0 */
	{MUX_DECODER3_128FS_MUX0, 0},
	/* set mux clk : decoder3_128fs_mux1_int_ck from dtsi to 0 */
	{MUX_DECODER3_128FS_MUX1, 0},
	/* set mux clk : decoder3_128fs_mux_int_ck from dtsi to 0 */
	{MUX_DECODER3_128FS_MUX, 0},
	/* set mux clk : viva_dvb4_synth_ref_int_ck from dtsi to 1 */
	{MUX_DVB4_SYNTH_REF, 1},
	/* set mux clk : decoder4_synth_hdmi_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER4_SYNTH_HDMI_CTSN_256FS, 0},
	/* set mux clk : decoder4_synth_hdmi_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER4_SYNTH_HDMI_CTSN_128FS, 0},
	/* set mux clk : decoder4_synth_hdmi2_ctsn_256fs_div_mux_int_ck from dtsi to 0  */
	{MUX_DECODER4_SYNTH_HDMI2_CTSN_256FS, 0},
	/* set mux clk : decoder4_synth_hdmi2_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER4_SYNTH_HDMI2_CTSN_128FS, 0},
	/* set mux clk : decoder4_synth_spdif_cdr_256fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER4_SYNTH_SPDIF_CDR_256FS, 0},
	/* set mux clk : decoder4_synth_spdif_cdr_128fs_div_mux_int_ck from dtsi to 0 */
	{MUX_DECODER4_SYNTH_SPDIF_CDR_128FS, 0},
	/* set mux clk : decoder4_256fs_mux0_int_ck from dtsi to 7 */
	{MUX_DECODER4_256FS_MUX0, 7},
	/* set mux clk : decoder4_256fs_mux1_int_ck from dtsi to 0 */
	{MUX_DECODER4_256FS_MUX1, 0},
	/* set mux clk : decoder4_256fs_mux_int_ck from dtsi to 0 */
	{MUX_DECODER4_256FS_MUX, 0},
	/* set mux clk : viva_dvb4_timing_gen_256fs_int_ck from dtsi to 1 */
	{MUX_DVB4_TIMING_GEN_256FS, 1},
	/* set mux clk : decoder4_128fs_mux0_int_ck from dtsi to 7 */
	{MUX_DECODER4_128FS_MUX0, 7},
	/* set mux clk : decoder4_128fs_mux1_int_ck from dtsi to 0 */
	{MUX_DECODER4_128FS_MUX1, 0},
	/* set mux clk : decoder4_128fs_mux_int_ck from dtsi to 0 */
	{MUX_DECODER4_128FS_MUX, 0},
	/* set mux clk : viva_dvb5_timing_gen_256fs_int_ck from dtsi to 1 */
	{MUX_DVB5_TIMING_GEN_256FS, 1},
	/* set mux clk : viva_dvb6_timing_gen_256fs_int_ck from dtsi to 1 */
	{MUX_DVB6_TIMING_GEN_256FS, 1},
	/* set mux clk : viva_audma_v2_r1_synth_ref_int_ck from dtsi to 1 */
	{MUX_AUDMA_V2_R1_SYNTH_REF, 1},
	/* set mux clk : viva_audma_v2_r1_256fs_mux_int_ck from dtsi to 1 */
	{MUX_AUDMA_V2_R1_256FS_MUX, 1},
	/* set mux clk : viva_audma_v2_r1_256fs_int_ck from dtsi to 1 */
	{MUX_AUDMA_V2_R1_256FS, 1},
	/* set mux clk : viva_audma_v2_r2_synth_ref_int_ck from dtsi to 1 */
	{MUX_AUDMA_V2_R2_SYNTH_REF, 1},
	/* set mux clk : viva_audma_v2_r2_256fs_int_ck from dtsi to 1 */
	{MUX_AUDMA_V2_R2_256FS, 1},
	/* set mux clk : viva_spdif_rx_128fs_int_ck from dtsi to 1 */
	{MUX_SPDIF_RX_128FS, 1},
	/* set mux clk : viva_spdif_rx_dg_int_ck from dtsi to 1 */
	{MUX_SPDIF_RX_DG, 1},
	/* set mux clk : viva_spdif_rx_synth_ref_int_ck from dtsi to 3 */
	{MUX_SPDIF_RX_SYNTH_REF, 3},
	/* set mux clk : viva_hdmi_rx_synth_ref_int_ck from dtsi to 1 */
	{MUX_HDMI_RX_SYNTH_REF, 1},
	/* set mux clk : hdmi_rx_s2p_256fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_RX_S2P_256FS_MUX, 0},
	/* set mux clk : viva_hdmi_rx_s2p_256fs_int_ck from dtsi to 1 */
	{MUX_HDMI_RX_S2P_256FS, 1},
	/* set mux clk : hdmi_rx_s2p_128fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_RX_S2P_128FS_MUX, 0},
	/* set mux clk : viva_hdmi_rx_s2p_128fs_int_ck from dtsi to 1 */
	{MUX_HDMI_RX_S2P_128FS, 1},
	/* set mux clk : viva_hdmi_rx_mpll_432_int_ck from dtsi to 1 */
	{MUX_HDMI_RX_MPLL_432, 1},
	/* set mux clk : viva_hdmi_rx2_synth_ref_int_ck from dtsi to 1 */
	{MUX_HDMI_RX2_SYNTH_REF, 1},
	/* set mux clk : hdmi_rx2_s2p_256fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_RX2_S2P_256FS_MUX, 0},
	/* set mux clk : viva_hdmi_rx2_s2p_256fs_int_ck from dtsi to 1 */
	{MUX_HDMI_RX2_S2P_256FS, 1},
	/* set mux clk : hdmi_rx2_s2p_128fs_mux_int_ck from dtsi to 0  */
	{MUX_HDMI_RX2_S2P_128FS_MUX, 0},
	/* set mux clk : viva_hdmi_rx2_s2p_128fs_int_ck from dtsi to 1 */
	{MUX_HDMI_RX2_S2P_128FS, 1},
	/* set mux clk : viva_hdmi_rx2_mpll_432_int_ck from dtsi to 1 */
	{MUX_HDMI_RX2_MPLL_432, 1},
	/* set mux clk : viva_bclk_i2s_decoder_int_ck from dtsi to 1 */
	{MUX_BCLK_I2S_DECODER, 1},
	/* set mux clk : viva_mclk_i2s_decoder_int_ck from dtsi to 5 */
	{MUX_MCLK_I2S_DECODER, 5},
	/* set mux clk : viva_i2s_bclk_dg_int_ck from dtsi to 1 */
	{MUX_I2S_BCLK_DG, 1},
	/* set mux clk : viva_i2s_synth_ref_int_ck from dtsi to 3 */
	{MUX_I2S_SYNTH_REF, 3},
	/* set mux clk : viva_bclk_i2s_decoder2_int_ck from dtsi to 1 */
	{MUX_BCLK_I2S_DECODER2, 1},
	/* set mux clk : viva_mclk_i2s_decoder2_int_ck from dtsi to 5 */
	{MUX_MCLK_I2S_DECODER2, 5},
	/* set mux clk : viva_i2s_bclk_dg2_int_ck from dtsi to 1 */
	{MUX_I2S_BCLK_DG2, 1},
	/* set mux clk : viva_bi2s_synth_ref2_int_ck from dtsi to 3 */
	{MUX_BI2S_SYNTH_REF2, 3},
	/* set mux clk : hdmi_tx_to_i2s_tx_256fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_TX_TO_I2S_TX_256FS_MUX, 0},
	/* set mux clk : hdmi_tx_to_i2s_tx_128fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_TX_TO_I2S_TX_128FS_MUX, 0},
	/* set mux clk : hdmi_tx_to_i2s_tx_64fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_TX_TO_I2S_TX_64FS_MUX, 0},
	/* set mux clk : hdmi_tx_to_i2s_tx_32fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_TX_TO_I2S_TX_32FS_MUX, 0},
	/* set mux clk : hdmi_rx_to_i2s_tx_256fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_RX_TO_I2S_TX_256FS_MUX, 0},
	/* set mux clk : hdmi_rx_to_i2s_tx_128fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_RX_TO_I2S_TX_128FS_MUX, 0},
	/* set mux clk : hdmi_rx_to_i2s_tx_64fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_RX_TO_I2S_TX_64FS_MUX, 0},
	/* set mux clk : hdmi_rx_to_i2s_tx_32fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_RX_TO_I2S_TX_32FS_MUX, 0},
	/* set mux clk : viva_i2s_tx_fifo_256fs_int_ck from dtsi to 1 */
	{MUX_I2S_TX_FIFO_256FS, 1},
	/* set mux clk : viva_i2s_mclk_synth_ref_int_ck from dtsi to 1 */
	{MUX_I2S_MCLK_SYNTH_REF, 1},
	/* set mux clk : viva_mclk_i2s_encoder_int_ck from dtsi to 4 */
	{MUX_MCLK_I2S_ENCODER, 4},
	/* set mux clk : i2s_bclk_synth_ref_mux_int_ck from dtsi to 0 */
	{MUX_I2S_BCLK_SYNTH_REF_MUX, 0},
	/* set mux clk : viva_i2s_bclk_synth_ref_int_ck from dtsi to 1 */
	{MUX_I2S_BCLK_SYNTH_REF, 1},
	/* set mux clk : bclk_i2s_encoder_128fs_ssc_mux_int_ck from dtsi to 0 */
	{MUX_BCLK_I2S_ENCODER_128FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder_64fs_ssc_mux_int_ck from dtsi to 0 */
	{MUX_BCLK_I2S_ENCODER_64FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder_32fs_ssc_mux_int_ck from dtsi to 0 */
	{MUX_BCLK_I2S_ENCODER_32FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder_mux_int_ck from dtsi to 4 */
	{MUX_BCLK_I2S_ENCODER_MUX, 4},
	/* set mux clk : bclk_i2s_encoder_int_ck from dtsi to 1 */
	{MUX_BCLK_I2S_ENCODER, 1},
	/* set mux clk : viva_i2s_tx_2_fifo_256fs_int_ck from dtsi to 1 */
	{MUX_I2S_TX_2_FIFO_256FS, 1},
	/* set mux clk : viva_mclk_i2s_encoder2_int_ck from dtsi to 7 */
	{MUX_MCLK_I2S_ENCODER2, 7},
	/* set mux clk : i2s_2_bclk_synth_ref_mux_int_ck from dtsi to 0 */
	{MUX_I2S_2_BCLK_SYNTH_REF_MUX, 0},
	/* set mux clk : viva_i2s_2_bclk_synth_ref_int_ck from dtsi to 1 */
	{MUX_I2S_2_BCLK_SYNTH_REF, 1},
	/* set mux clk : bclk_i2s_encoder2_128fs_ssc_mux_int_ck from dtsi to 0 */
	{MUX_BCLK_I2S_ENCODER2_128FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder2_64fs_ssc_mux_int_ck from dtsi to 0 */
	{MUX_BCLK_I2S_ENCODER2_64FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder2_32fs_ssc_mux_int_ck from dtsi to 0 */
	{MUX_BCLK_I2S_ENCODER2_32FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder2_mux_int_ck from dtsi to 4 */
	{MUX_BCLK_I2S_ENCODER2_MUX, 4},
	/* set mux clk : bclk_i2s_encoder2_int_ck from dtsi to 1 */
	{MUX_BCLK_I2S_ENCODER2, 1},
	/* set mux clk : viva_i2s_trx_synth_ref_int_ck from dtsi to 1 */
	{MUX_I2S_TRX_SYNTH_REF, 1},
	/* set mux clk : viva_i2s_trx_bclk_timing_gen_int_ck from dtsi to 1 */
	{MUX_I2S_TRX_BCLK_TIMING_GEN, 1},
	/* set mux clk : viva_pcm_clk_216_int_ck from dtsi to 1 */
	{MUX_PCM_CLK_216, 1},
	/* set mux clk : viva_pcm_clk_aupll_int_ck from dtsi to 1 */
	{MUX_PCM_CLK_AUPLL, 1},
	/* set mux clk : spdif_tx_gpa_synth_256fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_TX_GPA_SYNTH_256FS_MUX, 0},
	/* set mux clk : spdif_tx_256fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_TX_256FS_MUX, 0},
	/* set mux clk : viva_spdif_tx_256fs_int_ck from dtsi to 1 */
	{MUX_SPDIF_TX_256FS, 1},
	/* set mux clk : spdif_tx_gpa_128fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_TX_GPA_128FS_MUX, 0},
	/* set mux clk : spdif_tx_128fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_TX_128FS_MUX, 0},
	/* set mux clk : viva_spdif_tx_128fs_int_ck from dtsi to 1 */
	{MUX_SPDIF_TX_128FS, 1},
	/* set mux clk : viva_spdif_tx_synth_ref_int_ck from dtsi to 1 */
	{MUX_SPDIF_TX_SYNTH_REF, 1},
	/* set mux clk : spdif_tx2_gpa_synth_256fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_TX2_GPA_SYNTH_256FS_MUX, 0},
	/* set mux clk : spdif_tx2_256fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_TX2_256FS_MUX, 0},
	/* set mux clk : viva_spdif_tx2_256fs_int_ck from dtsi to 1 */
	{MUX_SPDIF_TX2_256FS, 1},
	/* set mux clk : spdif_tx2_gpa_128fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_TX2_GPA_128FS_MUX, 0},
	/* set mux clk : spdif_tx2_128fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_TX2_128FS_MUX, 0},
	/* set mux clk : viva_spdif_tx2_128fs_int_ck from dtsi to 1 */
	{MUX_SPDIF_TX2_128FS, 1},
	/* set mux clk : viva_npcm_synth_ref_int_ck from dtsi to 1 */
	{MUX_NPCM_SYNTH_REF, 1},
	/* set mux clk : viva_npcm2_synth_ref_int_ck from dtsi to 1 */
	{MUX_NPCM2_SYNTH_REF, 1},
	/* set mux clk : viva_npcm3_synth_ref_int_ck from dtsi to 1 */
	{MUX_NPCM3_SYNTH_REF, 1},
	/* set mux clk : viva_npcm4_synth_ref_int_ck from dtsi to 1 */
	{MUX_NPCM4_SYNTH_REF, 1},
	/* set mux clk : viva_npcm4_256fs_timing_gen_int_ck from dtsi to 1 */
	{MUX_NPCM4_256FS_TIMING_GEN, 1},
	/* set mux clk : viva_npcm5_synth_ref_int_ck from dtsi to 1 */
	{MUX_NPCM5_SYNTH_REF, 1},
	/* set mux clk : viva_npcm5_256fs_timing_gen_int_ck from dtsi to 1 */
	{MUX_NPCM5_256FS_TIMING_GEN, 1},
	/* set mux clk : hdmi_tx_256fs_mux1_int_ck from dtsi to 0 */
	{MUX_HDMI_TX_256FS_MUX1, 0},
	/* set mux clk : hdmi_tx_256fs_mux0_int_ck from dtsi to 0 */
	{MUX_HDMI_TX_256FS_MUX0, 0},
	/* set mux clk : hdmi_tx_256fs_mux_int_ck from dtsi to 0 */
	{MUX_HDMI_TX_256FS_MUX, 0},
	/* set mux clk : viva_hdmi_tx_256fs_int_ck from dtsi to 1 */
	{MUX_HDMI_TX_256FS, 1},
	/* set mux clk : viva_hdmi_tx_128fs_int_ck from dtsi to 1 */
	{MUX_HDMI_TX_128FS, 1},
	/* set mux clk : spdif_extearc_256fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_EXTEARC_256FS_MUX, 0},
	/* set mux clk : spdif_extearc_128fs_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_EXTEARC_128FS_MUX, 0},
	/* set mux clk : spdif_extearc_mux0_int_ck from dtsi to 0 */
	{MUX_SPDIF_EXTEARC_MUX0, 0},
	/* set mux clk : spdif_extearc_mux_int_ck from dtsi to 0 */
	{MUX_SPDIF_EXTEARC_MUX, 0},
	/* set mux clk : viva_spdif_extearc_int_ck from dtsi to 1 */
	{MUX_SPDIF_EXTEARC, 1},
	/* set mux clk : viva_earc_tx_synth_ref_int_ck from dtsi to 2 */
	{MUX_EARC_TX_SYNTH_REF, 2},
	/* set mux clk : viva_earc_tx_gpa_synth_ref_int_ck from dtsi to 1 */
	{MUX_EARC_TX_GPA_SYNTH_REF, 1},
	/* set mux clk : viva_earc_tx_synth_iir_ref_int_ck from dtsi to 1 */
	{MUX_EARC_TX_SYNTH_IIR_REF, 1},
	/* set mux clk : earc_tx_gpa_256fs_mux_int_ck from dtsi to 0 */
	{MUX_EARC_TX_GPA_256FS_MUX, 0},
	/* set mux clk : earc_tx_pll_256fs_ref_mux_int_ck from dtsi to 0 */
	{MUX_EARC_TX_PLL_256FS_REF_MUX, 0},
	/* set mux clk : earc_tx_pll_256fs_ref_int_ck from dtsi to 1 */
	{MUX_EARC_TX_PLL_256FS_REF, 1},
	/* set mux clk : viva_earc_tx_pll_int_ck from dtsi to 1 */
	{MUX_EARC_TX_PLL, 1},
	/* set mux clk : viva_earc_tx_64xchxfs_int_ck from dtsi to 1 */
	{MUX_EARC_TX_64XCHXFS, 1},
	/* set mux clk : viva_earc_tx_32xchxfs_int_ck from dtsi to 1 */
	{MUX_EARC_TX_32XCHXFS, 1},
	/* set mux clk : viva_earc_tx_32xfs_int_ck from dtsi to 1 */
	{MUX_EARC_TX_32XFS, 1},
	/* set mux clk : earc_tx_source_256fs_mux_int_ck from dtsi to 0 */
	{MUX_EARC_TX_SOURCE_256FS_MUX, 0},
	/* set mux clk : viva_earc_tx_source_256fs_int_ck from dtsi to 1 */
	{MUX_EARC_TX_SOURCE_256FS, 1},
	/* set mux clk : viva_sin_gen_int_ck from dtsi to 1 */
	{MUX_SIN_GEN, 1},
	/* set mux clk : viva_test_bus_int_ck from dtsi to 1 */
	{MUX_TEST_BUS, 1},
	/* set mux clk : viva_src_mac_bspline_int_ck from dtsi to 1 */
	{MUX_SRC_MAC_BSPLINE, 1},
	/* set mux clk : viva_i2s_tdm_6x16tx32b_bclk_int_ck from dtsi to 1 */
	{MUX_I2S_TDM_6X16TX32B_BCLK, 1},
	/* set mux clk : viva_i2s_tdm_6x16tx32b_fifo_int_ck from dtsi to 1 */
	{MUX_I2S_TDM_6X16TX32B_FIFO, 1},
	/* set mux clk : viva_i2s_tdm_6x16tx32b_bclk_to_pad_int_ck from dtsi to 1 */
	{MUX_I2S_TDM_6X16TX32B_BCLK_TO_PAD, 1},
	/* set mux clk : viva_i2s_tdm_1x16tx32b_bclk_int_ck from dtsi to 1 */
	{MUX_I2S_TDM_1X16TX32B_BCLK, 1},
	/* set mux clk : viva_i2s_tdm_1x16tx32b_fifo_int_ck from dtsi to 1 */
	{MUX_I2S_TDM_1X16TX32B_FIFO, 1},
	/* set mux clk : viva_i2s_tdm_1x16tx32b_bclk_to_pad_int_ck from dtsi to 1 */
	{MUX_I2S_TDM_1X16TX32B_BCLK_TO_PAD, 1},

	{END_OF_TABLE, 0x00}, // end of table
};

// default ip2 mux clock table
static const struct clk_type ip2_mux_clk_table[] = {
	/* set mux clk : viva_riu_brg_r2_int_ck from dtsi to 1 */
	{IP2_MUX_RIU_BRG_R2, 1},
	/* set mux clk : viva_au_mcu_int_ck from dtsi to 1 */
	{IP2_MUX_AU_MCU, 1},
	/* set mux clk : au_mcu_bus_int_ck from dtsi to 1 */
	{IP2_MUX_AU_MCU_BUS, 1},
	/* set mux clk : viva_synth_codec_int_ck from dtsi to 1 */
	{IP2_MUX_SYNTH_CODEC, 1},
	/* set mux clk : viva_synth_2nd_order_int_ck from dtsi to 1 */
	{IP2_MUX_SYNTH_2ND_ORDER, 1},
	/* set mux clk : viva_aupll_in_int_ck from dtsi to 2 */
	{IP2_MUX_AUPLL_IN, 2},
	/* set mux clk : viva_dsp_int_ck from dtsi to 5 */
	{IP2_MUX_DSP_INT, 5},
	/* set mux clk : viva_sif_adc_r2b_fifo_in_int_ck from dtsi to 1 */
	{IP2_MUX_SIF_ADC_R2B_FIFO_IN, 1},
	/* set mux clk : viva_sif_adc_r2b_int_ck from dtsi to 1 */
	{IP2_MUX_SIF_ADC_R2B, 1},
	/* set mux clk : viva_sif_adc_cic_int_ck from dtsi to 1 */
	{IP2_MUX_SIF_ADC_CIC, 1},
	/* set mux clk : viva_fm_demodulator_int_ck from dtsi to 2 */
	{IP2_MUX_FM_DEMODULATOR, 2},
	/* set mux clk : viva_sif_synth_int_ck from dtsi to 1 */
	{IP2_MUX_SIF_SYNTH, 1},
	/* set mux clk : viva_sif_cordic_int_ck from dtsi to 4 */
	{IP2_MUX_SIF_CORDIC, 4},
	/* set mux clk : viva_parser_int_ck from dtsi to 1 */
	{IP2_MUX_PARSER_INT, 1},
	/* set mux clk : viva_gpa_480fso_int_ck from dtsi to 1 */
	{IP2_MUX_GPA_480FSO, 1},
	/* set mux clk : viva_gpa_240fso_int_ck from dtsi to 1 */
	{IP2_MUX_GPA_240FSO, 1},
	/* set mux clk : viva_gpa_256fso_int_ck from dtsi to 1 */
	{IP2_MUX_GPA_256FSO, 1},
	/* set mux clk : viva_adc_512fs_int_ck from dtsi to 1 */
	{IP2_MUX_ADC_512FS, 1},
	/* set mux clk : viva_dac_512fs_int_ck from dtsi to 1 */
	{IP2_MUX_DAC_512FS, 1},
	/* set mux clk : viva_ab_mac_int_ck from dtsi to 1 */
	{IP2_MUX_AB_MAC, 1},
	/* set mux clk : viva_ab_mac_10ch_int_ck from dtsi to 1 */
	{IP2_MUX_AB_MAC_10CH, 1},
	/* set mux clk : viva_ab3_mac_int_ck from dtsi to 4 */
	{IP2_MUX_AB3_MAC, 4},
	/* set mux clk : viva_sh_int_ck from dtsi to 1 */
	{IP2_MUX_SH, 1},
	/* set mux clk : viva_codec_sh_int_ck from dtsi to 1 */
	{IP2_MUX_CODEC_SH, 1},
	/* set mux clk : pcm_i2s_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_PCM_I2S_256FS_MUX, 0},
	/* set mux clk : ch_m1_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M1_256FSI_MUX2, 0},
	/* set mux clk : ch_m1_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M1_256FSI_MUX1, 0},
	/* set mux clk : ch_m1_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M1_256FSI_MUX0, 0},
	/* set mux clk : ch_m1_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M1_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m1_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M1_256FSI, 1},
	/* set mux clk : ch_m2_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M2_256FSI_MUX2, 0},
	/* set mux clk : ch_m2_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M2_256FSI_MUX1, 0},
	/* set mux clk : ch_m2_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M2_256FSI_MUX0, 0},
	/* set mux clk : ch_m2_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M2_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m2_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M2_256FSI, 1},
	/* set mux clk : ch_m3_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M3_256FSI_MUX2, 0},
	/* set mux clk : ch_m3_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M3_256FSI_MUX1, 0},
	/* set mux clk : ch_m3_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M3_256FSI_MUX0, 0},
	/* set mux clk : ch_m3_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M3_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m3_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M3_256FSI, 1},
	/* set mux clk : ch_m4_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M4_256FSI_MUX2, 0},
	/* set mux clk : ch_m4_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M4_256FSI_MUX1, 0},
	/* set mux clk : ch_m4_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M4_256FSI_MUX0, 0},
	/* set mux clk : ch_m4_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M4_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m4_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M4_256FSI, 1},
	/* set mux clk : ch_m5_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M5_256FSI_MUX2, 0},
	/* set mux clk : ch_m5_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M5_256FSI_MUX1, 0},
	/* set mux clk : ch_m5_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M5_256FSI_MUX0, 0},
	/* set mux clk : ch_m5_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M5_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m5_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M5_256FSI, 1},
	/* set mux clk : ch6_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH6_256FSI_MUX2, 0},
	/* set mux clk : ch6_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH6_256FSI_MUX1, 0},
	/* set mux clk : ch6_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH6_256FSI_MUX0, 0},
	/* set mux clk : ch6_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH6_256FSI_MUX, 0},
	/* set mux clk : viva_ch6_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH6_256FSI, 1},
	/* set mux clk : ch7_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH7_256FSI_MUX2, 0},
	/* set mux clk : ch7_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH7_256FSI_MUX1, 0},
	/* set mux clk : ch7_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH7_256FSI_MUX0, 0},
	/* set mux clk : ch7_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH7_256FSI_MUX, 0},
	/* set mux clk : viva_ch7_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH7_256FSI, 1},
	/* set mux clk : ch8_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH8_256FSI_MUX2, 0},
	/* set mux clk : ch8_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH8_256FSI_MUX1, 0},
	/* set mux clk : ch8_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH8_256FSI_MUX0, 0},
	/* set mux clk : ch8_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH8_256FSI_MUX, 0},
	/* set mux clk : viva_ch8_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH8_256FSI, 1},
	/* set mux clk : ch9_256fsi_mux2_int_ck from dtsi to 0 */
	{IP2_MUX_CH9_256FSI_MUX2, 0},
	/* set mux clk : ch9_256fsi_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_CH9_256FSI_MUX1, 0},
	/* set mux clk : ch9_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH9_256FSI_MUX0, 0},
	/* set mux clk : ch9_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH9_256FSI_MUX, 0},
	/* set mux clk : viva_ch9_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH9_256FSI, 1},
	/* set mux clk : viva_dvb1_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_DVB1_SYNTH_REF, 1},
	/* set mux clk : decoder1_synth_hdmi_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_SYNTH_HDMI_CTSN_256FS, 0},
	/* set mux clk : decoder1_synth_hdmi_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_SYNTH_HDMI_CTSN_128FS, 0},
	/* set mux clk : decoder1_synth_hdmi2_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_SYNTH_HDMI2_CTSN_256FS, 0},
	/* set mux clk : decoder1_synth_hdmi2_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_SYNTH_HDMI2_CTSN_128FS, 0},
	/* set mux clk : decoder1_synth_spdif_cdr_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_SYNTH_SPDIF_CDR_256FS, 0},
	/* set mux clk : decoder1_synth_spdif_cdr_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_SYNTH_SPDIF_CDR_128FS, 0},
	/* set mux clk : decoder1_256fs_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_256FS_MUX0, 0},
	/* set mux clk : decoder1_256fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_256FS_MUX1, 0},
	/* set mux clk : decoder1_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_256FS_MUX, 0},
	/* set mux clk : viva_dvb1_timing_gen_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_DVB1_TIMING_GEN_256FS, 1},
	/* set mux clk : decoder1_128fs_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_128FS_MUX0, 0},
	/* set mux clk : decoder1_128fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_128FS_MUX1, 0},
	/* set mux clk : decoder1_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER1_128FS_MUX, 0},
	/* set mux clk : viva_dvb2_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_DVB2_SYNTH_REF, 1},
	/* set mux clk : decoder2_synth_hdmi_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_SYNTH_HDMI_CTSN_256FS, 0},
	/* set mux clk : decoder2_synth_hdmi_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_SYNTH_HDMI_CTSN_128FS, 0},
	/* set mux clk : decoder2_synth_hdmi2_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_SYNTH_HDMI2_CTSN_256FS, 0},
	/* set mux clk : decoder2_synth_hdmi2_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_SYNTH_HDMI2_CTSN_128FS, 0},
	/* set mux clk : decoder2_synth_spdif_cdr_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_SYNTH_SPDIF_CDR_256FS, 0},
	/* set mux clk : decoder2_synth_spdif_cdr_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_SYNTH_SPDIF_CDR_128FS, 0},
	/* set mux clk : decoder2_256fs_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_256FS_MUX0, 0},
	/* set mux clk : decoder2_256fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_256FS_MUX1, 0},
	/* set mux clk : decoder2_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_256FS_MUX, 0},
	/* set mux clk : viva_dvb2_timing_gen_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_DVB2_TIMING_GEN_256FS, 1},
	/* set mux clk : decoder2_128fs_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_128FS_MUX0, 0},
	/* set mux clk : decoder2_128fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_128FS_MUX1, 0},
	/* set mux clk : decoder2_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER2_128FS_MUX, 0},
	/* set mux clk : viva_dvb3_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_DVB3_SYNTH_REF, 1},
	/* set mux clk : decoder3_synth_hdmi_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_SYNTH_HDMI_CTSN_256FS, 0},
	/* set mux clk : decoder3_synth_hdmi_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_SYNTH_HDMI_CTSN_128FS, 0},
	/* set mux clk : decoder3_synth_hdmi2_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_SYNTH_HDMI2_CTSN_256FS, 0},
	/* set mux clk : decoder3_synth_hdmi2_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_SYNTH_HDMI2_CTSN_128FS, 0},
	/* set mux clk : decoder3_synth_spdif_cdr_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_SYNTH_SPDIF_CDR_256FS, 0},
	/* set mux clk : decoder3_synth_spdif_cdr_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_SYNTH_SPDIF_CDR_128FS, 0},
	/* set mux clk : decoder3_256fs_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_256FS_MUX0, 0},
	/* set mux clk : decoder3_256fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_256FS_MUX1, 0},
	/* set mux clk : decoder3_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_256FS_MUX, 0},
	/* set mux clk : viva_dvb3_timing_gen_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_DVB3_TIMING_GEN_256FS, 1},
	/* set mux clk : decoder3_128fs_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_128FS_MUX0, 0},
	/* set mux clk : decoder3_128fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_128FS_MUX1, 0},
	/* set mux clk : decoder3_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER3_128FS_MUX, 0},
	/* set mux clk : viva_dvb4_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_DVB4_SYNTH_REF, 1},
	/* set mux clk : decoder4_synth_hdmi_ctsn_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_SYNTH_HDMI_CTSN_256FS, 0},
	/* set mux clk : decoder4_synth_hdmi_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_SYNTH_HDMI_CTSN_128FS, 0},
	/* set mux clk : decoder4_synth_hdmi2_ctsn_256fs_div_mux_int_ck from dtsi to 0  */
	{IP2_MUX_DECODER4_SYNTH_HDMI2_CTSN_256FS, 0},
	/* set mux clk : decoder4_synth_hdmi2_ctsn_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_SYNTH_HDMI2_CTSN_128FS, 0},
	/* set mux clk : decoder4_synth_spdif_cdr_256fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_SYNTH_SPDIF_CDR_256FS, 0},
	/* set mux clk : decoder4_synth_spdif_cdr_128fs_div_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_SYNTH_SPDIF_CDR_128FS, 0},
	/* set mux clk : decoder4_256fs_mux0_int_ck from dtsi to 7 */
	{IP2_MUX_DECODER4_256FS_MUX0, 7},
	/* set mux clk : decoder4_256fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_256FS_MUX1, 0},
	/* set mux clk : decoder4_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_256FS_MUX, 0},
	/* set mux clk : viva_dvb4_timing_gen_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_DVB4_TIMING_GEN_256FS, 1},
	/* set mux clk : decoder4_128fs_mux0_int_ck from dtsi to 7 */
	{IP2_MUX_DECODER4_128FS_MUX0, 7},
	/* set mux clk : decoder4_128fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_128FS_MUX1, 0},
	/* set mux clk : decoder4_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_DECODER4_128FS_MUX, 0},
	/* set mux clk : viva_dvb5_timing_gen_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_DVB5_TIMING_GEN_256FS, 1},
	/* set mux clk : viva_dvb6_timing_gen_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_DVB6_TIMING_GEN_256FS, 1},
	/* set mux clk : viva_audma_v2_r1_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_AUDMA_V2_R1_SYNTH_REF, 1},
	/* set mux clk : viva_audma_v2_r1_256fs_mux_int_ck from dtsi to 1 */
	{IP2_MUX_AUDMA_V2_R1_256FS_MUX, 1},
	/* set mux clk : viva_audma_v2_r1_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_AUDMA_V2_R1_256FS, 1},
	/* set mux clk : viva_audma_v2_r2_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_AUDMA_V2_R2_SYNTH_REF, 1},
	/* set mux clk : viva_audma_v2_r2_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_AUDMA_V2_R2_256FS, 1},
	/* set mux clk : viva_spdif_rx_128fs_int_ck from dtsi to 1 */
	{IP2_MUX_SPDIF_RX_128FS, 1},
	/* set mux clk : viva_spdif_rx_dg_int_ck from dtsi to 1 */
	{IP2_MUX_SPDIF_RX_DG, 1},
	/* set mux clk : viva_spdif_rx_synth_ref_int_ck from dtsi to 3 */
	{IP2_MUX_SPDIF_RX_SYNTH_REF, 3},
	/* set mux clk : viva_hdmi_rx_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_HDMI_RX_SYNTH_REF, 1},
	/* set mux clk : hdmi_rx_s2p_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_RX_S2P_256FS_MUX, 0},
	/* set mux clk : viva_hdmi_rx_s2p_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_HDMI_RX_S2P_256FS, 1},
	/* set mux clk : hdmi_rx_s2p_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_RX_S2P_128FS_MUX, 0},
	/* set mux clk : viva_hdmi_rx_s2p_128fs_int_ck from dtsi to 1 */
	{IP2_MUX_HDMI_RX_S2P_128FS, 1},
	/* set mux clk : viva_hdmi_rx_mpll_432_int_ck from dtsi to 1 */
	{IP2_MUX_HDMI_RX_MPLL_432, 1},
	/* set mux clk : viva_bclk_i2s_decoder_int_ck from dtsi to 1 */
	{IP2_MUX_BCLK_I2S_DECODER, 1},
	/* set mux clk : viva_mclk_i2s_decoder_int_ck from dtsi to 5 */
	{IP2_MUX_MCLK_I2S_DECODER, 5},
	/* set mux clk : viva_i2s_bclk_dg_int_ck from dtsi to 1 */
	{IP2_MUX_I2S_BCLK_DG, 1},
	/* set mux clk : viva_i2s_synth_ref_int_ck from dtsi to 3 */
	{IP2_MUX_I2S_SYNTH_REF, 3},
	/* set mux clk : hdmi_tx_to_i2s_tx_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_TX_TO_I2S_TX_256FS_MUX, 0},
	/* set mux clk : hdmi_tx_to_i2s_tx_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_TX_TO_I2S_TX_128FS_MUX, 0},
	/* set mux clk : hdmi_tx_to_i2s_tx_64fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_TX_TO_I2S_TX_64FS_MUX, 0},
	/* set mux clk : hdmi_tx_to_i2s_tx_32fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_TX_TO_I2S_TX_32FS_MUX, 0},
	/* set mux clk : hdmi_rx_to_i2s_tx_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_RX_TO_I2S_TX_256FS_MUX, 0},
	/* set mux clk : hdmi_rx_to_i2s_tx_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_RX_TO_I2S_TX_128FS_MUX, 0},
	/* set mux clk : hdmi_rx_to_i2s_tx_64fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_RX_TO_I2S_TX_64FS_MUX, 0},
	/* set mux clk : hdmi_rx_to_i2s_tx_32fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_RX_TO_I2S_TX_32FS_MUX, 0},
	/* set mux clk : viva_i2s_tx_fifo_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_I2S_TX_FIFO_256FS, 1},
	/* set mux clk : viva_i2s_mclk_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_I2S_MCLK_SYNTH_REF, 1},
	/* set mux clk : viva_mclk_i2s_encoder_int_ck from dtsi to 4 */
	{IP2_MUX_MCLK_I2S_ENCODER, 4},
	/* set mux clk : i2s_bclk_synth_ref_mux_int_ck from dtsi to 0 */
	{IP2_MUX_I2S_BCLK_SYNTH_REF_MUX, 0},
	/* set mux clk : viva_i2s_bclk_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_I2S_BCLK_SYNTH_REF, 1},
	/* set mux clk : bclk_i2s_encoder_128fs_ssc_mux_int_ck from dtsi to 0 */
	{IP2_MUX_BCLK_I2S_ENCODER_128FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder_64fs_ssc_mux_int_ck from dtsi to 0 */
	{IP2_MUX_BCLK_I2S_ENCODER_64FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder_32fs_ssc_mux_int_ck from dtsi to 0 */
	{IP2_MUX_BCLK_I2S_ENCODER_32FS_SSC_MUX, 0},
	/* set mux clk : bclk_i2s_encoder_mux_int_ck from dtsi to 4 */
	{IP2_MUX_BCLK_I2S_ENCODER_MUX, 4},
	/* set mux clk : bclk_i2s_encoder_int_ck from dtsi to 1 */
	{IP2_MUX_BCLK_I2S_ENCODER, 1},
	/* set mux clk : viva_i2s_trx_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_I2S_TRX_SYNTH_REF, 1},
	/* set mux clk : viva_i2s_trx_bclk_timing_gen_int_ck from dtsi to 1 */
	{IP2_MUX_I2S_TRX_BCLK_TIMING_GEN, 1},
	/* set mux clk : viva_pcm_clk_216_int_ck from dtsi to 1 */
	{IP2_MUX_PCM_CLK_216, 1},
	/* set mux clk : viva_pcm_clk_aupll_int_ck from dtsi to 1 */
	{IP2_MUX_PCM_CLK_AUPLL, 1},
	/* set mux clk : spdif_tx_gpa_synth_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_TX_GPA_SYNTH_256FS_MUX, 0},
	/* set mux clk : spdif_tx_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_TX_256FS_MUX, 0},
	/* set mux clk : viva_spdif_tx_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_SPDIF_TX_256FS, 1},
	/* set mux clk : spdif_tx_gpa_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_TX_GPA_128FS_MUX, 0},
	/* set mux clk : spdif_tx_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_TX_128FS_MUX, 0},
	/* set mux clk : viva_spdif_tx_128fs_int_ck from dtsi to 1 */
	{IP2_MUX_SPDIF_TX_128FS, 1},
	/* set mux clk : viva_spdif_tx_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_SPDIF_TX_SYNTH_REF, 1},
	/* set mux clk : spdif_tx2_gpa_synth_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_TX2_GPA_SYNTH_256FS_MUX, 0},
	/* set mux clk : spdif_tx2_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_TX2_256FS_MUX, 0},
	/* set mux clk : viva_spdif_tx2_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_SPDIF_TX2_256FS, 1},
	/* set mux clk : spdif_tx2_gpa_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_TX2_GPA_128FS_MUX, 0},
	/* set mux clk : spdif_tx2_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_TX2_128FS_MUX, 0},
	/* set mux clk : viva_spdif_tx2_128fs_int_ck from dtsi to 1 */
	{IP2_MUX_SPDIF_TX2_128FS, 1},
	/* set mux clk : viva_npcm_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_NPCM_SYNTH_REF, 1},
	/* set mux clk : viva_npcm2_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_NPCM2_SYNTH_REF, 1},
	/* set mux clk : viva_npcm3_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_NPCM3_SYNTH_REF, 1},
	/* set mux clk : viva_npcm4_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_NPCM4_SYNTH_REF, 1},
	/* set mux clk : viva_npcm4_256fs_timing_gen_int_ck from dtsi to 1 */
	{IP2_MUX_NPCM4_256FS_TIMING_GEN, 1},
	/* set mux clk : viva_npcm5_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_NPCM5_SYNTH_REF, 1},
	/* set mux clk : viva_npcm5_256fs_timing_gen_int_ck from dtsi to 1 */
	{IP2_MUX_NPCM5_256FS_TIMING_GEN, 1},
	/* set mux clk : hdmi_tx_256fs_mux1_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_TX_256FS_MUX1, 0},
	/* set mux clk : hdmi_tx_256fs_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_TX_256FS_MUX0, 0},
	/* set mux clk : hdmi_tx_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_HDMI_TX_256FS_MUX, 0},
	/* set mux clk : viva_hdmi_tx_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_HDMI_TX_256FS, 1},
	/* set mux clk : viva_hdmi_tx_128fs_int_ck from dtsi to 1 */
	{IP2_MUX_HDMI_TX_128FS, 1},
	/* set mux clk : spdif_extearc_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_EXTEARC_256FS_MUX, 0},
	/* set mux clk : spdif_extearc_128fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_EXTEARC_128FS_MUX, 0},
	/* set mux clk : spdif_extearc_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_EXTEARC_MUX0, 0},
	/* set mux clk : spdif_extearc_mux_int_ck from dtsi to 0 */
	{IP2_MUX_SPDIF_EXTEARC_MUX, 0},
	/* set mux clk : viva_spdif_extearc_int_ck from dtsi to 1 */
	{IP2_MUX_SPDIF_EXTEARC, 1},
	/* set mux clk : viva_earc_tx_synth_ref_int_ck from dtsi to 2 */
	{IP2_MUX_EARC_TX_SYNTH_REF, 2},
	/* set mux clk : viva_earc_tx_gpa_synth_ref_int_ck from dtsi to 1 */
	{IP2_MUX_EARC_TX_GPA_SYNTH_REF, 1},
	/* set mux clk : viva_earc_tx_synth_iir_ref_int_ck from dtsi to 1 */
	{IP2_MUX_EARC_TX_SYNTH_IIR_REF, 1},
	/* set mux clk : earc_tx_gpa_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_EARC_TX_GPA_256FS_MUX, 0},
	/* set mux clk : earc_tx_pll_256fs_ref_mux_int_ck from dtsi to 0 */
	{IP2_MUX_EARC_TX_PLL_256FS_REF_MUX, 0},
	/* set mux clk : earc_tx_pll_256fs_ref_int_ck from dtsi to 1 */
	{IP2_MUX_EARC_TX_PLL_256FS_REF, 1},
	/* set mux clk : viva_earc_tx_pll_int_ck from dtsi to 1 */
	{IP2_MUX_EARC_TX_PLL, 1},
	/* set mux clk : viva_earc_tx_64xchxfs_int_ck from dtsi to 1 */
	{IP2_MUX_EARC_TX_64XCHXFS, 1},
	/* set mux clk : viva_earc_tx_32xchxfs_int_ck from dtsi to 1 */
	{IP2_MUX_EARC_TX_32XCHXFS, 1},
	/* set mux clk : viva_earc_tx_32xfs_int_ck from dtsi to 1 */
	{IP2_MUX_EARC_TX_32XFS, 1},
	/* set mux clk : earc_tx_source_256fs_mux_int_ck from dtsi to 0 */
	{IP2_MUX_EARC_TX_SOURCE_256FS_MUX, 0},
	/* set mux clk : viva_earc_tx_source_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_EARC_TX_SOURCE_256FS, 1},
	/* set mux clk : viva_sin_gen_int_ck from dtsi to 1 */
	{IP2_MUX_SIN_GEN, 1},
	/* set mux clk : viva_test_bus_int_ck from dtsi to 1 */
	{IP2_MUX_TEST_BUS, 1},
	/* set mux clk : viva_src_mac_bspline_int_ck from dtsi to 1 */
	{IP2_MUX_SRC_MAC_BSPLINE, 1},
	/* set mux clk : synth_pcm_trx_256fs_int_ck from dtsi to 1 */
	{IP2_MUX_SYNTH_PCM_TRX_256FS, 1},

	{END_OF_TABLE, 0x00}, // end of table
};

static const struct clk_type src1_clk_table[] = {
	/* set mux clk : ch_m1_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M1_256FSI_MUX0, 0},
	/* set mux clk : ch_m1_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M1_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m1_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M1_256FSI, 1},

	/* set mux clk : ch_m2_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M2_256FSI_MUX0, 0},
	/* set mux clk : ch_m2_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M2_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m2_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M2_256FSI, 1},

	/* set mux clk : ch_m3_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M3_256FSI_MUX0, 0},
	/* set mux clk : ch_m3_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M3_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m3_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M3_256FSI, 1},

	/* set mux clk : ch_m4_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M4_256FSI_MUX0, 0},
	/* set mux clk : ch_m4_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M4_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m4_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M4_256FSI, 1},

	/* set mux clk : ch_m5_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M5_256FSI_MUX0, 0},
	/* set mux clk : ch_m5_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M5_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m5_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M5_256FSI, 1},

	/* set mux clk : ch_m6_256fsi_mux0_int_ck from dtsi to 0 */
	{MUX_CH_M6_256FSI_MUX0, 0},
	/* set mux clk : ch_m6_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M6_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m6_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M6_256FSI, 1},

	{END_OF_TABLE, 0x00}, // end of table
};

static const struct clk_type src2_clk_table[] = {
	/* set mux clk : ch5_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH5_256FSI_MUX1, 4},
	/* set mux clk : ch5_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH5_256FSI_MUX, 1},
	/* set mux clk : viva_ch5_256fsi_int_ck from dtsi to 1 */
	{MUX_CH5_256FSI, 1},

	/* set mux clk : ch6_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH6_256FSI_MUX1, 4},
	/* set mux clk : ch6_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH6_256FSI_MUX, 1},
	/* set mux clk : viva_ch6_256fsi_int_ck from dtsi to 1 */
	{MUX_CH6_256FSI, 1},

	/* set mux clk : ch7_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH7_256FSI_MUX1, 4},
	/* set mux clk : ch7_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH7_256FSI_MUX, 1},
	/* set mux clk : viva_ch7_256fsi_int_ck from dtsi to 1 */
	{MUX_CH7_256FSI, 1},

	/* set mux clk : ch8_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH8_256FSI_MUX1, 4},
	/* set mux clk : ch8_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH8_256FSI_MUX, 1},
	/* set mux clk : viva_ch8_256fsi_int_ck from dtsi to 1 */
	{MUX_CH8_256FSI, 1},

	/* set mux clk : ch9_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH9_256FSI_MUX1, 4},
	/* set mux clk : ch9_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH9_256FSI_MUX, 1},
	/* set mux clk : viva_ch9_256fsi_int_ck from dtsi to 1 */
	{MUX_CH9_256FSI, 1},

	/* set mux clk : ch10_256fsi_mux1_int_ck from dtsi to 4 */
	{MUX_CH10_256FSI_MUX1, 4},
	/* set mux clk : ch10_256fsi_mux_int_ck from dtsi to 1 */
	{MUX_CH10_256FSI_MUX, 1},
	/* set mux clk : viva_ch10_256fsi_int_ck from dtsi to 1 */
	{MUX_CH10_256FSI, 1},

	{END_OF_TABLE, 0x00}, // end of table
};

static const struct clk_type src3_clk_table[] = {
	/* set mux clk : ch_m9_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M9_256FSI_MUX0, 2},
	/* set mux clk : ch_m9_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M9_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m9_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M9_256FSI, 1},

	/* set mux clk : ch_m10_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M10_256FSI_MUX0, 2},
	/* set mux clk : ch_m10_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M10_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m10_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M10_256FSI, 1},

	/* set mux clk : ch_m11_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M11_256FSI_MUX0, 2},
	/* set mux clk : ch_m11_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M11_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m11_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M11_256FSI, 1},

	/* set mux clk : ch_m12_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M12_256FSI_MUX0, 2},
	/* set mux clk : ch_m12_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M12_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m12_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M12_256FSI, 1},

	/* set mux clk : ch_m13_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M13_256FSI_MUX0, 2},
	/* set mux clk : ch_m13_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M13_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m13_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M13_256FSI, 1},

	/* set mux clk : ch_m14_256fsi_mux0_int_ck from dtsi to 2 */
	{MUX_CH_M14_256FSI_MUX0, 2},
	/* set mux clk : ch_m14_256fsi_mux_int_ck from dtsi to 0 */
	{MUX_CH_M14_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m14_256fsi_int_ck from dtsi to 1 */
	{MUX_CH_M14_256FSI, 1},

	{END_OF_TABLE, 0x00}, // end of table
};

// TODO: channel mapping refine? DSP capture side refine?
static const struct clk_type ip2_src1_clk_table[] = {
	/* set mux clk : ch_m1_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M1_256FSI_MUX0, 0},
	/* set mux clk : ch_m1_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M1_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m1_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M1_256FSI, 1},

	/* set mux clk : ch_m2_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M2_256FSI_MUX0, 0},
	/* set mux clk : ch_m2_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M2_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m2_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M2_256FSI, 1},

	/* set mux clk : ch_m3_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M3_256FSI_MUX0, 0},
	/* set mux clk : ch_m3_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M3_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m3_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M3_256FSI, 1},

	/* set mux clk : ch_m4_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M4_256FSI_MUX0, 0},
	/* set mux clk : ch_m4_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M4_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m4_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M4_256FSI, 1},

	/* set mux clk : ch_m5_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M5_256FSI_MUX0, 0},
	/* set mux clk : ch_m5_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH_M5_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m5_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M5_256FSI, 1},

	/* set mux clk : ch_m6_256fsi_mux0_int_ck from dtsi to 0 */
	{IP2_MUX_CH6_256FSI_MUX1, 0},
	/* set mux clk : ch_m6_256fsi_mux_int_ck from dtsi to 0 */
	{IP2_MUX_CH6_256FSI_MUX, 0},
	/* set mux clk : viva_ch_m6_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH6_256FSI, 1},

	{END_OF_TABLE, 0x00}, // end of table
};

// TODO: channel mapping refine? DSP capture side refine?
static const struct clk_type ip2_src2_clk_table[] = {
	/* set mux clk : ch_m1_256fsi_mux0_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M1_256FSI_MUX0, 4},
	/* set mux clk : ch_m1_256fsi_mux_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M1_256FSI_MUX, 4},
	/* set mux clk : viva_ch_m1_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M1_256FSI, 1},

	/* set mux clk : ch_m2_256fsi_mux0_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M2_256FSI_MUX0, 4},
	/* set mux clk : ch_m2_256fsi_mux_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M2_256FSI_MUX, 4},
	/* set mux clk : viva_ch_m2_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M2_256FSI, 1},

	/* set mux clk : ch_m3_256fsi_mux0_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M3_256FSI_MUX0, 4},
	/* set mux clk : ch_m3_256fsi_mux_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M3_256FSI_MUX, 4},
	/* set mux clk : viva_ch_m3_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M3_256FSI, 1},

	/* set mux clk : ch_m4_256fsi_mux0_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M4_256FSI_MUX0, 4},
	/* set mux clk : ch_m4_256fsi_mux_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M4_256FSI_MUX, 4},
	/* set mux clk : viva_ch_m4_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M4_256FSI, 1},

	/* set mux clk : ch_m5_256fsi_mux0_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M5_256FSI_MUX0, 4},
	/* set mux clk : ch_m5_256fsi_mux_int_ck from dtsi to 4 */
	{IP2_MUX_CH_M5_256FSI_MUX, 4},
	/* set mux clk : viva_ch_m5_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH_M5_256FSI, 1},

	/* set mux clk : ch_m7_256fsi_mux0_int_ck from dtsi to 4 */
	{IP2_MUX_CH7_256FSI_MUX1, 4},
	/* set mux clk : ch_m7_256fsi_mux_int_ck from dtsi to 4 */
	{IP2_MUX_CH7_256FSI_MUX, 4},
	/* set mux clk : viva_ch_m7_256fsi_int_ck from dtsi to 1 */
	{IP2_MUX_CH7_256FSI, 1},

	{END_OF_TABLE, 0x00}, // end of table
};

struct mtk_soc_priv {
	struct device	*dev;
	unsigned int	riu_base;
	unsigned int	bank_num;
	unsigned int	*reg_bank;
	void __iomem	**reg_base;
	const char	*pre_init_table;
	const char	*init_table;
	const char	*depop_table;
	const char	*earc_table;
	struct clk	**gate_clk;
	struct clk	**mux_clk;
	struct clk	**clk;
	unsigned int	log_level;
	struct snd_soc_component *component;
	bool		suspended;
	int		ip_version;
	int		atv_input_type;
};

static struct mtk_soc_priv *priv;

//-------------------------------------------------------------------
/// Suspend the calling task for u32Ms milliseconds
/// @param  u32Ms  \b IN: delay 1 ~ MSOS_WAIT_FOREVER ms
/// @return None
/// @note   Not allowed in interrupt context; otherwise,
///			exception will occur.
//-------------------------------------------------------------------
void mtk_alsa_delaytask(unsigned int u32Ms)
{
	mtk_alsa_delaytask_us(u32Ms * 1000UL);
}
EXPORT_SYMBOL_GPL(mtk_alsa_delaytask);

//-------------------------------------------------------------------
/// Delay for u32Us microseconds
/// @param  u32Us  \b IN: delay 0 ~ 999 us
/// @return None
/// @note   implemented by "busy waiting".
///			Plz call mtk_alsa_delaytask directly for ms-order delay
//-------------------------------------------------------------------
void mtk_alsa_delaytask_us(unsigned int u32Us)
{
	/*
	 * Refer to Documentation/timers/timers-howto.txt
	 * Non-atomic context
	 *		< 10us	:	udelay()
	 *	10us ~ 20ms	:	usleep_range()
	 *		> 10ms	:	msleep() or msleep_interruptible()
	 */

	if (u32Us < 10UL)
		udelay(u32Us);
	else if (u32Us < 20UL * 1000UL)
		usleep_range(u32Us, u32Us + 1);
	else
		msleep_interruptible((unsigned int)(u32Us / 1000UL));
}
EXPORT_SYMBOL_GPL(mtk_alsa_delaytask_us);

void __iomem *mtk_alsa_get_bank_base(unsigned int base)
{
	void __iomem *reg = NULL;
	int i;

	for (i = 0; i < priv->bank_num; i++) {
		if (base == priv->reg_bank[i]) {
			reg = priv->reg_base[i];
			break;
		}
	}

	if (reg == NULL)
		WARN_ON("[ALSA MTK]Invalid VIVALDI bank");

	return reg;
}

bool mtk_alsa_read_reg_polling(unsigned int addr,
		unsigned short mask, unsigned short val, int counter)
{
	int i = 0;
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	base = addr & 0xFFFFFF00;
	offset = addr & 0xFF;
	offset = offset << 1;

	reg = mtk_alsa_get_bank_base(base);

	while (i < counter) {
		if ((readw(reg + offset) & mask) == val)
			return 0;
		i++;
	}
	return 1;
}
EXPORT_SYMBOL_GPL(mtk_alsa_read_reg_polling);

bool mtk_alsa_read_reg_byte_polling(unsigned int addr,
		unsigned char mask, unsigned char val, int counter)
{
	int i = 0;
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	base = addr & 0xFFFFFF00;
	offset = addr & 0xFF;
	offset = (offset << 1) - (offset & 1);

	reg = mtk_alsa_get_bank_base(base);

	while (i < counter) {
		if ((readb(reg + offset) & mask) == val)
			return 0;
		i++;
	}
	return 1;
}
EXPORT_SYMBOL_GPL(mtk_alsa_read_reg_byte_polling);

unsigned char mtk_alsa_read_reg_byte(unsigned int addr)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	base = addr & 0xFFFFFF00;
	offset = addr & 0xFF;
	offset = (offset << 1) - (offset & 1);

	reg = mtk_alsa_get_bank_base(base);

	return readb(reg + offset);
}
EXPORT_SYMBOL_GPL(mtk_alsa_read_reg_byte);

unsigned short mtk_alsa_read_reg(unsigned int addr)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	base = addr & 0xFFFFFF00;
	offset = addr & 0xFF;
	offset = offset << 1;

	reg = mtk_alsa_get_bank_base(base);

	return readw(reg + offset);
}
EXPORT_SYMBOL_GPL(mtk_alsa_read_reg);

void mtk_alsa_write_reg_mask_byte(unsigned int addr,
			unsigned char mask, unsigned char val)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	if (addr == 0) {
		pr_err("[ALSA MTK] %s[%d] error! try to write addr 0x0\n", __func__, __LINE__);
		return;
	}

	base = addr & 0xFFFFFF00;
	offset = addr & 0xFF;
	offset = (offset << 1) - (offset & 1);

	reg = mtk_alsa_get_bank_base(base);

	ALSA_WRITE_MASK_BYTE(reg + offset, mask, val);
}
EXPORT_SYMBOL_GPL(mtk_alsa_write_reg_mask_byte);

void mtk_alsa_write_reg_byte(unsigned int addr, unsigned char val)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	if (addr == 0) {
		pr_err("[ALSA MTK] %s[%d] error! try to write addr 0x0\n", __func__, __LINE__);
		return;
	}


	base = addr & 0xFFFFFF00;
	offset = addr & 0xFF;
	offset = (offset << 1) - (offset & 1);

	reg = mtk_alsa_get_bank_base(base);

	writeb(val, reg + offset);
}
EXPORT_SYMBOL_GPL(mtk_alsa_write_reg_byte);

void mtk_alsa_write_reg_mask(unsigned int addr,
			unsigned short mask, unsigned short val)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	if (addr == 0) {
		pr_err("[ALSA MTK] %s[%d] error! try to write addr 0x0\n", __func__, __LINE__);
		return;
	}

	base = addr & 0xFFFFFF00;
	offset = addr & 0xFF;
	offset = offset << 1;

	reg = mtk_alsa_get_bank_base(base);

	ALSA_WRITE_MASK_2BYTE(reg + offset, mask, val);
}
EXPORT_SYMBOL_GPL(mtk_alsa_write_reg_mask);

void mtk_alsa_write_reg(unsigned int addr, unsigned short val)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	if (addr == 0) {
		pr_err("[ALSA MTK] %s[%d] error! try to write addr 0x0\n", __func__, __LINE__);
		return;
	}

	base = addr & 0xFFFFFF00;
	offset = addr & 0xFF;
	offset = offset << 1;

	reg = mtk_alsa_get_bank_base(base);

	writew(val, reg + offset);
}
EXPORT_SYMBOL_GPL(mtk_alsa_write_reg);

int mtk_alsa_gate_clock_init(struct platform_device *pdev)
{
	int i;

	priv->gate_clk = devm_kcalloc(&pdev->dev, GATE_CLK_NUM,
					sizeof(*priv->gate_clk), GFP_KERNEL);
	if (!priv->gate_clk)
		return -ENOMEM;

	for (i = 0; i < GATE_CLK_NUM; i++) {
		priv->gate_clk[i] = devm_clk_get(&pdev->dev, gate_clk[i]);
		if (IS_ERR(priv->gate_clk[i])) {
			dev_err(&pdev->dev, "[ALSA MTK]get gate clk %s fail\n",
				gate_clk[i]);
			return -ENODEV;
		}
	}

	return 0;
}

int mtk_alsa_ip2_gate_clock_init(struct platform_device *pdev)
{
	int i;

	priv->gate_clk = devm_kcalloc(&pdev->dev, IP2_GATE_CLK_NUM,
					sizeof(*priv->gate_clk), GFP_KERNEL);
	if (!priv->gate_clk)
		return -ENOMEM;

	for (i = 0; i < IP2_GATE_CLK_NUM; i++) {
		priv->gate_clk[i] = devm_clk_get(&pdev->dev, ip2_gate_clk[i]);
		if (IS_ERR(priv->gate_clk[i])) {
			dev_err(&pdev->dev, "[ALSA MTK]get gate clk %s fail\n",
				ip2_gate_clk[i]);
			return -ENODEV;
		}
	}

	return 0;
}

int mtk_alsa_gate_clock_enable(int clk_name)
{
	int ret;
	const char **gate_clock;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100))
		gate_clock = gate_clk;
	else
		gate_clock = ip2_gate_clk;

	ret = clk_prepare_enable(priv->gate_clk[clk_name]);
	if (ret) {
		dev_err(priv->dev, "[ALSA MTK]set gate clk %s fail\n", gate_clock[clk_name]);
		return ret;
	}

	return 0;
}

void mtk_alsa_gate_clock_disable(int clk_name)
{
	clk_disable_unprepare(priv->gate_clk[clk_name]);
}

int mtk_alsa_mux_clock_init(struct platform_device *pdev)
{
	int i;

	priv->mux_clk = devm_kcalloc(&pdev->dev, MUX_CLK_NUM,
				sizeof(*priv->mux_clk),	GFP_KERNEL);
	if (!priv->mux_clk)
		return -ENOMEM;

	for (i = 0; i < MUX_CLK_NUM; i++) {
		priv->mux_clk[i] = devm_clk_get(&pdev->dev, mux_clk[i]);
		if (IS_ERR(priv->mux_clk[i])) {
			dev_err(&pdev->dev, "[ALSA MTK]get mux clk %s fail\n",
				mux_clk[i]);
			return -ENODEV;
		}
	}

	return 0;
}

int mtk_alsa_ip2_mux_clock_init(struct platform_device *pdev)
{
	int i;

	priv->mux_clk = devm_kcalloc(&pdev->dev, IP2_MUX_CLK_NUM,
				sizeof(*priv->mux_clk),	GFP_KERNEL);
	if (!priv->mux_clk)
		return -ENOMEM;

	for (i = 0; i < IP2_MUX_CLK_NUM; i++) {
		priv->mux_clk[i] = devm_clk_get(&pdev->dev, ip2_mux_clk[i]);
		if (IS_ERR(priv->mux_clk[i])) {
			dev_err(&pdev->dev, "[ALSA MTK]get mux clk %s fail\n",
				ip2_mux_clk[i]);
			return -ENODEV;
		}
	}

	return 0;
}

int mtk_alsa_mux_clock_setting(int clk_name, int value)
{
	struct clk_hw *target_parent;
	int ret;
	const char **mux_clock;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100))
		mux_clock = mux_clk;
	else
		mux_clock = ip2_mux_clk;

	if (priv->mux_clk[clk_name]) {
		/* get clk parent */
		target_parent = clk_hw_get_parent_by_index(
				__clk_get_hw(priv->mux_clk[clk_name]), value);
		if (IS_ERR_OR_NULL(target_parent)) {
			dev_err(priv->dev, "[ALSA MTK]get mux target clk %s parent fail\n",
						mux_clock[clk_name]);
			return PTR_ERR(target_parent);
		}
		/* set current clk parent to target_parent : switch mux */
		ret = clk_set_parent(priv->mux_clk[clk_name], target_parent->clk);
		if (ret) {
			dev_err(priv->dev, "[ALSA MTK]set mux clk %s parent fail\n",
						mux_clock[clk_name]);
			return ret;
		}
	} else {
		dev_err(priv->dev, "[ALSA MTK] mux clk %s not available\n",
						mux_clock[clk_name]);
		return -ENODEV;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_alsa_mux_clock_setting);

int mtk_alsa_mux_clock_setting_by_name(const char *clk_name, int value)
{
	struct clk_hw *target_parent;
	int ret;

	/* get clk parent */
	target_parent = clk_hw_get_parent_by_index(
			__clk_get_hw(devm_clk_get(priv->dev, clk_name)), value);
	if (IS_ERR_OR_NULL(target_parent)) {
		dev_err(priv->dev, "[ALSA MTK]get mux target clk %s parent fail\n", clk_name);
		return PTR_ERR(target_parent);
	}
	/* set current clk parent to target_parent : switch mux */
	ret = clk_set_parent(devm_clk_get(priv->dev, clk_name), target_parent->clk);
	if (ret) {
		dev_err(priv->dev, "[ALSA MTK]set mux clk %s parent fail\n", clk_name);
		return ret;
	}

	return 0;
}

int mtk_alsa_mux_clock_enable(int clk_name)
{
	int ret;
	const char **mux_clock;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100))
		mux_clock = mux_clk;
	else
		mux_clock = ip2_mux_clk;


	ret = clk_prepare_enable(priv->mux_clk[clk_name]);
	if (ret) {
		dev_err(priv->dev, "[ALSA MTK]set mux clk %s fail\n", mux_clock[clk_name]);
		return ret;
	}

	return 0;
}

int mtk_alsa_mux_clock_enable_by_name(const char *clk_name)
{
	int ret;

	ret = clk_prepare_enable(devm_clk_get(priv->dev, clk_name));
	if (ret) {
		dev_err(priv->dev, "[ALSA MTK]set mux clk %s fail\n", clk_name);
		return ret;
	}

	return 0;
}

void mtk_alsa_mux_clock_disable(int clk_name)
{
	clk_disable_unprepare(priv->mux_clk[clk_name]);
}

void mtk_alsa_mux_clock_disable_by_name(const char *clk_name)
{
	clk_disable_unprepare(devm_clk_get(priv->dev, clk_name));
}

int mtk_alsa_sw_clock_init(struct platform_device *pdev)
{
	int i;

	priv->clk = devm_kcalloc(&pdev->dev, CLK_NUM, sizeof(*priv->clk),
				GFP_KERNEL);
	if (!priv->clk)
		return -ENOMEM;

	for (i = 0; i < CLK_NUM; i++) {
		priv->clk[i] = devm_clk_get(&pdev->dev, sw_clk[i]);
		if (IS_ERR(priv->clk[i])) {
			dev_err(&pdev->dev, "[ALSA MTK]get sw clk %s fail\n", sw_clk[i]);
			return -ENODEV;
		}
	}

	return 0;
}

int mtk_alsa_ip2_sw_clock_init(struct platform_device *pdev)
{
	int i;

	priv->clk = devm_kcalloc(&pdev->dev, IP2_CLK_NUM, sizeof(*priv->clk), GFP_KERNEL);

	if (!priv->clk)
		return -ENOMEM;

	for (i = 0; i < IP2_CLK_NUM; i++) {
		priv->clk[i] = devm_clk_get(&pdev->dev, ip2_sw_clk[i]);
		if (IS_ERR(priv->clk[i])) {
			dev_err(&pdev->dev, "[ALSA MTK]get sw clk %s fail\n", ip2_sw_clk[i]);
			return -ENODEV;
		}
	}

	return 0;
}

int mtk_alsa_sw_clock_enable(int clk_name)
{
	int ret;
	const char **sw_clock;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100))
		sw_clock = sw_clk;
	else
		sw_clock = ip2_sw_clk;

	ret = clk_prepare_enable(priv->clk[clk_name]);
	if (ret) {
		dev_err(priv->dev, "[ALSA MTK]set sw clk %s fail\n", sw_clock[clk_name]);
		return ret;
	}

	return 0;
}

int mtk_alsa_sw_clock_enable_by_name(const char *clk_name)
{
	int ret;

	ret = clk_prepare_enable(devm_clk_get(priv->dev, clk_name));
	if (ret) {
		dev_err(priv->dev, "[ALSA MTK]set sw clk %s fail\n", clk_name);
		return ret;
	}

	return 0;
}

void mtk_alsa_sw_clock_disable(int clk_name)
{
	if (__clk_is_enabled(priv->clk[clk_name]))
		clk_disable_unprepare(priv->clk[clk_name]);
}

void mtk_alsa_sw_clock_disable_by_name(const char *clk_name)
{
	if (__clk_is_enabled(devm_clk_get(priv->dev, clk_name)))
		clk_disable_unprepare(devm_clk_get(priv->dev, clk_name));
}

int mtk_i2s_tx_clk_open(void)
{
	int ret;

	/* set mux clk : viva_i2s_tx_fifo_256fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_i2s_tx_fifo_256fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_i2s_tx_fifo_256fs_int_ck");
	if (ret)
		return ret;
	/* set mux clk : hdmi_rx_to_i2s_tx_256fs_mux_int_ck from dtsi to 0 */
	ret = mtk_alsa_mux_clock_setting_by_name("hdmi_rx_to_i2s_tx_256fs_mux_int_ck", 0);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("hdmi_rx_to_i2s_tx_256fs_mux_int_ck");
	if (ret)
		return ret;
	/* set mux clk : hdmi_tx_to_i2s_tx_256fs_mux_int_ck from dtsi to 0 */
	ret = mtk_alsa_mux_clock_setting_by_name("hdmi_tx_to_i2s_tx_256fs_mux_int_ck", 0);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("hdmi_tx_to_i2s_tx_256fs_mux_int_ck");
	if (ret)
		return ret;
	/* set mux clk : viva_i2s_mclk_synth_ref_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_i2s_mclk_synth_ref_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_i2s_mclk_synth_ref_int_ck");
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_i2s_tx_clk_open);

int mtk_i2s_tx_clk_close(void)
{
	mtk_alsa_mux_clock_disable_by_name("viva_i2s_tx_fifo_256fs_int_ck");
	mtk_alsa_mux_clock_disable_by_name("hdmi_rx_to_i2s_tx_256fs_mux_int_ck");
	mtk_alsa_mux_clock_disable_by_name("hdmi_tx_to_i2s_tx_256fs_mux_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_i2s_mclk_synth_ref_int_ck");

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_i2s_tx_clk_close);

int mtk_i2s_tx_clk_set_enable(snd_pcm_format_t format, unsigned int rate)
{
	int ret;

	if (format == SNDRV_PCM_FORMAT_S32_LE) {
		/* set mux clk : bclk_i2s_encoder_mux_int_ck from dtsi to 4 */
		ret = mtk_alsa_mux_clock_setting_by_name("bclk_i2s_encoder_mux_int_ck", 4);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("bclk_i2s_encoder_mux_int_ck");
		if (ret)
			return ret;
	} else if (format == SNDRV_PCM_FORMAT_S24_LE) {
		/* set mux clk : bclk_i2s_encoder_mux_int_ck from dtsi to 6 */
		ret = mtk_alsa_mux_clock_setting_by_name("bclk_i2s_encoder_mux_int_ck", 6);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("bclk_i2s_encoder_mux_int_ck");
		if (ret)
			return ret;
	} else {  // SNDRV_PCM_FORMAT_S16_LE
		/* set mux clk : bclk_i2s_encoder_mux_int_ck from dtsi to 3 */
		ret = mtk_alsa_mux_clock_setting_by_name("bclk_i2s_encoder_mux_int_ck", 3);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("bclk_i2s_encoder_mux_int_ck");
		if (ret)
			return ret;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_i2s_tx_clk_set_enable);

int mtk_i2s_tx_clk_set_disable(void)
{
	mtk_alsa_mux_clock_disable_by_name("bclk_i2s_encoder_int_ck");
	mtk_alsa_mux_clock_disable_by_name("bclk_i2s_encoder_mux_int_ck");
	mtk_alsa_mux_clock_disable_by_name("bclk_i2s_encoder_64fs_ssc_mux_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_mclk_i2s_encoder_int_ck");

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_i2s_tx_clk_set_disable);

int mtk_spdif_rx_sw_clk_set_enable(void)
{
	int ret = 0;

	ret |= mtk_alsa_sw_clock_enable_by_name("viva_spdif_rx_synth_ref2spdif_rx_synth_ref");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_spdif_rx_dg2spdif_rx_dg");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_spdif_rx_128fs2spdif_rx_128fs");

	return ret;
}

void mtk_spdif_rx_sw_clk_set_disable(void)
{
	mtk_alsa_sw_clock_disable_by_name("viva_spdif_rx_synth_ref2spdif_rx_synth_ref");
	mtk_alsa_sw_clock_disable_by_name("viva_spdif_rx_dg2spdif_rx_dg");
	mtk_alsa_sw_clock_disable_by_name("viva_spdif_rx_128fs2spdif_rx_128fs");
}

int mtk_spdif_tx_sw_clk_set_enable(void)
{
	int ret = 0;

	ret |= mtk_alsa_sw_clock_enable_by_name("viva_spdif_tx_128fs2spdif_tx_128fs");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_spdif_tx_256fs2spdif_tx_256fs");

	return ret;
}

void mtk_spdif_tx_sw_clk_set_disable(void)
{
	mtk_alsa_sw_clock_disable_by_name("viva_spdif_tx_256fs2spdif_tx_256fs");
	mtk_alsa_sw_clock_disable_by_name("viva_spdif_tx_128fs2spdif_tx_128fs");
}

int mtk_spdif_tx_clk_set_enable(int format)
{
	int ret;

	/* set mux clk : viva_spdif_tx_128fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_spdif_tx_128fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_spdif_tx_128fs_int_ck");
	if (ret)
		return ret;

	/* set mux clk : viva_spdif_tx_256fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_spdif_tx_256fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_spdif_tx_256fs_int_ck");
	if (ret)
		return ret;

	if (format == 0) {
		/* set mux clk : spdif_tx_128fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx_128fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx_128fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx_gpa_128fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx_gpa_128fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx_gpa_128fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx_256fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx_256fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx_256fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx_gpa_synth_256fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx_gpa_synth_256fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx_gpa_synth_256fs_mux_int_ck");
		if (ret)
			return ret;
	} else if (format == 1) {
		/* set mux clk : spdif_tx_128fs_mux_int_ck from dtsi to 2 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx_128fs_mux_int_ck", 2);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx_128fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx_256fs_mux_int_ck from dtsi to 2 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx_256fs_mux_int_ck", 2);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx_256fs_mux_int_ck");
		if (ret)
			return ret;
	}
	/* enable spdif tx sw clock */
	ret = mtk_spdif_tx_sw_clk_set_enable();
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_spdif_tx_clk_set_enable);

int mtk_spdif_tx_clk_set_disable(int format)
{
	if (format == 0) {
		mtk_alsa_mux_clock_disable_by_name("viva_spdif_tx_128fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx_128fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx_gpa_128fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("viva_spdif_tx_256fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx_256fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx_gpa_synth_256fs_mux_int_ck");
	}

	else if (format == 1) {
		mtk_alsa_mux_clock_disable_by_name("viva_spdif_tx_128fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx_128fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("viva_spdif_tx_256fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx_256fs_mux_int_ck");
	}

	/* disable spdif tx sw clock */
	mtk_spdif_tx_sw_clk_set_disable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_spdif_tx_clk_set_disable);

int mtk_arc_sw_clk_set_enable(void)
{
	int ret = 0;

	/* EARC */
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_earc_tx_synth_ref2earc_tx_synth_ref");
	ret |= mtk_alsa_sw_clock_enable_by_name(
					"viva_earc_tx_gpa_synth_ref2earc_tx_gpa_synth_ref");
	ret |= mtk_alsa_sw_clock_enable_by_name(
					"viva_earc_tx_synth_iir_ref2earc_tx_synth_iir_ref");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_earc_tx_pll2earc_tx_pll");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_earc_tx_64xchxfs2earc_tx_64xchxfs");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_earc_tx_32xchxfs2earc_tx_32xchxfs");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_earc_tx_32xfs2earc_tx_32xfs");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_earc_tx_source_256fs2earc_tx_source_256fs");
	/* ARC */
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_spdif_tx2_128fs2spdif_tx2_128fs");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_spdif_tx2_256fs2spdif_tx2_256fs");

	return ret;
}

void mtk_arc_sw_clk_set_disable(void)
{
	/* EARC */
	mtk_alsa_sw_clock_disable_by_name("viva_earc_tx_source_256fs2earc_tx_source_256fs");
	mtk_alsa_sw_clock_disable_by_name("viva_earc_tx_32xfs2earc_tx_32xfs");
	mtk_alsa_sw_clock_disable_by_name("viva_earc_tx_32xchxfs2earc_tx_32xchxfs");
	mtk_alsa_sw_clock_disable_by_name("viva_earc_tx_64xchxfs2earc_tx_64xchxfs");
	mtk_alsa_sw_clock_disable_by_name("viva_earc_tx_pll2earc_tx_pll");
	mtk_alsa_sw_clock_disable_by_name("viva_earc_tx_synth_iir_ref2earc_tx_synth_iir_ref");
	mtk_alsa_sw_clock_disable_by_name("viva_earc_tx_gpa_synth_ref2earc_tx_gpa_synth_ref");
	mtk_alsa_sw_clock_disable_by_name("viva_earc_tx_synth_ref2earc_tx_synth_ref");
	/* ARC */
	mtk_alsa_sw_clock_disable_by_name("viva_spdif_tx2_256fs2spdif_tx2_256fs");
	mtk_alsa_sw_clock_disable_by_name("viva_spdif_tx2_128fs2spdif_tx2_128fs");
}

int mtk_arc_clk_set_enable(int format)
{
	int ret;

	/* set mux clk : viva_spdif_tx2_128fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_spdif_tx2_128fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_spdif_tx2_128fs_int_ck");
	if (ret)
		return ret;

	/* set mux clk : viva_spdif_tx2_256fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_spdif_tx2_256fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_spdif_tx2_256fs_int_ck");
	if (ret)
		return ret;

	if (format == 0) {
		/* set mux clk : spdif_tx2_128fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx2_128fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_128fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx2_gpa_128fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx2_gpa_128fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_gpa_128fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx2_256fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx2_256fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_256fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx2_gpa_synth_256fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name(
						"spdif_tx2_gpa_synth_256fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_gpa_synth_256fs_mux_int_ck");
		if (ret)
			return ret;
	} else if (format == 1) {
		/* set mux clk : spdif_tx2_128fs_mux_int_ck from dtsi to 2 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx2_128fs_mux_int_ck", 2);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_128fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx2_256fs_mux_int_ck from dtsi to 2 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx2_256fs_mux_int_ck", 2);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_256fs_mux_int_ck");
		if (ret)
			return ret;
	}

	/* enable arc sw clock */
	ret = mtk_arc_sw_clk_set_enable();
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_arc_clk_set_enable);

int mtk_arc_clk_set_disable(int format)
{
	if (format == 0) {
		mtk_alsa_mux_clock_disable_by_name("viva_spdif_tx2_128fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx2_128fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx2_gpa_128fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("viva_spdif_tx2_256fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx2_256fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx2_gpa_synth_256fs_mux_int_ck");
	}

	else if (format == 1) {
		mtk_alsa_mux_clock_disable_by_name("viva_spdif_tx2_128fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx2_128fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("viva_spdif_tx2_256fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("spdif_tx2_256fs_mux_int_ck");

	}
	/* disable arc sw clock */
	mtk_arc_sw_clk_set_disable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_arc_clk_set_disable);

int mtk_hdmi_rx1_sw_clk_set_enable(void)
{
	int ret = 0;

	ret |= mtk_alsa_sw_clock_enable_by_name("viva_hdmi_rx_mpll_4322hdmi_rx_mpll_432");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_hdmi_rx_s2p_256fs2hdmi_rx_s2p_256fs");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_hdmi_rx_s2p_128fs2hdmi_rx_s2p_128fs");

	return ret;
}

void mtk_hdmi_rx1_sw_clk_set_disable(void)
{
	mtk_alsa_sw_clock_disable_by_name("viva_hdmi_rx_mpll_4322hdmi_rx_mpll_432");
	mtk_alsa_sw_clock_disable_by_name("viva_hdmi_rx_s2p_256fs2hdmi_rx_s2p_256fs");
	mtk_alsa_sw_clock_disable_by_name("viva_hdmi_rx_s2p_128fs2hdmi_rx_s2p_128fs");
}

int mtk_hdmi_rx2_sw_clk_set_enable(void)
{
	int ret = 0;

	ret |= mtk_alsa_sw_clock_enable_by_name("viva_hdmi_rx2_mpll_4322hdmi_rx2_mpll_432");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_hdmi_rx2_s2p_256fs2hdmi_rx2_s2p_256fs");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_hdmi_rx2_s2p_128fs2hdmi_rx2_s2p_128fs");

	return ret;
}

void mtk_hdmi_rx2_sw_clk_set_disable(void)
{
	if (priv->ip_version == 0x200)
		return;

	mtk_alsa_sw_clock_disable_by_name("viva_hdmi_rx2_mpll_4322hdmi_rx2_mpll_432");
	mtk_alsa_sw_clock_disable_by_name("viva_hdmi_rx2_s2p_256fs2hdmi_rx2_s2p_256fs");
	mtk_alsa_sw_clock_disable_by_name("viva_hdmi_rx2_s2p_128fs2hdmi_rx2_s2p_128fs");
}

int mtk_earc_clk_set_enable(int format)
{
	int ret;

	/* set mux clk : viva_spdif_tx2_128fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_spdif_tx2_128fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_spdif_tx2_128fs_int_ck");
	if (ret)
		return ret;

	if (format == 0) {
		/* set mux clk : spdif_tx2_128fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx2_128fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_128fs_mux_int_ck");
		if (ret)
			return ret;

		/* set mux clk : spdif_tx2_128fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx2_128fs_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_128fs_mux_int_ck");
		if (ret)
			return ret;
		//wriu     0x10265a             0x02
		ret = mtk_alsa_mux_clock_setting_by_name("earc_tx_pll_256fs_ref_mux_int_ck", 2);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("earc_tx_pll_256fs_ref_mux_int_ck");
		if (ret)
			return ret;
	} else if (format == 1) {
		/* set mux clk : spdif_tx2_128fs_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("spdif_tx2_128fs_mux_int_ck", 2);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("spdif_tx2_128fs_mux_int_ck");
		if (ret)
			return ret;

		//wriu     0x10265a             0x02
		ret = mtk_alsa_mux_clock_setting_by_name("earc_tx_pll_256fs_ref_mux_int_ck", 2);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("earc_tx_pll_256fs_ref_mux_int_ck");
		if (ret)
			return ret;
	}

	/* enable arc sw clock */
	ret = mtk_arc_sw_clk_set_enable();
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_earc_clk_set_enable);

int mtk_earc_clk_set_disable(int format)
{
	if (format == 0) {
		mtk_alsa_mux_clock_disable_by_name("viva_earc_tx_source_256fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("earc_tx_source_256fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("earc_tx_gpa_256fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("earc_tx_pll_256fs_ref_mux_int_ck");
	} else if (format == 1) {
		mtk_alsa_mux_clock_disable_by_name("viva_earc_tx_source_256fs_int_ck");
		mtk_alsa_mux_clock_disable_by_name("earc_tx_source_256fs_mux_int_ck");
		mtk_alsa_mux_clock_disable_by_name("earc_tx_pll_256fs_ref_mux_int_ck");
	}

	/* disable arc sw clock */
	mtk_arc_sw_clk_set_disable();
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_earc_clk_set_disable);

int mtk_hdmi_rx1_clk_set_enable(void)
{
	//struct clk_hw *target_parent;
	int ret;

	/* set mux clk : hdmi_rx_s2p_128fs_mux_int_ck from dtsi to 0 */
	ret = mtk_alsa_mux_clock_setting_by_name("hdmi_rx_s2p_128fs_mux_int_ck", 0);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("hdmi_rx_s2p_128fs_mux_int_ck");
	if (ret)
		return ret;
	/* set mux clk : viva_hdmi_rx_s2p_128fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_hdmi_rx_s2p_128fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_hdmi_rx_s2p_128fs_int_ck");
	if (ret)
		return ret;
	/* set mux clk : hdmi_rx_s2p_256fs_mux_int_ck from dtsi to 0 */
	ret = mtk_alsa_mux_clock_setting_by_name("hdmi_rx_s2p_256fs_mux_int_ck", 0);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("hdmi_rx_s2p_256fs_mux_int_ck");
	if (ret)
		return ret;
	/* set mux clk : viva_hdmi_rx_s2p_256fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_hdmi_rx_s2p_256fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_hdmi_rx_s2p_256fs_int_ck");
	if (ret)
		return ret;
	/* set mux clk : viva_hdmi_rx_mpll_432_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_hdmi_rx_mpll_432_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_hdmi_rx_mpll_432_int_ck");
	if (ret)
		return ret;

	/* enable hdmi rx1 sw clock */
	ret = mtk_hdmi_rx1_sw_clk_set_enable();
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_hdmi_rx1_clk_set_enable);

int mtk_hdmi_rx1_clk_set_disable(void)
{
	mtk_alsa_mux_clock_disable_by_name("hdmi_rx_s2p_128fs_mux_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_hdmi_rx_s2p_128fs_int_ck");
	mtk_alsa_mux_clock_disable_by_name("hdmi_rx_s2p_256fs_mux_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_hdmi_rx_s2p_256fs_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_hdmi_rx_mpll_432_int_ck");

	/* disable hdmi rx1 sw clock */
	mtk_hdmi_rx1_sw_clk_set_disable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_hdmi_rx1_clk_set_disable);

int mtk_hdmi_rx2_clk_set_enable(void)
{
	//struct clk_hw *target_parent;
	int ret;

	if (priv->ip_version == 0x200)
		return 0;

	/* set mux clk : hdmi_rx2_s2p_128fs_mux_int_ck from dtsi to 0 */
	ret = mtk_alsa_mux_clock_setting_by_name("hdmi_rx2_s2p_128fs_mux_int_ck", 0);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("hdmi_rx2_s2p_128fs_mux_int_ck");
	if (ret)
		return ret;
	/* set mux clk : viva_hdmi_rx2_s2p_128fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_hdmi_rx2_s2p_128fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_hdmi_rx2_s2p_128fs_int_ck");
	if (ret)
		return ret;
	/* set mux clk : hdmi_rx2_s2p_256fs_mux_int_ck from dtsi to 0 */
	ret = mtk_alsa_mux_clock_setting_by_name("hdmi_rx2_s2p_256fs_mux_int_ck", 0);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("hdmi_rx2_s2p_256fs_mux_int_ck");
	if (ret)
		return ret;
	/* set mux clk : viva_hdmi_rx2_s2p_256fs_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_hdmi_rx2_s2p_256fs_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_hdmi_rx2_s2p_256fs_int_ck");
	if (ret)
		return ret;
	/* set mux clk : viva_hdmi_rx2_mpll_432_int_ck from dtsi to 1 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_hdmi_rx2_mpll_432_int_ck", 1);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_hdmi_rx2_mpll_432_int_ck");
	if (ret)
		return ret;

	/* enable hdmi rx2 sw clock */
	ret = mtk_hdmi_rx2_sw_clk_set_enable();
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_hdmi_rx2_clk_set_enable);

int mtk_hdmi_rx2_clk_set_disable(void)
{
	if (priv->ip_version == 0x200)
		return 0;

	mtk_alsa_mux_clock_disable_by_name("hdmi_rx2_s2p_128fs_mux_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_hdmi_rx2_s2p_128fs_int_ck");
	mtk_alsa_mux_clock_disable_by_name("hdmi_rx2_s2p_256fs_mux_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_hdmi_rx2_s2p_256fs_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_hdmi_rx2_mpll_432_int_ck");

	/* disable hdmi rx2 sw clock */
	mtk_hdmi_rx2_sw_clk_set_disable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_hdmi_rx2_clk_set_disable);

int mtk_atv_sw_clk_set_enable(void)
{
	int ret = 0;

	ret |= mtk_alsa_sw_clock_enable_by_name("viva_sif_cordic2sif_cordic");
	// ret |= mtk_alsa_sw_clock_enable_by_name("viva_sif_synth2sif_synth");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_fm_demodulator2fm_demodulator");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_sif_adc_cic2sif_adc_cic");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_sif_adc_r2b2sif_adc_r2b");
	ret |= mtk_alsa_sw_clock_enable_by_name("viva_sif_adc_r2b_fifo_in2sif_adc_r2b_fifo_in");

	return ret;
}

void mtk_atv_sw_clk_set_disable(void)
{
	mtk_alsa_sw_clock_disable_by_name("viva_sif_cordic2sif_cordic");
	// mtk_alsa_sw_clock_disable_by_name("viva_sif_synth2sif_synth");
	mtk_alsa_sw_clock_disable_by_name("viva_fm_demodulator2fm_demodulator");
	mtk_alsa_sw_clock_disable_by_name("viva_sif_adc_cic2sif_adc_cic");
	mtk_alsa_sw_clock_disable_by_name("viva_sif_adc_r2b2sif_adc_r2b");
	mtk_alsa_sw_clock_disable_by_name("viva_sif_adc_r2b_fifo_in2sif_adc_r2b_fifo_in");
}

int mtk_atv_clk_set_enable(void)
{
	int ret = 0;

	// FIXME:
	if (priv->ip_version != 0x200) {
		//atv select ch-m7 0x83
		//ch_m7_256_fsi_int need select to ch_m7_256fsi_mux (default)
		//ch_m7_256_fsi_mux need select to ch_m7_256fsi_mux0 (parent 0)
		//ch_m7_256_fsi_mux0 need select to decoder4_256fs_mux_buf_ck (parent 3)
		/* set mux clk : ch_m7_256fsi_mux0_int_ck from dtsi to 3 */
		ret = mtk_alsa_mux_clock_setting_by_name("ch_m7_256fsi_mux0_int_ck", 3);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("ch_m7_256fsi_mux0_int_ck");
		if (ret)
			return ret;
		/* set mux clk : ch_m7_256fsi_mux_int_ck from dtsi to 0 */
		ret = mtk_alsa_mux_clock_setting_by_name("ch_m7_256fsi_mux_int_ck", 0);
		if (ret)
			return ret;
		ret = mtk_alsa_mux_clock_enable_by_name("ch_m7_256fsi_mux_int_ck");
		if (ret)
			return ret;
	}

	if (priv->atv_input_type == ATV_INPUT_VIF_INTERNAL) {
		/* set mux clk : viva_sif_adc_r2b_fifo_in_int_ck from dtsi to 2 */
		ret |= mtk_alsa_mux_clock_setting_by_name("viva_sif_adc_r2b_fifo_in_int_ck", 2);
		ret |= mtk_alsa_mux_clock_enable_by_name("viva_sif_adc_r2b_fifo_in_int_ck");

		/* set mux clk : viva_sif_adc_r2b_int_ck from dtsi to 2 */
		ret |= mtk_alsa_mux_clock_setting_by_name("viva_sif_adc_r2b_int_ck", 2);
		ret |= mtk_alsa_mux_clock_enable_by_name("viva_sif_adc_r2b_int_ck");

		/* set mux clk : viva_sif_adc_cic_int_ck from dtsi to 2 */
		ret |= mtk_alsa_mux_clock_setting_by_name("viva_sif_adc_cic_int_ck", 2);
		ret |= mtk_alsa_mux_clock_enable_by_name("viva_sif_adc_cic_int_ck");

		/* set mux clk : viva_fm_demodulator_int_ck from dtsi to 4 */
		ret |= mtk_alsa_mux_clock_setting_by_name("viva_fm_demodulator_int_ck", 4);
		ret |= mtk_alsa_mux_clock_enable_by_name("viva_fm_demodulator_int_ck");
	} else {
		/* set mux clk : viva_sif_adc_r2b_fifo_in_int_ck from dtsi to 1 */
		ret |= mtk_alsa_mux_clock_setting_by_name("viva_sif_adc_r2b_fifo_in_int_ck", 1);
		ret |= mtk_alsa_mux_clock_enable_by_name("viva_sif_adc_r2b_fifo_in_int_ck");

		/* set mux clk : viva_sif_adc_r2b_int_ck from dtsi to 1 */
		ret |= mtk_alsa_mux_clock_setting_by_name("viva_sif_adc_r2b_int_ck", 1);
		ret |= mtk_alsa_mux_clock_enable_by_name("viva_sif_adc_r2b_int_ck");

		/* set mux clk : viva_sif_adc_cic_int_ck from dtsi to 1 */
		ret |= mtk_alsa_mux_clock_setting_by_name("viva_sif_adc_cic_int_ck", 1);
		ret |= mtk_alsa_mux_clock_enable_by_name("viva_sif_adc_cic_int_ck");

		/* set mux clk : viva_fm_demodulator_int_ck from dtsi to 2 */
		ret |= mtk_alsa_mux_clock_setting_by_name("viva_fm_demodulator_int_ck", 2);
		ret |= mtk_alsa_mux_clock_enable_by_name("viva_fm_demodulator_int_ck");
	}

	/* set mux clk : viva_sif_synth_int_ck from dtsi to 1 */
	ret |= mtk_alsa_mux_clock_setting_by_name("viva_sif_synth_int_ck", 1);
	ret |= mtk_alsa_mux_clock_enable_by_name("viva_sif_synth_int_ck");

	/* set mux clk : viva_sif_cordic_int_ck from dtsi to 4 */
	ret |= mtk_alsa_mux_clock_setting_by_name("viva_sif_cordic_int_ck", 4);
	ret |= mtk_alsa_mux_clock_enable_by_name("viva_sif_cordic_int_ck");

	if (ret) {
		dev_err(priv->dev, "[ALSA MTK] %s set mux clk fail\n", __func__);
		return ret;
	}

	/* enable atv sw clock */
	ret = mtk_atv_sw_clk_set_enable();
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_atv_clk_set_enable);

int mtk_atv_clk_set_disable(void)
{
	// FIXME:
	if (priv->ip_version != 0x200) {
		mtk_alsa_mux_clock_disable_by_name("ch_m7_256fsi_mux0_int_ck");
		mtk_alsa_mux_clock_disable_by_name("ch_m7_256fsi_mux_int_ck");
	}
	mtk_alsa_mux_clock_disable_by_name("viva_sif_cordic_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_sif_synth_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_fm_demodulator_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_sif_adc_cic_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_sif_adc_r2b_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_sif_adc_r2b_fifo_in_int_ck");
	/* disable atv sw clock */
	mtk_atv_sw_clk_set_disable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_atv_clk_set_disable);

int mtk_i2s_rx2_clk_set_enable(void)
{
	int ret;

	if (priv->ip_version == 0x200)
		return 0;

	/* set mux clk : viva_bclk_i2s_decoder2_int_ck from dtsi to 2 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_bclk_i2s_decoder2_int_ck", 2);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_bclk_i2s_decoder2_int_ck");
	if (ret)
		return ret;
	/* set mux clk : viva_mclk_i2s_decoder2_int_ck from dtsi to 3 */
	ret = mtk_alsa_mux_clock_setting_by_name("viva_mclk_i2s_decoder2_int_ck", 3);
	if (ret)
		return ret;
	ret = mtk_alsa_mux_clock_enable_by_name("viva_mclk_i2s_decoder2_int_ck");
	if (ret)
		return ret;
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_i2s_rx2_clk_set_enable);

int mtk_i2s_rx2_clk_set_disable(void)
{
	if (priv->ip_version == 0x200)
		return 0;

	mtk_alsa_mux_clock_disable_by_name("viva_bclk_i2s_decoder2_int_ck");
	mtk_alsa_mux_clock_disable_by_name("viva_mclk_i2s_decoder2_int_ck");

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_i2s_rx2_clk_set_disable);

void mtk_adc0_power_set_enable(void)
{
	/* 0x9502E2[8] = 0 */
	mtk_alsa_write_reg_mask(0x9502E2, 0x100, 0x0);
	/* 0x9502DA[1] = 0 */
	mtk_alsa_write_reg_mask(0x9502DA, 0x2, 0x0);
	/* delay 2ms to wait adc analog stable */
	mtk_alsa_delaytask(2);
}

void mtk_adc0_power_set_disable(void)
{
	/* 0x9502DA[1] = 1 */
	mtk_alsa_write_reg_mask(0x9502DA, 0x2, 0x2);
	/* 0x9502E2[8] = 1 */
	mtk_alsa_write_reg_mask(0x9502E2, 0x100, 0x100);
}

void mtk_adc1_power_set_enable(void)
{
	/* 0x9502E2[9] = 0 */
	mtk_alsa_write_reg_mask(0x9502E2, 0x200, 0x0);
	/* 0x9502DA[0] = 0 */
	mtk_alsa_write_reg_mask(0x9502DA, 0x1, 0x0);
	/* delay 2ms to wait adc analog stable */
	mtk_alsa_delaytask(2);
}

void mtk_adc1_power_set_disable(void)
{
	/* 0x9502DA[0] = 1 */
	mtk_alsa_write_reg_mask(0x9502DA, 0x1, 0x1);
	/* 0x9502E2[9] = 1 */
	mtk_alsa_write_reg_mask(0x9502E2, 0x200, 0x200);
}

int mtk_adc0_clk_set_enable(void)
{
	/* power on ADC0 */
	mtk_adc0_power_set_enable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_adc0_clk_set_enable);

int mtk_adc1_clk_set_enable(void)
{
	/* power on ADC1 */
	mtk_adc1_power_set_enable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_adc1_clk_set_enable);

int mtk_adc0_clk_set_disable(void)
{
	/* power off ADC0 */
	mtk_adc0_power_set_disable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_adc0_clk_set_disable);

int mtk_adc1_clk_set_disable(void)
{
	/* power off ADC1 */
	mtk_adc1_power_set_disable();

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_adc1_clk_set_disable);

int mtk_src1_clk_set_enable(void)
{
	int i = 0;
	int ret;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100)) {
		while (src1_clk_table[i].type != END_OF_TABLE) {
			ret = mtk_alsa_mux_clock_setting(src1_clk_table[i].type
							, src1_clk_table[i].value);
			if (ret)
				return ret;

			ret = mtk_alsa_mux_clock_enable(src1_clk_table[i].type);
			if (ret)
				return ret;

			i++;
		}
	} else {
		while (ip2_src1_clk_table[i].type != END_OF_TABLE) {
			ret = mtk_alsa_mux_clock_setting(ip2_src1_clk_table[i].type
							, ip2_src1_clk_table[i].value);
			if (ret)
				return ret;

			ret = mtk_alsa_mux_clock_enable(ip2_src1_clk_table[i].type);
			if (ret)
				return ret;

			i++;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_src1_clk_set_enable);

int mtk_src1_clk_set_disable(void)
{
	int i = 0;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100)) {
		while (src1_clk_table[i].type != END_OF_TABLE) {
			mtk_alsa_mux_clock_disable(src1_clk_table[i].type);
			i++;
		}
	} else {
		while (ip2_src1_clk_table[i].type != END_OF_TABLE) {
			mtk_alsa_mux_clock_disable(ip2_src1_clk_table[i].type);
			i++;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_src1_clk_set_disable);

int mtk_src2_clk_set_enable(void)
{
	int i = 0;
	int ret;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100)) {
		while (src2_clk_table[i].type != END_OF_TABLE) {
			ret = mtk_alsa_mux_clock_setting(src2_clk_table[i].type
							, src2_clk_table[i].value);
			if (ret)
				return ret;

			ret = mtk_alsa_mux_clock_enable(src2_clk_table[i].type);
			if (ret)
				return ret;
			i++;
		}
	} else {
		while (ip2_src2_clk_table[i].type != END_OF_TABLE) {
			ret = mtk_alsa_mux_clock_setting(ip2_src2_clk_table[i].type
							, ip2_src2_clk_table[i].value);
			if (ret)
				return ret;

			ret = mtk_alsa_mux_clock_enable(ip2_src2_clk_table[i].type);
			if (ret)
				return ret;
			i++;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_src2_clk_set_enable);

int mtk_src2_clk_set_disable(void)
{
	int i = 0;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100)) {
		while (src2_clk_table[i].type != END_OF_TABLE) {
			mtk_alsa_mux_clock_disable(src2_clk_table[i].type);
			i++;
		}
	} else {
		while (ip2_src2_clk_table[i].type != END_OF_TABLE) {
			mtk_alsa_mux_clock_disable(ip2_src2_clk_table[i].type);
			i++;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_src2_clk_set_disable);

int mtk_src3_clk_set_enable(void)
{
	int i = 0;
	int ret;

	if (priv->ip_version == 0x200)
		return 0;

	while (src3_clk_table[i].type != END_OF_TABLE) {
		ret = mtk_alsa_mux_clock_setting(src3_clk_table[i].type, src3_clk_table[i].value);
		if (ret)
			return ret;

		ret = mtk_alsa_mux_clock_enable(src3_clk_table[i].type);
		if (ret)
			return ret;
		i++;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_src3_clk_set_enable);

int mtk_src3_clk_set_disable(void)
{
	int i = 0;

	if (priv->ip_version == 0x200)
		return 0;

	while (src3_clk_table[i].type != END_OF_TABLE) {
		mtk_alsa_mux_clock_disable(src3_clk_table[i].type);
		i++;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_src3_clk_set_disable);

#define	AUDIO_INIT_SIZE	(32*1024)

char pre_init_table[AUDIO_INIT_SIZE] = {0};
char init_table[AUDIO_INIT_SIZE] = {0};
char depop_table[AUDIO_INIT_SIZE] = {0};
char earc_table[AUDIO_INIT_SIZE] = {0};

static int mtk_alsa_read_file(const char *name, int index, struct device *dev)
{
	const struct firmware *fw_entry = NULL;
	char *src_buf;
	char *des_buf;
	int ret;

	if (fw_entry == NULL) {
		ret = request_firmware(&fw_entry, name, dev);
		if (ret) {
			pr_err("firmware_example: Firmware not available\n");
			return ret;
		}
	}

	if (index == 0)
		des_buf = pre_init_table;
	else if (index == 1)
		des_buf = init_table;
	else if (index == 2)
		des_buf = depop_table;
	else
		des_buf = earc_table;

	src_buf = (char *) fw_entry->data;
	strncpy(des_buf, src_buf, sizeof(char) * fw_entry->size);

	if (index == 0)
		pr_err("[ALSA MTK]read pre-init done\n");
	else if (index == 1)
		pr_err("[ALSA MTK]read init done\n");
	else if (index == 2)
		pr_err("[ALSA MTK]read depop off done\n");
	else
		pr_err("[ALSA MTK]read earc done\n");
	ret = 0;

	release_firmware(fw_entry);

	return ret;
}

static int mtk_alsa_init(int index)
{
	char *buf;
	char str1[] = "{0x";
	char str2[] = "0x";
	unsigned int u32Value = 0;
	u16 u16Value = 0;
	unsigned int u32Addr;
	unsigned char u8Mask;
	unsigned char u8Value;
	char *loc = NULL;
	int pos = 0;
	int ret;
	char temp_str[7] = {0};
	int audio_init_flag = 0;

	if (index == 0)
		buf = pre_init_table;
	else if (index == 1)
		buf = init_table;
	else if (index == 2)
		buf = depop_table;
	else
		buf = earc_table;

	while (1) {
		loc = strstr(buf, str1);
		if (loc) {
			pos = loc - buf;
			pos += strlen(str1);

			strncpy(temp_str, (buf + pos), sizeof(int));
			temp_str[4] = '\0';
			ret = kstrtouint(temp_str, 16, &u32Value);
			if (ret != 0) {
				pr_err("[ALSA MTK]read addr high byte fail\n");
				break;
			}
			strncpy(temp_str, (buf + pos + 4), sizeof(u16));
			temp_str[2] = '\0';
			ret = kstrtou16(temp_str, 16, &u16Value);
			if (ret != 0) {
				pr_err("[ALSA MTK]read addr low byte fail\n");
				break;
			}

			u32Addr = (u32Value << 8) | (u16Value);
			buf += pos;
		} else
			break;

		loc = strstr(buf, str2);
		if (loc) {
			pos = loc - buf;
			pos += strlen(str2);

			strncpy(temp_str, (buf + pos), sizeof(u16));
			temp_str[2] = '\0';
			ret = kstrtou16(temp_str, 16, &u16Value);
			if (ret != 0) {
				pr_err("[ALSA MTK]read mask fail\n");
				break;
			}

			u8Mask = u16Value;
			buf += pos;
		} else
			break;

		loc = strstr(buf, str2);
		if (loc) {
			pos = loc - buf;
			pos += strlen(str2);

			strncpy(temp_str, (buf + pos), sizeof(u16));
			temp_str[2] = '\0';
			ret = kstrtou16(temp_str, 16, &u16Value);
			if (ret != 0) {
				pr_err("[ALSA MTK]read value fail\n");
				break;
			}

			u8Value = u16Value;
			buf += pos;
		} else
			break;

		/* {0xffffff, 0x00, 0x00} means end of audio init table */
		if (u32Addr == 0xffffff && u8Mask == 0x0 && u8Value == 0x0) {
			audio_init_flag = 1;
			break;
		}

		if ((u32Addr == 0xFFFFFF) && (u8Mask == 1) && (u8Value > 0))
			msleep_interruptible(u8Value);
		else
			mtk_alsa_write_reg_mask_byte(u32Addr, u8Mask, u8Value);
	}

	if (audio_init_flag == 1) {
		if (index == 0)
			pr_err("[ALSA MTK]do pre-init done\n");
		else if (index == 1)
			pr_err("[ALSA MTK]do init done\n");
		else if (index == 2)
			pr_err("[ALSA MTK]do depop off done\n");
		else
			pr_err("[ALSA MTK]do earc init done\n");
		ret = 0;
	} else {
		if (index == 0)
			pr_err("[ALSA MTK]do pre-init fail\n");
		else if (index == 1)
			pr_err("[ALSA MTK]do init fail\n");
		else if (index == 2)
			pr_err("[ALSA MTK]do depop off fail\n");
		else
			pr_err("[ALSA MTK]do earc init fail\n");
		ret = -EIO;
	}
	return ret;
}

int mtk_alsa_read_chip(void)
{
	int chip_num = 0;

	if (priv->ip_version == 0x0100) {
		pr_info("[ALSA MTK] this is m6L\n");
		chip_num = 1;
	} else {
		pr_info("[ALSA MTK] this is m6\n");
		chip_num = 0;
	}
	return chip_num;
}
EXPORT_SYMBOL_GPL(mtk_alsa_read_chip);

int mtk_alsa_check_atv_input_type(void)
{
	if (priv->atv_input_type == ATV_INPUT_SIF_EXTERNAL)
		return ATV_INPUT_SIF_EXTERNAL;
	else
		return ATV_INPUT_VIF_INTERNAL;
}
EXPORT_SYMBOL_GPL(mtk_alsa_check_atv_input_type);

int mtk_alsa_audio_clock_enable(void)
{
	int ret;
	int i;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100)) {
		// gate clock enable
		for (i = 0; i < GATE_CLK_NUM; i++) {
			ret = mtk_alsa_gate_clock_enable(i);
			if (ret)
				return ret;
		}

		// mux clock enable
		i = 0;
		while (mux_clk_table[i].type != END_OF_TABLE) {
			ret = mtk_alsa_mux_clock_setting(mux_clk_table[i].type
							, mux_clk_table[i].value);
			if (ret)
				return ret;

			ret = mtk_alsa_mux_clock_enable(mux_clk_table[i].type);
			if (ret)
				return ret;
			i++;
		}

		// sw clock enable
		for (i = 0; i < CLK_NUM; i++) {
			ret = mtk_alsa_sw_clock_enable(i);
			if (ret)
				return ret;
		}
	} else {
		// gate clock enable
		for (i = 0; i < IP2_GATE_CLK_NUM; i++) {
			ret = mtk_alsa_gate_clock_enable(i);
			if (ret)
				return ret;
		}

		// mux clock enable
		i = 0;
		while (ip2_mux_clk_table[i].type != END_OF_TABLE) {
			ret = mtk_alsa_mux_clock_setting(ip2_mux_clk_table[i].type
							, ip2_mux_clk_table[i].value);
			if (ret)
				return ret;

			ret = mtk_alsa_mux_clock_enable(ip2_mux_clk_table[i].type);
			if (ret)
				return ret;
			i++;
		}

		// sw clock enable
		for (i = 0; i < IP2_CLK_NUM; i++) {
			ret = mtk_alsa_sw_clock_enable(i);
			if (ret)
				return ret;
		}
	}
	return 0;
}

void mtk_alsa_audio_clock_disable(void)
{
	int i;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100)) {
		// opposite direction to disable clock
		/* disable sw en clock */
		for (i = CLK_NUM - 1; i >= 0; i--)
			mtk_alsa_sw_clock_disable(i);

		/* disable mux clock */
		for (i = MUX_CLK_NUM - 1; i >= 0; i--)
			mtk_alsa_mux_clock_disable(i);

		/* disable gate clock */
		for (i = GATE_CLK_NUM - 1; i >= 0; i--)
			mtk_alsa_gate_clock_disable(i);
	} else {
		// opposite direction to disable clock
		/* disable sw en clock */
		for (i = IP2_CLK_NUM - 1; i >= 0; i--)
			mtk_alsa_sw_clock_disable(i);

		/* disable mux clock */
		for (i = IP2_MUX_CLK_NUM - 1; i >= 0; i--)
			mtk_alsa_mux_clock_disable(i);

		/* disable gate clock */
		for (i = IP2_GATE_CLK_NUM - 1; i >= 0; i--)
			mtk_alsa_gate_clock_disable(i);
	}
}

int mtk_alsa_clock_init(struct platform_device *pdev)
{
	int ret;

	if ((priv->ip_version == 0x0) || (priv->ip_version == 0x100)) {
		/* gate clock */
		ret = mtk_alsa_gate_clock_init(pdev);
		if (ret) {
			dev_err(&pdev->dev, "[ALSA MTK]can't init gate clock\n");
			return ret;
		}
		/* mux clock */
		ret = mtk_alsa_mux_clock_init(pdev);
		if (ret) {
			dev_err(&pdev->dev, "[ALSA MTK]can't init mux clock\n");
			return ret;
		}
		/* sw en clock */
		ret = mtk_alsa_sw_clock_init(pdev);
		if (ret) {
			dev_err(&pdev->dev, "[ALSA MTK]can't init sw clock\n");
			return ret;
		}
	} else {
		/* gate clock */
		ret = mtk_alsa_ip2_gate_clock_init(pdev);
		if (ret) {
			dev_err(&pdev->dev, "[ALSA MTK]can't init ip2 gate clock\n");
			return ret;
		}
		/* mux clock */
		ret = mtk_alsa_ip2_mux_clock_init(pdev);
		if (ret) {
			dev_err(&pdev->dev, "[ALSA MTK]can't init mux clock\n");
			return ret;
		}
		/* sw en clock */
		ret = mtk_alsa_ip2_sw_clock_init(pdev);
		if (ret) {
			dev_err(&pdev->dev, "[ALSA MTK]can't init sw clock\n");
			return ret;
		}
	}
	return 0;
}

static struct snd_soc_dai_driver init_dais[] = {
	{
		.name = "LOAD_INIT",
		.id = 0,
	},
};

void mtk_io_power_disable(void)
{
	/* disable all IO power */
	/* output */
	/* SPDIF TX */
	mtk_spdif_tx_sw_clk_set_disable();
	/* ARC/EARC */
	mtk_arc_sw_clk_set_disable();
	/* input */
	/* HDMI RX1 */
	mtk_hdmi_rx1_sw_clk_set_disable();
	/* HDMI RX2 */
	mtk_hdmi_rx2_sw_clk_set_disable();
	/* ATV */
	mtk_atv_sw_clk_set_disable();
	/* ADC 0 */
	mtk_adc0_power_set_disable();
	/* ADC 1 */
	mtk_adc1_power_set_disable();
	/* SPDIF RX */
	mtk_spdif_rx_sw_clk_set_disable();
}

int mtk_init_comp_suspend(struct snd_soc_component *component)
{
	struct timer_list *timer = &timer_charge;

	if (priv->suspended)
		return 0;

	/* del timer_charge monitor before clock disable */
	if (timer != NULL)
		del_timer_sync(timer);

	/* DSP power down command */
	mtk_alsa_write_reg_mask_byte(0x950331, 0x02, 0x02);
	mtk_alsa_delaytask(5);

	/* disable audio clock */
	mtk_alsa_audio_clock_disable();

	priv->suspended = true;

	return 0;
}

int mtk_init_comp_resume(struct snd_soc_component *component)
{
	int ret;
	struct device *dev = component->dev;

	if (!priv->suspended)
		return 0;

	ret = mtk_alsa_audio_clock_enable();
	if (ret)
		dev_err(priv->dev, "[ALSA MTK] [%s] clock enable fail\n", __func__);

	/* do audio pre init */
	ret = mtk_alsa_init(0);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't load pre-init table\n");
		return -EINVAL;
	}

	/* do audio init */
	ret = mtk_alsa_init(1);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't load init table\n");
		return -EINVAL;
	}

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	/* do earc init */
	ret = mtk_alsa_init(3);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't load earc init table\n");
		return -EINVAL;
	}
#endif

	mtk_io_power_disable();

	priv->suspended = false;

	return 0;
}


static void mtk_charge_monitor(struct timer_list *timer)
{
	unsigned int chg_reg_value;
	unsigned int chg_finished = 0;

	if (timer == NULL) {
		pr_err("[%s] timer is NULL.\n", __func__);
		return;
	}

	chg_reg_value = mtk_alsa_read_reg(0x951040);
	if ((chg_reg_value & 0x4) > 0)
		chg_finished = 1;
	if (chg_finished == 1) {
		mtk_alsa_write_reg_mask(0x9502EC, 0x1000, 0x0000);
		del_timer(timer);
	} else {
		timer->expires = jiffies + msecs_to_jiffies(10);
		add_timer(timer);
	}
}

//[Sysfs] HELP command
static ssize_t help_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_soc_priv *priv;  //need to modify

	if (dev == NULL) {
		pr_err("[AUDIO][ERROR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		pr_err("[AUDIO][ERROR][%s] attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		pr_err("[AUDIO][ERROR][%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 150, "Debug Help:\n"
			"- check audio sw_en clock info\n"
			"                <W>: set sw_en\n"
			"                (0:disable, 1:enable)\n"
			"                <R>: get sw_en\n"
			"- check all audio sw_en:\n"
			"                <R>: Read all audio sw_en status.\n");

}

static ssize_t suspend_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret;
	/* reset DSP */
	mtk_alsa_write_reg_mask_byte(0x950080, 0xff, 0x00);

	ret = mtk_init_comp_suspend(priv->component);

	return sprintf(buf, "%d\n", ret);
}

static ssize_t resume_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret;

	ret = mtk_init_comp_resume(priv->component);

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t spdif_tx_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(char) * 150, "OUTPUT : SPDIF TX clk_status:\n"
			"clock on = 1 ; clock off = 0\n"
			"viva_spdif_tx_256fs %d\n"
			"viva_spdif_tx_128fs %d\n"
			"\n",
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_spdif_tx_256fs2spdif_tx_256fs")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_spdif_tx_128fs2spdif_tx_128fs"))
	);
}

static ssize_t arc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(char) * 300, "OUTPUT ARC clk_status:\n"
			"clock on = 1 ; clock off = 0\n"
			"EARC\n"
			"viva_earc_tx_source_256fs %d\n"
			"viva_earc_tx_32xfs %d\n"
			"viva_earc_tx_32xchxfs %d\n"
			"viva_earc_tx_64xchxfs %d\n"
			"viva_earc_tx_pll %d\n"
			"viva_earc_tx_synth_iir_ref %d\n"
			"viva_earc_tx_gpa_synth_ref %d\n"
			"viva_earc_tx_synth_ref %d\n"
			"ARC\n"
			"viva_spdif_tx2_256fs %d\n"
			"viva_spdif_tx2_128fs %d\n"
			"\n",
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_earc_tx_source_256fs2earc_tx_source_256fs")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_earc_tx_32xfs2earc_tx_32xfs")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_earc_tx_32xchxfs2earc_tx_32xchxfs")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_earc_tx_64xchxfs2earc_tx_64xchxfs")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_earc_tx_pll2earc_tx_pll")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_earc_tx_synth_iir_ref2earc_tx_synth_iir_ref")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_earc_tx_gpa_synth_ref2earc_tx_gpa_synth_ref")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_earc_tx_synth_ref2earc_tx_synth_ref")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_spdif_tx2_256fs2spdif_tx2_256fs")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_spdif_tx2_128fs2spdif_tx2_128fs"))
	);
}

static ssize_t atv_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(char) * 300, "INPUT : ATV clk_status:\n"
			"clock on = 1 ; clock off = 0\n"
			"viva_sif_cordic %d\n"
			"viva_sif_synth %d\n"
			"viva_fm_demodulator %d\n"
			"viva_sif_adc_cic %d\n"
			"viva_sif_adc_r2b %d\n"
			"viva_sif_adc_r2b_fifo_in %d\n"
			"\n",
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_sif_cordic2sif_cordic")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_sif_synth2sif_synth")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_fm_demodulator2fm_demodulator")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_sif_adc_cic2sif_adc_cic")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_sif_adc_r2b2sif_adc_r2b")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_sif_adc_r2b_fifo_in2sif_adc_r2b_fifo_in"))
	);
}

static ssize_t adc0_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(char) * 200, "INPUT : LINE-IN 0 power_status:\n"
			"power on = 0 ; power down = 1\n"
			"REG_ADC0_PD %d\n"
			"REG_ADC0_INMUX_PD %d\n"
			"\n",
			((mtk_alsa_read_reg(0x9502DA) & 0x2) >> 1),
			((mtk_alsa_read_reg(0x9502E2) & 0x100) >> 8)
	);
}

static ssize_t adc1_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(char) * 150, "INPUT : LINE-IN 1 power_status:\n"
			"power on = 0 ; power down = 1\n"
			"REG_ADC1_PD %d\n"
			"REG_ADC1_INMUX_PD %d\n"
			"\n",
			(mtk_alsa_read_reg(0x9502DA) & 0x1),
			((mtk_alsa_read_reg(0x9502E2) & 0x200) >> 9)
	);
}

static ssize_t hdmi_rx1_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(char) * 200, "INPUT : HDMI RX1 clk_status:\n"
			"clock on = 1 ; clock off = 0\n"
			"viva_hdmi_rx_mpll_432 %d\n"
			"viva_hdmi_rx_s2p_256fs %d\n"
			"viva_hdmi_rx_s2p_128fs %d\n"
			"\n",
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_hdmi_rx_mpll_4322hdmi_rx_mpll_432")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_hdmi_rx_s2p_256fs2hdmi_rx_s2p_256fs")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_hdmi_rx_s2p_128fs2hdmi_rx_s2p_128fs"))
	);
}

static ssize_t hdmi_rx2_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(char) * 200, "INPUT : HDMI RX2 clk_status:\n"
			"clock on = 1 ; clock off = 0\n"
			"viva_hdmi_rx2_mpll_432 %d\n"
			"viva_hdmi_rx2_s2p_256fs %d\n"
			"viva_hdmi_rx2_s2p_128fs %d\n"
			"\n",
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_hdmi_rx2_mpll_4322hdmi_rx2_mpll_432")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_hdmi_rx2_s2p_256fs2hdmi_rx2_s2p_256fs")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_hdmi_rx2_s2p_128fs2hdmi_rx2_s2p_128fs"))
	);
}

static ssize_t spdif_rx_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(char) * 150, "INPUT : SPDIF RX clk_status:\n"
			"clock on = 1 ; clock off = 0\n"
			"viva_spdif_rx_synth_ref %d\n"
			"viva_spdif_rx_dg %d\n"
			"viva_spdif_rx_128fs %d\n"
			"\n",
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_spdif_rx_synth_ref2spdif_rx_synth_ref")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_spdif_rx_dg2spdif_rx_dg")),
			__clk_is_enabled(devm_clk_get(priv->dev
					, "viva_spdif_rx_128fs2spdif_rx_128fs"))
	);
}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RO(suspend);
static DEVICE_ATTR_RO(resume);
/* output */
static DEVICE_ATTR_RO(spdif_tx);
static DEVICE_ATTR_RO(arc);
/* input */
static DEVICE_ATTR_RO(atv);
static DEVICE_ATTR_RO(adc0);
static DEVICE_ATTR_RO(adc1);
static DEVICE_ATTR_RO(hdmi_rx1);
static DEVICE_ATTR_RO(hdmi_rx2);
static DEVICE_ATTR_RO(spdif_rx);

static struct attribute *hdmi_rx2_attrs[] = {
	&dev_attr_hdmi_rx2.attr,
	NULL,
};

static struct attribute *debug_attrs[] = {
	&dev_attr_help.attr,
	NULL,
};

static struct attribute *test_attrs[] = {
	&dev_attr_suspend.attr,
	&dev_attr_resume.attr,
	NULL,
};

static struct attribute *power_attrs[] = {
	&dev_attr_spdif_tx.attr,
	&dev_attr_arc.attr,
	&dev_attr_atv.attr,
	&dev_attr_adc0.attr,
	&dev_attr_adc1.attr,
	&dev_attr_hdmi_rx1.attr,
	&dev_attr_spdif_rx.attr,
	NULL,
};

static const struct attribute_group hdmi_rx2_attr_group = {
	.name = "mtk_dbg",
	.attrs = hdmi_rx2_attrs,
};

static const struct attribute_group debug_attr_group = {
	.name = "mtk_dbg",
	.attrs = debug_attrs,
};

static const struct attribute_group test_attr_group = {
	.name = "mtk_test",
	.attrs = test_attrs,
};

static const struct attribute_group power_attr_group = {
	.name = "mtk_power",
	.attrs = power_attrs,
};

static const struct attribute_group *mtk_soc_attr_groups[] = {
	&debug_attr_group,
	&test_attr_group,
	&power_attr_group,
	NULL,
};

static int mtk_init_probe(struct snd_soc_component *component)
{
	struct mtk_soc_priv  *priv;
	struct device *dev;
	int ret;
	struct snd_soc_dai_link *dai_link;

	if (component == NULL) {
		pr_err("[AUDIO][ERROR][%s]component can't be NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (component->dev == NULL) {
		pr_err("[AUDIO][ERROR][%s]component->dev can't be NULL!!!\n", __func__);
		return -EINVAL;
	}

	priv = snd_soc_component_get_drvdata(component);

	if (priv == NULL) {
		pr_err("[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return -EFAULT;
	}

	dev = component->dev;
	ret = 0;

	/* Create device attribute files */
	ret = sysfs_create_groups(&dev->kobj, mtk_soc_attr_groups);
	if (ret)
		pr_err("[AUDIO][ERROR][%s]sysfs_create_groups fail !!!\n", __func__);

	for_each_card_links(component->card, dai_link) {
		if ((!strncmp(dai_link->cpus->dai_name, "HDMI-RX2", strlen("HDMI-RX2"))) &&
			(dai_link->ignore == 0)) {
			ret = sysfs_merge_group(&dev->kobj, &hdmi_rx2_attr_group);
			if (ret)
				pr_err("[AUDIO][ERROR][%s]sysfs_merge_groups fail!\n", __func__);
		}
	}

	priv->component = component;

	return 0;
}

static void mtk_init_remove(struct snd_soc_component *component)
{
	struct mtk_soc_priv *priv;
	struct device *dev;

	priv = NULL;
	dev = NULL;

	if (component == NULL)
		pr_err("[AUDIO][ERROR][%s]component can't be NULL!!!\n", __func__);

	if ((component != NULL) && (component->dev == NULL))
		pr_err("[AUDIO][ERROR][%s]component->dev can't be NULL!!!\n", __func__);

	if ((component != NULL) && (component->dev != NULL))
		priv = snd_soc_component_get_drvdata(component);

	if (priv == NULL)
		pr_err("[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);

	if (component != NULL)
		dev = component->dev;

	if (dev != NULL) {
		sysfs_unmerge_group(&dev->kobj, &hdmi_rx2_attr_group);
		sysfs_remove_groups(&dev->kobj, mtk_soc_attr_groups);
	}
}

#define MTK_CHIP_NAME		"MT5896"
const struct snd_soc_component_driver mtk_chip_platform = {
	.name		= MTK_CHIP_NAME,
	.probe		= mtk_init_probe,
	.remove		= mtk_init_remove,
	.suspend	= mtk_init_comp_suspend,
	.resume		= mtk_init_comp_resume,
};

int mtk_soc_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node;
	struct resource *res;
	const char *name = NULL;
	int ret;
	int i;
	int *pu32array;
	int ip_version;
	int atv_input_type;

	node = dev->of_node;
	if (!node)
		return -ENODEV;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;

	/* parse dts */
	ret = of_property_read_u32(node, "riu_base", &priv->riu_base);
	if (ret) {
		dev_err(dev, "[ALSA MTK]can't get riu base\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "audio_bank_num", &priv->bank_num);
	if (ret) {
		dev_err(dev, "[ALSA MTK]can't get bank num\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "ip-version", &ip_version);
	if (ret) {
		dev_err(dev, "[ALSA MTK] can't ip-version\n");
		return -EINVAL;
	}

	priv->ip_version = ip_version;

	ret = of_property_read_u32(node, "atv_input_type", &atv_input_type);
	if (ret) {
		dev_err(dev, "[ALSA MTK] can't atv_input_type\n");
		return -EINVAL;
	}

	priv->atv_input_type = atv_input_type;

	/* do audio clock init */
	ret = mtk_alsa_clock_init(pdev);
	if (ret) {
		dev_err(dev, "[ALSA MTK]can't init sw clock\n");
		return ret;
	}

	/* enable audio clock */
	ret = mtk_alsa_audio_clock_enable();
	if (ret) {
		dev_err(dev, "[ALSA MTK]audio clock enable fail\n");
		return ret;
	}

	priv->reg_bank = devm_kzalloc(&pdev->dev,
			sizeof(unsigned int) * priv->bank_num, GFP_KERNEL);
	if (!priv->reg_bank)
		return -ENOMEM;

	priv->reg_base = devm_kzalloc(&pdev->dev,
			sizeof(void *) * priv->bank_num, GFP_KERNEL);
	if (!priv->reg_base)
		return -ENOMEM;

	pu32array = kmalloc(sizeof(int) * IOMAP_NUM, GFP_KERNEL);

	ret = of_property_read_u32_array(node, "ioremap", pu32array, IOMAP_NUM);
	if (ret) {
		dev_err(dev, "[ALSA MTK] not support EARC\n");
	}

	for (i = 0; i < priv->bank_num; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(dev, "[ALSA MTK]fail get vivaldi addr %d\n", i);
			kfree(pu32array);
			return -EINVAL;
		}

		if ((res->start == pu32array[IOMAP_1]) || (res->start == pu32array[IOMAP_2]) ||
			(res->start == pu32array[IOMAP_3])) {
			priv->reg_base[i] = of_iomap(node, i);
		} else
			priv->reg_base[i] = devm_ioremap_resource(&pdev->dev, res);

		if (IS_ERR((const void *)priv->reg_base[i])) {
			dev_err(&pdev->dev, "[ALSA MTK]err reg_base=%lx\n",
					(unsigned long)priv->reg_base[i]);
			kfree(pu32array);
			return PTR_ERR((const void *)priv->reg_base[i]);
		}
		priv->reg_bank[i] = (res->start - priv->riu_base) >> 1;

	}

	kfree(pu32array);

	/* do audio pre init */
	ret = of_property_read_string(node, "pre_init_table", &name);
	if (ret) {
		dev_err(dev, "[ALSA MTK]can't get audio pre init table path\n");
		return -EINVAL;
	}
	priv->pre_init_table = name;
	ret = mtk_alsa_read_file(priv->pre_init_table, 0, dev);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't read pre-init table\n");
		return -EINVAL;
	}

	if (mtk_alsa_read_reg_byte(0x951040) & 0x4)
		dev_err(dev, "[ALSA MTK] don't need to re-do audio pre init\n");
	else {
		ret = mtk_alsa_init(0);
		if (ret < 0) {
			dev_err(dev, "[ALSA MTK]can't load pre-init table\n");
			return -EINVAL;
		}
	}
	name = NULL;

	/* do audio init */
	ret = of_property_read_string(node, "init_table", &name);
	if (ret) {
		dev_err(dev, "[ALSA MTK]can't get audio init table path\n");
		return -EINVAL;
	}
	priv->init_table = name;
	ret = mtk_alsa_read_file(priv->init_table, 1, dev);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't read init table\n");
		return -EINVAL;
	}
	if (mtk_alsa_read_reg_byte(0x951040) & 0x4)
		dev_err(dev, "[ALSA MTK] don't need to re-do audio init\n");
	else {
		ret = mtk_alsa_init(1);
		if (ret < 0) {
			dev_err(dev, "[ALSA MTK]can't load init table\n");
			return -EINVAL;
		}
	}
	name = NULL;

	/* get audio depop table */
	ret = of_property_read_string(node, "depop_table", &name);
	if (ret) {
		dev_err(dev, "[ALSA MTK]can't get audio depop table path\n");
		return -EINVAL;
	}
	priv->depop_table = name;
	ret = mtk_alsa_read_file(priv->depop_table, 2, dev);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't read depop off table\n");
		return -EINVAL;
	}

	/* Charge timer setup */
	timer_setup(&timer_charge, mtk_charge_monitor, 0);
	timer_charge.expires = jiffies + msecs_to_jiffies(10);
	add_timer(&timer_charge);

	#ifndef CONFIG_MSTAR_ARM_BD_FPGA

	//in haps env not insert earc clk
	//audio cant write e240 regisiter
	ret = of_property_read_string(node, "earc_table", &name);
	if (ret) {
		dev_err(dev, "[ALSA MTK]can't get audio earc table path\n");
		return -EINVAL;
	}
	priv->earc_table = name;
	ret = mtk_alsa_read_file(priv->earc_table, 3, dev);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't read earc table\n");
		return -EINVAL;
	}
	ret = mtk_alsa_init(3);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't load earc table\n");
		return -EINVAL;
	}

	#endif

	platform_set_drvdata(pdev, priv);

	ret = devm_snd_soc_register_component(&pdev->dev, &mtk_chip_platform,
						init_dais, ARRAY_SIZE(init_dais));
	if (ret) {
		dev_err(dev, "[ALSA MTK]soc_register_component fail %d\n", ret);
		return ret;
	}

	mtk_io_power_disable();

	return 0;
}

int mtk_soc_dev_remove(struct platform_device *pdev)
{
	return 0;
}
#ifdef CONFIG_PM
int mtk_soc_pm_prepare(struct device *dev)
{
	/* PM prepare */
	return 0;
}
int mtk_soc_pm_suspend(struct device *dev)
{
	int ret;
	struct timer_list *timer = &timer_charge;

	/* del timer_charge monitor before clock disable */
	if (timer != NULL)
		del_timer_sync(timer);

	/* load audio depop off table*/
	ret = mtk_alsa_init(2);
	if (ret < 0) {
		dev_err(dev, "[ALSA MTK]can't load depop off table\n");
		return -EINVAL;
	}
	return 0;
}

int mtk_soc_pm_resume(struct device *dev)
{
	/* The audio depop on table has been set up in audio init*/

	/* Charge timer setup */
	timer_setup(&timer_charge, mtk_charge_monitor, 0);
	timer_charge.expires = jiffies + msecs_to_jiffies(10);
	add_timer(&timer_charge);
	return 0;
}

void mtk_soc_pm_complete(struct device *dev)
{
	/* PM complete */
}

static const struct dev_pm_ops mtk_soc_pm_ops = {
	.prepare = mtk_soc_pm_prepare,
	.suspend = mtk_soc_pm_suspend,
	.resume = mtk_soc_pm_resume,
	.freeze = mtk_soc_pm_suspend,
	.thaw = mtk_soc_pm_resume,
	.poweroff = mtk_soc_pm_suspend,
	.restore = mtk_soc_pm_resume,
	.complete = mtk_soc_pm_complete,
};
#endif

static const struct of_device_id mtk_soc_dt_match[] = {
	{ .compatible = "mediatek,mtk-soc", },
	{ }
};
MODULE_DEVICE_TABLE(of, mtk_soc_dt_match);

static struct platform_driver mtk_soc_driver = {
	.driver = {
		.name		= "mtk-soc",
		.of_match_table	= mtk_soc_dt_match,
#ifdef CONFIG_PM
		.pm		= &mtk_soc_pm_ops,
#endif
	},
	.probe	= mtk_soc_dev_probe,
	.remove	= mtk_soc_dev_remove,
};

module_platform_driver(mtk_soc_driver);

MODULE_DESCRIPTION("ALSA SoC driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("AUDIO soc driver");
