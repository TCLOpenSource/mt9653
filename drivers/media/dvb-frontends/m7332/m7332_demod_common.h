/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _M7332_DEMOD_COMMON_H_
#define _M7332_DEMOD_COMMON_H_

#include <linux/dvb/frontend.h>
#include <media/dvb_frontend.h>

#if !defined(TRUE) && !defined(FALSE)
/* definition for TRUE */
#define TRUE                        true
/* definition for FALSE */
#define FALSE                       false
#endif

/* Demod MailBox */
#define MB_REG_BASE   0x112600UL
/* Dmd51 */
#define DMD51_REG_BASE  0x103480UL

#define	ISDBT_TDP_REG_BASE  0x1400
#define	ISDBT_FDP_REG_BASE  0x1500
#define	ISDBT_FDPE_REG_BASE 0x1600
#define BACKEND_REG_BASE  0x1f00
#define TOP_REG_BASE  0x2000
#define TDP_REG_BASE  0x2100
#define FDP_REG_BASE  0x2200
#define FEC_REG_BASE  0x2300
#define TDF_REG_BASE  0x2800
#define T2L1_REG_BASE  0x2b00
#define T2TDP_REG_BASE  0x3000
#define T2FEC_REG_BASE  0x3300
#define DVBTM_REG_BASE  0x3400


struct mtk_demod_dev {
	struct platform_device *pdev;
	struct dvb_adapter *adapter;
	struct dvb_frontend fe;
	u8     *virt_riu_base_addr;
	u32     dram_base_addr;
	int previous_system;
	bool fw_downloaded;
	unsigned int start_time;
};

/*
 *  E_POWER_SUSPEND:    State is backup in DRAM, components power off.
 *  E_POWER_RESUME:     Resume from Suspend or Hibernate mode
 *  E_POWER_MECHANICAL: Full power off mode. System return to working state
 *      only after full reboot
 *  E_POWER_SOFT_OFF:   The system appears to be off, but some components
 *      remain powered for trigging boot-up.
 */
enum EN_POWER_MODE {
	E_POWER_SUSPEND    = 1,
	E_POWER_RESUME     = 2,
	E_POWER_MECHANICAL = 3,
	E_POWER_SOFT_OFF   = 4,
};

enum op_type {
	ADD = 0,
	MINUS,
	MULTIPLY,
	DIVIDE
};

struct ms_float_st {
	s32 data;
	s8 exp;
};

/*---------------------------------------------------------------------------*/
void mtk_demod_init_riuaddr(struct dvb_frontend *fe);

void mtk_demod_delay_ms(u32 time_ms);
void mtk_demod_delay_us(u32 time_us);
unsigned int mtk_demod_get_time(void);
int mtk_demod_dvb_time_diff(u32 time);

int mtk_demod_mbx_dvb_wait_handshake(void);
int mtk_demod_mbx_dvb_write(u16 addr, u8 value);
int mtk_demod_mbx_dvb_read(u16 addr, u8 *value);
int mtk_demod_mbx_dvb_write_dsp(u16 addr, u8 value);
int mtk_demod_mbx_dvb_read_dsp(u16 addr, u8 *value);

unsigned int mtk_demod_read_byte(u32 addr);
void mtk_demod_write_byte(u32 addr, u8 value);
void mtk_demod_write_byte_mask(u32 reg, u8 val, u8 msk);
void mtk_demod_write_bit(u32 reg, bool enable_b, u8 mask);

struct ms_float_st mtk_demod_float_op(struct ms_float_st st_rn,
			struct ms_float_st st_rd, enum op_type op_code);
s64 mtk_demod_float_to_s64(struct ms_float_st msf_input);
/*---------------------------------------------------------------------------*/
#endif

