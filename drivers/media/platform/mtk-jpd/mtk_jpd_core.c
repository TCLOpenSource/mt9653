// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>

#include <linux/mtk-v4l2-jpd.h>
#include "mtk_jpd_core.h"
#include "mtk_platform_jpd.h"

//-------------------------------------------------------------------------------------------------
// Local Defines
//-------------------------------------------------------------------------------------------------
static int jpd_core_debug;
module_param(jpd_core_debug, int, 0644);

#define LEVEL_ALWAYS    0
#define LEVEL_FLOW      0x1
#define LEVEL_DEBUG     0x1
#define LEVEL_OUTPUT    0x2
#define LEVEL_CAPTURE   0x4
#define LEVEL_BUFFER    0x8
#define LEVEL_QUERY     0x10

#define jpd_debug(level, fmt, args...) \
	do { \
		if ((jpd_core_debug & level) == level) \
			pr_info("jpd_core: %s: " fmt "\n", __func__, ##args); \
	} while (0)

#define jpd_n_debug(level, fmt, args...) \
	do { \
		if ((jpd_core_debug & level) == level) \
			pr_info("jpd_core: [%d] %s: " fmt "\n", ctx->id, __func__, ##args); \
	} while (0)

#define jpd_err(fmt, args...) \
	pr_err("jpd_core: [ERR] %s: " fmt "\n", __func__, ##args)

#define jpd_n_err(fmt, args...) \
	pr_err("jpd_core: [ERR][%d] %s: " fmt "\n", ctx->id, __func__, ##args)

#define WAKE_UP_DRAIN(ctx) \
	do { \
		if (ctx->draining) { \
			ctx->draining = false; \
			jpd_n_debug(LEVEL_ALWAYS, "wake up drain_wq"); \
			wake_up_interruptible(&ctx->drain_wq); \
		} \
	} while (0)

#define MAX_JPEG_WIDTH              16360
#define MAX_JPEG_HEIGHT             16360
#define MAX_MJPEG_WIDTH             1920
#define MAX_MJPEG_HEIGHT            1088
#define JPD_STEP_SIZE               1
#define MAX_FRAME_RATE_NUMERATOR    60000
#define MIN_FRAME_RATE_NUMERATOR    1000
#define FRAME_RATE_DENOMINATOR      1000

//-------------------------------------------------------------------------------------------------
// Local Structures
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------------------------
static int mtk_jpd_queue_setup(struct vb2_queue *vq, unsigned int *num_buffers,
			       unsigned int *num_planes, unsigned int sizes[],
			       struct device *alloc_devs[]);
static int mtk_jpd_start_streaming(struct vb2_queue *q, unsigned int count);
static void mtk_jpd_stop_streaming(struct vb2_queue *q);
static int mtk_jpd_buf_prepare(struct vb2_buffer *vb);
static void mtk_jpd_buf_finish(struct vb2_buffer *vb);
static void mtk_jpd_buf_queue(struct vb2_buffer *vb);
static int mtk_jpd_g_v_ctrl(struct v4l2_ctrl *ctrl);
static int mtk_jpd_s_ctrl(struct v4l2_ctrl *ctrl);

static const struct v4l2_event ev_src_ch = {
	.type = V4L2_EVENT_SOURCE_CHANGE,
	.u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION,
};

static const struct v4l2_event ev_eos = {
	.type = V4L2_EVENT_EOS,
};

static const struct v4l2_event ev_error = {
	.type = V4L2_EVENT_MTK_JPD_ERROR,
};

static const struct vb2_ops mtk_jpd_vb2_ops = {
	.queue_setup = mtk_jpd_queue_setup,
	.start_streaming = mtk_jpd_start_streaming,
	.stop_streaming = mtk_jpd_stop_streaming,
	.buf_prepare = mtk_jpd_buf_prepare,
	.buf_finish = mtk_jpd_buf_finish,
	.buf_queue = mtk_jpd_buf_queue,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static const struct v4l2_ctrl_ops mtk_jpd_ctrl_ops = {
	.g_volatile_ctrl = mtk_jpd_g_v_ctrl,
	.s_ctrl = mtk_jpd_s_ctrl,
};

static const struct v4l2_ctrl_config jpd_v4l2_ctrl_configs[] = {
	{
	 .ops = &mtk_jpd_ctrl_ops,
	 .type = V4L2_CTRL_TYPE_INTEGER,
	 .flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .id = V4L2_CID_JPD_FRAME_METADATA_FD,
	 .name = "JPD metadata fd related to frame buffer",
	 .min = -1,
	 .max = 0x7fffffff,
	 .step = 1,
	 .def = -1,		// default invalid
	 },
	{
	 .ops = &mtk_jpd_ctrl_ops,
	 .type = V4L2_CTRL_TYPE_U8,
	 .flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .id = V4L2_CID_JPD_ACQUIRE_RESOURCE,
	 .name = "JPD acquire resource",
	 .min = 0,
	 .max = 0xFF,
	 .step = 1,
	 .def = 0,
	 .dims = {sizeof(struct v4l2_jpd_resource_parameter)},
	 },
	{
	 .ops = &mtk_jpd_ctrl_ops,
	 .type = V4L2_CTRL_TYPE_U8,
	 .flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .id = V4L2_CID_JPD_OUTPUT_FORMAT,
	 .name = "JPD output format",
	 .min = 0,
	 .max = 0xFF,
	 .step = 1,
	 .def = 0,
	 .dims = {sizeof(struct v4l2_jpd_output_format)},
	 },
	{
	 .ops = &mtk_jpd_ctrl_ops,
	 .type = V4L2_CTRL_TYPE_U8,
	 .flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .id = V4L2_CID_JPD_VERIFICATION_MODE,
	 .name = "JPD verification mode",
	 .min = 0,
	 .max = 0xFF,
	 .step = 1,
	 .def = 0,
	 .dims = {sizeof(struct v4l2_jpd_verification_mode)},
	 },
};

//-------------------------------------------------------------------------------------------------
// Local Functions
//-------------------------------------------------------------------------------------------------
static inline struct mtk_jpd_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct mtk_jpd_ctx, fh);
}

static inline struct mtk_jpd_ctx *ctrl_to_ctx(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct mtk_jpd_ctx, ctrl_handler);
}

static inline struct mtk_jpd_q_data *mtk_jpd_get_q_data(struct mtk_jpd_ctx *ctx,
							enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->out_q;

	return &ctx->cap_q;
}

static inline struct mtk_jpd_buf *mtk_jpd_vb2_to_jpdbuf(struct vb2_buffer *vb)
{
	return container_of(to_vb2_v4l2_buffer(vb), struct mtk_jpd_buf, vvb);
}

static inline void mtk_jpd_queue_res_chg_event(struct mtk_jpd_ctx *ctx)
{
	v4l2_event_queue_fh(&ctx->fh, &ev_src_ch);
	v4l2_m2m_set_dst_buffered(ctx->m2m_ctx, true);
}

static inline void mtk_jpd_queue_stop_play_event(struct mtk_jpd_ctx *ctx)
{
	v4l2_event_queue_fh(&ctx->fh, &ev_eos);
}

static inline void mtk_jpd_queue_error_event(struct mtk_jpd_ctx *ctx)
{
	v4l2_event_queue_fh(&ctx->fh, &ev_error);
}

void mtk_jpd_empty_output_queues(struct mtk_jpd_ctx *ctx)
{
	struct vb2_v4l2_buffer *src_buf = NULL;
	struct mtk_jpd_buf *jpd_buf;

	while ((src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx))) {
		if (src_buf != &ctx->fake_output_buf->vvb) {
			jpd_buf = container_of(src_buf, struct mtk_jpd_buf, vvb);
			platform_jpd_free_cmd(ctx->platform_handle,
					      (platform_jpd_base_cmd *)jpd_buf->driver_priv);
			v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);
		}
	}
}

