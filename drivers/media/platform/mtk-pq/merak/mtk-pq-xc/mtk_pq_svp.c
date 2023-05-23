// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#if IS_ENABLED(CONFIG_OPTEE)
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/err.h>
#include <asm/uaccess.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#include "mtk_pq.h"
#include "mtk_pq_common_ca.h"
#include "mtk_iommu_dtv_api.h"
#include "utpa2_XC.h"

static int _mtk_pq_teec_enable(struct mtk_pq_device *pq_dev)
{
	u32 tee_error = 0;
	int ret = 0;
	TEEC_UUID uuid = DISP_TA_UUID;
	TEEC_Context *pstContext = pq_dev->display_info.secure_info.pstContext;
	TEEC_Session *pstSession = pq_dev->display_info.secure_info.pstSession;

	if ((pstSession != NULL) && (pstContext != NULL)) {
		ret = mtk_pq_common_ca_teec_init_context(pq_dev, pstContext);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Init Context failed!\n");
			mtk_pq_common_ca_teec_finalize_context(pq_dev, pstContext);
			return -EPERM;
		}

		ret = mtk_pq_common_ca_teec_open_session(pq_dev, pstContext, pstSession, &uuid,
					NULL, &tee_error);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"TEEC Open pstSession failed with error(%u)\n", tee_error);
			mtk_pq_common_ca_teec_close_session(pq_dev, pstSession);
			mtk_pq_common_ca_teec_finalize_context(pq_dev, pstContext);
			return -EPERM;
		}

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "Enable disp teec success.\n");
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstSession or pstContext is NULL.\n");
		return -EPERM;
	}

	return 0;
}

static void _mtk_pq_teec_disable(struct mtk_pq_device *pq_dev)
{
	TEEC_Context *pstContext = pq_dev->display_info.secure_info.pstContext;
	TEEC_Session *pstSession = pq_dev->display_info.secure_info.pstSession;

	if ((pstContext != NULL) && (pstSession != NULL)) {
		mtk_pq_common_ca_teec_close_session(pq_dev, pstSession);
		mtk_pq_common_ca_teec_finalize_context(pq_dev, pstContext);

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "Disable disp teec success.\n");
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstSession or pstContext is NULL.\n");
	}
}

static int _mtk_pq_unauth_internal_buf(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	u8 idx = 0;
	u64 iova = 0;
	struct pq_buffer *pBufferTable = NULL;

	/* get device parameter */
	pBufferTable = pq_dev->display_info.BufferTable;

	if (pBufferTable == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pBufferTable = NULL\n");
		return -EPERM;
	}

	/* unauthorize pq buf */
	if (!pq_dev->display_info.secure_info.pq_buf_authed)
		goto already_unauth;

	for (idx = 0; idx < PQU_BUF_MAX; idx++) {
		if (pBufferTable[idx].valid && pBufferTable[idx].sec_buf) {
			iova = pBufferTable[idx].addr; //unlock secure buf only
			ret = mtkd_iommu_buffer_unauthorize(iova);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unauthorize fail\n");
				return -EPERM;
			}

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
				"unauth pq_buf[%u]: iova=0x%llx\n", idx, iova);
		}
	}
	pq_dev->display_info.secure_info.pq_buf_authed = false;
	return ret;

already_unauth:
	return ret;
}

static int _mtk_pq_auth_internal_buf(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	u8 idx = 0;
	u32 size = 0, pipelineID = 0, buf_tag = 0;
	u64 iova = 0;
	bool secbuf_found = false;
	struct pq_buffer *pBufferTable = NULL;
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(pq_dev->dev);

	/* get device parameter */
	pipelineID = pq_dev->display_info.secure_info.disp_pipeline_ID;
	pBufferTable = pq_dev->display_info.BufferTable;

	/* authorize pq buf */
	if (pq_dev->display_info.secure_info.pq_buf_authed)
		goto already_auth;

	if ((pipelineID == 0) || (!pq_dev->display_info.secure_info.disp_pipeline_valid))
		goto auth_next_time;

	for (idx = 0; idx < PQU_BUF_MAX; idx++) {
		if (pBufferTable[idx].valid && pBufferTable[idx].sec_buf) {
			buf_tag = mtk_pq_buf_get_iommu_idx(pqdev, idx);
			size = pBufferTable[idx].size;
			iova = pBufferTable[idx].addr; //lock secure buf only

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
				"auth pq_buf[%u]: buf_tag=%u, size=%u, iova=0x%llx, pipelineID=0x%llx\n",
				idx, buf_tag, size, iova, pipelineID);

			ret = mtkd_iommu_buffer_authorize(buf_tag, iova, size, pipelineID);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "authorize fail\n");
				return -EPERM;
			}
			secbuf_found = true;
		}
	}

	if (secbuf_found)
		pq_dev->display_info.secure_info.pq_buf_authed = true;

