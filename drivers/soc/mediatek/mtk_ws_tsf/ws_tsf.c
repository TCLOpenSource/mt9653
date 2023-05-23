// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2019 MediaTek Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

//-------------------------------------------------------------------------------------------------
//  Include files
//-------------------------------------------------------------------------------------------------
#include <linux/of.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include "ws_tsf.h"

MODULE_LICENSE("GPL");

//-------------------------------------------------------------------------------------------------
// Define & data type
//-------------------------------------------------------------------------------------------------

#define MTK_WS_TSF_GPIO_INVALID     0xFFFF
#define MTK_WS_TSF_DEVICE_COUNT     1
#define MDRV_NAME_WS_TSF            "ws_tsf"
#define MDRV_MINOR_WS_TSF           0x00
#define MASK_32BITS                 0xFFFFFFFF
#define MASK_8BITS                  0xFF
#define SHIFT_32                    32

static struct WS_TSF_DEV_t {
	int s32Major;
	int s32Minor;
	struct cdev cdev;
	const struct file_operations fops;
};

static struct MTK_WS_TSF_Params gt_ws_tsf = {
	.gpio = MTK_WS_TSF_GPIO_INVALID, 
	.ts.tv_sec = 0,
	.ts.tv_nsec = 0
	};

static struct class *tsf_class;
static struct platform_device *gdev;
static int interrupt_type = TYPE_NONE;


/*
 * No lock protect here to keep high precision. Need user space to make sure the gt_ws_tsf
 * won't write when ioctl cmd
 */
int ws_tsf_get_monotonic_raw(struct file *filp, unsigned long arg)
{
	MTK_WS_TSF_DBG("get monotonic raw, sec[%lu] nsec[%lu]\n",
		gt_ws_tsf.ts.tv_sec, gt_ws_tsf.ts.tv_nsec);
	unsigned long long sys_time_ns =  (gt_ws_tsf.ts.tv_nsec & MASK_32BITS) |
		((gt_ws_tsf.ts.tv_sec & MASK_32BITS) << SHIFT_32);

	__put_user(sys_time_ns, (unsigned long long*)arg);

	return 0;
}

static irqreturn_t tsf_gpio_irq_handler(int irq, void *data)
{
	//MTK_WS_TSF_INFO("enter\n");

	getrawmonotonic(&gt_ws_tsf.ts);

	//MTK_WS_TSF_INFO("TSF get latch sys time, second[%lu] ns[%lu]\n",
	//	gt_ws_tsf.ts.tv_sec,  gt_ws_tsf.ts.tv_nsec);
	return IRQ_HANDLED;
}

int ws_set_irq(unsigned long irq_type)
{
	int i4_ret = 0;

	if(gdev == NULL)
	{
		MTK_WS_TSF_ERR("gdev is null\n");
		return -1;
	}
	
	if(gt_ws_tsf.gpio != MTK_WS_TSF_GPIO_INVALID)
	{
		MTK_WS_TSF_INFO("[%s] irq has been set\n", __func__);
		return 0;
	}

	gt_ws_tsf.gpio = platform_get_irq(gdev, 0);
	if (gt_ws_tsf.gpio != MTK_WS_TSF_GPIO_INVALID)
		MTK_WS_TSF_INFO("[%s] TSF Irq num is %u\n", __func__, gt_ws_tsf.gpio);
	else {
		MTK_WS_TSF_ERR("[%s] Get Irq num failed, ERROR...\n", __func__);
		return -1;
	}
	i4_ret = devm_request_irq(&gdev->dev, gt_ws_tsf.gpio,
			   tsf_gpio_irq_handler,
			   irq_type,
			   "ws_tsf_gpio_irq", &gdev->dev);
	if (i4_ret)
	{
		MTK_WS_TSF_ERR("[%s]devm_request_irq failed, ret = %d\n", __func__, i4_ret);
		return -1;
	}
	else
	{
		MTK_WS_TSF_INFO("[%s]set irq type as %lu\n", __func__, irq_type);
	}
	return 0;
	
}

