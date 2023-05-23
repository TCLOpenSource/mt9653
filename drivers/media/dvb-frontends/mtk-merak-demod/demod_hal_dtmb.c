// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//---------------------------
//	Include	Files
//---------------------------

#include	"demod_hal_dtmb.h"

#if !defined	MTK_PROJECT
#ifdef	MSOS_TYPE_LINUX_KERNEL
#include	<linux/string.h>
#include	<linux/kernel.h>
#include	<linux/delay.h>
#else
#include	<string.h>
#include	<stdio.h>
#include	<math.h>
#endif
#endif

#ifndef DVB_STI // VT:	Temp	define	for	DVB_Frontend
	#define	DVB_STI	1
#endif
#include	<linux/delay.h>
#include	<asm/io.h>
#if DMD_DTMB_3PARTY_EN
//	Please	add	header	files	needed	by	3	perty	platform
#endif

//---------------------------
//	Driver	Compiler	Options
//---------------------------

#ifndef	BIN_RELEASE
#define	BIN_RELEASE			0
#endif

#if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE || DVB_STI


#define	DMD_DTMB_CHIP_M7332         0x00
#define DMD_DTMB_CHIP_MT5896        0x01

//#if 0
//#if defined(CHIP_M7332)
//	#define	DMD_DTMB_CHIP_VERSION DMD_DTMB_CHIP_M7332
//#elif defined(CHIP_MT5896)
//	#define	DMD_DTMB_CHIP_VERSION DMD_DTMB_CHIP_MT5896
//#else
//	#define	DMD_DTMB_CHIP_VERSION DMD_DTMB_CHIP_MT5896
//#endif
//#endif

#define	DMD_DTMB_CHIP_VERSION DMD_DTMB_CHIP_MT5896

#endif // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

//---------------------------
//	Local	Defines
//---------------------------

#if defined	MTK_PROJECT
#define	_riu_read_byte(addr)\
((addr)&0x01 ? ((u8)((IO_READ32((addr&0x00FFFF00),\
(addr&0x000000FE)<<1))>>8)) : ((u8)((IO_READ32((addr&0x00FFFF00),\
(addr&0x000000FE)<<1))&0x000000FF)))
#define _riu_write_byte(addr,	val) do { if ((addr)&0x01) IO_WRITE32((
	addr&0x00FFFF00), (addr&0x000000FE)<<1, (IO_READ32((addr&0x00FFFF00),
	(addr&0x000000FE)<<1)&0xFFFF00FF)|((((u32)val)<<8)));
	else
		IO_WRITE32((addr&0x00FFFF00), (addr&0x000000FE)<<1, (IO_READ32((
	addr&0x00FFFF00), (addr&0x000000FE)<<1)&0xFFFFFF00)|((u32)val)); }
	while (0)
#elif	DVB_STI
#define	_riu_readbyte(addr)\
(readb(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (addr)))
#define	_riu_writebyte(addr, val)\
(writeb(val, pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (addr)))

#define _riu_read_byte(addr) \
	((addr)&0x01 ? \
	(_riu_readbyte((((addr)&0xFFFFFF00)+(((addr)&0x000000FE)<<1)+1))) :\
	(_riu_readbyte((((addr)&0xFFFFFF00)+(((addr)&0x000000FE)<<1)))))
#define _riu_write_byte(addr, val) \
	((addr)&0x01 ? \
	(_riu_writebyte((((addr)&0xFFFFFF00)+(((addr)&0x000000FE)<<1)+1), val)) :\
	(_riu_writebyte((((addr)&0xFFFFFF00)+(((addr)&0x000000FE)<<1)), val)))


#elif	DMD_DTMB_3PARTY_EN
// Please add riu read&write define according to 3 perty platform
#define	_riu_read_byte(addr)
#define	_riu_write_byte(addr,	val)

#else

#define	_riu_read_byte(addr)\
(read_byte(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (addr)))
#define	_riu_write_byte(addr, val)\
(write_byte(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (addr), val))
#endif

#define	USE_DEFAULT_CHECK_TIME	1

#define	HAL_INTERN_DTMB_DBINFO(y)	y

#ifndef	MBRegBase
	#ifdef	MTK_PROJECT
	#define	MBRegBase	    0x10505E00
	#elif DVB_STI
 #define MBRegBase     0x5E00
	#else
	#define	MBRegBase	0x112600
	#endif
#endif

#ifndef	DMDMcuBase
	#ifdef	MTK_PROJECT
	#define	DMDMcuBase	0x10527D00
	#elif DVB_STI
 #define DMDMcuBase             0x62300
	#else
	#define	DMDMcuBase	0x103480
	#endif
#endif

#ifndef MCU2RegBase
#define MCU2RegBase             0x62400
#endif

#ifndef TOPRegBase
#define TOPRegBase               0x4000
#endif

#if (DMD_DTMB_CHIP_VERSION >= DMD_DTMB_CHIP_M7332)
#define	ISLDPCMERGEDVBT2	1
#else
#define	ISLDPCMERGEDVBT2	0
#endif

#if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE || DVB_STI

//#if (DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_MADISON)
//#define	DTMB_PNM_REG 0x37BA
//#define	DTMB_PND_REG 0x37BB
//#define	DTMB_PNP_REG 0x3849
//#define	DTMB_F_SI_L_REG 0x3790
//#define	DTMB_F_SI_H_REG 0x3791
//#define	DTMB_SI_L_REG 0x379E
//#define	DTMB_SI_H_REG 0x379F
//#define	DTMB_F_CFO_H_REG 0x384D
//#define	DTMB_F_CFO_L_REG 0x384C
//#define	DTMB_S_CFO_REG 0x3850
//#define	DTMB_SNR_2_REG 0x37DA
//#define	DTMB_SNR_1_REG 0x37D9
//#define	DTMB_SNR_0_REG 0x37D8
//#define	DTMB_BER_ERR_3_REG 0x2D3B
//#define	DTMB_BER_ERR_2_REG 0x2D3A
//#define	DTMB_BER_ERR_1_REG 0x2D39
//#define	DTMB_BER_ERR_0_REG 0x2D38
//#define	DTMB_BER_WIN_H_REG 0x2D2F
//#define	DTMB_BER_WIN_L_REG 0x2D2E
//#elif DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_MAXIM\
//|| DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_MACAN\
//|| DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_MASERATI
////USE	P2	bank
//#define	DTMB_PNM_REG 0x11BA
//#define	DTMB_PND_REG 0x11BB
//#define	DTMB_PNP_REG 0x1249
//#define	DTMB_F_SI_L_REG 0x1190
//#define	DTMB_F_SI_H_REG 0x1191
//#define	DTMB_SI_L_REG 0x119E
//#define	DTMB_SI_H_REG 0x119F
//#define	DTMB_F_CFO_H_REG 0x124D
//#define	DTMB_F_CFO_L_REG 0x124C
//#define	DTMB_S_CFO_REG 0x1250
//#define	DTMB_SNR_2_REG 0x11DA
//#define	DTMB_SNR_1_REG 0x11D9
//#define	DTMB_SNR_0_REG 0x11D8
//#define	DTMB_BER_ERR_3_REG 0x163B
//#define	DTMB_BER_ERR_2_REG 0x163A
//#define	DTMB_BER_ERR_1_REG 0x1639
//#define	DTMB_BER_ERR_0_REG 0x1638
//#define	DTMB_BER_WIN_H_REG 0x162F
//#define	DTMB_BER_WIN_L_REG 0x162E
//#elif DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_MONACO\
//|| DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_MUJI\
//|| DMD_DTMB_CHIP_VERSION >= DMD_DTMB_CHIP_MONET
#if (DMD_DTMB_CHIP_VERSION >= DMD_DTMB_CHIP_M7332)
#define	DTMB_PNM_REG 0x3BBA
#define	DTMB_PND_REG 0x3BBB
#define	DTMB_PNP_REG 0x3C49
#define	DTMB_F_SI_L_REG 0x3B90
#define	DTMB_F_SI_H_REG 0x3B91
#define	DTMB_SI_L_REG 0x3B9E
#define	DTMB_SI_H_REG 0x3B9F
#define	DTMB_F_CFO_H_REG 0x3C4D
#define	DTMB_F_CFO_L_REG 0x3C4C
#define	DTMB_S_CFO_REG 0x3C50
#define	DTMB_SNR_2_REG 0x3BDA
#define	DTMB_SNR_1_REG 0x3BD9
#define	DTMB_SNR_0_REG 0x3BD8
#if (ISLDPCMERGEDVBT2)
#define	DTMB_BER_ERR_3_REG 0x3367
#define	DTMB_BER_ERR_2_REG 0x3366
#define	DTMB_BER_ERR_1_REG 0x3365
#define	DTMB_BER_ERR_0_REG 0x3364
#define	DTMB_BER_WIN_H_REG 0x3325
#define	DTMB_BER_WIN_L_REG 0x3324
#else
#define	DTMB_BER_ERR_3_REG 0x3F3B
#define	DTMB_BER_ERR_2_REG 0x3F3A
#define	DTMB_BER_ERR_1_REG 0x3F39
#define	DTMB_BER_ERR_0_REG 0x3F38
#define	DTMB_BER_WIN_H_REG 0x3F2F
#define	DTMB_BER_WIN_L_REG 0x3F2E
#endif
#else
#define	DTMB_PNM_REG 0x22BA
#define	DTMB_PND_REG 0x22BB
#define	DTMB_PNP_REG 0x2349
#define	DTMB_F_SI_L_REG 0x2290
#define	DTMB_F_SI_H_REG 0x2291
#define	DTMB_SI_L_REG 0x229E
#define	DTMB_SI_H_REG 0x229F
#define	DTMB_F_CFO_H_REG 0x234D
#define	DTMB_F_CFO_L_REG 0x234C
#define	DTMB_S_CFO_REG 0x2350
#define	DTMB_SNR_2_REG 0x22DA
#define	DTMB_SNR_1_REG 0x22D9
#define	DTMB_SNR_0_REG 0x22D8
#define	DTMB_BER_ERR_3_REG 0x263B
#define	DTMB_BER_ERR_2_REG 0x263A
#define	DTMB_BER_ERR_1_REG 0x2639
#define	DTMB_BER_ERR_0_REG 0x2638
#define	DTMB_BER_WIN_H_REG 0x262F
#define	DTMB_BER_WIN_L_REG 0x262E
#endif

