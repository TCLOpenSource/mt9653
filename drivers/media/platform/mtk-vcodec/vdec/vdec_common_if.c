// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "mtk_vcodec_intr.h"
#include "vdec_vcu_if.h"
#include "vdec_drv_base.h"
#include "mtk_vcodec_dec.h"
#include "mtk_vcodec_drv.h"
#include "vdec_drv_if.h"


static void put_fb_to_free(struct vdec_inst *inst, struct vdec_fb *fb)
{
	struct ring_fb_list *list;

	if (fb != NULL) {
		list = &inst->vsi->list_free;
		if (list->count == DEC_MAX_FB_NUM) {
			mtk_vcodec_err(inst, "[FB] put fb free_list full");
			return;
		}

		mtk_vcodec_debug(inst,
						 "[FB] put fb into free_list @(%p, %llx)",
						 fb->fb_base[0].va,
						 (u64)fb->fb_base[1].dma_addr);

		list->fb_list[list->write_idx].vdec_fb_va = (u64)(uintptr_t)fb;
		list->write_idx = (list->write_idx == DEC_MAX_FB_NUM - 1U) ?
						  0U : list->write_idx + 1U;
		list->count++;
	}
}

static int put_bs_to_free(struct vdec_inst *inst, struct mtk_vcodec_mem *bs)
{
	struct ring_bs_list *list;
	struct vdec_vsi *vsi;

	if (!inst || !bs)
		return -EINVAL;

	vsi = inst->vsi;

	mtk_vcodec_debug(inst, "[BS] put bs into free_list @(%p) idx = %d\n", bs, vsi->list_free_bs.write_idx);
	list = &vsi->list_free_bs;

	if (list->count == DEC_MAX_BS_NUM) {
		mtk_vcodec_err(inst, "bs list is full!\n");
		return -EINVAL;
	} else {
		list->vdec_bs_va_list[list->write_idx] = (u64)(uintptr_t)bs;
		list->write_idx = (list->write_idx == DEC_MAX_BS_NUM - 1U) ? 0U : list->write_idx + 1U;
		list->count++;
	}

	return 0;
}

static void get_pic_info(struct vdec_inst *inst,
						 struct vdec_pic_info *pic)
{
	if (inst == NULL)
		return;
	if (inst->vsi == NULL)
		return;

	memcpy(pic, &inst->vsi->pic, sizeof(struct vdec_pic_info));

	mtk_vcodec_debug(inst, "pic(%d, %d), buf(%d, %d), bitdepth = %d, fourcc = %d\n",
		pic->pic_w, pic->pic_h, pic->buf_w, pic->buf_h,
		pic->bitdepth, pic->fourcc);
	mtk_vcodec_debug(inst, "Y/C(%d, %d)", pic->fb_sz[0], pic->fb_sz[1]);
}

static void get_crop_info(struct vdec_inst *inst, struct v4l2_rect *cr)
{
	if (inst == NULL)
		return;
	if (inst->vsi == NULL)
		return;

	cr->left      = inst->vsi->crop.left;
	cr->top       = inst->vsi->crop.top;
	cr->width     = inst->vsi->crop.width;
	cr->height    = inst->vsi->crop.height;
	mtk_vcodec_debug(inst, "l=%d, t=%d, w=%d, h=%d",
		cr->left, cr->top, cr->width, cr->height);
}

static void get_dpb_size(struct vdec_inst *inst, unsigned int *dpb_sz)
{
	if (inst == NULL)
		return;
	if (inst->vsi == NULL)
		return;

	*dpb_sz = inst->vsi->dec.dpb_sz + 1U;
	mtk_vcodec_debug(inst, "sz=%d", *dpb_sz);
}

