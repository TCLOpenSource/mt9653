/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//Page SMART
#define REG_0000_SMART (0x000)
    #define REG_THR_RBR_DLL_0000 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0004_SMART (0x004)
    #define REG_IER_DLH_0004 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0008_SMART (0x008)
    #define REG_FCR_IIR_0008 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_000C_SMART (0x00C)
    #define REG_LCR_CHAR_BITS_000C Fld(2, 0, AC_MSKB0)//[1:0]
    #define REG_LCR_STOP_BITS_000C Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_LCR_PARITY_EN_000C Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_LCR_EVEN_PARITY_SEL_000C Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_LCR_STICK_PARITY_SEL_000C Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_LCR_BREAK_CTRL_000C Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_LCR_DL_ACCESS_000C Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_0010_SMART (0x010)
    #define REG_MCR_DTR_0010 Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_MCR_RTS_0010 Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_MCR_OUT1_0010 Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_MCR_OUT2_0010 Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_MCR_LOOPBACK_0010 Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_MCR_AFCE_0010 Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_0014_SMART (0x014)
    #define REG_LSR_DR_0014 Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_LSR_OE_0014 Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_LSR_PE_0014 Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_LSR_FE_0014 Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_LSR_BI_0014 Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_LSR_TXFIFO_EMPTY_0014 Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_LSR_TX_EMPTY_0014 Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_LSR_ERROR_0014 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_0018_SMART (0x018)
    #define REG_MSR_DCTS_0018 Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_MSR_DDSR_0018 Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_MSR_TERI_0018 Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_MSR_DDCD_0018 Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_MSR_CTS_COMP_0018 Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_MSR_DSR_COMP_0018 Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_MSR_RI_COMP_0018 Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_MSR_DCD_COMP_0018 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_001C_SMART (0x01C)
    #define REG_SRR_001C Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0020_SMART (0x020)
    #define REG_DEBUG0_0020 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0024_SMART (0x024)
    #define REG_DEBUG1_0024 Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_0028_SMART (0x028)
    #define REG_RF_COUNT_0028 Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_CONAX_RST_TO_IO_EDGE_DETECT_FAIL_0028 Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_CONAX_RST_TO_IO_EDGE_DETECT_EN_0028 Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_002C_SMART (0x02C)
    #define REG_RF_BOTTOM_002C Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_IO_OFF_002C Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_SMART_CLK_OFF_002C Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_RST_OFF_002C Fld(1, 7, AC_MSKB0)//[7:7]
#define REG_0030_SMART (0x030)
    #define REG_EXPONENTIAL_0030 Fld(5, 0, AC_MSKB0)//[4:0]
