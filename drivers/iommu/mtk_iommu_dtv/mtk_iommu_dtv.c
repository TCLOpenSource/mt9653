// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */
#include <linux/bug.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/dma-iommu.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/iopoll.h>
#include <linux/kmemleak.h>
#include <linux/list.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <asm/barrier.h>
#include <linux/module.h>
#include <linux/dma-buf.h>
#include <linux/fs.h>
#include <linux/cma.h>
#include <linux/syscalls.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/cache.h>
#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <soc/mediatek/smi.h>
#include <linux/version.h>
#include <linux/memblock.h>
#include <linux/sizes.h>

#include "mtk_iommu_dtv.h"
#include "mtk_iommu_internal.h"
#include "mtk_iommu_of.h"
#include "mtk_iommu_ion.h"
#include "mtk_iommu_sysfs.h"
#include "mtk_iommu_common.h"
#include "mtk_iommu_tee_interface.h"
#include "heaps/mtk_iommu_ion_heap.h"
#include "mtk_iommu_test.h"
#include "mtk_iommu_statist.h"
#include "mtk_iommu_mixed_mma.h"

#define DMA_ERROR_CODE	DMA_MAPPING_ERROR

static struct mtk_dtv_iommu_data *iommu_data;
static struct mma_callback *mma_callback_ops;

int __mma_callback_register(struct mma_callback *mma_cb)
{
	if (!mma_cb)
		return -EINVAL;

	mma_callback_ops->alloc = mma_cb->alloc;
	mma_callback_ops->free = mma_cb->free;
	mma_callback_ops->map_iova = mma_cb->map_iova;

	if (!mma_callback_ops->alloc || !mma_callback_ops->free)
		return -EINVAL;

	return 0;
}

int get_iommu_data(struct mtk_dtv_iommu_data **data)
{
	if (!iommu_data) {
		pr_err("[IOMMU] iommu driver is not ready\n");
		return -ENODEV;
	}

	if (!iommu_data->iommu_status) {
		pr_err("[IOMMU] iommu driver is not support\n");
		return -EOPNOTSUPP;
	}

	*data = iommu_data;

	return 0;
}

void __do_TEEMap_internal(struct mtk_iommu_buf_handle *handle)
{
	int ret = 0;
	u64 addr = 0;
	char *space_tag = NULL;
	struct timespec64 tv0 = {0}, tv1 = {0};
	u32 maptime = 0;
	struct mtk_dtv_iommu_data *data = NULL;

	if (IS_ERR_OR_NULL(handle) || IS_ERR_OR_NULL(handle->sgt))
		return;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	if (data->record_alloc_time)
		ktime_get_real_ts64(&tv0);

	mutex_lock(&handle->handle_lock);
	if (handle->tee_map == TEE_MAP_NON)
		handle->tee_map = TEE_MAP_DELAY;
	else
		goto out;

	if (handle->addr > 0)
		goto out;

	__mtk_iommu_get_space_tag(handle->buf_tag_id << BUFTAG_SHIFT,
				&space_tag, data);

	ret = mtk_iommu_tee_map(space_tag, handle->sgt,
			      &addr, handle->iova2x, handle->buf_tag);
	if (ret) {
		pr_err("%s: bugtag [%s] __do_TEEMap failed with %d\n",
			__func__, handle->buf_tag, ret);
		handle->tee_map = TEE_MAP_NON;
		goto out;
	}

	handle->is_iova = addr & IOVA_START_ADDR;
	handle->addr = addr;

	if (mma_callback_ops->map_iova)
		mma_callback_ops->map_iova(handle->b.db_ion, handle->serial, addr);
out:
	if (data->record_alloc_time) {
		ktime_get_real_ts64(&tv1);
		maptime = (tv1.tv_sec - tv0.tv_sec) * USEC_PER_SEC +
				(tv1.tv_nsec - tv0.tv_nsec) / NSEC_PER_USEC;
		pr_err("%s  %d,addr =0x%llx,  maptime(us) =%d!\n",
			__func__, __LINE__, addr, maptime);
	}
	mutex_unlock(&handle->handle_lock);
}

static void __do_TEEMap(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct mtk_iommu_buf_handle *handle =
	    container_of(dwork, struct mtk_iommu_buf_handle, work);

	__do_TEEMap_internal(handle);
}

void *__mtk_iommu_vmap_internal(struct dma_buf *db)
{
	int ret = dma_buf_begin_cpu_access(db, DMA_BIDIRECTIONAL);

	if (ret < 0)
		return NULL;

	return dma_buf_vmap(db);
}

void __mtk_iommu_vunmap_internal(struct dma_buf *db, void *vaddr)
{
	dma_buf_vunmap(db, vaddr);
	dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
}

static inline void _update_memory_statistics(const char *buf_tag, unsigned long long sz,
				unsigned long attrs, bool add)
{
	struct mem_statistics *stat;
	struct mtk_dtv_iommu_data *data = NULL;
	int inc = 1;
	int ret;

	ret = get_iommu_data(&data);
	if (ret)
		return;
	
	stat = get_buf_tag_statistics(buf_tag);
	if (IS_ERR_OR_NULL(stat))
		return;

	if (strncmp(buf_tag, stat->buftag, strlen(stat->buftag)))
		return;

	if (add) {
		atomic64_add(sz, &stat->total_sz);
		if (attrs & IOMMUMD_FLAG_DMAZONE)
			atomic64_add(sz, &stat->dma_sz);
		else
			atomic64_add(sz, &stat->others_sz);

		atomic64_add(inc, &stat->num_entry);
	} else {
		atomic64_sub(sz, &stat->total_sz);
		if (attrs & IOMMUMD_FLAG_DMAZONE)
			atomic64_sub(sz, &stat->dma_sz);
		else
			atomic64_sub(sz, &stat->others_sz);

		atomic64_sub(inc, &stat->num_entry);
	}

	IOMMU_DEBUG(E_LOG_DEBUG, "%s, %s0x%llx, Total: 0x%llx (DMA: 0x%llx + Others: 0x%llx)\n",
		buf_tag, add ? "+" : "-", sz, atomic64_read(&stat->total_sz),
		atomic64_read(&stat->dma_sz), atomic64_read(&stat->others_sz));
}

