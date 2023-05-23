// SPDX-License-Identifier: GPL-2.0

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>

#include "comm_debug.h"
#include "plat_iommu.h"
#include "comm_mem_mgt.h"

#define MDLA_CPU_IOVA_BASE              0x200000000ULL
#define MDLA_CPU_LX_BASE                0x20000000

static inline u32 MIU_CPU_PA_TO_MDLA_MVA(u32 addr)
{
	return (addr > MDLA_CPU_LX_BASE)?(addr - MDLA_CPU_LX_BASE):addr;
}

static LIST_HEAD(iommu_alloc_buf_list);
static DEFINE_MUTEX(iommu_alloc_buf_list_lock);

static long iommu_alloc_buf_register(int iommu_fd, void *virt
, u64 phy, u32 size, struct dma_buf *db, int mma_fd, int cacheable)
{
	struct mdla_iommu_buf *p;

	p = kzalloc(sizeof(struct mdla_iommu_buf), GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	INIT_LIST_HEAD(&p->node);
	p->buf_fd = iommu_fd;
	p->mma_fd = mma_fd;
	p->buf_virt = virt;
	p->buf_phy = phy;
	p->buf_size = size;
	p->pid = task_pid_nr(current);
	p->ppid = task_ppid_nr(current);
	p->dmabuf = db;
	p->cacheable = cacheable;
	mutex_lock(&iommu_alloc_buf_list_lock);
	list_add_tail(&p->node, &iommu_alloc_buf_list);
	mutex_unlock(&iommu_alloc_buf_list_lock);
	return 0;
}

static int iommu_alloc_buf_unregister(void *virt)
{
	struct mdla_iommu_buf *p;
	struct mdla_iommu_buf *found = NULL;

	mutex_lock(&iommu_alloc_buf_list_lock);
	list_for_each_entry(p, &iommu_alloc_buf_list, node) {
		if (p->buf_virt == virt) {
			found = p;
			break;
		}
	}
	if (found == NULL) {
		comm_drv_debug("iommu alloc buf unregister: va (%p) not found!\n", virt);
		mutex_unlock(&iommu_alloc_buf_list_lock);
		return -EINVAL;
	}
	list_del(&found->node);
	mutex_unlock(&iommu_alloc_buf_list_lock);
	kfree(found);
	return 0;
}

static int mdla_iova_buf_get_by_fd(int fd, struct mdla_iommu_buf *buf)
{
	struct mdla_iommu_buf *p;

	mutex_lock(&iommu_alloc_buf_list_lock);
	list_for_each_entry(p, &iommu_alloc_buf_list, node) {
		#ifdef MDLA_IOMMU_API_V2
		if ((fd == p->buf_fd) && (current->pid == p->pid)) {
		#else
		if ((fd == p->buf_fd) /*&& (current->pid == p->pid)*/) {
		#endif
			memcpy(buf, p, sizeof(struct mdla_iommu_buf));
			mutex_unlock(&iommu_alloc_buf_list_lock);
			return true;
		}
	}
	mutex_unlock(&iommu_alloc_buf_list_lock);
	return false;
}
#define BIT32_MAX 0xFFFFFFFF
static int mdla_iova_buf_get_by_phy(u64 phy, struct mdla_iommu_buf *buf)
{
	struct mdla_iommu_buf *p;
	u64 buf_start;
	u64 addr64 = (phy > BIT32_MAX)?phy:(phy + MDLA_CPU_IOVA_BASE);

	mutex_lock(&iommu_alloc_buf_list_lock);
	list_for_each_entry(p, &iommu_alloc_buf_list, node) {
		buf_start = p->buf_phy;
		if ((addr64 >= buf_start) && (addr64 < buf_start + p->buf_size)) {
			memcpy(buf, p, sizeof(struct mdla_iommu_buf));
			mutex_unlock(&iommu_alloc_buf_list_lock);
			return true;
		}
	}
	mutex_unlock(&iommu_alloc_buf_list_lock);
	return false;
}

int mdla_iova_buf_get_by_virt(void *virt, struct mdla_iommu_buf *buf)
{
	struct mdla_iommu_buf *p;
	void *kva_start;

	mutex_lock(&iommu_alloc_buf_list_lock);
	list_for_each_entry(p, &iommu_alloc_buf_list, node) {
		kva_start = p->buf_virt;
		if (virt >= kva_start && virt < kva_start + p->buf_size) {
			memcpy(buf, p, sizeof(struct mdla_iommu_buf));
			mutex_unlock(&iommu_alloc_buf_list_lock);
			return true;
		}
	}
	mutex_unlock(&iommu_alloc_buf_list_lock);
	return false;
}

int mdla_iova_phy_to_cacheable(u64 phy_addr)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_phy(phy_addr, &buf))
		return buf.cacheable;


	return NULL;
}

