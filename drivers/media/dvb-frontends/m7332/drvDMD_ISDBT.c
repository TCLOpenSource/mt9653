// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
//-----------------------------------------------------------------
//  Driver Compiler Options
//------------------------------------------------------------------


//-----------------------------------------------------------------
//  Include Files
//-----------------------------------------------------------------

#include "drvDMD_ISDBT.h"
#if DMD_ISDBT_UTOPIA2_EN
#include "drvDMD_ISDBT_v2.h"
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

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

#include "MsOS.h"
#include "drvMMIO.h"

#ifndef DONT_USE_CMA
#ifdef UTPA2
#include "drvCMAPool_v2.h"
#else
#include "drvCMAPool.h"
#endif
#include "halCHIP.h"
#endif // #ifndef DONT_USE_CMA

#else // #if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

#define DONT_USE_CMA

#endif // #if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

#if DMD_ISDBT_3PARTY_EN
// Please add header files needed by 3 party platform
#endif

//typedef u8 (*IOCTL)(DMD_ISDBT_IOCTL_DATA*,
//DMD_ISDBT_HAL_COMMAND, void*);

//static IOCTL _NULL_IOCTL_FP_ISDBT = NULL;

u8 (*_null_ioctl_fp_isdbt)(struct dmd_isdbt_ioctl_data*,
enum dmd_isdbt_hal_command, void*);

#ifdef MSOS_TYPE_LINUX_KERNEL
#if !DMD_ISDBT_UTOPIA_EN && DMD_ISDBT_UTOPIA2_EN
//typedef u8 (*SWI2C)(u8 u8BusNum, u8 u8SlaveID, u8 AddrCnt,
//u8 *pu8addr, u16 u16size, u8 *pBuf);

u8 (*_null_swi2c_fp_isdbt)(u8 bus_num, u8 slave_id,
u8 addrcnt, u8 *paddr, u16 size, u8 *pbuf);

//typedef u8 (*HWI2C)(u8 u8Port, u8 u8SlaveIdIIC,
//u8 u8AddrSizeIIC, u8 *pu8AddrIIC, u32 u32BufSizeIIC,
//u8 *pu8BufIIC);

u8 (*_null_hwi2c_fp_isdbt)(u8 port, u8 slave_id_iic,
u8 addr_size_iic, u8 *paddr_iic, u32 buf_size_iic, u8 *pbuf_iic);

//static SWI2C _NULL_SWI2C_FP = NULL;
//static HWI2C _NULL_HWI2C_FP = NULL;
#endif
#endif
static u8 _default_ioctl_cmd(struct dmd_isdbt_ioctl_data *pdata,
enum dmd_isdbt_hal_command ecmd, void *pargs)
{
	#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
	#ifdef REMOVE_HAL_INTERN_ISDBT
	PRINT("LINK ERROR: REMOVE_HAL_INTERN_ISDBT\n");
	#else
	PRINT("LINK ERROR: Please link ext demod library\n");
	#endif
	#else
	PRINT("LINK ERROR: Please link int or ext demod library\n");
	#endif
	return TRUE;
}

//#if (defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG) && \
//defined(CONFIG_MSTAR_CHIP))
#if !DMD_ISDBT_3PARTY_EN
static u8
(*hal_intern_isdbt_ioctl_cmd)(struct dmd_isdbt_ioctl_data *pdata,
enum dmd_isdbt_hal_command ecmd, void *pargs);
static u8
(*hal_extern_isdbt_ioctl_cmd)(struct dmd_isdbt_ioctl_data *pdata,
enum dmd_isdbt_hal_command ecmd, void *pargs);

static u8 (*mdrv_sw_iic_writebytes)(u8 bus_num, u8 slave_id,
u8 addrcnt, u8 *paddr, u16 size, u8 *pbuf);
static u8 (*mdrv_sw_iic_readbytes)(u8 bus_num, u8 slave_id,
u8 subaddr, u8 *paddr, u16 buf_len, u8 *pbuf);
static u8 (*mdrv_hw_iic_writebytes)(u8 port, u8 slave_id_iic,
u8 addr_size_iic, u8 *paddr_iic, u32 buf_size_iic, u8 *pbuf_iic);
static u8 (*mdrv_hw_iic_readbytes)(u8 port, u8 slave_id_iic,
u8 addr_size_iic, u8 *paddr_iic, u32 buf_size_iic, u8 *pbuf_iic);

#else

extern u8 hal_intern_isdbt_ioctl_cmd(struct dmd_isdbt_ioctl_data *pdata,
enum dmd_isdbt_hal_command ecmd, void *pargs); //__attribute__((weak));
u8 hal_extern_isdbt_ioctl_cmd(struct dmd_isdbt_ioctl_data *pdata,
enum dmd_isdbt_hal_command ecmd, void *pargs)  __attribute__((weak));

#ifndef MSOS_TYPE_LINUX_KERNEL

#else // #ifndef MSOS_TYPE_LINUX_KERNEL
#if !DMD_ISDBT_UTOPIA_EN && DMD_ISDBT_UTOPIA2_EN
u8 mdrv_sw_iic_writebytes(u8 bus_num, u8 slave_id,
u8 addrcnt, u8 *paddr, u16 size,
u8 *pbuf)  __attribute__((weak));
u8 mdrv_sw_iic_readbytes(u8 bus_num, u8 slave_id,
u8 subAddr, u8 *paddr, u16 buf_len,
u8 *pbuf)  __attribute__((weak));
u8 mdrv_hw_iic_writebytes(u8 port, u8 slave_id_iic,
u8 addr_size_iic, u8 *paddr_iic, u32 buf_size_iic,
u8 *pbuf_iic)  __attribute__((weak));
u8 mdrv_hw_iic_readbytes(u8 port, u8 slave_id_iic,
u8 addr_size_iic, u8 *paddr_iic, u32 buf_size_iic,
u8 *pbuf_iic)  __attribute__((weak));
#endif
#endif // #ifndef MSOS_TYPE_LINUX_KERNEL

#endif


//-----------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------

#if DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN
 //#define DMD_LOCK()      \
	//do { \
		//MS_ASSERT(MsOS_In_Interrupt() == FALSE); \
		//if (_s32DMD_ISDBT_Mutex == -1) \
			//return FALSE; \
		//if (_u8DMD_ISDBT_DbgLevel == DMD_ISDBT_DBGLV_DEBUG) \
			//PRINT("%s lock mutex\n", __func__); \
		//MsOS_ObtainMutex(_s32DMD_ISDBT_Mutex, MSOS_WAIT_FOREVER); \
		//} while (0)

 //#define DMD_UNLOCK()      \
	//do { \
		//MsOS_ReleaseMutex(_s32DMD_ISDBT_Mutex); \
		//if (_u8DMD_ISDBT_DbgLevel == DMD_ISDBT_DBGLV_DEBUG) \
			//PRINT("%s unlock mutex\n", __func__); \
		//} while (0)
#elif ((!DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN && \
DMD_ISDBT_MULTI_THREAD_SAFE))
 #define DMD_LOCK()  pio_data->sys.lock_dmd(TRUE)
 #define DMD_UNLOCK()  pio_data->sys.lock_dmd(FALSE)
#else
 #define DMD_LOCK()
 #define DMD_UNLOCK()
#endif

#ifdef MS_DEBUG
#define DMD_DBG(x)          (x)
#else
#define DMD_DBG(x)          (x)
#endif

#ifndef UTOPIA_STATUS_SUCCESS
#define UTOPIA_STATUS_SUCCESS               0x00000000
#endif
#ifndef UTOPIA_STATUS_FAIL
#define UTOPIA_STATUS_FAIL                  0x00000001
#endif

//-----------------------------------------------------------------
//  Local Structurs
//-----------------------------------------------------------------

//------------------------------------------------------------------
//  Global Variables
//------------------------------------------------------------------

#if DMD_ISDBT_UTOPIA2_EN
struct dmd_isdbt_resdata *psdmd_isdbt_resdata;

struct dmd_isdbt_ioctl_data dmd_isdbt_iodata = {0, 0, {0} };
#endif

//------------------------------------------------------------------
//  Local Variables
//------------------------------------------------------------------

#if !DMD_ISDBT_UTOPIA2_EN
static struct dmd_isdbt_resdata sdmd_isdbt_resdata[DMD_ISDBT_MAX_DEMOD_NUM]
= {{{0}, {0}, {0} } };

static struct dmd_isdbt_resdata *psdmd_isdbt_resdata = sdmd_isdbt_resdata;

static struct dmd_isdbt_ioctl_data dmd_isdbt_iodata = {0, 0, {0} };
#endif

static struct dmd_isdbt_ioctl_data *pio_data = &dmd_isdbt_iodata;

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
//static MSIF_Version _drv_dmd_isdbt_version;
#endif

#if DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN
static s32 _dmd_isdbt_mutex = -1;
#elif defined MTK_PROJECT
static HANDLE_T _dmd_isdbt_mutex = -1;
#elif DVB_STI
static s32 _dmd_isdbt_mutex = -1;
#endif

#ifndef DONT_USE_CMA
static struct CMA_Pool_Init_Param  _isdbt_cma_pool_init_param;
// for MApi_CMA_Pool_Init
static struct CMA_Pool_Alloc_Param _isdbt_cma_alloc_param;
// for MApi_CMA_Pool_GetMem
static struct CMA_Pool_Free_Param  _isdbt_cma_free_param;
// for MApi_CMA_Pool_PutMem
#endif

#ifdef UTPA2

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

static void *_pisdbt_instant;// = NULL;

static u32 _isdbt_open;// = 0;

#endif

#endif

static enum dmd_isdbt_dbglv _dmd_isdbt_dbglevel = DMD_ISDBT_DBGLV_NONE;

//----------------------------------------------------------------
//  Debug Functions
//----------------------------------------------------------------


//----------------------------------------------------------------
//  Local Functions
//----------------------------------------------------------------

static enum dmd_isdbt_lock_status _isdbt_checklock(void)
{
	u8 (*ioctl)(struct dmd_isdbt_ioctl_data *pdata,
	enum dmd_isdbt_hal_command ecmd, void *para);

	struct dmd_isdbt_resdata  *pres  = pio_data->pres;
	struct dmd_isdbt_initdata *pinit = &(pres->sdmd_isdbt_initdata);
	struct dmd_isdbt_info     *pinfo = &(pres->sdmd_isdbt_info);

	ioctl = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd;

	if (ioctl(pio_data, DMD_ISDBT_HAL_CMD_Check_FEC_Lock, NULL)) {
		pinfo->isdbt_lock_status |= DMD_ISDBT_LOCK_FEC_LOCK;
		#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
		pinfo->isdbt_fec_lock_time = MsOS_GetSystemTime();
		#else
		pinfo->isdbt_fec_lock_time = pio_data->sys.get_system_time_ms();
		#endif
		return DMD_ISDBT_LOCK;
	} /*else {*/
		#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
		if
	(!((pinfo->isdbt_lock_status & DMD_ISDBT_LOCK_FEC_LOCK) &&
	((MsOS_GetSystemTime()-pinfo->isdbt_fec_lock_time) < 100)))
		#else
		if
	(!((pinfo->isdbt_lock_status & DMD_ISDBT_LOCK_FEC_LOCK) &&
	((pio_data->sys.get_system_time_ms()-pinfo->isdbt_fec_lock_time)
	< 100)))
		#endif
			pinfo->isdbt_lock_status &= (~DMD_ISDBT_LOCK_FEC_LOCK);
		else
			return DMD_ISDBT_LOCK;

		if (pinfo->isdbt_lock_status &
			DMD_ISDBT_LOCK_ICFO_CH_EXIST_LOCK) {//STEP 3
			#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
			if
			((MsOS_GetSystemTime()-pinfo->isdbt_scan_time_start) <
				pinit->isdbt_fec_lock_check_time)
			#else
			if
	      ((pio_data->sys.get_system_time_ms()-pinfo->isdbt_scan_time_start)
	      < pinit->isdbt_fec_lock_check_time)
			#endif
				return DMD_ISDBT_CHECKING;
		} else {//STEP 1,2
			if (ioctl(pio_data,
				DMD_ISDBT_HAL_CMD_Check_ICFO_CH_EXIST_Lock,
				NULL)) {
				pinfo->isdbt_lock_status |=
				DMD_ISDBT_LOCK_ICFO_CH_EXIST_LOCK;
				if
				(_dmd_isdbt_dbglevel >=
				DMD_ISDBT_DBGLV_DEBUG) {
				PRINT("DMD_ISDBT_LOCK_ICFO_CH_EXIST_LOCK\n");
				}
				#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
				pinfo->isdbt_scan_time_start =
				MsOS_GetSystemTime();
				#else
				pinfo->isdbt_scan_time_start =
				pio_data->sys.get_system_time_ms();
				#endif
			} else {
				#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
				if
	((MsOS_GetSystemTime()-pinfo->isdbt_scan_time_start) <
					pinit->isdbt_icfo_ch_exist_check_time)
				#else
				if
	((pio_data->sys.get_system_time_ms()-pinfo->isdbt_scan_time_start)
					< pinit->isdbt_icfo_ch_exist_check_time)
				#endif
					return DMD_ISDBT_CHECKING;
			}
			return DMD_ISDBT_CHECKING;
		}
		return DMD_ISDBT_UNLOCK;
	//}
}

#if defined MTK_PROJECT

static u32 _get_system_time_ms(void)
{
	u32 current_time = 0;

	current_time = x_os_get_sys_tick() * x_os_drv_get_tick_period();

	return current_time;
}

static void _delayms(u32 ms)
{
	mcDELAY_US(ms*1000);
}

static u8 _create_mutex(u8 enable)
{
	if (enable) {
		if (_dmd_isdbt_mutex == -1) {
			if (x_sema_create(&_dmd_isdbt_mutex,
				X_SEMA_TYPE_MUTEX,
				X_SEMA_STATE_UNLOCK) != OSR_OK) {
				PRINT("%s:  x_sema_create Fail!\n", __func__);
				return FALSE;
			}
		}
	} else {
		x_sema_delete(_dmd_isdbt_mutex);

		_dmd_isdbt_mutex = -1;
	}

	return TRUE;
}

static void _lock_dmd(u8 enable)
{
	if (enable)
		x_sema_lock(_dmd_isdbt_mutex, X_SEMA_OPTION_WAIT);
	else
		x_sema_unlock(_dmd_isdbt_mutex);
}

