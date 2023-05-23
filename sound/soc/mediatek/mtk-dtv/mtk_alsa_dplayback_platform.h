/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * mtk_alsa_dplayback_platform.h  --  Mediatek DTV dplayback header
 *
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DPLAYBACK_DMA_PLATFORM_HEADER
#define _DPLAYBACK_DMA_PLATFORM_HEADER

#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>
#include <sound/soc.h>

#include "mtk_alsa_pcm_common.h"
#include "mtk_alsa_playback_platform.h"


#define AUD_PR_PRINT_DPB(audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case SPDIF_PCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[SPDIF_PCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][SPDIF_PCM][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][SPDIF_PCM][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][SPDIF_PCM][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][SPDIF_PCM][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][SPDIF_PCM][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][SPDIF_PCM][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][SPDIF_PCM][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][SPDIF_PCM][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case SPDIF_NONPCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][SPDIF_NON][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][SPDIF_NON][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][SPDIF_NON][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][SPDIF_NON][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][SPDIF_NON][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][SPDIF_NON][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][SPDIF_NON][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][SPDIF_NON][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ARC_PCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[ARC_PCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][ARC_PCM][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][ARC_PCM][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][ARC_PCM][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][ARC_PCM][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][ARC_PCM][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][ARC_PCM][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][ARC_PCM][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][ARC_PCM][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ARC_NONPCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[ARC_NONPCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][ARC_NON][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][ARC_NON][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][ARC_NON][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][ARC_NON][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][ARC_NON][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][ARC_NON][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][ARC_NON][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][ARC_NON][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case DPLAYBACK_NUM: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][ARC_NON][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_alert("[AUDIO][ARC_NON][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_crit("[AUDIO][ARC_NON][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_err("[AUDIO][ARC_NON][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_warn("[AUDIO][ARC_NON][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_notice("[AUDIO][ARC_NON][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_info("[AUDIO][ARC_NON][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_debug("[AUDIO][ARC_NON][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

#define AUD_DEV_PRINT_DPB(audio_dev, audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case SPDIF_PCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[SPDIF_PCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_PCM][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_PCM][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_PCM][CRIT]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_PCM][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_PCM][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_PCM][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_PCM][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_PCM][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case SPDIF_NONPCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[SPDIF_NONPCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_NON][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_NON][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_NON][CRIT]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_NON][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_NON][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_NON][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_NON][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][SPDIF_NON][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ARC_PCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[ARC_PCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_PCM][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_PCM][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_PCM][CRITI]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_PCM][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_PCM][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_PCM][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_PCM][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_PCM][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ARC_NONPCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[ARC_NONPCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][CRITI]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case EARC_PCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[EARC_PCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_PCM][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_PCM][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_PCM][CRITI]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_PCM][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_PCM][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_PCM][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_PCM][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_PCM][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case EARC_NONPCM_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[EARC_NONPCM_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_NON][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_NON][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_NON][CRITI]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_NON][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_NON][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_NON][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_NON][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][EARC_NON][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case DPLAYBACK_NUM: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][ARC_NON][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_alert(audio_dev, "[AUDIO][ARC_NON][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_crit(audio_dev, "[AUDIO][ARC_NON][CRITI]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_err(audio_dev, "[AUDIO][ARC_NON][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_warn(audio_dev, "[AUDIO][ARC_NON][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_notice(audio_dev, "[AUDIO][ARC_NON][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_info(audio_dev, "[AUDIO][ARC_NON][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_dbg(audio_dev, "[AUDIO][ARC_NON][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

#define SOC_CHAR_CHANNEL_STATUS(xname, xvalue) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name = xname, \
	.info = dplayback_cs_control_info, \
	.get = dplayback_get_cs, .put = dplayback_put_cs, \
	.private_value = (unsigned short)xvalue }

#define SOC_CHAR_EARC(xname, xvalue) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name = xname, \
	.info = earc_control_info, \
	.get = earc_get_param, .put = earc_put_param, \
	.private_value = (unsigned short)xvalue }

#define SOC_EARC_SLT(xname, xvalue) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name = xname, \
	.info = earc_control_info, \
	.get = earc_get_slt_param, .put = earc_put_slt_param, \
	.private_value = (unsigned short)xvalue }


enum {
	SPDIF_PCM_PLAYBACK = 0,
	SPDIF_NONPCM_PLAYBACK,
	ARC_PCM_PLAYBACK,
	ARC_NONPCM_PLAYBACK,
	EARC_PCM_PLAYBACK,
	EARC_NONPCM_PLAYBACK,
	DPLAYBACK_NUM,
};

