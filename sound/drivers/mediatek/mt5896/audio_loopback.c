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
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <sound/soc.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/info.h>
#include <sound/initval.h>
#include <sound/snd_mediatek.h>
#include <asm-generic/div64.h>

#include "audio_loopback.h"
#include "mtk-reserved-memory.h"
#include "clk-dtv.h"

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define SND_AUDIO_LOOPBACK_CARD "mtkaloopsndcard"
#define SND_AUDIO_LOOPBACK_DRIVER "mtk-audio-loopback"
#define SND_AUDIO_LOOPBACK_PCM_NAME "mtk-audio-loopback-platform"
#define SND_AUDIO_LOOPBACK_IRQ_NAME "mediatek,mtk-audio-loopback-platform"
#define SND_AUDIO_LOOPBACK_CAPTURE_CARD_NUM 1
#define SND_AUDIO_LOOPBACK_PLAYBACK_CARD_NUM 1
#define SND_AUDIO_LOOPBACK_DEVICE_NUM_MAX 1

#define SND_AUDIO_BITS_TO_BYTES(bits)      (bits >> 3)
#define SND_AUDIO_REG_READ_WORD(reg)       (readw(reg))

#define	SND_AUDIO_REG_WRITE_BYTE(reg, val)	(writeb(val, reg))
#define	SND_AUDIO_REG_WRITE_WORD(reg, val)	(writew(val, reg))

#define SND_AUDIO_DO_ALIGNMENT(val, alignment_size)			\
	(val = (val / alignment_size) * alignment_size)

#define	SND_AUDIO_REG_WRITE_MASK_BYTE(reg, mask, val)			\
	SND_AUDIO_REG_WRITE_BYTE(reg, (readb(reg) & ~(mask)) | (val & mask))

#define	SND_AUDIO_REG_WRITE_MASK_2BYTE(reg, mask, val)			\
	SND_AUDIO_REG_WRITE_WORD(reg, (readw(reg) & ~(mask)) | (val & mask))

#define MAX_LEN                (512)
#define EXPIRATION_TIME        (500)

#define FROM_SPEAKER           (0)
#define FROM_ADC2              (1)
#define FROM_ADC1              (2)
#define FROM_I2S_RX_2CH        (3)
#define FROM_I2S_RX_4CH        (4)
#define FROM_I2S_RX_8CH        (5)

//hardware parameter define
#define CH_2                   (2)
#define CH_4                   (4)
#define CH_8                   (8)
#define CH_12                  (12)
#define CH_14                  (14)

#define RATE_16000             (16000)
#define RATE_48000             (48000)

#define FORMAT_S16             (2)
#define FORMAT_S32             (4)

#define MAX_BUFFER_SIZE        (128*1024)
#define MIN_PERIOD_BYTE        (64)
#define MAX_PERIOD_BYTE        (MAX_BUFFER_SIZE >> 2)

#define MIN_PERIOD_NUM         (2)
#define MAX_PERIOD_NUM         (150)

#define FIFO_SIZE              (0)
#define LINE_SIZE              (16)
#define ALIGN_SIZE             (4096)
#define BYTE_ALIGN_SIZE        (4)
#define FIFO_ALIGN_SIZE        (8)
#define MS_UNIT                (1000)
#define INTERVAL_UNIT          (256) //256 frames
#define PERIOD_NS_UNIT         (16000000)
#define TIMER_UNIT             (1)
#define TIMER_SHIFT            (2)

#define PERIOD_SHIFT           (2)
#define THRESHOLD_SHIFT        (3)

//delay unit
#define WAIT_1MS               (1)
#define WAIT_2MS               (2)
#define WAIT_5MS               (5)
#define WAIT_50US              (50)

//src parameter define
#define AEC_RATE               RATE_16000
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//ALSA card related
static unsigned long mtk_log_level = LOGLEVEL_ERR;// LOGLEVEL_ERR LOGLEVEL_CRIT
static LIST_HEAD(audio_loopback_card_dev_list);
static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int pcm_capture_substreams[SNDRV_CARDS] = {
	[0 ... (SNDRV_CARDS - 1)] = SND_AUDIO_LOOPBACK_CAPTURE_CARD_NUM
};
static int pcm_playback_substreams[SNDRV_CARDS] = {
	[0 ... (SNDRV_CARDS - 1)] = SND_AUDIO_LOOPBACK_PLAYBACK_CARD_NUM
};

module_param_array(pcm_capture_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_capture_substreams,
"PCM capture substreams # (1) for mediatek audio loopback driver.");
module_param_array(pcm_playback_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_playback_substreams,
"PCM playback substreams # (1) for mediatek audio loopback driver.");

static struct platform_device *device;
struct audio_loopback_register REG;
static struct timespec _stDspTimeStamp;
static struct timespec _stAecTimeStamp;
static struct timespec _stSwTimeStamp;
//------------------------------------------------------------------------------
//  Enumerate And Structure
//------------------------------------------------------------------------------
static unsigned int card_reg[LOOPBACK_REG_LEN] = {
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
};

static const char *device_name = { "Loopback" };

static const char * const aec_sigen_onoff_text[] = { "Off", "On" };
static const char * const pcm_slave_active_onoff_text[] = { "Off", "On" };
static const char * const pcm_capture_source_text[] = {
	"Speaker",
	"ADC2",
	"ADC1",
	"I2S RX 2CH",
	"I2S RX 4CH",
	"I2S RX 8CH"};

static const struct soc_enum aec_sigen_onoff_enum =
SOC_ENUM_SINGLE(LOOPBACK_REG_AEC_SIGEN_ONOFF, 0, 2, aec_sigen_onoff_text);

static const struct soc_enum pcm_slave_active_onoff_enum =
SOC_ENUM_SINGLE(LOOPBACK_REG_PCM_SLAVE_ACTIVE_ONOFF, 0, 2, pcm_slave_active_onoff_text);

static const struct soc_enum pcm_capture_source_enum =
SOC_ENUM_SINGLE(LOOPBACK_REG_PCM_CAPTURE_SOURCE, 0, 6, pcm_capture_source_text);

//PCM related
static const struct snd_pcm_hardware audio_loopback_pcm_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates = SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_48000,
	.rate_min = RATE_16000,
	.rate_max = RATE_48000,
	.channels_min = CH_2,
	.channels_max = CH_14,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MIN_PERIOD_BYTE,
	.period_bytes_max = MAX_PERIOD_BYTE,
	.periods_min = MIN_PERIOD_NUM,
	.periods_max = MAX_PERIOD_NUM,
	.fifo_size = FIFO_SIZE,
};

/* Default hardware settings of ALSA PCM playback */
static struct snd_pcm_hardware audio_loopback_pcm_pb_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates = SNDRV_PCM_RATE_48000,
	.rate_min = RATE_48000,
	.rate_max = RATE_48000,
	.channels_min = CH_2,
	.channels_max = CH_14,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MIN_PERIOD_BYTE,
	.period_bytes_max = MAX_PERIOD_BYTE,
	.periods_min = MIN_PERIOD_NUM,
	.periods_max = MAX_PERIOD_NUM,
	.fifo_size = FIFO_SIZE,
};
//------------------------------------------------------------------------------
//  Reigster Function
//------------------------------------------------------------------------------
static void __iomem *audio_loopback_get_bank_base(unsigned int bank)
{
	void __iomem *reg = NULL;
	int i;

	for (i = 0; i < REG.AEC_BANK_NUM; i++) {
		if (bank == REG.AEC_REG_BANK[i]) {
			reg = REG.AEC_REG_BASE[i];
			break;
		}
	}

	for (i = 0; i < REG.DSP_BANK_NUM; i++) {
		if (bank == REG.DSP_REG_BANK[i]) {
			reg = REG.DSP_REG_BASE[i];
			break;
		}
	}

	if (reg == NULL)
		audio_warn("[%s]Invalid bank", __func__);

	return reg;
}

static unsigned short audio_loopback_read_reg(unsigned int addr)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	base = addr & BANK_MASK;
	offset = addr & OFFSET_MASK;
	offset = offset << OFFSET_SHIFT;

	reg = audio_loopback_get_bank_base(base);
	if (!reg)
		return 0;

	return SND_AUDIO_REG_READ_WORD(reg + offset);
}

static void audio_loopback_write_reg_mask_byte(unsigned int addr,
			unsigned char mask, unsigned char val)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	if (addr == 0) {
		audio_warn("[%s] error! try to write addr 0x0\n", __func__);
		return;
	}

	base = addr & BANK_MASK;
	offset = addr & OFFSET_MASK;
	offset = (offset << OFFSET_SHIFT) - (offset & OFFSET_SHIFT);

	reg = audio_loopback_get_bank_base(base);
	if (!reg)
		return;

	SND_AUDIO_REG_WRITE_MASK_BYTE(reg + offset, mask, val);
}

static void audio_loopback_write_reg_mask(unsigned int addr,
			unsigned short mask, unsigned short val)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	if (addr == 0) {
		audio_warn("[%s] error! try to write addr 0x0\n", __func__);
		return;
	}

	base = addr & BANK_MASK;
	offset = addr & OFFSET_MASK;
	offset = offset << OFFSET_SHIFT;

	reg = audio_loopback_get_bank_base(base);
	if (!reg)
		return;

	SND_AUDIO_REG_WRITE_MASK_2BYTE(reg + offset, mask, val);
}

static void audio_loopback_write_reg(unsigned int addr, unsigned short val)
{
	void __iomem *reg = NULL;
	unsigned int base;
	unsigned short offset;

	if (addr == 0) {
		audio_warn("[%s] error! try to write addr 0x0\n", __func__);
		return;
	}

	base = addr & BANK_MASK;
	offset = addr & OFFSET_MASK;
	offset = offset << OFFSET_SHIFT;

	reg = audio_loopback_get_bank_base(base);
	if (!reg)
		return;

	SND_AUDIO_REG_WRITE_WORD(reg + offset, val);
}

static void audio_loopback_timer_valid_enable(void)
{
	struct device_node *np;
	struct device_node *timer_np;
	unsigned int reg = 0;
	unsigned short mask = 0;
	unsigned short val = 0;
	u32 tmp = 0;
	int cnt = 0;
	int ret;
	int i;

	np = of_find_node_by_name(NULL, "mtk-audio-loopback");
	if (!np) {
		audio_err("[%s] No found device\n", __func__);
		return;
	}

	timer_np = of_find_node_by_name(np, "timeren");
	if (!timer_np) {
		audio_err("[%s]No found timeren device\n", __func__);
		return;
	}

	audio_info("[%s]timer valid en!\n", __func__);
	ret = of_property_read_u32(timer_np, "num", &cnt);
	if (ret == 0) {
		audio_info("[%s]found %d timer valid needs to enable\n", __func__, cnt);
		for (i = 0; i < cnt * TIMEREN_IN_LINE; i++) {
			ret = of_property_read_u32_index(timer_np, "list", i, &tmp);
			if (ret != 0) {
				audio_err("[%s]Invalid timer en info(%d-%d)\n", __func__,
					i / TIMEREN_IN_LINE, i % TIMEREN_IN_LINE);
				break;
			}
			switch (i % TIMEREN_IN_LINE) {
			case TIMEREN_REG:
				reg = (unsigned int)(tmp);
				break;
			case TIMEREN_MASK:
				mask = (unsigned short)(tmp & WORD_MASK);
				break;
			case TIMEREN_VALUE:
				val = (unsigned short)(tmp & WORD_MASK);
				break;
			}
			if (i % TIMEREN_IN_LINE == TIMEREN_VALUE) {
				audio_info("[%s]%d: timer(%x, %x, %x)\n", __func__,
					i / TIMEREN_IN_LINE, reg, mask, val);
				audio_loopback_write_reg_mask(reg, mask, val);
			}
		}
	}
}

static void audio_loopback_clk_sw_enable(void)
{
	struct device_node *np;
	struct device_node *sw_np;
	struct swen_info info;
	u32 tmp = 0;
	int cnt = 0;
	int ret;
	int i;

	np = of_find_node_by_name(NULL, "mtk-audio-loopback");
	if (!np) {
		audio_err("[%s] No found device\n", __func__);
		return;
	}

	sw_np = of_find_node_by_name(np, "swen");
	if (!sw_np) {
		audio_err("[%s]No found swen device\n", __func__);
		return;
	}

	audio_info("[%s]sw en!\n", __func__);
	ret = of_property_read_u32(sw_np, "num", &cnt);
	if (ret == 0) {
		audio_info("[%s]found %d clk needs to enable\n", __func__, cnt);
		for (i = 0; i < cnt * SWEN_IN_LINE; i++) {
			ret = of_property_read_u32_index(sw_np, "list", i, &tmp);
			if (ret != 0) {
				audio_err("[%s]Invalid clk sw en info(%d-%d)\n", __func__,
					i / SWEN_IN_LINE, i % SWEN_IN_LINE);
				break;
			}
			switch (i % SWEN_IN_LINE) {
			case SWEN_OFFSET:
				info.offset = tmp;
				break;
			case SWEN_START:
				info.start = tmp;
				break;
			case SWEN_END:
				info.end = tmp;
				break;
			}
			if (i % SWEN_IN_LINE == SWEN_END) {
				audio_info("[%s]%d: clk(%x, %x, %x)\n", __func__,
					i / SWEN_IN_LINE, info.offset, info.start, info.end);
				clock_write(CLK_NONPM, info.offset, 1, info.start, info.end);
				memset(&info, 0, sizeof(struct swen_info));
			}
		}
	}
}

static void audio_loopback_aud_init_table(void)
{
	//Check whether DSP already init AUPLL
	if ((audio_loopback_read_reg(MIX_DSP_REG_AUD02_0X76) & MIX_DSP_AUPLL_CHECK)
		== MIX_DSP_AUPLL_CHECK) {

		audio_info("[%s]Aud init!(%d)\n", __func__,
			audio_loopback_read_reg(MIX_DSP_REG_AUD02_0X76) & MIX_DSP_AUPLL_CHECK);

		//Init pre-AUPLL
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X9E,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X20);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X9F,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X1C);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X9C,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X00);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X9D,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0XC0);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X9C,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X00);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X9D,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X80);

		//Init AUPLL
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X70,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0XD0);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X71,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X12);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X76,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X00);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X77,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X03);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X00,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X0D);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X01,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X7F);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X00,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X0F);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X01,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X7F);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X00,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X0D);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X01,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X7F);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X00,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X00);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD02_0X01,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X00);
	}

	if (((audio_loopback_read_reg(MIX_DSP_REG_AUD01_0X46) >> MIX_DSP_AUPLL_SHIFT)
		& MIX_DSP_AUPLL_EN_ALL) == ZERO) {

		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD01_0X46,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_RESET_0X00);
		audio_loopback_write_reg_mask_byte(MIX_DSP_REG_AUD01_0X47,
				MIX_DSP_AUPLL_MASK, MIX_DSP_AUPLL_EN_ALL);
	}
}

static void audio_loopback_delaytask_us(unsigned int u32Us)
{
/*
 * Refer to Documentation/timers/timers-howto.txt
 * Non-atomic context
 *		< 10us	:	udelay()
 *      10us ~ 20ms     :       usleep_range()
 *              > 10ms  :       msleep() or msleep_interruptible()
 */

	if (u32Us < 10UL)
		udelay(u32Us);
	else if (u32Us < 20UL * 1000UL)
		usleep_range(u32Us, u32Us + 1);
	else
		msleep_interruptible((unsigned int)(u32Us / 1000UL));
}

