/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef DVB_EVENT_H
#define DVB_EVENT_H

#include <linux/types.h>
#include <linux/dvb/dmx2.h>

#include <media/dvb_fh.h>

#define DVB_DEFAULT_EVENTQ_NUM	300

struct dvb_kevent {
	struct list_head list;
	struct dvb_subscribed_event *sev;
	struct dmx2_event event;
};

struct dvb_subscribed_event {
	struct list_head	list;
	u32					type;
	u32					mask;
	u32					flags;
	struct dvb_fh		*fh;
	unsigned int		elems;
	unsigned int		first;
	unsigned int		in_use;
	struct dvb_kevent	events[];
};

int dvb_event_queue(struct dvb_fh *fh, const struct dmx2_event *ev);
int dvb_event_dequeue(struct dvb_fh *fh,
	struct dmx2_event *ev, int non_blocking);
int dvb_event_subscribe(struct dvb_fh *fh,
		const struct dmx2_event_subscription *sub, u32 elems);
int dvb_event_unsubscribe(struct dvb_fh *fh,
		const struct dmx2_event_subscription *sub);
int dvb_event_unsubscribe_all(struct dvb_fh *fh);

#endif /* DVB_EVENT_H */