struct dplayback_register {
	u32 DPB_DMA5_CTRL;
	u32 DPB_DMA5_WPTR;
	u32 DPB_DMA5_DDR_LEVEL;
	u32 DPB_DMA6_CTRL;
	u32 DPB_DMA6_WPTR;
	u32 DPB_DMA6_DDR_LEVEL;
	u32 DPB_DMA7_CTRL;
	u32 DPB_DMA7_WPTR;
	u32 DPB_DMA7_DDR_LEVEL;
	u32 DPB_EARC_SINE_GEN;
	u32 DPB_CS0;
	u32 DPB_CS1;
	u32 DPB_CS2;
	u32 DPB_CS3;
	u32 DPB_CS4;
	u32 DPB_SPDIF_CFG;
	u32 DPB_ARC_CFG;
	u32 DPB_CS_TOGGLE;
	u32 DPB_SPDIF_SYNTH_H;
	u32 DPB_SPDIF_SYNTH_L;
	u32 DPB_ARC_SYNTH_H;
	u32 DPB_ARC_SYNTH_L;
	u32 DPB_SYNTH_CFG;
	u32 DPB_EARC_CFG00;
	u32 DPB_EARC_CFG01;
	u32 DPB_EARC_CFG02;
	u32 DPB_EARC_CFG03;
	u32 DPB_EARC_CFG04;
	u32 DPB_EARC_CFG05;
	u32 DPB_EARC_CFG06;
	u32 DPB_EARC_CFG07;
	u32 DPB_EARC_CFG08;
	u32 DPB_EARC_CFG09;
	u32 DPB_EARC_CFG10;
	u32 DPB_EARC_CFG11;
	u32 DPB_EARC_CFG12;
	u32 DPB_EARC_CFG13;
	u32 DPB_EARC_CFG14;
	u32 DPB_EARC_CFG15;
	u32 DPB_EARC_CFG16;
	u32 DPB_EARC_CFG17;
	u32 DPB_EARC_CFG18;
	u32 DPB_EARC_CFG19;
	u32 DPB_EARC_CFG20;
	u32 DPB_EARC_CFG21;
	u32 DPB_EARC_CFG22;
	u32 DPB_EARC_CFG23;
	u32 DPB_EARC_CFG24;
	u32 DPB_EARC_CFG25;
	u32 DPB_EARC_CFG26;
	u32 DPB_EARC_CFG27;
	u32 DPB_EARC_CFG28;
	u32 DPB_EARC_CFG29;
	u32 DPB_EARC_CFG30;
	u32 DPB_EARC_CFG31;
	u32 DPB_EARC_CFG32;
	u32 DPB_EARC_CFG33;
	u32 DPB_EARC_CFG34;
	u32 DPB_EARC_CFG35;
	u32 DPB_EARC_CFG36;
	u32 DPB_EARC_CFG37;
	u32 DPB_EARC_CFG38;
	u32 DPB_EARC_CFG39;
	u32 DPB_EARC_CFG40;
	u32 DPB_EARC_CFG41;
	u32 DPB_EARC_CFG42;
	u32 DPB_EARC_CFG43;
	u32 DPB_EARC_CFG44;
	u32 DPB_EARC_CFG45;
	u32 DPB_EARC_CFG46;
	u32 DPB_EARC_CFG47;
	u32 DPB_EARC_CFG48;
	u32 DPB_EARC_CFG49;
	u32 DPB_EARC_CFG50;
	u32 DPB_EARC_CFG51;
	u32 DPB_EARC_TX00;
	u32 DPB_EARC_TX18;
	u32 DPB_EARC_TX20;
	u32 DPB_EARC_TX24;
	u32 DPB_EARC_TX28;
	u32 DPB_EARC_TX2C;
	u32 DPB_EARC_TX30;
};

struct dplayback_priv {
	struct device *dev;
	struct mem_info mem_info;
	struct mtk_pb_runtime_struct *playback_dev[DPLAYBACK_NUM];
	u32 spdif_dma_offset;
	u32 spdif_dma_size;
	u32 arc_dma_offset;
	u32 arc_dma_size;
	u32 earc_dma_offset;
	u32 earc_dma_size;
	int ip_version;
	int dp_pcm_flag;
	int dp_np_flag;
};

int dplayback_dev_probe(struct platform_device *pdev);
int dplayback_dev_remove(struct platform_device *pdev);

extern void Chip_Flush_Memory(void);

#endif /* _DPLAYBACK_DMA_PLATFORM_HEADER */
