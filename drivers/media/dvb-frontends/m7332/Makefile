# SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)

m7332-objs := m7332_demod_drv_dvbt_t2.o \
		m7332_demod_drv_dvbc.o \
		drvDMD_DVBS.o \
		drvDMD_ATSC.o \
		drvDMD_ISDBT.o \
		drvDMD_VIF.o \
		drvDMD_DTMB.o \
		m7332_demod_hal_dvbt_t2.o \
		m7332_demod_hal_dvbc.o \
		halDMD_INTERN_DVBS.o \
		halDMD_INTERN_ATSC.o \
		halDMD_INTERN_ISDBT.o \
		halDMD_INTERN_VIF.o \
		halDMD_INTERN_DTMB.o \
		m7332_demod_common.o \
		m7332_demod_core.o

obj-m += m7332.o

ccflags-y += -I$(srctree)/drivers/media/dvb-frontends
ccflags-y += -I$(srctree)/drivers/media/dvb-frontends/mtk_tuner