int mtk_jpd_ctrls_setup(struct mtk_jpd_ctx *ctx)
{
	u32 ctrl_count, ctrl_num;

	v4l2_ctrl_handler_init(&ctx->ctrl_handler, MTK_MAX_CTRLS_HINT);

	ctrl_num = sizeof(jpd_v4l2_ctrl_configs) / sizeof(struct v4l2_ctrl_config);

	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++)
		v4l2_ctrl_new_custom(&ctx->ctrl_handler, &jpd_v4l2_ctrl_configs[ctrl_count], NULL);

	if (unlikely(ctx->ctrl_handler.error)) {
		jpd_n_err("Adding control failed(%d)", ctx->ctrl_handler.error);
		return ctx->ctrl_handler.error;
	}

	v4l2_ctrl_handler_setup(&ctx->ctrl_handler);

	return 0;
}

static int mtk_jpd_queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	struct mtk_jpd_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_DMABUF | VB2_MMAP;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct mtk_jpd_buf);
	src_vq->ops = &mtk_jpd_vb2_ops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->jpd_dev->dev_mutex;
	src_vq->dev = ctx->jpd_dev->dev;
	ret = vb2_queue_init(src_vq);
	if (unlikely(ret)) {
		jpd_n_err("vb2_queue_init(output) failed(%d)", ret);
		return ret;
	}

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_DMABUF | VB2_MMAP;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct mtk_jpd_buf);
	dst_vq->ops = &mtk_jpd_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->jpd_dev->dev_mutex;
	dst_vq->dev = ctx->jpd_dev->dev;
	ret = vb2_queue_init(dst_vq);
	if (unlikely(ret)) {
		vb2_queue_release(src_vq);
		jpd_n_err("vb2_queue_init(capture) failed(%d)", ret);
	}

	return ret;
}

static int mtk_jpd_return_buf(platform_jpd_base_cmd *base_cmd,
			      platform_jpd_cmd_state state, void *user_priv)
{
	struct mtk_jpd_ctx *ctx = (struct mtk_jpd_ctx *)user_priv;
	struct vb2_v4l2_buffer *vvb = (struct vb2_v4l2_buffer *)base_cmd->buf_priv;

	vvb->vb2_buf.timestamp = base_cmd->timestamp;

	switch (state) {
	case E_PLATFORM_JPD_CMD_STATE_DECODE_DONE:
		v4l2_m2m_buf_done(vvb, VB2_BUF_STATE_DONE);
		break;
	case E_PLATFORM_JPD_CMD_STATE_ERROR:
	case E_PLATFORM_JPD_CMD_STATE_SKIP:
	case E_PLATFORM_JPD_CMD_STATE_DROP:
		vb2_set_plane_payload(&vvb->vb2_buf, 0, 0);
		v4l2_m2m_buf_done(vvb, VB2_BUF_STATE_ERROR);
		if (state == E_PLATFORM_JPD_CMD_STATE_ERROR &&
			ctx->out_q.fourcc == V4L2_PIX_FMT_JPEG)
			mtk_jpd_queue_error_event(ctx);
		break;
	case E_PLATFORM_JPD_CMD_STATE_EOS:
		jpd_n_debug(LEVEL_ALWAYS, "FB(%d) eos callback", vvb->vb2_buf.index);
		vvb->flags |= V4L2_BUF_FLAG_LAST;
		vb2_set_plane_payload(&vvb->vb2_buf, 0, 0);
		v4l2_m2m_buf_done(vvb, VB2_BUF_STATE_DONE);
		mtk_jpd_queue_stop_play_event(ctx);
		break;
	case E_PLATFORM_JPD_CMD_STATE_DRAIN_DONE:
		jpd_n_debug(LEVEL_ALWAYS, "FB(%d) drain callback", vvb->vb2_buf.index);
		WAKE_UP_DRAIN(ctx);
		vb2_set_plane_payload(&vvb->vb2_buf, 0, 0);
		v4l2_m2m_buf_done(vvb, VB2_BUF_STATE_ERROR);
		break;
	default:
		jpd_n_err("bad state(%d)", state);
		return -EFAULT;
	}

	if (vvb->vb2_buf.vb2_queue->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		jpd_n_debug(LEVEL_OUTPUT, "BS(%d) state(%d)", vvb->vb2_buf.index, state);
	else
		jpd_n_debug(LEVEL_CAPTURE, "FB(%d) state(%d)", vvb->vb2_buf.index, state);

	return 0;
}

//-------------------------------------------------------------------------------------------------
// Operations Functions
//-------------------------------------------------------------------------------------------------
static int mtk_jpd_open(struct file *file)
{
	struct mtk_jpd_dev *jpd_dev = video_drvdata(file);
	struct mtk_jpd_ctx *ctx;
	struct mtk_jpd_buf *jpd_buf;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (unlikely(!ctx)) {
		jpd_err("alloc mtk_jpd_ctx failed");
		return -ENOMEM;
	}
	jpd_buf = kzalloc(sizeof(*jpd_buf), GFP_KERNEL);
	if (unlikely(!jpd_buf)) {
		jpd_err("alloc mtk_jpd_buf failed");
		kfree(ctx);
		return -ENOMEM;
	}

	ret = platform_jpd_open(&ctx->platform_handle, ctx, mtk_jpd_return_buf);
	if (unlikely(ret)) {
		jpd_n_err("platform_jpd_open failed(%d)", ret);
		goto err_platform_open;
	}

	mutex_lock(&jpd_dev->dev_mutex);
	v4l2_fh_init(&ctx->fh, video_devdata(file));
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);
	ctx->id = jpd_dev->id_counter++;
	ctx->fake_output_buf = jpd_buf;

	ctx->jpd_dev = jpd_dev;

	ret = mtk_jpd_ctrls_setup(ctx);
	if (unlikely(ret)) {
		jpd_n_err("mtk_jpd_ctrls_setup failed(%d)", ret);
		goto err_ctrls_setup;
	}
	ctx->fh.ctrl_handler = &ctx->ctrl_handler;

	ctx->m2m_ctx = v4l2_m2m_ctx_init(jpd_dev->m2m_dev, ctx, mtk_jpd_queue_init);
	if (IS_ERR((__force void *)ctx->m2m_ctx)) {
		ret = PTR_ERR((__force void *)ctx->m2m_ctx);
		jpd_n_err("v4l2_m2m_ctx_init failed(%d)", ret);
		goto err_m2m_ctx_init;
	}
	ctx->m2m_ctx->q_lock = &ctx->jpd_dev->dev_mutex;
	ctx->fh.m2m_ctx = ctx->m2m_ctx;

	jpd_buf->vvb.vb2_buf.vb2_queue =
	    v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	spin_lock_init(&ctx->q_data_lock);
	init_waitqueue_head(&ctx->drain_wq);

	ctx->active = true;

	mutex_unlock(&jpd_dev->dev_mutex);

	jpd_n_debug(LEVEL_FLOW, "%s decoder, platform(%d)", dev_name(jpd_dev->dev),
		    ctx->platform_handle);

	return 0;

err_m2m_ctx_init:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
err_ctrls_setup:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&jpd_dev->dev_mutex);
	platform_jpd_close(ctx->platform_handle);
err_platform_open:
	kfree(jpd_buf);
	kfree(ctx);

	return ret;
}

