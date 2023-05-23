// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <sound/soc.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/info.h>
#include <sound/initval.h>

#include "audio_mcu_communication.h"
#include "audio_mcu_rpmsg.h"
#include "audio_mtk_dbg.h"
#include "mtk-reserved-memory.h"
#include <asm-generic/div64.h>

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define SND_AUDIO_MCU_DRIVER "audio-mcu"
#define SND_PCM_NAME "audio-mcu-platform"
#define SND_DRV_CARD_NUM 8
#define SND_DMA_BIT_MASK 32

#define SND_MCU_PARM_SIZE 256
#define SND_MCU_BUFFER_SIZE_MAX 0x7FFFFFFF

#define ADEC1_PARA "ADEC1 Parameter"
#define ADEC2_PARA "ADEC2 Parameter"
#define ASND_PARA "ASND Parameter"
#define ADVS_PARA "ADVS Parameter"
#define AENC_PARA "AENC Parameter"
#define ADEC1_IN_BUF_SIZE "ADEC1 in buffer size"
#define ADEC2_IN_BUF_SIZE "ADEC2 in buffer size"
#define ASND_IN_BUF_SIZE "ASND in buffer size"
#define ADVS_IN_BUF_SIZE "ADVS in buffer size"
#define AENC_IN_BUF_SIZE "AENC in buffer size"

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//ALSA card related
static unsigned long mtk_log_level = LOGLEVEL_INFO;// LOGLEVEL_ERR LOGLEVEL_CRIT
static LIST_HEAD(audio_card_dev_list);
static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int pcm_capture_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = SND_DRV_CARD_NUM};
static int pcm_playback_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = SND_DRV_CARD_NUM};
module_param_array(pcm_capture_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_capture_substreams, "PCM capture substreams # (1) for audio mcu driver.");
module_param_array(pcm_playback_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_playback_substreams, "PCM playback substreams # (1) for audio mcu driver.");

//------------------------------------------------------------------------------
//  Enumerate And Structure
//------------------------------------------------------------------------------
enum {
	ADEC1_PARAMETER = 0,
	ADEC2_PARAMETER,
	ASND_PARAMETER,
	ADVS_PARAMETER,
	AENC_PARAMETER,
	ADEC1_IN_BUFFER_SIZE,
	ADEC2_IN_BUFFER_SIZE,
	ASND_IN_BUFFER_SIZE,
	ADVS_IN_BUFFER_SIZE,
	AENC_IN_BUFFER_SIZE,
	AUDIO_REG_LEN,
};

static unsigned int card_reg[AUDIO_REG_LEN] = {
	0x0,	//ADEC1_PARAMETER
	0x0,	//ADEC2_PARAMETER
	0x0,	//ASND_PARAMETER
	0x0,	//ADVS_PARAMETER
	0x0,	//AENC_PARAMETER
	0x0,	//ADEC1_IN_BUFFER_SIZE
	0x0,	//ADEC2_IN_BUFFER_SIZE
	0x0,	//ASND_IN_BUFFER_SIZE
	0x0,	//ADVS_IN_BUFFER_SIZE
	0x0,	//AENC_IN_BUFFER_SIZE
};

//PCM related
static const struct snd_pcm_hardware audio_pcm_hardware = {
	.info = SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_INTERLEAVED,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
		SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000,
	.rate_min = 8000,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = 6 * 1024 * 1024, // 768 * 1024
	.period_bytes_min = 1 * 1024,
	.period_bytes_max = 64 * 1024,
	.periods_min = 4,
	.periods_max = 1512,	//32,
	.fifo_size = 0,
};

static const char *audio_mcu_device_size[AUDIO_MCU_DEVICE_NUM_MAX] = {
	"audio-mcu-adec1-size",
	"audio-mcu-adec2-size",
	"audio-mcu-asnd-size",
	"audio-mcu-advs-size",
	"audio-mcu-aenc-size",
};

static const char *audio_mcu_device_offset[AUDIO_MCU_DEVICE_NUM_MAX] = {
	"audio-mcu-adec1-offset",
	"audio-mcu-adec2-offset",
	"audio-mcu-asnd-offset",
	"audio-mcu-advs-offset",
	"audio-mcu-aenc-offset",
};

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
static unsigned int audio_card_read(struct mtk_snd_audio *audio,
				unsigned int reg);
static int audio_card_write(struct mtk_snd_audio *audio,
				unsigned int reg, unsigned int value);
static int audio_card_get_volsw(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol);
static int audio_card_put_volsw(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol);
static int audio_card_get_mcu_parm(struct snd_kcontrol *kcontrol,
				unsigned int __user *data,
				unsigned int size);
static int audio_card_put_mcu_parm(struct snd_kcontrol *kcontrol,
				const unsigned int __user *data,
				unsigned int size);
static int audio_pcm_dma_new(struct mtk_snd_audio *audio,
				struct snd_pcm_substream *substream);

