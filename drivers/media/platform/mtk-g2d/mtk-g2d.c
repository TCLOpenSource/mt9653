// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/sysfs.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-memops.h>

#include "mtk-g2d.h"
#include "mtk-g2d-utp-wrapper.h"
#include <uapi/linux/mtk-v4l2-g2d.h>


#define fh2ctx(__fh) container_of(__fh, struct g2d_ctx, fh)
#define ATTR_MOD     (0644)

__u32 log_level = LOG_LEVEL_NORMAL;
__u16 device_num;

static ssize_t mtk_g2d_show_G2D_CRC(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	int ssize = 0;
	uint16_t u16Size = 255;
	u32 CrcValue = 0;

	g2d_get_crc(&CrcValue);

	ssize +=
	    snprintf(buf + ssize, u16Size - ssize, "%x", CrcValue);

	return ssize;
}

static ssize_t mtk_g2d_store_G2D_CRC(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_g2d_show_loglevel(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	int ssize = 0;
	uint16_t u16Size = SYS_SIZE;

	ssize +=
	    scnprintf(buf + ssize, u16Size - ssize, "%u\n", log_level);
	return ssize;
}

static ssize_t mtk_g2d_store_loglevel(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);

	if (err)
		return err;

	log_level = val;
	return count;
}

static ssize_t mtk_g2d_show_status(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	int ssize = 0;
	uint16_t u16Size = SYS_SIZE;

	dump_g2d_status();
	ssize +=
	    scnprintf(buf + ssize, u16Size - ssize, "dump g2d status\n");
	return ssize;
}

static ssize_t mtk_g2d_show_device_num(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	int ssize = 0;
	uint16_t u16Size = SYS_SIZE;

	ssize +=
	    scnprintf(buf + ssize, u16Size - ssize, "%u", device_num);
	return ssize;
}

static struct device_attribute mtk_g2d_attr[] = {
	__ATTR(g2d_query_G2D_CRC, (ATTR_MOD), mtk_g2d_show_G2D_CRC,
	       mtk_g2d_store_G2D_CRC),
	__ATTR(g2d_loglevel, (ATTR_MOD), mtk_g2d_show_loglevel,
	       mtk_g2d_store_loglevel),
	__ATTR(g2d_status, (ATTR_MOD), mtk_g2d_show_status,
	       mtk_g2d_store_loglevel),
	__ATTR(g2d_device_num, (ATTR_MOD), mtk_g2d_show_device_num,
	       mtk_g2d_store_loglevel),
};

static int g2d_ioctl_querycap(struct file *file, void *priv,
	struct v4l2_capability *cap)
{
	strncpy(cap->driver, G2D_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, G2D_NAME, sizeof(cap->card) - 1);
	cap->bus_info[0] = 0;

	return 0;
}

static int vidioc_g_ext_ctrls(struct file *file, void *prv, struct v4l2_ext_controls *ext_ctrls)
{
	int i = 0;
	struct v4l2_ext_control *ctrl = NULL;

	for (i = 0; i < ext_ctrls->count; i++) {
		ctrl = &ext_ctrls->controls[i];
		switch (ctrl->id) {
		case V4L2_CID_EXT_G2D_RENDER:
			g2d_get_render_info(ctrl);
			break;
		case V4L2_CID_EXT_G2D_CTRL:
			break;
		}
	}
	return 0;
}

