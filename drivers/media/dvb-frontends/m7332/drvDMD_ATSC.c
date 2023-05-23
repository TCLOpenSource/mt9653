// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//------------------------------------------------------
// Driver Compiler Options
//------------------------------------------------------

//------------------------------------------------------
// Include Files
//------------------------------------------------------
#include <media/dvb_frontend.h>
#include "drvDMD_ATSC.h"
#if DMD_ATSC_UTOPIA2_EN
#include "drvDMD_ATSC_v2.h"
#endif

#if !defined MTK_PROJECT
#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/string.h>
#include <linux/kernel.h>
#if defined(CONFIG_CC_IS_CLANG) && defined(CONFIG_MSTAR_CHIP)
#include <linux/module.h>
#endif
#else
#include <string.h>
#include <stdio.h>
#include <math.h>
#endif
#endif

#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
#include "MsOS.h"
#include "drvMMIO.h"
#endif

#if	DMD_ATSC_3PARTY_EN
// Please add header files needed by 3 perty platform
#endif

//typedef u8(*IOCTL)
//(struct ATSC_IOCTL_DATA, enum DMD_ATSC_HAL_COMMAND, void*);
//static IOCTL _null_ioctl_fp_atsc;

static u8 (*_null_ioctl_fp_atsc)(struct ATSC_IOCTL_DATA*,
enum DMD_ATSC_HAL_COMMAND, void*);

#ifdef MSOS_TYPE_LINUX_KERNEL
#if !DMD_ATSC_UTOPIA_EN && DMD_ATSC_UTOPIA2_EN
//typedef u8(*SWI2C)(u8 busnum, u8 slaveid, u8 addrcnt,
//u8 * pu8addr, u16 size, u8 * pbuf);
//typedef u8(*HWI2C)(u8 u8Port, u8 slaveidiiC, u8 addrsizeiic,
//u8 * paddriic, u32 bufsizeiic, u8 * pbufiiC);

//static SWI2C _NULL_SWI2C_FP_ATSC;
//static HWI2C _null_hwi2c_fp_atsc;
static u8 (*_null_swi2c_fp_atsc)(u8 busnum, u8 slaveid,
u8 addrcnt, u8 *pu8addr, u16 size, u8 *pbuf);

static u8 (*_null_hwi2c_fp_atsc)(u8 busnum, u8 slaveid,
u8 addrcnt, u8 *pu8addr, u16 size, u8 *pbuf);

//#else
//static SWI2C NULL;
//static HWI2C NULL;
#endif
#endif

static u8 _default_ioctl_cmd(
struct ATSC_IOCTL_DATA *pdata, enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs)
{
	#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
	#ifdef REMOVE_HAL_INTERN_ATSC
	PRINT("LINK ERROR: REMOVE_HAL_INTERN_ATSC\n");
	#else
	PRINT("LINK ERROR: Please link ext demod library\n");
	#endif
	#else
	PRINT("LINK	ERROR: Please link int or ext demod library\n");
	#endif

	return TRUE;
}

//#if defined(MSOS_TYPE_LINUX_KERNEL) && \
//defined(CONFIG_CC_IS_CLANG) && defined(CONFIG_MSTAR_CHIP)
#if !DVB_STI
static u8 (*hal_intern_atsc_ioctl_cmd)(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs);
static u8 (*hal_extern_atsc_ioctl_cmd)(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs);

static u8 (*mdrv_sw_iic_writebytes)(u8 busnum, u8 slaveid,
u8 addrcnt, u8 *pu8addr, u16 size, u8 *pbuf);

static u8 (*mdrv_sw_iic_readbytes)(u8 busnum, u8 slaveid,
u8 ucsubddr, u8 *paddr, u16 ucbuflen, u8 *pbuf);

static u8 (*mdrv_hw_iic_writebytes)(u8 u8Port, u8 slaveidiiC,
u8 addrsizeiic, u8 *paddriic, u32 bufsizeiic, u8 *pbufiiC);

static u8 (*mdrv_hw_iic_readbytes)(u8 u8Port, u8 slaveidiiC,
u8 addrsizeiic, u8 *paddriic, u32 bufsizeiic, u8 *pbufiiC);

#else
extern u8 hal_intern_atsc_ioctl_cmd(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs);// __attribute__((weak));

u8 hal_extern_atsc_ioctl_cmd(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs) __attribute__((weak));

#ifndef MSOS_TYPE_LINUX_KERNEL
#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
u8 hal_extern_atsc_ioctl_cmd_0(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs) __attribute__((weak));
u8 hal_extern_atsc_ioctl_cmd_1(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs) __attribute__((weak));

u8 hal_extern_atsc_ioctl_cmd_2(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs) __attribute__((weak));

u8 hal_extern_atsc_ioctl_cmd_3(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pargs) __attribute__((weak));

typedef u8 (*ext_ioctl)(struct ATSC_IOCTL_DATA*,
enum DMD_ATSC_HAL_COMMAND, void*);

u8 EXT_ATSC_CHIP[4] = { 0x77, 0xB5, 0xC7, 0x00};
ext_ioctl EXT_ATSC_IOCTL[4] = {
hal_extern_atsc_ioctl_cmd_0, hal_extern_atsc_ioctl_cmd_1,
hal_extern_atsc_ioctl_cmd_2, hal_extern_atsc_ioctl_cmd_3};
#endif
#else	// #ifndef MSOS_TYPE_LINUX_KERNEL
#if !DMD_ATSC_UTOPIA_EN && DMD_ATSC_UTOPIA2_EN
u8 mdrv_sw_iic_writebytes(u8 busnum, u8 slaveid,
u8 addrcnt, u8 *pu8addr, u16 size,
u8 *pbuf) __attribute__((weak));

u8 mdrv_sw_iic_readbytes(u8 busnum, u8 slaveid,
u8 ucsubddr, u8 *paddr, u16 ucbuflen,
u8 *pbuf) __attribute__((weak));

u8 mdrv_hw_iic_writebytes(u8 u8Port, u8 slaveidiiC,
u8 addrsizeiic, u8 *paddriic, u32 bufsizeiic,
u8 *pbufiiC) __attribute__((weak));

u8 mdrv_hw_iic_readbytes(u8 u8Port, u8 slaveidiiC,
u8 addrsizeiic, u8 *paddriic, u32 bufsizeiic,
u8 *pbufiiC) __attribute__((weak));
#endif
#endif // #ifndef MSOS_TYPE_LINUX_KERNEL

#endif

//------------------------------------------------------
//	Local	Defines
//------------------------------------------------------

#if	DMD_ATSC_UTOPIA_EN && !DMD_ATSC_UTOPIA2_EN
 #define DMD_LOCK()		 \
	do {
		MS_ASSERT(MsOS_In_Interrupt() == FALSE);
		if (_s32DMD_ATSC_Mutex == -1)
			return FALSE;
		if (dmd_atsc_dbglevel == DMD_ATSC_DBGLV_DEBUG)
			PRINT("%s lock mutex\n", __func__);
		MsOS_ObtainMutex(_s32DMD_ATSC_Mutex, MSOS_WAIT_FOREVER);
		}	while (0)

 #define DMD_UNLOCK() \
	do	{	\
		MsOS_ReleaseMutex(_s32DMD_ATSC_Mutex);\
		if (dmd_atsc_dbglevel ==	DMD_ATSC_DBGLV_DEBUG)
			PRINT("%s unlock mutex\n", __func__); } while (0)
#elif (!DMD_ATSC_UTOPIA_EN \
&& (!DMD_ATSC_UTOPIA2_EN) && (DMD_ATSC_MULTI_THREAD_SAFE))
 #define DMD_LOCK() piodata->sys.lockdmd(TRUE)
 #define DMD_UNLOCK() piodata->sys.lockdmd(FALSE)
#else
 #define DMD_LOCK()
 #define DMD_UNLOCK()
#endif

#ifdef MS_DEBUG
#define	DMD_DBG(x)			(x)
#else
#define	DMD_DBG(x)			//(x)
#endif

#ifndef	UTOPIA_STATUS_SUCCESS
#define	UTOPIA_STATUS_SUCCESS				0x00000000
#endif
#ifndef	UTOPIA_STATUS_FAIL
#define	UTOPIA_STATUS_FAIL					0x00000001
#endif

//------------------------------------------------------
//	Local	Structurs
//------------------------------------------------------



//------------------------------------------------------
//	Global Variables
//------------------------------------------------------

#if	DMD_ATSC_UTOPIA2_EN
struct DMD_ATSC_RESDATA *psdmd_atsc_resdata;
u8 *pDMD_ATSC_EXT_CHIP_CH_NUM;
u8 *pDMD_ATSC_EXT_CHIP_CH_ID;
u8 *pDMD_ATSC_EXT_CHIP_I2C_CH;

struct ATSC_IOCTL_DATA dmd_atsc_iodata = { 0, 0, 0, 0, 0, { 0 } };
#endif

//------------------------------------------------------
//	Local	Variables
//------------------------------------------------------

#if !DMD_ATSC_UTOPIA2_EN
struct DMD_ATSC_RESDATA sDMD_ATSC_ResData[DMD_ATSC_MAX_DEMOD_NUM] = { { {0},
{0}, {0} } };
static u8 DMD_ATSC_EXT_CHIP_CH_NUM[DMD_ATSC_MAX_EXT_CHIP_NUM] = { 0 };
static u8 DMD_ATSC_EXT_CHIP_CH_ID[DMD_ATSC_MAX_DEMOD_NUM] = { 0 };
static u8 DMD_ATSC_EXT_CHIP_I2C_CH[DMD_ATSC_MAX_EXT_CHIP_NUM] = { 0 };

struct DMD_ATSC_RESDATA	*psdmd_atsc_resdata	=	sDMD_ATSC_ResData;
static u8 *pDMD_ATSC_EXT_CHIP_CH_NUM = DMD_ATSC_EXT_CHIP_CH_NUM;
static u8 *pDMD_ATSC_EXT_CHIP_CH_ID = DMD_ATSC_EXT_CHIP_CH_ID;
static u8 *pDMD_ATSC_EXT_CHIP_I2C_CH = DMD_ATSC_EXT_CHIP_I2C_CH;

struct ATSC_IOCTL_DATA dmd_atsc_iodata = { 0, 0, 0, 0, 0, { 0 } };
#endif

struct ATSC_IOCTL_DATA *piodata	=	&dmd_atsc_iodata;

#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
//static MSIF_Version	_drv_dmd_atsc_version;
#endif

#if	DMD_ATSC_UTOPIA_EN && !DMD_ATSC_UTOPIA2_EN
static s32	_s32DMD_ATSC_Mutex = -1;
#elif	defined	MTK_PROJECT
static HANDLE_T	_s32DMD_ATSC_Mutex = -1;
#elif	DVB_STI
static s32	_s32DMD_ATSC_Mutex = -1;
#endif

#ifdef UTPA2

#if	DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN

static void *_ppatscinstant;

static u32 _atscopen;

#endif

#endif

enum DMD_ATSC_DBGLV dmd_atsc_dbglevel = DMD_ATSC_DBGLV_NONE;

//------------------------------------------------
//	Debug	Functions
//------------------------------------------------


//------------------------------------------------
//	Local	Functions
//------------------------------------------------

