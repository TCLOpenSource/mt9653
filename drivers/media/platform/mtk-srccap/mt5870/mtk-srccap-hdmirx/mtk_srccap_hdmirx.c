// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/debugfs.h>
//#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <asm/siginfo.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/videodev2.h>
#include <linux/errno.h>
#include <linux/compat.h>
#include <linux/delay.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-memops.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-common.h>
#include <media/v4l2-subdev.h>

#include "mtk_srccap.h"
#include "mtk_srccap_hdmirx.h"
#include "drv_HDMI.h"

#define HDMIRX_LABEL	"[HDMIRX] "
#define HDMIRX_MSG_DEBUG(format, args...) \
					pr_debug(HDMIRX_LABEL format, ##args)
#define HDMIRX_MSG_INFO(format, args...) \
					pr_info(HDMIRX_LABEL format, ##args)
#define HDMIRX_MSG_WARNING(format, args...) \
					pr_warn(HDMIRX_LABEL format, ##args)
#define HDMIRX_MSG_ERROR(format, args...) \
					pr_err(HDMIRX_LABEL format, ##args)
#define HDMIRX_MSG_FATAL(format, args...) \
					pr_crit(HDMIRX_LABEL format, ##args)

#define DEF_READ_DONE_EVENT 0

#define DEF_TEST_USE	0
bool gHDMIRxInitFlag = FALSE;


static int mtk_hdmirx_ctrl_packet_monitor_task(void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	stCMD_HDMI_GET_PACKET_STATUS stGetPktStatus;
	MS_HDMI_EXTEND_PACKET_RECEIVE_t stPktStatus;
	stCMD_HDMI_GET_AVI_PARSING_INFO stGetAVI;
	stAVI_PARSING_INFO stAVIInfo;
	stCMD_HDMI_GET_GC_PARSING_INFO stGetGC;
	stGC_PARSING_INFO stGCInfo;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	stCMD_HDMI_GET_PACKET_CONTENT stPktContent;
	__u8 u8VSPkt[HDMI_GET_PACKET_LENGTH];
	__u8 u8ALLMMode = 0;
	__u8 u8Index = 0;

	memset(&stGetPktStatus, 0, sizeof(stCMD_HDMI_GET_PACKET_STATUS));
	memset(&stGetAVI, 0, sizeof(stCMD_HDMI_GET_AVI_PARSING_INFO));
	memset(&stPktContent, 0, sizeof(stCMD_HDMI_GET_PACKET_CONTENT));
	memset(u8VSPkt, 0, HDMI_GET_PACKET_LENGTH);
	memset(srccap_dev->hdmirx_info.AVI, 0xFF,
	       (INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0) * sizeof(stAVI_PARSING_INFO));
	memset(srccap_dev->hdmirx_info.bAVMute, 0x00,
	       (INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0) * sizeof(bool));
	srccap_dev->hdmirx_info.u8ALLMMode = 0;

	stGetPktStatus.pExtendPacketReceive = &stPktStatus;
	stGetAVI.pstAVIParsingInfo = &stAVIInfo;
	stGetGC.pstGCParsingInfo = &stGCInfo;
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0;
		     enInputPortType < INPUT_PORT_DVI_MAX; enInputPortType++) {
			memset(&stPktStatus, 0, sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t));
			memset(&stAVIInfo, 0, sizeof(stAVI_PARSING_INFO));
			memset(&stGCInfo, 0, sizeof(stGC_PARSING_INFO));
			memset(&event, 0, sizeof(struct v4l2_event));
			stGetPktStatus.enInputPortType = enInputPortType;
			stGetAVI.enInputPortType = enInputPortType;
			stGetGC.enInputPortType = enInputPortType;
			stPktStatus.u16Size = sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t);
			u8Index = enInputPortType - INPUT_PORT_DVI0;

			if (Drv_HDMI_Ctrl(E_HDMI_INTERFACE_CMD_GET_PACKET_STATUS,
					   &stGetPktStatus, sizeof(stCMD_HDMI_GET_PACKET_STATUS))) {
				if (stPktStatus.bPKT_SPD_RECEIVE) {
					//HDMIRX_MSG_INFO("Receive SPD!\r\n");
					event.u.ctrl.changes |=
					    V4L2_EVENT_CTRL_CH_HDMI_RX_RECEIVE_SPD;
				}

				if (stPktStatus.bPKT_VS_RECEIVE) {
					//HDMIRX_MSG_INFO("Receive VSIF!\r\n");
					event.u.ctrl.changes |=
					    V4L2_EVENT_CTRL_CH_HDMI_RX_RECEIVE_VS;
				}

				if (stPktStatus.bPKT_AUI_RECEIVE) {
					//HDMIRX_MSG_INFO("Receive Audio Info!\r\n");
					event.u.ctrl.changes |=
					    V4L2_EVENT_CTRL_CH_HDMI_RX_RECEIVE_AUI;
				}

				if (stPktStatus.bPKT_MPEG_RECEIVE) {
					//HDMIRX_MSG_INFO("Receive MPEG!\r\n");
					event.u.ctrl.changes |=
					    V4L2_EVENT_CTRL_CH_HDMI_RX_RECEIVE_MPEG;
				}
			}

			if (Drv_HDMI_Ctrl(E_HDMI_INTERFACE_CMD_GET_GC_PARSING_INFO,
					   &stGetGC, sizeof(stCMD_HDMI_GET_GC_PARSING_INFO))) {
				//HDMIRX_MSG_INFO("Receive GC!\r\n");
				if (srccap_dev->hdmirx_info.bAVMute[u8Index] !=
					stGCInfo.u8ControlAVMute) {
					//HDMIRX_MSG_INFO("AV Mute Changed : %d!\r\n",
						//stGCInfo.u8ControlAVMute);
					srccap_dev->hdmirx_info.bAVMute[u8Index] =
					stGCInfo.u8ControlAVMute;
					event.u.ctrl.changes |=
					    V4L2_EVENT_CTRL_CH_HDMI_RX_AVMUTE_CHANGE;
				}
			}

			if (Drv_HDMI_Ctrl(E_HDMI_INTERFACE_CMD_GET_AVI_PARSING_INFO,
					   &stGetAVI, sizeof(stCMD_HDMI_GET_AVI_PARSING_INFO))) {
				//HDMIRX_MSG_INFO("Receive AVI!\r\n");
				if ((srccap_dev->hdmirx_info.AVI[u8Index].enColorForamt
					!= stAVIInfo.enColorForamt) |
					(srccap_dev->hdmirx_info.AVI[u8Index].enAspectRatio
					!= stAVIInfo.enAspectRatio) |
				(srccap_dev->hdmirx_info.AVI[u8Index].enActiveFormatAspectRatio
					!= stAVIInfo.enActiveFormatAspectRatio)) {
					//HDMIRX_MSG_INFO("AVI Changed!\r\n");
					memcpy(&srccap_dev->hdmirx_info.AVI[u8Index],
					       &stAVIInfo, sizeof(stAVI_PARSING_INFO));
					event.u.ctrl.changes |=
					    V4L2_EVENT_CTRL_CH_HDMI_RX_AVI_CHANGE;
				}
			}
			stPktContent.enInputPortType = enInputPortType;
			stPktContent.enPacketType = PKT_VS;
			stPktContent.u8PacketLength = HDMI_GET_PACKET_LENGTH;
			stPktContent.pu8PacketContent = u8VSPkt;

			if (Drv_HDMI_Ctrl(E_HDMI_INTERFACE_CMD_GET_PACKET_CONTENT,
					   &stPktContent, sizeof(stCMD_HDMI_GET_PACKET_CONTENT))) {
				//HDMIRX_MSG_INFO("Receive VS!\r\n");
				if ((u8VSPkt[7] == 0xC4) && (u8VSPkt[6] == 0x5D)
				    && (u8VSPkt[5] == 0xD8)) {

					if (u8VSPkt[9] & BIT(1))
						u8ALLMMode = 1;
					else
						u8ALLMMode = 0;

					if (srccap_dev->hdmirx_info.u8ALLMMode != u8ALLMMode) {
					//HDMIRX_MSG_INFO("ALLM Changed! : %d\r\n", u8ALLMMode);
						srccap_dev->hdmirx_info.u8ALLMMode = u8ALLMMode;
						event.u.ctrl.changes |=
						    V4L2_EVENT_CTRL_CH_HDMI_RX_ALLM_CHANGE;
					}
				}
			}

			if (event.u.ctrl.changes != 0) {
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_PKT_STATUS;
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(10000, 11000);	// sleep 10ms

		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_cable_detect_monitor_task(void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	__u8 u8Index = 0;

	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0;
		     enInputPortType < INPUT_PORT_DVI_MAX; enInputPortType++) {
			u8Index = enInputPortType - INPUT_PORT_DVI0;
			if (srccap_dev->hdmirx_info.bCableDetect[u8Index]
			    != KDrv_HDMIRx_GetSCDCCableDetectFlag(enInputPortType)) {
				srccap_dev->hdmirx_info.bCableDetect[u8Index]
				= KDrv_HDMIRx_GetSCDCCableDetectFlag(enInputPortType);
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_CABLE_DETECT;
				event.u.ctrl.changes =
				    (srccap_dev->hdmirx_info.bCableDetect[u8Index]
				     ? V4L2_EVENT_CTRL_CH_HDMI_RX_CABLE_CONNECT :
				     V4L2_EVENT_CTRL_CH_HDMI_RX_CABLE_DISCONNECT);
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(10000, 11000);	// sleep 10ms
		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_hdcp2x_irq_monitor_task(void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;

	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = 0;
		     enInputPortType < (INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0); enInputPortType++) {
			if (KDrv_HDCP22_PollingWriteDone(enInputPortType)) {
				//HDMIRX_MSG_ERROR("Get Write Done Status %d\r\n", enInputPortType);
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS;
				event.u.ctrl.changes =
					V4L2_EVENT_CTRL_CH_HDMI_RX_HDCP2X_WRITE_DONE;
				event.u.ctrl.value = enInputPortType + INPUT_PORT_DVI0;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
			#if DEF_READ_DONE_EVENT
			if (Drv_HDCP22_PollingReadDone(enInputPortType)) {
				memset(&event, 0, sizeof(struct v4l2_event));
				//HDMIRX_MSG_ERROR("Get Read Done Status %d\r\n", enInputPortType);
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS;
				event.u.ctrl.changes =
					V4L2_EVENT_CTRL_CH_HDMI_RX_HDCP2X_READ_DONE;
				event.u.ctrl.value = enInputPortType + INPUT_PORT_DVI0;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
			#endif
		}
		usleep_range(100, 110);	// sleep 100us

		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_hdcp1x_irq_monitor_task(void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;

	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = 0;
		     enInputPortType < (INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0); enInputPortType++) {
			if (KDrv_HDCP14_PollingR0ReadDone(enInputPortType)) {
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP1X_STATUS;
				event.u.ctrl.value = enInputPortType + INPUT_PORT_DVI0;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(100, 110); // sleep 100us

		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_hdmi_crc_monitor_task(void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	MS_DVI_CHANNEL_TYPE enChannelType = MS_DVI_CHANNEL_R;
	unsigned short usCrc = 0;
	__u8 u8Index = 0;
	__u8 u8Type = 0;

	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0;
		     enInputPortType < INPUT_PORT_DVI_MAX; enInputPortType++) {
			for (enChannelType = MS_DVI_CHANNEL_R;
			     enChannelType <= MS_DVI_CHANNEL_B; enChannelType++) {
				u8Index = enInputPortType - INPUT_PORT_DVI0;
				u8Type = enChannelType - MS_DVI_CHANNEL_R;
				if (Drv_HDMI_GetCRCValue(enInputPortType, enChannelType, &usCrc)) {
					if (srccap_dev->hdmirx_info.usHDMICrc[u8Index][u8Type]
					    != usCrc) {
						memset(&event, 0, sizeof(struct v4l2_event));
						event.type = V4L2_EVENT_CTRL;
						event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_CRC;
						event.u.ctrl.value = enInputPortType;
						srccap_dev->hdmirx_info.usHDMICrc[u8Index][u8Type]
						    = usCrc;
						v4l2_event_queue(srccap_dev->vdev, &event);
					}
				}
			}
		}
		usleep_range(1000, 1100);	// sleep 1ms

		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_hdmi_mode_monitor_task(void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	__u8 u8Index = 0;

	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0;
		     enInputPortType < INPUT_PORT_DVI_MAX; enInputPortType++) {
			u8Index = enInputPortType - INPUT_PORT_DVI0;
			if (srccap_dev->hdmirx_info.bHDMIMode[u8Index] !=
			    Drv_HDMI_IsHDMI_Mode(enInputPortType)) {
				memset(&event, 0, sizeof(struct v4l2_event));
				srccap_dev->hdmirx_info.bHDMIMode[u8Index] =
				    Drv_HDMI_IsHDMI_Mode(enInputPortType);
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_MODE;
				event.u.ctrl.changes =
				    (srccap_dev->hdmirx_info.bHDMIMode[u8Index] ?
				     V4L2_EVENT_CTRL_CH_HDMI_RX_HDMI_MODE :
				     V4L2_EVENT_CTRL_CH_HDMI_RX_DVI_MODE);
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(10000, 11000);	// sleep 10ms

		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_hdcp_state_monitor_task(void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	__u8 u8Index = 0;
	__u8 u8HDCPState = 0;

	memset(srccap_dev->hdmirx_info.ucHDCPState, 0x00,
		(INPUT_PORT_DVI_MAX - INPUT_PORT_DVI0) *
		sizeof(unsigned char));
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0;
		     enInputPortType < INPUT_PORT_DVI_MAX; enInputPortType++) {
			u8Index = enInputPortType - INPUT_PORT_DVI0;
			u8HDCPState =
				Drv_HDMI_CheckHDCPENCState(enInputPortType);
			if (srccap_dev->hdmirx_info.ucHDCPState[u8Index] !=
			    u8HDCPState) {
				memset(&event, 0, sizeof(struct v4l2_event));
				srccap_dev->hdmirx_info.ucHDCPState[u8Index] =
				    u8HDCPState;
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP_STATE;
				switch (srccap_dev->hdmirx_info.ucHDCPState[u8Index]) {
				case 0:
				default:
					event.u.ctrl.changes =
					V4L2_EXT_HDMI_HDCP_NO_ENCRYPTION;
					break;
				case 1:
					event.u.ctrl.changes =
					V4L2_EXT_HDMI_HDCP_1_4;
					break;
				case 2:
					event.u.ctrl.changes =
					V4L2_EXT_HDMI_HDCP_2_2;
					break;
				}
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(100000, 110000);	// sleep 100ms

		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

void mtk_hdmirx_hdcp_isr(int irq, void *priv)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)priv;
	struct v4l2_event event;
	unsigned char ucHDCPStatusIndex = 0;
	unsigned char ucVersion = 0;

	ucVersion = Drv_HDMI_HDCP_ISR(&ucHDCPStatusIndex);
	if (ucVersion > 0) {
		memset(&event, 0, sizeof(struct v4l2_event));
		event.type = V4L2_EVENT_CTRL;
		switch (ucVersion) {
		case 1:
			event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP1X_STATUS;
			break;
		case 2:
			event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS;
			event.u.ctrl.changes =
				V4L2_EVENT_CTRL_CH_HDMI_RX_HDCP2X_WRITE_DONE;
			//HDMIRX_MSG_ERROR("Get Write Done Status ISR \r\n");
			break;
#if DEF_READ_DONE_EVENT
		case 3:
			event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS;
			event.u.ctrl.changes =
				V4L2_EVENT_CTRL_CH_HDMI_RX_HDCP2X_READ_DONE;
			//HDMIRX_MSG_ERROR("Get Read Done Status ISR \r\n");
			break;
#endif
		default:
			event.id = 0;
			break;
		}

		if (event.id != 0) {
			if (ucHDCPStatusIndex & 0x1) {
				event.u.ctrl.value = INPUT_PORT_DVI0;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
			if (ucHDCPStatusIndex & 0x2) {
				event.u.ctrl.value = INPUT_PORT_DVI1;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
			if (ucHDCPStatusIndex & 0x4) {
				event.u.ctrl.value = INPUT_PORT_DVI2;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
			if (ucHDCPStatusIndex & 0x8) {
				event.u.ctrl.value = INPUT_PORT_DVI3;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}

		//event.u.ctrl.value = ucHDCPStatusIndex;
		//v4l2_event_queue(srccap_dev->vdev, &event);
	}
}

unsigned int mtk_hdmirx_STREventProc(EN_POWER_MODE usPowerState)
{
	return drv_HDMI_STREventProc(usPowerState);
}

int mtk_hdmirx_init(struct mtk_srccap_dev *srccap_dev)
{
	HDMIRX_MSG_INFO("%s HDMI INIT!!.\n", __func__);
	if (srccap_dev != NULL) {
		srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		srccap_dev->hdmirx_info.ctrl_packet_monitor_task = NULL;
		srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task = NULL;
		srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task = NULL;
		srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task = NULL;
		srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task = NULL;
		srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task = NULL;
		srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task = NULL;
	} else
		return -ENOMEM;

	if (!gHDMIRxInitFlag) {
		Drv_HDMI_init();
		gHDMIRxInitFlag = TRUE;
	}
	return 0;
}

int mtk_hdmirx_release(struct mtk_srccap_dev *srccap_dev)
{
	HDMIRX_MSG_INFO("%s HDMI Release!!.\n", __func__);
	if (srccap_dev != NULL) {
		if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
		if (srccap_dev->hdmirx_info.ctrl_packet_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_packet_monitor_task);
			srccap_dev->hdmirx_info.ctrl_packet_monitor_task = NULL;
		}
		if (srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task);
			srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task = NULL;
		}
		if (srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task = NULL;
		}
		if (srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task = NULL;
		}
		if (srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task = NULL;
		}
		if (srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task = NULL;
		}
		if (srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task = NULL;
		}
	} else
		return -ENOMEM;

	return 0;
}

int mtk_hdmirx_subscribe_ctrl_event(struct mtk_srccap_dev *srccap_dev,
				    const struct v4l2_event_subscription *event_sub)
{
	if (event_sub == NULL) {
		HDMIRX_MSG_ERROR("NULL Event !\r\n");
		return -ENXIO;
	}

	switch (event_sub->id) {
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_PKT_STATUS:
		//HDMIRX_MSG_INFO("Subscribe PKT_STATUS !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_packet_monitor_task == NULL) {
			srccap_dev->hdmirx_info.ctrl_packet_monitor_task =
			    kthread_create(mtk_hdmirx_ctrl_packet_monitor_task, srccap_dev,
					   "hdmi ctrl_packet_monitor_task");
			if (srccap_dev->hdmirx_info.ctrl_packet_monitor_task ==
				ERR_PTR(-ENOMEM))
				return -ENOMEM;
			wake_up_process(srccap_dev->hdmirx_info.ctrl_packet_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_CABLE_DETECT:
		//HDMIRX_MSG_INFO("Subscribe CABLE_DETECT !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task == NULL) {
			memset(srccap_dev->hdmirx_info.bCableDetect, 0x00,
			       sizeof(srccap_dev->hdmirx_info.bCableDetect));
			srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task =
			    kthread_create(mtk_hdmirx_ctrl_cable_detect_monitor_task, srccap_dev,
					   "hdmi ctrl_5v_detect_monitor_task");
			if (srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task ==
				ERR_PTR(-ENOMEM))
				return -ENOMEM;
			wake_up_process(srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS:
		//HDMIRX_MSG_INFO("Subscribe HDCP2X_STATUS !\r\n");
		if (!srccap_dev->hdmirx_info.ubHDCP22ISREnable) {
			//HDMIRX_MSG_ERROR("HDCP22 Status Thread Create !\r\n");
			if (srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task == NULL) {
				srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task =
					kthread_create(mtk_hdmirx_ctrl_hdcp2x_irq_monitor_task,
					srccap_dev, "hdmi ctrl_hdcp2x_irq_monitor_task");
				if (srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task ==
					ERR_PTR(-ENOMEM))
					return -ENOMEM;
				wake_up_process(
					srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task);
			}
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP1X_STATUS:
		//HDMIRX_MSG_INFO("Subscribe HDCP1X_STATUS !\r\n");
		if (!srccap_dev->hdmirx_info.ubHDCP14ISREnable) {
			//HDMIRX_MSG_ERROR("HDCP14 Status Thread Create !\r\n");
			if (srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task == NULL) {
				srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task =
				    kthread_create(mtk_hdmirx_ctrl_hdcp1x_irq_monitor_task,
				    srccap_dev, "hdmi ctrl_hdcp1x_irq_monitor_task");
				if (srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task ==
				    ERR_PTR(-ENOMEM))
					return -ENOMEM;
				wake_up_process(
					srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task);
			}
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_CRC:
		//HDMIRX_MSG_INFO("Subscribe HDMI_CRC !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task == NULL) {
			srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task =
			    kthread_create(mtk_hdmirx_ctrl_hdmi_crc_monitor_task, srccap_dev,
					   "hdmi ctrl_hdmi_crc_monitor_task");
			if (srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task ==
				ERR_PTR(-ENOMEM))
				return -ENOMEM;
			wake_up_process(srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_MODE:
		//HDMIRX_MSG_INFO("Subscribe HDMI_MODE !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task == NULL) {
			memset(srccap_dev->hdmirx_info.bHDMIMode, 0x00,
			       sizeof(srccap_dev->hdmirx_info.bHDMIMode));
			srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task =
			    kthread_create(mtk_hdmirx_ctrl_hdmi_mode_monitor_task, srccap_dev,
					   "hdmi ctrl_hdmi_mode_monitor_task");
			if (srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task ==
				ERR_PTR(-ENOMEM))
				return -ENOMEM;
			wake_up_process(srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP_STATE:
		//HDMIRX_MSG_INFO("Subscribe HDCP_STATE !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task == NULL) {
			srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task =
				kthread_create(mtk_hdmirx_ctrl_hdcp_state_monitor_task,
				srccap_dev, "hdmi ctrl_hdcp_state_monitor_task");
			if (srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task
				== ERR_PTR(-ENOMEM))
				return -ENOMEM;
			wake_up_process(
				srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task);
		}
		break;
	default:
		break;
	}

	return 0;
}

int mtk_hdmirx_unsubscribe_ctrl_event(struct mtk_srccap_dev *srccap_dev,
				      const struct v4l2_event_subscription *event_sub)
{
	if (event_sub == NULL) {
		HDMIRX_MSG_ERROR("NULL Event !\r\n");
		return -ENXIO;
	}

	switch (event_sub->id) {
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_PKT_STATUS:
		//HDMIRX_MSG_INFO("UnSubscribe PKT_STATUS !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_packet_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_packet_monitor_task);
			srccap_dev->hdmirx_info.ctrl_packet_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_CABLE_DETECT:
		//HDMIRX_MSG_INFO("UnSubscribe CABLE_DETECT !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task);
			srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS:
		//HDMIRX_MSG_INFO("UnSubscribe HDCP2X_STATUS !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP1X_STATUS:
		//HDMIRX_MSG_INFO("UnSubscribe HDCP1X_STATUS !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_CRC:
		//HDMIRX_MSG_INFO("UnSubscribe HDMI_CRC !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_MODE:
		//HDMIRX_MSG_INFO("UnSubscribe HDMI_MODE !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP_STATE:
		//HDMIRX_MSG_INFO("UnSubscribe HDCP_STATE !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task = NULL;
		}
		break;
	default:
		break;
	}

	return 0;
}

int mtk_hdmirx_set_current_port(unsigned int current_input)
{
	if (Drv_HDMI_Current_Port_Set(current_input))
		return 0;
	else
		return -EINVAL;
}

int mtk_hdmirx_g_edid(struct v4l2_edid *edid)
{
	XC_DDCRAM_PROG_INFO pstDDCRam_Info;
	MS_U8 u8EDID[512];

	if ((edid != NULL) && (edid->edid != NULL)) {
		if ((edid->start_block + edid->blocks) > 4)
			return -EINVAL;

		pstDDCRam_Info.EDID = u8EDID;
		pstDDCRam_Info.u16EDIDSize =
				(edid->start_block + edid->blocks) * 128;
		pstDDCRam_Info.eDDCProgType = edid->pad;
		Drv_HDMI_READ_DDCRAM(&pstDDCRam_Info);
		memcpy(edid->edid, (u8EDID + (edid->start_block*128)),
							edid->blocks * 128);
	} else
		return -EINVAL;

	return 0;
}

int mtk_hdmirx_s_edid(struct v4l2_edid *edid)
{
	XC_DDCRAM_PROG_INFO pstDDCRam_Info;

	if ((edid != NULL) && (edid->edid != NULL)) {
		if (edid->blocks > 4)
			return -E2BIG;

		pstDDCRam_Info.EDID = edid->edid;
		pstDDCRam_Info.u16EDIDSize = edid->blocks * 128;
		pstDDCRam_Info.eDDCProgType = edid->pad;
		Drv_HDMI_PROG_DDCRAM(&pstDDCRam_Info);
	} else
		return -EINVAL;

	return 0;
}

int mtk_hdmirx_g_audio_channel_status(struct v4l2_audio *audio)
{
	if (audio != NULL) {
		// index which byte to get.
		snprintf(audio->name, sizeof(audio->name), "byte %d",
							audio->index);
		audio->index = Drv_HDMI_audio_channel_status(audio->index);
	} else
		return -EINVAL;

	return 0;
}

int mtk_hdmirx_g_dv_timings(struct v4l2_dv_timings *timings)
{
	if (timings != NULL) {
		timings->bt.width = Drv_HDMI_GetDataInfo(E_HDMI_GET_VDE);
		timings->bt.vfrontporch =
				Drv_HDMI_GetDataInfo(E_HDMI_GET_VFRONT);
		timings->bt.vsync = Drv_HDMI_GetDataInfo(E_HDMI_GET_VSYNC);
		timings->bt.vbackporch =
			Drv_HDMI_GetDataInfo(E_HDMI_GET_VTT)
			-timings->bt.width - timings->bt.vfrontporch -
			timings->bt.vsync;
		timings->bt.height = Drv_HDMI_GetDataInfo(E_HDMI_GET_HDE);
		timings->bt.hfrontporch =
			Drv_HDMI_GetDataInfo(E_HDMI_GET_HFRONT);
		timings->bt.hsync = Drv_HDMI_GetDataInfo(E_HDMI_GET_HSYNC);
		timings->bt.hbackporch = Drv_HDMI_GetDataInfo(E_HDMI_GET_HTT)
			- timings->bt.height - timings->bt.hfrontporch -
			timings->bt.hsync;
		timings->bt.interlaced =
			Drv_HDMI_GetDataInfo(E_HDMI_GET_INTERLACE);
		timings->bt.pixelclock =
			Drv_HDMI_GetDataInfo(E_HDMI_GET_PIXEL_CLOCK);
	} else
		return -EINVAL;

	return 0;
}


int mtk_hdmirx_g_fmt_vid_cap(struct v4l2_format *format)
{
	if (format != NULL) {
		MS_HDMI_EXT_COLORIMETRY_FORMAT enColormetry =
						MS_HDMI_EXT_COLOR_UNKNOWN;
		format->fmt.pix.width = Drv_HDMI_GetDataInfo(E_HDMI_GET_VDE);
		format->fmt.pix.height = Drv_HDMI_GetDataInfo(E_HDMI_GET_HDE);
		format->fmt.pix.pixelformat = Drv_HDMI_Get_ColorFormat();
		enColormetry = Drv_HDMI_Get_ExtColorimetry();
		switch (enColormetry) {
		case MS_HDMI_EXT_COLORIMETRY_SMPTE_170M:
			format->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
			break;

		case MS_HDMI_EXT_COLORIMETRY_ITU_R_BT_709:
			format->fmt.pix.colorspace = V4L2_COLORSPACE_REC709;
			format->fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_709;
			break;

		case MS_HDMI_EXT_COLOR_XVYCC601:
			format->fmt.pix.colorspace = V4L2_YCBCR_ENC_XV601;
			break;

		case MS_HDMI_EXT_COLOR_XVYCC709:
			format->fmt.pix.colorspace = V4L2_YCBCR_ENC_XV709;
			break;

		case MS_HDMI_EXT_COLOR_SYCC601:
			format->fmt.pix.colorspace = V4L2_YCBCR_ENC_601;
			break;

		case MS_HDMI_EXT_COLOR_BT2020YcCbcCrc:
		case MS_HDMI_EXT_COLOR_BT2020RGBYCbCr:
			format->fmt.pix.colorspace = V4L2_COLORSPACE_BT2020;
			format->fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_BT2020;
			break;

		default:
			format->fmt.pix.colorspace = V4L2_COLORSPACE_DEFAULT;
			format->fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
			break;
		}
	} else
		return -EINVAL;

	return 0;
}

int mtk_hdmirx_store_color_fmt(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	srccap_dev->hdmirx_info.color_format = Drv_HDMI_Get_ColorFormat();

	return 0;
}

int _mtk_hdmirx_ctrl_process(__u32 u32Cmd, void *pBuf, __u32 u32BufSize)
{
	switch (u32Cmd) {
	case V4L2_EXT_HDMI_INTERFACE_CMD_GET_HDCP_STATE:
	{
		unsigned long ret;
		stCMD_HDMI_CHECK_HDCP_STATE stHDCP_State;

		ret = copy_from_user(&stHDCP_State,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_State,
								u32BufSize);
#if DEF_TEST_USE
			stHDCP_State.ucHDCPState =
					0x11 + stHDCP_State.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP State failed\n"
						"\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
						&stHDCP_State, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user HDCP State failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_INTERFACE_CMD_WRITE_X74:
	{
		unsigned long ret;
		stCMD_HDCP_WRITE_X74 stHDCP_WriteX74;

		ret = copy_from_user(&stHDCP_WriteX74,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_WriteX74,
								u32BufSize);
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Write X74 failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_INTERFACE_CMD_READ_X74:
	{
		unsigned long ret;
		stCMD_HDCP_READ_X74 stHDCP_ReadX74;

		ret = copy_from_user(&stHDCP_ReadX74,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_ReadX74,
								u32BufSize);
#if DEF_TEST_USE
			stHDCP_ReadX74.ucRetData =
				0x11 + stHDCP_ReadX74.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Read X74 failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
					&stHDCP_ReadX74, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Read X74 failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_INTERFACE_CMD_SET_REPEATER:
	{
		unsigned long ret;
		stCMD_HDCP_SET_REPEATER stHDCP_SetRepeater;

		ret = copy_from_user(&stHDCP_SetRepeater,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_SetRepeater,
								u32BufSize);
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP Set Repeater\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_SET_BSTATUS:
	{
		unsigned long ret;
		stCMD_HDCP_SET_BSTATUS stHDCP_SetBstatus;

		ret = copy_from_user(&stHDCP_SetBstatus,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_SetBstatus,
								u32BufSize);
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP Set BStatus\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_SET_HDMI_MODE:
	{
		unsigned long ret;
		stCMD_HDCP_SET_HDMI_MODE stHDMI_SetMode;

		ret = copy_from_user(&stHDMI_SetMode,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0)
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDMI_SetMode,
								u32BufSize);
		else {
			HDMIRX_MSG_ERROR("copy_from_user Set HDMI Mode\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_INTERRUPT_STATUS:
	{
		unsigned long ret;
		stCMD_HDCP_GET_INTERRUPT_STATUS stHDCP_IRQStatus;

		ret = copy_from_user(&stHDCP_IRQStatus,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_IRQStatus,
								u32BufSize);
#if DEF_TEST_USE
			stHDCP_IRQStatus.ucRetIntStatus =
				0x11 + stHDCP_IRQStatus.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP IRQ Status\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
					&stHDCP_IRQStatus, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user HDCP IRQ Status\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_WRITE_KSV_LIST:
	{
	//"Untrusted value as argument"
	// Passing tainted variable "stHDCP_KSVList.ulLen" to a tainted sink.

		unsigned long ret;
		stCMD_HDCP_WRITE_KSV_LIST stHDCP_KSVList;
		__u8 *u8KSVList = NULL;

		ret = copy_from_user(&stHDCP_KSVList,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);

		if (ret == 0) {
			if ((stHDCP_KSVList.ulLen > 0) && (stHDCP_KSVList.ulLen <= 635)) {
				u8KSVList = kmalloc(stHDCP_KSVList.ulLen,
								GFP_KERNEL);
				if (u8KSVList == NULL) {
					HDMIRX_MSG_ERROR("Allocate KSVList\n"
							"Memory Failed!\r\n");
					return -ENOMEM;
				}
				ret = copy_from_user(u8KSVList,
				compat_ptr(ptr_to_compat(
				stHDCP_KSVList.pucKSV)), stHDCP_KSVList.ulLen);

				if (ret == 0) {
					stHDCP_KSVList.pucKSV = u8KSVList;
					Drv_HDMI_Ctrl(u32Cmd,
					(void *)&stHDCP_KSVList, u32BufSize);
					kfree(u8KSVList);
				} else {
					HDMIRX_MSG_ERROR("copy_from_user KSV\n"
							"List failed\r\n");
					kfree(u8KSVList);
					return -EINVAL;
				}

			} else {
				HDMIRX_MSG_ERROR("KSV Length is < 0\r\n");
				return -EINVAL;
			}
		} else {
			HDMIRX_MSG_ERROR("copy_from_user KSV Struct\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_SET_VPRIME:
	{
		unsigned long ret;
		stCMD_HDCP_SET_VPRIME stHDCP_Vprime;
		__u8 *u8Vprime = NULL;

		ret = copy_from_user(&stHDCP_Vprime,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			u8Vprime = kmalloc(HDMI_HDCP_VPRIME_LENGTH, GFP_KERNEL);
			if (u8Vprime == NULL) {
				HDMIRX_MSG_ERROR("Allocate Vprime Memory\n"
							"Failed!\r\n");
				return -ENOMEM;
			}

			ret = copy_from_user(u8Vprime, compat_ptr(
				ptr_to_compat(stHDCP_Vprime.pucVPrime)),
				HDMI_HDCP_VPRIME_LENGTH);

			if (ret == 0) {
				stHDCP_Vprime.pucVPrime = u8Vprime;
				Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_Vprime,
								u32BufSize);
				kfree(u8Vprime);
			} else {
				HDMIRX_MSG_ERROR("copy_from_user Vprime\n"
							"failed\r\n");
				kfree(u8Vprime);
				return -EINVAL;
			}
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Vprime Struct\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_DATA_RTERM_CONTROL:
	{
		unsigned long ret;
		stCMD_HDMI_DATA_RTERM_CONTROL stData_Rterm;

		ret = copy_from_user(&stData_Rterm,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0)
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stData_Rterm,
								u32BufSize);
		else {
			HDMIRX_MSG_ERROR("copy_from_user Data Rterm\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_SCDC_VALUE:
	{
		unsigned long ret;
		stCMD_HDMI_GET_SCDC_VALUE stSCDC_Value;

		ret = copy_from_user(&stSCDC_Value,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stSCDC_Value,
								u32BufSize);
#if DEF_TEST_USE
			stSCDC_Value.ucRetData =
					0x11 + stSCDC_Value.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user SCDC Value\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
						&stSCDC_Value, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user SCDC Value failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_TMDS_RATES_KHZ:
	{
		unsigned long ret;
		stCMD_HDMI_GET_TMDS_RATES_KHZ stTMDS_Rates;

		ret = copy_from_user(&stTMDS_Rates,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stTMDS_Rates,
								u32BufSize);
#if DEF_TEST_USE
			stTMDS_Rates.ulRetRates =
					0x11 + stTMDS_Rates.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user TMDS Rates\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
						&stTMDS_Rates, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user TMDS Rates failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_CABLE_DETECT:
	{
		unsigned long ret;
		stCMD_HDMI_GET_SCDC_CABLE_DETECT stCable_Detect;

		ret = copy_from_user(&stCable_Detect,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stCable_Detect,
								u32BufSize);
#if DEF_TEST_USE
			stCable_Detect.bCableDetectFlag = TRUE;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Cable Detect\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
						&stCable_Detect, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Cable Detect\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_PACKET_STATUS:
	{
		unsigned long ret;
		stCMD_HDMI_GET_PACKET_STATUS stPkt_Status;
		MS_HDMI_EXTEND_PACKET_RECEIVE_t stPktStatus;
		MS_HDMI_EXTEND_PACKET_RECEIVE_t *pstUser_PktStatus;

		ret = copy_from_user(&stPkt_Status,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_PktStatus = stPkt_Status.pExtendPacketReceive;
			ret = copy_from_user(&stPktStatus,
				compat_ptr(ptr_to_compat(
				stPkt_Status.pExtendPacketReceive)),
				sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t));
			if (ret == 0) {
				stPkt_Status.pExtendPacketReceive =
							&stPktStatus;
				Drv_HDMI_Ctrl(u32Cmd, (void *)&stPkt_Status,
							u32BufSize);
#if DEF_TEST_USE
				stPkt_Status.pExtendPacketReceive
						->bPKT_GC_RECEIVE = TRUE;
#endif
				ret = copy_to_user(compat_ptr(ptr_to_compat(
				pstUser_PktStatus)),
				stPkt_Status.pExtendPacketReceive,
				sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t));
				if (ret == 0)
					stPkt_Status.pExtendPacketReceive =
							pstUser_PktStatus;
				else {
					HDMIRX_MSG_ERROR("copy_to_user Packet\n"
							"Status failed\r\n");
					return -EINVAL;
				}
			} else {
				HDMIRX_MSG_ERROR("copy_form_user Packet\n"
						"Status failed\r\n");
				return -EINVAL;
			}
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Get Packet Status\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
						&stPkt_Status, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get Packet Status\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_PACKET_CONTENT:
	{
	//"Untrusted value as argument"
	//Passing tainted variable "stPkt_Content.u8PacketLength"
	//						to a tainted sink.

		unsigned long ret;
		stCMD_HDMI_GET_PACKET_CONTENT stPkt_Content;
		__u8 *u8PktContent = NULL;

		ret = copy_from_user(&stPkt_Content, compat_ptr(
				ptr_to_compat(pBuf)), u32BufSize);
		u8PktContent = stPkt_Content.pu8PacketContent;// get user Addr
		if (ret == 0) {
			if ((stPkt_Content.u8PacketLength > 0) &&
			    (stPkt_Content.u8PacketLength <= 121)) {
				stPkt_Content.pu8PacketContent =
					kmalloc(stPkt_Content.u8PacketLength,
					GFP_KERNEL);
				if (stPkt_Content.pu8PacketContent == NULL) {
					HDMIRX_MSG_ERROR("Allocate Packet\n"
						"Content Memory Failed!\r\n");
					return -ENOMEM;
				}

				Drv_HDMI_Ctrl(u32Cmd, (void *)&stPkt_Content,
								u32BufSize);
#if DEF_TEST_USE
				*stPkt_Content.pu8PacketContent =
					0x11 + stPkt_Content.enInputPortType;
#endif
				ret = copy_to_user(
				compat_ptr(ptr_to_compat(u8PktContent)),
				stPkt_Content.pu8PacketContent,
				stPkt_Content.u8PacketLength);
				if (ret == 0) {
					kfree(stPkt_Content.pu8PacketContent);
					stPkt_Content.pu8PacketContent =
						u8PktContent;
				} else {
					HDMIRX_MSG_ERROR("copy_to_user\n"
						"Packet Content failed\r\n");
					kfree(stPkt_Content.pu8PacketContent);
					return -EINVAL;
				}
			} else {
				HDMIRX_MSG_ERROR("Packet Length Error\r\n");
				return -EINVAL;
			}
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Get Packet Content\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
					&stPkt_Content, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get Packet Content\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_HDR_METADATA:
	{
		unsigned long ret;
		stCMD_HDMI_GET_HDR_METADATA stHDR_GetData;
		sHDR_METADATA stHDR_Meta;
		sHDR_METADATA *pstUser_HDRMeta;

		ret = copy_from_user(&stHDR_GetData,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_HDRMeta = stHDR_GetData.pstHDRMetadata;
			ret = copy_from_user(&stHDR_Meta,
			compat_ptr(
			ptr_to_compat(stHDR_GetData.pstHDRMetadata)),
			sizeof(sHDR_METADATA));
			if (ret == 0) {
				stHDR_GetData.pstHDRMetadata = &stHDR_Meta;
				Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDR_GetData,
								u32BufSize);
#if DEF_TEST_USE
				stHDR_Meta.u8Length =
					0x11 + stHDR_GetData.enInputPortType;
#endif
				ret = copy_to_user(compat_ptr(
				ptr_to_compat(pstUser_HDRMeta)),
				stHDR_GetData.pstHDRMetadata,
				sizeof(sHDR_METADATA));
				if (ret == 0) {
					stHDR_GetData.pstHDRMetadata =
							pstUser_HDRMeta;
				} else {
					HDMIRX_MSG_ERROR("copy_to_user HDR\n"
						"Meta Data failed\r\n");
					return -EINVAL;
				}
			} else {
				HDMIRX_MSG_ERROR("copy_form_user HDR Meta\n"
						"Data failed\r\n");
				return -EINVAL;
			}
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Get HDR failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
					&stHDR_GetData, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get HDR failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_AVI_PARSING_INFO:
	{
		unsigned long ret;
		stCMD_HDMI_GET_AVI_PARSING_INFO stAVI_GetInfo;
		stAVI_PARSING_INFO stAVI_ParseInfo;
		stAVI_PARSING_INFO *pstUser_ParseAVIInfo;

		ret = copy_from_user(&stAVI_GetInfo,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_ParseAVIInfo = stAVI_GetInfo.pstAVIParsingInfo;
			ret = copy_from_user(&stAVI_ParseInfo,
			compat_ptr(ptr_to_compat(
			stAVI_GetInfo.pstAVIParsingInfo)),
			sizeof(stAVI_PARSING_INFO));
			if (ret == 0) {
				stAVI_GetInfo.pstAVIParsingInfo =
							&stAVI_ParseInfo;
				Drv_HDMI_Ctrl(u32Cmd,
					(void *)&stAVI_GetInfo, u32BufSize);
#if DEF_TEST_USE
				stAVI_ParseInfo.u8Length =
				0x11 + stAVI_GetInfo.enInputPortType;
#endif
				ret = copy_to_user(compat_ptr(ptr_to_compat(
				pstUser_ParseAVIInfo)),
				stAVI_GetInfo.pstAVIParsingInfo,
				sizeof(stAVI_PARSING_INFO));
				if (ret == 0)
					stAVI_GetInfo.pstAVIParsingInfo =
							pstUser_ParseAVIInfo;
				else {
					HDMIRX_MSG_ERROR("copy_to_user AVI\n"
						"Parse Info failed\r\n");
					return -EINVAL;
				}
			} else {
				HDMIRX_MSG_ERROR("copy_form_user AVI Parse\n"
						"Info failed\r\n");
				return -EINVAL;
			}
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Get AVI Info\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
						&stAVI_GetInfo, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get AVI Info\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_VS_PARSING_INFO:
	{
		unsigned long ret;
		stCMD_HDMI_GET_VS_PARSING_INFO stVS_GetInfo;
		stVS_PARSING_INFO stVS_Info;
		stVS_PARSING_INFO *pstUser_VSInfo;

		ret = copy_from_user(&stVS_GetInfo, compat_ptr(
					ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_VSInfo = stVS_GetInfo.pstVSParsingInfo;
			ret = copy_from_user(&stVS_Info, compat_ptr(
				ptr_to_compat(stVS_GetInfo.pstVSParsingInfo)),
				sizeof(stVS_PARSING_INFO));
			if (ret == 0) {
				stVS_GetInfo.pstVSParsingInfo = &stVS_Info;
				Drv_HDMI_Ctrl(u32Cmd, (void *)&stVS_GetInfo,
								u32BufSize);
#if DEF_TEST_USE
				stVS_Info.u8Length =
					0x11 + stVS_GetInfo.enInputPortType;
#endif
				ret = copy_to_user(
				compat_ptr(ptr_to_compat(pstUser_VSInfo)),
				stVS_GetInfo.pstVSParsingInfo,
				sizeof(stVS_PARSING_INFO));
				if (ret == 0)
					stVS_GetInfo.pstVSParsingInfo =
							pstUser_VSInfo;
				else {
					HDMIRX_MSG_ERROR("copy_to_user VS\n"
						"Parse Info failed\r\n");
					return -EINVAL;
				}
			} else {
				HDMIRX_MSG_ERROR("copy_form_user VS Parse\n"
						"Info failed\r\n");
				return -EINVAL;
			}
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Get VS Info\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
					&stVS_GetInfo, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get VS Info\n"
					"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_GC_PARSING_INFO:
	{
		unsigned long ret;
		stCMD_HDMI_GET_GC_PARSING_INFO stGC_GetInfo;
		stGC_PARSING_INFO stGC_Info;
		stGC_PARSING_INFO *pstUser_GCInfo;

		ret = copy_from_user(&stGC_GetInfo,
			compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_GCInfo = stGC_GetInfo.pstGCParsingInfo;
			ret = copy_from_user(&stGC_Info, compat_ptr(
				ptr_to_compat(stGC_GetInfo.pstGCParsingInfo)),
				sizeof(stGC_PARSING_INFO));
			if (ret == 0) {
				stGC_GetInfo.pstGCParsingInfo = &stGC_Info;
				Drv_HDMI_Ctrl(u32Cmd, (void *)&stGC_GetInfo,
								u32BufSize);
#if DEF_TEST_USE
				stGC_Info.enColorDepth =
				MS_HDMI_COLOR_DEPTH_8_BIT +
				stGC_GetInfo.enInputPortType - 80;
#endif
				ret = copy_to_user(compat_ptr(
				ptr_to_compat(pstUser_GCInfo)),
				stGC_GetInfo.pstGCParsingInfo,
				sizeof(stGC_PARSING_INFO));
				if (ret == 0)
					stGC_GetInfo.pstGCParsingInfo =
							pstUser_GCInfo;
				else {
					HDMIRX_MSG_ERROR("copy_to_user GC\n"
						"Parse Info failed\r\n");
					return -EINVAL;
				}
			} else {
				HDMIRX_MSG_ERROR("copy_form_user GC Parse\n"
						"Info failed\r\n");
				return -EINVAL;
			}
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Get GC Info\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
					&stGC_GetInfo, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get GC Info failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_TIMING_INFO:
	{
		unsigned long ret;
		stCMD_HDMI_GET_TIMING_INFO stTiming_Info;

		ret = copy_from_user(&stTiming_Info,
			compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stTiming_Info,
								u32BufSize);
#if DEF_TEST_USE
			stTiming_Info.u16TimingInfo = 1080;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Timing Info\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
					&stTiming_Info, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Timing Info\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_HDCP_AUTHVERSION:
	{
		unsigned long ret;
		stCMD_HDMI_GET_HDCP_AUTHVERSION stHDCP_AuthVer;

		ret = copy_from_user(&stHDCP_AuthVer,
				compat_ptr(ptr_to_compat(pBuf)), u32BufSize);
		if (ret == 0) {
			Drv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_AuthVer,
								u32BufSize);
#if DEF_TEST_USE
			stHDCP_AuthVer.enHDCPAuthVersion = E_HDCP_2_2;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP Auth Version\n"
						"failed\r\n");
			return -EINVAL;
		}

		ret = copy_to_user(compat_ptr(ptr_to_compat(pBuf)),
						&stHDCP_AuthVer, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user HDCP Auth Version\n"
						"failed\r\n");
			return -EINVAL;
		}
	}
	break;

	default:
		HDMIRX_MSG_ERROR("Command ID Not Found! \r\n");
	return -EINVAL;
	}

	return 0;
}

static int mtk_hdmirx_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev;
	int ret = 0;

	if (ctrl == NULL) {
		HDMIRX_MSG_ERROR("Get ctrls Error\r\n");
		return -ENOMEM;
	}

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, hdmirx_ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_HDMI_RX_GET_IT_CONTENT_TYPE:
	{
		ctrl->val = Drv_HDMI_Get_Content_Type();
#if DEF_TEST_USE
		ctrl->val = 16;
#endif
	}
	break;

	case V4L2_CID_HDMI_RX_HDCP22_READDONE:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDCP22_READDONE stHDCP_Read;

			memcpy(&stHDCP_Read, srccap_dev->hdmirx_info.ucSetBuffer,
				sizeof(ST_HDCP22_READDONE));
			stHDCP_Read.bReadFlag =
				Drv_HDCP22_PollingReadDone(
				stHDCP_Read.u8PortIdx);
#if DEF_TEST_USE
			stHDCP_Read.bReadFlag = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDCP_Read,
					sizeof(ST_HDCP22_READDONE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_GC_INFO:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_GC_INFO stHDMI_GC;

			memcpy(&stHDMI_GC, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_GC_INFO));
			stHDMI_GC.u16GCdata =
			Drv_HDMI_GC_Info(stHDMI_GC.enInputPortType,
			stHDMI_GC.gcontrol);
#if DEF_TEST_USE
			stHDMI_GC.u16GCdata =
			0x11 + stHDMI_GC.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_GC,
			sizeof(ST_HDMI_GC_INFO));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_ERR_STATUS:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_ERR_STATUS stErr_status;

			memcpy(&stErr_status, srccap_dev->hdmirx_info.ucSetBuffer,
				sizeof(ST_HDMI_ERR_STATUS));
			stErr_status.u8RetValue =
			Drv_HDMI_err_status_update(
			stErr_status.enInputPortType,
			stErr_status.u8value, stErr_status.bread);
#if DEF_TEST_USE
			stErr_status.u8RetValue =
			0x11 + stErr_status.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stErr_status,
					sizeof(ST_HDMI_ERR_STATUS));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_ACP:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BYTE stHDMI_BYTE;

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
			sizeof(ST_HDMI_BYTE));
			stHDMI_BYTE.u8RetValue = Drv_HDMI_audio_cp_hdr_info(
						stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue =
					0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE,
					sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_PIXEL_REPETITION:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BYTE stHDMI_BYTE;

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
						sizeof(ST_HDMI_BYTE));
			stHDMI_BYTE.u8RetValue =
				Drv_HDMI_Get_Pixel_Repetition(
				stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue =
				0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE,
						sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_AVI_VER:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BYTE stHDMI_BYTE;

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
						sizeof(ST_HDMI_BYTE));
			stHDMI_BYTE.u8RetValue =
				Drv_HDMI_Get_AVIInfoFrameVer(
				stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue =
				0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE,
						sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_AVI_ACTIVEINFO:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BOOL stHDMI_Bool;

			memcpy(&stHDMI_Bool, srccap_dev->hdmirx_info.ucSetBuffer,
				sizeof(ST_HDMI_BOOL));
			stHDMI_Bool.bRet =
			Drv_HDMI_Get_AVIInfoActiveInfoPresent(
			stHDMI_Bool.enInputPortType);
#if DEF_TEST_USE
			stHDMI_Bool.bRet = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_Bool,
						sizeof(ST_HDMI_BOOL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_ISHDMIMODE:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BOOL stHDMI_Bool;

			memcpy(&stHDMI_Bool, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_BOOL));
			stHDMI_Bool.bRet =
				Drv_HDMI_IsHDMI_Mode(
				stHDMI_Bool.enInputPortType);
#if DEF_TEST_USE
			stHDMI_Bool.bRet = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_Bool,
				sizeof(ST_HDMI_BOOL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_CHECK_4K2K:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BOOL stHDMI_Bool;

			memcpy(&stHDMI_Bool, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_BOOL));
			stHDMI_Bool.bRet = Drv_HDMI_Check4K2K(
					stHDMI_Bool.enInputPortType);
#if DEF_TEST_USE
			stHDMI_Bool.bRet = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_Bool,
					sizeof(ST_HDMI_BOOL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_CHECK_ADDITION_FMT:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BYTE stHDMI_BYTE;

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_BYTE));
			stHDMI_BYTE.u8RetValue =
			Drv_HDMI_Check_Additional_Format(
			stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue =
			0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE,
					sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_3D_STRUCTURE:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BYTE stHDMI_BYTE;

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_BYTE));
			stHDMI_BYTE.u8RetValue =
				Drv_HDMI_Get_3D_Structure(
				stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue =
			0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE,
				sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_3D_EXT_DATA:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BYTE stHDMI_BYTE;

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_BYTE));
			stHDMI_BYTE.u8RetValue =
				Drv_HDMI_Get_3D_Ext_Data(
				stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue =
				0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE,
				sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_3D_META_FIELD:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_META_FIELD stMeta_field;

			memcpy(&stMeta_field, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_META_FIELD));
			Drv_HDMI_Get_3D_Meta_Field(
				stMeta_field.enInputPortType,
				&stMeta_field.st3D_META_DATA);
#if DEF_TEST_USE
			stMeta_field.st3D_META_DATA.t3D_Metadata_Type =
						E_3D_META_DATA_MAX;
			stMeta_field.st3D_META_DATA.u83D_Metadata_Length =
					0x11 + stMeta_field.enInputPortType;
			stMeta_field.st3D_META_DATA.b3D_Meta_Present =
								TRUE;
#endif
			memcpy(ctrl->p_new.p, &stMeta_field,
					sizeof(ST_HDMI_META_FIELD));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_SOURCE_VER:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BYTE stHDMI_BYTE;

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_BYTE));
			stHDMI_BYTE.u8RetValue =
				Drv_HDMI_GetSourceVersion(
				stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue =
				0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE,
					sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_DE_STABLE_STATUS:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_BOOL stHDMI_Bool;

			memcpy(&stHDMI_Bool, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_BOOL));
			stHDMI_Bool.bRet =
				Drv_HDMI_GetDEStableStatus(
				stHDMI_Bool.enInputPortType);
#if DEF_TEST_USE
			stHDMI_Bool.bRet = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_Bool,
					sizeof(ST_HDMI_BOOL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_CRC:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			stCMD_HDMI_CMD_GET_CRC_VALUE stHDMI_CRC_Value;
			__u16 u16crc = 0;
			unsigned long ret;

			memcpy(&stHDMI_CRC_Value, srccap_dev->hdmirx_info.ucSetBuffer,
				sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE));
			stHDMI_CRC_Value.bIsValid =
				Drv_HDMI_GetCRCValue(
				stHDMI_CRC_Value.enInputPortType,
				stHDMI_CRC_Value.u8Channel, &u16crc);
#if DEF_TEST_USE
			stHDMI_CRC_Value.bIsValid = TRUE;
			u16crc =
			0x11 + stHDMI_CRC_Value.enInputPortType;
#endif
			ret = copy_to_user(compat_ptr(ptr_to_compat(
			stHDMI_CRC_Value.u16CRCVal)),
			&u16crc, sizeof(__u16));

			if (ret != 0) {
				HDMIRX_MSG_ERROR("copy_to_user failed\r\n");
				return -EINVAL;
			}
			memcpy(ctrl->p_new.p, &stHDMI_CRC_Value,
				sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_EMP:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			stCMD_HDMI_GET_EMPacket stHDMI_EMP;
			__u8 u8TotalNum = 0;
			__u8 u8EmpData[31] = {0};
			unsigned long ret = 0;
			bool bret = FALSE;

			memcpy(&stHDMI_EMP, srccap_dev->hdmirx_info.ucSetBuffer,
				sizeof(stCMD_HDMI_GET_EMPacket));
			HDMIRX_MSG_INFO("Input Port : %d, EMP Type : %X\r\n",
				stHDMI_EMP.enInputPortType,
				stHDMI_EMP.enEmpType);
			bret = Drv_HDMI_Get_EMP(
			stHDMI_EMP.enInputPortType,
			stHDMI_EMP.enEmpType,
			stHDMI_EMP.u8CurrentPacketIndex,
			&u8TotalNum,
			u8EmpData);
#if DEF_TEST_USE
			stHDMI_EMP.u8CurrentPacketIndex =
			0x11 + stHDMI_EMP.enInputPortType - 0x50;
			u8TotalNum = stHDMI_EMP.enInputPortType;
			u8EmpData[0] =
				0x11 + stHDMI_EMP.enInputPortType;
			bret = TRUE;
#endif
			if (bret) {
				ret = copy_to_user(compat_ptr(ptr_to_compat(
				stHDMI_EMP.pu8TotalPacketNumber)),
				&u8TotalNum, sizeof(__u8));
			}

			if (ret != 0) {
				HDMIRX_MSG_ERROR("copy_to_user EMP total\n"
							"number failed\r\n");
				return -EINVAL;
			}

			if (bret) {
				ret = copy_to_user(compat_ptr(ptr_to_compat(
					stHDMI_EMP.pu8EmPacket)),
					&u8EmpData, 31*sizeof(__u8));
			}

			if (ret != 0) {
				HDMIRX_MSG_ERROR("copy_to_user EMP data\n"
							"failed\r\n");
				return -EINVAL;
			}
			memcpy(ctrl->p_new.p, &stHDMI_EMP,
				sizeof(stCMD_HDMI_GET_EMPacket));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_CTRL:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMIRX_CTRL stHDMI_CTRL;

			memcpy(&stHDMI_CTRL, srccap_dev->hdmirx_info.ucSetBuffer,
				sizeof(ST_HDMIRX_CTRL));
			ret = _mtk_hdmirx_ctrl_process(stHDMI_CTRL.u32Cmd,
			stHDMI_CTRL.pBuf, stHDMI_CTRL.u32BufSize);
			memcpy(ctrl->p_new.p, &stHDMI_CTRL,
				sizeof(ST_HDMIRX_CTRL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_FS:
	{
		HDMIRX_MSG_ERROR("Get V4L2_CID_HDMI_RX_GET_FS\n");
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDMI_WORD stHDMI_WORD;

			memcpy(&stHDMI_WORD, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDMI_WORD));
			stHDMI_WORD.u16RetValue =
				Drv_HDMI_GetDataInfoByPort(E_HDMI_GET_FS,
				stHDMI_WORD.enInputPortType);
#if DEF_TEST_USE
			stHDMI_WORD.u16RetValue =
				44;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_WORD,
					sizeof(ST_HDMI_WORD));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	case V4L2_CID_HDMI_RX_HDCP22_GETMSG:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
							"first\r\n");
				return -ENOMEM;
			}

			ST_HDCP22_MSG stHDCPMsg;
			MS_U8 ucMsgData[HDMI_HDCP22_MESSAGE_LENGTH + 1] = {0};
			unsigned long ret;

			memcpy(&stHDCPMsg, srccap_dev->hdmirx_info.ucSetBuffer,
					sizeof(ST_HDCP22_MSG));
			Drv_HDCP22_RecvMsg(stHDCPMsg.ucPortIdx, ucMsgData);
			stHDCPMsg.dwDataLen = ucMsgData[129];
#if DEF_TEST_USE
			stHDCPMsg.dwDataLen =
				21;
			ucMsgData[0] = 21;
			ucMsgData[1] = 0x11 + stHDCPMsg.ucPortIdx;
#endif
			if (stHDCPMsg.dwDataLen > 0) {
				ret = copy_to_user(compat_ptr(ptr_to_compat(
				stHDCPMsg.pucData)),
				&ucMsgData, 130*sizeof(__u8));
				if (ret != 0) {
					HDMIRX_MSG_ERROR("copy_to_user HDCP22\n"
								"MSG failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, &stHDCPMsg,
					sizeof(ST_HDCP22_MSG));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
		}
	}
	break;

	default:
		HDMIRX_MSG_ERROR("Wrong CID\r\n");
		return -EINVAL;
	}

	return ret;
}

static int mtk_hdmirx_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev;

	if (ctrl == NULL) {
		HDMIRX_MSG_ERROR("Set ctrls Error\r\n");
		return -ENOMEM;
	}

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, hdmirx_ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_HDMI_RX_SET_CURRENT_PORT:
	{
		Drv_HDMI_Current_Port_Set(ctrl->val);
	}
	break;

	case V4L2_CID_HDMI_RX_HDCP22_READDONE:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDCP22_READDONE),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDCP22_READDONE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_GC_INFO:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_GC_INFO),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_GC_INFO));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_ERR_STATUS:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_ERR_STATUS),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_ERR_STATUS));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_ACP:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BYTE),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
						sizeof(ST_HDMI_BYTE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_PIXEL_REPETITION:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BYTE),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BYTE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_AVI_VER:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BYTE),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BYTE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_AVI_ACTIVEINFO:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BOOL),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BOOL));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_ISHDMIMODE:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BOOL),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BOOL));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_CHECK_4K2K:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BOOL),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BOOL));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_CHECK_ADDITION_FMT:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BYTE),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BYTE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_3D_STRUCTURE:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BYTE),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
					sizeof(ST_HDMI_BYTE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_3D_EXT_DATA:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BYTE),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BYTE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_3D_META_FIELD:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_META_FIELD),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_META_FIELD));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_SOURCE_VER:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BYTE),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BYTE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_DE_STABLE_STATUS:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_BOOL),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_BOOL));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_CRC:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(
				sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE),
				GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_EMP:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(
				sizeof(stCMD_HDMI_GET_EMPacket), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(stCMD_HDMI_GET_EMPacket));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_CTRL:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMIRX_CTRL),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMIRX_CTRL));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_GET_FS:
	{
		HDMIRX_MSG_ERROR("Set V4L2_CID_HDMI_RX_GET_FS\n");
		if (ctrl->p_new.p != NULL) {
			HDMIRX_MSG_ERROR("Not NULL V4L2_CID_HDMI_RX_GET_FS\n");
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDMI_WORD),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDMI_WORD));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_HDCP22_IRQ_EANBLE:
	{
		if (ctrl->val > 0) {
			srccap_dev->hdmirx_info.ubHDCP22ISREnable = TRUE;
			if (srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task != NULL) {
				//HDMIRX_MSG_ERROR(
				//"Enable HDCP22 ISR but thread has created !\r\n");
				kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task);
				srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task = NULL;
			}
		} else
			srccap_dev->hdmirx_info.ubHDCP22ISREnable = FALSE;

		Drv_HDCP22_IRQEnable(
			srccap_dev->hdmirx_info.ubHDCP22ISREnable);
	}
	break;

	case V4L2_CID_HDMI_RX_HDCP22_PORTINIT:
	{
		Drv_HDCP22_PortInit(ctrl->val);
	}
	break;

	case V4L2_CID_HDMI_RX_HDCP22_GETMSG:
	{
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer = kmalloc(sizeof(ST_HDCP22_MSG),
							GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				sizeof(ST_HDCP22_MSG));
		}
	}
	break;

	case V4L2_CID_HDMI_RX_HDCP22_SENDMSG:
	{
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDCP22_MSG stHDCPMsg;
			MS_U8 *u8Msg = NULL;
			unsigned long ret;

			memcpy(&stHDCPMsg, ctrl->p_new.p_u8,
				sizeof(ST_HDCP22_MSG));
			if (stHDCPMsg.dwDataLen == 0)
				return 0;

			u8Msg = kmalloc(stHDCPMsg.dwDataLen, GFP_KERNEL);

			if (u8Msg != NULL) {
				ret = copy_from_user(u8Msg, compat_ptr(
					ptr_to_compat(stHDCPMsg.pucData)),
					stHDCPMsg.dwDataLen);
				if (ret != 0) {
					HDMIRX_MSG_ERROR("copy_from_user\n"
							"failed\r\n");
					kfree(u8Msg);
					return -EINVAL;
				}

				Drv_HDCP22_SendMsg(0,
					stHDCPMsg.ucPortIdx, u8Msg,
					stHDCPMsg.dwDataLen, NULL);

				kfree(u8Msg);
			}
		} else
			return -ENOMEM;
	}
	break;

	case V4L2_CID_HDMI_RX_HDCP14_R0IRQENABLE:
	{
		if (ctrl->val > 0) {
			srccap_dev->hdmirx_info.ubHDCP14ISREnable = TRUE;
			if (srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task != NULL) {
				//HDMIRX_MSG_ERROR(
				//"Enable HDCP14 ISR but thread has created !\r\n");
				kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task);
				srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task = NULL;
			}
		} else
			srccap_dev->hdmirx_info.ubHDCP14ISREnable = FALSE;

		Drv_HDCP14_ReadR0IRQEnable(
			srccap_dev->hdmirx_info.ubHDCP14ISREnable);
	}
	break;

	case V4L2_CID_HDMI_RX_SET_EQ_TO_PORT:
	{
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_EQ stHDMI_EQ;

			memcpy(&stHDMI_EQ, ctrl->p_new.p_u8,
				sizeof(ST_HDMI_EQ));
			Drv_HDMI_Set_EQ_To_Port(stHDMI_EQ.enEq,
			stHDMI_EQ.u8EQValue, stHDMI_EQ.enInputPortType);
		} else
			return -ENOMEM;
	}
	break;

	case V4L2_CID_HDMI_RX_AUDIO_STATUS_CLEAR:
	{
		Drv_HDMI_Audio_Status_Clear();
	}
	break;

	case V4L2_CID_HDMI_RX_HPD_CONTROL:
	{
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_HPD stHDMI_HPD;

			memcpy(&stHDMI_HPD, ctrl->p_new.p_u8,
				sizeof(ST_HDMI_HPD));
			Drv_HDMI_pullhpd(stHDMI_HPD.bHighLow,
				stHDMI_HPD.enInputPortType,
				stHDMI_HPD.bInverse);
		} else
			return -ENOMEM;
	}
	break;

	case V4L2_CID_HDMI_RX_SW_RESET:
	{
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_RESET stHDMI_Reset;

			memcpy(&stHDMI_Reset, ctrl->p_new.p_u8,
				sizeof(ST_HDMI_RESET));
			Drv_DVI_Software_Reset(stHDMI_Reset.enInputPortType,
				stHDMI_Reset.u16Reset);
		} else
			return -ENOMEM;
	}
	break;

	case V4L2_CID_HDMI_RX_PKT_RESET:
	{
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_PACKET_RESET stHDMI_PKT_Reset;

			memcpy(&stHDMI_PKT_Reset, ctrl->p_new.p_u8,
				sizeof(ST_HDMI_PACKET_RESET));
			Drv_HDMI_PacketReset(stHDMI_PKT_Reset.enInputPortType,
				stHDMI_PKT_Reset.enResetType);
		} else
			return -ENOMEM;
	}
	break;

	case V4L2_CID_HDMI_RX_CLKRTERM_CONTROL:
	{
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_CLKRTERM stHDMI_CLK;

			memcpy(&stHDMI_CLK, ctrl->p_new.p_u8,
				sizeof(ST_HDMI_CLKRTERM));
			Drv_DVI_ClkPullLow(stHDMI_CLK.bPullLow,
				stHDMI_CLK.enInputPortType);
		} else
			return -ENOMEM;
	}
	break;

	case V4L2_CID_HDMI_RX_ARCPIN_CONTROL:
	{
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDCP_HDMI_ENABLE stHDCP_ENABLE;

			memcpy(&stHDCP_ENABLE, ctrl->p_new.p_u8,
				sizeof(ST_HDCP_HDMI_ENABLE));
			Drv_HDMI_ARC_PINControl(stHDCP_ENABLE.enInputPortType,
				stHDCP_ENABLE.bEnable, 0);
		} else
			return -ENOMEM;
	}
	break;

	case V4L2_CID_HDMI_RX_AUDIO_MUTE:
	{
		Drv_HDMI_Audio_MUTE_Enable(ctrl->val);
	}
	break;

	case V4L2_CID_HDMI_RX_SET_DITHER:
	{
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_DITHER stHDMI_Dither;

			memcpy(&stHDMI_Dither, ctrl->p_new.p_u8,
				sizeof(ST_HDMI_DITHER));
			Drv_HDMI_Dither_Set(stHDMI_Dither.enInputPortType,
				stHDMI_Dither.enDitherType,
				stHDMI_Dither.ubRoundEnable);
		} else
			return -ENOMEM;
	}
	break;


	default:
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ctrl_ops hdmirx_ctrl_ops = {
	.g_volatile_ctrl = mtk_hdmirx_g_ctrl,
	.s_ctrl = mtk_hdmirx_s_ctrl,
};

