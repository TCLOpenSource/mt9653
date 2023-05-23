// SPDX-License-Identifier: GPL-2.0
/*
 * mtk_alsa_duplex_platform.c  --  Mediatek DTV Duplex function
 *
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/io.h>
#include <sound/soc.h>

#include "mtk_alsa_chip.h"
#include "mtk_alsa_pcm_platform.h"
#include "mtk-reserved-memory.h"

#define DUPLEX_NAME		"snd-duplex-pcm-dai"

#define MAX_DUPLEX_BUF_SIZE	(12 * 4 * 2048 * 4)
#define SRC_PB_RATE_MAX		192000
#define SRC_PB_RATE_MIN		8000
#define SRC_CH_NUM_MAX	12
#define SRC_CH_NUM_MIN	2
#define SRC_PERIOD_SIZE_MAX	(MAX_DUPLEX_BUF_SIZE >> 2)
#define SRC_PERIOD_SIZE_MIN	64
#define SRC_PERIOD_COUNT_MAX	8
#define SRC_PERIOD_COUNT_MIN	2

#define EXPIRATION_TIME	500

#define ALSA_AUDIO_ENTER(dev, id, priv)	\
({do {AUD_DEV_PRINT_SRC(dev, id, AUD_INFO, priv	\
, "[%s:%d] !!!\n", __func__, __LINE__); } while (0); })

static struct duplex_register REG;

static unsigned int duplex_play_rates[] = {
	8000,
	11025,
	12000,
	16000,
	22050,
	24000,
	32000,
	44100,
	48000,
	64000,
	88200,
	96000,
	176400,
	192000,
};

static unsigned int duplex_cap_rates[] = {
	48000,
};

static unsigned int duplex_play_channels[] = {
	2, 8, 12
};

static unsigned int duplex_cap_channels[] = {
	2, 8, 12
};

static const struct snd_pcm_hw_constraint_list duplex_cap_rates_list = {
	.count = ARRAY_SIZE(duplex_cap_rates),
	.list = duplex_cap_rates,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list duplex_cap_channels_list = {
	.count = ARRAY_SIZE(duplex_cap_channels),
	.list = duplex_cap_channels,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list duplex_play_rates_list = {
	.count = ARRAY_SIZE(duplex_play_rates),
	.list = duplex_play_rates,
	.mask = 0,
};

static const struct snd_pcm_hw_constraint_list duplex_play_channels_list = {
	.count = ARRAY_SIZE(duplex_play_channels),
	.list = duplex_play_channels,
	.mask = 0,
};

/* Default hardware settings of ALSA PCM playback */
static struct snd_pcm_hardware mtk_pcm_default_hw = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE |
				SNDRV_PCM_FMTBIT_S32_BE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates = SNDRV_PCM_RATE_8000_192000,
	.rate_min = SRC_PB_RATE_MIN,
	.rate_max = SRC_PB_RATE_MAX,
	.channels_min = SRC_CH_NUM_MIN,
	.channels_max = SRC_CH_NUM_MAX,
	.buffer_bytes_max = MAX_DUPLEX_BUF_SIZE,
	.period_bytes_min = SRC_PERIOD_SIZE_MIN,
	.period_bytes_max = SRC_PERIOD_SIZE_MAX,
	.periods_min = SRC_PERIOD_COUNT_MIN,
	.periods_max = SRC_PERIOD_COUNT_MAX,
	.fifo_size = 0,
};

/* Default hardware settings of ALSA PCM playback */
static struct snd_pcm_hardware src2_pb_dma_playback_default_hw = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates = SNDRV_PCM_RATE_8000_192000,
	.rate_min = SRC_PB_RATE_MIN,
	.rate_max = SRC_PB_RATE_MAX,
	.channels_min = SRC_CH_NUM_MIN,
	.channels_max = SRC_CH_NUM_MAX,
	.buffer_bytes_max = MAX_DUPLEX_BUF_SIZE,
	.period_bytes_min = SRC_PERIOD_SIZE_MIN,
	.period_bytes_max = SRC_PERIOD_SIZE_MAX,
	.periods_min = SRC_PERIOD_COUNT_MIN,
	.periods_max = SRC_PERIOD_COUNT_MAX,
	.fifo_size = 0,
};

//[Sysfs] HELP command
static ssize_t help_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct duplex_priv *priv;  //need to modify

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
		AUD_DEV_PRINT_SRC(dev, DUPLEX_DEV_NUM, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_SRC(dev, DUPLEX_DEV_NUM, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 350, "Debug Help:\n"
		"- src1_log_level\n"
		"- src2_log_level\n"
		"- src3_log_level\n"
		"	<W>: Set debug log level.\n"
		"	(0:emerg, 1:alert, 2:critical, 3:error, 4:warn, 5:notice, 6:info, 7:debug)\n"
		"	<R>: Read debug log level.\n");
}