static const struct snd_kcontrol_new audio_snd_controls[] = {
	SND_SOC_BYTES_TLV(ADEC1_PARA, SND_MCU_PARM_SIZE,
		audio_card_get_mcu_parm, audio_card_put_mcu_parm),
	SND_SOC_BYTES_TLV(ADEC2_PARA, SND_MCU_PARM_SIZE,
		audio_card_get_mcu_parm, audio_card_put_mcu_parm),
	SND_SOC_BYTES_TLV(ASND_PARA, SND_MCU_PARM_SIZE,
		audio_card_get_mcu_parm, audio_card_put_mcu_parm),
	SND_SOC_BYTES_TLV(ADVS_PARA, SND_MCU_PARM_SIZE,
		audio_card_get_mcu_parm, audio_card_put_mcu_parm),
	SND_SOC_BYTES_TLV(AENC_PARA, SND_MCU_PARM_SIZE,
		audio_card_get_mcu_parm, audio_card_put_mcu_parm),
	SOC_SINGLE_EXT(ADEC1_IN_BUF_SIZE, ADEC1_IN_BUFFER_SIZE, 0,
		SND_MCU_BUFFER_SIZE_MAX, 0, audio_card_get_volsw, audio_card_put_volsw),
	SOC_SINGLE_EXT(ADEC2_IN_BUF_SIZE, ADEC2_IN_BUFFER_SIZE, 0,
		SND_MCU_BUFFER_SIZE_MAX, 0, audio_card_get_volsw, audio_card_put_volsw),
	SOC_SINGLE_EXT(ASND_IN_BUF_SIZE, ASND_IN_BUFFER_SIZE, 0,
		SND_MCU_BUFFER_SIZE_MAX, 0, audio_card_get_volsw, audio_card_put_volsw),
	SOC_SINGLE_EXT(ADVS_IN_BUF_SIZE, ADVS_IN_BUFFER_SIZE, 0,
		SND_MCU_BUFFER_SIZE_MAX, 0, audio_card_get_volsw, audio_card_put_volsw),
	SOC_SINGLE_EXT(AENC_IN_BUF_SIZE, AENC_IN_BUFFER_SIZE, 0,
		SND_MCU_BUFFER_SIZE_MAX, 0, audio_card_get_volsw, audio_card_put_volsw),
};
//------------------------------------------------------------------------------
//  Audio MCU Function
//------------------------------------------------------------------------------
static uint64_t audio_mcu_get_ns_time(void)
{
	struct timespec64 current_time;
	uint64_t time;

	ktime_get_ts64(&current_time);
	time = current_time.tv_sec*1000000000LL + current_time.tv_nsec;

	return time;
}

static void audio_mcu_cmd_parameter(uint32_t pa_addr, uint32_t size, uint8_t device)
{
	struct audio_parameter_config audio_packet;
	struct audio_mcu_device_config audio_device;

	audio_packet.mcu_device_id = device;
	audio_packet.pa_addr = pa_addr;
	audio_packet.size = size;

	audio_device.device = device;

	audio_mcu_communication_cmd_parameter(&audio_packet, &audio_device);
}

static void audio_mcu_cmd_shm_buffer(uint8_t device, uint32_t addr, uint32_t size)
{
	struct audio_shm_buffer_config audio_packet;
	struct audio_mcu_device_config audio_device;
	// uint32_t pa_addr = 0x0;
	// uint32_t size = 0x0;

	audio_packet.mcu_device_id = device;
	audio_packet.pa_addr = addr;
	audio_packet.size = size;
	audio_packet.type = AUDIO_MCU_INPUT_BUFFER;

	audio_device.device = device;

	audio_mcu_communication_cmd_shm_buffer(&audio_packet, &audio_device);
}

static int audio_mcu_parameter_cb(struct audio_parameter_config *audio_packet,
			struct audio_mcu_device_config *audio_device)
{
	audio_info("[%s]n", __func__);
	// TO-DO
	return 0;
}

static int audio_mcu_shm_buffer_cb(struct audio_shm_buffer_config *audio_packet,
			struct audio_mcu_device_config *audio_device)
{
	audio_info("[%s]n", __func__);
	// TO-DO
	return 0;
}

static unsigned int audio_card_read(struct mtk_snd_audio *audio,
			unsigned int reg)
{
	audio_err("%s: reg = 0x%x, val = 0x%x\n", __func__, reg, card_reg[reg]);

	if (!audio) {
		audio_err("Invalid audio argument\n");
		return -EINVAL;
	}

	return card_reg[reg];
}

