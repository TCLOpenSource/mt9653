// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/i2c.h>

#include "i2c-mt5896-riu.h"
#include "i2c-mt5896-coda.h"

#define MTK_I2C_LOG_NONE	(-1)	/* set mtk loglevel to be useless */
#define MTK_I2C_LOG_MUTE	(0)
#define MTK_I2C_LOG_ALERT	(1)
#define MTK_I2C_LOG_CRIT	(2)
#define MTK_I2C_LOG_ERR		(3)
#define MTK_I2C_LOG_WARN	(4)
#define MTK_I2C_LOG_NOTICE	(5)
#define MTK_I2C_LOG_INFO	(6)
#define MTK_I2C_LOG_DEBUG	(7)

#define i2c_log(i2c_dev, level, args...) \
do { \
	if (i2c_dev->log_level < 0) { \
		if (level == MTK_I2C_LOG_DEBUG) \
			dev_dbg(i2c_dev->dev, ## args); \
		else if (level == MTK_I2C_LOG_INFO) \
			dev_info(i2c_dev->dev, ## args); \
		else if (level == MTK_I2C_LOG_NOTICE) \
			dev_notice(i2c_dev->dev, ## args); \
		else if (level == MTK_I2C_LOG_WARN) \
			dev_warn(i2c_dev->dev, ## args); \
		else if (level == MTK_I2C_LOG_ERR) \
			dev_err(i2c_dev->dev, ## args); \
		else if (level == MTK_I2C_LOG_CRIT) \
			dev_crit(i2c_dev->dev, ## args); \
		else if (level == MTK_I2C_LOG_ALERT) \
			dev_alert(i2c_dev->dev, ## args); \
	} else if (i2c_dev->log_level >= level) \
		dev_emerg(i2c_dev->dev, ## args); \
} while (0)

enum HAL_HWI2C_STATE {
	E_HAL_HWI2C_STATE_IDLE = 0,
	E_HAL_HWI2C_STATE_START,
	E_HAL_HWI2C_STATE_WRITE,
	E_HAL_HWI2C_STATE_READ,
	E_HAL_HWI2C_STATE_INT,
	E_HAL_HWI2C_STATE_WAIT,
	E_HAL_HWI2C_STATE_STOP
};

#define HWI2C_HAL_WAIT_TIMEOUT		msecs_to_jiffies(1 * 1000)
#define HWI2C_HAL_WAIT_TIMEOUT_POL	(1000)

#define HWI2C_FIFO_SIZE_MAX		(48)
#define HWI2C_DMA_MASK_SIZE		(32)

#define HWI2C_TICKS_PER_US		(12)
#define HWI2C_START_DELAY_US		(5)
#define HWI2C_FINISH_SLEEP_MIN		(60)
#define HWI2C_FINISH_SLEEP_MAX		(120)
#define HWI2C_STD_MODE_TSUSDA_EXT_DELAY_US	(2)

#define I2C_TEN_BIT_ADDR_HIGH_MASK	(0xFF00)

// MIIC_CFG register field, not defined in coda
#define REG_0000_MIIC0_REG_MIIC_CFG_RST		Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_0000_MIIC0_REG_MIIC_CFG_EN_DMA	Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_0000_MIIC0_REG_MIIC_CFG_EN_INT	Fld(1, 2, AC_MSKB0)//[2:2]
#define REG_0000_MIIC0_REG_MIIC_CFG_EN_CLKSTR	Fld(1, 3, AC_MSKB0)//[3:3]
#define REG_0000_MIIC0_REG_MIIC_CFG_EN_TMTINT	Fld(1, 4, AC_MSKB0)//[4:4]
#define REG_0000_MIIC0_REG_MIIC_CFG_FILTER	Fld(1, 5, AC_MSKB0)//[5:5]
#define REG_0000_MIIC0_REG_MIIC_CFG_PUSH1T	Fld(1, 6, AC_MSKB0)//[6:6]

// DMA_CFG register field, not defined in coda
#define REG_0080_MIIC0_REG_DMA_CFG_RST		Fld(1, 1, AC_MSKB0)//[1:1]
#define REG_0000_MIIC0_REG_DMA_CFG_EN_INT	Fld(1, 2, AC_MSKB0)//[2:2]

// reserve0 register field, not defined in coda
#define REG_0040_MIIC0_REG_EN_INT_TRIG_DMA	Fld(1, 0, AC_MSKB0)//[0:0]
#define REG_0040_MIIC0_REG_EN_BYTE_DLY		Fld(1, 3, AC_MSKB0)//[3:3]

//------------------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------------------

#define KHZ(freq)               (1000 * freq)

#define I2C_STD_MODE_SPEED	KHZ(100)

struct dtv_i2c_clk_fld {
	u32 clk_freq_out;
	u8 t_high_cnt;
	u8 t_low_cnt;
};

/**
 * struct dtv_i2c_dev - dtv i2c driver private data
 *
 * @dev: platform device of i2c bus
 * @adapter: i2c adapter of i2c subsystem
 * @clks: array of i2c module clock soource, include sw_en for interfacing with
 *        other module, #0 clock for select source clock rate, #1 clock for
 *        gated clock to save module power
 * @clk_num: number of clks
 * @base: register bank base after ioremap
 * @irq: interrupt number of i2c bus, will polling i2c ready when negative value
 * @completion: completion when waiting irq
 * @bus_speed: specified i2c bus speed
 * @clk_fld: table for bus speed register setting
 * @ext_clk_fld: extended bus speed register setting
 * @start_cnt: SCL and SDA count for start
 * @stop_cnt: SCL and SDA count for stop
 * @sda_cnt: clock count between falling edge SCL and SDA
 * @latch_cnt: data latch timing
 * @byte_ext_dly_cnt: byte-to-byte extra delay count
 * @scl_stretch: enable/disable clock stretch
 * @push_disable: disable pull-up
 * @fifo_depth: I2C IP internal fifo depth
 * @support_dma: hw dma capability
 * @use_dma: indicate current transaction using DMA, for check flag in ISR.
 * @dma_phy_off: Due to memory driver implementation, dma_addr_t have offset
 *               with DRAM address, I2C DMA engine need DRAM address.
 * @log_level: override kernel log for mtk_dbg
 * @nak_log_level: specify NAK error log level
 */
struct dtv_i2c_dev {
	struct device *dev;
	struct i2c_adapter adapter;
	struct clk **clks;
	int clk_num;
	void __iomem *base;
	int irq;
	struct completion completion;
	u32 bus_speed;
	struct dtv_i2c_clk_fld *clk_fld;
	struct dtv_i2c_clk_fld ext_clk_fld;
	u32 start_cnt;
	u32 stop_cnt;
	u32 sda_cnt;
	u32 latch_cnt;
	u32 byte_ext_dly_cnt;
	bool scl_stretch;
	bool push_disable;
	u32 fifo_depth;
	bool support_dma;
	bool use_dma;
	u64 dma_phy_off;
	int log_level;
	int nak_log_level;
};

//------------------------------------------------------------------------------
//  Global Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Forward declaration
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Local Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Debug Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------
/* Due to memory driver implementation, need get i2c_dev->dma_phy_off and
 * use CMA for I2C DMA engine bounce buffer
 */

/*
 * DMA bounce buffer APIs
 */
static int _dtv_i2c_prepare_dma_buf(struct dtv_i2c_dev *i2c_dev)
{
	struct device_node *target_memory_np = NULL;
	uint32_t len = 0;
	__be32 *p = NULL;
	int ret = 0;

	if (!i2c_dev->support_dma)
		// describe not support dma in DTS, no need cma
		return -EINVAL;

	// find memory_info node in dts
	target_memory_np = of_find_node_by_name(NULL, "memory_info");
	if (!target_memory_np)
		return -ENODEV;

	// query cpu_emi0_base value from memory_info node
	p = (__be32 *)of_get_property(target_memory_np, "cpu_emi0_base", &len);
	if (p != NULL) {
		i2c_dev->dma_phy_off = be32_to_cpup(p);
		dev_info(i2c_dev->dev, "cpu_emi0_base = 0x%llX\n",
			 i2c_dev->dma_phy_off);
		of_node_put(target_memory_np);
		p = NULL;
	} else {
		dev_err(i2c_dev->dev, "can not find cpu_emi0_base info\n");
		of_node_put(target_memory_np);
		return -EINVAL;
	}

	// reserved memory for DMA bounce buffer
	ret = of_reserved_mem_device_init(i2c_dev->dev);
	if (ret) {
		dev_warn(i2c_dev->dev, "reserved memory presetting failed\n");
		return ret;
	}

	ret = dma_set_mask_and_coherent(i2c_dev->dev,
					DMA_BIT_MASK(HWI2C_DMA_MASK_SIZE));
	if (ret) {
		dev_warn(i2c_dev->dev, "dma_set_mask_and_coherent failed\n");
		of_reserved_mem_device_release(i2c_dev->dev);
		return ret;
	}

	return 0;
}

static inline void *_dtv_i2c_get_dma_buf(struct dtv_i2c_dev *i2c_dev,
					 struct i2c_msg *msg,
					 dma_addr_t *dma_pa_ptr)
{
	void *dma_va = NULL;

	if ((msg->len > i2c_dev->fifo_depth) && i2c_dev->support_dma) {
		// try use DMA, need allocate bounce buffer

		dma_va = dma_alloc_coherent(i2c_dev->dev, msg->len, dma_pa_ptr,
					    GFP_KERNEL);

		/* dma_va != NULL indicate bounce buffer is available,
		 * use riu mode if allocate bounce buffer failed.
		 */
		if (IS_ERR_OR_NULL(dma_va)) {
			i2c_log(i2c_dev, MTK_I2C_LOG_WARN,
				"dma buffer error!\n");
			dma_va = NULL;
		}
	}

	return dma_va;
}

static inline void _dtv_i2c_put_dma_buf(struct dtv_i2c_dev *i2c_dev,
					struct i2c_msg *msg, void *dma_va,
					dma_addr_t dma_pa)
{
	if (dma_va)
		/* free bounce buffer */
		dma_free_coherent(i2c_dev->dev, msg->len, dma_va, dma_pa);
}

/*
 * DMA mode internal APIs
 */
static inline void ___dtv_i2c_clear_dma_done(struct dtv_i2c_dev *i2c_dev)
{
	mtk_writeb(i2c_dev->base + HWI2C_REG_0090_MIIC0, 1);
}

static inline void __dtv_i2c_dma_enable(struct dtv_i2c_dev *i2c_dev,
					bool enable)
{
	/* Pass miic int for state transition in DMA mode  */
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0040_MIIC0, (enable) ? 1 : 0,
		      REG_0040_MIIC0_REG_EN_INT_TRIG_DMA);
	/* Disable byte mode int when using DMA mode  */
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0000_MIIC0, (enable) ? 0 : 1,
		      REG_0000_MIIC0_REG_MIIC_CFG_EN_INT);
	/* Enable DMA mode  */
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0000_MIIC0, (enable) ? 1 : 0,
		      REG_0000_MIIC0_REG_MIIC_CFG_EN_DMA);
	i2c_dev->use_dma = enable;
}

static inline void __dtv_i2c_dma_enable_fifo(struct dtv_i2c_dev *i2c_dev,
					     bool enable)
{
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0080_MIIC0, (enable) ? 1 : 0,
		      REG_0080_MIIC0_REG_DMA_USE_REG);
}

static inline void __dtv_i2c_dma_ctrl(struct dtv_i2c_dev *i2c_dev,
				      bool direction, bool stop)
{
	mtk_write_fld_multi(i2c_dev->base + HWI2C_REG_008C_MIIC0, 2,
			    (stop) ? 0 : 1, REG_008C_MIIC0_REG_STOP_DISABLE,
			    (direction) ? 1 : 0, REG_008C_MIIC0_REG_READ_CMD);
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "DMA ctrl stop : %d, dir: %d\n",
		stop, direction);
}

static inline void __dtv_i2c_dma_rst(struct dtv_i2c_dev *i2c_dev)
{
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0080_MIIC0, 1,
		      REG_0080_MIIC0_REG_DMA_CFG_RST);
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0080_MIIC0, 0,
		      REG_0080_MIIC0_REG_DMA_CFG_RST);
}

static inline void __dtv_i2c_dma_rst_miu(struct dtv_i2c_dev *i2c_dev)
{
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0080_MIIC0, 1,
		      REG_0080_MIIC0_REG_MIU_RST);
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0080_MIIC0, 0,
		      REG_0080_MIIC0_REG_MIU_RST);
}

