/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MDRV_CFD_IF_H_
#define _MDRV_CFD_IF_H_

#ifdef _MDRV_CFD_IF_C_
#ifndef INTERFACE
#define INTERFACE
#endif
#else
#ifndef INTERFACE
#define INTERFACE extern
#endif
#endif

#include <linux/types.h>

/// Below Definition is for XXXX_Mode
/// in struct STU_CFDAPI_HDRIP and struct STU_CFDAPI_SDRIP
/* BIT7  1: Auto Mode, 0: Manuel Mode */
#define CFDAPI_MODE_CTRL_AUTO           (0x80)
/* BIT6  1: Write, 0: Not to Write */
#define CFDAPI_MODE_CTRL_WRTIE          (0x40)
/// To prevent from extral instrustion of
/// CFDAPI_MODE_CTRL_AUTO | CFDAPI_MODE_CTRL_WRTIE
#define CFDAPI_MODE_CTRL_AUTO_WRITE     (0xC0)

#define CFD_MAIN_CONTROL_ST_VERSION 0
#define CFD_MM_ST_VERSION 0
///version of struct STU_CFDAPI_HDMI_INFOFRAME_PARSER
//version 0: init version
//version 1: add HDMI 2094_10/20/30/40 pointer
#define CFD_HDMI_INFOFRAME_ST_VERSION 1
#define CFD_HDMI_INFOFRAME_OUT_ST_VERSION 0
#define CFD_HDMI_EDID_ST_VERSION 0

#define CFD_HDMI_OSD_ST_VERSION 0
#define CFD_HDMI_PANEL_ST_VERSION 0
#define CFD_HDMI_HDR_METADATA_VERSION 0
#define CFD_HDMI_TMO_ST_VERSION 0

#define CFD_KANO_TMOIP_ST_VERSION 0
#define CFD_KANO_HDRIP_ST_VERSION 0
#define CFD_KANO_SDRIP_ST_VERSION 0

#define CFD_Manhattan_TMOIP_ST_VERSION 0
#define CFD_Manhattan_HDRIP_ST_VERSION 0
#define CFD_Manhattan_SDRIP_ST_VERSION 0

#define CFD_Maserati_DLCIP_ST_VERSION 0
#define CFD_Maserati_TMOIP_ST_VERSION 0
#define CFD_Maserati_HDRIP_ST_VERSION 0
#define CFD_Maserati_SDRIP_ST_VERSION 0

#define CFD_GENERAL_CONTROL_ST_VERSION 0
#define CFD_CONTROL_POINT_ST_VERSION 0

#define CFD_GENERAL_CONTROL_ST_GOP_VERSION 0

//CFD GOP output size
#define CFD_GOP_PRESDR_RGBOFFSET_OUTPUT_REG_MAX_COMMAND   3
#define CFD_GOP_PRESDR_CSC_OUTPUT_REG_MAX_COMMAND   10
#define CFD_GOP_PRESDR_RGBOFFSET_OUTPUT_ADL_MAX_NUM    3

//CFD output size
#define CFD_PRESDR_GENERAL_OUTPUT_REG_MAX_COMMAND   10
#define CFD_PRESDR_IPRANGECONVY_OUTPUT_REG_MAX_COMMAND   10
#define CFD_PRESDR_IPRANGECONVC_OUTPUT_REG_MAX_COMMAND   10
#define CFD_PRESDR_IPCSC_OUTPUT_REG_MAX_COMMAND   20
#define CFD_PRESDR_YHSLY2R_OUTPUT_REG_MAX_COMMAND   10
#define CFD_PRESDR_YHSLR2Y_OUTPUT_REG_MAX_COMMAND   10
#define CFD_HDR_GENERAL_OUTPUT_REG_MAX_COMMAND   90
#define CFD_HDR_IPRANGECONVY_OUTPUT_REG_MAX_COMMAND   10
#define CFD_HDR_IPRANGECONVC_OUTPUT_REG_MAX_COMMAND   10
#define CFD_HDR_IPCSC_OUTPUT_REG_MAX_COMMAND   40
#define CFD_HDR_DEGAMMA_OUTPUT_REG_MAX_COMMAND   50
#define CFD_HDR_GAMUT_MAPPING_OUTPUT_REG_MAX_COMMAND   30
#define CFD_HDR_OOTF_OUTPUT_REG_MAX_COMMAND   60
#define CFD_HDR_OOTFR2Y_OUTPUT_REG_MAX_COMMAND   10
#define CFD_HDR_RGB3DLUT_OUTPUT_REG_MAX_COMMAND   60
#define CFD_HDR_GAMMA_OUTPUT_REG_MAX_COMMAND   70
#define CFD_HDR_OPCSC_OUTPUT_REG_MAX_COMMAND   30
#define CFD_HDR_TMOY2R_OUTPUT_REG_MAX_COMMAND   20
#define CFD_HDR_UVCY2R_OUTPUT_REG_MAX_COMMAND   10
#define CFD_HDR_OPRANGECONVY_OUTPUT_REG_MAX_COMMAND   10
#define CFD_HDR_OPRANGECONVC_OUTPUT_REG_MAX_COMMAND   10
#define CFD_HDR_TMO_OUTPUT_REG_MAX_COMMAND   50
#define CFD_HDR_YLITER2Y_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_GENERAL_OUTPUT_REG_MAX_COMMAND   40
#define CFD_POSTSDR_VOPCSC_OUTPUT_REG_MAX_COMMAND   30
#define CFD_POSTSDR_DEGAMMA_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_GAMUT_MAPPING_OUTPUT_REG_MAX_COMMAND   20
#define CFD_POSTSDR_GAMMA_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_RGB3DLUT_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_VIPIN_RANGECONVY_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_VIPIN_RANGECONVC_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_DLCIN_RANGECONVY_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_DLCIN_RANGECONVC_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_DLCOUT_RANGECONVY_OUTPUT_REG_MAX_COMMAND   10
#define CFD_POSTSDR_DLCOUT_RANGECONVC_HW_OUTPUT_REG_MAX_COMMAND   20
#define CFD_POSTSDR_HSYMAINCSC_OUTPUT_REG_MAX_COMMAND   40
#define CFD_POSTSDR_HSYSUBCSC_OUTPUT_REG_MAX_COMMAND   30
#define CFD_POSTSDR_UVCMAINY2R_OUTPUT_REG_MAX_COMMAND   10
#define CFD_CHROMASAMPLING_OUTPUT_REG_MAX_COMMAND (64)
#define CFD_FILMGRAIN_OUTPUT_REG_MAX_COMMAND (64)