static void audio_loopback_delaytask(unsigned int u32Ms)
{
	audio_loopback_delaytask_us(u32Ms * 1000UL);
}
//------------------------------------------------------------------------------
// Thread Function
//------------------------------------------------------------------------------
static int audio_loopback_pb_trigger(void *arg)
{
	struct mtk_runtime_struct *dev_runtime = (struct mtk_runtime_struct *)arg;
	struct snd_pcm_substream *substream = NULL;

	if (dev_runtime == NULL) {
		audio_err("[%s]Invalid loopback runtime\n", __func__);
		return -EINVAL;
	}

	for (;;) {
		wait_event_interruptible(dev_runtime->kthread_waitq,
				kthread_should_stop() || dev_runtime->timer_notify);

		if (kthread_should_stop() || dev_runtime->timer_notify == TIMER_STOP_EVENT) {
			audio_info("[%s]Receive kthread_should_stop\n", __func__);
			break;
		}
		dev_runtime->timer_notify = TIMER_WAIT_EVENT;
		mutex_lock(&dev_runtime->kthread_lock);
		substream = dev_runtime->substream;
		if (substream == NULL) {
			mutex_unlock(&dev_runtime->kthread_lock);
			audio_warn("[%s]Invalid substream\n", __func__);
			continue;
		}
		snd_pcm_period_elapsed(substream);
		mutex_unlock(&dev_runtime->kthread_lock);
	}

	return 0;
}

static int audio_loopback_trigger(void *arg)
{
	struct mtk_runtime_struct *dev_runtime = (struct mtk_runtime_struct *)arg;
	struct snd_pcm_substream *substream = NULL;

	if (dev_runtime == NULL) {
		audio_err("[%s]Invalid loopback runtime\n", __func__);
		return -EINVAL;
	}

	for (;;) {
		wait_event_interruptible(dev_runtime->kthread_waitq,
				kthread_should_stop() || dev_runtime->timer_notify);

		if (kthread_should_stop() ||  dev_runtime->timer_notify == TIMER_STOP_EVENT) {
			audio_info("[%s]Receive kthread_should_stop\n", __func__);
			break;
		}

		dev_runtime->timer_notify = TIMER_WAIT_EVENT;
		mutex_lock(&dev_runtime->kthread_lock);
		substream = dev_runtime->substream;
		if (substream == NULL) {
			mutex_unlock(&dev_runtime->kthread_lock);
			audio_warn("[%s]Invalid substream\n", __func__);
			continue;
		}
		snd_pcm_period_elapsed(substream);
		mutex_unlock(&dev_runtime->kthread_lock);
	}

	return 0;
}

//------------------------------------------------------------------------------
// Timer Function
//------------------------------------------------------------------------------
static void audio_loopback_timer_monitor(struct timer_list *t)
{
	struct mtk_runtime_struct *dev_runtime = NULL;
	struct mtk_monitor_struct *monitor = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_pcm_runtime *runtime = NULL;
	unsigned int expiration_counter = EXPIRATION_TIME;

	dev_runtime = from_timer(dev_runtime, t, timer);

	if (dev_runtime == NULL) {
		audio_err("[%s]Invalid parameters!\n", __func__);
		return;
	}

	monitor = &dev_runtime->monitor;

	if (monitor->monitor_status == CMD_STOP) {
		audio_info("[%s] No action is required, exit Monitor\n", __func__);
		spin_lock(&dev_runtime->spin_lock);
		memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
		spin_unlock(&dev_runtime->spin_lock);
		return;
	}

	substream = dev_runtime->substream;
	if (substream != NULL) {
		runtime = substream->runtime;
		if (runtime != NULL) {
			/* If nothing to do, increase "expiration_counter" */
			if ((monitor->last_appl_ptr == runtime->control->appl_ptr) &&
				(monitor->last_hw_ptr == runtime->status->hw_ptr)) {
				monitor->expiration_counter++;

				if (monitor->expiration_counter >= expiration_counter) {
					audio_warn("[%s] Expire, exit Monitor\n", __func__);
					spin_lock(&(dev_runtime->spin_lock));
					memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
					spin_unlock(&(dev_runtime->spin_lock));
					return;
				}
			} else {
				monitor->last_appl_ptr = runtime->control->appl_ptr;
				monitor->last_hw_ptr = runtime->status->hw_ptr;
				monitor->expiration_counter = 0;
			}

			ktime_get_ts(&_stSwTimeStamp);
			dev_runtime->timer_notify = TIMER_TRIGGER_EVENT;
			wake_up_interruptible(&dev_runtime->kthread_waitq);
		}
		mod_timer(&(dev_runtime->timer),
			(jiffies + msecs_to_jiffies(TIMER_UNIT) / TIMER_SHIFT));
	}
}

static void audio_loopback_pb_timer_monitor(struct timer_list *t)
{
	struct mtk_runtime_struct *dev_runtime = NULL;
	struct mtk_monitor_struct *monitor = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_pcm_runtime *runtime = NULL;
	unsigned int expiration_counter = EXPIRATION_TIME;

	dev_runtime = from_timer(dev_runtime, t, timer);

	if (dev_runtime == NULL) {
		audio_err("[%s]Invalid parameters!\n", __func__);
		return;
	}

	monitor = &dev_runtime->monitor;

	if (monitor->monitor_status == CMD_STOP) {
		audio_info("[%s] No action is required, exit Monitor\n", __func__);
		spin_lock(&dev_runtime->spin_lock);
		memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
		spin_unlock(&dev_runtime->spin_lock);
		return;
	}

	substream = dev_runtime->substream;
	if (substream != NULL) {
		runtime = substream->runtime;
		if (runtime != NULL) {
			/* If nothing to do, increase "expiration_counter" */
			if ((monitor->last_appl_ptr == runtime->control->appl_ptr) &&
				(monitor->last_hw_ptr == runtime->status->hw_ptr)) {
				monitor->expiration_counter++;

				if (monitor->expiration_counter >= expiration_counter) {
					audio_warn("[%s] Expire, exit Monitor\n", __func__);
					spin_lock(&(dev_runtime->spin_lock));
					memset(monitor, 0x00, sizeof(struct mtk_monitor_struct));
					spin_unlock(&(dev_runtime->spin_lock));
					return;
				}
			} else {
				monitor->last_appl_ptr = runtime->control->appl_ptr;
				monitor->last_hw_ptr = runtime->status->hw_ptr;
				monitor->expiration_counter = 0;
			}

			dev_runtime->timer_notify = TIMER_TRIGGER_EVENT;
			wake_up_interruptible(&dev_runtime->kthread_waitq);
		}
		mod_timer(&(dev_runtime->timer), (jiffies + msecs_to_jiffies(TIMER_UNIT)));
	}
}
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
static irqreturn_t aec_irq_handler(int irq, void *irq_priv)
{
	audio_loopback_write_reg_mask(REG.AEC_DMA_TCRL, AEC_TIMER_CLEAR, AEC_TIMER_CLEAR);
	audio_loopback_write_reg_mask(REG.AEC_DMA_TCRL, AEC_TIMER_CLEAR, ZERO);
	return IRQ_HANDLED;
}

static irqreturn_t dsp_irq_handler(int irq, void *irq_priv)
{
	struct mtk_snd_loopback *loopback = (struct mtk_snd_loopback *)irq_priv;

	if (loopback == NULL) {
		audio_err("[%s]Invalid loopback\n", __func__);
		return IRQ_HANDLED;
	}

	loopback->dsp_dma_buf.r_buf.sw_w_offset =
		audio_loopback_read_reg(REG.DSP_DMA_WPTR) * BYTES_IN_MIU_LINE;

	// update global variable _stDspTimeStamp after DSP write xxx frames data to DDR
	ktime_get_ts(&_stDspTimeStamp);

	loopback->dev_runtime.timer_notify = TIMER_TRIGGER_EVENT;
	wake_up_interruptible(&loopback->dev_runtime.kthread_waitq);

	return IRQ_HANDLED;
}

static unsigned int audio_loopback_card_read(struct mtk_snd_loopback *loopback,
			unsigned int reg)
{
	if (loopback == NULL) {
		audio_err("[%s]Invalid loopback\n", __func__);
		return -EINVAL;
	}

	audio_info("%s: reg = 0x%x, val = 0x%x\n", __func__, reg, card_reg[reg]);
	return card_reg[reg];
}

static int audio_loopback_card_write(struct mtk_snd_loopback *loopback, unsigned int reg,
				unsigned int value)
{
	int ret = 0;
	struct snd_kcontrol *kctl = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_ctl_elem_id elem_id;

	if (loopback == NULL) {
		audio_err("[%s]Invalid loopback\n", __func__);
		return -EINVAL;
	}

	switch (reg) {
	case LOOPBACK_REG_AEC_SIGEN_ONOFF: {
		audio_info("%s: aec sigen reg: x%x, val = 0x%x\n",
			__func__, reg, value);
		if (loopback->loopback_aec_bypass) {
			audio_err("%s: aec sigen not support\n",
				__func__);
		} else if (value == 1) {
			audio_loopback_write_reg(REG.AEC_DMA_SCFG, AEC_SIGEN_CFG);
			audio_loopback_write_reg_mask(
				REG.AEC_DMA_SINE, AEC_SIGEN_MASK, AEC_SIGEN_ON);
		} else {
			audio_loopback_write_reg_mask(
				REG.AEC_DMA_SINE, AEC_SIGEN_MASK, AEC_SIGEN_OFF);
		}
		break;
	}
	case LOOPBACK_REG_PCM_SLAVE_FORMAT:
	case LOOPBACK_REG_PCM_SLAVE_RATE: {
		audio_info("%s: pcm slave reg: x%x, val = 0x%x\n",
			__func__, reg, value);
		if (loopback->dsp_mixing_mode) {
			audio_err("%s: format/rate not support without playback\n",
				__func__);
			ret = -1;
			break;
		}
		break;
	}
	case LOOPBACK_REG_PCM_SLAVE_CHANNELS: {
		audio_info("%s: pcm slave reg: x%x, val = 0x%x\n",
			__func__, reg, value);
		break;
	}
	case LOOPBACK_REG_PCM_CAPTURE_SOURCE: {
		audio_info("%s: pcm capture source reg: x%x, val = 0x%x\n",
			__func__, reg, value);
		if (value < FROM_SPEAKER || value > FROM_I2S_RX_8CH) {
			ret = -1;
			break;
		}
		loopback->dsp_capture_source = value;
		if (loopback->dsp_capture_source >= FROM_I2S_RX_2CH) {
			card_reg[LOOPBACK_REG_PCM_SLAVE_FORMAT] = FORMAT_S16;
			card_reg[LOOPBACK_REG_PCM_SLAVE_RATE] = RATE_48000;
			switch (loopback->dsp_capture_source) {
			case FROM_I2S_RX_2CH:
				card_reg[LOOPBACK_REG_PCM_SLAVE_CHANNELS] = CH_2;
				break;
			case FROM_I2S_RX_4CH:
				card_reg[LOOPBACK_REG_PCM_SLAVE_CHANNELS] = CH_4;
				break;
			case FROM_I2S_RX_8CH:
				card_reg[LOOPBACK_REG_PCM_SLAVE_CHANNELS] = CH_8;
				break;
			}
		}
		substream = loopback->dev_play_runtime.substream;
		if (loopback->dsp_mixing_mode && loopback->loopback_pcm_running &&
			substream != NULL) {
			memset(&elem_id, 0, sizeof(elem_id));
			elem_id.numid = LOOPBACK_REG_PCM_SLAVE_ACTIVE_ONOFF;
			substream = loopback->dev_play_runtime.substream;
			kctl = snd_ctl_find_id(substream->pcm->card, &elem_id);
			if (kctl != NULL)
				snd_ctl_notify(loopback->card,
						SNDRV_CTL_EVENT_MASK_VALUE,
						&kctl->id);
		}
		break;
	}
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
		audio_info("%s: reg = 0x%x, val = 0x%x\n", __func__, reg, value);
		card_reg[reg] = value;
		ret = 0;
	}

	return ret;
}

static int audio_loopbck_card_get_int(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_loopback *loopback = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;

	ucontrol->value.integer.value[0] = audio_loopback_card_read(loopback, mc->reg);
	return 0;
}

static int audio_loopback_card_put_int(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_loopback *loopback = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
	    (struct soc_mixer_control *)kcontrol->private_value;

	if ((ucontrol->value.integer.value[0] < 0)
		|| (ucontrol->value.integer.value[0] > mc->max)) {
		audio_info("reg:%d range MUST is (0->%d), %ld is overflow\r\n",
		       mc->reg, mc->max, ucontrol->value.integer.value[0]);
		return -1;
	}
	return audio_loopback_card_write(loopback, mc->reg,
					ucontrol->value.integer.value[0]);
}

static int audio_loopback_card_get_enum(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_loopback *loopback = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;

	ucontrol->value.integer.value[0] = audio_loopback_card_read(loopback, e->reg);
	return 0;
}

static int audio_loopback_card_put_enum(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_loopback *loopback = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;

	if (item[0] >= e->items)
		return -EINVAL;

	return audio_loopback_card_write(loopback, e->reg,
					ucontrol->value.integer.value[0]);
}

static const struct snd_kcontrol_new audio_loopback_snd_controls[] = {
	SOC_ENUM_EXT("AEC Sigen Switch", aec_sigen_onoff_enum,
			audio_loopback_card_get_enum, audio_loopback_card_put_enum),
	SOC_ENUM_EXT("PCM Slave Active", pcm_slave_active_onoff_enum,
			audio_loopback_card_get_enum, audio_loopback_card_put_enum),
	SOC_SINGLE_EXT("PCM Slave Format", LOOPBACK_REG_PCM_SLAVE_FORMAT, 0, FORMAT_S16, 0,
			audio_loopbck_card_get_int, audio_loopback_card_put_int),
	SOC_SINGLE_EXT("PCM Slave Rate", LOOPBACK_REG_PCM_SLAVE_RATE, 0, RATE_48000, 0,
			audio_loopbck_card_get_int, audio_loopback_card_put_int),
	SOC_SINGLE_EXT("PCM Slave Channels", LOOPBACK_REG_PCM_SLAVE_CHANNELS, 0, CH_12, 0,
			audio_loopbck_card_get_int, audio_loopback_card_put_int),
	SOC_ENUM_EXT("PCM Capture Source", pcm_capture_source_enum,
			audio_loopback_card_get_enum, audio_loopback_card_put_enum)
};

static int audio_loopback_card_mixer_new(struct mtk_snd_loopback *loopback)
{
	struct snd_card *card = loopback->card;
	struct snd_kcontrol *kcontrol;
	unsigned int idx;
	int err;

	audio_info("[%s][%d]\n", __func__, __LINE__);
	strlcpy(card->mixername, "audio_loopback_snd_card", sizeof(card->mixername));
	for (idx = 0; idx < ARRAY_SIZE(audio_loopback_snd_controls); idx++) {
		kcontrol = snd_ctl_new1(&audio_loopback_snd_controls[idx], loopback);
		err = snd_ctl_add(card, kcontrol);
		if (err < 0)
			return err;
	}

	return 0;
}

static int audio_loopback_check_aec_support(struct mtk_snd_loopback *loopback,
						int sample_rate, int channels)
{
	int ret = 0;

	if (loopback == NULL)
		return 0;

	if (!loopback->loopback_aec_bypass && (sample_rate == AEC_RATE ||
		loopback->loopback_pcm_playback_running || channels <= loopback->aec_max_channel))
		ret = 1; //Can enable aec

	return ret;
}

static int audio_loopback_check_dsp_support(struct mtk_snd_loopback *loopback,
						int sample_rate, int channels)
{
	int support = 0;
	int i = 0;

	if (loopback == NULL)
		return 0;

	for (i = 0 ; i < loopback->dsp_channels_num &&
		!loopback->dsp_mixing_mode && sample_rate == RATE_48000 ; i++) {
		if (loopback->dsp_channel_list[i] == channels) {
			support = 1;
			break;
		}
	}

	return support;
}

