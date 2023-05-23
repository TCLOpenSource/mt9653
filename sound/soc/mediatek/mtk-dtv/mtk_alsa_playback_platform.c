// SPDX-License-Identifier: GPL-2.0
/*
 * mtk_alsa_playback_platform.c  --  Mediatek DTV playback function
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
#include "mtk_alsa_playback_platform.h"
#include "mtk-reserved-memory.h"

#define PLAYBACK_DMA_NAME		"snd-output-pcm-dai"

#define MAX_PCM_BUF_SIZE	(48*1024)

#define EXPIRATION_TIME	500

/* Channel mode */
#define CH_1		1
#define CH_2		2
#define CH_8		8
#define CH_12		12

#define ALSA_AUDIO_ENTER(dev, id, priv)	\
({do {AUD_DEV_PRINT_PB(dev, id, AUD_INFO, priv	\
, "[%s:%d] !!!\n", __func__, __LINE__); } while (0); })


static struct playback_register REG;

/* 1k -18dB tone, mono, 16bit, 48khz */
#define	SINETONE_SIZE	96
unsigned char sinetone[SINETONE_SIZE] = {
0x01, 0x00, 0x1A, 0x02, 0x2B, 0x04, 0x2B, 0x06, 0x0E, 0x08,
0xCF, 0x09, 0x65, 0x0B, 0xC8, 0x0C, 0xF4, 0x0D, 0xE3, 0x0E,
0x91, 0x0F, 0xFA, 0x0F, 0x1E, 0x10, 0xFA, 0x0F, 0x91, 0x0F,
0xE3, 0x0E, 0xF4, 0x0D, 0xC9, 0x0C, 0x64, 0x0B, 0xD0, 0x09,
0x0F, 0x08, 0x2B, 0x06, 0x2C, 0x04, 0x1B, 0x02, 0x00, 0x00,
0xE6, 0xFD, 0xD5, 0xFB, 0xD6, 0xF9, 0xF2, 0xF7, 0x31, 0xF6,
0x9B, 0xF4, 0x38, 0xF3, 0x0B, 0xF2, 0x1D, 0xF1, 0x70, 0xF0,
0x06, 0xF0, 0xE3, 0xEF, 0x06, 0xF0, 0x6F, 0xF0, 0x1D, 0xF1,
0x0B, 0xF2, 0x37, 0xF3, 0x9B, 0xF4, 0x31, 0xF6, 0xF1, 0xF7,
0xD5, 0xF9, 0xD5, 0xFB, 0xE6, 0xFD,
};

/* Speaker */
static unsigned int pb_dma_playback_rates[] = {
	48000,
};

static unsigned int pb_dma_playback_channels[] = {
	2, 8, 12
};
/* Headphone, Lineout */
static unsigned int pb_hp_playback_rates[] = {
	48000,
};

static unsigned int pb_hp_playback_channels[] = {
	2,
};

