/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk_alsa_pcm_platform.h  --  Mediatek PCM device header
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */
#ifndef _PCM_PLATFORM_HEADER
#define _PCM_PLATFORM_HEADER

#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/interrupt.h>

#include "mtk_alsa_pcm_common.h"

#define SOC_INTEGER_SRC(xname, xvalue) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name = xname, \
	.info = src_control_info, \
	.get = src_control_get_value, .put = src_control_put_value, \
	.private_value = (unsigned short)xvalue }

#define SOC_INTEGER_CAP(xname, xvalue) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name = xname, \
	.info = cap_control_info, \
	.get = cap_control_get_value, .put = cap_control_put_value, \
	.private_value = (unsigned short)xvalue }

#define SOC_TIMESTAMP_CAP(xname, xvalue) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name = xname, \
	.info = cap_timestamp_info, \
	.get = cap_timestamp_get_value, .put = cap_timestamp_put_value, \
	.private_value = (unsigned short)xvalue }

#define SOC_INTEGER_ATV(xname, xvalue) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
	.name = xname, \
	.info = atv_control_info, \
	.get = atv_control_get_value, .put = atv_control_put_value, \
	.private_value = (unsigned short)xvalue }

#define AUD_PR_PRINT_CAP(audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case HDMI_RX1_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[HDMI_RX1_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][HDMI1][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][HDMI1][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][HDMI1][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][HDMI1][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][HDMI1][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][HDMI1][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][HDMI1][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][HDMI1][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case HDMI_RX2_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[HDMI_RX2_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][HDMI2][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][HDMI2][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][HDMI2][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][HDMI2][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][HDMI2][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][HDMI2][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][HDMI2][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][HDMI2][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ATV_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[ATV_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][ATV][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][ATV][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][ATV][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][ATV][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][ATV][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][ATV][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][ATV][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][ATV][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ADC_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[ADC_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][ADC][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][ADC][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][ADC][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][ADC][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][ADC][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][ADC][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][ADC][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][ADC][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ADC1_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[ADC1_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][ADC1][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][ADC1][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][ADC1][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][ADC1][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][ADC1][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][ADC1][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][ADC1][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][ADC1][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case I2S_RX2_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[I2S_RX2_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][I2S-RX2][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][I2S-RX2][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][I2S-RX2][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][I2S-RX2][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][I2S-RX2][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][I2S-RX2][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][I2S-RX2][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][I2S-RX2][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case LOOPBACK_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[LOOPBACK_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][LOOPBACK][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][LOOPBACK][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][LOOPBACK][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][LOOPBACK][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][LOOPBACK][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][LOOPBACK][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][LOOPBACK][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][LOOPBACK][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case HFP_RX_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[HFP_RX_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][HFP-RX][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][HFP-RX][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][HFP-RX][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][HFP-RX][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][HFP-RX][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][HFP-RX][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][HFP-RX][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][HFP-RX][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case CAP_DEV_NUM: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][CAP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_alert("[AUDIO][CAP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_crit("[AUDIO][CAP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_err("[AUDIO][CAP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_warn("[AUDIO][CAP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_notice("[AUDIO][CAP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_info("[AUDIO][CAP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_debug("[AUDIO][CAP][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

