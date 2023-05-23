# Makefile for MTK Source Capture device drivers

#KBUILD_EXTRA_SYMBOLS += $(PREBUILD_SRC_DIR)/$(PRODUCT_NAME_DTB)/linux/linuxkernel/common/symver/scriptmgt/Module.symvers

MTK_EARC_PATH=$(src)
ccflags-y := -DMSOS_TYPE_LINUX_KERNEL
ccflags-y += -DSTI_PLATFORM_BRING_UP

ccflags-y += -I$(MTK_EARC_PATH)/
ccflags-y += -I$(MTK_EARC_PATH)/include


ccflags-y += -I$(HWREG_HEADER_PATH)
ccflags-y += -I$(HWREG_HEADER_PATH)/common
ccflags-y += -I$(HWREG_HEADER_PATH)/common/linux_kernel
ccflags-y += -I$(HWREG_HEADER_PATH)/earc/

ccflags-y += -I$(srctree)/drivers/iommu/mtk_iommu_dtv
ccflags-y += -I$(srctree)/drivers/soc/mediatek/tv_metadata/include
ccflags-y += -I$(MDRV_MTKDRV_SCRIPTMGT_SRC_DIR)/include


ccflags-y += -I$(MTK_EARC_PATH)/utp
ccflags-y += -I$(MTK_EARC_PATH)/utp/drv/earc/include
ccflags-y += -I$(MTK_EARC_PATH)/utp/drv/earc


CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))

mtk-earc-objs := mtk_earc.o
mtk-earc-objs += mtk_earc_sysfs.o

mtk-earc-objs += utp/sti_msos.o
mtk-earc-objs += utp/drv/earc/drvEARC.o

#obj-$(CONFIG_VIDEO_MEDIATEK_EARC)+= mtk-earc.o
#obj-y += mtk-earc.o
obj-m += mtk-earc.o
