// SPDX-License-Identifier: GPL-2.0
/*
 * mtk_alsa_evb.c  --  machine driver
 *
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/vmalloc.h>

#include "mtk_alsa_chip.h"

enum {
	DAI_LINK_SRC1  = 0,
	DAI_LINK_SRC2,
	DAI_LINK_SRC3,
	/* FE */
	DAI_LINK_SPEAKER_PLAYBACK,
	DAI_LINK_HEADPHONE_PLAYBACK,
	DAI_LINK_LINEOUT_PLAYBACK,
	DAI_LINK_HFP_TX_PLAYBACK,
	/* DIGITAL OUT */
	DAI_LINK_SPDIF_PLAYBACK,
	DAI_LINK_SPDIF_NONPCM_PLAYBACK,
	DAI_LINK_ARC_PLAYBACK,
	DAI_LINK_ARC_NONPCM_PLAYBACK,
	DAI_LINK_EARC_PLAYBACK,
	DAI_LINK_EARC_NONPCM_PLAYBACK,
	/* INPUT */
	DAI_LINK_HDMI1_CAPTURE,
	DAI_LINK_HDMI2_CAPTURE,
	DAI_LINK_ATV_CAPTURE,
	DAI_LINK_ADC_CAPTURE,
	DAI_LINK_ADC1_CAPTURE,
	DAI_LINK_I2S_RX2_CAPTURE,
	DAI_LINK_LOOPBACK_CAPTURE,
	DAI_LINK_HFP_RX_CAPTURE,
	/* BE */
	DAI_LINK_I2S0_BE,
	DAI_LINK_I2S5_BE,
	DAI_LINK_DAC0_BE,
	DAI_LINK_DAC1_BE,
	/* DSP LOAD CODE DAI*/
	DAI_LINK_DSP_LOAD,
	DAI_LINK_INIT_LOAD,
};

SND_SOC_DAILINK_DEFS(fe_src1,
	DAILINK_COMP_ARRAY(COMP_CPU("SRC1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-duplex-platform")));

SND_SOC_DAILINK_DEFS(fe_src2,
	DAILINK_COMP_ARRAY(COMP_CPU("SRC2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-duplex-platform")));

