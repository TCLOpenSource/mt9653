// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include	"drvDMD_DTMB.h"
#if	DMD_DTMB_UTOPIA2_EN
#include	"drvDMD_DTMB_v2.h"
#endif

#if (!defined	MTK_PROJECT)
#ifdef	MSOS_TYPE_LINUX_KERNEL
#include	<linux/string.h>
#include	<linux/kernel.h>
#if	defined(CONFIG_CC_IS_CLANG) && defined(CONFIG_MSTAR_CHIP)
#include	<linux/module.h>
#endif
#else
#include	<string.h>
#include	<stdio.h>
#include	<math.h>
#endif
#endif

#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

#include	"MsOS.h"
#include	"drvMMIO.h"
#include	"drvSYS.h"

#ifndef	DONT_USE_CMA
#ifdef	UTPA2
#include	"drvCMAPool_v2.h"
#else
#include	"drvCMAPool.h"
#endif
#include	"halCHIP.h"
#endif	//	#ifndef	DONT_USE_CMA

#else	//	#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

#define	DONT_USE_CMA

#endif	//	#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

#if	DMD_DTMB_3PARTY_EN
//	Please	add	header	files	needed	by	3	perty	platform
#endif

//typedef	u8(*IOCTL)(DMD_DTMB_IOCTL_DATA*,
//enum DMD_DTMB_HAL_COMMAND, void*);

//static	IOCTL	_NULL_IOCTL_FP_DTMB;//VT = NULL;

u8 (*_NULL_IOCTL_FP_DTMB)(struct DTMB_IOCTL_DATA,
enum DMD_DTMB_HAL_COMMAND, void*);


#ifdef	MSOS_TYPE_LINUX_KERNEL
#if	!DMD_DTMB_UTOPIA_EN && DMD_DTMB_UTOPIA2_EN
u8 (*_NULL_SWI2C_FP1)(u8 ubusnum8, u8 uslaveid8, u8 addrcnt,
u8 *pu8addr, u16 usize16, u8 *pbuf);
//typedef u8(*SWI2C)(u8	ubusnum8, u8	uslaveid8,
//u8 addrcnt, u8 * pu8addr, u16 usize16, u8 * pbuf);
//typedef u8(*HWI2C)(u8 uport8, u8	uslaveid8IIC,
//u8 uaddrsizeiic8, u8 * pu8AddrIIC,
//u32 ubufsizeiic32, u8 * pu8BufIIC);
u8 (*_NULL_HWI2C_FP1)(u8 ubusnum8, u8 uslaveid8, u8 addrcnt,
u8 *pu8addr, u16 usize16, u8 *pbuf);

//static	SWI2C	_NULL_SWI2C_FP1;//VT = NULL;
//static	HWI2C	_NULL_HWI2C_FP1;//VT = NULL;
#endif
#endif

static u8 _default_ioctl_cmd(struct DTMB_IOCTL_DATA	*pdata,
enum DMD_DTMB_HAL_COMMAND	eCmd, void	*pArgs)
{
	#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
	#ifdef	REMOVE_HAL_INTERN_DTMB
	PRINT("LINK	ERROR:	REMOVE_HAL_INTERN_DTMB\n");
	#else
	PRINT("LINK	ERROR:	Please	link	ext	demod	library\n");
	#endif
	#else
	PRINT("LINK ERROR: Please link int or	ext demod library\n");
	#endif

	return	TRUE;
}


//#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG)\
//&& defined(CONFIG_MSTAR_CHIP)
#if !DMD_DTMB_3PARTY_EN
static u8 (*hal_intern_dtmb_ioctl_cmd)(struct DTMB_IOCTL_DATA *pdata,
enum DMD_DTMB_HAL_COMMAND	eCmd, void *pArgs);

static u8 (*hal_extern_dtmb_ioctl_cmd)(struct DTMB_IOCTL_DATA *pdata,
enum DMD_DTMB_HAL_COMMAND	eCmd, void *pArgs);

static u8 (*mdrv_sw_iic_writebytes)(u8 ubusnum8, u8 uslaveid8,
u8 addrcnt, u8	*pu8addr, u16	usize16, u8 *pbuf);

static u8 (*mdrv_sw_iic_readbytes)(u8 ubusnum8, u8 uslaveid8,
u8 ucSubAdr, u8 *paddr, u16	ucBufLen, u8 *pbuf);

static u8 (*mdrv_hw_iic_writebytes)(u8 uport8, u8	uslaveid8IIC,
u8	uaddrsizeiic8, u8 *pu8AddrIIC, u32 ubufsizeiic32,
u8	*pu8BufIIC);


static u8 (*mdrv_hw_iic_readbytes)(u8 uport8, u8	uslaveid8IIC,
u8 uaddrsizeiic8, u8 *pu8AddrIIC, u32 ubufsizeiic32,
u8 *pu8BufIIC);

#else

extern u8 hal_intern_dtmb_ioctl_cmd(struct DTMB_IOCTL_DATA *pdata,
enum DMD_DTMB_HAL_COMMAND eCmd, void *pArgs);//__attribute__((weak));

u8 hal_extern_dtmb_ioctl_cmd(struct DTMB_IOCTL_DATA *pdata,
enum DMD_DTMB_HAL_COMMAND	eCmd, void	*pArgs)	__attribute__((weak));

#ifndef	MSOS_TYPE_LINUX_KERNEL

#else	//	#ifndef	MSOS_TYPE_LINUX_KERNEL
#if	!DMD_DTMB_UTOPIA_EN && DMD_DTMB_UTOPIA2_EN
u8 mdrv_sw_iic_writebytes(u8 ubusnum8, u8 uslaveid8, u8 addrcnt,
u8	*pu8addr, u16	usize16, u8	*pbuf)	__attribute__((weak));

u8 mdrv_sw_iic_readbytes(u8 ubusnum8, u8 uslaveid8,
u8	ucSubAdr, u8	*paddr, u16	ucBufLen, u8	*pbuf)
__attribute__((weak));


u8 mdrv_hw_iic_writebytes(u8 uport8, u8 uslaveid8IIC,
u8 uaddrsizeiic8, u8	*pu8AddrIIC, u32	ubufsizeiic32,
u8	*pu8BufIIC)	__attribute__((weak));


u8 mdrv_hw_iic_readbytes(u8 uport8, u8 uslaveid8IIC,
u8 uaddrsizeiic8, u8	*pu8AddrIIC, u32	ubufsizeiic32,
u8	*pu8BufIIC)	__attribute__((weak));
#endif
#endif	//	#ifndef	MSOS_TYPE_LINUX_KERNEL

#endif

//-------------------------------------------------
//	Local	Defines
//-------------------------------------------------

#if	DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN
	#define	DMD_LOCK()		\
		do	{ \
			MS_ASSERT(MsOS_In_Interrupt() == FALSE);	\
		if (_s32DMD_DTMB_Mutex == -1)
			return	FALSE;
		if (_u8DMD_DTMB_DbgLevel == DMD_DTMB_DBGLV_DEBUG)
			PRINT("%s	lock	mutex\n", __func__);
		MsOS_ObtainMutex(_s32DMD_DTMB_Mutex, MSOS_WAIT_FOREVER);
		}	while (0)

	#define	DMD_UNLOCK()	do	{	MsOS_ReleaseMutex(
		_s32DMD_DTMB_Mutex);
		if (_u8DMD_DTMB_DbgLevel == DMD_DTMB_DBGLV_DEBUG)
			PRINT("%s	unlock	mutex\n", __func__);	}
		while (0)
#elif (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN\
&& DMD_DTMB_MULTI_THREAD_SAFE)
//#define	DMD_LOCK() do { if (!(pRes->sDMD_DTMB_PriData.bInit))
//	return	FALSE;	\
//pIOData->sys.LockDMD(TRUE); } \
//while (0)
//#define	DMD_UNLOCK() pIOData->sys.LockDMD(FALSE)
	#define DMD_LOCK() pIOData->sys.LockDMD(TRUE)
	#define DMD_UNLOCK() pIOData->sys.LockDMD(FALSE)
#else
	#define	DMD_LOCK()
	#define	DMD_UNLOCK()
#endif

#ifdef	MS_DEBUG
#define	DMD_DBG(x)		 (x)
#else
#define	DMD_DBG(x)			// (x)
#endif

#ifndef	UTOPIA_STATUS_SUCCESS
#define	UTOPIA_STATUS_SUCCESS				0x00000000
#endif
#ifndef	UTOPIA_STATUS_FAIL
#define	UTOPIA_STATUS_FAIL					0x00000001
#endif

//-------------------------------------------------
//	Local	Structurs
//-------------------------------------------------


//-------------------------------------------------
//	Global	Variables
//-------------------------------------------------

#if	DMD_DTMB_UTOPIA2_EN
struct DMD_DTMB_ResData	*psDMD_DTMB_ResData;

struct DTMB_IOCTL_DATA	DMD_DTMB_IOData = {	0, 0, {	0	}	};
#endif

//-------------------------------------------------
//	Local	Variables
//-------------------------------------------------

#if	!DMD_DTMB_UTOPIA2_EN
static	struct DMD_DTMB_ResData
sDMD_DTMB_ResData[DMD_DTMB_MAX_DEMOD_NUM] = { { {0}, {0}, {0} } };

static	struct DMD_DTMB_ResData	*psDMD_DTMB_ResData = sDMD_DTMB_ResData;

static struct DTMB_IOCTL_DATA	DMD_DTMB_IOData = { 0, 0, { 0 }	};
#endif

static	struct DTMB_IOCTL_DATA	*pIOData = &DMD_DTMB_IOData;

#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
//static	MSIF_Version	_drv_dmd_dtmb_version;// = {
//	.MW = {	DMD_DTMB_VER, },
//};
#endif

#if	DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN
static	s32	_s32DMD_DTMB_Mutex = -1;
#elif	defined	MTK_PROJECT
static	HANDLE_T	_s32DMD_DTMB_Mutex = -1;
#elif	DVB_STI
static	s32	_s32DMD_DTMB_Mutex = -1;
#endif

#ifndef	DONT_USE_CMA
static	struct	CMA_Pool_Init_Param	_DTMB_CMA_Pool_Init_PARAM;
static	struct	CMA_Pool_Alloc_Param	_DTMB_CMA_Alloc_PARAM;
static	struct	CMA_Pool_Free_Param	_DTMB_CMA_Free_PARAM;
#endif

#ifdef	UTPA2

#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

static	void	*_ppDTMBInstant;//VT = NULL;

static	u32	_u32DTMBopen;	//VT= 0;

#endif

#endif

static	enum DMD_DTMB_DbgLv	_u8DMD_DTMB_DbgLevel = DMD_DTMB_DBGLV_NONE;

//-------------------------------------------------
//	Debug	Functions
//-------------------------------------------------



//-------------------------------------------------
//	Local	Functions
//-------------------------------------------------

static	enum DMD_DTMB_LOCK_STATUS	_DTMB_CheckLock(void)
{
	u8 (*ioctl)(struct DTMB_IOCTL_DATA *pdata,
	enum DMD_DTMB_HAL_COMMAND eCmd, void *pPara);

	struct DMD_DTMB_ResData	*pRes = pIOData->pRes;
	struct DMD_DTMB_InitData	*pInit = &(pRes->sDMD_DTMB_InitData);
	struct DMD_DTMB_Info		*pInfo = &(pRes->sDMD_DTMB_Info);

	ioctl = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD;

	if (ioctl(pIOData, DMD_DTMB_HAL_CMD_DTMB_FEC_Lock, NULL))	{
		pInfo->udtmblockstatus32	|= DMD_DTMB_LOCK_DTMB_FEC_LOCK;
		#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
		pInfo->udtmbfeclocktime32 = MsOS_GetSystemTime();
		#else
		pInfo->udtmbfeclocktime32 = pIOData->sys.getsystemtimems();
		#endif
		return	DMD_DTMB_LOCK;
	} //else {
		#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
		if ((pInfo->udtmblockstatus32 & DMD_DTMB_LOCK_DTMB_FEC_LOCK)
			&& ((MsOS_GetSystemTime()-
			pInfo->udtmbfeclocktime32) < 500))
		#else
		if ((pInfo->udtmblockstatus32 & DMD_DTMB_LOCK_DTMB_FEC_LOCK)
			&& ((pIOData->sys.getsystemtimems()
			-pInfo->udtmbfeclocktime32) < 500))
		#endif
		{
			return	DMD_DTMB_LOCK;
		} //else {
			pInfo->udtmblockstatus32
			&= (~DMD_DTMB_LOCK_DTMB_FEC_LOCK);
		//}

		if (pInfo->udtmblockstatus32
			& DMD_DTMB_LOCK_DTMB_PNP_LOCK) {//STEP 3
			#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
			if ((MsOS_GetSystemTime()-
				pInfo->udtmbscantimestart32)
				< pInit->udtmbfeclockchecktime16)
			#else
			if ((pIOData->sys.getsystemtimems()-
				pInfo->udtmbscantimestart32)
				<	pInit->udtmbfeclockchecktime16)
			#endif
			{
				return	DMD_DTMB_CHECKING;
			}
		} else {//STEP	1,2
			if (ioctl(pIOData, DMD_DTMB_HAL_CMD_DTMB_PNP_Lock,
				 NULL)) {
				pInfo->udtmblockstatus32 |=
				 DMD_DTMB_LOCK_DTMB_PNP_LOCK;
				#ifdef	MS_DEBUG
				if (_u8DMD_DTMB_DbgLevel >=
					 DMD_DTMB_DBGLV_DEBUG)
					PRINT("DMD_DTMB_LOCK_DTMB_PNP_LOCK\n");

				#endif
				#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
				pInfo->udtmbscantimestart32 =
				MsOS_GetSystemTime();
				#else
				pInfo->udtmbscantimestart32 =
				pIOData->sys.getsystemtimems();
				#endif
				return	DMD_DTMB_CHECKING;
			}	//else	{
				#if DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
				if ((MsOS_GetSystemTime()-
					pInfo->udtmbscantimestart32)
					<	pInit->udtmbpnmlockchecktime16)
				#else
				if ((pIOData->sys.getsystemtimems()-
					pInfo->udtmbscantimestart32)	<
					pInit->udtmbpnmlockchecktime16)
				#endif
				{
					return	DMD_DTMB_CHECKING;
				}
			//}
		}
		return	DMD_DTMB_UNLOCK;
//	}
}

