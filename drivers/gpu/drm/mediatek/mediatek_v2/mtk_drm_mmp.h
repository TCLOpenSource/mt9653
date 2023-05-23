/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_DRM_MMP_H__
#define __MTK_DRM_MMP_H__

#include "mmprofile.h"
#include "mmprofile_function.h"
#include "mtk_drm_ddp.h"

#define MMP_CRTC_NUM 3

/* if changed, need to update init_drm_mmp_event() */
struct DRM_MMP_Events {
	mmp_event drm;
	mmp_event crtc[MMP_CRTC_NUM];

	/* define for IRQ */
	mmp_event IRQ;
	mmp_event ovl;
	mmp_event ovl0;
	mmp_event ovl1;
	mmp_event ovl0_2l;
	mmp_event ovl1_2l;
	mmp_event ovl2_2l;
	mmp_event ovl3_2l;
	mmp_event rdma;
	mmp_event rdma0;
	mmp_event rdma1;
	mmp_event wdma;
	mmp_event wdma0;
	mmp_event dsi;
	mmp_event dsi0;
	mmp_event dsi1;
	mmp_event ddp;
	mmp_event mutex[DISP_MUTEX_DDP_COUNT];
	mmp_event postmask;
	mmp_event postmask0;
	mmp_event abnormal_irq;
	mmp_event pmqos;
	mmp_event hrt_bw;
	mmp_event mutex_lock;
	mmp_event layering;
	mmp_event dma_alloc;
	mmp_event dma_free;
	mmp_event ion_import_dma;
	mmp_event ion_import_fd;
	mmp_event ion_import_free;
	mmp_event set_mode;
};

/* if changed, need to update init_crtc_mmp_event() */
struct CRTC_MMP_Events {
	mmp_event trig_loop_done;
	mmp_event enable;
	mmp_event disable;
	mmp_event release_fence;
	mmp_event update_present_fence;
	mmp_event release_present_fence;
	mmp_event atomic_begin;
	mmp_event atomic_flush;
	mmp_event enable_vblank;
	mmp_event disable_vblank;
	mmp_event esd_check;
	mmp_event esd_recovery;
	mmp_event leave_idle;
	mmp_event enter_idle;
	mmp_event frame_cfg;
	mmp_event suspend;
	mmp_event resume;
	mmp_event dsi_suspend;
	mmp_event dsi_resume;
	mmp_event backlight;
	mmp_event backlight_grp;
	mmp_event ddic_send_cmd;
	mmp_event ddic_read_cmd;
	mmp_event path_switch;
	mmp_event user_cmd;
	mmp_event check_trigger;
	mmp_event kick_trigger;
	mmp_event atomic_commit;
	mmp_event user_cmd_cb;
	mmp_event bl_cb;
	mmp_event clk_change;
	mmp_event layerBmpDump;
	mmp_event layer_dump[6];
};

struct DRM_MMP_Events *get_drm_mmp_events(void);
struct CRTC_MMP_Events *get_crtc_mmp_events(unsigned long id);
void drm_mmp_init(void);
int mtk_drm_mmp_ovl_layer(struct mtk_plane_state *state,
			  u32 downSampleX, u32 downSampleY);

/* print mmp log for DRM_MMP_Events */
#define DRM_MMP_MARK(event, v1, v2)                                            \
	mmprofile_log_ex(get_drm_mmp_events()->event,                  \
			 MMPROFILE_FLAG_PULSE, v1, v2)

#define DRM_MMP_EVENT_START(event, v1, v2)                                     \
	mmprofile_log_ex(get_drm_mmp_events()->event,                  \
			 MMPROFILE_FLAG_START, v1, v2)

#define DRM_MMP_EVENT_END(event, v1, v2)                                       \
	mmprofile_log_ex(get_drm_mmp_events()->event,                  \
			 MMPROFILE_FLAG_END, v1, v2)

/* print mmp log for CRTC_MMP_Events */
#define CRTC_MMP_MARK(id, event, v1, v2)                                       \
	do {								\
		if (id >= 0 && id < MMP_CRTC_NUM)                              \
			mmprofile_log_ex(get_crtc_mmp_events(id)->event,       \
					 MMPROFILE_FLAG_PULSE, v1, v2);       \
	} while (0)

#define CRTC_MMP_EVENT_START(id, event, v1, v2)                                \
	do {								\
		if (id >= 0 && id < MMP_CRTC_NUM)                              \
			mmprofile_log_ex(get_crtc_mmp_events(id)->event,       \
					 MMPROFILE_FLAG_START, v1, v2);       \
	} while (0)

#define CRTC_MMP_EVENT_END(id, event, v1, v2)                                  \
	do {								\
		if (id >= 0 && id < MMP_CRTC_NUM)                              \
			mmprofile_log_ex(get_crtc_mmp_events(id)->event,       \
					 MMPROFILE_FLAG_END, v1, v2);       \
	} while (0)

#endif
