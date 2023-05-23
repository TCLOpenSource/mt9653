// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <asm/io.h>

#include "demod_hal_vif.h"
#include "demod_hal_reg_vif.h"

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
#define MBREG_BASE  0x5E00//0x112600  //0x112600 //in "m7332_demod_common.h"
#define DMDMCU_BASE 0x62300//0x103480  //0x103480
#define DMDMCU2_BASE 0x62400

//#define T_101E_BASE 0x101E00
//#define T_1033_BASE 0x103300
//#define T_111F_BASE 0x111F00
//#define T_1120_BASE 0x112000
//#define T_1127_BASE 0x112700
//#define T_1128_BASE 0x112800

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
	mtk_demod_write_byte(reg, ((mtk_demod_read_byte(reg) & (~msk)) | (val & msk)))
#define _riu_write_bit(reg, en_bool, msk)\
	((en_bool) ? (mtk_demod_write_byte(reg, (mtk_demod_read_byte(reg) | msk))) :\
	(mtk_demod_write_byte(reg, (mtk_demod_read_byte(reg) & (~msk)))))


#define os_get_systime_ms()        mtk_demod_get_time()

#define VIF_FW_RUN_ON_DRAM_MODE     0

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
#if !VIF_FW_RUN_ON_DRAM_MODE
const u8 intern_vif_table[] = {
	#include "dmd_vif_sram.dat"
};
#else
const u8 intern_vif_table[] = {
	#include "dmd_vif_dram.dat"
};
#endif

static u8 g_vif_prev_scan_flag;

/*
 * VIF_IS_ADC_48MHz
 * 0:144MHz, 1:48MHz
 */
#define VIF_IS_ADC_48MHz   1

#define DRIVER_VER_L    0x08   // 08 (dd)
#define DRIVER_VER_H    0x95  // 09/21 (mm/yy)

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

