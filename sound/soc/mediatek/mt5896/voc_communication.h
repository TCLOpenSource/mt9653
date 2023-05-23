/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _VOC_COMMUNICATION_H
#define _VOC_COMMUNICATION_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define PCM_NORMAL 0
#define PCM_XRUN 1
#define MAX_LEN		512
#define MAX_LOG_LEN	30
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------
extern struct list_head voc_soc_card_dev_list;
extern struct list_head voc_pcm_dev_list;
struct operation_log {
	struct timespec tstamp;
	char event[MAX_LEN];
};
struct voc_dma_dbg {
	struct operation_log log[MAX_LOG_LEN];
	int writed;
};
struct voc_pcm_data {
	struct snd_pcm_substream *capture_stream;
	spinlock_t lock;
#if defined(CONFIG_MS_VOC_MMAP_SUPPORT)
	int mmap_mode;
#endif
	struct timer_list timer;
	// void *private_data;
	struct device *dev;
	struct mtk_voc_rpmsg *rpmsg_dev;
	struct completion rpmsg_bind;
	struct list_head list;
	u32 voc_buf_paddr;
	u32 voc_buf_size;
	u32 voc_pcm_offset;
	u32 voc_pcm_size;
	bool voc_pcm_running;
	uint32_t pcm_cm4_period;
	uint32_t pcm_level_cnt;
	uint32_t pcm_last_period_num;
	uint32_t pcm_period_size;
	uint32_t pcm_buf_size;
	uint32_t pcm_status;
	u32 sysfs_notify_test;
	struct voc_dma_dbg dbgInfo;
};

struct voc_soc_card_drvdata {
	struct device *dev;
	struct mtk_voc_rpmsg *rpmsg_dev;
	struct completion rpmsg_bind;
	struct list_head list;
	void *snd_model_array;
	dma_addr_t snd_model_paddr;
	u32 snd_model_offset;
	u32 voc_buf_paddr;
	u32 voc_buf_size;
	bool vbdma_ch;
	u32 *card_reg;
	int32_t status;
};

//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
int _mtk_bind_rpmsg_voc_soc_card(struct mtk_voc_rpmsg *rpmsg_voc_dev,
				struct voc_soc_card_drvdata *voc_soc_card_dev);
int _mtk_bind_rpmsg_voc_pcm(struct mtk_voc_rpmsg *rpmsg_voc_dev,
				struct voc_pcm_data *pcm_data_dev);
int voc_dma_start_channel(struct mtk_voc_rpmsg *voc_rpmsg);
int voc_dma_stop_channel(struct mtk_voc_rpmsg *voc_rpmsg);
int voc_dma_init_channel(
		struct mtk_voc_rpmsg *voc_rpmsg,
		uint32_t dma_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t sample_width,
		uint32_t sample_rate);
int voc_dma_enable_sinegen(struct mtk_voc_rpmsg *voc_rpmsg, bool en);
int voc_enable_vq(struct mtk_voc_rpmsg *voc_rpmsg, bool en);
int voc_config_vq(struct mtk_voc_rpmsg *voc_rpmsg, uint8_t mode);
int voc_enable_hpf(struct mtk_voc_rpmsg *voc_rpmsg, int32_t stage);
int voc_config_hpf(struct mtk_voc_rpmsg *voc_rpmsg, int32_t coeff);
int voc_dmic_number(struct mtk_voc_rpmsg *voc_rpmsg, uint8_t mic);
int voc_dmic_gain(struct mtk_voc_rpmsg *voc_rpmsg, uint16_t gain);
int voc_dmic_bitwidth(struct mtk_voc_rpmsg *voc_rpmsg, uint8_t bitwidth);
int voc_reset_voice(struct mtk_voc_rpmsg *voc_rpmsg);
int voc_update_snd_model(struct mtk_voc_rpmsg *voc_rpmsg,
		dma_addr_t snd_model_paddr, uint32_t size);
int voc_get_hotword_ver(struct mtk_voc_rpmsg *voc_rpmsg);
int voc_enable_slt_test(struct mtk_voc_rpmsg *voc_rpmsg, int32_t loop, uint32_t addr);
unsigned int voc_machine_get_mic_num(struct mtk_voc_rpmsg *voc_rpmsg);
unsigned int voc_machine_get_mic_bitwidth(struct mtk_voc_rpmsg *voc_rpmsg);
#endif /* _VOC_COMMUNICATION_H */
