/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_COMMON_H
#define MTK_SRCCAP_COMMON_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/mtk-v4l2-srccap.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

#include "mtk_srccap_common_irq.h"

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
#define SRCCAP_COMMON_FHD_W (1920)
#define SRCCAP_COMMON_FHD_H (1080)
#define SRCCAP_COMMON_SW_IRQ_DLY (20)
#define SRCCAP_COMMON_PQ_IRQ_DLY (20)

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum srccap_irq_interrupts {
	SRCCAP_HDMI_IRQ_START,
	SRCCAP_HDMI_IRQ_PHY = SRCCAP_HDMI_IRQ_START,
	SRCCAP_HDMI_IRQ_MAC,
	SRCCAP_HDMI_IRQ_PKT_QUE,
	SRCCAP_HDMI_IRQ_PM_SQH_ALL_WK,
	SRCCAP_HDMI_IRQ_PM_SCDC,
	SRCCAP_HDMI_FIQ_CLK_DET_0,
	SRCCAP_HDMI_FIQ_CLK_DET_1,
	SRCCAP_HDMI_FIQ_CLK_DET_2,
	SRCCAP_HDMI_FIQ_CLK_DET_3,
	SRCCAP_HDMI_IRQ_END,
	SRCCAP_XC_IRQ_START = SRCCAP_HDMI_IRQ_END,
	SRCCAP_XC_IRQ = SRCCAP_XC_IRQ_START,
	SRCCAP_XC_IRQ_END,
	SRCCAP_VBI_IRQ_START = SRCCAP_XC_IRQ_END,
	SRCCAP_VBI_IRQ = SRCCAP_VBI_IRQ_START,
	SRCCAP_VBI_IRQ_END,
};

enum srccap_common_device_num {
	SRCCAP_COMMON_DEVICE_0 = 0,
	SRCCAP_COMMON_DEVICE_1,
};

enum E_SRCCAP_FILE_TYPE {
	E_SRCCAP_FILE_PIM_JSON,
	E_SRCCAP_FILE_RM_JSON,
	E_SRCCAP_FILE_RULELIST_JSON,
	E_SRCCAP_FILE_HWREG_JSON,
	E_SRCCAP_FILE_SWREG_JSON,
	E_SRCCAP_FILE_MAX,
};

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct srccap_ml_script_info;
struct hdmi_avi_packet_info;

struct srccap_common_pqmap_info {
	bool	inited;
	void	*pqmap_va;
	char	*pim_va;
	uint32_t pim_len;
	void	*pPim;
	void	*alg_ctx;
	void	*hwreg_func_table_addr;
};

struct mtk_srccap_dev;

struct srccap_main_color_info {
	u8 data_format;
	u8 data_range;
	u8 bit_depth;
	u8 color_primaries;
	u8 transfer_characteristics;
	u8 matrix_coefficients;
};

struct srccap_color_info {
	struct srccap_main_color_info in;
	struct srccap_main_color_info out;
	struct srccap_main_color_info dip_point;
	u32 chroma_sampling_mode;
	u8 hdr_type;
};

struct srccap_common_info {
	struct srccap_color_info color_info;
	struct srccap_common_irq_info irq_info;
	u8 srccap_handled_top_irq[SRCCAP_COMMON_IRQ_MAX_NUM][SRCCAP_COMMON_IRQTYPE_MAX];
	struct srccap_common_pqmap_info pqmap_info;
	struct srccap_common_pqmap_info vdpqmap_info;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_common_updata_color_info(struct mtk_srccap_dev *srccap_dev);
int mtk_common_updata_frame_color_info(struct mtk_srccap_dev *srccap_dev);
int mtk_common_get_cfd_metadata(struct mtk_srccap_dev *srccap_dev,
	struct meta_srccap_color_info *cfd_meta);
int mtk_common_set_cfd_setting(struct mtk_srccap_dev *srccap_dev);
int mtk_common_subscribe_event_vsync(struct mtk_srccap_dev *srccap_dev);
int mtk_common_unsubscribe_event_vsync(struct mtk_srccap_dev *srccap_dev);
int mtk_common_init_triggergen(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_register_common_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_common,
	struct v4l2_ctrl_handler *common_ctrl_handler);
void mtk_srccap_unregister_common_subdev(
	struct v4l2_subdev *subdev_common);
int mtk_srccap_common_set_pqmap_info(struct v4l2_srccap_pqmap_info *info,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_get_pqmap_info(struct v4l2_srccap_pqmap_info *info,
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_load_pqmap(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_ml_script_info *ml_script_info);
int mtk_srccap_common_load_vd_pqmap(
	struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_deinit_pqmap(
	struct mtk_srccap_dev *srccap_dev);
void mtk_srccap_common_set_hdmi_420to444
	(struct mtk_srccap_dev *srccap_dev);
void mtk_srccap_common_set_hdmi422pack
	(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_streamon(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_stream_off(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_pq_hwreg_init(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_pq_hwreg_deinit(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_vd_pq_hwreg_init(struct mtk_srccap_dev *srccap_dev);
int mtk_srccap_common_vd_pq_hwreg_deinit(struct mtk_srccap_dev *srccap_dev);
void mtk_srccap_common_set_stub_mode(u8 stub);
bool mtk_srccap_common_is_Cfd_stub_mode(void);
int mtk_srccap_common_get_cfd_test_HDMI_pkt_HDRIF(struct meta_srccap_hdr_pkt *phdr_pkt);
int mtk_srccap_common_get_cfd_test_HDMI_pkt_AVI(struct hdmi_avi_packet_info *pavi_pkt);
int mtk_srccap_common_get_cfd_test_HDMI_pkt_VSIF(struct hdmi_vsif_packet_info *vsif_pkt);



#endif  // MTK_SRCCAP_COMMON_H
