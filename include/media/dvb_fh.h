/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef DVB_FH_H
#define DVB_FH_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>

#include <media/dvbdev.h>

struct dvb_fh_mgr {
	spinlock_t			fh_lock;
	struct list_head	fh_list;
};

struct dvb_fh {
	enum dvb_device_type	type;

	struct list_head		list;
	struct dvb_fh_mgr		*mgr;
	struct dvb_ctrl_handler	*hdl;

	/* Events */
	wait_queue_head_t		wait;
	struct mutex			subscribe_lock;
	struct list_head		subscribed;
	struct list_head		available;
	u32						navailable;
	u32						sequence;
};

int dvb_fh_init(
	struct dvb_fh *fh,
	enum dvb_device_type type,
	struct dvb_fh_mgr *mgr,
	struct dvb_ctrl_handler *hdl);
int dvb_fh_add(struct dvb_fh *fh);
int dvb_fh_del(struct dvb_fh *fh);
int dvb_fh_exit(struct dvb_fh *fh);

#endif /* DVB_FH_H */
