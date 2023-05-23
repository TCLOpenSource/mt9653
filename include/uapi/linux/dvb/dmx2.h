/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef DMX2_H
#define DMX2_H

#include <stdbool.h>
#include <linux/compiler.h>
#include <linux/types.h>

#define DMX2_FILTER_SIZE			16

/* DMX events */
#define DMX2_EVENT_ALL				0
#define DMX2_EVENT_BUFFER_STATUS		1
#define DMX2_EVENT_PRIVATE_BASE			0x08000000

/* DMX subscribe flags */
#define DMX2_EVENT_SUB_FL_SEND_INITIAL	(1 << 0)

/* DMX control id */
#define DMX2_CID_BASE				0x00000001
#define DMX2_CID_PRIVATE_BASE			0x10000000

/* Control flags  */
#define DMX2_CTRL_FLAG_DISABLED			0x00000001
#define DMX2_CTRL_FLAG_GRABBED			0x00000002
#define DMX2_CTRL_FLAG_READ_ONLY		0x00000004
#define DMX2_CTRL_FLAG_WRITE_ONLY		0x00000008
#define DMX2_CTRL_MAX_DIMS			4

enum dmx2_input {
	DMX2_IN_FRONTEND,
	DMX2_IN_DVR,
	DMX2_IN_FILTER /* Demux input from an internal filter */
};

enum dmx2_output {
	DMX2_OUT_DECODER,
	/*
	 * output data to memory from demux filter
	 * and user may need to get metadata by using "DMX2_CID_GET_METADATA"
	 */
	DMX2_OUT_TAP,
	DMX2_OUT_DEMUX_TAP,	/* output data to memory from demux filter with main type header */
	DMX2_OUT_DVR_TAP,
	DMX2_OUT_FILTER,	/* Demux output to an internal filter directly */
	DMX2_OUT_NONE
};

enum dmx2_ts_data_type {
	/* with ts header */
	DMX2_TS_DATA_ALL,
	/* without ts header, for payload througth */
	DMX2_TS_DATA_PAYLOAD,
	/* without ts header, with PES header */
	DMX2_TS_DATA_PES,
	/* dmx2_section_data_params */
	DMX2_TS_DATA_SECTION,
	/* update PCR HW and ouptut PES header */
	DMX2_TS_DATA_PCR,
	/* video es, unit: AU */
	DMX2_TS_DATA_AU_VIDEO,
	/* audio es, unit: AU */
	DMX2_TS_DATA_AU_AUDIO,
	/* private use */
	DMX2_TS_DATA_PRIVATE_BASE = 0xffff
};

enum dmx2_tlv_data_type {
	/* with tlv header */
	DMX2_TLV_DATA_ALL,
	/* without TLV header, for payload througth */
	DMX2_TLV_DATA_PAYLOAD,
	/* signaling data */
	DMX2_TLV_DATA_SIGNALING,
	/* private use */
	DMX2_TLV_DATA_PRIVATE_BASE = 0xffff
};

enum dmx2_alp_data_type {
	/* with alp header */
	DMX2_ALP_DATA_ALL,
	/* without alp header, for payload througth */
	DMX2_ALP_DATA_PAYLOAD,
	/* signaling data */
	DMX2_ALP_DATA_SIGNALING,
	/* private use */
	DMX2_ALP_DATA_PRIVATE_BASE = 0xffff
};

enum dmx2_alp_length_type {
	/* Length does NOT include additional header. Used in US region. */
	DMX2_ALP_WITHOUT_ADDITIONAL_HEADER,
	/* Length includes additional header. Used in Korea region. */
	DMX2_ALP_WITH_ADDITIONAL_HEADER,
};

enum dmx2_ip_data_type {
	/* with IP header */
	DMX2_IP_DATA_ALL,
	/* without IP header, with UDP header */
	DMX2_IP_DATA_UDP_PAYLOAD,
	/* NTP */
	DMX2_IP_DATA_NTP,
	/* private use */
	DMX2_IP_DATA_PRIVATE_BASE = 0xffff
};

