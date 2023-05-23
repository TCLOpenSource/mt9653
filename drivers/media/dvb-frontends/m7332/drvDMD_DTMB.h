/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef	_DRV_DTMB_H_
#define	_DRV_DTMB_H_

//-------------------------------------------------------------------
// Driver	Compiler	Options
//-------------------------------------------------------------------
#ifndef	DVB_STI // VT:	Temp	define	for	DVB_Frontend
	#define	DVB_STI		1
#endif

#if	DVB_STI	||	!defined	UTPA2//M5	Enter
#define	DMD_DTMB_UTOPIA_EN  1
#define	DMD_DTMB_UTOPIA2_EN 0
#else
#define	DMD_DTMB_UTOPIA_EN  0
#define	DMD_DTMB_UTOPIA2_EN 1
#endif

#if	DVB_STI	||	defined	MTK_PROJECT//VT
#define	DMD_DTMB_STR_EN              1
#define	DMD_DTMB_MULTI_THREAD_SAFE   1
#define	DMD_DTMB_MULTI_DMD_EN        1
#define	DMD_DTMB_3PARTY_EN           1
#define	DMD_DTMB_3PARTY_IN_KERNEL_EN 1
#define	DMD_DTMB_3PARTY_MSPI_EN      1
#else
#define	DMD_DTMB_STR_EN              1
#define	DMD_DTMB_MULTI_THREAD_SAFE   0
#define	DMD_DTMB_MULTI_DMD_EN        1
#define	DMD_DTMB_3PARTY_EN           0
#define	DMD_DTMB_3PARTY_IN_KERNEL_EN 0
#define	DMD_DTMB_3PARTY_MSPI_EN      0
#endif

#if	DMD_DTMB_3PARTY_EN
#undef	DMD_DTMB_UTOPIA_EN
#undef	DMD_DTMB_UTOPIA2_EN

#define	DMD_DTMB_UTOPIA_EN 0
#define	DMD_DTMB_UTOPIA2_EN	0

#if	DMD_DTMB_3PARTY_IN_KERNEL_EN
	#ifndef	UTPA2
	#define	UTPA2
	#endif
	#ifndef	MSOS_TYPE_LINUX_KERNEL
	#define	MSOS_TYPE_LINUX_KERNEL
	#endif
#endif // #if	DMD_DTMB_3PARTY_IN_KERNEL_EN
#endif // #if	DMD_DTMB_3PARTY_EN

//-------------------------------------------------------------------
// Include	Files
//-------------------------------------------------------------------

#if	defined	MTK_PROJECT//VT
	#include	"PD_Def.h"
#elif	DVB_STI
#include <media/dvb_frontend.h>
#include <asm/io.h>
#include <linux/delay.h>
#include "m7332_demod_common.h"
#elif	DMD_DTMB_3PARTY_EN
//Please add header files needed by 3 party	platform PRINT and MEMCPY define
#endif

//#include	"MsTypes.h"
#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
#ifndef	MSIF_TAG
#include	"MsVersion.h"
#endif
#include	"MsCommon.h"
#endif
#if DMD_DTMB_UTOPIA2_EN || (DMD_DTMB_STR_EN && DMD_DTMB_UTOPIA_EN)
#include	"utopia.h"
#endif
#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
#include	"drvMSPI.h"
#endif

//-------------------------------------------------------------------
// Driver	Capability
//-------------------------------------------------------------------


//-------------------------------------------------------------------
// Macro and	Define
//-------------------------------------------------------------------

#ifndef	DLL_PACKED
#define	DLL_PACKED
#endif

#ifndef	DLL_PUBLIC
#define	DLL_PUBLIC
#endif

#if	defined	MTK_PROJECT//VT
#define	PRINT(fmt, ...)	mcSHOW_DBG_MSG4((fmt, ##__VA_ARGS__))
#define	MEMCPY					x_memcpy
#elif	DVB_STI
#define	PRINT		printk
#define	MEMCPY		memcpy
#elif	DMD_DTMB_3PARTY_EN
//Please define PRINT and MEMCPY according to 3 party platform
#define	PRINT		printk
#define	MEMCPY		memcpy
#else // #if	DMD_DTMB_3PARTY_EN
#ifdef	MSOS_TYPE_LINUX_KERNEL
#define	PRINT		printk
#define	MEMCPY		memcpy
#else // #ifdef	MSOS_TYPE_LINUX_KERNEL
#define	PRINT		printf
#define	MEMCPY		memcpy
#endif // #ifdef	MSOS_TYPE_LINUX_KERNEL
#endif // #if	DMD_DTMB_3PARTY_EN

#if	DMD_DTMB_MULTI_DMD_EN
#define	DMD_DTMB_MAX_DEMOD_NUM					2
#else
#define	DMD_DTMB_MAX_DEMOD_NUM					1
#endif

#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
#define	MSIF_DMD_DTMB_LIB_CODE {'D', 'M', 'D', '_'	'D', 'T', 'M', 'B', '_'}
#define	MSIF_DMD_DTMB_LIBVER {'0', '0'} //LIB	version
#define	MSIF_DMD_DTMB_BUILDNUM {'0', '0'	} //Build	Number
#define	MSIF_DMD_DTMB_CHANGELIST {'0', '0', '0', '0', '0', '0', '0', '0'}