static ssize_t src1_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct duplex_priv *priv;

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
		AUD_DEV_PRINT_SRC(dev, SRC1_DEV, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_SRC(dev, SRC1_DEV, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->pcm_device[SRC1_DEV]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t src1_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct duplex_priv *priv;

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
		AUD_DEV_PRINT_SRC(dev, SRC1_DEV, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_SRC(dev, SRC1_DEV, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 25, "src_log_level:%d\n",
				  priv->pcm_device[SRC1_DEV]->log_level);
}

static ssize_t src2_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct duplex_priv *priv;

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
		AUD_DEV_PRINT_SRC(dev, SRC2_DEV, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_SRC(dev, SRC2_DEV, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->pcm_device[SRC2_DEV]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t src2_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct duplex_priv *priv;

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
		AUD_DEV_PRINT_SRC(dev, SRC2_DEV, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_SRC(dev, SRC2_DEV, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 25, "src_log_level:%d\n",
				  priv->pcm_device[SRC2_DEV]->log_level);
}


static ssize_t src3_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct duplex_priv *priv;

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
		AUD_DEV_PRINT_SRC(dev, SRC3_DEV, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_SRC(dev, SRC3_DEV, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->pcm_device[SRC3_DEV]->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t src3_log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct duplex_priv *priv;

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
		AUD_DEV_PRINT_SRC(dev, SRC3_DEV, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_SRC(dev, SRC3_DEV, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 25, "src_log_level:%d\n",
				  priv->pcm_device[SRC3_DEV]->log_level);
}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RW(src1_log_level);
static DEVICE_ATTR_RW(src2_log_level);
static DEVICE_ATTR_RW(src3_log_level);

static struct attribute *src3_attrs[] = {
	&dev_attr_src3_log_level.attr,
	NULL,
};

static struct attribute *src_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_src1_log_level.attr,
	&dev_attr_src2_log_level.attr,
	NULL,
};

static const struct attribute_group src3_attr_group = {
	.name = "mtk_dbg",
	.attrs = src3_attrs,
};

static const struct attribute_group src_attr_group = {
	.name = "mtk_dbg",
	.attrs = src_attrs,
};

static const struct attribute_group *src_attr_groups[] = {
	&src_attr_group,
	NULL,
};

static int duplex_probe(struct snd_soc_component *component)
{
	struct duplex_priv *priv;
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

	/*Set default log level*/
	priv->pcm_device[SRC1_DEV]->log_level = AUD_EMERG;
	priv->pcm_device[SRC2_DEV]->log_level = AUD_EMERG;
	priv->pcm_device[SRC3_DEV]->log_level = AUD_EMERG;

	/* Create device attribute files */
	ret = sysfs_create_groups(&dev->kobj, src_attr_groups);
	if (ret)
		AUD_DEV_PRINT_SRC(dev, DUPLEX_DEV_NUM, AUD_ERR, priv,
		"Audio playback Sysfs init fail\n");

	for_each_card_links(component->card, dai_link) {
		if ((!strncmp(dai_link->cpus->dai_name, "SRC3", strlen("SRC3"))) &&
			(dai_link->ignore == 0)) {
			ret = sysfs_merge_group(&dev->kobj, &src3_attr_group);
			if (ret)
				pr_err("[AUDIO][ERROR][%s]sysfs_merge_groups fail!\n", __func__);
		}
	}

	return 0;

}

static void duplex_remove(struct snd_soc_component *component)
{
	struct duplex_priv *priv;
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
		sysfs_unmerge_group(&dev->kobj, &src3_attr_group);
		sysfs_remove_groups(&dev->kobj, src_attr_groups);
	}
}

static void dma_reader_timer_open(struct timer_list *timer,
					unsigned long data, void *func)
{
	if (timer == NULL) {
		pr_err("[ALSA DUP]Err! timer pointer is invalid.\n");
		return;
	} else if (func == NULL) {
		pr_err("[ALSA DUP]Err! timer callback function is invalid.\n");
		return;
	}

	timer_setup(timer, func, 0);

	timer->expires = 0;

}

static void dma_reader_timer_close(struct timer_list *timer)
{
	if (timer == NULL) {
		pr_err("[ALSA DUP]Err! timer pointer is invalid.\n");
		return;
	}

	del_timer_sync(timer);
	memset(timer, 0x00, sizeof(struct timer_list));
}

static int dma_reader_timer_reset(struct timer_list *timer)
{
	if (timer == NULL) {
		pr_err("[ALSA DUP]Err! timer pointer is invalid.\n");
		return -EINVAL;
	} else if (timer->function == NULL) {
		pr_err("[ALSA DUP]Err! timer callback function is invalid.\n");
		return -EINVAL;
	}

	mod_timer(timer, (jiffies + msecs_to_jiffies(20)));

	return 0;
}

static int dma_reader_timer_update(struct timer_list *timer,
				unsigned long time_interval)
{
	if (timer == NULL) {
		pr_err("[ALSA DUP]Err! timer pointer is invalid.\n");
		return -EINVAL;
	} else if (timer->function == NULL) {
		pr_err("[ALSA DUP]Err! timer callback function is invalid.\n");
		return -EINVAL;
	}

	mod_timer(timer, (jiffies + time_interval));

	return 0;
}

static void dma_reader_monitor(struct timer_list *t)
{
	struct mtk_runtime_struct *dev_runtime = NULL;
	struct mtk_monitor_struct *monitor = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_pcm_runtime *runtime = NULL;
	char time_interval = 1;
	/* Set expiration as 500ms */
	unsigned int expiration_counter = EXPIRATION_TIME / time_interval;
	struct duplex_priv *priv;
	int id;

	dev_runtime = from_timer(dev_runtime, t, timer);

	if (dev_runtime == NULL) {
		pr_err("[ALSA DUP]Err! invalid parameters!\n");
		return;
	}

	monitor = &dev_runtime->monitor;
	priv = (struct duplex_priv *) dev_runtime->private;
	id = dev_runtime->id;

	if (monitor->monitor_status == CMD_STOP) {
		AUD_PR_PRINT_SRC(id, AUD_INFO, priv,
			"[ALSA DUP]No action is required, exit PB Monitor\n");
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
					AUD_PR_PRINT_SRC(id, AUD_INFO, priv,
						"[ALSA DUP]Exit PB Monitor\n");
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
		AUD_PR_PRINT_SRC(id, AUD_INFO, priv,
			"[ALSA DUP]Err! fail to update timer for PB Monitor!\n");
		spin_lock(&dev_runtime->spin_lock);
		memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
		spin_unlock(&dev_runtime->spin_lock);
	}
}

#define RATE_8000	8000
#define RATE_11025	11025
#define RATE_12000	12000
#define RATE_16000	16000
#define RATE_22050	22050
#define RATE_24000	24000
#define RATE_32000	32000
#define RATE_44100	44100
#define RATE_48000	48000
#define RATE_64000	64000
#define RATE_88200	88200
#define RATE_96000	96000
#define RATE_176400	176400
#define RATE_192000	192000

#define SRC_SYNTH_RATE_8000	0x69780000
#define SRC_SYNTH_RATE_11025	0x4C87D634
#define SRC_SYNTH_RATE_12000	0x46500000
#define SRC_SYNTH_RATE_16000	0x34BC0000
#define SRC_SYNTH_RATE_22050	0x2643EB1A
#define SRC_SYNTH_RATE_24000	0x23280000
#define SRC_SYNTH_RATE_32000	0x1A5E0000
#define SRC_SYNTH_RATE_44100	0x1321F58D
#define SRC_SYNTH_RATE_48000	0x11940000
#define SRC_SYNTH_RATE_64000	0x0D2F0000
#define SRC_SYNTH_RATE_88200	0x0990FAC6
#define SRC_SYNTH_RATE_96000	0x08CA0000
#define SRC_SYNTH_RATE_176400	0x04C87D63
#define SRC_SYNTH_RATE_192000	0x04650000

u32 calc_synthsizerRate(u32 sampleRate)
{
	u32 ret_val = SRC_SYNTH_RATE_48000;           //48k as default

	switch (sampleRate) {
	case RATE_8000:
		ret_val = SRC_SYNTH_RATE_8000;
		break;
	case RATE_11025:
		ret_val = SRC_SYNTH_RATE_11025;
		break;
	case RATE_12000:
		ret_val = SRC_SYNTH_RATE_12000;
		break;
	case RATE_16000:
		ret_val = SRC_SYNTH_RATE_16000;
		break;
	case RATE_22050:
		ret_val = SRC_SYNTH_RATE_22050;
		break;
	case RATE_24000:
		ret_val = SRC_SYNTH_RATE_24000;
		break;
	case RATE_32000:
		ret_val = SRC_SYNTH_RATE_32000;
		break;
	case RATE_44100:
		ret_val = SRC_SYNTH_RATE_44100;
		break;
	case RATE_48000:
		ret_val = SRC_SYNTH_RATE_48000;
		break;
	case RATE_64000:
		ret_val = SRC_SYNTH_RATE_64000;
		break;
	case RATE_88200:
		ret_val = SRC_SYNTH_RATE_88200;
		break;
	case RATE_96000:
		ret_val = SRC_SYNTH_RATE_96000;
		break;
	case RATE_176400:
		ret_val = SRC_SYNTH_RATE_176400;
		break;
	case RATE_192000:
		ret_val = SRC_SYNTH_RATE_192000;
		break;
	default:
		pr_err("[ALSA DUP]Err! un-supported sample rate!!!\n");
		ret_val = SRC_SYNTH_RATE_48000;
		//ret_val = 216000000/sampleRate;
		//ret_val <<= 16;
		break;
	}

	return ret_val;
}

void r2_dma_reader_init(struct mtk_dma_struct *playback,
	struct snd_soc_dai *dai, int id)
{
	u32 u32_synthrate;
	u16 u16_synthrate_h;
	u16 u16_synthrate_l;
	unsigned short val;
	unsigned int smp_bytes = 0;
	ptrdiff_t p_addr = playback->str_mode_info.physical_addr / BYTES_IN_MIU_LINE;
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);

	AUD_DEV_PRINT_SRC(dai->dev, id, AUD_INFO, priv,
		"[ALSA DUP]input sample rate is %u\n", playback->sample_rate);
	AUD_DEV_PRINT_SRC(dai->dev, id, AUD_INFO, priv,
		"[ALSA DUP]channel mode is %u\n", playback->channel_mode);

	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x7FFF, 0x0000);

	u32_synthrate = calc_synthsizerRate(playback->sample_rate);
	u16_synthrate_h = (u32_synthrate >> 16);
	u16_synthrate_l = (u32_synthrate & 0xFFFF);

	mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_H, 0xFFFF, u16_synthrate_h);
	mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_L, 0xFFFF, u16_synthrate_l);
	mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0101, 0x0100);
	mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0001, 0x0001);
	mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0001, 0x0000);
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_03, 0xFFFF,
			(playback->r_buf.size / BYTES_IN_MIU_LINE));
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_05, 0xFFFF,
			(playback->high_threshold / BYTES_IN_MIU_LINE));
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_06, 0xFFFF,
			((playback->r_buf.size >> 3) / BYTES_IN_MIU_LINE));

	// TODO:
	if (playback->channel_mode == 12) {
		/* Sel_1ch (cfg0[7]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0080, 0x0000);
		/* Sel_3ch (cfg8[10]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0400, 0x0000);
		/* Sel_4ch (cfg8[11]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0800, 0x0000);
		/* Sel_6ch (cfg8[7]) on */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0080, 0x0080);
		/* Sel_8ch (cfg8[9]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0200, 0x0000);
		val = 0x11; /* MIU burst length = 0x11 */
		val += (1 << 7); /* MIU Step En = 1 */
		val += (1 << 15); /* Sync Valid En = 1 */
		val += (1 << 14); /* Sync Mode En = 0 */
		val += (0xB << 8); /* Sync Step Count = 0xB */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_07, 0xFFFF, val);
	} else if (playback->channel_mode == 8) {
		/* Sel_1ch (cfg0[7]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0080, 0x0000);
		/* Sel_3ch (cfg8[10]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0400, 0x0000);
		/* Sel_4ch (cfg8[11]) on */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0800, 0x0800);
		/* Sel_6ch (cfg8[7]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0080, 0x0000);
		/* Sel_8ch (cfg8[9]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0200, 0x0000);
		val = 0xB; /* MIU burst length = 0xB */
		val += (1 << 7); /* MIU Step En = 1 */
		val += (1 << 15); /* Sync Valid En = 1 */
		val += (1 << 14); /* Sync Mode En = 0 */
		val += (0xB << 8); /* Sync Step Count = 0xB */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_07, 0xFFFF, val);
	} else {
		/* Sel_1ch (cfg0[7]) on */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0080, 0x0080);
		/* Sel_3ch (cfg8[10]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0400, 0x0000);
		/* Sel_4ch (cfg8[11]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0800, 0x0000);
		/* Sel_6ch (cfg8[7]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0080, 0x0000);
		/* Sel_8ch (cfg8[9]) off */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_08, 0x0200, 0x0000);
		val = 2; /* MIU burst length = 2 */
		val += (0 << 7); /* MIU Step En = 0 */
		val += (0 << 15); /* Sync Valid En = 0 */
		// val += (1 << 8); /* Sync Step Count = 1 */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_07, 0xFFFF, val);
	}

	if (playback->format == SNDRV_PCM_FORMAT_S24_LE)
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0860, 0x0840);
	else if (playback->format == SNDRV_PCM_FORMAT_S32_LE)
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0860, 0x0840);
	else
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0860, 0x0020);

	/* set r2 dma reader to MCU control */
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x8000, 0x8000);
	mtk_alsa_write_reg_mask(REG.PB_DMA_ENABLE, 0x0003, 0x0003);

	/* reset & enable r2 dma reader engine */
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x2000, 0x2000);
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x2000, 0x0000);
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x1000, 0x1000);
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x1000, 0x0000);
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0002, 0x0002);
	/* set r2 dma reader memory address */
	/* BASE [15:0] */
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_01, 0xFFFF, (p_addr & 0xFFFF));
	/* BASE [26:16] */
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_02, 0xFFFF, (p_addr >> 16));

	// TODO:
	/* SRC channel sel mux for R2 DMA Reader1 */
	if (playback->channel_mode == 12) {
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M1_CFG, 0x9F, 0x80);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M2_CFG, 0x9F, 0x80);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M3_CFG, 0x9F, 0x80);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M4_CFG, 0x9F, 0x80);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M5_CFG, 0x9F, 0x80);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M6_CFG, 0x9F, 0x80);
	} else if (playback->channel_mode == 8) {
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M1_CFG, 0x9F, 0x80);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M2_CFG, 0x9F, 0x80);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M3_CFG, 0x9F, 0x80);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M4_CFG, 0x9F, 0x80);
	} else {
		// defaut 2ch
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M1_CFG, 0x9F, 0x80);
	}

	smp_bytes = (playback->sample_bits / BITS_IN_BYTE);

	playback->c_buf.u32RemainSize = 0;
	playback->u32TargetAlignmentSize =
		(playback->channel_mode * smp_bytes * BYTES_IN_MIU_LINE);
}

void r2_dma_reader2_init(struct mtk_dma_struct *playback,
	struct snd_soc_dai *dai, int id)
{
	u32 u32_synthrate;
	u16 u16_synthrate_h;
	u16 u16_synthrate_l;
	unsigned short val;
	unsigned int smp_bytes = 0;
	ptrdiff_t p_addr = playback->str_mode_info.physical_addr / BYTES_IN_MIU_LINE;
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);

	AUD_DEV_PRINT_SRC(dai->dev, id, AUD_INFO, priv,
		"[ALSA DUP]input sample rate is %u\n", playback->sample_rate);
	AUD_DEV_PRINT_SRC(dai->dev, id, AUD_INFO, priv,
		"[ALSA DUP]channel mode is %u\n", playback->channel_mode);

	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x7FFF, 0x0000);

	u32_synthrate = calc_synthsizerRate(playback->sample_rate);
	u16_synthrate_h = (u32_synthrate >> 16);
	u16_synthrate_l = (u32_synthrate & 0xFFFF);

	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_SYNTH_H, 0xFFFF, u16_synthrate_h);
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_SYNTH_L, 0xFFFF, u16_synthrate_l);
	mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0404, 0x0400);
	mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0004, 0x0004);
	mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0004, 0x0000);

	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_03, 0xFFFF,
			(playback->r_buf.size / BYTES_IN_MIU_LINE));
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_05, 0xFFFF,
			(playback->high_threshold / BYTES_IN_MIU_LINE));
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_06, 0xFFFF,
			((playback->r_buf.size >> 3) / BYTES_IN_MIU_LINE));

	// TODO:
	if (playback->channel_mode == 12) {
		/* Sel_1ch (cfg0[7]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0080, 0x0000);
		/* Sel_3ch (cfg8[10]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0400, 0x0000);
		/* Sel_4ch (cfg8[11]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0800, 0x0000);
		/* Sel_6ch (cfg8[7]) on */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0080, 0x0080);
		/* Sel_8ch (cfg8[9]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0200, 0x0000);
		val = 0x11; /* MIU burst length = 0x11 */
		val += (1 << 7); /* MIU Step En = 1 */
		val += (1 << 15); /* Sync Valid En = 1 */
		val += (1 << 14); /* Sync Mode En = 0 */
		val += (0xB << 8); /* Sync Step Count = 0xB */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_07, 0xFFFF, val);
	} else if (playback->channel_mode == 8) {
		/* Sel_1ch (cfg0[7]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0080, 0x0000);
		/* Sel_3ch (cfg8[10]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0400, 0x0000);
		/* Sel_4ch (cfg8[11]) on */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0800, 0x0800);
		/* Sel_6ch (cfg8[7]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0080, 0x0000);
		/* Sel_8ch (cfg8[9]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0200, 0x0000);
		val = 0xB; /* MIU burst length = 0xB */
		val += (1 << 7); /* MIU Step En = 1 */
		val += (1 << 15); /* Sync Valid En = 1 */
		val += (1 << 14); /* Sync Mode En = 0 */
		val += (0xB << 8); /* Sync Step Count = 0xB */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_07, 0xFFFF, val);
	} else {
		/* Sel_1ch (cfg0[7]) on */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0080, 0x0080);
		/* Sel_3ch (cfg8[10]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0400, 0x0000);
		/* Sel_4ch (cfg8[11]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0800, 0x0000);
		/* Sel_6ch (cfg8[7]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0080, 0x0000);
		/* Sel_8ch (cfg8[9]) off */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_08, 0x0200, 0x0000);
		val = 2; /* MIU burst length = 2 */
		val += (0 << 7); /* MIU Step En = 0 */
		val += (0 << 15); /* Sync Valid En = 0 */
		// val += (1 << 8); /* Sync Step Count = 1 */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_07, 0xFFFF, val);
	}

	if (playback->format == SNDRV_PCM_FORMAT_S24_LE)
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0860, 0x0840);
	else if (playback->format == SNDRV_PCM_FORMAT_S32_LE)
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0860, 0x0840);
	else
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0860, 0x0020);

	/* set r2 dma reader to MCU control */
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x8000, 0x8000);
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_ENABLE, 0x0003, 0x0003);

	/* reset & enable r2 dma reader engine */
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x2000, 0x2000);
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x2000, 0x0000);
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x1000, 0x1000);
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x1000, 0x0000);
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0002, 0x0002);
	/* set r2 dma reader memory address */
	/* BASE [15:0] */
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_01, 0xFFFF, (p_addr & 0xFFFF));
	/* BASE [26:16] */
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_02, 0xFFFF, (p_addr >> 16));

	// TODO:
	/* SRC channel sel mux for R2 DMA Reader2 */
	if (playback->channel_mode == 12) {
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M9_CFG, 0x9F, 0x82);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M10_CFG, 0x9F, 0x82);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M11_CFG, 0x9F, 0x82);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M12_CFG, 0x9F, 0x82);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M13_CFG, 0x9F, 0x82);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M14_CFG, 0x9F, 0x82);
	} else if (playback->channel_mode == 8) {
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M9_CFG, 0x9F, 0x82);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M10_CFG, 0x9F, 0x82);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M11_CFG, 0x9F, 0x82);
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M12_CFG, 0x9F, 0x82);
	} else {
		// defaut 2ch
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_M9_CFG, 0x9F, 0x82);
	}

	smp_bytes = (playback->sample_bits / BITS_IN_BYTE);

	playback->c_buf.u32RemainSize = 0;
	playback->u32TargetAlignmentSize =
		(playback->channel_mode * smp_bytes * BYTES_IN_MIU_LINE);
}

