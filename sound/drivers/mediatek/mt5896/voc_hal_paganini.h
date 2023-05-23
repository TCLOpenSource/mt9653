/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _VOC_HAL_PAGANINI_H
#define _VOC_HAL_PAGANINI_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "voc_common.h"
#include "voc_common_reg.h"
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#ifdef CHIP_MT5896
#undef CHIP_MT5896
#define CHIP_MT5896 0x101
#else
#define CHIP_MT5896 0x101
#endif

#ifdef CHIP_MT5897
#undef CHIP_MT5897
#define CHIP_MT5897 0x109
#else
#define CHIP_MT5897 0x109
#endif

#ifdef CHIP_MT5876
#undef CHIP_MT5876
#define CHIP_MT5876 0x10C
#else
#define CHIP_MT5876 0x10C
#endif
#define REG_PAGA_PLL_DIV_48 0xe1

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef BOOL
#define BOOL unsigned int
#endif
#ifndef BYTE
#define BYTE unsigned char
#endif
#ifndef WORD
#define WORD unsigned short
#endif
#ifndef DWORD
#define DWORD unsigned int
#endif
#ifndef U8
#define U8 unsigned char
#endif
#ifndef U16
#define U16 unsigned short
#endif
#ifndef U32
#define U32 unsigned int
#endif
#ifndef S32
#define S32 int
#endif

//cfg0
#define VOICE_REG_SW_RST_DMA                    (1 << 15)
#define VOICE_REG_WR_BIT_MODE                   (1 << 9)
#define VOICE_REG_WR_FULL_FLAG_CLR              (1 << 7)
#define VOICE_REG_WR_LEVEL_CNT_MASK             (1 << 5)
#define VOICE_REG_WR_TRIG                       (1 << 4)
#define VOICE_REG_WR_SWAP                       (1 << 3)
#define VOICE_REG_WR_FREERUN                    (1 << 2)
#define VOICE_REG_WR_ENABLE                     (1 << 1)
#define VOICE_REG_MIU_REQUSTER_ENABLE           (1 << 0)

//cfg1
#define VOICE_REG_WR_BASE_ADDR_LO_POS           (0)
#define VOICE_REG_WR_BASE_ADDR_LO_MSK           (0xFFFF << VOICE_REG_WR_BASE_ADDR_LO_POS)

//cfg2
#define VOICE_REG_WR_BASE_ADDR_HI_OFFSET        (16)
#define VOICE_REG_WR_BASE_ADDR_HI_POS           (0)
#define VOICE_REG_WR_BASE_ADDR_HI_MSK           (0x0FFF << VOICE_REG_WR_BASE_ADDR_HI_POS)

//cfg3
#define VOICE_REG_WR_BUFF_SIZE_POS              (0)
#define VOICE_REG_WR_BUFF_SIZE_MSK              (0xFFFF << VOICE_REG_WR_BUFF_SIZE_POS)

//cfg4
#define VOICE_REG_WR_SIZE_POS                   (0)
#define VOICE_REG_WR_SIZE_MSK                   (0xFFFF << VOICE_REG_WR_SIZE_POS)

//cfg5
#define VOICE_REG_WR_OVERRUN_TH_POS             (0)
#define VOICE_REG_WR_OVERRUN_TH_MSK             (0xFFFF << VOICE_REG_WR_OVERRUN_TH_POS)

//cfg8
#define VOICE_REG_WR_LOCAL_FULL_INT_EN          (1 << 2)
#define VOICE_REG_WR_OVERRUN_INT_EN             (1 << 1)
#define VOICE_REG_WR_FULL_INT_EN                (1 << 0)

//sts1
#define VOICE_REG_WR_LOCALBUF_FULL_STATUS       (1 << 9)
#define VOICE_REG_WR_FULL_STATUS                (1 << 8)
#define VOICE_REG_WR_OVERRUN_FLAG               (1 << 1)
#define VOICE_REG_WR_FULL_FLAG                  (1 << 0)

//timer2
#define VOICE_REG_WR_TIMER2_INTERRUPT_BIT       (1 << 8)
#define VOICE_REG_WR_TIMER2_ONCE_BIT            (1 << 1)
#define VOICE_REG_WR_TIMER2_ROLL_BIT            (1 << 0)

//fiq
#define VOICE_REG_WR_FIQ_STATUS_TIMER2_BIT      (1 << 6)

#define VOICE_DMA_W1_CFG_0                      (0x60)
#define VOICE_DMA_W1_CFG_1                      (0x62)
#define VOICE_DMA_W1_CFG_2                      (0x64)
#define VOICE_DMA_W1_CFG_3                      (0x66)
#define VOICE_DMA_W1_CFG_4                      (0x68)
#define VOICE_DMA_W1_CFG_5                      (0x6A)
#define VOICE_DMA_W1_CFG_6                      (0x6C)
#define VOICE_DMA_W1_CFG_7                      (0x6E)
#define VOICE_DMA_W1_CFG_8                      (0x70)
#define VOICE_DMA_W1_CFG_9                      (0x72)
#define VOICE_DMA_W1_STS_0                      (0x74)
#define VOICE_DMA_W1_STS_1                      (0x76)
#define VOICE_DMA_W1_STS_2                      (0x78)
#define VOICE_DMA_W1_TST_0                      (0x7A)
#define VOICE_DMA_W1_TST_1                      (0x7C)
#define VOICE_DMA_W1_TST_2                      (0x7E)

#define VOICE_TIMER2_CTRL_OFFSET                (0x00)
#define VOICE_TIMER2_COUNT_MAX_L_OFFSET         (0x08)
#define VOICE_TIMER2_COUNT_MAX_H_OFFSET         (0x0C)
#define VOICE_FIQ_STATUS_OFFSET                 (0xB0)

