// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/version.h>	// KERNEL_VERSION

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/control.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include "snd_soc_dma.h"
#include <linux/kthread.h>
#include "voc_communication.h"
#include "mtk_dbg.h"

#define PCM_NAME "voc-platform"
#define MIU_BASE	0x20000000
#define FAKE_CAPTURE 0


//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

#define params_period_bytes(p) \
	(hw_param_interval_c((p), SNDRV_PCM_HW_PARAM_PERIOD_BYTES)->min)

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
static unsigned long mtk_log_level = LOGLEVEL_ERR;

static const struct snd_pcm_hardware voc_pcm_capture_hardware = {
	.info =
#if defined(CONFIG_MS_VOC_MMAP_SUPPORT)
		SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
#endif
		SNDRV_PCM_INFO_INTERLEAVED,

	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
		SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000,
	.rate_min = 8000,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = 1512 * 1024,
	.period_bytes_min = 1 * 1024,
	.period_bytes_max = 64 * 1024,
	.periods_min = 4,
	.periods_max = 256,	//32,
};

#if FAKE_CAPTURE
#define SINE_TONE_SIZE (16)
static const short sine_tone[SINE_TONE_SIZE] = {
	0, 1254, 2317, 3027,
	3277, 3027, 2317, 1254,
	0, -1254, -2317, -3027,
	-3277, -3027, -2317, -1254};
#endif

//------------------------------------------------------------------------------
//  Function definition
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
void voc_dma_write2Log(struct voc_pcm_data *pcm_data, char *str)
{
	if (pcm_data->dbgInfo.writed < MAX_LOG_LEN) {
		if (scnprintf(pcm_data->dbgInfo.log[pcm_data->dbgInfo.writed].event,
								MAX_LEN, "%s", str) > 0) {
			ktime_get_ts(&(pcm_data->dbgInfo.log[pcm_data->dbgInfo.writed].tstamp));
			pcm_data->dbgInfo.writed++;
		} else {
			voc_err("snprintf fail, event: %s\n",
					pcm_data->dbgInfo.log[pcm_data->dbgInfo.writed].event);
		}
	}
}
void voc_dma_params_init(struct voc_pcm_data *pcm_data, u32 dma_buf_size, u32 period_size)
{
	pcm_data->pcm_level_cnt = 0;
	pcm_data->pcm_last_period_num = 0;
	pcm_data->pcm_period_size = period_size;//frames_to_bytes(runtime, runtime->period_size);
	pcm_data->pcm_buf_size = dma_buf_size;//runtime->dma_bytes;
	pcm_data->pcm_cm4_period = 0;
	pcm_data->pcm_status = PCM_NORMAL;
}

void voc_dma_reset(struct voc_pcm_data *pcm_data)
{
	pcm_data->pcm_level_cnt = 0;
	pcm_data->pcm_last_period_num = 0;
}

u32 voc_dma_get_level_cnt(struct voc_pcm_data *pcm_data)
{
	return pcm_data->pcm_level_cnt;
}

u32 voc_dma_trig_level_cnt(struct voc_pcm_data *pcm_data, u32 data_size)
{
	int ret = 0;

	if (data_size <= pcm_data->pcm_level_cnt) {
		pcm_data->pcm_level_cnt -= data_size;
		ret = data_size;
	} else {
		pr_err("%s size %u too large %u\n", __func__
		, data_size, pcm_data->pcm_level_cnt);
	}
	return ret;
}

#if FAKE_CAPTURE
static void capture_timer_function(struct timer_list *t)
{
	struct voc_pcm_data *pcm_data = from_timer(pcm_data, t, timer);
	struct snd_pcm_substream *substream =
		(struct snd_pcm_substream *)pcm_data->capture_stream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	//struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct voc_pcm_runtime_data *prtd = runtime->private_data;

	//capture_timer_start(pcm_data);
	mod_timer(&pcm_data->timer, jiffies + msecs_to_jiffies(16));//16ms

	prtd->dma_level_count += 4096;
	snd_pcm_period_elapsed(substream);
}
#endif

