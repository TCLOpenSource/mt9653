// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Bayi Cheng <bayi.cheng@mediatek.com>
 */


#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/spi-nor.h>

#define FLASH_XFER_CMD_SIZE	4
#define PAGE_WRITE_SIZE	256
#define FIFO_MAX_SIZE	32
#define FIFO_LOOP_SIZE	(PAGE_WRITE_SIZE/FIFO_MAX_SIZE)
#define NOR_CLK	24000000
#define MSPI_CH		1


struct spi_device *km_spidev_node_get(char channel);

struct mtk_nor {
	struct spi_nor nor;
	struct device *dev;
	void __iomem *base;	/* nor flash base address */
	int clk;
	int channel;
	int type;
};

#define TRUE 1
#define FALSE 0

#define MS_BOOL bool
#define MS_U32 u32
#define MS_U16 u16
#define MS_U8 u8

#define TIME_MS							300
#define SERFLASH_SAFETY_FACTOR			1000000
#define SER_FLASH_TIME(_stamp, _msec)	(_stamp =_msec)
#define SER_FLASH_EXPIRE(_stamp)		( !(--_stamp))

#define BITS(_bits_, _val_)			((BIT(((1)?_bits_)+1)-BIT(((0)?_bits_))) & (_val_<<((0)?_bits_)))
#define BMASK(_bits_)				(BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))

// FSP Register
#define REG_FSP_RCV				BITS(12:11,3)
#define REG_FSP_WBF_SIZE_OUTSIDE	0x78
#define REG_FSP_WBF_REPLACED		0x79
#define REG_FSP_WDB0	0x60
#define REG_FSP_WDB0_MASK		BMASK(7:0)
#define REG_FSP_WDB0_DATA(d)	BITS(7:0, d)
#define REG_FSP_WDB1	0x60
#define REG_FSP_WDB1_MASK		BMASK(15:8)
#define REG_FSP_WDB1_DATA(d)	BITS(15:8, d)
#define REG_FSP_WDB2	0x61
#define REG_FSP_WDB2_MASK		BMASK(7:0)
#define REG_FSP_WDB2_DATA(d)	BITS(7:0, d)
#define REG_FSP_WDB3	0x61
#define REG_FSP_WDB3_MASK		BMASK(15:8)
#define REG_FSP_WDB3_DATA(d)	BITS(15:8, d)
#define REG_FSP_WDB4	0x62
#define REG_FSP_WDB4_MASK		BMASK(7:0)
#define REG_FSP_WDB4_DATA(d)	BITS(7:0, d)
#define REG_FSP_WDB5	0x62
#define REG_FSP_WDB5_MASK		BMASK(15:8)
#define REG_FSP_WDB5_DATA(d)	BITS(15:8, d)
#define REG_FSP_WDB6	0x63
#define REG_FSP_WDB6_MASK		BMASK(7:0)
#define REG_FSP_WDB6_DATA(d)	BITS(7:0, d)
#define REG_FSP_WDB7	0x63
#define REG_FSP_WDB7_MASK		BMASK(15:8)
#define REG_FSP_WDB7_DATA(d)	BITS(15:8, d)
#define REG_FSP_WDB8	0x64
#define REG_FSP_WDB8_MASK		BMASK(7:0)
#define REG_FSP_WDB8_DATA(d)	BITS(7:0, d)
#define REG_FSP_WDB9	0x64
#define REG_FSP_WDB9_MASK		BMASK(15:8)
#define REG_FSP_WDB9_DATA(d)	BITS(15:8, d)
#define REG_FSP_RDB0	0x65
#define REG_FSP_RDB1	0x65
#define REG_FSP_RDB2	0x66
#define REG_FSP_RDB3	0x66
#define REG_FSP_RDB4	0x67
#define REG_FSP_RDB5	0x67
#define REG_FSP_RDB6	0x68
#define REG_FSP_RDB7	0x68
#define REG_FSP_RDB8	0x69
#define REG_FSP_RDB9	0x69
#define REG_FSP_WBF_SIZE	0x6a
#define REG_FSP_WBF_SIZE0_MASK	 BMASK(3:0)
#define REG_FSP_WBF_SIZE0(s)		BITS(3:0,s)
#define REG_FSP_WBF_SIZE1_MASK	 BMASK(7:4)
#define REG_FSP_WBF_SIZE1(s)		BITS(7:4,s)
#define REG_FSP_WBF_SIZE2_MASK	 BMASK(11:8)
#define REG_FSP_WBF_SIZE2(s)		BITS(11:8,s)
#define REG_FSP_RBF_SIZE	0x6b
#define REG_FSP_RBF_SIZE0_MASK	 BMASK(3:0)
#define REG_FSP_RBF_SIZE0(s)		BITS(3:0,s)
#define REG_FSP_RBF_SIZE1_MASK	 BMASK(7:4)
#define REG_FSP_RBF_SIZE1(s)		BITS(7:4,s)
#define REG_FSP_RBF_SIZE2_MASK	 BMASK(11:8)
#define REG_FSP_RBF_SIZE2(s)		BITS(11:8,s)
#define REG_FSP_CTRL		0x6c
#define REG_FSP_ENABLE_MASK		BMASK(0:0)
#define REG_FSP_ENABLE			 BITS(0:0,1)
#define REG_FSP_DISABLE			BITS(0:0,0)
#define REG_FSP_RESET_MASK		 BMASK(1:1)
#define REG_FSP_RESET				BITS(1:1,0)
#define REG_FSP_NRESET			 BITS(1:1,1)
#define REG_FSP_INT_MASK			BMASK(2:2)
#define REG_FSP_INT				BITS(2:2,1)
#define REG_FSP_INT_OFF			BITS(2:2,0)
#define REG_FSP_CHK_MASK			BMASK(3:3)
#define REG_FSP_CHK				BITS(3:3,1)
#define REG_FSP_CHK_OFF			BITS(3:3,0)
#define REG_FSP_RDSR_MASK			BMASK(12:11)
#define REG_FSP_1STCMD			 BITS(12:11,0)
#define REG_FSP_2NDCMD			 BITS(12:11,1)
#define REG_FSP_3THCMD			 BITS(12:11,2)
#define REG_FSP_FSCHK_MASK		 BMASK(13:13)
#define REG_FSP_FSCHK_ON			BITS(13:13,1)
#define REG_FSP_FSCHK_OFF			BITS(13:13,0)
#define REG_FSP_3THCMD_MASK		BMASK(14:14)
#define REG_FSP_3THCMD_ON			BITS(14:14,1)
#define REG_FSP_3THCMD_OFF		 BITS(14:14,0)
#define REG_FSP_2NDCMD_MASK		BMASK(15:15)
#define REG_FSP_2NDCMD_ON			BITS(15:15,1)
#define REG_FSP_2NDCMD_OFF		 BITS(15:15,0)
#define REG_FSP_TRIGGER		0x6d
#define REG_FSP_TRIGGER_MASK		BMASK(0:0)
#define REG_FSP_FIRE			 BITS(0:0,1)
#define REG_FSP_DONE_FLAG	0x6e
#define REG_FSP_DONE_FLAG_MASK		BMASK(0:0)
#define REG_FSP_DONE			 BITS(0:0,1)
#define REG_FSP_DONE_CLR	0x6f
#define REG_FSP_DONE_CLR_MASK		BMASK(0:0)
#define REG_FSP_CLR			 BITS(0:0,1)

