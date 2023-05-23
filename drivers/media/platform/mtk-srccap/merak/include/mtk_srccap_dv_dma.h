/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DV_DMA_H
#define MTK_SRCCAP_DV_DMA_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define SRCCAP_DV_MAX_PLANE_NUM (3)
#define SRCCAP_DV_DMA_BUS2IP_ADDR_OFFSET (0x20000000)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------
enum srccap_dv_dma_status {
	SRCCAP_DV_DMA_STATUS_DISABLE = 0,
	SRCCAP_DV_DMA_STATUS_ENABLE_FB,
	SRCCAP_DV_DMA_STATUS_ENABLE_FBL,
	SRCCAP_DV_DMA_STATUS_MAX
};

struct srccap_dv_init_in;
struct srccap_dv_init_out;
struct srccap_dv_set_fmt_mplane_in;
struct srccap_dv_set_fmt_mplane_out;
struct srccap_dv_streamon_in;
struct srccap_dv_streamon_out;
struct srccap_dv_streamoff_in;
struct srccap_dv_streamoff_out;
struct srccap_dv_qbuf_in;
struct srccap_dv_qbuf_out;
struct srccap_dv_dqbuf_in;
struct srccap_dv_dqbuf_out;

struct srccap_dv_dma_buf {
	bool     available;
	bool     valid;
	uint32_t size;
	uint64_t addr; /* by HW */
	uint64_t va;   /* by CPU */
	uint64_t addr_mplane[SRCCAP_DV_MAX_PLANE_NUM];
	uint32_t size_mplane[SRCCAP_DV_MAX_PLANE_NUM];
};

struct srccap_dv_dma_format_conversion {
	uint8_t ip_fmt;
	uint8_t dvdma_fmt;
};

struct srccap_dv_dma_mem_fmt {
	uint8_t  num_planes;
	uint8_t  num_bufs;
	bool     pack;
	struct   srccap_dv_dma_format_conversion fmt_conv;
	bool     argb;
	uint8_t  access_mode;
	uint8_t  bitmode_alpha;
	uint8_t  bitmode[SRCCAP_DV_MAX_PLANE_NUM];
	uint8_t  ce[SRCCAP_DV_MAX_PLANE_NUM];
	uint8_t  frame_num;
	uint8_t  rw_diff;
	uint32_t src_width;
	uint32_t src_height;
	uint32_t dma_width;
	uint32_t dma_height;
	uint32_t pitch[SRCCAP_DV_MAX_PLANE_NUM];       /* unit: n bit align */
	uint32_t frame_pitch[SRCCAP_DV_MAX_PLANE_NUM]; /* unit: byte */
};

struct srccap_dv_dma_info {
	enum     srccap_dv_dma_status dma_status; /* dma control */
	struct   srccap_dv_dma_buf buf;           /* dma buf allocation info */
	struct   srccap_dv_dma_mem_fmt mem_fmt;
	uint32_t mmap_size;
	uint64_t mmap_addr;
	uint8_t  w_index;
};

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------
int mtk_dv_dma_init(
	struct srccap_dv_init_in *in,
	struct srccap_dv_init_out *out);

int mtk_dv_dma_set_fmt_mplane(
	struct srccap_dv_set_fmt_mplane_in *in,
	struct srccap_dv_set_fmt_mplane_out *out);

int mtk_dv_dma_streamon(
	struct srccap_dv_streamon_in *in,
	struct srccap_dv_streamon_out *out);

int mtk_dv_dma_streamoff(
	struct srccap_dv_streamoff_in *in,
	struct srccap_dv_streamoff_out *out);

int mtk_dv_dma_qbuf(
	struct srccap_dv_qbuf_in *in,
	struct srccap_dv_qbuf_out *out);

int mtk_dv_dma_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out);

#endif  // MTK_SRCCAP_DV_DMA_H