static int mtk_jpd_release(struct file *file)
{
	struct mtk_jpd_dev *jpd_dev = video_drvdata(file);
	struct mtk_jpd_ctx *ctx = fh_to_ctx(file->private_data);

	jpd_n_debug(LEVEL_FLOW, "%s decoder, platform(%d)", dev_name(jpd_dev->dev),
		    ctx->platform_handle);

	WAKE_UP_DRAIN(ctx);

	mutex_lock(&jpd_dev->dev_mutex);

	v4l2_m2m_streamoff(file, ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	v4l2_m2m_streamoff(file, ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	mtk_jpd_empty_output_queues(ctx);
	jpd_n_debug(LEVEL_FLOW, "set state to FREE");
	ctx->state = MTK_JPD_STATE_FREE;

	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);

	mutex_unlock(&jpd_dev->dev_mutex);

	platform_jpd_close(ctx->platform_handle);

	kfree(ctx->fake_output_buf);
	kfree(ctx);

	return 0;
}

static const struct v4l2_file_operations mtk_jpd_file_fops = {
	.owner = THIS_MODULE,
	.open = mtk_jpd_open,
	.release = mtk_jpd_release,
	.poll = v4l2_m2m_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_m2m_fop_mmap,
};

static int mtk_jpd_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	struct mtk_jpd_dev *jpd_dev = video_drvdata(file);
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);

	strlcpy(cap->driver, MTK_JPD_NAME, sizeof(cap->driver));
	strlcpy(cap->card, MTK_JPD_NAME, sizeof(cap->card));
	if (snprintf(cap->bus_info, sizeof(cap->bus_info),
				"platform:%s", dev_name(jpd_dev->dev)) < 0)
		cap->bus_info[0] = '\0';
	cap->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	jpd_n_debug(LEVEL_DEBUG, "driver(%s) bus_info(%s) capabilities(0x%x)",
		    MTK_JPD_NAME, cap->bus_info, cap->capabilities);

	return 0;
}

static int mtk_jpd_enum_fmt_vid_out(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	char *fourcc = (char *)(&f->pixelformat);

	switch (f->index) {
	case 0:
		f->pixelformat = V4L2_PIX_FMT_JPEG;
		break;
	case 1:
		f->pixelformat = V4L2_PIX_FMT_MJPEG;
		break;
	default:
		jpd_n_err("invalid index(%d)", f->index);
		return -EINVAL;
	}
	f->flags = V4L2_FMT_FLAG_COMPRESSED;
	memset(f->reserved, 0, sizeof(f->reserved));

	jpd_n_debug(LEVEL_QUERY, "index(%d) fourcc %c%c%c%c",
		    f->index, fourcc[0], fourcc[1], fourcc[2], fourcc[3]);

	return 0;
}

static int mtk_jpd_enum_fmt_vid_cap(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	char *fourcc = (char *)(&f->pixelformat);

	switch (f->index) {
	case 0:
		f->pixelformat = V4L2_PIX_FMT_YUYV;
		break;
	default:
		jpd_n_err("invalid index(%d)", f->index);
		return -EINVAL;
	}
	f->flags = 0;
	memset(f->reserved, 0, sizeof(f->reserved));

	jpd_n_debug(LEVEL_QUERY, "index(%d) fourcc %c%c%c%c",
		    f->index, fourcc[0], fourcc[1], fourcc[2], fourcc[3]);

	return 0;
}

static int mtk_jpd_enum_framesizes(struct file *file, void *priv, struct v4l2_frmsizeenum *fsize)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	char *fourcc = (char *)(&fsize->pixel_format);

	if (fsize->index != 0) {
		jpd_n_err("invalid index(%d)", fsize->index);
		return -EINVAL;
	}

	switch (fsize->pixel_format) {
	case V4L2_PIX_FMT_JPEG:
		fsize->stepwise.max_width = MAX_JPEG_WIDTH;
		fsize->stepwise.max_height = MAX_JPEG_HEIGHT;
		break;
	case V4L2_PIX_FMT_MJPEG:
		fsize->stepwise.max_width = MAX_MJPEG_WIDTH;
		fsize->stepwise.max_height = MAX_MJPEG_HEIGHT;
		break;
	default:
		jpd_n_err("unkonw pixelformat %c%c%c%c", fourcc[0], fourcc[1], fourcc[2],
			  fourcc[3]);
		return -EINVAL;
	}
	fsize->stepwise.min_width = JPD_STEP_SIZE;
	fsize->stepwise.min_height = JPD_STEP_SIZE;
	fsize->stepwise.step_width = JPD_STEP_SIZE;
	fsize->stepwise.step_height = JPD_STEP_SIZE;
	fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
	memset(fsize->reserved, 0, sizeof(fsize->reserved));

	jpd_n_debug(LEVEL_QUERY, "fourcc %c%c%c%c max(%dx%d) min(%dx%d) step(%dx%d)",
		    fourcc[0], fourcc[1], fourcc[2], fourcc[3],
		    fsize->stepwise.max_width, fsize->stepwise.max_height,
		    fsize->stepwise.min_width, fsize->stepwise.min_height,
		    fsize->stepwise.step_width, fsize->stepwise.step_height);

	return 0;
}

static int mtk_jpd_enum_frameintervals(struct file *file, void *priv,
				       struct v4l2_frmivalenum *fival)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	char *fourcc = (char *)(&fival->pixel_format);

	if (fival->index != 0 || fival->width > MAX_MJPEG_WIDTH
						  || fival->height > MAX_MJPEG_HEIGHT) {
		jpd_n_err("invalid params(%d, %dx%d)", fival->index, fival->width, fival->height);
		return -EINVAL;
	}

	switch (fival->pixel_format) {
	case V4L2_PIX_FMT_JPEG:
	case V4L2_PIX_FMT_MJPEG:
		fival->stepwise.max.numerator = MAX_FRAME_RATE_NUMERATOR;
		fival->stepwise.max.denominator = FRAME_RATE_DENOMINATOR;
		fival->stepwise.min.numerator = MIN_FRAME_RATE_NUMERATOR;
		fival->stepwise.min.denominator = FRAME_RATE_DENOMINATOR;
		fival->stepwise.step.numerator = MIN_FRAME_RATE_NUMERATOR;
		fival->stepwise.step.denominator = FRAME_RATE_DENOMINATOR;
		break;
	default:
		jpd_n_err("unkonw pixelformat %c%c%c%c", fourcc[0], fourcc[1], fourcc[2],
			  fourcc[3]);
		return -EINVAL;
	}
	fival->type = V4L2_FRMIVAL_TYPE_CONTINUOUS;

	jpd_n_debug(LEVEL_QUERY, "fourcc %c%c%c%c max(%d/%d) min(%d/%d) step(%d/%d)",
		    fourcc[0], fourcc[1], fourcc[2], fourcc[3],
		    fival->stepwise.max.numerator, fival->stepwise.max.denominator,
		    fival->stepwise.min.numerator, fival->stepwise.min.denominator,
		    fival->stepwise.step.numerator, fival->stepwise.step.denominator);

	return 0;
}

static int mtk_jpd_s_fmt_vid_out(struct file *file, void *priv, struct v4l2_format *f)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	char *fourcc = (char *)(&pix_mp->pixelformat);
	struct mtk_jpd_q_data *q_data;
	struct vb2_queue *vq;
	platform_jpd_format format;
	int ret;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (unlikely(!vq)) {
		jpd_n_err("vq is NULL");
		return -EINVAL;
	}

	if (vb2_is_busy(vq)) {
		jpd_n_err("vb2 is busy");
		return -EBUSY;
	}

	switch (pix_mp->pixelformat) {
	case V4L2_PIX_FMT_JPEG:
		format = E_PLATFORM_JPD_FORMAT_JPEG;
		break;
	case V4L2_PIX_FMT_MJPEG:
		format = E_PLATFORM_JPD_FORMAT_MJPEG;
		break;
	default:
		jpd_n_err("unkonw pixelformat %c%c%c%c", fourcc[0], fourcc[1], fourcc[2],
			  fourcc[3]);
		return -EINVAL;
	}

	q_data = mtk_jpd_get_q_data(ctx, f->type);
	spin_lock(&ctx->q_data_lock);
	q_data->fourcc = pix_mp->pixelformat;
	q_data->sizeimage = pix_mp->plane_fmt[0].sizeimage;
	spin_unlock(&ctx->q_data_lock);

	ret = platform_jpd_set_format(ctx->platform_handle, format);
	if (unlikely(ret)) {
		jpd_n_err("platform_jpd_set_format(%d) failed(%d)", format, ret);
		return ret;
	}

	if (ctx->state == MTK_JPD_STATE_FREE) {
		v4l2_m2m_set_dst_buffered(ctx->m2m_ctx, true);
		jpd_n_debug(LEVEL_ALWAYS, "set state to INIT");
		ctx->state = MTK_JPD_STATE_INIT;
	}

	jpd_n_debug(LEVEL_ALWAYS, "fourcc %c%c%c%c, %dx%d, sizeimage 0x%x",
		    fourcc[0], fourcc[1], fourcc[2], fourcc[3], pix_mp->width, pix_mp->height,
		    q_data->sizeimage);

	return 0;
}

