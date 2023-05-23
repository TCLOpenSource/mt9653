/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __APW_PLAT_MT5879_REG_H__
#define __APW_PLAT_MT5879_REG_H__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/time.h>
#include "apu_common.h"

#define APMCU_RIUBASE 0x1C000000
#define RIU_ADDR(bank, offset)		(APMCU_RIUBASE + ((bank<<8)<<1) + (offset<<2))
#define REG_OFFSET_SHIFT_BITS		2

#define CKGEN00	0x1020
#define X32_AIA_TOP 0x271a
#define MCUSYS_XIU_BRIDGE 0x2009
#define X32_APUSYS (0x2800)

#define X32_APUSYS_BASE (0x2800<<9)


// APB Module apu_conn
//#define APU_CONN_BASE (0x19020000)
#define APU_CONN_BASE (APMCU_RIUBASE+X32_APUSYS_BASE+0x00000)
#define APU_CONN_CG_CON	((phys_addr_t)(APU_CONN_BASE+0x000))
#define APU_CONN_CG_SET	((phys_addr_t)(APU_CONN_BASE+0x004))
#define APU_CONN_CG_CLR	((phys_addr_t)(APU_CONN_BASE+0x008))

// APB Module apusys_vcore
//#define APUSYS_VCORE_BASE (0x19029000)
#define APUSYS_VCORE_BASE (APMCU_RIUBASE+X32_APUSYS_BASE+0x01000)
#define APUSYS_VCORE_CG_CON	((phys_addr_t)(APUSYS_VCORE_BASE+0x000))
#define APUSYS_VCORE_CG_SET	((phys_addr_t)(APUSYS_VCORE_BASE+0x004))
#define APUSYS_VCORE_CG_CLR	((phys_addr_t)(APUSYS_VCORE_BASE+0x008))

// APB Module apu_mdla
//#define APU_MDLA0_BASE (0x19034000)
#define APU_MDLA0_BASE (APMCU_RIUBASE+X32_APUSYS_BASE+0x08000)
#define APU_MDLA0_APU_MDLA_CG_CON	((phys_addr_t)(APU_MDLA0_BASE+0x000))
#define APU_MDLA0_APU_MDLA_CG_SET	((phys_addr_t)(APU_MDLA0_BASE+0x004))
#define APU_MDLA0_APU_MDLA_CG_CLR	((phys_addr_t)(APU_MDLA0_BASE+0x008))

//#define APU_MDLA0_CMDE_BASE (0x19036000)
#define APU_MDLA0_CMDE_BASE (APMCU_RIUBASE+X32_APUSYS_BASE+0x09000)
// bit0: idle, bit4: swcmd done
#define APU_MDLA0_CMDE_INT_STATUS ((phys_addr_t)(APU_MDLA0_CMDE_BASE+0x504))
#define APU_MDLA0_CMDE_INT_ENABLE ((phys_addr_t)(APU_MDLA0_CMDE_BASE+0x508))
#define APU_MDLA0_CMDE_INT_MASK   ((phys_addr_t)(APU_MDLA0_CMDE_BASE+0x50c))
#define APU_MDLA0_CMDE_510        ((phys_addr_t)(APU_MDLA0_CMDE_BASE+0x510))
#define APU_MDLA0_CMDE_514        ((phys_addr_t)(APU_MDLA0_CMDE_BASE+0x514))
#define APU_MDLA0_CMDE_518        ((phys_addr_t)(APU_MDLA0_CMDE_BASE+0x518))
#define APU_MDLA0_CMDE_51c        ((phys_addr_t)(APU_MDLA0_CMDE_BASE+0x51c))

#define APU_MDLA0_BIU_BASE (APMCU_RIUBASE+X32_APUSYS_BASE+0x0b000)

#endif
