/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef DMX_MTK_H
#define DMX_MTK_H

#include <linux/types.h>
#include "dmx.h"

// flags extension of struct dmx_pes_filter_params
#define DMX_ES_OUT			8

typedef struct dmx_caps {
	__u32 caps;
#define DMX_CAPS_SUPPORT_FILEIN			1
#define DMX_CAPS_MAX_DVR_REQBUF_SIZE	2
	/* int num_decoders; */
	__u32 value;
} dmx_caps_t;

typedef enum dmx_source {
	DMX_SOURCE_FRONT0		= 0,//internal demod
	DMX_SOURCE_FRONT1,			//external demod
	DMX_SOURCE_FRONT2,
	DMX_SOURCE_FRONT3,
	DMX_SOURCE_DVR0			= 16,
	DMX_SOURCE_DVR1,
	DMX_SOURCE_DVR2,
	DMX_SOURCE_DVR3,
	DMX_SOURCE_CI0			= 32,
	DMX_SOURCE_CI1,
	DMX_SOURCE_SERIAL_IN	= 0x80000000	// INI parser ? DTS ?
} dmx_source_t;

typedef enum {
	DMX_MODULE_TYPE_TSIO,
	DMX_MODULE_TYPE_PVR,
	DMX_MODULE_TYPE_MAX
} dmx_module_type;

typedef enum {
	DMX_MODULE_STATE_IDLE,
	DMX_MODULE_STATE_BUSY,
	DMX_MODULE_STATE_PAUSE,
	DMX_MODULE_STATE_MAX
} dmx_module_state;

typedef enum {
	DMX_PKT_TYPE_M2TS,
	DMX_PKT_TYPE_M2TS_TIMESTAMP,
	DMX_PKT_TYPE_TLV,
	DMX_PKT_TYPE_ALP,
	DMX_PKT_TYPE_ALP_TIMESTAMP,
	DMX_PKT_TYPE_MAX
} dmx_pkt_type;

typedef enum {
	DVR_PLAY_MODE_NORMAL,
	DVR_PLAY_MODE_BYPASS_TIMESTAMP,
	DVR_PLAY_MODE_MAX
} dvr_play_mode;

typedef enum {
	DVR_REC_MODE_NORMAL,
	DVR_REC_MODE_ALL,
	DVR_REC_MODE_MAX
} dvr_rec_mode;

typedef enum {
	DVR_ENCRYPT_TYPE_NONE,
	DVR_ENCRYPT_TYPE_PAYLOAD,
	DVR_ENCRYPT_TYPE_BLOCK,
	DVR_ENCRYPT_TYPE_MAX
} dvr_encrypt_type;

typedef enum {
	DVR_VIDEO_CODEC_TYPE_NONE,
	DVR_VIDEO_CODEC_TYPE_MPEG,
	DVR_VIDEO_CODEC_TYPE_AVC,
	DVR_VIDEO_CODEC_TYPE_HEVC,
	DVR_VIDEO_CODEC_TYPE_MAX
} dvr_video_codec_type;

typedef enum {
	DVR_TIMESTAMP_TYPE_REC_TIME,
	DVR_TIMESTAMP_TYPE_PLAY_TIME,
	DVR_TIMESTAMP_TYPE_PKT_TIME,
	DVR_TIMESTAMP_TYPE_MAX
} dvr_timestamp_type;

typedef enum {
	DMX_ZAPPING_ENABLE,
	DMX_ZAPPING_DISABLE,
	DMX_ZAPPING_TRIGGER,
	DMX_ZAPPING_MAX
} dmx_zapping_op;

typedef enum {
	DMX_RUSH_MODE_NORMAL,
	DMX_RUSH_MODE_SEARCH_BY_TIME,
	DMX_RUSH_MODE_MAX
} dmx_rush_mode;

typedef enum {
	DMX_FILTER_TYPE_PID,
	DMX_FILTER_TYPE_PCR,
	DMX_FILTER_TYPE_MAX
} dmx_filter_type;