static int mtk_jpd_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	char *fourcc = (char *)(&pix_mp->pixelformat);
	struct mtk_jpd_q_data *q_data;
	struct vb2_queue *vq;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (unlikely(!vq)) {
		jpd_n_err("vq is NULL");
		return -EINVAL;
	}

	if (vb2_is_busy(vq)) {
		jpd_n_err("vb2 is busy");
		return -EBUSY;
	}

	q_data = mtk_jpd_get_q_data(ctx, f->type);
	spin_lock(&ctx->q_data_lock);
	q_data->fourcc = pix_mp->pixelformat;
	spin_unlock(&ctx->q_data_lock);

	jpd_n_debug(LEVEL_ALWAYS, "fourcc %c%c%c%c", fourcc[0], fourcc[1], fourcc[2], fourcc[3]);

	return 0;
}

static int mtk_jpd_g_fmt_vid(struct file *file, void *priv, struct v4l2_format *f)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	char *fourcc;
	struct mtk_jpd_q_data *q_data;
	struct vb2_queue *vq;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (unlikely(!vq)) {
		jpd_n_err("type(%d) vq is NULL", f->type);
		return -EINVAL;
	}

	q_data = mtk_jpd_get_q_data(ctx, f->type);
	spin_lock(&ctx->q_data_lock);
	pix_mp->pixelformat = q_data->fourcc;
	pix_mp->width = q_data->width;
	pix_mp->height = q_data->height;
	pix_mp->plane_fmt[0].sizeimage = q_data->sizeimage;
	spin_unlock(&ctx->q_data_lock);

	fourcc = (char *)(&pix_mp->pixelformat);
	jpd_n_debug(LEVEL_QUERY, "type(%d) fourcc %c%c%c%c, %dx%d, sizeimage 0x%x", f->type,
		    fourcc[0], fourcc[1], fourcc[2], fourcc[3], pix_mp->width, pix_mp->height,
		    q_data->sizeimage);

	return 0;
}

static int mtk_jpd_qbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *vq;
	struct vb2_buffer *vb;
	struct mtk_jpd_buf *jpd_buf;
	int ret;

	if (unlikely(ctx->state == MTK_JPD_STATE_ABORT)) {
		jpd_n_err("type(%d) state ABORT", buf->type);
		return -EIO;
	}

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, buf->type);
	vb = vb2_get_buffer(vq, buf->index);
	if (unlikely(vb == NULL)) {
		jpd_n_err("type(%d) id=%d, vb is NULL", buf->type, buf->index);
		return -EFAULT;
	}
	jpd_buf = mtk_jpd_vb2_to_jpdbuf(vb);

	jpd_buf->flags = buf->flags;
	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		jpd_buf->lastframe = false;
		jpd_n_debug(LEVEL_OUTPUT, "BS(%d) offset(0x%x) size(0x%x,0x%x) vb=%p flags=0x%x pts=%llu",
			   buf->index, buf->m.planes[0].data_offset, buf->m.planes[0].bytesused,
			   buf->m.planes[0].length, vb, buf->flags, timeval_to_ns(&buf->timestamp));
	} else {
		jpd_n_debug(LEVEL_CAPTURE, "FB(%d) size(0x%x) vb=%p flags=0x%x",
			    buf->index, buf->m.planes[0].length, vb, buf->flags);
	}

	ret = v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);

	if (ret) {
		if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
			jpd_n_err("failed, BS(%d) ret=%d", buf->index, ret);
		else
			jpd_n_err("failed, FB(%d) ret=%d", buf->index, ret);
	}
	return ret;
}

static int mtk_jpd_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	int ret;

	if (unlikely(ctx->state == MTK_JPD_STATE_ABORT)) {
		jpd_n_err("type(%d) state ABORT", buf->type);
		return -EIO;
	}

	ret = v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);

	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (!ret) {
			jpd_n_debug(LEVEL_OUTPUT, "BS(%d) size(0x%x) flags=0x%x pts=%llu",
					buf->index, buf->m.planes[0].length, buf->flags,
					timeval_to_ns(&buf->timestamp));
		} else {
			jpd_n_err("failed, BS ret=%d", ret);
		}
	} else {
		if (!ret) {
			jpd_n_debug(LEVEL_CAPTURE, "FB(%d) bytesused(0x%x) flags=0x%x pts=%llu",
					buf->index, buf->m.planes[0].bytesused, buf->flags,
					timeval_to_ns(&buf->timestamp));
		} else {
			jpd_n_err("failed, FB ret=%d", ret);
		}
	}
	return ret;
}

static int mtk_jpd_subscribe_event(struct v4l2_fh *fh, const struct v4l2_event_subscription *sub)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(fh);

	switch (sub->type) {
	case V4L2_EVENT_EOS:
		jpd_n_debug(LEVEL_DEBUG, "EOS");
		return v4l2_event_subscribe(fh, sub, 2, NULL);
	case V4L2_EVENT_SOURCE_CHANGE:
		jpd_n_debug(LEVEL_DEBUG, "SOURCE_CHANGE");
		return v4l2_src_change_event_subscribe(fh, sub);
	case V4L2_EVENT_MTK_JPD_ERROR:
		jpd_n_debug(LEVEL_DEBUG, "MTK_JPD_ERROR");
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	default:
		jpd_n_debug(LEVEL_DEBUG, "unknown event(0x%x)", sub->type);
		return v4l2_ctrl_subscribe_event(fh, sub);
	}
}

static int mtk_jpd_decoder_cmd(struct file *file, void *priv, struct v4l2_decoder_cmd *cmd)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *src_vq, *dst_vq;

	dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
		src_vq = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
		if (!vb2_is_streaming(src_vq)) {
			jpd_n_debug(LEVEL_ALWAYS, "Output stream is off");
			return 0;
		}
		if (!vb2_is_streaming(dst_vq)) {
			jpd_n_debug(LEVEL_ALWAYS, "Capture stream is off");
			return 0;
		}
		jpd_n_debug(LEVEL_ALWAYS, "receive eos cmd");
		ctx->fake_output_buf->lastframe = true;
		ctx->fake_output_buf->vvb.vb2_buf.planes[0].bytesused = 0;
		v4l2_m2m_buf_queue(ctx->m2m_ctx, &ctx->fake_output_buf->vvb);
		v4l2_m2m_try_schedule(ctx->m2m_ctx);
		break;

	case V4L2_DEC_CMD_START:
		jpd_n_debug(LEVEL_ALWAYS, "receive start cmd");
		vb2_clear_last_buffer_dequeued(dst_vq);
		break;

	default:
		jpd_n_err("unknown cmd(%d)", cmd->cmd);
		return -EINVAL;
	}

	return 0;
}

static int mtk_jpd_g_selection(struct file *file, void *priv, struct v4l2_selection *s)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	struct mtk_jpd_q_data *q_data;

	q_data = mtk_jpd_get_q_data(ctx, s->type);

	if (V4L2_TYPE_IS_OUTPUT(s->type)) {
		switch (s->target) {
		case V4L2_SEL_TGT_CROP_BOUNDS:
		case V4L2_SEL_TGT_CROP_DEFAULT:
			// image
			s->r.left = 0;
			s->r.top = 0;
			spin_lock(&ctx->q_data_lock);
			s->r.width = q_data->width;
			s->r.height = q_data->height;
			spin_unlock(&ctx->q_data_lock);
			break;
		case V4L2_SEL_TGT_CROP:
			// roi
			spin_lock(&ctx->q_data_lock);
			s->r = q_data->crop;
			spin_unlock(&ctx->q_data_lock);
			break;
		default:
			jpd_n_err("bad params, (%d, %d)", s->type, s->target);
			return -EINVAL;
		}
	} else {
		switch (s->target) {
		case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		case V4L2_SEL_TGT_COMPOSE_DEFAULT:
			// buffer
			s->r.left = 0;
			s->r.top = 0;
			spin_lock(&ctx->q_data_lock);
			s->r.width = q_data->width;
			s->r.height = q_data->height;
			spin_unlock(&ctx->q_data_lock);
			break;
		case V4L2_SEL_TGT_COMPOSE:
			// image
			spin_lock(&ctx->q_data_lock);
			s->r = q_data->crop;
			spin_unlock(&ctx->q_data_lock);
			break;
		default:
			jpd_n_err("bad params, (%d, %d)", s->type, s->target);
			return -EINVAL;
		}
	}

	jpd_n_debug(LEVEL_QUERY, "type(%d) target(%d) [%d, %d, %d, %d]",
		    s->type, s->target, s->r.left, s->r.top, s->r.width, s->r.height);

	return 0;
}