static inline void __dtv_i2c_dma_chipaddr(struct dtv_i2c_dev *i2c_dev,
					  int addr_fld)
{
	// set slave address, cmd buffer only for 10-bit address SW workaround
	if ((addr_fld & 0xFF00)) {
		// 10-bit address
		// slave addr 1st 7 bits in slave addr
		mtk_writeb(i2c_dev->base + HWI2C_REG_00B8_MIIC0, addr_fld >> 9);
		// slave addr 2nd byte in cmd
		mtk_writeb(i2c_dev->base + HWI2C_REG_0094_MIIC0,
			   addr_fld & 0xFF);
		// 1 byte cmd for slave addr 2nd byte
		mtk_writeb(i2c_dev->base + HWI2C_REG_00A4_MIIC0, 1);
	} else {
		// 7-bit address
		// slave addr 7 bits in slave addr
		mtk_writeb(i2c_dev->base + HWI2C_REG_00B8_MIIC0, addr_fld >> 1);
		// no cmd for slave addr 2nd byte
		mtk_writeb(i2c_dev->base + HWI2C_REG_00A4_MIIC0, 0);
	}
}

static inline void __dtv_i2c_dma_setlength(struct dtv_i2c_dev *i2c_dev,
					   int len)
{
	mtk_writew(i2c_dev->base + HWI2C_REG_00A8_MIIC0, len & 0xFFFF);
	mtk_writew(i2c_dev->base + HWI2C_REG_00AC_MIIC0, (len >> 16) & 0xFFFF);
}

static inline void __dtv_i2c_dma_setmiuaddr(struct dtv_i2c_dev *i2c_dev,
					    dma_addr_t addr)
{
	mtk_writew(i2c_dev->base + HWI2C_REG_0084_MIIC0, addr & 0xFFFF);
	mtk_writew(i2c_dev->base + HWI2C_REG_0088_MIIC0,
		   (addr >> 16) & 0xFFFF);
	mtk_writeb(i2c_dev->base + HWI2C_REG_00C8_MIIC0,
		   ((u64)addr >> BITS_PER_TYPE(u32)) & U8_MAX);
}

static inline void __dtv_i2c_dma_fill_fifo(struct dtv_i2c_dev *i2c_dev,
					   u8 *data, int length)
{
	int i;
	u16 val;

	for (i = 0 ; i < length ; i += 2) {
		val = (u16)(*data);
		if (i != length - 1)
			val |= (u16)(*(data + 1)) << 8;

		mtk_writew(i2c_dev->base + HWI2C_REG_0100_MIIC0 +
			       ((i >> 1) * 4),
			   val);
		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(w)fifo:(%02d) 0x%04X\n",
			i, val);
		data += 2;
	}
}

static inline void __dtv_i2c_dma_get_fifo(struct dtv_i2c_dev *i2c_dev, u8 *data,
					  int length)
{
	int i;
	u16 val;

	for (i = 0 ; i < length ; i += 2) {
		val = mtk_readw(i2c_dev->base + HWI2C_REG_0180_MIIC0 +
				    ((i >> 1) * 4));
		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(r)fifo:(%02d) 0x%04X\n",
			i, val);

		*data = val & 0xFF;
		data++;

		if (i == length - 1)
			break;

		*data = val >> 8;
		data++;
	}
}

static inline void __dtv_i2c_dma_start(struct dtv_i2c_dev *i2c_dev)
{
	mtk_writeb(i2c_dev->base + HWI2C_REG_00BC_MIIC0, 1);
}

static inline void __dtv_i2c_dma_restart(struct dtv_i2c_dev *i2c_dev)
{
	mtk_writeb(i2c_dev->base + HWI2C_REG_00BC_MIIC0 + 1, 1);
}

static inline int __dtv_i2c_dma_get_tfr_len(struct dtv_i2c_dev *i2c_dev)
{
	int length = mtk_readw(i2c_dev->base + HWI2C_REG_00B0_MIIC0) +
		     (mtk_readw(i2c_dev->base + HWI2C_REG_00B4_MIIC0) << 16);

	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "actural xfer len %d\n", length);

	return length;
}

/*
 * RIU mode internal APIs
 */
static inline void ___dtv_i2c_clear_int(struct dtv_i2c_dev *i2c_dev)
{
	mtk_writeb(i2c_dev->base + HWI2C_REG_0010_MIIC0, 1);
}

static int ___dtv_i2c_wait_int(struct dtv_i2c_dev *i2c_dev)
{
	unsigned long time_left;

	if (i2c_dev->irq <= 0) {
		time_left = HWI2C_HAL_WAIT_TIMEOUT_POL;

		if (i2c_dev->use_dma)
			while (time_left != 0) {
				if (mtk_readb(i2c_dev->base +
					      HWI2C_REG_0090_MIIC0)) {
					usleep_range(120, 240);
					break;
				}

				udelay(10);
				time_left--;
			}
		else
			while (time_left != 0) {
				if (mtk_readb(i2c_dev->base +
					      HWI2C_REG_0010_MIIC0))
					break;

				udelay(10);
				time_left--;
			}
	} else
		time_left =
		    wait_for_completion_timeout(&i2c_dev->completion,
						HWI2C_HAL_WAIT_TIMEOUT);

	if (!time_left) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR,
			"wait int timeout, use_dma = %d, dma irq status: %d, I2C irq status: %d\n",
			i2c_dev->use_dma, mtk_readb(i2c_dev->base + HWI2C_REG_0090_MIIC0),
			mtk_readb(i2c_dev->base + HWI2C_REG_0010_MIIC0));
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR,
			"miic_state,miic_status 0x%04X, dma_tc %d, dma_state,miu_done_z 0x%04X\n",
			mtk_readw(i2c_dev->base + HWI2C_REG_0014_MIIC0),
			__dtv_i2c_dma_get_tfr_len(i2c_dev),
			mtk_readw(i2c_dev->base + HWI2C_REG_00C4_MIIC0));

		// transfer done even timeout occurr
		if ((i2c_dev->use_dma) && mtk_readb(i2c_dev->base + HWI2C_REG_0090_MIIC0))
			time_left = 1;
		else if (mtk_readb(i2c_dev->base + HWI2C_REG_0010_MIIC0))
			time_left = 1;
	}

	/* clear interrupt */
	if (i2c_dev->use_dma)
		___dtv_i2c_clear_dma_done(i2c_dev);
	else
		___dtv_i2c_clear_int(i2c_dev);

	return (time_left) ? 0 : -ETIMEDOUT;
}

