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
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
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
#include "voc_hal_paganini.h"


//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define ENABLE_CLOCK_FRAMEWORK    1
#define ENABLE_PINCTRL_FRAMEWORK  0

#define VOC_SIN_FREQ_SEL		  0x2021 //0x5021
#define VOC_AUDMA_W1_CFG00_VALUE  0x020B
#define VOC_VREC_CTRl06_VALUE	  0x011F
#define VOC_MAX_CHANNEL           VOC_CHANNEL_4
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------
enum voice_state {
	VOICE_STATE_NONE,
	VOICE_STATE_INIT,
	VOICE_STATE_START,
	VOICE_STATE_STOP,
	VOICE_STATE_XRUN,
	VOICE_STATE_MAX
};
//------------------------------------------------------------------------------
//	Global Variables
//------------------------------------------------------------------------------
struct mtk_voice_reg_t g_voc_reg;
//------------------------------------------------------------------------------
//	Local Variables
//------------------------------------------------------------------------------
//static enum voice_state voc_state = VOICE_STATE_NONE;
static uint8_t mic_numbers  = VOC_CHANNEL_4;
static uint8_t bit_width    = VOC_BIT_WIDTH_16;
static uint32_t sample_rate = VOC_SAMPLE_RATE_16K;
static uint32_t g_pad_init;

static uint8_t *dma_buf_addr;
static uint8_t *dma_wr_ptr;
static uint32_t dma_buf_size;
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
static void voc_hal_paganini_pad_init(void)
{
	#define DMIC_PAD_20    0x0111
	#define DMIC_PAD_MASK  0x0FFF
	#if ENABLE_PINCTRL_FRAMEWORK
	int ret;
	#endif

	if (g_pad_init)
		return;
	g_pad_init = 1;

	#if ENABLE_PINCTRL_FRAMEWORK
	ret = mtk_pinctrl_select("dmic_record_pmux");
	if (ret != 0)
		voc_info("set dmic_record_pmux fail: %d\n", ret);
	else
		voc_info("set dmic_record_pmux success: %d\n", ret);

	#else
	INSREG16((g_voc_reg.pm_pmux)|(OFFSET(VOC_REG_0X20)), DMIC_PAD_MASK, DMIC_PAD_20);
	#endif

}
//==============================================================================
int voc_hal_get_chip_id(void)
{
	int chip_id;

	//0x00108_00[15:0] 16-bit offset chip id
	//0x00108_01[15:8] 16-bit offset chip version
	chip_id = INREG16((g_voc_reg.pm_top)|(OFFSET8(VOC_REG_0X00)));
	//voc_info("%s: Chip ID:%X\n", __func__, chip_id);

	return chip_id;
}

//==============================================================================
int voc_hal_paganini_dma_start_channel(void)
{
	voc_hal_paganini_pad_init(); //temporary

	INSREG8(g_voc_reg.vrec	   + OFFSET8(VOC_REG_0X84), 0x01, 0x01); // VDMA & AEC Common enable
	INSREG8(g_voc_reg.vrec	   + OFFSET8(VOC_REG_0X60), BIT1, BIT1); // VDMA En
	INSREG8(g_voc_reg.paganini + OFFSET8(VOC_REG_0X24), 0x01, 0x01); // VDMA_VLD_SEL
	INSREG8(g_voc_reg.vrec	   + OFFSET8(VOC_REG_0X02), BIT3, 0x00); // VDMA Pause
	return 0;
}
//==============================================================================
int voc_hal_paganini_dma_stop_channel(void)
{
	INSREG8(g_voc_reg.vrec     + OFFSET8(VOC_REG_0X84), 0x01, 0x00); // VDMA & AEC Common enable
	INSREG8(g_voc_reg.vrec     + OFFSET8(VOC_REG_0X60), BIT1, 0x00); // VDMA En
	INSREG8(g_voc_reg.vrec     + OFFSET8(VOC_REG_0X02), BIT3, BIT3); // VDMA Pause
	return 0;
}
//==============================================================================
int voc_hal_paganini_dma_enable_sinegen(bool en)
{
	uint16_t u16Value;

	voc_info("[%s]\n", __func__);
	//[cm4]if (voc_state != VOICE_STATE_NONE)
	//[cm4]	return 0;
	if (en)
		u16Value = VOC_SIN_FREQ_SEL;
	else
		u16Value = 0;

	OUTREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0X06), u16Value); //[15:12] SIN_FREQ_SEL
	return 0;
}
//==============================================================================
int voc_hal_paganini_dma_enable_incgen(bool en)
{
	uint8_t u8Val1 = 0;
	uint8_t u8Val2 = 0;

	voc_info("[%s]\n", __func__);
	if (en) {
		u8Val1 = 0x01;
		u8Val2 = VOC_BIT_0XFF;
	}

	INSREG8(g_voc_reg.vrec + OFFSET8(VOC_REG_0X06), VOC_REG_0X30, 0x00);
	INSREG8(g_voc_reg.vrec + OFFSET8(VOC_REG_0XA1), VOC_REG_0X01, u8Val1);
	OUTREG8(g_voc_reg.vrec + OFFSET8(VOC_REG_0X15), u8Val2);
	INSREG8(g_voc_reg.vrec + OFFSET8(VOC_REG_0X11), VOC_REG_0X08, 0x00);
	return 0;
}
//==============================================================================
uint8_t voc_hal_paganini_get_channel(void)
{
	return mic_numbers;
}