static void audio_loopback_update_capture_dsp_offset(struct mtk_snd_loopback *loopback,
			unsigned int size)
{
	unsigned int ptr_offset = 0;

	if (loopback == NULL)
		return;

	if (!loopback->dsp_mixing_mode) {
		if (loopback->loopback_aec_enabled)
			loopback->dsp_dma_buf.r_buf.r_ptr = loopback->dsp_dma_buf.r_buf.w_ptr;

		if (loopback->dsp_dma_buf.sw_loop_status == 1) {
			loopback->dsp_dma_buf.r_buf.buffer_level -=
				(size > loopback->dsp_dma_buf.r_buf.buffer_level) ?
				loopback->dsp_dma_buf.r_buf.buffer_level : size;
		} else {
			ptr_offset = loopback->dsp_dma_buf.r_buf.r_ptr -
				loopback->dsp_dma_buf.r_buf.addr;
			audio_loopback_write_reg(REG.DSP_DMA_RPTR,
						(ptr_offset / BYTES_IN_MIU_LINE));
		}
		loopback->dsp_dma_buf.r_buf.r_ptr = loopback->dsp_dma_buf.r_buf.addr +
				((loopback->dsp_dma_buf.r_buf.r_ptr -
					loopback->dsp_dma_buf.r_buf.addr + size) %
					loopback->dsp_dma_buf.r_buf.size);
	}
	loopback->dsp_dma_buf.r_buf.total_read_size += size;
}

static unsigned int audio_loopback_get_playback_offset(struct mtk_snd_loopback *loopback)
{
	unsigned int bytes_per_millseconds = 0;
	unsigned long delta_jiffies = 0;
	unsigned int delta_bytes = 0;
	unsigned int offset = 0;
	int sw_offset_bytes = 0;

	if (loopback == NULL)
		return 0;

	if (loopback->loopback_aec_bypass) {
		//Simulate position
		delta_jiffies = jiffies - loopback->dsp_pb_dma_buf.r_buf.update_jiffies;
		bytes_per_millseconds = loopback->dsp_pb_dma_buf.sample_rate *
			loopback->dsp_pb_dma_buf.channel_mode *
			SND_AUDIO_BITS_TO_BYTES(loopback->dsp_pb_dma_buf.sample_bits) /
			MS_UNIT;
		delta_bytes = jiffies_to_msecs(delta_jiffies) * bytes_per_millseconds;
		sw_offset_bytes = (delta_bytes > loopback->dsp_pb_dma_buf.r_buf.buffer_level) ?
			loopback->dsp_pb_dma_buf.r_buf.buffer_level : delta_bytes;

		loopback->dsp_pb_dma_buf.r_buf.sw_w_offset =
			(loopback->dsp_pb_dma_buf.r_buf.sw_w_offset + sw_offset_bytes) %
			(loopback->dsp_pb_dma_buf.r_buf.size);
		//If capture not open, we should ignore data for playback
		//if (sw_buffer_level > 0 && !loopback->loopback_pcm_running)
		if (loopback->dsp_pb_dma_buf.r_buf.buffer_level > 0) {
			loopback->dsp_pb_dma_buf.r_buf.buffer_level -= sw_offset_bytes;
			if (loopback->loopback_pcm_running)
				loopback->dsp_dma_buf.r_buf.buffer_level += sw_offset_bytes;
		}
		loopback->dsp_pb_dma_buf.r_buf.update_jiffies +=
			msecs_to_jiffies(jiffies_to_msecs(delta_jiffies));
	}

	offset = (loopback->loopback_aec_bypass) ?
		loopback->dsp_pb_dma_buf.r_buf.buffer_level :
		audio_loopback_read_reg(REG.OUT_DMA_LVEL) * BYTES_IN_MIU_LINE;

	return offset;
}

static unsigned int audio_loopback_get_capture_dsp_offset(struct mtk_snd_loopback *loopback)
{
	unsigned int bytes_per_millseconds = 0;
	unsigned long delta_jiffies = 0;
	unsigned int delta_bytes = 0;
	unsigned int offset_bytes = 0;
	unsigned int offset = 0;

	audio_loopback_get_playback_offset(loopback);
	if (loopback->dsp_dma_buf.sw_loop_status == 1) {
		if (loopback->dsp_dma_buf.sw_loop_stopped == 1) {
			delta_jiffies = jiffies - loopback->dsp_dma_buf.r_buf.update_jiffies;
			bytes_per_millseconds = loopback->dsp_dma_buf.sample_rate *
				loopback->dsp_dma_buf.channel_mode *
				SND_AUDIO_BITS_TO_BYTES(loopback->dsp_dma_buf.sample_bits) /
				MS_UNIT;
			delta_bytes = jiffies_to_msecs(delta_jiffies) * bytes_per_millseconds;
			loopback->dsp_dma_buf.r_buf.update_jiffies +=
				msecs_to_jiffies(jiffies_to_msecs(delta_jiffies));

			offset_bytes = delta_bytes;
			offset = ((loopback->dsp_dma_buf.r_buf.w_ptr -
					loopback->dsp_dma_buf.r_buf.addr + offset_bytes) %
					loopback->dsp_dma_buf.r_buf.size);
		} else {
			offset = loopback->dsp_pb_dma_buf.r_buf.sw_w_offset;
			loopback->dsp_dma_buf.r_buf.update_jiffies = jiffies;
		}
		loopback->pcm_capture_ts.tv_sec = _stSwTimeStamp.tv_sec;
		loopback->pcm_capture_ts.tv_nsec = _stSwTimeStamp.tv_nsec;
	} else {
		//offset = audio_loopback_read_reg(REG.DSP_DMA_WPTR) * BYTES_IN_MIU_LINE;
		offset = loopback->dsp_dma_buf.r_buf.sw_w_offset;
		loopback->dsp_dma_buf.r_buf.update_jiffies = jiffies;
		loopback->pcm_capture_ts.tv_sec = _stDspTimeStamp.tv_sec;
		loopback->pcm_capture_ts.tv_nsec = _stDspTimeStamp.tv_nsec;
	}

	return offset;
}

static void audio_loopback_get_avail_bytes
				(struct mtk_snd_loopback *loopback)
{
	int avail_bytes = 0;

	//Update AEC
	avail_bytes = loopback->aec_dma_buf.r_buf.w_ptr - loopback->aec_dma_buf.r_buf.r_ptr;

	if (avail_bytes < 0)
		avail_bytes += loopback->aec_dma_buf.r_buf.size;

	loopback->aec_dma_buf.c_buf.avail_size = avail_bytes;

	//Update DSP
	avail_bytes = loopback->dsp_dma_buf.r_buf.w_ptr - loopback->dsp_dma_buf.r_buf.r_ptr;

	if (avail_bytes < 0)
		avail_bytes += loopback->dsp_dma_buf.r_buf.size;

	loopback->dsp_dma_buf.c_buf.avail_size = avail_bytes;

	//Update Playback
	if (loopback->loopback_pcm_playback_running) {
		avail_bytes = ((loopback->dsp_pb_dma_buf.r_buf.size / BYTES_IN_MIU_LINE) -
				audio_loopback_read_reg(REG.OUT_DMA_LVEL)) * BYTES_IN_MIU_LINE;

		if (avail_bytes < 0)
			avail_bytes = 0;

		loopback->dsp_pb_dma_buf.c_buf.avail_size = avail_bytes;
	}
}

static int audio_loopback_get_copy_buffer(
	struct mtk_snd_loopback *loopback, int is_playback, int is_nonblock,
	unsigned int request_size, unsigned char **buffer, unsigned int *size)
{
	if (loopback == NULL || buffer == NULL || size == NULL) {
		audio_err("[%s]Err! Invalid buffer/size\n", __func__);
		return -EINVAL;
	}

	if (is_playback) {
		if ((loopback->dsp_pb_dma_buf.status == CMD_STOP) ||
			(loopback->dsp_pb_dma_buf.status == CMD_SUSPEND)) {
			audio_err("[%s]Err! playback status not ready\n", __func__);
			return -EAGAIN;
		}
		(*buffer) = loopback->dsp_pb_dma_buf.c_buf.addr;
		(*size) = loopback->dsp_pb_dma_buf.c_buf.avail_size;
	} else if (loopback->loopback_aec_enabled) {
		if ((loopback->aec_dma_buf.status == CMD_STOP) ||
			(loopback->aec_dma_buf.status == CMD_SUSPEND)) {
			audio_err("[%s]Err! capture status not ready\n", __func__);
			return -EAGAIN;
		}
		(*buffer) = loopback->aec_dma_buf.c_buf.addr;
		(*size) = loopback->aec_dma_buf.c_buf.avail_size;
	} else {
		if ((loopback->dsp_dma_buf.status == CMD_STOP) ||
			(loopback->dsp_dma_buf.status == CMD_SUSPEND)) {
			audio_err("[%s]Err! capture status not ready\n", __func__);
			return -EAGAIN;
		}
		(*buffer) = loopback->dsp_dma_buf.c_buf.addr;
		(*size) = loopback->dsp_dma_buf.c_buf.avail_size;
	}

	if ((*buffer) == NULL) {
		audio_err("[%s]Err! need to allocate copy buffer\n", __func__);
		return -ENXIO;
	}

	if ((is_nonblock > 0) && (request_size > (*size))) {
		audio_err("[%s]Err! request_size = %u but avail_size = %u!!!\n",
		__func__, request_size, (*size));
		return -EAGAIN;
	}

	return 0;
}

static int audio_loopback_dma_set(struct mtk_dma_struct *dma_buf,
	struct mem_info mem_info, uint32_t dma_offset, uint32_t dma_size)
{
	ptrdiff_t dma_base_pa = 0;
	ptrdiff_t dma_base_ba = 0;
	ptrdiff_t dma_base_va = 0;

	if (dma_buf == NULL)
		return -EINVAL;

	if ((dma_offset == 0) || (dma_size == 0))
		return -ENXIO;

	if ((mem_info.bus_addr == 0) || (mem_info.memory_bus_base == 0))
		return -ENXIO;

	dma_buf->r_buf.size = dma_size;
	dma_buf->high_threshold = dma_size - (dma_size >> THRESHOLD_SHIFT);
	dma_base_va = dma_buf->str_mode_info.virtual_addr;

	if ((dma_buf->initialized_status != 1) || (dma_buf->status != CMD_RESUME)) {
		dma_base_pa = mem_info.bus_addr - mem_info.memory_bus_base + dma_offset;
		dma_base_ba = mem_info.bus_addr + dma_offset;

		if (dma_base_ba % ALIGN_SIZE)
			return -EFAULT;

		/* convert Bus Address to Virtual Address */
		if (dma_base_va == 0) {
			dma_base_va = (ptrdiff_t)ioremap_wc(dma_base_ba,
					dma_buf->r_buf.size);
			if (dma_base_va == 0)
				return -ENOMEM;
		}
		dma_buf->str_mode_info.physical_addr = dma_base_pa;
		dma_buf->str_mode_info.bus_addr = dma_base_ba;
		dma_buf->str_mode_info.virtual_addr = dma_base_va;

		dma_buf->initialized_status = 1;
	} else {
		dma_base_pa = dma_buf->str_mode_info.physical_addr;
		dma_base_ba = dma_buf->str_mode_info.bus_addr;
		dma_base_va = dma_buf->str_mode_info.virtual_addr;
	}

	dma_buf->r_buf.addr = (unsigned char *)dma_base_va;
	dma_buf->r_buf.r_ptr = dma_buf->r_buf.addr;
	dma_buf->r_buf.w_ptr = dma_buf->r_buf.addr;
	dma_buf->r_buf.buf_end = dma_buf->r_buf.addr + dma_size;
	dma_buf->r_buf.sw_w_offset = 0;
	dma_buf->r_buf.total_read_size = 0;
	dma_buf->r_buf.total_write_size = 0;
	dma_buf->r_buf.buffer_level = 0;
	dma_buf->copy_size = 0;

	memset((void *)dma_buf->r_buf.addr, 0x00, dma_buf->r_buf.size);

	//Wwait dma sync after dma_set
	smp_mb();

	return 0;
}

static int audio_loopback_pb_dma_init(struct mtk_snd_loopback *loopback)
{
	int dsp_channel = DSP_CHANNEL_2;
	int ret = 0;

	if (loopback == NULL)
		return -EINVAL;

	ret = audio_loopback_dma_set(&loopback->dsp_pb_dma_buf,
					loopback->dsp_pb_mem_info,
					loopback->dsp_pb_dma_offset,
					loopback->dsp_pb_dma_size);

	if (ret < 0) {
		audio_err("[%s]PLAYBACK dma set failed(%d)\n", __func__, ret);
		return -EFAULT;
	}

	if (loopback->loopback_aec_enabled) {
		/* reset PCM playback engine */
		audio_loopback_write_reg(REG.OUT_DMA_CTRL, BIT0);

		/* clear PCM playback write pointer */
		audio_loopback_write_reg(REG.OUT_DMA_WPTR, ZERO);

		audio_loopback_write_reg_mask_byte(REG.OUT_DMA_CTRL,
			DSP_FORMAT_MASK, DSP_FORMAT_16BIT);  // 16bit mode

		if (loopback->dsp_pb_dma_buf.channel_mode == CH_12) {
			/* DSP decoder5 dma output 12ch */
			dsp_channel = DSP_CHANNEL_12;
		} else if (loopback->dsp_pb_dma_buf.channel_mode == CH_8) {
			/* DSP decoder5 dma output 8ch */
			dsp_channel = DSP_CHANNEL_8;
		}

		audio_loopback_write_reg_mask_byte(REG.OUT_DMA_CTRL,
			DSP_CHANNEL_MASK, dsp_channel);
	}

	// Timer
	timer_setup(
		&(loopback->dev_play_runtime.timer),
		audio_loopback_pb_timer_monitor,
		0);

	loopback->dev_play_runtime.timer.expires = 0;

	loopback->dsp_pb_dma_buf.c_buf.u32RemainSize = 0;

	return 0;
}

