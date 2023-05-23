/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _INTERN_DVBC_H_
#define _INTERN_DVBC_H_

#include "m7332_demod_common.h"

/*---------------------------------------------------------------------------*/
void m7332_dmd_hal_dvbc_init_clk(struct dvb_frontend *fe);
int m7332_dmd_hal_dvbc_load_code(void);
int m7332_dmd_hal_dvbc_config(struct dvb_frontend *fe, u32 if_freq);
int m7332_dmd_hal_dvbc_exit(void);
int m7332_dmd_hal_dvbc_get_frontend(struct dvb_frontend *fe,
				struct dtv_frontend_properties *p);
int m7332_dmd_hal_dvbc_read_status(struct dvb_frontend *fe,
			enum fe_status *status);

ssize_t m7332_demod_hal_dvbc_get_information(struct device_driver *driver,
			char *buf);
/*---------------------------------------------------------------------------*/
#endif

