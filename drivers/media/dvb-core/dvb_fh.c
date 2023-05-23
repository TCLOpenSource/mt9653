// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/kernel.h>

#include <media/dvb_fh.h>
#include <media/dvb_event.h>

int dvb_fh_init(
	struct dvb_fh *fh,
	enum dvb_device_type type,
	struct dvb_fh_mgr *mgr,
	struct dvb_ctrl_handler *hdl)
{
	if (!fh || !mgr)
		return -EINVAL;

	fh->type = type;
	fh->hdl = hdl;
	fh->mgr = mgr;

	INIT_LIST_HEAD(&fh->list);
	init_waitqueue_head(&fh->wait);
	INIT_LIST_HEAD(&fh->available);
	INIT_LIST_HEAD(&fh->subscribed);
	fh->navailable = 0;
	fh->sequence = -1;
	mutex_init(&fh->subscribe_lock);

	return 0;
}
EXPORT_SYMBOL(dvb_fh_init);

int dvb_fh_add(struct dvb_fh *fh)
{
	if (!fh || !fh->mgr)
		return -EINVAL;

	spin_lock(&fh->mgr->fh_lock);
	list_add(&fh->list, &fh->mgr->fh_list);
	spin_unlock(&fh->mgr->fh_lock);

	return 0;
}
EXPORT_SYMBOL(dvb_fh_add);

int dvb_fh_del(struct dvb_fh *fh)
{
	if (!fh || !fh->mgr)
		return -EINVAL;

	spin_lock(&fh->mgr->fh_lock);
	list_del_init(&fh->list);
	spin_unlock(&fh->mgr->fh_lock);

	return 0;
}
EXPORT_SYMBOL(dvb_fh_del);

int dvb_fh_exit(struct dvb_fh *fh)
{
	if (!fh || !fh->mgr)
		return -EINVAL;

	if (dvb_event_unsubscribe_all(fh) < 0)
		return -EFAULT;

	mutex_destroy(&fh->subscribe_lock);
	fh->mgr = NULL;
	return 0;
}
EXPORT_SYMBOL(dvb_fh_exit);
