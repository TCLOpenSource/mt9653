/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef DVB_DVR2_H
#define DVB_DVR2_H

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/bits.h>

#include <linux/dvb/dmx2.h>

#include <media/dvb_fh.h>

#define DVB_DVR2_USERS_MAX		10
#define DVB_DVR2_BUFFER_SIZE	(10*188*1024)

struct dvb_dvr2;
struct dmx2_buffer_ctx;

enum dvr2_protocol_format {
	DVR2_FORMAT_NONE,
	DVR2_FORMAT_M2TS = BIT(0),
	DVR2_FORMAT_ISDB3 = BIT(1),
	DVR2_FORMAT_ATSC3 = BIT(2),
};

enum dvr2_buf_ctx_type {
	DVR2_BUF_CTX_TYPE_IN,
	DVR2_BUF_CTX_TYPE_OUT,
	DVR2_BUF_CTX_TYPE_MAX
};

enum dvb_dvr2_state {
	DVR2_STATE_FREE,
	DVR2_STATE_ALLOCATED,
	DVR2_STATE_GO
};

typedef int (*dvb_dvr2_cb)(
	const u8 *buf1, size_t len1,
	const u8 *buf2, size_t len2,
	struct dvb_dvr2 *src, u32 *buf_flags);

struct dvr2_operations {
	int (*start)(struct dvb_dvr2 *dvr);
	int (*stop)(struct dvb_dvr2 *dvr);
	int (*flush)(struct dvb_dvr2 *dvr);
	int (*write)(struct dvb_dvr2 *dvr);
};

struct dvb_dvr2 {
	enum dvr2_protocol_format protocol;
	int index;
	enum dvb_dvr2_state state;

	struct dvr2_operations *ops;

	struct dvb_fh *fh;
	struct dmx2_buffer_ctx *buf_ctx[DVR2_BUF_CTX_TYPE_MAX];

	dvb_dvr2_cb callback;

	struct mutex mutex;
	spinlock_t lock;

	int users;

	void *priv;
};

int dvb_dvr2_init(struct dvb_dvr2 *dvr);
int dvb_dvr2_exit(struct dvb_dvr2 *dvr);

int dvb_dvr2_open(struct dvb_dvr2 *dvr, struct dvb_fh *fh);
int dvb_dvr2_start(struct dvb_dvr2 *dvr);
int dvb_dvr2_stop(struct dvb_dvr2 *dvr);
int dvb_dvr2_flush(struct dvb_dvr2 *dvr);
int dvb_dvr2_write(struct dvb_dvr2 *dvr);
int dvb_dvr2_close(struct dvb_dvr2 *dvr);

int dvb_dvr2_set_buffer(
	struct dvb_dvr2 *dvr,
	enum dvr2_buf_ctx_type type,
	struct dmx2_buffer_ctx *buf_ctx);

#endif /* DVB_DVR2_H */