static u16 _hal_vif_read_2bytes(u32 reg)
{
	//2 bytes were read at different time, could be a trouble
	u16 demod_addr;
	u8 tmp_data = 0;
	u16 ret_data = 0;

	demod_addr = _hal_vif_get_demod_addr(reg);

	if (((demod_addr&0xFF00) != 0) && g_vif_downloaded) {
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

	if (((demod_addr&0xFF00) != 0) && g_vif_downloaded) {
		_mbx_readreg(demod_addr, &u8tmp);
		return u8tmp;
	} else
		return _riu_read_byte(reg);
}

static void _hal_vif_write_bit(u32 reg, bool enable_b, u8 mask)
{
	u16 demod_addr;
	u8 u8tmp;

	demod_addr = _hal_vif_get_demod_addr(reg);

	if (((demod_addr&0xFF00) != 0) && g_vif_downloaded) {
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

	if (((demod_addr&0xFF00) != 0) && g_vif_downloaded)
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
	#if VIF_FW_RUN_ON_DRAM_MODE
	u8 reg = 0;
	u16 temp = 0;
	#endif
	u32 temp2 = 0;

	pr_info("%s is called\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return;

	// demod ckgen enable
	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x14E4) & 0xFFFFFDBF;
	temp2 |= 0x00000240;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x14E4));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x1D8) & 0xFFFFFCF8;
	temp2 |= 0x00000004;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x1D8));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x2B8) & 0xFFFFFFF8;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x2B8));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x2C0) & 0xFFFFFFFC;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x2C0));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x2C8) & 0xFFFFFFFC;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x2C8));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x2D8) & 0xFFFFFFFC;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x2D8));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x510) & 0xFFFFFFFC;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x510));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0xAB8) & 0xFFFFFFFC;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0xAB8));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x1598) & 0xFFFFFFFE;
	temp2 |= 0x00000001;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x1598));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x1740) & 0xFFFFFFFE;
	temp2 |= 0x00000001;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x1740));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x19C8) & 0xFFFFFFFE;
	temp2 |= 0x00000001;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x19C8));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x19CC) & 0xFFFFFFFE;
	temp2 |= 0x00000001;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x19CC));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x19D0) & 0xFFFFFFFE;
	temp2 |= 0x00000001;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x19D0));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x19D4) & 0xFFFFFFFE;
	temp2 |= 0x00000001;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x19D4));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x3040) & 0xFFFFFCE0;
	temp2 |= 0x00000004;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x3040));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x3110) & 0xFFFFFCE0;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x3110));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x31C0) & 0xFFFFFFFC;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x31C0));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x3B4C) & 0xFFFFFFDF;
	temp2 |= 0x00000020;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x3B4C));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x3B5C) & 0xFFFFFFFE;
	temp2 |= 0x00000001;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x3B5C));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x3B74) & 0xFFFFFFFB;
	temp2 |= 0x00000004;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x3B74));

	// Start IMI SRAM initial setting
	// M6 version : 0x0100 / M6L version : 0x0200
	if (_pvif_in->ip_version > 0x0100) {
		//[0:0]
		temp2 = readl(_pvif_in->virt_sram_base_addr) & 0xFFFFFFFE;
		writel(temp2, _pvif_in->virt_sram_base_addr);

		//[3:0]
		temp2 = readl(_pvif_in->virt_pmux_base_addr) & 0xFFFFFFF0;
		temp2 |= 0x00000003;
		writel(temp2, _pvif_in->virt_pmux_base_addr);

		//[11:8]
		temp2 = readl(_pvif_in->virt_vagc_base_addr) & 0xFFFFF0FF;
		temp2 |= 0x00000300;
		writel(temp2, _pvif_in->virt_vagc_base_addr);
	} else
		writel(0x000000FC, _pvif_in->virt_sram_base_addr);

	// Start TOP CLKGEN initial setting
	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x1F94) & 0xFFFFFFF0;
	temp2 |= 0x00000004;
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x1F94));

	temp2 = readl(_pvif_in->virt_clk_base_addr + 0x3110) & 0xFFFFFCE0;
	#if VIF_FW_RUN_ON_DRAM_MODE
	//temp2 |= 0x00000004;
	temp2 |= _BIT2;
	#else
	//temp2 |= 0x00000014;
	temp2 |= (_BIT2 | _BIT4);
	#endif
	writel(temp2, (_pvif_in->virt_clk_base_addr + 0x3110));

	#if VIF_FW_RUN_ON_DRAM_MODE
	//HAL_DMD_RIU_WriteByte(0x153de0,0x23);
	//HAL_DMD_RIU_WriteByte(0x153de1,0x21);
	_riu_write_byte(DMDMCU2_BASE + 0xE0, 0x23);
	_riu_write_byte(DMDMCU2_BASE + 0xE1, 0x21);

	//HAL_DMD_RIU_WriteByte(0x153de4,0x01);
	//HAL_DMD_RIU_WriteByte(0x153de6,0x11);
	_riu_write_byte(DMDMCU2_BASE + 0xE4, 0x01);
	_riu_write_byte(DMDMCU2_BASE + 0xE6, 0x11);

	//HAL_DMD_RIU_WriteByte(0x153d00,0x00);
	//HAL_DMD_RIU_WriteByte(0x153d01,0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x00, 0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x01, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d04,0x00);
	//HAL_DMD_RIU_WriteByte(0x153d05,0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x04, 0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x05, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d02,0x00);
	//HAL_DMD_RIU_WriteByte(0x153d03,0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x02, 0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x03, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d06,(INTERNAL_DVBT2_DRAM_OFFSET-1)&0xff);
	//HAL_DMD_RIU_WriteByte(0x153d07,(INTERNAL_DVBT2_DRAM_OFFSET-1)>>8);
	_riu_write_byte(DMDMCU2_BASE + 0x06, 0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x07, 0x00);

	//u32DMD_DVBT2_FW_START_ADDR -= 0x5000;
	//u16_temp_val = (MS_U16)(u32DMD_DVBT2_FW_START_ADDR>>16);
	//HAL_DMD_RIU_WriteByte(0x153d1a,(MS_U8)u16_temp_val);
	//HAL_DMD_RIU_WriteByte(0x153d1b,(MS_U8)(u16_temp_val>>8));
	temp = (u16)((_pvif_in->dram_base_addr * 16)>>16);
	_riu_write_byte(DMDMCU2_BASE + 0x1A, (u8)temp);
	_riu_write_byte(DMDMCU2_BASE + 0x1B, (u8)(temp>>8));

	temp = (u16)((_pvif_in->dram_base_addr * 16)>>32);
	_riu_write_byte(DMDMCU2_BASE + 0x20, (u8)temp);

	//HAL_DMD_RIU_WriteByte(0x153D08,0x00);
	//HAL_DMD_RIU_WriteByte(0x153d09,0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x08, 0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x09, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d0c,INTERNAL_DVBT2_DRAM_OFFSET&0xff);
	//HAL_DMD_RIU_WriteByte(0x153d0d,INTERNAL_DVBT2_DRAM_OFFSET>>8);
	_riu_write_byte(DMDMCU2_BASE + 0x0C, 0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x0D, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d0a,0x01);
	//HAL_DMD_RIU_WriteByte(0x153d0b,0x00);
	_riu_write_byte(DMDMCU2_BASE + 0x0A, 0x01);
	_riu_write_byte(DMDMCU2_BASE + 0x0B, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d0e,0xff);
	//HAL_DMD_RIU_WriteByte(0x153d0f,0xff);
	_riu_write_byte(DMDMCU2_BASE + 0x0E, 0xff);
	_riu_write_byte(DMDMCU2_BASE + 0x0F, 0xff);

	//HAL_DMD_RIU_WriteByte(0x153d18,0x04);
	_riu_write_byte(DMDMCU2_BASE + 0x18, 0x04);

	//u8_temp_val = HAL_DMD_RIU_ReadByte(0x112001);
	//u8_temp_val &= (~0x10);
	//HAL_DMD_RIU_WriteByte(0x112001, u8_temp_val);
	reg = _riu_read_byte(DMDTOP_REG_BASE + 0x01);
	reg &= (~0x10);
	_riu_write_byte(DMDTOP_REG_BASE + 0x01, reg);

	//HAL_DMD_RIU_WriteByte(0x153d1c,0x01);
	_riu_write_byte(DMDMCU2_BASE + 0x1C, 0x01);
	#endif

	// M6 version : 0x0100 / M6L version : 0x0200
	if (_pvif_in->ip_version > 0x0100)
		_riu_write_byte(MBREG_BASE + 0x1A, 0x01);
	else
		_riu_write_byte(MBREG_BASE + 0x1A, 0x00);
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

	#if VIF_FW_RUN_ON_DRAM_MODE
	u8 *virt_dram_base_addr;
	u16 address_offset;
	#endif

	pr_info("%s()\n", __func__);

	#if VIF_FW_RUN_ON_DRAM_MODE
	virt_dram_base_addr = _pvif_in->virt_dram_base_addr;
	address_offset = (intern_vif_table[0x400] << 8)|intern_vif_table[0x401];

	// must follow backend register order
	tmp_data = (u8)_pvif_in->vif_top; //+0
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_if_base_freq;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 1), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_tuner_step_size;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 2), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_saw_arch;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 3), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_vga_max;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 4), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_vga_max >> 8); //+5
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 5), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_vga_min;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 6), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_vga_min >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 7), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->gain_distribute_thr;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 8), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->gain_distribute_thr >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 9), &tmp_data, 1);

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
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 10), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_agc_vga_offs;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 11), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_agc_ref_negative;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 12), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_agc_ref_positive;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 13), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_dagc1_ref;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 14), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_dagc2_ref; //+15
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 15), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_dagc1_gain_ov;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 16), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_dagc1_gain_ov >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 17), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_dagc2_gain_ov;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 18), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_dagc2_gain_ov >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 19), &tmp_data, 1);
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
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 20), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_kp1;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 21), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_ki1;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 22), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_kp2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 23), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_ki2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 24), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_kp; //+25
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 25), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_ki;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 26), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_lock_thr;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 27), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_lock_thr >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 28), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_thr;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 29), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_thr >> 8); //+30
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 30), &tmp_data, 1);
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
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 31), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_lock_num >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 32), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_lock_num >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 33), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_lock_num >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 34), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_unlock_num;//+35
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 35), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_unlock_num >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 36), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_unlock_num >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 37), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_unlock_num >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 38), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_pd_err_max;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 39), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_pd_err_max >> 8);//+40
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 40), &tmp_data, 1);

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
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 41), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_pd_x2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 42), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_lpf_sel;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 43), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_pd_mode_sel;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 44), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust;//+45
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 45), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust_gear;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 46), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust_thr1;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 47), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust_thr2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 48), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_kpki_adjust_thr3;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 49), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_dynamic_top_adjust;//+50
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 50), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_dynamic_top_min;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 51), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_am_hum_detection;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 52), &tmp_data, 1);

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
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 53), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_syncbott_ref;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 54), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_syncheight_ref;//+55
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 55), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_kc;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 56), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_kg;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 57), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_clamp_oren;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 58), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_gain_oren;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 59), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_clamp_ov_negative;//+60
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 60), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->clampgain_clamp_ov_negative >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 61), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_gain_ov_negative;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 62), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->clampgain_gain_ov_negative >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 63), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_clamp_ov_positive;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 64), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->clampgain_clamp_ov_positive >> 8);//+65
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 65), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_gain_ov_positive;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 66), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->clampgain_gain_ov_positive >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 67), &tmp_data, 1);

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
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 68), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_clamp_max;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 69), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_gain_min;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 70), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_gain_max;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 71), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->clampgain_porch_cnt;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 72), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->clampgain_porch_cnt >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 73), &tmp_data, 1);

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
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 74), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_delay_reduce;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 75), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_overmodulation;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 76), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_overmodulation_detect;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 77), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_aci_detect;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 78), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_serious_aci_detect;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 79), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_aci_agc_ref;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 80), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_adc_overflow_agc_ref;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 81), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_channel_scan_agc_ref;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 82), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_aci_det_thr1;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 83), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_aci_det_thr2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 84), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_aci_det_thr3;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 85), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_aci_det_thr4;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 86), &tmp_data, 1);

	/* vif_rf_band */
	tmp_data = (u8)_pvif_in->vif_rf_band;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 87), &tmp_data, 1);

	/*BYTE vif_tuner_type;*/
	tmp_data = (u8)_pvif_in->vif_tuner_type;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 88), &tmp_data, 1);

	/*CrRate(4)+CrInv(1) for (B,GH,DK,I,L,LL,MN)*/
	tmp_data = (u8)_pvif_in->vif_cr_rate_b;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 89), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_b >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 90), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_b >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 91), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_b >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 92), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_inv_b;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 93), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_rate_gh;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 94), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_gh >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 95), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_gh >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 96), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_gh >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 97), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_inv_gh;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 98), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_rate_dk;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 99), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_dk >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 100), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_dk >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 101), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_dk >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 102), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_inv_dk;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 103), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_rate_i;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 104), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_i >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 105), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_i >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 106), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_i >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 107), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_inv_i;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 108), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_rate_l;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 109), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_l >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 110), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_l >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 111), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_l >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 112), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_inv_l;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 113), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_rate_ll;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 114), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_ll >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 115), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_ll >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 116), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_ll >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 117), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_inv_ll;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 118), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_rate_mn;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 119), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_mn >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 120), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_mn >> 16);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 121), &tmp_data, 1);

	tmp_data = (u8)(_pvif_in->vif_cr_rate_mn >> 24);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 122), &tmp_data, 1);

	tmp_data = (u8)_pvif_in->vif_cr_inv_mn;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 123), &tmp_data, 1);

	/* vif_reserve */
	tmp_data = (u8)_pvif_in->vif_reserve;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 124), &tmp_data, 1);


	/* N_a1 C0 to C2, N_a2 C0 to C2 */
	/* 228th to 239th byte */
	tmp_data = (u8)_pn_a1a2->vif_n_a1_c0;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 125), &tmp_data, 1);

	tmp_data = (u8)(_pn_a1a2->vif_n_a1_c0 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 126), &tmp_data, 1);

	tmp_data = (u8)_pn_a1a2->vif_n_a1_c1;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 127), &tmp_data, 1);

	tmp_data = (u8)(_pn_a1a2->vif_n_a1_c1 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 128), &tmp_data, 1);

	tmp_data = (u8)_pn_a1a2->vif_n_a1_c2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 129), &tmp_data, 1);

	tmp_data = (u8)(_pn_a1a2->vif_n_a1_c2 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 130), &tmp_data, 1);

	tmp_data = (u8)_pn_a1a2->vif_n_a2_c0;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 131), &tmp_data, 1);

	tmp_data = (u8)(_pn_a1a2->vif_n_a2_c0 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 132), &tmp_data, 1);

	tmp_data = (u8)_pn_a1a2->vif_n_a2_c1;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 133), &tmp_data, 1);

	tmp_data = (u8)(_pn_a1a2->vif_n_a2_c1 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 134), &tmp_data, 1);

	tmp_data = (u8)_pn_a1a2->vif_n_a2_c2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 135), &tmp_data, 1);

	tmp_data = (u8)(_pn_a1a2->vif_n_a2_c2 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 136), &tmp_data, 1);


	/* Sos11 C0 to C4, Sos12 C0 to C4 */
	/* 240th to 259th byte */
	tmp_data = (u8)_psos_1112->vif_sos_11_c0;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 137), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_11_c0 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 138), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_11_c1;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 139), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_11_c1 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 140), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_11_c2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 141), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_11_c2 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 142), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_11_c3;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 143), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_11_c3 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 144), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_11_c4;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 145), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_11_c4 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 146), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_12_c0;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 147), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_12_c0 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 148), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_12_c1;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 149), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_12_c1 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 150), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_12_c2;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 151), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_12_c2 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 152), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_12_c3;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 153), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_12_c3 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 154), &tmp_data, 1);

	tmp_data = (u8)_psos_1112->vif_sos_12_c4;
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 155), &tmp_data, 1);

	tmp_data = (u8)(_psos_1112->vif_sos_12_c4 >> 8);
	//_riu_write_byte(DMDMCU_BASE+0x0C, tmp_data);
	memcpy((virt_dram_base_addr + address_offset + 156), &tmp_data, 1);
	#else
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
	#endif
}

