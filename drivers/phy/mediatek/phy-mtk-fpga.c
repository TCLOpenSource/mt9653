// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

#include <dt-bindings/phy/phy.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include "../../usb/mtu3/mtu3.h"
#include "phy-mtk-fpga.h"

#ifdef CONFIG_MSTAR_ARM_BD_FPGA
#include <linux/clk.h>
#endif

/* ------------------------ I2C  IO API ------------------------------ */

static inline void i2c_dummy_delay(int count)
{
	udelay(count);
}

static void gpio_set_direction(void __iomem *port,
		enum i2c_pin pin, enum i2c_dir dir)
{
	void __iomem  *addr;
	u32 temp;

	addr = port + SSUSB_FPGA_I2C_OUT;
	temp = readl(addr);

	if (pin == I2C_SDA) {
		if (dir == I2C_OUTPUT)
			temp |= SSUSB_FPGA_I2C_SDA_OEN;
		else
			temp &= ~SSUSB_FPGA_I2C_SDA_OEN;
	} else {
		if (dir == I2C_OUTPUT)
			temp |= SSUSB_FPGA_I2C_SCL_OEN;
		else
			temp &= ~SSUSB_FPGA_I2C_SCL_OEN;
	}
	writel(temp, addr);
}

static void gpio_set_value(void __iomem *port,
		enum i2c_pin pin, bool value)
{
	void __iomem  *addr;
	u32 temp;

	addr = port + SSUSB_FPGA_I2C_OUT;
	temp = readl(addr);

	if (pin == I2C_SDA) {
		if (value)
			temp |= SSUSB_FPGA_I2C_SDA_OUT;
		else
			temp &= ~SSUSB_FPGA_I2C_SDA_OUT;
	} else {
		if (value)
			temp |= SSUSB_FPGA_I2C_SCL_OUT;
		else
			temp &= ~SSUSB_FPGA_I2C_SCL_OUT;
	}
	writel(temp, addr);
}

static int gpio_get_value(void __iomem *port, enum i2c_pin pin)
{
	void __iomem *addr;
	u32 temp;

	addr = port + SSUSB_FPGA_I2C_IN;
	temp = readl(addr);

	if (pin == I2C_SDA)
		temp &= SSUSB_FPGA_I2C_SDA_IN;
	else
		temp &= SSUSB_FPGA_I2C_SCL_IN;

	return !!temp;
}

static void i2c_stop(void __iomem *port)
{
	gpio_set_direction(port, I2C_SDA, I2C_OUTPUT);
	gpio_set_value(port, I2C_SCL, 0);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SDA, 0);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SDA, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_direction(port, I2C_SCL, I2C_INPUT);
	gpio_set_direction(port, I2C_SDA, I2C_INPUT);
}

/* Prepare the I2C_SDA and I2C_SCL for sending/receiving */
static void i2c_start(void __iomem *port)
{
	gpio_set_direction(port, I2C_SCL, I2C_OUTPUT);
	gpio_set_direction(port, I2C_SDA, I2C_OUTPUT);
	gpio_set_value(port, I2C_SDA, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SDA, 0);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 0);
	i2c_dummy_delay(I2C_DELAY);
}

/* return 0 --> ack */
static u32 i2c_send_byte(void __iomem *port, u8 data)
{
	int i, ack;

	gpio_set_direction(port, I2C_SDA, I2C_OUTPUT);

	for (i = 8; --i > 0;) {
		gpio_set_value(port, I2C_SDA, (data >> i) & 0x1);
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(port, I2C_SCL,  1);
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(port, I2C_SCL,  0);
		i2c_dummy_delay(I2C_DELAY);
	}
	gpio_set_value(port, I2C_SDA, (data >> i) & 0x1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL,  1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL,  0);
	i2c_dummy_delay(I2C_DELAY);

	gpio_set_value(port, I2C_SDA, 0);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_direction(port, I2C_SDA, I2C_INPUT);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 1);
	i2c_dummy_delay(I2C_DELAY);
	/* ack 1: error, 0:ok */
	ack = gpio_get_value(port, I2C_SDA);
	gpio_set_value(port, I2C_SCL, 0);
	i2c_dummy_delay(I2C_DELAY);

	return (ack == 1) ? PHY_FALSE : PHY_TRUE;
}

