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
#include <linux/err.h>
#include <linux/freezer.h>

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
#include "mtk_srccap_hdmirx_phy.h"
#include "mtk_srccap_hdmirx_sysfs.h"
#include "hwreg_srccap_hdmirx_packetreceiver.h"
#include "hwreg_srccap_hdmirx_mux.h"
#include "hwreg_srccap_hdmirx_dsc.h"
#include "drv_HDMI.h"
#include "mtk-reserved-memory.h"

#include "hwreg_srccap_hdmirx.h"
#include "hwreg_srccap_hdmirx_hdcp.h"
#include "hwreg_srccap_hdmirx_phy.h"
#include "hwreg_srccap_hdmirx_mac.h"
#include "hwreg_srccap_hdmirx_irq.h"

#define DEF_READ_DONE_EVENT 1
#define DEF_TEST_USE 0
static bool gHDMIRxInitFlag = FALSE;
static stHDMIRx_Bank stHDMIRxBank;
static struct srccap_port_map *pV4l2SrccapPortMap;

#define SUBSCRIBE_CTRL_EVT(dev, task, func, string, retval)                         \
do {                                                                            \
	if ((task) == NULL) {                                                       \
		task = kthread_create(func, dev, string);                               \
		if (IS_ERR_OR_NULL(task))                                               \
			retval = -ENOMEM;                                                   \
		else                                                                    \
			wake_up_process(task);                                              \
	}                                                                           \
} while (0)

#define UNSUBSCRIBE_CTRL_EVT(task)                                                  \
do {                                                                            \
	if ((task) != NULL) {                                                       \
		kthread_stop(task);                                                     \
		task = NULL;                                                            \
	}                                                                           \
} while (0)

#define VTEM_EVENT_BITMAP(event)\
(1<<(event - V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_VRR_EN_CHG))

#define IS_VTEM_EVENT_SUBSCRIBE(map, event)\
(map & VTEM_EVENT_BITMAP(event))

typedef  void (*funptr_mac_top_irq_handler) (void *);

static int _hdmirx_interrupt_register(
	struct mtk_srccap_dev *srccap_dev,
	struct platform_device *pdev);
static void _hdmirx_isr_handler_setup(struct srccap_hdmirx_interrupt *p_hdmi_int_s);
static int _hdmirx_enable(struct mtk_srccap_dev *srccap_dev);
static int _hdmirx_hw_init(struct mtk_srccap_dev *srccap_dev, int hw_version);
static int _hdmirx_hw_deinit(struct mtk_srccap_dev *srccap_dev);
static int _hdmirx_disable(struct mtk_srccap_dev *srccap_dev);
static void
_hdmirx_isr_handler_release(struct srccap_hdmirx_interrupt *p_hdmi_int_s);
static int _hdmirx_irq_suspend(struct mtk_srccap_dev *srccap_dev);
static int _hdmirx_irq_resume(struct mtk_srccap_dev *srccap_dev);


static void _irq_handler_mac_dec_misc_p0(void *);
static void _irq_handler_mac_dec_misc_p1(void *);
static void _irq_handler_mac_dec_misc_p2(void *);
static void _irq_handler_mac_dec_misc_p3(void *);
static void _irq_handler_mac_dep_misc_p0(void *);
static void _irq_handler_mac_dep_misc_p1(void *);
static void _irq_handler_mac_dep_misc_p2(void *);
static void _irq_handler_mac_dep_misc_p3(void *);
static void _irq_handler_mac_dtop_misc(void *);
static void _irq_handler_mac_inner_misc(void *);
static void _irq_handler_mac_dscd(void *);

static funptr_mac_top_irq_handler _irq_handler_mac_top[E_MAC_TOP_ISR_EVT_N] = {
	_irq_handler_mac_dec_misc_p0,
	_irq_handler_mac_dec_misc_p1,
	_irq_handler_mac_dec_misc_p2,
	_irq_handler_mac_dec_misc_p3,
	_irq_handler_mac_dep_misc_p0,
	_irq_handler_mac_dep_misc_p1,
	_irq_handler_mac_dep_misc_p2,
	_irq_handler_mac_dep_misc_p3,
	_irq_handler_mac_dtop_misc,
	_irq_handler_mac_inner_misc,
	_irq_handler_mac_dscd
};


static bool isV4L2PortAsHDMI(enum v4l2_srccap_input_source src)
{
	bool isHDMI = false;

	if ((src >= V4L2_SRCCAP_INPUT_SOURCE_HDMI &&
		 src <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4) ||
		(src >= V4L2_SRCCAP_INPUT_SOURCE_DVI &&
		 src <= V4L2_SRCCAP_INPUT_SOURCE_DVI4)) {
		isHDMI = true;
	}
	return isHDMI;
}

static int get_memory_info(u64 *addr)
{
	struct device_node *target_memory_np = NULL;
	uint32_t len = 0;
	__be32 *p = NULL;

	// find memory_info node in dts
	target_memory_np = of_find_node_by_name(NULL, "memory_info");
	if (!target_memory_np)
		return -ENODEV;

	// query cpu_emi0_base value from memory_info node
	p = (__be32 *)of_get_property(target_memory_np, "cpu_emi0_base", &len);
	if (p != NULL) {
		*addr = be32_to_cpup(p);
		of_node_put(target_memory_np);
		p = NULL;
	} else {
		HDMIRX_MSG_ERROR("can not find cpu_emi0_base info\n");
		of_node_put(target_memory_np);
		return -EINVAL;
	}
	return 0;
}

static enum srccap_input_port
v4l2src_to_srccap_src(enum v4l2_srccap_input_source src)
{
	enum srccap_input_port eSrccapPort = SRCCAP_INPUT_PORT_NONE;

	if (isV4L2PortAsHDMI(src))
		eSrccapPort = pV4l2SrccapPortMap[src].data_port;
	return eSrccapPort;
}

E_MUX_INPUTPORT mtk_hdmirx_to_muxinputport(enum v4l2_srccap_input_source src)
{
	E_MUX_INPUTPORT ePort = INPUT_PORT_NONE_PORT;
	enum srccap_input_port eSrccapPort = v4l2src_to_srccap_src(src);

	switch (eSrccapPort) {
	case SRCCAP_INPUT_PORT_HDMI0:
		ePort = INPUT_PORT_DVI0;
		break;
	case SRCCAP_INPUT_PORT_HDMI1:
		ePort = INPUT_PORT_DVI1;
		break;
	case SRCCAP_INPUT_PORT_HDMI2:
		ePort = INPUT_PORT_DVI2;
		break;
	case SRCCAP_INPUT_PORT_HDMI3:
		ePort = INPUT_PORT_DVI3;
		break;
	default:
		ePort = INPUT_PORT_NONE_PORT;
		break;
	}
	return ePort;
}

static enum v4l2_srccap_input_source
srccap_src_to_v4l2src(enum srccap_input_port eSrccapPort)
{
	enum v4l2_srccap_input_source ePort;

	for (ePort = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
		 ePort <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4; ePort++) {
		if (pV4l2SrccapPortMap[ePort].data_port == eSrccapPort)
			return ePort;
	}

	return ePort;
}

enum v4l2_srccap_input_source mtk_hdmirx_muxinput_to_v4l2src(E_MUX_INPUTPORT src)
{
	enum srccap_input_port eSrccapPort;

	switch (src) {
	case INPUT_PORT_DVI0:
		eSrccapPort = SRCCAP_INPUT_PORT_HDMI0;
		break;
	case INPUT_PORT_DVI1:
		eSrccapPort = SRCCAP_INPUT_PORT_HDMI1;
		break;
	case INPUT_PORT_DVI2:
		eSrccapPort = SRCCAP_INPUT_PORT_HDMI2;
		break;
	case INPUT_PORT_DVI3:
		eSrccapPort = SRCCAP_INPUT_PORT_HDMI3;
		break;
	default:
		eSrccapPort = SRCCAP_INPUT_PORT_HDMI0;
		break;
	}

	return srccap_src_to_v4l2src(eSrccapPort);
}

static MS_HDMI_CONTENT_TYPE
mtk_hdmirx_Content_Type(enum v4l2_srccap_input_source src)
{
	MS_U8 u8ITCValue = 0;
	MS_U8 u8CN10Value = 0;
	enum v4l2_ext_hdmi_content_type enContentType = V4L2_EXT_HDMI_CONTENT_NoData;
	MS_U8 u8HDMIInfoSource = 0;

	if (isV4L2PortAsHDMI(src)) {
		E_MUX_INPUTPORT ePort = mtk_hdmirx_to_muxinputport(src);

		u8HDMIInfoSource = drv_hwreg_srccap_hdmirx_mux_inport_2_src(ePort);

		if (u8HDMIInfoSource < HDMI_PORT_MAX_NUM) {
			if (MDrv_HDMI_Get_PacketStatus(ePort) &
				HDMI_STATUS_AVI_PACKET_RECEIVE_FLAG) {
				stHDMI_POLLING_INFO polling = MDrv_HDMI_Get_HDMIPollingInfo(ePort);

				u8ITCValue =
					drv_hwreg_srccap_hdmirx_pkt_avi_infoframe_info(
						u8HDMIInfoSource, _BYTE_3, &polling) &
							 BIT(7);
				u8CN10Value =
					drv_hwreg_srccap_hdmirx_pkt_avi_infoframe_info(
						u8HDMIInfoSource, _BYTE_5, &polling) &
							  BMASK(5 : 4);

				switch (u8CN10Value) {
				case 0x00:
					if (u8ITCValue > 0)
						enContentType = V4L2_EXT_HDMI_CONTENT_Graphics;
					else
						enContentType = V4L2_EXT_HDMI_CONTENT_NoData;
					break;

				case 0x10:
					enContentType = V4L2_EXT_HDMI_CONTENT_Photo;
					break;

				case 0x20:
					enContentType = V4L2_EXT_HDMI_CONTENT_Cinema;
					break;

				case 0x30:
					enContentType = V4L2_EXT_HDMI_CONTENT_Game;
					break;

				default:
					enContentType = V4L2_EXT_HDMI_CONTENT_NoData;
					break;
				};
			}
		}
	}

	return (MS_HDMI_CONTENT_TYPE)enContentType;
}