static int audio_card_write(struct mtk_snd_audio *audio, unsigned int reg,
				unsigned int value)
{
	int ret = 0;
	struct audio_card_drvdata *card_drvdata = NULL;

	if (!audio) {
		audio_err("Invalid audio argument\n");
		return -EINVAL;
	}

	card_drvdata = audio->audio_card;
	audio_err("%s: reg = 0x%x, val = 0x%x\n", __func__, reg, value);

	switch (reg) {
	case ADEC1_IN_BUFFER_SIZE:
		if (value <= card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC1].in_buf_max_size)
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC1].in_buf_size = value;
		audio_err("%s: audio reg: x%x, val = 0x%x val = 0x%x\n", __func__, reg,
			value, card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC1].in_buf_size);
		break;
	case ADEC2_IN_BUFFER_SIZE:
		if (value <= card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC2].in_buf_max_size)
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC2].in_buf_size = value;
		audio_err("%s: audio reg: x%x, val = 0x%x val = 0x%x\n", __func__, reg, value,
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC2].in_buf_size);
		break;
	case ASND_IN_BUFFER_SIZE:
		if (value <= card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ASND].in_buf_max_size)
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ASND].in_buf_size = value;
		audio_err("%s: audio reg: x%x, val = 0x%x val = 0x%x\n", __func__, reg, value,
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ASND].in_buf_size);
		break;
	case ADVS_IN_BUFFER_SIZE:
		if (value <= card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADVS].in_buf_max_size)
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADVS].in_buf_size = value;
		audio_err("%s: audio reg: x%x, val = 0x%x val = 0x%x\n", __func__, reg, value,
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADVS].in_buf_size);
		break;
	case AENC_IN_BUFFER_SIZE:
		if (value <= card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_AENC].in_buf_max_size)
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_AENC].in_buf_size = value;
		audio_err("%s: audio reg: x%x, val = 0x%x val = 0x%x\n", __func__, reg, value,
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_AENC].in_buf_size);
		break;
	default:
		audio_err("%s: error parameter, reg = 0x%x, val = 0x%x\n",
				__func__, reg, value);
		return -1;
	}

	if (ret) {
		audio_err("%s: update error , reg = 0x%x, val = 0x%x\n",
				__func__, reg, value);
		ret = -1;
	} else {
		card_reg[reg] = value;
		ret = 0;
	}

	return ret;
}

static int audio_card_get_volsw(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_audio *audio = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;

	ucontrol->value.integer.value[0] = audio_card_read(audio, mc->reg);
	return 0;
}

static int audio_card_put_volsw(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_audio *audio = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
	    (struct soc_mixer_control *)kcontrol->private_value;

	if ((ucontrol->value.integer.value[0] < 0)
		|| (ucontrol->value.integer.value[0] > mc->max)) {
		audio_err("reg:%d range MUST is (0->%d), %ld is overflow\r\n",
		       mc->reg, mc->max, ucontrol->value.integer.value[0]);
		return -1;
	}
	return audio_card_write(audio, mc->reg,
					ucontrol->value.integer.value[0]);
}

static int audio_card_get_mcu_parm(struct snd_kcontrol *kcontrol,
						unsigned int __user *data,
						unsigned int size)
{
	struct mtk_snd_audio *audio = snd_kcontrol_chip(kcontrol);
	int ret = 0;
	uint32_t parm = 0x0;
	uint8_t *parm_ptr = NULL;
	uint8_t id_base = 1;

	audio_info("%s numid:%d name:%s index:%d\n", __func__,
		kcontrol->id.numid,
		kcontrol->id.name,
		kcontrol->id.index);

	if (size > SND_MCU_PARM_SIZE) {
		audio_err("%s(), Size overflow.(%d>%d)\r\n", __func__, size, SND_MCU_PARM_SIZE);
		return -EINVAL;
	}

	switch (kcontrol->id.numid-id_base) {
	case ADEC1_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_ADEC1].parm_addr;
		break;
	case ADEC2_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_ADEC2].parm_addr;
		break;
	case ASND_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_ASND].parm_addr;
		break;
	case ADVS_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_ADVS].parm_addr;
		break;
	case AENC_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_AENC].parm_addr;
		break;
	default:
		audio_err("%s: error parameter, name = %s\n", __func__, kcontrol->id.name);
	}

	parm_ptr = (uint8_t *)ioremap_wc(parm, SND_MCU_PARM_SIZE);

	ret = copy_to_user(data, parm_ptr, size);
	audio_info("%s %p 0x%x\n", __func__, parm_ptr, parm);
	if (ret) {
		audio_err("%s(), copy_to_user fail", __func__);
		ret = -EFAULT;
	}

	return ret;
}

static int audio_card_put_mcu_parm(struct snd_kcontrol *kcontrol,
						const unsigned int __user *data,
						unsigned int size)
{
	struct mtk_snd_audio *audio = snd_kcontrol_chip(kcontrol);
	int ret = 0;
	uint32_t parm = 0x0;
	uint8_t device = 0;
	uint8_t *parm_ptr = NULL;
	uint8_t id_base = 1;
	uint64_t ts = 0;

	ts = audio_mcu_get_ns_time();

	if (size > SND_MCU_PARM_SIZE) {
		audio_err("%s(), Size overflow.(%d>%d)\r\n", __func__, size, SND_MCU_PARM_SIZE);
		return -EINVAL;
	}

	switch (kcontrol->id.numid-id_base) {
	case ADEC1_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_ADEC1].parm_addr;
		device = AUDIO_MCU_DEVICE_ADEC1;
		break;
	case ADEC2_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_ADEC2].parm_addr;
		device = AUDIO_MCU_DEVICE_ADEC2;
		break;
	case ASND_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_ASND].parm_addr;
		device = AUDIO_MCU_DEVICE_ASND;
		break;
	case ADVS_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_ADVS].parm_addr;
		device = AUDIO_MCU_DEVICE_ADVS;
		break;
	case AENC_PARAMETER:
		parm = audio->audio_card->device_buf_info[AUDIO_MCU_DEVICE_AENC].parm_addr;
		device = AUDIO_MCU_DEVICE_AENC;
		break;
	default:
		audio_err("%s: error parameter, name = %s\n", __func__, kcontrol->id.name);
	}

	parm_ptr = (uint8_t *)ioremap_wc(parm, SND_MCU_PARM_SIZE);

	ret = copy_from_user(parm_ptr, data, size);

	if (ret) {
		audio_err("%s(), copy_to_user fail", __func__);
		ret = -EFAULT;
	}

	// Send to MCU
	audio_mcu_cmd_parameter(parm, SND_MCU_PARM_SIZE, device);

	audio_info("%s cost: %lld ns\n", __func__, audio_mcu_get_ns_time()-ts);

	return ret;
}

