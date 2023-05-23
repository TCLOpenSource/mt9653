// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021-2022 MediaTek Inc.
 */


//-------------------------------------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include "mtk_sti_msos.h"
// Internal Definition
#include "mtk_drv_jpd.h"
#include "mtk_reg_jpd.h"
#include "mtk_hal_jpd.h"


//-------------------------------------------------------------------------------------------------
// Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local Defines
//-------------------------------------------------------------------------------------------------

#define __HAL_NJPD_DelayMs(x)    do { __u32 ticks = 0; while ((ticks++) > (x*10)); } while (0)

#define NJPD_RIU_MAP u32NJPDRiuBaseAdd	//obtain in init

#define NJPD_DEBUG_HAL_MSG(format, args...) \
	do { if ((_u8NJPDHalDbgLevel & E_NJPD_DEBUG_HAL_MSG) == E_NJPD_DEBUG_HAL_MSG) \
		printf(format, ##args); } while (0)
#define NJPD_DEBUG_HAL_ERR(format, args...) \
	do { if ((_u8NJPDHalDbgLevel & E_NJPD_DEBUG_HAL_ERR) == E_NJPD_DEBUG_HAL_ERR) \
		printf(format, ##args); } while (0)


#define ENABLE_IOMMU_FPGA_VERIFICATION    FALSE
#define IOMMU_BIT33_OFFSET                (0x2)
#define IOMMU_FPGA_MAPPING_ADDR_OFFSET    (0x5)

//-------------------------------------------------------------------------------------------------
// Local Enumerations
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local Structurs
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Global Variables
//-------------------------------------------------------------------------------------------------
REG16 __iomem *u32NJPDRiuBaseAdd;
//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------
static __u8 _u8NJPDHalDbgLevel = E_NJPD_DEBUG_HAL_ERR;
static __u8 bEnable8G = FALSE;
//-------------------------------------------------------------------------------------------------
// Debug Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local Functions
//-------------------------------------------------------------------------------------------------

mtk_jpd_riu_banks *jpd_bank_offsets;

static void _HAL_NJPD_EXT_8G_DRAM_ENABLE(__u8 bEnable)
{
	const __u8 u8Enable = 0xff;
	const __u8 u8Disable = 0x00;

	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, bEnable);
	reg8_mask_write(BK_NJPD_EXT_GLOBAL_SETTING00,
			NJPD0_EXT_8G_DRAM_ENABLE, bEnable ? u8Enable : u8Disable);
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, reg16_read(BK_NJPD_EXT_GLOBAL_SETTING00));
}


//-------------------------------------------------------------------------------------------------
// Global Functions
//-------------------------------------------------------------------------------------------------
void HAL_NJPD_SetRIUBank(mtk_jpd_riu_banks *riu_banks)
{
	jpd_bank_offsets = riu_banks;
	NJPD_DEBUG_HAL_MSG("NJPD_REG_BASE = 0x%p\n", NJPD_REG_BASE);
	NJPD_DEBUG_HAL_MSG("NJPD_EXT_REG_BASE = 0x%p\n", NJPD_EXT_REG_BASE);
	NJPD_DEBUG_HAL_MSG("clk_jpd = 0x%p\n", jpd_bank_offsets->clk_jpd);
	NJPD_DEBUG_HAL_MSG("sw_en_smi2jpd = 0x%p\n", jpd_bank_offsets->sw_en_smi2jpd);
	NJPD_DEBUG_HAL_MSG("clk_njpd2jpd = 0x%p\n", jpd_bank_offsets->clk_njpd2jpd);
}

void HAL_NJPD_EXT_8G_WRITE_BOUND_ENABLE(__u8 bEnable)
{
	const __u8 u8Enable = 0xff;
	const __u8 u8Disable = 0x00;

#if ENABLE_IOMMU_FPGA_VERIFICATION
// TODO: to be checked.
//    bEnable = FALSE;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, bEnable);
	reg8_mask_write(BK_NJPD_EXT_GLOBAL_SETTING02,
			NJPD0_EXT_8G_WRITE_BOUND_ENABLE, bEnable ? u8Enable : u8Disable);

	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, reg16_read(BK_NJPD_EXT_GLOBAL_SETTING02));
}

void HAL_NJPD_EXT_Enable8G(__u8 bEnable)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	bEnable = TRUE;
#endif
	bEnable8G = bEnable;
	_HAL_NJPD_EXT_8G_DRAM_ENABLE(bEnable);
}

__u8 HAL_NJPD_EXT_IsEnable8G(void)
{
	return bEnable8G;
}

void HAL_NJPD_EXT_SetMRCBuf0_Start_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetMRCBuf0_StartLow(u16Value);
}

void HAL_NJPD_EXT_SetMRCBuf0_Start_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif

	HAL_NJPD_SetMRCBuf0_StartHigh(u16Value);
}

void HAL_NJPD_EXT_SetMRCBuf0_Start_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_READ_BUFFER0_START_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetMRCBuf0_Start_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_READ_BUFFER0_START_8GADDR_2));
}

void HAL_NJPD_EXT_SetMRCBuf0_End_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetMRCBuf0_EndLow(u16Value);
}

void HAL_NJPD_EXT_SetMRCBuf0_End_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif

	HAL_NJPD_SetMRCBuf0_EndHigh(u16Value);
}