enum dmx2_mmtp_data_type {
	/* with mmtp header, for app data */
	DMX2_MMTP_DATA_ALL,
	/* without mmtp header, for payload througth */
	DMX2_MMTP_DATA_PAYLOAD,
	/* without mmtp/MPU header, with MFU header */
	DMX2_MMTP_DATA_MPU,
	/* message data */
	DMX2_MMTP_DATA_MESSAGE,
	/* video es, unit: AU */
	DMX2_MMTP_DATA_AU_VIDEO,
	/* audio es, unit: AU */
	DMX2_MMTP_DATA_AU_AUDIO,
	/* private use */
	DMX2_MMTP_DATA_PRIVATE_BASE = 0xffff
};

enum dmx2_buffer_flags {
	DMX2_BUFFER_FLAG_HAD_CRC32_DISCARD		= 1 << 0,
	DMX2_BUFFER_FLAG_TEI				= 1 << 1,
	DMX2_BUFFER_PKT_COUNTER_MISMATCH		= 1 << 2,
	DMX2_BUFFER_FLAG_DISCONTINUITY_DETECTED		= 1 << 3,
	DMX2_BUFFER_FLAG_DISCONTINUITY_INDICATOR	= 1 << 4,
};

enum dmx2_memory {
	DMX2_MEMORY_MMAP = 1, /* default */
	DMX2_MEMORY_USERPTR = 2,
	DMX2_MEMORY_DMABUF = 4,
};

struct dmx2_sec_filter {
	__u8  filter[DMX2_FILTER_SIZE];
	__u8  mask[DMX2_FILTER_SIZE];
	__u8  mode[DMX2_FILTER_SIZE];
};

struct dmx2_video_data_params {
	__u32 codec_type;
};

struct dmx2_audio_data_params {
	__u32 codec_type;
};

struct dmx2_section_data_params {
	struct dmx2_sec_filter section_bits;
	__u32 timeout;
	__u32 flags;
#define DMX2_CHECK_CRC	1
#define DMX2_ONESHOT	2
};

/* TS filter */
struct dmx2_ts_filter_params {
	__u16 pid;
	enum dmx2_ts_data_type data_type;
	union {
		struct dmx2_section_data_params section;
		struct dmx2_video_data_params video;
		struct dmx2_audio_data_params audio;
		__u8 private_param[48];
		__u32 reserved[12];
	};
	__u32 flags;
#define DMX2_IMMEDIATE_START	1
};

/* TLV filter */
struct dmx2_tlv_filter_params {
	/* 0x01 - IPv4 packet
	 * 0x02 - IPv6 packet
	 * 0x03 - IP packet with header compression
	 * 0xFE - Signalling packet
	 */
	__u8 packet_type;
#define DMX2_TLV_PACKET_TYPE_IP_V4		0x1
#define DMX2_TLV_PACKET_TYPE_IP_V6		0x2
#define DMX2_TLV_PACKET_TYPE_COMPRESSED_IP	0x3
#define DMX2_TLV_PACKET_TYPE_SIGNALING		0xFE
	enum dmx2_tlv_data_type data_type;
	union {
		struct dmx2_section_data_params section;
		__u8 private_param[48];
		__u32 reserved[12];
	};
	__u32 flags;
#define DMX2_IMMEDIATE_START	1
};

/* ALP filter */
struct dmx2_alp_filter_params {
	/*
	 * 0x0 - IPv4
	 * 0x2 - Compressed IP
	 * 0x4 - Signaling
	 */
	__u8 packet_type;
#define DMX2_ALP_PACKET_TYPE_IP_V4		0
#define DMX2_ALP_PACKET_TYPE_COMPRESSED_IP	2
#define DMX2_ALP_PACKET_TYPE_SIGNALING		4
	enum dmx2_alp_length_type length_type;
	enum dmx2_alp_data_type data_type;
	__u8  plp_id;
	__u8  sid;
	union {
		struct dmx2_section_data_params signalling;
		__u8 private_param[48];
		__u32 reserved[12];
	};
	__u32 flags;
#define DMX2_IMMEDIATE_START	1
#define DMX2_PLP_FILTER_EN	2
#define DMX2_SID_FILTER_EN	4
};