static MS_HDMI_COLOR_FORMAT
mtk_hdmirx_Get_ColorFormat(enum v4l2_srccap_input_source src)
{
	MS_U8 u8HDMIInfoSource = 0;
	MS_HDMI_COLOR_FORMAT enColorFormat = MS_HDMI_COLOR_UNKNOWN;

	if (isV4L2PortAsHDMI(src)) {
		E_MUX_INPUTPORT ePort = mtk_hdmirx_to_muxinputport(src);

		u8HDMIInfoSource = drv_hwreg_srccap_hdmirx_mux_inport_2_src(ePort);

		if (u8HDMIInfoSource < HDMI_PORT_MAX_NUM) {
			stHDMI_POLLING_INFO polling = MDrv_HDMI_Get_HDMIPollingInfo(ePort);

			switch (drv_hwreg_srccap_hdmirx_pkt_avi_infoframe_info(
						u8HDMIInfoSource, _BYTE_1, &polling) &
					0x60) {
			case 0x00:
				enColorFormat = MS_HDMI_COLOR_RGB;
				break;

			case 0x40:
				enColorFormat = MS_HDMI_COLOR_YUV_444;
				break;

			case 0x20:
				enColorFormat = MS_HDMI_COLOR_YUV_422;
				break;

			case 0x60:
				enColorFormat = MS_HDMI_COLOR_YUV_420;
				break;

			default:
				break;
			};
		}
	}

	return enColorFormat;
}

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
	struct hdmi_vsif_packet_info vsif_data[META_SRCCAP_HDMIRX_VSIF_PKT_MAX_SIZE];
	__u8 u8ALLMMode = 0;
	__u8 u8Index = 0;

	memset(&stGetPktStatus, 0, sizeof(stCMD_HDMI_GET_PACKET_STATUS));
	memset(&stGetAVI, 0, sizeof(stCMD_HDMI_GET_AVI_PARSING_INFO));
	memset(vsif_data, 0, sizeof(vsif_data));
	memset(srccap_dev->hdmirx_info.AVI, 0xFF,
		   HDMI_PORT_MAX_NUM * sizeof(stAVI_PARSING_INFO));
	memset(srccap_dev->hdmirx_info.bAVMute, 0x00, HDMI_PORT_MAX_NUM * sizeof(bool));
	srccap_dev->hdmirx_info.u8ALLMMode = 0;

	stGetPktStatus.pExtendPacketReceive = &stPktStatus;
	stGetAVI.pstAVIParsingInfo = &stAVIInfo;
	stGetGC.pstGCParsingInfo = &stGCInfo;
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0; enInputPortType < INPUT_PORT_DVI_MAX;
			 enInputPortType++) {
			enum v4l2_srccap_input_source src =
				mtk_hdmirx_muxinput_to_v4l2src(enInputPortType);
			__u8 u8VSCount = 0;
			__u8 u8VSIndex = 0;

			memset(&stPktStatus, 0, sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t));
			memset(&stAVIInfo, 0, sizeof(stAVI_PARSING_INFO));
			memset(&stGCInfo, 0, sizeof(stGC_PARSING_INFO));
			memset(&event, 0, sizeof(struct v4l2_event));
			stGetPktStatus.enInputPortType = enInputPortType;
			stGetAVI.enInputPortType = enInputPortType;
			stGetGC.enInputPortType = enInputPortType;
			stPktStatus.u16Size = sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t);
			u8Index = enInputPortType - INPUT_PORT_DVI0;

			if (MDrv_HDMI_Ctrl(E_HDMI_INTERFACE_CMD_GET_PACKET_STATUS,
							   &stGetPktStatus,
							   sizeof(stCMD_HDMI_GET_PACKET_STATUS))) {
				if (stPktStatus.bPKT_SPD_RECEIVE) {
					// HDMIRX_MSG_INFO("Receive SPD!\r\n");
					event.u.ctrl.changes |=
						V4L2_EVENT_CTRL_CH_HDMI_RX_RECEIVE_SPD;
				}
			}

			if (MDrv_HDMI_Ctrl(
							E_HDMI_INTERFACE_CMD_GET_GC_PARSING_INFO,
							&stGetGC,
							sizeof(stCMD_HDMI_GET_GC_PARSING_INFO))) {
				// HDMIRX_MSG_INFO("Receive GC!\r\n");
				if (srccap_dev->hdmirx_info.bAVMute[u8Index] !=
					stGCInfo.u8ControlAVMute) {
					// HDMIRX_MSG_INFO("AV Mute Changed : %d!\r\n",
					// stGCInfo.u8ControlAVMute);
					srccap_dev->hdmirx_info.bAVMute[u8Index] =
						stGCInfo.u8ControlAVMute;
					event.u.ctrl.changes |=
						V4L2_EVENT_CTRL_CH_HDMI_RX_AVMUTE_CHANGE;
				}
			}

			if (MDrv_HDMI_Ctrl(
							E_HDMI_INTERFACE_CMD_GET_AVI_PARSING_INFO,
							&stGetAVI,
							sizeof(stCMD_HDMI_GET_AVI_PARSING_INFO))) {
				// HDMIRX_MSG_INFO("Receive AVI!\r\n");
				if ((srccap_dev->hdmirx_info.AVI[u8Index].enColorForamt !=
					 stAVIInfo.enColorForamt) |
					(srccap_dev->hdmirx_info.AVI[u8Index].enAspectRatio !=
					 stAVIInfo.enAspectRatio) |
					(srccap_dev->hdmirx_info.AVI[u8Index]
						 .enActiveFormatAspectRatio !=
					 stAVIInfo.enActiveFormatAspectRatio)) {
					// HDMIRX_MSG_INFO("AVI Changed!\r\n");
					memcpy(&srccap_dev->hdmirx_info.AVI[u8Index], &stAVIInfo,
						   sizeof(stAVI_PARSING_INFO));
					event.u.ctrl.changes |=
						V4L2_EVENT_CTRL_CH_HDMI_RX_AVI_CHANGE;
				}
			}

			u8VSCount = mtk_srccap_hdmirx_pkt_get_VSIF(
				src, vsif_data,
				sizeof(struct hdmi_vsif_packet_info) *
					META_SRCCAP_HDMIRX_VSIF_PKT_MAX_SIZE);
			for (u8VSIndex = 0; u8VSIndex < u8VSCount; u8VSIndex++) {
				// HDMIRX_MSG_INFO("Receive VS!\r\n");

				if ((vsif_data[u8VSIndex].au8ieee[2] == 0xC4) &&
					(vsif_data[u8VSIndex].au8ieee[1] == 0x5D) &&
					(vsif_data[u8VSIndex].au8ieee[0] == 0xD8)) {
					if (vsif_data[u8VSIndex].au8payload[1] & BIT(1))
						u8ALLMMode = 1;
					else
						u8ALLMMode = 0;

					if (srccap_dev->hdmirx_info.u8ALLMMode != u8ALLMMode) {
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
		usleep_range(10000, 11000); // sleep 10ms

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

	set_freezable();
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0; enInputPortType < INPUT_PORT_DVI_MAX;
			 enInputPortType++) {
			u8Index = enInputPortType - INPUT_PORT_DVI0;
			if (srccap_dev->hdmirx_info.bCableDetect[u8Index] !=
				drv_hwreg_srccap_hdmirx_mac_get_scdccabledet(enInputPortType)) {
				srccap_dev->hdmirx_info.bCableDetect[u8Index] =
					drv_hwreg_srccap_hdmirx_mac_get_scdccabledet(
						enInputPortType);
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_CABLE_DETECT;
				event.u.ctrl.changes =
					(srccap_dev->hdmirx_info.bCableDetect[u8Index]
						 ? V4L2_EVENT_CTRL_CH_HDMI_RX_CABLE_CONNECT
						 : V4L2_EVENT_CTRL_CH_HDMI_RX_CABLE_DISCONNECT);
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(10000, 11000); // sleep 10ms

		try_to_freeze();
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

	set_freezable();
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = 0; enInputPortType < HDMI_PORT_MAX_NUM;
			 enInputPortType++) {
			if (drv_hwreg_srccap_hdmirx_hdcp_22_polling_writedone(enInputPortType)) {
				// HDMIRX_MSG_ERROR("Get Write Done Status %d\r\n",
				// enInputPortType);
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS;
				event.u.ctrl.changes = V4L2_EVENT_CTRL_CH_HDMI_RX_HDCP2X_WRITE_DONE;
				event.u.ctrl.value = enInputPortType + INPUT_PORT_DVI0;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
#if DEF_READ_DONE_EVENT
			if (drv_hwreg_srccap_hdmirx_hdcp_22_polling_readdone(enInputPortType)) {
				// HDMIRX_MSG_ERROR("Get Read Done Status %d\r\n",
				// enInputPortType);
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS;
				event.u.ctrl.changes = V4L2_EVENT_CTRL_CH_HDMI_RX_HDCP2X_READ_DONE;
				event.u.ctrl.value = enInputPortType + INPUT_PORT_DVI0;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
#endif
		}
		usleep_range(100, 110); // sleep 100us

		try_to_freeze();
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

	set_freezable();
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = 0; enInputPortType < HDMI_PORT_MAX_NUM;
			 enInputPortType++) {
			if (drv_hwreg_srccap_hdmirx_hdcp_22_polling_r0_readdone(
					enInputPortType)) {
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP1X_STATUS;
				event.u.ctrl.value = enInputPortType + INPUT_PORT_DVI0;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(100, 110); // sleep 100us

		try_to_freeze();
		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);

			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_hdmi_crc_monitor_task(void *data)
{
#define crc_sleep_min 20000
#define crc_sleep_max 21000
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	MS_DVI_CHANNEL_TYPE enChannelType = MS_DVI_CHANNEL_R;
	unsigned short usCrc = 0;
	__u8 u8Index = 0;
	__u8 u8Type = 0;

	set_freezable();
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0; enInputPortType < INPUT_PORT_DVI_MAX;
			 enInputPortType++) {
			for (enChannelType = MS_DVI_CHANNEL_R; enChannelType <= MS_DVI_CHANNEL_B;
				 enChannelType++) {
				u8Index = enInputPortType - INPUT_PORT_DVI0;
				u8Type = enChannelType - MS_DVI_CHANNEL_R;
				if (MDrv_HDMI_GetCRCValue(enInputPortType, enChannelType, &usCrc)) {
					if (srccap_dev->hdmirx_info.usHDMICrc[u8Index][u8Type] !=
						usCrc) {
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
		usleep_range(crc_sleep_min, crc_sleep_max);

		try_to_freeze();
		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_hdmi_mode_monitor_task(void *data)
{
#define hdmi_mode_sleep_min 20000
#define hdmi_mode_sleep_max 21000
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	__u8 u8Index = 0;

	set_freezable();
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0; enInputPortType < INPUT_PORT_DVI_MAX;
			 enInputPortType++) {
			u8Index = enInputPortType - INPUT_PORT_DVI0;
			if (srccap_dev->hdmirx_info.bHDMIMode[u8Index] !=
				MDrv_HDMI_IsHDMI_Mode(enInputPortType)) {
				memset(&event, 0, sizeof(struct v4l2_event));
				srccap_dev->hdmirx_info.bHDMIMode[u8Index] =
					MDrv_HDMI_IsHDMI_Mode(enInputPortType);
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_MODE;
				event.u.ctrl.changes =
					(srccap_dev->hdmirx_info.bHDMIMode[u8Index]
						? V4L2_EVENT_CTRL_CH_HDMI_RX_HDMI_MODE
						: V4L2_EVENT_CTRL_CH_HDMI_RX_DVI_MODE);
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(hdmi_mode_sleep_min, hdmi_mode_sleep_max);

		try_to_freeze();
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

	set_freezable();
	memset(srccap_dev->hdmirx_info.ucHDCPState, 0x00,
		   HDMI_PORT_MAX_NUM * sizeof(unsigned char));
	while (video_is_registered(srccap_dev->vdev)) {
		for (enInputPortType = INPUT_PORT_DVI0; enInputPortType < INPUT_PORT_DVI_MAX;
			 enInputPortType++) {
			u8Index = enInputPortType - INPUT_PORT_DVI0;
			u8HDCPState = MDrv_HDMI_CheckHDCPENCState(enInputPortType);
			if (srccap_dev->hdmirx_info.ucHDCPState[u8Index] != u8HDCPState) {
				memset(&event, 0, sizeof(struct v4l2_event));
				srccap_dev->hdmirx_info.ucHDCPState[u8Index] = u8HDCPState;
				event.type = V4L2_EVENT_CTRL;
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP_STATE;
				switch (srccap_dev->hdmirx_info.ucHDCPState[u8Index]) {
				case 0:
				default:
					event.u.ctrl.changes = V4L2_EXT_HDMI_HDCP_NO_ENCRYPTION;
					break;
				case 1:
					event.u.ctrl.changes = V4L2_EXT_HDMI_HDCP_1_4;
					break;
				case 2:
					event.u.ctrl.changes = V4L2_EXT_HDMI_HDCP_2_2;
					break;
				}
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
		usleep_range(100000, 110000); // sleep 100ms

		try_to_freeze();
		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_dsc_state_monitor_task(void *data)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	__u8 u8Index = 0;
	__u8 u8_tmp = 0;
	__u8 u8_diff = 0;

	set_freezable();
	memset(srccap_dev->hdmirx_info.ucDSCState, 0x00,
		   HDMI_PORT_MAX_NUM * sizeof(unsigned char));
	while (video_is_registered(srccap_dev->vdev)) {
		u8_diff = 0;
		for (enInputPortType = INPUT_PORT_DVI0; enInputPortType < INPUT_PORT_DVI_MAX;
			 enInputPortType++) {
			u8Index = enInputPortType - INPUT_PORT_DVI0;
			u8_tmp = mtk_srccap_hdmirx_pkt_is_dsc(
				mtk_hdmirx_muxinput_to_v4l2src(enInputPortType));
			if (srccap_dev->hdmirx_info.ucDSCState[u8Index] != u8_tmp) {
				u8_diff |= (1 << u8Index);
				srccap_dev->hdmirx_info.ucDSCState[u8Index] = u8_tmp;
			}
			if (u8_tmp && u8_diff) {
				drv_hwreg_srccap_hdmirx_mux_sel_dsc(enInputPortType, E_DSC_D0);
				break;
			}
		}
		if (u8_diff) {
			event.type = V4L2_EVENT_CTRL;
			event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_DSC_STATE;
			event.u.ctrl.changes = V4L2_EXT_HDMI_DSC_PKT_RECV_STATE_CHG;

			memcpy((void *)&event.u.ctrl.value,
				   (void *)&srccap_dev->hdmirx_info.ucDSCState[0],
				   min(sizeof(event.u.ctrl.value),
					   HDMI_PORT_MAX_NUM * sizeof(unsigned char)));
			v4l2_event_queue(srccap_dev->vdev, &event);
		}
		usleep_range(10000, 11000); // sleep 10ms

		try_to_freeze();
		if (kthread_should_stop()) {
			HDMIRX_MSG_ERROR("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_hdmirx_ctrl_vtem_info_monitor_task(
	void *data,
	E_MUX_INPUTPORT enInputPortType)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct srccap_hdmirx_info *p_hdmirx_info = &srccap_dev->hdmirx_info;

	struct s_pkt_emp_vtem *p_emp_vtem_prev
= &p_hdmirx_info->emp_vtem_info[enInputPortType - INPUT_PORT_DVI0];

	struct v4l2_event event;

	struct s_pkt_emp_vtem st_pkt_emp_vtem;

	MS_U32 u32_bitmap = p_hdmirx_info->u32Emp_vtem_info_subscribe_bitmap;

	enum v4l2_ext_hdmi_event_id v4l2_id
		= V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_VRR_EN_CHG;

	if (!u32_bitmap)
		return 0;

	memset((void *)&st_pkt_emp_vtem, 0,
		sizeof(struct s_pkt_emp_vtem));

	drv_hwreg_srccap_hdmirx_pkt_get_emp_vtem_info(
		enInputPortType, &st_pkt_emp_vtem);

	if (memcmp((void *)&st_pkt_emp_vtem,
		(void *)p_emp_vtem_prev,
		sizeof(struct s_pkt_emp_vtem))) {
		// content diff

		event.type = V4L2_EVENT_CTRL;
		event.u.ctrl.value = enInputPortType;

		v4l2_id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_VRR_EN_CHG;
		if (st_pkt_emp_vtem.vrr_en
			!= p_emp_vtem_prev->vrr_en
		&& IS_VTEM_EVENT_SUBSCRIBE(u32_bitmap, v4l2_id)) {
			event.id = v4l2_id;
			event.u.ctrl.changes =
				st_pkt_emp_vtem.vrr_en;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}

		v4l2_id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_M_CONST_CHG;
		if (st_pkt_emp_vtem.m_const
			!= p_emp_vtem_prev->m_const
		&& IS_VTEM_EVENT_SUBSCRIBE(u32_bitmap, v4l2_id)) {
			event.id = v4l2_id;
			event.u.ctrl.changes =
				st_pkt_emp_vtem.m_const;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}

		v4l2_id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_QMS_EN_CHG;
		if (st_pkt_emp_vtem.qms_en
			!= p_emp_vtem_prev->qms_en
		&& IS_VTEM_EVENT_SUBSCRIBE(u32_bitmap, v4l2_id)) {
			event.id = v4l2_id;
			event.u.ctrl.changes =
				st_pkt_emp_vtem.qms_en;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}

		v4l2_id =
		V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_FVA_FACTOR_M1_CHG;
		if (st_pkt_emp_vtem.fva_factor_m1
			!= p_emp_vtem_prev->fva_factor_m1
		&& IS_VTEM_EVENT_SUBSCRIBE(u32_bitmap, v4l2_id)) {
			event.id = v4l2_id;
			event.u.ctrl.changes =
				st_pkt_emp_vtem.fva_factor_m1;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}

		v4l2_id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_NEXT_TFR_CHG;
		if (st_pkt_emp_vtem.next_tfr
			!= p_emp_vtem_prev->next_tfr
		&& IS_VTEM_EVENT_SUBSCRIBE(u32_bitmap, v4l2_id)) {
			event.id = v4l2_id;
			event.u.ctrl.changes =
			(enum E_HDMI_NEXT_TFR)st_pkt_emp_vtem.next_tfr;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}
	}

	memcpy((void *)p_emp_vtem_prev, (void *)&st_pkt_emp_vtem,
		sizeof(struct s_pkt_emp_vtem));
	return 0;
}

static EN_KDRV_HDMIRX_INT _mtk_hdmirx_getKDrvInt(enum srccap_irq_interrupts e_int)
{
	EN_KDRV_HDMIRX_INT e_ret = HDMI_IRQ_PHY;

	switch (e_int) {
	case SRCCAP_HDMI_IRQ_PHY:
		e_ret = HDMI_IRQ_PHY;
		break;

	case SRCCAP_HDMI_IRQ_MAC:
		e_ret = HDMI_IRQ_MAC;
		break;

	case SRCCAP_HDMI_IRQ_PKT_QUE:
		e_ret = HDMI_IRQ_PKT_QUE;
		break;

	case SRCCAP_HDMI_IRQ_PM_SQH_ALL_WK:
		e_ret = HDMI_IRQ_PM_SQH_ALL_WK;
		break;

	case SRCCAP_HDMI_IRQ_PM_SCDC:
		e_ret = HDMI_IRQ_PM_SCDC;
		break;

	case SRCCAP_HDMI_FIQ_CLK_DET_0:
		e_ret = HDMI_FIQ_CLK_DET_0;
		break;

	case SRCCAP_HDMI_FIQ_CLK_DET_1:
		e_ret = HDMI_FIQ_CLK_DET_1;
		break;

	case SRCCAP_HDMI_FIQ_CLK_DET_2:
		e_ret = HDMI_FIQ_CLK_DET_2;
		break;

	case SRCCAP_HDMI_FIQ_CLK_DET_3:
		e_ret = HDMI_FIQ_CLK_DET_3;
		break;

	default:
		HDMIRX_MSG_ERROR("Wring e_int[%d] !\r\n", e_int);
		e_ret = HDMI_IRQ_PHY;
		break;
	}

	return e_ret;
}

static void mtk_hdmirx_hdcp_isr(int irq, void *priv)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)priv;
	struct v4l2_event event;
	unsigned char ucHDCPWriteDoneIndex = 0;
	unsigned char ucHDCPReadDoneIndex = 0;
	unsigned char ucHDCPR0ReadDoneIndex = 0;
	unsigned char ucVersion = 0;

	ucVersion = MDrv_HDMI_HDCP_ISR(&ucHDCPWriteDoneIndex, &ucHDCPReadDoneIndex,
								   &ucHDCPR0ReadDoneIndex);

	if (ucVersion > 0) {
		E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
		MS_U8 ucPortIndex = 0;

		memset(&event, 0, sizeof(struct v4l2_event));
		event.type = V4L2_EVENT_CTRL;

		for (enInputPortType = INPUT_PORT_DVI0;
			 enInputPortType <= INPUT_PORT_DVI_END; enInputPortType++) {
			ucPortIndex = HDMI_GET_PORT_SELECT(enInputPortType);
			// ucVersion = E_HDCP2X_WRITE_DONE
			if ((ucHDCPWriteDoneIndex & BIT(ucPortIndex)) != 0) {
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS;
				event.u.ctrl.changes = V4L2_EVENT_CTRL_CH_HDMI_RX_HDCP2X_WRITE_DONE;
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
				// HDMIRX_MSG_ERROR("Get Write Done Status ISR \r\n");
			}
#if DEF_READ_DONE_EVENT
			// ucVersion = E_HDCP2X_READ_DONE
			if ((ucHDCPReadDoneIndex & BIT(ucPortIndex)) != 0) {
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS;
				event.u.ctrl.changes = V4L2_EVENT_CTRL_CH_HDMI_RX_HDCP2X_READ_DONE;
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
				// HDMIRX_MSG_ERROR("Get Read Done Status ISR \r\n");
			}
#endif
			// ucVersion = E_HDCP1X_R0READ_DONE
			if ((ucHDCPR0ReadDoneIndex & BIT(ucPortIndex)) != 0) {
				event.id = V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP1X_STATUS;
				event.u.ctrl.changes = 0;
				event.u.ctrl.value = enInputPortType;
				v4l2_event_queue(srccap_dev->vdev, &event);
				// HDMIRX_MSG_ERROR("Get R0 Read Done Status ISR \r\n");
			}
		}
	}
}

int mtk_srccap_hdmirx_init(struct mtk_srccap_dev *srccap_dev)
{
	HDMIRX_MSG_DBG("mdbgin_%s :  start\r\n", __func__);
	HDMIRX_MSG_INFO("%s HDMI INIT!!.\n", __func__);

	return 0;
}

int mtk_srccap_hdmirx_release(struct mtk_srccap_dev *srccap_dev)
{
	HDMIRX_MSG_DBG("mdbgin_%s: start \r\n", __func__);
	HDMIRX_MSG_INFO("%s HDMI Release!!.\n", __func__);

	return 0;
}

static int _hdmirx_parse_dts(struct mtk_srccap_dev *srccap_dev,
							 struct platform_device *pdev)
{
	int ret = 0;
	struct device *property_dev = &pdev->dev;

	HDMIRX_MSG_DBG("mdbgin_%s :  start\r\n", __func__);
	HDMIRX_MSG_INFO("%s HDMI INIT!!.\n", __func__);

	/* parse irq info and register irq */
	ret = _hdmirx_interrupt_register(srccap_dev, pdev);
	/* parse register base */
	ret = mtk_srccap_hdmirx_parse_dts_reg(srccap_dev, property_dev);

	return ret;
}

int mtk_hdmirx_init(struct mtk_srccap_dev *srccap_dev, struct platform_device *pdev,
					int logLevel)
{
	int ret = 0;

	//hdmirxloglevel = LOG_LEVEL_DEBUG;
	drv_hwreg_srccap_hdmirx_setloglevel(hdmirxloglevel);

	HDMIRX_MSG_DBG("mdbgin_%s :  start\r\n", __func__);
	HDMIRX_MSG_INFO("%s HDMI INIT!!.\n", __func__);

	ret = _hdmirx_parse_dts(srccap_dev, pdev);

	_hdmirx_isr_handler_setup(&srccap_dev->hdmirx_info.hdmi_int_s);

	ret = _hdmirx_enable(srccap_dev);
	ret = _hdmirx_hw_init(srccap_dev,
	srccap_dev->srccap_info.cap.u32HDMI_Srccap_HWVersion);

	ret = mtk_srccap_hdmirx_sysfs_init(srccap_dev);

	return ret;
}

int mtk_hdmirx_deinit(struct mtk_srccap_dev *srccap_dev)
{
	HDMIRX_MSG_DBG("mdbgin_%s: start \r\n", __func__);
	HDMIRX_MSG_INFO("%s HDMI Release!!.\n", __func__);

	mtk_srccap_hdmirx_sysfs_deinit(srccap_dev);
	_hdmirx_hw_deinit(srccap_dev);
	_hdmirx_disable(srccap_dev);
	_hdmirx_isr_handler_release(&srccap_dev->hdmirx_info.hdmi_int_s);

	return 0;
}

int mtk_hdmirx_subscribe_ctrl_event(
		struct mtk_srccap_dev *srccap_dev,
		const struct v4l2_event_subscription *event_sub)
{
	int retval = 0;
	unsigned int u32_i = 0;
	struct srccap_hdmirx_info *p_hdmirx_info = &srccap_dev->hdmirx_info;

	HDMIRX_MSG_DBG("mdbgin_%s: start \r\n", __func__);
	if (event_sub == NULL) {
		HDMIRX_MSG_ERROR("NULL Event !\r\n");
		HDMIRX_MSG_ERROR("mdbgerr_ %s:  %d \r\n", __func__, -ENXIO);
		return -ENXIO;
	}

	switch (event_sub->id) {
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_PKT_STATUS:
		// HDMIRX_MSG_INFO("Subscribe PKT_STATUS !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_packet_monitor_task == NULL) {
			srccap_dev->hdmirx_info.ctrl_packet_monitor_task =
				kthread_create(mtk_hdmirx_ctrl_packet_monitor_task, srccap_dev,
							   "hdmi ctrl_packet_monitor_task");
			if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.ctrl_packet_monitor_task)) {
				HDMIRX_MSG_ERROR("mdbgerr_ %s:  %d \r\n", __func__, -ENOMEM);

				return -ENOMEM;
			}
			wake_up_process(srccap_dev->hdmirx_info.ctrl_packet_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_CABLE_DETECT:
		// HDMIRX_MSG_INFO("Subscribe CABLE_DETECT !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task == NULL) {
			memset(srccap_dev->hdmirx_info.bCableDetect, 0x00,
				   sizeof(srccap_dev->hdmirx_info.bCableDetect));
			srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task =
				kthread_create(
					mtk_hdmirx_ctrl_cable_detect_monitor_task,
					srccap_dev,
					"hdmi ctrl_5v_detect_monitor_task");
			if (IS_ERR_OR_NULL(
					srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task)) {
				HDMIRX_MSG_ERROR("mdbgerr_%s:  %d\r\n", __func__, -ENOMEM);
				return -ENOMEM;
			}
			wake_up_process(srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS:
		// HDMIRX_MSG_INFO("Subscribe HDCP2X_STATUS !\r\n");
		if (!srccap_dev->hdmirx_info.ubHDCP22ISREnable) {
			// HDMIRX_MSG_ERROR("HDCP22 Status Thread Create !\r\n");
			if (srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task == NULL) {
				srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task =
					kthread_create(
						mtk_hdmirx_ctrl_hdcp2x_irq_monitor_task,
						srccap_dev,
						"hdmi ctrl_hdcp2x_irq_monitor_task");
				if (IS_ERR_OR_NULL(
					srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task))
					return -ENOMEM;
				wake_up_process(
					srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task);
			}
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP1X_STATUS:
		// HDMIRX_MSG_INFO("Subscribe HDCP1X_STATUS !\r\n");
		if (!srccap_dev->hdmirx_info.ubHDCP14ISREnable) {
			// HDMIRX_MSG_ERROR("HDCP14 Status Thread Create !\r\n");
			if (srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task == NULL) {
				srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task =
					kthread_create(
						mtk_hdmirx_ctrl_hdcp1x_irq_monitor_task,
						srccap_dev,
						"hdmi ctrl_hdcp1x_irq_monitor_task");
				if (IS_ERR_OR_NULL(
					srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task))
					return -ENOMEM;
				wake_up_process(
					srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task);
			}
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_CRC:
		// HDMIRX_MSG_INFO("Subscribe HDMI_CRC !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task == NULL) {
			srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task =
				kthread_create(mtk_hdmirx_ctrl_hdmi_crc_monitor_task, srccap_dev,
							   "hdmi ctrl_hdmi_crc_monitor_task");
			if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task))
				return -ENOMEM;
			wake_up_process(srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_MODE:
		// HDMIRX_MSG_INFO("Subscribe HDMI_MODE !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task == NULL) {
			memset(srccap_dev->hdmirx_info.bHDMIMode, 0x00,
				   sizeof(srccap_dev->hdmirx_info.bHDMIMode));
			srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task =
				kthread_create(mtk_hdmirx_ctrl_hdmi_mode_monitor_task, srccap_dev,
							   "hdmi ctrl_hdmi_mode_monitor_task");
			if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task))
				return -ENOMEM;
			wake_up_process(srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP_STATE:
		// HDMIRX_MSG_INFO("Subscribe HDCP_STATE !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task == NULL) {
			srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task =
				kthread_create(mtk_hdmirx_ctrl_hdcp_state_monitor_task, srccap_dev,
							   "hdmi ctrl_hdcp_state_monitor_task");
			if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task))
				return -ENOMEM;
			wake_up_process(srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task);
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_DSC_STATE:
		SUBSCRIBE_CTRL_EVT(
			srccap_dev,
			srccap_dev->hdmirx_info.ctrl_dsc_state_monitor_task,
			mtk_hdmirx_ctrl_dsc_state_monitor_task,
			"hdmi ctrl_dsc_state_monitor_task",
			retval);
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_VRR_EN_CHG:
		for (u32_i = 0 ; u32_i < HDMI_PORT_MAX_NUM; u32_i++)
			p_hdmirx_info->emp_vtem_info[u32_i].vrr_en = 0;
		p_hdmirx_info->u32Emp_vtem_info_subscribe_bitmap |=
			VTEM_EVENT_BITMAP(event_sub->id);
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_M_CONST_CHG:
		for (u32_i = 0 ; u32_i < HDMI_PORT_MAX_NUM; u32_i++)
			p_hdmirx_info->emp_vtem_info[u32_i].m_const = 0;
		p_hdmirx_info->u32Emp_vtem_info_subscribe_bitmap |=
			VTEM_EVENT_BITMAP(event_sub->id);
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_QMS_EN_CHG:
		for (u32_i = 0 ; u32_i < HDMI_PORT_MAX_NUM; u32_i++)
			p_hdmirx_info->emp_vtem_info[u32_i].qms_en = 0;
		p_hdmirx_info->u32Emp_vtem_info_subscribe_bitmap |=
			VTEM_EVENT_BITMAP(event_sub->id);
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_FVA_FACTOR_M1_CHG:
		for (u32_i = 0 ; u32_i < HDMI_PORT_MAX_NUM; u32_i++)
			p_hdmirx_info->emp_vtem_info[u32_i].fva_factor_m1 = 0;
		p_hdmirx_info->u32Emp_vtem_info_subscribe_bitmap |=
			VTEM_EVENT_BITMAP(event_sub->id);
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_NEXT_TFR_CHG:
		for (u32_i = 0 ; u32_i < HDMI_PORT_MAX_NUM; u32_i++)
			p_hdmirx_info->emp_vtem_info[u32_i].next_tfr = 0;
		p_hdmirx_info->u32Emp_vtem_info_subscribe_bitmap |=
			VTEM_EVENT_BITMAP(event_sub->id);
		break;
	default:
		break;
	}

	return retval;
}

int mtk_hdmirx_unsubscribe_ctrl_event(
	struct mtk_srccap_dev *srccap_dev,
	const struct v4l2_event_subscription *event_sub)
{
	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);
	if (event_sub == NULL) {
		HDMIRX_MSG_ERROR("NULL Event !\r\n");
		HDMIRX_MSG_ERROR("mdbgerr_%s:  %d \r\n", __func__, -ENXIO);
		return -ENXIO;
	}

	switch (event_sub->id) {
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_PKT_STATUS:
		// HDMIRX_MSG_INFO("UnSubscribe PKT_STATUS !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_packet_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_packet_monitor_task);
			srccap_dev->hdmirx_info.ctrl_packet_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_CABLE_DETECT:
		// HDMIRX_MSG_INFO("UnSubscribe CABLE_DETECT !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task);
			srccap_dev->hdmirx_info.ctrl_5v_detect_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP2X_STATUS:
		// HDMIRX_MSG_INFO("UnSubscribe HDCP2X_STATUS !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP1X_STATUS:
		// HDMIRX_MSG_INFO("UnSubscribe HDCP1X_STATUS !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_CRC:
		// HDMIRX_MSG_INFO("UnSubscribe HDMI_CRC !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdmi_crc_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDMI_MODE:
		// HDMIRX_MSG_INFO("UnSubscribe HDMI_MODE !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdmi_mode_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_HDCP_STATE:
		// HDMIRX_MSG_INFO("UnSubscribe HDCP_STATE !\r\n");
		if (srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task != NULL) {
			kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task);
			srccap_dev->hdmirx_info.ctrl_hdcp_state_monitor_task = NULL;
		}
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_DSC_STATE:
		UNSUBSCRIBE_CTRL_EVT(srccap_dev->hdmirx_info.ctrl_dsc_state_monitor_task);
		break;
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_VRR_EN_CHG:
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_M_CONST_CHG:
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_QMS_EN_CHG:
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_FVA_FACTOR_M1_CHG:
	case V4L2_SRCCAP_CTRL_EVENT_HDMIRX_VTEM_NEXT_TFR_CHG:
		srccap_dev->hdmirx_info.u32Emp_vtem_info_subscribe_bitmap &=
			~((MS_U32)VTEM_EVENT_BITMAP(event_sub->id));
		break;
	default:
		break;
	}

	return 0;
}

int mtk_hdmirx_set_current_port(struct mtk_srccap_dev *srccap_dev)
{
	E_MUX_INPUTPORT ePort = mtk_hdmirx_to_muxinputport(srccap_dev->srccap_info.src);

	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);
	HDMIRX_MSG_DBG("%s: dev_id[%d] src[%d] ePort[%d] \r\n", __func__,
				   srccap_dev->dev_id, srccap_dev->srccap_info.src, ePort);
	drv_hwreg_srccap_hdmirx_mux_sel_video(srccap_dev->dev_id, ePort);
	return 0;
}

int mtk_hdmirx_g_edid(struct v4l2_edid *edid)
{
#define EDID_BLOCK_MAX 4
#define EDID_BLOCK_SIZE 128
	XC_DDCRAM_PROG_INFO pstDDCRam_Info;
	MS_U8 u8EDID[EDID_BLOCK_MAX * EDID_BLOCK_SIZE];
	HDMIRX_MSG_DBG("mdbgin_%s: start \r\n", __func__);
	if ((edid != NULL) && (edid->edid != NULL)) {
		if ((edid->start_block >= EDID_BLOCK_MAX) ||
			(edid->blocks > EDID_BLOCK_MAX))
			return -EINVAL;

		if (((edid->start_block + edid->blocks) > EDID_BLOCK_MAX) ||
			(((edid->start_block + edid->blocks) * EDID_BLOCK_SIZE) >
			 (EDID_BLOCK_MAX * EDID_BLOCK_SIZE)))
			return -EINVAL;

		pstDDCRam_Info.EDID = u8EDID;
		pstDDCRam_Info.u16EDIDSize =
			(edid->start_block + edid->blocks) * EDID_BLOCK_SIZE;
		pstDDCRam_Info.eDDCProgType = edid->pad;
		MDrv_HDMI_READ_DDCRAM(&pstDDCRam_Info);
		memcpy(edid->edid, (u8EDID + (edid->start_block * EDID_BLOCK_SIZE)),
			   edid->blocks * EDID_BLOCK_SIZE);
	} else {
		HDMIRX_MSG_ERROR("mdbgerr_%s:  %d \r\n", __func__, -EINVAL);
		return -EINVAL;
	}

	return 0;
}

int mtk_hdmirx_s_edid(struct v4l2_edid *edid)
{
	XC_DDCRAM_PROG_INFO pstDDCRam_Info;
	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);
	if ((edid != NULL) && (edid->edid != NULL)) {
		if (edid->blocks > 4)
			return -E2BIG;

		pstDDCRam_Info.EDID = edid->edid;
		pstDDCRam_Info.u16EDIDSize = edid->blocks * 128;
		pstDDCRam_Info.eDDCProgType = edid->pad;
		MDrv_HDMI_PROG_DDCRAM(&pstDDCRam_Info);
	} else {
		HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, -EINVAL);
		return -EINVAL;
	}

	return 0;
}

int mtk_hdmirx_g_audio_channel_status(struct mtk_srccap_dev *srccap_dev,
									  struct v4l2_audio *audio)
{
	HDMIRX_MSG_DBG("mdbgin_%s:  start\r\n", __func__);
	if (audio != NULL) {
		E_MUX_INPUTPORT ePort =
			mtk_hdmirx_to_muxinputport(srccap_dev->srccap_info.src);
		MS_U8 u8HDMIInfoSource = drv_hwreg_srccap_hdmirx_mux_inport_2_src(ePort);
		// index which byte to get.
		snprintf(audio->name, sizeof(audio->name), "byte %d", audio->index);
		audio->index = drv_hwreg_srccap_hdmirx_pkt_audio_channel_status(
			u8HDMIInfoSource, audio->index, NULL);
	} else {
		HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, -EINVAL);
		return -EINVAL;
	}

	return 0;
}

int mtk_hdmirx_g_dv_timings(struct mtk_srccap_dev *srccap_dev,
							struct v4l2_dv_timings *timings)
{
	HDMIRX_MSG_DBG("mdbgin_%s: start\r\n", __func__);
	if ((timings != NULL) && (isV4L2PortAsHDMI(srccap_dev->srccap_info.src))) {
		E_MUX_INPUTPORT ePort =
			mtk_hdmirx_to_muxinputport(srccap_dev->srccap_info.src);
		stHDMI_POLLING_INFO polling = MDrv_HDMI_Get_HDMIPollingInfo(ePort);
		MS_U8 u8HDMIInfoSource = drv_hwreg_srccap_hdmirx_mux_inport_2_src(ePort);

		timings->bt.width = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_HDE, u8HDMIInfoSource, NULL);
		timings->bt.hfrontporch = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_HFRONT, u8HDMIInfoSource, NULL);
		timings->bt.hsync = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_HSYNC, u8HDMIInfoSource, NULL);
		timings->bt.hbackporch = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_HBACK, u8HDMIInfoSource, NULL);
		timings->bt.height = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_VDE, u8HDMIInfoSource, NULL);
		timings->bt.vfrontporch = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_VFRONT, u8HDMIInfoSource, NULL);
		timings->bt.vsync = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_VSYNC, u8HDMIInfoSource, NULL);
		timings->bt.vbackporch = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_VBACK, u8HDMIInfoSource, NULL);
		timings->bt.interlaced = drv_hwreg_srccap_hdmirx_mac_get_datainfo(
			E_HDMI_GET_INTERLACE, u8HDMIInfoSource, NULL);
		timings->bt.pixelclock = drv_hwreg_srccap_hdmirx_mac_getratekhz(
			ePort, E_HDMI_PIX,
			(polling.ucSourceVersion == HDMI_SOURCE_VERSION_HDMI21 ? TRUE : FALSE));
		timings->bt.polarities =
			(drv_hwreg_srccap_hdmirx_mac_get_datainfo(E_HDMI_GET_H_POLARITY,
					u8HDMIInfoSource, NULL) & 0x1);
		timings->bt.polarities |=
			(drv_hwreg_srccap_hdmirx_mac_get_datainfo(E_HDMI_GET_V_POLARITY,
					u8HDMIInfoSource, NULL) & 0x1) << 4;
	} else {
		HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, -EINVAL);
		return -EINVAL;
	}

	return 0;
}