static	enum DMD_DTMB_LOCK_STATUS	_DVBC_CheckLock(void)
{
	u8 (*ioctl)(struct DTMB_IOCTL_DATA	*pdata,
	enum DMD_DTMB_HAL_COMMAND	eCmd, void	*pPara);

	struct DMD_DTMB_ResData	*pRes = pIOData->pRes;
	struct DMD_DTMB_InitData	*pInit = &(pRes->sDMD_DTMB_InitData);
	struct DMD_DTMB_Info		*pInfo = &(pRes->sDMD_DTMB_Info);

	ioctl = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD;

	if (ioctl(pIOData, DMD_DTMB_HAL_CMD_DVBC_Main_Lock, NULL))	{
		pInfo->udtmblockstatus32	|= DMD_DTMB_LOCK_DVBC_MAIN_LOCK;
		#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
		pInfo->udtmbfeclocktime32 = MsOS_GetSystemTime();
		#else
		pInfo->udtmbfeclocktime32 = pIOData->sys.getsystemtimems();
		#endif
		return	DMD_DTMB_LOCK;
	}	//else	{
		#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
		if ((pInfo->udtmblockstatus32 & DMD_DTMB_LOCK_DVBC_MAIN_LOCK) &&
			((MsOS_GetSystemTime()-
			pInfo->udtmbfeclocktime32)	<	100))
		#else
		if ((pInfo->udtmblockstatus32 & DMD_DTMB_LOCK_DVBC_MAIN_LOCK) &&
			((pIOData->sys.getsystemtimems()
			-pInfo->udtmbfeclocktime32) < 100))
		#endif
		{
			return	DMD_DTMB_LOCK;
		}	//else	{
			pInfo->udtmblockstatus32	&=
			 (~DMD_DTMB_LOCK_DVBC_MAIN_LOCK);
		//}

		if (pInfo->udtmblockstatus32
			& DMD_DTMB_LOCK_DVBC_PRE_LOCK) {//STEP3
			#if	DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
			if ((MsOS_GetSystemTime()-
				pInfo->udtmbscantimestart32)	<
					pInit->uqammainlockchecktime16)
			#else
			if ((pIOData->sys.getsystemtimems()-
				pInfo->udtmbscantimestart32)
				< pInit->uqammainlockchecktime16)
			#endif
			{
				return	DMD_DTMB_CHECKING;
			}
		} else {//STEP 1,2
			if (ioctl(pIOData, DMD_DTMB_HAL_CMD_DVBC_PreLock,
				NULL)) {
				pInfo->udtmblockstatus32 |=
				 DMD_DTMB_LOCK_DVBC_PRE_LOCK;
			#ifdef	MS_DEBUG
			if (_u8DMD_DTMB_DbgLevel	>= DMD_DTMB_DBGLV_DEBUG)
				PRINT("DMD_DTMB_LOCK_DVBC_PRE_LOCK\n");

			#endif
			#if	DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
			pInfo->udtmbscantimestart32 = MsOS_GetSystemTime();
			#else
			pInfo->udtmbscantimestart32 =
			pIOData->sys.getsystemtimems();
			#endif
			return	DMD_DTMB_CHECKING;
		}	//else	{
			#if	DMD_DTMB_UTOPIA_EN || DMD_DTMB_UTOPIA2_EN
			if ((MsOS_GetSystemTime()-
				pInfo->udtmbscantimestart32)
				 < pInit->uqamprelockchecktime16)
			#else
			if ((pIOData->sys.getsystemtimems()
				-pInfo->udtmbscantimestart32) <
					pInit->uqamprelockchecktime16)
			#endif
			{
				return	DMD_DTMB_CHECKING;
			}
			//}
		}
		return	DMD_DTMB_UNLOCK;
	//}
}

#if (defined	MTK_PROJECT)

static	u32	_getsystemtimems(void)
{
	u32	currenttime = 0;

	currenttime =
	x_os_get_sys_tick() * x_os_drv_get_tick_period();

	return	currenttime;
}

static	void	_delayms(u32	ms)
{
	mcDELAY_US(ms*1000);
}

static	u8	_createmutex(u8	enable)
{
	if (enable)	{
		if (_s32DMD_DTMB_Mutex == -1)	{
			if (x_sema_create(
				&_s32DMD_DTMB_Mutex, X_SEMA_TYPE_MUTEX,
				X_SEMA_STATE_UNLOCK) != OSR_OK) {
				PRINT("%d: x_sema_create Fail!\n", __LINE__);
				return	FALSE;
			}
		}
	}	else	{
		x_sema_delete(_s32DMD_DTMB_Mutex);

		_s32DMD_DTMB_Mutex = -1;
	}

	return	TRUE;
}

static	void	_lockdmd(u8	enable)
{
	if (enable)
		x_sema_lock(_s32DMD_DTMB_Mutex, X_SEMA_OPTION_WAIT);
	else
		x_sema_unlock(_s32DMD_DTMB_Mutex);
}


#elif	DVB_STI


static	u8	_createmutex(u8	enable)
{

	return	TRUE;
}

static	void	_lockdmd(u8	enable)
{

}


static	u32	_getsystemtimems(void)
{
	struct	timespec			ts;

	getrawmonotonic(&ts);
	return	ts.tv_sec	*	1000	+	ts.tv_nsec/1000000;
}

//-------------------------------------------------
///	Delay	for	u32Us	microseconds
///	@param	u32Us	\b	IN:	delay	0	~	999	us
///	@return	None
///	@note	implemented	by	"busy	waiting".
// Plz	call	MsOS_DelayTask	directly	for	ms-order	delay
//-------------------------------------------------
//#if 0
//void	MTK_Demod_DelayTaskUs(u32	u32Us)
//{
//
// *	Refer	to	Documentation/timers/timers-howto.txt
// *	Non-atomic	context
// *		<	10us : udelay()
// *	10us	~	20ms : usleep_range()
// * > 10ms : msleep()	or	msleep_interruptible()
//
//	if (u32Us	<	10UL)	{
//		udelay(u32Us);
//	}	else if (u32Us	<	20UL	*	1000UL)	{
//		usleep_range(u32Us, u32Us);
//	}	else	{
//		msleep_interruptible((unsigned int)(u32Us / 1000UL));
//	}
//}
//#endif
static	void	_delayms(u32	ms)
{
	mtk_demod_delay_us(ms	*	1000UL);
}

#elif	!DMD_DTMB_UTOPIA_EN && DMD_DTMB_UTOPIA2_EN

#ifdef	MSOS_TYPE_LINUX_KERNEL
static u8 _sw_i2c_writebytes(u16 ubusnumslaveid16, u8 u8addrcount,
u8 *pu8addr, u16 usize16, u8 *pudata8)
{
	if (mdrv_sw_iic_writebytes != _NULL_SWI2C_FP1) {
		if (mdrv_sw_iic_writebytes((ubusnumslaveid16>>8)&0x0F,
		ubusnumslaveid16, u8addrcount, pu8addr, usize16, pudata8) != 0)
			return FALSE;
		else
			return TRUE;
	}	else	{
		PRINT("LINK ERROR: I2c function is missing\n");

		return	FALSE;
	}
}

static u8 _sw_i2c_readbytes(u16 ubusnumslaveid16, u8 u8AddrNum,
u8 *paddr, u16 usize16, u8 *pudata8)
{
	if (mdrv_sw_iic_readbytes	!= _NULL_SWI2C_FP1)	{
		if (mdrv_sw_iic_readbytes((ubusnumslaveid16>>8)&0x0F,
		ubusnumslaveid16, u8AddrNum, paddr, usize16, pudata8) != 0)
			return	FALSE;
		else
			return	TRUE;
	}	else	{
		PRINT("LINK ERROR: I2c function is missing\n");

		return	FALSE;
	}
}

static u8 _hw_i2c_writebytes(u16 ubusnumslaveid16,
u8 u8addrcount, u8 *pu8addr, u16 usize16, u8	*pudata8)
{
	if (mdrv_hw_iic_writebytes	!= _NULL_HWI2C_FP1)	{
		if (mdrv_hw_iic_writebytes((ubusnumslaveid16>>12)&0x03,
			ubusnumslaveid16, u8addrcount, pu8addr,
			 usize16, pudata8) != 0)
			return	FALSE;
		else
			return	TRUE;
	}	else	{
		PRINT("LINK ERROR: I2c function is missing\n");

		return	FALSE;
	}
}

static	u8	_hw_i2c_readbytes(u16	ubusnumslaveid16,
u8	u8AddrNum, u8	*paddr, u16	usize16, u8	*pudata8)
{
	if (mdrv_hw_iic_readbytes	!= _NULL_HWI2C_FP1)	{
		if (mdrv_hw_iic_readbytes((ubusnumslaveid16>>12)&0x03,
		ubusnumslaveid16, u8AddrNum, paddr,
			 usize16, pudata8)	!= 0)
			return	FALSE;
		else
			return	TRUE;
	}	else	{
		PRINT("LINK ERROR: I2c function is missing\n");

		return	FALSE;
	}
}
#endif

#endif

static	void	_updateiodata(u8 id, struct DMD_DTMB_ResData	*pRes)
{
	#if	!DMD_DTMB_UTOPIA_EN && DMD_DTMB_UTOPIA2_EN
	#ifdef	MSOS_TYPE_LINUX_KERNEL
	//sDMD_DTMB_InitData.u8I2CSlaveBus bit7 0 : old format 1 : new format
	//sDMD_DTMB_InitData.u8I2CSlaveBus bit6 0 : SW I2C 1 : HW I2C
	//sDMD_DTMB_InitData.u8I2CSlaveBus bit4&5 HW I2C port number

	if ((pRes->sDMD_DTMB_InitData.u8I2CSlaveBus&0x80)
		 == 0x80)	{//Use	Kernel	I2c
		if ((pRes->sDMD_DTMB_InitData.u8I2CSlaveBus&0x40)
			 == 0x40)	{//HW	I2c
			pRes->sDMD_DTMB_InitData.I2C_WriteBytes =
			 _hw_i2c_writebytes;
			pRes->sDMD_DTMB_InitData.I2C_ReadBytes =
			 _hw_i2c_readbytes;
		}	else	{//SW	I2c
			pRes->sDMD_DTMB_InitData.I2C_WriteBytes =
			 _sw_i2c_writebytes;
			pRes->sDMD_DTMB_InitData.I2C_ReadBytes =
			 _sw_i2c_readbytes;
		}
	}
	#endif
	#endif	//	#if	!DMD_DTMB_UTOPIA_EN && DMD_DTMB_UTOPIA2_EN

	pIOData->id = id;
	pIOData->pRes = pRes;
}

//-------------------------------------------------
//	Global	Functions
//-------------------------------------------------

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_setdbglevel(enum DMD_DTMB_DbgLv	udbglevel8)
#else
u8	mdrv_dmd_dtmb_setdbglevel(enum DMD_DTMB_DbgLv	udbglevel8)
#endif
{
	_u8DMD_DTMB_DbgLevel = udbglevel8;

	return	TRUE;
}

#ifdef	UTPA2
struct DMD_DTMB_Info	*_mdrv_dmd_dtmb_getinfo(void)
#else
struct DMD_DTMB_Info	*mdrv_dmd_dtmb_getinfo(void)
#endif
{
	psDMD_DTMB_ResData->sDMD_DTMB_Info.uversion8 = 0;

	return	&(psDMD_DTMB_ResData->sDMD_DTMB_Info);
}

#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

////////////////////////////////////////////////////////////////////////////////
/// SINGLE	DEMOD	API ///
////////////////////////////////////////////////////////////////////////////////

u8	mdrv_dmd_dtmb_init(struct DMD_DTMB_InitData
*pDMD_DTMB_InitData, u32	uinitdatalen32)
{
	return	mdrv_dmd_dtmb_md_init(0, pDMD_DTMB_InitData, uinitdatalen32);
}

u8	mdrv_dmd_dtmb_exit(void)
{
	return	mdrv_dmd_dtmb_md_exit(0);
}

u32	mdrv_dmd_dtmb_getconfig(struct DMD_DTMB_InitData *psDMD_DTMB_InitData)
{
	return	mdrv_dmd_dtmb_md_getconfig(0, psDMD_DTMB_InitData);
}

u8	mdrv_dmd_dtmb_setconfig(DMD_DTMB_DEMOD_TYPE eType,
u8 benable)
{
	return	mdrv_dmd_dtmb_md_setconfig(0, eType, benable);
}

u8	mdrv_dmd_dtmb_setreset(void)
{
	return	mdrv_dmd_dtmb_md_setreset(0);
}

u8	mdrv_dmd_dtmb_set_qam_sr(DMD_DTMB_DEMOD_TYPE eType,
u16	symbol_rate)
{
	return	mdrv_dmd_dtmb_md_set_qam_sr(0, eType, symbol_rate);
}

