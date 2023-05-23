// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
 *         Tiffany Lin <tiffany.lin@mediatek.com>
 */

#include <media/v4l2-event.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/delay.h>

#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_dec.h"
#include "mtk_vcodec_intr.h"
#include "mtk_vcodec_util.h"
#include "mtk_vcodec_dec_pm.h"
#include "vdec_drv_if.h"

#define MTK_VDEC_MIN_W  64U
#define MTK_VDEC_MIN_H  64U
#define DFT_CFG_WIDTH   MTK_VDEC_MIN_W
#define DFT_CFG_HEIGHT  MTK_VDEC_MIN_H

static struct mtk_video_fmt
	mtk_video_formats[MTK_MAX_DEC_CODECS_SUPPORT] = { {0} };
static struct mtk_codec_framesizes
	mtk_vdec_framesizes[MTK_MAX_DEC_CODECS_SUPPORT] = { {0} };
static unsigned int default_out_fmt_idx;
static unsigned int default_cap_fmt_idx;

#define NUM_SUPPORTED_FRAMESIZE ARRAY_SIZE(mtk_vdec_framesizes)
#define NUM_FORMATS ARRAY_SIZE(mtk_video_formats)
#ifdef CONFIG_VB2_MEDIATEK_DMA_CONTIG
static struct vb2_mem_ops vdec_ion_dma_contig_memops;
#endif

static void get_supported_format(struct mtk_vcodec_ctx *ctx)
{
	unsigned int i;

	if (mtk_video_formats[0].fourcc == 0) {
		if (vdec_if_get_param(ctx,
			GET_PARAM_CAPABILITY_SUPPORTED_FORMATS,
			&mtk_video_formats)  != 0) {
			mtk_v4l2_err("Error!! Cannot get supported format");
			return;
		}

		for (i = 0; i < MTK_MAX_DEC_CODECS_SUPPORT; i++) {
			if (mtk_video_formats[i].fourcc != 0)
				mtk_v4l2_debug(1,
				  "fmt[%d] fourcc %d type %d planes %d",
				  i, mtk_video_formats[i].fourcc,
				  mtk_video_formats[i].type,
				  mtk_video_formats[i].num_planes);
		}
		for (i = 0; i < MTK_MAX_DEC_CODECS_SUPPORT; i++) {
			if (mtk_video_formats[i].fourcc != 0 &&
				mtk_video_formats[i].type == MTK_FMT_DEC) {
				default_out_fmt_idx = i;
				break;
			}
		}
		for (i = 0; i < MTK_MAX_DEC_CODECS_SUPPORT; i++) {
			if (mtk_video_formats[i].fourcc != 0 &&
				mtk_video_formats[i].type == MTK_FMT_FRAME) {
				default_cap_fmt_idx = i;
				break;
			}
		}
	}
}

static struct mtk_video_fmt *mtk_vdec_find_format(struct mtk_vcodec_ctx *ctx,
	struct v4l2_format *f, unsigned int t)
{
	struct mtk_video_fmt *fmt;
	unsigned int k;

	mtk_v4l2_debug(3, "[%d] fourcc %d", ctx->id, f->fmt.pix_mp.pixelformat);
	for (k = 0; k < MTK_MAX_DEC_CODECS_SUPPORT &&
		 mtk_video_formats[k].fourcc != 0; k++) {
		fmt = &mtk_video_formats[k];
		if (fmt->fourcc == f->fmt.pix_mp.pixelformat &&
			mtk_video_formats[k].type == t)
			return fmt;
	}

	return NULL;
}

static bool is_mtk_supported_fmt(unsigned int pixelformat)
{
	struct mtk_video_fmt *fmt;
	unsigned int k;

	if (pixelformat == 0)
		return false;

	for (k = 0; k < NUM_FORMATS; k++) {
		fmt = &mtk_video_formats[k];
		if (fmt->fourcc == pixelformat)
			return true;
	}

	return false;
}

static struct mtk_video_fmt *mtk_find_fmt_by_pixel(unsigned int pixelformat)
{
	struct mtk_video_fmt *fmt;
	unsigned int k;

	for (k = 0; k < NUM_FORMATS; k++) {
		fmt = &mtk_video_formats[k];
		if (fmt->fourcc == pixelformat)
			return fmt;
	}
	mtk_v4l2_err("Error!! Cannot find fourcc: %d use default", pixelformat);

	return &mtk_video_formats[default_out_fmt_idx];
}

static struct mtk_q_data *mtk_vdec_get_q_data(struct mtk_vcodec_ctx *ctx,
	enum v4l2_buf_type type)
{
	if (ctx == NULL)
		return NULL;

	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->q_data[MTK_Q_DATA_SRC];

	return &ctx->q_data[MTK_Q_DATA_DST];
}

static bool mtk_vdec_is_ts_valid(u64 timestamp)
{
	struct timespec64 ts;
	u64 timestamp_us;

	ts = ns_to_timespec64(timestamp);
	timestamp_us = ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / NSEC_PER_USEC;

	return timestamp_us != MTK_INVALID_TIMESTAMP;
}

static void mtk_vdec_ts_reset(struct mtk_vcodec_ctx *ctx)
{
	struct mtk_detect_ts_param *param = &ctx->detect_ts_param;

	mutex_lock(&param->lock);
	if (ctx->dec_params.enable_detect_ts)
		param->mode = MTK_TS_MODE_DETECTING;
	else
		param->mode = MTK_TS_MODE_PTS;
	param->read_idx = 0;
	param->recorded_size = 0;
	param->first_disp_ts = MTK_INVALID_TIMESTAMP;
	mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] reset to mode %d", ctx->id, param->mode);
	mutex_unlock(&param->lock);
}

static void mtk_vdec_ts_insert(struct mtk_vcodec_ctx *ctx, u64 timestamp, bool is_csd)
{
	struct mtk_detect_ts_param *param = &ctx->detect_ts_param;
	int write_idx;
	struct mtk_q_data *q_data;

	q_data = &ctx->q_data[MTK_Q_DATA_SRC];

	mutex_lock(&param->lock);
	if (param->mode == MTK_TS_MODE_PTS) {
		mutex_unlock(&param->lock);
		return;
	}

	if (param->recorded_size && !is_csd) {
		int last_write_idx;
		u64 last_timestamp;

		last_write_idx = (param->read_idx + param->recorded_size - 1) % VB2_MAX_FRAME;
		last_timestamp = param->record[last_write_idx];
		if ((s64)last_timestamp > (s64)timestamp) {
			mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] decreasing ts %lld -> %lld, choose pts mode",
				ctx->id, last_timestamp, timestamp);
			param->mode = MTK_TS_MODE_PTS;
			mutex_unlock(&param->lock);
			return;
		}
		if (q_data) {
			if (q_data->fmt->fourcc == V4L2_PIX_FMT_AV1 &&
				(s64)last_timestamp == (s64)timestamp) {
				mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] duplicated ts %lld for AV1 super frame, do not record ts",
					ctx->id, timestamp);
				mutex_unlock(&param->lock);
				return;
			}
		}
	}

	write_idx = (param->read_idx + param->recorded_size) % VB2_MAX_FRAME;
	param->record[write_idx] = timestamp;
	param->recorded_size++;
	mtk_v4l2_debug(VCODEC_DBG_L2, "record ts %lld at index %d size %d is_csd %d",
		timestamp, write_idx, param->recorded_size, is_csd);
	if (param->recorded_size > VB2_MAX_FRAME) {
		mtk_v4l2_err("[%d] ts record size is too large", ctx->id);
		param->recorded_size = VB2_MAX_FRAME;
	}
	mutex_unlock(&param->lock);
}

static void mtk_vdec_ts_remove_last(struct mtk_vcodec_ctx *ctx)
{
	struct mtk_detect_ts_param *param = &ctx->detect_ts_param;
	int remove_idx;

	mutex_lock(&param->lock);
	if (param->mode == MTK_TS_MODE_PTS) {
		mutex_unlock(&param->lock);
		return;
	}

	// in case bitstream is skipped before the first src_chg
	if (param->recorded_size == 0) {
		mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] skip due to ts record size is 0", ctx->id);
		mutex_unlock(&param->lock);
		return;
	}

	remove_idx = (param->read_idx + param->recorded_size - 1) % VB2_MAX_FRAME;
	mtk_v4l2_debug(VCODEC_DBG_L2, "remove last ts %lld at index %d size %d",
		param->record[remove_idx], remove_idx, param->recorded_size);
	param->recorded_size--;
	mutex_unlock(&param->lock);
}

static u64 mtk_vdec_ts_update_mode_and_timestamp(struct mtk_vcodec_ctx *ctx, u64 pts)
{
	struct mtk_detect_ts_param *param = &ctx->detect_ts_param;
	u64 dts, timestamp;

	mutex_lock(&param->lock);

	if (param->mode == MTK_TS_MODE_PTS) {
		mutex_unlock(&param->lock);
		return pts;
	}

	/* DTV case, we may have invalid output timestamp even if all valid input timestamp */
	if (!mtk_vdec_is_ts_valid(pts)) {
		mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] got invalid pts, choose pts mode", ctx->id);
		param->mode = MTK_TS_MODE_PTS;
		mutex_unlock(&param->lock);
		return pts;
	}

	if (param->recorded_size == 0) {
		mtk_v4l2_err("[%d] ts record size is 0", ctx->id);
		mutex_unlock(&param->lock);
		return pts;
	}

	dts = param->record[param->read_idx];
	param->read_idx++;
	if (param->read_idx == VB2_MAX_FRAME)
		param->read_idx = 0;

	param->recorded_size--;

	do {
		if (param->mode != MTK_TS_MODE_DETECTING)
			break;

		if (!mtk_vdec_is_ts_valid(param->first_disp_ts)) {
			WARN_ON(!mtk_vdec_is_ts_valid(pts));
			param->first_disp_ts = pts;
			/* for DTS mode, there should be no reorder, so the first disp frame
			 * must be the first decode frame which means pts == dts
			 */
			if (pts != dts) {
				param->mode = MTK_TS_MODE_PTS;
				mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] first pts %lld != dts %lld. choose pts mode",
					ctx->id, pts, dts);
				break;
			}
		}

		if ((s64)pts < (s64)dts) {
			param->mode = MTK_TS_MODE_PTS;
			mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] pts %lld < dts %lld. choose pts mode",
				ctx->id, pts, dts);
		} else if ((s64)dts < (s64)pts) {
			param->mode = MTK_TS_MODE_DTS;
			mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] dts %lld < pts %lld. choose dts mode",
				ctx->id, dts, pts);
		}
	} while (0);

	if (param->mode == MTK_TS_MODE_DTS) {
		timestamp = dts;
		mtk_v4l2_debug(VCODEC_DBG_L2, "use dts %lld instead of pts %lld", dts, pts);
	} else
		timestamp = pts;

	mutex_unlock(&param->lock);

	return timestamp;
}

/*
 * This function tries to clean all display buffers, the buffers will return
 * in display order.
 * Note the buffers returned from codec driver may still be in driver's
 * reference list.
 */
static struct vb2_buffer *get_display_buffer(struct mtk_vcodec_ctx *ctx,
	bool got_early_eos)
{
	struct vdec_fb *disp_frame_buffer = NULL;
	struct mtk_video_dec_buf *dstbuf;
	unsigned int i = 0;
	unsigned int num_planes = 0;

	mtk_v4l2_debug(4, "[%d]", ctx->id);
	if (vdec_if_get_param(ctx,
						  GET_PARAM_DISP_FRAME_BUFFER,
						  &disp_frame_buffer)) {
		mtk_v4l2_err("[%d]Cannot get param : GET_PARAM_DISP_FRAME_BUFFER",
					 ctx->id);
		return NULL;
	}

	if (disp_frame_buffer == NULL) {
		mtk_v4l2_debug(4, "No display frame buffer");
		return NULL;
	}
	if (!virt_addr_valid(disp_frame_buffer)) {
		mtk_v4l2_debug(3, "Bad display frame buffer %p",
			disp_frame_buffer);
		return NULL;
	}

	dstbuf = container_of(disp_frame_buffer, struct mtk_video_dec_buf,
						  frame_buffer);
	num_planes = dstbuf->vb.vb2_buf.num_planes;
	mutex_lock(&ctx->buf_lock);
	if (dstbuf->used) {
		for (i = 0; i < num_planes; i++) {
			vb2_set_plane_payload(&dstbuf->vb.vb2_buf, i,
				ctx->picinfo.fb_sz[i]);
		}

		dstbuf->ready_to_display = true;

		switch (dstbuf->frame_buffer.frame_type) {
		case MTK_FRAME_I:
			dstbuf->vb.flags |= V4L2_BUF_FLAG_KEYFRAME;
			break;
		case MTK_FRAME_P:
			dstbuf->vb.flags |= V4L2_BUF_FLAG_PFRAME;
			break;
		case MTK_FRAME_B:
			dstbuf->vb.flags |= V4L2_BUF_FLAG_BFRAME;
			break;
		default:
			mtk_v4l2_debug(2, "[%d] unknown frame type %d",
				ctx->id, dstbuf->frame_buffer.frame_type);
			break;
		}

		if (got_early_eos &&
			dstbuf->vb.vb2_buf.timestamp == ctx->input_max_ts)
			dstbuf->vb.flags |= V4L2_BUF_FLAG_LAST;

		dstbuf->vb.vb2_buf.timestamp =
			disp_frame_buffer->timestamp;

		dstbuf->vb.vb2_buf.timestamp =
			mtk_vdec_ts_update_mode_and_timestamp(ctx, dstbuf->vb.vb2_buf.timestamp);
		dstbuf->vb.field = disp_frame_buffer->field;

		mtk_v4l2_debug(2,
			"[%d]status=%x queue id=%d to done_list %d %d flag=%x field %d pts=%llu drop %d",
			ctx->id, disp_frame_buffer->status,
			dstbuf->vb.vb2_buf.index,
			dstbuf->queued_in_vb2, got_early_eos,
			dstbuf->vb.flags, dstbuf->vb.field,
			dstbuf->vb.vb2_buf.timestamp,
			disp_frame_buffer->drop);

		if (disp_frame_buffer->drop)
			v4l2_m2m_buf_done(&dstbuf->vb, VB2_BUF_STATE_ERROR);
		else
			v4l2_m2m_buf_done(&dstbuf->vb, VB2_BUF_STATE_DONE);
		ctx->decoded_frame_cnt++;
	}
	mutex_unlock(&ctx->buf_lock);
	return &dstbuf->vb.vb2_buf;
}

/*
 * This function tries to clean all capture buffers that are not used as
 * reference buffers by codec driver any more
 * In this case, we need re-queue buffer to vb2 buffer if user space
 * already returns this buffer to v4l2 or this buffer is just the output of
 * previous sps/pps/resolution change decode, or do nothing if user
 * space still owns this buffer
 */
static struct vb2_buffer *get_free_buffer(struct mtk_vcodec_ctx *ctx)
{
	struct mtk_video_dec_buf *dstbuf;
	struct vdec_fb *free_frame_buffer = NULL;

	if (vdec_if_get_param(ctx,
						  GET_PARAM_FREE_FRAME_BUFFER,
						  &free_frame_buffer)) {
		mtk_v4l2_err("[%d] Error!! Cannot get param", ctx->id);
		return NULL;
	}
	if (free_frame_buffer == NULL) {
		mtk_v4l2_debug(4, " No free frame buffer");
		return NULL;
	}
	if (!virt_addr_valid(free_frame_buffer)) {
		mtk_v4l2_debug(3, "Bad free frame buffer %p",
			free_frame_buffer);
		return NULL;
	}
	mtk_v4l2_debug(4, "[%d] tmp_frame_addr = 0x%p",
				   ctx->id, free_frame_buffer);

	dstbuf = container_of(free_frame_buffer, struct mtk_video_dec_buf,
						  frame_buffer);
	dstbuf->flags |= REF_FREED;

	mutex_lock(&ctx->buf_lock);
	if (dstbuf->used) {
		if ((dstbuf->queued_in_vb2) &&
			(dstbuf->queued_in_v4l2) &&
			(free_frame_buffer->status == FB_ST_FREE)) {
			/*
			 * After decode sps/pps or non-display buffer, we don't
			 * need to return capture buffer to user space, but
			 * just re-queue this capture buffer to vb2 queue.
			 * This reduce overheads that dq/q unused capture
			 * buffer. In this case, queued_in_vb2 = true.
			 */
			mtk_v4l2_debug(2,
				"[%d]status=%x queue id=%d to rdy_queue %d",
				ctx->id, free_frame_buffer->status,
				dstbuf->vb.vb2_buf.index,
				dstbuf->queued_in_vb2);
			v4l2_m2m_buf_queue(ctx->m2m_ctx, &dstbuf->vb);
		} else if ((dstbuf->queued_in_vb2 == false) &&
				   (dstbuf->queued_in_v4l2 == true)) {
			/*
			 * If buffer in v4l2 driver but not in vb2 queue yet,
			 * and we get this buffer from free_list, it means
			 * that codec driver do not use this buffer as
			 * reference buffer anymore. We should q buffer to vb2
			 * queue, so later work thread could get this buffer
			 * for decode. In this case, queued_in_vb2 = false
			 * means this buffer is not from previous decode
			 * output.
			 */
			mtk_v4l2_debug(2,
				"[%d]status=%x queue id=%d to rdy_queue",
				ctx->id, free_frame_buffer->status,
				dstbuf->vb.vb2_buf.index);
			v4l2_m2m_buf_queue(ctx->m2m_ctx, &dstbuf->vb);
			dstbuf->queued_in_vb2 = true;
		} else {
			/*
			 * Codec driver do not need to reference this capture
			 * buffer and this buffer is not in v4l2 driver.
			 * Then we don't need to do any thing, just add log when
			 * we need to debug buffer flow.
			 * When this buffer q from user space, it could
			 * directly q to vb2 buffer
			 */
			mtk_v4l2_debug(4, "[%d]status=%x err queue id=%d %d %d",
				ctx->id, free_frame_buffer->status,
				dstbuf->vb.vb2_buf.index,
				dstbuf->queued_in_vb2,
				dstbuf->queued_in_v4l2);
		}
		dstbuf->used = false;
	}
	mutex_unlock(&ctx->buf_lock);
	return &dstbuf->vb.vb2_buf;
}

static struct vb2_buffer *get_free_bs_buffer(struct mtk_vcodec_ctx *ctx,
	struct mtk_vcodec_mem *current_bs)
{
	struct mtk_vcodec_mem *free_bs_buffer;
	struct mtk_video_dec_buf *srcbuf;

