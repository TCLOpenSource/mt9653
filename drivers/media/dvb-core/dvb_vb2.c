// SPDX-License-Identifier: GPL-2.0
/*
 * dvb-vb2.c - dvb-vb2
 *
 * Copyright (C) 2015 Samsung Electronics
 *
 * Author: jh1009.sung@samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#ifdef CONFIG_ARCH_MEDIATEK_DTV
#include <linux/dma-buf.h>
#endif

#include <media/dvbdev.h>
#ifdef CONFIG_ARCH_MEDIATEK_DTV
#include <media/dmxdev.h>
#endif
#include <media/dvb_vb2.h>

#define DVB_V2_MAX_SIZE		(4096 * 188)

static int vb2_debug;
module_param(vb2_debug, int, 0644);

#define dprintk(level, fmt, arg...)						  \
	do {									  \
		if (vb2_debug >= level)						  \
			pr_info("vb2: %s: " fmt, __func__, ## arg); \
	} while (0)

static int _queue_setup(struct vb2_queue *vq,
			unsigned int *nbuffers, unsigned int *nplanes,
			unsigned int sizes[], struct device *alloc_devs[])
{
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	int i;
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vq);

	ctx->buf_cnt = *nbuffers;
	*nplanes = ctx->plane_depth;

	for (i = 0; i < *nplanes; i++)
		sizes[i] = ctx->buf_siz[i];

	/*
	 * videobuf2-vmalloc allocator is context-less so no need to set
	 * alloc_ctxs array.
	 */
	dprintk(3, "[%s] count=%d\n", ctx->name, *nbuffers);
	for (i = 0; i < *nplanes; i++)
		dprintk(3, "size[%d]=%d\n", i, sizes[i]);
	dprintk(3, "\n");

	return 0;
#else
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vq);

	ctx->buf_cnt = *nbuffers;
	*nplanes = 1;
	sizes[0] = ctx->buf_siz;

	/*
	 * videobuf2-vmalloc allocator is context-less so no need to set
	 * alloc_ctxs array.
	 */

	dprintk(3, "[%s] count=%d, size=%d\n", ctx->name,
		*nbuffers, sizes[0]);

	return 0;
#endif
}

static int _buffer_prepare(struct vb2_buffer *vb)
{
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size;
	int i;

	for (i = 0; i < vb->num_planes; i++) {
		size = ctx->buf_siz[i];

		if (vb2_plane_size(vb, i) < size) {
			dprintk(1, "[%s] data will not fit into plane (%lu < %lu)\n",
				ctx->name, vb2_plane_size(vb, i), size);
			return -EINVAL;
		}

		vb2_set_plane_payload(vb, i, size);
	}

	dprintk(3, "[%s]\n", ctx->name);

	return 0;
#else
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size = ctx->buf_siz;

	if (vb2_plane_size(vb, 0) < size) {
		dprintk(1, "[%s] data will not fit into plane (%lu < %lu)\n",
			ctx->name, vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	vb2_set_plane_payload(vb, 0, size);
	dprintk(3, "[%s]\n", ctx->name);

	return 0;
#endif
}

static void _buffer_queue(struct vb2_buffer *vb)
{
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct dvb_buffer *buf = container_of(vb, struct dvb_buffer, vb);
	unsigned long flags = 0;

	spin_lock_irqsave(&ctx->slock, flags);
	list_add_tail(&buf->list, &ctx->dvb_q);
	spin_unlock_irqrestore(&ctx->slock, flags);

	dprintk(3, "[%s]\n", ctx->name);
}

static int _start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vq);

	dprintk(3, "[%s] count=%d\n", ctx->name, count);
	return 0;
}

static void _stop_streaming(struct vb2_queue *vq)
{
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vq);
	struct dvb_buffer *buf;
	unsigned long flags = 0;

	dprintk(3, "[%s]\n", ctx->name);

	spin_lock_irqsave(&ctx->slock, flags);
	while (!list_empty(&ctx->dvb_q)) {
		buf = list_entry(ctx->dvb_q.next,
				 struct dvb_buffer, list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
		list_del(&buf->list);
	}
	spin_unlock_irqrestore(&ctx->slock, flags);
}