#define	DMD_DTMB_VER /*Character String	for	DRV/API	version*/\
	 (MSIF_TAG,  /*	'MSIF'	 */	\
		MSIF_CLASS,  /*	'00'		 */	\
		MSIF_CUS,  /*	0x0000	 */	\
		MSIF_MOD,  /*	0x0000	 */	\
		MSIF_CHIP,	\
		MSIF_CPU, \
		MSIF_DMD_DTMB_LIB_CODE,  /*	IP__		 */	\
		MSIF_DMD_DTMB_LIBVER, /*0.0	~Z.Z */	\
		MSIF_DMD_DTMB_BUILDNUM,  /*	00	~	99 */	\
		MSIF_DMD_DTMB_CHANGELIST,  /*	CL#		 */	\
		MSIF_OS)
#endif // #if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

#ifndef	BIT_
#define	BIT_(n) (1 << (n))
#endif

#define	DMD_DTMB_LOCK_DTMB_PNP_LOCK  BIT_(0)
#define	DMD_DTMB_LOCK_DTMB_FEC_LOCK  BIT_(1)

#define	DMD_DTMB_LOCK_DVBC_AGC_LOCK  BIT_(8)
#define	DMD_DTMB_LOCK_DVBC_PRE_LOCK  BIT_(9)
#define	DMD_DTMB_LOCK_DVBC_MAIN_LOCK BIT_(10)

//-------------------------------------------------------------------
// Type and	Structure
//-------------------------------------------------------------------

//typedef	enum{
enum DMD_DTMB_HAL_COMMAND {
		DMD_DTMB_HAL_CMD_Exit	= 0,
		DMD_DTMB_HAL_CMD_InitClk,
		DMD_DTMB_HAL_CMD_Download,
		DMD_DTMB_HAL_CMD_FWVERSION,
		DMD_DTMB_HAL_CMD_SoftReset,
		DMD_DTMB_HAL_CMD_SetACICoef,
		DMD_DTMB_HAL_CMD_SetDTMBMode,
		DMD_DTMB_HAL_CMD_SetModeClean,
		DMD_DTMB_HAL_CMD_Set_QAM_SR,
		DMD_DTMB_HAL_CMD_Active,
		DMD_DTMB_HAL_CMD_AGCLock,
		DMD_DTMB_HAL_CMD_DTMB_PNP_Lock,
		DMD_DTMB_HAL_CMD_DTMB_FEC_Lock,
		DMD_DTMB_HAL_CMD_DVBC_PreLock,
		DMD_DTMB_HAL_CMD_DVBC_Main_Lock,
		DMD_DTMB_HAL_CMD_GetModulation,
		DMD_DTMB_HAL_CMD_ReadIFAGC,
		DMD_DTMB_HAL_CMD_ReadFrequencyOffset,
		DMD_DTMB_HAL_CMD_ReadSNRPercentage,
		DMD_DTMB_HAL_CMD_GetPreLdpcBer,
		DMD_DTMB_HAL_CMD_GetPreViterbiBer,
		DMD_DTMB_HAL_CMD_GetPostViterbiBer,
		DMD_DTMB_HAL_CMD_GetSNR,
		DMD_DTMB_HAL_CMD_TS_INTERFACE_CONFIG,
		DMD_DTMB_HAL_CMD_IIC_Bypass_Mode,
		DMD_DTMB_HAL_CMD_SSPI_TO_GPIO,
		DMD_DTMB_HAL_CMD_GPIO_GET_LEVEL,
		DMD_DTMB_HAL_CMD_GPIO_SET_LEVEL,
		DMD_DTMB_HAL_CMD_GPIO_OUT_ENABLE,
		DMD_DTMB_HAL_CMD_DoIQSwap,
		DMD_DTMB_HAL_CMD_GET_REG,
		DMD_DTMB_HAL_CMD_SET_REG,
		DMD_DTMB_HAL_CMD_SQI,
		DMD_DTMB_HAL_CMD_SSI,
		DMD_DTMB_HAL_CMD_ICP,
		DMD_DTMB_HAL_CMD_IFAGC
};//DMD_DTMB_HAL_COMMAND;

//typedef	enum{
enum DMD_DTMB_DEMOD_TYPE {
		DMD_DTMB_DEMOD_DTMB,
		DMD_DTMB_DEMOD_DTMB_7M,
		DMD_DTMB_DEMOD_DTMB_6M,
		DMD_DTMB_DEMOD_DTMB_5M,
		DMD_DTMB_DEMOD_DVBC_64QAM,
		DMD_DTMB_DEMOD_DVBC_256QAM,
		DMD_DTMB_DEMOD_DVBC_16QAM,
		DMD_DTMB_DEMOD_DVBC_32QAM,
		DMD_DTMB_DEMOD_DVBC_128QAM,
		DMD_DTMB_DEMOD_MAX,
		DMD_DTMB_DEMOD_NULL	= DMD_DTMB_DEMOD_MAX,
};//	DMD_DTMB_DEMOD_TYPE;