//==============================================================================
int voc_hal_paganini_set_channel(uint8_t u8Channel)
{
	uint16_t chn = 0;

	voc_info("[%s]\n", __func__);
	switch (u8Channel) {
	case VOC_CHANNEL_2:
		chn = VOC_VAL_0;
		break;
	case VOC_CHANNEL_4:
		chn = VOC_VAL_1;
		break;
	#if (VOC_MAX_CHANNEL >= VOC_CHANNEL_6)
	case VOC_CHANNEL_6:
		chn = VOC_VAL_2;
		break;
	#endif
	#if (VOC_MAX_CHANNEL >= VOC_CHANNEL_8)
	case VOC_CHANNEL_8:
		chn = VOC_VAL_3;
		break;
	#endif
	default:
		voc_err("%s failed\n", __func__);
		return -EINVAL;
	}
	INSREG8(g_voc_reg.vrec + OFFSET8(VOC_REG_0X80), VOC_REG_0X07, chn);
	mic_numbers = u8Channel;

	return 0;
}
//==============================================================================
uint32_t voc_hal_paganini_get_sample_rate(void)
{
	return sample_rate;
}
//==============================================================================
int voc_hal_paganini_set_sample_rate(u32 u32SampleRate, u16 bck_mode, u32 mute_enable)
{
	u16 pdm_mode = 0;

	switch (u32SampleRate) {
	case VOC_SAMPLE_RATE_8K:
		pdm_mode = VOC_VAL_0;
		break;
	case VOC_SAMPLE_RATE_16K:
		pdm_mode = VOC_VAL_1;
		break;
	case VOC_SAMPLE_RATE_32K:
		pdm_mode = VOC_VAL_2;
		break;
	case VOC_SAMPLE_RATE_48K:
		pdm_mode = VOC_VAL_3;
		break;
	default:
		voc_err("%s failed\n", __func__);
		return -EINVAL;
	}

	pdm_mode <<= VOC_VAL_4;

	if (mute_enable == 0)
		pdm_mode += bck_mode;

	OUTREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0X08), pdm_mode);
	sample_rate = u32SampleRate;

	return 0;
}
//==============================================================================
uint8_t voc_hal_paganini_get_bit_width(void)
{
	return bit_width;
}
//==============================================================================
int voc_hal_paganini_set_bit_width(uint8_t u8BitWidth)
{
	uint16_t sel;

	voc_info("[%s]\n", __func__);
	switch (u8BitWidth) {
	case VOC_BIT_WIDTH_16:
		sel = 0;
		break;
	case VOC_BIT_WIDTH_24:
	case VOC_BIT_WIDTH_32:
		sel = VOICE_REG_WR_BIT_MODE;
		break;
	default:
		voc_err("%s failed\n", __func__);
		return -EINVAL;
	}
	INSREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0X60), VOICE_REG_WR_BIT_MODE, sel);
	bit_width = u8BitWidth;

	return 0;
}

//==============================================================================
int voc_hal_paganini_enable_vq(bool en)
{
	UNUSED(en);
	return 0;
}
//==============================================================================
int voc_hal_paganini_config_vq(uint8_t mode)
{
	UNUSED(mode);
	return 0;
}
//==============================================================================
int voc_hal_paganini_enable_hpf(int32_t stage)
{
	uint16_t config_value;

	config_value = INREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0XA6));

	if (stage == 0)
		config_value |= VOICE_BYP_HPF;
	else if (stage == 1)
		config_value &= ~(VOICE_BYP_HPF | VOICE_EN_HPF_2STAGE);
	else if (stage == VOC_VAL_2) {
		config_value &= ~VOICE_BYP_HPF;
		config_value |= VOICE_EN_HPF_2STAGE;
	} else
		return -EINVAL;

	OUTREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0XA6), config_value);

	return 0;
}
//==============================================================================
int voc_hal_paganini_config_hpf(int32_t coeff)
{
	#define VOC_HPF_COEF_RANGE_MAX   (9)
	#define VOC_HPF_COEF_RANGE_MIN  (-2)
	#define VOC_HPF_COEF_RANGE_BASE  (3)

	uint16_t config_value;

	config_value = INREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0XA6));

	//-2~9 mapping to 1~12
	if (coeff <= VOC_HPF_COEF_RANGE_MAX && coeff >= VOC_HPF_COEF_RANGE_MIN) {
		coeff += VOC_HPF_COEF_RANGE_BASE;
		config_value &= ~VOICE_HPF_N_MSK;
		config_value |= (coeff << VOICE_HPF_N_POS);
	} else
		return -EINVAL;

	OUTREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0XA6), config_value);

	return 0;
}
//==============================================================================
int voc_hal_paganini_dmic_gain(int16_t gain)
{
	#define VOC_GAIN_MASK  0xFFF
	#define VOC_GAIN_SHIFT    15
	uint16_t value;

	voc_info("[%s]\n", __func__);

	if (gain < VOC_GAIN_MIN || gain > VOC_GAIM_MAX)
		return -EINVAL;

	INSREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0XAE), 0x01, 0x01);

	value = ((1 << VOC_GAIN_SHIFT) | (gain & VOC_GAIN_MASK));
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0XB2), value);

	value = (gain & VOC_GAIN_MASK);
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0XB2), value);

	return 0;
}
//==============================================================================
static int voc_hal_paganini_pdm_clk_enable(bool en)
{
	u16 value;

	if (en)
		value = 0x0001;
	else
		value = 0x0000;

	//reg_ckg_dig_mic
	#if ENABLE_CLOCK_FRAMEWORK
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X14, value, VOC_VAL_0, VOC_VAL_0);
	#else
	INSREG16(g_voc_reg.ckgen01_pm +	OFFSET8(VOC_REG_0X14), 0x0001, value);
	#endif
	return 0;
}
//==============================================================================
static int voc_hal_paganini_bck_enable(bool en, u16 bck_mode)
{
	u16 value;

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0X08));
	if (en)
		value = (value & VOC_REG_0XF0) + bck_mode;
	else
		value = value & VOC_REG_0XF0; //Clear

	OUTREG16(g_voc_reg.vrec + OFFSET8(VOC_REG_0X08), value);

	return 0;
}
//==============================================================================
int voc_hal_paganini_dmic_mute(bool en, u16 bck_mode)
{
	//[cm4]	if (voc_state == VOICE_STATE_NONE)
	//[cm4]		return 0;

	voc_hal_paganini_bck_enable(!en, bck_mode);

	return 0;
}
//==============================================================================
int voc_hal_paganini_dmic_switch(bool en, u16 bck_mode, u32 mute_enable)
{
	if (mute_enable) {
		voc_hal_paganini_pdm_clk_enable(en);
		return 0;
	}

	if (en) {
		//mic switch on: PDM engine clk on -> bck on
		voc_hal_paganini_pdm_clk_enable(en);
		voc_hal_paganini_bck_enable(en, bck_mode);
	} else {
		//mic switch off: bck off -> PDM engine clk off
		voc_hal_paganini_bck_enable(en, bck_mode);
		voc_hal_paganini_pdm_clk_enable(en);
	}

	return 0;
}
//==============================================================================
int voc_hal_paganini_reset_voice(void)
{
	return 0;
}
//==============================================================================

