/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#define PM_CEC_BK_E402 0x00080400

// Page PM_CEC
#define REG_0000_PM_CEC (0x000)
    #define REG_CEC_TX_LEN Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_CEC_TX_SW_RST Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_HOT_PLUG_TX_SW_RST Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_RETRY_CNT Fld(3, 8, AC_MSKB1)//[10:8]
    #define REG_TX_LOW_BIT_SEL Fld(2, 11, AC_MSKB1)//[12:11]
    #define REG_CEC_SAMPLE_SEL Fld(3, 13, AC_MSKB1)//[15:13]
#define REG_0004_PM_CEC (0x004)
    #define REG_TX_RISING_SHIFT_SEL Fld(2, 0, AC_MSKB0)//[1:0]
    #define REG_TX_FALLING_SHIFT_SEL Fld(2, 2, AC_MSKB0)//[3:2]
    #define REG_CANCEL_TX_REQ Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_CEC_CLK_GATE Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_CEC_TEST_MD Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_CEC_CTRL_EN Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_CEC_FREE_CNT1 Fld(4, 8, AC_MSKB1)//[11:8]
    #define REG_CEC_FREE_CNT2 Fld(4, 12, AC_MSKB1)//[15:12]
#define REG_0008_PM_CEC (0x008)
    #define REG_CEC_FREE_CNT3 Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_CEC_LOGICAL_ADDR Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_CNT_10US_VALUE Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_000C_PM_CEC (0x00C)
    #define REG_F_CNT_10US_VALUE Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_RX_BOUND_SHIFT Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_DIS_EH Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_EN_LOST_ABT_RX Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_CEC_OVERRIDE_FUN Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_LOST_ABT_SEL Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_CEC_CTRL_SEL Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_NSUP_CMD_ACT Fld(2, 13, AC_MSKB1)//[14:13]
    #define REG_IGNORE_UNEXP_OP_LEN Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0010_PM_CEC (0x010)
    #define REG_CEC_RX_LEN Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_CEC_LINE Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_CEC_TX_REQ_STS Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_HW_CEC_STS Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_RESERVED0_IN Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0014_PM_CEC (0x014)
    #define REG_CEC_PHY_STATE Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0018_PM_CEC (0x018)
    #define REG_CEC_PD_STATE Fld(15, 0, AC_MSKW10)//[14:0]
#define REG_001C_PM_CEC (0x01C)
    #define REG_OP_EN Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_OP2_OPERAND1_EN Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_OP3_OPERAND1_EN Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_WAKEUP_INT_MASK Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_OPERAND_EN Fld(4, 8, AC_MSKB1)//[11:8]
    #define REG_EXP_OP4_LEN Fld(4, 12, AC_MSKB1)//[15:12]
#define REG_0020_PM_CEC (0x020)
    #define REG_OPCODE0 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_OPCODE1 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0024_PM_CEC (0x024)
    #define REG_OPCODE2 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_OPCODE3 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0028_PM_CEC (0x028)
    #define REG_OPCODE4 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_OP0_OPERAND Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_002C_PM_CEC (0x02C)
    #define REG_OP1_OPERAND Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_OP2_OPERAND0 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0030_PM_CEC (0x030)
    #define REG_OP2_OPERAND1 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_OP3_OPERAND0 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0034_PM_CEC (0x034)
    #define REG_OP3_OPERAND1 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_CEC_VERSION Fld(3, 8, AC_MSKB1)//[10:8]
    #define REG_OP_ACC_TYPE Fld(5, 11, AC_MSKB1)//[15:11]
#define REG_0038_PM_CEC (0x038)
    #define REG_PHYSICAL_ADDR Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_003C_PM_CEC (0x03C)
    #define REG_VENDOR_ID_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0040_PM_CEC (0x040)
    #define REG_VENDOR_ID_1 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_ABORT_REASON Fld(3, 8, AC_MSKB1)//[10:8]
    #define REG_IGNOR_ADDR_SW Fld(4, 11, AC_MSKB1)//[14:11]
    #define REG_IGNOR_ENB_SW Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0044_PM_CEC (0x044)
    #define REG_HW_RX_EVENT_STS0 Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_HW_RX_EVENT_STS1 Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_HW_RX_EVENT_STS2 Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_HW_RX_EVENT_STS3 Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_RX_BIT_TOO_SHORT Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_RX_BIT_TOO_LONG Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_WAKEUP_INT Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_IGNOR_OCCURS Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_CEC_EVENT_INT Fld(7, 8, AC_MSKB1)//[14:8]
#define REG_0048_PM_CEC (0x048)
    #define REG_CEC_EVENT_INT_FORCE Fld(7, 0, AC_MSKB0)//[6:0]
    #define REG_CEC_EVENT_INT0_CLEAR Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_CEC_EVENT_INT1_CLEAR Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_CEC_EVENT_INT2_CLEAR Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_CEC_EVENT_INT3_CLEAR Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_CEC_EVENT_INT4_CLEAR Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_CEC_EVENT_INT5_CLEAR Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_CEC_EVENT_INT6_CLEAR Fld(1, 14, AC_MSKB1)//[14:14]
#define REG_004C_PM_CEC (0x04C)
    #define REG_CEC_EVENT_INT_MASK Fld(7, 0, AC_MSKB0)//[6:0]
