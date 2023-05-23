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
#include <linux/types.h>
#include <linux/of_address.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/of_platform.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/fcntl.h>
#include <linux/mman.h>
#include <linux/string.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include <linux/mtk-v4l2-srccap.h>
#include "mtk_srccap.h"
#include "mtk_srccap_vbi.h"
#include "mtk_drv_vbi.h"
#include "show_param_vbi.h"
#include "vbi-ex.h"
#include "mtk-reserved-memory.h"

static __u32 CCInfoSelector;
static __u32 TTXInfoSelector;
static unsigned long pVBIphyAddress;
static unsigned long pVBICCvirAddress;
static unsigned long pVBITTXvirAddress;
static MS_U32 vbiBufSize;
static MS_U32 vbiBufHeapId;

#define MTK_ADDR_CAST(t)
#define MTK_SRCCAP_VBI_TTX_BUFFER_LENGTH    (360*48)
#define VBI_TTX_BUFFER_INIT    0x20
#define VPS_PACKET_LEN  15
static MS_U8 _u8VPSData[VPS_PACKET_LEN];

/* ============================================================================================== */
/* --------------------------------------- v4l2_ctrl_ops ---------------------------------------- */
/* ============================================================================================== */
static VBI_VIDEO_STANDARD _srccap_vbi_GetVideoType(v4l2_std_id AVDVideoStandardType)
{
	VBI_VIDEO_STANDARD eTvSystemTypeVBI = VBI_VIDEO_SECAM;

	switch (AVDVideoStandardType) {
	case V4L2_STD_PAL:
		eTvSystemTypeVBI = VBI_VIDEO_PAL;
		break;
	case V4L2_STD_NTSC:
		eTvSystemTypeVBI = VBI_VIDEO_NTSC;
		break;
	case V4L2_STD_NTSC_443:
	case V4L2_STD_PAL_60:
		eTvSystemTypeVBI = VBI_VIDEO_NTSC443_PAL60;
		break;
	case V4L2_STD_PAL_M:
		eTvSystemTypeVBI = VBI_VIDEO_PAL_M;
		break;
	case V4L2_STD_PAL_N:
		eTvSystemTypeVBI = VBI_VIDEO_PAL_NC;
		break;
	case V4L2_STD_SECAM:
		eTvSystemTypeVBI = VBI_VIDEO_SECAM;
		break;
	default:
		eTvSystemTypeVBI = VBI_VIDEO_NTSC;
		break;
	}

	return eTvSystemTypeVBI;
}


