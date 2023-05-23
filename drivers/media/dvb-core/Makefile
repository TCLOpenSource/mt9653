# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel DVB device drivers.
#

dvb-net-$(CONFIG_DVB_NET) := dvb_net.o
dvb-vb2-$(CONFIG_DVB_MMAP) := dvb_vb2.o
ifneq ($(CONFIG_MEDIATEK_DVB_DEMUX2),)
dvb-vb2-v2-$(CONFIG_DVB_MMAP) := dvb_vb2_v2.o
endif

dvb-core-objs := dvbdev.o dmxdev.o dvb_demux.o			\
		 dvb_ca_en50221.o dvb_frontend.o		\
		 $(dvb-net-y) dvb_ringbuffer.o $(dvb-vb2-y) dvb_math.o

ifneq ($(CONFIG_MEDIATEK_DVB_DEMUX2),)
dvb-core-objs += dmx2dev.o dvb_demux2.o dvb_dvr2.o		\
				 dvb_fh.o dvb_event.o dvb_ctrls.o		\
				 $(dvb-vb2-v2-y)
endif

obj-$(CONFIG_DVB_CORE) += dvb-core.o
