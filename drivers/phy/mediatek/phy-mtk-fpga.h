/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author:
 *  Chunfeng.Yun <chunfeng.yun@mediatek.com>
 */

#ifndef _MTU3_PHY_H_
#include <linux/phy/phy.h>
#define _MTU3_PHY_H_
#define SSUSB_IPPC_LEN				0x100
#define SSUSB_FPGA_I2C_PORT_SIZE		0x8
#define SSUSB_FPGA_I2C_PORT_OFFSET(x)	\
	(0xd0 + (SSUSB_FPGA_I2C_PORT_SIZE) * (x))

#define SSUSB_PHY_VERSION_BANK	0x20
#define SSUSB_PHY_VERSION_ADDR	0xe4


#define PHY_TRUE 1
#define PHY_FALSE 0

#define SSUSB_FPGA_I2C_OUT	0x0
#define SSUSB_FPGA_I2C_SDA_OUT	BIT(0)
#define SSUSB_FPGA_I2C_SDA_OEN	BIT(1)
#define SSUSB_FPGA_I2C_SCL_OUT	BIT(2)
#define SSUSB_FPGA_I2C_SCL_OEN	BIT(3)

#define SSUSB_FPGA_I2C_IN	0x4
#define SSUSB_FPGA_I2C_SDA_IN	BIT(0)
#define SSUSB_FPGA_I2C_SCL_IN	BIT(1)

#define I2C_DELAY 1

enum i2c_dir {
	I2C_INPUT = 0,
	I2C_OUTPUT,
};

enum i2c_pin {
	I2C_SDA = 0,
	I2C_SCL,
};

#define PHY_TEST_CHIP_A60931 0xa60931a
#define PHY_TEST_CHIP_A60810 0xa60810a
#define PHY_TEST_CHIP_A60855 0xa60855a
/* some test chip got default name without changed to correct one*/
#define PHY_TEST_CHIP_NONAME 0xd60802a
#define NAME_LEN 8

struct fpga_phy_instance {
	struct phy *phy;
	void __iomem *i2c_base;
	struct dentry *dbg;
	struct device *dev;
	u32 index;
	u32 port;
	u32 u3_pclk;
	/*
	 * chip id: assigned from dts, determines which phy init to be called
	 * chip version: read from PHYD version register, sometimes mismatches
	 *				with its real chip id
	 */
	u32 chip_id;
	u32 chip_version;
	char name[NAME_LEN];
	u8 type;
};

struct fpga_u3phy {
	struct device *dev;
	void __iomem *ippc_base;
	struct dentry *dbg_root;
	struct fpga_phy_instance **phys;
	int nphys;
};

int fpga_phy_set_pclk(struct phy *phy, int pclk);
int phy_writeb(void __iomem *port, u8 i2c_addr, u8 addr, u8 value);
u8 phy_readb(void __iomem *port, u8 i2c_addr,  u8 addr);

#endif
