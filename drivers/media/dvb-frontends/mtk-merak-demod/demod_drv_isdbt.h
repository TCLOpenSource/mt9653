/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DEMOD_DRV_ISDBT_H_
#define _DEMOD_DRV_ISDBT_H_

//----------------------------------------------------------------------
//  Driver Compiler Options
//----------------------------------------------------------------------
#ifndef DVB_STI
 #define  DVB_STI    1
#endif

#if DVB_STI || !defined UTPA2
#define DMD_ISDBT_UTOPIA_EN                  1
#define DMD_ISDBT_UTOPIA2_EN                 0
#else
#define DMD_ISDBT_UTOPIA_EN                  0
#define DMD_ISDBT_UTOPIA2_EN                 1
#endif

#if DVB_STI || defined MTK_PROJECT
#define DMD_ISDBT_STR_EN                     1
#define DMD_ISDBT_MULTI_THREAD_SAFE          1
#define DMD_ISDBT_MULTI_DMD_EN               1
#define DMD_ISDBT_3PARTY_EN                  1
#define DMD_ISDBT_3PARTY_IN_KERNEL_EN        1
#define DMD_ISDBT_3PARTY_MSPI_EN             1
#else
#define DMD_ISDBT_STR_EN                     1
#define DMD_ISDBT_MULTI_THREAD_SAFE          0
#define DMD_ISDBT_MULTI_DMD_EN               1
#define DMD_ISDBT_3PARTY_EN                  0
#define DMD_ISDBT_3PARTY_IN_KERNEL_EN        0
#define DMD_ISDBT_3PARTY_MSPI_EN             0
#endif

#if DMD_ISDBT_3PARTY_EN
#undef DMD_ISDBT_UTOPIA_EN
#undef DMD_ISDBT_UTOPIA2_EN

#define DMD_ISDBT_UTOPIA_EN                  0
#define DMD_ISDBT_UTOPIA2_EN                 0

#if DMD_ISDBT_3PARTY_IN_KERNEL_EN
 #ifndef UTPA2
  #define UTPA2
 #endif
 #ifndef MSOS_TYPE_LINUX_KERNEL
  #define MSOS_TYPE_LINUX_KERNEL
 #endif
#endif // #if DMD_ISDBT_3PARTY_IN_KERNEL_EN
#endif // #if DMD_ISDBT_3PARTY_EN

//----------------------------------------------------------------------
//  Include Files
//----------------------------------------------------------------------

#if defined MTK_PROJECT//VT
 #include "PD_Def.h"
#elif DVB_STI
//ckang, #include "mtkm6_frontend.h"
#include <asm/io.h>
#include <linux/delay.h>
#include "demod_common.h"
#elif DMD_ISDBT_3PARTY_EN
// Please add header files needed by 3 perty platform PRINT and MEMCPY define
#endif

//#include "MsTypes.h"
#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
#ifndef MSIF_TAG
#include "MsVersion.h"
#endif
#include "MsCommon.h"
#endif
#if DMD_ISDBT_UTOPIA2_EN || (DMD_ISDBT_STR_EN && DMD_ISDBT_UTOPIA_EN)
#include "utopia.h"
#endif

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
#include "drvMSPI.h"
#include "drvMIU.h"
#endif

//-----------------------------------------------------------------------
//  Driver Capability
//-----------------------------------------------------------------------

//----------------------------------------------------------------------
//  Macro and Define
//----------------------------------------------------------------------

#ifndef DLL_PACKED
#define DLL_PACKED
#endif

#ifndef DLL_PUBLIC
#define DLL_PUBLIC
#endif

#if defined MTK_PROJECT
#define PRINT(fmt, ...) mcSHOW_DBG_MSG4((fmt, ##__VA_ARGS__))
#define MEMCPY(pv_to, pv_from, z_len) do { \
		x_memcpy(pv_to, pv_from, z_len);\
		HalFlushDCacheMultipleLine((UPTR)pv_to, z_len);\
	} while (0)
#elif DVB_STI
#define PRINT    printk
#define MEMCPY   memcpy
#elif DMD_ISDBT_3PARTY_EN
// Please define PRINT and MEMCPY according to 3 party platform
#define PRINT    printk
#define MEMCPY   memcpy
#else // #if DMD_ISDBT_3PARTY_EN
#ifdef MSOS_TYPE_LINUX_KERNEL
#define PRINT    printk
#define MEMCPY   memcpy
#else // #ifdef MSOS_TYPE_LINUX_KERNEL
#define PRINT    printf
#define MEMCPY   memcpy
#endif // #ifdef MSOS_TYPE_LINUX_KERNEL
#endif // #if DMD_ISDBT_3PARTY_EN