//#if (DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_M7221)
//#define	DEMOD_GROUP_RSTZ 0x101E68
//#define	DEMOD_RSTZ_EN 0xFE
//#define	DEMOD_RSTZ_DIS 0xFF
//#else
#define	DEMOD_GROUP_RSTZ 0x101E82
#define	DEMOD_RSTZ_EN 0x00
#define	DEMOD_RSTZ_DIS 0x11
//#endif

#else // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

// Please set the following values according to 3 perty platform
#define	DTMB_PNM_REG
#define	DTMB_PND_REG
#define	DTMB_PNP_REG
#define	DTMB_F_SI_L_REG
#define	DTMB_F_SI_H_REG
#define	DTMB_SI_L_REG
#define	DTMB_SI_H_REG
#define	DTMB_F_CFO_H_REG
#define	DTMB_F_CFO_L_REG
#define	DTMB_S_CFO_REG
#define	DTMB_SNR_2_REG
#define	DTMB_SNR_1_REG
#define	DTMB_SNR_0_REG
#define	DTMB_BER_ERR_3_REG
#define	DTMB_BER_ERR_2_REG
#define	DTMB_BER_ERR_1_REG
#define	DTMB_BER_ERR_0_REG
#define	DTMB_BER_WIN_H_REG
#define	DTMB_BER_WIN_L_REG

#define	DEMOD_GROUP_RSTZ
#define	DEMOD_RSTZ_EN
#define	DEMOD_RSTZ_DIS

#endif // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

#define	DTMB_ACI_COEF_SIZE		112

#if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE || DVB_STI

//#if DMD_DTMB_CHIP_VERSION >= DMD_DTMB_CHIP_MASERATI\
//	&& DMD_DTMB_CHIP_VERSION != DMD_DTMB_CHIP_MAINZ
#define	USE_T2_MERGED_FEND	1
//#else
//	#define	USE_T2_MERGED_FEND	0
//#endif

//#if DMD_DTMB_CHIP_VERSION >= DMD_DTMB_CHIP_M7221
#define	POW_SAVE_BY_DRV	1
//#else
//	#define	POW_SAVE_BY_DRV	0
//#endif

//#if DMD_DTMB_CHIP_VERSION >= DMD_DTMB_CHIP_M7632
	#define	GLO_RST_IN_DOWNLOAD	0//1
//#else
//	#define	GLO_RST_IN_DOWNLOAD	0
//#endif

#define	FW_RUN_ON_DRAM_MODE	0//0
// 0: FW download to SRAM, 1: FW download to DRAM

#else // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

// Please set the following values according to 3 perty platform
	#define	USE_T2_MERGED_FEND 0
	#define	POW_SAVE_BY_DRV 0
	#define	GLO_RST_IN_DOWNLOAD 0

	#define	FW_RUN_ON_DRAM_MODE 0
	//0: FW download to SRAM, 1: FW download	to DRAM

#endif // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

#if (FW_RUN_ON_DRAM_MODE == 1)
#define	SRAM_OFFSET 0x8000
#endif

//---------------------------
//	Local	Variables
//---------------------------
#define REG_LARB12_CLK1 0x14e4
#define REG_LARB12_CLK2 0x1d8
#define BIT_6 6
#define BIT_8 8

#define DRAM_FW_OFFSET 0x300000
static u8 id;

static struct DMD_DTMB_ResData	*pRes;

static struct DMD_DTMB_SYS_PTR	sys;

#if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE || DVB_STI
const u8 INTERN_DTMB_table[] = {
	#include	"dmd_intern_dtmb.dat"
};

const u8 INTERN_DTMB_6M_table[] = {
	#include	"dmd_intern_dtmb_6m.dat"
};
const u8 *INTERN_DTMB_table_Waltz = INTERN_DTMB_table;
const u8 *INTERN_DTMB_6M_table_Waltz = INTERN_DTMB_6M_table;

#else // #if !DMD_DTMB_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE
// Please set fw dat file used by 3 perty platform
const u8 INTERN_DTMB_table[] = {
	#include	"dmd_intern_dtmb.dat"
};
#endif // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

static const u32 _SNR_TABLE[50] = {4340, 4983, 5721, 6569, 7542, 8659, 9942,
11415, 13107, 15048, 17278, 19838, 22777, 26151, 30026, 34474, 39581, 45446,
52179, 59909, 68785, 78975, 90676, 104110, 119534, 137244, 157577, 180922,
207726, 238502, 273837, 314407, 360987, 414469, 475874, 546376, 627324,
720264, 826974, 949493, 1090164, 1251676, 1437116, 1650030, 1894488,
2175163, 2497421, 2867423, 3292242, 3292242};
//---------------------------
//	Global	Variables
//---------------------------
//static u8 _riu_read_byte1(u32 uaddr32)
//{
//	return readb(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (uaddr32));
//}

//static void _riu_write_byte1(u32 uaddr32,	u8 val8)
//{
//	writeb(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (uaddr32), val8);
//}
//---------------------------
//	Local	Functions
//---------------------------
static u8 _hal_dmd_riu_readbyte(u32 uaddr32)
{
	#if (!DVB_STI) && (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN)
	return _riu_read_byte(uaddr32);
	#elif	DVB_STI
	//return _riu_read_byte(((uaddr32) << 1)	-	((uaddr32) &	1));
	return  _riu_read_byte(uaddr32);
	#else
	return _riu_read_byte(((uaddr32) << 1)	-	((uaddr32) &	1));
	#endif
}

static void _hal_dmd_riu_writebyte(u32 uaddr32,	u8 val8)
{
	#if (!DVB_STI) && (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN)
	_riu_write_byte(uaddr32,	val8);
	#elif	DVB_STI
	//_riu_write_byte(((uaddr32) << 1) - ((uaddr32) & 1), val8);
	_riu_write_byte(uaddr32, val8);
	#else
	_riu_write_byte(((uaddr32) << 1) - ((uaddr32) & 1), val8);
	#endif
}

static void _hal_dmd_riu_writebytemask(
u32 uaddr32, u8 val8, u8 umask8)
{
	#if (!DVB_STI) && (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN)
	_riu_write_byte(uaddr32, (_riu_read_byte(uaddr32) & ~(umask8))
	| ((val8) & (umask8)));
	#elif DVB_STI
	_riu_write_byte(uaddr32, (_riu_read_byte(uaddr32) & ~(umask8))
	| ((val8) & (umask8)));
	#else
	_riu_write_byte((((uaddr32) << 1) - ((uaddr32) & 1)),
	(_riu_read_byte((((uaddr32) << 1) - ((uaddr32) & 1)))
	& ~(umask8)) | ((val8) & (umask8)));
	#endif
}

