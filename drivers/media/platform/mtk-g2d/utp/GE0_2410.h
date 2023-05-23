/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define GE0_2410_BASE 0x00482000

//Page GE0_1
#define REG_0000_GE0 (GE0_2410_BASE + 0x000)
    #define REG_EN_GE Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_EN_GE_DITHER Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_EN_GE_ABL Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_EN_GE_ABL_SCK Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_EN_GE_ABL_DCK Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_EN_GE_ROP Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_EN_GE_SCK Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_EN_GE_DCK Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_EN_GE_LPT Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_EN_GE_RN Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_EN_GE_DFB Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_EN_GE_ALPHA_CMP Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_EN_GE_DSTAC Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_EN_GE_CALC_S_WH Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_EN_GE_BURST Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_EN_GE_ONE_PIXEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0004_GE0 (GE0_2410_BASE + 0x004)
    #define REG_EN_GE_CMQ Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_EN_GE_VCMQ Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_EN_GE_PRIORITY Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_EN_GE_DISABLE_MIU Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_EN_GE_STBB Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_EN_GE_ITC Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_EN_GE_LENGTH_LIMIT Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_EN_GE_NO_WBE Fld(2, 8, AC_MSKB1)//[9:8]
    #define REG_EN_GE_MIU0_PMA Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_EN_GE_MIU1_PMA Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_EN_GE_CLR_MIU_INVALID Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_SRAM_SD_EN Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_EN_GE_RW_SPLIT Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_EN_GE_CLKGATE Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0008_GE0 (GE0_2410_BASE + 0x008)
    #define REG_GE_DBG Fld(6, 0, AC_MSKB0)//[5:0]
    #define REG_GE_CMQ_DBG Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_GE_UNFREEZE Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_GE_LENGTH_TH Fld(6, 8, AC_MSKB1)//[13:8]
    #define REG_GE_LENGTH_AUTO_POL Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_GE_LENGTH_MODE Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_000C_GE0 (GE0_2410_BASE + 0x00C)
    #define REG_GE_STBB_TH Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_GE_VCMQ_W_TH Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_GE_VCMQ_R_TH Fld(4, 8, AC_MSKB1)//[11:8]
    #define REG_EN_GE_ARB_PRIORITY Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_EN_GE_MIU_2PORT Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0010_GE0 (GE0_2410_BASE + 0x010)
    #define REG_GE_VCMQ_STATUS_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0014_GE0 (GE0_2410_BASE + 0x014)
    #define REG_GE_VCMQ_STATUS_1 Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_GE_BIST_STATUS Fld(7, 1, AC_MSKB0)//[7:1]
    #define REG_GE_DBG_STATUS0 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0018_GE0 (GE0_2410_BASE + 0x018)
    #define REG_GE_DBG_STATUS1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_001C_GE0 (GE0_2410_BASE + 0x01C)
    #define REG_GE_BUSY Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_GE_BIST_STATUS2 Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_GE_CMQ1_STATUS Fld(5, 3, AC_MSKB0)//[7:3]
    #define REG_GE_CMQ2_STATUS Fld(5, 11, AC_MSKB1)//[15:11]
#define REG_0020_GE0 (GE0_2410_BASE + 0x020)
    #define REG_GE_MIU0_PMA_LTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0024_GE0 (GE0_2410_BASE + 0x024)
    #define REG_GE_MIU0_PMA_LTH_1 Fld(15, 0, AC_MSKW10)//[14:0]
    #define REG_GE_MIU0_PMA_MODE Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0028_GE0 (GE0_2410_BASE + 0x028)
    #define REG_GE_MIU0_PMA_HTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_002C_GE0 (GE0_2410_BASE + 0x02C)
    #define REG_GE_MIU0_PMA_HTH_1 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_0030_GE0 (GE0_2410_BASE + 0x030)
    #define REG_GE_MIU1_PMA_LTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0034_GE0 (GE0_2410_BASE + 0x034)
    #define REG_GE_MIU1_PMA_LTH_1 Fld(15, 0, AC_MSKW10)//[14:0]
    #define REG_GE_MIU1_PMA_MODE Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0038_GE0 (GE0_2410_BASE + 0x038)
    #define REG_GE_MIU1_PMA_HTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_003C_GE0 (GE0_2410_BASE + 0x03C)
    #define REG_GE_MIU1_PMA_HTH_1 Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_0040_GE0 (GE0_2410_BASE + 0x040)
    #define REG_GE_ROP2 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_0044_GE0 (GE0_2410_BASE + 0x044)
    #define REG_GE_ABL_COEF Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_GE_ABL_1555 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0048_GE0 (GE0_2410_BASE + 0x048)
    #define REG_GE_DB_ABL Fld(5, 8, AC_MSKB1)//[12:8]