int mtk_hdmirx_store_color_fmt(struct mtk_srccap_dev *srccap_dev)
{
	HDMIRX_MSG_DBG("mdbgin_%s:  start\r\n", __func__);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (isV4L2PortAsHDMI(srccap_dev->srccap_info.src)) {
		srccap_dev->hdmirx_info.color_format =
			mtk_hdmirx_Get_ColorFormat(srccap_dev->srccap_info.src);
	}
	return 0;
}

int mtk_hdmirx_SetPowerState(struct mtk_srccap_dev *srccap_dev,
							 EN_POWER_MODE ePowerState)
{
	if (srccap_dev->dev_id == 0) {
		if (ePowerState == E_POWER_SUSPEND) {
			_hdmirx_irq_suspend(srccap_dev);
			MDrv_HDMI_STREventProc(E_POWER_SUSPEND, &stHDMIRxBank);
			_hdmirx_disable(srccap_dev);
		} else if (ePowerState == E_POWER_RESUME) {
			_hdmirx_enable(srccap_dev);
			MDrv_HDMI_STREventProc(E_POWER_RESUME, &stHDMIRxBank);
			_hdmirx_irq_resume(srccap_dev);
		} else {
			HDMIRX_MSG_ERROR("Not support this Power State(%d).\n", ePowerState);
			return -EINVAL;
		}
	}
	return 0;
}

