// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//---------------------------
//	Include	Files
//---------------------------

#include	"halDMD_INTERN_DTMB.h"

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


#define	DMD_DTMB_CHIP_M7332 0x00


#if defined(CHIP_M7332)
	#define	DMD_DTMB_CHIP_VERSION DMD_DTMB_CHIP_M7332
#else
	#define	DMD_DTMB_CHIP_VERSION DMD_DTMB_CHIP_M7332
#endif

#endif // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

//---------------------------
//	Local	Defines
//---------------------------

#if defined	MTK_PROJECT
#define	_RIU_READ_BYTE(addr)\
((addr)&0x01 ? ((u8)((IO_READ32((addr&0x00FFFF00),\
(addr&0x000000FE)<<1))>>8)) : ((u8)((IO_READ32((addr&0x00FFFF00),\
(addr&0x000000FE)<<1))&0x000000FF)))
#define _RIU_WRITE_BYTE(addr,	val) do { if ((addr)&0x01) IO_WRITE32((
	addr&0x00FFFF00), (addr&0x000000FE)<<1, (IO_READ32((addr&0x00FFFF00),
	(addr&0x000000FE)<<1)&0xFFFF00FF)|((((u32)val)<<8)));
	else
		IO_WRITE32((addr&0x00FFFF00), (addr&0x000000FE)<<1, (IO_READ32((
	addr&0x00FFFF00), (addr&0x000000FE)<<1)&0xFFFFFF00)|((u32)val)); }
	while (0)
#elif	DVB_STI
#define	_RIU_READ_BYTE(addr)\
(readb(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (addr)))
#define	_RIU_WRITE_BYTE(addr, val)\
(writeb(val, pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (addr)))

#elif	DMD_DTMB_3PARTY_EN
// Please add riu read&write define according to 3 perty platform
#define	_RIU_READ_BYTE(addr)
#define	_RIU_WRITE_BYTE(addr,	val)

#else

#define	_RIU_READ_BYTE(addr)\
(READ_BYTE(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (addr)))
#define	_RIU_WRITE_BYTE(addr, val)\
(WRITE_BYTE(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (addr), val))
#endif

#define	USE_DEFAULT_CHECK_TIME	1

#define	HAL_INTERN_DTMB_DBINFO(y)	y

#ifndef	MBRegBase
	#ifdef	MTK_PROJECT
	#define	MBRegBase	0x10505E00
	#else
	#define	MBRegBase	0x112600
	#endif
#endif
#ifndef	DMDMcuBase
	#ifdef	MTK_PROJECT
	#define	DMDMcuBase	0x10527D00
	#else
	#define	DMDMcuBase	0x103480
	#endif
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
	#define	GLO_RST_IN_DOWNLOAD	1
//#else
//	#define	GLO_RST_IN_DOWNLOAD	0
//#endif

#define	FW_RUN_ON_DRAM_MODE	0
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

static u8 id;

static struct DMD_DTMB_ResData	*pRes;

static struct DMD_DTMB_SYS_PTR	sys;

#if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE || DVB_STI
const u8 INTERN_DTMB_table[] = {
	#include	"DMD_INTERN_DTMB.dat"
};

const u8 INTERN_DTMB_6M_table[] = {
	#include	"DMD_INTERN_DTMB_6M.dat"
};
const u8 *INTERN_DTMB_table_Waltz = INTERN_DTMB_table;
const u8 *INTERN_DTMB_6M_table_Waltz = INTERN_DTMB_6M_table;

#else // #if !DMD_DTMB_3PARTY_EN || defined MTK_PROJECT || BIN_RELEASE
// Please set fw dat file used by 3 perty platform
const u8 INTERN_DTMB_table[] = {
	#include	"DMD_INTERN_DTMB.dat"
};
#endif // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