static void i2c_receive_byte(void __iomem *port, u8 *data, u8 ack)
{
	int i;
	u32 dataCache = 0;

	gpio_set_direction(port, I2C_SDA, I2C_INPUT);

	for (i = 8; --i >= 0;) {
		dataCache <<= 1;
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(port, I2C_SCL, 1);
		i2c_dummy_delay(I2C_DELAY);
		dataCache |= gpio_get_value(port, I2C_SDA);
		gpio_set_value(port, I2C_SCL, 0);
		i2c_dummy_delay(I2C_DELAY);
	}
	gpio_set_direction(port, I2C_SDA, I2C_OUTPUT);
	gpio_set_value(port, I2C_SDA, ack);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 1);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(port, I2C_SCL, 0);
	i2c_dummy_delay(I2C_DELAY);
	*data = (u8)dataCache;
}

static int i2c_write_reg(void __iomem *port, u8 i2c_addr, u8 addr, u8 data)
{
	int ack = 0;

	i2c_start(port);

	ack = i2c_send_byte(port, (i2c_addr << 1) & 0xff);
	if (ack)
		ack = i2c_send_byte(port, addr);
	else
		return PHY_FALSE;

	ack = i2c_send_byte(port, data);
	if (ack) {
		i2c_stop(port);
		return PHY_FALSE;
	} else {
		return PHY_TRUE;
	}
}

static int i2c_read_reg(void __iomem *port, u8 i2c_addr, u8 addr, u8 *data)
{
	int ack = 0;

	i2c_start(port);

	ack = i2c_send_byte(port, (i2c_addr << 1) & 0xff);
	if (ack)
		ack = i2c_send_byte(port, addr);
	else
		return PHY_FALSE;

	i2c_start(port);

	ack = i2c_send_byte(port, ((i2c_addr << 1) & 0xff) | 0x1);
	/* ack 0: ok, 1 error */
	if (ack)
		i2c_receive_byte(port, data, 1);
	else
		return PHY_FALSE;

	i2c_stop(port);
	return ack;
}

int phy_writeb(void __iomem *port, u8 i2c_addr, u8 addr, u8 value)
{
	i2c_write_reg(port, i2c_addr, addr, value);
	return PHY_TRUE;
}

u8 phy_readb(void __iomem *port, u8 i2c_addr,  u8 addr)
{
	u8 buf;
	int ret;

	ret = i2c_read_reg(port, i2c_addr, addr, &buf);
	if (ret == PHY_FALSE) {
		pr_err("Read failed(i2c_addr: %d, addr: 0x%x)\n",
				i2c_addr, addr);
		return ret;
	}

	return buf;
}

static int phy_writel(void __iomem *port, u8 i2c_addr, u32 addr, u32 data)
{
	u8 addr8;
	u8 data_0, data_1, data_2, data_3;

	addr8 = addr & 0xff;
	data_0 = data & 0xff;
	data_1 = (data >> 8) & 0xff;
	data_2 = (data >> 16) & 0xff;
	data_3 = (data >> 24) & 0xff;

	phy_writeb(port, i2c_addr, addr8, data_0);
	phy_writeb(port, i2c_addr, addr8 + 1, data_1);
	phy_writeb(port, i2c_addr, addr8 + 2, data_2);
	phy_writeb(port, i2c_addr, addr8 + 3, data_3);

	return 0;
}

static u32 phy_readl(void __iomem *port, u8 i2c_addr, u32 addr)
{
	u8 addr8;
	u32 data;

	addr8 = addr & 0xff;

	data = phy_readb(port, i2c_addr, addr8);
	data |= (phy_readb(port, i2c_addr, addr8 + 1) << 8);
	data |= (phy_readb(port, i2c_addr, addr8 + 2) << 16);
	data |= (phy_readb(port, i2c_addr, addr8 + 3) << 24);

	return data;
}