void HAL_NJPD_EXT_SetMRCBuf0_End_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_READ_BUFFER0_END_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetMRCBuf0_End_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_READ_BUFFER0_END_8GADDR_2));
}

void HAL_NJPD_EXT_SetMRCBuf1_Start_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetMRCBuf1_StartLow(u16Value);
}

void HAL_NJPD_EXT_SetMRCBuf1_Start_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif
	HAL_NJPD_SetMRCBuf1_StartHigh(u16Value);
}

void HAL_NJPD_EXT_SetMRCBuf1_Start_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_READ_BUFFER1_START_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetMRCBuf1_Start_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_READ_BUFFER1_START_8GADDR_2));
}

void HAL_NJPD_EXT_SetMRCBuf1_End_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetMRCBuf1_EndLow(u16Value);
}

void HAL_NJPD_EXT_SetMRCBuf1_End_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif
	HAL_NJPD_SetMRCBuf1_EndHigh(u16Value);
}

void HAL_NJPD_EXT_SetMRCBuf1_End_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_READ_BUFFER1_END_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetMRCBuf1_End_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_READ_BUFFER1_END_8GADDR_2));
}

void HAL_NJPD_EXT_SetMRCStart_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetMRCStart_Low(u16Value);
}

void HAL_NJPD_EXT_SetMRCStart_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif

	HAL_NJPD_SetMRCStart_High(u16Value);
}

void HAL_NJPD_EXT_SetMRCStart_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_READ_START_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetMRCStart_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_READ_START_8GADDR_2));
}




void HAL_NJPD_EXT_SetMWCBuf_Start_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetMWCBuf_StartLow(u16Value);
}

void HAL_NJPD_EXT_SetMWCBuf_Start_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif

	HAL_NJPD_SetMWCBuf_StartHigh(u16Value);
}

void HAL_NJPD_EXT_SetMWCBuf_Start_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_WRITE_START_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetMWCBuf_Start_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_WRITE_START_8GADDR_2));
}

void HAL_NJPD_EXT_SetWPENUBound_8G_0(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_0, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetWPENUBound_8G_0 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_0));
}

void HAL_NJPD_EXT_SetWPENUBound_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif

	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_1, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetWPENUBound_8G_1 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_1));
}

void HAL_NJPD_EXT_SetWPENUBound_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetWPENUBound_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_2));
}

void HAL_NJPD_EXT_SetWPENLBound_8G_0(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_0, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetWPENLBound_8G_0 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_0));
}

void HAL_NJPD_EXT_SetWPENLBound_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif

	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_1, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetWPENLBound_8G_1 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_1));
}

void HAL_NJPD_EXT_SetWPENLBound_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetWPENLBound_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_2));
}

void HAL_NJPD_EXT_SetHTableStart_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetHTableStart_Low(u16Value);
}

void HAL_NJPD_EXT_SetHTableStart_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif

	HAL_NJPD_SetHTableStart_High(u16Value);
}

void HAL_NJPD_EXT_SetHTableStart_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_HTABLE_START_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetHTableStart_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_HTABLE_START_8GADDR_2));
}

void HAL_NJPD_EXT_SetQTableStart_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetQTableStart_Low(u16Value);
}

void HAL_NJPD_EXT_SetQTableStart_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif
	HAL_NJPD_SetQTableStart_High(u16Value);
}

void HAL_NJPD_EXT_SetQTableStart_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_QTABLE_START_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetQTableStart_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_QTABLE_START_8GADDR_2));
}

void HAL_NJPD_EXT_SetGTableStart_8G_0(__u16 u16Value)
{
	HAL_NJPD_SetGTableStart_Low(u16Value);
}

void HAL_NJPD_EXT_SetGTableStart_8G_1(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value -= IOMMU_FPGA_MAPPING_ADDR_OFFSET;
#endif
	HAL_NJPD_SetGTableStart_High(u16Value);
}

void HAL_NJPD_EXT_SetGTableStart_8G_2(__u16 u16Value)
{
#if ENABLE_IOMMU_FPGA_VERIFICATION
	u16Value |= IOMMU_BIT33_OFFSET;
#endif
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_EXT_MIU_GTABLE_START_8GADDR_2, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_EXT_GetGTableStart_8G_2 : 0x%04x\n",
			   reg16_read(BK_NJPD_EXT_MIU_GTABLE_START_8GADDR_2));
}

void HAL_NJPD_Set_GlobalSetting00(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_GLOBAL_SETTING00, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_Get_GlobalSetting00 : 0x%04x\n",
			   reg16_read(BK_NJPD_GLOBAL_SETTING00));
}

__u16 HAL_NJPD_Get_GlobalSetting00(void)
{
	return reg16_read(BK_NJPD_GLOBAL_SETTING00);
}

void HAL_NJPD_Set_GlobalSetting01(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_GLOBAL_SETTING01, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_Get_GlobalSetting01 : 0x%04x\n",
			   reg16_read(BK_NJPD_GLOBAL_SETTING01));
}

__u16 HAL_NJPD_Get_GlobalSetting01(void)
{
	return reg16_read(BK_NJPD_GLOBAL_SETTING01);
}

