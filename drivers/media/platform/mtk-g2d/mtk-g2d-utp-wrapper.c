// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/io.h>

#include "mtk-g2d.h"
#include <linux/mtk-v4l2-g2d.h>
#include <linux/compat.h>

#include "sti_msos.h"
#include "apiGFX.h"
#include "mtk-g2d-utp-wrapper.h"

MS_BOOL bGEDriverInit = FALSE;

int g2d_utp_init(void)
{
	GFX_Config stGFXCfg;

	if (bGEDriverInit == FALSE) {
		GFXRegisterToUtopia(NULL);
		bGEDriverInit = TRUE;
	}

	memset(&stGFXCfg, 0, sizeof(GFX_Config));
	stGFXCfg.bIsCompt = TRUE;
	stGFXCfg.bIsHK = TRUE;
	MApi_GFX_Init(&stGFXCfg);

	return 0;
}

int g2d_resource(u32 type)
{
	switch (type) {
	case V4L2_CID_EXT_G2D_GET_RESOURCE:
		MApi_GFX_BeginDraw();
		break;
	case V4L2_CID_EXT_G2D_FREE_RESOURCE:
		MApi_GFX_EndDraw();
		break;
	}

	return 0;
}

int g2d_wait_done(struct v4l2_ext_control *ext_ctrl)
{
	MS_U16 u16tagID;

	if (copy_from_user(&u16tagID, (void __user *)ext_ctrl->p_u16, sizeof(u16))) {
		G2D_ERROR("copy_from_user fail\n");
		return -1;
	}


	MApi_GFX_WaitForTAGID(u16tagID);

	return 0;
}

int g2d_get_clip(struct v4l2_selection *s)
{
	GFX_Point v0, v1;

	MApi_GFX_GetClip(&v0, &v1);

	s->r.left = v0.x;
	s->r.top = v0.y;
	s->r.width = v1.x - v0.x;
	s->r.height = v1.y - v0.y;

	return 0;
}

int g2d_set_clip(struct v4l2_selection *s)
{
	GFX_Point stPt0, stPt1;

	G2D_DEBUG("left = %u, top = %u, width = %u, height = %u\n",
		s->r.left, s->r.top, s->r.width, s->r.height);

	stPt0.x = s->r.left;
	stPt0.y = s->r.top;
	stPt1.x = s->r.left + s->r.width;
	stPt1.y = s->r.top + s->r.height;
	MApi_GFX_SetClip(&stPt0, &stPt1);

	return 0;
}

GFX_Buffer_Format _utp_fmt_convert(u32 fmt)
{
	switch (fmt) {
	case V4L2_PIX_FMT_I1:
		return GFX_FMT_I1;
	case V4L2_PIX_FMT_I2:
		return GFX_FMT_I2;
	case V4L2_PIX_FMT_I4:
		return GFX_FMT_I4;
	case V4L2_PIX_FMT_PAL8:
		return GFX_FMT_I8;
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_RGB565X:
		return GFX_FMT_RGB565;
	case V4L2_PIX_FMT_ARGB555:
		return GFX_FMT_ARGB1555;
	case V4L2_PIX_FMT_ARGB444:
		return GFX_FMT_ARGB4444;
	case V4L2_PIX_FMT_YUYV:
		return GFX_FMT_YUV422;
	case V4L2_PIX_FMT_ARGB32:
		return GFX_FMT_ARGB8888;
	case V4L2_PIX_FMT_ABGR32:
		return GFX_FMT_ABGR8888;
	default:
		G2D_ERROR("Format not support %u\n", fmt);
		return GFX_FMT_ARGB8888;
	}
}

void _fill_utp_color(GFX_RgbColor *utp_color, struct v4l2_argb_color *g2d_color)
{
	G2D_DEBUG("color a = 0x%x, r = 0x%x, g = 0x%x, b = 0x%x\n",
		g2d_color->a, g2d_color->r, g2d_color->g, g2d_color->b);

	utp_color->a = g2d_color->a;
	utp_color->r = g2d_color->r;
	utp_color->g = g2d_color->g;
	utp_color->b = g2d_color->b;
}