static int voc_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	struct snd_pcm *pcm = rtd->pcm;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);
	struct voc_pcm_runtime_data *prtd;
	int ret = 0;
	voc_debug("%s: stream = %s, pcmC%dD%d (substream = %s), dai_link = %s\n",
			__func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
			pcm->card->number,
			pcm->device, substream->name,
			dai_link->name);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		snd_soc_set_runtime_hwparams(substream,
						&voc_pcm_capture_hardware);
	} else {
		ret = -EINVAL;
		goto out;
	}
	/* Ensure that buffer size is a multiple of period size */
//#if 0
//	ret = snd_pcm_hw_constraint_integer(runtime,
//						SNDRV_PCM_HW_PARAM_PERIODS);
//	if (ret < 0)
//		goto out;
//#endif

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (prtd == NULL) {
		ret = -ENOMEM;
		goto out;
	}
//	spin_lock_init(&prtd->lock);
	runtime->private_data = prtd;
	pcm_data->capture_stream = substream;	// for snd_pcm_period_elapsed
#if defined(CONFIG_MS_VOC_MMAP_SUPPORT)
	pcm_data->mmap_mode = 0;
#endif
	voc_dma_write2Log(pcm_data, "pcm open");
out:
	return ret;
}

static int voc_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);

	voc_debug("%s: stream = %s\n", __func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

#if FAKE_CAPTURE
		del_timer_sync(&pcm_data->timer);
#endif

	kfree(runtime->private_data);//free voc_pcm_runtime_data
	runtime->private_data = NULL;
	pcm_data->capture_stream = NULL;
	voc_dma_write2Log(pcm_data, "pcm close");
	return 0;
}

//extern unsigned int voc_soc_get_ref_num(void);

/* this may get called several times by oss emulation */
//hw params: hw argument for user space, including
//channels, sample rate, pcm format, period size,
//period count. These are constrained by HW constraints.
static int voc_pcm_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct voc_pcm_runtime_data *prtd = runtime->private_data;
	struct voc_pcm_dma_data *dma_data;
	int ret = 0;
#if FAKE_CAPTURE
#else
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);
#endif
	unsigned int frame_bits;

	runtime->format = params_format(params);
	runtime->subformat = params_subformat(params);
	runtime->channels = params_channels(params);
	runtime->rate = params_rate(params);

	voc_debug("%s: stream = %s\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));
	voc_debug("params_channels     = %d\n", params_channels(params));
	voc_debug("params_rate         = %d\n", params_rate(params));
	voc_debug("params_period_size  = %d\n", params_period_size(params));
	voc_debug("params_periods      = %d\n", params_periods(params));
	voc_debug("params_buffer_size  = %d\n", params_buffer_size(params));
	voc_debug("params_buffer_bytes = %d\n", params_buffer_bytes(params));
	voc_debug("params_sample_width = %d\n",
			snd_pcm_format_physical_width(params_format(params)));
	voc_debug
		("params_access = %d\n", params_access(params));
	voc_debug
		("params_format = %d\n,", params_format(params));
	voc_debug
		("params_subformat = %d\n", params_subformat(params));

	frame_bits =
		snd_pcm_format_physical_width(params_format(params)) / 8 *
		params_channels(params);

	dma_data = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

	if (!dma_data)
		return 0;

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);
	if (prtd->dma_data)
		return 0;

	prtd->dma_data = dma_data;

	/*
	 * Link channel with itself so DMA doesn't need any
	 * reprogramming while looping the buffer
	 */
	voc_debug("dma name = %s, channel id = %lu\n",
			dma_data->name,
			dma_data->channel);
//	voc_debug("dma buf phy addr = 0x%x\n",
//			(unsigned long)
//			(runtime->dma_addr
//				- MIU_BASE));
	voc_debug("dma buf vir addr = %p\n", runtime->dma_area);
	voc_debug("dma buf size = %zu\n", runtime->dma_bytes);

#if FAKE_CAPTURE
	return 0;