void HAL_NJPD_Set_GlobalSetting02(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_GLOBAL_SETTING02, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_Get_GlobalSetting02 : 0x%04x\n",
			   reg16_read(BK_NJPD_GLOBAL_SETTING02));
}

__u16 HAL_NJPD_Get_GlobalSetting02(void)
{
	return reg16_read(BK_NJPD_GLOBAL_SETTING02);
}


void HAL_NJPD_SetMRCBuf0_StartLow(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_BUFFER0_START_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCBuf0_StartLow : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_BUFFER0_START_ADDR_L));
}

void HAL_NJPD_SetMRCBuf0_StartHigh(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_BUFFER0_START_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCBuf0_StartHigh : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_BUFFER0_START_ADDR_H));
}

void HAL_NJPD_SetMRCBuf0_EndLow(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_BUFFER0_END_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCBuf0_EndLow : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_BUFFER0_END_ADDR_L));
}

void HAL_NJPD_SetMRCBuf0_EndHigh(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_BUFFER0_END_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCBuf0_EndHigh : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_BUFFER0_END_ADDR_H));
}

void HAL_NJPD_SetMRCBuf1_StartLow(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_BUFFER1_START_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCBuf1_StartLow : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_BUFFER1_START_ADDR_L));
}

void HAL_NJPD_SetMRCBuf1_StartHigh(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_BUFFER1_START_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCBuf1_StartHigh : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_BUFFER1_START_ADDR_H));
}

void HAL_NJPD_SetMRCBuf1_EndLow(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_BUFFER1_END_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCBuf1_EndLow : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_BUFFER1_END_ADDR_L));
}

void HAL_NJPD_SetMRCBuf1_EndHigh(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_BUFFER1_END_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCBuf1_EndHigh : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_BUFFER1_END_ADDR_H));
}


void HAL_NJPD_SetMRCStart_Low(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_START_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCStart_Low : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_START_ADDR_L));
}

void HAL_NJPD_SetMRCStart_High(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_READ_START_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMRCStart_High : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_START_ADDR_H));
}


void HAL_NJPD_SetMWCBuf_StartLow(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_WRITE_START_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMWCBuf_StartLow : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_WRITE_START_ADDR_L));
}

void HAL_NJPD_SetMWCBuf_StartHigh(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_WRITE_START_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMWCBuf_StartHigh : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_WRITE_START_ADDR_H));
}

__u16 HAL_NJPD_GetMWCBuf_StartLow(void)
{
	return reg16_read(BK_NJPD_MIU_WRITE_START_ADDR_L);
}

__u16 HAL_NJPD_GetMWCBuf_StartHigh(void)
{
	return reg16_read(BK_NJPD_MIU_WRITE_START_ADDR_H);
}


__u16 HAL_NJPD_GetMWCBuf_WritePtrLow(void)
{
	return reg16_read(BK_NJPD_MIU_WRITE_POINTER_ADDR_L);
}

__u16 HAL_NJPD_GetMWCBuf_WritePtrHigh(void)
{
	return reg16_read(BK_NJPD_MIU_WRITE_POINTER_ADDR_H);
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_SetPic_H(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_IMG_HSIZE, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetPic_H : 0x%04x\n", reg16_read(BK_NJPD_IMG_HSIZE));
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_SetPic_V(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_IMG_VSIZE, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetPic_V : 0x%04x\n", reg16_read(BK_NJPD_IMG_VSIZE));
}

void HAL_NJPD_ClearEventFlag(__u16 u16Value)
{
	// Write 1 clear
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_IRQ_CLEAR, u16Value);
}

void HAL_NJPD_ForceEventFlag(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_IRQ_FORCE, u16Value);
}

void HAL_NJPD_MaskEventFlag(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_IRQ_MASK, u16Value);
}

