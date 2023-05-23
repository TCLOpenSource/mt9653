/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef	_DRV_ATSC_H_
#define	_DRV_ATSC_H_

//--------------------------------------------------
//	Driver Compiler	Options
//--------------------------------------------------
#ifndef	DVB_STI	// VT: Temp	define for DVB_Frontend
 #define	DVB_STI	 1
#endif

#if DVB_STI || defined UTPA2
#define	DMD_ATSC_UTOPIA_EN					1
#define	DMD_ATSC_UTOPIA2_EN					0
#else
#define	DMD_ATSC_UTOPIA_EN					0
#define	DMD_ATSC_UTOPIA2_EN					1
#endif

#if	DVB_STI	|| defined MTK_PROJECT
#define	DMD_ATSC_STR_EN						1
#define	DMD_ATSC_MULTI_THREAD_SAFE			1
#define	DMD_ATSC_MULTI_DMD_EN				1
#define	DMD_ATSC_3PARTY_EN					1
#define	DMD_ATSC_3PARTY_IN_KERNEL_EN		1
#define	DMD_ATSC_3PARTY_MSPI_EN				1
#else
#define	DMD_ATSC_STR_EN						1
#define	DMD_ATSC_MULTI_THREAD_SAFE			0
#define	DMD_ATSC_MULTI_DMD_EN				1
#define	DMD_ATSC_3PARTY_EN					0
#define	DMD_ATSC_3PARTY_IN_KERNEL_EN		0
#define	DMD_ATSC_3PARTY_MSPI_EN				0
#endif

#if	DMD_ATSC_3PARTY_EN
#undef DMD_ATSC_UTOPIA_EN
#undef DMD_ATSC_UTOPIA2_EN

#define	DMD_ATSC_UTOPIA_EN					0
#define	DMD_ATSC_UTOPIA2_EN					0

#if	DMD_ATSC_3PARTY_IN_KERNEL_EN
 #ifndef UTPA2
	#define	UTPA2
 #endif
 #ifndef MSOS_TYPE_LINUX_KERNEL
	#define	MSOS_TYPE_LINUX_KERNEL
 #endif
#endif //	#if	DMD_ATSC_3PARTY_IN_KERNEL_EN
#endif //	#if	DMD_ATSC_3PARTY_EN

//------------------------------------------------------
//	Include	Files
//------------------------------------------------------

#if	defined	MTK_PROJECT
#include "PD_Def.h"
#include "x_hal_arm.h"
#elif	DVB_STI
//ckang, #include "mtkm6_frontend.h"
#include <asm/io.h>
#include <linux/delay.h>
#include "m7332_demod_common.h"
#elif	DMD_ATSC_3PARTY_EN
// Please add header files needed by 3 perty platform PRINT and MEMCPY define
#endif

//#include "MsTypes.h"
#if	DMD_ATSC_UTOPIA_EN ||	DMD_ATSC_UTOPIA2_EN
#ifndef	MSIF_TAG
#include "MsVersion.h"
#endif
#include "MsCommon.h"
#endif
#if	 DMD_ATSC_UTOPIA2_EN ||	(DMD_ATSC_STR_EN &&	DMD_ATSC_UTOPIA_EN)
#include "utopia.h"
#endif
#if	DMD_ATSC_UTOPIA_EN ||	DMD_ATSC_UTOPIA2_EN
#include "drvMSPI.h"
#endif

//----------------------------------------------------
//	Driver Capability
//----------------------------------------------------


//----------------------------------------------------
//	Macro	and	Define
//----------------------------------------------------

#ifndef	DLL_PACKED
#define	DLL_PACKED
#endif

#ifndef	DLL_PUBLIC
#define	DLL_PUBLIC
#endif

#if defined MTK_PROJECT
#define PRINT(fmt, ...) mcSHOW_DBG_MSG4((fmt, ##__VA_ARGS__))
#define MEMCPY(pv_to,	pv_from, z_len) do { x_memcpy(pv_to, pv_from, z_len);\
HalFlushDCacheMultipleLine(pv_to, z_len); } \
while (0)
#elif DVB_STI
#define PRINT  printk
#define MEMCPY  memcpy
#elif DMD_ATSC_3PARTY_EN
// Please define PRINT and MEMCPY according to 3 perty platform
#define	PRINT	 printk
#define	MEMCPY	 memcpy
#else	// #if DMD_ATSC_3PARTY_EN
#ifdef MSOS_TYPE_LINUX_KERNEL
#define	PRINT	 printk
#define	MEMCPY	 memcpy
#else	// #ifdef	MSOS_TYPE_LINUX_KERNEL
#define	PRINT	 printf
#define	MEMCPY	 memcpy
#endif //	#ifdef MSOS_TYPE_LINUX_KERNEL
#endif //	#if	DMD_ATSC_3PARTY_EN

