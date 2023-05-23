/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MSRIU_H_
#define _MSRIU_H_

#ifdef __KERNEL__
#include <linux/types.h>
#include <asm/io.h>
#else
#include <stdint.h>
#endif

#define UNUSED __attribute__((unused))

/*********************************************
 * ---------------- AC_FULL/MSK ----------------
 * B3[31:24] | B2[23:16] | B1[15:8] | B0[7:0]
 * W32[31:16] | W21[23:8] | W10 [15:0]
 * DW[31:0]
 * ---------------------------------------------
 *********************************************/
#define AC_FULLB0		1
#define AC_FULLB1		2
#define AC_FULLB2		3
#define AC_FULLB3		4
#define AC_FULLW10		5
#define AC_FULLW21		6
#define AC_FULLW32		7
#define AC_FULLDW		8
#define AC_MSKB0		9
#define AC_MSKB1		10
#define AC_MSKB2		11
#define AC_MSKB3		12
#define AC_MSKW10		13
#define AC_MSKW21		14
#define AC_MSKW32		15
#define AC_MSKDW		16

/*********************************************
 * -------------------- Fld --------------------
 * wid[31:16] | shift[15:8] | ac[7:0]
 * ---------------------------------------------
 *********************************************/
#define Fld(wid, shft, ac)		(((uint32_t)wid<<16)|(shft<<8)|ac)
#define Fld_wid(fld)			(uint8_t)((fld)>>16)
#define Fld_shft(fld)			(uint8_t)((fld)>>8)
#define Fld_ac(fld)				(uint8_t)((fld))

/*********************************************
 * Prototype and Macro
 *********************************************/
static inline uint8_t UNUSED
	u1IO32Read1B(void *reg32);
static inline uint16_t UNUSED
	u2IO32Read2B(void *reg32);
static inline uint32_t UNUSED
	u4IO32Read4B(void *reg32);
static inline uint32_t UNUSED
	u4IO32ReadFld(void *reg32, uint32_t fld);
static inline void UNUSED
	vIO32Write1B(void *reg32, uint8_t val8);
static inline void UNUSED
	vIO32Write2B(void *reg32, uint16_t val16);
static inline void UNUSED
	vIO32Write4B(void *reg32, uint32_t val32);
#define vIO32WriteFld(reg32, val32, fld) \
	__vIO32WriteFldMulti(0, reg32, 1, val32, fld)
#define vIO32WriteFldMulti(reg32, num_fld, ...) \
	__vIO32WriteFldMulti(0, reg32, num_fld, __VA_ARGS__)
#define vIO32WriteFldMultiMask(reg32, num_fld, ...) \
	__vIO32WriteFldMulti(1, reg32, num_fld, __VA_ARGS__)

static void RegWrite4B(void *reg32_l, void *reg32_h, uint32_t val);
static uint32_t RegRead4B(void *reg32_l, void *reg32_h);
/*********************************************
 * Functions
 *********************************************/
static uint32_t UNUSED __u4IO32AccessFld(
	uint8_t write, uint32_t tmp32, uint32_t val32, uint32_t fld)
{
	uint32_t t = 0;

	switch (Fld_ac(fld)) {
	case AC_FULLB0:
	case AC_FULLB1:
	case AC_FULLB2:
	case AC_FULLB3:
		if (write == 1)
			t =
			(tmp32 & (~((uint32_t) 0xFF <<
			(8 * (Fld_ac(fld) - AC_FULLB0))))) |
			((val32 & 0xFF) <<
			(8 * (Fld_ac(fld) - AC_FULLB0)));
		else
			t =
			(tmp32 & ((uint32_t) 0xFF <<
			(8 * (Fld_ac(fld) - AC_FULLB0)))) >>
			(8 * (Fld_ac(fld) - AC_FULLB0));
		break;
	case AC_FULLW10:
	case AC_FULLW21:
	case AC_FULLW32:
		if (write == 1)
			t =
			(tmp32 & (~((uint32_t) 0xFFFF <<
			(8 * (Fld_ac(fld) - AC_FULLW10))))) |
			((val32 & 0xFFFF) <<
			(8 * (Fld_ac(fld) - AC_FULLW10)));
		else
			t =
			(tmp32 & (((uint32_t) 0xFFFF <<
			(8 * (Fld_ac(fld) - AC_FULLW10))))) >>
			(8 * (Fld_ac(fld) - AC_FULLW10));
		break;
	case AC_FULLDW:
		t = val32;
		break;
	case AC_MSKB0:
	case AC_MSKB1:
	case AC_MSKB2:
	case AC_MSKB3:
		if (write == 1)
			t =
				(tmp32 & (~((((uint32_t) 1 <<
				Fld_wid(fld)) - 1) <<
				Fld_shft(fld)))) |
				((val32 & (((uint32_t) 1 <<
				Fld_wid(fld)) - 1)) <<
				Fld_shft(fld));
		else
			t =
			(tmp32 & ((((uint32_t) 1 <<
			Fld_wid(fld)) - 1) <<
			Fld_shft(fld))) >>
			Fld_shft(fld);
		break;
	case AC_MSKW10:
	case AC_MSKW21:
	case AC_MSKW32:
		if (write == 1)
			t =
			(tmp32 & (~(((((uint32_t) 1 <<
			Fld_wid(fld)) - 1) <<
			Fld_shft(fld))))) |
			(((val32 & (((uint32_t) 1 <<
			Fld_wid(fld)) - 1)) <<
			Fld_shft(fld)));
		else
			t =
			(tmp32 & (((((uint32_t) 1 <<
			Fld_wid(fld)) - 1) <<
			Fld_shft(fld)))) >>
			Fld_shft(fld);
		break;
	case AC_MSKDW:
		if (write == 1)
			t =
			(tmp32 & (~(((((uint32_t) 1 <<
			Fld_wid(fld)) - 1) <<
			Fld_shft(fld))))) |
			(((val32 & (((uint32_t) 1 <<
			Fld_wid(fld)) - 1)) <<
			Fld_shft(fld)));
		else
			t =
			(tmp32 & (((((uint32_t) 1 <<
			Fld_wid(fld)) - 1) <<
			Fld_shft(fld)))) >>
			Fld_shft(fld);
		break;
	default:
		break;
	}
	return t;
}