void __free_internal(struct mtk_iommu_buf_handle *buf_handle)
{
	int i;
	struct mtk_iommu_range_t range_out;
	struct timespec64 tv0, tv1;
	unsigned int time;

	ktime_get_real_ts64(&tv0);
	if (!buf_handle)
		return;

	if (mma_callback_ops->free)
		mma_callback_ops->free(buf_handle->b.db_ion, buf_handle->serial, buf_handle->addr);

	if (buf_handle->kvaddr) {
		__mtk_iommu_vunmap_internal(buf_handle->b.db_ion,
						buf_handle->kvaddr);
		buf_handle->kvaddr = NULL;
	}

	// unauthorize
	if (buf_handle->is_secure && buf_handle->auth_count != 0) {
		mtkd_iommu_buffer_unauthorize(buf_handle->addr);
		buf_handle->auth_count = 0;
	}

	if (buf_handle->tee_map != TEE_MAP_DEFAULT) {
		buf_handle->tee_map = TEE_MAP_REMOVE;
		cancel_delayed_work_sync(&buf_handle->work);
	}
	mutex_lock(&buf_handle->handle_lock);
	if (buf_handle->global_name > 0) {
		mutex_lock(&(iommu_data->buf_lock));
		idr_remove(&iommu_data->global_name_idr,
				buf_handle->global_name);
		mutex_unlock(&(iommu_data->buf_lock));
		buf_handle->global_name = 0;
	}
	if (buf_handle->sg_map_count) {
		pr_err("%s  %d, addr=0x%llx,buftag=%s, sg_map_count=%d error\n",
		       __func__, __LINE__, buf_handle->addr,
		       buf_handle->buf_tag,
		       buf_handle->sg_map_count);
	}

	// unmap iova
	if (buf_handle->is_iova) {
		mtk_iommu_tee_unmap(MTK_IOMMU_ADDR_TYPE_IOVA,
				buf_handle->addr, &range_out);
		buf_handle->is_iova = false;
	}

	if (buf_handle->va2iova_pages) {
		if (buf_handle->num_pages) {
			for (i = 0; i < buf_handle->num_pages; i++)
				put_page(buf_handle->va2iova_pages[i]);
		}
		kfree(buf_handle->va2iova_pages);
		buf_handle->va2iova_pages = NULL;
		if (buf_handle->sgt) {
			sg_free_table(buf_handle->sgt);
			kfree(buf_handle->sgt);
			buf_handle->sgt = NULL;
		}
	}
	// release memory
	if (buf_handle->b.db_ion) {
		dma_buf_put(buf_handle->b.db_ion);
		buf_handle->b.db_ion = NULL;
	}
	mutex_unlock(&buf_handle->handle_lock);
	ktime_get_real_ts64(&tv1);
	time = (tv1.tv_sec - tv0.tv_sec) * USEC_PER_SEC +
			(tv1.tv_nsec - tv0.tv_nsec) / NSEC_PER_USEC;
	IOMMU_DEBUG(E_LOG_ALERT, "buf_tag=%s, size=0x%zx, iova=0x%llx, free time =%d\n",
			buf_handle->buf_tag, buf_handle->length,
			buf_handle->addr, time);

	_update_memory_statistics(buf_handle->buf_tag, buf_handle->length,
			buf_handle->attrs, false);
}

static inline void ___show_info_by_buftag(const char *buf_tag,
	struct mtk_dtv_iommu_data *data)
{
	struct mtk_iommu_buf_handle *handle;

	if (!buf_tag || !data)
		return;

	if (!strncmp(buf_tag, "mali_gop_dma", strlen(buf_tag)))
		return;

	mutex_lock(&(data->buf_lock));
	list_for_each_entry(handle, &(data->buf_list_head), buf_list_node) {
		if (!strncmp(handle->buf_tag, buf_tag, MAX_NAME_SIZE))
			pr_err("iova: %llx, size: %x, pid: %d, comm: %s",
				handle->addr, (uint32_t)handle->length,
				handle->pid, handle->comm);
	}
	mutex_unlock(&(data->buf_lock));
}

static int __mtk_iommu_maxima_check(struct device *dev,
		const char *buf_tag, u32 size, u64 max)
{
	u64 total = 0;
	struct mem_statistics *stat;
	struct mtk_iommu_buf_handle *buf_handle = NULL;
	struct mtk_dtv_iommu_data *data = NULL;
	int ret;

	ret = get_iommu_data(&data);
	if (ret)
		return ret;

	stat = get_buf_tag_statistics(buf_tag);
	if (IS_ERR_OR_NULL(stat))
		return ret;

	if (strncmp(buf_tag, stat->buftag, strlen(stat->buftag)))
		return ret;
	total += atomic64_read(&stat->total_sz);
	if (total > max) {
		dev_err(dev, "%s: OVERSIZE, buf_tag [%s] total=0x%llx > max=0x%llx\n",
			__func__, buf_tag, total, max);
		___show_info_by_buftag(buf_tag, data);
		return -ENOMEM;
	}

	return 0;
}

