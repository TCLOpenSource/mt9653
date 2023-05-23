# SPDX-License-Identifier: GPL-2.0

mtk-vcu-y += mtk_vcu.o mtk_vcu_tee_interface.o

obj-$(CONFIG_VIDEO_MEDIATEK_VCU) += mtk-vcu.o

subdir-ccflags-y += -Werror -Wall
ccflags-y += -I$(srctree)/drivers/media/platform/mtk-vcu/
ccflags-y += -I$(REPO_ROOT_PATH)/mtktv_drivers/ree/include/vcu
ccflags-y += -I$(srctree)/include/linux