static u32 __maybe_unused phy_readlmsk(void __iomem *port,	u8 i2c_addr,
		u32 reg_addr32, u32 offset, u32 mask)
{
	u32 value;

	value = phy_readl(port, i2c_addr, reg_addr32);
	return ((value & mask) >> offset);
}

static int phy_writelmsk(void __iomem *port, u8 i2c_addr,
		u32 reg_addr32, u32 offset, u32 mask, u32 data)
{
	u32 cur_value;
	u32 new_value;

	cur_value = phy_readl(port, i2c_addr, reg_addr32);
	new_value = (cur_value & (~mask)) | ((data << offset) & mask);
	phy_writel(port, i2c_addr, reg_addr32, new_value);

	return 0;
}

/* ------------------------ SUB BOARD INIT API ------------------------------ */

unsigned int ssusb_read_phy_version(struct fpga_u3phy *u3phy,
		struct fpga_phy_instance *instance)
{
	void __iomem *i2c = instance->i2c_base;
	u32 version;

	phy_writeb(i2c, 0x60, 0xff, SSUSB_PHY_VERSION_BANK);

	version = phy_readl(i2c, 0x60, SSUSB_PHY_VERSION_ADDR);
	dev_info(u3phy->dev, "ssusb phy version: %x\n", version);

	return version;
}

static int a60931_u3phy_init(struct fpga_u3phy *u3phy,
		struct fpga_phy_instance *instance)
{
	void __iomem *i2c = instance->i2c_base;

	dev_err(u3phy->dev, "%s\n", __func__);

	/* 0xFC[31:24], Change bank address to 0 */
	phy_writeb(i2c, 0x60, 0xff, 0x0);
	/* 0x14[14:12],  RG_USB20_HSTX_SRCTRL, set U2 slew rate as 4 */
	phy_writelmsk(i2c, 0x60, 0x14, 12, GENMASK(14, 12), 0x4);
	/* 0x18[23:23],  RG_USB20_BC11_SW_EN, Disable BC 1.1 */
	phy_writelmsk(i2c, 0x60, 0x18, 23, BIT(23), 0x0);
	/* 0x68[18:18],  force_suspendm = 0 */
	phy_writelmsk(i2c, 0x60, 0x68, 18, BIT(18), 0x0);
	/* 0xFC[31:24], Change bank address to 0x30 */
	phy_writeb(i2c, 0x60, 0xff, 0x30);
	/* 0x04[29:29],  RG_VUSB10_ON, SSUSB 1.0V power ON */
	phy_writelmsk(i2c, 0x60, 0x04, 29, BIT(29), 0x1);
	/* 0x04[25:21], RG_SSUSB_XTAL_TOP_RESERVE */
	phy_writelmsk(i2c, 0x60, 0x04, 21, GENMASK(25, 21), 0x11);
	/* 0xFC[31:24], Change bank address to 0x40 */
	phy_writeb(i2c, 0x60, 0xff, 0x40);
	/* 0x38[15:0], DA_SSUSB_PLL_SSC_DELTA1 */
	/* fine tune SSC delta1 to let SSC min average ~0ppm */
	phy_writelmsk(i2c, 0x60, 0x38, 0, GENMASK(15, 0)<<0, 0x47);
	/* 0x40[31:16], DA_SSUSB_PLL_SSC_DELTA */
	/* fine tune SSC delta to let SSC min average ~0ppm */
	phy_writelmsk(i2c, 0x60, 0x40, 16, GENMASK(31, 16), 0x44);
	/* 0xFC[31:24], Change bank address to 0x30 */
	phy_writeb(i2c, 0x60, 0xff, 0x30);
	/* 0x14[15:0],  RG_SSUSB_PLL_SSC_PRD */
	/* fine tune SSC PRD to let SSC freq average 31.5KHz */
	phy_writelmsk(i2c, 0x60, 0x14, 0, GENMASK(15, 0), 0x190);
	/* 0xFC[31:24], Change bank address to 0x70 */
	phy_writeb(i2c, 0x70, 0xff, 0x70);
	/* 0x88[3:2], Pipe reset, clk driving current */
	phy_writelmsk(i2c, 0x70, 0x88, 2, GENMASK(3, 2), 0x1);
	/* 0x88[5:4], Data lane 0 driving current */
	phy_writelmsk(i2c, 0x70, 0x88, 4, GENMASK(5, 4), 0x1);
	/* 0x88[7:6], Data lane 1 driving current */
	phy_writelmsk(i2c, 0x70, 0x88, 6, GENMASK(7, 6), 0x1);
	/* 0x88[9:8], Data lane 2 driving current */
	phy_writelmsk(i2c, 0x70, 0x88, 8, GENMASK(9, 8), 0x1);
	/* 0x88[11:10], Data lane 3 driving current */
	phy_writelmsk(i2c, 0x70, 0x88, 10, GENMASK(11, 10), 0x1);
	/* 0x9C[4:0],  rg_ssusb_ckphase, PCLK phase 0x00~0x1F */
	phy_writelmsk(i2c, 0x70, 0x9c, 0, GENMASK(4, 0), 0x19);

