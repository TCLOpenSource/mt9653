/* SPDX-License-Identifier: GPL-2.0*/
#ifndef __APUSYS_PLAT_IOMMU_H__
#define __APUSYS_PLAT_IOMMU_H__

#include <linux/platform_device.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include "comm_mem_mgt.h"
#include "plat_mem_wrap.h"

struct mdla_iommu_buf {
	struct list_head  node;
	u32 buf_size;
	int buf_fd;
	int mma_fd;
	void *buf_virt;
	u64 buf_phy;
	pid_t pid;
	pid_t ppid;
	struct dma_buf *dmabuf;
	int cacheable;
};

int mdla_iova_buf_get_by_virt(void *virt, struct mdla_iommu_buf *buf);
void *mdla_iova_phy_to_virt(u64 phy_addr);
struct dma_buf *mdla_iova_phy_to_dmabuf(u64 phy_addr);
u32 mdla_iova_phy_to_size(u64 phy_addr);
struct dma_buf *mdla_iova_fd_to_dmabuf(int fd);
u64 mdla_iova_fd_to_kva(int fd);
u64 mdla_iova_fd_to_mva(int fd);
int mdla_iova_phy_to_all(u64 pa, u32 *size, u64 *kva, u64 *mva, u64 *pid, u64 *fd);
int mdla_iova_fd_to_all(u64 fd, u32 *size, u64 *kva, u64 *mva, u64 *pid, u64 *pa);
void *mdla_iova_alloc(struct device *dev, u32 size, u64 *pa, int *fd, u32 cache);
int mdla_iova_free(struct device *dev, void *virt, u32 size);
int mdla_iova_cache_flush(struct device *dev, int fd, u32 size);
void mdla_iommu_open(void);
void mdla_iommu_release(void);

int mdla_iova_map_uva(struct device *dev, struct vm_area_struct *vma);
#endif