#else
	// Re-set up the underrun and overrun
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
//		if (voc_machine_get_mic_bitwidth(pcm_data->rpmsg_dev) > 16) {
//			if (snd_pcm_format_physical_width(params_format(params))
//				!= 32) {
//				voc_err("bitwidth %d should be 32\n",
//						snd_pcm_format_physical_width
//						(params_format(params)));
//				return -EFAULT;
//			}
//		} else if (snd_pcm_format_physical_width
//					(params_format(params)) != 16) {
//			pr_err("bitwidth %d should be 16\n",
//			snd_pcm_format_physical_width(params_format(params)));
//			return -EFAULT;
//		}
//        if(voc_soc_get_mic_num()+voc_soc_get_ref_num()
//			!=params_channels(params))
//		{
//            printk( "channel number should be %d\n",
//					voc_soc_get_mic_num()
//					+ voc_soc_get_ref_num());
//            return -EFAULT;
//		}

		//printk( "%s: param OK!!\n", __func__);
		/* enable sinegen for debug */
		//VocDmaEnableSinegen(true);
		voc_dma_params_init(pcm_data, params_buffer_bytes(params),
		params_period_bytes(params));

		// TODO: The hardcode setting will be got from device tree.
		ret = voc_dma_init_channel(pcm_data->rpmsg_dev,
			(u32)(runtime->dma_addr - MIU_BASE),
			runtime->dma_bytes,
			params_channels(params),
			snd_pcm_format_physical_width(params_format(params)),
			params_rate(params));
		if (ret)
			return -EINVAL;
		voc_dma_write2Log(pcm_data, "initial channel");
	}
#endif

	memset(runtime->dma_area, 0, runtime->dma_bytes);

	return ret;
}

static int voc_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct voc_pcm_runtime_data *prtd = runtime->private_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);
	voc_debug("%s: stream = %s\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	if (prtd->dma_data == NULL)
		return 0;

	//free_irq(VOC_IRQ_ID, (void *)substream);

	prtd->dma_data = NULL;
	snd_pcm_set_runtime_buffer(substream, NULL);
	voc_dma_write2Log(pcm_data, "voc_pcm_hw_free");
	return 0;
}

static int voc_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct voc_pcm_runtime_data *prtd = runtime->private_data;
	struct voc_pcm_dma_data *dma_data = prtd->dma_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);
	int ret = 0;

#if FAKE_CAPTURE
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data = snd_soc_component_get_drvdata(component);
#else
#endif
	voc_debug("%s: stream = %s, channel = %s\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
			dma_data->name);
	voc_dma_write2Log(pcm_data, "voc_pcm_prepare");

#if FAKE_CAPTURE
	del_timer_sync(&pcm_data->timer);
#endif

	return ret;
}

static int voc_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct voc_pcm_runtime_data *prtd = runtime->private_data;
	struct voc_pcm_dma_data *dma_data = prtd->dma_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);
	unsigned long flags;
	int ret = 0;
//    struct timespec   tv;

	voc_debug("%s: stream = %s, channel = %s, cmd = %d\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
			dma_data->name, cmd);

// AUD_PRINTF(KERN_INFO, "!!BACH_DMA1_CTRL_0 = 0x%x,
//		BACH_DMA1_CTRL_8 = 0x%x, level count = %d\n",
//		InfinityReadReg(BACH_REG_BANK1,
//				BACH_DMA1_CTRL_0),
//		InfinityReadReg(BACH_REG_BANK1,
//				BACH_DMA1_CTRL_8),
//		InfinityDmaGetLevelCnt(prtd->dma_data->channel));
	if (substream->stream != SNDRV_PCM_STREAM_CAPTURE)
		ret = -EINVAL;

	spin_lock_irqsave(&pcm_data->lock, flags);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		//voc_dma_reset();
#if FAKE_CAPTURE
		mod_timer(&pcm_data->timer, jiffies + msecs_to_jiffies(16));//16ms
#else
		pcm_data->voc_pcm_running = 1;
		voc_dma_start_channel(pcm_data->rpmsg_dev);
