/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_MIUP_H
#define __MTK_MIUP_H

/*********************************************************************
 *                         Private Define                            *
 *********************************************************************/
#define MAX_REG_SIZE	(4)
#define SHIFT_16BIT	(16)
#define SHIFT_32BIT	(32)
#define LOW_16BIT	(0xFFFF)
#define TRUNCATE_33BIT	(0x1FFFFFFFF)
#define START_ADDR_H	(0)
#define START_ADDR_L	(1)
#define LEN_ADDR_H	(2)
#define LEN_ADDR_L	(3)

#define PROT_CID_MAX_INDEX 0xFFFFFFFF
#define PROT_ADDR_ALIGN 0x1000
#define PROT_NULL_CID 0x0
#define MIUP_LOG_NONE 0
#define MIUP_LOG_ERR 1
#define MIUP_LOG_WARN 2
#define MIUP_LOG_INFO 3
#define MIUP_LOG_ALL 4
#define MIUP_LOG_DEFAULT MIUP_LOG_ERR
#define PROT_BA_BASE 0x20000000
#define PROT_BLK7 (7)

#define SLPPROT_MAX_CONTENT	(9)
#define SLPPROT_MT5896_GALS	(1)
#define SLPPROT_MT5897_GALS	(1)
#define SLPPROT_MT5876_GALS	(1)
#define SLPPROT_MT5879_GALS	(1)
#define SMI_CTRL_BANK		(0x1c33e000)
#define SMI_CTRL_REG_SIZE	(0x100)
#define GALS_SLPPROT_CTRL_OFFSET	(0)
#define GALS_MST_RST			(1)
#define GALS_SLPPROT_MST_EN		(2)
#define GALS_SLV_RST			(3)
#define GALS_SLPPROT_SLV_EN		(4)
#define GALS_SLPPROT_RDY_OFFSET		(5)
#define GALS_SLPPROT_MST_RDY		(6)
#define GALS_SLPPROT_SLV_RDY		(7)
#define GALS_SLPPROT_ENABLE		(8)
#define GALS_RDY_TIMEOUT		(65000)

#define miup_setbit(offset, flag, reg)					\
({									\
	typeof(offset) _offset = (offset);				\
	typeof(flag) _flag = (flag);					\
	typeof(reg) _reg = (reg);					\
	(_flag) ?							\
	writew_relaxed((readw_relaxed(_reg) | (1 << _offset)), _reg) :	\
	writew_relaxed((readw_relaxed(_reg) & ~(1 << _offset)), _reg);	\
})									\