void *__mtk_iommu_alloc_attrs(struct device *dev, size_t size,
		dma_addr_t *dma_addr, gfp_t flag, unsigned long attrs)
{
	int i = 0, ret = -1;
	int wide = 0, narrow = 0;;
	unsigned int ion_flag = 0;
	unsigned int heap_mask = 0;
	unsigned int map_time = 0;
	bool secure = false;
	char *space_tag = NULL;
	dma_addr_t addr = 0;
	struct sg_table *sgt = NULL;
	struct sg_table *internal_sgt = NULL;
	struct scatterlist *sg = NULL;
	struct timespec64 tv0 = {0};
	struct timespec64 tv1 = {0};
	struct timespec64 tv2 = {0};
	struct timespec64 tv3 = {0};
	struct ion_buffer *buffer;
	struct dma_buf *db_ion = NULL;
	struct mtk_iommu_buf_handle *buf_handle = NULL;
	struct mtk_dtv_iommu_data *data = NULL;
	struct buf_tag_info tag_info = {0};
	void *vaddr = NULL;
	bool doubleiova = false;
	char timebuf[KTIME_FORMAT_LEN];
	struct timespec64 mono_time = {0};
	struct dma_buf_attachment *attach = NULL;
	struct mtk_iommu_range_t range_out;

	if (!dma_addr)
		return NULL;

	*dma_addr = DMA_MAPPING_ERROR;

	tag_info.id = (attrs & BUFTAGMASK) >> BUFTAG_SHIFT;

	// 1. get iommu data
	ret = get_iommu_data(&data);
	if (ret)
		return NULL;

	IOMMU_DEBUG(E_LOG_CRIT, "size=0x%zx, buf_tag_id=%d, attrs=0x%lx\n",
			size, tag_info.id, attrs);
	if (data->calltrace_enable == 1)
		dump_stack();

	ktime_get_real_ts64(&tv0);

	// 2. get buf_tag info from dts
	ret = mtk_iommu_get_buftag_info(&tag_info);
	if (ret) {
		dev_err(dev, "%s(%d): get bufinfo failed\n",
			__func__, __LINE__);
		return NULL;
	}

	// 3. max size check
	ret = __mtk_iommu_maxima_check(dev, tag_info.name,
				size, tag_info.maxsize);
	if (ret) {
		dev_err(dev, "%s(%d): buftag %s overflow\n",
			__func__, __LINE__, tag_info.name);
		return NULL;
	}

	// 4.get ion flag
	ret = mtk_iommu_get_id_flag(tag_info.heap_type,
				tag_info.miu, tag_info.zone,
				secure, &heap_mask, &ion_flag,
				size, tag_info.name, attrs);
	if (ret) {
		dev_err(dev, "%s(%d): get id flag failed\n",
			__func__, __LINE__);
		return NULL;
	}

	// 5. ion alloc
ALLOCATE_AGAIN:
	ret = mtk_iommu_ion_alloc(size, heap_mask, ion_flag, &db_ion);
	if (ret < 0) {
		if (ion_flag & IOMMUMD_FLAG_DMAZONE) {
			ion_flag &= ~IOMMUMD_FLAG_DMAZONE;
			dev_err(dev,
				"%s  %d,tag:%s,size=0x%zx DMAZONE alloc Failed\n",
				__func__, __LINE__, tag_info.name, size);
			dev_err(dev, "%s  %d,fallback to high zone\n",
				__func__, __LINE__);
			goto ALLOCATE_AGAIN;
		} else {
			dev_err(dev, "%s  %d,ion allocate Failed!\n",
				__func__, __LINE__);
			return NULL;
		}
	}

	if (IS_ERR(db_ion))
		return NULL;

	buffer = (struct ion_buffer *)db_ion->priv;
	if (IS_ERR_OR_NULL(buffer)) {
		dma_buf_put(db_ion);
		return NULL;
	}

	sgt = buffer->sg_table;
	if (IS_ERR_OR_NULL(sgt)) {
		dma_buf_put(db_ion);
		return NULL;
	}

	__mtk_iommu_get_space_tag(attrs, &space_tag, data);
	ktime_get_real_ts64(&tv2);

	if (tag_info.heap_type == HEAP_TYPE_IOMMU
		   || tag_info.heap_type == HEAP_TYPE_CMA_IOMMU
		   || tag_info.heap_type == HEAP_TYPE_CARVEOUT) {
		if (attrs & IOMMUMD_FLAG_2XIOVA)
			doubleiova = true;
		if (!(attrs & IOMMUMD_FLAG_NOMAPIOVA)) {
			ret = mtk_iommu_tee_map(space_tag,
					sgt, &addr, doubleiova,
					tag_info.name);
			if (ret) {
				dev_err(dev, "tee map failed!\n");
				dma_buf_put(db_ion);
				return NULL;
			}
		}
		tag_info.miu = 0;
	} else {
		dev_err(dev, "invalid heap type\n");
		dma_buf_put(db_ion);
		return NULL;
	}

	ktime_get_real_ts64(&tv3);
	map_time = (tv3.tv_sec - tv2.tv_sec) * USEC_PER_SEC +
				(tv3.tv_nsec - tv2.tv_nsec) / NSEC_PER_USEC;

	buf_handle = __mtk_iommu_create_buf_handle(size, tag_info.name);
	if (!buf_handle) {
		dev_err(dev, "cannot create buf_handle\n");
		if (!(attrs & IOMMUMD_FLAG_NOMAPIOVA))
			mtk_iommu_tee_unmap(MTK_IOMMU_ADDR_TYPE_IOVA, addr, &range_out);
		dma_buf_put(db_ion);
		return NULL;
	}

	buf_handle->b.db_ion = db_ion;
	buf_handle->addr = addr;
	buf_handle->is_iova = addr & IOVA_START_ADDR;
	buf_handle->miu_select = tag_info.miu;
	buf_handle->auth_count = 0;
	buf_handle->sgt = sgt;
	strncpy(buf_handle->buf_tag, tag_info.name, MAX_NAME_SIZE);
	buf_handle->buf_tag[MAX_NAME_SIZE - 1] = '\0';
	buf_handle->buf_tag_id = tag_info.id;
	buf_handle->iova2x = doubleiova;
	buf_handle->map_time = map_time;
	buf_handle->is_secure = false;
	buf_handle->is_freeing = false;
	buf_handle->attrs = attrs;

	*dma_addr = addr;
	for_each_sg(sgt->sgl, sg, sgt->orig_nents, i) {
		sg->dma_address = addr + sg->offset;
		sg->dma_length = sg->length;
		addr += sg->length;
	}
	// remap to cpu addr
	if ((attrs & DMA_ATTR_NO_KERNEL_MAPPING))
		vaddr = 0;
	else {
		vaddr = __mtk_iommu_vmap_internal(buf_handle->b.db_ion);
		if (!vaddr)
			dev_err(dev, "%s %d,kmap fail,buftag=%s,addr=0x%llx,size=0x%zx\n",
				__func__, __LINE__, tag_info.name,
				buf_handle->addr, size);
		else
			buf_handle->kvaddr = vaddr;
	}

	mutex_lock(&(data->buf_lock));
	list_add(&buf_handle->buf_list_node, &data->buf_list_head);
	mutex_unlock(&(data->buf_lock));

	if (mma_callback_ops->alloc)
		mma_callback_ops->alloc(buf_handle->b.db_ion, buf_handle->serial);

	if (attrs & IOMMUMD_FLAG_NOMAPIOVA) {
		mutex_lock(&(data->buf_lock));
		ret = idr_alloc(&data->global_name_idr,
				buf_handle, 1, 0, GFP_KERNEL);
		mutex_unlock(&(data->buf_lock));
		if (ret < 0) {
			dma_buf_put(db_ion);
			kfree(buf_handle);
			dev_err(dev, "%s(%d): alloc global_name failed\n", __func__, __LINE__);
			return NULL;
		}

		buf_handle->tee_map = TEE_MAP_NON;
		INIT_DELAYED_WORK(&buf_handle->work, __do_TEEMap);
		if (data->wq)
			queue_delayed_work(data->wq, &buf_handle->work,
				msecs_to_jiffies(2));
		/*
		 *when NOMAPIOVA set, assign global_name to dma_addr,
		 *get buffer handle by global_name when buffer free
		 *only for ION allocate
		 */
		*dma_addr = ret;
		buf_handle->global_name = ret;
	} else
		buf_handle->tee_map = TEE_MAP_DEFAULT;


	ktime_get_real_ts64(&tv1);
	ktime_get_ts64(&mono_time);
	buf_handle->alloc_time = (tv1.tv_sec - tv0.tv_sec) * USEC_PER_SEC +
					(tv1.tv_nsec - tv0.tv_nsec) / NSEC_PER_USEC;

	// add alloc timestamp
	scnprintf(timebuf, KTIME_FORMAT_LEN, "%5lu.%06lu",
			(unsigned long)mono_time.tv_sec,
			mono_time.tv_nsec / NSEC_PER_USEC);
	strncpy(buf_handle->buf_alloc_time, timebuf, KTIME_FORMAT_LEN);
	buf_handle->buf_alloc_time[KTIME_FORMAT_LEN - 1] = '\0';

	if (data->record_alloc_time) {
		pr_err("%s,buftag=%s,addr=0x%llx,size=0x%zx,alloc time=%d us, map time=%d us\n",
		     __func__, tag_info.name, buf_handle->addr, size,
		     buf_handle->alloc_time, buf_handle->map_time);
	}
	if (!(attrs & DMA_ATTR_WRITE_COMBINE)) {
		attach = dma_buf_attach(db_ion, dev);
		if (IS_ERR_OR_NULL(attach)) {
			mutex_lock(&(data->buf_lock));
			list_del(&buf_handle->buf_list_node);
			mutex_unlock(&(data->buf_lock));
			if (buf_handle->is_iova)
				mtk_iommu_tee_unmap(MTK_IOMMU_ADDR_TYPE_IOVA, addr, &range_out);
			dma_buf_put(db_ion);
			*dma_addr = DMA_MAPPING_ERROR;
			dev_err(dev, "[%s][%d] attach failed\n", __func__, __LINE__);
			return NULL;
		}

		internal_sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
		if (IS_ERR_OR_NULL(internal_sgt)) {
			dma_buf_detach(db_ion, attach);
			mutex_lock(&(data->buf_lock));
			list_del(&buf_handle->buf_list_node);
			mutex_unlock(&(data->buf_lock));
			if (buf_handle->is_iova)
				mtk_iommu_tee_unmap(MTK_IOMMU_ADDR_TYPE_IOVA, addr, &range_out);
			dma_buf_put(db_ion);
			*dma_addr = DMA_MAPPING_ERROR;
			dev_err(dev, "[%s][%d] internal_sgt is null\n", __func__, __LINE__);
			return NULL;
		}

		mutex_lock(&buf_handle->handle_lock);
		buf_handle->attach = attach;
		buf_handle->internal_sgt = internal_sgt;
		mutex_unlock(&buf_handle->handle_lock);
	}


	if (mma_callback_ops->map_iova) {
		mma_callback_ops->map_iova(buf_handle->b.db_ion,
				buf_handle->serial, buf_handle->addr);
	}

	IOMMU_DEBUG(E_LOG_ALERT,
	"id=%d,buf_tag=%s,addr=0x%llx,size=0x%zx,vaddr=0x%lx,global_name=0x%x,atime=%d us,mtime=%d us (at %s), ino = %ld\n",
	     buf_handle->buf_tag_id, buf_handle->buf_tag,
	     buf_handle->addr, size, (unsigned long)vaddr,
	     buf_handle->global_name, buf_handle->alloc_time,
	     buf_handle->map_time, buf_handle->buf_alloc_time,
	     file_inode((buf_handle->b.db_ion)->file)->i_ino);

	_update_memory_statistics(buf_handle->buf_tag, buf_handle->length,
			buf_handle->attrs, true);

	return vaddr;
}

