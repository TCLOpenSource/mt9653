// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#define file_name "dmx2dev"
#define pr_fmt(fmt) file_name ": " fmt
#define debug_flag dmx2dev_debug

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mutex.h>

#include <media/dvb_vb2.h>
#include <media/dmx2dev.h>

#define NUMBER_OF_SINK_PADS	(1)

#define dprintk(fmt, arg...) do {	\
	if (debug_flag)	\
		pr_debug(pr_fmt("%s: " fmt),	\
		       __func__, ##arg);	\
} while (0)

#define has_op(dev, type, op)	\
	(dev->type##_fops && dev->type##_fops->op)

#define get_op(dev, type, op)	\
	(has_op(dev, type, op) ? dev->type##_fops->op : NULL)

#define call_op(dev, type, op, args...)	\
	(has_op(dev, type, op) ? dev->type##_fops->op(args) : -ENOTTY)

struct dvbdev_list_node {
	struct list_head list;
	struct dvb_device *dvbdev;
};

static int debug_flag;
module_param(debug_flag, int, 0644);
MODULE_PARM_DESC(debug_flag, "Turn on/off " file_name " debugging (default:off).");

static LIST_HEAD(dvbdev_list);
static DEFINE_MUTEX(dvbdev_list_mutex);

static struct dvbdev_list_node *find_dvbdev_list_node(int minor)
{
	struct list_head *entry;
	struct dvbdev_list_node *node;

	mutex_lock(&dvbdev_list_mutex);

	list_for_each(entry, &dvbdev_list) {
		node = list_entry(entry, struct dvbdev_list_node, list);
		if (node->dvbdev->minor == minor) {
			mutex_unlock(&dvbdev_list_mutex);
			return node;
		}
	}

	mutex_unlock(&dvbdev_list_mutex);

	return NULL;
}

static inline int dvbdev_list_add(struct dvb_device *dvbdev)
{
	struct dvbdev_list_node *node;

	node = find_dvbdev_list_node(dvbdev->minor);
	if (node)
		return 0;

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	node->dvbdev = dvbdev;

	mutex_lock(&dvbdev_list_mutex);
	list_add_tail(&node->list, &dvbdev_list);
	mutex_unlock(&dvbdev_list_mutex);

	return 0;
}

static inline int dvbdev_list_remove(int minor)
{
	struct dvbdev_list_node *node;

	node = find_dvbdev_list_node(minor);
	if (!node)
		return 0;

	mutex_lock(&dvbdev_list_mutex);
	list_del(&node->list);
	mutex_unlock(&dvbdev_list_mutex);

	kfree(node);
	return 0;
}

static int dmx_usercopy(struct file *file,
		     unsigned int cmd, unsigned long arg,
		     long (*func)(struct file *file,
		     unsigned int cmd, unsigned long arg))
{
#define BUF_SIZE 128

	char    sbuf[BUF_SIZE];
	void    *mbuf = NULL;
	void    *parg = NULL;
	int     err  = -EINVAL;

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		/*
		 * For this command, the pointer is actually an integer
		 * argument.
		 */
		parg = (void *) arg;
		break;
	case _IOC_READ: /* some v4l ioctls are marked wrong ... */
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (mbuf == NULL)
				return -ENOMEM;
			parg = mbuf;
		}

		err = -EFAULT;
		if (copy_from_user(parg, (void __user *)arg, _IOC_SIZE(cmd)))
			goto out;
		break;
	}

	/* call driver */
	err = func(file, cmd, (unsigned long)parg);
	if (err == -ENOIOCTLCMD)
		err = -ENOTTY;

	if (err < 0)
		goto out;

	/*  Copy results into user buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user((void __user *)arg, parg, _IOC_SIZE(cmd)))
			err = -EFAULT;
		break;
	}

out:
	kfree(mbuf);
	return err;
}

static int dvb_demux2_open(struct inode *inode, struct file *file)
{
	int ret;
	struct dvb_device *dvbdev = file->private_data;
	struct dmx2dev *dmx2dev = dvbdev->priv;

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	if (dmx2dev->exit) {
		mutex_unlock(&dmx2dev->mutex);
		return -ENODEV;
	}

	ret = call_op(dmx2dev, dmx, open, inode, file);

	dvbdev->users++;

	mutex_unlock(&dmx2dev->mutex);

	return ret;
}

static int dvb_demux2_release(struct inode *inode, struct file *file)
{
	int ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	mutex_lock(&dmx2dev->mutex);

	ret = call_op(dmx2dev, dmx, release, inode, file);

	dmx2dev->dvbdev->users--;

	if (dmx2dev->dvbdev->users == 1 && dmx2dev->exit == 1) {
		mutex_unlock(&dmx2dev->mutex);
		wake_up(&dmx2dev->dvbdev->wait_queue);
	} else
		mutex_unlock(&dmx2dev->mutex);

	return ret;
}

static ssize_t dvb_demux2_read(
	struct file *file,
	char __user *buf, size_t count,
	loff_t *ppos)
{
	ssize_t ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = call_op(dmx2dev, dmx, read, file, buf, count, ppos);

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}

static long dvb_demux2_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = dmx_usercopy(file, cmd, arg, get_op(dmx2dev, dmx, unlocked_ioctl));

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}

#ifdef CONFIG_COMPAT
static long dvb_demux2_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = dmx_usercopy(file, cmd, arg, get_op(dmx2dev, dmx, compat_ioctl));

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}
#endif

static __poll_t dvb_demux2_poll(struct file *file, poll_table *wait)
{
	__poll_t ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -EPOLLERR;

	ret = call_op(dmx2dev, dmx, poll, file, wait);

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}

#ifdef CONFIG_DVB_MMAP
static int dvb_demux2_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = call_op(dmx2dev, dmx, mmap, file, vma);

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}
#endif

static const struct file_operations dvb_demux2_fops = {
	.owner = THIS_MODULE,
	.open = dvb_demux2_open,
	.release = dvb_demux2_release,
	.read = dvb_demux2_read,
	.unlocked_ioctl = dvb_demux2_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dvb_demux2_compat_ioctl,
#endif
	.poll = dvb_demux2_poll,
	.llseek = default_llseek,
#ifdef CONFIG_DVB_MMAP
	.mmap = dvb_demux2_mmap,
#endif
};

static const struct dvb_device dvbdev_demux2 = {
	.priv = NULL,
	.users = 1,
	.writers = 1,
#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
	.name = "dvb-demux2",
#endif
	.fops = &dvb_demux2_fops
};

static int dvb_dvr2_open(struct inode *inode, struct file *file)
{
	int ret;
	struct dvb_device *dvbdev = file->private_data;
	struct dmx2dev *dmx2dev = dvbdev->priv;

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	if (dmx2dev->exit) {
		mutex_unlock(&dmx2dev->mutex);
		return -ENODEV;
	}

	if ((file->f_flags & O_ACCMODE) == O_RDONLY ||
		(file->f_flags & O_ACCMODE) == O_RDWR) {
		if (!dvbdev->readers) {
			mutex_unlock(&dmx2dev->mutex);
			return -EBUSY;
		}

		dvbdev->readers--;
	}

	ret = call_op(dmx2dev, dvr, open, inode, file);

	dvbdev->users++;

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}

static int dvb_dvr2_release(struct inode *inode, struct file *file)
{
	int ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	mutex_lock(&dmx2dev->mutex);

	if ((file->f_flags & O_ACCMODE) == O_RDONLY ||
		(file->f_flags & O_ACCMODE) == O_RDWR)
		dmx2dev->dvr_dvbdev->readers++;

	ret = call_op(dmx2dev, dvr, release, inode, file);

	dmx2dev->dvr_dvbdev->users--;

	if (dmx2dev->dvr_dvbdev->users == 1 && dmx2dev->exit == 1) {
		mutex_unlock(&dmx2dev->mutex);
		wake_up(&dmx2dev->dvr_dvbdev->wait_queue);
	} else
		mutex_unlock(&dmx2dev->mutex);

	return ret;
}

static ssize_t dvb_dvr2_read(
	struct file *file,
	char __user *buf, size_t count,
	loff_t *ppos)
{
	ssize_t ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = call_op(dmx2dev, dvr, read, file, buf, count, ppos);

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}

static ssize_t dvb_dvr2_write(
	struct file *file,
	const char __user *buf, size_t count,
	loff_t *ppos)
{
	ssize_t ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = call_op(dmx2dev, dvr, write, file, buf, count, ppos);

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}

static long dvb_dvr2_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = dmx_usercopy(file, cmd, arg, get_op(dmx2dev, dvr, unlocked_ioctl));

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}

#ifdef CONFIG_COMPAT
static long dvb_dvr2_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = dmx_usercopy(file, cmd, arg, get_op(dmx2dev, dvr, compat_ioctl));

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}
#endif

static __poll_t dvb_dvr2_poll(struct file *file, poll_table *wait)
{
	__poll_t ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -EPOLLERR;

	ret = call_op(dmx2dev, dvr, poll, file, wait);

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}

#if CONFIG_DVB_MMAP
static int dvb_dvr2_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;
	struct dvb_fh *fh = file->private_data;
	struct dmx2dev *dmx2dev = fh_to_dmx2dev(fh);

	if (mutex_lock_interruptible(&dmx2dev->mutex))
		return -ERESTARTSYS;

	ret = call_op(dmx2dev, dvr, mmap, file, vma);

	mutex_unlock(&dmx2dev->mutex);
	return ret;
}
#endif

static const struct file_operations dvb_dvr2_fops = {
	.owner = THIS_MODULE,
	.open = dvb_dvr2_open,
	.release = dvb_dvr2_release,
	.read = dvb_dvr2_read,
	.write = dvb_dvr2_write,
	.unlocked_ioctl = dvb_dvr2_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dvb_dvr2_compat_ioctl,
#endif
	.poll = dvb_dvr2_poll,
	.llseek = default_llseek,
#ifdef CONFIG_DVB_MMAP
	.mmap = dvb_dvr2_mmap,
#endif
};

static const struct dvb_device dvbdev_dvr2 = {
	.priv = NULL,
	.readers = 1,
	.users = 1,
#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
	.name = "dvb-dvr2",
#endif
	.fops = &dvb_dvr2_fops
};

int dvb_dmx2dev_init(struct dmx2dev *dmx2dev, struct dvb_adapter *adap)
{
	if (!dmx2dev || !adap || !dmx2dev->dmx_fops || !dmx2dev->dvr_fops)
		return -EINVAL;

	mutex_init(&dmx2dev->mutex);

	spin_lock_init(&dmx2dev->fh_mgr.fh_lock);
	INIT_LIST_HEAD(&dmx2dev->fh_mgr.fh_list);

	dvb_register_device(adap, &dmx2dev->dvbdev, &dvbdev_demux2,
		dmx2dev, DVB_DEVICE_DEMUX2, NUMBER_OF_SINK_PADS);
	dvbdev_list_add(dmx2dev->dvbdev);

	dvb_register_device(adap, &dmx2dev->dvr_dvbdev, &dvbdev_dvr2,
		dmx2dev, DVB_DEVICE_DVR2, NUMBER_OF_SINK_PADS);
	dvbdev_list_add(dmx2dev->dvr_dvbdev);
	return 0;
}
EXPORT_SYMBOL(dvb_dmx2dev_init);

void dvb_dmx2dev_exit(struct dmx2dev *dmx2dev)
{
	if (!dmx2dev)
		return;

	dmx2dev->exit = 1;

	if (dmx2dev->dvbdev->users > 1) {
		wait_event(dmx2dev->dvbdev->wait_queue,
				dmx2dev->dvbdev->users == 1);
	}

	if (dmx2dev->dvr_dvbdev->users > 1) {
		wait_event(dmx2dev->dvr_dvbdev->wait_queue,
				dmx2dev->dvr_dvbdev->users == 1);
	}

	dvbdev_list_remove(dmx2dev->dvbdev->minor);
	dvb_unregister_device(dmx2dev->dvbdev);

	dvbdev_list_remove(dmx2dev->dvr_dvbdev->minor);
	dvb_unregister_device(dmx2dev->dvr_dvbdev);
}
EXPORT_SYMBOL(dvb_dmx2dev_exit);