#define CFD_HDR_DEGAMMA_OUTPUT_ADL_MAX_NUM    512
#define CFD_HDR_OOTF_OUTPUT_ADL_MAX_NUM       512
#define CFD_HDR_RGB3DLUT_OUTPUT_ADL_MAX_NUM   (6000*3)
#define CFD_HDR_GAMMA_OUTPUT_ADL_MAX_NUM      512
#define CFD_HDR_TMO_GAMMA_OUTPUT_ADL_MAX_NUM  512
#define CFD_HDR_TMO_TMO_OUTPUT_ADL_MAX_NUM    512
#define CFD_POSTSDR_DEGAMMA_OUTPUT_ADL_MAX_NUM   600
#define CFD_POSTSDR_GAMMA_OUTPUT_ADL_MAX_NUM     344
#define CFD_POSTSDR_RGB3DLUT_OUTPUT_ADL_MAX_NUM  (6000*3)
#define CFD_FILMGRAIN_OUTPUT_ADL_MAX_NUM         (1300)	//1287
#define CFD_FILMGRAIN_OUTPUT_ADL_LEN_PER_CMD     (256)

//422 Struce
//Version 0 init struct
//Version 1 add u16HDRPos && bIsHVSP
#define CFD_CONTROL_420_VERSION (1)

enum SC_TMO_QUALITY_MAP_INDEX_e_Mainy {
	QM_RESERVED0_TMO_Main,	//0
	QM_BT709_TMO_Main,	//1
	QM_UNSPECIFIED_TMO_Main,	//2
	QM_RESERVED3_TMO_Main,	//3
	QM_GAMMA2P2_TMO_Main,	//4
	QM_GAMMA2P8_TMO_Main,	//5
	QM_BT601525_601625_TMO_Main,	//6
	QM_SMPTE240M_TMO_Main,	//7
	QM_LINEAR_TMO_Main,	//8
	QM_LOG0_TMO_Main,	//9
	QM_LOG1_TMO_Main,	//10
	QM_XVYCC_TMO_Main,	//11
	QM_BT1361_TMO_Main,	//12
	QM_SRGB_SYCC_TMO_Main,	//13
	QM_BT2020NCL_TMO_Main,	//14
	QM_BT2020CL_TMO_Main,	//15
	QM_SMPTE2084_TMO_Main,	//16
	QM_SMPTE428_TMO_Main,	//17
	QM_HLG_TMO_Main,	//18
	QM_BT1886_TMO_Main,	//19
	QM_DOLBYMETA_TMO_Main,	//20
	QM_ADOBERGB_TMO_Main,	//21
	QM_TMO_INPUTTYPE_NUM_Main,	// 22
};

enum InforClient {
	E_Gamut,		//          0
	E_IP_Dataformat,	//            1
	E_OP1_Dataformat,	//           2
	E_OP2_Dataformat,	//           3
	E_VOP_Dataformat,	//     4
	E_TransFunction,	//   5
	E_Info_CLIENT_MAX
};

enum E_CFD_IP_CSC_PROCESS {
	E_CFD_IP_CSC_OFF = 0x0,
	E_CFD_IP_CSC_RFULL_TO_RLIMIT = 0x1,
	E_CFD_IP_CSC_RFULL_TO_YFULL = 0x2,
	E_CFD_IP_CSC_RFULL_TO_YLIMIT = 0x3,
	E_CFD_IP_CSC_RLIMIT_TO_RFULL = 0x4,
	E_CFD_IP_CSC_RLIMIT_TO_YFULL = 0x5,
	E_CFD_IP_CSC_RLIMIT_TO_YLIMIT = 0x6,
	E_CFD_IP_CSC_YFULL_TO_RFULL = 0x7,
	E_CFD_IP_CSC_YFULL_TO_RLIMIT = 0x8,
	E_CFD_IP_CSC_YFULL_TO_YLIMIT = 0x9,
	E_CFD_IP_CSC_YLIMIT_TO_RFULL = 0xA,
	E_CFD_IP_CSC_YLIMIT_TO_RLIMIT = 0xB,
	E_CFD_IP_CSC_YLIMIT_TO_YFULL = 0xC,
	E_CFD_IP_CSC_MANUAL = 0xD,
	E_CFD_IP_CSC_RESERVED = 0xE
};