static const struct snd_pcm_hw_constraint_list constraints_pb_dma_rates = {
	.count = ARRAY_SIZE(pb_dma_playback_rates),
	.list = pb_dma_playback_rates,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list constraints_pb_dma_channels = {
	.count = ARRAY_SIZE(pb_dma_playback_channels),
	.list = pb_dma_playback_channels,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list constraints_pb_hp_rates = {
	.count = ARRAY_SIZE(pb_hp_playback_rates),
	.list = pb_hp_playback_rates,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list constraints_pb_hp_channels = {
	.count = ARRAY_SIZE(pb_hp_playback_channels),
	.list = pb_hp_playback_channels,
	.mask = 0,
};

/* Default hardware settings of ALSA PCM playback */
static struct snd_pcm_hardware pb_dma_playback_default_hw = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE |
				SNDRV_PCM_FMTBIT_S32_BE | SNDRV_PCM_FMTBIT_S24_LE,
	.channels_min = 2,
	.channels_max = 12,
	.buffer_bytes_max = MAX_PCM_BUF_SIZE,
	.period_bytes_min = 64,
	.period_bytes_max = 1024 * 8,
	.periods_min = 2,
	.periods_max = 8,
	.fifo_size = 0,
};

/* Headphone, Lineout hardware settings of ALSA PCM playback */
static struct snd_pcm_hardware pb_dma_playback_hp_hw = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE |
				SNDRV_PCM_FMTBIT_S32_BE | SNDRV_PCM_FMTBIT_S24_LE,
	.buffer_bytes_max = MAX_PCM_BUF_SIZE,
	.period_bytes_min = 64,
	.period_bytes_max = (MAX_PCM_BUF_SIZE >> 2),
	.periods_min = 1,
	.periods_max = 1024,
	.fifo_size = 0,
};

static int playback_get_output_port(const char *sink)
{
	if (!strncmp(sink, "DAC0", strlen("DAC0")))
		return AUDIO_OUT_DAC0;
	else if (!strncmp(sink, "DAC1", strlen("DAC1")))
		return AUDIO_OUT_DAC1;
	else if (!strncmp(sink, "DAC2", strlen("DAC2")))
		return AUDIO_OUT_DAC2;
	else if (!strncmp(sink, "DAC3", strlen("DAC3")))
		return AUDIO_OUT_DAC3;
	else if (!strncmp(sink, "I2S0", strlen("I2S0")))
		return AUDIO_OUT_I2S0;
	else if (!strncmp(sink, "I2S1", strlen("I2S1")))
		return AUDIO_OUT_I2S1;
	else if (!strncmp(sink, "I2S5", strlen("I2S5")))
		return AUDIO_OUT_I2S5;
	else
		return AUDIO_OUT_NULL;
}

static void dma_reader_timer_open(struct timer_list *timer,
					unsigned long data, void *func)
{
	if (timer == NULL) {
		pr_err("[ALSA PB]spk timer pointer is invalid.\n");
		return;
	} else if (func == NULL) {
		pr_err("[ALSA PB]spk timer callback function is invalid.\n");
		return;
	}

	timer_setup(timer, func, 0);

	timer->expires = 0;
}

static void dma_reader_timer_close(struct timer_list *timer)
{
	if (timer == NULL) {
		pr_err("[ALSA PB]spk timer pointer is invalid.\n");
		return;
	}

	del_timer_sync(timer);
	memset(timer, 0x00, sizeof(struct timer_list));
}

static int dma_reader_timer_reset(struct timer_list *timer)
{
	if (timer == NULL) {
		pr_err("[ALSA PB]spk timer pointer is invalid.\n");
		return -EINVAL;
	} else if (timer->function == NULL) {
		pr_err("[ALSA PB]spk timer callback function is invalid.\n");
		return -EINVAL;
	}

	mod_timer(timer, (jiffies + msecs_to_jiffies(20)));

	return 0;
}

static int dma_reader_timer_update(struct timer_list *timer,
				unsigned long time_interval)
{
	if (timer == NULL) {
		pr_err("[ALSA PB]spk timer pointer is invalid.\n");
		return -EINVAL;
	} else if (timer->function == NULL) {
		pr_err("[ALSA PB]spk timer callback function is invalid.\n");
		return -EINVAL;
	}

	mod_timer(timer, (jiffies + time_interval));

	return 0;
}

static void chg_finished(struct mtk_pb_runtime_struct *dev_runtime)
{
	unsigned int chg_reg_value;

	chg_reg_value = mtk_alsa_read_reg(REG.PB_CHG_FINISHED);

	if ((chg_reg_value & 0x4) && (dev_runtime->chg_finished == 0)) {
		dev_runtime->chg_finished = 1;
		pr_info("[%s] : Change charge state to finished!!\n", __func__);
	}
	if ((dev_runtime->sink_dev == AUDIO_OUT_DAC0 ||
			dev_runtime->sink_dev == AUDIO_OUT_DAC1) &&
			(dev_runtime->chg_finished == 0))
		pr_info("[%s] : Charge not finished!!\n", __func__);
}

static void dma_register_get_reg(int id, u32 *ctrl, u32 *wptr, u32 *level)
{
	switch (id)  {
	case SPEAKER_PLAYBACK:
		*ctrl = REG.PB_DMA1_CTRL;
		*wptr = REG.PB_DMA1_WPTR;
		*level = REG.PB_DMA1_DDR_LEVEL;
		break;
	case HEADPHONE_PLAYBACK:
		*ctrl = REG.PB_DMA2_CTRL;
		*wptr = REG.PB_DMA2_WPTR;
		*level = REG.PB_DMA2_DDR_LEVEL;
		break;
	case LINEOUT_PLAYBACK:
		*ctrl = REG.PB_DMA3_CTRL;
		*wptr = REG.PB_DMA3_WPTR;
		*level = REG.PB_DMA3_DDR_LEVEL;
		break;
	case HFP_PLAYBACK:
		*ctrl = REG.PB_DMA4_CTRL;
		*wptr = REG.PB_DMA4_WPTR;
		*level = REG.PB_DMA4_DDR_LEVEL;
		break;
	default:
		break;
	}
}

static void dma_register_get_offset(int id, struct playback_priv *priv, unsigned int *offset)
{
	switch (id)  {
	case SPEAKER_PLAYBACK:
		*offset = priv->spk_dma_offset;
		break;
	case HEADPHONE_PLAYBACK:
		*offset = priv->hp_dma_offset;
		break;
	case LINEOUT_PLAYBACK:
		*offset = priv->lineout_dma_offset;
		break;
	case HFP_PLAYBACK:
		*offset = priv->bt_dma_offset;
		break;
	default:
		break;
	}
}

static void dma_reader_monitor(struct timer_list *t)
{
	struct mtk_pb_runtime_struct *dev_runtime = NULL;
	struct mtk_monitor_struct *monitor = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_pcm_runtime *runtime = NULL;
	char time_interval = 1;
	unsigned int expiration_counter = EXPIRATION_TIME;
	struct playback_priv *priv;
	int id;

	dev_runtime = from_timer(dev_runtime, t, timer);

	if (dev_runtime == NULL) {
		pr_err("[ALSA PB]spk invalid parameters!\n");
		return;
	}

	monitor = &dev_runtime->monitor;
	priv = (struct playback_priv *) dev_runtime->private;
	id = dev_runtime->id;

	if (monitor->monitor_status == CMD_STOP) {
		AUD_PR_PRINT_PB(id, AUD_INFO, priv,
			"[ALSA PB]spk No action is required, exit Monitor\n");
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
					AUD_PR_PRINT_PB(id, AUD_INFO, priv,
						"[ALSA PB]spk exit Monitor\n");
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

	if (dma_reader_timer_update(&dev_runtime->timer,
				msecs_to_jiffies(time_interval)) != 0) {
		AUD_PR_PRINT_PB(id, AUD_ERR, priv,
			"[ALSA PB]spk fail to update timer for Monitor!\n");
		spin_lock(&dev_runtime->spin_lock);
		memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
		spin_unlock(&dev_runtime->spin_lock);
	}
}

static int dma_reader_init(struct playback_priv *priv,
			struct mtk_pb_runtime_struct *dev_runtime, int id)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	ptrdiff_t dmaRdr_base_pa = 0;
	ptrdiff_t dmaRdr_base_ba = 0;
	ptrdiff_t dmaRdr_base_va = pcm_playback->str_mode_info.virtual_addr;
	unsigned int dmaRdr_offset = priv->spk_dma_offset;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;
	int ret = 0;
	AUD_PR_PRINT_PB(id, AUD_INFO, priv, "Initiate MTK PCM Playback engine\n");
	dma_register_get_reg(id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);
	dma_register_get_offset(id, priv, &dmaRdr_offset);

	if ((pcm_playback->initialized_status != 1) ||
		(pcm_playback->status != CMD_RESUME)) {
		dmaRdr_base_pa = priv->mem_info.bus_addr -
				priv->mem_info.memory_bus_base + dmaRdr_offset;
		dmaRdr_base_ba = priv->mem_info.bus_addr + dmaRdr_offset;

		if ((dmaRdr_base_ba % 0x1000)) {
			pr_err("[ALSA PB]spk invalid addr,should align 4KB\n");
			return -EFAULT;
		}

		/* convert Bus Address to Virtual Address */
		if (dmaRdr_base_va == 0) {
			dmaRdr_base_va = (ptrdiff_t)ioremap_wc(dmaRdr_base_ba,
						pcm_playback->buffer.size);
			if (dmaRdr_base_va == 0) {
				pr_err("[ALSA PB]spk fail to convert addr\n");
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

	/* reset PCM playback engine */
	mtk_alsa_write_reg(REG_CTRL, 0x0001);

	/* clear PCM playback write pointer */
	mtk_alsa_write_reg(REG_WPTR, 0x0000);

	/* reset written size */
	pcm_playback->written_size = 0;

	/* clear PCM playback pcm buffer */
	memset((void *)pcm_playback->buffer.addr, 0x00,
					pcm_playback->buffer.size);
	smp_mb();

	if ((id == SPEAKER_PLAYBACK) && (priv->hfp_enable_flag)) {
		ret = mtk_i2s_tx_clk_set_enable(dev_runtime->format, 48000);
		if (dev_runtime->format == SNDRV_PCM_FORMAT_S24_LE)
			mtk_alsa_write_reg_mask(REG.PB_I2S_BCK_WIDTH, 0x0007, 0x0005);
		else if (dev_runtime->format == SNDRV_PCM_FORMAT_S32_LE)
			mtk_alsa_write_reg_mask(REG.PB_I2S_BCK_WIDTH, 0x0007, 0x0006);
		else
			mtk_alsa_write_reg_mask(REG.PB_I2S_BCK_WIDTH, 0x0007, 0x0004);
	}

	if (dev_runtime->format == SNDRV_PCM_FORMAT_S24_LE) {
		mtk_alsa_write_reg_mask_byte(REG_CTRL, 0x30, 0x10);
		dev_runtime->byte_length = 3;
	} else if (dev_runtime->format == SNDRV_PCM_FORMAT_S32_LE) {
		mtk_alsa_write_reg_mask_byte(REG_CTRL, 0x30, 0x20);
		dev_runtime->byte_length = 4;
	} else {
		mtk_alsa_write_reg_mask_byte(REG_CTRL, 0x30, 0x00);
		dev_runtime->byte_length = 2;
	}

	if ((dev_runtime->channel_mode == CH_12) && (!priv->hfp_enable_flag))
		mtk_alsa_write_reg_mask_byte(REG_CTRL, 0xC0, 0x40);
	else if ((dev_runtime->channel_mode == CH_8)
			|| (dev_runtime->channel_mode == CH_12))
		mtk_alsa_write_reg_mask_byte(REG_CTRL, 0xC0, 0x80);
	else
		mtk_alsa_write_reg_mask_byte(REG_CTRL, 0xC0, 0x00);

	return ret;
}

static int dma_reader_exit(struct mtk_pb_runtime_struct *dev_runtime, int id)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	ptrdiff_t dmaRdr_base_va = pcm_playback->str_mode_info.virtual_addr;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;
	//pr_info("Exit MTK PCM Playback engine\n");
	dma_register_get_reg(id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);

	/* reset PCM playback engine */
	mtk_alsa_write_reg(REG_CTRL, 0x0001);

	/* clear PCM playback write pointer */
	mtk_alsa_write_reg(REG_WPTR, 0x0000);

	if (dmaRdr_base_va != 0) {
		if (pcm_playback->buffer.addr) {
			iounmap((void *)pcm_playback->buffer.addr);
			pcm_playback->buffer.addr = 0;
		} else
			pr_err("[ALSA PB]spk buf addr should not be 0\n");

		pcm_playback->str_mode_info.virtual_addr = 0;
	}

	pcm_playback->status = CMD_STOP;
	pcm_playback->initialized_status = 0;

	return 0;
}

static int dma_reader_start(struct snd_soc_component *component,
		struct mtk_pb_runtime_struct *dev_runtime, int id)
{
	struct snd_soc_card *card = component->card;
	const struct snd_soc_dapm_route *routes = card->of_dapm_routes;
	const char *sink = NULL;
	int port;
	int i;
	const char *source;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;

	dma_register_get_reg(id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);

	switch (id)  {
	case SPEAKER_PLAYBACK:
		source = "SPEAKER_PLAYBACK";
		break;
	case HEADPHONE_PLAYBACK:
		source = "HEADPHONE_PLAYBACK";
		break;
	case LINEOUT_PLAYBACK:
		source = "LINEOUT_PLAYBACK";
		break;
	case HFP_PLAYBACK:
		source = "HFP_PLAYBACK";
		break;
	default:
		source = "NO_THIS_PLAYBACK_TYPE";
		break;
	}

	// pr_info("Start MTK PCM Playback engine\n");
	for (i = 0; i < card->num_of_dapm_routes; i++)
		if (!strncmp(routes[i].source, source, strlen(source)))
			sink = routes[i].sink;

	if (sink != NULL) {
		/* set speaker dma to output port */
		port = playback_get_output_port(sink);
		mtk_alsa_write_reg_mask(REG_CTRL, 0xF000, port << 12);
		dev_runtime->sink_dev = port;
	} else {
		pr_err("[ALSA PB]spk can't get output port will no sound\n");
		dev_runtime->sink_dev = 0;
	}
	/* start PCM playback engine */
	mtk_alsa_write_reg_mask(REG_CTRL, 0x0003, 0x0002);

	/* trigger DSP to reset DMA engine */
	mtk_alsa_write_reg_mask(REG_CTRL, 0x0001, 0x0001);
	mtk_alsa_delaytask_us(50);
	mtk_alsa_write_reg_mask(REG_CTRL, 0x0001, 0x0000);

	return 0;
}

static int dma_reader_stop(struct mtk_pb_runtime_struct *dev_runtime, int id)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;
	//pr_info("Stop MTK PCM Playback engine\n");

	dma_register_get_reg(id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);

	/* reset PCM playback engine */
	mtk_alsa_write_reg_mask(REG_CTRL, 0x0001, 0x0001);

	/* stop PCM playback engine */
	mtk_alsa_write_reg_mask(REG_CTRL, 0x0002, 0x0000);

	/* clear PCM playback write pointer */
	mtk_alsa_write_reg(REG_WPTR, 0x0000);

	/* reset Write Pointer */
	pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;

	/* reset written size */
	pcm_playback->written_size = 0;

	pcm_playback->status = CMD_STOP;

	return 0;
}

static int dma_reader_set_sample_rate(struct mtk_pb_runtime_struct *dev_runtime,
					unsigned int sample_rate)
{
	struct playback_priv *priv;
	int id;

	priv = (struct playback_priv *) dev_runtime->private;
	id = dev_runtime->id;

	AUD_PR_PRINT_PB(id, AUD_INFO, priv,
	"[ALSA PB]spk target sample rate is %u\n", sample_rate);

	dev_runtime->sample_rate = sample_rate;

	return 0;
}

static void dma_reader_set_channel_mode(struct playback_priv *priv,
	struct mtk_pb_runtime_struct *dev_runtime, unsigned int channel_mode, int id)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	unsigned int buffer_size = 0;

	AUD_PR_PRINT_PB(id, AUD_INFO, priv,
	"[ALSA PB]spk target channel mode is %u\n", channel_mode);

	switch (id)  {
	case SPEAKER_PLAYBACK:
		buffer_size = priv->spk_dma_size;
		break;
	case HEADPHONE_PLAYBACK:
		buffer_size = priv->hp_dma_size;
		break;
	case LINEOUT_PLAYBACK:
		buffer_size = priv->lineout_dma_size;
		break;
	case HFP_PLAYBACK:
		buffer_size = priv->bt_dma_size;
		break;
	default:
		break;
	}

	dev_runtime->channel_mode = channel_mode;
	pcm_playback->period_size = buffer_size >> 2;
	pcm_playback->buffer.size = buffer_size;
	pcm_playback->high_threshold =
		pcm_playback->buffer.size - (pcm_playback->buffer.size >> 3);

}

static int dma_reader_get_inused_lines(int id)
{
	int inused_lines = 0;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;

	dma_register_get_reg(id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);

	mtk_alsa_write_reg_mask(REG_CTRL, 0x0008, 0x0008);
	inused_lines = mtk_alsa_read_reg(REG_DDR_LEVEL);
	mtk_alsa_write_reg_mask(REG_CTRL, 0x0008, 0x0000);

	return inused_lines;
}

static void dma_reader_get_avail_bytes
			(struct mtk_pb_runtime_struct *dev_runtime, int id)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	int inused_lines = 0;
	int avail_lines = 0;
	int avail_bytes = 0;

	inused_lines = dma_reader_get_inused_lines(id);
	avail_lines = (pcm_playback->buffer.size / BYTES_IN_MIU_LINE) -
					inused_lines;

	if (avail_lines < 0) {
		pr_err("[ALSA PB]spk incorrect inused line %d\n", inused_lines);
		avail_lines = 0;
	}

	avail_bytes = avail_lines * BYTES_IN_MIU_LINE;

	dev_runtime->buffer.avail_size = avail_bytes;
}