static int vidioc_s_ext_ctrls(struct file *file, void *prv, struct v4l2_ext_controls *ext_ctrls)
{
	int ret = 0, i = 0;
	struct v4l2_ext_control *ctrl = NULL;

	G2D_DEBUG("ext_ctrls->count =%d\n", ext_ctrls->count);
	for (i = 0; i < ext_ctrls->count; i++) {
		ctrl = &ext_ctrls->controls[i];
		switch (ctrl->id) {
		case V4L2_CID_EXT_G2D_GET_RESOURCE:
			G2D_DEBUG("Begin draw\n");
			ret = g2d_resource(V4L2_CID_EXT_G2D_GET_RESOURCE);
			break;
		case V4L2_CID_EXT_G2D_FREE_RESOURCE:
			G2D_DEBUG("End draw\n");
			ret = g2d_resource(V4L2_CID_EXT_G2D_FREE_RESOURCE);
			break;
		case V4L2_CID_EXT_G2D_RENDER:
			ret = g2d_render(ctrl);
			break;
		case V4L2_CID_EXT_G2D_CTRL:
			ret = g2d_set_ext_ctrl(ctrl);
			break;
		case V4L2_CID_EXT_G2D_WAIT_DONE:
			G2D_DEBUG("End draw\n");
			ret = g2d_wait_done(ctrl);
			break;
		default:
			G2D_ERROR("ctrl id not support %d\n", ctrl->id);
			ret = -1;
			break;
		}
	}
	return ret;
}

static int vidioc_s_ctrl(struct file *file, void *prv, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_ROTATE:
		g2d_set_rotate(ctrl->value);
		break;
	}

	return 0;
}

static int g2d_open(struct file *file)
{
	struct g2d_dev *dev = video_drvdata(file);
	struct g2d_ctx *ctx = NULL;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->dev = dev;

	if (mutex_lock_interruptible(&dev->mutex)) {
		kfree(ctx);
		return -ERESTARTSYS;
	}

	v4l2_fh_init(&ctx->fh, video_devdata(file));
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	mutex_unlock(&dev->mutex);

	G2D_DEBUG("instance opened\n");
	return 0;
}

static int g2d_release(struct file *file)
{
	struct g2d_dev *dev = video_drvdata(file);
	struct g2d_ctx *ctx = fh2ctx(file->private_data);

	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);

	G2D_DEBUG("instance closed\n");
	return 0;
}

static int vidioc_g_selection(struct file *file, void *prv, struct v4l2_selection *s)
{
	g2d_get_clip(s);

	return 0;
}

static int vidioc_s_selection(struct file *file, void *prv, struct v4l2_selection *s)
{
	struct g2d_ctx *ctx = prv;
	struct g2d_dev *dev = ctx->dev;

	if (s->r.top < 0 || s->r.left < 0) {
		v4l2_err(&dev->v4l2_dev, "doesn't support negative values for top & left\n");
		return -EINVAL;
	}

	g2d_set_clip(s);

	return 0;
}

static const struct v4l2_file_operations g2d_fops = {
	.owner = THIS_MODULE,
	.open = g2d_open,
	.release = g2d_release,
	//.poll = v4l2_m2m_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	//.mmap = v4l2_m2m_fop_mmap,
};

static const struct v4l2_ioctl_ops g2d_ioctl_ops = {
	.vidioc_querycap    = g2d_ioctl_querycap,
	.vidioc_s_ctrl = vidioc_s_ctrl,
	.vidioc_s_ext_ctrls = vidioc_s_ext_ctrls,
	.vidioc_g_ext_ctrls = vidioc_g_ext_ctrls,

	.vidioc_g_selection = vidioc_g_selection,
	.vidioc_s_selection = vidioc_s_selection,
};

static struct video_device g2d_videodev = {
	.name = G2D_NAME,
	.fops = &g2d_fops,
	.ioctl_ops = &g2d_ioctl_ops,
	.minor = -1,
	.release = video_device_release,
	.vfl_dir = VFL_DIR_M2M,
};

static const struct of_device_id mtk_g2d_match[];

void __iomem *g2d_pm_base;

static int g2d_disable_clock(struct device *dev, char *dev_clk_name)
{
	int ret = 0;
	struct clk *dev_clk;

	dev_clk = devm_clk_get(dev, dev_clk_name);

	if (IS_ERR(dev_clk)) {
		pr_err("[G2D][%s, %d]: failed to get %s\n", __func__, __LINE__, dev_clk_name);
		return -ENODEV;
	}

	clk_disable_unprepare(dev_clk);

	return ret;
}

