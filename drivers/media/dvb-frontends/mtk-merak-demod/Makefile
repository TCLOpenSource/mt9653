# SPDX-License-Identifier: GPL-2.0

mtk-merak-demod-objs := demod_drv_dvbt_t2.o \
		demod_drv_dvbc.o \
		demod_drv_dvbs.o \
		demod_drv_atsc.o \
		demod_drv_isdbt.o \
		demod_drv_vif.o \
		demod_drv_dtmb.o \
		demod_hal_dvbt_t2.o \
		demod_hal_dvbc.o \
		demod_hal_dvbs.o \
		demod_hal_atsc.o \
		demod_hal_isdbt.o \
		demod_hal_vif.o \
		demod_hal_dtmb.o \
		demod_common.o \
		demod_core.o

obj-m += mtk-merak-demod.o

ccflags-y += -Idrivers/media/dvb-frontends
ccflags-y += -Idrivers/media/dvb-frontends/cfm