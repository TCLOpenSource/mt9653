// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//----------------------------------------------------------
//  Include Files
//----------------------------------------------------------

#include "halDMD_INTERN_ISDBT.h"

#if !defined MTK_PROJECT
#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/kernel.h>
#include <linux/delay.h>
#else
#include <stdio.h>
#include <math.h>
#endif
#endif
#ifndef DVB_STI
 #define DVB_STI    1
#endif
#include <linux/delay.h>
#include <asm/io.h>
#if DMD_ISDBT_3PARTY_EN
// Please add header files needed by 3 party platform
#endif

//----------------------------------------------------------
//  Driver Compiler Options
//----------------------------------------------------------

#ifndef BIN_RELEASE
#define BIN_RELEASE                 0
#endif

#if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI

#define DMD_ISDBT_CHIP_EULER        0x00
#define DMD_ISDBT_CHIP_NUGGET       0x01
#define DMD_ISDBT_CHIP_KAPPA        0x02
#define DMD_ISDBT_CHIP_EINSTEIN     0x03
#define DMD_ISDBT_CHIP_NAPOLI       0x04
#define DMD_ISDBT_CHIP_MONACO       0x05
#define DMD_ISDBT_CHIP_MIAMI        0x06
#define DMD_ISDBT_CHIP_MUJI         0x07
#define DMD_ISDBT_CHIP_MUNICH       0x08
#define DMD_ISDBT_CHIP_MANHATTAN    0x09
#define DMD_ISDBT_CHIP_MULAN        0x0A
#define DMD_ISDBT_CHIP_MESSI        0x0B
#define DMD_ISDBT_CHIP_MASERATI     0x0C
#define DMD_ISDBT_CHIP_KIWI         0x0D
#define DMD_ISDBT_CHIP_MACAN        0x0E
#define DMD_ISDBT_CHIP_MUSTANG      0x0F
#define DMD_ISDBT_CHIP_MAXIM        0x10
#define DMD_ISDBT_CHIP_MARLON       0x11
#define DMD_ISDBT_CHIP_KENTUCKY     0x12
#define DMD_ISDBT_CHIP_MAINZ        0x13
#define DMD_ISDBT_CHIP_M5621        0x14
#define DMD_ISDBT_CHIP_M7621        0x15
#define DMD_ISDBT_CHIP_K5AP         0x16
#define DMD_ISDBT_CHIP_M5321        0x17
#define DMD_ISDBT_CHIP_M7622        0x18
#define DMD_ISDBT_CHIP_M7322        0x19
#define DMD_ISDBT_CHIP_M3822        0x20
#define DMD_ISDBT_CHIP_MT5895       0x21
#define DMD_ISDBT_CHIP_M7332        0x22
#define DMD_ISDBT_CHIP_M7632        0x23
#define DMD_ISDBT_CHIP_M7642        0x24
#define DMD_ISDBT_CHIP_MT5867       0x25


#if defined(CHIP_M7322)
 #define DMD_ISDBT_CHIP_VERSION     DMD_ISDBT_CHIP_M7322
#elif defined(CHIP_M3822)
 #define DMD_ISDBT_CHIP_VERSION     DMD_ISDBT_CHIP_M3822
#elif defined(CC_MT5895)
 #define DMD_ISDBT_CHIP_VERSION     DMD_ISDBT_CHIP_MT5895
#elif defined(CHIP_M7332)
 #define DMD_ISDBT_CHIP_VERSION     DMD_ISDBT_CHIP_M7332
#elif defined(CHIP_M7632)
 #define DMD_ISDBT_CHIP_VERSION     DMD_ISDBT_CHIP_M7632
#elif defined(CHIP_M7642)
 #define DMD_ISDBT_CHIP_VERSION     DMD_ISDBT_CHIP_M7642
#else
 #define DMD_ISDBT_CHIP_VERSION     DMD_ISDBT_CHIP_M7332
#endif

#endif // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

//------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------

#if defined MTK_PROJECT
#define _riu_read_byte(addr) \
	((addr)&0x01 ? \
	((u8)((IO_READ32((addr&0x00FFFF00), (addr&0x000000FE)<<1))>>8)) : \
	((u8)((IO_READ32((addr&0x00FFFF00), (addr&0x000000FE)<<1)) & \
	0x000000FF)))
#define _riu_write_byte(addr, val)  do { \
if ((addr)&0x01) \
	IO_WRITE32((addr&0x00FFFF00), (addr&0x000000FE)<<1, \
	(IO_READ32((addr&0x00FFFF00), (addr&0x000000FE)<<1) & 0xFFFF00FF) | \
	((((u32)val)<<8))); \
else \
	IO_WRITE32((addr&0x00FFFF00), (addr&0x000000FE)<<1, \
	(IO_READ32((addr&0x00FFFF00), (addr&0x000000FE)<<1)&0xFFFFFF00) | \
	((u32)val)); \
} while (0)
#elif DVB_STI
#define _riu_read_byte(addr) \
(readb(pres->sdmd_isdbt_pridata.virt_dmd_base_addr + (addr)))
#define _riu_write_byte(addr, val) \
(writeb(val, pres->sdmd_isdbt_pridata.virt_dmd_base_addr + (addr)))

#elif DMD_ISDBT_3PARTY_EN
// Please add riu read&write define according to 3 party platform
#define _riu_read_byte(addr)
#define _riu_write_byte(addr, val)
#else
#define _riu_read_byte(addr) \
(READ_BYTE(pres->sdmd_isdbt_pridata.virt_dmd_base_addr + (addr)))
#define _riu_write_byte(addr, val) \
(WRITE_BYTE(pres->sdmd_isdbt_pridata.virt_dmd_base_addr + (addr), val))
#endif

#define USE_DEFAULT_CHECK_TIME       1

#define HAL_INTERN_ISDBT_DBINFO(y)   //y

#ifdef MTK_PROJECT
#ifndef DEMOD_BASE
#define DEMOD_BASE               (IO_VIRT + 0x500000)
#endif

#ifndef ANA1_BASE
#define ANA1_BASE                (IO_VIRT + 0x33000)
#endif

#ifndef CKGEN_BASE
#define CKGEN_BASE               (IO_VIRT + 0x0d000)
#endif

#ifndef SMI_LARB15_BASE
#define SMI_LARB15_BASE          (IO_VIRT + 0xc8000)
#endif
#endif

#ifndef MBRegBase
#ifdef MTK_PROJECT
 #define MBRegBase              0x10505E00//DEMOD_BASE+0x5E00
#else
 #define MBRegBase              0x112600
#endif
#endif
#ifndef DMDMcuBase
#ifdef MTK_PROJECT
 #define DMDMcuBase             0x10527D00//DEMOD_BASE+0x27D00
#else
 #define DMDMcuBase             0x103480
#endif
#endif

#if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI
#define DEFAULT_ICFO_CHECK_TIME    500
#define DEFAULT_FEC_CHECK_TIME     1500

#define REG_ISDBT_LOCK_STATUS   0x11F5
#define REG_ISDBT_LOCK_STATUS2  0x20C3
#define ISDBT_TDP_REG_BASE      0x1400
#define ISDBT_FDP_REG_BASE      0x1500
#define ISDBT_FDPEXT_REG_BASE   0x1600
#define ISDBT_OUTER_REG_BASE    0x1700

#if ((DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_EULER) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_NUGGET) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_MUNICH))
#define ISDBT_MIU_CLIENTW_ADDR      0xF5
#define ISDBT_MIU_CLIENTR_ADDR      0xF5
#define ISDBT_MIU_CLIENTW_MASK      0x87
#define ISDBT_MIU_CLIENTR_MASK      0x87
#define ISDBT_MIU_CLIENTW_BIT_MASK  0x01
#define ISDBT_MIU_CLIENTR_BIT_MASK  0x02
#elif ((DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_EINSTEIN) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_NAPOLI) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_MONACO) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_MIAMI) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_MUJI) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_MANHATTAN) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_MESSI))
#define ISDBT_MIU_CLIENTW_ADDR      0xF2
#define ISDBT_MIU_CLIENTR_ADDR      0xF2
#define ISDBT_MIU_CLIENTW_MASK      0x66
#define ISDBT_MIU_CLIENTR_MASK      0x66
#define ISDBT_MIU_CLIENTW_BIT_MASK  0x02
#define ISDBT_MIU_CLIENTR_BIT_MASK  0x04
#elif (DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_KAPPA)
#define ISDBT_MIU_CLIENTW_ADDR      0xF1
#define ISDBT_MIU_CLIENTR_ADDR      0xF0
#define ISDBT_MIU_CLIENTW_MASK      0x47
#define ISDBT_MIU_CLIENTR_MASK      0x46
#define ISDBT_MIU_CLIENTW_BIT_MASK  0x02
#define ISDBT_MIU_CLIENTR_BIT_MASK  0x20
#elif (DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_KIWI)
#define ISDBT_MIU_CLIENTW_ADDR      0xF1
#define ISDBT_MIU_CLIENTR_ADDR      0xF0
#define ISDBT_MIU_CLIENTW_MASK      0x47
#define ISDBT_MIU_CLIENTR_MASK      0x46
#define ISDBT_MIU_CLIENTW_BIT_MASK  0x04
#define ISDBT_MIU_CLIENTR_BIT_MASK  0x20
#endif

#else // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

// Please set the following values according to 3 party platform
#define DEFAULT_ICFO_CHECK_TIME
#define DEFAULT_FEC_CHECK_TIME

#define REG_ISDBT_LOCK_STATUS
#define ISDBT_TDP_REG_BASE
#define ISDBT_FDP_REG_BASE
#define ISDBT_FDPEXT_REG_BASE
#define ISDBT_OUTER_REG_BASE

#define ISDBT_MIU_CLIENTW_ADDR
#define ISDBT_MIU_CLIENTR_ADDR
#define ISDBT_MIU_CLIENTW_MASK
#define ISDBT_MIU_CLIENTR_MASK
#define ISDBT_MIU_CLIENTW_BIT_MASK
#define ISDBT_MIU_CLIENTR_BIT_MASK

#endif // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

#if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI

#if (DMD_ISDBT_CHIP_VERSION >= DMD_ISDBT_CHIP_MULAN)
 #define DMD_ISDBT_TBVA_EN       1
#else
 #define DMD_ISDBT_TBVA_EN       0
#endif

#if ((DMD_ISDBT_CHIP_VERSION < DMD_ISDBT_CHIP_MULAN) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_MESSI) || \
(DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_KIWI))
 #define TDI_MIU_FREE_BY_DRV     1
#else
 #define TDI_MIU_FREE_BY_DRV     0
#endif

#if DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_MUSTANG
 #define REMAP_RST_BY_DRV        1
#else
 #define REMAP_RST_BY_DRV        0
#endif

#if ((DMD_ISDBT_CHIP_VERSION >= DMD_ISDBT_CHIP_M5621) && \
(DMD_ISDBT_CHIP_VERSION != DMD_ISDBT_CHIP_M7621))
 #define POW_SAVE_BY_DRV         1
