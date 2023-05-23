/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define GE2_2412_BASE 0x00482400

//Page GE2
#define REG_0000_GE2 (GE2_2412_BASE + 0x00)
    #define REG_EN_GE_GCLK Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_EN_GE_MCM Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_EN_GE_BUSY_CNT_CLR Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_EN_GE_CHK_LFSR Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_EN_GE_CHK_CRC Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_EN_GE_ADC_STOP_DUMP Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_EN_GE_CDC_FIX Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_EN_GE_ADC_DEBUG Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0004_GE2 (GE2_2412_BASE + 0x04)
    #define REG_EN_GE_MIU2_PMA Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_EN_GE_MIU3_PMA Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_EN_GE_CHK_CLR_LFSR Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_EN_GE_CHK_CLR_CRC Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0008_GE2 (GE2_2412_BASE + 0x08)
    #define REG_EN_GE_SB_MTLB Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_EN_GE_DB_MTLB Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_0014_GE2 (GE2_2412_BASE + 0x14)
    #define REG_GE_BIST_STATUS3 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0020_GE2 (GE2_2412_BASE + 0x20)
    #define REG_GE_MIU2_PMA_LTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0024_GE2 (GE2_2412_BASE + 0x24)
    #define REG_GE_MIU2_PMA_LTH_1 Fld(15, 0, AC_MSKW10)//[14:0]
    #define REG_GE_MIU2_PMA_MODE Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0028_GE2 (GE2_2412_BASE + 0x28)
    #define REG_GE_MIU2_PMA_HTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_002C_GE2 (GE2_2412_BASE + 0x2C)
    #define REG_GE_MIU2_PMA_HTH_1 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_0030_GE2 (GE2_2412_BASE + 0x30)
    #define REG_GE_MIU3_PMA_LTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0034_GE2 (GE2_2412_BASE + 0x34)
    #define REG_GE_MIU3_PMA_LTH_1 Fld(15, 0, AC_MSKW10)//[14:0]
    #define REG_GE_MIU3_PMA_MODE Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0038_GE2 (GE2_2412_BASE + 0x38)
    #define REG_GE_MIU3_PMA_HTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_003C_GE2 (GE2_2412_BASE + 0x3C)
    #define REG_GE_MIU3_PMA_HTH_1 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_0048_GE2 (GE2_2412_BASE + 0x48)
    #define REG_GE_CHK_LFSR_INI Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0050_GE2 (GE2_2412_BASE + 0x50)
    #define REG_GE_CHK_CRC_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0054_GE2 (GE2_2412_BASE + 0x54)
    #define REG_GE_CHK_CRC_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0074_GE2 (GE2_2412_BASE + 0x74)
    #define REG_GE_MIUPROT_IRQ_MASK Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_GE_MIUPROT_IRQ_FORCE Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_GE_MIUPROT_IRQ_CLR Fld(4, 8, AC_MSKB1)//[11:8]
    #define REG_MIUPROT_IRQ_FINAL_STATUS Fld(4, 12, AC_MSKB1)//[15:12]
#define REG_0078_GE2 (GE2_2412_BASE + 0x78)
    #define REG_MIUPROT_IRQ_RAW_STATUS Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_0088_GE2 (GE2_2412_BASE + 0x88)
    #define REG_GE_MIU2_PMA_LTH_MSB Fld(2, 0, AC_MSKB0)//[1:0]
    #define REG_GE_MIU2_PMA_HTH_MSB Fld(2, 4, AC_MSKB0)//[5:4]
    #define REG_GE_MIU3_PMA_LTH_MSB Fld(2, 8, AC_MSKB1)//[9:8]
    #define REG_GE_MIU3_PMA_HTH_MSB Fld(2, 12, AC_MSKB1)//[13:12]
#define REG_00C4_GE2 (GE2_2412_BASE + 0xC4)
    #define REG_GE_BUSY_CNT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00C8_GE2 (GE2_2412_BASE + 0xC8)
    #define REG_GE_BUSY_CNT_1 Fld(16, 0, AC_FULLW10)//[15:0]

