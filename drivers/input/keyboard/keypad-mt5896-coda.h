/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Owen Tseng <Owen.Tseng@mediatek.com>
 */
#ifndef _SAR_MT5896_CODA_H_
#define _SAR_MT5896_CODA_H_

//Page SAR
#define REG_0000_SAR (0x000)
    #define REG_SAR_SINGLE_CH Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_SAR_MODE Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_SAR_DIG_PD Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_SAR_START Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_SAR_PD Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_SAR_FREERUN Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_SAR_MEAN_EN Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_SINGLE Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_SAR_SW_RST Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_SAR_LOAD_EN Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_0004_SAR (0x004)
    #define REG_CKSAMP_PRD Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_KEYPAD_LEVEL Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_0008_SAR (0x008)
    #define REG_PWRGD_LVL Fld(2, 0, AC_MSKB0)//[1:0]
#define REG_000C_SAR (0x00C)
    #define REG_PM_DMY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0010_SAR (0x010)
    #define REG_SAR_DFT_DMY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0014_SAR (0x014)
    #define REG_VPLUG_SEL Fld(3, 0, AC_MSKB0)//[2:0]
    #define REG_DMSUS Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_SAR_1P8V_MODE_EN Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_0018_SAR (0x018)
    #define REG_EN_TRIM_VBG Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_TRVBG Fld(6, 1, AC_MSKB0)//[6:1]
    #define REG_PM_TEST Fld(6, 8, AC_MSKB1)//[13:8]