#define REG_ISP_SPI_MODE			0x72

// please refer to the serial flash datasheet
#define SPI_CMD_READ				(0x03)
#define SPI_CMD_FASTREAD			(0x0B)
#define SPI_CMD_RDID				(0x9F)
#define SPI_CMD_WREN				(0x06)
#define SPI_CMD_WRDI				(0x04)
#define SPI_CMD_SE					(0x20)
#define SPI_CMD_32BE				(0x52)
#define SPI_CMD_64BE				(0xD8)
#define SPI_CMD_CE					(0xC7)
#define SPI_CMD_PP					(0x02)
#define SPI_CMD_RDSR				(0x05)
#define SPI_CMD_RDSR2				(0x35) // support for new WinBond Flash#define SPI_CMD_WRSR				(0x01)
#define FLASH_OIP					0x01

#define QUAD_EN						0x40
#define QUAD_DIS					0xBF

#define REG_FSP_WRITEDATA_SIZE		0x4
#define REG_FSP_MAX_WRITEDATA_SIZE	0xA
#define REG_FSP_READDATA_SIZE		0xA

enum {
	FLASH_ERASE_04K = SPI_CMD_SE,
	FLASH_ERASE_32K = SPI_CMD_32BE,
	FLASH_ERASE_64K = SPI_CMD_64BE
} EN_FLASH_ERASE;

#define FSP_READ(nor,addr)					READ_WORD(nor->base + ((addr)<<2))
#define FSP_WRITE(nor,addr, val)			WRITE_WORD(nor->base + ((addr)<<2), (val))
#define FSP_WRITE_MASK(nor,addr, val, mask)	WRITE_WORD_MASK(nor->base + ((addr)<<2), (val), (mask))

static inline u16 READ_WORD(void __iomem *reg)
{
	return readw_relaxed(reg);
}

static inline void WRITE_WORD(void __iomem *reg, u16 val)
{
	writew_relaxed(val, reg);
}

static inline void WRITE_WORD_MASK(void __iomem *reg, u16 val, u16 mask)
{
	u16 org = READ_WORD(reg);

	WRITE_WORD(reg, ((org) & ~(mask)) | ((val) & (mask)));
}

static MS_BOOL _HAL_FSP_WaitDone(struct mtk_nor *mtk_nor)
{
	MS_BOOL bRet = FALSE;
	MS_U32 u32Timer;

	SER_FLASH_TIME(u32Timer, SERFLASH_SAFETY_FACTOR * 30);
	do{
		if ( (FSP_READ(mtk_nor,REG_FSP_DONE_FLAG) & REG_FSP_DONE_FLAG_MASK) == REG_FSP_DONE ) {
			bRet = TRUE;
			break;
		}
	} while (!SER_FLASH_EXPIRE(u32Timer));

	FSP_WRITE_MASK(mtk_nor,REG_FSP_DONE_CLR,REG_FSP_CLR,REG_FSP_DONE_CLR_MASK);
	return bRet;
}