#elif DVB_STI

static u8 _create_mutex(u8 enable)
{

	return TRUE;
}

static void _lock_dmd(u8 enable)
{

}

static u32 _get_system_time_ms(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);

	return (ts.tv_sec*1000 + ts.tv_nsec/1000000);
}

//-----------------------------------------------------------------
/// Delay for u32Us microseconds
/// @param  u32Us  \b IN: delay 0 ~ 999 us
/// @return None
/// @note   implemented by "busy waiting".
///         Plz call MsOS_DelayTask directly for ms-order delay
//-----------------------------------------------------------------

static void _delayms(u32 ms)
{
	mtk_demod_delay_us(ms * 1000UL);
}

#elif !DMD_ISDBT_UTOPIA_EN && DMD_ISDBT_UTOPIA2_EN

#ifdef MSOS_TYPE_LINUX_KERNEL
static u8 _sw_i2c_writebytes(u16 bus_num_slave_id,
u8 addrcount, u8 *paddr, u16 size, u8 *pdata)
{
	if (mdrv_sw_iic_writebytes != NULL) {
		if (mdrv_sw_iic_writebytes((bus_num_slave_id>>8)&0x0F,
			bus_num_slave_id, addrcount, paddr, size, pdata) != 0)
			return FALSE;
		else
			return TRUE;
	} else {
		PRINT(
			"LINK ERROR: I2c function is missing and please check it\n");

		return FALSE;
	}
}

static u8 _sw_i2c_readbytes(u16 bus_num_slave_id,
u8 addrnum, u8 *paddr, u16 size, u8 *pdata)
{
	if (mdrv_sw_iic_readbytes != NULL) {
		if (mdrv_sw_iic_readbytes((bus_num_slave_id>>8)&0x0F,
			bus_num_slave_id, addrnum, paddr, size, pdata) != 0)
			return FALSE;
		else
			return TRUE;
	} else {
		PRINT(
			"LINK ERROR: I2c function is missing and please check it\n");

		return FALSE;
	}
}

static u8 _hw_i2c_writebytes(u16 bus_num_slave_id,
u8 addrcount, u8 *paddr, u16 size, u8 *pdata)
{
	if (mdrv_hw_iic_writebytes != NULL) {
		if (mdrv_hw_iic_writebytes((bus_num_slave_id>>12)&0x03,
			bus_num_slave_id, addrcount, paddr, size, pdata) != 0)
			return FALSE;
		else
			return TRUE;
	} else {
		PRINT(
			"LINK ERROR: I2c function is missing and please check it\n");

		return FALSE;
	}
}

static u8 _hw_i2c_readbytes(u16 bus_num_slave_id,
u8 addrnum, u8 *paddr, u16 size, u8 *pdata)
{
	if (mdrv_hw_iic_readbytes != NULL) {
		if (mdrv_hw_iic_readbytes((bus_num_slave_id>>12)&0x03,
			bus_num_slave_id, addrnum, paddr, size, pdata) != 0)
			return FALSE;
		else
			return TRUE;
	} else {
		PRINT(
			"LINK ERROR: I2c function is missing and please check it\n");

		return FALSE;
	}
}
#endif

#endif

static void _update_io_data(u8 id, struct dmd_isdbt_resdata *pres)
{
	#if !DMD_ISDBT_UTOPIA_EN && DMD_ISDBT_UTOPIA2_EN
	#ifdef MSOS_TYPE_LINUX_KERNEL
	//sDMD_ISDBT_InitData.u8I2CSlaveBus
	//bit7    0 : old format 1 : new format
	//sDMD_ISDBT_InitData.u8I2CSlaveBus
	//bit6 0 : SW I2C     1 : HW I2C
	//sDMD_ISDBT_InitData.u8I2CSlaveBus
	//bit4&5  HW I2C port number

	if ((pres->sdmd_isdbt_initdata.i2c_slave_bus&0x80) == 0x80) {
		//Use Kernel I2c
		if ((pres->sdmd_isdbt_initdata.i2c_slave_bus&0x40)
			== 0x40) {//HW I2c
			pres->sdmd_isdbt_initdata.i2c_writebytes =
			_hw_i2c_writebytes;
			pres->sdmd_isdbt_initdata.i2c_readbytes  =
			_hw_i2c_readbytes;
		} else {//SW I2c
			pres->sdmd_isdbt_initdata.i2c_writebytes =
			_sw_i2c_writebytes;
			pres->sdmd_isdbt_initdata.i2c_readbytes  =
			_sw_i2c_readbytes;
		}
	}
	#endif
	#endif // #if !DMD_ISDBT_UTOPIA_EN && DMD_ISDBT_UTOPIA2_EN

	pio_data->id   = id;
	pio_data->pres = pres;
}

//---------------------------------------------------------------
//  Global Functions
//---------------------------------------------------------------

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_setdbglevel(enum dmd_isdbt_dbglv dbglv)
#else
u8 mdrv_dmd_isdbt_setdbglevel(enum dmd_isdbt_dbglv dbglv)
#endif
{
	_dmd_isdbt_dbglevel = dbglv;

	return TRUE;
}

#ifdef UTPA2
struct dmd_isdbt_info *_mdrv_dmd_isdbt_getinfo(void)
#else
struct dmd_isdbt_info *mdrv_dmd_isdbt_getinfo(void)
#endif
{
	psdmd_isdbt_resdata->sdmd_isdbt_info.isdbt_version = 0;

	return &(psdmd_isdbt_resdata->sdmd_isdbt_info);
}

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

////////////////////////////////////////////////////////////////////////////////
///                            SINGLE DEMOD API                              ///
////////////////////////////////////////////////////////////////////////////////

u8 mdrv_dmd_isdbt_init(struct dmd_isdbt_initdata *pdmd_isdbt_initdata,
u32 init_data_len)
{
	return mdrv_dmd_isdbt_md_init(0, pdmd_isdbt_initdata, init_data_len);
}

u8 mdrv_dmd_isdbt_exit(void)
{
	return mdrv_dmd_isdbt_md_exit(0);
}

u32 mdrv_dmd_isdbt_getconfig(
struct dmd_isdbt_initdata *psdmd_isdbt_initdata)
{
	return mdrv_dmd_isdbt_md_getconfig(0, psdmd_isdbt_initdata);
}

u8 mdrv_dmd_isdbt_getfwver(u16 *fw_ver)
{
	return TRUE;
}

void mdrv_dmd_isdbt_setreset(void)
{
	return mdrv_dmd_isdbt_md_setreset(0);
}

u8 mdrv_dmd_isdbt_advsetconfig(enum dmd_isdbt_demod_type etype,
u8 benable)
{
	return mdrv_dmd_isdbt_md_advsetconfig(0, etype, benable);
}

u8 mdrv_dmd_isdbt_setconfig(u8 benable)
{
	return mdrv_dmd_isdbt_md_setconfig(0, benable);
}

u8 mdrv_dmd_isdbt_setactive(u8 benable)
{
	return mdrv_dmd_isdbt_md_setactive(0, benable);
}

#if DMD_ISDBT_STR_EN
u32 mdrv_dmd_isdbt_setpowerstate(enum EN_POWER_MODE power_state)
{
	return mdrv_dmd_isdbt_md_setpowerstate(0, power_state);
}
#endif

enum dmd_isdbt_lock_status mdrv_dmd_isdbt_getlock(
enum dmd_isdbt_getlock_type etype)
{
	return mdrv_dmd_isdbt_md_getlock(0, etype);
}

u8 mdrv_dmd_isdbt_get_modulation_mode(enum en_isdbt_layer elayer_index,
struct isdbt_modulation_mode *sisdbt_modulation_mode)
{
	return mdrv_dmd_isdbt_md_get_modulation_mode(0, elayer_index,
	sisdbt_modulation_mode);
}

u8 mdrv_dmd_isdbt_get_signalstrength(u16 *strength)
{
	return mdrv_dmd_isdbt_md_get_signalstrength(0, strength);
}

#ifndef MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_isdbt_getfreqoffset(float *freq_offset)
#else
u8 mdrv_dmd_isdbt_getfreqoffset(struct dmd_isdbt_cfo_data *freq_offset)
#endif
{
	return mdrv_dmd_isdbt_md_getfreqoffset(0, freq_offset);
}

u16 mdrv_dmd_isdbt_get_signalquality(void)
{
	return mdrv_dmd_isdbt_md_get_signalquality(0);
}

u16 mdrv_dmd_isdbt_get_signalqualityoflayerA(void)
{
	return mdrv_dmd_isdbt_md_get_signalqualityoflayerA(0);
}

u16 mdrv_dmd_isdbt_get_signalqualityoflayerB(void)
{
	return mdrv_dmd_isdbt_md_get_signalqualityoflayerB(0);
}

u16 mdrv_dmd_isdbt_get_signalqualityoflayerC(void)
{
	return mdrv_dmd_isdbt_md_get_signalqualityoflayerC(0);
}

u16 mdrv_dmd_isdbt_get_signalqualitycombine(void)
{
	return mdrv_dmd_isdbt_md_get_signalqualitycombine(0);
}

#ifndef MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_isdbt_getsnr(float *f_snr)
#else
u8 mdrv_dmd_isdbt_getsnr(struct dmd_isdbt_snr_data *f_snr)
#endif
{
	return mdrv_dmd_isdbt_md_getsnr(0, f_snr);
}

#ifndef MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_isdbt_get_previterbi_ber(enum en_isdbt_layer elayer_index,
float *fber)
#else
u8 mdrv_dmd_isdbt_get_previterbi_ber(enum en_isdbt_layer elayer_index,
struct dmd_isdbt_get_ber_value *fber)
#endif
{
	return mdrv_dmd_isdbt_md_get_previterbi_ber(0, elayer_index, fber);
}

#ifndef MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_isdbt_get_postviterbi_ber(enum en_isdbt_layer elayer_index,
float *fber)
#else
u8 mdrv_dmd_isdbt_get_postviterbi_ber(enum en_isdbt_layer elayer_index,
struct dmd_isdbt_get_ber_value *fber)
#endif
{
	return mdrv_dmd_isdbt_md_get_postviterbi_ber(0, elayer_index, fber);
}

u8 mdrv_dmd_isdbt_read_pkt_err(enum en_isdbt_layer elayer_index,
u16 *packet_err)
{
	return mdrv_dmd_isdbt_md_read_pkt_err(0, elayer_index, packet_err);
}

u8 mdrv_dmd_isdbt_set_serialcontrol(u8 ts_config_data)
{
	return mdrv_dmd_isdbt_md_set_serialcontrol(0, ts_config_data);
}

u8 mdrv_dmd_isdbt_iic_bypass_mode(u8 benable)
{
	return mdrv_dmd_isdbt_md_iic_bypass_mode(0, benable);
}

u8 mdrv_dmd_isdbt_switch_sspi_gpio(u8 benable)
{
	return mdrv_dmd_isdbt_md_switch_sspi_gpio(0, benable);
}

u8 mdrv_dmd_isdbt_gpio_get_level(u8 pin, u8 *blevel)
{
	return mdrv_dmd_isdbt_md_gpio_get_level(0, pin, blevel);
}

u8 mdrv_dmd_isdbt_gpio_set_level(u8 pin, u8 blevel)
{
	return mdrv_dmd_isdbt_md_gpio_set_level(0, pin, blevel);
}

u8 mdrv_dmd_isdbt_gpio_out_enable(u8 pin, u8 enable_out)
{
	return mdrv_dmd_isdbt_md_gpio_out_enable(0, pin, enable_out);
}

u8 mdrv_dmd_isdbt_do_iq_swap(u8 bis_qpad)
{
	return mdrv_dmd_isdbt_md_do_iq_swap(0, bis_qpad);
}

u8 mdrv_dmd_isdbt_getreg(u16 addr, u8 *pdata)
{
	return mdrv_dmd_isdbt_md_getreg(0, addr, pdata);
}

u8 mdrv_dmd_isdbt_setreg(u16 addr, u8 data)
{
	return mdrv_dmd_isdbt_md_setreg(0, addr, data);
}

#endif // #if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