static int vdec_init(struct mtk_vcodec_ctx *ctx, unsigned long *h_vdec)
{
	struct vdec_inst *inst = NULL;
	int err = 0;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;
	if (!ctx) {
		mtk_v4l2_err("ctx is NULL");
		err = -ENOMEM;
		goto error_free_inst;
	}

	inst->ctx = ctx;

	switch (ctx->q_data[MTK_Q_DATA_SRC].fmt->fourcc) {
	case V4L2_PIX_FMT_H264:
		inst->vcu.id = IPI_VDEC_H264;
		break;
	case V4L2_PIX_FMT_HEVC:
		inst->vcu.id = IPI_VDEC_H265;
		break;
	case V4L2_PIX_FMT_HEIF:
		inst->vcu.id = IPI_VDEC_HEIF;
		break;
	case V4L2_PIX_FMT_SHVC:
		inst->vcu.id = IPI_VDEC_SHVC;
		break;
	case V4L2_PIX_FMT_VP8:
		inst->vcu.id = IPI_VDEC_VP8;
		break;
	case V4L2_PIX_FMT_VP9:
		inst->vcu.id = IPI_VDEC_VP9;
		break;
	case V4L2_PIX_FMT_MPEG4:
		inst->vcu.id = IPI_VDEC_MPEG4;
		break;
	case V4L2_PIX_FMT_H263:
		inst->vcu.id = IPI_VDEC_H263;
		break;
	case V4L2_PIX_FMT_MPEG1:
	case V4L2_PIX_FMT_MPEG2:
		inst->vcu.id = IPI_VDEC_MPEG12;
		break;
	case V4L2_PIX_FMT_S263:
		inst->vcu.id = IPI_VDEC_S263;
		break;
	case V4L2_PIX_FMT_HEVCDV:
		inst->vcu.id = IPI_VDEC_H265_DV;
		break;
	case V4L2_PIX_FMT_AVCDV:
		inst->vcu.id = IPI_VDEC_H264_DV;
		break;
#ifndef TV_INTEGRATION
	case V4L2_PIX_FMT_WMV1:
	case V4L2_PIX_FMT_WMV2:
	case V4L2_PIX_FMT_WMV3:
	case V4L2_PIX_FMT_WMVA:
	case V4L2_PIX_FMT_WVC1:
		inst->vcu.id = IPI_VDEC_WMV;
		break;
#endif
	case V4L2_PIX_FMT_RV30:
		inst->vcu.id = IPI_VDEC_RV30;
		break;
	case V4L2_PIX_FMT_RV40:
		inst->vcu.id = IPI_VDEC_RV40;
		break;
	case V4L2_PIX_FMT_AV1:
		inst->vcu.id = IPI_VDEC_AV1;
		break;
	case V4L2_PIX_FMT_AVS:
		inst->vcu.id = IPI_VDEC_AVS;
		break;
	case V4L2_PIX_FMT_AVS2:
		inst->vcu.id = IPI_VDEC_AVS2;
		break;
	case V4L2_PIX_FMT_VC1_ANNEX_G:
		inst->vcu.id = IPI_VDEC_VC1;
		break;
	case V4L2_PIX_FMT_VC1_ANNEX_L:
		inst->vcu.id = IPI_VDEC_WMV;
		break;
	case V4L2_PIX_FMT_H266:
		inst->vcu.id = IPI_VDEC_H266;
		break;
	case V4L2_PIX_FMT_AVIF:
		inst->vcu.id = IPI_VDEC_AVIF;
		break;
	case V4L2_PIX_FMT_AVS3:
		inst->vcu.id = IPI_VDEC_AVS3;
		break;
	default:
		mtk_vcodec_err(inst, "%s no fourcc", __func__);
		err = -EINVAL;
		goto error_free_inst;
	}

	inst->vcu.dev = vcu_get_plat_device(ctx->dev->plat_dev);
	if (inst->vcu.dev  == NULL) {
		mtk_vcodec_err(inst, "vcu device is not ready");
		goto error_free_inst;
	}

	inst->vcu.ctx = ctx;
	inst->vcu.handler = vcu_dec_ipi_handler;
	*h_vdec = (unsigned long)inst;

	mtk_vcodec_add_ctx_list(ctx);

	err = vcu_dec_init(&inst->vcu);
	if (err != 0) {
		mtk_vcodec_err(inst, "%s err=%d", __func__, err);
		goto error_free_inst;
	}

	inst->vsi = (struct vdec_vsi *)inst->vcu.vsi;
	ctx->input_driven = inst->vsi->input_driven;
#ifdef TV_INTEGRATION
	ctx->use_lat = inst->vsi->use_lat;
#endif
	ctx->ipi_blocked = &inst->vsi->ipi_blocked;
	*(ctx->ipi_blocked) = 0;

	mtk_vcodec_debug(inst, "Decoder Instance >> %p", inst);

	return 0;

error_free_inst:
	if (ctx)
		mtk_vcodec_del_ctx_list(ctx);
	kfree(inst);
	*h_vdec = (unsigned long)NULL;

	return err;
}

