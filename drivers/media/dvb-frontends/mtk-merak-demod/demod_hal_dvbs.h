/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DEMOD_HAL_DVBS_H_
#define _DEMOD_HAL_DVBS_H_

#include "demod_common.h"
//--------------------------------------------------------------------

//For DVBS
#define TOP_REG_BASE                0x2000    //DMDTOP
#define _REG_DMDTOP(idx)           (TOP_REG_BASE + (idx)*2)
#define DMDTOP_ADC_IQ_SWAP         (_REG_DMDTOP(0x30)+0)
#define TOP_WR_DBG_90              (_REG_DMDTOP(0x60)+0)

#define REG_BACKEND                 0x1F00//_REG_BACKEND
#define DVBSFEC_REG_BASE            0x3F00
#define DVBT2FEC_REG_BASE           0x3300
#define DVBS2_REG_BASE              0x3A00
#define DVBS2_INNER_REG_BASE        0x3B00
#define DVBS2_INNER_EXT_REG_BASE    0x3C00
#define DVBS2_INNER_EXT2_REG_BASE    0x3D00

#define FRONTEND_REG_BASE           0x2800
#define _REG_FRONTEND(idx)           (FRONTEND_REG_BASE + (idx)*2)
#define FRONTEND_LATCH                (_REG_FRONTEND(0x02)+1)//[15]
#define FRONTEND_AGC_DEBUG_SEL         (_REG_FRONTEND(0x11)+0)//[3:0]
#define FRONTEND_AGC_DEBUG_OUT_R0      (_REG_FRONTEND(0x12)+0)
#define FRONTEND_AGC_DEBUG_OUT_R1      (_REG_FRONTEND(0x12)+1)
#define FRONTEND_AGC_DEBUG_OUT_R2      (_REG_FRONTEND(0x13)+0)
#define FRONTEND_IF_MUX                (_REG_FRONTEND(0x15)+0)//[1]
#define FRONTEND_IF_AGC_MANUAL0        (_REG_FRONTEND(0x19)+0)
#define FRONTEND_IF_AGC_MANUAL1        (_REG_FRONTEND(0x19)+1)
#define FRONTEND_MIXER_IQ_SWAP_OUT     (_REG_FRONTEND(0x2F)+0)//[1]
#define FRONTEND_INFO_01               (_REG_FRONTEND(0x7C)+0)
#define FRONTEND_INFO_02               (_REG_FRONTEND(0x7C)+1)
#define FRONTEND_INFO_07               (_REG_FRONTEND(0x7F)+0)

#define FRONTENDEXT_REG_BASE        0x2900
#define FRONTENDEXT2_REG_BASE       0x2A00
#define DMDANA_REG_BASE             0x2E00
#define DVBTM_REG_BASE              0x3400

#define DVBS2OPPRO_REG_BASE              0x3E00
#define _REG_DVBS2OPPRO(idx)             (DVBS2OPPRO_REG_BASE + (idx)*2)
#define DVBS2OPPRO_SIS_EN                (_REG_DVBS2OPPRO(0x43)+0) //[2]
#define DVBS2OPPRO_OPPRO_ISID   (_REG_DVBS2OPPRO(0x43)+1) //[15:8]
#define DVBS2OPPRO_OPPRO_ISID_SEL        (_REG_DVBS2OPPRO(0x50)+0)
#define DVBS2OPPRO_ROLLOFF_DET_DONE      (_REG_DVBS2OPPRO(0x74)+0)  //[0]
#define DVBS2OPPRO_ROLLOFF_DET_VALUE     (_REG_DVBS2OPPRO(0x74)+0)  //[6:4]
#define DVBS2OPPRO_ROLLOFF_DET_ERR       (_REG_DVBS2OPPRO(0x74)+1)  //[8]

#define _REG_DVBT2FEC(idx)              (DVBT2FEC_REG_BASE + (idx)*2)
#define DVBT2FEC_OUTER_FREEZE           (_REG_DVBT2FEC(0x02)+0)//[0]
#define DVBT2FEC_LDPC_ERROR_WINDOW0     (_REG_DVBT2FEC(0x12)+0)
#define DVBT2FEC_LDPC_ERROR_WINDOW1     (_REG_DVBT2FEC(0x12)+1)
#define DVBT2FEC_BCH_EFLAG_CNT_NUM0   (_REG_DVBT2FEC(0x25)+0)
#define DVBT2FEC_BCH_EFLAG_CNT_NUM1   (_REG_DVBT2FEC(0x25)+1)
#define DVBT2FEC_BCH_EFLAG_SUM0   (_REG_DVBT2FEC(0x26)+0)
#define DVBT2FEC_BCH_EFLAG_SUM1   (_REG_DVBT2FEC(0x26)+1)
#define DVBT2FEC_LDPC_BER_COUNT0        (_REG_DVBT2FEC(0x32)+0)
#define DVBT2FEC_LDPC_BER_COUNT1        (_REG_DVBT2FEC(0x32)+1)
#define DVBT2FEC_LDPC_BER_COUNT2        (_REG_DVBT2FEC(0x33)+0)
#define DVBT2FEC_LDPC_BER_COUNT3        (_REG_DVBT2FEC(0x33)+1)