static u8 _ACI_COEF_TABLE_FS24M_SR8M[DTMB_ACI_COEF_SIZE] = {
0x80, 0x06, 0x9f, 0xf4, 0x9f, 0xe8, 0x9f, 0xf0, 0x80, 0x09, 0x80, 0x1f, 0x80,
0x1d, 0x80, 0x03, 0x9f, 0xe3, 0x9f, 0xdc, 0x9f, 0xf7, 0x80, 0x1d, 0x80, 0x2c,
0x80, 0x12, 0x9f, 0xe2, 0x9f, 0xc9, 0x9f, 0xe2, 0x80, 0x1a, 0x80, 0x42, 0x80,
0x2f, 0x9f, 0xeb, 0x9f, 0xb2, 0x9f, 0xbe, 0x80, 0x0c, 0x80, 0x5b, 0x80, 0x5e,
0x80, 0x05, 0x9f, 0x9a, 0x9f, 0x81, 0x9f, 0xdf, 0x80, 0x6c, 0x80, 0xa7, 0x80,
0x45, 0x9f, 0x8c, 0x9f, 0x24, 0x9f, 0x84, 0x80, 0x7d, 0x81, 0x38, 0x80, 0xe3,
0x9f, 0x7b, 0x9e, 0x0e, 0x9e, 0x1f, 0x80, 0x87, 0x84, 0xa6, 0x88, 0x8c, 0x8a,
0x25, 0x80, 0x08, 0x80, 0x0b, 0x80, 0x0b, 0x80, 0x01, 0x9f, 0xee, 0x9f, 0xdf,
0x9f, 0xdb, 0x9f, 0xe8, 0x9f, 0xfd, 0x80, 0x0a};

static u8 _ACI_COEF_TABLE_FS24M_SR6M[DTMB_ACI_COEF_SIZE] = {
0x9F, 0xF1, 0x9F, 0xFB, 0x80, 0x09, 0x80, 0x15, 0x80, 0x17, 0x80, 0x0D, 0x9F,
0xFB, 0x9F, 0xE9, 0x9F, 0xE2, 0x9F, 0xEC, 0x80, 0x04, 0x80, 0x1D, 0x80, 0x27,
0x80, 0x19, 0x9F, 0xFA, 0x9F, 0xD9, 0x9F, 0xCE, 0x9F, 0xE1, 0x80, 0x0C, 0x80,
0x35, 0x80, 0x42, 0x80, 0x24, 0x9F, 0xEA, 0x9F, 0xB6, 0x9F, 0xAA, 0x9F, 0xD6,
0x80, 0x26, 0x80, 0x6A, 0x80, 0x72, 0x80, 0x2E, 0x9F, 0xBF, 0x9F, 0x66, 0x9F,
0x65, 0x9F, 0xCE, 0x80, 0x71, 0x80, 0xED, 0x80, 0xE2, 0x80, 0x35, 0x9F, 0x2B,
0x9E, 0x5C, 0x9E, 0x72, 0x9F, 0xCA, 0x82, 0x3B, 0x85, 0x13, 0x87, 0x59, 0x88,
0x38, 0x80, 0x00, 0x80, 0x00, 0x80, 0x01, 0x80, 0x02, 0x80, 0x02, 0x80, 0x00,
0x9F, 0xFC, 0x9F, 0xF6, 0x9F, 0xF0, 0x9F, 0xED};

//---------------------------
//	Global	Variables
//---------------------------
static u8 _riu_read_byte1(u32 uaddr32)
{
	return readb(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (uaddr32));
}

static void _riu_write_byte1(u32 uaddr32,	u8 val8)
{

	writeb(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr + (uaddr32), val8);
}
//---------------------------
//	Local	Functions
//---------------------------
static u8 _hal_dmd_riu_readbyte(u32 uaddr32)
{
	#if (!DVB_STI) && (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN)
	return _RIU_READ_BYTE(uaddr32);
	#elif	DVB_STI
	return _RIU_READ_BYTE(((uaddr32) << 1)	-	((uaddr32) &	1));
	#else
	return _RIU_READ_BYTE(((uaddr32) << 1)	-	((uaddr32) &	1));
	#endif
}

static void _hal_dmd_riu_writebyte(u32 uaddr32,	u8 val8)
{
	#if (!DVB_STI) && (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN)
	_RIU_WRITE_BYTE(uaddr32,	val8);
	#elif	DVB_STI
	_RIU_WRITE_BYTE(((uaddr32) << 1) - ((uaddr32) & 1), val8);
	#else
	_RIU_WRITE_BYTE(((uaddr32) << 1) - ((uaddr32) & 1), val8);
	#endif
}

static void _hal_dmd_riu_writebytemask(
u32 uaddr32, u8 val8, u8 umask8)
{
	#if (!DVB_STI) && (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN)
	_RIU_WRITE_BYTE(uaddr32, (_RIU_READ_BYTE(uaddr32) & ~(umask8))
	| ((val8) & (umask8)));
	#else

	_RIU_WRITE_BYTE((((uaddr32) << 1) - ((uaddr32) & 1)),
	(_RIU_READ_BYTE((((uaddr32) << 1) - ((uaddr32) & 1)))
	& ~(umask8)) | ((val8) & (umask8)));
	#endif
}