auth_next_time:
already_auth:
	return ret;
}

static int _mtk_pq_auth_buf(struct mtk_pq_device *pq_dev, struct mtk_pq_dma_buf *buf)
{
	int ret = 0;
	u32 pipelineID = 0;

	if (buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "buf = NULL\n");
		return -EPERM;
	}

	/* get device parameter */
	pipelineID = pq_dev->display_info.secure_info.disp_pipeline_ID;
	if (pipelineID == 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid disp pipelineID = 0\n");
		return -EPERM;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
		"Auth buf iova = 0x%llx, size = 0x%llx\n", buf->iova, buf->size);

	/* authorize buf */
	ret = mtkd_iommu_buffer_authorize((29 >> 24), buf->iova, buf->size, pipelineID);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "iommu authorize fail\n");
		return -EPERM;
	}

	return ret;
}

static int _mtk_pq_parse_outbuf_meta(struct mtk_pq_device *pq_dev,
			int meta_fd, enum mtk_pq_aid *aid, u32 *pipeline_id)
{
	int ret = 0;
	struct vdec_dd_svp_info vdec_svp_md;
	struct meta_srccap_svp_info srccap_svp_md;

	if (aid == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "aid = NULL\n");
		goto err_out;
	}
	if (pipeline_id == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pipeline_id = NULL\n");
		goto err_out;
	}

	memset(&vdec_svp_md, 0, sizeof(struct vdec_dd_svp_info));
	memset(&srccap_svp_md, 0, sizeof(struct meta_srccap_svp_info));

	*aid = PQ_AID_NS;
	*pipeline_id = 0;

	if (IS_INPUT_B2R(pq_dev->common_info.input_source)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "MVOP source!!\n");

		ret = mtk_pq_common_read_metadata(meta_fd,
			EN_PQ_METATAG_VDEC_SVP_INFO, (void *)&vdec_svp_md);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "metadata do not has svp tag\n");
			goto non_svp_out;
		}

		if (!CHECK_AID_VALID(vdec_svp_md.aid)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Received invalid aid %u\n", vdec_svp_md.aid);
			goto err_out;
		}

		*aid = (enum mtk_pq_aid)vdec_svp_md.aid;
		*pipeline_id = vdec_svp_md.pipeline_id;

		ret = mtk_pq_common_delete_metadata(meta_fd,
			EN_PQ_METATAG_VDEC_SVP_INFO);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "delete svp tag fail\n");
			goto err_out;
		}
	}

	if (IS_INPUT_SRCCAP(pq_dev->common_info.input_source)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "EXTIN source!!\n");

		ret = mtk_pq_common_read_metadata(meta_fd,
			EN_PQ_METATAG_SRCCAP_SVP_INFO, (void *)&srccap_svp_md);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "metadata do not has svp tag\n");
			goto non_svp_out;
		}

		if (!CHECK_AID_VALID(srccap_svp_md.aid)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Received invalid aid %u\n", srccap_svp_md.aid);
			goto err_out;
		}

		*aid = (enum mtk_pq_aid)srccap_svp_md.aid;
		*pipeline_id = srccap_svp_md.pipelineid;

		ret = mtk_pq_common_delete_metadata(meta_fd,
			EN_PQ_METATAG_SRCCAP_SVP_INFO);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "delete svp tag fail\n");
			goto err_out;
		}
	}
	return ret;

non_svp_out:
	return 0;

err_out:
	return -EPERM;
}

static int _mtk_pq_teec_smc_call(struct mtk_pq_device *pq_dev,
	EN_XC_OPTEE_ACTION action, TEEC_Operation *op)
{
	u32 error = 0;
	int ret = 0;
	TEEC_Session *pstSession = pq_dev->display_info.secure_info.pstSession;

	if (pstSession == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstSession is NULL!\n");
		return -EPERM;
	}
	if (op == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "op is NULL!\n");
		return -EPERM;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQ SMC cmd:%u\n", action);

	ret = mtk_pq_common_ca_teec_invoke_cmd(pq_dev, pstSession, (u32)action, op, &error);
	if (ret) {
		if (action == E_XC_OPTEE_DESTROY_DISP_PIPELINE)
			ret = 0; //The cmd fail is acceptable while destroy
		else {
			ret = -EPERM;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"SMC call failed with error(%u)\n", error);
		}
	}

	return ret;
}