static MS_U8 _HAL_FSP_ReadBuf0(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB0) & 0x00FF) >> 0);
}

static MS_U8 _HAL_FSP_ReadBuf1(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB1) & 0xFF00) >> 8);
}

static MS_U8 _HAL_FSP_ReadBuf2(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB2) & 0x00FF) >> 0);
}

static MS_U8 _HAL_FSP_ReadBuf3(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB3) & 0xFF00) >> 8);
}

static MS_U8 _HAL_FSP_ReadBuf4(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB4) & 0x00FF) >> 0);
}

static MS_U8 _HAL_FSP_ReadBuf5(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB5) & 0xFF00) >> 8);
}

static MS_U8 _HAL_FSP_ReadBuf6(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB6) & 0x00FF) >> 0);
}

static MS_U8 _HAL_FSP_ReadBuf7(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB7) & 0xFF00) >> 8);
}

static MS_U8 _HAL_FSP_ReadBuf8(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB8) & 0x00FF) >> 0);
}

static MS_U8 _HAL_FSP_ReadBuf9(struct mtk_nor *mtk_nor)
{
	return (MS_U8)((FSP_READ(mtk_nor,REG_FSP_RDB9) & 0xFF00) >> 8);
}

static MS_U8 HAL_FSP_ReadBufs(struct mtk_nor *mtk_nor,MS_U8 u8Idx)
{
	MS_U8 u8Data;

	switch (u8Idx) {
	case 0:
		 u8Data = _HAL_FSP_ReadBuf0(mtk_nor);
		break;
	case 1:
		 u8Data = _HAL_FSP_ReadBuf1(mtk_nor);
		break;
	case 2:
		 u8Data = _HAL_FSP_ReadBuf2(mtk_nor);
		break;
	case 3:
		 u8Data = _HAL_FSP_ReadBuf3(mtk_nor);
		break;
	case 4:
		 u8Data = _HAL_FSP_ReadBuf4(mtk_nor);
		break;
	case 5:
		 u8Data = _HAL_FSP_ReadBuf5(mtk_nor);
		break;
	case 6:
		 u8Data = _HAL_FSP_ReadBuf6(mtk_nor);
		break;
	case 7:
		u8Data = _HAL_FSP_ReadBuf7(mtk_nor);
		break;
	case 8:
		u8Data = _HAL_FSP_ReadBuf8(mtk_nor);
		break;
	case 9:
		u8Data = _HAL_FSP_ReadBuf9(mtk_nor);
		break;
	default:
		u8Data=0xFF;
		dev_err(mtk_nor->dev, "Buffer Index not Supported\n");
		break;
	}
	return u8Data;
}

static void _HAL_FSP_WriteBuf0(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB0,REG_FSP_WDB0_DATA(u8Data),REG_FSP_WDB0_MASK);
}

static void _HAL_FSP_WriteBuf1(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB1,REG_FSP_WDB1_DATA(u8Data),REG_FSP_WDB1_MASK);
}

static void _HAL_FSP_WriteBuf2(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB2,REG_FSP_WDB2_DATA(u8Data),REG_FSP_WDB2_MASK);
}

static void _HAL_FSP_WriteBuf3(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB3,REG_FSP_WDB3_DATA(u8Data),REG_FSP_WDB3_MASK);
}

static void _HAL_FSP_WriteBuf4(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB4,REG_FSP_WDB4_DATA(u8Data),REG_FSP_WDB4_MASK);
}

static void _HAL_FSP_WriteBuf5(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB5,REG_FSP_WDB5_DATA(u8Data),REG_FSP_WDB5_MASK);
}

static void _HAL_FSP_WriteBuf6(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB6,REG_FSP_WDB6_DATA(u8Data),REG_FSP_WDB6_MASK);
}

static void _HAL_FSP_WriteBuf7(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB7,REG_FSP_WDB7_DATA(u8Data),REG_FSP_WDB7_MASK);
}

static void _HAL_FSP_WriteBuf8(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB8,REG_FSP_WDB8_DATA(u8Data),REG_FSP_WDB8_MASK);
}

static void _HAL_FSP_WriteBuf9(struct mtk_nor *mtk_nor,MS_U8 u8Data)
{
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WDB9,REG_FSP_WDB9_DATA(u8Data),REG_FSP_WDB9_MASK);
}