static u8/*MS_BOOL*/ _mbx_writereg(u16 uaddr16,	u8 udata8)
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

static u8/*MS_BOOL*/ _mbx_readreg(u16 uaddr16,	u8 *udata8)
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

	HAL_INTERN_DTMB_DBINFO(PRINT("------DMD_DTMB_CHIP_M7332------\n"));

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

#else
static void _hal_intern_dtmb_initclk(void)
{
//VT	PRINT("------DMD_DTMB_CHIP_NONE------\n");

}
#endif

#else // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

static void _hal_intern_dtmb_initclk(void)
{
//VT	PRINT("------DMD_DTMB_CHIP_NONE------\n");

}

#endif // #if !DMD_DTMB_3PARTY_EN || defined	MTK_PROJECT || BIN_RELEASE

static u8/*MS_BOOL*/ _hal_intern_dtmb_ready(void)
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

static u8/*MS_BOOL*/ _hal_intern_dtmb_download(void)
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
	u64/*MS_PHY*/	VA_DramCodeAddr;
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

	#else // #if (FW_RUN_ON_DRAM_MODE == 0)

	//download	DRAM	part
 // VA = MsOS_PA2KSEG1(PA);//NonCache
	HAL_INTERN_DTMB_DBINFO(PRINT(">>DTMB_FW_DRAM_START_ADDR=0x%lx\n", ((
	pRes->sDMD_DTMB_InitData.utdistartaddr32 * 16) + (3 * 1024 * 1024))));
	#if defined	MTK_PROJECT
	VA_DramCodeAddr = (u64/*MS_PHY*/)BSP_MapReservedMem((void *)((
	pRes->sDMD_DTMB_InitData.utdistartaddr32 * 16)
	+ (3 * 1024 * 1024)),	1024*1024);
	#else
	VA_DramCodeAddr = MsOS_PA2KSEG1(((
	pRes->sDMD_DTMB_InitData.utdistartaddr32 * 16) + (3 * 1024 * 1024)));
	#endif
	MEMCPY((void *)(u32/*MS_VIRT*/)VA_DramCodeAddr, &DTMB_table[0],
	ulib_size16);

	uaddressOffset16 = (DTMB_table[0x400] << 8)|DTMB_table[0x401];

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.uif_khz16;
	HAL_INTERN_DTMB_DBINFO(PRINT("uif_khz16=%d\n",
	pRes->sDMD_DTMB_InitData.uif_khz16));
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.uif_khz16 >> 8);
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 1),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.bIQSwap;
	HAL_INTERN_DTMB_DBINFO(PRINT("bIQSwap=%d\n",
	pRes->sDMD_DTMB_InitData.bIQSwap));
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 2),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.uagc_reference16;
	HAL_INTERN_DTMB_DBINFO(PRINT("uagc_reference16=%X\n",
	pRes->sDMD_DTMB_InitData.uagc_reference16));
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 3),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.uagc_reference16 >> 8);
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 4),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_InitData.utdistartaddr32;
	HAL_INTERN_DTMB_DBINFO(PRINT("utdistartaddr32=%X\n",
	pRes->sDMD_DTMB_InitData.utdistartaddr32));
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 5),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 8);
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 6),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 16);
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 7),
	&utmpdata8, 1);

	utmpdata8 = (u8)(pRes->sDMD_DTMB_InitData.utdistartaddr32 >> 24);
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 8),
	&utmpdata8, 1);

	utmpdata8 = (u8)pRes->sDMD_DTMB_PriData.eLastType;
	HAL_INTERN_DTMB_DBINFO(PRINT("eLastType=%d\n",
	pRes->sDMD_DTMB_PriData.eLastType));
	MEMCPY((void *)(u32/*MS_VIRT*/)(VA_DramCodeAddr + uaddressOffset16 + 9),
	&utmpdata8, 1);

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

static u8/*MS_BOOL*/ _hal_intern_dtmb_exit(void)
{
	u8 ucheckcount8 = 0;

	_hal_dmd_riu_writebyte(MBRegBase + 0x1C, 0x01);

	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,	_hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)|0x02);// assert interrupt to VD MCU51
	_hal_dmd_riu_writebyte(DMDMcuBase + 0x03,	_hal_dmd_riu_readbyte(
	DMDMcuBase + 0x03)&(~0x02));// de-assert interrupt to VD MCU51

	while ((_hal_dmd_riu_readbyte(MBRegBase + 0x1C)&0x02) != 0x02) {
		sys.DelayMS(1);

	if (ucheckcount8++ == 0xFF) {
//VT		PRINT(">> DTMB Exit	Fail!\n");

		return FALSE;
	}
	}

