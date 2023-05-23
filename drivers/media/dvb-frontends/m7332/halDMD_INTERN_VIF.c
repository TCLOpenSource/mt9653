// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "halDMD_INTERN_VIF.h"
#include "regVIF.h"

//-----------------------------------------------
//  Driver Compiler Options
//-----------------------------------------------
#define HALVIFDBG(x)      //x

//#define HALVIFDBG_BIT     (DBB1_REG_BASE+0x06)  // Bit 4~7
//#define HALVIFDBG1_BIT    (DBB1_REG_BASE+0x04)  // Bit 1
//#define HALVIFDBG2_BIT    (DBB1_REG_BASE+0xF6)  // Bit 0~1
//-----------------------------------------------
//  extern function
//-----------------------------------------------
// Please add DelayTime definition according to Third party platform.
#define hal_vif_delayms(x)    mtk_demod_delay_ms(x)
#define hal_vif_delayus(x)    mtk_demod_delay_us(x)
//-----------------------------------------------
//  Local Defines
//-----------------------------------------------

//if TMP_RIU_BASE_ADDRESS   0x1F200000
//#define MBREG_BASE  0x12600  //0x112600
//#define DMDMCU_BASE 0x03480  //0x103480
//#define T_101E_BASE 0x01E00
//#define T_1033_BASE 0x03300
//#define T_111F_BASE 0x11F00
//#define T_1120_BASE 0x12000
//#define T_1127_BASE 0x12700
//#define T_1128_BASE 0x12800

//else if TMP_RIU_BASE_ADDRESS   0x1F000000
#define MBREG_BASE  0x112600  //0x112600 //in "m7332_demod_common.h"
#define DMDMCU_BASE 0x103480  //0x103480

#define T_101E_BASE 0x101E00
#define T_1033_BASE 0x103300
#define T_111F_BASE 0x111F00
#define T_1120_BASE 0x112000
#define T_1127_BASE 0x112700
#define T_1128_BASE 0x112800

#define __CHIP_VERSION 0x1ECF

#ifndef _END_OF_TBL_
#define _END_OF_TBL_		0xFFFF
#endif

// Base address should be initial.
#if defined(__aeon__)           // Non-OS
	#define BASEADDR_RIU 0xA0000000UL
#else                           // ecos
	#define BASEADDR_RIU 0xBF800000UL
#endif

/*short ver*/
#define _riu_read_byte(reg) mtk_demod_read_byte(reg)
#define _riu_write_byte(reg, value) mtk_demod_write_byte(reg, value)
#define _riu_write_byte_mask(reg, val, msk)\
	mtk_demod_write_byte_mask(reg, val, msk)
#define _riu_write_bit(reg, en_bool, msk)\
	mtk_demod_write_bit(reg, en_bool, msk)


#define os_get_systime_ms()        mtk_demod_get_time()


static u32 hal_vif_get_sys_time(void)
{
	return mtk_demod_get_time();
}


//-----------------------------------------------
//  Local Structures
//-----------------------------------------------
struct hal_vif_t {
	//MS_VIRT virtVIFBaseAddr;
	bool base_addr_init_done;
};
//-----------------------------------------------
//  Local Variables
//-----------------------------------------------
static struct hal_vif_t _hal_vif; // = {BASEADDR_RIU, 0};
static bool g_vif_downloaded;

//-----------------------------------------------
//  Driver Variables Pointers
//-----------------------------------------------
static struct vif_initial_in *_pvif_in;
static struct vif_notch_a1a2 *_pn_a1a2;
static struct vif_sos_1112 *_psos_1112;
static u16 *_pvif_filters;


// Please use dat file with pre-defined parameter
// according to Third party platform.
const u8 intern_vif_table[] = {
	#include "DMD_INTERN_VIF.dat"
};

static u8 g_vif_prev_scan_flag;

/*
 * VIF_IS_ADC_48MHz
 * 0:144MHz, 1:48MHz
 */
#define VIF_IS_ADC_48MHz   1

