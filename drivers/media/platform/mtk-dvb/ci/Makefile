# general options
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_PATH))

ccflags-y +=	-I$(MAKEFILE_DIR)/../adapter				\
			-Idrivers/media/dvb-core			\
				-Idrivers/media/platform/mtk-dvb/adapter

mtk_pcmcia-objs += mtk_pcmcia_common.o\
								 mtk_pcmcia_drv.o		\
								 mtk_pcmcia_core.o		\
								 mtk_pcmcia_interrupt.o	\
								 mtk_pcmcia_resource.o	\
								 mtk_pcmcia_clock.o	\
								 mtk_pcmcia_hal.o	\
								 mtk_pcmcia_stub.o

obj-$(CONFIG_MEDIATEK_DVB_CI) = mtk_pcmcia.o