static int _vbi_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev =
	    container_of(ctrl->handler, struct mtk_srccap_dev, srccap_ctrl_handler);
	int ret = V4L2_EXT_VBI_FAIL;
	bool bWssReady, bIsOverflow, bVpsReady;
	phys_addr_t phyPacketPointer;
	v4L2_ext_vbi_get_ttxdata TTxPacket;
	__u16 WssPacketCnt;
	__u16 TtxPacketCnt;
	__u16 WssPacketData;
	__u32 CCInfo, TTXInfo;
	__u8 u8Success;
	__u8 *VPSBuffer = NULL;
	__u8 VPSdata1 = 0;
	__u8 VPSdata2 = 0;
	__u8 VPSdata3 = 0;
	__u8 VPSdata4 = 0;
	v4l2_ext_vbi_vps_data stVPSData;
	v4l2_ext_vbi_vps_alldata stVPSAllData;

	switch (ctrl->id) {
	case V4L2_CID_VBI_CC_INFO:
		memset(&CCInfo, 0, sizeof(__u32));
		ret = mtk_vbi_cc_get_info(CCInfoSelector, &CCInfo);
		memcpy((void *)ctrl->p_new.p_u8, &CCInfo, sizeof(__u32));
		break;
	case V4L2_CID_VBI_TTX_INFO:
		memset(&TTXInfo, 0, sizeof(__u32));
		ret = mtk_vbi_ttx_get_info(TTXInfoSelector, &TTXInfo);
		memcpy((void *)ctrl->p_new.p_u8, &TTXInfo, sizeof(__u32));
		break;
	case V4L2_CID_VBI_WSS_READY:
		memset(&bWssReady, 0, sizeof(bool));
		ret = mtk_vbi_is_wss_ready(&bWssReady);
		memcpy((void *)ctrl->p_new.p_u8, &bWssReady, sizeof(bool));
		break;
	case V4L2_CID_VBI_TTX_OVERFLOW:
		memset(&bIsOverflow, 0, sizeof(bool));
		ret = mtk_vbi_ttx_packet_buffer_is_overflow(&bIsOverflow);
		memcpy((void *)ctrl->p_new.p_u8, &bIsOverflow, sizeof(bool));
		break;
	case V4L2_CID_VBI_TTX_DATA:
		memset(&TTxPacket, 0, sizeof(v4L2_ext_vbi_get_ttxdata));
		memset(&phyPacketPointer, 0, sizeof(__u64));
		memset(&u8Success, 0, sizeof(__u8));
		ret = mtk_vbi_ttx_get_packet((MS_PHY *)&phyPacketPointer, &u8Success);
		TTxPacket.phyPacketPointer = phyPacketPointer;
		TTxPacket.u8Success = u8Success;
		memcpy((void *)ctrl->p_new.p_u8, &TTxPacket, sizeof(v4L2_ext_vbi_get_ttxdata));

		break;
	case V4L2_CID_VBI_TTX_PKTCOUNT:
		memset(&TtxPacketCnt, 0, sizeof(__u16));
		ret = mtk_vbi_ttx_get_packet_count(&TtxPacketCnt);
		memcpy((void *)ctrl->p_new.p_u8, &TtxPacketCnt, sizeof(__u16));
		break;
	case V4L2_CID_VBI_WSS_PKTCOUNT:
		memset(&WssPacketCnt, 0, sizeof(__u16));
		ret = mtk_vbi_wss_get_packet_count(&WssPacketCnt);
		memcpy((void *)ctrl->p_new.p_u8, &WssPacketCnt, sizeof(__u16));
		break;
	case V4L2_CID_VBI_WSS_DATA:
		memset(&WssPacketData, 0, sizeof(__u16));
		ret = mtk_vbi_get_wss_data(&WssPacketData);
		memcpy((void *)ctrl->p_new.p_u8, &WssPacketData, sizeof(__u16));
		break;
	case V4L2_CID_VBI_VPS_READY:
		memset(&bVpsReady, 0, sizeof(bool));
		ret = mtk_vbi_is_vps_ready(&bVpsReady);
		memcpy((void *)ctrl->p_new.p_u8, &bVpsReady, sizeof(bool));
		break;
	case V4L2_CID_VBI_VPS_DATA:
		memset(&stVPSData, 0, sizeof(v4l2_ext_vbi_vps_data));
		ret = mtk_vbi_get_vps_data(&stVPSData.u8lowerWord, &stVPSData.u8higherWord);
		memcpy((void *)ctrl->p_new.p_u8, &stVPSData, sizeof(v4l2_ext_vbi_vps_data));
		break;
	case V4L2_CID_VBI_VPS_ALL_DATA:
		memset(&stVPSAllData, 0, sizeof(v4l2_ext_vbi_vps_alldata));
		ret = mtk_vbi_get_raw_vps_data(&VPSdata1, &VPSdata2, &VPSdata3, &VPSdata4);
		memset(_u8VPSData, 0, VPS_PACKET_LEN);
		_u8VPSData[0x0D - 1] = VPSdata1;
		_u8VPSData[0x0E - 1] = VPSdata2;
		_u8VPSData[0x05 - 1] = VPSdata3;
		_u8VPSData[0x0B - 1] = VPSdata4;
		stVPSAllData.dataLen = VPS_PACKET_LEN;
		memcpy(stVPSAllData.au8Data, _u8VPSData, VPS_PACKET_LEN);
		memcpy((void *)ctrl->p_new.p_u8, &stVPSAllData, sizeof(v4l2_ext_vbi_vps_alldata));
		break;
	case V4L2_CID_VBI_GET_KERNEL_VA:
		SRCCAP_VBI_MSG_ERROR("[V4L2][VBI] VA = 0x%lx\n", pVBICCvirAddress);
		if (pVBICCvirAddress == 0) {
			ret = V4L2_EXT_VBI_FAIL;
		} else {
			memcpy((void *)ctrl->p_new.p_u8, &pVBICCvirAddress, sizeof(unsigned long));
			ret = V4L2_EXT_VBI_OK;
		}
		break;
	default:
		SRCCAP_VBI_MSG_ERROR("[V4L2] Unknown get control id = 0x%x\n", ctrl->id);
		ret = V4L2_EXT_VBI_FAIL;
		break;
	}

	return ret;
}