#else
 #define POW_SAVE_BY_DRV         0
#endif

#if ((DMD_ISDBT_CHIP_VERSION >= DMD_ISDBT_CHIP_MACAN) && \
(DMD_ISDBT_CHIP_VERSION != DMD_ISDBT_CHIP_MAINZ))
 #define TDI_USE_DRAM_SIZE       0x500000
#else
 #define TDI_USE_DRAM_SIZE       0x700000
#endif

#if DMD_ISDBT_CHIP_VERSION >= DMD_ISDBT_CHIP_M7622
 #define USE_NEW_MODECP_DET      1
#else
 #define USE_NEW_MODECP_DET      0
#endif

#if (DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_K5AP) || (defined MTK_PROJECT)
 // 0: FW download to SRAM, 1: FW download to DRAM
 #define FW_RUN_ON_DRAM_MODE     1
#else
 #define FW_RUN_ON_DRAM_MODE     0
#endif

#if DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_M3822
 #define PACKAGE_INFORMATION     1 // NEW/OLD = QFP/BGA
#else
 #define PACKAGE_INFORMATION     0
#endif

#if DMD_ISDBT_CHIP_VERSION >= DMD_ISDBT_CHIP_M7332
 #define GLO_RST_IN_DOWNLOAD     0
#else
 #define GLO_RST_IN_DOWNLOAD     0
#endif

#else // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

 // Please set the following values according to 3 party platform
 #define DMD_ISDBT_TBVA_EN       0
 #define TDI_MIU_FREE_BY_DRV     0
 #define REMAP_RST_BY_DRV        0
 #define POW_SAVE_BY_DRV         0
 #define GLO_RST_IN_DOWNLOAD     0

 #define TDI_USE_DRAM_SIZE       0x000000

 #define USE_NEW_MODECP_DET      0

 // 0: FW download to SRAM, 1: FW download to DRAM
 #define FW_RUN_ON_DRAM_MODE     0

#endif  // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

//#if FW_RUN_ON_DRAM_MODE && !defined MTK_PROJECT
//#define SRAM_OFFSET 0x8000

//extern void *memcpy(void *destination, const void *source, size_t num);
//#endif

//------------------------------------------------------------------
//  Local Variables
//------------------------------------------------------------------

static u8 id;

static struct dmd_isdbt_resdata *pres;

static struct dmd_isdbt_sys_ptr sys;

#if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI
const u8 intern_isdbt_table[] = {
	#include "DMD_INTERN_ISDBT.dat"
};
const u8 intern_isdbt_8M_table[] = {
	#include "DMD_INTERN_ISDBT_8M.dat"
};
#else // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE
// Please set fw dat file used by 3 party platform
const u8 intern_isdbt_table[] = {
	#include "DMD_INTERN_ISDBT.dat"
};
const u8 intern_isdbt_8M_table[] = {
	#include "DMD_INTERN_ISDBT_8M.dat"
};
#endif // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

#ifndef UTPA2
static const float _log_approx_table_x[80] = {
	1.00, 1.30, 1.69, 2.20, 2.86, 3.71, 4.83, 6.27, 8.16, 10.60,
	13.79, 17.92, 23.30, 30.29, 39.37, 51.19, 66.54,
	86.50, 112.46, 146.19,
	190.05, 247.06, 321.18, 417.54, 542.80, 705.64,
	917.33, 1192.53, 1550.29, 2015.38,
	2620.00, 3405.99, 4427.79, 5756.13, 7482.97,
	9727.86, 12646.22, 16440.08, 21372.11, 27783.74,
	36118.86, 46954.52, 61040.88, 79353.15, 103159.09,
	134106.82, 174338.86, 226640.52, 294632.68, 383022.48,
	497929.22, 647307.99, 841500.39, 1093950.50, 1422135.65,
	1848776.35, 2403409.25, 3124432.03, 4061761.64, 5280290.13,
	6864377.17, 8923690.32, 11600797.42, 15081036.65, 19605347.64,
	25486951.94, 33133037.52, 43072948.77, 55994833.40, 72793283.42,
	94631268.45, 123020648.99, 159926843.68, 207904896.79, 270276365.82,
	351359275.57, 456767058.24, 593797175.72, 771936328.43, 1003517226.96
};

static const float _log_approx_table_y[80] = {
	0.00, 0.11, 0.23, 0.34, 0.46, 0.57, 0.68, 0.80, 0.91, 1.03,
	1.14, 1.25, 1.37, 1.48, 1.60, 1.71, 1.82, 1.94, 2.05, 2.16,
	2.28, 2.39, 2.51, 2.62, 2.73, 2.85, 2.96, 3.08, 3.19, 3.30,
	3.42, 3.53, 3.65, 3.76, 3.87, 3.99, 4.10, 4.22, 4.33, 4.44,
	4.56, 4.67, 4.79, 4.90, 5.01, 5.13, 5.24, 5.36, 5.47, 5.58,
	5.70, 5.81, 5.93, 6.04, 6.15, 6.27, 6.38, 6.49, 6.60, 6.72,
	6.83, 6.95, 7.06, 7.17, 7.29, 7.40, 7.52, 7.63, 7.74, 7.86,
	7.97, 8.08, 8.20, 8.31, 8.43, 8.54, 8.65, 8.77, 8.88, 9.00
};
#else
static const u32 _log_approx_table_x[58] = {
	1, 2, 3, 4, 6, 8, 10, 13, 17, 23, //10
	30, 39, 51, 66, 86, 112, 146, 190, 247, 321, //10
	417, 542, 705, 917, 1192, 1550, 2015, 2620, 3405, 4427, //10
	5756, 7482, 9727, 12646, 16440, 21372, 27783, 36118, 46954, 61040, //10
	79353, 103159, 134106, 174338, 226640, 294632,
	383022, 497929, 647307, 841500, //10
	1093950, 1422135, 1848776, 2403409, 3124432,
	4061761, 5280290, 6160384 //8
};

static const u16 _log_approx_table_y[58] = {
	 0,  30,  47,  60,  77,  90, 100, 111, 123, 136,
	147, 159, 170, 181, 193, 204, 216, 227, 239, 250,
	262, 273, 284, 296, 307, 319, 330, 341, 353, 364,
	376, 387, 398, 410, 421, 432, 444, 455, 467, 478,
	489, 501, 512, 524, 535, 546, 558, 569, 581, 592,
	603, 615, 626, 638, 649, 660, 672, 678
};
#endif

//--------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------
static u8 _riu_read_byte1(u32 addr)
{
	//printk(KERN_INFO
	//"_RIU_READ_BYTE1:pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr=0x%lx\n",
	//pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr);
	return readb(pres->sdmd_isdbt_pridata.virt_dmd_base_addr + (addr));

}

static void _riu_write_byte1(u32 addr, u8 val)
{
	//printk(KERN_INFO
	//"_RIU_WRITE_BYTE1:pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr=0x%lx\n",
	//pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr);
	writeb(pres->sdmd_isdbt_pridata.virt_dmd_base_addr + (addr), val);
}

//------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------
#ifndef UTPA2

#ifndef MSOS_TYPE_LINUX
static float log10_approx(float flt_x)
{
	u8 indx = 0;

	do {
		if (flt_x < _log_approx_table_x[indx])
			break;
		indx++;
	} while (indx < 79);   //stop at indx = 80

	return _log_approx_table_y[indx];
}
#endif

static u16 _calculate_sqi(float fber)
{
	float flog_ber;
	u16 sqi;

	#ifdef MSOS_TYPE_LINUX
	flog_ber = (float)log10((double)fber);
	#else
	if (fber != 0.0)
		flog_ber = (float)(-1.0*log10_approx((double)(1.0 / fber)));
	else
		flog_ber = -8.0;//when fber=0 means u16SQI=100
	#endif

	//PRINT("dan fber = %f\n", fber);
	//PRINT("dan flog_ber = %f\n", flog_ber);
	// Part 2: transfer ber value to u16SQI value.
	if (flog_ber <= (-7.0))
		sqi = 100;    //quality = 100;
	else if (flog_ber < -6.0)
		sqi = (90+(((-6.0) - flog_ber) / ((-6.0) - (-7.0))*(100-90)));
	else if (flog_ber < -5.5)
		sqi = (80+(((-5.5) - flog_ber) / ((-5.5) - (-6.0))*(90-80)));
	else if (flog_ber < -5.0)
		sqi = (70+(((-5.0) - flog_ber) / ((-5.0) - (-5.5))*(80-70)));
	else if (flog_ber < -4.5)
		sqi = (60+(((-4.5) - flog_ber) / ((-4.5) - (-5.0))*(70-50)));
	else if (flog_ber < -4.0)
		sqi = (50+(((-4.0) - flog_ber) / ((-4.0) - (-4.5))*(60-50)));
	else if (flog_ber < -3.5)
		sqi = (40+(((-3.5) - flog_ber) / ((-3.5) - (-4.0))*(50-40)));
	else if (flog_ber < -3.0)
		sqi = (30+(((-3.0) - flog_ber) / ((-3.0) - (-3.5))*(40-30)));
	else if (flog_ber < -2.5)
		sqi = (20+(((-2.5) - flog_ber) / ((-2.5) - (-3.0))*(30-20)));
	else if (flog_ber < -2.0)
		sqi = (0+(((-2.0) - flog_ber) / ((-2.0) - (-2.5))*(20-0)));
	else
		sqi = 0;


	return sqi;
}

#else // #ifndef UTPA2

static u16 log10_approx(u32 flt_x)
{
	u8 indx = 0;

	do {
		if (flt_x < _log_approx_table_x[indx])
			break;
		indx++;
	} while (indx < 57);   //stop at indx = 58

	return _log_approx_table_y[indx];
}

static u16 _calculate_sqi(u32 bervalue, u32 berperiod)
{
	u16 sqi = 0;

	// BerPeriod = 188*8*128*32 = 6160384
	// Log10Approx(6160384) = 678

	if (bervalue == 0)
		sqi = 100;    //quality = 100;
	else if (bervalue * 1000000 < berperiod)
		sqi = ((9000 + (-600 + 678 - log10_approx(bervalue))*10)/100);
	else if (bervalue * 316228 < berperiod)
		sqi = ((8000 + (-550 + 678 - log10_approx(bervalue))*20)/100);
	else if (bervalue * 100000 < berperiod)
		sqi = ((7000 + (-500 + 678 - log10_approx(bervalue))*20)/100);
	else if (bervalue * 31623 < berperiod)
		sqi = ((6000 + (-450 + 678 - log10_approx(bervalue))*20)/100);
	else if (bervalue * 10000 < berperiod)
		sqi = ((5000 + (-400 + 678 - log10_approx(bervalue))*20)/100);
	else if (bervalue * 3162 < berperiod)
		sqi = ((4000 + (-350 + 678 - log10_approx(bervalue))*20)/100);
	else if (bervalue * 1000 < berperiod)
		sqi = ((3000 + (-300 + 678 - log10_approx(bervalue))*20)/100);
	else if (bervalue * 316 < berperiod)
		sqi = ((2000 + (-250 + 678 - log10_approx(bervalue)) * 20)/100);
	else if (bervalue * 100 < berperiod)
		sqi = ((0 + (-200 + 678 - log10_approx(bervalue)) * 40)/100);
	else
		sqi = 0;


	return sqi;
}