int _g2d_drawline(struct v4l2_g2d_drawline line, u32 fmt)
{
	GFX_DrawLineInfo stDrawLineInfo;

	G2D_DEBUG("X1 = %u, X2 = %u, Y1 = %u, Y2 = %u, width = %u\n",
			   line.x1, line.x2, line.y1, line.y2, line.width);

	stDrawLineInfo.x1 = line.x1;
	stDrawLineInfo.y1 = line.y1;
	stDrawLineInfo.x2 = line.x2;
	stDrawLineInfo.y2 = line.y2;
	stDrawLineInfo.width = line.width;

	stDrawLineInfo.fmt = _utp_fmt_convert(fmt);

	_fill_utp_color(&stDrawLineInfo.colorRange.color_s, &line.colorRange.color_s);
	_fill_utp_color(&stDrawLineInfo.colorRange.color_e, &line.colorRange.color_e);

	stDrawLineInfo.flag = line.drawflag;

	MApi_GFX_Set_Line_Pattern((MS_BOOL) line.pattern.enable, line.pattern.linePatten,
				  line.pattern.repeatFactor);
	if (line.reset == TRUE)
		MApi_GFX_Line_Pattern_Reset();

	if (MApi_GFX_DrawLine(&stDrawLineInfo) != GFX_SUCCESS)
		return -1;

	return 0;
}

int _g2d_rectfill(struct v4l2_g2d_rect rect, u32 fmt)
{
	GFX_RectFillInfo stRectFillInfo;

	G2D_DEBUG("DstX = %u, DstY = %u, DstWidth = %u, DstHeight = %u, flag = %u\n",
			   rect.dstBlock.x, rect.dstBlock.x,
			   rect.dstBlock.width, rect.dstBlock.height,
			   rect.drawflag);

	memset(&stRectFillInfo, 0, sizeof(GFX_RectFillInfo));
	stRectFillInfo.dstBlock.x = rect.dstBlock.x;
	stRectFillInfo.dstBlock.y = rect.dstBlock.y;
	stRectFillInfo.dstBlock.width = rect.dstBlock.width;
	stRectFillInfo.dstBlock.height = rect.dstBlock.height;

	stRectFillInfo.fmt = _utp_fmt_convert(fmt);

	_fill_utp_color(&stRectFillInfo.colorRange.color_s, &rect.colorRange.color_s);
	_fill_utp_color(&stRectFillInfo.colorRange.color_e, &rect.colorRange.color_e);

	stRectFillInfo.flag = rect.drawflag;

	if (MApi_GFX_RectFill(&stRectFillInfo) != GFX_SUCCESS)
		return -1;

	return 0;
}

int _g2d_bitblt(struct v4l2_g2d_bitblt bitblt)
{
	GFX_DrawRect stBitBltInfo;

	G2D_DEBUG("SrcX = %u, SrcY = %u, SrcWidth = %u, SrcHeight = %u\n",
			   bitblt.srcblock.x, bitblt.srcblock.x,
			   bitblt.srcblock.width, bitblt.srcblock.height);
	G2D_DEBUG("DstX = %u, DstY = %u, DstWidth = %u, DstHeight = %u, flag = %u\n",
			   bitblt.dstblock.x, bitblt.dstblock.x,
			   bitblt.dstblock.width, bitblt.dstblock.height,
			   bitblt.drawflag);

	memset(&stBitBltInfo, 0, sizeof(GFX_DrawRect));
	stBitBltInfo.srcblk.x = bitblt.srcblock.x;
	stBitBltInfo.srcblk.y = bitblt.srcblock.y;
	stBitBltInfo.srcblk.width = bitblt.srcblock.width;
	stBitBltInfo.srcblk.height = bitblt.srcblock.height;

	stBitBltInfo.dstblk.x = bitblt.dstblock.x;
	stBitBltInfo.dstblk.y = bitblt.dstblock.y;
	stBitBltInfo.dstblk.width = bitblt.dstblock.width;
	stBitBltInfo.dstblk.height = bitblt.dstblock.height;
	if (MApi_GFX_BitBlt(&stBitBltInfo, bitblt.drawflag) != GFX_SUCCESS)
		return -1;

	return 0;
}