#define REG_004C_GE0 (GE0_2410_BASE + 0x04C)
    #define REG_GE_ABL_CONST Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0050_GE0 (GE0_2410_BASE + 0x050)
    #define REG_GE_SCK_HTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0054_GE0 (GE0_2410_BASE + 0x054)
    #define REG_GE_SCK_HTH_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0058_GE0 (GE0_2410_BASE + 0x058)
    #define REG_GE_SCK_LTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_005C_GE0 (GE0_2410_BASE + 0x05C)
    #define REG_GE_SCK_LTH_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0060_GE0 (GE0_2410_BASE + 0x060)
    #define REG_GE_DCK_HTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0064_GE0 (GE0_2410_BASE + 0x064)
    #define REG_GE_DCK_HTH_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0068_GE0 (GE0_2410_BASE + 0x068)
    #define REG_GE_DCK_LTH_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_006C_GE0 (GE0_2410_BASE + 0x06C)
    #define REG_GE_DCK_LTH_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0070_GE0 (GE0_2410_BASE + 0x070)
    #define REG_GE_SCK_OP_MODE Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_GE_DCK_OP_MODE Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_GE_ABL_SCK_OP_MODE Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_GE_ABL_DCK_OP_MODE Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_GE_ALPHA_CMP_MODE Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_GE_DSTAC_MODE Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_0074_GE0 (GE0_2410_BASE + 0x074)
    #define REG_GE_DSTAC_HTH Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_GE_DSTAC_LTH Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0078_GE0 (GE0_2410_BASE + 0x078)
    #define REG_GE_IRQ_TAG_MODE Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_GE_IRQ_MODE Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_GE_IRQ_MASK Fld(2, 6, AC_MSKB0)//[7:6]
    #define REG_GE_IRQ_FORCE Fld(2, 8, AC_MSKB1)//[9:8]
    #define REG_GE_IRQ_CLR Fld(2, 10, AC_MSKB1)//[11:10]
    #define REG_IRQ_FINAL_STATUS Fld(2, 12, AC_MSKB1)//[13:12]
    #define REG_IRQ_RAW_STATUS Fld(2, 14, AC_MSKB1)//[15:14]
#define REG_007C_GE0 (GE0_2410_BASE + 0x07C)
    #define REG_GE_DC_CSC_FM Fld(2, 0, AC_MSKB0)//[1:0]
    #define REG_GE_YUV_RANGE Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_GE_UV_RANGE Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_GE_SB_YUV_FM Fld(2, 4, AC_MSKB0)//[5:4]
    #define REG_GE_DB_YUV_FM Fld(2, 6, AC_MSKB0)//[7:6]
    #define REG_GE_YUV_FILTER Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_GE_DUMMY Fld(4, 9, AC_MSKB1)//[12:9]
    #define REG_GE_SB_MIU_SEL_MSB Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_GE_DB_MIU_SEL_MSB Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_GE_VCMQ_MIU_SEL_MSB Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0080_GE0 (GE0_2410_BASE + 0x080)
    #define REG_GE_SB_BASE_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0084_GE0 (GE0_2410_BASE + 0x084)
    #define REG_GE_SB_BASE_1 Fld(15, 0, AC_MSKW10)//[14:0]
    #define REG_GE_SB_MIU_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0088_GE0 (GE0_2410_BASE + 0x088)
    #define REG_GE_MIU0_PMA_LTH_MSB Fld(3, 0, AC_MSKB0)//[2:0]
    #define REG_GE_MIU0_PMA_HTH_MSB Fld(3, 4, AC_MSKB0)//[6:4]
    #define REG_GE_MIU1_PMA_LTH_MSB Fld(3, 8, AC_MSKB1)//[10:8]
    #define REG_GE_MIU1_PMA_HTH_MSB Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_008C_GE0 (GE0_2410_BASE + 0x08C)
    #define REG_GE_SB_BASE_MSB Fld(3, 0, AC_MSKB0)//[2:0]
    #define REG_GE_DB_BASE_MSB Fld(3, 4, AC_MSKB0)//[6:4]
    #define REG_GE_VCMQ_BASE_MSB Fld(3, 8, AC_MSKB1)//[10:8]
    #define REG_GE_SB_PIT_MSB Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_GE_DB_PIT_MSB Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_0090_GE0 (GE0_2410_BASE + 0x090)
    #define REG_GE_TAG_FOR_IRQ_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0094_GE0 (GE0_2410_BASE + 0x094)
    #define REG_GE_TAG_FOR_IRQ_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0098_GE0 (GE0_2410_BASE + 0x098)
    #define REG_GE_DB_BASE_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_009C_GE0 (GE0_2410_BASE + 0x09C)
    #define REG_GE_DB_BASE_1 Fld(15, 0, AC_MSKW10)//[14:0]
    #define REG_GE_DB_MIU_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_00A0_GE0 (GE0_2410_BASE + 0x0A0)
    #define REG_GE_VCMQ_BASE_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00A4_GE0 (GE0_2410_BASE + 0x0A4)
    #define REG_GE_VCMQ_BASE_1 Fld(15, 0, AC_MSKW10)//[14:0]
    #define REG_GE_VCMQ_MIU_SEL Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_00A8_GE0 (GE0_2410_BASE + 0x0A8)
    #define REG_GE_VCMQ_SIZE Fld(3, 0, AC_MSKB0)//[2:0]
    #define REG_GE_SRCBLEND Fld(4, 8, AC_MSKB1)//[11:8]
    #define REG_GE_DSTBLEND Fld(4, 12, AC_MSKB1)//[15:12]
