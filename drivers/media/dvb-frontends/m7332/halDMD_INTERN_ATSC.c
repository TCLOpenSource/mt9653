// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//-----------------------------------------------------------------------
//	Include	Files
//-----------------------------------------------------------------------

#include "drvDMD_ATSC.h"

#if	!defined MTK_PROJECT
#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/kernel.h>
#include <linux/delay.h>
#else
#include <stdio.h>
#include <math.h>
#endif
#else	//#if	!defined MTK_PROJECT
#include "tz_if.h"
#endif //#if !defined	MTK_PROJECT

#ifndef DVB_STI	// VT: Temp	define for DVB_Frontend
 #define	DVB_STI	 1
#endif
#include <linux/delay.h>
#include <asm/io.h>

#if	DMD_ATSC_3PARTY_EN
// Please	add	header files needed	by 3 perty platform
#endif

//-----------------------------------------------------------------------
//	Driver Compiler	Options
//-----------------------------------------------------------------------

#ifndef	BIN_RELEASE
#define	BIN_RELEASE					0
#endif

#if !DMD_ATSC_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI

#define	DMD_ATSC_CHIP_T3_T10		0x01
#define	DMD_ATSC_CHIP_T7			0x02
#define	DMD_ATSC_CHIP_T8_T9			0x03
#define	DMD_ATSC_CHIP_A1			0x04
#define	DMD_ATSC_CHIP_A3			0x05
#define	DMD_ATSC_CHIP_A5			0x06
#define	DMD_ATSC_CHIP_A7			0x07
#define	DMD_ATSC_CHIP_A7P			0x08
#define	DMD_ATSC_CHIP_AGATE			0x09
#define	DMD_ATSC_CHIP_EDISON		0x0A
#define	DMD_ATSC_CHIP_EINSTEIN		0x0B
#define	DMD_ATSC_CHIP_EMERALD		0x0C
#define	DMD_ATSC_CHIP_EIFFEL		0x0D
#define	DMD_ATSC_CHIP_EDEN			0x0E
#define	DMD_ATSC_CHIP_EINSTEIN3		0x0F
#define	DMD_ATSC_CHIP_MONACO		0x10
#define	DMD_ATSC_CHIP_MIAMI			0x11
#define	DMD_ATSC_CHIP_MUJI			0x12
#define	DMD_ATSC_CHIP_MUNICH		0x13
#define	DMD_ATSC_CHIP_MAYA			0x14
#define	DMD_ATSC_CHIP_MANHATTAN		0x15
#define	DMD_ATSC_CHIP_WHISKY		0x16
#define	DMD_ATSC_CHIP_MASERATI		0x17 //start to	support	T2_MERGED_FEND
#define	DMD_ATSC_CHIP_MACAN			0x18
#define	DMD_ATSC_CHIP_MAXIM			0x19
#define	DMD_ATSC_CHIP_MUSTANG		0x1A //start to	support	bank remapping
#define	DMD_ATSC_CHIP_M5621			0x1B
#define	DMD_ATSC_CHIP_M7621			0x1C
#define	DMD_ATSC_CHIP_M5321			0x1D
#define	DMD_ATSC_CHIP_M7622			0x1E
#define	DMD_ATSC_CHIP_M7322			0x1F
#define	DMD_ATSC_CHIP_M3822			0x20
#define	DMD_ATSC_CHIP_MT5895		0x21
#define	DMD_ATSC_CHIP_M7632			0x22
#define	DMD_ATSC_CHIP_M7332			0x23
#define	DMD_ATSC_CHIP_M7642			0x24
#define	DMD_ATSC_CHIP_MT5862		0x25
//UTOF	start	from 0x80
#define	DMD_ATSC_CHIP_K3			0x80
#define	DMD_ATSC_CHIP_KELTIC		0x81
#define	DMD_ATSC_CHIP_KERES			0x82
#define	DMD_ATSC_CHIP_KIRIN			0x83
#define	DMD_ATSC_CHIP_KAYLA			0x84
#define	DMD_ATSC_CHIP_KENTUCKY		0x85
#define	DMD_ATSC_CHIP_K1C			0x86

#if	defined(CHIP_A1)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_A1
#elif	defined(CHIP_A3)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_A3
#elif	defined(CHIP_A5)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_A5
#elif	defined(CHIP_A7)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_A7
#elif	defined(CHIP_AMETHYST)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_A7P
#elif	defined(CHIP_AGATE)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_AGATE
#elif	defined(CHIP_EDISON)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_EDISON
#elif	defined(CHIP_EINSTEIN)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_EINSTEIN
#elif	defined(CHIP_EINSTEIN3)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_EINSTEIN3
#elif	defined(CHIP_MONACO)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MONACO
#elif	defined(CHIP_EMERALD)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_EMERALD
#elif	defined(CHIP_EIFFEL)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_EIFFEL
#elif	defined(CHIP_KAISER)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_K3
#elif	defined(CHIP_KELTIC)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_KELTIC
#elif	defined(CHIP_EDEN)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_EDEN
#elif	defined(CHIP_MIAMI)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MIAMI
#elif	defined(CHIP_KERES)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_KERES
#elif	defined(CHIP_MUJI)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MUJI
#elif	defined(CHIP_MUNICH)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MUNICH
#elif	defined(CHIP_KIRIN)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_KIRIN
#elif	defined(CHIP_MAYA)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MAYA
#elif	defined(CHIP_MANHATTAN)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MANHATTAN
#elif	defined(CHIP_WHISKY)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_WHISKY
#elif	defined(CHIP_MASERATI)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MASERATI
#elif	defined(CHIP_MACAN)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MACAN
#elif	defined(CHIP_MUSTANG)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MUSTANG
#elif	defined(CHIP_MAXIM)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MAXIM
#elif	defined(CHIP_KAYLA)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_KAYLA
#elif	defined(CHIP_K5TN)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_KENTUCKY
#elif	defined(CHIP_K1C)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_K1C
#elif	defined(CHIP_M5621)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M5621
#elif	defined(CHIP_M7621)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M7621
#elif	defined(CHIP_M5321)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M5321
#elif	defined(CHIP_M7622)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M7622
#elif	defined(CHIP_M7322)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M7322
#elif	defined(CHIP_M3822)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M3822
#elif	defined(CC_MT5895)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_MT5895
#elif	defined(CHIP_M7632)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M7632
#elif	defined(CHIP_M7332)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M7332
#elif	defined(CHIP_M7642)
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M7642
#else
 #define DMD_ATSC_CHIP_VERSION		DMD_ATSC_CHIP_M7332
#endif

#endif
//#if !DMD_ATSC_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

//----------------------------------------------------------
//	Local	Defines
//----------------------------------------------------------

#if	defined	MTK_PROJECT
#define	_riu_read_byte(addr) ((addr)&0x01 ? \
((u8)((IO_READ32((addr&0xFFFFFF00), (addr&0x000000FE)<<1))>>8)) :\
((u8)((IO_READ32((addr&0xFFFFFF00), (addr&0x000000FE)<<1))&0x000000FF)))
#define	_riu_write_byte(addr,	val)	do { if	((addr)&0x01)
	IO_WRITE32((addr&0xFFFFFF00), (addr&0x000000FE)<<1,
	(IO_READ32((addr&0xFFFFFF00),
	(addr&0x000000FE)<<1)&0xFFFF00FF)|((((u32)val)<<8)));
	else
		IO_WRITE32((addr&0xFFFFFF00), (addr&0x000000FE)<<1,
	(IO_READ32((addr&0xFFFFFF00),
	(addr&0x000000FE)<<1)&0xFFFFFF00)|((u32)val)); } while (0)
#elif	DVB_STI
#define	_riu_read_byte(addr)  (\
readb(pres->sdmd_atsc_pridata.virtdmdbaseaddr + (addr)))

#define	_riu_write_byte(addr, val) (\
writeb(val, pres->sdmd_atsc_pridata.virtdmdbaseaddr + (addr)))
#elif	DMD_ATSC_3PARTY_EN
// Please add riu read&write define according to 3 perty platform
#define	_riu_read_byte(addr)
#define	_riu_write_byte(addr,	val)
#else
#define _riu_read_byte(addr)  (\
	READ_BYTE(pres->sdmd_atsc_pridata.virtdmdbaseaddr + (addr)))

#define _riu_write_byte(addr, val) (\
	WRITE_BYTE(pres->sdmd_atsc_pridata.virtdmdbaseaddr + (addr), val))
#endif

#define	USE_DEFAULT_CHECK_TIME		1

#define	HAL_INTERN_ATSC_DBINFO(y)	//y

#ifndef	MB_REG_BASE
#ifdef MTK_PROJECT
 #define MB_REG_BASE				 (DEMOD_BASE	+	0x5E00)
 #else
 #define MB_REG_BASE				 0x112600
 #endif
#endif
#ifndef	MB_REG_BASE_DMD1
 #define MB_REG_BASE_DMD1			 0x112400
#endif
#ifndef	DMD_MCU_BASE
 #ifdef	MTK_PROJECT
 #define DMD_MCU_BASE				 (DEMOD_BASE	+	0x27D00)
 #else
 #define DMD_MCU_BASE				 0x103480
 #endif
#endif

#define	INTERN_ATSC_VSB_TRAIN_SNR_LIMIT	0x05//0xBE//14.5dB
#define	INTERN_ATSC_FEC_ENABLE			0x1F

#define	VSB_ATSC			 0x04
#define	QAM256_ATSC		 0x02

#define	QAM16_J83ABC		 0x00
#define	QAM32_J83ABC		 0x01
#define	QAM64_J83ABC		 0x02
#define	QAM128_J83ABC		 0x03
#define	QAM256_J83ABC		 0x04

#if !DMD_ATSC_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI

#if	DMD_ATSC_CHIP_VERSION	>= DMD_ATSC_CHIP_K3
#define	J83ABC_ENABLE			 1
#else
#define	J83ABC_ENABLE			 0
#endif

#if	J83ABC_ENABLE

 #define USE_PSRAM_32KB			 0

 #if (DMD_ATSC_CHIP_VERSION	== DMD_ATSC_CHIP_K3)
 #define IS_MULTI_INT_DMD		 1
 #else
 #define IS_MULTI_INT_DMD		 0
 #endif

 #if (DMD_ATSC_CHIP_VERSION >= DMD_ATSC_CHIP_KENTUCKY)
	&&(DMD_ATSC_CHIP_VERSION != DMD_ATSC_CHIP_K1C)
 #define USE_T2_MERGED_FEND		 1
 #define USE_BANK_REMAP			 1
 #elif DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_KAYLA
 #define USE_T2_MERGED_FEND		 1
 #define USE_BANK_REMAP			 0
 #else
 #define USE_T2_MERGED_FEND		 0
 #define USE_BANK_REMAP			 0
 #endif

 #define REMAP_RST_BY_DRV		 0
 #define POW_SAVE_BY_DRV		 0
 #define IS_UTOF_FW				 1
 #define FIX_TS_NON_SYNC_ISSUE	 0
 #define FS_RATE                 24000 // KHz
#else	// #if J83ABC_ENABLE

 #if DMD_ATSC_CHIP_VERSION >= DMD_ATSC_CHIP_A1
 #define MIRANDA_ENABLE          1
 #else
 #define MIRANDA_ENABLE          0
 #endif

 #if DMD_ATSC_CHIP_VERSION > DMD_ATSC_CHIP_WHISKY
 #define USE_PSRAM_32KB			 1
 #else
 #define USE_PSRAM_32KB			 0
 #endif

 #define IS_MULTI_INT_DMD		 0

 #if DMD_ATSC_CHIP_VERSION >=	DMD_ATSC_CHIP_MUSTANG
 #define USE_T2_MERGED_FEND		 1
 #define USE_BANK_REMAP			 1
 #elif DMD_ATSC_CHIP_VERSION >=	DMD_ATSC_CHIP_MASERATI
 #define USE_T2_MERGED_FEND		 1
 #define USE_BANK_REMAP			 0
 #else
 #define USE_T2_MERGED_FEND		 0
 #define USE_BANK_REMAP			 0
 #endif

 #if DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MUSTANG
 #define REMAP_RST_BY_DRV		 1
 #else
 #define REMAP_RST_BY_DRV		 0
 #endif

 #if ((DMD_ATSC_CHIP_VERSION >= DMD_ATSC_CHIP_M5621)\
	&& (DMD_ATSC_CHIP_VERSION != DMD_ATSC_CHIP_M7621)\
	&& (DMD_ATSC_CHIP_VERSION != DMD_ATSC_CHIP_M5321))
 #define POW_SAVE_BY_DRV		 1
 #else
 #define POW_SAVE_BY_DRV		 0
 #endif

 #define IS_UTOF_FW				 0

 #if USE_T2_MERGED_FEND && (DMD_ATSC_CHIP_VERSION <= DMD_ATSC_CHIP_M5621)
 #define FIX_TS_NON_SYNC_ISSUE	 1
 #else
 #define FIX_TS_NON_SYNC_ISSUE	 0
 #endif

 #if DMD_ATSC_CHIP_VERSION >=	DMD_ATSC_CHIP_M5621
 #define USE_NEW_FEC_LOCK		 1
 #else
 #define USE_NEW_FEC_LOCK		 0
 #endif

 #if ((DMD_ATSC_CHIP_VERSION >= DMD_ATSC_CHIP_M5621)\
	&& (DMD_ATSC_CHIP_VERSION != DMD_ATSC_CHIP_MT5895))
 #define SUPPORT_NO_CH_DET		 1
 #else
 #define SUPPORT_NO_CH_DET		 0
 #endif

 #if DMD_ATSC_CHIP_VERSION >=	DMD_ATSC_CHIP_M7632
 #define GLO_RST_IN_DOWNLOAD	 1
 #else
 #define GLO_RST_IN_DOWNLOAD	 0
 #endif

 #if DMD_ATSC_CHIP_VERSION >= DMD_ATSC_CHIP_EINSTEIN
 #define FS_RATE                 25410 //KHz
 #elif DMD_ATSC_CHIP_VERSION >= DMD_ATSC_CHIP_A1
 #define FS_RATE                 24000 //KHz
 #else
 #define FS_RATE                 24857 //KHz
 #endif
#endif //	#if	J83ABC_ENABLE

#else	// #if !DMD_ATSC_3PARTY_EN ||	defined	MTK_PROJECT	|| BIN_RELEASE

 //	Please set the following values	according	to 3 perty platform
 #define J83ABC_ENABLE			 0
 #define USE_PSRAM_32KB			 0
 #define IS_MULTI_INT_DMD		 0
 #define USE_T2_MERGED_FEND		 0
 #define REMAP_RST_BY_DRV		 0
 #define USE_BANK_REMAP			 0
 #define POW_SAVE_BY_DRV		 0
 #define IS_UTOF_FW				 0
 #define FIX_TS_NON_SYNC_ISSUE	 0
 #define USE_NEW_FEC_LOCK		 0
 #define SUPPORT_NO_CH_DET		 0
 #define GLO_RST_IN_DOWNLOAD	 0

#endif
// #if !DMD_ATSC_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE

#if MIRANDA_ENABLE
	#define MIX_RATE_BIT                     27
#else
	#define MIX_RATE_BIT                     20
#endif

#if	J83ABC_ENABLE	|| IS_UTOF_FW
 #define INTERN_ATSC_OUTER_STATE			0xF0
#else
 #define INTERN_ATSC_OUTER_STATE			0x80
#endif

//------------------------------------------------------------
//	Local	Variables
//------------------------------------------------------------

static u8 id;

struct DMD_ATSC_RESDATA	*pres;

struct DMD_ATSC_SYS_PTR	sys;

#if	!DMD_ATSC_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI
const u8 INTERN_ATSC_TABLE[] = {
	#include "DMD_INTERN_ATSC.dat"
};
#else
// Please	set	fw dat file	used by	3	perty	platform
const	u8	INTERN_ATSC_TABLE[]	=	{
	#include "DMD_INTERN_ATSC.dat"
};
#endif

static u16 lib_size = sizeof(INTERN_ATSC_TABLE);

#if	(!J83ABC_ENABLE)
static u8 demod_flow_register[21] = {
	0x52, 0x72, 0x52, 0x72, 0x5C, 0x5C, 0xA3, 0xEC, 0xEA,
	0x05, 0x74, 0x1E, 0x38, 0x3A, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};
#endif //	#if	(!J83ABC_ENABLE)

#ifndef	UTPA2
static const float _LOGAPPROXTABLEX[80]	= {
	1.00, 1.30, 1.69, 2.20, 2.86, 3.71, 4.83, 6.27, 8.16, 10.60,
	13.79, 17.92, 23.30, 30.29, 39.37, 51.19, 66.54, 86.50, 112.46, 146.19,
	190.05, 247.06, 321.18, 417.54, 542.80, 705.64,	917.33, 1192.53,
	1550.29, 2015.38, 2620.00, 3405.99, 4427.79, 5756.13, 7482.97,
	9727.86, 12646.22, 16440.08, 21372.11, 27783.74,
	36118.86, 46954.52, 61040.88, 79353.15, 103159.09,
	134106.82, 174338.86, 226640.52, 294632.68, 83022.48,
	497929.22, 647307.99, 841500.39, 1093950.50, 1422135.65,
	1848776.35, 2403409.25, 3124432.03, 4061761.64, 5280290.13,
	6864377.17, 8923690.32, 11600797.42, 15081036.65, 19605347.64,
	25486951.94, 33133037.52, 43072948.77, 55994833.40, 72793283.42,
	94631268.45, 123020648.99, 159926843.68, 207904896.79, 270276365.82,
	351359275.57, 456767058.24, 593797175.72, 771936328.43, 1003517226.96
};

