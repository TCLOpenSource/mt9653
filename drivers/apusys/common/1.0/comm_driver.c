// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <asm/mman.h>
#include <linux/dmapool.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/version.h>
#include <linux/compat.h>

#ifdef CONFIG_OF
#include <linux/cpu.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#endif

#include "comm_driver.h"
#include "mdla_driver.h"
#include "apusys_device.h"
#include "apusys_drv.h"
#include "comm_util.h"
#include "comm_debug.h"
#include "comm_mem_mgt.h"
#include "comm_teec.h"

#define COMM_NAME "APUSYS_COMM"

/* TODO: move these global control vaiables into device specific data.  */

#ifdef APU_COMM_IOCTL
static int comm_open(struct inode *inodep, struct file *filep)
{
	static u32 numberOpens;

	numberOpens = 0;
	numberOpens++;
	comm_drv_debug("MDLA: Device has been opened %d time(s)\n",
		numberOpens);

	return 0;
}

static int comm_release(struct inode *inodep, struct file *filep)
{
	comm_drv_debug("MDLA: Device successfully closed\n");

	return 0;
}

static int comm_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;

}

static long comm_ioctl_entry(unsigned int cmd, unsigned long arg, unsigned int from_user)
{
	return 0;
}

static long comm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return comm_ioctl_entry(cmd, arg, 1);
}

long comm_kernel_ioctl(unsigned int cmd, void *arg)
{
	return comm_ioctl_entry(cmd, (unsigned long)arg, 0);
}
EXPORT_SYMBOL(comm_kernel_ioctl);

#ifdef CONFIG_COMPAT
static long comm_compat_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	return comm_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations fops = {
	.open		= comm_open,
	.unlocked_ioctl	= comm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= comm_compat_ioctl,
#endif
	.mmap		= comm_mmap,
	.release	= comm_release,
};
#endif //APU_COMM_IOCTL

void comm_reset_lock(int core, int res)
{

}

static int comm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	comm_util_init(pdev);
	comm_mem_init(pdev);
	comm_dbg_init();

	dev_info(dev, "%s: done\n", __func__);

	return 0;

}

static int comm_remove(struct platform_device *pdev)
{
	comm_drv_debug("%s start -\n", __func__);

	platform_set_drvdata(pdev, NULL);

	comm_drv_debug("%s done -\n", __func__);

	return 0;
}

static int comm_resume(struct platform_device *pdev)
{
	comm_drv_debug("%s: resume\n", __func__);

	return 0;
}

static int comm_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	comm_drv_debug("%s: suspend\n", __func__);
	comm_mem_suspend();

	return 0;
}

static const struct of_device_id comm_of_match[] = {
	{ .compatible = "mediatek,apusys_comm", },
	{ /* end of list */ },
};

static struct platform_driver comm_driver = {
	.driver = {
		.name = COMM_NAME,
		.owner = THIS_MODULE,
		.of_match_table = comm_of_match,
	},
	.probe = comm_probe,

	.suspend = comm_suspend,
	.resume = comm_resume,

	.remove = comm_remove,
};

static int comm_init(void)
{
	int ret;

	ret = platform_driver_register(&comm_driver);
	if (ret != 0)
		return ret;
	comm_drv_debug("%s done!\n", __func__);

	return 0;
}

static void comm_exit(void)
{
	comm_drv_debug("Goodbye from the LKM!\n");
	platform_driver_unregister(&comm_driver);
}

static struct  comm_mem_cb comm_mem = {
	.alloc		=	comm_mem_alloc,
	.free		=	comm_mem_free,
	.import		=	comm_mem_import,
	.cache_flush	=	comm_mem_cache_flush,
	.unimport	=	comm_mem_unimport,
	.map_iova	=	comm_mem_map_iova,
	.map_kva	=	comm_mem_map_kva,
	.unmap_iova	=	comm_mem_unmap_iova,
	.unmap_kva	=	comm_mem_unmap_kva,
	.map_uva	=	comm_mem_map_uva,
	.query_mem	=	comm_mem_query_mem,
	.vlm_info	=	comm_mem_vlm_info,
	.hw_supported	=	comm_hw_supported,
	.open_teec_context	=	comm_teec_open_context,
	.close_teec_context	=	comm_teec_close_context,
	.send_teec_cmd		=	comm_teec_send_command,
};

struct comm_mem_cb *comm_util_get_cb(void)
{
	return &comm_mem;
}
EXPORT_SYMBOL(comm_util_get_cb);

late_initcall(comm_init);
module_exit(comm_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("APUCOMM Driver");
MODULE_VERSION("2.0");