static int dma_reader_get_consumed_bytes
		(struct mtk_pb_runtime_struct *dev_runtime, int id)
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

	return consumed_bytes;
}

static unsigned int dma_reader_write_zero(struct mtk_pb_runtime_struct *dev_runtime,
					unsigned int bytes)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	unsigned int copy_size = bytes;
	int loop = 0;

	for (loop = 0; loop < copy_size; loop++) {
		*(pcm_playback->buffer.w_ptr++) = 0;

		if (pcm_playback->buffer.w_ptr >= (pcm_playback->buffer.addr +
					pcm_playback->buffer.size))
			pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;
	}
	return copy_size;
}

static unsigned int dma_reader_write_inject(struct mtk_pb_runtime_struct *dev_runtime,
					unsigned int samples)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	unsigned char *buffer_tmp;
	unsigned char byte_hi = 0;
	unsigned char byte_lo = 0;
	unsigned int copy_sample = samples;
	unsigned int frame_size = dev_runtime->channel_mode * dev_runtime->byte_length;
	int loop = 0;
	int i = 0;
	int j = 0;

	if ((dev_runtime->inject.r_ptr + sizeof(short)) > SINETONE_SIZE)
		dev_runtime->inject.r_ptr = 0;

	if ((pcm_playback->buffer.w_ptr + frame_size) > (pcm_playback->buffer.addr +
				pcm_playback->buffer.size))
		pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;

	for (loop = 0; loop < copy_sample; loop++) {
		buffer_tmp = sinetone + dev_runtime->inject.r_ptr;
		if (buffer_tmp >= (sinetone + SINETONE_SIZE))
			buffer_tmp = sinetone;
		byte_lo = *(buffer_tmp++);
		if (buffer_tmp >= (sinetone + SINETONE_SIZE))
			buffer_tmp = sinetone;
		byte_hi = *(buffer_tmp++);
		for (i = 0; i < dev_runtime->channel_mode; i++) {
			for (j = 0; j < (dev_runtime->byte_length - sizeof(short)); j++)
				*(pcm_playback->buffer.w_ptr++) = 0;

			*(pcm_playback->buffer.w_ptr++) = byte_lo;
			*(pcm_playback->buffer.w_ptr++) = byte_hi;
		}

		if (pcm_playback->buffer.w_ptr >= (pcm_playback->buffer.addr +
					pcm_playback->buffer.size))
			pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;

		dev_runtime->inject.r_ptr += sizeof(short);
		if (dev_runtime->inject.r_ptr >= SINETONE_SIZE)
			dev_runtime->inject.r_ptr = 0;
	}
	return copy_sample;
}

