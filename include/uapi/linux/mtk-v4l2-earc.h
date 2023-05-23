/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __UAPI_MTK_V4L2_EARC_H__
#define __UAPI_MTK_V4L2_EARC_H__

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */
#define	_BIT(shifts) (1 << (shifts))


#define V4L2_CID_EXE_EARC_OFFSET 0xE000
#define V4L2_CID_EXE_EARC_BASE	        (V4L2_CID_USER_BASE+V4L2_CID_EXE_EARC_OFFSET)
#define V4L2_CID_EARC_SUPPORT	        (V4L2_CID_EXE_EARC_BASE+1)
#define V4L2_CID_EARC_STATUS	        (V4L2_CID_EXE_EARC_BASE+2)
#define V4L2_CID_EARC_CAPABILITY	    (V4L2_CID_EXE_EARC_BASE+3)
#define V4L2_CID_EARC_LATENCY_INFO	    (V4L2_CID_EXE_EARC_BASE+4)
#define V4L2_CID_EARC_PORT_SEL	        (V4L2_CID_EXE_EARC_BASE+5)
#define V4L2_CID_EARC_SUPPORT_MODE	    (V4L2_CID_EXE_EARC_BASE+6)
#define V4L2_CID_EARC_ARC_PIN	        (V4L2_CID_EXE_EARC_BASE+7)
#define V4L2_CID_EARC_HEARTBEAT_STATUS	(V4L2_CID_EXE_EARC_BASE+8)
#define V4L2_CID_EARC_LATENCY	        (V4L2_CID_EXE_EARC_BASE+9)
#define V4L2_CID_EARC_STUB	            (V4L2_CID_EXE_EARC_BASE+10)
#define V4L2_CID_EARC_DIFF_DRIV	        (V4L2_CID_EXE_EARC_BASE+11)
#define V4L2_CID_EARC_DIFF_SKEW	        (V4L2_CID_EXE_EARC_BASE+12)
#define V4L2_CID_EARC_COMM_DRIV	        (V4L2_CID_EXE_EARC_BASE+13)
#define V4L2_CID_EARC_DIFF_ONOFF	    (V4L2_CID_EXE_EARC_BASE+14)
#define V4L2_CID_EARC_COMM_ONOFF	    (V4L2_CID_EXE_EARC_BASE+15)

/************************************************
 *********************EVENTs TYPE**********************
 ************************************************
 */
//EARC
#define V4L2_EARC_EVENT_START			    (V4L2_EVENT_PRIVATE_START | 0xB000)
#define V4L2_EVENT_EARC_STATUS_CHANGE       (V4L2_EARC_EVENT_START + 0x1)
//#define V4L2_EVENT_EARC_CAPIBILITY_CHANGE   (V4L2_EARC_EVENT_START | 0x2)
//#define V4L2_EVENT_EARC_LATENCY_CHANGE      (V4L2_EARC_EVENT_START | 0x3)



/* EARC EVENT ID */
enum v4l2_ext_earc_event_id {
	V4L2_EARC_CTRL_EVENT_STATUS = 0,
	V4L2_EARC_CTRL_EVENT_MAX,
};

//CHANGES
#define V4L2_EVENT_CTRL_CH_EARC_BASE                    0x8000
#define V4L2_EVENT_CTRL_CH_EARC_CONNECT_STATUS       (V4L2_EVENT_CTRL_CH_EARC_BASE+_BIT(0))
#define V4L2_EVENT_CTRL_CH_EARC_CAPIBILITY           (V4L2_EVENT_CTRL_CH_EARC_BASE+_BIT(1))
#define V4L2_EVENT_CTRL_CH_EARC_LATENCY              (V4L2_EVENT_CTRL_CH_EARC_BASE+_BIT(2))


#define CAPIBILITY_NUM 256

enum v4l2_earc_support_port {
	V4L2_EARC_PORT_0 = 0,
	V4L2_EARC_PORT_1,
	V4L2_EARC_PORT_2,
	V4L2_EARC_PORT_3,
	V4L2_EARC_PORT_NUM,
};

enum v4l2_earc_support_mode {
	V4L2_EARC_SUPPORT_NONE = 0,
	V4L2_EARC_SUPPORT_EARC,
	V4L2_EARC_SUPPORT_ARC,
	V4L2_EARC_SUPPORT_NUM,
};

enum v4l2_earc_heartbeat_status {
	V4L2_EARC_HDMI_HPD = 0,
	V4L2_EARC_CAP_CHNG_CONF,
	V4L2_EARC_STAT_CHNG_CONF,
	V4L2_EARC_VALID,
};

struct v4l2_ext_earc_info {
	__u32 version;
	__u32 size;
	__u8 u8IsSupportEarc;
	__u8 u8EarcConnectState;
	enum v4l2_earc_support_port enSupportEarcPort;
	__u8 u8EarcLatency;
	__u8 u8EarcCapibility[CAPIBILITY_NUM];
	__u8 u8EarcStub;
} __attribute__ ((__packed__));

struct v4l2_ext_earc_config {
	__u32 version;
	__u32 size;
	enum v4l2_earc_support_port enSupportEarcPort;
	enum v4l2_earc_support_mode enEarcSupportMode;
	__u8 u8Enable5VDetectInvert;
	__u8 u8EnableHPDInvert;
	__u8 u8SetEarcLatency;
	__u8 u8SetHeartBeatStatus;
	__u8 u8OverWriteEnable;
	__u8 u8SetValue;
	__u8 u8SetArcPinEnable;	//Todo
	__u8 u8SetStubEnable;
} __attribute__ ((__packed__));

struct v4l2_ext_earc_hw_config {
	__u32 version;
	__u32 size;
	__u8 u8DifferentialDriveStrength;
	__u8 u8DifferentialSkew;
	__u8 u8CommonDriveStrength;
	__u8 u8DifferentialOnOff;
	__u8 u8CommonOnOff;
} __attribute__ ((__packed__));


#endif
