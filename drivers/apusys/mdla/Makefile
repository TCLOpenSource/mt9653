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

APUSYS_VERSION_MIDWARE = 1.0

obj-m += mdla.o

#src=$(obj)
src=$(srctree)/drivers/apusys/mdla

# apusys
ccflags-y += -I$(src)/../include/
ccflags-y += -I$(src)/../midware/1.0/
ccflags-y += -I$(src)/../util/
ccflags-y += -I$(src)/common/
ccflags-y += -I$(src)/platform/
ccflags-y += -I$(src)/utilities/
ccflags-y += -I$(src)/interface/
ccflags-y += -I$(src)/../common/1.0/
ccflags-y += -I$(obj)/../midware/$(APUSYS_VERSION_MIDWARE)/

# mdla
ccflags-y += -I$(src)

# dummp functions for simulator
#ccflags-y += -I$(src)/simulator/
ccflags-y += -I$(src)/../power/2.5/inc/

# only for vir_hashkey test
ifeq ($(ENABLE_VIRTUAL_HASHKEY),true)
ccflags-y += -DENABLE_VIRTUAL_HASHKEY
endif

mdla-objs += $(shell cd $(src) && find -name "*.c" | sed 's/\.c/\.o/g')