static void _dmxdev_lock(struct vb2_queue *vq)
{
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vq);
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	struct dmxdev *dmxdev;
	struct dmxdev_filter *dmxdevfilter;
#endif

	mutex_lock(&ctx->mutex);

#ifdef CONFIG_ARCH_MEDIATEK_DTV
	if (!strncmp(ctx->name, "dvr", DVB_VB2_NAME_MAX)) {
		dmxdev = container_of(ctx, struct dmxdev, dvr_vb2_ctx);

		mutex_lock(&dmxdev->mutex);
	} else if (!strncmp(ctx->name, "demux_filter", DVB_VB2_NAME_MAX)) {
		dmxdevfilter = container_of(ctx, struct dmxdev_filter, vb2_ctx);
		dmxdev = dmxdevfilter->dev;

		mutex_lock(&dmxdevfilter->mutex);
		mutex_lock(&dmxdev->mutex);
	}
#endif

	dprintk(3, "[%s]\n", ctx->name);
}

static void _dmxdev_unlock(struct vb2_queue *vq)
{
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vq);
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	struct dmxdev *dmxdev;
	struct dmxdev_filter *dmxdevfilter;
#endif

	if (mutex_is_locked(&ctx->mutex))
		mutex_unlock(&ctx->mutex);

#ifdef CONFIG_ARCH_MEDIATEK_DTV
	if (!strncmp(ctx->name, "dvr", DVB_VB2_NAME_MAX)) {
		dmxdev = container_of(ctx, struct dmxdev, dvr_vb2_ctx);

		if (mutex_is_locked(&dmxdev->mutex))
			mutex_unlock(&dmxdev->mutex);
	} else if (!strncmp(ctx->name, "demux_filter", DVB_VB2_NAME_MAX)) {
		dmxdevfilter = container_of(ctx, struct dmxdev_filter, vb2_ctx);
		dmxdev = dmxdevfilter->dev;

		if (mutex_is_locked(&dmxdevfilter->mutex))
			mutex_unlock(&dmxdevfilter->mutex);

		if (mutex_is_locked(&dmxdev->mutex))
			mutex_unlock(&dmxdev->mutex);
	}
#endif

	dprintk(3, "[%s]\n", ctx->name);
}

static const struct vb2_ops dvb_vb2_qops = {
	.queue_setup		= _queue_setup,
	.buf_prepare		= _buffer_prepare,
	.buf_queue		= _buffer_queue,
	.start_streaming	= _start_streaming,
	.stop_streaming		= _stop_streaming,
	.wait_prepare		= _dmxdev_unlock,
	.wait_finish		= _dmxdev_lock,
};

static void _fill_dmx_buffer(struct vb2_buffer *vb, void *pb)
{
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct dmx_buffer *b = pb;

	b->index = vb->index;

	if (ctx->type == DVB_BUF_TYPE_CAPTURE_MPLANE) {
		struct dmx_plane plane;
		int i;

		b->length = vb->num_planes;

		for (i = 0; i < vb->num_planes; i++) {
			memset(&plane, 0, sizeof(plane));

			plane.length = vb->planes[i].length;
			plane.bytesused = vb->planes[i].bytesused;

			if (ctx->memory & VB2_MEMORY_DMABUF)
				plane.m.fd = vb->planes[i].m.fd;
			else
				plane.m.mem_offset = vb->planes[i].m.offset;

			if (copy_to_user((void __user *)&b->planes[i], &plane, sizeof(plane)))
				return;
		}
	} else {
		b->length = vb->planes[0].length;
		b->bytesused = vb->planes[0].bytesused;

		if (ctx->memory & VB2_MEMORY_DMABUF)
			b->fd = vb->planes[0].m.fd;
		else
			b->offset = vb->planes[0].m.offset;
	}

	dprintk(3, "[%s]\n", ctx->name);
#else
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct dmx_buffer *b = pb;

	b->index = vb->index;
	b->length = vb->planes[0].length;
	b->bytesused = vb->planes[0].bytesused;
	b->offset = vb->planes[0].m.offset;
	dprintk(3, "[%s]\n", ctx->name);
#endif
}

