/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk_alsa_playback_platform.h  --  Mediatek DTV playback header
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */
#ifndef _PLAYBACK_DMA_PLATFORM_HEADER
#define _PLAYBACK_DMA_PLATFORM_HEADER

#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>
#include <sound/soc.h>

#include "mtk_alsa_pcm_common.h"

#define AUD_PR_PRINT_PB(audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case SPEAKER_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[SPEAKER_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][SPK][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][SPK][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][SPK][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][SPK][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][SPK][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][SPK][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][SPK][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][SPK][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case HEADPHONE_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[HEADPHONE_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][HP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][HP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][HP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][HP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][HP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][HP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][HP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][HP][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case LINEOUT_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[LINEOUT_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][LINE][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][LINE][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][LINE][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][LINE][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][LINE][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][LINE][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][LINE][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][LINE][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case HFP_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[HFP_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][HFP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][HFP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][HFP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][HFP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][HFP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][HFP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][HFP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][HFP][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case PLAYBACK_NUM: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][PB][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_alert("[AUDIO][PB][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_crit("[AUDIO][PB][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_err("[AUDIO][PB][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_warn("[AUDIO][PB][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_notice("[AUDIO][PB][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_info("[AUDIO][PB][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_debug("[AUDIO][PB][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

#define AUD_DEV_PRINT_PB(audio_dev, audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case SPEAKER_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[SPEAKER_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][SPK][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][SPK][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][SPK][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][SPK][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][SPK][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][SPK][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][SPK][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][SPK][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case HEADPHONE_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[HEADPHONE_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][HP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][HP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][HP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][HP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][HP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][HP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][HP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][HP][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case LINEOUT_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[LINEOUT_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case HFP_PLAYBACK: \
	if (audio_log_level <= (audio_priv->playback_dev[HFP_PLAYBACK]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][HFP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][HFP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][HFP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][HFP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][HFP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][HFP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][HFP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][LINE][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case PLAYBACK_NUM: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][PB][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_alert(audio_dev, "[AUDIO][PB][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_crit(audio_dev, "[AUDIO][PB][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_err(audio_dev, "[AUDIO][PB][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_warn(audio_dev, "[AUDIO][PB][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_notice(audio_dev, "[AUDIO][PB][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_info(audio_dev, "[AUDIO][PB][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_dbg(audio_dev, "[AUDIO][PB][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

enum {
	SPEAKER_PLAYBACK = 0,
	HEADPHONE_PLAYBACK,
	LINEOUT_PLAYBACK,
	HFP_PLAYBACK,
	PLAYBACK_NUM,
	I2S0_PLAYBACK,
	I2S5_PLAYBACK,
	DAC0_PLAYBACK,
	DAC1_PLAYBACK,
	PLAYBACK_DEVICE_END,
};

enum {
	AUDIO_OUT_NULL = 0,
	AUDIO_OUT_DAC0,
	AUDIO_OUT_DAC1,
	AUDIO_OUT_DAC2,
	AUDIO_OUT_DAC3,
	AUDIO_OUT_I2S0,
	AUDIO_OUT_I2S1,
	AUDIO_OUT_I2S5,
};

/* Define a Ring Buffer data structure */
struct mtk_pb_ring_buffer_struct {
	unsigned char *addr;
	unsigned int size;
	unsigned char *w_ptr;
	unsigned char *r_ptr;
};

/* Define a DMA Reader data structure */
struct mtk_dma_reader_struct {
	struct mtk_pb_ring_buffer_struct buffer;
	struct mtk_str_mode_struct str_mode_info;
	unsigned int initialized_status;
	unsigned int period_size;
	unsigned int high_threshold;
	unsigned int written_size;
	unsigned int status;
};

/* Define a Ring Buffer data structure */
struct mtk_device_buffer_struct {
	unsigned char *addr;
	unsigned int size;
	unsigned int avail_size;
	unsigned int inused_size;
	unsigned int consumed_size;
};

struct mtk_inject_struct {
	bool status;
	unsigned int r_ptr;
};

struct mtk_channel_status_struct {
	bool status;
	unsigned char cs_bytes[24];
};

/* Define a Runtime data structure */
struct mtk_pb_runtime_struct {
	struct snd_pcm_hw_constraint_list constraints_rates;
	struct mtk_substream_struct substreams;
	struct mtk_device_buffer_struct buffer;
	struct mtk_dma_reader_struct pcm_playback;
	struct mtk_monitor_struct monitor;
	struct mtk_inject_struct inject;
	struct mtk_channel_status_struct channel_status;
	struct timer_list timer;
	spinlock_t spin_lock;
	unsigned int channel_mode;
	unsigned int byte_length;
	unsigned int sample_rate;
	unsigned int runtime_status;
	unsigned int device_status;
	snd_pcm_format_t format;
	unsigned int log_level;
	unsigned int clk_status;
	unsigned int chg_finished;
	unsigned int sink_dev;
	void *private;
	int id;
	unsigned int suspend_flag;
};

struct playback_register {
	u32 PB_DMA1_CTRL;
	u32 PB_DMA1_WPTR;
	u32 PB_DMA1_DDR_LEVEL;
	u32 PB_DMA2_CTRL;
	u32 PB_DMA2_WPTR;
	u32 PB_DMA2_DDR_LEVEL;
	u32 PB_DMA3_CTRL;
	u32 PB_DMA3_WPTR;
	u32 PB_DMA3_DDR_LEVEL;
	u32 PB_DMA4_CTRL;
	u32 PB_DMA4_WPTR;
	u32 PB_DMA4_DDR_LEVEL;
	u32 PB_CHG_FINISHED;
	u32 PB_HP_OPENED;
	u32 PB_I2S_BCK_WIDTH;
};

struct playback_priv {
	struct device *dev;
	struct mem_info mem_info;
	struct mtk_pb_runtime_struct *playback_dev[PLAYBACK_NUM];
	struct snd_soc_component *component;
	u32 spk_dma_offset;
	u32 spk_dma_size;
	u32 hp_dma_offset;
	u32 hp_dma_size;
	u32 lineout_dma_offset;
	u32 lineout_dma_size;
	u32 bt_dma_offset;
	u32 bt_dma_size;
	u32 hfp_enable_flag;
};

int playback_dev_probe(struct platform_device *pdev);
int playback_dev_remove(struct platform_device *pdev);

#endif /* _PLAYBACK_DMA_PLATFORM_HEADER */