#if	DMD_ATSC_MULTI_DMD_EN
#define	DMD_ATSC_MAX_DEMOD_NUM			8
#define	DMD_ATSC_MAX_EXT_CHIP_NUM		2
#else
#define	DMD_ATSC_MAX_DEMOD_NUM			1
#define	DMD_ATSC_MAX_EXT_CHIP_NUM		1
#endif

#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
#define MSIF_DMD_ATSC_LIB_CODE    {'D', 'M', 'D', '_', 'A', 'T', 'S', 'C', '_'}
#define MSIF_DMD_ATSC_LIBVER      {'0', '0'} //LIB version
#define MSIF_DMD_ATSC_BUILDNUM    {'0', '0' } //Build Number
#define MSIF_DMD_ATSC_CHANGELIST  {'0', '0', '0', '0', '0', '0', '0', '0'}

#define	DMD_ATSC_VER {	/* Character String for DRV/API version */\
	MSIF_TAG,				/* 'MSIF'*/	\
	MSIF_CLASS,				/* '00'	*/	\
	MSIF_CUS,				/* 0x0000*/	\
	MSIF_MOD,				/* 0x0000*/	\
	MSIF_CHIP,					        \
	MSIF_CPU,					        \
	MSIF_DMD_ATSC_LIB_CODE,    /* IP__*/\
	MSIF_DMD_ATSC_LIBVER,  /*0.0 ~ Z.Z*/\
	MSIF_DMD_ATSC_BUILDNUM,   /* 00~99*/\
	MSIF_DMD_ATSC_CHANGELIST,   /* CL#*/\
	MSIF_OS}
#endif //	#if	DMD_ATSC_UTOPIA_EN ||	DMD_ATSC_UTOPIA2_EN

#ifndef	BIT_
#define	BIT_(n) (1 << (n))
#endif

#define	DMD_ATSC_LOCK_VSB_PRE_LOCK				BIT_(0)
#define	DMD_ATSC_LOCK_VSB_FSYNC_LOCK			BIT_(1)
#define	DMD_ATSC_LOCK_VSB_CE_LOCK				BIT_(2)
#define	DMD_ATSC_LOCK_VSB_FEC_LOCK				BIT_(3)

#define	DMD_ATSC_LOCK_QAM_AGC_LOCK				BIT_(8)
#define	DMD_ATSC_LOCK_QAM_PRE_LOCK				BIT_(9)
#define	DMD_ATSC_LOCK_QAM_MAIN_LOCK				BIT_(10)

//--------------------------------------------------------
//	Type and Structure
//--------------------------------------------------------

enum DMD_ATSC_HAL_COMMAND {
	DMD_ATSC_HAL_CMD_EXIT	=	0,
	DMD_ATSC_HAL_CMD_INITCLK,
	DMD_ATSC_HAL_CMD_DOWNLOAD,
	DMD_ATSC_HAL_CMD_FWVERSION,
	DMD_ATSC_HAL_CMD_SOFTRESET,
	DMD_ATSC_HAL_CMD_SETVSBMODE,
	DMD_ATSC_HAL_CMD_SET64QAMMODE,
	DMD_ATSC_HAL_CMD_SET256QAMMODE,
	DMD_ATSC_HAL_CMD_SETMODECLEAN,
	DMD_ATSC_HAL_CMD_SET_QAM_SR,
	DMD_ATSC_HAL_CMD_ACTIVE,
	DMD_ATSC_HAL_CMD_CHECK8VSB64_256QAM,
	DMD_ATSC_HAL_CMD_AGCLOCK,
	DMD_ATSC_HAL_CMD_VSB_PRELOCK,
	DMD_ATSC_HAL_CMD_VSB_FSYNC_LOCK,
	DMD_ATSC_HAL_CMD_VSB_CE_LOCK,
	DMD_ATSC_HAL_CMD_VSB_FEC_LOCK,
	DMD_ATSC_HAL_CMD_QAM_PRELOCK,
	DMD_ATSC_HAL_CMD_QAM_MAIN_LOCK,
	DMD_ATSC_HAL_CMD_READIFAGC,
	DMD_ATSC_HAL_CMD_CHECKSIGNALCONDITION,
	DMD_ATSC_HAL_CMD_READSNRPERCENTAGE,
	DMD_ATSC_HAL_CMD_GET_QAM_SNR,
	DMD_ATSC_HAL_CMD_READPKTERR,
	DMD_ATSC_HAL_CMD_GETPREVITERBIBER,
	DMD_ATSC_HAL_CMD_GETPOSTVITERBIBER,
	DMD_ATSC_HAL_CMD_READFREQUENCYOFFSET,
	DMD_ATSC_HAL_CMD_TS_INTERFACE_CONFIG,
	DMD_ATSC_HAL_CMD_IIC_BYPASS_MODE,
	DMD_ATSC_HAL_CMD_SSPI_TO_GPIO,
	DMD_ATSC_HAL_CMD_GPIO_GET_LEVEL,
	DMD_ATSC_HAL_CMD_GPIO_SET_LEVEL,
	DMD_ATSC_HAL_CMD_GPIO_OUT_ENABLE,
	DMD_ATSC_HAL_CMD_DOIQSWAP,
	DMD_ATSC_HAL_CMD_GET_REG,
	DMD_ATSC_HAL_CMD_SET_REG,
	DMD_ATSC_HAL_CMD_SET_DIGRF_FREQ
};