int _g2d_drawoval(struct v4l2_g2d_drawoval oval)
{
	GFX_OvalFillInfo stOvalInfo;

	stOvalInfo.u32LineWidth = oval.lineWidth;
	_fill_utp_color(&stOvalInfo.color, &oval.color);

	stOvalInfo.dstBlock.width = oval.dstBlock.width;
	stOvalInfo.dstBlock.height = oval.dstBlock.height;
	stOvalInfo.dstBlock.x = oval.dstBlock.x;
	stOvalInfo.dstBlock.y = oval.dstBlock.y;

	if (MApi_GFX_DrawOval(&stOvalInfo) != GFX_SUCCESS)
		return -1;

	return 0;
}

int _g2d_spanfill(struct v4l2_g2d_spanfill spanfill, u32 fmt)
{
	struct v4l2_g2d_spans *g2d_spans = NULL;
	GFX_SpanFillInfo stSpanFillInfo;
	Span *spans;
	int idx;
	int ret = 0;

	g2d_spans = kmalloc_array(spanfill.span.num_spans,
		sizeof(struct v4l2_g2d_spans), GFP_KERNEL);
	spans = kmalloc_array(spanfill.span.num_spans, sizeof(Span), GFP_KERNEL);

	if (is_compat_task()) {
		if (copy_from_user((void *)g2d_spans,
			       compat_ptr((uintptr_t)spanfill.span.spans),
			       sizeof(struct v4l2_g2d_spans) * spanfill.span.num_spans)) {
			G2D_ERROR("[copy_from_user fail\n");
			ret = -1;
			goto exit;
		}
	} else {
		if (copy_from_user((void *)g2d_spans, (void __user *)spanfill.span.spans,
			       sizeof(struct v4l2_g2d_spans) * spanfill.span.num_spans)) {
			G2D_ERROR("[copy_from_user fail\n");
			ret = -1;
			goto exit;
		}
	}
	spanfill.span.spans = g2d_spans;

	stSpanFillInfo.span.y = spanfill.span.y;
	stSpanFillInfo.span.num_spans = spanfill.span.num_spans;
	stSpanFillInfo.span.spans = spans;

	for (idx = 0; idx < spanfill.span.num_spans; idx++) {
		stSpanFillInfo.span.spans[idx].x = spanfill.span.spans[idx].x;
		stSpanFillInfo.span.spans[idx].w = spanfill.span.spans[idx].width;
	}

	stSpanFillInfo.clip_box.x = spanfill.dstBlock.x;
	stSpanFillInfo.clip_box.y = spanfill.dstBlock.y;
	stSpanFillInfo.clip_box.width = spanfill.dstBlock.width;
	stSpanFillInfo.clip_box.height = spanfill.dstBlock.height;
	stSpanFillInfo.fmt = _utp_fmt_convert(fmt);
	stSpanFillInfo.flag = spanfill.drawflag;
	_fill_utp_color(&stSpanFillInfo.colorRange.color_s, &spanfill.colorRange.color_s);
	_fill_utp_color(&stSpanFillInfo.colorRange.color_e, &spanfill.colorRange.color_e);

	if (MApi_GFX_SpanFill(&stSpanFillInfo) != GFX_SUCCESS)
		ret = -1;

exit:
	kfree(g2d_spans);
	kfree(spans);
	return ret;
}

