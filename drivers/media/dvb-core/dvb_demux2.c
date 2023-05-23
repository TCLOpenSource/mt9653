// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#define file_name "dvb_demux2"
#define pr_fmt(fmt) file_name ": " fmt
#define debug_flag dvb_demux2_debug

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>

#include <media/dvb_demux2.h>

#define dprintk(fmt, arg...) do {	\
	if (debug_flag)	\
		pr_debug(pr_fmt("%s: " fmt),	\
		       __func__, ##arg);				\
} while (0)

#define has_op(dev, type, op)	\
	(dev->type##_ops && dev->type##_ops->op)

#define call_op(dev, type, op, args...)	\
	(has_op(dev, type, op) ? dev->type##_ops->op(args) : -ENOTTY)

static int debug_flag;
module_param(debug_flag, int, 0644);
MODULE_PARM_DESC(debug_flag, "Turn on/off " file_name " debugging (default:off).");

static bool validate_capabilities(struct dvb_demux2 *demux, struct dmx2_filter_params *param)
{
	enum dmx2_main_filter_type filter_type = param->type;

	switch (demux->protocol) {
	case DMX2_FORMAT_M2TS:
		if (filter_type != DMX2_FILTER_TYPE_TS)
			return false;
		break;
	case DMX2_FORMAT_ISDB3:
		if ((filter_type != DMX2_FILTER_TYPE_TLV) &&
			(filter_type != DMX2_FILTER_TYPE_IP) &&
			(filter_type != DMX2_FILTER_TYPE_MMTP))
			return false;
		break;
	case DMX2_FORMAT_ATSC3:
		if ((filter_type != DMX2_FILTER_TYPE_ALP) &&
			(filter_type != DMX2_FILTER_TYPE_IP))
			return false;
		break;
	default:
		return false;
	}

	return true;
}

static bool validate_connection(struct dvb_demux2 *demux, struct dmx2_filter_params *param)
{
	if (param->input == DMX2_IN_FILTER) {
		enum dmx2_main_filter_type filter_type;
		enum dmx2_main_filter_type prev_filter_type;

		if (!param->prev_filter)
			return false;

		filter_type = param->type;
		prev_filter_type = param->prev_filter->params.type;

		switch (filter_type) {
		case DMX2_FILTER_TYPE_IP:
			if ((prev_filter_type != DMX2_FILTER_TYPE_TLV) &&
				(prev_filter_type != DMX2_FILTER_TYPE_ALP))
				return false;
			break;
		case DMX2_FILTER_TYPE_MMTP:
			if (prev_filter_type != DMX2_FILTER_TYPE_IP)
				return false;
			break;
		case DMX2_FILTER_TYPE_TS:
		case DMX2_FILTER_TYPE_TLV:
		case DMX2_FILTER_TYPE_ALP:
		default:
			return false;
		}
	}

	return true;
}

static int validate_param(struct dvb_demux2 *demux, struct dmx2_filter_params *param)
{
	if (!validate_capabilities(demux, param)) {
		pr_err("protocol(%x) - filter setting fail\n", demux->protocol);
		return -EINVAL;
	}

	if (!validate_connection(demux, param)) {
		pr_err("protocol(%x) - connection fail\n", demux->protocol);
		return -EINVAL;
	}

	return 0;
}

static inline bool check_prev_filters_state(struct dmx2_filter *filter, enum dvb_dmx2_state state)
{
	struct dmx2_filter *f;

	for (f = filter->params.prev_filter; f; f = f->params.prev_filter)
		if (f->state != state)
			return false;

	return true;
}

static inline bool check_next_filters_state(struct dmx2_filter *filter, enum dvb_dmx2_state state)
{
	struct dmx2_filter *f;

	for (f = filter->next; f; f = f->sibling)
		if (f->state != state)
			return false;

	return true;
}

int dvb_dmx2_filter_open(
	struct dvb_demux2 *demux,
	struct dvb_fh *fh,
	struct dmx2_filter_params *param,
	struct dmx2_filter *filter)
{
	int ret;
	unsigned long flags;
	struct dmx2_feed *feed;

	if (mutex_lock_interruptible(&demux->mutex))
		return -ERESTARTSYS;

	ret = validate_param(demux, param);
	if (ret < 0) {
		pr_err("invalid param setting\n");
		goto fail;
	}

	if (!filter || !filter->feed) {
		pr_err("filter or feed is NULL\n");
		ret = -EFAULT;
		goto fail;
	}

	if (param->input == DMX2_IN_FILTER) {
		struct dmx2_filter *prev = param->prev_filter;

		if (!prev) {
			pr_err("previous filter is NULL\n");
			goto fail;
		}

		if (!prev->next)
			prev->next = filter;
		else {
			struct dmx2_filter *last = prev->next;

			while (last->sibling)
				last = last->sibling;

			last->sibling = filter;
		}

		prev->edge_num++;
	}

	memcpy(&filter->params, param, sizeof(*param));
	filter->parent = demux;
	filter->fh = fh;
	filter->next = filter->sibling = NULL;
	filter->state = DMX2_STATE_ALLOCATED;

	feed = filter->feed;
	feed->parent = demux;
	feed->state = DMX2_STATE_ALLOCATED;
	feed->filter = filter;
	mutex_init(&feed->mutex);

	spin_lock_irqsave(&demux->lock, flags);

	list_add_tail(&filter->list, &demux->filter_list);
	list_add_tail(&feed->list, &demux->feed_list);

	spin_unlock_irqrestore(&demux->lock, flags);

	mutex_unlock(&demux->mutex);
	return 0;

fail:
	mutex_unlock(&demux->mutex);
	return ret;
}
EXPORT_SYMBOL(dvb_dmx2_filter_open);

int dvb_dmx2_filter_start(struct dmx2_filter *filter)
{
	int ret;
	struct dvb_demux2 *demux;

	if (!filter) {
		pr_err("invalid filter\n");
		return -EINVAL;
	}

	demux = filter->parent;

	if (mutex_lock_interruptible(&demux->mutex))
		return -ERESTARTSYS;

	if (filter->state != DMX2_STATE_ALLOCATED) {
		pr_err("filter state fail\n");
		ret = -EINVAL;
		goto fail;
	}

	if (check_prev_filters_state(filter, DMX2_STATE_GO)) {
		struct dmx2_filter *first = filter;
		struct dmx2_filter *stack[DVB_DEMUX2_FILTER_MAX_DEPTH];
		struct dmx2_feed *feed;
		u8 top = 0;

		while (first) {
			stack[top++] = first;
			first = first->params.prev_filter;
		}

		while (top) {
			feed = stack[--top]->feed;

			mutex_lock(&feed->mutex);

			if (feed->state != DMX2_STATE_GO) {
				ret = call_op(demux, feed, start, feed);
				if (ret < 0) {
					pr_err("start feed fail\n");
					mutex_unlock(&feed->mutex);
					goto fail;
				}

				feed->state = DMX2_STATE_GO;
			}

			mutex_unlock(&feed->mutex);
		}
	}

	filter->state = DMX2_STATE_GO;
	mutex_unlock(&demux->mutex);
	return 0;

fail:
	mutex_unlock(&demux->mutex);
	return ret;
}
EXPORT_SYMBOL(dvb_dmx2_filter_start);

int dvb_dmx2_filter_stop(struct dmx2_filter *filter)
{
	int ret;
	struct dvb_demux2 *demux;
	struct dmx2_feed *feed;

	if (!filter) {
		pr_err("invalid filter\n");
		return -EINVAL;
	}

	demux = filter->parent;

	if (mutex_lock_interruptible(&demux->mutex))
		return -ERESTARTSYS;

	if (filter->state != DMX2_STATE_GO) {
		pr_err("filter state fail\n");
		ret = -EINVAL;
		goto fail;
	}

	if (!check_next_filters_state(filter, DMX2_STATE_ALLOCATED)) {
		pr_err("next layer filters need to stop first\n");
		ret = -EFAULT;
		goto fail;
	}

	feed = filter->feed;

	mutex_lock(&feed->mutex);

	ret = call_op(demux, feed, stop, feed);
	if (ret < 0) {
		pr_err("stop feed fail\n");
		mutex_unlock(&feed->mutex);
		goto fail;
	}

	feed->state = DMX2_STATE_ALLOCATED;

	mutex_unlock(&feed->mutex);

	filter->state = DMX2_STATE_ALLOCATED;

	mutex_unlock(&demux->mutex);
	return 0;

fail:
	mutex_unlock(&demux->mutex);
	return ret;
}
EXPORT_SYMBOL(dvb_dmx2_filter_stop);

int dvb_dmx2_filter_flush(struct dmx2_filter *filter)
{
	int ret = 0;
	struct dvb_demux2 *demux;

	if (!filter) {
		pr_err("invalid filter\n");
		return -EINVAL;
	}

	demux = filter->parent;

	if (mutex_lock_interruptible(&demux->mutex))
		return -ERESTARTSYS;

	if (filter->state == DMX2_STATE_GO) {
		struct dmx2_feed *feed = filter->feed;

		mutex_lock(&feed->mutex);

		ret = call_op(demux, feed, flush, feed);
		if (ret < 0)
			pr_err("flush feed fail\n");

		mutex_unlock(&feed->mutex);
	}

	mutex_unlock(&demux->mutex);
	return ret;
}
EXPORT_SYMBOL(dvb_dmx2_filter_flush);

int dvb_dmx2_filter_close(struct dmx2_filter *filter)
{
	struct dvb_demux2 *demux;
	unsigned long flags;

	if (!filter) {
		pr_err("invalid filter\n");
		return -EINVAL;
	}

	demux = filter->parent;

	if (mutex_lock_interruptible(&demux->mutex))
		return -ERESTARTSYS;

	if (filter->next) {
		pr_err("please close following filters first\n");
		goto fail;
	}

	if (filter->params.input == DMX2_IN_FILTER) {
		struct dmx2_filter *prev = filter->params.prev_filter;

		if (!prev) {
			pr_err("previous filter is NULL\n");
			goto fail;
		}

		if (prev->edge_num-- == 1)
			prev->next = NULL;
		else if (prev->next == filter)
			prev->next = filter->sibling;
		else {
			struct dmx2_filter *last = prev->next;

			while (last->sibling != filter)
				last = last->sibling;

			last->sibling = filter->sibling;
		}
	}

	filter->sibling = NULL;

	spin_lock_irqsave(&demux->lock, flags);
	list_del(&filter->list);
	list_del(&filter->feed->list);
	spin_unlock_irqrestore(&demux->lock, flags);

	mutex_unlock(&demux->mutex);
	return 0;

fail:
	mutex_unlock(&demux->mutex);
	return -EFAULT;
}
EXPORT_SYMBOL(dvb_dmx2_filter_close);

int dvb_dmx2_filter_set_buffer(struct dmx2_filter *filter, struct dmx2_buffer_ctx *buf_ctx)
{
	struct dvb_demux2 *demux;
	struct dmx2_feed *feed;

	if (!filter) {
		pr_err("invalid filter\n");
		return -EINVAL;
	}

	demux = filter->parent;
	feed = filter->feed;

	if (mutex_lock_interruptible(&demux->mutex))
		return -ERESTARTSYS;

	mutex_lock(&feed->mutex);
	feed->buf_ctx = buf_ctx;
	mutex_unlock(&feed->mutex);

	mutex_unlock(&demux->mutex);
	return 0;
}
EXPORT_SYMBOL(dvb_dmx2_filter_set_buffer);

struct dmx2_filter *dvb_dmx2_filter_find(struct dvb_demux2 *demux, struct dvb_fh *fh)
{
	unsigned long flags;
	struct dmx2_filter *filter;

	spin_lock_irqsave(&demux->lock, flags);

	list_for_each_entry(filter, &demux->filter_list, list)
		if (filter != NULL && filter->fh == fh) {
			spin_unlock_irqrestore(&demux->lock, flags);
			return filter;
		}

	spin_unlock_irqrestore(&demux->lock, flags);
	return NULL;
}
EXPORT_SYMBOL(dvb_dmx2_filter_find);

struct dmx2_feed *dvb_dmx2_feed_find(struct dvb_demux2 *demux, u16 filter_id)
{
	unsigned long flags;
	struct dmx2_feed *feed;

	spin_lock_irqsave(&demux->lock, flags);

	list_for_each_entry(feed, &demux->feed_list, list) {
		if (feed != NULL && feed->index == filter_id) {
			spin_unlock_irqrestore(&demux->lock, flags);
			return feed;
		}
	}

	spin_unlock_irqrestore(&demux->lock, flags);
	return NULL;
}
EXPORT_SYMBOL(dvb_dmx2_feed_find);

int dvb_dmx2_init(struct dvb_demux2 *demux)
{
	if (!demux->feed_ops) {
		pr_err("invalid fops\n");
		return -EINVAL;
	}

	if (demux->users >= DVB_DEMUX2_USERS_MAX) {
		pr_err("reach max user limit\n");
		return -EUSERS;
	}

	demux->users++;

	mutex_init(&demux->mutex);
	spin_lock_init(&demux->lock);

	INIT_LIST_HEAD(&demux->filter_list);
	INIT_LIST_HEAD(&demux->feed_list);

	return 0;
}
EXPORT_SYMBOL(dvb_dmx2_init);

int dvb_dmx2_exit(struct dvb_demux2 *demux)
{
	demux->users--;

	return 0;
}
EXPORT_SYMBOL(dvb_dmx2_exit);
