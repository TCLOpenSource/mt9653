/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#ifndef _MTK_IOMMU_DEBUGFS_H_
#define _MTK_IOMMU_DEBUGFS_H_

int mtk_iommu_sysfs_init(struct device *dev,
			struct mtk_dtv_iommu_data *mtk_iommu_data);
int mtk_iommu_sysfs_destroy(struct device *dev);

#endif