#ifdef CONFIG_ARCH_MEDIATEK_DTV
static int _fill_vb2_buffer_from_user(struct vb2_queue *q, void *pb)
{
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(q);
	struct vb2_buffer *vb;
	struct dvb_buffer *buf;
	struct dmx_buffer *b = pb;
	int i;

	if (b->index >= q->num_buffers) {
		dprintk(1, "[%s] index out-of-range\n", ctx->name);
		return -EINVAL;
	}

	vb = q->bufs[b->index];
	if (!vb) {
		dprintk(1, "[%s] vb is NULL\n", ctx->name);
		return -EINVAL;
	}

	if ((ctx->type == DVB_BUF_TYPE_CAPTURE_MPLANE) && (b->length != vb->num_planes)) {
		dprintk(1, "[%s] incorrect planes array length\n", ctx->name);
		return -EINVAL;
	}

	buf = container_of(vb, struct dvb_buffer, vb);

	if (ctx->type == DVB_BUF_TYPE_CAPTURE_MPLANE) {
		struct dmx_plane *planes;
		int ret;

		planes = kcalloc(b->length, sizeof(struct dmx_plane), GFP_KERNEL);
		if (!planes) {
			dprintk(1, "[%s] kcalloc fail\n", ctx->name);
			return -ENOMEM;
		}

		ret = copy_from_user(planes, b->planes, b->length * sizeof(struct dmx_plane));
		if (ret) {
			dprintk(1, "[%s] copy_from_user fail\n", ctx->name);
			kfree(planes);
			return -EFAULT;
		}

		memset(&buf->planes[0], 0, b->length * sizeof(struct vb2_plane));

		for (i = 0; i < b->length; i++) {
			buf->planes[i].m.fd = planes[i].m.fd;
			buf->planes[i].length = planes[i].length;
			buf->planes[i].bytesused = 0;
			buf->planes[i].data_offset = 0;
		}

		kfree(planes);
	} else {
		buf->planes[0].m.fd = b->fd;
		buf->planes[0].length = b->length;
		buf->planes[0].bytesused = 0;
		buf->planes[0].data_offset = 0;
	}

	return 0;
}
#endif

static int _fill_vb2_buffer(struct vb2_buffer *vb, struct vb2_plane *planes)
{
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct dvb_buffer *buf = container_of(vb, struct dvb_buffer, vb);
	int i;

	if (ctx->memory & VB2_MEMORY_DMABUF) {
		for (i = 0; i < vb->num_planes; i++) {
			planes[i].m.fd = buf->planes[i].m.fd;
			planes[i].length = buf->planes[i].length;
			planes[i].bytesused = buf->planes[i].bytesused;
			planes[i].data_offset = buf->planes[i].data_offset;
		}
	} else {
		for (i = 0; i < vb->num_planes; i++)
			planes[i].bytesused = 0;
	}

	dprintk(3, "[%s]\n", ctx->name);
	return 0;
#else
	struct dvb_vb2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	planes[0].bytesused = 0;
	dprintk(3, "[%s]\n", ctx->name);

	return 0;
#endif
}

static const struct vb2_buf_ops dvb_vb2_buf_ops = {
	.fill_user_buffer	= _fill_dmx_buffer,
	.fill_vb2_buffer	= _fill_vb2_buffer,
};

/*
 * Videobuf operations
 */
int dvb_vb2_init(struct dvb_vb2_ctx *ctx, const char *name, int nonblocking)
{
	struct vb2_queue *q = &ctx->vb_q;
	int ret;

	memset(ctx, 0, sizeof(struct dvb_vb2_ctx));
	q->type = DVB_BUF_TYPE_CAPTURE;
	/**capture type*/
	q->is_output = 0;
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	/**support mmap and dmabuf scheme*/
	q->io_modes = VB2_DMABUF | VB2_MMAP;
#else
	/**only mmap is supported currently*/
	q->io_modes = VB2_MMAP;
#endif
	q->drv_priv = ctx;
	q->buf_struct_size = sizeof(struct dvb_buffer);
	q->min_buffers_needed = 1;
	q->ops = &dvb_vb2_qops;
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	q->mem_ops = &vb2_dma_sg_memops;
#else
	q->mem_ops = &vb2_vmalloc_memops;
#endif
	q->buf_ops = &dvb_vb2_buf_ops;
	q->num_buffers = 0;
	ret = vb2_core_queue_init(q);
	if (ret) {
		ctx->state = DVB_VB2_STATE_NONE;
		dprintk(1, "[%s] errno=%d\n", ctx->name, ret);
		return ret;
	}

	mutex_init(&ctx->mutex);
	spin_lock_init(&ctx->slock);
	INIT_LIST_HEAD(&ctx->dvb_q);

	strscpy(ctx->name, name, DVB_VB2_NAME_MAX);
	ctx->nonblocking = nonblocking;
	ctx->state = DVB_VB2_STATE_INIT;

	dprintk(3, "[%s]\n", ctx->name);

	return 0;
}

