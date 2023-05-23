// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <sound/soc.h>
#include "voc_common.h"
#include "voc_common_reg.h"
#include "voc_drv_paganini.h"
#include "voc_hal_paganini.h"



//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define ENABLE_DUMP_PCM_BUFFER (0)
#define ENABLE_INT_CONTROL     (1)

struct voc_paganini_data {
	wait_queue_head_t kthread_waitq;
	struct task_struct *thread_task;
	struct voice_packet voice_msg;
	struct work_struct work_dma_status;
	struct timespec64 timestamp;
	uint64_t time_interval;
	int irq;
	u32 irqtype;
	u32 mute_enable;
	u32 voc_delta_time;
	bool dma_buf_wr_enable;
	u16 bck_mode;

};


#define CONFIG_PCM_SAMPLE_RATE      48000 //16000
#define CONFIG_PCM_CHANNEL              2
#define CONFIG_PCM_STREAM_BUFFER_MS   125
#define CONFIG_PCM_STREAM_CHANNEL       4
#define VOICE_DMA_BUFFER_PERIOD_MS      (64) //0.5s

#define VOICE_UNDERRUN_MS           (10) //10 ms
#define VOICE_OVERRUN_MS            (VOICE_DMA_BUFFER_PERIOD_MS/4) //1/4 buffer size



#define VOC_MAX_PCM_BIT_WIDTH   (sizeof(int32_t))
#define VOC_TASK_PCM_BUF_MS     (16)
#define VOC_TASK_PCM_BUF_COUNT  ((CONFIG_PCM_SAMPLE_RATE * CONFIG_PCM_STREAM_CHANNEL *\
					VOC_TASK_PCM_BUF_MS * VOC_MAX_PCM_BIT_WIDTH)/1000)
#define VOC_TASK_READ_PCM_FRAME ((CONFIG_PCM_SAMPLE_RATE * VOC_TASK_PCM_BUF_MS)/1000)
#define VOC_BIT_TO_BYTE(_x_)    ((_x_)/8)
#define VOC_SECOND_IN_MS        (1000)
#define VOC_MS_IN_US            (1000)
#define VOC_MS_IN_NS            (1000 * 1000)
#define VOC_MAX_VDMA_SIZE       (0xFF000)
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
static struct voc_communication_operater voc_paganini_op;
struct voc_paganini_data g_paganini;

static uint8_t voc_task_pcm_buf[VOC_TASK_PCM_BUF_COUNT]; //12K
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
#if ENABLE_DUMP_PCM_BUFFER
void dump_buffer(void)
{
	#define DUMP_LEN  0x10
	#define DUMP_ADDR 0x200

	int i;
	unsigned char *ptr = NULL;

	ptr = (unsigned char *)g_voc->pcm->streams[1].substream->dma_buffer.area;

	voc_info("[%s]ptr=[%lx]\n", __func__, (unsigned long)ptr);
	//rmb();
	for (i = 0; i < DUMP_LEN; i++)
		voc_info("val[%x]:%x\n", i, ptr[i]);

	for (i = DUMP_ADDR; i < (DUMP_ADDR + DUMP_LEN); i++)
		voc_info("val[%x]:%x\n", i, ptr[i]);
}
#endif
//==============================================================================
int voc_drv_paganini_bind(struct mtk_snd_voc *snd_voc)
{
	UNUSED(snd_voc);
	return 0;
}