static void HAL_FSP_WriteBufs(struct mtk_nor *mtk_nor,MS_U8 u8Idx, MS_U8 u8Data)
{
	switch(u8Idx) {
	case 0:
		_HAL_FSP_WriteBuf0(mtk_nor,u8Data);
		break;
	case 1:
		_HAL_FSP_WriteBuf1(mtk_nor,u8Data);
		break;
	case 2:
		_HAL_FSP_WriteBuf2(mtk_nor,u8Data);
		break;
	case 3:
		_HAL_FSP_WriteBuf3(mtk_nor,u8Data);
		break;
	case 4:
		_HAL_FSP_WriteBuf4(mtk_nor,u8Data);
		break;
	case 5:
		_HAL_FSP_WriteBuf5(mtk_nor,u8Data);
		break;
	case 6:
		_HAL_FSP_WriteBuf6(mtk_nor,u8Data);
		break;
	case 7:
		_HAL_FSP_WriteBuf7(mtk_nor,u8Data);
		break;
	case 8:
		_HAL_FSP_WriteBuf8(mtk_nor,u8Data);
		break;
	case 9:
		_HAL_FSP_WriteBuf9(mtk_nor,u8Data);
		break;
	default:
		dev_err(mtk_nor->dev, "%s fails\n", __func__);
		break;
	}
}

static void HAL_FSP_Entry(struct mtk_nor *mtk_nor)
{
	FSP_WRITE(mtk_nor,REG_ISP_SPI_MODE, 0x0000); // need check
}

static void HAL_FSP_Exit(struct mtk_nor *mtk_nor)
{
}

static MS_U8 HAL_FSP_CheckFlashBusy(struct mtk_nor *mtk_nor, MS_U32 u32Delay)
{
	MS_U8	u8Status;
	MS_U32 u32Timer;
	MS_BOOL bRet = TRUE;

	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_ENABLE,REG_FSP_ENABLE_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_NRESET,REG_FSP_RESET_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_2NDCMD_OFF,REG_FSP_2NDCMD_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_3THCMD_OFF,REG_FSP_3THCMD_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_RCV,REG_FSP_RDSR_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_FSCHK_OFF,REG_FSP_FSCHK_MASK);
	SER_FLASH_TIME(u32Timer, SERFLASH_SAFETY_FACTOR * TIME_MS);
	do {
		HAL_FSP_WriteBufs(mtk_nor,0,SPI_CMD_RDSR);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE0(1),REG_FSP_WBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE0(1),REG_FSP_RBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_TRIGGER,REG_FSP_FIRE,REG_FSP_TRIGGER_MASK);
		bRet &= _HAL_FSP_WaitDone(mtk_nor);
		u8Status = HAL_FSP_ReadBufs(mtk_nor,0);
		u8Status &= FLASH_OIP ;

		if(!u8Status)
			break;

		udelay(u32Delay);
	} while(!SER_FLASH_EXPIRE(u32Timer));

	return u8Status;
}

static MS_BOOL HAL_FSP_BlockErase(struct mtk_nor *mtk_nor,MS_U32 u32StartBlock, MS_U32 u32Size)
{
	MS_BOOL bRet = TRUE;
	MS_U32 u32FlashAddr=0;
	MS_U8	u8Status=TRUE;
	MS_U8  eSize;

	switch(u32Size)	{
	case 4*1024:
		eSize = FLASH_ERASE_04K;
		break;
	case 32*1024:
		eSize = FLASH_ERASE_32K;
		break;
	//case 64*1024:
	default:
		eSize = FLASH_ERASE_64K;
		break;
	}

	FSP_WRITE(mtk_nor,REG_FSP_WBF_SIZE, 0x0);
	FSP_WRITE(mtk_nor,REG_FSP_RBF_SIZE, 0x0);
	u32FlashAddr = u32StartBlock*u32Size;
	HAL_FSP_WriteBufs(mtk_nor,0,SPI_CMD_WREN);
	HAL_FSP_WriteBufs(mtk_nor,1,eSize);
	HAL_FSP_WriteBufs(mtk_nor,2,(MS_U8)((u32FlashAddr>>0x10)&0xFF));
	HAL_FSP_WriteBufs(mtk_nor,3,(MS_U8)((u32FlashAddr>>0x08)&0xFF));
	HAL_FSP_WriteBufs(mtk_nor,4,(MS_U8)((u32FlashAddr>>0x00)&0xFF));
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE0(1),REG_FSP_WBF_SIZE0_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE1(4),REG_FSP_WBF_SIZE1_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE0(0),REG_FSP_RBF_SIZE0_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE1(0),REG_FSP_RBF_SIZE1_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_ENABLE,REG_FSP_ENABLE_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_NRESET,REG_FSP_RESET_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_INT,REG_FSP_INT_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_2NDCMD_ON,REG_FSP_2NDCMD_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_3THCMD_OFF,REG_FSP_3THCMD_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_RCV,REG_FSP_RDSR_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_FSCHK_OFF,REG_FSP_FSCHK_MASK);
	FSP_WRITE_MASK(mtk_nor,REG_FSP_TRIGGER,REG_FSP_FIRE,REG_FSP_TRIGGER_MASK);

	bRet = _HAL_FSP_WaitDone(mtk_nor);
	u8Status = HAL_FSP_CheckFlashBusy(mtk_nor, 1000);
	return ((u8Status|(bRet^0x01))?FALSE:TRUE);
}