static u8 _mbx_writereg(u16 uaddr16,	u8 udata8)
{
	u8 ucheckcount8;
	u8 ucheckflag8;

	_hal_dmd_riu_writebyte(MBRegBase + 0x00,	(uaddr16&0xff));
	_hal_dmd_riu_writebyte(MBRegBase + 0x01,	(uaddr16>>8));
	_hal_dmd_riu_writebyte(MBRegBase + 0x10,	udata8);
	_hal_dmd_riu_writebyte(MBRegBase + 0x1E, 0x01);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03, _hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)|0x02);//assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03, _hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)&(~0x02));//de-assert interrupt to VD MCU51

	for	(ucheckcount8 = 0; ucheckcount8 < 10; ucheckcount8++) {
		ucheckflag8 = _hal_dmd_riu_readbyte(MBRegBase + 0x1E);
	if ((ucheckflag8&0x01) == 0)
		break;

	sys.DelayMS(1);
	}

	if (ucheckflag8&0x01) {
//VT PRINT("ERROR: DTMB INTERN DEMOD MBX WRITE TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

static u8 _mbx_readreg(u16 uaddr16,	u8 *udata8)
{
	u8 ucheckcount8;
	u8 ucheckflag8;

	_hal_dmd_riu_writebyte(MBRegBase + 0x00,	(uaddr16&0xff));
	_hal_dmd_riu_writebyte(MBRegBase + 0x01,	(uaddr16>>8));
	_hal_dmd_riu_writebyte(MBRegBase + 0x1E, 0x02);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,	_hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)|0x02);// assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,	_hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)&(~0x02));// de-assert interrupt to VD MCU51

	for	(ucheckcount8 = 0; ucheckcount8 < 10; ucheckcount8++) {
		ucheckflag8 = _hal_dmd_riu_readbyte(MBRegBase + 0x1E);
	if ((ucheckflag8&0x02) == 0) {
		*udata8 = _hal_dmd_riu_readbyte(MBRegBase + 0x10);
		break;
	}

	sys.DelayMS(1);
	}

	if (ucheckflag8&0x02) {
//VT  PRINT("ERROR: DTMB INTERN DEMOD MBX READ TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

#if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE || DVB_STI

#if (DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_M7332)
static void _hal_intern_dtmb_initclk(void)
{

	u8 rb = 0;
	#if USE_DEFAULT_CHECK_TIME
	pRes->sDMD_DTMB_InitData.udtmbagclockchecktime16 = 50;
	pRes->sDMD_DTMB_InitData.udtmbprelockchecktime16 = 300;
	pRes->sDMD_DTMB_InitData.udtmbpnmlockchecktime16 = 500;
	pRes->sDMD_DTMB_InitData.udtmbfeclockchecktime16 = 4000;

	pRes->sDMD_DTMB_InitData.uqamagclockchecktime16 = 50;
	pRes->sDMD_DTMB_InitData.uqamprelockchecktime16 = 1000;
	pRes->sDMD_DTMB_InitData.uqammainlockchecktime16 = 3000;
	#endif

	HAL_INTERN_DTMB_DBINFO(PRINT("!-----DMD_DTMB_CHIP_M7332-----!\n"));

	rb = _hal_dmd_riu_readbyte(0x1120C0);

	_hal_dmd_riu_writebytemask(0x101e39, 0x00, 0x03);

	_hal_dmd_riu_writebyte(0x101e68, 0xfc);

	_hal_dmd_riu_writebyte(0x10331f, 0x00);
	_hal_dmd_riu_writebyte(0x10331e, 0x10);

	_hal_dmd_riu_writebyte(0x103301, 0x19);
	_hal_dmd_riu_writebyte(0x103300, 0x13);

	_hal_dmd_riu_writebyte(0x103309, 0x00);
	_hal_dmd_riu_writebyte(0x103308, 0x00);

	_hal_dmd_riu_writebyte(0x103321, 0x00);
	_hal_dmd_riu_writebyte(0x103320, 0x00);

	_hal_dmd_riu_writebyte(0x103302, 0x01);
	_hal_dmd_riu_writebyte(0x103302, 0x00);

	_hal_dmd_riu_writebyte(0x103314, 0x04);

	_hal_dmd_riu_writebytemask(0x101e39, 0x03, 0x03);

}
#elif (DMD_DTMB_CHIP_VERSION == DMD_DTMB_CHIP_MT5896)
static void _hal_intern_dtmb_initclk(void)
{
	u8 reg = 0;
	u16 temp = 0;
	u32 temp1 = 0;

	HAL_INTERN_DTMB_DBINFO(PRINT("------DMD_DTMB_CHIP_MERAK------\n"));
	#if USE_DEFAULT_CHECK_TIME
	pRes->sDMD_DTMB_InitData.udtmbagclockchecktime16 = 50;
	pRes->sDMD_DTMB_InitData.udtmbprelockchecktime16 = 300;
	pRes->sDMD_DTMB_InitData.udtmbpnmlockchecktime16 = 500;
	pRes->sDMD_DTMB_InitData.udtmbfeclockchecktime16 = 4000;

	pRes->sDMD_DTMB_InitData.uqamagclockchecktime16 = 50;
	pRes->sDMD_DTMB_InitData.uqamprelockchecktime16 = 1000;
	pRes->sDMD_DTMB_InitData.uqammainlockchecktime16 = 3000;
	#endif

	temp = readw(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x14e4);
	temp |= 0x240;
	writew(temp, pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x14e4);

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x330) & 0xFFFFFFFE;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x330));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x340) & 0xFFFFFFFE;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x340));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1D8) & 0xFFFFFCF8;
	temp1 |= 0x00000004;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1D8));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x2B8) & 0xFFFFFFF8;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x2B8));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x2C0) & 0xFFFFFFFC;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x2C0));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x2C8) & 0xFFFFFFFC;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x2C8));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x2D8) & 0xFFFFFFFC;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x2D8));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x510) & 0xFFFFFFFC;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x510));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0xAB8) & 0xFFFFFFFC;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0xAB8));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1598) & 0xFFFFFFFE;
	temp1 |= 0x00000001;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1598));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1740) & 0xFFFFFFFE;
	temp1 |= 0x00000001;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1740));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x19C8) & 0xFFFFFFFE;
	temp1 |= 0x00000001;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x19C8));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x19CC) & 0xFFFFFFFE;
	temp1 |= 0x00000001;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x19CC));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x19D0) & 0xFFFFFFFE;
	temp1 |= 0x00000001;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x19D0));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x19D4) & 0xFFFFFFFE;
	temp1 |= 0x00000001;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x19D4));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3040) & 0xFFFFFCE0;
	temp1 |= 0x00000004;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3040));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3110) & 0xFFFFFCE0;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3110));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x31C0) & 0xFFFFFFFC;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x31C0));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3B4C) & 0xFFFFFFDF;
	temp1 |= 0x00000020;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3B4C));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3B5C) & 0xFFFFFFFE;
	temp1 |= 0x00000001;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3B5C));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3B74) & 0xFFFFFFFB;
	temp1 |= 00000004;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3B74));

	if (pRes->sDMD_DTMB_PriData.m6_version == 0x0100) {
		HAL_INTERN_DTMB_DBINFO(PRINT("------DMD_DTMB_CHIP_MT5896------\n"));
		writel(0x000000FC, pRes->sDMD_DTMB_PriData.virt_sram_base_addr);
		_hal_dmd_riu_writebyte(MBRegBase + 0x1A, 0x00);
	}

	else if (pRes->sDMD_DTMB_PriData.m6_version == 0x0200)	{
		HAL_INTERN_DTMB_DBINFO(PRINT("------DMD_DTMB_CHIP_MT5897------\n"));
		_hal_dmd_riu_writebyte(MBRegBase + 0x1A, 0x01);
		reg = readb(pRes->sDMD_DTMB_PriData.virt_sram_base_addr);
		reg &= 0xfe;
		writeb(reg, pRes->sDMD_DTMB_PriData.virt_sram_base_addr);
		//writel(0x00000000, pRes->sDMD_DTMB_PriData.virt_sram_base_addr);//-0x1AE4

		reg = readb(pRes->sDMD_DTMB_PriData.virt_pmux_base_addr);
		reg = (reg&0xf0) | 0x03;
		writeb(reg, pRes->sDMD_DTMB_PriData.virt_pmux_base_addr);
		/* [11:8] */
		temp = readw(pRes->sDMD_DTMB_PriData.virt_vagc_base_addr);
		temp = (temp&0xf0ff) | 0x0300;
		writew(temp, pRes->sDMD_DTMB_PriData.virt_vagc_base_addr);
	}
	reg = _hal_dmd_riu_readbyte(MBRegBase + 0x1A);
	HAL_INTERN_DTMB_DBINFO(PRINT("MBRegBase + 0x1A = %d\n", reg));


	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3110) & 0xFFFFFCE0;
	temp1 |= 0x00000014;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x3110));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F80) & 0xFFFFFF00;
	temp1 |= 0x00000013;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F80));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F88) & 0xFFFFFFF0;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F88));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F8C) & 0xFFFFF0FF;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F8C));

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F84) & 0xFFFFFFFE;
	temp1 |= 0x00000001;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F84));

	sys.DelayMS(1);

	temp1 = readl(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F84) & 0xFFFFFFFE;
	writel(temp1, (pRes->sDMD_DTMB_PriData.virt_clk_base_addr + 0x1F84));

	#if FW_RUN_ON_DRAM_MODE
	HAL_INTERN_DTMB_DBINFO(PRINT("--load DRAM DTMB Init_clk setting--\n"));
	//HAL_DMD_RIU_WriteByte(0x153de0,0x23);
	//HAL_DMD_RIU_WriteByte(0x153de1,0x21);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0xE0, 0x23);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0xE1, 0x21);

	//HAL_DMD_RIU_WriteByte(0x153de4,0x01);
	//HAL_DMD_RIU_WriteByte(0x153de6,0x11);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0xE4, 0x01);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0xE6, 0x11);

	//HAL_DMD_RIU_WriteByte(0x153d00,0x00);
	//HAL_DMD_RIU_WriteByte(0x153d01,0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x00, 0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x01, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d04,0x00);
	//HAL_DMD_RIU_WriteByte(0x153d05,0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x04, 0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x05, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d02,0x00);
	//HAL_DMD_RIU_WriteByte(0x153d03,0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x02, 0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x03, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d06,(INTERNAL_DVBT2_DRAM_OFFSET-1)&0xff);
	//HAL_DMD_RIU_WriteByte(0x153d07,(INTERNAL_DVBT2_DRAM_OFFSET-1)>>8);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x06, 0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x07, 0x00);

	//u32DMD_DVBT2_FW_START_ADDR -= 0x5000;
	//u16_temp_val = (MS_U16)(u32DMD_DVBT2_FW_START_ADDR>>16);
	//HAL_DMD_RIU_WriteByte(0x153d1a,(MS_U8)u16_temp_val);
	//HAL_DMD_RIU_WriteByte(0x153d1b,(MS_U8)(u16_temp_val>>8));
	temp = (u16)(((pRes->sDMD_DTMB_InitData.utdistartaddr32 * 16) + (3 * 1024 * 1024))>>16);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x1A, (u8)temp);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x1B, (u8)(temp>>8));
	temp = (u16)(((pRes->sDMD_DTMB_InitData.utdistartaddr32 * 16) +
		(DRAM_FW_OFFSET))>>32);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x20, (u8)temp);

	//HAL_DMD_RIU_WriteByte(0x153D08,0x00);
	//HAL_DMD_RIU_WriteByte(0x153d09,0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x08, 0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x09, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d0c,INTERNAL_DVBT2_DRAM_OFFSET&0xff);
	//HAL_DMD_RIU_WriteByte(0x153d0d,INTERNAL_DVBT2_DRAM_OFFSET>>8);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x0C, 0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x0D, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d0a,0x01);
	//HAL_DMD_RIU_WriteByte(0x153d0b,0x00);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x0A, 0x01);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x0B, 0x00);

	//HAL_DMD_RIU_WriteByte(0x153d0e,0xff);
	//HAL_DMD_RIU_WriteByte(0x153d0f,0xff);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x0E, 0xff);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x0F, 0xff);

	//HAL_DMD_RIU_WriteByte(0x153d18,0x04);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x18, 0x04);

	//u8_temp_val = HAL_DMD_RIU_ReadByte(0x112001);
	//u8_temp_val &= (~0x10);
	//HAL_DMD_RIU_WriteByte(0x112001, u8_temp_val);
	reg = _hal_dmd_riu_readbyte(TOPRegBase + 0x01);
	reg &= (~0x10);
	_hal_dmd_riu_writebyte(TOPRegBase + 0x01, reg);

	//HAL_DMD_RIU_WriteByte(0x153d1c,0x01);
	_hal_dmd_riu_writebyte(MCU2RegBase + 0x1C, 0x01);
	#endif
}

