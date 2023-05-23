// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * MediaTek	Inc. (C) 2020. All rights	reserved.
 */

#include <media/dvb_frontend.h>
#include "demod_drv_dvbt_t2.h"
#include "demod_hal_dvbt_t2.h"
#include "demod_common.h"

#define DVBT_T2_TOTAL_TIME_OUT  7000


int dmd_drv_dvbt_t2_init(struct dvb_frontend *fe)
{
	dmd_hal_dvbt_t2_init_clk(fe);
	return dmd_hal_dvbt_t2_load_code();
}

int dmd_drv_dvbt_t2_config(struct dvb_frontend *fe, u32 if_freq)
{
	return dmd_hal_dvbt_t2_config(fe, if_freq);
}

int dmd_drv_dvbt_t2_exit(struct dvb_frontend *fe)
{
	return dmd_hal_dvbt_t2_exit(fe);
}

int dmd_drv_dvbt_t2_set_frontend(struct dvb_frontend *fe)
{
	return 0;
}

int dmd_drv_dvbt_t2_get_frontend(struct dvb_frontend *fe, struct dtv_frontend_properties *p)
{
	return dmd_hal_dvbt_t2_get_frontend(fe, p);
}

int dmd_drv_dvbt_t2_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	return dmd_hal_dvbt_t2_read_status(fe, status);
}

ssize_t dvbt_t2_get_information_show(struct device_driver *driver, char *buf)
{
	return demod_hal_dvbt_t2_get_information(driver, buf);
}
ssize_t dvbt_t2_get_information(char *buf)
{
	struct device_driver driver;

	return demod_hal_dvbt_t2_get_information(&driver, buf);
}