	/* Set INTR & TX/RX Impedance */

	/* 0xFC[31:24], Change bank address to 0x30 */
	phy_writeb(i2c, 0x60, 0xff, 0x30);
	/* 0x00[26:26],  RG_SSUSB_INTR_EN */
	phy_writelmsk(i2c, 0x60, 0x00, 26, BIT(26), 0x1);
	/* 0x00[15:10],  RG_SSUSB_IEXT_INTR_CTRL, Set Iext R selection */
	phy_writelmsk(i2c, 0x60, 0x00, 10, GENMASK(15, 10), 0x26);
	/* 0xFC[31:24], Change bank address to 0x10 */
	phy_writeb(i2c, 0x60, 0xff, 0x10);
	/* 0x10[31:31],  rg_ssusb_force_tx_impsel,  enable */
	phy_writelmsk(i2c, 0x60, 0x10, 31, BIT(31), 0x1);
	/* 0x10[28:24],  rg_ssusb_tx_impsel, Set TX Impedance */
	phy_writelmsk(i2c, 0x60, 0x10, 24, GENMASK(28, 24), 0x10);
	/* 0x14[31:31],  rg_ssusb_force_rx_impsel, enable */
	phy_writelmsk(i2c, 0x60, 0x14, 31, BIT(31), 0x1);
	/* 0x14[28:24],  rg_ssusb_rx_impsel, Set RX Impedance */
	phy_writelmsk(i2c, 0x60, 0x14, 24, GENMASK(28, 24), 0x10);
	/* 0xFC[31:24], Change bank address to 0x00 */
	phy_writeb(i2c, 0x60, 0xff, 0x00);
	/* 0x00[05:05],  RG_USB20_INTR_EN, U2 INTR_EN */
	phy_writelmsk(i2c, 0x60, 0x00, 5, BIT(5), 0x1);
	/* 0x04[23:19],  RG_USB20_INTR_CAL, Set Iext R selection */
	phy_writelmsk(i2c, 0x60, 0x04, 19, GENMASK(23, 19), 0x14);

	return 0;
}

static void a60931_u3phy_set_pclk(struct fpga_u3phy *u3phy,
		struct fpga_phy_instance *instance, int pclk)
{
	void __iomem *i2c = instance->i2c_base;

	phy_writeb(i2c, 0x70, 0xff, 0x70);
	phy_writelmsk(i2c, 0x70, 0x9c, 0, GENMASK(4, 0), pclk);
}

static int a60810_u3phy_init(struct fpga_u3phy *u3phy,
		struct fpga_phy_instance *instance)
{
	void __iomem *i2c = instance->i2c_base;

	dev_err(u3phy->dev, "%s\n", __func__);

	phy_writeb(i2c, 0x60, 0xFF, 0x00);
	phy_writeb(i2c, 0x60, 0x05, 0x55);
	phy_writeb(i2c, 0x60, 0x18, 0x84);

	phy_writeb(i2c, 0x60, 0xFF, 0x10);
	phy_writeb(i2c, 0x60, 0x0A, 0x84);