static void vdec_deinit(unsigned long h_vdec)
{
	struct vdec_inst *inst = (struct vdec_inst *)h_vdec;

	mtk_vcodec_debug_enter(inst);

	vcu_dec_deinit(&inst->vcu);

	mtk_vcodec_del_ctx_list(inst->ctx);

	kfree(inst);
}

static int vdec_decode(unsigned long h_vdec, struct mtk_vcodec_mem *bs,
	struct vdec_fb *fb, unsigned int *src_chg)
{
	struct vdec_inst *inst = (struct vdec_inst *)h_vdec;
	struct vdec_vcu_inst *vcu = &inst->vcu;
	int ret = 0;
	unsigned int data[4];
	uint64_t vdec_fb_va;
	uint64_t fb_dma[VIDEO_MAX_PLANES] = { 0 };
	uint32_t num_planes;
	unsigned int i = 0;
	unsigned int bs_fourcc = inst->ctx->q_data[MTK_Q_DATA_SRC].fmt->fourcc;
	unsigned int fm_fourcc = inst->ctx->q_data[MTK_Q_DATA_DST].fmt->fourcc;
	unsigned int *errormap_info = &inst->ctx->errormap_info[0];

	num_planes = fb ? inst->vsi->dec.fb_num_planes : 0U;

	for (i = 0; i < num_planes; i++)
		fb_dma[i] = (u64)fb->fb_base[i].dma_addr;

	vdec_fb_va = (u64)(uintptr_t)fb;

	mtk_vcodec_debug(inst, "+ [%d] FB y_dma=%llx c_dma=%llx va=%p num_planes %d",
		inst->num_nalu, fb_dma[0], fb_dma[1], fb, num_planes);

	/* bs NULL means flush decoder */
	if (bs == NULL)
		return vcu_dec_reset(vcu);

	mtk_vcodec_debug(inst, "+ BS dma=0x%llx dmabuf=%p format=%c%c%c%c",
		(uint64_t)bs->dma_addr, bs->dmabuf, bs_fourcc & 0xFF,
		(bs_fourcc >> 8) & 0xFF, (bs_fourcc >> 16) & 0xFF,
		(bs_fourcc >> 24) & 0xFF);

	inst->vsi->dec.vdec_bs_va = (u64)(uintptr_t)bs;
	inst->vsi->dec.bs_dma = (uint64_t)bs->dma_addr;
#ifdef TV_INTEGRATION
	inst->vsi->dec.bs_va = (uintptr_t)bs->va;
#endif

	for (i = 0; i < num_planes; i++)
		inst->vsi->dec.fb_dma[i] = fb_dma[i];

	if (fb != NULL) {
		inst->vsi->dec.vdec_fb_va = vdec_fb_va;
		inst->vsi->dec.index = fb->index;
		if (fb->dma_general_buf != 0) {
			inst->vsi->general_buf_fd = fb->general_buf_fd;
			inst->vsi->general_buf_size = fb->dma_general_buf->size;
			mtk_vcodec_debug(inst, "get_mapped_fd fb->dma_general_buf = %p, mapped fd = %d, size = %zu",
			    fb->dma_general_buf, inst->vsi->general_buf_fd,
			    fb->dma_general_buf->size);
		} else {
			fb->general_buf_fd = -1;
			inst->vsi->general_buf_fd = -1;
			inst->vsi->general_buf_size = 0;
			mtk_vcodec_debug(inst, "no general buf dmabuf");
		}
	} else {
		inst->vsi->dec.index = 0xFF;
	}

	inst->vsi->dec.queued_frame_buf_count =
		inst->ctx->dec_params.queued_frame_buf_count;
	inst->vsi->dec.timestamp =
		inst->ctx->dec_params.timestamp;
	memcpy(&inst->vsi->hdr10plus_buf, bs->hdr10plus_buf, sizeof(struct hdr10plus_info));