static int audio_loopback_dma_init(struct mtk_snd_loopback *loopback)
{
	int aec_format = FORMAT_S16;
	int aec_rate = 0;
	int aec_channel = 0;
	int dsp_channel = DSP_CHANNEL_2;
	int ret = 0;

	if (loopback == NULL)
		return -EINVAL;

	if (!loopback->loopback_aec_bypass) {
		ret = audio_loopback_dma_set(&loopback->aec_dma_buf,
				loopback->aec_mem_info,
				loopback->aec_dma_offset,
				loopback->aec_dma_size);
	}

	if (!loopback->dsp_mixing_mode) {
		if (loopback->loopback_aec_bypass &&
			loopback->loopback_pcm_playback_running) {
			mutex_lock(&loopback->loopback_pcm_lock);
			loopback->dsp_dma_buf.r_buf.addr = loopback->dsp_pb_dma_buf.r_buf.addr;
			loopback->dsp_dma_buf.r_buf.size = loopback->dsp_pb_dma_buf.r_buf.size;
			loopback->dsp_dma_buf.r_buf.r_ptr = loopback->dsp_dma_buf.r_buf.addr +
					loopback->dsp_pb_dma_buf.r_buf.sw_w_offset;
			loopback->dsp_dma_buf.r_buf.w_ptr = loopback->dsp_dma_buf.r_buf.addr +
					loopback->dsp_pb_dma_buf.r_buf.sw_w_offset;
			loopback->dsp_dma_buf.r_buf.buf_end =
					loopback->dsp_pb_dma_buf.r_buf.buf_end;
			loopback->dsp_dma_buf.r_buf.total_read_size = 0;
			loopback->dsp_dma_buf.r_buf.total_write_size = 0;
			loopback->dsp_dma_buf.copy_size = 0;
			loopback->dsp_dma_buf.sw_loop_status = 1;
			loopback->dsp_dma_buf.sw_loop_stopped = 0;
			loopback->dsp_dma_buf.r_buf.buffer_level = 0;
			loopback->dsp_dma_buf.initialized_status = 1;
			mutex_unlock(&loopback->loopback_pcm_lock);
		} else {
			ret |= audio_loopback_dma_set(&loopback->dsp_dma_buf,
							loopback->dsp_mem_info,
							loopback->dsp_dma_offset,
							loopback->dsp_dma_size);
		}
	}

	if (ret < 0) {
		audio_err("[%s]Capture dma set failed(%d)\n", __func__, ret);
		return -EFAULT;
	}

	if (!loopback->loopback_pcm_playback_running && !loopback->dsp_mixing_mode) {
		if (!loopback->loopback_aec_enabled) {
			// DSP ISR timestamp interval : 256 frames
			audio_loopback_write_reg_mask_byte(REG.DSP_DMA_CTRL + OFFSET_SHIFT,
				DSP_CTRL_MASK, BIT0);
		}

		// DSP formaet
		audio_loopback_write_reg_mask_byte(REG.DSP_DMA_CTRL,
			DSP_FORMAT_MASK, DSP_FORMAT_16BIT);

		dsp_channel = (loopback->dsp_dma_buf.channel_mode == CH_8) ?
			DSP_CHANNEL_8 : DSP_CHANNEL_2;
	}

	if (loopback->loopback_aec_enabled) {
		audio_loopback_write_reg_mask(REG.AEC_SRC_CTRL, BIT0, BIT0);

		dsp_channel = DSP_CHANNEL_8;
		aec_channel = (loopback->aec_dma_buf.channel_mode/CH_2) - AEC_CHANNEL_SHIFT;
		aec_rate = (loopback->aec_dma_buf.sample_rate/RATE_16000) - AEC_RATE_SHIFT;
		aec_format = SND_AUDIO_BITS_TO_BYTES(loopback->aec_dma_buf.sample_bits);

		//Timer
		audio_loopback_timer_valid_enable();
		audio_loopback_write_reg_mask(REG.AEC_DMA_TCRL,
			AEC_TIMER_ENABLE, AEC_TIMER_ENABLE);

		//If aec src and aec dma has crossprocssor, it should be set
		audio_loopback_write_reg_mask_byte(REG.AEC_DMA_RGEN,
			AEC_REGEN_MODE_MASK, AEC_REGEN_MODE);

		audio_loopback_write_reg_mask_byte(REG.AEC_DMA_RGEN,
			AEC_REGEN_BYPASS_MASK,
			(loopback->loopback_aec_regen_bypass) ?
			AEC_REGEN_BYPASS_ON : AEC_REGEN_BYPASS_OFF);

		//Ser address to aec dma
		loopback->aec_dma_addr =
			(void *)loopback->aec_dma_buf.str_mode_info.virtual_addr;
		loopback->aec_dma_paddr =
			(void *)loopback->aec_dma_buf.str_mode_info.physical_addr;

		audio_loopback_write_reg(REG.AEC_DMA_ARHI,
		((unsigned long)loopback->aec_dma_paddr >> AEC_HI_ADDR_SHIFT));
		audio_loopback_write_reg(REG.AEC_DMA_ARLO,
		((unsigned long)loopback->aec_dma_paddr >> AEC_ADDR_SHIFT) & AEC_ADDR_MASK);
		audio_loopback_write_reg(REG.AEC_DMA_ARZE,
		(loopback->aec_dma_size >> AEC_SIZE_SHIFT) & AEC_SIZE_MASK);

		audio_loopback_write_reg_mask(REG.AEC_SRC_MCFG,
				AEC_CHANNEL_MASK, aec_channel);
		audio_loopback_write_reg_mask_byte(REG.AEC_SRC_SCFG,
				AEC_RATE_MASK, aec_rate);
		audio_loopback_write_reg_mask(REG.AEC_SRC_SCFG, BIT4, BIT4);
		audio_loopback_write_reg(REG.AEC_DMA_RPTR, ZERO);

		audio_loopback_write_reg(REG.AEC_DMA_CTRL, AEC_CTRL_RESET);
		audio_loopback_write_reg_mask(REG.AEC_DMA_CTRL,
				AEC_BIT_MASK, (aec_format == FORMAT_S32) << AEC_BIT_SHIFT);
	}

	if (loopback->dsp_mixing_mode) {
		audio_loopback_aud_init_table();
		if (loopback->slt_test_config == 1) {
			audio_loopback_write_reg_mask(REG.DSP_DMA_CTRL, MIX_DSP_AEC_MASK,
				MIX_DSP_AEC_PATH_SEL_DBG_SINE_TONE << MIX_DSP_AEC_FROM_ADC2_EN_BIT);
		} else {
			dsp_channel = (loopback->dsp_capture_source > FROM_I2S_RX_2CH) ?
				(MIX_DSP_AEC_PATH_SEL_I2S_RX_4ch +
				(loopback->dsp_capture_source - FROM_I2S_RX_4CH)) :
				loopback->dsp_capture_source;
			audio_loopback_write_reg_mask(REG.DSP_DMA_CTRL, MIX_DSP_AEC_MASK,
				dsp_channel << MIX_DSP_AEC_FROM_ADC2_EN_BIT);
		}
	} else if (!loopback->loopback_pcm_playback_running) {
		audio_loopback_write_reg_mask_byte(REG.DSP_DMA_CTRL,
			DSP_CHANNEL_MASK, dsp_channel);

		card_reg[LOOPBACK_REG_PCM_CAPTURE_SOURCE] =
			(dsp_channel == DSP_CHANNEL_8) ? FROM_I2S_RX_8CH : FROM_I2S_RX_2CH;
	}

	// Timer
	timer_setup(
		&(loopback->dev_runtime.timer),
		audio_loopback_timer_monitor,
		0);

	loopback->dev_runtime.timer.expires = 0;

	return 0;
}

static int audio_loopback_pb_dma_start(struct mtk_snd_loopback *loopback)
{
	struct mtk_runtime_struct *dev_runtime;
	unsigned long flags;
	int monitor_status = 0;

	if (loopback == NULL)
		return -EINVAL;

	if (loopback->dsp_pb_dma_buf.status == CMD_START) {
		//Already started
		audio_info("[%s]Playback already start\n", __func__);
		return 0;
	}

	if (loopback->loopback_aec_enabled) {
		/* start PCM playback engine */
		audio_loopback_write_reg_mask(REG.OUT_DMA_CTRL, BIT1, BIT1);
		/* wait DSP dma */
		audio_loopback_delaytask_us(WAIT_50US);
		audio_loopback_write_reg_mask(REG.OUT_DMA_CTRL, BIT0, ZERO);
	}

	/* Wake up Monitor if necessary */
	dev_runtime = (struct mtk_runtime_struct *)&loopback->dev_play_runtime;

	spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
	monitor_status = dev_runtime->monitor.monitor_status;
	spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);
	if (monitor_status == CMD_STOP) {
		spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
		dev_runtime->monitor.monitor_status = CMD_START;
		spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);

		mod_timer(&(dev_runtime->timer),
			(jiffies + msecs_to_jiffies(TIMER_UNIT) / TIMER_SHIFT));

		loopback->dsp_pb_dma_buf.r_buf.update_jiffies = jiffies;
	}

	loopback->dsp_pb_dma_buf.status = CMD_START;
	card_reg[LOOPBACK_REG_PCM_SLAVE_ACTIVE_ONOFF] = 1;
	return 0;
}

static int audio_loopback_dma_start(struct mtk_snd_loopback *loopback)
{
	struct mtk_runtime_struct *dev_runtime;
	unsigned long flags;
	int monitor_status = 0;
	int timer_enable = 0;

	if (loopback == NULL)
		return -EINVAL;

	if (loopback->loopback_aec_enabled) {
		if ((loopback->aec_dma_buf.status == CMD_START) &&
			(loopback->dsp_dma_buf.status == CMD_START)) {
			//Already started
			audio_info("[%s]Already start\n", __func__);
			return 0;
		}
		audio_loopback_write_reg_mask_byte(REG.AEC_DMA_CTRL, BIT1, BIT1);
		audio_loopback_write_reg_mask_byte(REG.AEC_DMA_PAGA, BIT0, BIT0);
		audio_loopback_write_reg_mask_byte(REG.AEC_SRC_MCRL, BIT0, BIT0);
		audio_loopback_write_reg_mask(REG.AEC_SRC_SCFG, BIT15, BIT15);
		if ((audio_loopback_read_reg(REG.AEC_DMA_RCRL) & BIT0) == 0)
			audio_loopback_write_reg_mask_byte(REG.AEC_DMA_RCRL, BIT0, BIT0);
		audio_loopback_write_reg_mask(REG.AEC_SRC_CTRL, BIT0, ZERO);
	} else if (loopback->dsp_dma_buf.status == CMD_START) {
		//Already started
		audio_info("[%s]Already start\n", __func__);
		return 0;
	}

	if (loopback->dsp_mixing_mode) {
		if (loopback->dsp_capture_source >= FROM_I2S_RX_2CH &&
			loopback->slt_test_config == 0) {
			// I2S in format, (0:standard, 1:Left-justified)
			audio_loopback_write_reg_mask_byte(REG.I2S_INPUT_CFG,
				I2S_FORMAT_MASK, loopback->i2s_input_format << I2S_FORMAT_SHIFT);

			// I2S out(BCK and WS) clock bypass to I2S in
			audio_loopback_write_reg_mask_byte(REG.I2S_OUT2_CFG + I2S_CFG_SHIFT,
				I2S_CLK_MASK, loopback->i2s_out_clk << I2S_CLK_SHIFT);
		}
		audio_loopback_write_reg_mask(REG.DSP_DMA_CTRL,
			MIX_DSP_AEC_PATH_EN_BIT, MIX_DSP_AEC_PATH_EN_BIT);
	}
	else if (!loopback->loopback_pcm_playback_running)
		audio_loopback_write_reg_mask(REG.DSP_DMA_CTRL, BIT0, BIT0);

	/* Wake up Monitor if necessary */
	dev_runtime = (struct mtk_runtime_struct *)&loopback->dev_runtime;
	timer_enable = (loopback->loopback_aec_enabled || loopback->dsp_dma_buf.sw_loop_status);

	spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
	monitor_status = dev_runtime->monitor.monitor_status;
	spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);
	if (monitor_status == CMD_STOP && timer_enable) {
		spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
		dev_runtime->monitor.monitor_status = CMD_START;
		spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);

		mod_timer(&(dev_runtime->timer),
			(jiffies + msecs_to_jiffies(TIMER_UNIT) / TIMER_SHIFT));
	}

	ktime_get_ts(&_stAecTimeStamp);
	ktime_get_ts(&_stDspTimeStamp);
	loopback->aec_dma_buf.status = CMD_START;
	loopback->dsp_dma_buf.status = CMD_START;

	return 0;
}

static int audio_loopback_pb_dma_stop(struct mtk_snd_loopback *loopback)
{
	struct mtk_runtime_struct *dev_runtime;
	unsigned long flags;

	if (loopback == NULL)
		return -EINVAL;

	if (loopback->dsp_pb_dma_buf.status == CMD_STOP) {
		//Already stopped
		return 0;
	}

	dev_runtime = &loopback->dev_play_runtime;

	spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
	dev_runtime->monitor.monitor_status = CMD_STOP;
	spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);

	if (loopback->loopback_aec_enabled) {
		/* reset PCM playback engine */
		audio_loopback_write_reg_mask(REG.OUT_DMA_CTRL, (BIT0 | BIT1), BIT0);

		/* clear PCM playback write pointer */
		audio_loopback_write_reg(REG.OUT_DMA_WPTR, ZERO);

		/* Disable dma */
		audio_loopback_write_reg_mask(REG.OUT_DMA_CTRL, DSP_OUT_CTRL_MASK, ZERO);
	}

	/* reset Read & Write Pointer */
	loopback->dsp_pb_dma_buf.r_buf.r_ptr = loopback->dsp_pb_dma_buf.r_buf.addr;
	loopback->dsp_pb_dma_buf.r_buf.w_ptr = loopback->dsp_pb_dma_buf.r_buf.addr;
	loopback->dsp_pb_dma_buf.r_buf.sw_w_offset = 0;
	loopback->dsp_pb_dma_buf.r_buf.buffer_level = 0;
	/* reset written size */
	loopback->dsp_pb_dma_buf.copy_size = 0;
	loopback->dsp_pb_dma_buf.status = CMD_STOP;

	card_reg[LOOPBACK_REG_PCM_SLAVE_ACTIVE_ONOFF] = 0;
	return 0;
}

static int audio_loopback_dma_stop(struct mtk_snd_loopback *loopback)
{
	struct mtk_runtime_struct *dev_runtime;
	unsigned long flags;

	if (loopback == NULL)
		return -EINVAL;

	if (loopback->loopback_aec_enabled) {
		if ((loopback->aec_dma_buf.status == CMD_STOP) &&
			(loopback->dsp_dma_buf.status == CMD_STOP)) {
			//Already stopped
			return 0;
		}
	} else if (loopback->dsp_dma_buf.status == CMD_STOP) {
		//Already stopped
		return 0;
	}

	if (loopback->dsp_mixing_mode) {
		audio_loopback_write_reg_mask(REG.DSP_DMA_CTRL, MIX_DSP_AEC_PATH_EN_BIT, ZERO);
	} else if (!loopback->loopback_pcm_playback_running) {
		audio_loopback_write_reg_mask(REG.DSP_DMA_CTRL, BIT0, ZERO);
		audio_loopback_write_reg(REG.DSP_DMA_RPTR, DSP_DMA_RESET);
	}

	dev_runtime = &loopback->dev_runtime;

	spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
	dev_runtime->monitor.monitor_status = CMD_STOP;
	spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);

	if (loopback->loopback_aec_enabled) {
		audio_loopback_write_reg_mask(REG.AEC_DMA_TCRL, AEC_TIMER_ENABLE, ZERO);
		audio_loopback_write_reg_mask(REG.AEC_DMA_TCRL, AEC_TIMER_CLEAR, AEC_TIMER_CLEAR);
		audio_loopback_write_reg_mask(REG.AEC_DMA_TCRL, AEC_TIMER_CLEAR, ZERO);

		audio_loopback_write_reg_mask(REG.AEC_DMA_CTRL, BIT15, BIT15);
		audio_loopback_write_reg_mask(REG.AEC_SRC_CTRL, BIT0, BIT0);
		audio_loopback_write_reg_mask(REG.AEC_SRC_SCFG, BIT15, ZERO);
		audio_loopback_write_reg_mask_byte(REG.AEC_SRC_MCRL, BIT0, ZERO);
		audio_loopback_write_reg_mask_byte(REG.AEC_DMA_CTRL, BIT1, ZERO);
		audio_loopback_write_reg_mask(REG.AEC_DMA_CTRL, BIT15, ZERO);
	}

	/* reset Read & Write Poiinter for SRC */
	loopback->aec_dma_buf.r_buf.r_ptr = loopback->aec_dma_buf.r_buf.addr;
	loopback->aec_dma_buf.r_buf.w_ptr = loopback->aec_dma_buf.r_buf.addr;
	loopback->aec_dma_buf.r_buf.total_read_size = 0;
	loopback->aec_dma_buf.r_buf.total_write_size = 0;
	 /* reset written size for DSP */
	loopback->aec_dma_buf.copy_size = 0;
	loopback->aec_dma_buf.status = CMD_STOP;

	/* reset Read & Write Pointer for DSP */
	loopback->dsp_dma_buf.r_buf.r_ptr = loopback->dsp_dma_buf.r_buf.addr;
	loopback->dsp_dma_buf.r_buf.w_ptr = loopback->dsp_dma_buf.r_buf.addr;
	loopback->dsp_dma_buf.r_buf.total_read_size = 0;
	loopback->dsp_dma_buf.r_buf.total_write_size = 0;
	/* reset written size for DSP */
	loopback->dsp_dma_buf.copy_size = 0;
	loopback->dsp_dma_buf.status = CMD_STOP;

	return 0;
}

