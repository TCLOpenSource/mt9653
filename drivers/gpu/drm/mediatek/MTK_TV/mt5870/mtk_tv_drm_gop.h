/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_DRM_GOP_H_
#define _MTK_DRM_GOP_H_

#include "mtk_tv_drm_gop_wrapper.h"

#define MTK_PLANE_DISPLAY_PIPE (3)
#define INVALID_NUM (0xff)

#define MTK_DRM_GOP_COLOR_MODE_DEFAULT  (0)
#define MTK_DRM_GOP_SRC_MAXLUM_DEFAULT  (500)

#ifndef BIT
#define BIT(_bit_)	(1 << (_bit_))
#endif

#ifndef BMASK
#define BMASK(_bits_)	(BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))
#endif

#define MAX_GOP_PLANE_NUM (8)

//common GOP capability
#define MTK_GOP_CAPS_OFFSET_GOPNUM_MSK BMASK(2:0)
#define MTK_GOP_CAPS_OFFSET_DRAMADDR_ISWORD_UNIT	(3)
#define MTK_GOP_CAPS_OFFSET_H_PIXELBASE	(4)
#define MTK_GOP_CAPS_OFFSET_VOP_PATHSEL	(5)

//GOP0 capability
#define MTK_GOP_CAPS_OFFSET_GOP0_AFBC	(6)
#define MTK_GOP_CAPS_OFFSET_GOP0_HMIRROR	(7)
#define MTK_GOP_CAPS_OFFSET_GOP0_VMIRROR	(8)
#define MTK_GOP_CAPS_OFFSET_GOP0_TRANSCOLOR	(9)
#define MTK_GOP_CAPS_OFFSET_GOP0_SCROLL	(10)
#define MTK_GOP_CAPS_OFFSET_GOP0_BLEND_XCIP	(11)
#define MTK_GOP_CAPS_OFFSET_GOP0_CSC	(12)
#define MTK_GOP_CAPS_OFFSET_GOP0_HDR_MODE_MSK BMASK(17:13)
#define MTK_GOP_CAPS_OFFSET_GOP0_HDR_MODE_SFT	(13)
	#define MTK_GOP_CAPS_OFFSET_GOP0_HLG	(13)
	#define MTK_GOP_CAPS_OFFSET_GOP0_HDR10	(14)
	#define MTK_GOP_CAPS_OFFSET_GOP0_HDR10PLUS	(15)
	#define MTK_GOP_CAPS_OFFSET_GOP0_TCH	(16)
	#define MTK_GOP_CAPS_OFFSET_GOP0_DV	(17)
#define MTK_GOP_CAPS_OFFSET_GOP0_HSCALING_MODE_MSK BMASK(22:18)
#define MTK_GOP_CAPS_OFFSET_GOP0_HSCALING_MODE_SFT	(18)
	#define MTK_GOP_CAPS_OFFSET_GOP0_H2TAP	(18)
	#define MTK_GOP_CAPS_OFFSET_GOP0_HDUPLICATE	(19)
	#define MTK_GOP_CAPS_OFFSET_GOP0_HNEAREST	(20)
	#define MTK_GOP_CAPS_OFFSET_GOP0_H4TAP	(21)
	#define MTK_GOP_CAPS_OFFSET_GOP0_H10TAP	(22)
#define MTK_GOP_CAPS_OFFSET_GOP0_VSCALING_MODE_MSK BMASK(27:23)
#define MTK_GOP_CAPS_OFFSET_GOP0_VSCALING_MODE_SFT	(23)
	#define MTK_GOP_CAPS_OFFSET_GOP0_VDUPLICATE	(23)
	#define MTK_GOP_CAPS_OFFSET_GOP0_VNEAREST	(24)
	#define MTK_GOP_CAPS_OFFSET_GOP0_VBILINEAR	(25)
	#define MTK_GOP_CAPS_OFFSET_GOP0_V4TAP	(26)
	#define MTK_GOP_CAPS_OFFSET_GOP0_V6TAP	(27)
#define MTK_GOP_CAPS_OFFSET_GOP0_H10V6_SWITCH	(28)

//GOP1~GOPn capability
#define MTK_GOP_CAPS_OFFSET_GOP_AFBC_COMMON	(0)
#define MTK_GOP_CAPS_OFFSET_GOP_HMIRROR_COMMON	(1)
#define MTK_GOP_CAPS_OFFSET_GOP_VMIRROR_COMMON	(2)
#define MTK_GOP_CAPS_OFFSET_GOP_TRANSCOLOR_COMMON	(3)
#define MTK_GOP_CAPS_OFFSET_GOP_SCROLL_COMMON	(4)
#define MTK_GOP_CAPS_OFFSET_GOP_BLEND_XCIP_COMMON	(5)
#define MTK_GOP_CAPS_OFFSET_GOP_CSC_COMMON	(6)
#define MTK_GOP_CAPS_OFFSET_GOP_HDR_MODE_MSK_COMMON BMASK(11:7)
#define MTK_GOP_CAPS_OFFSET_GOP_HDR_MODE_SFT_COMMON	(7)
	#define MTK_GOP_CAPS_OFFSET_GOP_HLG_COMMON	(7)
	#define MTK_GOP_CAPS_OFFSET_GOP_HDR10_COMMON	(8)
	#define MTK_GOP_CAPS_OFFSET_GOP_HDR10PLUS_COMMON	(9)
	#define MTK_GOP_CAPS_OFFSET_GOP_TCH_COMMON	(10)
	#define MTK_GOP_CAPS_OFFSET_GOP_DV_COMMON	(11)