static uint64_t voc_drv_paganini_ktime_to_ms(struct timespec64 *ktime)
{
	uint64_t value;

	value = ktime->tv_sec * VOC_SECOND_IN_MS;
	value +=  ktime->tv_nsec / VOC_MS_IN_NS;
	return value;
}
//==============================================================================
static int voc_drv_paganini_read_pcm(uint8_t *sample_buf, u32 frame_count,
						uint32_t *ret_frame, int blk,
						uint64_t *time_interval)
{
	uint32_t rd_size       = 0;
	uint32_t ret_size      = 0;
	uint32_t mute_delay    = 0;
	uint8_t *byte_buf      = sample_buf;
	uint8_t  channels      = voc_hal_paganini_get_channel();
	uint32_t bytes_of_bit_width = VOC_BIT_TO_BYTE(voc_hal_paganini_get_bit_width());
	uint32_t sample_per_ms = voc_hal_paganini_get_sample_rate()/VOC_SECOND_IN_MS;
	uint32_t buf_size      = frame_count * bytes_of_bit_width * channels;


	uint32_t underrun_size = (CONFIG_PCM_SAMPLE_RATE / VOC_SECOND_IN_MS) *
							 bytes_of_bit_width *
							 channels *
							 VOICE_UNDERRUN_MS;

	if (frame_count == 0)
		return FALSE;

	if (channels == 0 || sample_per_ms == 0 || bytes_of_bit_width == 0)
		return FALSE; //protection of div by zero.


	if (underrun_size > buf_size)
		underrun_size = buf_size;

	do {
		if (g_paganini.dma_buf_wr_enable == 0)
			return FALSE;
		else if (g_paganini.mute_enable) {
			ret_size = buf_size;
			buf_size = 0;

			*ret_frame = (ret_size / bytes_of_bit_width) / channels;

			mute_delay = *ret_frame / sample_per_ms; //to ms
			mute_delay *= VOC_MS_IN_US;
			usleep_range(mute_delay, mute_delay + 1);


			ktime_get_ts64(&g_paganini.timestamp);

			*time_interval = voc_drv_paganini_ktime_to_ms(
				&g_paganini.timestamp);
			return TRUE;
		} else if (voc_hal_paganini_dma_wr_level_cnt() > underrun_size || !blk) {
			rd_size = voc_hal_paganini_read_dma_data((uint8_t *)byte_buf, buf_size);
			byte_buf += rd_size;
			buf_size -= rd_size;
			ret_size += rd_size;

			if ((blk && (buf_size == 0)) || !blk) {
				*ret_frame = (ret_size / bytes_of_bit_width) / channels;

				*time_interval = voc_drv_paganini_ktime_to_ms(
					&g_paganini.timestamp);
				return TRUE;
			}
		} else {
			usleep_range(VOC_MS_IN_US, VOC_MS_IN_US + 1);
			//refactor:removed interrupt setting.
			//voc_hal_paganini_clear_dma_interrupt(voice_dma_intclr_wr_full);
			//voc_hal_paganini_dma_enable_interrupt(1, 1);
		}

	} while (1);
}

