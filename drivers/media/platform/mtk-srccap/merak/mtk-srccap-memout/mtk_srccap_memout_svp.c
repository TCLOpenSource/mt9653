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
#include <linux/version.h>
#include <asm/uaccess.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#include "mtk_srccap.h"
#include "mtk_srccap_memout.h"
#include "mtk_srccap_memout_svp.h"
#include "hwreg_srccap_memout.h"
#include "mtk_srccap_common_ca.h"
#include "mtk_iommu_dtv_api.h"
#include "linux/metadata_utility/m_srccap.h"
#include "utpa2_XC.h"

MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

static int _mtk_memout_teec_enable(struct mtk_srccap_dev *srccap_dev)
{
	u32 tee_error = 0;
	int ret = 0;
	TEEC_UUID uuid = DISP_TA_UUID;
	TEEC_Context *pstContext =
			srccap_dev->memout_info.secure_info.pstContext;
	TEEC_Session *pstSession =
			srccap_dev->memout_info.secure_info.pstSession;

	if ((pstSession != NULL) && (pstContext != NULL)) {
		ret = mtk_common_ca_init_context(srccap_dev, pstContext);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] Init Context failed!\n");
			mtk_common_ca_finalize_context(srccap_dev, pstContext);
			return -EPERM;
		}

		ret = mtk_common_ca_open_session(srccap_dev, pstContext, pstSession, &uuid,
					NULL, &tee_error);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] TEEC Open pstSession failed with error (%u)\n",
				tee_error);
			mtk_common_ca_close_session(srccap_dev, pstSession);
			mtk_common_ca_finalize_context(srccap_dev, pstContext);
			return -EPERM;
		}
	} else {
		SRCCAP_MSG_ERROR("[SVP] pstSession or pstContext is NULL.\n");
		return -EINVAL;
	}

	return 0;
}

static void _mtk_memout_teec_disable(struct mtk_srccap_dev *srccap_dev)
{
	TEEC_Context *pstContext =
			srccap_dev->memout_info.secure_info.pstContext;
	TEEC_Session *pstSession =
			srccap_dev->memout_info.secure_info.pstSession;

	if ((pstContext != NULL) && (pstSession != NULL)) {
		mtk_common_ca_close_session(srccap_dev, pstSession);
		mtk_common_ca_finalize_context(srccap_dev, pstContext);
	} else {
		SRCCAP_MSG_ERROR("[SVP] pstSession or pstContext is NULL.\n");
	}
}

static int _mtk_memout_auth_buf(struct mtk_srccap_dev *srccap_dev, int buf_fd)
{
	int ret = 0;
	u32 pipelineID = 0;
	unsigned long long iova = 0;
	unsigned int size = 0;

	/* get device parameter */
	pipelineID = srccap_dev->memout_info.secure_info.video_pipeline_ID;
	if (pipelineID == 0) {
		SRCCAP_MSG_INFO("[SVP] Invalid video pipelineID = 0\n");
		return -EINVAL;
	}

	/* get buf iova & size */
	ret = mtkd_iommu_getiova(buf_fd, &iova, &size);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP] Get buf IOVA fail, fd = %d\n", buf_fd);
		return -EACCES;
	}
	SRCCAP_MSG_DEBUG("[SVP] buf iova = 0x%llx, size = 0x%llx\n", iova, size);

	/* authorize source buf */
	ret = mtkd_iommu_buffer_authorize(22, iova, size, pipelineID);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP]iommu authorize fail\n");
		return -EPERM;
	}

	return ret;
}

static int _mtk_memout_svp_smc_call(struct mtk_srccap_dev *srccap_dev,
			EN_XC_OPTEE_ACTION action, TEEC_Operation *op)
{
	u32 error = 0;
	int ret = 0;
	TEEC_Session *pstSession = srccap_dev->memout_info.secure_info.pstSession;

	if (pstSession == NULL) {
		SRCCAP_MSG_ERROR("[SVP] pstSession is NULL!\n");
		return -EINVAL;
	}
	if (op == NULL) {
		SRCCAP_MSG_ERROR("[SVP] para is NULL!\n");
		return -EINVAL;
	}

	SRCCAP_MSG_INFO("[SVP] SRCCAP SMC cmd:%u\n", action);

	ret = mtk_common_ca_teec_invoke_cmd(srccap_dev, pstSession, (u32)action, op, &error);
	if (ret) {
		if (action == E_XC_OPTEE_DESTROY_VIDEO_PIPELINE)
			ret = 0; //The cmd fail is acceptable while destroy
		else {
			ret = -EPERM;
			SRCCAP_MSG_ERROR("[SVP] invoke cmd failed with errorno:(%x)\n", error);
		}
	}

	return ret;
}