struct DLL_PACKED DMD_ATSC_CFO_DATA {
	u8  mode;
	s16 ff;
	s16 rate;
};

struct DLL_PACKED DMD_ATSC_BER_DATA {
	u32 biterr;
	u16 error_window;
	u32 win_unit;
};

struct DLL_PACKED DMD_ATSC_SNR_DATA {
	u16 noisepower;
	u32 sym_num;
};

struct DLL_PACKED DMD_ATSC_GPIO_PIN_DATA {
	u8	pin_8;
	union {
		u8	blevel;
		u8	bisout;
	};
};

struct DLL_PACKED DMD_ATSC_REG_DATA {
	u16 addr_16;
	u8  data_8;
};

enum DMD_ATSC_DBGLV {
	DMD_ATSC_DBGLV_NONE,	// disable all the debug message
	DMD_ATSC_DBGLV_INFO,	// information
	DMD_ATSC_DBGLV_NOTICE,	// normal but significant condition
	DMD_ATSC_DBGLV_WARNING,	// warning conditions
	DMD_ATSC_DBGLV_ERR,		// error conditions
	DMD_ATSC_DBGLV_CRIT,	// critical conditions
	DMD_ATSC_DBGLV_ALERT,	// action must be taken immediately
	DMD_ATSC_DBGLV_EMERG,	// system is unusable
	DMD_ATSC_DBGLV_DEBUG	// debug-level messages
};

enum DMD_ATSC_DEMOD_TYPE {
	DMD_ATSC_DEMOD_ATSC_VSB,
	DMD_ATSC_DEMOD_ATSC_64QAM,
	DMD_ATSC_DEMOD_ATSC_256QAM,
	DMD_ATSC_DEMOD_ATSC_16QAM,
	DMD_ATSC_DEMOD_ATSC_32QAM,
	DMD_ATSC_DEMOD_ATSC_128QAM,
	DMD_ATSC_DEMOD_MAX,
	DMD_ATSC_DEMOD_NULL = DMD_ATSC_DEMOD_MAX
};

enum DMD_ATSC_SIGNAL_CONDITION {
	DMD_ATSC_SIGNAL_NO = 0, /*little or	no input power detected */
	DMD_ATSC_SIGNAL_WEAK = 1, /*some power detected. */
	DMD_ATSC_SIGNAL_MODERATE = 2,
	DMD_ATSC_SIGNAL_STRONG = 4,
	DMD_ATSC_SIGNAL_VERY_STRONG = 8
};

enum DMD_ATSC_GETLOCK_TYPE {
	DMD_ATSC_GETLOCK,
	DMD_ATSC_GETLOCK_VSB_AGCLOCK,
	DMD_ATSC_GETLOCK_VSB_PRELOCK,	// pilot lock
	DMD_ATSC_GETLOCK_VSB_FSYNCLOCK,
	DMD_ATSC_GETLOCK_VSB_CELOCK,
	DMD_ATSC_GETLOCK_VSB_FECLOCK,
	DMD_ATSC_GETLOCK_QAM_AGCLOCK,
	DMD_ATSC_GETLOCK_QAM_PRELOCK,	// TR	lock
	DMD_ATSC_GETLOCK_QAM_MAINLOCK
};

enum DMD_ATSC_LOCK_STATUS {
	DMD_ATSC_LOCK,
	DMD_ATSC_CHECKING,
	DMD_ATSC_CHECKEND,
	DMD_ATSC_UNLOCK,
	DMD_ATSC_NULL
};

