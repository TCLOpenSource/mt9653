/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRV_DVBC_H_
#define _DRV_DVBC_H_

#include <media/dvb_frontend.h>

#ifdef __cplusplus
extern "C"
{
#endif


enum dvbc_param {
	E_DMD_DVBC_OP_AUTO_SCAN_SYM_RATE = 0x09,
	E_DMD_DVBC_OP_AUTO_SCAN_QAM = 0x0A,

	E_DMD_DVBC_IF_INV_PWM_OUT_EN = 0x10,

	E_DMD_DVBC_CFG_FIF_L = 0x15,
	E_DMD_DVBC_CFG_FIF_H = 0x16,
	E_DMD_DVBC_CFG_FC_L = 0x17,
	E_DMD_DVBC_CFG_FC_H = 0x18,
	E_DMD_DVBC_CFG_BW0_L = 0x19,
	E_DMD_DVBC_CFG_BW0_H = 0x1A,

	E_DMD_DVBC_CFG_QAM = 0x32,

	E_DMD_DVBC_CFG_TS_SERIAL = 0x35,

	E_DMD_DVBC_AGC_REF_L = 0x3A,
	E_DMD_DVBC_AGC_REF_H = 0x3B,

	E_DMD_DVBC_SNR100_L = 0x62,
	E_DMD_DVBC_SNR100_H = 0x63,

	E_DMD_DVBC_TS_DATA_RATE_0  = 0x8d,
	E_DMD_DVBC_TS_DATA_RATE_1  = 0x8e,
	E_DMD_DVBC_TS_DATA_RATE_2  = 0x8f,
	E_DMD_DVBC_TS_DATA_RATE_3  = 0x90,
	E_DMD_DVBC_PARAM_NUM,
};

/*
 *-----------------------------------------------------------------------------
 *  Function and Variable
 *-----------------------------------------------------------------------------
 */
extern int m7332_dmd_drv_dvbc_init(struct dvb_frontend *fe);

extern int m7332_dmd_drv_dvbc_config(struct dvb_frontend *fe,
			u32 if_freq);

extern int m7332_dmd_drv_dvbc_exit(void);

extern int m7332_dmd_drv_dvbc_set_frontend(struct dvb_frontend *fe);

extern int m7332_dmd_drv_dvbc_get_frontend(struct dvb_frontend *fe,
			struct dtv_frontend_properties *p);

extern int m7332_dmd_drv_dvbc_read_status(struct dvb_frontend *fe,
			enum fe_status *status);

extern ssize_t dvbc_get_information_show(struct device_driver *driver,
			char *buf);

#ifdef __cplusplus
}
#endif


#endif