void __mtk_iommu_free_attrs(struct device *dev,
		size_t size, void *cpu_addr,
		dma_addr_t handle, unsigned long attrs)
{
	struct mtk_iommu_buf_handle *buf_handle = NULL;
	struct mtk_dtv_iommu_data *data = NULL;
	bool is_freeing = false;
	int ret;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	IOMMU_DEBUG(E_LOG_CRIT, "size=0x%zx, va=0x%lx, iova=0x%llx, attrs=0x%lx\n",
			size, (unsigned long)cpu_addr,
			(unsigned long long)handle, attrs);

	if (handle) {
		if (handle >= IOVA_START_ADDR)
			buf_handle = __mtk_iommu_find_buf_handle(handle, data);

		if (!buf_handle && attrs & IOMMUMD_FLAG_NOMAPIOVA)
			buf_handle = __mtk_iommu_find_name(handle, data);

		if (!buf_handle) {
			dev_err(dev, "%s(%d): input iova: 0x%lx error\n",
				__func__, __LINE__, (unsigned long)handle);
			return;
		}
	} else if (cpu_addr)
		buf_handle = __mtk_iommu_find_kva(cpu_addr, data);
	else {
		dev_err(dev, "%s(%d): input error\n", __func__, __LINE__);
		return;
	}
	if (!buf_handle) {
		dev_err(dev, "%s %d, can't find buffer,addr=0x%llx,kva=0x%lx!\n",
			__func__, __LINE__, handle, (unsigned long)cpu_addr);
		return;
	}


	if (buf_handle->attach && buf_handle->internal_sgt) {
		dma_buf_unmap_attachment(buf_handle->attach,
				buf_handle->internal_sgt, DMA_BIDIRECTIONAL);
		buf_handle->internal_sgt = NULL;
	}

	if (buf_handle->attach) {
		dma_buf_detach(buf_handle->b.db_ion, buf_handle->attach);
		buf_handle->attach = NULL;
	}

	mutex_lock(&(data->buf_lock));
	if (buf_handle && !buf_handle->is_freeing) {
		list_del(&buf_handle->buf_list_node);
		is_freeing = true;
		buf_handle->is_freeing = true;
	}
	mutex_unlock(&(data->buf_lock));

	if (!is_freeing)
		goto out;

	__free_internal(buf_handle);

	kfree(buf_handle);
out:
	return;
}