static int mtk_jpd_s_selection(struct file *file, void *priv, struct v4l2_selection *s)
{
	struct mtk_jpd_ctx *ctx = fh_to_ctx(priv);
	platform_jpd_rect rect = { s->r.left, s->r.top, s->r.width, s->r.height };
	platform_jpd_pic pic = { s->r.width, s->r.height };

	if (V4L2_TYPE_IS_OUTPUT(s->type)) {
		if (s->target == V4L2_SEL_TGT_CROP && s->flags == V4L2_SEL_FLAG_GE) {
			jpd_n_debug(LEVEL_ALWAYS, "set roi(%d, %d, %d, %d)",
				    s->r.left, s->r.top, s->r.width, s->r.height);
			return platform_jpd_set_roi(ctx->platform_handle, rect);
		}
	} else {
		if (s->target == V4L2_SEL_TGT_COMPOSE && s->flags == V4L2_SEL_FLAG_GE) {
			jpd_n_debug(LEVEL_ALWAYS, "set down scale(%d, %d)", s->r.width,
				    s->r.height);
			return platform_jpd_set_down_scale(ctx->platform_handle, pic);
		} else if (s->target == V4L2_SEL_TGT_COMPOSE && s->flags == V4L2_SEL_FLAG_LE) {
			jpd_n_debug(LEVEL_ALWAYS, "set max resolution(%d, %d)", s->r.width,
				    s->r.height);
			return platform_jpd_set_max_resolution(ctx->platform_handle, pic);
		}
	}

	jpd_n_err("bad params, (%d, %d, %d)", s->type, s->target, s->flags);
	return -EINVAL;
}

static const struct v4l2_ioctl_ops mtk_jpd_ioctl_ops = {
	.vidioc_querycap = mtk_jpd_querycap,
	.vidioc_enum_fmt_vid_out = mtk_jpd_enum_fmt_vid_out,
	.vidioc_enum_fmt_vid_cap = mtk_jpd_enum_fmt_vid_cap,
	.vidioc_enum_framesizes = mtk_jpd_enum_framesizes,
	.vidioc_enum_frameintervals = mtk_jpd_enum_frameintervals,
	.vidioc_s_fmt_vid_out_mplane = mtk_jpd_s_fmt_vid_out,
	.vidioc_s_fmt_vid_cap_mplane = mtk_jpd_s_fmt_vid_cap,
	.vidioc_g_fmt_vid_out_mplane = mtk_jpd_g_fmt_vid,
	.vidioc_g_fmt_vid_cap_mplane = mtk_jpd_g_fmt_vid,
	.vidioc_qbuf = mtk_jpd_qbuf,
	.vidioc_dqbuf = mtk_jpd_dqbuf,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_subscribe_event = mtk_jpd_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_decoder_cmd = mtk_jpd_decoder_cmd,
	.vidioc_g_selection = mtk_jpd_g_selection,
	.vidioc_s_selection = mtk_jpd_s_selection,
};

static void mtk_jpd_device_run(void *priv)
{
	struct mtk_jpd_ctx *ctx = priv;
	struct mtk_jpd_dev *jpd_dev = ctx->jpd_dev;
	struct vb2_v4l2_buffer *src_buf;
	struct mtk_jpd_buf *jpd_buf;
	int ret;

	if (unlikely(ctx->state != MTK_JPD_STATE_HEADER)) {
		jpd_n_err("wrong state(%d)", ctx->state);
		goto job_finish;
	}

	src_buf = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	if (unlikely(src_buf == NULL)) {
		jpd_n_err("src_buf is empty");
		goto job_finish;
	}

	jpd_buf = container_of(src_buf, struct mtk_jpd_buf, vvb);

	if (unlikely(src_buf == &ctx->fake_output_buf->vvb && jpd_buf->lastframe)) {
		jpd_n_debug(LEVEL_ALWAYS, "send eos to driver");
		ret = platform_jpd_eos(ctx->platform_handle);
		if (unlikely(ret))
			jpd_n_err("platform_jpd_eos failed(%d)", ret);

		v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		goto job_finish;
	}

	if (unlikely(ctx->last_decoded_q.width != jpd_buf->decoded_q.width ||
		     ctx->last_decoded_q.height != jpd_buf->decoded_q.height)) {
		jpd_n_debug(LEVEL_ALWAYS,
			    "BS(%d) new sequence, image WxH(%dx%d) buf WxH(%dx%d)[%d %d %dx%d](0x%x)",
			    src_buf->vb2_buf.index, jpd_buf->decoded_q.width,
			    jpd_buf->decoded_q.height, jpd_buf->display_q.width,
			    jpd_buf->display_q.height, jpd_buf->display_q.crop.left,
			    jpd_buf->display_q.crop.top, jpd_buf->display_q.crop.width,
			    jpd_buf->display_q.crop.height, jpd_buf->display_q.sizeimage);

		spin_lock(&ctx->q_data_lock);
		ctx->last_decoded_q.width = jpd_buf->decoded_q.width;
		ctx->last_decoded_q.height = jpd_buf->decoded_q.height;
		ctx->last_decoded_q.crop.left = jpd_buf->decoded_q.crop.left;
		ctx->last_decoded_q.crop.top = jpd_buf->decoded_q.crop.top;
		ctx->last_decoded_q.crop.width = jpd_buf->decoded_q.crop.width;
		ctx->last_decoded_q.crop.height = jpd_buf->decoded_q.crop.height;
		ctx->last_display_q.width = jpd_buf->display_q.width;
		ctx->last_display_q.height = jpd_buf->display_q.height;
		ctx->last_display_q.crop.left = jpd_buf->display_q.crop.left;
		ctx->last_display_q.crop.top = jpd_buf->display_q.crop.top;
		ctx->last_display_q.crop.width = jpd_buf->display_q.crop.width;
		ctx->last_display_q.crop.height = jpd_buf->display_q.crop.height;
		ctx->last_display_q.sizeimage = jpd_buf->display_q.sizeimage;
		spin_unlock(&ctx->q_data_lock);

		jpd_n_debug(LEVEL_ALWAYS, "BS(%d) start draining", src_buf->vb2_buf.index);
		ctx->draining = true;
		ret = platform_jpd_drain(ctx->platform_handle);
		if (unlikely(ret)) {
			jpd_n_err("platform_jpd_drain failed(%d)", ret);
			ctx->draining = false;
		}
		jpd_n_debug(LEVEL_ALWAYS, "BS(%d) waiting drain", src_buf->vb2_buf.index);
		ret = wait_event_interruptible(ctx->drain_wq, !ctx->draining);
		if (unlikely(ret != 0))
			jpd_err("wait draining corrupted(%d)", ret);

		jpd_n_debug(LEVEL_ALWAYS, "BS(%d) drain done", src_buf->vb2_buf.index);

		mtk_jpd_queue_res_chg_event(ctx);
		goto job_finish;
	}

	ret = platform_jpd_insert_cmd(ctx->platform_handle,
				    (platform_jpd_base_cmd *)jpd_buf->driver_priv);
	if (unlikely(ret)) {
		jpd_n_err("platform_jpd_insert_cmd failed(%d)", ret);
		v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);
	}

	v4l2_m2m_src_buf_remove(ctx->m2m_ctx);

job_finish:
	v4l2_m2m_job_finish(jpd_dev->m2m_dev, ctx->m2m_ctx);
}

