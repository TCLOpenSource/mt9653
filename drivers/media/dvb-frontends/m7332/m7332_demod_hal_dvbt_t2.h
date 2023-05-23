/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _INTERN_DVBT_T2_H_
#define _INTERN_DVBT_T2_H_

#include "m7332_demod_common.h"

enum dmd_signal_type {
	UNKNOWN_SIGNAL   = 0x00,
	T_SIGNAL         = 0x55,
	T2_SIGNAL        = 0xaa,
};

/*---------------------------------------------------------------------------*/
void m7332_dmd_hal_dvbt_t2_init_clk(struct dvb_frontend *fe);
int m7332_dmd_hal_dvbt_t2_load_code(void);
int m7332_dmd_hal_dvbt_t2_config(struct dvb_frontend *fe, u32 if_freq);
int m7332_dmd_hal_dvbt_t2_exit(void);
int m7332_dmd_hal_dvbt_t2_get_frontend(struct dvb_frontend *fe,
				struct dtv_frontend_properties *p);
int m7332_dmd_hal_dvbt_t2_read_status(struct dvb_frontend *fe,
			enum fe_status *status);

ssize_t m7332_demod_hal_dvbt_t2_get_information(struct device_driver *driver,
			char *buf);
/*---------------------------------------------------------------------------*/
#endif

