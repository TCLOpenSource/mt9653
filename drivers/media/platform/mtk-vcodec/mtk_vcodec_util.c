// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
* Copyright (c) 2016 MediaTek Inc.
* Author: PC Chen <pc.chen@mediatek.com>
*	Tiffany Lin <tiffany.lin@mediatek.com>
*/

#include <linux/module.h>
#include <linux/dma-resv.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>

#include "mtk_vcodec_fence.h"
#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_util.h"
#if IS_ENABLED(CONFIG_VIDEO_MEDIATEK_VCU)
#include "mtk_vcu.h"
#endif
#include "mtk_vcodec_dec.h"

/* For encoder, this will enable logs in venc/*/
bool mtk_vcodec_dbg;
EXPORT_SYMBOL(mtk_vcodec_dbg);

/* For vcodec performance measure */
bool mtk_vcodec_perf;
EXPORT_SYMBOL(mtk_vcodec_perf);

/* The log level of v4l2 encoder or decoder driver.
 * That is, files under mtk-vcodec/.
 */
int mtk_v4l2_dbg_level;
EXPORT_SYMBOL(mtk_v4l2_dbg_level);

void __iomem *mtk_vcodec_get_dec_reg_addr(struct mtk_vcodec_ctx *data,
	unsigned int reg_idx)
{
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)data;

	if (!data || reg_idx >= NUM_MAX_VDEC_REG_BASE) {
		mtk_v4l2_err("Invalid arguments, reg_idx=%d", reg_idx);
		return NULL;
	}
	return ctx->dev->dec_reg_base[reg_idx];
}
EXPORT_SYMBOL(mtk_vcodec_get_dec_reg_addr);

void __iomem *mtk_vcodec_get_enc_reg_addr(struct mtk_vcodec_ctx *data,
	unsigned int reg_idx)
{
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)data;

	if (!data || reg_idx >= NUM_MAX_VENC_REG_BASE) {
		mtk_v4l2_err("Invalid arguments, reg_idx=%d", reg_idx);
		return NULL;
	}
	return ctx->dev->enc_reg_base[reg_idx];
}
EXPORT_SYMBOL(mtk_vcodec_get_enc_reg_addr);


int mtk_vcodec_mem_alloc(struct mtk_vcodec_ctx *data,
						 struct mtk_vcodec_mem *mem)
{
	unsigned long size = mem->size;
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)data;
	struct device *dev = &ctx->dev->plat_dev->dev;

	mem->va = dma_alloc_coherent(dev, size, &mem->dma_addr, GFP_KERNEL);

	if (!mem->va) {
		mtk_v4l2_err("%s dma_alloc size=%ld failed!", dev_name(dev),
					 size);
		return -ENOMEM;
	}

	memset(mem->va, 0, size);

	mtk_v4l2_debug(4, "[%d]  - va      = %p", ctx->id, mem->va);
	mtk_v4l2_debug(4, "[%d]  - dma     = 0x%lx", ctx->id,
				   (unsigned long)mem->dma_addr);
	mtk_v4l2_debug(4, "[%d]    size = 0x%lx", ctx->id, size);

	return 0;
}
EXPORT_SYMBOL(mtk_vcodec_mem_alloc);

void mtk_vcodec_mem_free(struct mtk_vcodec_ctx *data,
						 struct mtk_vcodec_mem *mem)
{
	unsigned long size = mem->size;
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)data;
	struct device *dev = &ctx->dev->plat_dev->dev;

	if (!mem->va) {
		mtk_v4l2_err("%s dma_free size=%ld failed!", dev_name(dev),
					 size);
		return;
	}

	mtk_v4l2_debug(4, "[%d]  - va      = %p", ctx->id, mem->va);
	mtk_v4l2_debug(4, "[%d]  - dma     = 0x%lx", ctx->id,
				   (unsigned long)mem->dma_addr);
	mtk_v4l2_debug(4, "[%d]    size = 0x%lx", ctx->id, size);

	dma_free_coherent(dev, size, mem->va, mem->dma_addr);
	mem->va = NULL;
	mem->dma_addr = 0;
	mem->size = 0;
}
EXPORT_SYMBOL(mtk_vcodec_mem_free);

void mtk_vcodec_set_curr_ctx(struct mtk_vcodec_dev *dev,
	struct mtk_vcodec_ctx *ctx, unsigned int hw_id)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->irqlock, flags);
	dev->curr_dec_ctx[hw_id] = ctx;
	spin_unlock_irqrestore(&dev->irqlock, flags);
}
EXPORT_SYMBOL(mtk_vcodec_set_curr_ctx);

