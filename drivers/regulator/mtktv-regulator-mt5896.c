// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/slab.h>
#include <linux/iio/consumer.h>
#define MT5896_ID_VCPU 0
#define MT5896_ID_VGPU 1
#define MT5896_MAX_REGULATOR 2
#define SAR_1_1_VOLTAGE              (1100000)
#define SAR_RESLOUTION               (1023)
struct iio_channel *CPUCorePower;
struct iio_channel *GPUCorePower;
/*
 * LOG
 */
//#define DEBUG_MSG
#define mt5896reg_error(fmt, args...) pr_err("[MT5896-regulator]: " fmt, ##args)
#ifdef DEBUG_MSG
#define mt5896reg_warning(fmt, args...)                                        \
	pr_warn("[MT5896-regulator]: " fmt, ##args)
#define mt5896reg_notice(fmt, args...)                                         \
	pr_info("[MT5896-regulator]: " fmt, ##args)
#define mt5896reg_info(fmt, args...) pr_info("[MT5896-regulator]: " fmt, ##args)
#define mt5896reg_debug(fmt, args...)                                          \
	pr_debug("[MT5896-regulator]: " fmt, ##args)
#else
#define mt5896reg_warning(fmt, args...)
#define mt5896reg_notice(fmt, args...)
#define mt5896reg_info(fmt, args...)
#define mt5896reg_debug(fmt, args...)
#endif

//#define CUSTOM_PARAM
#ifdef CUSTOM_PARAM
#define I2C_VCPU_DEVADDR 0xC6 // RT5758
#define I2C_VCPU_DATADDR 0x02 // RT5758
#define I2C_VCPU_MODADDR 0x03 // RT5758
#define I2C_VCPU_MODMASK 0X04 // RT5758
#define I2C_VCPU_MODPWM_EN 1  // RT5758
#define I2C_VCPU_VMIN 600     // unit: mV
#define I2C_VCPU_VMAX 1400    // unit: mV
#define I2C_VCPU_STEP 1000    // unit: 10uV

#define I2C_VGPU_DEVADDR 0xC8 // RT5090 Buck3
#define I2C_VGPU_DATADDR 0x05 // RT5090 Buck3
#define I2C_VGPU_MODADDR 0X06 // RT5090 Buck3
#define I2C_VGPU_MODMASK 0X10 // RT5090 Buck3
#define I2C_VGPU_MODPWM_EN 1  // RT5090 Buck3
#define I2C_VGPU_VMIN 600     // unit: mV
#define I2C_VGPU_VMAX 1400    // unit: mV
#define I2C_VGPU_STEP 1000    // unit: 10uV
#define I2C_CHANNEL_ID 4      // pdwnc
#else
struct mt_regulator_info {
	char devaddr;
	char dataddr;
	char modaddr;
	char modmask;
	char modepwm_en;
	unsigned int vmin;
	unsigned int vmax;
	unsigned int vstep;
	unsigned int channel_id;
};

struct mt_regulator_info mtreg_info[MT5896_MAX_REGULATOR] = {
	[MT5896_ID_VCPU] = {
		.devaddr = 0x64,
		.dataddr = 0x05,
		.modaddr = 0x06,
		.modmask = 0X10,
		.modepwm_en = 1,
		.vmin = 600,
		.vmax = 1400,
		.vstep = 1000,
		.channel_id = 4,
	},
	[MT5896_ID_VGPU] = {
		.devaddr = 0x64,
		.dataddr = 0x04,
		.modaddr = 0X06,
		.modmask = 0X10,
		.modepwm_en = 1,
		.vmin = 600,
		.vmax = 1400,
		.vstep = 1000,
		.channel_id = 4,
	},
};

#define I2C_VCPU_DEVADDR mtreg_info[MT5896_ID_VCPU].devaddr
#define I2C_VCPU_DATADDR mtreg_info[MT5896_ID_VCPU].dataddr
#define I2C_VCPU_MODADDR mtreg_info[MT5896_ID_VCPU].modaddr
#define I2C_VCPU_MODMASK mtreg_info[MT5896_ID_VCPU].modmask
#define I2C_VCPU_MODPWM_EN mtreg_info[MT5896_ID_VCPU].modepwm_en
#define I2C_VCPU_VMIN mtreg_info[MT5896_ID_VCPU].vmin
#define I2C_VCPU_VMAX mtreg_info[MT5896_ID_VCPU].vmax
#define I2C_VCPU_STEP mtreg_info[MT5896_ID_VCPU].vstep
#define I2C_CHANNEL_ID mtreg_info[MT5896_ID_VCPU].channel_id

#define I2C_VGPU_DEVADDR mtreg_info[MT5896_ID_VGPU].devaddr
#define I2C_VGPU_DATADDR mtreg_info[MT5896_ID_VGPU].dataddr
#define I2C_VGPU_MODADDR mtreg_info[MT5896_ID_VGPU].modaddr
#define I2C_VGPU_MODMASK mtreg_info[MT5896_ID_VGPU].modmask
#define I2C_VGPU_MODPWM_EN mtreg_info[MT5896_ID_VGPU].modepwm_en
#define I2C_VGPU_VMIN mtreg_info[MT5896_ID_VGPU].vmin
#define I2C_VGPU_VMAX mtreg_info[MT5896_ID_VGPU].vmax
#define I2C_VGPU_STEP mtreg_info[MT5896_ID_VGPU].vstep
#endif
#define SIF_CLK_DIV 0x43

#define STEP_OFFSET 0
/*
 * MT5896 regulators' information
 *
 * @desc: standard fields of regulator description.
 * @qi: Mask for query enable signal status of regulators
 * @vselon_reg: Register sections for hardware control mode of bucks
 * @vselctrl_reg: Register for controlling the buck control mode.
 * @vselctrl_mask: Mask for query buck's voltage control mode.
 */
struct mt5896_regulator_info {
	struct regulator_desc desc;
	u32 I2C_addr;
	unsigned int vcpuMode;
	unsigned int vgpuMode;
};

#define MT5896_BUCK(match, vreg, min, max, step, volt_ranges, _I2C_addr)       \
	[MT5896_ID_##vreg] = {                                                       \
		.desc =                                                                  \
		{                                                                    \
			.name = #vreg,                                                   \
			.of_match = of_match_ptr(match),                                 \
			.ops = &mt5896_volt_range_ops,                                   \
			.type = REGULATOR_VOLTAGE,                                       \
			.id = MT5896_ID_##vreg,                                          \
			.owner = THIS_MODULE,                                            \
			.n_voltages = (max - min) / step + 1,                            \
			.linear_ranges = volt_ranges,                                    \
			.n_linear_ranges = ARRAY_SIZE(volt_ranges),                      \
		},                                                                   \
		.I2C_addr = _I2C_addr,                                                   \
	}

static struct regulator_linear_range buck_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(600000, 0, (90000 / 1000), 10000),
};
static struct regulator_linear_range buck_volt_range2[] = {
	REGULATOR_LINEAR_RANGE(600000, 0, (90000 / 1000), 10000),
};

#define WAIT_I2C_READY (1)
#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)

static int _I2C_Xfer_Read_Cmd(u8 device_addr, u8 addr)
{
#if WAIT_I2C_READY
	u8 u1Dev;
	u16 u2ClkDiv;
	u32 u4Addr;
	u32 u4Count;
	u32 u4IsRead;
	int i4RetVal;
	//    u8* pu1Buf;
	//    u8* pu2Buf;
	u8 u1AddrNum;
	u8 u1ChannelId;
	u8 buf1[sizeof(u2ClkDiv) + sizeof(u1AddrNum) + sizeof(u4Addr)];
	struct i2c_msg xfer_msg[2];
	struct i2c_adapter *i2c_adapt;

	unsigned char data;

	u1AddrNum = 1;
	u4Addr = addr;
	u4Count = 1;
	u4IsRead = 1;

	u1ChannelId = I2C_CHANNEL_ID;
	u2ClkDiv = SIF_CLK_DIV;
	u1Dev = device_addr;

	memcpy(buf1, &u2ClkDiv, sizeof(u2ClkDiv)); // 2B clock div
	buf1[sizeof(u2ClkDiv)] = u1AddrNum;        // addr num
	memcpy(buf1 + sizeof(u2ClkDiv) + sizeof(u1AddrNum), &u4Addr,
			sizeof(u4Addr)); // 4B addr
	//    pu2Buf=pu1Buf = (u8*)kmalloc(u4Count, GFP_KERNEL);

	//    if (pu1Buf == NULL)
	//    {
	//        return -1;
	//    }
	//    memset((void*)pu1Buf, 0, u4Count);


	xfer_msg[0].addr = device_addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].buf = &addr;
	xfer_msg[0].flags = 0;
	xfer_msg[1].addr = device_addr;
	xfer_msg[1].len = 1;
	xfer_msg[1].buf = &data;
	xfer_msg[1].flags = I2C_M_RD;

	i2c_adapt = i2c_get_adapter(u1ChannelId);

	if (!i2c_adapt) {
		mt5896reg_error("can not get adapter for channel %d\n ", u1ChannelId);
		i4RetVal = -1;
	} else {
		i4RetVal = i2c_transfer(i2c_adapt, xfer_msg, ARRAY_SIZE(xfer_msg));
		if (i4RetVal < 0)
			mt5896reg_error("[%s](%d)i2c transfer fails\n", __func__, __LINE__);
		else
		i4RetVal = data;
		i2c_put_adapter(i2c_adapt);
	}
	//    kfree(pu2Buf);
	return i4RetVal;
#else
	return 0;
#endif
}

static int _I2C_Xfer_Write_Cmd(u8 device_addr, u8 addr, u8 data)
{
#if WAIT_I2C_READY
	u8 u1Dev;
	u16 u2ClkDiv;
	u32 u4Addr;
	u32 u4Count;
	u32 u4IsRead;
	int i4RetVal;
	//    u8* pu1Buf;
	//    u8* pu2Buf;
	u8 u1AddrNum;
	u8 u1ChannelId;
	u8 buf1[sizeof(u2ClkDiv) + sizeof(u1AddrNum) + sizeof(u4Addr)];
	struct i2c_msg xfer_msg;
	struct i2c_adapter *i2c_adapt;
	u8 reg_data[2];

	u1ChannelId = I2C_CHANNEL_ID;
	u2ClkDiv = SIF_CLK_DIV;
	u1Dev = device_addr;
	u1AddrNum = 1;
	u4Addr = addr;
	u4Count = 1;
	u4IsRead = 0;

	memcpy(buf1, &u2ClkDiv, sizeof(u2ClkDiv)); // 2B clock div
	buf1[sizeof(u2ClkDiv)] = u1AddrNum;        // addr num
	memcpy(buf1 + sizeof(u2ClkDiv) + sizeof(u1AddrNum), &u4Addr,
			sizeof(u4Addr)); // 4B addr

	//    pu2Buf=pu1Buf = (UINT8*)x_mem_alloc(u4Count);

	//    if (pu1Buf == NULL)
	//    {
	//        return -1;
	//    }
	// dev_addr, reg, reg_data
	// 0x64, 0x5, step_data
	reg_data[0] = addr;
	reg_data[1] = data;

	xfer_msg.addr = device_addr;
	xfer_msg.len = 2;
	xfer_msg.buf = reg_data;
	xfer_msg.flags = u4IsRead;

	i2c_adapt = i2c_get_adapter(u1ChannelId);

	if (!i2c_adapt) {
		mt5896reg_error("can not get adapter for channel %d\n ", u1ChannelId);
	} else {
		i4RetVal = i2c_transfer(i2c_adapt, &xfer_msg, 1);
		if (i4RetVal < 0)
			mt5896reg_error("[%s](%d)i2c transfer fails\n", __func__, __LINE__);
		i2c_put_adapter(i2c_adapt);
		if (i4RetVal > 0)
			mt5896reg_info("write successfully! byte count = %d\n", i4RetVal);
		else
			mt5896reg_info("write failed! return value = %d\n", i4RetVal);
	}
	//    x_mem_free(pu2Buf);
	return 0;
#else
	return 0;
#endif
}
#endif // !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)

static int mt5896_regulator_set_voltage(struct regulator_dev *rdev, int min_uV,
		int max_uV, unsigned int *selector)
{
	int i4RetVal = 0;
#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	u8 step;
	u32 VCPU_table[] = {
		I2C_VCPU_VMIN * 1000, I2C_VCPU_VMAX * 1000,
		I2C_VCPU_STEP * 10 + STEP_OFFSET,
	};
	u32 VGPU_table[] = {
		I2C_VGPU_VMIN * 1000, I2C_VGPU_VMAX * 1000,
		I2C_VGPU_STEP * 10 + STEP_OFFSET,
	};
	struct mt5896_regulator_info *info = rdev_get_drvdata(rdev);
	// (info->I2C_addr == I2C_VCPU_DEVADDR)
	if (strcmp(info->desc.name, "VCPU") == 0) {
		step = (min_uV - VCPU_table[0] + VCPU_table[2] - 1) / VCPU_table[2];
		i4RetVal = _I2C_Xfer_Write_Cmd((u8)info->I2C_addr, I2C_VCPU_DATADDR, step);
		mt5896reg_info("%s for VCPU addr=0x%x voltage step=0x%x\n", __func__,
				info->I2C_addr, step);
	} else {
		step = (min_uV - VGPU_table[0] + VGPU_table[2] - 1) / VGPU_table[2];
		i4RetVal = _I2C_Xfer_Write_Cmd((u8)info->I2C_addr, I2C_VGPU_DATADDR, step);
		mt5896reg_info("%s for VGPU addr=0x%x voltage step=0x%x\n", __func__,
				info->I2C_addr, step);
	}
#endif // !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	return i4RetVal;
}

static int mt5896_regulator_get_voltage(struct regulator_dev *rdev)
{
	struct mt5896_regulator_info *info = rdev_get_drvdata(rdev);
	int voltage = 0, saradc = 0;

	if (info == NULL) {
		mt5896reg_error("can not find regulator\n");
		return voltage;
	}

	if (strcmp(info->desc.name, "VCPU") == 0) {
		if (CPUCorePower == NULL) {
			mt5896reg_info("CPUCorePower is NULL\n");
			return voltage;
		}

		iio_read_channel_processed(CPUCorePower, &saradc);
		voltage = saradc * (SAR_1_1_VOLTAGE / SAR_RESLOUTION); // uV
		mt5896reg_info("%s for VCPU voltage = 0x%x\n", __func__, voltage);
	} else {
		if (GPUCorePower == NULL) {
			mt5896reg_info("GPUCorePower is NULL\n");
			return voltage;
		}
		iio_read_channel_processed(GPUCorePower, &saradc);
		voltage = saradc * (SAR_1_1_VOLTAGE / SAR_RESLOUTION); // uV
		mt5896reg_info("%s for VGPU voltage = 0x%x\n", __func__, voltage);
	}
	return voltage;
}

static int mt5896_regulator_set_mode(struct regulator_dev *rdev,
		unsigned int mode)
{

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	int i4RetVal = 0;
	u8 mode_data;
	struct mt5896_regulator_info *info = rdev_get_drvdata(rdev);

	if (strcmp(info->desc.name, "VCPU") == 0) { // (info->I2C_addr == I2C_VCPU_DEVADDR)
		mode_data = _I2C_Xfer_Read_Cmd((u8)info->I2C_addr, I2C_VCPU_MODADDR);
		mode_data &= ~I2C_VCPU_MODMASK;
		switch (mode) {
		case REGULATOR_MODE_FAST:
			mode_data = (I2C_VCPU_MODPWM_EN == 1) ? (mode_data | I2C_VCPU_MODMASK)
				: mode_data;
			info->vcpuMode = REGULATOR_MODE_FAST;
			break;
		case REGULATOR_MODE_NORMAL:
			mode_data = (I2C_VCPU_MODPWM_EN == 1) ? mode_data
				: (mode_data | I2C_VCPU_MODMASK);
			info->vcpuMode = REGULATOR_MODE_NORMAL;
			break;
		default:
			return 0;
		}
		i4RetVal =
			_I2C_Xfer_Write_Cmd((u8)info->I2C_addr, I2C_VCPU_MODADDR, mode_data);
	} else {
		mode_data = _I2C_Xfer_Read_Cmd((u8)info->I2C_addr, I2C_VGPU_MODADDR);
		mode_data &= ~I2C_VGPU_MODMASK;
		switch (mode) {
		case REGULATOR_MODE_FAST:
			mode_data = (I2C_VGPU_MODPWM_EN == 1) ? (mode_data | I2C_VGPU_MODMASK)
				: mode_data;
			info->vgpuMode = REGULATOR_MODE_FAST;
			break;
		case REGULATOR_MODE_NORMAL:
			mode_data = (I2C_VGPU_MODPWM_EN == 1) ? mode_data
				: (mode_data | I2C_VGPU_MODMASK);
			info->vgpuMode = REGULATOR_MODE_NORMAL;
			break;
		default:
			return 0;
		}
		i4RetVal =
			_I2C_Xfer_Write_Cmd((u8)info->I2C_addr, I2C_VGPU_MODADDR, mode_data);
	}

	mt5896reg_info("%s addr=0x%x voltage mode=0x%x\n", __func__, info->I2C_addr,
			mode_data);
#endif // !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	return 0;
}

static unsigned int mt5896_regulator_get_mode(struct regulator_dev *rdev)
{
	struct mt5896_regulator_info *info = rdev_get_drvdata(rdev);
	unsigned int mode = 0;

	if (info == NULL) {
		mt5896reg_error("can not find regulator\n");
		return mode;
	}

	if (strcmp(info->desc.name, "VCPU") == 0) { // (info->I2C_addr == I2C_VCPU_DEVADDR)
		mode = info->vcpuMode;
	} else {
		mode = info->vgpuMode;
	}
	return mode;
}

static struct regulator_ops mt5896_volt_range_ops = {
	//	.list_voltage = regulator_list_voltage_linear_range,
	//	.map_voltage = regulator_map_voltage_linear_range,
	.set_voltage = mt5896_regulator_set_voltage,
	.get_voltage = mt5896_regulator_get_voltage,
	.set_mode = mt5896_regulator_set_mode,
	.get_mode = mt5896_regulator_get_mode,
};

/* The array is indexed by id(MT5896_ID_XXX) */
struct mt5896_regulator_info mt5896_regulators[] = {
	MT5896_BUCK("buck_vcpu", VCPU, 600000, 1400000, 10000, buck_volt_range1, 0),
	MT5896_BUCK("buck_vgpu", VGPU, 600000, 1400000, 10000, buck_volt_range2, 0),
};

static int mt5896_regulator_probe(struct platform_device *pdev)
{
	struct regulator_config config = {};
	struct regulator_dev *rdev;
	struct regulation_constraints *c;
	struct device *dev = &pdev->dev;
	int i;

	mt5896_regulators[0].I2C_addr = I2C_VCPU_DEVADDR;
	mt5896_regulators[1].I2C_addr = I2C_VGPU_DEVADDR;

	CPUCorePower = devm_iio_channel_get(dev, "adc16");
	if (IS_ERR(CPUCorePower))
		return PTR_ERR(CPUCorePower);

	GPUCorePower = devm_iio_channel_get(dev, "adc20");
	if (IS_ERR(GPUCorePower))
		return PTR_ERR(GPUCorePower);

	for (i = 0; i < MT5896_MAX_REGULATOR; i++) {
		config.dev = &pdev->dev;
		config.driver_data = &mt5896_regulators[i];
		rdev = devm_regulator_register(&pdev->dev, &mt5896_regulators[i].desc,
				&config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to register %s\n",
					mt5896_regulators[i].desc.name);
			return PTR_ERR(rdev);
		}
		/* Constrain board-specific capabilities according to what
		 * this driver and the chip itself can actually do.
		 */
		c = rdev->constraints;
		c->valid_modes_mask |=
			REGULATOR_MODE_NORMAL | REGULATOR_MODE_STANDBY | REGULATOR_MODE_FAST;
		c->valid_ops_mask |= REGULATOR_CHANGE_MODE;
		if (i == MT5896_ID_VCPU)
			c->max_uV = I2C_VCPU_VMAX * 1000;
	}

	return 0;
}

static const struct platform_device_id mt5896_platform_ids[] = {
	{"mt5896-regulator", 0}, {/* sentinel */},
};
MODULE_DEVICE_TABLE(platform, mt5896_platform_ids);

static const struct of_device_id mt5896_of_match[] = {
	{
		.compatible = "mediatek,mt5896-regulator",
	},
	{/* sentinel */},
};
MODULE_DEVICE_TABLE(of, mt5896_of_match);

static struct platform_driver mt5896_regulator_driver = {
	.driver = {
		.name = "mt5896-regulator",
		.of_match_table = of_match_ptr(mt5896_of_match),
	},
	.probe = mt5896_regulator_probe,
	.id_table = mt5896_platform_ids,
};

module_platform_driver(mt5896_regulator_driver);

MODULE_AUTHOR("Chen Zhong <chen.zhong@mediatek.com>");
MODULE_DESCRIPTION("Regulator Driver for MediaTek MT6392 PMIC");
MODULE_LICENSE("GPL v2");