int dvb_vb2_release(struct dvb_vb2_ctx *ctx)
{
	struct vb2_queue *q = (struct vb2_queue *)&ctx->vb_q;

	if (ctx->state & DVB_VB2_STATE_INIT)
		vb2_core_queue_release(q);

	ctx->state = DVB_VB2_STATE_NONE;
	dprintk(3, "[%s]\n", ctx->name);

	return 0;
}

int dvb_vb2_stream_on(struct dvb_vb2_ctx *ctx)
{
	struct vb2_queue *q = &ctx->vb_q;
	int ret;

	ret = vb2_core_streamon(q, q->type);
	if (ret) {
		ctx->state = DVB_VB2_STATE_NONE;
		dprintk(1, "[%s] errno=%d\n", ctx->name, ret);
		return ret;
	}
	ctx->state |= DVB_VB2_STATE_STREAMON;
	dprintk(3, "[%s]\n", ctx->name);

	return 0;
}

int dvb_vb2_stream_off(struct dvb_vb2_ctx *ctx)
{
	struct vb2_queue *q = (struct vb2_queue *)&ctx->vb_q;
	int ret;

	ctx->state &= ~DVB_VB2_STATE_STREAMON;
	ret = vb2_core_streamoff(q, q->type);
	if (ret) {
		ctx->state = DVB_VB2_STATE_NONE;
		dprintk(1, "[%s] errno=%d\n", ctx->name, ret);
		return ret;
	}
	dprintk(3, "[%s]\n", ctx->name);

	return 0;
}

int dvb_vb2_is_streaming(struct dvb_vb2_ctx *ctx)
{
	return (ctx->state & DVB_VB2_STATE_STREAMON);
}
#ifdef CONFIG_ARCH_MEDIATEK_DTV
EXPORT_SYMBOL(dvb_vb2_is_streaming);
#endif

#ifdef CONFIG_ARCH_MEDIATEK_DTV
static int _map_dmabuf_va(struct dma_buf *db, void **va, u64 *size)
{
	int ret = 0;

	if ((db == NULL) || (va == NULL) || (size == NULL)) {
		dprintk(1, "[%s] invalid parameters\n", __func__);
		return -EINVAL;
	}

	*size = db->size;

	ret = dma_buf_begin_cpu_access(db, DMA_BIDIRECTIONAL);
	if (ret < 0) {
		dprintk(1, "[%s] dma_buf_begin_cpu_access fail\n", __func__);
		return ret;
	}

	*va = dma_buf_vmap(db);
	if ((*va == NULL) || (*va == -1)) {
		dprintk(1, "[%s] dma_buf_kmap fail\n", __func__);
		dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
		return -EFAULT;
	}

	dprintk(1, "[%s] va=0x%llx, size=%llu\n", __func__, *va, *size);
	return 0;
}

static int _unmap_dmabuf_va(struct dma_buf *db, void *va)
{
	if ((db == NULL) || (va == NULL)) {
		dprintk(1, "[%s] invalid parameters\n", __func__);
		return -EINVAL;
	}

	dma_buf_vunmap(db, va);
	dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);

	return 0;
}
#endif

int dvb_vb2_fill_buffer(struct dvb_vb2_ctx *ctx,
			const unsigned char *src, int len,
			enum dmx_buffer_flags *buffer_flags)
{
	unsigned long flags = 0;
	void *vbuf = NULL;
	int todo = len;
	unsigned char *psrc = (unsigned char *)src;
	int ll = 0;