//typedef	struct	DLL_PACKED {
struct	DLL_PACKED DMD_DTMB_MODULATION_INFO {
		union{
				#ifdef	UTPA2
				u16 fsicoderate; //0.4, 0.6, 0.8
				u16 fDvbcSymRateL;
				#else
				float	fsicoderate; //0.4, 0.6, 0.8
				float	fDvbcSymRateL;
				#endif
		};
		union{
				u16 usiinterleaver16; //240, 720
				u16 udvbcsymrateh16;
		};
		union{
				u8 usiqammode8; //4QAM, 16QAM, 32QAM, 64QAM
				u8 udvbcqammode8;
		};
		u8 usinr8;
		u8 usicarriermode8; //0: multiple carrier,1:single carrier
		u16 upnm16; //420, 595, 945
		u8 upnc8; //0:	variable, 1:	constant
		enum DMD_DTMB_DEMOD_TYPE eDemodType; //DTMB or	DVBC
};//	DMD_DTMB_MODULATION_INFO;

//typedef	struct	DLL_PACKED {
struct	DLL_PACKED DMD_DTMB_CFO_DATA {
		s16	fftfirstcfo;
		s8	fftsecondcfo;
		s16	sr;
};//	DMD_DTMB_CFO_DATA;

//typedef	struct	DLL_PACKED {
struct DLL_PACKED DMD_DTMB_BER_DATA {
		u32	biterr;
		u16 error_window;
};//DMD_DTMB_BER_DATA;

//typedef	struct	DLL_PACKED {
struct	DLL_PACKED DMD_DTMB_SNR_DATA {
		u32	snr;
		u16 sym_num;
};//	DMD_DTMB_SNR_DATA;

//typedef	struct	DLL_PACKED {
struct	DLL_PACKED DMD_DTMB_GPIO_PIN_DATA {
		u8 upin8;
		union{
				u8	blevel;
				u8	bIsOut;
		};
};//	DMD_DTMB_GPIO_PIN_DATA;

//typedef	struct	DLL_PACKED {
struct DLL_PACKED DMD_DTMB_REG_DATA {
		u16 uaddr16;
		u8 udata8;
};//	DMD_DTMB_REG_DATA;

//typedef	enum{
enum DMD_DTMB_DbgLv {
		DMD_DTMB_DBGLV_NONE,  // disable all the debug message
		DMD_DTMB_DBGLV_INFO,  // information
		DMD_DTMB_DBGLV_NOTICE, // normal but significant condition
		DMD_DTMB_DBGLV_WARNING, // warning	conditions
		DMD_DTMB_DBGLV_ERR, // error	conditions
		DMD_DTMB_DBGLV_CRIT,  // critical	conditions
		DMD_DTMB_DBGLV_ALERT,  // action must be taken immediately
		DMD_DTMB_DBGLV_EMERG,  // system	is	unusable
		DMD_DTMB_DBGLV_DEBUG,  // debug-level	messages
};//	DMD_DTMB_DbgLv;

//typedef	enum{
enum DMD_DTMB_GETLOCK_TYPE {
		DMD_DTMB_GETLOCK,
		DMD_DTMB_GETLOCK_DTMB_AGCLOCK,
		DMD_DTMB_GETLOCK_DTMB_PNPLOCK,
		DMD_DTMB_GETLOCK_DTMB_FECLOCK,
		DMD_DTMB_GETLOCK_DVBC_AGCLOCK,
		DMD_DTMB_GETLOCK_DVBC_PRELOCK,
		DMD_DTMB_GETLOCK_DVBC_MAINLOCK,
};//	DMD_DTMB_GETLOCK_TYPE;

//typedef	enum{
enum DMD_DTMB_LOCK_STATUS {
		DMD_DTMB_LOCK,
		DMD_DTMB_CHECKING,
		DMD_DTMB_CHECKEND,
		DMD_DTMB_UNLOCK,
		DMD_DTMB_NULL,
};//	DMD_DTMB_LOCK_STATUS;

//#if (DMD_DTMB_STR_EN && !DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN)
//typedef	enum{
//		E_POWER_SUSPEND = 1,
//		E_POWER_RESUME = 2,
//		E_POWER_MECHANICAL	= 3,
//		E_POWER_SOFT_OFF = 4,
//}	EN_POWER_MODE;
//#endif

/// For	demod	init
//typedef	struct	DLL_PACKED {
struct	DLL_PACKED	DMD_DTMB_InitData {
	 // init
		u16 udtmbagclockchecktime16;//50
		u16 udtmbprelockchecktime16;//300
		u16 udtmbpnmlockchecktime16;//1200
		u16 udtmbfeclockchecktime16;//5000

		u16 uqamagclockchecktime16;//50
		u16 uqamprelockchecktime16;//1000
		u16 uqammainlockchecktime16;//3000

	 // register	init
		u8 *u8DMD_DTMB_DSPRegInitExt; //TODO use system variable type
		u8 u8DMD_DTMB_DSPRegInitSize;
		u8 *u8DMD_DTMB_InitExt; // TODO use system variable type

	 //By	Tuners:
		u16 uif_khz16;//By	Tuners
		u8	bIQSwap;//0
		u16 uagc_reference16;//0
		u8	btunergaininvert;//0

	 //By	IC:
		u8 uis_dual8;//0
		u8	bisextdemod;//0

	 //By	TS (Only	for	MCP	or	ext	demod):
		u8 u1TsConfigByte_SerialMode	:	1;
		u8 u1TsConfigByte_DataSwap		:	1;
		u8 u1TsConfigByte_ClockInv		:	1;
		u8 u5TsConfigByte_DivNum			:	5;