#if DMD_ISDBT_MULTI_DMD_EN
#define DMD_ISDBT_MAX_DEMOD_NUM          2
#else
#define DMD_ISDBT_MAX_DEMOD_NUM          1
#endif

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
 //Lib code
#define MSIF_DMD_ISDBT_LIB_CODE {'D', 'M', 'D', '_',     \
	'I', 'S', 'D', 'B', 'T', '_'}
//LIB version
#define MSIF_DMD_ISDBT_LIBVER            {'0', '0'}
//Build Number
#define MSIF_DMD_ISDBT_BUILDNUM       {'0', '0'}
//P4 ChangeList Number
#define MSIF_DMD_ISDBT_CHANGELIST   {'0', '0', '0', '0', '0', '0', '0', '0'}

#define DMD_ISDBT_VER {         /* Character String for DRV/API version */  \
	MSIF_TAG,                            /* 'MSIF'                    */  \
	MSIF_CLASS,                          /* '00'                      */  \
	MSIF_CUS,                            /* 0x0000                  */  \
	MSIF_MOD,                            /* 0x0000                 */  \
	MSIF_CHIP,                                                     \
	MSIF_CPU,                                                       \
	MSIF_DMD_ISDBT_LIB_CODE,             /* IP__                   */  \
	MSIF_DMD_ISDBT_LIBVER,               /* 0.0 ~ Z.Z              */  \
	MSIF_DMD_ISDBT_BUILDNUM,             /* 00 ~ 99                   */  \
	MSIF_DMD_ISDBT_CHANGELIST,           /* CL#                      */  \
	MSIF_OS }
#endif // #if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

#ifndef BIT_
#define BIT_(n)                             (1 << (n))
#endif

#define DMD_ISDBT_LOCK_FSA_TRACK_LOCK       BIT_(0)
#define DMD_ISDBT_LOCK_PSYNC_LOCK           BIT_(1)
#define DMD_ISDBT_LOCK_ICFO_CH_EXIST_LOCK   BIT_(2)
#define DMD_ISDBT_LOCK_FEC_LOCK             BIT_(3)

//-------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------

enum dmd_isdbt_hal_command {
	DMD_ISDBT_HAL_CMD_Exit = 0,
	DMD_ISDBT_HAL_CMD_InitClk,
	DMD_ISDBT_HAL_CMD_Download,
	DMD_ISDBT_HAL_CMD_FWVERSION,
	DMD_ISDBT_HAL_CMD_SoftReset,
	DMD_ISDBT_HAL_CMD_SetACICoef,
	DMD_ISDBT_HAL_CMD_SetISDBTMode,
	DMD_ISDBT_HAL_CMD_SetEWBSMode,
	DMD_ISDBT_HAL_CMD_SetModeClean,
	DMD_ISDBT_HAL_CMD_Active,
	DMD_ISDBT_HAL_CMD_Check_EWBS_Lock,
	DMD_ISDBT_HAL_CMD_Check_FEC_Lock,
	DMD_ISDBT_HAL_CMD_Check_FSA_TRACK_Lock,
	DMD_ISDBT_HAL_CMD_Check_PSYNC_Lock,
	DMD_ISDBT_HAL_CMD_Check_ICFO_CH_EXIST_Lock,
	DMD_ISDBT_HAL_CMD_GetSignalCodeRate,
	DMD_ISDBT_HAL_CMD_GetSignalGuardInterval,
	DMD_ISDBT_HAL_CMD_GetSignalTimeInterleaving,
	DMD_ISDBT_HAL_CMD_GetSignalFFTValue,
	DMD_ISDBT_HAL_CMD_GetSignalModulation,
	DMD_ISDBT_HAL_CMD_ReadIFAGC,
	DMD_ISDBT_HAL_CMD_GetFreqOffset,
	DMD_ISDBT_HAL_CMD_GetSignalQuality,
	DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerA,
	DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerB,
	DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerC,
	DMD_ISDBT_HAL_CMD_GetSignalQualityCombine,
	DMD_ISDBT_HAL_CMD_GetSNR,
	DMD_ISDBT_HAL_CMD_GetPreViterbiBer,
	DMD_ISDBT_HAL_CMD_GetPostViterbiBer,
	DMD_ISDBT_HAL_CMD_Read_PKT_ERR,
	DMD_ISDBT_HAL_CMD_TS_INTERFACE_CONFIG,
	DMD_ISDBT_HAL_CMD_IIC_Bypass_Mode,
	DMD_ISDBT_HAL_CMD_SSPI_TO_GPIO,
	DMD_ISDBT_HAL_CMD_GPIO_GET_LEVEL,
	DMD_ISDBT_HAL_CMD_GPIO_SET_LEVEL,
	DMD_ISDBT_HAL_CMD_GPIO_OUT_ENABLE,
	DMD_ISDBT_HAL_CMD_DoIQSwap,
	DMD_ISDBT_HAL_CMD_GET_REG,
	DMD_ISDBT_HAL_CMD_SET_REG
};