void dma_reader_init(struct mtk_dma_struct *playback,
	struct snd_soc_dai *dai, int id)
{
	u32 u32_synthrate;
	u16 u16_synthrate_h;
	u16 u16_synthrate_l;
	struct duplex_priv *priv;

	u32_synthrate = calc_synthsizerRate(playback->sample_rate);
	u16_synthrate_h = (u32_synthrate >> 16);
	u16_synthrate_l = (u32_synthrate & 0xFFFF);

	priv = snd_soc_dai_get_drvdata(dai);

	AUD_DEV_PRINT_SRC(dai->dev, id, AUD_INFO, priv,
		"[ALSA DUP]input sample rate is %u\n", playback->sample_rate);
	AUD_DEV_PRINT_SRC(dai->dev, id, AUD_INFO, priv,
		"[ALSA DUP]channel mode is %u\n", playback->channel_mode);

	// decoder5 bank control
	/* enable reg_dvb5_fix_synth_nf_h */
	mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_SYNTH_UPDATE, 0xA080, 0xA080);

	/* configure sample rate */
	mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_SYNTH_H, 0xFFFF, u16_synthrate_h);
	mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_SYNTH_L, 0xFFFF, u16_synthrate_l);

	/* trigger reg_dvb5_fix_synth_nf_h to apply configuration */
	mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_SYNTH_UPDATE, 0x1000, 0x1000);
	mtk_alsa_delaytask_us(50);
	mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_SYNTH_UPDATE, 0x1000, 0x0000);

	/* reset PCM playback engine */
	mtk_alsa_write_reg(REG.SRC2_PB_DMA_CTRL, 0x0001);

	/* clear PCM playback write pointer */
	mtk_alsa_write_reg(REG.SRC2_PB_DMA_WPTR, 0x0000);

	if (playback->format == SNDRV_PCM_FORMAT_S24_LE)
		mtk_alsa_write_reg_mask_byte(REG.SRC2_PB_DMA_CTRL, 0x30, 0x10);  // 24bit mode
	else
		mtk_alsa_write_reg_mask_byte(REG.SRC2_PB_DMA_CTRL, 0x30, 0x00);  // 16bit mode

	if (playback->channel_mode == 12) {
		/* DSP decoder5 dma output 12ch */
		mtk_alsa_write_reg_mask_byte(REG.SRC2_PB_DMA_CTRL, 0xC0, 0x40);

		/* CH7 sel to Decoder5(1,2) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_7_CFG, 0x9F, 0x8A);
		/* CH8 sel to Decoder5(3,4) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_8_CFG, 0x9F, 0x8A);
		/* CH5 sel to Decoder5(5,6) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_5_CFG, 0x9F, 0x8A);
		/* CH6 sel to Decoder5(7,8) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_6_CFG, 0x9F, 0x8A);
		/* CH9 sel to Decoder5(9,10) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_9_CFG, 0x9F, 0x8A);
		/* CH10 sel to Decoder5(11,12) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_10_CFG, 0x9F, 0x8A);
	} else if (playback->channel_mode == 8) {
		/* DSP decoder5 dma output 8ch */
		mtk_alsa_write_reg_mask_byte(REG.SRC2_PB_DMA_CTRL, 0xC0, 0x80);

		/* CH7 sel to Decoder5(1,2) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_7_CFG, 0x9F, 0x8A);
		/* CH8 sel to Decoder5(3,4) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_8_CFG, 0x9F, 0x8A);
		/* CH5 sel to Decoder5(5,6) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_5_CFG, 0x9F, 0x8A);
		/* CH6 sel to Decoder5(7,8) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_6_CFG, 0x9F, 0x8A);
		/* CH9 sel to Decoder5(9,10) DMA Reader */
	} else {
		/* DSP decoder5 dma output 2ch */
		mtk_alsa_write_reg_mask_byte(REG.SRC2_PB_DMA_CTRL, 0xC0, 0x00);

		/* CH7 sel to Decoder5(1,2) DMA Reader */
		mtk_alsa_write_reg_mask_byte(REG.CHANNEL_7_CFG, 0x9F, 0x8A);
	}

	playback->c_buf.u32RemainSize = 0;
}

void duplex_playback_init(struct mtk_dma_struct *playback,
	struct snd_soc_dai *dai, int id)
{
	switch (id) {
	case SRC1_DEV:
		r2_dma_reader_init(playback, dai, id);
		/* start PCM playback engine */
		mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0001, 0x0001);
		break;
	case SRC2_DEV:
		// decoder5 dma init //
		dma_reader_init(playback, dai, id);
		/* start PCM playback engine */
		mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_CTRL, 0x0002, 0x0002);
		/* wait DSP dma */
		mtk_alsa_delaytask(1);
		mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_CTRL, 0x0001, 0x0000);
		break;
	case SRC3_DEV:
		r2_dma_reader2_init(playback, dai, id);
		/* start PCM playback engine */
		mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0001, 0x0001);
		break;
	default:
		break;
	}

}

void duplex_capture_init(struct mtk_dma_struct *capture, int id)
{
	u32 REG_CTRL = 0;

	if (id == SRC1_DEV)
		REG_CTRL = REG.CAP_DMA_CTRL;
	else if (id == SRC2_DEV)
		REG_CTRL = REG.SRC2_CAP_DMA_CTRL;
	else
		REG_CTRL = REG.SRC3_CAP_DMA_CTRL;

	if (capture->channel_mode == 12)
		mtk_alsa_write_reg_mask_byte(REG_CTRL, CAP_CHANNEL_MASK, CAP_CHANNEL_12);
	else if (capture->channel_mode == 8)
		mtk_alsa_write_reg_mask_byte(REG_CTRL, CAP_CHANNEL_MASK, CAP_CHANNEL_8);
	else
		mtk_alsa_write_reg_mask_byte(REG_CTRL, CAP_CHANNEL_MASK, CAP_CHANNEL_2);

	if (capture->format == SNDRV_PCM_FORMAT_S24_LE)
		mtk_alsa_write_reg_mask_byte(REG_CTRL, CAP_FORMAT_MASK, CAP_FORMAT_24BIT);
	else
		mtk_alsa_write_reg_mask_byte(REG_CTRL, CAP_FORMAT_MASK, CAP_FORMAT_16BIT);

	capture->c_buf.u32RemainSize = 0;

	// start pcm capture
	mtk_alsa_write_reg_mask(REG_CTRL, 0x0001, 0x0001);
}

static int duplex_dma_init(struct duplex_priv *priv,
	struct mtk_dma_struct *pcm_stream, unsigned int offset, unsigned int size)
{
	unsigned int buffer_size = 0;
	ptrdiff_t dma_base_pa = 0;
	ptrdiff_t dma_base_ba = 0;
	ptrdiff_t dma_base_va = pcm_stream->str_mode_info.virtual_addr;

	buffer_size = size;
	pcm_stream->period_size = buffer_size >> 2;
	pcm_stream->r_buf.size = buffer_size;
	pcm_stream->high_threshold =
			pcm_stream->r_buf.size - (pcm_stream->r_buf.size >> 3);

	//pr_err("cap_init: size = %d\n", pcm_stream->r_buf.size);

	if ((pcm_stream->initialized_status != 1) ||
		(pcm_stream->status != CMD_RESUME)) {
		dma_base_pa = priv->mem_info.bus_addr -
				priv->mem_info.memory_bus_base + offset;
		dma_base_ba = priv->mem_info.bus_addr + offset;

		if (dma_base_ba % 0x1000) {
			pr_err("[ALSA DUP]Invalid bus addr,should align 4KB\n");
			return -EFAULT;
		}

		/* convert Bus Address to Virtual Address */
		if (dma_base_va == 0) {
			dma_base_va = (ptrdiff_t)ioremap_wc(dma_base_ba,
						pcm_stream->r_buf.size);
			if (dma_base_va == 0) {
				pr_err("[ALSA DUP]fail to convert Bus Addr\n");
				return -ENOMEM;
			}
		}
		pcm_stream->str_mode_info.physical_addr = dma_base_pa;
		pcm_stream->str_mode_info.bus_addr = dma_base_ba;
		pcm_stream->str_mode_info.virtual_addr = dma_base_va;

		pcm_stream->initialized_status = 1;
	} else {
		dma_base_pa = pcm_stream->str_mode_info.physical_addr;
		dma_base_ba = pcm_stream->str_mode_info.bus_addr;
		dma_base_va = pcm_stream->str_mode_info.virtual_addr;
	}

	pcm_stream->r_buf.addr = (unsigned char *)dma_base_va;
	pcm_stream->r_buf.r_ptr = pcm_stream->r_buf.addr;
	pcm_stream->r_buf.w_ptr = pcm_stream->r_buf.addr;
	pcm_stream->r_buf.buf_end = pcm_stream->r_buf.addr + buffer_size;
	pcm_stream->copy_size = 0;

	memset((void *)pcm_stream->r_buf.addr, 0x00, pcm_stream->r_buf.size);
	smp_mb();

	return 0;
}