static int __mtk_iommu_mmap_attrs(struct device *dev,
		struct vm_area_struct *vma,
		void *cpu_addr, dma_addr_t dma_addr, size_t size,
		unsigned long attrs)
{
	int ret;
	struct mtk_dtv_iommu_data *data = NULL;
	struct mtk_iommu_buf_handle *buf_handle = NULL;

	ret = get_iommu_data(&data);
	if (ret)
		return ret;

	if (dma_addr)
		buf_handle = __mtk_iommu_find_buf_handle(dma_addr, data);
	else if (cpu_addr)
		buf_handle = __mtk_iommu_find_kva(cpu_addr, data);
	else {
		pr_err("%s(%d): invalid dma_addr or cpu_addr\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	if (!buf_handle) {
		pr_err("%s(%d): find buf handle failed!\n", __func__, __LINE__);
		return -EINVAL;
	}

	IOMMU_DEBUG(E_LOG_NOTICE, "cpu_addr=0x%llx, size=0x%zx, addr=0x%llx, va=0x%lx\n",
		    (unsigned long long)cpu_addr, size,
		    dma_addr, vma->vm_start);

	return dma_buf_mmap(buf_handle->b.db_ion, vma, vma->vm_pgoff);
}

static int __mtk_iommu_get_sgtable(struct device *dev, struct sg_table *sgt,
				   void *cpu_addr, dma_addr_t dma_addr,
				   size_t size, unsigned long attrs)
{
	int ret = -1, i;
	struct mtk_iommu_buf_handle *buf_handle = NULL;
	struct mtk_dtv_iommu_data *data = NULL;
	struct scatterlist *sg, *new_sg;

	if (!sgt)
		return -EINVAL;

	ret = get_iommu_data(&data);
	if (ret)
		return ret;

	if (dma_addr) {
		if (dma_addr >= IOVA_START_ADDR)
			buf_handle = __mtk_iommu_find_buf_handle(dma_addr,
							data);
		else if (attrs & IOMMUMD_FLAG_NOMAPIOVA)
			buf_handle = __mtk_iommu_find_name(dma_addr, data);
		else {
			pr_err("%s(%d): input iova error\n", __func__, __LINE__);
			return -1;
		}
	} else if (cpu_addr)
		buf_handle = __mtk_iommu_find_kva(cpu_addr, data);
	else {
		pr_err("%s(%d): input error\n", __func__, __LINE__);
		return -1;
	}

	if (!buf_handle) {
		pr_err("%s(%d): input error\n", __func__, __LINE__);
		return -1;
	}

	ret = sg_alloc_table(sgt, buf_handle->sgt->orig_nents, GFP_KERNEL);
	if (ret) {
		pr_err("%s(%d): sg_alloc_table fail\n", __func__, __LINE__);
		return -1;
	}

	new_sg = sgt->sgl;
	for_each_sg(buf_handle->sgt->sgl, sg, buf_handle->sgt->orig_nents, i) {
		memcpy(new_sg, sg, sizeof(*sg));
		new_sg->dma_address = 0;
		new_sg = sg_next(new_sg);
	}

	IOMMU_DEBUG(E_LOG_DEBUG, "size=0x%zx, va=0x%lx, iova=0x%llx, attrs=0x%lx\n",
			size, (unsigned long)cpu_addr,
			(unsigned long long)dma_addr, attrs);

	return 0;
}

static void __mtk_iommu_sync_single_for_cpu(struct device *dev,
					    dma_addr_t dev_addr, size_t size,
					    enum dma_data_direction dir)
{
	int ret = -1;
	struct mtk_iommu_buf_handle *buf_handle = NULL;
	struct mtk_dtv_iommu_data *data = NULL;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	buf_handle = __mtk_iommu_find_buf_handle(dev_addr, data);
	if (!buf_handle)
		return;
	dev->dma_pfn_offset = 0;
	dma_direct_sync_single_for_cpu(data->direct_dev, buf_handle->b.phys, size, dir);

	IOMMU_DEBUG(E_LOG_INFO, "dev_addr: 0x%llx, b.phys: 0x%llx, size: 0x%zx\n",
			dev_addr, (unsigned long long)buf_handle->b.phys, size);
}

static void __mtk_iommu_sync_single_for_device(struct device *dev,
					       dma_addr_t dev_addr, size_t size,
					       enum dma_data_direction dir)
{
	int ret = -1;
	struct mtk_iommu_buf_handle *buf_handle = NULL;
	struct mtk_dtv_iommu_data *data = NULL;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	buf_handle = __mtk_iommu_find_buf_handle(dev_addr, data);
	if (!buf_handle)
		return;

	dev->dma_pfn_offset = 0;
	dma_direct_sync_single_for_device(data->direct_dev, buf_handle->b.phys, size, dir);

	IOMMU_DEBUG(E_LOG_INFO, "dev_addr: 0x%llx, b.phys: 0x%llx, size: 0x%zx\n",
			dev_addr, (unsigned long long)buf_handle->b.phys, size);
}

static dma_addr_t __mtk_iommu_map_page(struct device *dev, struct page *page,
		unsigned long offset, size_t size,
		enum dma_data_direction dir, unsigned long attrs)
{
	int ret = 0;
	dma_addr_t addr = DMA_ERROR_CODE;

	struct mtk_dtv_iommu_data *data = NULL;
	struct mtk_iommu_buf_handle *buf_handle = NULL;

	ret = get_iommu_data(&data);
	if (ret)
		return DMA_ERROR_CODE;

	buf_handle = __mtk_iommu_create_buf_handle(size, "map_page");
	if (!buf_handle) {
		pr_err("%s(%d): create buf handle failed\n", __func__, __LINE__);
		return DMA_ERROR_CODE;
	}

	buf_handle->b.phys = (phys_addr_t) (__pfn_to_phys(page_to_pfn(page)));

	ret = mtk_iommu_tee_map_page(MTK_IOMMU_ADDR_TYPE_IOVA, NULL,
			page, size, offset, false, &addr);
	if (ret < 0) {
		dev_err(dev, "%s %d, tee map page fail!\n", __func__, __LINE__);
		return DMA_ERROR_CODE;
	}

	if ((attrs & DMA_ATTR_SKIP_CPU_SYNC) == 0)
		__mtk_iommu_sync_single_for_device(dev, addr, size, dir);

	buf_handle->addr = addr;
	mutex_lock(&(data->buf_lock));
	list_add(&buf_handle->buf_list_node, &data->buf_list_head);
	mutex_unlock(&(data->buf_lock));

	IOMMU_DEBUG(E_LOG_NOTICE, "size=0x%zx, addr=0x%llx\n", size, addr);
	return addr;
}

static void __mtk_iommu_unmap_page(struct device *dev, dma_addr_t dev_addr,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	struct mtk_iommu_range_t range_out;
	struct mtk_iommu_buf_handle *buf_handle = NULL;

	buf_handle = __mtk_iommu_find_buf_handle(dev_addr, iommu_data);
	if (!buf_handle)
		return;
	if ((attrs & DMA_ATTR_SKIP_CPU_SYNC) == 0)
		__mtk_iommu_sync_single_for_cpu(dev, dev_addr, size, dir);

	mtk_iommu_tee_unmap(MTK_IOMMU_ADDR_TYPE_IOVA, dev_addr, &range_out);

	mutex_lock(&(iommu_data->buf_lock));
	list_del(&buf_handle->buf_list_node);
	mutex_unlock(&(iommu_data->buf_lock));

	kfree(buf_handle);
	IOMMU_DEBUG(E_LOG_NOTICE, "size=0x%zx, addr=0x%llx\n", size, dev_addr);
}

static void __mtk_iommu_sync_sg_for_device(struct device *dev,
		struct scatterlist *sgl, int nelems,
		enum dma_data_direction dir)
{
	int i = 0, ret;
	struct scatterlist *sg = NULL;
	struct mtk_dtv_iommu_data *data = NULL;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	dev->dma_pfn_offset = 0;
	for_each_sg(sgl, sg, nelems, i)
		dma_direct_sync_single_for_device(data->direct_dev,
			sg_phys(sg), sg->length, dir);
}

static void __mtk_iommu_sync_sg_for_cpu(struct device *dev,
		struct scatterlist *sgl, int nelems,
		enum dma_data_direction dir)
{
	int i = 0, ret;
	struct scatterlist *sg = NULL;
	struct mtk_dtv_iommu_data *data = NULL;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	dev->dma_pfn_offset = 0;
	for_each_sg(sgl, sg, nelems, i)
		dma_direct_sync_single_for_cpu(data->direct_dev,
			sg_phys(sg), sg->length, dir);

}

static int __mtk_iommu_map_sg_attrs(struct device *dev, struct scatterlist *sgl,
		int nelems, enum dma_data_direction dir, unsigned long attrs)
{
	int i = 0, ret = -1;
	struct scatterlist *s = NULL;
	struct scatterlist *new_sg;
	struct sg_table *sgt;
	dma_addr_t addr = DMA_ERROR_CODE;
	struct mtk_iommu_buf_handle *buf_handle = NULL;
	int found = 0, tee_map;
	size_t total_size = 0;
	struct mtk_dtv_iommu_data *data = NULL;

	if (!sgl || nelems <= 0) {
		dev_err(dev, "%s %d, invalid input!\n", __func__, __LINE__);
		return 0;
	}

	ret = get_iommu_data(&data);
	if (ret)
		return ret;

	mutex_lock(&(data->mapsg_lock));
	mutex_lock(&(data->buf_lock));
	list_for_each_entry(buf_handle, &(data->buf_list_head),
				buf_list_node) {
		// sg_table is NULL which means this buf_handle is freeding
		if (!buf_handle->sgt)
			continue;

		if (buf_handle->sgt->orig_nents == nelems) {
			new_sg = buf_handle->sgt->sgl;
			//just check first sg only
			if (sgl->page_link == new_sg->page_link
					&& sgl->length == new_sg->length)
				found = 1;
			else
				continue;

			if (found == 1) {
				mutex_unlock(&(data->buf_lock));
				mutex_lock(&buf_handle->handle_lock);
				tee_map = buf_handle->tee_map;
				mutex_unlock(&buf_handle->handle_lock);
				if (tee_map == TEE_MAP_NON)
					__do_TEEMap_internal(buf_handle);
				mutex_lock(&buf_handle->handle_lock);
				addr = buf_handle->addr;
				buf_handle->sg_map_count++;
				mutex_unlock(&buf_handle->handle_lock);
				mutex_unlock(&(data->mapsg_lock));
				goto out;
			}
		}
	}
	mutex_unlock(&(data->buf_lock));

	buf_handle = __mtk_iommu_create_buf_handle(0, "map_sg");
	if (!buf_handle) {
		mutex_unlock(&(data->mapsg_lock));
		return 0;
	}

	sgt = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!sgt) {
		mutex_unlock(&(data->mapsg_lock));
		kfree(buf_handle);
		return 0;
	}
	ret = sg_alloc_table(sgt, nelems, GFP_KERNEL);
	if (ret) {
		mutex_unlock(&(data->mapsg_lock));
		kfree(buf_handle);
		kfree(sgt);
		return 0;
	}
	new_sg = sgt->sgl;
	for_each_sg(sgl, s, nelems, i) {
		memcpy(new_sg, s, sizeof(*s));
		new_sg->dma_address = 0;
		new_sg = sg_next(new_sg);
	}

	ret = mtk_iommu_tee_map(NULL, sgt, &addr, false, buf_handle->buf_tag);
	if (ret) {
		mutex_unlock(&(data->mapsg_lock));
		dev_err(dev, "%s %d, tee map sg fail!\n", __func__, __LINE__);
		kfree(buf_handle);
		sg_free_table(sgt);
		kfree(sgt);
		return 0;
	}
	buf_handle->addr = addr;
	buf_handle->sgt = sgt;

	mutex_lock(&(data->buf_lock));
	list_add(&buf_handle->buf_list_node, &data->buf_list_head);
	mutex_unlock(&(data->buf_lock));

	mutex_unlock(&(data->mapsg_lock));
	if ((attrs & DMA_ATTR_SKIP_CPU_SYNC) == 0)
		__mtk_iommu_sync_sg_for_device(dev, sgl, nelems, dir);

	if (mma_callback_ops->map_iova) {
		mma_callback_ops->map_iova(buf_handle->b.db_ion,
				buf_handle->serial, buf_handle->addr);
	}
out:
	for_each_sg(sgl, s, nelems, i) {
		s->dma_address = addr + s->offset;
		s->dma_length = s->length;
		addr += s->length;
		total_size += s->length;
	}
	if (found == 0) {
		buf_handle->length = total_size;
		_update_memory_statistics(buf_handle->buf_tag, buf_handle->length,
				buf_handle->attrs, true);
	}
	IOMMU_DEBUG(E_LOG_NOTICE, "size=0x%zx, addr=0x%llx, found=%d\n",
			buf_handle->length, buf_handle->addr, found);

	return nelems;
}

