/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_DVBFRONTEND_H_
#define _MTK_DVBFRONTEND_H_

#include <linux/types.h>

#include <linux/dvb/frontend.h>


/**
 * enum fe_sec_voltage - DC Voltage used to feed the LNBf
 *
 * @SEC_VOLTAGE_13:	Output 13V to the LNBf
 * @SEC_VOLTAGE_18:	Output 18V to the LNBf
 * @SEC_VOLTAGE_OFF:	Don't feed the LNBf with a DC voltage
 */
enum mtk_fe_sec_voltage {
	//SEC_VOLTAGE_13,
	//SEC_VOLTAGE_18,
	//SEC_VOLTAGE_OFF

	SEC_VOLTAGE_14 = SEC_VOLTAGE_OFF+1,
	SEC_VOLTAGE_19,
	SEC_VOLTAGE_15,
	SEC_VOLTAGE_NONE
};


/**
 * enum fe_code_rate - Type of Forward Error Correction (FEC)
 *
 *
 * @FEC_NONE: No Forward Error Correction Code
 * @FEC_1_2:  Forward Error Correction Code 1/2
 * @FEC_2_3:  Forward Error Correction Code 2/3
 * @FEC_3_4:  Forward Error Correction Code 3/4
 * @FEC_4_5:  Forward Error Correction Code 4/5
 * @FEC_5_6:  Forward Error Correction Code 5/6
 * @FEC_6_7:  Forward Error Correction Code 6/7
 * @FEC_7_8:  Forward Error Correction Code 7/8
 * @FEC_8_9:  Forward Error Correction Code 8/9
 * @FEC_AUTO: Autodetect Error Correction Code
 * @FEC_3_5:  Forward Error Correction Code 3/5
 * @FEC_9_10: Forward Error Correction Code 9/10
 * @FEC_2_5:  Forward Error Correction Code 2/5
 *
 * Please note that not all FEC types are supported by a given standard.
 */
enum mtk_fe_code_rate {
/*
 *	FEC_NONE = 0,
 *	FEC_1_2,
 *	FEC_2_3,
 *	FEC_3_4,
 *	FEC_4_5,
 *	FEC_5_6,
 *	FEC_6_7,
 *	FEC_7_8,
 *	FEC_8_9,
 *	FEC_AUTO,
 *	FEC_3_5,
 *	FEC_9_10,
 *	FEC_2_5,
 */

	FEC_1_4 = FEC_2_5 + 1,
	FEC_1_3,
	FEC_2_9,
	FEC_13_45,
	FEC_9_20,
	FEC_90_180,
	FEC_96_180,
	FEC_11_20,
	FEC_100_180,
	FEC_104_180,
	FEC_26_45_L,
	FEC_18_30,
	FEC_28_45,
	FEC_23_36,
	FEC_116_180,
	FEC_20_30,
	FEC_124_180,
	FEC_25_36,
	FEC_128_180,
	FEC_13_18,
	FEC_132_180,
	FEC_22_30,
	FEC_135_180,
	FEC_140_180,
	FEC_7_9,
	FEC_154_180,
	FEC_11_45,
	FEC_4_15,
	FEC_14_45,
	FEC_7_15,
	FEC_8_15,
	FEC_26_45_S,
	FEC_32_45
};


/**
 * enum fe_modulation - Type of modulation/constellation
 * @QPSK:	QPSK modulation
 * @QAM_16:	16-QAM modulation
 * @QAM_32:	32-QAM modulation
 * @QAM_64:	64-QAM modulation
 * @QAM_128:	128-QAM modulation
 * @QAM_256:	256-QAM modulation
 * @QAM_AUTO:	Autodetect QAM modulation
 * @VSB_8:	8-VSB modulation
 * @VSB_16:	16-VSB modulation
 * @PSK_8:	8-PSK modulation
 * @APSK_16:	16-APSK modulation
 * @APSK_32:	32-APSK modulation
 * @DQPSK:	DQPSK modulation
 * @QAM_4_NR:	4-QAM-NR modulation
 *
 * Please note that not all modulations are supported by a given standard.
 *
 */
enum mtk_fe_modulation {
/*
 *	QPSK,
 *	QAM_16,
 *	QAM_32,
 *	QAM_64,
 *	QAM_128,
 *	QAM_256,
 *	QAM_AUTO,
 *	VSB_8,
 *	VSB_16,
 *	PSK_8,
 *	APSK_16,
 *	APSK_32,
 *	DQPSK,
 *	QAM_4_NR,
 */

	APSK_8 = QAM_4_NR + 1,
	APSK_8_8,
	APSK_4_8_4_16,
	QPSK_R,
	QAM_16_R,
	QAM_64_R,
	QAM_256_R,
	MODULATION_AUTO
};