static int duplex_dma_stop(struct mtk_runtime_struct *dev_runtime, int id, int direction)
{
	struct mtk_dma_struct *playback = &dev_runtime->playback;
	struct mtk_dma_struct *capture = &dev_runtime->capture;

	/* Reset some variables */
	dev_runtime->substreams.substream_status = CMD_STOP;

	if (direction == E_PLAYBACK) {
		switch (id) {
		case SRC1_DEV:
			mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0001, 0x0000);

			/* reset dma engine */
			mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0002, 0x0000);
			/* reset & enable r2 dma reader engine */
			mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x2000, 0x2000);
			mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x2000, 0x0000);
			mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x1000, 0x1000);
			mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x1000, 0x0000);
			mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0002, 0x0002);
			/* reset Read & Write Pointer */
			playback->r_buf.r_ptr = playback->r_buf.addr;
			playback->r_buf.w_ptr = playback->r_buf.addr;
			/* reset written size */
			playback->copy_size = 0;
			playback->status = CMD_STOP;

			mtk_alsa_write_reg_mask(REG.PB_DMA_ENABLE, 0x0001, 0x0000);
			break;
		case SRC2_DEV:
			/* reset PCM playback engine */
			mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_CTRL, 0x0003, 0x0001);

			/* clear PCM playback write pointer */
			mtk_alsa_write_reg(REG.SRC2_PB_DMA_WPTR, 0x0000);

			/* Disable dma */
			mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_CTRL, 0xFFFF, 0x0000);

			/* reset Read & Write Pointer */
			playback->r_buf.r_ptr = playback->r_buf.addr;
			playback->r_buf.w_ptr = playback->r_buf.addr;
			/* reset written size */
			playback->copy_size = 0;
			playback->status = CMD_STOP;
			break;
		case SRC3_DEV:
			mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0001, 0x0000);

			/* reset dma engine */
			mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0002, 0x0000);
			/* reset & enable r2 dma reader engine */
			mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x2000, 0x2000);
			mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x2000, 0x0000);
			mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x1000, 0x1000);
			mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x1000, 0x0000);
			mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0002, 0x0002);
			/* reset Read & Write Pointer */
			playback->r_buf.r_ptr = playback->r_buf.addr;
			playback->r_buf.w_ptr = playback->r_buf.addr;
			/* reset written size */
			playback->copy_size = 0;
			playback->status = CMD_STOP;

			mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_ENABLE, 0x0001, 0x0000);
			break;
		default:
			pr_err("[ALSA DUP] %s[%d] invalid id = %d\n", __func__, __LINE__, id);
			break;
		}

	} else if (direction == E_CAPTURE) {
		dev_runtime->substreams.c_substream = NULL;

		/* Stop DMA Writer  */
		switch (id) {
		case SRC1_DEV:
			mtk_alsa_write_reg_mask(REG.CAP_DMA_CTRL, 0x0001, 0x0000);
			mtk_alsa_write_reg(REG.CAP_DMA_RPTR, 0x0000);
			break;
		case SRC2_DEV:
			mtk_alsa_write_reg_mask(REG.SRC2_CAP_DMA_CTRL, 0x0001, 0x0000);
			mtk_alsa_write_reg(REG.SRC2_CAP_DMA_RPTR, 0x0000);
			break;
		case SRC3_DEV:
			mtk_alsa_write_reg_mask(REG.SRC3_CAP_DMA_CTRL, 0x0001, 0x0000);
			mtk_alsa_write_reg(REG.SRC3_CAP_DMA_RPTR, 0x0000);
			break;
		default:
			pr_err("[ALSA DUP] %s[%d] invalid id = %d\n", __func__, __LINE__, id);
			break;
		}

		/* reset Read & Write Pointer */
		capture->r_buf.r_ptr = capture->r_buf.addr;
		capture->r_buf.w_ptr = capture->r_buf.addr;
		/* reset written size */
		capture->copy_size = 0;
		capture->status = CMD_STOP;
	}
	return 0;
}

static int duplex_dma_exit(struct mtk_dma_struct *pcm_stream)
{
	ptrdiff_t dma_base_va = pcm_stream->str_mode_info.virtual_addr;

	if (pcm_stream->c_buf.addr != NULL) {
		vfree(pcm_stream->c_buf.addr);
		pcm_stream->c_buf.addr = NULL;
	}

	if (dma_base_va != 0) {
		if (pcm_stream->r_buf.addr) {
			iounmap((void *)pcm_stream->r_buf.addr);
			pcm_stream->r_buf.addr = 0;
			pcm_stream->r_buf.buf_end = 0;
		} else {
			pr_err("[ALSA DUP]Err! buffer addr should not be 0\n");
		}
		pcm_stream->str_mode_info.virtual_addr = 0;
	}
	pcm_stream->sample_rate = 0;
	pcm_stream->channel_mode = 0;
	pcm_stream->status = CMD_STOP;
	pcm_stream->initialized_status = 0;

	return 0;
}

static void dma_reader_get_avail_bytes
			(struct mtk_dma_struct *playback, int id)
{
	int inused_lines = 0;
	int avail_lines = 0;
	int avail_bytes = 0;

	if (id == SRC1_DEV)
		inused_lines = mtk_alsa_read_reg(REG.PB_DMA_STATUS_00);
	else if (id == SRC2_DEV)
		inused_lines = mtk_alsa_read_reg(REG.SRC2_PB_DMA_DDR_LEVEL);
	else
		inused_lines = mtk_alsa_read_reg(REG.SRC3_PB_DMA_STATUS_00);

	avail_lines = (playback->r_buf.size / BYTES_IN_MIU_LINE) -
					inused_lines;

	if (avail_lines < 0) {
		pr_err("[ALSA DUP]Incorrect inused lines %d!\n", inused_lines);
		avail_lines = 0;
	}

	avail_bytes = avail_lines * BYTES_IN_MIU_LINE;

	playback->c_buf.avail_size = avail_bytes;
}

static unsigned int dma_reader_get_consumed_bytes
		(struct mtk_dma_struct *playback, int id)
{
	unsigned int inused_bytes = 0;
	unsigned int consumed_bytes = 0;
	unsigned int dma_level = 0;

	if (id == SRC1_DEV)
		dma_level = mtk_alsa_read_reg(REG.PB_DMA_STATUS_00);
	else if (id == SRC2_DEV)
		dma_level = mtk_alsa_read_reg(REG.SRC2_PB_DMA_DDR_LEVEL);
	else
		dma_level = mtk_alsa_read_reg(REG.SRC3_PB_DMA_STATUS_00);


	inused_bytes = dma_level * BYTES_IN_MIU_LINE;
	if (playback->copy_size > inused_bytes) {
		consumed_bytes = playback->copy_size - inused_bytes;
		playback->copy_size = inused_bytes;
	} else {
		consumed_bytes = 0;
	}

	return consumed_bytes;
}

static void capture_get_avail_bytes
			(struct mtk_dma_struct *capture)
{
	int avail_bytes = 0;

	avail_bytes = capture->r_buf.w_ptr - capture->r_buf.r_ptr;

	if (avail_bytes < 0)
		avail_bytes += capture->r_buf.size;

	capture->c_buf.avail_size = avail_bytes;
}

static unsigned int r2_dma_reader_write(struct mtk_dma_struct *playback,
	struct mtk_runtime_struct *dev_runtime, void *buffer, unsigned int bytes)
{
	unsigned int copy_size = 0;
	unsigned int copy_tmp = 0;
	unsigned int inused_bytes = 0;
	struct duplex_priv *priv;
	int id;

	priv = (struct duplex_priv *) dev_runtime->private;
	id = dev_runtime->id;

	copy_size = bytes;
	inused_bytes = mtk_alsa_read_reg(REG.PB_DMA_STATUS_00) * BYTES_IN_MIU_LINE;

	if ((inused_bytes == 0) && (playback->status == CMD_START)) {
		AUD_PR_PRINT_SRC(id, AUD_INFO, priv,
			"[ALSA DUP]*****PCM Playback Buffer empty !!*****\n");
	} else if ((inused_bytes + copy_size) > playback->high_threshold) {
		//pr_err("*****PCM Playback Buffer full !!*****\n");
		return 0;
	}

	if ((inused_bytes + copy_size + playback->c_buf.u32RemainSize) <= playback->r_buf.size) {
		do {
			copy_tmp = playback->r_buf.buf_end - playback->r_buf.w_ptr;
			copy_tmp = (copy_tmp > copy_size) ? copy_size : copy_tmp;

			memcpy(playback->r_buf.w_ptr, buffer, copy_tmp);
			playback->r_buf.w_ptr += copy_tmp;
			buffer += copy_tmp;

			if (playback->r_buf.w_ptr == playback->r_buf.buf_end)
				playback->r_buf.w_ptr = playback->r_buf.addr;

			copy_size -= copy_tmp;
		} while (copy_size > 0);
	}

	/* flush MIU */
	smp_mb();

	copy_size = bytes;
	copy_size += playback->c_buf.u32RemainSize;
	playback->c_buf.u32RemainSize = copy_size % playback->u32TargetAlignmentSize;
	AUDIO_DO_ALIGNMENT(copy_size, playback->u32TargetAlignmentSize);

	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_04, 0xFFFF,
					(copy_size / BYTES_IN_MIU_LINE));
	/* BASE [26:16] */
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0400, 0x0400);
	mtk_alsa_write_reg_mask(REG.PB_DMA_CFG_00, 0x0400, 0x0000);

	playback->copy_size += copy_size;
	playback->status = CMD_START;

	mtk_alsa_delaytask(1);

	return bytes;
}


static unsigned int r2_dma_reader2_write(struct mtk_dma_struct *playback,
	struct mtk_runtime_struct *dev_runtime, void *buffer, unsigned int bytes)
{
	unsigned int copy_size = 0;
	unsigned int copy_tmp = 0;
	unsigned int inused_bytes = 0;
	struct duplex_priv *priv;
	int id;

	priv = (struct duplex_priv *) dev_runtime->private;
	id = dev_runtime->id;

	copy_size = bytes;
	inused_bytes = mtk_alsa_read_reg(REG.SRC3_PB_DMA_STATUS_00) * BYTES_IN_MIU_LINE;

	if ((inused_bytes == 0) && (playback->status == CMD_START)) {
		AUD_PR_PRINT_SRC(id, AUD_INFO, priv,
			"[ALSA DUP]*****PCM Playback Buffer empty !!*****\n");
	} else if ((inused_bytes + copy_size) > playback->high_threshold) {
		//pr_err("*****PCM Playback Buffer full !!*****\n");
		return 0;
	}

	if ((inused_bytes + copy_size + playback->c_buf.u32RemainSize) <= playback->r_buf.size) {
		do {
			copy_tmp = playback->r_buf.buf_end - playback->r_buf.w_ptr;
			copy_tmp = (copy_tmp > copy_size) ? copy_size : copy_tmp;

			memcpy(playback->r_buf.w_ptr, buffer, copy_tmp);
			playback->r_buf.w_ptr += copy_tmp;
			buffer += copy_tmp;

			if (playback->r_buf.w_ptr == playback->r_buf.buf_end)
				playback->r_buf.w_ptr = playback->r_buf.addr;

			copy_size -= copy_tmp;
		} while (copy_size > 0);
	}

	/* flush MIU */
	smp_mb();

	copy_size = bytes;
	copy_size += playback->c_buf.u32RemainSize;
	playback->c_buf.u32RemainSize = copy_size % playback->u32TargetAlignmentSize;
	AUDIO_DO_ALIGNMENT(copy_size, playback->u32TargetAlignmentSize);

	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_04, 0xFFFF,
					(copy_size / BYTES_IN_MIU_LINE));
	/* BASE [26:16] */
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0400, 0x0400);
	mtk_alsa_write_reg_mask(REG.SRC3_PB_DMA_CFG_00, 0x0400, 0x0000);

	playback->copy_size += copy_size;
	playback->status = CMD_START;

	mtk_alsa_delaytask(1);

	return bytes;
}

static unsigned int dma_reader_write(struct mtk_dma_struct *playback,
	struct mtk_runtime_struct *dev_runtime, void *buffer, unsigned int bytes)
{
	unsigned int copy_size = 0;
	unsigned int copy_tmp = 0;
	unsigned int w_ptr_offset = 0;
	unsigned int inused_bytes = 0;
	struct duplex_priv *priv;
	int id;

