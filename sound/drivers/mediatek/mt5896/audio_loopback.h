/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _AUDIO_LOOPBACK_H
#define _AUDIO_LOOPBACK_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "audio_mtk_dbg.h"

//------------------------------------------------------------------------------
//  Defines
//------------------------------------------------------------------------------
#define ZERO         (0)
#define BIT0         (1<<0)
#define BIT1         (1<<1)
#define BIT2         (1<<2)
#define BIT3         (1<<3)
#define BIT4         (1<<4)
#define BIT5         (1<<5)
#define BIT6         (1<<6)
#define BIT7         (1<<7)
#define BIT8         (1<<8)
#define BIT9         (1<<9)
#define BIT10        (1<<10)
#define BIT11        (1<<11)
#define BIT12        (1<<12)
#define BIT13        (1<<13)
#define BIT14        (1<<14)
#define BIT15        (1<<15)

#define BANK_MASK               (0xFFFFFF00)
#define WORD_MASK               (0xFFFF)
#define OFFSET_MASK             (0xFF)
#define OFFSET_SHIFT            (1)

#define AEC_CTRL_RESET          (BIT0 | BIT3)

#define AEC_BIT_MASK            (BIT9)
#define AEC_BIT_SHIFT           (9)

#define AEC_CHANNEL_MASK        (BIT0 | BIT1 | BIT2 | BIT7)
#define AEC_CHANNEL_SHIFT       (1)

#define AEC_RATE_MASK           (BIT0 | BIT1 | BIT2 | BIT3)
#define AEC_RATE_SHIFT          (1)

#define AEC_TIMER_ENABLE        (BIT1)
#define AEC_TIMER_CLEAR         (BIT3)

#define AEC_REGEN_BYPASS_MASK   (BIT0 | BIT1)
#define AEC_REGEN_BYPASS_ON     (BIT1)
#define AEC_REGEN_BYPASS_OFF    (ZERO)
#define AEC_REGEN_MODE_MASK     (BIT4 | BIT5 | BIT6 | BIT7)
#define AEC_REGEN_MODE          (BIT6)

#define AEC_SIGEN_MASK          (BIT10)
#define AEC_SIGEN_ON            (BIT10)
#define AEC_SIGEN_OFF           (ZERO)
#define AEC_SIGEN_CFG           (BIT0 | BIT2 | BIT8 | BIT10) //3000Hz for 48Khz, 1000Hz for 16Khz

#define AEC_SIZE_MASK           (WORD_MASK)
#define AEC_SIZE_SHIFT          (4)
#define AEC_ADDR_MASK           (WORD_MASK)
#define AEC_ADDR_SHIFT          (4)
#define AEC_HI_ADDR_SHIFT       (16 + AEC_ADDR_SHIFT)

#define DSP_DMA_RESET           (ZERO)

#define DSP_CHANNEL_MASK        (BIT6 | BIT7)
#define DSP_CHANNEL_2           (ZERO)
#define DSP_CHANNEL_8           (BIT6)
#define DSP_CHANNEL_12          (BIT7)

#define DSP_CTRL_MASK           (BIT0 | BIT1 | BIT2 | BIT3)
#define DSP_FORMAT_MASK         (BIT4 | BIT5)
#define DSP_FORMAT_16BIT        (ZERO)
#define DSP_OUT_ADDR_MASK       (WORD_MASK)
#define DSP_OUT_CTRL_MASK       (WORD_MASK)

#define I2S_FORMAT_MASK         (BIT4)
#define I2S_CLK_MASK            (BIT4)
#define I2S_FORMAT_SHIFT        (4)
#define I2S_CLK_SHIFT           (4)
#define I2S_CFG_SHIFT           (1)

//[11] AEC path on/off
//[12] AEC path from ADC2[1] or internal speaker[0] path
#define MIX_DSP_AEC_PATH_EN_BIT                      (BIT11)
#define MIX_DSP_AEC_FROM_ADC2_EN_BIT                 (12)
#define MIX_DSP_AEC_MASK                             (0xF << MIX_DSP_AEC_FROM_ADC2_EN_BIT)

#define MIX_DSP_AEC_PATH_SEL_SPK_DELAY_SE            (0)      /* AEC ref Path */
#define MIX_DSP_AEC_PATH_SEL_ADC2                    (1)
#define MIX_DSP_AEC_PATH_SEL_ADC1                    (2)
#define MIX_DSP_AEC_PATH_SEL_I2S_RX                  (3)
#define MIX_DSP_AEC_PATH_SEL_DBG_SINE_TONE           (4)
#define MIX_DSP_AEC_PATH_SEL_I2S_RX_4ch              (5)      //SD_0_1
#define MIX_DSP_AEC_PATH_SEL_I2S_RX_8ch              (6)      //SD_0_1_2_3

