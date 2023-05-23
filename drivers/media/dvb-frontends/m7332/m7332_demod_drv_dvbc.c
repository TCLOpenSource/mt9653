// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <media/dvb_frontend.h>
#include "m7332_demod_drv_dvbc.h"
#include "m7332_demod_hal_dvbc.h"
#include "m7332_demod_common.h"

#define	DVBC_TOTAL_TIME_OUT	7000


int m7332_dmd_drv_dvbc_init(struct dvb_frontend *fe)
{
	u32	ret	= 0;

	m7332_dmd_hal_dvbc_init_clk(fe);
	ret = m7332_dmd_hal_dvbc_load_code();


	return ret;
}

int m7332_dmd_drv_dvbc_config(struct dvb_frontend *fe, u32 if_freq)
{
	u32	ret	= 0;

	ret = m7332_dmd_hal_dvbc_config(fe, if_freq);

	return ret;
}

int m7332_dmd_drv_dvbc_exit(void)
{
	u32 ret = 0;

	ret = m7332_dmd_hal_dvbc_exit();

	return ret;
}

int m7332_dmd_drv_dvbc_set_frontend(struct dvb_frontend *fe)
{
	return 0;
}

int m7332_dmd_drv_dvbc_get_frontend(struct dvb_frontend *fe,
				 struct	dtv_frontend_properties *p)
{
	return m7332_dmd_hal_dvbc_get_frontend(fe, p);
}

int m7332_dmd_drv_dvbc_read_status(struct dvb_frontend *fe,
			enum fe_status *status)
{
	u32	ret	= 0;

	ret = m7332_dmd_hal_dvbc_read_status(fe, status);

	return ret;
}

ssize_t dvbc_get_information_show(struct device_driver *driver, char *buf)
{
	return m7332_demod_hal_dvbc_get_information(driver, buf);
}
