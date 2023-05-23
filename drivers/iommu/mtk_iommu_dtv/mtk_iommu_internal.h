/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_IOMMU_INTERNAL_H_
#define __MTK_IOMMU_INTERNAL_H_
#include "mtk_iommu_dtv.h"
#include "mtk_iommu_mixed_mma.h"

extern u32 dbg_level;

enum {
	E_LOG_ALERT = 3,	/* alloc + free */
	E_LOG_CRIT,		/* alloc + free + ops + util */
	E_LOG_NOTICE,		/* alloc + free + ops + util + map + unmap */
	E_LOG_INFO,		/* alloc + free + ops + util + map + unmap + sync */
	E_LOG_DEBUG,		/* verbose */
};

#define IOMMU_DEBUG(_loglevel, fmt, args...)	do {					\
	if (_loglevel <= dbg_level)							\
		pr_emerg("[IOMMU][%s][%d][tgid:%d][pid:%d] " fmt,			\
			__func__, __LINE__, current->tgid, current->pid, ## args);	\
} while (0)

int __mma_callback_register(struct mma_callback *mma_cb);

int get_iommu_data(struct mtk_dtv_iommu_data **data);

void *__mtk_iommu_alloc_attrs(struct device *dev, size_t size,
		dma_addr_t *dma_addr, gfp_t flag, unsigned long attrs);

void __mtk_iommu_free_attrs(struct device *dev,
		size_t size, void *cpu_addr,
		dma_addr_t handle, unsigned long attrs);

void __free_internal(struct mtk_iommu_buf_handle *buf_handle);

void __do_TEEMap_internal(struct mtk_iommu_buf_handle *handle);

int iommud_misc_register(struct mtk_dtv_iommu_data *data);

int seal_register(struct device *dev);

static inline struct mtk_iommu_buf_handle *__mtk_iommu_create_buf_handle(u32 length,
						const char *buf_tag)
{
	static u32 serial_num;
	struct mtk_iommu_buf_handle *buf_handle = NULL;

	buf_handle = kzalloc(sizeof(*buf_handle), GFP_KERNEL);
	if (!buf_handle)
		return NULL;

	buf_handle->length = length;
	if (buf_tag) {
		strncpy(buf_handle->buf_tag, buf_tag, MAX_NAME_SIZE);
		buf_handle->buf_tag[MAX_NAME_SIZE - 1] = '\0';
	}
	buf_handle->pid = current->pid;
	buf_handle->tgid = current->tgid;
	scnprintf(buf_handle->comm, sizeof(buf_handle->comm),
			"%s", current->comm);
	buf_handle->kvaddr = NULL;
	serial_num++;
	buf_handle->serial = serial_num;
	buf_handle->entry = NULL;
	buf_handle->va2iova_pages = NULL;
	buf_handle->tee_map = TEE_MAP_DEFAULT;
	mutex_init(&buf_handle->handle_lock);

	return buf_handle;
}

static inline struct mtk_iommu_buf_handle *__mtk_iommu_find_name(dma_addr_t name,
			struct mtk_dtv_iommu_data *data)
{
	struct mtk_iommu_buf_handle *buf_handle = NULL;

	mutex_lock(&(data->buf_lock));
	buf_handle = idr_find(&data->global_name_idr, (unsigned long)name);
	if (IS_ERR_OR_NULL(buf_handle)) {
		mutex_unlock(&(data->buf_lock));
		return NULL;
	}
	mutex_unlock(&(data->buf_lock));

	return buf_handle;
}

static inline struct mtk_iommu_buf_handle *__mtk_iommu_find_buf_handle(dma_addr_t addr,
			struct mtk_dtv_iommu_data *data)
{
	struct mtk_iommu_buf_handle *buf_handle = NULL;

	if (!addr)
		return NULL;

	mutex_lock(&(data->buf_lock));
	list_for_each_entry(buf_handle,
		&(data->buf_list_head), buf_list_node) {
		if (buf_handle->addr <= addr && buf_handle->addr + buf_handle->length > addr) {
			mutex_unlock(&(data->buf_lock));
			return buf_handle;
		}
	}
	mutex_unlock(&(data->buf_lock));

	return NULL;
}

static inline struct mtk_iommu_buf_handle *__mtk_iommu_find_kva(void *kva,
			struct mtk_dtv_iommu_data *data)
{
	struct mtk_iommu_buf_handle *buf_handle = NULL;

	if (!kva)
		return NULL;

	mutex_lock(&(data->buf_lock));
	list_for_each_entry(buf_handle,
		&(data->buf_list_head), buf_list_node) {
		if (buf_handle->kvaddr == kva) {
			mutex_unlock(&(data->buf_lock));
			return buf_handle;
		}
	}
	mutex_unlock(&(data->buf_lock));

	return NULL;
}

static inline int __mtk_iommu_get_space_tag(unsigned int buf_tag,
			char **space_tag, struct mtk_dtv_iommu_data *data)
{
	int i;
	struct mtk_iommu_space_handle *handle = NULL;

	buf_tag = buf_tag & BUFTAGMASK;
	*space_tag = NULL;
	if (list_empty(&(data->space_list_head)))
		return 0;

	mutex_lock(&(data->buf_lock));
	list_for_each_entry(handle, &(data->space_list_head), list_node) {
		for (i = 0; i < handle->data.buf_tag_num; i++) {
			if (buf_tag == handle->data.buf_tag_array[i]) {
				*space_tag = handle->data.space_tag;
				mutex_unlock(&(data->buf_lock));
				return 0;
			}
		}
	}
	mutex_unlock(&(data->buf_lock));

	return 0;
}
#endif