	/*
	 * normal case: This func is called twice from demux driver
	 * one with valid src pointer, second time with NULL pointer
	 */
	if (!src || !len)
		return 0;
	spin_lock_irqsave(&ctx->slock, flags);
	if (buffer_flags && *buffer_flags) {
		ctx->flags |= *buffer_flags;
		*buffer_flags = 0;
	}
	while (todo) {
		if (!ctx->buf) {
			if (list_empty(&ctx->dvb_q)) {
				dprintk(3, "[%s] Buffer overflow!!!\n",
					ctx->name);
				break;
			}

			ctx->buf = list_entry(ctx->dvb_q.next,
						  struct dvb_buffer, list);
		#ifdef CONFIG_ARCH_MEDIATEK_DTV
			ctx->remain[0] = vb2_plane_size(&ctx->buf->vb, 0);
			ctx->offset[0] = 0;
		#else
			ctx->remain = vb2_plane_size(&ctx->buf->vb, 0);
			ctx->offset = 0;
		#endif
		}

		if (!dvb_vb2_is_streaming(ctx)) {
			vb2_buffer_done(&ctx->buf->vb, VB2_BUF_STATE_ERROR);
			list_del(&ctx->buf->list);
			ctx->buf = NULL;
			break;
		}

		/* Fill buffer */
	#ifdef CONFIG_ARCH_MEDIATEK_DTV
		ll = min(todo, ctx->remain[0]);

		if (ctx->memory & VB2_MEMORY_DMABUF) {
			struct dma_buf *db = ctx->buf->vb.planes[0].dbuf;
			void *va = NULL;
			u64 size = 0;

			if (_map_dmabuf_va(db, &va, &size) < 0) {
				dprintk(1, "[%s] map dmabuf fail!!!\n", ctx->name);
				break;
			}

			vbuf = va;
		} else
			vbuf = vb2_plane_vaddr(&ctx->buf->vb, 0);

		memcpy(vbuf + ctx->offset[0], psrc, ll);

		if (ctx->memory & VB2_MEMORY_DMABUF) {
			struct dma_buf *db = ctx->buf->vb.planes[0].dbuf;

			_unmap_dmabuf_va(db, vbuf);
		}

		todo -= ll;
		psrc += ll;

		ctx->remain[0] -= ll;
		ctx->offset[0] += ll;
	#else
		ll = min(todo, ctx->remain);
		vbuf = vb2_plane_vaddr(&ctx->buf->vb, 0);
		memcpy(vbuf + ctx->offset, psrc, ll);
		todo -= ll;
		psrc += ll;

		ctx->remain -= ll;
		ctx->offset += ll;
	#endif

	#ifdef CONFIG_ARCH_MEDIATEK_DTV
		if (ctx->remain[0] == 0) {
	#else
		if (ctx->remain == 0) {
	#endif
			vb2_buffer_done(&ctx->buf->vb, VB2_BUF_STATE_DONE);
			list_del(&ctx->buf->list);
			ctx->buf = NULL;
		}
	}

	if (ctx->nonblocking && ctx->buf) {
		vb2_set_plane_payload(&ctx->buf->vb, 0, ll);
		vb2_buffer_done(&ctx->buf->vb, VB2_BUF_STATE_DONE);
		list_del(&ctx->buf->list);
		ctx->buf = NULL;
	}
	spin_unlock_irqrestore(&ctx->slock, flags);

	if (todo)
		dprintk(1, "[%s] %d bytes are dropped.\n", ctx->name, todo);
	else
		dprintk(3, "[%s]\n", ctx->name);

	dprintk(3, "[%s] %d bytes are copied\n", ctx->name, len - todo);
	return (len - todo);
}
#ifdef CONFIG_ARCH_MEDIATEK_DTV
EXPORT_SYMBOL(dvb_vb2_fill_buffer);
#endif

#ifdef CONFIG_ARCH_MEDIATEK_DTV
int dvb_vb2_set_plane_attr(struct dvb_vb2_ctx *ctx,
			unsigned int plane_no,
			enum dvb_vb2_plane_attrs attrs)
{
	unsigned long flags = 0;

	if (plane_no >= ctx->plane_depth) {
		dprintk(1, "[%s] plane no. out-of-range\n", ctx->name);
		return -EINVAL;
	}