/* IP filter */
struct dmx2_ip_addr {
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

struct dmx2_ip_filter_params {
	struct dmx2_ip_addr ip_addr;
	enum dmx2_ip_data_type data_type;
	union {
		struct dmx2_section_data_params signalling; /* for ALP */
		__u8 private_param[48];
		__u32 reserved[12];
	};
	__u32 flags;
#define DMX2_IMMEDIATE_START	1
#define DMX2_HEADER_CRC_CHECK	2 /* check CRC field in IP header and drop error unit */
};

/* MMTP filter */
struct dmx2_mmtp_filter_params {
	__u16 pid;
	enum dmx2_mmtp_data_type data_type;
	union {
		struct dmx2_section_data_params message;
		struct dmx2_video_data_params video;
		struct dmx2_audio_data_params audio;
		__u8 private_param[48];
		__u32 reserved[12];
	};
	__u32 flags;
#define DMX2_IMMEDIATE_START	1
};

/* general filter */
struct dmx2_filter_settings {
	enum dmx2_input input;
	int fd; /* if input == DMX2_IN_FILTER, fill the filter fd */
	enum dmx2_output output;
	union {
		struct dmx2_ts_filter_params ts;
		struct dmx2_tlv_filter_params tlv;
		struct dmx2_alp_filter_params alp;
		struct dmx2_ip_filter_params ip;
		struct dmx2_mmtp_filter_params mmtp;
		__u32 reserved[32];
	};
};

/* capabilities */
struct dmx2_device_caps {
	char match[32];			// [IN] Match the name of caps from client user.
	bool supported;			// [OUT] return if the capability query supported or not.
	union {
		char cap[32];		// [OUT] The corresponding capability value.
		__u32 flags;		// [OUT] flags
	};
} __attribute__ ((packed));

struct dmx2_capability {
	__u32	capabilities;	// [OUT]  DMX2_CAP_TS_FILTER, ...
	__u32	device_caps;	// [OUT] device specific caps by flags
	__u32	count;			// [IN] the number of struct dmx2_device_caps
	union {
		/* [OUT] device specific caps by stream */
		struct dmx2_device_caps *device_info;
		__u32 reserved[2];
	};
} __attribute__ ((packed));

/* Values for 'capabilities' field */
#define DMX2_CAP_IN_MEMORY	0x00000001  /* Is a memory input supported */
#define DMX2_CAP_IN_FRONTEND	0x00000002  /* Is a frontend input supported */
#define DMX2_CAP_OUT_MEMORY	0x00000004  /* Is a memory input supported */
//0x8 reserved
#define DMX2_CAP_TS_FILTER	0x00000010  /* Is a TS filter supported */
#define DMX2_CAP_ALP_FILTER	0x00000020  /* Is a TLV filter supported */
#define DMX2_CAP_TLV_FILTER	0x00000040  /* Can ALP filter supported */
#define DMX2_CAP_IP_FILTER	0x00000080  /* Can IP filter supported */
#define DMX2_CAP_MMTP_FILTER	0x00000100  /* Can MMTP filter supported */

#define DMX2_CAP_STREAMING	0x00001000  /* streaming I/O ioctls supported */
#define DMX2_CAP_DEVICE_CAPS	0x80000000  /* sets device capabilities field */

/* DMX controls */
struct dmx2_queryctrl {
	__u32	id;
	char	name[32];
	__s64	minimum;
	__s64	maximum;
	__u64	step;
	__s64	default_value;
	__u32	flags;
	__u32	elem_size;
	__u32	elems;
	__u32	nr_of_dims;
	__u32	dims[DMX2_CTRL_MAX_DIMS];
	__u32	reserved[7];
};

struct dmx2_control {
	__u32 id;
	__u32 size;
	union {
		__s32 value;
		__s64 value64;
		char __user *string;
		__u8 __user *p_u8;
		__u16 __user *p_u16;
		__u32 __user *p_u32;
		void __user *ptr;
	};
} __attribute__ ((packed));

struct dmx2_controls {
	__u32 count;
	__u32 error_idx;
	__u32 reserved[2];
	struct dmx2_control *controls;
};

/* DMX events */
struct dmx2_event {
	__u32		type;
	__u32		mask;
	union {
		__u32	flags;
		__u8	data[64];
	} u;
	__u32		pending;
	__u32		sequence;
	__u32		reserved[5];
};

struct dmx2_event_subscription {
	__u32		type;
	__u32		mask;
	__u32		flags;
	__u32		reserved[5];
};

/* DMX buffer ctrls */
struct dmx2_requestbuffers {
	__u32	count;
	__u32	size;
	__u32	memory;	/* enum dmx2_memory */
	__u32	reserved[2];
};

struct dmx2_exportbuffer {
	__u32		index;
	__u32		flags;
	__s32		fd;
};

struct dmx2_buffer {
	__u32				index;
	__u32				bytesused;
	union {
		__u32			offset;  /* DMX2_MEMORY_MMAP */
		__u64			userptr; /* DMX2_MEMORY_USERPTR */
		__s32			fd;	 /* DMX2_MEMORY_DMABUF */
	} m;
	__u32				length;
	__u32				flags;
	__u32				count;
};

struct dmx2_stc {
	unsigned int base;
	__u64 stc;
};

struct dmx2_audio_description {
	__u8 fade;
	__u8 pan;
	__u8 version_text_tag;
	__u8 gain_center;
	__u8 gain_front;
	__u8 gain_surround;
};

struct dmx2_mpu_metadata {
	__u32 length;
	__u32 mpu_seq_numb;
};

struct dmx2_ip_payload_metadata {
	__u32 length;
};

struct dmx2_ts_video_metadata {
	__u32 data_offset;
	bool pts_present;
	__u64 timestamp;
	bool secure;
	__u32 reserved[4];
};

struct dmx2_mmtp_video_metadata {
	__u32 data_offset;
	bool secure;
	__u32 mpu_seq_numb;
	__u32 reserved[4];
};

struct dmx2_ts_audio_metadata {
	__u32 data_offset;
	bool pts_present;
	__u64 timestamp;
	bool ad_present;
	struct dmx2_audio_description ad;
	__u32 reserved[4];
};

struct dmx2_mmtp_audio_metadata {
	__u32 data_offset;
	__u32 mpu_seq_numb;
	__u32 reserved[4];
};

// control ID
/* Get the metadata corresponding to the data type. non blcking, not metadatat return -1 */
#define DMX2_CID_GET_METADATA	(DMX2_CID_BASE+1)

#define DMX2_QUERYCAP		_IOWR('o', 65, struct dmx2_capability)
#define DMX2_SET_BUFFER_SIZE	_IO('o', 66)
#define DMX2_GET_STC			_IOWR('o', 67, struct dmx2_stc)

#define DMX2_SET_TS_FILTER	_IOW('o', 68, struct dmx2_filter_settings)
#define DMX2_SET_TLV_FILTER	_IOW('o', 69, struct dmx2_filter_settings)
#define DMX2_SET_ALP_FILTER	_IOW('o', 70, struct dmx2_filter_settings)
#define DMX2_SET_IP_FILTER	_IOW('o', 71, struct dmx2_filter_settings)
#define DMX2_SET_MMTP_FILTER	_IOW('o', 72, struct dmx2_filter_settings)

#define DMX2_START		_IO('o', 73)
#define DMX2_STOP		_IO('o', 74)
#define DMX2_FLUSH		_IO('o', 75)

#define DMX2_SET_CTRL		_IOWR('o', 76, struct dmx2_control)
#define DMX2_GET_CTRL		_IOWR('o', 77, struct dmx2_control)
#define DMX2_SET_CTRLS		_IOWR('o', 78, struct dmx2_controls)
#define DMX2_GET_CTRLS		_IOWR('o', 79, struct dmx2_controls)
#define DMX2_QUERYCTRL		_IOWR('o', 80, struct dmx2_queryctrl)

#define DMX2_DQEVENT		_IOR('o', 81, struct dmx2_event)
#define DMX2_SUBSCRIBE_EVENT	_IOW('o', 82, struct dmx2_event_subscription)
#define DMX2_UNSUBSCRIBE_EVENT	_IOW('o', 83, struct dmx2_event_subscription)

#define DMX2_REQBUFS		_IOWR('o', 84, struct dmx2_requestbuffers)
#define DMX2_QUERYBUF		_IOWR('o', 85, struct dmx2_buffer)
#define DMX2_EXPBUF		_IOWR('o', 86, struct dmx2_exportbuffer)
#define DMX2_QBUF		_IOWR('o', 87, struct dmx2_buffer)
#define DMX2_DQBUF		_IOWR('o', 88, struct dmx2_buffer)


#endif /* DMX2_H */
