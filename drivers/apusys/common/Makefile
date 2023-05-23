APUSYS_VERSION_POWER = 2.0
APUSYS_VERSION_MNOC = 1.0
APUSYS_VERSION_MIDWARE = 1.05
APUSYS_VERSION_REVISER = 1.0
APUSYS_VERSION_DTV = 1.0
MTK_PLATFORM := secure
srcpath2=$(srctree)/drivers/apusys/common

#ccflags-y += -I$(srctree)/drivers/staging/android/ion
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach
#ccflags-y += -I$(srctree)/drivers/iommu/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/include
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/$(MTK_PLATFORM)
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y += -I$(srcpath2)/../power/$(APUSYS_VERSION_POWER)/
ccflags-y += -I$(srcpath2)/../power/$(APUSYS_VERSION_POWER)/$(MTK_PLATFORM)/
ccflags-y += -I$(srcpath2)/../include/
ccflags-y += -I$(srcpath2)/../mdla/common
ccflags-y += -I$(srcpath2)/../midware/$(APUSYS_VERSION_MIDWARE)/
ccflags-y += -I$(srcpath2)/$(MTK_PLATFORM)/
ccflags-y += -I$(srcpath2)/$(APUSYS_VERSION_DTV)/
ccflags-y += -I$(srctree)/drivers/iommu/mtk_iommu_dtv/


ccflags-y += -I$(obj)/../$(MTK_PLATFORM)

obj-m += apu_comm.o

apu_comm-objs:=	$(APUSYS_VERSION_DTV)/comm_mem_mgt.o \
		$(APUSYS_VERSION_DTV)/comm_mem_dma.o \
		$(APUSYS_VERSION_DTV)/comm_driver.o \
		$(APUSYS_VERSION_DTV)/comm_debug.o \
		$(APUSYS_VERSION_DTV)/comm_util.o \
		$(APUSYS_VERSION_DTV)/comm_teec.o \
		$(MTK_PLATFORM)/plat_mem_wrap.o	\
		$(MTK_PLATFORM)/plat_iommu.o