static int _vbi_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev =
	    container_of(ctrl->handler, struct mtk_srccap_dev, srccap_ctrl_handler);
	int ret = V4L2_EXT_VBI_FAIL;
	__u8 *pu8table, cnt;
	bool bEnable;
	v4l2_srccap_vbi_init_type stInitType;
	v4L2_ext_vbi_init_ccslicer stCCInitParam;
	v4L2_ext_vbi_init_ttxslicer stTTXInitParam;
	v4l2_std_id VideoStandard;
	VBI_VIDEO_STANDARD eTvSystemTypeVBI;

	switch (ctrl->id) {
	case V4L2_CID_VBI_INIT:
		memset(&stInitType, 0, sizeof(v4l2_srccap_vbi_init_type));
		memcpy(&stInitType, (void *)ctrl->p_new.p_u8, sizeof(v4l2_srccap_vbi_init_type));
		ret = mtk_vbi_init((VBI_INIT_TYPE) stInitType);
		SRCCAP_VBI_MSG_INFO("[V4L2] V4L2_CID_VBI_INIT Type=%d\n", stInitType);
		break;
	case V4L2_CID_VBI_INIT_CC_SLICER:
		memset(&stCCInitParam, 0, sizeof(v4L2_ext_vbi_init_ccslicer));
		memcpy(&stCCInitParam, (void *)ctrl->p_new.p_u8,
		       sizeof(v4L2_ext_vbi_init_ccslicer));
		stCCInitParam.bufferAddr = pVBIphyAddress;
		ret =
		    mtk_vbi_cc_init_slicer(stCCInitParam.RiuAddr, stCCInitParam.bufferAddr,
					   stCCInitParam.packetCount);
		break;
	case V4L2_CID_VBI_CC_RATE:
		pu8table = ctrl->p_new.p_u8;
		ret = mtk_vbi_cc_set_data_rate(pu8table);
		break;
	case V4L2_CID_VBI_CC_FRAME_COUNT:
		memset(&cnt, 0, sizeof(__u8));
		memcpy(&cnt, (void *)ctrl->p_new.p_u8, sizeof(__u8));
		ret = mtk_vbi_cc_set_frame_cnt(cnt);
		break;
	case V4L2_CID_VBI_CC_ENABLE_SLICER:
		memset(&bEnable, 0, sizeof(bool));
		memcpy(&bEnable, (void *)ctrl->p_new.p_u8, sizeof(bool));
		ret = mtk_vbi_cc_enable_slicer(bEnable);
		break;
	case V4L2_CID_VBI_INIT_TTX_SLICER:
		SRCCAP_VBI_MSG_INFO("[V4L2] V4L2_CID_VBI_INIT_TTX_SLICER\n");
		memset(&stTTXInitParam, 0, sizeof(v4L2_ext_vbi_init_ttxslicer));
		memcpy(&stTTXInitParam, (void *)ctrl->p_new.p_u8,
			sizeof(v4L2_ext_vbi_init_ttxslicer));
		stTTXInitParam.bufferAddr = pVBIphyAddress;
		ret =
		    mtk_vbi_ttx_initialize_slicer(stTTXInitParam.bufferAddr,
						  stTTXInitParam.packetCount);
		memset(pVBICCvirAddress, VBI_TTX_BUFFER_INIT, MTK_SRCCAP_VBI_TTX_BUFFER_LENGTH);
		SRCCAP_VBI_MSG_INFO
		    ("[V4L2] V4L2_CID_VBI_INIT_TTX_SLICER bufAddr=0x%lx\n",
		     stTTXInitParam.bufferAddr);
		break;
	case V4L2_CID_VBI_ENABLE_TTX_SLICER:
		memset(&bEnable, 0, sizeof(bool));
		memcpy(&bEnable, (void *)ctrl->p_new.p_u8, sizeof(bool));
		ret = mtk_vbi_ttx_enable_slicer(bEnable);
		break;
	case V4L2_CID_VBI_TTX_RESET_BUF:
		ret = mtk_vbi_ring_buffer_reset();
		memset(pVBICCvirAddress, VBI_TTX_BUFFER_INIT, MTK_SRCCAP_VBI_TTX_BUFFER_LENGTH);
		break;
	case V4L2_CID_VBI_CC_INFO:
		memset(&CCInfoSelector, 0, sizeof(__u32));
		memcpy(&CCInfoSelector, (void *)ctrl->p_new.p_u8, sizeof(__u32));
		ret = V4L2_EXT_VBI_OK;
		break;
	case V4L2_CID_VBI_TTX_INFO:
		memset(&TTXInfoSelector, 0, sizeof(__u32));
		memcpy(&TTXInfoSelector, (void *)ctrl->p_new.p_u8, sizeof(__u32));
		ret = V4L2_EXT_VBI_OK;
		break;
	case V4L2_CID_VBI_VIDEO_STANDARD:
		memset(&VideoStandard, 0, sizeof(v4l2_std_id));
		memcpy(&VideoStandard, (void *)ctrl->p_new.p_u8, sizeof(v4l2_std_id));
		eTvSystemTypeVBI = _srccap_vbi_GetVideoType(VideoStandard);
		ret = mtk_vbi_set_video_standard(eTvSystemTypeVBI);
		break;
	default:
		SRCCAP_VBI_MSG_ERROR("[V4L2] Unknown set control id = 0x%x\n",
			    ctrl->id);
		ret = V4L2_EXT_VBI_FAIL;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops vbi_ctrl_ops = {
	.g_volatile_ctrl = _vbi_g_ctrl,
	.s_ctrl = _vbi_s_ctrl,
};

static const struct v4l2_ctrl_config vbi_ctrl[] = {
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_INIT,
	 .name = "VBI Init",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(v4l2_srccap_vbi_init_type)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_EXIT,
	 .name = "VBI exit",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_INIT_TTX_SLICER,
	 .name = "VBI_INIT_TTX_SLICER",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(v4L2_ext_vbi_init_ttxslicer)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_ENABLE_TTX_SLICER,
	 .name = "VBI_ENABLE_TTX_SLICER",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(__u8)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_TTX_INFO,
	 .name = "VBI_TTX_INFO",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(__u32)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_INIT_CC_SLICER,
	 .name = "VBI_INIT_CC_SLICER",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(v4L2_ext_vbi_init_ccslicer)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_WSS_READY,
	 .name = "VBI_WSS_READY",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(bool)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_CC_INFO,
	 .name = "VBI_CC_GET_INFO",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(__u32)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_CC_RATE,
	 .name = "VBI_CC_RATE",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(__u8) * 8}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_CC_FRAME_COUNT,
	 .name = "VBI_CC_FRAME_COUNT",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(__u8)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_CC_ENABLE_SLICER,
	 .name = "VBI_CC_ENABLE_SLICER",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(bool)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_TTX_OVERFLOW,
	 .name = "VBI_TTX_OVERFLOW",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(bool)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_TTX_RESET_BUF,
	 .name = "VBI_RESET_BUF",
	 .type = V4L2_CTRL_TYPE_BUTTON,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_TTX_DATA,
	 .name = "VBI_TTX_DATA",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(v4L2_ext_vbi_get_ttxdata)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_VPS_READY,
	 .name = "VBI_VPS_READY",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(bool)},
	 },
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_VPS_DATA,
	 .name = "VBI_VPS_DATA",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(v4l2_ext_vbi_vps_data)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_VPS_ALL_DATA,
	 .name = "VBI_VPS_ALL_DATA",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .step = 1,  .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(v4l2_ext_vbi_vps_alldata)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_TTX_PKTCOUNT,
	 .name = "VBI_TTX_PKTCOUNT",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(__u16)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_WSS_PKTCOUNT,
	 .name = "VBI_WSS_PKTCOUNT",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(__u16)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_WSS_DATA,
	 .name = "VBI_WSS_DATA",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(__u16)}
	 ,
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_VIDEO_STANDARD,
	 .name = "VBI_VIDEO_STANDARD",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(v4l2_std_id)},
	}
	,
	{
	 .ops = &vbi_ctrl_ops,
	 .id = V4L2_CID_VBI_GET_KERNEL_VA,
	 .name = "VBI_GET_KERNEL_VA",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE,
	 .dims = {sizeof(MS_PHY)}
	,
	}
	,
};

