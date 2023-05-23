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

#include <linux/videodev2.h>
#include <uapi/linux/mtk-v4l2-pq.h>
#include "mtk_pq.h"
#include "mtk_pq_b2r.h"
#include "mtk_pq_display_manager.h"

#include "hwreg_pq_display_b2r.h"

#include "apiXC.h"
#include "drvPQ.h"

/* local definition */
#define B2R_MIN_FPS     1000
#define B2R_MAX_H_ALIGN     32
#define B2R_H_BLK	300
#define B2R_V_BLK	45

#define VBLK0_DEF_STR	0x1

#define ALIGN_UPTO_32(x)  ((((x) + 31) >> 5) << 5)

/* resource name assign */
static const struct resources_name b2r_res_name = {
	.regulator  = { NULL },
	.clock      = { NULL },
	.reg        = { NULL },
	.interrupt  = { "b2r" }
};

int _mtk_pq_b2r_chk_ipt_timing(struct v4l2_timing *input_timing_info)
{
	if ((input_timing_info->v_freq < B2R_MIN_FPS)
		|| (input_timing_info->de_width < B2R_MAX_H_ALIGN)
		|| (input_timing_info->de_height == 0)) {
		return -EINVAL;
	}

	return 0;
}

int _mtk_pq_b2r_calc_clk(struct v4l2_timing *input_timing_info, __u64 *clock_rate)
{
	__u64 clock_rate_est;
	__u64 htt_est;
	__u64 vtt_est;

	htt_est = ALIGN_UPTO_32(input_timing_info->de_width) + B2R_H_BLK;
	vtt_est = ALIGN_UPTO_32(input_timing_info->de_height) + B2R_V_BLK;
	clock_rate_est = (htt_est * vtt_est * input_timing_info->v_freq) / 1000;

	if (clock_rate_est > B2R_CLK_690MHZ) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
			"%s: input timing out of spec\n", __func__);
		return -EINVAL;
	}

	if (clock_rate_est < B2R_CLK_160MHZ)
		*clock_rate = B2R_CLK_160MHZ;
	else if (clock_rate_est < B2R_CLK_320MHZ)
		*clock_rate = B2R_CLK_320MHZ;
	else
		*clock_rate = B2R_CLK_690MHZ;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "%s: clock = %d\n",
		__func__, *clock_rate);

	return 0;
}

int _mtk_pq_b2r_gen_timing(struct v4l2_timing *input_timing_info,
	struct v4l2_b2r_info *b2r_info, __u64 clock_rate)
{
	__u64 htt_est;
	__u64 vtt_est;
	__u16 VBlank0, Vblank1;

	vtt_est = input_timing_info->de_height + B2R_V_BLK;
	htt_est = clock_rate * 1000 / input_timing_info->v_freq / vtt_est;

	input_timing_info->h_total = htt_est;
	input_timing_info->v_total = vtt_est;
	/* unit: 100Hz */
	input_timing_info->h_freq = input_timing_info->v_total *
		input_timing_info->v_freq / 100000;

	memcpy(&b2r_info->timing_in, input_timing_info,
		sizeof(struct v4l2_timing));
	b2r_info->current_clk = clock_rate;
	b2r_info->timing_out.V_TotalCount = input_timing_info->v_total;
	b2r_info->timing_out.H_TotalCount = input_timing_info->h_total;
	VBlank0 = b2r_info->timing_out.V_TotalCount
		- input_timing_info->de_height;
	b2r_info->timing_out.VBlank0_Start = VBLK0_DEF_STR;
	b2r_info->timing_out.VBlank0_End = VBlank0 - b2r_info->timing_out.VBlank0_Start;
	b2r_info->timing_out.TopField_VS = b2r_info->timing_out.VBlank0_Start
		+ (VBlank0 >> 1);
	b2r_info->timing_out.TopField_Start = b2r_info->timing_out.TopField_VS;
	b2r_info->timing_out.HActive_Start = b2r_info->timing_out.H_TotalCount
		- input_timing_info->de_width;
	b2r_info->timing_out.HImg_Start = b2r_info->timing_out.HActive_Start;
	b2r_info->timing_out.VImg_Start0 = b2r_info->timing_out.VBlank0_End;