	mtk_vcodec_debug(inst, "+ FB y_fd=%llx c_fd=%llx BS fd=%llx format=%c%c%c%c",
		inst->vsi->dec.fb_fd[0], inst->vsi->dec.fb_fd[1],
		inst->vsi->dec.bs_fd, fm_fourcc & 0xFF,
		(fm_fourcc >> 8) & 0xFF, (fm_fourcc >> 16) & 0xFF,
		(fm_fourcc >> 24) & 0xFF);

	data[0] = (unsigned int)bs->size;
	if (bs->dmabuf)
		data[1] = (unsigned int)bs->dmabuf->size;
	else
		data[1] = (unsigned int)bs->length;
	data[2] = (unsigned int)bs->flags;
	data[3] = (unsigned int)bs->data_offset;
	ret = vcu_dec_start(vcu, data, 4);

	*src_chg = inst->vsi->dec.vdec_changed_info;
	*(errormap_info + bs->index % VB2_MAX_FRAME) =
		inst->vsi->dec.error_map;

	if ((*src_chg & VDEC_NEED_SEQ_HEADER) != 0U)
		mtk_vcodec_err(inst, "- need first seq header -");
	else if ((*src_chg & VDEC_RES_CHANGE) != 0U)
		mtk_vcodec_debug(inst, "- resolution changed -");
	else if ((*src_chg & VDEC_HW_NOT_SUPPORT) != 0U)
		mtk_vcodec_err(inst, "- unsupported -");
	if ((*src_chg & VDEC_BS_GENERATE_FRAME) != 0U)
		bs->gen_frame = true;

	/*ack timeout means vpud has crashed*/
	if (ret == -EIO) {
		mtk_vcodec_err(inst, "- IPI msg ack timeout  -");
		*src_chg = *src_chg | VDEC_HW_NOT_SUPPORT;
	}

	// if vcu does not queue data at init state
	// do it here
	if (!inst->vsi->queue_init_data && inst->ctx->state == MTK_STATE_INIT)
		put_bs_to_free(inst, bs);

	if (ret < 0 || ((*src_chg & VDEC_HW_NOT_SUPPORT) != 0U)
		|| ((*src_chg & VDEC_NEED_SEQ_HEADER) != 0U))
		goto err_free_fb_out;

	inst->ctx->input_driven = inst->vsi->input_driven;
	mtk_vcodec_debug(inst, "\n - NALU[%d] -\n", inst->num_nalu);
	inst->num_nalu++;
	return ret;

err_free_fb_out:
	put_fb_to_free(inst, fb);
	mtk_vcodec_err(inst, "\n - NALU[%d] err=%d -\n", inst->num_nalu, ret);
	return ret;
}

static void vdec_get_bs(struct vdec_inst *inst,
						struct ring_bs_list *list,
						struct mtk_vcodec_mem **out_bs)
{
	unsigned long vdec_bs_va;
	struct mtk_vcodec_mem *bs;

	if (list->count == 0) {
		mtk_vcodec_debug(inst, "[BS] there is no bs");
		*out_bs = NULL;
		return;
	}

	vdec_bs_va = (unsigned long)list->vdec_bs_va_list[list->read_idx];
	bs = (struct mtk_vcodec_mem *)vdec_bs_va;

	*out_bs = bs;
	mtk_vcodec_debug(inst, "[BS] get free bs %lx", vdec_bs_va);

	list->read_idx = (list->read_idx == DEC_MAX_BS_NUM - 1) ?
					 0 : list->read_idx + 1;
	list->count--;
}