	priv = (struct duplex_priv *) dev_runtime->private;
	id = dev_runtime->id;

	copy_size = bytes;
	inused_bytes = mtk_alsa_read_reg(REG.SRC2_PB_DMA_DDR_LEVEL) * BYTES_IN_MIU_LINE;

	if ((inused_bytes == 0) && (playback->status == CMD_START)) {
		AUD_PR_PRINT_SRC(id, AUD_INFO, priv,
			"[ALSA DUP]*****PCM Playback Buffer empty !!*****\n");
	} else if ((inused_bytes + copy_size) > playback->high_threshold) {
		//pr_err("[ALSA DUP]*****PCM Playback Buffer full !!*****\n");
		return 0;
	}

	if ((inused_bytes + copy_size + playback->c_buf.u32RemainSize) <= playback->r_buf.size) {
		do {
			copy_tmp = playback->r_buf.buf_end - playback->r_buf.w_ptr;
			copy_tmp = (copy_tmp > copy_size) ? copy_size : copy_tmp;

			memcpy(playback->r_buf.w_ptr, buffer, copy_tmp);
			playback->r_buf.w_ptr += copy_tmp;
			buffer += copy_tmp;

			if (playback->r_buf.w_ptr == playback->r_buf.buf_end)
				playback->r_buf.w_ptr = playback->r_buf.addr;

			copy_size -= copy_tmp;
		} while (copy_size > 0);
	}

	/* update PCM playback write pointer */
	w_ptr_offset = playback->r_buf.w_ptr - playback->r_buf.addr;
	w_ptr_offset += playback->c_buf.u32RemainSize;
	playback->c_buf.u32RemainSize = w_ptr_offset % BYTES_IN_MIU_LINE;
	AUDIO_DO_ALIGNMENT(w_ptr_offset, BYTES_IN_MIU_LINE);
	mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_WPTR, 0xFFFF,
				(w_ptr_offset / BYTES_IN_MIU_LINE));

	playback->copy_size += bytes;
	playback->status = CMD_START;

	/* flush MIU */
	smp_mb();

	/* ensure write pointer can be applied */
	mtk_alsa_delaytask(1);

	return bytes;
}

static unsigned int capture_read(struct mtk_dma_struct *capture,
					void *buffer, unsigned int bytes, int id)
{
	unsigned int rest_size_to_buffer_end = capture->r_buf.buf_end - capture->r_buf.r_ptr;
	unsigned int tmp_size = 0;
	unsigned int ptr_offset = 0;
	unsigned int REG_CAP_RPTR = 0;

	tmp_size = (rest_size_to_buffer_end > bytes) ? bytes : rest_size_to_buffer_end;

	if (id == SRC1_DEV)
		REG_CAP_RPTR = REG.CAP_DMA_RPTR;
	else if (id == SRC2_DEV)
		REG_CAP_RPTR = REG.SRC2_CAP_DMA_RPTR;
	else
		REG_CAP_RPTR = REG.SRC3_CAP_DMA_RPTR;

	memcpy(buffer, capture->r_buf.r_ptr, tmp_size);

	capture->r_buf.r_ptr += tmp_size;

	if (capture->r_buf.r_ptr == capture->r_buf.buf_end)
		capture->r_buf.r_ptr = capture->r_buf.addr;

	ptr_offset = capture->r_buf.r_ptr - capture->r_buf.addr;  // bytes unit

	ptr_offset += capture->c_buf.u32RemainSize;
	capture->c_buf.u32RemainSize = ptr_offset % BYTES_IN_MIU_LINE;
	AUDIO_DO_ALIGNMENT(ptr_offset, BYTES_IN_MIU_LINE);

	mtk_alsa_write_reg(REG_CAP_RPTR, (ptr_offset >> BYTES_IN_MIU_LINE_LOG2));

	/* flush MIU */
	smp_mb();

	capture->status = CMD_START;

	return tmp_size;
}

static int duplex_dma_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_runtime_struct *dev_runtime = priv->pcm_device[id];
	struct mtk_dma_struct *pcm_stream = NULL;
	struct snd_pcm_hardware *pb_hw;
	const struct snd_pcm_hw_constraint_list *rate_hw, *channel_hw;
	int ret;

	dev_runtime->private = priv;
	dev_runtime->id = id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	// clock init
	switch (id) {
	case SRC1_DEV:
		mtk_src1_clk_set_enable();
		break;
	case SRC2_DEV:
		mtk_src2_clk_set_enable();
		break;
	case SRC3_DEV:
		mtk_src3_clk_set_enable();
		break;
	default:
		pr_err("[ALSA CAP] %s[%d] id = %u clk enable fail\n"
						, __func__, __LINE__, id);
		break;
	}
	dev_runtime->clk_status = 1;

	if (substream->stream == E_PLAYBACK) {
		// playback case
		if (id == SRC1_DEV) {
			pb_hw = &mtk_pcm_default_hw;
			rate_hw = &duplex_play_rates_list;
			channel_hw = &duplex_play_channels_list;
		} else if (id == SRC2_DEV) {
			pb_hw = &src2_pb_dma_playback_default_hw;
			rate_hw = &duplex_play_rates_list;
			channel_hw = &duplex_play_channels_list;
		} else if (id == SRC3_DEV) {
			pb_hw = &mtk_pcm_default_hw;
			rate_hw = &duplex_play_rates_list;
			channel_hw = &duplex_play_channels_list;
		} else {
			pb_hw = &mtk_pcm_default_hw;
			rate_hw = &duplex_play_rates_list;
			channel_hw = &duplex_play_channels_list;
		}

		/* Set specified information */
		dev_runtime->substreams.p_substream = substream;

		dma_reader_timer_open(&dev_runtime->timer,
				(unsigned long)dev_runtime,
				(void *)dma_reader_monitor);

		snd_soc_set_runtime_hwparams(substream, pb_hw);

		ret = snd_pcm_hw_constraint_list(runtime, 0,
				SNDRV_PCM_HW_PARAM_RATE,
				rate_hw);
		if (ret < 0)
			return ret;

		ret = snd_pcm_hw_constraint_list(runtime, 0,
				SNDRV_PCM_HW_PARAM_CHANNELS,
				channel_hw);
		if (ret < 0)
			return ret;

		if (dev_runtime->spin_init_status == 0) {
			spin_lock_init(&dev_runtime->spin_lock);
			dev_runtime->spin_init_status = 1;
		}

		pcm_stream = &dev_runtime->playback;
	} else if (substream->stream == E_CAPTURE) {
		// capture case
		/* Set specified information */
		dev_runtime->substreams.c_substream = substream;

		snd_soc_set_runtime_hwparams(substream, &mtk_pcm_default_hw);

		ret = snd_pcm_hw_constraint_list(runtime, 0,
				SNDRV_PCM_HW_PARAM_RATE,
				&duplex_cap_rates_list);
		if (ret < 0)
			return ret;

		ret = snd_pcm_hw_constraint_list(runtime, 0,
				SNDRV_PCM_HW_PARAM_CHANNELS,
				&duplex_cap_channels_list);
		if (ret < 0)
			return ret;

		pcm_stream = &dev_runtime->capture;
	} else {
		pr_err("[ALSA DUP] %s[%d] EINVAL fail.\n", __func__, __LINE__);
		return -EINVAL;
	}

	/* Allocate system memory for Specific Copy */
	pcm_stream->c_buf.addr = vmalloc(MAX_DUPLEX_BUF_SIZE);
	if (pcm_stream->c_buf.addr == NULL)
		return -ENOMEM;

	memset((void *)pcm_stream->c_buf.addr, 0x00, MAX_DUPLEX_BUF_SIZE);
	pcm_stream->c_buf.size = MAX_DUPLEX_BUF_SIZE;

	return 0;
}

static void src_dma_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_runtime_struct *dev_runtime = priv->pcm_device[id];
	struct mtk_dma_struct *playback = &dev_runtime->playback;
	struct mtk_dma_struct *capture = &dev_runtime->capture;
	unsigned long flags;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	if (substream->stream == E_PLAYBACK) {
		dma_reader_timer_close(&dev_runtime->timer);

		spin_lock_irqsave(&dev_runtime->spin_lock, flags);
		memset(&dev_runtime->monitor, 0x00,
				sizeof(struct mtk_monitor_struct));
		spin_unlock_irqrestore(&dev_runtime->spin_lock, flags);
	}

	/* Stop DMA Reader */
	duplex_dma_stop(dev_runtime, id, substream->stream);
	mtk_alsa_delaytask(2);

	if (substream->stream == E_PLAYBACK)
		duplex_dma_exit(playback);
	else
		duplex_dma_exit(capture);

	switch (id) {
	case SRC1_DEV:
		mtk_src1_clk_set_disable();
		break;
	case SRC2_DEV:
		mtk_src2_clk_set_disable();
		break;
	case SRC3_DEV:
		mtk_src3_clk_set_disable();
		break;
	default:
		pr_err("[ALSA CAP] %s[%d] id = %u clk disable fail\n"
						, __func__, __LINE__, id);
		break;
	}
	dev_runtime->clk_status = 0;

}

static int duplex_dma_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	return 0;
}

static int duplex_dma_hw_free(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int id = rtd->cpu_dai->id;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);

	return 0;
}

void dup_dma_get_size_offset(struct duplex_priv *priv, int id,
				unsigned int *pb_offset, unsigned int *pb_size,
				unsigned int *cap_offset, unsigned int *cap_size)
{
	switch (id) {
	case SRC1_DEV:
		*pb_offset = priv->pb_dma_offset;
		*pb_size = priv->pb_dma_size;
		*cap_offset = priv->cap_dma_offset;
		*cap_size = priv->cap_dma_size;
		break;
	case SRC2_DEV:
		*pb_offset = priv->src2_pb_dma_offset;
		*pb_size = priv->src2_pb_dma_size;
		*cap_offset = priv->src2_cap_dma_offset;
		*cap_size = priv->src2_cap_dma_size;
		break;
	case SRC3_DEV:
		*pb_offset = priv->src3_pb_dma_offset;
		*pb_size = priv->src3_pb_dma_size;
		*cap_offset = priv->src3_cap_dma_offset;
		*cap_size = priv->src3_cap_dma_size;
		break;
	default:
		pr_err("[ALSA DUP] %s[%d] fail!\n", __func__, __LINE__);
		break;
	}
}

static int src_dma_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_runtime_struct *dev_runtime = priv->pcm_device[id];
	struct mtk_dma_struct *playback = &dev_runtime->playback;
	struct mtk_dma_struct *capture = &dev_runtime->capture;
	int err;
	unsigned int pb_dma_offset = 0;
	unsigned int pb_dma_size = 0;
	unsigned int cap_dma_offset = 0;
	unsigned int cap_dma_size = 0;

	ALSA_AUDIO_ENTER(dai->dev, id, priv);
	/*print hw parameter*/
	AUD_DEV_PRINT_SRC(dai->dev, id, AUD_INFO, priv, "\nparameter :\n"
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

	dup_dma_get_size_offset(priv, id, &pb_dma_offset, &pb_dma_size,
						&cap_dma_offset, &cap_dma_size);
	if ((!pb_dma_offset) || (!pb_dma_size) || (!cap_dma_offset) || (!cap_dma_size))
		return -ENXIO;

	if (substream->stream == E_PLAYBACK) {
		if ((playback->status != CMD_START) || (playback->initialized_status == 0)) {
			playback->channel_mode = runtime->channels;
			playback->sample_rate = runtime->rate;
			playback->sample_bits = runtime->sample_bits;
			/* Open SW DMA */
			err = duplex_dma_init(priv, playback, pb_dma_offset, pb_dma_size);
			if (err != 0)
				pr_err("[ALSA DUP]Err! SRC1 prepare init fail\n");

			duplex_playback_init(playback, dai, id);

			/* Configure SW DMA device status */
			playback->status = CMD_START;
		} else {
			/* Stop SW DMA */
			duplex_dma_stop(dev_runtime, id, E_PLAYBACK);
			mtk_alsa_delaytask(1);
			/* Start SW DMA */
			duplex_playback_init(playback, dai, id);
		}
	} else if (substream->stream == E_CAPTURE) {
		if ((capture->status != CMD_START) || (capture->initialized_status == 0)) {
			capture->channel_mode = runtime->channels;
			/* Open SW DMA */
			err = duplex_dma_init(priv, capture, cap_dma_offset, cap_dma_size);
			if (err != 0)
				pr_err("[ALSA DUP]Err! SRC1 prepare init fail\n");

			duplex_capture_init(capture, id);

			/* Configure SW DMA device status */
			capture->status = CMD_START;
		}
	} else {
		pr_err("[ALSA DUP] %s[%d] EINVAL fail.\n", __func__, __LINE__);
		return -EINVAL;
	}
	return 0;
}