int ws_set_interrupt_type(struct file *filp, unsigned long arg)
{
	__get_user(interrupt_type, (int *)arg);
	
	if(interrupt_type == TYPE_WOW_INTERRUPT)//FALLING INTERRUPT
	{
		return ws_set_irq(IRQF_TRIGGER_FALLING);
	}
	else if(interrupt_type == TYPE_TSF_INTERRUPT)//RISING INTERRUPT
	{
		return ws_set_irq(IRQF_TRIGGER_RISING);
	}
	else
	{
		MTK_WS_TSF_ERR("Wrong param: %d", interrupt_type);
		return -1;
	}
}

static long mtk_ws_tsf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int err = 0;

	MTK_WS_TSF_DBG("%s, cmd [%u]\n", __func__, cmd);

	/* check cmd valid */
	if (_IOC_TYPE(cmd) == WS_TSF_IOC_MAGIC) {
		if (_IOC_NR(cmd) >= WS_TSF_IOCTL_MAXNR) {
			MTK_WS_TSF_ERR("ioctl NR Error!!! (Cmd=%x)\n", cmd);
			return -ENOTTY;
		}
	} else {
		MTK_WS_TSF_ERR("ioctl MAGIC Error!!! (Cmd=%x)\n", cmd);
		return -ENOTTY;
	}

	/* verify Access */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		MTK_WS_TSF_ERR("access fail, cmd: %x\n", cmd);
		return -EFAULT;
	}

	switch (cmd & MASK_8BITS) {
	case (IOCTL_WS_TSF_GET_LATCH_MONOTONIC_RAW & MASK_8BITS):
		ret = ws_tsf_get_monotonic_raw(filp, arg);
		break;
	case (IOCTL_WS_TSF_SET_INTERRUPT_TYPE & MASK_8BITS):
		ret = ws_set_interrupt_type(filp, arg);
		break;
	default:
		MTK_WS_TSF_ERR("Unsupport cmd: %x\n", cmd & MASK_8BITS);
		ret = -EINVAL;
		break;
	}

	return (long)ret;
}

#if defined(CONFIG_COMPAT)
static long mtk_ws_tsf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return mtk_ws_tsf_ioctl(filp, cmd, arg);
}
#endif


static struct WS_TSF_DEV_t gt_tsf_dev = {

	.s32Major = 0,
	.s32Minor = MDRV_MINOR_WS_TSF,
	.cdev = {
		.kobj = {.name = MDRV_NAME_WS_TSF,},
		.owner = THIS_MODULE,
	},
	.fops = {
	.owner = THIS_MODULE,
#ifdef HAVE_UNLOCKED_IOCTL
	.unlocked_ioctl = mtk_ws_tsf_ioctl,
#else
	.ioctl = mtk_ws_tsf_ioctl,
#endif

#if defined(CONFIG_COMPAT)
	.compat_ioctl = mtk_ws_tsf_compat_ioctl,
#endif
	}
};

static int mtk_ws_tsf_init_dev(void)
{
	int i4_ret;
	dev_t dev = MKDEV(0, 0);

	MTK_WS_TSF_INFO("enter\n");

	//create device file under /dev
	tsf_class = class_create(THIS_MODULE, MDRV_NAME_WS_TSF);
	if (IS_ERR(tsf_class))
		return PTR_ERR(tsf_class);

	if (gt_tsf_dev.s32Major > 0) {
		dev = MKDEV(gt_tsf_dev.s32Major, gt_tsf_dev.s32Minor);
		i4_ret = register_chrdev_region(dev, MTK_WS_TSF_DEVICE_COUNT, MDRV_NAME_WS_TSF);
	} else {
		i4_ret = alloc_chrdev_region(&dev, gt_tsf_dev.s32Minor, MTK_WS_TSF_DEVICE_COUNT,
			MDRV_NAME_WS_TSF);
		gt_tsf_dev.s32Major = MAJOR(dev);
	}

	if (i4_ret != 0) {
		MTK_WS_TSF_ERR("register_chrdev_region fail, error: %d\n", i4_ret);
		return -ENODEV;
	}

	cdev_init(&gt_tsf_dev.cdev, &gt_tsf_dev.fops);

	i4_ret = cdev_add(&gt_tsf_dev.cdev, dev, MTK_WS_TSF_DEVICE_COUNT);
	if (i4_ret != 0) {
		MTK_WS_TSF_ERR("cdev_add fail, error: %d\n", i4_ret);
		goto fail;
	}

	device_create(tsf_class, NULL, dev, NULL, MDRV_NAME_WS_TSF);
	return 0;

fail:
	unregister_chrdev_region(dev, MTK_WS_TSF_DEVICE_COUNT);
	return -ENODEV;
}

