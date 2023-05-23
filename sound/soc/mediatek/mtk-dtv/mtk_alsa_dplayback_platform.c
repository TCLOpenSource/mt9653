// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * mtk_alsa_dplayback_platform.c  --  Mediatek dplayback function
 *
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <sound/soc.h>

#include "mtk_alsa_chip.h"
#include "mtk_alsa_dplayback_platform.h"
#include "mtk-reserved-memory.h"

#define DPLAYBACK_DMA_NAME	"snd-doutput-pcm-dai"

#define MAX_DIGITAL_BUF_SIZE	(12 * 4 * 2048 * 4)

#define EXPIRATION_TIME	500

#define CH_2 2

#define ALSA_AUDIO_ENTER(dev, id, priv)	\
({do {AUD_DEV_PRINT_DPB(dev, id, AUD_INFO, priv	\
, "[%s:%d] !!!\n", __func__, __LINE__); } while (0); })

static struct dplayback_register REG;

static unsigned int dplayback_pcm_rates[] = {
	32000,
	44100,
	48000,
	176000,
	192000
};

static unsigned int spdif_nonpcm_rates[] = {
	32000,
	44100,
	48000,
};

static unsigned int arc_nonpcm_rates[] = {
	32000,
	44100,
	48000,
	176000,
	192000,
};

static unsigned int earc_nonpcm_rates[] = {
	32000,
	44100,
	48000,
	176000,
	192000,
};

static unsigned int dplayback_channels[] = {
	2, 6, 8,
};

static const struct snd_pcm_hw_constraint_list constraints_dpb_pcm_rates = {
	.count = ARRAY_SIZE(dplayback_pcm_rates),
	.list = dplayback_pcm_rates,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list constraints_spdif_nonpcm_rates = {
	.count = ARRAY_SIZE(spdif_nonpcm_rates),
	.list = spdif_nonpcm_rates,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list constraints_arc_nonpcm_rates = {
	.count = ARRAY_SIZE(arc_nonpcm_rates),
	.list = arc_nonpcm_rates,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list constraints_dpb_channels = {
	.count = ARRAY_SIZE(dplayback_channels),
	.list = dplayback_channels,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list constraints_earc_nonpcm_rates = {
	.count = ARRAY_SIZE(earc_nonpcm_rates),
	.list = earc_nonpcm_rates,
	.mask = 0,
};

/* Default hardware settings of ALSA PCM playback */
static struct snd_pcm_hardware dplayback_default_hw = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE |
				SNDRV_PCM_FMTBIT_S32_BE | SNDRV_PCM_FMTBIT_S24_LE,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = MAX_DIGITAL_BUF_SIZE,
	.period_bytes_min = 64,
	.period_bytes_max = MAX_DIGITAL_BUF_SIZE >> 3,
	.periods_min = 2,
	.periods_max = 8,
	.fifo_size = 0,
};

static int spdif_nonpcm_set_synthesizer(unsigned int sample_rate)
{
	mtk_alsa_write_reg_mask_byte(REG.DPB_SPDIF_CFG, 0x07, 0x02);
	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG+1, 0x10, 0x10);

	switch (sample_rate) {
	case 32000:
		//pr_info("[ALSA DPB]spdif synthesizer rate is 32 KHz\n");
		mtk_alsa_write_reg(REG.DPB_SPDIF_SYNTH_H, 0x34BC);
		mtk_alsa_write_reg(REG.DPB_SPDIF_SYNTH_L, 0x0000);
		break;
	case 44100:
		//pr_info("[ALSA DPB]spdif synthesizer rate is 44.1 KHz\n");
		mtk_alsa_write_reg(REG.DPB_SPDIF_SYNTH_H, 0x2643);
		mtk_alsa_write_reg(REG.DPB_SPDIF_SYNTH_L, 0xEB1A);
		break;
	case 48000:
		//pr_info("[ALSA DPB]spdif synthesizer rate is 48 KHz\n");
		mtk_alsa_write_reg(REG.DPB_SPDIF_SYNTH_H, 0x2328);
		mtk_alsa_write_reg(REG.DPB_SPDIF_SYNTH_L, 0x0000);
		break;
	default:
		pr_err("[ALSA DPB]Err! un-supported rate !!!\n");
		mtk_alsa_write_reg(REG.DPB_SPDIF_SYNTH_H, 0x2328);
		mtk_alsa_write_reg(REG.DPB_SPDIF_SYNTH_L, 0x0000);
		break;
	}
	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG, 0x20, 0x20);
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG, 0x20, 0x00);
	return 0;
}

static int arc_nonpcm_set_synthesizer(unsigned int sample_rate)
{
	mtk_alsa_write_reg_mask_byte(REG.DPB_ARC_CFG, 0x07, 0x02);
	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG+1, 0x20, 0x20);

	switch (sample_rate) {
	case 32000:
		//pr_info("[ALSA DPB]arc synthesizer rate is 32 KHz\n");
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x34BC);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	case 44100:
		//pr_info("[ALSA DPB]arc synthesizer rate is 44.1 KHz\n");
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x2643);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0xEB1A);
		break;
	case 48000:
		//pr_info("[ALSA DPB]arc synthesizer rate is 48 KHz\n");
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x2328);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	case 96000:
		//pr_info("[ALSA DPB]arc synthesizer rate is 96 KHz\n");
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x1194);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;

	case 192000:
		//pr_info("[ALSA DPB]arc synthesizer rate is 192 KHz\n");
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x8CA);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	default:
		pr_err("[ALSA DPB]Err! un-supported rate !!!\n");
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x2328);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	}
	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG, 0x10, 0x10);
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG, 0x10, 0x00);
	return 0;
}

static int earc_nonpcm_set_synthesizer(unsigned int sample_rate)
{
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG10+1, 0x70, 0x20);  //NPCM SYNTH APN page 6
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG10, 0x10, 0x10);  // RIU select

	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG+1, 0x70, 0x20);// RIU select

	switch (sample_rate) {
	case 32000:
		pr_info("[ALSA DPB]earc synthesizer rate is 32 KHz\n");
		mtk_alsa_write_reg(REG.DPB_EARC_CFG12, 0x6978);
		mtk_alsa_write_reg(REG.DPB_EARC_CFG11, 0x0000);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x34BC);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	case 44100:
		pr_info("[ALSA DPB]earc synthesizer rate is 44.1 KHz\n");
		mtk_alsa_write_reg(REG.DPB_EARC_CFG12, 0x4C87);
		mtk_alsa_write_reg(REG.DPB_EARC_CFG11, 0xD634);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x2643);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0xEB1A);
		break;
	case 48000:
		pr_info("[ALSA DPB]earc synthesizer rate is 48 KHz\n");
		mtk_alsa_write_reg(REG.DPB_EARC_CFG12, 0x4650);
		mtk_alsa_write_reg(REG.DPB_EARC_CFG11, 0x0000);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x2328);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	case 96000:
		pr_info("[ALSA DPB]earc synthesizer rate is 96 KHz\n");
		mtk_alsa_write_reg(REG.DPB_EARC_CFG12, 0x2328);
		mtk_alsa_write_reg(REG.DPB_EARC_CFG11, 0x0000);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x1194);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	case 192000:
		pr_info("[ALSA DPB]earc synthesizer rate is 192 KHz\n");
		mtk_alsa_write_reg(REG.DPB_EARC_CFG12, 0x1194);
		mtk_alsa_write_reg(REG.DPB_EARC_CFG11, 0x0000);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x8CA);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	default:
		pr_err("[ALSA DPB]Err! un-supported rate !!!\n");
		mtk_alsa_write_reg(REG.DPB_EARC_CFG12, 0x4650);
		mtk_alsa_write_reg(REG.DPB_EARC_CFG11, 0x0000);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_H, 0x2328);
		mtk_alsa_write_reg(REG.DPB_ARC_SYNTH_L, 0x0000);
		break;
	}
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG10, 0x20, 0x20);//update bit toggle
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG10, 0x20, 0x00);


	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG, 0x10, 0x10); //update bit 0x112bce [4]
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.DPB_SYNTH_CFG, 0x10, 0x00); //update bit 0x112bce [4]
	return 0;
}


static int earc_atop_aupll(int rate, int channels)
{
	int error = 0;

	switch (rate) {
	case 0:
		error = 1;
		break;
	case 32000:
	case 44100:
	case 48000:
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX2C, 0x03, 0x01);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX2C+1, 0xFF, 0x28);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX30+1, 0x03, 0x00);
		if (channels == 2)
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX20+1, 0x70, 0x50);
		else if (channels == 8)
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX20+1, 0x70, 0x30);
		else
			error = 1;
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX28, 0x70, 0x30);
		break;
	case 88000:
	case 96000:
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX2C, 0x03, 0x01);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX2C+1, 0xFF, 0x14);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX30+1, 0x03, 0x00);
		if (channels == 2)
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX20+1, 0x70, 0x40);
		else if (channels == 8)
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX20+1, 0x70, 0x20);
		else
			error = 1;
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX28, 0x70, 0x30);
		break;
	case 176000:
	case 192000:
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX2C, 0x03, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX2C+1, 0xFF, 0x14);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX30+1, 0x03, 0x01);
		if (channels == 2)
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX20+1, 0x70, 0x30);
		else if (channels == 8)
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX20+1, 0x70, 0x10);
		else
			error = 1;
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX28, 0x70, 0x30);
		break;
	default:
		pr_err("[ALSA DPB]Err! un-supported earc rate or channel !!!\n");
		error = 1;
		break;
	}

	if (error) {
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX2C, 0x03, 0x01);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX2C+1, 0xFF, 0x04);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX30+1, 0x03, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX20+1, 0x70, 0x30);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX28, 0x70, 0x20);
	}
	return error;
}

static int earc_channel_status(struct mtk_pb_runtime_struct *dev_runtime,
						int id, struct mtk_channel_status_struct *cs)
{
	int datarate;

	/* set channel status */
	if (cs->status) {
		//pr_info("[ALSA DPB]arc cs info use kcontrol\n");
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG27, cs->cs_bytes[0]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG27+1, cs->cs_bytes[1]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG26, cs->cs_bytes[2]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG26+1, cs->cs_bytes[3]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG25, cs->cs_bytes[4]);
	} else if (id == EARC_NONPCM_PLAYBACK) {
		//[5:3][1:0]unencrypted IEC61937
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG27, 0x3F, 0x02);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG27+1, 0x80, 0x80);
		//Table 9-27 ICE61937 Compressed Audio Layout A
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG25+1, 0xF0, 0x00);

		datarate = (dev_runtime->channel_mode / 2) * dev_runtime->sample_rate;
		switch (datarate) {
		case 32000:
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x03);
			break;
		case 44100:
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x00);
			break;
		case 48000:
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x02);
			break;
		case 176000:
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x0C);
			break;
		case 192000:
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x0E);
			break;
		case 768000:
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x09);
			break;
		default:
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x02);
			pr_err("[ALSA DPB] wrong nonpcm data rate\n");
			break;
		}
		//[35:32] 16bits length
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG25, 0x0F, 0x02);
		//[143:136] speaker map: NA
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG19+1, 0xFF, 0x00);
	} else {
		//PCM case
		//pr_info("[ALSA DPB]earc cs info use hw param\n");

		if (dev_runtime->channel_mode == 2) {
			//[5:3][1:0]unencrypted 2-ch LPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG27, 0x3F, 0x00);
			//[15:12]2-channel layout
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG25+1, 0xF0, 0x00);
			//sample rate
			switch (dev_runtime->sample_rate) {
			case 48000:
				//[11:8]48000Hz
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x02);
				break;
			case 44100:
				//[11:8]44100Hz
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x00);
				break;
			case 32000:
				//[11:8]32000Hz
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x03);
				break;
			case 88000:
				//[11:8]88200Hz
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x08);
				break;
			case 96000:
				//[11:8]96000Hz
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x0A);
				break;
			case 176000:
				//[11:8]176000Hz
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x0C);
				break;
			case 192000:
				//[11:8]192000Hz
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x0E);
				break;
			default:
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x01);
				break;
			}
		} else if (dev_runtime->channel_mode == 8) {
			//[5:3][1:0]unencrypted 8-ch LPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG27, 0x3F, 0x20);
			//[15:12]8-channel layout
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG25+1, 0xF0, 0x70);
			//sample rate
			switch (dev_runtime->sample_rate) {
			case 48000:
				//[11:8]48000Hz*4
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x0E);
				break;
			case 44100:
				//[11:8]44100Hz*4
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x0C);
				break;
			case 32000:
				//[11:8]32000Hz*4
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x8B);
				break;
			case 88200:
				//[11:8]88200Hz*4
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x0D);
				break;
			case 96000:
				//[11:8]96000Hz*4
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x05);
				break;
			case 176000:
				//[11:8]176400Hz*4
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x8D);
				break;
			case 192000:
				//[11:8]192000Hz*4
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x09);
				break;
			default:
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG26+1, 0x0F, 0x01);
				break;
			}
		}
		//mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG27+1, 0x80, 0x80);//sync with utopia

		//[35:32] 16bits length
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG25, 0x0F, 0x02);
		if (dev_runtime->channel_mode == 8) {
			//[15:8]FR/FL/LFE1/FC/LS/RS/RLC/RRC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG19+1, 0xFF, 0x13);
		} else if (dev_runtime->channel_mode == 6) {
			//[15:8]FR/FL/LFE1/FC/LS/RS/RLC/RRC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG19+1, 0xFF, 0x0B);
		} else {
			//[15:8]FR/FL/LFE1/FC/LS/RS/RLC/RRC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG19+1, 0xFF, 0);
		}

	}
	return 0;
}