static const float _LogApproxTableY[80]	= {
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
static const u32	_LOGAPPROXTABLEX[58] = {
1, 2, 3, 4, 6, 8, 10, 13, 17, 23, //10
30, 39, 51, 66, 86, 112, 146, 190, 247, 321, //10
417, 542, 705, 917, 1192, 1550, 2015, 2620, 3405, 4427, //10
5756, 7482, 9727, 12646, 16440, 21372, 27783, 36118, 46954, 61040, //10
79353, 103159, 134106, 174338, 226640, 294632, 383022, 497929, 647307, 841500,
1093950, 1422135, 1848776, 2403409, 3124432, 4061761, 5280290, 6160384 //8
};

static const u16	_LogApproxTableY[58] = {
	 0,  30, 47,  60, 77, 90, 100, 111, 123, 136,
	 147, 159, 170, 181, 193, 204, 216, 227, 239, 250,
	 262, 273, 284, 296, 307, 319, 330, 341, 353, 364,
	 376, 387, 398, 410, 421, 432, 444, 455, 467, 478,
	 489, 501, 512, 524, 535, 546, 558, 569, 581, 592,
	 603, 615, 626, 638, 649, 660, 672, 678
};
#endif

//-------------------------------------------------------------------
//	Global Variables
//-------------------------------------------------------------------


//-------------------------------------------------------------------
//	Local	Functions
//-------------------------------------------------------------------
#ifndef	UTPA2
#ifndef	MSOS_TYPE_LINUX
static float Log10Approx(float flt_x)
{
	u8	 indx	=	0;

	do {
		if (flt_x	<	_LOGAPPROXTABLEX[indx])
			break;
		indx++;
	} while (indx < 79);	//stop at	indx = 80

	return _LogApproxTableY[indx];
}
#endif
#else	// #ifndef UTPA2
static u16	Log10Approx(u32 flt_x)
{
	u8	indx = 0;

	do {
		if (flt_x	<	_LOGAPPROXTABLEX[indx])
			break;
		indx++;
	} while (indx < 57);	//stop at	indx = 58

	return _LogApproxTableY[indx];
}
#endif //	#ifndef	UTPA2

static u8 _hal_dmd_riu_readbyte(u32	addr)
{
	#if	(!DVB_STI) &&	(!DMD_ATSC_UTOPIA_EN &&	!DMD_ATSC_UTOPIA2_EN)
	return _riu_read_byte(addr);
	#elif	(DVB_STI)
	return _riu_read_byte(((addr) <<	1) - ((addr)	&	1));
	#else
	return _riu_read_byte(((addr) <<	1) - ((addr)	&	1));
	#endif
}

static void	_hal_dmd_riu_writebyte(u32 addr, u8 value)
{
	#if	(!DVB_STI) &&	(!DMD_ATSC_UTOPIA_EN &&	!DMD_ATSC_UTOPIA2_EN)
	_riu_write_byte(addr,	value);
	#elif	(DVB_STI)
	_riu_write_byte(((addr) << 1) - ((addr) & 1), value);
	#else
	_riu_write_byte(((addr) << 1) - ((addr) & 1), value);
	#endif
}

static void _hal_dmd_riu_writebyteMask
(u32 addr, u8 value, u8 mask)
{
	#if	(!DVB_STI) && (!DMD_ATSC_UTOPIA_EN && !DMD_ATSC_UTOPIA2_EN)
	_riu_write_byte(addr,
	(_riu_read_byte(addr) & ~(mask)) | ((value) & (mask)));
	#else
	_riu_write_byte((((addr) << 1) - ((addr) & 1)),
	(_riu_read_byte((((addr) << 1) - ((addr) & 1))) & ~(mask))
	| ((value) & (mask)));
	#endif
}

static u8/*bool*/ _mbx_writereg(u16	addr_16, u8 data_8)
{
	u8 checkcount;
	u8 checkflag = 0xFF;
	u32 mb_reg_base_tmp = MB_REG_BASE;

	if (id ==	0)
		mb_reg_base_tmp = MB_REG_BASE;
	else if	(id	== 1)
		mb_reg_base_tmp = MB_REG_BASE_DMD1;

	_hal_dmd_riu_writebyte(mb_reg_base_tmp	+	0x00,	(addr_16&0xff));
	_hal_dmd_riu_writebyte(mb_reg_base_tmp	+	0x01,	(addr_16>>8));
	_hal_dmd_riu_writebyte(mb_reg_base_tmp	+	0x10,	data_8);
	_hal_dmd_riu_writebyte(mb_reg_base_tmp	+	0x1E,	0x01);

	//	assert interrupt to	VD MCU51
	_hal_dmd_riu_writebyte(DMD_MCU_BASE + 0x03,
	_hal_dmd_riu_readbyte(DMD_MCU_BASE + 0x03)|0x02);

	//	de-assert	interrupt	to VD	MCU51
	_hal_dmd_riu_writebyte(DMD_MCU_BASE + 0x03,
	_hal_dmd_riu_readbyte(DMD_MCU_BASE + 0x03)&(~0x02));

	for	(checkcount = 0; checkcount < 10; checkcount++) {
		checkflag = _hal_dmd_riu_readbyte(mb_reg_base_tmp + 0x1E);
		if ((checkflag&0x01) == 0)
			break;

		sys.delayms(1);
	}

	if (checkflag&0x01) {
		PRINT("ERROR: ATSC INTERN DEMOD MBX WRITE TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

static u8/*bool*/ _mbx_readreg(u16 addr_16,	u8 *data_8)
{
	u8 checkcount;
	u8 checkflag = 0xFF;
	u32 mb_reg_base_tmp = MB_REG_BASE;

	if (id ==	0)
		mb_reg_base_tmp = MB_REG_BASE;
	else if	(id	== 1)
		mb_reg_base_tmp = MB_REG_BASE_DMD1;

	_hal_dmd_riu_writebyte(mb_reg_base_tmp	+	0x00,	(addr_16&0xff));
	_hal_dmd_riu_writebyte(mb_reg_base_tmp	+	0x01,	(addr_16>>8));
	_hal_dmd_riu_writebyte(mb_reg_base_tmp	+	0x1E,	0x02);

	//	assert interrupt to	VD MCU51
	_hal_dmd_riu_writebyte(DMD_MCU_BASE + 0x03,
	_hal_dmd_riu_readbyte(DMD_MCU_BASE + 0x03)|0x02);

	//	de-assert	interrupt	to VD	MCU51
	_hal_dmd_riu_writebyte(DMD_MCU_BASE + 0x03,
	_hal_dmd_riu_readbyte(DMD_MCU_BASE + 0x03)&(~0x02));

	for	(checkcount = 0; checkcount < 10; checkcount++) {
		checkflag = _hal_dmd_riu_readbyte(mb_reg_base_tmp + 0x1E);
		if ((checkflag&0x02) == 0) {
			*data_8 = _hal_dmd_riu_readbyte(mb_reg_base_tmp + 0x10);
			break;
		}

		sys.delayms(1);
	}

	if (checkflag&0x02) {
		PRINT("ERROR: ATSC INTERN DEMOD MBX READ TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

#if	IS_MULTI_INT_DMD
static u8/*bool*/ _sel_dmd(void)
{
	u8	data_8 = 0;

	data_8 = _hal_dmd_riu_readbyte(0x101e3c);

	if (id ==	0) //select	DMD0
		data_8 &=	(~0x10);
	else if	(id	== 1)	//sel	DMD1
		data_8 |=	0x10;

	_hal_dmd_riu_writebyte(0x101e3c, data_8);

	return TRUE;
}
#endif

#if	!J83ABC_ENABLE
static void	_inittable(void)
{
	if (pres->sdmd_atsc_initdata.btunergaininvert)
		demod_flow_register[13] = 1;
	else
		demod_flow_register[13] = 0;

	if (pres->sdmd_atsc_initdata.biqswap)
		demod_flow_register[14]	=	1;
	else
		demod_flow_register[14] = 0;

	demod_flow_register[15] = pres->sdmd_atsc_initdata.if_khz&0xFF;
	demod_flow_register[16] = (pres->sdmd_atsc_initdata.if_khz)>>8;

	PRINT("\n#### IF_KHz = [%d]\n", pres->sdmd_atsc_initdata.if_khz);
	PRINT("\n#### IQ_SWAP = [%d]\n", pres->sdmd_atsc_initdata.biqswap);
	PRINT("\n#### Tuner Gain Invert = [%d]\n",
	pres->sdmd_atsc_initdata.btunergaininvert);
}
#endif

#if !DMD_ATSC_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE || DVB_STI

#if	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_A1)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_A1--------------\n"));

	//Set	register at	CLKGEN1
	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	// Denny: change 0x10!! 108M
	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103314, 0x01);	// Disable ADC clock

	// set parallet	ts clock
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	// enable	atsc,	DVBTC	ts clock
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);

	// enable	dvbc adc clock
	_hal_dmd_riu_writebyte(0x103315, 0x00);

	// enable	vif	DAC	clock
	_hal_dmd_riu_writebyte(0x10331b, 0x00);
	_hal_dmd_riu_writebyte(0x10331a, 0x00);

	// Set register	at CLKGEN_DMD
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);

	// enable	clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	// enable	dvbt inner clock
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);

	// enable	dvbt inner clock
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);

	// enable	dvbt inner clock
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);

	// enable	dvbc outer clock
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);

	// enable	dvbc inner-c clock
	_hal_dmd_riu_writebyte(0x111f15, 0x00);
	_hal_dmd_riu_writebyte(0x111f14, 0x00);

	// enable	dvbc eq	clock
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);

	// enable	vif	clock
	_hal_dmd_riu_writebyte(0x111f1d, 0x00);
	_hal_dmd_riu_writebyte(0x111f1c, 0x00);

	// For ADC DMA Dump
	_hal_dmd_riu_writebyte(0x111f21, 0x00);
	_hal_dmd_riu_writebyte(0x111f20, 0x00);

	// select	clock
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);

	sys.delayms(1);

	_hal_dmd_riu_writebyte(0x103314, 0x00);	// enable	ADC	clock

	//	Turn TSP
	_hal_dmd_riu_writebyte(0x100b55, 0x00);
	_hal_dmd_riu_writebyte(0x100b54, 0x00);

	// set the ts0_clk from	demod
	_hal_dmd_riu_writebyte(0x100b51, 0x00);
	_hal_dmd_riu_writebyte(0x100b50, 0x0C);
	_hal_dmd_riu_writebyte(0x101e22, 0x02);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_A7)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_A7--------------\n"));

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103314, 0x01);	// Disable ADC clock

	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x00);
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	_hal_dmd_riu_writebyte(0x111f25, 0x00);
	_hal_dmd_riu_writebyte(0x111f24, 0x00);
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);

	sys.delayms(1);
	_hal_dmd_riu_writebyte(0x103314, 0x00);	// enable	ADC	clock

	_hal_dmd_riu_writebyteMask(0x000e13, 0x00, 0x04);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_K3)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_K3--------------\n"));

	if (pres->sdmd_atsc_initdata.is_dual) {
		_hal_dmd_riu_writebyte(0x101e39, 0x00);
		_hal_dmd_riu_writebyte(0x101e3d, 0x00);

		/****************DMD0****************/

		//set	CLK_DMDMCU as	108M Hz
		_hal_dmd_riu_writebyte(0x10331e, 0x10);

		// set parallet	ts clock
		_hal_dmd_riu_writebyte(0x103301, 0x07);
		_hal_dmd_riu_writebyte(0x103300, 0x11);

		// enable	DVBTC	ts clock
		_hal_dmd_riu_writebyte(0x103309, 0x00);

		// enable	dvbc adc clock
		_hal_dmd_riu_writebyte(0x103315, 0x00);
		_hal_dmd_riu_writebyte(0x103314, 0x00);

		// enable	clk_atsc_adcd_sync
		_hal_dmd_riu_writebyte(0x111f0b, 0x00);
		_hal_dmd_riu_writebyte(0x111f0a, 0x04);

		// enable	dvbt inner clock
		_hal_dmd_riu_writebyte(0x111f0c, 0x00);

		// enable	dvbt outer clock
		_hal_dmd_riu_writebyte(0x111f11, 0x00);

		// enable	dvbc outer clock
		_hal_dmd_riu_writebyte(0x111f13, 0x00);
		_hal_dmd_riu_writebyte(0x111f12, 0x00);

		// enable	dvbc inner-c clock
		_hal_dmd_riu_writebyte(0x111f15, 0x04);

		// enable	dvbc eq	clock
		_hal_dmd_riu_writebyte(0x111f17, 0x00);
		_hal_dmd_riu_writebyte(0x111f16, 0x00);

		// For ADC DMA Dump
		_hal_dmd_riu_writebyte(0x111f22, 0x04);

		//	Turn TSP
		//_hal_dmd_riu_writebyte(0x000e13, 0x01);

		//set	reg_allpad_in
		_hal_dmd_riu_writebyte(0x101ea1, 0x00);
		_hal_dmd_riu_writebyte(0x101e04, 0x02);
		_hal_dmd_riu_writebyte(0x101e76, 0x03);

		/****************DMD1****************/
		_hal_dmd_riu_writebyte(0x10331f, 0x10);

		_hal_dmd_riu_writebyte(0x103321, 0x07);
		_hal_dmd_riu_writebyte(0x103320, 0x11);

		_hal_dmd_riu_writebyte(0x103323, 0x00);
		_hal_dmd_riu_writebyte(0x103322, 0x00);

		_hal_dmd_riu_writebyte(0x11220b, 0x00);
		_hal_dmd_riu_writebyte(0x11220a, 0x04);

		_hal_dmd_riu_writebyte(0x11220c, 0x00);
		_hal_dmd_riu_writebyte(0x112211, 0x00);

		_hal_dmd_riu_writebyte(0x112213, 0x00);
		_hal_dmd_riu_writebyte(0x112212, 0x00);

		_hal_dmd_riu_writebyte(0x112215, 0x04);

		_hal_dmd_riu_writebyte(0x112217, 0x00);
		_hal_dmd_riu_writebyte(0x112216, 0x00);

		_hal_dmd_riu_writebyte(0x112222, 0x04);
		//force	ANA	MISC controlled	by DMD0
		_hal_dmd_riu_writebyte(0x101e39, 0x03);
		_hal_dmd_riu_writebyte(0x101e3d, 0x01);
	} else {
		_hal_dmd_riu_writebyte(0x101e39, 0x00);

		_hal_dmd_riu_writebyte(0x10331f, 0x00);
		_hal_dmd_riu_writebyte(0x10331e, 0x10);

		_hal_dmd_riu_writebyte(0x103301, 0x07);
		_hal_dmd_riu_writebyte(0x103300, 0x11);

		_hal_dmd_riu_writebyte(0x103309, 0x00);

		_hal_dmd_riu_writebyte(0x103315, 0x00);
		_hal_dmd_riu_writebyte(0x103314, 0x00);

		_hal_dmd_riu_writebyte(0x111f0b, 0x00);
		_hal_dmd_riu_writebyte(0x111f0a, 0x00);

		_hal_dmd_riu_writebyte(0x111f0c, 0x00);

		_hal_dmd_riu_writebyte(0x111f11, 0x00);

		_hal_dmd_riu_writebyte(0x111f13, 0x00);
		_hal_dmd_riu_writebyte(0x111f12, 0x00);

		_hal_dmd_riu_writebyte(0x111f15, 0x00);

		_hal_dmd_riu_writebyte(0x111f17, 0x00);
		_hal_dmd_riu_writebyte(0x111f16, 0x00);

		_hal_dmd_riu_writebyte(0x111f22, 0x00);

		_hal_dmd_riu_writebyte(0x101ea1, 0x00);
		_hal_dmd_riu_writebyte(0x101e04, 0x02);
		_hal_dmd_riu_writebyte(0x101e76, 0x03);
		//force	ANA	MISC controlled	by DMD0
		_hal_dmd_riu_writebyte(0x101e39, 0x03);
		_hal_dmd_riu_writebyte(0x101e3d, 0x01);
	}
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_KELTIC)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_KELTIC--------------\n"));

	_hal_dmd_riu_writebyte(0x101e39, 0x00);

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103301, 0x07);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	_hal_dmd_riu_writebyte(0x103309, 0x00);

	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x00);

	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x00);

	_hal_dmd_riu_writebyte(0x111f15, 0x00);

	_hal_dmd_riu_writebyte(0x111f17, 0x00);
	_hal_dmd_riu_writebyte(0x111f16, 0x00);

	_hal_dmd_riu_writebyte(0x111f22, 0x00);

	_hal_dmd_riu_writebyte(0x1120bc, 0x00);

	_hal_dmd_riu_writebyte(0x101ea1, 0x00);
	_hal_dmd_riu_writebyte(0x101e04, 0x02);

	_hal_dmd_riu_writebyte(0x101e39, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_KERES)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp = 0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_KERES--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x07);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x00);
	_hal_dmd_riu_writebyte(0x111f15, 0x00);
	_hal_dmd_riu_writebyte(0x111f17, 0x00);
	_hal_dmd_riu_writebyte(0x111f16, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	_hal_dmd_riu_writebyte(0x111f2b, 0x00);	//enable clk_rs
	_hal_dmd_riu_writebyte(0x111f2a, 0x10);
	//	No need, it	cause	uart issue.
	//_hal_dmd_riu_writebyte(0x000e13,0x01);
	_hal_dmd_riu_writebyte(0x101ea1, 0x00);

	_hal_dmd_riu_writebyte(0x101e04, 0x02);
	_hal_dmd_riu_writebyte(0x101e76, 0x03);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_EDEN)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_EDEN--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x04);
	_hal_dmd_riu_writebyte(0x103300, 0x0B);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x04);

	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);

	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	_hal_dmd_riu_writebyte(0x111f23, 0x40);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, (val_tmp|0x03));
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_EMERALD)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_EMERALD--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331f, 0x00);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x05);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x103300, 0x11);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);//Different	with EDEN!

	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);

	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	_hal_dmd_riu_writebyte(0x111f23, 0x00);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x111f22, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x00);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x111f24, 0x00);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x111f1E, 0x00);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x111f09, 0x00);//Different	with EDEN!

	val_tmp	=	_hal_dmd_riu_readbyte(0x000e13);
	_hal_dmd_riu_writebyte(0x000e13, val_tmp&0xFB);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_EINSTEIN)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_EINSTEIN--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	//Denny: change	0x10!! 108M
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	//addy update 0809 MAdp_Demod_WriteReg(0x103301, 0x06);
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	//addy update 0809 MAdp_Demod_WriteReg(0x103300, 0x0B);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);

	_hal_dmd_riu_writebyte(0x111f28, 0x00);	//dan	add	for	nugget
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	// note	enable clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);

	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);	//0406 update	0->8
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);

	_hal_dmd_riu_writebyte(0x111f43, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f42, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f45, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f44, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f46, 0x01);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f49, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f48, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f4b, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f4a, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f4c, 0x00);	 //dan add for nugget

	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);//0x08);	VT found some	err.

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_EINSTEIN3)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_EINSTEIN3--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	/*Denny: change 0x10!! 108M*/
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	/*addy update 0809 MAdp_Demod_WriteReg(0x103301, 0x06);*/
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	/*addy update 0809 MAdp_Demod_WriteReg(0x103300, 0x0B);*/
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);

	_hal_dmd_riu_writebyte(0x111f28, 0x00);	//dan	add	for	nugget
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	/*note enable clk_atsc_adcd_sync*/
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);

	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);//0406	update 0->8
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);

	_hal_dmd_riu_writebyte(0x111f43, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f42, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f45, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f44, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f46, 0x01);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f49, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f48, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f4b, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f4a, 0x00);	 //dan add for nugget
	_hal_dmd_riu_writebyte(0x111f4c, 0x00);	 //dan add for nugget

	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);//0x08);	VT found some	err.

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MONACO)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MONACO--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	// DMDMCU	108M
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	// Set parallel	TS clock
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	// enable	ATSC,	DVBTC	TS clock
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	// enable	ADC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	// enable	VIF	DAC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x111f29, 0x00);
	_hal_dmd_riu_writebyte(0x111f28, 0x00);
	// enable	ATSC clock
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	// enable	clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);
	// enable	DVBT inner clock
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);
	// enable	DVBT outer clock
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	// enable	DVBC outer clock
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	// enable	SRAM clock
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	// enable	ISDBT	SRAM share clock and symbol	rate clock
	_hal_dmd_riu_writebyte(0x111f49, 0x44);
	_hal_dmd_riu_writebyte(0x111f48, 0x00);
	// select	clock
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	_hal_dmd_riu_writebyte(0x111f71, 0x1C);
	_hal_dmd_riu_writebyte(0x111f70, 0xC1);
	_hal_dmd_riu_writebyte(0x111f77, 0x04);
	_hal_dmd_riu_writebyte(0x111f76, 0x04);

	//enable SRAM	power	saving
	_hal_dmd_riu_writebyte(0x112091, 0x44);
	_hal_dmd_riu_writebyte(0x112090, 0x00);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_EDISON)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_EDISON--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331f, 0x00);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x06);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x103300, 0x0B);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);//Different	with EDEN!

	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x00);

	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);

	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	_hal_dmd_riu_writebyte(0x111f23, 0x00);//Different	with EDEN!
	_hal_dmd_riu_writebyte(0x111f22, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x00);
	_hal_dmd_riu_writebyte(0x111f24, 0x00);

	_hal_dmd_riu_writebyte(0x111F1E, 0x00);
	_hal_dmd_riu_writebyte(0x111F09, 0x00);

	val_tmp	=	_hal_dmd_riu_readbyte(0x000e13);
	_hal_dmd_riu_writebyte(0x000e13, val_tmp&0xFB);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_EIFFEL)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_EIFFEL--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x101e39, 0x00);
	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	_hal_dmd_riu_writebyte(0x111f28, 0x00);
 //	_hal_dmd_riu_writebyte(0x112028	,0x03);
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);

	val_tmp	=	_hal_dmd_riu_readbyte(0x000e61);
	_hal_dmd_riu_writebyte(0x000e61, val_tmp&0xFE);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MIAMI)
