/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DEMOD_HAL_VIF_H_
#define _DEMOD_HAL_VIF_H_


#include "demod_common.h"
#include "demod_drv_vif.h"


#ifndef _BIT0
#define _BIT0  0x0001
#endif
#ifndef _BIT1
#define _BIT1  0x0002
#endif
#ifndef _BIT2
#define _BIT2  0x0004
#endif
#ifndef _BIT3
#define _BIT3  0x0008
#endif
#ifndef _BIT4
#define _BIT4  0x0010
#endif
#ifndef _BIT5
#define _BIT5  0x0020
#endif
#ifndef _BIT6
#define _BIT6  0x0040
#endif
#ifndef _BIT7
#define _BIT7  0x0080
#endif
#ifndef _BIT8
#define _BIT8  0x0100
#endif
#ifndef _BIT9
#define _BIT9  0x0200
#endif
#ifndef _BIT10
#define _BIT10 0x0400
#endif
#ifndef _BIT11
#define _BIT11 0x0800
#endif
#ifndef _BIT12
#define _BIT12 0x1000
#endif
#ifndef _BIT13
#define _BIT13 0x2000
#endif
#ifndef _BIT14
#define _BIT14 0x4000
#endif
#ifndef _BIT15
#define _BIT15 0x8000
#endif


//#define AGC_MEAN16_UPBOUND              0x8C     // 1.256*0xE0/2
//#define AGC_MEAN16_LOWBOUND             0x13     // (1/4)*0x9A/2
//#define AGC_MEAN16_UPBOUND_SECAM        0x28     // 1.256*0x40/2
//#define AGC_MEAN16_LOWBOUND_SECAM       0x08     // (1/4)*0x40/2

//#define FREQ_STEP_31_25KHz            0x00
//#define FREQ_STEP_50KHz               0x01
//#define FREQ_STEP_62_5KHz             0x02

#define IF_FREQ_45_75                   0x01//IF_FREQ_MN
#define IF_FREQ_38_90                   0x02//IF_FREQ_B
#define IF_FREQ_38_00                   0x03//IF_FREQ_PAL_38

// register
struct MS_VIF_REG_TYPE {
	u32 address;   // register index
	u8  value;     // register value
};


enum vif_saw_arch {
	EXTERNAL_DUAL_SAW,                 // 0
	EXTERNAL_SINGLE_SAW,               // 1
	SILICON_TUNER,                     // 2
	NO_SAW,                            // 3
	INTERNAL_SINGLE_SAW,               // 4
	NO_SAW_DIF,                        // 5
	SAVE_PIN_VIF,                      // 6
	SAW_ARCH_NUMS
};


extern void hal_vif_update_fw(void);

extern void hal_vif_reg_init(struct vif_initial_in *pvif_in,
	struct vif_notch_a1a2 *pn_a1a2, struct vif_sos_1112 *psos_1112,
	u16 pvif_filters[]);

extern void hal_vif_set_clock(bool enable_b);
extern void hal_vif_set_if_freq(enum if_freq_type ucIfFreq);
extern void hal_vif_load(void);
//extern void msVifInitial(void);
extern u8 hal_vif_exit(void);
extern void hal_vif_set_scan(u8 is_scanning);
extern void hal_vif_set_sound_sys(enum vif_sound_sys sound_sys, u32 if_freq);
extern void hal_vif_adc_init(void);
extern void hal_vif_top_adjust(void);

extern u8 hal_vif_read_foe(void);
extern u8 hal_vif_read_lock_status(void);

extern void hal_vif_writebyte(u32 u32Reg, u8 u8Val);
extern u8 hal_vif_readbyte(u32 u32Reg);

extern bool hal_vif_download(void);
extern void hal_vif_update_fw_backend(void);
extern bool hal_vif_filter_coef_download(void);


#endif