static int mtk_jpd_job_ready(void *priv)
{
	struct mtk_jpd_ctx *ctx = priv;

	if (ctx->state == MTK_JPD_STATE_ABORT)
		return 0;

	if ((ctx->out_q.width != ctx->last_decoded_q.width) ||
	    (ctx->out_q.height != ctx->last_decoded_q.height)) {
		return 0;
	}

	if (ctx->state != MTK_JPD_STATE_HEADER)
		return 0;

	return 1;
}

static void mtk_jpd_job_abort(void *priv)
{
	struct mtk_jpd_ctx *ctx = priv;

	jpd_n_debug(LEVEL_ALWAYS, "set state to ABORT");
	ctx->state = MTK_JPD_STATE_ABORT;
}

const struct v4l2_m2m_ops mtk_jpd_m2m_ops = {
	.device_run = mtk_jpd_device_run,
	.job_ready = mtk_jpd_job_ready,
	.job_abort = mtk_jpd_job_abort,
};

static int mtk_jpd_queue_setup(struct vb2_queue *vq, unsigned int *num_buffers,
			       unsigned int *num_planes, unsigned int sizes[],
			       struct device *alloc_devs[])
{
	struct mtk_jpd_ctx *ctx = vb2_get_drv_priv(vq);
	struct mtk_jpd_q_data *q_data;

	q_data = mtk_jpd_get_q_data(ctx, vq->type);

	if (unlikely(!q_data)) {
		jpd_n_err("type(%d) q_data is NULL", vq->type);
		return -EINVAL;
	}

	*num_planes = 1;
	spin_lock(&ctx->q_data_lock);
	sizes[0] = q_data->sizeimage;
	spin_unlock(&ctx->q_data_lock);

	jpd_n_debug(LEVEL_ALWAYS, "type(%d) buffer num(%d) size(0x%x)",
		    vq->type, *num_buffers, sizes[0]);

	return 0;
}

static int mtk_jpd_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct mtk_jpd_ctx *ctx = vb2_get_drv_priv(q);
	int ret;

	jpd_n_debug(LEVEL_ALWAYS, "type(%d) state(%d)", q->type, ctx->state);

	if (ctx->state == MTK_JPD_STATE_FLUSH) {
		jpd_n_debug(LEVEL_ALWAYS, "set state to HEADER");
		ctx->state = MTK_JPD_STATE_HEADER;
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		ret = platform_jpd_start_port(ctx->platform_handle, E_PLATFORM_JPD_PORT_READ);
	else
		ret = platform_jpd_start_port(ctx->platform_handle, E_PLATFORM_JPD_PORT_WRITE);

	if (unlikely(ret))
		jpd_n_err("type(%d) platform_jpd_start_port failed(%d)", q->type, ret);

	return ret;
}

static void mtk_jpd_stop_streaming(struct vb2_queue *q)
{
	struct mtk_jpd_ctx *ctx = vb2_get_drv_priv(q);
	int ret;

	jpd_n_debug(LEVEL_ALWAYS, "type(%d) state(%d)", q->type, ctx->state);

	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = platform_jpd_flush_port(ctx->platform_handle, E_PLATFORM_JPD_PORT_READ);
		mtk_jpd_empty_output_queues(ctx);
	} else {
		ret = platform_jpd_flush_port(ctx->platform_handle, E_PLATFORM_JPD_PORT_WRITE);
	}

	if (unlikely(ret))
		jpd_n_err("type(%d) platform_jpd_flush_port failed(%d)", q->type, ret);

	WAKE_UP_DRAIN(ctx);

	if (ctx->state >= MTK_JPD_STATE_HEADER) {
		ctx->state = MTK_JPD_STATE_FLUSH;
		jpd_n_debug(LEVEL_ALWAYS, "set state to FLUSH");
		jpd_n_debug(LEVEL_ALWAYS,
			"ES state(%d), old(%dx%d)(0x%x) -> new(%dx%d)(0x%x)",
			ctx->state, ctx->out_q.width, ctx->out_q.height, ctx->out_q.sizeimage,
			ctx->last_decoded_q.width, ctx->last_decoded_q.height,
			ctx->last_decoded_q.sizeimage);
		jpd_n_debug(LEVEL_ALWAYS,
			"FB state(%d), old(%dx%d)[%d %d %dx%d](0x%x) -> new(%dx%d)[%d %d %dx%d](0x%x)",
			ctx->state, ctx->cap_q.width, ctx->cap_q.height,
			ctx->cap_q.sizeimage, ctx->cap_q.crop.left, ctx->cap_q.crop.top,
			ctx->cap_q.crop.width, ctx->cap_q.crop.height,
			ctx->last_display_q.width, ctx->last_display_q.height,
			ctx->last_display_q.sizeimage, ctx->last_display_q.crop.left,
			ctx->last_display_q.crop.top, ctx->last_display_q.crop.width,
			ctx->last_display_q.crop.height);

		spin_lock(&ctx->q_data_lock);
		memcpy(&ctx->out_q, &ctx->last_decoded_q, sizeof(struct mtk_jpd_q_data));
		memcpy(&ctx->cap_q, &ctx->last_display_q, sizeof(struct mtk_jpd_q_data));
		spin_unlock(&ctx->q_data_lock);
	}
}

static int mtk_jpd_buf_prepare(struct vb2_buffer *vb)
{
	struct mtk_jpd_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_jpd_q_data *q_data = NULL;

	q_data = mtk_jpd_get_q_data(ctx, vb->vb2_queue->type);
	if (unlikely(!q_data)) {
		jpd_n_err("type(%d) index=%d, q_data is NULL", vb->vb2_queue->type, vb->index);
		return -EINVAL;
	}

	if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		jpd_n_debug((LEVEL_OUTPUT | LEVEL_BUFFER), "BS(%d) vb=%p", vb->index, vb);
	else
		jpd_n_debug((LEVEL_CAPTURE | LEVEL_BUFFER), "FB(%d) vb=%p", vb->index, vb);

	if (unlikely(vb2_plane_size(vb, 0) < q_data->sizeimage)) {
		jpd_n_err("bad vb2_plane_size(0x%lx < 0x%x)", vb2_plane_size(vb, 0),
			  q_data->sizeimage);
	}

	return 0;
}

static void mtk_jpd_buf_finish(struct vb2_buffer *vb)
{
	struct mtk_jpd_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_jpd_buf *jpd_buf;
	struct dma_buf_attachment *buf_att;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned long size;

	if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		jpd_n_debug((LEVEL_OUTPUT | LEVEL_BUFFER), "BS(%d) vb=%p", vb->index, vb);
	} else {
		jpd_n_debug((LEVEL_CAPTURE | LEVEL_BUFFER), "FB(%d) vb=%p", vb->index, vb);

		jpd_buf = mtk_jpd_vb2_to_jpdbuf(vb);
		if (vb->vb2_queue->memory == VB2_MEMORY_DMABUF &&
		    !(jpd_buf->flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE)) {
			jpd_n_debug(LEVEL_BUFFER, "FB(%d) Cache sync+", vb->index);

			buf_att = dma_buf_attach(vb->planes[0].dbuf, ctx->jpd_dev->dev);
			sgt = dma_buf_map_attachment(buf_att, DMA_FROM_DEVICE);
			dma_sync_sg_for_cpu(ctx->jpd_dev->dev, sgt->sgl, sgt->orig_nents,
					    DMA_FROM_DEVICE);
			dma_buf_unmap_attachment(buf_att, sgt, DMA_FROM_DEVICE);

			dma_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
			size = vb2_plane_size(vb, 0);
			dma_buf_detach(vb->planes[0].dbuf, buf_att);

			jpd_n_debug(LEVEL_BUFFER, "FB(%d) Cache sync- iova=%pad size=0x%lx",
				    vb->index, &dma_addr, size);
		}
	}
}