static int earc_nonpcm_192K_8ch(struct dplayback_priv *priv)
{
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[EARC_NONPCM_PLAYBACK];
	struct mtk_channel_status_struct *cs = &dev_runtime->channel_status;
	int synth_spdif_npcm2_256fs_out_ck = 276;  // this is define in 5870 clk table enum
	int ret;

	ret = mtk_alsa_mux_clock_setting(synth_spdif_npcm2_256fs_out_ck, 2);  //102874 bit 0
	if (ret)
		pr_err("[AUDIO][%s]earc npcm 192k 8ch mux clock setting error!\n", __func__);

	earc_atop_aupll(192000, 8);
	earc_nonpcm_set_synthesizer(192000);

	dev_runtime->channel_mode = 8;
	dev_runtime->sample_rate = 192000;
	cs->status = 0;

	earc_channel_status(dev_runtime, EARC_NONPCM_PLAYBACK, cs);


	//0x951136  //pcm channel status (this is pcm if is nonpcm 0x2)
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG27, 0x3F, 0x00);
	//update bit
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG29+1, 0x01, 0x01);
	// enable differential
	//mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00+1, 0xff, 0x44);
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00, 0xff, 0x08);
	return ret;
}

static int earc_nonpcm_32K_2ch(struct dplayback_priv *priv)
{

	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[EARC_NONPCM_PLAYBACK];
	struct mtk_channel_status_struct *cs = &dev_runtime->channel_status;
	int synth_spdif_npcm2_256fs_out_ck = 276;  // this is define in 5870 clk table enum
	int ret;

	ret = mtk_alsa_mux_clock_setting(synth_spdif_npcm2_256fs_out_ck, 2);
	if (ret)
		pr_err("[AUDIO][%s]earc npcm 32k 2ch mux clock setting error!\n", __func__);

	earc_atop_aupll(32000, 2);
	earc_nonpcm_set_synthesizer(32000);

	dev_runtime->channel_mode = 2;
	dev_runtime->sample_rate = 32000;
	cs->status = 0;

	earc_channel_status(dev_runtime, EARC_NONPCM_PLAYBACK, cs);

	//0x951136  //pcm channel status
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG27, 0x3F, 0x00);
	//update bit
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG29+1, 0x01, 0x01);
	// enable differential
	//mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00+1, 0xff, 0x44);
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00, 0xff, 0x08);
	return ret;
}

//[Sysfs] HELP command
static ssize_t help_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dplayback_priv *priv;  //need to modify

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
		AUD_DEV_PRINT_DPB(dev, DPLAYBACK_NUM, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, DPLAYBACK_NUM, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 350, "Debug Help:\n"
		"- spdif_pcm_log_level / spdif_nonpcm_log_level / arc_pcm_log_level / arc_nonpcm_log_level\n"
		"	<W>: Set debug log level.\n"
		"	(0:emerg, 1:alert, 2:critical, 3:error, 4:warn, 5:notice, 6:info, 7:debug)\n"
		"	<R>: Read debug log level.\n");
}


static ssize_t spdif_pcm_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct dplayback_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][COPR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_PCM_PLAYBACK, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_PCM_PLAYBACK, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[SPDIF_PCM_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t spdif_pcm_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct dplayback_priv *priv;

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
		AUD_DEV_PRINT_DPB(dev, SPDIF_PCM_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_PCM_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 25, "spdif_pcm_log_level:%d\n",
				  priv->playback_dev[SPDIF_PCM_PLAYBACK]->log_level);
}

static ssize_t spdif_nonpcm_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct dplayback_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][COPR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_NONPCM_PLAYBACK, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_NONPCM_PLAYBACK, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t spdif_nonpcm_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct dplayback_priv *priv;

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
		AUD_DEV_PRINT_DPB(dev, SPDIF_NONPCM_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_NONPCM_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 35, "spdif_nonpcm_log_level:%d\n",
				  priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->log_level);
}

static ssize_t arc_pcm_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct dplayback_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][COPR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_DPB(dev, ARC_PCM_PLAYBACK, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, ARC_PCM_PLAYBACK, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[ARC_PCM_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t arc_pcm_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct dplayback_priv *priv;

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
		AUD_DEV_PRINT_DPB(dev, ARC_PCM_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, ARC_PCM_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 35, "arc_pcm_pcm_log_level:%d\n",
				  priv->playback_dev[ARC_PCM_PLAYBACK]->log_level);
}

static ssize_t arc_nonpcm_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct dplayback_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][COPR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_DPB(dev, ARC_NONPCM_PLAYBACK, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, ARC_NONPCM_PLAYBACK, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[ARC_NONPCM_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t arc_nonpcm_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct dplayback_priv *priv;

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
		AUD_DEV_PRINT_DPB(dev, ARC_NONPCM_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, ARC_NONPCM_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 35, "arc_nonpcm_log_level:%d\n",
				  priv->playback_dev[ARC_NONPCM_PLAYBACK]->log_level);
}


static ssize_t earc_pcm_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct dplayback_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][COPR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_PCM_PLAYBACK, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_PCM_PLAYBACK, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[EARC_PCM_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t earc_pcm_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct dplayback_priv *priv;

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
		AUD_DEV_PRINT_DPB(dev, EARC_PCM_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_PCM_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 30, "earc_pcm_log_level:%d\n",
				  priv->playback_dev[EARC_PCM_PLAYBACK]->log_level);
}

static ssize_t earc_nonpcm_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct dplayback_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][COPR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_NONPCM_PLAYBACK, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_NONPCM_PLAYBACK, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[EARC_NONPCM_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t earc_nonpcm_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct dplayback_priv *priv;

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
		AUD_DEV_PRINT_DPB(dev, EARC_NONPCM_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_NONPCM_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 35, "earc_nonpcm_log_level:%d\n",
				  priv->playback_dev[EARC_NONPCM_PLAYBACK]->log_level);
}

static ssize_t earc_nonpcm_synth_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct dplayback_priv *priv;

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
		AUD_DEV_PRINT_DPB(dev, EARC_NONPCM_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_NONPCM_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 35, "earc nonpcm synth rate is :%d\n",
				  priv->playback_dev[EARC_NONPCM_PLAYBACK]->sample_rate);
}

static ssize_t earc_nonpcm_synth_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int sample_rate;
	struct dplayback_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][DPB][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_NONPCM_PLAYBACK, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, EARC_NONPCM_PLAYBACK, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &sample_rate);
	if (retval == 0) {
		if ((sample_rate > 192000) || (sample_rate < 32000))
			retval = -EINVAL;
		else
			priv->playback_dev[EARC_NONPCM_PLAYBACK]->sample_rate = sample_rate;
	}
	//synth setting
	//earc_nonpcm_set_synthesizer(sample_rate);

	//32K 2ch script
	if (sample_rate == 32000)
		earc_nonpcm_32K_2ch(priv);

	//192k 8ch script
	if (sample_rate == 192000)
		earc_nonpcm_192K_8ch(priv);
	return (retval < 0) ? retval : count;
}

static ssize_t digital_out_cs_set_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{

	struct dplayback_priv *priv;

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
		AUD_DEV_PRINT_DPB(dev, SPDIF_PCM_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_PCM_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 1000, "spdif pcm channel status :%d , cs_bytes[0] %d\n"
			"spdif non channel status :%d , cs_bytes[0] %d\n"
			"arc pcm channel status :%d , cs_bytes[0] %d\n"
			"arc non pcm channel status :%d , cs_bytes[0] %d\n"
			"earc pcm channel status :%d , cs_bytes[0] %d\n"
			"earc non pcm channel status :%d , cs_bytes[0] %d\n"
			, priv->playback_dev[SPDIF_PCM_PLAYBACK]->channel_status.status
			, priv->playback_dev[SPDIF_PCM_PLAYBACK]->channel_status.cs_bytes[0]
			, priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->channel_status.status
			, priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->channel_status.cs_bytes[0]
			, priv->playback_dev[ARC_PCM_PLAYBACK]->channel_status.status
			, priv->playback_dev[ARC_PCM_PLAYBACK]->channel_status.cs_bytes[0]
			, priv->playback_dev[ARC_NONPCM_PLAYBACK]->channel_status.status
			, priv->playback_dev[ARC_NONPCM_PLAYBACK]->channel_status.cs_bytes[0]
			, priv->playback_dev[EARC_PCM_PLAYBACK]->channel_status.status
			, priv->playback_dev[EARC_PCM_PLAYBACK]->channel_status.cs_bytes[0]
			, priv->playback_dev[EARC_NONPCM_PLAYBACK]->channel_status.status
			, priv->playback_dev[EARC_NONPCM_PLAYBACK]->channel_status.cs_bytes[0]);
}



