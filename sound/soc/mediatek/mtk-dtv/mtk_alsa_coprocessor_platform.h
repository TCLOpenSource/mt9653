/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk_alsa_coprocessor_platform.h  --  Mediatek DTV coprocessor header
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#ifndef _COPROCESSOR_PLATFORM_HEADER
#define _COPROCESSOR_PLATFORM_HEADER

#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>

#include "mtk_alsa_pcm_common.h"

enum {
	AU_SE_SYSTEM = 0,
	AU_DVB2_NONE,
	AU_SIF,
	AU_CODE_SEGMENT_MAX,
};

enum {
	DSP_MAIN_COUNTER = 0,
	DSP_ISR_COUNTER,
	DSP_DISABLE,
	DSP_REBOOT,
	DSP_END,
};


#define AUD_PR_PRINT_COP(audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case COPRO_DEVICE: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][PB][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg("[AUDIO][PB][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg("[AUDIO][PB][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg("[AUDIO][PB][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg("[AUDIO][PB][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg("[AUDIO][PB][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg("[AUDIO][PB][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg("[AUDIO][PB][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

#define AUD_DEV_PRINT_COP(audio_dev, audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case COPRO_DEVICE: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][COP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][COP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][COP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][COP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][COP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][COP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][COP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][COP][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

enum {
	COPRO_DEVICE,
	COPRO_DEVICE_END,
};

#define MST_CODEC_PM1_FLASH_ADDR                  0x00000000
#define MST_CODEC_PM2_FLASH_ADDR                  0x00001ff8
#define MST_CODEC_PM3_FLASH_ADDR                  0x00002190
#define MST_CODEC_PM4_FLASH_ADDR                  0x00003c48
#define MST_CODEC_DEC_PM1_FLASH_ADDR              0x00003c60
#define MST_CODEC_DEC_PM2_FLASH_ADDR              0x00003c78
#define MST_CODEC_DEC_PM3_FLASH_ADDR              0x00003ca8
#define MST_CODEC_DEC_PM4_FLASH_ADDR              0x00003cc0
#define MST_CODEC_SIF_PM1_FLASH_ADDR              0x00003cd8
#define MST_CODEC_SIF_PM2_FLASH_ADDR              0x00003d80
#define MST_CODEC_SIF_PM3_FLASH_ADDR              0x00005418

#define MST_CODEC_PM1_ADDR                        0x00000000
#define MST_CODEC_PM1_SIZE                        0x00001fef
#define MST_CODEC_PM2_ADDR                        0x00001f40
#define MST_CODEC_PM2_SIZE                        0x00000198
#define MST_CODEC_PM3_ADDR                        0x00008000
#define MST_CODEC_PM3_SIZE                        0x00001ab2
#define MST_CODEC_PM4_ADDR                        0x0000c000
#define MST_CODEC_PM4_SIZE                        0x00000003
#define MST_CODEC_DEC_PM1_ADDR                    0x00001f00
#define MST_CODEC_DEC_PM1_SIZE                    0x00000012
#define MST_CODEC_DEC_PM2_ADDR                    0x00002040
#define MST_CODEC_DEC_PM2_SIZE                    0x00000021
#define MST_CODEC_DEC_PM3_ADDR                    0x00009000
#define MST_CODEC_DEC_PM3_SIZE                    0x00000003
#define MST_CODEC_DEC_PM4_ADDR                    0x0000c800
#define MST_CODEC_DEC_PM4_SIZE                    0x00000012
#define MST_CODEC_SIF_PM1_ADDR                    0x00001f00
#define MST_CODEC_SIF_PM1_SIZE                    0x00000096
#define MST_CODEC_SIF_PM2_ADDR                    0x00002040
#define MST_CODEC_SIF_PM2_SIZE                    0x00001698
#define MST_CODEC_SIF_PM3_ADDR                    0x00009000
#define MST_CODEC_SIF_PM3_SIZE                    0x0000722d

struct dsp_alg_info {
	unsigned int	cm_addr;
	unsigned int	cm_len;
	unsigned char	*cm_buf;

	unsigned int	pm_addr;
	unsigned int	pm_len;
	unsigned char	*pm_buf;

	unsigned int	cache_addr;
	unsigned int	cache_len;
	unsigned char	*cache_buf;

	unsigned int	prefetch_addr;
	unsigned int	prefetch_len;
	unsigned char	*prefetch_buf;
};

struct copr_register {
	u32 DEC_AUD_CTRL;
	u32 DEC_MAD_OFFSET_BASE_L;
	u32 DEC_MAD_OFFSET_BASE_H;
	u32 DEC_MAD_OFFSET_BASE_EXT;
	u32 SE_AUD_CTRL;
	u32 SE_MAD_OFFSET_BASE_L;
	u32 SE_MAD_OFFSET_BASE_H;
	u32 SE_MAD_OFFSET_BASE_EXT;
	u32 SE_IDMA_CTRL0;
	u32 SE_DSP_ICACHE_BASE_L;
	u32 SE_BDMA_CFG;
	u32 FD230_SEL;
	u32 SE_DSP_BRG_DATA_L;
	u32 SE_DSP_BRG_DATA_H;
	u32 SE_IDMA_WRBASE_ADDR_L;
	u32 RIU_MAIL_00;
	u32 DSP_POWER_DOWN_H;
	u32 DSP_DBG_CMD1;
	u32 DSP_ISR_CNTR;
	u32 DSP_WHILE1_CNTR;
	u32 DSP_DBG_RESULT1;
};

struct copr_priv {
	struct device	*dev;
	struct mem_info	mem_info;
	const char	*fw_name;
	unsigned int	fw_size;
	unsigned int	log_level;
	struct snd_soc_component *component;
	bool		suspended;
};

#endif /* _COPROCESSOR_PLATFORM_HEADER */