struct mtk_vcodec_ctx *mtk_vcodec_get_curr_ctx(struct mtk_vcodec_dev *dev,
	unsigned int hw_id)
{
	unsigned long flags;
	struct mtk_vcodec_ctx *ctx;

	spin_lock_irqsave(&dev->irqlock, flags);
	ctx = dev->curr_dec_ctx[hw_id];
	spin_unlock_irqrestore(&dev->irqlock, flags);
	return ctx;
}
EXPORT_SYMBOL(mtk_vcodec_get_curr_ctx);

bool flush_abort_state(enum mtk_instance_state state)
{
	return state == MTK_STATE_ABORT || state == MTK_STATE_FLUSH;
}
EXPORT_SYMBOL(flush_abort_state);

void mtk_vcodec_init_slice_info(struct mtk_vcodec_ctx *ctx, struct mtk_video_dec_buf *dst_buf_info)
{
	struct vb2_v4l2_buffer *dst_buf = &dst_buf_info->vb;
	struct vdec_fb *pfb = &dst_buf_info->frame_buffer;
	struct dma_buf *dbuf = dst_buf->vb2_buf.planes[0].dbuf;
	struct dma_fence *fence;

	pfb->slice_done_count = 0;
	fence = mtk_vcodec_create_fence(ctx->dec_params.slice_count);
	dma_resv_add_excl_fence(dbuf->resv, fence);
	dma_fence_put(fence); // make dbuf->resv the only owner
}
EXPORT_SYMBOL(mtk_vcodec_init_slice_info);

struct vdec_fb *mtk_vcodec_get_fb(struct mtk_vcodec_ctx *ctx)
{
	struct vb2_buffer *dst_buf = NULL;
	struct vdec_fb *pfb;
	struct mtk_video_dec_buf *dst_buf_info;
	struct vb2_v4l2_buffer *dst_vb2_v4l2;
	int i;

	if (!ctx) {
		mtk_v4l2_err("Ctx is NULL!");
		return NULL;
	}

	mtk_v4l2_debug_enter();
	dst_vb2_v4l2 = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (dst_vb2_v4l2 != NULL)
		dst_buf = &dst_vb2_v4l2->vb2_buf;
	if (dst_buf != NULL) {
		dst_buf_info = container_of(
			dst_vb2_v4l2, struct mtk_video_dec_buf, vb);
		pfb = &dst_buf_info->frame_buffer;
		pfb->num_planes = dst_vb2_v4l2->vb2_buf.num_planes;
		pfb->index = dst_buf->index;

		for (i = 0; i < dst_vb2_v4l2->vb2_buf.num_planes; i++) {
#ifdef TV_INTEGRATION
			if (i == dst_buf->num_planes - 1) /* meta */
				pfb->fb_base[i].va = vb2_plane_vaddr(dst_buf, i);
			else
				pfb->fb_base[i].va = 0; /* to speed up */
#else
			pfb->fb_base[i].va = vb2_plane_vaddr(dst_buf, i);
#endif
#ifdef CONFIG_VB2_MEDIATEK_DMA_SG
			pfb->fb_base[i].dma_addr =
				mtk_vb2_dma_contig_plane_dma_addr(dst_buf, i);
#else
			pfb->fb_base[i].dma_addr =
				vb2_dma_contig_plane_dma_addr(dst_buf, i);
#endif
			pfb->fb_base[i].size = ctx->picinfo.fb_sz[i];
			pfb->fb_base[i].length = dst_buf->planes[i].length;
			pfb->fb_base[i].dmabuf = dst_buf->planes[i].dbuf;
		}
		mtk_vcodec_init_slice_info(ctx, dst_buf_info);
		pfb->status = 0;
		mtk_v4l2_debug(1, "[%d] id=%d pfb=0x%p VA=%p dma_addr[0]=%p dma_addr[1]=%p Size=%zx fd:%x, dma_general_buf = %p, general_buf_fd = %d",
				ctx->id, dst_buf->index, pfb,
				pfb->fb_base[0].va,
				&pfb->fb_base[0].dma_addr,
				&pfb->fb_base[1].dma_addr,
				pfb->fb_base[0].size,
				dst_buf->planes[0].m.fd,
				pfb->dma_general_buf,
				pfb->general_buf_fd);

		mutex_lock(&ctx->buf_lock);
		dst_buf_info->used = true;
		mutex_unlock(&ctx->buf_lock);
		dst_vb2_v4l2 = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		if (dst_vb2_v4l2 != NULL)
			dst_buf = &dst_vb2_v4l2->vb2_buf;
			mtk_v4l2_debug(VCODEC_DBG_L8, "[%d] index=%d, num_rdy_bufs=%d\n",
				ctx->id, dst_buf->index,
				v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx));
	} else {
		mtk_v4l2_debug(VCODEC_DBG_L8, "[%d] No free framebuffer in v4l2!!\n", ctx->id);
		pfb = NULL;
	}
	mtk_v4l2_debug_leave();

	return pfb;
}
EXPORT_SYMBOL(mtk_vcodec_get_fb);