//u8HDMISource_SMD_ID
enum E_CFD_HDMI_HDR_INFOFRAME_METADATA {
	E_CFD_HDMI_META_TYPE1 = 0x0,
	E_CFD_HDMI_META_RESERVED = 0x1
};

//u8MM_Codec
enum E_CFD_VALIDORNOT {
	E_CFD_NOT_VALID = 0x0,
	E_CFD_VALID = 0x1,
	E_CFD_VALID_EMUEND = 0x2
};

//enum for u8HDR2SDR_Mode/u8SDR_Mode
enum E_CFD_RESERVED_START {
	E_CFD_RESERVED_AT0x00 = 0x00,
	E_CFD_RESERVED_AT0x01 = 0x01,
	E_CFD_RESERVED_AT0x02 = 0x02,
	E_CFD_RESERVED_AT0x03 = 0x03,
	E_CFD_RESERVED_AT0x04 = 0x04,
	E_CFD_RESERVED_AT0x05 = 0x05,
	E_CFD_RESERVED_AT0x06 = 0x06,	/// for HDR10+
	E_CFD_RESERVED_AT0x07 = 0x07,	/// for TCH Linear mode
	E_CFD_RESERVED_AT0x08 = 0x08,
	E_CFD_RESERVED_AT0x0D = 0x0D,
	E_CFD_RESERVED_AT0x0E = 0x0E,
	E_CFD_RESERVED_AT0x80 = 0x80,
	E_CFD_RESERVED_RESERVED_START
};

//enum for u8HDR2SDR_Mode/u8SDR_Mode
enum E_CFD_ASSIGN_MODE {
	E_CFD_MODE_AT0x00 = 0x00,
	E_CFD_MODE_AT0x01 = 0x01,
	E_CFD_MODE_AT0x02 = 0x02,
	E_CFD_MODE_AT0x03 = 0x03,
	E_CFD_MODE_AT0x04 = 0x04,
	E_CFD_MODE_AT0x05 = 0x05,
	E_CFD_MODE_RESERVED_START
};

enum E_12P_TMO_VERSION {
	E_12P_TMO_VERSION_0 = 0x0,
	E_12P_TMO_VERSION_1 = 0x1,
	E_12P_TMO_VERSION_2 = 0x2,
	E_12P_TMO_VERSION_3 = 0x3,
	E_12P_TMO_VERSION_4 = 0x4,
	E_12P_TMO_VERSION_5 = 0x5,	/// HDR10+
};

enum E_CFD_PATH_MODE {
	E_MIAN_PATH = 0x0,
	E_SUB_PATH = 0x1,
	E_GOP_PATH = 0x2,
	E_HFR_PATH = 0x3,
};

struct STU_CFD_SHARE_INFORMATION {
	__u8 u8Gamut4cQmap;
	__u8 u8IP_DataFormat;
	__u8 u8OP1_DataFormat;
	__u8 u8OP2_DataFormat;
	__u8 u8VOP_DataFormat;
	__u8 u8TransferFunction;
};

struct STU_CFDAPI_DOLBY_CONTROL {
	__u8 *pu8CompMetadta;
	__u8 *pu8DmMetadta;
	void *pCompRegtable;	//MsHdr_Comp_Regtable_t
	void *pCompConfig;	//DoVi_Comp_ExtConfig_t
	void *pDmRegtable;	//MsHdr_RegTable_t
	void *pDmConfig;	//DoVi_Config_t
	void *pDmMds;		//DoVi_MdsExt_t
};

struct HAL_VPQ_HDR_TOP {
	__u32 *ptr_Hal_VPQ_HDR_EOTF;
	__u32 *ptr_Hal_VPQ_HDR_OETF;
	__u32 *ptr_Hal_VPQ_HDR_OOTF;
	__u16 *ptr_Hal_VPQ_HDR_ToneMap;
	__u16 *ptr_Hal_VPQ_HDR_RGB3DLUT;
	__s32 *ptr_Hal_VPQ_HDR_GamutMatrixHDR;
	__s32 *ptr_Hal_VPQ_SDR_GamutMatrixSDR;
	__u16 *ptr_Hal_VPQ_SDR_RGB3DLUT;
	__u32 *ptr_Hal_VPQ_SDR_deGamma;
	__u32 *ptr_Hal_VPQ_SDR_Gamma;
};

/// HDR10plus Metadata Output
struct STU_CFDAPI_HDR10PLUSIP {
	__u8 u8HDR10plus_Enable;
	// 0: Disable HDR10+ processing in TMO
	// 1: Enable HDR10+ processing in TMO
	// If HDR10plus_Metadata_Valid=0, set to Disable
	// If Graphics_Overlay_Flag=1, set to Disable

	__u8 u8HDR10plus_Metadata_Valid;
	// 0: Not valid
	// 1: Valid

	__u8 u8Input_Source;
	// 0: MM SEI
	// 1: HDMI VSIF

	__u16 u16Panel_Max_Luminance;

	__u16 u16Target_System_Display_Max_Luminance;
	// MM SEI: [0, max:10000] nits
	// HDMI VSIF: [0, 31] * 32nits = [0, 992] nits
	// if tone_mapping_flag=0,
	// then targeted_system_display_maximum_luminance=0

	__u32 u32MaxSCL[3];
	// MM SEI Only
	// [0, 10000] * 10 (shall be divided by 10 when processing)
	// MAY be set to 0x00000 to indicate "Not Used" by the content provider

	__u32 u32Average_MaxRGB;
	// MM SEI: [0, 10000] * 10 (shall be divided by 10 when processing)
	// HDMI VSIF: [0, 255] * 16 = [0, 4080]