int _g2d_trifill(struct v4l2_g2d_trifill tri, u32 fmt)
{
	GFX_TriFillInfo stTriFill;

	stTriFill.tri.x0 = tri.triangle.x0;
	stTriFill.tri.y0 = tri.triangle.y0;
	stTriFill.tri.x1 = tri.triangle.x1;
	stTriFill.tri.y1 = tri.triangle.y1;
	stTriFill.tri.x2 = tri.triangle.x2;
	stTriFill.tri.y2 = tri.triangle.y2;
	stTriFill.clip_box.x = tri.dstblock.x;
	stTriFill.clip_box.y = tri.dstblock.y;
	stTriFill.clip_box.width = tri.dstblock.width;
	stTriFill.clip_box.height = tri.dstblock.height;
	stTriFill.fmt = _utp_fmt_convert(fmt);
	stTriFill.flag = tri.drawflag;
	_fill_utp_color(&stTriFill.colorRange.color_s, &tri.colorRange.color_s);
	_fill_utp_color(&stTriFill.colorRange.color_e, &tri.colorRange.color_e);

	if (MApi_GFX_TriFill(&stTriFill) != GFX_SUCCESS)
		return -1;

	return 0;
}

int g2d_render(struct v4l2_ext_control *ext_ctrl)
{
	struct v4l2_ext_g2d_render_info render;
	GFX_BufferInfo stSrcBuffInfo, stDstBuffInfo;
	int ret = 0;

	if (copy_from_user(&render, (void __user *)ext_ctrl->ptr,
		       sizeof(struct v4l2_ext_g2d_render_info))) {
		G2D_ERROR("copy_from_user fail\n");
		return -1;
	}


	if (render.type == V4L2_G2D_BITBLT) {
		G2D_DEBUG("SrcAddr = 0x%llx, Width = %u, Height = %u, Pitch = %u, Fmt = %u\n",
			render.src.addr, render.src.width,
			render.src.height, render.src.pitch, render.src.fmt);

		stSrcBuffInfo.u32Addr = render.src.addr;
		stSrcBuffInfo.u32Width = render.src.width;
		stSrcBuffInfo.u32Height = render.src.height;
		stSrcBuffInfo.u32Pitch = render.src.pitch;
		stSrcBuffInfo.u32ColorFmt = _utp_fmt_convert(render.src.fmt);
		MApi_GFX_SetSrcBufferInfo(&stSrcBuffInfo, 0);
	}

	G2D_DEBUG("DstAddr = 0x%llx, Width = %u, Height = %u, Pitch = %u, Fmt = %u\n",
		render.dst.addr, render.dst.width,
		render.dst.height, render.dst.pitch, render.dst.fmt);

	stDstBuffInfo.u32Addr = render.dst.addr;
	stDstBuffInfo.u32Width = render.dst.width;
	stDstBuffInfo.u32Height = render.dst.height;
	stDstBuffInfo.u32Pitch = render.dst.pitch;
	stDstBuffInfo.u32ColorFmt = _utp_fmt_convert(render.dst.fmt);
	MApi_GFX_SetDstBufferInfo(&stDstBuffInfo, 0);

	switch (render.type) {
	case V4L2_G2D_DRAW_LINE:
		_g2d_drawline(render.line, render.dst.fmt);
		break;
	case V4L2_G2D_RECTFILL:
		_g2d_rectfill(render.rect, render.dst.fmt);
		break;
	case V4L2_G2D_BITBLT:
		_g2d_bitblt(render.bitblt);
		break;
	case V4L2_G2D_DRAW_OVAL:
		_g2d_drawoval(render.oval);
		break;
	case V4L2_G2D_SPANFILL:
		_g2d_spanfill(render.spanfill, render.dst.fmt);
		break;
	case V4L2_G2D_TRIFILL:
		_g2d_trifill(render.tri, render.dst.fmt);
		break;
	case V4L2_G2D_CLEAR_FRAME:
		MApi_GFX_ClearFrameBuffer(render.frame.addr, render.frame.length,
					  (MS_U8) render.frame.value);
		break;
	case V4L2_G2D_CLEAR_FRAME_BY_WORD:
		MApi_GFX_ClearFrameBufferByWord(render.frame.addr, render.frame.length,
						render.frame.value);
		break;
	default:
		break;
	}

	render.tagID = MApi_GFX_SetNextTAGID();
	if (copy_to_user((void __user *)ext_ctrl->ptr, &render,
		     sizeof(struct v4l2_ext_g2d_render_info))) {
		G2D_ERROR("copy_to_user fail\n");
		return -1;
	}

	return ret;
}

