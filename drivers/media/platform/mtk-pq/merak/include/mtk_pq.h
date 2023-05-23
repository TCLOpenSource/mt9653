/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_H
#define _MTK_PQ_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/videobuf2-v4l2.h>
#include <uapi/linux/mtk-v4l2-pq.h>
#include <uapi/linux/metadata_utility/m_pq.h>
#include <uapi/linux/metadata_utility/m_srccap.h>
#include <uapi/linux/metadata_utility/m_vdec.h>
#include "metadata_utility.h"

#include "mtk_pq_common.h"
#include "mtk_pq_b2r.h"
#include "mtk_pq_enhance.h"
#include "mtk_pq_display.h"
#include "mtk_pq_pattern.h"
#include "mtk_pq_dv.h"
#include "mtk_pq_idk.h"
#include "mtk_pq_thermal.h"

#define PQ_WIN_MAX_NUM			(16)
#define PQ_NEVENTS			(64)
#define PQU_INPUT_STREAM_OFF_DONE	(BIT(0))
#define PQU_OUTPUT_STREAM_OFF_DONE	(BIT(1))
#define PQU_CLOSE_WIN_DONE		(BIT(2))
#define PQ_FILE_PATH_LENGTH		(256)
#define PQ_IRQ_MAX_NUM			(16)

#define MAX_WIN_NUM (16)
#define MAX_DEV_NAME_LEN (16)

#define STI_PQ_LOG_LEVEL_ERROR		BIT(0)
#define STI_PQ_LOG_LEVEL_COMMON		BIT(1)
#define STI_PQ_LOG_LEVEL_ENHANCE	BIT(2)
#define STI_PQ_LOG_LEVEL_B2R		BIT(3)
#define STI_PQ_LOG_LEVEL_DISPLAY	BIT(4)
#define STI_PQ_LOG_LEVEL_XC		BIT(5)
#define STI_PQ_LOG_LEVEL_3D		BIT(6)
#define STI_PQ_LOG_LEVEL_BUFFER		BIT(7)
#define STI_PQ_LOG_LEVEL_SVP		BIT(8)
#define STI_PQ_LOG_LEVEL_IRQ		BIT(9)
#define STI_PQ_LOG_LEVEL_PATTERN	BIT(10)
#define STI_PQ_LOG_LEVEL_DBG		BIT(11)
#define STI_PQ_LOG_LEVEL_ALL		(BIT(12) - 1)

#define IS_INPUT_VGA(x)		(x == MTK_PQ_INPUTSRC_VGA)
#define IS_INPUT_ATV(x)		(x == MTK_PQ_INPUTSRC_TV)
#define IS_INPUT_CVBS(x)	((x >= MTK_PQ_INPUTSRC_CVBS) && (x < MTK_PQ_INPUTSRC_CVBS_MAX))
#define IS_INPUT_SVIDEO(x)	((x >= MTK_PQ_INPUTSRC_SVIDEO) && (x < MTK_PQ_INPUTSRC_SVIDEO_MAX))
#define IS_INPUT_YPBPR(x)	((x >= MTK_PQ_INPUTSRC_YPBPR) && (x < MTK_PQ_INPUTSRC_YPBPR_MAX))
#define IS_INPUT_SCART(x)	((x >= MTK_PQ_INPUTSRC_SCART) && (x < MTK_PQ_INPUTSRC_SCART_MAX))
#define IS_INPUT_HDMI(x)	((x >= MTK_PQ_INPUTSRC_HDMI) && (x < MTK_PQ_INPUTSRC_HDMI_MAX))
#define IS_INPUT_DVI(x)		((x >= MTK_PQ_INPUTSRC_DVI) && (x < MTK_PQ_INPUTSRC_DVI_MAX))
#define IS_INPUT_DTV(x)		((x >= MTK_PQ_INPUTSRC_DTV) && (x < MTK_PQ_INPUTSRC_DTV_MAX))
#define IS_INPUT_MM(x)	((x >= MTK_PQ_INPUTSRC_STORAGE) && (x < MTK_PQ_INPUTSRC_STORAGE_MAX))

#define IS_INPUT_B2R(x)		(IS_INPUT_DTV(x) || IS_INPUT_MM(x))
#define IS_INPUT_SRCCAP(x)	(IS_INPUT_VGA(x) || IS_INPUT_CVBS(x) || IS_INPUT_SVIDEO(x) || \
				IS_INPUT_YPBPR(x) || IS_INPUT_SCART(x) || IS_INPUT_HDMI(x) || \
				IS_INPUT_DVI(x) || IS_INPUT_ATV(x))

extern __u32 u32DbgLevel;
extern __u32 atrace_enable_pq;