#else
static void _hal_intern_dtmb_initclk(void)
{
//VT	PRINT("------DMD_DTMB_CHIP_NONE------\n");
//printk("\n[%s][%d]\n", __func__, __LINE__);
}
#endif

#else // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

static void _hal_intern_dtmb_initclk(void)
{
//VT	PRINT("------DMD_DTMB_CHIP_NONE------\n");
//printk("\n[%s][%d]\n", __func__, __LINE__);
}

#endif // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

static u8 _hal_intern_dtmb_ready(void)
{
	u8 udata = 0x00;

	_hal_dmd_riu_writebyte(MBRegBase + 0x1E, 0x02);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,	_hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)|0x02);// assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,	_hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)&(~0x02));// de-assert interrupt to VD MCU51

	sys.DelayMS(1);

	udata = _hal_dmd_riu_readbyte(MBRegBase + 0x1E);

	if (udata)
		return FALSE;

	_mbx_readreg(0x20C2, &udata);

	udata = udata & 0x0F;

	if (udata != 0x03)
		return FALSE;

	return TRUE;
}

static u8 _hal_intern_dtmb_download(void)
{
	#if (FW_RUN_ON_DRAM_MODE == 0)
	u8 udata = 0x00;
	u16 i = 0;
	u16 fail_cnt = 0;
	#endif
	u8 utmpdata8;
	u16 uaddressOffset16;
	const u8 *DTMB_table;
	u16 ulib_size16;
	#if (FW_RUN_ON_DRAM_MODE == 1)
	#if DVB_STI
	u8 *va_dramcodeaddr;
	#else
	u64	va_dramcodeaddr;
	#endif
 #endif

	if (pRes->sDMD_DTMB_PriData.bDownloaded) {
	#if POW_SAVE_BY_DRV
	#if !FW_RUN_ON_DRAM_MODE

		_hal_dmd_riu_writebyte(DMDMcuBase+0x01, 0x01);
	//enable	DMD	MCU51	SRAM

	#endif

	_hal_dmd_riu_writebytemask(DMDMcuBase+0x00, 0x00, 0x01);

	sys.DelayMS(1);

	#endif

	if (_hal_intern_dtmb_ready()) {

		_hal_dmd_riu_writebyte(DMDMcuBase+0x00, 0x01);// reset	VD_MCU

		_hal_dmd_riu_writebyte(DMDMcuBase+0x00, 0x00);

		sys.DelayMS(20);

		return TRUE;
	}
	}

	#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	if (pRes->sDMD_DTMB_PriData.uchipid16 == 0x9C) {

		if (pRes->sDMD_DTMB_PriData.eLastType ==
			DMD_DTMB_DEMOD_DTMB_6M) {

			DTMB_table = &INTERN_DTMB_6M_table_Waltz[0];

		ulib_size16 = sizeof(INTERN_DTMB_6M_table_Waltz);
	}	else	{

		DTMB_table = &INTERN_DTMB_table_Waltz[0];

		ulib_size16 = sizeof(INTERN_DTMB_table_Waltz);

	}
	}	else	{
		if (pRes->sDMD_DTMB_PriData.eLastType ==
				DMD_DTMB_DEMOD_DTMB_6M) {

			DTMB_table = &INTERN_DTMB_6M_table[0];

		ulib_size16 = sizeof(INTERN_DTMB_6M_table);

	}	else	{

		DTMB_table = &INTERN_DTMB_table[0];

		ulib_size16 = sizeof(INTERN_DTMB_table);

	}
	}
	#else // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN

	DTMB_table = &INTERN_DTMB_table[0];

	ulib_size16 = sizeof(INTERN_DTMB_table);

	#endif

	_hal_dmd_riu_writebyte(DMDMcuBase+0x00, 0x01);
	// reset	VD_MCU
	_hal_dmd_riu_writebyte(DMDMcuBase+0x01, 0x00);
	// disable	SRAM

	_hal_dmd_riu_writebyte(DMDMcuBase+0x00, 0x00);
	// release	MCU,	madison	patch

	_hal_dmd_riu_writebyte(DMDMcuBase+0x03, 0x50);
	// enable	"vdmcu51_if"
	_hal_dmd_riu_writebyte(DMDMcuBase+0x03, 0x51);
	// enable	auto-increase

	#if (FW_RUN_ON_DRAM_MODE == 0)

	_hal_dmd_riu_writebyte(DMDMcuBase+0x04, 0x00);
	// sram	address	low	byte
	_hal_dmd_riu_writebyte(DMDMcuBase+0x05, 0x00);
	// sram	address	high	byte

	////	Load	code	thru	VDMCU_IF	////
	HAL_INTERN_DTMB_DBINFO(PRINT(">Load	Code...\n"));

	for	(i = 0; i < ulib_size16; i++) {
		_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, DTMB_table[i]);
	//	write	data	to	VD	MCU	51	code	sram
	}

	////	Content	verification	////
	HAL_INTERN_DTMB_DBINFO(PRINT(">Verify	Code...\n"));

	_hal_dmd_riu_writebyte(DMDMcuBase+0x04, 0x00);
	// sram	address	low	byte
	_hal_dmd_riu_writebyte(DMDMcuBase+0x05, 0x00);
	// sram	address	high	byte

	for	(i = 0; i < ulib_size16; i++) {//read sram data
		udata = _hal_dmd_riu_readbyte(DMDMcuBase+0x10);
	//
	if (udata	!=	DTMB_table[i]) {
		HAL_INTERN_DTMB_DBINFO(PRINT(">fail	add = 0x%x\n",	i));
		HAL_INTERN_DTMB_DBINFO(PRINT(">code = 0x%x\n",	DTMB_table[i]));
		HAL_INTERN_DTMB_DBINFO(PRINT(">data = 0x%x\n",	udata));

		if (fail_cnt++ > 10) {
			HAL_INTERN_DTMB_DBINFO(PRINT(" DSP Loadcode fail!"));
			return FALSE;
		}
	}
	}

	uaddressOffset16 = (DTMB_table[0x400] << 8)|DTMB_table[0x401];

	_hal_dmd_riu_writebyte(DMDMcuBase+0x04, (uaddressOffset16&0xFF));
	// sram	address	low	byte
	_hal_dmd_riu_writebyte(DMDMcuBase+0x05, (uaddressOffset16>>8));
	// sram	address	high	byte

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.uif_khz16;
	HAL_INTERN_DTMB_DBINFO(PRINT("uif_khz16=%d\n",
	pRes->sDMD_DTMB_InitData.uif_khz16));
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.uif_khz16 >> 8);
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.bIQSwap;
	HAL_INTERN_DTMB_DBINFO(PRINT("bIQSwap=%d\n",
	pRes->sDMD_DTMB_InitData.bIQSwap));
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.uagc_reference16;
	HAL_INTERN_DTMB_DBINFO(PRINT("uagc_reference16=%X\n",
	pRes->sDMD_DTMB_InitData.uagc_reference16));
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.uagc_reference16 >> 8);
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.utdistartaddr32;
	HAL_INTERN_DTMB_DBINFO(PRINT("utdistartaddr32=%X\n",
	pRes->sDMD_DTMB_InitData.utdistartaddr32));
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 8);
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 16);
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 24);
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write	data	to	VD	MCU	51	code	sram

	utmpdata8 = (u8)pRes->sDMD_DTMB_PriData.eLastType;
	HAL_INTERN_DTMB_DBINFO(PRINT("eLastType=%d\n",
	pRes->sDMD_DTMB_PriData.eLastType));
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);
	// write	data	to	VD	MCU	51	code	sram
	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.btunergaininvert;
	HAL_INTERN_DTMB_DBINFO(PRINT("btunergaininvert=%d\n",
	pRes->sDMD_DTMB_InitData.btunergaininvert));
	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C, utmpdata8);


	#else // #if (FW_RUN_ON_DRAM_MODE == 0)
	//printk("\n dtmb_download / Enter DRAM mode\n");
	//download	DRAM	part
 // VA = MsOS_PA2KSEG1(PA);//NonCache
	HAL_INTERN_DTMB_DBINFO(PRINT(">>DTMB_FW_DRAM_START_ADDR=0x%lx\n", ((
	pRes->sDMD_DTMB_InitData.utdistartaddr32 * 16) + (3 * 1024 * 1024))));

	#if defined	MTK_PROJECT
	va_dramcodeaddr = (u64/*MS_PHY*/)BSP_MapReservedMem((void *)((
	pRes->sDMD_DTMB_InitData.utdistartaddr32 * 16)
	+ (3 * 1024 * 1024)),	1024*1024);
	#elif DVB_STI
	va_dramcodeaddr =
	(pRes->sDMD_DTMB_InitData.virt_dram_base_addr + (3 * 1024 * 1024));
	#else
	va_dramcodeaddr = MsOS_PA2KSEG1(((
	pRes->sDMD_DTMB_InitData.utdistartaddr32 * 16) + (3 * 1024 * 1024)));
	#endif

	#if DVB_STI
	MEMCPY(va_dramcodeaddr, &DTMB_table[0], ulib_size16);
	#else
	MEMCPY((void *)(u32/*MS_VIRT*/)va_dramcodeaddr, &DTMB_table[0],
	ulib_size16);
	#endif

	uaddressOffset16 = (DTMB_table[0x400] << 8)|DTMB_table[0x401];

	#if DVB_STI
	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.uif_khz16;
	HAL_INTERN_DTMB_DBINFO(PRINT("uif_khz16=%d\n",
	pRes->sDMD_DTMB_InitData.uif_khz16));
	MEMCPY((va_dramcodeaddr + uaddressOffset16),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.uif_khz16 >> 8);
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 1),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.bIQSwap;
	HAL_INTERN_DTMB_DBINFO(PRINT("bIQSwap=%d\n",
	pRes->sDMD_DTMB_InitData.bIQSwap));
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 2),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.uagc_reference16;
	HAL_INTERN_DTMB_DBINFO(PRINT("uagc_reference16=%X\n",
	pRes->sDMD_DTMB_InitData.uagc_reference16));
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 3),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.uagc_reference16 >> 8);
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 4),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.utdistartaddr32;
	HAL_INTERN_DTMB_DBINFO(PRINT("utdistartaddr32=%X\n",
	pRes->sDMD_DTMB_InitData.utdistartaddr32));
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 5),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 8);
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 6),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 16);
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 7),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 24);
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 8),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_PriData.eLastType;
	HAL_INTERN_DTMB_DBINFO(PRINT("eLastType=%d\n",
	pRes->sDMD_DTMB_PriData.eLastType));
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 9),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.btunergaininvert;
	HAL_INTERN_DTMB_DBINFO(PRINT("btunergaininvert=%d\n",
	pRes->sDMD_DTMB_InitData.btunergaininvert));
	MEMCPY((va_dramcodeaddr + uaddressOffset16 + 10),
	&utmpdata8, 1);
	#else
	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.uif_khz16;
	HAL_INTERN_DTMB_DBINFO(PRINT("uif_khz16=%d\n",
	pRes->sDMD_DTMB_InitData.uif_khz16));
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.uif_khz16 >> 8);
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 1),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.bIQSwap;
	HAL_INTERN_DTMB_DBINFO(PRINT("bIQSwap=%d\n",
	pRes->sDMD_DTMB_InitData.bIQSwap));
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 2),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.uagc_reference16;
	HAL_INTERN_DTMB_DBINFO(PRINT("uagc_reference16=%X\n",
	pRes->sDMD_DTMB_InitData.uagc_reference16));
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 3),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.uagc_reference16 >> 8);
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 4),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.utdistartaddr32;
	HAL_INTERN_DTMB_DBINFO(PRINT("utdistartaddr32=%X\n",
	pRes->sDMD_DTMB_InitData.utdistartaddr32));
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 5),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 8);
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 6),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 16);
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 7),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 24);
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 8),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_PriData.eLastType;
	HAL_INTERN_DTMB_DBINFO(PRINT("eLastType=%d\n",
	pRes->sDMD_DTMB_PriData.eLastType));
	MEMCPY((void *)(u32/*MS_VIRT*/)(va_dramcodeaddr + uaddressOffset16 + 9),
	&utmpdata8, 1);
 #endif

	#if defined	MTK_PROJECT
	BSP_UnMapReservedMem((void *)(pRes->sDMD_DTMB_InitData.utdistartaddr32
	* 16) + (3 * 1024 * 1024));
	#endif

	HAL_INTERN_DTMB_DBINFO(PRINT(">Load	DRAM	code	done...\n"));
	#endif // #if (FW_RUN_ON_DRAM_MODE == 0)

	_hal_dmd_riu_writebyte(DMDMcuBase+0x03, 0x50);
	// disable	auto-increase
	_hal_dmd_riu_writebyte(DMDMcuBase+0x03, 0x00);
	// disable	"vdmcu51_if"

	_hal_dmd_riu_writebyte(DMDMcuBase+0x00, 0x01);
	// reset	MCU,	madison	patch

	#if GLO_RST_IN_DOWNLOAD
	_hal_dmd_riu_writebyte(0x101E82, 0x00);// reset	demod	0,1
	sys.DelayMS(1);
	_hal_dmd_riu_writebyte(0x101E82, 0x11);
	#endif

	#if (FW_RUN_ON_DRAM_MODE == 0)
	_hal_dmd_riu_writebyte(DMDMcuBase+0x01, 0x01);// enable	SRAM
	#endif
	_hal_dmd_riu_writebyte(DMDMcuBase+0x00, 0x00);// release	VD_MCU

	pRes->sDMD_DTMB_PriData.bDownloaded = TRUE;

	sys.DelayMS(20);

	HAL_INTERN_DTMB_DBINFO(PRINT(">DSP	Loadcode	done."));

	return TRUE;
}

