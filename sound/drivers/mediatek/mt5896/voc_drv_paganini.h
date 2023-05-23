/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _VOC_DRV_PAGANINI_H
#define _VOC_DRV_PAGANINI_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "voc_common.h"
#include "voc_communication.h"
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#ifndef U32_MAX
	#define U32_MAX (0xFFFFFFFF)
#endif
#ifndef U16_MAX
	#define U16_MAX (0xFFFF)
#endif

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
int voc_drv_paganini_bind(struct mtk_snd_voc *snd_voc);
int voc_drv_paganini_vd_task_state(void);
int voc_drv_paganini_dma_start_channel(void);
int voc_drv_paganini_dma_stop_channel(void);
int voc_drv_paganini_dma_init_channel(
		uint64_t cpu_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t bit_width,
		uint32_t sample_rate);
int voc_drv_paganini_dma_enable_sinegen(bool en);
int voc_drv_paganini_dma_get_reset_status(void);
int voc_drv_paganini_enable_vq(bool en);
int voc_drv_paganini_config_vq(uint8_t mode);
int voc_drv_paganini_enable_hpf(int32_t stage);
int voc_drv_paganini_config_hpf(int32_t coeff);
int voc_drv_paganini_dmic_gain(int16_t gain);
int voc_drv_paganini_dmic_mute(bool en);
int voc_drv_paganini_dmic_switch(bool en);
int voc_drv_paganini_reset_voice(void);
int voc_drv_paganini_update_snd_model(dma_addr_t snd_model_paddr, uint32_t size);
int voc_drv_paganini_get_hotword_ver(void);
int voc_drv_paganini_get_hotword_identifier(void);
int voc_drv_paganini_enable_slt_test(int32_t loop, uint32_t addr);
int voc_drv_paganini_enable_seamless(bool en);
int voc_drv_paganini_notify_status(int32_t status);
int voc_drv_paganini_ts_sync_start(void);
struct voc_communication_operater *voc_drv_paganini_get_op(void);
void voc_drv_paganini_exit(void);
void voc_drv_paganini_init(void);
#endif /* _VOC_DRV_PAGANINI_H */