static bool _mbx_writereg(u16 addr, u8 data)
{
	u8 check_count;
	u8 check_flag;

	if (!_hal_vif.base_addr_init_done)
		return FALSE;

	if (!g_vif_downloaded) {
		pr_debug("VIF NOT DOWNLOADED\n");
		return FALSE;
	}

	_riu_write_byte(MBREG_BASE + 0x00, (addr & 0xff));
	_riu_write_byte(MBREG_BASE + 0x01, (addr >> 8));
	_riu_write_byte(MBREG_BASE + 0x10, data);
	_riu_write_byte(MBREG_BASE + 0x1E, 0x01);

	// assert interrupt to VD MCU51
	_riu_write_bit(DMDMCU_BASE + 0x03, 1, _BIT1);
	// de-assert interrupt to VD MCU51
	_riu_write_bit(DMDMCU_BASE + 0x03, 0, _BIT1);

	for (check_count = 0; check_count < 100; check_count++) {
		check_flag = _riu_read_byte(MBREG_BASE + 0x1E);

		if ((check_flag & 0x01) == 0)
			break;

		hal_vif_delayms(1);
	}

	if ((check_flag & 0x01) != 0) {
		pr_info("ERROR: VIF INTERN DEMOD MBX WRITE TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

static bool _mbx_readreg(u16 addr, u8 *data)
{
	u8 check_count;
	u8 check_flag;

	if (!_hal_vif.base_addr_init_done)
		return FALSE;

	if (!g_vif_downloaded) {
		pr_debug("VIF NOT DOWNLOADED\n");
		return FALSE;
	}

	_riu_write_byte(MBREG_BASE + 0x00, (addr & 0xff));
	_riu_write_byte(MBREG_BASE + 0x01, (addr >> 8));
	_riu_write_byte(MBREG_BASE + 0x1E, 0x02);

	// assert interrupt to VD MCU51
	_riu_write_bit(DMDMCU_BASE + 0x03, 1, _BIT1);
	// de-assert interrupt to VD MCU51
	_riu_write_bit(DMDMCU_BASE + 0x03, 0, _BIT1);

	for (check_count = 0; check_count < 100; check_count++) {
		check_flag = _riu_read_byte(MBREG_BASE + 0x1E);

		if ((check_flag&0x02) == 0) {
			*data = _riu_read_byte(MBREG_BASE + 0x10);
			break;
		}

		hal_vif_delayms(1);
	}

	if ((check_flag&0x02) != 0) {
		pr_info("ERROR: VIF INTERN DEMOD MBX READ TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

static u16 _hal_vif_get_demod_addr(u32 reg)
{
	u16 addr;

	switch (reg&(0xFFFFFF00)) {
	case ADC_REG_BASE:
		addr = VIF_SLAVE_ADC_REG_BASE;
		break;
	case RF_REG_BASE:
		addr = VIF_SLAVE_RF_REG_BASE;
		break;
	case DBB1_REG_BASE:
		addr = VIF_SLAVE_DBB1_REG_BASE;
		break;
	case DBB2_REG_BASE:
		addr = VIF_SLAVE_DBB2_REG_BASE;
		break;
	case DBB3_REG_BASE:
		addr = VIF_SLAVE_DBB3_REG_BASE;
		break;
	case DMDTOP_REG_BASE:
		addr = VIF_SLAVE_DMDTOP_REG_BASE;
		break;
	default:
		addr = 0;
		break;
	}

	return (addr|(u16)(reg&0x000000FF));
}

static bool _is_ctrl_by_demod(void)
{
	bool ret = (((_riu_read_byte(T_101E_BASE+0x39L) & 0x03) != 0)
			&& g_vif_downloaded);

	return ret;
}

static u16 _hal_vif_read_2bytes(u32 reg)
{
	//2 bytes were read at different time, could be a trouble
	u16 demod_addr;
	u8 tmp_data = 0;
	u16 ret_data = 0;

	demod_addr = _hal_vif_get_demod_addr(reg);

	if (((demod_addr&0xFF00) != 0) && _is_ctrl_by_demod()) {
		if (_mbx_readreg(demod_addr+1, &tmp_data)) {
			ret_data = tmp_data;

			_mbx_readreg(demod_addr, &tmp_data);

			return ((ret_data << 8) | tmp_data);
		} else
			return 0;
	} else {
		tmp_data = _riu_read_byte(reg+1);

		ret_data = tmp_data;

		tmp_data = _riu_read_byte(reg);

		return ((ret_data << 8) | tmp_data);
	}
}

static u8 _hal_vif_read_byte(u32 reg)
{
	u16 demod_addr;
	u8 u8tmp = 0;

	demod_addr = _hal_vif_get_demod_addr(reg);

	if (((demod_addr&0xFF00) != 0) && _is_ctrl_by_demod()) {
		_mbx_readreg(demod_addr, &u8tmp);
		return u8tmp;
	} else
		return _riu_read_byte(reg);
}

static void _hal_vif_write_byte_mask(u32 reg, u8 val, u8 mask)
{
	u16 demod_addr;
	u8 u8tmp;

	demod_addr = _hal_vif_get_demod_addr(reg);

	if (((demod_addr&0xFF00) != 0) && _is_ctrl_by_demod()) {
		if (_mbx_readreg(demod_addr, &u8tmp))
			_mbx_writereg(demod_addr,
				(val&mask)|(u8tmp&(~mask)));
		else
			pr_info("%s() Fail\n", __func__);
	} else
		_riu_write_byte_mask(reg, val, mask);

	hal_vif_load();
}

static void _hal_vif_write_bit(u32 reg, bool enable_b, u8 mask)
{
	u16 demod_addr;
	u8 u8tmp;

	demod_addr = _hal_vif_get_demod_addr(reg);

	if (((demod_addr&0xFF00) != 0) && _is_ctrl_by_demod()) {
		if (_mbx_readreg(demod_addr, &u8tmp)) {
			if (enable_b)
				_mbx_writereg(demod_addr, (u8tmp|mask));
			else
				_mbx_writereg(demod_addr, (u8tmp&(~mask)));
		} else {
			pr_info("%s() Fail\n", __func__);
		}
	} else
		_riu_write_bit(reg, enable_b, mask);

	hal_vif_load();
}

static void _hal_vif_write_byte(u32 reg, u8 val)
{
	u16 demod_addr;

	demod_addr = _hal_vif_get_demod_addr(reg);

	if (((demod_addr&0xFF00) != 0) && _is_ctrl_by_demod())
		_mbx_writereg(demod_addr, val);
	else
		_riu_write_byte(reg, val);

	hal_vif_load();

}
/*
 *void HAL_VIF_WriteByteMask(u32 reg, u8 val, u8 mask)
 *{
 *	_hal_vif_write_byte_mask(reg, val, mask);
 *}
 **/
/*
 *void HAL_VIF_WriteBit(u32 reg, bool enable_b, u8 mask)
 *{
 *	_hal_vif_write_bit(reg, enable_b, mask);
 *}
 **/
void hal_vif_writebyte(u32 reg, u8 val)
{
	_hal_vif_write_byte(reg, val);
}

u8 hal_vif_readbyte(u32 reg)
{
	return _hal_vif_read_byte(reg);
}

void hal_vif_reg_init(struct vif_initial_in *pvif_in,
		struct vif_notch_a1a2 *pn_a1a2,
		struct vif_sos_1112 *psos_1112,
		u16 pvif_filters[])
{
	/* TODO, move to common? */
	_hal_vif.base_addr_init_done = 1;

	/* driver struct pointers */
	_pvif_in = pvif_in;
	_pn_a1a2 = pn_a1a2;
	_psos_1112 = psos_1112;
	_pvif_filters = pvif_filters;
}

void hal_vif_adc_init(void)
{
	pr_info("%s is called\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return;

	// DMDTOP/DMDANA_controlled by HK_MCU (0) or DMD_MCU (1)
	_riu_write_byte_mask(T_101E_BASE+0x39, 0x00, 0x03);
	g_vif_downloaded = FALSE;

	// hiro add for imi register
	// reg_reserved[0] is imi share sram
	_riu_write_byte(T_101E_BASE+0x68L, 0xFC);

	// SRAM Power Control
	_riu_write_byte(T_1120_BASE+0x90L, 0x20);
	_riu_write_byte(T_1120_BASE+0x91L, 0x00);
	_riu_write_byte(T_1127_BASE+0xE0L, 0x01);
	_riu_write_byte(T_1127_BASE+0xE1L, 0x03);
	_riu_write_byte(T_1127_BASE+0xE2L, 0x00);
	_riu_write_byte(T_1127_BASE+0xE3L, 0x62);
	_riu_write_byte(T_1127_BASE+0xE4L, 0x00);
	_riu_write_byte(T_1127_BASE+0xE5L, 0x00);
	_riu_write_byte(T_1127_BASE+0xE6L, 0x00);
	_riu_write_byte(T_1127_BASE+0xE7L, 0x00);
	_riu_write_byte(T_1127_BASE+0xE8L, 0x00);
	_riu_write_byte(T_1127_BASE+0xE9L, 0x00);
	_riu_write_byte(T_1127_BASE+0xEAL, 0x00);
	_riu_write_byte(T_1127_BASE+0xEBL, 0x00);
	_riu_write_byte(T_1127_BASE+0xDEL, 0x00);
	_riu_write_byte(T_1127_BASE+0xDFL, 0x00);

	// enable vif ADC clcok
	_riu_write_byte(T_1033_BASE+0x14L, 0x04);
	_riu_write_byte(T_1033_BASE+0x15L, 0x00);

	 // enable vif DAC clock
	_riu_write_byte(T_1033_BASE+0x1AL, 0x04);
	_riu_write_byte(T_1033_BASE+0x1BL, 0x00);

	// [Maxim] enable ADCI & ADCQ clock
	_riu_write_byte(T_1033_BASE+0x20L, 0x04);
	_riu_write_byte(T_1033_BASE+0x21L, 0x00);

	// DMD_MCU clock rate = 108MHz
	_riu_write_byte(T_1033_BASE+0x1EL, 0x10);
	_riu_write_byte(T_1033_BASE+0x1FL, 0x00);

	// Start demod_0 CLKGEN setting
	// reg_clock_default_spec_sel[3:0], = 15 (select SRD)
	_riu_write_byte(T_111F_BASE+0x00L, 0x0F);
	_riu_write_byte(T_111F_BASE+0x01L, 0x00);

	// Enable VIF, DVBT, ATSC and VIF reset
	_riu_write_byte(T_1120_BASE+0x02L, 0x74);
	_riu_write_byte(T_1120_BASE+0x03L, 0x00);
	_riu_write_byte(T_1120_BASE+0x04L, 0x22);
	_riu_write_byte(T_1120_BASE+0x05L, 0x00);

	// Disable ADC sign bit
	_riu_write_byte(T_1120_BASE+0x60L, 0x04);
	_riu_write_byte(T_1120_BASE+0x61L, 0x00);

	// ADC I channel offset
	// Change unsin into sin
	// [11:0] reg_adc_offset_i
	_riu_write_byte(T_1120_BASE+0x64L, 0x00);
	_riu_write_byte(T_1120_BASE+0x65L, 0x00);

	// ADC Q channel offset
	// Change unsin into sin
	// [11:0] reg_adc_offset_q
	_riu_write_byte(T_1120_BASE+0x66L, 0x00);
	_riu_write_byte(T_1120_BASE+0x67L, 0x00);

	// VIF use DVB SRAM and FIR
	_riu_write_byte(T_1120_BASE+0xA0L, 0x01);

	// MPLL_output_div_second
	_riu_write_byte(T_1128_BASE+0x64L, 0x00);
	_riu_write_byte(T_1128_BASE+0x65L, 0x00);

	_riu_write_byte(T_1128_BASE+0x76L, 0x00);

	// Calibration buffer
	_riu_write_byte(T_1128_BASE+0x1EL, 0x80);
	_riu_write_byte(T_1128_BASE+0x1FL, 0x00);

	_riu_write_byte(T_1128_BASE+0xABL, 0x00);

	_riu_write_byte(T_1128_BASE+0xC0L, 0x00);

	// Enable LDOS
	_riu_write_byte(T_1128_BASE+0xAEL, 0x00);
	_riu_write_byte(T_1128_BASE+0xAFL, 0x20);
	_riu_write_byte(T_1128_BASE+0xB0L, 0x00);
	_riu_write_byte(T_1128_BASE+0xB1L, 0x02);
	// ana_setting_sel
	_riu_write_byte(T_1128_BASE+0xB2L, 0x11);
	_riu_write_byte(T_1128_BASE+0xB3L, 0x00);

	// Set MPLL_ADC_DIV_SE
	// DMPLL post divider
	if (VIF_IS_ADC_48MHz == 0) {
		// 144MHz case
		_riu_write_byte(T_1128_BASE+0x60L, 0x00);
		_riu_write_byte(T_1128_BASE+0x61L, 0x06);
	} else {
		// 48MHz case
		_riu_write_byte(T_1128_BASE+0x60L, 0x00);
		_riu_write_byte(T_1128_BASE+0x61L, 0x12);
	}

	// EN_VCO_DIG
	_riu_write_bit(T_1128_BASE+0x34L, 1, _BIT4);

	// Set IMUXS QMUXS
	// VIF path, Bypass PGA
	_riu_write_byte(T_1128_BASE+0x02L, 0x40);
	// Mux selection
	_riu_write_byte(T_1128_BASE+0x03L, 0x04);

	// Set enable ADC clock
	// ADC_Q power down
	_riu_write_byte(T_1128_BASE+0x18L, 0x02);
	_riu_write_byte(T_1128_BASE+0x19L, 0x00);

	// Set ADC gain is 1
	_riu_write_byte(T_1128_BASE+0x16L, 0x05);
	_riu_write_byte(T_1128_BASE+0x17L, 0x05);

	// AGC control
	// AGC enable
	_riu_write_byte(T_1128_BASE+0x30L, 0x01);
	_riu_write_byte(T_1128_BASE+0x31L, 0x00);

	// reset
	// adcd_sync_rst_ctrl
	_riu_write_byte(T_1128_BASE+0xE6L, 0x03);
	// adcd_sync
	_riu_write_byte(T_1120_BASE+0x05L, 0x01);

	// release reset
	// adcd_rst_ctrl
	_riu_write_byte(T_1128_BASE+0xE6L, 0x00);

	// Enable VIF
	_riu_write_byte(T_1120_BASE+0x02L, 0x14);
	_riu_write_byte(T_1120_BASE+0x03L, 0x00);

	// adcd_sync
	_riu_write_byte(T_1120_BASE+0x05L, 0x00);

	// Move this setting to F/W
	// reg_vif_sram_on for demod 0
	//_riu_write_byte(0x371082L, 0x00);
	//_riu_write_byte(0x371083L, 0x01);

	if (_pvif_in->vif_saw_arch == NO_SAW_DIF)
		_pvif_in->vif_saw_arch = NO_SAW;

	hal_vif_delayms(1);

	// RFAGC and IFAGC control (ADC)
	_riu_write_byte_mask(RFAGC_DATA_SEL, 0, 0x0C);    // RFAGC
	_riu_write_byte_mask(IFAGC_DATA_SEL, 0, 0xC0);    // IFAGC
	_riu_write_bit(RFAGC_ODMODE, 0, _BIT1);
	_riu_write_bit(IFAGC_ODMODE, 0, _BIT5);

	if ((_pvif_in->vif_saw_arch == SILICON_TUNER)
		|| (_pvif_in->vif_saw_arch == NO_SAW)
		|| (_pvif_in->vif_saw_arch == SAVE_PIN_VIF))
		_riu_write_bit(RFAGC_ENABLE, 0, _BIT0);   // RFAGC disable
	else
		_riu_write_bit(RFAGC_ENABLE, 1, _BIT0);   // RFAGC enable
	_riu_write_bit(IFAGC_ENABLE, 1, _BIT4);       // IFAGC enable

	// RFAGC and IFAGC control (RF)
	// 0: 1st order; 1: 2nd order
	// dither disable
	// RFAGC polarity 0: negative logic
	// RFAGC 0: BB control; 1: I2C control
	_hal_vif_write_bit(RFAGC_SEL_SECONDER, 1, _BIT6);
	_hal_vif_write_bit(RFAGC_DITHER_EN, 1, _BIT7);
	_hal_vif_write_bit(RFAGC_POLARITY, 1, _BIT4);
	_hal_vif_write_bit(OREN_RFAGC, 0, _BIT5);

	// 0: 1st order; 1: 2nd order
	// dither disable
	// IFAGC polarity 0: negative logic
	// IFAGC 0: BB control; 1: I2C control
	_hal_vif_write_bit(IFAGC_SEL_SECONDER, 1, _BIT6);
	_hal_vif_write_bit(IFAGC_DITHER_EN, 1, _BIT7);
	_hal_vif_write_bit(IFAGC_POLARITY, 1, _BIT4);
	_hal_vif_write_bit(OREN_IFAGC, 0, _BIT6);

	// Video PGA1 0: BB control; 1: I2C control
	// Video PGA2 0: BB control; 1: I2C control
	// Audio PGA1 0: BB control; 1: I2C control
	// Audio PGA2 0: BB control; 1: I2C control
	_hal_vif_write_bit(OREN_PGA1_V, 0, _BIT3);
	_hal_vif_write_bit(OREN_PGA2_V, 0, _BIT2);
	_hal_vif_write_bit(OREN_PGA1_S, 0, _BIT1);
	_hal_vif_write_bit(OREN_PGA2_S, 0, _BIT0);

	// EQ BYPASS
	_hal_vif_write_bit(BYPASS_EQFIR, 1, _BIT0);

	// DMDTOP/DMDANA_controlled by HK_MCU (0) or DMD_MCU (1)
	_riu_write_byte_mask(T_101E_BASE+0x39L, 0x03, 0x03);
}

static bool _hal_vif_ready(void)
{
	u8 data = 0x00;

	// Check if Mailbox is working or not
	_riu_write_byte(MBREG_BASE + 0x1E, 0x02);

	// assert interrupt to VD MCU51
	_riu_write_bit(DMDMCU_BASE + 0x03, 1, _BIT1);
	// de-assert interrupt to VD MCU51
	_riu_write_bit(DMDMCU_BASE + 0x03, 0, _BIT1);

	hal_vif_delayms(1);

	data = _riu_read_byte(MBREG_BASE + 0x1E);

	if (data)
		return FALSE;

	// Check if it is VIF F/W or not
	_mbx_readreg(0x20C2, &data);

	if (((data&0x0F) != 0x05) && ((data&0x0F) != 0x06))
		return FALSE;

	return TRUE;
}

static void _download_backend(void)
{
	u8 tmp_data;//take too long

	pr_info("%s()\n", __func__);

	// must follow backend register order
	tmp_data = (u8)_pvif_in->vif_top; //+0
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_if_base_freq;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_tuner_step_size;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_saw_arch;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_vga_max;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_vga_max >> 8); //+5
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_vga_min;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_vga_min >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->gain_distribute_thr;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->gain_distribute_thr >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

/*
 *	BYTE vif_top;
 *	BYTE vif_if_base_freq;
 *	BYTE vif_tuner_step_size;
 *	BYTE vif_saw_arch;
 *	U8 VifVgaMaximum_L;
 *	U8 VifVgaMaximum_H;
 *	U8 VifVgaMinimum_L;
 *	U8 VifVgaMinimum_H;
 *	U8 GainDistributionThr_L;
 *	U8 GainDistributionThr_H;
 */
	tmp_data = (u8)_pvif_in->vif_agc_vga_base; //+10
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_agc_vga_offs;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_agc_ref_negative;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_agc_ref_positive;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_dagc1_ref;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_dagc2_ref; //+15
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_dagc1_gain_ov;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_dagc1_gain_ov >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_dagc2_gain_ov;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_dagc2_gain_ov >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
/*
 *	BYTE vif_agc_vga_base;
 *	BYTE vif_agc_vga_offs;
 *	BYTE vif_agc_ref_negative;
 *	BYTE vif_agc_ref_positive;
 *	BYTE vif_dagc1_ref;
 *	BYTE vif_dagc2_ref;
 *	U8 VifDagc1GainOv_L;
 *	U8 VifDagc1GainOv_H;
 *	U8 VifDagc2GainOv_L;
 *	U8 VifDagc2GainOv_H;
 */
	tmp_data = (u8)_pvif_in->vif_cr_kf1; //+20
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_kp1;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_ki1;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_kp2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_ki2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_kp; //+25
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_ki;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_lock_thr;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_lock_thr >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_thr;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_thr >> 8); //+30
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
/*
 *	BYTE vif_cr_kf1;
 *	BYTE vif_cr_kp1;
 *	BYTE vif_cr_ki1;
 *	BYTE vif_cr_kp2;
 *	BYTE vif_cr_ki2;
 *	BYTE vif_cr_kp;
 *	BYTE vif_cr_ki;
 *	U8 vif_cr_lock_thr_l;
 *	u8 vif_cr_lock_thr_h;
 *	u8 vif_cr_thr_l;
 *	u8 vif_cr_thr_h;
 */
	tmp_data = (u8)_pvif_in->vif_cr_lock_num;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_lock_num >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_lock_num >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_lock_num >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_unlock_num;//+35
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_unlock_num >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_unlock_num >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_unlock_num >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_pd_err_max;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_pd_err_max >> 8);//+40
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
/*
 *	U8 vif_cr_lock_num_0;
 *	U8 vif_cr_lock_num_1;
 *	U8 vif_cr_lock_num_2;
 *	U8 vif_cr_lock_num_3;
 *	U8 vif_cr_unlock_num_0;
 *	U8 vif_cr_unlock_num_1;
 *	U8 vif_cr_unlock_num_2;
 *	U8 vif_cr_unlock_num_3;
 *	U8 vif_cr_pd_err_max_L;
 *	U8 vif_cr_pd_err_max_H;
 */
	tmp_data = (u8)_pvif_in->vif_cr_lock_leaky_sel;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_pd_x2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_lpf_sel;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_pd_mode_sel;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust;//+45
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust_gear;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust_thr1;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust_thr2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust_thr3;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_dynamic_top_adjust;//+50
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_dynamic_top_min;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_am_hum_detection;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
/*
 *	BOOL vif_cr_lock_leaky_sel;
 *	BOOL vif_cr_pd_x2;
 *	BOOL vif_cr_lpf_sel;
 *	BOOL vif_cr_pd_mode_sel;
 *	BOOL vif_cr_kpki_adjust;
 *	BYTE vif_cr_kpki_adjust_gear;
 *	BYTE vif_cr_kpki_adjust_thr1;
 *	BYTE vif_cr_kpki_adjust_thr2;
 *	BYTE vif_cr_kpki_adjust_thr3;
 *	BOOL vif_dynamic_top_adjust;
 *	BYTE vif_dynamic_top_min;
 *	BOOL vif_am_hum_detection;
 */
	tmp_data = (u8)_pvif_in->clampgain_clamp_sel;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_syncbott_ref;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_syncheight_ref;//+55
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_kc;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_kg;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_clamp_oren;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_gain_oren;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_clamp_ov_negative;//+60
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->clampgain_clamp_ov_negative >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_gain_ov_negative;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->clampgain_gain_ov_negative >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_clamp_ov_positive;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->clampgain_clamp_ov_positive >> 8);//+65
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_gain_ov_positive;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->clampgain_gain_ov_positive >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
/*
 *	BOOL clampgain_clamp_sel;
 *	BYTE clampgain_syncbott_ref;
 *	BYTE clampgain_syncheight_ref;
 *	BYTE clampgain_kc;
 *	BYTE clampgain_kg;
 *	BOOL clampgain_clamp_oren;
 *	BOOL clampgain_gain_oren;
 *	U8 clampgain_clamp_ov_negative_l;
 *	u8 clampgain_clamp_ov_negative_h;
 *	u8 clampgain_gain_ov_negative_l;
 *	u8 clampgain_gain_ov_negative_h;
 *	u8 clampgain_clamp_ov_positive_l;
 *	u8 clampgain_clamp_ov_positive_h;
 *	u8 clampgain_gain_ov_positive_l;
 *	u8 clampgain_gain_ov_positive_h;
 */
	tmp_data = (u8)_pvif_in->clampgain_clamp_min;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_clamp_max;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_gain_min;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_gain_max;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->clampgain_porch_cnt;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->clampgain_porch_cnt >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
/*
 *	BYTE clampgain_clamp_min;
 *	BYTE clampgain_clamp_max;
 *	BYTE clampgain_gain_min;
 *	BYTE clampgain_gain_max;
 *	U8 clampgain_porch_cnt_L;
 *	U8 clampgain_porch_cnt_H;
 */

/*
 *	BYTE china_descramble_box;
 *	BYTE vif_delay_reduce;
 *	BOOL vif_overmodulation;
 *	BOOL vif_overmodulation_detect;
 *	BOOL vif_aci_detect;
 *	BOOL vif_serious_aci_detect;
 *	BYTE vif_aci_agc_ref;
 *	BYTE vif_adc_overflow_agc_ref;
 *	BYTE vif_channel_scan_agc_ref;
 *	BYTE vif_aci_det_thr1;
 *	BYTE vif_aci_det_thr2;
 *	BYTE vif_aci_det_thr3;
 *	BYTE vif_aci_det_thr4;
 */
	tmp_data = (u8)_pvif_in->china_descramble_box;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_delay_reduce;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_overmodulation;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_overmodulation_detect;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_aci_detect;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_serious_aci_detect;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_aci_agc_ref;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_adc_overflow_agc_ref;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_channel_scan_agc_ref;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_aci_det_thr1;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_aci_det_thr2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_aci_det_thr3;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_aci_det_thr4;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	/* vif_rf_band */
	tmp_data = (u8)_pvif_in->vif_rf_band;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	/*BYTE vif_tuner_type;*/
	tmp_data = (u8)_pvif_in->vif_tuner_type;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	/*CrRate(4)+CrInv(1) for (B,GH,DK,I,L,LL,MN)*/
	tmp_data = (u8)_pvif_in->vif_cr_rate_b;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_b >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_b >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_b >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_inv_b;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	tmp_data = (u8)_pvif_in->vif_cr_rate_gh;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_gh >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_gh >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_gh >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_inv_gh;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	tmp_data = (u8)_pvif_in->vif_cr_rate_dk;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_dk >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_dk >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_dk >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_inv_dk;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	tmp_data = (u8)_pvif_in->vif_cr_rate_i;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_i >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_i >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_i >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_inv_i;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	tmp_data = (u8)_pvif_in->vif_cr_rate_l;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_l >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_l >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_l >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_inv_l;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	tmp_data = (u8)_pvif_in->vif_cr_rate_ll;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_ll >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_ll >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_ll >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_inv_ll;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	tmp_data = (u8)_pvif_in->vif_cr_rate_mn;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_mn >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_mn >> 16);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pvif_in->vif_cr_rate_mn >> 24);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pvif_in->vif_cr_inv_mn;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	/* vif_reserve */
	tmp_data = (u8)_pvif_in->vif_reserve;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);


	/* N_a1 C0 to C2, N_a2 C0 to C2 */
	/* 228th to 239th byte */
	tmp_data = (u8)_pn_a1a2->vif_n_a1_c0;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pn_a1a2->vif_n_a1_c0 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pn_a1a2->vif_n_a1_c1;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pn_a1a2->vif_n_a1_c1 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pn_a1a2->vif_n_a1_c2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pn_a1a2->vif_n_a1_c2 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pn_a1a2->vif_n_a2_c0;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pn_a1a2->vif_n_a2_c0 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pn_a1a2->vif_n_a2_c1;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pn_a1a2->vif_n_a2_c1 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_pn_a1a2->vif_n_a2_c2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_pn_a1a2->vif_n_a2_c2 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);

	/* Sos11 C0 to C4, Sos12 C0 to C4 */
	/* 240th to 259th byte */
	tmp_data = (u8)_psos_1112->vif_sos_11_c0;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_11_c0 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_11_c1;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_11_c1 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_11_c2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_11_c2 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_11_c3;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_11_c3 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_11_c4;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_11_c4 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_12_c0;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_12_c0 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_12_c1;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_12_c1 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_12_c2;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_12_c2 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_12_c3;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_12_c3 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)_psos_1112->vif_sos_12_c4;
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	tmp_data = (u8)(_psos_1112->vif_sos_12_c4 >> 8);
	_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
}