/* subdev operations */
static const struct v4l2_subdev_ops vbi_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops vbi_sd_internal_ops = {
};

int mtk_srccap_register_vbi_subdev(struct v4l2_device *v4l2_dev, struct v4l2_subdev *subdev_vbi,
				   struct v4l2_ctrl_handler *vbi_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(vbi_ctrl) / sizeof(struct v4l2_ctrl_config);

	SRCCAP_MSG_ERROR("debug VBI:vbi ctrl_num = %d\n", ctrl_num);
	v4l2_ctrl_handler_init(vbi_ctrl_handler, ctrl_num);
	if (vbi_ctrl_handler->error) {
		SRCCAP_MSG_ERROR("failed to init vbi ctrl handler\n");
		goto exit;
	}
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(vbi_ctrl_handler, &vbi_ctrl[ctrl_count], NULL);
		if (vbi_ctrl_handler->error) {
			SRCCAP_MSG_ERROR(
				 "debug VBI: failed at ctrl_count = %d in v4l2_ctrl_new_custom\n",
				 ctrl_count);
			vbi_ctrl_handler->error = 0;
		}
	}

	ret = vbi_ctrl_handler->error;
	if (ret) {
		SRCCAP_MSG_ERROR("failed to create vbi ctrl handler\n");
		goto exit;
	}
	subdev_vbi->ctrl_handler = vbi_ctrl_handler;

	v4l2_subdev_init(subdev_vbi, &vbi_sd_ops);
	subdev_vbi->internal_ops = &vbi_sd_internal_ops;
	strlcpy(subdev_vbi->name, "mtk-vbi", sizeof(subdev_vbi->name));

	ret = v4l2_device_register_subdev(v4l2_dev, subdev_vbi);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to register vbi subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(vbi_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_vbi_subdev(struct v4l2_subdev *subdev_vbi)
{
	v4l2_ctrl_handler_free(subdev_vbi->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_vbi);
}

void mtk_vbi_SetBufferAddr(void *Kernelvaddr, struct srccap_vbi_info vbi_info)
{
	SRCCAP_VBI_MSG_INFO("[STI][VBI] SetBufferAddr IN, vaddr = 0x%lx\n", Kernelvaddr);

	pVBICCvirAddress = Kernelvaddr;

	pVBIphyAddress = vbi_info.vbiBufAddr;
	vbiBufSize = vbi_info.vbiBufSize;
	vbiBufHeapId = vbi_info.vbiBufHeapId;

	SRCCAP_VBI_MSG_INFO("[VBI] vbiBufAddr = 0x%lX\n", pVBIphyAddress);
	SRCCAP_VBI_MSG_INFO("[VBI] vbiBufSize = 0x%lX\n", vbiBufSize);
	SRCCAP_VBI_MSG_INFO("[VBI] vbiBufHeapId = 0x%lX\n", vbiBufHeapId);
}