enum en_isdbt_layer {
	E_ISDBT_Layer_A = 0x00,
	E_ISDBT_Layer_B = 0x01,
	E_ISDBT_Layer_C = 0x02,
	E_ISDBT_Layer_INVALID,
};

enum en_isdbt_fft_val {
	E_ISDBT_FFT_2K = 0x00,  /// 2K
	E_ISDBT_FFT_4K = 0x01,  /// 4k
	E_ISDBT_FFT_8K = 0x02,  /// 8k
	E_ISDBT_FFT_INVALID,    /// invalid indicator
};

enum en_isdbt_constel_type {
	E_ISDBT_DQPSK   = 0,   /// DQPSK
	E_ISDBT_QPSK    = 1,   /// QPSK
	E_ISDBT_16QAM   = 2,   /// 16QAM
	E_ISDBT_64QAM   = 3,   /// 64QAM
	E_ISDBT_QAM_INVALID,   /// invalid indicator
};

enum en_isdbt_code_rate {
	E_ISDBT_CODERATE_1_2 = 0,   /// 1/2
	E_ISDBT_CODERATE_2_3 = 1,   /// 2/3
	E_ISDBT_CODERATE_3_4 = 2,   /// 3/4
	E_ISDBT_CODERATE_5_6 = 3,   /// 5/6
	E_ISDBT_CODERATE_7_8 = 4,   /// 7/8
	E_ISDBT_CODERATE_INVALID,   /// invalid indicator
};

enum en_isdbt_guard_interval {
	E_ISDBT_GUARD_INTERVAL_1_4  = 0,   /// 1/4
	E_ISDBT_GUARD_INTERVAL_1_8  = 1,   /// 1/8
	E_ISDBT_GUARD_INTERVAL_1_16 = 2,   /// 1/16
	E_ISDBT_GUARD_INTERVAL_1_32 = 3,   /// 1/32
	E_ISDBT_GUARD_INTERVAL_INVALID,    /// invalid indicator
};

enum en_isdbt_time_interleaving {
	// 2K mode
	E_ISDBT_2K_TDI_0 = 0,   /// Tdi = 0
	E_ISDBT_2K_TDI_4 = 1,   /// Tdi = 4
	E_ISDBT_2K_TDI_8 = 2,   /// Tdi = 8
	E_ISDBT_2K_TDI_16 = 3,  /// Tdi = 16
	// 4K mode
	E_ISDBT_4K_TDI_0 = 4,   /// Tdi = 0
	E_ISDBT_4K_TDI_2 = 5,   /// Tdi = 2
	E_ISDBT_4K_TDI_4 = 6,   /// Tdi = 4
	E_ISDBT_4K_TDI_8 = 7,   /// Tdi = 8
	// 8K mode
	E_ISDBT_8K_TDI_0 = 8,   /// Tdi = 0
	E_ISDBT_8K_TDI_1 = 9,   /// Tdi = 1
	E_ISDBT_8K_TDI_2 = 10,  /// Tdi = 2
	E_ISDBT_8K_TDI_4 = 11,  /// Tdi = 4
	E_ISDBT_TDI_INVALID,    /// invalid indicator
};

struct DLL_PACKED isdbt_modulation_mode {
	enum en_isdbt_code_rate         eisdbtcoderate;
	enum en_isdbt_guard_interval    eisdbtgi;
	enum en_isdbt_time_interleaving eisdbttdi;
	enum en_isdbt_fft_val               eisdbtfft;
	enum en_isdbt_constel_type      eisdbtconstellation;
};

struct DLL_PACKED dmd_isdbt_get_modulation {
	enum en_isdbt_layer        eisdbtlayer;
	enum en_isdbt_constel_type econstellation;
};

struct DLL_PACKED dmd_isdbt_get_code_rate {
	enum en_isdbt_layer     eisdbtlayer;
	enum en_isdbt_code_rate ecoderate;
};

struct DLL_PACKED dmd_isdbt_get_time_interleaving {
	enum en_isdbt_layer             eisdbtlayer;
	enum en_isdbt_time_interleaving etimeinterleaving;
};