#define AUD_DEV_PRINT_CAP(audio_dev, audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case HDMI_RX1_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[HDMI_RX1_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI1][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI1][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI1][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI1][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI1][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI1][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI1][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI1][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case HDMI_RX2_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[HDMI_RX2_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI2][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI2][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI2][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI2][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI2][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI2][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI2][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][HDMI2][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ATV_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[ATV_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][ATV][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][ATV][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][ATV][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][ATV][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][ATV][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][ATV][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][ATV][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][ATV][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ADC_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[ADC_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][ADC][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][ADC][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][ADC][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][ADC][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][ADC][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][ADC][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][ADC][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][ADC][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case ADC1_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[ADC1_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][ADC1][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][ADC1][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][ADC1][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][ADC1][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][ADC1][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][ADC1][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][ADC1][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][ADC1][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case I2S_RX2_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[I2S_RX2_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][I2S-RX2][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][I2S-RX2][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][I2S-RX2][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][I2S-RX2][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][I2S-RX2][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][I2S-RX2][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][I2S-RX2][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][I2S-RX2][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case LOOPBACK_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[LOOPBACK_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][LOOPBACK][EMERG]" fmt, ## args); \
			break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][LOOPBACK][ALERT]" fmt, ## args); \
			break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][LOOPBACK][CRITICAL]" fmt, ## args); \
			break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][LOOPBACK][ERROR]" fmt, ## args); \
			break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][LOOPBACK][WARN]" fmt, ## args); \
			break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][LOOPBACK][NOTICE]" fmt, ## args); \
			break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][LOOPBACK][INFO]" fmt, ## args); \
			break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][LOOPBACK][DEBUG]" fmt, ## args); \
			break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case HFP_RX_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[HFP_RX_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][HFP-RX][EMERG]" fmt, ## args); \
			break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][HFP-RX][ALERT]" fmt, ## args); \
			break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][HFP-RX][CRITICAL]" fmt, ## args); \
			break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][HFP-RX][ERROR]" fmt, ## args); \
			break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][HFP-RX][WARN]" fmt, ## args); \
			break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][HFP-RX][NOTICE]" fmt, ## args); \
			break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][HFP-RX][INFO]" fmt, ## args); \
			break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][HFP-RX][DEBUG]" fmt, ## args); \
			break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case CAP_DEV_NUM: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][CAP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_alert(audio_dev, "[AUDIO][CAP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_crit(audio_dev, "[AUDIO][CAP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_err(audio_dev, "[AUDIO][CAP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_warn(audio_dev, "[AUDIO][CAP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_notice(audio_dev, "[AUDIO][CAP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_info(audio_dev, "[AUDIO][CAP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_dbg(audio_dev, "[AUDIO][CAP][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

#define AUD_PR_PRINT_SRC(audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case SRC1_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[SRC1_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][SRC1][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][SRC1][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][SRC1][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][SRC1][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][SRC1][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][SRC1][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][SRC1][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][SRC1][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case SRC2_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[SRC2_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][SRC2][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][SRC2][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][SRC2][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][SRC2][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][SRC2][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][SRC2][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][SRC2][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][SRC2][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case SRC3_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[SRC3_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][SRC3][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_emerg("[AUDIO][SRC3][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_emerg("[AUDIO][SRC3][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_emerg("[AUDIO][SRC3][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_emerg("[AUDIO][SRC3][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_emerg("[AUDIO][SRC3][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_emerg("[AUDIO][SRC3][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_emerg("[AUDIO][SRC3][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case DUPLEX_DEV_NUM: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][DUP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_alert("[AUDIO][DUP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_crit("[AUDIO][DUP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_err("[AUDIO][DUP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_warn("[AUDIO][DUP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_notice("[AUDIO][DUP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_info("[AUDIO][DUP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_debug("[AUDIO][DUP][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

#define AUD_DEV_PRINT_SRC(audio_dev, audio_module_id, audio_log_level, audio_priv, fmt, args...) \
do { \
	switch (audio_module_id) { \
	case SRC1_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[SRC1_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][SRC1][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC1][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC1][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][SRC1][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][SRC1][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC1][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][SRC1][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][SRC1][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case SRC2_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[SRC2_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][SRC2][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC2][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC2][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][SRC2][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][SRC2][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC2][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][SRC2][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][SRC2][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case SRC3_DEV: \
	if (audio_log_level <= (audio_priv->pcm_device[SRC3_DEV]->log_level)) { \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{dev_emerg(audio_dev, "[AUDIO][SRC3][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC3][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC3][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{dev_emerg(audio_dev, "[AUDIO][SRC3][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{dev_emerg(audio_dev, "[AUDIO][SRC3][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{dev_emerg(audio_dev, "[AUDIO][SRC3][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{dev_emerg(audio_dev, "[AUDIO][SRC3][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{dev_emerg(audio_dev, "[AUDIO][SRC3][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	} \
	break; \
	case DUPLEX_DEV_NUM: \
		switch (audio_log_level) { \
		case AUD_EMERG: \
			{pr_emerg("[AUDIO][DUP][EMERG]" fmt, ## args); break; } \
		case AUD_ALERT: \
			{pr_alert("[AUDIO][DUP][ALERT]" fmt, ## args); break; } \
		case AUD_CRIT: \
			{pr_crit("[AUDIO][DUP][CRITICAL]" fmt, ## args); break; } \
		case AUD_ERR: \
			{pr_err("[AUDIO][DUP][ERROR]" fmt, ## args); break; } \
		case AUD_WARN: \
			{pr_warn("[AUDIO][DUP][WARN]" fmt, ## args); break; } \
		case AUD_NOT: \
			{pr_notice("[AUDIO][DUP][NOTICE]" fmt, ## args); break; } \
		case AUD_INFO: \
			{pr_info("[AUDIO][DUP][INFO]" fmt, ## args); break; } \
		case AUD_DEB: \
			{pr_debug("[AUDIO][DUP][DEBUG]" fmt, ## args); break; } \
		default: \
			break; \
		} \
	break; \
	} \
} while (0)

enum {
	E_PLAYBACK,
	E_CAPTURE,
};

enum {
	SRC1_DEV,
	SRC2_DEV,
	SRC3_DEV,
	DUPLEX_DEV_NUM,
};

enum src_kcontrol_cmd {
	SRC1_RATE,
	SRC1_PB_BUF_LEVEL,    // unit: bytes
	SRC1_CAP_BUF_LEVEL,    // unit: bytes

	SRC2_RATE,
	SRC2_PB_BUF_LEVEL,    // unit: bytes
	SRC2_CAP_BUF_LEVEL,    // unit: bytes

	SRC3_RATE,
	SRC3_PB_BUF_LEVEL,    // unit: bytes
	SRC3_CAP_BUF_LEVEL,    // unit: bytes
};

enum {
	HDMI_RX1_DEV,
	HDMI_RX2_DEV,
	ATV_DEV,
	ADC_DEV,
	ADC1_DEV,
	I2S_RX2_DEV,
	LOOPBACK_DEV,
	HFP_RX_DEV,
	CAP_DEV_NUM,
};

enum atv_decode_system {
	ATV_SYS_PALSUM,
	ATV_SYS_BTSC,
	ATV_SYS_NUM,
};

enum atv_sub_std {
	ATV_SUB_STD_MONO,
	ATV_SUB_STD_HIDEV,
	ATV_SUB_STD_A2,
	ATV_SUB_STD_NICAM,
	ATV_SUB_STD_INVALID		= 0xFF,
};

enum atv_kcontrol_cmd {
	ATV_HIDEV,
	ATV_PRESCALE,
	ATV_STANDARD,
	ATV_SOUNDMODE,
	ATV_THRESHOLD,
	ATV_CARRIER_STATUS,
	ATV_AUTO_DET_STD_RESULT,
	ATV_SYS_CAPABILITY,

	// Palsum audio signal level
	ATV_PAL_SC1_AMP,
	ATV_PAL_SC1_NSR,
	ATV_PAL_SC2_AMP,
	ATV_PAL_SC2_NSR,
	ATV_PAL_PILOT_AMP,
	ATV_PAL_PILOT_PHASE,
	ATV_PAL_STEREO_DUAL_ID,
	ATV_PAL_NICAM_PHASE_ERR,
	ATV_PAL_BAND_STRENGTH,

	// Btsc audio signal level
	ATV_BTSC_SC1_AMP,
	ATV_BTSC_SC1_NSR,
	ATV_BTSC_PILOT_AMP,
	ATV_BTSC_SAP_AMP,
	ATV_BTSC_SAP_NSR,
};

enum atv_standard_type {
	ATV_STD_NOT_READY		= 0x0,
	ATV_STD_M_BTSC			= 0x1,
	ATV_STD_M_EIAJ			= 0x2,
	ATV_STD_M_A2			= 0x3,
	ATV_STD_BG_A2			= 0x4,
	ATV_STD_DK1_A2			= 0x5,
	ATV_STD_DK2_A2			= 0x6,
	ATV_STD_DK3_A2			= 0x7,
	ATV_STD_BG_NICAM		= 0x8,
	ATV_STD_DK_NICAM		= 0x9,
	ATV_STD_I_NICAM			= 0xA,
	ATV_STD_L_NICAM			= 0xB,
	ATV_STD_FM_RADIO		= 0xC,
	ATV_STD_BUSY			= 0x80,
	ATV_STD_SEL_AUTO		= 0xE0,
	ATV_STD_MAX,
};

enum {
	ATV_MAIN_STANDARD,
	ATV_SUB_STANDARD,
};

enum atv_carrier_status {
	ATV_NO_CARRIER			= 0x00, ///< No carrier detect
	ATV_PRIMARY_CARRIER		= 0x01, ///< Carrier 1 exist
	ATV_SECONDARY_CARRIER		= 0x02, ///< Carrier 2 exist
	ATV_NICAM			= 0x04, ///< Nicam lock state
	ATV_STEREO			= 0x08, ///< A2 Stereo exist
	ATV_BILINGUAL			= 0x10, ///< A2 Dual exist
	ATV_PILOT			= 0x20, ///< A2 Pilot exist
	ATV_DK2				= 0x40, ///< Sound standard is DK2
	ATV_DK3				= 0x80  ///< Sound standard is DK3
};

enum atv_sound_mode_detect {
	ATV_SOUND_MODE_DET_UNKNOWN,		///< unknown
	ATV_SOUND_MODE_DET_MONO,		///< MTS/A2/EIAJ/FM-Mono mono
	ATV_SOUND_MODE_DET_STEREO,		///< MTS/A2/EIAJ stereo
	ATV_SOUND_MODE_DET_MONO_SAP,		///< MTS mono + SAP
	ATV_SOUND_MODE_DET_STEREO_SAP,		///< MTS stereo + SAP
	ATV_SOUND_MODE_DET_DUAL,		///< A2/EIAJ dual 1
	ATV_SOUND_MODE_DET_NICAM_MONO,		///< NICAM mono (with FM/AM mono)
	ATV_SOUND_MODE_DET_NICAM_STEREO,	///< NICAM stereo (with FM/AM mono)
	ATV_SOUND_MODE_DET_NICAM_DUAL,		///< NICAM dual (with FM/AM mono)
};

enum atv_sound_mode {
	ATV_SOUND_MODE_UNKNOWN,			///< unknown
	ATV_SOUND_MODE_MONO,			///< MTS/A2/EIAJ/FM-Mono mono
	ATV_SOUND_MODE_STEREO,			///< MTS/A2/EIAJ stereo
	ATV_SOUND_MODE_SAP,			///< MTS SAP
	ATV_SOUND_MODE_A2_LANG_A,		///< A2/EIAJ dual A
	ATV_SOUND_MODE_A2_LANG_B,		///< A2/EIAJ dual B
	ATV_SOUND_MODE_A2_LANG_AB,		///< A2/EIAJ L:dual A, R:dual B
	ATV_SOUND_MODE_NICAM_MONO,		///< NICAM mono
	ATV_SOUND_MODE_NICAM_STEREO,		///< NICAM stereo
	ATV_SOUND_MODE_NICAM_DUAL_A,		///< NICAM L:dual A, R:dual A
	ATV_SOUND_MODE_NICAM_DUAL_B,		///< NICAM L:dual B, R:dual B
	ATV_SOUND_MODE_NICAM_DUAL_AB,		///< NICAM L:dual A, R:dual B
	ATV_SOUND_MODE_AUTO		= 0x80,	///< auto sound mode
};

enum atv_comm_cmd {
	ATV_SET_START,
	ATV_SET_STOP,
	ATV_ENABLE_HIDEV,
	ATV_SET_HIDEV_BW,
	ATV_ENABLE_AGC,
	ATV_RESET_AGC,
	ATV_ENABLE_FC_TRACKING,
	ATV_RESET_FC_TRACKING,

	// Palsum audio signal level
	ATV_PAL_GET_SC1_AMP,
	ATV_PAL_GET_SC1_NSR,
	ATV_PAL_GET_SC2_AMP,
	ATV_PAL_GET_SC2_NSR,
	ATV_PAL_GET_PILOT_AMP,
	ATV_PAL_GET_PILOT_PHASE,
	ATV_PAL_GET_STEREO_DUAL_ID,
	ATV_PAL_GET_NICAM_PHASE_ERR,
	ATV_PAL_GET_BAND_STRENGTH,

	// Btsc audio signal level
	ATV_BTSC_GET_SC1_AMP,
	ATV_BTSC_GET_SC1_NSR,
	ATV_BTSC_GET_STEREO_PILOT_AMP,
	ATV_BTSC_GET_SAP_AMP,
	ATV_BTSC_GET_SAP_NSR,

	ATV_COMM_CMD_NUM,
};

enum atv_gain_type {
	ATV_PAL_GAIN_A2_FM_M,
	ATV_PAL_GAIN_HIDEV_M,
	ATV_PAL_GAIN_A2_FM_X,
	ATV_PAL_GAIN_HIDEV_X,
	ATV_PAL_GAIN_NICAM,
	ATV_PAL_GAIN_AM,

	ATV_BTSC_GAIN_TOTAL,
	ATV_BTSC_GAIN_MONO,
	ATV_BTSC_GAIN_STEREO,
	ATV_BTSC_GAIN_SAP,
	ATV_GAIN_TYPE_NUM,
};

enum atv_hidev_mode {
	ATV_HIDEV_OFF,
	ATV_HIDEV_L1,
	ATV_HIDEV_L2,
	ATV_HIDEV_L3,
	ATV_HIDEV_NUM,
};

enum atv_hidev_filter_bw {
	ATV_HIDEV_BW_DEFAULT	= 0x00,
	ATV_HIDEV_BW_L1		= 0x10,
	ATV_HIDEV_BW_L2		= 0x20,
	ATV_HIDEV_BW_L3		= 0x30,
};

enum atv_thr_type {
	A2_M_CARRIER1_ON_AMP,
	A2_M_CARRIER1_OFF_AMP,
	A2_M_CARRIER1_ON_NSR,
	A2_M_CARRIER1_OFF_NSR,
	A2_M_CARRIER2_ON_AMP,
	A2_M_CARRIER2_OFF_AMP,
	A2_M_CARRIER2_ON_NSR,
	A2_M_CARRIER2_OFF_NSR,
	A2_M_A2_PILOT_ON_AMP,
	A2_M_A2_PILOT_OFF_AMP,
	A2_M_PILOT_PHASE_ON_THD,
	A2_M_PILOT_PHASE_OFF_THD,
	A2_M_POLIT_MODE_VALID_RATIO,
	A2_M_POLIT_MODE_INVALID_RATIO,
	A2_M_EXTENSION,

	A2_BG_CARRIER1_ON_AMP,
	A2_BG_CARRIER1_OFF_AMP,
	A2_BG_CARRIER1_ON_NSR,
	A2_BG_CARRIER1_OFF_NSR,
	A2_BG_CARRIER2_ON_AMP,
	A2_BG_CARRIER2_OFF_AMP,
	A2_BG_CARRIER2_ON_NSR,
	A2_BG_CARRIER2_OFF_NSR,
	A2_BG_A2_PILOT_ON_AMP,
	A2_BG_A2_PILOT_OFF_AMP,
	A2_BG_PILOT_PHASE_ON_THD,
	A2_BG_PILOT_PHASE_OFF_THD,
	A2_BG_POLIT_MODE_VALID_RATIO,
	A2_BG_POLIT_MODE_INVALID_RATIO,
	A2_BG_EXTENSION,

	A2_DK_CARRIER1_ON_AMP,
	A2_DK_CARRIER1_OFF_AMP,
	A2_DK_CARRIER1_ON_NSR,
	A2_DK_CARRIER1_OFF_NSR,
	A2_DK_CARRIER2_ON_AMP,
	A2_DK_CARRIER2_OFF_AMP,
	A2_DK_CARRIER2_ON_NSR,
	A2_DK_CARRIER2_OFF_NSR,
	A2_DK_A2_PILOT_ON_AMP,
	A2_DK_A2_PILOT_OFF_AMP,
	A2_DK_PILOT_PHASE_ON_THD,
	A2_DK_PILOT_PHASE_OFF_THD,
	A2_DK_POLIT_MODE_VALID_RATIO,
	A2_DK_POLIT_MODE_INVALID_RATIO,
	A2_DK_EXTENSION,

	FM_I_CARRIER1_ON_AMP,
	FM_I_CARRIER1_OFF_AMP,
	FM_I_CARRIER1_ON_NSR,
	FM_I_CARRIER1_OFF_NSR,

	AM_CARRIER1_ON_AMP,
	AM_CARRIER1_OFF_AMP,
	AM_EXTENSION,

	HIDEV_M_CARRIER1_ON_AMP,
	HIDEV_M_CARRIER1_OFF_AMP,
	HIDEV_M_CARRIER1_ON_NSR,
	HIDEV_M_CARRIER1_OFF_NSR,

	HIDEV_BG_CARRIER1_ON_AMP,
	HIDEV_BG_CARRIER1_OFF_AMP,
	HIDEV_BG_CARRIER1_ON_NSR,
	HIDEV_BG_CARRIER1_OFF_NSR,

	HIDEV_DK_CARRIER1_ON_AMP,
	HIDEV_DK_CARRIER1_OFF_AMP,
	HIDEV_DK_CARRIER1_ON_NSR,
	HIDEV_DK_CARRIER1_OFF_NSR,

	HIDEV_I_CARRIER1_ON_AMP,
	HIDEV_I_CARRIER1_OFF_AMP,
	HIDEV_I_CARRIER1_ON_NSR,
	HIDEV_I_CARRIER1_OFF_NSR,

	NICAM_BG_NICAM_ON_SIGERR,
	NICAM_BG_NICAM_OFF_SIGERR,
	NICAM_BG_EXTENSION,

	NICAM_I_NICAM_ON_SIGERR,
	NICAM_I_NICAM_OFF_SIGERR,
	NICAM_I_EXTENSION,

	PAL_THR_TYPE_END,

	BTSC_MONO_ON_NSR,
	BTSC_MONO_OFF_NSR,
	BTSC_PILOT_ON_AMPLITUDE,
	BTSC_PILOT_OFF_AMPLITUDE,
	BTSC_SAP_ON_NSR,
	BTSC_SAP_OFF_NSR,
	BTSC_STEREO_ON,
	BTSC_STEREO_OFF,
	BTSC_SAP_ON_AMPLITUDE,
	BTSC_SAP_OFF_AMPLITUDE,

	BTSC_THR_TYPE_END,
};

enum atv_input_type {
	ATV_INPUT_VIF_INTERNAL	= 0,	// vif mode
	ATV_INPUT_SIF_EXTERNAL	= 1,	// sif mode
};

enum dsp_memory_type {
	DSP_MEM_TYPE_PM,
	DSP_MEM_TYPE_DM,
};

struct mtk_snd_timestamp_info {
	unsigned long long byte_count;
	struct timespec kts;
	unsigned long hts;
};

struct mtk_snd_timestamp {
	struct mtk_snd_timestamp_info ts_info[512];
	unsigned int wr_index;
	unsigned int rd_index;
	unsigned int ts_count;
	unsigned long long total_size;
};

/* Define a Ring Buffer data structure */
struct mtk_ring_buffer_struct {
	unsigned char *addr;
	unsigned int size;
	unsigned char *w_ptr;
	unsigned char *r_ptr;
	unsigned char *buf_end;
	unsigned long long total_read_size;
	unsigned long long total_write_size;
};

/* Define a Ring Buffer data structure */
struct mtk_copy_buffer_struct {
	unsigned char *addr;
	unsigned int size;
	unsigned int avail_size;
	unsigned int u32RemainSize;
};

/* Define a DMA Reader data structure */
struct mtk_dma_struct {
	struct mtk_ring_buffer_struct r_buf;
	struct mtk_copy_buffer_struct c_buf;
	struct mtk_str_mode_struct str_mode_info;
	unsigned int initialized_status;
	unsigned int period_size;
	unsigned int high_threshold;
	unsigned int copy_size;
	unsigned int status;
	unsigned int channel_mode;
	unsigned int sample_rate;
	unsigned int sample_bits;
	snd_pcm_format_t format;
	unsigned int u32TargetAlignmentSize;
};

/* Define a Runtime data structure */
struct mtk_runtime_struct {
	struct snd_pcm_hw_constraint_list constraints_rates;
	struct mtk_substream_struct substreams;
	struct mtk_dma_struct playback;
	struct mtk_dma_struct capture;
	struct mtk_snd_timestamp timestamp;
	struct mtk_monitor_struct monitor;
	struct timer_list timer;
	unsigned int spin_init_status;
	spinlock_t spin_lock;
	unsigned int log_level;
	unsigned int clk_status;
	void *private;
	int id;
};

struct capture_register {
	u32 PAS_CTRL_0;
	u32 PAS_CTRL_9;
	u32 PAS_CTRL_39;
	u32 PAS_CTRL_48;
	u32 PAS_CTRL_49;
	u32 PAS_CTRL_50;
	u32 PAS_CTRL_64;
	u32 PAS_CTRL_65;
	u32 PAS_CTRL_66;
	u32 PAS_CTRL_73;
	u32 PAS_CTRL_74;
	u32 HDMI_EVENT_0;
	u32 HDMI_EVENT_1;
	u32 HDMI_EVENT_2;
	u32 HDMI2_EVENT_0;
	u32 HDMI2_EVENT_1;
	u32 HDMI2_EVENT_2;
	u32 HDMI2_EVENT_3;
	u32 HDMI2_EVENT_4;
	u32 SE_IDMA_CTRL_0;
	u32 SE_BDMA_CFG;
	u32 FD230_SEL;
	u32 SE_DSP_BRG_DATA_L;
	u32 SE_DSP_BRG_DATA_H;
	u32 SE_IDMA_WRBASE_ADDR_L;
	u32 SE_IDMA_RDBASE_ADDR_L;
	u32 SE_IDMA_RDDATA_H_0;
	u32 SE_IDMA_RDDATA_H_1;
	u32 SE_IDMA_RDDATA_L;
	u32 SE_AUD_CTRL;
	u32 SE_MBASE_H;
	u32 SE_MSIZE_H;
	u32 SE_MCFG;
	u32 SE_MBASE_EXT;
	u32 SE_INPUT_CFG;
	u32 STATUS_HDMI_CS;
	u32 STATUS_HDMI_PC;
	u32 STATUS_HDMI2_PC;
	u32 HDMI_CFG_6;
	u32 HDMI2_CFG_29;
	u32 HDMI_MODE_CFG;
	u32 HDMI2_MODE_CFG1;
	u32 HDMI2_MODE_CFG2;
	u32 DECODER1_CFG;
	u32 DECODER2_CFG;
	u32 CLK_CFG6;
	u32 SIF_AGC_CFG6;
	u32 ASIF_AMUX;
	u32 I2S_INPUT_CFG;
	u32 I2S_OUT2_CFG;
	u32 CAP_HDMI_CTRL;
	u32 CAP_DMA2_CTRL;
	u32 CAP_DMA2_RPTR;
	u32 CAP_DMA2_WPTR;
	u32 CAP_DMA4_CTRL;
	u32 CAP_DMA4_RPTR;
	u32 CAP_DMA4_WPTR;
	u32 CAP_DMA7_CTRL;
	u32 CAP_DMA7_RPTR;
	u32 CAP_DMA7_WPTR;
	u32 CAP_DMA8_CTRL;
	u32 CAP_DMA8_RPTR;
	u32 CAP_DMA8_WPTR;
	u32 CAP_DMA9_CTRL;
	u32 CAP_DMA9_RPTR;
	u32 CAP_DMA9_WPTR;
	u32 CAP_DMA10_CTRL;
	u32 CAP_DMA10_RPTR;
	u32 CAP_DMA10_WPTR;
	u32 CHANNEL_M7_CFG;
	u32 CHANNEL_M8_CFG;
	u32 CHANNEL_M15_CFG;
	u32 CHANNEL_M16_CFG;
	u32 ATV_PALY_CMD;
	u32 ATV_STD_CMD;
	u32 ATV_COMM_CFG;
	u32 ATV_SND_MODE_1;
	u32 ATV_SND_MODE_2;
	u32 ATV_DETECTION_RESULT;
	u32 ATV_SND_CARRIER_STATUS_1;
	u32 ATV_SND_CARRIER_STATUS_2;
	u32 DBG_CMD_1;
	u32 DBG_CMD_2;
	u32 DBG_RESULT_1;
	u32 DBG_RESULT_2;
	u32 SE_AID_CTRL;
	u32 ANALOG_MUX;
	u32 IRQ0_IDX;
	u32 IRQ1_IDX;
	u32 HWTSG_CFG4;
	u32 HWTSG_TS_L;
	u32 HWTSG_TS_H;
	u32 HWTSG2_CFG4;
	u32 HWTSG2_TS_L;
	u32 HWTSG2_TS_H;
};

struct duplex_register {
	u32 CHANNEL_M1_CFG;
	u32 CHANNEL_M2_CFG;
	u32 CHANNEL_M3_CFG;
	u32 CHANNEL_M4_CFG;
	u32 CHANNEL_M5_CFG;
	u32 CHANNEL_M6_CFG;
	u32 CHANNEL_M7_CFG;
	u32 CHANNEL_M8_CFG;
	u32 CHANNEL_M9_CFG;
	u32 CHANNEL_M10_CFG;
	u32 CHANNEL_M11_CFG;
	u32 CHANNEL_M12_CFG;
	u32 CHANNEL_M13_CFG;
	u32 CHANNEL_M14_CFG;
	u32 CHANNEL_M15_CFG;
	u32 CHANNEL_M16_CFG;
	u32 CHANNEL_5_CFG;
	u32 CHANNEL_6_CFG;
	u32 CHANNEL_7_CFG;
	u32 CHANNEL_8_CFG;
	u32 CHANNEL_9_CFG;
	u32 CHANNEL_10_CFG;
	u32 PB_DMA_SYNTH_UPDATE;

	// SRC1
	u32 PB_DMA_SYNTH_H;
	u32 PB_DMA_SYNTH_L;
	u32 PB_DMA_CFG_00;
	u32 PB_DMA_CFG_01;
	u32 PB_DMA_CFG_02;
	u32 PB_DMA_CFG_03;
	u32 PB_DMA_CFG_04;
	u32 PB_DMA_CFG_05;
	u32 PB_DMA_CFG_06;
	u32 PB_DMA_CFG_07;
	u32 PB_DMA_CFG_08;
	u32 PB_DMA_CFG_09;
	u32 PB_DMA_ENABLE;
	u32 PB_DMA_STATUS_00;
	u32 CAP_DMA_CTRL;
	u32 CAP_DMA_RPTR;
	u32 CAP_DMA_WPTR;

	// SRC2
	u32 SRC2_PB_DMA_CTRL;
	u32 SRC2_PB_DMA_WPTR;
	u32 SRC2_PB_DMA_DDR_LEVEL;
	u32 SRC2_PB_DMA_SYNTH_H;
	u32 SRC2_PB_DMA_SYNTH_L;
	u32 SRC2_PB_DMA_SYNTH_UPDATE;
	u32 SRC2_CAP_DMA_CTRL;
	u32 SRC2_CAP_DMA_RPTR;
	u32 SRC2_CAP_DMA_WPTR;

	// SRC3
	u32 SRC3_PB_DMA_SYNTH_H;
	u32 SRC3_PB_DMA_SYNTH_L;
	u32 SRC3_PB_DMA_CFG_00;
	u32 SRC3_PB_DMA_CFG_01;
	u32 SRC3_PB_DMA_CFG_02;
	u32 SRC3_PB_DMA_CFG_03;
	u32 SRC3_PB_DMA_CFG_04;
	u32 SRC3_PB_DMA_CFG_05;
	u32 SRC3_PB_DMA_CFG_06;
	u32 SRC3_PB_DMA_CFG_07;
	u32 SRC3_PB_DMA_CFG_08;
	u32 SRC3_PB_DMA_CFG_09;
	u32 SRC3_PB_DMA_ENABLE;
	u32 SRC3_PB_DMA_STATUS_00;
	u32 SRC3_CAP_DMA_CTRL;
	u32 SRC3_CAP_DMA_RPTR;
	u32 SRC3_CAP_DMA_WPTR;
};

struct duplex_priv {
	struct device *dev;
	struct mem_info mem_info;
	struct mtk_runtime_struct *pcm_device[DUPLEX_DEV_NUM];
	// SRC1
	u32 pb_dma_offset;
	u32 pb_dma_size;
	u32 cap_dma_offset;
	u32 cap_dma_size;

	// SRC2
	u32 src2_pb_dma_offset;
	u32 src2_pb_dma_size;
	u32 src2_cap_dma_offset;
	u32 src2_cap_dma_size;

	// SRC3
	u32 src3_pb_dma_offset;
	u32 src3_pb_dma_size;
	u32 src3_cap_dma_offset;
	u32 src3_cap_dma_size;
};

struct atv_thr_tbl {
	u32 a2_m_carrier1_on_amp;
	u32 a2_m_carrier1_off_amp;
	u32 a2_m_carrier1_on_nsr;
	u32 a2_m_carrier1_off_nsr;
	u32 a2_m_carrier2_on_amp;
	u32 a2_m_carrier2_off_amp;
	u32 a2_m_carrier2_on_nsr;
	u32 a2_m_carrier2_off_nsr;
	u32 a2_m_a2_pilot_on_amp;
	u32 a2_m_a2_pilot_off_amp;
	u32 a2_m_pilot_phase_on_thd;
	u32 a2_m_pilot_phase_off_thd;
	u32 a2_m_polit_mode_valid_ratio;
	u32 a2_m_polit_mode_invalid_ratio;
	u32 a2_m_carrier2_output_thr;

	u32 a2_bg_carrier1_on_amp;
	u32 a2_bg_carrier1_off_amp;
	u32 a2_bg_carrier1_on_nsr;
	u32 a2_bg_carrier1_off_nsr;
	u32 a2_bg_carrier2_on_amp;
	u32 a2_bg_carrier2_off_amp;
	u32 a2_bg_carrier2_on_nsr;
	u32 a2_bg_carrier2_off_nsr;
	u32 a2_bg_a2_pilot_on_amp;
	u32 a2_bg_a2_pilot_off_amp;
	u32 a2_bg_pilot_phase_on_thd;
	u32 a2_bg_pilot_phase_off_thd;
	u32 a2_bg_polit_mode_valid_ratio;
	u32 a2_bg_polit_mode_invalid_ratio;
	u32 a2_bg_carrier2_output_thr;

	u32 a2_dk_carrier1_on_amp;
	u32 a2_dk_carrier1_off_amp;
	u32 a2_dk_carrier1_on_nsr;
	u32 a2_dk_carrier1_off_nsr;
	u32 a2_dk_carrier2_on_amp;
	u32 a2_dk_carrier2_off_amp;
	u32 a2_dk_carrier2_on_nsr;
	u32 a2_dk_carrier2_off_nsr;
	u32 a2_dk_a2_pilot_on_amp;
	u32 a2_dk_a2_pilot_off_amp;
	u32 a2_dk_pilot_phase_on_thd;
	u32 a2_dk_pilot_phase_off_thd;
	u32 a2_dk_polit_mode_valid_ratio;
	u32 a2_dk_polit_mode_invalid_ratio;
	u32 a2_dk_carrier2_output_thr;

	u32 fm_i_carrier1_on_amp;
	u32 fm_i_carrier1_off_amp;
	u32 fm_i_carrier1_on_nsr;
	u32 fm_i_carrier1_off_nsr;

	u32 am_carrier1_on_amp;
	u32 am_carrier1_off_amp;
	u32 am_agc_ref;

	u32 hidev_m_carrier1_on_amp;
	u32 hidev_m_carrier1_off_amp;
	u32 hidev_m_carrier1_on_nsr;
	u32 hidev_m_carrier1_off_nsr;

	u32 hidev_bg_carrier1_on_amp;
	u32 hidev_bg_carrier1_off_amp;
	u32 hidev_bg_carrier1_on_nsr;
	u32 hidev_bg_carrier1_off_nsr;

	u32 hidev_dk_carrier1_on_amp;
	u32 hidev_dk_carrier1_off_amp;
	u32 hidev_dk_carrier1_on_nsr;
	u32 hidev_dk_carrier1_off_nsr;

	u32 hidev_i_carrier1_on_amp;
	u32 hidev_i_carrier1_off_amp;
	u32 hidev_i_carrier1_on_nsr;
	u32 hidev_i_carrier1_off_nsr;

	u32 nicam_bg_nicam_on_sigerr;
	u32 nicam_bg_nicam_off_sigerr;
	u32 nicam_bg_reset_hw_siger;

	u32 nicam_i_nicam_on_sigerr;
	u32 nicam_i_nicam_off_sigerr;
	u32 nicam_i_reset_hw_siger;

	u32 pal_thr_type_end;

	u32 btsc_mono_on_nsr;
	u32 btsc_mono_off_nsr;
	u32 btsc_pilot_on_amplitude;
	u32 btsc_pilot_off_amplitude;
	u32 btsc_sap_on_nsr;
	u32 btsc_sap_off_nsr;
	u32 btsc_stereo_on;
	u32 btsc_stereo_off;
	u32 btsc_sap_on_amplitude;
	u32 btsc_sap_off_amplitude;

	u32 btsc_thr_type_end;
};

struct atv_gain_tbl {
	//[-12 ~ +10]dB, 0.25dB/step
	//eg:  Fm + 5dB---->s32Fm = 5dB/0.25dB
	s32 pal_a2_fm_m;
	s32 pal_hidev_m;
	s32 pal_a2_fm_x;
	s32 pal_hidev_x;
	s32 pal_nicam;
	s32 pal_am;

	s32 btsc_total;
	s32 btsc_mono;
	s32 btsc_stereo;
	s32 btsc_sap;
};

struct capture_priv {
	int irq0;
	int irq1;
	int irq2;
	int irq3;
	struct tasklet_struct irq0_tasklet;
	struct device *dev;
	struct mem_info mem_info;
	struct mtk_runtime_struct *pcm_device[CAP_DEV_NUM];
	int irq0_counter;
	int hdmi_rx1_nonpcm_flag;
	int hdmi_rx2_nonpcm_flag;
	u32 hdmi1_dma_offset;
	u32 hdmi1_dma_size;
	u32 hdmi2_dma_offset;
	u32 hdmi2_dma_size;
	u32 cap2_dma_offset;
	u32 cap2_dma_size;
	u32 cap4_dma_offset;
	u32 cap4_dma_size;
	u32 cap7_dma_offset;
	u32 cap7_dma_size;
	u32 cap8_dma_offset;
	u32 cap8_dma_size;
	u32 cap9_dma_offset;
	u32 cap9_dma_size;
	u32 cap10_dma_offset;
	u32 cap10_dma_size;
	struct atv_thr_tbl atv_thr;
	struct atv_gain_tbl atv_gain;
};

int duplex_dev_probe(struct platform_device *pdev);
int duplex_dev_remove(struct platform_device *pdev);

int capture_dev_probe(struct platform_device *pdev);
int capture_dev_remove(struct platform_device *pdev);

#endif /* _PCM_PLATFORM_HEADER */