int voc_hal_paganini_update_snd_model(
				dma_addr_t snd_model_paddr,
				uint32_t size)
{
	UNUSED(snd_model_paddr);
	UNUSED(size);
	return 0;
}
//==============================================================================
int voc_hal_paganini_get_hotword_ver(void)
{
	return 0;
}
//==============================================================================
int voc_hal_paganini_get_hotword_identifier(void)
{
	return 0;
}
//==============================================================================
int voc_hal_paganini_ts_sync_start(void)
{
	return 0;
}
//==============================================================================
int voc_hal_paganini_enable_seamless(bool en)
{
	uint16_t u16Val;

	//Dummy:0x04301C
	voc_info("[%s:%d]enable :%d\n", __func__, __LINE__, (int)en);

	if (en)
		u16Val = 0x0001;
	else
		u16Val = 0x0000;

	INSREG16(g_voc_reg.vad_0 + OFFSET8(VOC_REG_0X1C), 0x0001, u16Val);

	return 0;
}
//==============================================================================
int voc_hal_paganini_notify_status(int32_t status)
{
	UNUSED(status);
	return 0;
}
//==============================================================================
int voc_hal_paganini_dma_get_reset_status(void)
{
	return 0;
}
//==============================================================================
void voc_hal_paganini_dma_enable_interrupt(int full, int overrun)
{
	uint16_t value;

	voc_info("[%s](%x,%x)\n", __func__, full, overrun);
	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_8));

	value |= VOICE_REG_WR_LOCAL_FULL_INT_EN; //1:mask, 0:unmask

	if (overrun)
		value &= ~VOICE_REG_WR_OVERRUN_INT_EN;
	else
		value |= VOICE_REG_WR_OVERRUN_INT_EN;

	if (full)
		value &= ~VOICE_REG_WR_FULL_INT_EN;
	else
		value |= VOICE_REG_WR_FULL_INT_EN;

	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_8), value);
}
//==============================================================================
void voc_hal_paganini_clear_dma_interrupt(enum voice_dma_intclr intclr)
{
	uint16_t value;

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0));
	switch (intclr) {
	case voice_dma_intclr_wr_full:
		value |= VOICE_REG_WR_FULL_FLAG_CLR;
		OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);
		value &= ~VOICE_REG_WR_FULL_FLAG_CLR;
		OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);
		break;

	case voice_dma_intclr_rd_empty:
		break;

	default:
		return;
	}
}
//==============================================================================
void voc_hal_paganini_enable_vdma_timer_interrupt(bool en)
{
	uint16_t value;

	if (en)
		value = BIT1;
	else
		value = 0;

	INSREG8(g_voc_reg.paganini + OFFSET8(VOC_REG_0X8C), BIT1, value);
}
//==============================================================================
void voc_hal_paganini_clear_vdma_timer_interrupt(void)
{
	INSREG8(g_voc_reg.paganini + OFFSET8(VOC_REG_0X8C), BIT3, BIT3);
	INSREG8(g_voc_reg.paganini + OFFSET8(VOC_REG_0X8C), BIT3, 0);
}
//==============================================================================
uint16_t voc_hal_paganini_get_vdma_timer_interrupt_status(void)
{
	uint16_t result;

	result = INREG16(g_voc_reg.paganini + OFFSET8(VOC_REG_0X8E)) & BIT0;
	return result;
}
//==============================================================================
void voc_hal_paganini_set_dma_buf_addr(uint32_t u32addr, uint32_t size)
{
	uint16_t miu_addr_lo, miu_addr_hi, miu_size;

	voc_info("[%s]\n", __func__);
	miu_addr_lo = (u32addr / VOICE_MIU_WORD_BYTE_SIZE) & VOICE_REG_WR_BASE_ADDR_LO_MSK;
	miu_addr_hi = ((u32addr / VOICE_MIU_WORD_BYTE_SIZE) >> VOICE_REG_WR_BASE_ADDR_HI_OFFSET)
					& VOICE_REG_WR_BASE_ADDR_HI_MSK;
	miu_size = (size / VOICE_MIU_WORD_BYTE_SIZE) & VOICE_REG_WR_BUFF_SIZE_MSK;

	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_1), miu_addr_lo);
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_2), miu_addr_hi);
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_3), miu_size);
}
//==============================================================================
void voc_hal_paganini_set_virtual_buf_addr(void *pAddr, uint32_t size)
{
	dma_buf_addr = dma_wr_ptr = (void *)pAddr;
	dma_buf_size = size;
	memset(pAddr, 0, size); //set 0 for mute case.
}
//==============================================================================
void voc_hal_paganini_set_dma_buf_overrun_thres(uint32_t overrun_thres)
{
	uint16_t miu_overrun_thres;

	miu_overrun_thres = (overrun_thres / VOICE_MIU_WORD_BYTE_SIZE) &
						VOICE_REG_WR_OVERRUN_TH_MSK;
	miu_overrun_thres += VOICE_DMA_INTERNAL_FIFO_SIZE; //lvl count include local buffer size
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_5), miu_overrun_thres);
}
//==============================================================================
void voc_hal_paganini_get_dma_interrupt_status(int *full, int *overrun)
{
	uint16_t value;

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_STS_1));
	*full = (value & VOICE_REG_WR_FULL_FLAG) ? TRUE : FALSE;
	*overrun  = (value & VOICE_REG_WR_OVERRUN_FLAG) ? TRUE : FALSE;
}
//==============================================================================
uint32_t voc_hal_paganini_dma_wr_level_cnt(void)
{
	uint16_t value;
	uint32_t wr_levelbytesize;

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0));
	value |= VOICE_REG_WR_LEVEL_CNT_MASK;
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_STS_0));
	value = ((value > VOICE_DMA_INTERNAL_FIFO_SIZE) ?
			(value - VOICE_DMA_INTERNAL_FIFO_SIZE) : 0);
	wr_levelbytesize = value * VOICE_MIU_WORD_BYTE_SIZE;

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0));
	value &= ~VOICE_REG_WR_LEVEL_CNT_MASK;
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);

	//voc_info("wr_levelbytesize =%x\n", wr_levelbytesize);
	return wr_levelbytesize;
}
//==============================================================================
void voc_hal_paganini_set_mcu_read(uint32_t buf_size)
{
	uint16_t value;

	value = (buf_size / VOICE_MIU_WORD_BYTE_SIZE) & VOICE_REG_WR_SIZE_MSK;
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_4), value);
}
//==============================================================================
void voc_hal_paganini_mcu_read_trigger(void)
{
	uint16_t value;

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0));
	value |= VOICE_REG_WR_TRIG;
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);

	value &= ~VOICE_REG_WR_TRIG;
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);
}
//==============================================================================
void voc_hal_paganini_clean_level_count(void)
{
	uint16_t value;
	uint32_t byte_size;

	//expecting dma was stopped.
	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_STS_0));
	byte_size = value * VOICE_MIU_WORD_BYTE_SIZE;

	voc_hal_paganini_set_mcu_read(byte_size);
	voc_hal_paganini_mcu_read_trigger();
}
//==============================================================================
uint32_t voc_hal_paganini_read_dma_data(uint8_t *dst_buf, uint32_t buf_size)
{
	uint32_t read_size, remain_size, dma_data_size;

	dma_data_size = voc_hal_paganini_dma_wr_level_cnt();
	if (dma_data_size == 0)
		return 0;

	if (buf_size > dma_data_size)
		buf_size = dma_data_size;

	if ((unsigned long)dma_wr_ptr + buf_size <= (unsigned long)dma_buf_addr + dma_buf_size) {
		read_size = buf_size;
		remain_size = 0;
		//memcpy(dst_buf, dma_wr_ptr, read_size);
		dma_wr_ptr += read_size;

		if ((unsigned long)dma_wr_ptr == (unsigned long)dma_buf_addr + dma_buf_size)
			dma_wr_ptr = dma_buf_addr;

	} else {
		read_size = (unsigned long)dma_buf_addr + dma_buf_size - (unsigned long)dma_wr_ptr;
		remain_size = buf_size - read_size;
		//memcpy(dst_buf, dma_wr_ptr, read_size);
		dma_wr_ptr = dma_buf_addr;
	}

	if (remain_size > 0) {
		//memcpy(dst_buf + read_size, dma_wr_ptr, remain_size);
		dma_wr_ptr += remain_size;
	}

	voc_hal_paganini_set_mcu_read(buf_size);
	voc_hal_paganini_mcu_read_trigger();

	return buf_size;
}
//==============================================================================
void voc_hal_paganini_dma_reset(void)
{
	uint16_t value;

	voc_info("[%s]\n", __func__);

	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_4), 0x0000);

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0));
	value |= VOICE_REG_SW_RST_DMA;
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);
	mdelay(VOC_VAL_20);

	value &= ~VOICE_REG_SW_RST_DMA;
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);

	value = INREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0));
	value |= (VOICE_REG_WR_SWAP | VOICE_REG_MIU_REQUSTER_ENABLE);
	OUTREG16(g_voc_reg.vrec + OFFSET8(VOICE_DMA_W1_CFG_0), value);

	/* reset pointer */
	dma_wr_ptr = dma_buf_addr;
}