static unsigned int dma_reader_write(struct mtk_pb_runtime_struct *dev_runtime,
					void *buffer, unsigned int bytes)
{
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	unsigned char *buffer_tmp = (unsigned char *)buffer;
	unsigned int w_ptr_offset = 0;
	unsigned int copy_sample = 0;
	unsigned int copy_size = 0;
	int inused_bytes = 0;
	int loop = 0;
	static unsigned int warning_cnt;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;
	struct playback_priv *priv;
	int id;

	priv = (struct playback_priv *) dev_runtime->private;
	id = dev_runtime->id;

	dma_register_get_reg(id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);

	if ((dev_runtime->channel_mode == 0) || (dev_runtime->byte_length == 0)) {
		pr_err("%s[%d] devision by zero error!\n", __func__, __LINE__);
		return 0;
	}

	copy_sample = bytes / (dev_runtime->channel_mode * dev_runtime->byte_length);
	copy_size = bytes;

	inused_bytes = dma_reader_get_inused_lines(id) * BYTES_IN_MIU_LINE;

	if ((inused_bytes == 0) && (pcm_playback->status == CMD_START)) {
		if (warning_cnt == 0)
			AUD_PR_PRINT_PB(id, AUD_INFO, priv,
			"[ALSA PB]*****SPK PB Buf empty !!*****\n");

		warning_cnt++;
	} else if ((inused_bytes + copy_size) > pcm_playback->high_threshold) {
		//pr_err("*****PCM Playback Buffer full !!*****\n");
		return 0;
	} else if (warning_cnt != 0) {
		AUD_PR_PRINT_PB(id, AUD_INFO, priv,
		"[ALSA PB]*****SPK PB Buf empty cnt = %d!!*****\n", warning_cnt);
		warning_cnt = 0;
	}

	/* open HP device */
	if (dev_runtime->sink_dev == AUDIO_OUT_DAC1 && dev_runtime->chg_finished)
		mtk_alsa_write_reg_mask(REG.PB_HP_OPENED, 0x1000, 0x1000);

	/* charging is not completed  */
	if ((dev_runtime->sink_dev == AUDIO_OUT_DAC0 ||
			dev_runtime->sink_dev == AUDIO_OUT_DAC1) &&
			(dev_runtime->chg_finished == 0))
		copy_size = dma_reader_write_zero(dev_runtime, copy_size);

	/* for inject debug mode */
	else if (dev_runtime->inject.status == 1)
		copy_sample = dma_reader_write_inject(dev_runtime, copy_sample);
	else {
		for (loop = 0; loop < copy_size; loop++) {
			*(pcm_playback->buffer.w_ptr++) = *(buffer_tmp++);

			if (pcm_playback->buffer.w_ptr >= (pcm_playback->buffer.addr +
						pcm_playback->buffer.size))
				pcm_playback->buffer.w_ptr = pcm_playback->buffer.addr;
		}
	}
	/* flush MIU */
	smp_mb();

	/* update PCM playback write pointer */
	w_ptr_offset = pcm_playback->buffer.w_ptr - pcm_playback->buffer.addr;
	mtk_alsa_write_reg_mask(REG_WPTR, 0xFFFF,
				(w_ptr_offset / BYTES_IN_MIU_LINE));
	pcm_playback->written_size += copy_size;
	pcm_playback->status = CMD_START;
	/* ensure write pointer can be applied */
	mtk_alsa_delaytask(1);

	return bytes;
}

static int dma_reader_fe_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct playback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	int ret;
	struct snd_pcm_hardware *pb_hw;
	const struct snd_pcm_hw_constraint_list *rate_hw, *channel_hw;

	dev_runtime->chg_finished = 0;
	dev_runtime->private = priv;
	dev_runtime->id = id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	if (id == SPEAKER_PLAYBACK) {
		pb_hw = &pb_dma_playback_default_hw;
		rate_hw = &constraints_pb_dma_rates;
		channel_hw = &constraints_pb_dma_channels;
	} else {
		pb_hw = &pb_dma_playback_hp_hw;
		rate_hw = &constraints_pb_hp_rates;
		channel_hw = &constraints_pb_hp_channels;
	}

	/* Set specified information */
	dev_runtime->substreams.p_substream = substream;

	snd_soc_set_runtime_hwparams(substream, pb_hw);

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, rate_hw);
	if (ret < 0) {
		AUD_DEV_PRINT_PB(dai->dev, id, AUD_ERR, priv,
		"snd_pcm_hw_constraint_list RATE  fail test ");
		return ret;
	}
	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_CHANNELS, channel_hw);
	if (ret < 0) {
		AUD_DEV_PRINT_PB(dai->dev, id, AUD_ERR, priv,
		"snd_pcm_hw_constraint_list  CHANNELS fail test ");
		return ret;
	}
	/* Allocate system memory for Specific Copy */
	dev_runtime->buffer.addr = vmalloc(MAX_PCM_BUF_SIZE);
	if (dev_runtime->buffer.addr == NULL)
		return -ENOMEM;

	memset((void *)dev_runtime->buffer.addr, 0x00, MAX_PCM_BUF_SIZE);
	dev_runtime->buffer.size = MAX_PCM_BUF_SIZE;

	spin_lock_init(&dev_runtime->spin_lock);
	dma_reader_timer_open(&dev_runtime->timer,
			(unsigned long)dev_runtime, (void *)dma_reader_monitor);

	/* Check if charging is complete */
	chg_finished(dev_runtime);
	return 0;
}

static void dma_reader_fe_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct playback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	unsigned long flags;

	/* Close HP device */
	if (dev_runtime->sink_dev == AUDIO_OUT_DAC1 && dev_runtime->chg_finished)
		mtk_alsa_write_reg_mask(REG.PB_HP_OPENED, 0x1000, 0x0000);

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	/* Reset some variables */
	dev_runtime->substreams.substream_status = CMD_STOP;
	dev_runtime->substreams.p_substream = NULL;

	/* Free allocated memory */
	if (dev_runtime->buffer.addr != NULL) {
		vfree(dev_runtime->buffer.addr);
		dev_runtime->buffer.addr = NULL;
	}

	dma_reader_timer_close(&dev_runtime->timer);

	spin_lock_irqsave(&dev_runtime->spin_lock, flags);
	memset(&dev_runtime->monitor, 0x00,
			sizeof(struct mtk_monitor_struct));
	spin_unlock_irqrestore(&dev_runtime->spin_lock, flags);

	/* Reset PCM configurations */
	dev_runtime->sample_rate = 0;
	dev_runtime->channel_mode = 0;
	dev_runtime->byte_length = 0;
	dev_runtime->runtime_status = CMD_STOP;
	dev_runtime->device_status = 0;

	/* Stop DMA Reader  */
	dma_reader_stop(dev_runtime, id);
	mtk_alsa_delaytask(2);

	/* Close MTK Audio DSP */
	dma_reader_exit(dev_runtime, id);
	mtk_alsa_delaytask(2);
}