#define REG_00AC_GE0 (GE0_2410_BASE + 0x0AC)
    #define REG_GE_COLORALPHA Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_GE_ALPHACH Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_GE_COLORIZE Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_GE_SRC_PREMUL Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_GE_SRC_PRECOL Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_GE_DST_PREMUL Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_GE_XOR Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_GE_DEMUL Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_GE_B_CONST Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00B0_GE0 (GE0_2410_BASE + 0x0B0)
    #define REG_GE_G_CONST Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_GE_R_CONST Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00B4_GE0 (GE0_2410_BASE + 0x0B4)
    #define REG_GE_P256_B Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_GE_P256_G Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00B8_GE0 (GE0_2410_BASE + 0x0B8)
    #define REG_GE_P256_R Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_GE_P256_A Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00BC_GE0 (GE0_2410_BASE + 0x0BC)
    #define REG_GE_P256_INDEX Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_GE_P256_RW Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_GE_P256_SCK_MODE Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_00C0_GE0 (GE0_2410_BASE + 0x0C0)
    #define REG_GE_SB_PIT Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00C4_GE0 (GE0_2410_BASE + 0x0C4)
    #define REG_GE_TAG_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00C8_GE0 (GE0_2410_BASE + 0x0C8)
    #define REG_GE_TAG_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00CC_GE0 (GE0_2410_BASE + 0x0CC)
    #define REG_GE_DB_PIT Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00D0_GE0 (GE0_2410_BASE + 0x0D0)
    #define REG_GE_SB_FM Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_GE_DB_FM Fld(5, 8, AC_MSKB1)//[12:8]
