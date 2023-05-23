/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
 */
#ifndef MTK_IOMMU_SMI_H
#define MTK_IOMMU_SMI_H

#include <linux/bitops.h>
#include <linux/device.h>
#include <dt-bindings/memory/mtk-smi-larb-port.h>

#if IS_ENABLED(CONFIG_MTK_SMI) || IS_ENABLED(CONFIG_MTK_SMI_TV)


#define MTK_SMI_MMU_EN(port)	BIT(port)

struct mtk_smi_larb_iommu {
	struct device *dev;
	unsigned int   mmu;
	unsigned int   bank[32];
};
/*
 * mtk_smi_larb_get: Enable the power domain and clocks for this local arbiter.
 *                   It also initialize some basic setting(like iommu).
 * mtk_smi_larb_put: Disable the power domain and clocks for this local arbiter.
 * Both should be called in non-atomic context.
 *
 * Returns 0 if successful, negative on failure.
 */
int mtk_smi_larb_get(struct device *larbdev);
void mtk_smi_larb_put(struct device *larbdev);
void mtk_smi_common_bw_set(struct device *dev, const u32 port, const u32 val);
void mtk_smi_larb_bw_set(struct device *dev, const u32 port, const u32 val);

#else

static inline int mtk_smi_larb_get(struct device *larbdev)
{
	return 0;
}

static inline void mtk_smi_larb_put(struct device *larbdev) { }

static inline void
mtk_smi_common_bw_set(struct device *dev, const u32 port, const u32 val) { }
static inline void
mtk_smi_larb_bw_set(struct device *dev, const u32 port, const u32 val) { }
#endif

#endif