static MS_BOOL HAL_FSP_Write(struct mtk_nor *mtk_nor,MS_U32 u32Addr, MS_U32 u32Size, MS_U8 *pu8Data)
{
	MS_BOOL bRet = TRUE;
	MS_U32 u32I, u32J;
	MS_U32 u32quotient;
	MS_U32 u32remainder;
	MS_U32 u32SizeTmp;
	MS_U8	u8BufSize = REG_FSP_WRITEDATA_SIZE;
	MS_U8	u8Status=TRUE;

	FSP_WRITE(mtk_nor,REG_FSP_WBF_SIZE, 0x0);
	FSP_WRITE(mtk_nor,REG_FSP_RBF_SIZE, 0x0);
	u32quotient = (u32Size / u8BufSize);
	u32remainder = (u32Size % u8BufSize);

	for(u32J = 0; u32J <= u32quotient; u32J++) {
		u32SizeTmp = (u32J == u32quotient) ? u32remainder : u8BufSize;

		if(!u32SizeTmp)
			break;

		HAL_FSP_WriteBufs(mtk_nor,0,SPI_CMD_WREN);
		HAL_FSP_WriteBufs(mtk_nor,1,SPI_CMD_PP);
		HAL_FSP_WriteBufs(mtk_nor,2,(MS_U8)((u32Addr>>0x10)&0xFF));
		HAL_FSP_WriteBufs(mtk_nor,3,(MS_U8)((u32Addr>>0x08)&0xFF));
		HAL_FSP_WriteBufs(mtk_nor,4,(MS_U8)((u32Addr>>0x00)&0xFF));

		for( u32I = 0; u32I < u32SizeTmp; u32I++ )
			HAL_FSP_WriteBufs(mtk_nor,(5+u32I),*(pu8Data+u32I));

		u32Addr += u32I;
		pu8Data += u32I;
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE0(1),REG_FSP_WBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE1((4+u32SizeTmp)),REG_FSP_WBF_SIZE1_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE2(0),REG_FSP_WBF_SIZE2_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE0(0),REG_FSP_RBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE1(0),REG_FSP_RBF_SIZE1_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE2(0),REG_FSP_RBF_SIZE2_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_ENABLE,REG_FSP_ENABLE_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_NRESET,REG_FSP_RESET_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_INT,REG_FSP_INT_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_2NDCMD_ON,REG_FSP_2NDCMD_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_3THCMD_OFF,REG_FSP_3THCMD_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_RCV,REG_FSP_RDSR_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_FSCHK_OFF,REG_FSP_FSCHK_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_TRIGGER,REG_FSP_FIRE,REG_FSP_TRIGGER_MASK);
		bRet &= _HAL_FSP_WaitDone(mtk_nor);
		u8Status = HAL_FSP_CheckFlashBusy(mtk_nor, 10);

		if(u8Status)
			dev_err(mtk_nor->dev, "%s HAL_FSP_CheckFlashBusy fail\n", __func__);

		u32SizeTmp = u8BufSize;
	}

	return ((u8Status|(bRet^0x01))?FALSE:TRUE);
}

static MS_BOOL HAL_FSP_Read(struct mtk_nor *mtk_nor,MS_U32 u32Addr, MS_U32 u32Size, MS_U8 *pu8Data)
{
	MS_BOOL bRet = TRUE;
	MS_U32 u32Idx, u32J;
	MS_U32 u32quotient;
	MS_U32 u32remainder;
	MS_U32 u32SizeTmp;
	MS_U8	u8BufSize = REG_FSP_READDATA_SIZE;

	u32quotient = (u32Size / u8BufSize);
	u32remainder = (u32Size % u8BufSize);
	FSP_WRITE(mtk_nor,REG_FSP_WBF_SIZE, 0x0);
	FSP_WRITE(mtk_nor,REG_FSP_RBF_SIZE, 0x0);

	for(u32J = 0; u32J <= u32quotient; u32J++) {
		u32SizeTmp = (u32J == u32quotient) ? u32remainder : u8BufSize;

		if(!u32SizeTmp)
			break;

		HAL_FSP_WriteBufs(mtk_nor,0, SPI_CMD_READ);
		HAL_FSP_WriteBufs(mtk_nor,1,(MS_U8)((u32Addr>>0x10)&0xFF));
		HAL_FSP_WriteBufs(mtk_nor,2,(MS_U8)((u32Addr>>0x08)&0xFF));
		HAL_FSP_WriteBufs(mtk_nor,3,(MS_U8)((u32Addr>>0x00)&0xFF));
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE0(4),REG_FSP_WBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE1(0),REG_FSP_WBF_SIZE1_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE2(0),REG_FSP_WBF_SIZE2_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE0(u32SizeTmp),REG_FSP_RBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE1(0),REG_FSP_RBF_SIZE1_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE2(0),REG_FSP_RBF_SIZE2_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_ENABLE,REG_FSP_ENABLE_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_NRESET,REG_FSP_RESET_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_2NDCMD_OFF,REG_FSP_2NDCMD_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_3THCMD_OFF,REG_FSP_3THCMD_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_FSCHK_OFF,REG_FSP_FSCHK_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_TRIGGER,REG_FSP_FIRE,REG_FSP_TRIGGER_MASK);
		bRet &= _HAL_FSP_WaitDone(mtk_nor);

		for( u32Idx = 0; u32Idx < u32SizeTmp; u32Idx++ )
			*(pu8Data + u32Idx) = HAL_FSP_ReadBufs(mtk_nor,u32Idx);

		pu8Data += u32Idx;
		u32Addr += u32Idx;
	}

	return bRet;
}

