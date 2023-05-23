// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#include "i2c-mt5870.h"

//------------------------------------------------------------------------------
//  Local Defines
//------------------------------------------------------------------------------

#define KHZ(freq)               (1000 * freq)

struct dtv_i2c_clk_fld {
	u32 clk_freq_out;
	u8 t_high_cnt;
	u8 t_low_cnt;
};

static void __iomem *i2c_pm_base;

/**
 * struct dtv_i2c_dev - dtv i2c driver private data
 *
 * @dev: platform device of i2c bus
 * @adapter: i2c adapter of i2c subsystem
 * @clk: i2c module clock soource
 * @base: register bank base after ioremap
 * @irq: interrupt number of i2c bus
 * @completion: completion when waiting irq
 * @bus_speed: specified i2c bus speed
 * TODO: @bank_resource: resource of register bank, should remove after pinctrl
 *                     ready
 * TODO: @pad_mode: pad mux mode, should remove after pinctrl ready
 * @clk_fld: table for bus speed register setting
 * @start_cnt: SCL and SDA count for start
 * @stop_cnt: SCL and SDA count for stop
 * @sda_cnt: clock count between falling edge SCL and SDA
 * @latch_cnt: data latch timing
 * @scl_stretch: enable/disable scl clock stretching
 */
struct dtv_i2c_dev {
	struct device *dev;
	struct i2c_adapter adapter;
	struct clk *clk;
	void __iomem *base;
	int irq;
	struct completion completion;
	u32 bus_speed;
	struct resource *bank_resource;	/* TODO: remove after pinctrl ready */
	u32 pad_mode;		/* TODO: remove after pinctrl ready */
	struct dtv_i2c_clk_fld *clk_fld;
	u32 start_cnt;
	u32 stop_cnt;
	u32 sda_cnt;
	u32 latch_cnt;
	bool scl_stretch;
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

static inline void ___dtv_i2c_clear_int(struct dtv_i2c_dev *i2c_dev)
{
	writeb(readb(i2c_dev->base + REG_HWI2C_INT_CTL) | _INT_CTL,
	       i2c_dev->base + REG_HWI2C_INT_CTL);
}

static int ___dtv_i2c_wait_int(struct dtv_i2c_dev *i2c_dev)
{
	unsigned long time_left;

	time_left =
	    wait_for_completion_timeout(&i2c_dev->completion,
					HWI2C_HAL_WAIT_TIMEOUT);

	/* clear interrupt */
	___dtv_i2c_clear_int(i2c_dev);

	return (time_left) ? 0 : -ETIMEDOUT;
}

static inline int ___dtv_i2c_get_state(struct dtv_i2c_dev *i2c_dev)
{
	int cur_state =
	    readb(i2c_dev->base + REG_HWI2C_CUR_STATE) & _CUR_STATE_MSK;

	dev_dbg(i2c_dev->dev, "%s, state %d\n", __func__, cur_state);

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
	int count = HWI2C_HAL_WAIT_TIMEOUT;

	while (count > 0) {
		if (state == ___dtv_i2c_get_state(i2c_dev))
			break;
		udelay(1);
		count--;
	}

	return (count != 0) ? 0 : -ETIMEDOUT;
}

static int __dtv_i2c_send_start(struct dtv_i2c_dev *i2c_dev)
{
	int val;

	dev_dbg(i2c_dev->dev, "%s\n", __func__);

	reinit_completion(&i2c_dev->completion);

	val = readb(i2c_dev->base + REG_HWI2C_CMD_START) | _CMD_START;
	writeb(val, i2c_dev->base + REG_HWI2C_CMD_START);

	if (___dtv_i2c_wait_int(i2c_dev)) {
		dev_err(i2c_dev->dev, "i2c bus send start timeout (0)\n");
		return -ETIMEDOUT;
	}

	return ___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_WAIT);
}

