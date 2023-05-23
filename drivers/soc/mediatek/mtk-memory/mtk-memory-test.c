// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Joe Liu <joe.liu@mediatek.com>
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/ion.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#include "mtk-reserved-memory.h"
#include "mtk-cma.h"

#define HEAP_CNT 32
#define TEST_256MB 0x10000000
#define TEST_128MB 0x08000000
#define TEST_1MB 0x100000
#define TEST_PATTERN_SHORT 0x78
#define TEST_PATTERN_LONG 0x78787878
#define TEST_PATTERN_2_SHORT 0x56
#define TEST_PATTERN_2_LONG 0x56565656

#define DMA_ALLOC_OFFSET 0x300000
#define DMA_REMAIN_FREE_SIZE 0x80000
#define ION_ALLOC_OFFSET 0x200000
#define MAP_PAGE_CNT 3
#define TEST_CNT 3

static struct miscdevice mmap_misc_dev;
static int mmap_buffer_start_index = 1;

int null_memory_test(char *heap_name, unsigned long size)
{
	struct ion_heap_data *data;
	unsigned int i = 0;
	unsigned int null_memory_heap_id = -1;
	int ret = 0;
	int fd = 1;
	int fd_2 = 1;
	int fd_3 = 1;
	struct ion_allocation_data null_memory_alloc_data = {0};
	size_t heap_cnt = 0;
	size_t kmalloc_size = 0;
	unsigned long buffer_pfn = 0;
	ptrdiff_t vaddr = 0;

	heap_cnt = ion_query_heaps_kernel(NULL, heap_cnt);
	kmalloc_size = sizeof(struct ion_heap_data) * heap_cnt;
	data = (struct ion_heap_data *)
		kmalloc(kmalloc_size, GFP_KERNEL);
	if (!data) {
		pr_emerg("Function = %s, no data!\n", __PRETTY_FUNCTION__);
		return -ENOMEM;
	}
	heap_cnt = ion_query_heaps_kernel(data, heap_cnt);

	pr_emerg("currently, query heap cnt is %u\n", heap_cnt);

	for (i = 0; i < heap_cnt; i++) {
		pr_emerg("data[%d] name %s\n", i, data[i].name);
		ret = strncmp(data[i].name, heap_name, 9);
		if (ret == 0) {
			null_memory_heap_id = data[i].heap_id;
			pr_emerg("    get heap_id %u\n", null_memory_heap_id);
		}
	}
	if (null_memory_heap_id == -1) {
		pr_emerg("Function = %s, get heap_id failed\n",
			__PRETTY_FUNCTION__);
		goto invalid_heap_id;
	} else
		pr_emerg("get nullmem heap_id %d OK\n", null_memory_heap_id);

	// get 256MB
	pr_emerg("null memory 256MB, start\n");
	fd = mtk_ion_alloc(TEST_256MB, 1 << null_memory_heap_id,
			ION_FLAG_CACHED);
	buffer_pfn = ion_get_mtkcma_buffer_info(fd);
	if (buffer_pfn > 0) {
		pr_emerg("null memory good access, 0x%lX start\n", buffer_pfn);
		vaddr = (ptrdiff_t)ioremap(buffer_pfn << PAGE_SHIFT, MAP_PAGE_CNT * PAGE_SIZE);
		pr_emerg("vaddr is 0x%lX\n", vaddr);

		pr_emerg("ori value is 0x%lX\n", *(unsigned int *)vaddr);
		memset(vaddr, TEST_PATTERN_SHORT, MAP_PAGE_CNT * PAGE_SIZE);
		if (*(unsigned int *)vaddr != TEST_PATTERN_LONG) {
			pr_emerg("null memory good access, fail\n");
			pr_emerg("value is 0x%lX\n", *(unsigned int *)vaddr);
			pr_emerg("TEST_PATTERN_LONG is 0x%lX\n", TEST_PATTERN_LONG);
		} else
			pr_emerg("null memory good access, pass\n");
		if (vaddr)
			iounmap(vaddr);
		pr_emerg("null memory good access, end\n");
	}
	ksys_close(fd);
	pr_emerg("null memory 256MB, end\n\n\n");

	// get 128MB
	pr_emerg("null memory 128MB, start\n");
	fd = mtk_ion_alloc(TEST_128MB, 1 << null_memory_heap_id,
			ION_FLAG_CACHED);
	ion_get_mtkcma_buffer_info(fd);
	ksys_close(fd);
	pr_emerg("null memory 128MB, end\n\n\n");

	// get 128MB
	pr_emerg("null memory 128MB, over-size, 1st, start\n");
	fd = mtk_ion_alloc(TEST_128MB, 1 << null_memory_heap_id,
			ION_FLAG_CACHED);
	ion_get_mtkcma_buffer_info(fd);

	pr_emerg("null memory 128MB, over-size, 2nd, start\n");
	fd_2 = mtk_ion_alloc(TEST_128MB, 1 << null_memory_heap_id,
			ION_FLAG_CACHED);
	ion_get_mtkcma_buffer_info(fd_2);

	// over size
	pr_emerg("null memory 128MB, over-size, 3rd, start\n");
	fd_3 = mtk_ion_alloc(TEST_128MB, 1 << null_memory_heap_id,
			ION_FLAG_CACHED);
	ion_get_mtkcma_buffer_info(fd_3);

	ksys_close(fd);
	ksys_close(fd_2);
	pr_emerg("null memory over-size, end\n\n\n");

invalid_heap_id:
	kfree(data);
	return fd;
}