//==============================================================================
int voc_drv_paganini_pcm_handler_thread(void *param)
{
	#define VOC_PERIOD_MS   (16)
	#define VOC_PERIOD_SIZE (2048) //temporary

	struct data_moving_notify *payload;
	uint32_t period_cnt = 0;
	uint32_t period_size = 0;
	uint32_t frame_count = 0;
	uint64_t time_interval = 0;
	uint8_t channels;
	uint32_t bytes_of_bit_width;
	uint32_t ret_frame;
	int ret = 0;

	UNUSED(param);
	payload = (struct data_moving_notify *)&g_paganini.voice_msg.payload;

	do {
		wait_event_interruptible(g_paganini.kthread_waitq,
					kthread_should_stop() ||
					g_paganini.dma_buf_wr_enable);

		if (kthread_should_stop()) {
			voc_err("%s Receive kthread_should_stop\n", __func__);
			break;
		}

		channels = voc_hal_paganini_get_channel();
		bytes_of_bit_width = VOC_BIT_TO_BYTE(voc_hal_paganini_get_bit_width());
		frame_count = (voc_hal_paganini_get_sample_rate() / VOC_SECOND_IN_MS) *
						VOC_PERIOD_MS;
		ret = voc_drv_paganini_read_pcm(voc_task_pcm_buf,
					frame_count,
					&ret_frame, 1, &time_interval);
		//voc_info("time_interval:%llu\n",  time_interval);

		if (ret == FALSE) {
			period_cnt = 0;
			continue;
		}

		period_cnt++;
		period_size = frame_count * channels * bytes_of_bit_width;

		g_paganini.time_interval += VOC_PERIOD_MS;
		time_interval = g_paganini.time_interval;
		mutex_lock(&g_voc->pcm_lock);
		if (g_paganini.dma_buf_wr_enable) {
			payload->period_count = period_cnt;
			payload->byte_count = period_size;
			payload->time_interval = time_interval;

			g_voc->vd_notify = 1;
			wake_up_interruptible(&g_voc->kthread_waitq);
		} else {
			period_cnt = 0;
			voc_info("[%d] g_paganini.dma_buf_wr_enable = false\n", __LINE__);
		}
		mutex_unlock(&g_voc->pcm_lock);

	} while (1);

	return 0;

}
//==============================================================================
//CM4 notification interval is based on the fixed frame size(16kHz,16bits),
//and it's irrelevant to bitwidth and sample rate.
//So we should update notification frequency
//in case the notification is frequent.
int voc_drv_paganini_dma_init_channel(
		uint64_t cpu_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t bit_width,
		uint32_t sample_rate)
{
	uint32_t u32DmaAddr;
	void *pVAddr;
	int ret;

	voc_info("[%s]\n", __func__);

	if (!g_voc)
		return -EINVAL;

	if (cpu_paddr > U32_MAX) {
		voc_err("[%s] only support 32bit address\n", __func__);
		return -EINVAL;
	}

	u32DmaAddr = (cpu_paddr & U32_MAX);

	if (u32DmaAddr < g_voc->miu_base) {
		voc_err("[%s] dma_paddr is small than miu_base\n", __func__);
		return -EINVAL;
	}
	u32DmaAddr -= g_voc->miu_base;

	voc_info("u32DmaAddr:0x%x\n", u32DmaAddr);
	voc_info("buf_size:%d\n", buf_size);
	voc_info("channels:%d\n", channels);
	voc_info("bit_width:%d\n", bit_width);
	voc_info("sample_rate:%d\n", sample_rate);

	ret = voc_hal_paganini_set_channel(channels);
	if (ret != 0)
		return ret;

	ret = voc_hal_paganini_set_bit_width(bit_width);
	if (ret != 0)
		return ret;

	ret = voc_hal_paganini_set_sample_rate(sample_rate,
						g_paganini.bck_mode,
						g_paganini.mute_enable);
	if (ret != 0)
		return ret;

	if (buf_size > VOC_MAX_VDMA_SIZE) {
		//to do:fix it
		voc_info("buf_size = 0x%x: out of range\n", buf_size);
		buf_size = VOC_MAX_VDMA_SIZE;
	}

	voc_hal_paganini_set_dma_buf_addr(u32DmaAddr, buf_size);
	pVAddr = (void *)g_voc->pcm->streams[1].substream->dma_buffer.area;
	voc_hal_paganini_set_virtual_buf_addr(pVAddr, buf_size);
	voc_hal_paganini_dma_reset();

	return 0;
}
//==============================================================================
int voc_drv_paganini_dma_start_channel(void)
{
	int ret = 0;
	uint32_t sample_byte = VOC_BIT_TO_BYTE(voc_hal_paganini_get_bit_width());
	uint8_t mic_num = voc_hal_paganini_get_channel();
	uint32_t overrun_thres;

	overrun_thres = (CONFIG_PCM_SAMPLE_RATE *
					sample_byte *
					mic_num *
					VOICE_OVERRUN_MS) / VOC_SECOND_IN_MS;

	voc_info("[%s]\n", __func__);

	if (!g_paganini.dma_buf_wr_enable) {
		voc_hal_paganini_set_dma_buf_overrun_thres(overrun_thres);
		voc_hal_paganini_dma_reset();

		//start
		voc_hal_paganini_clear_dma_interrupt(voice_dma_intclr_wr_full);
		#if ENABLE_INT_CONTROL
		ktime_get_ts64(&g_paganini.timestamp);
		g_paganini.time_interval = voc_drv_paganini_ktime_to_ms(
				&g_paganini.timestamp);
		voc_hal_paganini_dma_enable_interrupt(1, 0);
		voc_hal_paganini_enable_vdma_timer_interrupt(1);
		#endif
		ret = voc_hal_paganini_dma_start_channel();
		g_paganini.dma_buf_wr_enable = TRUE;

		//[cm4]if (sample_rate == VOICE_SAMPLE_RATE_32K)
		//	_voice_drv_timer_enable(handle, VOICE_TIMER_TASK_8MS, pdTRUE);
		//else
		//	_voice_drv_timer_enable(handle, VOICE_TIMER_TASK_16MS, pdTRUE);

		//[cm4]voc_state = VOICE_STATE_START;
		#if ENABLE_DUMP_PCM_BUFFER
		dump_buffer();
		#endif
		wake_up_interruptible(&g_paganini.kthread_waitq);
	}
	return ret;
}

