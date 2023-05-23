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

#define STI_TEST

static unsigned int CCInfo = 2;

static int _vbi_g_ctrl(struct v4l2_ctrl *ctrl)
{
	VBI_ERR_MSG("[V4L2][%s][%d]ctrl->id = 0x%x\n", __func__, __LINE__, ctrl->id);
	struct mtk_srccap_dev *srccap_dev =
		container_of(ctrl->handler, struct mtk_srccap_dev, srccap_ctrl_handler);
	int ret = 0;
	__u32 u32selector;
	unsigned int u32CCResult;
	__u16 WSSdata = 0;
	unsigned int  WSSready = 0;
	__u32 u32AVDFlag;

	switch (ctrl->id) {

	case V4L2_CID_VBI_CC_INFO:
	VBI_ERR_MSG("[V4L2][%s][%d]V4L2_CID_VBI_CC_INFO	 CCInfo = %d\n",
		__func__, __LINE__, CCInfo);
	memcpy((void *)ctrl->p_new.p_u8, &CCInfo, sizeof(unsigned int));
	break;
	case V4L2_CID_VBI_WSS_READY:
	WSSready = MDrv_VBI_IsWSS_Ready();
	VBI_ERR_MSG("[V4L2][%s][%d]V4L2_CID_VBI_WSS_READY  WSSready= %d\n",
		__func__, __LINE__, WSSready);
#ifdef STI_TEST
	WSSready = 0xf;
#endif
		memcpy((void *)ctrl->p_new.p_u8, &WSSready, sizeof(unsigned int));
		break;

	default:
	ret = -1;
	break;
	}

	return ret;
}

