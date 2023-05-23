/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _INTERN_DVBS_H_
#define _INTERN_DVBS_H_


#define NEW_TR_MODULE
//--------------------------------------------------------------------

#define     DVBS_LOAD_FW_TO_DRAM
#define     DEMOD_ADDR_H            0x00
#define     DEMOD_ADDR_L            0x01
#define     DEMOD_WRITE_REG         0x02
#define     DEMOD_WRITE_REG_EX      0x03
#define     DEMOD_READ_REG          0x04
#define     DEMOD_RAM_CONTROL       0x05
#define MBRegBase   0x112600UL //Demod MailBox
#define DMD_CLK_GEN 0x103300UL
#define VDMcuBase   0x103400UL //DmdMCU51 (40-4F)
#define DMDMcuBase  0x103480UL

//#define MDrv_ReadByte(x)  HAL_DMD_RIU_ReadByte(x)
//#define MDrv_WriteByte(x,y)  HAL_DMD_RIU_WriteByte(x,y)
#define DMD_CLKGEN1_REG_BASE              0x103300
#define _REG_DMD_CLKGEN1(idx)             (DMD_CLKGEN1_REG_BASE + (idx)*2)
#define CLKGEN1_DVBTM_TS_DIVNUM         (_REG_DMD_CLKGEN1(0x00)+0)//[7:0]
#define CLKGEN1_ATSC_DVB_DIV_SEL         (_REG_DMD_CLKGEN1(0x00)+1)//[10:8]
#define CLKGEN1_ATSC_DVBTC_TS_INV        (_REG_DMD_CLKGEN1(0x00)+1)//[11]
#define CLKGEN1_DVBTM_TS_OUT_MODE        (_REG_DMD_CLKGEN1(0x00)+1)//[12]
#define CLKGEN1_DEMOD_TEST_IN_EN        (_REG_DMD_CLKGEN1(0x00)+1)//[13]
#define CLKGEN1_TSOUT_PH_TUN_EN        (_REG_DMD_CLKGEN1(0x00)+1)//[14]
#define CLKGEN1_ATSC_DVB_DIV_RST        (_REG_DMD_CLKGEN1(0x01)+0)//[0]
#define CLKGEN1_DVBTC_TS        (_REG_DMD_CLKGEN1(0x04)+1)//[11:8]
#define CLKGEN1_TSOUT_PH_TUN_NUM        (_REG_DMD_CLKGEN1(0x05)+1)//[12:8]

#define DMD_CHIPTOP_REG_BASE              0x101E00
#define _REG_DMD_CHIPTOP(idx)             (DMD_CHIPTOP_REG_BASE + (idx)*2)
#define CHIPTOP_DMDTOP_DMD_SEL         (_REG_DMD_CHIPTOP(0x1C)+1)//[8]
#define CHIPTOP_DMDANA_DMD_SEL         (_REG_DMD_CHIPTOP(0x1C)+1)//[9]
#define CHIPTOP_DEMOD0_GROUP_RSTZ         (_REG_DMD_CHIPTOP(0x41)+0)//[0]
#define CHIPTOP_DEMOD1_GROUP_RSTZ         (_REG_DMD_CHIPTOP(0x41)+0)//[4]
#define CHIPTOP_DISEQC_OUT_CONFIG        (_REG_DMD_CLKGEN1(0x68)+0)//[5:4]

//For DVBS
//#define DVBT2FEC_REG_BASE           0x3300
#define DVBS2OPPRO_REG_BASE         0x3E00
#define TOP_REG_BASE                0x2000    //DMDTOP
#define REG_BACKEND                 0x1F00//_REG_BACKEND
#define DVBSFEC_REG_BASE            0x3F00
#define DVBS2FEC_REG_BASE           0x3300
#define DVBS2_REG_BASE              0x3A00
#define DVBS2_INNER_REG_BASE        0x3B00
#define DVBS2_INNER_EXT_REG_BASE    0x3C00
#define DVBS2_INNER_EXT2_REG_BASE    0x3D00
//#define DVBSTFEC_REG_BASE           0x2300    //DVBTFEC
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