	spin_lock_irqsave(&ctx->slock, flags);
	ctx->plane_attr[plane_no] = attrs;
	spin_unlock_irqrestore(&ctx->slock, flags);
	return 0;
}
EXPORT_SYMBOL(dvb_vb2_set_plane_attr);

static inline bool _is_vb2_plane_wo_copy(struct dvb_vb2_ctx *ctx, unsigned int plane_no)
{
	return (ctx->plane_attr[plane_no] & DVB_VB2_PLANE_ATTR_WO_COPY) ? true : false;
}

static inline bool _is_vb2_plane_nonblocking(struct dvb_vb2_ctx *ctx, unsigned int plane_no)
{
	if (ctx->nonblocking || ctx->plane_attr[plane_no] & DVB_VB2_PLANE_ATTR_NONBLOCK)
		return true;
	else
		return false;
}

static bool _get_vb2_queued_buf(struct dvb_vb2_ctx *ctx, unsigned int plane_no)
{
	if (ctx->buf) {
		dprintk(3, "[%s] use existing buffer!!!\n", ctx->name);
		return true;
	}

	if (list_empty(&ctx->dvb_q)) {
		dprintk(3, "[%s] Buffer overflow!!!\n", ctx->name);
		return false;
	}

	ctx->buf = list_entry(ctx->dvb_q.next, struct dvb_buffer, list);
	dprintk(3, "[%s] fill ctx->buf\n", ctx->name);

	return true;
}

static bool _check_vb2_buffer_done(struct dvb_vb2_ctx *ctx, unsigned int plane_no)
{
	int i, cnt = 0;
	unsigned long len;

	for (i = 0; i < ctx->plane_depth; i++) {
		if (ctx->plane_attr[i] & DVB_VB2_PLANE_ATTR_WO_COPY) {
			cnt++;
			continue;
		}

		if (_is_vb2_plane_nonblocking(ctx, plane_no)) {
			len = vb2_get_plane_payload(&ctx->buf->vb, plane_no);

			if (len == ctx->offset[plane_no])
				cnt++;
		} else if (ctx->offset[i] && !ctx->remain[i])
			cnt++;
	}

	return (cnt == ctx->plane_depth) ? true : false;
}

static void _reset_cached_buf(struct dvb_vb2_ctx *ctx)
{
	int i;

	ctx->buf = NULL;
	for (i = 0; i < ctx->plane_depth; i++) {
		ctx->offset[i] = 0;
		ctx->remain[i] = 0;
	}
}

int dvb_vb2_fill_plane(struct dvb_vb2_ctx *ctx,
			const unsigned char *src, int len,
			unsigned int plane_no,
			enum dmx_buffer_flags *buffer_flags)
{
	unsigned long flags = 0;
	void *vbuf = NULL;
	int todo = len;
	unsigned char *psrc = (unsigned char *)src;
	int ll = 0;

	if (plane_no >= ctx->plane_depth) {
		dprintk(1, "[%s] plane no. out-of-range\n", ctx->name);
		return -EINVAL;
	}

	/*
	 * normal case: This func is called twice from demux driver
	 * one with valid src pointer, second time with NULL pointer
	 */
	if (!_is_vb2_plane_wo_copy(ctx, plane_no) && (!src || !len))
		return 0;

	spin_lock_irqsave(&ctx->slock, flags);

	if (buffer_flags && *buffer_flags) {
		ctx->flags |= *buffer_flags;
		*buffer_flags = 0;
	}

	if (!ctx->buf && !_get_vb2_queued_buf(ctx, plane_no)) {
		spin_unlock_irqrestore(&ctx->slock, flags);
		return -EFAULT;
	}

	if (!ctx->offset[plane_no] && !ctx->remain[plane_no]) {
		ctx->remain[plane_no] = vb2_plane_size(&ctx->buf->vb, plane_no);
		ctx->offset[plane_no] = 0;
	}

	if (!dvb_vb2_is_streaming(ctx)) {
		vb2_buffer_done(&ctx->buf->vb, VB2_BUF_STATE_ERROR);
		list_del(&ctx->buf->list);
		_reset_cached_buf(ctx);
		spin_unlock_irqrestore(&ctx->slock, flags);
		return -EFAULT;
	}