stativ void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MIAMI--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	// DMDMCU	108M
	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	// Set parallel	TS clock
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	// enable	ATSC,	DVBTC	TS clock
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	// enable	ADC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	// Select	MPLLDIV17
	_hal_dmd_riu_writebyte(0x111f28, 0x00);
	// enable	ATSC clock
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	// enable	clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);
	// enable	DVBT inner clock
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);
	// enable	DVBT outer clock
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	// enable	DVBC outer clock
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	// enable	SRAM clock
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	// enable clk_dvbtc_sram4_isdbt_inner4x & clk_adc1x_eq1x clock
	_hal_dmd_riu_writebyte(0x111f49, 0x00);
	_hal_dmd_riu_writebyte(0x111f48, 0x00);
	// select	clock
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	// enable	CCI	LMS	clock
	_hal_dmd_riu_writebyte(0x111f51, 0x00);
	_hal_dmd_riu_writebyte(0x111f50, 0xCC);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MUJI)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MUJI--------------\n"));

	_hal_dmd_riu_writebyteMask(0x101e39, 0x00, 0x03);

	// DMDMCU	108M
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	// Set parallel	TS clock
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	// enable	ATSC,	DVBTC	TS clock
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	// enable	ADC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	// Reset TS	divider
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);
	// enable	VIF	DAC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x111f29, 0x00);
	_hal_dmd_riu_writebyte(0x111f28, 0x00);
	// enable	ATSC clock
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	// enable	clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);
	// enable	DVBT inner clock
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);
	// enable	DVBT outer clock
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	// enable	DVBC outer clock
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	// enable	SRAM clock
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	// enable	ISDBT	SRAM share clock and symbol	rate clock
	_hal_dmd_riu_writebyte(0x111f49, 0x44);
	_hal_dmd_riu_writebyte(0x111f48, 0x00);
	// select	clock
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	_hal_dmd_riu_writebyte(0x111f71, 0x1C);
	_hal_dmd_riu_writebyte(0x111f70, 0xC1);
	_hal_dmd_riu_writebyte(0x111f77, 0x04);
	_hal_dmd_riu_writebyte(0x111f76, 0x04);
	_hal_dmd_riu_writebyte(0x111f4f, 0x08);

	//enable SRAM	power	saving
	_hal_dmd_riu_writebyte(0x112091, 0x44);
	_hal_dmd_riu_writebyte(0x112090, 0x00);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MUNICH)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MUNICH--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	// DMDMCU	108M
	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	// Set parallel	TS clock
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	// enable	ATSC,	DVBTC	TS clock
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	// enable	ADC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	// Select	MPLLDIV17
	_hal_dmd_riu_writebyte(0x111f28, 0x00);
	// enable	ATSC clock
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	// enable	clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);
	// enable	DVBT inner clock
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);
	// enable	DVBT outer clock
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	// enable	DVBC outer clock
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	// enable	SRAM clock
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	// enable clk_dvbtc_sram4_isdbt_inner4x & clk_adc1x_eq1x clock
	_hal_dmd_riu_writebyte(0x111f49, 0x00);
	_hal_dmd_riu_writebyte(0x111f48, 0x00);
	// select	clock
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	// enable	CCI	LMS	clock
	_hal_dmd_riu_writebyte(0x111f51, 0x00);
	_hal_dmd_riu_writebyte(0x111f50, 0xCC);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_KIRIN)
void _hal_intern_atsc_initclk(u8/*bool*/	brfagctristateenable)
{
	u8	val_tmp = 0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_KIRIN--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x07);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x00);
	_hal_dmd_riu_writebyte(0x111f15, 0x00);
	_hal_dmd_riu_writebyte(0x111f17, 0x00);
	_hal_dmd_riu_writebyte(0x111f16, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	_hal_dmd_riu_writebyte(0x111f2b, 0x00);
	_hal_dmd_riu_writebyte(0x111f2a, 0x10);
	_hal_dmd_riu_writebyte(0x101ea1, 0x00);

	_hal_dmd_riu_writebyte(0x101e04, 0x02);
	_hal_dmd_riu_writebyte(0x101e76, 0x03);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, (val_tmp |	0x03));
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MAYA)
void _hal_intern_atsc_initclk(u8/*bool*/	brfagctristateenable)
{
	u8	val_tmp = 0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MAYA--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	// DMDMCU	108M
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	// Set parallel	TS clock
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	// enable	ATSC,	DVBTC	TS clock
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	// enable	ADC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	// Reset TS	divider
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);
	// ADC select	MPLLDIV17	&	EQ select	MPLLDIV5
	_hal_dmd_riu_writebyte(0x111f43, 0x00);
	_hal_dmd_riu_writebyte(0x111f42, 0x00);
	// enable	ATSC clock
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	// configure reg_ckg_atsc50, reg_ckg_atsc25,
	// reg_ckg_atsc_eq25 and reg_ckg_atsc_ce25
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	// enable	clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);
	// enable	DVBC outer clock
	_hal_dmd_riu_writebyte(0x111f12, 0x00);
	// enable	SRAM clock
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	// select	clock
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	// enable	CCI	LMS	clock
	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);
	// set symbol	rate
	_hal_dmd_riu_writebyte(0x111f77, 0x04);
	_hal_dmd_riu_writebyte(0x111f76, 0x04);
	// reg_ckg_adc1x_eq1x
	_hal_dmd_riu_writebyte(0x111f49, 0x04);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, (val_tmp |	0x03));
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MANHATTAN)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MANHATTAN--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	// DMDMCU	108M
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	// Set parallel	TS clock
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	// enable	ATSC,	DVBTC	TS clock
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	// enable	ADC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	// Reset TS	divider
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);
	// enable	VIF	DAC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x111f29, 0x00);
	_hal_dmd_riu_writebyte(0x111f28, 0x00);
	// enable	ATSC clock
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	// enable	clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);
	// enable	DVBT inner clock
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);
	// enable	DVBT outer clock
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	// enable	DVBC outer clock
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x08);
	// enable	SRAM clock
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	// enable	ISDBT	SRAM share clock and symbol	rate clock
	_hal_dmd_riu_writebyte(0x111f49, 0x44);
	_hal_dmd_riu_writebyte(0x111f48, 0x00);
	// select	clock
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	_hal_dmd_riu_writebyte(0x111f71, 0x1C);
	_hal_dmd_riu_writebyte(0x111f70, 0xC1);
	_hal_dmd_riu_writebyte(0x111f77, 0x04);
	_hal_dmd_riu_writebyte(0x111f76, 0x04);
	_hal_dmd_riu_writebyte(0x111f4f, 0x08);

	// ================================================================
	//	ISDBT	SRAM pool	clock	enable
	// ================================================================
	_hal_dmd_riu_writebyte(0x111f81, 0x00);
	_hal_dmd_riu_writebyte(0x111f80, 0x00);
	_hal_dmd_riu_writebyte(0x111f83, 0x00);
	_hal_dmd_riu_writebyte(0x111f82, 0x00);
	_hal_dmd_riu_writebyte(0x111f85, 0x00);
	_hal_dmd_riu_writebyte(0x111f84, 0x00);
	_hal_dmd_riu_writebyte(0x111f87, 0x00);
	_hal_dmd_riu_writebyte(0x111f86, 0x00);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_WHISKY)
void _hal_intern_atsc_initclk(u8/*bool*/	brfagctristateenable)
{
	u8	val_tmp = 0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_WHISKY--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	// DMDMCU	108M
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	// Set parallel	TS clock
	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	// enable	ATSC,	DVBTC	TS clock
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	// enable	ADC	clock	in clkgen_demod
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	// Reset TS	divider
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);
	// ADC select	MPLLDIV17	&	EQ select	MPLLDIV5
	_hal_dmd_riu_writebyte(0x111f43, 0x00);
	_hal_dmd_riu_writebyte(0x111f42, 0x00);
	// enable	ATSC clock
	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	// configure reg_ckg_atsc50, reg_ckg_atsc25,
	//reg_ckg_atsc_eq25 and reg_ckg_atsc_ce25
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	// enable	clk_atsc_adcd_sync
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);
	// enable	DVBC outer clock
	_hal_dmd_riu_writebyte(0x111f12, 0x00);
	// enable	SRAM clock
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	// select	clock
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	// enable	CCI	LMS	clock
	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);
	// set symbol	rate
	_hal_dmd_riu_writebyte(0x111f77, 0x04);
	_hal_dmd_riu_writebyte(0x111f76, 0x04);
	// reg_ckg_adc1x_eq1x
	_hal_dmd_riu_writebyte(0x111f49, 0x04);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, (val_tmp |	0x03));
}
#elif (DMD_ATSC_CHIP_VERSION == DMD_ATSC_CHIP_MASERATI)
	|| (DMD_ATSC_CHIP_VERSION == DMD_ATSC_CHIP_MACAN))
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	//u8	val_tmp	=	0;

	#if	DMD_ATSC_CHIP_VERSION	== DMD_ATSC_CHIP_MASERATI
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MASERATI--------------\n"));
	#else
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MACAN--------------\n"));
	#endif

	//val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	//_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));
	_hal_dmd_riu_writebyteMask(0x101e39, 0x00, 0x03);

	_hal_dmd_riu_writebyte(0x1128d0, 0x01);

	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);

	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);

	_hal_dmd_riu_writebyte(0x1128d0, 0x00);

	_hal_dmd_riu_writebyte(0x152928, 0x00);

	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);

	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);

	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);


	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);

	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);

	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0x81);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);

	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);

	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);

	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111feb, 0x18);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x14);

	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);

	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);

	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);

	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);

	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);

	//val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	//_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MUSTANG)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	//u8	val_tmp	=	0;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MUSTANG--------------\n"));

	//val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	//_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));
	_hal_dmd_riu_writebyteMask(0x101e39, 0x00, 0x03);

	_hal_dmd_riu_writebyte(0x101e39, 0x00);

	_hal_dmd_riu_writebyte(0x10331e, 0x10);


	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);


	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);


	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);

	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);


	_hal_dmd_riu_writebyte(0x152928, 0x00);


	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);

	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);

	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);


	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);
	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);


	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);


	_hal_dmd_riu_writebyte(0x111f77, 0x81);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);


	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);

	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);


	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);


	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111feb, 0x18);
	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);
	_hal_dmd_riu_writebyte(0x111f31, 0x14);


	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);



	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);



	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);


	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);


	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);

	//val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	//_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MAXIM)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	//u8	val_tmp	=	0;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_MAXIM--------------\n"));

	//val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	//_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));
	_hal_dmd_riu_writebyteMask(0x101e39, 0x00, 0x03);

	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x11);


	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);

	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);


	_hal_dmd_riu_writebyte(0x103302, 0x01);

	_hal_dmd_riu_writebyte(0x103302, 0x00);


	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x08);


	_hal_dmd_riu_writebyte(0x152928, 0x00);


	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);


	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);


	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);


	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);


	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);


	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);
	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0x81);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);

	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);


	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);


	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111feb, 0x18);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x14);

	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);


	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);


	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);



	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);

	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_KAYLA)
void _hal_intern_atsc_initclk(u8/*bool*/	brfagctristateenable)
{
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_KAYLA--------------\n"));

	_hal_dmd_riu_writebyteMask(0x101e39, 0x00, 0x03);

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103301, 0x05);
	_hal_dmd_riu_writebyte(0x103300, 0x13);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);

	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);

	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x00);

	_hal_dmd_riu_writebyte(0x111f21, 0x00);
	_hal_dmd_riu_writebyte(0x111f20, 0x00);

	_hal_dmd_riu_writebyte(0x111f29, 0x10);
	_hal_dmd_riu_writebyte(0x111f28, 0x44);

	_hal_dmd_riu_writebyte(0x111f31, 0xcc);
	_hal_dmd_riu_writebyte(0x111f30, 0x08);

	_hal_dmd_riu_writebyte(0x111f32, 0x10);

	_hal_dmd_riu_writebyte(0x111f2f, 0x00);
	_hal_dmd_riu_writebyte(0x111f2e, 0x00);

	_hal_dmd_riu_writebyte(0x111f2d, 0x0c);
	_hal_dmd_riu_writebyte(0x111f2c, 0x00);

	_hal_dmd_riu_writebyte(0x111f22, 0x41);

	_hal_dmd_riu_writebyte(0x111f25, 0x04);
	_hal_dmd_riu_writebyte(0x111f24, 0x00);

	_hal_dmd_riu_writebyte(0x111f26, 0x14);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_KENTUCKY)