static int __dtv_i2c_send_byte(struct dtv_i2c_dev *i2c_dev, u8 data)
{
	dev_dbg(i2c_dev->dev, "%s, 0x%02X\n", __func__, data);

	reinit_completion(&i2c_dev->completion);

	writeb(data, i2c_dev->base + REG_HWI2C_WDATA);
	if (___dtv_i2c_wait_int(i2c_dev)) {
		dev_err(i2c_dev->dev, "i2c bus send byte timeout (0)\n");
		return -ETIMEDOUT;
	}

	if (___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_WAIT)) {
		dev_err(i2c_dev->dev, "i2c bus get byte timeout (1)\n");
		return -ETIMEDOUT;
	}

	if (!(readb(i2c_dev->base + REG_HWI2C_WDATA_GET) & _WDATA_GET_ACKBIT)) {
		udelay(1);
		return 0;
	} else {
		return -EREMOTEIO;
	}
}

static int __dtv_i2c_get_byte(struct dtv_i2c_dev *i2c_dev, u8 *ptr,
			      bool ack_data)
{
	int val;

	dev_dbg(i2c_dev->dev, "%s, ack_data %d\n", __func__, ack_data);

	reinit_completion(&i2c_dev->completion);

	val = _RDATA_CFG_TRIG;
	if (!ack_data)
		val |= _RDATA_CFG_ACKBIT;
	writeb(val, i2c_dev->base + REG_HWI2C_RDATA_CFG);

	___dtv_i2c_wait_int(i2c_dev);

	if (___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_WAIT)) {
		dev_err(i2c_dev->dev, "i2c bus get byte timeout (0)\n");
		return -ETIMEDOUT;
	}

	*ptr = readb(i2c_dev->base + REG_HWI2C_RDATA);

	___dtv_i2c_clear_int(i2c_dev);
	if (___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_WAIT)) {
		dev_err(i2c_dev->dev, "i2c bus get byte timeout (1)\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int __dtv_i2c_send_stop(struct dtv_i2c_dev *i2c_dev)
{
	int val;

	dev_dbg(i2c_dev->dev, "%s\n", __func__);

	reinit_completion(&i2c_dev->completion);

	val = readb(i2c_dev->base + REG_HWI2C_CMD_STOP) | _CMD_STOP;
	writeb(val, i2c_dev->base + REG_HWI2C_CMD_STOP);

	if (___dtv_i2c_wait_int(i2c_dev)) {
		dev_err(i2c_dev->dev, "i2c bus send stop timeout\n");
		return -ETIMEDOUT;
	}

	return ___dtv_i2c_wait_state(i2c_dev, E_HAL_HWI2C_STATE_IDLE);
}

/*
 * Hardware uses the underlying formula to calculate time periods of
 * SCL clock cycle. Firmware uses some additional cycles excluded from the
 * below formula and it is confirmed that the time periods are within
 * specification limits.
 *
 * time of high period of SCL: t_high = (t_high_cnt * clk_div) / source_clock
 * time of low period of SCL: t_low = (t_low_cnt * clk_div) / source_clock
 * time of full period of SCL: t_cycle = (t_cycle_cnt * clk_div) / source_clock
 * clk_freq_out = t / t_cycle
 * source_clock = 19.2 MHz
 */
static const struct dtv_i2c_clk_fld dtv_i2c_clk_map[] = {
	{KHZ(25), 235, 237},
	{KHZ(50), 115, 117},
	{KHZ(100), 57, 59},
	{KHZ(200), 25, 27},
	{KHZ(300), 15, 17},
	{KHZ(400), 9, 13},
};

static int _dtv_i2c_clk_map_idx(struct dtv_i2c_dev *i2c_dev)
{
	int i;
	const struct dtv_i2c_clk_fld *itr = dtv_i2c_clk_map;

	for (i = 0; i < ARRAY_SIZE(dtv_i2c_clk_map); i++, itr++) {
		if (itr->clk_freq_out == i2c_dev->bus_speed) {
			i2c_dev->clk_fld = (struct dtv_i2c_clk_fld *)itr;
			return 0;
		}
	}
	return -EINVAL;
}

static int _dtv_i2c_init(struct dtv_i2c_dev *i2c_dev)
{
	int reg_val;

	/* TODO: Judgment pad mux registers according module base address */
	/* Remove pad mux setting after pinctrl ready */
	dev_dbg(i2c_dev->dev, "i2c_pm_base %tx\n", i2c_pm_base);
	if (i2c_dev->bank_resource->start == (resource_size_t) 0x1F223000) {
		reg_val = readb(CHIP_REG_HWI2C_MIIC0);
		reg_val &= ~CHIP_MIIC0_PAD_MSK;
		reg_val |= i2c_dev->pad_mode << 0;
		writeb(reg_val, CHIP_REG_HWI2C_MIIC0);
	} else if (i2c_dev->bank_resource->start ==
		   (resource_size_t) 0x1F223200) {
		reg_val = readb(CHIP_REG_HWI2C_MIIC1);
		reg_val &= ~CHIP_MIIC1_PAD_MSK;
		reg_val |= i2c_dev->pad_mode << 4;
		writeb(reg_val, CHIP_REG_HWI2C_MIIC1);
	} else if (i2c_dev->bank_resource->start ==
		   (resource_size_t) 0x1F223400) {
		reg_val = readb(CHIP_REG_HWI2C_MIIC2);
		reg_val &= ~CHIP_MIIC2_PAD_MSK;
		reg_val |= i2c_dev->pad_mode << 0;
		writeb(reg_val, CHIP_REG_HWI2C_MIIC2);
	} else if (i2c_dev->bank_resource->start ==
		   (resource_size_t) 0x1F223600) {
		reg_val = readb(CHIP_REG_HWI2C_DDCR);
		reg_val &= ~CHIP_DDCR_PAD_MSK;
		reg_val |= i2c_dev->pad_mode << 0;
		writeb(reg_val, CHIP_REG_HWI2C_DDCR);
	} else if (i2c_dev->bank_resource->start ==
		   (resource_size_t) 0x1F001600) {
		reg_val = readb(CHIP_REG_HWI2C_MIIC4);
		reg_val &= ~CHIP_MIIC4_PAD_MSK;
		reg_val |= i2c_dev->pad_mode << 6;
		writeb(reg_val, CHIP_REG_HWI2C_MIIC4);
	} else {
		dev_err(i2c_dev->dev, "Register resource %pr is invalid\n",
			i2c_dev->bank_resource);
		return -EINVAL;
	}

	/* bus speed timing setup */
	writew(i2c_dev->clk_fld->t_high_cnt, i2c_dev->base + REG_HWI2C_CKH_CNT);
	writew(i2c_dev->clk_fld->t_low_cnt, i2c_dev->base + REG_HWI2C_CKL_CNT);
	writew(i2c_dev->stop_cnt, i2c_dev->base + REG_HWI2C_STP_CNT);
	writew(i2c_dev->sda_cnt, i2c_dev->base + REG_HWI2C_SDA_CNT);
	writew(i2c_dev->start_cnt, i2c_dev->base + REG_HWI2C_STT_CNT);
	writew(i2c_dev->latch_cnt, i2c_dev->base + REG_HWI2C_LTH_CNT);

	/* clear interrupt */
	reg_val = readb(i2c_dev->base + REG_HWI2C_INT_CTL) | _INT_CTL;
	writeb(reg_val, i2c_dev->base + REG_HWI2C_INT_CTL);
	/* reset standard master iic */
	reg_val = readb(i2c_dev->base + REG_HWI2C_MIIC_CFG);
	writeb(reg_val | _MIIC_CFG_RESET, i2c_dev->base + REG_HWI2C_MIIC_CFG);
	writeb(reg_val & ~_MIIC_CFG_RESET, i2c_dev->base + REG_HWI2C_MIIC_CFG);
	/* configuration */
	reg_val = readb(i2c_dev->base + REG_HWI2C_MIIC_CFG);
	reg_val |= _MIIC_CFG_EN_INT;
	reg_val |= (i2c_dev->scl_stretch) ? _MIIC_CFG_EN_CLKSTR : 0;
	reg_val |= _MIIC_CFG_EN_FILTER;
	reg_val |= _MIIC_CFG_EN_PUSH1T;
	writeb(reg_val, i2c_dev->base + REG_HWI2C_MIIC_CFG);

	return 0;
}

static int _dtv_i2c_read_data(struct dtv_i2c_dev *i2c_dev, int addr, u8 *buf,
			      int len, bool stop)
{
	int retry = HWI2C_HAL_RETRY_TIMES;
	int i;
	u8 *ptr;

	while (retry--) {
		ptr = buf;

		if (__dtv_i2c_send_start(i2c_dev)) {
			dev_dbg(i2c_dev->dev, "%s Start condition failed\n",
				__func__);
			__dtv_i2c_send_stop(i2c_dev);
			continue;
		}

		udelay(5);

		if (__dtv_i2c_send_byte(i2c_dev, (addr << 1) | 1)) {
			dev_dbg(i2c_dev->dev, "%s Chip address failed\n",
				__func__);
			__dtv_i2c_send_stop(i2c_dev);
			continue;
		}

		for (i = 0; i < len; i++, ptr++) {
			if (__dtv_i2c_get_byte(i2c_dev, ptr, i < (len - 1))) {
				dev_dbg(i2c_dev->dev, "%s Data stage failed\n",
					__func__);
				break;
			}
		}

		if (i < len) {
			// data stage not finish well
			__dtv_i2c_send_stop(i2c_dev);
			continue;
		}

		if (stop)
			__dtv_i2c_send_stop(i2c_dev);

		return len;
	}

	return -EREMOTEIO;
}

static int _dtv_i2c_write_data(struct dtv_i2c_dev *i2c_dev, int addr, u8 *buf,
			       int len, bool stop)
{
	int retry = HWI2C_HAL_RETRY_TIMES;
	int i;
	u8 *ptr;

	while (retry--) {
		ptr = buf;

		if (__dtv_i2c_send_start(i2c_dev)) {
			dev_dbg(i2c_dev->dev, "%s Start condition failed\n",
				__func__);
			__dtv_i2c_send_stop(i2c_dev);
			continue;
		}

		udelay(5);

		if (__dtv_i2c_send_byte(i2c_dev, (addr << 1))) {
			dev_dbg(i2c_dev->dev, "%s Chip address failed\n",
				__func__);
			__dtv_i2c_send_stop(i2c_dev);
			continue;
		}

		for (i = 0; i < len; i++, ptr++) {
			if (__dtv_i2c_send_byte(i2c_dev, *ptr)) {
				dev_dbg(i2c_dev->dev, "%s Data stage failed\n",
					__func__);
				break;
			}
		}

		if (i < len) {
			// data stage not finish well
			__dtv_i2c_send_stop(i2c_dev);
			continue;
		}

		if (stop)
			__dtv_i2c_send_stop(i2c_dev);

		usleep_range(60, 120);

		return len;
	}

	return -EREMOTEIO;
}

static int _dtv_i2c_parse_dt(struct dtv_i2c_dev *i2c_dev)
{
	if (!i2c_dev->dev->of_node)
		return -ENXIO;

	/* TODO: remove pad_mode after pinctrl ready */
	/* Pad mode */
	if (device_property_read_u32
	    (i2c_dev->dev, "pad-mode", &i2c_dev->pad_mode)) {
		dev_err(i2c_dev->dev,
			"Can not get property \"pad-mode\" in dts\n");
		return -ENXIO;
	}
	dev_dbg(i2c_dev->dev, "PAD mode %d\n", i2c_dev->pad_mode);

	/* start count */
	if (device_property_read_u32
	    (i2c_dev->dev, "start-count", &i2c_dev->start_cnt)) {
		dev_info(i2c_dev->dev,
			 "Start count not specified, default to 73.\n");
		i2c_dev->start_cnt = 73;
	}

	/* stop count */
	if (device_property_read_u32
	    (i2c_dev->dev, "stop-count", &i2c_dev->stop_cnt)) {
		dev_info(i2c_dev->dev,
			 "Stop count not specified, default to 73.\n");
		i2c_dev->stop_cnt = 73;
	}

	/* sda count */
	if (device_property_read_u32
	    (i2c_dev->dev, "sda-count", &i2c_dev->sda_cnt)) {
		dev_info(i2c_dev->dev,
			 "Sda count not specified, default to 5.\n");
		i2c_dev->sda_cnt = 5;
	}

	/* latch count */
	if (device_property_read_u32
	    (i2c_dev->dev, "data-latch-count", &i2c_dev->latch_cnt)) {
		dev_info(i2c_dev->dev,
			 "Latch count not specified, default to 5.\n");
		i2c_dev->latch_cnt = 5;
	}

	/* scl stretch */
	i2c_dev->scl_stretch =
	     device_property_read_bool(i2c_dev->dev, "scl-stretch");


	dev_dbg(i2c_dev->dev,
		"start count %d, stop count %d, sda count %d, latch count %d, scl stretch %d\n",
		i2c_dev->start_cnt, i2c_dev->stop_cnt, i2c_dev->sda_cnt,
		i2c_dev->latch_cnt, i2c_dev->scl_stretch);

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

		dev_dbg(i2c_dev->dev, "Set start count to %ld\n", value);
		writew(value, i2c_dev->base + REG_HWI2C_STT_CNT);
		i2c_dev->start_cnt = readw(i2c_dev->base + REG_HWI2C_STT_CNT);

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

	i2c_dev->start_cnt = readw(i2c_dev->base + REG_HWI2C_STT_CNT);
	dev_dbg(i2c_dev->dev, "Start count %d\n", i2c_dev->start_cnt);
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

		dev_dbg(i2c_dev->dev, "Set stop count to %ld\n", value);
		writew(value, i2c_dev->base + REG_HWI2C_STP_CNT);
		i2c_dev->stop_cnt = readw(i2c_dev->base + REG_HWI2C_STP_CNT);

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

	i2c_dev->stop_cnt = readw(i2c_dev->base + REG_HWI2C_STP_CNT);
	dev_dbg(i2c_dev->dev, "Stop count %d\n", i2c_dev->stop_cnt);
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

		dev_dbg(i2c_dev->dev, "Set sda count to %ld\n", value);
		writew(value, i2c_dev->base + REG_HWI2C_SDA_CNT);
		i2c_dev->sda_cnt = readw(i2c_dev->base + REG_HWI2C_SDA_CNT);

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

	i2c_dev->sda_cnt = readw(i2c_dev->base + REG_HWI2C_SDA_CNT);
	dev_dbg(i2c_dev->dev, "Sda count %d\n", i2c_dev->sda_cnt);
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

		dev_dbg(i2c_dev->dev, "Set latch count to %ld\n", value);
		writew(value, i2c_dev->base + REG_HWI2C_LTH_CNT);
		i2c_dev->latch_cnt = readw(i2c_dev->base + REG_HWI2C_LTH_CNT);

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

	i2c_dev->latch_cnt = readw(i2c_dev->base + REG_HWI2C_LTH_CNT);
	dev_dbg(i2c_dev->dev, "Latch count %d\n", i2c_dev->latch_cnt);
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
	uint8_t reg_val;

	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	reg_val = readb(i2c_dev->base + REG_HWI2C_MIIC_CFG);

	if (reg_val & _MIIC_CFG_EN_CLKSTR)
		i2c_dev->scl_stretch = true;
	else
		i2c_dev->scl_stretch = false;

	dev_dbg(i2c_dev->dev, "clock stretch %d\n", i2c_dev->scl_stretch);
	str += scnprintf(str, end - str, "%d\n", i2c_dev->scl_stretch);

	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);

	return (str - buf);
}

DEVICE_ATTR_RO(scl_stretch);

static irqreturn_t dtv_i2c_interrupt(int irq, void *dev)
{
	struct dtv_i2c_dev *i2c_dev = dev_get_drvdata((struct device *)dev);
	int intr_reg = readb(i2c_dev->base + REG_HWI2C_INT_CTL);

	/* TODO: Also need handle interrupt for I2C DMA mode transfer */
	if (intr_reg & _INT_CTL) {
		writeb(_INT_CTL, i2c_dev->base + REG_HWI2C_INT_CTL);
		dev_dbg(i2c_dev->dev, "I2C %d int status rised\n",
			i2c_dev->adapter.nr);
		complete(&i2c_dev->completion);
		return IRQ_HANDLED;
	}

	dev_err(i2c_dev->dev, "Enter I2C %d isr with int status not rised\n",
		i2c_dev->adapter.nr);
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
		dev_dbg(i2c_dev->dev, "processing message %d:\n", i);
		/* do Read/Write */
		/* NOTICE: place stop at the end of the last msg */
		if (msgs[i].flags & I2C_M_RD) {
			/* TODO: implement I2C DMA mode read transfer */
			ret =
			    _dtv_i2c_read_data(i2c_dev, msgs[i].addr,
					       msgs[i].buf, msgs[i].len,
					       i == (num - 1));
			dev_dbg(i2c_dev->dev,
				"i2c received from slave %u result %d\n",
				(u32) msgs[i].addr, ret);
		} else {
			/* TODO: implement I2C DMA mode write transfer */
			ret =
			    _dtv_i2c_write_data(i2c_dev, msgs[i].addr,
						msgs[i].buf, msgs[i].len,
						i == (num - 1));
			dev_dbg(i2c_dev->dev,
				"i2c sent to slave %u result %d\n",
				(u32) msgs[i].addr, ret);
		}
	}

	return (ret > 0) ? i : ret;
}

