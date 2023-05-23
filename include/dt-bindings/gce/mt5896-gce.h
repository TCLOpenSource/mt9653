/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Bibby Hsieh <bibby.hsieh@mediatek.com>
 *
 */

#ifndef _DT_BINDINGS_GCE_MT5896_H
#define _DT_BINDINGS_GCE_MT5896_H

#define SUBSYS_VDEC_SOC   9
#define SUBSYS_VDEC_LAT   10
#define SUBSYS_VDEC_CORE0 11
#define SUBSYS_VDEC_CORE1 12
#define SUBSYS_VDEC_CORE2 13
#define SUBSYS_VDEC_0x1D9Dxxxx 17

/* GCE0 HW EVENT */

// VDEC event
#define CMDQ_EVENT_PIC_START       0
#define CMDQ_EVENT_FRAME_DONE      1
#define CMDQ_EVENT_VDEC_PAUSE      2
#define CMDQ_EVENT_VDEC_ERROR      3
#define CMDQ_EVENT_MC_BUSY         4
#define CMDQ_EVENT_DRAM_REQUEST    5
#define CMDQ_EVENT_FETCH_READY     6
#define CMDQ_EVENT_SRAM_STABLE     7
#define CMDQ_EVENT_SEARCH_DONE     8
#define CMDQ_EVENT_REF_REORER_DONE 9
#define CMDQ_EVENT_WP_TBLE_DONE    10
#define CMDQ_EVENT_CTX_SRAM_CLR    11
#define CMDQ_EVENT_RESERVED1       12
#define CMDQ_EVENT_RESERVED2       13
#define CMDQ_EVENT_RESERVED3       14
#define CMDQ_EVENT_RESERVED4       15
#endif