static void _hal_intern_dtmb_fwversion(void)
{
	u8 data1 = 0;
	u8 data2 = 0;
	u8 data3 = 0;

	_mbx_readreg(0x20C4, &data1);
	_mbx_readreg(0x20C5, &data2);
	_mbx_readreg(0x20C6, &data3);

	PRINT("INTERN_DTMB_FW_VERSION:%x.%x.%x\n",	data1,	data2,	data3);
}

static u8 _hal_intern_dtmb_exit(void)
{
	u8 ucheckcount8 = 0;
	u16 temp = 0;

	_hal_dmd_riu_writebyte(MBRegBase + 0x1C, 0x01);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,	_hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)|0x02);// assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,	_hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)&(~0x02));// de-assert interrupt to VD MCU51

	while ((_hal_dmd_riu_readbyte(MBRegBase + 0x1C)&0x02) != 0x02) {
		sys.DelayMS(1);

	if (ucheckcount8++ == 0xFF) {
		PRINT(">> DTMB Exit	Fail!\n");
		return FALSE;
	}
	}

	PRINT(">> DTMB Exit	Ok!\n");

	#if POW_SAVE_BY_DRV

	#if DVB_STI
	//printk("\nreset_base_addr = 0x%X\n", pRes->sDMD_DTMB_PriData.virt_reset_base_addr);

	//writel(0x00000000, pRes->sDMD_DTMB_PriData.virt_sram_base_addr);
	writel(0x00000000, pRes->sDMD_DTMB_PriData.virt_reset_base_addr);
	sys.DelayMS(1);
	writel(0x00000011, pRes->sDMD_DTMB_PriData.virt_reset_base_addr);
//For M6-E1 SMI CLK EN
	temp = readw(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + REG_LARB12_CLK1);
	temp |= 0x1 << BIT_6;
	writew(temp, pRes->sDMD_DTMB_PriData.virt_clk_base_addr + REG_LARB12_CLK1);

	temp = readw(pRes->sDMD_DTMB_PriData.virt_clk_base_addr + REG_LARB12_CLK2);
	temp &= ~(0x1 << BIT_8);
	writew(temp, pRes->sDMD_DTMB_PriData.virt_clk_base_addr + REG_LARB12_CLK2);
//For M6-E1 SMI CLK EN END
	#else
	_hal_dmd_riu_writebyte(DEMOD_GROUP_RSTZ,	DEMOD_RSTZ_EN);

	sys.DelayMS(1);

	_hal_dmd_riu_writebyte(DEMOD_GROUP_RSTZ,	DEMOD_RSTZ_DIS);
	#endif
	#endif

	return TRUE;
}

static u8 _hal_intern_dtmb_softreset(void)
{
	u8 udata8 = 0;

	//Reset	FSM
	if (_mbx_writereg(0x20C0, 0x00) == FALSE)
		return FALSE;

	while (udata8 != 0x02) {
		if (_mbx_readreg(0x20C1, &udata8) == FALSE)
			return FALSE;
	}

	return TRUE;
}

//static u8 _hal_intern_dtmb_setacicoef(void)
//{
//	return TRUE;
//}

static u8 _hal_intern_dtmb_setdtmbmode(void)
{
	if (_mbx_writereg(0x20C2, 0x03) == FALSE)
		return FALSE;
		return _mbx_writereg(0x20C0, 0x04);
}

static u8 _hal_intern_dtmb_setmodeclean(void)
{
	if (_mbx_writereg(0x20C2, 0x07) == FALSE)
		return FALSE;
		return _mbx_writereg(0x20C0, 0x00);
}

static u8 _hal_intern_dtmb_set_qam_sr(void)
{
	if (_mbx_writereg(0x20C2, 0x01) == FALSE)
		return FALSE;
		return _mbx_writereg(0x20C0, 0x04);
}

//static u8 _hal_intern_dtmb_agclock(void)
//{
//	return TRUE;
//}

static u8 _hal_intern_dtmb_pnp_lock(void)
{
	u8 data = 0;
	u8 data1 = 0;

	#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	if (pRes->sDMD_DTMB_PriData.uchipid16 == 0x9C) {
		_mbx_readreg(0x3BBA, &data);
		_mbx_readreg(0x3C49, &data1);//	CFO_FFT_SEC_VALID
	}	else	{
		_mbx_readreg(DTMB_PNM_REG, &data);
		_mbx_readreg(DTMB_PNP_REG, &data1);//	CFO_FFT_SEC_VALID
	}
	#else // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	_mbx_readreg(DTMB_PNM_REG, &data);
	_mbx_readreg(DTMB_PNP_REG, &data1);//	CFO_FFT_SEC_VALID
	#endif // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN

	if (((data&0x02) == 0x02) && ((data1&0x20) == 0x20))
		return TRUE;
		else
			return FALSE;
}