#define VOICE_MIU_WORD_BYTE_SIZE                (0x10)
#define VOICE_DMA_INTERNAL_FIFO_SIZE            (8)

//HPF
#define VOICE_EN_HPF_2STAGE                     (1<<1)
#define VOICE_BYP_HPF                           (1<<0)
#define VOICE_HPF_N_POS                         (8)
#define VOICE_HPF_N_MSK                         (0xF << VOICE_HPF_N_POS)
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------
#define VOC_CHANNEL_2             (2)
#define VOC_CHANNEL_4             (4)
#define VOC_CHANNEL_6             (6)
#define VOC_CHANNEL_8             (8)
#define VOC_SAMPLE_RATE_8K     (8000)
#define VOC_SAMPLE_RATE_16K   (16000)
#define VOC_SAMPLE_RATE_32K   (32000)
#define VOC_SAMPLE_RATE_48K   (48000)
#define VOC_BIT_WIDTH_16         (16)
#define VOC_BIT_WIDTH_24         (24)
#define VOC_BIT_WIDTH_32         (32)

#define VOC_GAIM_MAX            (911)
#define VOC_GAIN_MIN           (-480)

#define VOC_BCK_MODE_MIN         (0)
#define VOC_BCK_MODE_MAX         (10)
#define VOC_DEFAULT_BCK_MODE     (9) //3M
// Mode  Bck(kHz)
// 0     Disable Voice(SW defined)
// 1     100
// 2     400
// 3     800
// 4     1000
// 5     1200
// 6     1600
// 7     2000
// 8     2400
// 9     3000
// 10    4000

struct mtk_voice_reg_t {
	phys_addr_t paganini;  //0x432
	phys_addr_t pm_sleep;
	phys_addr_t pm_top;
	phys_addr_t pm_misc;
	phys_addr_t cpu_int;
	phys_addr_t inturrpt;
	phys_addr_t viv_bdma;
	phys_addr_t pm_pmux;   //0x110
	phys_addr_t block_arbiter_gp1;
	phys_addr_t vivaldi2;
	phys_addr_t MALIBOX;
	phys_addr_t ckgen00_pm;//0x0c
	phys_addr_t ckgen01_pm;//0x0d
	phys_addr_t vad_0;     //0x430
	phys_addr_t vrec;      //0x431
	phys_addr_t coin_cpu;
	phys_addr_t vad_1;
};

enum voice_dma_intclr {
	voice_dma_intclr_wr_full = 0,
	voice_dma_intclr_rd_empty,
};
//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
extern int mtk_pinctrl_select(const char *name);
//------------------------------------------------------------------------------

int voc_hal_get_chip_id(void);
int voc_hal_paganini_set_sample_rate(u32 u32SampleRate, u16 bck_mode, u32 mute_enable);
int voc_hal_paganini_set_channel(uint8_t u8Channel);
int voc_hal_paganini_set_bit_width(uint8_t u8BitWidth);
uint32_t voc_hal_paganini_get_sample_rate(void);
uint8_t voc_hal_paganini_get_channel(void);
uint8_t voc_hal_paganini_get_bit_width(void);
uint32_t voc_hal_paganini_dma_wr_level_cnt(void);
uint32_t voc_hal_paganini_read_dma_data(uint8_t *dst_buf, uint32_t buf_size);
void voc_hal_paganini_clear_dma_interrupt(enum voice_dma_intclr intclr);
void voc_hal_paganini_get_dma_interrupt_status(int *full, int *overrun);
void voc_hal_paganini_set_dma_buf_overrun_thres(uint32_t overrun_thres);

void voc_hal_paganini_enable_vdma_timer_interrupt(bool en);
void voc_hal_paganini_clear_vdma_timer_interrupt(void);
uint16_t voc_hal_paganini_get_vdma_timer_interrupt_status(void);
void voc_hal_paganini_dma_reset(void);
void voc_hal_paganini_dma_enable_interrupt(int full, int overrun);
int voc_hal_paganini_dma_enable_incgen(bool en);
void voc_hal_paganini_set_dma_buf_addr(uint32_t u32addr, uint32_t size);
void voc_hal_paganini_set_virtual_buf_addr(void *pAddr, uint32_t size);
void voc_hal_paganini_clean_level_count(void);

int voc_hal_paganini_dma_start_channel(void);
int voc_hal_paganini_dma_stop_channel(void);
int voc_hal_paganini_dma_enable_sinegen(bool en);
int voc_hal_paganini_dma_get_reset_status(void);
int voc_hal_paganini_enable_vq(bool en);
int voc_hal_paganini_config_vq(uint8_t mode);
int voc_hal_paganini_enable_hpf(int32_t stage);
int voc_hal_paganini_config_hpf(int32_t coeff);
int voc_hal_paganini_dmic_gain(int16_t gain);
int voc_hal_paganini_dmic_mute(bool en, u16 bck_mode);
int voc_hal_paganini_dmic_switch(bool en, u16 bck_mode, u32 mute_enable);
int voc_hal_paganini_reset_voice(void);
int voc_hal_paganini_update_snd_model(dma_addr_t snd_model_paddr, uint32_t size);
int voc_hal_paganini_get_hotword_ver(void);
int voc_hal_paganini_get_hotword_identifier(void);
int voc_hal_paganini_enable_seamless(bool en);
int voc_hal_paganini_notify_status(int32_t status);
int voc_hal_paganini_ts_sync_start(void);
int voc_hal_paganini_init(void);
#endif /* _VOC_HAL_PAGANINI_H */