static u32 dtv_i2c_func(struct i2c_adapter *adap)
{
	/* return i2c bus driver FUNCTIONALITY CONSTANTS
	 * I2C_FUNC_I2C        : Plain i2c-level commands (Pure SMBus adapters
	 *                       typically can not do these)
	 * I2C_FUNC_SMBUS_EMUL : Handles all SMBus commands that can be
	 *                       emulated by a real I2C adapter (using
	 *                       the transparent emulation layer)
	 *
	 * text copy from file : Documentation/i2c/functionality
	 */
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
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
	u32 tmp[2];

	dev_info(&pdev->dev, "%s\n", __func__);

	/* init dtv_i2c_dev device struct */
	i2c_dev =
	    devm_kzalloc(&pdev->dev, sizeof(struct dtv_i2c_dev), GFP_KERNEL);
	if (IS_ERR(i2c_dev)) {
		dev_err(&pdev->dev, "unable to allocate memory (%ld)\n",
			PTR_ERR(i2c_dev));
		return PTR_ERR(i2c_dev);
	}

	platform_set_drvdata(pdev, i2c_dev);

	init_completion(&i2c_dev->completion);
	i2c_dev->dev = &pdev->dev;

	/* I2C controllor registers base and range */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	/* TODO: remove bank_resource after pinctrl ready */
	dev_dbg(i2c_dev->dev, "resource %pr\n", res);
	i2c_dev->bank_resource = res;

	i2c_dev->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2c_dev->base)) {
		dev_err(&pdev->dev, "failed to map ioresource (%ld)\n",
			PTR_ERR(i2c_dev->base));
		return PTR_ERR(i2c_dev->base);
	}
	dev_dbg(i2c_dev->dev, "reg_base %p\n", i2c_dev->base);

	if (i2c_pm_base == NULL) {
		i2c_pm_base = of_iomap(pdev->dev.of_node, 1);
		if (!i2c_pm_base) {
			dev_err(&pdev->dev, "failed to map i2c_pm_base (%ld)\n",
					PTR_ERR(i2c_pm_base));
			dev_err(&pdev->dev, "node:%s reg base error\n", pdev->dev.of_node->name);
			return -EINVAL;
		}
		of_property_read_u32_array(pdev->dev.of_node, "reg", tmp, 2);
		dev_info(&pdev->dev, "%s reg base : 0x%x -> 0x%lx\n",
				pdev->dev.of_node->name, tmp[1], i2c_pm_base);
	}

	/* module clock source */
	/* TODO: clock control by using clk framework */
	/*
	 *i2c_dev->clk = devm_clk_get(&pdev->dev, NULL);
	 *if (IS_ERR(i2c_dev->clk)) {
	 *	dev_err(&pdev->dev, "failed to get clock (%d)\n",
	 *		PTR_ERR(i2c_dev->clk));
	 *	return PTR_ERR(i2c_dev->clk);
	 *}
	 *retval = clk_prepare_enable(i2c_dev->clk);
	 *if (retval < 0) {
	 *	dev_err(&pdev->dev, "unable to enable clock (%d)\n", retval);
	 *	return retval;
	 *}
	 */

	/* interrupt */
	retval = platform_get_irq(pdev, 0);
	if (retval < 0) {
		dev_err(&pdev->dev,
			"no irq resource defined, check DTS file\n");
		return retval;
	}
	i2c_dev->irq = retval;
	dev_dbg(i2c_dev->dev, "irq %d\n", retval);
	retval =
	    devm_request_irq(&pdev->dev, i2c_dev->irq, dtv_i2c_interrupt,
			     IRQF_TRIGGER_NONE, "dtv_i2c", &pdev->dev);
	if (retval) {
		dev_err(&pdev->dev, "Could not request IRQ\n");
		return retval;
	}

	/* bus speed */
	retval =
	    device_property_read_u32(&pdev->dev, "clock-frequency",
				     &i2c_dev->bus_speed);
	if (retval) {
		dev_info(&pdev->dev,
			 "Bus frequency not specified, default to 100kHz.\n");
		i2c_dev->bus_speed = KHZ(100);
	}
	retval = _dtv_i2c_clk_map_idx(i2c_dev);
	if (retval) {
		dev_err(&pdev->dev, "Invalid clk frequency %d Hz\n",
			i2c_dev->bus_speed);
		return retval;
	}

	/* bus number */
	retval =
	    device_property_read_u32(&pdev->dev, "bus-number",
				     &i2c_dev->adapter.nr);
	if (retval) {
		dev_info(&pdev->dev,
			 "Bus number not specified, dynamic assign.\n");
		i2c_dev->adapter.nr = -1;
	}

	/* parse proprietary i2c info */
	retval = _dtv_i2c_parse_dt(i2c_dev);
	if (retval < 0) {
		dev_err(&pdev->dev, "unable to parse device tree\n");
		return retval;
	}

	/* module initial */
	retval = _dtv_i2c_init(i2c_dev);
	if (retval) {
		dev_err(&pdev->dev, "failed to init i2c");
		return retval;
	}

	/* register i2c adapter and bus algorithm */
	i2c_dev->adapter.owner = THIS_MODULE;
	i2c_dev->adapter.class = I2C_CLASS_HWMON;
	scnprintf(i2c_dev->adapter.name, sizeof(i2c_dev->adapter.name),
		  "mtk dtv I2C adapter");
	i2c_dev->adapter.dev.parent = &pdev->dev;
	i2c_dev->adapter.algo = &dtv_i2c_algo;
	i2c_dev->adapter.dev.of_node = pdev->dev.of_node;
	i2c_set_adapdata(&i2c_dev->adapter, i2c_dev);
	retval = i2c_add_numbered_adapter(&i2c_dev->adapter);
	if (retval) {
		dev_err(&pdev->dev, "failed to add i2c adapter (%d)\n", retval);
		return retval;
	}

	/* create sysfs file for proprietary control parameter */
	retval = device_create_file(&pdev->dev, &dev_attr_start_count);
	if (retval)
		dev_err(&pdev->dev,
			"can not create attribute file \"start_count\"\n");
	retval = device_create_file(&pdev->dev, &dev_attr_stop_count);
	if (retval)
		dev_err(&pdev->dev,
			"can not create attribute file \"stop_count\"\n");
	retval = device_create_file(&pdev->dev, &dev_attr_sda_count);
	if (retval)
		dev_err(&pdev->dev,
			"can not create attribute file \"sda_count\"\n");
	retval = device_create_file(&pdev->dev, &dev_attr_latch_count);
	if (retval)
		dev_err(&pdev->dev,
			"can not create attribute file \"latch_count\"\n");
	retval = device_create_file(&pdev->dev, &dev_attr_scl_stretch);
	if (retval)
		dev_err(&pdev->dev,
			"can not create attribute file \"scl_stretch\"\n");

	dev_info(&pdev->dev, "Init i2c done, bus number %d\n",
		 i2c_dev->adapter.nr);

	return 0;
}