void *mdla_iova_phy_to_virt(u64 phy_addr)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_phy(phy_addr, &buf))
		return buf.buf_virt + (phy_addr - buf.buf_phy);


	return NULL;
}
//yuruei add
u64 mdla_iova_phy_to_u64pa(u64 phy_addr)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_phy(phy_addr, &buf)) {
		u64 addr64 = (phy_addr > BIT32_MAX)?phy_addr:(phy_addr + MDLA_CPU_IOVA_BASE);

		return buf.buf_phy + (addr64 - buf.buf_phy);
	}

	return NULL;
}

struct dma_buf *mdla_iova_phy_to_dmabuf(u64 phy_addr)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_phy(phy_addr, &buf))
		return buf.dmabuf;


	return NULL;
}

u32 mdla_iova_phy_to_size(u64 phy_addr)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_phy(phy_addr, &buf))
		return buf.buf_size - (u32)(phy_addr - buf.buf_phy);


	return 0;
}

struct dma_buf *mdla_iova_fd_to_dmabuf(int fd)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_fd(fd, &buf))
		return buf.dmabuf;

	return NULL;
}

u64 mdla_iova_fd_to_kva(int fd)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_fd(fd, &buf))
		return buf.buf_virt;

	return NULL;
}

u64 mdla_iova_fd_to_mva(int fd)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_fd(fd, &buf))
		return buf.buf_phy;

	return NULL;
}

//typedef int (*iova_foreach_cb)(u64 pa, u32 size
//, u64 kva, u64 fd,  struct dma_buf *db, u64 pid, u64 ppid);

//int mdla_iova_for_each_safe(iova_foreach_cb cb)
//{
//	struct mdla_iommu_buf *n, *p;
//
//	list_for_each_entry_safe(p, n, &iommu_alloc_buf_list, node) {
//		if (cb)
//			cb(p->buf_phy, p->buf_size
//, p->buf_virt, p->buf_fd, p->dmabuf, p->pid, p->ppid);
//	}
//	return 0;
//}

int mdla_iova_phy_to_all(u64 pa, u32 *size, u64 *kva, u64 *mva, u64 *pid, u64 *fd)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_phy(pa, &buf)) {
		*size = (u32)buf.buf_size;
		*kva = buf.buf_virt;
		*mva = (u64)MIU_CPU_PA_TO_MDLA_MVA(pa);
		*pid = (u64)buf.pid;
		*fd = (u64)buf.buf_fd;

		return 0;
	}

	return -1;
}

int mdla_iova_fd_to_all(u64 fd, u32 *size, u64 *kva, u64 *mva, u64 *pid, u64 *pa)
{
	struct mdla_iommu_buf buf;

	if (mdla_iova_buf_get_by_fd((int)fd, &buf)) {
		*size = (u32)buf.buf_size;
		*kva = (u64)buf.buf_virt;
		*mva = (u64)MIU_CPU_PA_TO_MDLA_MVA(buf.buf_phy);
		*pid = (u64)buf.pid;
		*pa = (u64)buf.buf_phy;

		return 0;
	}

	return -1;
}