#define REG_0040_SAR (0x040)
    #define REG_SAR_AISEL Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0044_SAR (0x044)
    #define REG_DRV_SAR_GPIO Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0050_SAR (0x050)
    #define REG_C_SAR_GPIO Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0054_SAR (0x054)
    #define REG_SAR_TEST Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0058_SAR (0x058)
    #define REG_SAR_INT_MASK Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_005C_SAR (0x05C)
    #define REG_SAR_INT_CLR Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0060_SAR (0x060)
    #define REG_SAR_INT_FORCE Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0064_SAR (0x064)
    #define REG_SAR_INT_STATUS Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0068_SAR (0x068)
    #define REG_CMP_OUT Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_SAR_RDY Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_006C_SAR (0x06C)
    #define REG_SAR_REF_V_SEL_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0070_SAR (0x070)
    #define REG_SAR_REF_V_SEL_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0080_SAR (0x080)
    #define REG_SAR_CH0_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0084_SAR (0x084)
    #define REG_SAR_CH1_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0088_SAR (0x088)
    #define REG_SAR_CH2_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_008C_SAR (0x08C)
    #define REG_SAR_CH3_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0090_SAR (0x090)
    #define REG_SAR_CH4_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0094_SAR (0x094)
    #define REG_SAR_CH5_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0098_SAR (0x098)
    #define REG_SAR_CH6_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_009C_SAR (0x09C)
    #define REG_SAR_CH7_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00A0_SAR (0x0A0)
    #define REG_SAR_CH8_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00A4_SAR (0x0A4)
    #define REG_SAR_CH9_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00A8_SAR (0x0A8)
    #define REG_SAR_CHA_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00AC_SAR (0x0AC)
    #define REG_SAR_CHB_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00B0_SAR (0x0B0)
    #define REG_SAR_CHC_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00B4_SAR (0x0B4)
    #define REG_SAR_CHD_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00B8_SAR (0x0B8)
    #define REG_SAR_CHE_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00BC_SAR (0x0BC)
    #define REG_SAR_CHF_UPB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00C0_SAR (0x0C0)
    #define REG_SAR_CH0_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00C4_SAR (0x0C4)
    #define REG_SAR_CH1_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00C8_SAR (0x0C8)
    #define REG_SAR_CH2_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00CC_SAR (0x0CC)
    #define REG_SAR_CH3_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00D0_SAR (0x0D0)
    #define REG_SAR_CH4_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00D4_SAR (0x0D4)
    #define REG_SAR_CH5_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00D8_SAR (0x0D8)
    #define REG_SAR_CH6_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00DC_SAR (0x0DC)
    #define REG_SAR_CH7_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00E0_SAR (0x0E0)
    #define REG_SAR_CH8_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00E4_SAR (0x0E4)
    #define REG_SAR_CH9_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00E8_SAR (0x0E8)
    #define REG_SAR_CHA_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00EC_SAR (0x0EC)
    #define REG_SAR_CHB_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00F0_SAR (0x0F0)
    #define REG_SAR_CHC_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00F4_SAR (0x0F4)
    #define REG_SAR_CHD_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00F8_SAR (0x0F8)
    #define REG_SAR_CHE_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_00FC_SAR (0x0FC)
    #define REG_SAR_CHF_LOB Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0100_SAR (0x100)
    #define REG_SAR_ADC_CH0_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0104_SAR (0x104)
    #define REG_SAR_ADC_CH1_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0108_SAR (0x108)
    #define REG_SAR_ADC_CH2_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_010C_SAR (0x10C)
    #define REG_SAR_ADC_CH3_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0110_SAR (0x110)
    #define REG_SAR_ADC_CH4_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0114_SAR (0x114)
    #define REG_SAR_ADC_CH5_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0118_SAR (0x118)
    #define REG_SAR_ADC_CH6_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_011C_SAR (0x11C)
    #define REG_SAR_ADC_CH7_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0120_SAR (0x120)
    #define REG_SAR_ADC_CH8_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0124_SAR (0x124)
    #define REG_SAR_ADC_CH9_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0128_SAR (0x128)
    #define REG_SAR_ADC_CHA_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_012C_SAR (0x12C)
    #define REG_SAR_ADC_CHB_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0130_SAR (0x130)
    #define REG_SAR_ADC_CHC_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0134_SAR (0x134)
    #define REG_SAR_ADC_CHD_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0138_SAR (0x138)
    #define REG_SAR_ADC_CHE_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_013C_SAR (0x13C)
    #define REG_SAR_ADC_CHF_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0140_SAR (0x140)
    #define REG_SAR_ADC_CH10_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0144_SAR (0x144)
    #define REG_SAR_ADC_CH11_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0148_SAR (0x148)
    #define REG_SAR_ADC_CH12_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_014C_SAR (0x14C)
    #define REG_SAR_ADC_CH13_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0150_SAR (0x150)
    #define REG_SAR_ADC_CH14_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0154_SAR (0x154)
    #define REG_SAR_ADC_CH15_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0158_SAR (0x158)
    #define REG_SAR_ADC_CH16_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_015C_SAR (0x15C)
    #define REG_SAR_ADC_CH17_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0160_SAR (0x160)
    #define REG_SAR_ADC_CH18_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0164_SAR (0x164)
    #define REG_SAR_ADC_CH19_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0168_SAR (0x168)
    #define REG_SAR_ADC_CH1A_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_016C_SAR (0x16C)
    #define REG_SAR_ADC_CH1B_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0170_SAR (0x170)
    #define REG_SAR_ADC_CH1C_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0174_SAR (0x174)
    #define REG_SAR_ADC_CH1D_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_0178_SAR (0x178)
    #define REG_SAR_ADC_CH1E_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_017C_SAR (0x17C)
    #define REG_SAR_ADC_CH1F_DATA Fld(10, 0, AC_MSKW10)//[9:0]
#define REG_01C0_SAR (0x1C0)
    #define REG_SMCARD_INT_SEL Fld(3, 0, AC_MSKB0)//[2:0]
    #define REG_SMCARD_INT_LEVEL Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_SMCARD_INT_TIME_CNT_H Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_SMCARD_INT_TIME_CNT_L Fld(4, 8, AC_MSKB1)//[11:8]
    #define REG_SMCARD_INT_TIME_CNT_EN Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_SMCARD_INT_PULSE Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_SMCARD_INT Fld(1, 14, AC_MSKB1)//[14:14]

#endif