int _mtk_hdmirx_ctrl_process(__u32 u32Cmd, void *pBuf, __u32 u32BufSize)
{
	switch (u32Cmd) {
	case V4L2_EXT_HDMI_INTERFACE_CMD_GET_HDCP_STATE: {
		unsigned long ret;
		stCMD_HDMI_CHECK_HDCP_STATE stHDCP_State;

		ret = copy_from_user(&stHDCP_State,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stHDCP_State.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDCP_State.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_State, u32BufSize);
#if DEF_TEST_USE
			stHDCP_State.ucHDCPState = 0x11 + stHDCP_State.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP State failed\n"
							 "\r\n");
			return -EINVAL;
		}
		ret = copy_to_user(pBuf, &stHDCP_State, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user HDCP State failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_INTERFACE_CMD_WRITE_X74: {
		unsigned long ret;
		stCMD_HDCP_WRITE_X74 stHDCP_WriteX74;

		ret = copy_from_user(&stHDCP_WriteX74,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stHDCP_WriteX74.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDCP_WriteX74.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_WriteX74, u32BufSize);
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Write X74 failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_INTERFACE_CMD_READ_X74: {
		unsigned long ret;
		stCMD_HDCP_READ_X74 stHDCP_ReadX74;

		ret = copy_from_user(&stHDCP_ReadX74,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stHDCP_ReadX74.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDCP_ReadX74.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_ReadX74, u32BufSize);
#if DEF_TEST_USE
			stHDCP_ReadX74.ucRetData = 0x11 + stHDCP_ReadX74.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Read X74 failed\r\n");
			return -EINVAL;
		}
		ret = copy_to_user(pBuf, &stHDCP_ReadX74, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Read X74 failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_INTERFACE_CMD_SET_REPEATER: {
		unsigned long ret;
		stCMD_HDCP_SET_REPEATER stHDCP_SetRepeater;

		ret = copy_from_user(&stHDCP_SetRepeater,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stHDCP_SetRepeater.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDCP_SetRepeater.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_SetRepeater, u32BufSize);
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP Set Repeater\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_SET_BSTATUS: {
		unsigned long ret;
		stCMD_HDCP_SET_BSTATUS stHDCP_SetBstatus;

		ret = copy_from_user(&stHDCP_SetBstatus,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stHDCP_SetBstatus.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDCP_SetBstatus.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_SetBstatus, u32BufSize);
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP Set BStatus\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_SET_HDMI_MODE: {
		unsigned long ret;
		stCMD_HDCP_SET_HDMI_MODE stHDMI_SetMode;

		ret = copy_from_user(&stHDMI_SetMode,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stHDMI_SetMode.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_SetMode.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDMI_SetMode, u32BufSize);
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Set HDMI Mode\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_INTERRUPT_STATUS: {
		unsigned long ret;
		stCMD_HDCP_GET_INTERRUPT_STATUS stHDCP_IRQStatus;

		ret = copy_from_user(&stHDCP_IRQStatus,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stHDCP_IRQStatus.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDCP_IRQStatus.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_IRQStatus, u32BufSize);
#if DEF_TEST_USE
			stHDCP_IRQStatus.ucRetIntStatus =
				0x11 + stHDCP_IRQStatus.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP IRQ Status\n"
							 "failed\r\n");
			return -EINVAL;
		}
		ret = copy_to_user(pBuf, &stHDCP_IRQStatus, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user HDCP IRQ Status\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_WRITE_KSV_LIST: {
		//"Untrusted value as argument"
		// Passing tainted variable "stHDCP_KSVList.ulLen" to a tainted sink.

		unsigned long ret;
		stCMD_HDCP_WRITE_KSV_LIST stHDCP_KSVList;
		__u8 *u8KSVList = NULL;

		ret = copy_from_user(&stHDCP_KSVList,
				     pBuf,
				     u32BufSize);

		if (ret == 0) {
			if ((stHDCP_KSVList.ulLen > 0) && (stHDCP_KSVList.ulLen <= 635)) {
				u8KSVList = kmalloc(stHDCP_KSVList.ulLen, GFP_KERNEL);
				if (u8KSVList == NULL) {
					HDMIRX_MSG_ERROR("Allocate KSVList\n"
									 "Memory Failed!\r\n");
					return -ENOMEM;
				}
				ret = copy_from_user(
						u8KSVList,
						stHDCP_KSVList.pucKSV,
						stHDCP_KSVList.ulLen);

				if (ret == 0) {
					stHDCP_KSVList.pucKSV = u8KSVList;
					stHDCP_KSVList.enInputPortType = mtk_hdmirx_to_muxinputport(
						(enum v4l2_srccap_input_source)
							stHDCP_KSVList.enInputPortType);
					MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_KSVList, u32BufSize);
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
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_SET_VPRIME: {
		unsigned long ret;
		stCMD_HDCP_SET_VPRIME stHDCP_Vprime;
		__u8 *u8Vprime = NULL;

		ret = copy_from_user(&stHDCP_Vprime,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			u8Vprime = kmalloc(HDMI_HDCP_VPRIME_LENGTH, GFP_KERNEL);
			if (u8Vprime == NULL) {
				HDMIRX_MSG_ERROR("Allocate Vprime Memory\n"
								 "Failed!\r\n");
				return -ENOMEM;
			}
			ret = copy_from_user(u8Vprime, stHDCP_Vprime.pucVPrime,
								 HDMI_HDCP_VPRIME_LENGTH);

			if (ret == 0) {
				stHDCP_Vprime.pucVPrime = u8Vprime;
				stHDCP_Vprime.enInputPortType = mtk_hdmirx_to_muxinputport(
					(enum v4l2_srccap_input_source)
						stHDCP_Vprime.enInputPortType);
				MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_Vprime, u32BufSize);
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
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_DATA_RTERM_CONTROL: {
		unsigned long ret;
		stCMD_HDMI_DATA_RTERM_CONTROL stData_Rterm;

		ret = copy_from_user(&stData_Rterm,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stData_Rterm.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stData_Rterm.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stData_Rterm, u32BufSize);
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Data Rterm\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_SCDC_VALUE: {
		unsigned long ret;
		stCMD_HDMI_GET_SCDC_VALUE stSCDC_Value;

		ret = copy_from_user(&stSCDC_Value,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stSCDC_Value.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stSCDC_Value.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stSCDC_Value, u32BufSize);
#if DEF_TEST_USE
			stSCDC_Value.ucRetData = 0x11 + stSCDC_Value.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user SCDC Value\n"
							 "failed\r\n");
			return -EINVAL;
		}
		ret = copy_to_user(pBuf, &stSCDC_Value, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user SCDC Value failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_TMDS_RATES_KHZ: {
		unsigned long ret;
		stCMD_HDMI_GET_TMDS_RATES_KHZ stTMDS_Rates;

		ret = copy_from_user(&stTMDS_Rates,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stTMDS_Rates.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stTMDS_Rates.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stTMDS_Rates, u32BufSize);
#if DEF_TEST_USE
			stTMDS_Rates.ulRetRates = 0x11 + stTMDS_Rates.enInputPortType;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user TMDS Rates\n"
							 "failed\r\n");
			return -EINVAL;
		}
		ret = copy_to_user(pBuf, &stTMDS_Rates, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user TMDS Rates failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_CABLE_DETECT: {
		unsigned long ret;
		stCMD_HDMI_GET_SCDC_CABLE_DETECT stCable_Detect;

		ret = copy_from_user(&stCable_Detect,
				     pBuf,
				     u32BufSize);
		if (ret == 0) {
			stCable_Detect.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stCable_Detect.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stCable_Detect, u32BufSize);
#if DEF_TEST_USE
			stCable_Detect.bCableDetectFlag = TRUE;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Cable Detect\n"
							 "failed\r\n");
			return -EINVAL;
		}
		ret = copy_to_user(pBuf, &stCable_Detect, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Cable Detect\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_PACKET_STATUS: {
		unsigned long ret;
		stCMD_HDMI_GET_PACKET_STATUS stPkt_Status;
		MS_HDMI_EXTEND_PACKET_RECEIVE_t stPktReceStatus;
		MS_HDMI_EXTEND_PACKET_RECEIVE_t *pstUser_PktReceStatus;

		ret = copy_from_user(&stPkt_Status, pBuf, u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_PktReceStatus = stPkt_Status.pExtendPacketReceive;
			ret = copy_from_user(
					&stPktReceStatus,
					stPkt_Status.pExtendPacketReceive,
					sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t));
			if (ret == 0) {
				stPkt_Status.pExtendPacketReceive = &stPktReceStatus;
				stPkt_Status.enInputPortType = mtk_hdmirx_to_muxinputport(
					(enum v4l2_srccap_input_source)
						stPkt_Status.enInputPortType);
				MDrv_HDMI_Ctrl(u32Cmd, (void *)&stPkt_Status, u32BufSize);
#if DEF_TEST_USE
				stPkt_Status.pExtendPacketReceive->bPKT_GC_RECEIVE = TRUE;
#endif
				ret = copy_to_user(
						pstUser_PktReceStatus,
						stPkt_Status.pExtendPacketReceive,
						sizeof(MS_HDMI_EXTEND_PACKET_RECEIVE_t));
				if (ret == 0)
					stPkt_Status.pExtendPacketReceive = pstUser_PktReceStatus;
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
		ret = copy_to_user(pBuf, &stPkt_Status, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get Packet Status\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_PACKET_CONTENT: {
		//"Untrusted value as argument"
		// Passing tainted variable "stPkt_Content.u8PacketLength"
		//						to a tainted sink.

		unsigned long ret;
		stCMD_HDMI_GET_PACKET_CONTENT stPkt_Content;
		__u8 *u8PktContent = NULL;

		ret = copy_from_user(&stPkt_Content,
					 pBuf,
					 u32BufSize);
		u8PktContent = stPkt_Content.pu8PacketContent; // get user Addr
		if (ret == 0) {
			if ((stPkt_Content.u8PacketLength > 0) &&
				(stPkt_Content.u8PacketLength <= 121)) {
				stPkt_Content.pu8PacketContent =
					kmalloc(stPkt_Content.u8PacketLength, GFP_KERNEL);
				if (stPkt_Content.pu8PacketContent == NULL) {
					HDMIRX_MSG_ERROR("Allocate Packet\n"
									 "Content Memory Failed!\r\n");
					return -ENOMEM;
				}

				stPkt_Content.enInputPortType = mtk_hdmirx_to_muxinputport(
					(enum v4l2_srccap_input_source)
						stPkt_Content.enInputPortType);
				MDrv_HDMI_Ctrl(u32Cmd, (void *)&stPkt_Content, u32BufSize);
#if DEF_TEST_USE
				*stPkt_Content.pu8PacketContent =
					0x11 + stPkt_Content.enInputPortType;
#endif
				ret = copy_to_user(
						u8PktContent,
						stPkt_Content.pu8PacketContent,
						stPkt_Content.u8PacketLength);
				if (ret == 0) {
					kfree(stPkt_Content.pu8PacketContent);
					stPkt_Content.pu8PacketContent = u8PktContent;
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
		ret = copy_to_user(pBuf, &stPkt_Content, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get Packet Content\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_HDR_METADATA: {
		unsigned long ret;
		stCMD_HDMI_GET_HDR_METADATA stHDR_GetData;
		sHDR_METADATA stHDR_Meta;
		sHDR_METADATA *pstUser_HDRMeta;

		ret = copy_from_user(&stHDR_GetData,
					 pBuf,
					 u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_HDRMeta = stHDR_GetData.pstHDRMetadata;
			ret = copy_from_user(&stHDR_Meta, stHDR_GetData.pstHDRMetadata,
								 sizeof(sHDR_METADATA));
			if (ret == 0) {
				stHDR_GetData.pstHDRMetadata = &stHDR_Meta;
				stHDR_GetData.enInputPortType = mtk_hdmirx_to_muxinputport(
					(enum v4l2_srccap_input_source)
						stHDR_GetData.enInputPortType);
				MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDR_GetData, u32BufSize);
#if DEF_TEST_USE
				stHDR_Meta.u8Length = 0x11 + stHDR_GetData.enInputPortType;
#endif
				ret = copy_to_user(
						pstUser_HDRMeta,
						stHDR_GetData.pstHDRMetadata,
						sizeof(sHDR_METADATA));
				if (ret == 0) {
					stHDR_GetData.pstHDRMetadata = pstUser_HDRMeta;
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
		ret = copy_to_user(pBuf, &stHDR_GetData, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get HDR failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_AVI_PARSING_INFO: {
		unsigned long ret;
		stCMD_HDMI_GET_AVI_PARSING_INFO stAVI_GetInfo;
		stAVI_PARSING_INFO stAVI_ParseInfo;
		stAVI_PARSING_INFO *pstUser_ParseAVIInfo;

		ret = copy_from_user(&stAVI_GetInfo,
					 pBuf,
					 u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_ParseAVIInfo = stAVI_GetInfo.pstAVIParsingInfo;
			ret =
				copy_from_user(
					&stAVI_ParseInfo, stAVI_GetInfo.pstAVIParsingInfo,
					sizeof(stAVI_PARSING_INFO));
			if (ret == 0) {
				stAVI_GetInfo.pstAVIParsingInfo = &stAVI_ParseInfo;
				stAVI_GetInfo.enInputPortType = mtk_hdmirx_to_muxinputport(
					(enum v4l2_srccap_input_source)
						stAVI_GetInfo.enInputPortType);
				MDrv_HDMI_Ctrl(u32Cmd, (void *)&stAVI_GetInfo, u32BufSize);
#if DEF_TEST_USE
				stAVI_ParseInfo.u8Length = 0x11 + stAVI_GetInfo.enInputPortType;
#endif
				ret = copy_to_user(
						pstUser_ParseAVIInfo,
						stAVI_GetInfo.pstAVIParsingInfo,
						sizeof(stAVI_PARSING_INFO));
				if (ret == 0)
					stAVI_GetInfo.pstAVIParsingInfo = pstUser_ParseAVIInfo;
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
		ret = copy_to_user(pBuf, &stAVI_GetInfo, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get AVI Info\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_VS_PARSING_INFO: {
		unsigned long ret;
		stCMD_HDMI_GET_VS_PARSING_INFO stVS_GetInfo;
		stVS_PARSING_INFO stVS_Info;
		stVS_PARSING_INFO *pstUser_VSInfo;

		ret = copy_from_user(&stVS_GetInfo,
					 pBuf,
					 u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_VSInfo = stVS_GetInfo.pstVSParsingInfo;
			ret = copy_from_user(&stVS_Info, stVS_GetInfo.pstVSParsingInfo,
								 sizeof(stVS_PARSING_INFO));
			if (ret == 0) {
				stVS_GetInfo.pstVSParsingInfo = &stVS_Info;
				stVS_GetInfo.enInputPortType = mtk_hdmirx_to_muxinputport(
					(enum v4l2_srccap_input_source)
						stVS_GetInfo.enInputPortType);
				MDrv_HDMI_Ctrl(u32Cmd, (void *)&stVS_GetInfo, u32BufSize);
#if DEF_TEST_USE
				stVS_Info.u8Length = 0x11 + stVS_GetInfo.enInputPortType;
#endif
				ret = copy_to_user(
						pstUser_VSInfo,
						stVS_GetInfo.pstVSParsingInfo,
						sizeof(stVS_PARSING_INFO));
				if (ret == 0)
					stVS_GetInfo.pstVSParsingInfo = pstUser_VSInfo;
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
		ret = copy_to_user(pBuf, &stVS_GetInfo, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get VS Info\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_GC_PARSING_INFO: {
		unsigned long ret;
		stCMD_HDMI_GET_GC_PARSING_INFO stGC_GetInfo;
		stGC_PARSING_INFO stGC_Info;
		stGC_PARSING_INFO *pstUser_GCInfo;

		ret = copy_from_user(&stGC_GetInfo,
					 pBuf,
					 u32BufSize);
		if (ret == 0) {
			// get user Addr
			pstUser_GCInfo = stGC_GetInfo.pstGCParsingInfo;
			ret = copy_from_user(&stGC_Info, stGC_GetInfo.pstGCParsingInfo,
								 sizeof(stGC_PARSING_INFO));
			if (ret == 0) {
				stGC_GetInfo.pstGCParsingInfo = &stGC_Info;
				stGC_GetInfo.enInputPortType = mtk_hdmirx_to_muxinputport(
					(enum v4l2_srccap_input_source)
						stGC_GetInfo.enInputPortType);
				MDrv_HDMI_Ctrl(u32Cmd, (void *)&stGC_GetInfo, u32BufSize);
#if DEF_TEST_USE
				stGC_Info.enColorDepth =
					MS_HDMI_COLOR_DEPTH_8_BIT +
					stGC_GetInfo.enInputPortType - 80;
#endif
				ret = copy_to_user(
						pstUser_GCInfo,
						stGC_GetInfo.pstGCParsingInfo,
						sizeof(stGC_PARSING_INFO));
				if (ret == 0)
					stGC_GetInfo.pstGCParsingInfo = pstUser_GCInfo;
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
		ret = copy_to_user(pBuf, &stGC_GetInfo, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Get GC Info failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_TIMING_INFO: {
		unsigned long ret;
		stCMD_HDMI_GET_TIMING_INFO stTiming_Info;

		ret = copy_from_user(&stTiming_Info,
					 pBuf,
					 u32BufSize);
		if (ret == 0) {
			stTiming_Info.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stTiming_Info.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stTiming_Info, u32BufSize);
#if DEF_TEST_USE
			stTiming_Info.u16TimingInfo = 1080;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user Timing Info\n"
							 "failed\r\n");
			return -EINVAL;
		}
		ret = copy_to_user(pBuf, &stTiming_Info, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user Timing Info\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

	case V4L2_EXT_HDMI_HDMI_INTERFACE_CMD_GET_HDCP_AUTHVERSION: {
		unsigned long ret;
		stCMD_HDMI_GET_HDCP_AUTHVERSION stHDCP_AuthVer;

		ret = copy_from_user(&stHDCP_AuthVer,
					 pBuf,
					 u32BufSize);
		if (ret == 0) {
			stHDCP_AuthVer.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDCP_AuthVer.enInputPortType);
			MDrv_HDMI_Ctrl(u32Cmd, (void *)&stHDCP_AuthVer, u32BufSize);
#if DEF_TEST_USE
			stHDCP_AuthVer.enHDCPAuthVersion = E_HDCP_2_2;
#endif
		} else {
			HDMIRX_MSG_ERROR("copy_from_user HDCP Auth Version\n"
							 "failed\r\n");
			return -EINVAL;
		}
		ret = copy_to_user(pBuf, &stHDCP_AuthVer, u32BufSize);
		if (ret != 0) {
			HDMIRX_MSG_ERROR("copy_to_user HDCP Auth Version\n"
							 "failed\r\n");
			return -EINVAL;
		}
	} break;

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

	srccap_dev =
		container_of(ctrl->handler, struct mtk_srccap_dev, hdmirx_ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_HDMI_RX_GET_IT_CONTENT_TYPE: {
		ctrl->val = mtk_hdmirx_Content_Type(srccap_dev->srccap_info.src);
#if DEF_TEST_USE
		ctrl->val = 16;
#endif
	} break;

	case V4L2_CID_HDMI_RX_HDCP22_READDONE: {
		if (ctrl->p_new.p != NULL) {
			ST_HDCP22_READDONE stHDCP_Read;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDCP_Read, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDCP22_READDONE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDCP_Read.bReadFlag =
				MDrv_HDCP22_PollingReadDone(stHDCP_Read.u8PortIdx);
#if DEF_TEST_USE
			stHDCP_Read.bReadFlag = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDCP_Read, sizeof(ST_HDCP22_READDONE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_GC_INFO: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_GC_INFO stHDMI_GC;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_GC, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_GC_INFO));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_GC.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_GC.enInputPortType);
			stHDMI_GC.u16GCdata =
				MDrv_HDMI_GC_Info(stHDMI_GC.enInputPortType, stHDMI_GC.gcontrol);
#if DEF_TEST_USE
			stHDMI_GC.u16GCdata = 0x11 + stHDMI_GC.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_GC, sizeof(ST_HDMI_GC_INFO));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_ERR_STATUS: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_ERR_STATUS stErr_status;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stErr_status, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_ERR_STATUS));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stErr_status.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stErr_status.enInputPortType);
			stErr_status.u8RetValue = MDrv_HDMI_err_status_update(
				stErr_status.enInputPortType, stErr_status.u8value,
				stErr_status.bread);
#if DEF_TEST_USE
			stErr_status.u8RetValue = 0x11 + stErr_status.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stErr_status, sizeof(ST_HDMI_ERR_STATUS));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_ACP: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BYTE stHDMI_BYTE;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_BYTE.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_BYTE.enInputPortType);
			stHDMI_BYTE.u8RetValue =
				MDrv_HDMI_audio_cp_hdr_info(stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue = 0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE, sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_PIXEL_REPETITION: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BYTE stHDMI_BYTE;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_BYTE.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_BYTE.enInputPortType);
			stHDMI_BYTE.u8RetValue =
				MDrv_HDMI_Get_Pixel_Repetition(stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue = 0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE, sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_AVI_VER: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BYTE stHDMI_BYTE;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_BYTE.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_BYTE.enInputPortType);
			stHDMI_BYTE.u8RetValue =
				MDrv_HDMI_Get_AVIInfoFrameVer(stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue = 0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE, sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_AVI_ACTIVEINFO: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BOOL stHDMI_Bool;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_Bool, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BOOL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_Bool.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_Bool.enInputPortType);
			stHDMI_Bool.bRet =
				MDrv_HDMI_Get_AVIInfoActiveInfoPresent(stHDMI_Bool.enInputPortType);
#if DEF_TEST_USE
			stHDMI_Bool.bRet = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_Bool, sizeof(ST_HDMI_BOOL));
		}
	} break;

	case V4L2_CID_HDMI_RX_ISHDMIMODE: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BOOL stHDMI_Bool;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_Bool, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BOOL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_Bool.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_Bool.enInputPortType);
			stHDMI_Bool.bRet = MDrv_HDMI_IsHDMI_Mode(stHDMI_Bool.enInputPortType);
#if DEF_TEST_USE
			stHDMI_Bool.bRet = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_Bool, sizeof(ST_HDMI_BOOL));
		}
	} break;

	case V4L2_CID_HDMI_RX_CHECK_4K2K: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BOOL stHDMI_Bool;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_Bool, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BOOL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_Bool.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_Bool.enInputPortType);
			stHDMI_Bool.bRet = MDrv_HDMI_Check4K2K(stHDMI_Bool.enInputPortType);
#if DEF_TEST_USE
			stHDMI_Bool.bRet = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_Bool, sizeof(ST_HDMI_BOOL));
		}
	} break;

	case V4L2_CID_HDMI_RX_CHECK_ADDITION_FMT: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BYTE stHDMI_BYTE;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_BYTE.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_BYTE.enInputPortType);
			stHDMI_BYTE.u8RetValue =
				MDrv_HDMI_Check_Additional_Format(stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue = 0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE, sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_3D_STRUCTURE: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BYTE stHDMI_BYTE;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_BYTE.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_BYTE.enInputPortType);
			stHDMI_BYTE.u8RetValue =
				MDrv_HDMI_Get_3D_Structure(stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue = 0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE, sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_3D_EXT_DATA: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BYTE stHDMI_BYTE;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_BYTE.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_BYTE.enInputPortType);
			stHDMI_BYTE.u8RetValue =
				MDrv_HDMI_Get_3D_Ext_Data(stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue = 0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE, sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_3D_META_FIELD: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_META_FIELD stMeta_field;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stMeta_field, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_META_FIELD));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stMeta_field.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stMeta_field.enInputPortType);
			MDrv_HDMI_Get_3D_Meta_Field(
				stMeta_field.enInputPortType,
				&stMeta_field.st3D_META_DATA);
#if DEF_TEST_USE
			stMeta_field.st3D_META_DATA.t3D_Metadata_Type = E_3D_META_DATA_MAX;
			stMeta_field.st3D_META_DATA.u83D_Metadata_Length =
				0x11 + stMeta_field.enInputPortType;
			stMeta_field.st3D_META_DATA.b3D_Meta_Present = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stMeta_field, sizeof(ST_HDMI_META_FIELD));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_SOURCE_VER: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BYTE stHDMI_BYTE;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_BYTE, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BYTE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_BYTE.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_BYTE.enInputPortType);
			stHDMI_BYTE.u8RetValue =
				MDrv_HDMI_GetSourceVersion(stHDMI_BYTE.enInputPortType);
#if DEF_TEST_USE
			stHDMI_BYTE.u8RetValue = 0x11 + stHDMI_BYTE.enInputPortType;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_BYTE, sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_DE_STABLE_STATUS: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMI_BOOL stHDMI_Bool;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_Bool, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMI_BOOL));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_Bool.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_Bool.enInputPortType);
			stHDMI_Bool.bRet =
				MDrv_HDMI_GetDEStableStatus(stHDMI_Bool.enInputPortType);
#if DEF_TEST_USE
			stHDMI_Bool.bRet = TRUE;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_Bool, sizeof(ST_HDMI_BOOL));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_CRC: {
		if (ctrl->p_new.p != NULL) {
			stCMD_HDMI_CMD_GET_CRC_VALUE stHDMI_CRC_Value;
			__u16 u16crc = 0;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_CRC_Value, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_CRC_Value.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_CRC_Value.enInputPortType);
			stHDMI_CRC_Value.bIsValid =
				MDrv_HDMI_GetCRCValue(
					stHDMI_CRC_Value.enInputPortType,
					stHDMI_CRC_Value.u8Channel, &u16crc);
#if DEF_TEST_USE
			stHDMI_CRC_Value.bIsValid = TRUE;
			u16crc = 0x11 + stHDMI_CRC_Value.enInputPortType;
#endif
			ret = copy_to_user(stHDMI_CRC_Value.u16CRCVal, &u16crc,
							   sizeof(__u16));

			if (ret != 0) {
				HDMIRX_MSG_ERROR("copy_to_user failed\r\n");
				return -EINVAL;
			}
			memcpy(ctrl->p_new.p, &stHDMI_CRC_Value,
				   sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_EMP: {
		if (ctrl->p_new.p != NULL) {
			stCMD_HDMI_GET_EMPacket stHDMI_EMP;
			__u8 u8TotalNum = 0;
			__u8 u8EmpData[HDMI_SINGLE_PKT_SIZE] = {0};
			unsigned long ret = 0;
			bool bret = FALSE;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_EMP, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(stCMD_HDMI_GET_EMPacket));
			HDMIRX_MSG_INFO(
				"Input Port : %d, EMP Type : %X\r\n",
				stHDMI_EMP.enInputPortType, stHDMI_EMP.enEmpType);
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_EMP.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_EMP.enInputPortType);
			bret = MDrv_HDMI_Get_EMP(
				stHDMI_EMP.enInputPortType, stHDMI_EMP.enEmpType,
				stHDMI_EMP.u8CurrentPacketIndex, &u8TotalNum, u8EmpData);
#if DEF_TEST_USE
			stHDMI_EMP.u8CurrentPacketIndex =
				0x11 + stHDMI_EMP.enInputPortType - 0x50;
			u8TotalNum = stHDMI_EMP.enInputPortType;
			u8EmpData[0] = 0x11 + stHDMI_EMP.enInputPortType;
			bret = TRUE;
#endif
			if (bret) {
				ret = copy_to_user(
						stHDMI_EMP.pu8TotalPacketNumber,
						&u8TotalNum, sizeof(__u8));
			}

			if (ret != 0) {
				HDMIRX_MSG_ERROR("copy_to_user EMP total\n"
								 "number failed\r\n");
				return -EINVAL;
			}

			if (bret) {
				ret = copy_to_user(
					stHDMI_EMP.pu8EmPacket,
					(void *)&u8EmpData[0],
					sizeof(u8EmpData));
			}

			if (ret != 0) {
				HDMIRX_MSG_ERROR("copy_to_user EMP data\n"
								 "failed\r\n");
				return -EINVAL;
			}
			memcpy(ctrl->p_new.p, &stHDMI_EMP, sizeof(stCMD_HDMI_GET_EMPacket));
		}
	} break;

	case V4L2_CID_HDMI_RX_CTRL: {
		if (ctrl->p_new.p != NULL) {
			ST_HDMIRX_CTRL stHDMI_CTRL;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_CTRL, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDMIRX_CTRL));
			ret = _mtk_hdmirx_ctrl_process(
						stHDMI_CTRL.u32Cmd,
						stHDMI_CTRL.pBuf,
						stHDMI_CTRL.u32BufSize);
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			memcpy(ctrl->p_new.p, &stHDMI_CTRL, sizeof(ST_HDMIRX_CTRL));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_CABLE_DETECT: {
		// HDMIRX_MSG_ERROR("Get V4L2_CID_HDMI_RX_GET_CABLE_DETECT\n");
		if (ctrl->p_new.p != NULL) {
			int i = 0;
			stCMD_HDMI_GET_SCDC_CABLE_DETECT stCable_Detect;
			struct v4l2_ext_hdmi_cmd_get_cable_detect *v4lCable_Detect =
				(void *)ctrl->p_new.p;

			for (i = 0; i < srccap_dev->hdmirx_info.port_count; i++) {
				stCable_Detect.enInputPortType = INPUT_PORT_DVI0 + i;
				MDrv_HDMI_Ctrl(
					E_HDMI_INTERFACE_CMD_GET_SCDC_CABLE_DETECT,
					(void *)&(stCable_Detect),
					sizeof(stCMD_HDMI_GET_SCDC_CABLE_DETECT));
				v4lCable_Detect[i].enInputPortType =
					V4L2_SRCCAP_INPUT_SOURCE_HDMI + i;
				v4lCable_Detect[i].bCableDetectFlag =
					stCable_Detect.bCableDetectFlag;
			}
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_FS: {
		HDMIRX_MSG_ERROR("Get V4L2_CID_HDMI_RX_GET_FS\n");
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_word stHDMI_WORD;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDMI_WORD, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_word));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			stHDMI_WORD.u16RetValue = MDrv_HDMI_GetDataInfoByPort(
				E_HDMI_GET_FS,
				mtk_hdmirx_to_muxinputport(stHDMI_WORD.enInputPortType));
#if DEF_TEST_USE
			stHDMI_WORD.u16RetValue = 44;
#endif
			memcpy(ctrl->p_new.p, &stHDMI_WORD, sizeof(struct v4l2_ext_hdmi_word));
		}
	} break;

	case V4L2_CID_HDMI_RX_HDCP22_GETMSG: {
		if (ctrl->p_new.p != NULL) {
			ST_HDCP22_MSG stHDCPMsg;
			MS_U8 ucMsgData[HDMI_HDCP22_MESSAGE_LENGTH + 1] = {0};
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy(&stHDCPMsg, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(ST_HDCP22_MSG));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;

			MDrv_HDCP22_RecvMsg(stHDCPMsg.ucPortIdx, ucMsgData);
			stHDCPMsg.dwDataLen
				= ucMsgData[HDMI_HDCP22_MESSAGE_LENGTH];
#if DEF_TEST_USE
			stHDCPMsg.dwDataLen = 21;
			ucMsgData[0] = 21;
			ucMsgData[1] = 0x11 + stHDCPMsg.ucPortIdx;
#endif

			if (stHDCPMsg.dwDataLen > 0) {
				ret = copy_to_user(
						stHDCPMsg.pucData,
						(void *)&ucMsgData[0],
						sizeof(ucMsgData));
				if (ret != 0) {
					HDMIRX_MSG_ERROR("copy_to_user HDCP22\n"
									 "MSG failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, &stHDCPMsg, sizeof(ST_HDCP22_MSG));
		}
	} break;
	case V4L2_CID_HDMI_RX_PKT_GET_AVI:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_AVI(
				my_pkt_s.enInputPortType, (struct hdmi_avi_packet_info *)buf_ptr,
				my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
						sizeof(struct hdmi_avi_packet_info));
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR("copy_to_user PKT_GET_AVI failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_VSIF:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_VSIF(
				my_pkt_s.enInputPortType, (struct hdmi_vsif_packet_info *)buf_ptr,
				my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
						sizeof(struct hdmi_vsif_packet_info) *
						my_pkt_s.u8Ret_n);
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR("copy_to_user PKT_GET_VSIF failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_GCP:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_GCP(
				my_pkt_s.enInputPortType, (struct hdmi_gc_packet_info *)buf_ptr,
				my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
						sizeof(struct hdmi_gc_packet_info) *
						my_pkt_s.u8Ret_n);
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR("copy_to_user GET_GCP failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_HDRIF:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_HDRIF(
				my_pkt_s.enInputPortType, (struct hdmi_hdr_packet_info *)buf_ptr,
				my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
						sizeof(struct hdmi_hdr_packet_info) *
						my_pkt_s.u8Ret_n);
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR("copy_to_user GET_HDRIF failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_VS_EMP:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_VS_EMP(
				my_pkt_s.enInputPortType, (struct hdmi_emp_packet_info *)buf_ptr,
				my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
						sizeof(struct hdmi_emp_packet_info) *
						my_pkt_s.u8Ret_n);
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR("copy_to_user GET_VS_EMP failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_DSC_EMP:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_DSC_EMP(
				my_pkt_s.enInputPortType, (struct hdmi_emp_packet_info *)buf_ptr,
				my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
						sizeof(struct hdmi_emp_packet_info) *
						my_pkt_s.u8Ret_n);
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR("copy_to_user GET_DSC_EMP failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_DYNAMIC_HDR_EMP:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_Dynamic_HDR_EMP(
				my_pkt_s.enInputPortType, (struct hdmi_emp_packet_info *)buf_ptr,
				my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
						sizeof(struct hdmi_emp_packet_info) *
						my_pkt_s.u8Ret_n);
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR(
						"copy_to_user GET_DYNAMIC_HDR_EMP failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_VRR_EMP:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_VRR_EMP(
				my_pkt_s.enInputPortType, (struct hdmi_emp_packet_info *)buf_ptr,
				my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
						sizeof(struct hdmi_emp_packet_info) *
						my_pkt_s.u8Ret_n);
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR("copy_to_user GET_VRR_EMP failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_REPORT:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			struct v4l2_ext_hdmi_pkt_report_s report_buf;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_pkt_s.u8Ret_n =
				(MS_U8)mtk_srccap_hdmirx_pkt_get_report(
					my_pkt_s.enInputPortType,
					my_pkt_s.e_pkt_type, &report_buf);
			ret = copy_to_user(
					my_pkt_s.pBuf, (void *)&report_buf,
					sizeof(struct v4l2_ext_hdmi_pkt_report_s));
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			if (ret != 0) {
				HDMIRX_MSG_ERROR("copy_to_user PKT_GET_REPORT failed\r\n");
				return -EINVAL;
			}
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_IS_DSC:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_bool my_bool_s;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy((void *)&my_bool_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_bool));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_bool_s.bRet = mtk_srccap_hdmirx_pkt_is_dsc(my_bool_s.enInputPortType);
			memcpy(ctrl->p_new.p, (void *)&my_bool_s,
				   sizeof(struct v4l2_ext_hdmi_bool));
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_IS_VRR:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_bool my_bool_s;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_bool_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_bool));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_bool_s.bRet = mtk_srccap_hdmirx_pkt_is_vrr(my_bool_s.enInputPortType);
			memcpy(ctrl->p_new.p, (void *)&my_bool_s,
				   sizeof(struct v4l2_ext_hdmi_bool));
		}
		break;
	case V4L2_CID_HDMI_RX_DSC_GET_PPS_INFO:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_dsc_info report_buf;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&report_buf, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_dsc_info));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			report_buf.bRet = mtk_srccap_hdmirx_dsc_get_pps_info(
								  report_buf.enInputPortType,
								  &report_buf) == TRUE
								  ? 1
								  : 0;
			memcpy(ctrl->p_new.p, (void *)&report_buf,
				   sizeof(struct v4l2_ext_hdmi_dsc_info));
		}
		break;
	case V4L2_CID_HDMI_RX_MUX_SEL_DSC:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_mux_ioctl_s my_mux_s;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_mux_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_mux_ioctl_s));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_mux_s.bRet = mtk_srccap_hdmirx_mux_sel_dsc(
								my_mux_s.enInputPortType,
								my_mux_s.dsc_mux_n);
			memcpy(ctrl->p_new.p, (void *)&my_mux_s,
				   sizeof(struct v4l2_ext_hdmi_mux_ioctl_s));
		}
		break;
	case V4L2_CID_HDMI_RX_MUX_QUERY_DSC:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_mux_ioctl_s my_mux_s;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_mux_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_mux_ioctl_s));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_mux_s.enInputPortType =
				mtk_srccap_hdmirx_mux_query_dsc(my_mux_s.dsc_mux_n);
			memcpy(ctrl->p_new.p, (void *)&my_mux_s,
				   sizeof(struct v4l2_ext_hdmi_mux_ioctl_s));
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_GET_GNL:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_pkt my_pkt_s;
			MS_U8 *buf_ptr = NULL;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_pkt_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			buf_ptr = kmalloc(my_pkt_s.u32BufSize, GFP_KERNEL);
			if (!buf_ptr) {
				HDMIRX_MSG_ERROR("%s,%d, FAIL\n", __func__, __LINE__);
				return -ENOMEM;
			}
			my_pkt_s.u8Ret_n = mtk_srccap_hdmirx_pkt_get_gnl(
				my_pkt_s.enInputPortType, my_pkt_s.e_pkt_type,
				(struct hdmi_packet_info *)buf_ptr, my_pkt_s.u32BufSize);
			if (my_pkt_s.u8Ret_n) {
				ret = copy_to_user(
						my_pkt_s.pBuf, (void *)buf_ptr,
							sizeof(struct hdmi_packet_info) *
							my_pkt_s.u8Ret_n);
				if (ret != 0) {
					kfree(buf_ptr);
					HDMIRX_MSG_ERROR("copy_to_user PKT_GET_GNL failed\r\n");
					return -EINVAL;
				}
			}
			memcpy(ctrl->p_new.p, (void *)&my_pkt_s,
				   sizeof(struct v4l2_ext_hdmi_pkt));
			kfree(buf_ptr);
		}
		break;
	case V4L2_CID_HDMI_RX_AUDIO_STATUS_CLEAR:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_bool my_bool_s;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_bool_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_bool));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_bool_s.bRet =
				mtk_srccap_hdmirx_pkt_audio_status_clr(my_bool_s.enInputPortType);
			memcpy(ctrl->p_new.p, (void *)&my_bool_s,
				   sizeof(struct v4l2_ext_hdmi_bool));
		}
		break;
	case V4L2_CID_HDMI_RX_AUDIO_MUTE:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_mux_ioctl_s my_mux_s;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_mux_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_mux_ioctl_s));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_mux_s.bRet = mtk_srccap_hdmirx_pkt_audio_mute_en(
				my_mux_s.enInputPortType, my_mux_s.u32_param[0] ? true : false);
			memcpy(ctrl->p_new.p, (void *)&my_mux_s,
				   sizeof(struct v4l2_ext_hdmi_mux_ioctl_s));
		}
		break;
	case V4L2_CID_HDMI_RX_GET_SIM_MODE: {
		// HDMIRX_MSG_ERROR("Get V4L2_CID_HDMI_RX_GET_SIM_MODE\n");
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_bool *pv4lCable_bool = (void *)ctrl->p_new.p;

			pv4lCable_bool->bRet = drv_hwreg_srccap_hdmirx_isSimMode();
		}
	} break;
	case V4L2_CID_HDMI_RX_GET_AUDIO_FS:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_get_audio_sampling_freq
				my_audio_fs;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy((void *)&my_audio_fs,
		srccap_dev->hdmirx_info.ucSetBuffer,
		sizeof(struct v4l2_ext_hdmi_get_audio_sampling_freq));

			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;

			my_audio_fs.u32RetValue_hz
		= mtk_srccap_hdmirx_pkt_get_audio_fs_hz(
			my_audio_fs.enInputPortType);

			memcpy(ctrl->p_new.p, (void *)&my_audio_fs,
		sizeof(struct v4l2_ext_hdmi_get_audio_sampling_freq));
		}
		break;
	case V4L2_CID_HDMI_RX_SET_PWR_SAVING_ONOFF:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_set_pwr_saving_onoff
				my_set_pwr_onoff;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy((void *)&my_set_pwr_onoff,
		srccap_dev->hdmirx_info.ucSetBuffer,
		sizeof(struct v4l2_ext_hdmi_set_pwr_saving_onoff));

			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;

			my_set_pwr_onoff.bRet =
				mtk_srccap_hdmirx_set_pwr_saving_onoff(
					my_set_pwr_onoff.enInputPortType,
					my_set_pwr_onoff.b_on);

			memcpy(ctrl->p_new.p, (void *)&my_set_pwr_onoff,
		sizeof(struct v4l2_ext_hdmi_set_pwr_saving_onoff));
		}
		break;
	case V4L2_CID_HDMI_RX_GET_PWR_SAVING_ONOFF:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_get_pwr_saving_onoff
				my_get_pwr_onoff;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy((void *)&my_get_pwr_onoff,
		srccap_dev->hdmirx_info.ucSetBuffer,
		sizeof(struct v4l2_ext_hdmi_get_pwr_saving_onoff));

			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;

			my_get_pwr_onoff.bRet_hwdef
		= mtk_srccap_hdmirx_get_pwr_saving_onoff_hwdef(
			my_get_pwr_onoff.enInputPortType);

			my_get_pwr_onoff.bRet_swprog
		= mtk_srccap_hdmirx_get_pwr_saving_onoff(
			my_get_pwr_onoff.enInputPortType);

			memcpy(ctrl->p_new.p, (void *)&my_get_pwr_onoff,
		sizeof(struct v4l2_ext_hdmi_get_pwr_saving_onoff));
		}
		break;
	case V4L2_CID_HDMI_RX_GET_HDMI_STATUS:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_bool my_bool_s;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
								 "first\r\n");
				return -ENOMEM;
			}

			memcpy((void *)&my_bool_s, srccap_dev->hdmirx_info.ucSetBuffer,
				   sizeof(struct v4l2_ext_hdmi_bool));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_bool_s.bRet =
				mtk_srccap_hdmirx_get_HDMIStatus(my_bool_s.enInputPortType);
			memcpy(ctrl->p_new.p, (void *)&my_bool_s,
				   sizeof(struct v4l2_ext_hdmi_bool));
		}
		break;
	case V4L2_CID_HDMI_RX_GET_FREESYNC_INFO:
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_get_freesync_info
			my_get_freesync_info;
			unsigned long ret;

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Need to set control\n"
						"first\r\n");
				return -ENOMEM;
			}
			memcpy((void *)&my_get_freesync_info, srccap_dev->hdmirx_info.ucSetBuffer,
				sizeof(struct v4l2_ext_hdmi_get_freesync_info));
			kfree(srccap_dev->hdmirx_info.ucSetBuffer);
			srccap_dev->hdmirx_info.ucSetBuffer = NULL;
			my_get_freesync_info.bRet = mtk_srccap_hdmirx_get_freesync_info(
			my_get_freesync_info.enInputPortType, &my_get_freesync_info);
			memcpy(ctrl->p_new.p, (void *)&my_get_freesync_info,
			sizeof(struct v4l2_ext_hdmi_get_freesync_info));
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

	srccap_dev =
		container_of(ctrl->handler, struct mtk_srccap_dev, hdmirx_ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_HDMI_RX_SET_CURRENT_PORT: {
		mtk_hdmirx_set_current_port(srccap_dev);
	} break;

	case V4L2_CID_HDMI_RX_HDCP22_READDONE: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDCP22_READDONE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDCP22_READDONE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_GC_INFO: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_GC_INFO), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_GC_INFO));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_ERR_STATUS: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_ERR_STATUS), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_ERR_STATUS));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_ACP: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BYTE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_PIXEL_REPETITION: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BYTE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_AVI_VER: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BYTE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_AVI_ACTIVEINFO: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BOOL), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BOOL));
		}
	} break;

	case V4L2_CID_HDMI_RX_ISHDMIMODE: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BOOL), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BOOL));
		}
	} break;

	case V4L2_CID_HDMI_RX_CHECK_4K2K: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BOOL), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BOOL));
		}
	} break;

	case V4L2_CID_HDMI_RX_CHECK_ADDITION_FMT: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BYTE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_3D_STRUCTURE: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BYTE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_3D_EXT_DATA: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BYTE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_3D_META_FIELD: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_META_FIELD), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_META_FIELD));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_SOURCE_VER: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BYTE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BYTE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_DE_STABLE_STATUS: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_BOOL), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_BOOL));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_CRC: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(stCMD_HDMI_CMD_GET_CRC_VALUE));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_EMP: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(stCMD_HDMI_GET_EMPacket), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(stCMD_HDMI_GET_EMPacket));
		}
	} break;

	case V4L2_CID_HDMI_RX_CTRL: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMIRX_CTRL), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMIRX_CTRL));
		}
	} break;

	case V4L2_CID_HDMI_RX_GET_FS: {
		HDMIRX_MSG_ERROR("Set V4L2_CID_HDMI_RX_GET_FS\n");
		if (ctrl->p_new.p != NULL) {
			HDMIRX_MSG_ERROR("Not NULL V4L2_CID_HDMI_RX_GET_FS\n");
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDMI_WORD), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDMI_WORD));
		}
	} break;

	case V4L2_CID_HDMI_RX_HDCP22_IRQ_EANBLE: {
		if (ctrl->val > 0) {
			srccap_dev->hdmirx_info.ubHDCP22ISREnable = TRUE;
			if (srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task != NULL) {
				HDMIRX_MSG_ERROR("Enable HDCP22 ISR but thread has created !\r\n");
				kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task);
				srccap_dev->hdmirx_info.ctrl_hdcp2x_irq_monitor_task = NULL;
			}
		} else
			srccap_dev->hdmirx_info.ubHDCP22ISREnable = FALSE;

		MDrv_HDCP22_IRQEnable(srccap_dev->hdmirx_info.ubHDCP22ISREnable);
	} break;

	case V4L2_CID_HDMI_RX_HDCP22_PORTINIT: {
		MDrv_HDCP22_PortInit(ctrl->val);
	} break;

	case V4L2_CID_HDMI_RX_HDCP22_GETMSG: {
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(ST_HDCP22_MSG), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(ST_HDCP22_MSG));

		}
	} break;

	case V4L2_CID_HDMI_RX_HDCP22_SENDMSG: {
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDCP22_MSG stHDCPMsg;
			MS_U8 *u8Msg = NULL;
			unsigned long ret;

			memcpy(&stHDCPMsg, ctrl->p_new.p_u8, sizeof(ST_HDCP22_MSG));
			if (stHDCPMsg.dwDataLen == 0)
				return 0;

			u8Msg = kmalloc(stHDCPMsg.dwDataLen, GFP_KERNEL);

			if (u8Msg != NULL) {
				ret = copy_from_user(
					u8Msg, stHDCPMsg.pucData,
					stHDCPMsg.dwDataLen);
				if (ret != 0) {
					HDMIRX_MSG_ERROR("copy_from_user\n"
									 "failed\r\n");
					kfree(u8Msg);
					return -EINVAL;
				}

				MDrv_HDCP22_SendMsg(0, stHDCPMsg.ucPortIdx, u8Msg,
									stHDCPMsg.dwDataLen, NULL);

				kfree(u8Msg);
			}
		} else
			return -ENOMEM;
	} break;

	case V4L2_CID_HDMI_RX_HDCP14_R0IRQENABLE: {
		if (ctrl->val > 0) {
			srccap_dev->hdmirx_info.ubHDCP14ISREnable = TRUE;
			if (srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task != NULL) {
				HDMIRX_MSG_ERROR("Enable HDCP14 ISR but thread has created !\r\n");
				kthread_stop(srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task);
				srccap_dev->hdmirx_info.ctrl_hdcp1x_irq_monitor_task = NULL;
			}
		} else
			srccap_dev->hdmirx_info.ubHDCP14ISREnable = FALSE;

		MDrv_HDCP14_ReadR0IRQEnable(srccap_dev->hdmirx_info.ubHDCP14ISREnable);
	} break;

	case V4L2_CID_HDMI_RX_SET_EQ_TO_PORT: {
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_EQ stHDMI_EQ;

			memcpy(&stHDMI_EQ, ctrl->p_new.p_u8, sizeof(ST_HDMI_EQ));
			stHDMI_EQ.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_EQ.enInputPortType);
			MDrv_HDMI_Set_EQ_To_Port(stHDMI_EQ.enEq, stHDMI_EQ.u8EQValue,
									 stHDMI_EQ.enInputPortType);
		} else
			return -ENOMEM;
	} break;
	case V4L2_CID_HDMI_RX_HPD_CONTROL: {
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_HPD stHDMI_HPD;

			memcpy(&stHDMI_HPD, ctrl->p_new.p_u8, sizeof(ST_HDMI_HPD));
			stHDMI_HPD.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_HPD.enInputPortType);
			MDrv_HDMI_pullhpd(stHDMI_HPD.bHighLow, stHDMI_HPD.enInputPortType,
							  stHDMI_HPD.bInverse);
		} else
			return -ENOMEM;
	} break;

	case V4L2_CID_HDMI_RX_SW_RESET: {
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_RESET stHDMI_Reset;

			memcpy(&stHDMI_Reset, ctrl->p_new.p_u8, sizeof(ST_HDMI_RESET));
			stHDMI_Reset.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_Reset.enInputPortType);
			MDrv_DVI_Software_Reset(stHDMI_Reset.enInputPortType,
									stHDMI_Reset.u16Reset);
		} else
			return -ENOMEM;
	} break;

	case V4L2_CID_HDMI_RX_PKT_RESET: {
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_PACKET_RESET stHDMI_PKT_Reset;

			memcpy(&stHDMI_PKT_Reset, ctrl->p_new.p_u8,
				   sizeof(ST_HDMI_PACKET_RESET));
			stHDMI_PKT_Reset.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_PKT_Reset.enInputPortType);
			MDrv_HDMI_PacketReset(stHDMI_PKT_Reset.enInputPortType,
								  stHDMI_PKT_Reset.enResetType);
		} else
			return -ENOMEM;
	} break;

	case V4L2_CID_HDMI_RX_CLKRTERM_CONTROL: {
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_CLKRTERM stHDMI_CLK;

			memcpy(&stHDMI_CLK, ctrl->p_new.p_u8, sizeof(ST_HDMI_CLKRTERM));
			stHDMI_CLK.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_CLK.enInputPortType);
			MDrv_DVI_ClkPullLow(stHDMI_CLK.bPullLow, stHDMI_CLK.enInputPortType);
		} else
			return -ENOMEM;
	} break;

	case V4L2_CID_HDMI_RX_ARCPIN_CONTROL: {
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDCP_HDMI_ENABLE stHDCP_ENABLE;

			memcpy(&stHDCP_ENABLE, ctrl->p_new.p_u8, sizeof(ST_HDCP_HDMI_ENABLE));
			stHDCP_ENABLE.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDCP_ENABLE.enInputPortType);
			MDrv_HDMI_ARC_PINControl(stHDCP_ENABLE.enInputPortType,
									 stHDCP_ENABLE.bEnable, 0);
		} else
			return -ENOMEM;
	} break;
	case V4L2_CID_HDMI_RX_SET_DITHER: {
		if (ctrl->p_new.p_u8 != NULL) {
			ST_HDMI_DITHER stHDMI_Dither;

			memcpy(&stHDMI_Dither, ctrl->p_new.p_u8, sizeof(ST_HDMI_DITHER));
			stHDMI_Dither.enInputPortType = mtk_hdmirx_to_muxinputport(
				(enum v4l2_srccap_input_source)stHDMI_Dither.enInputPortType);
			MDrv_HDMI_Dither_Set(stHDMI_Dither.enInputPortType,
								 stHDMI_Dither.enDitherType,
								 stHDMI_Dither.ubRoundEnable);
		} else
			return -ENOMEM;
	} break;
	case V4L2_CID_HDMI_RX_PKT_GET_AVI:
	case V4L2_CID_HDMI_RX_PKT_GET_VSIF:
	case V4L2_CID_HDMI_RX_PKT_GET_GCP:
	case V4L2_CID_HDMI_RX_PKT_GET_HDRIF:
	case V4L2_CID_HDMI_RX_PKT_GET_VS_EMP:
	case V4L2_CID_HDMI_RX_PKT_GET_DSC_EMP:
	case V4L2_CID_HDMI_RX_PKT_GET_DYNAMIC_HDR_EMP:
	case V4L2_CID_HDMI_RX_PKT_GET_VRR_EMP:
	case V4L2_CID_HDMI_RX_PKT_GET_REPORT:
	case V4L2_CID_HDMI_RX_PKT_GET_GNL:
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(struct v4l2_ext_hdmi_pkt), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(struct v4l2_ext_hdmi_pkt));
		}
		break;
	case V4L2_CID_HDMI_RX_DSC_GET_PPS_INFO:
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(struct v4l2_ext_hdmi_dsc_info), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(struct v4l2_ext_hdmi_dsc_info));
		}
		break;
	case V4L2_CID_HDMI_RX_MUX_SEL_DSC:
	case V4L2_CID_HDMI_RX_MUX_QUERY_DSC:
	case V4L2_CID_HDMI_RX_AUDIO_MUTE:
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(struct v4l2_ext_hdmi_mux_ioctl_s), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(struct v4l2_ext_hdmi_mux_ioctl_s));
		}
		break;
	case V4L2_CID_HDMI_RX_PKT_IS_DSC:
	case V4L2_CID_HDMI_RX_PKT_IS_VRR:
	case V4L2_CID_HDMI_RX_AUDIO_STATUS_CLEAR:
	case V4L2_CID_HDMI_RX_GET_HDMI_STATUS:
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(struct v4l2_ext_hdmi_bool), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(struct v4l2_ext_hdmi_bool));
		}
		break;
	case V4L2_CID_HDMI_RX_SET_SIM_MODE: {
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_bool shdmi_bool;

			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			memcpy(&shdmi_bool, ctrl->p_new.p, sizeof(struct v4l2_ext_hdmi_bool));
			HDMIRX_MSG_DBG("bRet [%d] \r\n", shdmi_bool.bRet);

			if (shdmi_bool.bRet) {
				_hdmirx_irq_suspend(srccap_dev);
				MDrv_HDMI_STREventProc(E_POWER_SUSPEND, &stHDMIRxBank);
			}

			drv_hwreg_srccap_hdmirx_setSimMode(shdmi_bool.bRet);

			if (!shdmi_bool.bRet) {
				MDrv_HDMI_STREventProc(E_POWER_RESUME, &stHDMIRxBank);
				_hdmirx_irq_resume(srccap_dev);
			}
		}
	} break;
	case V4L2_CID_HDMI_RX_SET_SIM_DATA: {
		if (ctrl->p_new.p != NULL) {
			struct v4l2_ext_hdmi_sim_data *pshdmi_sim_data;

			pshdmi_sim_data = kmalloc(sizeof(struct v4l2_ext_hdmi_sim_data),
				GFP_KERNEL);

			if (pshdmi_sim_data == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			memcpy(pshdmi_sim_data, ctrl->p_new.p,
				   sizeof(struct v4l2_ext_hdmi_sim_data));
			HDMIRX_MSG_DBG("u8data [%p] \r\n", pshdmi_sim_data->u8data);
			drv_hwreg_srccap_hdmirx_setSimData(pshdmi_sim_data->u8data);
			kfree(pshdmi_sim_data);
		}
	} break;
	case V4L2_CID_HDMI_RX_GET_AUDIO_FS:
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
		kmalloc(sizeof(struct v4l2_ext_hdmi_get_audio_sampling_freq),
				GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(
			srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
			sizeof(struct v4l2_ext_hdmi_get_audio_sampling_freq));
		}
		break;
	case V4L2_CID_HDMI_RX_SET_PWR_SAVING_ONOFF:
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
		kmalloc(sizeof(struct v4l2_ext_hdmi_set_pwr_saving_onoff),
				GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(
			srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
			sizeof(struct v4l2_ext_hdmi_set_pwr_saving_onoff));
		}
		break;
	case V4L2_CID_HDMI_RX_GET_PWR_SAVING_ONOFF:
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
		kmalloc(sizeof(struct v4l2_ext_hdmi_get_pwr_saving_onoff),
				GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(
			srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
			sizeof(struct v4l2_ext_hdmi_get_pwr_saving_onoff));
		}
		break;
	case V4L2_CID_HDMI_RX_GET_FREESYNC_INFO:
		if (ctrl->p_new.p != NULL) {
			if (srccap_dev->hdmirx_info.ucSetBuffer != NULL)
				kfree(srccap_dev->hdmirx_info.ucSetBuffer);

			srccap_dev->hdmirx_info.ucSetBuffer =
				kmalloc(sizeof(struct v4l2_ext_hdmi_get_freesync_info), GFP_KERNEL);

			if (srccap_dev->hdmirx_info.ucSetBuffer == NULL) {
				HDMIRX_MSG_ERROR("Get buffer memory Error\r\n");
				return -ENOMEM;
			}

			memcpy(srccap_dev->hdmirx_info.ucSetBuffer, ctrl->p_new.p,
				   sizeof(struct v4l2_ext_hdmi_get_freesync_info));
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_CABLE_DETECT,
		.name = "HDMI Rx Get Cable Detect",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_cmd_get_cable_detect) * 4},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.name = "Audio Status Clear",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_bool)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_mux_ioctl_s)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_AVI,
		.name = "HDMI Get PKT AVI",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_VSIF,
		.name = "HDMI Get PKT VSIF",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_GCP,
		.name = "HDMI Get PKT GCP",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_HDRIF,
		.name = "HDMI Get PKT HDRIF",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_VS_EMP,
		.name = "HDMI Get PKT VS EMP",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_DSC_EMP,
		.name = "HDMI Get PKT DSC EMP",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_DYNAMIC_HDR_EMP,
		.name = "HDMI Get PKT DYNAMIC HDR EMP",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_VRR_EMP,
		.name = "HDMI Get PKT VRR EMP",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_REPORT,
		.name = "HDMI Get PKT REPORT",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_IS_DSC,
		.name = "HDMI PKT IS DSC",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_bool)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_IS_VRR,
		.name = "HDMI PKT IS VRR",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_bool)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_DSC_GET_PPS_INFO,
		.name = "HDMI DSC GET PPS INFO",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_dsc_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_MUX_SEL_DSC,
		.name = "HDMI MUX SEL DSC",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_mux_ioctl_s)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_MUX_QUERY_DSC,
		.name = "HDMI MUX QUERY DSC",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_mux_ioctl_s)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_PKT_GET_GNL,
		.name = "HDMI  Get PKT General",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_pkt)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_SET_SIM_MODE,
		.name = "HDMI Set Sim Mode",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_bool)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_SET_SIM_DATA,
		.name = "HDMI Set Sim Data",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_sim_data)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_SIM_MODE,
		.name = "HDMI Get Sim Mode",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_bool)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_AUDIO_FS,
		.name = "HDMI GET AUDIO SAMPLING FREQUENCY HZ",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_get_audio_sampling_freq)},
		.flags = V4L2_CTRL_FLAG_VOLATILE
			| V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_SET_PWR_SAVING_ONOFF,
		.name = "HDMI SET PWR SAVING ON/OFF",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_set_pwr_saving_onoff)},
		.flags = V4L2_CTRL_FLAG_VOLATILE
			| V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_PWR_SAVING_ONOFF,
		.name = "HDMI GET PWR SAVING ON/OFF",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_get_pwr_saving_onoff)},
		.flags = V4L2_CTRL_FLAG_VOLATILE
			| V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_HDMI_STATUS,
		.name = "HDMI GET HDMI STATUS",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_bool)},
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &hdmirx_ctrl_ops,
		.id = V4L2_CID_HDMI_RX_GET_FREESYNC_INFO,
		.name = "HDMI GET FREESYNC INFO",
		.type = V4L2_CTRL_COMPOUND_TYPES,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_hdmi_get_freesync_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE
			| V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops hdmirx_sd_ops = {};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops hdmirx_sd_internal_ops = {};

