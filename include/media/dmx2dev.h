/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef DMX2DEV_H
#define DMX2DEV_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mutex.h>

#include <media/dvbdev.h>
#include <media/dvb_ringbuffer.h>
#include <media/dvb_vb2_v2.h>
#include <media/dvb_fh.h>
#include <media/dvb_ctrls.h>

#define fh_to_dmx2dev(fh)	\
	container_of(fh->mgr, struct dmx2dev, fh_mgr)

enum dmx2_memory_type {
	DMX2_MEMORY_RINGBUF,
	DMX2_MEMORY_VB2,
	DMX2_MEMORY_MAX
};

struct dmx2_buffer_ctx {
	enum dmx2_memory_type type;
	union {
		struct dvb_ringbuffer ring;
		struct dvb2_vb2_ctx vb2;
	};
};

struct dmx2dev {
	struct dvb_device *dvbdev;
	struct dvb_device *dvr_dvbdev;

	const struct file_operations *dmx_fops;
	const struct file_operations *dvr_fops;

	/* unsigned int may_do_mmap:1; */
	unsigned int exit:1;
	unsigned int use_fh:1;

	/* struct dmx_frontend *dvr_orig_fe; */

	struct mutex mutex;

	struct dvb_fh_mgr fh_mgr;
	struct dvb_ctrl_handler ctrl_hdl;
};

int dvb_dmx2dev_init(struct dmx2dev *dmx2dev, struct dvb_adapter *adap);
void dvb_dmx2dev_exit(struct dmx2dev *dmx2dev);

#endif /* DMX2DEV_H */
