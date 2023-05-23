/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _VOC_COMMON_H
#define _VOC_COMMON_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "mtk_dbg.h"
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define SHOW_BUSY_LOG  (0)


#define UNUSED(x) (void)(x)

#define PCM_NORMAL      0
#define PCM_XRUN        1
#define MAX_LEN         512
#define MAX_LOG_LEN     30
#define PTP_ARRAY_SIZE  4

#define DMA_EMPTY       0
#define DMA_UNDERRUN    1
#define DMA_OVERRUN     2
#define DMA_FULL        3
#define DMA_NORMAL      4
#define DMA_LOCALFULL   5

//#define VOICE_REQUEST_TIMEOUT (5 * HZ)
#define FULL_THRES 3 //CM4_PERIOD
#define MAX_CM4_PERIOD (1ULL<<32)
#define IPCM_MAX_DATA_SIZE  64
#define VOICE_MAX_DATA_SIZE (IPCM_MAX_DATA_SIZE - sizeof(uint8_t))
#define VOICE_DSP_IDENTIFIER_SIZE 40
#define VOICE_VQ_FORCE_WAKE_ID  3

//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------
enum {
	VOC_REG_MIC_ONOFF = 0,
	VOC_REG_VQ_ONOFF,
	VOC_REG_SEAMLESS_ONOFF,
	VOC_REG_MIC_GAIN,
	VOC_REG_HPF_ONOFF,
	VOC_REG_HPF_CONFIG,
	VOC_REG_SIGEN_ONOFF,
	VOC_REG_SOUND_MODEL,
	VOC_REG_HOTWORD_VER,
	VOC_REG_DSP_PLATFORM_ID,
	VOC_REG_MUTE_SWITCH,
	VOC_REG_LEN,
};

enum {
	VOC_WAKEUP_MODE_OFF = 0,
	VOC_WAKEUP_MODE_NORMAL,
	VOC_WAKEUP_MODE_PM,
	VOC_WAKEUP_MODE_BOTH,
};

enum {
	VOC_POWER_MODE_SHUTDOWN = 0,
	VOC_POWER_MODE_RESUME,
	VOC_POWER_MODE_SUSPEND,
};

enum voice_clock {
	VOC_CLKGEN0 = 0,
	VOC_CLKGEN1
};

struct voice_packet  {
	uint8_t cmd;
	uint8_t payload[VOICE_MAX_DATA_SIZE];
};

struct data_moving_config {
	uint32_t addr;
	uint32_t addr_H;
	uint32_t size;
	uint8_t mic_numbers;
	uint32_t sample_rate;
	uint8_t bitwidth;
};

struct data_moving_enable {
	bool enable;
};

struct data_moving_notify {
	uint32_t period_count;
	uint32_t byte_count;
	uint64_t time_interval;
};

struct operation_log {
	struct timespec tstamp;
	char event[MAX_LEN];
};
struct voc_dma_dbg {
	struct operation_log log[MAX_LOG_LEN];
	unsigned int writed;
};

struct voc_pcm_dma_data {
	unsigned char *name;    /* stream identifier */
	unsigned long channel;  /* Channel ID */
};

struct voc_pcm_runtime_data {
	spinlock_t            lock;
	unsigned int state;
	size_t dma_level_count;
	size_t int_level_count;
	struct voc_pcm_dma_data *dma_data;
	void *private_data;
};

struct voc_pcm_data {
	struct snd_pcm_substream *capture_stream;
#if defined(CONFIG_MS_VOC_MMAP_SUPPORT)
	int mmap_mode;
#endif
	struct timer_list timer;
	struct voc_pcm_runtime_data *prtd;
	int instance;
	uint32_t pcm_cm4_period;
	uint32_t pcm_level_cnt;
	uint32_t pcm_last_period_num;
	uint32_t pcm_period_size;
	uint32_t pcm_buf_size;
	uint32_t pcm_status;
	uint64_t pcm_time;
};

struct voc_card_drvdata {
	void *snd_model_array;
	dma_addr_t snd_model_paddr;
	u32 snd_model_offset;
	u32 voc_buf_paddr;
	u32 voc_buf_size;
	u32 voc_pcm_size;
	bool vbdma_ch;
	u32 *card_reg;
	int32_t status;
	int8_t *uuid_array;
	int32_t uuid_size;
};

struct mtk_snd_voc {
	struct device *dev;
	struct snd_card *card;
	struct snd_pcm *pcm;
	struct completion rpmsg_bind;
	struct list_head list;
	struct voc_card_drvdata *voc_card;
	struct voc_pcm_data *voc_pcm;
	struct voc_dma_dbg dbgInfo;
	uint32_t sysfs_notify_test;
	struct timespec64 suspend_duration_time;
	struct timespec64 resume_duration_time;
	struct task_struct *vd_task;
	wait_queue_head_t kthread_waitq;
	uint32_t communication_path_id;
	bool sysfs_fake_capture;
	bool voc_pcm_running;
	int32_t voc_pcm_status;
	int64_t sync_time;
	int64_t ptp_time[PTP_ARRAY_SIZE];
	u32 miu_base;
	struct mutex pcm_lock;
	bool vd_notify;
	bool task_state;
	u32 power_saving;
};
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
extern struct mtk_snd_voc *g_voc;

void voc_common_clock_set(enum voice_clock clk_bank,
					u32 reg_addr_8bit,
					u16 value,
					u32 start,
					u32 end);
#endif /* _VOC_COMMON_H */