//VT	PRINT(">> DTMB Exit	Ok!\n");

	#if POW_SAVE_BY_DRV
	_hal_dmd_riu_writebyte(DEMOD_GROUP_RSTZ,	DEMOD_RSTZ_EN);

	sys.DelayMS(1);

	_hal_dmd_riu_writebyte(DEMOD_GROUP_RSTZ,	DEMOD_RSTZ_DIS);
	#endif

	return TRUE;
}

static u8/*MS_BOOL*/ _hal_intern_dtmb_softreset(void)
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

static u8/*MS_BOOL*/ _hal_intern_dtmb_setacicoef(void)
{
	u8 *ACI_table;
	u8 i;
	u16 uaddressOffset16;

	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB)
		ACI_table = &_ACI_COEF_TABLE_FS24M_SR8M[0];
	else	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_7M)
		ACI_table = &_ACI_COEF_TABLE_FS24M_SR8M[0];
	else	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_6M)
		ACI_table = &_ACI_COEF_TABLE_FS24M_SR6M[0];
	else	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_5M)
		ACI_table = &_ACI_COEF_TABLE_FS24M_SR8M[0];
	else
		ACI_table = &_ACI_COEF_TABLE_FS24M_SR8M[0];

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
	_hal_dmd_riu_writebyte(DMDMcuBase+0x04, 0x00);
	// sram	address	low	byte
	_hal_dmd_riu_writebyte(DMDMcuBase+0x05, 0x00);
	// sram	address	high	byte

	//SET	SR	value
	uaddressOffset16 = INTERN_DTMB_table[0x400];
	uaddressOffset16 = ((uaddressOffset16 << 8)
	|INTERN_DTMB_table[0x401]) + 10;

	_hal_dmd_riu_writebyte(DMDMcuBase+0x04,
	(uaddressOffset16&0xFF));// sram	address	low	byte

	_hal_dmd_riu_writebyte(DMDMcuBase+0x05,
	(uaddressOffset16>>8));// sram	address	high	byte

	_hal_dmd_riu_writebyte(DMDMcuBase+0x0C,
		(u8)pRes->sDMD_DTMB_PriData.eLastType);

	//set	ACI	coefficient
	uaddressOffset16 = INTERN_DTMB_table[0x40A];
	uaddressOffset16 = ((uaddressOffset16 << 8)|INTERN_DTMB_table[0x40B]);
	uaddressOffset16 = ((INTERN_DTMB_table[uaddressOffset16] << 8)
	|INTERN_DTMB_table[uaddressOffset16+1]);

	_hal_dmd_riu_writebyte(DMDMcuBase+0x04,	(uaddressOffset16&0xFF));
	// sram	address	low	byte
	_hal_dmd_riu_writebyte(DMDMcuBase+0x05,	(uaddressOffset16>>8));
	// sram	address	high	byte

	for	(i = 0; i < DTMB_ACI_COEF_SIZE; i++) {
		_hal_dmd_riu_writebyte(DMDMcuBase+0x0C,	ACI_table[i]);
	//	write	data	to	VD	MCU	51	code	sram
	}

	_hal_dmd_riu_writebyte(DMDMcuBase+0x03, 0x50);
	// disable	auto-increase

	_hal_dmd_riu_writebyte(DMDMcuBase+0x03, 0x00);
	// disable	"vdmcu51_if"

	_hal_dmd_riu_writebyte(DMDMcuBase+0x00, 0x01);
	// reset MCU, madison patch

	_hal_dmd_riu_writebyte(DMDMcuBase+0x01, 0x01);// enable	SRAM
	_hal_dmd_riu_writebyte(DMDMcuBase+0x00, 0x00);// release	VD_MCU

	sys.DelayMS(20);

	return TRUE;
}

static u8/*MS_BOOL*/ _hal_intern_dtmb_setdtmbmode(void)
{
	if (_mbx_writereg(0x20C2, 0x03) == FALSE)
		return FALSE;
		return _mbx_writereg(0x20C0, 0x04);
}

static u8/*MS_BOOL*/ _hal_intern_dtmb_setmodeclean(void)
{
	if (_mbx_writereg(0x20C2, 0x07) == FALSE)
		return FALSE;
		return _mbx_writereg(0x20C0, 0x00);
}