	if (vdec_if_get_param(ctx, GET_PARAM_FREE_BITSTREAM_BUFFER,
						  &free_bs_buffer) != 0) {
		mtk_v4l2_err("[%d] Cannot get param : GET_PARAM_FREE_BITSTREAM_BUFFER",
					 ctx->id);
		return NULL;
	}

	if (free_bs_buffer == NULL) {
		mtk_v4l2_debug(3, "No free bitstream buffer");
		return NULL;
	}
	if (current_bs == free_bs_buffer) {
		mtk_v4l2_debug(4,
			"No free bitstream buffer except current bs: %p",
			current_bs);
		return NULL;
	}
	if (!virt_addr_valid(free_bs_buffer)) {
		mtk_v4l2_debug(3, "Bad free bitstream buffer %p",
			free_bs_buffer);
		return NULL;
	}

	srcbuf = container_of(free_bs_buffer,
		struct mtk_video_dec_buf, bs_buffer);
	mtk_v4l2_debug(2,
		"[%d] length=%zu size=%zu queue idx=%d to done_list %d gen_frame %d",
		ctx->id, free_bs_buffer->length, free_bs_buffer->size,
		srcbuf->vb.vb2_buf.index,
		srcbuf->queued_in_vb2,
		free_bs_buffer->gen_frame);

	if (free_bs_buffer->gen_frame)
		v4l2_m2m_buf_done(&srcbuf->vb, VB2_BUF_STATE_DONE);
	else {
		mtk_vdec_ts_remove_last(ctx);
		v4l2_m2m_buf_done(&srcbuf->vb, VB2_BUF_STATE_ERROR);
	}
	return &srcbuf->vb.vb2_buf;
}

static void clean_free_bs_buffer(struct mtk_vcodec_ctx *ctx,
	struct mtk_vcodec_mem *current_bs)
{
	struct vb2_buffer *framptr;

	do {
		framptr = get_free_bs_buffer(ctx, current_bs);
	} while (framptr);
}


static void clean_display_buffer(struct mtk_vcodec_ctx *ctx, bool got_early_eos)
{
	struct vb2_buffer *framptr;

	do {
		framptr = get_display_buffer(ctx, got_early_eos);
	} while (framptr);
}

static void clean_free_fm_buffer(struct mtk_vcodec_ctx *ctx)
{
	struct vb2_buffer *framptr;

	do {
		framptr = get_free_buffer(ctx);
	} while (framptr);
}

static void mtk_vdec_queue_res_chg_event(struct mtk_vcodec_ctx *ctx)
{
	static const struct v4l2_event ev_src_ch = {
		.type = V4L2_EVENT_SOURCE_CHANGE,
		.u.src_change.changes =
		V4L2_EVENT_SRC_CH_RESOLUTION,
	};

	mtk_v4l2_debug(1, "[%d]", ctx->id);
	v4l2_event_queue_fh(&ctx->fh, &ev_src_ch);

	v4l2_m2m_set_dst_buffered(ctx->m2m_ctx,
		ctx->input_driven);
}

static void mtk_vdec_queue_stop_play_event(struct mtk_vcodec_ctx *ctx)
{
	static const struct v4l2_event ev_eos = {
		.type = V4L2_EVENT_EOS,
	};

	mtk_v4l2_debug(1, "[%d]", ctx->id);
	v4l2_event_queue_fh(&ctx->fh, &ev_eos);
}

static void mtk_vdec_queue_noseqheader_event(struct mtk_vcodec_ctx *ctx)
{
	static const struct v4l2_event ev_eos = {
		.type = V4L2_EVENT_MTK_VDEC_NOHEADER,
	};

	mtk_v4l2_debug(1, "[%d]", ctx->id);
	v4l2_event_queue_fh(&ctx->fh, &ev_eos);
}

static void mtk_vdec_queue_error_event(struct mtk_vcodec_ctx *ctx)
{
	static const struct v4l2_event ev_error = {
		.type = V4L2_EVENT_MTK_VDEC_ERROR,
	};

	mtk_v4l2_debug(1, "[%d]", ctx->id);
	v4l2_event_queue_fh(&ctx->fh, &ev_error);
}

static void mtk_vdec_queue_unsupport_bs_event(struct mtk_vcodec_ctx *ctx)
{
	static const struct v4l2_event ev_error = {
		.type = V4L2_EVENT_MTK_VDEC_BITSTREAM_UNSUPPORT,
	};

	mtk_v4l2_debug(1, "[%d]", ctx->id);
	v4l2_event_queue_fh(&ctx->fh, &ev_error);
}

static void mtk_vdec_queue_framerate_chg_event(struct mtk_vcodec_ctx *ctx)
{
	static const struct v4l2_event ev_src_ch = {
		.type = V4L2_EVENT_SOURCE_CHANGE,
		.u.src_change.changes =
		V4L2_EVENT_SRC_CH_FRAMERATE,
	};

	mtk_v4l2_debug(1, "[%d]", ctx->id);
	v4l2_event_queue_fh(&ctx->fh, &ev_src_ch);
}

static void mtk_vdec_queue_scantype_chg_event(struct mtk_vcodec_ctx *ctx)
{
	static const struct v4l2_event ev_src_ch = {
		.type = V4L2_EVENT_SOURCE_CHANGE,
		.u.src_change.changes =
		V4L2_EVENT_SRC_CH_SCANTYPE,
	};

	mtk_v4l2_debug(1, "[%d]", ctx->id);
	v4l2_event_queue_fh(&ctx->fh, &ev_src_ch);
}

static void mtk_vdec_drain_decoder(struct mtk_vcodec_ctx *ctx)
{
	int ret = 0;

	ret = vdec_if_flush(ctx, FLUSH_FRAME);
	if (ret) {
		mtk_v4l2_err("DecodeFinal failed, ret=%d", ret);
		return;
	}

	clean_free_bs_buffer(ctx, NULL);
	clean_display_buffer(ctx, 0);
	clean_free_fm_buffer(ctx);
}

static void mtk_vdec_flush_decoder(struct mtk_vcodec_ctx *ctx,
		enum v4l2_buf_type type)
{
	int ret = 0;

	ctx->state = MTK_STATE_FLUSH;
	if (ctx->input_driven)
		wake_up(&ctx->fm_wq);

	ret = vdec_if_flush(ctx,
		V4L2_TYPE_IS_OUTPUT(type) ? FLUSH_BITSTREAM : FLUSH_FRAME);
	if (ret) {
		mtk_v4l2_err("DecodeFinal failed, ret=%d", ret);
		return;
	}

	clean_free_bs_buffer(ctx, NULL);
	clean_display_buffer(ctx, 0);
	clean_free_fm_buffer(ctx);
}

static enum mtk_vdec_hw_id mtk_vdec_get_avail_hw_id_locked(struct mtk_vcodec_ctx *ctx)
{
	struct mtk_vcodec_dev *dev = ctx->dev;
	enum mtk_vdec_hw_id i, target_hw_id = MTK_VDEC_HW_NUM;
	enum vdec_priority priority = ctx->dec_res_info.priority;

	for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++) {
		bool available = true;
		enum vdec_priority p;

		for (p = VDEC_PRIORITY_REAL_TIME; p < priority; p++) {
			if (dev->usage_counter[i][p]) {
				available = false;
				break;
			}
		}
		if (available) {
			target_hw_id = i;
			break;
		}
	}

	return target_hw_id;
}

static bool mtk_vdec_check_workqueue_for_non_real_time_locked(struct mtk_vcodec_ctx *ctx)
{
	int i;
	struct mtk_vcodec_dev *dev = ctx->dev;

	/* CPU resource (work queue) is shared among all playbacks
	 * and real-time task should be served before non-real-time
	 */
	for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++) {
		if (dev->usage_counter[i][VDEC_PRIORITY_REAL_TIME])
			return false;
	}

	return true;
}

static bool mtk_vdec_check_core_for_non_real_time_locked(struct mtk_vcodec_ctx *ctx)
{
	int i;
	struct mtk_vcodec_dev *dev = ctx->dev;
	enum vdec_priority priority = ctx->dec_res_info.priority;

	for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++) {
		enum vdec_priority p;

		if (!ctx->dec_res_info.hw_used[i])
			continue;
		for (p = VDEC_PRIORITY_REAL_TIME; p < priority; p++) {
			/* don't schedule non-real-time task
			 * if tasks with higher priority want to decode
			 */
			if (dev->usage_counter[i][p])
				return false;
		}
	}

	return true;
}

static bool mtk_vdec_check_scheduling(struct mtk_vcodec_ctx *ctx)
{
	int i;
	struct mtk_vcodec_dev *dev = ctx->dev;
	unsigned long flags;
	enum vdec_priority priority = ctx->dec_res_info.priority;
	bool scheduling = true;

	if (priority == VDEC_PRIORITY_REAL_TIME)
		return true;

	if (ctx->runtime_decide_hw) {
		bool not_decided = true;

		for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++) {
			// all true hw_used[i] means not decided yet
			if (!ctx->dec_res_info.hw_used[i]) {
				not_decided = false;
				break;
			}
		}

		if (not_decided)
			return false;
	}

	/* check scheduling for non-real-time task */
	spin_lock_irqsave(&dev->dec_counter_lock, flags);

	scheduling = mtk_vdec_check_workqueue_for_non_real_time_locked(ctx);
	if (scheduling)
		scheduling = mtk_vdec_check_core_for_non_real_time_locked(ctx);

	spin_unlock_irqrestore(&dev->dec_counter_lock, flags);
	return scheduling;
}

static void mtk_vdec_res_update_counter(struct mtk_vcodec_ctx *ctx, bool inc)
{
#ifdef TV_INTEGRATION
	int i;
	struct mtk_vcodec_dev *dev = ctx->dev;
	unsigned long flags;
	enum vdec_priority priority = ctx->dec_res_info.priority;

	if (priority < VDEC_PRIORITY_REAL_TIME || priority >= VDEC_PRIORITY_NUM) {
		WARN(1, "invalid priority %d\n", priority);
		return;
	}

	spin_lock_irqsave(&dev->dec_counter_lock, flags);
	if ((inc && ctx->dec_res_in_counter) || (!inc && !ctx->dec_res_in_counter)) {
		// should not inc until this job is handled or cancelled
		WARN_ON(inc && ctx->dec_res_in_counter);
		spin_unlock_irqrestore(&dev->dec_counter_lock, flags);
		return;
	}

	if (ctx->runtime_decide_hw && inc) {
		unsigned long hw_id = MTK_VDEC_HW_NUM;

		// we have the chance to use either core0/1, mark hw_used for them
		for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++)
			ctx->dec_res_info.hw_used[i] = true;

		hw_id = mtk_vdec_get_avail_hw_id_locked(ctx);
		if (hw_id != MTK_VDEC_HW_NUM) {
			for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++) {
				if (i == hw_id)
					continue;

				ctx->dec_res_info.hw_used[i] = false;
			}
			mtk_v4l2_debug(4, "[%d] update hw id to %ld", ctx->id, hw_id);
			vdec_if_set_param(ctx, SET_PARAM_DEC_HW_ID, &hw_id);
		} else
			mtk_v4l2_debug(4, "[%d] no available hw id", ctx->id);
	}
	for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++) {
		if (!ctx->dec_res_info.hw_used[i])
			continue;

		if (inc)
			dev->usage_counter[i][priority]++;
		else {
			WARN_ON(dev->usage_counter[i][priority] <= 0);
			dev->usage_counter[i][priority]--;
		}
	}
	ctx->dec_res_in_counter = inc;
	mtk_v4l2_debug(4, "[%d] after %s counter %d %d for priority %d",
			ctx->id,
			inc ? "inc" : "dec",
			dev->usage_counter[MTK_VDEC_CORE_0][priority],
			dev->usage_counter[MTK_VDEC_CORE_1][priority],
			priority);
	spin_unlock_irqrestore(&dev->dec_counter_lock, flags);
#endif
}

static bool mtk_vdec_need_decide_hw(struct mtk_vcodec_ctx *ctx, enum vdec_priority priority)
{
	int i;

	if (priority == VDEC_PRIORITY_REAL_TIME)
		return false;

	// return true if all hw_used are not set
	for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++) {
		if (ctx->dec_res_info.hw_used[i])
			return false;
	}

	return true;
}

static void mtk_vdec_res_info_update(struct mtk_vcodec_ctx *ctx)
{
#ifdef TV_INTEGRATION
	if (vdec_if_get_param(ctx, GET_PARAM_RES_INFO, &ctx->dec_res_info)) {
		mtk_v4l2_err(
			"[%d]Error!! Cannot get param : GET_PARAM_RES_INFO ERR",
			ctx->id);
		return;
	}
	ctx->runtime_decide_hw = mtk_vdec_need_decide_hw(ctx, ctx->dec_res_info.priority);

	mtk_v4l2_debug(4, "[%d] res in use %d %d usage %d gce %d priority %d runtime %d",
			ctx->id,
			ctx->dec_res_info.hw_used[MTK_VDEC_CORE_0],
			ctx->dec_res_info.hw_used[MTK_VDEC_CORE_1],
			ctx->dec_res_info.usage,
			ctx->dec_res_info.gce,
			ctx->dec_res_info.priority,
			ctx->runtime_decide_hw);
#endif
}

static void mtk_vdec_pic_info_update(struct mtk_vcodec_ctx *ctx)
{
	unsigned int dpbsize = 0;
	int ret;
	struct mtk_color_desc color_desc;

	if (vdec_if_get_param(ctx, GET_PARAM_PIC_INFO,
			      &ctx->last_decoded_picinfo)) {
		mtk_v4l2_err(
			"[%d]Error!! Cannot get param : GET_PARAM_PICTURE_INFO ERR",
			ctx->id);
		return;
	}

	if (ctx->last_decoded_picinfo.pic_w == 0 ||
	    ctx->last_decoded_picinfo.pic_h == 0 ||
	    ctx->last_decoded_picinfo.buf_w == 0 ||
	    ctx->last_decoded_picinfo.buf_h == 0) {
		mtk_v4l2_err("Cannot get correct pic info");
		return;
	}

	ret = vdec_if_get_param(ctx, GET_PARAM_DPB_SIZE, &dpbsize);
	if (dpbsize == 0)
		mtk_v4l2_err("Incorrect dpb size, ret=%d", ret);
	ctx->last_dpb_size = dpbsize;

	ret = vdec_if_get_param(ctx, GET_PARAM_COLOR_DESC, &color_desc);
	if (ret == 0)
		ctx->last_is_hdr = color_desc.is_hdr;

	mtk_v4l2_debug(
		1,
		"[%d]-> new(%d,%d),dpb(%d), old(%d,%d),dpb(%d), bit(%d) real(%d,%d) hdr(%d,%d)",
		ctx->id, ctx->last_decoded_picinfo.pic_w,
		ctx->last_decoded_picinfo.pic_h, ctx->last_dpb_size,
		ctx->picinfo.pic_w, ctx->picinfo.pic_h, ctx->dpb_size,
		ctx->picinfo.bitdepth, ctx->last_decoded_picinfo.buf_w,
		ctx->last_decoded_picinfo.buf_h, ctx->is_hdr, ctx->last_is_hdr);
}

int mtk_vdec_put_fb(struct mtk_vcodec_ctx *ctx, enum mtk_put_buffer_type type)
{
	struct mtk_video_dec_buf *dst_buf_info, *src_buf_info;
	struct vb2_v4l2_buffer *src_buf_v4l2, *dst_buf_v4l2;
	struct vdec_fb *pfb;
	int i, ret = 0;

	mtk_v4l2_debug(1, "[%d] type = %d", ctx->id, type);
	src_buf_v4l2 = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	if (src_buf_v4l2 == NULL) {
		if (type == PUT_BUFFER_WORKER)
			return 0;
		src_buf_info = NULL;
	} else
		src_buf_info = container_of(src_buf_v4l2, struct mtk_video_dec_buf, vb);

	if (type == PUT_BUFFER_WORKER && src_buf_info->lastframe == EOS) {
		clean_display_buffer(ctx, src_buf_v4l2->vb2_buf.planes[0].bytesused != 0U);
		clean_free_fm_buffer(ctx);

		if (src_buf_v4l2->vb2_buf.planes[0].bytesused == 0U) {
			src_buf_v4l2->flags |= V4L2_BUF_FLAG_LAST;
			vb2_set_plane_payload(&src_buf_info->vb.vb2_buf, 0, 0);
			if (src_buf_info != ctx->dec_flush_buf)
				v4l2_m2m_buf_done(&src_buf_info->vb,
					VB2_BUF_STATE_DONE);

			if (ctx->input_driven)
				ret = wait_event_interruptible(
					ctx->fm_wq,
					v4l2_m2m_num_dst_bufs_ready(
					ctx->m2m_ctx) > 0 ||
					flush_abort_state(ctx->state));

			/* update dst buf status */
			dst_buf_v4l2 = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
			if (flush_abort_state(ctx->state)
				|| ret != 0 || dst_buf_v4l2 == NULL) {
				mtk_v4l2_debug(
					0,
					"[%d] wait EOS dst break!state %d, ret %d, dst_buf %p",
					ctx->id, ctx->state, ret, dst_buf_v4l2);
				return 0;
			}

			dst_buf_info = container_of(dst_buf_v4l2,
				struct mtk_video_dec_buf, vb);

			dst_buf_info->used = false;
			dst_buf_v4l2->flags |= V4L2_BUF_FLAG_LAST;
			pfb = &dst_buf_info->frame_buffer;
			for (i = 0; i < pfb->num_planes; i++)
				vb2_set_plane_payload(&dst_buf_info->vb.vb2_buf,
					i, 0);
			v4l2_m2m_buf_done(&dst_buf_info->vb,
				VB2_BUF_STATE_DONE);
		}

		mtk_vdec_queue_stop_play_event(ctx);
	} else {
		if (!ctx->input_driven)
			dst_buf_v4l2 = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		clean_display_buffer(ctx,
			type == PUT_BUFFER_WORKER &&
			src_buf_info->lastframe == EOS_WITH_DATA);
		clean_free_fm_buffer(ctx);
	}

	if (ctx->input_driven)
		v4l2_m2m_try_schedule(ctx->m2m_ctx);

	return 0;
}

