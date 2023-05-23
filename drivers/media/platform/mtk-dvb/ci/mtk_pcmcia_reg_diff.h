/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _REG_PCMCIA_DIFF_H_
#define _REG_PCMCIA_DIFF_H_

#define REG_PCMCIA_BASE     0x3440UL
#define REG_TOP_PCM_PE_0     0x0009UL
#define REG_TOP_PCM_PE_1     0x000AUL
#define REG_TOP_PCM_PE_2     0x000BUL
#define REG_TOP_PCM_PE_0_MASK     0xFFFFUL
#define REG_TOP_PCM_PE_1_MASK     0xFFFFUL
#define REG_TOP_PCM_PE_2_MASK     0x001FUL
#define REG_PCM_MUX     ((0x22900 >> 1) + 0x08) // GPIO_FUNC_MUX (bank: 0x3229)
#define REG_PCMADCONFIG     0x0001UL
#define REG_PCMCTRLCONFIG     0x0010UL
#define REG_PCM2CTRLCONFIG     0x0100UL
#define REG_PCM2_CDN_CONFIG     0x1000UL


#endif