void _hal_intern_atsc_initclk(u8/*bool*/	brfagctristateenable)
{
	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_KENTUCKY--------------\n"));

	_hal_dmd_riu_writebyteMask(0x101e39, 0x00, 0x03);

	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103301, 0x22);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);

	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);

	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x152928, 0x05);

	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);

	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);

	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x00);

	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);

	_hal_dmd_riu_writebyte(0x111f23, 0x08);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x00);
	_hal_dmd_riu_writebyte(0x111f3a, 0x00);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);

	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x10);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0x01);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x111fe3, 0x08);
	_hal_dmd_riu_writebyte(0x111fe2, 0x10);

	_hal_dmd_riu_writebyte(0x111ff1, 0x00);
	_hal_dmd_riu_writebyte(0x111ff0, 0x08);

	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);

	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x14);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);

	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);

	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111fef, 0x18);
	_hal_dmd_riu_writebyte(0x111fea, 0x14);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x00);

	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);

	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);

	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);

	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);

	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);

	_hal_dmd_riu_writebyte(0x111f2b, 0x00);
	_hal_dmd_riu_writebyte(0x111f2a, 0x00);

	_hal_dmd_riu_writebyte(0x111f18, 0x04);

	_hal_dmd_riu_writebyte(0x111f26, 0x01);

	if (((_hal_dmd_riu_readbyte(0x0e00+2*0x64) & 0x03) == 2)
	|| ((_hal_dmd_riu_readbyte(0x0e00+2*0x64) & 0x03) == 1)) {
		_hal_dmd_riu_writebyte(0x112830, 0x01);
		_hal_dmd_riu_writebyte(0x112831, 0x00);
		_hal_dmd_riu_writebyte(0x1128b2, 0x11);
	} else {
		_hal_dmd_riu_writebyte(0x112830, 0x00);
		_hal_dmd_riu_writebyte(0x112831, 0x01);
		_hal_dmd_riu_writebyte(0x1128b2, 0x21);
	}

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M5621)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M5621--------------\n");

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));



	_hal_dmd_riu_writebyte(0x1128d0, 0x01);

	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103301, 0x11);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);

	_hal_dmd_riu_writebyte(0x103302, 0x01);

	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);

	_hal_dmd_riu_writebyte(0x1128d0, 0x00);

	_hal_dmd_riu_writebyte(0x152928, 0x00);


	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);


	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);

	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);




	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);


	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);

	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0x81);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);

	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);

	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);


	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111feb, 0x18);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x14);

	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);


	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);


	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);

	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);
	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);
	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M7621)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M7621--------------\n");

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x1128d0, 0x01);

	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103301, 0x01);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);



	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x08);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);


	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);

	_hal_dmd_riu_writebyte(0x1128d0, 0x00);

	_hal_dmd_riu_writebyte(0x152928, 0x00);


	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);

	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);

	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);


	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);


	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);

	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0x81);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);

	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);


	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);

	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);


	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);


	_hal_dmd_riu_writebyte(0x111feb, 0x18);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x14);


	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);


	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);


	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);


	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);

	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);
	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M5321)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M5321--------------\n");

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331e, 0x10);


	_hal_dmd_riu_writebyte(0x10337c, 0x20);
	_hal_dmd_riu_writebyte(0x103301, 0x01);
	_hal_dmd_riu_writebyte(0x103300, 0x15);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);


	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);


	_hal_dmd_riu_writebyte(0x10337c, 0x10);

	_hal_dmd_riu_writebyte(0x10337c, 0x00);


	_hal_dmd_riu_writebyte(0x121628, 0x03);


	_hal_dmd_riu_writebyte(0x121603, 0x00);
	_hal_dmd_riu_writebyte(0x121602, 0x00);


	_hal_dmd_riu_writebyte(0x121605, 0x00);
	_hal_dmd_riu_writebyte(0x121604, 0x00);


	_hal_dmd_riu_writebyte(0x121607, 0x00);
	_hal_dmd_riu_writebyte(0x121606, 0x00);


	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);


	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);

	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x121691, 0x88);
	_hal_dmd_riu_writebyte(0x121690, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0x81);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x12168f, 0x11);
	_hal_dmd_riu_writebyte(0x12168e, 0x88);

	_hal_dmd_riu_writebyte(0x121623, 0x00);
	_hal_dmd_riu_writebyte(0x121622, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x121671, 0x1c);
	_hal_dmd_riu_writebyte(0x121670, 0xc1);


	_hal_dmd_riu_writebyte(0x121677, 0x04);
	_hal_dmd_riu_writebyte(0x121676, 0x04);

	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111feb, 0x18);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x14);

	_hal_dmd_riu_writebyte(0x121681, 0x00);
	_hal_dmd_riu_writebyte(0x121680, 0x00);


	_hal_dmd_riu_writebyte(0x121683, 0x00);
	_hal_dmd_riu_writebyte(0x121682, 0x00);


	_hal_dmd_riu_writebyte(0x121685, 0x00);
	_hal_dmd_riu_writebyte(0x121684, 0x00);


	_hal_dmd_riu_writebyte(0x121687, 0x00);
	_hal_dmd_riu_writebyte(0x121686, 0x00);

	_hal_dmd_riu_writebyte(0x121679, 0x11);
	_hal_dmd_riu_writebyte(0x121678, 0x14);

	_hal_dmd_riu_writebyte(0x12168d, 0x81);
	_hal_dmd_riu_writebyte(0x12168c, 0x44);


	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M7622)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M7622--------------\n");

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));


	_hal_dmd_riu_writebyte(0x10331e, 0x10);


	_hal_dmd_riu_writebyte(0x103301, 0x01);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);


	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);


	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x08);

	_hal_dmd_riu_writebyte(0x152928, 0x00);


	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);

	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);


	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);


	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);

	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);
	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0xc1);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);


	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);


	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);

	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111feb, 0x18);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x14);

	_hal_dmd_riu_writebyte(0x111fef, 0x00);
	_hal_dmd_riu_writebyte(0x111fee, 0xc1);
	_hal_dmd_riu_writebyte(0x152997, 0x01);
	_hal_dmd_riu_writebyte(0x152996, 0x0c);

	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);


	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);

	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);

	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);

	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M7322)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M7322--------------\n");

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x101e68, 0xfc);

	_hal_dmd_riu_writebyte(0x10331e, 0x10);


	_hal_dmd_riu_writebyte(0x103301, 0x01);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);


	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);


	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x08);


	_hal_dmd_riu_writebyte(0x152928, 0x00);

	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);

	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);


	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);


	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);

	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);
	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0xc1);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);


	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);

	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);

	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111feb, 0x18);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x14);

	_hal_dmd_riu_writebyte(0x111fef, 0x00);
	_hal_dmd_riu_writebyte(0x111fee, 0xc1);
	_hal_dmd_riu_writebyte(0x152997, 0x01);
	_hal_dmd_riu_writebyte(0x152996, 0x0c);

	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);

	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);

	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);

	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);

	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);

	_hal_dmd_riu_writebyte(0x101e39, 0x03);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M3822)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M3822--------------\n");

	val_tmp	=	_hal_dmd_riu_readbyte(0x101EC1);
	_hal_dmd_riu_writebyte(0x11261A, val_tmp); //MARLON2 PACKAGE INFORMATION

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	/*add to clear bit 3*/
	val_tmp = _hal_dmd_riu_readbyte(0x100b14);
	val_tmp &= ~0x08;
	_hal_dmd_riu_writebyte(0x100b14, val_tmp);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);


	_hal_dmd_riu_writebyte(0x103301, 0x01);
	_hal_dmd_riu_writebyte(0x103300, 0x11);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);


	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x08);


	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x08);


	_hal_dmd_riu_writebyte(0x152928, 0x00);


	_hal_dmd_riu_writebyte(0x152903, 0x00);
	_hal_dmd_riu_writebyte(0x152902, 0x00);

	_hal_dmd_riu_writebyte(0x152905, 0x00);
	_hal_dmd_riu_writebyte(0x152904, 0x00);

	_hal_dmd_riu_writebyte(0x152907, 0x00);
	_hal_dmd_riu_writebyte(0x152906, 0x00);

	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x08);

	_hal_dmd_riu_writebyte(0x111f21, 0x44);
	_hal_dmd_riu_writebyte(0x111f20, 0x40);

	_hal_dmd_riu_writebyte(0x111f23, 0x10);
	_hal_dmd_riu_writebyte(0x111f22, 0x44);

	_hal_dmd_riu_writebyte(0x111f3b, 0x08);
	_hal_dmd_riu_writebyte(0x111f3a, 0x08);

	_hal_dmd_riu_writebyte(0x111f71, 0x00);
	_hal_dmd_riu_writebyte(0x111f70, 0x00);
	_hal_dmd_riu_writebyte(0x111f73, 0x00);
	_hal_dmd_riu_writebyte(0x111f72, 0x00);

	_hal_dmd_riu_writebyte(0x111f79, 0x11);
	_hal_dmd_riu_writebyte(0x111f78, 0x18);

	_hal_dmd_riu_writebyte(0x152991, 0x88);
	_hal_dmd_riu_writebyte(0x152990, 0x88);

	_hal_dmd_riu_writebyte(0x111f69, 0x44);
	_hal_dmd_riu_writebyte(0x111f68, 0x00);

	_hal_dmd_riu_writebyte(0x111f75, 0x81);
	_hal_dmd_riu_writebyte(0x111f74, 0x11);

	_hal_dmd_riu_writebyte(0x111f77, 0xc1);
	_hal_dmd_riu_writebyte(0x111f76, 0x88);

	_hal_dmd_riu_writebyte(0x15298f, 0x11);
	_hal_dmd_riu_writebyte(0x15298e, 0x88);

	_hal_dmd_riu_writebyte(0x152923, 0x00);
	_hal_dmd_riu_writebyte(0x152922, 0x00);

	_hal_dmd_riu_writebyte(0x111f25, 0x10);
	_hal_dmd_riu_writebyte(0x111f24, 0x11);

	_hal_dmd_riu_writebyte(0x152971, 0x1c);
	_hal_dmd_riu_writebyte(0x152970, 0xc1);

	_hal_dmd_riu_writebyte(0x152977, 0x04);
	_hal_dmd_riu_writebyte(0x152976, 0x04);

	_hal_dmd_riu_writebyte(0x111f6f, 0x11);
	_hal_dmd_riu_writebyte(0x111f6e, 0x00);

	_hal_dmd_riu_writebyte(0x111feb, 0x18);

	_hal_dmd_riu_writebyte(0x111f7f, 0x10);
	_hal_dmd_riu_writebyte(0x111f7e, 0x11);

	_hal_dmd_riu_writebyte(0x111f31, 0x14);

	_hal_dmd_riu_writebyte(0x111fef, 0x00);
	_hal_dmd_riu_writebyte(0x111fee, 0xc1);
	_hal_dmd_riu_writebyte(0x152997, 0x01);
	_hal_dmd_riu_writebyte(0x152996, 0x0c);

	_hal_dmd_riu_writebyte(0x152981, 0x00);
	_hal_dmd_riu_writebyte(0x152980, 0x00);

	_hal_dmd_riu_writebyte(0x152983, 0x00);
	_hal_dmd_riu_writebyte(0x152982, 0x00);

	_hal_dmd_riu_writebyte(0x152985, 0x00);
	_hal_dmd_riu_writebyte(0x152984, 0x00);

	_hal_dmd_riu_writebyte(0x152987, 0x00);
	_hal_dmd_riu_writebyte(0x152986, 0x00);

	_hal_dmd_riu_writebyte(0x152979, 0x11);
	_hal_dmd_riu_writebyte(0x152978, 0x14);

	_hal_dmd_riu_writebyte(0x15298d, 0x81);
	_hal_dmd_riu_writebyte(0x15298c, 0x44);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_MT5895)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_MT5895--------------\n");

	IO_WRITE32(ANA1_BASE,	0x504, 0xBC204624);
	IO_WRITE32(ANA1_BASE,	0x600, 0x4E100000);

	sys.delayms(10);

	IO_WRITE32(ANA1_BASE,	0x408, 0x34541024);
	IO_WRITE32(ANA1_BASE,	0x400, 0x1611000F);
	IO_WRITE32(ANA1_BASE,	0x404, 0x9611000F);
	IO_WRITE32(ANA1_BASE,	0x40c, 0x22214510);
	IO_WRITE32(ANA1_BASE,	0x410, 0x006071BC);
	IO_WRITE32(ANA1_BASE,	0x414, 0xFC806063);

	sys.delayms(1);

	IO_WRITE32(ANA1_BASE,	0x408, 0x74541024);
	IO_WRITE32(ANA1_BASE,	0x418, 0x00000300);

	sys.delayms(1);

	IO_WRITE32(ANA1_BASE,	0x418, 0x00001300);
	IO_WRITE32(CKGEN_BASE, 0x264,	0x00000002);
	TZ_DEMOD_RESET(0);
	sys.delayms(1);
	TZ_DEMOD_RESET(1);

	IO_WRITE32(DEMOD_BASE, 0x4004, 0x00002070);
	IO_WRITE32(DEMOD_BASE, 0x4004, 0x00000070);
	IO_WRITE32MSK(CKGEN_BASE,	0x180, 0x00000000, 0x00000FFF);

	IO_WRITE32(CKGEN_BASE, 0x810,	0x00000001);
	IO_WRITE32(CKGEN_BASE, 0x818,	0x00000001);
	IO_WRITE32(CKGEN_BASE, 0x820,	0x00000001);
	IO_WRITE32(CKGEN_BASE, 0x828,	0x00000001);
	IO_WRITE32(CKGEN_BASE, 0x844,	0x00000001);
	IO_WRITE32MSK(CKGEN_BASE,	0xb14, 0x00100000, 0x00F00000);
	IO_WRITE32MSK(CKGEN_BASE,	0xb18, 0x00100002, 0x0000000F);

	IO_WRITE32(DEMOD_BASE, 0x6a18, 0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x6a00, 0x00000111);
	IO_WRITE32(DEMOD_BASE, 0x6a10, 0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x6b48, 0x00000008);
	IO_WRITE32(DEMOD_BASE, 0x6a04, 0x00000001);
	IO_WRITE32(DEMOD_BASE, 0x6a04, 0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x6b44, 0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x24450,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x24404,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x24408,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x2440c,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x6a14, 0x00000008);
	IO_WRITE32(DEMOD_BASE, 0x6a40, 0x00004440);
	IO_WRITE32(DEMOD_BASE, 0x6a44, 0x00001044);
	IO_WRITE32(DEMOD_BASE, 0x6a74, 0x00000808);

	IO_WRITE32(DEMOD_BASE, 0x6ae0, 0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x6ae4, 0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x6af0, 0x00001118);
	IO_WRITE32(DEMOD_BASE, 0x24520,	0x00008888);
	IO_WRITE32(DEMOD_BASE, 0x6ad0, 0x00004400);

	IO_WRITE32(DEMOD_BASE, 0x6ae8, 0x00008111);
	IO_WRITE32(DEMOD_BASE, 0x6aec, 0x0000c188);
	IO_WRITE32(DEMOD_BASE, 0x2451c,	0x00001188);
	IO_WRITE32(DEMOD_BASE, 0x24444,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x6a48, 0x00001011);
	IO_WRITE32(DEMOD_BASE, 0x244e0,	0x00001cc1);
	IO_WRITE32(DEMOD_BASE, 0x244ec,	0x00000404);
	IO_WRITE32(DEMOD_BASE, 0x6adc, 0x00001100);
	IO_WRITE32(DEMOD_BASE, 0x6bd4, 0x00001811);
	IO_WRITE32(DEMOD_BASE, 0x6afc, 0x00001011);
	IO_WRITE32(DEMOD_BASE, 0x6a60, 0x00001411);
	IO_WRITE32(DEMOD_BASE, 0x6bdc, 0x000000c1);
	IO_WRITE32(DEMOD_BASE, 0x2452c,	0x0000010c);
	IO_WRITE32(DEMOD_BASE, 0x24500,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x24504,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x24508,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x2450c,	0x00000000);
	IO_WRITE32(DEMOD_BASE, 0x244f0,	0x00001114);
	IO_WRITE32(DEMOD_BASE, 0x24518,	0x00008144);

	/*Debug	GUI	Setting*/
	IO_WRITE32MSK(CKGEN_BASE,	0x624, 0x00000880, 0x00000FF0);

}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M7632)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M7632--------------\n");


	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x101e68, 0xfc);
	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x01);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x08);
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x0c);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M7332)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8 val_tmp = 0;
	u8 rb = 0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M7332--------------\n");

	rb = _hal_dmd_riu_readbyte(0x1120C0);

	_hal_dmd_riu_writebyte(0x1120C0, 0x33);
	rb = _hal_dmd_riu_readbyte(0x1120C0);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x101e68, 0xfc);
	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x01);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x08);
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x0c);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);

}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_M7642)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp	=	0;

	#if	USE_DEFAULT_CHECK_TIME
	pres->sdmd_atsc_initdata.vsbagclockchecktime	=	100;
	pres->sdmd_atsc_initdata.vsbprelockchecktime	=	300;
	pres->sdmd_atsc_initdata.vsbfsynclockchecktime	=	600;
	pres->sdmd_atsc_initdata.vsbfeclockchecktime	=	4000;

	pres->sdmd_atsc_initdata.qamagclockchecktime	=	60;
	pres->sdmd_atsc_initdata.qamprelockchecktime	=	1000;
	pres->sdmd_atsc_initdata.qammainlockchecktime	=	2000;
	#endif

	PRINT("--------------DMD_ATSC_CHIP_M7642--------------\n");


	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x101e68, 0xfc);
	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x01);
	_hal_dmd_riu_writebyte(0x103300, 0x11);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x08);
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x0c);

	_hal_dmd_riu_writebyteMask(0x101e39, 0x03, 0x03);
}
#elif	(DMD_ATSC_CHIP_VERSION ==	DMD_ATSC_CHIP_K1C)
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	u8	val_tmp = 0x00;

	HAL_INTERN_ATSC_DBINFO(
	PRINT("--------------DMD_ATSC_CHIP_K1C--------------\n"));

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp&(~0x03));

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);
	_hal_dmd_riu_writebyte(0x103301, 0x32);
	_hal_dmd_riu_writebyte(0x103300, 0x13);
	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);
	_hal_dmd_riu_writebyte(0x103315, 0x00);
	_hal_dmd_riu_writebyte(0x103314, 0x00);
	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x111f03, 0x00);
	_hal_dmd_riu_writebyte(0x111f02, 0x00);
	_hal_dmd_riu_writebyte(0x111f05, 0x00);
	_hal_dmd_riu_writebyte(0x111f04, 0x00);
	_hal_dmd_riu_writebyte(0x111f07, 0x00);
	_hal_dmd_riu_writebyte(0x111f06, 0x00);
	_hal_dmd_riu_writebyte(0x111f0b, 0x00);
	_hal_dmd_riu_writebyte(0x111f0a, 0x00);
	_hal_dmd_riu_writebyte(0x111f0d, 0x00);
	_hal_dmd_riu_writebyte(0x111f0c, 0x00);
	_hal_dmd_riu_writebyte(0x111f0f, 0x00);
	_hal_dmd_riu_writebyte(0x111f0e, 0x00);
	_hal_dmd_riu_writebyte(0x111f11, 0x00);
	_hal_dmd_riu_writebyte(0x111f10, 0x00);
	_hal_dmd_riu_writebyte(0x111f13, 0x00);
	_hal_dmd_riu_writebyte(0x111f12, 0x00);
	_hal_dmd_riu_writebyte(0x111f15, 0x00);
	_hal_dmd_riu_writebyte(0x111f14, 0x00);
	_hal_dmd_riu_writebyte(0x111f17, 0x00);
	_hal_dmd_riu_writebyte(0x111f16, 0x00);
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	_hal_dmd_riu_writebyte(0x111f25, 0x00);
	_hal_dmd_riu_writebyte(0x111f24, 0x00);
	_hal_dmd_riu_writebyte(0x111f23, 0x00);
	_hal_dmd_riu_writebyte(0x111f22, 0x00);
	_hal_dmd_riu_writebyte(0x111f29, 0x00);
	_hal_dmd_riu_writebyte(0x111f28, 0x00);
	_hal_dmd_riu_writebyte(0x111f2b, 0x00);	//enable clk_rs
	_hal_dmd_riu_writebyte(0x111f2a, 0x10);
	_hal_dmd_riu_writebyte(0x111f43, 0x00);
	_hal_dmd_riu_writebyte(0x111f42, 0x02);

	_hal_dmd_riu_writebyte(0x111f19, 0x11);
	_hal_dmd_riu_writebyte(0x111f18, 0x11);

	_hal_dmd_riu_writebyte(0x111f19, 0x22);
	_hal_dmd_riu_writebyte(0x111f18, 0x22);

	_hal_dmd_riu_writebyte(0x111f19, 0x33);
	_hal_dmd_riu_writebyte(0x111f18, 0x33);
	_hal_dmd_riu_writebyte(0x111f19, 0x00);
	_hal_dmd_riu_writebyte(0x111f18, 0x00);
	//_hal_dmd_riu_writebyte(0x000e13,0x01);
	//_hal_dmd_riu_writebyte(0x101ea1,0x00);

	//_hal_dmd_riu_writebyte(0x101e04,0x02);
	//_hal_dmd_riu_writebyte(0x101e76,0x03);

	val_tmp	=	_hal_dmd_riu_readbyte(0x101e39);
	_hal_dmd_riu_writebyte(0x101e39, val_tmp|0x03);
}
#else
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	PRINT("--------------DMD_ATSC_CHIP_NONE--------------\n");
}
#endif

#else
/*#if !DMD_ATSC_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE*/

/*Please modify init clock function according to 3 perty platform*/
static void	_hal_intern_atsc_initclk(u8/*bool*/ brfagctristateenable)
{
	PRINT("--------------DMD_ATSC_CHIP_NONE--------------\n");
}

#endif
/*#if !DMD_ATSC_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE*/

