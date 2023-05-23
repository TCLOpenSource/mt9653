/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef DVB_DEMUX2_H
#define DVB_DEMUX2_H

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/bits.h>

#include <linux/dvb/dmx2.h>

#include <media/dvb_fh.h>

#define DVB_DEMUX2_USERS_MAX	10
#define DVB_DEMUX2_BUFFER_SIZE	8192
#define DVB_DEMUX2_FILTER_MAX_DEPTH		3

struct dmx2_feed;
struct dmx2_buffer_ctx;

enum dvb_dmx2_state {
	DMX2_STATE_ALLOCATED,
	DMX2_STATE_GO,
	DMX2_STATE_DONE,
	DMX2_STATE_TIMEOUT,
};

enum dmx2_main_filter_type {
	DMX2_FILTER_TYPE_TS,
	DMX2_FILTER_TYPE_TLV,
	DMX2_FILTER_TYPE_ALP,
	DMX2_FILTER_TYPE_IP,
	DMX2_FILTER_TYPE_MMTP,
	DMX2_FILTER_TYPE_MAX
};

enum dmx2_protocol_format {
	DMX2_FORMAT_NONE,
	DMX2_FORMAT_M2TS = BIT(0),
	DMX2_FORMAT_ISDB3 = BIT(1),
	DMX2_FORMAT_ATSC3 = BIT(2),
};

typedef int (*dmx2_feed_cb)(
	const u8 *buf1, size_t len1,
	const u8 *buf2, size_t len2,
	struct dmx2_feed *src, u32 *buf_flags);

struct dmx2_filter_caps {
	int number;
};

struct dmx2_filter_params {
	enum dmx2_input input;
	struct dmx2_filter *prev_filter;
	enum dmx2_output output;

	enum dmx2_main_filter_type type;
	union {
		struct dmx2_ts_filter_params ts;
		struct dmx2_tlv_filter_params tlv;
		struct dmx2_alp_filter_params alp;
		struct dmx2_ip_filter_params ip;
		struct dmx2_mmtp_filter_params mmtp;
	};
};

struct dmx2_filter {
	struct dvb_demux2 *parent;
	int index;
	enum dvb_dmx2_state state;

	struct dmx2_feed *feed;
	struct dmx2_filter *next, *sibling;
	u32 edge_num;

	struct dvb_fh *fh;
	struct dmx2_filter_params params;

	struct list_head list;

	void *priv;
};

struct dmx2_feed {
	struct dvb_demux2 *parent;
	int index;
	enum dvb_dmx2_state state;

	struct dmx2_filter *filter;
	struct dmx2_buffer_ctx *buf_ctx;

	dmx2_feed_cb callback;

	struct list_head list;
	struct mutex mutex;

	void *priv;
};

struct dmx2_feed_operations {
	int (*start)(struct dmx2_feed *feed);
	int (*stop)(struct dmx2_feed *feed);
	int (*flush)(struct dmx2_feed *feed);
};

struct dvb_demux2 {
	enum dmx2_protocol_format protocol;
	struct dmx2_filter_caps filter_caps[DMX2_FILTER_TYPE_MAX];

	struct dmx2_feed_operations *feed_ops;

	void *priv;

	int users;
	int filternum;
	int feednum;

	struct list_head filter_list;	// dmx2_filter, all using filters
	struct list_head feed_list;		// dmx2_feed, all using feed

	/* struct list_head frontend_list; */

	struct mutex mutex;
	spinlock_t lock;
};

int dvb_dmx2_init(struct dvb_demux2 *demux);
int dvb_dmx2_exit(struct dvb_demux2 *demux);

int dvb_dmx2_filter_open(
	struct dvb_demux2 *demux,
	struct dvb_fh *fh,
	struct dmx2_filter_params *param,
	struct dmx2_filter *filter);
int dvb_dmx2_filter_start(struct dmx2_filter *filter);
int dvb_dmx2_filter_stop(struct dmx2_filter *filter);
int dvb_dmx2_filter_flush(struct dmx2_filter *filter);
int dvb_dmx2_filter_close(struct dmx2_filter *filter);
int dvb_dmx2_filter_set_buffer(struct dmx2_filter *filter, struct dmx2_buffer_ctx *buf_ctx);

struct dmx2_filter *dvb_dmx2_filter_find(struct dvb_demux2 *demux, struct dvb_fh *fh);
struct dmx2_feed *dvb_dmx2_feed_find(struct dvb_demux2 *demux, u16 filter_id);

#endif /* DVB_DEMUX2_H */