struct DLL_PACKED dmd_isdbt_get_ber_value {
	enum en_isdbt_layer eisdbtlayer;
	#ifdef UTPA2
	u32 bervalue;
	u16 berperiod;
	#else
	float  fbervalue;
	#endif
};

struct DLL_PACKED dmd_isdbt_get_pkt_err {
	enum en_isdbt_layer eisdbtlayer;
	u16 packeterr;
};

struct DLL_PACKED dmd_isdbt_cfo_data {
	u8   fft_mode;
	s32 tdcfo_regvalue;
	s32 fdcfo_regvalue;
	s16 icfo_regvalue;
};

struct DLL_PACKED dmd_isdbt_snr_data {
	u32 reg_snr;
	u16 reg_snr_obs_num;
};

struct DLL_PACKED dmd_isdbt_gpio_pin_data {
	u8 pin;
	union {
		u8 level;
		u8 is_out;
	};
};

struct DLL_PACKED dmd_isdbt_reg_data {
	u16 isdbt_addr;
	u8  isdbt_data;
};

enum dmd_isdbt_dbglv {
	DMD_ISDBT_DBGLV_NONE,    // disable all the debug message
	DMD_ISDBT_DBGLV_INFO,    // information
	DMD_ISDBT_DBGLV_NOTICE,  // normal but significant condition
	DMD_ISDBT_DBGLV_WARNING, // warning conditions
	DMD_ISDBT_DBGLV_ERR,     // error conditions
	DMD_ISDBT_DBGLV_CRIT,    // critical conditions
	DMD_ISDBT_DBGLV_ALERT,   // action must be taken immediately
	DMD_ISDBT_DBGLV_EMERG,   // system is unusable
	DMD_ISDBT_DBGLV_DEBUG,   // debug-level messages
};

enum dmd_isdbt_demod_type {
	DMD_ISDBT_DEMOD,
	DMD_ISDBT_DEMOD_6M = DMD_ISDBT_DEMOD,
	DMD_ISDBT_DEMOD_7M,
	DMD_ISDBT_DEMOD_8M,
	DMD_ISDBT_DEMOD_6M_EWBS,
	DMD_ISDBT_DEMOD_7M_EWBS,
	DMD_ISDBT_DEMOD_8M_EWBS,
	DMD_ISDBT_DEMOD_MAX,
	DMD_ISDBT_DEMOD_NULL = DMD_ISDBT_DEMOD_MAX,
};

enum dmd_isdbt_getlock_type {
	DMD_ISDBT_GETLOCK,
	DMD_ISDBT_GETLOCK_FSA_TRACK_LOCK,
	DMD_ISDBT_GETLOCK_PSYNC_LOCK,
	DMD_ISDBT_GETLOCK_ICFO_CH_EXIST_LOCK,
	DMD_ISDBT_GETLOCK_FEC_LOCK,
	DMD_ISDBT_GETLOCK_EWBS_LOCK
};

enum dmd_isdbt_lock_status {
	DMD_ISDBT_LOCK,
	DMD_ISDBT_CHECKING,
	DMD_ISDBT_CHECKEND,
	DMD_ISDBT_UNLOCK,
	DMD_ISDBT_NULL,
};

#if (DMD_ISDBT_STR_EN && !DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN)
//ckang, redefine
//typedef enum {
	//E_POWER_SUSPEND     = 1,
	// State is backup in DRAM, components power off.
	//E_POWER_RESUME      = 2,
	// Resume from Suspend or Hibernate mode
	//E_POWER_MECHANICAL  = 3,
	// Full power off mode.
	//System return to working state only after full reboot
	//E_POWER_SOFT_OFF    = 4,
	// The system appears to be off,
	//but some components remain powered for trigging boot-up.
//} EN_POWER_MODE;

#endif

/// For demod init
struct DLL_PACKED dmd_isdbt_initdata {
	// init
	u16 isdbt_icfo_ch_exist_check_time;
	u16 isdbt_fec_lock_check_time;

	// register init
	u8 *dmd_isdbt_dsp_reg_init_ext; // TODO use system variable type
	u8 dmd_isdbt_dsp_reg_init_size;
	u8 *dmd_isdbt_init_ext; // TODO use system variable type

	//By Tuners:
	u16  if_khz;//By Tuners
	u8 iq_swap;//0
	u16  agc_reference_value;//0
	u8 tuner_gain_invert;//0

	//By IC:
	u8 is_ext_demod;//0

	//By TS (Only for MCP or ext demod):
	u8 ts_config_byte_serial_mode : 1;
	u8 ts_config_byte_data_swap   : 1;
	u8 ts_config_byte_clock_inv   : 1;
	u8 ts_config_byte_div_num     : 5;