static int mtk_nor_erase(struct spi_nor *nor, loff_t offs)
{
	int ret = 0;
	MS_U32	u32StartBlock = 0;
	MS_U32	u32Size = (MS_U32)nor->mtd.erasesize;
	struct mtk_nor *mtk_nor = nor->priv;

	HAL_FSP_Entry(mtk_nor);
	u32StartBlock = (MS_U32)offs/u32Size;
	ret = HAL_FSP_BlockErase(mtk_nor, u32StartBlock, u32Size);
	HAL_FSP_Exit(mtk_nor);
	return 0;
}

static ssize_t mtk_nor_read(struct spi_nor *nor, loff_t from, size_t length,
			    u_char *buffer)
{
	struct mtk_nor *mtk_nor = nor->priv;
	int status;
	struct spi_transfer x[2];
	struct spi_message message;
	struct spi_device *spi_node;
	char tx_buf[FLASH_XFER_CMD_SIZE] = {0};
	loff_t addr;

	if (mtk_nor->type) {
		spi_message_init(&message);
		addr = from;
		tx_buf[0] = SPINOR_OP_READ;
		tx_buf[1] = addr >> 16;
		tx_buf[2] = (addr >> 8)&0xFF;
		tx_buf[3] = addr&0xFF;
		memset(x, 0, sizeof(x));
		x[0].tx_nbits    = SPI_NBITS_SINGLE;
		x[0].rx_nbits    = SPI_NBITS_SINGLE;
		x[0].speed_hz    = mtk_nor->clk;
		x[0].tx_buf = tx_buf;
		x[0].len = FLASH_XFER_CMD_SIZE;
		x[1].tx_nbits    = SPI_NBITS_SINGLE;
		x[1].rx_nbits    = SPI_NBITS_SINGLE;
		x[1].speed_hz    = mtk_nor->clk;
		x[1].len = length;
		x[1].rx_buf = buffer;
		spi_node = km_spidev_node_get(mtk_nor->channel);
		spi_message_add_tail(&x[0], &message);
		spi_message_add_tail(&x[1], &message);
		status = spi_sync(spi_node, &message);
		if (status) {
			dev_err(mtk_nor->dev, "%s fail status:%d\n", __func__, status);
			return -1;
		}
	} else {
		HAL_FSP_Entry(mtk_nor);
		if(!HAL_FSP_Read(mtk_nor, (MS_U32)from, (MS_U32)length, (MS_U8 *)buffer))
			length = 0;
		HAL_FSP_Exit(mtk_nor);
	}

	return length;
}


static ssize_t mtk_nor_write(struct spi_nor *nor, loff_t to, size_t len,
			     const u_char *buf)
{
	struct mtk_nor *mtk_nor = nor->priv;
	int status;
	struct spi_transfer x[2];
	struct spi_message message;
	struct spi_device *spi_node;
	char tx_buf[FLASH_XFER_CMD_SIZE] = {0};
	loff_t addr;

	if (mtk_nor->type) {
		spi_message_init(&message);
		addr = to;
		tx_buf[0] = SPINOR_OP_BP;
		tx_buf[1] = addr >> 16;
		tx_buf[2] = (addr >> 8)&0xFF;
		tx_buf[3] = addr&0xFF;
		memset(x, 0, sizeof(x));
		x[0].tx_nbits    = SPI_NBITS_SINGLE;
		x[0].rx_nbits    = SPI_NBITS_SINGLE;
		x[0].speed_hz    = mtk_nor->clk;
		x[0].tx_buf = tx_buf;
		x[0].len = FLASH_XFER_CMD_SIZE;
		x[1].tx_nbits    = SPI_NBITS_SINGLE;
		x[1].rx_nbits    = SPI_NBITS_SINGLE;
		x[1].speed_hz    = mtk_nor->clk;
		x[1].tx_buf = buf;
		x[1].len = len;
		spi_node = km_spidev_node_get(mtk_nor->channel);
		spi_message_add_tail(&x[0], &message);
		spi_message_add_tail(&x[1], &message);
		status = spi_sync(spi_node, &message);
		if (status) {
			dev_err(mtk_nor->dev, "%s fail status:%d\n", __func__, status);
			return -1;
		}
	} else {
		HAL_FSP_Entry(mtk_nor);
		if(!HAL_FSP_Write(mtk_nor, (MS_U32)to, (MS_U32)len, (MS_U8 *)buf))
			len = 0;
		HAL_FSP_Exit(mtk_nor);
	}
	return len;
}