#define REG_00D4_GE0 (GE0_2410_BASE + 0x0D4)
    #define REG_GE_I0_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00D8_GE0 (GE0_2410_BASE + 0x0D8)
    #define REG_GE_I0_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00DC_GE0 (GE0_2410_BASE + 0x0DC)
    #define REG_GE_I1_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00E0_GE0 (GE0_2410_BASE + 0x0E0)
    #define REG_GE_I1_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00E4_GE0 (GE0_2410_BASE + 0x0E4)
    #define REG_GE_I2_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00E8_GE0 (GE0_2410_BASE + 0x0E8)
    #define REG_GE_I2_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00EC_GE0 (GE0_2410_BASE + 0x0EC)
    #define REG_GE_I3_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00F0_GE0 (GE0_2410_BASE + 0x0F0)
    #define REG_GE_I3_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00F4_GE0 (GE0_2410_BASE + 0x0F4)
    #define REG_GE_I4_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00F8_GE0 (GE0_2410_BASE + 0x0F8)
    #define REG_GE_I4_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00FC_GE0 (GE0_2410_BASE + 0x0FC)
    #define REG_GE_I5_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0100_GE0 (GE0_2410_BASE + 0x100)
    #define REG_GE_I5_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0104_GE0 (GE0_2410_BASE + 0x104)
    #define REG_GE_I6_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0108_GE0 (GE0_2410_BASE + 0x108)
    #define REG_GE_I6_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_010C_GE0 (GE0_2410_BASE + 0x10C)
    #define REG_GE_I7_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0110_GE0 (GE0_2410_BASE + 0x110)
    #define REG_GE_I7_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0114_GE0 (GE0_2410_BASE + 0x114)
    #define REG_GE_I8_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0118_GE0 (GE0_2410_BASE + 0x118)
    #define REG_GE_I8_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_011C_GE0 (GE0_2410_BASE + 0x11C)
    #define REG_GE_I9_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0120_GE0 (GE0_2410_BASE + 0x120)
    #define REG_GE_I9_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0124_GE0 (GE0_2410_BASE + 0x124)
    #define REG_GE_I10_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0128_GE0 (GE0_2410_BASE + 0x128)
    #define REG_GE_I10_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_012C_GE0 (GE0_2410_BASE + 0x12C)
    #define REG_GE_I11_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0130_GE0 (GE0_2410_BASE + 0x130)
    #define REG_GE_I11_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0134_GE0 (GE0_2410_BASE + 0x134)
    #define REG_GE_I12_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0138_GE0 (GE0_2410_BASE + 0x138)
    #define REG_GE_I12_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_013C_GE0 (GE0_2410_BASE + 0x13C)
    #define REG_GE_I13_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0140_GE0 (GE0_2410_BASE + 0x140)
    #define REG_GE_I13_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0144_GE0 (GE0_2410_BASE + 0x144)
    #define REG_GE_I14_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0148_GE0 (GE0_2410_BASE + 0x148)
    #define REG_GE_I14_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_014C_GE0 (GE0_2410_BASE + 0x14C)
    #define REG_GE_I15_C_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0150_GE0 (GE0_2410_BASE + 0x150)
    #define REG_GE_I15_C_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0154_GE0 (GE0_2410_BASE + 0x154)
    #define REG_GE_CLIP_LEFT Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_0158_GE0 (GE0_2410_BASE + 0x158)
    #define REG_GE_CLIP_RIGHT Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_015C_GE0 (GE0_2410_BASE + 0x15C)
    #define REG_GE_CLIP_TOP Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_0160_GE0 (GE0_2410_BASE + 0x160)
    #define REG_GE_CLIP_BOT Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_0164_GE0 (GE0_2410_BASE + 0x164)
    #define REG_GE_ROT Fld(2, 0, AC_MSKB0)//[1:0]
    #define REG_GE_STBB_SCK_TYPE Fld(2, 6, AC_MSKB0)//[7:6]
#define REG_0170_GE0 (GE0_2410_BASE + 0x170)
    #define REG_GE_STBB_SCK_B Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_GE_STBB_SCK_G Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0174_GE0 (GE0_2410_BASE + 0x174)
    #define REG_GE_STBB_SCK_R Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_GE_STBB_SCK_A Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0178_GE0 (GE0_2410_BASE + 0x178)
    #define REG_GE_STBB_INI_DX Fld(13, 0, AC_MSKW10)//[12:0]
    #define REG_GE_STBB_DX_MSB Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_017C_GE0 (GE0_2410_BASE + 0x17C)
    #define REG_GE_STBB_INI_DY Fld(13, 0, AC_MSKW10)//[12:0]
    #define REG_GE_STBB_DY_MSB Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0180_GE0 (GE0_2410_BASE + 0x180)
    #define REG_GE_PRIM_TYPE Fld(3, 4, AC_MSKB0)//[6:4]
    #define REG_GE_PRI_S_X_DIR Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_GE_PRI_S_Y_DIR Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_GE_PRI_X_DIR Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_GE_PRI_Y_DIR Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_GE_LINE_C_TYPE Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_GE_RECT_CH Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_GE_RECT_CV Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_GE_STBB_TYPE Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_GE_STBB_PATCH Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0184_GE0 (GE0_2410_BASE + 0x184)
    #define REG_GE_LINE_MAJOR Fld(1, 15, AC_MSKB1)//[15:15]
    #define REG_GE_LINE_DELTA Fld(14, 1, AC_MSKW10)//[14:1]