////////////////////////////////////////////////////////////////////////////////
///                            MULTI DEMOD API                               ///
////////////////////////////////////////////////////////////////////////////////

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_init(u8 id,
struct dmd_isdbt_initdata *pdmd_isdbt_initdata, u32 init_data_len)
#else
u8 mdrv_dmd_isdbt_md_init(u8 id,
struct dmd_isdbt_initdata *pdmd_isdbt_initdata, u32 init_data_len)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	#if (defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG) && \
	defined(CONFIG_MSTAR_CHIP))
	const struct kernel_symbol *sym;
	#endif
	#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
	u64 phy_nonpm_bank_size;
	#endif
	#ifndef DONT_USE_CMA
	u32  phy_addr_fromVA = 0, heap_start_PA_addr = 0;
	#endif
	u8 benable = FALSE;

	if (!(pres->sdmd_isdbt_pridata.binit)) {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		if (sizeof(struct dmd_isdbt_initdata) == init_data_len)
			MEMCPY(&(pres->sdmd_isdbt_initdata),
			pdmd_isdbt_initdata, init_data_len);
		else {
			DMD_DBG(PRINT(
		"MDrv_DMD_ISDBT_Init input data structure incorrect\n"));
			return FALSE;
		}
		#else
		MEMCPY((&(pres->sdmd_isdbt_initdata)),
		pdmd_isdbt_initdata, sizeof(struct dmd_isdbt_initdata));
		#endif
	} else {
		pres->sdmd_isdbt_initdata.area_code_addr_set0 =
		pdmd_isdbt_initdata->area_code_addr_set0;
		pres->sdmd_isdbt_initdata.area_code_addr_set1 =
		pdmd_isdbt_initdata->area_code_addr_set1;
		pres->sdmd_isdbt_initdata.area_code_addr_set2 =
		pdmd_isdbt_initdata->area_code_addr_set2;
		pres->sdmd_isdbt_initdata.area_code_addr_set3 =
		pdmd_isdbt_initdata->area_code_addr_set3;

		DMD_DBG(PRINT("MDrv_DMD_ISDBT_Init more than once\n"));

		return TRUE;
	}

	#if defined MTK_PROJECT
	pio_data->sys.get_system_time_ms  = _get_system_time_ms;
	pio_data->sys.delayms          = _delayms;
	pio_data->sys.create_mutex      = _create_mutex;
	pio_data->sys.lock_dmd          = _lock_dmd;
	#elif DVB_STI
	pio_data->sys.get_system_time_ms  = _get_system_time_ms;
	pio_data->sys.delayms          = _delayms;
	pio_data->sys.create_mutex      = _create_mutex;
	pio_data->sys.lock_dmd          = _lock_dmd;
	#elif DMD_ISDBT_3PARTY_EN
	// Please initial mutex, timer
	//and MSPI function pointer for 3 party platform
	// pIOData->sys.GetSystemTimeMS  = ;
	// pIOData->sys.DelayMS          = ;
	// pIOData->sys.CreateMutex      = ;
	// pIOData->sys.LockDMD          = ;
	// pIOData->sys.MSPI_Init_Ext    = ;
	// pIOData->sys.MSPI_SlaveEnable = ;
	// pIOData->sys.MSPI_Write       = ;
	// pIOData->sys.MSPI_Read        = ;
	#elif !DMD_ISDBT_UTOPIA2_EN
	pio_data->sys.delayms           = MsOS_DelayTask;
	pio_data->sys.mspi_init_ext     = MDrv_MSPI_Init_Ext;
	pio_data->sys.mspi_slave_enable  = MDrv_MSPI_SlaveEnable;
	pio_data->sys.mspi_write        = MDrv_MSPI_Write;
	pio_data->sys.mspi_read         = MDrv_MSPI_Read;
	pio_data->sys.miu_is_support_miu1 = MDrv_MIU_IsSupportMIU1;
	pio_data->sys.miu_sel_miu        = MDrv_MIU_SelMIU;
	pio_data->sys.miu_unmask_req     = MDrv_MIU_UnMaskReq;
	pio_data->sys.miu_mask_req       = MDrv_MIU_MaskReq;
	#endif

	#if DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN
	if (_dmd_isdbt_mutex == -1) {
		_dmd_isdbt_mutex = MsOS_CreateMutex(E_MSOS_FIFO,
		"Mutex DMD ISDBT", MSOS_PROCESS_SHARED);
		if (_dmd_isdbt_mutex == -1) {
			DMD_DBG(PRINT(
				"MDrv_DMD_ISDBT_Init Create Mutex Fail\n"));
			return FALSE;
		}
	}
	#elif (!DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN && \
	DMD_ISDBT_MULTI_THREAD_SAFE)
	if (!pio_data->sys.create_mutex(TRUE)) {
		DMD_DBG(PRINT("MDrv_DMD_ISDBT_Init Create Mutex Fail\n"));
		return FALSE;
	}
	#endif

	#ifdef MS_DEBUG
	if (_dmd_isdbt_dbglevel >= DMD_ISDBT_DBGLV_INFO)
		PRINT("MDrv_DMD_ISDBT_Init\n");
	#endif

	#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
	if (!MDrv_MMIO_GetBASE(&(pres->sdmd_isdbt_pridata.virt_dmd_base_addr),
		&phy_nonpm_bank_size, MS_MODULE_PM)) {
		#ifdef MS_DEBUG
		PRINT("HAL_DMD_RegInit failure to get MS_MODULE_PM\n");
		#endif

		return FALSE;
	}
	#endif

	#if DVB_STI
	pres->sdmd_isdbt_pridata.virt_dmd_base_addr =
	(((size_t *)(pres->sdmd_isdbt_initdata.dmd_isdbt_init_ext)));
	#endif
	pres->sdmd_isdbt_pridata.binit = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	#if DMD_ISDBT_STR_EN
	if (pres->sdmd_isdbt_pridata.elast_state == 0) {
		pres->sdmd_isdbt_pridata.elast_state = E_POWER_RESUME;
		pres->sdmd_isdbt_pridata.elast_type = DMD_ISDBT_DEMOD;
	}
	#else
	pres->sdmd_isdbt_pridata.elast_type = DMD_ISDBT_DEMOD
	#endif