static int _mtk_pq_svp_check_outbuf_aid(struct mtk_pq_device *pq_dev,
	enum mtk_pq_aid bufaid, bool *spb)
{
	bool s_dev = false;
	enum mtk_pq_aid devaid = PQ_AID_NS;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}
	if (spb == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}
	if (!CHECK_AID_VALID(bufaid)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received invalid aid %u\n", bufaid);
		return -EINVAL;
	}

	s_dev = pq_dev->display_info.secure_info.svp_enable;

	if (pq_dev->display_info.secure_info.first_frame) {
		/* First frame just check dev condition */
		if ((bufaid != PQ_AID_NS) && (s_dev)) {
			*spb = true;
			if (bufaid == PQ_AID_SDC)
				pq_dev->display_info.secure_info.aid = PQ_AID_S;
			else
				pq_dev->display_info.secure_info.aid = bufaid;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "secure content playback!\n");
		}
		if ((bufaid != PQ_AID_NS) && (!s_dev)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"PQ QBUF ERROR! s-buffer queue into ns-dev!\n");
			return -EPERM;
		}
		if ((bufaid == PQ_AID_NS) && (s_dev)) {
			*spb = true;
			pq_dev->display_info.secure_info.aid = PQ_AID_S;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "secure content playback!\n");
		}
		if ((bufaid == PQ_AID_NS) && (!s_dev)) {
			*spb = false;
			pq_dev->display_info.secure_info.aid = PQ_AID_NS;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "non-secure content playback!\n");
		}
		pq_dev->display_info.secure_info.first_frame = false;
	} else {
		/* Dynamic stream aid change condition */
		devaid = pq_dev->display_info.secure_info.aid;
		if (!s_dev) {
			if (bufaid != PQ_AID_NS) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQ aid change, type = ns->s!\n");
				return -EADDRNOTAVAIL;
			}
		} else {
			if (bufaid > devaid) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQ aid change, type = ls->hs!\n");
				return -EADDRNOTAVAIL;
			} else if (bufaid < devaid) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
					"PQ aid change, type = hs->ls! Keep origin hs.\n");
				*spb = true;
			} else
				*spb = true;
		}
	}

	return 0;
}

int mtk_pq_svp_init(struct mtk_pq_device *pq_dev)
{
	TEEC_Context *pstContext = NULL;
	TEEC_Session *pstSession = NULL;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}

	pstContext = malloc(sizeof(TEEC_Context));
	if (pstContext == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstContext malloc fail!\n");
		return -EPERM;
	}

	pstSession = malloc(sizeof(TEEC_Session));
	if (pstSession == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pstSession malloc fail!\n");
		free(pstContext);
		return -EPERM;
	}

	memset(pstContext, 0, sizeof(TEEC_Context));
	memset(pstSession, 0, sizeof(TEEC_Session));

	if (pq_dev->display_info.secure_info.pstContext != NULL) {
		free(pq_dev->display_info.secure_info.pstContext);
		pq_dev->display_info.secure_info.pstContext = NULL;
	}

	if (pq_dev->display_info.secure_info.pstSession != NULL) {
		free(pq_dev->display_info.secure_info.pstSession);
		pq_dev->display_info.secure_info.pstSession = NULL;
	}

	memset(&pq_dev->display_info.secure_info, 0, sizeof(struct pq_secure_info));
	pq_dev->display_info.secure_info.pstContext = pstContext;
	pq_dev->display_info.secure_info.pstSession = pstSession;
	pq_dev->display_info.secure_info.optee_version = 3;

	if (_mtk_pq_teec_enable(pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "teec enable fail\n");
		return -EPERM;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQ SVP init success.\n");
	return 0;
}

void mtk_pq_svp_exit(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	u8 sec_md = 0;
	TEEC_Context *pstContext = NULL;
	TEEC_Session *pstSession = NULL;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return;
	}

	ret = mtk_pq_svp_set_sec_md(pq_dev, &sec_md);
	_mtk_pq_teec_disable(pq_dev);

	pstContext = pq_dev->display_info.secure_info.pstContext;
	pstSession = pq_dev->display_info.secure_info.pstSession;

	if (pstContext != NULL) {
		free(pstContext);
		pq_dev->display_info.secure_info.pstContext = NULL;
	}

	if (pstSession != NULL) {
		free(pstSession);
		pq_dev->display_info.secure_info.pstSession = NULL;
	}
}