#endif

		if (cmd == SNDRV_PCM_TRIGGER_START)
			voc_dma_write2Log(pcm_data, "alsa trigger vd start");
		else if (cmd == SNDRV_PCM_TRIGGER_RESUME)
			voc_dma_write2Log(pcm_data, "alsa trigger vd resume");
		else if (cmd == SNDRV_PCM_TRIGGER_PAUSE_RELEASE)
			voc_dma_write2Log(pcm_data, "alsa trigger PAUSE_RELEASE:");

		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		memset(runtime->dma_area, 0, runtime->dma_bytes);
		prtd->dma_level_count = 0;
		prtd->state = DMA_EMPTY;


		/* Ensure that the DMA done */
		wmb();
#if FAKE_CAPTURE
		del_timer(&pcm_data->timer);
		pcm_data->timer.expires = 0;
#else
		pcm_data->voc_pcm_running = 0;
		voc_dma_stop_channel(pcm_data->rpmsg_dev);

		if (cmd == SNDRV_PCM_TRIGGER_STOP)
			voc_dma_write2Log(pcm_data, "alsa trigger vd stop");
		else if (cmd == SNDRV_PCM_TRIGGER_SUSPEND)
			voc_dma_write2Log(pcm_data, "alsa trigger vd suspend");
		else if (cmd == SNDRV_PCM_TRIGGER_PAUSE_PUSH)
			voc_dma_write2Log(pcm_data, "alsa trigger PAUSE_PUSH");

		//voc_reset_audio(pcm_data->rpmsg_dev);
		//voc_dma_reset(pcm_data);
#endif
		break;
	default:
		ret = -EINVAL;
	}
	spin_unlock_irqrestore(&pcm_data->lock, flags);

	// AUD_PRINTF(KERN_INFO,
	//	"!!Out BACH_DMA1_CTRL_0 = 0x%x,
	//	BACH_DMA1_CTRL_8 = 0x%x,
	//	level count = %d\n",
	//	InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0),
	//	InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8),
	//	InfinityDmaGetLevelCnt(prtd->dma_data->channel));

	return ret;
}

static snd_pcm_uframes_t voc_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct voc_pcm_runtime_data *prtd = runtime->private_data;
	struct voc_pcm_dma_data *dma_data = prtd->dma_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);

	snd_pcm_uframes_t offset = 0;
	unsigned long flags;
#if FAKE_CAPTURE
#else
	size_t last_dma_count_level = 0;
#endif

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		spin_lock_irqsave(&pcm_data->lock, flags);

		/* Ensure that the DMA done */
		rmb();

		if (prtd->state == DMA_FULL || prtd->state == DMA_LOCALFULL) {
			spin_unlock_irqrestore(&pcm_data->lock, flags);
			voc_warn("BUFFER FULL : %d!!\n", prtd->state);
			return SNDRV_PCM_POS_XRUN;
		}

		//Update water level: point will call this function to
		//get current address of transmitted data every transmission,
		//then pcm native can calculate point address of dma buffer
		//and available space
#if FAKE_CAPTURE
	if (prtd->dma_level_count > runtime->dma_bytes * 2)
		prtd->dma_level_count -= runtime->dma_bytes;
	offset =
			bytes_to_frames(runtime,
				((prtd->dma_level_count) %
						runtime->dma_bytes));
#else

		last_dma_count_level = voc_dma_get_level_cnt(pcm_data);

		if (prtd->dma_level_count > (runtime->dma_bytes) * 2)
			prtd->dma_level_count -= runtime->dma_bytes;
		offset =
			bytes_to_frames(runtime,
					((prtd->dma_level_count +
						last_dma_count_level) %
						runtime->dma_bytes));
#endif
		spin_unlock_irqrestore(&pcm_data->lock, flags);
	}

	voc_debug("%s: stream id = %d, channel id = %lu, frame offset = 0x%x\n",
			__func__, substream->stream, dma_data->channel,
			(unsigned int)offset);

	return offset;
}