static u8/*MS_BOOL*/ _hal_intern_dtmb_set_qam_sr(void)
{
	if (_mbx_writereg(0x20C2, 0x01) == FALSE)
		return FALSE;
		return _mbx_writereg(0x20C0, 0x04);
}

static u8/*MS_BOOL*/ _hal_intern_dtmb_agclock(void)
{
	u8 data = 0;

	#if USE_T2_MERGED_FEND
		_mbx_readreg(0x2829, &data);//AGC_LOCK
	#else
		_mbx_readreg(0x271D, &data);//AGC_LOCK
	#endif
	if (data&0x01)
		return TRUE;
	else
		return FALSE;
}

static u8/*MS_BOOL*/ _hal_intern_dtmb_pnp_lock(void)
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

static u8/*MS_BOOL*/ _hal_intern_dtmb_fec_lock(void)
{
	u8 ustate8 = 0,	ustatus8 = 0;


	_mbx_readreg(0x20C1, &ustate8);
	_mbx_readreg(0x20C3, &ustatus8);

	if ((((ustate8 == 0x62) && !(ustatus8&0x02)) || (ustate8	> 0x62))
		&& (ustate8	<= 0xF0))
		return TRUE;
		else	{
			return FALSE;
	}
}

static u8/*MS_BOOL*/ _hal_intern_dtmb_getmodulation(
struct DMD_DTMB_MODULATION_INFO *psDtmbGetModulation)
{
	u8 cm,	qam,	il,	cr,	sinr;
	u8 data_l = 0;
	u8 data_h = 0;
	u8 data_1 = 0;
	u8 data_2 = 0;

	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB ||
	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_7M ||
	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_6M ||
	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_5M) {
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

static u8 _hal_intern_dtmb_readifagc(void)
{
	u8 data = 0;

	#if USE_T2_MERGED_FEND
	_mbx_readreg(0x280F, &data);
	#else
	_mbx_readreg(0x28FD, &data);
	#endif

	return data;
}

#ifdef	UTPA2
static u8/*MS_BOOL*/ _hal_intern_dtmb_readfrequencyoffset(s16 *pfftfirstcfo,
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

	if (pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB ||
	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_7M ||
	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_6M ||
	pRes->sDMD_DTMB_PriData.eLastType == DMD_DTMB_DEMOD_DTMB_5M) {
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
		if (snr <= 4340)
			level = 1;// SNR <= 0.6	dB
		else	if (snr <= 4983)
			level = 2;// SNR <= 1.2	dB
		else	if (snr <= 5721)
			level = 3;// SNR <= 1.8	dB
		else	if (snr <= 6569)
			level = 4;// SNR <= 2.4	dB
		else	if (snr <= 7542)
			level = 5;// SNR <= 3.0	dB
		else	if (snr <= 8659)
			level = 6;// SNR <= 3.6	dB
		else	if (snr <= 9942)
			level = 7;// SNR <= 4.2	dB
		else	if (snr <= 11415)
			level = 8;// SNR <= 4.8	dB
		else	if (snr <= 13107)
			level = 9;// SNR <= 5.4	dB
		else	if (snr <= 15048)
			level = 10;// SNR <= 6.0	dB
		else	if (snr <= 17278)
			level = 11;// SNR <= 6.6	dB
		else	if (snr <= 19838)
			level = 12;// SNR <= 7.2	dB
		else	if (snr <= 22777)
			level = 13;// SNR <= 7.8	dB
		else	if (snr <= 26151)
			level = 14;// SNR <= 8.4	dB
		else	if (snr <= 30026)
			level = 15;// SNR <= 9.0	dB
		else	if (snr <= 34474)
			level = 16;// SNR <= 9.6	dB
		else	if (snr <= 39581)
			level = 17;// SNR <= 10.2	dB
		else	if (snr <= 45446)
			level = 18;// SNR <= 10.8	dB
		else	if (snr <= 52179)
			level = 19;// SNR <= 11.4	dB
		else	if (snr <= 59909)
			level = 20;// SNR <= 12.0	dB
		else	if (snr <= 68785)
			level = 21;// SNR <= 12.6	dB
		else	if (snr <= 78975)
			level = 22;// SNR <= 13.2	dB
		else	if (snr <= 90676)
			level = 23;// SNR <= 13.8	dB
		else	if (snr <= 104110)
			level = 24;// SNR <= 14.4	dB
		else	if (snr <= 119534)
			level = 25;// SNR <= 15.0	dB
		else	if (snr <= 137244)
			level = 26;// SNR <= 15.6	dB
		else	if (snr <= 157577)
			level = 27;// SNR <= 16.2	dB
		else	if (snr <= 180922)
			level = 28;// SNR <= 16.8	dB
		else	if (snr <= 207726)
			level = 29;// SNR <= 17.4	dB
		else	if (snr <= 238502)
			level = 30;// SNR <= 18.0	dB
		else	if (snr <= 273837)
			level = 31;// SNR <= 18.6	dB
		else	if (snr <= 314407)
			level = 32;// SNR <= 19.2	dB
		else	if (snr <= 360987)
			level = 33;// SNR <= 19.8	dB
		else	if (snr <= 414469)
			level = 34;// SNR <= 20.4	dB
		else	if (snr <= 475874)
			level = 35;// SNR <= 21.0	dB
		else	if (snr <= 546376)
			level = 36;// SNR <= 21.6	dB
		else	if (snr <= 627324)
			level = 37;// SNR <= 22.2	dB
		else	if (snr <= 720264)
			level = 38;// SNR <= 22.8	dB
		else	if (snr <= 826974)
			level = 39;// SNR <= 23.4	dB
		else	if (snr <= 949493)
			level = 40;// SNR <= 24.0	dB
		else	if (snr <= 1090164)
			level = 41;// SNR <= 24.6	dB
		else	if (snr <= 1251676)
			level = 42;// SNR <= 25.2	dB
		else	if (snr <= 1437116)
			level = 43;// SNR <= 25.8	dB
		else	if (snr <= 1650030)
			level = 44;// SNR <= 26.4	dB
		else	if (snr <= 1894488)
			level = 45;// SNR <= 27.0	dB
		else	if (snr <= 2175163)
			level = 46;// SNR <= 27.6	dB
		else	if (snr <= 2497421)
			level = 47;// SNR <= 28.2	dB
		else	if (snr <= 2867423)
			level = 48;// SNR <= 28.8	dB
		else	if (snr <= 3292242)
			level = 49;// SNR <= 29.4	dB
		else	if (snr	>	3292242)
			level = 50;// SNR <= 30.0	dB
	}
	}	else	{
		level = 0;
	}

	return level*2;
}