bool hal_vif_filter_coef_download(void)
{
	u8 data = 0x00;
	u16 address_offset;
	u16 i = 0;
	u16 u16tmp = 0;
	u32 fw_start_time;

	#if VIF_FW_RUN_ON_DRAM_MODE
	u8 u8Tmp = 0;
	u8 *virt_dram_base_addr;
	u16 j = 0;
	#endif

	pr_info("%s()\n", __func__);

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

		#if !VIF_FW_RUN_ON_DRAM_MODE
		// release MCU, madison patch
		_riu_write_byte(DMDMCU_BASE+0x00, 0x00);
		#endif

		_riu_write_byte(DMDMCU_BASE+0x03, 0x50); // enable "vdmcu51_if"
		_riu_write_byte(DMDMCU_BASE+0x03, 0x51); // enable auto-increase

		#if !VIF_FW_RUN_ON_DRAM_MODE
		// sram address low byte & high byte
		_riu_write_byte(DMDMCU_BASE+0x04, 0x00);
		_riu_write_byte(DMDMCU_BASE+0x05, 0x00);
		#endif

		//Set Filter coefficient
		address_offset = (intern_vif_table[0x40C] << 8)
				|intern_vif_table[0x40D];
		address_offset = ((intern_vif_table[address_offset] << 8)
				|intern_vif_table[address_offset+1]);

		#if !VIF_FW_RUN_ON_DRAM_MODE
		_riu_write_byte(DMDMCU_BASE+0x04, (address_offset&0xFF));
		_riu_write_byte(DMDMCU_BASE+0x05, (address_offset>>8));
		#endif

		#if VIF_FW_RUN_ON_DRAM_MODE
		virt_dram_base_addr = _pvif_in->virt_dram_base_addr;
		#endif

		for (i = 0; i < EQ_COEF_LEN; i++) {
			u16tmp = _pvif_filters[i];

			#if !VIF_FW_RUN_ON_DRAM_MODE
			// write data to VD MCU 51 code sram
			_riu_write_byte(DMDMCU_BASE+0x0C, (u16tmp >> 8));
			_riu_write_byte(DMDMCU_BASE+0x0C, (u16tmp & 0xFF));
			#else
			u8Tmp = u16tmp >> 8;
			memcpy((virt_dram_base_addr + address_offset + j), &u8Tmp, 1);
			u8Tmp = u16tmp & 0xFF;
			memcpy((virt_dram_base_addr + address_offset + j + 1), &u8Tmp, 1);
			j += 2;
			#endif
		}

		_riu_write_byte(DMDMCU_BASE+0x03, 0x50); //disable auto-increase
		_riu_write_byte(DMDMCU_BASE+0x03, 0x00); // disable "vdmcu51_if"

		// reset MCU, madison patch
		_riu_write_byte(DMDMCU_BASE+0x00, 0x01);

		#if !VIF_FW_RUN_ON_DRAM_MODE
		_riu_write_byte(DMDMCU_BASE+0x01, 0x01); // enable SRAM
		#endif

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

	#if VIF_FW_RUN_ON_DRAM_MODE
	u8 *virt_dram_base_addr;
	#endif

	u8 data1 = 0;
	u8 data2 = 0;
	u8 data3 = 0;

	pr_info("%s is called\n", __func__);

	//if exit not called...
	if (g_vif_downloaded == TRUE) {
		#if !VIF_FW_RUN_ON_DRAM_MODE
		_riu_write_byte(DMDMCU_BASE+0x01, 0x01); //enable DMD MCU51 SRAM
		#endif

		_riu_write_byte_mask(DMDMCU_BASE+0x00, 0x00, 0x01);

		hal_vif_delayms(1);

		if (_hal_vif_ready() == FALSE) {
			//g_vif_downloaded = FALSE;

			g_vif_prev_scan_flag = FALSE;

			#if VIF_FW_RUN_ON_DRAM_MODE
			// To avoid DTV FW wake up
			_riu_write_byte_mask(DMDMCU_BASE+0x00, 0x01, 0x01); // reset VD_MCU
			#endif

			//hal_vif_adc_init();

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

		#if !VIF_FW_RUN_ON_DRAM_MODE
		// release MCU, madison patch
		_riu_write_byte(DMDMCU_BASE+0x00, 0x00);
		#endif

		_riu_write_byte(DMDMCU_BASE+0x03, 0x50); // enable "vdmcu51_if"
		_riu_write_byte(DMDMCU_BASE+0x03, 0x51); // enable auto-increase

		#if !VIF_FW_RUN_ON_DRAM_MODE
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
		#else
		// Download DRAM part
		virt_dram_base_addr = _pvif_in->virt_dram_base_addr;

		pr_info("\r\n == hal_vif_download_virt_dram_base_addr = 0x%X == \r\n",
			virt_dram_base_addr);

		memcpy(virt_dram_base_addr, &vif_table[0], lib_size);

		_download_backend();
		#endif

		_riu_write_byte(DMDMCU_BASE+0x03, 0x50); //disable auto-increase
		_riu_write_byte(DMDMCU_BASE+0x03, 0x00); // disable "vdmcu51_if"

		// reset MCU, madison patch
		_riu_write_byte(DMDMCU_BASE+0x00, 0x01);

		#if !VIF_FW_RUN_ON_DRAM_MODE
		_riu_write_byte(DMDMCU_BASE+0x01, 0x01); // enable SRAM
		#endif

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

		_mbx_readreg(0x20C4, &data1);
		_mbx_readreg(0x20C5, &data2);
		_mbx_readreg(0x20C6, &data3);

		pr_info("INTERN_VIF_FW_VERSION:%x.%x.%x\n", data1, data2, data3);

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
	//_hal_vif_write_byte(FIRMWARE_VERSION_L, 0x17);    // 23(dd)
	//_hal_vif_write_byte(FIRMWARE_VERSION_H, 0x25);    // 02/21 (mm/yy)

	_hal_vif_write_bit(RF_LOAD, 1, _BIT0);
	_hal_vif_write_bit(DBB1_LOAD, 1, _BIT0);

}

void hal_vif_set_sound_sys(enum vif_sound_sys sound_sys, u32 if_freq)
{
	u8 u8tmp = 0;
	u8 spectrum_inv = 0;
	u16 clamp_gain = 0;
	u32 video_if_khz = 0;
	u32 cr_rate = 0;

	pr_info("%s() sound_sys=%d\n", __func__, sound_sys);
	pr_info("%s() if_freq=%d\n", __func__, if_freq);

	if (!_hal_vif.base_addr_init_done)
		return;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return;
	}

	if (sound_sys == VIF_SOUND_LL)
		spectrum_inv = _pvif_in->spectrum_inv;
	else {
		if (_pvif_in->spectrum_inv != 0)
			spectrum_inv = 0;
		else
			spectrum_inv = 1;
	}

	/* TODO: get IF freq from tuner, and overwrite default setting */
	if (if_freq != 0) {
		switch (sound_sys) {
		case VIF_SOUND_B:
		case VIF_SOUND_B_NICAM:
			if (spectrum_inv == 0)
				video_if_khz = if_freq + 2250;
			else
				video_if_khz = if_freq - 2250;

			pr_info("%s() VIF_SOUND_B / video_if_khz = %d\n", __func__, video_if_khz);

			//clamp_gain = 0x05A0;
			clamp_gain = 0x02B5;

			pr_info("%s() VIF_SOUND_B / clamp_gain = 0x%X\n", __func__, clamp_gain);

			cr_rate = ((video_if_khz * 97090) / 1000) + 3;

			_pvif_in->vif_cr_rate_b = cr_rate;
			_pvif_in->vif_cr_inv_b = spectrum_inv;
			break;
		case VIF_SOUND_GH:
		case VIF_SOUND_GH_NICAM:
			if (spectrum_inv == 0)
				video_if_khz = if_freq + 2750;
			else
				video_if_khz = if_freq - 2750;

			pr_info("%s() VIF_SOUND_GH / video_if_khz = %d\n", __func__, video_if_khz);

			//clamp_gain = 0x05A0;
			clamp_gain = 0x02B5;

			pr_info("%s() VIF_SOUND_GH / clamp_gain = 0x%X\n", __func__, clamp_gain);

			cr_rate = ((video_if_khz * 97090) / 1000) + 3;

			_pvif_in->vif_cr_rate_gh = cr_rate;
			_pvif_in->vif_cr_inv_gh = spectrum_inv;
			break;
		case VIF_SOUND_DK1:
		case VIF_SOUND_DK2:
		case VIF_SOUND_DK3:
		case VIF_SOUND_DK_NICAM:
			if (spectrum_inv == 0)
				video_if_khz = if_freq + 2750;
			else
				video_if_khz = if_freq - 2750;

			pr_info("%s() VIF_SOUND_DK2 / video_if_khz = %d\n", __func__, video_if_khz);

			//clamp_gain = 0x05CE;
			clamp_gain = 0x02C9;

			pr_info("%s() VIF_SOUND_DK2 / clamp_gain = 0x%X\n", __func__, clamp_gain);

			cr_rate = ((video_if_khz * 97090) / 1000) + 3;

			_pvif_in->vif_cr_rate_dk = cr_rate;
			_pvif_in->vif_cr_inv_dk = spectrum_inv;
			break;
		case VIF_SOUND_I:
			if (spectrum_inv == 0)
				video_if_khz = if_freq + 2750;
			else
				video_if_khz = if_freq - 2750;

			pr_info("%s() VIF_SOUND_I / video_if_khz = %d\n", __func__, video_if_khz);

			//clamp_gain = 0x065E;
			clamp_gain = 0x0310;

			pr_info("%s() VIF_SOUND_I / clamp_gain = 0x%X\n", __func__, clamp_gain);

			cr_rate = ((video_if_khz * 97090) / 1000) + 3;

			_pvif_in->vif_cr_rate_i = cr_rate;
			_pvif_in->vif_cr_inv_i = spectrum_inv;
			break;
		case VIF_SOUND_MN:
			if (spectrum_inv == 0)
				video_if_khz = if_freq + 1750;
			else
				video_if_khz = if_freq - 1750;

			pr_info("%s() VIF_SOUND_MN / video_if_khz = %d\n", __func__, video_if_khz);

			//clamp_gain = 0x05AE;
			clamp_gain = 0x02B0;

			pr_info("%s() VIF_SOUND_MN / clamp_gain = 0x%X\n", __func__, clamp_gain);

			cr_rate = ((video_if_khz * 97090) / 1000) + 3;

			_pvif_in->vif_cr_rate_mn = cr_rate;
			_pvif_in->vif_cr_inv_mn = spectrum_inv;
			break;
		case VIF_SOUND_L:
			if (spectrum_inv == 0)
				video_if_khz = if_freq + 2750;
			else
				video_if_khz = if_freq - 2750;

			pr_info("%s() VIF_SOUND_L / video_if_khz = %d\n", __func__, video_if_khz);

			clamp_gain = 0x0772;

			pr_info("%s() VIF_SOUND_L / clamp_gain = 0x%X\n", __func__, clamp_gain);

			cr_rate = ((video_if_khz * 97090) / 1000) + 3;

			_pvif_in->vif_cr_rate_l = cr_rate;
			_pvif_in->vif_cr_inv_l = spectrum_inv;
			break;
		case VIF_SOUND_LL:
			if (spectrum_inv == 0)
				video_if_khz = if_freq + 2750;
			else
				video_if_khz = if_freq - 2750;

			pr_info("%s() VIF_SOUND_LL / video_if_khz = %d\n", __func__, video_if_khz);

			clamp_gain = 0x0600;

			pr_info("%s() VIF_SOUND_LL / clamp_gain = 0x%X\n", __func__, clamp_gain);

			cr_rate = ((video_if_khz * 97090) / 1000) + 3;

			_pvif_in->vif_cr_rate_ll = cr_rate;
			_pvif_in->vif_cr_inv_ll = spectrum_inv;
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

	if (_pvif_in->tuner_gain_inv != 0) {
		_hal_vif_write_bit(AGC_GAIN_SLOPE, 1, _BIT4); // AGC_Negative
		pr_info("%s() AGC_Negative\n", __func__);
	} else {
		_hal_vif_write_bit(AGC_GAIN_SLOPE, 0, _BIT4); // AGC_Positive
		pr_info("%s() AGC_Positive\n", __func__);
	}

	// load clamp gain
	_hal_vif_write_byte(CLAMPGAIN_GAIN_OVERWRITE, (clamp_gain & 0xFF));
	_hal_vif_write_byte(CLAMPGAIN_GAIN_OVERWRITE+1, (clamp_gain >> 8));

	// Utopia driver version control
	_hal_vif_write_byte(FIRMWARE_VERSION_L, DRIVER_VER_L);
	_hal_vif_write_byte(FIRMWARE_VERSION_H, DRIVER_VER_H);
}

void hal_vif_top_adjust(void)
{
}

void hal_vif_load(void)
{
	if (!_hal_vif.base_addr_init_done)
		return;

	if (g_vif_downloaded)
		return;
	//else {
		_riu_write_bit(RF_LOAD, 1, _BIT0);
		_riu_write_bit(DBB1_LOAD, 1, _BIT0);
		_riu_write_bit(DBB2_LOAD, 1, _BIT0);
		_riu_write_bit(DBB2_LOAD, 0, _BIT0);

		return;
	//}
}

u8 hal_vif_exit(void)
{
	u8 check_count = 0;

	pr_info("%s is called\n", __func__);

	if (!_hal_vif.base_addr_init_done)
		return FALSE;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return FALSE;
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
			return FALSE;
		}
	}

	pr_info(">> VIF Exit Ok!\n");
	//g_vif_downloaded = FALSE;

	//if (g_vif_downloaded == FALSE) {
		writel(0x00000000, _pvif_in->virt_reset_base_addr);
		hal_vif_delayms(1);
		writel(0x00000011, _pvif_in->virt_reset_base_addr);

		pr_info(">> VIF Exit2 Ok!\n");
	//}

	return TRUE;
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
	if (!_hal_vif.base_addr_init_done)
		return 0;

	if (!g_vif_downloaded) {
		pr_info("VIF NOT DOWNLOADED\n");
		return 0;
	}

	return _riu_read_byte(MBREG_BASE + 0x1D) & 0x01;
}

void hal_vif_update_fw_backend(void)
{
	#if !VIF_FW_RUN_ON_DRAM_MODE
	u16 address_offset;
	#endif
	const u8 *vif_table;
	//u16 lib_size;
	u32 fw_start_time;
	u8 data = 0x00;

	vif_table = &intern_vif_table[0];
	//lib_size = sizeof(intern_vif_table);

	_riu_write_byte(DMDMCU_BASE+0x00, 0x01); // reset VD_MCU
	_riu_write_byte(DMDMCU_BASE+0x01, 0x00); // disable SRAM

	#if !VIF_FW_RUN_ON_DRAM_MODE
	_riu_write_byte(DMDMCU_BASE+0x00, 0x00); // release MCU, madison patch
	#endif

	_riu_write_byte(DMDMCU_BASE+0x03, 0x50); // enable "vdmcu51_if"
	_riu_write_byte(DMDMCU_BASE+0x03, 0x51); // enable auto-increase

	#if !VIF_FW_RUN_ON_DRAM_MODE
	_riu_write_byte(DMDMCU_BASE+0x04, 0x00); // sram address low byte
	_riu_write_byte(DMDMCU_BASE+0x05, 0x00); // sram address high byte

	////  Load code thru VDMCU_IF ////

	address_offset = (vif_table[0x400] << 8)|vif_table[0x401];

	// sram address low byte
	_riu_write_byte(DMDMCU_BASE+0x04, (address_offset&0xFF));
	// sram address high byte
	_riu_write_byte(DMDMCU_BASE+0x05, (address_offset>>8));
	#endif

	_download_backend();

	_riu_write_byte(DMDMCU_BASE+0x03, 0x50); // disable auto-increase
	_riu_write_byte(DMDMCU_BASE+0x03, 0x00); // disable "vdmcu51_if"

	_riu_write_byte(DMDMCU_BASE+0x00, 0x01); // reset MCU, madison patch

	#if !VIF_FW_RUN_ON_DRAM_MODE
	_riu_write_byte(DMDMCU_BASE+0x01, 0x01); // enable SRAM
	#endif

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