#if !DMD_ISDBT_3PARTY_EN
	#if (defined(MSOS_TYPE_LINUX_KERNEL) && defined(CONFIG_CC_IS_CLANG) && \
	defined(CONFIG_MSTAR_CHIP))
	#ifdef CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
	sym = find_symbol("hal_intern_isdbt_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym != NULL)
		hal_intern_isdbt_ioctl_cmd =
		(u8 (*)(struct dmd_isdbt_ioctl_data*,
		enum dmd_isdbt_hal_command, void*))(
		(unsigned long)offset_to_ptr(&sym->value_offset));
	else
		hal_intern_isdbt_ioctl_cmd = _null_ioctl_fp_isdbt;
	sym = find_symbol("hal_extern_isdbt_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym != NULL)
		hal_extern_isdbt_ioctl_cmd =
		(u8 (*)(struct dmd_isdbt_ioctl_data*,
		enum dmd_isdbt_hal_command, void*))(
		(unsigned long)offset_to_ptr(&sym->value_offset));
	else
		hal_extern_isdbt_ioctl_cmd = _null_ioctl_fp_isdbt;
	sym = find_symbol("mdrv_sw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym != NULL)
		mdrv_sw_iic_writebytes =
		(u8 (*)(u8, u8, u8, u8 *, u16, u8 *))(
		(unsigned long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_sw_iic_writebytes = _null_swi2c_fp_isdbt;
	sym = find_symbol("mdrv_sw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym != NULL)
		mdrv_sw_iic_readbytes =
		(u8 (*)(u8, u8, u8, u8 *, u16, u8 *))(
		(unsigned long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_sw_iic_readbytes = _null_swi2c_fp_isdbt;
	sym = find_symbol("mdrv_hw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym != NULL)
		mdrv_hw_iic_writebytes =
		(u8 (*)(u8, u8, u8, u8 *, u32, u8 *))(
		(unsigned long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_hw_iic_writebytes = _null_hwi2c_fp_isdbt;
	sym = find_symbol("mdrv_hw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym != NULL)
		mdrv_hw_iic_readbytes =
		(u8 (*)(u8, u8, u8, u8 *, u32, u8 *))(
		(unsigned long)offset_to_ptr(&sym->value_offset));
	else
		mdrv_hw_iic_readbytes = _null_hwi2c_fp_isdbt;
	#else // #ifdef CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
	sym = find_symbol("hal_intern_isdbt_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym != NULL)
		hal_intern_isdbt_ioctl_cmd =
		(u8 (*)(struct dmd_isdbt_ioctl_data*,
		enum dmd_isdbt_hal_command, void*))(sym->value);
	else
		hal_intern_isdbt_ioctl_cmd = _null_ioctl_fp_isdbt;
	sym = find_symbol("hal_extern_isdbt_ioctl_cmd", NULL, NULL, 0, 0);
	if (sym != NULL)
		hal_extern_isdbt_ioctl_cmd =
		(u8 (*)(struct dmd_isdbt_ioctl_data*,
		enum dmd_isdbt_hal_command, void*))(sym->value);
	else
		hal_extern_isdbt_ioctl_cmd = _null_ioctl_fp_isdbt;
	sym = find_symbol("mdrv_sw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym != NULL)
		mdrv_sw_iic_writebytes =
		(u8 (*)(u8, u8, u8, u8 *,
		u16, u8 *))(sym->value);
	else
		mdrv_sw_iic_writebytes = _null_swi2c_fp_isdbt;
	sym = find_symbol("mdrv_sw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym != NULL)
		mdrv_sw_iic_readbytes =
		(u8 (*)(u8, u8, u8, u8 *,
		u16, u8 *))(sym->value);
	else
		mdrv_sw_iic_readbytes = _null_swi2c_fp_isdbt;
	sym = find_symbol("mdrv_hw_iic_writebytes", NULL, NULL, 0, 0);
	if (sym != NULL)
		mdrv_hw_iic_writebytes =
		(u8 (*)(u8, u8, u8, u8 *,
		u32, u8 *))(sym->value);
	else
		mdrv_hw_iic_writebytes = _null_hwi2c_fp_isdbt;
	sym = find_symbol("mdrv_hw_iic_readbytes", NULL, NULL, 0, 0);
	if (sym != NULL)
		mdrv_hw_iic_readbytes =
		(u8 (*)(u8, u8, u8, u8 *,
		u32, u8 *))(sym->value);
	else
		mdrv_hw_iic_readbytes = _null_hwi2c_fp_isdbt;
	#endif
#endif
#endif

#if !DMD_ISDBT_3PARTY_EN
	if (pres->sdmd_isdbt_initdata.is_ext_demod) {
		if (hal_extern_isdbt_ioctl_cmd == NULL)
			pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd =
			_default_ioctl_cmd;
		else
			pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd =
			hal_extern_isdbt_ioctl_cmd;
	} else {
		if (hal_intern_isdbt_ioctl_cmd == NULL)
			pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd =
			_default_ioctl_cmd;
		else
			pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd =
			hal_intern_isdbt_ioctl_cmd;
	}
#else
	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd =
	hal_intern_isdbt_ioctl_cmd;
#endif

	#ifndef MSOS_TYPE_LINUX_KERNEL
	#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
	if (HAL_EXTERN_ISDBT_IOCTL_CMD_0 == NULL)
		EXT_ISDBT_IOCTL[0] = _default_ioctl_cmd;
	if (HAL_EXTERN_ISDBT_IOCTL_CMD_1 == NULL)
		EXT_ISDBT_IOCTL[1] = _default_ioctl_cmd;
	#endif
	#endif // #ifndef MSOS_TYPE_LINUX_KERNEL

	#ifndef DONT_USE_CMA
	if (pres->sdmd_isdbt_initdata.is_ext_demod)
		_isdbt_cma_pool_init_param.heap_id = 0x7F;
	else if (((pres->sdmd_isdbt_initdata.tdi_start_addr) &
		0x80000000) == 0x80000000) {
		_isdbt_cma_pool_init_param.heap_id =
		((pres->sdmd_isdbt_initdata.tdi_start_addr & 0x7F000000)>>24);
	} else {
		_isdbt_cma_pool_init_param.heap_id = 26;
	}

	if (_isdbt_cma_pool_init_param.heap_id != 0x7F) {
		_isdbt_cma_pool_init_param.flags = CMA_FLAG_MAP_VMA;

		if (MApi_CMA_Pool_Init(&_isdbt_cma_pool_init_param) == FALSE)
			DMD_DBG(PRINT("\033[35mFunction = %s, Line = %d,
			get MApi_CMA_Pool_Init ERROR!!!\n", __PRETTY_FUNCTION__,
			__LINE__));
		else {
			DMD_DBG(PRINT("\033[35mFunction = %s, Line = %d,
			get pool_handle_id is %u\033[m\n", __PRETTY_FUNCTION__,
			__LINE__, _isdbt_cma_pool_init_param.pool_handle_id));
			DMD_DBG(PRINT("\033[35mFunction = %s, Line = %d,
			get miu is %u\033[m\n", __PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_pool_init_param.miu));
			DMD_DBG(PRINT("\033[35mFunction = %s, Line = %d,
			get heap_miu_start_offset is 0x%llX\033[m\n",
			__PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_pool_init_param.heap_miu_start_offset));
			DMD_DBG(PRINT("\033[35mFunction = %s, Line = %d,
			get heap_length is 0x%X\033[m\n", __PRETTY_FUNCTION__,
			__LINE__, _isdbt_cma_pool_init_param.heap_length));
		}

		_miu_offset_to_phy(_isdbt_cma_pool_init_param.miu,
		_isdbt_cma_pool_init_param.heap_miu_start_offset,
		heap_start_PA_addr);

		_isdbt_cma_alloc_param.pool_handle_id =
		_isdbt_cma_pool_init_param.pool_handle_id;

		if (((pres->sdmd_isdbt_initdata.tdi_start_addr) &
			0x80000000) == 0x80000000) {
			pres->sdmd_isdbt_initdata.tdi_start_addr =
			((pres->sdmd_isdbt_initdata.tdi_start_addr) &
			0x00FFFFFF);
			_isdbt_cma_alloc_param.offset_in_pool =
			(pres->sdmd_isdbt_initdata.tdi_start_addr * 4096) -
			heap_start_PA_addr;
			pres->sdmd_isdbt_initdata.tdi_start_addr =
			pres->sdmd_isdbt_initdata.tdi_start_addr*256;
		} else {
			_isdbt_cma_alloc_param.offset_in_pool =
			(pres->sdmd_isdbt_initdata.tdi_start_addr * 16) -
			heap_start_PA_addr;
		}

		_isdbt_cma_alloc_param.length =
		pres->sdmd_isdbt_pridata.tdi_dram_size;
		_isdbt_cma_alloc_param.flags = CMA_FLAG_VIRT_ADDR;

		if (MApi_CMA_Pool_GetMem(&_isdbt_cma_alloc_param) == FALSE)
			DMD_DBG(PRINT("\033[35mFunction = %s, Line = %d,
			CMA_Pool_GetMem ERROR!!\033[m\n", __PRETTY_FUNCTION__,
			__LINE__));
		else {
			DMD_DBG(PRINT(
			"\033[35mFunction = %s, Line = %d, get from heap_id %d\033[m\n",
			__PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_alloc_param.pool_handle_id));
			DMD_DBG(PRINT(
			"\033[35mFunction = %s, Line = %d, ask offset 0x%llX\033[m\n",
			__PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_alloc_param.offset_in_pool));
			DMD_DBG(PRINT(
			"\033[35mFunction = %s, Line = %d, ask length 0x%X\033[m\n",
			__PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_alloc_param.length));
			DMD_DBG(PRINT(
			"\033[37mFunction = %s, Line = %d, return va is 0x%X\033[m\n",
			__PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_alloc_param.virt_addr));
		}

		phy_addr_fromVA =
		MsOS_MPool_VA2PA(_isdbt_cma_alloc_param.virt_addr);

		DMD_DBG(PRINT("#######u32PhysicalAddr_FromVA1 = [0x%x]\n",
		phy_addr_fromVA));

		_miu_offset_to_phy(_isdbt_cma_pool_init_param.miu,
		_isdbt_cma_pool_init_param.heap_miu_start_offset +
		_isdbt_cma_alloc_param.offset_in_pool,
						   phy_addr_fromVA);

		DMD_DBG(PRINT("#######u32PhysicalAddr_FromVA2 = [0x%x]\n",
		phy_addr_fromVA));

		DMD_DBG(PRINT(
		"#######pRes->sDMD_ISDBT_InitData.u32TdiStartAddr =
		[0x%x]\n", (pres->sdmd_isdbt_initdata.tdi_start_addr * 16)));
	} else {//_ISDBT_CMA_Pool_Init_PARAM.heap_id = 0x7F, bypass CMA
		pres->sdmd_isdbt_initdata.tdi_start_addr =
		(pres->sdmd_isdbt_initdata.tdi_start_addr & 0x00FFFFFF) * 256;

		DMD_DBG(PRINT(
		"#######pRes->sDMD_ISDBT_InitData.u32TdiStartAddr = [0x%x]\n",
		(pres->sdmd_isdbt_initdata.tdi_start_addr)));
	}
	#else
	if (((pres->sdmd_isdbt_initdata.tdi_start_addr)&0x80000000) ==
		0x80000000) {
		DMD_DBG(PRINT("\033[35mFunction = %s, Line = %d, %s, %s",
		__PRETTY_FUNCTION__, __LINE__,
		"Current Utopia don't support CMA",
		"but AP use CMA mode!!!\n"));

		pres->sdmd_isdbt_initdata.tdi_start_addr =
		(pres->sdmd_isdbt_initdata.tdi_start_addr & 0x00FFFFFF) * 256;

		DMD_DBG(PRINT(
		"#######pRes->sDMD_ISDBT_InitData.u32TdiStartAddr = [0x%x]\n",
		(pres->sdmd_isdbt_initdata.tdi_start_addr)));
	}
	#endif // #ifndef DONT_USE_CMA

	_update_io_data(id, pres);
	// Disable i2c bypass for EWBS mode 2&3
	if (pres->sdmd_isdbt_initdata.ewbs_mode)
		pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
		DMD_ISDBT_HAL_CMD_IIC_Bypass_Mode, &benable);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_InitClk, NULL);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_Download, NULL);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_TS_INTERFACE_CONFIG, NULL);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_FWVERSION, NULL);

	DMD_UNLOCK();

	return TRUE;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_exit(u8 id)
#else
u8 mdrv_dmd_isdbt_md_exit(u8 id)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;
	u8 benable = TRUE;

	if (pres->sdmd_isdbt_pridata.binit == FALSE)
		return ret;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);
	// Eanble i2c bypass for EWBS mode 2&3
	if (pres->sdmd_isdbt_initdata.ewbs_mode)
		pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
		DMD_ISDBT_HAL_CMD_IIC_Bypass_Mode, &benable);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_Exit, NULL);

	pres->sdmd_isdbt_pridata.binit = FALSE;

	#ifndef DONT_USE_CMA
	if (pres->sdmd_isdbt_initdata.is_ext_demod)
		_isdbt_cma_pool_init_param.heap_id = 0x7F;
	else if (((pres->sdmd_isdbt_initdata.tdi_start_addr) &
		0x80000000) == 0x80000000)
		_isdbt_cma_pool_init_param.heap_id =
		((pres->sdmd_isdbt_initdata.tdi_start_addr & 0x7F000000)>>24);
	else {
		_isdbt_cma_pool_init_param.heap_id = 26;
	}

	if (_isdbt_cma_pool_init_param.heap_id != 0x7F) {
		_isdbt_cma_free_param.pool_handle_id =
		_isdbt_cma_pool_init_param.pool_handle_id;
		_isdbt_cma_free_param.offset_in_pool =
		_isdbt_cma_alloc_param.offset_in_pool;
		_isdbt_cma_free_param.length =
		pres->sdmd_isdbt_pridata.tdi_dram_size;

		if (MApi_CMA_Pool_PutMem(&_isdbt_cma_free_param) == FALSE)
			DMD_DBG(PRINT(
			"\033[35mFunction = %s, Line = %d, MsOS_CMA_Pool_Release ERROR!!\033[m\n",
			__PRETTY_FUNCTION__, __LINE__));
		else {
			DMD_DBG(PRINT(
			"\033[35mFunction = %s, Line = %d, get from heap_id %d\033[m\n",
			__PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_free_param.pool_handle_id));
			DMD_DBG(PRINT(
			"\033[35mFunction = %s, Line = %d, ask offset 0x%llX\033[m\n",
			__PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_free_param.offset_in_pool));
			DMD_DBG(PRINT(
			"\033[35mFunction = %s, Line = %d, ask length 0x%X\033[m\n",
			__PRETTY_FUNCTION__, __LINE__,
			_isdbt_cma_free_param.length));
		}
	}
	#endif
	DMD_UNLOCK();

	#if DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN
	MsOS_DeleteMutex(_dmd_isdbt_mutex);

	_dmd_isdbt_mutex = -1;
	#elif (!DMD_ISDBT_UTOPIA_EN && !DMD_ISDBT_UTOPIA2_EN && \
	DMD_ISDBT_MULTI_THREAD_SAFE)
	pio_data->sys.create_mutex(FALSE);
	#endif

	return ret;
}

#ifdef UTPA2
u32 _mdrv_dmd_isdbt_md_getconfig(u8 id,
struct dmd_isdbt_initdata *psdmd_isdbt_initdata)
#else
u32 mdrv_dmd_isdbt_md_getconfig(u8 id,
struct dmd_isdbt_initdata *psdmd_isdbt_initdata)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;

	MEMCPY(psdmd_isdbt_initdata, &(pres->sdmd_isdbt_initdata),
	sizeof(struct dmd_isdbt_initdata));

	return UTOPIA_STATUS_SUCCESS;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_get_fw_ver(u8 id, u16 *fw_ver)
#else
u8 mdrv_dmd_isdbt_md_get_fw_ver(u8 id, u16 *fw_ver)
#endif
{
	return TRUE;
}

void mdrv_dmd_isdbt_md_setreset(u8 id)
{
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_advsetconfig(u8 id,
enum dmd_isdbt_demod_type etype, u8 benable)
#else
u8 mdrv_dmd_isdbt_md_advsetconfig(u8 id,
enum dmd_isdbt_demod_type etype, u8 benable)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_SoftReset, NULL);

	if (benable) {
		switch (etype) {
		case DMD_ISDBT_DEMOD_6M:
		case DMD_ISDBT_DEMOD_6M_EWBS:
			if (etype != pres->sdmd_isdbt_pridata.elast_type) {
				if
					(etype != DMD_ISDBT_DEMOD_6M_EWBS) {
				pres->sdmd_isdbt_pridata.bdownloaded = FALSE;
				pres->sdmd_isdbt_pridata.elast_type =
				DMD_ISDBT_DEMOD_6M;
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_SetACICoef, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_Download, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data,
				DMD_ISDBT_HAL_CMD_TS_INTERFACE_CONFIG, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_DoIQSwap,
				&(pres->sdmd_isdbt_pridata.bis_qpad));
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_FWVERSION, NULL);
				} else
					pres->sdmd_isdbt_pridata.elast_type =
					DMD_ISDBT_DEMOD_6M_EWBS;
			}

			if (etype != DMD_ISDBT_DEMOD_6M_EWBS)
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_SetISDBTMode,
				NULL);
			else
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_SetEWBSMode, NULL);
			break;
		case DMD_ISDBT_DEMOD_7M:
		case DMD_ISDBT_DEMOD_7M_EWBS:
			if (etype != pres->sdmd_isdbt_pridata.elast_type) {
				if
					(etype != DMD_ISDBT_DEMOD_7M_EWBS) {
				pres->sdmd_isdbt_pridata.bdownloaded = FALSE;
				pres->sdmd_isdbt_pridata.elast_type =
				DMD_ISDBT_DEMOD_7M;
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_SetACICoef, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_Download, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data,
				DMD_ISDBT_HAL_CMD_TS_INTERFACE_CONFIG, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_DoIQSwap,
					&(pres->sdmd_isdbt_pridata.bis_qpad));
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_FWVERSION, NULL);
				} else
					pres->sdmd_isdbt_pridata.elast_type =
					DMD_ISDBT_DEMOD_7M_EWBS;
			}

			if (etype != DMD_ISDBT_DEMOD_7M_EWBS)
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
			(pio_data, DMD_ISDBT_HAL_CMD_SetISDBTMode, NULL);
			else
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
			(pio_data, DMD_ISDBT_HAL_CMD_SetEWBSMode, NULL);
			break;
		case DMD_ISDBT_DEMOD_8M:
		case DMD_ISDBT_DEMOD_8M_EWBS:
			if (etype != pres->sdmd_isdbt_pridata.elast_type) {
				if
				(etype != DMD_ISDBT_DEMOD_8M_EWBS) {
				pres->sdmd_isdbt_pridata.bdownloaded = FALSE;
				pres->sdmd_isdbt_pridata.elast_type =
				DMD_ISDBT_DEMOD_8M;
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
					(pio_data,
					DMD_ISDBT_HAL_CMD_SetACICoef, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
					(pio_data,
					DMD_ISDBT_HAL_CMD_Download, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
					(pio_data,
				DMD_ISDBT_HAL_CMD_TS_INTERFACE_CONFIG, NULL);
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
					(pio_data,
					DMD_ISDBT_HAL_CMD_DoIQSwap,
					&(pres->sdmd_isdbt_pridata.bis_qpad));
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
					(pio_data,
					DMD_ISDBT_HAL_CMD_FWVERSION, NULL);
				} else
					pres->sdmd_isdbt_pridata.elast_type =
					DMD_ISDBT_DEMOD_8M_EWBS;
			}

			if (etype != DMD_ISDBT_DEMOD_8M_EWBS)
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_SetISDBTMode,
				NULL);
			else
				pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
				(pio_data, DMD_ISDBT_HAL_CMD_SetEWBSMode, NULL);
			break;
		default:
			pres->sdmd_isdbt_pridata.elast_type =
			DMD_ISDBT_DEMOD_NULL;
			ret =
			pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd
			(pio_data, DMD_ISDBT_HAL_CMD_SetModeClean, NULL);
			break;
		}
	}

	#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN
	pres->sdmd_isdbt_info.isdbt_scan_time_start = MsOS_GetSystemTime();
	#else
	pres->sdmd_isdbt_info.isdbt_scan_time_start =
	pio_data->sys.get_system_time_ms();
	#endif
	pres->sdmd_isdbt_info.isdbt_lock_status = 0;
	DMD_UNLOCK();

	return ret;
}

#ifndef UTPA2
u8 mdrv_dmd_isdbt_md_setconfig(u8 id, u8 benable)
{
	return mdrv_dmd_isdbt_md_advsetconfig(id, DMD_ISDBT_DEMOD_6M,
	benable);
}
#endif

u8 mdrv_dmd_isdbt_md_setactive(u8 id, u8 benable)
{
	return TRUE;
}

#if DMD_ISDBT_STR_EN
#ifdef UTPA2
u32 _mdrv_dmd_isdbt_md_setpowerstate(u8 id,
enum EN_POWER_MODE power_state)
#else
u32 mdrv_dmd_isdbt_md_setpowerstate(u8 id,
enum EN_POWER_MODE power_state)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u32 ret = UTOPIA_STATUS_FAIL;

	if (pres->sdmd_isdbt_initdata.ewbs_mode == 2) {// For EWBS mode 3
		pres->sdmd_isdbt_pridata.elast_state = power_state;

		return UTOPIA_STATUS_SUCCESS;
	}

	if (power_state == E_POWER_SUSPEND) {
		if (!pres->sdmd_isdbt_initdata.ewbs_mode) // For EWBS mode 1
			pres->sdmd_isdbt_pridata.bdownloaded = FALSE;

		pres->sdmd_isdbt_pridata.bis_dtv      =
		pres->sdmd_isdbt_pridata.binit;
		pres->sdmd_isdbt_pridata.elast_state = power_state;

		if (pres->sdmd_isdbt_initdata.ewbs_mode) {// For EWBS mode 2
			#ifdef UTPA2
			if (pres->sdmd_isdbt_pridata.elast_type ==
				DMD_ISDBT_DEMOD_6M)
				_mdrv_dmd_isdbt_md_advsetconfig(id,
				DMD_ISDBT_DEMOD_6M_EWBS, TRUE);
			else if (pres->sdmd_isdbt_pridata.elast_type ==
				DMD_ISDBT_DEMOD_7M)
				_mdrv_dmd_isdbt_md_advsetconfig(id,
				DMD_ISDBT_DEMOD_7M_EWBS, TRUE);
			else if (pres->sdmd_isdbt_pridata.elast_type ==
				DMD_ISDBT_DEMOD_8M)
				_mdrv_dmd_isdbt_md_advsetconfig(id,
				DMD_ISDBT_DEMOD_8M_EWBS, TRUE);
			else
				pres->sdmd_isdbt_pridata.bis_dtv |= 0x10;
			#else
			if (pres->sdmd_isdbt_pridata.elast_type ==
				DMD_ISDBT_DEMOD_6M)
				mdrv_dmd_isdbt_md_advsetconfig(id,
				DMD_ISDBT_DEMOD_6M_EWBS, TRUE);
			else if (pres->sdmd_isdbt_pridata.elast_type ==
				DMD_ISDBT_DEMOD_7M)
				mdrv_dmd_isdbt_md_advsetconfig(id,
				DMD_ISDBT_DEMOD_7M_EWBS, TRUE);
			else if (pres->sdmd_isdbt_pridata.elast_type ==
				DMD_ISDBT_DEMOD_8M)
				mdrv_dmd_isdbt_md_advsetconfig(id,
				DMD_ISDBT_DEMOD_8M_EWBS, TRUE);
			else
				pres->sdmd_isdbt_pridata.bis_dtv |= 0x10;
			#endif
		} else if (pres->sdmd_isdbt_pridata.binit) {
			#ifdef UTPA2
			_mdrv_dmd_isdbt_md_exit(id);
			#else
			mdrv_dmd_isdbt_md_exit(id);
			#endif
		}

		ret = UTOPIA_STATUS_SUCCESS;
	} else if (power_state == E_POWER_RESUME) {
		if (pres->sdmd_isdbt_pridata.elast_state == E_POWER_SUSPEND) {
			PRINT("\nVT: (Check ISDBT Mode In DRV:) DTV Mode=%d\n",
			pres->sdmd_isdbt_pridata.bis_dtv);

			if (pres->sdmd_isdbt_pridata.bis_dtv == TRUE) {
				if (pres->sdmd_isdbt_initdata.ewbs_mode) {
					#ifdef UTPA2
					if
					(pres->sdmd_isdbt_pridata.elast_type ==
						DMD_ISDBT_DEMOD_6M_EWBS)
						_mdrv_dmd_isdbt_md_advsetconfig
						(id, DMD_ISDBT_DEMOD_6M, TRUE);
					else if
					(pres->sdmd_isdbt_pridata.elast_type ==
						DMD_ISDBT_DEMOD_7M_EWBS)
						_mdrv_dmd_isdbt_md_advsetconfig
						(id, DMD_ISDBT_DEMOD_7M, TRUE);
					else if
					(pres->sdmd_isdbt_pridata.elast_type ==
						DMD_ISDBT_DEMOD_8M_EWBS)
						_mdrv_dmd_isdbt_md_advsetconfig
						(id, DMD_ISDBT_DEMOD_8M, TRUE);
					#else
					if
					(pres->sdmd_isdbt_pridata.elast_type ==
						DMD_ISDBT_DEMOD_6M_EWBS)
						mdrv_dmd_isdbt_md_advsetconfig
						(id, DMD_ISDBT_DEMOD_6M, TRUE);
					else if
					(pres->sdmd_isdbt_pridata.elast_type ==
						DMD_ISDBT_DEMOD_7M_EWBS)
						mdrv_dmd_isdbt_md_advsetconfig
						(id, DMD_ISDBT_DEMOD_7M, TRUE);
					else if
					(pres->sdmd_isdbt_pridata.elast_type ==
						DMD_ISDBT_DEMOD_8M_EWBS)
						mdrv_dmd_isdbt_md_advsetconfig
						(id, DMD_ISDBT_DEMOD_8M, TRUE);
					#endif
				} else {
					#ifdef UTPA2
					_mdrv_dmd_isdbt_md_init(id,
					&(pres->sdmd_isdbt_initdata),
					sizeof(struct dmd_isdbt_initdata));
					_mdrv_dmd_isdbt_md_do_iq_swap(id,
					pres->sdmd_isdbt_pridata.bis_qpad);
					_mdrv_dmd_isdbt_md_advsetconfig(id,
					pres->sdmd_isdbt_pridata.elast_type,
					TRUE);
					#else
					mdrv_dmd_isdbt_md_init(id,
					&(pres->sdmd_isdbt_initdata),
					sizeof(struct dmd_isdbt_initdata));
					mdrv_dmd_isdbt_md_do_iq_swap(id,
					pres->sdmd_isdbt_pridata.bis_qpad);
					mdrv_dmd_isdbt_md_advsetconfig(id,
					pres->sdmd_isdbt_pridata.elast_type,
					TRUE);
					#endif
				}
			} else if (pres->sdmd_isdbt_initdata.ewbs_mode &&
				!(pres->sdmd_isdbt_pridata.bis_dtv)) {
				#ifdef UTPA2
				_mdrv_dmd_isdbt_md_exit(id);
				#else
				mdrv_dmd_isdbt_md_exit(id);
				#endif
			}

			pres->sdmd_isdbt_pridata.elast_state = power_state;

			ret = UTOPIA_STATUS_SUCCESS;
		} else {
			PRINT(
			"[%s,%5d]It is not suspended yet. We shouldn't resume\n",
			__func__, __LINE__);

			ret = UTOPIA_STATUS_FAIL;
		}
	} else {
		PRINT("[%s,%5d]Do Nothing: %d\n", __func__,
		__LINE__, power_state);

		ret = UTOPIA_STATUS_FAIL;
	}

	return ret;
}
#endif

#ifdef UTPA2
enum dmd_isdbt_lock_status _mdrv_dmd_isdbt_md_getlock(u8 id,
enum dmd_isdbt_getlock_type etype)
#else
enum dmd_isdbt_lock_status mdrv_dmd_isdbt_md_getlock(u8 id,
enum dmd_isdbt_getlock_type etype)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	enum dmd_isdbt_lock_status status  = DMD_ISDBT_UNLOCK;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	switch (etype) {
	case DMD_ISDBT_GETLOCK:
		status = _isdbt_checklock();
		break;
	case DMD_ISDBT_GETLOCK_FSA_TRACK_LOCK:
		status =
		(pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
		DMD_ISDBT_HAL_CMD_Check_FSA_TRACK_Lock, NULL)) ?
		DMD_ISDBT_LOCK : DMD_ISDBT_UNLOCK;
		break;
	case DMD_ISDBT_GETLOCK_PSYNC_LOCK:
		status =
		(pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
		DMD_ISDBT_HAL_CMD_Check_PSYNC_Lock, NULL)) ?
		DMD_ISDBT_LOCK : DMD_ISDBT_UNLOCK;
		break;
	case DMD_ISDBT_GETLOCK_ICFO_CH_EXIST_LOCK:
		status =
		(pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
		DMD_ISDBT_HAL_CMD_Check_ICFO_CH_EXIST_Lock, NULL)) ?
		DMD_ISDBT_LOCK : DMD_ISDBT_UNLOCK;
		break;
	case DMD_ISDBT_GETLOCK_FEC_LOCK:
		status =
		(pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
		DMD_ISDBT_HAL_CMD_Check_FEC_Lock, NULL)) ?
		DMD_ISDBT_LOCK : DMD_ISDBT_UNLOCK;
		break;
	case DMD_ISDBT_GETLOCK_EWBS_LOCK:
		status =
		(pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
		DMD_ISDBT_HAL_CMD_Check_EWBS_Lock, NULL)) ?
		DMD_ISDBT_LOCK : DMD_ISDBT_UNLOCK;
		break;
	default:
		status = DMD_ISDBT_UNLOCK;
		break;
	}
	DMD_UNLOCK();

	return status;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_get_modulation_mode(u8 id,
enum en_isdbt_layer elayer_index,
struct isdbt_modulation_mode *sisdbt_modulation_mode)
#else
u8 mdrv_dmd_isdbt_md_get_modulation_mode(u8 id,
enum en_isdbt_layer elayer_index,
struct isdbt_modulation_mode *sisdbt_modulation_mode)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	struct dmd_isdbt_get_modulation       sisdbt_get_modulation_mode;
	struct dmd_isdbt_get_time_interleaving sisdbt_get_time_interleaving;
	struct dmd_isdbt_get_code_rate         sisdbt_get_code_rate;

	sisdbt_get_modulation_mode.eisdbtlayer   = elayer_index;
	sisdbt_get_time_interleaving.eisdbtlayer = elayer_index;
	sisdbt_get_code_rate.eisdbtlayer         = elayer_index;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret &= pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalCodeRate, &sisdbt_get_code_rate);
	sisdbt_modulation_mode->eisdbtcoderate = sisdbt_get_code_rate.ecoderate;
	ret &= pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalGuardInterval,
	&(sisdbt_modulation_mode->eisdbtgi));
	ret &= pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalTimeInterleaving,
	&sisdbt_get_time_interleaving);
	sisdbt_modulation_mode->eisdbttdi =
	sisdbt_get_time_interleaving.etimeinterleaving;
	ret &= pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalFFTValue,
	&(sisdbt_modulation_mode->eisdbtfft));
	ret &= pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalModulation,
	&sisdbt_get_modulation_mode);
	sisdbt_modulation_mode->eisdbtconstellation =
	sisdbt_get_modulation_mode.econstellation;
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_get_signalstrength(u8 id, u16 *strength)
#else
u8 mdrv_dmd_isdbt_md_get_signalstrength(u8 id, u16 *strength)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_ReadIFAGC, strength);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_getfreqoffset(u8 id,
struct dmd_isdbt_cfo_data *cfo)
#else
u8 mdrv_dmd_isdbt_md_getfreqoffset(u8 id, float *cfo)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetFreqOffset, cfo);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u16 _mdrv_dmd_isdbt_md_get_signalquality(u8 id)
#else
u16 mdrv_dmd_isdbt_md_get_signalquality(u8 id)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u16 signal_quality = 0;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalQuality, &signal_quality);
	DMD_UNLOCK();

	return signal_quality;
}