int g2d_get_render_info(struct v4l2_ext_control *ext_ctrl)
{
	struct v4l2_ext_g2d_render_info render;
	GFX_BufferInfo stSrcBuffInfo, stDstBuffInfo;

	MApi_GFX_GetBufferInfo(&stSrcBuffInfo, &stDstBuffInfo);

	memset(&render, 0, sizeof(struct v4l2_ext_g2d_render_info));
	render.src.addr = (__u64) stSrcBuffInfo.u32Addr;
	render.src.width = stSrcBuffInfo.u32Width;
	render.src.height = stSrcBuffInfo.u32Height;
	render.src.pitch = stSrcBuffInfo.u32Pitch;
	render.src.fmt = stSrcBuffInfo.u32ColorFmt;

	render.dst.addr = (__u64) stDstBuffInfo.u32Addr;
	render.dst.width = stDstBuffInfo.u32Width;
	render.dst.height = stDstBuffInfo.u32Height;
	render.dst.pitch = stDstBuffInfo.u32Pitch;
	render.dst.fmt = stDstBuffInfo.u32ColorFmt;

	if (copy_to_user((void __user *)ext_ctrl->ptr, &render,
		     sizeof(struct v4l2_ext_g2d_render_info))) {
		G2D_ERROR("copy_to_user fail\n");
		return -1;
	}

	return 0;
}

int _g2d_set_vcmdq(struct v4l2_g2d_vcmdq vcmdq)
{
	G2D_DEBUG("enable = %d, Addr = %llx, Size = %u\n",
		vcmdq.enable, vcmdq.addr, vcmdq.size);

	if (vcmdq.enable == 1) {
		MApi_GFX_SetVCmdBuffer(vcmdq.addr, (GFX_VcmqBufSize) vcmdq.size);
		MApi_GFX_EnableVCmdQueue(vcmdq.enable);
	}

	return 0;
}

int _g2d_set_paletteopt(struct v4l2_g2d_palette palette)
{
	struct v4l2_argb_color *color = NULL;
	GFX_PaletteEntry *pstPaletteEntry;
	int i;
	int ret = 0;

	color =
	    kmalloc(sizeof(struct v4l2_argb_color) * (palette.palEnd - palette.palStart + 1),
		    GFP_KERNEL);
	pstPaletteEntry =
	    kmalloc(sizeof(GFX_PaletteEntry) * (palette.palEnd - palette.palStart + 1), GFP_KERNEL);

	if (is_compat_task()) {
		if (copy_from_user((void *)color, compat_ptr((uintptr_t)palette.color),
			       sizeof(struct v4l2_argb_color) * (palette.palEnd - palette.palStart +
								 1))) {
			G2D_ERROR("copy_from_user fail\n");
			ret = -1;
			goto exit;
		}
	} else {
		if (copy_from_user((void *)color, (void __user *)palette.color,
			       sizeof(struct v4l2_argb_color) * (palette.palEnd - palette.palStart +
								 1))) {
			G2D_ERROR("copy_from_user fail\n");
			ret = -1;
			goto exit;
		}
	}

	for (i = 0; i < palette.palEnd - palette.palStart; i++) {
		pstPaletteEntry[i].RGB.u8A = color[i].a;
		pstPaletteEntry[i].RGB.u8R = color[i].r;
		pstPaletteEntry[i].RGB.u8G = color[i].g;
		pstPaletteEntry[i].RGB.u8B = color[i].b;
	}

	MApi_GFX_SetPaletteOpt(pstPaletteEntry, palette.palStart, palette.palEnd);

exit:
	kfree(color);
	kfree(pstPaletteEntry);
	return ret;
}

