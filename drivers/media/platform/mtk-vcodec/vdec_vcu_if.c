// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/fdtable.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/dma-resv.h>
#include <media/v4l2-mem2mem.h>
#include "mtk_vcodec_fence.h"
#include "mtk_vcodec_dec_pm.h"
#include "mtk_vcodec_dec.h"
#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_intr.h"
#include "mtk_vcodec_util.h"
#include "vdec_ipi_msg.h"
#include "vdec_vcu_if.h"
#include "vdec_drv_if.h"

#define OSTL_LARB 10
#define OSTL_PORT 1

static void handle_init_ack_msg(struct vdec_vcu_ipi_init_ack *msg)
{
	struct vdec_vcu_inst *vcu = (struct vdec_vcu_inst *)
		(unsigned long)msg->ap_inst_addr;

	if (vcu == NULL)
		return;
	mtk_vcodec_debug(vcu, "+ ap_inst_addr = 0x%lx",
		(uintptr_t)msg->ap_inst_addr);

	/* mapping VCU address to kernel virtual address */
	/* the content in vsi is initialized to 0 in VCU */
	vcu->vsi = vcu_mapping_dm_addr(vcu->dev, msg->vcu_inst_addr);
	vcu->inst_addr = msg->vcu_inst_addr;
	mtk_vcodec_debug(vcu, "- vcu_inst_addr = 0x%lx", vcu->inst_addr);
}

static void handle_query_cap_ack_msg(struct vdec_vcu_ipi_query_cap_ack *msg)
{
	struct vdec_vcu_inst *vcu = (struct vdec_vcu_inst *)msg->ap_inst_addr;
	void *data;
	int size = 0;

	if (vcu == NULL)
		return;
	mtk_vcodec_debug(vcu, "+ ap_inst_addr = 0x%lx, vcu_data_addr = 0x%llx, id = %d",
		(uintptr_t)msg->ap_inst_addr, msg->vcu_data_addr, msg->id);
	/* mapping VCU address to kernel virtual address */
	data = vcu_mapping_dm_addr(vcu->dev, msg->vcu_data_addr);
	if (data == NULL)
		return;
	switch (msg->id) {
	case GET_PARAM_CAPABILITY_SUPPORTED_FORMATS:
		size = sizeof(struct mtk_video_fmt);
		memcpy((void *)msg->ap_data_addr, data,
			 size * MTK_MAX_DEC_CODECS_SUPPORT);
		break;
	case GET_PARAM_CAPABILITY_FRAME_SIZES:
		size = sizeof(struct mtk_codec_framesizes);
		memcpy((void *)msg->ap_data_addr, data,
			size * MTK_MAX_DEC_CODECS_SUPPORT);
		break;
	case GET_PARAM_DV_SUPPORTED_PROFILE_LEVEL:
		size = sizeof(struct v4l2_vdec_dv_profile_level);
		memcpy((void *)msg->ap_data_addr, data,
			size * V4L2_CID_VDEC_DV_MAX_PROFILE_CNT);
		break;
	default:
		break;
	}
	mtk_vcodec_debug(vcu, "- vcu_inst_addr = 0x%lx", vcu->inst_addr);
}

static bool vcu_msg_with_hw_id(struct vdec_vcu_ipi_ack *msg)
{
	switch (msg->msg_id) {
	case VCU_IPIMSG_DEC_LOCK_LAT:
	case VCU_IPIMSG_DEC_UNLOCK_LAT:
	case VCU_IPIMSG_DEC_LOCK_CORE:
	case VCU_IPIMSG_DEC_UNLOCK_CORE:
	case VCU_IPIMSG_DEC_WAITISR:
	case VCU_IPIMSG_DEC_SMI_LARB_OSTDL:
		return true;
	default:
		return false;
	}
}

/*
 * This function runs in interrupt context and it means there's a IPI MSG
 * from VCU.
 */
