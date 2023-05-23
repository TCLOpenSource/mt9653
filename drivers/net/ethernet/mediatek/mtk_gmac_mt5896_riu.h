/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/* Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#ifndef _MTK_GMAC_MT5896_RIU_H_
#define _MTK_GMAC_MT5896_RIU_H_

#include <linux/types.h>
#include <linux/io.h>

#define UNUSED __attribute__((unused))

/*********************************************
 * ---------------- AC_FULL/MSK ----------------
 * B3[31:24] | B2[23:16] | B1[15:8] | B0[7:0]
 * W32[31:16] | W21[23:8] | W10 [15:0]
 * DW[31:0]
 * ---------------------------------------------
 *********************************************/
#define AC_FULLB0           1
#define AC_FULLB1           2
#define AC_FULLB2           3
#define AC_FULLB3           4
#define AC_FULLW10          5
#define AC_FULLW21          6
#define AC_FULLW32          7
#define AC_FULLDW           8
#define AC_MSKB0            9
#define AC_MSKB1            10
#define AC_MSKB2            11
#define AC_MSKB3            12
#define AC_MSKW10           13
#define AC_MSKW21           14
#define AC_MSKW32           15
#define AC_MSKDW            16

/*********************************************
 * -------------------- Fld --------------------
 * wid[31:16] | shift[15:8] | ac[7:0]
 * ---------------------------------------------
 *********************************************/
#define Fld(wid, shft, ac)    (((uint32_t)wid << 16) | (shft << 8) | ac)
#define Fld_wid(fld)          (uint8_t)((fld) >> 16)
#define Fld_shft(fld)         (uint8_t)((fld) >> 8)
#define Fld_ac(fld)           (uint8_t)((fld))

/*********************************************
 * Prototype and Macro
 *********************************************/
static inline uint8_t UNUSED
	mtk_readb(void *reg32);
static inline uint16_t UNUSED
	mtk_readw(void *reg32);
static inline uint16_t UNUSED
	mtk_readw_relaxed(void *reg32);
static inline uint32_t UNUSED
	mtk_readl(void *reg32);
static inline uint32_t UNUSED
	mtk_read_fld(void *reg32, uint32_t fld);
static inline void UNUSED
	mtk_writeb(void *reg32, uint8_t val8);
static inline void UNUSED
	mtk_writew(void *reg32, uint16_t val16);
static inline void UNUSED
	mtk_writew_relaxed(void *reg32, uint16_t val16);
static inline void UNUSED
	mtk_writel(void *reg32, uint32_t val32);
#define mtk_write_fld(reg32, val32, fld) \
	__mtk_write_fld_multi(0, reg32, 1, val32, fld)
#define mtk_write_fld_multi(reg32, num_fld, ...) \
	__mtk_write_fld_multi(0, reg32, num_fld, __VA_ARGS__)
#define mtk_write_fld_multi_mask(reg32, num_fld, ...) \
	__mtk_write_fld_multi(1, reg32, num_fld, __VA_ARGS__)

/*********************************************
 * Functions
 *********************************************/
static uint32_t UNUSED mtk_access_fld(
	uint8_t write, uint32_t tmp32, uint32_t val32, uint32_t fld)
{
	uint32_t t = 0;

	switch (Fld_ac(fld)) {
	case AC_FULLB0:
	case AC_FULLB1:
	case AC_FULLB2:
	case AC_FULLB3:
		if (write == 1)
			t = (tmp32 & (~((uint32_t)0xFF <<
				(8 * (Fld_ac(fld) - AC_FULLB0))))) |
				((val32 & 0xFF) << (8 * (Fld_ac(fld) - AC_FULLB0)));
		else
			t = (tmp32 & ((uint32_t)0xFF <<
				(8 * (Fld_ac(fld) - AC_FULLB0)))) >>
				(8 * (Fld_ac(fld) - AC_FULLB0));
		break;
	case AC_FULLW10:
	case AC_FULLW21:
	case AC_FULLW32:
		if (write == 1)
			t = (tmp32 & (~((uint32_t)0xFFFF <<
				(8 * (Fld_ac(fld) - AC_FULLW10))))) |
				((val32 & 0xFFFF) << (8 * (Fld_ac(fld) - AC_FULLW10)));
		else
			t = (tmp32 & (((uint32_t)0xFFFF <<
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
			t = (tmp32 & (~(((((uint32_t)1 << Fld_wid(fld)) - 1) <<
				Fld_shft(fld))))) |
				(((val32 & (((uint32_t)1 << Fld_wid(fld)) - 1)) <<
				Fld_shft(fld)));
		else
			t = (tmp32 & (((((uint32_t)1 << Fld_wid(fld)) - 1) <<
				Fld_shft(fld)))) >>
				Fld_shft(fld);
		break;
	case AC_MSKW10:
	case AC_MSKW21:
	case AC_MSKW32:
		if (write == 1)
			t = (tmp32 & (~(((((uint32_t)1 << Fld_wid(fld)) - 1) <<
				Fld_shft(fld))))) |
				(((val32 & (((uint32_t)1 << Fld_wid(fld))-1)) <<
				Fld_shft(fld)));
		else
			t = (tmp32 & (((((uint32_t)1 << Fld_wid(fld)) - 1) <<
				Fld_shft(fld)))) >>
				Fld_shft(fld);
		break;
	case AC_MSKDW:
		if (write == 1)
			t = (tmp32 & (~(((((uint32_t)1 << Fld_wid(fld)) - 1) <<
				Fld_shft(fld))))) |
				(((val32 & (((uint32_t)1 << Fld_wid(fld)) - 1)) <<
				Fld_shft(fld)));
		else
			t = (tmp32 & (((((uint32_t)1 << Fld_wid(fld)) - 1) <<
				Fld_shft(fld)))) >>
				Fld_shft(fld);
		break;
	default:
		break;
	}
	return t;
}

static void UNUSED __mtk_write_fld_multi(
	uint32_t mask, void *reg32, uint32_t num_fld, ...)
{
	uint32_t i, val32, fld, tmp32;
	va_list argp;

	tmp32 = (mask == 0) ? mtk_readl(reg32) : 0;
	va_start(argp, num_fld);
	for (i = 0; i < num_fld; i++) {
		val32 = va_arg(argp, uint32_t);
		fld = va_arg(argp, uint32_t);
		tmp32 = mtk_access_fld(1, tmp32, val32, fld);
	}
	va_end(argp);
	mtk_writel(reg32, tmp32);
}

static inline uint8_t UNUSED mtk_readb(void *reg32)
{
	return readb(reg32);
}

static inline uint16_t UNUSED mtk_readw(void *reg32)
{
	return readw(reg32);
}

static inline uint16_t UNUSED mtk_readw_relaxed(void *reg32)
{
	return readw_relaxed(reg32);
}

static inline uint32_t UNUSED mtk_readl(void *reg32)
{
	return readl(reg32);
}

static inline uint32_t UNUSED mtk_read_fld(void *reg32, uint32_t fld)
{
	return mtk_access_fld(0, mtk_readl(reg32), 0, fld);
}

static inline void UNUSED mtk_writeb(void *reg32, uint8_t val8)
{
	writeb(val8, reg32);
}

static inline void UNUSED mtk_writew(void *reg32, uint16_t val16)
{
	writew(val16, reg32);
}

static inline void UNUSED mtk_writew_relaxed(void *reg32, uint16_t val16)
{
	writew_relaxed(val16, reg32);
}

static inline void UNUSED mtk_writel(void *reg32, uint32_t val32)
{
	writel(val32, reg32);
}

#endif