/* pos & count : bytes */
static int voc_pcm_copy(struct snd_pcm_substream *substream,
			int channel, unsigned long pos,
			void __user *dst, unsigned long count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct voc_pcm_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);
	//struct voc_pcm_dma_data *dma_data = prtd->dma_data;
	struct snd_kcontrol *kctl = NULL;
	struct snd_ctl_elem_id elem_id;
#if FAKE_CAPTURE
	unsigned char *zeroBuf = kmalloc(count, GFP_KERNEL);
	int bytewidth = snd_pcm_format_physical_width(runtime->format)/BITS_PER_BYTE;
	int channels = runtime->channels;
	int sample_frame = count/bytewidth/channels;
	int i, j;
	static int z;
#else
	unsigned long flags;
	unsigned char *hwbuf = runtime->dma_area + pos;
	unsigned long cnt;
#endif
	volatile unsigned int state;

	/* Ensure that the DMA done */
	rmb();
	state = prtd->state;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {

		/* Ensure that the DMA done */
		rmb();
#if FAKE_CAPTURE
#else
		if (prtd->state == DMA_FULL) {
			voc_err("BUFFER FULL!!\n");
			snd_pcm_stream_lock_irqsave(substream, flags);
			snd_pcm_stop(substream, SNDRV_PCM_STATE_XRUN);
			snd_pcm_stream_unlock_irqrestore(substream, flags);
			return -EPIPE;
		}
#endif

#if FAKE_CAPTURE
		for (i = 0; i < sample_frame; i++, z++) {
			for (j = 0; j < channels; j++)
				*((short *)zeroBuf + sample_frame*j + i)
				= sine_tone[z%SINE_TONE_SIZE];
		}
		if (copy_to_user(dst, zeroBuf, count)) {
			kfree(zeroBuf);
			return -EFAULT;
		}
		kfree(zeroBuf);
#else
		if (copy_to_user(dst, hwbuf, count))
			return -EFAULT;
		spin_lock_irqsave(&pcm_data->lock, flags);

		if (prtd->state == DMA_EMPTY && state != prtd->state) {
			spin_unlock_irqrestore(&pcm_data->lock, flags);
			return -EPIPE;
		}

		cnt = voc_dma_trig_level_cnt(pcm_data, count);
#endif
		prtd->state = DMA_NORMAL;

		/* Ensure that the DMA done */
		wmb();
#if FAKE_CAPTURE
		prtd->dma_level_count -= count;
#else
		prtd->dma_level_count += count;
		spin_unlock_irqrestore(&pcm_data->lock, flags);
#endif

	}
	if (pcm_data->sysfs_notify_test > 0) {
		memset(&elem_id, 0, sizeof(elem_id));
		elem_id.numid = pcm_data->sysfs_notify_test;
		kctl = snd_ctl_find_id(substream->pcm->card, &elem_id);
		if (kctl != NULL)
			snd_ctl_notify(substream->pcm->card, SNDRV_CTL_EVENT_MASK_VALUE, &kctl->id);
		pcm_data->sysfs_notify_test = 0;
	}

	voc_debug("frame_pos = 0x%lx, frame_count = 0x%lx, framLevelCnt = %u\n",
			pos, count, voc_dma_get_level_cnt(pcm_data));

	return 0;
}

static struct snd_pcm_ops voc_pcm_ops = {
	.open = voc_pcm_open,
	.close = voc_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = voc_pcm_hw_params,
	.hw_free = voc_pcm_hw_free,
	.prepare = voc_pcm_prepare,
	.trigger = voc_pcm_trigger,
	.pointer = voc_pcm_pointer,
	.copy_user = voc_pcm_copy,
};

