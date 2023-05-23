/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DV_H
#define MTK_SRCCAP_DV_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include "mtk_srccap.h"
#include "mtk_srccap_common_irq.h"
#include "mtk_srccap_dv_common.h"
#include "mtk_srccap_dv_descrb.h"
#include "mtk_srccap_dv_dma.h"
#include "mtk_srccap_dv_sd.h"
#include "mtk_srccap_dv_meta.h"
#include "mtk_srccap_dv_utility.h"
#include "mtk_srccap_dv_version.h"

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------

#define IS_SRCCAP_DV_STUB_MODE() \
	(stub_srccap_dv >= SRCCAP_STUB_MODE_DV \
	&& stub_srccap_dv < SRCCAP_STUB_MODE_DV_MAX)

#define IS_SRCCAP_DV_STUB_MODE_VSIF() \
	((stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_H14B) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_STD) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_LL_RGB) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_LL_YUV) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_NEG))

#define IS_SRCCAP_DV_STUB_MODE_EMP() \
	((stub_srccap_dv == SRCCAP_STUB_MODE_DV_EMP_FORM1) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_EMP_FORM2_RGB) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_EMP_FORM2_YUV) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_EMP_NEG))

#define IS_SRCCAP_DV_STUB_MODE_RGB() \
	((stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_LL_RGB) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_EMP_FORM2_RGB) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_DRM_LL_RGB))

#define IS_SRCCAP_DV_STUB_MODE_LL_FORM2() \
	((stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_LL_RGB) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_LL_YUV) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_EMP_FORM2_RGB) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_EMP_FORM2_YUV))

#define IS_SRCCAP_DV_STUB_MODE_H14B() \
	(stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_H14B)

#define IS_SRCCAP_DV_STUB_MODE_DRM() \
	((stub_srccap_dv == SRCCAP_STUB_MODE_DV_DRM_LL_RGB) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_DRM_LL_YUV))

#define IS_SRCCAP_DV_STUB_MODE_NEG() \
	((stub_srccap_dv == SRCCAP_STUB_MODE_DV_VSIF_NEG) \
	|| (stub_srccap_dv == SRCCAP_STUB_MODE_DV_EMP_NEG))


//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------
struct srccap_dv_info {
	struct srccap_dv_descrb_info descrb_info;
	struct srccap_dv_dma_info dma_info;
};

struct srccap_dv_init_in {
	__u16 version;
	struct mtk_srccap_dev *dev;
	__u32 mmap_size;
	__u64 mmap_addr;
};

struct srccap_dv_init_out {
	__u16 version;
};

struct srccap_dv_set_fmt_mplane_in {
	__u16 version;
	struct mtk_srccap_dev *dev;
	struct v4l2_format *fmt;
	__u8 frame_num;
	__u8 rw_diff;
};

struct srccap_dv_set_fmt_mplane_out {
	__u16 version;
};

struct srccap_dv_streamon_in {
	__u16 version;
	struct mtk_srccap_dev *dev;
	enum v4l2_buf_type buf_type;
	bool dma_available;
};

struct srccap_dv_streamon_out {
	__u16 version;
};

struct srccap_dv_streamoff_in {
	__u16 version;
	struct mtk_srccap_dev *dev;
	enum v4l2_buf_type buf_type;
};

struct srccap_dv_streamoff_out {
	__u16 version;
};

struct srccap_dv_qbuf_in {
	__u16 version;
	struct mtk_srccap_dev *dev;
	struct vb2_buffer *vb;
};

struct srccap_dv_qbuf_out {
	__u16 version;
};

struct srccap_dv_dqbuf_in {
	__u16 version;
	struct mtk_srccap_dev *dev;
	struct v4l2_buffer *buf;
	bool avi_valid; // valid bit of avi
	struct hdmi_avi_packet_info *avi;
	__u16 vsif_num; // number of received vsif
	struct hdmi_vsif_packet_info *vsif;
	__u16 emp_num; // number of received emp
	struct hdmi_emp_packet_info *emp;
	__u16 drm_num; // number of received drm
	struct hdmi_hdr_packet_info *drm;
};

struct srccap_dv_dqbuf_out {
	__u16 version;
};

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------

extern enum srccap_stub_mode stub_srccap_dv;


int mtk_dv_init(
	struct srccap_dv_init_in *in,
	struct srccap_dv_init_out *out);

void mtk_dv_resume(
	struct mtk_srccap_dev *srccap_dev);

int mtk_dv_set_fmt_mplane(
	struct srccap_dv_set_fmt_mplane_in *in,
	struct srccap_dv_set_fmt_mplane_out *out);

int mtk_dv_streamon(
	struct srccap_dv_streamon_in *in,
	struct srccap_dv_streamon_out *out);

int mtk_dv_streamoff(
	struct srccap_dv_streamoff_in *in,
	struct srccap_dv_streamoff_out *out);

int mtk_dv_qbuf(
	struct srccap_dv_qbuf_in *in,
	struct srccap_dv_qbuf_out *out);

int mtk_dv_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out);

void mtk_dv_handle_irq(
	void *param);

int mtk_dv_show(
	struct device *dev,
	const char *buf);

int mtk_dv_store(
	struct device *dev,
	const char *buf);

int mtk_dv_set_stub(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_stub_mode stub);


#endif  // MTK_SRCCAP_DV_H