static void mtk_vdec_worker(struct work_struct *work)
{
	struct mtk_vcodec_ctx *ctx = container_of(work, struct mtk_vcodec_ctx,
		decode_work);
	struct mtk_vcodec_dev *dev = ctx->dev;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	struct mtk_vcodec_mem *buf;
	struct vdec_fb *pfb = NULL;
	unsigned int src_chg = 0;
	bool res_chg = false;
	bool need_more_output = false;
	bool mtk_vcodec_unsupport = false;
	bool mtk_bs_unsupport = false;
	bool framerate_chg = false;
	bool scantype_chg = false;
	bool is_csd = false;
	int ret;
	unsigned int i = 0;
	unsigned int num_planes;
	struct timespec64 worktvstart;
	struct timespec64 worktvstart1;
	struct timespec64 vputvend;
	struct mtk_video_dec_buf *dst_buf_info = NULL, *src_buf_info = NULL;
	unsigned int dpbsize = 0;
	unsigned int field = V4L2_FIELD_NONE;
	struct mtk_color_desc color_desc;

	mutex_lock(&ctx->worker_lock);
	if (ctx->state != MTK_STATE_HEADER) {
		mtk_vdec_res_update_counter(ctx, false);
		v4l2_m2m_job_finish(dev->m2m_dev_dec, ctx->m2m_ctx);
		mtk_v4l2_debug(1, " %d", ctx->state);
		mutex_unlock(&ctx->worker_lock);
		return;
	}

	ktime_get_real_ts64(&worktvstart);
	src_buf = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	if (src_buf == NULL) {
		mtk_vdec_res_update_counter(ctx, false);
		v4l2_m2m_job_finish(dev->m2m_dev_dec, ctx->m2m_ctx);
		mtk_v4l2_debug(1, "[%d] src_buf empty!!", ctx->id);
		mutex_unlock(&ctx->worker_lock);
		return;
	}

	dst_buf = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (dst_buf == NULL && !ctx->input_driven) {
		mtk_vdec_res_update_counter(ctx, false);
		v4l2_m2m_job_finish(dev->m2m_dev_dec, ctx->m2m_ctx);
		mtk_v4l2_debug(1, "[%d] dst_buf empty!!", ctx->id);
		mutex_unlock(&ctx->worker_lock);
		return;
	}

	src_buf_info = container_of(src_buf, struct mtk_video_dec_buf, vb);

	if (!ctx->input_driven) {
		dst_buf_info = container_of(dst_buf,
			struct mtk_video_dec_buf, vb);

		pfb = &dst_buf_info->frame_buffer;
		num_planes = dst_buf->vb2_buf.num_planes;
		pfb->num_planes = num_planes;
		pfb->index = dst_buf->vb2_buf.index;

		for (i = 0; i < num_planes; i++) {
#ifdef TV_INTEGRATION
			pfb->fb_base[i].va = 0; // to speed up
#else
			pfb->fb_base[i].va = vb2_plane_vaddr(&dst_buf->vb2_buf, i);
#endif
#ifdef CONFIG_VB2_MEDIATEK_DMA_SG
			pfb->fb_base[i].dma_addr =
				mtk_dma_sg_plane_dma_addr(&dst_buf->vb2_buf, i);
#else
			pfb->fb_base[i].dma_addr =
				vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, i);
#endif
			pfb->fb_base[i].size = ctx->picinfo.fb_sz[i];
			pfb->fb_base[i].length = dst_buf->vb2_buf.planes[i].length;
			pfb->fb_base[i].dmabuf = dst_buf->vb2_buf.planes[i].dbuf;
		}
		pfb->status = 0;
		mtk_vcodec_init_slice_info(ctx, dst_buf_info);

		mtk_v4l2_debug(1,
				"id=%d Framebuf  pfb=%p VA=%p Y_DMA=%pad C_DMA=%pad ion_buffer=(%p %p) Size=%zx, general_buf_fd = %d",
			dst_buf->vb2_buf.index, pfb,
			pfb->fb_base[0].va, &pfb->fb_base[0].dma_addr,
			&pfb->fb_base[1].dma_addr, pfb->fb_base[0].dmabuf,
			pfb->fb_base[1].dmabuf, pfb->fb_base[0].size,
			dst_buf_info->general_buf_fd);
	}

	mtk_v4l2_debug(4, "===>[%d] vdec_if_decode() ===>", ctx->id);


	if (src_buf_info->lastframe == EOS) {
		mtk_v4l2_debug(4, "===>[%d] vdec_if_decode() EOS ===> %d %d",
			ctx->id, src_buf->vb2_buf.planes[0].bytesused,
			src_buf->vb2_buf.planes[0].length);

		vdec_if_flush(ctx, FLUSH_BITSTREAM | FLUSH_FRAME);

		mtk_vdec_put_fb(ctx, PUT_BUFFER_WORKER);
		v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		src_buf_info->lastframe = NON_EOS;
		clean_free_bs_buffer(ctx, NULL);

		mtk_vdec_res_update_counter(ctx, false);
		v4l2_m2m_job_finish(dev->m2m_dev_dec, ctx->m2m_ctx);
		mutex_unlock(&ctx->worker_lock);
		return;
	}

	if (!mtk_vdec_check_scheduling(ctx)) {
		mtk_vdec_res_update_counter(ctx, false);
		mtk_v4l2_debug(4, "[%d] not scheduled for this time", ctx->id);
		v4l2_m2m_job_finish(dev->m2m_dev_dec, ctx->m2m_ctx);
		mutex_unlock(&ctx->worker_lock);
		return;
	}

	buf = &src_buf_info->bs_buffer;
	buf->va = vb2_plane_vaddr(&src_buf->vb2_buf, 0);
	buf->dma_addr = vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
	buf->size = (size_t)src_buf->vb2_buf.planes[0].bytesused;
	buf->length = (size_t)src_buf->vb2_buf.planes[0].length;
	buf->data_offset = (size_t)src_buf->vb2_buf.planes[0].data_offset;
	buf->dmabuf = src_buf->vb2_buf.planes[0].dbuf;
	buf->flags = src_buf->flags;
	buf->index = src_buf->vb2_buf.index;
	buf->hdr10plus_buf = &src_buf_info->hdr10plus_buf;

	if (buf->va == NULL && buf->dmabuf == NULL) {
		mtk_vdec_res_update_counter(ctx, false);
		v4l2_m2m_job_finish(dev->m2m_dev_dec, ctx->m2m_ctx);
		mtk_v4l2_err("[%d] id=%d src_addr is NULL!!",
					 ctx->id, src_buf->vb2_buf.index);
		mutex_unlock(&ctx->worker_lock);
		return;
	}

	ctx->dec_params.timestamp = src_buf_info->vb.vb2_buf.timestamp;
	mtk_v4l2_debug(1,
	"[%d] Bs VA=%p DMA=%pad Size=%zx Offset=%zx Len=%zx ion_buf = %p vb=%p eos=%d pts=%llu",
		ctx->id, buf->va, &buf->dma_addr, buf->size, buf->data_offset,
		buf->length, buf->dmabuf, src_buf,
		src_buf_info->lastframe,
		src_buf_info->vb.vb2_buf.timestamp);
	if (!ctx->input_driven) {
		dst_buf_info->flags &= ~CROP_CHANGED;
		dst_buf_info->flags &= ~REF_FREED;

		dst_buf_info->vb.vb2_buf.timestamp
			= src_buf_info->vb.vb2_buf.timestamp;
		dst_buf_info->vb.timecode
			= src_buf_info->vb.timecode;
		mutex_lock(&ctx->buf_lock);
		dst_buf_info->used = true;
		mutex_unlock(&ctx->buf_lock);
	}

	is_csd = (src_buf_info->flags & V4L2_BUF_FLAG_CSD) ? true : false;
	if (src_buf_info->used == false)
		mtk_vdec_ts_insert(ctx, ctx->dec_params.timestamp, is_csd);
	src_buf_info->used = true;
	ktime_get_real_ts64(&worktvstart1);
	ret = vdec_if_decode(ctx, buf, pfb, &src_chg);
	ktime_get_real_ts64(&vputvend);
	mtk_vcodec_perf_log("vpud:%lld",
		(vputvend.tv_sec - worktvstart1.tv_sec) * 1000000 +
		(vputvend.tv_nsec - worktvstart1.tv_nsec) / 1000);

	res_chg = ((src_chg & VDEC_RES_CHANGE) != 0U) ? true : false;
	framerate_chg  = ((src_chg & VDEC_FRAMERATE_CHANGE) != 0U) ? true : false;
	scantype_chg  = ((src_chg & VDEC_SCANTYPE_CHANGE) != 0U) ? true : false;
	need_more_output =
		((src_chg & VDEC_NEED_MORE_OUTPUT_BUF) != 0U) ? true : false;
	mtk_vcodec_unsupport = ((src_chg & VDEC_HW_NOT_SUPPORT) != 0) ?
						   true : false;
	mtk_bs_unsupport = ((src_chg & VDEC_BS_UNSUPPORT) != 0) ? true : false;
	if (src_chg & VDEC_CROP_CHANGED && !ctx->input_driven)
		dst_buf_info->flags |= CROP_CHANGED;

	if (!ctx->input_driven)
		mtk_vdec_put_fb(ctx, PUT_BUFFER_WORKER);

	mtk_vdec_res_update_counter(ctx, false);
	mtk_vdec_res_info_update(ctx);

	if (ret < 0 || mtk_vcodec_unsupport || mtk_bs_unsupport) {
		mtk_v4l2_err(
			" <===[%d], src_buf[%d] last_frame = %d sz=0x%zx pts=%llu vdec_if_decode() ret=%d src_chg=%d===>",
			ctx->id,
			src_buf->vb2_buf.index,
			src_buf_info->lastframe,
			buf->size,
			src_buf_info->vb.vb2_buf.timestamp,
			ret, src_chg);
		src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		clean_free_bs_buffer(ctx, &src_buf_info->bs_buffer);
		if (mtk_vcodec_unsupport || mtk_bs_unsupport) {
			/*
			 * If cncounter the src unsupport (fatal) during play,
			 * egs: width/height, bitdepth, level, then teturn
			 * error event to user to stop play it
			 */
			mtk_v4l2_err(" <=== [%d] vcodec not support the source!===>",
				ctx->id);
#ifndef TV_INTEGRATION
			ctx->state = MTK_STATE_FLUSH;
#endif
			if (mtk_vcodec_unsupport)
				mtk_vdec_queue_error_event(ctx);
			else
				mtk_vdec_queue_unsupport_bs_event(ctx);
			v4l2_m2m_buf_done(&src_buf_info->vb,
				VB2_BUF_STATE_DONE);
		} else
			v4l2_m2m_buf_done(&src_buf_info->vb,
				VB2_BUF_STATE_ERROR);
	} else if (src_buf_info->lastframe == EOS_WITH_DATA &&
		need_more_output == false) {
		/*
		 * Getting early eos bitstream buffer, after decode this
		 * buffer, need to flush decoder. Use the flush_buf
		 * as normal EOS, and flush decoder.
		 */
		mtk_v4l2_debug(0, "[%d] EarlyEos: decode last frame %d",
			ctx->id, src_buf->vb2_buf.planes[0].bytesused);
		src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		if (src_buf)
			src_buf->flags |= V4L2_BUF_FLAG_LAST;
		clean_free_bs_buffer(ctx, NULL);
		if (ctx->dec_flush_buf->lastframe == NON_EOS) {
			ctx->dec_flush_buf->lastframe = EOS;
			ctx->dec_flush_buf->vb.vb2_buf.planes[0].bytesused = 1;
			v4l2_m2m_buf_queue(ctx->m2m_ctx, &ctx->dec_flush_buf->vb);
		} else {
			mtk_v4l2_debug(1, "Stopping no need to queue dec_flush_buf.");
		}
	} else if ((ret == 0) && (res_chg == false && need_more_output == false)) {
		/*
		 * we only return src buffer with VB2_BUF_STATE_DONE
		 * when decode success without resolution
		 * change.
		 */
		src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		clean_free_bs_buffer(ctx, NULL);
	} else {    /* res_chg == true || need_more_output == true*/
		clean_free_bs_buffer(ctx, &src_buf_info->bs_buffer);
		mtk_v4l2_debug(1, "Need more capture buffer  r:%d n:%d\n",
			res_chg, need_more_output);
	}

	if (ret == 0 && res_chg) {
		mtk_vdec_pic_info_update(ctx);
		/*
		 * On encountering a resolution change in the stream.
		 * The driver must first process and decode all
		 * remaining buffers from before the resolution change
		 * point, so call drain decode here
		 */
		mtk_vdec_drain_decoder(ctx);
		/*
		 * After all buffers containing decoded frames from
		 * before the resolution change point ready to be
		 * dequeued on the CAPTURE queue, the driver sends a
		 * V4L2_EVENT_SOURCE_CHANGE event for source change
		 * type V4L2_EVENT_SRC_CH_RESOLUTION
		 */
		mtk_vdec_queue_res_chg_event(ctx);
	} else if (ret == 0) {
		ret = vdec_if_get_param(ctx, GET_PARAM_DPB_SIZE, &dpbsize);
		if (dpbsize != 0) {
			ctx->dpb_size = dpbsize;
			ctx->last_dpb_size = dpbsize;
		} else {
			mtk_v4l2_err("[%d] GET_PARAM_DPB_SIZE fail=%d",
				 ctx->id, ret);
		}
		ret = vdec_if_get_param(ctx, GET_PARAM_COLOR_DESC, &color_desc);
		if (ret == 0) {
			ctx->is_hdr = color_desc.is_hdr;
			ctx->last_is_hdr = color_desc.is_hdr;
		} else {
			mtk_v4l2_err("[%d] GET_PARAM_COLOR_DESC fail=%d",
				 ctx->id, ret);
		}
		ret = vdec_if_get_param(ctx, GET_PARAM_INTERLACING, &field);
		if (ret == 0) {
			ctx->picinfo.field = field;
			ctx->last_decoded_picinfo.field = field;
		} else {
			mtk_v4l2_err("[%d] GET_PARAM_INTERLACING fail=%d",
				 ctx->id, ret);
		}
		if (framerate_chg)
			mtk_vdec_queue_framerate_chg_event(ctx);
		else if (scantype_chg)
			mtk_vdec_queue_scantype_chg_event(ctx);
	}

	mtk_v4l2_debug(VCODEC_DBG_L4, "[%d] src_buf_rdy_cnt = %u dst_buf_rdy_cnt = %u",
		ctx->id, v4l2_m2m_num_src_bufs_ready(ctx->m2m_ctx),
		v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx));

	v4l2_m2m_job_finish(dev->m2m_dev_dec, ctx->m2m_ctx);
	ktime_get_real_ts64(&vputvend);
	mtk_vcodec_perf_log("worker:%lld",
		(vputvend.tv_sec - worktvstart.tv_sec) * 1000000 +
		(vputvend.tv_nsec - worktvstart.tv_nsec) / 1000);

	mutex_unlock(&ctx->worker_lock);
}

static int vidioc_try_decoder_cmd(struct file *file, void *priv,
	struct v4l2_decoder_cmd *cmd)
{
	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
		cmd->flags = 0; // don't support flags
		break;
	case V4L2_DEC_CMD_START:
		cmd->flags = 0; // don't support flags
		if (cmd->start.speed < 0)
			cmd->start.speed = 0;
		cmd->start.format = V4L2_DEC_START_FMT_NONE;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int vidioc_decoder_cmd(struct file *file, void *priv,
	struct v4l2_decoder_cmd *cmd)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *src_vq, *dst_vq;
	int ret;

	ret = vidioc_try_decoder_cmd(file, priv, cmd);
	if (ret)
		return ret;

	mtk_v4l2_debug(1, "[%d] decoder cmd= %u", ctx->id, cmd->cmd);
	dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx,
		V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
		src_vq = v4l2_m2m_get_vq(ctx->m2m_ctx,
			V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
		if (!vb2_is_streaming(src_vq)) {
			mtk_v4l2_debug(1, "Output stream is off. No need to flush.");
			return 0;
		}
		if (!vb2_is_streaming(dst_vq)) {
			mtk_v4l2_debug(1, "Capture stream is off. No need to flush.");
			return 0;
		}
		if (ctx->dec_flush_buf->lastframe == NON_EOS) {
			ctx->dec_flush_buf->lastframe = EOS;
			ctx->dec_flush_buf->vb.vb2_buf.planes[0].bytesused = 0;
			v4l2_m2m_buf_queue(ctx->m2m_ctx, &ctx->dec_flush_buf->vb);
			v4l2_m2m_try_schedule(ctx->m2m_ctx);
		} else {
			mtk_v4l2_debug(1, "Stopping no need to queue cmd dec_flush_buf.");
		}
		break;

	case V4L2_DEC_CMD_START:
		vb2_clear_last_buffer_dequeued(dst_vq);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

void mtk_vdec_unlock(struct mtk_vcodec_ctx *ctx, u32 hw_id)
{
	mtk_v4l2_debug(4, "ctx %p [%d] hw_id %d sem_cnt %d",
		ctx, ctx->id, hw_id, ctx->dev->dec_sem[hw_id].count);
	if (hw_id < MTK_VDEC_HW_NUM)
		up(&ctx->dev->dec_sem[hw_id]);
}

void mtk_vdec_lock(struct mtk_vcodec_ctx *ctx, u32 hw_id)
{
	unsigned int suspend_block_cnt = 0;
	int ret = -1;

	while (ctx->dev->is_codec_suspending == 1) {
		suspend_block_cnt++;
		if (suspend_block_cnt > SUSPEND_TIMEOUT_CNT) {
			mtk_v4l2_debug(4, "VDEC blocked by suspend\n");
			suspend_block_cnt = 0;
		}
		usleep_range(10000, 20000);
	}

	mtk_v4l2_debug(4, "ctx %p [%d] hw_id %d sem_cnt %d",
		ctx, ctx->id, hw_id, ctx->dev->dec_sem[hw_id].count);
	while (hw_id < MTK_VDEC_HW_NUM && ret != 0)
		ret = down_interruptible(&ctx->dev->dec_sem[hw_id]);
}

void mtk_vcodec_dec_empty_queues(struct file *file, struct mtk_vcodec_ctx *ctx)
{
	struct vb2_v4l2_buffer *src_buf = NULL, *dst_buf = NULL;
	int i = 0;
	struct v4l2_fh *fh = file->private_data;

	// error handle for release before stream-off
	// stream off both queue mannually.
	v4l2_m2m_streamoff(file, fh->m2m_ctx,
		V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	v4l2_m2m_streamoff(file, fh->m2m_ctx,
		V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	while ((src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx)))
		if (src_buf != &ctx->dec_flush_buf->vb)
			v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);

	while ((dst_buf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx))) {
		for (i = 0; i < dst_buf->vb2_buf.num_planes; i++)
			vb2_set_plane_payload(&dst_buf->vb2_buf, i, 0);
		v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);
	}