int mtk_srccap_register_hdmirx_subdev(
	struct v4l2_device *v4l2_dev,
	struct v4l2_subdev *hdmirx_subdev,
	struct v4l2_ctrl_handler *hdmirx_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(hdmirx_ctrl) / sizeof(struct v4l2_ctrl_config);
	HDMIRX_MSG_DBG("mdbgin_%s: start \r\n", __func__);
	v4l2_ctrl_handler_init(hdmirx_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(hdmirx_ctrl_handler, &hdmirx_ctrl[ctrl_count], NULL);
	}

	ret = hdmirx_ctrl_handler->error;
	if (ret) {
		SRCCAP_MSG_ERROR("failed to create hdmirx ctrl handler\n");
		goto exit;
	}

	hdmirx_subdev->ctrl_handler = hdmirx_ctrl_handler;

	v4l2_subdev_init(hdmirx_subdev, &hdmirx_sd_ops);
	hdmirx_subdev->internal_ops = &hdmirx_sd_internal_ops;
	strlcpy(hdmirx_subdev->name, "mtk-hdmirx", sizeof(hdmirx_subdev->name));

	ret = v4l2_device_register_subdev(v4l2_dev, hdmirx_subdev);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to register hdmirx subdev\n");
		goto exit;
	}
	return 0;

