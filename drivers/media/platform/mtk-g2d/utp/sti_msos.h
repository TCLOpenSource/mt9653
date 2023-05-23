/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _STI_MSOS_H_
#define _STI_MSOS_H_

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/slab.h>

//////////////////////////////////// extern VAR defines
#define HAL_MIU1_BASE               0x80000000UL	// 1512MB

#define MODULE_GFX 0

#define UTOPIA_STATUS_SUCCESS               0x00000000
#define UTOPIA_STATUS_FAIL                  0x00000001
#define UTOPIA_STATUS_NOT_SUPPORTED         0x00000002
#define UTOPIA_STATUS_PARAMETER_ERROR       0x00000003

//////////////////////////////////// compiler defines

#ifndef DLL_PACKED
#define DLL_PACKED __attribute__((__packed__))
#endif

#ifndef DLL_PUBLIC
#define DLL_PUBLIC __attribute__((visibility("default")))
#endif

#ifndef DLL_LOCAL
#define DLL_LOCAL __attribute__((visibility("hidden")))
#endif

#ifdef MSOS_TYPE_LINUX_KERNEL
#define SYMBOL_WEAK
#else
#define SYMBOL_WEAK __attribute__((weak))
#endif

//////////////////////////////////// STDLIB defines

#ifdef MSOS_TYPE_LINUX_KERNEL
#undef printf
#define printf printk
#endif

#ifdef MSOS_TYPE_LINUX_KERNEL
#define free kfree
#define malloc(size) kmalloc((size), GFP_KERNEL)
#endif

//////////////////////////////////// type defines
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef MS_U8
typedef __u8 MS_U8;
#endif

#ifndef MS_U8
typedef __s8 MS_S8;
#endif

#ifndef MS_U16
typedef __u16 MS_U16;
#endif

#ifndef MS_S16
typedef __s16 MS_S16;
#endif

#ifndef MS_U32
typedef __u32 MS_U32;
#endif

#ifndef MS_S32
typedef __s32 MS_S32;
#endif

#ifndef MS_U64
typedef __u64 MS_U64;
#endif

#ifndef MS_S64
typedef __s64 MS_S64;
#endif

#ifndef MS_BOOL
typedef bool MS_BOOL;
#endif

//      #ifndef MS_FLOAT
//      typedef float MS_FLOAT;
//      #endif

#ifndef MS_PHYADDR
typedef __u64 MS_PHYADDR;	// 8 bytes
#endif

#ifndef MS_PHY
typedef __u64 MS_PHY;		// 8 bytes
#endif

#ifndef MS_VIRT
typedef __u64 MS_VIRT;		// 8 bytes
#endif

//////////////////////////////////// utility macro

#define ALIGN_4(_x_)                (((_x_) + 3) & ~3)
#define ALIGN_8(_x_)                (((_x_) + 7) & ~7)
#define ALIGN_16(_x_)               (((_x_) + 15) & ~15)
#define ALIGN_32(_x_)               (((_x_) + 31) & ~31)

#ifndef MIN
#define MIN(_a_, _b_)               ((_a_) < (_b_) ? (_a_) : (_b_))
#endif
#ifndef MAX
#define MAX(_a_, _b_)               ((_a_) > (_b_) ? (_a_) : (_b_))
#endif

#ifndef BIT			//for Linux_kernel type, BIT redefined in <linux/bitops.h>
#define BIT(_bit_)                  (1 << (_bit_))
#endif
#define BIT_(x)                     BIT(x)	//[OBSOLETED] //TODO: remove it later
#define BITS(_bits_, _val_)     ((BIT(((1)?_bits_)+1)-BIT(((0)?_bits_))) & (_val_<<((0)?_bits_)))
#define BMASK(_bits_)               (BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))
#define BIT_64(_bit_)                (1ULL << (_bit_))

//////////////////////////////////// debug macro
#define MAJOR_IPVER_MSK	BMASK(15:8)
#define MAJOR_IPVER_SHT	8
#define MINOR_IPVER_MSK	BMASK(7:0)
#define GE_3M_SERIES_MAJOR		(0)
#define GE_M6_SERIES_MAJOR		(1)
//////////////////////////////////// enum defines
typedef enum {
	/// color format I1
	E_MS_FMT_I1 = 0x0,
	/// color format I2
	E_MS_FMT_I2 = 0x1,
	/// color format I4
	E_MS_FMT_I4 = 0x2,
	/// color format palette 256(I8)
	E_MS_FMT_I8 = 0x4,
	/// color format blinking display
	E_MS_FMT_FaBaFgBg2266 = 0x6,
	/// color format for blinking display format
	E_MS_FMT_1ABFgBg12355 = 0x7,
	/// color format RGB565
	E_MS_FMT_RGB565 = 0x8,
	/// color format ARGB1555
	/// @note <b>[URANUS] <em>ARGB1555 is only RGB555</em></b>
	E_MS_FMT_ARGB1555 = 0x9,
	/// color format ARGB4444
	E_MS_FMT_ARGB4444 = 0xa,
	/// color format ARGB1555 DST
	E_MS_FMT_ARGB1555_DST = 0xc,
	/// color format YUV422
	E_MS_FMT_YUV422 = 0xe,
	/// color format ARGB8888
	E_MS_FMT_ARGB8888 = 0xf,
	/// color format RGBA5551
	E_MS_FMT_RGBA5551 = 0x10,
	/// color format RGBA4444
	E_MS_FMT_RGBA4444 = 0x11,
	/// Start of New color define
	/// color format BGRA5551
	E_MS_FMT_BGRA5551 = 0x12,
	/// color format ABGR1555
	E_MS_FMT_ABGR1555 = 0x13,
	/// color format ABGR4444
	E_MS_FMT_ABGR4444 = 0x14,
	/// color format BGRA4444
	E_MS_FMT_BGRA4444 = 0x15,
	/// color format BGR565
	E_MS_FMT_BGR565 = 0x16,
	/// color format RGBA8888
	E_MS_FMT_RGBA8888 = 0x1d,
	/// color format BGRA8888
	E_MS_FMT_BGRA8888 = 0x1e,
	/// color format ABGR8888
	E_MS_FMT_ABGR8888 = 0x1f,
	/// color format AYUV8888
	E_MS_FMT_AYUV8888 = 0x20,
	E_MS_FMT_GENERIC = 0xFFFF,
} MS_ColorFormat;