	ctx->state = MTK_STATE_FREE;
}

void mtk_vcodec_dec_release(struct mtk_vcodec_ctx *ctx)
{
	vdec_if_deinit(ctx);
}

void mtk_vcodec_dec_set_default_params(struct mtk_vcodec_ctx *ctx)
{
	struct mtk_q_data *q_data;

	ctx->m2m_ctx->q_lock = &ctx->dev->dev_mutex;
	ctx->fh.m2m_ctx = ctx->m2m_ctx;
	ctx->fh.ctrl_handler = &ctx->ctrl_hdl;
	INIT_WORK(&ctx->decode_work, mtk_vdec_worker);
	ctx->colorspace = V4L2_COLORSPACE_REC709;
	ctx->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	ctx->quantization = V4L2_QUANTIZATION_DEFAULT;
	ctx->xfer_func = V4L2_XFER_FUNC_DEFAULT;

	get_supported_format(ctx);

	q_data = &ctx->q_data[MTK_Q_DATA_SRC];
	memset(q_data, 0, sizeof(struct mtk_q_data));
	q_data->visible_width = DFT_CFG_WIDTH;
	q_data->visible_height = DFT_CFG_HEIGHT;
	q_data->fmt = &mtk_video_formats[default_out_fmt_idx];
	q_data->field = V4L2_FIELD_NONE;

	q_data->sizeimage[0] = DFT_CFG_WIDTH * DFT_CFG_HEIGHT;
	q_data->bytesperline[0] = 0;

	q_data = &ctx->q_data[MTK_Q_DATA_DST];
	memset(q_data, 0, sizeof(struct mtk_q_data));
	q_data->visible_width = DFT_CFG_WIDTH;
	q_data->visible_height = DFT_CFG_HEIGHT;
	q_data->coded_width = DFT_CFG_WIDTH;
	q_data->coded_height = DFT_CFG_HEIGHT;
	q_data->fmt = &mtk_video_formats[default_cap_fmt_idx];
	q_data->field = V4L2_FIELD_NONE;

	v4l_bound_align_image(&q_data->coded_width,
						  MTK_VDEC_MIN_W,
						  MTK_VDEC_MAX_W, 4,
						  &q_data->coded_height,
						  MTK_VDEC_MIN_H,
						  MTK_VDEC_MAX_H, 5, 6);

	if (q_data->fmt->num_planes == 1) {
		q_data->sizeimage[0] =
			q_data->coded_width * q_data->coded_height * 3/2;
		q_data->bytesperline[0] = q_data->coded_width;

	} else if (q_data->fmt->num_planes == 2) {
		q_data->sizeimage[0] =
			q_data->coded_width * q_data->coded_height;
		q_data->bytesperline[0] = q_data->coded_width;
		q_data->sizeimage[1] = q_data->sizeimage[0] / 2;
		q_data->bytesperline[1] = q_data->coded_width;
	}
}

static int mtk_vdec_set_param(struct mtk_vcodec_ctx *ctx)
{
#ifndef TV_INTEGRATION
	unsigned long in[8] = {0};
#endif
	mtk_v4l2_debug(4,
				   "[%d] param change %d decode mode %d frame width %d frame height %d max width %d max height %d",
				   ctx->id, ctx->dec_param_change,
				   ctx->dec_params.decode_mode,
				   ctx->dec_params.frame_size_width,
				   ctx->dec_params.frame_size_height,
				   ctx->dec_params.fixed_max_frame_size_width,
				   ctx->dec_params.fixed_max_frame_size_height);
#ifndef TV_INTEGRATION
	if (ctx->dec_param_change & MTK_DEC_PARAM_DECODE_MODE) {
		in[0] = ctx->dec_params.decode_mode;
		if (vdec_if_set_param(ctx, SET_PARAM_DECODE_MODE, in) != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot set param", ctx->id);
			return -EINVAL;
		}
		ctx->dec_param_change &= (~MTK_DEC_PARAM_DECODE_MODE);
	}

	if (ctx->dec_param_change & MTK_DEC_PARAM_FRAME_SIZE) {
		in[0] = ctx->dec_params.frame_size_width;
		in[1] = ctx->dec_params.frame_size_height;
		if (in[0] != 0 && in[1] != 0) {
			if (vdec_if_set_param(ctx,
				SET_PARAM_FRAME_SIZE, in) != 0) {
				mtk_v4l2_err("[%d] Error!! Cannot set param",
					ctx->id);
				return -EINVAL;
			}
		}
		ctx->dec_param_change &= (~MTK_DEC_PARAM_FRAME_SIZE);
	}

	if (ctx->dec_param_change &
		MTK_DEC_PARAM_FIXED_MAX_FRAME_SIZE) {
		in[0] = ctx->dec_params.fixed_max_frame_size_width;
		in[1] = ctx->dec_params.fixed_max_frame_size_height;
		if (in[0] != 0 && in[1] != 0) {
			if (vdec_if_set_param(ctx,
				SET_PARAM_SET_FIXED_MAX_OUTPUT_BUFFER,
				in) != 0) {
				mtk_v4l2_err("[%d] Error!! Cannot set param",
					ctx->id);
				return -EINVAL;
			}
		}
		ctx->dec_param_change &= (~MTK_DEC_PARAM_FIXED_MAX_FRAME_SIZE);
	}

	if (ctx->dec_param_change & MTK_DEC_PARAM_CRC_PATH) {
		in[0] = (unsigned long)ctx->dec_params.crc_path;
		if (vdec_if_set_param(ctx, SET_PARAM_CRC_PATH, in) != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot set param", ctx->id);
			return -EINVAL;
		}
		ctx->dec_param_change &= (~MTK_DEC_PARAM_CRC_PATH);
	}

	if (ctx->dec_param_change & MTK_DEC_PARAM_GOLDEN_PATH) {
		in[0] = (unsigned long)ctx->dec_params.golden_path;
		if (vdec_if_set_param(ctx, SET_PARAM_GOLDEN_PATH, in) != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot set param", ctx->id);
			return -EINVAL;
		}
		ctx->dec_param_change &= (~MTK_DEC_PARAM_GOLDEN_PATH);
	}

	if (ctx->dec_param_change & MTK_DEC_PARAM_WAIT_KEY_FRAME) {
		in[0] = (unsigned long)ctx->dec_params.wait_key_frame;
		if (vdec_if_set_param(ctx, SET_PARAM_WAIT_KEY_FRAME, in) != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot set param", ctx->id);
			return -EINVAL;
		}
		ctx->dec_param_change &= (~MTK_DEC_PARAM_WAIT_KEY_FRAME);
	}

	if (ctx->dec_param_change & MTK_DEC_PARAM_NAL_SIZE_LENGTH) {
		in[0] = (unsigned long)ctx->dec_params.wait_key_frame;
		if (vdec_if_set_param(ctx, SET_PARAM_NAL_SIZE_LENGTH,
					in) != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot set param", ctx->id);
			return -EINVAL;
		}
		ctx->dec_param_change &= (~MTK_DEC_PARAM_NAL_SIZE_LENGTH);
	}

	if (ctx->dec_param_change & MTK_DEC_PARAM_OPERATING_RATE) {
		in[0] = (unsigned long)ctx->dec_params.operating_rate;
		if (vdec_if_set_param(ctx, SET_PARAM_OPERATING_RATE, in) != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot set param", ctx->id);
			return -EINVAL;
		}
		ctx->dec_param_change &= (~MTK_DEC_PARAM_OPERATING_RATE);
	}
#endif

	if (vdec_if_get_param(ctx, GET_PARAM_INPUT_DRIVEN,
		&ctx->input_driven)) {
		mtk_v4l2_err("[%d] Error!! Cannot get param : GET_PARAM_INPUT_DRIVEN ERR",
					 ctx->id);
		return -EINVAL;
	}

	return 0;
}

static unsigned int mtk_vdec_bs_get_pattern(unsigned int fourcc,
	unsigned char *patt_buf, unsigned int patt_buf_size)
{
	unsigned int patt_size = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_MPEG4: {
		#define MPEG4_BS_PATT_SIZE 8
		const char bs_patt[MPEG4_BS_PATT_SIZE] = {
			0x00, 0x00, 0x01, 0xB6, 0x00, 0x00, 0x01, 0xB6,
		};
		if (patt_buf_size >= MPEG4_BS_PATT_SIZE) {
			patt_size = MPEG4_BS_PATT_SIZE;
			memcpy(patt_buf, bs_patt, patt_size);
		} else
			mtk_v4l2_debug(VCODEC_DBG_L7,
				"patt buf size 0x%x not enougth, need 0x%x",
				patt_buf_size, MPEG4_BS_PATT_SIZE);
		break;
	}
	default:
		break;
	}

	return patt_size;
}

static unsigned int mtk_vdec_bs_insert_pattern(struct vb2_buffer *vb,
	unsigned int fourcc)
{
	struct mtk_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_plane *plane = &vb->planes[0];
	unsigned char *bs_va;
	unsigned int bs_length;
	unsigned int patt_offset;
	unsigned int patt_size;

	bs_va = (unsigned char *)vb2_plane_vaddr(vb, 0);
	if (!bs_va) {
		mtk_v4l2_debug(VCODEC_DBG_L7, "[%d] (%d) id=%d bs_va invalid",
			ctx->id, vb->vb2_queue->type, vb->index);
		return 0;
	}

	if (plane->dbuf)
		bs_length = (unsigned int)plane->dbuf->size;
	else
		bs_length = (unsigned int)plane->length;

	patt_offset = plane->bytesused;

	if (patt_offset > bs_length) {
		mtk_v4l2_debug(VCODEC_DBG_L7,
			"[%d] (%d) id=%d patt_offset 0x%x invalid, larger than bs_length 0x%x",
			ctx->id, vb->vb2_queue->type, vb->index,
			patt_offset, bs_length);
		return 0;
	}

	patt_size = mtk_vdec_bs_get_pattern(fourcc, &bs_va[patt_offset],
		bs_length - patt_offset);

	if (patt_size)
		mtk_v4l2_debug(VCODEC_DBG_L7,
			"[%d] (%d) id=%d bs_va=%p patt_offset=0x%x patt_size=0x%x",
			ctx->id, vb->vb2_queue->type, vb->index,
			bs_va, patt_offset, patt_size);

	return patt_size;
}

static int vidioc_vdec_qbuf(struct file *file, void *priv,
			    struct v4l2_buffer *buf)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *vq;
	struct vb2_buffer *vb;
	struct mtk_video_dec_buf *mtkbuf;
	struct vb2_v4l2_buffer  *vb2_v4l2;

	if (ctx->state == MTK_STATE_ABORT) {
		mtk_v4l2_err("[%d] Call on QBUF after unrecoverable error",
			     ctx->id);
		return -EIO;
	}

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, buf->type);
	if (buf->index >= vq->num_buffers) {
		mtk_v4l2_err("[%d] buffer index %d out of range %d",
			ctx->id, buf->index, vq->num_buffers);
		return -EINVAL;
	}

	vb = vq->bufs[buf->index];
	vb2_v4l2 = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	mtkbuf = container_of(vb2_v4l2, struct mtk_video_dec_buf, vb);

	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		memcpy(&mtkbuf->hdr10plus_buf, &ctx->dec_params.hdr10plus_buf,
			sizeof(struct hdr10plus_info));
		memset(&ctx->dec_params.hdr10plus_buf, 0,
			sizeof(struct hdr10plus_info)); // invalidate
		ctx->input_max_ts =
			(timeval_to_ns(&buf->timestamp) > ctx->input_max_ts) ?
			timeval_to_ns(&buf->timestamp) : ctx->input_max_ts;
		if (buf->m.planes[0].bytesused == 0) {
			mtkbuf->lastframe = EOS;
			mtk_v4l2_debug(1, "[%d] index=%d Eos BS(%d,%d) vb=%p pts=%llu",
				ctx->id, buf->index,
				buf->bytesused,
				buf->length, vb,
				timeval_to_ns(&buf->timestamp));
		} else if (buf->flags & V4L2_BUF_FLAG_LAST) {
			mtkbuf->lastframe = EOS_WITH_DATA;
			mtk_v4l2_debug(1, "[%d] id=%d EarlyEos BS(%d,%d) vb=%p pts=%llu",
				ctx->id, buf->index, buf->m.planes[0].bytesused,
				buf->length, vb,
				timeval_to_ns(&buf->timestamp));
		} else {
			mtkbuf->lastframe = NON_EOS;
			mtk_v4l2_debug(1, "[%d] id=%d getdata BS(%d,%d) vb=%p pts=%llu %llu",
				ctx->id, buf->index,
				buf->m.planes[0].bytesused,
				buf->length, vb,
				timeval_to_ns(&buf->timestamp),
				ctx->input_max_ts);
		}
	} else {
#ifndef TV_INTEGRATION
		if (buf->reserved2 == 0xFFFFFFFF)
			mtkbuf->general_buf_fd = -1;
		else
			mtkbuf->general_buf_fd = (int)buf->reserved2;
#endif
		mtk_v4l2_debug(1, "[%d] id=%d FB (%d) vb=%p, general_buf_fd=%d, mtkbuf->general_buf_fd = %d",
				ctx->id, buf->index,
				buf->length, mtkbuf,
				buf->reserved2, mtkbuf->general_buf_fd);
	}

	if (buf->flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN) {
		mtk_v4l2_debug(4, "[%d] No need for Cache clean, buf->index:%d. mtkbuf:%p",
			ctx->id, buf->index, mtkbuf);
		mtkbuf->flags |= NO_CAHCE_CLEAN;
	}

	if (buf->flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE) {
		mtk_v4l2_debug(4, "[%d] No need for Cache invalidate, buf->index:%d. mtkbuf:%p",
			ctx->id, buf->index, mtkbuf);
		mtkbuf->flags |= NO_CAHCE_INVALIDATE;
	}

	return v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);
}

static int vidioc_vdec_dqbuf(struct file *file, void *priv,
			     struct v4l2_buffer *buf)
{
	int ret = 0;
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *vq;
	struct vb2_buffer *vb;
	struct mtk_video_dec_buf *mtkbuf;
	struct vb2_v4l2_buffer  *vb2_v4l2;

	if (ctx->state == MTK_STATE_ABORT) {
		mtk_v4l2_err("[%d] Call on DQBUF after unrecoverable error",
			     ctx->id);
		return -EIO;
	}

	ret = v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);
	buf->reserved = ctx->errormap_info[buf->index % VB2_MAX_FRAME];

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
		ret == 0) {
		vq = v4l2_m2m_get_vq(ctx->m2m_ctx, buf->type);
		if (buf->index >= vq->num_buffers) {
			mtk_v4l2_err("[%d] buffer index %d out of range %d",
				ctx->id, buf->index, vq->num_buffers);
			return -EINVAL;
		}
		vb = vq->bufs[buf->index];
		vb2_v4l2 = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
		mtkbuf = container_of(vb2_v4l2, struct mtk_video_dec_buf, vb);

		if (mtkbuf->flags & CROP_CHANGED)
			buf->flags |= V4L2_BUF_FLAG_CROP_CHANGED;
		if (mtkbuf->flags & REF_FREED)
			buf->flags |= V4L2_BUF_FLAG_REF_FREED;
#ifndef TV_INTEGRATION
		if (mtkbuf->general_buf_fd < 0)
			buf->reserved2 = 0xFFFFFFFF;
		else
			buf->reserved2 = mtkbuf->general_buf_fd;

		mtk_v4l2_debug(2,
			"dqbuf mtkbuf->general_buf_fd = %d",
			mtkbuf->general_buf_fd);
#endif
	}

	return ret;
}

static int vidioc_vdec_querycap(struct file *file, void *priv,
	struct v4l2_capability *cap)
{
	strlcpy(cap->driver, MTK_VCODEC_DEC_NAME, sizeof(cap->driver));
	strlcpy(cap->bus_info, MTK_PLATFORM_STR, sizeof(cap->bus_info));
	strlcpy(cap->card, MTK_PLATFORM_STR, sizeof(cap->card));

	cap->device_caps  = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int vidioc_vdec_subscribe_evt(struct v4l2_fh *fh,
				     const struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_EOS:
		return v4l2_event_subscribe(fh, sub, 2, NULL);
	case V4L2_EVENT_SOURCE_CHANGE:
		return v4l2_src_change_event_subscribe(fh, sub);
	case V4L2_EVENT_MTK_VDEC_ERROR:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_MTK_VDEC_NOHEADER:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_MTK_VDEC_BITSTREAM_UNSUPPORT:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	default:
		return v4l2_ctrl_subscribe_event(fh, sub);
	}
}

