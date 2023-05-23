/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _CEC_CLI_SVC_MSG_H_
#define _CEC_CLI_SVC_MSG_H_


#ifndef build_assert				// usage: build_assert(sizeof(int)==4);
#define build_assert(cond)					build_assert_at_line(cond, __LINE__)
#define build_assert_at_line(cond, line)	build_assert_to_line(cond, line)
#define build_assert_to_line(cond, line)	build_assert_msg(cond, at_line_##line)
#define build_assert_msg(cond, msg)		typedef char build_assert_##msg[(!!(cond))*2-1]
#endif

// because we create endpoint(device) to endpoint(device) communication, we do
// not need to specify target device in op message

//msg_id
#define CEC_SVCMSG_BASE						(0x80)
#define CEC_SVCMSG_ENABLE					(CEC_SVCMSG_BASE+1)
#define CEC_SVCMSG_LOG_ADDR					(CEC_SVCMSG_BASE+2)
#define CEC_SVCMSG_TRANSMIT					(CEC_SVCMSG_BASE+3)
#define CEC_SVCMSG_SUSPEND					(CEC_SVCMSG_BASE+4)
#define CEC_SVCMSG_RESUME					(CEC_SVCMSG_BASE+5)
#define CEC_SVCMSG_SET_POWERON_MSG				(CEC_SVCMSG_BASE+6)
#define CEC_SVCMSG_ENABLE_POWERON_MSG				(CEC_SVCMSG_BASE+7)
#define CEC_SVCMSG_ENABLE_FACTORY_PIN				(CEC_SVCMSG_BASE+8)
#define CEC_SVCMSG_CONFIG_WAKEUP				(CEC_SVCMSG_BASE+9)



#define CEC_CLIMSG_ACK						(0x0)
#define CEC_CLIMSG_TX_DONE					(0x1)
#define CEC_CLIMSG_TX_NACK					(0x2)
#define CEC_CLIMSG_TX_ERROR					(0x4)
#define CEC_CLIMSG_RX_DONE					(0x8)


#define CEC_MSG_PAYLOAD_SIZE				(31)

struct cec_msg_t {
	uint16_t length;
	uint8_t msg[CEC_MAX_MSG_SIZE];
};

//-------------------------------------------------------------------------------------------------
// Linux => PMU
// [op][parameter...]
//-------------------------------------------------------------------------------------------------
struct cec_generic_msg {
	uint8_t msg_id;
	uint8_t payload[CEC_MSG_PAYLOAD_SIZE];
};

struct cec_svcmsg_enable {
	uint8_t msg_id;
	uint32_t enable;
	uint32_t client;
};
build_assert(sizeof(struct cec_svcmsg_enable) <= sizeof(struct cec_generic_msg));

struct cec_svcmsg_factory_pin_enable {
	uint8_t msg_id;
	uint32_t enable;
	uint32_t client;
};
build_assert(sizeof(struct cec_svcmsg_factory_pin_enable) <= sizeof(struct cec_generic_msg));

struct cec_svcmsg_log_addr {
	uint8_t msg_id;
	uint32_t addr;
	uint32_t client;
};
build_assert(sizeof(struct cec_svcmsg_log_addr) <= sizeof(struct cec_generic_msg));

struct cec_svcmsg_transmit {
	uint8_t msg_id;
	uint8_t attempts;
	struct cec_msg_t cec_msg;
	uint32_t client;
};
build_assert(sizeof(struct cec_svcmsg_transmit) <= sizeof(struct cec_generic_msg));

struct cec_svcmsg_suspend {
	uint8_t msg_id;
};
build_assert(sizeof(struct cec_svcmsg_suspend) <= sizeof(struct cec_generic_msg));

struct cec_svcmsg_resume {
	uint8_t msg_id;
};
build_assert(sizeof(struct cec_svcmsg_resume) <= sizeof(struct cec_generic_msg));

struct cec_svcmsg_set_poweron_msg {
	uint8_t msg_id;
	struct cec_msg_t cec_msg;
};
build_assert(sizeof(struct cec_svcmsg_set_poweron_msg) <= sizeof(struct cec_generic_msg));

struct cec_svcmsg_enable_poweron_msg {
	uint8_t msg_id;
	uint32_t enable;
};
build_assert(sizeof(struct cec_svcmsg_enable_poweron_msg) <= sizeof(struct cec_generic_msg));

struct cec_svcmsg_config_wakeup {
	uint8_t msg_id;
	uint32_t enable;
};
build_assert(sizeof(struct cec_svcmsg_config_wakeup) <= sizeof(struct cec_generic_msg));

//-------------------------------------------------------------------------------------------------
// PMU => Linux
// ACK:   [CEC_CLIMSG_ACK][ret_val]
// EVENT: [CEC_CLIMSG_TX_*]
// EVENT: [CEC_CLIMSG_RX_DONE][cec_msg]
// EVENT: [CEC_EVENT_TX*|CEC_EVENT_RX_DONE][cec_msg]
//-------------------------------------------------------------------------------------------------

struct cec_climsg_enable {
	uint8_t msg_id;		//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_enable) <= sizeof(struct cec_generic_msg));

struct cec_climsg_factory_pin_enable {
	uint8_t msg_id;		//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_factory_pin_enable) <= sizeof(struct cec_generic_msg));

struct cec_climsg_log_addr {
	uint8_t msg_id;		//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_log_addr) <= sizeof(struct cec_generic_msg));

struct cec_climsg_transmit {
	uint8_t msg_id;		//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_transmit) <= sizeof(struct cec_generic_msg));

struct cec_climsg_suspend {
	uint8_t msg_id;		//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_suspend) <= sizeof(struct cec_generic_msg));

struct cec_climsg_resume {
	uint8_t msg_id;		//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_resume) <= sizeof(struct cec_generic_msg));

struct cec_climsg_set_poweron_msg {
	uint8_t msg_id;		//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_set_poweron_msg) <= sizeof(struct cec_generic_msg));

struct cec_climsg_enable_poweron_msg {
	uint8_t  msg_id;	//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_enable_poweron_msg) <= sizeof(struct cec_generic_msg));

struct cec_climsg_config_wakeup {
	uint8_t  msg_id;	//=CEC_CLIMSG_ACK
	uint32_t ret;
};
build_assert(sizeof(struct cec_climsg_config_wakeup) <= sizeof(struct cec_generic_msg));

struct cec_climsg_event {
	uint8_t msg_id;	//=CEC_CLIMSG_TX_* or CEC_CLIMSG_RX_DONE or both
	struct cec_msg_t msg;
};
build_assert(sizeof(struct cec_climsg_event) <= sizeof(struct cec_generic_msg));
#endif
