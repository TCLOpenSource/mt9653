// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <media/dvb_ctrls.h>

#define has_op(ctrl, op) \
	(ctrl->ops && ctrl->ops->op)
#define call_op(ctrl, op) \
	(has_op(ctrl, op) ? ctrl->ops->op(ctrl) : 0)

#define has_type_op(ctrl, op) \
	(ctrl->type_ops && ctrl->type_ops->op)
#define call_type_op(ctrl, op, args...) \
	(has_type_op(ctrl, op) ? ctrl->type_ops->op(ctrl, args) : 0)


static inline u32 node2id(struct list_head *node)
{
	return list_entry(node, struct dvb_ctrl_ref, node)->ctrl->id;
}

static inline int handler_set_err(struct dvb_ctrl_handler *hdl, int err)
{
	if (hdl->error == 0)
		hdl->error = err;

	return err;
}

static int check_range(const struct dvb_ctrl_cfg *cfg)
{
	int ret = 0;
	s64 min = cfg->min;
	s64 max = cfg->max;
	u64 step = cfg->step;
	s64 def = cfg->def;

	switch (cfg->type) {
	case DVB_CTRL_TYPE_SIGNED_NUM:
	case DVB_CTRL_TYPE_UNSIGNED_NUM:
		if (step == 0 || min > max || def < min || def > max)
			ret = -ERANGE;
		break;
	case DVB_CTRL_TYPE_STRING:
		if (min > max || min < 0 || step < 1 || def)
			ret = -ERANGE;
		break;
	case DVB_CTRL_TYPE_ARRAY:
	case DVB_CTRL_TYPE_PTR:
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int handler_new_ref(struct dvb_ctrl_handler *hdl,
			   struct dvb_ctrl *ctrl,
			   struct dvb_ctrl_ref **ctrl_ref,
			   bool from_other_dev)
{
	struct dvb_ctrl_ref *ref, *new_ref;
	u32 id = ctrl->id;
	int bucket = id % hdl->nr_of_buckets;

	if (ctrl_ref)
		*ctrl_ref = NULL;

	if (hdl->error)
		return hdl->error;

	new_ref = kzalloc(sizeof(*new_ref), GFP_KERNEL);
	if (!new_ref)
		return -ENOMEM;

	new_ref->ctrl = ctrl;
	new_ref->from_other_dev = from_other_dev;

	INIT_LIST_HEAD(&new_ref->node);

	mutex_lock(hdl->lock);

	/* O(1) insertion */
	if (list_empty(&hdl->ctrl_refs) || id > node2id(hdl->ctrl_refs.prev)) {
		list_add_tail(&new_ref->node, &hdl->ctrl_refs);
		goto insert_in_hash;
	}

	/* Find insert position in sorted list */
	list_for_each_entry(ref, &hdl->ctrl_refs, node) {
		if (ref->ctrl->id < id)
			continue;

		/* Don't add duplicates */
		if (ref->ctrl->id == id) {
			kfree(new_ref);
			goto unlock;
		}

		list_add(&new_ref->node, ref->node.prev);
		break;
	}

insert_in_hash:
	new_ref->next = hdl->buckets[bucket];
	hdl->buckets[bucket] = new_ref;

	if (ctrl_ref)
		*ctrl_ref = new_ref;

unlock:
	mutex_unlock(hdl->lock);
	return 0;
}

static struct dvb_ctrl_ref *find_ref(struct dvb_ctrl_handler *hdl, u32 id)
{
	struct dvb_ctrl_ref *ref;
	int bucket;

	bucket = id % hdl->nr_of_buckets;

	/* Simple optimization: cache the last control found */
	if (hdl->cached && hdl->cached->ctrl->id == id)
		return hdl->cached;

	/* Not in cache, search the hash */
	ref = hdl->buckets ? hdl->buckets[bucket] : NULL;
	while (ref && ref->ctrl->id != id)
		ref = ref->next;

	if (ref)
		hdl->cached = ref; /* cache it! */
	return ref;
}

static int validate_ctrl(struct dvb_ctrl *ctrl, bool set)
{
	if (ctrl->flags & DMX2_CTRL_FLAG_DISABLED)
		return -EACCES;

	if (set) {
		if (ctrl->flags & DMX2_CTRL_FLAG_READ_ONLY)
			return -EACCES;

		if (ctrl->flags & DMX2_CTRL_FLAG_GRABBED)
			return -EBUSY;
	} else if (ctrl->flags & DMX2_CTRL_FLAG_WRITE_ONLY)
		return -EACCES;

	return 0;
}

static int user_to_new(struct dmx2_control *c, struct dvb_ctrl *ctrl)
{
	int ret;
	u32 tot_ctrl_size = ctrl->elems * ctrl->elem_size;

	if (tot_ctrl_size == 0)
		return -EINVAL;

	if (c->size > tot_ctrl_size)
		return -EINVAL;

	ret = copy_from_user(ctrl->p_new.p, c->ptr, c->size) ? -EFAULT : 0;
	return ret;
}

static int new_to_user(struct dvb_ctrl *ctrl, struct dmx2_control *c)
{
	int ret;
	u32 tot_ctrl_size = ctrl->elems * ctrl->elem_size;

	if (tot_ctrl_size == 0)
		return -EINVAL;

	if (c->size > tot_ctrl_size)
		return -EINVAL;

	ret = copy_to_user(c->ptr, ctrl->p_new.p, c->size) ? -EFAULT : 0;
	return ret;
}

static int new_to_cur(struct dvb_ctrl *ctrl)
{
	if (ctrl == NULL)
		return -EINVAL;

	if (ctrl->has_changed) {
		u32 tot_ctrl_size = ctrl->elems * ctrl->elem_size;

		if (tot_ctrl_size == 0)
			return -EFAULT;

		memcpy(ctrl->p_cur.p, ctrl->p_new.p, tot_ctrl_size);

		if (ctrl->call_notify && ctrl->handler->notify)
			ctrl->handler->notify(ctrl, ctrl->handler->notify_priv);
	}

	return 0;
}

static int validate_new(const struct dvb_ctrl *ctrl, union dvb_ctrl_ptr p_new)
{
	int ret = 0;

	switch (ctrl->type) {
	case DVB_CTRL_TYPE_SIGNED_NUM:
		{
			s64 s_val;

			if (ctrl->elem_size == sizeof(s32))
				s_val = (s64)(*(p_new.p_s32));
			else
				s_val = *(p_new.p_s64);

			if ((s_val < ctrl->minimum) || (s_val > ctrl->maximum))
				ret = -EFAULT;
		}
		break;
	case DVB_CTRL_TYPE_UNSIGNED_NUM:
		{
			u32 u_val;

			if (ctrl->elem_size == sizeof(u8))
				u_val = (u32)(*(p_new.p_u8));
			else if (ctrl->elem_size == sizeof(u16))
				u_val = (u32)(*(p_new.p_u16));
			else
				u_val = *(p_new.p_u32);

			if ((u_val < ctrl->minimum) || (u_val > ctrl->maximum))
				ret = -EFAULT;
		}
		break;
	case DVB_CTRL_TYPE_STRING:
		{
			char last;
			u32 size = ctrl->elem_size;

			if (size > ctrl->maximum + 1)
				size = ctrl->maximum + 1;

			last = p_new.p_char[size - 1];
			p_new.p_char[size - 1] = 0;

			if (strlen(p_new.p_char) == ctrl->maximum && last)
				ret = -ERANGE;
		}
		break;
	case DVB_CTRL_TYPE_ARRAY:
	case DVB_CTRL_TYPE_PTR:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret || call_type_op(ctrl, validate, ctrl->p_new);
}

static int set_ctrl(struct dvb_fh *fh, struct dvb_ctrl *ctrl)
{
	ctrl->is_new = 0;

	if (validate_new(ctrl, ctrl->p_new) < 0)
		return -EINVAL;

	ctrl->is_new = 1;

	if (call_op(ctrl, try_ctrl) < 0)
		return -EFAULT;

	// no change
	if (call_type_op(ctrl, equal, ctrl->p_new, ctrl->p_cur))
		return 0;

	ctrl->has_changed = 1;
	ctrl->priv = fh;

	if (call_op(ctrl, s_ctrl) < 0)
		return -EFAULT;

	if (new_to_cur(ctrl) < 0)
		return -EFAULT;

	ctrl->has_changed = 0;
	return 0;
}

static int get_ctrl(struct dvb_fh *fh, struct dvb_ctrl *ctrl, struct dmx2_control *c)
{
	int ret;

	ctrl->priv = fh;
	ret = call_op(ctrl, g_ctrl);
	if (!ret)
		ret = new_to_user(ctrl, c);

	return ret;
}

int dvb_ctrl_handler_init(struct dvb_ctrl_handler *hdl, u32 nr_of_controls_hint)
{
#define SHIFT_BIT (3)
	mutex_init(&hdl->_lock);
	hdl->lock = &hdl->_lock;

	INIT_LIST_HEAD(&hdl->ctrls);
	INIT_LIST_HEAD(&hdl->ctrl_refs);

	hdl->nr_of_buckets = 1 + (nr_of_controls_hint >> SHIFT_BIT);
	hdl->buckets = kvmalloc_array(
					hdl->nr_of_buckets,
					sizeof(hdl->buckets[0]),
					GFP_KERNEL | __GFP_ZERO);
	hdl->error = hdl->buckets ? 0 : -ENOMEM;

	return hdl->error;
}
EXPORT_SYMBOL(dvb_ctrl_handler_init);

int dvb_ctrl_handler_free(struct dvb_ctrl_handler *hdl)
{
	struct dvb_ctrl_ref *ref, *next_ref;
	struct dvb_ctrl *ctrl, *next_ctrl;

	if (hdl == NULL || hdl->buckets == NULL)
		return -EINVAL;

	mutex_lock(hdl->lock);

	list_for_each_entry_safe(ref, next_ref, &hdl->ctrl_refs, node) {
		list_del(&ref->node);
		kfree(ref);
	}

	list_for_each_entry_safe(ctrl, next_ctrl, &hdl->ctrls, node) {
		list_del(&ctrl->node);
		kvfree(ctrl);
	}

	kvfree(hdl->buckets);

	hdl->buckets = NULL;
	hdl->cached = NULL;
	hdl->error = 0;

	mutex_unlock(hdl->lock);
	mutex_destroy(&hdl->_lock);

	return 0;
}
EXPORT_SYMBOL(dvb_ctrl_handler_free);

struct dvb_ctrl *dvb_ctrl_new_item(struct dvb_ctrl_handler *hdl, const struct dvb_ctrl_cfg *cfg)
{
	int ret;
	u32 nr_of_dims = 0;
	u32 elems = 1;
	struct dvb_ctrl *ctrl;
	u32 tot_ctrl_size, sz_extra;
	void *data;

	/* Sanity checks */
	if ((cfg->name == NULL) || (cfg->type >= DVB_CTRL_TYPE_MAX) || (!cfg->elem_size)) {
		handler_set_err(hdl, -EINVAL);
		return NULL;
	}

	ret = check_range(cfg);
	if (ret < 0) {
		handler_set_err(hdl, ret);
		return NULL;
	}

	if (hdl->error)
		return NULL;

	if (cfg->type == DVB_CTRL_TYPE_ARRAY)
		while (cfg->dims[nr_of_dims]) {
			elems *= cfg->dims[nr_of_dims];
			nr_of_dims++;
			if (nr_of_dims == DVB_CTRL_MAX_DIMS)
				break;
		}

	tot_ctrl_size = elems * cfg->elem_size;
	sz_extra = tot_ctrl_size << 1;
	ctrl = kvzalloc(sizeof(*ctrl) + sz_extra, GFP_KERNEL);
	if (ctrl == NULL) {
		handler_set_err(hdl, -ENOMEM);
		return NULL;
	}

	INIT_LIST_HEAD(&ctrl->node);
	ctrl->handler = hdl;
	ctrl->ops = cfg->ops;
	ctrl->type_ops = cfg->type_ops;
	ctrl->id = cfg->id;
	ctrl->name = cfg->name;
	ctrl->type = cfg->type;
	ctrl->flags = cfg->flags;
	ctrl->minimum = cfg->min;
	ctrl->maximum = cfg->max;
	ctrl->step = cfg->step;
	ctrl->default_value = cfg->def;
	ctrl->elems = elems;
	ctrl->nr_of_dims = nr_of_dims;
	if (nr_of_dims)
		memcpy(ctrl->dims, cfg->dims, nr_of_dims * sizeof(cfg->dims[0]));
	ctrl->elem_size = cfg->elem_size;
	ctrl->priv = cfg->priv;

	data = &ctrl[1];
	ctrl->p_new.p = data;
	ctrl->p_cur.p = data + tot_ctrl_size;

	call_type_op(ctrl, init, ctrl->p_cur);
	call_type_op(ctrl, init, ctrl->p_new);

	if (handler_new_ref(hdl, ctrl, NULL, false)) {
		kvfree(ctrl);
		handler_set_err(hdl, -EFAULT);
		return NULL;
	}

	mutex_lock(hdl->lock);
	list_add_tail(&ctrl->node, &hdl->ctrls);
	mutex_unlock(hdl->lock);

	return ctrl;
}
EXPORT_SYMBOL(dvb_ctrl_new_item);

struct dvb_ctrl *dvb_ctrl_find(struct dvb_ctrl_handler *hdl, u32 id)
{
	struct dvb_ctrl_ref *ref = NULL;

	if (hdl) {
		mutex_lock(hdl->lock);
		ref = find_ref(hdl, id);
		mutex_unlock(hdl->lock);
	}

	return ref ? ref->ctrl : NULL;
}
EXPORT_SYMBOL(dvb_ctrl_find);

int dvb_ctrl_grab(struct dvb_ctrl *ctrl, bool grabbed)
{
	if (!ctrl)
		return -EINVAL;

	mutex_lock(ctrl->handler->lock);

	if (grabbed)
		ctrl->flags |= DMX2_CTRL_FLAG_GRABBED;
	else
		ctrl->flags &= ~DMX2_CTRL_FLAG_GRABBED;

	mutex_unlock(ctrl->handler->lock);
	return 0;
}
EXPORT_SYMBOL(dvb_ctrl_grab);

int dvb_ctrl_notify(struct dvb_ctrl *ctrl, dvb_ctrl_notify_fnc notify, void *priv)
{
	if (!ctrl)
		return -EINVAL;

	if (!notify) {
		ctrl->call_notify = 0;
		ctrl->handler->notify = NULL;
		ctrl->handler->notify_priv = NULL;
		return 0;
	}

	if (WARN_ON(ctrl->handler->notify && ctrl->handler->notify != notify))
		return -EINVAL;

	ctrl->handler->notify = notify;
	ctrl->handler->notify_priv = priv;
	ctrl->call_notify = 1;
	return 0;
}
EXPORT_SYMBOL(dvb_ctrl_notify);

int dvb_ctrl_query(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_queryctrl *qc)
{
	struct dvb_ctrl *ctrl = dvb_ctrl_find(hdl, qc->id);

	if (!ctrl)
		return -EINVAL;

	memset(qc, 0, sizeof(*qc));
	qc->id = ctrl->id;
	strscpy(qc->name, ctrl->name, sizeof(qc->name));
	qc->minimum = ctrl->minimum;
	qc->maximum = ctrl->maximum;
	qc->step = ctrl->step;
	qc->default_value = ctrl->default_value;
	qc->flags = ctrl->flags;
	qc->elem_size = ctrl->elem_size;
	qc->elems = ctrl->elems;
	qc->nr_of_dims = ctrl->nr_of_dims;
	memcpy(qc->dims, ctrl->dims, qc->nr_of_dims * sizeof(qc->dims[0]));

	return 0;
}
EXPORT_SYMBOL(dvb_ctrl_query);

int dvb_ctrl_set(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_control *c)
{
	struct dvb_ctrl *ctrl = dvb_ctrl_find(hdl, c->id);

	if (!ctrl)
		return -EINVAL;

	mutex_lock(hdl->lock);

	if (validate_ctrl(ctrl, true) < 0)
		goto fail;

	if (user_to_new(c, ctrl) < 0)
		goto fail;

	if (set_ctrl(fh, ctrl) < 0)
		goto fail;

	mutex_unlock(hdl->lock);
	return 0;

fail:
	mutex_unlock(hdl->lock);
	return -EFAULT;
}
EXPORT_SYMBOL(dvb_ctrl_set);

int dvb_ctrl_get(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_control *c)
{
	struct dvb_ctrl *ctrl = dvb_ctrl_find(hdl, c->id);

	if (!ctrl)
		return -EINVAL;

	mutex_lock(hdl->lock);

	if (validate_ctrl(ctrl, false) < 0)
		goto fail;

	if (get_ctrl(fh, ctrl, c) < 0)
		goto fail;

	mutex_unlock(hdl->lock);
	return 0;

fail:
	mutex_unlock(hdl->lock);
	return -EFAULT;
}
EXPORT_SYMBOL(dvb_ctrl_get);

int dvb_ctrls_set(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_controls *c)
{
	int ret;
	u32 i;

	c->error_idx = 0;
	for (i = 0; i < c->count; i++) {
		ret = dvb_ctrl_set(fh, hdl, &c->controls[i]);

		if (ret < 0) {
			c->error_idx = i;
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(dvb_ctrls_set);

int dvb_ctrls_get(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_controls *c)
{
	int ret;
	u32 i;

	c->error_idx = 0;
	for (i = 0; i < c->count; i++) {
		ret = dvb_ctrl_get(fh, hdl, &c->controls[i]);

		if (ret < 0) {
			c->error_idx = i;
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(dvb_ctrls_get);