static int vidioc_try_fmt(struct v4l2_format *f, struct mtk_video_fmt *fmt)
{
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	unsigned int i;

	pix_fmt_mp->field = V4L2_FIELD_NONE;

	if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		pix_fmt_mp->num_planes = 1;
		pix_fmt_mp->plane_fmt[0].bytesperline = 0;
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		int tmp_w, tmp_h;

		pix_fmt_mp->height = clamp(pix_fmt_mp->height,
			MTK_VDEC_MIN_H,
			MTK_VDEC_MAX_H);
		pix_fmt_mp->width = clamp(pix_fmt_mp->width,
			MTK_VDEC_MIN_W,
			MTK_VDEC_MAX_W);

		/*
		 * Find next closer width align 64, heign align 64, size align
		 * 64 rectangle
		 * Note: This only get default value, the real HW needed value
		 *       only available when ctx in MTK_STATE_HEADER state
		 */
		tmp_w = pix_fmt_mp->width;
		tmp_h = pix_fmt_mp->height;
		v4l_bound_align_image(&pix_fmt_mp->width,
							  MTK_VDEC_MIN_W,
							  MTK_VDEC_MAX_W, 6,
							  &pix_fmt_mp->height,
							  MTK_VDEC_MIN_H,
							  MTK_VDEC_MAX_H, 6, 9);

		if (pix_fmt_mp->width < tmp_w &&
			(pix_fmt_mp->width + 64) <= MTK_VDEC_MAX_W)
			pix_fmt_mp->width += 64;
		if (pix_fmt_mp->height < tmp_h &&
			(pix_fmt_mp->height + 64) <= MTK_VDEC_MAX_H)
			pix_fmt_mp->height += 64;

		mtk_v4l2_debug(0,
			"before resize width=%d, height=%d, after resize width=%d, height=%d, sizeimage=%d",
			tmp_w, tmp_h, pix_fmt_mp->width,
			pix_fmt_mp->height,
			pix_fmt_mp->width * pix_fmt_mp->height);

		if (fmt->num_planes > 2)
			pix_fmt_mp->num_planes = 2;
		else
			pix_fmt_mp->num_planes = fmt->num_planes;

		pix_fmt_mp->plane_fmt[0].sizeimage =
				pix_fmt_mp->width * pix_fmt_mp->height;
		pix_fmt_mp->plane_fmt[0].bytesperline = pix_fmt_mp->width;

		if (pix_fmt_mp->num_planes == 2) {
			pix_fmt_mp->plane_fmt[1].sizeimage =
				(pix_fmt_mp->width * pix_fmt_mp->height) / 2;
			pix_fmt_mp->plane_fmt[1].bytesperline =
				pix_fmt_mp->width;
		} else if (pix_fmt_mp->num_planes == 1) {
			pix_fmt_mp->plane_fmt[0].sizeimage +=
				(pix_fmt_mp->width * pix_fmt_mp->height) / 2;
		}

	}

	for (i = 0; i < pix_fmt_mp->num_planes; i++)
		memset(&(pix_fmt_mp->plane_fmt[i].reserved[0]), 0x0,
			   sizeof(pix_fmt_mp->plane_fmt[0].reserved));

	pix_fmt_mp->flags = 0;
	memset(&pix_fmt_mp->reserved, 0x0, sizeof(pix_fmt_mp->reserved));
	return 0;
}

static int vidioc_try_fmt_vid_cap_mplane(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct mtk_video_fmt *fmt;
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);

	fmt = mtk_vdec_find_format(ctx, f, MTK_FMT_FRAME);
	if (!fmt && default_cap_fmt_idx < MTK_MAX_DEC_CODECS_SUPPORT) {
		f->fmt.pix.pixelformat =
			mtk_video_formats[default_cap_fmt_idx].fourcc;
		fmt = mtk_vdec_find_format(ctx, f, MTK_FMT_FRAME);
	}
	if (!fmt)
		return -EINVAL;

	return vidioc_try_fmt(f, fmt);
}

static int vidioc_try_fmt_vid_out_mplane(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	struct mtk_video_fmt *fmt;
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);

	fmt = mtk_vdec_find_format(ctx, f, MTK_FMT_DEC);
	if (!fmt && default_out_fmt_idx < MTK_MAX_DEC_CODECS_SUPPORT) {
		f->fmt.pix.pixelformat =
			mtk_video_formats[default_out_fmt_idx].fourcc;
		fmt = mtk_vdec_find_format(ctx, f, MTK_FMT_DEC);
	}

	if (pix_fmt_mp->plane_fmt[0].sizeimage == 0) {
		mtk_v4l2_err("sizeimage of output format must be given");
		return -EINVAL;
	}
	if (!fmt)
		return -EINVAL;

	return vidioc_try_fmt(f, fmt);
}

static int vidioc_vdec_g_selection(struct file *file, void *priv,
	struct v4l2_selection *s)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct mtk_q_data *q_data;

	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	q_data = &ctx->q_data[MTK_Q_DATA_DST];

	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_CROP_DEFAULT:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->picinfo.pic_w;
		s->r.height = ctx->picinfo.pic_h;
		break;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->picinfo.buf_w;
		s->r.height = ctx->picinfo.buf_h;
		break;
	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_CROP:
		if (vdec_if_get_param(ctx, GET_PARAM_CROP_INFO, &(s->r))) {
			/* set to default value if header info not ready yet*/
			s->r.left = 0;
			s->r.top = 0;
			s->r.width = q_data->visible_width;
			s->r.height = q_data->visible_height;
		}
		break;
	default:
		return -EINVAL;
	}

	if (ctx->state < MTK_STATE_HEADER) {
		/* set to default value if header info not ready yet*/
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = q_data->visible_width;
		s->r.height = q_data->visible_height;
		return 0;
	}

	return 0;
}

static int vidioc_vdec_s_selection(struct file *file, void *priv,
	struct v4l2_selection *s)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);

	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_CROP:
		return vdec_if_set_param(ctx, SET_PARAM_CROP_INFO, &s->r);
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef TV_INTEGRATION
static int vidioc_g_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *parm)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_captureparm *cp = &parm->parm.capture;

	if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	if (ctx->state < MTK_STATE_HEADER) {
		mtk_v4l2_err("can't get parm at state %d", ctx->state);
		return -EINVAL;
	}

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;

	if (vdec_if_get_param(ctx, GET_PARAM_FRAME_INTERVAL,
			&cp->timeperframe) != 0) {
		mtk_v4l2_err("[%d] Error!! Cannot get frame rate",
			ctx->id);
		return -EINVAL;
	}

	return 0;
}
#endif

static int vidioc_vdec_s_fmt(struct file *file, void *priv,
							 struct v4l2_format *f)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format_mplane *pix_mp;
	struct mtk_q_data *q_data;
	int ret = 0;
	struct mtk_video_fmt *fmt;
	unsigned long size[2];

	mtk_v4l2_debug(4, "[%d] type %d", ctx->id, f->type);

	q_data = mtk_vdec_get_q_data(ctx, f->type);
	if (!q_data)
		return -EINVAL;

	pix_mp = &f->fmt.pix_mp;
	if ((f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) &&
		vb2_is_busy(&ctx->m2m_ctx->out_q_ctx.q)) {
		mtk_v4l2_err("out_q_ctx buffers already requested");
		return -EBUSY;
	}

	if ((f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) &&
		vb2_is_busy(&ctx->m2m_ctx->cap_q_ctx.q)) {
		mtk_v4l2_err("cap_q_ctx buffers already requested");
		return -EBUSY;
	}

	fmt = mtk_vdec_find_format(ctx, f,
		(f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ?
		MTK_FMT_DEC : MTK_FMT_FRAME);
	if (fmt == NULL) {
		if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
			f->fmt.pix.pixelformat =
				mtk_video_formats[default_out_fmt_idx].fourcc;
			fmt = mtk_vdec_find_format(ctx, f, MTK_FMT_DEC);
		} else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
			f->fmt.pix.pixelformat =
				mtk_video_formats[default_cap_fmt_idx].fourcc;
			fmt = mtk_vdec_find_format(ctx, f, MTK_FMT_FRAME);
		}
	}
	if (!fmt)
		return -EINVAL;

	q_data->fmt = fmt;
	vidioc_try_fmt(f, q_data->fmt);

	if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		q_data->sizeimage[0] = pix_mp->plane_fmt[0].sizeimage;
		q_data->coded_width = pix_mp->width;
		q_data->coded_height = pix_mp->height;
		size[0] = pix_mp->width;
		size[1] = pix_mp->height;

		ctx->colorspace = f->fmt.pix_mp.colorspace;
		ctx->ycbcr_enc = f->fmt.pix_mp.ycbcr_enc;
		ctx->quantization = f->fmt.pix_mp.quantization;
		ctx->xfer_func = f->fmt.pix_mp.xfer_func;

		if (ctx->state == MTK_STATE_FREE) {
			ret = vdec_if_init(ctx, q_data->fmt->fourcc);
			v4l2_m2m_set_dst_buffered(ctx->m2m_ctx,
				ctx->input_driven);
			if (ctx->input_driven)
				init_waitqueue_head(&ctx->fm_wq);
			if (ret) {
				mtk_v4l2_err("[%d]: vdec_if_init() fail ret=%d",
							 ctx->id, ret);
				ctx->state = MTK_STATE_ABORT;
				return -EINVAL;
			}
			vdec_if_set_param(ctx,
				SET_PARAM_FRAME_SIZE, (void *) size);
			ctx->state = MTK_STATE_INIT;
		}
	}

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		vdec_if_set_param(ctx, SET_PARAM_FB_NUM_PLANES,
			(void *) &q_data->fmt->num_planes);

	return 0;
}

static int vidioc_enum_frameintervals(struct file *file, void *priv,
	struct v4l2_frmivalenum *fival)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct mtk_video_frame_frameintervals frameintervals;

	if (fival->index != 0)
		return -EINVAL;

	if (!is_mtk_supported_fmt(fival->pixel_format)) {
		mtk_v4l2_err("[%d] not supported format 0x%x", ctx->id, fival->pixel_format);
		return -EINVAL;
	}

	fival->type = V4L2_FRMIVAL_TYPE_STEPWISE;
	frameintervals.fourcc = fival->pixel_format;
	frameintervals.width = fival->width;
	frameintervals.height = fival->height;

	if (vdec_if_get_param(ctx, GET_PARAM_CAPABILITY_FRAMEINTERVALS,
		&frameintervals) != 0) {
		mtk_v4l2_err("[%d] Error!! Cannot get frame interval", ctx->id);
		return -EINVAL;
	}

	fival->stepwise = frameintervals.stepwise;
	mtk_v4l2_debug(1,
		"vdec frm_interval fourcc %d width %d height %d max %d/%d min %d/%d step %d/%d\n",
		fival->pixel_format,
		fival->width,
		fival->height,
		fival->stepwise.max.numerator,
		fival->stepwise.max.denominator,
		fival->stepwise.min.numerator,
		fival->stepwise.min.denominator,
		fival->stepwise.step.numerator,
		fival->stepwise.step.denominator);

	return 0;
}

static int vidioc_enum_framesizes(struct file *file, void *priv,
	struct v4l2_frmsizeenum *fsize)
{
	int i = 0;
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);

	if (fsize->index != 0)
		return -EINVAL;

	if (mtk_vdec_framesizes[0].fourcc == 0) {
		if (vdec_if_get_param(ctx, GET_PARAM_CAPABILITY_FRAME_SIZES,
			&mtk_vdec_framesizes) != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot get frame size",
				ctx->id);
			return -EINVAL;
		}

		for (i = 0; i < MTK_MAX_DEC_CODECS_SUPPORT; i++) {
			if (mtk_vdec_framesizes[i].fourcc != 0) {
				mtk_v4l2_debug(1,
				"vdec_fs[%d] fourcc %d s %d %d %d %d %d %d\n",
				i, mtk_vdec_framesizes[i].fourcc,
				mtk_vdec_framesizes[i].stepwise.min_width,
				mtk_vdec_framesizes[i].stepwise.max_width,
				mtk_vdec_framesizes[i].stepwise.step_width,
				mtk_vdec_framesizes[i].stepwise.min_height,
				mtk_vdec_framesizes[i].stepwise.max_height,
				mtk_vdec_framesizes[i].stepwise.step_height);
			}
		}
	}

	for (i = 0; i < MTK_MAX_DEC_CODECS_SUPPORT &&
		 mtk_vdec_framesizes[i].fourcc != 0; ++i) {
		if (fsize->pixel_format != mtk_vdec_framesizes[i].fourcc)
			continue;

		fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
		fsize->reserved[0] = mtk_vdec_framesizes[i].profile;
		fsize->reserved[1] = mtk_vdec_framesizes[i].level;
		fsize->stepwise = mtk_vdec_framesizes[i].stepwise;

		mtk_v4l2_debug(1, "%x, %d %d %d %d %d %d %d %d",
					   ctx->dev->dec_capability,
					   fsize->stepwise.min_width,
					   fsize->stepwise.max_width,
					   fsize->stepwise.step_width,
					   fsize->stepwise.min_height,
					   fsize->stepwise.max_height,
					   fsize->stepwise.step_height,
					   fsize->reserved[0],
					   fsize->reserved[1]);
		return 0;
	}

	return -EINVAL;
}

static int vidioc_enum_fmt(struct mtk_vcodec_ctx *ctx, struct v4l2_fmtdesc *f,
	bool output_queue)
{
	struct mtk_video_fmt *fmt;
	int i, j = 0;

	for (i = 0; i < MTK_MAX_DEC_CODECS_SUPPORT &&
		 mtk_video_formats[i].fourcc != 0; i++) {
		if ((output_queue == true) &&
			(mtk_video_formats[i].type != MTK_FMT_DEC))
			continue;
		else if ((output_queue == false) &&
				 (mtk_video_formats[i].type != MTK_FMT_FRAME))
			continue;

		if (j == f->index)
			break;
		++j;
	}

	if (i == MTK_MAX_DEC_CODECS_SUPPORT ||
		mtk_video_formats[i].fourcc == 0)
		return -EINVAL;

	fmt = &mtk_video_formats[i];

	f->pixelformat = fmt->fourcc;
	f->flags = 0;
	memset(f->reserved, 0, sizeof(f->reserved));

	if (mtk_video_formats[i].type != MTK_FMT_DEC)
		f->flags |= V4L2_FMT_FLAG_COMPRESSED;

	v4l_fill_mtk_fmtdesc(f);

	return 0;
}

static int vidioc_vdec_enum_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_fmtdesc *f)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);

	return vidioc_enum_fmt(ctx, f, false);
}

static int vidioc_vdec_enum_fmt_vid_out(struct file *file, void *priv,
					struct v4l2_fmtdesc *f)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);

	return vidioc_enum_fmt(ctx, f, true);
}

static int vidioc_vdec_g_fmt(struct file *file, void *priv,
							 struct v4l2_format *f)
{
	struct mtk_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct vb2_queue *vq;
	struct mtk_q_data *q_data;
	u32     fourcc;
	unsigned int i = 0;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq) {
		mtk_v4l2_err("no vb2 queue for type=%d", f->type);
		return -EINVAL;
	}

	q_data = mtk_vdec_get_q_data(ctx, f->type);

	pix_mp->field = V4L2_FIELD_NONE;
	pix_mp->colorspace = ctx->colorspace;
	pix_mp->ycbcr_enc = ctx->ycbcr_enc;
	pix_mp->quantization = ctx->quantization;
	pix_mp->xfer_func = ctx->xfer_func;

	if ((f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) &&
		(ctx->state >= MTK_STATE_HEADER)) {
		/* Until STREAMOFF is called on the CAPTURE queue
		 * (acknowledging the event), the driver operates as if
		 * the resolution hasn't changed yet.
		 * So we just return picinfo yet, and update picinfo in
		 * stop_streaming hook function
		 */
		for (i = 0; i < q_data->fmt->num_planes; i++) {
			q_data->sizeimage[i] = ctx->picinfo.fb_sz[i];
			q_data->bytesperline[i] =
				ctx->last_decoded_picinfo.buf_w;
		}
		q_data->coded_width = ctx->picinfo.buf_w;
		q_data->coded_height = ctx->picinfo.buf_h;
		q_data->field = ctx->picinfo.field;

		/*
		 * Width and height are set to the dimensions
		 * of the movie, the buffer is bigger and
		 * further processing stages should crop to this
		 * rectangle.
		 */
		fourcc = ctx->q_data[MTK_Q_DATA_SRC].fmt->fourcc;
		pix_mp->width = q_data->coded_width;
		pix_mp->height = q_data->coded_height;
		/*
		 * Set pixelformat to the format in which mt vcodec
		 * outputs the decoded frame
		 */
		pix_mp->num_planes = q_data->fmt->num_planes;
		pix_mp->pixelformat = q_data->fmt->fourcc;
		pix_mp->field = q_data->field;

		for (i = 0; i < pix_mp->num_planes; i++) {
			pix_mp->plane_fmt[i].bytesperline =
				q_data->bytesperline[i];
			pix_mp->plane_fmt[i].sizeimage =
				q_data->sizeimage[i];
		}

		mtk_v4l2_debug(1, "fourcc:(%d %d),field:%d,bytesperline:%d,sizeimage:%d,%d,%d\n",
			fourcc,
			q_data->fmt->fourcc,
			pix_mp->field,
			pix_mp->plane_fmt[0].bytesperline,
			pix_mp->plane_fmt[0].sizeimage,
			pix_mp->plane_fmt[1].bytesperline,
			pix_mp->plane_fmt[1].sizeimage);

	} else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		/*
		 * This is run on OUTPUT
		 * The buffer contains compressed image
		 * so width and height have no meaning.
		 * Assign value here to pass v4l2-compliance test
		 */
		pix_mp->width = q_data->visible_width;
		pix_mp->height = q_data->visible_height;
		pix_mp->plane_fmt[0].bytesperline = q_data->bytesperline[0];
		pix_mp->plane_fmt[0].sizeimage = q_data->sizeimage[0];
		pix_mp->pixelformat = q_data->fmt->fourcc;
		pix_mp->num_planes = q_data->fmt->num_planes;
	} else {
		pix_mp->num_planes = q_data->fmt->num_planes;
		pix_mp->pixelformat = q_data->fmt->fourcc;
		fourcc = ctx->q_data[MTK_Q_DATA_SRC].fmt->fourcc;

		pix_mp->width = q_data->coded_width;
		pix_mp->height = q_data->coded_height;
		for (i = 0; i < pix_mp->num_planes; i++) {
			pix_mp->plane_fmt[i].bytesperline =
				q_data->bytesperline[i];
			pix_mp->plane_fmt[i].sizeimage =
				q_data->sizeimage[i];
		}

		mtk_v4l2_debug(1,
					   " [%d] type=%d state=%d Format information could not be read, not ready yet!",
					   ctx->id, f->type, ctx->state);
	}

	return 0;
}

static int vb2ops_vdec_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				   unsigned int *nplanes, unsigned int sizes[],
				   struct device *alloc_devs[])
{
	struct mtk_vcodec_ctx *ctx;
	struct mtk_q_data *q_data;
	unsigned int i;

	if (IS_ERR_OR_NULL(vq) || IS_ERR_OR_NULL(nbuffers) ||
	    IS_ERR_OR_NULL(nplanes) || IS_ERR_OR_NULL(alloc_devs)) {
		mtk_v4l2_err("vq %p, nbuffers %p, nplanes %p, alloc_devs %p",
			vq, nbuffers, nplanes, alloc_devs);
		return -EINVAL;
	}

	ctx = vb2_get_drv_priv(vq);
	q_data = mtk_vdec_get_q_data(ctx, vq->type);
	if (q_data == NULL || (*nplanes) > MTK_VCODEC_MAX_PLANES) {
		mtk_v4l2_err("vq->type=%d nplanes %d err", vq->type, *nplanes);
		return -EINVAL;
	}

