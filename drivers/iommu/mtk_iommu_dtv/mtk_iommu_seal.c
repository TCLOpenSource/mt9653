// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/bug.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/sizes.h>
#include <linux/of_address.h>

#include "mtk_iommu_dtv.h"
#include "mtk_iommu_common.h"
#include "mtk_iommu_internal.h"

struct seal_regmaps {
	void __iomem *base;
	unsigned int size;
};

static struct seal_regmaps *seal_reg;

int mtkd_seal_getaidtype(struct mdseal_ioc_data *psealdata)
{
#define SEAL_GNAID_NUMMAXIMUM     (64)
#define SEAL_AID_TYPE_SLOTNUM     (8)
#define SEAL_AID_TYPE_BITNUM      (2)
#define SEAL_AID_TYPE_BITMASK     (0x3)
#define REG_SEAL_AID_TYPE         (0x94)
#define SEAL_REGOFFSET            (0x4)
	uint32_t aid;
	uint32_t entype_regoffset, entype_slotoffset, entype_val;
	void __iomem *regaddr = seal_reg->base;
	uint32_t regoffset;

	if (!psealdata || !seal_reg) {
		pr_err("%s %d, psealdata or regbase is NULL!", __func__, __LINE__);
		return -EINVAL;
	}

	aid = psealdata->pipelineID & SEAL_AID_INDEX_MASK;
	if (aid >= SEAL_GNAID_NUMMAXIMUM) {
		pr_err("%s %d, gaid in pipelineID %d is over GAID maximum %d!",
		       __func__, __LINE__, (int)psealdata->pipelineID,
					(int)SEAL_GNAID_NUMMAXIMUM);
		return -EINVAL;
	}

	pr_info("%s:%d, pipelineID = 0x%x!\n",
		__func__, __LINE__, psealdata->pipelineID);

	// one reg include 8 aid
	// one aid occupied 2 bits
	entype_regoffset = aid / SEAL_AID_TYPE_SLOTNUM;
	entype_slotoffset = (aid % SEAL_AID_TYPE_SLOTNUM) * SEAL_AID_TYPE_BITNUM;

	regoffset = REG_SEAL_AID_TYPE + (entype_regoffset * SEAL_REGOFFSET);
	if (regoffset > (seal_reg->size)) {
		pr_err("%s %d, regoffset 0x%x is over mapping size 0x%x!",
		       __func__, __LINE__, regoffset, seal_reg->size);
		return -EINVAL;
	}

	entype_val = readl(regaddr + regoffset);
	entype_val = (entype_val >> entype_slotoffset) & SEAL_AID_TYPE_BITMASK;
	psealdata->enType = (EN_AID_TYPE) entype_val;
	pr_info("%s %d, aidtype = 0x%x!\n", __func__, __LINE__, psealdata->enType);

	return 0;
}
EXPORT_SYMBOL(mtkd_seal_getaidtype);

int seal_register(struct device *dev)
{
	int size;
	struct device_node *node;
	struct resource r;

	if (!dev)
		return -ENODEV;

	node = dev->of_node;
	if (!node)
		return -ENODEV;

	dev_info(dev, "[%s] : %s\n", __func__, node->name);
	if (!of_get_property(node, "reg", &size)) {
		dev_err(dev, "mtk iommu failed to of_get_property\n");
		return -ENODEV;
	}

	if (of_address_to_resource(node, 0, &r)) {
		dev_err(dev, "mtk iommu failed to of_address_to_resource\n");
		return -ENOMEM;
	}

	pr_crit("we have : %s\n", r.name);

	seal_reg = devm_kzalloc(dev, sizeof(struct seal_regmaps), GFP_KERNEL);
	if (!seal_reg)
		return -ENOMEM;

	seal_reg->base = devm_ioremap_resource(dev, &r);
	if (IS_ERR(seal_reg->base)) {
		dev_err(dev, "mtk iommu failed to devm_ioremap_resource\n");
		return PTR_ERR(seal_reg->base);
	}

	seal_reg->size = resource_size(&r);
	dev_info(dev, "[%s] 0x%lx 0x%x\n", __func__,
			(unsigned long)seal_reg->base, seal_reg->size);

	return 0;
}
