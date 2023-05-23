/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * MediaTek	Inc. (C) 2020. All rights	reserved.
 */

#ifndef _DEMOD_HAL_DVBC_H_
#define _DEMOD_HAL_DVBC_H_

#include "demod_common.h"

/*---------------------------------------------------------------------------*/
void dmd_hal_dvbc_init_clk(struct dvb_frontend *fe);
int dmd_hal_dvbc_load_code(void);
int dmd_hal_dvbc_config(struct dvb_frontend *fe, u32 if_freq);
int dmd_hal_dvbc_exit(struct dvb_frontend *fe);
int dmd_hal_dvbc_get_frontend(struct dvb_frontend *fe, struct dtv_frontend_properties *p);
int dmd_hal_dvbc_read_status(struct dvb_frontend *fe, enum fe_status *status);

ssize_t demod_hal_dvbc_get_information(struct device_driver *driver, char *buf);
/*---------------------------------------------------------------------------*/
#endif

