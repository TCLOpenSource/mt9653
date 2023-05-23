/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_TV_SYNC_H_
#define MTK_TV_SYNC_H_

#include <linux/dma-fence.h>
#include "../../../../../dma-buf/sync_debug.h"

#define MTK_TV_INVALID_FENCE_FD (-1)

#define MAX_TIMELINE_NAME	(100)

struct mtk_fence_data {
	int fence_seqno;
	char name[32];
	__s32 fence; /* fd of new fence */
};

struct sync_timeline *mtk_tv_sync_timeline_create(const char *name);
int mtk_tv_sync_fence_create(struct sync_timeline *obj, struct mtk_fence_data *data);
void mtk_tv_sync_timeline_inc(struct sync_timeline *obj, u32 value);

#endif