static int dtv_i2c_remove(struct platform_device *pdev)
{
	struct dtv_i2c_dev *i2c_dev = platform_get_drvdata(pdev);

	i2c_del_adapter(&i2c_dev->adapter);
	device_remove_file(&pdev->dev, &dev_attr_scl_stretch);
	device_remove_file(&pdev->dev, &dev_attr_latch_count);
	device_remove_file(&pdev->dev, &dev_attr_sda_count);
	device_remove_file(&pdev->dev, &dev_attr_stop_count);
	device_remove_file(&pdev->dev, &dev_attr_start_count);

	return 0;
}

#ifdef CONFIG_PM
static int dtv_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* TODO: clock control by using clk framework */
	/*
	 *struct dtv_i2c_dev *i2c_dev = platform_get_drvdata(pdev);
	 *
	 *clk_disable_unprepare(i2c_dev->clk);
	 */
	return 0;
}

static int dtv_i2c_resume(struct platform_device *pdev)
{
	struct dtv_i2c_dev *i2c_dev = platform_get_drvdata(pdev);

	/* TODO: clock control by using clk framework */
	/*
	 *clk_prepare_enable(i2c_dev->clk);
	 */
	i2c_lock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);
	_dtv_i2c_init(i2c_dev);
	i2c_unlock_bus(&i2c_dev->adapter, I2C_LOCK_ROOT_ADAPTER);
	return 0;
}
#endif

static const struct of_device_id dtv_i2c_of_match[] = {
	{.compatible = "mediatek,mt5870-i2c"},
	{},
};

MODULE_DEVICE_TABLE(of, dtv_i2c_of_match);

static struct platform_driver dtv_i2c_driver = {
	.probe = dtv_i2c_probe,
	.remove = dtv_i2c_remove,
#ifdef CONFIG_PM
	.suspend = dtv_i2c_suspend,
	.resume = dtv_i2c_resume,
#endif
	.driver = {
		   .of_match_table = of_match_ptr(dtv_i2c_of_match),
		   .name = "mt5870-i2c",
		   .owner = THIS_MODULE,
		   },
};

static int __init dtv_i2c_init_driver(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&dtv_i2c_driver);
}

subsys_initcall(dtv_i2c_init_driver);

static void __exit dtv_i2c_exit_driver(void)
{
	platform_driver_unregister(&dtv_i2c_driver);
}

module_exit(dtv_i2c_exit_driver);

MODULE_AUTHOR("MediaTek");
MODULE_DESCRIPTION("MTK DTV I2C driver");
MODULE_LICENSE("GPL");