bool hal_vif_filter_coef_download(void)
{
	u8 data = 0x00;
	u16 address_offset;
	u16 i = 0;
	u16 u16tmp = 0;
	u32 fw_start_time;

	// External User-defined filter coefficients
	// over-write original filter coefficients
	if (_pvif_filters[0] != 0) {

		//pr_debug("_pvif_filters[0] = %x\n", _pvif_filters[0]);

		//Reset FSM
		fw_start_time = hal_vif_get_sys_time();
		_mbx_writereg(0x20C0, 0x00);
		//printf("\r\n[Reset State Machine][Time: %ld]",fw_start_time);

		while ((data != 0x30) && (data != 0x02)) {
			_mbx_readreg(0x20C1, &data);

			if ((hal_vif_get_sys_time()-fw_start_time) > 20) {
				pr_info("Over 20ms\n");
				break;
			}
		}

		_riu_write_byte(DMDMCU_BASE+0x00, 0x01); // reset VD_MCU
		_riu_write_byte(DMDMCU_BASE+0x01, 0x00); // disable SRAM
		// release MCU, madison patch
		_riu_write_byte(DMDMCU_BASE+0x00, 0x00);

		_riu_write_byte(DMDMCU_BASE+0x03, 0x50); // enable "vdmcu51_if"
		_riu_write_byte(DMDMCU_BASE+0x03, 0x51); // enable auto-increase
		// sram address low byte & high byte
		_riu_write_byte(DMDMCU_BASE+0x04, 0x00);
		_riu_write_byte(DMDMCU_BASE+0x05, 0x00);

		//Set Filter coefficient
		address_offset = (intern_vif_table[0x40C] << 8)
				|intern_vif_table[0x40D];
		address_offset = ((intern_vif_table[address_offset] << 8)
				|intern_vif_table[address_offset+1]);
		_riu_write_byte(DMDMCU_BASE+0x04, (address_offset&0xFF));
		_riu_write_byte(DMDMCU_BASE+0x05, (address_offset>>8));


		for (i = 0; i < EQ_COEF_LEN; i++) {
			u16tmp = _pvif_filters[i];
			// write data to VD MCU 51 code sram
			_riu_write_byte(DMDMCU_BASE+0x0C, (u16tmp >> 8));
			_riu_write_byte(DMDMCU_BASE+0x0C, (u16tmp & 0xFF));
		}

		_riu_write_byte(DMDMCU_BASE+0x03, 0x50); //disable auto-increase
		_riu_write_byte(DMDMCU_BASE+0x03, 0x00); // disable "vdmcu51_if"
		// reset MCU, madison patch
		_riu_write_byte(DMDMCU_BASE+0x00, 0x01);

		_riu_write_byte(DMDMCU_BASE+0x01, 0x01); // enable SRAM
		_riu_write_byte(DMDMCU_BASE+0x00, 0x00); // release VD_MCU

		_mbx_writereg(0x20C0, 0x04);
	}

	return TRUE;
}