static u8/*bool*/ _hal_intern_atsc_ready(void)
{
	u8 udata = 0x00;
	u32 mb_reg_base_tmp = MB_REG_BASE;

	if (id ==	0)
		mb_reg_base_tmp = MB_REG_BASE;
	else if	(id	== 1)
		mb_reg_base_tmp = MB_REG_BASE_DMD1;

	_hal_dmd_riu_writebyte(mb_reg_base_tmp	+	0x1E,	0x02);

	/*assert interrupt to VD MCU51*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE + 0x03,
	_hal_dmd_riu_readbyte(DMD_MCU_BASE + 0x03)|0x02);
	 /*de-assert interrupt to VD MCU51*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE + 0x03,
	_hal_dmd_riu_readbyte(DMD_MCU_BASE + 0x03)&(~0x02));

	sys.delayms(1);

	udata	=	_hal_dmd_riu_readbyte(mb_reg_base_tmp + 0x1E);

	if (udata)
		return	FALSE;

	_mbx_readreg(0x20C2, &udata);

	udata	=	udata	&	0x0F;

	if (udata	== 0x05	|| udata ==	0x06)
		return FALSE;

	return TRUE;
}

static u8/*bool*/ _hal_intern_atsc_download(void)
{
	u8 udata = 0x00;
	u16 i = 0;
	u16 fail_cnt = 0;
	#if	J83ABC_ENABLE
	u8 tmpdata;
	u16 addressoffset;
	#endif

	if (pres->sdmd_atsc_pridata.bdownloaded) {
		#if	POW_SAVE_BY_DRV
		/*enable DMD MCU51 SRAM*/
		_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x01, 0x01);
		_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00, 0x00, 0x01);
		sys.delayms(1);
		#endif

		if (_hal_intern_atsc_ready()) {
			#if	REMAP_RST_BY_DRV
			 /*reset RIU remapping*/
			_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00,
			0x02, 0x02);
			#endif

			/*reset VD_MCU*/
			_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00,
			0x01, 0x01);
			_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00,
			0x00, 0x03);

			sys.delayms(20);

			return TRUE;
		}
	}

	#if	REMAP_RST_BY_DRV
	/*reset RIU	remapping*/
	_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00, 0x02, 0x02);
	#endif
	/*reset VD_MCU*/
	_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00, 0x01, 0x01);

	/*disable SRAM*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x01, 0x00);

	/*release MCU, madison patch*/
	_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00, 0x00, 0x01);

	/*enable "vdmcu51_if"*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x03, 0x50);
	/*enable auto-increase*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x03, 0x51);
	 /*sram address low byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x04, 0x00);
	/*sram address high byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x05, 0x00);

	////	Load code	thru VDMCU_IF	////
	HAL_INTERN_ATSC_DBINFO(PRINT(">Load	Code...\n"));

	/*write data to VD MCU 51 code sram*/
	for	(i = 0; i < lib_size; i++)
		_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C, INTERN_ATSC_TABLE[i]);

	////	Content	verification ////
	HAL_INTERN_ATSC_DBINFO(PRINT(">Verify	Code...\n"));

	/*sram address low byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x04, 0x00);
	/*sram address high byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x05, 0x00);

	for	(i = 0;	i	<	lib_size; i++) {
		/* read sram data*/
		udata = _hal_dmd_riu_readbyte(DMD_MCU_BASE+0x10);

		if (udata	!= INTERN_ATSC_TABLE[i]) {
			HAL_INTERN_ATSC_DBINFO(PRINT(">fail add = 0x%x\n", i));
			HAL_INTERN_ATSC_DBINFO(PRINT(">code = 0x%x\n",
			INTERN_ATSC_TABLE[i]));
			HAL_INTERN_ATSC_DBINFO(PRINT(">data = 0x%x\n", udata));

		if (fail_cnt++ > 10) {
			HAL_INTERN_ATSC_DBINFO(PRINT(">DSP Loadcode fail!"));
			return FALSE;
		}
		}
	}

	#if	!J83ABC_ENABLE
	_inittable();
	#if	(USE_PSRAM_32KB)// kavana add sram size
	/* sram address low byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x04, 0x00);
	#else
	/* sram address low byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x04, 0x80);
	#endif
	#if	(USE_PSRAM_32KB) //	kavana add sram	 size
	/*sram address high byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x05, 0x70);
	#else
	/*sram address high byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x05, 0x6B);
	#endif

	for (i = 0; i < sizeof(demod_flow_register); i++)
		_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C,
			demod_flow_register[i]);

	#else	// #if !J83ABC_ENABLE
	addressoffset = (INTERN_ATSC_TABLE[0x400] << 8) |
	INTERN_ATSC_TABLE[0x401];

	/* sram address low byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x04, (addressoffset&0xFF));
	/* sram address high byte*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x05, (addressoffset>>8));

	tmpdata	=	(u8)pres->sdmd_atsc_initdata.if_khz;
	// write data to VD MCU 51 code sram
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C, tmpdata);
	tmpdata = (u8)(pres->sdmd_atsc_initdata.if_khz >> 8);
	/* write data to VD MCU 51 code sram*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C, tmpdata);
	tmpdata	=	(u8)pres->sdmd_atsc_initdata.biqswap;
	/* write data to VD MCU 51 code sram*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C, tmpdata);
	tmpdata = (u8)pres->sdmd_atsc_initdata.agc_reference;
	/* write data to VD MCU 51 code sram*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C, tmpdata);
	tmpdata = (u8)(pres->sdmd_atsc_initdata.agc_reference >> 8);
	/* write data to VD MCU 51 code sram*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C, tmpdata);
	tmpdata	=	(u8)pres->sdmd_atsc_initdata.is_dual;
	/* write data to VD MCU 51 code sram*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C, tmpdata);
	tmpdata = (u8)id;
	/* write data to VD MCU 51 code sram*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x0C, tmpdata);
	#endif //	#if	!J83ABC_ENABLE

	/*disable auto-increase*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x03, 0x50);
	/*disable	"vdmcu51_if"*/
	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x03, 0x00);

	_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00, 0x01, 0x01);

	#if	GLO_RST_IN_DOWNLOAD
	_hal_dmd_riu_writebyte(0x101E82, 0x00); //	reset	demod	0,1
	sys.delayms(1);
	_hal_dmd_riu_writebyte(0x101E82, 0x11);
	#endif

	_hal_dmd_riu_writebyte(DMD_MCU_BASE+0x01,	0x01); //enable SRAM

	_hal_dmd_riu_writebyteMask(DMD_MCU_BASE+0x00, 0x00, 0x03);

	pres->sdmd_atsc_pridata.bdownloaded	=	TRUE;

	sys.delayms(20);

	HAL_INTERN_ATSC_DBINFO(PRINT(">DSP Loadcode	done.\n"));

	return TRUE;
}

static void	_hal_intern_atsc_fwversion(void)
{
	u8	data1 = 0, data2 = 0, data3 = 0;

	#if	J83ABC_ENABLE
	_mbx_readreg(0x20C4, &data1);
	_mbx_readreg(0x20C5, &data2);
	_mbx_readreg(0x20C6, &data3);
	#else
	_mbx_readreg(0x20C4, &data1);
	_mbx_readreg(0x20CF, &data2);
	_mbx_readreg(0x20D0, &data3);
	#endif

	PRINT("INTERN_ATSC_FW_VERSION:%x.%x.%x\n", data1,	data2, data3);
}

static u8/*bool*/ _hal_intern_atsc_exit(void)
{
	u8 checkcount = 0;

	_hal_dmd_riu_writebyte(MB_REG_BASE + 0x1C, 0x01);

	_hal_dmd_riu_writebyte(DMD_MCU_BASE + 0x03,
	_hal_dmd_riu_readbyte(DMD_MCU_BASE + 0x03)|0x02);
	_hal_dmd_riu_writebyte(DMD_MCU_BASE + 0x03,
	_hal_dmd_riu_readbyte(DMD_MCU_BASE + 0x03)&(~0x02));

	while ((_hal_dmd_riu_readbyte(MB_REG_BASE + 0x1C)&0x02) != 0x02) {
		sys.delayms(1);

		if (checkcount++ ==	0xFF) {
			PRINT(">>	ATSC Exit	Fail!\n");
			return FALSE;
		}
	}

	PRINT(">>	ATSC Exit	Ok!\n");

	#if	defined	MTK_PROJECT
	IO_WRITE32MSK(CKGEN_BASE, 0xb14, 0x00000000, 0x00F00000);
	#endif
	#if	POW_SAVE_BY_DRV
	#if	defined	MTK_PROJECT
	TZ_DEMOD_RESET(0);
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

static u8/*bool*/ _hal_intern_atsc_softreset(void)
{
	u8 data_8 = 0xFF;

	//Reset	FSM
	if (_mbx_writereg(0x20C0,	0x00) == FALSE)
		return FALSE;

	#if	J83ABC_ENABLE
	while	(data_8	!= 0x02)
	#else
	while	(data_8	!= 0x00)
	#endif
	{
		if (_mbx_readreg(0x20C1, &data_8) == FALSE)
			return FALSE;
	}

	#if	!J83ABC_ENABLE
	//Execute	demod	top	reset
	_mbx_readreg(0x2002, &data_8);
	_mbx_writereg(0x2002,	(data_8|0x10));
	return _mbx_writereg(0x2002, (data_8&(~0x10)));
	#else
	return TRUE;
	#endif
}

static u8/*bool*/ _hal_intern_atsc_setvsbmode(void)
{
	return _mbx_writereg(0x20C0, 0x08);
}

static u8/*bool*/ _hal_intern_atsc_set64qammode(void)
{
	#if	!J83ABC_ENABLE
	if (_mbx_writereg(0x20C3,	0x00) == FALSE)
		return FALSE;
	#endif
	return _mbx_writereg(0x20C0, 0x04);
}

static u8/*bool*/ _hal_intern_atsc_set256qammode(void)
{
	#if	!J83ABC_ENABLE
	if (_mbx_writereg(0x20C3,	0x01) == FALSE)
		return FALSE;
	#endif
	return _mbx_writereg(0x20C0, 0x04);
}

static u8/*bool*/ _hal_intern_atsc_setmodeclean(void)
{
	return _mbx_writereg(0x20C0, 0x00);
}

static enum DMD_ATSC_DEMOD_TYPE _hal_intern_atsc_check8vsb64_256qam(void)
{
	u8 mode = 0;

	#if	J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x2302, &mode); //EQ	mode check
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x3902, &mode); //EQ	mode check
	#else
	_mbx_readreg(0x2A02, &mode); //EQ	mode check
	#endif

	mode &=	0x07;

	if (mode ==	QAM16_J83ABC)
		return DMD_ATSC_DEMOD_ATSC_16QAM;
	else if	(mode	== QAM32_J83ABC)
		return	DMD_ATSC_DEMOD_ATSC_32QAM;
	else if	(mode	== QAM64_J83ABC)
		return	DMD_ATSC_DEMOD_ATSC_64QAM;
	else if	(mode	== QAM128_J83ABC)
		return DMD_ATSC_DEMOD_ATSC_128QAM;
	else if	(mode	== QAM256_J83ABC)
		return DMD_ATSC_DEMOD_ATSC_256QAM;
	else
		return	DMD_ATSC_DEMOD_ATSC_256QAM;
	#else	// #if J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1700, &mode); //mode	check
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x0400, &mode); //mode	check	 (atsc_dmd)
	#else
	_mbx_readreg(0x2900, &mode); //mode	check
	#endif //	#if	J83ABC_ENABLE

	if ((mode&VSB_ATSC)	== VSB_ATSC)
		return	DMD_ATSC_DEMOD_ATSC_VSB;
	else if	((mode & QAM256_ATSC)	== QAM256_ATSC)
		return DMD_ATSC_DEMOD_ATSC_256QAM;
	else
		return	DMD_ATSC_DEMOD_ATSC_64QAM;
	#endif
}

static u8/*bool*/ _hal_intern_atsc_vsb_qam_agclock(void)
{
	#if	SUPPORT_NO_CH_DET
	u8	data1	=	0;
	#endif
	#if	!IS_UTOF_FW
	u32 mb_reg_base_tmp = MB_REG_BASE;
	#endif

	#if	!IS_UTOF_FW
	if (id ==	0)
		mb_reg_base_tmp = MB_REG_BASE;
	else if	(id	== 1)
		mb_reg_base_tmp = MB_REG_BASE_DMD1;

	#if	SUPPORT_NO_CH_DET
	data1	=	_hal_dmd_riu_readbyte(mb_reg_base_tmp + 0x12);
	if ((data1 ==	0x03)	|| (data1	== 0x00))
		return TRUE;
	else
		return FALSE;
	#else
	return TRUE;
	#endif
	#else	// #if !IS_UTOF_FW
	_mbx_readreg(0x20C3, &data1);

	//if (data1	&	0x01)
	//	return FALSE;
	//else
	//	return TRUE;
	return TRUE;
	#endif //	#if	!IS_UTOF_FW
}

static u8/*bool*/ _hal_intern_atsc_vsb_prelock(void)
{
	u8	data1	=	0;
	u8	data2	=	0;
	u16 checkValue;
	u8	ptk_loop_ff_r3 = 0;

	_mbx_readreg(0x20C2, &data1);	//<0>TR_LOCK,	<1>PTK_LOCK

	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1E8F, &ptk_loop_ff_r3); //read	reg_pll_ff_r
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x218F, &ptk_loop_ff_r3); //read	reg_pll_ff_r
	#else
	_mbx_writereg(0x297E,	0x01); //PTK debug output	selection
	_mbx_writereg(0x29E6,	0xff); //load
	_mbx_readreg(0x297D, &ptk_loop_ff_r3); //read	PTK	debug	output
	_mbx_writereg(0x297E,	0x00); //PTK debug output	selection
	_mbx_writereg(0x29E6,	0xff); //load
	#endif

	if ((data1&0x02) ==	0x02 &&
		((ptk_loop_ff_r3 < 0x28) || (ptk_loop_ff_r3 > 0xD8))) {
		#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
		_mbx_readreg(0x18EA, &data1);
		_mbx_readreg(0x18EB, &data2);
		#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
		_mbx_readreg(0x05EA, &data1);	//atsc_dmdext
		_mbx_readreg(0x05EB, &data2);	//atsc_dmdext
		#else
		_mbx_readreg(0x2AEA, &data1);
		_mbx_readreg(0x2AEB, &data2);
		#endif

		checkValue	=	data1	<< 8;
		checkValue |=	data2;

		HAL_INTERN_ATSC_DBINFO(
		PRINT("Internal Pre Locking time:[%d]ms\n", checkValue));

		return TRUE;

	} else {
		HAL_INTERN_ATSC_DBINFO(PRINT("\nPreLock	-	FALSE"));

		return FALSE;
	}
}

static u8/*bool*/ _hal_intern_atsc_vsb_fsync_lock(void)
{
	u8	data1	=	0;
	u8	data2	=	0;
	u16 checkValue;

	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1828, &data1);
	#elif	(USE_T2_MERGED_FEND && USE_BANK_REMAP)
	_mbx_readreg(0x0528, &data1);
	#else
	_mbx_readreg(0x2A28, &data1);
	#endif

	if ((data1&0x80) ==	0x80) {
		#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
		_mbx_readreg(0x18E2, &data1);	//DMDEXT_SPARE02
		_mbx_readreg(0x18E3, &data2);	//DMDEXT_SPARE03
		#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
		_mbx_readreg(0x05E2, &data1);	//DMDEXT_SPARE02
		_mbx_readreg(0x05E3, &data2);	//DMDEXT_SPARE03
		#else
		_mbx_readreg(0x2AE2, &data1);	//DMDEXT_SPARE02
		_mbx_readreg(0x2AE3, &data2);	//DMDEXT_SPARE03
		#endif

		checkValue	=	data1	<< 8;
		checkValue |=	data2;

		HAL_INTERN_ATSC_DBINFO(
		PRINT("Internal Fsync Locking time:[%d]ms\n", checkValue));

		return TRUE;

	} else {
		HAL_INTERN_ATSC_DBINFO(PRINT("\nFsync	Lock - FALSE"));

		return FALSE;
	}
}

static u8/*bool*/ _hal_intern_atsc_vsb_ce_lock(void)
{
	return TRUE;
}

static u8/*bool*/ _hal_intern_atsc_vsb_fec_lock(void)
{
	u8	data1 = 0;
	#if	!IS_UTOF_FW	&& !USE_NEW_FEC_LOCK
	u8	data2 = 0, data3 = 0, data4 = 0;
	u8	data5 = 0, data6 = 0, data7 = 0;
	#endif

	_mbx_readreg(0x20C1, &data1);
	#if	!IS_UTOF_FW	&& !USE_NEW_FEC_LOCK
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1A17, &data2);//AD_NOISE_PWR_TRAIN1
	#else
	_mbx_readreg(0x2C17, &data2);//AD_NOISE_PWR_TRAIN1
	#endif
	_mbx_readreg(0x20C2, &data3);//<0>TR_LOCK, <1>PTK_LOCK
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1901, &data4);//FEC_EN_CTL
	_mbx_readreg(0x1C67, &data5);//addy
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x0601, &data4);//FEC_EN_CTL	(atsc_fec)
	_mbx_readreg(0x2D67, &data5);//addy	 (atsc_eqext)
	#else
	_mbx_readreg(0x2B01, &data4);//FEC_EN_CTL
	_mbx_readreg(0x2D67, &data5);//addy
	#endif
	_mbx_readreg(0x1F01, &data6);
	_mbx_readreg(0x1F40, &data7);
	#endif

	//Driver update	0426 :suggestion for field claim
	#if	IS_UTOF_FW ||	USE_NEW_FEC_LOCK
	if (data1 == 0xF0) {
		HAL_INTERN_ATSC_DBINFO(PRINT("\nFEC	Lock"));
		return TRUE;

	} else {
		HAL_INTERN_ATSC_DBINFO(PRINT("\nFEC	unLock"));
		return FALSE;
	}
	#else
	if (data1 == INTERN_ATSC_OUTER_STATE &&
		((data2 <= INTERN_ATSC_VSB_TRAIN_SNR_LIMIT) ||
		(data5 <= INTERN_ATSC_VSB_TRAIN_SNR_LIMIT)) &&
		((data3&0x02) == 0x02) &&
		((data4&INTERN_ATSC_FEC_ENABLE) == INTERN_ATSC_FEC_ENABLE) &&
		((data6&0x10)	== 0x10) &&	((data7&0x01)	== 0x01)) {
		HAL_INTERN_ATSC_DBINFO(PRINT("\nFEC	Lock"));
		return TRUE;

	} else {
		HAL_INTERN_ATSC_DBINFO(PRINT("\nFEC	unLock"));
		return FALSE;
	}
	#endif
}