static void UNUSED __vIO32WriteFldMulti(
	uint32_t mask, void *reg32, uint32_t num_fld, ...)
{
	uint32_t i, val32, fld, tmp32;
	va_list argp;

	tmp32 = (mask == 0)?u4IO32Read4B(reg32):0;
	va_start(argp, num_fld);
	for (i = 0; i < num_fld; i++) {
		val32 = va_arg(argp, uint32_t);
		fld = va_arg(argp, uint32_t);
		tmp32 = __u4IO32AccessFld(1, tmp32, val32, fld);
	}
	va_end(argp);
	vIO32Write4B(reg32, tmp32);
}

static inline uint8_t UNUSED u1IO32Read1B(void *reg32)
{
#ifdef __KERNEL__
	return readb(reg32);
#else
	return (*((volatile uint8_t*)reg32));
#endif
}

static inline uint16_t UNUSED u2IO32Read2B(void *reg32)
{
	//pr_info("[%s][%d] reg32=%x\n", __func__, __LINE__, reg32);

#ifdef __KERNEL__
	return readw(reg32);
#else
	return (*((volatile uint16_t*)reg32));
#endif

	//pr_info("[%s][%d] reg32=%x done\n", __func__, __LINE__, reg32);

}

static inline uint32_t UNUSED u4IO32Read4B(void *reg32)
{
#ifdef __KERNEL__
	return readl(reg32);
#else
	return (*((volatile uint32_t*)reg32));
#endif
}

static inline uint32_t UNUSED u4IO32ReadFld(void *reg32, uint32_t fld)
{
	return __u4IO32AccessFld(0, u4IO32Read4B(reg32), 0, fld);
}

static inline void UNUSED vIO32Write1B(void *reg32, uint8_t val8)
{
#ifdef __KERNEL__
	writeb(val8, reg32);
#else
	*((volatile uint8_t*)reg32) = val8;
#endif
}

static inline void UNUSED vIO32Write2B(void *reg32, uint16_t val16)
{
	//pr_info("[%s][%d] reg32=%x val16=%x\n", __func__, __LINE__, reg32, val16);

#ifdef __KERNEL__
	writew(val16, reg32);
#else
	*((volatile uint16_t*)reg32) = val16;
#endif

	//pr_info("[%s][%d] reg32=%x val16=%x done\n", __func__, __LINE__, reg32, val16);
}

static inline void UNUSED vIO32Write4B(void *reg32, uint32_t val32)
{
#ifdef __KERNEL__
	writel(val32, reg32);
#else
	*((volatile uint32_t*)reg32) = val32;
#endif
}

static void UNUSED RegWrite4B(void *reg32_l, void *reg32_h, uint32_t val)
{
	vIO32Write2B(reg32_l, val);
	vIO32Write2B(reg32_h, (val >> 16));
}

static uint32_t UNUSED RegRead4B(void *reg32_l, void *reg32_h)
{
	uint32_t val_low, val_high;

	val_low = (uint32_t) u2IO32Read2B(reg32_l);
	val_high = (uint32_t) u2IO32Read2B(reg32_h);
	return ((val_high << 16) + val_low);
}

#endif
