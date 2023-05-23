/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#ifndef _MI_COMMON_H_
#define _MI_COMMON_H_

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>  // Needed by all modules
#include <linux/kernel.h>  // Needed for KERN_INFO
#include <linux/fs.h>      // Needed by filp

//-------------------------------------------------------------------------------------------------
//  System Data Type
//-------------------------------------------------------------------------------------------------
/// data type unsigned char, data length 1 byte
typedef unsigned char                                   MI_U8;      ///< 1 byte
/// data type unsigned short, data length 2 byte
typedef unsigned short                                  MI_U16;     ///< 2 bytes
/// data type unsigned int, data length 4 byte
typedef unsigned int                                    MI_U32;     ///< 4 bytes
/// data type unsigned int, data length 8 byte
typedef unsigned long long                              MI_U64;     ///< 8 bytes
/// data type signed char, data length 1 byte
typedef signed char                                     MI_S8;      ///< 1 byte
/// data type signed short, data length 2 byte
typedef signed short                                    MI_S16;     ///< 2 bytes
/// data type signed int, data length 4 byte
typedef signed int                                      MI_S32;     ///< 4 bytes
/// data type signed int, data length 8 byte
typedef signed long long                                MI_S64;     ///< 8 bytes

/// data type virtual address
typedef MI_U64                                          MI_VIRT;    ///< 8 bytes
/// date type physical address
typedef MI_U64                                          MI_PHY;     ///< 8 bytes

/// data type unsigned int, data length 4 byte
typedef unsigned int                                    MI_UINT;    ///< 4 bytes

/// date type size_t
typedef size_t                                          MI_SIZE;

/// data type null pointer
#ifdef NULL
#undef NULL
#endif
#define NULL                                            0

//-------------------------------------------------------------------------------------------------
//  Software Data Type
//-------------------------------------------------------------------------------------------------
/// definition for MI_BOOL
typedef unsigned char                                   MI_BOOL;
typedef MI_S32                                          MI_HANDLE;

#ifndef true
/// definition for true
#define true                                            1
/// definition for false
#define false                                           0
#endif

#if !defined(TRUE) && !defined(FALSE)
/// definition for TRUE
#define TRUE                                            1
/// definition for FALSE
#define FALSE                                           0
#endif

//-------------------------------------------------------------------------------------------------
//  Defines - Constant
//-------------------------------------------------------------------------------------------------
#define MI_HANDLE_NULL                                  (0)
#define MI_INVALID_PTS                                  ((MI_U64)-1)

///MI Resule and error code definitions
#define MI_RESULT                     MI_U32
#define MI_OK                     0x0 ///< succeeded
#define MI_CONTINUE               0x1 ///< not error but hasn' be succeeded. Flow is continue...
#define MI_HAS_INITED             0x2 ///< not error but init has be called again
#define MI_ERR_FAILED             0x3 ///< general failed
#define MI_ERR_NOT_INITED         0x4 ///< moudle hasn't be inited
#define MI_ERR_NOT_SUPPORT        0x5 ///< not supported
#define MI_ERR_NOT_IMPLEMENT      0x6 ///< not implemented
#define MI_ERR_INVALID_HANDLE     0x7 ///< invalid handle
#define MI_ERR_INVALID_PARAMETER  0x8 ///< invalid parameter
#define MI_ERR_RESOURCES          0x9 ///< system resource issue
#define MI_ERR_MEMORY_ALLOCATE    0xa ///< memory allocation
#define MI_ERR_CHAOS              0xb ///< chaos state-mechine
#define MI_ERR_DATA_ERROR         0xc ///< data error
#define MI_ERR_TIMEOUT            0xd ///< timeout
#define MI_ERR_LIMITION           0xe ///< limitation
#define MI_ERR_BUSY               0xf ///< can not be done due to current system is busy.


//MI_DBG_LEVEL
#define    MI_DBG_LEVEL               MI_U32
#define    MI_DBG_NONE                0
#define    MI_DBG_FATAL               0x10
#define    MI_DBG_ERR                 0x20
#define    MI_DBG_WRN                 0x30
#define    MI_DBG_INFO                0x40
#define    MI_DBG_TRACE               0x50
#define    MI_DBG_ALL                 0xF0
#define    MI_DBG_DRV_ERR             (1 << 8)
#define    MI_DBG_DRV_WRN             (2 << 8)
#define    MI_DBG_DRV_INFO            (3 << 8)
#define    MI_DBG_DRV_ALL             (4 << 8)
#define    MI_DBG_FW                  (1 << 16)
#define    MI_DBG_KDRV_WRAPPER_ERR    (1 << 24)
#define    MI_DBG_KDRV_WRAPPER_WRN    (2 << 24)
#define    MI_DBG_KDRV_WRAPPER_INFO   (3 << 24)
#define    MI_DBG_KDRV_WRAPPER_ALL    (4 << 24)