SND_SOC_DAILINK_DEFS(fe_src3,
	DAILINK_COMP_ARRAY(COMP_CPU("SRC3")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-duplex-platform")));

SND_SOC_DAILINK_DEFS(fe_speaker_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("SPEAKER")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-playback-platform")));

SND_SOC_DAILINK_DEFS(fe_headphone_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("HEADPHONE")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-playback-platform")));

SND_SOC_DAILINK_DEFS(fe_lineout_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("LINEOUT")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-playback-platform")));

SND_SOC_DAILINK_DEFS(fe_hfp_tx_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("HFP-TX")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-playback-platform")));

SND_SOC_DAILINK_DEFS(fe_spdif_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("SPDIF-TX")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-dplayback-platform")));

SND_SOC_DAILINK_DEFS(fe_spdif_nonpcm_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("SPDIF-TX-NONPCM")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-dplayback-platform")));

SND_SOC_DAILINK_DEFS(fe_arc_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("ARC")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-dplayback-platform")));

SND_SOC_DAILINK_DEFS(fe_arc_nonpcm_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("ARC-NONPCM")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-dplayback-platform")));

SND_SOC_DAILINK_DEFS(fe_earc_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("EARC")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-dplayback-platform")));

SND_SOC_DAILINK_DEFS(fe_earc_nonpcm_playback,
	DAILINK_COMP_ARRAY(COMP_CPU("EARC-NONPCM")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-dplayback-platform")));

SND_SOC_DAILINK_DEFS(fe_hdmi1_capture,
	DAILINK_COMP_ARRAY(COMP_CPU("HDMI-RX1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-capture-platform")));

SND_SOC_DAILINK_DEFS(fe_hdmi2_capture,
	DAILINK_COMP_ARRAY(COMP_CPU("HDMI-RX2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-capture-platform")));

SND_SOC_DAILINK_DEFS(fe_adc_capture,
	DAILINK_COMP_ARRAY(COMP_CPU("ADC")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-capture-platform")));

SND_SOC_DAILINK_DEFS(fe_adc1_capture,
	DAILINK_COMP_ARRAY(COMP_CPU("ADC1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-capture-platform")));

SND_SOC_DAILINK_DEFS(fe_i2s_rx2_capture,
	DAILINK_COMP_ARRAY(COMP_CPU("I2S-RX2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-capture-platform")));

SND_SOC_DAILINK_DEFS(fe_loopback_capture,
	DAILINK_COMP_ARRAY(COMP_CPU("LOOPBACK")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-capture-platform")));

SND_SOC_DAILINK_DEFS(fe_hfp_rx_capture,
	DAILINK_COMP_ARRAY(COMP_CPU("HFP-RX")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-capture-platform")));

SND_SOC_DAILINK_DEFS(fe_atv_capture,
	DAILINK_COMP_ARRAY(COMP_CPU("ATV")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-capture-platform")));

SND_SOC_DAILINK_DEFS(be_i2s0,
	DAILINK_COMP_ARRAY(COMP_CPU("BE_I2S0")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-playback-platform")));

SND_SOC_DAILINK_DEFS(be_i2s5,
	DAILINK_COMP_ARRAY(COMP_CPU("BE_I2S5")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-playback-platform")));

SND_SOC_DAILINK_DEFS(be_dac0,
	DAILINK_COMP_ARRAY(COMP_CPU("BE_DAC0")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-playback-platform")));

SND_SOC_DAILINK_DEFS(be_dac1,
	DAILINK_COMP_ARRAY(COMP_CPU("BE_DAC1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-playback-platform")));

SND_SOC_DAILINK_DEFS(load_dsp,
	DAILINK_COMP_ARRAY(COMP_CPU("LOAD_DSP")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("mtk-sound-coprocessor-platform")));

SND_SOC_DAILINK_DEFS(load_init,
	DAILINK_COMP_ARRAY(COMP_CPU("LOAD_INIT")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("1d2a0000.mtk-sound-soc")));

static struct snd_soc_dai_link mtk_snd_dais[] = {
	[DAI_LINK_SRC1] = {
		.name = "SRC1",
		.stream_name = "SRC1",
		.id = DAI_LINK_SRC1,
		SND_SOC_DAILINK_REG(fe_src1),
	},
	[DAI_LINK_SRC2] = {
		.name = "SRC2",
		.stream_name = "SRC2",
		.id = DAI_LINK_SRC2,
		SND_SOC_DAILINK_REG(fe_src2),
	},
	[DAI_LINK_SRC3] = {
		.name = "SRC3",
		.stream_name = "SRC3",
		.id = DAI_LINK_SRC3,
		SND_SOC_DAILINK_REG(fe_src3),
	},
	/* Front End DAI links */
	[DAI_LINK_SPEAKER_PLAYBACK] = {
		.name = "Speaker Playback",
		.stream_name = "Speaker Playback",
		.id = DAI_LINK_SPEAKER_PLAYBACK,
		.dynamic = 1,
		.dpcm_playback = 1,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_speaker_playback),
		},
	[DAI_LINK_HEADPHONE_PLAYBACK] = {
		.name = "Headphone Playback",
		.stream_name = "Headphone Playback",
		.id = DAI_LINK_HEADPHONE_PLAYBACK,
		.dynamic = 1,
		.dpcm_playback = 1,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_headphone_playback),
	},
	[DAI_LINK_LINEOUT_PLAYBACK] = {
		.name = "Lineout Playback",
		.stream_name = "Lineout Playback",
		.id = DAI_LINK_LINEOUT_PLAYBACK,
		.dynamic = 1,
		.dpcm_playback = 1,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_lineout_playback),
	},
	[DAI_LINK_HFP_TX_PLAYBACK] = {
		.name = "Hand-Free Profile Playback",
		.stream_name = "Hand-Free Profile Playback",
		.id = DAI_LINK_HFP_TX_PLAYBACK,
		.dynamic = 1,
		.dpcm_playback = 1,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_hfp_tx_playback),
	},
	/* Digital out links */
	[DAI_LINK_SPDIF_PLAYBACK] = {
		.name = "SPDIF-TX Playback",
		.stream_name = "SPDIF-TX Playback",
		.id = DAI_LINK_SPDIF_PLAYBACK,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_spdif_playback),
	},
	[DAI_LINK_SPDIF_NONPCM_PLAYBACK] = {
		.name = "SPDIF-TX NONPCM Playback",
		.stream_name = "SPDIF-TX NONPCM Playback",
		.id = DAI_LINK_SPDIF_NONPCM_PLAYBACK,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_spdif_nonpcm_playback),
	},
	[DAI_LINK_ARC_PLAYBACK] = {
		.name = "ARC Playback",
		.stream_name = "ARC Playback",
		.id = DAI_LINK_ARC_PLAYBACK,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_arc_playback),
	},
	[DAI_LINK_ARC_NONPCM_PLAYBACK] = {
		.name = "ARC NONPCM Playback",
		.stream_name = "ARC NONPCM Playback",
		.id = DAI_LINK_ARC_NONPCM_PLAYBACK,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_arc_nonpcm_playback),
	},
	[DAI_LINK_EARC_PLAYBACK] = {
		.name = "EARC Playback",
		.stream_name = "EARC Playback",
		.id = DAI_LINK_EARC_PLAYBACK,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_earc_playback),
	},
	[DAI_LINK_EARC_NONPCM_PLAYBACK] = {
		.name = "EARC NONPCM Playback",
		.stream_name = "EARC NONPCM Playback",
		.id = DAI_LINK_EARC_NONPCM_PLAYBACK,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(fe_earc_nonpcm_playback),
	},
	/* input DAI links */
	[DAI_LINK_HDMI1_CAPTURE] = {
		.name = "HDMI-RX1 Capture",
		.stream_name = "HDMI-RX1 Capture",
		.id = DAI_LINK_HDMI1_CAPTURE,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(fe_hdmi1_capture),
	},
	[DAI_LINK_HDMI2_CAPTURE] = {
		.name = "HDMI-RX2 Capture",
		.stream_name = "HDMI-RX2 Capture",
		.id = DAI_LINK_HDMI2_CAPTURE,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(fe_hdmi2_capture),
	},
	[DAI_LINK_ADC_CAPTURE] = {
		.name = "ADC Capture",
		.stream_name = "ADC Capture",
		.id = DAI_LINK_ADC_CAPTURE,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(fe_adc_capture),
	},
	[DAI_LINK_ADC1_CAPTURE] = {
		.name = "ADC1 Capture",
		.stream_name = "ADC1 Capture",
		.id = DAI_LINK_ADC1_CAPTURE,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(fe_adc1_capture),
	},
	[DAI_LINK_I2S_RX2_CAPTURE] = {
		.name = "I2S-RX2 Capture",
		.stream_name = "I2S-RX2 Capture",
		.id = DAI_LINK_I2S_RX2_CAPTURE,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(fe_i2s_rx2_capture),
	},
	[DAI_LINK_LOOPBACK_CAPTURE] = {
		.name = "LOOPBACK Capture",
		.stream_name = "LOOPBACK Capture",
		.id = DAI_LINK_LOOPBACK_CAPTURE,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(fe_loopback_capture),
	},
	[DAI_LINK_HFP_RX_CAPTURE] = {
		.name = "Hand-Free Profile Capture",
		.stream_name = "Hand-Free Profile Capture",
		.id = DAI_LINK_HFP_RX_CAPTURE,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(fe_hfp_rx_capture),
	},
	[DAI_LINK_ATV_CAPTURE] = {
		.name = "ATV Capture",
		.stream_name = "ATV Capture",
		.id = DAI_LINK_ATV_CAPTURE,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(fe_atv_capture),
	},
	/* Back End DAI links */
	[DAI_LINK_I2S0_BE] = {
		.name = "I2S0",
		.no_pcm = 1,
		.id = DAI_LINK_I2S0_BE,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.dpcm_playback = 1,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(be_i2s0),
	},
	[DAI_LINK_I2S5_BE] = {
		.name = "I2S5",
		.no_pcm = 1,
		.id = DAI_LINK_I2S5_BE,
		.dai_fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.dpcm_playback = 1,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(be_i2s5),
	},
	[DAI_LINK_DAC0_BE] = {
		.name = "DAC0",
		.no_pcm = 1,
		.id = DAI_LINK_DAC0_BE,
		.dpcm_playback = 1,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(be_dac0),
	},
	[DAI_LINK_DAC1_BE] = {
		.name = "DAC1",
		.no_pcm = 1,
		.id = DAI_LINK_DAC1_BE,
		.dpcm_playback = 1,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(be_dac1),
	},
	/* only for dsp platform probe, always at last */
	[DAI_LINK_DSP_LOAD] = {
		.name = "Load DSP",
		.no_pcm = 1,
		.id = DAI_LINK_DSP_LOAD,
		SND_SOC_DAILINK_REG(load_dsp),
	},
	[DAI_LINK_INIT_LOAD] = {
		.name = "Load INIT",
		.no_pcm = 1,
		.id = DAI_LINK_INIT_LOAD,
		SND_SOC_DAILINK_REG(load_init),
	},
};

static struct snd_soc_card mtk_snd_card = {
	.name = "mtk-dtv-snd-card",
	.long_name = "MTK-TV-ASOC-AUDIO-DRV",
	.owner = THIS_MODULE,
	.dai_link = mtk_snd_dais,
	.num_links = ARRAY_SIZE(mtk_snd_dais),
};

static int mtk_parse_duplex_capability(struct device_node *np)
{
	const char *src3_status = NULL;
	int ret = 0;

	if (!of_property_read_bool(np, "src1"))
		mtk_snd_dais[DAI_LINK_SRC1].ignore = 1;

	if (!of_property_read_bool(np, "src2"))
		mtk_snd_dais[DAI_LINK_SRC2].ignore = 1;

	if (!of_property_read_bool(np, "src3"))
		mtk_snd_dais[DAI_LINK_SRC3].ignore = 1;
	else {
		ret = of_property_read_string(np, "src3", &src3_status);

		if (ret == 0) {
			if (!strncmp(src3_status, "disable", strlen("disable")))
				mtk_snd_dais[DAI_LINK_SRC3].ignore = 1;
		} else
			pr_err("%s[%d] parser src3 dto error\n", __func__, __LINE__);
	}

	return 0;
}

static int mtk_parse_playback_capability(struct device_node *np)
{
	const char *hfp_tx_status = NULL;
	int ret = 0;

	if (!of_property_read_bool(np, "speaker"))
		mtk_snd_dais[DAI_LINK_SPEAKER_PLAYBACK].ignore = 1;

	if (!of_property_read_bool(np, "headphone"))
		mtk_snd_dais[DAI_LINK_HEADPHONE_PLAYBACK].ignore = 1;

	if (!of_property_read_bool(np, "lineout"))
		mtk_snd_dais[DAI_LINK_LINEOUT_PLAYBACK].ignore = 1;

	if (!of_property_read_bool(np, "hfp"))
		mtk_snd_dais[DAI_LINK_HFP_TX_PLAYBACK].ignore = 1;
	else {
		ret = of_property_read_string(np, "hfp", &hfp_tx_status);

		if (ret == 0) {
			if (!strncmp(hfp_tx_status, "disable", strlen("disable")))
				mtk_snd_dais[DAI_LINK_HFP_TX_PLAYBACK].ignore = 1;
		} else
			pr_err("%s[%d] parser hfp dto error\n", __func__, __LINE__);
	}

	return 0;
}

static int mtk_parse_dplayback_capability(struct device_node *np)
{
	if (!of_property_read_bool(np, "spdif-tx")) {
		mtk_snd_dais[DAI_LINK_SPDIF_PLAYBACK].ignore = 1;
		mtk_snd_dais[DAI_LINK_SPDIF_NONPCM_PLAYBACK].ignore = 1;
	}

	if (!of_property_read_bool(np, "arc")) {
		mtk_snd_dais[DAI_LINK_ARC_PLAYBACK].ignore = 1;
		mtk_snd_dais[DAI_LINK_ARC_NONPCM_PLAYBACK].ignore = 1;
	}

	if (!of_property_read_bool(np, "earc")) {
		mtk_snd_dais[DAI_LINK_EARC_PLAYBACK].ignore = 1;
		mtk_snd_dais[DAI_LINK_EARC_NONPCM_PLAYBACK].ignore = 1;
	}

	return 0;
}

static int mtk_parse_capture_capability(struct device_node *node,
	struct device_node *np)
{
	const char *loopback_status = NULL;
	const char *hfp_rx_status = NULL;
	const char *hdmi_rx2_status = NULL;
	const char *i2s_rx2_status = NULL;
	int ret = 0;

	if (!of_property_read_bool(np, "hdmi-rx1"))
		mtk_snd_dais[DAI_LINK_HDMI1_CAPTURE].ignore = 1;

	if (!of_property_read_bool(np, "hdmi-rx2"))
		mtk_snd_dais[DAI_LINK_HDMI2_CAPTURE].ignore = 1;
	else {
		ret = of_property_read_string(np, "hdmi-rx2", &hdmi_rx2_status);

		if (ret == 0) {
			if (!strncmp(hdmi_rx2_status, "disable", strlen("disable")))
				mtk_snd_dais[DAI_LINK_HDMI2_CAPTURE].ignore = 1;
		} else
			pr_err("%s[%d] parser hdmi_rx2 dto error\n", __func__, __LINE__);
	}

	if (!of_property_read_bool(np, "atv"))
		mtk_snd_dais[DAI_LINK_ATV_CAPTURE].ignore = 1;

	if (!of_property_read_bool(np, "adc"))
		mtk_snd_dais[DAI_LINK_ADC_CAPTURE].ignore = 1;
	else
		of_property_read_string(node, "adc_name",
			&mtk_snd_dais[DAI_LINK_ADC_CAPTURE].stream_name);

	if (!of_property_read_bool(np, "adc1"))
		mtk_snd_dais[DAI_LINK_ADC1_CAPTURE].ignore = 1;
	else
		of_property_read_string(node, "adc1_name",
			&mtk_snd_dais[DAI_LINK_ADC1_CAPTURE].stream_name);

	if (!of_property_read_bool(np, "i2s-rx2"))
		mtk_snd_dais[DAI_LINK_I2S_RX2_CAPTURE].ignore = 1;
	else {
		ret = of_property_read_string(np, "i2s-rx2", &i2s_rx2_status);

		if (ret == 0) {
			if (!strncmp(i2s_rx2_status, "disable", strlen("disable")))
				mtk_snd_dais[DAI_LINK_I2S_RX2_CAPTURE].ignore = 1;
		} else
			pr_err("%s[%d] parser i2s_rx2 dto error\n", __func__, __LINE__);
	}

	if (!of_property_read_bool(np, "loopback"))
		mtk_snd_dais[DAI_LINK_LOOPBACK_CAPTURE].ignore = 1;
	else {
		of_property_read_string(node, "loopback_name",
			&mtk_snd_dais[DAI_LINK_LOOPBACK_CAPTURE].stream_name);

		ret = of_property_read_string(np, "loopback", &loopback_status);

		if (ret == 0) {
			if (!strncmp(loopback_status, "disable", strlen("disable")))
				mtk_snd_dais[DAI_LINK_LOOPBACK_CAPTURE].ignore = 1;
		} else
			pr_err("%s[%d] parser loopback dto error\n", __func__, __LINE__);
	}

	if (!of_property_read_bool(np, "hfp"))
		mtk_snd_dais[DAI_LINK_HFP_RX_CAPTURE].ignore = 1;
	else {
		ret = of_property_read_string(np, "hfp", &hfp_rx_status);

		if (ret == 0) {
			if (!strncmp(hfp_rx_status, "disable", strlen("disable")))
				mtk_snd_dais[DAI_LINK_HFP_RX_CAPTURE].ignore = 1;
		} else
			pr_err("%s[%d] parser hfp dto error\n", __func__, __LINE__);
	}

	return 0;
}

static int mtk_parse_capability(struct device_node *node)
{
	struct device_node *np;
	int ret;

	np = of_find_node_by_name(node, "mtk-sound-capability");
	if (!np)
		return -ENODEV;

	ret = mtk_parse_duplex_capability(np);
	if (ret) {
		pr_err("[ALSA EVB]can't get duplex capability\n");
		return -EINVAL;
	}

	ret = mtk_parse_playback_capability(np);
	if (ret) {
		pr_err("[ALSA EVB]can't get playback capability\n");
		return -EINVAL;
	}

	ret = mtk_parse_dplayback_capability(np);
	if (ret) {
		pr_err("[ALSA EVB]can't get dplayback capability\n");
		return -EINVAL;
	}

	ret = mtk_parse_capture_capability(node, np);
	if (ret) {
		pr_err("[ALSA EVB]can't get capture capability\n");
		return -EINVAL;
	}

	return 0;
}

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
static void mtk_set_codec(struct snd_soc_card *card,
			struct device_node *codec_node, const char *name)
{
	const struct snd_soc_dapm_route *routes = card->of_dapm_routes;
	const char *sink = NULL;
	int codec_index = -1;
	int i;

	for (i = 0; i < card->num_of_dapm_routes; i++)
		if (!strncmp(routes[i].source, "SPEAKER_PLAYBACK",
					strlen("SPEAKER_PLAYBACK")))
			sink = routes[i].sink;

	if (sink != NULL) {
		if (!strncmp(sink, "DAC0", strlen("DAC0")))
			codec_index = DAI_LINK_DAC0_BE;
		else if (!strncmp(sink, "DAC1", strlen("DAC1")))
			codec_index = DAI_LINK_DAC1_BE;
		else if (!strncmp(sink, "I2S0", strlen("I2S0")))
			codec_index = DAI_LINK_I2S0_BE;
		else if (!strncmp(sink, "I2S5", strlen("I2S5")))
			codec_index = DAI_LINK_I2S5_BE;
	}

	if (codec_index >= 0) {
		mtk_snd_dais[codec_index].codecs->of_node = codec_node;
		mtk_snd_dais[codec_index].codecs->name = NULL;
		mtk_snd_dais[codec_index].codecs->dai_name = name;
	}
}
#endif

static int mtk_snd_machine_dev_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mtk_snd_card;
	struct device *dev = &pdev->dev;
	struct device_node *node, *codec_node;
	const char *codec_dai_name;
	int ret;

	codec_dai_name = NULL;
	codec_node = NULL;
	node = dev->of_node;
	if (!node)
		return -ENODEV;

	ret = mtk_parse_capability(node);
	if (ret) {
		dev_err(dev, "[ALSA EVB]can't get capability\n");
			return -EINVAL;
	}

	card->dev = &pdev->dev;

	ret = snd_soc_of_parse_audio_routing(card, "audio-routing");
	if (ret) {
		dev_err(dev, "[ALSA EVB]can't parse audio-routing\n");
			return -EINVAL;
	}

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	codec_node = of_parse_phandle(pdev->dev.of_node, "audio-codec", 0);
	if (!codec_node)
		dev_err(&pdev->dev, "[ALSA EVB]can't get audio-codec\n");

	ret = of_property_read_string(node, "codec-dai-name", &codec_dai_name);
	if (ret)
		dev_err(dev, "[ALSA EVB]can't get codec dai\n");

	if (codec_node && codec_dai_name)
		mtk_set_codec(card, codec_node, codec_dai_name);
	else
		dev_err(dev, "[ALSA EVB]no codec, I2S will no sound\n");
#endif

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(dev, "[ALSA EVB]register card fail %d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_snd_machine_dev_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mtk_snd_machine_dt_match[] = {
	{ .compatible = "mediatek,mtk-snd-machine", },
	{ }
};
MODULE_DEVICE_TABLE(of, mtk_snd_machine_dt_match);

static struct platform_driver mtk_snd_machine_driver = {
	.driver = {
		.name = "snd-machine",
		.of_match_table = mtk_snd_machine_dt_match,
#ifdef CONFIG_PM
		.pm = &snd_soc_pm_ops,
#endif
	},
	.probe = mtk_snd_machine_dev_probe,
	.remove = mtk_snd_machine_dev_remove,
};

module_platform_driver(mtk_snd_machine_driver);

MODULE_DESCRIPTION("ALSA SoC machine driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("soc card");