static int global_fd;
#define MTK_IOMMU_DTV_IOVA_START (0x200000000ULL)  //8G
#define MTK_IOMMU_DTV_IOVA_END   (0x400000000ULL)  //16G
#define MDLA_BUF_TAG 40
#define BUF_TAG_SHIT 24
#define DMA_BIT_MASK_PARA 34
void *mdla_iommu_alloc(struct device *dev, u32 size, u64 *pa, int *fd, u32 cache)
{
	int ret;
	void *kva = NULL;
	struct sg_table *table = NULL;
	//__u64 pa = NULL;
	//int fd = NULL;
	dma_addr_t dma_addr = 0;
	unsigned int BUF_TAG = (MDLA_BUF_TAG << BUF_TAG_SHIT);
	unsigned long attrs = 0;
	int mma_fd = NULL;
	u32 mva = NULL;

	comm_drv_debug("enter mdla iova alloc!!!!!!!!!!!\n");
	if (!dev) {
		comm_drv_debug("dev init failed\n");
		goto out;
	}

	if (dma_get_mask(dev) < DMA_BIT_MASK(DMA_BIT_MASK_PARA)) {
		comm_drv_debug("register IOMMU client fail, mask=0x%llx\n",
							dma_get_mask(dev));
		goto out;
	}

	if (!dma_supported(dev, 0)) {
		comm_drv_debug("IOMMU not support\n");
		goto out;
	}

	//setting buf_tag
	attrs = BUF_TAG;

	if (cache == 0) {
		// if mapping uncache ,add
		attrs |= DMA_ATTR_WRITE_COMBINE;
	}

	kva = dma_alloc_attrs(dev, size, &dma_addr, GFP_KERNEL, attrs);
	if (dma_mapping_error(dev, dma_addr) != 0) {
		comm_drv_debug("dma_alloc_attrs fail\n");
		goto free;
	}

	table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		goto out;

	// 1.2 get IOMMU buffer sg_table table
	if (dma_get_sgtable_attrs(dev, table, kva, dma_addr, size, 0)) {
		comm_drv_debug("[KTF_IOMMU_TEST]dma_get_sgtable_attrs fail\n");
		dma_free_attrs(dev, size, kva, dma_addr, BUF_TAG);
		kfree(table);
		goto out;
	}
	// 1.3 flush IOMMU buffer after cpu access buffer in cache
	dma_sync_sg_for_device(dev, table->sgl, table->nents, DMA_BIDIRECTIONAL);

	// 1.4 flush IOMMU buffer after device access buffer for cpu
	dma_sync_sg_for_cpu(dev, table->sgl, table->nents, DMA_BIDIRECTIONAL);


	if (dma_addr < MTK_IOMMU_DTV_IOVA_START || dma_addr >= MTK_IOMMU_DTV_IOVA_END) {
		comm_drv_debug("dma_alloc_attrs fail,iova =0x%llx\n", dma_addr);
		dma_free_attrs(dev, size, kva, dma_addr, BUF_TAG);
		goto free;
	}

	*pa = dma_addr;
	comm_drv_debug("after dma_to_phys\n");
	comm_drv_debug("iova: %08x kva: %08llx\n", *pa, kva);

	comm_drv_debug("after dma_to_phys\n");
	comm_drv_debug("iova: %llx kva: %llx\n", *pa, kva);

	*fd = dma_addr;
	mma_fd = dma_addr;

	comm_drv_debug("dma_addr & 0xFFFFFFFF = %x\n", dma_addr & BIT32_MAX);
	comm_drv_debug("dma_addr & 0xFFFFFFFF = %llx\n", dma_addr & BIT32_MAX);
	mva = dma_addr & BIT32_MAX;

	comm_drv_debug("mva = %x\n", mva);
	ret = iommu_alloc_buf_register(*fd, kva, *(u64 *)pa, size, NULL, mma_fd, cache);
	//ret = iommu_alloc_buf_register(*fd, kva, *(dma_addr_t *)pa, mva, size, NULL);
	if (ret < 0)
		goto free;

	if (ret == 0)
		return kva;

	iommu_alloc_buf_unregister(kva);
free:
	dma_free_attrs(dev, size, (void *) kva, pa, BUF_TAG);
out:
	return NULL;
}

int mdla_iova_free(struct device *dev, void *virt, u32 size)
{
	struct mdla_iommu_buf buf;
	unsigned int BUF_TAG = (MDLA_BUF_TAG << BUF_TAG_SHIT);
	ktime_t start, end;
	u64 actual_time;
	u64 pa = 0;

	if (mdla_iova_buf_get_by_virt(virt, &buf)) {
		pa = buf.buf_phy;
		iommu_alloc_buf_unregister(virt);

		start = ktime_get();

		dma_free_attrs(dev, size, (void *) virt, pa, BUF_TAG);

		end = ktime_get();
		actual_time = ktime_to_ns(ktime_sub(end, start));
		comm_drv_debug("after dma free attrs\n");
		comm_drv_debug("free spend time %lld ns size %x\n", (long long)actual_time, size);

		comm_drv_debug("mdla_iommu_free va:%llx pid:%x\n", virt, current->pid);
		return 0;
	}

	//dma_free_attrs(dev, size, (void *) virt, pa, BUF_TAG);

	//comm_drv_debug("mdla_iommu_free va:%llx pid:%x\n", virt, current->pid);

	return -EINVAL;
}