exit:
	v4l2_ctrl_handler_free(hdmirx_ctrl_handler);
	HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, ret);

	return ret;
}

void mtk_srccap_unregister_hdmirx_subdev(struct v4l2_subdev *hdmirx_subdev)
{
	HDMIRX_MSG_DBG("mdbgin_%s: start \r\n", __func__);
	v4l2_ctrl_handler_free(hdmirx_subdev->ctrl_handler);
	v4l2_device_unregister_subdev(hdmirx_subdev);
}

static int mtk_srccap_read_dts_u32(struct device *property_dev,
			struct device_node *node, char *s, u32 *val)
{
	int ret = 0;

	if (node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (s == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (val == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	ret = of_property_read_u32(node, s, val);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	HDMIRX_MSG_INFO("%s = 0x%x(%u)\n", s, *val, *val);
	return ret;
}

int mtk_srccap_hdmirx_parse_dts_reg(struct mtk_srccap_dev *srccap_dev,
									struct device *property_dev)
{
#define BUF_LEN_MAX 20
	int ret = 0;
	struct device_node *hdmirx_node;
	struct device_node *capability_node;
	struct device_node *bank_node;

	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);
	if (srccap_dev->dev_id == 0) {
		struct of_mmap_info_data of_mmap_info_hdmi = {0};

		memset(&stHDMIRxBank, 0, sizeof(stHDMIRx_Bank));

		hdmirx_node = of_find_node_by_name(property_dev->of_node, "hdmirx");
		if (hdmirx_node == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, -ENOENT);
			return -ENOENT;
		}

		of_mtk_get_reserved_memory_info_by_idx(hdmirx_node, 0, &of_mmap_info_hdmi);
		stHDMIRxBank.pExchangeDataPA = (u8 *)of_mmap_info_hdmi.start_bus_address;
		stHDMIRxBank.u32ExchangeDataSize = of_mmap_info_hdmi.buffer_size;

		HDMIRX_MSG_INFO("ExchangeDataPA is 0x%llX\n",
						(u64)stHDMIRxBank.pExchangeDataPA);
		HDMIRX_MSG_INFO("ExchangeDataSize is 0x%X\n",
						(u32)stHDMIRxBank.u32ExchangeDataSize);

		ret |= get_memory_info((u64 *)&stHDMIRxBank.pMemoryBusBase);
		HDMIRX_MSG_INFO("MemoryBusBase is 0x%llX\n",
						(u64)stHDMIRxBank.pMemoryBusBase);

		capability_node = of_find_node_by_name(property_dev->of_node, "capability");
		if (capability_node == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, -ENOENT);
			return -ENOENT;
		}

		bank_node = of_find_node_by_name(hdmirx_node, "banks");
		if (bank_node == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, -ENOENT);
			return -ENOENT;
		}

		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_hdmirx",
					&stHDMIRxBank.bank_hdmirx);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_hdmirx_end",
					&stHDMIRxBank.bank_hdmirx_end);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_pm_slp",
					&stHDMIRxBank.bank_pm_slp);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_ddc",
					&stHDMIRxBank.bank_ddc);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_scdc_p0",
					&stHDMIRxBank.bank_scdc_p0);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_scdc_xa8_p0",
					&stHDMIRxBank.bank_scdc_xa8_p0);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_scdc_p1",
					&stHDMIRxBank.bank_scdc_p1);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_scdc_xa8_p1",
					&stHDMIRxBank.bank_scdc_xa8_p1);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_scdc_p2",
					&stHDMIRxBank.bank_scdc_p2);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_scdc_xa8_p2",
					&stHDMIRxBank.bank_scdc_xa8_p2);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_scdc_p3",
					&stHDMIRxBank.bank_scdc_p3);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_scdc_xa8_p3",
					&stHDMIRxBank.bank_scdc_xa8_p3);
		ret |=
			mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_hdmirxphy_pm_p0",
					&stHDMIRxBank.bank_hdmirxphy_pm_p0);
		ret |=
			mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_hdmirxphy_pm_p1",
					&stHDMIRxBank.bank_hdmirxphy_pm_p1);
		ret |=
			mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_hdmirxphy_pm_p2",
					&stHDMIRxBank.bank_hdmirxphy_pm_p2);
		ret |=
			mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_hdmirxphy_pm_p3",
					&stHDMIRxBank.bank_hdmirxphy_pm_p3);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_efuse_0",
					&stHDMIRxBank.bank_efuse_0);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_pm_top",
					&stHDMIRxBank.bank_pm_top);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_pmux",
					 &stHDMIRxBank.bank_pmux);
		ret |= mtk_srccap_read_dts_u32(property_dev, bank_node, "bank_dig_mux",
					&stHDMIRxBank.bank_dig_mux);

		ret |= mtk_srccap_read_dts_u32(property_dev, capability_node, "hdmi_count",
					&srccap_dev->hdmirx_info.port_count);
		stHDMIRxBank.port_num = srccap_dev->hdmirx_info.port_count;

		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, -ENOENT);

			return -ENOENT;
		}

		srccap_dev->hdmirx_info.hdmi_r2_ck =
			devm_clk_get(property_dev, "hdmi_r2_ck");
		if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.hdmi_r2_ck)) {
			HDMIRX_MSG_ERROR("Get HDMIRx get hdmi_r2_ck failed!!\r\n");
			ret = PTR_ERR(srccap_dev->hdmirx_info.hdmi_r2_ck);
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, ret);

			return ret;
		}
		srccap_dev->hdmirx_info.hdmi_mcu_ck =
			devm_clk_get(property_dev, "hdmi_mcu_ck");
		if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.hdmi_mcu_ck)) {
			HDMIRX_MSG_ERROR("Get HDMIRx get hdmi_mcu_ck failed!!\r\n");
			ret = PTR_ERR(srccap_dev->hdmirx_info.hdmi_mcu_ck);
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, ret);

			return ret;
		}
		srccap_dev->hdmirx_info.hdmi_en_clk_xtal =
			devm_clk_get(property_dev, "hdmi_en_clk_xtal");
		if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.hdmi_en_clk_xtal)) {
			HDMIRX_MSG_ERROR("Get HDMIRx get hdmi_en_clk_xtal failed!!\r\n");
			ret = PTR_ERR(srccap_dev->hdmirx_info.hdmi_en_clk_xtal);
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, ret);

			return ret;
		}
		srccap_dev->hdmirx_info.hdmi_en_clk_r2_hdmi2mac =
			devm_clk_get(property_dev, "hdmi_en_clk_r2_hdmi2mac");
		if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.hdmi_en_clk_r2_hdmi2mac)) {
			HDMIRX_MSG_ERROR("Get HDMIRx get hdmi_en_clk_r2_hdmi2mac failed!!\r\n");
			ret = PTR_ERR(srccap_dev->hdmirx_info.hdmi_en_clk_r2_hdmi2mac);
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, ret);

			return ret;
		}
		srccap_dev->hdmirx_info.hdmi_en_smi2mac =
			devm_clk_get(property_dev, "hdmi_en_smi2mac");
		if (IS_ERR_OR_NULL(srccap_dev->hdmirx_info.hdmi_en_smi2mac)) {
			HDMIRX_MSG_ERROR("Get HDMIRx get hdmi_en_smi2mac failed!!\r\n");
			ret = PTR_ERR(srccap_dev->hdmirx_info.hdmi_en_smi2mac);
			HDMIRX_MSG_ERROR("mdbgerr_%s: %d\r\n", __func__, ret);
		}
	}

	return ret;
}