	if (*nplanes) {
		for (i = 0; i < *nplanes; i++) {
			if (sizes[i] < q_data->sizeimage[i])
				return -EINVAL;
		}
	} else {
		if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
			*nplanes = q_data->fmt->num_planes;
		else
			*nplanes = 1;

		for (i = 0; i < *nplanes; i++)
			sizes[i] = q_data->sizeimage[i];
	}

	mtk_v4l2_debug(1,
				   "[%d]\t type = %d, get %d plane(s), %d buffer(s) of size 0x%x 0x%x ",
				   ctx->id, vq->type, *nplanes, *nbuffers,
				   sizes[0], sizes[1]);

	return 0;
}

static int vb2ops_vdec_buf_prepare(struct vb2_buffer *vb)
{
	struct mtk_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_q_data *q_data;
	struct dma_buf_attachment *buf_att;
	struct sg_table *sgt;
	unsigned int plane = 0;
	unsigned int i;
	struct mtk_video_dec_buf *mtkbuf;
	struct vb2_v4l2_buffer *vb2_v4l2;
	unsigned int patt_size = 0;

	mtk_v4l2_debug(4, "[%d] (%d) id=%d",
				   ctx->id, vb->vb2_queue->type, vb->index);

	q_data = mtk_vdec_get_q_data(ctx, vb->vb2_queue->type);

	for (i = 0; i < q_data->fmt->num_planes; i++) {
		if (vb2_plane_size(vb, i) < q_data->sizeimage[i]) {
			mtk_v4l2_err("data will not fit into plane %d (%lu < %d)",
						 i, vb2_plane_size(vb, i),
						 q_data->sizeimage[i]);
		}
	}

	vb2_v4l2 = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	mtkbuf = container_of(vb2_v4l2, struct mtk_video_dec_buf, vb);

	// Check if need to insert patten
	if (vb->vb2_queue->memory == VB2_MEMORY_DMABUF &&
		!(ctx->dec_params.svp_mode)) {
		if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
			patt_size = mtk_vdec_bs_insert_pattern(vb, q_data->fmt->fourcc);
	}

	// Check if need to proceed cache operations
	if (vb->vb2_queue->memory == VB2_MEMORY_DMABUF &&
		!(mtkbuf->flags & NO_CAHCE_CLEAN) &&
		!(ctx->dec_params.svp_mode)) {
		if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
			struct mtk_vcodec_mem src_mem;
			unsigned int sync_size = vb->planes[0].bytesused + patt_size;

			mtk_v4l2_debug(4, "[%d] Cache sync+", ctx->id);

			buf_att = dma_buf_attach(vb->planes[0].dbuf,
				&ctx->dev->plat_dev->dev);

			sgt = dma_buf_map_attachment(buf_att, DMA_TO_DEVICE);
			if (IS_ERR_OR_NULL(sgt)) {
				mtk_v4l2_err("dma_buf_map_attachment fail %d.\n",
					(int)PTR_ERR(sgt));
				dma_buf_detach(vb->planes[0].dbuf, buf_att);
				return -EINVAL;
			}
			mtk_dma_sync_sg_range(sgt, &ctx->dev->plat_dev->dev,
				sync_size, DMA_TO_DEVICE);
			dma_buf_unmap_attachment(buf_att, sgt, DMA_TO_DEVICE);

			src_mem.dma_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
			src_mem.size = (size_t)sync_size;
			dma_buf_detach(vb->planes[0].dbuf, buf_att);

			mtk_v4l2_debug(4,
			   "[%d] Cache sync- TD for %pad sz=%d dev %p",
			   ctx->id,
			   &src_mem.dma_addr,
			   (unsigned int)src_mem.size,
			   &ctx->dev->plat_dev->dev);
		} else {
			for (plane = 0; plane < vb->num_planes; plane++) {
				struct vdec_fb dst_mem;

				mtk_v4l2_debug(4, "[%d] Cache sync+", ctx->id);

				buf_att = dma_buf_attach(vb->planes[plane].dbuf,
					&ctx->dev->plat_dev->dev);
				sgt = dma_buf_map_attachment(buf_att,
					DMA_TO_DEVICE);
				if (IS_ERR_OR_NULL(sgt)) {
					mtk_v4l2_err("dma_buf_map_attachment fail %d.\n",
						(int)PTR_ERR(sgt));
					dma_buf_detach(vb->planes[plane].dbuf, buf_att);
					return -EINVAL;
				}
				dma_sync_sg_for_device(&ctx->dev->plat_dev->dev,
					sgt->sgl,
					sgt->orig_nents,
					DMA_TO_DEVICE);
				dma_buf_unmap_attachment(buf_att,
					sgt, DMA_TO_DEVICE);

				dst_mem.fb_base[plane].dma_addr =
					vb2_dma_contig_plane_dma_addr(vb,
					plane);
				dst_mem.fb_base[plane].size =
					ctx->picinfo.fb_sz[plane];
				dma_buf_detach(vb->planes[plane].dbuf, buf_att);

				mtk_v4l2_debug(4,
				  "[%d] Cache sync- TD for %pad sz=%d dev %p",
				  ctx->id,
				  &dst_mem.fb_base[plane].dma_addr,
				  (unsigned int)dst_mem.fb_base[plane].size,
				  &ctx->dev->plat_dev->dev);
			}
		}
	}
	return 0;
}

static void vb2ops_vdec_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *src_buf;
	struct mtk_vcodec_mem *src_mem;
	unsigned int src_chg = 0;
	bool res_chg = false;
	bool mtk_vcodec_unsupport = false;
	bool need_seq_header = false;
	int ret = 0;
	unsigned long frame_size[2];
	unsigned int i = 0;
	unsigned int dpbsize = 1;
	unsigned int bs_fourcc, fm_fourcc;
	struct mtk_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vb2_v4l2 = NULL;
	struct mtk_video_dec_buf *buf = NULL;
	struct mtk_q_data *dst_q_data;
	u32 fourcc;
	int last_frame_type = 0;
	struct mtk_color_desc color_desc;

	mtk_v4l2_debug(4, "[%d] (%d) id=%d, vb=%p",
				   ctx->id, vb->vb2_queue->type,
				   vb->index, vb);
	/*
	 * check if this buffer is ready to be used after decode
	 */
	vb2_v4l2 = to_vb2_v4l2_buffer(vb);
	buf = container_of(vb2_v4l2, struct mtk_video_dec_buf, vb);
	if (vb->vb2_queue->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mutex_lock(&ctx->buf_lock);
		if (buf->used == false) {
			v4l2_m2m_buf_queue(ctx->m2m_ctx, vb2_v4l2);
			buf->queued_in_vb2 = true;
			buf->queued_in_v4l2 = true;
			buf->ready_to_display = false;
		} else {
			buf->queued_in_vb2 = false;
			buf->queued_in_v4l2 = true;
			buf->ready_to_display = false;
		}
		mutex_unlock(&ctx->buf_lock);
		if (ctx->input_driven)
			wake_up(&ctx->fm_wq);
		return;
	}

	buf->bs_buffer.gen_frame = false;
	buf->used = false;
	v4l2_m2m_buf_queue(ctx->m2m_ctx, to_vb2_v4l2_buffer(vb));

	if (ctx->state != MTK_STATE_INIT) {
		mtk_v4l2_debug(4, "[%d] already init driver %d",
					   ctx->id, ctx->state);
		return;
	}

	src_buf = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	if (!src_buf || src_buf == &ctx->dec_flush_buf->vb) {
		mtk_v4l2_err("No src buffer %p", src_buf);
		return;
	}

	buf = container_of(src_buf, struct mtk_video_dec_buf, vb);
	src_mem = &buf->bs_buffer;
	src_mem->va = vb2_plane_vaddr(&src_buf->vb2_buf, 0);
	src_mem->dma_addr = vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
	src_mem->size = (size_t)src_buf->vb2_buf.planes[0].bytesused;
	src_mem->length = (size_t)src_buf->vb2_buf.planes[0].length;
	src_mem->data_offset = (size_t)src_buf->vb2_buf.planes[0].data_offset;
	src_mem->dmabuf = src_buf->vb2_buf.planes[0].dbuf;
	src_mem->flags = vb2_v4l2->flags;
	src_mem->index = vb->index;
	src_mem->hdr10plus_buf = &buf->hdr10plus_buf;
	ctx->dec_params.timestamp = src_buf->vb2_buf.timestamp;

	mtk_v4l2_debug(2,
		"[%d] buf id=%d va=%p dma=%pad size=%zx offset %zx length=%zx dmabuf=%p pts %llu",
		ctx->id, src_buf->vb2_buf.index,
		src_mem->va, &src_mem->dma_addr,
		src_mem->size, src_mem->data_offset, src_mem->length,
		src_mem->dmabuf, ctx->dec_params.timestamp);

	if (src_mem->va != NULL) {
		mtk_v4l2_debug(VCODEC_DBG_L2, "[%d] %x %x %x %x %x %x %x %x %x\n",
					   ctx->id,
					   ((char *)src_mem->va)[0],
					   ((char *)src_mem->va)[1],
					   ((char *)src_mem->va)[2],
					   ((char *)src_mem->va)[3],
					   ((char *)src_mem->va)[4],
					   ((char *)src_mem->va)[5],
					   ((char *)src_mem->va)[6],
					   ((char *)src_mem->va)[7],
					   ((char *)src_mem->va)[8]);
	}

	frame_size[0] = ctx->dec_params.frame_size_width;
	frame_size[1] = ctx->dec_params.frame_size_height;

	vdec_if_set_param(ctx, SET_PARAM_FRAME_SIZE, frame_size);
	ret = vdec_if_decode(ctx, src_mem, NULL, &src_chg);
	mtk_vdec_set_param(ctx);

	/* src_chg bit0 for res change flag, bit1 for realloc mv buf flag,
	 * bit2 for not support flag, other bits are reserved
	 */
	res_chg = ((src_chg & VDEC_RES_CHANGE) != 0U) ? true : false;
	mtk_vcodec_unsupport = ((src_chg & VDEC_HW_NOT_SUPPORT) != 0) ?
						   true : false;
	need_seq_header = ((src_chg & VDEC_NEED_SEQ_HEADER) != 0U) ?
					  true : false;
	if (ret || !res_chg || mtk_vcodec_unsupport
		|| need_seq_header) {
		/*
		 * fb == NULL menas to parse SPS/PPS header or
		 * resolution info in src_mem. Decode can fail
		 * if there is no SPS header or picture info
		 * in bs
		 */
		vb2_v4l2 = to_vb2_v4l2_buffer(vb);
		buf = container_of(vb2_v4l2, struct mtk_video_dec_buf, vb);
		last_frame_type = buf->lastframe;

		src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		if (!src_buf) {
			mtk_v4l2_err("[%d]Error!!src_buf is NULL!", ctx->id);
			return;
		}
		clean_free_bs_buffer(ctx, NULL);
		mtk_v4l2_debug(ret ? 0 : 1,
			"[%d] vdec_if_decode() src_buf=%d, size=%zu, fail=%d, res_chg=%d, mtk_vcodec_unsupport=%d",
			ctx->id, src_buf->vb2_buf.index,
			src_mem->size, ret, res_chg,
			mtk_vcodec_unsupport);

		/* If not support the source, eg: w/h,
		 * bitdepth, level, we need to stop to play it
		 */
		if (need_seq_header) {
			mtk_v4l2_err("[%d]Error!! Need seq header!",
						 ctx->id);
			mtk_vdec_queue_noseqheader_event(ctx);
		} else if (mtk_vcodec_unsupport || last_frame_type != NON_EOS) {
			mtk_v4l2_err("[%d]Error!! Codec driver not support the file!",
						 ctx->id);
			mtk_vdec_queue_error_event(ctx);
		}
		return;
	}

	if (res_chg) {
		mtk_v4l2_debug(3, "[%d] vdec_if_decode() res_chg: %d\n",
					   ctx->id, res_chg);
		clean_free_bs_buffer(ctx, src_mem);
		mtk_vdec_queue_res_chg_event(ctx);

		/* remove all framebuffer.
		 * framebuffer with old byteused cannot use.
		 */
		while (v4l2_m2m_dst_buf_remove(ctx->m2m_ctx) != NULL)
			mtk_v4l2_debug(3, "[%d] v4l2_m2m_dst_buf_remove()",
					ctx->id);
	}

	if (vdec_if_get_param(ctx, GET_PARAM_PIC_INFO, &ctx->picinfo)) {
		mtk_v4l2_err("[%d]Error!! Cannot get param : GET_PARAM_PICTURE_INFO ERR",
					 ctx->id);
		return;
	}

	dst_q_data = &ctx->q_data[MTK_Q_DATA_DST];
	ctx->last_decoded_picinfo = ctx->picinfo;
	fourcc = ctx->picinfo.fourcc;
	dst_q_data->fmt = mtk_find_fmt_by_pixel(fourcc);

	for (i = 0; i < dst_q_data->fmt->num_planes; i++) {
		dst_q_data->sizeimage[i] = ctx->picinfo.fb_sz[i];
		dst_q_data->bytesperline[i] = ctx->picinfo.buf_w;
	}

	bs_fourcc = ctx->q_data[MTK_Q_DATA_SRC].fmt->fourcc;
	fm_fourcc = ctx->q_data[MTK_Q_DATA_DST].fmt->fourcc;
	mtk_v4l2_debug(0,
				   "[%d] Init Vdec OK wxh=%dx%d pic wxh=%dx%d bitdepth:%d lo:%d sz[0]=0x%x sz[1]=0x%x",
				   ctx->id,
				   ctx->picinfo.buf_w, ctx->picinfo.buf_h,
				   ctx->picinfo.pic_w, ctx->picinfo.pic_h,
				   ctx->picinfo.bitdepth,
				   ctx->picinfo.layout_mode,
				   dst_q_data->sizeimage[0],
				   dst_q_data->sizeimage[1]);

	mtk_v4l2_debug(0, "[%d] bs %c%c%c%c fm %c%c%c%c, num_planes %d, fb_sz[0] %d, fb_sz[1] %d",
				   ctx->id,
				   bs_fourcc & 0xFF, (bs_fourcc >> 8) & 0xFF,
				   (bs_fourcc >> 16) & 0xFF,
				   (bs_fourcc >> 24) & 0xFF,
				   fm_fourcc & 0xFF, (fm_fourcc >> 8) & 0xFF,
				   (fm_fourcc >> 16) & 0xFF,
				   (fm_fourcc >> 24) & 0xFF,
				   dst_q_data->fmt->num_planes,
				   ctx->picinfo.fb_sz[0],
				   ctx->picinfo.fb_sz[1]);

	ret = vdec_if_get_param(ctx, GET_PARAM_DPB_SIZE, &dpbsize);
	if (dpbsize == 0)
		mtk_v4l2_err("[%d] GET_PARAM_DPB_SIZE fail=%d", ctx->id, ret);

	ctx->dpb_size = dpbsize;
	ctx->last_dpb_size = dpbsize;

	ret = vdec_if_get_param(ctx, GET_PARAM_COLOR_DESC, &color_desc);
	if (ret == 0) {
		ctx->is_hdr = color_desc.is_hdr;
		ctx->last_is_hdr = color_desc.is_hdr;
	} else {
		mtk_v4l2_err("[%d] GET_PARAM_COLOR_DESC fail=%d",
		ctx->id, ret);
	}

	ctx->state = MTK_STATE_HEADER;
	mtk_v4l2_debug(1, "[%d] dpbsize=%d", ctx->id, ctx->dpb_size);

}

static void vb2ops_vdec_buf_finish(struct vb2_buffer *vb)
{
	struct mtk_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vb2_v4l2;
	struct mtk_video_dec_buf *buf;
	unsigned int plane = 0;
	struct mtk_video_dec_buf *mtkbuf;

	if (vb->vb2_queue->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return;

	vb2_v4l2 = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	buf = container_of(vb2_v4l2, struct mtk_video_dec_buf, vb);
	mutex_lock(&ctx->buf_lock);
	buf->queued_in_v4l2 = false;
	buf->queued_in_vb2 = false;
	mutex_unlock(&ctx->buf_lock);

	// Check if need to proceed cache operations for Capture Queue
	vb2_v4l2 = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	mtkbuf = container_of(vb2_v4l2, struct mtk_video_dec_buf, vb);

	if (vb->vb2_queue->memory == VB2_MEMORY_DMABUF &&
		!(mtkbuf->flags & NO_CAHCE_INVALIDATE) &&
		!(ctx->dec_params.svp_mode)) {
		for (plane = 0; plane < buf->frame_buffer.num_planes; plane++) {
			struct vdec_fb dst_mem;
			struct dma_buf_attachment *buf_att;
			struct sg_table *sgt;

			mtk_v4l2_debug(4, "[%d] Cache sync+", ctx->id);

			buf_att = dma_buf_attach(vb->planes[plane].dbuf,
				&ctx->dev->plat_dev->dev);
			sgt = dma_buf_map_attachment(buf_att, DMA_FROM_DEVICE);
			dma_sync_sg_for_cpu(&ctx->dev->plat_dev->dev, sgt->sgl,
				sgt->orig_nents, DMA_FROM_DEVICE);
			dma_buf_unmap_attachment(buf_att, sgt, DMA_FROM_DEVICE);

			dst_mem.fb_base[plane].dma_addr =
				vb2_dma_contig_plane_dma_addr(vb, plane);
			dst_mem.fb_base[plane].size = ctx->picinfo.fb_sz[plane];
			dma_buf_detach(vb->planes[plane].dbuf, buf_att);

			mtk_v4l2_debug(4,
				"[%d] Cache sync- FD for %pad sz=%d dev %p pfb %p",
				ctx->id,
				&dst_mem.fb_base[plane].dma_addr,
				(unsigned int)dst_mem.fb_base[plane].size,
				&ctx->dev->plat_dev->dev,
				&buf->frame_buffer);
		}
	}
}

static int vb2ops_vdec_buf_init(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vb2_v4l2 = container_of(vb,
					struct vb2_v4l2_buffer, vb2_buf);
	struct mtk_video_dec_buf *buf = container_of(vb2_v4l2,
					struct mtk_video_dec_buf, vb);

	if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mtk_v4l2_debug(4, "[%d] pfb=%p used %d ready_to_display %d queued_in_v4l2 %d",
			vb->vb2_queue->type, &buf->frame_buffer,
			buf->used, buf->ready_to_display, buf->queued_in_v4l2);
		/* User could use different struct dma_buf*
		 * with the same index & enter this buf_init.
		 * once this buffer buf->used == true will reset in mistake
		 * VB2 use kzalloc for struct mtk_video_dec_buf,
		 * so init could be no need
		 */
		if (buf->used == false) {
			buf->ready_to_display = false;
			buf->queued_in_v4l2 = false;
		}
	} else if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		/* Do not reset EOS for 1st buffer with Early EOS*/
		/* buf->lastframe = NON_EOS; */
	} else {
		mtk_v4l2_err("unknown queue type");
		return -EINVAL;
	}

	return 0;
}