static void __mtk_iommu_unmap_sg_attrs(struct device *dev,
		struct scatterlist *sgl, int nelems,
		enum dma_data_direction dir, unsigned long attrs)
{
	struct mtk_iommu_range_t range_out;
	dma_addr_t addr = sg_dma_address(sgl) & PAGE_MASK;
	struct mtk_iommu_buf_handle *buf_handle = NULL;
	struct mtk_dtv_iommu_data *data = NULL;
	int ret;

	ret = get_iommu_data(&data);
	if (ret)
		return;

	mutex_lock(&(data->mapsg_lock));
	buf_handle = __mtk_iommu_find_buf_handle(addr, data);
	if (!buf_handle) {
		mutex_unlock(&(data->mapsg_lock));
		dev_err(dev, "%s %d,addr=0x%llx, get buf fail!\n",
				__func__, __LINE__, addr);
		return;
	}
	IOMMU_DEBUG(E_LOG_NOTICE, "size=0x%zx, addr=0x%llx,sg_map_count=%d\n",
			buf_handle->length, buf_handle->addr,
			buf_handle->sg_map_count);
	mutex_lock(&buf_handle->handle_lock);
	if (buf_handle->sg_map_count) {
		buf_handle->sg_map_count--;
		mutex_unlock(&buf_handle->handle_lock);
		mutex_unlock(&(data->mapsg_lock));
		return;
	}
	mutex_unlock(&buf_handle->handle_lock);

	mutex_lock(&(data->buf_lock));
	list_del(&buf_handle->buf_list_node);
	mutex_unlock(&(data->buf_lock));

	mutex_unlock(&(data->mapsg_lock));
	if ((attrs & DMA_ATTR_SKIP_CPU_SYNC) == 0)
		__mtk_iommu_sync_sg_for_cpu(dev, sgl, nelems, dir);

	mtk_iommu_tee_unmap(MTK_IOMMU_ADDR_TYPE_IOVA, addr, &range_out);

	_update_memory_statistics(buf_handle->buf_tag, buf_handle->length,
			buf_handle->attrs, false);
	sg_free_table(buf_handle->sgt);
	kfree(buf_handle->sgt);
	kfree(buf_handle);
}