static enum DMD_ATSC_LOCK_STATUS _vsb_checklock(void)
{
u8 (*ioctl)(struct ATSC_IOCTL_DATA *pdata,
enum DMD_ATSC_HAL_COMMAND eCmd, void *pPara);
u16 timeout;

struct DMD_ATSC_RESDATA *pres = piodata->pres;
struct DMD_ATSC_INITDATA *pInit = &(pres->sdmd_atsc_initdata);
struct DMD_ATSC_INFO *pinfo = &(pres->sdmd_atsc_info);

//ioctl = qqq;
ioctl = (u8 (*)(struct ATSC_IOCTL_DATA *, enum DMD_ATSC_HAL_COMMAND,
void *))(pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd);

if (ioctl(piodata, DMD_ATSC_HAL_CMD_VSB_FEC_LOCK, NULL)) {
	pinfo->atsclockstatus |= DMD_ATSC_LOCK_VSB_FEC_LOCK;
	#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
	pinfo->atscfeclocktime = MsOS_GetSystemTime();
	#else
	pinfo->atscfeclocktime = piodata->sys.getsystemtimems();
	#endif
	return DMD_ATSC_LOCK;
} else {
	#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
	if ((pinfo->atsclockstatus & DMD_ATSC_LOCK_VSB_FEC_LOCK) &&
	((MsOS_GetSystemTime()-pinfo->atscfeclocktime) < 100))
	#else
	if ((pinfo->atsclockstatus & DMD_ATSC_LOCK_VSB_FEC_LOCK) &&
	((piodata->sys.getsystemtimems()-pinfo->atscfeclocktime) < 100))
	#endif
	{
		return DMD_ATSC_LOCK;

	} else {
		pinfo->atsclockstatus &= (~DMD_ATSC_LOCK_VSB_FEC_LOCK);
	}

	if (pinfo->atsclockstatus & DMD_ATSC_LOCK_VSB_PRE_LOCK) {
		if (pinfo->atsclockstatus & DMD_ATSC_LOCK_VSB_FSYNC_LOCK) {
			#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
			if ((MsOS_GetSystemTime()-
			pinfo->atscscantimestart) <
			pInit->vsbfeclockchecktime)
			#else
			if ((piodata->sys.getsystemtimems()-
			pinfo->atscscantimestart) <
			pInit->vsbfeclockchecktime)
			#endif
			{
				if (ioctl(
				piodata, DMD_ATSC_HAL_CMD_AGCLOCK, NULL))
					return DMD_ATSC_CHECKING;
			}
		} else {
			if (ioctl(piodata,
			DMD_ATSC_HAL_CMD_VSB_FSYNC_LOCK, NULL)) {
				pinfo->atsclockstatus |=
				DMD_ATSC_LOCK_VSB_FSYNC_LOCK;

				#ifdef MS_DEBUG
				if (dmd_atsc_dbglevel >=
				DMD_ATSC_DBGLV_DEBUG)
					PRINT("DMD_ATSC_LOCK_VSB_FSYNC_LOCK");
				#endif
				#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
				pinfo->atscscantimestart =
				MsOS_GetSystemTime();
				#else
				pinfo->atscscantimestart =
				piodata->sys.getsystemtimems();
				#endif
				return DMD_ATSC_CHECKING;

			} else {
				#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
				if ((MsOS_GetSystemTime()-
				pinfo->atscscantimestart) <
				pInit->vsbfsynclockchecktime)
				#else
				if ((piodata->sys.getsystemtimems()-
				pinfo->atscscantimestart) <
				pInit->vsbfsynclockchecktime)
				#endif
				{
					#if (DMD_ATSC_UTOPIA_EN) || \
					(DMD_ATSC_UTOPIA2_EN)
					if (ioctl(
				piodata, DMD_ATSC_HAL_CMD_AGCLOCK, NULL) ||
			(MsOS_GetSystemTime()-pinfo->atscscantimestart) <
					pInit->vsbagclockchecktime)
						return DMD_ATSC_CHECKING;
					#else
					if (ioctl(
				piodata, DMD_ATSC_HAL_CMD_AGCLOCK, NULL) ||
					(piodata->sys.getsystemtimems()-
					pinfo->atscscantimestart) <
					pInit->vsbagclockchecktime)
						return DMD_ATSC_CHECKING;
					#endif
				}
			}
		}
	}	else	{
		if (ioctl(piodata,
		DMD_ATSC_HAL_CMD_VSB_PRELOCK, NULL)) {
			pinfo->atsclockstatus |=
			DMD_ATSC_LOCK_VSB_PRE_LOCK;

			#ifdef MS_DEBUG
			if (dmd_atsc_dbglevel >=
				DMD_ATSC_DBGLV_DEBUG)
				PRINT("DMD_ATSC_LOCK_VSB_PRE_LOCK\n");
			#endif
			#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
			pinfo->atscscantimestart =
			MsOS_GetSystemTime();
			#else
			pinfo->atscscantimestart =
			piodata->sys.getsystemtimems();
			#endif
			return DMD_ATSC_CHECKING;

		} else {
			if (!ioctl(piodata,
			DMD_ATSC_HAL_CMD_AGCLOCK, NULL))
				timeout = pInit->vsbagclockchecktime;
			else
				timeout = pInit->vsbprelockchecktime;

			#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
			if ((MsOS_GetSystemTime()-
			pinfo->atscscantimestart) < timeout)
			#else
			if ((piodata->sys.getsystemtimems()
			-pinfo->atscscantimestart) < timeout)
			#endif
			{
				return DMD_ATSC_CHECKING;
			}
		}
	}
	return DMD_ATSC_UNLOCK;
}
}

static enum DMD_ATSC_LOCK_STATUS _qam_checklock(void)
{
	u8 (*ioctl)(struct ATSC_IOCTL_DATA *pdata,
	enum DMD_ATSC_HAL_COMMAND eCmd, void *pPara);
	u16 timeout;

	struct DMD_ATSC_RESDATA *pres = piodata->pres;
	struct DMD_ATSC_INITDATA *pInit = &(pres->sdmd_atsc_initdata);
	struct DMD_ATSC_INFO *pinfo = &(pres->sdmd_atsc_info);

	ioctl	=	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd;

	if (ioctl(piodata, DMD_ATSC_HAL_CMD_QAM_MAIN_LOCK, NULL))	{
		pinfo->atsclockstatus |=	DMD_ATSC_LOCK_QAM_MAIN_LOCK;
		#if	DMD_ATSC_UTOPIA_EN ||	DMD_ATSC_UTOPIA2_EN
		pinfo->atscfeclocktime	=	MsOS_GetSystemTime();
		#else
		pinfo->atscfeclocktime = piodata->sys.getsystemtimems();
		#endif
		return DMD_ATSC_LOCK;

	} else {
		#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
		if ((pinfo->atsclockstatus & DMD_ATSC_LOCK_QAM_MAIN_LOCK)
		&& ((MsOS_GetSystemTime()-pinfo->atscfeclocktime) < 100))
		#else
		if ((pinfo->atsclockstatus & DMD_ATSC_LOCK_QAM_MAIN_LOCK)
	&& ((piodata->sys.getsystemtimems()-pinfo->atscfeclocktime) < 100))
		#endif
		{
			return DMD_ATSC_LOCK;

		} else
			pinfo->atsclockstatus &=
			(~DMD_ATSC_LOCK_QAM_MAIN_LOCK);

		if (pinfo->atsclockstatus & DMD_ATSC_LOCK_QAM_PRE_LOCK) {
			#if	DMD_ATSC_UTOPIA_EN ||	DMD_ATSC_UTOPIA2_EN
			if ((MsOS_GetSystemTime()-pinfo->atscscantimestart)
				< pInit->qammainlockchecktime)
			#else
			if ((piodata->sys.getsystemtimems()-
		pinfo->atscscantimestart) < pInit->qammainlockchecktime)
			#endif
			{
				#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
				if (ioctl(piodata, DMD_ATSC_HAL_CMD_AGCLOCK,
				NULL) || (MsOS_GetSystemTime()-
				pinfo->atscscantimestart) <
				pInit->qamagclockchecktime)
					return DMD_ATSC_CHECKING;
				#else
				if (ioctl(piodata, DMD_ATSC_HAL_CMD_AGCLOCK,
				NULL) || (piodata->sys.getsystemtimems()-
				pinfo->atscscantimestart) <
				pInit->qamagclockchecktime)
					return DMD_ATSC_CHECKING;
				#endif
			}
		} else {
			if (ioctl(piodata,
				DMD_ATSC_HAL_CMD_QAM_PRELOCK, NULL)) {
				pinfo->atsclockstatus |=
				DMD_ATSC_LOCK_QAM_PRE_LOCK;
				#ifdef MS_DEBUG
				if (dmd_atsc_dbglevel >=
					DMD_ATSC_DBGLV_DEBUG)
					PRINT("DMD_ATSC_LOCK_QAM_PRE_LOCK\n");
				#endif
				#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
				pinfo->atscscantimestart =
				MsOS_GetSystemTime();
				#else
				pinfo->atscscantimestart =
				piodata->sys.getsystemtimems();
				#endif
				return DMD_ATSC_CHECKING;

			} else {
				if (!ioctl(piodata,
					DMD_ATSC_HAL_CMD_AGCLOCK, NULL))
					timeout	= pInit->qamagclockchecktime;
				else
					timeout = pInit->qamprelockchecktime;

				#if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
				if ((MsOS_GetSystemTime() -
				pinfo->atscscantimestart) < timeout)
				#else
				if ((piodata->sys.getsystemtimems()-
				pinfo->atscscantimestart) < timeout)
				#endif
				{
					return DMD_ATSC_CHECKING;
				}
			}
		}
		return DMD_ATSC_UNLOCK;
	}
}

#if	defined	MTK_PROJECT

static u32	_getsystemtimems(void)
{
	u32 CurrentTime = 0;

	CurrentTime = x_os_get_sys_tick() * x_os_drv_get_tick_period();

	return CurrentTime;
}

static void	_delayms(u32	ms)
{
	mcDELAY_US(ms*1000);
}

static u8 _createmutex(u8 enable)
{
	if (enable) {
		if (_s32DMD_ATSC_Mutex ==	-1) {
			if (x_sema_create(&_s32DMD_ATSC_Mutex,
			X_SEMA_TYPE_MUTEX, X_SEMA_STATE_UNLOCK) != OSR_OK) {
				PRINT("%s: x_sema_create Fail!\n",
				__func__);
				return FALSE;
			}
		}
	}	else {
		x_sema_delete(_s32DMD_ATSC_Mutex);

		_s32DMD_ATSC_Mutex = -1;
	}

	return TRUE;
}

static void	_lockdmd(u8 enable)
{
	if (enable)
		x_sema_lock(_s32DMD_ATSC_Mutex,	X_SEMA_OPTION_WAIT);
	else
		x_sema_unlock(_s32DMD_ATSC_Mutex);
}

#elif	DVB_STI


static u8 _createmutex(u8 enable)
{
	return TRUE;
}

static void	_lockdmd(u8 enable)
{

}


static u32	_getsystemtimems(void)
{
	struct timespec			ts;

	getrawmonotonic(&ts);
	return ts.tv_sec *	1000 +	ts.tv_nsec/1000000;
}