void audio_pcm_write2Log(struct mtk_snd_audio *audio, char *str)
{
	int ret = 0;

	if (audio->dbgInfo.writed < MAX_LOG_LEN) {
		ret = scnprintf(audio->dbgInfo.log[audio->dbgInfo.writed].event,
			MAX_LEN, "%s", str);
		if (ret > 0) {
			ktime_get_ts(&(audio->dbgInfo.log[audio->dbgInfo.writed].tstamp));
			audio->dbgInfo.writed++;
		} else {
			audio_err("snprintf fail, event: %s\n",
				audio->dbgInfo.log[audio->dbgInfo.writed].event);
		}
	}
}

static int audio_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_audio *audio;
	struct snd_pcm *pcm;
	int ret = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	if (substream->pcm->device >= AUDIO_MCU_DEVICE_NUM_MAX ||
		substream->pcm->device < AUDIO_MCU_DEVICE_ADEC1) {
		audio_err("[%s]Invalid devic\n", __func__);
		return -EINVAL;
	}

	audio = (struct mtk_snd_audio *)substream->private_data;

	audio_err("%s : %d", __func__, substream->pcm->device);
	pcm = audio->pcm[substream->pcm->device];

	audio_err("%s: stream = %s, pcmC%dD%d (substream = %s)\n",
			__func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
			pcm->card->number,
			pcm->device, substream->name
			);

	runtime = substream->runtime;
	runtime->hw = audio_pcm_hardware;

	audio_pcm_write2Log(audio, "pcm open");

	return ret;
}

static int audio_pcm_close(struct snd_pcm_substream *substream)
{
	struct mtk_snd_audio *audio;
	int ret = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	if (substream->pcm->device >= AUDIO_MCU_DEVICE_NUM_MAX ||
		substream->pcm->device < AUDIO_MCU_DEVICE_ADEC1) {
		audio_err("[%s]Invalid devic\n", __func__);
		return -EINVAL;
	}

	audio = (struct mtk_snd_audio *)substream->private_data;

	audio_mcu_communication_reset(substream->pcm->device);

	audio_err("%s: stream = %s\n", __func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	audio_pcm_write2Log(audio, "pcm close");
	return ret;
}

static int audio_pcm_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_audio *audio;
	int ret = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	if (substream->pcm->device >= AUDIO_MCU_DEVICE_NUM_MAX ||
		substream->pcm->device < AUDIO_MCU_DEVICE_ADEC1) {
		audio_err("[%s]Invalid devic\n", __func__);
		return -EINVAL;
	}

	runtime = substream->runtime;
	audio = (struct mtk_snd_audio *)substream->private_data;

	runtime->format = params_format(params);
	runtime->subformat = params_subformat(params);
	runtime->channels = params_channels(params);
	runtime->rate = params_rate(params);

	audio_info("%s: stream = %s\n", __func__,
		(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));
	audio_info("params_channels     = %d\n",
		params_channels(params));
	audio_info("params_rate         = %d\n",
		params_rate(params));
	audio_info("params_period_size  = %d\n",
		params_period_size(params));
	audio_info("params_periods      = %d\n",
		params_periods(params));
	audio_info("params_buffer_size  = %d\n",
		params_buffer_size(params));
	audio_info("params_buffer_bytes = %d\n",
		params_buffer_bytes(params));
	audio_info("params_sample_width = %d\n",
		snd_pcm_format_physical_width(params_format(params)));
	audio_info("params_access       = %d\n",
		params_access(params));
	audio_info("params_format       = %d\n",
		params_format(params));
	audio_info("params_subformat    = %d\n",
		params_subformat(params));

	ret = audio_pcm_dma_new(audio, substream);

	if (ret < 0) {
		audio_err("Fail to new DMA buffer\n");
		return ret;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->dma_bytes = params_buffer_bytes(params);

	audio_info("dma buf vir addr = %p size = %zu\n",
		runtime->dma_area, runtime->dma_bytes);

	// Re-set up the underrun and overrun
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (ret) {
			audio_err("[%s][%d]Fail to initial channel\n", __func__, __LINE__);
			return -EINVAL;
		}
		audio_pcm_write2Log(audio, "initial channel");
	}

	memset(runtime->dma_area, 0, runtime->dma_bytes);

	return ret;
}

