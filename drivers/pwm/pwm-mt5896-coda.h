/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Owen Tseng <Owen.Tseng@mediatek.com>
 */
#ifndef _PWM_MT5896_CODA_H_
#define _PWM_MT5896_CODA_H_

//Page PWM_1
#define REG_0004_PWM (0x004)
#define REG_PWM_CLKEN_BITMAP Fld(7, 0, AC_MSKB0)//[6:0]
#define REG_0008_PWM (0x008)
#define REG_PWM0_PERIOD Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_000C_PWM (0x00C)
#define REG_PWM0_DUTY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0010_PWM (0x010)
#define REG_PWM0_DIV Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM0_POLARITY Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM0_VDBEN Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM0_RESET_EN Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM0_DBEN Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_PWM0_IMPULSE_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_PWM0_ODDEVEN_SYNC Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_PWM0_VDBEN_SW Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_PWM0_SYNC_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0014_PWM (0x014)
#define REG_PWM1_PERIOD Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0018_PWM (0x018)
#define REG_PWM1_DUTY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_001C_PWM (0x01C)
#define REG_PWM1_DIV Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM1_POLARITY Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM1_VDBEN Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM1_RESET_EN Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM1_DBEN Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_PWM1_IMPULSE_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_PWM1_ODDEVEN_SYNC Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_PWM1_VDBEN_SW Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_PWM1_SYNC_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0020_PWM (0x020)
#define REG_PWM2_PERIOD Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0024_PWM (0x024)
#define REG_PWM2_DUTY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0028_PWM (0x028)
#define REG_PWM2_DIV Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM2_POLARITY Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM2_VDBEN Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM2_RESET_EN Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM2_DBEN Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_PWM2_IMPULSE_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_PWM2_ODDEVEN_SYNC Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_PWM2_VDBEN_SW Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_PWM2_SYNC_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_002C_PWM (0x02C)
#define REG_PWM3_PERIOD Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0030_PWM (0x030)
#define REG_PWM3_DUTY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0034_PWM (0x034)
#define REG_PWM3_DIV Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM3_POLARITY Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM3_VDBEN Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM3_RESET_EN Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM3_DBEN Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_PWM3_IMPULSE_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_PWM3_ODDEVEN_SYNC Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_PWM3_VDBEN_SW Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_PWM3_SYNC_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0038_PWM (0x038)
#define REG_PWM4_PERIOD Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_003C_PWM (0x03C)
#define REG_PWM4_DUTY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0040_PWM (0x040)
#define REG_PWM4_DIV Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM4_POLARITY Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM4_VDBEN Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM4_RESET_EN Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM4_DBEN Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_PWM4_IMPULSE_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_PWM4_ODDEVEN_SYNC Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_PWM4_VDBEN_SW Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_PWM4_SYNC_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0044_PWM (0x044)
#define REG_PWM5_PERIOD Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0048_PWM (0x048)
#define REG_PWM5_DUTY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_004C_PWM (0x04C)
#define REG_PWM5_DIV Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM5_POLARITY Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM5_VDBEN Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM5_RESET_EN Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM5_DBEN Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_PWM5_IMPULSE_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_PWM5_ODDEVEN_SYNC Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_PWM5_VDBEN_SW Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_PWM5_SYNC_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0050_PWM (0x050)
#define REG_RST_MUX0 Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_HS_RST_CNT0 Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_RST_MUX1 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_HS_RST_CNT1 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_0054_PWM (0x054)
#define REG_RST_MUX2 Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_HS_RST_CNT2 Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_RST_MUX3 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_HS_RST_CNT3 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_0058_PWM (0x058)
#define REG_RST_MUX4 Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_HS_RST_CNT4 Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_RST_MUX5 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_HS_RST_CNT5 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_005C_PWM (0x05C)
#define REG_IMPULSE_DUTY_SEL Fld(3, 0, AC_MSKB0)//[2:0]
#define REG_0060_PWM (0x060)
#define REG_PWM0_RST_BY_OTHER_PWM Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM0_RST_OTHER_SEL Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_PROC_ST0 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_IMPULSE_DUTY0 Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0064_PWM (0x064)
#define REG_PWM1_RST_BY_OTHER_PWM Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM1_RST_OTHER_SEL Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_PROC_ST1 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_IMPULSE_DUTY1 Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0068_PWM (0x068)
#define REG_PWM2_RST_BY_OTHER_PWM Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM2_RST_OTHER_SEL Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_PROC_ST2 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_IMPULSE_DUTY2 Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_006C_PWM (0x06C)
#define REG_PWM3_RST_BY_OTHER_PWM Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM3_RST_OTHER_SEL Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_PROC_ST3 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_IMPULSE_DUTY3 Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0070_PWM (0x070)
#define REG_PWM4_RST_BY_OTHER_PWM Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM4_RST_OTHER_SEL Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_PROC_ST4 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_IMPULSE_DUTY4 Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0074_PWM (0x074)
#define REG_PWM5_RST_BY_OTHER_PWM Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM5_RST_OTHER_SEL Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_PROC_ST5 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_IMPULSE_DUTY5 Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0078_PWM (0x078)
#define REG_IMPULSE_DUTY6 Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_007C_PWM (0x07C)
#define REG_IMPULSE_DUTY7 Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0080_PWM (0x080)
#define REG_PWM0_PERIOD_EXT Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_PWM1_PERIOD_EXT Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_PWM2_PERIOD_EXT Fld(2, 4, AC_MSKB0)//[5:4]
#define REG_PWM3_PERIOD_EXT Fld(2, 6, AC_MSKB0)//[7:6]
#define REG_PWM4_PERIOD_EXT Fld(2, 8, AC_MSKB1)//[9:8]
#define REG_PWM5_PERIOD_EXT Fld(2, 10, AC_MSKB1)//[11:10]
#define REG_0084_PWM (0x084)
#define REG_PWM0_DUTY_EXT Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_PWM1_DUTY_EXT Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_PWM2_DUTY_EXT Fld(2, 4, AC_MSKB0)//[5:4]
#define REG_PWM3_DUTY_EXT Fld(2, 6, AC_MSKB0)//[7:6]
#define REG_PWM4_DUTY_EXT Fld(2, 8, AC_MSKB1)//[9:8]
#define REG_PWM5_DUTY_EXT Fld(2, 10, AC_MSKB1)//[11:10]
#define REG_0088_PWM (0x088)
#define REG_PWM0_DIV_EXT Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM1_DIV_EXT Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_008C_PWM (0x08C)
#define REG_PWM2_DIV_EXT Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM3_DIV_EXT Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0090_PWM (0x090)
#define REG_PWM4_DIV_EXT Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_PWM5_DIV_EXT Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0094_PWM (0x094)
#define REG_PWM_SW_TEST_EN0 Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_PWM_SW_TEST_EN1 Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_PWM_SW_TEST_EN2 Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_PWM_SW_TEST_EN3 Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_PWM_SW_TEST_EN4 Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_PWM_SW_TEST_EN5 Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_PWM_SW_LR_VSP_SEL0 Fld(1, 6, AC_MSKB0)//[6:6]
#define REG_PWM_SW_LR_VSP_SEL1 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_PWM_SW_LR_VSP_SEL2 Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM_SW_LR_VSP_SEL3 Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM_SW_LR_VSP_SEL4 Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM_SW_LR_VSP_SEL5 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_0098_PWM (0x098)
#define REG_MOD_DBG_SEL0 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_MOD_DBG_SEL1 Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_MOD_DBG_SEL2 Fld(2, 4, AC_MSKB0)//[5:4]
#define REG_MOD_DBG_SEL3 Fld(2, 6, AC_MSKB0)//[7:6]
#define REG_MOD_DBG_SEL4 Fld(2, 8, AC_MSKB1)//[9:8]
#define REG_MOD_DBG_SEL5 Fld(2, 10, AC_MSKB1)//[11:10]
#define REG_MOD_DBG_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_00A0_PWM (0x0A0)
#define REG_PWM0_SHIFT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00A4_PWM (0x0A4)
#define REG_PWM0_SHIFT_1 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_PWM0_O_CTRL Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_PWM0_SHIFT_GAT Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_00A8_PWM (0x0A8)
#define REG_PWM1_SHIFT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00AC_PWM (0x0AC)
#define REG_PWM1_SHIFT_1 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_PWM1_O_CTRL Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_PWM1_SHIFT_GAT Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_00B0_PWM (0x0B0)
#define REG_PWM2_SHIFT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00B4_PWM (0x0B4)
#define REG_PWM2_SHIFT_1 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_PWM2_O_CTRL Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_PWM2_SHIFT_GAT Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_00B8_PWM (0x0B8)
#define REG_PWM3_SHIFT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00BC_PWM (0x0BC)
#define REG_PWM3_SHIFT_1 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_PWM3_O_CTRL Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_PWM3_SHIFT_GAT Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_00C0_PWM (0x0C0)
#define REG_PWM4_SHIFT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00C4_PWM (0x0C4)
#define REG_PWM4_SHIFT_1 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_PWM4_O_CTRL Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_PWM4_SHIFT_GAT Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_00C8_PWM (0x0C8)
#define REG_PWM5_SHIFT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00CC_PWM (0x0CC)
#define REG_PWM5_SHIFT_1 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_PWM5_O_CTRL Fld(2, 2, AC_MSKB0)//[3:2]
#define REG_PWM5_SHIFT_GAT Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_00D0_PWM (0x0D0)
#define REG_NVS_RST_EN0 Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_NVS_RST_EN1 Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_NVS_RST_EN2 Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_NVS_RST_EN3 Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_NVS_RST_EN4 Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_NVS_RST_EN5 Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_NVS_ALIGN_INV0 Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_NVS_ALIGN_INV1 Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_NVS_ALIGN_INV2 Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_NVS_ALIGN_INV3 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_NVS_ALIGN_INV4 Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_NVS_ALIGN_INV5 Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_SYNC_LR_ALIGN_EN Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_SYNC_LEFT_FLAG_INV Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_00D4_PWM (0x0D4)
#define REG_NVS_ALIGN_EN0 Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_NVS_ALIGN_EN1 Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_NVS_ALIGN_EN2 Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_NVS_ALIGN_EN3 Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_NVS_ALIGN_EN4 Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_NVS_ALIGN_EN5 Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_FORCE_SHIFT_SET_EN0 Fld(1, 6, AC_MSKB0)//[6:6]
#define REG_FORCE_SHIFT_SET_EN1 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_FORCE_SHIFT_SET_EN2 Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_FORCE_SHIFT_SET_EN3 Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_FORCE_SHIFT_SET_EN4 Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_FORCE_SHIFT_SET_EN5 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_00D8_PWM (0x0D8)
#define REG_PWM_DUMMY2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00DC_PWM (0x0DC)
#define REG_PWM_DUMMY3 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00E0_PWM (0x0E0)
#define REG_LC_HIT_SEL Fld(3, 0, AC_MSKB0)//[2:0]
#define REG_DIFF_P_EN Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_PWM0_DIFF_P_EXT Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_PWM1_DIFF_P_EXT Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_PWM_OUT_SHIFT_SEL Fld(2, 8, AC_MSKB1)//[9:8]
#define REG_COMB_PWM2_SEL Fld(2, 10, AC_MSKB1)//[11:10]
#define REG_COMB_PWM0_SEL Fld(2, 12, AC_MSKB1)//[13:12]
#define REG_V_CNT_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_00E4_PWM (0x0E4)
#define REG_LINE_CNT_RPT Fld(13, 0, AC_MSKW10)//[12:0]
#define REG_0100_PWM (0x100)
#define REG_DIM_TRG_EN Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_LR_ALIGN_EN Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_LR_FLAG_INV Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_DIM_TRG_SRC_SEL Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_DIM00_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0104_PWM (0x104)
#define REG_N_TRG_CNT Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_DIM01_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0108_PWM (0x108)
#define REG_N_HSYNC_CNT Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_DIM02_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_010C_PWM (0x10C)
#define REG_DIM03_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0110_PWM (0x110)
#define REG_DIM04_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0114_PWM (0x114)
#define REG_DIM05_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0118_PWM (0x118)
#define REG_DIM06_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_011C_PWM (0x11C)
#define REG_DIM07_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0120_PWM (0x120)
#define REG_DIM08_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0124_PWM (0x124)
#define REG_DIM09_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0128_PWM (0x128)
#define REG_DIM10_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_012C_PWM (0x12C)
#define REG_DIM11_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0130_PWM (0x130)
#define REG_DIM12_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0134_PWM (0x134)
#define REG_DIM13_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0138_PWM (0x138)
#define REG_DIM14_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_013C_PWM (0x13C)
#define REG_DIM15_TRG Fld(12, 0, AC_MSKW10)//[11:0]
#define REG_0140_PWM (0x140)
#define REG_PWM0_SHIFT4 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0144_PWM (0x144)
#define REG_PWM0_DUTY4 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0148_PWM (0x148)
#define REG_PWM1_SHIFT4 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_014C_PWM (0x14C)
#define REG_PWM1_DUTY4 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0150_PWM (0x150)
#define REG_PWM0_EN_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM0_HIT_CNT_ST Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_0154_PWM (0x154)
#define REG_PWM0_HIT_CNT_END Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0158_PWM (0x158)
#define REG_PWM1_EN_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM1_HIT_CNT_ST Fld(15, 0, AC_MSKW10)//[14:0]

