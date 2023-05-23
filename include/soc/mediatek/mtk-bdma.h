/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_BDMA_H__
#define __MTK_BDMA_H__


#include <linux/completion.h>
#include <linux/mutex.h>

#define _LSHIFT(v, n)				((v) << (n))
#define _RSHIFT(v, n)				((v) >> (n))
#define BDMA_SET_CPYTYPE(src, dst)	((src & 0xFF) | _LSHIFT((dst & 0xFF), 8))
#define MAX_CHANNEL_LABLE			16
#define CONID_STRING_SIZE			32
enum bdma_path {
	E_BDMA_DEV_MIU0,
	E_BDMA_DEV_MIU1,
	E_BDMA_DEV_SEARCH,
	E_BDMA_DEV_CRC32,
	E_BDMA_DEV_MEM_FILL,
	E_BDMA_DEV_FLASH,
	E_BDMA_DEV_VDMCU,
	E_BDMA_DEV_DMDMCU,
	E_BDMA_DEV_FCIC,
	E_BDMA_DEV_SUP9,
	E_BDMA_DEV_1KSRAM_HK51,
	E_BDMA_DEV_SUP11,
	E_BDMA_DEV_SUP12,
	E_BDMA_DEV_MSPI2,
	E_BDMA_DEV_CILINK,
	E_BDMA_DEV_PCLRC,
	/* add new here */
	E_BDMA_DEV_NOT_SUPPORT,
	/* Source Device */
	E_BDMA_SRCDEV_MIU0			= E_BDMA_DEV_MIU0,
	E_BDMA_SRCDEV_MIU1			= E_BDMA_DEV_MIU1,
	E_BDMA_SRCDEV_MEM_FILL		= E_BDMA_DEV_MEM_FILL,
	E_BDMA_SRCDEV_FLASH			= E_BDMA_DEV_FLASH,
	E_BDMA_SRCDEV_HKMCU			= E_BDMA_DEV_1KSRAM_HK51,
	E_BDMA_SRCDEV_MSPI_CI		= E_BDMA_DEV_CILINK,
	E_BDMA_SRCDEV_PCLRC			= E_BDMA_DEV_PCLRC,
	/* add new here */
	E_BDMA_SRCDEV_NOT_SUPPORT	= E_BDMA_DEV_NOT_SUPPORT,
	/* Destination Device */
	E_BDMA_DSTDEV_MIU0			= E_BDMA_DEV_MIU0,
	E_BDMA_DSTDEV_MIU1			= E_BDMA_DEV_MIU1,
	E_BDMA_DSTDEV_SEARCH		= E_BDMA_DEV_SEARCH,
	E_BDMA_DSTDEV_CRC32			= E_BDMA_DEV_CRC32,
	E_BDMA_DSTDEV_DMDMCU		= E_BDMA_DEV_DMDMCU,
	E_BDMA_DSTDEV_VDMCU			= E_BDMA_DEV_VDMCU,
	E_BDMA_DSTDEV_FCIC			= E_BDMA_DEV_FCIC,
	E_BDMA_DSTDEV_HK51_1KSRAM	= E_BDMA_DEV_1KSRAM_HK51,
	E_BDMA_DSTDEV_CI			= E_BDMA_DEV_CILINK,
	E_BDMA_DSTDEV_PCLRC			= E_BDMA_DEV_PCLRC,
	/* add new here */
	E_BDMA_DSTDEV_NOT_SUPPORT	= E_BDMA_DEV_NOT_SUPPORT,
};

enum _bdma_cpytype {
	E_BDMA_SDRAM2SDRAM	= BDMA_SET_CPYTYPE(E_BDMA_SRCDEV_MIU0, E_BDMA_DSTDEV_MIU0),
	E_BDMA_SDRAM2SERCH	= BDMA_SET_CPYTYPE(E_BDMA_SRCDEV_MIU0, E_BDMA_DSTDEV_SEARCH),
	E_BDMA_MEMFILL2SDRAM	=
		BDMA_SET_CPYTYPE(E_BDMA_SRCDEV_MEM_FILL, E_BDMA_DSTDEV_MIU0),
	E_BDMA_SDRAM2MSPI	= BDMA_SET_CPYTYPE(E_BDMA_SRCDEV_MIU0, E_BDMA_DEV_MSPI2),
	E_BDMA_MSPI2SDRAM	= BDMA_SET_CPYTYPE(E_BDMA_SRCDEV_MSPI_CI, E_BDMA_DSTDEV_MIU0),
	E_BDMA_DRAM2PCLRC	= BDMA_SET_CPYTYPE(E_BDMA_SRCDEV_MIU0, E_BDMA_DSTDEV_PCLRC),
	E_BDMA_PCLRC2DRAM	= BDMA_SET_CPYTYPE(E_BDMA_SRCDEV_PCLRC, E_BDMA_DSTDEV_MIU0),
	E_BDMA_CPYTYPE_MAX	=
		BDMA_SET_CPYTYPE(E_BDMA_SRCDEV_NOT_SUPPORT, E_BDMA_DEV_NOT_SUPPORT)
};