static int _hdmirx_enable(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	u32 hdmi_mcu_clk_rate = 0;
	u32 hdmi_r2_clk_rate = 0;

	ret = clk_prepare_enable(srccap_dev->hdmirx_info.hdmi_mcu_ck);
	hdmi_mcu_clk_rate =
		clk_round_rate(srccap_dev->hdmirx_info.hdmi_mcu_ck, 24000000);
	ret = clk_set_rate(srccap_dev->hdmirx_info.hdmi_mcu_ck, hdmi_mcu_clk_rate);
	clk_prepare_enable(srccap_dev->hdmirx_info.hdmi_en_clk_xtal);
	clk_prepare_enable(srccap_dev->hdmirx_info.hdmi_en_smi2mac);

	clk_prepare_enable(srccap_dev->hdmirx_info.hdmi_r2_ck);
	hdmi_r2_clk_rate = clk_round_rate(srccap_dev->hdmirx_info.hdmi_r2_ck, 432000000);
	ret = clk_set_rate(srccap_dev->hdmirx_info.hdmi_r2_ck, hdmi_r2_clk_rate);
	clk_prepare_enable(srccap_dev->hdmirx_info.hdmi_en_clk_r2_hdmi2mac);

	return ret;
}

static int _hdmirx_disable(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);

	clk_disable_unprepare(srccap_dev->hdmirx_info.hdmi_en_clk_r2_hdmi2mac);
	clk_disable_unprepare(srccap_dev->hdmirx_info.hdmi_r2_ck);
	clk_disable_unprepare(srccap_dev->hdmirx_info.hdmi_en_smi2mac);
	clk_disable_unprepare(srccap_dev->hdmirx_info.hdmi_en_clk_xtal);
	clk_disable_unprepare(srccap_dev->hdmirx_info.hdmi_mcu_ck);

	return ret;
}

int _hdmirx_hw_init(struct mtk_srccap_dev *srccap_dev, int hw_version)
{
	int ret = 0;

	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);

	if (!gHDMIRxInitFlag) {
		pV4l2SrccapPortMap = (struct srccap_port_map *)srccap_dev->srccap_info.map;

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
		MDrv_HDMI_init(hw_version, &stHDMIRxBank);
#endif
		gHDMIRxInitFlag = TRUE;
	}

	return ret;
}

static int _hdmirx_hw_deinit(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);

	if (gHDMIRxInitFlag) {
		if (!MDrv_HDMI_deinit(
			srccap_dev->srccap_info.cap.u32HDMI_Srccap_HWVersion,
			&stHDMIRxBank))
			ret = -1;

		gHDMIRxInitFlag = FALSE;
	} else
		ret = -1;
	return ret;
}

static irqreturn_t mtk_hdmirx_isr_phy(int irq, void *priv)
{
	unsigned int mask_save =
		MDrv_HDMI_IRQ_MASK_SAVE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_PHY));

	MDrv_HDMI_ISR_PHY();
	MDrv_HDMI_IRQ_MASK_RESTORE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_PHY),
							   mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}


static MS_BOOL mtk_hdmirx_hdcp_irq_evt(void)
{

	E_MUX_INPUTPORT enInputPortType = INPUT_PORT_DVI0;
	MS_BOOL ret_b = FALSE;

	for (enInputPortType = INPUT_PORT_DVI0;
			 enInputPortType <= INPUT_PORT_DVI_END;
			 enInputPortType++) {
		if (drv_hwreg_srccap_hdmirx_hdcp_irq_evt(enInputPortType)) {
			ret_b = TRUE;
			break;
		}
	}
	return ret_b;
}

static void _irq_handler_mac_dec_misc(
	void *priv,
	E_MUX_INPUTPORT enInputPortType,
	enum E_MAC_TOP_ISR_EVT e_evt)
{
	// px_dec handler
	drv_hwreg_srccap_hdmirx_mac_isr(enInputPortType, e_evt);
	// drv_hdmirx_irq_frl();
	// drv_hdmirx_irq_pkt();
	// drv_hdmirx_irq_tmds();
	// drv_hdmirx_irq_hdcp();
}

static void _irq_handler_mac_dep_misc(
	void *priv,
	E_MUX_INPUTPORT enInputPortType,
	enum E_MAC_TOP_ISR_EVT e_evt)
{