	/* Fill buffer */
	if (!_is_vb2_plane_wo_copy(ctx, plane_no)) {
		ll = min(todo, ctx->remain[plane_no]);

		if (ctx->memory & VB2_MEMORY_DMABUF) {
			struct dma_buf *db = ctx->buf->vb.planes[plane_no].dbuf;
			void *va = NULL;
			u64 size = 0;

			if (_map_dmabuf_va(db, &va, &size) < 0) {
				dprintk(1, "[%s] (no. %u) map dmabuf fail\n", ctx->name, plane_no);
				spin_unlock_irqrestore(&ctx->slock, flags);
				return -EFAULT;
			}

			vbuf = va;
		} else
			vbuf = vb2_plane_vaddr(&ctx->buf->vb, plane_no);

		memcpy(vbuf + ctx->offset[plane_no], psrc, ll);

		if (ctx->memory & VB2_MEMORY_DMABUF) {
			struct dma_buf *db = ctx->buf->vb.planes[plane_no].dbuf;

			_unmap_dmabuf_va(db, vbuf);
		}

		todo -= ll;
		psrc += ll;

		ctx->remain[plane_no] -= ll;
		ctx->offset[plane_no] += ll;
	}

	if (_is_vb2_plane_nonblocking(ctx, plane_no) && ctx->remain[plane_no])
		vb2_set_plane_payload(&ctx->buf->vb, plane_no, ctx->offset[plane_no]);

	if (_check_vb2_buffer_done(ctx, plane_no)) {
		vb2_buffer_done(&ctx->buf->vb, VB2_BUF_STATE_DONE);
		list_del(&ctx->buf->list);
		_reset_cached_buf(ctx);
		dprintk(3, "[%s] (no. %u) vb2_buffer_done\n", ctx->name, plane_no);
	}

	spin_unlock_irqrestore(&ctx->slock, flags);

	if (todo)
		dprintk(1, "[%s] (no. %u) %d bytes are dropped.\n", ctx->name, plane_no, todo);
	else
		dprintk(3, "[%s] (no. %u)\n", ctx->name, plane_no);

