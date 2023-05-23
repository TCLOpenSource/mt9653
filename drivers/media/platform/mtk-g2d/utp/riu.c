// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRV_HWREG_COMMON_RIU_C_
#define _DRV_HWREG_COMMON_RIU_C_

#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/io.h>

#include "riu.h"

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
uint64_t _gRIU_VBase;
uint32_t _gRIU_PBase;
//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
uint32_t __u4IO32AccessFld(
	uint8_t write, uint32_t tmp32, uint32_t val32, uint32_t fld)
{
	uint32_t t = 0;

	switch (Fld_ac(fld)) {
	case AC_FULLB0:
	case AC_FULLB1:
	case AC_FULLB2:
	case AC_FULLB3:
		if (write == 1)
			t = (tmp32&(~((uint32_t)0xFF<<
				(8*(Fld_ac(fld)-AC_FULLB0))))) |
				((val32&0xFF)<<(8*(Fld_ac(fld)-AC_FULLB0)));
		else
			t = (tmp32&((uint32_t)0xFF<<
				(8*(Fld_ac(fld)-AC_FULLB0)))) >>
				(8*(Fld_ac(fld)-AC_FULLB0));
		break;
	case AC_FULLW10:
	case AC_FULLW21:
	case AC_FULLW32:
		if (write == 1)
			t = (tmp32&(~((uint32_t)0xFFFF<<
				(8*(Fld_ac(fld)-AC_FULLW10))))) |
				((val32&0xFFFF)<<(8*(Fld_ac(fld)-AC_FULLW10)));
		else
			t = (tmp32&(((uint32_t)0xFFFF<<
				(8*(Fld_ac(fld)-AC_FULLW10))))) >>
				(8*(Fld_ac(fld)-AC_FULLW10));
		break;
	case AC_FULLDW:
		t = val32;
		break;
	case AC_MSKB0:
	case AC_MSKB1:
	case AC_MSKB2:
	case AC_MSKB3:
		if (write == 1)
			t = (tmp32&(~(((((uint32_t)1<<Fld_wid(fld))-1)<<
				Fld_shft(fld))))) |
				(((val32&(((uint32_t)1<<Fld_wid(fld))-1))<<
				Fld_shft(fld)));
		else
			t = (tmp32&(((((uint32_t)1<<Fld_wid(fld))-1)<<
				Fld_shft(fld)))) >>
				Fld_shft(fld);
		break;
	case AC_MSKW10:
	case AC_MSKW21:
	case AC_MSKW32:
		if (write == 1)
			t = (tmp32&(~(((((uint32_t)1<<Fld_wid(fld))-1)<<
				Fld_shft(fld))))) |
				(((val32&(((uint32_t)1<<Fld_wid(fld))-1))<<
				Fld_shft(fld)));
		else
			t = (tmp32&(((((uint32_t)1<<Fld_wid(fld))-1)<<
				Fld_shft(fld)))) >>
				Fld_shft(fld);
		break;
	case AC_MSKDW:
		if (write == 1)
			t = (tmp32&(~(((((uint32_t)1<<Fld_wid(fld))-1)<<
				Fld_shft(fld))))) |
				(((val32&(((uint32_t)1<<Fld_wid(fld))-1))<<
				Fld_shft(fld)));
		else
			t = (tmp32&(((((uint32_t)1<<Fld_wid(fld))-1)<<
				Fld_shft(fld)))) >>
				Fld_shft(fld);
		break;
	default:
		break;
	}
	return t;
}

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
void mtk_write2byte(
	uint32_t u32P_Addr,
	uint32_t u32Value)
{
	uint32_t PA_Offset = 0x00;
	uint64_t VA_Offset = 0x00;

	PA_Offset = u32P_Addr - _gRIU_PBase;
	VA_Offset = _gRIU_VBase + PA_Offset;
	writel(u32Value, (volatile void *)VA_Offset);
}

void mtk_write2bytemask(
	uint32_t u32P_Addr,
	uint32_t u32Value,
	uint32_t fld)
{
	uint32_t PA_Offset = 0x00;
	uint64_t VA_Offset = 0x00;
	uint32_t tmp = 0x00;

	PA_Offset = u32P_Addr - _gRIU_PBase;
	VA_Offset = _gRIU_VBase + PA_Offset;
	tmp = __u4IO32AccessFld(
		1,
		readl((const volatile void *)VA_Offset),
		u32Value,
		fld);
	writel(tmp, (volatile void *)VA_Offset);
}

uint32_t mtk_read2byte(
	uint32_t u32P_Addr)
{
	uint32_t PA_Offset = 0x00;
	uint64_t VA_Offset = 0x00;

	PA_Offset = u32P_Addr - _gRIU_PBase;
	VA_Offset = _gRIU_VBase + PA_Offset;

	return readl((const volatile void *)VA_Offset);
}

uint32_t mtk_read2bytemask(
	uint32_t u32P_Addr,
	uint32_t fld)
{
	uint32_t PA_Offset = 0x00;
	uint64_t VA_Offset = 0x00;
	uint32_t tmp;

	PA_Offset = u32P_Addr - _gRIU_PBase;
	VA_Offset = _gRIU_VBase + PA_Offset;
	tmp = __u4IO32AccessFld(0,
	readl((const volatile void *)VA_Offset),
	0, fld);

	return tmp;
}

void Set_RIU_Base(uint64_t u64V_Addr, uint32_t u32P_Addr)
{
	_gRIU_VBase = u64V_Addr;
	_gRIU_PBase = u32P_Addr;
}

#endif		//_DRV_HWREG_COMMON_RIU_C_