//==============================================================================
void voc_hal_paganini_dc_on(void)
{
	voc_info("[%s]\n", __func__);

	#if ENABLE_CLOCK_FRAMEWORK
	//set 0x1A[3:0] = 8
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X1A, VOC_VAL_8, VOC_VAL_0, VOC_VAL_3);
	#else
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X1A)), VOC_REG_0X0F, VOC_VAL_8);
	#endif

	//reg_ckg_paganini_imi_dfs [3:2]:
	//clkgfmux_imi_dfs_ref_int_ck.sel
	//0: xtal, 1: pagapll_div2, 2:smi_ck

	//wriu -b 0x0000c1a 0x0f 0x08
}
//==============================================================================
void voc_hal_paganini_dc_off(void)
{
	voc_info("[%s]\n", __func__);
	//switch to AC ON stage
	#if ENABLE_CLOCK_FRAMEWORK
	//set 0x1A[3:0] = 8
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X18, BIT2, VOC_VAL_0, VOC_VAL_3);
	#else
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X18)), VOC_BIT_0X0F, BIT2);
	#endif

	//reg_ckg_paganini_imi_dfs [3:2]:
	//clkgfmux_imi_dfs_ref_int_ck.sel
	//0: xtal, 1: pagapll_div2, 2:smi_ck

	//wriu -b 0x0000c18 0x0f 0x04
}

//==============================================================================
static int voc_hal_paganini_dts_init(void)
{
	int ret = 0;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-dma");
	if (!np)
		return -EFAULT;


	g_voc_reg.paganini	 = (phys_addr_t)of_iomap(np, 0); //0x432
	//g_voc_reg.pm_sleep   = (phys_addr_t)of_iomap(np, 1);
	g_voc_reg.pm_top	 = (phys_addr_t)of_iomap(np, VOC_VAL_2);
	//g_voc_reg.pm_misc	   = (phys_addr_t)of_iomap(np, VOC_VAL_3);
	//g_voc_reg.cpu_int	   = (phys_addr_t)of_iomap(np, VOC_VAL_4);
	//g_voc_reg.inturrpt   = (phys_addr_t)of_iomap(np, VOC_VAL_5);
	//g_voc_reg.viv_bdma   = (phys_addr_t)of_iomap(np, VOC_VAL_6);
	//g_voc_reg.vivaldi4   = (phys_addr_t)of_iomap(np, VOC_VAL_7);
	g_voc_reg.pm_pmux   = (phys_addr_t)of_iomap(np, VOC_VAL_8); //0x110
	//g_voc_reg.vivaldi2   = (phys_addr_t)of_iomap(np, VOC_VAL_9);
	//g_voc_reg.MALIBOX	   = (phys_addr_t)of_iomap(np, VOC_VAL_10);
	g_voc_reg.ckgen00_pm = (phys_addr_t)of_iomap(np, VOC_VAL_11); //0x0c
	g_voc_reg.ckgen01_pm = (phys_addr_t)of_iomap(np, VOC_VAL_12); //0x0d
	g_voc_reg.vad_0		 = (phys_addr_t)of_iomap(np, VOC_VAL_13); //0x430
	g_voc_reg.vrec		 = (phys_addr_t)of_iomap(np, VOC_VAL_14); //0x431
	//g_voc_reg.coin_cpu   = (phys_addr_t)of_iomap(np, VOC_VAL_15);
	//g_voc_reg.vad_1	   = (phys_addr_t)of_iomap(np, VOC_VAL_16); //0x940

	of_node_put(np);
	if ((g_voc_reg.paganini   == 0) ||
		(g_voc_reg.pm_top     == 0) ||
		(g_voc_reg.pm_pmux    == 0) ||
		(g_voc_reg.ckgen00_pm == 0) ||
		(g_voc_reg.ckgen01_pm == 0) ||
		(g_voc_reg.vad_0      == 0) ||
		(g_voc_reg.vrec       == 0))
		return -EFAULT;

	return ret;
}