static int g2d_enable_clock(struct device *dev, int clk_index, char *dev_clk_name, char bIsMuxClk)
{
	int ret = 0;
	struct clk *dev_clk;
	struct clk_hw *clk_hw;
	struct clk_hw *parent_clk_hw;

	dev_clk = devm_clk_get(dev, dev_clk_name);

	if (IS_ERR(dev_clk)) {
		pr_err("[G2D][%s, %d]: failed to get %s\n", __func__, __LINE__, dev_clk_name);
		return -ENODEV;
	}

	if (bIsMuxClk) {
		clk_hw = __clk_get_hw(dev_clk);
		parent_clk_hw = clk_hw_get_parent_by_index(clk_hw, clk_index);

		if (IS_ERR(parent_clk_hw)) {
			pr_err("[G2D][%s, %d]: failed to get parent_clk_hw\n", __func__, __LINE__);
			return -ENODEV;
		}

		ret = clk_set_parent(dev_clk, parent_clk_hw->clk);

		if (ret) {
			pr_err("[G2D][%s, %d]failed to change clk_index [%d]\n",
			__func__, __LINE__, clk_index);
			return -ENODEV;
		}
	}

	ret = clk_prepare_enable(dev_clk);

	if (ret) {
		pr_err("[G2D][%s, %d]failed to enable clk %s [%d]\n",
		__func__, __LINE__, dev_clk_name, clk_index);
		return -ENODEV;
	}

	return ret;
}

static int g2d_set_clock(struct device *dev, char bEnable)
{
	if (bEnable == 1) {
		if (g2d_enable_clock(dev, 4, "clk_ge", 1))
			return -ENODEV;
		if (g2d_enable_clock(dev, 0, "clk_ge_swen_r2ge", 0))
			return -ENODEV;
		if (g2d_enable_clock(dev, 0, "clk_ge_swen_w2ge", 0))
			return -ENODEV;
		if (g2d_enable_clock(dev, 0, "clk_ge_swen_ge2ge", 0))
			return -ENODEV;
		if (g2d_enable_clock(dev, 0, "clk_ge_swen_psram2ge", 0))
			return -ENODEV;
		if (g2d_enable_clock(dev, 0, "clk_ge_swen_smi2ge", 0))
			return -ENODEV;
		if (g2d_enable_clock(dev, 0, "clk_ge_swen_mcu_nonpm2ge", 0))
			return -ENODEV;
	} else {
		if (g2d_disable_clock(dev, "clk_ge"))
			return -ENODEV;
		if (g2d_disable_clock(dev, "clk_ge_swen_r2ge"))
			return -ENODEV;
		if (g2d_disable_clock(dev, "clk_ge_swen_w2ge"))
			return -ENODEV;
		if (g2d_disable_clock(dev, "clk_ge_swen_ge2ge"))
			return -ENODEV;
		if (g2d_disable_clock(dev, "clk_ge_swen_psram2ge"))
			return -ENODEV;
		if (g2d_disable_clock(dev, "clk_ge_swen_smi2ge"))
			return -ENODEV;
		if (g2d_disable_clock(dev, "clk_ge_swen_mcu_nonpm2ge"))
			return -ENODEV;
	}

	return 0;
}

