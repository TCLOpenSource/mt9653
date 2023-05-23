/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/* Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#ifndef _MTK_GMAC_MT5896_CODA_H_
#define _MTK_GMAC_MT5896_CODA_H_

//Page X32_GMAC0
#define GMAC0_REG_0000_X32_GMAC0 (0x0000)
    #define REG_ETH_CTL_LB Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_ETH_CTL_LBL Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_ETH_CTL_RE Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_ETH_CTL_TE Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_ETH_CTL_MPE Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_ETH_CTL_CSR Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_ETH_CTL_ISR Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_ETH_CTL_WES Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_ETH_CTL_BP Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_ETH_CTL_APB_GATING Fld(1, 31, AC_MSKB3)//[31:31]
#define GMAC0_REG_0008_X32_GMAC0 (0x0008)
    #define REG_ETH_CFG_SPEED Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_ETH_CFG_FD Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_ETH_CFG_BR Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_ETH_CFG_CAF Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_ETH_CFG_NBC Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_ETH_CFG_MTI Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_ETH_CFG_UNI Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_ETH_CFG_RLF Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_ETH_CFG_EAE Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_ETH_CFG_CLK Fld(2, 10, AC_MSKB1)//[11:10]
    #define REG_ETH_CFG_RT Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_ETH_CFG_CAMNEG Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_ETH_CFG_ID_EN Fld(1, 19, AC_MSKB2)//[19:19]
    #define REG_ETH_CFG_VLAN_ID Fld(12, 20, AC_MSKW32)//[31:20]
#define GMAC0_REG_0010_X32_GMAC0 (0x0010)
    #define REG_ETH_SR_LINK_PCLK_SYNC Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_ETH_SR_MDIO Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_ETH_SR_IDLE Fld(1, 2, AC_MSKB0)//[2:2]
#define GMAC0_REG_0018_X32_GMAC0 (0x0018)
    #define REG_ETH_TAR_ADDR_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_TAR_ADDR_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0020_X32_GMAC0 (0x0020)
    #define REG_ETH_TCR_LEN Fld(14, 0, AC_MSKW10)//[13:0]
    #define REG_ETH_TCR_NCRC Fld(1, 15, AC_MSKB1)//[15:15]
#define GMAC0_REG_0028_X32_GMAC0 (0x0028)
    #define REG_ETH_TSR_OVER Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_ETH_TST_COL Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_ETH_TSR_RLE Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_TSR_IDLE Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_TSR_BNQ Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_TSR_COMP Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_TSR_UND Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_TSR_TBNQ Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_TSR_FBNQ Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_TSR_FIFO_IDLE Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_TSR_FIFO_2ND_BNQ Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_TSR_FIFO_3RD_BNQ Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_TSR_FIFO_4TH_BNQ Fld(1, 12, AC_MSKB1)//[12:12]
#define GMAC0_REG_0030_X32_GMAC0 (0x0030)
    #define REG_ETH_RBQP_ADDR_L Fld(14, 2, AC_MSKW10)//[15:2]
    #define REG_ETH_RBQP_ADDR_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0038_X32_GMAC0 (0x0038)
    #define REG_ETH_BUFF_CONF_RX_BUFF_SIZE Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_ETH_BUFF_CONF_RX_BUFF_BASE_L Fld(5, 11, AC_MSKB1)//[15:11]
    #define REG_ETH_BUFF_CONF_RX_BUFF_BASE_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0040_X32_GMAC0 (0x0040)
    #define REG_ETH_RSR_DNA Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_ETH_RSR_REC Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_ETH_RSR_OVR_RSR Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_ETH_RSR_BNA Fld(1, 3, AC_MSKB0)//[3:3]
#define GMAC0_REG_0048_X32_GMAC0 (0x0048)
    #define REG_ETH_ISR_DONE Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_ETH_ISR_RCOM Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_ETH_ISR_RBNA Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_ETH_ISR_TOVR Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_ETH_ISR_TUND Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_ETH_ISR_RTRY Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_ETH_ISR_TBRE Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_ETH_ISR_TCOM Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_ETH_ISR_TIDLE Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_ISR_LINK Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_ETH_ISR_ROVR Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_ETH_ISR_EEE_RECEIVED Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_ETH_ISR_TX_BOTH_IDLE Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_ETH_ISR_TX_FRAMES_SINCE_LAST_INT Fld(3, 13, AC_MSKB1)//[15:13]
    #define REG_ETH_ISR_TX_FRAMES_CAN_BE_QUEUED Fld(4, 16, AC_MSKB2)//[19:16]
#define GMAC0_REG_0050_X32_GMAC0 (0x0050)
    #define REG_IER_DONE_IER Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_IER_RCOM_IER Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_IER_RBNA_IER Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_IER_TOVR_IER Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_IER_TUND_IER Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_IER_RTRY_IER Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_IER_TBRE_IER Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_IER_TCOM_IER Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_IER_TIDLE_IER Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_IER_LINK_IER Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_IER_ROVR_IER Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_IER_HRESP_IER Fld(1, 11, AC_MSKB1)//[11:11]
#define GMAC0_REG_0058_X32_GMAC0 (0x0058)
    #define REG_IDR_DONE_IDR Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_IDR_RCOM_IDR Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_IDR_RBNA_IDR Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_IDR_TOVR_IDR Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_IDR_TUND_IDR Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_IDR_RTRY_IDR Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_IDR_TBRE_IDR Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_IDR_TCOM_IDR Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_IDR_TIDLE_IDR Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_IDR_LINK_IDR Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_IDR_ROVR_IDR Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_IDR_HRESP_IDR Fld(1, 11, AC_MSKB1)//[11:11]
#define GMAC0_REG_0060_X32_GMAC0 (0x0060)
    #define REG_ETH_IMR_DONE_IMR Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_ETH_IMR_RCOM_IMR Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_ETH_IMR_RBNA_IMR Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_ETH_IMR_TOVR_IMR Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_ETH_IMR_TUND_IMR Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_ETH_IMR_RTRY_IMR Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_ETH_IMR_TBRE_IMR Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_ETH_IMR_TCOM_IMR Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_ETH_IMR_TIDLE_IMR Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_ETH_IMR_LINK_IMR Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_ETH_IMR_ROVR_IMR Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_ETH_IMR_HRESP_IMR Fld(1, 11, AC_MSKB1)//[11:11]
#define GMAC0_REG_0068_X32_GMAC0 (0x0068)
    #define REG_ETH_MAN_DATA Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_MAN_CODE Fld(2, 16, AC_MSKB2)//[17:16]
    #define REG_ETH_MAN_REGA Fld(5, 18, AC_MSKB2)//[22:18]
    #define REG_ETH_MAN_PHYA Fld(5, 23, AC_MSKW32)//[27:23]
    #define REG_ETH_MAN_RQ Fld(2, 28, AC_MSKB3)//[29:28]
    #define REG_ETH_MAN_HIGH Fld(1, 30, AC_MSKB3)//[30:30]
    #define REG_ETH_MAN_LOW Fld(1, 31, AC_MSKB3)//[31:31]
#define GMAC0_REG_0070_X32_GMAC0 (0x0070)
    #define REG_ETH_BUFFRDPTR_RX_BUFF_RD_PTR0 Fld(14, 2, AC_MSKW10)//[15:2]
    #define REG_ETH_BUFFRDPTR_RX_BUFF_RD_PTR1 Fld(4, 16, AC_MSKB2)//[19:16]
#define GMAC0_REG_0078_X32_GMAC0 (0x0078)
    #define REG_ETH_BUFFWRPTR_RX_BUFF_WR_PTR0 Fld(14, 2, AC_MSKW10)//[15:2]
    #define REG_ETH_BUFFWRPTR_RX_BUFF_WR_PTR1 Fld(4, 16, AC_MSKB2)//[19:16]
#define GMAC0_REG_0080_X32_GMAC0 (0x0080)
    #define REG_ETH_FRA_FRAMES_TXED_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_FRA_FRAMES_TXED_H Fld(8, 16, AC_FULLB2)//[23:16]
#define GMAC0_REG_0088_X32_GMAC0 (0x0088)
    #define REG_ETH_SCOL_SINGLE_COLLISIONS Fld(16, 0, AC_FULLW10)//[15:0]
#define GMAC0_REG_0090_X32_GMAC0 (0x0090)
    #define REG_ETH_MCOL_MULTIPLE_COLLISIONS Fld(16, 0, AC_FULLW10)//[15:0]
#define GMAC0_REG_0098_X32_GMAC0 (0x0098)
    #define REG_ETH_OK_FRAMES_RXED_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_OK_FRAMES_RXED_H Fld(8, 16, AC_FULLB2)//[23:16]
#define GMAC0_REG_00A0_X32_GMAC0 (0x00A0)
    #define REG_ETH_SEQE_FCS_ERRORS Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00A8_X32_GMAC0 (0x00A8)
    #define REG_ETH_ALE Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00B0_X32_GMAC0 (0x00B0)
    #define REG_ETH_DTE Fld(16, 0, AC_FULLW10)//[15:0]
#define GMAC0_REG_00B8_X32_GMAC0 (0x00B8)
    #define REG_ETH_LCOL Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00C0_X32_GMAC0 (0x00C0)
    #define REG_ETH_ECOL Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00C8_X32_GMAC0 (0x00C8)
    #define REG_ETH_TUE Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00D0_X32_GMAC0 (0x00D0)
    #define REG_ETH_CSE Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00D8_X32_GMAC0 (0x00D8)
    #define REG_ETH_RE Fld(16, 0, AC_FULLW10)//[15:0]
#define GMAC0_REG_00E0_X32_GMAC0 (0x00E0)
    #define REG_ETH_ROVR Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00E8_X32_GMAC0 (0x00E8)
    #define REG_ETH_SE Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00F0_X32_GMAC0 (0x00F0)
    #define REG_ETH_ELR Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_00F8_X32_GMAC0 (0x00F8)
    #define REG_ETH_RJB Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_0100_X32_GMAC0 (0x0100)
    #define REG_ETH_USF Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_0108_X32_GMAC0 (0x0108)
    #define REG_ETH_SQEE Fld(8, 0, AC_FULLB0)//[7:0]
#define GMAC0_REG_0120_X32_GMAC0 (0x0120)
    #define REG_ETH_HSL_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_HSL_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0128_X32_GMAC0 (0x0128)
    #define REG_ETH_HSH_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_HSH_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0130_X32_GMAC0 (0x0130)
    #define REG_ETH_SA1L_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_SA1L_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0138_X32_GMAC0 (0x0138)
    #define REG_ETH_SA1H Fld(16, 0, AC_FULLW10)//[15:0]
#define GMAC0_REG_0140_X32_GMAC0 (0x0140)
    #define REG_ETH_SA2L_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_SA2L_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0148_X32_GMAC0 (0x0148)
    #define REG_ETH_SA2H Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_MULTICAST_MASK Fld(2, 30, AC_MSKB3)//[31:30]
#define GMAC0_REG_0150_X32_GMAC0 (0x0150)
    #define REG_ETH_SA3L_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_SA3L_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0158_X32_GMAC0 (0x0158)
    #define REG_ETH_SA3H Fld(16, 0, AC_FULLW10)//[15:0]
#define GMAC0_REG_0160_X32_GMAC0 (0x0160)
    #define REG_ETH_SA4L_L Fld(16, 0, AC_FULLW10)//[15:0]
    #define REG_ETH_SA4L_H Fld(16, 16, AC_FULLW32)//[31:16]
#define GMAC0_REG_0168_X32_GMAC0 (0x0168)
    #define REG_ETH_SA4H Fld(16, 0, AC_FULLW10)//[15:0]
#define GMAC0_REG_0170_X32_GMAC0 (0x0170)
    #define REG_TAG_TYPE Fld(16, 0, AC_FULLW10)//[15:0]
#define GMAC0_REG_0180_X32_GMAC0 (0x0180)
    #define REG_RIU_C0 Fld(2, 0, AC_MSKB0)//[1:0]
#define GMAC0_REG_01A0_X32_GMAC0 (0x01A0)
    #define REG_TXQUEUE_INT_LV_H Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TXQUEUE_INT_LV_L Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_PREAMBLE_CNT_4B Fld(6, 16, AC_MSKB2)//[21:16]
    #define REG_PREAMBLE_CNT_8B Fld(6, 22, AC_MSKW32)//[27:22]
#define GMAC0_REG_01B0_X32_GMAC0 (0x01B0)
    #define REG_TX_FRAME_IN_PROCESS Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_FRAMES_IN_PROCESS_D Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_NEW_TX_QUEUE_CNT Fld(7, 8, AC_MSKB1)//[14:8]
    #define REG_PACKET_COUNT_ALL Fld(15, 16, AC_MSKW32)//[30:16]
#define GMAC0_REG_01B8_X32_GMAC0 (0x01B8)
    #define REG_TX_DMA_DONE_COUNT Fld(12, 0, AC_MSKW10)//[11:0]
    #define REG_RX_DMA_STATE_ERROR Fld(1, 27, AC_MSKB3)//[27:27]
    #define REG_RX_DMA_VALID_FAIL Fld(1, 28, AC_MSKB3)//[28:28]
    #define REG_RX_DMA_FLUSH_PROT_EN Fld(1, 29, AC_MSKB3)//[29:29]
    #define REG_RX_DMA_STATE_CHK_EN Fld(1, 30, AC_MSKB3)//[30:30]
    #define REG_RX_DMA_VALID_CHK_EN Fld(1, 31, AC_MSKB3)//[31:31]
#define GMAC0_REG_01C0_X32_GMAC0 (0x01C0)
    #define REG_TSO_1ST_FLAGS Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TSO_MID_FLAGS Fld(8, 8, AC_FULLB1)//[15:8]
    #define REG_TSO_LAST_FLAGS Fld(8, 16, AC_FULLB2)//[23:16]
#define GMAC0_REG_01C8_X32_GMAC0 (0x01C8)
    #define REG_TX_DESCRIPTOR_PTR_HI Fld(14, 0, AC_MSKW10)//[13:0]
    #define REG_TX_DESC_PTR_SW_HI Fld(14, 16, AC_MSKW32)//[29:16]
#define GMAC0_REG_01D0_X32_GMAC0 (0x01D0)
    #define REG_TX_DESCRIPTOR_PTR_LO Fld(14, 0, AC_MSKW10)//[13:0]
    #define REG_TX_DESC_PTR_SW_LO Fld(14, 16, AC_MSKW32)//[29:16]
#define GMAC0_REG_01D8_X32_GMAC0 (0x01D8)
    #define REG_RX_BUFFER_Q_PTR_COUNT_SW Fld(11, 0, AC_MSKW10)//[10:0]
    #define REG_RX_DESC_PTR_SW Fld(11, 16, AC_MSKW32)//[26:16]

//Page X32_OGMAC1
#define OGMAC1_REG_0000_X32_OGMAC1 (0x0000)
    #define REG_PDN_EMAC Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_RMII Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_RMII_12 Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_TX_INV Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_PIPE_LINE_DELAY Fld(2, 4, AC_MSKB0)//[5:4]
    #define REG_MIU_WP_EN Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_MIU_WP_INT_EN Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_SOFT_RSTZ2MIU Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_SOFT_RSTZ2MIU_RDY Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_SOFT_RSTZ2APB Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_SOFT_RSTZ2AHB Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_0004_X32_OGMAC1 (0x0004)
    #define REG_INT_MASK Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_INT_FORCE Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_AUTO_MASK_INT_EN Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_MIU_PRIORITY_CTL Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_MIU_WP_INT_STATUS Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_MIU0_SIZE Fld(2, 6, AC_MSKB0)//[7:6]
    #define REG_BIST_FAIL_CAM Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_BIST_FAIL_SRAM_FIFO Fld(2, 10, AC_MSKB1)//[11:10]
    #define REG_BIST_FAIL_FIFO_RX Fld(2, 12, AC_MSKB1)//[13:12]
    #define REG_BIST_FAIL_FIFO_TX Fld(2, 14, AC_MSKB1)//[15:14]
#define OGMAC1_REG_0008_X32_OGMAC1 (0x0008)
    #define REG_SW_DES_MD Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_RX_TCP_CHKSUM_EN Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_RX_UDP_CHKSUM_EN Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_RX_IP_CHKSUM_EN Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_TX_TCP_CHKSUM_EN Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_TX_UDP_CHKSUM_EN Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_TX_IP_CHKSUM_EN Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_RX_INT_DLY_EN Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_RX_TCP_CHKSUM0_IS_NO_CHK Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_TX_TCP_CHKSUM0_IS_NO_CHK Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_TX_CHKSUM_ENDIAN_CHANGE Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_TX_FRAME_BUFFER_FORCE_CLEAN Fld(1, 11, AC_MSKB1)//[11:11]
#define OGMAC1_REG_000C_X32_OGMAC1 (0x000C)
    #define REG_RX_FRAME_NUM Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_FRAME_CYC Fld(8, 8, AC_FULLB1)//[15:8]
#define OGMAC1_REG_0010_X32_OGMAC1 (0x0010)
    #define REG_RX_DLY_COM_NUM Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_DLY_COM_OVERFLOW Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_RX_DLY_INT Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_0014_X32_OGMAC1 (0x0014)
    #define REG_RX_IP_CHKSUM_REG_L Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_001C_X32_OGMAC1 (0x001C)
    #define REG_RX_IP_CHKSUM_REG_H Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0020_X32_OGMAC1 (0x0020)
    #define REG_VER_IP Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_LEN_IP_HEADER Fld(4, 4, AC_MSKB0)//[7:4]
    #define REG_PROTOCOL Fld(8, 8, AC_FULLB1)//[15:8]
#define OGMAC1_REG_0024_X32_OGMAC1 (0x0024)
    #define REG_TOTAL_LENGTH Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0028_X32_OGMAC1 (0x0028)
    #define REG_SRC_IP_ADDR_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_002C_X32_OGMAC1 (0x002C)
    #define REG_SRC_IP_ADDR_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0030_X32_OGMAC1 (0x0030)
    #define REG_DST_IP_ADDR_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0034_X32_OGMAC1 (0x0034)
    #define REG_DST_IP_ADDR_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0038_X32_OGMAC1 (0x0038)
    #define REG_TCP_CHECKSUM Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_003C_X32_OGMAC1 (0x003C)
    #define REG_MIU_WP_UB_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0040_X32_OGMAC1 (0x0040)
    #define REG_MIU_WP_UB_1 Fld(13, 0, AC_MSKW10)//[12:0]
#define OGMAC1_REG_0044_X32_OGMAC1 (0x0044)
    #define REG_MIU_WP_LB_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0048_X32_OGMAC1 (0x0048)
    #define REG_MIU_WP_LB_1 Fld(13, 0, AC_MSKW10)//[12:0]
#define OGMAC1_REG_0050_X32_OGMAC1 (0x0050)
    #define REG_EMAC2MI_ADR_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0054_X32_OGMAC1 (0x0054)
    #define REG_EMAC2MI_ADR_1 Fld(15, 0, AC_MSKW10)//[14:0]
#define OGMAC1_REG_0058_X32_OGMAC1 (0x0058)
    #define REG_TX_IP_CHECKSUM Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_005C_X32_OGMAC1 (0x005C)
    #define REG_TX_TOTAL_LENGTH Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0060_X32_OGMAC1 (0x0060)
    #define REG_TX_TCP_UDP_CHKSUM_TAIL Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0064_X32_OGMAC1 (0x0064)
    #define REG_TX_PROTOCOL Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_TX_LEN_IP_HEADER Fld(4, 8, AC_MSKB1)//[11:8]
    #define REG_TX_VER_IP Fld(4, 12, AC_MSKB1)//[15:12]
#define OGMAC1_REG_0068_X32_OGMAC1 (0x0068)
    #define REG_TX_IPV4 Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_TX_TCP_PKT Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_TX_UDP_PKT Fld(1, 2, AC_MSKB0)//[2:2]
#define OGMAC1_REG_006C_X32_OGMAC1 (0x006C)
    #define REG_EMAC_DUMMY0 Fld(2, 0, AC_MSKB0)//[1:0]
    #define REG_EN_RXSEQ_NUMBER Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_TX_RX_REQ_PRIORITY_SWITCH Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_FIX_TX_REQADDR Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_SET_RXFIFO_16_128 Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_SET_TXFIFO_16_128 Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_RX_TX_BURST16_MODE Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_SUPPORT_MIU_HIGHWAY Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_TX_ONE_FRAME_ONLY Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_0070_X32_OGMAC1 (0x0070)
    #define REG_REGION_FIX_HALF Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_REGION_FIX_FULL Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_FIX_RXOVER_DATAW Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_PHY_TEST_MODE Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_PHY_TEST_START Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_FIX_TX_ALIGN Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_FIX_RXOVER_DESW Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_TX_TEST_EN Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_FRAME_VALID_LEN_FIX_EN Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_FIX_DESW Fld(1, 10, AC_MSKB1)//[10:10]
    #define REG_FIX_SLOW_MIU_CLK Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_FIX_CDC Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_FIX_DESR_OVER Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_FIX_DESW_OVER Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_FIX_TXSRAM_CNT Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_0074_X32_OGMAC1 (0x0074)
    #define REG_PWR_CKENABLE Fld(4, 0, AC_MSKB0)//[3:0]
    #define REG_RXDES_INCMODE Fld(2, 8, AC_MSKB1)//[9:8]
    #define REG_GAT_SRAM_RX_2P Fld(1, 12, AC_MSKB1)//[12:12]
    #define REG_GAT_SRAM_TX_2P Fld(1, 13, AC_MSKB1)//[13:13]
    #define REG_GAT_SRAM_RX Fld(1, 14, AC_MSKB1)//[14:14]
    #define REG_GAT_SRAM_TX Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_007C_X32_OGMAC1 (0x007C)
    #define REG_EMAC_RIU1F Fld(2, 0, AC_MSKB0)//[1:0]
#define OGMAC1_REG_0080_X32_OGMAC1 (0x0080)
    #define REG_EN_RX_ALIGN_BE Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_EN_MIU_ARB Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_PHY_TEST_16B Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_FIX_HALF_SHORT Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_FIX_DESC_COUNT Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_FIX_DUPLEX_CHG Fld(1, 5, AC_MSKB0)//[5:5]
#define OGMAC1_REG_0084_X32_OGMAC1 (0x0084)
    #define REG_GMAC_21H Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0088_X32_OGMAC1 (0x0088)
    #define REG_GMAC_MODE Fld(1, 7, AC_MSKB0)//[7:7]
#define OGMAC1_REG_00AC_X32_OGMAC1 (0x00AC)
    #define REG_TX_DESC_BASEADDR_LO Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_00B0_X32_OGMAC1 (0x00B0)
    #define REG_TX_DESC_BASEADDR_LO Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_00B4_X32_OGMAC1 (0x00B4)
    #define REG_TX_DESCRIPTOR_PTR_LO Fld(14, 0, AC_MSKW10)//[13:0]
#define OGMAC1_REG_00B8_X32_OGMAC1 (0x00B8)
    #define REG_TX_PACKET_COUNT_LO Fld(14, 0, AC_MSKW10)//[13:0]
    #define REG_TX_DESC_OV_LO Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_00BC_X32_OGMAC1 (0x00BC)
    #define REG_TX_DESC_TH_LO Fld(14, 0, AC_MSKW10)//[13:0]
    #define REG_EN_TX_DESCRIPTOR Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_00C0_X32_OGMAC1 (0x00C0)
    #define REG_TX_DESC_BASEADDR_HI Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_00C4_X32_OGMAC1 (0x00C4)
    #define REG_TX_DESC_BASEADDR_HI Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_00C8_X32_OGMAC1 (0x00C8)
    #define REG_TX_DESCRIPTOR_PTR_HI Fld(14, 0, AC_MSKW10)//[13:0]
#define OGMAC1_REG_00CC_X32_OGMAC1 (0x00CC)
    #define REG_TX_PACKET_COUNT_HI Fld(14, 0, AC_MSKW10)//[13:0]
    #define REG_TX_DESC_OV_HI Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_00D0_X32_OGMAC1 (0x00D0)
    #define REG_TX_DESC_TH_HI Fld(14, 2, AC_MSKW10)//[15:2]
#define OGMAC1_REG_00EC_X32_OGMAC1 (0x00EC)
    #define REG_TX_COM_NUM Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_COM_OVERFLOW Fld(1, 9, AC_MSKB1)//[9:9]
    #define REG_DLY_INT Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_00F0_X32_OGMAC1 (0x00F0)
    #define REG_TX_FRAME_NUM Fld(8, 0, AC_FULLB0)//[7:0]
    #define REG_RX_FRAME_CYC Fld(8, 8, AC_FULLB1)//[15:8]
#define OGMAC1_REG_0110_X32_OGMAC1 (0x0110)
    #define REG_MAX_FRAME_L Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_0114_X32_OGMAC1 (0x0114)
    #define REG_MAX_FRAME_H Fld(5, 0, AC_MSKB0)//[4:0]
#define OGMAC1_REG_0118_X32_OGMAC1 (0x0118)
    #define REG_MAX_BYTE_L Fld(16, 0, AC_FULLW10)//[15:0]
#define OGMAC1_REG_011C_X32_OGMAC1 (0x011C)
    #define REG_MAX_BYTE_H Fld(5, 0, AC_MSKB0)//[4:0]
#define OGMAC1_REG_0120_X32_OGMAC1 (0x0120)
    #define REG_SUP_INTV Fld(6, 0, AC_MSKB0)//[5:0]
    #define REG_STORM_CTRL Fld(3, 8, AC_MSKB1)//[10:8]
    #define REG_STORM_MODE Fld(1, 11, AC_MSKB1)//[11:11]
    #define REG_PAD_RX_CLK_REF_SRC Fld(1, 15, AC_MSKB1)//[15:15]
#define OGMAC1_REG_0124_X32_OGMAC1 (0x0124)
    #define REG_RXSRAM_CNT_THR Fld(11, 0, AC_MSKW10)//[10:0]
#define OGMAC1_REG_0128_X32_OGMAC1 (0x0128)
    #define REG_RXDES_CNT_THR Fld(5, 0, AC_MSKB0)//[4:0]
    #define REG_RX_DES_UNAVAILABLE_OV Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_EN_RBNA_THR Fld(1, 15, AC_MSKB1)//[15:15]


#endif