#define MIUP_PRE(fmt, args...) \
	pr_err("[%s][%d]" fmt "\n", __func__, __LINE__, ##args)
#define MIUP_PRW(fmt, args...) \
	pr_warn("[%s][%d]" fmt "\n", __func__, __LINE__, ##args)
#define MIUP_PRI(fmt, args...) \
	pr_info("[%s][%d]" fmt "\n", __func__, __LINE__, ##args)

#define MIUP_DPRE(logl, dev, fmt, args...) do { \
	if (logl >= MIUP_LOG_ERR) \
		dev_err(dev, "[%s][%d]" fmt "\n", __func__, __LINE__, ##args); \
} while (0)
#define MIUP_DPRW(logl, dev, fmt, args...) do { \
	if (logl >= MIUP_LOG_WARN) \
		dev_err(dev, "[%s][%d]" fmt "\n", __func__, __LINE__, ##args); \
} while (0)
#define MIUP_DPRI(logl, dev, fmt, args...) do { \
	if (logl >= MIUP_LOG_INFO) \
		dev_err(dev, "[%s][%d]" fmt "\n", __func__, __LINE__, ##args); \
} while (0)

#define align_st(addr) (((addr + (PROT_ADDR_ALIGN - 1))/PROT_ADDR_ALIGN) * PROT_ADDR_ALIGN)
#define align_ed(addr) ((((addr + 1) / PROT_ADDR_ALIGN) * PROT_ADDR_ALIGN) - 1)

/*********************************************************************
 *                         Private Structure                         *
 *********************************************************************/
struct mtk_miup {
	struct device		*dev;
	void __iomem		*base;
	void __iomem		*hit_log0_base;
	void __iomem		*hit_log1_base;
	void __iomem		*hit_log2_base;
	int			irq;
	u32			max_prot_blk;
	u32			max_cid_group;
	u32			max_cid_number;
	bool			hit_panic;
	u32			kprot_num;
	struct mtk_miup_para	*kprot;
	u32			kcid_num;
	u64			*kcid;
	u32			cpu_cid_num;
	u64			*cpu_cid;
	u16			miup_all[128];
	u16			loglevel;
	u32			end_addr_ver;
	u32			chipid;
};

struct mtk_nullmem {
	u64	start_addr;
	u64	len;
};

#define REG_HIT_LAST_LOG_OFFSET		BIT(3)
#define REG_PROT_W_EN			0x0000UL /* 0x0000UL */
#define REG_PROT_R_EN			0x0004UL /* 0x0001UL */
#define REG_PROT_INV			0x0008UL /* 0x0002UL */
#define REG_PROT_ID_GP_SEL		0x0018UL /* 0x0006UL */
#define REG_PROT_HIT_FLAG		0x0020UL /* 0x0008UL */
#define REG_PROT_W_HIT_MASK		0x0024UL /* 0x0009UL */
#define REG_PROT_R_HIT_MASK		0x0028UL /* 0x000AUL */
#define REG_PROT_HIT_UNMASK		0x002CUL /* 0x000BUL */
#define REG_PROT_OUT_OF_AREA_START_L	0x0030UL /* 0x000CUL */
#define REG_PROT_OUT_OF_AREA_START_H	0x0034UL /* 0x000DUL */
#define REG_PROT_OUT_OF_AREA_END_L	0x0038UL /* 0x000EUL */
#define REG_PROT_OUT_OF_AREA_END_H	0x003CUL /* 0x000FUL */
#define REG_PROT_0_START_L		0x0040UL /* 0x0010UL */
#define REG_PROT_0_START_H		0x0044UL /* 0x0011UL */
#define REG_PROT_0_END_L		0x0048UL /* 0x0012UL */
#define REG_PROT_0_END_H		0x004CUL /* 0x0013UL */
#define REG_PROT_0_ID_EN		0x00C0UL /* 0x0030UL */
#define REG_PROT_OUT_OF_AREA_ID_EN	0x00E0UL /* 0x0038UL */
#define REG_PROT_DMA_EN			0x00E4UL /* 0x0039UL */
#define REG_PROT_DMA_ADDR_START_L	0x00E8UL /* 0x003AUL */
#define REG_PROT_CHECK_ID_SELECT	0x00E8UL /* 0x003AUL */
#define REG_PROT_DMA_ADDR_START_H	0x00ECUL /* 0x003BUL */
#define REG_PROT_DMA_ADDR_END_L		0x00F0UL /* 0x003CUL */
#define REG_PROT_DMA_ADDR_END_H		0x00F4UL /* 0x003DUL */
#define REG_PROT_DMA_ID_1_0		0x00F8UL /* 0x003EUL */
#define REG_PROT_DMA_ID_3_2		0x00FCUL /* 0x003FUL */
#define REG_PROT_ID_0			0x0100UL /* 0x0040UL */
#define REG_PROT_G1_ID_0		0x0140UL /* 0x0050UL */
#define REG_PROT_ID_0_MASK		0x0180UL /* 0x0060UL */
#define REG_PROT_G1_ID_0_MASK		0x01C0UL /* 0x0070UL */

#define REG_PROT_W_HIT_ADDR_L		0x0000UL /* 0x0000UL */
#define REG_PROT_W_HIT_ADDR_H		0x0004UL /* 0x0001UL */
#define REG_PROT_W_HIT_LOG		0x0008UL /* 0x0002UL */
#define REG_PROT_W_HIT_ID		0x000CUL /* 0x0003UL */
#define REG_PROT_R_HIT_ADDR_L		0x0020UL /* 0x0008UL */
#define REG_PROT_R_HIT_ADDR_H		0x0024UL /* 0x0009UL */
#define REG_PROT_R_HIT_LOG		0x0028UL /* 0x000AUL */
#define REG_PROT_R_HIT_ID		0x002CUL /* 0x000BUL */

#define PROT_SADDR_L_OFFSET(blk)	((REG_PROT_0_START_L) + ((blk) * 0x10))
#define PROT_SADDR_H_OFFSET(blk)	(PROT_SADDR_L_OFFSET(blk) + 0x4)
#define PROT_EADDR_L_OFFSET(blk)	((REG_PROT_0_END_L) + ((blk) * 0x10))
#define PROT_EADDR_H_OFFSET(blk)	(PROT_EADDR_L_OFFSET(blk) + 0x4)
#define PROT_ID_EN_OFFSET(blk)		((REG_PROT_0_ID_EN) + ((blk) * 0x4))
#define PROT_ID_OFFSET(gp, idx) \
	((REG_PROT_ID_0) + (idx) * 0x4 + (gp) * 0x40)
#define PROT_ID_MASK_OFFSET(gp, idx) \
	((REG_PROT_ID_0_MASK) + (idx) * 0x4 + (gp) * 0x40)

#define CHECK_ID_OFFSET			(0)
#define CHECK_AID			(0)
#define CHECK_AXID			(1)
#define CLIENT_ID_USE_ARRAY_CNT		(2)

#define END_ADDR_VER_1			(1)
#define END_ADDR_VER_2			(2)

#define BLK_LEN				(4)

enum {
	BLK_STYPE_IDX = 2,  /* block index of stype */
	BLK_ETYPE_IDX = 3,  /* block index of etype */
};

enum {
	CHIP_MT5896 = 1,
	CHIP_MT5897 = 2,
	CHIP_MT5876 = 3,
	CHIP_MT5879 = 4,
};

#endif /* __MTK_MIUP_H */
