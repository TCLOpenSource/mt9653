# SPDX-License-Identifier: GPL-2.0
MTK_PLATFORM := mtk-tv

obj-$(CONFIG_VIDEO_MEDIATEK_VCODEC) += mtk-vcodec-dec.o \
				       mtk-vcodec-enc.o \
				       mtk-vcodec-common.o \

mtk-vcodec-dec-y := mtk_vcodec_dec_drv.o \
		vdec_drv_if.o \
		mtk_vcodec_dec.o \
		$(MTK_PLATFORM)/mtk_vcodec_dec_pm.o \

ifdef CONFIG_VIDEO_MEDIATEK_VCU
	mtk-vcodec-dec-y += vdec/vdec_common_if.o \
		vdec_vcu_if.o
endif
ifdef CONFIG_VIDEO_MEDIATEK_VPU
	mtk-vcodec-dec-y += vdec/vdec_h264_if.o \
		vdec/vdec_vp8_if.o \
		vdec/vdec_vp9_if.o \
		vdec_vpu_if.o
endif

mtk-vcodec-enc-y := mtk_vcodec_enc_drv.o \
		venc_drv_if.o \
		mtk_vcodec_enc.o \
		$(MTK_PLATFORM)/mtk_vcodec_enc_pm.o \

ifdef CONFIG_VIDEO_MEDIATEK_VCU
	mtk-vcodec-enc-y += venc/venc_common_if.o \
		venc_vcu_if.o
endif
ifdef CONFIG_VIDEO_MEDIATEK_VPU
	mtk-vcodec-enc-y += venc/venc_vp8_if.o \
		venc/venc_h264_if.o \
		venc_vpu_if.o
endif

mtk-vcodec-common-y := $(MTK_PLATFORM)/mtk_vcodec_intr.o \
		mtk_vcodec_util.o\
		mtk-iommu-dtv-contig.o \
		mtk_vcodec_fence.o \

subdir-ccflags-y += -Werror -Wall
ifdef CONFIG_VIDEO_MEDIATEK_VCU
	ccflags-y += -I$(srctree)/drivers/media/platform/mtk-vcu
endif
ifdef CONFIG_VIDEO_MEDIATEK_VPU
	ccflags-y += -I$(srctree)/drivers/media/platform/mtk-vpu
endif
ifdef CONFIG_MSTAR_CHIP
ccflags-y += -I$(MDRV_MTKDRV_METAUTIL_SRC_DIR)/../include/meta_utility
endif
ccflags-y += -DCONFIG_VIDEO_MEDIATEK_VCU
ccflags-y += -I$(srctree)/drivers/media/platform/mtk-vcodec
ccflags-y += -I$(srctree)/drivers/media/platform/mtk-vcodec/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/iommu/mtk_iommu_dtv

KBUILD_EXTRA_SYMBOLS += $(MDRV_MTKDRV_METAUTIL_SRC_DIR)/Module.symvers