//-------------------------------------------------
/* Delay for u32Us microseconds */
//-------------------------------------------------
//ckang
static void	_delayms(u32	ms)
{
	mtk_demod_delay_us(ms * 1000UL);
}

#elif	!DMD_ATSC_UTOPIA_EN	&& DMD_ATSC_UTOPIA2_EN

#ifdef MSOS_TYPE_LINUX_KERNEL
static u8 _sw_i2c_readbytes(u16 busnumslaveid,
u8 u8addrcount, u8 *pu8addr, u16 size, u8 *pdata_8)
{
	if (mdrv_sw_iic_writebytes !=	_NULL_SWI2C_FP_ATSC) {
		if (mdrv_sw_iic_writebytes((busnumslaveid>>8)&0x0F,
		busnumslaveid, u8addrcount, pu8addr, size, pdata_8) != 0)
			return FALSE;
		else
			return	TRUE;
	} else {
		PRINT("LINK	ERROR:
		I2c functionis missing and please check it \n");

		return FALSE;
	}
}

static u8 _sw_i2c_readbytes(u16 busnumslaveid,
u8 addrnum, u8 *paddr, u16 size, u8 *pdata_8)
{
	if (mdrv_sw_iic_readbytes	!= _NULL_SWI2C_FP_ATSC) {
		if (mdrv_sw_iic_readbytes((busnumslaveid>>8)&0x0F,
		busnumslaveid, addrnum, paddr, size, pdata_8) != 0)
			return FALSE;
		else
			return	TRUE;
	} else {
		PRINT("LINK	ERROR:
	I2c function is missing and please check it\n");

		return FALSE;
	}
}

static u8 _hw_i2c_writebytes(u16 busnumslaveid,
u8 u8addrcount, u8 *pu8addr, u16 size, u8 *pdata_8)
{
	if (mdrv_hw_iic_writebytes != _null_hwi2c_fp_atsc) {
		if (mdrv_hw_iic_writebytes((busnumslaveid>>12)&0x03,
		busnumslaveid, u8addrcount, pu8addr, size, pdata_8) != 0)
			return FALSE;
		else
			return	TRUE;
	}	else {
		PRINT("LINK	ERROR:
	I2c function is missing and please check it\n");

		return FALSE;
	}
}

static u8 _hw_i2c_readbytes(u16 busnumslaveid,
u8 addrnum, u8 *paddr, u16 size, u8 *pdata_8)
{
	if (mdrv_hw_iic_readbytes != _null_hwi2c_fp_atsc) {
		if (mdrv_hw_iic_readbytes((busnumslaveid>>12)&0x03,
		busnumslaveid, addrnum, paddr, size, pdata_8) != 0)
			return FALSE;
		else
			return	TRUE;
	} else {
		PRINT("LINK	ERROR:
	I2c function is missing and please check it\n");

		return FALSE;
	}
}
#endif

#endif

static void _updateiodata(u8 id, struct DMD_ATSC_RESDATA *pres)
{
	#if !DMD_ATSC_UTOPIA_EN && DMD_ATSC_UTOPIA2_EN
	#ifdef MSOS_TYPE_LINUX_KERNEL
	//u8I2CSlaveBus bit7 0 : old format 1 : new format
	//u8I2CSlaveBus bit6 0 : SW	I2C 1 : HW I2C
	//u8I2CSlaveBus bit4&5 HW I2C port number

	if ((pres->sdmd_atsc_initdata.u8I2CSlaveBus&0x80)	== 0x80) {
		if ((pres->sdmd_atsc_initdata.u8I2CSlaveBus&0x40) ==
			0x40) {
			pres->sdmd_atsc_initdata.I2C_WriteBytes =
			_hw_i2c_writebytes;
			pres->sdmd_atsc_initdata.I2C_ReadBytes =
			_hw_i2c_readbytes;
		}	else {
			pres->sdmd_atsc_initdata.I2C_WriteBytes =
			_sw_i2c_readbytes;
			pres->sdmd_atsc_initdata.I2C_ReadBytes =
			_sw_i2c_readbytes;
		}
	}
	#endif
	#endif //	#if	!DMD_ATSC_UTOPIA_EN	&& DMD_ATSC_UTOPIA2_EN

	piodata->id	=	id;
	piodata->pres	=	pres;

	piodata->pextchipchnum = pDMD_ATSC_EXT_CHIP_CH_NUM + (id>>4);
	piodata->pextchipchid	 = pDMD_ATSC_EXT_CHIP_CH_ID	 + (id&0x0F);
	piodata->pextchipi2cch = pDMD_ATSC_EXT_CHIP_I2C_CH + (id>>4);
}

//-----------------------------------------------------
/*Global Functions*/
//-----------------------------------------------------

#ifdef UTPA2
u8 _mdrv_dmd_atsc_setdbglevel(enum DMD_ATSC_DBGLV dbglevel)
#else
u8 mdrv_dmd_atsc_setdbglevel(enum DMD_ATSC_DBGLV dbglevel)
#endif
{
	dmd_atsc_dbglevel = dbglevel;

	return TRUE;
}

#ifdef UTPA2
struct DMD_ATSC_INFO *_mdrv_dmd_atsc_getinfo(void)
#else
struct DMD_ATSC_INFO *mdrv_dmd_atsc_getinfo(void)
#endif
{
	psdmd_atsc_resdata->sdmd_atsc_info.version = 0;

	return &(psdmd_atsc_resdata->sdmd_atsc_info);
}

#if	DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN

///////////////////////////////////////////////////
/*		SINGLE DEMOD API		 */
///////////////////////////////////////////////////

u8 mdrv_dmd_atsc_init(struct DMD_ATSC_INITDATA *pdmd_atsc_initdata,
u32 initdatalen)
{
	return mdrv_dmd_atsc_md_init(0,	pdmd_atsc_initdata,	initdatalen);
}

u8 mdrv_dmd_atsc_exit(void)
{
	return mdrv_dmd_atsc_md_exit(0);
}

u32 mdrv_dmd_atsc_getconfig(struct DMD_ATSC_INITDATA *psdmd_atsc_initdata)
{
	return mdrv_dmd_atsc_md_getconfig(0, psdmd_atsc_initdata);
}

u8 mdrv_dmd_atsc_set_digrf_freq(u32	rf_freq)
{
	return mdrv_dmd_atsc_md_set_digrf_freq(0,	rf_freq);
}

u8 mdrv_dmd_atsc_setconfig(enum DMD_ATSC_DEMOD_TYPE etype, u8 benable)
{
	return mdrv_dmd_atsc_md_setconfig(0, etype,	benable);
}

u8 mdrv_dmd_atsc_setreset(void)
{
	return mdrv_dmd_atsc_md_setreset(0);
}

u8 mdrv_dmd_atsc_set_qam_sr(
enum DMD_ATSC_DEMOD_TYPE etype, u16 symbol_rate)
{
	return mdrv_dmd_atsc_md_set_qam_sr(0,	etype, symbol_rate);
}

u8 mdrv_dmd_atsc_setactive(u8 benable)
{
	return mdrv_dmd_atsc_md_setactive(0, benable);
}

#if	DMD_ATSC_STR_EN
u32 mdrv_dmd_atsc_setpowerstate(enum EN_POWER_MODE powerstate)
{
	return mdrv_dmd_atsc_md_setpowerstate(0, powerstate);
}
#endif

enum DMD_ATSC_LOCK_STATUS mdrv_dmd_atsc_getlock(
enum DMD_ATSC_GETLOCK_TYPE etype)
{
	return mdrv_dmd_atsc_md_getlock(0, etype);
}

enum DMD_ATSC_DEMOD_TYPE mdrv_dmd_atsc_getmodulationmode(void)
{
	return mdrv_dmd_atsc_md_getmodulationmode(0);
}

u8 mdrv_dmd_atsc_getsignalstrength(u16 *strength)
{
	return mdrv_dmd_atsc_md_getsignalstrength(0, strength);
}

enum DMD_ATSC_SIGNAL_CONDITION mdrv_dmd_atsc_getsignalquality(void)
{
	return mdrv_dmd_atsc_md_getsignalquality(0);
}

u8 mdrv_dmd_atsc_getsnrpercentage(void)
{
	return mdrv_dmd_atsc_md_getsnrpercentage(0);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_atsc_get_qam_snr(float	*f_snr)
#else
u8 mdrv_dmd_atsc_get_qam_snr(struct DMD_ATSC_SNR_DATA	*f_snr)
#endif
{
	return mdrv_dmd_atsc_md_get_qam_snr(0, f_snr);
}

u8 mdrv_dmd_atsc_read_ucpkt_err(u16	*packeterr)
{
	return mdrv_dmd_atsc_md_read_ucpkt_err(0,	packeterr);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_atsc_getpreviterbiber(float *ber)
#else
u8 mdrv_dmd_atsc_getpreviterbiber(struct DMD_ATSC_BER_DATA *ber)
#endif
{
	return mdrv_dmd_atsc_md_getpreviterbiber(0,	ber);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_atsc_getpostviterbiber(float	*ber)
#else
u8 mdrv_dmd_atsc_getpostviterbiber(struct DMD_ATSC_BER_DATA	*ber)
#endif
{
	return mdrv_dmd_atsc_md_getpostviterbiber(0, ber);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_atsc_readfrequencyoffset(s16 *cfo)
#else
u8 mdrv_dmd_atsc_readfrequencyoffset(DMD_ATSC_CFO_DATA *cfo)
#endif
{
	return mdrv_dmd_atsc_md_readfrequencyoffset(0, cfo);
}

u8 mdrv_dmd_atsc_setserialcontrol(u8 tsconfig_data)
{
	return mdrv_dmd_atsc_md_setserialcontrol(0,	tsconfig_data);
}

u8 mdrv_dmd_atsc_iic_bypass_mode(u8 benable)
{
	return mdrv_dmd_atsc_md_iic_bypass_mode(0, benable);
}

u8 mdrv_dmd_atsc_switch_sspi_gpio(u8 benable)
{
	return mdrv_dmd_atsc_md_switch_sspi_gpio(0,	benable);
}

u8 mdrv_dmd_atsc_gpio_get_level(u8 pin_8,	u8 *blevel)
{
	return mdrv_dmd_atsc_md_gpio_get_level(0,	pin_8, blevel);
}

u8 mdrv_dmd_atsc_gpio_set_level(u8 pin_8,	u8 blevel)
{
	return mdrv_dmd_atsc_md_gpio_set_level(0,	pin_8, blevel);
}

u8 mdrv_dmd_atsc_gpio_out_enable(u8 pin_8, u8 benableout)
{
	return mdrv_dmd_atsc_md_gpio_out_enable(0, pin_8,	benableout);
}

u8 mdrv_dmd_atsc_doiqswap(u8 bis_qpad)
{
	return mdrv_dmd_atsc_md_doiqswap(0,	bis_qpad);
}

u8 mdrv_dmd_atsc_getreg(u16	addr_16, u8 *pdata_8)
{
	return mdrv_dmd_atsc_md_getreg(0,	addr_16, pdata_8);
}

u8 mdrv_dmd_atsc_setreg(u16	addr_16, u8 data_8)
{
	return mdrv_dmd_atsc_md_setreg(0,	addr_16, data_8);
}

////////////////////////////////////////////
///			 BACKWARD	COMPATIBLE API	 ///
////////////////////////////////////////////

#ifndef	UTPA2
u8 mdrv_dmd_atsc_sel_dmd(enum eDMD_SEL eDMD_NUM)
{
	return TRUE;
}

u8 mdrv_dmd_atsc_loadfw(enum eDMD_SEL DMD_NUM)
{
	return TRUE;
}

u8 mdrv_dmd_atsc_setvsbmode(void)
{
	return mdrv_dmd_atsc_md_setconfig(0, DMD_ATSC_DEMOD_ATSC_VSB,	TRUE);
}

u8 mdrv_dmd_atsc_set256qammode(void)
{
	return mdrv_dmd_atsc_md_setconfig(0, DMD_ATSC_DEMOD_ATSC_256QAM, TRUE);
}

u8 mdrv_dmd_atsc_set64qammode(void)
{
	return mdrv_dmd_atsc_md_setconfig(0, DMD_ATSC_DEMOD_ATSC_64QAM,	TRUE);
}
#endif //	#ifndef	UTPA2

#endif //	#if	DMD_ATSC_UTOPIA_EN ||	DMD_ATSC_UTOPIA2_EN

////////////////////////////////////////////////
///				 MULTI DEMOD API			 ///
////////////////////////////////////////////////

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_init(u8 id,
struct DMD_ATSC_INITDATA *pdmd_atsc_initdata, u32 initdatalen)
#else
u8 mdrv_dmd_atsc_md_init(u8 id,
struct DMD_ATSC_INITDATA *pdmd_atsc_initdata, u32 initdatalen)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	#if	defined(MSOS_TYPE_LINUX_KERNEL) &&\
	defined(CONFIG_CC_IS_CLANG) && defined(CONFIG_MSTAR_CHIP)
	const struct kernel_symbol *sym;
	#endif
	u8 brfagctristateenable = 0;
	#if	DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
	u64/*MS_PHY*/ phynonpmbanksize;
	#endif

	if (!(pres->sdmd_atsc_pridata.binit)) {
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		if (sizeof(struct DMD_ATSC_INITDATA) == initdatalen) {
			MEMCPY(&(pres->sdmd_atsc_initdata),
			 pdmd_atsc_initdata, initdatalen);
		} else {
			DMD_DBG(
	PRINT("mdrv_dmd_atsc_init input data structure incorrect\n"));
			return FALSE;
		}
		#else
		MEMCPY(&(pres->sdmd_atsc_initdata),
		pdmd_atsc_initdata, sizeof(struct DMD_ATSC_INITDATA));
		#endif
	} else {
		DMD_DBG(PRINT("mdrv_dmd_atsc_init more than once\n"));

		return TRUE;
	}

	#if	defined	MTK_PROJECT
	piodata->sys.getsystemtimems	=	_getsystemtimems;
	piodata->sys.delayms			=	_delayms;
	piodata->sys.createmutex		=	_createmutex;
	piodata->sys.lockdmd			=	_lockdmd;
	#elif	DVB_STI
	piodata->sys.getsystemtimems	=	_getsystemtimems;
	piodata->sys.delayms			=	_delayms;
	piodata->sys.createmutex		=	_createmutex;
	piodata->sys.lockdmd			=	_lockdmd;
	#elif	DMD_ATSC_3PARTY_EN
	// Please initial mutex, timer and MSPI function pointer for 3 perty
	// piodata->sys.getsystemtimems	 = ;
	// piodata->sys.delayms			 = ;
	// piodata->sys.createmutex		 = ;
	// piodata->sys.lockdmd			 = ;
	// piodata->sys.mspi_init_ext	 = ;
	// piodata->sys.MSPI_SlaveEnable = ;
	// piodata->sys.mspi_write		 = ;
	// piodata->sys.mspi_read		 = ;
	#elif	!DMD_ATSC_UTOPIA2_EN
	piodata->sys.delayms			=	MsOS_DelayTask;
	piodata->sys.mspi_init_ext	=	MDrv_MSPI_Init_Ext;
	piodata->sys.MSPI_SlaveEnable	=	MDrv_MSPI_SlaveEnable;
	piodata->sys.mspi_write		=	MDrv_MSPI_Write;
	piodata->sys.mspi_read		=	MDrv_MSPI_Read;
	#endif

	#if	DMD_ATSC_UTOPIA_EN && !DMD_ATSC_UTOPIA2_EN
	if (_s32DMD_ATSC_Mutex == -1) {
		_s32DMD_ATSC_Mutex = MsOS_createmutex(
		E_MSOS_FIFO, "Mutex DMD ATSC", MSOS_PROCESS_SHARED);
		if (_s32DMD_ATSC_Mutex == -1) {
			DMD_DBG(
			PRINT("mdrv_dmd_atsc_init Create Mutex Fail\n"));
			return FALSE;
		}
	}
	#elif (!DMD_ATSC_UTOPIA_EN && \
	!DMD_ATSC_UTOPIA2_EN && DMD_ATSC_MULTI_THREAD_SAFE)
	if (!piodata->sys.createmutex(TRUE)) {
		DMD_DBG(PRINT("mdrv_dmd_atsc_init Create Mutex Fail\n"));
		return FALSE;
	}
	#endif

	#ifdef MS_DEBUG
	if (dmd_atsc_dbglevel >= DMD_ATSC_DBGLV_INFO)
		PRINT("mdrv_dmd_atsc_init\n");

	#endif

	#if	DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN
	if (!MDrv_MMIO_GetBASE(&
		(pres->sdmd_atsc_pridata.virtdmdbaseaddr),
		&phynonpmbanksize, MS_MODULE_PM)) {
		#ifdef MS_DEBUG
		PRINT("HAL_DMD_RegInit failure to get MS_MODULE_PM\n");
		#endif

		return FALSE;
	}
	#endif

	#if	DVB_STI
	pres->sdmd_atsc_pridata.virtdmdbaseaddr = (
	((size_t/*MS_VIRT*/ *)(pres->sdmd_atsc_initdata.dmd_atsc_initext)));
	#endif

	pres->sdmd_atsc_pridata.binit = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();

	#if	DMD_ATSC_STR_EN
	if (pres->sdmd_atsc_pridata.elaststate == 0) {
		pres->sdmd_atsc_pridata.elaststate = E_POWER_RESUME;

		pres->sdmd_atsc_pridata.elasttype = DMD_ATSC_DEMOD_ATSC_VSB;
		pres->sdmd_atsc_pridata.symrate = 10762;
	}
	#else
	pres->sdmd_atsc_pridata.elasttype  = DMD_ATSC_DEMOD_ATSC_VSB;
	pres->sdmd_atsc_pridata.symrate = 10762;
	#endif

	#if !DMD_ATSC_3PARTY_EN
	#if defined(MSOS_TYPE_LINUX_KERNEL)	&&\
	defined(CONFIG_CC_IS_CLANG) && defined(CONFIG_MSTAR_CHIP)
	#ifdef CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
	sym = find_symbol("hal_intern_atsc_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		hal_intern_atsc_ioctl_cmd =
		(u8 (*)(struct ATSC_IOCTL_DATA, enum DMD_ATSC_HAL_COMMAND,
	void *))((unsigned long)offset_to_ptr(&sym->value_offset));
	else
		hal_intern_atsc_ioctl_cmd = _null_ioctl_fp_atsc;
	sym = find_symbol("hal_extern_atsc_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		hal_extern_atsc_ioctl_cmd = (IOCTL)(
		(unsigned long)offset_to_ptr(&sym->value_offset));
	else
		hal_extern_atsc_ioctl_cmd = _null_ioctl_fp_atsc;
	sym = find_symbol("mdrv_sw_iic_writebytes", NULL,	NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_sw_iic_writebytes =
		(u8 (*)(u8, u8, u8, u8 *,
	u16, u8 *))((unsigned long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_sw_iic_writebytes	=	_NULL_SWI2C_FP_ATSC;
	sym = find_symbol("mdrv_sw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_sw_iic_readbytes =
		(u8 (*)(u8, u8, u8, u8 *,
	u16, u8 *))((unsigned long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_sw_iic_readbytes = _NULL_SWI2C_FP_ATSC;
	sym = find_symbol("mdrv_hw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_hw_iic_writebytes =
		(u8 (*)(u8, u8, u8, u8 *,
	u16, u8 *))((unsigned long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_hw_iic_writebytes = _null_hwi2c_fp_atsc;
	sym = find_symbol("mdrv_hw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_hw_iic_readbytes = (u8 (*)(u8, u8, u8,
u8 *, u16, u8 *))((unsigned long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_hw_iic_readbytes = _null_hwi2c_fp_atsc;
	#else	// #ifdef	CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
	sym = find_symbol("hal_intern_atsc_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym	!= NULL) {
		hal_intern_atsc_ioctl_cmd = (u8 (*)(struct ATSC_IOCTL_DATA,
	enum DMD_ATSC_HAL_COMMAND, void *))(sym->value);

	} else {
		hal_intern_atsc_ioctl_cmd = _null_ioctl_fp_atsc;
	}
	sym = find_symbol("hal_extern_atsc_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym	!= NULL) {
		hal_extern_atsc_ioctl_cmd = (u8 (*)(struct ATSC_IOCTL_DATA,
	enum DMD_ATSC_HAL_COMMAND, void *))(sym->value);

	} else {
		hal_extern_atsc_ioctl_cmd = _null_ioctl_fp_atsc;
	}
	sym = find_symbol("mdrv_sw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym	!= NULL) {
		mdrv_sw_iic_writebytes = (u8 (*)(u8, u8, u8,
	u8 *, u16, u8 *))(sym->value);

	} else {
		mdrv_sw_iic_writebytes	=	_NULL_SWI2C_FP_ATSC;
	}
	sym = find_symbol("mdrv_sw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym	!= NULL) {
		mdrv_sw_iic_readbytes = (u8 (*)(u8, u8, u8,
	u8 *, u16, u8 *))sym->value;
	} else {
		mdrv_sw_iic_readbytes = _NULL_SWI2C_FP_ATSC;
	}
	sym = find_symbol("mdrv_hw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_hw_iic_writebytes = (u8 (*)(u8, u8,
		u8, u8 *, u16, u8 *))(sym->value);
	else
		mdrv_hw_iic_writebytes = _null_hwi2c_fp_atsc;
	sym = find_symbol("mdrv_hw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_hw_iic_readbytes = (u8 (*)(u8, u8,
	u8, u8 *, u16, u8 *))sym->value;
	else
		mdrv_hw_iic_readbytes = _null_hwi2c_fp_atsc;
	#endif // #ifdef CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
	#endif
	#endif

	#if !DVB_STI
	if (pres->sdmd_atsc_initdata.bisextdemod) {
		if (hal_extern_atsc_ioctl_cmd	== _null_ioctl_fp_atsc)
			pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd =
			_default_ioctl_cmd;
		else
			pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd =
			hal_extern_atsc_ioctl_cmd;
	} else {
		if (hal_intern_atsc_ioctl_cmd	== _null_ioctl_fp_atsc)
			pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd =
			_default_ioctl_cmd;
		else {
			pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd =
			hal_intern_atsc_ioctl_cmd;
		}
	}
	#else
	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd =
			hal_intern_atsc_ioctl_cmd;
	#endif

	#ifndef	MSOS_TYPE_LINUX_KERNEL
	#if	DMD_ATSC_UTOPIA_EN ||	DMD_ATSC_UTOPIA2_EN
	if (hal_extern_atsc_ioctl_cmd_0	== _null_ioctl_fp_atsc)
		EXT_ATSC_IOCTL[0] = _default_ioctl_cmd;
	if (hal_extern_atsc_ioctl_cmd_1	== _null_ioctl_fp_atsc)
		EXT_ATSC_IOCTL[1] = _default_ioctl_cmd;
	if (hal_extern_atsc_ioctl_cmd_2	== _null_ioctl_fp_atsc)
		EXT_ATSC_IOCTL[2] = _default_ioctl_cmd;
	if (hal_extern_atsc_ioctl_cmd_3	== _null_ioctl_fp_atsc)
		EXT_ATSC_IOCTL[3] = _default_ioctl_cmd;
	#endif
	#endif //	#ifndef	MSOS_TYPE_LINUX_KERNEL

	_updateiodata(id, pres);

	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_INITCLK, &brfagctristateenable);

	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_DOWNLOAD, NULL);

	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_TS_INTERFACE_CONFIG, NULL);

	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_FWVERSION, NULL);
	DMD_UNLOCK();

	return TRUE;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_exit(u8 id)
#else
u8 mdrv_dmd_atsc_md_exit(u8 id)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (pres->sdmd_atsc_pridata.binit	== FALSE)
		return bret;
	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd
	(piodata, DMD_ATSC_HAL_CMD_EXIT, NULL);

	pres->sdmd_atsc_pridata.binit	=	FALSE;
	DMD_UNLOCK();

	#if	DMD_ATSC_UTOPIA_EN &&	!DMD_ATSC_UTOPIA2_EN
	MsOS_DeleteMutex(_s32DMD_ATSC_Mutex);

	_s32DMD_ATSC_Mutex = -1;
	#elif (!DMD_ATSC_UTOPIA_EN && \
	!DMD_ATSC_UTOPIA2_EN && DMD_ATSC_MULTI_THREAD_SAFE)
	piodata->sys.createmutex(FALSE);
	#endif

	return bret;
}

#ifdef UTPA2
u32 _mdrv_dmd_atsc_md_getconfig(
u8 id, struct DMD_ATSC_INITDATA *psdmd_atsc_initdata)
#else
u32 mdrv_dmd_atsc_md_getconfig(
u8 id, struct DMD_ATSC_INITDATA *psdmd_atsc_initdata)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + id;

	MEMCPY(psdmd_atsc_initdata,
	&(pres->sdmd_atsc_initdata), sizeof(struct DMD_ATSC_INITDATA));

	return UTOPIA_STATUS_SUCCESS;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_setconfig(
u8 id, enum DMD_ATSC_DEMOD_TYPE etype, u8 benable)
#else
u8 mdrv_dmd_atsc_md_setconfig(
u8 id, enum DMD_ATSC_DEMOD_TYPE etype, u8 benable)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_SOFTRESET, NULL);

	if (benable) {
		switch (etype) {
		case DMD_ATSC_DEMOD_ATSC_VSB:
			pres->sdmd_atsc_pridata.elasttype =
			DMD_ATSC_DEMOD_ATSC_VSB;
			pres->sdmd_atsc_pridata.symrate = 10762;
			bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
			piodata, DMD_ATSC_HAL_CMD_SETVSBMODE, NULL);
		break;
		case DMD_ATSC_DEMOD_ATSC_64QAM:
			pres->sdmd_atsc_pridata.elasttype =
			DMD_ATSC_DEMOD_ATSC_64QAM;
			pres->sdmd_atsc_pridata.symrate = 5056;
			bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
			piodata, DMD_ATSC_HAL_CMD_SET64QAMMODE, NULL);
		break;
		case DMD_ATSC_DEMOD_ATSC_256QAM:
			pres->sdmd_atsc_pridata.elasttype =
			DMD_ATSC_DEMOD_ATSC_256QAM;
			pres->sdmd_atsc_pridata.symrate = 5360;
			bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
			piodata, DMD_ATSC_HAL_CMD_SET256QAMMODE, NULL);
		break;
		default:
			pres->sdmd_atsc_pridata.elasttype = DMD_ATSC_DEMOD_NULL;
			pres->sdmd_atsc_pridata.symrate = 0;
			bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
			piodata, DMD_ATSC_HAL_CMD_SETMODECLEAN, NULL);
		break;
		}
	}

	#if	DMD_ATSC_UTOPIA_EN ||	DMD_ATSC_UTOPIA2_EN
	pres->sdmd_atsc_info.atscscantimestart =
	MsOS_GetSystemTime();
	#else
	pres->sdmd_atsc_info.atscscantimestart =
	piodata->sys.getsystemtimems();
	#endif
	pres->sdmd_atsc_info.atsclockstatus = 0;
	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_setreset(u8 id)
#else
u8 mdrv_dmd_atsc_md_setreset(u8 id)
#endif
{
	return TRUE;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_set_qam_sr(
u8 id, enum DMD_ATSC_DEMOD_TYPE etype, u16 symbol_rate)
#else
u8 mdrv_dmd_atsc_md_set_qam_sr(
u8 id, enum DMD_ATSC_DEMOD_TYPE etype, u16 symbol_rate)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	pres->sdmd_atsc_pridata.elasttype	 = etype;
	pres->sdmd_atsc_pridata.symrate = symbol_rate;

	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_SET_QAM_SR, NULL);
	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_setactive(u8 id,	u8 benable)
#else
u8 mdrv_dmd_atsc_md_setactive(u8 id, u8 benable)
#endif
{
	return TRUE;
}

#if	DMD_ATSC_STR_EN
#ifdef UTPA2
u32 _mdrv_dmd_atsc_md_setpowerstate(u8 id,
enum EN_POWER_MODE powerstate)
#else
u32 mdrv_dmd_atsc_md_setpowerstate(u8 id,
enum EN_POWER_MODE powerstate)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u32 u32return_tmp = UTOPIA_STATUS_FAIL;

	if (powerstate	== E_POWER_SUSPEND) {
		pres->sdmd_atsc_pridata.bdownloaded = FALSE;
		pres->sdmd_atsc_pridata.bisdtv =
		pres->sdmd_atsc_pridata.binit;
		pres->sdmd_atsc_pridata.elaststate = powerstate;

		if (pres->sdmd_atsc_pridata.binit) {
			#ifdef UTPA2
			_mdrv_dmd_atsc_md_exit(id);
			#else
			mdrv_dmd_atsc_md_exit(id);
			#endif
		}

		u32return_tmp = UTOPIA_STATUS_SUCCESS;
	} else if	(powerstate == E_POWER_RESUME) {
		if (pres->sdmd_atsc_pridata.elaststate ==
			E_POWER_SUSPEND) {
			if (pres->sdmd_atsc_pridata.bisdtv) {
				#ifdef UTPA2
				_mdrv_dmd_atsc_md_init(id,
				&(pres->sdmd_atsc_initdata),
				sizeof(struct DMD_ATSC_INITDATA));
				_mdrv_dmd_atsc_md_doiqswap(id,
				pres->sdmd_atsc_pridata.bis_qpad);
				#else
				mdrv_dmd_atsc_md_init(id,
				&(pres->sdmd_atsc_initdata),
				sizeof(struct DMD_ATSC_INITDATA));
				mdrv_dmd_atsc_md_doiqswap(id,
				pres->sdmd_atsc_pridata.bis_qpad);
				#endif

				if (pres->sdmd_atsc_pridata.elasttype !=
					DMD_ATSC_DEMOD_ATSC_VSB) {
					if (pres->sdmd_atsc_pridata.elasttype ==
						DMD_ATSC_DEMOD_ATSC_64QAM &&
				pres->sdmd_atsc_pridata.symrate == 5056) {
						#ifdef UTPA2
						_mdrv_dmd_atsc_md_setconfig(id,
				pres->sdmd_atsc_pridata.elasttype, TRUE);
						#else
						mdrv_dmd_atsc_md_setconfig(id,
				pres->sdmd_atsc_pridata.elasttype, TRUE);
						#endif
					} else if (
					pres->sdmd_atsc_pridata.elasttype ==
						DMD_ATSC_DEMOD_ATSC_256QAM &&
				pres->sdmd_atsc_pridata.symrate == 5360) {
						#ifdef UTPA2
						_mdrv_dmd_atsc_md_setconfig(id,
					pres->sdmd_atsc_pridata.elasttype,
						TRUE);
						#else
						mdrv_dmd_atsc_md_setconfig(id,
					pres->sdmd_atsc_pridata.elasttype,
						TRUE);
						#endif
					} else {
						#ifdef UTPA2
						_mdrv_dmd_atsc_md_set_qam_sr(id,
					pres->sdmd_atsc_pridata.elasttype,
					pres->sdmd_atsc_pridata.symrate);
						#else
						mdrv_dmd_atsc_md_set_qam_sr(id,
					pres->sdmd_atsc_pridata.elasttype,
					pres->sdmd_atsc_pridata.symrate);
						#endif
					}
				} else {
					#ifdef UTPA2
					_mdrv_dmd_atsc_md_setconfig(id,
				pres->sdmd_atsc_pridata.elasttype, TRUE);
					#else
					mdrv_dmd_atsc_md_setconfig(id,
				pres->sdmd_atsc_pridata.elasttype, TRUE);
					#endif
				}
			}

			pres->sdmd_atsc_pridata.elaststate = powerstate;

			u32return_tmp	=	UTOPIA_STATUS_SUCCESS;
		} else {
			PRINT("It is not suspended yet. We shouldn't resume\n");

			u32return_tmp	=	UTOPIA_STATUS_FAIL;
		}
	} else {
		PRINT("Do Nothing: %d\n", powerstate);

		u32return_tmp	=	UTOPIA_STATUS_FAIL;
	}

	return u32return_tmp;
}
#endif

#ifdef UTPA2
enum DMD_ATSC_LOCK_STATUS _mdrv_dmd_atsc_md_getlock(
u8 id, enum DMD_ATSC_GETLOCK_TYPE etype)
#else
enum DMD_ATSC_LOCK_STATUS mdrv_dmd_atsc_md_getlock(
u8 id, enum DMD_ATSC_GETLOCK_TYPE etype)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	enum DMD_ATSC_LOCK_STATUS status = DMD_ATSC_UNLOCK;

	if (!(pres->sdmd_atsc_pridata.binit))
		return DMD_ATSC_UNLOCK;
	DMD_LOCK();
	_updateiodata(id, pres);

	switch (etype)	{
	case DMD_ATSC_GETLOCK:
		switch (pres->sdmd_atsc_pridata.elasttype)	{
		case DMD_ATSC_DEMOD_ATSC_VSB:
		status = _vsb_checklock();
		break;
		case DMD_ATSC_DEMOD_ATSC_16QAM:
		case DMD_ATSC_DEMOD_ATSC_32QAM:
		case DMD_ATSC_DEMOD_ATSC_64QAM:
		case DMD_ATSC_DEMOD_ATSC_128QAM:
		case DMD_ATSC_DEMOD_ATSC_256QAM:
		status = _qam_checklock();
		break;
		default:
		status = DMD_ATSC_UNLOCK;
		break;
		}
	break;
	case DMD_ATSC_GETLOCK_VSB_AGCLOCK:
		status = (pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
		piodata, DMD_ATSC_HAL_CMD_AGCLOCK, NULL)) ?
		DMD_ATSC_LOCK : DMD_ATSC_UNLOCK;
		break;
	case DMD_ATSC_GETLOCK_VSB_PRELOCK:
		status = (pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
		piodata, DMD_ATSC_HAL_CMD_VSB_PRELOCK, NULL)) ?
		DMD_ATSC_LOCK : DMD_ATSC_UNLOCK;
		break;
	case DMD_ATSC_GETLOCK_VSB_FSYNCLOCK:
		status = (pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
		piodata, DMD_ATSC_HAL_CMD_VSB_FSYNC_LOCK, NULL)) ?
		DMD_ATSC_LOCK : DMD_ATSC_UNLOCK;
		break;
	case DMD_ATSC_GETLOCK_VSB_CELOCK:
		status = (pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
		piodata, DMD_ATSC_HAL_CMD_VSB_CE_LOCK, NULL)) ?
		DMD_ATSC_LOCK : DMD_ATSC_UNLOCK;
		break;
	case DMD_ATSC_GETLOCK_VSB_FECLOCK:
		status = (pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
		piodata, DMD_ATSC_HAL_CMD_VSB_FEC_LOCK, NULL)) ?
		DMD_ATSC_LOCK : DMD_ATSC_UNLOCK;
		break;
	case DMD_ATSC_GETLOCK_QAM_AGCLOCK:
		status = (pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
		piodata, DMD_ATSC_HAL_CMD_AGCLOCK, NULL)) ?
		DMD_ATSC_LOCK : DMD_ATSC_UNLOCK;
		break;
	case DMD_ATSC_GETLOCK_QAM_PRELOCK:
		status = (pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
		piodata, DMD_ATSC_HAL_CMD_QAM_PRELOCK, NULL)) ?
		DMD_ATSC_LOCK : DMD_ATSC_UNLOCK;
		break;
	case DMD_ATSC_GETLOCK_QAM_MAINLOCK:
		status = (pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
		piodata, DMD_ATSC_HAL_CMD_QAM_MAIN_LOCK, NULL)) ?
		DMD_ATSC_LOCK : DMD_ATSC_UNLOCK;
		break;
	default:
		status = DMD_ATSC_UNLOCK;
		break;
	}
	DMD_UNLOCK();

	return status;
}