static int audio_loopback_dma_exit(struct mtk_snd_loopback *loopback)
{
	ptrdiff_t dma_base_va;

	if (loopback == NULL)
		return -EINVAL;

	//For SRC
	dma_base_va = loopback->aec_dma_buf.str_mode_info.virtual_addr;
	if (dma_base_va != 0) {
		if (loopback->aec_dma_buf.r_buf.addr) {
			iounmap((void *)loopback->aec_dma_buf.r_buf.addr);
			loopback->aec_dma_buf.r_buf.addr = 0;
			loopback->aec_dma_buf.r_buf.buf_end = 0;
		} else {
			audio_warn("[%s]buffer address should not be 0 !\n", __func__);
		}
		loopback->aec_dma_buf.str_mode_info.virtual_addr = 0;
	}
	loopback->aec_dma_buf.sample_bits = 0;
	loopback->aec_dma_buf.sample_rate = 0;
	loopback->aec_dma_buf.channel_mode = 0;
	loopback->aec_dma_buf.status = CMD_STOP;
	loopback->aec_dma_buf.initialized_status = 0;

	//For DSP
	dma_base_va = loopback->dsp_dma_buf.str_mode_info.virtual_addr;
	if (dma_base_va != 0) {
		if (loopback->dsp_dma_buf.r_buf.addr) {
			iounmap((void *)loopback->dsp_dma_buf.r_buf.addr);
			loopback->dsp_dma_buf.r_buf.addr = 0;
			loopback->dsp_dma_buf.r_buf.buf_end = 0;
		} else {
			audio_warn("[%s]buffer address should not be 0 !\n", __func__);
		}
		loopback->dsp_dma_buf.str_mode_info.virtual_addr = 0;
	}
	loopback->dsp_dma_buf.sample_bits = 0;
	loopback->dsp_dma_buf.sample_rate = 0;
	loopback->dsp_dma_buf.channel_mode = 0;
	loopback->dsp_dma_buf.status = CMD_STOP;
	loopback->dsp_dma_buf.sw_loop_status = 0;
	loopback->dsp_dma_buf.sw_loop_stopped = 0;
	loopback->dsp_dma_buf.initialized_status = 0;

	return 0;
}

static int audio_loopback_pb_dma_exit(struct mtk_snd_loopback *loopback)
{
	ptrdiff_t dma_base_va;

	if (loopback == NULL)
		return -EINVAL;

	//For SRC
	dma_base_va = loopback->dsp_pb_dma_buf.str_mode_info.virtual_addr;
	if (dma_base_va != 0) {
		if (loopback->dsp_pb_dma_buf.r_buf.addr) {
			iounmap((void *)loopback->dsp_pb_dma_buf.r_buf.addr);
			loopback->dsp_pb_dma_buf.r_buf.addr = 0;
			loopback->dsp_pb_dma_buf.r_buf.buf_end = 0;
		} else {
			audio_warn("[%s]buffer address should not be 0 !\n", __func__);
		}
		loopback->dsp_pb_dma_buf.str_mode_info.virtual_addr = 0;
	}
	loopback->dsp_pb_dma_buf.sample_bits = 0;
	loopback->dsp_pb_dma_buf.sample_rate = 0;
	loopback->dsp_pb_dma_buf.channel_mode = 0;
	loopback->dsp_pb_dma_buf.status = CMD_STOP;
	loopback->dsp_pb_dma_buf.initialized_status = 0;

	return 0;
}

static int audio_loopback_dma_write_check(struct mtk_snd_loopback *loopback,
		unsigned int bytes, unsigned int *inused_bytes)
{
	unsigned char *xrun_ptr = NULL;
	unsigned int xrun_buffer_level = 0;
	unsigned int remain_size = 0;

	if (loopback == NULL || inused_bytes == NULL) {
		audio_warn("[%s]parameter is null\n", __func__);
		return -EFAULT;
	}

	if (loopback->loopback_aec_bypass) {
		(*inused_bytes) = loopback->dsp_pb_dma_buf.r_buf.buffer_level;
		if (loopback->loopback_pcm_running) {
			xrun_buffer_level = loopback->dsp_dma_buf.r_buf.buffer_level;
			xrun_ptr = loopback->dsp_dma_buf.r_buf.r_ptr;
		}
	} else
		(*inused_bytes) = audio_loopback_read_reg(REG.OUT_DMA_LVEL) * BYTES_IN_MIU_LINE;

	if ((xrun_buffer_level == 0) && ((*inused_bytes) == 0) &&
			(loopback->dsp_pb_dma_buf.status == CMD_START)) {
		audio_debug("[%s]PCM Playback Buffer empty !!\n", __func__);
	} else if (((*inused_bytes) + bytes) > loopback->dsp_pb_dma_buf.high_threshold) {
		audio_debug("[%s]PCM Playback near Buffer threshold(%u, %u, %u) !!\n", __func__,
			(*inused_bytes), bytes, loopback->dsp_pb_dma_buf.high_threshold);
		return -EFAULT;
	} else if (xrun_ptr != NULL) {
		if (xrun_ptr > loopback->dsp_pb_dma_buf.r_buf.w_ptr) {
			remain_size = (unsigned int)(xrun_ptr -
							loopback->dsp_pb_dma_buf.r_buf.w_ptr);
		} else {
			remain_size = (unsigned int)(loopback->dsp_pb_dma_buf.r_buf.buf_end -
							loopback->dsp_pb_dma_buf.r_buf.w_ptr);
			remain_size += (unsigned int)(xrun_ptr -
							loopback->dsp_dma_buf.r_buf.addr);
		}
		if (remain_size < bytes) {
			audio_debug("[%s]PCM Playback Xrun !!\n", __func__);
			return -EFAULT;
		}
	}

	return 0;
}

static unsigned int audio_loopback_dma_write(struct mtk_snd_loopback *loopback,
				void *buffer, unsigned int bytes)
{
	unsigned int copy_size = 0;
	unsigned int copy_tmp = 0;
	unsigned int w_ptr_offset = 0;
	unsigned int inused_bytes = 0;

	if (audio_loopback_dma_write_check(loopback, bytes, &inused_bytes) < 0)
		return 0;

	copy_size = bytes;
	if ((inused_bytes + copy_size + loopback->dsp_pb_dma_buf.c_buf.u32RemainSize) <=
		loopback->dsp_pb_dma_buf.r_buf.size) {
		do {
			copy_tmp = loopback->dsp_pb_dma_buf.r_buf.buf_end -
					loopback->dsp_pb_dma_buf.r_buf.w_ptr;

			copy_tmp = (copy_tmp > copy_size) ? copy_size : copy_tmp;

			memcpy(loopback->dsp_pb_dma_buf.r_buf.w_ptr, buffer, copy_tmp);
			loopback->dsp_pb_dma_buf.r_buf.w_ptr += copy_tmp;
			buffer += copy_tmp;

			if (loopback->dsp_pb_dma_buf.r_buf.w_ptr ==
					loopback->dsp_pb_dma_buf.r_buf.buf_end)
				loopback->dsp_pb_dma_buf.r_buf.w_ptr =
					loopback->dsp_pb_dma_buf.r_buf.addr;

			copy_size -= copy_tmp;
		} while (copy_size > 0);
	}

	/* update PCM playback write pointer */
	w_ptr_offset = loopback->dsp_pb_dma_buf.r_buf.w_ptr -
				loopback->dsp_pb_dma_buf.r_buf.addr;
	w_ptr_offset += loopback->dsp_pb_dma_buf.c_buf.u32RemainSize;
	loopback->dsp_pb_dma_buf.c_buf.u32RemainSize = w_ptr_offset % BYTES_IN_MIU_LINE;
	SND_AUDIO_DO_ALIGNMENT(w_ptr_offset, BYTES_IN_MIU_LINE);

	if (loopback->loopback_aec_enabled)
		audio_loopback_write_reg_mask(REG.OUT_DMA_WPTR, DSP_OUT_ADDR_MASK,
					(w_ptr_offset / BYTES_IN_MIU_LINE));

	mutex_lock(&loopback->loopback_pcm_lock);
	loopback->dsp_pb_dma_buf.r_buf.buffer_level += bytes;
	loopback->dsp_pb_dma_buf.copy_size += bytes;
	mutex_unlock(&loopback->loopback_pcm_lock);
	loopback->dsp_pb_dma_buf.status = CMD_START;

	/* flush MIU */
	smp_mb();

	/* ensure write pointer can be applied */
	audio_loopback_delaytask(WAIT_1MS);

	return bytes;
}

static unsigned int audio_loopback_dma_read(struct mtk_snd_loopback *loopback,
				void *buffer, unsigned int bytes)
{
	unsigned int rest_size_to_end = 0;
	unsigned int tmp_size = 0;
	unsigned int tmp_bytes = 0;
	int sample_bytes = 0;
	int orig_channel = 0;
	int orig_channel_shift = 0;
	int dst_channel = 0;
	int dst_channel_shift = 0;
	int channel_shift = 0;
	int channel_move = 0;
	int channel_end = 0;
	int idx = 0;
	int i = 0;

	if (loopback == NULL)
		return -EINVAL;

	if (loopback->loopback_aec_enabled) {
		sample_bytes = SND_AUDIO_BITS_TO_BYTES(loopback->aec_dma_buf.sample_bits);
		channel_end = tmp_size / sample_bytes;
		orig_channel = loopback->aec_dma_buf.channel_mode + AEC_CHANNEL_SHIFT;
		orig_channel_shift = orig_channel;
		dst_channel = loopback->aec_dma_buf.channel_mode;
		dst_channel_shift = dst_channel;
		channel_move = dst_channel;

		channel_shift = loopback->aec_dma_buf.channel_mode % CH_2;
		rest_size_to_end = loopback->aec_dma_buf.r_buf.buf_end
					- loopback->aec_dma_buf.r_buf.r_ptr;
		if (channel_shift != 0) {
			bytes = (bytes * orig_channel) / dst_channel;
		}
	} else {
		rest_size_to_end = loopback->dsp_dma_buf.r_buf.buf_end
					- loopback->dsp_dma_buf.r_buf.r_ptr;
	}

	tmp_bytes = bytes;
	if (loopback->dsp_dma_buf.sw_loop_status == 0)
		tmp_bytes = (bytes >> BYTE_ALIGN_SIZE) << BYTE_ALIGN_SIZE;
	tmp_size = (rest_size_to_end > tmp_bytes) ?
				tmp_bytes : rest_size_to_end;

	if (tmp_size == 0)
		return 0;

	if (loopback->loopback_aec_enabled)
		memcpy(buffer, loopback->aec_dma_buf.r_buf.r_ptr, tmp_size);
	else if (loopback->dsp_dma_buf.sw_loop_status == 1 &&
			loopback->dsp_dma_buf.sw_loop_stopped == 1)
		memset(buffer, 0, tmp_size);
	else
		memcpy(buffer, loopback->dsp_dma_buf.r_buf.r_ptr, tmp_size);

	//wait dma sync buffer
	smp_mb();

	mutex_lock(&loopback->loopback_pcm_lock);
	if (loopback->loopback_aec_enabled) {
		loopback->aec_dma_buf.r_buf.r_ptr = loopback->aec_dma_buf.r_buf.addr +
				((loopback->aec_dma_buf.r_buf.r_ptr -
					loopback->aec_dma_buf.r_buf.addr + tmp_size) %
					loopback->aec_dma_buf.r_buf.size);

		audio_loopback_write_reg(REG.AEC_DMA_RPTR, (tmp_size / BYTES_IN_MIU_LINE));

		audio_loopback_write_reg_mask_byte(REG.AEC_DMA_CTRL, BIT4, BIT4);
		audio_loopback_write_reg_mask_byte(REG.AEC_DMA_CTRL, BIT4, ZERO);

		if (channel_shift != 0) {
			for (i = orig_channel, idx = dst_channel; i < channel_end;
			i += orig_channel_shift, idx += dst_channel_shift) {
				memmove(buffer + idx * sample_bytes, buffer + i * sample_bytes,
					channel_move * sample_bytes);
			}

			tmp_size = (tmp_size * dst_channel) / orig_channel;
		}
		loopback->aec_dma_buf.r_buf.total_read_size += tmp_size;
	}

	audio_loopback_update_capture_dsp_offset(loopback, tmp_size);
	mutex_unlock(&loopback->loopback_pcm_lock);
	return tmp_size;
}

static int audio_loopback_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_loopback *loopback;
	struct snd_pcm *pcm;
	int playback = 0;
	int ret = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	loopback = (struct mtk_snd_loopback *)substream->private_data;
	runtime = (struct snd_pcm_runtime *)substream->runtime;
	if (loopback == NULL || runtime == NULL) {
		audio_err("[%s]Invalid loopback or runtime\n", __func__);
		return -EINVAL;
	}

	pcm = loopback->pcm;
	if (pcm == NULL) {
		audio_err("[%s]Invalid pcm\n", __func__);
		return -EINVAL;
	}

	/* Set specified information */
	playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	if (playback == 1) {
		memcpy(&runtime->hw, &audio_loopback_pcm_pb_hardware,
					sizeof(struct snd_pcm_hardware));

		/* Allocate system memory for Specific Copy */
		loopback->dsp_pb_dma_buf.c_buf.addr = vmalloc(MAX_BUFFER_SIZE);
		if (loopback->dsp_pb_dma_buf.c_buf.addr == NULL)
			return -ENOMEM;

		memset((void *)loopback->dsp_pb_dma_buf.c_buf.addr, 0x00, MAX_BUFFER_SIZE);
		loopback->dsp_pb_dma_buf.c_buf.size = MAX_BUFFER_SIZE;

		loopback->dev_play_runtime.substream = substream;
		loopback->dev_play_runtime.timer_notify = TIMER_WAIT_EVENT;

		mutex_lock(&loopback->loopback_pcm_lock);

		loopback->loopback_pcm_playback_running = 1;
	} else {
		memcpy(&runtime->hw, &audio_loopback_pcm_hardware,
					sizeof(struct snd_pcm_hardware));

		if (!loopback->loopback_pcm_playback_running)
			runtime->hw.channels_max = loopback->dsp_max_channel;

		/* Allocate system memory for Specific Copy */
		loopback->capture_buf.addr = vmalloc(MAX_BUFFER_SIZE);
		if (loopback->capture_buf.addr == NULL)
			return -ENOMEM;

		memset((void *)loopback->capture_buf.addr, 0x00, MAX_BUFFER_SIZE);

		loopback->capture_buf.size = MAX_BUFFER_SIZE;
		loopback->dev_runtime.substream = substream;
		loopback->dev_runtime.timer_notify = TIMER_WAIT_EVENT;

		mutex_lock(&loopback->loopback_pcm_lock);
		//From AEC
		if (!loopback->loopback_aec_bypass) {
			// AEC timer interrupt register
			ret = devm_request_irq(loopback->dev,
				loopback->aec_dma_irq, aec_irq_handler,
				IRQF_SHARED, SND_AUDIO_LOOPBACK_IRQ_NAME, loopback);
			if (ret < 0)
				audio_err("[%s]fail to request aec int irq\n", __func__);
		}

		//From DSP
		if (!loopback->loopback_pcm_playback_running) {
			if (!loopback->dsp_mixing_mode) {
				// MB112E10 timer interrupt register
				ret = devm_request_irq(loopback->dev,
					loopback->dsp_dma_irq, dsp_irq_handler,
					IRQF_SHARED, SND_AUDIO_LOOPBACK_IRQ_NAME, loopback);
				if (ret < 0)
					audio_err("[%s]fail to request dsp int irq\n", __func__);
			} else
				audio_loopback_clk_sw_enable();

			// I2S in format, (0:standard, 1:Left-justified)
			audio_loopback_write_reg_mask_byte(REG.I2S_INPUT_CFG,
				I2S_FORMAT_MASK, loopback->i2s_input_format << I2S_FORMAT_SHIFT);

			// I2S out(BCK and WS) clock bypass to I2S in
			audio_loopback_write_reg_mask_byte(REG.I2S_OUT2_CFG + I2S_CFG_SHIFT,
				I2S_CLK_MASK, loopback->i2s_out_clk << I2S_CLK_SHIFT);

			loopback->loopback_i2s_enabled = 1;
		}

		loopback->loopback_aec_enabled = 0;
		loopback->loopback_pcm_running = 1;
	}

	mutex_unlock(&loopback->loopback_pcm_lock);

	audio_info("%s: stream = %s, pcmC%dD%d (substream = %s)\n",
				__func__,
				(substream->stream ==
				SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
				pcm->card->number,
				pcm->device, substream->name
				);

	return ret;
}