int _g2d_set_colorkey(struct v4l2_g2d_colorkey colorkey)
{
	GFX_RgbColor colorS, colorE;

	G2D_DEBUG("enable = %d, mode = %u\n",
		colorkey.enable, colorkey.mode);

	_fill_utp_color(&colorS, &colorkey.range.color_s);
	_fill_utp_color(&colorE, &colorkey.range.color_s);
	if (colorkey.type == G2D_COLORKEY_SRC) {
		MApi_GFX_SetSrcColorKey((MS_BOOL) colorkey.enable, (GFX_ColorKeyMode) colorkey.mode,
					_utp_fmt_convert(colorkey.fmt), &colorS, &colorE);
	} else if (colorkey.type == G2D_COLORKEY_DST) {
		MApi_GFX_SetDstColorKey((MS_BOOL) colorkey.enable, (GFX_ColorKeyMode) colorkey.mode,
					_utp_fmt_convert(colorkey.fmt), &colorS, &colorE);
	}

	return 0;
}

int _g2d_set_strblt_ck(struct v4l2_g2d_strblt_ck strbltck)
{
	GFX_RgbColor color;

	_fill_utp_color(&color, &strbltck.color);
	MApi_GFX_SetStrBltSckType((GFX_StretchCKType) strbltck.type, &color);

	return 0;
}

int _g2d_set_abl(struct v4l2_g2d_abl abl)
{
	G2D_DEBUG("enable = %d, bldcoef = %d, constAlpha = %d, alphaSrc = %d\n",
		abl.enable, abl.eBlendCoef, abl.constAlpha, abl.eAlphaSrc);

	MApi_GFX_EnableAlphaBlending((MS_BOOL) abl.enable);
	MApi_GFX_SetAlphaBlending((GFX_BlendCoef) abl.eBlendCoef, abl.constAlpha);
	MApi_GFX_SetAlphaSrcFrom((GFX_AlphaSrcFrom) abl.eAlphaSrc);

	return 0;
}

int _g2d_set_dfb(struct v4l2_g2d_dfbbld dfb)
{
	GFX_RgbColor color;

	G2D_DEBUG("enable = %d, flag = %d, SrcOP = %d, DstOP = %d\n",
		dfb.enable, dfb.flags, dfb.eSrcDFBBldOP, dfb.eDstDFBBldOP);

	MApi_GFX_EnableDFBBlending((MS_BOOL) dfb.enable);
	MApi_GFX_EnableAlphaBlending((MS_BOOL) dfb.enable);
	if (dfb.enable == TRUE) {
		MApi_GFX_SetDFBBldFlags(dfb.flags);
		MApi_GFX_SetDFBBldOP((GFX_DFBBldOP) dfb.eSrcDFBBldOP,
				     (GFX_DFBBldOP) dfb.eDstDFBBldOP);
		_fill_utp_color(&color, &dfb.color);
		MApi_GFX_SetDFBBldConstColor(color);
	}

	return 0;
}