#if	(DMD_ATSC_STR_EN &&	!DMD_ATSC_UTOPIA_EN	&& !DMD_ATSC_UTOPIA2_EN)
/*ckang, redefine*/
//typedef	enum {
//	E_POWER_SUSPEND = 1,
//	E_POWER_RESUME = 2,
//	E_POWER_MECHANICAL = 3,
//	E_POWER_SOFT_OFF = 4,
//}	EN_POWER_MODE;

#endif

enum DMD_ATSC_SEL_IQPAD {
	I_PAD,
	Q_PAD,
};

struct DLL_PACKED DMD_J83B_Info {
	u8	qam_type;
	u16	symbol_rate;
};

enum eDMD_SEL {
	DMD0 = 0,
	DMD1,
	HK,
};

///	For	demod	init
struct DLL_PACKED DMD_ATSC_INITDATA {
	// Timeout time
	u16 vsbagclockchecktime;//100
	u16 vsbprelockchecktime;//300
	u16 vsbfsynclockchecktime;//1200
	u16 vsbfeclockchecktime;//5000

	u16 qamagclockchecktime;//100
	u16 qamprelockchecktime;//1000
	u16 qammainlockchecktime;//3000

	// register	init
	u8	*u8DMD_ATSC_DSPRegInitExt; //	TODO use system	variable type
	u8	u8DMD_ATSC_DSPRegInitSize;
	u8	*dmd_atsc_initext; //	TODO use system	variable type

	//By Tuners:
	u16	if_khz;//By Tuners
	u8	biqswap;//0
	u16	agc_reference;//0
	u8	btunergaininvert;//0
	u8	bis_qpad;//0	//Don't	beed used	anymore

	//By IC:
	u8	is_dual;//0
	u8	bisextdemod;//0

	//By TS	(Only	for	MCP	or ext demod):
	u8	u1TsConfigByte_SerialMode	:	1;
	u8	u1TsConfigByte_DataSwap	:	1;
	u8	u1TsConfigByte_ClockInv	:	1;
	u8	u5TsConfigByte_DivNum		:	5;

	//By SYS I2C (Only for MCP or	ext	demod):
	u8	u8I2CSlaveAddr;
	u8	u8I2CSlaveBus;
	u8 (*I2C_WriteBytes)(u16 busnumslaveid, u8 u8addrcount,
		u8 *pu8addr, u16 size, u8 *pdata_8);
	u8 (*I2C_ReadBytes)(u16	busnumslaveid,
		u8 addrnum, u8 *paddr, u16 size, u8 *pdata_8);

	//By SYS MSPI (Only for MCP or ext demod):
	u8	bIsUseSspiLoadCode;
	u8	bIsSspiUseTsPin;
};

//struct ATSC_IOCTL_DATA DMD_ATSC_IOCTL_DATA;




struct DLL_PACKED DMD_ATSC_INFO {
	u8  version;
	u32 atscscantimestart;
	u32 atscfeclocktime;
	u32 atsclockstatus;
};


#if	DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
struct DLL_PACKED DMD_ATSC_SYS_PTR {
	void (*delayms)(u32 u32Ms);
	void (*mspi_slaveenable)(u8 enable);

	MSPI_ErrorNo(*mspi_init_ext)(u8 hw_num);
	MSPI_ErrorNo(*mspi_write)(u8 *pdata, u16 size);
	MSPI_ErrorNo(*mspi_read)(u8 *pdata, u16 size);
};
#else
struct DLL_PACKED DMD_ATSC_SYS_PTR {
	u32  (*getsystemtimems)(void); // Get sys time (unit: ms)
	void    (*delayms)(u32 ms); // Delay time (unit: ms)
	u8 (*createmutex)(u8 enable); // Create&Delete mutex
	void    (*lockdmd)(u8 enable); // Enter&Leave mutex

	u32 (*mspi_init_ext)(u8 hw_num);
	void   (*mspi_slaveenable)(u8 enable);
	u32 (*mspi_write)(u8 *pdata, u16 size);
	u32 (*mspi_read)(u8 *pdata, u16 size);
};
#endif

struct DLL_PACKED ATSC_IOCTL_DATA
{
	u8   id;
	struct  DMD_ATSC_RESDATA *pres;
	u8   *pextchipchnum;
	u8   *pextchipchid;
	u8   *pextchipi2cch;
	struct  DMD_ATSC_SYS_PTR sys;
};

struct DLL_PACKED DMD_ATSC_PRIDATA {
	size_t/*MS_VIRT*/ virtdmdbaseaddr;