void *dma_alloc_free_test(struct device *dev, unsigned long size,
		dma_addr_t *alloc_offset, gfp_t flag, unsigned long attrs)
{
	void *va_ptr = dma_alloc_attrs(dev, size, alloc_offset, flag, attrs);

	if (IS_ERR_OR_NULL(va_ptr)) {
		pr_emerg("Function = %s, fail, va_ptr is 0x%lX, err\n",
				__PRETTY_FUNCTION__, va_ptr);
		pr_emerg("Function = %s, allocated address is 0x%llX\n",
				__PRETTY_FUNCTION__, *alloc_offset);
		return va_ptr;
	}

	dma_free_attrs(dev, (size - DMA_REMAIN_FREE_SIZE), va_ptr, *alloc_offset, attrs);
	dma_free_attrs(dev, DMA_REMAIN_FREE_SIZE, (va_ptr + (size - DMA_REMAIN_FREE_SIZE)),
			(*alloc_offset + (size - DMA_REMAIN_FREE_SIZE)), attrs);
}

static int mmap_userdev_open(struct inode *inode, struct file *file)
{
	// test_0: mtk_ltp_mmap_query_mmap_info
	int ret;
	int npages;
	u64 start_bus_pfn;
	pgprot_t pgprot;
	void *vaddr;
	struct of_mmap_info_data of_mmap_info = {0};
	int i = 0;

	// step_1: get mmap info
	pr_info("mmap test index: 0\n");
	ret = of_mtk_get_reserved_memory_info_by_idx(mtk_mmap_device->of_node,
					mmap_buffer_start_index, &of_mmap_info);
	if (ret < 0) {
		pr_emerg("mmap return %d\n", ret);
		return ret;
	}

	// step_2: mmap to kernel_va
	pr_info("Test mmap to kernel va\n");
	npages = PAGE_ALIGN(of_mmap_info.buffer_size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;

	start_bus_pfn = of_mmap_info.start_bus_address >> PAGE_SHIFT;

	if (of_mmap_info.is_cache)
		pgprot = PAGE_KERNEL;
	else
		pgprot = pgprot_writecombine(PAGE_KERNEL);

	pr_info("start_bus_pfn is 0x%lX\n", start_bus_pfn);
	pr_info("npages is %d\n", npages);
	for (i = 0; i < npages; ++i) {
		*(tmp++) = __pfn_to_page(start_bus_pfn);
		start_bus_pfn++;
	}

	for (i = 0; i < TEST_CNT; i++) {
		vaddr = vmap(pages, npages, VM_MAP, pgprot);
		if (!vaddr) {
			pr_emerg("[%d] vaddr is NULL, return\n", i);
			vfree(pages);
			return -ENOMEM;
		}
		pr_info("[%d] vaddr is 0x%lX\n", i, vaddr);

		memset(vaddr, 0, of_mmap_info.buffer_size);
		pr_info("value is 0x%lX\n", *(unsigned int *)vaddr);
		if (*(unsigned int *)vaddr != 0x0) {
			pr_emerg("write value is not 0x0\n");
			vunmap(vaddr);
			vfree(pages);
			return -EINVAL;
		}

		memset(vaddr, TEST_PATTERN_SHORT, of_mmap_info.buffer_size);
		pr_info("value is 0x%lX\n", *(unsigned int *)vaddr);
		if (*(unsigned int *)vaddr != TEST_PATTERN_LONG) {
			pr_emerg("write value is not 0x78\n");
			vunmap(vaddr);
			vfree(pages);
			return -EINVAL;
		}

		memset(vaddr, TEST_PATTERN_2_SHORT, of_mmap_info.buffer_size);
		pr_info("value is 0x%lX\n", *(unsigned int *)vaddr);
		if (*(unsigned int *)vaddr != TEST_PATTERN_2_LONG) {
			pr_emerg("write value is not 0x56\n");
			vunmap(vaddr);
			vfree(pages);
			return -EINVAL;
		}

		pr_info("do vunmap...\n");
		vunmap(vaddr);
	}
	pr_info("Test mmap to kernel va, finish\n");
	vfree(pages);

	return 0;
}

static int mmap_userdev_release(struct inode *inode,
		struct file *file)
{
	return 0;
}

static long mmap_userdev_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int mmap_userdev_mmap(struct file *filp,
		struct vm_area_struct *vma)
{
	// test_1: mtk_ltp_mmap_map_user_va_rw
	int ret;
	u64 start_bus_pfn;
	pgprot_t pgprot;
	struct of_mmap_info_data of_mmap_info = {0};

	// step_1: get mmap info
	pr_info("mmap test index: 1\n");
	ret = of_mtk_get_reserved_memory_info_by_idx(mtk_mmap_device->of_node,
					mmap_buffer_start_index, &of_mmap_info);
	if (ret < 0) {
		pr_emerg("mmap return %d\n", ret);
		return ret;
	}

	// step_2: mmap to user_va
	pr_info("Test mmap to user va\n");
	start_bus_pfn = of_mmap_info.start_bus_address >> PAGE_SHIFT;

	if (io_remap_pfn_range(vma, vma->vm_start,
			start_bus_pfn, (vma->vm_end - vma->vm_start),
			vma->vm_page_prot)) {
		pr_emerg("mmap user space va failed\n");
		ret = -EAGAIN;
	} else {
		pr_info("mmap user space va: 0x%lX ~ 0x%lX\n",
			vma->vm_start, vma->vm_end);
		ret = 0;
	}

	return ret;
}

static const struct file_operations mmap_userdev_fops = {
	.owner = THIS_MODULE,
	.open = mmap_userdev_open,
	.release = mmap_userdev_release,
	.unlocked_ioctl = mmap_userdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mmap_userdev_ioctl,
#endif
	.mmap = mmap_userdev_mmap,
};

static int get_mmap_buffer_info(struct device *dev, int pool_index)
{
	pgprot_t pgprot;
	int npages;
	u64 start_bus_pfn;
	int ret = 0;

	struct device_node *mtk_test_cma_device_node = dev->of_node;
	struct of_mmap_info_data of_mmap_info = {0};

	if (of_mtk_get_reserved_memory_info_by_idx(
		mtk_test_cma_device_node, pool_index, &of_mmap_info) == 0) {
		pr_emerg("    show mmap info\n");
		pr_emerg("    layer is %u\n",
			of_mmap_info.layer);
		pr_emerg("    miu is %u\n",
			of_mmap_info.miu);
		pr_emerg("    is_cache is %u\n",
			of_mmap_info.is_cache);
		pr_emerg("    cma_id is %u\n",
			of_mmap_info.cma_id);
		pr_emerg("    start_bus_address is 0x%llX\n",
			of_mmap_info.start_bus_address);
		pr_emerg("    buffer_size is 0x%llX\n",
			of_mmap_info.buffer_size);
		ret = 0;
	} else {
		pr_emerg("get mmap info fail\n");
		ret = -ENODEV;
	}

	return ret;
}

int mtk_mmap_test_init(struct device *mtk_test_mmap_device)
{
	int fd = -1;
	unsigned long ion_alloc_offset = ION_ALLOC_OFFSET;
	unsigned long buffer_pfn = 0;

	pr_emerg("[MMAP] Test Start!!\n");

	if (dev_name(mtk_test_mmap_device))
		pr_emerg("device name is %s\n\n\n",
			dev_name(mtk_test_mmap_device));
	else
		pr_emerg("no dev_name for device!!\n\n\n");

	// test_1: get MMAP Buffer use the platform_device->device
	pr_emerg("get MMAP buffer info, start\n");
	get_mmap_buffer_info(mtk_test_mmap_device, 1);
	pr_emerg("get MMAP buffer info, end\n\n\n");

	// test_2: ion_ops
	pr_emerg("user mode cma_alloc (ion), start\n");

	fd = ion_test(mtk_test_mmap_device, ion_alloc_offset,
				PAGE_ALIGN(TEST_1MB), ION_FLAG_CACHED);
	if (fd < 0) {
		pr_emerg("test ion_ops failed, return\n");
		return -EBADF;
	}
	buffer_pfn = ion_get_mtkcma_buffer_info(fd);
	pr_emerg("ion get: buffer_phy is 0x%lX\n", buffer_pfn << PAGE_SHIFT);
	ksys_close(fd);
	pr_emerg("user mode cma_alloc (ion), end\n\n\n");

	// test_3: dma_ops
	pr_emerg("kernel mode cma_alloc (dma_alloc_attrs), start\n");
	dma_addr_t dma_alloc_offset = DMA_ALLOC_OFFSET;

	dma_alloc_free_test(mtk_test_mmap_device, TEST_1MB, &dma_alloc_offset,
				GFP_KERNEL, DMA_ATTR_NO_KERNEL_MAPPING);
	pr_emerg("kernel mode cma_alloc (dma_alloc_attrs), end\n\n\n");

	// test_4: null memory
	pr_emerg("user mode null memory alloc, start\n");
	null_memory_test("nullmem_1", TEST_128MB);
	pr_emerg("user mode null memory alloc, end\n\n\n");

	// test_5: add misc device for test mmap buffer(kernel & user)
	mmap_misc_dev.minor = MISC_DYNAMIC_MINOR;
	mmap_misc_dev.name = "mtk_mmap";
	mmap_misc_dev.fops = &mmap_userdev_fops;

	/* register & deregister misc device */
	if (misc_register(&mmap_misc_dev) < 0)
		pr_emerg("mtk mmap failed to register misc dev\n");
	misc_deregister(&mmap_misc_dev);

	return 0;
}
EXPORT_SYMBOL(mtk_mmap_test_init);