bool hal_vif_download(void)
{
	u8 udata = 0x00;
	u8 tmp_data = 0x00;
	u8 u8tmp2 = 0x00;

	u16 i = 0;
	u16 fail_cnt = 0;
	u16 address_offset;
	const u8 *vif_table;
	u16 lib_size;

	u32 fw_start_time;
	u8 data = 0x00;

	pr_info("%s is called\n", __func__);

	//if exit not called...
	if (g_vif_downloaded == TRUE) {
		_riu_write_byte(DMDMCU_BASE+0x01, 0x01); //enable DMD MCU51 SRAM
		_riu_write_byte_mask(DMDMCU_BASE+0x00, 0x00, 0x01);

		hal_vif_delayms(1);

		if (_hal_vif_ready() == FALSE) {
			g_vif_downloaded = FALSE;

			g_vif_prev_scan_flag = FALSE;

			hal_vif_adc_init();

			pr_info("\n=== %s/Re-load F/W ===\n", __func__);
		} else {
			// reset VD_MCU
			_riu_write_byte_mask(DMDMCU_BASE+0x00, 0x01, 0x01);
			_riu_write_byte_mask(DMDMCU_BASE+0x00, 0x00, 0x03);

			pr_info("\n=== %s/Not Re-load F/W ===\n", __func__);

			return TRUE;
		}
	}

	//when ready == false
	{
		vif_table = &intern_vif_table[0];
		lib_size = sizeof(intern_vif_table);

		_riu_write_byte(DMDMCU_BASE+0x00, 0x01); // reset VD_MCU
		_riu_write_byte(DMDMCU_BASE+0x01, 0x00); // disable SRAM
		// release MCU, madison patch
		_riu_write_byte(DMDMCU_BASE+0x00, 0x00);

		_riu_write_byte(DMDMCU_BASE+0x03, 0x50); // enable "vdmcu51_if"
		_riu_write_byte(DMDMCU_BASE+0x03, 0x51); // enable auto-increase
		// sram address low byte & high byte
		_riu_write_byte(DMDMCU_BASE+0x04, 0x00);
		_riu_write_byte(DMDMCU_BASE+0x05, 0x00);

		////  Load code thru VDMCU_IF ////
		pr_info(">Load Code...\n");

		for (i = 0; i < lib_size; i++) {
			// write data to VD MCU 51 code sram
			_riu_write_byte(DMDMCU_BASE+0x0C, vif_table[i]);
		}

		////  Content verification ////
		pr_info(">Verify Code...\n");

		// sram address low byte & high byte
		_riu_write_byte(DMDMCU_BASE+0x04, 0x00);
		_riu_write_byte(DMDMCU_BASE+0x05, 0x00);

		for (i = 0; i < lib_size; i++) {
			// read sram data
			udata = _riu_read_byte(DMDMCU_BASE+0x10);

			if (udata != vif_table[i]) {
				pr_info(">fail add = 0x%x\n", i);
				pr_info(">code = 0x%x\n", vif_table[i]);
				pr_info(">data = 0x%x\n", udata);

				if (fail_cnt++ > 10) {
					pr_info(">DSP Loadcode fail!\n");
					return FALSE;
				}
			}
		}

		address_offset = (vif_table[0x400] << 8)|vif_table[0x401];
		// sram address low byte
		_riu_write_byte(DMDMCU_BASE+0x04, (address_offset&0xFF));
		// sram address high byte
		_riu_write_byte(DMDMCU_BASE+0x05, (address_offset>>8));

		_download_backend();

		_riu_write_byte(DMDMCU_BASE+0x03, 0x50); //disable auto-increase
		_riu_write_byte(DMDMCU_BASE+0x03, 0x00); // disable "vdmcu51_if"
		// reset MCU, madison patch
		_riu_write_byte(DMDMCU_BASE+0x00, 0x01);

		_riu_write_byte(DMDMCU_BASE+0x01, 0x01); // enable SRAM
		_riu_write_byte(DMDMCU_BASE+0x00, 0x00); // release VD_MCU

		hal_vif_delayms(20);

		pr_info(">DSP Loadcode done.\n");
		g_vif_downloaded = TRUE;
		//_mbx_writereg(0x20C2, 0x06);
		_mbx_readreg(0x20C2, &udata);
		_mbx_writereg(0x20C2, (u8)((udata&0xF0)|0x06));

		//Init state, mailbox 0x0080(does it work?)
		_mbx_readreg(0x0080, &tmp_data);
		_mbx_readreg(0x0081, &u8tmp2);
		//be carefull, endian
		address_offset = (tmp_data << 8)|(u8tmp2);
		//start with state MST_STATE_DMD_RST
		_mbx_writereg(address_offset, 0x02);

		//Reset FSM
		fw_start_time = os_get_systime_ms();
		_mbx_writereg(0x20C0, 0x00);
		//printk("\r\n[Reset State Machine][Time: %ld]",fw_start_time);

		while ((data != 0x30) && (data != 0x02)) {
			_mbx_readreg(0x20C1, &data);

			if (os_get_systime_ms()-fw_start_time > 20) {
				pr_info("Over 20ms\n");
				break;
			}
		}
		_mbx_writereg(0x20C0, 0x04);

		return TRUE;
	}
}