static int audio_loopback_pcm_close(struct snd_pcm_substream *substream)
{
	struct mtk_snd_loopback *loopback;
	struct mtk_runtime_struct *dev_runtime;
	unsigned long flags;
	int playback = 0;
	int ret = 0;

	audio_info("%s\n", __func__);
	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	loopback = (struct mtk_snd_loopback *)substream->private_data;
	if (loopback == NULL) {
		audio_err("[%s]Invalid loopback\n", __func__);
		return -EINVAL;
	}

	playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	dev_runtime = (playback == 1) ? (struct mtk_runtime_struct *)&loopback->dev_play_runtime :
		(struct mtk_runtime_struct *)&loopback->dev_runtime;

	if (dev_runtime->timer.function != NULL) {
		del_timer_sync(&(dev_runtime->timer));

		spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
		memset(&(dev_runtime->monitor), 0x00, sizeof(struct mtk_monitor_struct));
		spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);
	}

	mutex_lock(&dev_runtime->kthread_lock);
	dev_runtime->substream = NULL;
	mutex_unlock(&dev_runtime->kthread_lock);

	mutex_lock(&loopback->loopback_pcm_lock);

	if (playback == 1) {
		ret = audio_loopback_pb_dma_stop(loopback);
		if (ret < 0)
			audio_err("[%s]Stop dma failed:%d\n", __func__, ret);

		audio_loopback_delaytask(WAIT_2MS);

		ret = audio_loopback_pb_dma_exit(loopback);
		if (ret < 0)
			audio_err("[%s]Exit dma failed:%d\n", __func__, ret);

		if (loopback->dsp_pb_dma_buf.c_buf.addr != NULL) {
			vfree(loopback->dsp_pb_dma_buf.c_buf.addr);
			loopback->dsp_pb_dma_buf.c_buf.addr = NULL;
		}

		// Init default setting (i2s)
		card_reg[LOOPBACK_REG_PCM_SLAVE_FORMAT] = FORMAT_S16;
		card_reg[LOOPBACK_REG_PCM_SLAVE_RATE] = RATE_48000;
		card_reg[LOOPBACK_REG_PCM_SLAVE_CHANNELS] = loopback->i2s_input_channel;

		loopback->loopback_pcm_playback_running = 0;
	} else {
		ret = audio_loopback_dma_stop(loopback);
		if (ret < 0)
			audio_err("[%s]Stop dma failed:%d\n", __func__, ret);

		ret = audio_loopback_dma_exit(loopback);
		if (ret < 0)
			audio_err("[%s]Exit dma failed:%d\n", __func__, ret);

		if (loopback->capture_buf.addr != NULL) {
			vfree(loopback->capture_buf.addr);
			loopback->capture_buf.addr = NULL;
			loopback->aec_dma_buf.c_buf.addr = NULL;
			loopback->dsp_dma_buf.c_buf.addr = NULL;
		}

		if (!loopback->loopback_aec_bypass)
			devm_free_irq(loopback->dev, loopback->aec_dma_irq, loopback);

		if (loopback->loopback_i2s_enabled) {
			if (!loopback->dsp_mixing_mode)
				devm_free_irq(loopback->dev, loopback->dsp_dma_irq, loopback);

			// I2S in format, (0:standard, 1:Left-justified)
			audio_loopback_write_reg_mask_byte(REG.I2S_INPUT_CFG,
				I2S_FORMAT_MASK,
				(loopback->i2s_input_format^BIT0) << I2S_FORMAT_SHIFT);

			// I2S out(BCK and WS) clock bypass to I2S in
			audio_loopback_write_reg_mask_byte(REG.I2S_OUT2_CFG + I2S_CFG_SHIFT,
				I2S_CLK_MASK, ZERO);

			loopback->loopback_i2s_enabled = 0;
		}

		loopback->loopback_pcm_running = 0;
	}

	mutex_unlock(&loopback->loopback_pcm_lock);

	audio_debug("%s: %s close done\n", __func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));
	return ret;
}

static int audio_loopback_pcm_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct mtk_snd_loopback *loopback;
	int dsp_support = 0;
	int ret = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	loopback = (struct mtk_snd_loopback *)substream->private_data;
	if (loopback == NULL) {
		audio_err("[%s]Invalid loopback\n", __func__);
		return -EINVAL;
	}

	audio_debug("[%s]%s substream\n", __func__,
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "PLAYBACK" : "CAPTURE");
	audio_debug("[%s]params_channels     = %d\n", __func__, params_channels(params));
	audio_debug("[%s]params_rate         = %d\n", __func__, params_rate(params));
	audio_debug("[%s]params_period_size  = %d\n", __func__, params_period_size(params));
	audio_debug("[%s]params_periods      = %d\n", __func__, params_periods(params));
	audio_debug("[%s]params_buffer_size  = %d\n", __func__, params_buffer_size(params));
	audio_debug("[%s]params_buffer_bytes = %d\n", __func__, params_buffer_bytes(params));
	audio_debug("[%s]params_sample_width = %d\n", __func__,
			snd_pcm_format_physical_width(params_format(params)));
	audio_debug("[%s]params_access       = %d\n", __func__, params_access(params));
	audio_debug("[%s]params_format       = %d\n", __func__, params_format(params));
	audio_debug("[%s]params_subformat    = %d\n", __func__, params_subformat(params));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		card_reg[LOOPBACK_REG_PCM_SLAVE_FORMAT] = params_format(params);
		card_reg[LOOPBACK_REG_PCM_SLAVE_RATE] = params_rate(params);
		card_reg[LOOPBACK_REG_PCM_SLAVE_CHANNELS] = params_channels(params);

		loopback->dsp_pb_dma_buf.sample_rate = params_rate(params);
		loopback->dsp_pb_dma_buf.sample_bits =
					snd_pcm_format_physical_width(params_format(params));
		loopback->dsp_pb_dma_buf.channel_mode = params_channels(params);

		//playback dma init & start
		ret = audio_loopback_pb_dma_init(loopback);
		if (ret < 0) {
			audio_err("[%s]hw params dma init failed(%d)\n", __func__, ret);
			ret = -EFAULT;
		}
	} else {
		//Check SRC first
		loopback->loopback_aec_enabled = audio_loopback_check_aec_support(loopback,
					params_rate(params), params_channels(params));

		//Check whether DSP can capture
		dsp_support = audio_loopback_check_dsp_support(loopback,
					params_rate(params), params_channels(params));

		if (loopback->loopback_aec_enabled) {
			if (!loopback->loopback_pcm_playback_running && dsp_support)
				loopback->loopback_aec_enabled = 0;
		} else if (loopback->loopback_pcm_playback_running) {
			if (params_channels(params) != card_reg[LOOPBACK_REG_PCM_SLAVE_CHANNELS] ||
				params_rate(params) != card_reg[LOOPBACK_REG_PCM_SLAVE_RATE] ||
				params_format(params) != card_reg[LOOPBACK_REG_PCM_SLAVE_FORMAT]) {
				audio_warn("[%s]channel should match in sw loopback\n", __func__);
				ret = -EINVAL;
			}
		} else if (!dsp_support)
			//For this case, if it is not dsp can accept, it should be return error
			ret = -EINVAL;
	}

	return ret;
}

static int audio_loopback_pcm_hw_free(struct snd_pcm_substream *substream)
{
	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	audio_info("%s: stream = %s\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	return 0;
}

static int audio_loopback_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct mtk_snd_loopback *loopback;
	struct snd_pcm_runtime *runtime;
	int ret = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	loopback = (struct mtk_snd_loopback *)substream->private_data;
	runtime = (struct snd_pcm_runtime *)substream->runtime;
	if (loopback == NULL || runtime == NULL) {
		audio_err("[%s]Invalid loopback or runtime\n", __func__);
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if ((loopback->dsp_pb_dma_buf.status == CMD_START) &&
			(loopback->dsp_pb_dma_buf.initialized_status == 1)) {
			//Already start
			return ret;
		}

		ret = audio_loopback_pb_dma_start(loopback);
		if (ret < 0) {
			audio_err("[%s]Prepare playback start failed(%d)\n", __func__, ret);
			return -EFAULT;
		}
	} else {
		if ((loopback->dsp_dma_buf.status == CMD_START) &&
			(loopback->dsp_dma_buf.initialized_status == 1) &&
			((loopback->loopback_aec_bypass == 1) ||
			((loopback->aec_dma_buf.status == CMD_START) &&
			(loopback->aec_dma_buf.initialized_status == 1)))) {
			//Already init
			return ret;
		}

		//set mem info
		loopback->aec_dma_buf.c_buf.addr = loopback->capture_buf.addr;
		loopback->aec_dma_buf.c_buf.size = loopback->capture_buf.size;
		loopback->aec_dma_buf.c_buf.u32RemainSize = 0;
		loopback->aec_dma_buf.sample_rate = runtime->rate;
		loopback->aec_dma_buf.sample_bits = runtime->sample_bits;
		loopback->aec_dma_buf.channel_mode = runtime->channels;

		loopback->dsp_dma_buf.c_buf.addr = loopback->capture_buf.addr;
		loopback->dsp_dma_buf.c_buf.size = loopback->capture_buf.size;
		loopback->dsp_dma_buf.c_buf.u32RemainSize = 0;
		loopback->dsp_dma_buf.sample_rate = runtime->rate;
		loopback->dsp_dma_buf.sample_bits = runtime->sample_bits;
		loopback->dsp_dma_buf.channel_mode = runtime->channels;

		//dma init & start
		ret = audio_loopback_dma_init(loopback);
		if (ret < 0) {
			audio_err("[%s]Prepare init failed(%d)\n", __func__, ret);
			return -EFAULT;
		}

		ret = audio_loopback_dma_start(loopback);
		if (ret < 0) {
			audio_err("[%s]Prepare start failed(%d)\n", __func__, ret);
			return -EFAULT;
		}
	}

	audio_info("%s: stream = %s prepare\n", __func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	return ret;
}

static int audio_loopback_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct mtk_snd_loopback *loopback;
	struct snd_kcontrol *kctl = NULL;
	struct snd_ctl_elem_id elem_id;
	int playback = 0;
	int ret = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	loopback = (struct mtk_snd_loopback *)substream->private_data;

	audio_info("%s\n", __func__);

	mutex_lock(&loopback->loopback_pcm_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (playback == 1) {
			if (loopback->dsp_dma_buf.sw_loop_status == 1)
				loopback->dsp_dma_buf.sw_loop_stopped = 1;
			else
				ret = audio_loopback_pb_dma_stop(loopback);
		} else {
			ret = audio_loopback_dma_stop(loopback);
		}
		break;
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (playback == 1)
			ret = audio_loopback_pb_dma_start(loopback);
		else
			ret = audio_loopback_dma_start(loopback);
		break;
	default:
		audio_err("[%s]Invalid trigger cmd: %d\n", __func__, cmd);
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&loopback->loopback_pcm_lock);

	if (playback == 1) {
		memset(&elem_id, 0, sizeof(elem_id));
		elem_id.numid = LOOPBACK_REG_PCM_SLAVE_ACTIVE_ONOFF;
		kctl = snd_ctl_find_id(substream->pcm->card, &elem_id);
		if (kctl != NULL)
			snd_ctl_notify(loopback->card, SNDRV_CTL_EVENT_MASK_VALUE, &kctl->id);
	}

	return ret;
}

static snd_pcm_uframes_t audio_loopback_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct mtk_snd_loopback *loopback;
	struct snd_pcm_runtime *runtime;
	snd_pcm_uframes_t offset_pcm_samples = 0;
	snd_pcm_uframes_t new_hw_ptr = 0;
	snd_pcm_uframes_t new_hw_ptr_pos = 0;
	unsigned int new_w_ptr_offset = 0;
	unsigned char *new_w_ptr = NULL;
	int offset_bytes = 0;
	int period_size = 0;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	loopback = (struct mtk_snd_loopback *)substream->private_data;
	runtime = (struct snd_pcm_runtime *)substream->runtime;
	if (loopback == NULL || runtime == NULL) {
		audio_err("[%s]Invalid loopback or runtime\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&loopback->loopback_pcm_lock);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		new_w_ptr_offset = audio_loopback_get_playback_offset(loopback);

		if (loopback->dsp_pb_dma_buf.copy_size > new_w_ptr_offset) {
			offset_bytes = loopback->dsp_pb_dma_buf.copy_size - new_w_ptr_offset;
			loopback->dsp_pb_dma_buf.copy_size = new_w_ptr_offset;
		}
	} else {
		if (!loopback->dsp_mixing_mode) {
			new_w_ptr_offset = audio_loopback_get_capture_dsp_offset(loopback);

			new_w_ptr = loopback->dsp_dma_buf.r_buf.addr + new_w_ptr_offset;
			offset_bytes = new_w_ptr - loopback->dsp_dma_buf.r_buf.w_ptr;
			if (offset_bytes < 0)
				offset_bytes += loopback->dsp_dma_buf.r_buf.size;

			loopback->dsp_dma_buf.r_buf.w_ptr = new_w_ptr;
			loopback->dsp_dma_buf.r_buf.total_write_size += offset_bytes;
		}

		//Update AEC write position if AEC enabled
		if (loopback->loopback_aec_enabled) {
			audio_loopback_write_reg_mask_byte(REG.AEC_DMA_CTRL, BIT5, BIT5);
			new_w_ptr_offset = audio_loopback_read_reg(REG.AEC_DMA_WPTR);
			new_w_ptr_offset = (new_w_ptr_offset > FIFO_ALIGN_SIZE) ?
						new_w_ptr_offset - FIFO_ALIGN_SIZE : ZERO;
			new_w_ptr_offset = new_w_ptr_offset * BYTES_IN_MIU_LINE;
			audio_loopback_write_reg_mask_byte(REG.AEC_DMA_CTRL, BIT5, ZERO);

			//offset is data size instead of position, it will increase if not read
			//After read, it will decrease
			new_w_ptr = loopback->aec_dma_buf.r_buf.addr +
					((loopback->aec_dma_buf.r_buf.r_ptr -
					loopback->aec_dma_buf.r_buf.addr + new_w_ptr_offset) %
					loopback->aec_dma_buf.r_buf.size);

			offset_bytes = new_w_ptr - loopback->aec_dma_buf.r_buf.w_ptr;
			if (offset_bytes < 0)
				offset_bytes += loopback->aec_dma_buf.r_buf.size;

			offset_pcm_samples = bytes_to_frames(runtime, offset_bytes);
			period_size = INTERVAL_UNIT *
				(loopback->aec_dma_buf.sample_rate/RATE_16000);

			if (offset_pcm_samples < period_size) {
				mutex_unlock(&loopback->loopback_pcm_lock);
				audio_info("%s: aec no update(%u)\n", __func__,
					(unsigned int)offset_pcm_samples);
				return runtime->status->hw_ptr % runtime->buffer_size;
			}
			offset_bytes -= frames_to_bytes(runtime, offset_pcm_samples - period_size);
			new_w_ptr = loopback->aec_dma_buf.r_buf.addr +
				((loopback->aec_dma_buf.r_buf.w_ptr -
				loopback->aec_dma_buf.r_buf.addr + offset_bytes) %
				loopback->aec_dma_buf.r_buf.size);
			loopback->pcm_capture_ts.tv_sec = _stAecTimeStamp.tv_sec;
			loopback->pcm_capture_ts.tv_nsec = _stAecTimeStamp.tv_nsec;
			timespec_add_ns(&_stAecTimeStamp, PERIOD_NS_UNIT);
			loopback->aec_dma_buf.r_buf.w_ptr = new_w_ptr;
			loopback->aec_dma_buf.r_buf.total_write_size += offset_bytes;

			//If channel is odd
			if (loopback->aec_dma_buf.channel_mode % CH_2 != 0) {
				offset_bytes = offset_bytes * loopback->aec_dma_buf.channel_mode /
					(loopback->aec_dma_buf.channel_mode + AEC_CHANNEL_SHIFT);
			}
		}

		//Update avail bytes information
		audio_loopback_get_avail_bytes(loopback);
	}
	mutex_unlock(&loopback->loopback_pcm_lock);

	offset_pcm_samples = bytes_to_frames(runtime, offset_bytes);
	new_hw_ptr = runtime->status->hw_ptr + offset_pcm_samples;
	new_hw_ptr_pos = new_hw_ptr % runtime->buffer_size;

	audio_info("%s: stream = %s hw_ptr = %u new_hw_ptr = %u offset = %u\n", __func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE")
			, (unsigned int)runtime->status->hw_ptr
			, (unsigned int)new_hw_ptr
			, (unsigned int)new_hw_ptr_pos);
	return new_hw_ptr_pos;
}