#define REG_0188_GE0 (GE0_2410_BASE + 0x188)
    #define REG_GE_LPT Fld(6, 0, AC_MSKB0)//[5:0]
    #define REG_GE_LPT_RST Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_GE_LINE_LAST Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_GE_LPT_RPF Fld(2, 6, AC_MSKB0)//[7:6]
#define REG_018C_GE0 (GE0_2410_BASE + 0x18C)
    #define REG_GE_LINE_LENGTH Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_0190_GE0 (GE0_2410_BASE + 0x190)
    #define REG_GE_STBB_DX Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0194_GE0 (GE0_2410_BASE + 0x194)
    #define REG_GE_STBB_DY Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0198_GE0 (GE0_2410_BASE + 0x198)
    #define REG_GE_ITC_LINE Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_GE_ITC_DIS Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_019C_GE0 (GE0_2410_BASE + 0x19C)
    #define REG_GE_ITC_DELTA Fld(5, 0, AC_MSKB0)//[4:0]
#define REG_01A0_GE0 (GE0_2410_BASE + 0x1A0)
    #define REG_GE_PRI_V0_X Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_01A4_GE0 (GE0_2410_BASE + 0x1A4)
    #define REG_GE_PRI_V0_Y Fld(14, 0, AC_MSKW10)//[13:0]

//Page GE0_2
#define REG_01A8_GE0 (GE0_2410_BASE + 0x1A8)
    #define REG_GE_PRI_V1_X Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_01AC_GE0 (GE0_2410_BASE + 0x1AC)
    #define REG_GE_PRI_V1_Y Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_01B0_GE0 (GE0_2410_BASE + 0x1B0)
    #define REG_GE_PRI_V2_X Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_01B4_GE0 (GE0_2410_BASE + 0x1B4)
    #define REG_GE_PRI_V2_Y Fld(14, 0, AC_MSKW10)//[13:0]
#define REG_01B8_GE0 (GE0_2410_BASE + 0x1B8)
    #define REG_GE_STBB_S_W Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_01BC_GE0 (GE0_2410_BASE + 0x1BC)
    #define REG_GE_STBB_S_H Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_01C0_GE0 (GE0_2410_BASE + 0x1C0)
    #define REG_GE_PRI_G_ST Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_GE_PRI_B_ST Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_01C4_GE0 (GE0_2410_BASE + 0x1C4)
    #define REG_GE_PRI_A_ST Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_GE_PRI_R_ST Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_01C8_GE0 (GE0_2410_BASE + 0x1C8)
    #define REG_GE_PRI_R_DX_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01CC_GE0 (GE0_2410_BASE + 0x1CC)
    #define REG_GE_PRI_R_DX_1 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_01D0_GE0 (GE0_2410_BASE + 0x1D0)
    #define REG_GE_PRI_R_DY_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01D4_GE0 (GE0_2410_BASE + 0x1D4)
    #define REG_GE_PRI_R_DY_1 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_01D8_GE0 (GE0_2410_BASE + 0x1D8)
    #define REG_GE_PRI_G_DX_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01DC_GE0 (GE0_2410_BASE + 0x1DC)
    #define REG_GE_PRI_G_DX_1 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_01E0_GE0 (GE0_2410_BASE + 0x1E0)
    #define REG_GE_PRI_G_DY_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01E4_GE0 (GE0_2410_BASE + 0x1E4)
    #define REG_GE_PRI_G_DY_1 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_01E8_GE0 (GE0_2410_BASE + 0x1E8)
    #define REG_GE_PRI_B_DX_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01EC_GE0 (GE0_2410_BASE + 0x1EC)
    #define REG_GE_PRI_B_DX_1 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_01F0_GE0 (GE0_2410_BASE + 0x1F0)
    #define REG_GE_PRI_B_DY_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01F4_GE0 (GE0_2410_BASE + 0x1F4)
    #define REG_GE_PRI_B_DY_1 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_01F8_GE0 (GE0_2410_BASE + 0x1F8)
    #define REG_GE_PRI_A_DX Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_01FC_GE0 (GE0_2410_BASE + 0x1FC)
    #define REG_GE_PRI_A_DY Fld(16, 0, AC_FULLW10)//[15:0]