	__u8 u8Distribution_Index[9];
	// MM SEI Only
	// [0]: 1
	// [1]: 5
	// [2]: 10
	// [3]: 25
	// [4]: 50
	// [5]: 75
	// [6]: 90
	// [7]: 95
	// [8]: 99

	__u32 u32Distribution_Values[9];
	// [0]: 1% percentile linearized maxRGB value * 10
	// [1]: DistributionY99 * 10
	// [2]: DistributionY100nits
	// [3]: 25% percentile linearized maxRGB value * 10
	// [4]: 50% percentile linearized maxRGB value * 10
	// [5]: 75% percentile linearized maxRGB value * 10
	// [6]: 90% percentile linearized maxRGB value * 10
	// [7]: 95% percentile linearized maxRGB value * 10
	// [8]: 99.98% percentile linearized maxRGB value * 10
	// [idx=2]: 0-100
	// [others]: MM SEI: [0, 10000] * 10
	// (shall be divided by 10 when processing)
	//           HDMI VSIF: [0, 255] * 16 = [0, 4080]

	__u8 u8Tone_Mapping_Flag;
	// set to 0 for Profile A
	// set to 1 for Profile B: with Basis Tone Mapping Curve (Bezier curve)

	__u32 u32Knee_Point_x;
	__u32 u32Knee_Point_y;
	// When in Profile A: Format: 12.8
	// When in Profile B:
	// 12 bits
	// MM SEI: [0, 4095]
	// HDMI VSIF: [0, 1023] * 4 = [0, 4092]

	__u8 u8Num_Bezier_Curve_Anchors;
	// 0-9
	// if value=0, no Bezier_Curve_Anchors information

	__u32 u32Bezier_Curve_Anchors[9];
	// When in Profile A: Format: 10.10
	// When in Profile B:
	// 10 bits
	// MM SEI: [0, 1023]
	// HDMI VSIF: [0, 255]*4 = [0, 1020]

	__u8 u8Graphics_Overlay_Flag;
	// HDMI VSIF Only
	// 1:
	// video signal associated with the HDR10+ Metadata has graphics overlay

	__u8 u8No_Delay_Flag;
	// HDMI VSIF Only (vsif_timing_mode)
	// SHALL always set to 0
	// set to 0: video signal associated with the HDR10+ Metadata
	// is transmitted one frame after the HDR10+ Metadata
	//               i.e. HDR10+ Metadata is transmitted one frame ahead of
	//               the associated video signal
	// set to 1: video signal associated with the HDR10+ Metadata
	// is transmitted synchronously
};

struct STU_CFDAPI_TCHPARAM {
	__u16 u16Panel_Max_Luminance;
};

struct STU_CFDAPI_HDR10PLUS_BASISOOTF_PARA {
	//==============================
	// Knee-Point (KP) parameters
	//==============================
	// KP ramp base thresholds (two bounds KP # 1 and KP # 2 are computed)
	__u32 u32SY1_V1;
	__u32 u32SY1_V2;
	__u32 u32SY1_T1;
	__u32 u32SY1_T2;
	__u32 u32SY2_V1;
	__u32 u32SY2_V2;
	__u32 u32SY2_T1;
	__u32 u32SY2_T2;

	// KP mixing gain (final KP from bounds KP # 1 and KP # 2
	// as a function of scene percentile)
	__u32 u32KP_G_V1;
	__u32 u32KP_G_V2;
	__u32 u32KP_G_T1;
	__u32 u32KP_G_T2;

	//==============================
	// P coefficient parameters
	//==============================
	// Thresholds of minimum bound of P1 coefficient
	__u32 u32P1_LIMIT_V1;
	__u32 u32P1_LIMIT_V2;
	__u32 u32P1_LIMIT_T1;
	__u32 u32P1_LIMIT_T2;

	// Thresholds to compute relative shape of curve (P2~P9 coefficient)
	// by pre-defined bounds - as a function of scene percentile
	__u32 u32P2To9_T1;
	__u32 u32P2To9_T2;

	// Defined relative shape bounds (P2~P9 coefficient) for a given
	// maximum TM dynamic compression (eg : 20x )
	__u32 u32P2ToP9_MAX1[8];
	__u32 u32P2ToP9_MAX2[8];

	// Ps mixing gain (obtain all Ps coefficients)
	// - as a function of TM dynamic compression ratio
	__u32 u32PS_G_T1;
	__u32 u32PS_G_T2;

	// Post-processing : Reduce P1/P2 (to enhance mid tone)
	// for high TM dynamic range compression cases
	__u32 u32LOW_SY_T1;
	__u32 u32LOW_SY_T2;
	__u32 u32LOW_K_T1;
	__u32 u32LOW_K_T2;
	__u32 u32RED_P1_V1;
	__u32 u32RED_P1_T1;
	__u32 u32RED_P1_T2;
	__u32 u32RED_P2_V1;
	__u32 u32RED_P2_T1;
	__u32 u32RED_P2_T2;
};

struct STU_CFD_MS_ALG_INTERFACE_DLC {
	struct STU_CFDAPI_DLCIP stu_Maserati_DLC_Param;
};

struct STU_CFD_MS_ALG_INTERFACE_TMO {
	__u8 u8Controls;
	///0 : bypass
	///1 : normal
	///2 : test

	//STU_CFDAPI_Kano_TMOIP        stu_Kano_TMOIP_Param;
	//STU_CFDAPI_Manhattan_TMOIP   stu_Manhattan_TMOIP_Param;
	struct STU_CFDAPI_TMOIP stu_Maserati_TMO_Param;