#define REG_0050_PM_CEC (0x050)
    #define REG_PD_NORMAL_SEL Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_CEC_SRC_CK_SEL Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_CEC_BUS_TST_MD Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_TST_BUS_SEL Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_BYTE_CNT_LABT Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_DEV_TYPE Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0060_PM_CEC (0x060)
    #define REG_TX_DATA0 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_DATA1 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0064_PM_CEC (0x064)
    #define REG_TX_DATA2 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_DATA3 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0068_PM_CEC (0x068)
    #define REG_TX_DATA4 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_DATA5 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_006C_PM_CEC (0x06C)
    #define REG_TX_DATA6 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_DATA7 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0070_PM_CEC (0x070)
    #define REG_TX_DATA8 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_DATA9 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0074_PM_CEC (0x074)
    #define REG_TX_DATA10 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_DATA11 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0078_PM_CEC (0x078)
    #define REG_TX_DATA12 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_DATA13 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_007C_PM_CEC (0x07C)
    #define REG_TX_DATA14 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_DATA15 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0080_PM_CEC (0x080)
    #define REG_RX_DATA0 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DATA1 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0084_PM_CEC (0x084)
    #define REG_RX_DATA2 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DATA3 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0088_PM_CEC (0x088)
    #define REG_RX_DATA4 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DATA5 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_008C_PM_CEC (0x08C)
    #define REG_RX_DATA6 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DATA7 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0090_PM_CEC (0x090)
    #define REG_RX_DATA8 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DATA9 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0094_PM_CEC (0x094)
    #define REG_RX_DATA10 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DATA11 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_0098_PM_CEC (0x098)
    #define REG_RX_DATA12 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DATA13 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_009C_PM_CEC (0x09C)
    #define REG_RX_DATA14 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DATA15 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00A0_PM_CEC (0x0A0)
    #define REG_RX1_DATA0 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX1_DATA1 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00A4_PM_CEC (0x0A4)
    #define REG_RX1_DATA2 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX1_DATA3 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00A8_PM_CEC (0x0A8)
    #define REG_RX1_DATA4 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX1_DATA5 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00AC_PM_CEC (0x0AC)
    #define REG_RX1_DATA6 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX1_DATA7 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00B0_PM_CEC (0x0B0)
    #define REG_RX1_DATA8 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX1_DATA9 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00B4_PM_CEC (0x0B4)
    #define REG_RX1_DATA10 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX1_DATA11 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00B8_PM_CEC (0x0B8)
    #define REG_RX1_DATA12 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX1_DATA13 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00BC_PM_CEC (0x0BC)
    #define REG_RX1_DATA14 Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX1_DATA15 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00C0_PM_CEC (0x0C0)
    #define REG_EN_CEC_MULTI_FUN Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_TX_REQ_SEL Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_TX_REQ Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_INT_CLR_TYPE_SEL Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_CEC_INT_PD_MASK Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_CEC_INT_NORMAL_MASK Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_CEC_INT_NORMAL_R2_MASK Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_CEC1_LOGICAL_ADDR Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_00C4_PM_CEC (0x0C4)
    #define REG_CEC1_RX_LEN Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_DEV_TYPE_1 Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00C8_PM_CEC (0x0C8)
    #define REG_PHYSICAL_ADDR_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00CC_PM_CEC (0x0CC)
    #define REG_HOT_PLUG_RAW Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_HOT_PLUG_DB Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_HOT_PLUG_PAD_I Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_HOT_PLUG_PAD_OEN Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_HOT_PLUG_IRQ_NORMAL_R2_MASK Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_HOT_PLUG_IRQ Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_HOT_PLUG_IRQ_MASK Fld(2, 8, AC_MSKB1)//[9:8]
    #define REG_HOT_PLUG_IRQ_TYPE_SEL Fld(2, 10, AC_MSKB1)//[11:10]
    #define REG_HOT_PLUG_WAKEUP_IRQ Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_HOT_PLUG_WAKEUP_MASK Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_HOT_PLUG_WAKEUP_TYPE_SEL Fld(2, 14, AC_MSKB1)//[15:14]
#define REG_00D0_PM_CEC (0x0D0)
    #define REG_CEC_PD_CHKSUM Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_00D4_PM_CEC (0x0D4)
    #define REG_ACT_SRC_RX0_FLAG Fld(1, 15, AC_MSKB1)//[15:15]
    #define REG_ACT_SRC_RX1_FLAG Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_ACT_SRC_EN Fld(1, 13, AC_MSKB1)//[13:13]
#define REG_00D8_PM_CEC (0x0D8)
    #define REG_ACT_SRC_RX0_HEADER Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00DC_PM_CEC (0x0DC)
    #define REG_ACT_SRC_RX0_PHY0 Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_ACT_SRC_RX0_PHY1 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_00E0_PM_CEC (0x0E0)
    #define REG_ACT_SRC_RX1_HEADER Fld(8, 8, AC_FULLB1)//[15:8]
#define REG_00E4_PM_CEC (0x0E4)
    #define REG_ACT_SRC_RX1_PHY0 Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_ACT_SRC_RX1_PHY1 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0100_PM_CEC (0x100)
    #define REG_SERI_OPERAND_CHK_EN Fld(1, 15, AC_MSKB1)//[15:15]
#define REG_0104_PM_CEC (0x104)
    #define REG_OP4_OPERAND1 Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_OP4_OPERAND0 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_0108_PM_CEC (0x108)
    #define REG_OP4_OPERAND3 Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_OP4_OPERAND2 Fld(8, 0, AC_FULLB0)//[7:0]
#define REG_010C_PM_CEC (0x10C)
    #define REG_PHYSICAL_ADR_CHK_CNT Fld(3, 0, AC_MSKB0)//[2:0]
    #define REG_EN_PHYSICAL_ADR_CHK Fld(1, 3, AC_MSKB0)//[3:3]