s16 _hal_intern_dtmb_nordig_get_cn(void)
{
		struct DMD_DTMB_MODULATION_INFO sDtmbModulationMode;
		u8/*MS_BOOL*/ ret1 = 0;
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
	u8/*MS_BOOL*/ ret1 = 0;
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
	u8/*MS_BOOL*/ ret1 = 0;
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
	u8/*MS_BOOL*/ ret = 0;

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
	u8/*MS_BOOL*/ ret = 0;
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
static u8/*MS_BOOL*/ _hal_intern_dtmb_getpreldpcber(u32 *pbiterr,
	u16 *perror_window)
#else
static u8/*MS_BOOL*/ _hal_intern_dtmb_getpreldpcber(float	*pber)
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

static u8/*MS_BOOL*/ _hal_intern_dtmb_getreg(u16 uaddr16,	u8 *pudata8)
{
	return _mbx_readreg(uaddr16,	pudata8);
}

static u8/*MS_BOOL*/ _hal_intern_dtmb_setreg(u16 uaddr16,	u8 udata8)
{
	return _mbx_writereg(uaddr16,	udata8);
}

//---------------------------
//	Global	Functions
//---------------------------
u8/*MS_BOOL*/	hal_intern_dtmb_ioctl_cmd(struct DTMB_IOCTL_DATA *pData,
	enum DMD_DTMB_HAL_COMMAND eCmd, void *pArgs)
{
	u8/*MS_BOOL*/ bResult = TRUE;

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
	case	DMD_DTMB_HAL_CMD_SetACICoef:
	bResult = _hal_intern_dtmb_setacicoef();
	break;
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
	case	DMD_DTMB_HAL_CMD_AGCLock:
	bResult = _hal_intern_dtmb_agclock();
	break;
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
	case	DMD_DTMB_HAL_CMD_ReadIFAGC:
	*((u16 *)pArgs) = _hal_intern_dtmb_readifagc();
	break;
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

u8/*MS_BOOL*/	mdrv_dmd_dtmb_initial_hal_interface(void)
{
	return TRUE;
}

#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG)\
&& defined(CONFIG_MSTAR_CHIP)
EXPORT_SYMBOL(hal_intern_dtmb_ioctl_cmd);
#endif