static u8/*bool*/ _hal_intern_atsc_qam_prelock(void)
{
	u8 data1 = 0;

	#if	J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x2250, &data1);	//TR_LOCK
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x3850, &data1);	//TR_LOCK
	#else
	_mbx_readreg(0x2950, &data1);	//TR_LOCK
	#endif
	#else	// #if J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1B15, &data1);	//TR_LOCK
	#else
	_mbx_readreg(0x2615, &data1);	//TR_LOCK
	#endif
	#endif //	#if	J83ABC_ENABLE

	#if	J83ABC_ENABLE
	if (data1&0x01) {
		HAL_INTERN_ATSC_DBINFO(PRINT(" QAM preLock OK\n"));
		return TRUE;

	} else {
		HAL_INTERN_ATSC_DBINFO(PRINT("QAM preLock NOT OK\n"));
		return FALSE;
	}
	#else
	if ((data1&0x10) == 0x10) {
		HAL_INTERN_ATSC_DBINFO(PRINT("QAM preLock OK\n"));
		return TRUE;

	} else {
		HAL_INTERN_ATSC_DBINFO(PRINT("QAM preLock NOT OK\n"));
		return FALSE;
	}
	#endif
}

static u8/*bool*/ _hal_intern_atsc_qam_main_lock(void)
{
	u8 data1 = 0, data2 = 0, data3 = 0;
	u8 data4 = 0, data5 = 0, data6 = 0;
	#if	FIX_TS_NON_SYNC_ISSUE
	static u8 last_tsp_count;
	u8 tsp_count = 0;
	u8 check_ts_en;
	#endif

	#if	J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x20C1, &data1);
	_mbx_readreg(0x2418, &data2);	//boundary detected
	_mbx_readreg(0x2250, &data3);	//TR_LOCK
	_mbx_readreg(0x2401, &data4);	//FEC_EN_CTL
	_mbx_readreg(0x2501, &data5);	//RS_backend
	_mbx_readreg(0x2540, &data6);	//RS_backend
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x20C1, &data1);
	_mbx_readreg(0x0618, &data2);	//boundary detected
	_mbx_readreg(0x3850, &data3);	//TR_LOCK
	_mbx_readreg(0x0601, &data4);	//FEC_EN_CTL
	_mbx_readreg(0x1F01, &data5);	//RS_backend
	_mbx_readreg(0x1F40, &data6);	//RS_backend
	#else
	_mbx_readreg(0x20C1, &data1);
	_mbx_readreg(0x2B18, &data2);	//boundary detected
	_mbx_readreg(0x2950, &data3);	//TR_LOCK
	_mbx_readreg(0x2B01, &data4);	//FEC_EN_CTL
	_mbx_readreg(0x2101, &data5);	//RS_backend
	_mbx_readreg(0x2140, &data6);	//RS_backend
	#endif

	if (data1 == INTERN_ATSC_OUTER_STATE &&	(data2&0x01) == 0x01 &&
		data4 == INTERN_ATSC_FEC_ENABLE	&& (data3&0x01)	== 0x01 &&
		((data5&0x10)	== 0x10) &&	((data6&0x01)	== 0x01)) {
		return TRUE;
	} else {
		return FALSE;
	}
	#else	// #if J83ABC_ENABLE
	_mbx_readreg(0x20C1, &data1);
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1918, &data2);	//boundary detected
	_mbx_readreg(0x1B15, &data3);	//TR_LOCK
	_mbx_readreg(0x1901, &data4);	//FEC_EN_CTL
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x0618, &data2);	//boundary detected	(atsc_fec)
	_mbx_readreg(0x2615, &data3);	//TR_LOCK	 (dmd_tr)
	_mbx_readreg(0x0601, &data4);	//FEC_EN_CTL	(atsc_fec)
	#else
	_mbx_readreg(0x2B18, &data2);	//boundary detected
	_mbx_readreg(0x2615, &data3);	//TR_LOCK
	_mbx_readreg(0x2B01, &data4);	//FEC_EN_CTL
	#endif
	_mbx_readreg(0x1F01, &data5);
	_mbx_readreg(0x1F40, &data6);

	if (data1 == INTERN_ATSC_OUTER_STATE &&	(data2&0x01) == 0x01 &&
		data4 == INTERN_ATSC_FEC_ENABLE	&& (data3&0x10) == 0x10	&&
		((data5&0x10)	== 0x10) &&	((data6&0x01)	== 0x01)) {
		#if	FIX_TS_NON_SYNC_ISSUE
		tsp_count = _hal_dmd_riu_readbyte(0x161731);

	if ((last_tsp_count != tsp_count) && (last_tsp_count != 0)) {
		_mbx_readreg(0x3440, &check_ts_en);
		check_ts_en = check_ts_en & (~0x4);
		_mbx_writereg(0x3440, check_ts_en);
		_mbx_readreg(0x3440, &check_ts_en);
		check_ts_en = check_ts_en | (0x4);
		_mbx_writereg(0x3440, check_ts_en);
	}

	last_tsp_count = tsp_count;
	#endif

	return TRUE;

	} else {
		#if	FIX_TS_NON_SYNC_ISSUE
		last_tsp_count = 0;
		#endif

		return FALSE;
	}
	#endif //	#if	J83ABC_ENABLE
}

static u8 _hal_intern_atsc_readifagc(void)
{
	u16 data	=	0;

	#if	J83ABC_ENABLE
	#if	USE_T2_MERGED_FEND
	//debug	select
	_mbx_writereg(0x2822,	0x03);
	//set	freeze & dump
	_mbx_writereg(0x2805,	0x80);
	//Read High	Byte of	IF AGC
	_mbx_readreg(0x2825,	(u8 *)(&data));
	//Unfreeze & dump
	//set	freeze & dump
	_mbx_writereg(0x2805,	0x00);
	#else
	//debug	select
	_mbx_writereg(0x2716,	0x03);
	//set	freeze & dump
	_mbx_writereg(0x2703,	0x80);
	//Read High	Byte of	IF AGC
	_mbx_readreg(0x2719,	(u8 *)(&data));
	//Unfreeze & dump
	//set	freeze & dump
	_mbx_writereg(0x2703,	0x00);
	#endif
	#else	// #if J83ABC_ENABLE
	#if	USE_T2_MERGED_FEND
	//debug	select
	_mbx_writereg(0x2822,	0x03);
	//set	freeze & dump
	_mbx_writereg(0x2805,	0x80);
	//Read High	Byte of	IF AGC
	_mbx_readreg(0x2825,	(u8 *)(&data));
	//Unfreeze & dump
	//set	freeze & dump
	_mbx_writereg(0x2805,	0x00);
	#else
	//debug	select
	_mbx_writereg(0x2716,	0x03);
	//set	freeze & dump
	_mbx_writereg(0x2703,	0x80);
	//Read High	Byte of	IF AGC
	_mbx_readreg(0x2719,	(u8 *)(&data));
	//Unfreeze & dump
	//set	freeze & dump
	_mbx_writereg(0x2703,	0x00);
	#endif
	#endif //	#if	J83ABC_ENABLE

	return data;
}

static void	_hal_intern_atsc_checksignalcondition
(enum DMD_ATSC_SIGNAL_CONDITION *pstatus)
{
	enum DMD_ATSC_DEMOD_TYPE emode;
	#if	J83ABC_ENABLE
	u8 noisepower_h = 0, noisepower_l = 0;
	static u8 noisepowerl_last = 0xff;
	#else
	u8 noisepower_h = 0;
	#endif
	static u8 noisepowerh_last = 0xff;

	emode	=	_hal_intern_atsc_check8vsb64_256qam();

	#if	J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x23BE, &noisepower_l);	//DVBC_EQ
	_mbx_readreg(0x23BF, &noisepower_h);
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x39BE, &noisepower_l);	//DVBC_EQ
	_mbx_readreg(0x39BF, &noisepower_h);
	#else
	_mbx_readreg(0x2ABE, &noisepower_l);	//DVBC_EQ
	_mbx_readreg(0x2ABF, &noisepower_h);
	#endif
	#else	// #if J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1A15, &noisepower_h);
	#else
	_mbx_readreg(0x2C15, &noisepower_h);	//(atsc_eq)
	#endif
	#endif //	#if	J83ABC_ENABLE

	if (emode	== DMD_ATSC_DEMOD_ATSC_VSB) {
		if (!_hal_intern_atsc_vsb_fec_lock())
			noisepower_h = 0xFF;
		else if	(abs(noisepowerh_last-noisepower_h) > 5)
			noisepowerh_last = noisepower_h;
		else
			noisepower_h = noisepowerh_last;

		if (noisepower_h	>	0xBE)	//SNR<14.5
			*pstatus = DMD_ATSC_SIGNAL_NO;
		else if	(noisepower_h > 0x4D) //SNR<18.4
			*pstatus = DMD_ATSC_SIGNAL_WEAK;
		else if	(noisepower_h > 0x23) //SNR<21.8
			*pstatus = DMD_ATSC_SIGNAL_MODERATE;
		else if	(noisepower_h > 0x0A) //SNR<26.9
			*pstatus = DMD_ATSC_SIGNAL_STRONG;
		else
			*pstatus = DMD_ATSC_SIGNAL_VERY_STRONG;
	} else {
		#if	J83ABC_ENABLE
		if (!_hal_intern_atsc_qam_main_lock() || noisepower_h)
			noisepower_l = 0xFF;
		else if	(abs(noisepowerl_last-noisepower_l) > 5)
			noisepowerl_last = noisepower_l;
		else
			noisepower_l = noisepowerl_last;

		//SNR=10*log10(65536/noisepower)
		if (emode	== DMD_ATSC_DEMOD_ATSC_256QAM) {
			if (noisepower_l	>	0x71)	//SNR<27.6
				*pstatus = DMD_ATSC_SIGNAL_NO;
			else if	(noisepower_l > 0x31) //SNR<31.2
				*pstatus = DMD_ATSC_SIGNAL_WEAK;
			else if	(noisepower_l > 0x25) //SNR<32.4
				*pstatus = DMD_ATSC_SIGNAL_MODERATE;
			else if	(noisepower_l > 0x17) //SNR<34.4
				*pstatus = DMD_ATSC_SIGNAL_STRONG;
			else
				*pstatus = DMD_ATSC_SIGNAL_VERY_STRONG;
		} else {
			if (noisepower_l	>	0x1D)	//SNR<21.5
				*pstatus = DMD_ATSC_SIGNAL_NO;
			else if	(noisepower_l > 0x14) //SNR<25.4
				*pstatus = DMD_ATSC_SIGNAL_WEAK;
			else if	(noisepower_l > 0x0F) //SNR<27.8
				*pstatus = DMD_ATSC_SIGNAL_MODERATE;
			else if	(noisepower_l > 0x0B) //SNR<31.4
				*pstatus = DMD_ATSC_SIGNAL_STRONG;
			else
				*pstatus = DMD_ATSC_SIGNAL_VERY_STRONG;
		}
		#else
		if (!_hal_intern_atsc_qam_main_lock())
			noisepower_h = 0xFF;
		else if	(abs(noisepowerh_last-noisepower_h) > 5)
			noisepowerh_last = noisepower_h;
		else
			noisepower_h = noisepowerh_last;

		if (emode	== DMD_ATSC_DEMOD_ATSC_256QAM) {
			if (noisepower_h	>	0x13)	//SNR<27.5
				*pstatus = DMD_ATSC_SIGNAL_NO;
			else if	(noisepower_h > 0x08) //SNR<31.2
				*pstatus = DMD_ATSC_SIGNAL_WEAK;
			else if	(noisepower_h > 0x06) //SNR<32.4
				*pstatus = DMD_ATSC_SIGNAL_MODERATE;
			else if	(noisepower_h > 0x04) //SNR<34.2
				*pstatus = DMD_ATSC_SIGNAL_STRONG;
			else
				*pstatus = DMD_ATSC_SIGNAL_VERY_STRONG;
		} else {
			if (noisepower_h	>	0x4C)	//SNR<21.5
				*pstatus = DMD_ATSC_SIGNAL_NO;
			else if	(noisepower_h > 0x1F) //SNR<25.4
				*pstatus = DMD_ATSC_SIGNAL_WEAK;
			else if	(noisepower_h > 0x11) //SNR<27.8
				*pstatus = DMD_ATSC_SIGNAL_MODERATE;
			else if	(noisepower_h > 0x07) //SNR<31.4
				*pstatus = DMD_ATSC_SIGNAL_STRONG;
			else
				*pstatus = DMD_ATSC_SIGNAL_VERY_STRONG;
		}
		#endif
	}
}

