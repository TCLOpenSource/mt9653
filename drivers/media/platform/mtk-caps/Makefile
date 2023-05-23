
#KBUILD_EXTRA_SYMBOLS += $(PREBUILD_SRC_DIR)/$(PRODUCT_NAME_DTB)/linux/linuxkernel/common/symver/scriptmgt/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(MDRV_MTKDRV_EFUSE_SRC_DIR)/Module.symvers

ccflags-y += -I$(KERNELDIR)/include/media
ccflags-y += -I$(MDRV_MTKDRV_SCRIPTMGT_SRC_DIR)/include
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/common
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/common/linux_kernel
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/meta_utility
ccflags-y += -I$(srctree)/drivers/iommu/mtk_iommu_dtv
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/efuse

mtk-caps-objs := mtk_dip.o mtk_dip_svc.o

obj-$(CONFIG_VIDEO_MEDIATEK_CAPS)	+= mtk-caps.o