#ifdef UTPA2
u16 _mdrv_dmd_isdbt_md_get_signalqualityoflayerA(u8 id)
#else
u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerA(u8 id)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u16 signal_quality = 0;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerA, &signal_quality);
	DMD_UNLOCK();

	return signal_quality;
}

#ifdef UTPA2
u16 _mdrv_dmd_isdbt_md_get_signalqualityoflayerB(u8 id)
#else
u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerB(u8 id)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u16 signal_quality = 0;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerB, &signal_quality);
	DMD_UNLOCK();

	return signal_quality;
}

#ifdef UTPA2
u16 _mdrv_dmd_isdbt_md_get_signalqualityoflayerC(u8 id)
#else
u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerC(u8 id)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u16 SignalQuality = 0;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalQualityOfLayerC, &SignalQuality);
	DMD_UNLOCK();

	return SignalQuality;
}

#ifdef UTPA2
u16 _mdrv_dmd_isdbt_md_get_signalqualitycombine(u8 id)
#else
u16 mdrv_dmd_isdbt_md_get_signalqualitycombine(u8 id)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u16 signal_quality = 0;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSignalQualityCombine, &signal_quality);
	DMD_UNLOCK();

	return signal_quality;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_getsnr(u8 id, struct dmd_isdbt_snr_data *f_snr)
#else
u8 mdrv_dmd_isdbt_md_getsnr(u8 id, float *f_snr)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetSNR, f_snr);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_get_previterbi_ber(u8 id,
struct dmd_isdbt_get_ber_value *ber)
#else
u8 mdrv_dmd_isdbt_md_get_previterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, float *fber)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	#ifndef UTPA2
	struct dmd_isdbt_get_ber_value sisdbt_get_ber_value;

	sisdbt_get_ber_value.eisdbtlayer = elayer_index;
	#endif

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	#ifdef UTPA2
	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetPreViterbiBer, ber);
	#else
	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetPreViterbiBer, &sisdbt_get_ber_value);
	#endif
	DMD_UNLOCK();

	#ifndef UTPA2
	*fber = sisdbt_get_ber_value.fbervalue;
	#endif

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_get_postviterbi_ber(u8 id,
struct dmd_isdbt_get_ber_value *ber)
#else
u8 mdrv_dmd_isdbt_md_get_postviterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, float *fber)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	#ifndef UTPA2
	struct dmd_isdbt_get_ber_value sisdbt_get_ber_value;

	sisdbt_get_ber_value.eisdbtlayer = elayer_index;
	#endif

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	#ifdef UTPA2
	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetPostViterbiBer, ber);
	#else
	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GetPostViterbiBer, &sisdbt_get_ber_value);
	#endif
	DMD_UNLOCK();

	#ifndef UTPA2
	*fber = sisdbt_get_ber_value.fbervalue;
	#endif

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_read_pkt_err(u8 id,
enum en_isdbt_layer elayer_index, u16 *packet_err)
#else
u8 mdrv_dmd_isdbt_md_read_pkt_err(u8 id,
enum en_isdbt_layer elayer_index, u16 *packet_err)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	struct dmd_isdbt_get_pkt_err sisdbt_get_pkt_err;

	sisdbt_get_pkt_err.eisdbtlayer = elayer_index;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_Read_PKT_ERR, &sisdbt_get_pkt_err);
	DMD_UNLOCK();

	*packet_err = sisdbt_get_pkt_err.packeterr;

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_set_serialcontrol(u8 id, u8 ts_config_data)
#else
u8 mdrv_dmd_isdbt_md_set_serialcontrol(u8 id, u8 ts_config_data)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	pres->sdmd_isdbt_initdata.ts_config_byte_serial_mode =
	ts_config_data;
	pres->sdmd_isdbt_initdata.ts_config_byte_data_swap   =
	ts_config_data >> 1;
	pres->sdmd_isdbt_initdata.ts_config_byte_clock_inv   =
	ts_config_data >> 2;
	pres->sdmd_isdbt_initdata.ts_config_byte_div_num     =
	ts_config_data >> 3;

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_TS_INTERFACE_CONFIG, NULL);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_iic_bypass_mode(u8 id, u8 benable)
#else
u8 mdrv_dmd_isdbt_md_iic_bypass_mode(u8 id, u8 benable)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_IIC_Bypass_Mode, &benable);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_switch_sspi_gpio(u8 id, u8 benable)
#else
u8 mdrv_dmd_isdbt_md_switch_sspi_gpio(u8 id, u8 benable)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_SSPI_TO_GPIO, &benable);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_gpio_get_level(u8 id,
u8 pin, u8 *blevel)
#else
u8 mdrv_dmd_isdbt_md_gpio_get_level(u8 id,
u8 pin, u8 *blevel)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	struct dmd_isdbt_gpio_pin_data spin;
	u8 ret = TRUE;

	spin.pin = pin;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GPIO_GET_LEVEL, &spin);
	DMD_UNLOCK();

	*blevel = spin.level;

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_gpio_set_level(u8 id,
u8 pin, u8 blevel)
#else
u8 mdrv_dmd_isdbt_md_gpio_set_level(u8 id,
u8 pin, u8 blevel)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	struct dmd_isdbt_gpio_pin_data spin;
	u8 ret = TRUE;

	spin.pin = pin;
	spin.level = blevel;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GPIO_SET_LEVEL, &spin);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_gpio_out_enable(u8 id,
u8 pin, u8 enable_out)
#else
u8 mdrv_dmd_isdbt_md_gpio_out_enable(u8 id,
u8 pin, u8 enable_out)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	struct dmd_isdbt_gpio_pin_data spin;
	u8 ret = TRUE;

	spin.pin = pin;
	spin.is_out = enable_out;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GPIO_OUT_ENABLE, &spin);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_do_iq_swap(u8 id, u8 bis_qpad)
#else
u8 mdrv_dmd_isdbt_md_do_iq_swap(u8 id, u8 bis_qpad)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	u8 ret = TRUE;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_DoIQSwap, &bis_qpad);
	DMD_UNLOCK();

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_getreg(u8 id, u16 addr, u8 *pdata)
#else
u8 mdrv_dmd_isdbt_md_getreg(u8 id, u16 addr, u8 *pdata)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	struct dmd_isdbt_reg_data reg;
	u8 ret = TRUE;

	reg.isdbt_addr = addr;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_GET_REG, &reg);
	DMD_UNLOCK();

	*pdata = reg.isdbt_data;

	return ret;
}