#ifdef UTPA2
enum DMD_ATSC_DEMOD_TYPE _mdrv_dmd_atsc_md_getmodulationmode(u8 id)
#else
enum DMD_ATSC_DEMOD_TYPE mdrv_dmd_atsc_md_getmodulationmode(u8 id)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	enum DMD_ATSC_DEMOD_TYPE etype = DMD_ATSC_DEMOD_NULL;

	if (!(pres->sdmd_atsc_pridata.binit))
		return 0;
	DMD_LOCK();
	_updateiodata(id, pres);

	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_CHECK8VSB64_256QAM, &etype);

	DMD_UNLOCK();

	return etype;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_getsignalstrength(u8 id, u16 *strength)
#else
u8 mdrv_dmd_atsc_md_getsignalstrength(u8 id, u16	*strength)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_READIFAGC, strength);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
enum DMD_ATSC_SIGNAL_CONDITION _mdrv_dmd_atsc_md_getsignalquality(u8 id)
#else
enum DMD_ATSC_SIGNAL_CONDITION mdrv_dmd_atsc_md_getsignalquality(u8 id)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	enum DMD_ATSC_SIGNAL_CONDITION eSignal = DMD_ATSC_SIGNAL_NO;

	if (!(pres->sdmd_atsc_pridata.binit))
		return 0;
	DMD_LOCK();
	_updateiodata(id, pres);

	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_CHECKSIGNALCONDITION, &eSignal);

	DMD_UNLOCK();

	return eSignal;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_getsnrpercentage(u8 id)