static u8 _hal_intern_dtmb_fec_lock(void)
{
	u8 ustate8 = 0,	ustatus8 = 0;


	_mbx_readreg(0x20C1, &ustate8);
	_mbx_readreg(0x20C3, &ustatus8);

	if ((((ustate8 == 0x62) && !(ustatus8&0x80)) || (ustate8	> 0x62))
		&& (ustate8	<= 0xF0)) {
		HAL_INTERN_DTMB_DBINFO(PRINT("DMD_DTMB_FEC_LOCK:0x%x\n", ustate8));
	return TRUE;
		}
	else	{
		HAL_INTERN_DTMB_DBINFO(PRINT("DMD_DTMB_FEC_UNLOCK:0x%x\n", ustate8));
	return FALSE;
	}
}

static u8 _hal_intern_dtmb_getmodulation(
struct DMD_DTMB_MODULATION_INFO *psDtmbGetModulation)
{
	u8 cm,	qam,	il,	cr,	sinr;
	u8 data_l = 0;
	u8 data_h = 0;
	u8 data_1 = 0;
	u8 data_2 = 0;

	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB) {// ||
//	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_7M ||
//	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_6M ||
//	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_5M) {
	#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
		if (pRes->sDMD_DTMB_PriData.uchipid16 == 0x9C) {
			_mbx_readreg(0x3B90, &data_l);
		_mbx_readreg(0x3B91, &data_h);
		_mbx_readreg(DTMB_PND_REG, &data_1);
		_mbx_readreg(DTMB_PNM_REG, &data_2);
	}	else	{
		_mbx_readreg(DTMB_F_SI_L_REG, &data_l);
		_mbx_readreg(DTMB_F_SI_H_REG, &data_h);
		_mbx_readreg(DTMB_PND_REG, &data_1);
		_mbx_readreg(DTMB_PNM_REG, &data_2);
	}
	#else // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	_mbx_readreg(DTMB_F_SI_L_REG, &data_l);
	_mbx_readreg(DTMB_F_SI_H_REG, &data_h);
	_mbx_readreg(DTMB_PND_REG, &data_1);
	_mbx_readreg(DTMB_PNM_REG, &data_2);
	#endif // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN

	if (data_l & 0x1) {
		cr = (data_l >> 6) & 0x03;
		il = (data_l >> 3) & 0x01;
		qam = (data_l >> 4) & 0x03;
		sinr = (data_l >> 2) & 0x01;
		cm = (data_l >> 1) & 0x01;
	}	else	{
		#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
		if (pRes->sDMD_DTMB_PriData.uchipid16 == 0x9C) {
			_mbx_readreg(0x3B9E, &data_l);
		_mbx_readreg(0x3B9F, &data_h);
		}	else	{
			_mbx_readreg(DTMB_SI_L_REG, &data_l);
			_mbx_readreg(DTMB_SI_H_REG, &data_h);
		}
		#else // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
		_mbx_readreg(DTMB_SI_L_REG, &data_l);
		_mbx_readreg(DTMB_SI_H_REG, &data_h);
		#endif // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN

		cr = (data_h >> 4) & 0x03;
		il = (data_h >> 6) & 0x01;
		qam = (data_h >> 2) & 0x03;
		sinr = (data_h >> 1) & 0x01;
		cm = (data_h) & 0x01;
	}
	if ((data_1&0x03) == 0)
		psDtmbGetModulation->upnm16 = 420;
	else	if ((data_1&0x03) == 1)
		psDtmbGetModulation->upnm16 = 595;
	else
		psDtmbGetModulation->upnm16 = 945;

	if ((data_2&0x10) == 0x10)
		psDtmbGetModulation->upnc8 = 1;
	else
		psDtmbGetModulation->upnc8 = 0;
	#ifdef	UTPA2
	if (cr == 0)
		psDtmbGetModulation->fsicoderate = 4;
	else	if (cr == 1)
		psDtmbGetModulation->fsicoderate = 6;
	else	if (cr == 2)
		psDtmbGetModulation->fsicoderate = 8;
	#else
	if (cr == 0)
		psDtmbGetModulation->fsicoderate = 0.4;
	else	if (cr == 1)
		psDtmbGetModulation->fsicoderate = 0.6;
	else	if (cr == 2)
		psDtmbGetModulation->fsicoderate = 0.8;
	#endif

	if (il == 0)
		psDtmbGetModulation->usiinterleaver16 = 240;
	else
		psDtmbGetModulation->usiinterleaver16 = 720;

	if (qam == 0)
		psDtmbGetModulation->usiqammode8 = 4;
	else	if (qam == 1)
		psDtmbGetModulation->usiqammode8 = 16;
	else	if (qam == 2)
		psDtmbGetModulation->usiqammode8 = 32;
	else	if (qam == 3)
		psDtmbGetModulation->usiqammode8 = 64;

	psDtmbGetModulation->usicarriermode8 = cm;// 0:Multi,	1:Single
	psDtmbGetModulation->usinr8 = sinr;
	}	else	{
	}

	return TRUE;
}

//static u8 _hal_intern_dtmb_readifagc(void)
//{
//	u8 data = 0;

//	#if USE_T2_MERGED_FEND
//	_mbx_readreg(0x280F, &data);
//	#else
//	_mbx_readreg(0x28FD, &data);
//	#endif

//	return data;
//}

#ifdef	UTPA2
static u8 _hal_intern_dtmb_readfrequencyoffset(s16 *pfftfirstcfo,
	s8 *pfftsecondcfo, s16 *psr)
#else
static s16	_hal_intern_dtmb_readfrequencyoffset(void)
#endif
{
	u8 udata8 = 0;
	s16	fftfirstcfo = 0;
	s8	fftsecondcfo = 0;
	s16	sr = 0;

	#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	if (pRes->sDMD_DTMB_PriData.uchipid16 == 0x9C) {
		_mbx_readreg(0x3C4D, &udata8);
		fftfirstcfo = udata8;
		_mbx_readreg(0x3C4C, &udata8);
		fftfirstcfo = (fftfirstcfo<<8)|udata8;

	_mbx_readreg(0x3C50, &udata8);
	fftsecondcfo = udata8;
	}	else	{
		_mbx_readreg(DTMB_F_CFO_H_REG, &udata8);
		fftfirstcfo = udata8;
		_mbx_readreg(DTMB_F_CFO_L_REG, &udata8);
		fftfirstcfo = (fftfirstcfo<<8)|udata8;

		_mbx_readreg(DTMB_S_CFO_REG, &udata8);
		fftsecondcfo = udata8;
	}
	#else // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	_mbx_readreg(DTMB_F_CFO_H_REG, &udata8);
	fftfirstcfo = udata8;
	_mbx_readreg(DTMB_F_CFO_L_REG, &udata8);
	fftfirstcfo = (fftfirstcfo<<8)|udata8;

	_mbx_readreg(DTMB_S_CFO_REG, &udata8);
	fftsecondcfo = udata8;
	#endif // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN

	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_6M)
		sr = 5670;
	else
		sr = 7560;

	#ifdef	UTPA2
	*pfftfirstcfo = fftfirstcfo;
	*pfftsecondcfo = fftsecondcfo;
	*psr	= sr;

	return TRUE;
	#else
	return (s16)((((double)fftfirstcfo/0x10000+
	(double)fftsecondcfo/0x20000))*(double)sr);
	#endif
}


static u8 _hal_intern_dtmb_readsnrpercentage(void)
{
	u8 data = 0;
	u8 level = 0;
	u32 snr = 0;
	s8 i = 0, j = 0;

	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB) {// ||
//	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_7M ||
//	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_6M ||
//	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_5M) {
		if (!_hal_intern_dtmb_fec_lock())
			level = 0;
	else	{
		#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
		if (pRes->sDMD_DTMB_PriData.uchipid16 == 0x9C) {
			_mbx_readreg(0x3BDA, &data);
			snr = data&0x3F;
			_mbx_readreg(0x3BD9, &data);
			snr = (snr<<8)|data;
			_mbx_readreg(0x3BD8, &data);
			snr = (snr<<8)|data;
		}	else	{
			_mbx_readreg(DTMB_SNR_2_REG, &data);
			snr = data&0x3F;
			_mbx_readreg(DTMB_SNR_1_REG, &data);
			snr = (snr<<8)|data;
			_mbx_readreg(DTMB_SNR_0_REG, &data);
			snr = (snr<<8)|data;
		}
		#else // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
		_mbx_readreg(DTMB_SNR_2_REG, &data);
		snr = data&0x3F;
		_mbx_readreg(DTMB_SNR_1_REG, &data);
		snr = (snr<<8)|data;
		_mbx_readreg(DTMB_SNR_0_REG, &data);
		snr = (snr<<8)|data;
		#endif // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
//PRINT("DTMB_SNR_2_REG=0x%x\n", DTMB_SNR_2_REG);
//PRINT("DTMB_SNR_1_REG=0x%x\n", DTMB_SNR_1_REG);
//PRINT("DTMB_SNR_0_REG=0x%x\n", DTMB_SNR_0_REG);
//PRINT("snr=%d\n", snr);
//=============new snr start:
	j = sizeof(_SNR_TABLE) / sizeof(u32);//50

	for (i = j-2; i >= 0 ; i--) {
		if (snr > _SNR_TABLE[i]) {
			level = i+2;
		break;
	}
	}
	//=============new snr end:
	}
	}	else	{
		level = 0;
	}

	return level*2;
}