u8	mdrv_dmd_dtmb_setactive(u8 benable)
{
	return	mdrv_dmd_dtmb_md_setactive(0, benable);
}

#if	DMD_DTMB_STR_EN
u32	mdrv_dmd_dtmb_setpowerstate(enum EN_POWER_MODE upowerstate16)
{
	return	mdrv_dmd_dtmb_md_setpowerstate(0, upowerstate16);
}
#endif

enum DMD_DTMB_LOCK_STATUS	mdrv_dmd_dtmb_getlock(
enum DMD_DTMB_GETLOCK_TYPE	eType)
{
	return	mdrv_dmd_dtmb_md_getlock(0, eType);
}

u8	mdrv_dmd_dtmb_getmodulationmode(
DMD_DTMB_MODULATION_INFO	*sDtmbModulationMode)
{
	return	mdrv_dmd_dtmb_md_getmodulationmode(0, sDtmbModulationMode);
}

u8	mdrv_dmd_dtmb_getsignalstrength(u16	*ustrength16)
{
	return	mdrv_dmd_dtmb_md_getsignalstrength(0, ustrength16);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_readfrequencyoffset(s16	*cfo)
#else
u8	mdrv_dmd_dtmb_readfrequencyoffset(struct DMD_DTMB_CFO_DATA	*cfo)
#endif
{
	return	mdrv_dmd_dtmb_md_readfrequencyoffset(0, cfo);
}

u8	mdrv_dmd_dtmb_getsignalquality(void)
{
	return	mdrv_dmd_dtmb_md_getsignalquality(0);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_getpreldpcber(float	*pber)
#else
u8	mdrv_dmd_dtmb_getpreldpcber(struct DMD_DTMB_BER_DATA	*pber)
#endif
{
	return	mdrv_dmd_dtmb_md_getpreldpcber(0, pber);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_getpreviterbiber(float	*ber)
#else
u8	mdrv_dmd_dtmb_getpreviterbiber(struct DMD_DTMB_BER_DATA	*ber)
#endif
{
	return	mdrv_dmd_dtmb_md_getpreviterbiber(0, ber);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_getpostviterbiber(float	*ber)
#else
u8	mdrv_dmd_dtmb_getpostviterbiber(struct DMD_DTMB_BER_DATA	*ber)
#endif
{
	return	mdrv_dmd_dtmb_md_getpostviterbiber(0, ber);
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_getsnr(float	*snr)
#else
u8 mdrv_dmd_dtmb_getsnr(struct DMD_DTMB_SNR_DATA *snr)
#endif
{
	return	mdrv_dmd_dtmb_md_getsnr(0, snr);
}

u8	mdrv_dmd_dtmb_setserialcontrol(u8	utsconfigdata8)
{
	return	mdrv_dmd_dtmb_md_setserialcontrol(0, utsconfigdata8);
}

u8	mdrv_dmd_dtmb_iic_bypass_mode(u8 benable)
{
	return	mdrv_dmd_dtmb_md_iic_bypass_mode(0, benable);
}

u8	mdrv_dmd_dtmb_switch_sspi_gpio(u8 benable)
{
	return	mdrv_dmd_dtmb_md_switch_sspi_gpio(0, benable);
}

u8	mdrv_dmd_dtmb_gpio_get_level(u8 upin8, u8	*blevel)
{
	return	mdrv_dmd_dtmb_md_gpio_get_level(0, upin8, blevel);
}

u8	mdrv_dmd_dtmb_gpio_set_level(u8 upin8, u8	blevel)
{
	return	mdrv_dmd_dtmb_md_gpio_set_level(0, upin8, blevel);
}

u8	mdrv_dmd_dtmb_gpio_out_enable(u8 upin8, u8 benableOut)
{
	return	mdrv_dmd_dtmb_md_gpio_out_enable(0, upin8, benableOut);
}

u8	mdrv_dmd_dtmb_doiqswap(u8	bIsQPad)
{
	return	mdrv_dmd_dtmb_md_doiqswap(0, bIsQPad);
}

u8	mdrv_dmd_dtmb_getreg(u16 uaddr16, u8	*pudata8)
{
	return	mdrv_dmd_dtmb_md_getreg(0, uaddr16, pudata8);
}

u8	mdrv_dmd_dtmb_setreg(u16 uaddr16, u8 udata8)
{
	return	mdrv_dmd_dtmb_md_setreg(0, uaddr16, udata8);
}

#endif	//	#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

////////////////////////////////////////////////////////////////////////////////
/// MULTI	DEMOD	API ///
////////////////////////////////////////////////////////////////////////////////

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_init(u8 id, struct DMD_DTMB_InitData
	*pDMD_DTMB_InitData, u32	uinitdatalen32)
#else
u8	mdrv_dmd_dtmb_md_init(u8 id, struct DMD_DTMB_InitData
	*pDMD_DTMB_InitData, u32	uinitdatalen32)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	#if	defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG) \
	&& defined(CONFIG_MSTAR_CHIP)
	const	struct	kernel_symbol	*sym;
	#endif
	u8	bRFAGCTristateEnable = 0;
	#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
	u64/*MS_PHY*/	phyNonPMBankSize;
	#endif
	#ifndef	DONT_USE_CMA
	u32	u32PhysicalAddr_FromVA = 0, u32HeapStartPAAddress = 0;
	#endif


	if (!(pRes->sDMD_DTMB_PriData.bInit))	{

		#ifndef	MSOS_TYPE_LINUX_KERNEL
		if (sizeof(struct DMD_DTMB_InitData) == uinitdatalen32)	{

			MEMCPY(&(pRes->sDMD_DTMB_InitData),
			pDMD_DTMB_InitData, uinitdatalen32);

		}	else	{

			DMD_DBG(PRINT("mdrv_dmd_dtmb_init
						input data structure err\n"));
			return	FALSE;
		}
		#else

		MEMCPY(&(pRes->sDMD_DTMB_InitData), pDMD_DTMB_InitData,
		 sizeof(struct DMD_DTMB_InitData));

		#endif
	}	else	{
		DMD_DBG(PRINT("mdrv_dmd_dtmb_init more than once\n"));

		return	TRUE;
	}

	#if	defined	MTK_PROJECT
	pIOData->sys.getsystemtimems = _getsystemtimems;
	pIOData->sys.DelayMS		 = _delayms;
	pIOData->sys.CreateMutex	 = _createmutex;
	pIOData->sys.LockDMD		 = _lockdmd;

	#elif	DVB_STI
	pIOData->sys.getsystemtimems = _getsystemtimems;
	pIOData->sys.DelayMS		 = _delayms;
	pIOData->sys.CreateMutex	 = _createmutex;
	pIOData->sys.LockDMD		 = _lockdmd;


	#elif	DMD_DTMB_3PARTY_EN
	//	pIOData->sys.getsystemtimems = ;
	//	pIOData->sys.DelayMS		 = ;
	//	pIOData->sys.CreateMutex	 = ;
	//	pIOData->sys.LockDMD		 = ;
	//	pIOData->sys.MSPI_Init_Ext = ;
	//	pIOData->sys.MSPI_SlaveEnable = ;
	//	pIOData->sys.MSPI_Write	 = ;
	//	pIOData->sys.MSPI_Read	 = ;

	#elif	!DMD_DTMB_UTOPIA2_EN

	pIOData->sys.DelayMS		 = MsOS_DelayTask;
	pIOData->sys.MSPI_Init_Ext = MDrv_MSPI_Init_Ext;
	pIOData->sys.MSPI_SlaveEnable = MDrv_MSPI_SlaveEnable;
	pIOData->sys.MSPI_Write	 = MDrv_MSPI_Write;
	pIOData->sys.MSPI_Read	 = MDrv_MSPI_Read;
	#endif

	#if	DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN
	if (_s32DMD_DTMB_Mutex == -1)	{
		_s32DMD_DTMB_Mutex =
		MsOS_createmutex(E_MSOS_FIFO, "Mutex DMD DTMB",
		 MSOS_PROCESS_SHARED);
		if ((_s32DMD_DTMB_Mutex) == -1)	{
			DMD_DBG(PRINT("mdrv_dmd_dtmb_init CreateMutexFail\n"));
			return	FALSE;
		}
	}
	#elif (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN \
	 && DMD_DTMB_MULTI_THREAD_SAFE)
	if (!pIOData->sys.CreateMutex(TRUE))	{

		DMD_DBG(PRINT("mdrv_dmd_dtmb_init Create Mutex Fail\n"));
		return	FALSE;
	}
	#endif

	#ifdef	MS_DEBUG
	if (_u8DMD_DTMB_DbgLevel	>= DMD_DTMB_DBGLV_INFO)
		PRINT("mdrv_dmd_dtmb_init\n");
	#endif

	#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
	if (!MDrv_MMIO_GetBASE(&(pRes->sDMD_DTMB_PriData.virtDMDBaseAddr),
		 &phyNonPMBankSize, MS_MODULE_PM))	{
		DMD_DBG(PRINT("mdrv_dmd_dtmb_init Get MS_MODULE_PM Fail\n"));

		return	FALSE;
	}
	#endif

	#if	DVB_STI
/*MS_VIRT*/
	pRes->sDMD_DTMB_PriData.virtDMDBaseAddr =
	 (((size_t *)(pRes->sDMD_DTMB_InitData.u8DMD_DTMB_InitExt)));

	#endif

	pRes->sDMD_DTMB_PriData.bInit = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	#if	DMD_DTMB_STR_EN
	if (pRes->sDMD_DTMB_PriData.eLastState == 0)	{
		pRes->sDMD_DTMB_PriData.eLastState = E_POWER_RESUME;

		pRes->sDMD_DTMB_PriData.eLastType = DMD_DTMB_DEMOD_DTMB;
		pRes->sDMD_DTMB_PriData.usymrate16 = 7560;

	}
	#else
	pRes->sDMD_DTMB_PriData.eLastType = DMD_DTMB_DEMOD_DTMB;
	pRes->sDMD_DTMB_PriData.usymrate16 = 7560;

	#endif

#if !DMD_DTMB_3PARTY_EN
	#if	defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG)\
	 && defined(CONFIG_MSTAR_CHIP)
	#ifdef	CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
	sym = find_symbol("hal_intern_dtmb_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		hal_intern_dtmb_ioctl_cmd =
		 (IOCTL)((unsigned	long)offset_to_ptr(&sym->value_offset));
	else
		hal_intern_dtmb_ioctl_cmd = _NULL_IOCTL_FP_DTMB;
	sym = find_symbol("hal_extern_dtmb_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		hal_extern_dtmb_ioctl_cmd =
		 (IOCTL)((unsigned	long)offset_to_ptr(&sym->value_offset));
	else
		hal_extern_dtmb_ioctl_cmd = _NULL_IOCTL_FP_DTMB;
	sym = find_symbol("mdrv_sw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_sw_iic_writebytes =
		 (SWI2C)((unsigned	long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_sw_iic_writebytes = _NULL_SWI2C_FP1;
	sym = find_symbol("mdrv_sw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_sw_iic_readbytes =
		 (SWI2C)((unsigned	long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_sw_iic_readbytes = _NULL_SWI2C_FP1;
	sym = find_symbol("mdrv_hw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_hw_iic_writebytes =
		 (HWI2C)((unsigned	long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_hw_iic_writebytes = _NULL_HWI2C_FP1;
	sym = find_symbol("mdrv_hw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_hw_iic_readbytes =
		(HWI2C)((unsigned	long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_hw_iic_readbytes = _NULL_HWI2C_FP1;
	#else	//	#ifdef	CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
	sym = find_symbol("hal_intern_dtmb_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		hal_intern_dtmb_ioctl_cmd = (IOCTL)sym->value;
	else
		hal_intern_dtmb_ioctl_cmd = _NULL_IOCTL_FP_DTMB;
	sym = find_symbol("hal_extern_dtmb_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		hal_extern_dtmb_ioctl_cmd = (IOCTL)sym->value;
	else
		hal_extern_dtmb_ioctl_cmd = _NULL_IOCTL_FP_DTMB;
	sym = find_symbol("mdrv_sw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_sw_iic_writebytes = (SWI2C)sym->value;
	else
		mdrv_sw_iic_writebytes = _NULL_SWI2C_FP1;
	sym = find_symbol("mdrv_sw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_sw_iic_readbytes = (SWI2C)sym->value;
	else
		mdrv_sw_iic_readbytes = _NULL_SWI2C_FP1;
	sym = find_symbol("mdrv_hw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_hw_iic_writebytes = (HWI2C)sym->value;
	else
		mdrv_hw_iic_writebytes = _NULL_HWI2C_FP1;
	sym = find_symbol("mdrv_hw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym	!= NULL)
		mdrv_hw_iic_readbytes = (HWI2C)sym->value;
	else
		mdrv_hw_iic_readbytes = _NULL_HWI2C_FP1;
	#endif	//	#ifdef	CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
	#endif
	#endif

#if !DMD_DTMB_3PARTY_EN
	if (pRes->sDMD_DTMB_InitData.bisextdemod)	{

		if (hal_extern_dtmb_ioctl_cmd == _NULL_IOCTL_FP_DTMB)
			pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD =
			 _default_ioctl_cmd;
		else
			pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD =
			 hal_extern_dtmb_ioctl_cmd;
	}	else	{

		#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
		pRes->sDMD_DTMB_PriData.uchipid16 = MDrv_SYS_GetChipID();
		#endif

		if (hal_intern_dtmb_ioctl_cmd == _NULL_IOCTL_FP_DTMB)	{
			pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD =
			 _default_ioctl_cmd;

		}	else	{
			pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD =
			hal_intern_dtmb_ioctl_cmd;

		}
	}
#else
	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD =
			hal_intern_dtmb_ioctl_cmd;


#endif

	#ifndef	DONT_USE_CMA

	if (pRes->sDMD_DTMB_InitData.bisextdemod)
		_DTMB_CMA_Pool_Init_PARAM.heap_id = 0x7F;
	else if (((pRes->sDMD_DTMB_InitData.utdistartaddr32)&0x80000000)
		 == 0x80000000)	{

		_DTMB_CMA_Pool_Init_PARAM.heap_id =
		((pRes->sDMD_DTMB_InitData.utdistartaddr32 & 0x7F000000)>>24);
	}	else	{

		_DTMB_CMA_Pool_Init_PARAM.heap_id = 26;
	}

	if (_DTMB_CMA_Pool_Init_PARAM.heap_id	!= 0x7F)	{
		_DTMB_CMA_Pool_Init_PARAM.flags = CMA_FLAG_MAP_VMA;


	if (MApi_CMA_Pool_Init(&_DTMB_CMA_Pool_Init_PARAM) == FALSE) {

			//DMD_DBG(PRINT("Function = %s, Line = %d,
			//get	MApi_CMA_Pool_Init	ERROR!!!\n",
			// __PRETTY_FUNCTION__, __LINE__));
		}	else	{

	//DMD_DBG(PRINT("Function = %s, Line = %d,
	// get	pool_handle_id	is	%u\n",
	// __PRETTY_FUNCTION__, __LINE__,
	//_DTMB_CMA_Pool_Init_PARAM.pool_handle_id));
	//DMD_DBG(PRINT("Function = %s, Line = %d, get	miu	is	%u\n",
	//__PRETTY_FUNCTION__, __LINE__, _DTMB_CMA_Pool_Init_PARAM.miu));
	//DMD_DBG(PRINT("Function = %s, Line = %d,
	// get	heap_miu_start_offset	is	0x%llX\n",
	// __PRETTY_FUNCTION__, __LINE__,
	//_DTMB_CMA_Pool_Init_PARAM.heap_miu_start_offset));
	//DMD_DBG(PRINT("Function=%s,Line=%d,get heap length is 0x%X\n",
	//__PRETTY_FUNCTION__,__LINE__,_DTMB_CMA_Pool_Init_PARAM.heap_length));
		}

		_miu_offset_to_phy(_DTMB_CMA_Pool_Init_PARAM.miu,
		_DTMB_CMA_Pool_Init_PARAM.heap_miu_start_offset,
		u32HeapStartPAAddress);

		_DTMB_CMA_Alloc_PARAM.pool_handle_id =
		 _DTMB_CMA_Pool_Init_PARAM.pool_handle_id;

		if (((pRes->sDMD_DTMB_InitData.utdistartaddr32)&0x80000000)
			== 0x80000000)	{

			pRes->sDMD_DTMB_InitData.utdistartaddr32 =
	((pRes->sDMD_DTMB_InitData.utdistartaddr32) & 0x00FFFFFF);

	 _DTMB_CMA_Alloc_PARAM.offset_in_pool =
	 (pRes->sDMD_DTMB_InitData.utdistartaddr32
				* (u64)4096)	-	u32HeapStartPAAddress;

			pRes->sDMD_DTMB_InitData.utdistartaddr32 =
			pRes->sDMD_DTMB_InitData.utdistartaddr32*256;
		}	else	{

			_DTMB_CMA_Alloc_PARAM.offset_in_pool =
	  (pRes->sDMD_DTMB_InitData.utdistartaddr32 * (u64)16)
			-	u32HeapStartPAAddress;
		}

		_DTMB_CMA_Alloc_PARAM.length = 0x500000UL;
		_DTMB_CMA_Alloc_PARAM.flags = CMA_FLAG_VIRT_ADDR;

	if (MApi_CMA_Pool_GetMem(&_DTMB_CMA_Alloc_PARAM) == FALSE)	{
	//DMD_DBG(PRINT("Function=%s,Line=%d,CMA_Pool_GetMem	ERROR!!\n",
	  // __PRETTY_FUNCTION__, __LINE__));
		}	else	{
	//DMD_DBG(PRINT("Function = %s, Line = %d, get	from	heap_id	%d\n",
	//__PRETTY_FUNCTION__, __LINE__, _DTMB_CMA_Alloc_PARAM.pool_handle_id));
	//DMD_DBG(PRINT("Function = %s, Line = %d, ask	offset	0x%llX\n",
	//__PRETTY_FUNCTION__, __LINE__, _DTMB_CMA_Alloc_PARAM.offset_in_pool));
	//DMD_DBG(PRINT("Function=%s, Line=%d, ask	length	0x%X\n",
	//__PRETTY_FUNCTION__, __LINE__, _DTMB_CMA_Alloc_PARAM.length));
	//DMD_DBG(PRINT("Function=%s, Line=%d,return	va is 0x%X\n",
	//__PRETTY_FUNCTION__, __LINE__, _DTMB_CMA_Alloc_PARAM.virt_addr));
		}

	u32PhysicalAddr_FromVA =
	MsOS_MPool_VA2PA(_DTMB_CMA_Alloc_PARAM.virt_addr);

		//DMD_DBG(PRINT("#######u32PhysicalAddr_FromVA1 = [0x%x]\n",
		//u32PhysicalAddr_FromVA));

		_miu_offset_to_phy(_DTMB_CMA_Pool_Init_PARAM.miu,
	_DTMB_CMA_Pool_Init_PARAM.heap_miu_start_offset	+
	_DTMB_CMA_Alloc_PARAM.offset_in_pool,
			u32PhysicalAddr_FromVA);

		//DMD_DBG(PRINT("#######u32PhysicalAddr_FromVA2 = [0x%x]\n",
		//u32PhysicalAddr_FromVA));

		//DMD_DBG(PRINT("#######pRes->sDMD_DTMB_InitData.utdistartaddr32
		//=[0x%x]\n", (pRes->sDMD_DTMB_InitData.utdistartaddr32*16)));

	}	else {
		//_DTMB_CMA_Pool_Init_PARAM.heap_id = 0x7F, bypass	CMA
		pRes->sDMD_DTMB_InitData.utdistartaddr32 =
		(pRes->sDMD_DTMB_InitData.utdistartaddr32 & 0x00FFFFFF) * 256;

		//DMD_DBG(PRINT("#pRes->sDMD_DTMB_InitData.utdistartaddr32=
		//[0x%x]\n",(pRes->sDMD_DTMB_InitData.utdistartaddr32)));
	}
	#else	//	#ifndef	DONT_USE_CMA
	if (((pRes->sDMD_DTMB_InitData.utdistartaddr32)
		&0x80000000) == 0x80000000)	{
	//DMD_DBG(PRINT("Utopia Don't support CMA, but AP use CMA mode!\n"));

		pRes->sDMD_DTMB_InitData.utdistartaddr32 =
		(pRes->sDMD_DTMB_InitData.utdistartaddr32 & 0x00FFFFFF) * 256;

		//DMD_DBG(PRINT("#pRes->sDMD_DTMB_InitData.utdistartaddr32 =
		//[0x%x]\n", (pRes->sDMD_DTMB_InitData.utdistartaddr32)));
	}
	#endif	//	#ifndef	DONT_USE_CMA

	_updateiodata(id, pRes);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_InitClk, &bRFAGCTristateEnable);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_Download, NULL);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_TS_INTERFACE_CONFIG, NULL);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_FWVERSION, NULL);

	DMD_UNLOCK();

	return	TRUE;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_exit(u8	id)
#else
u8	mdrv_dmd_dtmb_md_exit(u8	id)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (pRes->sDMD_DTMB_PriData.bInit == false)
		return	bRet;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_Exit, NULL);

	pRes->sDMD_DTMB_PriData.bInit = FALSE;

	#ifndef	DONT_USE_CMA
	if (pRes->sDMD_DTMB_InitData.bisextdemod)
		_DTMB_CMA_Pool_Init_PARAM.heap_id = 0x7F;
	else if (((pRes->sDMD_DTMB_InitData.utdistartaddr32)&0x80000000)
		 == 0x80000000)	{
		_DTMB_CMA_Pool_Init_PARAM.heap_id =
		((pRes->sDMD_DTMB_InitData.utdistartaddr32 & 0x7F000000)>>24);
	}	else	{
		_DTMB_CMA_Pool_Init_PARAM.heap_id = 26;
	}

	if (_DTMB_CMA_Pool_Init_PARAM.heap_id != 0x7F) {
		_DTMB_CMA_Free_PARAM.pool_handle_id =
		_DTMB_CMA_Pool_Init_PARAM.pool_handle_id;
		_DTMB_CMA_Free_PARAM.offset_in_pool =
		_DTMB_CMA_Alloc_PARAM.offset_in_pool;
		_DTMB_CMA_Free_PARAM.length = 0x500000UL;

	if (MApi_CMA_Pool_PutMem(&_DTMB_CMA_Free_PARAM) == FALSE) {
		DMD_DBG(PRINT("Function = %s, Line = %d, MsOS_CMA_Pool_Release
			ERROR!!\n", __PRETTY_FUNCTION__, __LINE__));
	} else {
		DMD_DBG(PRINT("Function = %s, Line = %d, get from heap_id=%d\n",
		 __PRETTY_FUNCTION__, __LINE__,
		 _DTMB_CMA_Free_PARAM.pool_handle_id));
		DMD_DBG(PRINT("Function = %s, Line = %d,
		 ask offset 0x%llX\n", __PRETTY_FUNCTION__, __LINE__,
		 _DTMB_CMA_Free_PARAM.offset_in_pool));
		DMD_DBG(PRINT("Function = %s, Line = %d, ask length	0x%X\n",
		 __PRETTY_FUNCTION__, __LINE__, _DTMB_CMA_Free_PARAM.length));
		}
	}
	#endif
	DMD_UNLOCK();

	#if	DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN
	MsOS_DeleteMutex(_s32DMD_DTMB_Mutex);
	_s32DMD_DTMB_Mutex = -1;
	#elif (!DMD_DTMB_UTOPIA_EN && !DMD_DTMB_UTOPIA2_EN \
	 && DMD_DTMB_MULTI_THREAD_SAFE)
	pIOData->sys.CreateMutex(FALSE);
	#endif

	return	bRet;
}

#ifdef	UTPA2
u32	_mdrv_dmd_dtmb_md_getconfig(u8 id,
struct DMD_DTMB_InitData *psDMD_DTMB_InitData)
#else
u32	mdrv_dmd_dtmb_md_getconfig(u8 id,
struct DMD_DTMB_InitData *psDMD_DTMB_InitData)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;

	MEMCPY(psDMD_DTMB_InitData, &(pRes->sDMD_DTMB_InitData),
	sizeof(struct DMD_DTMB_InitData));

	return	UTOPIA_STATUS_SUCCESS;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_setconfig(u8 id, enum DMD_DTMB_DEMOD_TYPE eType,
u8 benable)
#else
u8	mdrv_dmd_dtmb_md_setconfig(u8 id, enum DMD_DTMB_DEMOD_TYPE eType,
u8 benable)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_SoftReset, NULL);

	if (benable)	{
		switch (eType)	{
		case	DMD_DTMB_DEMOD_DTMB:
		case	DMD_DTMB_DEMOD_DTMB_7M:
		case	DMD_DTMB_DEMOD_DTMB_5M:
	if (eType	!= pRes->sDMD_DTMB_PriData.eLastType) {
		pRes->sDMD_DTMB_PriData.bDownloaded = FALSE;
		pRes->sDMD_DTMB_PriData.eLastType = DMD_DTMB_DEMOD_DTMB;
		pRes->sDMD_DTMB_PriData.usymrate16 = 7560;
		pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		DMD_DTMB_HAL_CMD_Download, NULL);
		pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		DMD_DTMB_HAL_CMD_TS_INTERFACE_CONFIG, NULL);
		pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		DMD_DTMB_HAL_CMD_DoIQSwap, &(pRes->sDMD_DTMB_PriData.bIsQPad));

				}
		bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_SetDTMBMode, NULL);
				break;
		case	DMD_DTMB_DEMOD_DTMB_6M:
	if (eType	!= pRes->sDMD_DTMB_PriData.eLastType) {
		pRes->sDMD_DTMB_PriData.bDownloaded = FALSE;
		pRes->sDMD_DTMB_PriData.eLastType = DMD_DTMB_DEMOD_DTMB_6M;
		pRes->sDMD_DTMB_PriData.usymrate16 = 5670;
		pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		DMD_DTMB_HAL_CMD_Download, NULL);
		pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
		pIOData, DMD_DTMB_HAL_CMD_TS_INTERFACE_CONFIG, NULL);
		pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
		pIOData, DMD_DTMB_HAL_CMD_DoIQSwap,
		&(pRes->sDMD_DTMB_PriData.bIsQPad));
				}
			bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
			pIOData, DMD_DTMB_HAL_CMD_SetDTMBMode, NULL);
				break;
		default:
			pRes->sDMD_DTMB_PriData.eLastType = DMD_DTMB_DEMOD_NULL;
			pRes->sDMD_DTMB_PriData.usymrate16 = 0;
			bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
			pIOData, DMD_DTMB_HAL_CMD_SetModeClean, NULL);
				break;
		}
	}

	#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN
	pRes->sDMD_DTMB_Info.udtmbscantimestart32 = MsOS_GetSystemTime();
	#else
	pRes->sDMD_DTMB_Info.udtmbscantimestart32 =
	pIOData->sys.getsystemtimems();
	#endif
	pRes->sDMD_DTMB_Info.udtmblockstatus32 = 0;
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_setreset(u8	id)
#else
u8	mdrv_dmd_dtmb_md_setreset(u8	id)
#endif
{
	return	TRUE;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_set_qam_sr(u8 id,
enum DMD_DTMB_DEMOD_TYPE eType, u16 symbol_rate)
#else
u8	mdrv_dmd_dtmb_md_set_qam_sr(u8 id,
DMD_DTMB_DEMOD_TYPE eType, u16	symbol_rate)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	pRes->sDMD_DTMB_PriData.eLastType = eType;
	pRes->sDMD_DTMB_PriData.usymrate16 = symbol_rate;

	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_Set_QAM_SR, NULL);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_setactive(u8 id, u8 benable)
#else
u8	mdrv_dmd_dtmb_md_setactive(u8 id, u8 benable)
#endif
{
	return	TRUE;
}

#if	DMD_DTMB_STR_EN
#ifdef	UTPA2
u32	_mdrv_dmd_dtmb_md_setpowerstate(u8 id,
enum EN_POWER_MODE	upowerstate16)
#else
u32	mdrv_dmd_dtmb_md_setpowerstate(u8 id,
enum EN_POWER_MODE	upowerstate16)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u32	u32Return = UTOPIA_STATUS_FAIL;

	if (upowerstate16 == E_POWER_SUSPEND)	{
		pRes->sDMD_DTMB_PriData.bDownloaded = FALSE;
		pRes->sDMD_DTMB_PriData.bIsDTV = pRes->sDMD_DTMB_PriData.bInit;
		pRes->sDMD_DTMB_PriData.eLastState = upowerstate16;

		if (pRes->sDMD_DTMB_PriData.bInit)	{
			#ifdef	UTPA2
			_mdrv_dmd_dtmb_md_exit(id);
			#else
			mdrv_dmd_dtmb_md_exit(id);
			#endif
		}

		u32Return = UTOPIA_STATUS_SUCCESS;
	}	else if (upowerstate16 == E_POWER_RESUME)	{
		if (pRes->sDMD_DTMB_PriData.eLastState == E_POWER_SUSPEND) {
			PRINT("\nVT: (Check DTMB Mode In DRV:) DTV Mode=%d\n",
			pRes->sDMD_DTMB_PriData.bIsDTV);

		if (pRes->sDMD_DTMB_PriData.bIsDTV)	{
			#ifdef	UTPA2
			_mdrv_dmd_dtmb_md_init(id, &(pRes->sDMD_DTMB_InitData),
			sizeof(struct DMD_DTMB_InitData));
			_mdrv_dmd_dtmb_md_doiqswap(id,
			pRes->sDMD_DTMB_PriData.bIsQPad);
			#else
			mdrv_dmd_dtmb_md_init(id, &(pRes->sDMD_DTMB_InitData),
			sizeof(struct DMD_DTMB_InitData));
			mdrv_dmd_dtmb_md_doiqswap(id,
			pRes->sDMD_DTMB_PriData.bIsQPad);
			#endif

		if (pRes->sDMD_DTMB_PriData.eLastType != DMD_DTMB_DEMOD_DTMB &&
		pRes->sDMD_DTMB_PriData.eLastType != DMD_DTMB_DEMOD_DTMB_7M &&
		pRes->sDMD_DTMB_PriData.eLastType != DMD_DTMB_DEMOD_DTMB_6M &&
		pRes->sDMD_DTMB_PriData.eLastType != DMD_DTMB_DEMOD_DTMB_5M) {
			#ifdef	UTPA2
			_mdrv_dmd_dtmb_md_set_qam_sr(id,
			pRes->sDMD_DTMB_PriData.eLastType,
			pRes->sDMD_DTMB_PriData.usymrate16);
			#else
			mdrv_dmd_dtmb_md_set_qam_sr(id,
			pRes->sDMD_DTMB_PriData.eLastType,
			pRes->sDMD_DTMB_PriData.usymrate16);
			#endif
		}	else	{
			#ifdef	UTPA2
			_mdrv_dmd_dtmb_md_setconfig(id,
			 pRes->sDMD_DTMB_PriData.eLastType,
			 TRUE);
			#else
			mdrv_dmd_dtmb_md_setconfig(id,
			pRes->sDMD_DTMB_PriData.eLastType,
			TRUE);
			#endif
			}
			}

			pRes->sDMD_DTMB_PriData.eLastState = upowerstate16;

			u32Return = UTOPIA_STATUS_SUCCESS;
		}	else	{
/* PRINT("[%5d]Not Suspended Yet.Shouldn't  resume\n",  __LINE__); */

			u32Return = UTOPIA_STATUS_FAIL;
		}
	} else {
/*PRINT("[%s,%5d]Do Nothing:%d\n", __FUNCTION__, __LINE__, upowerstate16);*/

		u32Return = UTOPIA_STATUS_FAIL;
	}

	return	u32Return;
}
#endif

#ifdef	UTPA2
enum DMD_DTMB_LOCK_STATUS _mdrv_dmd_dtmb_md_getlock(u8 id,
	enum DMD_DTMB_GETLOCK_TYPE eType)
#else
enum DMD_DTMB_LOCK_STATUS mdrv_dmd_dtmb_md_getlock(u8 id,
	enum DMD_DTMB_GETLOCK_TYPE eType)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	enum DMD_DTMB_LOCK_STATUS	status = DMD_DTMB_UNLOCK;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	switch (eType)	{
	case	DMD_DTMB_GETLOCK:
	switch (pRes->sDMD_DTMB_PriData.eLastType)	{
	case	DMD_DTMB_DEMOD_DTMB:
	case	DMD_DTMB_DEMOD_DTMB_7M:
	case	DMD_DTMB_DEMOD_DTMB_6M:
	case	DMD_DTMB_DEMOD_DTMB_5M:
			status = _DTMB_CheckLock();
			break;
	case	DMD_DTMB_DEMOD_DVBC_16QAM:
	case	DMD_DTMB_DEMOD_DVBC_32QAM:
	case	DMD_DTMB_DEMOD_DVBC_64QAM:
	case	DMD_DTMB_DEMOD_DVBC_128QAM:
	case	DMD_DTMB_DEMOD_DVBC_256QAM:
			status = _DVBC_CheckLock();
			break;
	default:
				status = DMD_DTMB_UNLOCK;
				break;
			}
			break;
	case	DMD_DTMB_GETLOCK_DTMB_AGCLOCK:
			status =
		 (pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
			pIOData, DMD_DTMB_HAL_CMD_AGCLock, NULL)) ?
			DMD_DTMB_LOCK : DMD_DTMB_UNLOCK;
			break;
	case	DMD_DTMB_GETLOCK_DTMB_PNPLOCK:
			status =
		 (pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
			pIOData, DMD_DTMB_HAL_CMD_DTMB_PNP_Lock, NULL)) ?
			DMD_DTMB_LOCK : DMD_DTMB_UNLOCK;
			break;
	case	DMD_DTMB_GETLOCK_DTMB_FECLOCK:
			status =
		 (pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
			pIOData, DMD_DTMB_HAL_CMD_DTMB_FEC_Lock, NULL)) ?
			DMD_DTMB_LOCK : DMD_DTMB_UNLOCK;
			break;
	case	DMD_DTMB_GETLOCK_DVBC_AGCLOCK:
			status =
		 (pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
			pIOData, DMD_DTMB_HAL_CMD_AGCLock, NULL)) ?
			DMD_DTMB_LOCK : DMD_DTMB_UNLOCK;
			break;
	case	DMD_DTMB_GETLOCK_DVBC_PRELOCK:
			status =
		 (pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
			pIOData, DMD_DTMB_HAL_CMD_DVBC_PreLock, NULL)) ?
			DMD_DTMB_LOCK : DMD_DTMB_UNLOCK;
			break;
	case	DMD_DTMB_GETLOCK_DVBC_MAINLOCK:
			status =
		 (pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(
			pIOData, DMD_DTMB_HAL_CMD_DVBC_Main_Lock, NULL)) ?
			DMD_DTMB_LOCK : DMD_DTMB_UNLOCK;
			break;
	default:
			status = DMD_DTMB_UNLOCK;
			break;
	}
	DMD_UNLOCK();

	return	status;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_getmodulationmode(u8 id,
	struct DMD_DTMB_MODULATION_INFO	*sDtmbModulationMode)
#else
u8	mdrv_dmd_dtmb_md_getmodulationmode(u8 id,
	struct DMD_DTMB_MODULATION_INFO	*sDtmbModulationMode)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_GetModulation, sDtmbModulationMode);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_getsignalstrength(u8 id, u16	*ustrength16)
#else
u8	mdrv_dmd_dtmb_md_getsignalstrength(u8 id, u16	*ustrength16)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_ReadIFAGC, ustrength16);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8 _mdrv_dmd_dtmb_md_readfrequencyoffset(u8 id,
struct DMD_DTMB_CFO_DATA *cfo)
#else
u8	mdrv_dmd_dtmb_md_readfrequencyoffset(u8 id, s16	*cfo)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_ReadFrequencyOffset, cfo);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_getsignalquality(u8	id)
#else
u8	mdrv_dmd_dtmb_md_getsignalquality(u8	id)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	percentage = 0;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		  DMD_DTMB_HAL_CMD_ReadSNRPercentage, &percentage);
	DMD_UNLOCK();

	return	percentage;
}



