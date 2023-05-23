/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */
#ifndef _LINUX_MTK_IOMMU_CARVEOUT_HEAP_H
#define _LINUX_MTK_IOMMU_CARVEOUT_HEAP_H

#define MTK_ION_CARVEOUT_HEAP_NAME "mtkdtv_carveout_heap"

int mtk_carveout_heap_init(void);
void mtk_carveout_heap_exit(void);

#endif