	//By SYS I2C (Only for MCP or ext demod):
	u8 i2c_slave_addr;
	u8 i2c_slave_bus;
	u8 (*i2c_writebytes)(u16 BusNumSlaveID, u8 addrcount,
		u8 *paddr, u16 size, u8 *pdata);
	u8 (*i2c_readbytes)(u16 BusNumSlaveID, u8 addrnum,
		u8 *paddr, u16 size, u8 *pdata);

	//By SYS MSPI (Only for MCP or ext demod):
	u8 is_use_sspi_load_code;
	u8 is_sspi_use_ts_pin;

	//By SYS memory mapping (Only for int demod):
	u64 tdi_start_addr;

	//By SYS EWBS mode setting (Only for EWBS):
	u8 ewbs_mode;

	//By SYS area code setting (Only for EWBS):
	u16 area_code_addr_set0;
	u16 area_code_addr_set1;
	u16 area_code_addr_set2;
	u16 area_code_addr_set3;

	#if DVB_STI
	u8 *virt_dram_base_addr;
	u8 *virt_riu_base_addr;
	u8 *virt_reset_base_addr;
	u8 *virt_ts_base_addr;
	u8 *virt_clk_base_addr;
	u8 *virt_sram_base_addr;
	u8 *virt_pmux_base_addr;
	u8 *virt_vagc_base_addr;
	#endif

	//By SYS:
	u16 ip_version;
};

//typedef struct ISDBT_IOCTL_DATA DMD_ISDBT_IOCTL_DATA;

struct DLL_PACKED dmd_isdbt_info {
	u8  isdbt_version;
	u32 isdbt_scan_time_start;
	u32 isdbt_fec_lock_time;
	u32 isdbt_lock_status;
};

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
struct DLL_PACKED dmd_isdbt_sys_ptr {

	void (*delayms)(u32 ms);

	MSPI_ErrorNo(*mspi_init_ext)(u8 HWNum);

	void (*mspi_slave_enable)(u8 Enable);

	MSPI_ErrorNo(*mspi_write)(u8 *pData, u16 Size);
	MSPI_ErrorNo(*mspi_read)(u8 *pData, u16 Size);
	u8 (*miu_is_support_miu1)(void);
	u8 (*miu_sel_miu)(eMIUClientID eClientID, eMIU_SelType eType);
	void (*miu_mask_req)(u8 Miu, eMIUClientID eClientID);
	void (*miu_unmask_req)(u8 Miu, eMIUClientID eClientID);
};
#else
struct DLL_PACKED dmd_isdbt_sys_ptr {
	u32  (*get_system_time_ms)(void);       // Get sys time (unit: ms)
	void    (*delayms)(u32 ms);          // Delay time (unit: ms)
	u8 (*create_mutex)(u8 Enable); // Create&Delete mutex
	void    (*lock_dmd)(u8 Enable);     // Enter&Leave mutex

	u32 (*mspi_init_ext)(u8 HWNum);
	void (*mspi_slave_enable)(u8 Enable);
	u32 (*mspi_write)(u8 *pData, u16 Size);
	u32 (*mspi_read)(u8 *pData, u16 Size);
};
#endif

//struct DLL_PACKED ISDBT_IOCTL_DATA {
	//u8 id;
	//struct DMD_ISDBT_ResData *pRes;
	//struct DMD_ISDBT_SYS_PTR sys;
//};

struct DLL_PACKED dmd_isdbt_ioctl_data {
	u8 id;
	struct dmd_isdbt_resdata *pres;
	struct dmd_isdbt_sys_ptr sys;
};

struct DLL_PACKED dmd_isdbt_pridata {
	#if DVB_STI
	u8 *virt_dmd_base_addr;
	#else
	size_t virt_dmd_base_addr;
	#endif

	u8 binit;
	u8 bdownloaded;

	#if DMD_ISDBT_STR_EN
	u8              bis_dtv;
	enum EN_POWER_MODE        elast_state;
	#endif
	enum dmd_isdbt_demod_type elast_type;

	u8 bis_qpad;

	u32 tdi_dram_size;

	#if DVB_STI
	u8 *virt_reset_base_addr;
	u8 *virt_ts_base_addr;
	u8 *virt_clk_base_addr;
	u8 *virt_sram_base_addr;
	u8 *virt_pmux_base_addr;
	u8 *virt_vagc_base_addr;
	#endif

	u8 (*hal_dmd_isdbt_ioctl_cmd)(struct dmd_isdbt_ioctl_data *pdata,
		enum dmd_isdbt_hal_command ecmd, void *para);
};