//==============================================================================
int voc_drv_paganini_dma_stop_channel(void)
{
	int ret = 0;

	voc_info("[%s]\n", __func__);
	#if ENABLE_DUMP_PCM_BUFFER
	dump_buffer();
	#endif
	if (g_paganini.dma_buf_wr_enable) {
		voc_hal_paganini_dma_enable_interrupt(0, 0);
		voc_hal_paganini_enable_vdma_timer_interrupt(0);
		ret = voc_hal_paganini_dma_stop_channel();
		g_paganini.dma_buf_wr_enable = FALSE;
	}
	g_paganini.voc_delta_time = 0;
	//[cm4]voc_state = VOICE_STATE_STOP;

	return ret;

}
//==============================================================================
int voc_drv_paganini_dma_enable_sinegen(bool en)
{
	return voc_hal_paganini_dma_enable_sinegen(en);
}
//==============================================================================
int voc_drv_paganini_enable_vq(bool en)
{
	return voc_hal_paganini_enable_vq(en);
}
//==============================================================================
int voc_drv_paganini_config_vq(uint8_t mode)
{
	if (mode == VOICE_VQ_FORCE_WAKE_ID)
		g_voc->sysfs_notify_test = 1;

	return voc_hal_paganini_config_vq(mode);
}
//==============================================================================
int voc_drv_paganini_enable_hpf(int32_t stage)
{
	return voc_hal_paganini_enable_hpf(stage);
}
//==============================================================================
int voc_drv_paganini_config_hpf(int32_t coeff)
{
	return voc_hal_paganini_config_hpf(coeff);
}
//==============================================================================
int voc_drv_paganini_dmic_gain(int16_t gain)
{
	return voc_hal_paganini_dmic_gain(gain);
}
//==============================================================================
int voc_drv_paganini_dmic_mute(bool en)
{
	int ret;

	ret = voc_hal_paganini_dmic_mute(en, g_paganini.bck_mode);
	g_paganini.mute_enable = en;
	return ret;
}
//==============================================================================
int voc_drv_paganini_dmic_switch(bool en)
{
	return voc_hal_paganini_dmic_switch(en, g_paganini.bck_mode, g_paganini.mute_enable);
}
//==============================================================================
int voc_drv_paganini_reset_voice(void)
{
	return voc_hal_paganini_reset_voice();
}
//==============================================================================
int voc_drv_paganini_update_snd_model(
				dma_addr_t snd_model_paddr,
				uint32_t size)
{
	return voc_hal_paganini_update_snd_model(snd_model_paddr, size);
}
//==============================================================================
int voc_drv_paganini_get_hotword_ver(void)
{
	return voc_hal_paganini_get_hotword_ver();
}
//==============================================================================
int voc_drv_paganini_get_hotword_identifier(void)
{
	voc_info("[%s]\n", __func__);
	if (g_voc && g_voc->voc_card) {
		if (g_voc->voc_card->uuid_array)
			memset(g_voc->voc_card->uuid_array, 1,
					g_voc->voc_card->uuid_size);

	}
	return voc_hal_paganini_get_hotword_identifier();
}
//==============================================================================
void voc_drv_paganini_slt_init(uint32_t buf_size)
{
	uint16_t *ptr;

	ptr = (uint16_t *)g_voc->pcm->streams[1].substream->dma_buffer.area;
	memset(ptr, 0, buf_size);
}
//==============================================================================
int voc_drv_paganini_check_slt_result(void)
{
	#define SLT_CHECK_COUNT 0x100
	#define SLT_CHECK_START 0x10
	#define SLT_NEXT_ADDR   2
	int i;
	int result = 1; // 0:fail, 1:pass
	uint16_t *ptr;

	ptr = (uint16_t *)g_voc->pcm->streams[1].substream->dma_buffer.area;
	wmb(); // dma
	for (i = SLT_CHECK_START; i < (SLT_CHECK_COUNT + SLT_CHECK_START); i++) {
		if (((ptr[i] + 1) & U16_MAX) != ptr[i + SLT_NEXT_ADDR])
			result = 0;
	}

	return result;
}