#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_getagc(u8	id)
#else
u8	mdrv_dmd_dtmb_md_getagc(u8	id)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	ifagc = 0;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		  DMD_DTMB_HAL_CMD_IFAGC, &ifagc);
	DMD_UNLOCK();

	return	ifagc;
}


#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_getssi(u8	id)
#else
u8	mdrv_dmd_dtmb_md_getssi(u8	id)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	ssi = 0;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		  DMD_DTMB_HAL_CMD_SSI, &ssi);
	DMD_UNLOCK();

	return	ssi;
}


#ifdef	UTPA2//
u16	_mdrv_dmd_dtmb_md_geticp(u8	id)
#else
u16	mdrv_dmd_dtmb_md_geticp(u8	id)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u16	icp = 0;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		  DMD_DTMB_HAL_CMD_ICP, &icp);
	DMD_UNLOCK();

	return	icp;
}


#ifdef	UTPA2//DMD_DTMB_HAL_CMD_SQI
u8	_mdrv_dmd_dtmb_md_getsqi(u8	id)
#else
u8	mdrv_dmd_dtmb_md_getsqi(u8	id)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	sqi = 0;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		  DMD_DTMB_HAL_CMD_SQI, &sqi);
	DMD_UNLOCK();

	return	sqi;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_getpreldpcber(u8 id,
struct DMD_DTMB_BER_DATA	*pber)
#else
u8	mdrv_dmd_dtmb_md_getpreldpcber(u8 id, float	*pber)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		  DMD_DTMB_HAL_CMD_GetPreLdpcBer, pber);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_getpreviterbiber(u8 id,
struct DMD_DTMB_BER_DATA	*ber)
#else
u8	mdrv_dmd_dtmb_md_getpreviterbiber(u8 id, float	*ber)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_GetPreViterbiBer, ber);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_getpostviterbiber(u8 id,
struct DMD_DTMB_BER_DATA	*ber)
#else
u8	mdrv_dmd_dtmb_md_getpostviterbiber(u8 id, float	*ber)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		  DMD_DTMB_HAL_CMD_GetPostViterbiBer, ber);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8 _mdrv_dmd_dtmb_md_getsnr(u8 id, struct DMD_DTMB_SNR_DATA *snr)
#else
u8	mdrv_dmd_dtmb_md_getsnr(u8 id, float	*snr)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_GetSNR, snr);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_setserialcontrol(u8 id, u8	utsconfigdata8)
#else
u8	mdrv_dmd_dtmb_md_setserialcontrol(u8 id, u8	utsconfigdata8)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	pRes->sDMD_DTMB_InitData.u1TsConfigByte_SerialMode = utsconfigdata8;
	pRes->sDMD_DTMB_InitData.u1TsConfigByte_DataSwap = utsconfigdata8 >> 1;
	pRes->sDMD_DTMB_InitData.u1TsConfigByte_ClockInv = utsconfigdata8 >> 2;
	pRes->sDMD_DTMB_InitData.u5TsConfigByte_DivNum = utsconfigdata8 >> 3;

	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_TS_INTERFACE_CONFIG, NULL);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_iic_bypass_mode(u8 id, u8 benable)
