# SPDX-License-Identifier: GPL-2.0

ccflags-y	+= -I$(srctree)/mm

mtk_iommu-objs := mtk_iommu_dtv.o \
		mtk_iommu_helper.o \
		mtk_iommu_tee_interface.o \
		mtk_iommu_of.o \
		mtk_iommu_ion.o \
		mtk_iommu_sysfs.o \
		mtk_iommu_test.o

mtk_iommu-objs += heaps/mtk_iommu_sys_heap.o	\
		heaps/mtk_iommu_cma_heap.o	\
		heaps/mtk_iommu_ion_heap.o	\
		heaps/mtk_iommu_carveout_heap.o

mtk_iommu-objs += mtk_iommu_seal.o

obj-$(CONFIG_MTK_IOMMU_DTV) += mtk_iommu.o
