/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __UAPI_MTK_V4L2_JPD_H__
#define __UAPI_MTK_V4L2_JPD_H__

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>

typedef enum{
	E_JPD_OUT_FORMAT_ORIGINAL        = 0x0,
	E_JPD_OUT_FORMAT_YC_SWAP         = 0x1,
	E_JPD_OUT_FORMAT_UV_SWAP         = 0x2,
	E_JPD_OUT_FORMAT_UV_7BIT         = 0x3,
	E_JPD_OUT_FORMAT_UV_MSB          = 0x4,
} NJPD_OUT_FORMAT;

typedef enum{
	E_JPD_VERIFICATION_MODE_NONE            = 0x0,
	E_JPD_VERIFICATION_MODE_IBUF_BRUST      = 0x1,
	E_JPD_VERIFICATION_MODE_NO_RESET_TABLE  = 0x2,
} NJPD_VERIFICATION_MODE;

struct v4l2_jpd_resource_parameter {
	__u32 priority;		/* Smaller value means higher priority */
} __attribute__ ((packed));

struct v4l2_jpd_output_format {
	NJPD_OUT_FORMAT format;       /*1.yc swap 2.uv swap 3.uv 7-bit 4.uv msb*/
} __attribute__ ((packed));

struct v4l2_jpd_verification_mode {
	NJPD_VERIFICATION_MODE mode;       /*1.Ibuf burst 2.no reset table*/
} __attribute__ ((packed));

/* Mediatek JPD private event */
#define V4L2_EVENT_MTK_JPD_START	(V4L2_EVENT_PRIVATE_START + 0x00003000)
#define V4L2_EVENT_MTK_JPD_ERROR	(V4L2_EVENT_MTK_JPD_START + 1)

#define V4L2_CTRL_CLASS_JPEG					0x009d0000
#define V4L2_CID_JPD_CLASS_BASE					(V4L2_CTRL_CLASS_JPEG | 0x1900)

// set before qbuf and get after dqbuf
#define V4L2_CID_JPD_FRAME_METADATA_FD  (V4L2_CID_JPD_CLASS_BASE + 1)	// [set][get]
// struct v4l2_jpd_resource_parameter
#define V4L2_CID_JPD_ACQUIRE_RESOURCE   (V4L2_CID_JPD_CLASS_BASE + 2)	// [set]
// yc swap, uv swap, uv 7-bit, uv msb
#define V4L2_CID_JPD_OUTPUT_FORMAT      (V4L2_CID_JPD_CLASS_BASE + 3)	// [set]
// Ibuf burst, no reset table
#define V4L2_CID_JPD_VERIFICATION_MODE  (V4L2_CID_JPD_CLASS_BASE + 4)	// [set]


#endif