#endif // #ifndef UTPA2

static u8 _hal_dmd_riu_readbyte(u32 addr)
{
	#if (!DVB_STI) && (!DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN)
	return  _riu_read_byte(addr);
	#elif DVB_STI
	//printk(KERN_INFO
	//"rrr:pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr=0x%lx\n",
	//pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr);
	return _riu_read_byte(((addr) << 1) - ((addr) & 1));
	#else
	return _riu_read_byte(((addr) << 1) - ((addr) & 1));
	#endif
}

static void _hal_dmd_riu_writebyte(u32 addr, u8 val)
{
	#if (!DVB_STI) && (!DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN)
	_riu_write_byte(addr, val);
	#elif DVB_STI
	//printk(KERN_INFO
	//"www:pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr=0x%lx\n",
	//pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr);
	_riu_write_byte(((addr) << 1) - ((addr) & 1), val);
	#else
	_riu_write_byte(((addr) << 1) - ((addr) & 1), val);
	#endif
}

static void _hal_dmd_riu_writebytemask(u32 addr, u8 val,
u8 mask)
{
	#if (!DVB_STI) && (!DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN)
	_riu_write_byte(addr, (_riu_read_byte(addr) & ~(mask)) |
	((val) & (mask)));
	#else
	//printk(KERN_INFO
	//"WriteByteMask:pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr=0x%lx\n",
	//pRes->sDMD_ISDBT_PriData.virtDMDBaseAddr);
	_riu_write_byte((((addr) << 1) - ((addr) & 1)),
	(_riu_read_byte((((addr) << 1) - ((addr) & 1))) &
	~(mask)) | ((val) & (mask)));
	#endif
}

static u8 _mbx_writereg(u16 addr, u8 data)
{
	u8 CheckCount;
	u8 CheckFlag = 0xFF;

	_hal_dmd_riu_writebyte(MBRegBase + 0x00, (addr & 0xff));
	_hal_dmd_riu_writebyte(MBRegBase + 0x01, (addr >> 8));
	_hal_dmd_riu_writebyte(MBRegBase + 0x10, data);
	_hal_dmd_riu_writebyte(MBRegBase + 0x1E, 0x01);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,
	_hal_dmd_riu_readbyte(DMDMcuBase + 0x03) | 0x02);
	 // assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,
	_hal_dmd_riu_readbyte(DMDMcuBase + 0x03) & (~0x02));
	// de-assert interrupt to VD MCU51

	for (CheckCount = 0; CheckCount < 100; CheckCount++) {
		CheckFlag = _hal_dmd_riu_readbyte(MBRegBase + 0x1E);
		if ((CheckFlag & 0x01) == 0)
			break;

		sys.delayms(1);
	}

	if (CheckFlag & 0x01) {
		PRINT("ERROR: ISDBT INTERN DEMOD MBX WRITE TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

static u8 _mbx_readreg(u16 addr, u8 *pdata)
{
	u8 CheckCount;
	u8 CheckFlag = 0xFF;

	_hal_dmd_riu_writebyte(MBRegBase + 0x00, (addr & 0xff));
	_hal_dmd_riu_writebyte(MBRegBase + 0x01, (addr >> 8));
	_hal_dmd_riu_writebyte(MBRegBase + 0x1E, 0x02);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,
	_hal_dmd_riu_readbyte(DMDMcuBase + 0x03) | 0x02);
	// assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,
	_hal_dmd_riu_readbyte(DMDMcuBase + 0x03) & (~0x02));
	// de-assert interrupt to VD MCU51

	for (CheckCount = 0; CheckCount < 100; CheckCount++) {
		CheckFlag = _hal_dmd_riu_readbyte(MBRegBase + 0x1E);
		if ((CheckFlag & 0x02) == 0) {
			*pdata = _hal_dmd_riu_readbyte(MBRegBase + 0x10);
			break;
		}

		sys.delayms(1);
	}

	if (CheckFlag & 0x02) {
		PRINT("ERROR: ISDBT INTERN DEMOD MBX READ TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

#if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI


#if (DMD_ISDBT_CHIP_VERSION == DMD_ISDBT_CHIP_M7332)
static void _hal_intern_isdbt_initclk(void)
{
	HAL_INTERN_ISDBT_DBINFO(PRINT("--DMD_ISDBT_CHIP_M7332--\n"));

	// SRAM End Address
	_hal_dmd_riu_writebyte(0x111707, 0xff);
	_hal_dmd_riu_writebyte(0x111706, 0xff);

	// DRAM Disable
	_hal_dmd_riu_writebyte(0x111718,
	_hal_dmd_riu_readbyte(0x111718) & (~0x04));

	_hal_dmd_riu_writebytemask(0x101e39, 0x00, 0x03);

	_hal_dmd_riu_writebyte(0x101e68, 0xfc);
	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x00);
	_hal_dmd_riu_writebyte(0x103300, 0x0b);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x00);
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x04);
	_hal_dmd_riu_writebyte(0x111f01, 0x00);
	_hal_dmd_riu_writebyte(0x111f00, 0x0e);

	_hal_dmd_riu_writebytemask(0x101e39, 0x03, 0x03);
}

#else
static void _hal_intern_isdbt_initclk(void)
{
	PRINT("--------------DMD_ISDBT_CHIP_NONE--------------\n");
}
#endif

#else // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

// Please modify init clock function according to 3 party platform
static void _hal_intern_isdbt_initclk(void)
{
	PRINT("--------------DMD_ISDBT_CHIP_NONE--------------\n");
}

#endif // #if !DMD_ISDBT_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

static u8 _hal_intern_isdbt_ready(void)
{
	u8 udata = 0x00;

	_hal_dmd_riu_writebyte(MBRegBase + 0x1E, 0x02);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,
	_hal_dmd_riu_readbyte(DMDMcuBase + 0x03)|0x02);
	// assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,
	_hal_dmd_riu_readbyte(DMDMcuBase + 0x03)&(~0x02));
	// de-assert interrupt to VD MCU51

	sys.delayms(1);

	udata = _hal_dmd_riu_readbyte(MBRegBase + 0x1E);

	if (udata)
		return FALSE;

	_mbx_readreg(0x20C2, &udata);

	udata = udata & 0x0F;

	if (udata != 0x04)
		return FALSE;

	return TRUE;
}