	u8 binit;
	u8 bdownloaded;

	#if DMD_ATSC_STR_EN
	u8 bisdtv;
	enum EN_POWER_MODE elaststate;
	#endif
	enum DMD_ATSC_DEMOD_TYPE elasttype;
	u16 symrate;

	u8 bis_qpad;

	u8 (*hal_dmd_atsc_ioctl_cmd)(
	struct ATSC_IOCTL_DATA *pdata, enum DMD_ATSC_HAL_COMMAND eCmd,
	void *pPara);
};

struct DLL_PACKED DMD_ATSC_RESDATA {
	struct DMD_ATSC_INITDATA sdmd_atsc_initdata;
	struct DMD_ATSC_PRIDATA sdmd_atsc_pridata;
	struct DMD_ATSC_INFO sdmd_atsc_info;
};
//--------------------------------------------------------------------
//	Function and Variable
//--------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------------------------------------------
///	Set	detailed level of	ATSC driver	debug	message
///	dbglevel : debug level for Parallel	Flash	driver\n
///	AVD_DBGLV_NONE,	 ///<	disable	all	the	debug	message\n
///	AVD_DBGLV_INFO,	 ///<	information\n
///	AVD_DBGLV_NOTICE,	 ///<	normal but significant condition\n
///	AVD_DBGLV_WARNING, ///<	warning	conditions\n
///	AVD_DBGLV_ERR,	 ///<	error	conditions\n
///	AVD_DBGLV_CRIT,	 ///<	critical conditions\n
///	AVD_DBGLV_ALERT,	 ///<	action must	be taken immediately\n
///	AVD_DBGLV_EMERG,	 ///<	system is	unusable\n
///	AVD_DBGLV_DEBUG,	 ///<	debug-level	messages\n
///	@return	TRUE : succeed
///	@return	FALSE	:	failed to	set	the	debug	level
//--------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_atsc_setdbglevel(
enum DMD_ATSC_DBGLV dbglevel);
//--------------------------------------------------------------------
///	Get	the	information	of ATSC	driver\n
///	@return	the	pointer	to the driver	information
//--------------------------------------------------------------------
DLL_PUBLIC extern struct DMD_ATSC_INFO *mdrv_dmd_atsc_getinfo(void);
//--------------------------------------------------------------------
///	Get	ATSC driver	version
///	when get ok, return	the	pointer	to the driver	version
//--------------------------------------------------------------------
//DLL_PUBLIC extern	u8	mdrv_dmd_atsc_getlibver
//(const	MSIF_Version **ppversion);

////////////////////////////////////////////////////////////////////////////////
///	Should be	called once	when power on	init (no use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_initial_hal_interface(void);

#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN

////////////////////////
/// SINGLE DEMOD API ///
////////////////////////