	phy_writeb(i2c, 0x60, 0xFF, 0x40);
	phy_writeb(i2c, 0x60, 0x38, 0x46);
	phy_writeb(i2c, 0x60, 0x42, 0x40);
	phy_writeb(i2c, 0x60, 0x08, 0xAB);
	phy_writeb(i2c, 0x60, 0x09, 0x0C);
	phy_writeb(i2c, 0x60, 0x0C, 0x71);
	phy_writeb(i2c, 0x60, 0x0E, 0x4F);
	phy_writeb(i2c, 0x60, 0x10, 0xE1);
	phy_writeb(i2c, 0x60, 0x14, 0x5F);

	phy_writeb(i2c, 0x60, 0xFF, 0x60);
	phy_writeb(i2c, 0x60, 0x14, 0x03);

	phy_writeb(i2c, 0x60, 0xFF, 0x00);
	phy_writeb(i2c, 0x60, 0x6A, 0x04);
	phy_writeb(i2c, 0x60, 0x68, 0x08);
	phy_writeb(i2c, 0x60, 0x6C, 0x26);
	phy_writeb(i2c, 0x60, 0x6D, 0x36);

	dev_info(u3phy->dev, "%s\n", __func__);
	return 0;

}

static int a60855_u3phy_init(struct fpga_u3phy *u3phy,
		struct fpga_phy_instance *instance)
{
	void __iomem *i2c_port_base = instance->i2c_base;

	dev_err(u3phy->dev, "%s\n", __func__);

	phy_writeb(i2c_port_base, 0x10, 0xff, 0x02);
	phy_writelmsk(i2c_port_base, 0x10, 0x0, 1, 0x1<<1, 0x0);
	phy_writelmsk(i2c_port_base, 0x10, 0x34, 24, 0x1f<<24, 0x0);

	//SPHY calibration
	phy_writeb(i2c_port_base, 0x60, 0xff, 0x10);
	phy_writelmsk(i2c_port_base, 0x60, 0x14, 31, 0x1<<31, 0x1);
	phy_writelmsk(i2c_port_base, 0x60, 0x14, 24, 0x1f<<24, 0x5);

	phy_writeb(i2c_port_base, 0x60, 0xff, 0x10);
	phy_writelmsk(i2c_port_base, 0x60, 0x10, 31, 0x1<<31, 0x1);
	phy_writelmsk(i2c_port_base, 0x60, 0x10, 24, 0x1f<<24, 0x6);

	phy_writeb(i2c_port_base, 0x60, 0xff, 0x30);
	phy_writelmsk(i2c_port_base, 0x60, 0x0, 10, 0x3f<<10, 0xb);

	//U2 PHY init
	phy_writeb(i2c_port_base, 0x68, 0xff, 0x0);
	phy_writelmsk(i2c_port_base, 0x68, 0x0, 5, 0x1<<5, 0x1);
	phy_writelmsk(i2c_port_base, 0x68, 0x18, 23, 0x1<<23, 0x0);
	phy_writelmsk(i2c_port_base, 0x68, 0x68, 18, 0x1<<18, 0x0);
	phy_writelmsk(i2c_port_base, 0x68, 0x68, 3, 0x1<<3, 0x1);

	phy_writelmsk(i2c_port_base, 0x68, 0x60, 24, 0x1<<24, 0x0);
	phy_writelmsk(i2c_port_base, 0x68, 0x60, 25, 0x1<<25, 0x1);
	//fine tune HS disconnect threshold
	phy_writelmsk(i2c_port_base, 0x68, 0x18, 4, 0xf<<4, 0xf);
	//phy_writelmsk(i2c_port_base, 0x68, 0x4, 8, 0x7<<8, 0x0);
	//disable disconnect detect
	//phy_writelmsk(i2c_port_base, 0x68, 0x18, 12, 0x3<<12, 0x2);

	//U2 performance fine tune
	//I2C  0x68  0x18[03:00]  0x2   RW  RG_USB20_SQTH
	//phy_writelmsk(i2c_port_base, 0x68, 0x18, 0, 0xf, 0x2);

	return 0;
}

static int a60855_u3phy_set_pclk(struct fpga_u3phy *u3phy,
		struct fpga_phy_instance *instance, int pclk)
{
	void __iomem *i2c_port_base = instance->i2c_base;