struct DLL_PACKED dmd_isdbt_resdata {
	struct dmd_isdbt_initdata sdmd_isdbt_initdata;
	struct dmd_isdbt_pridata sdmd_isdbt_pridata;
	struct dmd_isdbt_info sdmd_isdbt_info;
};

//------------------------------------------------------------------
//  Function and Variable
//------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif

//-------------------------------------------------------------------
/// Set detailed level of ISDBT driver debug message
/// u8DbgLevel : debug level for Parallel Flash driver\n
/// AVD_DBGLV_NONE,    ///< disable all the debug message\n
/// AVD_DBGLV_INFO,    ///< information\n
/// AVD_DBGLV_NOTICE,  ///< normal but significant condition\n
/// AVD_DBGLV_WARNING, ///< warning conditions\n
/// AVD_DBGLV_ERR,     ///< error conditions\n
/// AVD_DBGLV_CRIT,    ///< critical conditions\n
/// AVD_DBGLV_ALERT,   ///< action must be taken immediately\n
/// AVD_DBGLV_EMERG,   ///< system is unusable\n
/// AVD_DBGLV_DEBUG,   ///< debug-level messages\n
/// @return TRUE : succeed
/// @return FALSE : failed to set the debug level
//-------------------------------------------------------------------
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_setdbglevel(enum dmd_isdbt_dbglv dbglv);
//------------------------------------------------------------------
/// Get the information of ISDBT driver\n
/// @return the pointer to the driver information
//-------------------------------------------------------------------
DLL_PUBLIC extern struct dmd_isdbt_info *mdrv_dmd_isdbt_getinfo(void);
//-------------------------------------------------------------------
/// Get ISDBT driver version
/// when get ok, return the pointer to the driver version
//-------------------------------------------------------------------
//DLL_PUBLIC extern u8
//mdrv_dmd_isdbt_getlibver(const MSIF_Version **pversion);

/////////////////////////////////////////////////////////////////
/// Should be called once when power on init
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_initial_hal_interface(void);

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

/////////////////////////////////////////////////////////////////
///                            SINGLE DEMOD API
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
/// Should be called every time when enter DTV input source
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_init(struct dmd_isdbt_initdata *pdmd_isdbt_initdata,
u32 init_data_len);
/////////////////////////////////////////////////////////////////
/// Should be called every time when exit DTV input source
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_exit(void);
/////////////////////////////////////////////////////////////////
/// Get Initial Data
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u32
mdrv_dmd_isdbt_getconfig(struct dmd_isdbt_initdata *psdmd_isdbt_initdata);
/////////////////////////////////////////////////////////////////
/// Get demod FW version (no use)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_getfwver(u16 *fw_ver);
/////////////////////////////////////////////////////////////////
/// Reset demod (no use)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern void mdrv_dmd_isdbt_setreset(void);
/////////////////////////////////////////////////////////////////
/// Set demod mode and enable demod
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_advsetconfig(enum dmd_isdbt_demod_type etype,
u8 benable);
/////////////////////////////////////////////////////////////////
/// Set demod mode and enable demod
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_setconfig(u8 benable);
/////////////////////////////////////////////////////////////////
/// Active demod (not used)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_setactive(u8 benable);
/////////////////////////////////////////////////////////////////
/// Set demod power state for STR
/////////////////////////////////////////////////////////////////
#if DMD_ISDBT_STR_EN
DLL_PUBLIC extern u32
mdrv_dmd_isdbt_setpowerstate(enum EN_POWER_MODE power_state);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod lock status
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern enum dmd_isdbt_lock_status
mdrv_dmd_isdbt_getlock(enum dmd_isdbt_getlock_type etype);
/////////////////////////////////////////////////////////////////
/// Get demod modulation mode
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_get_modulation_mode(enum en_isdbt_layer elayer_index,
struct isdbt_modulation_mode *sisdbt_modulation_mode);
/////////////////////////////////////////////////////////////////
/// Get demod signal strength (IF AGC gain)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_get_signalstrength(
u16 *strength);
/////////////////////////////////////////////////////////////////
/// Get demod frequency offset
/////////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_getfreqoffset(float *freq_offset);
#else
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_getfreqoffset(struct dmd_isdbt_cfo_data *freq_offset);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod signal quality (post Viterbi BER)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_get_signalquality(void);
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_get_signalqualityoflayerA(void);
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_get_signalqualityoflayerB(void);
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_get_signalqualityoflayerC(void);
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_get_signalqualitycombine(void);
/////////////////////////////////////////////////////////////////
/// Get demod SNR
/////////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_getsnr(float *f_snr);
#else
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_getsnr(
struct dmd_isdbt_snr_data *f_snr);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod pre Viterbi BER
/////////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_get_previterbi_ber(enum en_isdbt_layer elayer_index,
float *fber);
#else
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_get_previterbi_ber(enum en_isdbt_layer elayer_index,
struct dmd_isdbt_get_ber_value *fber);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod post Viterbi BER
/////////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_get_postviterbi_ber(enum en_isdbt_layer elayer_index,
float *fber);
#else
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_get_postviterbi_ber(enum en_isdbt_layer elayer_index,
struct dmd_isdbt_get_ber_value *fber);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod packet error
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8
mdrv_dmd_isdbt_read_pkt_err(enum en_isdbt_layer elayer_index,
u16 *packet_err);
/////////////////////////////////////////////////////////////////
/// Set TS output mode (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_set_serialcontrol(
u8 ts_config_data);
/////////////////////////////////////////////////////////////////
/// Enable I2C bypass mode (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_iic_bypass_mode(u8 benable);
/////////////////////////////////////////////////////////////////
/// Switch pin to SSPI or GPIO (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_switch_sspi_gpio(u8 benable);
/////////////////////////////////////////////////////////////////
/// Get GPIO pin high or low (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_gpio_get_level(u8 pin,
u8 *blevel);
/////////////////////////////////////////////////////////////////
/// Set GPIO pin high or low (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_gpio_set_level(u8 pin,
u8 blevel);
/////////////////////////////////////////////////////////////////
/// Set GPIO pin output or input (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_gpio_out_enable(u8 pin,
u8 enable_out);
/////////////////////////////////////////////////////////////////
/// Swap ADC input (usually for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_do_iq_swap(u8 is_qpad);