	/// HDR10plus Output Metadata
	struct STU_CFDAPI_HDR10PLUSIP stu_Maserati_HDR10plus_Param;

	/// TCH TMO parameter
	struct STU_CFDAPI_TCHPARAM stu_TCH_Param;
};

struct STU_CFD_MS_ALG_INTERFACE_HDRIP {
	//STU_CFDAPI_Kano_HDRIP stu_Kano_HDRIP_Param;
	//STU_CFDAPI_Manhattan_HDRIP stu_Manhattan_HDRIP_Param;
	struct STU_CFDAPI_HDRIP stu_Maserati_HDRIP_Param;
};

struct STU_CFD_MS_ALG_INTERFACE_SDRIP {
	__u8 u8Controls;
	///0 : bypass
	///1 : normal
	///2 : test

	//STU_CFDAPI_Kano_SDRIP stu_Kano_SDRIP_Param;
	//STU_CFDAPI_Manhattan_SDRIP stu_Manhattan_SDRIP_Param;
	struct STU_CFDAPI_SDRIP stu_Maserati_SDRIP_Param;
};

struct STU_CFDAPI_420CONTROL {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_MAIN_CONTROL_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure, u16Length=sizeof(STU_CFDAPI_Kano_SDRIP)

	__u8 u8Input_DataMode;
	// 0:I mode, 1:P mode
	__u8 u8Input_Dolby;
	// 0:Dolby, 1:non-Dolby

	enum EN_CFD_MC_FORMAT Input_DataFormat;
	enum EN_CFD_MC_FORMAT IP1_DataFormat;
	enum EN_CFD_MC_FORMAT DRAM_Format;
	enum EN_CFD_MC_FORMAT Output_Format;
	enum EN_CFD_INPUT_SOURCE Input_Source;
	__u16 u16HDRPos;
	bool bIsHVSPEnable;
	//HFR VSD enable
	bool bIsVSDEnable;
};

struct STU_CFD_MS_ALG_INTERFACE_420CONTROL {
	__u8 u8Controls;
	///0 : bypass
	///1 : normal
	///2 : test

	//STU_CFDAPI_Kano_SDRIP stu_Kano_SDRIP_Param;
	//STU_CFDAPI_Manhattan_SDRIP stu_Manhattan_SDRIP_Param;
	struct STU_CFDAPI_420CONTROL stu_Maserati_420CONTROL_Param;
};

struct STU_CFD_MS_ALG_INTERFACE_GOP_PRESDRIP {
	struct STU_CFDAPI_GOP_PRESDRIP stu_GOP_PRESDRIP_Param;
};

struct STU_CFDAPI_HW_IPS {
	//Main sub control mode
	__u8 u8HW_MainSub_Mode;
	//0: current control is for SC0 (Main)
	//1: current control is for SC1 (Sub)
	//2-255 is reversed

	struct STU_CFD_MS_ALG_INTERFACE_DLC *pstu_DLC_Input;
	struct STU_CFD_MS_ALG_INTERFACE_TMO *pstu_TMO_Input;
	struct STU_CFD_MS_ALG_INTERFACE_HDRIP *pstu_HDRIP_Input;
	struct STU_CFD_MS_ALG_INTERFACE_SDRIP *pstu_SDRIP_Input;
};

struct STU_CFDAPI_HW_IPS_GOP {
	//u8HWGroup is for GOP group ID, starts from 0
	__u8 u8HWGroup;

	struct STU_CFD_MS_ALG_INTERFACE_GOP_PRESDRIP *pstu_PRESDRIP_Input;
};

// new API for directly input table start=======================================
struct STU_CFDAPI_PREPROCESS_STATUS {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_CFD_MS_ALG_COLOR_FORMAT stu_Cfd_color_format;
	struct STU_CFD_DePQRefence stu_Cfd_DePQRefence;
	struct HDR_CONTENT_LIGHT_MATADATA stu_CLL_metadata;
	struct STU_CFD_SHARE_INFORMATION stu_Cfd_share_info[2];
	struct STU_CFD_MS_ALG_INTERFACE_HDRIP stu_CFD_HDRIP;
	struct STU_CFD_MS_ALG_INTERFACE_SDRIP stu_CFD_SDRIP;
	struct STU_CFD_MS_ALG_INTERFACE_TMO stu_CFD_TMO;
	struct STU_CFD_MS_ALG_INTERFACE_DLC stu_CFD_DLC;
};

struct STU_CFDAPI_PREPROCESS_STATUS_LITE {
	__u32 u32Version;
	__u16 u16Length;

	struct STU_CFD_MS_ALG_COLOR_FORMAT_LITE stu_Cfd_color_format;
};

struct STU_CFDAPI_PREPROCESS_OUT {
	__u32 u32Version;
	__u16 u16Length;

	struct STU_CFDAPI_HDMI_INFOFRAME_PARSER_OUT
		stu_Cfd_HDMI_infoFrame_Parameters_out;
	struct STU_CFDAPI_PREPROCESS_STATUS stu_Cfd_preprocess_status;
};

struct STU_CFDAPI_GOP_PREPROCESS_OUT {
	__u32 u32Version;
	__u16 u16Length;

	struct STU_CFDAPI_PREPROCESS_STATUS_LITE stu_Cfd_preprocess_status;
};