#define STI_PQ_LOG(_dbgLevel_, _fmt, _args...) { \
	if ((_dbgLevel_ & u32DbgLevel) != 0) { \
		if (_dbgLevel_ & STI_PQ_LOG_LEVEL_ERROR) \
			pr_err("[STI PQ ERROR]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_COMMON) { \
			pr_info("[STI PQ COMMON]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_ENHANCE) { \
			pr_info("[STI PQ ENHANCE]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_B2R) { \
			pr_info("[STI PQ B2R]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_DISPLAY) { \
			pr_info("[STI PQ DISPLAY]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_XC) { \
			pr_info("[STI PQ XC]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_3D) { \
			pr_info("[STI PQ 3D]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_BUFFER) { \
			pr_info("[STI PQ BUFFER]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_SVP) {\
			pr_info("[STI PQ SVP]  [%s,%5d]"_fmt,\
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_IRQ) {\
			pr_info("[STI PQ IRQ]  [%s,%5d]"_fmt,\
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_PATTERN) {\
			pr_info("[STI PQ PATTERN]  [%s,%5d]"_fmt,\
			__func__, __LINE__, ##_args);\
		} \
		else if (_dbgLevel_ & STI_PQ_LOG_LEVEL_DBG) {\
			pr_info("[STI PQ DBG]  [%s,%5d]"_fmt,\
			__func__, __LINE__, ##_args);\
		} \
	} \
}


#define PQ_LABEL    "[PQ] "
#define PQ_MSG_DEBUG(format, args...) \
	pr_debug(PQ_LABEL format, ##args)
#define PQ_MSG_INFO(format, args...) \
	pr_info(PQ_LABEL format, ##args)
#define PQ_MSG_WARNING(format, args...) \
	pr_warn(PQ_LABEL format, ##args)
#define PQ_MSG_ERROR(format, args...) \
	pr_err(PQ_LABEL format, ##args)
#define PQ_MSG_FATAL(format, args...) \
	pr_crit(PQ_LABEL format, ##args)
#define PQ_CAPABILITY_SHOW(PQItem, PQItemCap) { \
	if (ssize > u8Size) \
		PQ_MSG_ERROR("Failed to get" PQItem "\n"); \
	else \
		ssize += snprintf(buf + ssize, u8Size - ssize, "%d\n", \
		PQItemCap); \
}
#define PQ_CAPABILITY_CHECK_RET(ret, PQItemCap)          \
	do { \
		if (ret) { \
			PQ_MSG_ERROR("Failed to get" PQItemCap "resource\r\n"); \
		} \
	} while (0)

#define PQ_DIVIDE_BASE_2	(2)
#define PQ_X_ALIGN		(2)
#define PQ_Y_ALIGN		(2)
#define PQ_ALIGN(val, align) (((val) + ((align) - 1)) & (~((align) - 1)))

#define MAX_SYSFS_SIZE (255)

extern int boottime_print(const char *s);
/*
 * enum mtk_ctx_state - The state of an MTK pq instance.
 * @MTK_STATE_FREE - default state when instance is created
 * @MTK_STATE_INIT - job instance is initialized
 * @MTK_STATE_ABORT - job should be aborted
 */
enum mtk_ctx_state {
	MTK_STATE_FREE = 0,
	MTK_STATE_INIT = 1,
	MTK_STATE_ABORT = 2,
};

struct mtk_pq_ctx {
	struct v4l2_fh fh;
	struct mtk_pq_device *pq_dev;
	struct v4l2_m2m_ctx *m2m_ctx;
	enum mtk_ctx_state state;
};

struct mtk_pq_m2m_dev {
	struct video_device *video_dev;
	struct v4l2_m2m_dev *m2m_dev;
	struct mtk_pq_ctx *ctx;
	int refcnt;
};

struct mtk_pq_caps {
	u32 u32XC_ED_TARGET_CLK;
	u32 u32XC_FN_TARGET_CLK;
	u32 u32XC_FS_TARGET_CLK;
	u32 u32XC_FD_TARGET_CLK;
	u32 u32TCH_color;
	u32 u32PreSharp;
	u32 u322D_Peaking;
	u32 u32LCE;
	u32 u323D_LUT;
	u32 u32DMA_SCL;
	u32 u32PQU;
	u32 u32Window_Num;
	u32 u32SRS_Support;
	u32 u32FULL_PQ_I_Register;
	u32 u32FULL_PQ_P_Register;
	u32 u32LITE_PQ_I_Register;
	u32 u32LITE_PQ_P_Register;
	u32 u32ZFD_PQ_I_Register;
	u32 u32ZFD_PQ_P_Register;
	u32 u32FULL_PQ_I_Performance;
	u32 u32FULL_PQ_P_Performance;
	u32 u32LITE_PQ_I_Performance;
	u32 u32LITE_PQ_P_Performance;
	u32 u32ZFD_PQ_I_Performance;
	u32 u32ZFD_PQ_P_Performance;
	u32 u32DSCL_MM;
	u32 u32DSCL_IPDMA;
	u32 u32LineDelay_MTK;
	u32 u32LineDelay_SRS_8K;
	u32 u32LineDelay_SRS_4K;
	u32 u32LineDelay_SRS_FHD;
	u32 u32LineDelay_SRS_Other;
	u32 u32Phase_I_Input;
	u32 u32Phase_I_Output;
	u32 u32Phase_P_Input;
	u32 u32Phase_P_Output;
	u32 u32Device_register_Num[MAX_WIN_NUM];
	u32 u32AISR_Support;
	u32 u32NREnginePhase;
	u32 u32MDWEnginePhase;
	u32 u32Memc;
	u32 u32Phase_IP2;
	u32 u32AISR_Version;
	u32 u32HSY_Support;
	u32 u32B2R_Version;
	u32 u32IRQ_Version;
	u32 u32CFD_PQ_Version;
	u32 u32MDW_Version;
	u32 u32Config_Version;
	u32 u32Idr_SwMode_Support;
	u32 u32DV_PQ_Version;
	u32 u32HSY_Version;
	u32 u32Qmap_Version;
};

/* refer to struct st_dv_config_hw_info */
struct mtk_pq_dv_config_hw_info {
	bool updatedFlag;
	__u32 hw_ver;  //ts domain dv hw version (include FE & BE)
};

struct mtk_pq_dma_buf {
	int fd;
	void *va;
	__u64 pa;
	__u64 iova;
	__u64 size;
	struct dma_buf *db;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct device *dev;
	void *priv;
};

struct mtk_pq_remap_buf {
	__u64 pa;
	__u64 va;
	__u32 size;
};

struct v4l2_pix_format_info {
	struct v4l2_pix_format_mplane v4l2_fmt_pix_idr_mp;
	struct v4l2_pix_format_mplane v4l2_fmt_pix_mdw_mp;
};

struct mtk_pq_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head	list;

	struct mtk_pq_dma_buf meta_buf;
	struct mtk_pq_dma_buf frame_buf;

	__u64 queue_time;
	__u64 dequeue_time;
	__u64 process_time;
};

struct mtk_pq_stream_info {
	bool streaming;
	enum v4l2_buf_type type;
	struct mtk_pq_dma_buf meta;
};

/*INFO OF PQ DD*/
struct mtk_pq_device {
	spinlock_t slock;
	unsigned int ref_count;
	wait_queue_head_t wait;
	unsigned int event_flag;
	__u8 stream_on_ref;
	__u64 memory_bus_base;
	bool trigger_pattern;
	struct mutex mutex;
	struct v4l2_device v4l2_dev;
	struct video_device video_dev;
	struct device *dev;
	struct mtk_pq_m2m_dev m2m;
	struct v4l2_subdev subdev_common;
	struct v4l2_subdev subdev_enhance;
	struct v4l2_subdev subdev_xc;
	struct v4l2_subdev subdev_b2r;
	struct v4l2_subdev subdev_display;

	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl_handler common_ctrl_handler;
	struct v4l2_ctrl_handler enhance_ctrl_handler;
	struct v4l2_ctrl_handler xc_ctrl_handler;
	struct v4l2_ctrl_handler b2r_ctrl_handler;
	struct v4l2_ctrl_handler display_ctrl_handler;

	__u16 dev_indx;    /*pq device ID or window number*/

	struct v4l2_common_info common_info;
	void *enhance_info;	//pointer to struct v4l2_enhance_info
	struct v4l2_b2r_info b2r_info;
	struct v4l2_display_info display_info;
	struct v4l2_pix_format_info pix_format_info;

	struct b2r_device b2r_dev;
	struct pq_pattern_info pattern_info;
	struct pq_pattern_status pattern_status;
	struct mtk_pq_stream_info input_stream;
	struct mtk_pq_stream_info output_stream;
	struct mtk_pq_dv_win_info dv_win_info;

	struct mtk_pq_dma_buf stream_meta;
	struct mtk_pq_remap_buf pqu_stream_meta;
	__u64 qbuf_meta_pool_addr;
	__u32 qbuf_meta_pool_size;
};

struct mtk_pq_platform_device {
	struct mtk_pq_device *pq_devs[PQ_WIN_MAX_NUM];
	struct device *dev;
	bool cus_dev;
	__u8 cus_id[PQ_WIN_MAX_NUM];

	struct b2r_device b2r_dev;
	struct mtk_pq_caps pqcaps;
	struct mtk_pq_enhance pq_enhance;
	struct display_device display_dev;
	struct mtk_pq_dv_ctrl dv_ctrl;
	__u8 pq_irq[PQ_IRQ_MAX_NUM];
	uint8_t usable_win;
	uint8_t pq_memory_index;
	struct mtk_pq_remap_buf hwmap_config;
	struct mtk_pq_remap_buf dv_config;
	struct mtk_pq_remap_buf DDBuf[MMAP_DDBUF_MAX];
};

extern struct mtk_pq_platform_device *pqdev;

#endif