unsigned char *voc_alloc_dmem(struct voc_pcm_data *pcm_data,
			int size, dma_addr_t *addr)
{
	unsigned char *ptr = NULL;
	u32 mem_free_size = pcm_data->voc_pcm_size;
	phys_addr_t mem_cur_pos = pcm_data->voc_buf_paddr
				+ pcm_data->voc_pcm_offset;

	if (!addr)
		return NULL;

	if (mem_free_size && mem_free_size > size) {
		ptr = (unsigned char *)ioremap_wc(mem_cur_pos + MIU_BASE, size);
		if (!ptr) {
			voc_err("ioremap NULL address,phys : %pa\n", &mem_cur_pos);
			*addr = 0x0;
		} else
			*addr = (dma_addr_t)(mem_cur_pos + MIU_BASE);
	} else {
		*addr = 0x0;
		voc_err("No enough memory, free = %u\n", mem_free_size);
	}

	return ptr;
}

static u64 voc_pcm_dmamask = DMA_BIT_MASK(32);
#if FAKE_CAPTURE
#else
static int voc_pcm_preallocate_dma_buffer(struct snd_pcm *pcm,
				int stream, struct voc_pcm_data *pcm_data)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = 0;

	voc_debug("%s: stream = %s\r\n", __func__,
				(substream->stream ==
				SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	if (stream == SNDRV_PCM_STREAM_CAPTURE)  // CAPTURE device
		size = voc_pcm_capture_hardware.buffer_bytes_max;
	else
		return -EINVAL;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
#if FAKE_CAPTURE
#else
	buf->area = voc_alloc_dmem(pcm_data, PAGE_ALIGN(size), &buf->addr);
#endif
	if (!buf->area)
		return -ENOMEM;
	buf->bytes = PAGE_ALIGN(size);

	voc_debug("dma buffer size 0x%lx\n", (unsigned long)buf->bytes);
	voc_debug("physical dma address %pad\n", &buf->addr);
	voc_debug("virtual dma address,%p\n", buf->area);

	return 0;
}
#endif

void voc_free_dmem(void *addr)
{
	iounmap(addr);
}

static void voc_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;

	voc_debug("%s\n", __func__);

	substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
	if (!substream)
		return;

	buf = &substream->dma_buffer;
	if (!buf->area)
		return;

#if FAKE_CAPTURE
#else
	voc_free_dmem((void *)buf->area);
#endif
	buf->area = NULL;

}

static int voc_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	struct snd_pcm_substream *substream;
#if FAKE_CAPTURE
#else
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, PCM_NAME);
	struct voc_pcm_data *pcm_data =
		snd_soc_component_get_drvdata(component);
#endif
	int ret = 0;

	voc_info("%s: snd_pcm device id = %d\r\n", __func__, pcm->device);

	substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &voc_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (substream) {
#if FAKE_CAPTURE
		snd_pcm_lib_preallocate_pages(substream,
				SNDRV_DMA_TYPE_CONTINUOUS,
				snd_dma_continuous_data(GFP_KERNEL),
				le32_to_cpu(64 * 1024),
				le32_to_cpu(64 * 1024));
		substream->dma_buffer.addr =
			virt_to_phys(substream->dma_buffer.area);
		//TODO: use DMA support va to pa
		voc_info("%s: phy = 0x%llx, virt = 0x%p\r\n",
				__func__, substream->dma_buffer.addr,
				substream->dma_buffer.area);
		//voc_info("%s: size = %ld\r\n", __func__,
		//		virt_to_phys(substream->dma_buffer.area),
		//		substream->dma_buffer.bytes);
#else
		ret = voc_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE, pcm_data);
		if (ret)
			goto out;
#endif
	}
#if FAKE_CAPTURE
#else
out:
	/* free preallocated buffers in case of error */
	if (ret)
		voc_pcm_free_dma_buffers(pcm);
#endif

	return ret;
}

const struct snd_soc_component_driver voc_platform_driver = {
	.name = PCM_NAME,
	.ops = &voc_pcm_ops,
	.pcm_new = voc_pcm_new,
	.pcm_free = voc_pcm_free_dma_buffers,
};

/*
 * TODO: Maybe we should use other mechanism (deferred probe mechanism or
 *       somthing like that) for binding rpmsg device driver and pwm chip device
 *       driver.
 */