static u8 _hal_intern_isdbt_download(void)
{
	#if (FW_RUN_ON_DRAM_MODE == 0)
	u8  udata = 0x00;
	u16 i = 0;
	u16 fail_cnt = 0;
	#endif
	u8  u8tmp_data;
	u16 addess_offset;
	const u8 *isdbt_table;
	u16 lib_size;
	#if (FW_RUN_ON_DRAM_MODE == 1)
	u64 dramcode_virtual_addr;
	#endif

	pres->sdmd_isdbt_pridata.tdi_dram_size = TDI_USE_DRAM_SIZE;

	if (pres->sdmd_isdbt_pridata.bdownloaded) {
		#if POW_SAVE_BY_DRV
		 #if !FW_RUN_ON_DRAM_MODE
		_hal_dmd_riu_writebyte(DMDMcuBase + 0x01, 0x01);
		//enable DMD MCU51 SRAM
		 #endif
		_hal_dmd_riu_writebytemask(DMDMcuBase +
		0x00, 0x00, 0x01);
		sys.delayms(1);
		#endif

		if (_hal_intern_isdbt_ready()) {
			#if REMAP_RST_BY_DRV
			_hal_dmd_riu_writebytemask(DMDMcuBase +
			0x00, 0x02, 0x02);
			// reset RIU remapping
			#endif
			_hal_dmd_riu_writebytemask(DMDMcuBase +
			0x00, 0x01, 0x01);
			// reset VD_MCU
			_hal_dmd_riu_writebytemask(DMDMcuBase +
			0x00, 0x00, 0x03);

			sys.delayms(20);

			return TRUE;
		}
	}

	if (pres->sdmd_isdbt_pridata.elast_type == DMD_ISDBT_DEMOD_8M) {
		isdbt_table = &intern_isdbt_8M_table[0];
		lib_size = sizeof(intern_isdbt_8M_table);
	} else {
		isdbt_table = &intern_isdbt_table[0];
		lib_size = sizeof(intern_isdbt_table);
	}

	#if REMAP_RST_BY_DRV
	_hal_dmd_riu_writebytemask(DMDMcuBase + 0x00, 0x02, 0x02);
	// reset RIU remapping
	#endif
	_hal_dmd_riu_writebytemask(DMDMcuBase + 0x00, 0x01, 0x01);
	// reset VD_MCU
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x01, 0x00);
	// disable SRAM

	_hal_dmd_riu_writebytemask(DMDMcuBase + 0x00, 0x00, 0x01);
	// release MCU, madison patch

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03, 0x50);
	// enable "vdmcu51_if"
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03, 0x51);
	// enable auto-increase

	#if (FW_RUN_ON_DRAM_MODE == 0)
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x04, 0x00);
	// sram address low byte
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x05, 0x00);
	// sram address high byte

	////  Load code thru VDMCU_IF ////
	HAL_INTERN_ISDBT_DBINFO(PRINT(">Load Code...\n"));

	for (i = 0; i < lib_size; i++) {
		_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, isdbt_table[i]);
		// write data to VD MCU 51 code sram
	}

	////  Content verification ////
	HAL_INTERN_ISDBT_DBINFO(PRINT(">Verify Code...\n"));

	_hal_dmd_riu_writebyte(DMDMcuBase+0x04, 0x00); // sram address low byte
	_hal_dmd_riu_writebyte(DMDMcuBase+0x05, 0x00); // sram address high byte

	for (i = 0; i < lib_size; i++) {
		udata =
		_hal_dmd_riu_readbyte(DMDMcuBase+0x10); // read sram data

		if (udata != isdbt_table[i]) {
			HAL_INTERN_ISDBT_DBINFO(PRINT(">fail add = 0x%x\n", i));
			HAL_INTERN_ISDBT_DBINFO(
			PRINT(">code = 0x%x\n", intern_isdbt_table[i]));
			HAL_INTERN_ISDBT_DBINFO(PRINT(">data = 0x%x\n", udata));

			if (fail_cnt++ > 10) {
				HAL_INTERN_ISDBT_DBINFO(
				PRINT(">DSP Loadcode fail!"));
				return FALSE;
			}
		}
	}

	addess_offset = (isdbt_table[0x400] << 8)|isdbt_table[0x401];

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x04, (addess_offset&0xFF));
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x05, (addess_offset>>8));

	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.if_khz;
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	u8tmp_data = (u8)(pres->sdmd_isdbt_initdata.if_khz >> 8);
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.iq_swap;
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.agc_reference_value;
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	u8tmp_data =
	(u8)(pres->sdmd_isdbt_initdata.agc_reference_value >> 8);
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.tdi_start_addr;
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	u8tmp_data = (u8)(pres->sdmd_isdbt_initdata.tdi_start_addr >> 8);
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	u8tmp_data = (u8)(pres->sdmd_isdbt_initdata.tdi_start_addr >> 16);
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	u8tmp_data = (u8)(pres->sdmd_isdbt_initdata.tdi_start_addr >> 24);
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x0C, u8tmp_data);
	#else  // #if (FW_RUN_ON_DRAM_MODE == 0)
	//download DRAM part
	// VA = MsOS_PA2KSEG1(PA); //NonCache
	HAL_INTERN_ISDBT_DBINFO(PRINT(">>> ISDBT_FW_DRAM_START_ADDR=0x%lx\n",
	((pres->sdmd_isdbt_initdata.tdi_start_addr * 16) +
	(4 * 1024 * 1024))));
	#if defined MTK_PROJECT
	dramcode_virtual_addr = (u64)BSP_MapReservedMem(
	(void *)((pres->sdmd_isdbt_initdata.tdi_start_addr * 16) +
	(4 * 1024 * 1024)), 1024*1024);
	#else
	dramcode_virtual_addr = MsOS_PA2KSEG1(
	((pres->sdmd_isdbt_initdata.tdi_start_addr * 16) +
	(4 * 1024 * 1024)));
	#endif
	MEMCPY((void *)(size_t)dramcode_virtual_addr, &isdbt_table[0],
	lib_size);

	addess_offset = (isdbt_table[0x400] << 8)|isdbt_table[0x401];

	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.if_khz;
	HAL_INTERN_ISDBT_DBINFO(
	PRINT("u16IF_KHZ=%d\n", pres->sdmd_isdbt_initdata.if_khz));
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset), &u8tmp_data, 1);
	u8tmp_data = (u8)(pres->sdmd_isdbt_initdata.if_khz >> 8);
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 1), &u8tmp_data, 1);
	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.iq_swap;
	HAL_INTERN_ISDBT_DBINFO(PRINT(
	"bIQSwap=%d\n", pres->sdmd_isdbt_initdata.iq_swap));
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 2), &u8tmp_data, 1);
	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.agc_reference_value;
	HAL_INTERN_ISDBT_DBINFO(PRINT("u16AGC_REFERENCE=%X\n",
	pres->sdmd_isdbt_initdata.agc_reference_value));
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 3), &u8tmp_data, 1);
	u8tmp_data =
	(u8)(pres->sdmd_isdbt_initdata.agc_reference_value >> 8);
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 4), &u8tmp_data, 1);
	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.tdi_start_addr;
	HAL_INTERN_ISDBT_DBINFO(PRINT(
	"u32TdiStartAddr=%X\n", pres->sdmd_isdbt_initdata.tdi_start_addr));
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 5), &u8tmp_data, 1);
	u8tmp_data = (u8)(pres->sdmd_isdbt_initdata.tdi_start_addr >> 8);
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 6), &u8tmp_data, 1);
	u8tmp_data = (u8)(pres->sdmd_isdbt_initdata.tdi_start_addr >> 16);
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 7), &u8tmp_data, 1);
	u8tmp_data = (u8)(pres->sdmd_isdbt_initdata.tdi_start_addr >> 24);
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 8), &u8tmp_data, 1);
	u8tmp_data = (u8)pres->sdmd_isdbt_initdata.tuner_gain_invert;
	MEMCPY((void *)(size_t)(
	dramcode_virtual_addr + addess_offset + 9), &u8tmp_data, 1);
	#if defined MTK_PROJECT
	BSP_UnMapReservedMem((void *)dramcode_virtual_addr);
	#endif

	HAL_INTERN_ISDBT_DBINFO(PRINT(">Load DRAM code done...\n"));
	#endif // #if (FW_RUN_ON_DRAM_MODE == 0)

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03, 0x50);
	// disable auto-increase
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03, 0x00);
	// disable "vdmcu51_if"

	_hal_dmd_riu_writebytemask(DMDMcuBase+0x00, 0x01, 0x01);
	// reset MCU, madison patch

	#if GLO_RST_IN_DOWNLOAD
	_hal_dmd_riu_writebyte(0x101E82, 0x00); // reset demod 0,1
	sys.delayms(1);
	_hal_dmd_riu_writebyte(0x101E82, 0x11);
	#endif

	#if FW_RUN_ON_DRAM_MODE == 0
	_hal_dmd_riu_writebyte(DMDMcuBase+0x01, 0x01); // enable SRAM
	#endif
	_hal_dmd_riu_writebytemask(DMDMcuBase+0x00, 0x00, 0x03);
	// release VD_MCU

	pres->sdmd_isdbt_pridata.bdownloaded = TRUE;

	sys.delayms(20);

	HAL_INTERN_ISDBT_DBINFO(PRINT(">DSP Loadcode done."));
	_hal_dmd_riu_writebytemask(0x101e39, 0x03, 0x00);
	_hal_dmd_riu_writebyte(0x111f00, 0x0e); //Temp patch
	PRINT("@@@@ 0x111f00 = %x\n", _hal_dmd_riu_readbyte(0x111f00));
	_hal_dmd_riu_writebytemask(0x101e39, 0x03, 0x03);
	return TRUE;
}

static void _hal_intern_isdbt_fw_version(void)
{
	u8 data1 = 0;
	u8 data2 = 0;
	u8 data3 = 0;

	_mbx_readreg(0x20C4, &data1);
	_mbx_readreg(0x20C5, &data2);
	_mbx_readreg(0x20C6, &data3);

	PRINT("INTERN_ISDBT_FW_VERSION:%x.%x.%x\n", data1, data2, data3);
}

static u8 _hal_intern_isdbt_exit(void)
{
	u8 check_count = 0;

	#if TDI_MIU_FREE_BY_DRV
	#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
	sys.miu_mask_req(0, MIU_CLIENT_ISDBT_TDI_W);
	sys.miu_mask_req(0, MIU_CLIENT_ISDBT_TDI_R);
	if (sys.miu_is_support_miu1()) {
		sys.miu_mask_req(1, MIU_CLIENT_ISDBT_TDI_W);
		sys.miu_mask_req(1, MIU_CLIENT_ISDBT_TDI_R);
	}
	#else
	u8 regval_tmp = 0;

	regval_tmp = _hal_dmd_riu_readbyte(0x101200+ISDBT_MIU_CLIENTW_ADDR);
	if (regval_tmp & ISDBT_MIU_CLIENTW_BIT_MASK) {
		_hal_dmd_riu_writebytemask(0x100600 + ISDBT_MIU_CLIENTW_MASK,
		ISDBT_MIU_CLIENTW_BIT_MASK, ISDBT_MIU_CLIENTW_BIT_MASK);
		_hal_dmd_riu_writebytemask(0x100600 + ISDBT_MIU_CLIENTR_MASK,
		ISDBT_MIU_CLIENTR_BIT_MASK, ISDBT_MIU_CLIENTR_BIT_MASK);
	} else {
		_hal_dmd_riu_writebytemask(0x101200 + ISDBT_MIU_CLIENTW_MASK,
		ISDBT_MIU_CLIENTW_BIT_MASK, ISDBT_MIU_CLIENTW_BIT_MASK);
		_hal_dmd_riu_writebytemask(0x101200 + ISDBT_MIU_CLIENTR_MASK,
		ISDBT_MIU_CLIENTR_BIT_MASK, ISDBT_MIU_CLIENTR_BIT_MASK);
	}
	#endif
	#endif // #if TDI_MIU_FREE_BY_DRV

	_hal_dmd_riu_writebyte(MBRegBase + 0x1C, 0x01);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,
	_hal_dmd_riu_readbyte(DMDMcuBase + 0x03) | 0x02);
	// assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,
	_hal_dmd_riu_readbyte(DMDMcuBase + 0x03) & (~0x02));
	// de-assert interrupt to VD MCU51

	while ((_hal_dmd_riu_readbyte(MBRegBase + 0x1C) & 0x02) != 0x02) {
		sys.delayms(1);

		if (check_count++ == 0xFF) {
			PRINT(">> ISDBT Exit Fail!\n");
			return FALSE;
		}
	}

	PRINT(">> ISDBT Exit Ok!\n");

	#if POW_SAVE_BY_DRV
	#if defined MTK_PROJECT
	IO_WRITE32MSK(CKGEN_BASE, 0xb14, 0x00000000, 0x00F00000);
	TZ_DEMOD_RESET(0);
	IO_WRITE32MSK(CKGEN_BASE, 0x1c4, 0x00000300, 0x00000F00);
	IO_WRITE32MSK(CKGEN_BASE, 0x5D8, 0x03000000, 0x0F000000); //ES2
	#else
	_hal_dmd_riu_writebyte(0x101e83, 0x00);
	_hal_dmd_riu_writebyte(0x101e82, 0x00);

	sys.delayms(1);

	_hal_dmd_riu_writebyte(0x101e83, 0x00);
	_hal_dmd_riu_writebyte(0x101e82, 0x11);
	#endif
	#endif

	return TRUE;
}

static u8 _hal_intern_isdbt_softreset(void)
{
	u8 data = 0;

	#if TDI_MIU_FREE_BY_DRV
	#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
	sys.miu_mask_req(0, MIU_CLIENT_ISDBT_TDI_W);
	sys.miu_mask_req(0, MIU_CLIENT_ISDBT_TDI_R);
	if (sys.miu_is_support_miu1()) {
		sys.miu_mask_req(1, MIU_CLIENT_ISDBT_TDI_W);
		sys.miu_mask_req(1, MIU_CLIENT_ISDBT_TDI_R);
	}
	#else
	u8 regval_tmp = 0;

	regval_tmp = _hal_dmd_riu_readbyte(0x101200+ISDBT_MIU_CLIENTW_ADDR);
	if (regval_tmp & ISDBT_MIU_CLIENTW_BIT_MASK) {
		_hal_dmd_riu_writebytemask(0x100600 + ISDBT_MIU_CLIENTW_MASK,
		ISDBT_MIU_CLIENTW_BIT_MASK, ISDBT_MIU_CLIENTW_BIT_MASK);
		_hal_dmd_riu_writebytemask(0x100600 + ISDBT_MIU_CLIENTR_MASK,
		ISDBT_MIU_CLIENTR_BIT_MASK, ISDBT_MIU_CLIENTR_BIT_MASK);
	} else {
		_hal_dmd_riu_writebytemask(0x101200 + ISDBT_MIU_CLIENTW_MASK,
		ISDBT_MIU_CLIENTW_BIT_MASK, ISDBT_MIU_CLIENTW_BIT_MASK);
		_hal_dmd_riu_writebytemask(0x101200 + ISDBT_MIU_CLIENTR_MASK,
		ISDBT_MIU_CLIENTR_BIT_MASK, ISDBT_MIU_CLIENTR_BIT_MASK);
	}
	#endif
	#endif // #if TDI_MIU_FREE_BY_DRV

	//Reset FSM
	if (_mbx_writereg(0x20C0, 0x00) == FALSE)
		return FALSE;

	while (data != 0x02) {
		if (_mbx_readreg(0x20C1, &data) == FALSE)
			return FALSE;
	}

	return TRUE;
}