static int dma_reader_fe_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct playback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	return 0;
}

static int dma_reader_fe_hw_free(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct playback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	return 0;
}

static int dma_reader_fe_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct playback_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct snd_soc_component *component = dai->component;

	dev_runtime->private = priv;
	dev_runtime->device_status = 1;
	ALSA_AUDIO_ENTER(dai->dev, id, priv);
	/*print hw parameter*/

	AUD_DEV_PRINT_PB(dai->dev, id, AUD_INFO, priv, "\nparameter :\n"
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

	if (dev_runtime->runtime_status != CMD_START) {
		/* Configure SW DMA sample rate */
		dma_reader_set_sample_rate(dev_runtime, runtime->rate);
		/* Configure SW DMA channel mode */
		dma_reader_set_channel_mode(priv, dev_runtime, runtime->channels, id);
		/* Open SW DMA */
		dev_runtime->format = runtime->format;
		dma_reader_init(priv, dev_runtime, id);
		/* Start SW DMA */
		dma_reader_start(component, dev_runtime, id);
		/* Configure SW DMA device status */
		dev_runtime->runtime_status = CMD_START;
	} else {
		/* Stop SW DMA */
		dma_reader_stop(dev_runtime, id);
		mtk_alsa_delaytask(2);
		/* Start SW DMA */
		dma_reader_start(component, dev_runtime, id);
	}

	return 0;
}

static int dma_reader_fe_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct playback_priv *priv = snd_soc_dai_get_drvdata(dai);
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
		pr_info("[ALSA PB]spk invalid PB's trigger command %d\n", cmd);
		err = -EINVAL;
		break;
	}

	return 0;
}

static int i2s_be_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	return 0;
}

static void i2s_be_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
}

static int i2s_be_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	return 0;
}

static int i2s_be_hw_free(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	return 0;
}

static int i2s_be_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	return 0;
}

static int i2s_be_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	return 0;
}

/* FE DAIs */
static const struct snd_soc_dai_ops dma_reader_fe_dai_ops = {
	.startup	= dma_reader_fe_startup,
	.shutdown	= dma_reader_fe_shutdown,
	.hw_params	= dma_reader_fe_hw_params,
	.hw_free	= dma_reader_fe_hw_free,
	.prepare	= dma_reader_fe_prepare,
	.trigger	= dma_reader_fe_trigger,
};

/* BE DAIs */
static const struct snd_soc_dai_ops i2s_be_dai_ops = {
	.startup	= i2s_be_startup,
	.shutdown	= i2s_be_shutdown,
	.hw_params	= i2s_be_hw_params,
	.hw_free	= i2s_be_hw_free,
	.prepare	= i2s_be_prepare,
	.trigger	= i2s_be_trigger,
};

int playback_dai_suspend(struct snd_soc_dai *dai)
{
	struct playback_priv *priv = snd_soc_dai_get_drvdata(dai);
	int id = dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;

	if (dev_runtime->runtime_status == CMD_STOP)
		return 0;

	dma_reader_stop(dev_runtime, id);

	pcm_playback->status = CMD_SUSPEND;
	dev_runtime->runtime_status = CMD_SUSPEND;

	return 0;
}

int playback_dai_resume(struct snd_soc_dai *dai)
{
	struct playback_priv *priv = snd_soc_dai_get_drvdata(dai);
	int id = dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct mtk_dma_reader_struct *pcm_playback = &dev_runtime->pcm_playback;
	struct snd_soc_component *component = dai->component;

	dev_runtime->private = priv;

	if (dev_runtime->runtime_status == CMD_SUSPEND) {
		pcm_playback->status = CMD_RESUME;
		dma_reader_set_sample_rate(dev_runtime, dev_runtime->sample_rate);
		dma_reader_set_channel_mode(priv, dev_runtime, dev_runtime->channel_mode, id);
		dma_reader_init(priv, dev_runtime, id);
		dma_reader_start(component, dev_runtime, id);
		dev_runtime->runtime_status = CMD_START;
	}

	return 0;
}

int i2s0_dai_suspend(struct snd_soc_dai *dai)
{
	return 0;
}

int i2s0_dai_resume(struct snd_soc_dai *dai)
{
	return 0;
}

int dac1_dai_suspend(struct snd_soc_dai *dai)
{
	return 0;
}

int dac1_dai_resume(struct snd_soc_dai *dai)
{
	return 0;
}

