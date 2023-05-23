/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_DV_VERSION_H
#define _MTK_PQ_DV_VERSION_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------

// Range: mtk-pq/merak/mtk-pq-enhance/hdr/mtk_pq_dv.c,
//        mtk-pq/merak/mtk-pq-enhance/include/mtk_pq_dv.h
// 1: init version
// 2: Fix force_reg drop 0xFFFF and X32 offset
// 3: Add force_viewmode
// 4: Add PR implementation
// 5: Fix pyramid frame pitch alignment
// 6: Add mm case PR implementation
// 7: Add auto config & Adv. SDK EUC (sync with DV_HWREG_VERSION 4)
// 8: Add Adv. SDK light sense
// 9: Refactor stack size
#define DV_PQ_VERSION (9)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------

#endif  // _MTK_PQ_DV_VERSION_H