/**
 * enum fe_hierarchy - Hierarchy
 * @HIERARCHY_NONE:	No hierarchy
 * @HIERARCHY_AUTO:	Autodetect hierarchy (if supported)
 * @HIERARCHY_1:	Hierarchy 1
 * @HIERARCHY_2:	Hierarchy 2
 * @HIERARCHY_4:	Hierarchy 4
 *
 * @HIERARCHY_NON_NATIVE:	No hierarchy with native interleaver
 * @HIERARCHY_1_NATIVE:	Hierarchy 1 with native interleaver
 * @HIERARCHY_2_NATIVE:	Hierarchy 2 with native interleaver
 * @HIERARCHY_4_NATIVE:	Hierarchy 4 with native interleaver
 * @HIERARCHY_NON_INDEPTH:	No hierarchy with in-depth interleaver
 * @HIERARCHY_1_INDEPTH:	Hierarchy 1 with in-depth interleaver
 * @HIERARCHY_2_INDEPTH:	Hierarchy 2 with in-depth interleaver
 * @HIERARCHY_4_INDEPTH:	Hierarchy 4 with in-depth interleaver
 *
 * Please note that not all hierarchy types are supported by a given standard.
 */
enum mtk_fe_hierarchy {
	//HIERARCHY_NONE,
	//HIERARCHY_1,
	//HIERARCHY_2,
	//HIERARCHY_4,
	//HIERARCHY_AUTO,

	HIERARCHY_NON_NATIVE = HIERARCHY_AUTO + 1,
	HIERARCHY_1_NATIVE,
	HIERARCHY_2_NATIVE,
	HIERARCHY_4_NATIVE,
	HIERARCHY_NON_INDEPTH,
	HIERARCHY_1_INDEPTH,
	HIERARCHY_2_INDEPTH,
	HIERARCHY_4_INDEPTH,
};


/**
 * enum fe_interleaving - Interleaving
 * @INTERLEAVING_NONE:	No interleaving.
 * @INTERLEAVING_AUTO:	Auto-detect interleaving.
 * @INTERLEAVING_240:	Interleaving of 240 symbols.
 * @INTERLEAVING_720:	Interleaving of 720 symbols.
 *
 * Please note that, currently, only DTMB uses it.
 */
enum mtk_fe_interleaving {
	//INTERLEAVING_NONE,
	//INTERLEAVING_AUTO,
	//INTERLEAVING_240,
	//INTERLEAVING_720,

	INTERLEAVING_128_1_0 = INTERLEAVING_720 + 1,
	INTERLEAVING_128_1_1,
	INTERLEAVING_64_2,
	INTERLEAVING_32_4,
	INTERLEAVING_16_8,
	INTERLEAVING_8_16,
	INTERLEAVING_128_2,
	INTERLEAVING_128_3,
	INTERLEAVING_128_4,
};

enum mtk_fe_stream_id_type {
	STREAM_ID,
	RELATIVE_STREAM_NUMBER
};

/* DVBv5 property Commands */


/* ANALOG */
#define ATV_SOUND_SYSTEM	71

#define DTV_DVBT2_PLP_ARRAY	72

#define DTV_FREQUENCY_OFFSET	73
#define DTV_STAT_SIGNAL_QUALITY	74

#define DTV_AGC							75

#define DTV_ISDBT_LAYERA_POST_BER		76
#define DTV_ISDBT_LAYERB_POST_BER		77
#define DTV_ISDBT_LAYERC_POST_BER		78
#define DTV_ISDBT_EWBS_FLAG			79
#define DTV_DVBS2_VCM_ISID_ARRAY   80
#define DTV_DVBS2_VCM_Enable   81
#define DTV_POST_BER                   82
#define DTV_STREAM_ID_TYPE 83

// DVB_FE_DRIVER_ATSC30 start
#define DTV_ATSC3T_PLP_ID1 84
#define DTV_ATSC3T_PLP_ID2 85
#define DTV_ATSC3T_PLP_ID3 86
#define DTV_ATSC3T_PLP_ID4 87
#define DTV_ATSC3T_DEMOD_OUTPUT_FORMAT 88
#define DTV_ATSC3T_PLP1_MODULATION 89
#define DTV_ATSC3T_PLP2_MODULATION 90
#define DTV_ATSC3T_PLP3_MODULATION 91
#define DTV_ATSC3T_PLP4_MODULATION 92
#define DTV_ATSC3T_PLP1_FEC 93
#define DTV_ATSC3T_PLP2_FEC 94
#define DTV_ATSC3T_PLP3_FEC 95
#define DTV_ATSC3T_PLP4_FEC 96
#define DTV_ATSC3T_PLP1_TIME_INTERLEAVE_MODE 97
#define DTV_ATSC3T_PLP2_TIME_INTERLEAVE_MODE 98
#define DTV_ATSC3T_PLP3_TIME_INTERLEAVE_MODE 99
#define DTV_ATSC3T_PLP4_TIME_INTERLEAVE_MODE 100
#define DTV_ATSC3T_PLP1_CODE_RATE 101
#define DTV_ATSC3T_PLP2_CODE_RATE 102
#define DTV_ATSC3T_PLP3_CODE_RATE 103
#define DTV_ATSC3T_PLP4_CODE_RATE 104
#define DTV_ATSC3T_VALID_PLP_IDS 105
#define DTV_ATSC3T_LLS_FLAG 106
#define DTV_ATSC3T_PLP_STATUS 107
#define DTV_ATSC3T_FREQUENCY_OFFSET 108
#define DTV_ATSC3T_PLP_BER 109