int g2d_set_ext_ctrl(struct v4l2_ext_control *ext_ctrl)
{
	struct v4l2_ext_g2d_config config;

	if (copy_from_user(&config, (void __user *)ext_ctrl->ptr,
		       sizeof(struct v4l2_ext_g2d_config))) {
		G2D_ERROR("copy_from_user fail\n");
		return -1;
	}

	switch (config.type) {
	case V4L2_G2D_VCMDQ:
		_g2d_set_vcmdq(config.vcmdq);
		break;
	case V4L2_G2D_FLUSHQUEUE:
		MApi_GFX_FlushQueue();
		break;
	case V4L2_G2D_INTENSITY:
		MApi_GFX_SetIntensity(config.intensity.id, _utp_fmt_convert(config.intensity.fmt),
				      (MS_U32 *) &config.intensity.color);
		break;
	case V4L2_G2D_PALETTEOPT:
		_g2d_set_paletteopt(config.palette);
		break;
	case V4L2_G2D_COLORKEY:
		_g2d_set_colorkey(config.colorkey);
		break;
	case V4L2_G2D_ROP2:
		G2D_DEBUG("Set ROP2 enable = %d, mode = %d\n",
			config.rop2.enable, config.rop2.eRopMode);
		MApi_GFX_SetROP2((MS_BOOL) config.rop2.enable, (GFX_ROP2_Op) config.rop2.eRopMode);
		break;
	case V4L2_G2D_MIRROR:
		G2D_DEBUG("Set Mirror H = %d, V = %d\n",
			config.mirror.HMirror, config.mirror.VMirror);
		MApi_GFX_SetMirror((MS_BOOL) config.mirror.HMirror,
				   (MS_BOOL) config.mirror.VMirror);
		break;
	case V4L2_G2D_DSTMIRROR:
		G2D_DEBUG("Set Mirror H = %d, V = %d\n",
			config.mirror.HMirror, config.mirror.VMirror);
		MApi_GFX_SetDstMirror((MS_BOOL) config.mirror.HMirror,
				      (MS_BOOL) config.mirror.VMirror);
		break;
	case V4L2_G2D_DITHER:
		G2D_DEBUG("Set dither enable = %d\n", config.enable);
		MApi_GFX_SetDither((MS_BOOL) config.enable);
		break;
	case V4L2_G2D_NEARESTMODE:
		G2D_DEBUG("Set nearest enable = %d\n", config.enable);
		MApi_GFX_SetNearestMode((MS_BOOL) config.enable);
		break;
	case V4L2_G2D_STRBLT_CK:
		_g2d_set_strblt_ck(config.strbltck);
		break;
	case V4L2_G2D_DC_CSC_FMT:
		G2D_DEBUG("CSCmode = %d, outRange = %d, inRange = %d, srcfmt = %d, dstfmt = %d\n",
			config.cscfmt.mode, config.cscfmt.outRange, config.cscfmt.inRange,
			config.cscfmt.srcfmt, config.cscfmt.dstfmt);
		MApi_GFX_SetDC_CSC_FMT((GFX_YUV_Rgb2Yuv) config.cscfmt.mode,
				       (GFX_YUV_OutRange) config.cscfmt.outRange,
				       (GFX_YUV_InRange) config.cscfmt.inRange,
				       (GFX_YUV_422) config.cscfmt.srcfmt,
				       (GFX_YUV_422) config.cscfmt.dstfmt);
		break;
	case V4L2_G2D_ALPHA_CMP:
		G2D_DEBUG("Set alphacmp enable = %d, mode = %d\n",
			config.alphacmp.enable, config.alphacmp.eCmpOp);
		MApi_GFX_SetAlphaCmp((MS_BOOL) config.alphacmp.enable,
				     (GFX_ACmpOp) config.alphacmp.eCmpOp);
		break;
	case V4L2_G2D_ABL:
		_g2d_set_abl(config.abl);
		break;
	case V4L2_G2D_DFB:
		_g2d_set_dfb(config.dfb);
		break;
	default:
		break;
	}
	return 0;
}

int g2d_set_rotate(u32 u32Rotate)
{
	G2D_DEBUG("rotate = %u\n", u32Rotate);

	switch (u32Rotate) {
	case 0:
		MApi_GFX_SetRotate(GEROTATE_0);
		break;
	case 90:
		MApi_GFX_SetRotate(GEROTATE_90);
		break;
	case 180:
		MApi_GFX_SetRotate(GEROTATE_180);
		break;
	case 270:
		MApi_GFX_SetRotate(GEROTATE_270);
		break;
	}

	return 0;
}

int g2d_set_powerstate(u32 state)
{
	if (GFXStr(state, NULL) != UTOPIA_STATUS_SUCCESS)
		return -1;

	return 0;
}
EXPORT_SYMBOL(g2d_set_powerstate);

int g2d_get_crc(u32 *CRCvalue)
{
	_MApi_GE_GetCRC(CRCvalue);

	return 0;
}