int mtk_pq_svp_set_sec_md(struct mtk_pq_device *pq_dev, u8 *secure_mode_flag)
{
	int ret = 0;
	bool secure_mode = 0;
	XC_STI_OPTEE_HANDLER optee_handler;
	TEEC_Operation op = {0};

	memset(&optee_handler, 0, sizeof(XC_STI_OPTEE_HANDLER));
	memset(&op, 0, sizeof(TEEC_Operation));

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}
	if (secure_mode_flag == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}

	if (pq_dev->input_stream.streaming || pq_dev->output_stream.streaming) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Can not control sec mode while stream on!\n");
		return -EPERM;
	}

	secure_mode = (bool)*secure_mode_flag;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQDD secure mode:%s\n",
		(secure_mode ? "ENABLE":"DISABLE"));

	if ((!secure_mode) && (pq_dev->display_info.secure_info.disp_pipeline_valid)) {
		/* destroy disp pipeline */
		optee_handler.u16Version = XC_STI_OPTEE_HANDLER_VERSION;
		optee_handler.u16Length = sizeof(XC_STI_OPTEE_HANDLER);
		optee_handler.enAID = (EN_XC_STI_AID)pq_dev->display_info.secure_info.aid;
		optee_handler.u32DispPipelineID = pq_dev->display_info.secure_info.disp_pipeline_ID;
		op.params[PQ_CA_SMC_PARAM_IDX_0].tmpref.buffer = (void *)&optee_handler;
		op.params[PQ_CA_SMC_PARAM_IDX_0].tmpref.size = sizeof(XC_STI_OPTEE_HANDLER);
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
				TEEC_NONE, TEEC_NONE, TEEC_NONE);
		ret = _mtk_pq_teec_smc_call(pq_dev,
					E_XC_OPTEE_DESTROY_DISP_PIPELINE,
					&op);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "smc call fail\n");
			return ret;
		}

		/* clear para*/
		pq_dev->display_info.secure_info.disp_pipeline_valid = 0;
		pq_dev->display_info.secure_info.aid = PQ_AID_NS;
		pq_dev->display_info.secure_info.vdo_pipeline_ID = 0;
		pq_dev->display_info.secure_info.disp_pipeline_ID = 0;
	}

	pq_dev->display_info.secure_info.svp_enable = secure_mode;
	return ret;
}

int mtk_pq_svp_cfg_outbuf_sec(
	struct mtk_pq_device *pq_dev,
	struct v4l2_buffer *buffer)
{
	int ret = 0;
	bool s_dev = false;
	bool secure_playback = false;
	u32 video_pipeline_id = 0;
	enum mtk_pq_aid aid = 0;
	XC_STI_OPTEE_HANDLER optee_handler;
	struct v4l2_plane *plane_ptr = NULL;
	TEEC_Operation op = {0};