void hal_vif_set_if_freq(enum if_freq_type ucIfFreq)
{
	pr_info("%s is called\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return;

	if (!g_vif_downloaded) {
		pr_debug("VIF NOT DOWNLOADED\n");
		return;
	}

	//g_FreqType = ucIfFreq; // 0x1121_D2
	_hal_vif_write_byte(VIF_RF_RESERVED_1, ucIfFreq);

	// Utopia driver version control
	_hal_vif_write_byte(FIRMWARE_VERSION_L, 0x02);    // 02(dd)
	_hal_vif_write_byte(FIRMWARE_VERSION_H, 0x43);    // 04/19 (mm/yy)

	_hal_vif_write_bit(RF_LOAD, 1, _BIT0);
	_hal_vif_write_bit(DBB1_LOAD, 1, _BIT0);

}

void hal_vif_set_sound_sys(enum vif_sound_sys sound_sys, u32 if_freq)
{
	u8 u8tmp = 0;

	pr_info("%s() sound_sys=%d\n", __func__, sound_sys);

	if (!_hal_vif.base_addr_init_done)
		return;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return;
	}

	/* TODO: get IF freq from tuner, and overwrite default setting */
	if (if_freq != 0) {
		switch (sound_sys) {
		case VIF_SOUND_B:
			_pvif_in->vif_cr_rate_b;
			_pvif_in->vif_cr_inv_b;
			break;
		case VIF_SOUND_GH:
			_pvif_in->vif_cr_rate_gh;
			_pvif_in->vif_cr_inv_gh;
			break;
		case VIF_SOUND_DK2:
			_pvif_in->vif_cr_rate_dk;
			_pvif_in->vif_cr_inv_dk;
			break;
		case VIF_SOUND_I:
			_pvif_in->vif_cr_rate_i;
			_pvif_in->vif_cr_inv_i;
			break;
		case VIF_SOUND_MN:
			_pvif_in->vif_cr_rate_mn;
			_pvif_in->vif_cr_inv_mn;
			break;
		case VIF_SOUND_L:
			_pvif_in->vif_cr_rate_l;
			_pvif_in->vif_cr_inv_l;
			break;
		case VIF_SOUND_LL:
			_pvif_in->vif_cr_rate_ll;
			_pvif_in->vif_cr_inv_ll;
			break;
		default:
			break;
		}
	}

	hal_vif_update_fw_backend();

	//High 4 bits is Sound standard, low 4 bits is TV mode.
	_mbx_writereg(0x20C2, (u8)(((sound_sys<<4)&0xF0) | 0x06));

	//vif_rf_band // 0x1121_D4, low 4 bits
	//bit4 is trigger bit, utopia set when set sound is called
	//and FW clear when write reg is done
	u8tmp = (0x10) | (_pvif_in->vif_rf_band & 0x0F);
	_hal_vif_write_byte((RF_REG_BASE+0xD4), u8tmp);

}

