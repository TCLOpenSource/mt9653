/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author Owen Tseng <Owen.Tseng@mediatek.com>
 */

#ifndef _BDMA_MT5896_CODA_H_
#define _BDMA_MT5896_CODA_H_

#define REG_0000_BDMA_CH0 (0x000)
#define REG_CH0_TRIG Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_CH0_STOP Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_SRC_TLB Fld(1, 8, AC_MSKB1)//[8:8]
#define REG_DST_TLB Fld(1, 9, AC_MSKB1)//[9:9]
#define REG_CH0_SRC_A_H Fld(2, 10, AC_MSKB1)//[11:10]
#define REG_CH0_DST_A_H Fld(2, 12, AC_MSKB1)//[13:12]
#define REG_CH0_SIZE_H Fld(2, 14, AC_MSKB1)//[15:14]
#define REG_0004_BDMA_CH0 (0x004)
#define REG_CH0_QUEUED Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_CH0_BUSY Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_CH0_INT_BDMA Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_CH0_DONE Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_CH0_RESULT0 Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_XIU_BDMA_NS Fld(3, 5, AC_MSKB0)//[7:5]
#define REG_0008_BDMA_CH0 (0x008)
#define REG_CH0_SRC_SEL Fld(4, 0, AC_MSKB0)//[3:0]
#define REG_CH0_SRC_DW Fld(3, 4, AC_MSKB0)//[6:4]
#define REG_CH0_DST_SEL Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_CH0_DST_DW Fld(3, 12, AC_MSKB1)//[14:12]
#define REG_000C_BDMA_CH0 (0x00C)
#define REG_CH0_DEC Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_CH0_INT_EN Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_CH0_MIU_PRIORITY Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_CH0_CFG Fld(4, 4, AC_MSKB0)//[7:4]
#define REG_CH0_FLUSH_WD Fld(4, 8, AC_MSKB1)//[11:8]
#define REG_CH0_REPLACE_MIU Fld(4, 12, AC_MSKB1)//[15:12]
#define REG_0010_BDMA_CH0 (0x010)
#define REG_CH0_SRC_A_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0014_BDMA_CH0 (0x014)
#define REG_CH0_SRC_A_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0018_BDMA_CH0 (0x018)
#define REG_CH0_DST_A_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_001C_BDMA_CH0 (0x01C)
#define REG_CH0_DST_A_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0020_BDMA_CH0 (0x020)
#define REG_CH0_SIZE_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0024_BDMA_CH0 (0x024)
#define REG_CH0_SIZE_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0028_BDMA_CH0 (0x028)
#define REG_CH0_CMD0_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_002C_BDMA_CH0 (0x02C)
#define REG_CH0_CMD0_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0030_BDMA_CH0 (0x030)
#define REG_CH0_CMD1_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0034_BDMA_CH0 (0x034)
#define REG_CH0_CMD1_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0038_BDMA_CH0 (0x038)
#define REG_CH0_CMD2_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_003C_BDMA_CH0 (0x03C)
#define REG_CH0_CMD2_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define REG_0040_BDMA_CH0 (0x040)
#define REG_CH0_PL_REMAINED_SPACE_TRIG Fld(1, 5, AC_MSKB0)//[5:5]

#endif