int __mtk_iommu_dma_supported(struct device *dev, u64 mask)
{
	int ret;
	struct mtk_dtv_iommu_data *data = NULL;

	ret = get_iommu_data(&data);
	if (ret)
		return 0;

	return data->iommu_status;
}

static const struct dma_map_ops mtk_iommu_dma_ops = {
	.alloc = __mtk_iommu_alloc_attrs,
	.free = __mtk_iommu_free_attrs,
	.mmap = __mtk_iommu_mmap_attrs,
	.get_sgtable = __mtk_iommu_get_sgtable,
	.map_page = __mtk_iommu_map_page,
	.unmap_page = __mtk_iommu_unmap_page,
	.map_sg = __mtk_iommu_map_sg_attrs,
	.unmap_sg = __mtk_iommu_unmap_sg_attrs,
	.sync_single_for_cpu = __mtk_iommu_sync_single_for_cpu,
	.sync_single_for_device = __mtk_iommu_sync_single_for_device,
	.sync_sg_for_cpu = __mtk_iommu_sync_sg_for_cpu,
	.sync_sg_for_device = __mtk_iommu_sync_sg_for_device,
	.dma_supported = __mtk_iommu_dma_supported,
};

static struct iommu_domain *mtk_iommu_dtv_domain_alloc(unsigned int type)
{
	struct iommu_domain *domain = NULL;

	if (type != IOMMU_DOMAIN_UNMANAGED)
		return NULL;

	domain = kzalloc(sizeof(*domain), GFP_KERNEL);
	if (!domain)
		return NULL;
	return domain;
}

static void mtk_iommu_dtv_domain_free(struct iommu_domain *domain)
{
	kfree(domain);
	domain = NULL;
}

static int mtk_iommu_dtv_add_device(struct device *dev)
{
	struct iommu_group *group = NULL;
	struct mtk_dtv_iommu_data *data;

	if (!dev->archdata.iommu)
		return -ENODEV;	/* Not a iommu client device */

	data = dev->archdata.iommu;

	iommu_device_link(&data->iommu, dev);
	group = iommu_group_get_for_dev(dev);
	if (IS_ERR(group)) {
		dev->dma_ops = NULL;
		*dev->dma_mask = DMA_BIT_MASK(32);
		iommu_fwspec_free(dev);
		return PTR_ERR(group);
	}
	*dev->dma_mask = DMA_BIT_MASK(IOMMU_DMA_MASK);
	dev->dma_ops = &mtk_iommu_dma_ops;

	iommu_group_put(group);
	return 0;
}

static void mtk_iommu_dtv_remove_device(struct device *dev)
{
	struct mtk_dtv_iommu_data *data;

	if (!dev->archdata.iommu)
		return;

	data = dev->archdata.iommu;
	iommu_device_unlink(&data->iommu, dev);
	iommu_group_remove_device(dev);
	iommu_fwspec_free(dev);
}

static struct iommu_domain *__mtk_iommu_domain_alloc(struct bus_type *bus,
		unsigned int type)
{
	struct iommu_domain *domain = NULL;

	if (bus == NULL || bus->iommu_ops == NULL)
		return NULL;

	domain = bus->iommu_ops->domain_alloc(type);
	if (!domain)
		return NULL;

	domain->ops = bus->iommu_ops;
	domain->type = type;
	domain->pgsize_bitmap = bus->iommu_ops->pgsize_bitmap;

	return domain;
}

static struct iommu_group *mtk_iommu_dtv_device_group(struct device *dev)
{
	if (!iommu_data)
		return ERR_PTR(-ENODEV);

	iommu_group_ref_get(iommu_data->group);
	if (!iommu_data->group->default_domain) {
		iommu_data->group->default_domain = __mtk_iommu_domain_alloc(dev->bus,
					IOMMU_DOMAIN_UNMANAGED);
		iommu_data->group->domain = iommu_data->group->default_domain;
		if (!iommu_data->group->default_domain)
			dev_err(dev, "invalid default_domain property for IOMMU\n");
	}

	return iommu_data->group;
}

static int mtk_iommu_dtv_of_xlate(struct device *dev,
		struct of_phandle_args *args)
{

	struct platform_device *pdev = NULL;

	if (args->args_count != 1) {
		dev_err(dev, "invalid #iommu-cells(%d) property for IOMMU\n",
				args->args_count);
		return -EINVAL;
	}

	if (!dev->archdata.iommu) {
		/* Get the iommu device */
		pdev = of_find_device_by_node(args->np);
		if (WARN_ON(!pdev))
			return -EINVAL;

		dev->archdata.iommu = platform_get_drvdata(pdev);
	}
	if (dev->dma_mask == &dev->coherent_dma_mask) {
		dev->dma_mask = devm_kzalloc(dev,
					sizeof(*dev->dma_mask), GFP_KERNEL);
		if (!dev->dma_mask)
			return -ENOMEM;
	}
	*dev->dma_mask = DMA_BIT_MASK(34);
	dev->dma_ops = &mtk_iommu_dma_ops;

	return iommu_fwspec_add_ids(dev, args->args, 1);
}

static int mtk_iommu_dtv_attach_device(struct iommu_domain *domain,
		struct device *dev)
{
	return 0;
}

static void mtk_iommu_dtv_detach_device(struct iommu_domain *domain,
		struct device *dev)
{
}

static int __mtk_set_mpu_area(void)
{
	int ret;
	uint32_t count = 0;
	struct mtk_iommu_range_t mpu_range[MAX_MPU_NR] = { };

	ret = mtk_iommu_get_memoryinfo(
			&mpu_range[0].start, &mpu_range[0].size,
			&mpu_range[1].start, &mpu_range[1].size);
	if (ret) {
		pr_err("IOMMU get LX fail!\n");
		return 0;
	}
	if (!mpu_range[0].size) {
		pr_err("IOMMU mpu area size = 0!error\n");
		return 0;
	}
	if (mpu_range[0].size > SZ_4G || mpu_range[1].size > SZ_4G) {
		pr_err("IOMMU mpu area size over 4GB! error!\n");
		return 0;
	}

	if (mpu_range[1].size)
		count = 2;
	else
		count = 1;
	ret = mtk_iommu_tee_set_mpu_area(MTK_IOMMU_ADDR_TYPE_IOVA,
			mpu_range, count);
	if (ret == 0)
		return 1;

	pr_err("IOMMU set mpu area fail!!!!!\n");
	return 0;
}

static void __mtk_iommu_reserved_from_dtb(void)
{
	int count = 0, ret;
	struct list_head head;
	struct list_head *list, *tmp;
	struct reserved_info *info;
	u64 addr;

	INIT_LIST_HEAD(&head);
	count = mtk_iommu_get_reserved(&head);
	if (count <= 0)
		return;

	list_for_each_safe(list, tmp, &head) {
		list_del_init(list);
		info = container_of(list, struct reserved_info, list);
		ret = mtkd_iommu_reserve_iova(info->reservation.space_tag,
				info->reservation.buf_tag_array,
			    &addr, info->reservation.size,
			    info->reservation.buf_tag_num);
		if (ret)
			pr_err("%s,%s fail\n", __func__,
					info->reservation.space_tag);
		kfree(info);
	}
}