#else
u8	mdrv_dmd_dtmb_md_iic_bypass_mode(u8 id, u8 benable)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_IIC_Bypass_Mode, &benable);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_switch_sspi_gpio(u8 id, u8 benable)
#else
u8	mdrv_dmd_dtmb_md_switch_sspi_gpio(u8 id, u8 benable)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_SSPI_TO_GPIO, &benable);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_gpio_get_level(u8 id, u8 upin8, u8	*blevel)
#else
u8	mdrv_dmd_dtmb_md_gpio_get_level(u8 id, u8 upin8, u8	*blevel)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	struct DMD_DTMB_GPIO_PIN_DATA	sPin;
	u8	bRet = TRUE;

	sPin.upin8 = upin8;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_GPIO_GET_LEVEL, &sPin);
	DMD_UNLOCK();

	*blevel = sPin.blevel;

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_gpio_set_level(u8 id, u8 upin8, u8	blevel)
#else
u8	mdrv_dmd_dtmb_md_gpio_set_level(u8 id, u8 upin8, u8	blevel)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	struct DMD_DTMB_GPIO_PIN_DATA	sPin;
	u8	bRet = TRUE;

	sPin.upin8 = upin8;
	sPin.blevel = blevel;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
				 DMD_DTMB_HAL_CMD_GPIO_SET_LEVEL, &sPin);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_gpio_out_enable(u8 id, u8 upin8,
		u8 benableOut)