static int mtk_nor_write_reg(struct spi_nor *nor, u8 opcode,
				u8 *buf, int len)
{
	struct mtk_nor *mtk_nor = nor->priv;
	int status;
	struct spi_transfer x[1];
	struct spi_message message;
	struct spi_device *spi_node;
	char tx_buf[FLASH_XFER_CMD_SIZE] = {0};

	if (mtk_nor->type) {
		spi_message_init(&message);
		memset(x, 0, sizeof(x));
		x[0].tx_nbits    = SPI_NBITS_SINGLE;
		x[0].rx_nbits    = SPI_NBITS_SINGLE;
		x[0].speed_hz    = mtk_nor->clk;

		switch (opcode) {
		case SPINOR_OP_WREN:
			tx_buf[0] = SPINOR_OP_WREN;
			x[0].len = 1;
			break;
		case SPINOR_OP_WRDI:
			tx_buf[0] = SPINOR_OP_WRDI;
			x[0].len = 1;
			break;
		case SPINOR_OP_BE_4K:
			tx_buf[0] = SPINOR_OP_BE_4K;
			tx_buf[1] = buf[0];
			tx_buf[2] = buf[1];
			tx_buf[3] = buf[2];
			x[0].len = FLASH_XFER_CMD_SIZE;
			break;
		default:
			return -EINVAL;
		}
		x[0].tx_buf = tx_buf;
		spi_node = km_spidev_node_get(mtk_nor->channel);
		spi_message_add_tail(&x[0], &message);
		status = spi_sync(spi_node, &message);
		if (status) {
			dev_err(mtk_nor->dev, "%s fail status:%d\n", __func__, status);
			return -1;
		}
	} else {
		MS_U8 u8Status;
		MS_U32 u32Idx;
		MS_BOOL bRet = TRUE;

		HAL_FSP_Entry(mtk_nor);

		FSP_WRITE(mtk_nor,REG_FSP_WBF_SIZE, 0x0);
		FSP_WRITE(mtk_nor,REG_FSP_RBF_SIZE, 0x0);
		HAL_FSP_WriteBufs(mtk_nor,0, opcode);

		for( u32Idx = 0; u32Idx < len; u32Idx++ )
			HAL_FSP_WriteBufs(mtk_nor,(1+u32Idx),*(buf+u32Idx));

		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE0(1),REG_FSP_WBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE0(len),REG_FSP_WBF_SIZE1_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE0(0),REG_FSP_RBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_ENABLE,REG_FSP_ENABLE_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_NRESET,REG_FSP_RESET_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_INT,REG_FSP_INT_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_2NDCMD_OFF,REG_FSP_2NDCMD_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_3THCMD_OFF,REG_FSP_3THCMD_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_FSCHK_OFF,REG_FSP_FSCHK_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_TRIGGER,REG_FSP_FIRE,REG_FSP_TRIGGER_MASK);

		bRet &= _HAL_FSP_WaitDone(mtk_nor);
		u8Status = HAL_FSP_CheckFlashBusy(mtk_nor, 10);

		if(u8Status)
			dev_err(mtk_nor->dev, "HAL_FSP_CheckFlashBusy fail\n");

		HAL_FSP_Exit(mtk_nor);
	}
	return 0;
}

static int mtk_nor_read_reg(struct spi_nor *nor, u8 opcode,
				u8 *buf, int len)
{
	struct mtk_nor *mtk_nor = nor->priv;
	int status;
	struct spi_transfer x[2];
	struct spi_message message;
	struct spi_device *spi_node;
	char tx_buf[1] = {0};

	if (mtk_nor->type) {
		spi_message_init(&message);
		memset(x, 0, sizeof(x));

		x[0].tx_nbits    = SPI_NBITS_SINGLE;
		x[0].rx_nbits    = SPI_NBITS_SINGLE;
		x[0].speed_hz    = mtk_nor->clk;

		x[1].rx_buf    = buf;
		x[1].tx_nbits    = SPI_NBITS_SINGLE;
		x[1].rx_nbits    = SPI_NBITS_SINGLE;
		x[1].speed_hz    = mtk_nor->clk;

		switch (opcode) {
		case SPINOR_OP_RDID:
			tx_buf[0] = SPINOR_OP_RDID;
			x[0].len = 1;
			x[1].len  = len;
			break;
		case SPINOR_OP_RDSR:
			tx_buf[0] = SPINOR_OP_RDSR;
			x[0].len  = 1;
			x[1].len  = len;
			break;
		default:
			return -EINVAL;
		}
		x[0].tx_buf = tx_buf;
		spi_node = km_spidev_node_get(mtk_nor->channel);
		spi_message_add_tail(&x[0], &message);
		spi_message_add_tail(&x[1], &message);
		status = spi_sync(spi_node, &message);
		if (status) {
			dev_err(mtk_nor->dev, "%s fail status:%d\n", __func__, status);
			return -1;
		}
	} else {
		MS_U32 u32Idx;
		MS_BOOL bRet = TRUE;

		HAL_FSP_Entry(mtk_nor);
		FSP_WRITE(mtk_nor,REG_FSP_WBF_SIZE, 0x0);
		FSP_WRITE(mtk_nor,REG_FSP_RBF_SIZE, 0x0);
		HAL_FSP_WriteBufs(mtk_nor,0, opcode);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_WBF_SIZE,REG_FSP_WBF_SIZE0(1),REG_FSP_WBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_RBF_SIZE,REG_FSP_RBF_SIZE0(len),REG_FSP_RBF_SIZE0_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_ENABLE,REG_FSP_ENABLE_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_NRESET,REG_FSP_RESET_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_INT,REG_FSP_INT_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_2NDCMD_OFF,REG_FSP_2NDCMD_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_3THCMD_OFF,REG_FSP_3THCMD_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_CTRL,REG_FSP_FSCHK_OFF,REG_FSP_FSCHK_MASK);
		FSP_WRITE_MASK(mtk_nor,REG_FSP_TRIGGER,REG_FSP_FIRE,REG_FSP_TRIGGER_MASK);
		bRet &= _HAL_FSP_WaitDone(mtk_nor);

		for( u32Idx = 0; u32Idx < len; u32Idx++ )
			*(buf + u32Idx) = HAL_FSP_ReadBufs(mtk_nor,u32Idx);

		HAL_FSP_Exit(mtk_nor);
	}
	return 0;
}