	phy_writeb(i2c_port_base, 0x10, 0xff, 0x2);
	phy_writelmsk(i2c_port_base, 0x10, 0x34, 24, 0x1f<<24, pclk);

	return 0;
}

/* --------------------------- PHY DRIVER API ------------------------------- */

static int fpga_phy_init(struct phy *phy)
{
	struct fpga_phy_instance *instance = phy_get_drvdata(phy);
	struct fpga_u3phy *u3phy = dev_get_drvdata(phy->dev.parent);

	switch (instance->chip_id) {
	case PHY_TEST_CHIP_A60931:
		a60931_u3phy_init(u3phy, instance);
		a60931_u3phy_set_pclk(u3phy, instance, instance->u3_pclk);
		break;
	case PHY_TEST_CHIP_A60810:
		a60810_u3phy_init(u3phy, instance);
		break;
	case PHY_TEST_CHIP_A60855:
		a60855_u3phy_init(u3phy, instance);
		break;
	default:
		dev_err(u3phy->dev, "%s: incompatible PHY type:0x%x\n",
				__func__, instance->chip_id);
		return -EINVAL;
	}

	return 0;
}

int fpga_phy_set_pclk(struct phy *phy, int pclk)
{
	struct fpga_phy_instance *instance = phy_get_drvdata(phy);
	struct fpga_u3phy *u3phy = dev_get_drvdata(phy->dev.parent);

	if (pclk < 0)
		pclk = instance->u3_pclk;

	switch (instance->chip_id) {
	case PHY_TEST_CHIP_A60931:
		a60931_u3phy_set_pclk(u3phy, instance, pclk);
		break;
	case PHY_TEST_CHIP_A60855:
		a60855_u3phy_set_pclk(u3phy, instance, pclk);
		break;
	case PHY_TEST_CHIP_A60810:
		/* nothing to do */
		break;
	default:
		dev_err(u3phy->dev, "%s: incompatible PHY type:0x%x\n",
				__func__, instance->chip_id);
		return -EINVAL;
	}

	return 0;
}

static struct phy *fpga_phy_xlate(struct device *dev,
		struct of_phandle_args *args)
{
	struct fpga_u3phy *u3phy = dev_get_drvdata(dev);
	struct fpga_phy_instance *instance = NULL;
	struct device_node *phy_np = args->np;
	int index;

	if (args->args_count != 1) {
		dev_err(dev, "invalid number of cells in 'phy' property\n");
		return ERR_PTR(-EINVAL);
	}

	for (index = 0; index < u3phy->nphys; index++)
		if (phy_np == u3phy->phys[index]->phy->dev.of_node) {
			instance = u3phy->phys[index];
			break;
		}

	if (!instance) {
		dev_err(dev, "failed to find appropriate phy\n");
		return ERR_PTR(-EINVAL);
	}

	instance->type = args->args[0];
	if (!(instance->type == PHY_TYPE_USB2 ||
				instance->type == PHY_TYPE_USB3)) {
		dev_err(dev, "unsupported device type: %d\n", instance->type);
		return ERR_PTR(-EINVAL);
	}

	return instance->phy;
}

static const struct phy_ops fpga_u3phy_ops = {
	.init		= fpga_phy_init,
	.owner		= THIS_MODULE,
};

static const struct of_device_id fpga_u3phy_id_table[] = {
	{ .compatible = "mediatek,fpga-u3phy", },
	{ },
};
MODULE_DEVICE_TABLE(of, fpga_u3phy_id_table);

static void dbg_write_reg(struct fpga_phy_instance *instance, const char *buf)
{
	struct device *dev = instance->dev;
	u32 i2c_addr = 0;
	u32 addr = 0;
	u32 value = 0;
	u32 old_val;
	u32 new_val;
	u32 param;

	param = sscanf(buf, "%*s 0x%x 0x%x 0x%x", &i2c_addr, &addr, &value);
	/* DTV fix coverity */
	if (param != 3) {
		dev_err(dev, "%s invalid input (%s)\n", __func__, buf);
		return;
	}
	dev_info(dev, "params-%d (i2c_addr:%#x, addr:%#x, value:%#x)\n",
			param, i2c_addr, addr, value);

	old_val = phy_readb(instance->i2c_base, i2c_addr, addr);
	phy_writeb(instance->i2c_base, i2c_addr, addr, value);
	new_val = phy_readb(instance->i2c_base, i2c_addr, addr);
	dev_info(dev, "0x%2.2x: 0x%2.2x --> 0x%2.2x\n",
			addr, old_val, new_val);
}