static void vdec_get_fb(struct vdec_inst *inst,
	struct ring_fb_list *list,
	bool disp_list, struct vdec_fb **out_fb)
{
	unsigned long vdec_fb_va;
	struct vdec_fb *fb;

	if (list->count == 0) {
		mtk_vcodec_debug(inst, "[FB] there is no %s fb",
						 disp_list ? "disp" : "free");
		*out_fb = NULL;
		return;
	}

	vdec_fb_va = (unsigned long)list->fb_list[list->read_idx].vdec_fb_va;
	fb = (struct vdec_fb *)vdec_fb_va;
	fb->timestamp = list->fb_list[list->read_idx].timestamp;
	fb->field = list->fb_list[list->read_idx].field;
	fb->frame_type = list->fb_list[list->read_idx].frame_type;
	fb->drop = list->fb_list[list->read_idx].drop;

	if (disp_list)
		fb->status |= FB_ST_DISPLAY;
	else
		fb->status |= FB_ST_FREE;

	*out_fb = fb;
	mtk_vcodec_debug(inst, "[FB] get %s fb st=%d poc=%d ts=%llu %llx gbuf fd %d dma %p",
		disp_list ? "disp" : "free",
		fb->status, list->fb_list[list->read_idx].poc,
		list->fb_list[list->read_idx].timestamp,
		list->fb_list[list->read_idx].vdec_fb_va,
		fb->general_buf_fd, fb->dma_general_buf);

	list->read_idx = (list->read_idx == DEC_MAX_FB_NUM - 1U) ?
					 0U : list->read_idx + 1U;
	list->count--;
}

static void get_supported_format(struct vdec_inst *inst,
	struct mtk_video_fmt *video_fmt)
{
	unsigned int i = 0;

	inst->vcu.ctx = inst->ctx;
	vcu_dec_query_cap(&inst->vcu, GET_PARAM_CAPABILITY_SUPPORTED_FORMATS,
					  video_fmt);
	for (i = 0; i < MTK_MAX_DEC_CODECS_SUPPORT; i++) {
		if (video_fmt[i].fourcc != 0) {
			mtk_vcodec_debug(inst, "video_formats[%d] fourcc %d type %d num_planes %d\n",
				i, video_fmt[i].fourcc, video_fmt[i].type,
				video_fmt[i].num_planes);
		}
	}
}

static void get_dv_supported_profile_level(struct vdec_inst *inst,
	struct v4l2_vdec_dv_profile_level *dv_profile_info)
{
	unsigned int i = 0;

	inst->vcu.ctx = inst->ctx;
	vcu_dec_query_cap(&inst->vcu, GET_PARAM_DV_SUPPORTED_PROFILE_LEVEL,
					  dv_profile_info);
	for (i = 0; i < V4L2_CID_VDEC_DV_MAX_PROFILE_CNT; i++) {
		if (dv_profile_info[i].valid != 0) {
			mtk_vcodec_debug(inst, "dv_profile[%d] level %d\n",
				i, dv_profile_info[i].highest_level);
		}
	}
}

static void get_frame_intervals(struct vdec_inst *inst,
	struct mtk_video_frame_frameintervals *f_ints)
{
	inst->vcu.ctx = inst->ctx;
	vcu_dec_query_cap(&inst->vcu, GET_PARAM_CAPABILITY_FRAMEINTERVALS,
					  f_ints);

	mtk_vcodec_debug(inst, "codec fourcc %d w %d h %d max %d/%d min %d/%d step %d/%d\n",
			 f_ints->fourcc, f_ints->width, f_ints->height,
			 f_ints->stepwise.max.numerator, f_ints->stepwise.max.denominator,
			 f_ints->stepwise.min.numerator, f_ints->stepwise.min.denominator,
			 f_ints->stepwise.step.numerator, f_ints->stepwise.step.denominator);
}

static void get_frame_sizes(struct vdec_inst *inst,
	struct mtk_codec_framesizes *codec_framesizes)
{
	unsigned int i = 0;

	inst->vcu.ctx = inst->ctx;
	vcu_dec_query_cap(&inst->vcu, GET_PARAM_CAPABILITY_FRAME_SIZES,
					  codec_framesizes);
	for (i = 0; i < MTK_MAX_DEC_CODECS_SUPPORT; i++) {
		if (codec_framesizes[i].fourcc != 0) {
			mtk_vcodec_debug(inst,
				"codec_fs[%d] fourcc %d s %d %d %d %d %d %d P %d L %d\n",
				i, codec_framesizes[i].fourcc,
				codec_framesizes[i].stepwise.min_width,
				codec_framesizes[i].stepwise.max_width,
				codec_framesizes[i].stepwise.step_width,
				codec_framesizes[i].stepwise.min_height,
				codec_framesizes[i].stepwise.max_height,
				codec_framesizes[i].stepwise.step_height,
				codec_framesizes[i].profile,
				codec_framesizes[i].level);
		}
	}

}