static void mtk_jpd_buf_queue(struct vb2_buffer *vb)
{
	struct mtk_jpd_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vvb = to_vb2_v4l2_buffer(vb);
	struct mtk_jpd_buf *jpd_buf;
	struct dma_buf_attachment *buf_att;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned long size;
	platform_jpd_buf plat_buf;
	platform_jpd_image_info image_info;
	platform_jpd_buf_info buf_info;
	platform_jpd_parser_result plat_ret;
	int ret;

	memset(&plat_buf, 0, sizeof(platform_jpd_buf));

	if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		jpd_buf = mtk_jpd_vb2_to_jpdbuf(vb);
		plat_buf.pa = vb2_dma_contig_plane_dma_addr(vb, 0);
		plat_buf.va = vb2_plane_vaddr(vb, 0);
		plat_buf.offset = vb->planes[0].data_offset;
		plat_buf.filled_length = vb2_get_plane_payload(vb, 0);
		plat_buf.size = vb2_plane_size(vb, 0);
		jpd_n_debug(LEVEL_OUTPUT, "BS(%d) vb=%p iova=%pad size=0x%zx va=%p offset=0x%zx filled=0x%zx",
			    vb->index, vb, &plat_buf.pa, plat_buf.size, plat_buf.va,
			    plat_buf.offset, plat_buf.filled_length);

		plat_ret = platform_jpd_prepare_decode(ctx->platform_handle,
						plat_buf, vvb,
						(platform_jpd_base_cmd **)&jpd_buf->driver_priv);
		if (unlikely(plat_ret != E_PLATFORM_JPD_PARSER_OK)) {
			jpd_n_err("BS(%d) platform_jpd_prepare_decode failed(%d)",
						vb->index, plat_ret);
			v4l2_m2m_buf_done(vvb, VB2_BUF_STATE_ERROR);
			if (plat_ret == E_PLATFORM_JPD_PARSER_FAIL &&
				ctx->out_q.fourcc == V4L2_PIX_FMT_JPEG) {
				mtk_jpd_queue_error_event(ctx);
			}
			return;
		}
		((platform_jpd_base_cmd *)jpd_buf->driver_priv)->timestamp = vvb->vb2_buf.timestamp;

		ret = platform_jpd_get_image_info(ctx->platform_handle, &image_info);
		if (unlikely(ret)) {
			jpd_n_err("BS(%d) platform_jpd_get_image_info failed(%d)", vb->index, ret);
			v4l2_m2m_buf_done(vvb, VB2_BUF_STATE_ERROR);
			return;
		}
		jpd_buf->decoded_q.width = image_info.width;
		jpd_buf->decoded_q.height = image_info.height;
		jpd_buf->decoded_q.crop.left = 0;
		jpd_buf->decoded_q.crop.top = 0;
		jpd_buf->decoded_q.crop.width = image_info.width;
		jpd_buf->decoded_q.crop.height = image_info.height;

		ret = platform_jpd_get_buf_info(ctx->platform_handle, &buf_info);
		if (unlikely(ret)) {
			jpd_n_err("BS(%d) platform_jpd_get_buf_info failed(%d)", vb->index, ret);
			v4l2_m2m_buf_done(vvb, VB2_BUF_STATE_ERROR);
			return;
		}
		jpd_buf->display_q.width = buf_info.width;
		jpd_buf->display_q.height = buf_info.height;
		jpd_buf->display_q.crop.left = buf_info.rect.x;
		jpd_buf->display_q.crop.top = buf_info.rect.y;
		jpd_buf->display_q.crop.width = buf_info.rect.width;
		jpd_buf->display_q.crop.height = buf_info.rect.height;
		jpd_buf->display_q.sizeimage = buf_info.write_buf_size;

		jpd_n_debug(LEVEL_DEBUG,
			    "BS(%d) image WxH(%dx%d) buf WxH(%dx%d)[%d %d %dx%d](0x%x) ret(%d)",
			    vb->index, image_info.width, image_info.height, buf_info.width,
			    buf_info.height, buf_info.rect.x, buf_info.rect.y, buf_info.rect.width,
			    buf_info.rect.height, buf_info.write_buf_size, plat_ret);

		if (vb->vb2_queue->memory == VB2_MEMORY_DMABUF &&
		    !(jpd_buf->flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN)) {
			jpd_n_debug(LEVEL_BUFFER, "BS(%d) Cache sync+", vb->index);

			buf_att = dma_buf_attach(vb->planes[0].dbuf, ctx->jpd_dev->dev);
			sgt = dma_buf_map_attachment(buf_att, DMA_TO_DEVICE);
			dma_sync_sg_for_device(ctx->jpd_dev->dev, sgt->sgl, sgt->orig_nents,
					       DMA_TO_DEVICE);
			dma_buf_unmap_attachment(buf_att, sgt, DMA_TO_DEVICE);

			dma_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
			size = vb2_plane_size(vb, 0);
			dma_buf_detach(vb->planes[0].dbuf, buf_att);

			jpd_n_debug(LEVEL_BUFFER, "BS(%d) Cache sync- iova=%pad size=0x%lx",
				    vb->index, &dma_addr, size);
		}

		if (ctx->state == MTK_JPD_STATE_INIT) {
			jpd_n_debug(LEVEL_ALWAYS,
				    "BS(%d) new sequence, image WxH(%dx%d) buf WxH(%dx%d)[%d %d %dx%d](0x%x)",
				    vb->index, image_info.width, image_info.height, buf_info.width,
				    buf_info.height, buf_info.rect.x, buf_info.rect.y,
				    buf_info.rect.width, buf_info.rect.height,
				    buf_info.write_buf_size);

			spin_lock(&ctx->q_data_lock);
			ctx->out_q.width = image_info.width;
			ctx->out_q.height = image_info.height;
			ctx->out_q.crop.left = 0;
			ctx->out_q.crop.top = 0;
			ctx->out_q.crop.width = image_info.width;
			ctx->out_q.crop.height = image_info.height;
			ctx->cap_q.width = buf_info.width;
			ctx->cap_q.height = buf_info.height;
			ctx->cap_q.crop.left = buf_info.rect.x;
			ctx->cap_q.crop.top = buf_info.rect.y;
			ctx->cap_q.crop.width = buf_info.rect.width;
			ctx->cap_q.crop.height = buf_info.rect.height;
			ctx->cap_q.sizeimage = buf_info.write_buf_size;
			memcpy(&ctx->last_decoded_q, &ctx->out_q, sizeof(struct mtk_jpd_q_data));
			memcpy(&ctx->last_display_q, &ctx->cap_q, sizeof(struct mtk_jpd_q_data));
			spin_unlock(&ctx->q_data_lock);

			mtk_jpd_queue_res_chg_event(ctx);
			jpd_n_debug(LEVEL_ALWAYS, "set state to HEADER");
			ctx->state = MTK_JPD_STATE_HEADER;
		}

		v4l2_m2m_buf_queue(ctx->m2m_ctx, vvb);
	} else {
		plat_buf.pa = vb2_dma_contig_plane_dma_addr(vb, 0);
		plat_buf.size = vb2_plane_size(vb, 0);

		jpd_n_debug(LEVEL_CAPTURE, "FB(%d) vb=%p iova=%pad size=0x%zx",
			    vb->index, vb, &plat_buf.pa, plat_buf.size);
		ret = platform_jpd_insert_write_buf(ctx->platform_handle, plat_buf, vvb);
		if (unlikely(ret)) {
			jpd_n_err("FB(%d) platform_jpd_insert_write_buf failed(%d)", vb->index,
				  ret);
			v4l2_m2m_buf_done(vvb, VB2_BUF_STATE_ERROR);
			return;
		}
		vb2_set_plane_payload(vb, 0, vb2_plane_size(vb, 0));
	}
}

