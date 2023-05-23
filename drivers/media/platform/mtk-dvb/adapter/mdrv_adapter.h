/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MDRV_ADAPTER_H
#define MDRV_ADAPTER_H

#define MAX_PROPERTY_DATA_SIZE  32

enum dvbdev_link_state {
	DVBDEV_LINK_STATE_DISCONNECT,
	DVBDEV_LINK_STATE_CONNECT
};

struct dvbdev_link {
	struct dvb_device *remote;
	enum dvbdev_link_state state;
};

struct dvbdev_property {
	int result;
	u32 cmd;
	union {
		u32 value;
		u8 data[MAX_PROPERTY_DATA_SIZE];
		struct {
			void *buf;
			u32 len;
		} buffer;
	};
};

struct dvbdev_properties {
	u32 num;
	struct dvbdev_property *props;
};

struct dvbdev_notifier {
	int (*link_notify)(struct dvb_device *dev, struct dvbdev_link *link);
	int (*query_notify)(struct dvb_device *dev, struct dvbdev_properties *prop);
};

struct dvb_adapter *mdrv_get_dvb_adapter(void);

int mdrv_adapter_register_device_notifier(
	struct dvb_device *dev, struct dvbdev_notifier *notifier);

int mdrv_adapter_query_device_params(
	struct dvb_device *dev, struct dvbdev_properties *props);

#endif /* MDRV_ADAPTER_H */
