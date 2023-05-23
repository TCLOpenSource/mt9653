/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */


#ifndef _APUSYS_POWER_H_
#define _APUSYS_POWER_H_

#include "apusys_power_user.h"


/******************************************************
 * for apusys power platform device API
 ******************************************************/
extern int apu_power_device_register(enum DVFS_USER, struct platform_device*);
extern void apu_power_device_unregister(enum DVFS_USER);
extern int apu_device_power_on(enum DVFS_USER);
extern int apu_device_power_off(enum DVFS_USER);
extern int apu_device_power_suspend(enum DVFS_USER user, int suspend);
extern void apu_device_set_opp(enum DVFS_USER user, uint8_t opp);
extern uint64_t apu_get_power_info(uint8_t force);
extern bool apu_get_power_on_status(enum DVFS_USER user);
extern void apu_power_on_callback(void);
extern int apu_power_callback_device_register(enum POWER_CALLBACK_USER user,
					void (*power_on_callback)(void *para),
					void (*power_off_callback)(void *para));
extern void apu_power_callback_device_unregister(enum POWER_CALLBACK_USER user);
extern uint8_t apusys_boost_value_to_opp
				(enum DVFS_USER user, uint8_t boost_value);
extern enum DVFS_FREQ apusys_opp_to_freq(enum DVFS_USER user, uint8_t opp);
extern uint8_t apusys_freq_to_opp(enum DVFS_VOLTAGE_DOMAIN buck_domain,
							uint32_t freq);
extern int8_t apusys_get_ceiling_opp(enum DVFS_USER user);
extern int8_t apusys_get_opp(enum DVFS_USER user);
extern void apu_power_reg_dump(void);
extern int apu_power_power_stress(int type, int device, int opp);
extern bool apusys_power_check(void);
extern void apu_set_vcore_boost(bool enable);

#ifdef EDMA_DUMMY_POWER

#define apu_power_device_register(user, pdev) 0
#define apu_power_device_unregister(user)
#define apu_device_power_on(user) 0
#define apu_device_power_off(user) 0
#define apu_device_power_suspend(user, suspend) 0
#define apu_device_set_opp(user, opp)
#define apusys_boost_value_to_opp(user, val) 0
#define apusys_power_check(user) true

#ifndef APUSYS_MAX_NUM_OPPS
#define APUSYS_MAX_NUM_OPPS 1
#endif

#define trace_tag_customer(...)
#define trace_async_tag(...)
#define apusys_mem_invalidate(...)
#define apusys_mem_flush(...)

#define apusys_unregister_device(...)	0
#define apusys_register_device(...)	0
#define apusys_mem_query_kva(addr)	addr
#endif
#endif