	dprintk(3, "[%s] (no. %u) %d bytes are copied\n", ctx->name, plane_no, len - todo);
	return (len - todo);
}
EXPORT_SYMBOL(dvb_vb2_fill_plane);
#endif

int dvb_vb2_reqbufs(struct dvb_vb2_ctx *ctx, struct dmx_requestbuffers *req)
{
#ifdef CONFIG_ARCH_MEDIATEK_DTV
	int ret;
	enum dvb_buf_type type = (req->type == DMX_TYPE_CAPTURE_MPLANE) ?
		DVB_BUF_TYPE_CAPTURE_MPLANE : DVB_BUF_TYPE_CAPTURE;
	enum vb2_memory memory = (req->memory == DMX_MEMORY_DMABUF) ?
		VB2_MEMORY_DMABUF : VB2_MEMORY_MMAP;

	/* Adjust size to a sane value */
	int i, depth = 0;

	for (i = 0; i < VB2_MAX_PLANES; i++) {
		if (req->size[i] > 0) {
			if ((req->memory == DMX_MEMORY_MMAP) &&
				(req->size[i] > DVB_V2_MAX_SIZE))
				req->size[i] = DVB_V2_MAX_SIZE;

			ctx->buf_siz[i] = req->size[i];
			depth++;
		}
	}

	if ((req->type == DMX_TYPE_CAPTURE) && (depth != 1)) {
		dprintk(3, "[%s] truncate plane depth to 1\n", ctx->name);
		depth = 1;
	}

	/* FIXME: round req->size to a 188 or 204 multiple */

	ctx->buf_cnt = req->count;
	ctx->type = type;
	ctx->vb_q.type = type; // type of vb2_queue
	ctx->memory = memory;
	ctx->plane_depth = depth;
	ret = vb2_core_reqbufs(&ctx->vb_q, memory, &req->count);
	if (ret) {
		ctx->state = DVB_VB2_STATE_NONE;

		dprintk(1, "[%s] errno=%d count=%d\n", ctx->name, ret, ctx->buf_cnt);
		for (i = 0; i < VB2_MAX_PLANES; i++)
			if (ctx->buf_siz[i])
				dprintk(1, "size(%d)=%d\n", i, ctx->buf_siz[i]);

		return ret;
	}
	ctx->state |= DVB_VB2_STATE_REQBUFS;
	dprintk(3, "[%s] count=%d num_planes=%d\n", ctx->name, ctx->buf_cnt, ctx->plane_depth);
	for (i = 0; i < VB2_MAX_PLANES; i++)
		if (ctx->buf_siz[i])
			dprintk(3, "size(%d)=%d\n", i, ctx->buf_siz[i]);

	return 0;
#else
	int ret;

	/* Adjust size to a sane value */
	if (req->size > DVB_V2_MAX_SIZE)
		req->size = DVB_V2_MAX_SIZE;

	/* FIXME: round req->size to a 188 or 204 multiple */

	ctx->buf_siz = req->size;
	ctx->buf_cnt = req->count;
	ret = vb2_core_reqbufs(&ctx->vb_q, VB2_MEMORY_MMAP, &req->count);
	if (ret) {
		ctx->state = DVB_VB2_STATE_NONE;
		dprintk(1, "[%s] count=%d size=%d errno=%d\n", ctx->name,
			ctx->buf_cnt, ctx->buf_siz, ret);
		return ret;
	}
	ctx->state |= DVB_VB2_STATE_REQBUFS;
	dprintk(3, "[%s] count=%d size=%d\n", ctx->name,
		ctx->buf_cnt, ctx->buf_siz);

	return 0;
#endif
}

int dvb_vb2_querybuf(struct dvb_vb2_ctx *ctx, struct dmx_buffer *b)
{
	vb2_core_querybuf(&ctx->vb_q, b->index, b);
	dprintk(3, "[%s] index=%d\n", ctx->name, b->index);
	return 0;
}

int dvb_vb2_expbuf(struct dvb_vb2_ctx *ctx, struct dmx_exportbuffer *exp)
{
	struct vb2_queue *q = &ctx->vb_q;
	int ret;

	ret = vb2_core_expbuf(&ctx->vb_q, &exp->fd, q->type, exp->index,
				  0, exp->flags);
	if (ret) {
		dprintk(1, "[%s] index=%d errno=%d\n", ctx->name,
			exp->index, ret);
		return ret;
	}
	dprintk(3, "[%s] index=%d fd=%d\n", ctx->name, exp->index, exp->fd);

	return 0;
}

int dvb_vb2_qbuf(struct dvb_vb2_ctx *ctx, struct dmx_buffer *b)
{
	int ret;

#ifdef CONFIG_ARCH_MEDIATEK_DTV
	if (ctx->memory & VB2_MEMORY_DMABUF) {
		ret = _fill_vb2_buffer_from_user(&ctx->vb_q, b);
		if (ret < 0)
			return ret;
	}
#endif

	ret = vb2_core_qbuf(&ctx->vb_q, b->index, b, NULL);
	if (ret) {
		dprintk(1, "[%s] index=%d errno=%d\n", ctx->name,
			b->index, ret);
		return ret;
	}
	dprintk(5, "[%s] index=%d\n", ctx->name, b->index);

	return 0;
}

int dvb_vb2_dqbuf(struct dvb_vb2_ctx *ctx, struct dmx_buffer *b)
{
	unsigned long flags;
	int ret;

	ret = vb2_core_dqbuf(&ctx->vb_q, &b->index, b, ctx->nonblocking);
	if (ret) {
		dprintk(1, "[%s] errno=%d\n", ctx->name, ret);
		return ret;
	}

	spin_lock_irqsave(&ctx->slock, flags);
	b->count = ctx->count++;
	b->flags = ctx->flags;
	ctx->flags = 0;
	spin_unlock_irqrestore(&ctx->slock, flags);

	dprintk(5, "[%s] index=%d, count=%d, flags=%d\n",
		ctx->name, b->index, ctx->count, b->flags);


	return 0;
}

int dvb_vb2_mmap(struct dvb_vb2_ctx *ctx, struct vm_area_struct *vma)
{
	int ret;

	ret = vb2_mmap(&ctx->vb_q, vma);
	if (ret) {
		dprintk(1, "[%s] errno=%d\n", ctx->name, ret);
		return ret;
	}
	dprintk(3, "[%s] ret=%d\n", ctx->name, ret);

	return 0;
}

__poll_t dvb_vb2_poll(struct dvb_vb2_ctx *ctx, struct file *file,
			  poll_table *wait)
{
	dprintk(3, "[%s]\n", ctx->name);
	return vb2_core_poll(&ctx->vb_q, file, wait);
}