static int vb2ops_vdec_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct mtk_vcodec_ctx *ctx = vb2_get_drv_priv(q);
	unsigned long total_frame_bufq_count;

	mtk_v4l2_debug(4, "[%d] (%d) state=(%x)", ctx->id, q->type, ctx->state);

	if (ctx->state == MTK_STATE_FLUSH)
		ctx->state = MTK_STATE_HEADER;

	//SET_PARAM_TOTAL_FRAME_BUFQ_COUNT for SW DEC
	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		total_frame_bufq_count = q->num_buffers;
		if (vdec_if_set_param(ctx,
			SET_PARAM_TOTAL_FRAME_BUFQ_COUNT,
			&total_frame_bufq_count)) {
			mtk_v4l2_err("[%d] Error!! Cannot set param",
				ctx->id);
		}
	} else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		// set SET_PARAM_TOTAL_BITSTREAM_BUFQ_COUNT for
		// error handling when framing
		total_frame_bufq_count = q->num_buffers;
		if (vdec_if_set_param(ctx,
			SET_PARAM_TOTAL_BITSTREAM_BUFQ_COUNT,
			&total_frame_bufq_count)) {
			mtk_v4l2_err("[%d] Error!! Cannot set param",
				ctx->id);
		}
		mtk_vdec_ts_reset(ctx);
	}

	mtk_vdec_set_param(ctx);
	return 0;
}

static void vb2ops_vdec_stop_streaming(struct vb2_queue *q)
{
	struct vb2_v4l2_buffer *src_buf = NULL, *dst_buf = NULL;
	struct mtk_vcodec_ctx *ctx = vb2_get_drv_priv(q);
	unsigned int i = 0;

	mtk_v4l2_debug(4, "[%d] (%d) state=(%x) ctx->decoded_frame_cnt=%d",
		ctx->id, q->type, ctx->state, ctx->decoded_frame_cnt);

	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (ctx->state >= MTK_STATE_HEADER)
			mtk_vdec_flush_decoder(ctx, q->type);
		while ((src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx)))
			if (src_buf != &ctx->dec_flush_buf->vb)
				v4l2_m2m_buf_done(src_buf,
					VB2_BUF_STATE_ERROR);
		ctx->dec_flush_buf->lastframe = NON_EOS;
		mtk_vdec_res_update_counter(ctx, false);
		return;
	}

	if (ctx->state >= MTK_STATE_HEADER) {

		/* Until STREAMOFF is called on the CAPTURE queue
		 * (acknowledging the event), the driver operates
		 * as if the resolution hasn't changed yet, i.e.
		 * VIDIOC_G_FMT< etc. return previous resolution.
		 * So we update picinfo here
		 */
		ctx->picinfo = ctx->last_decoded_picinfo;
		ctx->dpb_size = ctx->last_dpb_size;
		ctx->is_hdr = ctx->last_is_hdr;

		mtk_v4l2_debug(2,
			"[%d]-> new(%d,%d), old(%d,%d), real(%d,%d) bit:%d\n",
			ctx->id, ctx->last_decoded_picinfo.pic_w,
			ctx->last_decoded_picinfo.pic_h,
			ctx->picinfo.pic_w, ctx->picinfo.pic_h,
			ctx->last_decoded_picinfo.buf_w,
			ctx->last_decoded_picinfo.buf_h,
			ctx->picinfo.bitdepth);

		mtk_vdec_flush_decoder(ctx, q->type);
	}

	while ((dst_buf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx))) {
		for (i = 0; i < dst_buf->vb2_buf.num_planes; i++)
			vb2_set_plane_payload(&dst_buf->vb2_buf, i, 0);
		v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);
	}
	mtk_vdec_res_update_counter(ctx, false);
}

static void m2mops_vdec_device_run(void *priv)
{
	struct mtk_vcodec_ctx *ctx = priv;
	struct mtk_vcodec_dev *dev = ctx->dev;

	queue_work(dev->decode_workqueue, &ctx->decode_work);
}

static int m2mops_vdec_job_ready(void *m2m_priv)
{
	struct mtk_vcodec_ctx *ctx = m2m_priv;

	mtk_v4l2_debug(4, "[%d]", ctx->id);

	if (ctx->state == MTK_STATE_ABORT)
		return 0;

	if ((ctx->last_decoded_picinfo.pic_w != ctx->picinfo.pic_w) ||
		(ctx->last_decoded_picinfo.pic_h != ctx->picinfo.pic_h) ||
		(ctx->last_decoded_picinfo.bitdepth != ctx->picinfo.bitdepth) ||
		(ctx->last_dpb_size != ctx->dpb_size) ||
		(ctx->last_is_hdr != ctx->is_hdr))
		return 0;

	if (ctx->state != MTK_STATE_HEADER)
		return 0;

	if (ctx->input_driven && (*ctx->ipi_blocked)) {
		mtk_v4l2_debug(VCODEC_DBG_L4, "[%d] not ready: ipi blocked", ctx->id);
		return 0;
	}
#ifdef TV_INTEGRATION
	if (ctx->input_driven && !ctx->use_lat &&
	    !v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx)) {
		mtk_v4l2_debug(VCODEC_DBG_L4, "[%d] not ready: no cap buf", ctx->id);
		return 0;
	}
#endif

	mtk_vdec_res_update_counter(ctx, true);

	return 1;
}

static void m2mops_vdec_job_abort(void *priv)
{
	struct mtk_vcodec_ctx *ctx = priv;

	mtk_v4l2_debug(4, "[%d]", ctx->id);
	ctx->state = MTK_STATE_ABORT;
	if (ctx->input_driven)
		wake_up(&ctx->fm_wq);
}

#ifdef TV_INTEGRATION
static enum vdec_set_param_type CID2SetParam(u32 id)
{
	switch (id) {
	case V4L2_CID_VDEC_TRICK_MODE:
		return SET_PARAM_TRICK_MODE;
	case V4L2_CID_VDEC_UFO_MODE:
		return SET_PARAM_UFO_MODE;
	case V4L2_CID_VDEC_HDR10_INFO:
		return SET_PARAM_HDR10_INFO;
	case V4L2_CID_VDEC_NO_REORDER:
		return SET_PARAM_NO_REORDER;
	case V4L2_CID_VDEC_SUBSAMPLE_MODE:
		return SET_PARAM_PER_FRAME_SUBSAMPLE_MODE;
	case V4L2_CID_VDEC_INCOMPLETE_BITSTREAM:
		return SET_PARAM_INCOMPLETE_BS;
	case V4L2_CID_VDEC_ACQUIRE_RESOURCE:
		return SET_PARAM_ACQUIRE_RESOURCE;
	case V4L2_CID_VDEC_SLICE_COUNT:
		return SET_PARAM_SLICE_COUNT;
	case V4L2_CID_VDEC_VPEEK_MODE:
		return SET_PARAM_VPEEK_MODE;
	case V4L2_CID_VDEC_PLUS_DROP_RATIO:
		return SET_PARAM_VDEC_PLUS_DROP_RATIO;
	case V4L2_CID_VDEC_CONTAINER_FRAMERATE:
		return SET_PARAM_CONTAINER_FRAMERATE;
	}

	return SET_PARAM_MAX;
}

static enum vdec_get_param_type CID2GetParam(u32 id)
{
	switch (id) {
	case V4L2_CID_VDEC_TRICK_MODE:
		return GET_PARAM_TRICK_MODE;
	case V4L2_CID_VDEC_DV_SUPPORTED_PROFILE_LEVEL:
		return GET_PARAM_DV_SUPPORTED_PROFILE_LEVEL;
	case V4L2_CID_VDEC_COLOR_DESC:
		return GET_PARAM_VDEC_COLOR_DESC;
	case V4L2_CID_MPEG_MTK_ASPECT_RATIO:
		return GET_PARAM_ASPECT_RATIO;
	}

	return GET_PARAM_MAX;
}

static int vdec_set_hdr10plus_data(struct mtk_vcodec_ctx *ctx,
				   struct v4l2_vdec_hdr10plus_data *hdr10plus_data)
{
	struct __u8 __user *buffer = u64_to_user_ptr(hdr10plus_data->addr);
	struct hdr10plus_info *hdr10plus_buf = &ctx->dec_params.hdr10plus_buf;

	mtk_v4l2_debug(4, "hdr10plus_data size %d", hdr10plus_data->size);
	hdr10plus_buf->size = hdr10plus_data->size;
	if (hdr10plus_buf->size > HDR10_PLUS_MAX_SIZE)
		hdr10plus_buf->size = HDR10_PLUS_MAX_SIZE;

	if (hdr10plus_buf->size == 0)
		return 0;

	if (buffer == NULL) {
		mtk_v4l2_err("invalid null pointer");
		return -EINVAL;
	}

	if (copy_from_user(&hdr10plus_buf->data, buffer, hdr10plus_buf->size)) {
		mtk_v4l2_err("copy hdr10plus from user fails");
		return -EFAULT;
	}

	return 0;
}
#endif

static int mtk_vdec_g_v_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_vcodec_ctx *ctx = ctrl_to_ctx(ctrl);
	int ret = 0;
#ifndef TV_INTEGRATION
	static unsigned int value;
	struct mtk_color_desc *color_desc;
