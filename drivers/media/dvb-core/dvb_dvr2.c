// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#define file_name "dvb_dvr2"
#define pr_fmt(fmt) file_name ": " fmt
#define debug_flag dvb_dvr2_debug

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <media/dvb_dvr2.h>

#define dprintk(fmt, arg...) do {	\
	if (debug_flag)	\
		pr_debug(pr_fmt("%s: " fmt),	\
		       __func__, ##arg);	\
} while (0)

#define has_op(dev, op)	\
	(dev->ops && dev->ops->op)

#define call_op(dev, op, args...)	\
	(has_op(dev, op) ? dev->ops->op(args) : -ENOTTY)

static int debug_flag;
module_param(debug_flag, int, 0644);
MODULE_PARM_DESC(debug_flag, "Turn on/off" file_name " debugging (default:off).");

int dvb_dvr2_open(struct dvb_dvr2 *dvr, struct dvb_fh *fh)
{
	if (!dvr) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&dvr->mutex))
		return -ERESTARTSYS;

	// TODO: Overriding avoidance is checked in mtk_dvr2_open
	dvr->fh = fh;
	dvr->state = DVR2_STATE_ALLOCATED;

	mutex_unlock(&dvr->mutex);
	return 0;
}
EXPORT_SYMBOL(dvb_dvr2_open);

int dvb_dvr2_close(struct dvb_dvr2 *dvr)
{
	if (!dvr) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&dvr->mutex))
		return -ERESTARTSYS;

	dvr->fh = NULL;
	dvr->state = DVR2_STATE_FREE;

	mutex_unlock(&dvr->mutex);
	return 0;
}
EXPORT_SYMBOL(dvb_dvr2_close);

int dvb_dvr2_start(struct dvb_dvr2 *dvr)
{
	int ret;

	if (!dvr) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&dvr->mutex))
		return -ERESTARTSYS;

	ret = call_op(dvr, start, dvr);
	if (ret < 0) {
		pr_err("start dvr fail\n");
		mutex_unlock(&dvr->mutex);
		return -EFAULT;
	}

	dvr->state = DVR2_STATE_GO;

	mutex_unlock(&dvr->mutex);
	return 0;
}
EXPORT_SYMBOL(dvb_dvr2_start);

int dvb_dvr2_stop(struct dvb_dvr2 *dvr)
{
	int ret;

	if (!dvr) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&dvr->mutex))
		return -ERESTARTSYS;

	ret = call_op(dvr, stop, dvr);
	if (ret < 0) {
		pr_err("stop dvr fail\n");
		mutex_unlock(&dvr->mutex);
		return -EFAULT;
	}

	dvr->state = DVR2_STATE_ALLOCATED;

	mutex_unlock(&dvr->mutex);
	return 0;
}
EXPORT_SYMBOL(dvb_dvr2_stop);

int dvb_dvr2_flush(struct dvb_dvr2 *dvr)
{
	int ret;

	if (!dvr) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&dvr->mutex))
		return -ERESTARTSYS;

	ret = call_op(dvr, flush, dvr);
	if (ret < 0) {
		pr_err("flush dvr fail\n");
		mutex_unlock(&dvr->mutex);
		return -EFAULT;
	}

	mutex_unlock(&dvr->mutex);
	return 0;
}
EXPORT_SYMBOL(dvb_dvr2_flush);

int dvb_dvr2_write(struct dvb_dvr2 *dvr)
{
	int ret;

	if (!dvr) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&dvr->mutex))
		return -ERESTARTSYS;

	ret = call_op(dvr, write, dvr);
	if (ret < 0) {
		pr_err("write dvr fail\n");
		mutex_unlock(&dvr->mutex);
		return -EFAULT;
	}

	mutex_unlock(&dvr->mutex);
	return ret;
}
EXPORT_SYMBOL(dvb_dvr2_write);

int dvb_dvr2_set_buffer(
	struct dvb_dvr2 *dvr,
	enum dvr2_buf_ctx_type type,
	struct dmx2_buffer_ctx *buf_ctx)
{
	if (!dvr) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if ((type < DVR2_BUF_CTX_TYPE_IN) || (type >= DVR2_BUF_CTX_TYPE_MAX)) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&dvr->mutex))
		return -ERESTARTSYS;

	dvr->buf_ctx[(unsigned int)type] = buf_ctx;

	mutex_unlock(&dvr->mutex);
	return 0;
}
EXPORT_SYMBOL(dvb_dvr2_set_buffer);

int dvb_dvr2_init(struct dvb_dvr2 *dvr)
{
	if (!dvr || !dvr->ops) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	if (dvr->users >= DVB_DVR2_USERS_MAX) {
		pr_err("reach max user limit\n");
		return -EUSERS;
	}

	dvr->users++;
	dvr->state = DVR2_STATE_FREE;

	mutex_init(&dvr->mutex);
	spin_lock_init(&dvr->lock);

	return 0;
}
EXPORT_SYMBOL(dvb_dvr2_init);

int dvb_dvr2_exit(struct dvb_dvr2 *dvr)
{
	if (!dvr) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	dvr->users--;

	return 0;
}
EXPORT_SYMBOL(dvb_dvr2_exit);
