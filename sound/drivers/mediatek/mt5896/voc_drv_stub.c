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
#include "voc_drv_stub.h"




//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
struct voc_stub_data {
	wait_queue_head_t kthread_stub_waitq;
	struct task_struct *stub_task;
	struct voice_packet voice_msg;
};

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
static struct voc_communication_operater voc_stub_op;
struct voc_stub_data g_stub;
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
static int voc_drv_stub_capture_thread(void *param)
{

	struct voc_pcm_data *pcm_data;
	struct snd_pcm_substream *substream;
	const unsigned int u32Us = 16 * 1000;

	UNUSED(param);

	if (g_voc == NULL) {
		voc_err("[%s]Invalid voc structure\n", __func__);
		return 0;
	}


	do {
		wait_event_interruptible(g_stub.kthread_stub_waitq,
				kthread_should_stop() || g_voc->voc_pcm_running);

		if (kthread_should_stop())
			break;

		usleep_range(u32Us, u32Us + 1);
		if (g_voc->voc_pcm_running == 0)
			continue;

		pcm_data = (struct voc_pcm_data *)g_voc->voc_pcm;


		if (g_voc->voc_pcm == NULL) {
			voc_err("[%s]Invalid voc pcm structure\n", __func__);
			g_voc->voc_pcm_running = 0;
			continue;
		}

		if (pcm_data->capture_stream == NULL) {
			voc_err("[%s]Invalid pcm_data->capture_stream\n", __func__);
			g_voc->voc_pcm_running = 0;
			continue;
		}

		substream =	(struct snd_pcm_substream *)pcm_data->capture_stream;


		if (pcm_data->prtd == NULL) {
			voc_err("[%s]Invalid pcm_data->prtd\n", __func__);
			g_voc->voc_pcm_running = 0;
			continue;
		}

		if (substream->runtime == NULL) {
			voc_err("[%s]Invalid substream->runtime\n", __func__);
			g_voc->voc_pcm_running = 0;
			continue;
		}

		pcm_data->prtd->dma_level_count += substream->runtime->buffer_size;
		snd_pcm_period_elapsed(substream);

	} while (!kthread_should_stop());

	return 0;
}
//==============================================================================
static void voc_drv_stub_capture_thread_init(void)
{
	static bool bInitFlag;//=0;//INITIALISED_STATIC: do not initialise statics to 0

	if (bInitFlag == 0) {
		init_waitqueue_head(&g_stub.kthread_stub_waitq);
		bInitFlag = 1;
	}
}
//==============================================================================
int voc_drv_stub_wakeup(void)
{
	int ret = 0;

	wake_up_interruptible(&g_stub.kthread_stub_waitq);
	return ret;
}
//==============================================================================
int voc_drv_stub_run(void)
{
	int ret = 0;

	if (g_stub.stub_task != NULL)
		return 0;
	voc_drv_stub_capture_thread_init();
	g_stub.stub_task = kthread_run(voc_drv_stub_capture_thread, 0, "voc_stub_task");
	if (IS_ERR(g_stub.stub_task)) {
		voc_err("create thread for add stub_task failed!\n");
		ret = PTR_ERR(g_stub.stub_task);
		g_stub.stub_task = NULL;
	}

	return ret;
}
//==============================================================================
int voc_drv_stub_stop(void)
{
	int ret = 0;

	if (g_stub.stub_task == NULL)
		return 0;

	//to do: refactory ltp test item.
	kthread_stop(g_stub.stub_task);
	g_stub.stub_task = NULL;

	return ret;
}
//==============================================================================
int voc_drv_stub_bind(struct mtk_snd_voc *snd_voc)
{
	UNUSED(snd_voc);
	return 0;
}

int voc_drv_stub_dma_init_channel(
		uint64_t dma_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t sample_width,
		uint32_t sample_rate)
{
	UNUSED(dma_paddr);
	UNUSED(buf_size);
	UNUSED(channels);
	UNUSED(sample_width);
	UNUSED(sample_rate);
	return 0;
}

int voc_drv_stub_dma_start_channel(void)
{
	return 0;
}

int voc_drv_stub_dma_stop_channel(void)
{
	return 0;
}

