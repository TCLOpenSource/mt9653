// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/mm.h>

#include <media/dvb_event.h>

static unsigned int sev_pos(const struct dvb_subscribed_event *sev, unsigned int idx)
{
	idx += sev->first;
	return idx >= sev->elems ? idx - sev->elems : idx;
}

/* Caller must hold fh->mgr->fh_lock! */
static struct dvb_subscribed_event *_dvb_event_subscribed(
		struct dvb_fh *fh, u32 type, u32 mask)
{
	struct dvb_subscribed_event *sev;

	assert_spin_locked(&fh->mgr->fh_lock);

	list_for_each_entry(sev, &fh->subscribed, list)
		if (sev->type == type && sev->mask == mask)
			return sev;

	return NULL;
}

static void _dvb_event_unsubscribe(struct dvb_subscribed_event *sev)
{
	struct dvb_fh *fh = sev->fh;
	unsigned int i;

	lockdep_assert_held(&fh->subscribe_lock);
	assert_spin_locked(&fh->mgr->fh_lock);

	/* Remove any pending events for this subscription */
	for (i = 0; i < sev->in_use; i++) {
		list_del(&sev->events[sev_pos(sev, i)].list);
		fh->navailable--;
	}

	list_del(&sev->list);
}

static void _dvb_event_queue_fh(struct dvb_fh *fh, const struct dmx2_event *ev)
{
	struct dvb_subscribed_event *sev;
	struct dvb_kevent *kev;

	/* Are we subscribed? */
	sev = _dvb_event_subscribed(fh, ev->type, ev->mask);
	if (sev == NULL)
		return;

	/* Increase event sequence number on fh. */
	fh->sequence++;

	/* Do we have any free events? */
	if (sev->in_use == sev->elems)
		return;

	/* Take one and fill it. */
	kev = sev->events + sev_pos(sev, sev->in_use);
	kev->event.type = ev->type;
	kev->event.u = ev->u;
	kev->event.mask = ev->mask;
	kev->event.sequence = fh->sequence;
	sev->in_use++;
	list_add_tail(&kev->list, &fh->available);

	fh->navailable++;

	wake_up_all(&fh->wait);
}

int dvb_event_queue(struct dvb_fh *fh, const struct dmx2_event *ev)
{
	if (!fh || !fh->mgr)
		return -EINVAL;

	spin_lock(&fh->mgr->fh_lock);

	_dvb_event_queue_fh(fh, ev);

	spin_unlock(&fh->mgr->fh_lock);

	return 0;
}
EXPORT_SYMBOL(dvb_event_queue);

static int _dvb_event_dequeue(struct dvb_fh *fh, struct dmx2_event *ev)
{
	struct dvb_kevent *kev;

	spin_lock(&fh->mgr->fh_lock);

	if (list_empty(&fh->available)) {
		spin_unlock(&fh->mgr->fh_lock);
		return -ENOENT;
	}

	WARN_ON(fh->navailable == 0);

	kev = list_first_entry(&fh->available, struct dvb_kevent, list);
	list_del(&kev->list);
	fh->navailable--;

	kev->event.pending = fh->navailable;
	*ev = kev->event;
	kev->sev->first = sev_pos(kev->sev, 1);
	kev->sev->in_use--;

	spin_unlock(&fh->mgr->fh_lock);

	return 0;
}

int dvb_event_dequeue(struct dvb_fh *fh, struct dmx2_event *ev, int nonblocking)
{
	int ret;

	if (!fh || !fh->mgr)
		return -EINVAL;

	if (nonblocking)
		return _dvb_event_dequeue(fh, ev);

	do {
		ret = wait_event_interruptible(fh->wait,
				fh->navailable != 0);
		if (ret < 0)
			break;

		ret = _dvb_event_dequeue(fh, ev);
	} while (ret == -ENOENT);

	return ret;
}
EXPORT_SYMBOL(dvb_event_dequeue);

int dvb_event_subscribe(struct dvb_fh *fh,
	const struct dmx2_event_subscription *sub,
	u32 elems)
{
	struct dvb_subscribed_event *sev, *found_ev;
	unsigned int i;

	if (sub->type == DMX2_EVENT_ALL)
		return -EINVAL;

	if (elems < 1)
		elems = 1;

	sev = kvzalloc(struct_size(sev, events, elems), GFP_KERNEL);
	if (!sev)
		return -ENOMEM;

	for (i = 0; i < elems; i++)
		sev->events[i].sev = sev;
	sev->type = sub->type;
	sev->mask = sub->mask;
	sev->flags = sub->flags;
	sev->fh = fh;
	sev->elems = elems;

	mutex_lock(&fh->subscribe_lock);

	spin_lock(&fh->mgr->fh_lock);
	found_ev = _dvb_event_subscribed(fh, sub->type, sub->mask);
	if (!found_ev)
		list_add(&sev->list, &fh->subscribed);
	spin_unlock(&fh->mgr->fh_lock);

	if (found_ev) // Already listening
		kvfree(sev);

	mutex_unlock(&fh->subscribe_lock);
	return 0;
}
EXPORT_SYMBOL(dvb_event_subscribe);

int dvb_event_unsubscribe(struct dvb_fh *fh,
		const struct dmx2_event_subscription *sub)
{
	struct dvb_subscribed_event *sev;

	if (sub->type == DMX2_EVENT_ALL)
		return dvb_event_unsubscribe_all(fh);

	mutex_lock(&fh->subscribe_lock);

	spin_lock(&fh->mgr->fh_lock);

	sev = _dvb_event_subscribed(fh, sub->type, sub->mask);
	if (sev != NULL)
		_dvb_event_unsubscribe(sev);

	spin_unlock(&fh->mgr->fh_lock);

	mutex_unlock(&fh->subscribe_lock);

	kvfree(sev);

	return 0;
}
EXPORT_SYMBOL(dvb_event_unsubscribe);

int dvb_event_unsubscribe_all(struct dvb_fh *fh)
{
	struct dmx2_event_subscription sub;
	struct dvb_subscribed_event *sev;

	do {
		sev = NULL;

		spin_lock(&fh->mgr->fh_lock);
		if (!list_empty(&fh->subscribed)) {
			sev = list_first_entry(&fh->subscribed,
					struct dvb_subscribed_event, list);
			sub.type = sev->type;
			sub.mask = sev->mask;
		}
		spin_unlock(&fh->mgr->fh_lock);

		if (sev)
			dvb_event_unsubscribe(fh, &sub);
	} while (sev);

	return 0;
}
EXPORT_SYMBOL(dvb_event_unsubscribe_all);
