/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _SND_SOC_DMA_H_
#define _SND_SOC_DMA_H_

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define DMA_EMPTY		0
#define DMA_UNDERRUN	1
#define DMA_OVERRUN		2
#define DMA_FULL		3
#define DMA_NORMAL		4
#define DMA_LOCALFULL	5
#define MIU_BASE 0x20000000

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
extern unsigned long long g_nCapStartTime;

struct voc_pcm_dma_data {
	unsigned char *name;	/* stream identifier */
	unsigned long channel;	/* Channel ID */
};

struct voc_pcm_runtime_data {
	spinlock_t            lock;
	unsigned int state;
	size_t dma_level_count;
	size_t int_level_count;
	struct voc_pcm_dma_data *dma_data;
	void *private_data;
};

#endif /* _SND_SOC_DMA_H_ */