static inline int ___dtv_i2c_get_state(struct dtv_i2c_dev *i2c_dev)
{
	uint32_t cur_state = mtk_read_fld(i2c_dev->base + HWI2C_REG_0014_MIIC0,
					  REG_0014_MIIC0_REG_MIIC_STATE);

	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "current state %d\n", cur_state);

	if (cur_state <= 0)	// 0: idle
		return E_HAL_HWI2C_STATE_IDLE;
	else if (cur_state <= 2)	// 1~2:start
		return E_HAL_HWI2C_STATE_START;
	else if (cur_state <= 6)	// 3~6:write
		return E_HAL_HWI2C_STATE_WRITE;
	else if (cur_state <= 10)	// 7~10:read
		return E_HAL_HWI2C_STATE_READ;
	else if (cur_state <= 11)	// 11:interrupt
		return E_HAL_HWI2C_STATE_INT;
	else if (cur_state <= 12)	// 12:wait
		return E_HAL_HWI2C_STATE_WAIT;
	else			// 13~15:stop
		return E_HAL_HWI2C_STATE_STOP;
}

static int ___dtv_i2c_wait_state(struct dtv_i2c_dev *i2c_dev, int state)
{
	int count = HWI2C_HAL_WAIT_TIMEOUT_POL;

	while (count > 0) {
		if (state == ___dtv_i2c_get_state(i2c_dev))
			break;
		udelay(1);
		count--;
	}

	return (count != 0) ? 0 : -ETIMEDOUT;
}

static inline void __dtv_i2c_rst(struct dtv_i2c_dev *i2c_dev)
{
	/* reset standard master iic */
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0000_MIIC0, 1,
		      REG_0000_MIIC0_REG_MIIC_CFG_RST);
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0000_MIIC0, 0,
		      REG_0000_MIIC0_REG_MIIC_CFG_RST);
	/* reset DMA engine */
	__dtv_i2c_dma_rst(i2c_dev);
	/*  reset MIU module in DMA engine */
	__dtv_i2c_dma_rst_miu(i2c_dev);

	/* clear interrupt */
	___dtv_i2c_clear_dma_done(i2c_dev);
	/* clear dma interrupt */
	___dtv_i2c_clear_int(i2c_dev);

	/* confirm SDA/SCL is return to high */
	if (!mtk_read_fld(i2c_dev->base + HWI2C_REG_0018_MIIC0, REG_0018_MIIC0_REG_SCLI))
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "SCL not high after bus reset!\n");

	if (!mtk_read_fld(i2c_dev->base + HWI2C_REG_0018_MIIC0, REG_0018_MIIC0_REG_SCLO))
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "SDA output not high after bus reset!\n");

	if (!mtk_read_fld(i2c_dev->base + HWI2C_REG_0018_MIIC0, REG_0018_MIIC0_REG_SDAI))
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "SDA not high after bus reset!\n");
}

static int __dtv_i2c_send_start(struct dtv_i2c_dev *i2c_dev)
{
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "%s\n", __func__);

	reinit_completion(&i2c_dev->completion);

	mtk_writeb(i2c_dev->base + HWI2C_REG_0004_MIIC0, 1);

	if (___dtv_i2c_wait_int(i2c_dev)) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "send start int timeout\n");
		return -ETIMEDOUT;
	}

	if (___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_WAIT)) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "send start state timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int __dtv_i2c_send_byte(struct dtv_i2c_dev *i2c_dev, u8 data)
{
	uint32_t ack_status;

	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "send 0x%02X\n", data);

	reinit_completion(&i2c_dev->completion);

	mtk_writeb(i2c_dev->base + HWI2C_REG_0008_MIIC0, data);

	if (___dtv_i2c_wait_int(i2c_dev)) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "send byte int timeout\n");
		return -ETIMEDOUT;
	}

	if (___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_WAIT)) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "send byte state timeout\n");
		return -ETIMEDOUT;
	}

	ack_status = mtk_read_fld(i2c_dev->base + HWI2C_REG_0008_MIIC0,
				  REG_0008_MIIC0_REG_WRITE_ACK);
	if (ack_status) {
		i2c_log(i2c_dev, i2c_dev->nak_log_level, "send byte no ack\n");
		return -EREMOTEIO;
	}

	udelay(1);
	return 0;
}

static int __dtv_i2c_get_byte(struct dtv_i2c_dev *i2c_dev, u8 *ptr,
			      bool ack_data)
{
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "get byte with%s ack\n",
		(ack_data) ? "":"out");

	reinit_completion(&i2c_dev->completion);

	mtk_write_fld_multi(i2c_dev->base + HWI2C_REG_000C_MIIC0, 2, 1,
			    REG_000C_MIIC0_REG_RDATA_EN, (ack_data) ? 0 : 1,
			    REG_000C_MIIC0_REG_ACK_BIT);

	if (___dtv_i2c_wait_int(i2c_dev))
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "get byte int timeout\n");

	if (___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_WAIT)) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR,
			"get byte state(0) timeout\n");
		return -ETIMEDOUT;
	}

	*ptr = mtk_readb(i2c_dev->base + HWI2C_REG_000C_MIIC0);

	___dtv_i2c_clear_int(i2c_dev);
	if (___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_WAIT)) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR,
			"get byte state(1) timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int __dtv_i2c_send_stop(struct dtv_i2c_dev *i2c_dev)
{
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "%s\n", __func__);

	reinit_completion(&i2c_dev->completion);

	mtk_writeb(i2c_dev->base + HWI2C_REG_0004_MIIC0 + 1, 1);

	if (___dtv_i2c_wait_int(i2c_dev)) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "send stop int timeout\n");
		return -ETIMEDOUT;
	}

	if (___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_IDLE)) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "send stop state timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * Hardware uses the underlying formula to calculate time periods of
 * SCL clock cycle. Firmware uses some additional cycles excluded from the
 * below formula and it is confirmed that the time periods are within
 * specification limits.
 *
 * time of high period of SCL: t_high = (t_high_cnt + 3) / source_clock
 * time of low period of SCL: t_low = (t_low_cnt + 3) / source_clock
 * source_clock = 12 MHz
 */
static const struct dtv_i2c_clk_fld dtv_i2c_clk_map[] = {
	{KHZ(25), 235, 237},
	{KHZ(50), 115, 117},
	{KHZ(100), 57, 59},
	{KHZ(200), 25, 27},
	{KHZ(300), 15, 17},
	{KHZ(400), 11, 13},
};

static int _dtv_i2c_clk_map_idx(struct dtv_i2c_dev *i2c_dev)
{
	int i;
	const struct dtv_i2c_clk_fld *itr = dtv_i2c_clk_map;

	/* try use customize SCL timing setting(in dts) first if valid */
	if (i2c_dev->ext_clk_fld.clk_freq_out == i2c_dev->bus_speed) {
		i2c_dev->clk_fld = &i2c_dev->ext_clk_fld;
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(dtv_i2c_clk_map); i++, itr++)
		if (itr->clk_freq_out == i2c_dev->bus_speed) {
			i2c_dev->ext_clk_fld.clk_freq_out = itr->clk_freq_out;
			i2c_dev->ext_clk_fld.t_high_cnt = itr->t_high_cnt;
			i2c_dev->ext_clk_fld.t_low_cnt = itr->t_low_cnt;
			i2c_dev->clk_fld = &i2c_dev->ext_clk_fld;
			return 0;
		}

	return -EINVAL;
}