int mtk_dma_sync_sg_range(const struct sg_table *sgt,
	struct device *dev, unsigned int size,
	enum dma_data_direction direction)
{
	struct sg_table *sgt_tmp;
	struct scatterlist *s_sgl, *d_sgl;
	unsigned int contig_size = 0;
	int ret, i;

	sgt_tmp = kzalloc(sizeof(*sgt_tmp), GFP_KERNEL);
	if (!sgt_tmp)
		return -1;

	ret = sg_alloc_table(sgt_tmp, sgt->orig_nents, GFP_KERNEL);
	if (ret) {
		mtk_v4l2_debug(0, "sg alloc table failed %d.\n", ret);
		kfree(sgt_tmp);
		return -1;
	}
	sgt_tmp->nents = 0;
	d_sgl = sgt_tmp->sgl;

	for_each_sg(sgt->sgl, s_sgl, sgt->orig_nents, i) {
		mtk_v4l2_debug(VCODEC_DBG_L6, "%d contig_size %d bytesused %d.\n",
			i, contig_size, size);
		if (contig_size >= size)
			break;
		memcpy(d_sgl, s_sgl, sizeof(*s_sgl));
		contig_size += sg_dma_len(s_sgl);
		d_sgl = sg_next(d_sgl);
		sgt_tmp->nents++;
	}
	if (direction == DMA_TO_DEVICE) {
		dma_sync_sg_for_device(dev, sgt_tmp->sgl, sgt_tmp->nents, direction);
	} else if (direction == DMA_FROM_DEVICE) {
		dma_sync_sg_for_cpu(dev, sgt_tmp->sgl, sgt_tmp->nents, direction);
	} else {
		mtk_v4l2_debug(0, "direction %d not correct\n", direction);
		sg_free_table(sgt_tmp);
		kfree(sgt_tmp);
		return -1;
	}
	mtk_v4l2_debug(4, "flush nents %d total nents %d\n",
		sgt_tmp->nents, sgt->orig_nents);
	sg_free_table(sgt_tmp);
	kfree(sgt_tmp);

	return 0;
}
EXPORT_SYMBOL(mtk_dma_sync_sg_range);

void v4l_fill_mtk_fmtdesc(struct v4l2_fmtdesc *fmt)
{
	const char *descr = NULL;
	const unsigned int sz = sizeof(fmt->description);

	switch (fmt->pixelformat) {
	case V4L2_PIX_FMT_HEIF:
		descr = "HEIF"; break;
	case V4L2_PIX_FMT_S263:
		descr = "S.263"; break;
	case V4L2_PIX_FMT_RV30:
		descr = "RealVideo 8"; break;
	case V4L2_PIX_FMT_RV40:
		descr = "RealVideo 9/10"; break;
	case V4L2_PIX_FMT_AV1:
		descr = "AV1"; break;
	case V4L2_PIX_FMT_AVS:
		descr = "AVS"; break;
	case V4L2_PIX_FMT_AVS2:
		descr = "AVS2"; break;
	case V4L2_PIX_FMT_AVS3:
		descr = "AVS3"; break;
	case V4L2_PIX_FMT_HEVCDV:
		descr = "HEVC Dolby Vision"; break;
	case V4L2_PIX_FMT_AVCDV:
		descr = "AVC Dolby Vision"; break;
	case V4L2_PIX_FMT_SHVC:
		descr = "Scalable HEVC"; break;
	case V4L2_PIX_FMT_AVIF:
		descr = "AVIF"; break;
	case V4L2_PIX_FMT_H266:
		descr = "H.266"; break;
	case V4L2_PIX_FMT_MT21M:
		descr = "Mediatek yuv420 with meta"; break;
	default:
	    mtk_v4l2_debug(0, "MTK Unknown pixelformat 0x%08x\n", fmt->pixelformat);
	}

	if (descr)
		strscpy(fmt->description, descr, sz);

}
EXPORT_SYMBOL_GPL(v4l_fill_mtk_fmtdesc);

void mtk_vcodec_add_ctx_list(struct mtk_vcodec_ctx *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->dev->ctx_lock, flags);
	list_add(&ctx->list, &ctx->dev->ctx_list);
	spin_unlock_irqrestore(&ctx->dev->ctx_lock, flags);
}
EXPORT_SYMBOL_GPL(mtk_vcodec_add_ctx_list);

void mtk_vcodec_del_ctx_list(struct mtk_vcodec_ctx *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->dev->ctx_lock, flags);
	list_del_init(&ctx->list);
	spin_unlock_irqrestore(&ctx->dev->ctx_lock, flags);
}
EXPORT_SYMBOL_GPL(mtk_vcodec_del_ctx_list);

MODULE_LICENSE("GPL");