static int audio_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct mtk_snd_audio *audio;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}
	audio = (struct mtk_snd_audio *)substream->private_data;

	audio_err("%s: stream = %s\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	snd_pcm_set_runtime_buffer(substream, NULL);

	audio_pcm_write2Log(audio, "audio_pcm_hw_free");

	return 0;
}

static int audio_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct mtk_snd_audio *audio;
	int ret = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	audio = (struct mtk_snd_audio *)substream->private_data;

	audio_pcm_write2Log(audio, "audio_pcm_prepare");

	return ret;
}

static int audio_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct mtk_snd_audio *audio;
	unsigned long offset;
	unsigned long pfn;
	phys_addr_t mem_pfn;
	size_t align_bytes;
	long size;
	int ret;

	audio_info("%s(), vm_flags 0x%lx, vm_pgoff 0x%lx\n",
		 __func__, vma->vm_flags, vma->vm_pgoff);

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	if (substream->pcm->device >= AUDIO_MCU_DEVICE_NUM_MAX ||
		substream->pcm->device < AUDIO_MCU_DEVICE_ADEC1) {
		audio_err("[%s]Invalid devic\n", __func__);
		return -EINVAL;
	}

	audio = (struct mtk_snd_audio *)substream->private_data;
	size = vma->vm_end - vma->vm_start;
	align_bytes = PAGE_ALIGN(audio->audio_card->audio_buf_size);
	offset = vma->vm_pgoff << PAGE_SHIFT;
	mem_pfn = audio->audio_card->audio_buf_paddr;

	if ((size_t)size > align_bytes)
		return -EINVAL;
	if (offset > align_bytes - size)
		return -EINVAL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		mem_pfn = audio->audio_card->device_buf_info[substream->pcm->device].in_buf_addr;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		mem_pfn = audio->audio_card->device_buf_info[substream->pcm->device].in_buf_addr;

	pfn = mem_pfn >> PAGE_SHIFT;

	/* ensure that memory does not get swapped to disk */
	vma->vm_flags |= VM_IO;
	/* ensure non-cacheable */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	ret = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
	audio_info("%s(), ret %d, pfn 0x%lx, phys_addr %pa -> vm_start 0x%lx\n vm_end 0x%lx",
		__func__, ret, pfn, &mem_pfn, vma->vm_start, vma->vm_end);

	if (ret)
		audio_err("%s(), ret %d, remap failed 0x%lx, phys_addr %pa -> vm_start 0x%lx\n",
			__func__, ret, pfn, &mem_pfn, vma->vm_start);
	/*Comment*/
	smp_mb();

	audio_mcu_cmd_shm_buffer(0, mem_pfn, size); // Send to RV without trigger start.
	return 0;
}

unsigned char *audio_pcm_dma_alloc_dmem(struct mtk_snd_audio *audio,
			struct snd_pcm_substream *substream, int size, dma_addr_t *addr)
{
	unsigned char *ptr = NULL;
	phys_addr_t mem_cur_pos;
	u32 mem_free_size = audio->audio_card->audio_buf_size; // audio_pcm_size

	if (substream->pcm->device >= AUDIO_MCU_DEVICE_NUM_MAX ||
		substream->pcm->device < AUDIO_MCU_DEVICE_ADEC1) {
		audio_err("[%s]Invalid devic\n", __func__);
		return NULL;
	}

	mem_cur_pos =
		audio->audio_card->device_buf_info[substream->pcm->device].in_buf_addr;

	if (!addr)
		return NULL;

	if (!mem_free_size)
		return NULL;

	if (size < mem_free_size) {
		ptr = (unsigned char *)ioremap_wc(mem_cur_pos, size);
		if (!ptr) {
			*addr = 0x0;
			audio_err("ioremap NULL address,phys : %pa\n", &mem_cur_pos);
		} else
			*addr = (dma_addr_t)(mem_cur_pos);
	} else {
		*addr = 0x0;
		audio_err("No enough memory, free = %u\n", mem_free_size);
	}

	return ptr;
}

static int audio_pcm_dma_new(struct mtk_snd_audio *audio, struct snd_pcm_substream *substream)
{
	struct snd_dma_buffer *buf;
	size_t size = 0;
	int ret = 0;

	if (!audio) {
		audio_err("%s Invalid audio argument\n", __func__);
		return -EINVAL;
	}

	if (!substream) {
		audio_err("%s Invalid substream argument\n", __func__);
		return -EINVAL;
	}

	size = audio_pcm_hardware.buffer_bytes_max;

	buf = &substream->dma_buffer;
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = audio->card->dev;
	buf->private_data = NULL;
	buf->area = audio_pcm_dma_alloc_dmem(audio, substream, PAGE_ALIGN(size), &buf->addr);
	buf->bytes = PAGE_ALIGN(size);

	if (!buf->area)
		return -ENOMEM;

	audio_info("%s: stream = %s\r\n", __func__, (substream->stream ==
				SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));
	audio_info("dma buffer align size 0x%lx, size 0x%zx\n", (unsigned long)buf->bytes, size);
	audio_info("physical dma address %pad\n", &buf->addr);
	audio_info("virtual dma address,%p\n", buf->area);

	return ret;
}