static int _dtv_i2c_init(struct dtv_i2c_dev *i2c_dev)
{
	/*
	 * RIU mode I2C initial
	 */
	/* bus speed timing setup */
	mtk_writew(i2c_dev->base + HWI2C_REG_0024_MIIC0,
		   i2c_dev->clk_fld->t_high_cnt);
	mtk_writew(i2c_dev->base + HWI2C_REG_0028_MIIC0,
		   i2c_dev->clk_fld->t_low_cnt);
	mtk_writew(i2c_dev->base + HWI2C_REG_0020_MIIC0, i2c_dev->stop_cnt);
	mtk_writew(i2c_dev->base + HWI2C_REG_002C_MIIC0, i2c_dev->sda_cnt);
	mtk_writew(i2c_dev->base + HWI2C_REG_0030_MIIC0, i2c_dev->start_cnt);
	mtk_writew(i2c_dev->base + HWI2C_REG_0034_MIIC0, i2c_dev->latch_cnt);
	/* clear interrupt */
	___dtv_i2c_clear_int(i2c_dev);
	/* reset standard master iic */
	__dtv_i2c_rst(i2c_dev);
	/* configuration */
	mtk_write_fld_multi(i2c_dev->base + HWI2C_REG_0000_MIIC0, 6,
			    /* DMA default disable, enable when use it */
			    0, REG_0000_MIIC0_REG_MIIC_CFG_EN_DMA,
			    /* Enable interrupt */
			    1, REG_0000_MIIC0_REG_MIIC_CFG_EN_INT,
			    /* clock stretch (decide by dts) */
			    (i2c_dev->scl_stretch) ? 1 : 0,
			    REG_0000_MIIC0_REG_MIIC_CFG_EN_CLKSTR,
			    /* Disable timeout interrupt */
			    0, REG_0000_MIIC0_REG_MIIC_CFG_EN_TMTINT,
			    /* Enable filter */
			    1, REG_0000_MIIC0_REG_MIIC_CFG_FILTER,
			    /* Enable pull-up */
			    (i2c_dev->push_disable) ? 0 : 1, REG_0000_MIIC0_REG_MIIC_CFG_PUSH1T);

	/*
	 * DMA internal buffer mode I2C initial
	 */
	/* clear interrupt */
	___dtv_i2c_clear_dma_done(i2c_dev);
	/* reset DMA engine */
	__dtv_i2c_dma_rst(i2c_dev);
	/*  reset MIU module in DMA engine */
	__dtv_i2c_dma_rst_miu(i2c_dev);
	/* 7-bit address mode, SW workaround for 10-bit address */
	mtk_write_fld(i2c_dev->base + HWI2C_REG_00B8_MIIC0, 0,
		      REG_00B8_MIIC0_REG_10BIT_MODE);
	/* MIU channel: MIU_CH0 */
	mtk_write_fld(i2c_dev->base + HWI2C_REG_008C_MIIC0, 0,
		      REG_008C_MIIC0_REG_MIU_SEL);
	mtk_write_fld_multi(i2c_dev->base + HWI2C_REG_0080_MIIC0, 3,
			    /* MIU priority low */
			    0, REG_0080_MIIC0_REG_MIU_PRIORITY,
			    /* Enable DMA transfer interrupt */
			    1, REG_0000_MIIC0_REG_DMA_CFG_EN_INT,
			    /* DMA use internal buffer */
			    1, REG_0080_MIIC0_REG_DMA_USE_REG);

	/* DMA mode byte-to-byte delay */
	mtk_write_fld(i2c_dev->base + HWI2C_REG_0040_MIIC0, 1, REG_0040_MIIC0_REG_EN_BYTE_DLY);
	mtk_writew(i2c_dev->base + HWI2C_REG_004C_MIIC0, i2c_dev->byte_ext_dly_cnt);

	/* DMA mode default is disabled */
	i2c_dev->use_dma = false;

	return 0;
}

static int _dtv_i2c_nostart_data(struct dtv_i2c_dev *i2c_dev, bool start, struct i2c_msg *pmsg,
				 bool stop)
{
	int i;
	u8 *ptr = pmsg->buf;

	// only riu mode can skip start sequence

	if (start) {

		/*
		 * According Documentation/i2c/i2c-protocol.rst
		 * If you set the I2C_M_NOSTART variable for the first partial message,
		 * we do not generate Addr, but we do generate the startbit S. This will
		 * probably confuse all other clients on your bus, so don't try this.
		 */
		if (__dtv_i2c_send_start(i2c_dev))
			goto i2c_nostart_err;

		udelay(HWI2C_START_DELAY_US);
	}

	if (pmsg->flags & I2C_M_RD) {
		for (i = 0; i < pmsg->len; i++, ptr++) {
			if (__dtv_i2c_get_byte(i2c_dev, ptr, i < (pmsg->len - 1)))
				goto i2c_nostart_err;
			if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
				udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);
		}
	} else {
		for (i = 0; i < pmsg->len; i++, ptr++) {
			if (__dtv_i2c_send_byte(i2c_dev, *ptr)) {
				i2c_log(i2c_dev, i2c_dev->nak_log_level, "data #%d failed\n", i);
				goto i2c_nostart_err;
			}
			if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
				udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);
		}
	}

	if (stop)
		__dtv_i2c_send_stop(i2c_dev);

	if (!(pmsg->flags & I2C_M_RD))
		usleep_range(HWI2C_FINISH_SLEEP_MIN, HWI2C_FINISH_SLEEP_MAX);

	return pmsg->len;

i2c_nostart_err:
	__dtv_i2c_rst(i2c_dev);
	__dtv_i2c_send_stop(i2c_dev);
	return -EREMOTEIO;
}

static int _dtv_i2c_read_data(struct dtv_i2c_dev *i2c_dev, int addr_fld,
			      u8 *buf, int len, bool stop)
{
	u8 data_stage_addr;
	int i;
	u8 *ptr = buf;

	/*
	 * addressing stage
	 */
	if ((addr_fld & I2C_TEN_BIT_ADDR_HIGH_MASK)) {
		// 10-bit address
		if (__dtv_i2c_send_start(i2c_dev))
			goto i2c_read_err;

		udelay(HWI2C_START_DELAY_US);
		// slave addr 1st 7 bits
		if (__dtv_i2c_send_byte(i2c_dev, (addr_fld >> 8) | 1)) {
			i2c_log(i2c_dev, i2c_dev->nak_log_level, "addr(H) failed\n");
			goto i2c_read_err;
		}
		if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
			udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);

		// slave addr 2nd byte
		if (__dtv_i2c_send_byte(i2c_dev, (addr_fld & U8_MAX))) {
			i2c_log(i2c_dev, i2c_dev->nak_log_level, "addr(L) failed\n");
			goto i2c_read_err;
		}
		if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
			udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);

		data_stage_addr = (addr_fld >> 8) | 1;
	} else
		// 7-bit address (no addressing stage)
		data_stage_addr = (addr_fld & U8_MAX) | 1;

	/*
	 * data reveiving stage
	 */
	if (__dtv_i2c_send_start(i2c_dev))
		goto i2c_read_err;

	udelay(HWI2C_START_DELAY_US);

	if (__dtv_i2c_send_byte(i2c_dev, data_stage_addr)) {
		i2c_log(i2c_dev, i2c_dev->nak_log_level, "addr(D) failed\n");
		goto i2c_read_err;
	}
	if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
		udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);

	for (i = 0; i < len; i++, ptr++) {
		if (__dtv_i2c_get_byte(i2c_dev, ptr, i < (len - 1)))
			goto i2c_read_err;
		if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
			udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);
	}

	if (stop)
		__dtv_i2c_send_stop(i2c_dev);

	return len;

i2c_read_err:
	__dtv_i2c_rst(i2c_dev);
	__dtv_i2c_send_stop(i2c_dev);
	return -EREMOTEIO;
}

static int _dtv_i2c_write_data(struct dtv_i2c_dev *i2c_dev, int addr_fld,
			       u8 *buf, int len, bool stop)
{
	int i;
	u8 *ptr = buf;

	/*
	 * addressing stage
	 */
	if (__dtv_i2c_send_start(i2c_dev))
		goto i2c_write_err;

	udelay(HWI2C_START_DELAY_US);

	if ((addr_fld & I2C_TEN_BIT_ADDR_HIGH_MASK)) {
		// 10-bit address, slave addr 1st 7 bits
		if (__dtv_i2c_send_byte(i2c_dev, (addr_fld >> 8))) {
			i2c_log(i2c_dev, i2c_dev->nak_log_level, "addr(H) failed\n");
			goto i2c_write_err;
		}
		if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
			udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);
	}

	// slave addr 2nd byte or 7-bit address
	if (__dtv_i2c_send_byte(i2c_dev, (addr_fld & U8_MAX))) {
		i2c_log(i2c_dev, i2c_dev->nak_log_level, "addr(L) failed\n");
		goto i2c_write_err;
	}
	if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
		udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);

	/*
	 * data sending stage
	 */
	for (i = 0; i < len; i++, ptr++) {
		if (__dtv_i2c_send_byte(i2c_dev, *ptr)) {
			i2c_log(i2c_dev, i2c_dev->nak_log_level, "data #%d failed\n", i);
			goto i2c_write_err;
		}
		if (!!(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US))
			udelay(i2c_dev->byte_ext_dly_cnt / HWI2C_TICKS_PER_US);
	}

	if (stop)
		__dtv_i2c_send_stop(i2c_dev);

	usleep_range(HWI2C_FINISH_SLEEP_MIN, HWI2C_FINISH_SLEEP_MAX);

	return len;

i2c_write_err:
	__dtv_i2c_rst(i2c_dev);
	__dtv_i2c_send_stop(i2c_dev);
	return -EREMOTEIO;
}

static int _dtv_i2c_dma_read(struct dtv_i2c_dev *i2c_dev, int addr_fld,
			     dma_addr_t dma_pa, int len, bool stop)
{
	int ret, real_len;

	if ((addr_fld & I2C_TEN_BIT_ADDR_HIGH_MASK)) {
		// 10-bit address, addressing sequence the same as write with
		// 0 byte data w/o stop condition
		ret = _dtv_i2c_write_data(i2c_dev, addr_fld, NULL, 0, false);
		if (ret < 0)
			goto i2c_dma_read_error;

		// set data stage slave address for DMA mode
		__dtv_i2c_dma_chipaddr(i2c_dev, addr_fld >> 8);
	} else {
		// set slave address for DMA mode
		__dtv_i2c_dma_chipaddr(i2c_dev, addr_fld);
	}

	// enable DMA mode
	__dtv_i2c_dma_enable(i2c_dev, true);
	// DMA buffer use internal buffer or DRAM
	__dtv_i2c_dma_enable_fifo(i2c_dev, (len <= i2c_dev->fifo_depth));
	// request data transfer length
	__dtv_i2c_dma_setlength(i2c_dev, len);
	// set data address read from I2C DMA
	__dtv_i2c_dma_setmiuaddr(i2c_dev, dma_pa);

	// need reset dma engine before trigger dma
	__dtv_i2c_dma_rst(i2c_dev);
	// need reset dma miu before trigger dma
	__dtv_i2c_dma_rst_miu(i2c_dev);

	// direction & transfer with stop
	__dtv_i2c_dma_ctrl(i2c_dev, true, stop);
	// prepare wait for complete
	reinit_completion(&i2c_dev->completion);
	// trigger DMA
	__dtv_i2c_dma_start(i2c_dev);
	// wait DMA done
	ret = ___dtv_i2c_wait_int(i2c_dev);

	usleep_range(HWI2C_FINISH_SLEEP_MIN, HWI2C_FINISH_SLEEP_MAX);

	if (ret) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "dma read int timeout\n");
		goto i2c_dma_read_error;
	}

	// check actual data length
	real_len = __dtv_i2c_dma_get_tfr_len(i2c_dev);
	if (real_len != len) {
		i2c_log(i2c_dev, i2c_dev->nak_log_level, "only read %d byte(s) expect %d\n",
			real_len, len);
		ret = -EREMOTEIO;
		goto i2c_dma_read_error;
	}

	// disable DMA mode
	__dtv_i2c_dma_enable(i2c_dev, false);

	return len;