static void __mtk_iommu_tee_init(void)
{
	if (!iommu_data) {
		pr_err("%s: iommu data not found\n", __func__);
		return;
	}

	mtk_iommu_tee_open_session();
	mtk_iommu_tee_init();
	iommu_data->iommu_status = __mtk_set_mpu_area();
	mtk_iommu_optee_ta_store_buf_tags();
	__mtk_iommu_reserved_from_dtb();
}
#if IS_BUILTIN(CONFIG_MTK_IOMMU_DTV)
late_initcall(__mtk_iommu_tee_init);
#endif
static struct iommu_ops mtk_dtv_iommu_ops = {
	.domain_alloc = mtk_iommu_dtv_domain_alloc,
	.domain_free = mtk_iommu_dtv_domain_free,
	.attach_dev = mtk_iommu_dtv_attach_device,
	.detach_dev = mtk_iommu_dtv_detach_device,
	.add_device = mtk_iommu_dtv_add_device,
	.remove_device = mtk_iommu_dtv_remove_device,
	.device_group = mtk_iommu_dtv_device_group,
	.of_xlate = mtk_iommu_dtv_of_xlate,
	.pgsize_bitmap = SZ_4K | SZ_8K | SZ_16K | SZ_64K | SZ_1M | SZ_2M,
};

static const struct of_device_id mtk_iommu_dtv_of_ids[] = {
	{.compatible = "mediatek,dtv-iommu",},
	{}
};

static int mtk_iommu_dtv_probe(struct platform_device *pdev)
{
	int ret, irq;
	struct mtk_dtv_iommu_data *data = NULL;
	struct device *dev = &pdev->dev;
	struct device *direct_dev = NULL;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	direct_dev = devm_kzalloc(dev, sizeof(*direct_dev), GFP_KERNEL);
	if (!direct_dev) {
		ret = -ENOMEM;
		goto free_data;
	}

	data->dev = dev;
	mutex_init(&(data->buf_lock));
	mutex_init(&(data->mapsg_lock));
	mutex_init(&data->physical_buf_lock);
	mutex_init(&data->share_lock);
	INIT_LIST_HEAD(&(data->buf_list_head));
	INIT_LIST_HEAD(&(data->dfree_list_head));
	INIT_LIST_HEAD(&(data->space_list_head));
	INIT_LIST_HEAD(&data->physical_buf_list_head);
	INIT_LIST_HEAD(&data->share_list_head);
	platform_set_drvdata(pdev, data);

	data->group = iommu_group_alloc();
	if (IS_ERR(data->group)) {
		ret = PTR_ERR(data->group);
		goto free_dev;
	}

	ret = iommu_device_sysfs_add(&data->iommu, dev, NULL, "mtk-dtv-iommu");
	if (ret)
		goto error;
	iommu_device_set_ops(&data->iommu, &mtk_dtv_iommu_ops);
	iommu_device_set_fwnode(&data->iommu, &pdev->dev.of_node->fwnode);
	ret = iommu_device_register(&data->iommu);
	if (ret)
		goto err_sysfs;
	if (!iommu_present(&platform_bus_type))
		bus_set_iommu(&platform_bus_type, &mtk_dtv_iommu_ops);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		dev_err(dev, "unable to get irq (%d)\n", irq);
	else
		data->irq = irq;
	mtk_iommu_sysfs_init(data->iommu.dev, data);
	data->wq = system_highpri_wq;
	data->log_level = 6;
	idr_init(&data->global_name_idr);

	/* register misc device */
	ret = iommud_misc_register(data);
	if (ret) {
		dev_err(dev, "register iommu misc failed with %d\n", ret);
		goto free;
	}

	data->asym_addr_start = ULONG_MAX;

	iommu_data = data;
#if IS_MODULE(CONFIG_MTK_IOMMU_DTV)
	__mtk_iommu_tee_init();
#endif

	memcpy(direct_dev, dev, sizeof(struct device));
	direct_dev->dma_ops = NULL;
	data->direct_dev = direct_dev;

	dev_info(dev, "driver probe done\n");

	/* set reg base */
	seal_register(dev);

	mma_callback_ops = devm_kzalloc(dev, sizeof(struct mma_callback), GFP_KERNEL);
	if (!mma_callback_ops) {
		ret = -ENOMEM;
		goto free_misc;
	}
	return 0;
free_misc:
	misc_deregister(&data->misc_dev);
free:
	mtk_iommu_sysfs_destroy(data->iommu.dev);
	iommu_device_unregister(&data->iommu);
err_sysfs:
	iommu_device_sysfs_remove(&data->iommu);
error:
	dev_err(dev, "mtk iommu driver probe fail\n");
	iommu_group_put(data->group);
free_dev:
	devm_kfree(dev, direct_dev);
free_data:
	devm_kfree(dev, data);
	return ret;
}

static int mtk_iommu_dtv_remove(struct platform_device *pdev)
{
	struct mtk_dtv_iommu_data *data = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	if (iommu_present(&platform_bus_type))
		bus_set_iommu(&platform_bus_type, NULL);

	mtk_iommu_tee_close_session();
	devm_kfree(dev, mma_callback_ops);
	misc_deregister(&data->misc_dev);
	mtk_iommu_sysfs_destroy(data->iommu.dev);
	iommu_device_unregister(&data->iommu);
	iommu_device_sysfs_remove(&data->iommu);
	if (data->group) {
		iommu_group_put(data->group);
		data->group = NULL;
	}
	devm_kfree(dev, data->direct_dev);
	devm_kfree(dev, data);

	return 0;
}

static struct platform_driver mtk_iommu_dtv_driver = {
	.probe = mtk_iommu_dtv_probe,
	.remove = mtk_iommu_dtv_remove,
	.driver = {
		.name = "mtk-iommu-dtv",
		.of_match_table = mtk_iommu_dtv_of_ids,
	}
};

static int __init mtk_iommu_dtv_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_iommu_dtv_driver);
	if (ret) {
		pr_err("Failed to register MTK IOMMU driver\n");
		return ret;
	}

	return mtk_ion_iommu_init();
}

static void __exit mtk_iommu_dtv_deinit(void)
{
	platform_driver_unregister(&mtk_iommu_dtv_driver);
	mtk_ion_iommu_deinit();
}

module_init(mtk_iommu_dtv_init);
module_exit(mtk_iommu_dtv_deinit);

MODULE_DESCRIPTION("MTK DTV IOMMU");
MODULE_AUTHOR("Benson Liang <benson.liang@mediatek.com>");
MODULE_LICENSE("GPL v2");
