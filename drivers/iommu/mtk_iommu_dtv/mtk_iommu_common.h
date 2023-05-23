/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#ifndef _MTK_IOMMU_COMMON_H_
#define _MTK_IOMMU_COMMON_H_
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/iommu.h>
#include <linux/miscdevice.h>

#include "mtk_iommu_dtv_api.h"

#define ION_DEV_NAME "/dev/ion"

#define KTIME_FORMAT_LEN	16
#define MAX_NAME_SIZE		16
#define UUID_SIZE		16
#define BUFTAGMASK		0xFF000000
#define BUFTAG_SHIFT		24
#define IOMMU_DMA_MASK		34

struct TEE_SvpMmaPipeInfo {
	char buffer_tag[MAX_NAME_SIZE];
	uint32_t u32bufferTagLen;
	uint64_t u64BufferAddr;
	uint32_t u32BufferSize;
};

struct mtk_iommu_range_t {
	u64 start;		//start addr
	u64 size;		// size
};

struct iommu_group {
	struct kobject kobj;
	struct kobject *devices_kobj;
	struct list_head devices;
	struct mutex mutex;
	struct blocking_notifier_head notifier;
	void *iommu_data;
	void (*iommu_data_release)(void *iommu_data);
	char *name;
	int id;
	struct iommu_domain *default_domain;
	struct iommu_domain *domain;
};


struct mtk_dtv_iommu_data {
	struct device *dev;
	struct device *direct_dev;
	struct miscdevice misc_dev;
	struct iommu_device iommu;
	struct iommu_group *group;
	struct mutex buf_lock;
	struct mutex mapsg_lock;
	struct mutex share_lock;
	struct list_head buf_list_head;
	struct list_head space_list_head;
	struct list_head dfree_list_head;
	struct list_head physical_buf_list_head;
	struct list_head share_list_head;
	struct mutex physical_buf_lock;
	struct dentry *debug_root;
	struct dentry *buf_debug_root;
	u32 record_alloc_time;
	u32 calltrace_enable;
	int iommu_status;
	struct workqueue_struct *wq;
	int irq;
	struct idr global_name_idr;
	u32 log_level;
	unsigned long asym_addr_start;
};

enum mtk_iommu_addr_type {
	MTK_IOMMU_ADDR_TYPE_IOVA,
	MTK_IOMMU_ADDR_TYPE_PA,
	MTK_IOMMU_ADDR_TYPE_MAX
};

enum mtk_iommu_unauth_status_e {
	MTK_IOMMU_UNAUTH_SUCCESS,
	MTK_IOMMU_UNAUTH_DELAY_FREE,
	MTK_IOMMU_UNAUTH_FREE_RANGE,
	MTK_IOMMU_UNAUTH_MAX
};

enum mtk_iommu_of_heap_type {
	HEAP_TYPE_IOMMU,
	HEAP_TYPE_CMA,
	HEAP_TYPE_CMA_IOMMU,
	HEAP_TYPE_CARVEOUT,
	HEAP_TYPE_MAX,
};

#endif