/////////////////////////////////////////////////////////////////
/// To get ISDBT's register  value, only for special purpose.\n
/// u16Addr       : the address of ISDBT's register\n
/// return the value of AFEC's register\n
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_getreg(u16 addr,
u8 *pdata);
/////////////////////////////////////////////////////////////////
/// To set ISDBT's register value, only for special purpose.\n
/// u16Addr       : the address of ISDBT's register\n
/// u8Value       : the value to be set\n
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_setreg(u16 addr,
u8 data);

#endif // #if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

/////////////////////////////////////////////////////////////////
///                            MULTI DEMOD API                              ///
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
/// Should be called every time when enter DTV input source
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_init(u8 id,
struct dmd_isdbt_initdata *pdmd_isdbt_initdata, u32 init_data_len);
/////////////////////////////////////////////////////////////////
/// Should be called every time when exit DTV input source
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_exit(u8 id);
/////////////////////////////////////////////////////////////////
/// Get Initial Data
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u32 mdrv_dmd_isdbt_md_getconfig(u8 id,
struct dmd_isdbt_initdata *psdmd_isdbt_initdata);
/////////////////////////////////////////////////////////////////
/// Get demod FW version (no use)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_get_fw_ver(u8 id,
u16 *fw_ver);
/////////////////////////////////////////////////////////////////
/// Reset demod (no use)
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
/// Set demod mode and enable demod
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_advsetconfig(u8 id,
enum dmd_isdbt_demod_type etype, u8 benable);
////////////////////////////////////////////////////////////////
/// Set demod mode and enable demod
///////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_setconfig(u8 id,
u8 benable);
/////////////////////////////////////////////////////////////////
/// Active demod (not used)
//////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/// Set demod power state for STR
/////////////////////////////////////////////////////////////////
#if DMD_ISDBT_STR_EN
DLL_PUBLIC extern u32 mdrv_dmd_isdbt_md_setpowerstate(u8 id,
enum EN_POWER_MODE power_state);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod lock status
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern enum dmd_isdbt_lock_status mdrv_dmd_isdbt_md_getlock(
u8 id, enum dmd_isdbt_getlock_type etype);
/////////////////////////////////////////////////////////////////
/// Get demod modulation mode
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_get_modulation_mode(u8 id,
enum en_isdbt_layer elayer_index,
struct isdbt_modulation_mode *sisdbt_modulation_mode);
/////////////////////////////////////////////////////////////////
/// Get demod signal strength (IF AGC gain)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_get_signalstrength(u8 id,
u16 *strength);
/////////////////////////////////////////////////////////////////
/// Get demod frequency offset
//////////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_getfreqoffset(u8 id,
float *freq_offset);
#else
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_getfreqoffset(u8 id,
struct dmd_isdbt_cfo_data *freq_offset);
#endif
///////////////////////////////////////////////////////////////////
/// Get signal quality (post Viterbi BER)
///////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_md_get_signalquality(u8 id);
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerA(u8 id);
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerB(u8 id);
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerC(u8 id);
DLL_PUBLIC extern u16 mdrv_dmd_isdbt_md_get_signalqualitycombine(u8 id);
/////////////////////////////////////////////////////////////////
/// Get demod SNR
/////////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_getsnr(u8 id,
float *f_snr);
#else
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_getsnr(u8 id,
struct dmd_isdbt_snr_data *f_snr);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod pre Viterbi BER
/////////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_get_previterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, float *fber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_get_previterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, struct dmd_isdbt_get_ber_value *fber);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod post Viterbi BER
/////////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_get_postviterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, float *fber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_get_postviterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, struct dmd_isdbt_get_ber_value *fber);
#endif
/////////////////////////////////////////////////////////////////
/// Get demod packet error
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_read_pkt_err(u8 id,
enum en_isdbt_layer elayer_index, u16 *packet_err);
/////////////////////////////////////////////////////////////////
/// Set TS output mode (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_set_serialcontrol(u8 id,
u8 ts_config_data);
/////////////////////////////////////////////////////////////////
/// Enable I2C bypass mode (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_iic_bypass_mode(u8 id,
u8 benable);
/////////////////////////////////////////////////////////////////
/// Switch pin to SSPI or GPIO (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_switch_sspi_gpio(u8 id,
u8 benable);
/////////////////////////////////////////////////////////////////
/// Get GPIO pin high or low (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_gpio_get_level(u8 id,
u8 pin, u8 *blevel);
/////////////////////////////////////////////////////////////////
/// Set GPIO pin high or low (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_gpio_set_level(u8 id,
u8 pin, u8 blevel);
/////////////////////////////////////////////////////////////////
/// Set GPIO pin output or input (only for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_gpio_out_enable(u8 id,
u8 pin, u8 enable_out);
/////////////////////////////////////////////////////////////////
/// Swap ADC input (usually for external demod)
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_do_iq_swap(u8 id,
u8 bis_qpad);