	 //By	SYS	I2C (Only	for	MCP	or	ext	demod):
		u8 u8I2CSlaveAddr;
		u8 u8I2CSlaveBus;
		u8 (*I2C_WriteBytes)(u16 u16BusNumSlaveID,
		u8 u8addrcount, u8 *pu8addr, u16 usize16,
	    u8 *pudata8);
		u8 (*I2C_ReadBytes)(u16 u16BusNumSlaveID,
		u8 u8AddrNum, u8 *paddr, u16 usize16, u8 *pudata8);

	 //By	SYS	MSPI (Only	for	MCP	or	ext	demod):
		u8	bIsUseSspiLoadCode;
		u8	bIsSspiUseTsPin;

	 //By	SYS	memory	mapping (Only	for	int	demod):
		u32	utdistartaddr32;
};//	DMD_DTMB_InitData;

//typedef	struct	DTMB_IOCTL_DATA	DMD_DTMB_IOCTL_DATA;


//struct DLL_PACKED	DTMB_IOCTL_DATA {
//		u8 id;
//		struct DMD_DTMB_ResData *pRes;
//		struct DMD_DTMB_SYS_PTR	sys;
//};
//
////typedef	struct	DLL_PACKED {
//struct	DLL_PACKED DMD_DTMB_PriData {
//		size_t	virtDMDBaseAddr;
//
//		u8	bInit;
//		u8	bDownloaded;
//
//		#if	DMD_DTMB_STR_EN
//		u8	bIsDTV;
//		EN_POWER_MODE eLastState;
//		#endif
//		enum DMD_DTMB_DEMOD_TYPE eLastType;
//		u16 usymrate16;
//
//		u8	bIsQPad;
//		u16 uchipid16;
//
//	 u8 (*HAL_DMD_DTMB_IOCTL_CMD)(struct DTMB_IOCTL_DATA *pdata,
//		enum DMD_DTMB_HAL_COMMAND eCmd, void *pPara);
//};//	DMD_DTMB_PriData;

//typedef	struct	DLL_PACKED {
struct	DLL_PACKED DMD_DTMB_Info {
		u8 uversion8;
		u32	udtmbscantimestart32;
		u32	udtmbfeclocktime32;
		u32	udtmblockstatus32;
};//	DMD_DTMB_Info;

//typedef	struct	DLL_PACKED {
//struct	DLL_PACKED DMD_DTMB_ResData {
//		struct	DMD_DTMB_InitData	sDMD_DTMB_InitData;
//		struct DMD_DTMB_PriData		sDMD_DTMB_PriData;
//		struct DMD_DTMB_Info			sDMD_DTMB_Info;
//};//	DMD_DTMB_ResData;

#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
//typedef	struct	DLL_PACKED {
struct	DLL_PACKED DMD_DTMB_SYS_PTR {
		void (*DelayMS)(u32	ums32);

		MSPI_ErrorNo(*MSPI_Init_Ext)(u8 uhwnum8);
		void (*MSPI_SlaveEnable)(u8 enable);

		MSPI_ErrorNo(*MSPI_Write)(u8 *pdata, u16 usize16);
		MSPI_ErrorNo(*MSPI_Read)(u8 *pdata, u16 usize16);
};// DMD_DTMB_SYS_PTR;
#else
//typedef	struct	DLL_PACKED {
struct	DLL_PACKED DMD_DTMB_SYS_PTR {
		u32 (*getsystemtimems)(void);//Get sys time (unit: ms)
		void (*DelayMS)(u32	ms); // Delay time (unit: ms)
		u8 (*CreateMutex)(u8 enable); // Create&Delete mutex
		void (*LockDMD)(u8 enable); // Enter&Leave	mutex
		u32 (*MSPI_Init_Ext)(u8 uhwnum8);
		void (*MSPI_SlaveEnable)(u8 enable);
		u32 (*MSPI_Write)(u8 *pdata, u16 usize16);
		u32 (*MSPI_Read)(u8 *pdata, u16 usize16);
};// DMD_DTMB_SYS_PTR;
#endif
//struct DLL_PACKED	DTMB_IOCTL_DATA {
//		u8 id;
//		struct DMD_DTMB_ResData *pRes;
//		struct DMD_DTMB_SYS_PTR	sys;
//};

struct DLL_PACKED	DTMB_IOCTL_DATA {
		u8 id;
		struct DMD_DTMB_ResData *pRes;
		struct DMD_DTMB_SYS_PTR	sys;
};

//typedef	struct	DLL_PACKED {
struct	DLL_PACKED DMD_DTMB_PriData {
		size_t/*MS_VIRT*/	virtDMDBaseAddr;

		u8	bInit;
		u8	bDownloaded;

		#if	DMD_DTMB_STR_EN
		u8	bIsDTV;
		enum EN_POWER_MODE eLastState;
		#endif
		enum DMD_DTMB_DEMOD_TYPE eLastType;
		u16 usymrate16;

		u8	bIsQPad;
		u16 uchipid16;

	 u8 (*HAL_DMD_DTMB_IOCTL_CMD)(struct DTMB_IOCTL_DATA *pdata,
		enum DMD_DTMB_HAL_COMMAND eCmd, void *pPara);
};//	DMD_DTMB_PriData;


struct	DLL_PACKED DMD_DTMB_ResData {
		struct	DMD_DTMB_InitData	sDMD_DTMB_InitData;
		struct DMD_DTMB_PriData		sDMD_DTMB_PriData;
		struct DMD_DTMB_Info			sDMD_DTMB_Info;
};//	DMD_DTMB_ResData;