static int duplex_dma_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_runtime_struct *dev_runtime = priv->pcm_device[id];
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
		pr_info("[ALSA DUP]Stream direction: %d, ", substream->stream);
		pr_info("Invalid playback's trigger command %d\n", cmd);
		err = -EINVAL;
		break;
	}

	return 0;
}

static const struct snd_soc_dai_ops src_dma_dai_ops = {
	.startup	= duplex_dma_startup,
	.shutdown	= src_dma_shutdown,
	.hw_params	= duplex_dma_hw_params,
	.hw_free	= duplex_dma_hw_free,
	.prepare	= src_dma_prepare,
	.trigger	= duplex_dma_trigger,
};

int src_dai_suspend(struct snd_soc_dai *dai)
{
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);
	unsigned int id = dai->id;
	struct mtk_runtime_struct *dev_runtime = priv->pcm_device[id];
	struct mtk_dma_struct *playback = &dev_runtime->playback;
	struct mtk_dma_struct *capture = &dev_runtime->capture;

	pr_err("[ALSA DUP]device%d suspend\n", id);

	if (dev_runtime->clk_status) {
		// clock init
		switch (id) {
		case SRC1_DEV:
			mtk_src1_clk_set_disable();
			break;
		case SRC2_DEV:
			mtk_src2_clk_set_disable();
			break;
		case SRC3_DEV:
			mtk_src3_clk_set_disable();
			break;
		default:
			pr_err("[ALSA DUP] %s[%d] id = %u clk enable fail\n"
							, __func__, __LINE__, id);
			break;
		}
		dev_runtime->clk_status = 0;
	}

	if (playback->status != CMD_STOP) {
		duplex_dma_stop(dev_runtime, id, E_PLAYBACK);
		playback->status = CMD_SUSPEND;
	}

	if (capture->status != CMD_STOP) {
		duplex_dma_stop(dev_runtime, id, E_CAPTURE);
		capture->status = CMD_SUSPEND;
	}

	return 0;
}

int src_dai_resume(struct snd_soc_dai *dai)
{
	struct duplex_priv *priv = snd_soc_dai_get_drvdata(dai);
	unsigned int id = dai->id;
	struct mtk_runtime_struct *dev_runtime = priv->pcm_device[id];
	struct mtk_dma_struct *playback = &dev_runtime->playback;
	struct mtk_dma_struct *capture = &dev_runtime->capture;
	int err = 0;
	unsigned int pb_dma_offset = 0;
	unsigned int pb_dma_size = 0;
	unsigned int cap_dma_offset = 0;
	unsigned int cap_dma_size = 0;

	pr_err("[ALSA DUP]device%d resume\n", id);

	dup_dma_get_size_offset(priv, id, &pb_dma_offset, &pb_dma_size,
						&cap_dma_offset, &cap_dma_size);
	if ((!pb_dma_offset) || (!pb_dma_size) || (!cap_dma_offset) || (!cap_dma_size))
		return -ENXIO;

	if (dev_runtime->clk_status == 0) {
		if (id == SRC1_DEV)
			mtk_src1_clk_set_enable();
		else if (id == SRC2_DEV)
			mtk_src2_clk_set_enable();
		else if (id == SRC3_DEV)
			mtk_src3_clk_set_enable();
		else
			pr_err("[ALSA DUP] %s[%d] id = %d clk enable fail\n"
							, __func__, __LINE__, id);
		dev_runtime->clk_status = 1;
	}

	if (playback->status == CMD_SUSPEND) {
		playback->status = CMD_RESUME;
		/* Open SW DMA */
		err = duplex_dma_init(priv, playback, pb_dma_offset, pb_dma_size);
		if (err != 0)
			pr_err("[ALSA DUP]Err! prepare init fail\n");

		duplex_playback_init(playback, dai, id);

		playback->status = CMD_START;
	}

	if (capture->status == CMD_SUSPEND) {
		capture->status = CMD_RESUME;

		/* Open SW DMA */
		err = duplex_dma_init(priv, capture, cap_dma_offset, cap_dma_size);
		if (err != 0)
			pr_err("[ALSA DUP]Err! resume fail\n");

		duplex_capture_init(capture, id);

		/* Configure SW DMA device status */
		capture->status = CMD_START;
	}

	return err;
}

static struct snd_soc_dai_driver duplex_dais[] = {
	{
		.name = "SRC1",
		.id = SRC1_DEV,
		.playback = {
			.stream_name = "SRC_PLAYBACK",
			.channels_min = 2,
			.channels_max = 12,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "SRC_CAPTURE",
			.channels_min = 2,
			.channels_max = 12,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &src_dma_dai_ops,
		.suspend = src_dai_suspend,
		.resume = src_dai_resume,
	},
	{
		.name = "SRC2",
		.id = SRC2_DEV,
		.playback = {
			.stream_name = "SRC2_PLAYBACK",
			.channels_min = 2,
			.channels_max = 12,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.capture = {
			.stream_name = "SRC2_CAPTURE",
			.channels_min = 2,
			.channels_max = 12,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &src_dma_dai_ops,
		.suspend = src_dai_suspend,
		.resume = src_dai_resume,
	},
	{
		.name = "SRC3",
		.id = SRC3_DEV,
		.playback = {
			.stream_name = "SRC3_PLAYBACK",
			.channels_min = 2,
			.channels_max = 12,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "SRC3_CAPTURE",
			.channels_min = 2,
			.channels_max = 12,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &src_dma_dai_ops,
		.suspend = src_dai_suspend,
		.resume = src_dai_resume,
	},
};
static snd_pcm_uframes_t duplex_pcm_pointer
				(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
				snd_soc_rtdcom_lookup(rtd, DUPLEX_NAME);
	struct duplex_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_runtime_struct *dev_runtime = priv->pcm_device[id];
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mtk_dma_struct *stream = NULL;
	int offset_bytes = 0;
	snd_pcm_uframes_t offset_pcm_samples = 0;
	snd_pcm_uframes_t new_hw_ptr = 0;
	snd_pcm_uframes_t new_hw_ptr_pos = 0;
	unsigned int new_w_ptr_offset = 0;
	unsigned char *new_w_ptr = NULL;
	unsigned short cap_dma_wptr = 0;

	if (substream->stream == E_PLAYBACK) {
		stream = &dev_runtime->playback;
		offset_bytes = dma_reader_get_consumed_bytes(stream, id);
	} else if (substream->stream == E_CAPTURE) {
		stream = &dev_runtime->capture;

		if (id == SRC1_DEV)
			cap_dma_wptr = mtk_alsa_read_reg(REG.CAP_DMA_WPTR);
		else if (id == SRC2_DEV)
			cap_dma_wptr = mtk_alsa_read_reg(REG.SRC2_CAP_DMA_WPTR);
		else
			cap_dma_wptr = mtk_alsa_read_reg(REG.SRC3_CAP_DMA_WPTR);

		new_w_ptr_offset = cap_dma_wptr * BYTES_IN_MIU_LINE;

		new_w_ptr = stream->r_buf.addr + new_w_ptr_offset;

		offset_bytes = new_w_ptr - stream->r_buf.w_ptr;
		if (offset_bytes < 0)
			offset_bytes += stream->r_buf.size;

		stream->r_buf.w_ptr = new_w_ptr;
	} else {
		pr_err("[ALSA DUP] %s[%d] EINVAL fail.\n", __func__, __LINE__);
		return -EINVAL;
	}

	offset_pcm_samples = bytes_to_frames(runtime, offset_bytes);
	new_hw_ptr = runtime->status->hw_ptr + offset_pcm_samples;
	new_hw_ptr_pos = new_hw_ptr % runtime->buffer_size;

	return new_hw_ptr_pos;
}

static int duplex_pcm_copy(struct snd_pcm_substream *substream, int channel,
		unsigned long pos, void __user *buf, unsigned long bytes)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
				snd_soc_rtdcom_lookup(rtd, DUPLEX_NAME);
	struct duplex_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int id = rtd->cpu_dai->id;
	struct mtk_runtime_struct *dev_runtime = priv->pcm_device[id];
	struct mtk_dma_struct *stream = NULL;
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned char *buffer_tmp = NULL;
	unsigned int period_size = 0;
	unsigned int request_size = 0;
	unsigned int copied_size = 0;
	unsigned int size_to_copy = 0;
	unsigned int size = 0;
	int retry_counter = 0;
	unsigned long flags;
	int ret = 0;

	dev_runtime->private = priv;
	dev_runtime->id = id;

	if (buf == NULL) {
		pr_err("[ALSA DUP]Err! invalid memory address!\n");
		return -EINVAL;
	}

	if (bytes == 0) {
		pr_err("[ALSA DUP]Err! request bytes is zero!\n");
		return -EINVAL;
	}

	if (substream->stream == E_PLAYBACK) {
		stream = &dev_runtime->playback;
		period_size = stream->period_size;
		buffer_tmp = stream->c_buf.addr;
		if (buffer_tmp == NULL) {
			pr_err("[ALSA DUP]Err! need allocate copy buffer!\n");
			return -ENXIO;
		}

		/* Wake up Monitor if necessary */
		if (dev_runtime->monitor.monitor_status == CMD_STOP) {
			while (1) {
				if (dma_reader_timer_reset(&dev_runtime->timer) == 0) {
					spin_lock_irqsave(&dev_runtime->spin_lock, flags);
					dev_runtime->monitor.monitor_status = CMD_START;
					spin_unlock_irqrestore(&dev_runtime->spin_lock, flags);
					break;
				}

				if ((++retry_counter) > 50) {
					pr_err("[ALSA DUP]fail to wakeup PB Monitor %d\n",
						substream->pcm->device);
					break;
				}

				mtk_alsa_delaytask(1);
			}
		}

		request_size = bytes;

		dma_reader_get_avail_bytes(stream, id);

		if ((runtime->format == SNDRV_PCM_FORMAT_S32_LE) ||
			(runtime->format == SNDRV_PCM_FORMAT_S32_BE))
			period_size = stream->period_size << 1;

		if (((substream->f_flags & O_NONBLOCK) > 0) &&
			(request_size > stream->c_buf.avail_size)) {
			pr_err("[ALSA DUP] Err! avail %u but request %u bytes\n",
							stream->c_buf.avail_size, request_size);
			return -EAGAIN;
		}

		do {
			size_to_copy = request_size - copied_size;
			if (size_to_copy > period_size)
				size_to_copy = period_size;

			ret = copy_from_user(buffer_tmp,
						(buf + copied_size), size_to_copy);

			if (ret) {
				pr_err("[ALSA DUP]Err! %d byte not copied from.\n", ret);
				size_to_copy = size_to_copy - ret;
			}

			while (1) {
				if (id == SRC1_DEV) {
					size = r2_dma_reader_write(stream, dev_runtime,
						(void *)buffer_tmp, size_to_copy);
				} else if (id == SRC2_DEV) {
					size = dma_reader_write(stream, dev_runtime,
					(void *)buffer_tmp, size_to_copy);
				} else if (id == SRC3_DEV) {
					size = r2_dma_reader2_write(stream, dev_runtime,
						(void *)buffer_tmp, size_to_copy);
				}

				if (size == 0) {
					if ((++retry_counter) > EXPIRATION_TIME) {
						pr_err("[ALSA DUP]Retry write\n");
						retry_counter = 0;
					}
					mtk_alsa_delaytask(5);
				} else
					break;
			}

			if ((runtime->format == SNDRV_PCM_FORMAT_S32_LE) ||
				(runtime->format == SNDRV_PCM_FORMAT_S32_BE))
				size <<= 1;

			copied_size += size;
		} while (copied_size < request_size);
	} else if (substream->stream == E_CAPTURE) {
		stream = &dev_runtime->capture;
		buffer_tmp = stream->c_buf.addr;
		if (buffer_tmp == NULL) {
			pr_err("[ALSA DUP]Err! need allocate copy buffer!\n");
			return -ENXIO;
		}

		request_size = bytes;
		capture_get_avail_bytes(stream);

		if (((substream->f_flags & O_NONBLOCK) > 0) &&
			(request_size > stream->c_buf.avail_size)) {
			return -EAGAIN;
		}

		do {
			period_size = request_size - copied_size;
			size = capture_read(stream, (void *)buffer_tmp, period_size, id);
			if (size > 0) {
				ret = copy_to_user((buf + copied_size), buffer_tmp, size);

				if (ret) {
					pr_err("[ALSA DUP]Err! %d byte not copied.\n", ret);
					copied_size += (size - ret);
				} else {
					copied_size += size;
				}
			}
		} while (copied_size < request_size);
	} else {
		pr_err("[ALSA DUP] %s[%d] EINVAL fail.\n", __func__, __LINE__);
		return -EINVAL;
	}
	return 0;
}

const struct snd_pcm_ops duplex_pcm_ops = {
	.ioctl = snd_pcm_lib_ioctl,
	.pointer = duplex_pcm_pointer,
	.copy_user = duplex_pcm_copy,
};

static int src_put_rate(unsigned short synthrate_h, unsigned short synthrate_l, int id)
{
	switch (id) {
	case SRC1_DEV:
		mtk_alsa_write_reg(REG.PB_DMA_SYNTH_H, synthrate_h);
		mtk_alsa_write_reg(REG.PB_DMA_SYNTH_L, synthrate_l);
		mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0101, 0x0100);
		mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0001, 0x0001);
		mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0001, 0x0000);
		break;
	case SRC2_DEV:
		mtk_alsa_write_reg(REG.SRC2_PB_DMA_SYNTH_H, synthrate_h);
		mtk_alsa_write_reg(REG.SRC2_PB_DMA_SYNTH_L, synthrate_l);
		mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_SYNTH_UPDATE, 0x1000, 0x1000);
		mtk_alsa_delaytask_us(50);
		mtk_alsa_write_reg_mask(REG.SRC2_PB_DMA_SYNTH_UPDATE, 0x1000, 0x0000);
		break;
	case SRC3_DEV:
		mtk_alsa_write_reg(REG.SRC3_PB_DMA_SYNTH_H, synthrate_h);
		mtk_alsa_write_reg(REG.SRC3_PB_DMA_SYNTH_L, synthrate_l);
		mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0404, 0x0400);
		mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0004, 0x0004);
		mtk_alsa_write_reg_mask(REG.PB_DMA_SYNTH_UPDATE, 0x0004, 0x0000);
		break;
	default:
		pr_err("[ALSA DUP]Err! un-supported duplex device !!!\n");
		break;
	}

	return 0;
}