/*commad echo 1 32 >*/
/*sys/devices/platform/mtk-sound-dplayback-platform/mtk_dbg/digital_out_cs_set */
static ssize_t digital_out_cs_set_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{

	ssize_t retval;
	int en_ch_status;
	int ch_status;
	struct dplayback_priv *priv;
	char *str1 = NULL;	char *str2 = NULL;

	if (dev == NULL) {
		pr_err("[AUDIO][DPB][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_PCM_PLAYBACK, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_DPB(dev, SPDIF_PCM_PLAYBACK, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	str1 = (char *)buf;
	str2 = strsep(&str1, " ");

	retval = kstrtoint(str2, 10, &en_ch_status);

	if (retval == 0) {
		if ((en_ch_status > 0)) {
			priv->playback_dev[SPDIF_PCM_PLAYBACK]->channel_status.status
			= en_ch_status;
			priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->channel_status.status
			= en_ch_status;
			priv->playback_dev[ARC_PCM_PLAYBACK]->channel_status.status
			= en_ch_status;
			priv->playback_dev[ARC_NONPCM_PLAYBACK]->channel_status.status
			= en_ch_status;
			priv->playback_dev[EARC_PCM_PLAYBACK]->channel_status.status
			= en_ch_status;
			priv->playback_dev[EARC_NONPCM_PLAYBACK]->channel_status.status
			= en_ch_status;
		}
	}

	retval = kstrtoint(str1, 10, &ch_status);
	if (retval == 0) {
		priv->playback_dev[SPDIF_PCM_PLAYBACK]->channel_status.cs_bytes[0]
		= ch_status;
		priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->channel_status.cs_bytes[0]
		= ch_status;
		priv->playback_dev[ARC_PCM_PLAYBACK]->channel_status.cs_bytes[0]
		= ch_status;
		priv->playback_dev[ARC_NONPCM_PLAYBACK]->channel_status.cs_bytes[0]
		= ch_status;
		priv->playback_dev[EARC_PCM_PLAYBACK]->channel_status.cs_bytes[0]
		= ch_status;
		priv->playback_dev[EARC_NONPCM_PLAYBACK]->channel_status.cs_bytes[0]
		= ch_status;
	}
	return (retval < 0) ? retval : count;

}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RW(spdif_pcm_log_level);
static DEVICE_ATTR_RW(spdif_nonpcm_log_level);
static DEVICE_ATTR_RW(arc_pcm_log_level);
static DEVICE_ATTR_RW(arc_nonpcm_log_level);
static DEVICE_ATTR_RW(earc_pcm_log_level);
static DEVICE_ATTR_RW(earc_nonpcm_log_level);
static DEVICE_ATTR_RW(earc_nonpcm_synth);
static DEVICE_ATTR_RW(digital_out_cs_set);

static struct attribute *dplayback_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_spdif_pcm_log_level.attr,
	&dev_attr_spdif_nonpcm_log_level.attr,
	&dev_attr_arc_pcm_log_level.attr,
	&dev_attr_arc_nonpcm_log_level.attr,
	&dev_attr_earc_pcm_log_level.attr,
	&dev_attr_earc_nonpcm_log_level.attr,
	&dev_attr_earc_nonpcm_synth.attr,
	&dev_attr_digital_out_cs_set.attr,
	NULL,
};

static const struct attribute_group dplayback_attr_group = {
	.name = "mtk_dbg",
	.attrs = dplayback_attrs,
};


static const struct attribute_group *dplayback_attr_groups[] = {
	&dplayback_attr_group,
	NULL,
};


static int dplayback_probe(struct snd_soc_component *component)
{
	struct dplayback_priv *priv;
	struct device *dev;
	int ret;

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

	/*Set default log level*/
	priv->playback_dev[SPDIF_PCM_PLAYBACK]->log_level = AUD_EMERG;
	priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->log_level = AUD_EMERG;
	priv->playback_dev[ARC_PCM_PLAYBACK]->log_level = AUD_EMERG;
	priv->playback_dev[ARC_NONPCM_PLAYBACK]->log_level = AUD_EMERG;
	priv->playback_dev[EARC_PCM_PLAYBACK]->log_level = AUD_EMERG;
	priv->playback_dev[EARC_NONPCM_PLAYBACK]->log_level = AUD_EMERG;

	/* Create device attribute files */
	ret = sysfs_create_groups(&dev->kobj, dplayback_attr_groups);
	if (ret)
		pr_err("[AUDIO][ERROR][%s]sysfs_create_groups can't fail!!!\n", __func__);

	return 0;
}

static void dplayback_remove(struct snd_soc_component *component)
{
	struct dplayback_priv *priv;
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

	if (dev != NULL)
		sysfs_remove_groups(&dev->kobj, dplayback_attr_groups);
}

static void timer_open(struct timer_list *timer,
					unsigned long data, void *func)
{
	if (timer == NULL) {
		pr_err("[ALSA DPB]spdif timer pointer is invalid.\n");
		return;
	} else if (func == NULL) {
		pr_err("[ALSA DPB]spdif timer callback function is invalid.\n");
		return;
	}

	timer_setup(timer, func, 0);

	timer->expires = 0;
}

static void timer_close(struct timer_list *timer)
{
	if (timer == NULL) {
		pr_err("[ALSA DPB]spdif timer pointer is invalid.\n");
		return;
	}

	del_timer_sync(timer);
	memset(timer, 0x00, sizeof(struct timer_list));
}

static int timer_reset(struct timer_list *timer)
{
	if (timer == NULL) {
		pr_err("[ALSA DPB]spdif timer pointer is invalid.\n");
		return -EINVAL;
	} else if (timer->function == NULL) {
		pr_err("[ALSA DPB]spdif timer callback function is invalid.\n");
		return -EINVAL;
	}

	mod_timer(timer, (jiffies + msecs_to_jiffies(20)));

	return 0;
}

static int timer_update(struct timer_list *timer,
				unsigned long time_interval)
{
	if (timer == NULL) {
		pr_err("[ALSA DPB]spdif timer pointer is invalid.\n");
		return -EINVAL;
	} else if (timer->function == NULL) {
		pr_err("[ALSA DPB]spdif timer callback function is invalid.\n");
		return -EINVAL;
	}

	mod_timer(timer, (jiffies + time_interval));

	return 0;
}

static void dplayback_monitor(struct timer_list *t)
{
	struct mtk_pb_runtime_struct *dev_runtime = NULL;
	struct mtk_monitor_struct *monitor = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_pcm_runtime *runtime = NULL;
	char time_interval = 1;
	unsigned int expiration_counter = EXPIRATION_TIME / time_interval;
	struct dplayback_priv *priv;
	int id;

	dev_runtime = from_timer(dev_runtime, t, timer);

	if (dev_runtime == NULL) {
		pr_err("[ALSA DPB]spdif invalid parameters!\n");
		return;
	}

	monitor = &dev_runtime->monitor;
	priv = (struct dplayback_priv *) dev_runtime->private;
	id = dev_runtime->id;

	if (monitor->monitor_status == CMD_STOP) {
		AUD_PR_PRINT_DPB(id, AUD_INFO, priv,
			"[ALSA DPB]dplayback No action is required, exit Monitor\n");
		spin_lock(&dev_runtime->spin_lock);
		memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
		spin_unlock(&dev_runtime->spin_lock);
		return;
	}

	substream = dev_runtime->substreams.p_substream;
	if (substream != NULL) {
		snd_pcm_period_elapsed(substream);

		runtime = substream->runtime;
		if (runtime != NULL) {
			/* If nothing to do, increase "expiration_counter" */
			if ((monitor->last_appl_ptr == runtime->control->appl_ptr) &&
				(monitor->last_hw_ptr == runtime->status->hw_ptr)) {
				monitor->expiration_counter++;

				if (monitor->expiration_counter >= expiration_counter) {
					AUD_PR_PRINT_DPB(id, AUD_INFO, priv,
						"[ALSA DPB]dplayback exit Monitor\n");
					snd_pcm_period_elapsed(substream);
					spin_lock(&dev_runtime->spin_lock);
					memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
					spin_unlock(&dev_runtime->spin_lock);
					return;
				}
			} else {
				monitor->last_appl_ptr = runtime->control->appl_ptr;
				monitor->last_hw_ptr = runtime->status->hw_ptr;
				monitor->expiration_counter = 0;
			}
		}
	}

	if (timer_update(&dev_runtime->timer,
				msecs_to_jiffies(time_interval)) != 0) {
		AUD_PR_PRINT_DPB(id, AUD_ERR, priv,
			"[ALSA DPB]spdif fail to update timer for Monitor!\n");
		spin_lock(&dev_runtime->spin_lock);
		memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
		spin_unlock(&dev_runtime->spin_lock);
	}
}

static void dma_register_get_reg(int id, u32 *ctrl, u32 *wptr, u32 *level)
{
	switch (id)  {
	case SPDIF_PCM_PLAYBACK:
	case SPDIF_NONPCM_PLAYBACK:
		*ctrl = REG.DPB_DMA5_CTRL;
		*wptr = REG.DPB_DMA5_WPTR;
		*level = REG.DPB_DMA5_DDR_LEVEL;
		break;
	case ARC_PCM_PLAYBACK:
	case ARC_NONPCM_PLAYBACK:
		*ctrl = REG.DPB_DMA6_CTRL;
		*wptr = REG.DPB_DMA6_WPTR;
		*level = REG.DPB_DMA6_DDR_LEVEL;
		break;
	case EARC_PCM_PLAYBACK:
	case EARC_NONPCM_PLAYBACK:
		*ctrl = REG.DPB_DMA7_CTRL;
		*wptr = REG.DPB_DMA7_WPTR;
		*level = REG.DPB_DMA7_DDR_LEVEL;
		break;
	default:
		break;
	}
}

static int dma_reader_init(struct dplayback_priv *priv,
			struct mtk_pb_runtime_struct *dev_runtime,
			struct snd_pcm_runtime *runtime,
			unsigned int dmaRdr_offset, int id)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	ptrdiff_t dmaRdr_base_pa = 0;
	ptrdiff_t dmaRdr_base_ba = 0;
	ptrdiff_t dmaRdr_base_va = pcm_playback->str_mode_info.virtual_addr;
	//pr_info("Initiate MTK PCM Playback engine\n");

	if ((pcm_playback->initialized_status != 1) ||
		(pcm_playback->status != CMD_RESUME)) {
		dmaRdr_base_pa = priv->mem_info.bus_addr -
				priv->mem_info.memory_bus_base + dmaRdr_offset;
		dmaRdr_base_ba = priv->mem_info.bus_addr + dmaRdr_offset;

		if ((dmaRdr_base_ba % 0x1000)) {
			pr_err("[ALSA DPB]spdif invalid addr,should align 4KB\n");
			return -EFAULT;
		}

		/* convert Bus Address to Virtual Address */
		if (dmaRdr_base_va == 0) {
			dmaRdr_base_va = (ptrdiff_t)ioremap_wc(dmaRdr_base_ba,
						pcm_playback->buffer.size);
			if (dmaRdr_base_va == 0) {
				pr_err("[ALSA DPB]spdif fail to convert addr\n");
				return -ENOMEM;
			}
		}

		pcm_playback->str_mode_info.physical_addr = dmaRdr_base_pa;
		pcm_playback->str_mode_info.bus_addr = dmaRdr_base_ba;
		pcm_playback->str_mode_info.virtual_addr = dmaRdr_base_va;

		pcm_playback->initialized_status = 1;
	} else {
		dmaRdr_base_pa = pcm_playback->str_mode_info.physical_addr;
		dmaRdr_base_ba = pcm_playback->str_mode_info.bus_addr;
		dmaRdr_base_va = pcm_playback->str_mode_info.virtual_addr;
	}

	/* init PCM playback buffer address */
	pcm_playback->buffer.addr = (unsigned char *)dmaRdr_base_va;
	pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;

	runtime->dma_area =  (unsigned char *)dmaRdr_base_va;
	runtime->dma_addr = dmaRdr_base_ba;
	runtime->dma_bytes =  pcm_playback->buffer.size;

	/* clear PCM playback dma buffer */
	memset((void *)runtime->dma_area, 0x00,
					runtime->dma_bytes);

	/* reset written size */
	pcm_playback->written_size = 0;

	/* clear PCM playback pcm buffer */
	memset((void *)pcm_playback->buffer.addr, 0x00,
					pcm_playback->buffer.size);
	smp_mb();

	return 0;
}

static int dma_reader_exit(int id, struct mtk_pb_runtime_struct *dev_runtime)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	struct mtk_channel_status_struct *cs = &dev_runtime->channel_status;
	ptrdiff_t dmaRdr_base_va = pcm_playback->str_mode_info.virtual_addr;
	//pr_info("Exit MTK PCM Playback engine\n");

	cs->status = 0;

	switch (id) {
	case SPDIF_PCM_PLAYBACK:
	case SPDIF_NONPCM_PLAYBACK:
		/* reset PCM playback engine */
		mtk_alsa_write_reg(REG.DPB_DMA5_CTRL, 0x0001);
		/* clear PCM playback write pointer */
		mtk_alsa_write_reg(REG.DPB_DMA5_WPTR, 0x0000);
		break;
	case ARC_PCM_PLAYBACK:
	case ARC_NONPCM_PLAYBACK:
		/* reset PCM playback engine */
		mtk_alsa_write_reg(REG.DPB_DMA6_CTRL, 0x0001);
		/* clear PCM playback write pointer */
		mtk_alsa_write_reg(REG.DPB_DMA6_WPTR, 0x0000);
		break;
	case EARC_PCM_PLAYBACK:
	case EARC_NONPCM_PLAYBACK:
		/* reset PCM playback engine */
		mtk_alsa_write_reg(REG.DPB_DMA7_CTRL, 0x0001);
		/* clear PCM playback write pointer */
		mtk_alsa_write_reg(REG.DPB_DMA7_WPTR, 0x0000);
		break;
	default:
		pr_err("[ALSA DPB] set wrong device exit\n");
		break;
	}

	if (dmaRdr_base_va != 0) {
		if (pcm_playback->buffer.addr) {
			iounmap((void *)pcm_playback->buffer.addr);
			pcm_playback->buffer.addr = 0;
		} else
			pr_err("[ALSA DPB]digital buf addr should not be 0\n");

		pcm_playback->str_mode_info.virtual_addr = 0;
	}

	pcm_playback->status = CMD_STOP;
	pcm_playback->initialized_status = 0;

	return 0;
}

static int spdif_dma_start(int id, struct mtk_pb_runtime_struct *dev_runtime)
{
	struct mtk_channel_status_struct *cs = &dev_runtime->channel_status;

	if (id == SPDIF_PCM_PLAYBACK)
		mtk_alsa_write_reg_mask_byte(REG.DPB_SPDIF_CFG+1, 0x80, 0x00);
	else
		mtk_alsa_write_reg_mask_byte(REG.DPB_SPDIF_CFG+1, 0x80, 0x80);

	/* set channel status */
	if (cs->status) {
		//pr_info("[ALSA DPB]spdif cs info use kcontrol\n");
		mtk_alsa_write_reg_byte(REG.DPB_CS0, cs->cs_bytes[0]);
		mtk_alsa_write_reg_byte(REG.DPB_CS1, cs->cs_bytes[1]);
		mtk_alsa_write_reg_byte(REG.DPB_CS2, cs->cs_bytes[2]);
		mtk_alsa_write_reg_byte(REG.DPB_CS3, cs->cs_bytes[3]);
		mtk_alsa_write_reg_byte(REG.DPB_CS4, cs->cs_bytes[4]);
	} else if (id == SPDIF_NONPCM_PLAYBACK) {
		//pr_info("[ALSA DPB]spdif cs info use hw param\n");
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS0, 0x60, 0x40);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS1, 0x01, 0x01);
		switch (dev_runtime->sample_rate) {
		case 32000:
			//pr_info("[ALSA DPB]spdif cs sample rate is 32 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0xC0);
			break;
		case 44100:
			//pr_info("[ALSA DPB]spdif cs sample rate is 44.1 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x00);
			break;
		case 48000:
			//pr_info("[ALSA DPB]spdif cs sample rate is 48 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x40);
			break;
		default:
			pr_err("[ALSA DPB]Err! un-supported spdif rate !!!\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x40);
			break;
		}
	} else {
		//pr_info("[ALSA DPB]spdif cs info use hw param\n");
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS0, 0x60, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS1, 0x01, 0x01);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x40);
	}

	mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0xA0, 0xA0);
	mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x40, 0x00);
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x40, 0x40);
	mtk_alsa_delaytask(5);
	mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x40, 0x00);

	/* start dplayback playback engine */
	mtk_alsa_write_reg_mask(REG.DPB_DMA5_CTRL, 0x0003, 0x0002);

	/* trigger DSP to reset DMA engine */
	mtk_alsa_write_reg_mask(REG.DPB_DMA5_CTRL, 0x0001, 0x0001);
	mtk_alsa_delaytask_us(50);
	mtk_alsa_write_reg_mask(REG.DPB_DMA5_CTRL, 0x0001, 0x0000);

	return 0;
}

void hdmi_arc_pin_control(int status)
{

	if (status == CMD_START) {
		//earc_tx_00[8]:reg_dati_arc_oven
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00+1, 0x01, 0x00);
	} else {
		//earc_tx_00[9]:reg_dati_arc
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00+1, 0x02, 0x00);
		//earc_tx_00[8]:reg_dati_arc_oven
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00+1, 0x01, 0x01);
	}

	pr_info("HDMI ARC PIN is %s\r\n", status?"Enable":"Disable");
}

