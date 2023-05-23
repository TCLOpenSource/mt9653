/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRV_DVBT_T2_H_
#define _DRV_DVBT_T2_H_

#include <media/dvb_frontend.h>

#ifdef __cplusplus
extern "C"
{
#endif


enum dvbt_t2_param {
	E_DMD_DVBT_N_CFG_LP_SEL   = 0x1C,

	E_DMD_T_T2_BW               = 0x40,
	E_DMD_T_T2_FC_L             = 0x41,
	E_DMD_T_T2_FC_H             = 0x42,

	E_DMD_T_T2_AGC_REF = 0x48,

	E_DMD_T_T2_TS_SERIAL     = 0x52,

	E_DMD_T_T2_IF_AGC_INV_PWM_EN = 0x59,

	E_DMD_T_T2_TOTAL_CFO_0      = 0x85,
	E_DMD_T_T2_TOTAL_CFO_1,

	E_DMD_T_T2_DVBT2_LOCK_HIS   = 0xF0,

	E_DMD_T_T2_SNR_L = 0xF3,
	E_DMD_T_T2_SNR_H = 0xF4,

	E_DMD_T_T2_PLP_ID_ARR       = 0x100,
	E_DMD_T_T2_L1_FLAG          = 0x120,
	E_DMD_T_T2_PLP_ID,
	E_DMD_T_T2_GROUP_ID,
	E_DMD_T_T2_CHANNEL_SWITCH,

	E_DMD_T_T2_TS_DATA_RATE_0       = 0x130,
	E_DMD_T_T2_TS_DATA_RATE_1       = 0x131,
	E_DMD_T_T2_TS_DATA_RATE_2       = 0x132,
	E_DMD_T_T2_TS_DATA_RATE_3       = 0x133,
	E_DMD_T_T2_TS_DATA_RATE_CHANGE_IND       = 0x134,
	E_DMD_T_T2_TS_DIV_172           = 0x135,
	E_DMD_T_T2_TS_DIV_288           = 0x136,
	E_DMD_T_T2_TS_DIV_432           = 0x137,

	E_DMD_T_T2_JOINT_DETECTION_FLAG = 0x381,
	E_DMD_T_T2_PARAM_NUM,
};

/*
 *-----------------------------------------------------------------------------
 *  Function and Variable
 *-----------------------------------------------------------------------------
 */
extern int m7332_dmd_drv_dvbt_t2_init(struct dvb_frontend *fe);

extern int m7332_dmd_drv_dvbt_t2_config(struct dvb_frontend *fe,
			u32 if_freq);

extern int m7332_dmd_drv_dvbt_t2_exit(void);

extern int m7332_dmd_drv_dvbt_t2_set_frontend(struct dvb_frontend *fe);

extern int m7332_dmd_drv_dvbt_t2_get_frontend(struct dvb_frontend *fe,
			struct dtv_frontend_properties *p);

extern int m7332_dmd_drv_dvbt_t2_read_status(struct dvb_frontend *fe,
			enum fe_status *status);

extern ssize_t dvbt_t2_get_information_show(struct device_driver *driver,
			char *buf);

#ifdef __cplusplus
}
#endif


#endif