static struct snd_soc_dai_driver playback_dais[] = {
	/* FE DAIs */
	{
		.name = "SPEAKER",
		.id = SPEAKER_PLAYBACK,
		.playback = {
			.stream_name = "SPEAKER_PLAYBACK",
			.channels_min = CH_2,
			.channels_max = CH_12,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &dma_reader_fe_dai_ops,
		.suspend = playback_dai_suspend,
		.resume = playback_dai_resume,
	},
	{
		.name = "HEADPHONE",
		.id = HEADPHONE_PLAYBACK,
		.playback = {
			.stream_name = "HEADPHONE_PLAYBACK",
			.channels_min = CH_1,
			.channels_max = CH_2,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &dma_reader_fe_dai_ops,
		.suspend = playback_dai_suspend,
		.resume = playback_dai_resume,
	},
	{
		.name = "LINEOUT",
		.id = LINEOUT_PLAYBACK,
		.playback = {
			.stream_name = "LINEOUT_PLAYBACK",
			.channels_min = CH_1,
			.channels_max = CH_2,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &dma_reader_fe_dai_ops,
		.suspend = playback_dai_suspend,
		.resume = playback_dai_resume,
	},
	{
		.name = "HFP-TX",
		.id = HFP_PLAYBACK,
		.playback = {
			.stream_name = "HFP_PLAYBACK",
			.channels_min = CH_1,
			.channels_max = CH_2,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &dma_reader_fe_dai_ops,
		.suspend = playback_dai_suspend,
		.resume = playback_dai_resume,
	},
	/* BE DAIs */
	{
		.name = "BE_I2S0",
		.id = I2S0_PLAYBACK,
		.playback = {
			.stream_name = "I2S0",
			.channels_min = CH_1,
			.channels_max = CH_2,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &i2s_be_dai_ops,
		.suspend = i2s0_dai_suspend,
		.resume = i2s0_dai_resume,
	},
	{
		.name = "BE_I2S5",
		.id = I2S5_PLAYBACK,
		.playback = {
			.stream_name = "I2S5",
			.channels_min = CH_1,
			.channels_max = CH_2,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &i2s_be_dai_ops,
		.suspend = i2s0_dai_suspend,
		.resume = i2s0_dai_resume,
	},
	{
		.name = "BE_DAC0",
		.id = DAC0_PLAYBACK,
		.playback = {
			.stream_name = "DAC0",
			.channels_min = CH_1,
			.channels_max = CH_2,
			.rates = SNDRV_PCM_RATE_48000,
		},
	},
	{
		.name = "BE_DAC1",
		.id = DAC1_PLAYBACK,
		.playback = {
			.stream_name = "DAC1",
			.channels_min = CH_1,
			.channels_max = CH_2,
			.rates = SNDRV_PCM_RATE_48000,
		},
		.suspend = dac1_dai_suspend,
		.resume = dac1_dai_resume,
	},
};

static snd_pcm_uframes_t playback_pcm_pointer
				(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
				snd_soc_rtdcom_lookup(rtd, PLAYBACK_DMA_NAME);
	struct playback_priv *priv = snd_soc_component_get_drvdata(component);
	int id = rtd->cpu_dai->id;
	struct mtk_pb_runtime_struct *dev_runtime = priv->playback_dev[id];
	struct snd_pcm_runtime *runtime = substream->runtime;
	int consumed_bytes = 0;
	snd_pcm_uframes_t consumed_pcm_samples = 0;
	snd_pcm_uframes_t new_hw_ptr = 0;
	snd_pcm_uframes_t new_hw_ptr_pos = 0;

	if (dev_runtime->runtime_status == CMD_START) {
		consumed_bytes = dma_reader_get_consumed_bytes(dev_runtime, id);
		consumed_pcm_samples = bytes_to_frames(runtime, consumed_bytes);
	}

	new_hw_ptr = runtime->status->hw_ptr + consumed_pcm_samples;
	new_hw_ptr_pos = new_hw_ptr % runtime->buffer_size;

	return new_hw_ptr_pos;
}

static int playback_pcm_copy(struct snd_pcm_substream *substream, int channel,
		unsigned long pos, void __user *buf, unsigned long bytes)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
				snd_soc_rtdcom_lookup(rtd, PLAYBACK_DMA_NAME);
	struct playback_priv *priv = snd_soc_component_get_drvdata(component);
	int id = rtd->cpu_dai->id;
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
	dev_runtime->id = id;

	if (buf == NULL) {
		pr_err("[ALSA PB]Err! invalid memory address!\n");
		return -EINVAL;
	}

	if (bytes == 0) {
		pr_err("[ALSA PB]Err! request bytes is zero!\n");
		return -EINVAL;
	}

	/* Check if charging is complete */
	chg_finished(dev_runtime);

	warning_counter = 0;

	buffer_tmp = dev_runtime->buffer.addr;
	if (buffer_tmp == NULL) {
		pr_err("[ALSA PB]Err! need to alloc system mem for PCM buf\n");
		return -ENXIO;
	}

	/* Wake up Monitor if necessary */
	if (dev_runtime->monitor.monitor_status == CMD_STOP) {
	//pr_info("Wake up Playback Monitor %d\n", substream->pcm->device);
		while (1) {
			if (dma_reader_timer_reset(&dev_runtime->timer) == 0) {
				spin_lock_irqsave(&dev_runtime->spin_lock, flags);
				dev_runtime->monitor.monitor_status = CMD_START;
				spin_unlock_irqrestore(&dev_runtime->spin_lock, flags);
				break;
			}

			if ((++retry_counter) > 50) {
				pr_err("[ALSA PB]fail to wakeup Monitor %d\n",
						substream->pcm->device);
				break;
			}

			mtk_alsa_delaytask(1);
		}
	}

	request_size = bytes;

	dma_reader_get_avail_bytes(dev_runtime, id);

	if ((runtime->format == SNDRV_PCM_FORMAT_S32_LE) ||
		(runtime->format == SNDRV_PCM_FORMAT_S32_BE))
		period_size = pcm_playback->period_size << 1;

	if (((substream->f_flags & O_NONBLOCK) > 0) &&
		(request_size > dev_runtime->buffer.avail_size)) {
		pr_err("[ALSA PB]Err! avail %u but request %u bytes\n",
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
		if (receive_size) {
			pr_err("[ALSA PB]Err! %lu return byte\n", receive_size);
			size_to_copy -= receive_size;
		}

		while (1) {
			size = dma_reader_write(dev_runtime,
					(void *)buffer_tmp, size_to_copy);
			if (size == 0) {
				if ((++retry_counter) > 500) {
					pr_info("[ALSA PB]Retry write PCM\n");
					retry_counter = 0;
				}
				mtk_alsa_delaytask(1);
			} else
				break;
		}

		if ((runtime->format == SNDRV_PCM_FORMAT_S32_LE) ||
			(runtime->format == SNDRV_PCM_FORMAT_S32_BE))
			size <<= 1;

		copied_size += size;
	} while (copied_size < request_size);

	return 0;
}

const struct snd_pcm_ops playback_pcm_ops = {
	.ioctl = snd_pcm_lib_ioctl,
	.pointer = playback_pcm_pointer,
	.copy_user = playback_pcm_copy,
};

static int dma_inject_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
			snd_soc_kcontrol_component(kcontrol);
	struct playback_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int val = ucontrol->value.integer.value[0];
	unsigned int xdata = kcontrol->private_value;

	if (val < 0)
		return -EINVAL;

	dev_info(priv->dev, "[ALSA PB]Inject spk with Sinetone %d\n", val);

	priv->playback_dev[xdata]->inject.status = val;
	priv->playback_dev[xdata]->inject.r_ptr = 0;

	return 0;
}

static int dma_inject_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
			snd_soc_kcontrol_component(kcontrol);
	struct playback_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int xdata = kcontrol->private_value;

	dev_info(priv->dev, "[ALSA PB]get spk inject status\n");

	ucontrol->value.integer.value[0] =
			priv->playback_dev[xdata]->inject.status;

	return 0;
}

static const struct snd_kcontrol_new playback_controls[] = {
	SOC_SINGLE_BOOL_EXT("Inject Speaker Sinetone", 0,
			dma_inject_get, dma_inject_put),
};

//[Sysfs] HELP command
static ssize_t help_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, PLAYBACK_NUM, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, PLAYBACK_NUM, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 250, "Debug Help:\n"
		       "- spk_log_level / hp_log_level / lineout_log_level / HFP_log_level\n"
		       "                <W>: Set debug log level.\n"
		       "                (0:emerg, 1:alert, 2:critical, 3:error, 4:warn, 5:notice, 6:info, 7:debug)\n"
		       "                <R>: Read debug log level.\n"
		       "- spk_sinetone  <W>: write(1)/Clear(0) speaker sintone inject status.\n"
		       "                <R>: Read speaker sintone inject status.\n");
}

//[Sysfs] Speaker Log level
static ssize_t spk_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, SPEAKER_PLAYBACK, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, SPEAKER_PLAYBACK, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[SPEAKER_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t spk_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, SPEAKER_PLAYBACK, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, SPEAKER_PLAYBACK, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, 32, "spk_log_level:%d\n",
				  priv->playback_dev[SPEAKER_PLAYBACK]->log_level);
}

//[Sysfs] Speaker Sine tone
static ssize_t spk_sinetone_store(struct device *dev,
				  struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int enablesinetone = 0;
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, PLAYBACK_NUM, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, PLAYBACK_NUM, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &enablesinetone);
	if (retval == 0) {
		if ((enablesinetone != 0) && (enablesinetone != 1))
			dev_err(dev, "You should enter 0 or 1 !!!\n");
	}

	priv->playback_dev[SPEAKER_PLAYBACK]->inject.status = enablesinetone;
	priv->playback_dev[SPEAKER_PLAYBACK]->inject.r_ptr = 0;

	return (retval < 0) ? retval : count;
}

static ssize_t spk_sinetone_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, PLAYBACK_NUM, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, PLAYBACK_NUM, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, 32, "spk_sinetone status:%d\n",
				  priv->playback_dev[SPEAKER_PLAYBACK]->inject.status);
}