//==============================================================================
int voc_drv_paganini_enable_slt_test(int32_t loop, uint32_t addr)
{
	#define VOC_LTP_BUF_SIZE  ((VOC_SAMPLE_RATE_16K/1000) * \
								VOC_CHANNEL_2 * \
								VOC_TASK_PCM_BUF_MS * \
								VOC_BIT_TO_BYTE(VOC_BIT_WIDTH_16))

	if (loop == 0)
		return 0;

	if (g_paganini.dma_buf_wr_enable) {
		voc_err("[%s] paganini is running\n", __func__);
		return 0;
	}

	voc_drv_paganini_slt_init(VOC_TASK_PCM_BUF_COUNT);

	voc_hal_paganini_dma_enable_interrupt(0, 0);
	voc_hal_paganini_dma_enable_incgen(1);
	voc_drv_paganini_dma_init_channel(addr + g_voc->miu_base,
					VOC_TASK_PCM_BUF_COUNT,
					VOC_CHANNEL_2,
					VOC_BIT_WIDTH_16,
					VOC_SAMPLE_RATE_16K);
	voc_hal_paganini_dma_start_channel();
	mdelay(VOC_TASK_PCM_BUF_MS);
	mdelay(VOC_TASK_PCM_BUF_MS);
	voc_hal_paganini_dma_stop_channel();
	voc_hal_paganini_dma_enable_incgen(0);
	#if ENABLE_DUMP_PCM_BUFFER
	dump_buffer();
	#endif

	g_voc->voc_card->status = voc_drv_paganini_check_slt_result();
	return 0;
}
//==============================================================================
int voc_drv_paganini_ts_sync_start(void)
{
	return voc_hal_paganini_ts_sync_start();
}
//==============================================================================
int voc_drv_paganini_enable_seamless(bool en)
{
	return voc_hal_paganini_enable_seamless(en);
}
//==============================================================================
int voc_drv_paganini_notify_status(int32_t status)
{
	return voc_hal_paganini_notify_status(status);
}
//==============================================================================
int voc_drv_paganini_power_status(int32_t status)
{
	UNUSED(status);
	return 0;
}
//==============================================================================
int voc_drv_paganini_dma_get_reset_status(void)
{
	return voc_hal_paganini_dma_get_reset_status();
}
//==============================================================================
int voc_drv_paganini_vd_task_state(void)
{
	return 0;
}
//==============================================================================
void *voc_drv_paganini_get_voice_packet_addr(void)
{
	return (void *)&g_paganini.voice_msg;
}
//==============================================================================
static void voc_drv_paganini_dma_status_handler(struct work_struct *work)
{
	int full, overrun;

	if (g_paganini.dma_buf_wr_enable) {
		voc_hal_paganini_get_dma_interrupt_status(&full, &overrun);

		if (full) {
			voc_info("[%s] full detected\n", __func__);
			voc_hal_paganini_dma_stop_channel();
			g_paganini.dma_buf_wr_enable = FALSE;
			voc_hal_paganini_clean_level_count();
			voc_hal_paganini_clear_dma_interrupt(voice_dma_intclr_wr_full);
			g_paganini.voc_delta_time = 0;
			//[cm4]voc_state = VOICE_STATE_XRUN;
		} else if (overrun) {
			//no need to handle overrun.
			//voc_info("[%s] overrun detected\n", __func__);
			//voc_hal_paganini_dma_enable_interrupt(1, 0);
		}
	}
}