#ifdef UTPA2
u8 _mdrv_dmd_isdbt_md_setreg(u8 id, u16 addr, u8 data)
#else
u8 mdrv_dmd_isdbt_md_setreg(u8 id, u16 addr, u8 data)
#endif
{
	struct dmd_isdbt_resdata *pres = psdmd_isdbt_resdata + id;
	struct dmd_isdbt_reg_data reg;
	u8 ret = TRUE;

	reg.isdbt_addr = addr;
	reg.isdbt_data = data;

	if (!(pres->sdmd_isdbt_pridata.binit))
		return FALSE;
	DMD_LOCK();

	_update_io_data(id, pres);

	ret = pres->sdmd_isdbt_pridata.hal_dmd_isdbt_ioctl_cmd(pio_data,
	DMD_ISDBT_HAL_CMD_SET_REG, &reg);
	DMD_UNLOCK();

	return ret;
}

// Service mode
const u32 dataRate1Seg6MHzBW[4][5][4] = {
	// 1/4    1/8    1/16     1/32
	{
		// DQPSK
		{ 28085,  31206,  33042,  34043}, /* 1/2 */
		{ 37447,  41608,  44056,  45391}, /* 2/3 */
		{ 42128,  46809,  49563,  51065}, /* 3/4 */
		{ 46809,  52010,  55070,  56739}, /* 5/6 */
		{ 49150,  54611,  57823,  59576}  /* 7/8 */
	},
	{
		// QPSK
		{ 28085,  31206,  33042,  34043}, /* 1/2 */
		{ 37447,  41608,  44056,  45391}, /* 2/3 */
		{ 42128,  46809,  49563,  51065}, /* 3/4 */
		{ 46809,  52010,  55070,  56739}, /* 5/6 */
		{ 49150,  54611,  57823,  59576}  /* 7/8 */
	},
	{
		// 16QAM
		{ 56171,  62413,  66084,  68087}, /* 1/2 */
		{ 74895,  83217,  88112,  90782}, /* 2/3 */
		{ 84257,  93619,  99126, 102130}, /* 3/4 */
		{ 93619, 104021, 110140, 113478}, /* 5/6 */
		{ 98300, 109222, 115647, 119152}  /* 7/8 */
	},
	{
		// 64QAM
		{ 84257,  93619,  99126, 102130}, /* 1/2 */
		{112343, 124826, 132168, 136174}, /* 2/3 */
		{126386, 140429, 148690, 153195}, /* 3/4 */
		{140429, 156032, 165211, 170217}, /* 5/6 */
		{147450, 163834, 173471, 178728}  /* 7/8 */
	}
};

u8 _mdrv_dmd_isdbt_md_get_information(u8 id,
struct dvb_frontend *fe, struct dtv_frontend_properties *p)
{
	u8 ifagc_val = 0;
	u8 u8temp = 0;
	u16 packet_err = 0;
	u32 rf_freq_hz = 0;
	s32 freq_offset_khz = 0;
	s32 s32temp = 0;
	u8 ret = TRUE;
	struct dmd_isdbt_cfo_data isdbt_cfo_data;
	struct dmd_isdbt_snr_data isdbt_snr;
	struct dtv_fe_stats *isdbt_stats;
	struct dtv_fe_stats *isdbt_stats2;
	struct isdbt_modulation_mode sisdbt_modulation_mode;
	struct dmd_isdbt_get_ber_value sisdbt_get_ber_value;

	p->delivery_system = SYS_ISDBT;

	// Get real RF freq /offset
	rf_freq_hz = fe->dtv_property_cache.frequency;
	ret = _mdrv_dmd_isdbt_md_getfreqoffset(0, &isdbt_cfo_data);

	if ((isdbt_cfo_data.fft_mode & 0x30) == 0x0000) // 2k
		freq_offset_khz = (u32)isdbt_cfo_data.icfo_regvalue*250/63;
	else if ((isdbt_cfo_data.fft_mode & 0x30) == 0x0010) // 4k
		freq_offset_khz = (u32)isdbt_cfo_data.icfo_regvalue*125/63;
	else //if ((isdbt_cfo_data.fft_mode & 0x30) == 0x0020) // 8k
		freq_offset_khz = (u32)isdbt_cfo_data.icfo_regvalue*125/126;

	rf_freq_hz -= freq_offset_khz*1000;
	p->frequency = rf_freq_hz;
	p->frequency_offset = freq_offset_khz*1000;

	// Get IFAGC
	ret = _mdrv_dmd_isdbt_md_setreg(0, 0x2822, 0x03);
	ret = _mdrv_dmd_isdbt_md_getreg(0, 0x2825, &ifagc_val);

	ifagc_val ^= 0xFF;
	p->agc_val = ifagc_val;

	// Get SNR
	if (_mdrv_dmd_isdbt_md_getlock(0, DMD_ISDBT_GETLOCK)
	== DMD_ISDBT_LOCK) {
		s32temp = 0;
		for (u8temp = 0; u8temp < 16; ++u8temp) {
			_mdrv_dmd_isdbt_md_getsnr(0, &isdbt_snr);
			s32temp += (s32)(isdbt_snr.reg_snr);
			_delayms(1);
		}
		s32temp /= 10;
		s32temp >>= 4;
	} else
		s32temp = 0;

	isdbt_stats = &p->cnr;
	isdbt_stats->len = 1;
	isdbt_stats->stat[0].svalue = s32temp;
	isdbt_stats->stat[0].scale = FE_SCALE_DECIBEL;

	// UEC : Get Packet Error
	ret = _mdrv_dmd_isdbt_md_read_pkt_err(0, E_ISDBT_Layer_A,
	&packet_err);

	isdbt_stats2 = &p->block_error;
	isdbt_stats2->len = 1;
	isdbt_stats2->stat[0].uvalue = packet_err;
	isdbt_stats2->stat[0].scale = FE_SCALE_COUNTER;

	ret = _mdrv_dmd_isdbt_md_get_modulation_mode(0, E_ISDBT_Layer_A,
	&sisdbt_modulation_mode);

	switch (sisdbt_modulation_mode.eisdbtfft) {
	case E_ISDBT_FFT_2K:
		p->transmission_mode = TRANSMISSION_MODE_2K;
		break;
	case E_ISDBT_FFT_4K:
		p->transmission_mode = TRANSMISSION_MODE_4K;
		break;
	case E_ISDBT_FFT_8K:
		p->transmission_mode = TRANSMISSION_MODE_8K;
		break;
	case E_ISDBT_FFT_INVALID:
	default:
		p->transmission_mode = TRANSMISSION_MODE_AUTO;
		break;
	}

	switch (sisdbt_modulation_mode.eisdbtgi) {
	case E_ISDBT_GUARD_INTERVAL_1_32:
		p->guard_interval = GUARD_INTERVAL_1_32;
		break;
	case E_ISDBT_GUARD_INTERVAL_1_16:
		p->guard_interval = GUARD_INTERVAL_1_16;
		break;
	case E_ISDBT_GUARD_INTERVAL_1_8:
		p->guard_interval = GUARD_INTERVAL_1_8;
		break;
	case E_ISDBT_GUARD_INTERVAL_1_4:
		p->guard_interval = GUARD_INTERVAL_1_4;
		break;
	case E_ISDBT_GUARD_INTERVAL_INVALID:
	default:
		p->guard_interval = GUARD_INTERVAL_AUTO;
		break;
	}

	// layerA information
	switch (sisdbt_modulation_mode.eisdbtconstellation) {
	case E_ISDBT_DQPSK:
		p->layer[0].modulation = DQPSK;
		break;
	case E_ISDBT_QPSK:
		p->layer[0].modulation = QPSK;
		break;
	case E_ISDBT_16QAM:
		p->layer[0].modulation = QAM_16;
		break;
	case E_ISDBT_64QAM:
		p->layer[0].modulation = QAM_64;
		break;
	case E_ISDBT_QAM_INVALID:
	default:
		p->layer[0].modulation = QAM_AUTO;
		break;
	}

	switch (sisdbt_modulation_mode.eisdbtcoderate) {
	case E_ISDBT_CODERATE_1_2:
		p->layer[0].fec = FEC_1_2;
		break;
	case E_ISDBT_CODERATE_2_3:
		p->layer[0].fec = FEC_2_3;
		break;
	case E_ISDBT_CODERATE_3_4:
		p->layer[0].fec = FEC_3_4;
		break;
	case E_ISDBT_CODERATE_5_6:
		p->layer[0].fec = FEC_5_6;
		break;
	case E_ISDBT_CODERATE_7_8:
		p->layer[0].fec = FEC_7_8;
		break;
	case E_ISDBT_CODERATE_INVALID:
	default:
		p->layer[0].fec = FEC_NONE;
		break;
	}

	switch (sisdbt_modulation_mode.eisdbttdi) {
	case E_ISDBT_2K_TDI_0:
	case E_ISDBT_4K_TDI_0:
	case E_ISDBT_8K_TDI_0:
		p->layer[0].interleaving = 0;
		break;
	case E_ISDBT_8K_TDI_1:
		p->layer[0].interleaving = 1;
		break;
	case E_ISDBT_4K_TDI_2:
	case E_ISDBT_8K_TDI_2:
		p->layer[0].interleaving = 2;
		break;
	case E_ISDBT_2K_TDI_4:
	case E_ISDBT_4K_TDI_4:
	case E_ISDBT_8K_TDI_4:
		p->layer[0].interleaving = 4;
		break;
	case E_ISDBT_2K_TDI_8:
	case E_ISDBT_4K_TDI_8:
		p->layer[0].interleaving = 8;
		break;
	case E_ISDBT_2K_TDI_16:
		p->layer[0].interleaving = 16;
		break;
	case E_ISDBT_TDI_INVALID:
	default:
		p->layer[0].interleaving = 0;
		break;
	}

	ret = _mdrv_dmd_isdbt_md_getreg(0, 0x1508, &u8temp);
	p->isdbt_partial_reception = ((u8temp&0x02) == 0x02) ? 1:0;

	ret = _mdrv_dmd_isdbt_md_getreg(0, 0x150A, &u8temp);
	u8temp &= 0x0F;
	if ((u8temp != 0x0F) && (u8temp != 0x00))
		p->layer[0].segment_count = u8temp;
	else
		p->layer[0].segment_count = 0;

	sisdbt_get_ber_value.eisdbtlayer = E_ISDBT_Layer_A;
	ret = _mdrv_dmd_isdbt_md_get_postviterbi_ber(0, &sisdbt_get_ber_value);
	p->layer[0].post_ber =
(sisdbt_get_ber_value.bervalue*5194) / (sisdbt_get_ber_value.berperiod*100);

	// Temp : LayerA_BER
	p->post_ber = p->layer[0].post_ber;

	// layerB information
	ret = _mdrv_dmd_isdbt_md_get_modulation_mode(0, E_ISDBT_Layer_B,
	&sisdbt_modulation_mode);

	switch (sisdbt_modulation_mode.eisdbtconstellation) {
	case E_ISDBT_DQPSK:
		p->layer[1].modulation = DQPSK;
		break;
	case E_ISDBT_QPSK:
		p->layer[1].modulation = QPSK;
		break;
	case E_ISDBT_16QAM:
		p->layer[1].modulation = QAM_16;
		break;
	case E_ISDBT_64QAM:
		p->layer[1].modulation = QAM_64;
		break;
	case E_ISDBT_QAM_INVALID:
	default:
		p->layer[1].modulation = QAM_AUTO;
		break;
	}

	switch (sisdbt_modulation_mode.eisdbtcoderate) {
	case E_ISDBT_CODERATE_1_2:
		p->layer[1].fec = FEC_1_2;
		break;
	case E_ISDBT_CODERATE_2_3:
		p->layer[1].fec = FEC_2_3;
		break;
	case E_ISDBT_CODERATE_3_4:
		p->layer[1].fec = FEC_3_4;
		break;
	case E_ISDBT_CODERATE_5_6:
		p->layer[1].fec = FEC_5_6;
		break;
	case E_ISDBT_CODERATE_7_8:
		p->layer[1].fec = FEC_7_8;
		break;
	case E_ISDBT_CODERATE_INVALID:
	default:
		p->layer[1].fec = FEC_NONE;
		break;
	}

	switch (sisdbt_modulation_mode.eisdbttdi) {
	case E_ISDBT_2K_TDI_0:
	case E_ISDBT_4K_TDI_0:
	case E_ISDBT_8K_TDI_0:
		p->layer[1].interleaving = 0;
		break;
	case E_ISDBT_8K_TDI_1:
		p->layer[1].interleaving = 1;
		break;
	case E_ISDBT_4K_TDI_2:
	case E_ISDBT_8K_TDI_2:
		p->layer[1].interleaving = 2;
		break;
	case E_ISDBT_2K_TDI_4:
	case E_ISDBT_4K_TDI_4:
	case E_ISDBT_8K_TDI_4:
		p->layer[1].interleaving = 4;
		break;
	case E_ISDBT_2K_TDI_8:
	case E_ISDBT_4K_TDI_8:
		p->layer[1].interleaving = 8;
		break;
	case E_ISDBT_2K_TDI_16:
		p->layer[1].interleaving = 16;
		break;
	case E_ISDBT_TDI_INVALID:
	default:
		p->layer[1].interleaving = 0;
		break;
	}

	ret = _mdrv_dmd_isdbt_md_getreg(0, 0x150C, &u8temp);
	u8temp &= 0x0F;
	if ((u8temp != 0x0F) && (u8temp != 0x00))
		p->layer[1].segment_count = u8temp;
	else
		p->layer[1].segment_count = 0;

	sisdbt_get_ber_value.eisdbtlayer = E_ISDBT_Layer_B;
	ret = _mdrv_dmd_isdbt_md_get_postviterbi_ber(0, &sisdbt_get_ber_value);
	p->layer[1].post_ber =
(sisdbt_get_ber_value.bervalue*5194) / (sisdbt_get_ber_value.berperiod*100);

	// layerC information
	ret = _mdrv_dmd_isdbt_md_get_modulation_mode(0, E_ISDBT_Layer_C,
	&sisdbt_modulation_mode);