__u16 HAL_NJPD_GetEventFlag(void)
{
	return reg16_read(BK_NJPD_IRQ_FINAL_S);
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_SetROI_H(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_ROI_H_START, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetROI_H : 0x%04x\n", reg16_read(BK_NJPD_ROI_H_START));
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_SetROI_V(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_ROI_V_START, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetROI_V : 0x%04x\n", reg16_read(BK_NJPD_ROI_V_START));
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_SetROIWidth(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_ROI_H_SIZE, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetROIWidth : 0x%04x\n", reg16_read(BK_NJPD_ROI_H_SIZE));
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_SetROIHeight(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_ROI_V_SIZE, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetROIHeight : 0x%04x\n", reg16_read(BK_NJPD_ROI_V_SIZE));
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_SetClockGate(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_GATED_CLOCK_CTRL, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetClockGate : 0x%04x\n",
			   reg16_read(BK_NJPD_GATED_CLOCK_CTRL));
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_PowerOn(void)
{
	NJPD_DEBUG_HAL_MSG("%s", __func__);
	STI_NJPD_PowerOn();

	NJPD_DEBUG_HAL_MSG("NJPD_REG_BASE = 0x%p\n", NJPD_REG_BASE);
	NJPD_DEBUG_HAL_MSG("NJPD_EXT_REG_BASE = 0x%p\n", NJPD_EXT_REG_BASE);
	NJPD_DEBUG_HAL_MSG("clk_jpd = 0x%p\n", jpd_bank_offsets->clk_jpd);
	NJPD_DEBUG_HAL_MSG("sw_en_smi2jpd = 0x%p\n", jpd_bank_offsets->sw_en_smi2jpd);
	NJPD_DEBUG_HAL_MSG("clk_njpd2jpd = 0x%p\n", jpd_bank_offsets->clk_njpd2jpd);
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_PowerOff(void)
{
	NJPD_DEBUG_HAL_MSG("%s", __func__);
	STI_NJPD_PowerOff();
}


/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
void HAL_NJPD_SetRSTIntv(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_RESTART_INTERVAL, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetRSTIntv : 0x%04x\n", reg16_read(BK_NJPD_RESTART_INTERVAL));
}


/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return none
/******************************************************************************/
void HAL_NJPD_SetDbgLevel(__u8 u8DbgLevel)
{
	_u8NJPDHalDbgLevel = u8DbgLevel;
}

/******************************************************************************/
///Reset NJPD
///@param value \b IN
///@param value \b OUT
///@return none
/******************************************************************************/
void HAL_NJPD_Rst(void)
{
	const __u16 u16SetSpare00 = 0x27B;
	const __u8 u8Min = 0x10;
	const __u8 u8Max = 0x1f;

	NJPD_DEBUG_HAL_MSG("%s : start!!\n", __func__);

	// reset event flag
	HAL_NJPD_ClearEventFlag(NJPD_EVENT_ALL);
	// reset : low active

	HAL_NJPD_SetOutputFormat(TRUE, E_NJPD_OUTPUT_ORIGINAL);

	HAL_NJPD_Set_GlobalSetting02(NJPD_TBC_MODE | NJPD_LITTLE_ENDIAN |
				     NJPD_REMOVE_0xFF00 | NJPD_REMOVE_0xFFFF |
				     NJPD_HUFF_DATA_LOSS_ERROR);

	HAL_NJPD_Set_GlobalSetting00(HAL_NJPD_Get_GlobalSetting00() | NJPD_SWRST);

	// set 0 to turn on the irq
	HAL_NJPD_MaskEventFlag(~(NJPD_EVENT_DECODE_DONE | NJPD_EVENT_MINICODE_ERR |
		NJPD_EVENT_INV_SCAN_ERR | NJPD_EVENT_RES_MARKER_ERR | NJPD_EVENT_RMID_ERR |
		NJPD_EVENT_END_IMAGE_ERR | NJPD_EVENT_MRC0_EMPTY | NJPD_EVENT_MRC1_EMPTY |
		NJPD_EVENT_WRITE_PROTECT | NJPD_EVENT_DATA_LOSS_ERR | NJPD_EVENT_HUFF_TABLE_ERR));

	NJPD_DEBUG_HAL_MSG("Get BK_NJPD_GLOBAL_SETTING00 : 0x%04x\n",
			   HAL_NJPD_Get_GlobalSetting00());
	NJPD_DEBUG_HAL_MSG("Get BK_NJPD_GLOBAL_SETTING01 : 0x%04x\n",
			   HAL_NJPD_Get_GlobalSetting01());
	NJPD_DEBUG_HAL_MSG("Get BK_NJPD_GLOBAL_SETTING02 : 0x%04x\n",
			   HAL_NJPD_Get_GlobalSetting02());
	NJPD_DEBUG_HAL_MSG("HAL Get INTEN : 0x%04x\n", reg16_read(BK_NJPD_IRQ_MASK));
	NJPD_DEBUG_HAL_MSG("HAL Get EVENTFLAG : 0x%04x\n", reg16_read(BK_NJPD_IRQ_FINAL_S));

// Edison@20120620 by Arong
// 1. Add the function of auto last buffer when FFD9 is detected (reg_spare00[0])
// 2. Fix last buffer error (reg_spare00[1])
// 3. write burst_error (reg_spare00[3])        // Edison/Einstein doesn't have this bit
// 4. Fix read-burst error (reg_spare00[4])    // Edison/Einstein doesn't have this bit
// 5. Coding error (reg_spare00[5])
// 6. read-burst error (reg_spare00[6])

// Nike&Einstein@20130412 by Tony Tseng
// 9. ECO new error concealment(reg_spare00[9])    // Nike have this bit
	HAL_NJPD_SetSpare00(u16SetSpare00);

	HAL_NJPD_SetIBufReadLength(u8Min, u8Max);
	HAL_NJPD_SetCRCReadMode();
	NJPD_DEBUG_HAL_MSG("%s : end!!\n", __func__);
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
__u32 HAL_NJPD_GetCurMRCAddr(void)
{
	__u32 curMRCAddr;

	curMRCAddr = ((reg16_read(BK_NJPD_MIU_READ_POINTER_ADDR_H) << E_NUM16) |
		      reg16_read(BK_NJPD_MIU_READ_POINTER_ADDR_L));

	return curMRCAddr;
}

__u32 HAL_NJPD_GetCurMWCAddr(void)
{
	__u32 curMRCAddr;

	curMRCAddr = ((reg16_read(BK_NJPD_MIU_WRITE_POINTER_ADDR_H) << E_NUM16) |
		      reg16_read(BK_NJPD_MIU_WRITE_POINTER_ADDR_L));

	return curMRCAddr;
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
__u16 HAL_NJPD_GetCurRow(void)
{
	return reg16_read(BK_NJPD_ROW_IDEX);
}

/******************************************************************************/
///
///@param value \b IN
///@param value \b OUT
///@return status
/******************************************************************************/
__u16 HAL_NJPD_GetCurCol(void)
{
	return reg16_read(BK_NJPD_COLUMN_IDEX);
}

void HAL_NJPD_SetWriteProtect(__u8 enable)
{
	NJPD_DEBUG_HAL_MSG("%s : NOT IMPLEMENTED!!!\n", __func__);
	UNUSED(enable);
}

void HAL_NJPD_SetAutoProtect(__u8 enable)
{
	__u16 u16RegValue;
	const __u16 u16SetReg = 0x1FFF;

	u16RegValue = reg16_read(BK_NJPD_MARB_LBOUND_0_H);
	u16RegValue &= u16SetReg;
	u16RegValue |= enable << E_NUM13;
	reg16_write(BK_NJPD_MARB_LBOUND_0_H, u16RegValue);
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, reg16_read(BK_NJPD_MARB_LBOUND_0_H));
}


void HAL_NJPD_Set_MARB06(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_MARB_SETTING_06, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_Get_MARB06 : 0x%04X\n", reg16_read(BK_NJPD_MARB_SETTING_06));
}

__u16 HAL_NJPD_Get_MARB06(void)
{
	return reg16_read(BK_NJPD_MARB_SETTING_06);
}

void HAL_NJPD_Set_MARB07(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_MARB_SETTING_07, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_Get_MARB07 : 0x%04X\n", reg16_read(BK_NJPD_MARB_SETTING_07));
}

__u16 HAL_NJPD_Get_MARB07(void)
{
	return reg16_read(BK_NJPD_MARB_SETTING_07);
}


void HAL_NJPD_SetWPENUBound_0_L(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_MARB_UBOUND_0_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetWPENUBound_0_L : 0x%04X\n",
			   reg16_read(BK_NJPD_MARB_UBOUND_0_L));
}

void HAL_NJPD_SetWPENUBound_0_H(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_MARB_UBOUND_0_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetWPENUBound_0_H : 0x%04X\n",
			   reg16_read(BK_NJPD_MARB_UBOUND_0_H));
}

void HAL_NJPD_SetWPENLBound_0_L(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16Value);
	reg16_write(BK_NJPD_MARB_LBOUND_0_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetWPENLBound_0_L : 0x%04X\n",
			   reg16_read(BK_NJPD_MARB_LBOUND_0_L));
}

void HAL_NJPD_SetWPENLBound_0_H(__u16 u16Value)
{
	__u16 u16RegValue;
	const __u16 u16SetReg = 0xE000;
	const __u16 u16SetVal = 0x1FFF;

	u16RegValue = reg16_read(BK_NJPD_MARB_LBOUND_0_H);
	u16RegValue &= u16SetReg;
	u16Value &= u16SetVal;
	u16RegValue |= u16Value;
	NJPD_DEBUG_HAL_MSG("%s : 0x%04X\n", __func__, u16RegValue);
	reg16_write(BK_NJPD_MARB_LBOUND_0_H, u16RegValue);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetWPENLBound_0_H : 0x%04X\n",
			   reg16_read(BK_NJPD_MARB_LBOUND_0_H));
}

void HAL_NJPD_SetSpare00(__u16 u16Value)
{
// [0]: hw auto detect ffd9 => cannot be used in svld mode,
//       unless the "last buffer" cannot work, do not enable this
// [1]: input buffer bug => always turn on
// [2]: marb bug => always turn on (not used in 2011/12/28 ECO version)
// [3]: bypass marb => 2011/12/28 ECO version

	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_SPARE00, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetSpare00 : 0x%04x\n", reg16_read(BK_NJPD_SPARE00));
}

__u16 HAL_NJPD_GetSpare00(void)
{
	return reg16_read(BK_NJPD_SPARE00);
}

void HAL_NJPD_SetSpare01(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_SPARE01, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetSpare01 : 0x%04x\n", reg16_read(BK_NJPD_SPARE01));
}

__u16 HAL_NJPD_GetSpare01(void)
{
	return reg16_read(BK_NJPD_SPARE01);
}

void HAL_NJPD_SetSpare02(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_SPARE02, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetSpare02 : 0x%04x\n", reg16_read(BK_NJPD_SPARE02));
}

__u16 HAL_NJPD_GetSpare02(void)
{
	return reg16_read(BK_NJPD_SPARE02);
}

void HAL_NJPD_SetSpare03(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_SPARE03, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetSpare03 : 0x%04x\n", reg16_read(BK_NJPD_SPARE03));
}

__u16 HAL_NJPD_GetSpare03(void)
{
	return reg16_read(BK_NJPD_SPARE03);
}

void HAL_NJPD_SetSpare04(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_SPARE04, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetSpare04 : 0x%04x\n", reg16_read(BK_NJPD_SPARE04));
}

__u16 HAL_NJPD_GetSpare04(void)
{
	return reg16_read(BK_NJPD_SPARE04);
}

void HAL_NJPD_SetSpare05(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_SPARE05, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetSpare05 : 0x%04x\n", reg16_read(BK_NJPD_SPARE05));
}

__u16 HAL_NJPD_GetSpare05(void)
{
	return reg16_read(BK_NJPD_SPARE05);
}

void HAL_NJPD_SetSpare06(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_SPARE06, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetSpare06 : 0x%04x\n", reg16_read(BK_NJPD_SPARE06));
}

__u16 HAL_NJPD_GetSpare06(void)
{
	return reg16_read(BK_NJPD_SPARE06);
}

void HAL_NJPD_SetSpare07(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_SPARE07, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetSpare07 : 0x%04x\n", reg16_read(BK_NJPD_SPARE07));
}

__u16 HAL_NJPD_GetSpare07(void)
{
	return reg16_read(BK_NJPD_SPARE07);
}

void HAL_NJPD_SetWriteOneClearReg(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_WRITE_ONE_CLEAR, u16Value);
}

void HAL_NJPD_SetWriteOneClearReg_2(__u16 u16Value)
{
	reg16_write(BK_NJPD_WRITE_ONE_CLEAR, u16Value);
}

__u16 HAL_NJPD_GetWriteOneClearReg(void)
{
	return reg16_read(BK_NJPD_WRITE_ONE_CLEAR);
}

void HAL_NJPD_SetHTableStart_Low(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_HTABLE_START_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetHTableStart_Low : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_HTABLE_START_ADDR_L));
}