int mtk_memout_svp_init(struct mtk_srccap_dev *srccap_dev)
{
	TEEC_Context *pstContext = NULL;
	TEEC_Session *pstSession = NULL;

	pstContext = malloc(sizeof(TEEC_Context));
	if (pstContext == NULL) {
		SRCCAP_MSG_ERROR("[SVP] pstContext malloc fail!\n");
		return -EPERM;
	}

	pstSession = malloc(sizeof(TEEC_Session));
	if (pstSession == NULL) {
		SRCCAP_MSG_ERROR("[SVP] pstSession malloc fail!\n");
		free(pstContext);
		return -EPERM;
	}

	memset(pstContext, 0, sizeof(TEEC_Context));
	memset(pstSession, 0, sizeof(TEEC_Session));

	if (srccap_dev->memout_info.secure_info.pstContext != NULL) {
		free(srccap_dev->memout_info.secure_info.pstContext);
		srccap_dev->memout_info.secure_info.pstContext = NULL;
	}

	if (srccap_dev->memout_info.secure_info.pstSession != NULL) {
		free(srccap_dev->memout_info.secure_info.pstSession);
		srccap_dev->memout_info.secure_info.pstSession = NULL;
	}

	srccap_dev->memout_info.secure_info.pstContext = pstContext;
	srccap_dev->memout_info.secure_info.pstSession = pstSession;
	srccap_dev->memout_info.secure_info.optee_version = 3;
	srccap_dev->memout_info.secure_info.svp_enable = 0;
	srccap_dev->memout_info.secure_info.aid = 0;
	srccap_dev->memout_info.secure_info.video_pipeline_ID = 0;

	if (_mtk_memout_teec_enable(srccap_dev)) {
		SRCCAP_MSG_ERROR("[SVP] teec enable fail\n");
		return -EPERM;
	}

	SRCCAP_MSG_INFO("[SVP] SVP init success.\n");
	return 0;
}

void mtk_memout_svp_exit(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	struct v4l2_memout_secure_info secure_info;
	TEEC_Context *pstContext = NULL;
	TEEC_Session *pstSession = NULL;

	memset(&secure_info, 0, sizeof(struct v4l2_memout_secure_info));
	ret = mtk_memout_set_secure_mode(srccap_dev, &secure_info);
	_mtk_memout_teec_disable(srccap_dev);

	pstContext = srccap_dev->memout_info.secure_info.pstContext;
	pstSession = srccap_dev->memout_info.secure_info.pstSession;

	if (pstContext != NULL) {
		free(pstContext);
		srccap_dev->memout_info.secure_info.pstContext = NULL;
	}
	if (pstSession != NULL) {
		free(pstSession);
		srccap_dev->memout_info.secure_info.pstSession = NULL;
	}

	SRCCAP_MSG_INFO("[SVP] SVP exit success.\n");
}