static int mtk_jpd_g_v_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_JPD_FRAME_METADATA_FD:
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mtk_jpd_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_jpd_ctx *ctx = ctrl_to_ctx(ctrl);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_JPD_FRAME_METADATA_FD:
		break;
	case V4L2_CID_JPD_ACQUIRE_RESOURCE:{
			if (ctx->active) {
				struct v4l2_jpd_resource_parameter *param = ctrl->p_new.p;

				jpd_n_debug(LEVEL_ALWAYS, "ACQUIRE_RESOURCE(%d)", param->priority);
				ret = platform_jpd_set_rank(ctx->platform_handle, param->priority);
			}
		}
		break;
	case V4L2_CID_JPD_OUTPUT_FORMAT:{
			if (ctx->active) {
				struct v4l2_jpd_output_format *param = ctrl->p_new.p;

				jpd_n_debug(LEVEL_ALWAYS, "OUTPUT_FORMAT(%d)", param->format);
				ret = platform_jpd_set_output_format(ctx->platform_handle,
					param->format);
			}
		}
		break;
	case V4L2_CID_JPD_VERIFICATION_MODE:{
			if (ctx->active) {
				struct v4l2_jpd_verification_mode *param = ctrl->p_new.p;

				jpd_n_debug(LEVEL_ALWAYS, "VERIFICATION_MODE(%d)", param->mode);
				ret = platform_jpd_set_verification_mode(ctx->platform_handle,
					param->mode);
			}
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mtk_jpd_probe(struct platform_device *pdev)
{
	struct mtk_jpd_dev *jpd_dev;
	struct resource *res;
	platform_jpd_initParam initParam;
	int ret;

	jpd_dev = devm_kzalloc(&pdev->dev, sizeof(*jpd_dev), GFP_KERNEL);
	if (unlikely(!jpd_dev)) {
		jpd_err("alloc mtk_jpd_dev failed");
		return -ENOMEM;
	}

	jpd_dev->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	jpd_dev->jpd_reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(jpd_dev->jpd_reg_base)) {
		ret = PTR_ERR(jpd_dev->jpd_reg_base);
		jpd_err("get jpd_reg_base failed(%d)", ret);
		return ret;
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	jpd_dev->jpd_ext_reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(jpd_dev->jpd_ext_reg_base)) {
		ret = PTR_ERR(jpd_dev->jpd_ext_reg_base);
		jpd_err("get jpd_ext_reg_base failed(%d)", ret);
		return ret;
	}

	jpd_dev->clk_njpd = devm_clk_get(&pdev->dev, "clk_njpd");
	if (IS_ERR(jpd_dev->clk_njpd)) {
		ret = PTR_ERR(jpd_dev->clk_njpd);
		jpd_err("get clk_njpd failed(%d)", ret);
		return ret;
	}
	jpd_dev->clk_smi2jpd = devm_clk_get(&pdev->dev, "clk_smi2jpd");
	if (IS_ERR(jpd_dev->clk_smi2jpd)) {
		ret = PTR_ERR(jpd_dev->clk_smi2jpd);
		jpd_err("get clk_smi2jpd failed(%d)", ret);
		return ret;
	}
	jpd_dev->clk_njpd2jpd = devm_clk_get(&pdev->dev, "clk_njpd2jpd");
	if (IS_ERR(jpd_dev->clk_njpd2jpd)) {
		ret = PTR_ERR(jpd_dev->clk_njpd2jpd);
		jpd_err("get clk_njpd2jpd failed(%d)", ret);
		return ret;
	}

	jpd_dev->jpd_irq = platform_get_irq(pdev, 0);
	if (unlikely(jpd_dev->jpd_irq < 0)) {
		jpd_err("get jpd_irq failed(%d)", jpd_dev->jpd_irq);
		return -EINVAL;
	}
	ret = devm_request_irq(&pdev->dev, jpd_dev->jpd_irq, platform_jpd_irq, 0,
			       pdev->name, jpd_dev);
	if (unlikely(ret)) {
		jpd_err("request jpd_irq(%d) failed(%d)", jpd_dev->jpd_irq, ret);
		return ret;
	}

	mutex_init(&jpd_dev->dev_mutex);

	ret = v4l2_device_register(&pdev->dev, &jpd_dev->v4l2_dev);
	if (unlikely(ret)) {
		jpd_err("v4l2_device_register failed(%d)", ret);
		return ret;
	}

	jpd_dev->m2m_dev = v4l2_m2m_init(&mtk_jpd_m2m_ops);
	if (IS_ERR(jpd_dev->m2m_dev)) {
		ret = PTR_ERR(jpd_dev->m2m_dev);
		jpd_err("v4l2_m2m_init failed(%d)", ret);
		goto err_m2m_init;
	}

	jpd_dev->video_dev = video_device_alloc();
	if (unlikely(!jpd_dev->video_dev)) {
		jpd_err("video_device_alloc failed");
		ret = -ENOMEM;
		goto err_dec_vdev_alloc;
	}
	snprintf(jpd_dev->video_dev->name, sizeof(jpd_dev->video_dev->name), "%s-dec",
		 MTK_JPD_NAME);
	jpd_dev->video_dev->fops = &mtk_jpd_file_fops;
	jpd_dev->video_dev->ioctl_ops = &mtk_jpd_ioctl_ops;
	jpd_dev->video_dev->release = video_device_release;
	jpd_dev->video_dev->lock = &jpd_dev->dev_mutex;
	jpd_dev->video_dev->v4l2_dev = &jpd_dev->v4l2_dev;
	jpd_dev->video_dev->vfl_dir = VFL_DIR_M2M;
	jpd_dev->video_dev->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	ret = video_register_device(jpd_dev->video_dev, VFL_TYPE_GRABBER, -1);
	if (unlikely(ret)) {
		video_device_release(jpd_dev->video_dev);
		jpd_err("video_register_device failed(%d)", ret);
		goto err_dec_vdev_register;
	}

	video_set_drvdata(jpd_dev->video_dev, jpd_dev);
	platform_set_drvdata(pdev, jpd_dev);

	initParam.dev = jpd_dev->dev;
	initParam.jpd_reg_base = jpd_dev->jpd_reg_base;
	initParam.jpd_ext_reg_base = jpd_dev->jpd_ext_reg_base;
	initParam.clk_njpd = jpd_dev->clk_njpd;
	initParam.clk_smi2jpd = jpd_dev->clk_smi2jpd;
	initParam.clk_njpd2jpd = jpd_dev->clk_njpd2jpd;
	initParam.jpd_irq = jpd_dev->jpd_irq;
	ret = platform_jpd_init(&initParam);
	if (unlikely(ret)) {
		jpd_err("platform_jpd_init failed(%d)", ret);
		goto err_platform_init;
	}

	jpd_debug(LEVEL_ALWAYS, "decoder device registered as /dev/video%d (%d,%d)",
		  jpd_dev->video_dev->num, VIDEO_MAJOR, jpd_dev->video_dev->minor);

	return 0;

err_platform_init:
	video_unregister_device(jpd_dev->video_dev);
err_dec_vdev_register:
err_dec_vdev_alloc:
	v4l2_m2m_release(jpd_dev->m2m_dev);
err_m2m_init:
	v4l2_device_unregister(&jpd_dev->v4l2_dev);

	return ret;
}

static int mtk_jpd_remove(struct platform_device *pdev)
{
	struct mtk_jpd_dev *jpd_dev = platform_get_drvdata(pdev);

	platform_jpd_deinit();

	if (jpd_dev->video_dev)
		video_unregister_device(jpd_dev->video_dev);

	if (jpd_dev->m2m_dev)
		v4l2_m2m_release(jpd_dev->m2m_dev);

	v4l2_device_unregister(&jpd_dev->v4l2_dev);

	return 0;
}

static const struct of_device_id mtk_jpd_match[] = {
	{
	 .compatible = "mediatek,mt5870-jpd",
	 .data = NULL,
	 },
	{
	 .compatible = "mediatek,merak-jpd",
	 .data = NULL,
	 },
	{},
};

MODULE_DEVICE_TABLE(of, mtk_jpd_match);

static struct platform_driver mtk_jpd_driver = {
	.probe = mtk_jpd_probe,
	.remove = mtk_jpd_remove,
	.driver = {
		   .name = MTK_JPD_NAME,
		   .of_match_table = mtk_jpd_match,
		   },
};

module_platform_driver(mtk_jpd_driver);

MODULE_DESCRIPTION("MediaTek JPD codec driver");
MODULE_LICENSE("GPL v2");