static int voc_pcm_add_thread(void *param)
{
	struct voc_pcm_data *pcm_data = (struct voc_pcm_data *)param;

	voc_dev_info(pcm_data->dev, "Wait voc pcm and rpmsg device binding...\n");

	// complete when rpmsg device and voc soc card device binded
	wait_for_completion(&pcm_data->rpmsg_bind);
	voc_dev_info(pcm_data->dev, "voc pcm and rpmsg device binded\n");

	return 0;
}

///sys/devices/platform/voc-platform/mtk_dbg/event_log
static ssize_t event_log_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct voc_pcm_data *pcm_data = dev_get_drvdata(dev);
	int count = 0, ret = 0;
	int i = 0;

	for (i = 0; i < pcm_data->dbgInfo.writed; i++) {
		if (strlen(pcm_data->dbgInfo.log[i].event) != 0) {
			ret = snprintf(buf+count, MAX_LEN, "%d:%lld.%.9ld, %s\n", i,
				(long long)pcm_data->dbgInfo.log[i].tstamp.tv_sec,
				pcm_data->dbgInfo.log[i].tstamp.tv_nsec,
				pcm_data->dbgInfo.log[i].event);
			count += ret;
		}
	}
	return count;
}

static ssize_t event_log_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct voc_pcm_data *pcm_data = dev_get_drvdata(dev);
	unsigned long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;
	if (item > 0) { //clear log
		pcm_data->dbgInfo.writed = 0;
		memset(pcm_data->dbgInfo.log, 0, sizeof(struct operation_log) * MAX_LOG_LEN);
	}
	return count;
}
static DEVICE_ATTR_RW(event_log);
///sys/devices/platform/voc-platform/mtk_dbg/notify_test
static ssize_t notify_test_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct voc_pcm_data *pcm_data = dev_get_drvdata(dev);

	return scnprintf(buf, MAX_LEN, "%u\n", pcm_data->sysfs_notify_test);
}

static ssize_t notify_test_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct voc_pcm_data *pcm_data = dev_get_drvdata(dev);
	unsigned long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;
	if (item >= 0)
		pcm_data->sysfs_notify_test = item;
	return count;

}
static DEVICE_ATTR_RW(notify_test);
///sys/devices/platform/voc-platform/mtk_dbg/log_level
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
static ssize_t help_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	return scnprintf(buf, MAX_LEN, "Debug Help:\n"
				"- log_level <RW>: To control debug log level.\n"
				"                  Read log_level value.\n"
				"- notify_test <RW>: send value chagne event to notify user space.\n"
				"                  The input parameter represents the value of which\n"
				"                  item has changed\n"
				"- event_log <RW>: dump soc_dma event (max : 30)\n"
				"                 Enter a number greater than 0 to clear the log\n");
}
static DEVICE_ATTR_RO(help);
static struct attribute *mtk_dtv_snd_dma_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	&dev_attr_notify_test.attr,
	&dev_attr_event_log.attr,
	NULL,
};
static const struct attribute_group mtk_dtv_snd_dma_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_snd_dma_attrs
};
static int mtk_dtv_snd_dma_create_sysfs_attr(struct platform_device *pdev)
{
	int ret = 0;

	/* Create device attribute files */
	//voc_dev_info (pdev->dev, "Create device attribute files\n");
	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_dtv_snd_dma_attr_group);
	if (ret)
		voc_dev_err(&pdev->dev, "[%d]Fail to create sysfs files: %d\n", __LINE__, ret);
	return ret;
}
static int mtk_dtv_snd_dma_remove_sysfs_attr(struct platform_device *pdev)
{
	/* Remove device attribute files */
	voc_dev_info(&(pdev->dev), "Remove device attribute files\n");
	sysfs_remove_group(&pdev->dev.kobj, &mtk_dtv_snd_dma_attr_group);
	return 0;
}
static int voc_platform_probe(struct platform_device *pdev)
{
	struct voc_pcm_data *pcm_data;
	static struct task_struct *t;
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	u32 voc_pcm_offset, voc_pcm_size;
	u32 voc_buf_paddr, voc_buf_size;

	voc_info("%s\n", __func__);

	pcm_data =
	    devm_kzalloc(&pdev->dev, sizeof(struct voc_pcm_data), GFP_KERNEL);
	if (!pcm_data) {
		voc_err("Malloc failed!!\n");
		return -ENOMEM;
	}

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32))) {
		voc_dev_err(&pdev->dev, "dma_set_mask_and_coherent failed\n");
		return -ENODEV;
	}

	ret = of_property_read_u32_index(np, "voc-buf-info", 0, &voc_buf_paddr);
	pcm_data->voc_buf_paddr = voc_buf_paddr;

	ret = of_property_read_u32_index(np, "voc-buf-info", 1, &voc_buf_size);
	pcm_data->voc_buf_size = voc_buf_size;

	ret = of_property_read_u32(np, "voc-pcm-offset", &voc_pcm_offset);
	pcm_data->voc_pcm_offset = voc_pcm_offset;

	ret = of_property_read_u32(np, "voc-pcm-size", &voc_pcm_size);
	pcm_data->voc_pcm_size = voc_pcm_size;
	pcm_data->sysfs_notify_test = 0;
	memset(&pcm_data->dbgInfo, 0, sizeof(struct voc_dma_dbg));

	spin_lock_init(&pcm_data->lock);
	platform_set_drvdata(pdev, pcm_data);
