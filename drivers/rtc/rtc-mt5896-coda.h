/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#ifndef _RTC_MT5896_CODA_H_
#define _RTC_MT5896_CODA_H_

//Page PM_RTC0
#define PM_RTC_REG_0000_PM_RTC0 (0x0000)
    #define REG_C_SOFT_RSTZ Fld(1, 0, AC_MSKB0)//[0:0]
    #define REG_C_CNT_EN Fld(1, 1, AC_MSKB0)//[1:1]
    #define REG_C_WRAP_EN Fld(1, 2, AC_MSKB0)//[2:2]
    #define REG_C_LOAD_EN Fld(1, 3, AC_MSKB0)//[3:3]
    #define REG_C_READ_EN Fld(1, 4, AC_MSKB0)//[4:4]
    #define REG_C_INT_MASK Fld(1, 5, AC_MSKB0)//[5:5]
    #define REG_C_INT_FORCE Fld(1, 6, AC_MSKB0)//[6:6]
    #define REG_C_INT_CLEAR Fld(1, 7, AC_MSKB0)//[7:7]
    #define REG_RTC_RAW_INT Fld(1, 8, AC_MSKB1)//[8:8]
    #define REG_RTC_INT Fld(1, 9, AC_MSKB1)//[9:9]
#define PM_RTC_REG_0004_PM_RTC0 (0x0004)
    #define REG_C_FREQ_CW_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0008_PM_RTC0 (0x0008)
    #define REG_C_FREQ_CW_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_000C_PM_RTC0 (0x000C)
    #define REG_C_LOAD_VAL_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0010_PM_RTC0 (0x0010)
    #define REG_C_LOAD_VAL_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0014_PM_RTC0 (0x0014)
    #define REG_C_LOAD_VAL_2 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0018_PM_RTC0 (0x0018)
    #define REG_C_LOAD_VAL_3 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_001C_PM_RTC0 (0x001C)
    #define REG_C_MATCH_VAL_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0020_PM_RTC0 (0x0020)
    #define REG_C_MATCH_VAL_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0024_PM_RTC0 (0x0024)
    #define REG_C_MATCH_VAL_2 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0028_PM_RTC0 (0x0028)
    #define REG_C_MATCH_VAL_3 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_002C_PM_RTC0 (0x002C)
    #define REG_RTC_CNT_0 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0030_PM_RTC0 (0x0030)
    #define REG_RTC_CNT_1 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0034_PM_RTC0 (0x0034)
    #define REG_RTC_CNT_2 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0038_PM_RTC0 (0x0038)
    #define REG_RTC_CNT_3 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0040_PM_RTC0 (0x0040)
    #define REG_DUMMY0 Fld(16, 0, AC_FULLW10)//[15:0]
#define PM_RTC_REG_0044_PM_RTC0 (0x0044)
    #define REG_DUMMY1 Fld(16, 0, AC_FULLW10)//[15:0]

#endif

