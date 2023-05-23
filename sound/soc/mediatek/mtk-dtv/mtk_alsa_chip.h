/* SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause */
/*
 * mtk_alsa_chip.h  --  Mediatek DTV chip header
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */
#ifndef _MTK_CHIP_HEADER
#define _MTK_CHIP_HEADER

#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/mm.h>

#define	ALSA_WRITE_BYTE(reg, val)	(writeb(val, reg))
#define	ALSA_WRITE_WORD(reg, val)	(writew(val, reg))

#define	ALSA_WRITE_MASK_BYTE(reg, mask, val)			\
	ALSA_WRITE_BYTE(reg, (readb(reg) & ~(mask)) | (val & mask))

#define	ALSA_WRITE_MASK_2BYTE(reg, mask, val)			\
	ALSA_WRITE_WORD(reg, (readw(reg) & ~(mask)) | (val & mask))

#define	IOMAP_NUM		4
#define IOMAP_0     0
#define IOMAP_1     1
#define IOMAP_2     2
#define IOMAP_3     3

void mtk_alsa_delaytask(unsigned int u32Ms);
void mtk_alsa_delaytask_us(unsigned int u32Us);

unsigned char mtk_alsa_read_reg_byte(unsigned int addr);
unsigned short mtk_alsa_read_reg(unsigned int addr);
void mtk_alsa_write_reg_mask_byte(unsigned int addr,
			unsigned char mask, unsigned char val);
void mtk_alsa_write_reg_byte(unsigned int addr, unsigned char val);
void mtk_alsa_write_reg_mask(unsigned int addr,
			unsigned short mask, unsigned short val);
void mtk_alsa_write_reg(unsigned int addr, unsigned short val);
bool mtk_alsa_read_reg_polling(unsigned int addr,
		unsigned short mask, unsigned short val, int counter);
bool mtk_alsa_read_reg_byte_polling(unsigned int addr,
		unsigned char mask, unsigned char val, int counter);

int mtk_i2s_tx_clk_open(void);
int mtk_i2s_tx_clk_close(void);
int mtk_i2s_tx_clk_set_enable(snd_pcm_format_t format, unsigned int rate);
int mtk_i2s_tx_clk_set_disable(void);
int mtk_spdif_tx_clk_set_enable(int format);
int mtk_spdif_tx_clk_set_disable(int format);
int mtk_arc_clk_set_enable(int format);
int mtk_arc_clk_set_disable(int format);
int mtk_earc_clk_set_enable(int format);
int mtk_earc_clk_set_disable(int format);
int mtk_hdmi_rx1_clk_set_enable(void);
int mtk_hdmi_rx1_clk_set_disable(void);
int mtk_hdmi_rx2_clk_set_enable(void);
int mtk_hdmi_rx2_clk_set_disable(void);
int mtk_atv_clk_set_enable(void);
int mtk_atv_clk_set_disable(void);
int mtk_adc0_clk_set_enable(void);
int mtk_adc0_clk_set_disable(void);
int mtk_adc1_clk_set_enable(void);
int mtk_adc1_clk_set_disable(void);
int mtk_i2s_rx2_clk_set_enable(void);
int mtk_i2s_rx2_clk_set_disable(void);
int mtk_src1_clk_set_enable(void);
int mtk_src1_clk_set_disable(void);
int mtk_src2_clk_set_enable(void);
int mtk_src2_clk_set_disable(void);
int mtk_src3_clk_set_enable(void);
int mtk_src3_clk_set_disable(void);
int mtk_alsa_mux_clock_setting(int clk_name, int value);
int mtk_alsa_read_chip(void);
int mtk_alsa_check_atv_input_type(void);
#endif /* _MTK_CHIP_HEADER */