#define MTK_DTV_MAX_COMMAND		DTV_ATSC3T_PLP_BER
// DVB_FE_DRIVER_ATSC30 end


/**
 * enum fe_rolloff - Rolloff factor
 * @ROLLOFF_35:		Roloff factor: α=35%
 * @ROLLOFF_20:		Roloff factor: α=20%
 * @ROLLOFF_25:		Roloff factor: α=25%
 * @ROLLOFF_AUTO:	Auto-detect the roloff factor.
 *
 * .. note:
 *
 *    Roloff factor of 35% is implied on DVB-S. On DVB-S2, it is default.
 */
enum mtk_fe_rolloff {
/*
 *	ROLLOFF_35,
 *	ROLLOFF_20,
 *	ROLLOFF_25,
 *	ROLLOFF_AUTO,
 */

	ROLLOFF_15 = ROLLOFF_AUTO + 1,
	ROLLOFF_10,
	ROLLOFF_5,
	ROLLOFF_3
};

enum mtk_fe_fec_type {
	FEC_LONG,
	FEC_SHORT
};

/**
 * enum fe_delivery_system - Type of the delivery system
 *
 * @SYS_UNDEFINED:
 *	Undefined standard. Generally, indicates an error
 * @SYS_DVBC_ANNEX_A:
 *	Cable TV: DVB-C following ITU-T J.83 Annex A spec
 * @SYS_DVBC_ANNEX_B:
 *	Cable TV: DVB-C following ITU-T J.83 Annex B spec (ClearQAM)
 * @SYS_DVBC_ANNEX_C:
 *	Cable TV: DVB-C following ITU-T J.83 Annex C spec
 * @SYS_ISDBC:
 *	Cable TV: ISDB-C (no drivers yet)
 * @SYS_DVBT:
 *	Terrestrial TV: DVB-T
 * @SYS_DVBT2:
 *	Terrestrial TV: DVB-T2
 * @SYS_ISDBT:
 *	Terrestrial TV: ISDB-T
 * @SYS_ATSC:
 *	Terrestrial TV: ATSC
 * @SYS_ATSCMH:
 *	Terrestrial TV (mobile): ATSC-M/H
 * @SYS_DTMB:
 *	Terrestrial TV: DTMB
 * @SYS_DVBS:
 *	Satellite TV: DVB-S
 * @SYS_DVBS2:
 *	Satellite TV: DVB-S2
 * @SYS_TURBO:
 *	Satellite TV: DVB-S Turbo
 * @SYS_ISDBS:
 *	Satellite TV: ISDB-S
 * @SYS_DAB:
 *	Digital audio: DAB (not fully supported)
 * @SYS_DSS:
 *	Satellite TV: DSS (not fully supported)
 * @SYS_CMMB:
 *	Terrestrial TV (mobile): CMMB (not fully supported)
 * @SYS_DVBH:
 *	Terrestrial TV (mobile): DVB-H (standard deprecated)
 *
 * @SYS_ANALOG:
 *	ANALOG TV
 */
enum mtk_fe_delivery_system {
	SYS_ANALOG = SYS_DVBC_ANNEX_C + 1,
	SYS_ISDBS3,
	SYS_ATSC30
};

enum mtk_fe_analog_sound_std {
	FE_SOUND_UNDEFINED = 0x0000,
	FE_SOUND_B = 0x0001,
	FE_SOUND_G = 0x0002,
	FE_SOUND_D = 0x0004,
	FE_SOUND_K = 0x0008,
	FE_SOUND_I = 0x0010,
	FE_SOUND_M = 0x0020,
	FE_SOUND_N = 0x0040,
	FE_SOUND_L = 0x0080,
	FE_SOUND_L_P = 0x0100,
};

// DVB_FE_DRIVER_ATSC30 start
#define DTV_FE_ATSC3_PLP_ID_NUM_MAX (64)

struct atsc3t_plp_bitmap {
	__u64 bitmap;
};

