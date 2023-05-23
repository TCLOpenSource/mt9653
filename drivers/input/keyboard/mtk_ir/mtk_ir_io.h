/* SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause */

#ifndef _MDRV_IR_IO_H_
#define _MDRV_IR_IO_H_
#include "mtk_ir_st.h"

//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define IR_IOC_MAGIC                'u'

#define MDRV_IR_INIT                _IO(IR_IOC_MAGIC, 0)
#define MDRV_IR_SET_DELAYTIME       _IOW(IR_IOC_MAGIC, 1, int)
#define MDRV_IR_LTP_INIT            _IOW(IR_IOC_MAGIC, 2, int)
#define MDRV_IR_LTP_RESULT          _IOR(IR_IOC_MAGIC, 3, int)
#define MDRV_IR_ENABLE_IR           _IOW(IR_IOC_MAGIC, 6, int)
#define MDRV_IR_SET_MASTER_PID      _IOW(IR_IOC_MAGIC, 9, int)
#define MDRV_IR_GET_MASTER_PID      _IOW(IR_IOC_MAGIC, 10, int)
#define MDRV_IR_INITCFG             _IOW(IR_IOC_MAGIC, 11, struct MS_IR_InitCfg_s)
#define MDRV_IR_TIMECFG             _IOW(IR_IOC_MAGIC, 12, struct MS_IR_TimeTail_s)
#define MDRV_IR_GET_SWSHOT_BUF      _IOW(IR_IOC_MAGIC, 13, struct MS_IR_ShotInfo_s)
#define MDRV_IR_SEND_KEY            _IOW(IR_IOC_MAGIC, 14, int)
#define MDRV_IR_SET_HEADER          _IOW(IR_IOC_MAGIC, 15, struct MS_MultiIR_HeaderInfo_s)
#define MDRV_IR_SET_PROTOCOL        _IOW(IR_IOC_MAGIC, 16, int)
#define MDRV_IR_GET_POWERKEY        _IOW(IR_IOC_MAGIC, 17, int)
#define MDRV_IR_SET_KEY_EVENT       _IOW(IR_IOC_MAGIC, 18, u8)
#define MDRV_IR_ENABLE_WB_IR        _IOW(IR_IOC_MAGIC, 19, u8)

#ifdef CONFIG_MTK_TV_DYNAMIC_IR
#define IR_IOC_MAXNR                19
#else
#define IR_IOC_MAXNR                17
#endif
//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

#endif // _MDRV_IR_IO_H_
