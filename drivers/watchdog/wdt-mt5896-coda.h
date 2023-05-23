/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Owen Tseng <Owen.Tseng@mediatek.com>
 */
#ifndef _WDT_MT5896_CODA_H_
#define _WDT_MT5896_CODA_H_


//Page L4_MAIN
#define REG_0000_L4_MAIN (0x000)
    #define REG_RO_L4_MAIN_BANK_ID Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0004_L4_MAIN (0x004)
    #define REG_LZMA_DFS_EN Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_LZMA_DFS_DIV Fld(5, 8, AC_MSKB1)//[12:8]
#define REG_0008_L4_MAIN (0x008)
    #define REG_LZMA_DFS_UPDATE Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_000C_L4_MAIN (0x00C)
    #define REG_L4_CLK_CALC_EN Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_0010_L4_MAIN (0x010)
    #define REG_RO_L4_CALC_DONE Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_0014_L4_MAIN (0x014)
    #define REG_RO_L4_CALC_CNT_REPORT Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0040_L4_MAIN (0x040)
    #define REG_ACPU_STANDBY_WFI_SEL Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_ACPU_STANDBY_WFI Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0048_L4_MAIN (0x048)
    #define REG_PA_OPT_ENABLE Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_004C_L4_MAIN (0x04C)
    #define REG_MPU_SMI_OSD_MAX Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0050_L4_MAIN (0x050)
    #define REG_MPU_SMI_ARPREULTRA Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_MPU_SMI_ARULTRA Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_MPU_SMI_AWPREULTRA Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_MPU_SMI_AWULTRA Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_GALS_MST_SYNC_SEL Fld(2, 4, AC_MSKB0)//[5:4]
    #define REG_GALS_SLV_SYNC_SEL Fld(2, 6, AC_MSKB0)//[7:6]
    #define REG_GALS_SAMPLE_SEL Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_RO_GALS_INT_AXI_IDLE Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_RO_GALS_SLPPROT_AXI_IDLE_ASYNC Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_ACPU_FAKE_ROM_SEL_LATCH Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_ACPU_FAKE_ROM_SEL_VALUE Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_0054_L4_MAIN (0x054)
    #define REG_RO_GALS_AXI_DBG_MST_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0058_L4_MAIN (0x058)
    #define REG_RO_GALS_AXI_DBG_MST_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0080_L4_MAIN (0x080)
    #define REG_L4_INT_STATUS_CLEAR Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_0084_L4_MAIN (0x084)
    #define REG_L4_BRIDGE_DEBUG_SEL Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0088_L4_MAIN (0x088)
    #define REG_RO_L4_BRIDGE_DEBUG_OUT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_008C_L4_MAIN (0x08C)
    #define REG_RO_L4_BRIDGE_DEBUG_OUT_1 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0090_L4_MAIN (0x090)
    #define REG_RO_L4_BRIDGE_IDLE Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_0094_L4_MAIN (0x094)
    #define REG_RO_L4_BRIDGE_IDLE_STATUS_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0098_L4_MAIN (0x098)
    #define REG_RO_L4_BRIDGE_IDLE_STATUS_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_009C_L4_MAIN (0x09C)
    #define REG_RO_L4_BRIDGE_IDLE_STATUS_2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00A0_L4_MAIN (0x0A0)
    #define REG_RO_L4_BRIDGE_IDLE_STATUS_3 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00A4_L4_MAIN (0x0A4)
    #define REG_RO_L4_BRIDGE_IDLE_STATUS_4 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00A8_L4_MAIN (0x0A8)
    #define REG_RO_L4_BRIDGE_IDLE_STATUS_5 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00AC_L4_MAIN (0x0AC)
    #define REG_RO_L4_BRIDGE_IDLE_STATUS_6 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00B0_L4_MAIN (0x0B0)
    #define REG_RO_L4_BRIDGE_IDLE_STATUS_7 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00B4_L4_MAIN (0x0B4)
    #define REG_RO_L4_BRIDGE_INT Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_00B8_L4_MAIN (0x0B8)
    #define REG_RO_L4_BRIDGE_INT_STATUS Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00C0_L4_MAIN (0x0C0)
    #define REG_RO_L4_DEV_NODEF_HIT Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_RO_L4_DEV_NODEF_RW Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_00C4_L4_MAIN (0x0C4)
    #define REG_RO_L4_DEV_NODEF_ADDR_0 Fld(12, 4, AC_MSKW10)//[15:4]