#if FAKE_CAPTURE
	timer_setup(&pcm_data->timer, capture_timer_function, 0);
#endif

	if (ret < 0) {
		voc_err("request_irq failed, err = %d!!\n", ret);
		return ret;
	}

	ret = devm_snd_soc_register_component(&pdev->dev, &voc_platform_driver, NULL, 0);

	if (ret)
		voc_err("err_platform\n");
	else
		voc_info("ASoC: platform register %s\n", dev_name(&pdev->dev));

	// add to voc_soc_card list
	INIT_LIST_HEAD(&pcm_data->list);
	list_add_tail(&pcm_data->list, &voc_pcm_dev_list);
	init_completion(&pcm_data->rpmsg_bind);

	// check if there is a valid rpmsg_dev
	ret = _mtk_bind_rpmsg_voc_pcm(NULL, pcm_data);
	if (ret) {
		voc_dev_err(&pdev->dev, "binding rpmsg-voc failed! (%d)\n", ret);
		goto bind_fail;
	}

	pcm_data->dev = &pdev->dev;

	/*
	 * TODO: check probe behavior match definition of platform driver
	 *       because initialization is not complete
	 */
	/* thread for waiting rpmsg binded, continue rest initial sequence after
	 * rpmsg device binded.
	 */

	t = kthread_run(voc_pcm_add_thread, (void *)pcm_data, "voc_pcm_add");
	if (IS_ERR(t)) {
		voc_dev_err(&pdev->dev, "create thread for add voc sound card failed!\n");
		ret = IS_ERR(t);
		goto bind_fail;
	}

	mtk_dtv_snd_dma_create_sysfs_attr(pdev);
	pcm_data->dbgInfo.writed = 0;
	memset(pcm_data->dbgInfo.log, 0, sizeof(struct operation_log) * MAX_LOG_LEN);
	return ret;

bind_fail:
	list_del(&pcm_data->list);
	return ret;
}

static int voc_platform_remove(struct platform_device *pdev)
{
	struct voc_pcm_data *pcm_data = platform_get_drvdata(pdev);

	voc_debug("%s\r\n", __func__);

	voc_reset_voice(pcm_data->rpmsg_dev);
	list_del(&pcm_data->list);

	mtk_dtv_snd_dma_remove_sysfs_attr(pdev);
	return 0;
}

static struct platform_driver voc_dma_driver = {
	.driver = {
			.name = "voc-platform",
			.owner = THIS_MODULE,
			},
	.probe = voc_platform_probe,
	.remove = voc_platform_remove,
};
module_platform_driver(voc_dma_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allen-HW Hsu, allen-hw.hsu@mediatek.com");
MODULE_DESCRIPTION("Voice PCM module");
