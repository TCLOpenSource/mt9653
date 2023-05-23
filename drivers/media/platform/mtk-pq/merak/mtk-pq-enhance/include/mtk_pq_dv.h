/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_DV_H
#define _MTK_PQ_DV_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include <uapi/linux/metadata_utility/m_pq.h>

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define MTK_PQ_DV_LABEL "[PQ][DV]"

#define MTK_PQ_DV_BIT(_bit_) (1 << (_bit_))

#define MTK_PQ_DV_DBG_LEVEL_ERR       MTK_PQ_DV_BIT(0)
#define MTK_PQ_DV_DBG_LEVEL_INFO      MTK_PQ_DV_BIT(1)
#define MTK_PQ_DV_DBG_LEVEL_DEBUG     MTK_PQ_DV_BIT(2)
#define MTK_PQ_DV_DBG_LEVEL_PR        MTK_PQ_DV_BIT(3)

#define MTK_PQ_DV_LOG_TRACE(_dbg_level_, _fmt, _args...) \
{ \
	if (((_dbg_level_) & g_mtk_pq_dv_debug_level) != 0) { \
		if ((_dbg_level_) & MTK_PQ_DV_DBG_LEVEL_ERR) { \
			pr_err(MTK_PQ_DV_LABEL "[Error][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else if ((_dbg_level_) & MTK_PQ_DV_DBG_LEVEL_INFO) { \
			pr_err(MTK_PQ_DV_LABEL "[Info][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else if ((_dbg_level_) & MTK_PQ_DV_DBG_LEVEL_DEBUG) { \
			pr_err(MTK_PQ_DV_LABEL "[Debug][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else if ((_dbg_level_) & MTK_PQ_DV_DBG_LEVEL_PR) { \
			pr_err(MTK_PQ_DV_LABEL "[PR][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else { \
			pr_err(MTK_PQ_DV_LABEL "[%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} \
	} \
}

#define MTK_PQ_DV_LOG_CHECK_POINTER(errno) \
	do { \
		MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR, "pointer is NULL\n"); \
		ret = (errno); \
	} while (0)

#define MTK_PQ_DV_LOG_CHECK_RETURN(result, errno) \
	do { \
		MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR, "return is %d\n", (result)); \
		ret = (errno); \
	} while (0)

#define MTK_PQ_DV_DBG_MAX_SET_REG_NUM (20)
#define MTK_PQ_DV_PYR_NUM (7)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------
struct mtk_pq_device;
struct mtk_pq_buffer;

struct mtk_pq_dv_common {
	// require extra n frames before calc first queued frame
	uint8_t extra_frame_num;
};

struct mtk_pq_dv_debug_reg {
	uint32_t addr;
	uint32_t val;
	uint32_t mask;
};

struct mtk_pq_dv_debug_level {
	bool en;
	uint32_t debug_level_pqu;
	uint32_t debug_level_3pty;
};

struct mtk_pq_dv_debug_force_reg {
	bool en;
	uint8_t reg_num;
	struct mtk_pq_dv_debug_reg regs[MTK_PQ_DV_DBG_MAX_SET_REG_NUM];
};

struct mtk_pq_dv_debug_force_pr {
	bool    extra_frame_num_valid;
	uint8_t extra_frame_num;
	bool    pr_en_valid;
	bool    pr_en;
};

/* refer to struct m_pq_dv_debug_force_view_mode */
struct mtk_pq_dv_debug_force_view_mode {
	bool en;
	uint8_t view_id;
};

/* refer to struct m_pq_dv_debug_force_control */
struct mtk_pq_dv_debug_force_control {
	struct mtk_pq_dv_debug_force_view_mode view_mode;
};

struct mtk_pq_dv_pyr_buf {
	bool     valid;
	uint8_t  frame_num;
	uint8_t  rw_diff;
	uint32_t size;
	uint64_t addr; /* by HW */
	uint64_t va;   /* by CPU */
	uint32_t frame_pitch[MTK_PQ_DV_PYR_NUM]; /* unit: n bit align */
	uint32_t pyr_size[MTK_PQ_DV_PYR_NUM];    /* unit: byte */
	uint64_t pyr_addr[MTK_PQ_DV_PYR_NUM];    /* unit: n bit align */
	uint32_t pyr_width[MTK_PQ_DV_PYR_NUM];
	uint32_t pyr_height[MTK_PQ_DV_PYR_NUM];
};

struct mtk_pq_dv_pr_ctrl {
	bool     en;
	bool     is_dolby;
	bool     pre_is_dolby;
	uint32_t width;
	uint32_t height;
};

/* dolby info for each window */
struct mtk_pq_dv_win_info {
	struct mtk_pq_dv_common common;
	struct mtk_pq_dv_pyr_buf pyr_buf;
	struct mtk_pq_dv_pr_ctrl pr_ctrl;
	struct meta_pq_dv_ambient ambient;
};

struct mtk_pq_dv_pyr_ctrl {
	__u32 mmap_size;
	__u64 mmap_addr;
	bool available;
};

struct mtk_pq_dv_debug {
	struct mtk_pq_dv_debug_level debug_level;
	struct mtk_pq_dv_debug_force_reg force_reg;
	struct mtk_pq_dv_debug_force_control force_ctrl;
	struct mtk_pq_dv_debug_force_pr force_pr;
};

/* global dolby info */
struct mtk_pq_dv_ctrl {
	struct mtk_pq_dv_pyr_ctrl pyr_ctrl;
	struct mtk_pq_dv_debug debug;
};

struct mtk_pq_dv_ctrl_init_in {
	__u16 version;
	struct mtk_pq_platform_device *pqdev;
	__u32 mmap_size;
	__u64 mmap_addr;
};

struct mtk_pq_dv_ctrl_init_out {
	__u16 version;
};

struct mtk_pq_dv_win_init_in {
	__u16 version;
	struct mtk_pq_device *dev;
};

struct mtk_pq_dv_win_init_out {
	__u16 version;
};

struct mtk_pq_dv_streamon_in {
	__u16 version;
	struct mtk_pq_device *dev;
};

struct mtk_pq_dv_streamon_out {
	__u16 version;
};

struct mtk_pq_dv_streamoff_in {
	__u16 version;
	struct mtk_pq_device *dev;
};

struct mtk_pq_dv_streamoff_out {
	__u16 version;
};

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------
extern int g_mtk_pq_dv_debug_level;

int mtk_pq_dv_ctrl_init(
	struct mtk_pq_dv_ctrl_init_in *in,
	struct mtk_pq_dv_ctrl_init_out *out);

int mtk_pq_dv_win_init(
	struct mtk_pq_dv_win_init_in *in,
	struct mtk_pq_dv_win_init_out *out);

int mtk_pq_dv_streamon(
	struct mtk_pq_dv_streamon_in *in,
	struct mtk_pq_dv_streamon_out *out);

int mtk_pq_dv_streamoff(
	struct mtk_pq_dv_streamoff_in *in,
	struct mtk_pq_dv_streamoff_out *out);

int mtk_pq_dv_set_ambient(
	void *ctrl, struct mtk_pq_device *pd_dev);

int mtk_pq_dv_qbuf(
	struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf);

int mtk_pq_dv_show(
	struct device *dev,
	const char *buf);

int mtk_pq_dv_store(
	struct device *dev,
	const char *buf);

#endif  // _MTK_PQ_DV_H