	if (input_timing_info->interlance == V4L2_FIELD_NONE) {
		b2r_info->timing_out.VBlank1_Start = b2r_info->timing_out.VBlank0_Start;
		b2r_info->timing_out.VBlank1_End = b2r_info->timing_out.VBlank0_End;
		b2r_info->timing_out.BottomField_VS = b2r_info->timing_out.TopField_VS;
		b2r_info->timing_out.BottomField_Start = b2r_info->timing_out.TopField_Start;
		b2r_info->timing_out.VImg_Start1 = b2r_info->timing_out.VImg_Start0;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"%s: v_total = 0x%x, h_total = 0x%x\n",
		__func__,
		b2r_info->timing_in.v_total,
		b2r_info->timing_in.h_total);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"interlance = 0x%x, v_freq = %d, h_freq = %d\n",
		b2r_info->timing_in.interlance,
		b2r_info->timing_in.v_freq,
		b2r_info->timing_in.h_freq);

	return 0;
}

int _mtk_pq_b2r_config_reg_timing(struct v4l2_timing *input_timing_info,
	struct v4l2_b2r_info *b2r_info, __u64 clock_rate)
{
	__u64 htt_est;
	__u64 vtt_est;

	vtt_est = input_timing_info->de_height + B2R_V_BLK;
	htt_est = clock_rate * 1000 / input_timing_info->v_freq / vtt_est;

	input_timing_info->h_total = htt_est;
	input_timing_info->v_total = vtt_est;
	/* unit: 100Hz */
	input_timing_info->h_freq = input_timing_info->v_total *
		input_timing_info->v_freq / 100000;

	memcpy(&b2r_info->timing_in, input_timing_info,
		sizeof(struct v4l2_timing));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"%s: v_total = 0x%x, h_total = 0x%x\n",
		__func__,
		b2r_info->timing_in.v_total,
		b2r_info->timing_in.h_total);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"interlance = 0x%x, v_freq = %d, h_freq = %d\n",
		b2r_info->timing_in.interlance,
		b2r_info->timing_in.v_freq,
		b2r_info->timing_in.h_freq);

	return 0;
}


int _mtk_pq_b2r_set_timing(struct v4l2_timing *input_timing_info, struct v4l2_b2r_info *b2r_info)
{
	__u64 clock_rate = 0;
	struct b2r_timing_info reg_timing;
	struct reg_info reg;

	/* check input parameters */
	if (_mtk_pq_b2r_chk_ipt_timing(input_timing_info) < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
			"%s: _mtk_pq_b2r_check_input_timing failed!\n", __func__);
		return -EPERM;
	}
	/* calculate clock level */
	if (_mtk_pq_b2r_calc_clk(input_timing_info, &clock_rate) < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
			"%s: _mtk_pq_b2r_calc_clk failed!\n", __func__);
		return -EPERM;
	}

	/* generate timing */
	if (_mtk_pq_b2r_gen_timing(input_timing_info, b2r_info, clock_rate) < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
			"%s: _mtk_pq_b2r_calc_clk failed!\n", __func__);
		return -EPERM;
	}

	/* assign to hwreg */
	memcpy(&reg_timing, &b2r_info->timing_out,
		sizeof(struct b2r_timing_info));

	drv_hwreg_pq_display_b2r_set_timing(TRUE, &reg, reg_timing);

	return 0;
}

int mtk_pq_b2r_init(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	struct reg_info reg;
	uint64_t regbase = 0;

	drv_hwreg_pq_display_b2r_init(TRUE, &reg, regbase);

	return ret;
}

int mtk_pq_b2r_exit(void)
{
	int ret = 0;

	return ret;
}

int mtk_pq_b2r_set_pix_format_in_mplane(struct v4l2_format *format,
		struct v4l2_b2r_info *b2r_info)
{
	int ret = 0;

	if ((!format) || (!b2r_info)) {
		ret = -EINVAL;
		goto ERR;
	}
	memcpy(&b2r_info->pix_fmt_in, &format->fmt.pix_mp,
		sizeof(struct v4l2_pix_format_mplane));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "width = %d, height = %d\n",
		b2r_info->pix_fmt_in.width,
		b2r_info->pix_fmt_in.height);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"bytesperline = %d, field = %d, pixelformat = %d\n",
		b2r_info->pix_fmt_in.plane_fmt[0].bytesperline,
		b2r_info->pix_fmt_in.field,
		format->fmt.pix_mp.pixelformat);

ERR:
	return ret;

}

int mtk_pq_b2r_set_flip(__u8 win, __s32 enable, struct v4l2_b2r_info *b2r_info,
		enum v4l2_b2r_flip flip_type)
{
	int ret = 0;

	if (!b2r_info) {
		ret = -EINVAL;
		goto ERR;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"%s: type = %d, enable =%d\n",
		__func__, flip_type, enable);