static int arc_dma_start(int id, struct mtk_pb_runtime_struct *dev_runtime, int ip_version)
{
	struct mtk_channel_status_struct *cs = &dev_runtime->channel_status;

	if (id == ARC_PCM_PLAYBACK)
		mtk_alsa_write_reg_mask_byte(REG.DPB_ARC_CFG+1, 0x80, 0x00);
	else
		mtk_alsa_write_reg_mask_byte(REG.DPB_ARC_CFG+1, 0x80, 0x80);

	/* set channel status */
	if (cs->status) {
		//pr_info("[ALSA DPB]arc cs info use kcontrol\n");
		mtk_alsa_write_reg_byte(REG.DPB_CS0, cs->cs_bytes[0]);
		mtk_alsa_write_reg_byte(REG.DPB_CS1, cs->cs_bytes[1]);
		mtk_alsa_write_reg_byte(REG.DPB_CS2, cs->cs_bytes[2]);
		mtk_alsa_write_reg_byte(REG.DPB_CS3, cs->cs_bytes[3]);
		mtk_alsa_write_reg_byte(REG.DPB_CS4, cs->cs_bytes[4]);
	} else if (id == ARC_NONPCM_PLAYBACK) {
		//pr_info("[ALSA DPB]arc cs info use hw param\n");
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS0, 0x60, 0x40);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS1, 0x01, 0x01);
		switch (dev_runtime->sample_rate) {
		case 32000:
			//pr_info("[ALSA DPB]arc cs sample rate is 32 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0xC0);
			break;
		case 44100:
			//pr_info("[ALSA DPB]arc cs sample rate is 44.1 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x00);
			break;
		case 48000:
			//pr_info("[ALSA DPB]arc cs sample rate is 48 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x40);
			break;
		case 88200:
			//pr_info("[ALSA DPB]arc cs sample rate is 88.2 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x10);
			break;
		case 96000:
			//pr_info("[ALSA DPB]arc cs sample rate is 96 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x50);
			break;
		case 176400:
			//pr_info("[ALSA DPB]arc cs sample rate is 176 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x30);
			break;
		case 192000:
			//pr_info("[ALSA DPB]arc cs sample rate is 192 KHz\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x70);
			break;
		default:
			pr_err("[ALSA DPB]Err! un-supported arc rate !!!\n");
			mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x40);
			break;
		}
	} else {
		//pr_info("[ALSA DPB]arc cs info use hw param\n");
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS0, 0x60, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS1, 0x01, 0x01);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x40);
	}

	mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE, 0xA0, 0xA0);
	mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE, 0x10, 0x00);
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE, 0x10, 0x10);
	mtk_alsa_delaytask(5);
	mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE, 0x10, 0x00);

	if (ip_version == 0x1) {
		//[15]REG_SWRSTZ_64XCHXFS
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x80, 0x80);
		//[14]REG_SWRSTZ_32XCHXFS
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x40, 0x40);
		//[13]REG_SWRSTZ_32XFS
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x20, 0x20);
		//[12]REG_SWRSTZ_SOURCE
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x10, 0x10);
		mtk_alsa_delaytask(1);
		//[09]REG_IEC60958_BIPHASEMARK_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x02, 0x02);
		//[08]REG_PARITY_BIT_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x01, 0x01);
		//[07]REG_ECC_ENCODE_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x80, 0x80);
		//[06]REG_MULTI2DOUBLE_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x40, 0x40);
		//[11]REG_RST_AFIFO_RIU
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x08);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x00);
		//[04]REG_VALID_BIT_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x10, 0x10);
		//[03]REG_USER_STATUS_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x08, 0x08);
		//[02]REG_USER_STATUS_CRC_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x04, 0x04);
		//[01]REG_CHANNEL_STATUS_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x02, 0x02);
		//[05]REG_MIXER_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x20, 0x20);
		//[00]REG_MIXER_SOURCE_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x01, 0x01);
		mtk_alsa_delaytask(1);
	}
	/* start dplayback playback engine */
	mtk_alsa_write_reg_mask(REG.DPB_DMA6_CTRL, 0x0003, 0x0002);

	/* trigger DSP to reset DMA engine */
	mtk_alsa_write_reg_mask(REG.DPB_DMA6_CTRL, 0x0001, 0x0001);
	mtk_alsa_delaytask_us(50);
	mtk_alsa_write_reg_mask(REG.DPB_DMA6_CTRL, 0x0001, 0x0000);

	if (ip_version == 0x1) {
		// step6: start output audio stream(differential signal)
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00, 0x08, 0x08);

		//utopia step7: when audio part is ready
		//arc always 2 ch
		earc_atop_aupll(dev_runtime->sample_rate, 2);

		// step8: clear AVMUTE in channel status.
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG18, 0x04, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG29+1, 0x01, 0x01);//update
		mtk_alsa_delaytask(1);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG29+1, 0x01, 0x00);
	}

	return 0;
}



static int earc_dma_start(int id, struct mtk_pb_runtime_struct *dev_runtime)
{
	//[15]REG_SWRSTZ_64XCHXFS
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x80, 0x80);
	//[14]REG_SWRSTZ_32XCHXFS
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x40, 0x40);
	//[13]REG_SWRSTZ_32XFS
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x20, 0x20);
	//[12]REG_SWRSTZ_SOURCE
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x10, 0x10);
	mtk_alsa_delaytask(1);
	//[09]REG_IEC60958_BIPHASEMARK_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x02, 0x02);
	//[08]REG_PARITY_BIT_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x01, 0x01);
	//[07]REG_ECC_ENCODE_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x80, 0x80);
	//[06]REG_MULTI2DOUBLE_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x40, 0x40);
	//[11]REG_RST_AFIFO_RIU
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x00);
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x08);
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x00);
	//[04]REG_VALID_BIT_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x10, 0x10);
	//[03]REG_USER_STATUS_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x08, 0x08);
	//[02]REG_USER_STATUS_CRC_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x04, 0x04);
	//[01]REG_CHANNEL_STATUS_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x02, 0x02);
	//[05]REG_MIXER_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x20, 0x20);
	//[00]REG_MIXER_SOURCE_EN
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x01, 0x01);
	mtk_alsa_delaytask(1);

	/* start dplayback engine */
	mtk_alsa_write_reg_mask(REG.DPB_DMA7_CTRL, 0x0003, 0x0002);

	/* trigger DSP to reset DMA engine */
	mtk_alsa_write_reg_mask(REG.DPB_DMA7_CTRL, 0x0001, 0x0001);
	mtk_alsa_delaytask_us(50);
	mtk_alsa_write_reg_mask(REG.DPB_DMA7_CTRL, 0x0001, 0x0000);

	// step6: start output audio stream(differential signal)
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00, 0x08, 0x08);

	//utopia step7: when audio part is ready
	earc_atop_aupll(dev_runtime->sample_rate, dev_runtime->channel_mode);

	// step8: clear AVMUTE in channel status.
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG18, 0x04, 0x00);
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG29+1, 0x01, 0x01);//update
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG29+1, 0x01, 0x00);
	return 0;
}

static int dma_reader_stop(struct mtk_pb_runtime_struct *dev_runtime, int id)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	//pr_info("Stop MTK PCM Playback engine\n");

	switch (id) {
	case SPDIF_PCM_PLAYBACK:
	case SPDIF_NONPCM_PLAYBACK:
		mtk_alsa_write_reg_mask_byte(REG.DPB_SPDIF_CFG, 0x07, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_SPDIF_CFG+1, 0x80, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS0, 0x40, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x40);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0xA0, 0xA0);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x40, 0x00);
		mtk_alsa_delaytask(1);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x40, 0x40);
		mtk_alsa_delaytask(5);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x40, 0x00);
		/* reset PCM playback engine */
		mtk_alsa_write_reg_mask(REG.DPB_DMA5_CTRL, 0x0003, 0x0001);
		/* clear PCM playback write pointer */
		mtk_alsa_write_reg(REG.DPB_DMA5_WPTR, 0x0000);
		break;
	case ARC_PCM_PLAYBACK:
	case ARC_NONPCM_PLAYBACK:
		mtk_alsa_write_reg_mask_byte(REG.DPB_ARC_CFG, 0x07, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_ARC_CFG+1, 0x80, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS0, 0x40, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS3, 0xF0, 0x40);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0xA0, 0xA0);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x10, 0x00);
		mtk_alsa_delaytask(1);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x10, 0x10);
		mtk_alsa_delaytask(5);
		mtk_alsa_write_reg_mask_byte(REG.DPB_CS_TOGGLE+1, 0x10, 0x00);
		/* reset PCM playback engine */
		mtk_alsa_write_reg_mask(REG.DPB_DMA6_CTRL, 0x0003, 0x0001);
		/* clear PCM playback write pointer */
		mtk_alsa_write_reg(REG.DPB_DMA6_WPTR, 0x0000);
		break;
	case EARC_PCM_PLAYBACK:
	case EARC_NONPCM_PLAYBACK:
		// utopia step1: set AVMUTE bit in channel status
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG18, 0x04, 0x04);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG29+1, 0x01, 0x01);
		mtk_alsa_delaytask(1);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG29+1, 0x01, 0x00);
		// stop HDMI differential clock setting
		mtk_alsa_delaytask(10);
		earc_atop_aupll(0, 0);
		mtk_alsa_delaytask(10);
		// utopia step2: stop output audio stream(differential signal)
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_TX00, 0x08, 0x00);
		mtk_alsa_delaytask(125);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG10+1, 0x70, 0x00);
		//[00]REG_MIXER_SOURCE_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x01, 0x00);
		//[01]REG_CHANNEL_STATUS_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x02, 0x00);
		//[02]REG_USER_STATUS_CRC_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x04, 0x00);
		//[03]REG_USER_STATUS_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x08, 0x00);
		//[04]REG_VALID_BIT_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x10, 0x00);
		//[11]REG_RST_AFIFO_RIU
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x00);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x08);
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x08, 0x00);
		//[05]REG_MIXER_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x20, 0x00);
		//[06]REG_MULTI2DOUBLE_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x40, 0x00);
		//[07]REG_ECC_ENCODE_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02, 0x80, 0x00);
		//[08]REG_PARITY_BIT_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x01, 0x00);
		//[09]REG_IEC60958_BIPHASEMARK_EN
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x02, 0x00);

		//[12]REG_SWRSTZ_SOURCE
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x10, 0x00);
		//[13]REG_SWRSTZ_32XFS
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x20, 0x00);
		//[14]REG_SWRSTZ_32XCHXFS
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x40, 0x00);
		//[15]REG_SWRSTZ_64XCHXFS
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG02+1, 0x80, 0x00);
		mtk_alsa_delaytask(1);

		/* reset PCM playback engine */
		mtk_alsa_write_reg_mask(REG.DPB_DMA7_CTRL, 0x0003, 0x0001);
		/* clear PCM playback write pointer */
		mtk_alsa_write_reg(REG.DPB_DMA7_WPTR, 0x0000);
		break;
	default:
		pr_err("[ALSA DPB] set wrong device stop\n");
		break;
	}

	/* reset Write Pointer */
	pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;

	/* reset written size */
	pcm_playback->written_size = 0;

	pcm_playback->status = CMD_STOP;

	#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	//ARC pin control
	if (id == ARC_PCM_PLAYBACK ||  id == ARC_NONPCM_PLAYBACK)
		hdmi_arc_pin_control(pcm_playback->status);
	#endif
	return 0;
}

static void dma_reader_set_channel_mode(struct mtk_pb_runtime_struct *dev_runtime,
		unsigned int channel_mode, unsigned int dma_size)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	struct dplayback_priv *priv;
	int id;

	priv = (struct dplayback_priv *) dev_runtime->private;
	id = dev_runtime->id;

	AUD_PR_PRINT_DPB(id, AUD_INFO, priv,
		"[ALSA DPB]digital target channel mode is %u\n", channel_mode);

	dev_runtime->channel_mode = channel_mode;
	pcm_playback->period_size = dma_size >> 2;
	pcm_playback->buffer.size = dma_size;
	pcm_playback->high_threshold =
		pcm_playback->buffer.size - (pcm_playback->buffer.size >> 3);
}

static int dma_reader_get_inused_lines(int id)
{
	int inused_lines = 0;

	switch (id) {
	case SPDIF_PCM_PLAYBACK:
	case SPDIF_NONPCM_PLAYBACK:
		mtk_alsa_write_reg_mask(REG.DPB_DMA5_CTRL, 0x0008, 0x0008);
		inused_lines = mtk_alsa_read_reg(REG.DPB_DMA5_DDR_LEVEL);
		mtk_alsa_write_reg_mask(REG.DPB_DMA5_CTRL, 0x0008, 0x0000);
		break;
	case ARC_PCM_PLAYBACK:
	case ARC_NONPCM_PLAYBACK:
		mtk_alsa_write_reg_mask(REG.DPB_DMA6_CTRL, 0x0008, 0x0008);
		inused_lines = mtk_alsa_read_reg(REG.DPB_DMA6_DDR_LEVEL);
		mtk_alsa_write_reg_mask(REG.DPB_DMA6_CTRL, 0x0008, 0x0000);
		break;
	case EARC_PCM_PLAYBACK:
	case EARC_NONPCM_PLAYBACK:
		mtk_alsa_write_reg_mask(REG.DPB_DMA7_CTRL, 0x0008, 0x0008);
		inused_lines = mtk_alsa_read_reg(REG.DPB_DMA7_DDR_LEVEL);
		mtk_alsa_write_reg_mask(REG.DPB_DMA7_CTRL, 0x0008, 0x0000);
		break;
	default:
		pr_err("[ALSA DPB] get wrong device level\n");
		break;
	}
	return inused_lines;
}