static u8 _hal_intern_isdbt_set_aci_coeff(void)
{
	return TRUE;
}

static u8 _hal_intern_isdbt_set_isdbtmode(void)
{
	#if TDI_MIU_FREE_BY_DRV
	#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
	sys.miu_unmask_req(0, MIU_CLIENT_ISDBT_TDI_W);
	sys.miu_unmask_req(0, MIU_CLIENT_ISDBT_TDI_R);
	if (sys.miu_is_support_miu1()) {
		sys.miu_unmask_req(1, MIU_CLIENT_ISDBT_TDI_W);
		sys.miu_unmask_req(1, MIU_CLIENT_ISDBT_TDI_R);
	}
	#else
	u8 regval_tmp = 0;

	regval_tmp = _hal_dmd_riu_readbyte(0x101200+ISDBT_MIU_CLIENTW_ADDR);
	if (regval_tmp & ISDBT_MIU_CLIENTW_BIT_MASK) {
		_hal_dmd_riu_writebytemask(0x100600 + ISDBT_MIU_CLIENTW_MASK,
		0, ISDBT_MIU_CLIENTW_BIT_MASK);
		_hal_dmd_riu_writebytemask(0x100600 + ISDBT_MIU_CLIENTR_MASK,
		0, ISDBT_MIU_CLIENTR_BIT_MASK);
	} else {
		_hal_dmd_riu_writebytemask(0x101200 + ISDBT_MIU_CLIENTW_MASK,
		0, ISDBT_MIU_CLIENTW_BIT_MASK);
		_hal_dmd_riu_writebytemask(0x101200 + ISDBT_MIU_CLIENTR_MASK,
		0, ISDBT_MIU_CLIENTR_BIT_MASK);
	}
	#endif
	#endif // #if TDI_MIU_FREE_BY_DRV

	if (_mbx_writereg(0x20C2, 0x04) == FALSE)
		return FALSE;
	return _mbx_writereg(0x20C0, 0x04);
}

static u8 _hal_intern_isdbt_set_modeclean(void)
{
	if (_mbx_writereg(0x20C2, 0x07) == FALSE)
		return FALSE;
	return _mbx_writereg(0x20C0, 0x00);
}

static u8 _hal_intern_isdbt_check_ewbs_lock(void)
{
	u8 check_pass = FALSE;
	u8 data = 0;

	_mbx_readreg(ISDBT_FDP_REG_BASE + 0x08, &data);
	if ((data & 0x01) != 0x00)       // Check EWBS TMCC FLAG
		check_pass = TRUE;

	return check_pass;
}

static u8 _hal_intern_isdbt_check_fec_lock(void)
{
	u8 check_pass = FALSE;
	u8 data = 0;

	_mbx_readreg(REG_ISDBT_LOCK_STATUS2, &data);
	if ((data & 0x04) != 0x00)       // Check FEC Lock Flag
		check_pass = TRUE;

	return check_pass;
}

static u8 _hal_intern_isdbt_check_fsa_track_lock(void)
{
	u8 check_pass = FALSE;
	u8 data = 0;

	_mbx_readreg(REG_ISDBT_LOCK_STATUS, &data);

	if ((data & 0x01) != 0x00) // Check FSA Track Lock Flag
		check_pass = TRUE;

	return check_pass;
}

static u8 _hal_intern_isdbt_check_psync_lock(void)
{
	u8 check_pass = FALSE;
	u8 data = 0;

	_mbx_readreg(REG_ISDBT_LOCK_STATUS, &data);

	if ((data & 0x04) != 0x00) // Check Psync Lock Flag
		check_pass = TRUE;

	return check_pass;
}

static u8 _hal_intern_isdbt_check_icfo_ch_exist_lock(void)
{
	u8 check_pass = FALSE;
	u8 data = 0;

	_mbx_readreg(REG_ISDBT_LOCK_STATUS2, &data);
	if ((data & 0x02) != 0x00)       // Check ICFO Lock Flag
		check_pass = TRUE;

	return check_pass;
}

static u8 _hal_intern_isdbt_get_signal_code_rate(
enum en_isdbt_layer elayer_index, enum en_isdbt_code_rate *peisdbt_code_rate)
{
	u8 ret = TRUE;
	u8 data = 0;
	u8 code_rate = 0;

	switch (elayer_index) {
	case E_ISDBT_Layer_A:
		// [10:8] reg_tmcc_cur_convolution_code_rate_a
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x04*2+1, &data);
		code_rate = data & 0x07;
		break;
	case E_ISDBT_Layer_B:
		// [10:8] reg_tmcc_cur_convolution_code_rate_b
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x05*2 + 1, &data);
		code_rate = data & 0x07;
		break;
	case E_ISDBT_Layer_C:
		// [10:8] reg_tmcc_cur_convolution_code_rate_c
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x06*2 + 1, &data);
		code_rate = data & 0x07;
		break;
	default:
		code_rate = 15;
		break;
	}
	switch (code_rate) {
	case 0:
		*peisdbt_code_rate = E_ISDBT_CODERATE_1_2;
		break;
	case 1:
		*peisdbt_code_rate = E_ISDBT_CODERATE_2_3;
		break;
	case 2:
		*peisdbt_code_rate = E_ISDBT_CODERATE_3_4;
		break;
	case 3:
		*peisdbt_code_rate = E_ISDBT_CODERATE_5_6;
		break;
	case 4:
		*peisdbt_code_rate = E_ISDBT_CODERATE_7_8;
		break;
	default:
		*peisdbt_code_rate = E_ISDBT_CODERATE_INVALID;
		break;
	}
	return ret;
}

static u8 _hal_intern_isdbt_get_signalguardinterval(
enum en_isdbt_guard_interval *peisdbt_gi)
{
	u8 ret = TRUE;
	u8 data = 0;
	u8 cp = 0;

	// [7:6] reg_mcd_out_cp
	// output cp -> 00: 1/4
	//                    01: 1/8
	//                    10: 1/16
	//                    11: 1/32

	#if USE_NEW_MODECP_DET
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x5A*2, &data);
	cp = (data >> 4) & 0x03;
	#else
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x34*2, &data);
	cp = (data >> 6) & 0x03;
	#endif

	switch (cp) {
	case 0:
		*peisdbt_gi = E_ISDBT_GUARD_INTERVAL_1_4;
		break;
	case 1:
		*peisdbt_gi = E_ISDBT_GUARD_INTERVAL_1_8;
		break;
	case 2:
		*peisdbt_gi = E_ISDBT_GUARD_INTERVAL_1_16;
		break;
	case 3:
		*peisdbt_gi = E_ISDBT_GUARD_INTERVAL_1_32;
		break;
	}
	return ret;
}

static u8 _hal_intern_isdbt_get_signaltimeinterleaving(
enum en_isdbt_layer elayer_index,
enum en_isdbt_time_interleaving *peisdbt_tdi)
{
	u8 ret = TRUE;
	u8 data = 0;
	u8 mode = 0;
	u8 tdi = 0;

	// [5:4] reg_mcd_out_mode
	// output mode  -> 00: 2k
	//                         01: 4k
	//                         10: 8k

	#if USE_NEW_MODECP_DET
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE+0x5A*2, &data);
	mode  = data & 0x03;
	#else
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE+0x34*2, &data);
	mode  = (data >> 4) & 0x03;
	#endif

	switch (elayer_index) {
	case E_ISDBT_Layer_A:
		// [14:12] reg_tmcc_cur_interleaving_length_a
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE+0x04*2+1, &data);
		tdi = (data >> 4) & 0x07;
		break;
	case E_ISDBT_Layer_B:
		// [14:12] reg_tmcc_cur_interleaving_length_b
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE+0x05*2+1, &data);
		tdi = (data >> 4) & 0x07;
		break;
	case E_ISDBT_Layer_C:
		// [14:12] reg_tmcc_cur_interleaving_length_c
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE+0x06*2+1, &data);
		tdi = (data >> 4) & 0x07;
		break;
	default:
		tdi = 15;
		break;
	}
	// u8Tdi+u8Mode*4
	// => 0~3: 2K
	// => 4~7: 4K
	// => 8~11:8K
	switch (tdi+mode*4) {
	case 0:
		*peisdbt_tdi = E_ISDBT_2K_TDI_0;
		break;
	case 1:
		*peisdbt_tdi = E_ISDBT_2K_TDI_4;
		break;
	case 2:
		*peisdbt_tdi = E_ISDBT_2K_TDI_8;
		break;
	case 3:
		*peisdbt_tdi = E_ISDBT_2K_TDI_16;
		break;
	case 4:
		*peisdbt_tdi = E_ISDBT_4K_TDI_0;
		break;
	case 5:
		*peisdbt_tdi = E_ISDBT_4K_TDI_2;
		break;
	case 6:
		*peisdbt_tdi = E_ISDBT_4K_TDI_4;
		break;
	case 7:
		*peisdbt_tdi = E_ISDBT_4K_TDI_8;
		break;
	case 8:
		*peisdbt_tdi = E_ISDBT_8K_TDI_0;
		break;
	case 9:
		*peisdbt_tdi = E_ISDBT_8K_TDI_1;
		break;
	case 10:
		*peisdbt_tdi = E_ISDBT_8K_TDI_2;
		break;
	case 11:
		*peisdbt_tdi = E_ISDBT_8K_TDI_4;
		break;
	default:
		*peisdbt_tdi = E_ISDBT_TDI_INVALID;
		break;
	}
	return ret;
}

static u8 _hal_intern_isdbt_get_signalfftvalue(
enum en_isdbt_fft_val *peisdbt_fft)
{
	u8 ret = TRUE;
	u8 data = 0;
	u8 mode = 0;

	// [5:4]  reg_mcd_out_mode
	// output mode  -> 00: 2k
	//                         01: 4k
	//                         10: 8k

	#if USE_NEW_MODECP_DET
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE+0x5A*2, &data);
	mode  = data & 0x03;
	#else
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE+0x34*2, &data);
	mode  = (data >> 4) & 0x03;
	#endif

	switch (mode) {
	case 0:
		*peisdbt_fft = E_ISDBT_FFT_2K;
		break;
	case 1:
		*peisdbt_fft = E_ISDBT_FFT_4K;
		break;
	case 2:
		*peisdbt_fft = E_ISDBT_FFT_8K;
		break;
	default:
		*peisdbt_fft = E_ISDBT_FFT_INVALID;
		break;
	}
	return ret;
}