static int src_control_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 65535;

	return 0;
}

static u32 calc_cap_buf_level(struct duplex_priv *priv, int id)
{
	unsigned short u16_wrptr = 0;
	unsigned short u16_rdptr = 0;
	unsigned int u32_level = 0;
	unsigned int u32_offset = 0;
	signed int s32_temp;

	switch (id) {
	case SRC1_DEV:
		u16_wrptr = mtk_alsa_read_reg(REG.CAP_DMA_WPTR);
		u16_rdptr = mtk_alsa_read_reg(REG.CAP_DMA_RPTR);
		u32_level = priv->cap_dma_size / BYTES_IN_MIU_LINE;
		break;
	case SRC2_DEV:
		u16_wrptr = mtk_alsa_read_reg(REG.SRC2_CAP_DMA_WPTR);
		u16_rdptr = mtk_alsa_read_reg(REG.SRC2_CAP_DMA_RPTR);
		u32_level = priv->src2_cap_dma_size / BYTES_IN_MIU_LINE;
		break;
	case SRC3_DEV:
		u16_wrptr = mtk_alsa_read_reg(REG.SRC3_CAP_DMA_WPTR);
		u16_rdptr = mtk_alsa_read_reg(REG.SRC3_CAP_DMA_RPTR);
		u32_level = priv->src3_cap_dma_size / BYTES_IN_MIU_LINE;
		break;
	default:
		pr_err("[ALSA DUP] %s[%d] invalid id = %d\n", __func__, __LINE__, id);
		break;
	}

	s32_temp = u16_wrptr - u16_rdptr;

	if (s32_temp > 0)
		u32_offset = s32_temp;
	else
		u32_offset = u32_level + s32_temp;

	if (u32_offset > u32_level)
		pr_err("[ALSA DUP] %s[%d] fail !!!\n", __func__, __LINE__);

	u32_offset = u32_offset * BYTES_IN_MIU_LINE;

	return u32_offset;  // unit: bytes
}

static int src_control_get_value(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_kcontrol_component(kcontrol);
	struct duplex_priv *priv = snd_soc_component_get_drvdata(component);

	unsigned int u32_ret_0 = 0;
	unsigned int u32_ret_1 = 0;

	switch (kcontrol->private_value) {
	// SRC1
	case SRC1_RATE:
		u32_ret_0 = mtk_alsa_read_reg(REG.PB_DMA_SYNTH_H);
		u32_ret_1 = mtk_alsa_read_reg(REG.PB_DMA_SYNTH_L);
		break;
	case SRC1_PB_BUF_LEVEL:
		u32_ret_0 = mtk_alsa_read_reg(REG.PB_DMA_STATUS_00) * BYTES_IN_MIU_LINE;
		break;
	case SRC1_CAP_BUF_LEVEL:
		u32_ret_0 = calc_cap_buf_level(priv, SRC1_DEV);
		break;
	// SRC2
	case SRC2_RATE:
		u32_ret_0 = mtk_alsa_read_reg(REG.SRC2_PB_DMA_SYNTH_H);
		u32_ret_1 = mtk_alsa_read_reg(REG.SRC2_PB_DMA_SYNTH_L);
		break;
	case SRC2_PB_BUF_LEVEL:
		u32_ret_0 = mtk_alsa_read_reg(REG.SRC2_PB_DMA_DDR_LEVEL) * BYTES_IN_MIU_LINE;
		break;
	case SRC2_CAP_BUF_LEVEL:
		u32_ret_0 = calc_cap_buf_level(priv, SRC2_DEV);
		break;
	// SRC3
	case SRC3_RATE:
		u32_ret_0 = mtk_alsa_read_reg(REG.SRC3_PB_DMA_SYNTH_H);
		u32_ret_1 = mtk_alsa_read_reg(REG.SRC3_PB_DMA_SYNTH_L);
		break;
	case SRC3_PB_BUF_LEVEL:
		u32_ret_0 = mtk_alsa_read_reg(REG.SRC3_PB_DMA_STATUS_00) * BYTES_IN_MIU_LINE;
		break;
	case SRC3_CAP_BUF_LEVEL:
		u32_ret_0 = calc_cap_buf_level(priv, SRC3_DEV);
		break;
	default:
		pr_err("[ALSA DUP] %s[%d] fail !!!\n", __func__, __LINE__);
		break;
	}

	ucontrol->value.integer.value[0] = u32_ret_0;
	ucontrol->value.integer.value[1] = u32_ret_1;

	return 0;
}

static int src_control_put_value(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	long param1 = ucontrol->value.integer.value[0];
	long param2 = ucontrol->value.integer.value[1];

	switch (kcontrol->private_value) {
	case SRC1_RATE:
		// r2 dma reader
		if ((param1 > 0xFFFF) || (param2 > 0xFFFF)) {
			pr_err("[ALSA DUP] %s[%d] param overbound !!!\n", __func__, __LINE__);
			break;
		}

		src_put_rate((unsigned short) param1, (unsigned short) param2, SRC1_DEV);
		break;
	case SRC2_RATE:
		// decoder5 dma reader
		if ((param1 > 0xFFFF) || (param2 > 0xFFFF)) {
			pr_err("[ALSA DUP] %s[%d] param overbound !!!\n", __func__, __LINE__);
			break;
		}

		src_put_rate((unsigned short) param1, (unsigned short) param2, SRC2_DEV);
		break;
	case SRC3_RATE:
		// r2 dma reader2
		if ((param1 > 0xFFFF) || (param2 > 0xFFFF)) {
			pr_err("[ALSA DUP] %s[%d] param overbound !!!\n", __func__, __LINE__);
			break;
		}

		src_put_rate((unsigned short) param1, (unsigned short) param2, SRC3_DEV);
		break;
	default:
		pr_err("[ALSA DUP] %s[%d] fail !!!\n", __func__, __LINE__);
		break;
	}

	return 0;
}

static const struct snd_kcontrol_new duplex_controls[] = {
	SOC_INTEGER_SRC("SRC1 RATE", SRC1_RATE),
	SOC_INTEGER_SRC("SRC1 GET PB BUF LEVEL", SRC1_PB_BUF_LEVEL),
	SOC_INTEGER_SRC("SRC1 GET CAP BUF LEVEL", SRC1_CAP_BUF_LEVEL),

	SOC_INTEGER_SRC("SRC2 RATE", SRC2_RATE),
	SOC_INTEGER_SRC("SRC2 GET PB BUF LEVEL", SRC2_PB_BUF_LEVEL),
	SOC_INTEGER_SRC("SRC2 GET CAP BUF LEVEL", SRC2_CAP_BUF_LEVEL),

	SOC_INTEGER_SRC("SRC3 RATE", SRC3_RATE),
	SOC_INTEGER_SRC("SRC3 GET PB BUF LEVEL", SRC3_PB_BUF_LEVEL),
	SOC_INTEGER_SRC("SRC3 GET CAP BUF LEVEL", SRC3_CAP_BUF_LEVEL),
};

static const struct snd_soc_component_driver duplex_dai_component = {
	.name			= DUPLEX_NAME,
	.probe			= duplex_probe,
	.remove			= duplex_remove,
	.ops			= &duplex_pcm_ops,
	.controls		= duplex_controls,
	.num_controls		= ARRAY_SIZE(duplex_controls),
};

unsigned int duplex_get_dts_u32(struct device_node *np, const char *name)
{
	unsigned int value;
	int ret;

	ret = of_property_read_u32(np, name, &value);
	if (ret)
		WARN_ON("[ALSA DUP]Can't get DTS property\n");

	return value;
}