	switch (sisdbt_modulation_mode.eisdbtconstellation) {
	case E_ISDBT_DQPSK:
		p->layer[2].modulation = DQPSK;
		break;
	case E_ISDBT_QPSK:
		p->layer[2].modulation = QPSK;
		break;
	case E_ISDBT_16QAM:
		p->layer[2].modulation = QAM_16;
		break;
	case E_ISDBT_64QAM:
		p->layer[2].modulation = QAM_64;
		break;
	case E_ISDBT_QAM_INVALID:
	default:
		p->layer[2].modulation = QAM_AUTO;
		break;
	}

	switch (sisdbt_modulation_mode.eisdbtcoderate) {
	case E_ISDBT_CODERATE_1_2:
		p->layer[2].fec = FEC_1_2;
		break;
	case E_ISDBT_CODERATE_2_3:
		p->layer[2].fec = FEC_2_3;
		break;
	case E_ISDBT_CODERATE_3_4:
		p->layer[2].fec = FEC_3_4;
		break;
	case E_ISDBT_CODERATE_5_6:
		p->layer[2].fec = FEC_5_6;
		break;
	case E_ISDBT_CODERATE_7_8:
		p->layer[2].fec = FEC_7_8;
		break;
	case E_ISDBT_CODERATE_INVALID:
	default:
		p->layer[2].fec = FEC_NONE;
		break;
	}

	switch (sisdbt_modulation_mode.eisdbttdi) {
	case E_ISDBT_2K_TDI_0:
	case E_ISDBT_4K_TDI_0:
	case E_ISDBT_8K_TDI_0:
		p->layer[2].interleaving = 0;
		break;
	case E_ISDBT_8K_TDI_1:
		p->layer[2].interleaving = 1;
		break;
	case E_ISDBT_4K_TDI_2:
	case E_ISDBT_8K_TDI_2:
		p->layer[2].interleaving = 2;
		break;
	case E_ISDBT_2K_TDI_4:
	case E_ISDBT_4K_TDI_4:
	case E_ISDBT_8K_TDI_4:
		p->layer[2].interleaving = 4;
		break;
	case E_ISDBT_2K_TDI_8:
	case E_ISDBT_4K_TDI_8:
		p->layer[2].interleaving = 8;
		break;
	case E_ISDBT_2K_TDI_16:
		p->layer[2].interleaving = 16;
		break;
	case E_ISDBT_TDI_INVALID:
	default:
		p->layer[2].interleaving = 0;
		break;
	}

	ret = _mdrv_dmd_isdbt_md_getreg(0, 0x150E, &u8temp);
	u8temp &= 0x0F;
	if ((u8temp != 0x0F) && (u8temp != 0x00))
		p->layer[2].segment_count = u8temp;
	else
		p->layer[2].segment_count = 0;

	sisdbt_get_ber_value.eisdbtlayer = E_ISDBT_Layer_C;
	ret = _mdrv_dmd_isdbt_md_get_postviterbi_ber(0, &sisdbt_get_ber_value);
	p->layer[2].post_ber =
(sisdbt_get_ber_value.bervalue*5194) / (sisdbt_get_ber_value.berperiod*100);

	if (_mdrv_dmd_isdbt_md_getlock(0,
	DMD_ISDBT_GETLOCK_EWBS_LOCK) == DMD_ISDBT_LOCK)
		p->isdbt_ewbs_flag = 1;
	else
		p->isdbt_ewbs_flag = 0;

	return ret;
}

u8 _mdrv_dmd_isdbt_md_get_ts_rate(u8 id,
enum en_isdbt_layer elayer_index, u32 *pdata)
{
	u8 para1;
	u8 para2;
	u8 para3;
	u8 u8temp;
	u32 data_rate_khz;
	u8 ret = TRUE;
	struct isdbt_modulation_mode sisdbt_modulation_mode;

	ret = _mdrv_dmd_isdbt_md_get_modulation_mode(0, elayer_index,
	&sisdbt_modulation_mode);

	para1 = sisdbt_modulation_mode.eisdbtconstellation;
	para2 = sisdbt_modulation_mode.eisdbtcoderate;
	para3 = sisdbt_modulation_mode.eisdbtgi;

	if (elayer_index == E_ISDBT_Layer_A)
		ret = _mdrv_dmd_isdbt_md_getreg(0, 0x150A, &u8temp);
	else if (elayer_index == E_ISDBT_Layer_B)
		ret = _mdrv_dmd_isdbt_md_getreg(0, 0x150C, &u8temp);
	else if (elayer_index == E_ISDBT_Layer_C)
		ret = _mdrv_dmd_isdbt_md_getreg(0, 0x150E, &u8temp);
	else
		ret = _mdrv_dmd_isdbt_md_getreg(0, 0x150A, &u8temp);

	u8temp &= 0x0F;
	if ((u8temp != 0x0F) && (u8temp != 0x00)) {
		data_rate_khz =
			(u32)u8temp*dataRate1Seg6MHzBW[para1][para2][para3];
		data_rate_khz /= 100;
	} else
		data_rate_khz = 0;

	*pdata = data_rate_khz;

	return ret;
}

u8 _mdrv_dmd_isdbt_md_get_chip_revision(u8 id, u8 *pdata)
{
	u8 ret = TRUE;

	*pdata = 0x99;

	return ret;
}

u8 _mdrv_dmd_isdbt_md_get_layer_num(u8 id, u8 *pdata)
{
	u8 ret = TRUE;
	u8 layer_num = 0;
	struct isdbt_modulation_mode sisdbt_modulation_mode;

	ret = _mdrv_dmd_isdbt_md_get_modulation_mode(0, E_ISDBT_Layer_A,
		&sisdbt_modulation_mode);
	if (sisdbt_modulation_mode.eisdbtconstellation != E_ISDBT_QAM_INVALID)
		layer_num++;
	ret = _mdrv_dmd_isdbt_md_get_modulation_mode(0, E_ISDBT_Layer_B,
		&sisdbt_modulation_mode);
	if (sisdbt_modulation_mode.eisdbtconstellation != E_ISDBT_QAM_INVALID)
		layer_num++;
	ret = _mdrv_dmd_isdbt_md_get_modulation_mode(0, E_ISDBT_Layer_C,
		&sisdbt_modulation_mode);
	if (sisdbt_modulation_mode.eisdbtconstellation != E_ISDBT_QAM_INVALID)
		layer_num++;

	*pdata = layer_num;

	return ret;
}

ssize_t isdbt_get_information_show(struct device_driver *driver, char *buf)
{
	u8 u8temp;
	u16 u16temp;
	u32 u32temp;
	int err_flg[3];
	int ts_rate[3];
	int chip_revision;
	int layer_num;

	//printk(KERN_INFO "isdbt_get_information_show is called\n");

	// Get Err_Flg_layerA/B/C
	_mdrv_dmd_isdbt_md_read_pkt_err(0, E_ISDBT_Layer_A, &u16temp);
	err_flg[0] = (u16temp == 0) ? 0:1;

	_mdrv_dmd_isdbt_md_read_pkt_err(0, E_ISDBT_Layer_B, &u16temp);
	err_flg[1] = (u16temp == 0) ? 0:1;

	_mdrv_dmd_isdbt_md_read_pkt_err(0, E_ISDBT_Layer_C, &u16temp);
	err_flg[2] = (u16temp == 0) ? 0:1;

	// Get TS_Rate_layerA/B/C
	_mdrv_dmd_isdbt_md_get_ts_rate(0, E_ISDBT_Layer_A, &u32temp);
	ts_rate[0] = (int)u32temp;

	_mdrv_dmd_isdbt_md_get_ts_rate(0, E_ISDBT_Layer_B, &u32temp);
	ts_rate[1] = (int)u32temp;

	_mdrv_dmd_isdbt_md_get_ts_rate(0, E_ISDBT_Layer_C, &u32temp);
	ts_rate[2] = (int)u32temp;

	// Get Chip_Revision
	_mdrv_dmd_isdbt_md_get_chip_revision(0, &u8temp);
	chip_revision = (int)u8temp;

	// Get layer_Num
	_mdrv_dmd_isdbt_md_get_layer_num(0, &u8temp);
	layer_num = (int)u8temp;

	return scnprintf(buf, PAGE_SIZE, "%d %d %d %d %d %d %X %d\n",
	err_flg[0], err_flg[1], err_flg[2], ts_rate[0], ts_rate[1], ts_rate[2],
	chip_revision, layer_num);
}

#ifdef UTPA2

#if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

////////////////////////////////////////////////////////////////////////////////
///                            Utopia2 Interface                             ///
////////////////////////////////////////////////////////////////////////////////

u8 mdrv_dmd_isdbt_setdbglevel(enum dmd_isdbt_dbglv dbglv)
{
	isdbt_dbg_level_param dbglv_param = {DMD_ISDBT_DBGLV_NONE};

	if (!_isdbt_open)
		return FALSE;

	dbglv_param.dbglevel = dbglv;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_SetDbgLevel,
		&dbglv_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;
}

struct dmd_isdbt_info *mdrv_dmd_isdbt_getinfo(void)
{
	isdbt_get_info_param get_info_param = {0};

	if (!_isdbt_open)
		return NULL;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_GetInfo,
		&get_info_param) == UTOPIA_STATUS_SUCCESS)
		return get_info_param.info;
	else
		return NULL;
}

u8 mdrv_dmd_isdbt_md_init(u8 id,
struct dmd_isdbt_initdata *pdmd_isdbt_initdata,
u32 init_data_len)
{
	void *pattribte = NULL;
	isdbt_init_param init_param = {0};

	if (_isdbt_open == 0) {
		if (UtopiaOpen(MODULE_ISDBT, &_pisdbt_instant, 0,
			pattribte) == UTOPIA_STATUS_SUCCESS)
			_isdbt_open = 1;
		else
			return FALSE;
	}

	if (!_isdbt_open)
		return FALSE;

	init_param.id = id;
	init_param.pdmd_isdbt_initdata = pdmd_isdbt_initdata;
	init_param.init_data_len = init_data_len;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_Init,
		&init_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_exit(u8 id)
{
	isdbt_id_param id_param = {0};

	if (!_isdbt_open)
		return FALSE;

	id_param.id = id;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_Exit,
		&id_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;
}

u32 mdrv_dmd_isdbt_md_getconfig(u8 id,
struct dmd_isdbt_initdata *psdmd_isdbt_initdata)
{
	isdbt_init_param init_param = {0};

	if (!_isdbt_open)
		return FALSE;

	init_param.id = id;
	init_param.pdmd_isdbt_initdata = psdmd_isdbt_initdata;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetConfig,
		&init_param) == UTOPIA_STATUS_SUCCESS)
		return UTOPIA_STATUS_SUCCESS;
	else
		return UTOPIA_STATUS_ERR_NOT_AVAIL;

}

u8 mdrv_dmd_isdbt_md_advsetconfig(u8 id,
enum dmd_isdbt_demod_type etype, u8 benable)
{
	isdbt_set_config_param set_config_param = {0};

	if (!_isdbt_open)
		return FALSE;

	set_config_param.id = id;
	set_config_param.etype = etype;
	set_config_param.benable = benable;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_AdvSetConfig,
		&set_config_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_setconfig(u8 id, u8 benable)
{
	return mdrv_dmd_isdbt_md_advsetconfig(id, DMD_ISDBT_DEMOD_6M, benable);
}

#if DMD_ISDBT_STR_EN
u32 mdrv_dmd_isdbt_md_setpowerstate(u8 id,
enum EN_POWER_MODE power_state)
{
	isdbt_set_power_state_param set_powerstate_param = {0};

	if (!_isdbt_open)
		return FALSE;

	set_powerstate_param.id = id;
	set_powerstate_param.power_state = power_state;

	return UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_SetPowerState,
	&set_powerstate_param);
}
#endif

enum dmd_isdbt_lock_status mdrv_dmd_isdbt_md_getlock(u8 id,
enum dmd_isdbt_getlock_type etype)
{
	isdbt_get_lock_param get_lock_param = {0};

	if (!_isdbt_open)
		return DMD_ISDBT_NULL;

	get_lock_param.id = id;
	get_lock_param.etype = etype;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetLock,
		&get_lock_param) == UTOPIA_STATUS_SUCCESS)
		return get_lock_param.status;
	else
		return DMD_ISDBT_NULL;

}

u8 mdrv_dmd_isdbt_md_get_modulation_mode(u8 id,
enum en_isdbt_layer elayer_index,
struct isdbt_modulation_mode *sisdbt_modulation_mode)
{
	isdbt_get_modulation_mode_param get_modulation_mode_param = {0};

	if (!_isdbt_open)
		return FALSE;

	get_modulation_mode_param.elayer_index = elayer_index;
	get_modulation_mode_param.id = id;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetModulationMode,
		&get_modulation_mode_param) == UTOPIA_STATUS_SUCCESS) {
		*sisdbt_modulation_mode =
		get_modulation_mode_param.isdbt_modulation_mode;
		return TRUE;
	} else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_get_signalstrength(u8 id, u16 *strength)
{
	isdbt_get_signal_strength_param get_signalstrength_param = {0};

	if (!_isdbt_open)
		return FALSE;

	get_signalstrength_param.id = id;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetSignalStrength,
		&get_signalstrength_param) == UTOPIA_STATUS_SUCCESS) {
		*strength = get_signalstrength_param.strength;

		return TRUE;
	} else
		return FALSE;

}

