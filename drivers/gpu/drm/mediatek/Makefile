# SPDX-License-Identifier: GPL-2.0

KBUILD_EXTRA_SYMBOLS += ${MDRV_MTKDRV_HWREG_SRC_DIR}/Module.symvers
KBUILD_EXTRA_SYMBOLS += ${MDRV_MTKDRV_XCALG_SRC_DIR}/Module.symvers

ifeq (y, $(filter y, $(CONFIG_DRM_MEDIATEK_V2)))
obj-$(CONFIG_DRM_MEDIATEK) += mediatek_v2/
else

ifeq ($(CONFIG_DRM_MEDIATEK_TV_LEGACY),y)
SUB_FOLDER = mt5870
else
SUB_FOLDER = merak
endif

mediatek-drm-y := mtk_disp_color.o \
		  mtk_disp_ovl.o \
		  mtk_disp_rdma.o \
		  mtk_drm_crtc.o \
		  mtk_drm_ddp.o \
		  mtk_drm_ddp_comp.o \
		  mtk_drm_drv.o \
		  mtk_drm_fb.o \
		  mtk_drm_gem.o \
		  mtk_drm_plane.o \
		  mtk_dsi.o \
		  mtk_mipi_tx.o \
		  mtk_dpi.o
FULL_MTK_TV_SRC_PATH=$(src)
ccflags-y := -DMSOS_TYPE_LINUX_KERNEL
ccflags-y += -DSTI_PLATFORM_BRING_UP
ifdef CONFIG_DRM_MEDIATEK_TV
ccflags-y += -DMTK_TV_DRIVER
endif
ccflags-y += -DCONFIG_GOP_AFBC_FEATURE
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/gop/msos/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/gop/api/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/gop/drv/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/ldm/api/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/ldm/drv/include/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/ldm/drv/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/xc/api/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/video_display/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/video_pnl/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/video_pixelshift/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/video_globalpq/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/common/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/metabuf/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/pqmap/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/autogen/
ccflags-y += -I$(FULL_MTK_TV_SRC_PATH)/MTK_TV/$(SUB_FOLDER)/pqu_metadata/
ccflags-y += -I$(MDRV_MTKDRV_SCRIPTMGT_SRC_DIR)/include
ccflags-y += -I$(MDRV_MTKDRV_HWREG_SRC_DIR)/include
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/pqu
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/common
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/common/linux_kernel
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/common
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/render/video_display
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/render/video_globalpq
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/render/video_pnl
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/render/video_pixelshift
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/render/common
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/pqu
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/pqu/drivers/meta/include
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/meta_utility
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/render/cvbso
ccflags-y += -I$(HWREG_HEADER_PATH)
ccflags-y += -I$(srctree)/drivers/iommu/mtk_iommu_dtv
ccflags-y += -I$(srctree)/drivers/soc/mediatek/tv_metadata/include
ccflags-y += -I$(MDRV_MTKDRV_XCALG_SRC_DIR)/include
ccflags-y += -I$(XCALG_HEADER_PATH)
ccflags-y += -I$(srctree)/drivers/soc/mediatek/mtk-memory
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/tv_cpuif/include
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/tv_cpuif
ccflags-y += -I$(KERNEL_PATH)/drivers/soc/mediatek/realtime

obj-$(CONFIG_DRM_MEDIATEK) += mediatek-drm.o

mediatek-drm-tv-objs := MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_gop_wrapper.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_kms.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_pattern.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_video.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_gop.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_ld.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_drv.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_fb.o
#mediatek-drm-tv-objs += mtk_tv_drm_fb.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_plane.o
#mediatek-drm-tv-objs += mtk_tv_drm_plane.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_crtc.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_encoder.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_connector.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_gem.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_tvdac.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/mtk_tv_drm_sync.o
#mediatek-drm-tv-objs += mtk_tv_drm_gem.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/gop/msos/sti_msos.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/gop/drv/drvGOP.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/gop/api/mapiGOP.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/gop/api/apiGOP.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/ldm/drv/drvLDM_sti.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/ldm/api/apiLDM.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_display/mtk_tv_drm_video_display.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_display/mtk_tv_drm_video_frc.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_display/mtk_tv_drm_video_frc_thermal.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_pnl/mtk_tv_drm_gfx_panel.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_pnl/mtk_tv_drm_extdev_panel.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_pnl/mtk_tv_drm_video_panel.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_globalpq/mtk_tv_drm_global_pq.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_globalpq/mtk_tv_drm_pnlgamma.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_globalpq/mtk_tv_drm_pqgamma.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_globalpq/mtk_tv_drm_demura.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_pnl/mtk_tv_drm_panel_common.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/video_pixelshift/mtk_tv_drm_video_pixelshift.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/common/mtk_tv_drm_common.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/common/mtk_tv_drm_common_irq.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/metabuf/mtk_tv_drm_metabuf.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/pqmap/mtk_tv_drm_pqmap.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/autogen/mtk_tv_drm_autogen.o
mediatek-drm-tv-objs += MTK_TV/$(SUB_FOLDER)/pqu_metadata/mtk_tv_drm_pqu_metadata.o

#ifeq ($(CONFIG_DRM_MEDIATEK_TV), m)
#mediatek-drm-tv-objs += ../drm_gem_cma_helper.o
#endif

obj-$(CONFIG_DRM_MEDIATEK_TV) += mediatek-drm-tv.o

mediatek-drm-hdmi-objs := mtk_cec.o \
			  mtk_hdmi.o \
			  mtk_hdmi_ddc.o \
                          mtk_mt2701_hdmi_phy.o \
			  mtk_mt8173_hdmi_phy.o \
			  mtk_hdmi_phy.o

obj-$(CONFIG_DRM_MEDIATEK_HDMI) += mediatek-drm-hdmi.o
endif