static const struct v4l2_ctrl_config hdmirx_ctrl[] = {
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_IT_CONTENT_TYPE,
		.name = "HDMI Get IT Content Type",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_HDCP22_READDONE,
		.name = "HDMI Get HDCP22 Read Done",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDCP22_READDONE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_GC_INFO,
		.name = "HDMI Get GC",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_GC_INFO)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_ERR_STATUS,
		.name = "HDMI Get Error Status",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_ERR_STATUS)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_ACP,
		.name = "HDMI Get ACP",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BYTE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_PIXEL_REPETITION,
		.name = "HDMI Get Pexel Repetition",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BYTE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_AVI_VER,
		.name = "HDMI Get AVI Version",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BYTE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_AVI_ACTIVEINFO,
		.name = "HDMI Get AVI ActiveInfo",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BOOL)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_ISHDMIMODE,
		.name = "HDMI Get Is HDMI mode",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BOOL)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_CHECK_4K2K,
		.name = "HDMI Get 4k2k info",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BOOL)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_CHECK_ADDITION_FMT,
		.name = "HDMI Get Addition Format",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BYTE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_3D_STRUCTURE,
		.name = "HDMI Get 3D Structure",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BYTE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_3D_EXT_DATA,
		.name = "HDMI Get 3D Ext Data",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BYTE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_3D_META_FIELD,
		.name = "HDMI Get 3D Meta Field",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_META_FIELD)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_SOURCE_VER,
		.name = "HDMI Get Source Version",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BYTE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_DE_STABLE_STATUS,
		.name = "HDMI Get DE Stable",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_BOOL)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_CRC,
		.name = "HDMI Get CRC",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_EMP,
		.name = "HDMI Get EMP Packet contetn",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(stCMD_HDMI_GET_EMPacket)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_FS,
		.name = "HDMI Rx Get FS",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_WORD)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_CTRL,
		.name = "HDMI Rx Ctrl Command",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMIRX_CTRL)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_SET_CURRENT_PORT,
		.name = "HDMI Port Set",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 3,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_HDCP22_IRQ_EANBLE,
		.name = "HDCP22 IRQ ENABLE",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 255,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_HDCP22_PORTINIT,
		.name = "HDCP22 Port Init",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 3,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_HDCP22_GETMSG,
		.name = "HDMI Rx Get HDCP22 MSG",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDCP22_MSG)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_HDCP22_SENDMSG,
		.name = "HDCP22 Send MSG",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDCP22_MSG)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_HDCP14_R0IRQENABLE,
		.name = "HDCP14 R0IRQ Enable",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 255,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_SET_EQ_TO_PORT,
		.name = "HDMI Set EQ",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_EQ)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_AUDIO_STATUS_CLEAR,
		.name = "HDCP Enable",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_HPD_CONTROL,
		.name = "HPD Control",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_HPD)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_SW_RESET,
		.name = "HDMI SE Reset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_RESET)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_RESET,
		.name = "HDMI PKT Reset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_PACKET_RESET)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_CLKRTERM_CONTROL,
		.name = "HDMI Clock Control",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_CLKRTERM)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_ARCPIN_CONTROL,
		.name = "HDMI ARC Control",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDCP_HDMI_ENABLE)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_AUDIO_MUTE,
		.name = "HDMI Audio Mute",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_SET_DITHER,
		.name = "HDMI Set Dither",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(ST_HDMI_DITHER)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops hdmirx_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops hdmirx_sd_internal_ops = {
};

