/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * MediaTek	Inc. (C) 2020. All rights	reserved.
 */

#ifndef _DEMOD_HAL_DVBT_T2_H_
#define _DEMOD_HAL_DVBT_T2_H_

#include "demod_common.h"

enum dmd_signal_type {
	UNKNOWN_SIGNAL   = 0x00,
	T_SIGNAL         = 0x55,
	T2_SIGNAL        = 0xaa,
};

/*---------------------------------------------------------------------------*/
void dmd_hal_dvbt_t2_init_clk(struct dvb_frontend *fe);
int dmd_hal_dvbt_t2_load_code(void);
int dmd_hal_dvbt_t2_config(struct dvb_frontend *fe, u32 if_freq);
int dmd_hal_dvbt_t2_exit(struct dvb_frontend *fe);
int dmd_hal_dvbt_t2_get_frontend(struct dvb_frontend *fe, struct dtv_frontend_properties *p);
int dmd_hal_dvbt_t2_read_status(struct dvb_frontend *fe, enum fe_status *status);

ssize_t demod_hal_dvbt_t2_get_information(struct device_driver *driver, char *buf);
/*---------------------------------------------------------------------------*/
#endif