	switch (flip_type) {
	case B2R_HFLIP:
		b2r_info->b2r_stat.hflip = enable;
		break;
	case B2R_VFLIP:
		b2r_info->b2r_stat.vflip = enable;
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
				"flip type not supported by b2r!\n");
		break;
	}


ERR:
	return ret;

}

int mtk_pq_b2r_get_flip(__u8 win, bool *enable, struct v4l2_b2r_info *b2r_info,
		enum v4l2_b2r_flip flip_type)
{
	int ret = 0;

	if (!b2r_info) {
		ret = -EINVAL;
		goto ERR;
	}

	switch (flip_type) {
	case B2R_HFLIP:
		*enable = (__s32)b2r_info->b2r_stat.hflip;
		break;
	case B2R_VFLIP:
		*enable = (__s32)b2r_info->b2r_stat.vflip;
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
			"flip type not supported by b2r!\n");
		break;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "%s: type = %d, enable =%d\n",
		__func__, flip_type, *enable);

ERR:
	return ret;

}

int mtk_b2r_set_ext_timing(__u8 win, void *ctrl, struct v4l2_b2r_info *b2r_info)
{
	struct v4l2_timing input_timing_info;
	int ret = 0;

	if ((!ctrl) || (!b2r_info)) {
		ret = -EINVAL;
		goto ERR;
	}

	memset(&input_timing_info, 0, sizeof(struct v4l2_timing));
	memcpy(&input_timing_info, ctrl, sizeof(struct v4l2_timing));
	memcpy(&b2r_info->timing_in, &input_timing_info,
		sizeof(struct v4l2_timing));

	mtk_display_set_outputfrmrate((__u32)win, input_timing_info.v_freq);

	if (_mtk_pq_b2r_set_timing(&input_timing_info, b2r_info) < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
			"%s: _mtk_pq_b2r_dump_timing failed!\n", __func__);
		ret = -EPERM;
		goto ERR;
	}

ERR:
	return ret;
}

int mtk_b2r_get_ext_timing(__u8 win, void *ctrl, struct v4l2_b2r_info *b2r_info)
{
	struct v4l2_timing input_timing_info;
	int ret = 0;

	if ((!ctrl) || (!b2r_info)) {
		ret = -EINVAL;
		goto ERR;
	}

	memset(&input_timing_info, 0, sizeof(struct v4l2_timing));
	memcpy(&input_timing_info, &b2r_info->timing_in,
			sizeof(struct v4l2_timing));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"%s: v_total = 0x%x, h_total = 0x%x\n",
		__func__,
		input_timing_info.v_total,
		input_timing_info.h_total);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"interlance = 0x%x, v_freq = %d, h_freq = %d\n",
		input_timing_info.interlance,
		input_timing_info.v_freq,
		input_timing_info.h_freq);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"de_width = 0x%x, de_height = 0x%x\n",
		input_timing_info.de_width,
		input_timing_info.de_height);

	memcpy(ctrl, &input_timing_info, sizeof(struct v4l2_timing));

ERR:
	return ret;
}

int mtk_pq_b2r_set_forcep(__u8 win, bool enable, struct v4l2_b2r_info *b2r_info)
{
	int ret = 0;
	bool bforcep = enable;
	//MVOP_Handle stMvopHd = { E_MVOP_MODULE_MAIN };

	if (!b2r_info) {
		ret = -EINVAL;
		goto ERR;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "%s: enable =%d\n", __func__, enable);
	b2r_info->b2r_stat.forcep = enable;

ERR:
	return ret;

}

int mtk_pq_b2r_set_delaytime(__u8 win, __u32 fps,
	struct v4l2_b2r_info *b2r_info)
{
	int ret = 0;

	if (!b2r_info) {
		ret = -EINVAL;
		goto ERR;
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "%s: fps =%d\n", __func__, fps);

	b2r_info->delaytime_fps = fps;

ERR:
	return ret;

}

int mtk_pq_b2r_get_delaytime(__u8 win, __u16 *b2r_delay,
		struct v4l2_b2r_info *b2r_info)
{
	int ret = 0;

	if (!b2r_info) {
		ret = -EINVAL;
		goto ERR;
	}

	if (b2r_info->delaytime_fps > 0)
		*b2r_delay = ((((1000000 * 10) /
		b2r_info->delaytime_fps) + 5) / 10);
	else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "%s: warning fps =%d\n",
			__func__, b2r_info->delaytime_fps);
		*b2r_delay = 0;
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "%s: delaytime =%d\n", __func__,
		*b2r_delay);