#endif
	switch (ctrl->id) {
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		if (ctx->state >= MTK_STATE_HEADER)
			ctrl->val = ctx->dpb_size;
		else {
			mtk_v4l2_debug(0, "Seqinfo not ready");
			ctrl->val = 0;
		}
		break;
#ifndef TV_INTEGRATION
	case V4L2_CID_MPEG_MTK_FRAME_INTERVAL:
		if (vdec_if_get_param(ctx,
			GET_PARAM_FRAME_INTERVAL, &value) != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot get param", ctx->id);
			ret = -EINVAL;
		}
		ctrl->p_new.p_u32 = &value;
		mtk_v4l2_debug(2, "V4L2_CID_MPEG_MTK_FRAME_INTERVAL val = %u",
					   *(ctrl->p_new.p_u32));
		break;
	case V4L2_CID_MPEG_MTK_COLOR_DESC:
		color_desc = (struct mtk_color_desc *)ctrl->p_new.p_u32;
		if (vdec_if_get_param(ctx, GET_PARAM_COLOR_DESC, color_desc)
		    != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot get param", ctx->id);
			ret = -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_MTK_FIX_BUFFERS:
		if (vdec_if_get_param(ctx,
		    GET_PARAM_PLATFORM_SUPPORTED_FIX_BUFFERS, &ctrl->val)
		    != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot get param", ctx->id);
			ret = -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_MTK_FIX_BUFFERS_SVP:
		if (vdec_if_get_param(ctx,
		    GET_PARAM_PLATFORM_SUPPORTED_FIX_BUFFERS_SVP, &ctrl->val)
		    != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot get param", ctx->id);
			ret = -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_MTK_INTERLACING:
		if (vdec_if_get_param(ctx, GET_PARAM_INTERLACING, &ctrl->val)
		    != 0) {
			mtk_v4l2_err("[%d] Error!! Cannot get param", ctx->id);
			ret = -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_MTK_CODEC_TYPE:
		if (vdec_if_get_param(ctx, GET_PARAM_CODEC_TYPE, &ctrl->val)
			!= 0) {
			mtk_v4l2_err("[%d] Error!! Cannot get param", ctx->id);
			ret = -EINVAL;
		}
		break;
#else
	case V4L2_CID_VDEC_RESOURCE_METRICS: {
		struct v4l2_vdec_resource_metrics *metrics = ctrl->p_new.p;
		struct vdec_resource_info res_info;
		int i;

		if (vdec_if_get_param(ctx, GET_PARAM_RES_INFO, &res_info)) {
			mtk_v4l2_err(
				"[%d]Error!! Cannot get param : GET_PARAM_RES_INFO ERR",
				ctx->id);
			ret = -EINVAL;
			break;
		}
		for (i = MTK_VDEC_CORE_0; i < MTK_VDEC_LAT; i++) {
			if (res_info.hw_used[i])
				metrics->core_used |= (1 << (i - MTK_VDEC_CORE_0));
		}

		metrics->core_usage = res_info.usage;
		metrics->gce = res_info.gce;
		break;
	}
	case V4L2_CID_VDEC_BANDWIDTH_INFO: {
		struct v4l2_vdec_bandwidth_info *info = ctrl->p_new.p;
		struct vdec_bandwidth_info bandwidth_info;
		int i;

		if (vdec_if_get_param(ctx, GET_PARAM_BANDWIDTH_INFO, &bandwidth_info)) {
			mtk_v4l2_err(
				"[%d]Error!! Cannot get param : GET_PARAM_BANDWIDTH_INFO ERR",
				ctx->id);
			ret = -EINVAL;
			break;
		}

		BUILD_BUG_ON((int)MTK_BW_COUNT != (int)V4L2_BW_COUNT);

		for (i = 0; i < MTK_BW_COUNT; i++)
			info->bandwidth_per_pixel[i] = bandwidth_info.bandwidth_per_pixel[i];

		info->compress = bandwidth_info.compress;
		info->vsd = bandwidth_info.vsd;
		break;
	}
	case V4L2_CID_VDEC_TRICK_MODE:
	case V4L2_CID_VDEC_DV_SUPPORTED_PROFILE_LEVEL:
	case V4L2_CID_VDEC_COLOR_DESC:
	case V4L2_CID_MPEG_MTK_ASPECT_RATIO: {
		enum vdec_get_param_type type = CID2GetParam(ctrl->id);

		if (ctrl->is_ptr)
			ret = vdec_if_get_param(ctx, type, ctrl->p_new.p);
		else {
			unsigned int out = 0;

			ret = vdec_if_get_param(ctx, type, &out);
			ctrl->val = out;
		}
		break;
	}
#endif
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int mtk_vdec_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_vcodec_ctx *ctx = ctrl_to_ctx(ctrl);

	mtk_v4l2_debug(4, "[%d] id 0x%x val %d array[0] %d array[1] %d",
				   ctx->id, ctrl->id, ctrl->val,
				   ctrl->p_new.p_u32[0], ctrl->p_new.p_u32[1]);

	switch (ctrl->id) {
#ifndef TV_INTEGRATION
	case V4L2_CID_MPEG_MTK_SEC_DECODE:
		ctx->dec_params.svp_mode = ctrl->val;
		ctx->dec_param_change |= MTK_DEC_PARAM_SEC_DECODE;
#ifdef CONFIG_VB2_MEDIATEK_DMA_CONTIG
		mtk_dma_contig_set_secure_mode(&ctx->dev->plat_dev->dev,
					ctx->dec_params.svp_mode);
#endif
		mtk_v4l2_debug(0, "[%d] V4L2_CID_MPEG_MTK_SEC_DECODE id %d val %d",
			ctx->id, ctrl->id, ctrl->val);
	break;
#else
	case V4L2_CID_VDEC_SECURE_MODE: {
		ctx->dec_params.svp_mode = ctrl->val;
		return 0;
	}
	case V4L2_CID_VDEC_SECURE_PIPEID: {
		ctx->dec_params.pipeline_id = ctrl->p_new.p_u32[0];
		return 0;
	}
	case V4L2_CID_VDEC_DETECT_TIMESTAMP:
		ctx->dec_params.enable_detect_ts = ctrl->val;
		return 0;
	case V4L2_CID_VDEC_SLICE_COUNT:
		ctx->dec_params.slice_count = ctrl->val;
		break;
#endif
	default:
		break;
	}

	switch (ctrl->id) {
#ifndef TV_INTEGRATION
	case V4L2_CID_MPEG_MTK_DECODE_MODE:
		ctx->dec_params.decode_mode = ctrl->val;
		ctx->dec_param_change |= MTK_DEC_PARAM_DECODE_MODE;
		break;
	case V4L2_CID_MPEG_MTK_SEC_DECODE:
		ctx->dec_params.svp_mode = ctrl->val;
		ctx->dec_param_change |= MTK_DEC_PARAM_SEC_DECODE;
#ifdef CONFIG_VB2_MEDIATEK_DMA_CONTIG
		mtk_dma_contig_set_secure_mode(&ctx->dev->plat_dev->dev,
					ctx->dec_params.svp_mode);
#endif
		break;
	case V4L2_CID_MPEG_MTK_FRAME_SIZE:
		if (ctx->dec_params.frame_size_width == 0)
			ctx->dec_params.frame_size_width = ctrl->val;
		else if (ctx->dec_params.frame_size_height == 0)
			ctx->dec_params.frame_size_height = ctrl->val;
		ctx->dec_param_change |= MTK_DEC_PARAM_FRAME_SIZE;
		break;
	case V4L2_CID_MPEG_MTK_FIXED_MAX_FRAME_BUFFER:
		if (ctx->dec_params.fixed_max_frame_size_width == 0)
			ctx->dec_params.fixed_max_frame_size_width = ctrl->val;
		else if (ctx->dec_params.fixed_max_frame_size_height == 0)
			ctx->dec_params.fixed_max_frame_size_height = ctrl->val;
		ctx->dec_param_change |= MTK_DEC_PARAM_FIXED_MAX_FRAME_SIZE;
		break;
	case V4L2_CID_MPEG_MTK_CRC_PATH:
		ctx->dec_params.crc_path = ctrl->p_new.p_char;
		ctx->dec_param_change |= MTK_DEC_PARAM_CRC_PATH;
		break;
	case V4L2_CID_MPEG_MTK_GOLDEN_PATH:
		ctx->dec_params.golden_path = ctrl->p_new.p_char;
		ctx->dec_param_change |= MTK_DEC_PARAM_GOLDEN_PATH;
		break;
	case V4L2_CID_MPEG_MTK_SET_WAIT_KEY_FRAME:
		ctx->dec_params.wait_key_frame = ctrl->val;
		ctx->dec_param_change |= MTK_DEC_PARAM_WAIT_KEY_FRAME;
		break;
	case V4L2_CID_MPEG_MTK_SET_NAL_SIZE_LENGTH:
		ctx->dec_params.nal_size_length = ctrl->val;
		ctx->dec_param_change |= MTK_DEC_PARAM_NAL_SIZE_LENGTH;
		break;
	case V4L2_CID_MPEG_MTK_OPERATING_RATE:
		ctx->dec_params.operating_rate = ctrl->val;
		ctx->dec_param_change |= MTK_DEC_PARAM_OPERATING_RATE;
		break;
	case V4L2_CID_MPEG_MTK_QUEUED_FRAMEBUF_COUNT:
		ctx->dec_params.queued_frame_buf_count = ctrl->val;
		break;
#else
	case V4L2_CID_VDEC_HDR10PLUS_DATA:
		return vdec_set_hdr10plus_data(ctx, ctrl->p_new.p);
	case V4L2_CID_VDEC_TRICK_MODE:
	case V4L2_CID_VDEC_UFO_MODE:
	case V4L2_CID_VDEC_HDR10_INFO:
	case V4L2_CID_VDEC_NO_REORDER:
	case V4L2_CID_VDEC_SUBSAMPLE_MODE:
	case V4L2_CID_VDEC_INCOMPLETE_BITSTREAM:
	case V4L2_CID_VDEC_ACQUIRE_RESOURCE:
	case V4L2_CID_VDEC_SLICE_COUNT:
	case V4L2_CID_VDEC_VPEEK_MODE:
	case V4L2_CID_VDEC_PLUS_DROP_RATIO:
	case V4L2_CID_VDEC_CONTAINER_FRAMERATE: {
		int ret;
		enum vdec_set_param_type type = SET_PARAM_MAX;

		if (!ctx->drv_handle)
			return 0;

		type = CID2SetParam(ctrl->id);
		if (ctrl->is_ptr)
			ret = vdec_if_set_param(ctx, type, ctrl->p_new.p);
		else {
			unsigned long in = ctrl->val;

			ret = vdec_if_set_param(ctx, type, &in);
		}
		if (ctrl->id == V4L2_CID_VDEC_ACQUIRE_RESOURCE) {
			// for un-paused scenario
			v4l2_m2m_try_schedule(ctx->m2m_ctx);
		}
		if (ret < 0) {
			mtk_v4l2_err("ctrl-id=0x%x fails! ret %d",
					ctrl->id, ret);
			return ret;
		}
		break;
	}
#endif
	default:
		mtk_v4l2_err("ctrl-id=0x%x not support!", ctrl->id);
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ctrl_ops mtk_vcodec_dec_ctrl_ops = {
	.g_volatile_ctrl = mtk_vdec_g_v_ctrl,
	.s_ctrl = mtk_vdec_s_ctrl,
};

#ifdef TV_INTEGRATION
static const struct v4l2_ctrl_config vdec_v4l2_ctrl_configs[] = {
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.id = V4L2_CID_VDEC_TRICK_MODE,
		.name = "VDEC set/get trick mode",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			 V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.min = V4L2_VDEC_TRICK_MODE_ALL,
		.max = V4L2_VDEC_TRICK_MODE_I,
		.step = 1,
		.def = V4L2_VDEC_TRICK_MODE_ALL,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.id = V4L2_CID_VDEC_DV_SUPPORTED_PROFILE_LEVEL,
		.name = "VDEC get DV supported profile, level",
		.type = V4L2_CTRL_TYPE_U8,
		.flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_READ_ONLY,
		.min = 0,
		.max = 0xFF,
		.step = 1,
		.def = 0,
		.dims = {
			sizeof(struct v4l2_vdec_dv_profile_level) * V4L2_CID_VDEC_DV_MAX_PROFILE_CNT
		},
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_UFO_MODE,
		.name = "VDEC set ufo moce",
		.min = V4L2_VDEC_UFO_DEFAULT,
		.max = V4L2_VDEC_UFO_OFF,
		.step = 1,
		.def = V4L2_VDEC_UFO_DEFAULT,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_U8,
		.flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_HDR10_INFO,
		.name = "VDEC set HDR10 information",
		.min = 0,
		.max = 0xFF,
		.step = 1,
		.def = 0,
		.dims = {sizeof(struct v4l2_vdec_hdr10_info)},
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_U8,
		.flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_HDR10PLUS_DATA,
		.name = "VDEC set HDR10PLUS data",
		.min = 0,
		.max = 0xFF,
		.step = 1,
		.def = 0,
		.dims = {sizeof(struct v4l2_vdec_hdr10plus_data)},
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_NO_REORDER,
		.name = "VDEC show frame without reorder",
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.id = V4L2_CID_VDEC_SECURE_MODE,
		.name = "VDEC set enable secure mode",
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_U32,
		.id = V4L2_CID_VDEC_SECURE_PIPEID,
		.name = "VDEC set pipeline ID",
		.min = 0,
		.max = 0xffffffff,
		.step = 1,
		.def = INVALID_PIPELINE_ID,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_U8,
		.flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_VOLATILE |
			 V4L2_CTRL_FLAG_READ_ONLY,
		.id = V4L2_CID_VDEC_COLOR_DESC,
		.name = "VDEC color description for HDR",
		.min = 0,
		.max = 0xFF,
		.step = 1,
		.def = 0,
		.dims = {sizeof(struct v4l2_vdec_color_desc)},
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_SUBSAMPLE_MODE,
		.name = "VDEC set subsample mode",
		.min = V4L2_VDEC_SUBSAMPLE_DEFAULT,
		.max = V4L2_VDEC_SUBSAMPLE_ON,
		.step = 1,
		.def = V4L2_VDEC_SUBSAMPLE_DEFAULT,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_INCOMPLETE_BITSTREAM,
		.name = "VDEC set incomplete bitstream",
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_U8,
		.flags = V4L2_CTRL_FLAG_HAS_PAYLOAD |
			 V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_ACQUIRE_RESOURCE,
		.name = "VDEC acquire resource",
		.min = 0,
		.max = 0xFF,
		.step = 1,
		.def = 0,
		.dims = {sizeof(struct v4l2_vdec_resource_parameter)},
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_U8,
		.flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_VOLATILE |
			 V4L2_CTRL_FLAG_READ_ONLY,
		.id = V4L2_CID_VDEC_RESOURCE_METRICS,
		.name = "get VDEC resource metrics",
		.min = 0,
		.max = 0xFF,
		.step = 1,
		.def = 0,
		.dims = {sizeof(struct v4l2_vdec_resource_metrics)},
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.id = V4L2_CID_VDEC_SLICE_COUNT,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.name = "VDEC set slice count",
		.min = 0,
		.max = 256, // max 256 slice in a frame according to HW DE
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.id = V4L2_CID_VDEC_DETECT_TIMESTAMP,
		.name = "VDEC detect timestamp mode",
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_VPEEK_MODE,
		.name = "VDEC show first frame directly",
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.id = V4L2_CID_MPEG_MTK_ASPECT_RATIO,
		.name = "VDEC get aspect ratio",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			 V4L2_CTRL_FLAG_READ_ONLY,
		.min = V4L2_ASPECT_RATIO_NONE,
		.max = V4L2_ASPECT_RATIO_MAX,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.id = V4L2_CID_VDEC_PLUS_DROP_RATIO,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.name = "VDEC set drop ratio",
		.min = 0,
		.max = 4,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_U32,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.id = V4L2_CID_VDEC_CONTAINER_FRAMERATE,
		.name = "VDEC set container framerate",
		.min = 0,
		.max = 0xffffffff,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &mtk_vcodec_dec_ctrl_ops,
		.type = V4L2_CTRL_TYPE_U8,
		.flags = V4L2_CTRL_FLAG_HAS_PAYLOAD | V4L2_CTRL_FLAG_VOLATILE |
			 V4L2_CTRL_FLAG_READ_ONLY,
		.id = V4L2_CID_VDEC_BANDWIDTH_INFO,
		.name = "get VDEC bandwidth info",
		.min = 0,
		.max = 0xFF,
		.step = 1,
		.def = 0,
		.dims = {sizeof(struct v4l2_vdec_bandwidth_info)},
	},
};
#else
static const struct v4l2_ctrl_config mtk_color_desc_ctrl = {
	.ops = &mtk_vcodec_dec_ctrl_ops,
	.id = V4L2_CID_MPEG_MTK_COLOR_DESC,
	.name = "MTK Color Description for HDR",
	.type = V4L2_CTRL_TYPE_U32,
	.min = 0x00000000,
	.max = 0x00ffffff,
	.step = 1,
	.def = 0,
	.dims = { sizeof(struct mtk_color_desc)/sizeof(u32) },
};

static const struct v4l2_ctrl_config mtk_interlacing_ctrl = {
	.ops = &mtk_vcodec_dec_ctrl_ops,
	.id = V4L2_CID_MPEG_MTK_INTERLACING,
	.name = "MTK Query Interlacing",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_codec_type_ctrl = {
	.ops = &mtk_vcodec_dec_ctrl_ops,
	.id = V4L2_CID_MPEG_MTK_CODEC_TYPE,
	.name = "MTK Query HW/SW Codec Type",
	.type = V4L2_CTRL_TYPE_U32,
	.min = 0,
	.max = 10,
	.step = 1,
	.def = 0,
};
#endif

int mtk_vcodec_dec_ctrls_setup(struct mtk_vcodec_ctx *ctx)
{
	struct v4l2_ctrl *ctrl;
#ifdef TV_INTEGRATION
	u32 ctrl_count, ctrl_num;
#endif
	v4l2_ctrl_handler_init(&ctx->ctrl_hdl, MTK_MAX_CTRLS_HINT);

	/* g_volatile_ctrl */
	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
		&mtk_vcodec_dec_ctrl_ops,
		V4L2_CID_MIN_BUFFERS_FOR_CAPTURE,
		0, 32, 1, 1);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

#ifdef TV_INTEGRATION
	ctrl_num = sizeof(vdec_v4l2_ctrl_configs) /
		   sizeof(struct v4l2_ctrl_config);

	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++)
		v4l2_ctrl_new_custom(&ctx->ctrl_hdl,
				     &vdec_v4l2_ctrl_configs[ctrl_count], NULL);
#else
	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
		&mtk_vcodec_dec_ctrl_ops,
		V4L2_CID_MPEG_MTK_FRAME_INTERVAL,
		16666, 41719, 1, 33333);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
				&mtk_vcodec_dec_ctrl_ops,
				V4L2_CID_MPEG_MTK_ASPECT_RATIO,
				0, 0xF000F, 1, 0x10001);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
				&mtk_vcodec_dec_ctrl_ops,
				V4L2_CID_MPEG_MTK_FIX_BUFFERS,
				0, 0xF, 1, 0);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
				&mtk_vcodec_dec_ctrl_ops,
				V4L2_CID_MPEG_MTK_FIX_BUFFERS_SVP,
				0, 0xF, 1, 0);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl,
				&mtk_interlacing_ctrl, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl,
				&mtk_codec_type_ctrl, NULL);

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &mtk_color_desc_ctrl, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;


	/* s_ctrl */
	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
		&mtk_vcodec_dec_ctrl_ops,
		V4L2_CID_MPEG_MTK_DECODE_MODE,
		0, 32, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
		&mtk_vcodec_dec_ctrl_ops,
		V4L2_CID_MPEG_MTK_SEC_DECODE,
		0, 32, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
		&mtk_vcodec_dec_ctrl_ops,
		V4L2_CID_MPEG_MTK_FRAME_SIZE,
		0, 65535, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
		&mtk_vcodec_dec_ctrl_ops,
		V4L2_CID_MPEG_MTK_FIXED_MAX_FRAME_BUFFER,
		0, 65535, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
		&mtk_vcodec_dec_ctrl_ops,
		V4L2_CID_MPEG_MTK_CRC_PATH,
		0, 255, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
		&mtk_vcodec_dec_ctrl_ops,
		V4L2_CID_MPEG_MTK_GOLDEN_PATH,
		0, 255, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
				&mtk_vcodec_dec_ctrl_ops,
				V4L2_CID_MPEG_MTK_SET_WAIT_KEY_FRAME,
				0, 255, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
				&mtk_vcodec_dec_ctrl_ops,
				V4L2_CID_MPEG_MTK_OPERATING_RATE,
				0, 1024, 1, 0);

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
				&mtk_vcodec_dec_ctrl_ops,
				V4L2_CID_MPEG_MTK_QUEUED_FRAMEBUF_COUNT,
				0, 64, 1, 0);
#endif

	if (ctx->ctrl_hdl.error) {
		mtk_v4l2_err("Adding control failed %d",
					 ctx->ctrl_hdl.error);
		return ctx->ctrl_hdl.error;
	}

	v4l2_ctrl_handler_setup(&ctx->ctrl_hdl);
	return 0;
}

const struct v4l2_m2m_ops mtk_vdec_m2m_ops = {
	.device_run     = m2mops_vdec_device_run,
	.job_ready      = m2mops_vdec_job_ready,
	.job_abort      = m2mops_vdec_job_abort,
};

static const struct vb2_ops mtk_vdec_vb2_ops = {
	.queue_setup    = vb2ops_vdec_queue_setup,
	.buf_prepare    = vb2ops_vdec_buf_prepare,
	.buf_queue      = vb2ops_vdec_buf_queue,
	.wait_prepare   = vb2_ops_wait_prepare,
	.wait_finish    = vb2_ops_wait_finish,
	.buf_init       = vb2ops_vdec_buf_init,
	.buf_finish     = vb2ops_vdec_buf_finish,
	.start_streaming        = vb2ops_vdec_start_streaming,
	.stop_streaming = vb2ops_vdec_stop_streaming,
};

const struct v4l2_ioctl_ops mtk_vdec_ioctl_ops = {
	.vidioc_streamon        = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff       = v4l2_m2m_ioctl_streamoff,
	.vidioc_reqbufs         = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf        = v4l2_m2m_ioctl_querybuf,
	.vidioc_expbuf          = v4l2_m2m_ioctl_expbuf,

	.vidioc_qbuf            = vidioc_vdec_qbuf,
	.vidioc_dqbuf           = vidioc_vdec_dqbuf,

	.vidioc_try_fmt_vid_cap_mplane  = vidioc_try_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_out_mplane  = vidioc_try_fmt_vid_out_mplane,

	.vidioc_s_fmt_vid_cap_mplane    = vidioc_vdec_s_fmt,
	.vidioc_s_fmt_vid_out_mplane    = vidioc_vdec_s_fmt,
	.vidioc_g_fmt_vid_cap_mplane    = vidioc_vdec_g_fmt,
	.vidioc_g_fmt_vid_out_mplane    = vidioc_vdec_g_fmt,

	.vidioc_create_bufs             = v4l2_m2m_ioctl_create_bufs,

	.vidioc_enum_fmt_vid_cap        = vidioc_vdec_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_out        = vidioc_vdec_enum_fmt_vid_out,
	.vidioc_enum_framesizes         = vidioc_enum_framesizes,
	.vidioc_enum_frameintervals     = vidioc_enum_frameintervals,

	.vidioc_querycap                = vidioc_vdec_querycap,
	.vidioc_subscribe_event         = vidioc_vdec_subscribe_evt,
	.vidioc_unsubscribe_event       = v4l2_event_unsubscribe,
	.vidioc_g_selection             = vidioc_vdec_g_selection,
	.vidioc_s_selection             = vidioc_vdec_s_selection,
#ifdef TV_INTEGRATION
	.vidioc_g_parm                  = vidioc_g_parm,
#endif
	.vidioc_decoder_cmd     = vidioc_decoder_cmd,
	.vidioc_try_decoder_cmd = vidioc_try_decoder_cmd,
};

#ifdef CONFIG_VB2_MEDIATEK_DMA_CONTIG
static int vdec_dc_ion_map_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;

	mtk_vdec_ion_config_buff(buf->db_attach->dmabuf);
	return mtk_dma_contig_memops.map_dmabuf(mem_priv);
}
#endif

int mtk_vcodec_dec_queue_init(void *priv, struct vb2_queue *src_vq,
	struct vb2_queue *dst_vq)
{
	struct mtk_vcodec_ctx *ctx = priv;
	int ret = 0;

	mtk_v4l2_debug(4, "[%d]", ctx->id);

	src_vq->type            = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes        = VB2_DMABUF | VB2_MMAP;
	src_vq->drv_priv        = ctx;
	src_vq->buf_struct_size = sizeof(struct mtk_video_dec_buf);
	src_vq->ops             = &mtk_vdec_vb2_ops;
#ifdef CONFIG_VB2_MEDIATEK_DMA_CONTIG
	vdec_ion_dma_contig_memops = mtk_dma_contig_memops;
	vdec_ion_dma_contig_memops.map_dmabuf = vdec_dc_ion_map_dmabuf;

	src_vq->mem_ops         = &vdec_ion_dma_contig_memops;
	mtk_v4l2_debug(4, "src_vq use mtk_dma_contig_memops");
#else
	src_vq->mem_ops         = &vb2_dma_contig_memops;
	mtk_v4l2_debug(4, "src_vq use vb2_dma_contig_memops");
#endif
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock            = &ctx->dev->dev_mutex;
	src_vq->dev             = &ctx->dev->plat_dev->dev;

	ret = vb2_queue_init(src_vq);
	if (ret) {
		mtk_v4l2_err("Failed to initialize videobuf2 queue(output)");
		return ret;
	}
	dst_vq->type            = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes        = VB2_DMABUF | VB2_MMAP;
	dst_vq->drv_priv        = ctx;
	dst_vq->buf_struct_size = sizeof(struct mtk_video_dec_buf);
	dst_vq->ops             = &mtk_vdec_vb2_ops;
#ifdef CONFIG_VB2_MEDIATEK_DMA_SG
	dst_vq->mem_ops         = &mtk_dma_sg_memops;
#elif defined(CONFIG_VB2_MEDIATEK_DMA_CONTIG)
	dst_vq->mem_ops         = &vdec_ion_dma_contig_memops;
	mtk_v4l2_debug(4, "dst_vq use mtk_dma_contig_memops");
#else
	dst_vq->mem_ops         = &vb2_dma_contig_memops;
	mtk_v4l2_debug(4, "dst_vq use vb2_dma_contig_memops");
#endif
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock            = &ctx->dev->dev_mutex;
	dst_vq->dev             = &ctx->dev->plat_dev->dev;

	ret = vb2_queue_init(dst_vq);
	if (ret) {
		vb2_queue_release(src_vq);
		mtk_v4l2_err("Failed to initialize videobuf2 queue(capture)");
	}

	return ret;
}