#define _REG_DVBS2FEC(idx)              (DVBS2FEC_REG_BASE + (idx)*2)
#define DVBS2FEC_OUTER_FREEZE           (_REG_DVBS2FEC(0x02)+0)//[0]
#define DVBS2FEC_LDPC_ERROR_WINDOW0     (_REG_DVBS2FEC(0x12)+0)
#define DVBS2FEC_LDPC_ERROR_WINDOW1     (_REG_DVBS2FEC(0x12)+1)
#define DVBS2FEC_LDPC_BER_COUNT0        (_REG_DVBS2FEC(0x32)+0)
#define DVBS2FEC_LDPC_BER_COUNT1        (_REG_DVBS2FEC(0x32)+1)
#define DVBS2FEC_LDPC_BER_COUNT2        (_REG_DVBS2FEC(0x33)+0)
#define DVBS2FEC_LDPC_BER_COUNT3        (_REG_DVBS2FEC(0x33)+1)

#define U8      unsigned char
#define U16     unsigned short
#define U32     unsigned long
#define BOOL    unsigned char
#define BOOLEAN  unsigned char

#define INTERN_DVBS_TS_SERIAL_INVERSION       0
#define INTERN_DVBS_TS_PARALLEL_INVERSION     1
#define INTERN_DVBS_DTV_DRIVING_LEVEL          1
#define INTERN_DVBS_WEAK_SIGNAL_PICTURE_FREEZE_ENABLE  1

//--------------------------------------------------------------------
int intern_dvbs_exit(void);
int intern_dvbs_load_code(void);
void intern_dvbs_init_clk(struct dvb_frontend *fe);
int intern_dvbs_config(struct dvb_frontend *fe);
int intern_dvbs_read_status(struct dvb_frontend *fe, enum fe_status *status);
int intern_dvbs_ts_enable(int bEnable);
int intern_dvbs_get_ts_divnum(u16 *fTSDivNum);
int intern_dvbs_get_symbol_rate(u32 *u32SymbolRate);
int intern_dvbs_check_tr_ever_lock(void);
int intern_dvbs_get_frontend(struct dvb_frontend *fe,
		struct dtv_frontend_properties *p);
int intern_dvbs_version(u16 *ver);
int intern_dvbs_get_ifagc(u16 *ifagc);
int intern_dvbs_get_rf_level(u16 ifagc, u16 *rf_level);
int intern_dvbs_get_snr(s16 *snr);
int intern_dvbs2_set_isid(u8 isid);
void intern_dvbs2_get_isid_table(u8 *isid_table);
int intern_dvbs2_vcm_check(void);
int intern_dvbs2_vcm_mode(enum DMD_DVBS_VCM_OPT u8VCM_OPT);
int intern_dvbs_get_signal_strength(s32 rssi, u16 *strength);
int intern_dvbs_get_signal_quality(u16 *quality);
int intern_dvbs_get_ts_rate(u64 *clock_rate);
int intern_dvbs_get_ber(u32 *ber);
int intern_dvbs_get_ber_window(u32 *err_period, u32 *err);
int intern_dvbs_get_pkterr(u32 *pkterr);
int intern_dvbs_get_freqoffset(s32 *cfo);
int intern_dvbs_get_fec_type(u8 *fec_type);
int intern_dvbs_diseqc_set_22k(enum fe_sec_tone_mode tone);
int intern_dvbs_diseqc_send_cmd(struct dvb_diseqc_master_cmd *cmd);
int intern_dvbs_diseqc_set_tone(enum fe_sec_mini_cmd minicmd);
int intern_dvbs_power_saving(void);


#define INTERN_DVBS_LOAD_FW_FROM_CODE_MEMORY

#undef EXTSEL
#endif