static u8 _hal_intern_isdbt_get_signalmodulation(
enum en_isdbt_layer elayer_index,
enum en_isdbt_constel_type *peisdbt_constellation)
{
	u8 ret = TRUE;
	u8 data = 0;
	u8 qam = 0;

	switch (elayer_index) {
	case E_ISDBT_Layer_A:
		// [6:4] reg_tmcc_cur_carrier_modulation_a
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE+0x04*2, &data);
		qam = (data >> 4) & 0x07;
		break;
	case E_ISDBT_Layer_B:
		// [6:4] reg_tmcc_cur_carrier_modulation_b
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE+0x05*2, &data);
		qam = (data >> 4) & 0x07;
		break;
	case E_ISDBT_Layer_C:
		// [6:4] reg_tmcc_cur_carrier_modulation_c
		ret &= _mbx_readreg(ISDBT_FDP_REG_BASE+0x06*2, &data);
		qam = (data >> 4) & 0x07;
		break;
	default:
		qam = 15;
		break;
	}
	switch (qam) {
	case 0:
		*peisdbt_constellation = E_ISDBT_DQPSK;
		break;
	case 1:
		*peisdbt_constellation = E_ISDBT_QPSK;
		break;
	case 2:
		*peisdbt_constellation = E_ISDBT_16QAM;
		break;
	case 3:
		*peisdbt_constellation = E_ISDBT_64QAM;
		break;
	default:
		*peisdbt_constellation = E_ISDBT_QAM_INVALID;
		break;
	}
	return ret;
}

static u8 _hal_intern_isdbt_read_ifagc(void)
{
	u8 data = 0;

	_mbx_readreg(0x28FD, &data);

	return data;
}

#ifdef UTPA2
static u8 _hal_intern_isdbt_get_freqoffset(u8 *pfft_mode,
s32 *ptdcfo_regvalue, s32 *pfdcfo_regvalue, s16 *picfo_regvalue)
#else
static u8 _hal_intern_isdbt_get_freqoffset(float *pfreq_offset)
#endif
{
	u8 ret = TRUE;
	u8   data = 0;
	s32  tdcfo_regvalue = 0;
	s32  fdcfo_regvalue = 0;
	s16  icfo_regvalue = 0;
	#ifndef UTPA2
	float   ftdcfofreq = 0.0;
	float   ficfofreq = 0.0;
	float   ffdcfofreq = 0.0;
	#endif

	//Get TD CFO
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x04, &data);
	ret &= _mbx_writereg(ISDBT_TDP_REG_BASE + 0x04, (data|0x01));

	//read td_freq_error
	//Read <29,38>
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x8A, &data);
	tdcfo_regvalue = data;
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x8B, &data);
	tdcfo_regvalue |= data << 8;
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x8C, &data);
	tdcfo_regvalue = data << 16;
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x8D, &data);
	tdcfo_regvalue |= data << 24;

	if (data >= 0x10)
		tdcfo_regvalue = 0xE0000000 | tdcfo_regvalue;

	tdcfo_regvalue >>= 4;

	//TD_cfo_Hz = RegCfoTd * fb
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x04, &data);
	ret &= _mbx_writereg(ISDBT_TDP_REG_BASE + 0x04, (data&~0x01));

	#ifndef UTPA2
	ftdcfofreq = ((float)tdcfo_regvalue) / 17179869184.0; //<25,34>
	ftdcfofreq = ftdcfofreq * 8126980.0;
	#endif

	//Get FD CFO
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0xFE, &data);
	ret &= _mbx_writereg(ISDBT_FDP_REG_BASE + 0xFE, (data|0x01));
	//load
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0xFF, &data);
	ret &= _mbx_writereg(ISDBT_FDP_REG_BASE + 0xFF, (data|0x01));

	//read CFO_KI
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x5E, &data);
	fdcfo_regvalue = data;
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x5F, &data);
	fdcfo_regvalue |= data << 8;
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x60, &data);
	fdcfo_regvalue |= data << 16;
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x61, &data);
	fdcfo_regvalue |= data << 24;

	if (data >= 0x01)
		fdcfo_regvalue = 0xFE000000 | fdcfo_regvalue;

	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0xFE, &data);
	ret &= _mbx_writereg(ISDBT_FDP_REG_BASE + 0xFE, (data&~0x01));
	//load
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0xFF, &data);
	ret &= _mbx_writereg(ISDBT_FDP_REG_BASE + 0xFF, (data|0x01));

	#ifndef UTPA2
	ffdcfofreq = ((float)fdcfo_regvalue) / 17179869184.0;
	ffdcfofreq = ffdcfofreq * 8126980.0;
	#endif

	//Get ICFO
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x5C, &data);
	icfo_regvalue = data;
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0x5D, &data);
	icfo_regvalue |= data << 8;
	icfo_regvalue = (icfo_regvalue >> 4) & 0x07FF;

	if (icfo_regvalue >= 0x400)
		icfo_regvalue = icfo_regvalue | 0xFFFFF800;

	#if USE_NEW_MODECP_DET
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x5A*2, &data);
	data = data << 4;
	#else
	ret &= _mbx_readreg(ISDBT_TDP_REG_BASE + 0x68, &data);
	#endif

	#ifdef UTPA2
	*pfft_mode = data;
	*ptdcfo_regvalue = tdcfo_regvalue;
	*pfdcfo_regvalue = fdcfo_regvalue;
	*picfo_regvalue = icfo_regvalue;
	#else
	if ((data & 0x30) == 0x0000) // 2k
		ficfofreq = (float)icfo_regvalue*250000.0/63.0;
	else if ((data & 0x0030) == 0x0010)	// 4k
		ficfofreq = (float)icfo_regvalue*125000.0/63.0;
	else //if (u16data & 0x0030 == 0x0020) // 8k
		ficfofreq = (float)icfo_regvalue*125000.0/126.0;

	*pfreq_offset = ftdcfofreq + ffdcfofreq + ficfofreq;

	HAL_INTERN_ISDBT_DBINFO(PRINT("Total CFO value = %f\n", *pfreq_offset));
	#endif

	return ret;
}

#ifdef UTPA2
static u8 _hal_intern_isdbt_get_previterbi_ber(
enum en_isdbt_layer elayer_index, u32 *pber_value, u16 *pber_period)
#else
static u8 _hal_intern_isdbt_get_previterbi_ber(
enum en_isdbt_layer elayer_index, float *pfber)
#endif
{
	u8 ret = TRUE;
	u8   data = 0;
	u16  ber_value = 0;
	u32  ber_period = 0;

	// reg_rd_freezeber
	ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x60, &data);
	ret &= _mbx_writereg(ISDBT_OUTER_REG_BASE + 0x60, data|0x10);

	if (elayer_index == E_ISDBT_Layer_A) {
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x90, &data);
		ber_value = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x91, &data);
		ber_value |= (data << 8);
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x76, &data);
		ber_period = (data&0x3F);
		ber_period <<= 16;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x70, &data);
		ber_period |= data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x70, &data);
		ber_period |= (data << 8);
	} else if (elayer_index == E_ISDBT_Layer_B) {
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x92, &data);
		ber_value = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x93, &data);
		ber_value |= (data << 8);
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x77, &data);
		ber_period = (data&0x3F);
		ber_period <<= 16;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x72, &data);
		ber_period |= data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x73, &data);
		ber_period |= (data << 8);
	} else if (elayer_index == E_ISDBT_Layer_C) {
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x94, &data);
		ber_value = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x95, &data);
		ber_value |= (data << 8);
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x78, &data);
		ber_period = (data&0x003F);
		ber_period <<= 16;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x74, &data);
		ber_period |= data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x75, &data);
		ber_period |= (data << 8);
	} else {
		HAL_INTERN_ISDBT_DBINFO(PRINT("Please select correct Layer\n"));
		ret = FALSE;
	}

	// reg_rd_freezeber
	ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x60, &data);
	ret &= _mbx_writereg(ISDBT_OUTER_REG_BASE + 0x60, (data&~0x10));

	ber_period <<= 8; // *256

	if (ber_period == 0)
		ber_period = 1;

	#ifdef UTPA2
	*pber_period = ber_period;
	*pber_value = ber_value;
	#else
	*pfber = (float)ber_value/ber_period;
	HAL_INTERN_ISDBT_DBINFO(PRINT("Layer: 0x%x, Pre-Ber = %e\n",
	elayer_index, *pfber));
	#endif

	return ret;
}

#ifdef UTPA2
static u8 _hal_intern_isdbt_get_postviterbi_ber(
enum en_isdbt_layer elayer_index, u32 *pber_value, u16 *pber_period)
#else
static u8 _hal_intern_isdbt_get_postviterbi_ber(
enum en_isdbt_layer elayer_index, float *pfber)
#endif
{
	u8 ret = TRUE;
	u8   data = 0;
	u8   frz_data = 0;
	u32  ber_value = 0;
	u16  ber_period = 0;

	// reg_rd_freezeber
	ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE+0x01*2+1, &frz_data);
	data = frz_data | 0x01;
	ret &= _mbx_writereg(ISDBT_OUTER_REG_BASE+0x01*2+1, data);

	if (elayer_index == E_ISDBT_Layer_A) {
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x14, &data);
		ber_value = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x15, &data);
		ber_value |= data << 8;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x16, &data);
		ber_value |= data << 16;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x17, &data);
		ber_value |= data << 24;

		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x0A, &data);
		ber_period = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x0B, &data);
		ber_period |= data << 8;
	} else if (elayer_index == E_ISDBT_Layer_B) {
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x46, &data);
		ber_value = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x47, &data);
		ber_value |= data << 8;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x48, &data);
		ber_value |= data << 16;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x49, &data);
		ber_value |= data << 24;

		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x3A, &data);
		ber_period = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x3B, &data);
		ber_period |= data << 8;
	} else if (elayer_index == E_ISDBT_Layer_C) {
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x88, &data);
		ber_value = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x89, &data);
		ber_value |= data << 8;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x8A, &data);
		ber_value |= data << 16;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x8B, &data);
		ber_value |= data << 24;

		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x3E, &data);
		ber_period = data;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x3F, &data);
		ber_period |= data << 8;
	} else {
		HAL_INTERN_ISDBT_DBINFO(PRINT("Please select correct Layer\n"));
		ret = FALSE;
	}

	// reg_rd_freezeber
	ret &= _mbx_writereg(ISDBT_OUTER_REG_BASE+0x01*2+1, frz_data);

	if (ber_period == 0)
		ber_period = 1;

	#ifdef UTPA2
	*pber_period = ber_period;
	*pber_value = ber_value;
	#else
	*pfber = (float)ber_value/ber_period/(128.0*188.0*8.0);
	HAL_INTERN_ISDBT_DBINFO(PRINT("Layer: 0x%x, Post-Ber = %e\n",
	elayer_index, *pfber));
	#endif
	return ret;
}

