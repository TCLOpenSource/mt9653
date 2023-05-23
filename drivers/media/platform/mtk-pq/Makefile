#KBUILD_EXTRA_SYMBOLS += $(PREBUILD_SRC_DIR)/$(PRODUCT_NAME_DTB)/linux/linuxkernel/common/symver/scriptmgt/Module.symvers

export CONFIG_VIDEO_MEDIATEK_PQ=m

#ifeq ($(CONFIG_VIDEO_MEDIATEK_PQ_MERAK),y)
#SUB_FOLDER = merak
#else
#SUB_FOLDER = mt5870
#endif

ifeq ($(CONFIG_VIDEO_MEDIATEK_PQ_LEGACY),y)
SUB_FOLDER = mt5870
else
SUB_FOLDER = merak
endif

#MTK_PQ_PATH=$(src)
ccflags-y := -DMSOS_TYPE_LINUX_KERNEL
ccflags-y += -DSTI_PLATFORM_BRING_UP
ccflags-y += -I$(MTK_PQ_PATH)/$(SUB_FOLDER)/include/
ccflags-y += -I$(MTK_PQ_PATH)/$(SUB_FOLDER)/utopia/
ccflags-y += -I$(MTK_PQ_PATH)/$(SUB_FOLDER)/mtk-pq-enhance/include/
ccflags-y += -I$(srctree)/drivers/iommu/mtk_iommu_dtv
ccflags-y += -I$(srctree)/drivers/soc/mediatek/tv_metadata/include
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/pqu
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/common
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/pq/display
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/pq/common
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/hwreg/pq/dovi
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/tv_cpuif/include
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/tv_cpuif
ccflags-y += -I$(srctree)/drivers/soc/mediatek/mtk-memory
ifeq ($(PRODUCT_OUT),)
ccflags-y += -I$(MISDK_DIR)/utopia/utpa2-secure/build/bsp/TEE_HEADER
else
ccflags-y += -I$(ANDROID_TOP)/$(PRODUCT_OUT)/obj/SECURE_UTOPIA/TEE_HEADER
endif
ccflags-y += -I$(MTK_OBJ_DIR)/misdk/utopia/utpa2-secure/obj/SECURE_UTOPIA/TEE_HEADER

mtk-pq-objs := $(SUB_FOLDER)/mtk_pq.o \
               $(SUB_FOLDER)/mtk-pq-enhance/mtk_pq_enhance.o \
               $(SUB_FOLDER)/mtk-pq-enhance/hdr/mtk_pq_idk.o \
               $(SUB_FOLDER)/mtk-pq-enhance/hdr/mtk_pq_hdr.o \
               $(SUB_FOLDER)/mtk-pq-enhance/hdr/mtk_pq_cfd.o \
               $(SUB_FOLDER)/mtk-pq-enhance/hdr/mtk_pq_dv.o \
               $(SUB_FOLDER)/mtk-pq-enhance/pqd/mtk_pq_pqd.o \
               $(SUB_FOLDER)/mtk-pq-common/mtk_pq_common.o \
               $(SUB_FOLDER)/mtk-pq-common/mtk_pq_common_irq.o \
               $(SUB_FOLDER)/mtk-pq-common/mtk_pq_common_ca.o \
               $(SUB_FOLDER)/mtk-pq-xc/mtk_pq_buffer.o \
               $(SUB_FOLDER)/mtk-pq-xc/mtk_pq_svp.o \
               $(SUB_FOLDER)/mtk-pq-b2r/mtk_pq_b2r.o \
               $(SUB_FOLDER)/mtk-pq-display/mtk_pq_display.o \
               $(SUB_FOLDER)/mtk-pq-display/mtk_pq_display_b2r.o \
               $(SUB_FOLDER)/mtk-pq-display/mtk_pq_display_mdw.o \
               $(SUB_FOLDER)/mtk-pq-display/mtk_pq_display_idr.o \
               $(SUB_FOLDER)/mtk-pq-display/mtk_pq_pattern.o \
               $(SUB_FOLDER)/mtk-pq-display/mtk_pq_thermal.o \

obj-$(CONFIG_VIDEO_MEDIATEK_PQ)	+= mtk-pq.o

KBUILD_EXTRA_SYMBOLS += $(MDRV_MTKDRV_METAUTIL_SRC_DIR)/Module.symvers
