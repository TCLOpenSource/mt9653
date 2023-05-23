/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _INPUT_CLI_SVC_MSG_H_
#define _INPUT_CLI_SVC_MSG_H_

#ifndef build_assert				// usage: build_assert(sizeof(int)==4);
#define build_assert(cond)			build_assert_at_line(cond, __LINE__)
#define build_assert_at_line(cond, line)	build_assert_to_line(cond, line)
#define build_assert_to_line(cond, line)	build_assert_msg(cond, at_line_##line)
#define build_assert_msg(cond, msg)		typedef char build_assert_##msg[(!!(cond))*2-1]
#endif

// because we create endpoint(device) to endpoint(device) communication, we do
// not need to specify target device in op message

//msg_id
enum input_cmd_t {
	INPUT_SVCMSG_OPEN = 0,
	INPUT_SVCMSG_SUSPEND = 1,
	INPUT_SVCMSG_RESUME = 2,
	INPUT_SVCMSG_GET_WAKEUPKEY = 3,
	INPUT_SVCMSG_SET_WAKEUPKEY = 4,
	INPUT_SVCMSG_CONFIG_WAKEUP = 5,
	//...
	INPUT_SVCMSG_MAX,
};


#define INPUT_CLIMSG_ACK				(0x0)
#define INPUT_CLIMSG_KEY_EVENT				(0x1)
#define INPUT_CLIMSG_GETKEY_EVENT			(0x2)

#define INPUT_SVCMSG_PAYLOAD_SIZE			(11)
#define INPUT_CLIMSG_PAYLOAD_SIZE			(11)

//-------------------------------------------------------------------------------------------------
// Linux => PMU
// [op][parameter...]
//-------------------------------------------------------------------------------------------------
struct input_svcmsg {
	uint8_t msg_id;
	uint8_t payload[INPUT_SVCMSG_PAYLOAD_SIZE];
};
struct input_svcmsg_open {
	uint8_t msg_id;
};
build_assert(sizeof(struct input_svcmsg_open) <= sizeof(struct input_svcmsg));

struct input_svcmsg_suspend {
	uint8_t msg_id;
};
build_assert(sizeof(struct input_svcmsg_suspend) <= sizeof(struct input_svcmsg));

struct input_svcmsg_resume {
	uint8_t msg_id;
};
build_assert(sizeof(struct input_svcmsg_resume) <= sizeof(struct input_svcmsg));


struct input_svcmsg_config_wakeup {
		uint8_t msg_id;
		uint8_t enable;
};
build_assert(sizeof(struct input_svcmsg_config_wakeup) <= sizeof(struct input_svcmsg));


//-------------------------------------------------------------------------------------------------
// PMU => Linux
// ACK:   [INPUT_CLIMSG_ACK][ret_val]
// EVENT: [INPUT_CLIMSG_TX_*]
// EVENT: [INPUT_CLIMSG_RX_DONE][input_msg]
// EVENT: [INPUT_EVENT_TX*|INPUT_EVENT_RX_DONE][input_msg]
//-------------------------------------------------------------------------------------------------

struct input_climsg {
	uint8_t msg_id;
	uint8_t payload[INPUT_CLIMSG_PAYLOAD_SIZE];
};

struct input_climsg_suspend {
	uint8_t msg_id;		//=INPUT_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct input_climsg_suspend) <= sizeof(struct input_climsg));

struct input_climsg_resume {
	uint8_t msg_id;		//=INPUT_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct input_climsg_resume) <= sizeof(struct input_climsg));

struct input_climsg_event {
	uint8_t msg_id;			//=INPUT_CLIMSG_KEY_EVENT
	uint8_t pressed;		//1: pressed, 0: release
	uint8_t reserved[2];
	uint32_t keycode;
};
build_assert(sizeof(struct input_climsg_event) <= sizeof(struct input_climsg));

struct input_climsg_config_wakeup {
		uint8_t  msg_id;		//=INPUT_CLIMSG_ACK
		uint32_t ret;
};
build_assert(sizeof(struct input_climsg_config_wakeup) <= sizeof(struct input_climsg));

struct input_climsg_get_wakeupkey {
	uint8_t msg_id;			//=INPUT_CLIMSG_ACK
	uint32_t ret;
	uint32_t keycode;
};
build_assert(sizeof(struct input_climsg_get_wakeupkey) <= sizeof(struct input_climsg));

// keep sizeof(struct input_climsg) <= 12 for save memory
build_assert(sizeof(struct input_climsg) <= 12);
#endif