static int audio_loopback_pcm_copy(struct snd_pcm_substream *substream,
			int channel, unsigned long pos,
			void __user *dst, unsigned long count)
{
	struct mtk_snd_loopback *loopback;
	struct mtk_runtime_struct *dev_runtime;
	unsigned char *buffer_tmp = NULL;
	unsigned int period_size = 0;
	unsigned int request_size = 0;
	unsigned int size_to_copy = 0;
	unsigned int copied_size = 0;
	unsigned int avail_size = 0;
	unsigned int size = 0;
	unsigned long flags;
	int monitor_status = 0;
	int timer_enable = 0;
	int retry_counter = 0;
	int ret = 0;

	if (substream == NULL || dst == NULL) {
		audio_err("[%s]Invalid parameter\n", __func__);
		return -EINVAL;
	}

	loopback = (struct mtk_snd_loopback *)substream->private_data;
	if (loopback == NULL) {
		audio_err("[%s]Invalid loopback\n", __func__);
		return -EINVAL;
	}

	request_size = count;
	audio_loopback_get_avail_bytes(loopback);
	ret = audio_loopback_get_copy_buffer(loopback,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK),
			(substream->f_flags & O_NONBLOCK),
			request_size,
			&buffer_tmp, &avail_size);

	if (ret < 0)
		return ret;

	/* Wake up Monitor if necessary */
	dev_runtime = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ?
		(struct mtk_runtime_struct *)&loopback->dev_play_runtime :
		(struct mtk_runtime_struct *)&loopback->dev_runtime;

	timer_enable = (loopback->loopback_aec_enabled || loopback->loopback_pcm_playback_running ||
			loopback->dsp_dma_buf.sw_loop_status);

	spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
	monitor_status = dev_runtime->monitor.monitor_status;
	spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);

	if (monitor_status == CMD_STOP && timer_enable) {
		mod_timer(&(dev_runtime->timer),
			(jiffies + msecs_to_jiffies(TIMER_UNIT) / TIMER_SHIFT));

		spin_lock_irqsave(&(dev_runtime->spin_lock), flags);
		dev_runtime->monitor.monitor_status = CMD_START;
		spin_unlock_irqrestore(&(dev_runtime->spin_lock), flags);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			loopback->dsp_pb_dma_buf.r_buf.update_jiffies = jiffies;
	}

	audio_info("%s: stream = %s\n", __func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		period_size = loopback->dsp_pb_dma_buf.r_buf.size >> PERIOD_SHIFT;

		do {
			size_to_copy = request_size - copied_size;
			if (size_to_copy > period_size)
				size_to_copy = period_size;

			ret = copy_from_user(buffer_tmp,
						(dst + copied_size), size_to_copy);

			size_to_copy = size_to_copy - ret;

			audio_debug("[%s]%d byte not copied from.\n", __func__, ret);

			while (1) {
				size = audio_loopback_dma_write(loopback,
						(void *)buffer_tmp, size_to_copy);

				if (size == 0) {
					if ((++retry_counter) > EXPIRATION_TIME) {
						audio_err("[%s]Retry write\n", __func__);
						return -ENXIO;
					}
					audio_loopback_delaytask(WAIT_5MS);
				} else
					break;
			}

			copied_size += size;
		} while (copied_size < request_size);
	} else {
		do {
			period_size = request_size - copied_size;

			size = audio_loopback_dma_read(loopback,
					(void *)buffer_tmp, period_size);

			if (size > 0) {
				ret = copy_to_user((dst + copied_size), buffer_tmp, size);
				copied_size += (size - ret);

				audio_debug("[%s]%d byte not copied.\n", __func__, ret);
			}

			audio_debug("%s: size(%u), period(%u), copy(%u), request(%u)\n",
				__func__, size, period_size, copied_size, request_size);

		} while (copied_size < request_size);
	}

	return 0;
}

static int audio_loopback_pcm_get_time_info(struct snd_pcm_substream *substream,
			struct timespec *system_ts, struct timespec *audio_ts,
			struct snd_pcm_audio_tstamp_config *audio_tstamp_config,
			struct snd_pcm_audio_tstamp_report *audio_tstamp_report)
{
	struct mtk_snd_loopback *loopback;
	struct timespec64 systemts;

	if (substream == NULL) {
		audio_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	loopback = (struct mtk_snd_loopback *)substream->private_data;
	if (loopback == NULL) {
		audio_err("[%s]Invalid loopback\n", __func__);
		return -EINVAL;
	}

	ktime_get_ts64(&systemts);

	audio_ts->tv_sec = systemts.tv_sec;
	audio_ts->tv_nsec = systemts.tv_nsec;
	system_ts->tv_sec = loopback->pcm_capture_ts.tv_sec;
	system_ts->tv_nsec = loopback->pcm_capture_ts.tv_nsec;

	audio_info("%s: audio(%ld, %ld) system(%ld, %ld)\n",
		__func__,
		audio_ts->tv_sec, audio_ts->tv_nsec,
		system_ts->tv_sec, system_ts->tv_nsec);

	return 0;
}

static struct snd_pcm_ops audio_loopback_pcm_ops = {
	.open = audio_loopback_pcm_open,
	.close = audio_loopback_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = audio_loopback_pcm_hw_params,
	.hw_free = audio_loopback_pcm_hw_free,
	.prepare = audio_loopback_pcm_prepare,
	.trigger = audio_loopback_pcm_trigger,
	.pointer = audio_loopback_pcm_pointer,
	.copy_user = audio_loopback_pcm_copy,
	.get_time_info = audio_loopback_pcm_get_time_info
};

///sys/devices/platform/audio-mcu.0/mtk_dbg//audio_test_cmd
static ssize_t slt_test_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_loopback *loopback = (struct mtk_snd_loopback *)card->private_data;

	return scnprintf(buf, MAX_LEN, "%u\n", loopback->slt_test_config);
}

static ssize_t slt_test_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_loopback *loopback = (struct mtk_snd_loopback *)card->private_data;
	int mask = (BIT0 << loopback->slt_test_ctrl_bit) & WORD_MASK;
	long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;

	if (!loopback->dsp_mixing_mode) {
		if ((uint32_t)item == 1)
			audio_loopback_write_reg_mask(REG.SLT_DMA_CTRL, mask, mask);
		else
			audio_loopback_write_reg_mask(REG.SLT_DMA_CTRL, mask, ZERO);
	}

	loopback->slt_test_config = (uint32_t)item;

	return count;
}
static DEVICE_ATTR_RW(slt_test);

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

///sys/devices/platform/audio-loopback-src.0/mtk_dbg/help
static ssize_t help_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, MAX_LEN, "Debug Help:\n"
				"- log_level <RW>: To control debug log level.\n"
				"                  Read log_level value.\n"
				"- slt_test <RW>: To enable slt test mode.\n"
				"                  Enable: 1; Disable: 0\n");
}

static DEVICE_ATTR_RO(help);

static struct attribute *mtk_dtv_audio_loopback_attrs[] = {
	&dev_attr_slt_test.attr,
	&dev_attr_log_level.attr,
	&dev_attr_help.attr,
	NULL,
};

static const struct attribute_group mtk_dtv_audio_loopback_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_audio_loopback_attrs
};

static int mtk_dtv_audio_loopback_create_sysfs_attr(struct platform_device *pdev)
{
	int ret = 0;

	/* Create device attribute files */
	audio_info("Create device attribute files\n");
	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_dtv_audio_loopback_attr_group);
	if (ret)
		audio_err("[%d]Fail to create sysfs files: %d\n", __LINE__, ret);
	return ret;
}

static int mtk_dtv_audio_loopback_remove_sysfs_attr(struct platform_device *pdev)
{
	/* Remove device attribute files */
	audio_info("Remove device attribute files\n");
	sysfs_remove_group(&pdev->dev.kobj, &mtk_dtv_audio_loopback_attr_group);
	return 0;
}

static int audio_loopback_card_pcm_new(struct mtk_snd_loopback *loopback,
			    int device, int playback_substreams, int capture_substreams)
{
	struct device_node *np;
	struct snd_pcm *pcm;
	const char *name;
	int err;

	if (loopback == NULL) {
		audio_err("[%s]Invalid argument\n", __func__);
		return -EINVAL;
	}

	np = of_find_node_by_name(NULL, "mtk-audio-loopback");
	if (!np) {
		audio_err("[%s]Fail to find device node\n", __func__);
		return -ENODEV;
	}

	if (of_property_read_string(np, "loopback_name", &name))
		name = device_name;

	err = snd_pcm_new(loopback->card, name, device,
			  playback_substreams, capture_substreams, &pcm);

	if (err < 0) {
		audio_err("[%s][%d]Fail to create pcm instance err=%d\n"
			, __func__, __LINE__, err);
		return err;
	}

	if (playback_substreams)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &audio_loopback_pcm_ops);

	if (capture_substreams)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &audio_loopback_pcm_ops);

	pcm->private_data = loopback;
	pcm->info_flags = 0;
	pcm->nonatomic = true;
	strlcpy(pcm->name, SND_AUDIO_LOOPBACK_PCM_NAME, sizeof(pcm->name));

	loopback->pcm = pcm;

	return err;
}

static unsigned int audio_loopback_get_dts_u32(struct device_node *np, const char *name)
{
	unsigned int value = 0;
	int ret;

	ret = of_property_read_u32(np, name, &value);
	if (ret)
		audio_warn("Can't get DTS property\n");

	return value;
}

static int audio_loopback_parse_registers(
	struct platform_device *pdev, struct mtk_snd_loopback *loopback)
{
	struct device_node *loop_np, *dsp_np;
	struct device_node *soc_np;
	struct resource res;
	int mixing_mode;
	int ret;
	int i;

	if (!pdev || !loopback)
		return -EINVAL;

	mixing_mode = loopback->dsp_mixing_mode;
	//audio loopback register from DMA
	loop_np = of_find_node_by_name(NULL, "mtk-audio-loopback");
	dsp_np = of_find_node_by_name(NULL, "capture-register");
	soc_np = of_find_node_by_name(NULL, "mtk-sound-soc");
	if (!loop_np || (!mixing_mode && (!dsp_np || !soc_np)))
		return -ENODEV;

	memset(&REG, 0x0, sizeof(struct audio_loopback_register));
	REG.RIU_BASE = audio_loopback_get_dts_u32(loop_np, "riu_base");
	if (mixing_mode) {
		REG.I2S_INPUT_CFG = audio_loopback_get_dts_u32(loop_np, "reg_mix_i2s_input_cfg");
		REG.I2S_OUT2_CFG = audio_loopback_get_dts_u32(loop_np, "reg_mix_i2s_out2_cfg");
		REG.DSP_DMA_CTRL = audio_loopback_get_dts_u32(loop_np, "reg_mix_aec_ctrl");
	} else {
		REG.I2S_INPUT_CFG = audio_loopback_get_dts_u32(dsp_np, "reg_i2s_input_cfg");
		REG.I2S_OUT2_CFG = audio_loopback_get_dts_u32(dsp_np, "reg_i2s_out2_cfg");
		REG.DSP_BANK_NUM = audio_loopback_get_dts_u32(soc_np, "audio_bank_num");
		REG.DSP_DMA_CTRL = audio_loopback_get_dts_u32(dsp_np, "reg_cap_dma9_ctrl");
		REG.DSP_DMA_RPTR = audio_loopback_get_dts_u32(dsp_np, "reg_cap_dma9_rptr");
		REG.DSP_DMA_WPTR = audio_loopback_get_dts_u32(dsp_np, "reg_cap_dma9_wptr");
	}
	REG.AEC_BANK_NUM = audio_loopback_get_dts_u32(loop_np, "audio_loopback_bank_num");
	REG.OUT_DMA_CTRL = audio_loopback_get_dts_u32(loop_np, "reg_dsp_pb_dma_ctrl");
	REG.OUT_DMA_WPTR = audio_loopback_get_dts_u32(loop_np, "reg_dsp_pb_dma_wptr");
	REG.OUT_DMA_LVEL = audio_loopback_get_dts_u32(loop_np, "reg_dsp_pb_dma_ddr_level");
	REG.AEC_SRC_CTRL = audio_loopback_get_dts_u32(loop_np, "reg_aec_src_ctrl");
	REG.AEC_SRC_SCFG = audio_loopback_get_dts_u32(loop_np, "reg_aec_src_cfg");
	REG.AEC_SRC_MCRL = audio_loopback_get_dts_u32(loop_np, "reg_aec_src_mch_ctrl");
	REG.AEC_SRC_MCFG = audio_loopback_get_dts_u32(loop_np, "reg_aec_src_mch_cfg");
	REG.AEC_DMA_ARLO = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_addr_lo");
	REG.AEC_DMA_ARHI = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_addr_hi");
	REG.AEC_DMA_ARZE = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_wr_size");
	REG.AEC_DMA_CTRL = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_ctrl");
	REG.AEC_DMA_RGEN = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_regen");
	REG.AEC_DMA_RPTR = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_rptr");
	REG.AEC_DMA_WPTR = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_wptr");
	REG.AEC_DMA_PAGA = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_paga_ctrl");
	REG.AEC_DMA_TCRL = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_timer_ctrl");
	REG.AEC_DMA_RCRL = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_rec_ctrl");
	REG.AEC_DMA_SINE = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_test_ctrl");
	REG.AEC_DMA_SCFG = audio_loopback_get_dts_u32(loop_np, "reg_aec_dma_test_cfg");
	REG.SLT_DMA_CTRL = audio_loopback_get_dts_u32(loop_np, "reg_slt_test_ctrl");

	//Allocate bank register list
	REG.AEC_REG_BANK = devm_kzalloc(&pdev->dev,
		sizeof(u32) * REG.AEC_BANK_NUM, GFP_KERNEL);
	REG.AEC_REG_BASE = devm_kzalloc(&pdev->dev,
		sizeof(void *) * REG.AEC_BANK_NUM, GFP_KERNEL);

	if (!mixing_mode) {
		REG.DSP_REG_BANK = devm_kzalloc(&pdev->dev,
			sizeof(u32) * REG.DSP_BANK_NUM, GFP_KERNEL);
		REG.DSP_REG_BASE = devm_kzalloc(&pdev->dev,
			sizeof(void *) * REG.DSP_BANK_NUM, GFP_KERNEL);
	}
	if (!REG.AEC_REG_BANK || !REG.AEC_REG_BASE
	|| (mixing_mode == 0 && (!REG.DSP_REG_BANK || !REG.DSP_REG_BASE)))
		return -ENOMEM;

	//Load register from AEC and DSP
	for (i = 0; i < REG.AEC_BANK_NUM; i++) {
		ret = of_address_to_resource(loop_np, i, &res);
		if (ret) {
			audio_err("[%s]fail get loopback addr %d\n", __func__, i);
			return -EINVAL;
		}

		REG.AEC_REG_BASE[i] = of_iomap(loop_np, i);
		if (IS_ERR((const void *)REG.AEC_REG_BASE[i])) {
			audio_err("[%s]err reg_base=%lx\n",
				__func__, (unsigned long)REG.AEC_REG_BASE[i]);
			return PTR_ERR((const void *)REG.AEC_REG_BASE[i]);
		}
		REG.AEC_REG_BANK[i] = (res.start - REG.RIU_BASE) >> 1;
	}