static void dma_reader_get_avail_bytes(int id,
			struct mtk_pb_runtime_struct *dev_runtime)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	int inused_lines = 0;
	int avail_lines = 0;
	int avail_bytes = 0;
	struct dplayback_priv *priv;

	priv = (struct dplayback_priv *)dev_runtime->private;

	inused_lines = dma_reader_get_inused_lines(id);
	avail_lines = (pcm_playback->buffer.size / BYTES_IN_MIU_LINE) -
					inused_lines;

	if (avail_lines < 0) {
		AUD_PR_PRINT_DPB(id, AUD_ERR, priv,
			"[ALSA DPB]spdif incorrect inused line %d\n", inused_lines);
		avail_lines = 0;
	}

	avail_bytes = avail_lines * BYTES_IN_MIU_LINE;
	if (dev_runtime->channel_mode == CH_MONO)
		avail_bytes >>= 1;

	dev_runtime->buffer.avail_size = avail_bytes;
}

static int dma_reader_get_consumed_bytes(int id,
		struct mtk_pb_runtime_struct *dev_runtime)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	int inused_bytes = 0;
	int consumed_bytes = 0;

	inused_bytes = dma_reader_get_inused_lines(id) * BYTES_IN_MIU_LINE;
	consumed_bytes = pcm_playback->written_size - inused_bytes;
	if (consumed_bytes < 0)
		consumed_bytes = 0;
	else
		pcm_playback->written_size = inused_bytes;

	/* report actual level to driver layer */
	if (dev_runtime->channel_mode == CH_MONO)
		consumed_bytes >>= 1;

	return consumed_bytes;
}

static unsigned int dma_reader_write(int id,
				struct mtk_pb_runtime_struct *dev_runtime,
				void *buffer, unsigned int bytes)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	unsigned char *buffer_tmp = (unsigned char *)buffer;
	unsigned int w_ptr_offset = 0;
	unsigned int copy_lr_sample = 0;
	unsigned int copy_size = 0;
	int inused_bytes = 0;
	int loop = 0;
	struct dplayback_priv *priv;

	priv = (struct dplayback_priv *)dev_runtime->private;

	copy_lr_sample = bytes / CH_2; /* L + R samples */
	copy_size =
		(dev_runtime->channel_mode == CH_MONO) ? (bytes * 2) : bytes;
	inused_bytes = dma_reader_get_inused_lines(id) * BYTES_IN_MIU_LINE;

	if ((inused_bytes == 0) && (pcm_playback->status == CMD_START))
		AUD_PR_PRINT_DPB(id, AUD_INFO, priv,
			"[ALSA DPB]*****dplayback buffer empty !!*****\n");
	else if ((inused_bytes + copy_size) > pcm_playback->high_threshold) {
		//pr_err("*****PCM Playback Buffer full !!*****\n");
		return 0;
	}

	if (dev_runtime->channel_mode == CH_MONO) {
		for (loop = 0; loop < copy_lr_sample; loop++) {
			unsigned char sample_lo = *(buffer_tmp++);
			unsigned char sample_hi = *(buffer_tmp++);

			*(pcm_playback->buffer.w_ptr++) = sample_lo;
			*(pcm_playback->buffer.w_ptr++) = sample_hi;
			*(pcm_playback->buffer.w_ptr++) = sample_lo;
			*(pcm_playback->buffer.w_ptr++) = sample_hi;

			if (pcm_playback->buffer.w_ptr >=
				(pcm_playback->buffer.addr + pcm_playback->buffer.size))
				pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;
		}
	} else {
		for (loop = 0; loop < copy_lr_sample; loop++) {
			if (pcm_playback->buffer.w_ptr + CH_2 >
				(pcm_playback->buffer.addr + pcm_playback->buffer.size))
				pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;
			*(pcm_playback->buffer.w_ptr++) = *(buffer_tmp++);
			*(pcm_playback->buffer.w_ptr++) = *(buffer_tmp++);

		}
	}

	/* flush MIU */
	smp_mb();

	/* update PCM playback write pointer */
	w_ptr_offset = pcm_playback->buffer.w_ptr - pcm_playback->buffer.addr;
	switch (id) {
	case SPDIF_PCM_PLAYBACK:
	case SPDIF_NONPCM_PLAYBACK:
		mtk_alsa_write_reg_mask(REG.DPB_DMA5_WPTR, 0xFFFF,
				(w_ptr_offset / BYTES_IN_MIU_LINE));
		break;
	case ARC_PCM_PLAYBACK:
	case ARC_NONPCM_PLAYBACK:
		mtk_alsa_write_reg_mask(REG.DPB_DMA6_WPTR, 0xFFFF,
				(w_ptr_offset / BYTES_IN_MIU_LINE));
		break;
	case EARC_PCM_PLAYBACK:
	case EARC_NONPCM_PLAYBACK:
		mtk_alsa_write_reg_mask(REG.DPB_DMA7_WPTR, 0xFFFF,
				(w_ptr_offset / BYTES_IN_MIU_LINE));
		break;
	default:
		pr_err("[ALSA DPB] wrong device write\n");
		break;
	}
	pcm_playback->written_size += copy_size;
	pcm_playback->status = CMD_START;
	/* ensure write pointer can be applied */
	mtk_alsa_delaytask(1);

	return bytes;
}

static int digital_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	int ret;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	/* Set specified information */
	dev_runtime->substreams.p_substream = substream;
	dev_runtime->private = priv;
	dev_runtime->id = id;

	snd_soc_set_runtime_hwparams(substream, &dplayback_default_hw);

	switch (id) {
	case SPDIF_PCM_PLAYBACK:
	case ARC_PCM_PLAYBACK:
	case EARC_PCM_PLAYBACK:
		pr_info("[ALSA DPB]set PCM constraint rate list\n");
		ret = snd_pcm_hw_constraint_list(runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE, &constraints_dpb_pcm_rates);
		break;
	case SPDIF_NONPCM_PLAYBACK:
		pr_info("[ALSA DPB]set spdif nonpcm constraint rate list\n");
		ret = snd_pcm_hw_constraint_list(runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE, &constraints_spdif_nonpcm_rates);
		break;
	case ARC_NONPCM_PLAYBACK:
		pr_info("[ALSA DPB]set arc nonpcm constraint rate list\n");
		ret = snd_pcm_hw_constraint_list(runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE, &constraints_arc_nonpcm_rates);
		break;
	case EARC_NONPCM_PLAYBACK:
		pr_info("[ALSA DPB]set earc nonpcm constraint rate list\n");
		ret = snd_pcm_hw_constraint_list(runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE, &constraints_earc_nonpcm_rates);
		break;
	default:
		ret = -1;
		pr_err("[ALSA DPB] set wrong device hardware rate\n");
		break;
	}
	if (ret < 0)
		return ret;

	ret = snd_pcm_hw_constraint_list(runtime, 0,
		SNDRV_PCM_HW_PARAM_CHANNELS, &constraints_dpb_channels);
	if (ret < 0)
		return ret;

	/* Allocate system memory for Specific Copy */
	dev_runtime->buffer.addr = vmalloc(MAX_DIGITAL_BUF_SIZE);
	if (dev_runtime->buffer.addr == NULL)
		return -ENOMEM;

	memset((void *)dev_runtime->buffer.addr, 0x00, MAX_DIGITAL_BUF_SIZE);
	dev_runtime->buffer.size = MAX_DIGITAL_BUF_SIZE;

	spin_lock_init(&dev_runtime->spin_lock);
	timer_open(&dev_runtime->timer,
			(unsigned long)dev_runtime, (void *)dplayback_monitor);

	return 0;
}

static void digital_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	unsigned long flags;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	/* Reset some variables */
	dev_runtime->substreams.substream_status = CMD_STOP;
	dev_runtime->substreams.p_substream = NULL;

	/* Free allocated memory */
	if (dev_runtime->buffer.addr != NULL) {
		vfree(dev_runtime->buffer.addr);
		dev_runtime->buffer.addr = NULL;
	}

	timer_close(&dev_runtime->timer);

	spin_lock_irqsave(&dev_runtime->spin_lock, flags);
	memset(&dev_runtime->monitor, 0x00,
			sizeof(struct mtk_monitor_struct));
	spin_unlock_irqrestore(&dev_runtime->spin_lock, flags);

	/* Reset PCM configurations */
	dev_runtime->sample_rate = 0;
	dev_runtime->channel_mode = 0;
	dev_runtime->runtime_status = CMD_STOP;
	dev_runtime->device_status = 0;

	/* Stop DMA Reader  */
	dma_reader_stop(dev_runtime, id);
	mtk_alsa_delaytask(2);

	/* Close MTK Audio DSP */
	dma_reader_exit(id, dev_runtime);
	mtk_alsa_delaytask(2);
}

static int digital_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	return 0;
}

static int digital_hw_free(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	return 0;
}

static int spdif_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];

	dev_runtime->private = priv;
	dev_runtime->id = id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);
	/*print hw parameter*/
	AUD_DEV_PRINT_DPB(dai->dev, id, AUD_INFO, priv, "\nparameter :\n"
								"format is %d\n"
								"rate is %d\n"
								"channels is %d\n"
								"period_size is %lu\n"
								"sample_bits is %d\n "
								, runtime->format
								, runtime->rate
								, runtime->channels
								, runtime->period_size
								, runtime->sample_bits);

	dev_runtime->device_status = 1;

	if (dev_runtime->runtime_status != CMD_START) {
		/* Configure SW DMA sample rate */
		AUD_DEV_PRINT_DPB(dai->dev, id, AUD_INFO, priv,
			"[ALSA DPB]spdif target sample rate is %u\n", runtime->rate);
		dev_runtime->sample_rate = runtime->rate;
		/* Configure SW DMA channel mode */
		dma_reader_set_channel_mode(dev_runtime, runtime->channels,
						priv->spdif_dma_size);
		if (id == SPDIF_NONPCM_PLAYBACK)
			spdif_nonpcm_set_synthesizer(runtime->rate);
		else
			mtk_alsa_write_reg_mask_byte(REG.DPB_SPDIF_CFG, 0x07, 0x00);

		/* Init SW DMA */
		dma_reader_init(priv, dev_runtime, runtime, priv->spdif_dma_offset, id);
		/* reset SW DMA engine */
		mtk_alsa_write_reg(REG.DPB_DMA5_CTRL, 0x0001);
		/* clear SW DMA write pointer */
		mtk_alsa_write_reg(REG.DPB_DMA5_WPTR, 0x0000);
		/* Start SW DMA */
		spdif_dma_start(id, dev_runtime);
		/* Configure SW DMA device status */
		dev_runtime->runtime_status = CMD_START;
	} else {
		/* Stop SW DMA */
		dma_reader_stop(dev_runtime, id);
		mtk_alsa_delaytask(2);
		/* Start SW DMA */
		spdif_dma_start(id, dev_runtime);
	}
	return 0;
}