	memset(&optee_handler, 0, sizeof(XC_STI_OPTEE_HANDLER));
	memset(&op, 0, sizeof(TEEC_Operation));

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}
	if (buffer == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}

	s_dev = pq_dev->display_info.secure_info.svp_enable;

	/* parse source meta */
	if (buffer->m.planes == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}
	plane_ptr = &(buffer->m.planes[(buffer->length - 1)]);
	ret = _mtk_pq_parse_outbuf_meta(pq_dev,
		plane_ptr->m.fd,
		&aid,
		&video_pipeline_id);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Parse svp meta tag fail\n");
		goto out;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP,
		"Received video buf fd = %d, aid = %d, pipeline id = 0x%lx\n",
			buffer->m.planes[0].m.fd, aid, video_pipeline_id);

	/* check dev/buf secure relation */
	ret = _mtk_pq_svp_check_outbuf_aid(pq_dev, aid, &secure_playback);
	if (ret) {
		if (ret == -EADDRNOTAVAIL) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "AID dynamic change occurs!\n");
			goto out;
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "_mtk_pq_svp_check_outbuf_aid fail\n");
			goto out;
		}
	}

	if (secure_playback) {
		if (!pq_dev->display_info.secure_info.disp_pipeline_valid) {
			/* create disp pipeline */
			optee_handler.u16Version = XC_STI_OPTEE_HANDLER_VERSION;
			optee_handler.u16Length = sizeof(XC_STI_OPTEE_HANDLER);
			optee_handler.enAID = (EN_XC_STI_AID)pq_dev->display_info.secure_info.aid;
			op.params[PQ_CA_SMC_PARAM_IDX_0].tmpref.buffer = (void *)&optee_handler;
			op.params[PQ_CA_SMC_PARAM_IDX_0].tmpref.size = sizeof(XC_STI_OPTEE_HANDLER);
			op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
					TEEC_NONE, TEEC_NONE, TEEC_NONE);
			ret = _mtk_pq_teec_smc_call(pq_dev,
				E_XC_OPTEE_CREATE_DISP_PIPELINE,
				&op);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "smc call fail\n");
				goto out;
			}
			pq_dev->display_info.secure_info.disp_pipeline_ID
				= optee_handler.u32DispPipelineID;
			pq_dev->display_info.secure_info.disp_pipeline_valid = true;
		}
		/* authorize pq buffer */
		ret = _mtk_pq_auth_internal_buf(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "auth pq buf fail\n");
			goto out;
		}
	}

	pq_dev->display_info.secure_info.vdo_pipeline_ID = video_pipeline_id;

out:
	return ret;
}

int mtk_pq_svp_cfg_capbuf_sec(
	struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *buffer)
{
	int ret = 0;
	bool s_dev = false;
	struct meta_buffer meta_buf;
	struct meta_pq_disp_svp pq_svp_md;

	memset(&meta_buf, 0, sizeof(struct meta_buffer));
	memset(&pq_svp_md, 0, sizeof(struct meta_pq_disp_svp));

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}
	if (buffer == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		ret = -EINVAL;
		goto out;
	}

	s_dev = pq_dev->display_info.secure_info.svp_enable;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_SVP, "PQ is %s-device, received cap buf iova=0x%llx\n",
			s_dev?"s":"ns", buffer->frame_buf.iova);

	if (s_dev) {
		/* authorize win buf */
		ret = _mtk_pq_auth_buf(pq_dev, &buffer->frame_buf);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"auth window buf fail\n");
			goto out;
		}

		pq_svp_md.aid = (u8)pq_dev->display_info.secure_info.aid;
		pq_svp_md.pipelineid = pq_dev->display_info.secure_info.disp_pipeline_ID;
		pq_dev->display_info.secure_info.capbuf_valid = true;
	}

	/* write metadata for render */
	meta_buf.paddr = (unsigned char *)buffer->meta_buf.va;
	meta_buf.size = (unsigned int)buffer->meta_buf.size;

	ret = mtk_pq_common_write_metadata_addr(&meta_buf,
		EN_PQ_METATAG_SVP_INFO, (void *)&pq_svp_md);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Write svp meta tag fail\n");
		goto out;
	}

out:
	return ret;
}

int mtk_pq_svp_out_streamon(struct mtk_pq_device *pq_dev)
{
	int ret = 0;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}

	if (pq_dev->display_info.secure_info.svp_enable) {
		/* authorize pq internal buffer */
		ret = _mtk_pq_auth_internal_buf(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "auth pq internal buf fail\n");
			return ret;
		}

		pq_dev->display_info.secure_info.first_frame = true;
	}
	return 0;
}

int mtk_pq_svp_out_streamoff(struct mtk_pq_device *pq_dev)
{
	int ret = 0;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}

	if (pq_dev->display_info.secure_info.svp_enable) {
		/* unauthorize pq internal buffer */
		ret = _mtk_pq_unauth_internal_buf(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unauthorize pq internal buf fail\n");
			return ret;
		}

		// Patch for OPTEE auto release pipeline id
		if (!pq_dev->display_info.secure_info.capbuf_valid) {
			pq_dev->display_info.secure_info.disp_pipeline_valid = false;
			pq_dev->display_info.secure_info.disp_pipeline_ID = 0;
		}
	}
	return 0;
}

int mtk_pq_svp_cap_streamoff(struct mtk_pq_device *pq_dev)
{
	int ret = 0;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}

	// Patch for OPTEE auto release pipeline id
	if (pq_dev->display_info.secure_info.svp_enable)
		pq_dev->display_info.secure_info.capbuf_valid = false;

	return 0;
}

MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

#endif // IS_ENABLED(CONFIG_OPTEE)