enum demod_output_format {
	UNDEFINED = 0,
	ATSC3_LINKLAYER_PACKET = 1 << 0,  // ALP format. Typically used in US region.
	BASEBAND_PACKET = 1 << 1,  // BaseBand packet format. Typically used in Korea region.
};

enum atsc3_plp_fec {
	ATSC3_PLP_FEC_UNDEFINED,
	ATSC3_PLP_FEC_AUTO,
	ATSC3_PLP_FEC_BCH_LDPC_16K,
	ATSC3_PLP_FEC_BCH_LDPC_64K,
	ATSC3_PLP_FEC_CRC_LDPC_16K,
	ATSC3_PLP_FEC_CRC_LDPC_64K,
	ATSC3_PLP_FEC_LDPC_16K,
	ATSC3_PLP_FEC_LDPC_64K,
};

enum atsc3_time_interleave_mode {
	ATSC3_PLP_TIME_INTERLEAVE_MODE_UNDEFINED = 0,
	ATSC3_PLP_TIME_INTERLEAVE_MODE_AUTO = 1 << 0,
	ATSC3_PLP_TIME_INTERLEAVE_MODE_CTI = 1 << 1,
	ATSC3_PLP_TIME_INTERLEAVE_MODE_HTI = 1 << 2,
};

enum atsc3_plp_code_rate {
	ATSC3_PLP_CODERATE_UNDEFINED,
	ATSC3_PLP_CODERATE_AUTO,
	ATSC3_PLP_CODERATE_2_15,
	ATSC3_PLP_CODERATE_3_15,
	ATSC3_PLP_CODERATE_4_15,
	ATSC3_PLP_CODERATE_5_15,
	ATSC3_PLP_CODERATE_6_15,
	ATSC3_PLP_CODERATE_7_15,
	ATSC3_PLP_CODERATE_8_15,
	ATSC3_PLP_CODERATE_9_15,
	ATSC3_PLP_CODERATE_10_15,
	ATSC3_PLP_CODERATE_11_15,
	ATSC3_PLP_CODERATE_12_15,
	ATSC3_PLP_CODERATE_13_15,
};
enum atsc3_plp_modulation {
	ATSC3_PLP_MODULATION_UNDEFINED = 20,
	ATSC3_PLP_MODULATION_AUTO,
	ATSC3_PLP_MODULATION_4Q,
	ATSC3_PLP_MODULATION_16Q,
	ATSC3_PLP_MODULATION_64Q,
	ATSC3_PLP_MODULATION_256Q,
	ATSC3_PLP_MODULATION_1024Q,
	ATSC3_PLP_MODULATION_4096Q,
};
// DVB_FE_DRIVER_ATSC30 end

enum lnb_overload_status {
	NON_OVERLOAD,
	OVERLOAD,
};

struct DVBS_Blindscan_TPInfo {
	__u32        Frequency;
	__u32        SymbolRate;                           ///< Parameters for DVB-S front-end
};

struct DMD_DVBS_BlindScan_Start_param {
	__u16        StartFreq;
	__u16        EndFreq;                           ///< Parameters for DVB-S front-end
};

struct DMD_DVBS_BlindScan_GetTunerFreq_param {
	__u16        TunerCenterFreq;
	__u16        TunerCutOffFreq;                           ///< Parameters for DVB-S front-end
};

struct DMD_DVBS_BlindScan_NextFreq_param {
	__u8        bBlindScanEnd;
};

struct DMD_DVBS_BlindScan_WaitCurFreqFinished_param {
	__u8        u8Progress;
	__u8        u8FindNum;
};

struct DMD_DVBS_BlindScan_GetChannel_param {
	__u16        ReadStart;
	__u16        TPNum;
	struct DVBS_Blindscan_TPInfo pTable[100];
};

#define FE_BLINDSCAN_START		   _IOW('o', 84, struct DMD_DVBS_BlindScan_Start_param)
#define FE_BLINDSCAN_GET_TUNER_FREQ	_IOR('o', 85, struct DMD_DVBS_BlindScan_GetTunerFreq_param)
#define FE_BLINDSCAN_NEXT_FREQ		   _IOR('o', 86, struct DMD_DVBS_BlindScan_NextFreq_param)
#define FE_BLINDSCAN_WAIT_CURFREQ_FINISHED \
	_IOR('o', 87, struct DMD_DVBS_BlindScan_WaitCurFreqFinished_param)
#define FE_BLINDSCAN_GET_CHANNEL	_IOR('o', 88, struct DMD_DVBS_BlindScan_GetChannel_param)
#define FE_BLINDSCAN_END		   _IO('o', 89)
#define FE_BLINDSCAN_CANCEL		   _IO('o', 90)
#define FE_GET_LNB_OVERLOAD_STATUS _IOR('o', 91, __u32)


#endif /*_MTK_DVBFRONTEND_H_*/