#define REG_00C8_L4_MAIN (0x0C8)
    #define REG_RO_L4_DEV_NODEF_ADDR_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00CC_L4_MAIN (0x0CC)
    #define REG_RO_L4_AXI_PERIPHERAL_IDLE Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_RO_L4_AXI2DEV_R_IDLE Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_RO_L4_AXI2DEV_W_IDLE Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_RO_L4_RXIU_IDLE Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_0118_L4_MAIN (0x118)
    #define REG_APU_LASTDONZ_IGNORE Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_IMI_LASTDONZ_IGNORE Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_0120_L4_MAIN (0x120)
    #define REG_DEV_CTRL_DAT_MTCYC Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_0124_L4_MAIN (0x124)
    #define REG_DEV_CTRL_TIMEOUT_MAX Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0128_L4_MAIN (0x128)
    #define REG_IMI_CTRL_TIMEOUT_MAX Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_012C_L4_MAIN (0x12C)
    #define REG_PCIE_CTRL_TIMEOUT_MAX Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0130_L4_MAIN (0x130)
    #define REG_APU_CTRL_TIMEOUT_MAX Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0134_L4_MAIN (0x134)
    #define REG_AXI_PERI_EXCLU_ACCESS_EN Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_AXI_PERI_EXCLU_R_TWICE_PREVENT_EN Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_RO_RPAK_CASE00_ONCE Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_RO_RPAK_CASE01_ONCE Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_RO_RPAK_CASE02_ONCE Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_RO_RPAK_CASE03_ONCE Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_0138_L4_MAIN (0x138)
    #define REG_GIC_USER_ID_MASK Fld(3, 0, AC_MSKB0)//[2:0]
    #define REG_GIC_SELECT_EN Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_GIC_SELECT_VALUE Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_RO_GICCDISABLE_VALUE Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_0140_L4_MAIN (0x140)
    #define REG_FORCE_W_FUNNEL_SEL Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_FORCE_W_FUNNEL_EN Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_FORCE_R_FUNNEL_SEL Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_FORCE_R_FUNNEL_EN Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_RO_AXI_R_PERI_CNT_IDLE_M0 Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_RO_AXI_W_PERI_CNT_IDLE_M0 Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_RO_AXI_R_PERI_CNT_IDLE_M1 Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_RO_AXI_W_PERI_CNT_IDLE_M1 Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_RO_DEV2AXI_RFIFO_FULL Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_RO_DEV2AXI_WFIFO_FULL Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_AXI_MONITOR_PERI_EN Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0144_L4_MAIN (0x144)
    #define REG_RO_AXI_R_PERI_CNT_M0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0148_L4_MAIN (0x148)
    #define REG_RO_AXI_W_PERI_CNT_M0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_014C_L4_MAIN (0x14C)
    #define REG_RO_AXI_R_PERI_CNT_M1 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0150_L4_MAIN (0x150)
    #define REG_RO_AXI_W_PERI_CNT_M1 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0180_L4_MAIN (0x180)
    #define REG_WDT_CLR_TOG Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_0188_L4_MAIN (0x188)
    #define REG_RO_WDT_1ST_RST_FLAG Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_RO_WDT_1ST_RST_CNT_FLAG Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_WDT_1ST_RST_LEN Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_018C_L4_MAIN (0x18C)
    #define REG_WDT_1ST_INT Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0190_L4_MAIN (0x190)
    #define REG_WDT_1ST_MAX_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0194_L4_MAIN (0x194)
    #define REG_WDT_1ST_MAX_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0198_L4_MAIN (0x198)
    #define REG_RO_WDT_2ND_RST_FLAG Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_WDT_ENABLE_1ST Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_WDT_ENABLE_2ND Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_WDT_2ND_MAX Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_019C_L4_MAIN (0x19C)
    #define REG_RO_WDT_INT Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_01C0_L4_MAIN (0x1C0)
    #define REG_L4_DUMMY1_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01C4_L4_MAIN (0x1C4)
    #define REG_L4_DUMMY1_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01C8_L4_MAIN (0x1C8)
    #define REG_L4_DUMMY1_2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01CC_L4_MAIN (0x1CC)
    #define REG_L4_DUMMY1_3 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01D0_L4_MAIN (0x1D0)
    #define REG_RO_L4_DUMMY1_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01D4_L4_MAIN (0x1D4)
    #define REG_RO_L4_DUMMY1_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01D8_L4_MAIN (0x1D8)
    #define REG_RO_L4_DUMMY1_2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01DC_L4_MAIN (0x1DC)
    #define REG_RO_L4_DUMMY1_3 Fld(16, 0, AC_FULLW10)//[15:0]

#endif
