/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef DVB_CTRLS_H
#define DVB_CTRLS_H

#include <linux/types.h>
#include <linux/dvb/dmx2.h>

#include <media/dvb_fh.h>

#define DVB_CTRL_MAX_DIMS	4

struct dvb_ctrl;

enum dvb_ctrl_type {
	DVB_CTRL_TYPE_SIGNED_NUM,
	DVB_CTRL_TYPE_UNSIGNED_NUM,
	DVB_CTRL_TYPE_STRING,
	DVB_CTRL_TYPE_ARRAY,
	DVB_CTRL_TYPE_PTR,
	DVB_CTRL_TYPE_MAX
};

typedef void (*dvb_ctrl_notify_fnc)(struct dvb_ctrl *ctrl, void *priv);
typedef bool (*dvb_ctrl_filter)(const struct dvb_ctrl *ctrl);

union dvb_ctrl_ptr {
	s32 *p_s32;
	s64 *p_s64;
	u8 *p_u8;
	u16 *p_u16;
	u32 *p_u32;
	char *p_char;
	void *p;
};

struct dvb_ctrl_ops {
	int (*g_ctrl)(struct dvb_ctrl *ctrl);
	int (*try_ctrl)(struct dvb_ctrl *ctrl);
	int (*s_ctrl)(struct dvb_ctrl *ctrl);
};

struct dvb_ctrl_type_ops {
	bool (*equal)(const struct dvb_ctrl *ctrl,
		union dvb_ctrl_ptr ptr1, union dvb_ctrl_ptr ptr2);
	void (*init)(const struct dvb_ctrl *ctrl, union dvb_ctrl_ptr ptr);
	int (*validate)(const struct dvb_ctrl *ctrl, union dvb_ctrl_ptr ptr);
};

struct dvb_ctrl_ref {
	struct list_head node;
	struct dvb_ctrl_ref *next;
	struct dvb_ctrl *ctrl;
	bool from_other_dev;
};

struct dvb_ctrl_handler {
	struct mutex _lock;
	struct mutex *lock;
	struct list_head ctrls;
	struct list_head ctrl_refs;
	struct dvb_ctrl_ref *cached;
	struct dvb_ctrl_ref **buckets;
	dvb_ctrl_notify_fnc notify;
	void *notify_priv;
	u16 nr_of_buckets;
	int error;
};

struct dvb_ctrl {
	struct list_head node;
	struct dvb_ctrl_handler *handler;
	const struct dvb_ctrl_ops *ops;
	const struct dvb_ctrl_type_ops *type_ops;

	unsigned int is_new:1;
	unsigned int has_changed:1;
	unsigned int call_notify:1;

	enum dvb_ctrl_type type;
	u32 id;
	const char *name;

	s64 minimum, maximum, default_value;
	u32 elems;
	u32 elem_size;
	u32 dims[DVB_CTRL_MAX_DIMS];
	u32 nr_of_dims;
	u64 step;
	u32 flags;
	void *priv;

	union dvb_ctrl_ptr p_new;
	union dvb_ctrl_ptr p_cur;
};

struct dvb_ctrl_cfg {
	const struct dvb_ctrl_ops *ops;
	const struct dvb_ctrl_type_ops *type_ops;
	u32 id;
	const char *name;
	enum dvb_ctrl_type type;
	s64 min;
	s64 max;
	u64 step;
	s64 def;
	u32 dims[DVB_CTRL_MAX_DIMS];
	u32 elem_size;
	u32 flags; /* DMX2_CTRL_FLAG_XXX */
	void *priv;
};

int dvb_ctrl_handler_init(struct dvb_ctrl_handler *hdl, u32 nr_of_controls_hint);
int dvb_ctrl_handler_free(struct dvb_ctrl_handler *hdl);

struct dvb_ctrl *dvb_ctrl_new_item(struct dvb_ctrl_handler *hdl, const struct dvb_ctrl_cfg *cfg);
struct dvb_ctrl *dvb_ctrl_find(struct dvb_ctrl_handler *hdl, u32 id);

int dvb_ctrl_grab(struct dvb_ctrl *ctrl, bool grabbed);
int dvb_ctrl_notify(struct dvb_ctrl *ctrl, dvb_ctrl_notify_fnc notify, void *priv);

int dvb_ctrl_query(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_queryctrl *qc);
int dvb_ctrl_set(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_control *c);
int dvb_ctrl_get(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_control *c);
int dvb_ctrls_set(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_controls *c);
int dvb_ctrls_get(struct dvb_fh *fh, struct dvb_ctrl_handler *hdl, struct dmx2_controls *c);

#endif /* DVB_CTRLS_H */
