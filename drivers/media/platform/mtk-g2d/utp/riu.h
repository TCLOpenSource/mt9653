/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef RIU_H
#define RIU_H


//---------------- AC_FULL/MSK ----------------
//  B3[31:24] | B2[23:16] | B1[15:8] | B0[7:0]
//  W32[31:16] | W21[23:8] | W10 [15:0]
//  DW[31:0]
//---------------------------------------------

#define AC_FULLB0           1
#define AC_FULLB1           2
#define AC_FULLB2           3
#define AC_FULLB3           4
#define AC_FULLW10          5
#define AC_FULLW21          6
#define AC_FULLW32          7
#define AC_FULLDW           8
#define AC_MSKB0            9
#define AC_MSKB1            10
#define AC_MSKB2            11
#define AC_MSKB3            12
#define AC_MSKW10           13
#define AC_MSKW21           14
#define AC_MSKW32           15
#define AC_MSKDW            16


//-------------------- Fld --------------------
//    wid[31:16] | shift[15:8] | ac[7:0]
//---------------------------------------------

#define Fld(wid, shft, ac)    (((uint32_t)wid<<16)|(shft<<8)|ac)
#define Fld_wid(fld)        (uint8_t)((fld)>>16)
#define Fld_shft(fld)       (uint8_t)((fld)>>8)
#define Fld_ac(fld)         (uint8_t)((fld))

void mtk_write2byte(uint32_t u32P_Addr, uint32_t u32Value);
void mtk_write2bytemask(uint32_t u32P_Addr, uint32_t u32Value, uint32_t fld);
uint32_t mtk_read2byte(uint32_t u32P_Addr);
uint32_t mtk_read2bytemask(uint32_t u32P_Addr, uint32_t fld);
void Set_RIU_Base(uint64_t u64V_Addr, uint32_t u32P_Addr);

#endif//RIU_H