//==============================================================================
static irqreturn_t voc_drv_paganini_irq_handler(int irq, void *p)
{
	int full, overrun;

	UNUSED(irq);
	UNUSED(p);
	voc_hal_paganini_get_dma_interrupt_status(&full, &overrun);

	if (full || overrun) {
		voc_hal_paganini_dma_enable_interrupt(0, 0);
		schedule_work(&g_paganini.work_dma_status);
	}

	if (voc_hal_paganini_get_vdma_timer_interrupt_status())	{
		voc_hal_paganini_clear_vdma_timer_interrupt();
		timespec64_add_ns(&g_paganini.timestamp, VOC_PERIOD_MS*VOC_MS_IN_NS);
		//ktime_get_ts64(&g_paganini.timestamp);
	}

	return IRQ_HANDLED;
}
//==============================================================================
int voc_drv_paganini_irq_init(void)
{
	struct device_node *np;
	int err = 0;

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-dma");
	if (!np) {
		voc_err("[%s] could not find device tree\n", __func__);
		return -EINVAL;
	}

	g_paganini.irq = of_irq_get(np, 0);
	of_node_put(np);
	if (g_paganini.irq < 0) {
		voc_err("[%s] could not get irq resource\n", __func__);
		return -EINVAL;
	}
	g_paganini.irqtype = irq_get_trigger_type(g_paganini.irq);

	voc_info("[%s]irq = %d\n", __func__, g_paganini.irq);
	voc_info("[%s]irqtype = %d\n", __func__, g_paganini.irqtype);//IRQ_TYPE_LEVEL_HIGH

	INIT_WORK(&g_paganini.work_dma_status, voc_drv_paganini_dma_status_handler);

	err = devm_request_irq(g_voc->dev, g_paganini.irq, voc_drv_paganini_irq_handler,
			IRQF_TRIGGER_HIGH, "voice_irq", NULL);

	if (err) {
		voc_err("[%s] could not request IRQ: %d:%d\n", __func__, g_paganini.irq, err);
		return err;
	}

	return err;
}
//==============================================================================
static int voc_drv_paganini_dts_init(void)
{
	int ret = 0;
	struct device_node *np;
	u32 u32Tmp;

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-dma");
	if (!np)
		return -EFAULT;

	ret = of_property_read_u32(np, "bck_mode", &u32Tmp);

	if ((ret >= 0) &&
		((u32Tmp >= VOC_BCK_MODE_MIN) && (u32Tmp <= VOC_BCK_MODE_MAX)))
		g_paganini.bck_mode = (u16)u32Tmp;
	else
		g_paganini.bck_mode = VOC_DEFAULT_BCK_MODE;

	of_node_put(np);

	return ret;
}
//==============================================================================
void voc_drv_paganini_exit(void)
{
	kthread_stop(g_paganini.thread_task);
}
//==============================================================================
void voc_drv_paganini_init(void)
{
	voc_info("[%s]\n", __func__);
	g_paganini.voc_delta_time = 0;
	g_paganini.dma_buf_wr_enable = 0;
	g_paganini.mute_enable = 0;
	voc_drv_paganini_dts_init();
	voc_hal_paganini_init();
	voc_drv_paganini_irq_init();

	init_waitqueue_head(&g_paganini.kthread_waitq);
	g_paganini.thread_task = kthread_run(voc_drv_paganini_pcm_handler_thread,
										0, "paganini_task");
	if (IS_ERR(g_paganini.thread_task)) {
		voc_err("create thread for paganini failed!\n");
		return;
	}
}
//==============================================================================
struct voc_communication_operater *voc_drv_paganini_get_op(void)
{
	return &voc_paganini_op;
}
EXPORT_SYMBOL_GPL(voc_drv_paganini_get_op);

static struct voc_communication_operater voc_paganini_op = {
	.bind = voc_drv_paganini_bind,
	.dma_start_channel = voc_drv_paganini_dma_start_channel,
	.dma_stop_channel = voc_drv_paganini_dma_stop_channel,
	.dma_init_channel = voc_drv_paganini_dma_init_channel,
	.dma_enable_sinegen =  voc_drv_paganini_dma_enable_sinegen,
	.dma_get_reset_status = voc_drv_paganini_dma_get_reset_status,
	.enable_vq = voc_drv_paganini_enable_vq,
	.config_vq = voc_drv_paganini_config_vq,
	.enable_hpf = voc_drv_paganini_enable_hpf,
	.config_hpf = voc_drv_paganini_config_hpf,
	.dmic_gain = voc_drv_paganini_dmic_gain,
	.dmic_mute = voc_drv_paganini_dmic_mute,
	.dmic_switch = voc_drv_paganini_dmic_switch,
	.reset_voice = voc_drv_paganini_reset_voice,
	.update_snd_model = voc_drv_paganini_update_snd_model,
	.get_hotword_ver = voc_drv_paganini_get_hotword_ver,
	.get_hotword_identifier = voc_drv_paganini_get_hotword_identifier,
	.enable_slt_test = voc_drv_paganini_enable_slt_test,
	.enable_seamless = voc_drv_paganini_enable_seamless,
	.notify_status = voc_drv_paganini_notify_status,
	.power_status = voc_drv_paganini_power_status,
	.ts_sync_start = voc_drv_paganini_ts_sync_start,
	.get_voice_packet_addr = voc_drv_paganini_get_voice_packet_addr,
};