static int arc_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];

	ALSA_AUDIO_ENTER(dai->dev, id, priv);
	/*print hw parameter*/
	AUD_DEV_PRINT_DPB(dai->dev, id, AUD_INFO, priv, "\nparameter :\n"
								"format is %d\n"
								"rate is %d\n"
								"channels is %d\n"
								"period_size is %lu\n"
								"sample_bits is %d\n "
								, runtime->format
								, runtime->rate
								, runtime->channels
								, runtime->period_size
								, runtime->sample_bits);

	dev_runtime->device_status = 1;
	dma_register_get_reg(id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);

	priv->ip_version = mtk_alsa_read_chip();

	if (dev_runtime->runtime_status != CMD_START) {
		/* Configure SW DMA sample rate */
		pr_info("[ALSA DPB]arc target sample rate is %u\n", runtime->rate);
		dev_runtime->sample_rate = runtime->rate;
		/* Configure SW DMA channel mode */
		dma_reader_set_channel_mode(dev_runtime, runtime->channels,
						priv->arc_dma_size);

		if (priv->ip_version == 1) {
			//m6L arc data from earc ip
			/* Stop SW DMA */
			//utopia step1,2,3: StopSequence
			dma_reader_stop(dev_runtime, id);

			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG51, 0x10, 0x10);

			earc_nonpcm_set_synthesizer(runtime->rate);

			//utopia step4: SetSequence
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG10+1, 0x70, 0x00);
			//[14]REG_SWRSTZ_64XCHXFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x40, 0x00);
			//[14]REG_SWRSTZ_32XCHXFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x40, 0x00);
			//[06]REG_SWRSTZ_32XFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x40, 0x00);
			mtk_alsa_delaytask(1);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x40, 0x40);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x40, 0x40);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x40, 0x40);

			//[2:0]REG_EARC_TX_CH
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x07, 0x00);
			//[2:0]REG_EARC_TX_MIXER_CH
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x07, 0x00);
			//[6:4]REG_EARC_TX_MULTI2DOUBLE_CH
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x70, 0x00);
			//[10:8]REG_EARC_TX_USER_STATUS_CH
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x07, 0x00);
			//[14:12]REG_EARC_TX_CHANNEL_STATUS_CH
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x70, 0x00);

			//[12:8]REG_64XCHXFS_DFS_DIV
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x1F, 0x01);
			//[4:0]REG_32XFS_DFS_DIV
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x1F, 0x0F);
			//[12:8]REG_32XCHXFS_DFS_DIV
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x1F, 0x0F);

			//[13]REG_64XCHXFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x20, 0x20);
			//[05]REG_32XFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x20, 0x20);
			//[13]REG_32XCHXFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x20, 0x20);

			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x20, 0x00);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x20, 0x00);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x20, 0x00);

			//[04]REG_EARC_TX_NPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x10, 0x00);
			//[07]REG_EARC_TX_MIXER_NPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x80, 0x00);
			//[08]REG_EARC_TX_ECC_NPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08+1, 0x01, 0x00);
			//[05]REG_USER_STATUS_ARC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x20, 0x00);
			//[06]REG_CHANNEL_STATUS_ARC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x40, 0x00);

			//[04]REG_EARC_TX_CFG_DATA_SEL = 1'b1
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG03, 0x10, 0x10);

			//[06]REG_CHANNEL_STATUS_ARC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x40, 0x40);

			//[03]REG_EARC_TX_VALID_BIT_SEL
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG07, 0x08, 0x00);
			//[09]REG_EARC_TX_VALID_BIT_IN
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08+1, 0x02, 0x00);
		} else {
			//m6 arc ip from arc ip
			if (id == ARC_NONPCM_PLAYBACK)
				arc_nonpcm_set_synthesizer(runtime->rate);
			else
				mtk_alsa_write_reg_mask_byte(REG.DPB_ARC_CFG, 0x07, 0x00);
		}

		/* Init SW DMA */
		dma_reader_init(priv, dev_runtime, runtime, priv->arc_dma_offset, id);
		/* reset SW DMA engine */
		mtk_alsa_write_reg(REG_CTRL, 0x0001);

		if (dev_runtime->format == SNDRV_PCM_FORMAT_S24_LE)
			mtk_alsa_write_reg_mask_byte(REG_CTRL, 0x30, 0x10);

		/* clear SW DMA write pointer */
		mtk_alsa_write_reg(REG_WPTR, 0x0000);

		/* Start SW DMA */
		arc_dma_start(id, dev_runtime, priv->ip_version);
		/* Configure SW DMA device status */
		dev_runtime->runtime_status = CMD_START;
	} else {
		/* Stop SW DMA */
		dma_reader_stop(dev_runtime, id);
		mtk_alsa_delaytask(2);
		/* Start SW DMA */
		arc_dma_start(id, dev_runtime, priv->ip_version);
	}

	#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	//ARC pin control
	hdmi_arc_pin_control(dev_runtime->runtime_status);
	#endif

	return 0;
}

static int earc_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;
	struct mtk_channel_status_struct *cs = &dev_runtime->channel_status;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);
	/*print hw parameter*/
	AUD_DEV_PRINT_DPB(dai->dev, id, AUD_INFO, priv, "\nparameter :\n"
								"format is %d\n"
								"rate is %d\n"
								"channels is %d\n"
								"period_size is %lu\n"
								"sample_bits is %d\n "
								, runtime->format
								, runtime->rate
								, runtime->channels
								, runtime->period_size
								, runtime->sample_bits);


	dev_runtime->device_status = 1;
	dma_register_get_reg(id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);

	if (dev_runtime->runtime_status != CMD_START) {
		/* Configure SW DMA sample rate */
		pr_info("[ALSA DPB]earc target sample rate is %u\n", runtime->rate);
		dev_runtime->sample_rate = runtime->rate;
		/* Configure SW DMA channel mode */
		dma_reader_set_channel_mode(dev_runtime, runtime->channels,
					priv->earc_dma_size);
		/* Stop SW DMA */
		//utopia step1,2,3: StopSequence
		dma_reader_stop(dev_runtime, id);

		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG51, 0x10, 0x10);

		earc_nonpcm_set_synthesizer(runtime->rate);

		//utopia step4: SetSequence
		if (id == EARC_NONPCM_PLAYBACK) {
			//[14]REG_SWRSTZ_64XCHXFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x40, 0x00);
			//[14]REG_SWRSTZ_32XCHXFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x40, 0x00);
			//[06]REG_SWRSTZ_32XFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x40, 0x00);
			mtk_alsa_delaytask(1);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x40, 0x40);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x40, 0x40);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x40, 0x40);
			if (dev_runtime->channel_mode == 2) {
				//[2:0]REG_EARC_TX_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x07, 0x00);
				//[2:0]REG_EARC_TX_MIXER_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x07, 0x00);
				//[6:4]REG_EARC_TX_MULTI2DOUBLE_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x70, 0x00);
				//[10:8]REG_EARC_TX_USER_STATUS_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x07, 0x00);
				//[14:12]REG_EARC_TX_CHANNEL_STATUS_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x70, 0x00);
				//[12:8]REG_64XCHXFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x1F, 0x01);
				//[4:0]REG_32XFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x1F, 0x0F);
				//[12:8]REG_32XCHXFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x1F, 0x0F);
			} else if (dev_runtime->channel_mode == 8) {
				//[2:0]REG_EARC_TX_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x07, 0x01);
				//[2:0]REG_EARC_TX_MIXER_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x07, 0x01);
				//[6:4]REG_EARC_TX_MULTI2DOUBLE_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x70, 0x10);
				//[10:8]REG_EARC_TX_USER_STATUS_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x07, 0x01);
				//[14:12]REG_EARC_TX_CHANNEL_STATUS_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x70, 0x10);
				//[12:8]REG_64XCHXFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x1F, 0x07);
				//[4:0]REG_32XFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x1F, 0x0F);
				//[12:8]REG_32XCHXFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x1F, 0x03);
			}
			//[13]REG_64XCHXFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x20, 0x20);
			//[05]REG_32XFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x20, 0x20);
			//[13]REG_32XCHXFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x20, 0x20);

			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x20, 0x00);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x20, 0x00);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x20, 0x00);

			//[04]REG_EARC_TX_NPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x10, 0x10);

			//[04]REG_EARC_TX_CFG_DATA_SEL = 1'b1
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG03, 0x10, 0x10);

			//[06]REG_CHANNEL_STATUS_ARC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x40, 0x40);

			earc_channel_status(dev_runtime, id, cs);

			//[03]REG_EARC_TX_VALID_BIT_SEL
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG07, 0x08, 0x08);
			//[09]REG_EARC_TX_VALID_BIT_IN
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08+1, 0x02, 0x02);
		} else {
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG10+1, 0x70, 0x00);
			//[14]REG_SWRSTZ_64XCHXFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x40, 0x00);
			//[14]REG_SWRSTZ_32XCHXFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x40, 0x00);
			//[06]REG_SWRSTZ_32XFS_DFS
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x40, 0x00);
			mtk_alsa_delaytask(1);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x40, 0x40);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x40, 0x40);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x40, 0x40);

			if (dev_runtime->channel_mode == 2) {
				//[2:0]REG_EARC_TX_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x07, 0x00);
				//[2:0]REG_EARC_TX_MIXER_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x07, 0x00);
				//[6:4]REG_EARC_TX_MULTI2DOUBLE_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x70, 0x00);
				//[10:8]REG_EARC_TX_USER_STATUS_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x07, 0x00);
				//[14:12]REG_EARC_TX_CHANNEL_STATUS_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x70, 0x00);

				//[12:8]REG_64XCHXFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x1F, 0x01);
				//[4:0]REG_32XFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x1F, 0x0F);
				//[12:8]REG_32XCHXFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x1F, 0x0F);
			} else if (dev_runtime->channel_mode == 8) {  //multi ch pcm 8ch
				//[2:0]REG_EARC_TX_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x07, 0x01);
				//[2:0]REG_EARC_TX_MIXER_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x07, 0x01);
				//[6:4]REG_EARC_TX_MULTI2DOUBLE_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09, 0x70, 0x10);
				//[10:8]REG_EARC_TX_USER_STATUS_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x07, 0x01);
				//[14:12]REG_EARC_TX_CHANNEL_STATUS_CH
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG09+1, 0x70, 0x10);
				//[12:8]REG_64XCHXFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x1F, 0x07);
				//[4:0]REG_32XFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x1F, 0x0F);
				//[12:8]REG_32XCHXFS_DFS_DIV
				mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x1F, 0x03);
			}

			//[13]REG_64XCHXFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x20, 0x20);
			//[05]REG_32XFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x20, 0x20);
			//[13]REG_32XCHXFS_DFS_UPDATE
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x20, 0x20);

			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG00+1, 0x20, 0x00);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01, 0x20, 0x00);
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG01+1, 0x20, 0x00);

			//[04]REG_EARC_TX_NPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x10, 0x00);
			//[07]REG_EARC_TX_MIXER_NPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x80, 0x00);
			//[08]REG_EARC_TX_ECC_NPCM
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08+1, 0x01, 0x00);
			//[05]REG_USER_STATUS_ARC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x20, 0x00);
			//[06]REG_CHANNEL_STATUS_ARC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x40, 0x00);

			earc_channel_status(dev_runtime, id, cs);

			//[04]REG_EARC_TX_CFG_DATA_SEL = 1'b1
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG03, 0x10, 0x10);

			//[06]REG_CHANNEL_STATUS_ARC
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08, 0x40, 0x40);

			//[03]REG_EARC_TX_VALID_BIT_SEL
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG07, 0x08, 0x00);
			//[09]REG_EARC_TX_VALID_BIT_IN
			mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG08+1, 0x02, 0x00);
		}
		/* Init SW DMA */
		dma_reader_init(priv, dev_runtime, runtime, priv->earc_dma_offset, id);
		/* reset SW DMA engine */
		mtk_alsa_write_reg(REG.DPB_DMA7_CTRL, 0x0001);

		if (dev_runtime->channel_mode == 8)
			mtk_alsa_write_reg_mask_byte(REG_CTRL, 0xC0, 0x80);//for earc multi 8ch
		else
			mtk_alsa_write_reg_mask_byte(REG_CTRL, 0xC0, 0x00);

		if (dev_runtime->format == SNDRV_PCM_FORMAT_S24_LE)
			mtk_alsa_write_reg_mask_byte(REG_CTRL, 0x30, 0x10);//for earc 24bit bit4

		/* Start SW DMA */
		//utopia step5,6,7,8: PlaySequence
		earc_dma_start(id, dev_runtime);
		/* Configure SW DMA device status */
		dev_runtime->runtime_status = CMD_START;
	} else {
		/* Stop SW DMA */
		dma_reader_stop(dev_runtime, id);
		mtk_alsa_delaytask(2);
		/* Start SW DMA */
		earc_dma_start(id, dev_runtime);
	}
	return 0;
}

static int digital_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	int err = 0;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_STOP:
		dev_runtime->substreams.substream_status = CMD_STOP;
		break;
	case SNDRV_PCM_TRIGGER_START:
		dev_runtime->substreams.substream_status = CMD_START;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		dev_runtime->substreams.substream_status = CMD_PAUSE;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		dev_runtime->substreams.substream_status = CMD_PAUSE_RELEASE;
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		dev_runtime->substreams.substream_status = CMD_SUSPEND;
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		dev_runtime->substreams.substream_status = CMD_RESUME;
		break;
	default:
		pr_info("[ALSA DPB]dplayback invalid PB's trigger command %d\n", cmd);
		err = -EINVAL;
		break;
	}

	return 0;
}

static const struct snd_soc_dai_ops spdif_dai_ops = {
	.startup	= digital_startup,
	.shutdown	= digital_shutdown,
	.hw_params	= digital_hw_params,
	.hw_free	= digital_hw_free,
	.prepare	= spdif_prepare,
	.trigger	= digital_trigger,
};

static const struct snd_soc_dai_ops arc_dai_ops = {
	.startup	= digital_startup,
	.shutdown	= digital_shutdown,
	.hw_params	= digital_hw_params,
	.hw_free	= digital_hw_free,
	.prepare	= arc_prepare,
	.trigger	= digital_trigger,
};

static const struct snd_soc_dai_ops earc_dai_ops = {
	.startup	= digital_startup,
	.shutdown	= digital_shutdown,
	.hw_params	= digital_hw_params,
	.hw_free	= digital_hw_free,
	.prepare	= earc_prepare,
	.trigger	= digital_trigger,
};

int digital_dai_suspend(struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	unsigned int id = dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;

	pr_info("[ALSA DPB]device%d suspend\n", id);

	if (dev_runtime->clk_status) {
		switch (id) {
		case SPDIF_PCM_PLAYBACK:
			mtk_spdif_tx_clk_set_disable(0);
			break;
		case SPDIF_NONPCM_PLAYBACK:
			mtk_spdif_tx_clk_set_disable(1);
			break;
		case ARC_PCM_PLAYBACK:
			if (priv->ip_version == 1 && priv->dp_pcm_flag == 1) {
				mtk_earc_clk_set_disable(0);
				priv->dp_pcm_flag = 0;
			} else if (priv->ip_version == 0)
				mtk_arc_clk_set_disable(0);
			else
				pr_info("[ALSA DPB] ARC PCM can not close");
			break;
		case ARC_NONPCM_PLAYBACK:
			if (priv->ip_version == 1 && priv->dp_np_flag == 1) {
				mtk_earc_clk_set_disable(1);
				priv->dp_np_flag = 0;
			} else if (priv->ip_version == 0)
				mtk_arc_clk_set_disable(1);
			else
				pr_info("[ALSA DP] ARC NONPCM can not close");
			break;
		case EARC_PCM_PLAYBACK:
			if (priv->dp_pcm_flag == 1) {
				mtk_earc_clk_set_disable(0);
				priv->dp_pcm_flag = 0;
			} else
				pr_info("[ALSA DPB] EARC PCM can not close");
			break;
		case EARC_NONPCM_PLAYBACK:
			if (priv->dp_np_flag == 1) {
				mtk_earc_clk_set_disable(1);
				priv->dp_np_flag = 0;
			} else
				pr_info("[ALSA DPB] EARC NONPCM can not close");
			break;
		default:
			pr_info("[ALSA DPB]invalid pcm device clock disable %u\n", id);
			break;
		}
		dev_runtime->clk_status = 0;
	}

	if (dev_runtime->runtime_status == CMD_STOP)
		return 0;

	dma_reader_stop(dev_runtime, id);

	pcm_playback->status = CMD_SUSPEND;
	dev_runtime->runtime_status = CMD_SUSPEND;
	dev_runtime->suspend_flag = 1;

	return 0;
}