static int g2d_probe(struct platform_device *pdev)
{

	struct g2d_dev *dev;
	struct video_device *vfd;
	const struct of_device_id *of_id;
	int ret = 0;
	int i = 0;
	struct device_node *np;
	const char *name;
	int num = -1;
	__u32 u32Tmp = DEV_NUM_NONE;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->ctrl_lock);
	mutex_init(&dev->mutex);
	atomic_set(&dev->num_inst, 0);
	init_waitqueue_head(&dev->irq_queue);

	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret)
		goto unprep_clk_gate;
	vfd = video_device_alloc();
	if (!vfd) {
		v4l2_err(&dev->v4l2_dev, "Failed to allocate video device\n");
		ret = -ENOMEM;
		goto unreg_v4l2_dev;
	}
	*vfd = g2d_videodev;
	vfd->lock = &dev->mutex;
	vfd->v4l2_dev = &dev->v4l2_dev;
	vfd->device_caps = V4L2_CAP_VIDEO_M2M;

	np = of_find_compatible_node(NULL, NULL, "mtk-g2d");
	if (np != NULL) {
		name = "fixed_dd_index";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0) {
			pr_err("[%s, %d]:read DTS table failed, name = %s \r\n",
				__func__, __LINE__, name);
		}
		if (u32Tmp != DEV_NUM_NONE)
			num = u32Tmp;
		else
			num = -1;
	}

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, num);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "Failed to register video device\n");
		goto rel_vdev;
	}
	video_set_drvdata(vfd, dev);
	snprintf(vfd->name, sizeof(vfd->name), "%s", g2d_videodev.name);
	dev->vfd = vfd;
	v4l2_info(&dev->v4l2_dev, "device registered as /dev/video%d\n", vfd->num);
	device_num = vfd->num;
	platform_set_drvdata(pdev, dev);

	of_id = of_match_node(mtk_g2d_match, pdev->dev.of_node);
	if (!of_id) {
		ret = -ENODEV;
		goto unreg_video_dev;
	}

	for (i = 0; i < sizeof(mtk_g2d_attr)/sizeof(struct device_attribute); i++) {
		if (device_create_file(&pdev->dev, &mtk_g2d_attr[i]) != 0)
			v4l2_err(&dev->v4l2_dev, "device_create_file fail\n");
	}

	if (g2d_set_clock(&pdev->dev, 1) != 0) {
		v4l2_err(&dev->v4l2_dev, "set clock fail\n");
		ret = -ENODEV;
		goto unreg_video_dev;
	}
	g2d_utp_init();

	return 0;

unreg_video_dev:
	video_unregister_device(dev->vfd);
rel_vdev:
	video_device_release(vfd);
unreg_v4l2_dev:
	v4l2_device_unregister(&dev->v4l2_dev);
unprep_clk_gate:
	clk_unprepare(dev->gate);

	return ret;
}

static int g2d_remove(struct platform_device *pdev)
{
	struct g2d_dev *dev = platform_get_drvdata(pdev);

	video_unregister_device(dev->vfd);
	v4l2_device_unregister(&dev->v4l2_dev);

	return 0;
}

static int g2d_suspend(struct platform_device *dev, pm_message_t state)
{
	g2d_set_powerstate(1);

	if (g2d_set_clock(&dev->dev, 0) != 0) {
		pr_err("[G2D][Suspend]set clock fail\n");
		return -ENODEV;
	}

	return 0;
}

static int g2d_resume(struct platform_device *dev)
{
	if (g2d_set_clock(&dev->dev, 1) != 0) {
		pr_err("[G2D][Resume]set clock fail\n");
		return -ENODEV;
	}

	g2d_set_powerstate(2);

	return 0;
}

static const struct of_device_id mtk_g2d_match[] = {
	{
	 .compatible = "mtk-g2d",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, mtk_g2d_match);

static struct platform_driver g2d_pdrv = {
	.probe = g2d_probe,
	.remove = g2d_remove,
	.suspend = g2d_suspend,
	.resume = g2d_resume,
	.driver = {
		   .name = G2D_NAME,
		   .of_match_table = mtk_g2d_match,
		   },
};

module_platform_driver(g2d_pdrv);

MODULE_AUTHOR("MediaTek TV");
MODULE_DESCRIPTION("mtk g2d driver");
MODULE_LICENSE("GPL");