#ifndef MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_isdbt_md_getfreqoffset(u8 id, float *cfo)
#else
u8 mdrv_dmd_isdbt_md_getfreqoffset(u8 id,
struct dmd_isdbt_cfo_data *freq_offset)
#endif
{
	isdbt_get_freq_offset_param get_freqoffset_param = {0};
	#ifndef MSOS_TYPE_LINUX_KERNEL
	float   ftdcfo_freq = 0.0;
	float   ffdcfo_freq = 0.0;
	float   ficfo_freq  = 0.0;
	#endif

	if (!_isdbt_open)
		return FALSE;

	get_freqoffset_param.id = id;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetFreqOffset,
		&get_freqoffset_param) == UTOPIA_STATUS_SUCCESS) {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		ftdcfo_freq = ((float)get_freqoffset_param.cfo.tdcfo_regvalue) /
		17179869184.0; //<25,34>
		ftdcfo_freq = ftdcfo_freq * 8126980.0;

		ffdcfo_freq = ((float)get_freqoffset_param.cfo.fdcfo_regvalue) /
		17179869184.0;
		ffdcfo_freq = ffdcfo_freq * 8126980.0;

		if ((get_freqoffset_param.cfo.fft_mode & 0x30) == 0x0000) // 2k
			ficfo_freq =
			(float)get_freqoffset_param.cfo.icfo_regvalue*
			250000.0/63.0;
		else if ((get_freqoffset_param.cfo.fft_mode & 0x0030) == 0x0010)
			ficfo_freq =
			(float)get_freqoffset_param.cfo.icfo_regvalue*
			125000.0/63.0;
		else //if (u16data & 0x0030 == 0x0020) // 8k
			ficfo_freq =
			(float)get_freqoffset_param.cfo.icfo_regvalue*
			125000.0/126.0;

		*cfo = ftdcfo_freq + ffdcfo_freq + ficfo_freq;
		#else
		freq_offset->fft_mode      = get_freqoffset_param.cfo.fft_mode;
		freq_offset->tdcfo_regvalue =
			get_freqoffset_param.cfo.tdcfo_regvalue;
		freq_offset->fdcfo_regvalue =
			get_freqoffset_param.cfo.fdcfo_regvalue;
		freq_offset->fdcfo_regvalue =
			get_freqoffset_param.cfo.icfo_regvalue;
		#endif
	} else {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		*cfo = 0;
		#else
		freq_offset->fft_mode      = 0;
		freq_offset->tdcfo_regvalue = 0;
		freq_offset->fdcfo_regvalue = 0;
		freq_offset->icfo_regvalue = 0;
		#endif

		return FALSE;
	}
	return TRUE;
}

u16 mdrv_dmd_isdbt_md_get_signalquality(u8 id)
{
	return mdrv_dmd_isdbt_md_get_signalqualityoflayerA(id);
}

u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerA(u8 id)
{
	isdbt_get_lock_param get_lock_param = {0};
	isdbt_get_signal_quality_param get_signalquality_param = {0};

	if (!_isdbt_open)
		return 0;

	get_lock_param.id = id;
	get_lock_param.etype = DMD_ISDBT_GETLOCK_FEC_LOCK;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetLock,
		&get_lock_param) == UTOPIA_STATUS_SUCCESS) {
		if (get_lock_param.status == DMD_ISDBT_LOCK) {
			if (UtopiaIoctl(_pisdbt_instant,
			DMD_ISDBT_DRV_CMD_MD_GetSignalQualityOfLayerA,
			&get_signalquality_param) == UTOPIA_STATUS_SUCCESS)
				return get_signalquality_param.signal_quality;
		}
	}

	return 0;
}

u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerB(u8 id)
{
	isdbt_get_lock_param get_lock_param = {0};
	isdbt_get_signal_quality_param get_signalquality_param = {0};

	if (!_isdbt_open)
		return 0;

	get_lock_param.id = id;
	get_lock_param.etype = DMD_ISDBT_GETLOCK_FEC_LOCK;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetLock,
		&get_lock_param) == UTOPIA_STATUS_SUCCESS) {
		if (get_lock_param.status == DMD_ISDBT_LOCK) {
			if (UtopiaIoctl(_pisdbt_instant,
			DMD_ISDBT_DRV_CMD_MD_GetSignalQualityOfLayerB,
			&get_signalquality_param) == UTOPIA_STATUS_SUCCESS)
				return get_signalquality_param.signal_quality;
		}
	}

	return 0;
}

u16 mdrv_dmd_isdbt_md_get_signalqualityoflayerC(u8 id)
{
	isdbt_get_lock_param get_lock_param = {0};
	isdbt_get_signal_quality_param get_signalquality_param = {0};

	if (!_isdbt_open)
		return 0;

	get_lock_param.id = id;
	get_lock_param.etype = DMD_ISDBT_GETLOCK_FEC_LOCK;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetLock,
		&get_lock_param) == UTOPIA_STATUS_SUCCESS) {
		if (get_lock_param.status == DMD_ISDBT_LOCK) {
			if (UtopiaIoctl(_pisdbt_instant,
			DMD_ISDBT_DRV_CMD_MD_GetSignalQualityOfLayerC,
			&get_signalquality_param) == UTOPIA_STATUS_SUCCESS)
				return get_signalquality_param.signal_quality;
		}
	}

	return 0;
}

u16 mdrv_dmd_isdbt_md_get_signalqualitycombine(u8 id)
{
	isdbt_get_modulation_mode_param get_modulation_mode_param = {0};
	s8  layerA_value = 0, layerB_value = 0, layerC_value = 0;

	if (!_isdbt_open)
		return FALSE;

	get_modulation_mode_param.id = id;

	get_modulation_mode_param.elayer_index = E_ISDBT_Layer_A;
	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetModulationMode,
		&get_modulation_mode_param) != UTOPIA_STATUS_SUCCESS)
		return 0;
	layerA_value =
get_modulation_mode_param.isdbt_modulation_mode.eisdbt_constellation ==
	E_ISDBT_QAM_INVALID ? -1 :
(s8)get_modulation_mode_param.isdbt_modulation_mode.eisdbt_constellation;

get_modulation_mode_param.elayer_index = E_ISDBT_Layer_B;
	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetModulationMode,
		&get_modulation_mode_param) != UTOPIA_STATUS_SUCCESS)
		return 0;
	layerB_value =
get_modulation_mode_param.isdbt_modulation_mode.eisdbt_constellation ==
	E_ISDBT_QAM_INVALID ? -1 :
(s8)get_modulation_mode_param.isdbt_modulation_mode.eisdbt_constellation;

	get_modulation_mode_param.elayer_index = E_ISDBT_Layer_C;
	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetModulationMode,
		&get_modulation_mode_param) != UTOPIA_STATUS_SUCCESS)
		return 0;
	layerC_value =
get_modulation_mode_param.isdbt_modulation_mode.eisdbt_constellation ==
	E_ISDBT_QAM_INVALID ? -1 :
(s8)get_modulation_mode_param.isdbt_modulation_mode.eisdbt_constellation;

	if (layerA_value >= layerB_value) {
		if (layerC_value >= layerA_value)
			return mdrv_dmd_isdbt_md_get_signalqualityoflayerC(id);
		else  //A>C
			return mdrv_dmd_isdbt_md_get_signalqualityoflayerA(id);
	} else  {// B >= A
		if (layerC_value >= layerB_value)
			return mdrv_dmd_isdbt_md_get_signalqualityoflayerC(id);
		else  //B>C
			return mdrv_dmd_isdbt_md_get_signalqualityoflayerB(id);
	}
}

#ifndef MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_isdbt_md_getsnr(u8 id, float *f_snr)
#else
u8 mdrv_dmd_isdbt_md_getsnr(u8 id, struct dmd_isdbt_snr_data *f_snr)
#endif
{
	isdbt_get_snr_param get_snr_param = {0};

	if (!_isdbt_open)
		return FALSE;

	get_snr_param.id = id;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetSNR,
		&get_snr_param) == UTOPIA_STATUS_SUCCESS) {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		*f_snr = (float)get_snr_param.snr.reg_snr/100;
		#else
		f_snr->reg_snr       = get_snr_param.snr.reg_snr;
		f_snr->reg_snr_obs_num = get_snr_param.snr.reg_snr_obs_num;
		#endif
	} else {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		*f_snr = 0;
		#else
		f_snr->reg_snr       = 0;
		f_snr->reg_snr_obs_num = 0;
		#endif

		return FALSE;
	}
	return TRUE;
}

#ifndef MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_isdbt_md_get_previterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, float *fber)
#else
u8 mdrv_dmd_isdbt_md_get_previterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, struct dmd_isdbt_get_ber_value *fber)
#endif
{
	isdbt_get_ber_param get_ber_param = {0};

	if (!_isdbt_open)
		return FALSE;

	get_ber_param.id = id;
	get_ber_param.ber.eisdbt_layer = elayer_index;
	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetPreViterbiBer,
		&get_ber_param) == UTOPIA_STATUS_SUCCESS) {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		get_ber_param.ber.ber_period <<= 8; // *256

		if (get_ber_param.ber.ber_period == 0)
			get_ber_param.ber.ber_period = 1;

		*fber = (float)get_ber_param.ber.ber_value/
		get_ber_param.ber.ber_period;
		#else
		fber->eisdbtlayer = get_ber_param.ber.eisdbt_layer;
		fber->bervalue    = get_ber_param.ber.ber_value;
		fber->berperiod   = get_ber_param.ber.ber_period;
		#endif
	} else {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		*fber = 0;
		#else
		fber->eisdbtlayer = get_ber_param.ber.eisdbt_layer;
		fber->bervalue    = 0;
		fber->berperiod   = 0;
		#endif

		return FALSE;
	}
	return TRUE;
}

#ifndef MSOS_TYPE_LINUX_KERNEL
u8 mdrv_dmd_isdbt_md_get_postviterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, float *fber)
#else
u8 mdrv_dmd_isdbt_md_get_postviterbi_ber(u8 id,
enum en_isdbt_layer elayer_index, struct dmd_isdbt_get_ber_value *fber)
#endif
{
	isdbt_get_ber_param get_ber_param = {0};

	if (!_isdbt_open)
		return FALSE;

	get_ber_param.id = id;
	get_ber_param.ber.eisdbt_layer = elayer_index;
	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetPostViterbiBer,
		&get_ber_param) == UTOPIA_STATUS_SUCCESS) {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		if (get_ber_param.ber.ber_period == 0)
			get_ber_param.ber.ber_period = 1;

		*fber = (float)get_ber_param.ber.ber_value/
		get_ber_param.ber.ber_period/(128.0*188.0*8.0);

		#else
		fber->eisdbtlayer = get_ber_param.ber.eisdbt_layer;
		fber->bervalue    = get_ber_param.ber.ber_value;
		fber->berperiod   = get_ber_param.ber.ber_period;
		#endif


	} else {
		#ifndef MSOS_TYPE_LINUX_KERNEL
		*fber = 0;
		#else
		fber->eisdbtlayer = get_ber_param.ber.eisdbt_layer;
		fber->bervalue    = 0;
		fber->berperiod   = 0;
		#endif

		return FALSE;
	}
	return TRUE;
}

u8 mdrv_dmd_isdbt_md_read_pkt_err(u8 id,
enum en_isdbt_layer elayer_index, u16 *packet_err)
{
	isdbt_read_pkt_err_param read_pkterr_param = {0};

	if (!_isdbt_open)
		return FALSE;

	read_pkterr_param.id = id;
	read_pkterr_param.eisdbt_layer = elayer_index;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_Read_PKT_ERR,
		&read_pkterr_param) == UTOPIA_STATUS_SUCCESS) {
		*packet_err = read_pkterr_param.packet_err;

		return TRUE;
	} else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_set_serialcontrol(u8 id, u8 ts_config_data)
{
	isdbt_set_serial_control_param set_serialcontrol_param = {0};

	if (!_isdbt_open)
		return FALSE;

	set_serialcontrol_param.id = id;
	set_serialcontrol_param.ts_config_data = ts_config_data;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_SetSerialControl,
		&set_serialcontrol_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_iic_bypass_mode(u8 id, u8 benable)
{
	isdbt_iic_bypass_mode_param iic_bypassmode_param = {0};

	if (!_isdbt_open)
		return FALSE;

	iic_bypassmode_param.id = id;
	iic_bypassmode_param.benable = benable;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_IIC_BYPASS_MODE,
		&iic_bypassmode_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_switch_sspi_gpio(u8 id, u8 benable)
{
	isdbt_switch_sspi_gpio_param switch_sspi_gpio_param = {0};

	if (!_isdbt_open)
		return FALSE;

	switch_sspi_gpio_param.id = id;
	switch_sspi_gpio_param.benable = benable;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_SWITCH_SSPI_GPIO,
		&switch_sspi_gpio_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_gpio_get_level(u8 id, u8 pin, u8 *blevel)
{
	isdbt_gpio_level_param gpio_level_param = {0};

	if (!_isdbt_open)
		return FALSE;

	gpio_level_param.id = id;
	gpio_level_param.pin = pin;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GPIO_GET_LEVEL,
		&gpio_level_param) == UTOPIA_STATUS_SUCCESS) {
		*blevel = gpio_level_param.blevel;

		return TRUE;
	} else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_gpio_set_level(u8 id, u8 pin, u8 blevel)
{
	isdbt_gpio_level_param gpio_level_param = {0};

	if (!_isdbt_open)
		return FALSE;

	gpio_level_param.id = id;
	gpio_level_param.pin = pin;
	gpio_level_param.blevel = blevel;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GPIO_SET_LEVEL,
		&gpio_level_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_gpio_out_enable(u8 id, u8 pin,
u8 enable_out)
{
	isdbt_gpio_out_enable_param gpio_out_enable_param = {0};

	if (!_isdbt_open)
		return FALSE;

	gpio_out_enable_param.id = id;
	gpio_out_enable_param.pin = pin;
	gpio_out_enable_param.benable_out = enable_out;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GPIO_OUT_ENABLE,
		&gpio_out_enable_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_do_iq_swap(u8 id, u8 bis_qpad)
{
	isdbt_do_iq_swap_param do_iqswap_param = {0};

	if (!_isdbt_open)
		return FALSE;

	do_iqswap_param.id = id;
	do_iqswap_param.bis_qpad = bis_qpad;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_DoIQSwap,
		&do_iqswap_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_getreg(u8 id, u16 addr, u8 *pdata)
{
	isdbt_reg_param reg_param = {0};

	if (!_isdbt_open)
		return FALSE;

	reg_param.id = id;
	reg_param.addr = addr;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_GetReg,
		&reg_param) == UTOPIA_STATUS_SUCCESS) {
		*pdata = reg_param.data;

		return TRUE;
	} else
		return FALSE;

}

u8 mdrv_dmd_isdbt_md_setreg(u8 id, u16 addr, u8 data)
{
	isdbt_reg_param reg_param = {0};

	if (!_isdbt_open)
		return FALSE;

	reg_param.id = id;
	reg_param.addr = addr;
	reg_param.data = data;

	if (UtopiaIoctl(_pisdbt_instant, DMD_ISDBT_DRV_CMD_MD_SetReg,
		&reg_param) == UTOPIA_STATUS_SUCCESS)
		return TRUE;
	else
		return FALSE;

}

#endif // #if DMD_ISDBT_UTOPIA_EN || DMD_ISDBT_UTOPIA2_EN

#endif // #ifdef UTPA2

