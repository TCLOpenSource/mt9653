MTK_JPD_PATH=$(srctree)/drivers/media/platform/mtk-jpd
ccflags-y += -I$(MTK_JPD_PATH)/njpd
ccflags-y += -I$(MTK_JPD_PATH)/njpd/api
ccflags-y += -I$(MTK_JPD_PATH)/njpd/drv
ccflags-y += -I$(MTK_JPD_PATH)/njpd/hal

ccflags-y += -Idrivers/iommu/mtk_iommu_dtv
ccflags-y += -Idrivers/staging/android/uapi
ccflags-y += -DMSOS_TYPE_LINUX_KERNEL
ccflags-y += -Werror

obj-$(CONFIG_VIDEO_MEDIATEK_JPD)       := mtk-v4l2-jpd.o
mtk-v4l2-jpd-y += mtk_jpd_core.o
mtk-v4l2-jpd-y += mtk_platform_jpd.o
mtk-v4l2-jpd-y += njpd/api/mtk_api_jpd.o
mtk-v4l2-jpd-y += njpd/drv/mtk_drv_jpd.o
mtk-v4l2-jpd-y += njpd/hal/mtk_hal_jpd.o
mtk-v4l2-jpd-y += njpd/mtk_sti_msos.o