//[Sysfs] Headphone Log level
static ssize_t hp_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, HEADPHONE_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, HEADPHONE_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[HEADPHONE_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t hp_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, HEADPHONE_PLAYBACK, AUD_ERR, priv,
		"[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, HEADPHONE_PLAYBACK, AUD_ERR, priv,
		"[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	return snprintf(buf, 32, "hp_log_level:%d\n",
				  priv->playback_dev[HEADPHONE_PLAYBACK]->log_level);
}

//[Sysfs] Lineout Log level
static ssize_t lineout_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, LINEOUT_PLAYBACK, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, LINEOUT_PLAYBACK, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[LINEOUT_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t lineout_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, LINEOUT_PLAYBACK, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, LINEOUT_PLAYBACK, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, 32, "lineout_log_level:%d\n",
				  priv->playback_dev[LINEOUT_PLAYBACK]->log_level);
}

//[Sysfs] HFP Log level
static ssize_t hfp_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, HFP_PLAYBACK, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, HFP_PLAYBACK, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->playback_dev[HFP_PLAYBACK]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t hfp_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv;

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
		AUD_DEV_PRINT_PB(dev, HFP_PLAYBACK, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_PB(dev, HFP_PLAYBACK, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "hfp_log_level:%d\n",
				  priv->playback_dev[HFP_PLAYBACK]->log_level);
}

static struct snd_soc_dai *playback_find_dai(struct snd_soc_component *component,
					const char *name)
{
	struct snd_soc_dai *dai;

	list_for_each_entry(dai, &component->dai_list, list) {
		if (strncmp(dai->name, name, strlen(dai->name)) == 0)
			return dai;
	}

	return NULL;
}

static ssize_t spk_suspend_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;

	dai = playback_find_dai(component, "SPEAKER");
	if (dai)
		ret = playback_dai_suspend(dai);
	else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t spk_resume_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;

	dai = playback_find_dai(component, "SPEAKER");
	if (dai)
		ret = playback_dai_resume(dai);
	else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t spk_status_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;

	dai = playback_find_dai(component, "SPEAKER");
	if (dai) {
		dma_register_get_reg(dai->id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);
		ret = (mtk_alsa_read_reg_byte(REG_CTRL) & 0x3);
	} else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t hp_suspend_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;

	dai = playback_find_dai(component, "HEADPHONE");
	if (dai)
		ret = playback_dai_suspend(dai);
	else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t hp_resume_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;

	dai = playback_find_dai(component, "HEADPHONE");
	if (dai)
		ret = playback_dai_resume(dai);
	else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t hp_status_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;

	dai = playback_find_dai(component, "HEADPHONE");
	if (dai) {
		dma_register_get_reg(dai->id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);
		ret = (mtk_alsa_read_reg_byte(REG_CTRL) & 0x3);
	} else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t lineout_suspend_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;

	dai = playback_find_dai(component, "LINEOUT");
	if (dai)
		ret = playback_dai_suspend(dai);
	else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t lineout_resume_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;

	dai = playback_find_dai(component, "LINEOUT");
	if (dai)
		ret = playback_dai_resume(dai);
	else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t lineout_status_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;

	dai = playback_find_dai(component, "LINEOUT");
	if (dai) {
		dma_register_get_reg(dai->id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);
		ret = (mtk_alsa_read_reg_byte(REG_CTRL) & 0x3);
	} else
		ret = -1;

	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static ssize_t hfp_suspend_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;

	dai = playback_find_dai(component, "HFP-TX");
	if (dai)
		ret = playback_dai_suspend(dai);
	else
		ret = -1;

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t hfp_resume_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;

	dai = playback_find_dai(component, "HFP-TX");
	if (dai)
		ret = playback_dai_resume(dai);
	else
		ret = -1;

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t hfp_status_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct playback_priv *priv = dev_get_drvdata(dev);
	struct snd_soc_component *component = priv->component;
	struct snd_soc_dai *dai = NULL;
	int ret = 0;
	u32 REG_CTRL, REG_WPTR, REG_DDR_LEVEL;

	dai = playback_find_dai(component, "HFP-TX");
	if (dai) {
		dma_register_get_reg(dai->id, &REG_CTRL, &REG_WPTR, &REG_DDR_LEVEL);
		ret = (mtk_alsa_read_reg_byte(REG_CTRL) & (AU_BIT0 | AU_BIT1));
	} else
		ret = -1;

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RW(spk_log_level);
static DEVICE_ATTR_RW(spk_sinetone);
static DEVICE_ATTR_RW(hp_log_level);
static DEVICE_ATTR_RW(lineout_log_level);
static DEVICE_ATTR_RW(hfp_log_level);
static DEVICE_ATTR_RO(spk_suspend);
static DEVICE_ATTR_RO(spk_resume);
static DEVICE_ATTR_RO(spk_status);
static DEVICE_ATTR_RO(hp_suspend);
static DEVICE_ATTR_RO(hp_resume);
static DEVICE_ATTR_RO(hp_status);
static DEVICE_ATTR_RO(lineout_suspend);
static DEVICE_ATTR_RO(lineout_resume);
static DEVICE_ATTR_RO(lineout_status);
static DEVICE_ATTR_RO(hfp_suspend);
static DEVICE_ATTR_RO(hfp_resume);
static DEVICE_ATTR_RO(hfp_status);

static struct attribute *playback_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_spk_log_level.attr,
	&dev_attr_hp_log_level.attr,
	&dev_attr_lineout_log_level.attr,
	&dev_attr_hfp_log_level.attr,
	&dev_attr_spk_sinetone.attr,
	NULL,
};

static struct attribute *test_attrs[] = {
	&dev_attr_spk_suspend.attr,
	&dev_attr_spk_resume.attr,
	&dev_attr_spk_status.attr,
	&dev_attr_hp_suspend.attr,
	&dev_attr_hp_resume.attr,
	&dev_attr_hp_status.attr,
	&dev_attr_lineout_suspend.attr,
	&dev_attr_lineout_resume.attr,
	&dev_attr_lineout_status.attr,
	&dev_attr_hfp_suspend.attr,
	&dev_attr_hfp_resume.attr,
	&dev_attr_hfp_status.attr,
	NULL,
};

static const struct attribute_group playback_attr_group = {
	.name = "mtk_dbg",
	.attrs = playback_attrs,
};

static const struct attribute_group test_attr_group = {
	.name = "mtk_test",
	.attrs = test_attrs,
};

static const struct attribute_group *playback_attr_groups[] = {
	&playback_attr_group,
	&test_attr_group,
	NULL,
};

static int playback_probe(struct snd_soc_component *component)
{
	struct playback_priv *priv;
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
	priv->playback_dev[SPEAKER_PLAYBACK]->log_level = AUD_EMERG;
	priv->playback_dev[HEADPHONE_PLAYBACK]->log_level = AUD_EMERG;
	priv->playback_dev[LINEOUT_PLAYBACK]->log_level = AUD_EMERG;
	priv->playback_dev[HFP_PLAYBACK]->log_level = AUD_EMERG;

	/* Create device attribute files */
	ret = sysfs_create_groups(&dev->kobj, playback_attr_groups);
	if (ret)
		AUD_DEV_PRINT_PB(dev, PLAYBACK_NUM, AUD_ERR, priv,
		"Audio playback Sysfs init fail\n");

	priv->component = component;

	return 0;

}

static void playback_remove(struct snd_soc_component *component)
{
	struct playback_priv *priv;
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
		sysfs_remove_groups(&dev->kobj, playback_attr_groups);
}

static const struct snd_soc_component_driver playback_dai_component = {
	.name			= PLAYBACK_DMA_NAME,
	.probe			= playback_probe,
	.remove			= playback_remove,
	.ops			= &playback_pcm_ops,
	.controls		= playback_controls,
	.num_controls		= ARRAY_SIZE(playback_controls),
};