static void get_color_desc(struct vdec_inst *inst,
	struct mtk_color_desc *color_desc)
{
	inst->vcu.ctx = inst->ctx;
	memcpy(color_desc, &inst->vsi->color_desc, sizeof(*color_desc));
}

static void get_aspect_ratio(struct vdec_inst *inst, unsigned int *aspect_ratio)
{
	if (inst->vsi == NULL)
		return;

	inst->vcu.ctx = inst->ctx;
	*aspect_ratio = inst->vsi->aspect_ratio;
}

static void get_supported_fix_buffers(struct vdec_inst *inst,
					unsigned int *supported)
{
	inst->vcu.ctx = inst->ctx;
	if (inst->vsi != NULL)
		*supported = inst->vsi->fix_buffers;
}

static void get_supported_fix_buffers_svp(struct vdec_inst *inst,
					unsigned int *supported)
{
	inst->vcu.ctx = inst->ctx;
	if (inst->vsi != NULL)
		*supported = inst->vsi->fix_buffers_svp;
}

static void get_interlacing(struct vdec_inst *inst,
			    unsigned int *interlacing)
{
	inst->vcu.ctx = inst->ctx;
	if (inst->vsi != NULL)
		*interlacing = inst->vsi->pic.field;
}

static void get_codec_type(struct vdec_inst *inst,
			   unsigned int *codec_type)
{
	inst->vcu.ctx = inst->ctx;
	if (inst->vsi != NULL)
		*codec_type = inst->vsi->codec_type;
}

static void get_input_driven(struct vdec_inst *inst,
			   unsigned int *input_driven)
{
	inst->vcu.ctx = inst->ctx;
	if (inst->vsi != NULL)
		*input_driven = inst->vsi->input_driven;
}

#ifdef TV_INTEGRATION
static void get_v4l2_color_desc(struct vdec_inst *inst,
				struct v4l2_vdec_color_desc *v4l2_color_desc)
{
	struct mtk_color_desc *color_desc;
	int i;

	inst->vcu.ctx = inst->ctx;
	if (inst->vsi == NULL)
		return;

	color_desc = &inst->vsi->color_desc;

	v4l2_color_desc->max_content_light_level =
		color_desc->max_content_light_level;
	v4l2_color_desc->max_pic_light_level =
		color_desc->max_pic_light_level;

	v4l2_color_desc->max_display_mastering_luminance =
		color_desc->max_display_mastering_luminance;
	v4l2_color_desc->min_display_mastering_luminance =
		color_desc->min_display_mastering_luminance;
	for (i = 0; i < 3; i++) {
		v4l2_color_desc->display_primaries_x[i] =
			color_desc->display_primaries_x[i];
		v4l2_color_desc->display_primaries_y[i] =
			color_desc->display_primaries_y[i];
	}
	v4l2_color_desc->white_point_x = color_desc->white_point_x;
	v4l2_color_desc->white_point_y = color_desc->white_point_y;
	v4l2_color_desc->full_range = color_desc->full_range;
	v4l2_color_desc->color_primaries = color_desc->color_primaries;
	v4l2_color_desc->transform_character = color_desc->transform_character;
	v4l2_color_desc->matrix_coeffs = color_desc->matrix_coeffs;
	v4l2_color_desc->is_hdr = color_desc->is_hdr;
}

static void get_trick_mode(struct vdec_inst *inst,
			   unsigned int *trick_mode)
{
	inst->vcu.ctx = inst->ctx;
	if (inst->vsi != NULL)
		*trick_mode = inst->vsi->trick_mode;
}

static void get_frame_interval(struct vdec_inst *inst,
			       struct v4l2_fract *time_per_frame)
{
	inst->vcu.ctx = inst->ctx;
	if (inst->vsi != NULL)
		memcpy(time_per_frame, &inst->vsi->time_per_frame, sizeof(struct v4l2_fract));
}

static void get_res_info(struct vdec_inst *inst,
			 struct vdec_resource_info *res_info)
{
	if (inst->vsi != NULL)
		memcpy(res_info, &inst->vsi->res_info, sizeof(struct vdec_resource_info));
}