static u8 _hal_intern_atsc_readsnrpercentage(void)
{
	enum DMD_ATSC_DEMOD_TYPE emode;
	u8 noisepower_h = 0, noisepower_l = 0;
	u16 noisepower;

	emode = _hal_intern_atsc_check8vsb64_256qam();

	#if	J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x23BE, &noisepower_l);	//DVBC_EQ
	_mbx_readreg(0x23BF, &noisepower_h);
	#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
	_mbx_readreg(0x39BE, &noisepower_l);	//DVBC_EQ
	_mbx_readreg(0x39BF, &noisepower_h);
	#else
	_mbx_readreg(0x2ABE, &noisepower_l);	//DVBC_EQ
	_mbx_readreg(0x2ABF, &noisepower_h);
	#endif
	#else	// #if J83ABC_ENABLE
	#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
	_mbx_readreg(0x1A14, &noisepower_l);
	_mbx_readreg(0x1A15, &noisepower_h);
	#else
	_mbx_readreg(0x2C14, &noisepower_l);	 //atsc_eq
	_mbx_readreg(0x2C15, &noisepower_h);
	#endif
	#endif //	#if	J83ABC_ENABLE

	noisepower = (noisepower_h<<8) | noisepower_l;

	if (emode == DMD_ATSC_DEMOD_ATSC_VSB) {
		if (!_hal_intern_atsc_vsb_fec_lock())
			return 0;//SNR=0;
		else if	(noisepower <= 0x008A)//SNR>=40dB
			return 100;//SNR=MAX_SNR;
		else if	(noisepower <= 0x0097)//SNR>=39.6dB
			return 99;//
		else if	(noisepower <= 0x00A5)//SNR>=39.2dB
			return 98;//
		else if	(noisepower <= 0x00B5)//SNR>=38.8dB
			return 97;//
		else if	(noisepower <= 0x00C7)//SNR>=38.4dB
			return 96;//
		else if	(noisepower <= 0x00DA)//SNR>=38.0dB
			return 95;//
		else if	(noisepower <= 0x00EF)//SNR>=37.6dB
			return 94;//
		else if	(noisepower <= 0x0106)//SNR>=37.2dB
			return 93;//
		else if	(noisepower <= 0x0120)//SNR>=36.8dB
			return 92;//
		else if	(noisepower <= 0x013B)//SNR>=36.4dB
			return 91;//
		else if	(noisepower <= 0x015A)//SNR>=36.0dB
			return 90;//
		else if	(noisepower <= 0x017B)//SNR>=35.6dB
			return 89;//
		else if	(noisepower <= 0x01A0)//SNR>=35.2dB
			return 88;//
		else if	(noisepower <= 0x01C8)//SNR>=34.8dB
			return 87;//
		else if	(noisepower <= 0x01F4)//SNR>=34.4dB
			return 86;//
		else if	(noisepower <= 0x0224)//SNR>=34.0dB
			return 85;//
		else if	(noisepower <= 0x0259)//SNR>=33.6dB
			return 84;//
		else if	(noisepower <= 0x0293)//SNR>=33.2dB
			return 83;//
		else if	(noisepower <= 0x02D2)//SNR>=32.8dB
			return 82;//
		else if	(noisepower <= 0x0318)//SNR>=32.4dB
			return 81;//
		else if	(noisepower <= 0x0364)//SNR>=32.0dB
			return 80;//
		else if	(noisepower <= 0x03B8)//SNR>=31.6dB
			return 79;//
		else if	(noisepower <= 0x0414)//SNR>=31.2dB
			return 78;//
		else if	(noisepower <= 0x0479)//SNR>=30.8dB
			return 77;//
		else if	(noisepower <= 0x04E7)//SNR>=30.4dB
			return 76;//
		else if	(noisepower <= 0x0560)//SNR>=30.0dB
			return 75;//
		else if	(noisepower <= 0x05E5)//SNR>=29.6dB
			return 74;//
		else if	(noisepower <= 0x0677)//SNR>=29.2dB
			return 73;//
		else if	(noisepower <= 0x0716)//SNR>=28.8dB
			return 72;//
		else if	(noisepower <= 0x07C5)//SNR>=28.4dB
			return 71;//
		else if	(noisepower <= 0x0885)//SNR>=28.0dB
			return 70;//
		else if	(noisepower <= 0x0958)//SNR>=27.6dB
			return 69;//
		else if	(noisepower <= 0x0A3E)//SNR>=27.2dB
			return 68;//
		else if	(noisepower <= 0x0B3B)//SNR>=26.8dB
			return 67;//
		else if	(noisepower <= 0x0C51)//SNR>=26.4dB
			return 66;//
		else if	(noisepower <= 0x0D81)//SNR>=26.0dB
			return 65;//
		else if	(noisepower <= 0x0ECF)//SNR>=25.6dB
			return 64;//
		else if	(noisepower <= 0x103C)//SNR>=25.2dB
			return 63;//
		else if	(noisepower <= 0x11CD)//SNR>=24.8dB
			return 62;//
		else if	(noisepower <= 0x1385)//SNR>=24.4dB
			return 61;//
		else if	(noisepower <= 0x1567)//SNR>=24.0dB
			return 60;//
		else if	(noisepower <= 0x1778)//SNR>=23.6dB
			return 59;//
		else if	(noisepower <= 0x19BB)//SNR>=23.2dB
			return 58;//
		else if	(noisepower <= 0x1C37)//SNR>=22.8dB
			return 57;//
		else if	(noisepower <= 0x1EF0)//SNR>=22.4dB
			return 56;//
		else if	(noisepower <= 0x21EC)//SNR>=22.0dB
			return 55;//
		else if	(noisepower <= 0x2531)//SNR>=21.6dB
			return 54;//
		else if	(noisepower <= 0x28C8)//SNR>=21.2dB
			return 53;//
		else if	(noisepower <= 0x2CB7)//SNR>=20.8dB
			return 52;//
		else if	(noisepower <= 0x3108)//SNR>=20.4dB
			return 51;//
		else if	(noisepower <= 0x35C3)//SNR>=20.0dB
			return 50;//
		else if	(noisepower <= 0x3AF2)//SNR>=19.6dB
			return 49;//
		else if	(noisepower <= 0x40A2)//SNR>=19.2dB
			return 48;//
		else if	(noisepower <= 0x46DF)//SNR>=18.8dB
			return 47;//
		else if	(noisepower <= 0x4DB5)//SNR>=18.4dB
			return 46;//
		else if	(noisepower <= 0x5534)//SNR>=18.0dB
			return 45;//
		else if	(noisepower <= 0x5D6D)//SNR>=17.6dB
			return 44;//
		else if	(noisepower <= 0x6670)//SNR>=17.2dB
			return 43;//
		else if	(noisepower <= 0x7052)//SNR>=16.8dB
			return 42;//
		else if	(noisepower <= 0x7B28)//SNR>=16.4dB
			return 41;//
		else if	(noisepower <= 0x870A)//SNR>=16.0dB
			return 40;//
		else if	(noisepower <= 0x9411)//SNR>=15.6dB
			return 39;//
		else if	(noisepower <= 0xA25A)//SNR>=15.2dB
			return 38;//
		else if	(noisepower <= 0xB204)//SNR>=14.8dB
			return 37;//
		else if	(noisepower <= 0xC331)//SNR>=14.4dB
			return 36;//
		else if	(noisepower <= 0xD606)//SNR>=14.0dB
			return 35;//
		else if	(noisepower <= 0xEAAC)//SNR>=13.6dB
			return 34;//
		else// if	(noisepower>=0xEAAC)//SNR<13.6dB
			return 33;//
	} else {
		if (emode == DMD_ATSC_DEMOD_ATSC_256QAM) {
			if (!_hal_intern_atsc_qam_main_lock())
				return 0;//SNR=0;
			else if	(noisepower <= 0x0117)//SNR>=40dB
				return 100;//
			else if	(noisepower <= 0x0131)//SNR>=39.6dB
				return 99;//
			else if	(noisepower <= 0x014F)//SNR>=39.2dB
				return 98;//
			else if	(noisepower <= 0x016F)//SNR>=38.8dB
				return 97;//
			else if	(noisepower <= 0x0193)//SNR>=38.4dB
				return 96;//
			else if	(noisepower <= 0x01B9)//SNR>=38.0dB
				return 95;//
			else if	(noisepower <= 0x01E4)//SNR>=37.6dB
				return 94;//
			else if	(noisepower <= 0x0213)//SNR>=37.2dB
				return 93;//
			else if	(noisepower <= 0x0246)//SNR>=36.8dB
				return 92;//
			else if	(noisepower <= 0x027E)//SNR>=36.4dB
				return 91;//
			else if	(noisepower <= 0x02BC)//SNR>=36.0dB
				return 90;//
			else if	(noisepower <= 0x02FF)//SNR>=35.6dB
				return 89;//
			else if	(noisepower <= 0x0349)//SNR>=35.2dB
				return 88;//
			else if	(noisepower <= 0x039A)//SNR>=34.8dB
				return 87;//
			else if	(noisepower <= 0x03F3)//SNR>=34.4dB
				return 86;//
			else if	(noisepower <= 0x0455)//SNR>=34.0dB
				return 85;//
			else if	(noisepower <= 0x04C0)//SNR>=33.6dB
				return 84;//
			else if	(noisepower <= 0x0535)//SNR>=33.2dB
				return 83;//
			else if	(noisepower <= 0x05B6)//SNR>=32.8dB
				return 82;//
			else if	(noisepower <= 0x0643)//SNR>=32.4dB
				return 81;//
			else if	(noisepower <= 0x06DD)//SNR>=32.0dB
				return 80;//
			else if	(noisepower <= 0x0787)//SNR>=31.6dB
				return 79;//
			else if	(noisepower <= 0x0841)//SNR>=31.2dB
				return 78;//
			else if	(noisepower <= 0x090D)//SNR>=30.8dB
				return 77;//
			else if	(noisepower <= 0x09EC)//SNR>=30.4dB
				return 76;//
			else if	(noisepower <= 0x0AE1)//SNR>=30.0dB
				return 75;//
			else if	(noisepower <= 0x0BEE)//SNR>=29.6dB
				return 74;//
			else if	(noisepower <= 0x0D15)//SNR>=29.2dB
				return 73;//
			else if	(noisepower <= 0x0E58)//SNR>=28.8dB
				return 72;//
			else if	(noisepower <= 0x0FBA)//SNR>=28.4dB
				return 71;//
			else if	(noisepower <= 0x113E)//SNR>=28.0dB
				return 70;//
			else if	(noisepower <= 0x12E8)//SNR>=27.6dB
				return 69;//
			else if	(noisepower <= 0x14BB)//SNR>=27.2dB
				return 68;//
			else if	(noisepower <= 0x16BB)//SNR>=26.8dB
				return 67;//
			else if	(noisepower <= 0x18ED)//SNR>=26.4dB
				return 66;//
			else if	(noisepower <= 0x1B54)//SNR>=26.0dB
				return 65;//
			else if	(noisepower <= 0x1DF7)//SNR>=25.6dB
				return 64;//
			else if	(noisepower <= 0x20DB)//SNR>=25.2dB
				return 63;//
			else if	(noisepower <= 0x2407)//SNR>=24.8dB
				return 62;//
			else if	(noisepower <= 0x2781)//SNR>=24.4dB
				return 61;//
			else if	(noisepower <= 0x2B50)//SNR>=24.0dB
				return 60;//
			else if	(noisepower <= 0x2F7E)//SNR>=23.6dB
				return 59;//
			else if	(noisepower <= 0x3413)//SNR>=23.2dB
				return 58;//
			else if	(noisepower <= 0x3919)//SNR>=22.8dB
				return 57;//
			else if	(noisepower <= 0x3E9C)//SNR>=22.4dB
				return 56;//
			else if	(noisepower <= 0x44A6)//SNR>=22.0dB
				return 55;//
			else if	(noisepower <= 0x4B45)//SNR>=21.6dB
				return 54;//
			else if	(noisepower <= 0x5289)//SNR>=21.2dB
				return 53;//
			else if	(noisepower <= 0x5A7F)//SNR>=20.8dB
				return 52;//
			else if	(noisepower <= 0x633A)//SNR>=20.4dB
				return 51;//
			else if	(noisepower <= 0x6CCD)//SNR>=20.0dB
				return 50;//
			else if	(noisepower <= 0x774C)//SNR>=19.6dB
				return 49;//
			else if	(noisepower <= 0x82CE)//SNR>=19.2dB
				return 48;//
			else if	(noisepower <= 0x8F6D)//SNR>=18.8dB
				return 47;//
			else if	(noisepower <= 0x9D44)//SNR>=18.4dB
				return 46;//
			else if	(noisepower <= 0xAC70)//SNR>=18.0dB
				return 45;//
			else if	(noisepower <= 0xBD13)//SNR>=17.6dB
				return 44;//
			else if	(noisepower <= 0xCF50)//SNR>=17.2dB
				return 43;//
			else if	(noisepower <= 0xE351)//SNR>=16.8dB
				return 42;//
			else if	(noisepower <= 0xF93F)//SNR>=16.4dB
				return 41;//
			else// if	(noisepower>=0xF93F)//SNR<16.4dB
				return 40;//
		}	else {
			if (!_hal_intern_atsc_qam_main_lock())
				return 0;//SNR=0;
			else if	(noisepower <= 0x0113)//SNR>=40dB
				return 100;//
			else if	(noisepower <= 0x012E)//SNR>=39.6dB
				return 99;//
			else if	(noisepower <= 0x014B)//SNR>=39.2dB
				return 98;//
			else if	(noisepower <= 0x016B)//SNR>=38.8dB
				return 97;//
			else if	(noisepower <= 0x018E)//SNR>=38.4dB
				return 96;//
			else if	(noisepower <= 0x01B4)//SNR>=38.0dB
				return 95;//
			else if	(noisepower <= 0x01DE)//SNR>=37.6dB
				return 94;//
			else if	(noisepower <= 0x020C)//SNR>=37.2dB
				return 93;//
			else if	(noisepower <= 0x023F)//SNR>=36.8dB
				return 92;//
			else if	(noisepower <= 0x0277)//SNR>=36.4dB
				return 91;//
			else if	(noisepower <= 0x02B3)//SNR>=36.0dB
				return 90;//
			else if	(noisepower <= 0x02F6)//SNR>=35.6dB
				return 89;//
			else if	(noisepower <= 0x033F)//SNR>=35.2dB
				return 88;//
			else if	(noisepower <= 0x038F)//SNR>=34.8dB
				return 87;//
			else if	(noisepower <= 0x03E7)//SNR>=34.4dB
				return 86;//
			else if	(noisepower <= 0x0448)//SNR>=34.0dB
				return 85;//
			else if	(noisepower <= 0x04B2)//SNR>=33.6dB
				return 84;//
			else if	(noisepower <= 0x0525)//SNR>=33.2dB
				return 83;//
			else if	(noisepower <= 0x05A5)//SNR>=32.8dB
				return 82;//
			else if	(noisepower <= 0x0630)//SNR>=32.4dB
				return 81;//
			else if	(noisepower <= 0x06C9)//SNR>=32.0dB
				return 80;//
			else if	(noisepower <= 0x0770)//SNR>=31.6dB
				return 79;//
			else if	(noisepower <= 0x0828)//SNR>=31.2dB
				return 78;//
			else if	(noisepower <= 0x08F1)//SNR>=30.8dB
				return 77;//
			else if	(noisepower <= 0x09CE)//SNR>=30.4dB
				return 76;//
			else if	(noisepower <= 0x0AC1)//SNR>=30.0dB
				return 75;//
			else if	(noisepower <= 0x0BCA)//SNR>=29.6dB
				return 74;//
			else if	(noisepower <= 0x0CED)//SNR>=29.2dB
				return 73;//
			else if	(noisepower <= 0x0E2D)//SNR>=28.8dB
				return 72;//
			else if	(noisepower <= 0x0F8B)//SNR>=28.4dB
				return 71;//
			else if	(noisepower <= 0x110A)//SNR>=28.0dB
				return 70;//
			else if	(noisepower <= 0x12AF)//SNR>=27.6dB
				return 69;//
			else if	(noisepower <= 0x147D)//SNR>=27.2dB
				return 68;//
			else if	(noisepower <= 0x1677)//SNR>=26.8dB
				return 67;//
			else if	(noisepower <= 0x18A2)//SNR>=26.4dB
				return 66;//
			else if	(noisepower <= 0x1B02)//SNR>=26.0dB
				return 65;//
			else if	(noisepower <= 0x1D9D)//SNR>=25.6dB
				return 64;//
			else if	(noisepower <= 0x2078)//SNR>=25.2dB
				return 63;//
			else if	(noisepower <= 0x239A)//SNR>=24.8dB
				return 62;//
			else if	(noisepower <= 0x270A)//SNR>=24.4dB
				return 61;//
			else if	(noisepower <= 0x2ACE)//SNR>=24.0dB
				return 60;//
			else if	(noisepower <= 0x2EEF)//SNR>=23.6dB
				return 59;//
			else if	(noisepower <= 0x3376)//SNR>=23.2dB
				return 58;//
			else if	(noisepower <= 0x386D)//SNR>=22.8dB
				return 57;//
			else if	(noisepower <= 0x3DDF)//SNR>=22.4dB
				return 56;//
			else if	(noisepower <= 0x43D7)//SNR>=22.0dB
				return 55;//
			else if	(noisepower <= 0x4A63)//SNR>=21.6dB
				return 54;//
			else if	(noisepower <= 0x5190)//SNR>=21.2dB
				return 53;//
			else if	(noisepower <= 0x596E)//SNR>=20.8dB
				return 52;//
			else if	(noisepower <= 0x620F)//SNR>=20.4dB
				return 51;//
			else if	(noisepower <= 0x6B85)//SNR>=20.0dB
				return 50;//
			else if	(noisepower <= 0x75E5)//SNR>=19.6dB
				return 49;//
			else if	(noisepower <= 0x8144)//SNR>=19.2dB
				return 48;//
			else if	(noisepower <= 0x8DBD)//SNR>=18.8dB
				return 47;//
			else if	(noisepower <= 0x9B6A)//SNR>=18.4dB
				return 46;//
			else if	(noisepower <= 0xAA68)//SNR>=18.0dB
				return 45;//
			else if	(noisepower <= 0xBAD9)//SNR>=17.6dB
				return 44;//
			else if	(noisepower <= 0xCCE0)//SNR>=17.2dB
				return 43;//
			else if	(noisepower <= 0xE0A4)//SNR>=16.8dB
				return 42;//
			else if	(noisepower <= 0xF650)//SNR>=16.4dB
				return 41;//
			else// if	(noisepower>=0xF650)//SNR<16.4dB
				return 40;//
		}
	}
}

#ifdef UTPA2
static u8/*bool*/ _hal_intern_atsc_get_qam_snr
(u16 *pnoisepower, u32 *psym_num)
#else
static u8/*bool*/ _hal_intern_atsc_get_qam_snr(float *f_snr)
#endif
{
	u8	data_8 = 0;
	u16 noisepower	=	0;

	if (_hal_intern_atsc_qam_main_lock()) {
		#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
		// latch
		_mbx_writereg(0x2205,	0x80);
		// read	noise	power
		_mbx_readreg(0x2345, &data_8);
		noisepower = data_8;
		_mbx_readreg(0x2344, &data_8);
		noisepower = (noisepower<<8)|data_8;
		// unlatch
		_mbx_writereg(0x2205,	0x00);
		#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
		// latch
		_mbx_writereg(0x3805,	0x80);
		// read	noise	power
		_mbx_readreg(0x3945, &data_8);
		noisepower = data_8;
		_mbx_readreg(0x3944, &data_8);
		noisepower = (noisepower<<8)|data_8;
		// unlatch
		_mbx_writereg(0x3805,	0x00);
		#else
		// latch
		_mbx_writereg(0x2905,	0x80);
		// read	noise	power
		_mbx_readreg(0x2A45, &data_8);
		noisepower = data_8;
		_mbx_readreg(0x2A44, &data_8);
		noisepower = (noisepower<<8)|data_8;
		// unlatch
		_mbx_writereg(0x2905,	0x00);
		#endif

		if (noisepower == 0x0000)
			noisepower = 0x0001;

		#ifdef UTPA2
		 *pnoisepower = 10*Log10Approx(65536/noisepower);
		 HAL_INTERN_ATSC_DBINFO(
		PRINT("SNR*100 value = %u\n", *pnoisepower));
		#else
		 #ifdef	MSOS_TYPE_LINUX
		 *f_snr = 10.0f*log10f(65536.0f/(float)noisepower);
		 #else
		 *f_snr = 10.0f*Log10Approx(65536.0f/(float)noisepower);
		 #endif
		#endif
	} else {
		#ifdef UTPA2
		*pnoisepower = 0;
		*psym_num	=	0;
		#else
		*f_snr = 0.0f;
		#endif
	}

	return TRUE;
}

static u16 _hal_intern_atsc_readpkterr(void)
{
	u16 data = 0;
	u8 reg = 0;
	enum DMD_ATSC_DEMOD_TYPE emode;

	emode = _hal_intern_atsc_check8vsb64_256qam();

	if (emode	== DMD_ATSC_DEMOD_ATSC_VSB) {
		if (!_hal_intern_atsc_vsb_fec_lock())
			data = 0;
		else {
			_mbx_readreg(0x1F67, &reg);
			data = reg;
			_mbx_readreg(0x1F66, &reg);
			data = (data <<	8)|reg;
		}
	} else {
		if (!_hal_intern_atsc_qam_main_lock())
			data	=	0;
		else {
			#if	J83ABC_ENABLE
			#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
			_mbx_readreg(0x2567, &reg);
			data = reg;
			_mbx_readreg(0x2566, &reg);
			data = (data <<	8)|reg;
			#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
			_mbx_readreg(0x1F67, &reg);
			data = reg;
			_mbx_readreg(0x1F66, &reg);
			data = (data <<	8)|reg;
			#else
			_mbx_readreg(0x2167, &reg);
			data = reg;
			_mbx_readreg(0x2166, &reg);
			data = (data <<	8)|reg;
			#endif
			#else	// #if J83ABC_ENABLE
			_mbx_readreg(0x1F67, &reg);
			data = reg;
			_mbx_readreg(0x1F66, &reg);
			data = (data <<	8)|reg;
			#endif //	#if	J83ABC_ENABLE
		}
	}

	return data;
}

