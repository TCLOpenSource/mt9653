/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DV_DESCRB_H
#define MTK_SRCCAP_DV_DESCRB_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

#include "mtk_srccap.h"

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define SRCCAP_DV_DESCRB_BUF_NUM (2)
#define SRCCAP_DV_DESCRB_MAX_HW_SUPPORT_VSIF (4)
#define SRCCAP_DV_DESCRB_VSIF_SIZE (0x1F)
#define SRCCAP_DV_DESCRB_MAX_HW_SUPPORT_EMP (16)
#define SRCCAP_DV_DESCRB_EMP_SIZE (0x1F)
#define SRCCAP_DV_DESCRB_DRM_SIZE (0x1E)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------
enum srccap_dv_descrb_interface {
	SRCCAP_DV_DESCRB_INTERFACE_NONE = 0,
	SRCCAP_DV_DESCRB_INTERFACE_SINK_LED,
	SRCCAP_DV_DESCRB_INTERFACE_SOURCE_LED_RGB,
	SRCCAP_DV_DESCRB_INTERFACE_SOURCE_LED_YUV,
	SRCCAP_DV_DESCRB_INTERFACE_DRM_SOURCE_LED_RGB,
	SRCCAP_DV_DESCRB_INTERFACE_DRM_SOURCE_LED_YUV,
	SRCCAP_DV_DESCRB_INTERFACE_FORM_1,
	SRCCAP_DV_DESCRB_INTERFACE_FORM_2_RGB,
	SRCCAP_DV_DESCRB_INTERFACE_FORM_2_YUV,
	SRCCAP_DV_DESCRB_INTERFACE_MAX
};

enum srccap_dv_descrb_color_format {
	SRCCAP_DV_DESCRB_COLOR_FORMAT_RGB = 0,
	SRCCAP_DV_DESCRB_COLOR_FORMAT_YUV_422,
	SRCCAP_DV_DESCRB_COLOR_FORMAT_YUV_444,
	SRCCAP_DV_DESCRB_COLOR_FORMAT_YUV_420,
	SRCCAP_DV_DESCRB_COLOR_FORMAT_RESERVED,
	SRCCAP_DV_DESCRB_COLOR_FORMAT_DEFAULT =
		SRCCAP_DV_DESCRB_COLOR_FORMAT_RGB,
	SRCCAP_DV_DESCRB_COLOR_FORMAT_UNKNOWN = 7,
};

struct srccap_dv_init_in;
struct srccap_dv_init_out;
struct srccap_dv_streamon_in;
struct srccap_dv_streamon_out;
struct srccap_dv_streamoff_in;
struct srccap_dv_streamoff_out;
struct srccap_dv_dqbuf_in;
struct srccap_dv_dqbuf_out;

struct srccap_dv_descrb_common {
	enum srccap_dv_descrb_interface interface;
	bool dv_game_mode;
	bool hdmi_422_pack_en;
};

struct srccap_dv_descrb_buf {
	uint8_t index;
	uint8_t *data[SRCCAP_DV_DESCRB_BUF_NUM];
	uint32_t data_length[SRCCAP_DV_DESCRB_BUF_NUM];
	bool crc[SRCCAP_DV_DESCRB_BUF_NUM];
	/* NEG control */
	enum m_dv_crc_state neg_ctrl;
	uint8_t DoViCtr;
	/* transition */
	bool in_transition;
};

struct srccap_dv_descrb_pkt_info {
	enum srccap_dv_descrb_color_format color_format;
	bool is_vsif_received;
	uint8_t vsif[SRCCAP_DV_DESCRB_VSIF_SIZE];
	bool is_vsem_received;
	uint8_t vsem[SRCCAP_DV_DESCRB_MAX_HW_SUPPORT_EMP * SRCCAP_DV_DESCRB_EMP_SIZE];
	uint16_t vsem_size;
	bool is_drm_received;
	uint8_t drm[SRCCAP_DV_DESCRB_DRM_SIZE];
};

struct srccap_dv_descrb_info {
	struct srccap_dv_descrb_common common;
	struct srccap_dv_descrb_buf buf;
	struct srccap_dv_descrb_pkt_info pkt_info;
};

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------
int mtk_dv_descrb_init(
	struct srccap_dv_init_in *in,
	struct srccap_dv_init_out *out);

int mtk_dv_descrb_streamon(
	struct srccap_dv_streamon_in *in,
	struct srccap_dv_streamon_out *out);

int mtk_dv_descrb_streamoff(
	struct srccap_dv_streamoff_in *in,
	struct srccap_dv_streamoff_out *out);

int mtk_dv_descrb_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out);

void mtk_dv_descrb_handle_irq(
	void *param);

// debug
int mtk_dv_descrb_set_stub(
	int stub);

int mtk_dv_descrb_stub_get_test_avi(
	void *ptr,
	MS_U32 u32_buf_len);

int mtk_dv_descrb_stub_get_test_vsif(
	void *ptr,
	MS_U32 u32_buf_len);

int mtk_dv_descrb_sutb_get_test_emp(
	void *ptr,
	MS_U32 u32_buf_len);

int mtk_dv_descrb_stub_get_test_drm(
	void *ptr,
	MS_U32 u32_buf_len);
#endif  // MTK_SRCCAP_DV_DESCRB_H