i2c_dma_read_error:
	// disable DMA mode
	__dtv_i2c_dma_enable(i2c_dev, false);
	// reset standard master iic
	__dtv_i2c_rst(i2c_dev);

	return ret;
}

static int _dtv_i2c_dma_write(struct dtv_i2c_dev *i2c_dev, int addr_fld,
			      dma_addr_t dma_pa, int len, bool stop)
{
	int ret, real_len;

	// set slave address for DMA mode
	__dtv_i2c_dma_chipaddr(i2c_dev, addr_fld);

	// enable DMA mode
	__dtv_i2c_dma_enable(i2c_dev, true);
	// DMA buffer use internal or DRAM
	__dtv_i2c_dma_enable_fifo(i2c_dev, (len <= i2c_dev->fifo_depth));
	// request data transfer length
	__dtv_i2c_dma_setlength(i2c_dev, len);
	// set data address from I2C DMA to I2C slave
	__dtv_i2c_dma_setmiuaddr(i2c_dev, dma_pa);

	// need reset dma engine before trigger dma
	__dtv_i2c_dma_rst(i2c_dev);
	// need reset dma miu before trigger dma
	__dtv_i2c_dma_rst_miu(i2c_dev);

	// direction & transfer with stop
	__dtv_i2c_dma_ctrl(i2c_dev, false, stop);
	// prepare wait for complete
	reinit_completion(&i2c_dev->completion);
	// trigger DMA
	__dtv_i2c_dma_start(i2c_dev);
	// wait DMA done
	ret = ___dtv_i2c_wait_int(i2c_dev);

	usleep_range(HWI2C_FINISH_SLEEP_MIN, HWI2C_FINISH_SLEEP_MAX);

	if (ret) {
		i2c_log(i2c_dev, MTK_I2C_LOG_ERR, "dma write int timeout\n");
		goto i2c_dma_write_error;
	}

	// check actual data length
	real_len = __dtv_i2c_dma_get_tfr_len(i2c_dev);
	if (real_len != len) {
		i2c_log(i2c_dev, i2c_dev->nak_log_level, "only write %d byte(s) expect %d\n",
			real_len, len);
		ret = -EREMOTEIO;
		goto i2c_dma_write_error;
	}

	// disable DMA mode
	__dtv_i2c_dma_enable(i2c_dev, false);

	return len;

i2c_dma_write_error:
	// disable DMA mode
	__dtv_i2c_dma_enable(i2c_dev, false);
	// reset standard master iic
	__dtv_i2c_rst(i2c_dev);

	return ret;
}

static int _dtv_i2c_prepare_dma_xfer(struct dtv_i2c_dev *i2c_dev, struct i2c_msg *msg, void *dma_va,
				     dma_addr_t dma_pa, dma_addr_t *dma_relloc_pa, bool stop)
{
	if (msg->len == 0)
		// Can not detect whether error or not in DMA mode with zero byte data.
		return -1;

	if (msg->len <= i2c_dev->fifo_depth) {
		// Can use internal bouncing buffer, not use dma_pa in i2c dma xfer
		*dma_relloc_pa = 0;

		if (!(msg->flags & I2C_M_RD))
			// fill internal bouncing buffer for I2C write
			__dtv_i2c_dma_fill_fifo(i2c_dev, msg->buf, msg->len);

		return 0;
	}

	if (dma_va) {
		// Can use DRAM bouncing buffer, adjust dma_pa in i2c dma xfer
		*dma_relloc_pa = dma_pa - i2c_dev->dma_phy_off;

		if (!(msg->flags & I2C_M_RD))
			// fill write data to DRAM bouncing buffer
			memcpy(dma_va, msg->buf, msg->len);

		return 0;
	}

	return -1;
}

static void _dtv_i2c_finish_dma_xfer(struct dtv_i2c_dev *i2c_dev, struct i2c_msg *msg, void *dma_va)
{
	if (!(msg->flags & I2C_M_RD))
		// nothing todo in I2C write
		return;

	if (msg->len <= i2c_dev->fifo_depth)
		// get data from DMA internal buffer
		__dtv_i2c_dma_get_fifo(i2c_dev, msg->buf, msg->len);
	else if (dma_va)
		// get read data from DRAM bounce buffer
		memcpy(msg->buf, dma_va, msg->len);
}

static int _dtv_i2c_parse_dt(struct dtv_i2c_dev *i2c_dev)
{
#define EXT_SCL_FREQ_IDX	0
#define EXT_SCL_HIGH_IDX	1
#define EXT_SCL_LOW_IDX		2

	u32 ext_scl_setting[] = { 0, 0, 0 };

	if (!i2c_dev->dev->of_node)
		return -ENXIO;

	/* start count */
	if (device_property_read_u32(i2c_dev->dev, "start-count",
				     &i2c_dev->start_cnt))
		i2c_dev->start_cnt = 73;

	/* stop count */
	if (device_property_read_u32(i2c_dev->dev, "stop-count",
				     &i2c_dev->stop_cnt))
		i2c_dev->stop_cnt = 73;

	/* sda count */
	if (device_property_read_u32(i2c_dev->dev, "sda-count",
				     &i2c_dev->sda_cnt))
		i2c_dev->sda_cnt = 5;

	/* latch count */
	if (device_property_read_u32(i2c_dev->dev, "data-latch-count",
				     &i2c_dev->latch_cnt))
		i2c_dev->latch_cnt = 5;

	/* scl stretch */
	i2c_dev->scl_stretch =
	    device_property_read_bool(i2c_dev->dev, "scl-stretch");

	/* pull-up */
	i2c_dev->push_disable =
	    device_property_read_bool(i2c_dev->dev, "push-disable");

	/* hw capability of dma engine */
	i2c_dev->support_dma =
	    device_property_read_bool(i2c_dev->dev, "support-dma");

	/* dma bounce buffer size */
	if (device_property_read_u32(i2c_dev->dev, "fifo-depth",
				     &i2c_dev->fifo_depth))
		i2c_dev->fifo_depth = 0;

	if (i2c_dev->fifo_depth > HWI2C_FIFO_SIZE_MAX)
		i2c_dev->fifo_depth = HWI2C_FIFO_SIZE_MAX;

	/* customize SCL count */
	if (device_property_read_u32_array(i2c_dev->dev, "speed-scl-count", ext_scl_setting,
					   ARRAY_SIZE(ext_scl_setting))) {
		i2c_dev->ext_clk_fld.clk_freq_out = U32_MAX;
		i2c_dev->ext_clk_fld.t_high_cnt = U8_MAX;
		i2c_dev->ext_clk_fld.t_low_cnt = U8_MAX;
	} else {
		i2c_dev->ext_clk_fld.clk_freq_out =
		    ext_scl_setting[EXT_SCL_FREQ_IDX];
		i2c_dev->ext_clk_fld.t_high_cnt =
		    ext_scl_setting[EXT_SCL_HIGH_IDX] & U8_MAX;
		i2c_dev->ext_clk_fld.t_low_cnt =
		    ext_scl_setting[EXT_SCL_LOW_IDX] & U8_MAX;
	}

	/* byte-to-byte delay count */
	if (device_property_read_u32(i2c_dev->dev, "byte-delay-count", &i2c_dev->byte_ext_dly_cnt))
		i2c_dev->byte_ext_dly_cnt = 0;

	dev_info(i2c_dev->dev, "start_cnt %d, stop_cnt %d, sda_cnt %d, data_lat_cnt %d,\n",
		 i2c_dev->start_cnt, i2c_dev->stop_cnt, i2c_dev->sda_cnt, i2c_dev->latch_cnt);

	dev_info(i2c_dev->dev, "scl_stretch %d, use dma %d, fifo depth %d\n", i2c_dev->scl_stretch,
		 i2c_dev->support_dma, i2c_dev->fifo_depth);

	dev_info(i2c_dev->dev, "customize scl freq %d, scl high %d, scl low %d\n",
		 i2c_dev->ext_clk_fld.clk_freq_out, i2c_dev->ext_clk_fld.t_high_cnt,
		 i2c_dev->ext_clk_fld.t_low_cnt);

	dev_info(i2c_dev->dev, "byte-delay-count %d\n", i2c_dev->byte_ext_dly_cnt);

	return 0;
}