//-------------------------------------------------------------------
// Function and	Variable
//-------------------------------------------------------------------

#ifdef	__cplusplus
extern	"C"
{
#endif

//-------------------------------------------------------------------
/// Set	detailed	level	of	DTMB driver	debug	message
/// @ingroup	DTMB_BASIC
/// @param	u8DbgLevel \b IN: debug	level for	Parallel Flash driver
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_setdbglevel(
enum DMD_DTMB_DbgLv	u8DbgLevel);
//-------------------------------------------------------------------
/// Get	the	information	of	DTMB driver
/// @ingroup	DTMB_INFO
/// @return	:	the	pointer	to	the	driver	information
//-------------------------------------------------------------------
DLL_PUBLIC extern	struct DMD_DTMB_Info *mdrv_dmd_dtmb_getinfo(void);
//-------------------------------------------------------------------
/// Get	DTMB driver	version
/// @ingroup	DTMB_INFO
/// @param	ppVersion \b OUT: the	pointer	to	the	driver	version
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
//DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getlibver(
//const MSIF_Version **ppVersion);
//-------------------------------------------------------------------
/// Initialize	HAL	interface
/// @ingroup	DTMB_BASIC
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_initial_hal_interface(void);

#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

////////////////////////////////////////////////////////////////////////////////
///SINGLE DEMOD API///
////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------
/// Initialize	DTMB Demod
/// @ingroup	DTMB_BASIC
/// @param	pDMD_DTMB_InitData \b IN: DTMB initial	parameters
/// @param	uinitdatalen32 \b IN: size of	DTMB initial	parameters
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_init(
struct DMD_DTMB_InitData *pDMD_DTMB_InitData, u32 uinitdatalen32);
//-------------------------------------------------------------------
/// Exit	DTMB DTV	mode
/// @ingroup	DTMB_STR
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_exit(void);
//-------------------------------------------------------------------
/// Get	Initial	Data
/// @ingroup	DTMB_BASIC
/// @param	psDMD_DTMB_InitData \b IN: DTMB initial	parameters
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u32 mdrv_dmd_dtmb_getconfig(
struct DMD_DTMB_InitData *psDMD_DTMB_InitData);
//-------------------------------------------------------------------
/// Set	Demod	mode and	restart	Demod
/// @ingroup	DTMB_BASIC
/// @param	eType \b IN: Select	DTMB modulation	type
/// @param	bEnable	\b IN: Enable	SetConfig	function
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_setconfig(
DMD_DTMB_DEMOD_TYPE eType, u8 bEnable);
//-------------------------------------------------------------------
/// Reset	FW	state	machine
/// @ingroup	DTMB_ToBeRemoved
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_setreset(void);
//-------------------------------------------------------------------
/// Set	QAM	Type and	symbol rate
/// @ingroup	DTMB_ToBeRemoved
/// @param	eType \b IN: QAM	type
/// @param	symbol_rate \b IN: symbol rate
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_set_qam_sr(
DMD_DTMB_DEMOD_TYPE eType, u16 symbol_rate);
//-------------------------------------------------------------------
/// Active	Demod
/// @ingroup	DTMB_ToBeRemoved
/// @param	bEnable \b IN: Active	Demod	if	TRUE
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_setactive(u8 bEnable);
//-------------------------------------------------------------------
/// Support	DTMB STR	function
/// @ingroup	DTMB_STR
/// @param	upowerstate16 \b IN: Set	STR	status
/// @return	:	STR	status
//-------------------------------------------------------------------
#if	DMD_DTMB_STR_EN
DLL_PUBLIC extern u32 mdrv_dmd_dtmb_setpowerstate(
enum EN_POWER_MODE upowerstate16);
#endif
//-------------------------------------------------------------------
/// Get	lock	status
/// @ingroup	DTMB_INFO
/// @param	eType \b IN: select	lock	type
/// @return	:	lock	status
//-------------------------------------------------------------------
DLL_PUBLIC extern	enum DMD_DTMB_LOCK_STATUS mdrv_dmd_dtmb_getlock(
	DMD_DTMB_GETLOCK_TYPE	eType);
//-------------------------------------------------------------------
/// Get	modulation	mode
/// @ingroup	DTMB_INFO
/// @return	:	modulation	mode
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getmodulationmode(
	struct DMD_DTMB_MODULATION_INFO *sDtmbModulationMode);
//-------------------------------------------------------------------
/// Get	Demod	signal	strength (IF	AGC	gain)
/// @ingroup	DTMB_INFO
/// @param	ustrength16 \b OUT: the	pointer	to	signal	strength
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getsignalstrength(
	u16 *ustrength16);
//-------------------------------------------------------------------
/// Get	Demod	frequency	offset
/// @ingroup	DTMB_INFO
/// @param	cfo \b OUT: the	pointer	to	CFO
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_readfrequencyoffset(s16 *cfo);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_readfrequencyoffset(
struct DMD_DTMB_CFO_DATA *cfo);
#endif
//-------------------------------------------------------------------
/// Get	signal	quality
/// @ingroup	DTMB_INFO
/// @return	:	signal	quality
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getsignalquality(void);
//-------------------------------------------------------------------
/// Get	pre	LDPC	data
/// @ingroup	DTMB_INFO
/// @param	pber \b OUT: the	pointer	to	pre	LDPC	data
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getpreldpcber(float *pber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getpreldpcber(
struct DMD_DTMB_BER_DATA *pber);
#endif
//-------------------------------------------------------------------
/// Get	Demod	pre	Viterbi	number
/// @ingroup	DTMB_ToBeRemoved
/// @param	ber \b OUT: the	pointer	to	pre	Viterbi	BER
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getpreviterbiber(float *ber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getpreviterbiber(
struct DMD_DTMB_BER_DATA *ber);
#endif
//-------------------------------------------------------------------
/// Get	Demod	post	Viterbi	number
/// @ingroup	DTMB_INFO
/// @param	ber \b OUT: the	pointer	to	post	Viterbi	BER
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getpostviterbiber(float *ber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getpostviterbiber(
struct DMD_DTMB_BER_DATA *ber);
#endif
//-------------------------------------------------------------------
/// Get	SNR
/// @ingroup	DTMB_INFO
/// @param	snr \b OUT: the	pointer	to	SNR
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getsnr(float *snr);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getsnr(struct DMD_DTMB_SNR_DATA *snr);
#endif
//-------------------------------------------------------------------
/// Set	TS	output	mode
/// @ingroup	DTMB_BASIC
/// @param	utsconfigdata8 \b IN: TS	configuration
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_setserialcontrol(
u8 utsconfigdata8);
//-------------------------------------------------------------------
/// Enable	I2C	bypass	mode
/// @ingroup	DTMB_BASIC
/// @param	bEnable \b IN: Enable	bypass	mode
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_iic_bypass_mode(u8 bEnable);
//-------------------------------------------------------------------
/// Set	SSPI	pin	as	GPIO function
/// @ingroup	DTMB_BASIC
/// @param	bEnable \b IN: Switch	to	GPIO if	TRUE, otherwise SSPI
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_switch_sspi_gpio(u8 bEnable);
//-------------------------------------------------------------------
/// Get	GPIO level
/// @ingroup	DTMB_BASIC
/// @param	upin8 \b IN: Select	pin	number
/// @param	blevel \b OUT: the	pointer	to	GPIO level
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_gpio_get_level(u8 upin8,
	u8 *blevel);
//-------------------------------------------------------------------
/// Set	GPIO level
/// @ingroup	DTMB_BASIC
/// @param	upin8 \b IN: Select	pin	number
/// @param	blevel \b IN: Set	GPIO level
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_gpio_set_level(u8 upin8,
	u8	blevel);
//-------------------------------------------------------------------
/// Set	GPIO as	output	or	input
/// @ingroup	DTMB_BASIC
/// @param	upin8 \b IN: Select	pin	number
/// @param	bEnableOut \b IN: output	if	TRUE, otherwise input
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_gpio_out_enable(u8 upin8,
	u8 bEnableOut);
//-------------------------------------------------------------------
/// Swap	ADC	input
/// @ingroup	DTMB_BASIC
/// @param	bIsQPad \b IN: Q pad	if	TRUE, otherwise I	pad
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_doiqswap(u8	bIsQPad);
//-------------------------------------------------------------------
/// Get	Demod	register	value
/// @ingroup	DTMB_BASIC
/// @param	uaddr16 \b IN: register	address
/// @param	pudata8 \b OUT: the	pointer	to	register	data
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_getreg(u16 uaddr16,
	u8 *pudata8);
//-------------------------------------------------------------------
/// Set	Demod	register	value
/// @ingroup	DTMB_BASIC
/// @param	uaddr16 \b IN: register	address
/// @param	udata8 \b IN: register	data
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_setreg(u16 uaddr16,
	u8 udata8);

#endif // #if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

////////////////////////////////////////////////////////////////////////////////
///MULTI DEMOD API ///
////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------
/// Initialize	multiple	DTMB Demod
/// @ingroup	DTMB_BASIC
/// @param	id			 \b IN: Select	Demod number
/// @param	pDMD_DTMB_InitData \b IN: DTMB initial	parameters
/// @param	uinitdatalen32 \b IN: size of	DTMB initial	parameters
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_init(u8 id,
	struct DMD_DTMB_InitData *pDMD_DTMB_InitData, u32 uinitdatalen32);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	exiting	DTMB DTV	mode
/// @ingroup	DTMB_STR
/// @param	id \b IN: Select	Demod number
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_exit(u8 id);
//-------------------------------------------------------------------
/// Get	Initial	Data
/// @ingroup	DTMB_BASIC
/// @param	@param	id \b IN: Select	Demod number
/// @param	psDMD_DTMB_InitData \b IN: DTMB initial	parameters
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u32 mdrv_dmd_dtmb_md_getconfig(u8 id,
struct	DMD_DTMB_InitData *psDMD_DTMB_InitData);
//-------------------------------------------------------------------
/// Support multiple Demod of setting and	restarting	Demod
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	eType \b IN: Select	DTMB modulation	type
/// @param	bEnable	\b IN: Enable	SetConfig	function
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_setconfig(u8 id,
	enum DMD_DTMB_DEMOD_TYPE eType, u8 bEnable);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	reset	FW	state	machine
/// @ingroup	DTMB_ToBeRemoved
/// @param	id \b IN: Select	Demod number
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_setreset(u8 id);
//-------------------------------------------------------------------
/// Support multiple Demod of setting	QAM	Type and symbol rate
/// @ingroup	DTMB_ToBeRemoved
/// @param	id \b IN: Select	Demod number
/// @param	eType \b IN: QAM	type
/// @param	symbol_rate \b IN: symbol rate
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_set_qam_sr(u8 id,
	enum DMD_DTMB_DEMOD_TYPE eType, u16 symbol_rate);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	active	Demod
/// @ingroup	DTMB_ToBeRemoved
/// @param	id \b IN: Select	Demod number
/// @param	bEnable \b IN: Active	Demod	if	TRUE
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_setactive(u8 id,
	u8 bEnable);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	DTMB STR	function
/// @ingroup	DTMB_STR
/// @param	id \b IN: Select	Demod number
/// @param	upowerstate16 \b IN: Set	STR	status
/// @return	:	STR	status
//-------------------------------------------------------------------
#if	DMD_DTMB_STR_EN
DLL_PUBLIC extern u32 mdrv_dmd_dtmb_md_setpowerstate(u8 id,
	enum EN_POWER_MODE upowerstate16);
#endif
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	lock	status
/// @ingroup	DTMB_INFO
/// @param	id \b IN: Select	Demod number
/// @param	eType \b IN: select	lock	type
/// @return	:	lock	status
//-------------------------------------------------------------------
DLL_PUBLIC extern	enum DMD_DTMB_LOCK_STATUS mdrv_dmd_dtmb_md_getlock(
u8 id,	enum DMD_DTMB_GETLOCK_TYPE	eType);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	modulation	mode
/// @ingroup	DTMB_INFO
/// @param	id \b IN: Select	Demod number
/// @return	:	modulation	mode
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getmodulationmode(u8 id,
	struct DMD_DTMB_MODULATION_INFO *sDtmbModulationMode);
//-------------------------------------------------------------------
/// Support multiple Demod of getting Demod signal strength(IF AGC gain)
/// @ingroup	DTMB_INFO
/// @param	id \b IN: Select	Demod number
/// @param	ustrength16 \b OUT: the	pointer	to	signal	strength
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getsignalstrength(u8 id,
	u16 *ustrength16);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	Demod	fequency	offset
/// @ingroup	DTMB_INFO
/// @param	id \b IN: Select	Demod number
/// @param	cfo \b OUT: the	pointer	to	CFO
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_readfrequencyoffset(u8 id,
	s16 *cfo);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_readfrequencyoffset(u8 id,
struct DMD_DTMB_CFO_DATA *cfo);
#endif
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	Demod	signal	quality
/// @ingroup	DTMB_INFO
/// @param	id \b IN: Select	Demod number
/// @return	:	signal	quality
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getsignalquality(u8 id);

DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getsqi(u8 id);
DLL_PUBLIC extern u16 mdrv_dmd_dtmb_md_geticp(u8 id);
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getssi(u8 id);
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getagc(u8 id);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	pre	LDPC	data
/// @ingroup	DTMB_INFO
/// @param	id \b IN: Select	Demod number
/// @param	pber \b OUT: the	pointer	to	pre	LDPC	data
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getpreldpcber(u8 id,
	float *pber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getpreldpcber(u8 id,
struct DMD_DTMB_BER_DATA *pber);
#endif
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	Demod	pre	Viterbi	number
/// @ingroup	DTMB_ToBeRemoved
/// @param	id \b IN: Select	Demod number
/// @param	ber \b OUT: the	pointer	to	BER
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getpreviterbiber(u8 id,
	float *ber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getpreviterbiber(u8 id,
struct DMD_DTMB_BER_DATA *ber);
#endif
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	Demod	post	Viterbi	number
/// @ingroup	DTMB_INFO
/// @param	id \b IN: Select	Demod number
/// @param	ber \b OUT: the	pointer	to	BER
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getpostviterbiber(u8 id,
	float *ber);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getpostviterbiber(u8 id,
struct DMD_DTMB_BER_DATA *ber);
#endif
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	SNR
/// @ingroup	DTMB_INFO
/// @param	id \b IN: Select	Demod number
/// @param	snr \b OUT: the	pointer	to	SNR
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
#ifndef	MSOS_TYPE_LINUX_KERNEL
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getsnr(u8 id,
	float *snr);
#else
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getsnr(u8 id,
struct DMD_DTMB_SNR_DATA *snr);
#endif
//-------------------------------------------------------------------
/// Support multiple Demod of setting	TS	output	mode
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	utsconfigdata8 \b IN: TS	configuration
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_setserialcontrol(u8 id,
	u8 utsconfigdata8);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	enabling	I2C	bypass	mode
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	bEnable \b IN: Enable	bypass	mode
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_iic_bypass_mode(u8 id,
	u8 bEnable);
//-------------------------------------------------------------------
/// Support multiple Demod of setting	SSPI pin as	GPIO function
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	bEnable \b IN: Switch to GPIO if TRUE, otherwise SSPI
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_switch_sspi_gpio(u8 id,
	u8 bEnable);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	getting	GPIO level
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	upin8 \b IN: Select	pin	number
/// @param	blevel \b OUT: the	pointer	to	GPIO level
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_gpio_get_level(u8 id,
	u8 upin8, u8 *blevel);
//-------------------------------------------------------------------
/// Support multiple Demod of setting	GPIO level
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	upin8 \b IN: Select	pin	number
/// @param	blevel \b IN: Set	GPIO level
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_gpio_set_level(u8 id,
	u8 upin8, u8 blevel);
//-------------------------------------------------------------------
/// Support	multiple Demod of setting GPIO as output or input
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	upin8 \b IN: Select	pin	number
/// @param	bEnableOut \b IN: output	if	TRUE, otherwise input
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_gpio_out_enable(u8 id,
	u8 upin8, u8 bEnableOut);
//-------------------------------------------------------------------
/// Support	multiple Demod	of	swapping	ADC	input
/// @ingroup	DTMB_BASIC
/// @param	bIsQPad	\b IN: Q pad if	TRUE, otherwise I pad
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_doiqswap(u8 id,
	u8	bIsQPad);
//-------------------------------------------------------------------
/// Support	multiple Demod of setting	Demod register value
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	uaddr16 \b IN: register	address
/// @param	pudata8	\b OUT: the	pointer	to get register data
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_getreg(u8 id,
	u16 uaddr16, u8 *pudata8);
//-------------------------------------------------------------------
/// Support	multiple Demod of setting	Demod register value
/// @ingroup	DTMB_BASIC
/// @param	id \b IN: Select	Demod number
/// @param	uaddr16 \b IN: register	address
/// @param	udata8 \b IN: register	data
/// @return	TRUE	:	succeed
/// @return	FALSE	:	fail
//-------------------------------------------------------------------
DLL_PUBLIC extern u8 mdrv_dmd_dtmb_md_setreg(u8 id,
	u16 uaddr16, u8 udata8);

#ifdef	UTPA2
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_setdbglevel(
	enum DMD_DTMB_DbgLv u8DbgLevel);
DLL_PUBLIC extern	struct DMD_DTMB_Info *_mdrv_dmd_dtmb_getinfo(void);
//DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_getlibver(
//	const MSIF_Version **ppVersion);

DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_init(u8 id,
struct DMD_DTMB_InitData *pDMD_DTMB_InitData, u32	uinitdatalen32);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_exit(u8 id);
DLL_PUBLIC extern u32	_mdrv_dmd_dtmb_md_getconfig(u8 id,
struct DMD_DTMB_InitData *psDMD_DTMB_InitData);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_setconfig(u8 id,
	enum DMD_DTMB_DEMOD_TYPE eType, u8 bEnable);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_setreset(u8 id);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_set_qam_sr(u8 id,
	enum DMD_DTMB_DEMOD_TYPE eType, u16 symbol_rate);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_setactive(u8 id,
	u8 bEnable);
#if	DMD_DTMB_STR_EN
DLL_PUBLIC extern u32	_mdrv_dmd_dtmb_md_setpowerstate(u8 id,
	enum EN_POWER_MODE upowerstate16);
#endif
DLL_PUBLIC extern	enum DMD_DTMB_LOCK_STATUS _mdrv_dmd_dtmb_md_getlock(
u8 id,	enum DMD_DTMB_GETLOCK_TYPE	eType);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_getmodulationmode(u8 id,
	struct DMD_DTMB_MODULATION_INFO *sDtmbModulationMode);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_getsignalstrength(u8 id,
	u16 *ustrength16);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_readfrequencyoffset(u8 id,
struct DMD_DTMB_CFO_DATA *cfo);
DLL_PUBLIC extern u8 _mdrv_dmd_dtmb_md_getsignalquality(u8 id);
DLL_PUBLIC extern u8 _mdrv_dmd_dtmb_md_getagc(u8 id);
DLL_PUBLIC extern u8 _mdrv_dmd_dtmb_md_getsqi(u8 id);
DLL_PUBLIC extern u16 _mdrv_dmd_dtmb_md_geticp(u8 id);
DLL_PUBLIC extern u8 _mdrv_dmd_dtmb_md_getssi(u8 id);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_getpreldpcber(u8 id,
struct DMD_DTMB_BER_DATA *pber);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_getpreviterbiber(u8 id,
struct DMD_DTMB_BER_DATA *ber);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_getpostviterbiber(u8 id,
struct DMD_DTMB_BER_DATA *ber);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_getsnr(u8 id,
struct DMD_DTMB_SNR_DATA *snr);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_setserialcontrol(u8 id,
	u8 utsconfigdata8);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_iic_bypass_mode(u8 id,
	u8 bEnable);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_switch_sspi_gpio(u8 id,
	u8 bEnable);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_gpio_get_level(u8 id,
	u8 upin8, u8 *blevel);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_gpio_set_level(u8 id,
	u8 upin8, u8	blevel);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_gpio_out_enable(u8 id,
	u8 upin8, u8 bEnableOut);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_doiqswap(u8 id,
	u8	bIsQPad);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_getreg(u8 id,
	u16 uaddr16, u8 *pudata8);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_md_setreg(u8 id,
	u16 uaddr16, u8 udata8);
#ifdef	CONFIG_UTOPIA_PROC_DBG_SUPPORT
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_dbg_getmoduleinfo(u8 id,
	u64 *pu64ReqHdl);
DLL_PUBLIC extern u8	_mdrv_dmd_dtmb_dbg_echocmd(u8 id,
	u64 *pu64ReqHdl, u32	ucmdsize32, s8 *pcmd);
#endif

DLL_PUBLIC extern int	_mdrv_dmd_drv_dtmb_get_frontend(struct dvb_frontend *fe,
			struct dtv_frontend_properties *p);

DLL_PUBLIC extern ssize_t dtmb_get_information_show(
struct device_driver *driver, char *buf);

#endif // #ifdef	UTPA2

#ifdef	__cplusplus
}
#endif

#endif // _DRV_DTMB_H_