#ifdef UTPA2
static u8/*bool*/ _hal_intern_atsc_readber
(u32 *pbiterr, u16 *pError_window, u32 *pwin_unit)
#else
static u8/*bool*/ _hal_intern_atsc_readber(float *pber)
#endif
{
	u8/*bool*/ status = TRUE;
	u8 reg = 0, reg_frz = 0;
	u16 biterrperiod;
	u32 biterr;
	enum DMD_ATSC_DEMOD_TYPE emode;

	emode = _hal_intern_atsc_check8vsb64_256qam();

	if (emode == DMD_ATSC_DEMOD_ATSC_VSB) {
		if (!_hal_intern_atsc_vsb_fec_lock()) {
			#ifdef UTPA2
			*pbiterr = 0;
			*pError_window = 0;
			*pwin_unit = 0;
			#else
			*pber	=	0;
			#endif
		} else {
			_mbx_readreg(0x1F03, &reg_frz);
			_mbx_writereg(0x1F03,	reg_frz|0x03);

			_mbx_readreg(0x1F47, &reg);
			biterrperiod = reg;
			_mbx_readreg(0x1F46, &reg);
			biterrperiod = (biterrperiod <<	8)|reg;

			status &=	_mbx_readreg(0x1F6d, &reg);
			biterr = reg;
			status &=	_mbx_readreg(0x1F6c, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x1F6b, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x1F6a, &reg);
			biterr = (biterr <<	8)|reg;

			reg_frz = reg_frz&(~0x03);
			_mbx_writereg(0x1F03,	reg_frz);

			if (biterrperiod ==	0) //protect 0
				biterrperiod = 1;

			#ifdef UTPA2
			*pbiterr = biterr;
			*pError_window = biterrperiod;
			*pwin_unit = 8*187*128;
			#else
			if (biterr <= 0)
				*pber = 0.5f /
				((float)biterrperiod*8*187*128);
			else
				*pber = (float)biterr /
				((float)biterrperiod*8*187*128);
			#endif
		}
	} else {
		if (!_hal_intern_atsc_qam_main_lock()) {
			#ifdef UTPA2
			*pbiterr = 0;
			*pError_window = 0;
			*pwin_unit = 0;
			#else
			*pber	=	0;
			#endif
		} else {
			#if	J83ABC_ENABLE
			#if	(USE_T2_MERGED_FEND	&& !USE_BANK_REMAP)
			_mbx_readreg(0x2503, &reg_frz);
			_mbx_writereg(0x2503,	reg_frz|0x03);

			_mbx_readreg(0x2547, &reg);
			biterrperiod = reg;
			_mbx_readreg(0x2546, &reg);
			biterrperiod = (biterrperiod <<	8)|reg;

			status &=	_mbx_readreg(0x256d, &reg);
			biterr = reg;
			status &=	_mbx_readreg(0x256c, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x256b, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x256a, &reg);
			biterr = (biterr <<	8)|reg;

			reg_frz = reg_frz&(~0x03);
			_mbx_writereg(0x2503,	reg_frz);
			#elif	(USE_T2_MERGED_FEND	&& USE_BANK_REMAP)
			_mbx_readreg(0x1F03, &reg_frz);
			_mbx_writereg(0x1F03,	reg_frz|0x03);

			_mbx_readreg(0x1F47, &reg);
			biterrperiod = reg;
			_mbx_readreg(0x1F46, &reg);
			biterrperiod = (biterrperiod <<	8)|reg;

			status &=	_mbx_readreg(0x1F6d, &reg);
			biterr = reg;
			status &=	_mbx_readreg(0x1F6c, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x1F6b, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x1F6a, &reg);
			biterr = (biterr <<	8)|reg;

			reg_frz = reg_frz&(~0x03);
			_mbx_writereg(0x1F03,	reg_frz);
			#else
			_mbx_readreg(0x2103, &reg_frz);
			_mbx_writereg(0x2103,	reg_frz|0x03);

			_mbx_readreg(0x2147, &reg);
			biterrperiod = reg;
			_mbx_readreg(0x2146, &reg);
			biterrperiod = (biterrperiod <<	8)|reg;

			status &=	_mbx_readreg(0x216d, &reg);
			biterr = reg;
			status &=	_mbx_readreg(0x216c, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x216b, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x216a, &reg);
			biterr = (biterr <<	8)|reg;

			reg_frz = reg_frz&(~0x03);
			_mbx_writereg(0x2103,	reg_frz);
			#endif

			if (biterrperiod ==	0)	//protect	0
				biterrperiod = 1;

			#ifdef UTPA2
			*pbiterr = biterr;
			*pError_window = biterrperiod;
			*pwin_unit = 8*188*128;
			#else
			if (biterr <= 0)
				*pber = 0.5f /
				((float)biterrperiod*8*188*128);
			else
				*pber = (float)biterr /
				((float)biterrperiod*8*188*128);
			#endif
			#else	// #if J83ABC_ENABLE
			_mbx_readreg(0x1F03, &reg_frz);
			_mbx_writereg(0x1F03,	reg_frz|0x03);

			_mbx_readreg(0x1F47, &reg);
			biterrperiod = reg;
			_mbx_readreg(0x1F46, &reg);
			biterrperiod = (biterrperiod <<	8)|reg;

			status &=	_mbx_readreg(0x1F6d, &reg);
			biterr = reg;
			status &=	_mbx_readreg(0x1F6c, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x1F6b, &reg);
			biterr = (biterr <<	8)|reg;
			status &=	_mbx_readreg(0x1F6a, &reg);
			biterr = (biterr <<	8)|reg;

			reg_frz = reg_frz&(~0x03);
			_mbx_writereg(0x1F03,	reg_frz);

			if (biterrperiod ==	0)	 //protect 0
				biterrperiod = 1;

			#ifdef UTPA2
			*pbiterr = biterr;
			*pError_window = biterrperiod;
			*pwin_unit = 7*122*128;
			#else
			if (biterr <= 0)
				*pber	=	0.5f /
				((float)biterrperiod*7*122*128);
			else
				*pber = (float)biterr /
				((float)biterrperiod*7*122*128);
			#endif
			#endif //	#if	J83ABC_ENABLE
		}
	}

	return status;
}

#ifdef UTPA2
static u8/*bool*/ _hal_intern_atsc_readfrequencyoffset
(u8	*pMode,	s16 *pFF, s16	*pRate)
#else
static s16 _hal_intern_atsc_readfrequencyoffset(void)
#endif
{
	enum DMD_ATSC_DEMOD_TYPE emode;
	u8 ptk_loop_ff_r3 = 0, ptk_loop_ff_r2 = 0;
	u8 ptk_rate_2 = 0, ptk_rate_1 = 0;
	u8 ad_crl_loop_value0 = 0, ad_crl_loop_value1 = 0;
	#if MIRANDA_ENABLE
	u8 mix_rate_0 = 0, mix_rate_1 = 0;
	u8 mix_rate_2 = 0, mix_rate_3 = 0;
	#else
	u8 mix_rate_0 = 0, mix_rate_1 = 0, mix_rate_2 = 0;
	#endif
	s16 ptk_loop_ff, ptk_rate, ptk_rate_base;
	s16 ad_crl_loop_value;
	s32 mix_rate, mixer_rate_base;
	s16 freqoffset = 0; //kHz

	emode = _hal_intern_atsc_check8vsb64_256qam();

	#ifdef UTPA2
	*pMode = emode;
	#endif

	if (emode == DMD_ATSC_DEMOD_ATSC_VSB) {
		#if (USE_T2_MERGED_FEND && !USE_BANK_REMAP)
		_mbx_readreg(0x1E8E, &ptk_loop_ff_r2); //read reg_pll_ff_r
		_mbx_readreg(0x1E8F, &ptk_loop_ff_r3); //read reg_pll_ff_r
		#elif (USE_T2_MERGED_FEND && USE_BANK_REMAP)
		_mbx_readreg(0x218E, &ptk_loop_ff_r2); //read reg_pll_ff_r
		_mbx_readreg(0x218F, &ptk_loop_ff_r3); //read reg_pll_ff_r
		#else
		_mbx_writereg(0x297E, 0x01); //PTK debug output selection
		_mbx_writereg(0x29E6, 0xff); //load
		_mbx_readreg(0x297C, &ptk_loop_ff_r2); //read PTK debug output
		_mbx_readreg(0x297D, &ptk_loop_ff_r3); //read PTK debug output
		_mbx_writereg(0x297E, 0x00); //PTK debug output selection
		_mbx_writereg(0x29E6, 0xff); //load
		#endif

		ptk_loop_ff = (ptk_loop_ff_r3<<8) | ptk_loop_ff_r2;

		#ifdef UTPA2
		*pFF = ptk_loop_ff;
		#endif

		freqoffset = ptk_loop_ff*FS_RATE/0x80000;

		#if (USE_T2_MERGED_FEND && !USE_BANK_REMAP)
		_mbx_readreg(0x1E44, &ptk_rate_2);
		_mbx_readreg(0x1E43, &ptk_rate_1);
		#elif (USE_T2_MERGED_FEND && USE_BANK_REMAP)
		_mbx_readreg(0x2144, &ptk_rate_2);
		_mbx_readreg(0x2143, &ptk_rate_1);
		#else
		_mbx_readreg(0x2982, &ptk_rate_2);
		_mbx_readreg(0x2981, &ptk_rate_1);
		#endif

		ptk_rate = (ptk_rate_2<<8) | ptk_rate_1;

		ptk_rate_base = 10762*0x4000/(4*FS_RATE);

		freqoffset = (-1)*(freqoffset +
		((ptk_rate-ptk_rate_base) * FS_RATE/0x4000));

		#ifdef UTPA2
		*pRate = freqoffset;
		#endif
	} else {
		#if J83ABC_ENABLE
		#if (USE_T2_MERGED_FEND && !USE_BANK_REMAP)
		_mbx_readreg(0x2340, &ad_crl_loop_value0);
		_mbx_readreg(0x2341, &ad_crl_loop_value1);
		#elif (USE_T2_MERGED_FEND && USE_BANK_REMAP)
		_mbx_readreg(0x3940, &ad_crl_loop_value0);
		_mbx_readreg(0x3941, &ad_crl_loop_value1);
		#else
		_mbx_readreg(0x2A40, &ad_crl_loop_value0);
		_mbx_readreg(0x2A41, &ad_crl_loop_value1);
		#endif

		#if USE_T2_MERGED_FEND
		_mbx_readreg(0x2854, &mix_rate_0);
		_mbx_readreg(0x2855, &mix_rate_1);
		_mbx_readreg(0x2856, &mix_rate_2);
		_mbx_readreg(0x2857, &mix_rate_3);
		#else
		_mbx_readreg(0x2754, &mix_rate_0);
		_mbx_readreg(0x2755, &mix_rate_1);
		_mbx_readreg(0x2756, &mix_rate_2);
		_mbx_readreg(0x2757, &mix_rate_3);
		#endif

		ad_crl_loop_value = (ad_crl_loop_value1 << 8)
							| ad_crl_loop_value0;

		if (emode == DMD_ATSC_DEMOD_ATSC_256QAM)
			freqoffset = (ad_crl_loop_value*5360)/0x10000000;
		else if (emode == DMD_ATSC_DEMOD_ATSC_64QAM)
			freqoffset = (ad_crl_loop_value*5056)/0x200000;

		#else // #if J83ABC_ENABLE

		#if (USE_T2_MERGED_FEND && !USE_BANK_REMAP)
		_mbx_readreg(0x1A04, &ad_crl_loop_value0);
		_mbx_readreg(0x1A05, &ad_crl_loop_value1);
		#else
		_mbx_readreg(0x2C04, &ad_crl_loop_value0);
		_mbx_readreg(0x2C05, &ad_crl_loop_value1);
		#endif

		#if USE_T2_MERGED_FEND
		_mbx_readreg(0x2854, &mix_rate_0);
		_mbx_readreg(0x2855, &mix_rate_1);
		_mbx_readreg(0x2856, &mix_rate_2);
		_mbx_readreg(0x2857, &mix_rate_3);
		#elif MIRANDA_ENABLE
		_mbx_readreg(0x2754, &mix_rate_0);
		_mbx_readreg(0x2755, &mix_rate_1);
		_mbx_readreg(0x2756, &mix_rate_2);
		_mbx_readreg(0x2757, &mix_rate_3);
		#else
		_mbx_readreg(0x2904, &mix_rate_0);
		_mbx_readreg(0x2905, &mix_rate_1);
		_mbx_readreg(0x2906, &mix_rate_2);
		#endif

		ad_crl_loop_value = (ad_crl_loop_value1<<8)
							| ad_crl_loop_value0;

		if (emode == DMD_ATSC_DEMOD_ATSC_256QAM)
			freqoffset = (ad_crl_loop_value*5360)/0x80000;
		else if (emode == DMD_ATSC_DEMOD_ATSC_64QAM)
			freqoffset = (ad_crl_loop_value*5056)/0x80000;

		#endif // #if J83ABC_ENABLE

		#if MIRANDA_ENABLE
		mix_rate = (mix_rate_3 << 24) |
		(mix_rate_2 << 16) | (mix_rate_1 << 8) | mix_rate_0;
		#else
		mix_rate = (mix_rate_2 << 16) |
		(mix_rate_1 << 8) | mix_rate_0;
		#endif

		mixer_rate_base = ((0x01<<MIX_RATE_BIT)/FS_RATE)
				*pres->sdmd_atsc_initdata.if_khz;

		if (pres->sdmd_atsc_initdata.biqswap)
			freqoffset -= (mix_rate-
			mixer_rate_base)/((0x01<<MIX_RATE_BIT)/FS_RATE);
		else
			freqoffset += (mix_rate-
			mixer_rate_base)/((0x01<<MIX_RATE_BIT)/FS_RATE);

		#ifdef UTPA2
		*pFF = ad_crl_loop_value;
		*pRate = freqoffset;
		#endif
	}

	#ifdef UTPA2
	return TRUE;
	#else
	return freqoffset;
	#endif
}

static u8/*bool*/ _hal_intern_atsc_getreg(u16 addr_16, u8 *pdata_8)
{
	return _mbx_readreg(addr_16, pdata_8);
}

static u8/*bool*/ _hal_intern_atsc_setreg(u16 addr_16, u8 data_8)
{
	return _mbx_writereg(addr_16, data_8);
}

/*----------------------------------------*/
/*	Global Functions*/
/*----------------------------------------*/
u8/*bool*/	hal_intern_atsc_ioctl_cmd(struct ATSC_IOCTL_DATA *pdata,
		enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs)
{
	u8/*bool*/	bResult = TRUE;

	id	 = pdata->id;
	pres = pdata->pres;

	sys	 = pdata->sys;

	#if	IS_MULTI_INT_DMD
	_sel_dmd();
	#endif

	switch (eCmd) {
	case DMD_ATSC_HAL_CMD_EXIT:
		bResult = _hal_intern_atsc_exit();
		break;
	case DMD_ATSC_HAL_CMD_INITCLK:
		_hal_intern_atsc_initclk(false);
		break;
	case DMD_ATSC_HAL_CMD_DOWNLOAD:
		bResult = _hal_intern_atsc_download();
		break;
	case DMD_ATSC_HAL_CMD_FWVERSION:
		_hal_intern_atsc_fwversion();
		break;
	case DMD_ATSC_HAL_CMD_SOFTRESET:
		bResult = _hal_intern_atsc_softreset();
		break;
	case DMD_ATSC_HAL_CMD_SETVSBMODE:
		bResult = _hal_intern_atsc_setvsbmode();
		break;
	case DMD_ATSC_HAL_CMD_SET64QAMMODE:
		bResult = _hal_intern_atsc_set64qammode();
		break;
	case DMD_ATSC_HAL_CMD_SET256QAMMODE:
		bResult	=
		_hal_intern_atsc_set256qammode();
		break;
	case DMD_ATSC_HAL_CMD_SETMODECLEAN:
		bResult = _hal_intern_atsc_setmodeclean();
		break;
	case DMD_ATSC_HAL_CMD_SET_QAM_SR:
		break;
	case DMD_ATSC_HAL_CMD_ACTIVE:
		break;
	case DMD_ATSC_HAL_CMD_CHECK8VSB64_256QAM:
		*((enum DMD_ATSC_DEMOD_TYPE *)pargs)	=
		_hal_intern_atsc_check8vsb64_256qam();
		break;
	case DMD_ATSC_HAL_CMD_AGCLOCK:
		bResult	=
		_hal_intern_atsc_vsb_qam_agclock();
		break;
	case DMD_ATSC_HAL_CMD_VSB_PRELOCK:
		bResult = _hal_intern_atsc_vsb_prelock();
		break;
	case DMD_ATSC_HAL_CMD_VSB_FSYNC_LOCK:
		bResult	=
		_hal_intern_atsc_vsb_fsync_lock();
		break;
	case DMD_ATSC_HAL_CMD_VSB_CE_LOCK:
		bResult = _hal_intern_atsc_vsb_ce_lock();
		break;
	case DMD_ATSC_HAL_CMD_VSB_FEC_LOCK:
		bResult	=	_hal_intern_atsc_vsb_fec_lock();
		break;
	case DMD_ATSC_HAL_CMD_QAM_PRELOCK:
		bResult = _hal_intern_atsc_qam_prelock();
		break;
	case DMD_ATSC_HAL_CMD_QAM_MAIN_LOCK:
		bResult	=
		_hal_intern_atsc_qam_main_lock();
		break;
	case DMD_ATSC_HAL_CMD_READIFAGC:
		*((u16	*)pargs) =
		_hal_intern_atsc_readifagc();
		break;
	case DMD_ATSC_HAL_CMD_CHECKSIGNALCONDITION:
		_hal_intern_atsc_checksignalcondition
		((enum DMD_ATSC_SIGNAL_CONDITION *)pargs);
		break;
	case DMD_ATSC_HAL_CMD_READSNRPERCENTAGE:
		*((u8 *)pargs)	=
		_hal_intern_atsc_readsnrpercentage();
		break;
	case DMD_ATSC_HAL_CMD_GET_QAM_SNR:
		#ifdef UTPA2
		bResult = _hal_intern_atsc_get_qam_snr
		(&((*((struct DMD_ATSC_SNR_DATA *)pargs)).noisepower),
		&((*((struct DMD_ATSC_SNR_DATA *)pargs)).sym_num));
		#else
		bResult = _hal_intern_atsc_get_qam_snr
		((float	*)pargs);
		#endif
		break;
	case DMD_ATSC_HAL_CMD_READPKTERR:
		*((u16	*)pargs) =
		_hal_intern_atsc_readpkterr();
		break;
	case DMD_ATSC_HAL_CMD_GETPREVITERBIBER:
		break;
	case DMD_ATSC_HAL_CMD_GETPOSTVITERBIBER:
		#ifdef UTPA2
		bResult = _hal_intern_atsc_readber
		(&((*((struct DMD_ATSC_BER_DATA *)pargs)).biterr),
		&((*((struct DMD_ATSC_BER_DATA *)pargs)).error_window),
		&((*((struct DMD_ATSC_BER_DATA *)pargs)).win_unit));
		#else
		bResult = _hal_intern_atsc_readber
		((float	*)pargs);
		#endif
		break;
	case DMD_ATSC_HAL_CMD_READFREQUENCYOFFSET:
		#ifdef UTPA2
		bResult = _hal_intern_atsc_readfrequencyoffset
		(&((*((struct DMD_ATSC_CFO_DATA *)pargs)).mode),
		&((*((struct DMD_ATSC_CFO_DATA *)pargs)).ff),
		&((*((struct DMD_ATSC_CFO_DATA *)pargs)).rate));
		#else
		*((s16	*)pargs) =
		_hal_intern_atsc_readfrequencyoffset();
		#endif
		break;
	case DMD_ATSC_HAL_CMD_TS_INTERFACE_CONFIG:
		break;
	case DMD_ATSC_HAL_CMD_IIC_BYPASS_MODE:
		break;
	case DMD_ATSC_HAL_CMD_SSPI_TO_GPIO:
		break;
	case DMD_ATSC_HAL_CMD_GPIO_GET_LEVEL:
		break;
	case DMD_ATSC_HAL_CMD_GPIO_SET_LEVEL:
		break;
	case DMD_ATSC_HAL_CMD_GPIO_OUT_ENABLE:
		break;
	case DMD_ATSC_HAL_CMD_GET_REG:
		bResult = _hal_intern_atsc_getreg
		((*((struct DMD_ATSC_REG_DATA *)pargs)).addr_16,
		&((*((struct DMD_ATSC_REG_DATA *)pargs)).data_8));
		break;
	case DMD_ATSC_HAL_CMD_SET_REG:
		bResult = _hal_intern_atsc_setreg
		((*((struct DMD_ATSC_REG_DATA *)pargs)).addr_16,
		 (*((struct DMD_ATSC_REG_DATA *)pargs)).data_8);
		break;
	default:
		break;
	}

	return bResult;
}

u8/*bool*/	mdrv_dmd_atsc_initial_hal_interface(void)
{
	return TRUE;
}

#if	defined(MSOS_TYPE_LINUX_KERNEL) \
&& defined(CONFIG_CC_IS_CLANG) && defined(CONFIG_MSTAR_CHIP)
EXPORT_SYMBOL(hal_intern_atsc_ioctl_cmd);
#endif