#else
u8	mdrv_dmd_dtmb_md_gpio_out_enable(u8 id, u8 upin8,
		u8 benableOut)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	struct DMD_DTMB_GPIO_PIN_DATA	sPin;
	u8	bRet = TRUE;

	sPin.upin8 = upin8;
	sPin.bIsOut = benableOut;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
				 DMD_DTMB_HAL_CMD_GPIO_OUT_ENABLE, &sPin);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_doiqswap(u8 id, u8	bIsQPad)
#else
u8	mdrv_dmd_dtmb_md_doiqswap(u8 id, u8	bIsQPad)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	u8	bRet = TRUE;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		   DMD_DTMB_HAL_CMD_DoIQSwap, &bIsQPad);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	UTPA2
u8 _mdrv_dmd_dtmb_md_getreg(u8 id, u16 uaddr16, u8 *pudata8)
#else
u8	mdrv_dmd_dtmb_md_getreg(u8 id, u16 uaddr16, u8	*pudata8)
#endif
{
	struct DMD_DTMB_ResData    *pRes = psDMD_DTMB_ResData + id;
	struct DMD_DTMB_REG_DATA	reg;
	u8	bRet = TRUE;

	reg.uaddr16 = uaddr16;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
				DMD_DTMB_HAL_CMD_GET_REG, &reg);
	DMD_UNLOCK();

	*pudata8 = reg.udata8;

	return	bRet;
}

#ifdef	UTPA2
u8	_mdrv_dmd_dtmb_md_setreg(u8 id, u16 uaddr16, u8 udata8)
#else
u8	mdrv_dmd_dtmb_md_setreg(u8 id, u16 uaddr16, u8 udata8)
#endif
{
	struct DMD_DTMB_ResData *pRes = psDMD_DTMB_ResData	+ id;
	struct DMD_DTMB_REG_DATA	reg;
	u8	bRet = TRUE;

	reg.uaddr16 = uaddr16;
	reg.udata8 = udata8;

	if (!(pRes->sDMD_DTMB_PriData.bInit))
		return FALSE;
	DMD_LOCK();
	_updateiodata(id, pRes);

	bRet = pRes->sDMD_DTMB_PriData.HAL_DMD_DTMB_IOCTL_CMD(pIOData,
		DMD_DTMB_HAL_CMD_SET_REG, &reg);
	DMD_UNLOCK();

	return	bRet;
}

#ifdef	CONFIG_UTOPIA_PROC_DBG_SUPPORT
static s8 *_mdrv_dmd_str_tok(s8 *pstrSrc, s8 *pstrDes, s8 delimit)
{
	s8	*pstrRes = pstrSrc;

	*pstrDes = '\0';

	if (pstrSrc == NULL)
		return	NULL;

	while (*pstrRes	!= '\0')	{
		if ((*pstrRes == delimit) || (*pstrRes == ' '))
			break;
		*pstrDes++ = *pstrRes++;
	}
	*pstrDes = '\0';

	return (pstrRes	+	1);
}

int	_mdrv_dmd_strtoint(s8	*pstrSrc)
{
	int	iRes = 0;
	s8	*pstrRes = pstrSrc;

	while (*pstrRes	!= '\0')	{
		iRes	*= 10;
		iRes	+= (int)((*pstrRes++)	-	'0');
	}

	return	iRes;
}

u8	_mdrv_dmd_dtmb_dbg_getmoduleinfo(u8 id, u64	*pu64ReqHdl)
{
	DMD_DTMB_MODULATION_INFO sDtmbMdbModulationInfo = {{0} };
	struct DMD_DTMB_InitData	sDtmbMdbInitData = {0};
	u8	u8lockstatus = 0, udata81 = 0, udata82 = 0, udata83 = 0,
	 u8ldpcerr1 = 0, u8ldpcerr2 = 0, u8pnmode = 0, u8pnphase = 0,
	 u8cmode = 0, u8tditemp = 0, u8iqswap = 0;
	u16	u16ldpcerr3 = 0;
	u32	u32tdistartadd = 0;
	u8	bRet = TRUE;

	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x20C1, &u8lockstatus);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x20C4, &udata81);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x20C5, &udata82);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x20C6, &udata83);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3B90, &u8cmode);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3F32, &u8ldpcerr1);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3F33, &u8ldpcerr2);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3BBB, &u8pnmode);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3BBA, &u8pnphase);
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3FA3, &u8tditemp);
	u32tdistartadd = u8tditemp;
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3FA2, &u8tditemp);
	u32tdistartadd = u32tdistartadd<<8|u8tditemp;
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3FA1, &u8tditemp);
	u32tdistartadd = u32tdistartadd<<8|u8tditemp;
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x3FA0, &u8tditemp);
	u32tdistartadd = u32tdistartadd<<8|u8tditemp;
	bRet	&= _mdrv_dmd_dtmb_md_getreg(id, 0x285E, &u8iqswap);

	u16ldpcerr3 = u8ldpcerr2;
	u16ldpcerr3 = u16ldpcerr3<<8|u8ldpcerr1;
	u8pnmode = u8pnmode&0x03;

	bRet	&= _mdrv_dmd_dtmb_md_getmodulationmode(id,
		 &sDtmbMdbModulationInfo);

	if (_mdrv_dmd_dtmb_md_getconfig(id, &sDtmbMdbInitData)
			!= UTOPIA_STATUS_SUCCESS)
		bRet = FALSE;
	else
		bRet	&= TRUE;


	MdbPrint(pu64ReqHdl, "---------MStar	Demod	Info---------\n");
	MdbPrint(pu64ReqHdl, "Spec:	DTMB\n");
	if (u8lockstatus == 0xF0)
		MdbPrint(pu64ReqHdl, "Lock	status:	Lock\n");
	else
		MdbPrint(pu64ReqHdl, "Lock	status:	Unlock\n");

	MdbPrint(pu64ReqHdl, "Version Number: 0x%x.%x.%x\n",
		udata81, udata82, udata83);

	if (u8cmode&0x02)
		MdbPrint(pu64ReqHdl, "Carrier	Mode:	SC\n");
	else
		MdbPrint(pu64ReqHdl, "Carrier	Mode:	MC\n");

	MdbPrint(pu64ReqHdl, "QAMMode:%d\n",
		sDtmbMdbModulationInfo.usiqammode8);

	if (sDtmbMdbModulationInfo.fsicoderate == 4)
		MdbPrint(pu64ReqHdl, "Code	rate:	0.4\n");
	else if (sDtmbMdbModulationInfo.fsicoderate == 6)
		MdbPrint(pu64ReqHdl, "Code	rate:	0.6\n");
	else if (sDtmbMdbModulationInfo.fsicoderate == 8)
		MdbPrint(pu64ReqHdl, "Code	rate:	0.8\n");

	if (u8pnmode == 0)
		MdbPrint(pu64ReqHdl, "PN	Mode:	420\n");
	else if (u8pnmode == 1)
		MdbPrint(pu64ReqHdl, "PN	Mode:	595\n");
	else if (u8pnmode == 2)
		MdbPrint(pu64ReqHdl, "PN	Mode:	945\n");

	if (u8pnphase&0x10)
		MdbPrint(pu64ReqHdl, "PN	phase:	Constant\n");
	else
		MdbPrint(pu64ReqHdl, "PN	phase:	Variable\n");

	MdbPrint(pu64ReqHdl, "Interleaving	Mode: %d\n",
		sDtmbMdbModulationInfo.usiinterleaver16);
	MdbPrint(pu64ReqHdl, "IF:	%d\n", sDtmbMdbInitData.uif_khz16);
	MdbPrint(pu64ReqHdl, "TDI Address: 0x%x\n", u32tdistartadd);
	MdbPrint(pu64ReqHdl, "SNR:	%d	Percent.\n",
		_mdrv_dmd_dtmb_md_getsignalquality(id));
	MdbPrint(pu64ReqHdl, "LDPC	error	count:	%d\n", u16ldpcerr3);
	MdbPrint(pu64ReqHdl, "IQ	SWAP:	%d\n", u8iqswap&0x01);

	return	bRet;
}

u8	_mdrv_dmd_dtmb_dbg_echocmd(u8 id, u64	*pu64ReqHdl,
	u32	ucmdsize32, s8	*pcmd)
{
	s8	*ptr = NULL;
	s8	strbuf[128] = {0};
	u8	bRet = TRUE;

	if (strncmp((char	*)pcmd, "TDIAdd=0x", 9) == 0)	{
		u32	u32TDIaddress;

		ptr = pcmd	+	9;
		ptr = _mdrv_dmd_str_tok(ptr, strbuf, ';');
		u32TDIaddress = _mdrv_dmd_strtoint(strbuf);

	bRet &= _mdrv_dmd_dtmb_md_setreg(id, 0x3FA0, u32TDIaddress && 0xFF);
	bRet &= _mdrv_dmd_dtmb_md_setreg(id, 0x3FA1, u32TDIaddress>>8 && 0xFF);
	bRet &= _mdrv_dmd_dtmb_md_setreg(id, 0x3FA2, u32TDIaddress>>16 && 0xFF);
	bRet &= _mdrv_dmd_dtmb_md_setreg(id, 0x3FA3, u32TDIaddress>>24 && 0xFF);
	} else if (strncmp((char	*)pcmd, "IQSWAP=", 7) == 0) {
		u8	u8iqswap;

		ptr = pcmd + 7;
		ptr = _mdrv_dmd_str_tok(ptr, strbuf, ';');
		u8iqswap = _mdrv_dmd_strtoint(strbuf);
		bRet &= _mdrv_dmd_dtmb_md_setreg(id, 0x285E, u8iqswap & 0x01);
	}	else if (strncmp((char	*)pcmd, "help", 4) == 0) {
		MdbPrint(pu64ReqHdl, "----------MStar	help----------\n");
		MdbPrint(pu64ReqHdl, "1. Get	DEMOD DTMB Information\n");
		MdbPrint(pu64ReqHdl, "		[e.g.]cat	dtmb\n");
		MdbPrint(pu64ReqHdl, "\n");
	}

	return	bRet;
}
#endif	//#ifdef	CONFIG_UTOPIA_PROC_DBG_SUPPORT

#ifdef	UTPA2

#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

////////////////////////////////////////////////////////////////////////////////
///	UTOPIA	2K	API ///
////////////////////////////////////////////////////////////////////////////////

#ifndef	MSOS_TYPE_LINUX_KERNEL

#ifndef	MSOS_TYPE_LINUX
static	const	float	_LogApproxTableX[80] = {
	1.00, 1.30, 1.69, 2.20, 2.86, 3.71, 4.83, 6.27, 8.16, 10.60,
	13.79, 17.92, 23.30, 30.29, 39.37, 51.19, 66.54, 86.50, 112.46, 146.19,
	190.05, 247.06, 321.18, 417.54, 542.80, 705.64, 917.33, 1192.53,
	1550.29, 2015.38,
	2620.00, 3405.99, 4427.79, 5756.13, 7482.97,
	9727.86, 12646.22, 16440.08, 21372.11, 27783.74,
	36118.86, 46954.52, 61040.88, 79353.15, 103159.09,
	134106.82, 174338.86, 226640.52, 294632.68, 383022.48,
	497929.22, 647307.99, 841500.39, 1093950.50, 1422135.65,
	1848776.35, 2403409.25, 3124432.03, 4061761.64, 5280290.13,
	6864377.17, 8923690.32, 11600797.42, 15081036.65, 19605347.64,
	25486951.94, 33133037.52, 43072948.77, 55994833.40, 72793283.42,
	94631268.45, 123020648.99, 159926843.68, 207904896.79, 270276365.82,
	351359275.57, 456767058.24, 593797175.72, 771936328.43, 1003517226.96
};

static	const	float	_LogApproxTableY[80] = {
	0.00, 0.11, 0.23, 0.34, 0.46, 0.57, 0.68, 0.80, 0.91, 1.03,
	1.14, 1.25, 1.37, 1.48, 1.60, 1.71, 1.82, 1.94, 2.05, 2.16,
	2.28, 2.39, 2.51, 2.62, 2.73, 2.85, 2.96, 3.08, 3.19, 3.30,
	3.42, 3.53, 3.65, 3.76, 3.87, 3.99, 4.10, 4.22, 4.33, 4.44,
	4.56, 4.67, 4.79, 4.90, 5.01, 5.13, 5.24, 5.36, 5.47, 5.58,
	5.70, 5.81, 5.93, 6.04, 6.15, 6.27, 6.38, 6.49, 6.60, 6.72,
	6.83, 6.95, 7.06, 7.17, 7.29, 7.40, 7.52, 7.63, 7.74, 7.86,
	7.97, 8.08, 8.20, 8.31, 8.43, 8.54, 8.65, 8.77, 8.88, 9.00
};

static	float	log10approx(float	flt_x)
{
	u8	indx = 0;

	do	{
		if (flt_x	<	_LogApproxTableX[indx])
			break;
		indx++;
	}	while (indx < 79);//stop at indx = 80

	return	_LogApproxTableY[indx];
}
#endif	//	#ifndef	MSOS_TYPE_LINUX

#endif	//	#ifndef	MSOS_TYPE_LINUX_KERNEL

u8	mdrv_dmd_dtmb_setdbglevel(DMD_DTMB_DbgLv	udbglevel8)
{
	DTMB_DBG_LEVEL_PARAM	DbgLevelParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	DbgLevelParam.udbglevel8 = udbglevel8;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_SetDbgLevel,
		 &DbgLevelParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else	{
		return	FALSE;
	}
}