struct dmx_state {
	dmx_module_type module_type;
	__u16 id;
	dmx_module_state state;
};

struct dvr_play_config {
	dmx_pkt_type playtype;
	dvr_play_mode playmode;
};


//	@length:	size in bytes of the buffer
struct dvr_rec_config {
	dmx_pkt_type rectype;
	dvr_rec_mode recmode;
	dvr_encrypt_type encrypt_type;
	dvr_video_codec_type vcodec_type;
	__u32 length;
};

struct dvr_timeinfo {
	dvr_timestamp_type type;
	__u64 timestamp;
};

struct dmx_zapping_info {
	dmx_zapping_op op;
	union {
		dmx_rush_mode mode;
		__u64 rush_param; // ms
	} param;
};

struct dmx_filter_info {
	dmx_filter_type type;
	__u32 filter_id;
};

// extension section filter for TLV-SI, MMTP-SI, ALP-SI
// (*) only MMTP-SI use pid field
struct dmx_sec_ext_filter_params {
	struct dmx_sct_filter_params sec;
	__u32 type;
#define TLV_SI		1
#define MMTP_SI		2
};

struct dmx_ip_addr {
	__u8 version; // use IPv4 or v6
#define IPv4		4
#define IPv6		6
	union {
		__u8 v4[4];
		__u8 v6[16];
	} src_ip_addr;

	union {
		__u8 v4[4];
		__u8 v6[16];
	} dst_ip_addr;
	__u16 src_port;
	__u16 dst_port;
};

// IP filter
struct dmx_ip_filter_params {
	struct dmx_ip_addr ip_addr;
	__u16 cid; // only used for compressed form
	__u32 flags;
#define COMPRESSED		1
#define PAYLOAD_THROUGH 2
};

// MMTP filter
struct dmx_mmtp_filter_params {
	struct dmx_pes_filter_params pes;
	__u32 stream_type;
#define MMTP_STREAM		1
#define MPU_STREAM		2
#define ES_STREAM		4
	__u32 stream_id;
	__u32 flags;
#define COMPRESSED		1
#define PAYLOAD_THROUGH	2
#define DOWNLOAD		4
};

// ALP keyword/timestamp/sideband
struct dmx_alp_stream_key_words_params {
	__u8 key_words[4];
	__u32 flags;
#define KEY_WORD_EN         1
#define KEY_WORD_4B_EN      2
#define TIMESTAMP_EN        4
#define SIDEBAND_EN         8
};

struct dmx_parser_entry {
	__u8  version;
	__u8  reserved;
	__u16 struct_size;
	__u32 flag;
	__u64 pts;
	__u32 es_section_offset;
	__u32 es_section_length;
	__u32 mpu_sequence_number;
	__u32 s32Fd;
	__u32 parser_id;
	__u64 dataid;
};

struct dmx_alp_stream_params {
	__u8  plp_id;
	__u8  sid;
	__u32 flags;
#define PLP_ID_FILTER_EN    1
#define SINGLE_PLP_MODE_EN  2
#define SID_FILTER_EN       4
};

struct dmx_time_params {
	__u32 protocol;
#define TIME_NTP	1
#define TIME_PTP	2
	__u32 time_precision;
#define PREC_27MHz	1
#define PREC_16MHz	2
#define PREC_90kHz	3
};

//DMX_EVENT_ALL can only be used with UNSUBSCRIBE_EVENT for unsubscribing all events at once
#define DMX_EVENT_ALL				0
#define DMX_EVENT_PRIVATE_START		0x08000000

// Use dmx_event_sc_index for mask, dqevent.data[] use dmx_event_sc_info.
#define DMX_EVENT_SC_INDEX			(DMX_EVENT_PRIVATE_START + 0)
// Use dmx_event_sc_hevc_index and dqevent.data[] use dmx_event_sc_info.
#define DMX_EVENT_SC_HEVC_INDEX		(DMX_EVENT_PRIVATE_START + 0)