static void dbg_read_reg(struct fpga_phy_instance *instance, const char *buf)
{
	struct device *dev = instance->dev;
	u32 i2c_addr = 0;
	u32 addr = 0;
	u32 value = 0;
	u32 param;

	param = sscanf(buf, "%*s 0x%x 0x%x", &i2c_addr, &addr);
	/* DTV fix coverity */
	if (param != 2) {
		dev_err(dev, "%s invalid input (%s)\n", __func__, buf);
		return;
	}
	dev_info(dev, "params-%d (i2c_addr: %#x, addr: %#x)\n",
			param, i2c_addr, addr);

	value = phy_readb(instance->i2c_base, i2c_addr, addr);
	dev_info(dev, "0x%2.2x: 0x%2.2x\n", addr, value);
}

static void dbg_set_pclk(struct fpga_phy_instance *instance, const char *buf)
{
	struct device *dev = instance->dev;
	int pclk;
	u32 param;

	param = sscanf(buf, "%*s 0x%x", &pclk);
	/* DTV fix coverity */
	if (param != 1) {
		dev_err(dev, "%s invalid input (%s)\n", __func__, buf);
		return;
	}
	dev_info(dev, "params-%d (pclk: %#x)\n", param, pclk);
	fpga_phy_set_pclk(instance->phy, pclk);
}

static int phy_reg_show(struct seq_file *sf, void *unused)
{
	struct fpga_phy_instance *instance = sf->private;

	seq_printf(sf, "usage: %x\n(echo r[w] i2c_addr reg [value])(HEX)\n"
			"e.g. echo r 0x60 0xff > reg\n",
			instance->chip_id);

	return 0;
}

static int phy_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, phy_reg_show, inode->i_private);
}

static ssize_t phy_reg_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *sf = file->private_data;
	struct fpga_phy_instance *instance = sf->private;
	char buf[128] = "";

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	switch (buf[0]) {
	case 'w':
		dbg_write_reg(instance, buf);
		break;
	case 'r':
		dbg_read_reg(instance, buf);
		break;
	case 'p':
		dbg_set_pclk(instance, buf);
		break;
	default:
		dev_err(instance->dev, "No such cmd\n");
	}

	return count;
}

