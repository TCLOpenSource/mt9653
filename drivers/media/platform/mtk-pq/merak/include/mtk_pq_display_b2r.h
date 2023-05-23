/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_DISPLAY_B2R_H
#define _MTK_PQ_DISPLAY_B2R_H
#include <linux/types.h>
#include <linux/timekeeping.h>
#include <uapi/linux/mtk-v4l2-pq.h>

#include "mtk_pq.h"

//----------------------------------------------------------------------------
//  Driver Capability
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//  Macro and Define
//----------------------------------------------------------------------------

int mtk_pq_display_b2r_qbuf(struct mtk_pq_device *pq_dev, struct mtk_pq_buffer *pq_buf);
#endif