void voc_hal_paganini_ip_init(void)
{
	voc_info("[%s]\n", __func__);

	// AC ON
	//reg_paga_48_96_ck_div_ctrl
	OUTREG16((g_voc_reg.paganini)|(OFFSET(VOC_REG_0XE0)), (BIT15|BIT10));

	#if ENABLE_CLOCK_FRAMEWORK
	//reg_sw_en_imi2paganini_imi
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X1E, 0x00, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm42cm4
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0C, 0x00, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm4_core2cm4_core
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X02, 0x00, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm4_systick2cm4_systick
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X06, 0x00, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm4_tck2tck_int
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X08, 0x00, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm4_tsvalueb2cm4_tsvalueb
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0A, 0x00, VOC_VAL_0, VOC_VAL_0);
	#else
	//reg_sw_en_imi2paganini_imi
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X1E)), 0x01, 0x00);
	//reg_sw_en_cm42cm4
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X0C)), 0x01, 0x00);
	//reg_sw_en_cm4_core2cm4_core
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X02)), 0x01, 0x00);
	//reg_sw_en_cm4_systick2cm4_systick
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X06)), 0x01, 0x00);
	//reg_sw_en_cm4_tck2tck_int
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X08)), 0x01, 0x00);
	//reg_sw_en_cm4_tsvalueb2cm4_tsvalueb
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X0A)), 0x01, 0x00);
	#endif
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XE6)), BIT1, 0x00); //reg_cm4_core_en_lock
	//reg_clkgen_imi_dfs_ctrl
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF3)), BIT1, BIT1);
	//reg_clkgen_cm4_dfs_ctrl
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF7)), BIT1, BIT1);

	#if ENABLE_CLOCK_FRAMEWORK
	//reg_ckg_nf_synth_ref
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X16, BIT3, VOC_VAL_0, VOC_VAL_4);
	//reg_ckg_codec_i2s_rx_bck
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X0E, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_4);
	//reg_ckg_src_a1_256fsi
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X1E, BIT3, VOC_VAL_0, VOC_VAL_3);
	//reg_ckg_codec_i2s_rx_ms_bck_p
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X10, VOC_REG_0X10, VOC_VAL_0, VOC_VAL_4);
	//reg_ckg_paganini_imi_div2
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X26, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_2);
	#else
	//reg_ckg_nf_synth_ref
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X16)), VOC_REG_0X1F, BIT3);
	//reg_ckg_codec_i2s_rx_bck
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X0E)), VOC_REG_0X1F, VOC_REG_0X04);
	//reg_ckg_src_a1_256fsi
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X1E)), VOC_REG_0X0F, BIT3);
	//reg_ckg_codec_i2s_rx_ms_bck_p
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X10)), VOC_REG_0X1F, VOC_REG_0X10);

	//reg_ckg_paganini_imi_div2
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X26)), VOC_REG_0X07, VOC_REG_0X04);
	#endif
	//reg_nf_synth_en_trig
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XEC)), VOC_REG_0X02, VOC_REG_0X02);
	//reg_i2s_tdm_cfg_00
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X61)), VOC_REG_0X80, VOC_REG_0X80);

	// CM4 TSV & SYSTICK
	#if ENABLE_CLOCK_FRAMEWORK
	//reg_ckg_paga_pll_sel
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X20, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_4);
	//reg_ckg_pll2pagadiv
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X1C, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_3);
	//reg_ckg_cm4
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X00, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_2);
	//reg_ckg_cm4_dfs
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X04, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_3);
	//reg_ckg_cm4_core
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X02, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_2);
	//reg_ckg_cm4_tsvalueb
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X0A, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_2);
	//reg_ckg_cm4_systick
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X06, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_2);
	#else
	//reg_ckg_paga_pll_sel
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X20)), VOC_REG_0X1F, VOC_REG_0X04);
	//reg_ckg_pll2pagadiv
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X1C)), VOC_REG_0X0F, VOC_REG_0X04);
	//reg_ckg_cm4
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X00)), VOC_REG_0X07, VOC_REG_0X04);
	//reg_ckg_cm4_dfs
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X04)), VOC_REG_0X0F, VOC_REG_0X04);
	//reg_ckg_cm4_core
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X02)), VOC_REG_0X07, VOC_REG_0X04);
	//reg_ckg_cm4_tsvalueb
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X0A)), VOC_REG_0X07, VOC_REG_0X04);
	//reg_ckg_cm4_systick
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X06)), VOC_REG_0X07, VOC_REG_0X04);
	#endif
	//reg_clkgen_cm4_dfs_ctrl
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF6)), VOC_REG_0X3F, VOC_REG_0X1F);
	//reg_clkgen_cm4_dfs_ctrl
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF7)), VOC_REG_0X81, 0x00);
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF7)), VOC_REG_0X81, 0x01);
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF7)), VOC_REG_0X81, 0x00);

	// IMI CLK	XTAIL -> DFS OFF -> PLL -> DFS_ON
	#if ENABLE_CLOCK_FRAMEWORK
	//reg_ckg_paganini_imi
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X18, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_3);
	//reg_ckg_paganini_imi_dfs
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X1A, VOC_REG_0X04, VOC_VAL_0, VOC_VAL_3);
	#else
	//reg_ckg_paganini_imi
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X18)), VOC_REG_0X0F, VOC_REG_0X04);
	//reg_ckg_paganini_imi_dfs
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X1A)), VOC_REG_0X0F, VOC_REG_0X04);
	#endif
	//reg_clkgen_imi_dfs_ctrl
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF2)), VOC_REG_0X3F, VOC_REG_0X1F);
	//reg_clkgen_imi_dfs_ctrl
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF3)), VOC_REG_0X81, 0x00);
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF3)), VOC_REG_0X81, 0x01);
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XF3)), VOC_REG_0X81, 0x00);

	// CLK Release
	#if ENABLE_CLOCK_FRAMEWORK
	//reg_sw_en_imi2paganini_imi
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X1E, 0x01, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm42cm4
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0C, 0x01, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm4_core2cm4_core
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X02, 0x01, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm4_systick2cm4_systick
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X06, 0x01, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm4_tck2tck_int
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X08, 0x01, VOC_VAL_0, VOC_VAL_0);
	//reg_sw_en_cm4_tsvalueb2cm4_tsvalueb
	voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0A, 0x01, VOC_VAL_0, VOC_VAL_0);
	#else
	//reg_sw_en_imi2paganini_imi
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X1E)), 0x01, 0x01);
	//reg_sw_en_cm42cm4
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X0C)), 0x01, 0x01);
	//reg_sw_en_cm4_core2cm4_core
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X02)), 0x01, 0x01);
	//reg_sw_en_cm4_systick2cm4_systick
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X06)), 0x01, 0x01);
	//reg_sw_en_cm4_tck2tck_int
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X08)), 0x01, 0x01);
	//reg_sw_en_cm4_tsvalueb2cm4_tsvalueb
	INSREG8((g_voc_reg.ckgen01_pm)|(OFFSET8(VOC_REG_0X0A)), 0x01, 0x01);
	#endif
	//reg_cm4_core_en_lock
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XE6)), VOC_REG_0X02, VOC_REG_0X02);

	// DMIC BCK Gen Clk
	#if ENABLE_CLOCK_FRAMEWORK
	//reg_ckg_vrec_mac
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X22, VOC_REG_0X0C, VOC_VAL_0, VOC_VAL_3);
	//reg_ckg_dig_mic
	voc_common_clock_set(VOC_CLKGEN0, VOC_REG_0X12, BIT3, VOC_VAL_0, VOC_VAL_3);
	#else
	//reg_ckg_vrec_mac
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X22)), VOC_REG_0X0F, VOC_REG_0X0C);
	//reg_ckg_dig_mic
	INSREG8((g_voc_reg.ckgen00_pm)|(OFFSET8(VOC_REG_0X12)), VOC_REG_0X0F, BIT3);
	#endif

	//mokona
	//reg_paga_pll_div_enable
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XE0)), VOC_REG_0X01, 0x01);
	//[7:4]reg_paga_pll_div_96
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XE0)), VOC_REG_0XF8, VOC_REG_0X40);
	//wriu -b 0x00432e1 0xff 0x09  //[11:8]reg_paga_pll_div_48
	OUTREG8((g_voc_reg.paganini)|(OFFSET8(REG_PAGA_PLL_DIV_48)), BIT3|BIT0);

	//vrec dma setting
	//RIU_AUDMA_W1_CFG01[15:0],	 dram_base_addr
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X62)), BIT8);
	//RIU_AUDMA_W1_CFG05[15:0],	 overrun_th
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X6A)), BIT8);
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X60)), BIT15); //RIU_AUDMA_W1_CFG00
	//RIU_AUDMA_W1_CFG00 [0]:miu_req_en, [1]:dma_wr_en, [3]:lr_swap, [9]:bit_mode
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X60)), VOC_AUDMA_W1_CFG00_VALUE);
	//reg_dma_mch_if_cfg1, [2:0] channel mode, 0:2ch, 1:4ch
	//OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X80)), 0x0003);
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X80)), 0x0001);
	//reg_dmic_ctrl1, [0] enable phase select for ch1 and ch2
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X22)), 0x0001);
	//reg_dmic_ctrl2, [15] enable auto phase selection
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X24)), BIT15);
	//reg_dmic_ctrl3, [0] enable phase select for ch3 and ch4
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X26)), 0x0001);
	//reg_dmic_ctrl4, [15] enable auto phase selection
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X28)), BIT15);
	//reg_dmic_ctrl5, [0] enable phase select for ch5 and ch6
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X2A)), 0x0001);
	//reg_dmic_ctrl6, [15] enable auto phase selection
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X2C)), BIT15);
	//reg_dmic_ctrl7, [0] enable phase select for ch7 and ch8
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X2E)), 0x0001);
	//reg_dmic_ctrl8, [15] enable auto phase selection
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X30)), BIT15);
	//reg_dmic_ctrl0
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X20)), 0x0000);
	//reg_vrec_ctrl06,
	//[0] enable deciamtion filter, ch1/ch2
	//[1] enable deciamtion filter, ch3/ch4
	//[2] enable deciamtion filter, ch5/ch6
	//[3] enable deciamtion filter, ch7/ch8
	//[4] enable deciamtion filter
	//[8] enable output clock  generation
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X0C)), VOC_VREC_CTRl06_VALUE);
	OUTREG16((g_voc_reg.paganini)|(OFFSET(VOC_REG_0X22)), BIT14); //reg_paga_ctrl_11

	//VREC//reg_vrec_ctrl01, [0] enable phase selection and decimation filter
	INSREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X02)), VOC_REG_0X09, VOC_REG_0X09);
	INSREG8((g_voc_reg.vad_0)|(OFFSET8(VOC_REG_0X14)), 0x01, 0x01); //reg_vrec_ctrl0a
	//reg_paga_ctrl_1d
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X3B)), VOC_REG_0X0C, VOC_REG_0X04);
	INSREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA6)), 0x01, 0x01); //reg_pp_ctrl3
	INSREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA1)), VOC_REG_0X0F, 0x01); //reg_pp_ctrl0
	//reg_vrec_ctrl01,
	//[14] reset decimation filter 0:de-active 1:active
	INSREG8((g_voc_reg.vad_0)|(OFFSET8(VOC_REG_0X03)), VOC_REG_0XC0, VOC_REG_0XC0);
	//reg_dmic_ctrl1
	INSREG8((g_voc_reg.vad_0)|(OFFSET8(VOC_REG_0X23)), VOC_REG_0X80, VOC_REG_0X80);
	//reg_paga_ctrl_1d
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X3A)), VOC_REG_0X02, VOC_REG_0X02);
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X3A)), 0x01, 0x01); //reg_paga_ctrl_1d
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X3C)), 0x01, 0x01); //reg_paga_ctrl_1e
	//reg_paga_ctrl_1e
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X3C)), VOC_REG_0X0C, 0x00);
	//reg_vrec_ctrl06,
	//[0] enable deciamtion filter, ch1/ch2
	//[1] enable deciamtion filter, ch3/ch4
	//[2] enable deciamtion filter, ch5/ch6
	//[3] enable deciamtion filter, ch7/ch8
	//[4] enable deciamtion filter
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X0C)), VOC_REG_0X1F);
	//reg_vrec_ctrl06,
	//[8] enable output clock  generation
	//[15] enable SRAM initialization for decimation filter
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X0D)), VOC_REG_0X81);
	//reg_vrec_ctrl04,
	//[4:0] auto selection for (sampling rate, decimation rate, output clock) (8k, 10, 400k)
	INSREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X08)), VOC_REG_0X11, 0x01);
	//reg_vrec_ctrl01,
	//[0] enable phase selection and decimation filter
	//[1] invert output clock
	//[5:4] channel mode for digital microphone 0:1ch 1:2ch 2:4ch 3:8ch
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X02)), VOC_REG_0X29);
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X03)), 0x00); //reg_vrec_ctrl01
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X20)), 0x00); //reg_dmic_ctrl0
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X21)), 0x00); //reg_dmic_ctrl0
	//reg_cic_ctrl0, [0] enable decimation filter
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X40)), VOC_REG_0X09);
	//reg_cic_ctrl0, [0] enable decimation filter
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X41)), VOC_REG_0X0F);
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X0C)), VOC_REG_0X13); //reg_vrec_ctrl06
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X0D)), VOC_REG_0X05); //reg_vrec_ctrl06
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X22)), 0x01); //reg_dmic_ctrl1
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X23)), 0x00); //reg_dmic_ctrl1
	INSREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X21)), 0x01, 0x01); //reg_dmic_ctrl0
	//reg_dmic_ctrl3, [0] enable phase select for ch3 and ch4
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X26)), 0x01);
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X27)), 0x00); //reg_dmic_ctrl3
	INSREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X21)), BIT1, BIT1); //reg_dmic_ctrl0
	//reg_cic_ctrl1, [0] order for decimation filter ch1/ch2
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X42)), 0x01);
	//reg_cic_ctrl2, [0] order for decimation filter ch3/ch4
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X44)), 0x01);
	//reg_cic_ctrl3, [0] order for decimation filter ch5/ch6
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X46)), 0x01);
	//reg_cic_ctrl4, [0] order for decimation filter ch7/ch8
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X48)), 0x01);
	//reg_vrec_ctrl04, [3:0]: BCK [5:4]: FS	 BCK:  3M	FS: 16K
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X08)), VOC_REG_0X19);
	INSREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X40)), VOC_REG_0X09, 0x00); //reg_cic_ctrl0
	//reg_cic_ctrl0, [0] enable decimation filter
	INSREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0X40)), VOC_REG_0X09, 0x01);

	// PPCS REG setting
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA0)), 0x00); //reg_pp_ctrl0
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA1)), 0x00); //reg_pp_ctrl0
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA2)), VOC_REG_0X21); //reg_pp_ctrl1
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA3)), VOC_REG_0X43); //reg_pp_ctrl1
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA4)), VOC_REG_0X65); //reg_pp_ctrl2
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA5)), VOC_REG_0X87); //reg_pp_ctrl2
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA6)), VOC_REG_0X0A); //reg_pp_ctrl3
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA7)), VOC_REG_0X73); //reg_pp_ctrl3
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA8)), 0x00); //reg_pp_ctrl4
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XA9)), 0x00); //reg_pp_ctrl4
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XAA)), 0x00); //reg_pp_ctrl5
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XAB)), 0x00); //reg_pp_ctrl5
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XAC)), 0x00); //reg_pp_ctrl6
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XAD)), 0x00); //reg_pp_ctrl6
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XAE)), VOC_REG_0X06); //reg_pp_ctrl7
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XAF)), VOC_REG_0X07); //reg_pp_ctrl7
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XB0)), 0x00); //reg_pp_ctrl8
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XB1)), 0x00); //reg_pp_ctrl8
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XB2)), 0x00); //reg_pp_ctrl9
	OUTREG8((g_voc_reg.vrec)|(OFFSET8(VOC_REG_0XB3)), 0x00); //reg_pp_ctrl9

	//dma setting
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X6A)), BIT8);
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X60)), BIT15);
	OUTREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X60)), VOC_REG_0X09);
	//aec_dma setting
	//overrun_th
	OUTREG16((g_voc_reg.paganini)|(OFFSET(VOC_REG_0XCA)), BIT8);
	//RIU_AIDMA_W1_CFG, [15]sft_reset
	OUTREG16((g_voc_reg.paganini)|(OFFSET(VOC_REG_0XC0)), BIT15);
	//RIU_AIDMA_W1_CFG, [0]:miu_req_en, [3]:lr_swap
	OUTREG16((g_voc_reg.paganini)|(OFFSET(VOC_REG_0XC0)), VOC_REG_0X09);

	//i2s tdm rx setting
	//reg_paga_ctrl_18
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X30)), VOC_REG_0X01, VOC_REG_0X01);
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X30)), VOC_REG_0X10, VOC_REG_0X10);
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X30)), VOC_REG_0X40, VOC_REG_0X40);
	//reg_nf_synth_en_trig
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0XEC)), VOC_REG_0X02, VOC_REG_0X02);
	//reg_i2s_tdm_cfg_00
	INSREG8((g_voc_reg.paganini)|(OFFSET8(VOC_REG_0X61)), VOC_REG_0X80, VOC_REG_0X80);

	//reg_vrec_ctrl02, [0] enable phase selection and decimation filter
	INSREG16((g_voc_reg.vrec)|(OFFSET(VOC_REG_0X04)), 0x0001, 0x0001);


	//DC On
	voc_hal_paganini_dc_on();

	//Script:
	// AC ON
	//wriu -w 0x00432e0	 0x8001

	//wriu -b 0x0000d1e 0x01 0x00
	//wriu -b 0x0000d0c 0x01 0x00
	//wriu -b 0x0000d02 0x01 0x00
	//wriu -b 0x0000d06 0x01 0x00
	//wriu -b 0x0000d08 0x01 0x00
	//wriu -b 0x0000d0a 0x01 0x00
	//wriu -b 0x00432e6 0x02 0x00
	//wriu -b 0x00432f3 0x02 0x02
	//wriu -b 0x00432f7 0x02 0x02

	//wriu -b 0x0000c16 0x1f 0x08
	//wriu -b 0x0000c0e 0x1f 0x04
	//wriu -b 0x0000c1e 0x0f 0x08
	//wriu -b 0x0000c10 0x1f 0x10

	//wriu -b 0x0000c26 0x07 0x04

	//wriu -b 0x00432ec 0x02 0x02
	//wriu -b 0x0043261 0x80 0x80

	// CM4 TSV & SYSTICK
	//wriu -b 0x0000c20 0x1f 0x04
	//wriu -b 0x0000c1c 0x0f 0x04
	//wriu -b 0x0000c00 0x07 0x04
	//wriu -b 0x0000c04 0x0f 0x04
	//wriu -b 0x0000c02 0x07 0x04
	//wriu -b 0x0000c0a 0x07 0x04
	//wriu -b 0x0000c06 0x07 0x04

	//wriu -b 0x00432f6 0x3f 0x1f
	//wriu -b 0x00432f7 0x81 0x00
	//wriu -b 0x00432f7 0x81 0x01
	//wriu -b 0x00432f7 0x81 0x00

	// IMI CLK	XTAIL -> DFS OFF -> PLL -> DFS_ON
	//wriu -b 0x0000c18 0x0f 0x04
	//wriu -b 0x0000c1a 0x0f 0x04

	//wriu -b 0x00432f2 0x3f 0x1f
	//wriu -b 0x00432f3 0x81 0x00
	//wriu -b 0x00432f3 0x81 0x01
	//wriu -b 0x00432f3 0x81 0x00

	// CLK Release
	//wriu -b 0x0000d1e 0x01 0x01
	//wriu -b 0x0000d0c 0x01 0x01
	//wriu -b 0x0000d02 0x01 0x01
	//wriu -b 0x0000d06 0x01 0x01
	//wriu -b 0x0000d08 0x01 0x01
	//wriu -b 0x0000d0a 0x01 0x01
	//wriu -b 0x0000d90 0x07 0x07 //reg_sw_en_mcu_pmu2cm4
	//wriu -b 0x00432e6 0x02 0x02

	// DMIC BCK Gen Clk
	//wriu -b 0x0000c22 0x0f 0x0c
	//wriu -b 0x0000c12 0x0f 0x08

	//mokona
	//wriu -b 0x00432e0 0x01 0x01  //reg_paga_pll_div_enable
	//wriu -b 0x00432e0 0xf8 0x40  //[7:4]reg_paga_pll_div_96
	//wriu -b 0x00432e1 0xff 0x09  //[11:8]reg_paga_pll_div_48

	//dma setting
	//wriu -w 0x0043162	 0x0100
	//wriu -w 0x004316a	 0x0100
	//wriu -w 0x0043160	 0x8000
	//wriu -w 0x0043160	 0x020b

	//wriu -w 0x0043180	 0x0003

	//wriu -w 0x0043122	 0x0001
	//wriu -w 0x0043124	 0x8000
	//wriu -w 0x0043126	 0x0001
	//wriu -w 0x0043128	 0x8000
	//wriu -w 0x004312A	 0x0001
	//wriu -w 0x004312C	 0x8000
	//wriu -w 0x004312E	 0x0001
	//wriu -w 0x0043130	 0x8000
	//wriu -w 0x0043120	 0x0000
	//wriu -w 0x004310C	 0x011F

	//wriu -w 0x0043222	 0x4000

	//VREC
	//wriu -b 0x0043102 0x09 0x09
	//wriu -b 0x0043014 0x01 0x01
	//wriu -b 0x004323b 0x0c 0x04
	//wriu -b 0x00431a6 0x01 0x01
	//wriu -b 0x00431a1 0x0f 0x01
	//wriu -b 0x0043003 0xc0 0xc0
	//wriu -b 0x0043023 0x80 0x80
	//wriu -b 0x004323a 0x02 0x02
	//wriu -b 0x004323a 0x01 0x01
	//wriu -b 0x004323c 0x01 0x01
	//wriu -b 0x004323c 0x0c 0x00
	//wriu -b 0x004310c 0xff 0x1f
	//wriu -b 0x004310d 0xff 0x81
	//wriu -b 0x0043108 0x11 0x01
	//wriu -b 0x0043102 0xff 0x29
	//wriu -b 0x0043103 0xff 0x00
	//wriu -b 0x0043120 0xff 0x00
	//wriu -b 0x0043121 0xff 0x00
	//wriu -b 0x0043140 0xff 0x09
	//wriu -b 0x0043141 0xff 0x0f
	//wriu -b 0x004310c 0xff 0x13
	//wriu -b 0x004310d 0xff 0x05
	//wriu -b 0x0043122 0xff 0x01
	//wriu -b 0x0043123 0xff 0x00
	//wriu -b 0x0043121 0x01 0x01
	//wriu -b 0x0043126 0xff 0x01
	//wriu -b 0x0043127 0xff 0x00
	//wriu -b 0x0043121 0x02 0x02
	//wriu -b 0x0043142 0xff 0x01
	//wriu -b 0x0043144 0xff 0x01
	//wriu -b 0x0043146 0xff 0x01
	//wriu -b 0x0043148 0xff 0x01
	//wriu -b 0x0043108 0xff 0x19

	//reg_vrec_ctrl04, [5:0]	[5:4]: FS	 [3:0]: BCK
	//[5:4]FS:	0: 8K,	 1: 16K,   2: 32K	, 3:48KHZ
	//[3:0]BCK:	 0: 100k,
	//			 1: 400K,
	//			 2: 800K,
	//			 3: 1000K,
	//			 4: 12000K,
	//			 5:1600K,
	//			 6: 2000K,
	//			 7: 3000K,
	//			 8: 4000K HZ
	// p.s.		BCK: 1000kHZ , Fs: 48k	 not support

	//wriu -b 0x0043140 0x09 0x00
	//wriu -b 0x0043140 0x09 0x01

	// PPCS REG setting
	//wriu -b 0x00431a0 0xff 0x00
	//wriu -b 0x00431a1 0xff 0x00
	//wriu -b 0x00431a2 0xff 0x21
	//wriu -b 0x00431a3 0xff 0x43
	//wriu -b 0x00431a4 0xff 0x65
	//wriu -b 0x00431a5 0xff 0x87
	//wriu -b 0x00431a6 0xff 0x0a
	//wriu -b 0x00431a7 0xff 0x73
	//wriu -b 0x00431a8 0xff 0x00
	//wriu -b 0x00431a9 0xff 0x00
	//wriu -b 0x00431aa 0xff 0x00
	//wriu -b 0x00431ab 0xff 0x00
	//wriu -b 0x00431ac 0xff 0x00
	//wriu -b 0x00431ad 0xff 0x00
	//wriu -b 0x00431ae 0xff 0x06
	//wriu -b 0x00431af 0xff 0x07
	//wriu -b 0x00431b0 0xff 0x00
	//wriu -b 0x00431b1 0xff 0x00
	//wriu -b 0x00431b2 0xff 0x00
	//wriu -b 0x00431b3 0xff 0x00

	//dma setting
	//wriu -w 0x004316a 0x0100
	//wriu -w 0x0043160 0x8000
	//wriu -w 0x0043160 0x0009

	//aec_dma setting
	//wriu -w 0x00432ca 0x0100
	//wriu -w 0x00432c0 0x8000
	//wriu -w 0x00432c0 0x0009

	//i2s tdm rx setting
	//wriu -b 0x0043230 0x01 0x01
	//wriu -b 0x0043230 0x10 0x10
	//wriu -b 0x0043230 0x40 0x40
	//wriu -b 0x00432ec 0x02 0x02
	//wriu -b 0x0043261 0x80 0x80

	//wriu -b 0x0043104 0x0001 0x0001

	// DC ON
	//wriu -b 0x0000c1a 0x0f 0x08
}

int voc_hal_paganini_init(void)
{
	int ret = 0;

	ret = voc_hal_paganini_dts_init();
	if (ret) {
		voc_info("[%s] can't parse dts\n", __func__);
		return ret;
	}

	voc_hal_paganini_ip_init();
	//voc_hal_paganini_pad_init();

	return ret;
}