static int _vbi_s_ctrl(struct v4l2_ctrl *ctrl)
{
	VBI_ERR_MSG("[V4L2][%s][%d]ctrl->id = 0x%x\n", __func__, __LINE__, ctrl->id);
	struct mtk_srccap_dev *srccap_dev =
		container_of(ctrl->handler, struct mtk_srccap_dev, srccap_ctrl_handler);
	int ret = 0;
	__u8 *pu8table, cnt, bEnable;
	__u32 u32selector;
	v4l2_srccap_vbi_init_type stParam;
	v4L2_ext_vbi_init_ccslicer stCCInitParam;

	switch (ctrl->id) {

	case V4L2_CID_VBI_INIT:
	memset(&stParam, 0, sizeof(v4l2_srccap_vbi_init_type));
	memcpy(&stParam, (void *)ctrl->p_new.p_u8, sizeof(v4l2_srccap_vbi_init_type));
	VBI_ERR_MSG("[V4L2][%s][%d]VBI_INIT	 stParam = %d\n", __func__, __LINE__, stParam);
	ret = MDrv_VBI_Init((VBI_INIT_TYPE)stParam);
	break;
	case V4L2_CID_VBI_INIT_CC_SLICER:
	VBI_ERR_MSG("[V4L2][%s][%d][eddie] INIT_CC_SLICER\n", __func__, __LINE__);
	memcpy(&stCCInitParam, (void *)ctrl->p_new.p_u8, sizeof(v4L2_ext_vbi_init_ccslicer));
	VBI_ERR_MSG("[V4L2][%s][%d] [eddie]INIT_CC_SLICER  stCCInitParam.RiuAddr = 0x%x ",
		__func__, __LINE__, stCCInitParam.bufferAddr);
	VBI_ERR_MSG("stCCInitParam.packetCount = %d\n", stCCInitParam.packetCount);
	MDrv_VBI_CC_InitSlicer(stCCInitParam.RiuAddr,
		stCCInitParam.bufferAddr, stCCInitParam.packetCount);
	break;

	case V4L2_CID_VBI_CC_INFO:
	VBI_ERR_MSG("[V4L2][%s][%d]V4L2_CID_VBI_CC_INFO\n", __func__, __LINE__);
	memset(&u32selector, 0, sizeof(__u32));
	memcpy(&u32selector, (void *)ctrl->p_new.p_u8, sizeof(__u32));
	CCInfo = MDrv_VBI_CC_GetInfo(u32selector);
	break;

	case V4L2_CID_VBI_CC_RATE:
		VBI_ERR_MSG("[V4L2][%s][%d]VBI_CC_RATE\n", __func__, __LINE__);
		pu8table = ctrl->p_new.p_u8;
	VBI_ERR_MSG("[V4L2][%s][%d]pu8table[0] = %d, pu8table[3] = 0x%x ",
		__func__, __LINE__, pu8table[0], pu8table[3]);
	VBI_ERR_MSG("pu8table[7] = 0x%x pu8table[8]=0x%x\n",
		pu8table[7], pu8table[8]);
	ret = MDrv_VBI_CC_SetDataRate(pu8table);
	break;

	case V4L2_CID_VBI_CC_FRAME_COUNT:
	memset(&cnt, 0, sizeof(__u8));
	memcpy(&cnt, (void *)ctrl->p_new.p_u8, sizeof(__u8));
	VBI_ERR_MSG("[V4L2][%s][%d]V4L2_CID_VBI_CC_FRAME_COUNT cnt = %d\n",
		__func__, __LINE__, cnt);
	MDrv_VBI_CC_SetFrameCnt(cnt);
	break;
	case V4L2_CID_VBI_CC_ENABLE_SLICER:
	memset(&bEnable, 0, sizeof(__u8));
	memcpy(&bEnable, (void *)ctrl->p_new.p_u8, sizeof(__u8));
	VBI_ERR_MSG("[V4L2][%s][%d]V4L2_CID_VBI_CC_ENABLE_SLICER bEnable = %d\n",
		__func__, __LINE__, bEnable);
	MDrv_VBI_CC_EnableSlicer(bEnable);
	break;

	default:
	VBI_ERR_MSG("[V4L2][%s][%d]Unknown control id = 0x%x\n",
		__func__, __LINE__, ctrl->id);
	ret = -1;
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
	.dims = {sizeof(v4l2_srccap_vbi_init_type)},
	},
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
	},
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
	.dims = {sizeof(v4L2_ext_vbi_init_ttxslicer)},
	},
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
	.dims = {sizeof(unsigned int)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_TTX_INFO,
	.name = "VBI_TTX_INFO",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.dims = {sizeof(__u32)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_INIT_CC_SLICER,
	.name = "VBI_INIT_CC_SLICER",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.dims = {sizeof(v4L2_ext_vbi_init_ccslicer)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_WSS_READY,
	.name = "VBI_WSS_READY",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.dims = {sizeof(unsigned int)},
	},

	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_CC_INFO,
	.name = "VBI_CC_INFO",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.dims = {sizeof(unsigned int)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_CC_INFO,
	.name = "VBI_CC_GET_INFO",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.dims = {sizeof(unsigned int)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_CC_RATE,
	.name = "VBI_CC_RATE",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.dims = {sizeof(__u8)*9},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_CC_FRAME_COUNT,
	.name = "VBI_CC_FRAME_COUNT",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.dims = {sizeof(__u8)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_CC_ENABLE_SLICER,
	.name = "VBI_CC_ENABLE_SLICER",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.dims = {sizeof(__u8)},
	},
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
	.dims = {sizeof(__u8)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_TTX_RESET_BUF,
	.name = "VBI_RESET_BUF",
	.type = V4L2_CTRL_TYPE_BUTTON,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
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
	.dims = {sizeof(unsigned long long)},
	},
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
	.dims = {sizeof(unsigned short)},
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
	.dims = {sizeof(bool)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_VPS_ALL_DATA,
	.name = "VBI_VPS_ALL_DATA",
	.type = V4L2_CTRL_TYPE_BUTTON,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_TTX_PKTCOUNT,
	.name = "VBI_TTX_PKTCOUNT",
	.type = V4L2_CTRL_TYPE_BUTTON,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_WSS_DATA,
	.name = "VBI_WSS_DATA",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.dims = {sizeof(v4l2_ext_vbi_vps_alldata)},
	},
	{
	.ops = &vbi_ctrl_ops,
	.id = V4L2_CID_VBI_VIDEO_STANDARD,
	.name = "VBI_VIDEO_STANDARD",
	.type = V4L2_CTRL_TYPE_U8,
	.def = 0,
	.min = 0,
	.max = 0xff,
	.step = 1,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.dims = {sizeof(unsigned char)},
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops vbi_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops vbi_sd_internal_ops = {
};

int mtk_srccap_register_vbi_subdev(struct v4l2_device *srccap_dev, struct v4l2_subdev *subdev_vbi,
	struct v4l2_ctrl_handler *vbi_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(vbi_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_err(srccap_dev, "debug VBI:vbi ctrl_num = %d\n", ctrl_num);
	v4l2_ctrl_handler_init(vbi_ctrl_handler, ctrl_num);
	if (vbi_ctrl_handler->error) {
		v4l2_err(srccap_dev, "failed to init vbi ctrl handler\n");
		goto exit;
	}
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(vbi_ctrl_handler, &vbi_ctrl[ctrl_count], NULL);
		if (vbi_ctrl_handler->error) {
			v4l2_err(srccap_dev, "debug VBI: failed at ctrl_count = %d\n", ctrl_count);
			vbi_ctrl_handler->error = 0;
		}
	}

	ret = vbi_ctrl_handler->error;
	if (ret) {
		v4l2_err(srccap_dev, "failed to create vbi ctrl handler\n");
		goto exit;
	}
	subdev_vbi->ctrl_handler = vbi_ctrl_handler;

	v4l2_subdev_init(subdev_vbi, &vbi_sd_ops);
	subdev_vbi->internal_ops = &vbi_sd_internal_ops;
	strlcpy(subdev_vbi->name, "mtk-vbi", sizeof(subdev_vbi->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_vbi);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register vbi subdev\n");
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