int mdla_iova_cache_flush(struct device *dev, int fd, u32 size)
{
	int ret;
	void *kva = NULL;
	struct sg_table *table = NULL;
	//__u64 pa = NULL;
	//int fd = NULL;
	dma_addr_t dma_addr = 0;
	unsigned int BUF_TAG = (MDLA_BUF_TAG << BUF_TAG_SHIT);
	unsigned long attrs = 0;
	int mma_fd = NULL;
	u32 mva = NULL;

	comm_drv_debug("enter mdla iova cache flush!!!!!!!!!!!\n");
	if (!dev) {
		comm_drv_debug("dev init failed\n");
		goto out;
	}

	comm_drv_debug("flush fd : %08x\n", fd);

	kva = mdla_iova_fd_to_kva(fd);

	dma_addr = mdla_iova_fd_to_mva(fd);

	comm_drv_debug("dma_addr : %08x kva: %08llx\n", dma_addr, kva);

	table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		goto out;

	// 1.2 get IOMMU buffer sg_table table
	if (dma_get_sgtable_attrs(dev, table, kva, dma_addr, size, 0)) {
		comm_drv_debug("[KTF_IOMMU_TEST]dma_get_sgtable_attrs fail\n");
		dma_free_attrs(dev, size, kva, dma_addr, BUF_TAG);
		kfree(table);
		goto out;
	}
	// 1.3 flush IOMMU buffer after cpu access buffer in cache
	dma_sync_sg_for_device(dev, table->sgl, table->nents, DMA_BIDIRECTIONAL);

	// 1.4 flush IOMMU buffer after device access buffer for cpu
	dma_sync_sg_for_cpu(dev, table->sgl, table->nents, DMA_BIDIRECTIONAL);

	return 0;

out:
	return -EINVAL;
}

void mdla_iommu_open(void)
{
	//mma_open();
}

void mdla_iommu_release(void)
{
	//mma_release();
}

int mdla_iova_map_uva(struct device *dev, struct vm_area_struct *vma)
{
	u64 addr;
	u64 addr_trans;
	unsigned long size;
	void *virt;
	int ret;
	int cacheable;
	unsigned long attrs = 0;

	comm_drv_debug("enter mdla iova map uva\n");
	comm_drv_debug(" vma->vm_pgoff= %lx\n", vma->vm_pgoff);
	addr = (vma->vm_pgoff) << PAGE_SHIFT;
	size = vma->vm_end - vma->vm_start;

	vma->vm_flags |= VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP | VM_MIXEDMAP;

	virt = mdla_iova_phy_to_virt(addr);
	addr_trans = mdla_iova_phy_to_u64pa(addr);
	cacheable = mdla_iova_phy_to_cacheable(addr);
	comm_drv_debug("virt =%llx\n", virt);
	comm_drv_debug("addr_trans =%llx\n", addr_trans);
	vma->vm_pgoff = 0;
	if (virt) {
		if (cacheable > 0) {
			ret =  dma_mmap_attrs(dev, vma, virt, addr_trans, size, attrs);
			comm_drv_debug("cacheable buffer mmap\n");
		} else {
			attrs |= DMA_ATTR_WRITE_COMBINE;
			ret =  dma_mmap_attrs(dev, vma, virt, addr_trans, size, attrs);
			comm_drv_debug("non cacheable buffer mmap\n");
		}
		comm_drv_debug("!!!!ret = %d\n", ret);
		return ret;
		//return dma_mmap_attrs(dev, vma, virt, addr_trans, size, DMA_ATTR_WRITE_COMBINE);
	} else {
		//non-valid mapping range
		comm_drv_debug("mdla iova map uva  mmap fail! addr (%llx) size %x!\n", addr, size);
		return -EINVAL;
	}
}