void HAL_NJPD_SetHTableStart_High(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_HTABLE_START_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetHTableStart_High : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_HTABLE_START_ADDR_H));
}

void HAL_NJPD_SetQTableStart_Low(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_QTABLE_START_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetQTableStart_Low : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_QTABLE_START_ADDR_L));
}

void HAL_NJPD_SetQTableStart_High(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_QTABLE_START_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetQTableStart_High : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_QTABLE_START_ADDR_H));
}

void HAL_NJPD_SetGTableStart_Low(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_GTABLE_START_ADDR_L, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetGTableStart_Low : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_GTABLE_START_ADDR_L));
}

void HAL_NJPD_SetGTableStart_High(__u16 u16Value)
{
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_MIU_GTABLE_START_ADDR_H, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetGTableStart_High : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_GTABLE_START_ADDR_H));
}

void HAL_NJPD_SetHTableSize(__u16 u16Value)
{
	const __u16 u16SetVal = 0x00FF;

	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	u16Value &= u16SetVal;
	reg16_write(BK_NJPD_MIU_HTABLE_SIZE, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetHTableSize : 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_HTABLE_SIZE));
}

__u16 HAL_NJPD_GetHTableSize(void)
{
	return reg16_read(BK_NJPD_MIU_HTABLE_SIZE);
}