#define _REG_DVBSFEC(idx)              (DVBSFEC_REG_BASE + (idx)*2)
#define DVBSFEC_IQ_SWAP_FLAG           (_REG_DVBSFEC(0x41)+0)

#define U8      unsigned char
#define U16     unsigned short
#define U32     unsigned long
#define BOOL    unsigned char
#define BOOLEAN  unsigned char

#define INTERN_DVBS_TS_SERIAL_INVERSION       0
#define INTERN_DVBS_TS_PARALLEL_INVERSION     1
#define INTERN_DVBS_DTV_DRIVING_LEVEL          1
#define INTERN_DVBS_WEAK_SIGNAL_PICTURE_FREEZE_ENABLE  1
#define transfer_unit 1000
#define half_transfer_unit 500
#define CNR_unit 256
#define half_CNR_unit 128
#define u16_sign_criteria 0x8000
#define BIT0 1
#define BIT1 2
#define BIT2 4
#define BIT3 8
#define BIT4 16
#define BIT5 32
#define BIT6 64
#define BIT7 128
#define u16_highbyte_to_u8 8
#define u32_lowbyte_to_u8 16
#define u32_highbyte_to_u8 24
#define blindscan_tuner_cutoff 44000
#define auto_swap_force_cfo_small  -1000
#define auto_swap_force_cfo_large  -2000
#define auto_swap_force_cfo_criterion 15000
#define Max_TP_info 2
#define Max_TP_number 500
#define progress 100
#define tuner_stable_delay 10
#define Max_u8 255
#define dvbs_err_window   192512
#define dvbs2_long_err_window  64800
#define dvbs2_short_err_window  16200
#define BER_unit     10000000
#define DVBS_AGC_STATE          1
#define DVBS_DCR_STATE     2
#define DVBS_DECIDAGC_STATE     3
#define DVBS_ACIDAGC_STATE     4
#define DVBS_TR_STATE              5
#define DVBS_CCILMS_STATE       6
#define DVBS2_FS_STATE            7
#define DVBS_CMA_STATE          8
#define DVBS_PR_STATE             9
#define DVBS2_PR_STATE           10
#define DVBS_EQ_STATE            11
#define DVBS2_EQ_STATE          12
#define DVBS_OUTER_STATE    13
#define DVBS2_OUTER_STATE  14
#define DVBS_IDLE_STATE         15
#define DVBS2_IDLE_STATE       16
#define DVBS_BLIND_SCAN      17

//--------------------------------------------------------------------
int intern_dvbs_exit(struct dvb_frontend *fe);
int intern_dvbs_load_code(void);
void intern_dvbs_init_clk(struct dvb_frontend *fe);
int intern_dvbs_config(struct dvb_frontend *fe);
int intern_dvbs_get_ts_rate(u64 *clock_rate);
int intern_dvbs_read_status(struct dvb_frontend *fe, enum fe_status *status);
int intern_dvbs_get_frontend(struct dvb_frontend *fe, struct dtv_frontend_properties *p);
int intern_dvbs_get_ber_window(u32 *err_period, u32 *err);
int intern_dvbs_BlindScan_Start(struct DMD_DVBS_BlindScan_Start_param *Start_param);
int intern_dvbs_BlindScan_NextFreq(struct DMD_DVBS_BlindScan_NextFreq_param *NextFreq_param);
int intern_dvbs_BlindScan_GetTunerFreq(
	struct DMD_DVBS_BlindScan_GetTunerFreq_param *GetTunerFreq_param);
int intern_dvbs_BlindScan_WaitCurFreqFinished(
	struct DMD_DVBS_BlindScan_WaitCurFreqFinished_param *WaitCurFreqFinished_param);
int intern_dvbs_BlindScan_Cancel(void);
int intern_dvbs_BlindScan_End(void);
int intern_dvbs_BlindScan_GetChannel(struct DMD_DVBS_BlindScan_GetChannel_param *GetChannel_param);
int intern_dvbs_blindscan_config(struct dvb_frontend *fe);
int intern_dvbs_diseqc_set_22k(enum fe_sec_tone_mode tone);
int intern_dvbs_diseqc_send_cmd(struct dvb_diseqc_master_cmd *cmd);
int intern_dvbs_diseqc_set_tone(enum fe_sec_mini_cmd minicmd);
ssize_t demod_hal_dvbs_get_information(struct device_driver *driver, char *buf);
int intern_dvbs_BlindScan_iq_swap_check(
	struct DMD_DVBS_BlindScan_WaitCurFreqFinished_param *WaitCurFreqFinished_param);
int intern_dvbs_BlindScan_TP_lock(void);
int intern_dvbs_BlindScan_TP_unlock(void);

#endif

