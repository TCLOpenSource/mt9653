#
# Copyright (C) 2019 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

PDIR = $(MTK_PLATFORM)

#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/include
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.0/$(PDIR)

#ccflags-y += -I$(obj)/../include/


UTIL_DIR = ../util

ccflags-y += -I$(srctree)/drivers/staging/android/ion
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/iommu/
ccflags-y += -I$(srctree)/$(obj)/../include/
ccflags-y += -I$(srctree)/drivers/apusys/common/1.0/
ccflags-y += -D'DEBUG'


obj-m += edma.o

edma-objs += edma_driver.o edma_dbgfs.o edma_queue.o
edma-objs += edma_cmd_hnd.o
edma-objs += edma_ioctl.o