//------------------------------------------------------------------------------
//  Global Functions
//------------------------------------------------------------------------------

static ssize_t start_count_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t n)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	if (buf) {
		unsigned long value;

		if (kstrtoul(buf, 10, &value))
			return -EINVAL;

		i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(W)start_cnt %ld\n",
			value);
		mtk_writew(i2c_dev->base + HWI2C_REG_0030_MIIC0, value);
		i2c_dev->start_cnt =
		    mtk_readw(i2c_dev->base + HWI2C_REG_0030_MIIC0);

		i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		return n;
	}

	return -EINVAL;
}

static ssize_t start_count_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	i2c_dev->start_cnt = mtk_readw(i2c_dev->base + HWI2C_REG_0030_MIIC0);
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)start_cnt %d\n",
		i2c_dev->start_cnt);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->start_cnt);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RW(start_count);

static ssize_t stop_count_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t n)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	if (buf) {
		unsigned long value;

		if (kstrtoul(buf, 10, &value))
			return -EINVAL;

		i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(W)stop_cnt %ld\n", value);
		mtk_writew(i2c_dev->base + HWI2C_REG_0020_MIIC0, value);
		i2c_dev->stop_cnt =
		    mtk_readw(i2c_dev->base + HWI2C_REG_0020_MIIC0);

		i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		return n;
	}

	return -EINVAL;
}

static ssize_t stop_count_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	i2c_dev->stop_cnt = mtk_readw(i2c_dev->base + HWI2C_REG_0020_MIIC0);
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)stop_cnt %d\n",
		i2c_dev->stop_cnt);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->stop_cnt);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RW(stop_count);

static ssize_t sda_count_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t n)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	if (buf) {
		unsigned long value;

		if (kstrtoul(buf, 10, &value))
			return -EINVAL;

		i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(W)sda_cnt %ld\n", value);
		mtk_writew(i2c_dev->base + HWI2C_REG_002C_MIIC0, value);
		i2c_dev->sda_cnt =
		    mtk_readw(i2c_dev->base + HWI2C_REG_002C_MIIC0);

		i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		return n;
	}

	return -EINVAL;
}

static ssize_t sda_count_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	i2c_dev->sda_cnt = mtk_readw(i2c_dev->base + HWI2C_REG_002C_MIIC0);
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)sda_cnt %d\n",
		i2c_dev->sda_cnt);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->sda_cnt);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RW(sda_count);

static ssize_t latch_count_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t n)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	if (buf) {
		unsigned long value;

		if (kstrtoul(buf, 10, &value))
			return -EINVAL;

		i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(W)data_latch_cnt %ld\n",
			value);
		mtk_writew(i2c_dev->base + HWI2C_REG_0034_MIIC0, value);
		i2c_dev->latch_cnt =
		    mtk_readw(i2c_dev->base + HWI2C_REG_0034_MIIC0);

		i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		return n;
	}

	return -EINVAL;
}

static ssize_t latch_count_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	i2c_dev->latch_cnt = mtk_readw(i2c_dev->base + HWI2C_REG_0034_MIIC0);
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)data_latch_cnt %d\n",
		i2c_dev->latch_cnt);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->latch_cnt);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RW(latch_count);

static ssize_t scl_stretch_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	if (mtk_read_fld(i2c_dev->base + HWI2C_REG_0000_MIIC0,
			 REG_0000_MIIC0_REG_MIIC_CFG_EN_CLKSTR))
		i2c_dev->scl_stretch = true;
	else
		i2c_dev->scl_stretch = false;

	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)clock stretch %d\n",
		i2c_dev->scl_stretch);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->scl_stretch);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RO(scl_stretch);

static ssize_t oen_push_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t n)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	if (buf) {
		int value;

		if (kstrtoint(buf, 10, &value))
			return -EINVAL;

		value = (value) ? 1 : 0;

		i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(W)oen push %d\n", value);
		mtk_write_fld(i2c_dev->base + HWI2C_REG_0000_MIIC0, value,
			      REG_0000_MIIC0_REG_MIIC_CFG_PUSH1T);
		if (mtk_read_fld(i2c_dev->base + HWI2C_REG_0000_MIIC0,
				 REG_0000_MIIC0_REG_MIIC_CFG_PUSH1T))
			i2c_dev->push_disable = false;
		else
			i2c_dev->push_disable = true;

		i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		return n;
	}

	return -EINVAL;
}

static ssize_t oen_push_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	if (mtk_read_fld(i2c_dev->base + HWI2C_REG_0000_MIIC0,
			REG_0000_MIIC0_REG_MIIC_CFG_PUSH1T))
		i2c_dev->push_disable = false;
	else
		i2c_dev->push_disable = true;

	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)oen push %d\n", !i2c_dev->push_disable);

	str += scnprintf(str, end - str, "%d\n", !i2c_dev->push_disable);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RW(oen_push);

static ssize_t clkh_count_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t n)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	if (buf) {
		unsigned long value;

		if (kstrtoul(buf, 10, &value))
			return -EINVAL;

		i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(W)clkh_cnt %ld\n",
			value);
		mtk_writew(i2c_dev->base + HWI2C_REG_0024_MIIC0, value);
		i2c_dev->clk_fld->t_high_cnt =
		    mtk_readw(i2c_dev->base + HWI2C_REG_0024_MIIC0);

		i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		return n;
	}

	return -EINVAL;
}

static ssize_t clkh_count_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	i2c_dev->clk_fld->t_high_cnt =
	    mtk_readw(i2c_dev->base + HWI2C_REG_0024_MIIC0);
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)clkh_cnt %d\n",
		i2c_dev->clk_fld->t_high_cnt);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->clk_fld->t_high_cnt);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RW(clkh_count);

static ssize_t clkl_count_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t n)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	if (buf) {
		unsigned long value;

		if (kstrtoul(buf, 10, &value))
			return -EINVAL;

		i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(W)clkl_cnt %ld\n",
			value);
		mtk_writew(i2c_dev->base + HWI2C_REG_0028_MIIC0, value);
		i2c_dev->clk_fld->t_low_cnt =
		    mtk_readw(i2c_dev->base + HWI2C_REG_0028_MIIC0);

		i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		return n;
	}

	return -EINVAL;
}

static ssize_t clkl_count_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	i2c_dev->clk_fld->t_low_cnt =
	    mtk_readw(i2c_dev->base + HWI2C_REG_0028_MIIC0);
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)clkl_cnt %d\n",
		i2c_dev->clk_fld->t_low_cnt);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->clk_fld->t_low_cnt);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RW(clkl_count);

static ssize_t byte_delay_count_store(struct device *dev, struct device_attribute *attr,
				      const char *buf, size_t n)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	if (buf) {
		unsigned long value;

		if (kstrtoul(buf, 10, &value))
			return -EINVAL;

		i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(W)byte_delay_count %ld\n", value);
		mtk_writew(i2c_dev->base + HWI2C_REG_004C_MIIC0, value);
		i2c_dev->byte_ext_dly_cnt = mtk_readw(i2c_dev->base + HWI2C_REG_004C_MIIC0);

		i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

		return n;
	}

	return -EINVAL;
}

static ssize_t byte_delay_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	i2c_dev->byte_ext_dly_cnt = mtk_readw(i2c_dev->base + HWI2C_REG_004C_MIIC0);
	i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "(R)byte_delay_count %d\n", i2c_dev->byte_ext_dly_cnt);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->byte_ext_dly_cnt);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RW(byte_delay_count);

static void _dtv_i2c_create_priv_ctrl_attr(struct device *dev)
{
	int retval;

	retval = device_create_file(dev, &dev_attr_start_count);
	if (retval)
		dev_err(dev, "failed to create file node for adjust start count (%d)\n", retval);
	retval = device_create_file(dev, &dev_attr_stop_count);
	if (retval)
		dev_err(dev, "failed to create file node for adjust stop count (%d)\n", retval);
	retval = device_create_file(dev, &dev_attr_sda_count);
	if (retval)
		dev_err(dev, "failed to create file node for adjust sda count (%d)\n", retval);
	retval = device_create_file(dev, &dev_attr_latch_count);
	if (retval)
		dev_err(dev, "failed to create file node for adjust data latch count (%d)\n",
			retval);
	retval = device_create_file(dev, &dev_attr_scl_stretch);
	if (retval)
		dev_err(dev, "failed to create file node for check scl stretch count (%d)\n",
			retval);
	retval = device_create_file(dev, &dev_attr_oen_push);
	if (retval)
		dev_err(dev, "failed to create file node for check oen push (%d)\n", retval);
	retval = device_create_file(dev, &dev_attr_clkh_count);
	if (retval)
		dev_err(dev, "failed to create file node for adjust scl high count (%d)\n", retval);
	retval = device_create_file(dev, &dev_attr_clkl_count);
	if (retval)
		dev_err(dev, "failed to create file node for adjust scl low count (%d)\n", retval);
	retval = device_create_file(dev, &dev_attr_byte_delay_count);
	if (retval)
		dev_err(dev, "failed to create file node for adjust byte-to-byte delay count (%d)\n",
			retval);
}

/* mtk_dbg sysfs group */
static ssize_t help_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	str +=
	    scnprintf(str, end - str, "Debug Help:\n"
		      "- log_level <RW>: To control debug log level.\n"
		      "                  Read log_level for the definition of log level number.\n");

	return (str - buf);
}

static DEVICE_ATTR_RO(help);

