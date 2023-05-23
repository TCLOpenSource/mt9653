/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#ifndef _STI_MSOS_H_
#define _STI_MSOS_H_

// #include <sys/ioccom.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/firmware.h>
#include "mtk_reg_jpd.h"

#define SUPPORT_PROGRESSIVE 1

#ifndef DLL_PACKED
#define DLL_PACKED __attribute__((__packed__))
#endif

#ifdef MSOS_TYPE_LINUX_KERNEL
#undef printf
#define printf printk
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef enum {
	E_NUM0 = 0,
	E_NUM1,
	E_NUM2,
	E_NUM3,
	E_NUM4,
	E_NUM5,
	E_NUM6,
	E_NUM7,
	E_NUM8,
	E_NUM9,
	E_NUM10,
	E_NUM11,
	E_NUM12,
	E_NUM13,
	E_NUM14,
	E_NUM15,
	E_NUM16,
	E_NUM24 = 24,
	E_NUM32 = 32,
	E_NUM63 = 63,
	E_NUM64 = 64,
	E_NUM66 = 66,
	E_NUM71 = 71,
	E_NUM82 = 82,
} BIT_NUM;

typedef enum {
	E_OFFSET3 = 3,
	E_OFFSET4 = 4,
	E_OFFSET8 = 8,
	E_OFFSET16 = 16,
	E_OFFSET32 = 32,
	E_OFFSET48 = 48,
	E_OFFSET64 = 64,
	E_OFFSET80 = 80,
	E_OFFSET96 = 96,
} OFFSET_NUM;


typedef enum {
	E_MARKER_00 = 0x00,
	E_MARKER_0F = 0x0f,
	E_MARKER_10 = 0x10,
	E_MARKER_F0 = 0xf0,
	E_MARKER_FF = 0xff,
	E_MARKER_8000 = 0x8000,
	E_MARKER_FF00 = 0xff00,
	E_MARKER_FFFE = 0xfffeUL,
	E_MARKER_FFFF = 0xffffUL,
} MARKER;

void reg16_write(REG16 __iomem *offset, __u16 value);
__u16 reg16_read(REG16 __iomem *offset);
void reg8_write(REG16 __iomem *offset, __u8 value);
__u8 reg8_read(REG16 __iomem *offset);
void reg8_mask_write(REG16 __iomem *offset, __u8 mask, __u8 value);
void STI_NJPD_PowerOn(void);
void STI_NJPD_PowerOff(void);

void *STI_MALLOC(__u32 size);
void STI_FREE(void *x, __u32 size);

void MsOS_DelayTask(__u32 u32Ms);

void MsOS_DelayTaskUs(__u32 u32Us);

__u32 MsOS_GetSystemTime(void);

void MsOS_FlushMemory(void);


#endif				// _STI_MSOS_H_