ERR:
	return ret;

}

int mtk_pq_b2r_set_pattern(__u8 win, enum v4l2_ext_pq_pattern pat_type,
	struct v4l2_b2r_pattern *b2r_pat_para, struct v4l2_b2r_info *b2r_info)
{
	int ret = 0;
	enum b2r_pattern reg_pat = B2R_PAT_None;
	struct reg_info reg;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "%s: pattern type =%x\n", __func__, pat_type);
	if (!b2r_info) {
		ret = -EINVAL;
		goto ERR;
	}

	switch (pat_type) {
	case V4L2_EXT_PQ_B2R_PAT_FRAMECOLOR:
		reg_pat = B2R_PAT_Colorbar;
		break;
	default:
		ret = -1;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "b2r not support pattern type\n");
		break;
	}

	drv_hwreg_pq_display_b2r_set_pattern(TRUE,
	&reg, reg_pat);

ERR:
	return ret;
}

static int _b2r_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev = container_of(ctrl->handler,
			struct mtk_pq_device, b2r_ctrl_handler);
	int ret;

	switch (ctrl->id) {
	case V4L2_CID_B2R_TIMING:
		ret = mtk_b2r_get_ext_timing(pq_dev->dev_indx,
			(void *)ctrl->p_new.p_u8, &pq_dev->b2r_info);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _b2r_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev = container_of(ctrl->handler,
			struct mtk_pq_device, b2r_ctrl_handler);
	int ret;

	switch (ctrl->id) {
	case V4L2_CID_B2R_TIMING:
	ret = mtk_b2r_set_ext_timing(pq_dev->dev_indx, (void *)ctrl->p_new.p_u8,
			&pq_dev->b2r_info);
	break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

int mtk_pq_b2r_subdev_init(struct device *dev,
	struct b2r_device *b2r)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;
	int ret = 0;
	const struct resources_name *b2r_res = &b2r_res_name;

	/* Interrupt */
	res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
		b2r_res->interrupt[0]);
	if (!res) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "b2r missing IRQ\n");
		return -EINVAL;
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R,
		"get b2r irq numver = %d\n", res->start);
	b2r->irq = res->start;

	ret = devm_request_irq(&pdev->dev, b2r->irq, mtk_display_mvop_isr,
		IRQF_TRIGGER_HIGH, pdev->name, dev);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_B2R, "failed to install IRQ\n");
		return -EINVAL;
	}

	disable_irq(b2r->irq);

	return ret;
}

static const struct v4l2_ctrl_ops b2r_ctrl_ops = {
	.g_volatile_ctrl = _b2r_g_ctrl,
	.s_ctrl = _b2r_s_ctrl,
};

static const struct v4l2_ctrl_config b2r_ctrl[] = {
	{
		.ops = &b2r_ctrl_ops,
		.id = V4L2_CID_B2R_TIMING,
		.name = "input timing ext info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_timing)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	}
};

/* subdev operations */
static const struct v4l2_subdev_ops b2r_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops b2r_sd_internal_ops = {
};

int mtk_pq_register_b2r_subdev(struct v4l2_device *pq_dev,
	struct v4l2_subdev *subdev_b2r,
	struct v4l2_ctrl_handler *b2r_ctrl_handler)
{
	int ret = 0;
	__u32 ctrl_count;
	__u32 ctrl_num = sizeof(b2r_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(b2r_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(b2r_ctrl_handler,
		&b2r_ctrl[ctrl_count], NULL);
	}
	ret = b2r_ctrl_handler->error;
	if (ret) {
		v4l2_err(pq_dev, "failed to create b2r ctrl handler\n");
		goto exit;
	}
	subdev_b2r->ctrl_handler = b2r_ctrl_handler;

	v4l2_subdev_init(subdev_b2r, &b2r_sd_ops);
	subdev_b2r->internal_ops = &b2r_sd_internal_ops;
	strlcpy(subdev_b2r->name, "mtk-b2r", sizeof(subdev_b2r->name));

	ret = v4l2_device_register_subdev(pq_dev, subdev_b2r);
	if (ret) {
		v4l2_err(pq_dev, "failed to register b2r subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(b2r_ctrl_handler);
	return ret;
}

void mtk_pq_unregister_b2r_subdev(struct v4l2_subdev *subdev_b2r)
{
	v4l2_ctrl_handler_free(subdev_b2r->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_b2r);
}

