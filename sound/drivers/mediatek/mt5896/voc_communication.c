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
#include "voc_communication.h"
#include "voc_drv_paganini.h"
#include "voc_drv_rpmsg.h"
#include "voc_drv_mailbox.h"
#include "voc_drv_stub.h"



//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Variables
struct mtk_snd_voc *g_voc;
static struct voc_communication_operater *voc_communication_op;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------

/*
 * VOC chip device and rpmsg device binder
 */
int voc_communication_bind(struct mtk_snd_voc *snd_voc)
{
	if ((!voc_communication_op) || (!voc_communication_op->bind))
		return -EINVAL;

	return voc_communication_op->bind(snd_voc);

}

//CM4 notification interval is based on the fixed frame size(16kHz,16bits),
//and it's irrelevant to bitwidth and sample rate.
//So we should update notification frequency
//in case the notification is frequent.
int voc_communication_dma_init_channel(uint32_t dma_paddr,
									   uint32_t buf_size,
									   uint32_t channels,
									   uint32_t sample_width,
									   uint32_t sample_rate)
{
	if ((!voc_communication_op) || (!voc_communication_op->dma_init_channel))
		return -EINVAL;


	return voc_communication_op->dma_init_channel(
									dma_paddr,
									buf_size,
									channels,
									sample_width,
									sample_rate);
}

int voc_communication_dma_start_channel(void)
{
	if ((!voc_communication_op) ||
		(!voc_communication_op->dma_start_channel))
		return -EINVAL;

	return voc_communication_op->dma_start_channel();
}

int voc_communication_dma_stop_channel(void)
{
	if ((!voc_communication_op) ||
		(!voc_communication_op->dma_stop_channel))
		return -EINVAL;

	return voc_communication_op->dma_stop_channel();
}

int voc_communication_dma_enable_sinegen(bool en)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->dma_enable_sinegen))
		return -EINVAL;

	return voc_communication_op->dma_enable_sinegen(en);
}

int voc_communication_enable_vq(bool en)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->enable_vq))
		return -EINVAL;

	return voc_communication_op->enable_vq(en);
}

int voc_communication_config_vq(uint8_t mode)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->config_vq))
		return -EINVAL;

	return voc_communication_op->config_vq(mode);
}

int voc_communication_enable_hpf(int32_t stage)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->enable_hpf))
		return -EINVAL;

	return voc_communication_op->enable_hpf(stage);
}

int voc_communication_config_hpf(int32_t coeff)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->config_hpf))
		return -EINVAL;

	return voc_communication_op->config_hpf(coeff);
}

int voc_communication_dmic_gain(int16_t gain)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->dmic_gain))
		return -EINVAL;

	return voc_communication_op->dmic_gain(gain);
}

int voc_communication_dmic_mute(bool en)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->dmic_mute))
		return -EINVAL;

	return voc_communication_op->dmic_mute(en);
}

int voc_communication_dmic_switch(bool en)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->dmic_switch))
		return -EINVAL;

	return voc_communication_op->dmic_switch(en);

}

int voc_communication_reset_voice(void)
{
	if ((!voc_communication_op) || (!voc_communication_op->reset_voice))
		return -EINVAL;

	return voc_communication_op->reset_voice();
}


int voc_communication_update_snd_model(dma_addr_t snd_model_paddr,
									   uint32_t size)
{
	if ((!voc_communication_op) || (!voc_communication_op->update_snd_model))
		return -EINVAL;

	return voc_communication_op->update_snd_model(snd_model_paddr, size);
}

int voc_communication_get_hotword_ver(void)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->get_hotword_ver))
		return -EINVAL;

	return voc_communication_op->get_hotword_ver();
}

int voc_communication_get_hotword_identifier(void)
{
	if ((!voc_communication_op) || (!voc_communication_op->get_hotword_identifier))
		return -EINVAL;

	return voc_communication_op->get_hotword_identifier();
}

int voc_communication_enable_slt_test(int32_t loop, uint32_t addr)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture) {
		g_voc->voc_card->status = 1;
		return 0;
	}

	if ((!voc_communication_op) || (!voc_communication_op->enable_slt_test))
		return -EINVAL;

	return voc_communication_op->enable_slt_test(loop, addr);
}

int voc_communication_ts_sync_start(void)
{
	if ((!voc_communication_op) || (!voc_communication_op->ts_sync_start))
		return -EINVAL;

	return voc_communication_op->ts_sync_start();
}

int voc_communication_enable_seamless(bool en)
{
	if (!g_voc)
		return -EINVAL;
	else if (g_voc->sysfs_fake_capture)
		return 0;

	if ((!voc_communication_op) || (!voc_communication_op->enable_seamless))
		return -EINVAL;

	return voc_communication_op->enable_seamless(en);
}

int voc_communication_notify_status(int32_t status)
{
	if ((!voc_communication_op) || (!voc_communication_op->notify_status))
		return -EINVAL;

	return voc_communication_op->notify_status(status);
}

int voc_communication_power_status(int32_t status)
{
	if ((!voc_communication_op) || (!voc_communication_op->power_status))
		return -EINVAL;

	return voc_communication_op->power_status(status);
}

int voc_communication_dma_get_reset_status(void)
{
	if ((!voc_communication_op) || (!voc_communication_op->dma_get_reset_status))
		return -EINVAL;

	return voc_communication_op->dma_get_reset_status();
}

void *voc_communication_get_voice_packet_addr(void)
{
	if ((!voc_communication_op) || (!voc_communication_op->get_voice_packet_addr))
		return NULL;

	return voc_communication_op->get_voice_packet_addr();
}

void voc_communication_exit(void)
{
	if (!g_voc) {
		voc_err("[%s]:g_voc is NULL\n", __func__);
		return;
	}

	switch (g_voc->communication_path_id) {
	case COMMUNICATION_PATH_RPMSG:
		//voc_drv_rpmsg_exit();
		break;
	case COMMUNICATION_PATH_PAGANINI:
		voc_drv_paganini_exit();
		break;
	case COMMUNICATION_PATH_MAILBOX:
		//voc_drv_mailbox_exit();
		break;
	case COMMUNICATION_PATH_STUB:
		voc_drv_stub_exit();
		break;
	default:
		//voc_drv_rpmsg_exit();
		break;
	}
}

void voc_communication_init(struct mtk_snd_voc *snd_voc)
{
	voc_info("[%s]\n", __func__);
	g_voc = snd_voc;

	if (!g_voc) {
		voc_err("[%s]:g_voc is NULL\n", __func__);
		return;
	}

	switch (g_voc->communication_path_id) {
	case COMMUNICATION_PATH_RPMSG:
		voc_communication_op = voc_drv_rpmsg_get_op();
		voc_drv_rpmsg_init();
		break;
	case COMMUNICATION_PATH_PAGANINI:
		voc_communication_op = voc_drv_paganini_get_op();
		voc_drv_paganini_init();
		break;
	case COMMUNICATION_PATH_MAILBOX:
		voc_communication_op = voc_drv_mailbox_get_op();
		voc_drv_mailbox_init();
		break;
	case COMMUNICATION_PATH_STUB:
		voc_communication_op = voc_drv_stub_get_op();
		voc_drv_stub_init();
		break;
	default:
		voc_communication_op = voc_drv_rpmsg_get_op();
		voc_drv_rpmsg_init();
	}
}