int voc_drv_stub_dma_enable_sinegen(bool en)
{
	UNUSED(en);
	return 0;
}

int voc_drv_stub_enable_vq(bool en)
{
	UNUSED(en);
	return 0;
}

int voc_drv_stub_config_vq(uint8_t mode)
{
	UNUSED(mode);
	return 0;
}

int voc_drv_stub_enable_hpf(int32_t stage)
{
	UNUSED(stage);
	return 0;
}

int voc_drv_stub_config_hpf(int32_t coeff)
{
	UNUSED(coeff);
	return 0;
}

int voc_drv_stub_dmic_gain(int16_t gain)
{
	UNUSED(gain);
	return 0;
}

int voc_drv_stub_dmic_mute(bool en)
{
	UNUSED(en);
	return 0;
}

int voc_drv_stub_dmic_switch(bool en)
{
	UNUSED(en);
	return 0;
}

int voc_drv_stub_reset_voice(void)
{
	return 0;
}


int voc_drv_stub_update_snd_model(
				dma_addr_t snd_model_paddr,
				uint32_t size)
{
	UNUSED(snd_model_paddr);
	UNUSED(size);
	return 0;
}

int voc_drv_stub_get_hotword_ver(void)
{
	return 0;
}

int voc_drv_stub_get_hotword_identifier(void)
{
	return 0;
}

int voc_drv_stub_enable_slt_test(int32_t loop, uint32_t addr)
{
	UNUSED(loop);
	UNUSED(addr);
	return 0;
}

int voc_drv_stub_ts_sync_start(void)
{
	return 0;
}

int voc_drv_stub_enable_seamless(bool en)
{
	UNUSED(en);
	return 0;
}

int voc_drv_stub_notify_status(int32_t status)
{
	UNUSED(status);
	return 0;
}

int voc_drv_stub_power_status(int32_t status)
{
	UNUSED(status);
	return 0;
}

int voc_drv_stub_dma_get_reset_status(void)
{
	return 0;
}

int voc_drv_stub_vd_task_state(void)
{
	return 0;
}
void *voc_drv_stub_get_voice_packet_addr(void)
{
	return (void *)&g_stub.voice_msg;
}
//==============================================================================
void voc_drv_stub_exit(void)
{

}
//==============================================================================
void voc_drv_stub_init(void)
{
	voc_info("[%s]\n", __func__);
}

//==============================================================================
struct voc_communication_operater *voc_drv_stub_get_op(void)
{
	return &voc_stub_op;
}
EXPORT_SYMBOL_GPL(voc_drv_stub_get_op);

static struct voc_communication_operater voc_stub_op = {
	.bind = voc_drv_stub_bind,
	.dma_start_channel = voc_drv_stub_dma_start_channel,
	.dma_stop_channel = voc_drv_stub_dma_stop_channel,
	.dma_init_channel = voc_drv_stub_dma_init_channel,
	.dma_enable_sinegen =  voc_drv_stub_dma_enable_sinegen,
	.dma_get_reset_status = voc_drv_stub_dma_get_reset_status,
	.enable_vq = voc_drv_stub_enable_vq,
	.config_vq = voc_drv_stub_config_vq,
	.enable_hpf = voc_drv_stub_enable_hpf,
	.config_hpf = voc_drv_stub_config_hpf,
	.dmic_gain = voc_drv_stub_dmic_gain,
	.dmic_mute = voc_drv_stub_dmic_mute,
	.dmic_switch = voc_drv_stub_dmic_switch,
	.reset_voice = voc_drv_stub_reset_voice,
	.update_snd_model = voc_drv_stub_update_snd_model,
	.get_hotword_ver = voc_drv_stub_get_hotword_ver,
	.get_hotword_identifier = voc_drv_stub_get_hotword_identifier,
	.enable_slt_test = voc_drv_stub_enable_slt_test,
	.enable_seamless = voc_drv_stub_enable_seamless,
	.notify_status = voc_drv_stub_notify_status,
	.power_status = voc_drv_stub_power_status,
	.ts_sync_start = voc_drv_stub_ts_sync_start,
	.get_voice_packet_addr = voc_drv_stub_get_voice_packet_addr,
};

