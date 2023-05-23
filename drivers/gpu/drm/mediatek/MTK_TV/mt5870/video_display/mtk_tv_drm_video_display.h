/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_VIDEO_DISPLAY_H_
#define _MTK_TV_DRM_VIDEO_DISPLAY_H_

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define MTK_PLANE_DISPLAY_PIPE (3)

//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------

enum ext_video_plane_prop {
	EXT_VPLANE_PROP_MUTE_SCREEN,
	EXT_VPLANE_PROP_SNOW_SCREEN,
	EXT_VPLANE_PROP_WINDOW_ALPHA,
	EXT_VPLANE_PROP_WINDOW_R,
	EXT_VPLANE_PROP_WINDOW_G,
	EXT_VPLANE_PROP_WINDOW_B,
	EXT_VPLANE_PROP_BACKGROUND_ALPHA,
	EXT_VPLANE_PROP_BACKGROUND_R,
	EXT_VPLANE_PROP_BACKGROUND_G,
	EXT_VPLANE_PROP_BACKGROUND_B,
	EXT_VPLANE_PROP_VIDEO_PLANE_TYPE,
	EXT_VPLANE_PROP_META_FD,
	EXT_VPLANE_PROP_FREEZE,
	EXT_VPLANE_PROP_INPUT_VFREQ,
	EXT_VPLANE_PROP_MAX,
};

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
struct mtk_video_plane_type_num {
	uint8_t MEMC_num;
	uint8_t MGW_num;
	uint8_t DMA1_num;
	uint8_t DMA2_num;
};

struct video_plane_prop {
	uint64_t prop_val[EXT_VPLANE_PROP_MAX];//new prop value
	uint64_t old_prop_val[EXT_VPLANE_PROP_MAX];
	uint8_t prop_update[EXT_VPLANE_PROP_MAX];
};

struct mtk_video_context {
	struct drm_property *drm_video_plane_prop[EXT_VPLANE_PROP_MAX];
	struct video_plane_prop *video_plane_properties;
	struct mtk_video_plane_type_num videoPlaneType_PlaneNum;
	uint8_t videoPlaneType_TypeNum;
	uint16_t byte_per_word;
	uint16_t reg_num;
};

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_video_display_init(
	struct device *dev,
	struct device *master,
	void *data,
	struct mtk_drm_plane **primary_plane,
	struct mtk_drm_plane **cursor_plane);
void mtk_video_display_update_plane(
	struct mtk_plane_state *state);
void mtk_video_display_disable_plane(
	struct mtk_plane_state *state);
int mtk_video_display_atomic_set_plane_property(
	struct mtk_drm_plane *mplane,
	struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t val);
int mtk_video_display_atomic_get_plane_property(
	struct mtk_drm_plane *mplane,
	const struct drm_plane_state *state,
	struct drm_property *property,
	uint64_t *val);
int mtk_video_check_plane(
	struct drm_plane_state *plane_state,
	const struct drm_crtc_state *crtc_state,
	struct mtk_plane_state *state);

#endif
