/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#if IS_ENABLED(CONFIG_OPTEE)
#ifndef _MTK_PQ_SVP_H
#define _MTK_PQ_SVP_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/tee_drv.h>
#include <uapi/linux/tee.h>
#include <uapi/linux/metadata_utility/m_vdec.h>
#include <uapi/linux/metadata_utility/m_srccap.h>
#include <tee_client_api.h>

#define DISP_TA_UUID {0x4dd53ca0, 0x0248, 0x11e6, \
	{0x86, 0xc0, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b} }

#define CHECK_AID_VALID(x) \
	(((x >= PQ_AID_NS) && (x < PQ_AID_MAX))?(true):(false))

enum mtk_pq_aid {
	PQ_AID_NS = 0,
	PQ_AID_SDC,
	PQ_AID_S,
	PQ_AID_CSP,
	PQ_AID_MAX,
};

struct pq_secure_info {
	int optee_version;
	TEEC_Context *pstContext;
	TEEC_Session *pstSession;
	bool svp_enable;
	enum mtk_pq_aid aid;
	u32 vdo_pipeline_ID;
	bool disp_pipeline_valid;
	u32 disp_pipeline_ID;
	bool pq_buf_authed;
	bool first_frame;
	bool capbuf_valid;
};

struct mtk_pq_device;

// function
int mtk_pq_svp_init(struct mtk_pq_device *pq_dev);
void mtk_pq_svp_exit(struct mtk_pq_device *pq_dev);
int mtk_pq_svp_set_sec_md(
	struct mtk_pq_device *pq_dev,
	u8 *secure_mode_flag);
int mtk_pq_svp_cfg_outbuf_sec(
	struct mtk_pq_device *pq_dev,
	struct v4l2_buffer *buffer);
int mtk_pq_svp_cfg_capbuf_sec(
	struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *buffer);
int mtk_pq_svp_out_streamon(struct mtk_pq_device *pq_dev);
int mtk_pq_svp_out_streamoff(struct mtk_pq_device *pq_dev);
int mtk_pq_svp_cap_streamoff(struct mtk_pq_device *pq_dev);

#endif
#endif // IS_ENABLED(CONFIG_OPTEE)