#else
u8 mdrv_dmd_atsc_md_getsnrpercentage(u8 id)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 percentage = 0;

	if (!(pres->sdmd_atsc_pridata.binit))
		return 0;
	DMD_LOCK();
	_updateiodata(id, pres);

	pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_READSNRPERCENTAGE, &percentage);

	DMD_UNLOCK();

	return percentage;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_get_qam_snr(u8 id,
struct DMD_ATSC_SNR_DATA *f_snr)
#else
u8 mdrv_dmd_atsc_md_get_qam_snr(u8 id, float *f_snr)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_GET_QAM_SNR, f_snr);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_read_ucpkt_err(u8 id, u16	*packeterr)
#else
u8 mdrv_dmd_atsc_md_read_ucpkt_err(u8 id,	u16 *packeterr)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_READPKTERR, packeterr);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_getpreviterbiber(u8 id,
struct DMD_ATSC_BER_DATA *ber)
#else
u8 mdrv_dmd_atsc_md_getpreviterbiber(u8 id, float *ber)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_GETPREVITERBIBER, ber);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_getpostviterbiber(u8 id,
struct DMD_ATSC_BER_DATA *ber)
#else
u8 mdrv_dmd_atsc_md_getpostviterbiber(u8 id, float *ber)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_GETPOSTVITERBIBER, ber);
	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_readfrequencyoffset(
u8 id, struct DMD_ATSC_CFO_DATA *cfo)
#else
u8 mdrv_dmd_atsc_md_readfrequencyoffset(u8 id, s16 *cfo)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_READFREQUENCYOFFSET, cfo);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_setserialcontrol(u8 id, u8 tsconfig_data)
#else
u8 mdrv_dmd_atsc_md_setserialcontrol(u8 id,	u8 tsconfig_data)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	pres->sdmd_atsc_initdata.u1TsConfigByte_SerialMode = tsconfig_data;
	pres->sdmd_atsc_initdata.u1TsConfigByte_DataSwap = tsconfig_data >> 1;
	pres->sdmd_atsc_initdata.u1TsConfigByte_ClockInv = tsconfig_data >> 2;
	pres->sdmd_atsc_initdata.u5TsConfigByte_DivNum = tsconfig_data >> 3;

	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_TS_INTERFACE_CONFIG, NULL);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_iic_bypass_mode(u8 id,	u8 benable)