//Page PWM_2
#define REG_015C_PWM (0x15C)
#define REG_PWM1_HIT_CNT_END Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0160_PWM (0x160)
#define REG_PWM2_EN_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM2_HIT_CNT_ST Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_0164_PWM (0x164)
#define REG_PWM2_HIT_CNT_END Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0168_PWM (0x168)
#define REG_PWM3_EN_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM3_HIT_CNT_ST Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_016C_PWM (0x16C)
#define REG_PWM3_HIT_CNT_END Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0170_PWM (0x170)
#define REG_PWM4_EN_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM4_HIT_CNT_ST Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_0174_PWM (0x174)
#define REG_PWM4_HIT_CNT_END Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0178_PWM (0x178)
#define REG_PWM5_EN_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM5_HIT_CNT_ST Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_017C_PWM (0x17C)
#define REG_PWM5_HIT_CNT_END Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0180_PWM (0x180)
#define REG_PWM_DUMMY0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0184_PWM (0x184)
#define REG_PWM_DUMMY1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0188_PWM (0x188)
#define REG_PWM0_LR_SRC Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_PWM1_LR_SRC Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_PWM2_LR_SRC Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_PWM3_LR_SRC Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_PWM4_LR_SRC Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_PWM5_LR_SRC Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_LR_RP_FP_SEL Fld(1, 6, AC_MSKB0)//[6:6]
#define REG_PWM0_DB_SRC Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM1_DB_SRC Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM2_DB_SRC Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM3_DB_SRC Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_PWM4_DB_SRC Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_PWM5_DB_SRC Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_0190_PWM (0x190)
#define REG_PWM0_LEFT_MASK Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_PWM1_LEFT_MASK Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_PWM2_LEFT_MASK Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_PWM3_LEFT_MASK Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_PWM4_LEFT_MASK Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_PWM5_LEFT_MASK Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_0194_PWM (0x194)
#define REG_PWM0_INV_LEFT Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_PWM1_INV_LEFT Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_PWM2_INV_LEFT Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_PWM3_INV_LEFT Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_PWM4_INV_LEFT Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_PWM5_INV_LEFT Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_0198_PWM (0x198)
#define REG_EN_RP_L_INT0 Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_EN_FP_L_INT0 Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_EN_RP_L_INT1 Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_EN_FP_L_INT1 Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_EN_RP_L_INT2 Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_EN_FP_L_INT2 Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_EN_RP_L_INT3 Fld(1, 6, AC_MSKB0)//[6:6]
#define REG_EN_FP_L_INT3 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_EN_RP_L_INT4 Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_EN_FP_L_INT4 Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_EN_RP_L_INT5 Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_EN_FP_L_INT5 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_019C_PWM (0x19C)
#define REG_EN_RP_R_INT0 Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_EN_FP_R_INT0 Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_EN_RP_R_INT1 Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_EN_FP_R_INT1 Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_EN_RP_R_INT2 Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_EN_FP_R_INT2 Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_EN_RP_R_INT3 Fld(1, 6, AC_MSKB0)//[6:6]
#define REG_EN_FP_R_INT3 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_EN_RP_R_INT4 Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_EN_FP_R_INT4 Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_EN_RP_R_INT5 Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_EN_FP_R_INT5 Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_01A0_PWM (0x1A0)
#define REG_PWM0_EN_LR_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM0_HIT_CNT_ST2 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_01A4_PWM (0x1A4)
#define REG_PWM0_HIT_CNT_END2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01A8_PWM (0x1A8)
#define REG_PWM1_EN_LR_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM1_HIT_CNT_ST2 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_01AC_PWM (0x1AC)
#define REG_PWM1_HIT_CNT_END2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01B0_PWM (0x1B0)
#define REG_PWM2_EN_LR_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM2_HIT_CNT_ST2 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_01B4_PWM (0x1B4)
#define REG_PWM2_HIT_CNT_END2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01B8_PWM (0x1B8)
#define REG_PWM3_EN_LR_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM3_HIT_CNT_ST2 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_01BC_PWM (0x1BC)
#define REG_PWM3_HIT_CNT_END2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01C0_PWM (0x1C0)
#define REG_PWM4_EN_LR_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM4_HIT_CNT_ST2 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_01C4_PWM (0x1C4)
#define REG_PWM4_HIT_CNT_END2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01C8_PWM (0x1C8)
#define REG_PWM5_EN_LR_MASK Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_PWM5_HIT_CNT_ST2 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_01CC_PWM (0x1CC)
#define REG_PWM5_HIT_CNT_END2 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01D8_PWM (0x1D8)
#define REG_GPO2PWM0_SEL Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_GPO2PWM1_SEL Fld(4, 4, AC_MSKB0)//[7:4]
#define REG_GPO2PWM2_SEL Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_GPO2PWM3_SEL Fld(4, 12, AC_MSKB1)//[15:12]
#define REG_01DC_PWM (0x1DC)
#define REG_GPO2PWM4_SEL Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_GPO2PWM5_SEL Fld(4, 4, AC_MSKB0)//[7:4]
#define REG_GPO2PWM0_EN Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_GPO2PWM1_EN Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_GPO2PWM2_EN Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_GPO2PWM3_EN Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_GPO2PWM4_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_GPO2PWM5_EN Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_01E0_PWM (0x1E0)
#define REG_PWM0_LR_SYNC_RP Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_PWM1_LR_SYNC_RP Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_PWM2_LR_SYNC_RP Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_PWM3_LR_SYNC_RP Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_PWM4_LR_SYNC_RP Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_PWM5_LR_SYNC_RP Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_PWM0_LR_SYNC_FP Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_PWM1_LR_SYNC_FP Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_PWM2_LR_SYNC_FP Fld(1, 10, AC_MSKB1)//[10:10]
#define REG_PWM3_LR_SYNC_FP Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_PWM4_LR_SYNC_FP Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_PWM5_LR_SYNC_FP Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_INV_3D_FLAG Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_01E4_PWM (0x1E4)
#define REG_DET1_RP_DLY1T_EN Fld(1, 12, AC_MSKB1)//[12:12]
#define REG_DET0_RST_DIV_EN Fld(1, 11, AC_MSKB1)//[11:11]
#define REG_DEGLITCH_SEL Fld(3, 8, AC_MSKB1)//[10:8]
#define REG_DET_DIV Fld(4, 4, AC_MSKB0)//[7:4]
#define REG_SRC_INV Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_SRC_SWAP Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_PAUSE_DET Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_DET_EN Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_01E8_PWM (0x1E8)
#define REG_DET_PRD_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01EC_PWM (0x1EC)
#define REG_DET_PRD_1 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_RESERVED_1 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_01F0_PWM (0x1F0)
#define REG_DET_DUTY_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01F4_PWM (0x1F4)
#define REG_DET_DUTY_1 Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_DET_PH_DIF_EXT Fld(2, 4, AC_MSKB0)//[5:4]
#define REG_RESERVED_0 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_01F8_PWM (0x1F8)
#define REG_DET_PH_DIF Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01FC_PWM (0x1FC)
#define REG_DIM_IRE_CLKEN_BITMAP Fld(14, 0, AC_MSKW10)//[13:0]

#endif