static int mtk_nor_drv_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;
	struct mtk_nor *mtk_nor;
	const struct spi_nor_hwcaps hwcaps = {
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_READ_FAST |
			SNOR_HWCAPS_READ_1_1_2 |
			SNOR_HWCAPS_PP,
	};

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "No DT found\n");
		return -EINVAL;
	}

	mtk_nor = devm_kzalloc(&pdev->dev, sizeof(*mtk_nor), GFP_KERNEL);
	if (!mtk_nor)
		return -ENOMEM;
	platform_set_drvdata(pdev, mtk_nor);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mtk_nor->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mtk_nor->base))
		return PTR_ERR(mtk_nor->base);

	mtk_nor->dev = &pdev->dev;
	mtk_nor->nor.dev = mtk_nor->dev;
	mtk_nor->nor.read = mtk_nor_read;
	mtk_nor->nor.write = mtk_nor_write;
	mtk_nor->nor.read_reg = mtk_nor_read_reg;
	mtk_nor->nor.write_reg = mtk_nor_write_reg;
	ret = of_property_read_u32(pdev->dev.of_node,
				"nor-clk", &mtk_nor->clk);
	if (ret) {
		dev_info(&pdev->dev, "could not get clk_div\n");
		mtk_nor->clk = NOR_CLK;
	}
	ret = of_property_read_u32(pdev->dev.of_node,
				"mspi-channel", &mtk_nor->channel);
	if (ret) {
		dev_info(&pdev->dev, "could not get mspi channel\n");
		mtk_nor->channel = MSPI_CH;
	}
	ret = of_property_read_u32(pdev->dev.of_node,
				"nor-type", &mtk_nor->type);
	if (ret) {
		dev_info(&pdev->dev, "could not get mspi nor-type\n");
		mtk_nor->type = 0;
		mtk_nor->nor.erase = mtk_nor_erase;
	} else {
		mtk_nor->type = 1;
	}
	if ((mtk_nor->channel == 0)&&(mtk_nor->type == 1))
		mtk_nor->nor.mtd.name = "mtxx_serflash_ext";
	else
		mtk_nor->nor.mtd.name = "mtxx_serflash";
	mtk_nor->nor.priv = mtk_nor;
	ret = spi_nor_scan(&mtk_nor->nor, NULL, &hwcaps);
	if (ret) {
		dev_info(&pdev->dev, "failed to locate the chip\n");
		return (int)ERR_PTR(ret);
	}
	ret = mtd_device_register(&mtk_nor->nor.mtd, NULL, 0);

	return ret;
}

static int mtk_nor_drv_remove(struct platform_device *pdev)
{
	//struct mtk_nor *mtk_nor = platform_get_drvdata(pdev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mtk_nor_suspend(struct device *dev)
{
	//struct mtk_nor *mtk_nor = dev_get_drvdata(dev);

	return 0;
}

static int mtk_nor_resume(struct device *dev)
{

	return 0;
}

static const struct dev_pm_ops mtk_nor_dev_pm_ops = {
	.suspend = mtk_nor_suspend,
	.resume = mtk_nor_resume,
};

#define MTK_NOR_DEV_PM_OPS	(&mtk_nor_dev_pm_ops)
#else
#define MTK_NOR_DEV_PM_OPS	NULL
#endif

static const struct of_device_id mtk_nor_of_ids[] = {
	{ .compatible = "mediatek,mt5896-nor"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mtk_nor_of_ids);

static struct platform_driver mtk_nor_driver = {
	.probe = mtk_nor_drv_probe,
	.remove = mtk_nor_drv_remove,
	.driver = {
		.name = "mtk-nor",
		.pm = MTK_NOR_DEV_PM_OPS,
		.of_match_table = mtk_nor_of_ids,
	},
};

module_platform_driver(mtk_nor_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek SPI NOR Flash Driver");