#else
u8 mdrv_dmd_atsc_md_iic_bypass_mode(u8 id, u8 benable)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(piodata,
	DMD_ATSC_HAL_CMD_IIC_BYPASS_MODE, &benable);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_switch_sspi_gpio(u8 id, u8 benable)
#else
u8 mdrv_dmd_atsc_md_switch_sspi_gpio(u8 id,	u8 benable)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_SSPI_TO_GPIO, &benable);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_gpio_get_level(u8 id, u8 pin_8, u8 *blevel)
#else
u8 mdrv_dmd_atsc_md_gpio_get_level(u8 id, u8 pin_8, u8 *blevel)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	struct DMD_ATSC_GPIO_PIN_DATA spin;
	u8 bret = TRUE;

	spin.pin_8 = pin_8;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_GPIO_GET_LEVEL, &spin);

	DMD_UNLOCK();

	*blevel = spin.blevel;

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_gpio_set_level(u8 id, u8 pin_8, u8 blevel)
#else
u8 mdrv_dmd_atsc_md_gpio_set_level(u8 id, u8 pin_8, u8 blevel)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	struct DMD_ATSC_GPIO_PIN_DATA spin;
	u8 bret = TRUE;

	spin.pin_8	=	pin_8;
	spin.blevel	=	blevel;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_GPIO_SET_LEVEL, &spin);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_gpio_out_enable(
u8 id, u8 pin_8, u8 benableout)
#else
u8 mdrv_dmd_atsc_md_gpio_out_enable(
u8 id, u8 pin_8, u8 benableout)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	struct DMD_ATSC_GPIO_PIN_DATA spin;
	u8 bret = TRUE;

	spin.pin_8	=	pin_8;
	spin.bisout	=	benableout;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(piodata,
	DMD_ATSC_HAL_CMD_GPIO_OUT_ENABLE, &spin);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_doiqswap(u8 id, u8 bis_qpad)
#else
u8 mdrv_dmd_atsc_md_doiqswap(u8 id, u8 bis_qpad)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(
	piodata, DMD_ATSC_HAL_CMD_DOIQSWAP, &bis_qpad);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_getreg(u8 id, u16 addr_16, u8 *pdata_8)
#else
u8 mdrv_dmd_atsc_md_getreg(u8 id, u16 addr_16, u8 *pdata_8)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	struct DMD_ATSC_REG_DATA reg;
	u8 bret = TRUE;

	reg.addr_16 = addr_16;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(piodata,
	DMD_ATSC_HAL_CMD_GET_REG, &reg);

	DMD_UNLOCK();

	*pdata_8 = reg.data_8;

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_setreg(u8 id, u16 addr_16, u8 data_8)
#else
u8 mdrv_dmd_atsc_md_setreg(u8 id, u16 addr_16, u8 data_8)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	struct DMD_ATSC_REG_DATA reg;
	u8 bret = TRUE;

	reg.addr_16 = addr_16;
	reg.data_8 = data_8;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(piodata,
	DMD_ATSC_HAL_CMD_SET_REG, &reg);

	DMD_UNLOCK();

	return bret;
}