int vcu_dec_ipi_handler(void *data, unsigned int len, void *priv)
{
	struct vdec_vcu_ipi_ack *msg = data;
	struct vdec_vcu_inst *vcu = NULL;
	struct vdec_fb *pfb;
	struct timespec64 t_s, t_e;
	struct vdec_vsi *vsi;
	uint64_t vdec_fb_va;
	long timeout_jiff;
	int ret = 0;
	int i = 0;
	struct list_head *p, *q;
	struct mtk_vcodec_ctx *temp_ctx;
	struct mtk_vcodec_dev *dev = (struct mtk_vcodec_dev *)priv;
	struct vdec_inst *inst = NULL;
	int msg_valid = 0;
	unsigned long flags;

	vcu = (struct vdec_vcu_inst *)(unsigned long)msg->ap_inst_addr;
	/* Check IPI inst is valid */
	spin_lock_irqsave(&dev->ctx_lock, flags);
	list_for_each_safe(p, q, &dev->ctx_list) {
		temp_ctx = list_entry(p, struct mtk_vcodec_ctx, list);
		inst = (struct vdec_inst *)temp_ctx->drv_handle;
		if (inst != NULL && vcu == &inst->vcu && vcu->ctx == temp_ctx) {
			msg_valid = 1;
			break;
		}
	}
	if (!msg_valid) {
		mtk_v4l2_err(" msg vcu not exist %p\n", vcu);
		spin_unlock_irqrestore(&dev->ctx_lock, flags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&dev->ctx_lock, flags);

	vsi = (struct vdec_vsi *)vcu->vsi;
	mtk_vcodec_debug(vcu, "+ id=%X status = %d\n",
		msg->msg_id, msg->status);

	if (vcu->abort)
		return -EINVAL;

	if (msg->msg_id == VCU_IPIMSG_DEC_WAITISR) {
		/* wait decoder done interrupt */
		ktime_get_real_ts64(&t_s);
		ret = mtk_vcodec_wait_for_done_ctx(vcu->ctx,
			msg->status,
			MTK_INST_IRQ_RECEIVED,
			WAIT_INTR_TIMEOUT_MS);
		ktime_get_real_ts64(&t_e);
		mtk_vcodec_perf_log("irq:%lld",
			(t_e.tv_sec - t_s.tv_sec) * 1000000 +
			(t_e.tv_nsec - t_s.tv_nsec) / 1000);
		msg->status = ret;
		ret = 1;
	} else if (msg->status == 0 || vcu_msg_with_hw_id(msg)) {
		switch (msg->msg_id) {
		case VCU_IPIMSG_DEC_INIT_ACK:
			handle_init_ack_msg(data);
			break;
		case VCU_IPIMSG_DEC_QUERY_CAP_ACK:
			handle_query_cap_ack_msg(data);
			break;
		case VCU_IPIMSG_DEC_START_ACK:
		case VCU_IPIMSG_DEC_END_ACK:
		case VCU_IPIMSG_DEC_DEINIT_ACK:
		case VCU_IPIMSG_DEC_RESET_ACK:
		case VCU_IPIMSG_DEC_SET_PARAM_ACK:
			break;
		case VCU_IPIMSG_DEC_LOCK_LAT: {
			int hw_id = msg->status;

			vdec_decode_prepare(vcu->ctx, MTK_VDEC_LAT + hw_id);
			ret = 1;
			msg->status = 0;
			break;
		}
		case VCU_IPIMSG_DEC_UNLOCK_LAT: {
			int hw_id = msg->status;

			vdec_decode_unprepare(vcu->ctx, MTK_VDEC_LAT + hw_id);
			ret = 1;
			msg->status = 0;
			break;
		}
		case VCU_IPIMSG_DEC_LOCK_CORE: {
			int hw_id = msg->status;

			vdec_decode_prepare(vcu->ctx, MTK_VDEC_CORE + hw_id);
			ret = 1;
			msg->status = 0;
			break;
		}
		case VCU_IPIMSG_DEC_UNLOCK_CORE: {
			int hw_id = msg->status;

			vdec_decode_unprepare(vcu->ctx, MTK_VDEC_CORE + hw_id);
			ret = 1;
			msg->status = 0;
			break;
		}
		case VCU_IPIMSG_DEC_GET_FRAME_BUFFER:
			mtk_vcodec_debug(vcu, "+ try get fm form rdy q size=%d\n",
				v4l2_m2m_num_dst_bufs_ready(vcu->ctx->m2m_ctx));

			pfb = mtk_vcodec_get_fb(vcu->ctx);
			timeout_jiff = msecs_to_jiffies(1000);
			/* 1s timeout */
			while (pfb == NULL) {
				ret = wait_event_interruptible_timeout(
					vcu->ctx->fm_wq,
					 v4l2_m2m_num_dst_bufs_ready(
						 vcu->ctx->m2m_ctx) > 0 ||
					 flush_abort_state(vcu->ctx->state),
					 timeout_jiff);
				pfb = mtk_vcodec_get_fb(vcu->ctx);
				if (flush_abort_state(vcu->ctx->state))
					mtk_vcodec_debug(vcu, "get fm fail: state == %d (pfb=0x%p)\n",
						vcu->ctx->state, pfb);
				else if (ret == 0)
					mtk_vcodec_debug(vcu, "get fm fail: timeout (pfb=0x%p)\n",
						pfb);
				else if (pfb == NULL)
					mtk_vcodec_debug(vcu, "get fm fail: unknown (ret = %d)\n",
						ret);

				if (flush_abort_state(vcu->ctx->state) ||
					ret != 0)
					break;
			}
			mtk_vcodec_debug(vcu, "- wait get fm pfb=0x%p\n", pfb);

			vdec_fb_va = (u64)(uintptr_t)pfb;
			vsi->dec.vdec_fb_va = vdec_fb_va;
			if (pfb != NULL) {
				vsi->dec.index = pfb->index;
				for (i = 0; i < pfb->num_planes; i++) {
					vsi->dec.fb_dma[i] = (u64)
						pfb->fb_base[i].dma_addr;
					vsi->dec.fb_va[i] = (uintptr_t)pfb->fb_base[i].va;
					vsi->dec.fb_length[i] = (uint32_t)
						pfb->fb_base[i].length;
					mtk_vcodec_debug(vcu, "+ vsi->dec.fb_fd[%d]:%llx\n",
						i, vsi->dec.fb_fd[i]);
				}
				if (pfb->dma_general_buf != 0) {
					vsi->general_buf_fd =
						pfb->general_buf_fd;
					vsi->general_buf_size =
						pfb->dma_general_buf->size;
					mtk_vcodec_debug(vcu, "fb->dma_general_buf = %p, mapped fd = %d, size = %zu",
						pfb->dma_general_buf,
						vsi->general_buf_fd,
						pfb->dma_general_buf->size);
				} else {
					pfb->general_buf_fd = -1;
					vsi->general_buf_fd = -1;
					vsi->general_buf_size = 0;
					mtk_vcodec_debug(vcu, "no general buf dmabuf");
				}
			} else {
				vsi->dec.index = 0xFF;
				mtk_vcodec_debug(vcu, "Cannot get frame buffer from V4l2\n");
			}
			mtk_vcodec_debug(vcu, "+ FB y_fd=%llx c_fd=%llx BS fd=0x%llx index:%d vdec_fb_va:%llx,dma_addr(%llx,%llx)",
				vsi->dec.fb_fd[0], vsi->dec.fb_fd[1],
				vsi->dec.bs_fd, vsi->dec.index,
				vsi->dec.vdec_fb_va,
				vsi->dec.fb_dma[0], vsi->dec.fb_dma[1]);

			ret = 1;
			break;
		case VCU_IPIMSG_DEC_PUT_FRAME_BUFFER:
			mtk_vdec_put_fb(vcu->ctx, PUT_BUFFER_CALLBACK);
			ret = 1;
			break;
		case VCU_IPIMSG_DEC_SLICE_DONE_ISR: {
			struct vdec_fb *pfb = (struct vdec_fb *)(uintptr_t)msg->payload;
			struct dma_buf *dbuf = pfb->fb_base[0].dmabuf;
			struct dma_fence *fence = dma_resv_get_excl_rcu(dbuf->resv);

			if (fence) {
				mtk_vcodec_fence_signal(fence, pfb->slice_done_count);
				pfb->slice_done_count++;
				mtk_vcodec_debug(vcu, "slice done count %d\n",
					pfb->slice_done_count);
				dma_fence_put(fence);
			}
			break;
		}
		case VCU_IPIMSG_DEC_SMI_LARB_OSTDL:
			mtk_smi_larb_ostdl(OSTL_LARB, OSTL_PORT, (int) msg->status);
			break;
		default:
			mtk_vcodec_err(vcu, "invalid msg=%X", msg->msg_id);
			ret = 1;
			break;
		}
	}

	mtk_vcodec_debug(vcu, "- id=%X", msg->msg_id);

	/* deinit ack timeout case handling do not touch vdec_vcu_inst
	 * or memory used after freed
	 */
	if (msg->msg_id != VCU_IPIMSG_DEC_DEINIT_ACK) {
		vcu->signaled = 1;
		vcu->failure = msg->status;
	}

	return ret;
}

static int vcodec_vcu_send_msg(struct vdec_vcu_inst *vcu, void *msg, int len)
{
	int err;

	mtk_vcodec_debug(vcu, "id=%X", *(uint32_t *)msg);
	if (vcu->abort)
		return -EIO;

	vcu->failure = 0;
	vcu->signaled = 0;

	err = vcu_ipi_send(vcu->dev, vcu->id, msg, len);
	if (err) {
		mtk_vcodec_err(vcu, "send fail vcu_id=%d msg_id=%X status=%d",
					   vcu->id, *(uint32_t *)msg, err);
		if (err == -EIO)
			vcu->abort = 1;
		return err;
	}
	mtk_vcodec_debug(vcu, "- ret=%d", err);
	return vcu->failure;
}

static int vcodec_send_ap_ipi(struct vdec_vcu_inst *vcu, unsigned int msg_id)
{
	struct vdec_ap_ipi_cmd msg;
	int err = 0;

	mtk_vcodec_debug(vcu, "+ id=%X", msg_id);

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = msg_id;
	msg.vcu_inst_addr = vcu->inst_addr;

	err = vcodec_vcu_send_msg(vcu, &msg, sizeof(msg));
	mtk_vcodec_debug(vcu, "- id=%X ret=%d", msg_id, err);
	return err;
}

int vcu_dec_init(struct vdec_vcu_inst *vcu)
{
	struct vdec_ap_ipi_init msg;
	int err;

	mtk_vcodec_debug_enter(vcu);

	init_waitqueue_head(&vcu->wq);
	vcu->signaled = 0;
	vcu->failure = 0;

	err = vcu_ipi_register(vcu->dev, vcu->id, vcu->handler, NULL, vcu->ctx->dev);
	if (err != 0) {
		mtk_vcodec_err(vcu, "vcu_ipi_register fail status=%d", err);
		return err;
	}

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = AP_IPIMSG_DEC_INIT;
	msg.ap_inst_addr = (unsigned long)vcu;

	if (vcu->ctx->dec_params.svp_mode == 1)
		msg.reserved = vcu->ctx->dec_params.svp_mode;

	mtk_vcodec_debug(vcu, "vdec_inst=%p svp_mode=%d", vcu, msg.reserved);

	err = vcodec_vcu_send_msg(vcu, (void *)&msg, sizeof(msg));
	mtk_vcodec_debug(vcu, "- ret=%d", err);
	return err;
}

int vcu_dec_start(struct vdec_vcu_inst *vcu, unsigned int *data,
				  unsigned int len)
{
	struct vdec_ap_ipi_dec_start msg;
	int i;
	int err = 0;

	mtk_vcodec_debug_enter(vcu);

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = AP_IPIMSG_DEC_START;
	msg.vcu_inst_addr = vcu->inst_addr;

	for (i = 0; i < len; i++)
		msg.data[i] = data[i];

	err = vcodec_vcu_send_msg(vcu, (void *)&msg, sizeof(msg));
	mtk_vcodec_debug(vcu, "- ret=%d", err);
	return err;
}

int vcu_dec_end(struct vdec_vcu_inst *vcu)
{
	return vcodec_send_ap_ipi(vcu, AP_IPIMSG_DEC_END);
}

int vcu_dec_deinit(struct vdec_vcu_inst *vcu)
{
	return vcodec_send_ap_ipi(vcu, AP_IPIMSG_DEC_DEINIT);
}

int vcu_dec_reset(struct vdec_vcu_inst *vcu)
{
	return vcodec_send_ap_ipi(vcu, AP_IPIMSG_DEC_RESET);
}

int vcu_dec_query_cap(struct vdec_vcu_inst *vcu, unsigned int id, void *out)
{
	struct vdec_ap_ipi_query_cap msg;
	int err = 0;

	mtk_vcodec_debug(vcu, "+ id=%X", AP_IPIMSG_DEC_QUERY_CAP);
	vcu->dev = vcu_get_plat_device(vcu->ctx->dev->plat_dev);
	if (vcu->dev  == NULL) {
		mtk_vcodec_err(vcu, "vcu device in not ready");
		return -EPROBE_DEFER;
	}

	vcu->id = (vcu->id == IPI_VCU_INIT) ? IPI_VDEC_COMMON : vcu->id;
	vcu->handler = vcu_dec_ipi_handler;

	err = vcu_ipi_register(vcu->dev, vcu->id, vcu->handler, NULL, vcu->ctx->dev);
	if (err != 0) {
		mtk_vcodec_err(vcu, "vcu_ipi_register fail status=%d", err);
		return err;
	}

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = AP_IPIMSG_DEC_QUERY_CAP;
	msg.id = id;
	msg.ap_inst_addr = (uintptr_t)vcu;
	msg.ap_data_addr = (uintptr_t)out;

	err = vcodec_vcu_send_msg(vcu, &msg, sizeof(msg));
	mtk_vcodec_debug(vcu, "- id=%X ret=%d", msg.msg_id, err);
	return err;
}

int vcu_dec_set_param(struct vdec_vcu_inst *vcu, unsigned int id, void *param,
					  unsigned int size)
{
	struct vdec_ap_ipi_set_param msg;
	unsigned long *param_ptr = (unsigned long *)param;
	int err = 0;
	int i = 0;

	mtk_vcodec_debug(vcu, "+ id=%X", AP_IPIMSG_DEC_SET_PARAM);

	memset(&msg, 0, sizeof(msg));
	msg.msg_id = AP_IPIMSG_DEC_SET_PARAM;
	msg.id = id;
	msg.vcu_inst_addr = vcu->inst_addr;
	for (i = 0; i < size; i++) {
		msg.data[i] = (__u32)(*(param_ptr + i));
		mtk_vcodec_debug(vcu, "msg.id = 0x%X, msg.data[%d]=%d",
			msg.id, i, msg.data[i]);
	}

	err = vcodec_vcu_send_msg(vcu, &msg, sizeof(msg));
	mtk_vcodec_debug(vcu, "- id=%X ret=%d", AP_IPIMSG_DEC_SET_PARAM, err);

	return err;
}