struct mtk_bdma_dat {
	uint32_t searchoffset;
	uint64_t srcaddr;
	uint64_t dstaddr;
	uint32_t len;
	uint32_t pattern;
	uint32_t exclubit;
	uint16_t bdmapath;
};

struct mtk_bdma {
	struct device *dev;
	void __iomem *base;
	void __iomem *ckgen;
	char channel[CONID_STRING_SIZE];
	int irq;
	struct list_head list;
	struct completion done;
	struct mtk_bdma_dat data;
	bool addr_match;
	struct mutex bus_mutex;
};

struct mtk_bdma_channel {
	struct mtk_bdma *bdma;
	char label[MAX_CHANNEL_LABLE];// consumer label
	//struct bdma_statistics statistics;
	struct list_head link;
};

int mtk_bdma_transact(struct mtk_bdma_channel *ch, struct mtk_bdma_dat *data);
struct mtk_bdma_channel *mtk_bdma_request_channel
		(const char *channel_name, const char *label);

void mtk_bdma_release_channel(struct mtk_bdma_channel *ch);

static inline int mtk_bdma_dram2pclrc(struct mtk_bdma_channel *ch,
			uint64_t srcaddr,
			uint64_t dstaddr,
			uint32_t len)
{
	struct mtk_bdma_dat data;
	int ret;

	memset(&data, 0, sizeof(struct mtk_bdma_dat));
	data.bdmapath = E_BDMA_DRAM2PCLRC;
	data.srcaddr = srcaddr;
	data.dstaddr = dstaddr;
	data.len = len;
	ret = mtk_bdma_transact(ch, &data);
	return ret;
}

static inline int mtk_bdma_dram_pattern_fill(struct mtk_bdma_channel *ch,
			uint64_t addr,
			uint32_t len,
			uint32_t pattern)
{
	struct mtk_bdma_dat data;
	int ret;

	memset(&data, 0, sizeof(struct mtk_bdma_dat));
	data.bdmapath = E_BDMA_MEMFILL2SDRAM;
	data.dstaddr = addr;
	data.len = len;
	data.pattern = pattern;
	ret = mtk_bdma_transact(ch, &data);
	return ret;
}

static inline int mtk_bdma_dram_copy(struct mtk_bdma_channel *ch,
			uint64_t srcaddr,
			uint64_t dstaddr,
			uint32_t len)
{
	struct mtk_bdma_dat data;
	int ret;

	memset(&data, 0, sizeof(struct mtk_bdma_dat));
	data.bdmapath = E_BDMA_SDRAM2SDRAM;
	data.srcaddr = srcaddr;
	data.dstaddr = dstaddr;
	data.len = len;
	ret = mtk_bdma_transact(ch, &data);
	return ret;
}

static inline int mtk_bdma_dram_pattern_search(struct mtk_bdma_channel *ch,
			uint32_t *searchaddr,
			uint64_t addr,
			uint32_t len,
			uint32_t pattern,
			uint32_t exclubit)
{
	struct mtk_bdma_dat data;
	int ret;

	memset(&data, 0, sizeof(struct mtk_bdma_dat));
	data.bdmapath = E_BDMA_SDRAM2SERCH;
	data.srcaddr = addr;
	data.pattern = pattern;
	data.len = len;
	data.exclubit = exclubit;
	ret = mtk_bdma_transact(ch, &data);
	*searchaddr = data.searchoffset;
	return ret;
}

static inline int mtk_mspi_bdma_write_dev(struct mtk_bdma_channel *ch,
			uint64_t srcaddr,
			uint64_t dstaddr,
			uint32_t len)
{
	struct mtk_bdma_dat data;
	int ret;

	memset(&data, 0, sizeof(struct mtk_bdma_dat));
	data.bdmapath = E_BDMA_SDRAM2MSPI;
	/*DRAM SRC Address*/
	data.srcaddr = srcaddr;
	/*SPIDEV DST Address*/
	data.dstaddr = dstaddr;
	data.len = len;
	ret = mtk_bdma_transact(ch, &data);
	return ret;
}

static inline int mtk_mspi_bdma_read_dev(struct mtk_bdma_channel *ch,
			uint64_t srcaddr,
			uint64_t dstaddr,
			uint32_t len)
{
	struct mtk_bdma_dat data;
	int ret;

	memset(&data, 0, sizeof(struct mtk_bdma_dat));
	data.bdmapath = E_BDMA_MSPI2SDRAM;
	/*SPIDEV SRC Address*/
	data.srcaddr = srcaddr;
	/*DRAM DST Address*/
	data.dstaddr = dstaddr;
	data.len = len;
	ret = mtk_bdma_transact(ch, &data);
	return ret;
}

#endif
