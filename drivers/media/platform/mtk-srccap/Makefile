# Makefile for MTK Source Capture device drivers

#KBUILD_EXTRA_SYMBOLS += $(PREBUILD_SRC_DIR)/$(PRODUCT_NAME_DTB)/linux/linuxkernel/common/symver/scriptmgt/Module.symvers

#ifeq ($(CONFIG_VIDEO_MEDIATEK_SRCCAP_MERAK),y)
#SUB_FOLDER = merak
#else
#SUB_FOLDER = mt5870
#endif

ifeq ($(CONFIG_VIDEO_MEDIATEK_SRCCAP_LEGACY),y)
SUB_FOLDER = mt5870
else
SUB_FOLDER = merak
endif

MTK_SRCCAP_PATH=$(src)/$(SUB_FOLDER)
ccflags-y := -DMSOS_TYPE_LINUX_KERNEL
ccflags-y += -DSTI_PLATFORM_BRING_UP

ccflags-y += -I$(MTK_SRCCAP_PATH)
ccflags-y += -I$(MTK_SRCCAP_PATH)/include
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-adc
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-avd
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-common
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-pattern
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-dscl
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-hdmirx
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-memout
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-mux
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-timingdetect
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-vbi
ccflags-y += -I$(MTK_SRCCAP_PATH)/mtk-srccap-dv

ccflags-y += -I$(HWREG_HEADER_PATH)
ccflags-y += -I$(HWREG_HEADER_PATH)/common
ccflags-y += -I$(HWREG_HEADER_PATH)/common/linux_kernel
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/adc
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/avd
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/common
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/dscl
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/hdmirx
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/memout
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/mux
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/timingdetect
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/vbi
ccflags-y += -I$(HWREG_HEADER_PATH)/srccap/dv
#ccflags-y += -I$(MDRV_MTKDRV_HWREG_SRC_DIR)/include
#ccflags-y += -I$(MDRV_MTKDRV_HWREG_SRC_DIR)/drv/vbi

ccflags-y += -I$(srctree)/drivers/iommu/mtk_iommu_dtv
ccflags-y += -I$(srctree)/drivers/soc/mediatek/tv_metadata/include
ccflags-y += -I$(MDRV_MTKDRV_SCRIPTMGT_SRC_DIR)/include

ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/common
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/scriptmgt
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/xc_alg
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/xc_alg/reg
ccflags-y += -I$(MTK_SRCCAP_PATH)/utp
ccflags-y += -I$(MTK_SRCCAP_PATH)/utp/drv/hdmirx/include
ccflags-y += -I$(MTK_SRCCAP_PATH)/utp/drv/hdmirx
ccflags-y += -I$(MTK_SRCCAP_PATH)/utp/drv/avd/include
ccflags-y += -I$(MTK_SRCCAP_PATH)/utp/drv/avd
#ccflags-y += -I$(MTK_SRCCAP_PATH)/utp/hal/$(CONFIG_MSTAR_CHIP_NAME)/avd/include
#ccflags-y += -I$(MTK_SRCCAP_PATH)/utp/hal/$(CONFIG_MSTAR_CHIP_NAME)/avd

ifeq ($(PRODUCT_OUT),)
ccflags-y += -I$(MISDK_DIR)/utopia/utpa2-secure/build/bsp/TEE_HEADER
else
ccflags-y += -I$(ANDROID_TOP)/$(PRODUCT_OUT)/obj/SECURE_UTOPIA/TEE_HEADER
endif
ccflags-y += -I$(MTK_OBJ_DIR)/misdk/utopia/utpa2-secure/obj/SECURE_UTOPIA/TEE_HEADER

CONFIG_MSTAR_CHIP_NAME := $(subst ",,$(CONFIG_MSTAR_CHIP_NAME))

mtk-srccap-objs := $(SUB_FOLDER)/mtk_srccap.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-adc/mtk_srccap_adc.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-avd/mtk_srccap_avd.o \
					$(SUB_FOLDER)/mtk-srccap-avd/show_param.o \
					$(SUB_FOLDER)/mtk-srccap-avd/mtk_srccap_avd_avd.o \
					$(SUB_FOLDER)/mtk-srccap-avd/mtk_srccap_avd_mux.o \
					$(SUB_FOLDER)/mtk-srccap-avd/mtk_srccap_avd_colorsystem.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-common/mtk_srccap_common.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-common/mtk_srccap_common_irq.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-common/mtk_srccap_common_ca.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-dscl/mtk_srccap_dscl.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-hdmirx/mtk_srccap_hdmirx.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-pattern/mtk_srccap_pattern.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-memout/mtk_srccap_memout.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-memout/mtk_srccap_memout_svp.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-timingdetect/mtk_srccap_timingdetect.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-vbi/mtk_srccap_vbi.o $(SUB_FOLDER)/mtk-srccap-vbi/mtk_drv_vbi.o $(SUB_FOLDER)/mtk-srccap-vbi/show_param_vbi.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-dv/mtk_srccap_dv.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-dv/mtk_srccap_dv_common.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-dv/mtk_srccap_dv_descrb.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-dv/mtk_srccap_dv_dma.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-dv/mtk_srccap_dv_sd.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-dv/mtk_srccap_dv_meta.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-dv/mtk_srccap_dv_utility.o

#ifeq ($(CONFIG_VIDEO_MEDIATEK_SRCCAP_MERAK),y)
ifneq ($(CONFIG_VIDEO_MEDIATEK_SRCCAP_LEGACY),y)
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-hdmirx/mtk_srccap_hdmirx_phy.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-hdmirx/mtk_srccap_hdmirx_sysfs.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-hdmirx/mtk_srccap_hdmirx_packetreceiver.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-hdmirx/mtk_srccap_hdmirx_dsc.o
mtk-srccap-objs += $(SUB_FOLDER)/mtk-srccap-hdmirx/mtk_srccap_hdmirx_mux.o
endif

mtk-srccap-objs += $(SUB_FOLDER)/utp/sti_msos.o
mtk-srccap-objs += $(SUB_FOLDER)/utp/drv/avd/drvAVD.o $(SUB_FOLDER)/utp/drv/avd/AVD.o
mtk-srccap-objs += $(SUB_FOLDER)/utp/drv/hdmirx/mdrv_hdmi.o

obj-$(CONFIG_VIDEO_MEDIATEK_SOURCE_CAPTURE)+= mtk-srccap.o