#define MIX_DSP_AUPLL_SHIFT            (8)
#define MIX_DSP_AUPLL_MASK             (0xFF)
#define MIX_DSP_AUPLL_CHECK            (0xFF)
#define MIX_DSP_AUPLL_EN_ALL           (0xFF)
#define MIX_DSP_AUPLL_RESET_0X00       (0x00)
#define MIX_DSP_AUPLL_RESET_0X03       (0x03)
#define MIX_DSP_AUPLL_RESET_0X0D       (0x0D)
#define MIX_DSP_AUPLL_RESET_0X0F       (0x0F)
#define MIX_DSP_AUPLL_RESET_0X12       (0x12)
#define MIX_DSP_AUPLL_RESET_0X1C       (0x1C)
#define MIX_DSP_AUPLL_RESET_0X20       (0x20)
#define MIX_DSP_AUPLL_RESET_0X7F       (0x7F)
#define MIX_DSP_AUPLL_RESET_0X80       (0x80)
#define MIX_DSP_AUPLL_RESET_0XC0       (0xC0)
#define MIX_DSP_AUPLL_RESET_0XD0       (0xD0)
#define MIX_DSP_REG_AUD01_0X46         (0x950146)
#define MIX_DSP_REG_AUD01_0X47         (0x950147)
#define MIX_DSP_REG_AUD02_0X00         (0x950200)
#define MIX_DSP_REG_AUD02_0X01         (0x950201)
#define MIX_DSP_REG_AUD02_0X9C         (0x95029C)
#define MIX_DSP_REG_AUD02_0X9D         (0x95029D)
#define MIX_DSP_REG_AUD02_0X9E         (0x95029E)
#define MIX_DSP_REG_AUD02_0X9F         (0x95029F)
#define MIX_DSP_REG_AUD02_0X70         (0x950270)
#define MIX_DSP_REG_AUD02_0X71         (0x950271)
#define MIX_DSP_REG_AUD02_0X76         (0x950276)
#define MIX_DSP_REG_AUD02_0X77         (0x950277)

#define TIMEREN_IN_LINE         (3)
#define TIMEREN_REG             (0)
#define TIMEREN_MASK            (1)
#define TIMEREN_VALUE           (2) //index

#define BYTES_IN_MIU_LINE       (16)
#define SWEN_IN_LINE            (3)

#define SWEN_OFFSET             (0)
#define SWEN_START              (1)
#define SWEN_END                (2) //index
#define END_OF_TABLE            (0xFFFFFF)
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------
enum {
	LOOPBACK_REG_AEC_SIGEN_ONOFF = 0,
	LOOPBACK_REG_PCM_SLAVE_ACTIVE_ONOFF,
	LOOPBACK_REG_PCM_SLAVE_FORMAT,
	LOOPBACK_REG_PCM_SLAVE_RATE,
	LOOPBACK_REG_PCM_SLAVE_CHANNELS,
	LOOPBACK_REG_PCM_CAPTURE_SOURCE,
	LOOPBACK_REG_LEN,
};

enum {
	CMD_STOP = 0,
	CMD_START,
	CMD_PAUSE,
	CMD_PAUSE_RELEASE,
	CMD_PREPARE,
	CMD_SUSPEND,
	CMD_RESUME,
};

enum {
	TIMER_WAIT_EVENT,
	TIMER_TRIGGER_EVENT,
	TIMER_STOP_EVENT,
};

struct mem_info {
	u32 miu;
	u64 bus_addr;
	u64 buffer_size;
	u64 memory_bus_base;
};

struct swen_info {
	u32 offset;
	u32 start;
	u32 end;
};

/* Define a STR (Suspend To Ram) data structure */
struct mtk_str_mode_struct {
	ptrdiff_t physical_addr;
	ptrdiff_t bus_addr;
	ptrdiff_t virtual_addr;
};

/* Define a Ring Buffer data structure */
struct mtk_ring_buffer_struct {
	unsigned char *addr;
	unsigned int size;
	unsigned int buffer_level;
	unsigned int sw_w_offset;
	unsigned char *w_ptr;
	unsigned char *r_ptr;
	unsigned char *buf_end;
	unsigned long long total_read_size;
	unsigned long long total_write_size;
	unsigned long update_jiffies;
};

/* Define a Ring Buffer data structure */
struct mtk_copy_buffer_struct {
	unsigned char *addr;
	unsigned int size;
	unsigned int avail_size;
	unsigned int u32RemainSize;
};