static struct snd_pcm_ops audio_mcu_pcm_ops = {
	.open = audio_pcm_open,
	.close = audio_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = audio_pcm_hw_params,
	.hw_free = audio_pcm_hw_free,
	.prepare = audio_pcm_prepare,
	.mmap = audio_pcm_mmap,
};

///sys/devices/platform/audio-mcu.0/mtk_dbg//audio_test_cmd
static ssize_t audio_test_cmd_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_audio *audio = (struct mtk_snd_audio *)card->private_data;

	return scnprintf(buf, MAX_LEN, "%u\n", audio->sysfs_test_config);
}

static ssize_t audio_test_cmd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_audio *audio = (struct mtk_snd_audio *)card->private_data;
	long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;

	if (item == 0) {
		audio->sysfs_test_config = (uint32_t)item;
	} else if (item == 1) {
		struct audio_metadata_struct audio_metadata_packet;
		struct audio_mcu_device_config audio_device;
		uint8_t device = AUDIO_MCU_DEVICE_ADEC1;

		audio_metadata_packet.audio_meta_value1 = 0x0;
		audio_metadata_packet.audio_meta_value2 = 0x0;
		audio_metadata_packet.audio_meta_value3 = 0x0;
		audio_metadata_packet.audio_mcu_device_id = device;

		audio_device.device = device;

		ret = audio_mcu_communication_cmd_adec1(&audio_metadata_packet, &audio_device);
	} else if (item == 2) {
		struct audio_metadata_struct audio_metadata_packet;
		struct audio_mcu_device_config audio_device;
		uint8_t device = AUDIO_MCU_DEVICE_ASND;

		audio_metadata_packet.audio_meta_value1 = 0x0;
		audio_metadata_packet.audio_meta_value2 = 0x0;
		audio_metadata_packet.audio_meta_value3 = 0x0;
		audio_metadata_packet.audio_mcu_device_id = device;

		audio_device.device = device;

		ret = audio_mcu_communication_cmd_asnd(&audio_metadata_packet, &audio_device);
	} else if (item == 3) {
		audio_err("%s to-do cmd\n", __func__);
	} else
		audio_err("item is not support %ld\n", item);
	return count;
}
static DEVICE_ATTR_RW(audio_test_cmd);

///sys/devices/platform/audio-mcu.0/mtk_dbg//audio_communication_cmd
static ssize_t audio_communication_cmd_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int config = audio_mcu_communication_get_config();

	return scnprintf(buf, MAX_LEN, "%u\n", config);
}

static ssize_t audio_communication_cmd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;

	if (item >= 0 && item < 3)
		ret = audio_mcu_communication_set_config(item);
	else
		audio_err("item is not support %ld\n", item);
	return count;
}
static DEVICE_ATTR_RW(audio_communication_cmd);

///sys/devices/platform/audio-mcu.0/mtk_dbg//audio_performance_cmd
static ssize_t audio_performance_cmd_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int config = audio_mcu_communication_get_performance();

	return scnprintf(buf, MAX_LEN, "%u\n", config);
}

static ssize_t audio_performance_cmd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;

	if (item >= 0 && item < 5)
		ret = audio_mcu_communication_set_performance(item);
	else
		audio_err("item is not support %ld\n", item);
	return count;
}
static DEVICE_ATTR_RW(audio_performance_cmd);

///sys/devices/platform/audio-mcu.0/mtk_dbg/log_level
static ssize_t log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, MAX_LEN, "%lu\n", mtk_log_level);
}

static ssize_t log_level_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long level = 0;
	int ret;

	ret = kstrtoul(buf, 10, &level);

	if (ret != 0)
		return ret;
	if (level >= 0 && level <= 7)
		mtk_log_level = level;
	return count;

}
static DEVICE_ATTR_RW(log_level);

static struct attribute *mtk_dtv_audio_mcu_attrs[] = {
	&dev_attr_audio_test_cmd.attr,
	&dev_attr_audio_communication_cmd.attr,
	&dev_attr_audio_performance_cmd.attr,
	&dev_attr_log_level.attr,
	NULL,
};

static const struct attribute_group mtk_dtv_audio_mcu_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_audio_mcu_attrs
};

static int mtk_dtv_audio_mcu_create_sysfs_attr(struct platform_device *pdev)
{
	int ret = 0;

	/* Create device attribute files */
	audio_err("Create device attribute files\n");
	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_dtv_audio_mcu_attr_group);
	if (ret)
		audio_err("[%d]Fail to create sysfs files: %d\n", __LINE__, ret);
	return ret;
}

static int mtk_dtv_audio_mcu_remove_sysfs_attr(struct platform_device *pdev)
{
	/* Remove device attribute files */
	audio_err("Remove device attribute files\n");
	sysfs_remove_group(&pdev->dev.kobj, &mtk_dtv_audio_mcu_attr_group);
	return 0;
}