s16 _hal_intern_dtmb_nordig_get_cn(void)
{
		struct DMD_DTMB_MODULATION_INFO sDtmbModulationMode;
		u8 ret1 = 0;
		u8 i = 0, j = 0, k = 0;

		s16 stemp16 = 0;
		//MC/SC, QAM, CR
		s16 cn[6][3] = {
			{34, 52, 74},
			{93, 112, 138},
			{139, 164, 197},
			{34, 52, 73},
			{87, 110, 137},
			{131, 160, 194}
		};

	ret1 = _mdrv_dmd_dtmb_md_getmodulationmode(0, &sDtmbModulationMode);

	switch (sDtmbModulationMode.usicarriermode8)	{
	case 0:
			i = 0; break;
	case 1:
			i = 1; break;
	default:
			i = 0; break;
	}

	switch (sDtmbModulationMode.usiqammode8/*psDtmbDemodCtx->u1Mod*/) {
	case 4:
			j = 0; break;
	case 16:
			j = 1; break;

	case 64:
			j = 2; break;
	default:
			j = 0; break;
	}

	switch (sDtmbModulationMode.fsicoderate) {
	case 4:
		k = 0; break;
	case 6:
		k = 1; break;
	case 8:
		k = 2; break;
	default:
		k = 0; break;
	}

	if (sDtmbModulationMode.usinr8)
		return 44;

	if ((sDtmbModulationMode.usiqammode8 == 32) && (i == 0))
		return 171;

	else if ((sDtmbModulationMode.usiqammode8 == 32) && (i == 1))
		return 167;

	stemp16 = cn[i*3+j][k];

	return stemp16;
}

static const u32 _logapproxtablex[58] = {
	1, 2, 3, 4, 6, 8, 10, 13, 17, 23, //10
	30, 39, 51, 66, 86, 112, 146, 190, 247, 321, //10
	417, 542, 705, 917, 1192, 1550, 2015, 2620, 3405, 4427, //10
	5756, 7482, 9727, 12646, 16440, 21372, 27783, 36118, 46954, 61040, //10
	79353, 103159, 134106, 174338, 226640, 294632, 383022, 497929, 647307,
	841500, 1093950, 1422135, 1848776, 2403409, 3124432, 4061761, 5280290,
	6160384
};

static const u16 _logapproxtabley[58] = {
	0,  30,  47,  60,  77,  90, 100, 111, 123, 136,
	147, 159, 170, 181, 193, 204, 216, 227, 239, 250,
	262, 273, 284, 296, 307, 319, 330, 341, 353, 364,
	376, 387, 398, 410, 421, 432, 444, 455, 467, 478,
	489, 501, 512, 524, 535, 546, 558, 569, 581, 592,
	603, 615, 626, 638, 649, 660, 672, 678
};

u16 intern_dtmb_log10approx(u32 flt_x)
{
	u8 indx = 0;

	do {
		if (flt_x < _logapproxtablex[indx])
			break;
		indx++;
	} while (indx < 57);//stop at indx = 58

	return _logapproxtabley[indx];
}

s16 intern_dtmb_nordig_get_pref(void)
{
	struct DMD_DTMB_MODULATION_INFO sDtmbModulationMode;
	u8 ret1 = 0;
	u8 i = 0, j = 0, k = 0;

	s16 stemp16 = 0;
	//MC/SC, QAM, CR
	s16 pref[6][3] = {
	{-964, -946, -924},
	{-905, -886, -860},
	{-859, -834, -801},
	{-964, -946, -925},
	{-911, -888, -861},
	{-867, -838, -804}
	};

	ret1 = _mdrv_dmd_dtmb_md_getmodulationmode(0, &sDtmbModulationMode);
	switch (sDtmbModulationMode.usicarriermode8) {
	case 0/*CarrierMode_MC*/:
			i = 0; break;
	case 1/*CarrierMode_SC*/:
			i = 1; break;
	default:
			i = 0; break;
	}

	switch (sDtmbModulationMode.usiqammode8) {
	case 4:
			j = 0; break;
	case 16:
			j = 1; break;
		//case 32:  j = 2; break;
	case 64:
			j = 2; break;
	default:
		j = 0; break;
	}

	switch (sDtmbModulationMode.fsicoderate)	{
	case 4/*FecRateLow*/:
			k = 0; break;
	case 6/*FecRateMiddle*/:
			k = 1; break;
	case 8/*FecRateHigh*/:
			k = 2; break;
	default:
			k = 0; break;
	}

	if (sDtmbModulationMode.usinr8) {
		stemp16 = -954;
	return stemp16;
	}

	if ((sDtmbModulationMode.usiqammode8 == 32) && (i == 0)) {
		stemp16 = -827;

	return stemp16;
	}

	else if ((sDtmbModulationMode.usiqammode8 == 32) && (i == 1)) {
		stemp16 = -831;
	return stemp16;
	}

	stemp16 = pref[i*3+j][k];

	return stemp16;
}

u16 _hal_intern_dtmb_nordig_get_ber_sqi(void)
{
	u8 ret1 = 0;
	u16 ber_sqi = 0, utemp161 = 0, utemp162 = 0;
	u8	udata81 = 0, udata82 = 0;
	u32 upreldpcbertempdata32 = 0;

	ret1 &= _mbx_readreg(0x20C1, &udata81);

	if (udata81	== 0x62 || udata81	== 0x63) {
		sys.DelayMS(300);
		ret1 &= _mbx_readreg(0x20C1, &udata81);
	}


	ret1 &= _mbx_readreg(0x3364, &udata81);
	ret1 &= _mbx_readreg(0x3365, &udata82);
	utemp161 = (u16) ((udata82<<8)|udata81);

	ret1 &= _mbx_readreg(0x3366, &udata81);
	ret1 &= _mbx_readreg(0x3367, &udata82);
	utemp162 = (u16) ((udata82<<8)|udata81);

	upreldpcbertempdata32 = (u32) ((utemp162<<16)|utemp161);

	ret1 &= _mbx_readreg(0x3324, &udata81);
	ret1 &= _mbx_readreg(0x3325, &udata82);
	utemp161 = (u16) ((udata82<<8)|udata81);

	upreldpcbertempdata32 = (u32) (upreldpcbertempdata32*26083/100000);

	if (upreldpcbertempdata32 > 25000)//(upreldpcbertempdata32/3000>=10)
		ber_sqi = 0;

	else if (((upreldpcbertempdata32*10+1500)/3000) <= 1)
		ber_sqi = 100;

	else
		ber_sqi = (u16) (intern_dtmb_log10approx((
	1000000/upreldpcbertempdata32)*8000))/5-40;

	return (u16) (ber_sqi);
}


static u8 _hal_intern_dtmb_getssi(void)
{

	s16 stemp161 = 0, stemp162 = 0, spref16 = 0;

//	ITuner_OP(ITunerGetCtx(), itGetRSSI, 0, &stemp161);
	stemp162 = ((stemp161-50)/100);//dbm

	spref16 = intern_dtmb_nordig_get_pref();

	stemp161 = (s16)((s16) stemp162 - (s16)(spref16/10));

	if	(stemp161 < -15)
		stemp162 = 0;

	else if (stemp161 < 0)
		stemp162 = (s16) (2*(stemp161+15)/3);

	else if (stemp161 < 20)
		stemp162 = (s16) ((stemp161*4+10));

	else if (stemp161 < 35)
		stemp162 = (s16) (2*(stemp161-20)/3+90);

	else
		stemp162 = 100;

return stemp162;
}


static u8 _hal_intern_dtmb_getagc(void)
{
	u8 ifagc = 0, udata81 = 0;
	u8 ret = 0;

	ret = _mbx_readreg(0x2822, &udata81);
	udata81 = udata81|0x03;// choose IF Gain
	ret = _mbx_writereg(0x2822, udata81);
	//ret = _mbx_readreg(0x2824, &udata81);
	//ret = _mbx_readreg(0x2825, &udata82);
	//utemp16 = (UINT16) (udata82<<8)|(udata81);
	ret = _mbx_readreg(0x2825, &ifagc);
	//ifagcgain = (u8)(255*utemp16/65536);
	ret = _mbx_readreg(0x2810, &udata81);
	if (udata81&0x08)
		ifagc = 255 - ifagc;


return ifagc;
}

static u16 _hal_intern_dtmb_geticp(void)
{
		//!Incorrect Packets porting
	u8 ret = 0;
	u16 icp = 0;
	u8 udata81 = 0, udata82 = 0;

	ret &= _mbx_readreg(0x20C1, &udata81);
	if (udata81 == 0xF0) {
		ret = _mbx_readreg(0x3332, &udata81);
		ret = _mbx_readreg(0x3333, &udata82);
		icp = (u16) (udata82<<8)|udata81;
	} else
		icp = 0;

	return icp;
}

static u8 _hal_intern_dtmb_getsqi(void)
{
	u8 snrqty = 0, utemp8 = 0;
	s16 scnpref16 = 0, stemp16 = 0, sber_sqi16 = 0;

	snrqty = (u8) _hal_intern_dtmb_readsnrpercentage();
	scnpref16 = (s16) _hal_intern_dtmb_nordig_get_cn();
	sber_sqi16 = (s16) _hal_intern_dtmb_nordig_get_ber_sqi();

	stemp16 = (s16) (((s16)(snrqty*3) - (s16) scnpref16 + 5)/10);


	if (stemp16 < -7)
		utemp8 = 0;

	else if (stemp16 < 3)
		utemp8 = (u8) (((stemp16-3)+10) * sber_sqi16/10);

	else
		utemp8 = (u8) sber_sqi16;

return utemp8;
}