struct STU_CFDAPI_HDR_TABLE {
	// 0: Disable table direct input from API
	// 1: Input from HAL_VPQ_HDR
	__u8 u8API_mode;

	__u8 u8API_Eoft_unEqualInterval_En;
	__u8 u8API_Oeft_unEqualInterval_En;
	__u8 u8API_Ooft_unEqualInterval_En;

	struct HAL_VPQ_HDR_TOP Hal_VPQ_HDR;
};

struct STU_CFDAPI_TOP_CONTROL {
	//share with different HW
	__u32 u32Version;
	__u16 u16Length;
	struct STU_CFDAPI_MAIN_CONTROL *pstu_Main_Control;
	struct STU_CFDAPI_MM_PARSER *pstu_MM_Param;
	struct STU_CFDAPI_HDMI_EDID_PARSER *pstu_HDMI_EDID_Param;
	struct STU_CFDAPI_HDMI_INFOFRAME_PARSER *pstu_HDMI_InfoFrame_Param;
	struct STU_CFDAPI_HDR_METADATA_FORMAT *pstu_HDR_Metadata_Format_Param;
	struct STU_CFDAPI_PANEL_FORMAT *pstu_Panel_Param;
	struct STU_CFDAPI_UI_CONTROL *pstu_OSD_Param;

	//HDR vendor-specific zone
	struct STU_CFDAPI_DOLBY_CONTROL *pstu_Dolby_Param;

	struct STU_CFDAPI_HW_IPS *pstu_HW_IP_Param;

	struct STU_CFDAPI_HDR_TABLE *pstu_HDR_TABLE_Param;
	struct XC_KDRV_REF_2086_METADATA *pstu_ref_2086_metadata;
};

struct STU_CFDAPI_TOP_CONTROL_GOP {
	//share with different HW
	__u32 u32Version;
	__u16 u16Length;
	struct STU_CFDAPI_MAIN_CONTROL_GOP *pstu_Main_Control;
	//struct STU_CFDAPI_MM_PARSER           *pstu_MM_Param;
	//struct STU_CFDAPI_HDMI_EDID_PARSER    *pstu_HDMI_EDID_Param;
	struct STU_CFDAPI_UI_CONTROL *pstu_UI_Param;
	struct STU_CFDAPI_PANEL_FORMAT *pstu_Panel_Param;
	struct STU_CFDAPI_HW_IPS_GOP *pstu_HW_IP_Param;
	struct STU_CFDAPI_GOP_FORMAT *pstu_GOP_Param;

	struct STU_CFDAPI_DEBUG *pstu_Debug_Param;
};

struct STU_CFDAPI_MAIN_CONTROL_GOP_TESTING {
	__u32 u32TestCases;
};

struct STU_CFDAPI_UI_CONTROL_TESTING {
	__u32 u32TestCases;
};

struct STU_CFDAPI_TOP_CONTROL_GOP_TESTING {
	//share with different HW
	__u32 u32Version;
	__u16 u16Length;

	__u8 u8TestEn;

	struct STU_CFDAPI_MAIN_CONTROL_GOP_TESTING stu_Main_Control;

	struct STU_CFDAPI_UI_CONTROL_TESTING stu_UI_Param;
	//struct STU_CFDAPI_PANEL_FORMAT_TESTING       stu_Panel_Param;
	//struct STU_CFDAPI_HW_IPS_GOP_TESTING         stu_HW_IP_Param;
	//struct STU_CFDAPI_GOP_FORMAT_TESTING         stu_GOP_Param;
};

struct ST_CFD_STATUS {
	__u32 u32Version;
	__u16 u16Length;
	__u16 u16UpdateStatus;	// Check whitch IPs need to update
	//CFD path
	// 0: main path
	// 1: sub path
	// 2: GOP path
	// 3: HFR path
	__u8 u8Path;
};

// Coloect all output
struct ST_CFD_GENERAL_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_RANGECONVY_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_RANGECONVC_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_CSC_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_DEGAMMA_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_GAMUT_MAPPING_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_OOTF_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_3DLUT_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_GAMMA_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTable;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_TMO_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct STU_Register_Table stRegTable;
	struct STU_Autodownload_Table stAdlTableGamma;
	struct STU_Autodownload_Table stAdlTableTmo;
	struct ST_CFD_STATUS stStatus;
};

struct ST_CFD_IP_ML_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;

	struct STU_Register_Table stRegTable;
};

struct ST_CFD_OP_ML_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;

	struct STU_Register_Table stRegTable;
};

struct ST_CFD_PreSDR_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct ST_CFD_GENERAL_HW_OUTPUT stu_cfd_presdr_general_hw_output;
	struct ST_CFD_RANGECONVY_HW_OUTPUT
		stu_cfd_presdr_ipRangeConvY_hw_output;
	struct ST_CFD_RANGECONVC_HW_OUTPUT
		stu_cfd_presdr_ipRangeConvC_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_presdr_ipcsc_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_presdr_yhsly2r_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_presdr_yhslr2y_hw_output;
};

struct ST_CFD_GOP_PreSDR_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_GOP_PreSDR_CSC_hw_output;
	struct ST_CFD_GENERAL_HW_OUTPUT stu_cfd_GOP_PreSDR_RGBOffset_hw_output;
};

