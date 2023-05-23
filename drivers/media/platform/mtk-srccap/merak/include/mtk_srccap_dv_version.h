/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DV_VER_H
#define MTK_SRCCAP_DV_VER_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
// Range: mtk_srccap_dv.h, mtk_srccap_dv.c
// 1: init version
// 2: VSIF H14B and testcase (hashtag: DolbyVisionIDKDump)
// 3: Add STR Flow and Fix KASAN Error
// 4: Add PR implementation
// 5. Add BT2020 low-latency feature
// 6: Add DV descrb streamon/streamoff handler
#define SRCCAP_DV_VERSION (6)

// Range: mtk_srccap_dv_common.h, mtk_srccap_dv_common.c
// 1: init version
// 2: Add PR implementation
// 3: Add SRCCAP DV HW version handle
// 4: Add update deq setting function for HDMI422pack
#define SRCCAP_DV_COMMON_VERSION (4)

// Range: mtk_srccap_dv_descrb.h, mtk_srccap_dv_descrb.c
// 1: init version
// 2: VSIF H14B and testcase (hashtag: DolbyVisionIDKDump)
// 3: Add STR Flow and Fix KASAN Error
// 4: Fix descramble read back issue.
// 5. Add BT2020 low-latency feature
// 6: Add DV descrb streamon/streamoff handler, add NEG flow, add DV Game report
// 7: Add M6L diff & Add SRCCAP DV HW version handle
// 8: Add Transition flow
// 9: Fix coverity
//10: Add FRL as condition of np node in descrb
//11: Add DV Game Detection
#define SRCCAP_DV_DESCRB_VERSION (11)

// Range: mtk_srccap_dv_dma.h, mtk_srccap_dv_dma.c
//        mtk_srccap_dv_sd.h, mtk_srccap_dv_sd.c
// 1: init version
// 2: Add PR implementation
// 3: Add M6L diff & Add SRCCAP DV HW version handle
// 4: Fix scaling down condition
// 5: Add RGB path for STD_Tunnel / LL_RGB in dv_dma
// 6: refine pre-sd conditions
// 7: add rgb case for dv dma memory format
#define SRCCAP_DV_DMA_VERSION (7)

// Range: mtk_srccap_dv_meta.h, mtk_srccap_dv_meta.c
// 1: init version
// 2: Add STR Flow and Fix KASAN Error
// 3: Add PR implementation
// 4: Add BT2020 low-latency feature
// 5: Add srccap HW version to meta
// 6: refine stack size
#define SRCCAP_DV_META_VERSION (6)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------

#endif  // MTK_SRCCAP_DV_VER_H