static const struct file_operations phy_reg_fops = {
	.open = phy_reg_open,
	.write = phy_reg_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void create_debugfs_interface(struct fpga_u3phy *u3phy,
		struct fpga_phy_instance *instance)
{
	struct dentry *root;

	root = debugfs_create_dir(instance->name, u3phy->dbg_root);
	if (!root) {
		dev_err(u3phy->dev, "create debugfs root failed\n");
		return;
	}
	instance->dbg = root;

	debugfs_create_file("reg", 0644, root, instance, &phy_reg_fops);
}

#ifdef CONFIG_MSTAR_ARM_BD_FPGA
static int dtv_fpga_clk_init(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct clk *clk;
	int ret = 0;

	pr_err("%s clock init\n", __func__);
	clk = devm_clk_get(dev, "sys_ck");
	ret = clk_prepare_enable(clk);
	if (ret) {
		dev_err(dev, "failed to enable sys_ck\n");
		goto exit;
	}

	clk = devm_clk_get(dev, "ref_ck");
	ret = clk_prepare_enable(clk);
	if (ret) {
		dev_err(dev, "failed to enable ref_ck\n");
		goto exit;
	}

	clk = devm_clk_get(dev, "mcu_ck");
	ret = clk_prepare_enable(clk);
	if (ret) {
		dev_err(dev, "failed to enable mcu_ck\n");
		goto exit;
	}

	clk = devm_clk_get(dev, "dma_ck");
	ret = clk_prepare_enable(clk);
	if (ret) {
		dev_err(dev, "failed to enable dma_ck\n");
		goto exit;
	}
exit:
	return ret;
}
#endif

static int fpga_u3phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct phy_provider *provider;
	struct fpga_u3phy *u3phy;
	u32 ippc;
	int index;
	int ret;

	pr_err("%s\n", __func__);

#ifdef CONFIG_MSTAR_ARM_BD_FPGA
	dtv_fpga_clk_init(pdev);
#endif
	u3phy = devm_kzalloc(dev, sizeof(*u3phy), GFP_KERNEL);
	if (!u3phy)
		return -ENOMEM;

	u3phy->dbg_root = debugfs_create_dir("fpga_phy", NULL);
	if (!u3phy->dbg_root)
		return -ENODEV;

	u3phy->nphys = of_get_child_count(np);
	u3phy->phys = devm_kcalloc(dev, u3phy->nphys,
			sizeof(*u3phy->phys), GFP_KERNEL);
	if (!u3phy->phys)
		return -ENOMEM;

	u3phy->dev = dev;
	platform_set_drvdata(pdev, u3phy);

	ret = of_property_read_u32(np, "mediatek,ippc", &ippc);
	if (ret) {
		dev_err(dev, "Failed to parse ippc value\n");
		return ret;
	}
	u3phy->ippc_base = ioremap(ippc, SSUSB_IPPC_LEN);
	if (!u3phy->ippc_base) {
		dev_err(dev, "could not ioremap ippc regs\n");
		return -ENOMEM;
	}

	index = 0;
	for_each_child_of_node(np, child_np) {
		struct fpga_phy_instance *instance;
		struct phy *phy;

		instance = devm_kzalloc(dev, sizeof(*instance), GFP_KERNEL);
		if (!instance) {
			ret = -ENOMEM;
			goto put_child;
		}

		u3phy->phys[index] = instance;

		phy = devm_phy_create(dev, child_np, &fpga_u3phy_ops);
		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create phy\n");
			ret = PTR_ERR(phy);
			goto put_child;
		}

		of_property_read_u32(child_np, "port",
				&instance->port);
		of_property_read_u32(child_np, "pclk_phase",
				&instance->u3_pclk);
		of_property_read_u32(child_np, "chip-id",
				&instance->chip_id);

		instance->i2c_base = u3phy->ippc_base +
			SSUSB_FPGA_I2C_PORT_OFFSET(instance->port);
		instance->phy = phy;
		instance->chip_version = ssusb_read_phy_version(u3phy, instance);
		if (instance->chip_version) {
			if (instance->chip_version != PHY_TEST_CHIP_NONAME)
				/* this chip has a correct chip version */
				instance->chip_id = instance->chip_version;
			else if (instance->chip_id != instance->chip_version) {
				dev_warn(dev, "WARN: this test chip maybe uses a wrong chip id\n"
							"use dts chip_id:%x\n", instance->chip_id);
			}
		} else {
			dev_warn(dev, "ERROR: maybe phy DTB not connected or powered?\n");
		}
		snprintf(instance->name, NAME_LEN, "%x", instance->chip_id);
		instance->index = index;
		instance->dev = dev;
		phy_set_drvdata(phy, instance);
		create_debugfs_interface(u3phy, instance);
		index++;
		if (instance->chip_version)
			dev_info(dev, "sub-board: %s, port:%d, u3_pclk: %d\n",
				instance->name,	instance->port, instance->u3_pclk);
	}

	provider = devm_of_phy_provider_register(dev, fpga_phy_xlate);

	return PTR_ERR_OR_ZERO(provider);
put_child:
	of_node_put(child_np);
	return ret;
}

static struct platform_driver fpga_u3phy_driver = {
	.probe		= fpga_u3phy_probe,
	.driver		= {
		.name	= "fpga-u3phy",
		.of_match_table = fpga_u3phy_id_table,
	},
};

module_platform_driver(fpga_u3phy_driver);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_DESCRIPTION("MediaTek FPGA USB PHY driver");
MODULE_LICENSE("GPL v2");