#ifdef UTPA2
u8 _mdrv_dmd_atsc_md_set_digrf_freq(u8 id, u32 rf_freq)
#else
u8 mdrv_dmd_atsc_md_set_digrf_freq(u8 id, u32 rf_freq)
#endif
{
	struct DMD_ATSC_RESDATA *pres = psdmd_atsc_resdata + (id&0x0F);
	u8 bret = TRUE;

	if (!(pres->sdmd_atsc_pridata.binit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pres);

	bret = pres->sdmd_atsc_pridata.hal_dmd_atsc_ioctl_cmd(piodata,
	DMD_ATSC_HAL_CMD_SET_DIGRF_FREQ, &rf_freq);

	DMD_UNLOCK();

	return bret;
}

u8 _mdrv_dmd_atsc_md_get_info(struct dvb_frontend *fe,
			struct dtv_frontend_properties *p)
{
	u8 ret_tmp;
	u8 data1 = 0;
	u8 data2 = 0;
	u8 data3 = 0;
	u16 ifagc_tmp = 0;
	u16 ifagc = 0;
	u16 snr, err;
	s16 cfo;
	u32 postber;
	u8 bret;
	u8 fgdemodStatus = false;
	u8 fgagcstatus = false;
	enum DMD_ATSC_LOCK_STATUS atsclocksStatus = DMD_ATSC_NULL;
	struct DMD_ATSC_BER_DATA getber;
	struct DMD_ATSC_CFO_DATA cfoParameter;
	struct dtv_fe_stats *stats;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	/* SNR */
	snr = _mdrv_dmd_atsc_md_getsnrpercentage(0);
	snr = snr * 4; //steve DBG / 10;
	stats = &p->cnr;
	stats->len = 1;
	stats->stat[0].svalue = snr;
	stats->stat[0].scale = FE_SCALE_DECIBEL;

	PRINT("DTD_GetSignalSNR:%d\n", snr);

	/* Packet Error */
	ret_tmp = _mdrv_dmd_atsc_md_read_ucpkt_err(0, &err);
	stats = &p->block_error;
	stats->len = 1;
	stats->stat[0].uvalue = err;
	stats->stat[0].scale = FE_SCALE_COUNTER;
	PRINT("HELLO,e_get_type =TUNER_GET_TYPE_UEC\r\n");
	PRINT("UEC = %u\n", err);

	/* BER */
	ret_tmp = _mdrv_dmd_atsc_md_getpostviterbiber(0, &getber);
	if (getber.biterr <= 0)
		postber = 0;
	else
		postber = (getber.biterr * 10000) / (
		(u32)(getber.error_window)*(191488));
	postber = postber * 100;

	stats = &p->post_bit_error;
	stats->len = 1;
	stats->stat[0].uvalue = postber;
	stats->stat[0].scale = FE_SCALE_COUNTER;
	PRINT("HELLO,e_get_type =TUNER_GET_TYPE_BER\r\n");
	PRINT("error_window = %d\n", getber.biterr);
	PRINT("error_window = %d\n", getber.error_window);
	PRINT("postber = %d\n", postber);

	/* CFO */
	ret_tmp = _mdrv_dmd_atsc_md_readfrequencyoffset(0, &cfoParameter);
	p->frequency_offset = (s32)cfoParameter.rate;
	PRINT("HELLO,e_get_type =TUNER_GET_TYPE_CFO\r\n");
	PRINT("Carrier Frequency Offset = %d\n", cfoParameter.rate);

	/* Interleaving Mode */
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x0624, &data1);
	data1 = data1>>4;

	PRINT("Interleaving Mode = %d\n", data1);
	if (c->delivery_system == SYS_ATSC)
		p->interleaving = INTERLEAVING_NONE;
	else {
		if (data1 == 0)
			p->interleaving = INTERLEAVING_128_1_0;
		else if (data1 == 1)
			p->interleaving = INTERLEAVING_128_1_1;
		else if (data1 == 3)
			p->interleaving = INTERLEAVING_64_2;
		else if (data1 == 5)
			p->interleaving = INTERLEAVING_32_4;
		else if (data1 == 7)
			p->interleaving = INTERLEAVING_16_8;
		else if (data1 == 9)
			p->interleaving = INTERLEAVING_8_16;
		else if (data1 == 2)
			p->interleaving = INTERLEAVING_128_2;
		else if (data1 == 4)
			p->interleaving = INTERLEAVING_128_3;
		else if (data1 == 6)
			p->interleaving = INTERLEAVING_128_4;
		else
			p->interleaving = INTERLEAVING_128_1_0;
	}

	/* Lock Status */
	atsclocksStatus = _mdrv_dmd_atsc_md_getlock(0, DMD_ATSC_GETLOCK);
	if (atsclocksStatus == DMD_ATSC_LOCK)
		fgdemodStatus = TRUE;
	else if (atsclocksStatus == DMD_ATSC_UNLOCK)
		fgdemodStatus = FALSE;

	PRINT("HELLO,e_get_type =TUNER_GET_TYPE_DEMOD_LOCK_STATUS\r\n");
	PRINT("DEMOD Lock = %d\n", fgdemodStatus);

	/* AGC Lock Status */
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x2829, &data1);
	if (data1 == 0x01)
		fgagcstatus = TRUE;
	else
		fgagcstatus = FALSE;

	PRINT("HELLO,e_get_type =TUNER_GET_TYPE_AGC_STATUS\r\n");
	PRINT("DEMOD AGC Lock = %d\n", fgagcstatus);

	/* AGC Level */
	ret_tmp = _mdrv_dmd_atsc_md_setreg(0, 0x2822, 0x03);
	ret_tmp = _mdrv_dmd_atsc_md_setreg(0, 0x2805, 0x80);

	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x2824, &data1);
	ifagc_tmp = (u16)data1;
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x2825, &data1);
	ifagc_tmp |= (u16)data1<<8;
	ret_tmp = _mdrv_dmd_atsc_md_setreg(0, 0x2805, 0x00);

	ifagc_tmp = (ifagc_tmp>>8)&0xFF;
	ifagc = (~ifagc_tmp)&0xFF;

	PRINT("e_get_type =TUNER_GET_TYPE_AGC\r\n");
	PRINT("DTD AGC: %d\n", ifagc);

	/* Get FW Version */
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x20C4, &data1);
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x20CF, &data2);
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x20D0, &data3);
	PRINT("INTERN_ATSC_FW_VERSION:%x.%x.%x\n", data1,	data2, data3);

	bret = 0;

	return bret;

}

ssize_t atsc_get_information_show(struct device_driver *driver,
char *buf)
{
	u8 ret_tmp;
	u8 data1, data2, data3;
	u8 fgdemodStatus;
	u16 ifagc_tmp, ifagc;
	s32 err_flg[6];
	enum DMD_ATSC_LOCK_STATUS atsclocksStatus = DMD_ATSC_NULL;

	/* AGC Level */
	ret_tmp = _mdrv_dmd_atsc_md_setreg(0, 0x2822, 0x03);
	ret_tmp = _mdrv_dmd_atsc_md_setreg(0, 0x2805, 0x80);

	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x2824, &data1);
	ifagc_tmp = (u16)data1;
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x2825, &data1);
	ifagc_tmp |= (u16)data1<<8;
	ret_tmp = _mdrv_dmd_atsc_md_setreg(0, 0x2805, 0x00);

	ifagc_tmp = (ifagc_tmp>>8)&0xFF;
	ifagc = (~ifagc_tmp)&0xFF;

	PRINT("TUNER_GET_TYPE_AGC\r\n");
	PRINT("DTD AGC: %d\n", ifagc);
	err_flg[0] = ifagc;

	/* Get FW Version */
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x20C4, &data1);
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x20CF, &data2);
	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x20D0, &data3);
	PRINT("INTERN_ATSC_FW_VERSION:%x.%x.%x\n", data1, data2, data3);
	err_flg[1] = data1;
	err_flg[2] = data2;
	err_flg[3] = data3;

	/* Get Lock Status */
	atsclocksStatus = _mdrv_dmd_atsc_md_getlock(0, DMD_ATSC_GETLOCK);
	if (atsclocksStatus == DMD_ATSC_LOCK)
		fgdemodStatus = TRUE;
	else if (atsclocksStatus == DMD_ATSC_UNLOCK)
		fgdemodStatus = FALSE;
	err_flg[4] = fgdemodStatus;
	PRINT("TUNER_GET_LOCK_STATUS\r\n");
	PRINT("TUNER_GET_DEMOD_LOCK: %d\r\n", fgdemodStatus);

	ret_tmp = _mdrv_dmd_atsc_md_getreg(0, 0x20C1, &data1);
	if (data1 == 0xF0 || data1 == 0x80)
		err_flg[5] = 1;
	else
		err_flg[5] = 0;
	PRINT("TUNER_GET_TS_LOCK: %d\r\n", err_flg[5]);

	return scnprintf(buf, PAGE_SIZE, "%d %d %d %d %d %d\n", err_flg[0],
	err_flg[1], err_flg[2], err_flg[3], err_flg[4], err_flg[5]);
}
#ifdef UTPA2

#if	DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN

////////////////////////////////////////////////
///				 UTOPIA	2K API				 ///
////////////////////////////////////////////////

u8 mdrv_dmd_atsc_setdbglevel(enum DMD_ATSC_DBGLV dbglevel)
{
	ATSC_DBG_LEVEL_PARAM dbglevelparam;

	if (!_atscopen)
		return	FALSE;

	dbglevelparam.dbglevel = dbglevel;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_SetDbgLevel,
		&dbglevelparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

struct DMD_ATSC_INFO *mdrv_dmd_atsc_getinfo(void)
{
	ATSC_GET_INFO_PARAM	getinfoparam = {0};

	if (!_atscopen)
		return	NULL;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_GetInfo, &getinfoparam)
		== UTOPIA_STATUS_SUCCESS) {
		return getinfoparam.pinfo;
	} else {
		return NULL;
	}
}

u8 mdrv_dmd_atsc_md_init(
u8 id, struct DMD_ATSC_INITDATA *pdmd_atsc_initdata, u32 initdatalen)
{
	void	*pattribte	=	NULL;
	ATSC_INIT_PARAM	initparam	=	{0};

	if (_atscopen ==	0) {
		if (UtopiaOpen(MODULE_ATSC, &_ppatscinstant, 0, pattribte)
		== UTOPIA_STATUS_SUCCESS)
			_atscopen = 1;
		else {
			return FALSE;
		}
	}

	if (!_atscopen)
		return	FALSE;

	initparam.id = id;
	initparam.pdmd_atsc_initdata = pdmd_atsc_initdata;
	initparam.initdatalen = initdatalen;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_Init,
		&initparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_exit(u8 id)
{
	ATSC_ID_PARAM idparam = {0};

	if (!_atscopen)
		return	FALSE;

	idparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_Exit,
		&idparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u32 mdrv_dmd_atsc_md_getconfig(
u8 id, struct DMD_ATSC_INITDATA *psdmd_atsc_initdata)
{
	ATSC_INIT_PARAM	initparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	initparam.id = id;
	initparam.pdmd_atsc_initdata = psdmd_atsc_initdata;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetConfig,
		&initparam) == UTOPIA_STATUS_SUCCESS)
		return UTOPIA_STATUS_SUCCESS;
	else
		return UTOPIA_STATUS_ERR_NOT_AVAIL;

}

u8 mdrv_dmd_atsc_md_setconfig(
u8 id, enum DMD_ATSC_DEMOD_TYPE etype, u8 benable)
{
	ATSC_SET_CONFIG_PARAM	setconfigparam = {0};

	if (!_atscopen)
		return	FALSE;

	setconfigparam.id	=	id;
	setconfigparam.etype = etype;
	setconfigparam.benable = benable;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_SetConfig,
		&setconfigparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_setreset(u8 id)
{
	ATSC_ID_PARAM	idparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	idparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_SetReset,
		idparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_set_qam_sr(
u8 id, enum DMD_ATSC_DEMOD_TYPE etype, u16 symbol_rate)
{
	ATSC_SET_QAM_SR_PARAM	setqamsrparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	setqamsrparam.id = id;
	setqamsrparam.etype = etype;
	setqamsrparam.symbol_rate = symbol_rate;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_Set_QAM_SR,
		&setqamsrparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_setactive(u8 id, u8 benable)
{
	ATSC_SET_ACTIVE_PARAM	setactiveparam = {0};

	if (!_atscopen)
		return	FALSE;

	setactiveparam.id	=	id;
	setactiveparam.benable = benable;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_SetActive,
		&setactiveparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

#if	DMD_ATSC_STR_EN
u32 mdrv_dmd_atsc_md_setpowerstate(u8 id,
enum EN_POWER_MODE powerstate)
{
	ATSC_SET_POWER_STATE_PARAM setpowerstateparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	setpowerstateparam.id	=	id;
	setpowerstateparam.powerstate = powerstate;

	return UtopiaIoctl(_ppatscinstant,
	DMD_ATSC_DRV_CMD_MD_SetPowerState, &setpowerstateparam);
}
#endif

enum DMD_ATSC_LOCK_STATUS mdrv_dmd_atsc_md_getlock(
u8 id, enum DMD_ATSC_GETLOCK_TYPE etype)
{
	ATSC_GET_LOCK_PARAM	getlockparam = {0};

	if (!_atscopen)
		return	DMD_ATSC_NULL;

	getlockparam.id	=	id;
	getlockparam.etype = etype;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetLock,
		&getlockparam) == UTOPIA_STATUS_SUCCESS)
		return getlockparam.status;
	else
		return DMD_ATSC_NULL;

}

enum DMD_ATSC_DEMOD_TYPE mdrv_dmd_atsc_md_getmodulationmode(u8 id)
{
	ATSC_GET_MODULATION_MODE_PARAM getmodulationmodeparam;

	if (!_atscopen)
		return	FALSE;

	getmodulationmodeparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetModulationMode,
		&getmodulationmodeparam) == UTOPIA_STATUS_SUCCESS)
		return getmodulationmodeparam.etype;
	else
		return DMD_ATSC_DEMOD_NULL;

}