int digital_dai_resume(struct snd_soc_dai *dai)
{
	struct dplayback_priv *priv = snd_soc_dai_get_drvdata(dai);
	unsigned int id = dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;

	pr_info("[ALSA DPB]device%u resume\n", id);

	if (dev_runtime->runtime_status == CMD_SUSPEND) {
		pcm_playback->status = CMD_RESUME;
		dev_runtime->runtime_status = CMD_RESUME;
	}

	return 0;
}

static struct snd_soc_dai_driver dplayback_dais[] = {
	{
		.name = "SPDIF-TX",
		.id = SPDIF_PCM_PLAYBACK,
		.playback = {
			.stream_name = "SPDIF_PLAYBACK",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.ops = &spdif_dai_ops,
		.suspend = digital_dai_suspend,
		.resume = digital_dai_resume,
	},
	{
		.name = "SPDIF-TX-NONPCM",
		.id = SPDIF_NONPCM_PLAYBACK,
		.playback = {
			.stream_name = "SPDIF_NONPCM_PLAYBACK",
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
					SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.ops = &spdif_dai_ops,
		.suspend = digital_dai_suspend,
		.resume = digital_dai_resume,
	},
	{
		.name = "ARC",
		.id = ARC_PCM_PLAYBACK,
		.playback = {
			.stream_name = "ARC_PLAYBACK",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_FMTBIT_S24_LE |
					SNDRV_PCM_FMTBIT_S32_LE,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.ops = &arc_dai_ops,
		.suspend = digital_dai_suspend,
		.resume = digital_dai_resume,
	},
	{
		.name = "ARC-NONPCM",
		.id = ARC_NONPCM_PLAYBACK,
		.playback = {
			.stream_name = "ARC_NONPCM_PLAYBACK",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
					SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000|
					SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &arc_dai_ops,
		.suspend = digital_dai_suspend,
		.resume = digital_dai_resume,
	},
	{
		.name = "EARC",
		.id = EARC_PCM_PLAYBACK,
		.playback = {
			.stream_name = "EARC_PLAYBACK",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
					SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
					SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &earc_dai_ops,
		.suspend = digital_dai_suspend,
		.resume = digital_dai_resume,
	},
	{
		.name = "EARC-NONPCM",
		.id = EARC_NONPCM_PLAYBACK,
		.playback = {
			.stream_name = "EARC_NONPCM_PLAYBACK",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
					SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
					SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &earc_dai_ops,
		.suspend = digital_dai_suspend,
		.resume = digital_dai_resume,
	},
};


static int dplayback_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
				snd_soc_rtdcom_lookup(rtd, DPLAYBACK_DMA_NAME);
	struct dplayback_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	int ret = 0;

	switch (id) {
	case SPDIF_PCM_PLAYBACK:
		mtk_spdif_tx_clk_set_enable(0);
		break;
	case SPDIF_NONPCM_PLAYBACK:
		mtk_spdif_tx_clk_set_enable(1);
		break;
	case ARC_PCM_PLAYBACK:
		if (priv->ip_version == 1 && priv->dp_pcm_flag == 0) {
			mtk_earc_clk_set_enable(0);
			priv->dp_pcm_flag = 1;
		} else if (priv->ip_version == 0)
			mtk_arc_clk_set_enable(0);
		else
			pr_err("[ALSA DPB] ARC pcm  is busy");
		break;
	case ARC_NONPCM_PLAYBACK:
		if (priv->ip_version == 1 && priv->dp_np_flag == 0) {
			mtk_earc_clk_set_enable(1);
			priv->dp_np_flag = 1;
		} else if (priv->ip_version == 0)
			mtk_arc_clk_set_enable(1);
		else
			pr_err("[ALSA DPB]digital ARC NONPCM is busy");
		break;
	case EARC_PCM_PLAYBACK:
		if (priv->dp_pcm_flag == 0) {
			mtk_earc_clk_set_enable(0);
			priv->dp_pcm_flag = 1;
		} else
			pr_err("[ALSA DPB] EARC PCM  is busy");
		break;
	case EARC_NONPCM_PLAYBACK:
		if (priv->dp_np_flag == 0) {
			mtk_earc_clk_set_enable(1);
			priv->dp_np_flag = 1;
		} else
			pr_err("[ALSA DPB] EARC NONPCM  is busy");
		break;
	default:
		pr_err("[ALSA DPB]invalid DPB device open %u\n", id);
		ret = -ENODEV;
		break;
	}
	dev_runtime->clk_status = 1;

	return ret;
}

static int dplayback_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
				snd_soc_rtdcom_lookup(rtd, DPLAYBACK_DMA_NAME);
	struct dplayback_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	int ret = 0;

	if (dev_runtime->clk_status) {
		switch (id) {
		case SPDIF_PCM_PLAYBACK:
			mtk_spdif_tx_clk_set_disable(0);
			break;
		case SPDIF_NONPCM_PLAYBACK:
			mtk_spdif_tx_clk_set_disable(1);
			break;
		case ARC_PCM_PLAYBACK:
			if (priv->ip_version == 1 && priv->dp_pcm_flag == 1) {
				mtk_earc_clk_set_disable(0);
				priv->dp_pcm_flag = 0;
			} else if (priv->ip_version == 0)
				mtk_arc_clk_set_disable(0);
			else
				pr_err("[ALSA DPB] ARC pcm can not close");
			break;
		case ARC_NONPCM_PLAYBACK:
			if (priv->ip_version == 1  && priv->dp_np_flag == 1) {
				mtk_earc_clk_set_disable(1);
				priv->dp_np_flag = 0;
			} else if (priv->ip_version == 0)
				mtk_arc_clk_set_disable(1);
			else
				pr_err("[ALSA DPB] ARC NONPCM can not close");
			break;
		case EARC_PCM_PLAYBACK:
			if (priv->dp_pcm_flag == 1) {
				mtk_earc_clk_set_disable(0);
				priv->dp_pcm_flag = 0;
			} else
				pr_err("[ALSA DPB] EARC pcm can not close");
			break;
		case EARC_NONPCM_PLAYBACK:
			if (priv->dp_np_flag == 1) {
				mtk_earc_clk_set_disable(1);
				priv->dp_np_flag = 0;
			} else
				pr_err("[ALSA DPB] EARC NONPCM can not close");
			break;
		default:
			pr_err("[ALSA DPB]invalid DPB device close %u\n", id);
			ret = -ENODEV;
			break;
		}
		dev_runtime->clk_status = 0;
	}
	dev_runtime->suspend_flag = 0;

	return ret;
}

static snd_pcm_uframes_t dplayback_pcm_pointer
				(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
				snd_soc_rtdcom_lookup(rtd, DPLAYBACK_DMA_NAME);
	struct dplayback_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct snd_pcm_runtime *runtime = substream->runtime;
	int consumed_bytes = 0;
	snd_pcm_uframes_t consumed_pcm_samples = 0;
	snd_pcm_uframes_t new_hw_ptr = 0;
	snd_pcm_uframes_t new_hw_ptr_pos = 0;

	if (dev_runtime->runtime_status == CMD_START) {
		consumed_bytes = dma_reader_get_consumed_bytes(id, dev_runtime);
		consumed_pcm_samples = bytes_to_frames(runtime, consumed_bytes);
	}

	new_hw_ptr = runtime->status->hw_ptr + consumed_pcm_samples;
	new_hw_ptr_pos = new_hw_ptr % runtime->buffer_size;

	return new_hw_ptr_pos;
}

static int dplayback_pcm_copy(struct snd_pcm_substream *substream, int channel,
		unsigned long pos, void __user *buf, unsigned long bytes)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
				snd_soc_rtdcom_lookup(rtd, DPLAYBACK_DMA_NAME);
	struct dplayback_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned char *buffer_tmp = NULL;
	unsigned int period_size = pcm_playback->period_size;
	unsigned int request_size = 0;
	unsigned int copied_size = 0;
	unsigned int size_to_copy = 0;
	unsigned int size = 0;
	int retry_counter = 0;
	unsigned long flags;
	static unsigned int warning_counter;

	dev_runtime->private = priv;

	if (buf == NULL) {
		pr_err("[ALSA DPB]Err! invalid memory address!\n");
		return -EINVAL;
	}

	if (bytes == 0) {
		pr_err("[ALSA DPB]Err! request bytes is zero!\n");
		return -EINVAL;
	}

	if (dev_runtime->suspend_flag == 1) {
		runtime->status->state = SNDRV_PCM_STATE_SUSPENDED;
		pr_err("[ALSA DPB]Err! after suspend need re-open device!\n");
		return -EINVAL;
	}

	warning_counter = 0;

	buffer_tmp = dev_runtime->buffer.addr;
	if (buffer_tmp == NULL) {
		pr_err("[ALSA DPB]Err! need to alloc system mem for PCM buf\n");
		return -ENXIO;
	}

	/* Wake up Monitor if necessary */
	if (dev_runtime->monitor.monitor_status == CMD_STOP) {
	//pr_info("Wake up Playback Monitor %d\n", substream->pcm->device);
		while (1) {
			if (timer_reset(&dev_runtime->timer) == 0) {
				spin_lock_irqsave(&dev_runtime->spin_lock, flags);
				dev_runtime->monitor.monitor_status = CMD_START;
				spin_unlock_irqrestore(&dev_runtime->spin_lock, flags);
				break;
			}

			if ((++retry_counter) > 50) {
				pr_err("[ALSA DPB]fail to wakeup Monitor %d\n",
						substream->pcm->device);
				break;
			}

			mtk_alsa_delaytask(1);
		}
	}

	request_size = bytes;

	dma_reader_get_avail_bytes(id, dev_runtime);

	if (((substream->f_flags & O_NONBLOCK) > 0) &&
		(request_size > dev_runtime->buffer.avail_size)) {
		pr_err("[ALSA DPB]Err! avail %u but request %u bytes\n",
				dev_runtime->buffer.avail_size, request_size);
		return -EAGAIN;
	}

	do {
		unsigned long receive_size = 0;

		size_to_copy = request_size - copied_size;
		if (size_to_copy > period_size)
			size_to_copy = period_size;

		/* Deliver PCM data */
		receive_size = copy_from_user(buffer_tmp, (buf + copied_size),
						size_to_copy);
		while (1) {
			size = dma_reader_write(id, dev_runtime,
					(void *)buffer_tmp, size_to_copy);
			if (size == 0) {
				if ((++retry_counter) > 500) {
					pr_info("[ALSA DPB]Retry write PCM\n");
					retry_counter = 0;
				}
				mtk_alsa_delaytask(1);
			} else
				break;
		}

		copied_size += size;
	} while (copied_size < request_size);

	return 0;
}

const struct snd_pcm_ops dplayback_pcm_ops = {
	.open = dplayback_pcm_open,
	.close = dplayback_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.pointer = dplayback_pcm_pointer,
	.copy_user = dplayback_pcm_copy,
};

static int earc_control_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = 16;
	return 0;
}

static int earc_get_param(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int earc_put_param(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int id = kcontrol->private_value;
	unsigned short temp_val = 0;
	int i;

	if (id == 0) {
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG19+1,
			ucontrol->value.bytes.data[3]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG18,
			ucontrol->value.bytes.data[4]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG18+1,
			ucontrol->value.bytes.data[5]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG17,
			ucontrol->value.bytes.data[6]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG17+1,
			ucontrol->value.bytes.data[7]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG16,
			ucontrol->value.bytes.data[8]);
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG16+1,
			ucontrol->value.bytes.data[9]);
	}

	if ((id == 4) || (id == 5) || (id == 6)) {
		mtk_alsa_write_reg_byte(REG.DPB_EARC_CFG34, id);
		mtk_alsa_write_reg(REG.DPB_EARC_CFG35, 0x0000);

		for (i = 0; i < 8; i++) {
			temp_val = (unsigned short)((ucontrol->value.bytes.data[i*2]<<8) |
				(ucontrol->value.bytes.data[(i*2)+1]&0xFF));
			mtk_alsa_write_reg((REG.DPB_EARC_CFG36 + (i*2)), temp_val);
		}
		//for (i = 0; i < 6; i++)
		//	mtk_alsa_write_reg((REG.DPB_EARC_CFG44 + (i*2)), 0x0000);
	}

	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG34+1, 0x01, 0x01);
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_CFG34+1, 0x01, 0x00);

	return 0;
}