struct DMD_DTMB_Info	*mdrv_dmd_dtmb_getinfo(void)
{
	DTMB_GET_INFO_PARAM	GetInfoParam = {0};

	if (!_u32DTMBopen)
		return	NULL;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_GetInfo,
		 &GetInfoParam) == UTOPIA_STATUS_SUCCESS)
		return	GetInfoParam.pInfo;
	else	{
		return	NULL;
	}
}

u8	mdrv_dmd_dtmb_md_init(u8 id, struct
DMD_DTMB_InitData *pDMD_DTMB_InitData,	u32	uinitdatalen32)
{
	void	*pAttribte = NULL;
	DTMB_INIT_PARAM	InitParam = {0};

	if (_u32DTMBopen == 0)	{
		if (UtopiaOpen(MODULE_DTMB, &_ppDTMBInstant, 0, pAttribte)
			 == UTOPIA_STATUS_SUCCESS)
			_u32DTMBopen = 1;
		else	{
			return	FALSE;
		}
	}

	if (!_u32DTMBopen)
		return	FALSE;

	InitParam.id = id;
	InitParam.pDMD_DTMB_InitData = pDMD_DTMB_InitData;
	InitParam.u32InitDataLen = uinitdatalen32;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_Init,
		&InitParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_exit(u8	id)
{
	DTMB_ID_PARAM	IdParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	IdParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_Exit,
		 &IdParam) == UTOPIA_STATUS_SUCCESS)

		return	TRUE;
	else	{
		return	FALSE;
	}
}

u32	mdrv_dmd_dtmb_md_getconfig(u8 id,
struct DMD_DTMB_InitData *psDMD_DTMB_InitData)
{
	DTMB_INIT_PARAM	InitParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	InitParam.id = id;
	InitParam.pDMD_DTMB_InitData = psDMD_DTMB_InitData;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetConfig,
		 &InitParam) == UTOPIA_STATUS_SUCCESS)
		return	UTOPIA_STATUS_SUCCESS;
	else
		return	UTOPIA_STATUS_ERR_NOT_AVAIL;
}

u8	mdrv_dmd_dtmb_md_setconfig(u8 id, DMD_DTMB_DEMOD_TYPE eType,
	u8 benable)
{
	DTMB_SET_CONFIG_PARAM	SetConfigParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	SetConfigParam.id = id;
	SetConfigParam.eType = eType;
	SetConfigParam.benable = benable;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_SetConfig,
		 &SetConfigParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_setreset(u8	id)
{
	DTMB_ID_PARAM	IdParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	IdParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_SetReset,
		 &IdParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_set_qam_sr(u8 id,
	DMD_DTMB_DEMOD_TYPE eType, u16 symbol_rate)
{
	DTMB_SET_QAM_SR_PARAM	SetQamSrParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	SetQamSrParam.id = id;
	SetQamSrParam.eType = eType;
	SetQamSrParam.symbol_rate = symbol_rate;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_Set_QAM_SR,
		 &SetQamSrParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_setactive(u8 id, u8 benable)
{
	DTMB_SET_ACTIVE_PARAM	SetActiveParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	SetActiveParam.id = id;
	SetActiveParam.benable = benable;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_SetActive,
		 &SetActiveParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else	{
		return	FALSE;
	}
}

#if	DMD_DTMB_STR_EN
u32	mdrv_dmd_dtmb_md_setpowerstate(u8 id,
enum EN_POWER_MODE	upowerstate16)
{
	DTMB_SET_POWER_STATE_PARAM	SetPowerStateParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	SetPowerStateParam.id = id;
	SetPowerStateParam.u16PowerState = upowerstate16;

	return
	UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_SetPowerState,
	 &SetPowerStateParam);
}
#endif

enum DMD_DTMB_LOCK_STATUS mdrv_dmd_dtmb_md_getlock(u8 id,
	enum DMD_DTMB_GETLOCK_TYPE eType)
{
	DTMB_GET_LOCK_PARAM	GetLockParam = {0};

	if (!_u32DTMBopen)
		return	DMD_DTMB_NULL;

	GetLockParam.id = id;
	GetLockParam.eType = eType;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetLock,
		 &GetLockParam) == UTOPIA_STATUS_SUCCESS)
		return	GetLockParam.status;
	else	{
		return	DMD_DTMB_NULL;
	}
}

u8	mdrv_dmd_dtmb_md_getmodulationmode(u8 id,
	DMD_DTMB_MODULATION_INFO *sDtmbModulationMode)
{
	DTMB_GET_MODULATION_MODE_PARAM	GetModulationModeParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GetModulationModeParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetModulationMode,
		 &GetModulationModeParam) == UTOPIA_STATUS_SUCCESS) {
		*sDtmbModulationMode = GetModulationModeParam.info;

		return	TRUE;
	}	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_getsignalstrength(u8 id, u16	*ustrength16)
{
	DTMB_GET_SIGNAL_STRENGTH_PARAM	GetSignalStrengthParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GetSignalStrengthParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetSignalStrength,
		 &GetSignalStrengthParam) == UTOPIA_STATUS_SUCCESS) {
		*ustrength16 = GetSignalStrengthParam.ustrength16;
		return	TRUE;
	}	else	{
		return	FALSE;
	}
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_md_readfrequencyoffset(u8 id, s16	*cfo)
#else
u8	mdrv_dmd_dtmb_md_readfrequencyoffset(
	u8 id, struct DMD_DTMB_CFO_DATA *cfo)
#endif
{
	DTMB_READ_FREQ_OFFSET_PARAM	ReadFreqOffsetParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	ReadFreqOffsetParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_ReadFrequencyOffset,
		 &ReadFreqOffsetParam) == UTOPIA_STATUS_SUCCESS)	{
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*cfo = (s16)((((double)(ReadFreqOffsetParam.cfo.fftfirstcfo)/
		0x10000 + (double)(ReadFreqOffsetParam.cfo.fftsecondcfo)/
		0x20000)) * (double)(ReadFreqOffsetParam.cfo.sr));
		#else
		cfo->fftfirstcfo  = ReadFreqOffsetParam.cfo.fftfirstcfo;
		cfo->fftsecondcfo = ReadFreqOffsetParam.cfo.fftsecondcfo;
		cfo->sr		 =   ReadFreqOffsetParam.cfo.sr;
		#endif

		return	TRUE;
	}	/* else	{ */
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*cfo = 0;
		#else
		cfo->fftfirstcfo = 0;
		cfo->fftsecondcfo = 0;
		cfo->sr		 = 0;
		#endif

		return	FALSE;
	/* } */
}

u8	mdrv_dmd_dtmb_md_getsignalquality(u8	id)
{
	DTMB_GET_SIGNAL_QUALITY_PARAM	GetSignalQualityParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GetSignalQualityParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetSignalQuality,
		 &GetSignalQualityParam) == UTOPIA_STATUS_SUCCESS)	{
		return	GetSignalQualityParam.u8Percentage;
	}	else	{
		return	0;
	}
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_md_getpreldpcber(u8 id, float	*pber)
#else
u8	mdrv_dmd_dtmb_md_getpreldpcber(u8 id,
struct DMD_DTMB_BER_DATA	*pber)
#endif
{
	DTMB_GET_BER_PARAM	GetBerParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GetBerParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetPreLdpcBer,
		 &GetBerParam) == UTOPIA_STATUS_SUCCESS)	{
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		if (GetBerParam.ber.error_window == 0)	{
			*pber = 0;
		}	else	{
			*pber = (float)(GetBerParam.ber.biterr)/7488.0/
		 (float)(GetBerParam.ber.error_window);
		}
		#else
		pber->biterr	 = GetBerParam.ber.biterr;
		pber->error_window = GetBerParam.ber.error_window;
		#endif

		return	TRUE;
	}	/* else	{ */
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*pber = 0;
		#else
		pber->biterr	 = 0;
		pber->error_window = 0;
		#endif

		return	FALSE;
	/* } */
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_md_getpreviterbiber(u8 id, float	*ber)
#else
u8	mdrv_dmd_dtmb_md_getpreviterbiber(
u8 id, struct DMD_DTMB_BER_DATA	*ber)
#endif
{
	DTMB_GET_BER_PARAM	GetBerParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GetBerParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetPreViterbiBer,
		 &GetBerParam) == UTOPIA_STATUS_SUCCESS)	{
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*ber = 0;
		#else
		ber->biterr	 = 0;
		ber->error_window = 0;
		#endif

		return	TRUE;
	}	/* else	{ */
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*ber = 0;
		#else
		ber->biterr	 = 0;
		ber->error_window = 0;
		#endif

		return	FALSE;
	/* } */
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_md_getpostviterbiber(u8 id, float	*ber)
#else
u8	mdrv_dmd_dtmb_md_getpostviterbiber(u8 id,
struct DMD_DTMB_BER_DATA	*ber)
#endif
{
	DTMB_GET_BER_PARAM	GetBerParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GetBerParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetPostViterbiBer,
		 &GetBerParam) == UTOPIA_STATUS_SUCCESS)	{
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*ber = 0;
		#else
		ber->biterr	 = 0;
		ber->error_window = 0;
		#endif

		return	TRUE;
	}	/* else	{ */
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*ber = 0;
		#else
		ber->biterr	 = 0;
		ber->error_window = 0;
		#endif

		return	FALSE;
	/* } */
}

#ifndef	MSOS_TYPE_LINUX_KERNEL
u8	mdrv_dmd_dtmb_md_getsnr(u8 id, float	*snr)
#else
u8 mdrv_dmd_dtmb_md_getsnr(u8 id, struct DMD_DTMB_SNR_DATA *snr)
#endif
{
	DTMB_GET_SNR_PARAM	GetSnrParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GetSnrParam.id = id;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetSNR,
		 &GetSnrParam) == UTOPIA_STATUS_SUCCESS)	{
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		if (GetSnrParam.snr.sym_num == 0)	{
			*snr = 0;
		}	else	{
			#ifdef	MSOS_TYPE_LINUX
			*snr = 10.0f*log10f((float)(GetSnrParam.snr.snr)/
		 ((float)GetSnrParam.snr.sym_num));
			#else
			*snr = 10.0f*log10approx((float)(GetSnrParam.snr.snr)/
		 ((float)GetSnrParam.snr.sym_num));
			#endif
		}
		#else
		snr->snr	 = GetSnrParam.snr.snr;
		snr->sym_num = GetSnrParam.snr.sym_num;
		#endif

		return	TRUE;
	}	/* else	{ */
		#ifndef	MSOS_TYPE_LINUX_KERNEL
		*snr = 0;
		#else
		snr->snr	 = 0;
		snr->sym_num = 0;
		#endif

		return	FALSE;
	/* } */
}

u8	mdrv_dmd_dtmb_md_setserialcontrol(u8 id, u8	utsconfigdata8)
{
	DTMB_SET_SERIAL_CONTROL_PARAM	SetSerialControlParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	SetSerialControlParam.id = id;
	SetSerialControlParam.utsconfigdata8 = utsconfigdata8;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_SetSerialControl,
		 &SetSerialControlParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else
		return	FALSE;
}

u8	mdrv_dmd_dtmb_md_iic_bypass_mode(u8 id, u8 benable)
{
	DTMB_IIC_BYPASS_MODE_PARAM	IicBypassModeParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	IicBypassModeParam.id = id;
	IicBypassModeParam.benable = benable;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_IIC_BYPASS_MODE,
		 &IicBypassModeParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else
		return	FALSE;
}

u8	mdrv_dmd_dtmb_md_switch_sspi_gpio(u8 id, u8 benable)
{
	DTMB_SWITCH_SSPI_GPIO_PARAM	SwitchSspiGpioParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	SwitchSspiGpioParam.id = id;
	SwitchSspiGpioParam.benable = benable;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_SWITCH_SSPI_GPIO,
		&SwitchSspiGpioParam) == UTOPIA_STATUS_SUCCESS)	{
		return	TRUE;
	}	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_gpio_get_level(u8 id, u8 upin8, u8	*blevel)
{
	DTMB_GPIO_LEVEL_PARAM	GpioLevelParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GpioLevelParam.id = id;
	GpioLevelParam.upin8 = upin8;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GPIO_GET_LEVEL,
		 &GpioLevelParam) == UTOPIA_STATUS_SUCCESS)	{
		*blevel = GpioLevelParam.blevel;

		return	TRUE;
	}	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_gpio_set_level(u8 id, u8 upin8, u8	blevel)
{
	DTMB_GPIO_LEVEL_PARAM	GpioLevelParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GpioLevelParam.id = id;
	GpioLevelParam.upin8 = upin8;
	GpioLevelParam.blevel = blevel;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GPIO_SET_LEVEL,
		 &GpioLevelParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else
		return	FALSE;
}

u8	mdrv_dmd_dtmb_md_gpio_out_enable(u8 id, u8 upin8,
	u8 benableOut)
{
	DTMB_GPIO_OUT_ENABLE_PARAM	GpioOutEnableParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	GpioOutEnableParam.id = id;
	GpioOutEnableParam.upin8 = upin8;
	GpioOutEnableParam.benableOut = benableOut;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GPIO_OUT_ENABLE,
		 &GpioOutEnableParam) == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else
		return	FALSE;
}