u8 mdrv_dmd_atsc_md_getsignalstrength(u8 id, u16 *strength)
{
	ATSC_GET_SIGNAL_STRENGTH_PARAM getsignalstrengthparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	getsignalstrengthparam.id	=	id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetSignalStrength,
		&getsignalstrengthparam) == UTOPIA_STATUS_SUCCESS) {
		*strength = getsignalstrengthparam.strength;

		return TRUE;
	} else {
		return FALSE;
	}
}

enum DMD_ATSC_SIGNAL_CONDITION mdrv_dmd_atsc_md_getsignalquality(u8 id)
{
	ATSC_GET_SIGNAL_QUALITY_PARAM	getsignalqualityparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	getsignalqualityparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetSignalQuality,
		&getsignalqualityparam) == UTOPIA_STATUS_SUCCESS) {
		return getsignalqualityparam.equality;
	} else {
		return DMD_ATSC_SIGNAL_NO;
	}
}

u8 mdrv_dmd_atsc_md_getsnrpercentage(u8 id)
{
	ATSC_GET_SNR_PERCENTAGE_PARAM	getsnrpercentageparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	getsnrpercentageparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetSNRPercentage,
		&getsnrpercentageparam == UTOPIA_STATUS_SUCCESS))
		return getsnrpercentageparam.percentage;
	else
		return 0;

}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_atsc_md_get_qam_snr(u8 id, float *f_snr)
#else
u8 mdrv_dmd_atsc_md_get_qam_snr(u8 id, struct DMD_ATSC_SNR_DATA *f_snr)
#endif
{
	ATSC_GET_SNR_PARAM getsnrparam = {0};

	if (!_atscopen)
		return	FALSE;

	getsnrparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GET_QAM_SNR,
		&getsnrparam) == UTOPIA_STATUS_SUCCESS) {
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*f_snr = (float)getsnrparam.snr.noisepower/100;
		#else
		f_snr->noisepower	=	getsnrparam.snr.noisepower;
		f_snr->sym_num	=	getsnrparam.snr.sym_num;
		#endif

		return TRUE;

	} else {
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*f_snr = 0;
		#else
		f_snr->noisepower	=	0;
		f_snr->sym_num	=	0;
		#endif

		return FALSE;
	}
}

u8 mdrv_dmd_atsc_md_read_ucpkt_err(u8 id, u16 *packeterr)
{
	ATSC_GET_UCPKT_ERR_PARAM getucpkt_errparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	getucpkt_errparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_Read_uCPKT_ERR,
		&getucpkt_errparam) == UTOPIA_STATUS_SUCCESS) {
		*packeterr = getucpkt_errparam.packeterr;

		return TRUE;
	} else {
		return FALSE;
	}
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_atsc_md_getpreviterbiber(u8 id, float *ber)
#else
u8 mdrv_dmd_atsc_md_getpreviterbiber(u8 id,
struct DMD_ATSC_BER_DATA *ber)
#endif
{
	ATSC_GET_BER_PARAM getberparam = {0};

	if (!_atscopen)
		return	FALSE;

	getberparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetPreViterbiBer,
		&getberparam) == UTOPIA_STATUS_SUCCESS) {
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*ber = 0;
		#else
		ber->biterr		=	0;
		ber->error_window	=	0;
		ber->win_unit		=	0;
		#endif

		return TRUE;

	} else {
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*ber = 0;
		#else
		ber->biterr		=	0;
		ber->error_window	=	0;
		ber->win_unit		=	0;
		#endif

		return FALSE;
	}
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_atsc_md_getpostviterbiber(u8 id, float *ber)
#else
u8 mdrv_dmd_atsc_md_getpostviterbiber(u8 id,
struct DMD_ATSC_BER_DATA *ber)
#endif
{
	ATSC_GET_BER_PARAM getberparam = {0};

	if (!_atscopen)
		return	FALSE;

	getberparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetPostViterbiBer,
		&getberparam) == UTOPIA_STATUS_SUCCESS) {
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		if (!getberparam.ber.biterr &&
		!getberparam.ber.error_window && !getberparam.ber.win_unit)
			*ber = 0;
		else if	(getberparam.ber.biterr	<= 0)
			*ber = 0.5f / ((float)(getberparam.ber.error_window)*(
		getberparam.ber.win_unit));
		else
			*ber = (float)(getberparam.ber.biterr) / ((float)(
		getberparam.ber.error_window)*(getberparam.ber.win_unit));
		#else
		ber->biterr		=	getberparam.ber.biterr;
		ber->error_window	=	getberparam.ber.error_window;
		ber->win_unit		=	getberparam.ber.win_unit;
		#endif

		return TRUE;

	} else {
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*ber = 0;
		#else
		ber->biterr		=	0;
		ber->error_window	=	0;
		ber->win_unit		=	0;
		#endif

		return FALSE;
	}
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_atsc_md_readfrequencyoffset(u8 id, s16 *cfo)
#else
u8 mdrv_dmd_atsc_md_readfrequencyoffset(u8 id, DMD_ATSC_CFO_DATA *cfo)
#endif
{
	ATSC_READ_FREQ_OFFSET_PARAM readfreq_offsetparam = {0};

	if (!_atscopen)
		return FALSE;

	readfreq_offsetparam.id = id;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_ReadFrequencyOffset,
		&readfreq_offsetparam) == UTOPIA_STATUS_SUCCESS) {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		*cfo = readfreq_offsetparam.cfo.rate;
		#else
		cfo->mode = readfreq_offsetparam.cfo.mode;
		cfo->ff   = readfreq_offsetparam.cfo.ff;
		cfo->rate = readfreq_offsetparam.cfo.rate;
		#endif

		return TRUE;

	} else {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		*cfo = 0;
		#else
		cfo->mode = 0;
		cfo->ff   = 0;
		cfo->rate = 0;
		#endif

		return FALSE;
	}
}

u8 mdrv_dmd_atsc_md_setserialcontrol(u8 id, u8 tsconfig_data)
{
	ATSC_SET_SERIAL_CONTROL_PARAM	setserial_controlparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	setserial_controlparam.id = id;
	setserial_controlparam.tsconfig_data = tsconfig_data;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_SetSerialControl,
		&setserial_controlparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_iic_bypass_mode(u8 id, u8 benable)
{
	ATSC_IIC_BYPASS_MODE_PARAM iicbypass_modeparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	iicbypass_modeparam.id	=	id;
	iicbypass_modeparam.benable = benable;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_IIC_BYPASS_MODE,
		&iicbypass_modeparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_switch_sspi_gpio(u8 id, u8 benable)
{
	ATSC_SWITCH_SSPI_GPIO_PARAM	switchsspi_gpioparam	=	{0};

	if (!_atscopen)
		return	FALSE;

	switchsspi_gpioparam.id = id;
	switchsspi_gpioparam.benable = benable;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_SWITCH_SSPI_GPIO,
		&switchsspi_gpioparam) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_gpio_get_level(u8 id, u8 pin_8, u8 *blevel)
{
	ATSC_GPIO_LEVEL_PARAM	gpioLevel_param = {0};

	if (!_atscopen)
		return	FALSE;

	gpioLevel_param.id	=	id;
	gpioLevel_param.pin_8 = pin_8;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GPIO_GET_LEVEL,
		&gpioLevel_param) == UTOPIA_STATUS_SUCCESS) {
		*blevel	=	gpioLevel_param.blevel;

		return TRUE;
	} else {
		return FALSE;
	}
}

u8 mdrv_dmd_atsc_md_gpio_set_level(u8 id, u8 pin_8, u8 blevel)
{
	ATSC_GPIO_LEVEL_PARAM gpioLevel_param = {0};

	if (!_atscopen)
		return	FALSE;

	gpioLevel_param.id = id;
	gpioLevel_param.pin_8 = pin_8;
	gpioLevel_param.blevel = blevel;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GPIO_SET_LEVEL,
		&gpioLevel_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_gpio_out_enable(u8 id,
u8 pin_8, u8 benableout)
{
	ATSC_GPIO_OUT_ENABLE_PARAM gpiooutenable_param	=	{0};

	if (!_atscopen)
		return	FALSE;

	gpiooutenable_param.id	=	id;
	gpiooutenable_param.pin_8 = pin_8;
	gpiooutenable_param.benableout	=	benableout;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GPIO_OUT_ENABLE,
		&gpiooutenable_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_doiqswap(u8 id, u8 bis_qpad)
{
	ATSC_DO_IQ_SWAP_PARAM	doiqswap_param	=	{0};

	if (!_atscopen)
		return	FALSE;

	doiqswap_param.id = id;
	doiqswap_param.bis_qpad = bis_qpad;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_DoIQSwap,
		&doiqswap_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_getreg(u8 id, u16 addr_16, u8 *pdata_8)
{
	ATSC_REG_PARAM reg_param = {0};

	if (!_atscopen)
		return	FALSE;

	reg_param.id	=	id;
	reg_param.addr_16 = addr_16;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_GetReg,
		&reg_param) == UTOPIA_STATUS_SUCCESS) {
		*pdata_8 = reg_param.data_8;

		return TRUE;
	} else {
		return FALSE;
	}
}

u8 mdrv_dmd_atsc_md_setreg(u8 id, u16 addr_16, u8 data_8)
{
	ATSC_REG_PARAM reg_param	=	{0};

	if (!_atscopen)
		return	FALSE;

	reg_param.id = id;
	reg_param.addr_16 = addr_16;
	reg_param.data_8 = data_8;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_SetReg,
		&reg_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_atsc_md_set_digrf_freq(u8 id, u32 rf_freq)
{
	ATSC_SET_DIGRF_PARAM DigRfParam	=	{0};

	if (!_atscopen)
		return	FALSE;

	DigRfParam.id	=	id;
	DigRfParam.rf_freq = rf_freq;

	if (UtopiaIoctl(_ppatscinstant, DMD_ATSC_DRV_CMD_MD_SET_DIGRF_FREQ,
		&DigRfParam) == UTOPIA_STATUS_SUCCESS) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#endif // #if DMD_ATSC_UTOPIA_EN || DMD_ATSC_UTOPIA2_EN

#endif // #ifdef UTPA2