void hal_vif_top_adjust(void)
{
	pr_info("%s is called\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return;
	}

	if (_pvif_in->vif_tuner_type == 0) {
		_hal_vif_write_byte_mask(AGC_PGA2_MIN, _pvif_in->vif_top, 0x1F);
		_hal_vif_write_byte_mask(AGC_PGA2_OV, _pvif_in->vif_top, 0x1F);
		_hal_vif_write_bit(AGC_PGA2_OREN, 1, _BIT1);
		_hal_vif_write_bit(AGC_PGA2_OREN, 0, _BIT1);
	}
}


void hal_vif_load(void)
{
	if (!_hal_vif.base_addr_init_done)
		return;

	if (_is_ctrl_by_demod())
		return;
	//else {
		_riu_write_bit(RF_LOAD, 1, _BIT0);
		_riu_write_bit(DBB1_LOAD, 1, _BIT0);
		_riu_write_bit(DBB2_LOAD, 1, _BIT0);
		_riu_write_bit(DBB2_LOAD, 0, _BIT0);

		return;
	//}
}

void hal_vif_exit(void)
{
	u8 check_count = 0;

	pr_info("%s is called\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return;
	}

	_riu_write_byte(MBREG_BASE + 0x1C, 0x01);

	// assert interrupt to VD MCU51
	_riu_write_bit(DMDMCU_BASE + 0x03, 1, _BIT1);
	// de-assert interrupt to VD MCU51
	_riu_write_bit(DMDMCU_BASE + 0x03, 0, _BIT1);

	while ((_riu_read_byte(MBREG_BASE + 0x1C) & 0x02) != 0x02) {
		hal_vif_delayus(10);

		if (check_count++ == 0xFF) {
			pr_info(">> VIF Exit Fail!\n");
			return;
		}
	}

	pr_info(">> VIF Exit Ok!\n");
	g_vif_downloaded = FALSE;

	if (g_vif_downloaded == FALSE) {
		// DMDTOP/DMDANA_controlled by HK_MCU (0) or DMD_MCU (1)
		_riu_write_byte_mask(T_101E_BASE+0x39L, 0x00, 0x03);

		// Recovery setting to default.
		_riu_write_byte(T_101E_BASE+0x82L, 0x00);
		_riu_write_byte(T_101E_BASE+0x83L, 0x00);
		//MsOS_DelayTaskUs(5);
		hal_vif_delayus(5);
		_riu_write_byte(T_101E_BASE+0x82L, 0x11);
		_riu_write_byte(T_101E_BASE+0x83L, 0x00);

		// dmd_ana_misc setting recover
		_riu_write_byte(T_1128_BASE+0x02L, 0x0F);
		_riu_write_byte(T_1128_BASE+0x03L, 0x00);
		_riu_write_byte(T_1128_BASE+0x16L, 0x00);
		_riu_write_byte(T_1128_BASE+0x17L, 0x00);
		_riu_write_byte(T_1128_BASE+0x18L, 0x07);
		_riu_write_byte(T_1128_BASE+0x19L, 0x00);
		_riu_write_byte(T_1128_BASE+0x1EL, 0x80);
		_riu_write_byte(T_1128_BASE+0x1FL, 0x00);
		_riu_write_byte(T_1128_BASE+0x30L, 0x00);
		_riu_write_byte(T_1128_BASE+0x31L, 0x00);
		_riu_write_byte(T_1128_BASE+0x34L, 0x10);
		_riu_write_byte(T_1128_BASE+0x35L, 0x00);
		_riu_write_byte(T_1128_BASE+0x60L, 0x80);
		_riu_write_byte(T_1128_BASE+0x61L, 0x24);
		_riu_write_byte(T_1128_BASE+0x64L, 0x00);
		_riu_write_byte(T_1128_BASE+0x65L, 0x00);
		_riu_write_byte(T_1128_BASE+0x6CL, 0x00);
		_riu_write_byte(T_1128_BASE+0x6DL, 0x00);
		_riu_write_byte(T_1128_BASE+0x76L, 0x00);
		_riu_write_byte(T_1128_BASE+0x77L, 0x00);
		_riu_write_byte(T_1128_BASE+0xAAL, 0x00);
		_riu_write_byte(T_1128_BASE+0xABL, 0x00);
		_riu_write_byte(T_1128_BASE+0xAEL, 0x58);
		_riu_write_byte(T_1128_BASE+0xAFL, 0x20);
		_riu_write_byte(T_1128_BASE+0xB0L, 0x00);
		_riu_write_byte(T_1128_BASE+0xB1L, 0x03);
		_riu_write_byte(T_1128_BASE+0xB2L, 0x00);
		_riu_write_byte(T_1128_BASE+0xB3L, 0x00);
		_riu_write_byte(T_1128_BASE+0xC0L, 0x00);
		_riu_write_byte(T_1128_BASE+0xC1L, 0x00);
		_riu_write_byte(T_1128_BASE+0xE2L, 0x00);
		_riu_write_byte(T_1128_BASE+0xE3L, 0x00);
		_riu_write_byte(T_1128_BASE+0xE6L, 0x00);
		_riu_write_byte(T_1128_BASE+0xE7L, 0x00);

		pr_info(">> VIF Exit2 Ok!\n");
	}
}