static void mtk_ws_tsf_remove_dev(void)
{
	MTK_WS_TSF_INFO("enter\n");

	cdev_del(&gt_tsf_dev.cdev);
	unregister_chrdev_region(MKDEV(gt_tsf_dev.s32Major, gt_tsf_dev.s32Minor),
		MTK_WS_TSF_DEVICE_COUNT);
}

static int mtk_ws_tsf_probe(struct platform_device *pdev)
{
	MTK_WS_TSF_INFO("enter\n");
    gdev = pdev;

	return 0;
}

static int mtk_ws_tsf_remove(struct platform_device *pdev)
{
	MTK_WS_TSF_INFO("enter\n");

	if (gt_ws_tsf.gpio != MTK_WS_TSF_GPIO_INVALID)
	{
		devm_free_irq(&pdev->dev, gt_ws_tsf.gpio, &pdev->dev);
		gt_ws_tsf.gpio = MTK_WS_TSF_GPIO_INVALID;
	}

	return 0;
}

static int mtk_ws_tsf_suspend(struct platform_device *pdev, pm_message_t state)
{
	MTK_WS_TSF_INFO("enter\n");

	/*
	 * Need to free irq when suspend, and request gpio irq again when resume.
	 * Otherwise the irq handler won't work after suspend.
	 */
	if (gt_ws_tsf.gpio != MTK_WS_TSF_GPIO_INVALID)
	{
		devm_free_irq(&pdev->dev, gt_ws_tsf.gpio, &pdev->dev);
		gt_ws_tsf.gpio = MTK_WS_TSF_GPIO_INVALID;
	}

	return 0;
}

static int mtk_ws_tsf_resume(struct platform_device *pdev)
{
	MTK_WS_TSF_INFO("enter\n");
	if(interrupt_type == TYPE_WOW_INTERRUPT)//FALLING INTERRUPT
	{
		return ws_set_irq(IRQF_TRIGGER_FALLING);
	}
	else if(interrupt_type == TYPE_TSF_INTERRUPT)//RISING INTERRUPT
	{
		return ws_set_irq(IRQF_TRIGGER_RISING);
	}
	else
	{
		MTK_WS_TSF_ERR("Wrong interrupt type, not set tsf interrupt: %d", interrupt_type);
	}
	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id mtk_ws_tsf_of_device_ids[] = {
	 {.compatible = MTK_WS_TSF_NODE},
	 {},
};
#endif

static struct platform_driver mtk_ws_tsf_driver = {
	.probe      = mtk_ws_tsf_probe,
	.remove     = mtk_ws_tsf_remove,
	.suspend    = mtk_ws_tsf_suspend,
	.resume     = mtk_ws_tsf_resume,

	.driver = {
#if defined(CONFIG_OF)
	.of_match_table = mtk_ws_tsf_of_device_ids,
#endif
	.name   = MTK_WS_TSF_NODE,
	.owner  = THIS_MODULE,
	}
};

static int __init mtk_ws_tsf_init(void)
{
	int i4_ret = 0;

	i4_ret = mtk_ws_tsf_init_dev();
	if (i4_ret != 0) {
		MTK_WS_TSF_ERR("mtk_ws_tsf_init_dev fail\n");
		return -EINVAL;
	}

	platform_driver_register(&mtk_ws_tsf_driver);

	return 0;
}

static void __exit mtk_ws_tsf_exit(void)
{
	mtk_ws_tsf_remove_dev();
	platform_driver_unregister(&mtk_ws_tsf_driver);
}

MODULE_AUTHOR("MediaTek");
module_init(mtk_ws_tsf_init);
module_exit(mtk_ws_tsf_exit);