/// For MI module connection use
#define MI_MODULE_TYPE_NONE           0x00    ///< Not defined
#define MI_MODULE_TYPE_ACAP           0x12    ///< MI_ACAP
#define MI_MODULE_TYPE_AEXTIN         0X14    ///< MI_AEXTIN
#define MI_MODULE_TYPE_AOUT           0x17    ///< MI_AOUT
#define MI_MODULE_TYPE_AUDIO          0x19    ///< MI_AUDIO
#define MI_MODULE_TYPE_CAP            0x21    ///< MI_CAP
#define MI_MODULE_TYPE_DISP           0x36    ///< MI_DISP
#define MI_MODULE_TYPE_EXTIN          0x43    ///< MI_EXTIN
#define MI_MODULE_TYPE_PCM            0x81    ///< MI_PCM
#define MI_MODULE_TYPE_TSIO           0xAB    ///< MI_TSIO
#define MI_MODULE_TYPE_TUNER          0xAE    ///< MI_TUNER
#define MI_MODULE_TYPE_VIDEO          0Xc7    ///< MI_VIDEO
#define MI_MODULE_TYPE_PCMCIA         0X82    ///< MI_PCMCIA

///ASCII color code
#define ASCII_COLOR_BLACK             "\033[30m"
#define ASCII_COLOR_DARK_RED          "\033[31m"
#define ASCII_COLOR_DARK_GREEN        "\033[32m"
#define ASCII_COLOR_BROWN             "\033[33m"
#define ASCII_COLOR_DARK_BLUE         "\033[34m"
#define ASCII_COLOR_DARK_PURPLE       "\033[35m"
#define ASCII_COLOR_DARK_CYAN         "\033[36m"
#define ASCII_COLOR_GRAY              "\033[37m"
#define ASCII_COLOR_DARK_GRAY         "\033[1;30m"
#define ASCII_COLOR_RED               "\033[1;31m"
#define ASCII_COLOR_GREEN             "\033[1;32m"
#define ASCII_COLOR_YELLOW            "\033[1;33m"
#define ASCII_COLOR_BLUE              "\033[1;34m"
#define ASCII_COLOR_PURPLE            "\033[1;35m"
#define ASCII_COLOR_CYAN              "\033[1;36m"
#define ASCII_COLOR_WHITE             "\033[1;37m"
#define ASCII_COLOR_INVERSE_BLACK     "\033[1;7;30m"
#define ASCII_COLOR_INVERSE_RED       "\033[1;7;31m"
#define ASCII_COLOR_INVERSE_GREEN     "\033[1;7;32m"
#define ASCII_COLOR_INVERSE_YELLOW    "\033[1;7;33m"
#define ASCII_COLOR_INVERSE_BLUE      "\033[1;7;34m"
#define ASCII_COLOR_INVERSE_PURPLE    "\033[1;7;35m"
#define ASCII_COLOR_INVERSE_CYAN      "\033[1;7;36m"
#define ASCII_COLOR_INVERSE_WHITE     "\033[1;7;37m"
#define ASCII_COLOR_END               "\033[0m"

//Handle string length
#define  MI_MODULE_HANDLE_NAME_LENGTH_MAX 64

//-------------------------------------------------------------------------------------------------
//  Defines - Macros
//-------------------------------------------------------------------------------------------------
#define MI_ADDR_CAST(t)                                 (t)(size_t)
#define MI_BIT(_bit_)                                   (1 << (_bit_))

#ifndef MI_MEM_ALIGN
#define MI_MEM_ALIGN(align, address)                    ((((address)+(align)-1)/(align))*(align))
#endif

#ifndef MI_LOW_MEM_ALIGN
#define MI_LOW_MEM_ALIGN(align, address)                (((address)/(align))*(align))
#endif


//-------------------------------------------------------------------------------------------------
//  Types - Enums
//-------------------------------------------------------------------------------------------------


#endif///_MI_COMMON_H_

