# general options
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_PATH))

ccflags-y +=	-I$(MAKEFILE_DIR)/../adapter				\
			-Idrivers/media/dvb-core			\
				-Idrivers/media/platform/mtk-dvb/adapter

mtk_sc-objs += mtk_sc_common.o \
								 mtk_sc_drv.o	\
								 mtk_sc_mdrv.o	\
								 mtk_sc_core.o	\
								 mtk_sc_interrupt.o	\
								 mtk_sc_resource.o	\
								 mtk_sc_clock.o	\
								 mtk_sc_hal.o

obj-$(CONFIG_MEDIATEK_DVB_SC) += mtk_sc.o