void HAL_NJPD_SetRIUInterface(__u16 u16Value)
{
//    NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	reg16_write(BK_NJPD_TBC, u16Value);
}

__u16 HAL_NJPD_GetRIUInterface(void)
{
	return reg16_read(BK_NJPD_TBC);
}

__u16 HAL_NJPD_TBCReadData_L(void)
{
	return reg16_read(BK_NJPD_TBC_RDATA_L);
}

__u16 HAL_NJPD_TBCReadData_H(void)
{
	return reg16_read(BK_NJPD_TBC_RDATA_H);
}

void HAL_NJPD_SetIBufReadLength(__u8 u8Min, __u8 u8Max)
{
	__u16 u16Value;
	const __u16 u16SetVal = 0xfc00;

	u16Value = (u8Min << E_NUM5) + u8Max;
	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	u16Value = (u16Value | (reg16_read(BK_NJPD_IBUF_READ_LENGTH) & u16SetVal));
	reg16_write(BK_NJPD_IBUF_READ_LENGTH, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetIBufReadLength : 0x%04x\n",
			   reg16_read(BK_NJPD_IBUF_READ_LENGTH));
}


void HAL_NJPD_SetMRBurstThd(__u16 u16Value)
{
	const __u16 u16SetVal = 0xffe0;

	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16Value);
	u16Value = (u16Value | (reg16_read(BK_NJPD_MARB_SETTING_04) & u16SetVal));
	reg16_write(BK_NJPD_MARB_SETTING_04, u16Value);
	NJPD_DEBUG_HAL_MSG("HAL_NJPD_GetMARB_SETTING_04 : 0x%04x\n",
			   reg16_read(BK_NJPD_MARB_SETTING_04));
}



void HAL_NJPD_SetCRCReadMode(void)
{
	const __u16 u16SetReg = 0x200;

	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16SetReg);
	reg16_write(BK_NJPD_CRC_MODE, u16SetReg);
}

void HAL_NJPD_SetCRCWriteMode(void)
{
	const __u16 u16SetReg = 0x300;

	NJPD_DEBUG_HAL_MSG("%s : 0x%04x\n", __func__, u16SetReg);
	reg16_write(BK_NJPD_CRC_MODE, u16SetReg);
}

