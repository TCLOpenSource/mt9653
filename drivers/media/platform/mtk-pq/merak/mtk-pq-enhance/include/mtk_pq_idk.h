/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_IDK_H
#define _MTK_PQ_IDK_H


//#define DOLBY_IDK_DUMP_ENABLE


// function
int _mtk_pq_idkdump_store(struct device *dev, const char *buf);
int _mtk_pq_idkdump_show(struct device *dev, char *buf);
#endif