unsigned int playback_get_dts_u32(struct device_node *np, const char *name)
{
	unsigned int value;
	int ret;

	ret = of_property_read_u32(np, name, &value);
	if (ret)
		WARN_ON("[ALSA PB]Can't get DTS property\n");

	return value;
}

static int playback_parse_capability(struct device_node *node,
				struct playback_priv *priv)
{
	struct device_node *np;
	const char *hfp_tx_status = NULL;

	np = of_find_node_by_name(NULL, "mtk-sound-capability");
	if (!np)
		return -ENODEV;

	priv->hfp_enable_flag = 1;

	if (!of_property_read_bool(np, "hfp"))
		priv->hfp_enable_flag = 0;
	else {
		of_property_read_string(np, "hfp", &hfp_tx_status);

		if (!strncmp(hfp_tx_status, "disable", strlen("disable")))
			priv->hfp_enable_flag = 0;
	}

	return 0;
}

static int playback_parse_registers(void)
{
	struct device_node *np;

	memset(&REG, 0x0, sizeof(struct playback_register));

	np = of_find_node_by_name(NULL, "playback-register");
	if (!np)
		return -ENODEV;

	REG.PB_DMA1_CTRL = playback_get_dts_u32(np, "reg_pb_dma1_ctrl");
	REG.PB_DMA1_WPTR = playback_get_dts_u32(np, "reg_pb_dma1_wptr");
	REG.PB_DMA1_DDR_LEVEL = playback_get_dts_u32(np, "reg_pb_dma1_ddr_level");
	REG.PB_DMA2_CTRL = playback_get_dts_u32(np, "reg_pb_dma2_ctrl");
	REG.PB_DMA2_WPTR = playback_get_dts_u32(np, "reg_pb_dma2_wptr");
	REG.PB_DMA2_DDR_LEVEL = playback_get_dts_u32(np, "reg_pb_dma2_ddr_level");
	REG.PB_DMA3_CTRL = playback_get_dts_u32(np, "reg_pb_dma3_ctrl");
	REG.PB_DMA3_WPTR = playback_get_dts_u32(np, "reg_pb_dma3_wptr");
	REG.PB_DMA3_DDR_LEVEL = playback_get_dts_u32(np, "reg_pb_dma3_ddr_level");
	REG.PB_DMA4_CTRL = playback_get_dts_u32(np, "reg_pb_dma4_ctrl");
	REG.PB_DMA4_WPTR = playback_get_dts_u32(np, "reg_pb_dma4_wptr");
	REG.PB_DMA4_DDR_LEVEL = playback_get_dts_u32(np, "reg_pb_dma4_ddr_level");
	REG.PB_CHG_FINISHED = playback_get_dts_u32(np, "reg_pb_chg_finished");
	REG.PB_HP_OPENED = playback_get_dts_u32(np, "reg_pb_hp_opened");
	REG.PB_I2S_BCK_WIDTH = playback_get_dts_u32(np, "reg_pb_i2s_bck_width");

	return 0;
}

static int playback_parse_mmap(struct device_node *np,
				struct playback_priv *priv)
{
	struct of_mmap_info_data of_mmap_info = {};
	int ret;

	ret = of_mtk_get_reserved_memory_info_by_idx(np,
						0, &of_mmap_info);
	if (ret) {
		pr_err("[ALSA PB]get audio mmap fail\n");
		return -EINVAL;
	}

	priv->mem_info.miu = of_mmap_info.miu;
	priv->mem_info.bus_addr = of_mmap_info.start_bus_address;
	priv->mem_info.buffer_size = of_mmap_info.buffer_size;

	return 0;
}

static void playback_hfp_active_check(struct snd_soc_dai_driver *dai_driver,
	struct playback_priv *priv)
{
	struct snd_soc_pcm_stream *stream = &dai_driver[SPEAKER_PLAYBACK].playback;

	if (priv->hfp_enable_flag) {
		stream->channels_max = CH_8;

		AUD_PR_PRINT_PB(SPEAKER_PLAYBACK, AUD_INFO, priv,
			"[ALSA PB]spk channel max is set to 8 channels, since HFP is enable\n");
	}
}

int playback_parse_memory(struct device_node *np, struct playback_priv *priv)
{
	int ret;

	ret = of_property_read_u32(np, "speaker_dma_offset",
					&priv->spk_dma_offset);
	if (ret) {
		pr_err("[ALSA PB]can't get speaker dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "speaker_dma_size", &priv->spk_dma_size);
	if (ret) {
		pr_err("[ALSA PB]can't get speaker dma size\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "headphone_dma_offset",
					&priv->hp_dma_offset);
	if (ret) {
		pr_err("[ALSA PB]can't get headphone dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "headphone_dma_size",
					&priv->hp_dma_size);
	if (ret) {
		pr_err("[ALSA PB]can't get headphone dma size\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "lineout_dma_offset",
					&priv->lineout_dma_offset);
	if (ret) {
		pr_err("[ALSA PB]can't get lineout dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "lineout_dma_size",
					&priv->lineout_dma_size);
	if (ret) {
		pr_err("[ALSA PB]can't get lineout dma size\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "bt_dma_offset",
					&priv->bt_dma_offset);
	if (ret) {
		pr_err("[ALSA PB]can't get hfp dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "bt_dma_size",
					&priv->bt_dma_size);
	if (ret) {
		pr_err("[ALSA PB]can't get hfp dma size\n");
		return -EINVAL;
	}

	return 0;
}

int playback_dev_probe(struct platform_device *pdev)
{
	struct playback_priv *priv;
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

	for (i = 0; i < PLAYBACK_NUM; i++) {
		priv->playback_dev[i] = devm_kzalloc(&pdev->dev,
				sizeof(struct mtk_pb_runtime_struct), GFP_KERNEL);
		if (!priv->playback_dev[i])
			return -ENOMEM;
	}

	platform_set_drvdata(pdev, priv);

	ret = devm_snd_soc_register_component(&pdev->dev,
					&playback_dai_component,
					playback_dais,
					ARRAY_SIZE(playback_dais));
	if (ret) {
		dev_err(dev, "[ALSA PB]soc_register_component fail %d\n", ret);
		return ret;
	}
	/* parse dts */
	ret = of_property_read_u64(node, "memory_bus_base",
					&priv->mem_info.memory_bus_base);
	if (ret) {
		dev_err(dev, "[ALSA PB]can't get miu bus base\n");
		return -EINVAL;
	}
	/* parse registers */
	ret = playback_parse_registers();
	if (ret) {
		dev_err(dev, "[ALSA PB]parse register fail %d\n", ret);
		return ret;
	}
	/* parse dma memory info */
	ret = playback_parse_memory(node, priv);
	if (ret) {
		dev_err(dev, "[ALSA PB]parse memory fail %d\n", ret);
		return ret;
	}
	/* parse mmap */
	ret = playback_parse_mmap(node, priv);
	if (ret) {
		dev_err(dev, "[ALSA PB]parse mmap fail %d\n", ret);
		return ret;
	}
	/* parse capability */
	ret = playback_parse_capability(node, priv);
	if (ret) {
		dev_err(dev, "[ALSA PB]parse capability fail %d\n", ret);
		return ret;
	}
	/* Check SPK channel max setting*/
	playback_hfp_active_check(playback_dais, priv);
	return 0;
}

int playback_dev_remove(struct platform_device *pdev)
{
	return 0;
}

MODULE_DESCRIPTION("Mediatek ALSA SoC Playback DMA platform driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:snd-evb");