static int earc_get_slt_param(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}


static int earc_put_slt_param(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	if (ucontrol->value.bytes.data[0] == 1)
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_SINE_GEN+1, 0x20, 0x20);
	else
		mtk_alsa_write_reg_mask_byte(REG.DPB_EARC_SINE_GEN+1, 0x20, 0x00);
	return 0;
}


static int dplayback_cs_control_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = 5;
	return 0;
}

static int dplayback_get_cs(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int dplayback_put_cs(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct dplayback_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int id = kcontrol->private_value;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct mtk_channel_status_struct *cs = &dev_runtime->channel_status;

	cs->status = 1;
	cs->cs_bytes[0] = ucontrol->value.bytes.data[0];
	cs->cs_bytes[1] = ucontrol->value.bytes.data[1];
	cs->cs_bytes[2] = ucontrol->value.bytes.data[2];
	cs->cs_bytes[3] = ucontrol->value.bytes.data[3];
	cs->cs_bytes[4] = ucontrol->value.bytes.data[4];

	return 0;
}

static const struct snd_kcontrol_new cs_controls[] = {
	SOC_CHAR_CHANNEL_STATUS("SPDIF PLAYBACK CHANNEL STATUS", SPDIF_PCM_PLAYBACK),
	SOC_CHAR_CHANNEL_STATUS("ARC PLAYBACK CHANNEL STATUS", ARC_PCM_PLAYBACK),
	SOC_CHAR_CHANNEL_STATUS("EARC PLAYBACK CHANNEL STATUS", EARC_PCM_PLAYBACK),
	SOC_CHAR_CHANNEL_STATUS("SPDIF NONPCM PLAYBACK CHANNEL STATUS", SPDIF_NONPCM_PLAYBACK),
	SOC_CHAR_CHANNEL_STATUS("ARC NONPCM PLAYBACK CHANNEL STATUS", ARC_NONPCM_PLAYBACK),
	SOC_CHAR_CHANNEL_STATUS("EARC NONPCM PLAYBACK CHANNEL STATUS", EARC_NONPCM_PLAYBACK),
	SOC_CHAR_EARC("EARC INFOFRAME", 0),
	SOC_CHAR_EARC("EARC ACP", 4),
	SOC_CHAR_EARC("EARC ISRC1", 5),
	SOC_CHAR_EARC("EARC ISRC2", 6),
	SOC_EARC_SLT("EARC PLAYBACK SLT SINEGEN TX", EARC_PCM_PLAYBACK),
};

static const struct snd_soc_component_driver dplayback_dai_component = {
	.name			= DPLAYBACK_DMA_NAME,
	.probe			= dplayback_probe,
	.remove			= dplayback_remove,
	.ops			= &dplayback_pcm_ops,
	.controls		= cs_controls,
	.num_controls		= ARRAY_SIZE(cs_controls),
};

unsigned int dpb_get_dts_u32(struct device_node *np, const char *name)
{
	unsigned int value;
	int ret;

	ret = of_property_read_u32(np, name, &value);
	if (ret)
		WARN_ON("[ALSA DPB]Can't get DTS property\n");

	return value;
}

static int parse_registers(void)
{
	struct device_node *np;

	memset(&REG, 0x0, sizeof(struct dplayback_register));

	np = of_find_node_by_name(NULL, "dplayback-register");
	if (!np)
		return -ENODEV;

	REG.DPB_DMA5_CTRL = dpb_get_dts_u32(np, "reg_dpb_dma5_ctrl");
	REG.DPB_DMA5_WPTR = dpb_get_dts_u32(np, "reg_dpb_dma5_wptr");
	REG.DPB_DMA5_DDR_LEVEL = dpb_get_dts_u32(np, "reg_dpb_dma5_ddr_level");
	REG.DPB_DMA6_CTRL = dpb_get_dts_u32(np, "reg_dpb_dma6_ctrl");
	REG.DPB_DMA6_WPTR = dpb_get_dts_u32(np, "reg_dpb_dma6_wptr");
	REG.DPB_DMA6_DDR_LEVEL = dpb_get_dts_u32(np, "reg_dpb_dma6_ddr_level");
	REG.DPB_DMA7_CTRL = dpb_get_dts_u32(np, "reg_dpb_dma7_ctrl");
	REG.DPB_DMA7_WPTR = dpb_get_dts_u32(np, "reg_dpb_dma7_wptr");
	REG.DPB_DMA7_DDR_LEVEL = dpb_get_dts_u32(np, "reg_dpb_dma7_ddr_level");

	REG.DPB_CS0 = dpb_get_dts_u32(np, "reg_digital_out_cs0");
	REG.DPB_CS1 = dpb_get_dts_u32(np, "reg_digital_out_cs1");
	REG.DPB_CS2 = dpb_get_dts_u32(np, "reg_digital_out_cs2");
	REG.DPB_CS3 = dpb_get_dts_u32(np, "reg_digital_out_cs3");
	REG.DPB_CS4 = dpb_get_dts_u32(np, "reg_digital_out_cs4");
	REG.DPB_SPDIF_CFG = dpb_get_dts_u32(np, "reg_digital_spdif_out_cfg");
	REG.DPB_ARC_CFG = dpb_get_dts_u32(np, "reg_digital_arc_out_cfg");
	REG.DPB_CS_TOGGLE = dpb_get_dts_u32(np, "reg_digital_out_cs_toggle");

	REG.DPB_SPDIF_SYNTH_H = dpb_get_dts_u32(np, "reg_digital_out_spdif_synth_h");
	REG.DPB_SPDIF_SYNTH_L = dpb_get_dts_u32(np, "reg_digital_out_spdif_synth_l");
	REG.DPB_ARC_SYNTH_H = dpb_get_dts_u32(np, "reg_digital_out_arc_synth_h");
	REG.DPB_ARC_SYNTH_L = dpb_get_dts_u32(np, "reg_digital_out_arc_synth_l");
	REG.DPB_SYNTH_CFG = dpb_get_dts_u32(np, "reg_digital_out_synth_cfg");

	REG.DPB_EARC_CFG00 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg00");
	REG.DPB_EARC_CFG01 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg01");
	REG.DPB_EARC_CFG02 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg02");
	REG.DPB_EARC_CFG03 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg03");
	REG.DPB_EARC_CFG04 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg04");
	REG.DPB_EARC_CFG05 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg05");
	REG.DPB_EARC_CFG06 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg06");
	REG.DPB_EARC_CFG07 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg07");
	REG.DPB_EARC_CFG08 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg08");
	REG.DPB_EARC_CFG09 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg09");
	REG.DPB_EARC_CFG10 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg10");
	REG.DPB_EARC_CFG11 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg11");
	REG.DPB_EARC_CFG12 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg12");
	REG.DPB_EARC_CFG13 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg13");
	REG.DPB_EARC_CFG14 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg14");
	REG.DPB_EARC_CFG15 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg15");
	REG.DPB_EARC_CFG16 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg16");
	REG.DPB_EARC_CFG17 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg17");
	REG.DPB_EARC_CFG18 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg18");
	REG.DPB_EARC_CFG19 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg19");
	REG.DPB_EARC_CFG20 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg20");
	REG.DPB_EARC_CFG21 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg21");
	REG.DPB_EARC_CFG22 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg22");
	REG.DPB_EARC_CFG23 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg23");
	REG.DPB_EARC_CFG24 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg24");
	REG.DPB_EARC_CFG25 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg25");
	REG.DPB_EARC_CFG26 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg26");
	REG.DPB_EARC_CFG27 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg27");
	REG.DPB_EARC_CFG28 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg28");
	REG.DPB_EARC_CFG29 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg29");
	REG.DPB_EARC_CFG30 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg30");
	REG.DPB_EARC_CFG31 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg31");
	REG.DPB_EARC_CFG32 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg32");
	REG.DPB_EARC_CFG33 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg33");
	REG.DPB_EARC_CFG34 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg34");
	REG.DPB_EARC_CFG35 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg35");
	REG.DPB_EARC_CFG36 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg36");
	REG.DPB_EARC_CFG37 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg37");
	REG.DPB_EARC_CFG38 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg38");
	REG.DPB_EARC_CFG39 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg39");
	REG.DPB_EARC_CFG40 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg40");
	REG.DPB_EARC_CFG41 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg41");
	REG.DPB_EARC_CFG42 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg42");
	REG.DPB_EARC_CFG43 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg43");
	REG.DPB_EARC_CFG44 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg44");
	REG.DPB_EARC_CFG45 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg45");
	REG.DPB_EARC_CFG46 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg46");
	REG.DPB_EARC_CFG47 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg47");
	REG.DPB_EARC_CFG48 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg48");
	REG.DPB_EARC_CFG49 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg49");
	REG.DPB_EARC_CFG50 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg50");
	REG.DPB_EARC_CFG51 = dpb_get_dts_u32(np, "reg_digital_earc_out_cfg51");
	REG.DPB_EARC_TX00 = dpb_get_dts_u32(np, "reg_digital_earc_out_tx00");
	REG.DPB_EARC_TX18 = dpb_get_dts_u32(np, "reg_digital_earc_out_tx18");
	REG.DPB_EARC_TX20 = dpb_get_dts_u32(np, "reg_digital_earc_out_tx20");
	REG.DPB_EARC_TX24 = dpb_get_dts_u32(np, "reg_digital_earc_out_tx24");
	REG.DPB_EARC_TX28 = dpb_get_dts_u32(np, "reg_digital_earc_out_tx28");
	REG.DPB_EARC_TX2C = dpb_get_dts_u32(np, "reg_digital_earc_out_tx2C");
	REG.DPB_EARC_TX30 = dpb_get_dts_u32(np, "reg_digital_earc_out_tx30");
	REG.DPB_EARC_SINE_GEN = dpb_get_dts_u32(np, "reg_dpb_earc_sine_gen");


	return 0;
}

static int parse_mmap(struct device_node *np,
				struct dplayback_priv *priv)
{
	struct of_mmap_info_data of_mmap_info = {};
	int ret;

	ret = of_mtk_get_reserved_memory_info_by_idx(np,
						0, &of_mmap_info);
	if (ret) {
		pr_err("[ALSA DPB]get audio mmap fail\n");
		return -EINVAL;
	}

	priv->mem_info.miu = of_mmap_info.miu;
	priv->mem_info.bus_addr = of_mmap_info.start_bus_address;
	priv->mem_info.buffer_size = of_mmap_info.buffer_size;

	return 0;
}

int parse_memory(struct device_node *np, struct dplayback_priv *priv)
{
	int ret;

	ret = of_property_read_u32(np, "spdif_dma_offset",
					&priv->spdif_dma_offset);
	if (ret) {
		pr_err("[ALSA DPB]can't get spdif dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "spdif_dma_size", &priv->spdif_dma_size);
	if (ret) {
		pr_err("[ALSA DPB]can't get spdif dma size\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "arc_dma_offset",
					&priv->arc_dma_offset);
	if (ret) {
		pr_err("[ALSA DPB]can't get arc dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "arc_dma_size", &priv->arc_dma_size);
	if (ret) {
		pr_err("[ALSA DPB]can't get arc dma size\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "earc_dma_offset",
					&priv->earc_dma_offset);
	if (ret) {
		pr_err("[ALSA DPB]can't get earc dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "earc_dma_size", &priv->earc_dma_size);
	if (ret) {
		pr_err("[ALSA DPB]can't get earc dma size\n");
		return -EINVAL;
	}

	return 0;
}

int dplayback_dev_probe(struct platform_device *pdev)
{
	struct dplayback_priv *priv;
	struct device *dev = &pdev->dev;
	struct device_node *node;
	int ret, i;

	node = dev->of_node;
	if (!node)
		return -ENODEV;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;

	for (i = 0; i < DPLAYBACK_NUM; i++) {
		priv->playback_dev[i] = devm_kzalloc(&pdev->dev,
				sizeof(struct mtk_pb_runtime_struct), GFP_KERNEL);
		if (!priv->playback_dev[i])
			return -ENOMEM;
	}

	platform_set_drvdata(pdev, priv);

	ret = devm_snd_soc_register_component(&pdev->dev,
					&dplayback_dai_component,
					dplayback_dais,
					ARRAY_SIZE(dplayback_dais));
	if (ret) {
		dev_err(dev, "[ALSA DPB]soc_register_component fail %d\n", ret);
		return ret;
	}

	/* parse dts */
	ret = of_property_read_u64(node, "memory_bus_base",
					&priv->mem_info.memory_bus_base);
	if (ret) {
		dev_err(dev, "[ALSA DPB]can't get miu bus base\n");
		return -EINVAL;
	}

	/* parse registers */
	ret = parse_registers();
	if (ret) {
		dev_err(dev, "[ALSA DPB]parse register fail %d\n", ret);
		return ret;
	}

	/* parse dma memory info */
	ret = parse_memory(node, priv);
	if (ret) {
		dev_err(dev, "[ALSA DPB]parse memory fail %d\n", ret);
		return ret;
	}
	/* parse mmap */
	ret = parse_mmap(node, priv);
	if (ret) {
		dev_err(dev, "[ALSA DPB]parse mmap fail %d\n", ret);
		return ret;
	}

	return 0;
}

int dplayback_dev_remove(struct platform_device *pdev)
{
	return 0;
}

MODULE_DESCRIPTION("Mediatek ALSA SoC DPlayback DMA platform driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:dplayback");