	MS_BOOL b_save = drv_hwreg_srccap_hdmirx_pkt_get_emp_irq_onoff(
			enInputPortType,
			E_EMP_MTW_PULSE);

	MS_U16 u16_sts = drv_hwreg_srccap_hdmirx_pkt_get_emp_evt(
			enInputPortType,
			E_EMP_MTW_PULSE);

	drv_hwreg_srccap_hdmirx_pkt_set_emp_irq_onoff(
			enInputPortType,
			E_EMP_MTW_PULSE,
			FALSE);

	if (b_save && u16_sts) {
		drv_hwreg_srccap_hdmirx_pkt_clr_emp_evt(
			enInputPortType,
			E_EMP_MTW_PULSE);
#ifdef HDCP22_REAUTH_ACCU
		drv_hwreg_srccap_hdmirx_hdcp_isr(
			(MS_U8)(enInputPortType - INPUT_PORT_DVI0),
			E_HDMI_HDCP22_CLR_BCH_ERR,
			(void *)NULL);
#endif
		mtk_hdmirx_ctrl_vtem_info_monitor_task(
			priv, enInputPortType);
	}
	drv_hwreg_srccap_hdmirx_mac_isr(enInputPortType, e_evt);
	// drv_hdmirx_irq_pkt();

	drv_hwreg_srccap_hdmirx_pkt_set_emp_irq_onoff(
			enInputPortType,
			E_EMP_MTW_PULSE,
			b_save);
}

static void _irq_handler_mac_dtop_misc(void *priv)
{
}

static void _irq_handler_mac_inner_misc(void *priv)
{
}

static void _irq_handler_mac_dscd(void *priv)
{
}

static void _irq_handler_mac_dec_misc_p0(void *priv)
{
	_irq_handler_mac_dec_misc(priv, INPUT_PORT_DVI0, E_DEC_MISC_P0);
}

static void _irq_handler_mac_dec_misc_p1(void *priv)
{
	_irq_handler_mac_dec_misc(priv, INPUT_PORT_DVI1, E_DEC_MISC_P1);
}
static void _irq_handler_mac_dec_misc_p2(void *priv)
{
	_irq_handler_mac_dec_misc(priv, INPUT_PORT_DVI2, E_DEC_MISC_P2);
}

static void _irq_handler_mac_dec_misc_p3(void *priv)
{
	_irq_handler_mac_dec_misc(priv, INPUT_PORT_DVI3, E_DEC_MISC_P3);
}

static void _irq_handler_mac_dep_misc_p0(void *priv)
{
	_irq_handler_mac_dep_misc(priv, INPUT_PORT_DVI0, E_DEP_MISC_P0);
}

static void _irq_handler_mac_dep_misc_p1(void *priv)
{
	_irq_handler_mac_dep_misc(priv, INPUT_PORT_DVI1, E_DEP_MISC_P1);
}
static void _irq_handler_mac_dep_misc_p2(void *priv)
{
	_irq_handler_mac_dep_misc(priv, INPUT_PORT_DVI2, E_DEP_MISC_P2);
}

static void _irq_handler_mac_dep_misc_p3(void *priv)
{
	_irq_handler_mac_dep_misc(priv, INPUT_PORT_DVI3, E_DEP_MISC_P3);
}

static irqreturn_t mtk_hdmirx_isr_mac(int irq, void *priv)
{
	unsigned int mask_save =
		drv_hwreg_srccap_hdmirx_irq_mask_save_global(
		_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_MAC));

	unsigned int evt = drv_hwreg_srccap_hdmirx_irq_status(
			_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_MAC),
			INPUT_PORT_DVI0);

	unsigned int evt_2 = (~mask_save) & evt;
	unsigned char u8_bit = 0;

	if (evt_2 &
	((1<<E_DEC_MISC_P0) | (1<<E_DEC_MISC_P1)
	| (1<<E_DEC_MISC_P2) | (1<<E_DEC_MISC_P3))) {

		if (mtk_hdmirx_hdcp_irq_evt())
			mtk_hdmirx_hdcp_isr(irq, priv);

	}

	while (evt_2 && u8_bit < E_MAC_TOP_ISR_EVT_N) {
		if (evt_2 & HDMI_BIT0) {
			if (_irq_handler_mac_top[u8_bit])
				_irq_handler_mac_top[u8_bit](priv);
			else
				HDMIRX_MSG_ERROR("\r\n");
		}
		evt_2 >>= 1;
		u8_bit++;
	};

	evt_2 = (~mask_save) & evt;

	drv_hwreg_srccap_hdmirx_irq_clr(
		_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_MAC),
		INPUT_PORT_DVI0,
		evt_2);

	drv_hwreg_srccap_hdmirx_irq_mask_restore_global(
		_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_MAC),
		mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_hdmirx_isr_pkt_que(int irq, void *priv)
{
	unsigned int mask_save =
		MDrv_HDMI_IRQ_MASK_SAVE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_PKT_QUE));

	MDrv_HDMI_ISR_PKT_QUE();
	MDrv_HDMI_IRQ_MASK_RESTORE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_PKT_QUE),
							   mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_hdmirx_isr_scdc(int irq, void *priv)
{
	unsigned int mask_save =
		MDrv_HDMI_IRQ_MASK_SAVE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_PM_SCDC));

	MDrv_HDMI_ISR_SCDC();
	MDrv_HDMI_IRQ_MASK_RESTORE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_PM_SCDC),
							   mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_hdmirx_isr_sqh_all_wk(int irq, void *priv)
{
	unsigned int mask_save = MDrv_HDMI_IRQ_MASK_SAVE(
		_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_PM_SQH_ALL_WK));

	MDrv_HDMI_ISR_SQH_ALL_WK();
	MDrv_HDMI_IRQ_MASK_RESTORE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_IRQ_PM_SQH_ALL_WK),
							   mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_hdmirx_isr_clk_det_0(int irq, void *priv)
{
	unsigned int mask_save =
		MDrv_HDMI_IRQ_MASK_SAVE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_FIQ_CLK_DET_0));

	MDrv_HDMI_ISR_CLK_DET_0();
	MDrv_HDMI_IRQ_MASK_RESTORE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_FIQ_CLK_DET_0),
							   mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_hdmirx_isr_clk_det_1(int irq, void *priv)
{
	unsigned int mask_save =
		MDrv_HDMI_IRQ_MASK_SAVE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_FIQ_CLK_DET_1));

	MDrv_HDMI_ISR_CLK_DET_1();
	MDrv_HDMI_IRQ_MASK_RESTORE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_FIQ_CLK_DET_1),
							   mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_hdmirx_isr_clk_det_2(int irq, void *priv)
{
	unsigned int mask_save =
		MDrv_HDMI_IRQ_MASK_SAVE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_FIQ_CLK_DET_2));

	MDrv_HDMI_ISR_CLK_DET_2();
	MDrv_HDMI_IRQ_MASK_RESTORE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_FIQ_CLK_DET_2),
							   mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_hdmirx_isr_clk_det_3(int irq, void *priv)
{
	unsigned int mask_save =
		MDrv_HDMI_IRQ_MASK_SAVE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_FIQ_CLK_DET_3));

	MDrv_HDMI_ISR_CLK_DET_3();
	MDrv_HDMI_IRQ_MASK_RESTORE(_mtk_hdmirx_getKDrvInt(SRCCAP_HDMI_FIQ_CLK_DET_3),
							   mask_save);
	// HDMIRX_MSG_DBG("%s,irq=%d\n", __func__, irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_srccap_hdmirx_isr_entry(int irq, void *priv)
{
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)priv;
	struct srccap_hdmirx_interrupt *p_hdmi_int_s =
		&srccap_dev->hdmirx_info.hdmi_int_s;
	irqreturn_t ret_val = IRQ_NONE;
	int idx = 0;

	for (idx = SRCCAP_HDMI_IRQ_START; idx < SRCCAP_HDMI_IRQ_END; idx++) {
		if (irq == p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START]) {
			if (p_hdmi_int_s->int_func[idx - SRCCAP_HDMI_IRQ_START]) {
				ret_val =
				p_hdmi_int_s->int_func[idx - SRCCAP_HDMI_IRQ_START](irq, priv);
			}
			break;
		}
	}
	return ret_val;
}

static void _hdmirx_isr_handler_setup(struct srccap_hdmirx_interrupt *p_hdmi_int_s)
{
	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);
	if (p_hdmi_int_s) {
		p_hdmi_int_s->int_func[SRCCAP_HDMI_IRQ_PHY - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_phy;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_IRQ_MAC - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_mac;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_IRQ_PKT_QUE - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_pkt_que;
		p_hdmi_int_s
			->int_func[SRCCAP_HDMI_IRQ_PM_SQH_ALL_WK - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_sqh_all_wk;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_IRQ_PM_SCDC - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_scdc;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_FIQ_CLK_DET_0 - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_clk_det_0;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_FIQ_CLK_DET_1 - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_clk_det_1;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_FIQ_CLK_DET_2 - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_clk_det_2;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_FIQ_CLK_DET_3 - SRCCAP_HDMI_IRQ_START] =
			mtk_hdmirx_isr_clk_det_3;
	}
}

static void _hdmirx_isr_handler_release(struct srccap_hdmirx_interrupt *p_hdmi_int_s)
{
	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);
	if (p_hdmi_int_s) {
		p_hdmi_int_s->int_func[SRCCAP_HDMI_IRQ_PHY - SRCCAP_HDMI_IRQ_START] = NULL;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_IRQ_MAC - SRCCAP_HDMI_IRQ_START] = NULL;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_IRQ_PKT_QUE - SRCCAP_HDMI_IRQ_START] =
			NULL;
		p_hdmi_int_s
			->int_func[SRCCAP_HDMI_IRQ_PM_SQH_ALL_WK - SRCCAP_HDMI_IRQ_START] = NULL;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_IRQ_PM_SCDC - SRCCAP_HDMI_IRQ_START] =
			NULL;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_FIQ_CLK_DET_0 - SRCCAP_HDMI_IRQ_START] =
			NULL;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_FIQ_CLK_DET_1 - SRCCAP_HDMI_IRQ_START] =
			NULL;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_FIQ_CLK_DET_2 - SRCCAP_HDMI_IRQ_START] =
			NULL;
		p_hdmi_int_s->int_func[SRCCAP_HDMI_FIQ_CLK_DET_3 - SRCCAP_HDMI_IRQ_START] =
			NULL;
	}
}

static int _hdmirx_interrupt_register(struct mtk_srccap_dev *srccap_dev,
				struct platform_device *pdev)
{
	struct srccap_hdmirx_interrupt *p_hdmi_int_s =
		&srccap_dev->hdmirx_info.hdmi_int_s;
	int ret = 0;
	int ret_val = 0;
	int idx = 0;
	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);
	if (srccap_dev->dev_id == 0) {
		for (idx = SRCCAP_HDMI_IRQ_START; idx < SRCCAP_HDMI_IRQ_END; idx++) {
			p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START] =
				platform_get_irq(pdev, idx);
			if (p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START] < 0)
				HDMIRX_MSG_ERROR("Failed to get irq, hdmi index=%d\r\n", idx);
			else {
				HDMIRX_MSG_INFO("irq[%d]=0x%x\r\n", idx,
					p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START]);
				if (idx == SRCCAP_HDMI_IRQ_PM_SQH_ALL_WK) {
					HDMIRX_MSG_INFO("skip SRCCAP_HDMI_IRQ_PM_SQH_ALL_WK\r\n");
					continue;
				}
				ret = devm_request_irq(
					&pdev->dev,
					p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START],
					mtk_srccap_hdmirx_isr_entry, IRQF_ONESHOT, pdev->name,
					srccap_dev);
				if (ret) {
					HDMIRX_MSG_ERROR(
						"Failed to request irq[%d]=0x%x\r\n", idx,
						p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START]);
					HDMIRX_MSG_ERROR("mdbgerr_%s:  %d\r\n", __func__, -EINVAL);
					ret_val = -EINVAL;
				}
			}
		}
	}

	return ret_val;
}

static int _hdmirx_irq_suspend(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	int idx = 0;

	struct srccap_hdmirx_interrupt *p_hdmi_int_s =
		&srccap_dev->hdmirx_info.hdmi_int_s;

	for (idx = SRCCAP_HDMI_IRQ_START; idx < SRCCAP_HDMI_IRQ_END; idx++) {
		if (p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START] < 0) {
			HDMIRX_MSG_ERROR("Wrong irq, failed = %d\r\n", idx);
			ret = -EINVAL;
		} else {
			disable_irq(p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START]);
		}
	}

	return ret;
}

static int _hdmirx_irq_resume(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	int idx = 0;

	struct srccap_hdmirx_interrupt *p_hdmi_int_s =
		&srccap_dev->hdmirx_info.hdmi_int_s;

	for (idx = SRCCAP_HDMI_IRQ_START; idx < SRCCAP_HDMI_IRQ_END; idx++) {
		if (p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START] < 0) {
			HDMIRX_MSG_ERROR("Wrong irq, hdmi index = %d\r\n", idx);
			ret = -EINVAL;
		} else {
			enable_irq(p_hdmi_int_s->int_id[idx - SRCCAP_HDMI_IRQ_START]);
		}
	}

	return ret;
}

bool mtk_srccap_hdmirx_isHDMIMode(enum v4l2_srccap_input_source src)
{
	E_MUX_INPUTPORT ePort = mtk_hdmirx_to_muxinputport(src);

	HDMIRX_MSG_DBG("mdbgin_%s:  start \r\n", __func__);
	HDMIRX_MSG_INFO("src[%d] to ePort[%d]\r\n", src, ePort);
	return MDrv_HDMI_IsHDMI_Mode(ePort);
}

bool mtk_srccap_hdmirx_get_pwr_saving_onoff_hwdef(
	enum v4l2_srccap_input_source src)
{
	return drv_hwreg_srccap_hdmirx_query_module_hw_param(
			E_HDMI_IMMESWITCH_E,
			E_HDMI_PARM_HW_DEF,
		mtk_hdmirx_to_muxinputport(src)) ? true : false;
}

bool mtk_srccap_hdmirx_get_pwr_saving_onoff(
	enum v4l2_srccap_input_source src)
{
	return drv_hwreg_srccap_hdmirx_query_module_hw_param(
			E_HDMI_IMMESWITCH_E,
			E_HDMI_PARM_SW_PROG,
		mtk_hdmirx_to_muxinputport(src)) ? true : false;
}

bool mtk_srccap_hdmirx_set_pwr_saving_onoff(
	enum v4l2_srccap_input_source src,
	bool b_on)
{
	return drv_hwreg_srccap_hdmirx_set_module_hw_param_sw_prog(
		E_HDMI_IMMESWITCH_E,
		mtk_hdmirx_to_muxinputport(src),
		b_on ? 1 : 0);
}

bool mtk_srccap_hdmirx_get_HDMIStatus(
	enum v4l2_srccap_input_source src)
{
	bool bret = FALSE;
	E_MUX_INPUTPORT ePort =
			mtk_hdmirx_to_muxinputport(src);
	MS_U8 u8HDMIInfoSource = drv_hwreg_srccap_hdmirx_mux_inport_2_src(ePort);
	MS_U8 u8ErrorStatus = 0;

	u8ErrorStatus = drv_hwreg_srccap_hdmirx_pkt_get_errstatus(u8HDMIInfoSource,
		u8ErrorStatus, TRUE, NULL);

	if (drv_hwreg_srccap_hdmirx_phy_get_clklosedet(ePort)
		|| (!drv_hwreg_srccap_hdmirx_mac_get_destable(ePort))
		|| u8ErrorStatus){
		bret = FALSE;
	} else {
		bret = TRUE;
	}

	// clear errstatus
	u8ErrorStatus = drv_hwreg_srccap_hdmirx_pkt_get_errstatus(u8HDMIInfoSource,
		u8ErrorStatus, FALSE, NULL);

	return bret;
}

bool mtk_srccap_hdmirx_get_freesync_info(
	enum v4l2_srccap_input_source src,
	struct v4l2_ext_hdmi_get_freesync_info *p_info)
{
	int pkt_count = 0;
	bool bval = false;
	struct hdmi_packet_info *gnl_ptr =  NULL;

	gnl_ptr = kzalloc(
	sizeof(struct hdmi_packet_info)*META_SRCCAP_HDMIRX_PKT_COUNT_MAX_SIZE,
	GFP_KERNEL);
	if (gnl_ptr == NULL)
		return -ENOMEM;

	pkt_count = mtk_srccap_hdmirx_pkt_get_gnl(src, V4L2_EXT_HDMI_PKT_SPD,
			gnl_ptr, META_SRCCAP_HDMIRX_PKT_MAX_SIZE);

	if (pkt_count > 0) {
		int i;

		p_info->u8_fsinfo = 0;

		for (i = 0; i < pkt_count; i++) {
			if ((gnl_ptr[i].pb[1] == 0x1A) && (gnl_ptr[i].pb[2] == 0x00)
			&& (gnl_ptr[i].pb[3] == 0x00)) {
				if (gnl_ptr[i].pb[6] & BIT(0)) {
					p_info->u8_fsinfo =
					p_info->u8_fsinfo | V4L2_SRCCAP_FREESYNC_SUPPORTED;
				}
				if (gnl_ptr[i].pb[6] & BIT(1)) {
					p_info->u8_fsinfo =
					p_info->u8_fsinfo | V4L2_SRCCAP_FREESYNC_ENABLED;
				}
				if (gnl_ptr[i].pb[6] & BIT(2)) {
					p_info->u8_fsinfo =
					p_info->u8_fsinfo | V4L2_SRCCAP_FREESYNC_ACTIVE;
				}

				if ((gnl_ptr[i].pb[6] & BIT(0)) && (gnl_ptr[i].pb[6] & BIT(1)))
					bval = true;
			}
		}
	}
	kfree(gnl_ptr);

	return bval;
}