u8	mdrv_dmd_dtmb_md_doiqswap(u8 id, u8	bIsQPad)
{
	DTMB_DO_IQ_SWAP_PARAM	DoIQSwapParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	DoIQSwapParam.id = id;
	DoIQSwapParam.bIsQPad = bIsQPad;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_DoIQSwap,
		 &DoIQSwapParam) == UTOPIA_STATUS_SUCCESS)	{
		return	TRUE;
	}	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_getreg(u8 id, u16 uaddr16, u8	*pudata8)
{
	DTMB_REG_PARAM	RegParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	RegParam.id = id;
	RegParam.uaddr16 = uaddr16;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_GetReg,
		 &RegParam) == UTOPIA_STATUS_SUCCESS)	{
		*pudata8 = RegParam.udata8;

		return	TRUE;
	}	else	{
		return	FALSE;
	}
}

u8	mdrv_dmd_dtmb_md_setreg(u8 id, u16 uaddr16, u8 udata8)
{
	DTMB_REG_PARAM	RegParam = {0};

	if (!_u32DTMBopen)
		return	FALSE;

	RegParam.id = id;
	RegParam.uaddr16 = uaddr16;
	RegParam.udata8 = udata8;

	if (UtopiaIoctl(_ppDTMBInstant, DMD_DTMB_DRV_CMD_MD_SetReg, &RegParam)
	 == UTOPIA_STATUS_SUCCESS)
		return	TRUE;
	else
		return	FALSE;

}

#endif	//	#if	DMD_DTMB_UTOPIA_EN	||	DMD_DTMB_UTOPIA2_EN

int _mdrv_dmd_drv_dtmb_get_frontend(struct dvb_frontend *fe,
			struct dtv_frontend_properties *p)
{
	u8 ssi = 0, sqi = 0, qrate = 0;
	//u8 ifagc = 0;
	u8 ret = 0;
	u16 freqoffset = 0, edtmbpninfo = 0;
	u16 icp = 0, snrqty = 0;
	u32 et2tsrate = 0, uber32 = 0;
	u8 dmdstatus = 0;

	struct dtv_fe_stats *stats;
	struct DMD_DTMB_CFO_DATA cfo;
	struct DMD_DTMB_MODULATION_INFO sDtmbModulationMode;
	struct DMD_DTMB_BER_DATA berr;

	p->bandwidth_hz = 8000000;
/////////////////////////////////////////////////////////
	//ret = _mdrv_dmd_dtmb_md_getreg(0, 0x20C0, &dmdstatus);
	//PRINT("DTMB_DRV: dmdmch = 0x%x\n", dmdstatus);
	//ret = _mdrv_dmd_dtmb_md_getreg(0, 0x20C1, &dmdstatus);
	//PRINT("DTMB_DRV: dmdstatus = 0x%x\n", dmdstatus);
//!SNR porting
	snrqty = (u16) _mdrv_dmd_dtmb_md_getsignalquality(0);//VT adds
	//PRINT("DTMB_DRV: SNR1 = %d\n", snrqty);
	snrqty = (u16) snrqty*3; //doing "/10" upper layer;
	//PRINT("DTMB_DRV: SNR1*3 = %d\n", snrqty);
	 //printk(KERN_INFO"[%s][%d], SNR = %d\n", __func__, __LINE__, snrqty);
	stats = &p->cnr;
	stats->len = 1;
	stats->stat[0].svalue = snrqty;
	stats->stat[0].scale = FE_SCALE_DECIBEL;
/////////////////////////////////////////////////////////
		//!SSI porting
		ssi = _mdrv_dmd_dtmb_md_getssi(0);
		//!IFAGC porting
		//ifagc = _mdrv_dmd_dtmb_md_getagc(0);
		//!sqi porting
		sqi = _mdrv_dmd_dtmb_md_getsqi(0);

		//!IncorrectPackets porting
		icp = _mdrv_dmd_dtmb_md_geticp(0);
		//PRINT("DTMB_DRV: IncorrectPackets = %d\n", icp);
		stats = &p->block_error;
		stats->len = 1;
		stats->stat[0].uvalue = icp;
		stats->stat[0].scale = FE_SCALE_COUNTER;

		//!ber porting: format=10^9???
	 ret =  _mdrv_dmd_dtmb_md_getpreldpcber(0, &berr);
	//_MDrv_DMD_DTMB_MD_GetPreLdpcBer(u8 id, DMD_DTMB_BER_DATA *pber)
	//u32 BitErr;
	//u16 Error_window;
	//BER = (UINT16)(((double)berr.BitErr/(double)7488.0/
	//(double)berr.Error_window)*(double)50000);
	if (berr.biterr > 85899)
		uber32 = (u32) (((u32)((berr.biterr/7488)*50000
		))/berr.error_window);
	else
		uber32 = (u32) (((u32)(berr.biterr*50000
		))/7488/berr.error_window);
	//mcSHOW_DBG_MSG(("Pre-LDPC BER*50000 = %d\n",BER));
	uber32 = uber32*2*10000;// format: original-ber*10^9
	stats = &p->post_bit_error;
	stats->len = 1;
	stats->stat[0].uvalue = uber32;
	stats->stat[0].scale = FE_SCALE_COUNTER;
	//PRINT("(in DTMB_DRV)berr.biterr = %d\n", berr.biterr);
	//PRINT("(in DTMB_DRV)error_window = %d\n", berr.error_window);
	//PRINT("(in DTMB_DRV)postber(10^9) = %d\n", uber32);



	//!CFO porting
	ret = _mdrv_dmd_dtmb_md_readfrequencyoffset(0, &cfo);
	freqoffset = (short)((((s32)cfo.fftfirstcfo*(s32)cfo.sr/0x10000
	+(s32)cfo.fftsecondcfo*(s32)cfo.sr/0x20000)));

	p->frequency_offset = (s32) freqoffset;
	//PRINT("(in DTMB_DRV)Carrier Frequency Offset = %d\n",
	//p->frequency_offset);

	 //!SI Information porting
	 ret = _mdrv_dmd_dtmb_md_getmodulationmode(0, &sDtmbModulationMode);
	//eT2TsRate=(UINT32)756*Qrate*sDtmbModulationMode.fsicoderate;//Kbit/s
	switch (sDtmbModulationMode.usiqammode8/*psDtmbDemodCtx->u1Mod*/) {
	case 4/*constellation_qam4*/:
	{
		qrate = 2;
		p->modulation = QPSK;
	}
	break;
	//case constellation_qam4NR:  _pSignal->QAMSize = 5;   break;
	case 16/*constellation_qam16*/:
	{
		qrate = 4;
		p->modulation = QAM_16;
	}
	break;

	case 32/*constellation_qam32*/:
	{
		qrate = 5;
		p->modulation = QAM_32;
	}
	break;

	case 64/*constellation_qam64*/:
	{
		qrate = 6;
		p->modulation = QAM_64;
	}
	break;

	default:
	{
		p->modulation = QAM_64;
		qrate = 0;
	}
	break;
	}

		if (sDtmbModulationMode.usinr8)	{
			p->modulation = QAM_4_NR;
			qrate = 2;
		}

	//((sDtmbModulationMode.upnc8) == 0)//var phase
	switch (sDtmbModulationMode.upnm16) {
	case 420:
			p->guard_interval = GUARD_INTERVAL_PN420;
		break;

	case 595:
		p->guard_interval = GUARD_INTERVAL_PN595;
		break;

	case 945:
		p->guard_interval = GUARD_INTERVAL_PN945;
		break;

	default:
		p->guard_interval = GUARD_INTERVAL_PN945;
		break;
}

	switch (sDtmbModulationMode.usiinterleaver16) {
	case 240:
		p->interleaving = INTERLEAVING_240; break;

	case 720:
		p->interleaving = INTERLEAVING_720; break;

	default:
		p->interleaving = INTERLEAVING_240; break;
	}
	switch (sDtmbModulationMode.fsicoderate)	{
	case 4:
		p->fec_inner = FEC_2_5;   break;

	case 6:
		p->fec_inner = FEC_3_5;   break;

	case 8:
		p->fec_inner = FEC_4_5;   break;

	default:
		p->fec_inner = FEC_2_5;   break;
	}

	switch (sDtmbModulationMode.usicarriermode8) {
	case 0/*CarrierMode_MC*/:
	p->transmission_mode = TRANSMISSION_MODE_C3780; break;

	case 1/*CarrierMode_SC*/:
	p->transmission_mode = TRANSMISSION_MODE_C1; break;

	default:
	p->transmission_mode = TRANSMISSION_MODE_C1; break;
	}
	  //!TS Rate porting
			//7.56M * QAM mode * code rate = TS rate
	   et2tsrate = (u32) (2842560000/(3780+sDtmbModulationMode.upnm16)
			*qrate*sDtmbModulationMode.fsicoderate);
				et2tsrate = (u32) (et2tsrate/1000);

	return 0;
}

ssize_t dtmb_get_information_show(struct device_driver *driver,
char *buf)
{
	u8 ret_tmp;
	u8 data1 = 0, data2 = 0, data3 = 0;
	u8 dmdstatus = 0;
	u8 ifagc = 0, qrate = 0;
	s32 err_flg[10];
	u32 et2tsrate = 0;

	enum DMD_DTMB_LOCK_STATUS dtmblockstatus  = DMD_DTMB_NULL;
	struct DMD_DTMB_MODULATION_INFO sDtmbModulationMode;

	/* AGC Level */
	ifagc = _mdrv_dmd_dtmb_md_getagc(0);
	//PRINT("(in DTMB_DRV)AGC1: %d\n", ifagc);
	err_flg[0] = (s32) ifagc;
	//PRINT("(in DTMB_DRV)AGC2: %d\n", err_flg[0]);
	/* Get FW Version */
	ret_tmp = _mdrv_dmd_dtmb_md_getreg(0, 0x20C4, &data1);
	ret_tmp = _mdrv_dmd_dtmb_md_getreg(0, 0x20C5, &data2);
	ret_tmp = _mdrv_dmd_dtmb_md_getreg(0, 0x20C6, &data3);
	//PRINT("(in DTMB_DRV)FW_VERSION1:%x.%x.%x\n", data1, data2, data3);
	err_flg[1] = (s32) data1;
	err_flg[2] = (s32) data2;
	err_flg[3] = (s32) data3;
	//PRINT("(in DTMB_DRV)FW_VERSION2:%x.%x.%x\n",
	//err_flg[1], err_flg[2], err_flg[3]);

	/* Get Lock Status */
	dtmblockstatus = _mdrv_dmd_dtmb_md_getlock(0,
				DMD_DTMB_GETLOCK);
	if (dtmblockstatus == DMD_DTMB_LOCK)
		dmdstatus = TRUE;
	else if (dtmblockstatus == DMD_DTMB_UNLOCK)
		dmdstatus = FALSE;
	err_flg[4] = (s32) dmdstatus;
	//PRINT("(in DTMB_DRV)DEMOD_LOCK: %d\r\n", err_flg[4]);

	ret_tmp = _mdrv_dmd_dtmb_md_getreg(0, 0x20C1, &data1);
	if (data1 == 0xF0)
		err_flg[5] = 1;
	else
		err_flg[5] = 0;
	//PRINT("(in DTMB_DRV)TS_LOCK: %d\r\n", err_flg[5]);

	err_flg[6] = _mdrv_dmd_dtmb_md_getsqi(0);
	//PRINT("(in DTMB_DRV)SQI: %d\r\n", err_flg[6]);

	err_flg[7] = _mdrv_dmd_dtmb_md_getssi(0);
	//PRINT("(in DTMB_DRV)SSI: %d\r\n", err_flg[7]);

	ret_tmp = _mdrv_dmd_dtmb_md_getmodulationmode(
	0, &sDtmbModulationMode);
	//eT2TsRate=(UINT32)756*Qrate*sDtmbModulationMode.fsicoderate;//Kbit/s
	switch (sDtmbModulationMode.usiqammode8/*psDtmbDemodCtx->u1Mod*/) {
	case 4/*constellation_qam4*/:
		qrate = 2;
	break;

	case 16/*constellation_qam16*/:
		qrate = 4;
	break;

	case 32/*constellation_qam32*/:
		qrate = 5;
	break;

	case 64/*constellation_qam64*/:
		qrate = 6;
	break;

	default:
		qrate = 0;
	break;
	}

		if (sDtmbModulationMode.usinr8)
			qrate = 2;

	 //!TS Rate porting
		//7.56M * QAM mode * code rate = TS rate
	 et2tsrate = (u32) (2842560000/(3780+sDtmbModulationMode.upnm16)
		*qrate*sDtmbModulationMode.fsicoderate);
		et2tsrate = (u32) (et2tsrate/1000);
		err_flg[8] = (s32) et2tsrate;

		if ((sDtmbModulationMode.upnc8) == 0)//var phase
			err_flg[9] = 1;//PNP: 0 = conts phase, 1 = var phase
		else
			err_flg[9] = 0;//PNP: 0 = conts phase, 1 = var phase
		//PRINT("(in DTMB_DRV)sDtmbModulationMode.upnm16: %d\r\n",
		//sDtmbModulationMode.upnm16);
		//PRINT("(in DTMB_DRV)qrate: %d\r\n", qrate);
		//PRINT("(in DTMB_DRV)sDtmbModulationMode.fsicoderate: %d\r\n",
		//sDtmbModulationMode.fsicoderate);
		//PRINT("(in DTMB_DRV)TS Rate(Kbps): %d\r\n", err_flg[8]);

	return scnprintf(buf, PAGE_SIZE, "%d %d %d %d %d %d %d %d %d %d\n",
	err_flg[0], err_flg[1], err_flg[2], err_flg[3], err_flg[4], err_flg[5],
	err_flg[6], err_flg[7], err_flg[8], err_flg[9]);
}

#endif	//	#ifdef	UTPA2