void HAL_NJPD_ResetMarb(void)
{
	const __u8 u8ByteFE = 0xFE;
	const __u8 u8Byte01 = 0x01;
	const __u8 u8ByteFD = 0xFD;
	const __u8 u8Byte02 = 0x02;

	NJPD_DEBUG_HAL_MSG("%s\n", __func__);
	NJPD_DEBUG_HAL_MSG("set BK_NJPD_MARB_RESET to 0x%02x\n",
			   reg8_read(BK_NJPD_MARB_RESET) & u8ByteFE);
	reg8_write(BK_NJPD_MARB_RESET, reg8_read(BK_NJPD_MARB_RESET) & u8ByteFE);
	NJPD_DEBUG_HAL_MSG("set BK_NJPD_MARB_RESET to 0x%02x\n",
			   reg8_read(BK_NJPD_MARB_RESET) | u8Byte01);
	reg8_write(BK_NJPD_MARB_RESET, reg8_read(BK_NJPD_MARB_RESET) | u8Byte01);
	NJPD_DEBUG_HAL_MSG("set BK_NJPD_MARB_RESET to 0x%02x\n",
			   reg8_read(BK_NJPD_MARB_RESET) & u8ByteFD);
	reg8_write(BK_NJPD_MARB_RESET, reg8_read(BK_NJPD_MARB_RESET) & u8ByteFD);
	NJPD_DEBUG_HAL_MSG("set BK_NJPD_MARB_RESET to 0x%02x\n",
			   reg8_read(BK_NJPD_MARB_RESET) | u8Byte02);
	reg8_write(BK_NJPD_MARB_RESET, reg8_read(BK_NJPD_MARB_RESET) | u8Byte02);

}


void HAL_NJPD_SetOutputFormat(__u8 bRst, NJPD_OutputFormat eOutputFormat)
{
	const __u16 u16SetGlobalSetting00 = 0xf3ff;
	const __u16 u16SetGlobalSetting01 = 0xfcff;

	switch (eOutputFormat) {
	case E_NJPD_OUTPUT_ORIGINAL:
		if (bRst) {
			HAL_NJPD_Set_GlobalSetting00(0);
			HAL_NJPD_ResetMarb();
			HAL_NJPD_Set_GlobalSetting01(HAL_NJPD_Get_GlobalSetting01() &
						     NJPD_GTABLE_RST);
		} else {
			HAL_NJPD_Set_GlobalSetting00(HAL_NJPD_Get_GlobalSetting00() &
				u16SetGlobalSetting00);
			HAL_NJPD_Set_GlobalSetting01(HAL_NJPD_Get_GlobalSetting01() &
				u16SetGlobalSetting01);
		}
		break;
	case E_NJPD_OUTPUT_YC_SWAP:
		if (bRst) {
			HAL_NJPD_Set_GlobalSetting00(NJPD_YC_SWAP);
			HAL_NJPD_Set_GlobalSetting01(HAL_NJPD_Get_GlobalSetting01() &
						     NJPD_GTABLE_RST);
		} else {
			HAL_NJPD_Set_GlobalSetting00((HAL_NJPD_Get_GlobalSetting00() &
				u16SetGlobalSetting00) | NJPD_YC_SWAP);
			HAL_NJPD_Set_GlobalSetting01(HAL_NJPD_Get_GlobalSetting01() &
				u16SetGlobalSetting01);
		}
		break;
	case E_NJPD_OUTPUT_UV_SWAP:
		if (bRst) {
			HAL_NJPD_Set_GlobalSetting00(NJPD_UV_SWAP);
			HAL_NJPD_Set_GlobalSetting01(HAL_NJPD_Get_GlobalSetting01() &
						     NJPD_GTABLE_RST);
		} else {
			HAL_NJPD_Set_GlobalSetting00((HAL_NJPD_Get_GlobalSetting00() &
				u16SetGlobalSetting00) | NJPD_UV_SWAP);
			HAL_NJPD_Set_GlobalSetting01(HAL_NJPD_Get_GlobalSetting01() &
				u16SetGlobalSetting01);
		}
		break;
	case E_NJPD_OUTPUT_UV_7BIT:
		if (bRst) {
			HAL_NJPD_Set_GlobalSetting00(0);
			HAL_NJPD_Set_GlobalSetting01((HAL_NJPD_Get_GlobalSetting01() &
						      NJPD_GTABLE_RST) | NJPD_UV_7BIT);
		} else {
			HAL_NJPD_Set_GlobalSetting00(HAL_NJPD_Get_GlobalSetting00() &
				u16SetGlobalSetting00);
			HAL_NJPD_Set_GlobalSetting01((HAL_NJPD_Get_GlobalSetting01() &
				u16SetGlobalSetting01) | NJPD_UV_7BIT);
		}

		break;
	case E_NJPD_OUTPUT_UV_MSB:
		if (bRst) {
			HAL_NJPD_Set_GlobalSetting00(0);
			HAL_NJPD_Set_GlobalSetting01((HAL_NJPD_Get_GlobalSetting01()
				& NJPD_GTABLE_RST) | (NJPD_UV_7BIT | NJPD_UV_MSB));
		} else {
			HAL_NJPD_Set_GlobalSetting00(HAL_NJPD_Get_GlobalSetting00() &
				u16SetGlobalSetting00);
			HAL_NJPD_Set_GlobalSetting01((HAL_NJPD_Get_GlobalSetting01()
				& u16SetGlobalSetting01) | NJPD_UV_7BIT | NJPD_UV_MSB);
		}
		break;
	}
}