/////////////////////////////////////////////////////////////////
/// To get ISDBT's register  value, only for special purpose.\n
/// u16Addr       : the address of ISDBT's register\n
/// return the value of AFEC's register\n
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_getreg(u8 id,
u16 addr, u8 *pdata);
/////////////////////////////////////////////////////////////////
/// To set ISDBT's register value, only for special purpose.\n
/// u16Addr       : the address of ISDBT's register\n
/// u8Value       : the value to be set\n
/////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_isdbt_md_setreg(u8 id,
u16 addr, u8 data);

#ifdef UTPA2

DLL_PUBLIC extern struct dmd_isdbt_info *_mdrv_dmd_isdbt_getinfo(void);
//DLL_PUBLIC extern u8
//_mdrv_dmd_isdbt_getlibver(const MSIF_Version **pversion);
DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_init(u8 id,
struct dmd_isdbt_initdata *pdmd_isdbt_initdata, u32 init_data_len);
DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_exit(u8 id);

DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_advsetconfig(u8 id,
enum dmd_isdbt_demod_type etype, u8 benable);
#if DMD_ISDBT_STR_EN
DLL_PUBLIC extern u32 _mdrv_dmd_isdbt_md_setpowerstate(u8 id,
enum EN_POWER_MODE power_state);
#endif
DLL_PUBLIC extern enum dmd_isdbt_lock_status _mdrv_dmd_isdbt_md_getlock(
u8 id, enum dmd_isdbt_getlock_type etype);
DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_get_modulation_mode(u8 id,
enum en_isdbt_layer elayer_index,
struct isdbt_modulation_mode *sisdbt_modulation_mode);

DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_getfreqoffset(u8 id,
struct dmd_isdbt_cfo_data *cfo);

DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_getsnr(u8 id,
struct dmd_isdbt_snr_data *f_snr);

DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_get_postviterbi_ber(u8 id,
struct dmd_isdbt_get_ber_value *ber);
DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_read_pkt_err(u8 id,
enum en_isdbt_layer elayer_index, u16 *packet_err);

DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_getreg(u8 id, u16 addr,
u8 *pdata);
DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_setreg(u8 id, u16 addr,
u8 data);
DLL_PUBLIC extern u8 _mdrv_dmd_isdbt_md_get_information(u8 id,
struct dvb_frontend *fe, struct dtv_frontend_properties *p);

DLL_PUBLIC extern ssize_t isdbt_get_information_show(
struct device_driver *driver, char *buf);

DLL_PUBLIC extern ssize_t isdbt_get_information(char *buf);
#endif

#ifdef __cplusplus
}
#endif

#endif // _DEMOD_DRV_ISDBT_H_