static void get_bandwidth_info(struct vdec_inst *inst,
			 struct vdec_bandwidth_info *bandwidth_info)
{
	if (inst->vsi != NULL)
		memcpy(bandwidth_info, &inst->vsi->bandwidth_info,
			sizeof(struct vdec_bandwidth_info));
}
#endif

static int vdec_get_param(unsigned long h_vdec,
	enum vdec_get_param_type type, void *out)
{
	struct vdec_inst *inst = (struct vdec_inst *)h_vdec;
	int ret = 0;

	if (inst == NULL)
		return -EINVAL;

	switch (type) {
	case GET_PARAM_FREE_BITSTREAM_BUFFER:
		if (inst->vsi == NULL)
			return -EINVAL;
		vdec_get_bs(inst, &inst->vsi->list_free_bs, out);
		break;

	case GET_PARAM_DISP_FRAME_BUFFER:
	{
		if (inst->vsi == NULL)
			return -EINVAL;
		vdec_get_fb(inst, &inst->vsi->list_disp, true, out);

		break;
	}

	case GET_PARAM_FREE_FRAME_BUFFER:
	{
		if (inst->vsi == NULL)
			return -EINVAL;
		vdec_get_fb(inst, &inst->vsi->list_free, false, out);

		break;
	}

	case GET_PARAM_PIC_INFO:
		get_pic_info(inst, out);
		break;

	case GET_PARAM_DPB_SIZE:
		get_dpb_size(inst, out);
		break;

	case GET_PARAM_CROP_INFO:
		get_crop_info(inst, out);
		break;

	case GET_PARAM_CAPABILITY_SUPPORTED_FORMATS:
		get_supported_format(inst, out);
		break;

	case GET_PARAM_CAPABILITY_FRAME_SIZES:
		get_frame_sizes(inst, out);
		break;

	case GET_PARAM_COLOR_DESC:
		if (inst->vsi == NULL)
			return -EINVAL;
		get_color_desc(inst, out);
		break;

	case GET_PARAM_ASPECT_RATIO:
		get_aspect_ratio(inst, out);
		break;

	case GET_PARAM_PLATFORM_SUPPORTED_FIX_BUFFERS:
		get_supported_fix_buffers(inst, out);
		break;

	case GET_PARAM_PLATFORM_SUPPORTED_FIX_BUFFERS_SVP:
		get_supported_fix_buffers_svp(inst, out);
		break;

	case GET_PARAM_INTERLACING:
		get_interlacing(inst, out);
		break;

	case GET_PARAM_CODEC_TYPE:
		get_codec_type(inst, out);
		break;

	case GET_PARAM_INPUT_DRIVEN:
		get_input_driven(inst, out);
		break;

#ifdef TV_INTEGRATION
	case GET_PARAM_VDEC_COLOR_DESC:
		get_v4l2_color_desc(inst, out);
		break;
	case GET_PARAM_TRICK_MODE:
		get_trick_mode(inst, out);
		break;
	case GET_PARAM_FRAME_INTERVAL:
		get_frame_interval(inst, out);
		break;
	case GET_PARAM_RES_INFO:
		get_res_info(inst, out);
		break;
	case GET_PARAM_DV_SUPPORTED_PROFILE_LEVEL:
		get_dv_supported_profile_level(inst, out);
		break;
	case GET_PARAM_CAPABILITY_FRAMEINTERVALS:
		get_frame_intervals(inst, out);
		break;
	case GET_PARAM_BANDWIDTH_INFO:
		get_bandwidth_info(inst, out);
		break;
	default:
#endif
		mtk_vcodec_err(inst, "invalid get parameter type=%d", type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int vdec_set_param(unsigned long h_vdec,
	enum vdec_set_param_type type, void *in)
{
	struct vdec_inst *inst = (struct vdec_inst *)h_vdec;
	uint64_t size;
	int ret = 0;

	if (inst == NULL)
		return -EINVAL;

	switch (type) {
	case SET_PARAM_FRAME_SIZE:
	case SET_PARAM_SET_FIXED_MAX_OUTPUT_BUFFER:
		vcu_dec_set_param(&inst->vcu, (unsigned int)type, in, 2U);
		break;
	case SET_PARAM_DECODE_MODE:
	case SET_PARAM_NAL_SIZE_LENGTH:
	case SET_PARAM_WAIT_KEY_FRAME:
	case SET_PARAM_OPERATING_RATE:
	case SET_PARAM_TOTAL_FRAME_BUFQ_COUNT:
	case SET_PARAM_TOTAL_BITSTREAM_BUFQ_COUNT:
	case SET_PARAM_PER_FRAME_SUBSAMPLE_MODE:
	case SET_PARAM_INCOMPLETE_BS:
	case SET_PARAM_NO_REORDER:
	case SET_PARAM_SLICE_COUNT:
	case SET_PARAM_DEC_HW_ID:
	case SET_PARAM_UFO_MODE:
	case SET_PARAM_VPEEK_MODE:
	case SET_PARAM_VDEC_PLUS_DROP_RATIO:
	case SET_PARAM_CONTAINER_FRAMERATE:
		vcu_dec_set_param(&inst->vcu, (unsigned int)type, in, 1U);
		break;
#ifdef TV_INTEGRATION
	case SET_PARAM_ACQUIRE_RESOURCE: {
		struct v4l2_vdec_resource_parameter *res_param = in;
		struct v4l2_fract *framerate = &res_param->frame_rate;
		unsigned long in_vcu[4] = {0};

		in_vcu[0] = res_param->width;
		in_vcu[1] = res_param->height;
		if (framerate->denominator) {
			in_vcu[2] = (framerate->numerator + framerate->denominator / 2) /
				    framerate->denominator;
		}
		in_vcu[3] = res_param->priority;
		ret = vcu_dec_set_param(&inst->vcu, (unsigned int)type, in_vcu, 4U);
		break;
	}
	case SET_PARAM_TRICK_MODE:
		if (inst->vsi == NULL)
			return -EINVAL;
		inst->vsi->trick_mode = *(unsigned int *)in;
		break;
	case SET_PARAM_HDR10_INFO: {
		struct v4l2_vdec_hdr10_info *hdr10_info = in;

		if (inst->vsi == NULL)
			return -EINVAL;

		memcpy(&inst->vsi->hdr10_info, hdr10_info, sizeof(struct v4l2_vdec_hdr10_info));
		inst->vsi->hdr10_info_valid = true;
		break;
	}
#endif
	case SET_PARAM_CROP_INFO:
		if (inst->vsi == NULL)
			return -EINVAL;
		memcpy(&inst->vsi->crop, (struct v4l2_rect *)in, sizeof(struct v4l2_rect));
		break;
	case SET_PARAM_CRC_PATH:
		if (inst->vsi == NULL)
			return -EINVAL;
		size = strlen((char *) *(uintptr_t *)in);
		memcpy(inst->vsi->crc_path, (void *) *(uintptr_t *)in, size);
		break;
	case SET_PARAM_GOLDEN_PATH:
		if (inst->vsi == NULL)
			return -EINVAL;
		size = strlen((char *) *(uintptr_t *)in);
		memcpy(inst->vsi->golden_path, (void *) *(uintptr_t *)in, size);
		break;
	case SET_PARAM_FB_NUM_PLANES:
		if (inst->vsi == NULL)
			return -EINVAL;
		inst->vsi->dec.fb_num_planes = *(unsigned int *)in;
		break;
	default:
		mtk_vcodec_err(inst, "invalid set parameter type=%d\n", type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int vdec_flush(unsigned long h_vdec, enum vdec_flush_type type)
{
	struct vdec_inst *inst = (struct vdec_inst *)h_vdec;
	struct vdec_vcu_inst *vcu = &inst->vcu;
	int ret;

	mtk_vcodec_debug(inst, "+ flush with type %d", type);

	inst->vsi->flush_type = type;
	ret = vcu_dec_reset(vcu);

	mtk_vcodec_debug(inst, "+ flush ret %d", ret);
	return ret;
}

static struct vdec_common_if vdec_if = {
	vdec_init,
	vdec_decode,
	vdec_get_param,
	vdec_set_param,
	vdec_deinit,
	vdec_flush,
};

struct vdec_common_if *get_dec_common_if(void)
{
	return &vdec_if;
}