	for (i = 0; i < REG.DSP_BANK_NUM; i++) {
		ret = of_address_to_resource(soc_np, i, &res);
		if (ret) {
			audio_err("[%s]fail get audio addr %d\n", __func__, i);
			return -EINVAL;
		}

		REG.DSP_REG_BASE[i] = of_iomap(soc_np, i);
		if (IS_ERR((const void *)REG.DSP_REG_BASE[i])) {
			audio_err("[%s]err reg_base=%lx\n",
					__func__, (unsigned long)REG.DSP_REG_BASE[i]);
			return PTR_ERR((const void *)REG.DSP_REG_BASE[i]);
		}
		REG.DSP_REG_BANK[i] = (res.start - REG.RIU_BASE) >> 1;
	}

	return ret;
}

static int audio_loopback_parse_memory(struct mtk_snd_loopback *loopback)
{
	struct of_mmap_info_data of_mmap_info = {};
	struct device_node *loop_np, *dsp_np;
	int mixing_mode;
	int ret;

	if (!loopback)
		return -EINVAL;

	mixing_mode = loopback->dsp_mixing_mode;
	//audio loopback register from DMA and DSP
	loop_np = of_find_node_by_name(NULL, "mtk-audio-loopback");
	dsp_np = of_find_node_by_name(NULL, "mtk-sound-capture-platform");
	if (!loop_np || (!mixing_mode && !dsp_np))
		return -ENODEV;

	ret = of_property_read_u32(loop_np, "aec_dma_offset", &loopback->aec_dma_offset);
	ret |= of_property_read_u32(loop_np, "aec_dma_size", &loopback->aec_dma_size);
	ret |= of_property_read_u32(loop_np, "dsp_pb_dma_offset", &loopback->dsp_pb_dma_offset);
	ret |= of_property_read_u32(loop_np, "dsp_pb_dma_size", &loopback->dsp_pb_dma_size);
	if (!mixing_mode) {
		ret |= of_property_read_u32(dsp_np, "capture9_dma_offset",
							&loopback->dsp_dma_offset);
		ret |= of_property_read_u32(dsp_np, "capture9_dma_size",
							&loopback->dsp_dma_size);
	}
	if (ret) {
		audio_err("[%s]get dma info fail(%u,%u), (%d,%u,%u), (%u,%u)\n", __func__,
			loopback->aec_dma_offset, loopback->aec_dma_size,
			mixing_mode, loopback->dsp_dma_offset, loopback->dsp_dma_size,
			loopback->dsp_pb_dma_offset, loopback->dsp_pb_dma_size);
		return -EINVAL;
	}


	ret = of_property_read_u64(loop_np, "memory_bus_base",
					&loopback->aec_mem_info.memory_bus_base);
	if (!mixing_mode) {
		ret |= of_property_read_u64(dsp_np, "memory_bus_base",
					&loopback->dsp_mem_info.memory_bus_base);
		ret |= of_property_read_u64(dsp_np, "memory_bus_base",
					&loopback->dsp_pb_mem_info.memory_bus_base);
	}
	if (ret) {
		audio_err("[%s]can't get miu bus base (%llu, %d,%llu)\n", __func__,
			loopback->aec_mem_info.memory_bus_base,
			mixing_mode, loopback->dsp_mem_info.memory_bus_base);
		return -EINVAL;
	}

	ret = of_mtk_get_reserved_memory_info_by_idx(loop_np, 0, &of_mmap_info);
	loopback->aec_mem_info.miu = of_mmap_info.miu;
	loopback->aec_mem_info.bus_addr = of_mmap_info.start_bus_address;
	loopback->aec_mem_info.buffer_size = of_mmap_info.buffer_size;

	if (!mixing_mode) {
		ret |= of_mtk_get_reserved_memory_info_by_idx(dsp_np, 0, &of_mmap_info);
		loopback->dsp_mem_info.miu = of_mmap_info.miu;
		loopback->dsp_mem_info.bus_addr = of_mmap_info.start_bus_address;
		loopback->dsp_mem_info.buffer_size = of_mmap_info.buffer_size;
		loopback->dsp_pb_mem_info.miu = of_mmap_info.miu;
		loopback->dsp_pb_mem_info.bus_addr = of_mmap_info.start_bus_address;
		loopback->dsp_pb_mem_info.buffer_size = of_mmap_info.buffer_size;
	}
	if (ret) {
		audio_err("[%s]get mmap fail(%u,%llu,%llu), (%d,%u,%llu,%llu)\n", __func__,
			loopback->aec_mem_info.miu,
			loopback->aec_mem_info.bus_addr,
			loopback->aec_mem_info.buffer_size,
			mixing_mode,
			loopback->dsp_mem_info.miu,
			loopback->dsp_mem_info.bus_addr,
			loopback->dsp_mem_info.buffer_size);
		return -EINVAL;
	}

	return ret;
}

static int audio_loopback_card_machine_init(
	struct platform_device *pdev, struct mtk_snd_loopback *loopback)
{
	struct device_node *loop_np, *dsp_np;
	struct device_node *soc_np;
	int mixing_mode;
	u32 dsp_timer_irq_idx = 0;
	int aec_bypass = 0;
	int aec_regen_bypass = 0;
	int ret = 0;

	if (!loopback)
		return -EINVAL;

	mixing_mode = loopback->dsp_mixing_mode;
	//audio capture platform from DSP
	loop_np = of_find_node_by_name(NULL, "mtk-audio-loopback");
	dsp_np = of_find_node_by_name(NULL, "mtk-sound-capture-platform");
	soc_np = of_find_node_by_name(NULL, "mtk-sound-soc");
	if (!loop_np || (!mixing_mode && (!dsp_np | !soc_np)))
		return -ENODEV;

	ret = of_property_read_u32(loop_np, "aec_bypass", &aec_bypass);
	ret |= of_property_read_u32(loop_np, "aec_regen_bypass", &aec_regen_bypass);
	ret |= of_property_read_u32(loop_np, "aec_max_channel",  &loopback->aec_max_channel);
	ret |= of_property_read_u32(loop_np, "slt_test_ctrl_bit", &loopback->slt_test_ctrl_bit);
	ret |= of_property_read_u32(loop_np, "dsp_timer_irq_idx", &dsp_timer_irq_idx);
	ret |= of_property_read_u32(loop_np, "dsp_channels_num", &loopback->dsp_channels_num);
	ret |= of_property_read_u32(loop_np, "i2s_input_format", &loopback->i2s_input_format);
	ret |= of_property_read_u32(loop_np, "i2s_input_channel", &loopback->i2s_input_channel);
	ret |= of_property_read_u32(loop_np, "i2s_out_clk", &loopback->i2s_out_clk);
	if (ret)
		return -EINVAL;

	switch (loopback->i2s_input_channel) {
	case CH_2:
		loopback->dsp_capture_source = FROM_I2S_RX_2CH;
		break;
	case CH_4:
		loopback->dsp_capture_source = FROM_I2S_RX_4CH;
		break;
	case CH_8:
		loopback->dsp_capture_source = FROM_I2S_RX_8CH;
		break;
	default:
		audio_warn("[%s] channel is not match, use default(%d)!\n", __func__,
			loopback->i2s_input_channel);
		loopback->i2s_input_channel = CH_2;
		loopback->dsp_capture_source = FROM_I2S_RX_2CH;
	}
	loopback->loopback_aec_bypass = aec_bypass;
	loopback->loopback_aec_regen_bypass = aec_regen_bypass;
	loopback->aec_dma_irq = irq_of_parse_and_map(loop_np, 0);
	if (!mixing_mode)
		loopback->dsp_dma_irq = irq_of_parse_and_map(dsp_np, dsp_timer_irq_idx);
	if (loopback->aec_dma_irq <= 0 || (!mixing_mode && loopback->dsp_dma_irq <= 0))
		return -EFAULT;

	loopback->dsp_channel_list = devm_kzalloc(&pdev->dev,
		sizeof(int) * loopback->dsp_channels_num, GFP_KERNEL);
	if (!loopback->dsp_channel_list)
		return -ENOMEM;

	ret = of_property_read_u32_array(loop_np, "dsp_channel_list",
			loopback->dsp_channel_list, loopback->dsp_channels_num);
	if (ret)
		return -EINVAL;

	return ret;
}

static int audio_loopback_card_module_init(
	struct platform_device *pdev, struct mtk_snd_loopback *loopback)
{
	int ret;
	int max_ch;
	int i;

	if (!pdev || !loopback) {
		audio_err("Invalid pointer\n");
		return -EINVAL;
	}

	// PARSE -> MACHINE -> OTHER
	ret = audio_loopback_parse_registers(pdev, loopback);
	if (ret < 0) {
		audio_err("Fail to parse dts:%d\n", ret);
		return ret;
	}

	ret = audio_loopback_parse_memory(loopback);
	if (ret < 0) {
		audio_err("Fail to parse memory:%d\n", ret);
		return ret;
	}

	// 2. MACHINE initial
	ret = audio_loopback_card_machine_init(pdev, loopback);
	if (ret < 0) {
		audio_err("Fail to init MACHINE:%d\n", ret);
		return ret;
	}

	// 3. other init (ex: mutex, timer valid)
	loopback->dsp_max_channel = CH_2;
	if (loopback->dsp_channels_num > 0) {
		max_ch = loopback->dsp_channel_list[0];
		for (i = 0 ; i < loopback->dsp_channels_num ; i++) {
			if (loopback->dsp_channel_list[i] > max_ch)
				max_ch = loopback->dsp_channel_list[i];
		}
		loopback->dsp_max_channel = max_ch;
	}
	mutex_init(&loopback->loopback_pcm_lock);
	mutex_init(&(loopback->dev_runtime.kthread_lock));
	mutex_init(&(loopback->dev_play_runtime.kthread_lock));
	spin_lock_init(&(loopback->dev_runtime.spin_lock));
	spin_lock_init(&(loopback->dev_play_runtime.spin_lock));
	init_waitqueue_head(&(loopback->dev_runtime.kthread_waitq));
	init_waitqueue_head(&(loopback->dev_play_runtime.kthread_waitq));

	/* Start playback thread */
	loopback->capture_task = kthread_run(audio_loopback_trigger,
		(void *)&loopback->dev_runtime, "audio loopback capture pcm trigger");
	if (IS_ERR(loopback->capture_task)) {
		audio_err("create capture thread for mtk loopback snd card failed!\n");
		return IS_ERR(loopback->capture_task);
	}

	if (!loopback->dsp_mixing_mode) {
		loopback->play_task = kthread_run(audio_loopback_pb_trigger,
		(void *)&loopback->dev_play_runtime, "audio loopback playback pcm trigger");
		if (IS_ERR(loopback->play_task)) {
			audio_err("create playback thread for mtk loopback snd card failed!\n");
			return IS_ERR(loopback->play_task);
		}

		if (loopback->loopback_aec_bypass) {
			loopback->dsp_pb_dma_offset = loopback->aec_dma_offset;
			loopback->dsp_pb_dma_size = loopback->aec_dma_size;
			loopback->dsp_pb_mem_info.miu = loopback->aec_mem_info.miu;
			loopback->dsp_pb_mem_info.bus_addr = loopback->aec_mem_info.bus_addr;
			loopback->dsp_pb_mem_info.buffer_size = loopback->aec_mem_info.buffer_size;
			loopback->dsp_pb_mem_info.memory_bus_base =
							loopback->aec_mem_info.memory_bus_base;
		}
	}

	// Init default setting (i2s)
	card_reg[LOOPBACK_REG_PCM_SLAVE_FORMAT] = FORMAT_S16;
	card_reg[LOOPBACK_REG_PCM_SLAVE_RATE] = RATE_48000;
	card_reg[LOOPBACK_REG_PCM_SLAVE_CHANNELS] = loopback->i2s_input_channel;
	card_reg[LOOPBACK_REG_PCM_CAPTURE_SOURCE] = loopback->dsp_capture_source;

	// add to mtk_snd_audio list
	INIT_LIST_HEAD(&loopback->list);
	list_add_tail(&loopback->list, &audio_loopback_card_dev_list);

	audio_info("[%s][%d]Initial success\n", __func__, __LINE__);
	return ret;
}

static int audio_loopback_card_probe(struct platform_device *pdev)
{
	struct snd_card *card;
	struct mtk_snd_loopback *loopback;
	struct device_node *np;
	uint16_t i = 0;
	int dev = pdev->id;
	int ret;

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-audio-loopback");
	if (!of_device_is_available(np)) {
		audio_warn("[%s][%d]node status disable\n",
			__func__, __LINE__);
		ret = -ENODEV;
		return ret;
	}

	// Start to new card
	ret = snd_card_new(&pdev->dev, index[dev], id[dev],
		THIS_MODULE, sizeof(struct mtk_snd_loopback), &card);
	if (ret < 0) {
		audio_err("[%s][%d]Fail to create card error:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}

	loopback = card->private_data;
	loopback->card = card;
	loopback->dsp_mixing_mode = audio_loopback_get_dts_u32(np, "dsp_mixing_mode");
	if (loopback->dsp_mixing_mode)
		pcm_playback_substreams[dev] = 0;

	// new pcm
	for (i = 0 ; i < SND_AUDIO_LOOPBACK_DEVICE_NUM_MAX ; i++) {
		ret = audio_loopback_card_pcm_new(loopback, i,
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
	ret = audio_loopback_card_mixer_new(loopback);
	if (ret < 0) {
		audio_err("[%s][%d]Fail to create mixer error:%d\n",
			__func__, __LINE__, ret);
		goto nodev;
	}

	strlcpy(card->driver, SND_AUDIO_LOOPBACK_CARD, sizeof(card->driver));
	strlcpy(card->shortname, SND_AUDIO_LOOPBACK_CARD, sizeof(card->shortname));

	snprintf(card->longname, sizeof(card->longname), "%s %i",
			SND_AUDIO_LOOPBACK_CARD, dev + 1);

	snd_card_set_dev(card, &pdev->dev);
	loopback->dev = &pdev->dev;

	ret = snd_card_register(card);
	if (!ret)
		platform_set_drvdata(pdev, card);

	ret = audio_loopback_card_module_init(pdev, loopback);
	if (ret < 0) {
		audio_err("[%s][%d]Fail to initial module:%d\n",
			__func__, __LINE__, ret);
		goto nodev;
	}

	mtk_dtv_audio_loopback_create_sysfs_attr(pdev);

	return 0;

nodev:
	snd_card_free(card);
	return ret;
}

static int audio_loopback_card_remove(struct platform_device *pdev)
{
	struct snd_card *card = platform_get_drvdata(pdev);
	struct mtk_snd_loopback *loopback = (struct mtk_snd_loopback *)card->private_data;

	//Stop thread
	loopback->dev_runtime.timer_notify = TIMER_STOP_EVENT;
	loopback->dev_play_runtime.timer_notify = TIMER_STOP_EVENT;
	wake_up_interruptible(&(loopback->dev_runtime.kthread_waitq));
	wake_up_interruptible(&(loopback->dev_play_runtime.kthread_waitq));

	list_del(&loopback->list);
	mutex_destroy(&loopback->loopback_pcm_lock);
	mutex_destroy(&(loopback->dev_runtime.kthread_lock));
	mutex_destroy(&(loopback->dev_play_runtime.kthread_lock));
	mtk_dtv_audio_loopback_remove_sysfs_attr(pdev);
	snd_card_free(platform_get_drvdata(pdev));
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver audio_loopback_card_driver = {
	.probe = audio_loopback_card_probe,
	.remove = audio_loopback_card_remove,
	.driver = {
			.name = SND_AUDIO_LOOPBACK_DRIVER,
			.owner = THIS_MODULE,
			},
};

static int __init audio_loopback_card_init(void)
{
	int err;

	audio_info("[%s]\n", __func__);
	err = platform_driver_register(&audio_loopback_card_driver);
	if (err < 0)
		return err;

	device = platform_device_register_simple(SND_AUDIO_LOOPBACK_DRIVER, 0, NULL, 0);

	return 0;
}

static void __exit audio_loopback_card_exit(void)
{
	audio_info("[%s]\n", __func__);
	if (device != NULL) {
		platform_device_unregister(device);
		device = NULL;
	}
	platform_driver_unregister(&audio_loopback_card_driver);
}

module_init(audio_loopback_card_init);
module_exit(audio_loopback_card_exit);

/* Module information */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Audio ALSA Loopback driver");