////////////////////////////////////////////////////////////////////////////////
///	Should be	called every time	when enter DTV input source
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_init(
struct DMD_ATSC_INITDATA *pdmd_atsc_initdata, u32 initdatalen);
////////////////////////////////////////////////////////////////////////////////
/// Should be called every time when exit DTV input source
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_exit(void);
////////////////////////////////////////////////////////////////////////////////
///	Get	Initial	Data
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u32 mdrv_dmd_atsc_getconfig(
struct DMD_ATSC_INITDATA *psdmd_atsc_initdata);
////////////////////////////////////////////////////////////////////////////////
///	Set	demod	mode and enable	demod
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_setconfig(
enum DMD_ATSC_DEMOD_TYPE etype, u8 benable);
////////////////////////////////////////////////////////////////////////////////
///	Reset	FW state machine (no use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_setreset(void);
////////////////////////////////////////////////////////////////////////////////
///	Set	demod	mode and enable	demod	(only	for	J83B)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_set_qam_sr(
enum DMD_ATSC_DEMOD_TYPE etype, u16 symbol_rate);
////////////////////////////////////////////////////////////////////////////////
///	Active demod (not	use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_setactive(u8	benable);
////////////////////////////////////////////////////////////////////////////////
///	Set	demod	power	state	for	STR
////////////////////////////////////////////////////////////////////////////////
#if	DMD_ATSC_STR_EN
DLL_PUBLIC extern u32 mdrv_dmd_atsc_setpowerstate(
enum EN_POWER_MODE powerstate);
#endif
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	lock status
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern enum DMD_ATSC_LOCK_STATUS mdrv_dmd_atsc_getlock(
enum DMD_ATSC_GETLOCK_TYPE etype);
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	modulation mode
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern enum DMD_ATSC_DEMOD_TYPE
mdrv_dmd_atsc_getmodulationmode(void);
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	signal strength	(IF	AGC	gain)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_getsignalstrength(u16 *strength);
////////////////////////////////////////////////////////////////////////////////
/// Get demod signal quality (NO SIGNAL, WEAK, MODERATE, STRONG and VERY_STRONG)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern enum DMD_ATSC_SIGNAL_CONDITION mdrv_dmd_atsc_getsignalquality(
void);
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	SNR	percentage (MAX	SNR	40dB)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_getsnrpercentage(void);
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	SNR	(only	for	J.83ABC)
////////////////////////////////////////////////////////////////////////////////
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_get_qam_snr(float	*f_snr);
#else
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_get_qam_snr
(struct DMD_ATSC_SNR_DATA *f_snr);
#endif
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	packet error number
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_read_ucpkt_err(u16 *packeterr);
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	pre	Viterbi	BER
////////////////////////////////////////////////////////////////////////////////
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_getpreviterbiber(float *ber);
#else
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_getpreviterbiber
(struct DMD_ATSC_BER_DATA *ber);
#endif
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	post Viterbi BER
////////////////////////////////////////////////////////////////////////////////
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_getpostviterbiber(float	*ber);
#else
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_getpostviterbiber
(struct DMD_ATSC_BER_DATA *ber);
#endif
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	frequency	offset
////////////////////////////////////////////////////////////////////////////////
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_readfrequencyoffset(s16 *cfo);
#else
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_readfrequencyoffset
(struct DMD_ATSC_CFO_DATA *cfo);
#endif
////////////////////////////////////////////////////////////////////////////////
///	Set	TS output	mode (only for external	demod)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_setserialcontrol(u8 tsconfig_data);
////////////////////////////////////////////////////////////////////////////////
///	enable I2C bypass	mode (only for external	demod)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_iic_bypass_mode(u8 benable);
////////////////////////////////////////////////////////////////////////////////
///	Switch pin to	SSPI or	GPIO (only for external	demod)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_switch_sspi_gpio(u8 benable);
////////////////////////////////////////////////////////////////////////////////
///	Get	GPIO pin high	or low (only for external	demod)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_gpio_get_level(
u8 pin_8, u8 *blevel);
////////////////////////////////////////////////////////////////////////////////
///	Set	GPIO pin high	or low (only for external	demod)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_gpio_set_level(
u8 pin_8, u8 blevel);
////////////////////////////////////////////////////////////////////////////////
///	Set	GPIO pin output	or input (only for external	demod)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_gpio_out_enable(u8 pin_8,
u8 benableout);
////////////////////////////////////////////////////////////////////////////////
///	Swap ADC input (usually	for	external demod)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_doiqswap(u8 bis_qpad);

////////////////////////////////////////////////////////////////////////////////
/// To get ATSC's register value, only for special purpose.\n
///	addr_16		:	the	address	of ATSC's	register\n
///	pdata_8		:	the	value	to be	gotten\n
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_getreg(u16 addr_16, u8 *pdata_8);
////////////////////////////////////////////////////////////////////////////////
///	To set ATSC's	register value,	only for special purpose.\n
///	addr_16		:	the	address	of ATSC's	register\n
///	value		:	the	value	to be	set\n
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_setreg(u16 addr_16, u8 data_8);

////////////////////////////////////////////////////////////////////////////////
///	Set	Digital	RF frequency KHz (only for Digital RF)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_set_digrf_freq(u32 rf_freq);

/////////////////////////////
///BACKWARD	COMPATIBLE API///
/////////////////////////////

#ifndef	UTPA2
////////////////////////////////////////////////////////////////////////////////
///	SEL	DEMOD	(no	use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_sel_dmd(enum eDMD_SEL eDMD_NUM);
////////////////////////////////////////////////////////////////////////////////
///	Load FW	(no	use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_loadfw(enum eDMD_SEL DMD_NUM);
////////////////////////////////////////////////////////////////////////////////
///	Set	VSB	mode (no use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_setvsbmode(void);
////////////////////////////////////////////////////////////////////////////////
///	Set	256QAM mode	(no	use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_set256qammode(void);
////////////////////////////////////////////////////////////////////////////////
///	Set	64QAM	mode (no use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_set64qammode(void);
#endif

#endif // #if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN

////////////////////////////
///	 MULTI DEMOD API	 ///
////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// Should be called every time when enter DTV input source
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_init(u8 id,
struct DMD_ATSC_INITDATA *pdmd_atsc_initdata, u32 initdatalen);
////////////////////////////////////////////////////////////////////////////////
/// Should be called every time when exit DTV input source
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_exit(u8 id);
////////////////////////////////////////////////////////////////////////////////
///	Get	Initial	Data
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u32 mdrv_dmd_atsc_md_getconfig(u8 id,
struct DMD_ATSC_INITDATA *psdmd_atsc_initdata);
////////////////////////////////////////////////////////////////////////////////
///	Set	demod	mode and enable	demod
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_setconfig(u8 id,
enum DMD_ATSC_DEMOD_TYPE etype, u8 benable);
////////////////////////////////////////////////////////////////////////////////
///	Reset	FW state machine (no use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_md_setreset(u8	id);
////////////////////////////////////////////////////////////////////////////////
///	Set	demod	mode and enable	demod	(only	for	J83B)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_set_qam_sr(u8 id,
enum DMD_ATSC_DEMOD_TYPE etype, u16 symbol_rate);
////////////////////////////////////////////////////////////////////////////////
///	Active demod (not	use)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_setactive(u8 id, u8 benable);
////////////////////////////////////////////////////////////////////////////////
///	Set	demod	power	state	for	STR
////////////////////////////////////////////////////////////////////////////////
#if DMD_ATSC_STR_EN
DLL_PUBLIC extern u32 mdrv_dmd_atsc_md_setpowerstate(u8 id,
enum EN_POWER_MODE powerstate);
#endif
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	lock status
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern enum DMD_ATSC_LOCK_STATUS mdrv_dmd_atsc_md_getlock(u8 id,
enum DMD_ATSC_GETLOCK_TYPE etype);
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	modulation mode
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern enum DMD_ATSC_DEMOD_TYPE mdrv_dmd_atsc_md_getmodulationmode(
u8 id);
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	signal strength	(IF	AGC	gain)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_getsignalstrength(
u8 id, u16 *strength);
////////////////////////////////////////////////////////////////////////////////
///Get demod signal quality (NO SIGNAL, WEAK, MODERATE, STRONG and VERY_STRONG)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern enum DMD_ATSC_SIGNAL_CONDITION
mdrv_dmd_atsc_md_getsignalquality(u8 id);
////////////////////////////////////////////////////////////////////////////////
///	Get	demod	SNR	percentage (MAX	SNR	40dB)
////////////////////////////////////////////////////////////////////////////////
DLL_PUBLIC extern	u8	mdrv_dmd_atsc_md_getsnrpercentage(u8	id);
///////////////////////////////////////////////////////////////////
///	Get	demod	SNR	(only	for	J.83ABC)
///////////////////////////////////////////////////////////////////
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_get_qam_snr(u8 id,
float *f_snr);
#else
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_get_qam_snr(u8 id,
struct DMD_ATSC_SNR_DATA *f_snr);
#endif
//////////////////////////////////////////////////////////////
/// Get demod packet error number
//////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_read_ucpkt_err(
u8 id, u16 *packeterr);
//////////////////////////////////////////////////////////////
/// Get demod pre Viterbi BER
//////////////////////////////////////////////////////////////
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_getpreviterbiber(u8 id,
float *ber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_getpreviterbiber(u8 id,
struct DMD_ATSC_BER_DATA *ber);
#endif
//////////////////////////////////////////////////////////////
///	Get	demod	post Viterbi BER
//////////////////////////////////////////////////////////////
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_getpostviterbiber(
u8 id, float *ber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_getpostviterbiber(u8 id,
struct DMD_ATSC_BER_DATA *ber);
#endif
////////////////////////////////////////////////////////////
/// Get demod frequency offset
////////////////////////////////////////////////////////////
#ifndef MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_readfrequencyoffset(
u8 id, s16 *cfo);
#else
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_readfrequencyoffset(
u8 id, struct DMD_ATSC_CFO_DATA *cfo);
#endif
////////////////////////////////////////////////////////////
/// Set TS output mode (only for external demod)
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_setserialcontrol(
u8 id, u8 tsconfig_data);
////////////////////////////////////////////////////////////
/// enable I2C bypass mode (only for external demod)
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_iic_bypass_mode(
u8 id, u8 benable);
////////////////////////////////////////////////////////////
/// Switch pin to SSPI or GPIO (only for external demod)
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_switch_sspi_gpio(
u8 id, u8 benable);
////////////////////////////////////////////////////////////
/// Get GPIO pin high or low (only for external demod)
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_gpio_get_level(
u8 id, u8 pin_8, u8 *blevel);
////////////////////////////////////////////////////////////
/// Set GPIO pin high or low (only for external demod)
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_gpio_set_level(
u8 id, u8 pin_8, u8 blevel);
////////////////////////////////////////////////////////////
/// Set GPIO pin output or input (only for external demod)
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_gpio_out_enable(u8 id,
u8 pin_8, u8	benableout);
////////////////////////////////////////////////////////////
/// Swap ADC input (usually for external demod)
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_doiqswap(
u8 id, u8 bis_qpad);

////////////////////////////////////////////////////////////
///To get ATSC's register value, only for special purpose.\n
///addr_16 :the address of ATSC's register\n
///pdata_8 :the value to be gotten\n
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_getreg(
u8 id, u16 addr_16, u8 *pdata_8);
////////////////////////////////////////////////////////////
///To set ATSC's register value, only for special purpose.\n
///addr_16 : the address of ATSC's register\n
///value : the value to be set\n
////////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_setreg(u8 id,
u16 addr_16, u8 data_8);