// General Key frame info for all video codec type
enum dmx_event_sc_index {
	DMX_I_FRAME = 1 << 0,	// Start Code is for a new I Frame
	DMX_P_FRAME = 1 << 1,	// Start Code is for a new P Frame
	DMX_B_FRAME = 1 << 2,	// Start Code is for a new B Frame
	DMX_SEQUENCE = 1 << 3,	// Start Code is for a new Sequence
};

// Key frame info for all HEVC codec type
enum dmx_event_sc_hevc_index {
	DMX_SPS = 1 << 0,
	DMX_AUD = 1 << 1,
	DMX_SLICE_CE_BLA_W_LP = 1 << 2,
	DMX_SLICE_BLA_W_RADL = 1 << 3,
	DMX_SLICE_BLA_N_LP = 1 << 4,
	DMX_SLICE_IDR_W_RADL = 1 << 5,
	DMX_SLICE_IDR_N_LP = 1 << 6,
	DMX_SLICE_TRAIL_CRA = 1 << 7,
};

struct dmx_event_sc_info {
	// HEVC use dmx_event_sc_hevc_index, others use dmx_event_sc_index.
	__u32 sc_index;
	// Byte number from beginning of the filter's output
	__u64 byte_number;
	// pts
	__u64 pts;
	// flags
	__u32 flags;
#define DMX_EVENT_SC_INFO_END	0x1
};

struct dmx_event {
	__u32		type;
	__u32		mask;
	union {
		__u8	data[64];
	} u;
	__u32		pending;
	__u32		sequence;
	__u32		reserved[5];
};

struct dmx_event_subscription {
	__u32	type;
	__u32	mask;
	__u32	flags;
	__u32	reserved[5];
};

#define DMX_IOCTL_BASE				80

#define DMX_SET_STC					_IOW('o', 80, struct dmx_stc)
#define DMX_GET_CAPS				_IOR('o', 81, dmx_caps_t)
#define DMX_SET_SOURCE				_IOW('o', 82, dmx_source_t)
#define DMX_RESET					_IO('o', 83)
#define DMX_PAUSE					_IO('o', 84)
#define DMX_RESUME					_IO('o', 85)
#define DMX_GET_STATE				_IOR('o', 86, struct dmx_state)
#define DVR_SET_PLAY_CONFIG			_IOW('o', 87, struct dvr_play_config)
#define DVR_SET_REC_CONFIG			_IOW('o', 88, struct dvr_rec_config)
#define DVR_SET_TIME_FILTER			_IOW('o', 89, struct dvr_timeinfo)
#define DVR_GET_TIME_FILTER			_IOR('o', 90, struct dvr_timeinfo)
#define DMX_FAST_ZAPPING			_IOW('o', 91, struct dmx_zapping_info)
#define DMX_GET_HW_FILTER_ID		_IOR('o', 92, struct dmx_filter_info)
#define DMX_SET_SEC_EXT_FILTER		_IOW('o', 93, struct dmx_sec_ext_filter_params)
#define DMX_SET_IP_FILTER			_IOW('o', 94, struct dmx_ip_filter_params)
#define DMX_SET_MMTP_FILTER			_IOW('o', 95, struct dmx_mmtp_filter_params)
#define DMX_GET_PAS_ENTRY			_IOR('o', 96, struct dmx_parser_entry)
#define DMX_SET_ALP_STREAM_FILTER	_IOW('o', 97, struct dmx_alp_stream_params)
#define DMX_SET_NTP_PTP_FILTER		_IOW('o', 98, struct dmx_time_params)
#define DMX_DQEVENT					_IOR('o', 99, struct dmx_event)
#define DMX_SUBSCRIBE_EVENT			_IOW('o', 100, struct dmx_event_subscription)
#define DMX_UNSUBSCRIBE_EVENT		_IOW('o', 101, struct dmx_event_subscription)

#define DMX_IOCTL_END			127

#endif /* DMX_MTK_H */