static int audio_card_pcm_new(struct mtk_snd_audio *audio,
			    int device, int playback_substreams, int capture_substreams)
{
	struct snd_pcm *pcm;
	int err;

	if (audio == NULL) {
		audio_err("[%s][%d]Invalid argument\n", __func__, __LINE__);
		return -EINVAL;
	}

	err = snd_pcm_new(audio->card, SND_PCM_NAME, device,
			  playback_substreams, capture_substreams, &pcm);

	if (err < 0) {
		audio_err("[%s][%d]Fail to create pcm instance err=%d\n"
			, __func__, __LINE__, err);
		return err;
	}

	if (playback_substreams)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &audio_mcu_pcm_ops);

	if (capture_substreams)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &audio_mcu_pcm_ops);

	pcm->private_data = audio;
	pcm->info_flags = 0;
	pcm->nonatomic = true;
	strlcpy(pcm->name, SND_PCM_NAME, sizeof(pcm->name));

	audio->pcm[device] = pcm;

	return err;
}

static int audio_card_mixer_new(struct mtk_snd_audio *audio)
{
	struct snd_card *card = audio->card;
	struct snd_kcontrol *kcontrol;
	unsigned int idx;
	int err;

	audio_err("[%s][%d]\n", __func__, __LINE__);
	strlcpy(card->mixername, "audio_snd_card", sizeof(card->mixername));
	for (idx = 0; idx < ARRAY_SIZE(audio_snd_controls); idx++) {
		kcontrol = snd_ctl_new1(&audio_snd_controls[idx], audio);
		err = snd_ctl_add(card, kcontrol);
		if (err < 0)
			return err;
	}

	return 0;
}

static int audio_card_dai_init(void)
{
	// TO-DO

	return 0;
}

static int audio_card_dma_init(struct platform_device *pdev, struct mtk_snd_audio *audio)
{
	int ret = 0;

	if (pdev == NULL) {
		audio_err("pdev is NULL\n");
		return -ENODEV;
	}

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(SND_DMA_BIT_MASK))) {
		audio_err("dma_set_mask_and_coherent failed\n");
		return -ENODEV;
	}

	audio->dbgInfo.writed = 0;
	memset(&audio->dbgInfo, 0, sizeof(struct audio_dma_dbg));
	memset(audio->dbgInfo.log, 0, sizeof(struct operation_log) * MAX_LOG_LEN);

	return ret;
}

static int audio_card_machine_init(struct platform_device *pdev,  struct mtk_snd_audio *audio)
{
	int ret = 0;
	uint16_t i = 0;
	uint32_t offset = 0;
	struct audio_card_drvdata *card_drvdata = audio->audio_card;
	struct device_node *np;
	struct of_mmap_info_data of_mmap_info = {0};

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-audio-mcu-adv");

	// mapping device buffer size
	for (i = 0 ; i < AUDIO_MCU_DEVICE_NUM_MAX ; i++)
		of_property_read_u32(np, audio_mcu_device_size[i],
			&card_drvdata->device_buf_info[i].in_buf_max_size);

	audio_info("[%s] mcu device buffer : 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC1].in_buf_max_size,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC2].in_buf_max_size,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ASND].in_buf_max_size,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADVS].in_buf_max_size,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_AENC].in_buf_max_size
	);

	ret = of_mtk_get_reserved_memory_info_by_idx(np, 0, &of_mmap_info);
	if (ret == 0) {
		card_drvdata->audio_buf_paddr = of_mmap_info.start_bus_address;
		card_drvdata->audio_buf_size = of_mmap_info.buffer_size;
	} else {
		audio_err("%s reserved_memory error.\n", __func__);
		return ret;
	}

	// mapping device input buffer addr
	for (i = 0 ; i < AUDIO_MCU_DEVICE_NUM_MAX ; i++) {
		of_property_read_u32(np, audio_mcu_device_offset[i],
			&card_drvdata->device_buf_info[i].in_buf_offset);
		offset = card_drvdata->audio_buf_paddr +
			card_drvdata->device_buf_info[i].in_buf_offset;

		card_drvdata->device_buf_info[i].in_buf_addr = offset;
		audio_info("%s ID:%d buf addr:0x%x\n",  __func__, i,
			card_drvdata->device_buf_info[i].in_buf_addr);
	}

	audio_info("[%s] mcu device input buffer : 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC1].in_buf_addr,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC2].in_buf_addr,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ASND].in_buf_addr,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADVS].in_buf_addr,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_AENC].in_buf_addr
	);

	// mapping device parm addr
	for (i = 0 ; i < AUDIO_MCU_DEVICE_NUM_MAX ; i++) {
		card_drvdata->device_buf_info[i].parm_addr =
			card_drvdata->audio_buf_paddr + SND_MCU_PARM_SIZE * i +
			card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_AENC].in_buf_offset;
	}

	audio_info("[%s] mcu device parm : 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC1].parm_addr,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADEC2].parm_addr,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ASND].parm_addr,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_ADVS].parm_addr,
		card_drvdata->device_buf_info[AUDIO_MCU_DEVICE_AENC].parm_addr
	);

	card_drvdata->card_reg = card_reg;

	return ret;
}