int mtk_srccap_register_hdmirx_subdev(struct v4l2_device *srccap_dev,
			struct v4l2_subdev *hdmirx_subdev,
			struct v4l2_ctrl_handler *hdmirx_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(hdmirx_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(hdmirx_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(hdmirx_ctrl_handler,
			&hdmirx_ctrl[ctrl_count], NULL);
	}

	ret = hdmirx_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create hdmirx ctrl handler\n");
		goto exit;
	}

	hdmirx_subdev->ctrl_handler = hdmirx_ctrl_handler;

	v4l2_subdev_init(hdmirx_subdev, &hdmirx_sd_ops);
	hdmirx_subdev->internal_ops = &hdmirx_sd_internal_ops;
	strlcpy(hdmirx_subdev->name, "mtk-hdmirx",
					sizeof(hdmirx_subdev->name));

	ret = v4l2_device_register_subdev(srccap_dev, hdmirx_subdev);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register hdmirx subdev\n");
		goto exit;
	}
	return 0;

exit:
	v4l2_ctrl_handler_free(hdmirx_ctrl_handler);
	return ret;

}

void mtk_srccap_unregister_hdmirx_subdev(struct v4l2_subdev *hdmirx_subdev)
{
	v4l2_ctrl_handler_free(hdmirx_subdev->ctrl_handler);
	v4l2_device_unregister_subdev(hdmirx_subdev);
}