#ifdef	UTPA2
static u8 _hal_intern_dtmb_getpreldpcber(u32 *pbiterr,
	u16 *perror_window)
#else
static u8 _hal_intern_dtmb_getpreldpcber(float	*pber)
#endif
{
	u8 udata8 = 0;
	u32 biterr;
	u16 error_window;

	#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	if (pRes->sDMD_DTMB_PriData.uchipid16 == 0x9C) {
		_mbx_readreg(0x3F3B, &udata8);
		biterr = udata8;
		_mbx_readreg(0x3F3A, &udata8);
		biterr = (biterr << 8)|udata8;
		_mbx_readreg(0x3F39, &udata8);
		biterr = (biterr << 8)|udata8;
		_mbx_readreg(0x3F38, &udata8);
		biterr = (biterr << 8)|udata8;
	}	else	{
		_mbx_readreg(DTMB_BER_ERR_3_REG, &udata8);
		biterr = udata8;
		_mbx_readreg(DTMB_BER_ERR_2_REG, &udata8);
		biterr = (biterr << 8)|udata8;
		_mbx_readreg(DTMB_BER_ERR_1_REG, &udata8);
		biterr = (biterr << 8)|udata8;
		_mbx_readreg(DTMB_BER_ERR_0_REG, &udata8);
		biterr = (biterr << 8)|udata8;
	}
	#else // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	_mbx_readreg(DTMB_BER_ERR_3_REG, &udata8);
	biterr = udata8;
	_mbx_readreg(DTMB_BER_ERR_2_REG, &udata8);
	biterr = (biterr << 8)|udata8;
	_mbx_readreg(DTMB_BER_ERR_1_REG, &udata8);
	biterr = (biterr << 8)|udata8;
	_mbx_readreg(DTMB_BER_ERR_0_REG, &udata8);
	biterr = (biterr << 8)|udata8;
	#endif // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN

	#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	if (pRes->sDMD_DTMB_PriData.uchipid16 == 0x9C) {
		_mbx_readreg(0x3F2F, &udata8);
		error_window = udata8;
		_mbx_readreg(0x3F2E, &udata8);
		error_window = (error_window << 8)|udata8;
	}	else	{
		_mbx_readreg(DTMB_BER_WIN_H_REG, &udata8);
		error_window = udata8;
		_mbx_readreg(DTMB_BER_WIN_L_REG, &udata8);
		error_window = (error_window << 8)|udata8;
	}
	#else // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
	_mbx_readreg(DTMB_BER_WIN_H_REG, &udata8);
	error_window = udata8;
	_mbx_readreg(DTMB_BER_WIN_L_REG, &udata8);
	error_window = (error_window << 8)|udata8;
	#endif // #if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN

	#ifdef	UTPA2
	*pbiterr = biterr;
	*perror_window = error_window;
	#else
	*pber = (float)biterr/7488.0/(float)error_window;
	#endif

	return TRUE;
}

static u8 _hal_intern_dtmb_getreg(u16 uaddr16,	u8 *pudata8)
{
	return _mbx_readreg(uaddr16,	pudata8);
}

static u8 _hal_intern_dtmb_setreg(u16 uaddr16,	u8 udata8)
{
	return _mbx_writereg(uaddr16,	udata8);
}

//---------------------------
//	Global	Functions
//---------------------------
u8	hal_intern_dtmb_ioctl_cmd(struct DTMB_IOCTL_DATA *pData,
	enum DMD_DTMB_HAL_COMMAND eCmd, void *pArgs)
{
	u8 bResult = TRUE;

	id = pData->id;
 // printk(KERN_INFO"123:pData->pRes=0x%p",	pData->pRes);
	pRes = pData->pRes;

	sys = pData->sys;

	switch	(eCmd) {
	case	DMD_DTMB_HAL_CMD_Exit:
	bResult = _hal_intern_dtmb_exit();
	break;
	case	DMD_DTMB_HAL_CMD_InitClk:

	_hal_intern_dtmb_initclk();

	break;
	case	DMD_DTMB_HAL_CMD_Download:

	bResult = _hal_intern_dtmb_download();

	break;
	case	DMD_DTMB_HAL_CMD_FWVERSION:
	_hal_intern_dtmb_fwversion();
	break;
	case	DMD_DTMB_HAL_CMD_SoftReset:
	bResult = _hal_intern_dtmb_softreset();
	break;
//	case	DMD_DTMB_HAL_CMD_SetACICoef:
//	bResult = _hal_intern_dtmb_setacicoef();
//	break;
	case	DMD_DTMB_HAL_CMD_SetDTMBMode:
	bResult = _hal_intern_dtmb_setdtmbmode();
	break;
	case	DMD_DTMB_HAL_CMD_SetModeClean:
	bResult = _hal_intern_dtmb_setmodeclean();
	break;
	case	DMD_DTMB_HAL_CMD_Set_QAM_SR:
	bResult = _hal_intern_dtmb_set_qam_sr();
	break;
	case	DMD_DTMB_HAL_CMD_Active:
	break;
//	case	DMD_DTMB_HAL_CMD_AGCLock:
//	bResult = _hal_intern_dtmb_agclock();
//	break;
	case	DMD_DTMB_HAL_CMD_DTMB_PNP_Lock:
	bResult = _hal_intern_dtmb_pnp_lock();
	break;
	case	DMD_DTMB_HAL_CMD_DTMB_FEC_Lock:
	bResult = _hal_intern_dtmb_fec_lock();
	break;
	case	DMD_DTMB_HAL_CMD_DVBC_PreLock:
	break;
	case	DMD_DTMB_HAL_CMD_DVBC_Main_Lock:
	break;
	case	DMD_DTMB_HAL_CMD_GetModulation:
	bResult = _hal_intern_dtmb_getmodulation((
	struct DMD_DTMB_MODULATION_INFO *)	pArgs);
	break;
//	case	DMD_DTMB_HAL_CMD_ReadIFAGC:
//	*((u16 *)pArgs) = _hal_intern_dtmb_readifagc();
//	break;
	case	DMD_DTMB_HAL_CMD_ReadFrequencyOffset:
	#ifdef	UTPA2
	bResult = _hal_intern_dtmb_readfrequencyoffset(&((*((
	struct DMD_DTMB_CFO_DATA *)	pArgs)).fftfirstcfo),
	&((*((struct DMD_DTMB_CFO_DATA *)pArgs)).fftsecondcfo),
	&((*((struct DMD_DTMB_CFO_DATA	*)pArgs)).sr));
	#else
	*((s16	*)pArgs) = _hal_intern_dtmb_readfrequencyoffset();
	#endif
	break;
	case	DMD_DTMB_HAL_CMD_ReadSNRPercentage:
	*((u8 *)pArgs) = _hal_intern_dtmb_readsnrpercentage();
	break;
		case	DMD_DTMB_HAL_CMD_IFAGC:
	*((u8 *)pArgs) = _hal_intern_dtmb_getagc();
	break;
		case	DMD_DTMB_HAL_CMD_ICP:
		*((u16 *)pArgs) = _hal_intern_dtmb_geticp();
	break;
	case	DMD_DTMB_HAL_CMD_SQI:
		*((u8 *)pArgs) = _hal_intern_dtmb_getsqi();
	break;
		case	DMD_DTMB_HAL_CMD_SSI:
		*((u8 *)pArgs) = _hal_intern_dtmb_getssi();
	break;
	case	DMD_DTMB_HAL_CMD_GetPreLdpcBer:
	#ifdef	UTPA2
	bResult = _hal_intern_dtmb_getpreldpcber(&((*((
	struct DMD_DTMB_BER_DATA *) pArgs)).biterr), &(
	(*((struct DMD_DTMB_BER_DATA *)pArgs)).error_window));
	#else
	bResult = _hal_intern_dtmb_getpreldpcber((float	*)pArgs);
	#endif
	break;
	case	DMD_DTMB_HAL_CMD_GetPreViterbiBer:
	break;
	case	DMD_DTMB_HAL_CMD_GetPostViterbiBer:
	break;
	case	DMD_DTMB_HAL_CMD_GetSNR:
	break;
	case	DMD_DTMB_HAL_CMD_TS_INTERFACE_CONFIG:
	break;
	case	DMD_DTMB_HAL_CMD_IIC_Bypass_Mode:
	break;
	case	DMD_DTMB_HAL_CMD_SSPI_TO_GPIO:
	break;
	case	DMD_DTMB_HAL_CMD_GPIO_GET_LEVEL:
	break;
	case	DMD_DTMB_HAL_CMD_GPIO_SET_LEVEL:
	break;
	case	DMD_DTMB_HAL_CMD_GPIO_OUT_ENABLE:
	break;
	case	DMD_DTMB_HAL_CMD_DoIQSwap:
	break;
	case	DMD_DTMB_HAL_CMD_GET_REG:
	bResult = _hal_intern_dtmb_getreg((*((struct DMD_DTMB_REG_DATA *)
	pArgs)).uaddr16, &((*((struct DMD_DTMB_REG_DATA *)pArgs)).udata8));
	break;
	case	DMD_DTMB_HAL_CMD_SET_REG:
	bResult = _hal_intern_dtmb_setreg((*((struct DMD_DTMB_REG_DATA *)
	pArgs)).uaddr16, (*((struct DMD_DTMB_REG_DATA *)pArgs)).udata8);
	break;
	default:
	break;
	}

	return bResult;
}

u8	mdrv_dmd_dtmb_initial_hal_interface(void)
{
	return TRUE;
}

#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG)\
&& defined(CONFIG_MSTAR_CHIP)
EXPORT_SYMBOL(hal_intern_dtmb_ioctl_cmd);
#endif