void hal_vif_set_scan(u8 is_scanning)
{
	pr_info("%s is called\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return;
	}

	if (is_scanning != g_vif_prev_scan_flag) {
		//is_scanning at 0x1121_D5
		_hal_vif_write_byte(VIF_RF_RESERVED_2+1, is_scanning);
		g_vif_prev_scan_flag = is_scanning;
	}
}

u8 hal_vif_read_foe(void)
{
	pr_info("%s()\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return 0;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return 0x80;
	}

	return _riu_read_byte(MBREG_BASE + 0x1F) & 0xFF;
}

u8 hal_vif_read_lock_status(void)
{
	pr_info("%s()\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return 0;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return 0;
	}

	return _riu_read_byte(MBREG_BASE + 0x1D) & 0x01;
}

void hal_vif_shift_clk(u8 vif_shift_clk)
{
	pr_info("%s() = %x\n", __func__, vif_shift_clk);

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return;
	}

	if (VIF_IS_ADC_48MHz == 0) {
		if (vif_shift_clk == 1) {
			//g_VifShiftClk = 1; // 0x1121_D3
			_hal_vif_write_byte(VIF_RF_RESERVED_1+1, 0x01);
			//loop divider
			_hal_vif_write_byte(T_1128_BASE+0x66L, 0x00);
			_hal_vif_write_byte(T_1128_BASE+0x67L, 0x23);
			if (_pvif_in->vif_tuner_type == 0) {
				// move to clk 42 Mhz
				// cr_rate for 15 MHz
				_hal_vif_write_byte(CR_RATE, 0x6D);
				_hal_vif_write_byte(CR_RATE+1, 0xDB);
				_hal_vif_write_byte_mask(CR_RATE+2, 0x16, 0x1F);
				// cr_rate not invert
				_hal_vif_write_bit(CR_RATE_INV, 0, _BIT0);

				// move to clk 140 Mhz
				// IF rate for 23 MHz
				_hal_vif_write_byte(IF_RATE, 0xA8);
				_hal_vif_write_byte(IF_RATE+1, 0x83);
				_hal_vif_write_byte_mask(IF_RATE+2, 0x0A, 0x3F);
			}
		} else if (vif_shift_clk == 2) {
			//g_VifShiftClk = 2; // 0x1121_D3
			_hal_vif_write_byte(VIF_RF_RESERVED_1+1, 0x02);
			//loop divider
			_hal_vif_write_byte(T_1128_BASE+0x66L, 0x00);
			_hal_vif_write_byte(T_1128_BASE+0x67L, 0x25);
			if (_pvif_in->vif_tuner_type == 0) {
				// move to clk 44.4 Mhz
				// cr_rate for 15 MHz
				_hal_vif_write_byte(CR_RATE, 0x22);
				_hal_vif_write_byte(CR_RATE+1, 0x9F);
				_hal_vif_write_byte_mask(CR_RATE+2, 0x15, 0x1F);
				// cr_rate not invert
				_hal_vif_write_bit(CR_RATE_INV, 0, _BIT0);

				// move to clk 148 Mhz
				// IF rate for 23 MHz
				_hal_vif_write_byte(IF_RATE, 0x29);
				_hal_vif_write_byte(IF_RATE+1, 0xF2);
				_hal_vif_write_byte_mask(IF_RATE+2, 0x09, 0x3F);
			}
		} else {
			//g_VifShiftClk = 0; // 0x1121_D3
			_hal_vif_write_byte(VIF_RF_RESERVED_1+1, 0x00);
			//loop divider
			_hal_vif_write_byte(T_1128_BASE+0x66L, 0x00);
			_hal_vif_write_byte(T_1128_BASE+0x67L, 0x24);

			if (_pvif_in->vif_tuner_type == 0) {
				// move to clk 43.2 Mhz
				// cr_rate for 15 MHz
				_hal_vif_write_byte(CR_RATE, 0xE3);
				_hal_vif_write_byte(CR_RATE+1, 0x38);
				_hal_vif_write_byte_mask(CR_RATE+2, 0x16, 0x1F);
				// cr_rate not invert
				_hal_vif_write_bit(CR_RATE_INV, 0, _BIT0);

				// move to clk 142 Mhz
				// IF rate for 23 MHz
				_hal_vif_write_byte(IF_RATE, 0xE3);
				_hal_vif_write_byte(IF_RATE+1, 0x38);
				_hal_vif_write_byte_mask(IF_RATE+2, 0x0A, 0x3F);
			}
		}
		_hal_vif_write_bit(RF_LOAD, 1, _BIT0);
		_hal_vif_write_bit(DBB1_LOAD, 1, _BIT0);
		_hal_vif_write_bit(DBB2_LOAD, 1, _BIT0);
		_hal_vif_write_bit(DBB2_LOAD, 0, _BIT0);
	}
}

