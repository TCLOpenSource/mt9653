/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _VOC_LINK_RPMSG_H
#define _VOC_LINK_RPMSG_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "voc_common.h"
#include "voc_communication.h"
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------
extern struct list_head voc_card_dev_list;
//extern struct list_head voc_pcm_dev_list;
//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
//int _mtk_bind_rpmsg_voc_card(struct mtk_voc_rpmsg *rpmsg_voc_dev);
int voc_link_rpmsg_bind(struct mtk_snd_voc *snd_voc);
int voc_dma_start_channel(void);
int voc_dma_stop_channel(void);
int voc_dma_init_channel(
		uint64_t dma_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t sample_width,
		uint32_t sample_rate);
int voc_dma_enable_sinegen(bool en);
int voc_dma_get_reset_status(void);
int voc_enable_vq(bool en);
int voc_config_vq(uint8_t mode);
int voc_enable_hpf(int32_t stage);
int voc_config_hpf(int32_t coeff);
int voc_dmic_gain(int16_t gain);
int voc_dmic_mute(bool en);
// TODO: rename voc_dmic_switch()
//       to distinguish voc_dmic_switch() from voc_dmic_mute()
int voc_dmic_switch(bool en);
int voc_reset_voice(void);
int voc_update_snd_model(dma_addr_t snd_model_paddr, uint32_t size);
int voc_get_hotword_ver(void);
int voc_get_hotword_identifier(void);
int voc_enable_slt_test(int32_t loop, uint32_t addr);
int voc_enable_seamless(bool en);
int voc_notify_status(int32_t status);
int voc_ts_sync_start(void);
int voc_ts_delay_req(void);
struct voc_communication_operater *voc_drv_rpmsg_get_op(void);
void *voc_drv_rpmsg_get_voice_packet_addr(void);
void voc_drv_rpmsg_init(void);
#endif /* _VOC_LINK_RPMSG_H */
