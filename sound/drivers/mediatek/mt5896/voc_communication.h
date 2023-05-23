/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _VOC_COMMUNICATION_H
#define _VOC_COMMUNICATION_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "voc_common.h"
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
enum {
	COMMUNICATION_PATH_RPMSG = 0,
	COMMUNICATION_PATH_PAGANINI,
	COMMUNICATION_PATH_MAILBOX,
	COMMUNICATION_PATH_STUB,
	COMMUNICATION_PATH_MAX,
};

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------
struct voc_communication_operater {
	int (*bind)(struct mtk_snd_voc *snd_voc);
	int (*dma_start_channel)(void);
	int (*dma_stop_channel)(void);
	int (*dma_init_channel)(uint64_t dma_paddr,
						   uint32_t buf_size,
						   uint32_t channels,
						   uint32_t sample_width,
						   uint32_t sample_rate);
	int (*dma_enable_sinegen)(bool en);
	int (*dma_get_reset_status)(void);
	int (*enable_vq)(bool en);
	int (*config_vq)(uint8_t mode);
	int (*enable_hpf)(int32_t stage);
	int (*config_hpf)(int32_t coeff);
	int (*dmic_gain)(int16_t gain);
	int (*dmic_mute)(bool en);
	int (*dmic_switch)(bool en);
	int (*reset_voice)(void);
	int (*update_snd_model)(dma_addr_t snd_model_paddr, uint32_t size);
	int (*get_hotword_ver)(void);
	int (*get_hotword_identifier)(void);
	int (*enable_slt_test)(int32_t loop, uint32_t addr);
	int (*enable_seamless)(bool en);
	int (*notify_status)(int32_t status);
	int (*power_status)(int32_t status);
	int (*ts_sync_start)(void);
	void *(*get_voice_packet_addr)(void);
};

//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
int voc_communication_bind(struct mtk_snd_voc *snd_voc);
int voc_communication_dma_start_channel(void);
int voc_communication_dma_stop_channel(void);
int voc_communication_dma_init_channel(
		uint32_t dma_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t sample_width,
		uint32_t sample_rate);
int voc_communication_dma_enable_sinegen(bool en);
int voc_communication_dma_get_reset_status(void);
int voc_communication_enable_vq(bool en);
int voc_communication_config_vq(uint8_t mode);
int voc_communication_enable_hpf(int32_t stage);
int voc_communication_config_hpf(int32_t coeff);
int voc_communication_dmic_gain(int16_t gain);
int voc_communication_dmic_mute(bool en);
int voc_communication_dmic_switch(bool en);
int voc_communication_reset_voice(void);
int voc_communication_update_snd_model(dma_addr_t snd_model_paddr, uint32_t size);
int voc_communication_get_hotword_ver(void);
int voc_communication_get_hotword_identifier(void);
int voc_communication_enable_slt_test(int32_t loop, uint32_t addr);
int voc_communication_enable_seamless(bool en);
int voc_communication_notify_status(int32_t status);
int voc_communication_power_status(int32_t status);
int voc_communication_ts_sync_start(void);
void *voc_communication_get_voice_packet_addr(void);
//uint32_t voc_communication_get_path_id(void);
void voc_communication_exit(void);
void voc_communication_init(struct mtk_snd_voc *snd_voc);
#endif /* _VOC_COMMUNICATION_H */
