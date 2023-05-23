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
#include "voc_communication.h"
#include "voc_vd_task.h"




//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------

int voc_vd_task_state(struct mtk_snd_voc *voc)
{
	/* sanity check */
	if (!voc) {
		voc_err("snd_voc is NULL\n");
		return -EINVAL;
	}

	return (voc->task_state == true)?true:false;
}


static bool voc_dma_data_ready(struct mtk_snd_voc *voc)
{
	struct voc_pcm_data *pcm_data = NULL;

	/* sanity check */
	if (!voc) {
		voc_err("snd_voc is NULL\n");
		return -EINVAL;
	}

	pcm_data = voc->voc_pcm;

	return (pcm_data->pcm_level_cnt >= pcm_data->pcm_period_size?true:false);
}

static bool voc_dma_xrun(struct mtk_snd_voc *voc)
{
	struct voc_pcm_data *pcm_data = NULL;

	/* sanity check */
	if (!voc) {
		voc_err("snd_voc is NULL\n");
		return -EINVAL;
	}

	pcm_data = voc->voc_pcm;

	return (pcm_data->pcm_status == PCM_XRUN?true:false);
}

static int voc_dma_update_level_cnt(struct mtk_snd_voc *voc,
	uint32_t period_num, uint32_t period_size)
{
	struct voc_pcm_data *pcm_data = voc->voc_pcm;
	uint64_t interval;

	if (pcm_data->pcm_cm4_period == 0) {
		pcm_data->pcm_cm4_period = period_size;
		//g_level_cnt = period_size; //1st frame, period num = 0
	}

	if (period_num <= pcm_data->pcm_last_period_num) {
		voc_err("wrap around last= %u, new %u\n",
		pcm_data->pcm_last_period_num, period_num);
		interval =
			(uint64_t)period_num + MAX_CM4_PERIOD - pcm_data->pcm_last_period_num;
	} else {
		interval = (period_num - pcm_data->pcm_last_period_num);
		//if(interval>1)
	}

	pcm_data->pcm_level_cnt += interval*pcm_data->pcm_cm4_period;

	/* xrun threshold */
	if (pcm_data->pcm_level_cnt
			> pcm_data->pcm_buf_size - pcm_data->pcm_cm4_period*FULL_THRES) {
		voc_err("%s XRUN\n", __func__);
		pcm_data->pcm_status = PCM_XRUN;
		pcm_data->pcm_level_cnt = 0;
		pcm_data->pcm_last_period_num = 0;
	}

	pcm_data->pcm_last_period_num = period_num;
	return pcm_data->pcm_level_cnt;
}
void voc_vd_notify_event(struct mtk_snd_voc *voc, struct snd_pcm_substream *substream)
{
	struct snd_kcontrol *kctl = NULL;
	struct snd_ctl_elem_id elem_id;

	if (voc->sysfs_notify_test > 0) {
		memset(&elem_id, 0, sizeof(elem_id));
		elem_id.numid = voc->sysfs_notify_test;
		kctl = snd_ctl_find_id(substream->pcm->card, &elem_id);
		if (kctl != NULL)
			snd_ctl_notify(substream->pcm->card, SNDRV_CTL_EVENT_MASK_VALUE, &kctl->id);
		voc->sysfs_notify_test = 0;
	}
}

int voc_vd_task_thread(void *param)
{
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)param;

	if (voc == NULL) {
		voc_err("%s Invalid voc structure\n", __func__);
		return 0;
	}

	for (;;) {

		wait_event_interruptible(voc->kthread_waitq,
						kthread_should_stop() || voc->vd_notify);

		if (kthread_should_stop()) {
			voc_err("%s Receive kthread_should_stop\n", __func__);
			break;
		}

		voc->task_state = true;

		if (voc->voc_pcm_running) {
			struct voice_packet *voice_msg =
				(struct voice_packet *)voc_communication_get_voice_packet_addr();
			struct snd_pcm_substream *substream = NULL;
			struct snd_pcm_runtime *runtime = NULL;
			struct voc_pcm_data *pcm_data = NULL;
			struct voc_pcm_runtime_data *prtd = NULL;
			uint32_t period_cnt, period_size, level_cnt;
			uint64_t time_interval;
			unsigned long flag;

			if (voc->voc_pcm == NULL) {
				voc_err("%s Invalid voc pcm structure\n", __func__);
				goto exit_thread;
			}
			if ((voice_msg == NULL) ||
				(((struct data_moving_notify *)voice_msg->payload) == NULL)) {
				voc_err("%s Invalid voice_msg->payload\n", __func__);
				goto exit_thread;
			}
			substream =
				(struct snd_pcm_substream *)voc->voc_pcm->capture_stream;
			runtime = substream->runtime;
			pcm_data = runtime->private_data;

			if (pcm_data->prtd == NULL) {
				voc_err("%s Invalid runtime data structure\n", __func__);
				goto exit_thread;
			}
			voc_vd_notify_event(voc, substream);
			prtd = pcm_data->prtd;

			period_cnt =
				((struct data_moving_notify *)voice_msg->payload)->period_count;
			period_size =
				((struct data_moving_notify *)voice_msg->payload)->byte_count;
			time_interval =
				((struct data_moving_notify *)voice_msg->payload)->time_interval;
			#if SHOW_BUSY_LOG
			voc_debug("period_cnt = %d, period_size = %d, time_interval = %llu\n",
				period_cnt,
				period_size,
				time_interval);
			#endif
			mutex_lock(&voc->pcm_lock);
			level_cnt =
				voc_dma_update_level_cnt(voc, period_cnt, period_size);
			voc->voc_pcm->pcm_time = time_interval;//ms
			mutex_unlock(&voc->pcm_lock);

			voc_debug("updated level count %d\n", level_cnt);

			mutex_lock(&voc->pcm_lock);
			if (prtd->state != DMA_FULL) {
				if (voc_dma_xrun(voc)) {
					prtd->state = DMA_FULL;
					/* Ensure that the DMA done */
					wmb();

					mutex_unlock(&voc->pcm_lock);
					snd_pcm_period_elapsed(substream);
					snd_pcm_stream_lock_irqsave(substream, flag);
					if (snd_pcm_running(substream))
						snd_pcm_stop(substream, SNDRV_PCM_STATE_XRUN);
					snd_pcm_stream_unlock_irqrestore(substream, flag);
					mutex_lock(&voc->pcm_lock);
				} else if ((prtd->state != DMA_OVERRUN) &&
								voc_dma_data_ready(voc)) {
					prtd->state = DMA_OVERRUN;
					/* Ensure that the DMA done */
					wmb();
					mutex_unlock(&voc->pcm_lock);
					snd_pcm_period_elapsed(substream);
					mutex_lock(&voc->pcm_lock);
				}
			}
			mutex_unlock(&voc->pcm_lock);
		}
		voc->vd_notify = 0;
		voc->task_state = false;
	}
exit_thread:
	return 0;

}

