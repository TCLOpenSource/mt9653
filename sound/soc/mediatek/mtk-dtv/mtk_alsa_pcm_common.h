/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk_alsa_pcm_common.h  --  Mediatek pcm common header
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */
#ifndef _ALSA_PLATFORM_HEADER
#define _ALSA_PLATFORM_HEADER

#include <linux/delay.h>
#include <linux/version.h>
#include <linux/module.h>

#define AU_BIT0		(1<<0)
#define AU_BIT1		(1<<1)
#define AU_BIT2		(1<<2)
#define AU_BIT3		(1<<3)
#define AU_BIT4		(1<<4)
#define AU_BIT5		(1<<5)
#define AU_BIT6		(1<<6)
#define AU_BIT7		(1<<7)
#define AU_BIT8		(1<<8)
#define AU_BIT9		(1<<9)
#define AU_BIT10	(1<<10)
#define AU_BIT11	(1<<11)
#define AU_BIT12	(1<<12)
#define AU_BIT13	(1<<13)
#define AU_BIT14	(1<<14)
#define AU_BIT15	(1<<15)

#define CAP_CHANNEL_MASK	(AU_BIT6 | AU_BIT7)
#define CAP_CHANNEL_2		0
#define CAP_CHANNEL_8		AU_BIT6
#define CAP_CHANNEL_12		AU_BIT7

#define CAP_FORMAT_MASK		(AU_BIT4 | AU_BIT5)
#define CAP_FORMAT_16BIT	0
#define CAP_FORMAT_24BIT	AU_BIT4

#define	BYTES_IN_MIU_LINE	16
#define	BYTES_IN_MIU_LINE_LOG2	4
#define	ICACHE_MEMOFFSET_LOG2	12

#define BITS_IN_BYTE		8

#define AUDIO_DO_ALIGNMENT(val, alignment_size)	(val = (val / alignment_size) * alignment_size)

enum {
	CH_MONO = 1,
	CH_STEREO = 2,
};

enum {
	CMD_STOP = 0,
	CMD_START,
	CMD_PAUSE,
	CMD_PAUSE_RELEASE,
	CMD_PREPARE,
	CMD_SUSPEND,
	CMD_RESUME,
};

/* Define a STR (Suspend To Ram) data structure */
struct mtk_str_mode_struct {
	ptrdiff_t physical_addr;
	ptrdiff_t bus_addr;
	ptrdiff_t virtual_addr;
};

/* Define a Monitor data structure */
struct mtk_monitor_struct {
	unsigned int monitor_status;
	unsigned int expiration_counter;
	snd_pcm_uframes_t last_appl_ptr;
	snd_pcm_uframes_t last_hw_ptr;
};

/* Define a Substream data structure */
struct mtk_substream_struct {
	struct snd_pcm_substream *p_substream;
	struct snd_pcm_substream *c_substream;
	unsigned int substream_status;
};

struct mem_info {
	u32 miu;
	u64 bus_addr;
	u64 buffer_size;
	u64 memory_bus_base;
};

typedef enum {
	AUD_EMERG,
	AUD_ALERT,
	AUD_CRIT,
	AUD_ERR,
	AUD_WARN,
	AUD_NOT,
	AUD_INFO,
	AUD_DEB,
} AUDIO_DEBUG_LEVEL;

#endif /* _ALSA_PLATFORM_HEADER */