static ssize_t log_level_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	str +=
	    scnprintf(str, end - str,
		      "none:\t\t%d\nmute:\t\t%d\nalert:\t\t%d\ncritical:\t\t%d\nerror:\t\t%d\n"
		      "warning:\t\t%d\nnotice:\t\t%d\ninfo:\t\t%d\ndebug:\t\t%d\ncurrent:\t%d\n",
		      MTK_I2C_LOG_NONE, MTK_I2C_LOG_MUTE, MTK_I2C_LOG_ALERT, MTK_I2C_LOG_CRIT,
		      MTK_I2C_LOG_ERR, MTK_I2C_LOG_WARN, MTK_I2C_LOG_NOTICE, MTK_I2C_LOG_INFO,
		      MTK_I2C_LOG_DEBUG, i2c_dev->log_level);

	return (str - buf);
}

static ssize_t log_level_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	ssize_t retval;
	int level;

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < MTK_I2C_LOG_NONE) || (level > MTK_I2C_LOG_DEBUG))
			retval = -EINVAL;
		else
			i2c_dev->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static DEVICE_ATTR_RW(log_level);

static ssize_t nak_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	str +=
	    scnprintf(str, end - str,
		      "none:\t\t%d\nmute:\t\t%d\nalert:\t\t%d\ncritical:\t\t%d\nerror:\t\t%d\n"
		      "warning:\t\t%d\nnotice:\t\t%d\ninfo:\t\t%d\ndebug:\t\t%d\ncurrent:\t%d\n",
		      MTK_I2C_LOG_NONE, MTK_I2C_LOG_MUTE, MTK_I2C_LOG_ALERT, MTK_I2C_LOG_CRIT,
		      MTK_I2C_LOG_ERR, MTK_I2C_LOG_WARN, MTK_I2C_LOG_NOTICE, MTK_I2C_LOG_INFO,
		      MTK_I2C_LOG_DEBUG, i2c_dev->nak_log_level);

	return (str - buf);
}

static ssize_t nak_log_level_store(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	ssize_t retval;
	int level;

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < MTK_I2C_LOG_NONE) || (level > MTK_I2C_LOG_DEBUG))
			retval = -EINVAL;
		else
			i2c_dev->nak_log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static DEVICE_ATTR_RW(nak_log_level);

static struct attribute *mtk_dtv_i2c_dbg_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	&dev_attr_nak_log_level.attr,
	NULL,
};

static const struct attribute_group mtk_dtv_i2c_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_i2c_dbg_attrs,
};

static irqreturn_t dtv_i2c_interrupt(int irq, void *dev)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata((struct device *)dev);

	if (i2c_dev->use_dma &&
	    (mtk_readb(i2c_dev->base + HWI2C_REG_0090_MIIC0))) {
		___dtv_i2c_clear_dma_done(i2c_dev);
		complete(&i2c_dev->completion);
		return IRQ_HANDLED;
	} else if (mtk_readb(i2c_dev->base + HWI2C_REG_0010_MIIC0)) {
		___dtv_i2c_clear_int(i2c_dev);
		complete(&i2c_dev->completion);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

//------------------------------------------------------------------------------
//  linux i2c driver framework
//------------------------------------------------------------------------------

static int dtv_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
			int num)
{
	struct dtv_i2c_dev *i2c_dev = i2c_get_adapdata(adap);
	int ret = 0;
	int i;

	for (i = 0; ret >= 0 && i < num; i++) {
		/* address(include R/W bit) field, also for 10-bit addr. */
		int addr_fld;
		/* DMA mode */
		dma_addr_t dma_pa = 0, dma_relloc_pa = 0;
		void *dma_va = NULL;
		bool stop = (i == (num - 1));

		if (msgs[i].flags & I2C_M_TEN) {
			/* 10-bit address mode */

			/* address[9:8] */
			addr_fld = msgs[i].addr & 0x0300;
			/* reserve R/W bit */
			addr_fld <<= 1;
			/* high byte value of addressing 0b11110XX+(R/W) */
			addr_fld |= 0xF000;
			/* low byte value of addressing */
			addr_fld |= msgs[i].addr & 0xFF;
		} else
			/* 7-bit address mode, reserve R/W bit */
			addr_fld = (msgs[i].addr & 0x7F) << 1;

		/*
		 * do Read/Write, NOTICE: place stop at the end of the last msg
		 * use different transfer mode according data length
		 *
		 * NOTE: Due to tSU:STA of I2C standard mode(bus speed 100KHz), place I2C module
		 * reset make SDA high before next i2c_msg when necessary.
		 */
		if (i2c_dev->bus_speed <= I2C_STD_MODE_SPEED)
			if ((i > 0) && !(msgs[i].flags & I2C_M_NOSTART)) {
				i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG, "tSU:SDA workaround reset\n");
				__dtv_i2c_rst(i2c_dev);
				udelay(HWI2C_STD_MODE_TSUSDA_EXT_DELAY_US);
			}

		/*
		 * 1. Can not detect whether error or not in DMA mode with zero byte data.
		 * 2. PM_I2C DMA engine can not access DRAM
		 * 3. due to memory driver implementation, need bounce buffer for DMA direct access
		 *    DRAM
		 *
		 * Try allocate DMA bounce buffer if needed,
		 * dma_va != NULL indicate bounce buffer is available,
		 * use riu mode if allocate bounce buffer failed.
		 * This function is similar as i2c_get_dma_safe_msg_buf() api.
		 */
		dma_va = _dtv_i2c_get_dma_buf(i2c_dev, &(msgs[i]), &dma_pa);

		// check whether or not using DMA engine according:
		// 1. data length, 2. DRAM bounce buffer existence, 3. have remain transaction
		if (msgs[i].flags & I2C_M_NOSTART) {
			// special case for "NOSTART"
			ret = _dtv_i2c_nostart_data(i2c_dev, i == 0, &(msgs[i]), stop);
		} else if (_dtv_i2c_prepare_dma_xfer(i2c_dev, &msgs[i], dma_va, dma_pa,
						     &dma_relloc_pa, stop) >= 0) {
			// may use dma xfer
			if (msgs[i].flags & I2C_M_RD)
				// execute dma read
				ret = _dtv_i2c_dma_read(i2c_dev, addr_fld, dma_relloc_pa,
							msgs[i].len, stop);
			else
				// execute dma write
				ret = _dtv_i2c_dma_write(i2c_dev, addr_fld, dma_relloc_pa,
							 msgs[i].len, stop);

			if (ret > 0)
				_dtv_i2c_finish_dma_xfer(i2c_dev, &msgs[i], dma_va);
		} else {
			// inappropriate dma xfer
			if (msgs[i].flags & I2C_M_RD)
				ret = _dtv_i2c_read_data(i2c_dev, addr_fld, msgs[i].buf,
							 msgs[i].len, stop);
			else
				ret = _dtv_i2c_write_data(i2c_dev, addr_fld, msgs[i].buf,
							  msgs[i].len, stop);
		}

		if (ret < 0)
			i2c_log(i2c_dev, i2c_dev->nak_log_level,
				"msg %d: chip=0x%X, len=%d, va %p, pa %llX ret=%d\n",
				i, addr_fld, msgs[i].len, dma_va, (u64)dma_pa, ret);
		else
			i2c_log(i2c_dev, MTK_I2C_LOG_DEBUG,
				"msg %d: chip=0x%X, len=%d, va %p, pa %llX ret=%d\n",
				i, addr_fld, msgs[i].len, dma_va, (u64)dma_pa, ret);

		// free allocated DMA bounce buffer if needed
		_dtv_i2c_put_dma_buf(i2c_dev, &(msgs[i]), dma_va, dma_pa);
	}

	return (ret >= 0) ? i : ret;
}