//////////////////////////////////////////////////////////
/// Set Digital RF frequency KHz (only for Digital RF)
//////////////////////////////////////////////////////////
DLL_PUBLIC extern u8 mdrv_dmd_atsc_md_set_digrf_freq(
u8 id, u32 rf_freq);

#ifdef UTPA2
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_setdbglevel(
enum DMD_ATSC_DBGLV dbglevel);
DLL_PUBLIC extern struct DMD_ATSC_INFO *_mdrv_dmd_atsc_getinfo(void);
//DLL_PUBLIC extern u8 _mdrv_dmd_atsc_getlibver(
//const MSIF_Version **ppversion);

DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_init(u8 id,
struct DMD_ATSC_INITDATA *pdmd_atsc_initdata, u32 initdatalen);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_exit(u8 id);
DLL_PUBLIC extern u32 _mdrv_dmd_atsc_md_getconfig(u8 id,
struct DMD_ATSC_INITDATA *psdmd_atsc_initdata);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_setconfig(u8 id,
enum DMD_ATSC_DEMOD_TYPE etype, u8 benable);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_setreset(u8 id);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_set_qam_sr(u8 id,
enum DMD_ATSC_DEMOD_TYPE etype, u16 symbol_rate);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_setactive(u8 id,
u8 benable);
#if	DMD_ATSC_STR_EN
DLL_PUBLIC extern u32 _mdrv_dmd_atsc_md_setpowerstate(
u8 id, enum EN_POWER_MODE powerstate);
#endif
DLL_PUBLIC extern enum DMD_ATSC_LOCK_STATUS _mdrv_dmd_atsc_md_getlock(
u8 id, enum DMD_ATSC_GETLOCK_TYPE etype);
DLL_PUBLIC extern enum DMD_ATSC_DEMOD_TYPE _mdrv_dmd_atsc_md_getmodulationmode(
u8 id);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_getsignalstrength(u8 id,
u16 *strength);
DLL_PUBLIC extern enum DMD_ATSC_SIGNAL_CONDITION
_mdrv_dmd_atsc_md_getsignalquality(u8 id);

DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_getsnrpercentage(u8 id);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_get_qam_snr(u8 id,
struct DMD_ATSC_SNR_DATA *f_snr);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_read_ucpkt_err(u8 id,
u16 *packeterr);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_getpreviterbiber(u8 id,
struct DMD_ATSC_BER_DATA *ber);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_getpostviterbiber(u8 id,
struct DMD_ATSC_BER_DATA *ber);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_readfrequencyoffset(u8 id,
struct DMD_ATSC_CFO_DATA *cfo);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_setserialcontrol(u8 id,
u8 tsconfig_data);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_iic_bypass_mode(u8 id,
u8	benable);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_switch_sspi_gpio(u8 id,
u8 benable);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_gpio_get_level(u8 id,
u8 pin_8, u8 *blevel);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_gpio_set_level(u8 id,
u8 pin_8, u8 blevel);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_gpio_out_enable(u8 id,
u8 pin_8, u8 benableout);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_doiqswap(u8 id,
u8 bis_qpad);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_getreg(u8 id,
u16 addr_16, u8 *pdata_8);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_setreg(u8 id,
u16	addr_16, u8 data_8);

DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_set_digrf_freq(u8 id,
u32 rf_freq);
DLL_PUBLIC extern u8 _mdrv_dmd_atsc_md_get_info(struct dvb_frontend *fe,
struct dtv_frontend_properties *p);
DLL_PUBLIC extern ssize_t atsc_get_information_show(
struct device_driver *driver, char *buf);
#endif //	#ifdef UTPA2

#ifdef __cplusplus
}
#endif

#endif //	_DRV_ATSC_H_