int mtk_memout_set_secure_mode(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_memout_secure_info *secureinfo)
{
	int ret = 0;
	u32 pipeline_id = 0;
	EN_XC_STI_AID aid = E_XC_STI_AID_NS;
	XC_STI_OPTEE_HANDLER optee_handler;
	struct reg_srccap_memout_sstinfo hwreg_sstinfo;
	TEEC_Operation op = {0};

	if (secureinfo == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	/* prepare handler */
	memset(&optee_handler, 0, sizeof(XC_STI_OPTEE_HANDLER));
	memset(&hwreg_sstinfo, 0, sizeof(struct reg_srccap_memout_sstinfo));

	/* prepare handler */
	optee_handler.u16Version = XC_STI_OPTEE_HANDLER_VERSION;
	optee_handler.u16Length = sizeof(XC_STI_OPTEE_HANDLER);

	/* enable secure mode */
	if (secureinfo->bEnable) {
		SRCCAP_MSG_INFO("[SVP] ENABLE Secure Mode\n");

		/* read aid from SST */
		ret = drv_hwreg_srccap_memout_get_sst(&hwreg_sstinfo);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] HWREG get sst fail/n");
			return -EINVAL;
		}
		switch (srccap_dev->srccap_info.src) {
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
			aid = (EN_XC_STI_AID)hwreg_sstinfo.secure_hdmi_0;
			break;
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
			aid = (EN_XC_STI_AID)hwreg_sstinfo.secure_hdmi_1;
			break;
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
			aid = (EN_XC_STI_AID)hwreg_sstinfo.secure_hdmi_2;
			break;
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
			aid = (EN_XC_STI_AID)hwreg_sstinfo.secure_hdmi_3;
			break;
		default:
			SRCCAP_MSG_ERROR("[SVP] Unsupported SVP extin source %d/n",
				srccap_dev->srccap_info.src);
			return -EINVAL;
		}

		if (secureinfo->bForceSVP)
			aid = E_XC_STI_AID_S;

		if (aid == E_XC_STI_AID_NS) {
			SRCCAP_MSG_ERROR("[SVP] Get invalid aid from sst %d/n", aid);
			return -EINVAL;
		}

		optee_handler.enAID = aid;
		op.params[SRCCAP_CA_SMC_PARAM_IDX_0].tmpref.buffer = (void *)&optee_handler;
		op.params[SRCCAP_CA_SMC_PARAM_IDX_0].tmpref.size =
				sizeof(XC_STI_OPTEE_HANDLER);
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
			TEEC_NONE, TEEC_NONE, TEEC_NONE);
		ret = _mtk_memout_svp_smc_call(srccap_dev,
				E_XC_OPTEE_CREATE_VIDEO_PIPELINE,
				&op);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] smc call CREATE_VIDEO_PIPELINE fail\n");
			return ret;
		}
		pipeline_id = optee_handler.u32VdoPipelineID;
		SRCCAP_MSG_INFO("[SVP] aid=%d, video pipeline ID=0x%x\n", aid, pipeline_id);
	}
	/* disable secure mode */
	else {
		optee_handler.u32VdoPipelineID =
				srccap_dev->memout_info.secure_info.video_pipeline_ID;
		if (srccap_dev->memout_info.secure_info.aid < E_XC_STI_AID_MAX)
			optee_handler.enAID = srccap_dev->memout_info.secure_info.aid;
		op.params[SRCCAP_CA_SMC_PARAM_IDX_0].tmpref.buffer = (void *)&optee_handler;
		op.params[SRCCAP_CA_SMC_PARAM_IDX_0].tmpref.size =
				sizeof(XC_STI_OPTEE_HANDLER);
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
			TEEC_NONE, TEEC_NONE, TEEC_NONE);
		if ((optee_handler.u32VdoPipelineID != 0) &&
			(optee_handler.enAID != E_XC_STI_AID_NS)) {
			SRCCAP_MSG_INFO("[SVP] DISABLE Secure Mode\n");
			ret = _mtk_memout_svp_smc_call(srccap_dev,
					E_XC_OPTEE_DESTROY_VIDEO_PIPELINE,
					&op);
			if (ret) {
				SRCCAP_MSG_ERROR("[SVP] smc call: DESTROY_VIDEO_PIPELINE fail\n");
				return ret;
			}
		}
		pipeline_id = 0;
		aid = E_XC_STI_AID_NS;
	}

	/* store svp info in device */
	srccap_dev->memout_info.secure_info.aid = (u8)aid;
	srccap_dev->memout_info.secure_info.svp_enable = secureinfo->bEnable;
	srccap_dev->memout_info.secure_info.video_pipeline_ID = pipeline_id;
	return ret;
}

int mtk_memout_get_sst(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_memout_sst_info *sst_info)
{
	int ret = 0;
	struct reg_srccap_memout_sstinfo hwreg_sstinfo;

	if (sst_info == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&hwreg_sstinfo, 0, sizeof(struct reg_srccap_memout_sstinfo));
	ret = drv_hwreg_srccap_memout_get_sst(&hwreg_sstinfo);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP] HWREG get sst fail/n");
		return -EINVAL;
	}

	memset(sst_info, 0, sizeof(struct v4l2_memout_sst_info));
	if (hwreg_sstinfo.secure_hdmi_0)
		sst_info->HDMI1_secure = true;
	if (hwreg_sstinfo.secure_hdmi_1)
		sst_info->HDMI2_secure = true;
	if (hwreg_sstinfo.secure_hdmi_2)
		sst_info->HDMI3_secure = true;
	if (hwreg_sstinfo.secure_hdmi_3)
		sst_info->HDMI4_secure = true;
	return ret;
}

int mtk_memout_cfg_buf_security(
	struct mtk_srccap_dev *srccap_dev,
	s32 buf_fd,
	s32 meta_fd)
{
	int ret = 0;
	struct meta_srccap_svp_info svp_meta;

	memset(&svp_meta, 0, sizeof(struct meta_srccap_svp_info));

	/* lock each buf queued into s-DD */
	if (srccap_dev->memout_info.secure_info.svp_enable) {
		ret = _mtk_memout_auth_buf(srccap_dev, buf_fd);
		if (ret) {
			SRCCAP_MSG_ERROR("[SVP] buf authorize fail\n");
			return -EPERM;
		}
	}

	/* set AID & pipeline id in metadata */
	svp_meta.aid = srccap_dev->memout_info.secure_info.aid;
	svp_meta.pipelineid =
		srccap_dev->memout_info.secure_info.video_pipeline_ID;

	SRCCAP_MSG_INFO("[SVP] aid=%d, video pipeline ID=0x%x\n",
		svp_meta.aid, svp_meta.pipelineid);

	ret = mtk_memout_write_metadata(meta_fd,
		SRCCAP_MEMOUT_METATAG_SVP_INFO, &svp_meta);
	if (ret) {
		SRCCAP_MSG_ERROR("[SVP] write svp meta tag fail\n");
		return -EPERM;
	}

	return ret;
}

#endif // IS_ENABLED(CONFIG_OPTEE)