static u32 dtv_i2c_func(struct i2c_adapter *adap)
{
	/* return i2c bus driver FUNCTIONALITY CONSTANTS
	 * I2C_FUNC_I2C        : Plain i2c-level commands (Pure SMBus adapters
	 *                       typically can not do these)
	 * I2C_FUNC_10BIT_ADDR : Handles the 10-bit address extensions
	 * I2C_FUNC_NOSTART    : Can skip repeated start sequence
	 * I2C_FUNC_SMBUS_EMUL : Handles all SMBus commands that can be
	 *                       emulated by a real I2C adapter (using
	 *                       the transparent emulation layer)
	 *
	 * text copy from file : Documentation/i2c/functionality
	 */
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR | I2C_FUNC_NOSTART | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm dtv_i2c_algo = {
	.master_xfer = dtv_i2c_xfer,
	.functionality = dtv_i2c_func,
};

static int dtv_i2c_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct dtv_i2c_dev *i2c_dev;
	struct resource *res;
	struct property *clk_prop;
	const char *clk_name;
	int i;

	dev_info(&pdev->dev, "%s\n", __func__);

	/* init dtv_i2c_dev device struct */
	i2c_dev = devm_kzalloc(&pdev->dev, sizeof(*i2c_dev), GFP_KERNEL);
	if (!i2c_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, i2c_dev);

	init_completion(&i2c_dev->completion);
	i2c_dev->dev = &pdev->dev;

	/* I2C controllor registers base and range */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c_dev->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2c_dev->base)) {
		dev_err(&pdev->dev, "failed to map ioresource (%ld)\n",
			PTR_ERR(i2c_dev->base));
		return PTR_ERR(i2c_dev->base);
	}
	dev_dbg(i2c_dev->dev, "reg_base %p\n", i2c_dev->base);

	/* module clock source */
	/* get clock count */
	i2c_dev->clk_num =
	    of_property_count_strings(pdev->dev.of_node, "clock-names");
	dev_dbg(i2c_dev->dev, "have %d clock names\n", i2c_dev->clk_num);
	if (i2c_dev->clk_num <= 0)
		dev_info(i2c_dev->dev, "no clock names in dts\n");
	else {
		/* alloc for clock handle */
		i2c_dev->clks = devm_kmalloc_array(&pdev->dev, i2c_dev->clk_num,
						   sizeof(*(i2c_dev->clks)),
						   GFP_KERNEL | __GFP_ZERO);
		if (!i2c_dev->clks)
			return -ENOMEM;

		/* traverse all clock names and get clk handle */
		i = 0;
		of_property_for_each_string(pdev->dev.of_node, "clock-names",
					    clk_prop, clk_name) {
			i2c_dev->clks[i] = devm_clk_get(&pdev->dev, clk_name);
			if (IS_ERR(i2c_dev->clks[i])) {
				dev_err(&pdev->dev, "clk_get #%d failed\n", i);
				break;
			}
			i++;
		}

		if (i < i2c_dev->clk_num)
			/* not all devm_clk_get() success */
			return PTR_ERR(i2c_dev->clks[i]);

		/* enable all clks, include sw_en*/
		for (i = 0 ; i < i2c_dev->clk_num ; i++) {
			retval = clk_prepare_enable(i2c_dev->clks[i]);
			if (retval < 0) {
				dev_err(&pdev->dev,
					"clk enable failed in #%d clks (%d)\n",
					i, retval);
				return retval;
			}
		}
	}

	/* interrupt */
	retval = platform_get_irq(pdev, 0);
	i2c_dev->irq = retval;

	/* parse proprietary i2c info */
	retval = _dtv_i2c_parse_dt(i2c_dev);
	if (retval < 0) {
		dev_err(&pdev->dev, "unable to parse device tree\n");
		return retval;
	}

	/* bus speed */
	retval = device_property_read_u32(&pdev->dev, "clock-frequency",
					  &i2c_dev->bus_speed);
	if (retval) {
		dev_info(&pdev->dev, "DTB no bus frequency, default 100kHz.\n");
		i2c_dev->bus_speed = KHZ(100);
	}
	retval = _dtv_i2c_clk_map_idx(i2c_dev);
	if (retval) {
		dev_err(&pdev->dev, "Invalid clk frequency %d Hz\n",
			i2c_dev->bus_speed);
		return retval;
	}

	/* bus number */
	retval = device_property_read_u32(&pdev->dev, "bus-number",
					  &i2c_dev->adapter.nr);
	if (retval) {
		dev_info(&pdev->dev, "DTB no bus number, dynamic assign.\n");
		i2c_dev->adapter.nr = -1;
	}

	retval = _dtv_i2c_prepare_dma_buf(i2c_dev);
	if (retval) {
		dev_info(&pdev->dev, "not support dma because no reserved memory\n");
		i2c_dev->support_dma = 0;
	}

	/* module initial */
	retval = _dtv_i2c_init(i2c_dev);
	if (retval) {
		dev_err(&pdev->dev, "failed to init i2c");
		return retval;
	}

	/* interrupt service routine */
	if (i2c_dev->irq <= 0)
		dev_err(&pdev->dev, "no irq resource, check DTS file (%d)\n", i2c_dev->irq);
	else {
		dev_dbg(i2c_dev->dev, "irq %d\n", i2c_dev->irq);
		retval = devm_request_irq(&pdev->dev, i2c_dev->irq, dtv_i2c_interrupt,
					  IRQF_TRIGGER_NONE, dev_name(&pdev->dev), &pdev->dev);
		if (retval) {
			dev_err(&pdev->dev, "Could not request IRQ\n");
			return retval;
		}
	}

	/* register i2c adapter and bus algorithm */
	i2c_dev->adapter.owner = THIS_MODULE;
	i2c_dev->adapter.class = 0;
	scnprintf(i2c_dev->adapter.name, sizeof(i2c_dev->adapter.name),
		  "i2c-mt5896");
	i2c_dev->adapter.dev.parent = &pdev->dev;
	i2c_dev->adapter.algo = &dtv_i2c_algo;
	i2c_dev->adapter.dev.of_node = pdev->dev.of_node;
	i2c_set_adapdata(&i2c_dev->adapter, i2c_dev);
	retval = i2c_add_numbered_adapter(&i2c_dev->adapter);
	if (retval) {
		dev_err(&pdev->dev, "failed to add i2c adapter (%d)\n", retval);
		return retval;
	}

	/* create sysfs file for proprietary control parameter, if sysfs file
	 * creation failed, proprietary control parameter can not adjusted but
	 * I2C bus still works with default value.
	 */
	_dtv_i2c_create_priv_ctrl_attr(&pdev->dev);

	/* mtk loglevel */
	i2c_dev->log_level = MTK_I2C_LOG_NONE;
	i2c_dev->nak_log_level = MTK_I2C_LOG_ERR;

	/* create mtk_dbg sysfs group, if sysfs group creation failed,
	 * mtk_dbg function can not work but I2C bus still works.
	 */
	retval = sysfs_create_group(&pdev->dev.kobj, &mtk_dtv_i2c_attr_group);
	if (retval < 0)
		dev_err(&pdev->dev, "failed to create file group for loglevel. (%d)\n", retval);

	dev_info(&pdev->dev, "Init i2c done, bus number %d\n",
		 i2c_dev->adapter.nr);

	return 0;
}

static int dtv_i2c_remove(struct platform_device *pdev)
{
	struct dtv_i2c_dev *i2c_dev = platform_get_drvdata(pdev);
	int i;

	if (i2c_dev->irq > 0)
		disable_irq(i2c_dev->irq);

	i2c_del_adapter(&i2c_dev->adapter);

	/* disable all clks, include sw_en*/
	for (i = 0 ; i < i2c_dev->clk_num ; i++)
		clk_disable_unprepare(i2c_dev->clks[i]);

	device_remove_file(&pdev->dev, &dev_attr_byte_delay_count);
	device_remove_file(&pdev->dev, &dev_attr_clkl_count);
	device_remove_file(&pdev->dev, &dev_attr_clkh_count);
	device_remove_file(&pdev->dev, &dev_attr_oen_push);
	device_remove_file(&pdev->dev, &dev_attr_scl_stretch);
	device_remove_file(&pdev->dev, &dev_attr_latch_count);
	device_remove_file(&pdev->dev, &dev_attr_sda_count);
	device_remove_file(&pdev->dev, &dev_attr_stop_count);
	device_remove_file(&pdev->dev, &dev_attr_start_count);

	if (i2c_dev->support_dma)
		of_reserved_mem_device_release(&pdev->dev);

	sysfs_remove_group(&pdev->dev.kobj, &mtk_dtv_i2c_attr_group);

	return 0;
}

#ifdef CONFIG_PM
static int dtv_i2c_suspend_late(struct device *dev)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	int i;

	i2c_mark_adapter_suspended(&i2c_dev->adapter);

	if (i2c_dev->irq > 0)
		disable_irq(i2c_dev->irq);

	/* disable all clks, include sw_en*/
	for (i = 0 ; i < i2c_dev->clk_num ; i++)
		clk_disable_unprepare(i2c_dev->clks[i]);

	return 0;
}

static int dtv_i2c_resume_early(struct device *dev)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata(dev);
	int i, retval;

	/* enable all clks, include sw_en*/
	for (i = 0 ; i < i2c_dev->clk_num ; i++) {
		retval = clk_prepare_enable(i2c_dev->clks[i]);
		if (retval < 0) {
			dev_err(dev, "clk enable failed in #%d clks (%d)\n", i, retval);
			return retval;
		}
	}

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);
	_dtv_i2c_init(i2c_dev);
	if (i2c_dev->irq > 0)
		enable_irq(i2c_dev->irq);
	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	i2c_mark_adapter_resumed(&i2c_dev->adapter);

	return 0;
}

static const struct dev_pm_ops dtv_i2c_bus_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(dtv_i2c_suspend_late, dtv_i2c_resume_early)
};

#define MTK_I2C_BUS_PMOPS (&dtv_i2c_bus_pm_ops)
#else
#define MTK_I2C_BUS_PMOPS NULL
#endif

static const struct of_device_id dtv_i2c_of_match[] = {
	{.compatible = "mediatek,mt5896-i2c"},
	{},
};

MODULE_DEVICE_TABLE(of, dtv_i2c_of_match);

static struct platform_driver dtv_i2c_driver = {
	.probe = dtv_i2c_probe,
	.remove = dtv_i2c_remove,
	.driver = {
		.of_match_table = of_match_ptr(dtv_i2c_of_match),
		.name = "i2c-mt5896",
		.owner = THIS_MODULE,
		.pm = MTK_I2C_BUS_PMOPS,
#ifndef MODULE
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
#endif
	},
};

static int __init dtv_i2c_init_driver(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&dtv_i2c_driver);
}

module_init(dtv_i2c_init_driver);

static void __exit dtv_i2c_exit_driver(void)
{
	platform_driver_unregister(&dtv_i2c_driver);
}

module_exit(dtv_i2c_exit_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek MT5896 I2C Bus Driver");
MODULE_AUTHOR("Benjamin Lin");