/* Define a DMA Reader data structure */
struct mtk_dma_struct {
	struct mtk_ring_buffer_struct r_buf;
	struct mtk_copy_buffer_struct c_buf;
	struct mtk_str_mode_struct str_mode_info;
	unsigned int initialized_status;
	unsigned int high_threshold;
	unsigned int copy_size;
	unsigned int status;
	unsigned int channel_mode;
	unsigned int sample_rate;
	unsigned int sample_bits;
	unsigned int u32TargetAlignmentSize;
	unsigned int sw_loop_status;
	unsigned int sw_loop_stopped;
};

/* Define a Monitor data structure */
struct mtk_monitor_struct {
	unsigned int monitor_status;
	unsigned int expiration_counter;
	snd_pcm_uframes_t last_appl_ptr;
	snd_pcm_uframes_t last_hw_ptr;
};

/* Define a Runtime data structure */
struct mtk_runtime_struct {
	struct snd_pcm_substream *substream;
	struct mtk_monitor_struct monitor;
	struct timer_list timer;
	wait_queue_head_t kthread_waitq;
	struct mutex kthread_lock;
	spinlock_t spin_lock;
	int timer_notify;
};

struct audio_loopback_register {
	u32 RIU_BASE;
	u32 I2S_INPUT_CFG;
	u32 I2S_OUT2_CFG;
	u32 AEC_SRC_CTRL;
	u32 AEC_SRC_SCFG;
	u32 AEC_SRC_MCRL;
	u32 AEC_SRC_MCFG;
	u32 AEC_DMA_CTRL;
	u32 AEC_DMA_RGEN;
	u32 AEC_DMA_RPTR;
	u32 AEC_DMA_WPTR;
	u32 AEC_DMA_ARLO;
	u32 AEC_DMA_ARHI;
	u32 AEC_DMA_ARZE;
	u32 AEC_DMA_PAGA;
	u32 AEC_DMA_RCRL;
	u32 AEC_DMA_TCRL;
	u32 AEC_DMA_SINE;
	u32 AEC_DMA_SCFG;
	u32 AEC_BANK_NUM;
	u32 OUT_DMA_CTRL;
	u32 OUT_DMA_WPTR;
	u32 OUT_DMA_LVEL;
	u32 DSP_DMA_CTRL;
	u32 DSP_DMA_RPTR;
	u32 DSP_DMA_WPTR;
	u32 DSP_BANK_NUM;
	u32 SLT_DMA_CTRL;
	u32 *AEC_REG_BANK;
	u32 *DSP_REG_BANK;
	void __iomem **AEC_REG_BASE;
	void __iomem **DSP_REG_BASE;
};

struct mtk_snd_loopback {
	struct device *dev;
	struct snd_card *card;
	struct snd_pcm *pcm;
	struct mem_info aec_mem_info;
	struct mem_info dsp_mem_info;
	struct mem_info dsp_pb_mem_info;
	struct mtk_dma_struct aec_dma_buf;
	struct mtk_dma_struct dsp_dma_buf;
	struct mtk_dma_struct dsp_pb_dma_buf;
	struct mtk_copy_buffer_struct capture_buf;
	struct mtk_runtime_struct dev_runtime;
	struct mtk_runtime_struct dev_play_runtime;
	struct task_struct *play_task;
	struct task_struct *capture_task;
	struct timespec pcm_capture_ts;
	struct mutex loopback_pcm_lock;
	struct list_head list;

	uint32_t slt_test_config;
	int slt_test_ctrl_bit;
	int dsp_mixing_mode;
	int dsp_capture_source;
	int i2s_input_format;
	int i2s_input_channel;
	int i2s_out_clk;
	int aec_max_channel;
	int dsp_max_channel;
	int dsp_channels_num;
	int *dsp_channel_list;
	int dsp_dma_irq;
	int aec_dma_irq;

	void *aec_dma_addr;
	void *aec_dma_paddr;
	u32 aec_dma_offset;
	u32 aec_dma_size;
	u32 dsp_dma_offset;
	u32 dsp_dma_size;
	u32 dsp_pb_dma_offset;
	u32 dsp_pb_dma_size;

	//capture case
	bool loopback_aec_enabled;
	bool loopback_aec_bypass;
	bool loopback_aec_regen_bypass;
	bool loopback_i2s_enabled;

	//pcm status
	bool loopback_pcm_running;
	bool loopback_pcm_playback_running;
};

#endif /* _AUDIO_LOOPBACK_H */