void hal_vif_update_fw_backend(void)
{
	u16 address_offset;
	const u8 *vif_table;
	//u16 lib_size;
	u32 fw_start_time;
	u8 data = 0x00;


	vif_table = &intern_vif_table[0];
	//lib_size = sizeof(intern_vif_table);

	_riu_write_byte(DMDMCU_BASE+0x00, 0x01); // reset VD_MCU
	_riu_write_byte(DMDMCU_BASE+0x01, 0x00); // disable SRAM

	_riu_write_byte(DMDMCU_BASE+0x00, 0x00); // release MCU, madison patch

	_riu_write_byte(DMDMCU_BASE+0x03, 0x50); // enable "vdmcu51_if"
	_riu_write_byte(DMDMCU_BASE+0x03, 0x51); // enable auto-increase
	_riu_write_byte(DMDMCU_BASE+0x04, 0x00); // sram address low byte
	_riu_write_byte(DMDMCU_BASE+0x05, 0x00); // sram address high byte

	////  Load code thru VDMCU_IF ////

	address_offset = (vif_table[0x400] << 8)|vif_table[0x401];
	// sram address low byte
	_riu_write_byte(DMDMCU_BASE+0x04, (address_offset&0xFF));
	// sram address high byte
	_riu_write_byte(DMDMCU_BASE+0x05, (address_offset>>8));

	_download_backend();

	_riu_write_byte(DMDMCU_BASE+0x03, 0x50); // disable auto-increase
	_riu_write_byte(DMDMCU_BASE+0x03, 0x00); // disable "vdmcu51_if"

	_riu_write_byte(DMDMCU_BASE+0x00, 0x01); // reset MCU, madison patch

	_riu_write_byte(DMDMCU_BASE+0x01, 0x01); // enable SRAM
	_riu_write_byte(DMDMCU_BASE+0x00, 0x00); // release VD_MCU

	//_mbx_writereg(0x20C0, 0x00);//reset state machine
	//MsOS_DelayTask(20);
	//DTV tune result. MODIFY: MEASURE AGAIN or WAIT 0x0200 in WHILE LOOP
	//hal_vif_delayms(20);
	//_mbx_writereg(0x20C0, 0x04);//start state machine

	//Reset FSM
	fw_start_time = os_get_systime_ms();
	_mbx_writereg(0x20C0, 0x00);
	//pr_info("\n[Reset State Machine][Time: %ld]\n",fw_start_time);

	while ((data != 0x30) && (data != 0x02)) {
		_mbx_readreg(0x20C1, &data);

		if (os_get_systime_ms() - fw_start_time > 20) {
			pr_info("Over 20ms\n");
			break;
		}
	}
	//pr_info("\n[Start State Machine][Time: %ld]\n",os_get_systime_ms());
	_mbx_writereg(0x20C0, 0x04);
}

