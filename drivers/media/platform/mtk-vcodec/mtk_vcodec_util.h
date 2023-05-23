/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
* Copyright (c) 2016 MediaTek Inc.
* Author: PC Chen <pc.chen@mediatek.com>
*	Tiffany Lin <tiffany.lin@mediatek.com>
*/

#ifndef _MTK_VCODEC_UTIL_H_
#define _MTK_VCODEC_UTIL_H_

#include <linux/types.h>
#include <linux/dma-direction.h>

#ifdef CONFIG_MSTAR_CHIP
#ifndef TV_INTEGRATION
#define TV_INTEGRATION
#endif
#endif

/* #define FPGA_PWRCLK_API_DISABLE */
/* #define FPGA_INTERRUPT_API_DISABLE */

struct mtk_vcodec_mem {
	size_t length;
	size_t size;
	size_t data_offset;
	void *va;
	dma_addr_t dma_addr;
	struct dma_buf *dmabuf;
	__u32 flags;
	__u32 index;
	__s64 buf_fd;
	// output info after GET_PARAM_FREE_BITSTREAM_BUFFER
	bool gen_frame; // true if this buffer could result in a complete frame
	struct hdr10plus_info *hdr10plus_buf;
};

/**
 * enum flags  - decoder different operation types
 * @NO_CAHCE_FLUSH	: no need to proceed cache flush
 * @NO_CAHCE_INVALIDATE	: no need to proceed cache invalidate
 * @CROP_CHANGED	: frame buffer crop changed
 * @REF_FREED	: frame buffer is reference freed
 */
enum mtk_vcodec_flags {
	NO_CAHCE_CLEAN = 1,
	NO_CAHCE_INVALIDATE = 1 << 1,
	CROP_CHANGED = 1 << 2,
	REF_FREED = 1 << 3
};

enum mtk_put_buffer_type {
	PUT_BUFFER_WORKER = -1,
	PUT_BUFFER_CALLBACK = 0,
};

struct mtk_vcodec_ctx;
struct mtk_vcodec_dev;
enum mtk_instance_state;
struct mtk_video_dec_buf;

extern int mtk_v4l2_dbg_level;
extern bool mtk_vcodec_dbg;
extern bool mtk_vcodec_perf;

#define DEBUG   1

typedef enum {
	VCODEC_DBG_L0 = 0,
	VCODEC_DBG_L1 = 1,
	VCODEC_DBG_L2 = 2,
	VCODEC_DBG_L3 = 3,
	VCODEC_DBG_L4 = 4,
	VCODEC_DBG_L5 = 5,
	VCODEC_DBG_L6 = 6,
	VCODEC_DBG_L7 = 7,
	VCODEC_DBG_L8 = 8,
} mtk_vcodec_debug_level;

#if defined(DEBUG)

#define mtk_v4l2_debug(level, fmt, args...)                              \
	do {                                                             \
		if ((mtk_v4l2_dbg_level & level) == level)           \
			pr_info("[MTK_V4L2] level=%d %s(),%d: " fmt "\n",\
				level, __func__, __LINE__, ##args);      \
	} while (0)

#define mtk_v4l2_err(fmt, args...)                \
	pr_err("[MTK_V4L2][ERROR] %s:%d: " fmt "\n", __func__, __LINE__, \
		   ##args)


#define mtk_v4l2_debug_enter()  mtk_v4l2_debug(8, "+")
#define mtk_v4l2_debug_leave()  mtk_v4l2_debug(8, "-")

#define mtk_vcodec_debug(h, fmt, args...)                               \
	do {                                                            \
		if (mtk_vcodec_dbg)                                  \
			pr_info("[MTK_VCODEC][%d]: %s() " fmt "\n",     \
				((struct mtk_vcodec_ctx *)h->ctx)->id,  \
				__func__, ##args);                      \
	} while (0)

#define mtk_vcodec_perf_log(fmt, args...)                               \
	do {                                                            \
		if (mtk_vcodec_perf)                          \
			pr_info("[MTK_PERF] " fmt "\n", ##args);        \
	} while (0)


#define mtk_vcodec_err(h, fmt, args...)                                 \
	pr_info("[MTK_VCODEC][ERROR][%d]: %s() " fmt "\n",               \
		   ((struct mtk_vcodec_ctx *)h->ctx)->id, __func__, ##args)

#define mtk_vcodec_debug_enter(h)  mtk_vcodec_debug(h, "+")
#define mtk_vcodec_debug_leave(h)  mtk_vcodec_debug(h, "-")

#else

#define mtk_v4l2_debug(level, fmt, args...)
#define mtk_v4l2_err(fmt, args...)
#define mtk_v4l2_debug_enter()
#define mtk_v4l2_debug_leave()

#define mtk_vcodec_debug(h, fmt, args...)
#define mtk_vcodec_err(h, fmt, args...)
#define mtk_vcodec_debug_enter(h)
#define mtk_vcodec_debug_leave(h)

#endif

void __iomem *mtk_vcodec_get_dec_reg_addr(struct mtk_vcodec_ctx *data,
	unsigned int reg_idx);
void __iomem *mtk_vcodec_get_enc_reg_addr(struct mtk_vcodec_ctx *data,
	unsigned int reg_idx);
int mtk_vcodec_mem_alloc(struct mtk_vcodec_ctx *data,
	struct mtk_vcodec_mem *mem);
void mtk_vcodec_mem_free(struct mtk_vcodec_ctx *data,
	struct mtk_vcodec_mem *mem);
void mtk_vcodec_set_curr_ctx(struct mtk_vcodec_dev *dev,
	struct mtk_vcodec_ctx *ctx, unsigned int hw_id);
struct mtk_vcodec_ctx *mtk_vcodec_get_curr_ctx(struct mtk_vcodec_dev *dev,
	unsigned int hw_id);
bool flush_abort_state(enum mtk_instance_state state);
struct vdec_fb *mtk_vcodec_get_fb(struct mtk_vcodec_ctx *ctx);
int mtk_vdec_put_fb(struct mtk_vcodec_ctx *ctx, enum mtk_put_buffer_type type);
int mtk_dma_sync_sg_range(const struct sg_table *sgt,
	struct device *dev, unsigned int size,
	enum dma_data_direction direction);
void v4l_fill_mtk_fmtdesc(struct v4l2_fmtdesc *fmt);
void mtk_vcodec_init_slice_info(struct mtk_vcodec_ctx *ctx, struct mtk_video_dec_buf *dst_buf_info);
void mtk_vcodec_add_ctx_list(struct mtk_vcodec_ctx *ctx);
void mtk_vcodec_del_ctx_list(struct mtk_vcodec_ctx *ctx);

#endif /* _MTK_VCODEC_UTIL_H_ */