static int duplex_parse_registers(void)
{
	struct device_node *np;

	memset(&REG, 0x0, sizeof(struct duplex_register));

	np = of_find_node_by_name(NULL, "duplex-register");
	if (!np)
		return -ENODEV;

	// SRC1
	REG.PB_DMA_SYNTH_H = duplex_get_dts_u32(np, "reg_pb_dma_synth_h");
	REG.PB_DMA_SYNTH_L = duplex_get_dts_u32(np, "reg_pb_dma_synth_l");
	REG.PB_DMA_SYNTH_UPDATE = duplex_get_dts_u32(np, "reg_pb_dma_synth_update");
	REG.PB_DMA_CFG_00 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_00");
	REG.PB_DMA_CFG_01 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_01");
	REG.PB_DMA_CFG_02 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_02");
	REG.PB_DMA_CFG_03 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_03");
	REG.PB_DMA_CFG_04 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_04");
	REG.PB_DMA_CFG_05 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_05");
	REG.PB_DMA_CFG_06 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_06");
	REG.PB_DMA_CFG_07 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_07");
	REG.PB_DMA_CFG_08 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_08");
	REG.PB_DMA_CFG_09 = duplex_get_dts_u32(np, "reg_pb_dma_cfg_09");
	REG.PB_DMA_ENABLE = duplex_get_dts_u32(np, "reg_pb_dma_enable");
	REG.PB_DMA_STATUS_00 = duplex_get_dts_u32(np, "reg_pb_dma_status_00");
	REG.CAP_DMA_CTRL = duplex_get_dts_u32(np, "reg_cap_dma_ctrl");
	REG.CAP_DMA_RPTR = duplex_get_dts_u32(np, "reg_cap_dma_rptr");
	REG.CAP_DMA_WPTR = duplex_get_dts_u32(np, "reg_cap_dma_wptr");

	// SRC2
	REG.SRC2_PB_DMA_CTRL = duplex_get_dts_u32(np, "reg_src2_pb_dma_ctrl");
	REG.SRC2_PB_DMA_WPTR = duplex_get_dts_u32(np, "reg_src2_pb_dma_wptr");
	REG.SRC2_PB_DMA_DDR_LEVEL = duplex_get_dts_u32(np, "reg_src2_pb_dma_ddr_level");
	REG.SRC2_PB_DMA_SYNTH_H = duplex_get_dts_u32(np, "reg_src2_pb_dma_synth_h");
	REG.SRC2_PB_DMA_SYNTH_L = duplex_get_dts_u32(np, "reg_src2_pb_dma_synth_l");
	REG.SRC2_PB_DMA_SYNTH_UPDATE = duplex_get_dts_u32(np, "reg_src2_pb_dma_synth_update");
	REG.SRC2_CAP_DMA_CTRL = duplex_get_dts_u32(np, "reg_src2_cap_dma_ctrl");
	REG.SRC2_CAP_DMA_RPTR = duplex_get_dts_u32(np, "reg_src2_cap_dma_rptr");
	REG.SRC2_CAP_DMA_WPTR = duplex_get_dts_u32(np, "reg_src2_cap_dma_wptr");

	// SRC3
	REG.SRC3_PB_DMA_SYNTH_H = duplex_get_dts_u32(np, "reg_src3_pb_dma_synth_h");
	REG.SRC3_PB_DMA_SYNTH_L = duplex_get_dts_u32(np, "reg_src3_pb_dma_synth_l");
	REG.SRC3_PB_DMA_CFG_00 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_00");
	REG.SRC3_PB_DMA_CFG_01 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_01");
	REG.SRC3_PB_DMA_CFG_02 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_02");
	REG.SRC3_PB_DMA_CFG_03 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_03");
	REG.SRC3_PB_DMA_CFG_04 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_04");
	REG.SRC3_PB_DMA_CFG_05 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_05");
	REG.SRC3_PB_DMA_CFG_06 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_06");
	REG.SRC3_PB_DMA_CFG_07 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_07");
	REG.SRC3_PB_DMA_CFG_08 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_08");
	REG.SRC3_PB_DMA_CFG_09 = duplex_get_dts_u32(np, "reg_src3_pb_dma_cfg_09");
	REG.SRC3_PB_DMA_ENABLE = duplex_get_dts_u32(np, "reg_src3_pb_dma_enable");
	REG.SRC3_PB_DMA_STATUS_00 = duplex_get_dts_u32(np, "reg_src3_pb_dma_status_00");
	REG.SRC3_CAP_DMA_CTRL = duplex_get_dts_u32(np, "reg_src3_cap_dma_ctrl");
	REG.SRC3_CAP_DMA_RPTR = duplex_get_dts_u32(np, "reg_src3_cap_dma_rptr");
	REG.SRC3_CAP_DMA_WPTR = duplex_get_dts_u32(np, "reg_src3_cap_dma_wptr");

	np = of_find_node_by_name(NULL, "channel-mux-register");
	if (!np)
		return -ENODEV;

	REG.CHANNEL_M1_CFG = duplex_get_dts_u32(np, "reg_ch_m1_cfg");
	REG.CHANNEL_M2_CFG = duplex_get_dts_u32(np, "reg_ch_m2_cfg");
	REG.CHANNEL_M3_CFG = duplex_get_dts_u32(np, "reg_ch_m3_cfg");
	REG.CHANNEL_M4_CFG = duplex_get_dts_u32(np, "reg_ch_m4_cfg");
	REG.CHANNEL_M5_CFG = duplex_get_dts_u32(np, "reg_ch_m5_cfg");
	REG.CHANNEL_M6_CFG = duplex_get_dts_u32(np, "reg_ch_m6_cfg");
	REG.CHANNEL_M7_CFG = duplex_get_dts_u32(np, "reg_ch_m7_cfg");
	REG.CHANNEL_M8_CFG = duplex_get_dts_u32(np, "reg_ch_m8_cfg");
	REG.CHANNEL_M9_CFG = duplex_get_dts_u32(np, "reg_ch_m9_cfg");
	REG.CHANNEL_M10_CFG = duplex_get_dts_u32(np, "reg_ch_m10_cfg");
	REG.CHANNEL_M11_CFG = duplex_get_dts_u32(np, "reg_ch_m11_cfg");
	REG.CHANNEL_M12_CFG = duplex_get_dts_u32(np, "reg_ch_m12_cfg");
	REG.CHANNEL_M13_CFG = duplex_get_dts_u32(np, "reg_ch_m13_cfg");
	REG.CHANNEL_M14_CFG = duplex_get_dts_u32(np, "reg_ch_m14_cfg");
	REG.CHANNEL_M15_CFG = duplex_get_dts_u32(np, "reg_ch_m15_cfg");
	REG.CHANNEL_M16_CFG = duplex_get_dts_u32(np, "reg_ch_m16_cfg");

	REG.CHANNEL_5_CFG = duplex_get_dts_u32(np, "reg_ch_5_cfg");
	REG.CHANNEL_6_CFG = duplex_get_dts_u32(np, "reg_ch_6_cfg");
	REG.CHANNEL_7_CFG = duplex_get_dts_u32(np, "reg_ch_7_cfg");
	REG.CHANNEL_8_CFG = duplex_get_dts_u32(np, "reg_ch_8_cfg");
	REG.CHANNEL_9_CFG = duplex_get_dts_u32(np, "reg_ch_9_cfg");
	REG.CHANNEL_10_CFG = duplex_get_dts_u32(np, "reg_ch_10_cfg");

	return 0;
}

static int duplex_parse_mmap(struct device_node *np, struct duplex_priv *priv)
{
	struct of_mmap_info_data of_mmap_info = {};
	int ret;

	ret = of_mtk_get_reserved_memory_info_by_idx(np,
						0, &of_mmap_info);
	if (ret) {
		pr_err("[ALSA DUP]get audio mmap fail\n");
		return -EINVAL;
	}

	priv->mem_info.miu = of_mmap_info.miu;
	priv->mem_info.bus_addr = of_mmap_info.start_bus_address;
	priv->mem_info.buffer_size = of_mmap_info.buffer_size;

	return 0;
}

int duplex_parse_memory(struct device_node *np, struct duplex_priv *priv)
{
	int ret;

	// SRC1
	ret = of_property_read_u32(np, "playback_dma_offset",
					&priv->pb_dma_offset);
	if (ret) {
		pr_err("[ALSA DUP]can't get playback dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "playback_dma_size",
					&priv->pb_dma_size);
	if (ret) {
		pr_err("[ALSA DUP]can't get speaker dma size\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "capture_dma_offset",
					&priv->cap_dma_offset);
	if (ret) {
		pr_err("[ALSA DUP]can't get capture dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "capture_dma_size",
					&priv->cap_dma_size);
	if (ret) {
		pr_err("[ALSA DUP]can't get capture dma size\n");
		return -EINVAL;
	}

	// SRC2
	ret = of_property_read_u32(np, "src2_playback_dma_offset",
					&priv->src2_pb_dma_offset);
	if (ret) {
		pr_err("[ALSA DUP]can't get src2 playback dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "src2_playback_dma_size",
					&priv->src2_pb_dma_size);
	if (ret) {
		pr_err("[ALSA DUP]can't get src2 speaker dma size\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "src2_capture_dma_offset",
					&priv->src2_cap_dma_offset);
	if (ret) {
		pr_err("[ALSA DUP]can't get src2 capture dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "src2_capture_dma_size",
					&priv->src2_cap_dma_size);
	if (ret) {
		pr_err("[ALSA DUP]can't get src2 capture dma size\n");
		return -EINVAL;
	}

	// SRC3
	ret = of_property_read_u32(np, "src3_playback_dma_offset",
					&priv->src3_pb_dma_offset);
	if (ret) {
		pr_err("[ALSA DUP]can't get src3 playback dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "src3_playback_dma_size",
					&priv->src3_pb_dma_size);
	if (ret) {
		pr_err("[ALSA DUP]can't get src3 speaker dma size\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "src3_capture_dma_offset",
					&priv->src3_cap_dma_offset);
	if (ret) {
		pr_err("[ALSA DUP]can't get src3 capture dma offset\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "src3_capture_dma_size",
					&priv->src3_cap_dma_size);
	if (ret) {
		pr_err("[ALSA DUP]can't get src3 capture dma size\n");
		return -EINVAL;
	}

	return 0;
}

int duplex_dev_probe(struct platform_device *pdev)
{
	struct duplex_priv *priv;
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

	for (i = 0; i < DUPLEX_DEV_NUM; i++) {
		priv->pcm_device[i] = devm_kzalloc(&pdev->dev,
				sizeof(struct mtk_runtime_struct), GFP_KERNEL);
		if (!priv->pcm_device[i])
			return -ENOMEM;
	}

	platform_set_drvdata(pdev, priv);

	ret = devm_snd_soc_register_component(&pdev->dev,
					&duplex_dai_component,
					duplex_dais,
					ARRAY_SIZE(duplex_dais));
	if (ret) {
		dev_err(dev, "[ALSA DUP]soc_register_component fail %d\n", ret);
		return ret;
	}

	/* parse dts */
	ret = of_property_read_u64(node, "memory_bus_base",
					&priv->mem_info.memory_bus_base);
	if (ret) {
		dev_err(dev, "[ALSA DUP]can't get miu bus base\n");
		return -EINVAL;
	}

	/* parse registers */
	ret = duplex_parse_registers();
	if (ret) {
		dev_err(dev, "[ALSA DUP]parse register fail %d\n", ret);
		return ret;
	}
	/* parse dma memory info */
	ret = duplex_parse_memory(node, priv);
	if (ret) {
		dev_err(dev, "[ALSA DUP]parse memory fail %d\n", ret);
		return ret;
	}
	/* parse mmap */
	ret = duplex_parse_mmap(node, priv);
	if (ret) {
		dev_err(dev, "[ALSA DUP]parse mmap fail %d\n", ret);
		return ret;
	}

	return 0;
}

int duplex_dev_remove(struct platform_device *pdev)
{
	// TODO: free memory and clear parameters
	return 0;
}

MODULE_DESCRIPTION("Mediatek ALSA SoC Duplex platform driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:snd-duplex");
