/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * MediaTek	Inc. (C) 2020. All rights	reserved.
 */
#ifndef _CFM_PRIV_H
#define _CFM_PRIV_H
#include  "cfm.h"

extern struct dvb_adapter *mdrv_get_dvb_adapter(void);

#define MAX_DEMOD_NUMBER_FOR_FRONTEND 5
#define MAX_TUNER_NUMBER_FOR_FRONTEND 5

struct cfm_info_table {
	struct dvb_frontend *fe;
	// demod
	u16 demod_unique_id[MAX_DEMOD_NUMBER_FOR_FRONTEND];
	struct cfm_demod_ops *reg_demod_ops[MAX_DEMOD_NUMBER_FOR_FRONTEND];
	u8 demod_device_number;
	u8 sel_demod_index;
	u8 act_demod_index;
	u8 del_sys_num;
	// tuner
	u16 tuner_unique_id[MAX_TUNER_NUMBER_FOR_FRONTEND];
	struct cfm_tuner_ops *reg_tuner_ops[MAX_TUNER_NUMBER_FOR_FRONTEND];
	struct cfm_tuner_pri_data tuner_pri_data[MAX_TUNER_NUMBER_FOR_FRONTEND];
	u8 tuner_device_number;
	u8 sel_tuner_index;
	u8 act_tuner_index;
	// dish
	u16 dish_unique_id;
	struct cfm_dish_ops *reg_dish_ops;
	u8 act_dish_index;
};

#endif