#define MTK_GOP_CAPS_OFFSET_GOP_HSCALING_MODE_MSK_COMMON BMASK(16:12)
#define MTK_GOP_CAPS_OFFSET_GOP_HSCALING_MODE_SFT_COMMON	(12)
	#define MTK_GOP_CAPS_OFFSET_GOP_H2TAP_COMMON	(12)
	#define MTK_GOP_CAPS_OFFSET_GOP_HDUPLICATE_COMMON	(13)
	#define MTK_GOP_CAPS_OFFSET_GOP_HNEAREST_COMMON	(14)
	#define MTK_GOP_CAPS_OFFSET_GOP_H4TAP_COMMON	(15)
	#define MTK_GOP_CAPS_OFFSET_GOP_H10TAP_COMMON	(16)
#define MTK_GOP_CAPS_OFFSET_GOP_VSCALING_MODE_MSK_COMMON BMASK(21:17)
#define MTK_GOP_CAPS_OFFSET_GOP_VSCALING_MODE_SFT_COMMON	(17)
	#define MTK_GOP_CAPS_OFFSET_GOP_VDUPLICATE_COMMON	(17)
	#define MTK_GOP_CAPS_OFFSET_GOP_VNEAREST_COMMON	(18)
	#define MTK_GOP_CAPS_OFFSET_GOP_VBILINEAR_COMMON	(19)
	#define MTK_GOP_CAPS_OFFSET_GOP_V4TAP_COMMON	(20)
	#define MTK_GOP_CAPS_OFFSET_GOP_V6TAP_COMMON	(21)
#define MTK_GOP_CAPS_OFFSET_GOP_H10V6_SWITCH_COMMON	(22)

enum {
	E_GOP_PROP_HMIRROR,
	E_GOP_PROP_VMIRROR,
	E_GOP_PROP_HSTRETCH,
	E_GOP_PROP_VSTRETCH,
	E_GOP_PROP_AFBC_FEATURE, //immutable
	E_GOP_PROP_HUE,
	E_GOP_PROP_SATURATION,
	E_GOP_PROP_CONTRAST,
	E_GOP_PROP_BRIGHTNESS,
	E_GOP_PROP_RGAIN,
	E_GOP_PROP_GGAIN,
	E_GOP_PROP_BGAIN,
	E_GOP_PROP_COLOR_MODE,
	E_GOP_PROP_SRC_MAXLUM,
	E_GOP_PROP_PNL_CURLUM,
	E_GOP_PROP_MAX,
};

struct gop_prop {
	uint64_t prop_val[E_GOP_PROP_MAX];
};

struct gop_capability {
	uint8_t u8GOPNum;
	uint8_t u8IsDramAddrWordUnit;
	uint8_t u8IsPixelBaseHorizontal;
	uint8_t u8SupportVopPathSel;
	uint8_t u8SupportAFBC[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportHMirror[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportVMirror[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportTransColor[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportScroll[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportBlendXcIP[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportCSC[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportHDRMode[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportHScalingMode[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportVScalingMode[MAX_GOP_PLANE_NUM];
	uint8_t u8SupportH10V6Switch[MAX_GOP_PLANE_NUM];
};

struct mtk_gop_context {
	int GopNum;
	int nBootlogoGop;
	UTP_GOP_INIT_INFO utpGopInit;

	struct drm_property *drm_gop_plane_prop[E_GOP_PROP_MAX];
	struct gop_prop *gop_plane_properties;
	struct gop_capability gop_plane_capability;
	enum drm_color_encoding out_plane_color_encoding;
	enum drm_color_range out_plane_color_range;
	UTP_EN_GOP_DATA_FMT out_plane_color_format;
	UTP_GOP_CSC_PARAM *pCurrent_CSCParam;
	UTP_GOP_PNL_INFO *pPnlInfo;
};

int mtk_gop_init(struct device *dev, struct device *master,
	void *data, struct mtk_drm_plane **primary_plane,
	struct mtk_drm_plane **cursor_plane);
int mtk_drm_gop_suspend(struct platform_device *dev, pm_message_t state);
int mtk_drm_gop_resume(struct platform_device *dev);
int readDTB2GOPPrivate(struct mtk_gop_context *pctx_gop);
int mtk_gop_atomic_set_plane_property(struct mtk_drm_plane *mplane,
	struct drm_plane_state *state, struct drm_property *property,
	uint64_t val);
int mtk_gop_atomic_get_plane_property(struct mtk_drm_plane *mplane,
	const struct drm_plane_state *state, struct drm_property *property,
	uint64_t *val);
int mtk_gop_bootlogo_ctrl(struct mtk_tv_drm_crtc *crtc, unsigned int cmd, unsigned int *gop);
int mtk_gop_atomic_set_crtc_property(struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state, struct drm_property *property,
	uint64_t val);
int mtk_gop_atomic_get_crtc_property(struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state, struct drm_property *property, uint64_t *val);
void mtk_gop_disable_plane(struct mtk_plane_state *state);
void mtk_gop_update_plane(struct mtk_plane_state *state);
int mtk_gop_enable_vblank(struct mtk_tv_drm_crtc *crtc);
void mtk_gop_disable_vblank(struct mtk_tv_drm_crtc *crtc);
void mtk_gop_enable(struct mtk_tv_drm_crtc *crtc);
void mtk_gop_disable(struct mtk_tv_drm_crtc *crtc);

#endif
