// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include <linux/string.h>
#include "mtk_sti_msos.h"

// Internal Definition
#include "mtk_njpeg_def.h"
#include "mtk_drv_jpd.h"
#include "mtk_reg_jpd.h"

//-------------------------------------------------------------------------------------------------
// Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local Defines
//-------------------------------------------------------------------------------------------------
//NJPD driver debug macro

static __u8 bIs3HuffTbl;

#define NJPD_DEBUG_DRV_MSG(format, args...) \
	do { if ((u8NJPD_DrvDbgLevel & E_NJPD_DEBUG_DRV_MSG) == E_NJPD_DEBUG_DRV_MSG) \
		printf(format, ##args); } while (0)
#define NJPD_DEBUG_DRV_ERR(format, args...) \
	do { if ((u8NJPD_DrvDbgLevel & E_NJPD_DEBUG_DRV_ERR) == E_NJPD_DEBUG_DRV_ERR) \
		printf(format, ##args); } while (0)

#ifndef UNUSED
#define UNUSED(x) ((x) = (x))
#endif



#define NJPD_OVER_4G_ADDR		(0x300000000ULL)

//-------------------------------------------------------------------------------------------------
// Local Enumerations
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local Structurs
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Global Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------


static NJPD_Status _stNJPD_Status = {
	0,			//Current MRC address
	0,			//Current decoded Vidx
	0,			//Current decoded Row
	0,			//Current decoded Col
	FALSE,			//busy or not
	FALSE			//Isr is enabled or not
};

static __u8 u8NJPD_DrvDbgLevel = E_NJPD_DEBUG_DRV_ERR;

static __u64 u32MRC0_Start;
static __u64 u32MRC0_End;
static __u64 u32MRC1_Start;
static __u64 u32MRC1_End;
static __u64 u32READ_Start;

//-------------------------------------------------------------------------------------------------
// Debug Functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Local Functions
//-------------------------------------------------------------------------------------------------
void _NJPD_PrintMem(uintptr_t u32Addr, __u32 u32Size)
{
	__u32 u32i;

	NJPD_DEBUG_DRV_MSG("===========================================================\n");
	NJPD_DEBUG_DRV_MSG("print memory addr=0x%tx, size=0x%tx\n",
			   (ptrdiff_t) u32Addr, (ptrdiff_t) u32Size);
	NJPD_DEBUG_DRV_MSG("===========================================================\n");
	for (u32i = 0; u32i < u32Size / E_OFFSET8 + ((u32Size % E_OFFSET8) ? 1 : 0); u32i++) {
		NJPD_DEBUG_DRV_MSG("%02x %02x %02x %02x %02x %02x %02x %02x\n",
				   *((__u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM0)),
				   *((__u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM1)),
				   *((__u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM2)),
				   *((__u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM3)),
				   *((__u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM4)),
				   *((__u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM5)),
				   *((__u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM6)),
				   *((__u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM7))
			);
	}
	NJPD_DEBUG_DRV_MSG("===========================================================\n");
}

//-------------------------------------------------------------------------------------------------
// Global Functions
//-------------------------------------------------------------------------------------------------
__u64 _jpd_iova2va(__u64 u64iova)
{
	// TODO: need _jpd_iova2va?
	return 0;
}

/********************************************************************/
///Reset NJPD -- Reset must be called before trigger NJPD
///@param NULL
///@return none
/********************************************************************/
void MDrv_NJPD_SetRIUBank(mtk_jpd_riu_banks *riu_banks)
{
	HAL_NJPD_SetRIUBank(riu_banks);
}

void MDrv_NJPD_Rst(void)
{
	HAL_NJPD_Rst();
}

void MDrv_NJPD_SetMRCStartAddr(__u64 u32ByteOffset)
{
	u32READ_Start = u32ByteOffset;
	if (HAL_NJPD_EXT_IsEnable8G()) {
		HAL_NJPD_EXT_SetMRCStart_8G_0((u32ByteOffset & E_MARKER_FFFF));
		HAL_NJPD_EXT_SetMRCStart_8G_1((u32ByteOffset >> E_NUM16) & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetMRCStart_8G_2(((__u64) u32ByteOffset) >> E_NUM32);
	} else {
		HAL_NJPD_SetMRCStart_Low((u32ByteOffset & E_MARKER_FFFF));
		HAL_NJPD_SetMRCStart_High((u32ByteOffset >> E_NUM16));
	}
}

__u8 MDrv_NJPD_SetReadBuffer(__u64 u64BufAddr, __u32 u32BufSize, __u8 index)
{
	u64 u64MRC_Start = u64BufAddr;
	u64 u64MRC_End = ((u64BufAddr + u32BufSize) - 1);

	if (index == 0) {
		if (HAL_NJPD_EXT_IsEnable8G()) {
			HAL_NJPD_EXT_SetMRCBuf0_Start_8G_0(u64MRC_Start & E_MARKER_FFFF);
			HAL_NJPD_EXT_SetMRCBuf0_Start_8G_1(
				(u64MRC_Start >> E_NUM16) & E_MARKER_FFFF);
			HAL_NJPD_EXT_SetMRCBuf0_Start_8G_2(((__u64) u64MRC_Start) >> E_NUM32);
			HAL_NJPD_EXT_SetMRCBuf0_End_8G_0(u64MRC_End & E_MARKER_FFFF);
			HAL_NJPD_EXT_SetMRCBuf0_End_8G_1((u64MRC_End >> E_NUM16) & E_MARKER_FFFF);
			HAL_NJPD_EXT_SetMRCBuf0_End_8G_2(((__u64) u64MRC_End) >> E_NUM32);
		} else {
			HAL_NJPD_SetMRCBuf0_StartLow(u64MRC_Start & E_MARKER_FFFF);
			HAL_NJPD_SetMRCBuf0_StartHigh(u64MRC_Start >> E_NUM16);
			HAL_NJPD_SetMRCBuf0_EndLow(u64MRC_End & E_MARKER_FFFF);
			HAL_NJPD_SetMRCBuf0_EndHigh(u64MRC_End >> E_NUM16);
		}
	} else if (index == 1) {
		if (HAL_NJPD_EXT_IsEnable8G()) {
			HAL_NJPD_EXT_SetMRCBuf1_Start_8G_0(u64MRC_Start & E_MARKER_FFFF);
			HAL_NJPD_EXT_SetMRCBuf1_Start_8G_1(
				(u64MRC_Start >> E_NUM16) & E_MARKER_FFFF);
			HAL_NJPD_EXT_SetMRCBuf1_Start_8G_2(((__u64) u64MRC_Start) >> E_NUM32);
			HAL_NJPD_EXT_SetMRCBuf1_End_8G_0(u64MRC_End & E_MARKER_FFFF);
			HAL_NJPD_EXT_SetMRCBuf1_End_8G_1((u64MRC_End >> E_NUM16) & E_MARKER_FFFF);
			HAL_NJPD_EXT_SetMRCBuf1_End_8G_2(((__u64) u64MRC_End) >> E_NUM32);
		} else {
			HAL_NJPD_SetMRCBuf1_StartLow(u64MRC_Start & E_MARKER_FFFF);
			HAL_NJPD_SetMRCBuf1_StartHigh(u64MRC_Start >> E_NUM16);
			HAL_NJPD_SetMRCBuf1_EndLow(u64MRC_End & E_MARKER_FFFF);
			HAL_NJPD_SetMRCBuf1_EndHigh(u64MRC_End >> E_NUM16);
		}
	} else {
		//error
	}


	NJPD_DEBUG_DRV_MSG("set MRC start:%tx, MRC end:%tx, Offset:%tx, Index:%d\n",
			   (ptrdiff_t) u64MRC_Start, (ptrdiff_t) u64MRC_End,
			   (ptrdiff_t) u64MRC_Start, index);

	return true;
}

/******************************************************************************/
///Set output frame buffer address for NJPD writing NJPEG uncompressed data
///@param u32BufAddr \b IN Start address for NJPD reading in MRC buffer
///@return none
/******************************************************************************/
void MDrv_NJPD_SetOutputFrameBuffer(__u64 u32BufAddr, __u16 u16LineNum)
{

	if (HAL_NJPD_EXT_IsEnable8G()) {
		HAL_NJPD_EXT_SetMWCBuf_Start_8G_0(u32BufAddr & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetMWCBuf_Start_8G_1((u32BufAddr >> E_NUM16) & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetMWCBuf_Start_8G_2((u32BufAddr) >> E_NUM32);
	} else {
		HAL_NJPD_SetMWCBuf_StartLow(u32BufAddr & E_MARKER_FFFF);
		HAL_NJPD_SetMWCBuf_StartHigh(u32BufAddr >> E_NUM16);
	}
	UNUSED(u16LineNum);
}

__u8 MDrv_NJPD_InitBuf(NJPD_BufCfg in)
{
	if (in.u64MRCBufAddr & NJPD_OVER_4G_ADDR)
		HAL_NJPD_EXT_Enable8G(TRUE);
	else
		HAL_NJPD_EXT_Enable8G(FALSE);

	NJPD_DEBUG_DRV_MSG("MRC start:%tx, MRC end:%tx, Offset:%tx\n",
			   (ptrdiff_t) in.u64MRCBufAddr,
			   (ptrdiff_t) (in.u64MRCBufAddr + in.u32MRCBufSize),
			   (ptrdiff_t) (in.u64MRCBufAddr + in.u32MRCBufOffset));

	MDrv_NJPD_SetMRCStartAddr(in.u64MRCBufAddr + in.u32MRCBufOffset);
	MDrv_NJPD_SetReadBuffer(in.u64MRCBufAddr, in.u32MRCBufSize, 0);

	MDrv_NJPD_SetOutputFrameBuffer(in.u64MWCBufAddr, in.u16MWCBufLineNum);

	return TRUE;
}

void MDrv_NJPD_SetPicDimension(__u16 u16PicWidth, __u16 u16PicHeight)
{
	HAL_NJPD_SetPic_H((u16PicWidth >> E_NUM3));
	HAL_NJPD_SetPic_V((u16PicHeight >> E_NUM3));
}

__u16 MDrv_NJPD_GetEventFlag(void)
{
	return HAL_NJPD_GetEventFlag();
}

void MDrv_NJPD_SetEventFlag(__u16 u16Value)
{
	// clear by write
	HAL_NJPD_ClearEventFlag(u16Value);
}

void MDrv_NJPD_SetROI(__u16 start_x, __u16 start_y, __u16 width, __u16 height)
{
	HAL_NJPD_SetROI_H(start_x >> E_NUM3);
	HAL_NJPD_SetROI_V(start_y >> E_NUM3);
	HAL_NJPD_SetROIWidth(width >> E_NUM3);
	HAL_NJPD_SetROIHeight(height >> E_NUM3);
}

void MDrv_NJPD_PowerOn(void)
{
	HAL_NJPD_PowerOn();
	_stNJPD_Status.bIsBusy = TRUE;
}

void MDrv_NJPD_PowerOff(void)
{
	HAL_NJPD_PowerOff();
	_stNJPD_Status.bIsBusy = FALSE;
}

void MDrv_NJPD_SetRSTIntv(__u16 u16Value)
{
	HAL_NJPD_SetRSTIntv(u16Value);
}

__u16 MDrv_NJPD_GetCurVidx(void)
{
	NJPD_DEBUG_DRV_ERR("Error!!! Do not support %s() in NJPD!!!!!!!!!!!!!!!!\n", __func__);
	return 0;
}

void MDrv_NJPD_TableRead(void)
{				//TODO only debug
	__u16 u16Value = 0;
	__u16 u16Addr;
	__u16 u16RIUValue = HAL_NJPD_GetRIUInterface();
	const __u8 u8TableMask = 0x03;
	const __u8 u8TableH = 0;		// 2'b00: h table, 2'b01: g table, 2'b10: q table
	const __u16 u16TableCntH = HAL_NJPD_GetHTableSize();
	const __u8 u8TableG = 1;		// 2'b00: h table, 2'b01: g table, 2'b10: q table
	const __u16 u16TableCntG = (bIs3HuffTbl) ? 96 : 64;
	const __u8 u8TableQ = 2;		// 2'b00: h table, 2'b01: g table, 2'b10: q table
	const __u16 u16TableCntQ = 64;

	HAL_NJPD_Set_GlobalSetting02(HAL_NJPD_Get_GlobalSetting02() & ~NJPD_TBC_MODE);

	NJPD_DEBUG_DRV_MSG("\n%s(), read h table start================\n", __func__);
	for (u16Addr = 0; u16Addr < u16TableCntH; u16Addr++) {
		u16Value = NJPD_JPD_TBC_TABLE_READ |
			((u8TableH & u8TableMask) << E_NUM1 & NJPD_JPD_TBC_SEL) | u16Addr << E_NUM8;
		HAL_NJPD_SetRIUInterface(u16Value);
		HAL_NJPD_SetWriteOneClearReg_2(NJPD_TBC_EN);
		while (1) {
			if (HAL_NJPD_GetRIUInterface() & NJPD_JPD_TBC_TABLE_READ) {
				NJPD_DEBUG_DRV_MSG("%02x %02x %02x %02x ",
				   HAL_NJPD_TBCReadData_L() & E_MARKER_FF,
				   (HAL_NJPD_TBCReadData_L() & E_MARKER_FF00) >> E_NUM8,
				   HAL_NJPD_TBCReadData_H() & E_MARKER_FF,
				   (HAL_NJPD_TBCReadData_H() & E_MARKER_FF00) >> E_NUM8);
				if (u16Addr % 2 == 1)
					NJPD_DEBUG_DRV_MSG("\n");
				break;
			}
		}
	}
	NJPD_DEBUG_DRV_MSG("%s(), read h table end ================\n", __func__);

	NJPD_DEBUG_DRV_MSG("\n%s(), read g table start================\n", __func__);

	for (u16Addr = 0; u16Addr < u16TableCntG; u16Addr++) {
		u16Value = NJPD_JPD_TBC_TABLE_READ | ((u8TableG & u8TableMask) << E_NUM1
			& NJPD_JPD_TBC_SEL) | u16Addr << E_NUM8;
		HAL_NJPD_SetRIUInterface(u16Value);
		HAL_NJPD_SetWriteOneClearReg_2(NJPD_TBC_EN);
		while (1) {
			if (HAL_NJPD_GetRIUInterface() & NJPD_JPD_TBC_TABLE_READ) {
				NJPD_DEBUG_DRV_MSG("%02x %02x %02x %02x ",
				   HAL_NJPD_TBCReadData_L() & E_MARKER_FF,
				   (HAL_NJPD_TBCReadData_L() & E_MARKER_FF00) >> E_NUM8,
				   HAL_NJPD_TBCReadData_H() & E_MARKER_FF,
				   (HAL_NJPD_TBCReadData_H() & E_MARKER_FF00) >> E_NUM8);
				if (u16Addr % 2 == 1)
					NJPD_DEBUG_DRV_MSG("\n");
				break;
			}
		}
	}
	NJPD_DEBUG_DRV_MSG("%s(), read g table end ================\n", __func__);

	NJPD_DEBUG_DRV_MSG("\n%s(), read q table start================\n", __func__);
	for (u16Addr = 0; u16Addr < u16TableCntQ; u16Addr++) {
		u16Value = NJPD_JPD_TBC_TABLE_READ |
			((u8TableQ & u8TableMask) << E_NUM1 & NJPD_JPD_TBC_SEL) | u16Addr << E_NUM8;
		HAL_NJPD_SetRIUInterface(u16Value);
		HAL_NJPD_SetWriteOneClearReg_2(NJPD_TBC_EN);
		while (1) {
			if (HAL_NJPD_GetRIUInterface() & NJPD_JPD_TBC_TABLE_READ) {
				NJPD_DEBUG_DRV_MSG("%02x %02x %02x %02x ",
				   HAL_NJPD_TBCReadData_L() & E_MARKER_FF,
				   (HAL_NJPD_TBCReadData_L() & E_MARKER_FF00) >> E_NUM8,
				   HAL_NJPD_TBCReadData_H() & E_MARKER_FF,
				   (HAL_NJPD_TBCReadData_H() & E_MARKER_FF00) >> E_NUM8);
				if (u16Addr % 2 == 1)
					NJPD_DEBUG_DRV_MSG("\n");
				break;
			}
		}
	}
	NJPD_DEBUG_DRV_MSG("%s(), read q table end ================\n", __func__);
	HAL_NJPD_Set_GlobalSetting02(HAL_NJPD_Get_GlobalSetting02() | NJPD_TBC_MODE);
	HAL_NJPD_SetRIUInterface(u16RIUValue);
}

void MDrv_NJPD_Debug(void)
{
	HAL_NJPD_Debug();

	NJPD_DEBUG_DRV_MSG("[offset: MRC0: MRC1]=[%td, %td, %td]\n",
	(ptrdiff_t) ((u32READ_Start / E_OFFSET16 - u32MRC0_Start / E_OFFSET16 + 1) % E_OFFSET16),
	(ptrdiff_t) ((u32MRC0_End / E_OFFSET16 - u32MRC0_Start / E_OFFSET16 + 1) % E_OFFSET16),
	(ptrdiff_t) ((u32MRC1_End / E_OFFSET16 - u32MRC1_Start / E_OFFSET16 + 1) % E_OFFSET16)
	);

	//_NJPD_PrintMem(VirtTableAddr, 2048);

}

void MDrv_NJPD_SetDbgLevel(__u8 u8DbgLevel)
{
	u8NJPD_DrvDbgLevel = u8DbgLevel;
	HAL_NJPD_SetDbgLevel(u8DbgLevel);
}

NJPD_Status *MDrv_NJPD_GetStatus(void)
{
	_stNJPD_Status.u32CurMRCAddr = HAL_NJPD_GetCurMRCAddr();
	_stNJPD_Status.u16CurVidx = 0;
	_stNJPD_Status.u16CurRow = HAL_NJPD_GetCurRow();
	_stNJPD_Status.u16CurCol = HAL_NJPD_GetCurCol();
	return &_stNJPD_Status;
}


__u32 MDrv_NJPD_GetCurMRCAddr(void)
{
	return HAL_NJPD_GetCurMRCAddr();
}

__u32 MDrv_NJPD_GetCurMWCAddr(void)
{
	return HAL_NJPD_GetCurMWCAddr();
}

__u16 MDrv_NJPD_GetCurRow(void)
{
	return HAL_NJPD_GetCurRow();
}

__u16 MDrv_NJPD_GetCurCol(void)
{
	return HAL_NJPD_GetCurCol();
}

void MDrv_NJPD_SetAutoProtect(__u8 enable)
{
	if (HAL_NJPD_EXT_IsEnable8G()) {
		HAL_NJPD_EXT_8G_WRITE_BOUND_ENABLE(enable);
		HAL_NJPD_SetAutoProtect(FALSE);
	} else
		HAL_NJPD_SetAutoProtect(enable);
}

void MDrv_NJPD_SetWPENEndAddr(__u64 u32ByteOffset)
{
	NJPD_DEBUG_DRV_MSG("%s=0x%tx\n", __func__, (ptrdiff_t) u32ByteOffset);
	if (HAL_NJPD_EXT_IsEnable8G()) {
		HAL_NJPD_EXT_SetWPENUBound_8G_0(u32ByteOffset & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetWPENUBound_8G_1((u32ByteOffset >> E_NUM16) & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetWPENUBound_8G_2(((__u64) u32ByteOffset) >> E_NUM32);
	} else {
		HAL_NJPD_SetWPENUBound_0_L((u32ByteOffset >> E_NUM3) & E_MARKER_FFFF);
		HAL_NJPD_SetWPENUBound_0_H((u32ByteOffset >> E_NUM3) >> E_NUM16);
	}
}

void MDrv_NJPD_SetWPENStartAddr(__u64 u32ByteOffset)
{
	NJPD_DEBUG_DRV_MSG("%s=0x%tx\n", __func__, (ptrdiff_t) u32ByteOffset);
	if (HAL_NJPD_EXT_IsEnable8G()) {
		HAL_NJPD_EXT_SetWPENLBound_8G_0(u32ByteOffset & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetWPENLBound_8G_1((u32ByteOffset >> E_NUM16) & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetWPENLBound_8G_2((u32ByteOffset) >> E_NUM32);
	} else {
		HAL_NJPD_SetWPENLBound_0_L((u32ByteOffset >> E_NUM3) & E_MARKER_FFFF);
		HAL_NJPD_SetWPENLBound_0_H((u32ByteOffset >> E_NUM3) >> E_NUM16);
	}
}

void MDrv_NJPD_SetSpare(__u16 u16Value)
{
	HAL_NJPD_SetSpare00(u16Value);
}

__u16 MDrv_NJPD_GetSpare(void)
{
	return HAL_NJPD_GetSpare00();
}

void MDrv_NJPD_SetMRC_Valid(__u8 u8Index)
{
	NJPD_DEBUG_DRV_MSG("%s() with u8Index=%d\n", __func__, u8Index);
	if (u8Index == 0)
		HAL_NJPD_SetWriteOneClearReg(NJPD_MRC0_VALID);
	else
		HAL_NJPD_SetWriteOneClearReg(NJPD_MRC1_VALID);
}

void MDrv_NJPD_DecodeEnable(void)
{
	NJPD_DEBUG_DRV_MSG("%s().....\n", __func__);
	HAL_NJPD_SetWriteOneClearReg(NJPD_DECODE_ENABLE);
}


void MDrv_NJPD_TableLoadingStart(void)
{
	NJPD_DEBUG_DRV_MSG("%s().....\n", __func__);
	HAL_NJPD_SetWriteOneClearReg(NJPD_TABLE_LOADING_START);
}


void MDrv_NJPD_ReadLastBuffer(void)
{
	NJPD_DEBUG_DRV_MSG("%s().....\n", __func__);
	HAL_NJPD_SetWriteOneClearReg(NJPD_MRC_LAST);
}

void MDrv_NJPD_SetScalingDownFactor(NJPD_SCALING_DOWN_FACTOR eScalingFactor)
{
	__u16 u16Value;

	u16Value = ((HAL_NJPD_Get_GlobalSetting01() &
			 ~NJPD_DOWN_SCALE) | ((__u16) eScalingFactor << E_NUM0));
	HAL_NJPD_Set_GlobalSetting01(u16Value);
}

void MDrv_NJPD_GTable_Rst(__u8 bEnable)
{
	__u16 u16Value;

	if (bEnable) {
		u16Value = (HAL_NJPD_Get_GlobalSetting01() & ~NJPD_GTABLE_RST);
		HAL_NJPD_Set_GlobalSetting01(u16Value);
		u16Value = ((HAL_NJPD_Get_GlobalSetting01() &
				 ~NJPD_GTABLE_RST) | ((__u16) bEnable << E_NUM10));
		HAL_NJPD_Set_GlobalSetting01(u16Value);
	}
}

void MDrv_NJPD_HTable_Reload_Enable(__u8 bEnable)
{
	__u16 u16Value;

	u16Value = ((HAL_NJPD_Get_GlobalSetting01() &
			 ~NJPD_HTABLE_RELOAD_EN) | ((__u16) bEnable << E_NUM11));
	HAL_NJPD_Set_GlobalSetting01(u16Value);
}

void MDrv_NJPD_GTable_Reload_Enable(__u8 bEnable)
{
	__u16 u16Value;

	u16Value = ((HAL_NJPD_Get_GlobalSetting01() &
			 ~NJPD_GTABLE_RELOAD_EN) | ((__u16) bEnable << E_NUM12));
	HAL_NJPD_Set_GlobalSetting01(u16Value);
}

void MDrv_NJPD_QTable_Reload_Enable(__u8 bEnable)
{
	__u16 u16Value;

	u16Value = ((HAL_NJPD_Get_GlobalSetting01() &
			 ~NJPD_QTABLE_RELOAD_EN) | ((__u16) bEnable << E_NUM13));
	HAL_NJPD_Set_GlobalSetting01(u16Value);
}

void MDrv_NJPD_SetSoftwareVLD(__u8 bEnable)
{
	__u16 u16Value;

	u16Value = ((HAL_NJPD_Get_GlobalSetting02() &
			 ~(NJPD_REMOVE_0xFF00 | NJPD_REMOVE_0xFFFF | NJPD_FFD9_EN)) |
			NJPD_BITSTREAM_LE);
	HAL_NJPD_Set_GlobalSetting02(u16Value);
	u16Value = ((HAL_NJPD_Get_GlobalSetting01() & ~NJPD_SVLD) | ((__u16) bEnable << E_NUM6));
	HAL_NJPD_Set_GlobalSetting01(u16Value);
	if (bEnable)
		HAL_NJPD_SetSpare00(HAL_NJPD_GetSpare00() & E_MARKER_FFFE);
}

void MDrv_NJPD_SetDifferentQTable(__u8 bEnable)
{
	__u16 u16Value;

	u16Value = ((HAL_NJPD_Get_GlobalSetting00() & ~NJPD_SUVQ) | ((__u16) bEnable << E_NUM7));
	HAL_NJPD_Set_GlobalSetting00(u16Value);
}

void MDrv_NJPD_SetDifferentHTable(__u8 bEnable)
{
	__u16 u16Value;

	bIs3HuffTbl = bEnable;
	u16Value = ((HAL_NJPD_Get_GlobalSetting00() & ~NJPD_SUVH) | ((__u16) bEnable << E_NUM8));
	HAL_NJPD_Set_GlobalSetting00(u16Value);
}

void MDrv_NJPD_Set_GlobalSetting00(__u16 u16Value)
{
	HAL_NJPD_Set_GlobalSetting00(u16Value);
}

__u16 MDrv_NJPD_Get_GlobalSetting00(void)
{
	return HAL_NJPD_Get_GlobalSetting00();
}

void MDrv_NJPD_SetHTableStartAddr(__u64 u32ByteOffset)
{
	if (HAL_NJPD_EXT_IsEnable8G()) {
		HAL_NJPD_EXT_SetHTableStart_8G_0(u32ByteOffset & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetHTableStart_8G_1((u32ByteOffset >> E_NUM16) & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetHTableStart_8G_2((u32ByteOffset) >> E_NUM32);
	} else {
		HAL_NJPD_SetHTableStart_Low((u32ByteOffset & E_MARKER_FFFF));
		HAL_NJPD_SetHTableStart_High((u32ByteOffset >> E_NUM16));
	}
}

void MDrv_NJPD_SetHTableSize(__u16 u16Size)
{
	HAL_NJPD_SetHTableSize(u16Size);
}

void MDrv_NJPD_SetQTableStartAddr(__u64 u32ByteOffset)
{
	if (HAL_NJPD_EXT_IsEnable8G()) {
		HAL_NJPD_EXT_SetQTableStart_8G_0(u32ByteOffset & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetQTableStart_8G_1((u32ByteOffset >> E_NUM16) & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetQTableStart_8G_2((u32ByteOffset) >> E_NUM32);
	} else {
		HAL_NJPD_SetQTableStart_Low((u32ByteOffset & E_MARKER_FFFF));
		HAL_NJPD_SetQTableStart_High((u32ByteOffset >> E_NUM16));
	}
}


void MDrv_NJPD_SetGTableStartAddr(__u64 u32ByteOffset)
{
	if (HAL_NJPD_EXT_IsEnable8G()) {
		HAL_NJPD_EXT_SetGTableStart_8G_0(u32ByteOffset & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetGTableStart_8G_1((u32ByteOffset >> E_NUM16) & E_MARKER_FFFF);
		HAL_NJPD_EXT_SetGTableStart_8G_2((u32ByteOffset) >> E_NUM32);
	} else {
		HAL_NJPD_SetGTableStart_Low((u32ByteOffset & E_MARKER_FFFF));
		HAL_NJPD_SetGTableStart_High((u32ByteOffset >> E_NUM16));
	}
}


void MDrv_NJPD_EnablePowerSaving(void)
{
	HAL_NJPD_SetClockGate(E_MARKER_FF);
}

void MDrv_NJPD_SetOutputFormat(__u8 bRst, NJPD_OutputFormat eOutputFormat)
{
	HAL_NJPD_SetOutputFormat(bRst, eOutputFormat);
}

void MDrv_NJPD_SetIBufReadLength(__u8 u8Min, __u8 u8Max)
{
	HAL_NJPD_SetIBufReadLength(u8Min, u8Max);
}