static u16 _hal_intern_isdbt_get_signalqualityoflayerA(void)
{
	#ifdef UTPA2
	u32 ber_value  = 0;
	u16 ber_period = 0;
	#else
	float fber;
	#endif
	u8 ret = TRUE;
	enum en_isdbt_layer elayer_index;
	u16 sqi;

	// Tmp solution
	elayer_index = E_ISDBT_Layer_A;

	if (_hal_intern_isdbt_check_fec_lock() == FALSE) {
		//PRINT("Dan Demod unlock!!!\n");
		sqi = 0;
	} else {
		// Part 1: get ber value from demod.
		#ifdef UTPA2
		ret &= _hal_intern_isdbt_get_postviterbi_ber(elayer_index,
		&ber_value, &ber_period);

		sqi = _calculate_sqi(ber_value, ber_period*128*188*8);
		#else
		ret &= _hal_intern_isdbt_get_postviterbi_ber(elayer_index,
		&fber);

		sqi = _calculate_sqi(fber);
		#endif
	}

	//PRINT("dan SQI = %d\n", SQI);
	return sqi;
}

static u16 _hal_intern_isdbt_get_signalqualityoflayerB(void)
{
	#ifdef UTPA2
	u32 ber_value  = 0;
	u16 ber_period = 0;
	#else
	float fber;
	#endif
	u8 ret = TRUE;
	enum en_isdbt_layer elayer_index;
	u16 sqi;

	// Tmp solution
	elayer_index = E_ISDBT_Layer_B;

	if (_hal_intern_isdbt_check_fec_lock() == FALSE) {
		//PRINT("Dan Demod unlock!!!\n");
		sqi = 0;
	} else {
		// Part 1: get ber value from dem        .
		#ifdef UTPA2
		ret &= _hal_intern_isdbt_get_postviterbi_ber(elayer_index,
		&ber_value, &ber_period);

		sqi = _calculate_sqi(ber_value, ber_period*128*188*8);
		#else
		ret &= _hal_intern_isdbt_get_postviterbi_ber(elayer_index,
		&fber);

		sqi = _calculate_sqi(fber);
		#endif
	}

	//PRINT("dan SQI = %d\n", SQI);
	return sqi;
}

static u16 _hal_intern_isdbt_get_signalqualityoflayerC(void)
{
	#ifdef UTPA2
	u32 ber_value  = 0;
	u16 ber_period = 0;
	#else
	float fber;
	#endif
	u8 ret = TRUE;
	enum en_isdbt_layer elayer_index;
	u16 sqi;

	// Tmp solution
	elayer_index = E_ISDBT_Layer_C;

	if (_hal_intern_isdbt_check_fec_lock() == FALSE) {
		//PRINT("Dan Demod unlock!!!\n");
		sqi = 0;
	} else {
		// Part 1: get ber value from demod.
		#ifdef UTPA2
		ret &= _hal_intern_isdbt_get_postviterbi_ber(elayer_index,
		&ber_value, &ber_period);

		sqi = _calculate_sqi(ber_value, ber_period*128*188*8);
		#else
		ret &= _hal_intern_isdbt_get_postviterbi_ber(elayer_index,
		&fber);

		sqi = _calculate_sqi(fber);
		#endif
	}

	//PRINT("dan SQI = %d\n", SQI);
	return sqi;
}

static u16 _hal_intern_isdbt_get_signalqualityoflayer_combine(void)
{
	s8  layerA_val = 0, layerB_val = 0, layerC_val = 0;
	u16 sqi;
	enum en_isdbt_layer elayer_index;
	enum en_isdbt_constel_type eIsdbtConstellationA,
	eIsdbtConstellationB, eIsdbtConstellationC;

	//Get modulation of each layer
	elayer_index = E_ISDBT_Layer_A;
	_hal_intern_isdbt_get_signalmodulation(elayer_index,
	&eIsdbtConstellationA);
	elayer_index = E_ISDBT_Layer_B;
	_hal_intern_isdbt_get_signalmodulation(elayer_index,
	&eIsdbtConstellationB);
	elayer_index = E_ISDBT_Layer_C;
	_hal_intern_isdbt_get_signalmodulation(elayer_index,
	&eIsdbtConstellationC);

	if (eIsdbtConstellationA != E_ISDBT_QAM_INVALID)
		layerA_val = (s8)eIsdbtConstellationA;
	else
		layerA_val = -1;

	if (eIsdbtConstellationB != E_ISDBT_QAM_INVALID)
		layerB_val = (s8)eIsdbtConstellationB;
	else
		layerB_val = -1;

	if (eIsdbtConstellationC != E_ISDBT_QAM_INVALID)
		layerC_val = (s8)eIsdbtConstellationC;
	else
		layerC_val = -1;


	if (layerA_val >= layerB_val) {
		if (layerC_val >= layerA_val) {
			//Get Layer C u16SQI
			sqi = _hal_intern_isdbt_get_signalqualityoflayerC();
			//PRINT("dan u16SQI Layer C1: %d\n", u16SQI);
		} else { //A>C
			//Get Layer A u16SQI
			sqi = _hal_intern_isdbt_get_signalqualityoflayerA();
			//PRINT("dan u16SQI Layer A: %d\n", u16SQI);
		}
	} else { // B >= A
		if (layerC_val >= layerB_val) {
			//Get Layer C u16SQI
			sqi = _hal_intern_isdbt_get_signalqualityoflayerC();
			//PRINT("dan u16SQI Layer C2: %d\n", u16SQI);
		} else {//B>C
			//Get Layer B u16SQI
			sqi = _hal_intern_isdbt_get_signalqualityoflayerB();
			//PRINT("dan u16SQI Layer B: %d\n", u16SQI);
		}
	}

	return sqi;
}

#ifdef UTPA2
static u8 _hal_intern_isdbt_get_snr(u32 *preg_snr,
u16 *preg_snr_obs_num)
#else
static u8 _hal_intern_isdbt_get_snr(float *pf_snr)
#endif
{
	u8 ret = TRUE;
	u8   data = 0;
	u32  reg_snr = 0;
	u16  reg_snr_obs_num = 0;
	#ifndef UTPA2
	float   fsnr_avg = 0.0;
	#endif

	//set freeze
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0xFE, &data);
	ret &= _mbx_writereg(ISDBT_FDP_REG_BASE + 0xFE, (data|0x01));
	//load
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0xFF, &data);
	ret &= _mbx_writereg(ISDBT_FDP_REG_BASE + 0xFF, (data|0x01));

	// ==============Average SNR===============//
	// [26:0] reg_snr_accu
	ret &= _mbx_readreg(ISDBT_FDPEXT_REG_BASE + 0x2d*2+1, &data);
	reg_snr = data&0x07;
	ret &= _mbx_readreg(ISDBT_FDPEXT_REG_BASE + 0x2d*2, &data);
	reg_snr = (reg_snr<<8) | data;
	ret &= _mbx_readreg(ISDBT_FDPEXT_REG_BASE + 0x2c*2+1, &data);
	reg_snr = (reg_snr<<8) | data;
	ret &= _mbx_readreg(ISDBT_FDPEXT_REG_BASE + 0x2c*2, &data);
	reg_snr = (reg_snr<<8) | data;

	// [12:0] reg_snr_observe_sum_num
	ret &= _mbx_readreg(ISDBT_FDPEXT_REG_BASE + 0x2a*2+1, &data);
	reg_snr_obs_num = data&0x1f;
	ret &= _mbx_readreg(ISDBT_FDPEXT_REG_BASE + 0x2a*2, &data);
	reg_snr_obs_num = (reg_snr_obs_num<<8) | data;

	//release freeze
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0xFE, &data);
	ret &= _mbx_writereg(ISDBT_FDP_REG_BASE + 0xFE, (data&~0x01));
	//load
	ret &= _mbx_readreg(ISDBT_FDP_REG_BASE + 0xFF, &data);
	ret &= _mbx_writereg(ISDBT_FDP_REG_BASE + 0xFF, (data|0x01));

	if (reg_snr_obs_num == 0)
		reg_snr_obs_num = 1;


	#ifdef UTPA2
	 *preg_snr = 10 * log10_approx(reg_snr/reg_snr_obs_num / 2);
	 HAL_INTERN_ISDBT_DBINFO(PRINT("SNR*100 value = %u\n", *preg_snr));
	#else
	 fsnr_avg = (float)reg_snr/reg_snr_obs_num;
	if (fsnr_avg == 0)                 //protect value 0
		fsnr_avg = 0.01;

	 #ifdef MSOS_TYPE_LINUX
	 *pf_snr = 10.0f * (float)log10f((double)fsnr_avg / 2);
	 #else
	 *pf_snr = 10.0f*(float)log10_approx((double)fsnr_avg / 2);
	 #endif
	 HAL_INTERN_ISDBT_DBINFO(PRINT("SNR value = %f\n", *pf_snr));
	#endif

	return ret;
}

static u8 _hal_intern_isdbt_read_pkt_err(
enum en_isdbt_layer elayer_index, u16 *packet_err)
{
	u8 ret = TRUE;
	u8 data = 0;
	u8 frz_data = 0;
	u16 packet_err_A = 0xFFFF;
	u16 packet_err_B = 0xFFFF;
	u16 packet_err_C = 0xFFFF;
	#if DMD_ISDBT_TBVA_EN
	u8 tbva_bypass = 0;
	u8 tbva_layer = 0;
	#endif
	// Read packet errors of three layers
	// OUTER_FUNCTION_ENABLE
	// [8] reg_biterr_num_pcktprd_freeze
	// Freeze Packet error
	ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x01 * 2 + 1, &frz_data);
	data = frz_data | 0x01;
	ret &= _mbx_writereg(ISDBT_OUTER_REG_BASE + 0x01 * 2 + 1, data);
#if DMD_ISDBT_TBVA_EN
	ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x10 * 2, &data);
	tbva_bypass = data & 0x01;
	ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x11 * 2, &data);
	tbva_layer = data & 0x03;
	switch (elayer_index) {
	case E_ISDBT_Layer_A:
		// [15:0] OUTER_UNCRT_PKT_NUM_PCKTPRD_A
		if (!tbva_bypass && tbva_layer == 0) {
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x17*2+1, &data);
			packet_err_A = data << 8;
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x17*2, &data);
			packet_err_A = packet_err_A | data;
			*packet_err = packet_err_A;
		} else {
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x08*2+1, &data);
			packet_err_A = data << 8;
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x08*2, &data);
			packet_err_A = packet_err_A | data;
			*packet_err = packet_err_A;
		}
		break;
	case E_ISDBT_Layer_B:
		// [15:0] OUTER_UNCRT_PKT_NUM_PCKTPRD_B
		if (!tbva_bypass && tbva_layer == 1) {
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x17*2+1, &data);
			packet_err_B = data << 8;
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x17*2, &data);
			packet_err_B = packet_err_B | data;
			*packet_err = packet_err_B;
		} else {
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x21*2+1, &data);
			packet_err_B = data << 8;
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x21*2, &data);
			packet_err_B = packet_err_B | data;
			*packet_err = packet_err_B;
		}
		break;
	case E_ISDBT_Layer_C:
		// [15:0] OUTER_UNCRT_PKT_NUM_PCKTPRD_C
		if (!tbva_bypass && tbva_layer == 2) {
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x17*2+1, &data);
			packet_err_C = data << 8;
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x17*2, &data);
			packet_err_C = packet_err_C | data;
			*packet_err = packet_err_C;
		} else {
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x42*2+1, &data);
			packet_err_C = data << 8;
			ret &=
			_mbx_readreg(ISDBT_OUTER_REG_BASE + 0x42*2, &data);
			packet_err_C = packet_err_C | data;
			*packet_err = packet_err_C;
		}
		break;
	default:
		*packet_err = 0xFFFF;
		break;
	}