#define REG_0034_SMART (0x034)
    #define REG_DEBUG2_0034 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0038_SMART (0x038)
    #define REG_GUARDTIME_0038 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_003C_SMART (0x03C)
    #define REG_DEBOUNCE_TIME_003C Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0040_SMART (0x040)
    #define REG_CWT_0_0040 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0044_SMART (0x044)
    #define REG_CWT_1_0044 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0048_SMART (0x048)
    #define REG_BGT_0048 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0050_SMART (0x050)
    #define REG_BWT_0_0050 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0054_SMART (0x054)
    #define REG_BWT_1_0054 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0058_SMART (0x058)
    #define REG_BWT_2_0058 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_005C_SMART (0x05C)
    #define REG_BWT_3_005C Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0060_SMART (0x060)
    #define REG_CTRL2_0060 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0064_SMART (0x064)
    #define REG_DEBUG3_0064 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0068_SMART (0x068)
    #define REG_TF_COUNT_0068 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_006C_SMART (0x06C)
    #define REG_DEACTIVE_RST_006C Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0070_SMART (0x070)
    #define REG_DEACTIVE_CLK_0070 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0074_SMART (0x074)
    #define REG_DEACTIVE_IO_0074 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0078_SMART (0x078)
    #define REG_DEACTIVE_VCC_0078 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_007C_SMART (0x07C)
    #define REG_CTRL3_007C Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0080_SMART (0x080)
    #define REG_ACTIVE_RST_0080 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0084_SMART (0x084)
    #define REG_ACTIVE_CLK_0084 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0088_SMART (0x088)
    #define REG_ACTIVE_IO_0088 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_008C_SMART (0x08C)
    #define REG_ACTIVE_VCC_008C Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0090_SMART (0x090)
    #define REG_CTRL4_0090 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0094_SMART (0x094)
    #define REG_CTRL5_0094 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0098_SMART (0x098)
    #define REG_DEBUG4_0098 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_009C_SMART (0x09C)
    #define REG_CTRL6_009C Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00A0_SMART (0x0A0)
    #define REG_SHORT_CIRCUIT_DEBOUNCE_TIME_LOW_00A0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00A4_SMART (0x0A4)
    #define REG_SHORT_CIRCUIT_DEBOUNCE_TIME_HIGH_00A4 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00A8_SMART (0x0A8)
    #define REG_TEST_SMC_ATOP_LOW_00A8 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00AC_SMART (0x0AC)
    #define REG_TEST_SMC_ATOP_HIGH_00AC Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00B0_SMART (0x0B0)
    #define REG_CTRL7_00B0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00B4_SMART (0x0B4)
    #define REG_ANALOG_STATUS_00B4 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00B8_SMART (0x0B8)
    #define REG_CTRL8_00B8 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00BC_SMART (0x0BC)
    #define REG_CTRL9_00BC Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00C0_SMART (0x0C0)
    #define REG_CTRL10_00C0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00C4_SMART (0x0C4)
    #define REG_CTRL11_00C4 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00C8_SMART (0x0C8)
    #define REG_CTRL12_00C8 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00CC_SMART (0x0CC)
    #define REG_CTRL13_00CC Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00D0_SMART (0x0D0)
    #define REG_CTRL14_00D0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00D4_SMART (0x0D4)
    #define REG_ANALOG_STATUS2_00D4 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00D8_SMART (0x0D8)
    #define REG_LSR_CLR_FLAG_00D8 Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_LSR_CLR_FLAG_OPT_00D8 Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_00DC_SMART (0x0DC)
    #define REG_CONAX_RST_TO_IO_0_00DC Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00E0_SMART (0x0E0)
    #define REG_CONAX_RST_TO_IO_1_00E0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00E4_SMART (0x0E4)
    #define REG_CONAX_RST_TO_IO_2_00E4 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00E8_SMART (0x0E8)
    #define REG_CONAX_RST_TO_IO_3_00E8 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00EC_SMART (0x0EC)
    #define REG_CTRL15_00EC Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00F0_SMART (0x0F0)
    #define REG_SMC_ANALOG_CONFIG_00F0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00F4_SMART (0x0F4)
    #define REG_ANALOG_STATUS3_00F4 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00F8_SMART (0x0F8)
    #define REG_CTRL17_00F8 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00FC_SMART (0x0FC)
    #define REG_CTRL18_00FC Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0100_SMART (0x100)
    #define REG_TF_COUNT_CMPR_0100 Fld(9, 0, AC_MSKW10)//[8:0]
#define REG_0104_SMART (0x104)
    #define REG_RF_COUNT_CMPR_0104 Fld(9, 0, AC_MSKW10)//[8:0]
#define REG_0108_SMART (0x108)
    #define REG_TF_COUNT_RD_0108 Fld(9, 0, AC_MSKW10)//[8:0]
#define REG_010C_SMART (0x10C)
    #define REG_RF_COUNT_RD_010C Fld(9, 0, AC_MSKW10)//[8:0]
#define REG_01B0_SMART (0x1B0)
    #define REG_CKG_SMART_CA_01B0 Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_01B4_SMART (0x1B4)
    #define REG_CKG_SMART_CA_EXT_01B4 Fld(2, 0, AC_MSKB0)//[1:0]
    #define REG_CKG_SMART_CA_M_01B4 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_01B8_SMART (0x1B8)
    #define REG_CKG_SMART_CA_N_01B8 Fld(13, 0, AC_MSKW10)//[12:0]
#define REG_01BC_SMART (0x1BC)
    #define REG_SMC_3V5V_VSEL_01BC Fld(1, 0, AC_MSKB0)//[0:0]

