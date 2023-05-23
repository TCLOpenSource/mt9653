/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#ifndef _MTK_IOMMU_DTV_H_
#define _MTK_IOMMU_DTV_H_

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <soc/mediatek/smi.h>
#include <linux/dma-buf.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/cma.h>

#include "mtk_iommu_common.h"
#include "mtk_iommu_dtv_api.h"

#define MAX_TAG_NUM   8
#define MAX_MPU_NR    2

struct mtk_iommu_buf_handle {
	char buf_tag[MAX_NAME_SIZE];
	u32 buf_tag_id;
	struct list_head buf_list_node;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct delayed_work work;
	struct mutex handle_lock;
	union {
		struct dma_buf *db_ion;
		phys_addr_t phys;
	} b;
	struct sg_table *sgt;
	struct sg_table *internal_sgt;
	struct dentry *entry;
	size_t length;
	dma_addr_t addr;	//buffer address
	bool is_iova;		//whether addr is iova
	bool is_secure;		//whethe secure buffer
	bool iova2x;
	bool is_freeing;
	int miu_select;		//which miu
	int ref;		//reference count
	void *kvaddr;
	pid_t pid;
	pid_t tgid;
	pid_t map_pid;
	char comm[TASK_COMM_LEN];
	u32 serial;
	u32 alloc_time;
	char buf_alloc_time[KTIME_FORMAT_LEN];
	u32 map_time;
	u32 pipe_id;
	u32 auth_count;		// authorize ref count
	int tee_map;
	struct page **va2iova_pages;
	u32 num_pages;
	unsigned long attrs;
	int sg_map_count;
	int global_name;
	int wide;
	int narrow;
};

struct mtk_iommu_space_handle {
	struct mdiommu_reserve_iova data;
	struct list_head list_node;
};

struct diommu_userdev_info {
	pid_t tpid;
	pid_t mmap_tpid;
	void *private_data;
	void *tmp;
};

enum iommu_tee_map_type {
	TEE_MAP_REMOVE = -1,
	TEE_MAP_NON,
	TEE_MAP_DEFAULT,
	TEE_MAP_DELAY,
};

struct mtk_iommu_share {
	struct dma_buf *db;
	u64 addr;
	unsigned int size;
	u32 dmabuf_id;
	struct list_head list;
};

#endif