struct ST_CFD_HDR_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct ST_CFD_GENERAL_HW_OUTPUT stu_cfd_hdr_general_hw_output;
	struct ST_CFD_RANGECONVY_HW_OUTPUT stu_cfd_hdr_ipRangeConvY_hw_output;
	struct ST_CFD_RANGECONVC_HW_OUTPUT stu_cfd_hdr_ipRangeConvC_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_hdr_ipcsc_hw_output;
	struct ST_CFD_DEGAMMA_HW_OUTPUT stu_cfd_hdr_degamma_hw_output;
	struct ST_CFD_GAMUT_MAPPING_HW_OUTPUT
		stu_cfd_hdr_gamut_mapping_hw_output;
	struct ST_CFD_OOTF_HW_OUTPUT stu_cfd_hdr_ootf_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_hdr_ootfR2y_hw_output;
	struct ST_CFD_3DLUT_HW_OUTPUT stu_cfd_hdr_rgb3dlut_hw_output;
	struct ST_CFD_GAMMA_HW_OUTPUT stu_cfd_hdr_gamma_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_hdr_opcsc_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_hdr_tmoY2r_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_hdr_uvcY2r_hw_output;
	struct ST_CFD_RANGECONVY_HW_OUTPUT stu_cfd_hdr_opRangeConvY_hw_output;
	struct ST_CFD_RANGECONVC_HW_OUTPUT stu_cfd_hdr_opRangeConvC_hw_output;
	struct ST_CFD_TMO_HW_OUTPUT stu_cfd_tmo_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_hdr_yhslr2y_hw_output;
};

struct ST_CFD_POSTSDR_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct ST_CFD_GENERAL_HW_OUTPUT stu_cfd_sdr_general_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_sdr_vopcsc_hw_output;
	struct ST_CFD_DEGAMMA_HW_OUTPUT stu_cfd_sdr_degamma_hw_output;
	struct ST_CFD_GAMUT_MAPPING_HW_OUTPUT
		stu_cfd_sdr_gamut_mapping_hw_output;
	struct ST_CFD_GAMMA_HW_OUTPUT stu_cfd_sdr_gamma_hw_output;
	struct ST_CFD_3DLUT_HW_OUTPUT stu_cfd_sdr_rgb3dlut_hw_output;
	struct ST_CFD_RANGECONVY_HW_OUTPUT stu_cfd_dlcin_RangeConvY_hw_output;
	struct ST_CFD_RANGECONVC_HW_OUTPUT stu_cfd_dlcin_RangeConvC_hw_output;
	struct ST_CFD_RANGECONVY_HW_OUTPUT stu_cfd_dlcout_RangeConvY_hw_output;
	struct ST_CFD_RANGECONVC_HW_OUTPUT stu_cfd_dlcout_RangeConvC_hw_output;
	struct ST_CFD_RANGECONVY_HW_OUTPUT stu_cfd_vipin_RangeConvY_hw_output;
	struct ST_CFD_RANGECONVC_HW_OUTPUT stu_cfd_vipin_RangeConvC_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_sdr_hsymaincsc_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_sdr_hsysubcsc_hw_output;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_sdr_uvcmainy2r_hw_output;
};

struct STU_CFD_GENERAL_CONTROL {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_GENERAL_CONTROL_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFD_GENERAL_CONTROL)

	__u8 u8InputSource;
	__u8 u8MainSubMode;
	__u8 u8IsRGBBypass;
	__u8 u8InputDataDepth;

	//for process status
	//In SDR IP
	__u8 u8DoTMO_Flag;
	__u8 u8DoGamutMapping_Flag;
	__u8 u8DoDLC_Flag;
	__u8 u8DoBT2020CLP_Flag;

	//In HDR IP
	__u8 u8DoTMOInHDRIP_Flag;
	__u8 u8DoGamutMappingInHDRIP_Flag;
	__u8 u8DoDLCInHDRIP_Flag;
	__u8 u8DoBT2020CLPInHDRIP_Flag;
	//__u8 u8DoFull2LimitInHDRIP_Flag;
	__u8 u8DoForceEnterHDRIP_Flag;
	//__u8 u8DoForceFull2LimitInHDRIP_Flag;

	//for process status2
	__u8 u8DoMMIn_ForceHDMI_Flag;
	__u8 u8DoMMIn_Force709_Flag;
	__u8 u8DoHDMIIn_Force709_Flag;
	__u8 u8DoOtherIn_Force709_Flag;
	__u8 u8DoOutput_Force709_Flag;

	//for process status 3
	__u8 u8DoHDRIP_Forcebypass_Flag;
	__u8 u8DoSDRIP_ForceNOTMO_Flag;
	__u8 u8DoSDRIP_ForceNOGM_Flag;
	__u8 u8DoSDRIP_ForceNOBT2020CL_Flag;

	//
	__u8 u8DoPreConstraints_Flag;

	//E_CFD_CFIO_RANGE_LIMIT
	//E_CFD_CFIO_RANGE_FULL
	__u8 u8DoPathFullRange_Flag;

	// TMO CONTROL
	__u16 u16SourceMax;	//target maximum in nits, 1~10000
	__u8 u16SourceMaxFlag;	//flag of target maximum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16SourceMed;	//target minimum in nits, 1~10000
	__u8 u16SourceMedFlag;	//flag of target maximum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16SourceMin;	//target minimum in nits, 1~10000
	__u8 u16SourceMinFlag;	//flag of target minimum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16TgtMax;	//target maximum in nits, 1~10000
	__u8 u16TgtMaxFlag;	//flag of target maximum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16TgtMed;	//target maximum in nits, 1~10000
	__u8 u16TgtMedFlag;	//flag of target minimum
	//0: the unit of u16Luminance is 1 nits
	//1: the unit of u16Luminance is 0.0001 nits

	__u16 u16TgtMin;	//target minimum in nits, 1~10000
	__u8 u16TgtMinFlag;	//flag of target minimum

	__s16 u16DolbySupportStatus;

	//CFD path
	// 0: main path
	// 1: sub path
	// 2: GOP path
	// 3: HFR path
	__u8 u8Path;
};