static int audio_card_module_init(struct platform_device *pdev, struct mtk_snd_audio *audio)
{
	int ret = 0;
	// static struct task_struct *t;
	struct audio_card_drvdata *card_drvdata = NULL;

	card_drvdata = audio->audio_card;
	if (!card_drvdata) {
		card_drvdata = kzalloc(sizeof(struct audio_card_drvdata), GFP_KERNEL);
		if (!card_drvdata)
			return -ENOMEM;
		audio->audio_card = card_drvdata;
	}

	// DAI -> DMA -> MACHINE
	// 1. DAI initial
	ret = audio_card_dai_init();
	if (ret < 0) {
		audio_err("Fail to init DAI:%d\n", ret);
		return ret;
	}

	// 2. DMA initial
	ret = audio_card_dma_init(pdev, audio);
	if (ret < 0) {
		audio_err("Fail to init DMA:%d\n", ret);
		return ret;
	}

	// 3. MACHINE initial
	ret = audio_card_machine_init(pdev, audio);
	if (ret < 0) {
		audio_err("Fail to init MACHINE:%d\n", ret);
		return ret;
	}

	// add to mtk_snd_audio list
	INIT_LIST_HEAD(&audio->list);
	list_add_tail(&audio->list, &audio_card_dev_list);

	audio_info("[%s][%d]Initial success\n", __func__, __LINE__);
	return ret;
}

static int audio_mcu_card_probe(struct platform_device *pdev)
{
	struct snd_card *card;
	struct mtk_snd_audio *audio;
	struct device_node *np;
	uint16_t i = 0;
	int dev = pdev->id;
	int ret;

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-audio-mcu-adv");
	if (!of_device_is_available(np)) {
		audio_err("[%s][%d]node status disable\n",
			__func__, __LINE__);
		ret = -ENODEV;
		return ret;
	}

	// Start to new card
	ret = snd_card_new(&pdev->dev, index[dev], id[dev],
		THIS_MODULE, sizeof(struct mtk_snd_audio), &card);
	if (ret < 0) {
		audio_err("[%s][%d]Fail to create card error:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}

	audio = card->private_data;
	audio->card = card;

	// new pcm
	for (i = 0 ; i < AUDIO_MCU_DEVICE_NUM_MAX ; i++) {
		ret = audio_card_pcm_new(audio, i,
			pcm_playback_substreams[dev], pcm_capture_substreams[dev]);
		if (ret < 0)
			audio_err("[%s][%d]audio_card_pcm_new: %d fail: %d\n",
				__func__, __LINE__, i, ret);
	}

	audio_info("[%s][%d] pcm_substreams[%d] = %d %d\n", __func__, __LINE__,
		dev,
		pcm_playback_substreams[dev],
		pcm_capture_substreams[dev]);

	// new mixer
	ret = audio_card_mixer_new(audio);
	if (ret < 0) {
		audio_err("[%s][%d]Fail to create mixer error:%d\n",
			__func__, __LINE__, ret);
		goto nodev;
	}

	strlcpy(card->driver, "audiomcusndcard", sizeof(card->driver));
	strlcpy(card->shortname, "audiomcusndcard", sizeof(card->shortname));
	snprintf(card->longname, sizeof(card->longname), "audiomcusndcard %i", dev + 1);

	snd_card_set_dev(card, &pdev->dev);
	audio->dev = &pdev->dev;

	ret = snd_card_register(card);
	if (!ret)
		platform_set_drvdata(pdev, card);

	ret = audio_card_module_init(pdev, audio);
	if (ret < 0) {
		audio_err("[%s][%d]Fail to initial module:%d\n",
			__func__, __LINE__, ret);
		goto nodev;
	}

	mtk_dtv_audio_mcu_create_sysfs_attr(pdev);

	return 0;

nodev:
	snd_card_free(card);
	return ret;
}

static int audio_mcu_card_remove(struct platform_device *pdev)
{
	mtk_dtv_audio_mcu_remove_sysfs_attr(pdev);
	return 0;
}

static struct platform_driver audio_mcu_card_driver = {
	.probe = audio_mcu_card_probe,
	.remove = audio_mcu_card_remove,
	.driver = {
			.name = SND_AUDIO_MCU_DRIVER,
			.owner = THIS_MODULE,
			},
};

static int __init audio_mcu_card_init(void)
{
	int err;
	struct mtk_audio_mcu_communication audio_mcu_communication;

	audio_err("[%s][%d]\n", __func__, __LINE__);

	err = platform_driver_register(&audio_mcu_card_driver);
	if (err < 0)
		return err;

	platform_device_register_simple(SND_AUDIO_MCU_DRIVER, 0, NULL, 0);

	// Init communication
	audio_mcu_communication_init();

	audio_mcu_communication.audio_parameter_cb = audio_mcu_parameter_cb;
	audio_mcu_communication.audio_shm_buffer_cb = audio_mcu_shm_buffer_cb;
	audio_mcu_communication_register_notify(&audio_mcu_communication);

	return 0;
}

static void __exit audio_mcu_card_exit(void)
{
	audio_err("%s\n", __func__);
}

module_init(audio_mcu_card_init);
module_exit(audio_mcu_card_exit);

/* Module information */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Audio ALSA mcu driver");