typedef enum{
	E_POWER_SUSPEND = 1,	// State is backup in DRAM, components power off.
	E_POWER_RESUME = 2,	// Resume from Suspend or Hibernate mode
	E_POWER_MECHANICAL = 3,	// Full power off mode.
	E_POWER_SOFT_OFF = 4,	//some components remain powered for trigging boot-up.
} EN_POWER_MODE;


//////////////////////////////////// MSOS function prototyp
typedef MS_U32(*FUtopiaOpen) (void **ppInstance, const void *const pAttribute);
typedef MS_U32(*FUtopiaIOctl) (void *pInstance, MS_U32 u32Cmd, void *const pArgs);
typedef MS_U32(*FUtopiaClose) (void *pInstance);

//////////////////////////////////// MSOS function defines

#define _miu_offset_to_phy(MiuSel, Offset, PhysAddr) {PhysAddr = Offset; }
#define _phy_to_miu_offset(MiuSel, Offset, PhysAddr) {MiuSel = 0; Offset = PhysAddr; }
#define MDrv_MMIO_GetBASE(...) 0
void MsOS_DelayTask(MS_U32 u32Ms);
MS_U32 MsOS_GetSystemTime(void);
MS_S32 MsOS_GetOSThreadID(void);
void MsOS_CreateMutex(void);
MS_BOOL MsOS_ObtainMutex(void);
MS_BOOL MsOS_ReleaseMutex(void);

/*
 * instance functions
 */
MS_U32 UtopiaInstanceCreate(MS_U32 u32PrivateSize, void **ppInstance);
MS_U32 UtopiaInstanceDelete(void *pInstant);
MS_U32 UtopiaInstanceGetPrivate(void *pInstance, void **ppPrivate);
MS_U32 UtopiaInstanceGetModule(void *pInstance, void **ppModule);

/* We hope, we can support poly, ex: JPD and JPD3D as different Module */
MS_U32 UtopiaInstanceGetModuleID(void *pInstance, MS_U32 *pu32ModuleID);

/* We hope, we can support poly, ex: JPD and JPD3D as different Module */
MS_U32 UtopiaInstanceGetModuleVersion(void *pInstant, MS_U32 *pu32Version);

/* We hope we can support interface version mantain */
MS_U32 UtopiaInstanceGetAppRequiredModuleVersion(void *pInstance, MS_U32 *pu32ModuleVersion);
MS_U32 UtopiaInstanceGetPid(void *pInstance);

/*
 * module functions
 */
MS_U32 UtopiaModuleCreate(MS_U32 u32ModuleID, MS_U32 u32PrivateSize, void **ppModule);
MS_U32 UtopiaModuleGetPrivate(void *pModule, void **ppPrivate);
MS_U32 UtopiaModuleRegister(void *pModule);
MS_U32 UtopiaModuleSetupFunctionPtr(void *pModule, FUtopiaOpen fpOpen, FUtopiaClose fpClose,
								FUtopiaIOctl fpIoctl);

/*
 * resource functions
 */
MS_U32 UtopiaResourceCreate(char *u8ResourceName, MS_U32 u32PrivateSize, void **ppResource);
MS_U32 UtopiaResourceGetPrivate(void *pResource, void **ppPrivate);
MS_U32 UtopiaResourceRegister(void *pModule, void *pResouce, MS_U32 u32PoolID);
MS_U32 UtopiaResourceObtain(void *pInstant, MS_U32 u32PoolID, void **ppResource);
MS_U32 UtopiaResourceTryObtain(void *pInstant, MS_U32 u32PoolID, void **ppResource);
MS_U32 UtopiaResourceRelease(void *pResource);
MS_U32 UtopiaResourceGetPid(void *pResource);
MS_U32 UtopiaResourceGetNext(void *pModTmp, void **ppResource);
MS_U32 UtopiaModuleAddResourceStart(void *psModule, MS_U32 u32PoolID);
MS_U32 UtopiaModuleAddResourceEnd(void *psModule, MS_U32 u32PoolID);
MS_U32 UtopiaModuleResetPool(void *pModuleTmp, MS_U32 u32RPoolID);
MS_U32 UtopiaOpen(MS_U32 u32ModuleID, void **pInstant, MS_U32 u32ModuleVersion,
					const void *const pAttribte);
MS_U32 UtopiaIoctl(void *pInstant, MS_U32 u32Cmd, void *const pArgs);
MS_U32 UtopiaClose(void *pInstant);

#endif// _STI_MSOS_H_