struct STU_CFD_GENERAL_CONTROL_GOP {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_MAIN_CONTROL_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure, u16Length=sizeof(STU_CFD_MAIN_CONTROL)

	//__u8 u8InputSource;
	//__u8 u8MainSubMode;
	//__u8 u8IsRGBBypass;

	__u8 u8HWGroupMode;
	__u8 u8HWGroup;

	//E_CFD_CFIO_RANGE_LIMIT
	//E_CFD_CFIO_RANGE_FULL
	//__u8 u8DoPathFullRange;
	struct STU_CFDAPI_DEBUG stu_Debug_Param;
};

struct STU_CFD_CONTROL_POINT {
	__u32 u32Version;
	///<Version of current structure.
	///Please always set to "CFD_CONTROL_POINT_ST_VERSION" as input
	__u16 u16Length;
	///<Length of this structure,
	///u16Length=sizeof(struct STU_CFD_CONTROL_POINT)
	__u8 u8MainSubMode;
	__u8 u8Format;
	__u8 u8DataFormat;
	__u8 u8IsFullRange;
	__u8 u8HDRMode;
	__u8 u8SDRIPMode;
	__u8 u8HDRIPMode;
	__u8 u8GamutOrderIdx;
	__u8 u8ColorPriamries;
	__u8 u8TransferCharacterstics;
	__u8 u8MatrixCoefficients;
	__u8 u8InputBitExtendMode;

	__u16 u16BlackLevelY;
	__u16 u16BlackLevelC;
	__u16 u16WhiteLevelY;
	__u16 u16WhiteLevelC;

	/// HDR10plus
	__u8 u8HDR10plus_Enable;
	__u8 u8Tone_Mapping_Flag;
	__u32 u16PanelMaxLuminance;
	__u32 u32MaxRGB;
	__u8 u8Graphics_Overlay_Flag;

	// new part
	// for CFD control
	// 0: call Mapi_Cfd_Preprocessing and Mapi_Cfd_Decision
	// 1: call Mapi_Cfd_Preprocessing only
	__u8 u8Cfd_process_mode;

	// Process Mode
	// 0: color format driver on - auto
	// CFD process depends on the input of CFD
	// 1: shows SDR UI, UI 709, no video
	__u8 u8PredefinedProcess;

	// report Process
	// bit0 DoDLCflag
	// bit1 DoGamutMappingflag
	// bit2 DoTMOflag
	__u8 u8Process_Status;
	__u8 u8Process_Status2;
	__u8 u8Process_Status3;

	__u16 u16_check_status;
	struct STU_CFD_COLORIMETRY stu_Source_Param_Colorimetry;
	struct STU_CFD_COLORIMETRY stu_Panel_Param_Colorimetry;
};

struct ST_CFD_PRESDR_INPUT {
	struct STU_CFD_GENERAL_CONTROL *pstu_Cfd_General_Control;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_Front;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_End;
	struct STU_CFD_MS_ALG_INTERFACE_SDRIP *pstu_SDRIP_Param;
};

struct ST_CFD_GOP_PRESDR_INPUT {
	struct STU_CFD_GENERAL_CONTROL_GOP *pstu_Cfd_General_Control;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_Front;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_End;
	struct STU_CFD_MS_ALG_INTERFACE_GOP_PRESDRIP *pstu_SDRIP_Param;
	struct STU_CFDAPI_UI_CONTROL *pstu_UI_control;

};

struct ST_CFD_HDR_INPUT {
	struct STU_CFD_GENERAL_CONTROL *pstu_Cfd_General_Control;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_Front;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_End;
	struct STU_CFD_MS_ALG_INTERFACE_HDRIP *pstu_HDRIP_Param;
	struct STU_CFD_MS_ALG_INTERFACE_TMO *pstu_TMO_Param;
};

struct ST_CFD_POSTSDR_INPUT {
	struct STU_CFD_GENERAL_CONTROL *pstu_Cfd_General_Control;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_Front;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_End;
	struct STU_CFD_MS_ALG_INTERFACE_SDRIP *pstu_SDRIP_Param;
	struct STU_CFDAPI_UI_CONTROL *pstu_UI_control;
};

// winston
struct ST_CFD_420_CONTROL_INPUT {
	struct STU_CFD_GENERAL_CONTROL *pstu_Cfd_General_Control;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_Front;
	struct STU_CFD_CONTROL_POINT *pstu_Cfd_Control_Point_End;
	struct STU_CFD_MS_ALG_INTERFACE_420CONTROL *pstu_420CONTROL_Param;
};

struct ST_CFD_PreSDR_SUB_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_presdr_ipcsc_sub_hw_output;
};

struct ST_CFD_POSTSDR_SUB_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct ST_CFD_CSC_HW_OUTPUT stu_cfd_sdr_vopcsc_sub_hw_output;
};

struct ST_CFD_420_CONTROL_HW_OUTPUT {
	__u32 u32Version;
	__u16 u16Length;
	struct ST_CFD_GENERAL_HW_OUTPUT stu_cfd_420control_hw_output;
};

#undef INTERFACE
#endif		// _MDRV_CFD_IF_H_