void HAL_NJPD_Debug(void)
{
	__u16 u16i;
	const __u16 u16DebugBusSize = 0x7b;
	const __u16 u16SetBusReg = 0xFF;
	const __u16 u16DebugRegBaseSize = 0x80;

	NJPD_DEBUG_HAL_MSG("read BK_NJPD_MARB_CRC_RESULT_0: 0x%04x\n",
			   reg16_read(BK_NJPD_MARB_CRC_RESULT_0));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_MARB_CRC_RESULT_1: 0x%04x\n",
			   reg16_read(BK_NJPD_MARB_CRC_RESULT_1));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_MARB_CRC_RESULT_2: 0x%04x\n",
			   reg16_read(BK_NJPD_MARB_CRC_RESULT_2));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_MARB_CRC_RESULT_3: 0x%04x\n",
			   reg16_read(BK_NJPD_MARB_CRC_RESULT_3));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_GLOBAL_SETTING00: 0x%04x\n",
			   reg16_read(BK_NJPD_GLOBAL_SETTING00));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_GLOBAL_SETTING01: 0x%04x\n",
			   reg16_read(BK_NJPD_GLOBAL_SETTING01));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_GLOBAL_SETTING02: 0x%04x\n",
			   reg16_read(BK_NJPD_GLOBAL_SETTING02));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_MIU_READ_STATUS = 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_STATUS));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_MIU_READ_POINTER_ADDR_L: 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_POINTER_ADDR_L));
	NJPD_DEBUG_HAL_MSG("read BK_NJPD_MIU_READ_POINTER_ADDR_H: 0x%04x\n",
			   reg16_read(BK_NJPD_MIU_READ_POINTER_ADDR_H));

	for (u16i = 0; u16i <= u16DebugBusSize; u16i++) {
		reg16_write(BK_NJPD_DEBUG_BUS_SELECT, u16i);
		NJPD_DEBUG_HAL_MSG("BK_NJPD_DEBUG_BUS[H:L] = [0x%02x][0x%04x:0x%04x]\n", u16i,
				   reg16_read(BK_NJPD_DEBUG_BUS_H),
				   reg16_read(BK_NJPD_DEBUG_BUS_L));
	}
	reg16_write(BK_NJPD_DEBUG_BUS_SELECT, u16SetBusReg);
	NJPD_DEBUG_HAL_MSG("BK_NJPD_DEBUG_BUS[H:L] = [0x%02x][0x%04x:0x%04x]\n", u16SetBusReg,
			   reg16_read(BK_NJPD_DEBUG_BUS_H), reg16_read(BK_NJPD_DEBUG_BUS_L));

	NJPD_DEBUG_HAL_MSG("=======================================================\n");
	NJPD_DEBUG_HAL_MSG("NJPD | 00/08 01/09 02/0A 03/0B 04/0C 05/0D 06/0E 07/0F\n");
	NJPD_DEBUG_HAL_MSG("=======================================================\n");
	for (u16i = 0; u16i < u16DebugRegBaseSize; u16i += E_NUM8) {
		NJPD_DEBUG_HAL_MSG("%02x    | %04x %04x %04x %04x %04x %04x %04x %04x\n", u16i,
				   reg16_read(BK_NJPD_GENERAL(u16i + E_NUM0)),
				   reg16_read(BK_NJPD_GENERAL(u16i + E_NUM1)),
				   reg16_read(BK_NJPD_GENERAL(u16i + E_NUM2)),
				   reg16_read(BK_NJPD_GENERAL(u16i + E_NUM3)),
				   reg16_read(BK_NJPD_GENERAL(u16i + E_NUM4)),
				   reg16_read(BK_NJPD_GENERAL(u16i + E_NUM5)),
				   reg16_read(BK_NJPD_GENERAL(u16i + E_NUM6)),
				   reg16_read(BK_NJPD_GENERAL(u16i + E_NUM7))
		    );
	}
	NJPD_DEBUG_HAL_MSG("=======================================================\n");

	NJPD_DEBUG_HAL_MSG("=======================================================\n");
	NJPD_DEBUG_HAL_MSG("1231 | 00/08 01/09 02/0A 03/0B 04/0C 05/0D 06/0E 07/0F\n");
	NJPD_DEBUG_HAL_MSG("=======================================================\n");
	for (u16i = 0; u16i < u16DebugRegBaseSize; u16i += E_NUM8) {
		NJPD_DEBUG_HAL_MSG("%02x    | %04x %04x %04x %04x %04x %04x %04x %04x\n", u16i,
				   reg16_read(BK_NJPD_EXT_GENERAL(u16i + E_NUM0)),
				   reg16_read(BK_NJPD_EXT_GENERAL(u16i + E_NUM1)),
				   reg16_read(BK_NJPD_EXT_GENERAL(u16i + E_NUM2)),
				   reg16_read(BK_NJPD_EXT_GENERAL(u16i + E_NUM3)),
				   reg16_read(BK_NJPD_EXT_GENERAL(u16i + E_NUM4)),
				   reg16_read(BK_NJPD_EXT_GENERAL(u16i + E_NUM5)),
				   reg16_read(BK_NJPD_EXT_GENERAL(u16i + E_NUM6)),
				   reg16_read(BK_NJPD_EXT_GENERAL(u16i + E_NUM7))
		    );
	}
	NJPD_DEBUG_HAL_MSG("=======================================================\n");
}