#else
	switch (elayer_index) {
	case E_ISDBT_Layer_A:
		// [15:0] OUTER_UNCRT_PKT_NUM_PCKTPRD_A
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x08*2+1, &data);
		packet_err_A = data << 8;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x08*2, &data);
		packet_err_A = packet_err_A | data;
		*packet_err = packet_err_A;
		break;
	case E_ISDBT_Layer_B:
		// [15:0] OUTER_UNCRT_PKT_NUM_PCKTPRD_B
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x21*2+1, &data);
		packet_err_B = data << 8;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x21*2, &data);
		packet_err_B = packet_err_B | data;
		*packet_err = packet_err_B;
		break;
	case E_ISDBT_Layer_C:
		// [15:0] OUTER_UNCRT_PKT_NUM_PCKTPRD_C
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x42*2+1, &data);
		packet_err_C = data << 8;
		ret &= _mbx_readreg(ISDBT_OUTER_REG_BASE + 0x42*2, &data);
		packet_err_C = packet_err_C | data;
		*packet_err = packet_err_C;
		break;
	default:
		*packet_err = 0xFFFF;
		break;
	}
#endif
	// Unfreeze Packet error
	ret &= _mbx_writereg(ISDBT_OUTER_REG_BASE+0x01*2+1, frz_data);

	return ret;
}

static u8 _hal_intern_isdbt_get_reg(u16 addr, u8 *pdata)
{
	return _mbx_readreg(addr, pdata);
}

static u8 _hal_intern_isdbt_set_reg(u16 addr, u8 data)
{
	return _mbx_writereg(addr, data);
}

//------------------------------------------------------------------
//  Global Functions
//------------------------------------------------------------------
u8 hal_intern_isdbt_ioctl_cmd(struct dmd_isdbt_ioctl_data *pdata,
enum dmd_isdbt_hal_command ecmd, void *pargs)
{
	u8 result = TRUE;

	id   = pdata->id;
	pres = pdata->pres;

	sys  = pdata->sys;

	switch (ecmd) {

	case DMD_ISDBT_HAL_CMD_Exit:
		result = _hal_intern_isdbt_exit();
		break;
	case DMD_ISDBT_HAL_CMD_InitClk:
		_hal_intern_isdbt_initclk();
		break;
	case DMD_ISDBT_HAL_CMD_Download:
		result = _hal_intern_isdbt_download();
		break;
	case DMD_ISDBT_HAL_CMD_FWVERSION:
		_hal_intern_isdbt_fw_version();
		break;
	case DMD_ISDBT_HAL_CMD_SoftReset:
		result = _hal_intern_isdbt_softreset();
		break;
	case DMD_ISDBT_HAL_CMD_SetACICoef:
		result = _hal_intern_isdbt_set_aci_coeff();
		break;
	case DMD_ISDBT_HAL_CMD_SetISDBTMode:
		result = _hal_intern_isdbt_set_isdbtmode();
		break;
	case DMD_ISDBT_HAL_CMD_SetModeClean:
		result = _hal_intern_isdbt_set_modeclean();
		break;
	case DMD_ISDBT_HAL_CMD_Active:
		break;
	case DMD_ISDBT_HAL_CMD_Check_EWBS_Lock:
		result = _hal_intern_isdbt_check_ewbs_lock();
		break;
	case DMD_ISDBT_HAL_CMD_Check_FEC_Lock:
		result = _hal_intern_isdbt_check_fec_lock();
		break;
	case DMD_ISDBT_HAL_CMD_Check_FSA_TRACK_Lock:
		result = _hal_intern_isdbt_check_fsa_track_lock();
		break;
	case DMD_ISDBT_HAL_CMD_Check_PSYNC_Lock:
		result = _hal_intern_isdbt_check_psync_lock();
		break;
	case DMD_ISDBT_HAL_CMD_Check_ICFO_CH_EXIST_Lock:
		result = _hal_intern_isdbt_check_icfo_ch_exist_lock();
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalCodeRate:
		result = _hal_intern_isdbt_get_signal_code_rate(
		(*((struct dmd_isdbt_get_code_rate *)pargs)).eisdbtlayer,
		&((*((struct dmd_isdbt_get_code_rate *)pargs)).ecoderate));
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalGuardInterval:
		result = _hal_intern_isdbt_get_signalguardinterval(
		(enum en_isdbt_guard_interval *)pargs);
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalTimeInterleaving:
		result = _hal_intern_isdbt_get_signaltimeinterleaving(
		(*((struct dmd_isdbt_get_time_interleaving *)pargs)
		).eisdbtlayer,
		&((*((struct dmd_isdbt_get_time_interleaving *)pargs)
		).etimeinterleaving));
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalFFTValue:
		result =
		_hal_intern_isdbt_get_signalfftvalue(
		(enum en_isdbt_fft_val *)pargs);
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalModulation:
		result = _hal_intern_isdbt_get_signalmodulation(
		(*((struct dmd_isdbt_get_modulation *)pargs)).eisdbtlayer,
		&((*((struct dmd_isdbt_get_modulation *)pargs)
		).econstellation));
		break;
	case DMD_ISDBT_HAL_CMD_ReadIFAGC:
		*((u16 *)pargs) = _hal_intern_isdbt_read_ifagc();
		break;
	case DMD_ISDBT_HAL_CMD_GetFreqOffset:
		#ifdef UTPA2
		result = _hal_intern_isdbt_get_freqoffset(
		&((*((struct dmd_isdbt_cfo_data *)pargs)).fft_mode),
		&((*((struct dmd_isdbt_cfo_data *)pargs)).tdcfo_regvalue),
		&((*((struct dmd_isdbt_cfo_data *)pargs)).fdcfo_regvalue),
		&((*((struct dmd_isdbt_cfo_data *)pargs)).icfo_regvalue));
		#else
		result = _hal_intern_isdbt_get_freqoffset((float *)pargs);
		#endif
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalQuality:
	case DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerA:
		*((u16 *)pargs) =
		_hal_intern_isdbt_get_signalqualityoflayerA();
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerB:
		*((u16 *)pargs) =
		_hal_intern_isdbt_get_signalqualityoflayerB();
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerC:
		*((u16 *)pargs) =
		_hal_intern_isdbt_get_signalqualityoflayerC();
		break;
	case DMD_ISDBT_HAL_CMD_GetSignalQualityCombine:
		*((u16 *)pargs) =
		_hal_intern_isdbt_get_signalqualityoflayer_combine();
		break;
	case DMD_ISDBT_HAL_CMD_GetSNR:
		#ifdef UTPA2
		result = _hal_intern_isdbt_get_snr(
		&((*((struct dmd_isdbt_snr_data *)pargs)).reg_snr),
		&((*((struct dmd_isdbt_snr_data *)pargs)).reg_snr_obs_num));
		#else
		result = _hal_intern_isdbt_get_snr((float *)pargs);
		#endif
		break;
	case DMD_ISDBT_HAL_CMD_GetPreViterbiBer:
		#ifdef UTPA2
		result = _hal_intern_isdbt_get_previterbi_ber(
		(*((struct dmd_isdbt_get_ber_value *)pargs)).eisdbtlayer,
		&((*((struct dmd_isdbt_get_ber_value *)pargs)).bervalue),
		&((*((struct dmd_isdbt_get_ber_value *)pargs)).berperiod));
		#else
		result = _hal_intern_isdbt_get_previterbi_ber(
		(*((struct dmd_isdbt_get_ber_value *)pargs)).eIsdbtLayer,
		&((*((struct dmd_isdbt_get_ber_value *)pargs)).fbervalue));
		#endif
		break;
	case DMD_ISDBT_HAL_CMD_GetPostViterbiBer:
		#ifdef UTPA2
		result = _hal_intern_isdbt_get_postviterbi_ber(
		(*((struct dmd_isdbt_get_ber_value *)pargs)).eisdbtlayer,
		&((*((struct dmd_isdbt_get_ber_value *)pargs)).bervalue),
		&((*((struct dmd_isdbt_get_ber_value *)pargs)).berperiod));
		#else
		result = _hal_intern_isdbt_get_postviterbi_ber(
		(*((struct dmd_isdbt_get_ber_value *)pargs)).eisdbtlayer,
		&((*((struct dmd_isdbt_get_ber_value *)pargs)).fbervalue));
		#endif
		break;
	case DMD_ISDBT_HAL_CMD_Read_PKT_ERR:
		result = _hal_intern_isdbt_read_pkt_err(
		(*((struct dmd_isdbt_get_pkt_err *)pargs)).eisdbtlayer,
		&((*((struct dmd_isdbt_get_pkt_err *)pargs)).packeterr));
		break;
	case DMD_ISDBT_HAL_CMD_TS_INTERFACE_CONFIG:
		break;
	case DMD_ISDBT_HAL_CMD_IIC_Bypass_Mode:
		break;
	case DMD_ISDBT_HAL_CMD_SSPI_TO_GPIO:
		break;
	case DMD_ISDBT_HAL_CMD_GPIO_GET_LEVEL:
		break;
	case DMD_ISDBT_HAL_CMD_GPIO_SET_LEVEL:
		break;
	case DMD_ISDBT_HAL_CMD_GPIO_OUT_ENABLE:
		break;
	case DMD_ISDBT_HAL_CMD_GET_REG:
		result = _hal_intern_isdbt_get_reg(
		(*((struct dmd_isdbt_reg_data *)pargs)).isdbt_addr,
		&((*((struct dmd_isdbt_reg_data *)pargs)).isdbt_data));
		break;
	case DMD_ISDBT_HAL_CMD_SET_REG:
		result = _hal_intern_isdbt_set_reg(
		(*((struct dmd_isdbt_reg_data *)pargs)).isdbt_addr,
		(*((struct dmd_isdbt_reg_data *)pargs)).isdbt_data);
		break;
	default:
		break;
	}
	return result;
}

u8 mdrv_dmd_isdbt_initial_hal_interface(void)
{
	return TRUE;
}

#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG)
#if defined(CONFIG_MSTAR_CHIP)
EXPORT_SYMBOL(hal_intern_isdbt_ioctl_cmd);
#endif
#endif
