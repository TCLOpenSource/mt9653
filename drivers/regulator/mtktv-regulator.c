// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/ioport.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
//#include <mach/io.h>


/*********************************************************************
 *                         External Functions                        *
 *********************************************************************/


/*********************************************************************
 *                         Private Structure                         *
 *********************************************************************/

/*********************************************************************
 *                         Private Macro                             *
 *********************************************************************/
static void __iomem *regulator_pm_base;
#define BASE_ADDRESS                    (regulator_pm_base)

#define regulator_writew(data, addr) \
	writew(data, (void __iomem *)(BASE_ADDRESS + (addr << 1)))

#define regulator_readw(addr) \
	readw((void __iomem *)(BASE_ADDRESS + (addr << 1)))

#define PRADO_ID        (0x9A)
#define PRADO_DEFAULT   (100)
#define PRADO_SHIFT     (68)
#define PRADO_STEP      (3)

#define SAR_DEALY_US    (500)

#define IIC_SUCCESS     (0)
#define IIC_FAIL        (1)
/*********************************************************************
 *                         Private Data                              *
 *********************************************************************/
DEFINE_MUTEX(INIT_MUTEX);

static u8 regulator_init;

// prado iic client
struct i2c_client *iic_client;
/*********************************************************************
 *                          Local Function                          *
 *********************************************************************/
static int cpu_get_voltage_bysar(void)
{
	return 0;
}

static int cpu_set_voltage(int power)
{
	u32 cur = 0;
	u32 reg_cur, reg_target = 0;
	int ret = 0, iic_status = IIC_SUCCESS;
	u8 RegAddr[5] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	u8 Data[5] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};

	RegAddr[0] = 0x10;
	RegAddr[1] = (0x06 << 1);
	ret = i2c_master_send(iic_client, RegAddr, 2);
	if (ret != 2) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__,  ret);
		iic_status = IIC_FAIL;
		goto Set_End;
	}

	ret = i2c_master_recv(iic_client, Data, 2);
	if (ret != 2) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__,  ret);
		iic_status = IIC_FAIL;
		goto Set_End;
	} else {
		cur = Data[1] + PRADO_SHIFT;
		reg_cur = Data[1];
		reg_target = power - PRADO_SHIFT;
		pr_debug("[%s][%d] cur: %d, target: %d, reg_cur: 0x%x, reg_target: 0x%x\n"
			, __func__, __LINE__
			, cur, power, reg_cur, reg_target);

		if (power > cur) {
			for (; reg_cur <= reg_target; reg_cur += PRADO_STEP) {
				RegAddr[0] = 0x10;
				RegAddr[1] = (0x06 << 1);
				RegAddr[2] = 0x10;
				RegAddr[3] = reg_cur;
				ret = i2c_master_send(iic_client, RegAddr, 4);
				if (ret != 4) {
					pr_err("[%s][%d], send i2c fail,ret: %d\n"
						, __func__, __LINE__, ret);
					iic_status = IIC_FAIL;
					goto Set_End;
				}
			}
		} else if (power < cur) {
			for (; reg_cur >= reg_target; reg_cur -= PRADO_STEP) {
				RegAddr[0] = 0x10;
				RegAddr[1] = (0x06 << 1);
				RegAddr[2] = 0x10;
				RegAddr[3] = reg_cur;
				ret = i2c_master_send(iic_client, RegAddr, 4);
				if (ret != 4) {
					pr_err("[%s][%d], send i2c fail,ret: %d\n"
						, __func__, __LINE__, ret);
					iic_status = IIC_FAIL;
					goto Set_End;
				}
			}
		}

		if (power != cur) {
			RegAddr[0] = 0x10;
			RegAddr[1] = (0x06 << 1);
			RegAddr[2] = 0x10;
			RegAddr[3] = power - PRADO_SHIFT;
			ret = i2c_master_send(iic_client, RegAddr, 4);
			if (ret != 4) {
				pr_err("[%s][%d], send i2c fail,ret: %d\n"
						, __func__, __LINE__, ret);
				iic_status = IIC_FAIL;
				goto Set_End;
			}
		}
	}
Set_End:
	return iic_status;
}

static int prado_pmic_init(void)
{
	int ret = 0, iic_status = IIC_SUCCESS;

	u8  RegAddr[5] = {
		0x53, 0x45, 0x52, 0x44, 0x42
	};

	u8  Data[5] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};

	mutex_lock(&INIT_MUTEX);

	if (regulator_init == 1)
		goto Init_End;

	ret = i2c_master_send(iic_client, RegAddr, 5);
	if (ret != 5) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__, ret);
		//iic_status = IIC_FAIL;
		//goto Init_End;
	}

	RegAddr[0] = 0x7F;
	ret = i2c_master_send(iic_client, RegAddr, 1);
	if (ret != 1) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__, ret);
		iic_status = IIC_FAIL;
		goto Init_End;
	}

	RegAddr[0] = 0x7D;
	ret = i2c_master_send(iic_client, RegAddr, 1);
	if (ret != 1) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__, ret);
		iic_status = IIC_FAIL;
		goto Init_End;
	}

	RegAddr[0] = 0x50;
	ret = i2c_master_send(iic_client, RegAddr, 1);
	if (ret != 1) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__, ret);
		iic_status = IIC_FAIL;
		goto Init_End;
	}

	RegAddr[0] = 0x55;
	ret = i2c_master_send(iic_client, RegAddr, 1);
	if (ret != 1) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__, ret);
		iic_status = IIC_FAIL;
		goto Init_End;
	}

	RegAddr[0] = 0x35;
	ret = i2c_master_send(iic_client, RegAddr, 1);
	if (ret != 1) {
		pr_err("[%s][%d], i2c fail,ret: %d\n"
			, __func__, __LINE__, ret);
		iic_status = IIC_FAIL;
		goto Init_End;
	}

	RegAddr[0] = 0x10;
	RegAddr[1] = 0xc0;
	ret = i2c_master_send(iic_client, RegAddr, 2);
	if (ret != 2) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__, ret);
		iic_status = IIC_FAIL;
		goto Init_End;
	}


	ret = i2c_master_recv(iic_client, Data, 2);
	if (ret != 2) {
		pr_err("[%s][%d], send i2c fail,ret: %d\n"
			, __func__, __LINE__, ret);
		iic_status = IIC_FAIL;
		goto Init_End;
	} else {
		pr_info("[%s][%d], receive i2c successful, id = 0x%x\n"
			, __func__, __LINE__, Data[0]);
		pr_info("[%s][%d], receive i2c successful, id = 0x%x\n"
			, __func__, __LINE__, Data[1]);
	}

	if (Data[1] == PRADO_ID) {
		RegAddr[0] = 0x10;
		RegAddr[1] = (0x06 << 1);
		RegAddr[2] = 0x10;
		RegAddr[3] = PRADO_DEFAULT - PRADO_SHIFT;
		ret = i2c_master_send(iic_client, RegAddr, 4);
		if (ret != 4) {
			pr_err("[%s][%d], send i2c fail,ret: %d\n"
				, __func__, __LINE__, ret);
			iic_status = IIC_FAIL;
			goto Init_End;
		}

		RegAddr[0] = 0x10;
		RegAddr[1] = (0x05 << 1);
		RegAddr[2] = 0x40;
		RegAddr[3] = 0x00;
		ret = i2c_master_send(iic_client, RegAddr, 4);
		if (ret != 4) {
			pr_err("[%s][%d], send i2c fail,ret: %d\n"
				, __func__, __LINE__, ret);
			iic_status = IIC_FAIL;
			goto Init_End;
		}

		RegAddr[0] = 0x10;
		RegAddr[1] = (0x0C << 1);
		RegAddr[2] = 0xbe;
		RegAddr[3] = 0xaf;
		ret = i2c_master_send(iic_client, RegAddr, 4);
		if (ret != 4) {
			pr_err("[%s][%d], send i2c fail,ret: %d\n"
				, __func__, __LINE__, ret);
			iic_status = IIC_FAIL;
			goto Init_End;
		} else
			pr_info("pmic initialization successful\n");

		regulator_init = 1;
	} else {
		pr_err("[%s][%d] not supprt pmic id: %d\n"
			, __func__, __LINE__, Data[1]);
		iic_status = IIC_FAIL;
	}

Init_End:
	mutex_unlock(&INIT_MUTEX);
	return iic_status;
}



static int cpu_regulator_enable(struct regulator_dev *dev)
{
	return 0;
}

static int cpu_regulator_disable(struct regulator_dev *dev)
{
	mutex_lock(&INIT_MUTEX);
	regulator_init = 0;
	mutex_unlock(&INIT_MUTEX);
	return 0;
}

static int cpu_regulator_is_enabled(struct regulator_dev *dev)
{
	u8 bEnable = 1;

	mutex_lock(&INIT_MUTEX);
	bEnable = regulator_init;
	mutex_unlock(&INIT_MUTEX);
	return bEnable;
}

static int cpu_regulator_get_voltage(struct regulator_dev *rdev)
{
	return cpu_get_voltage_bysar()*1000;
}

static int cpu_regulator_set_voltage(struct regulator_dev *rdev,
		int reg_min, int reg_max, unsigned int *selector)
{
	u32 reg_target = 0;
	int iic_status = IIC_SUCCESS;

	if (regulator_init == 0) {
		iic_status = prado_pmic_init();
		if (iic_status == IIC_FAIL)
			return 1;
	}

	reg_target = (reg_min + reg_max)/2;
	reg_target = reg_target/10000;
	iic_status = cpu_set_voltage(reg_target);

	pr_debug("[%s][%d]min = %d uV, max = %d uV\n"
		, __func__, __LINE__, reg_min, reg_max);

	if (iic_status == IIC_FAIL)
		return 1;
	else
		return 0;
}

static struct regulator_ops cpu_regulator_voltage_ops = {
	.get_voltage    = cpu_regulator_get_voltage,
	.set_voltage    = cpu_regulator_set_voltage,
	.enable         = cpu_regulator_enable,
	.disable        = cpu_regulator_disable,
	.is_enabled     = cpu_regulator_is_enabled,
};

static struct regulator_desc cpu_regulator_desc = {
	.name                      = "mtk-cpu-regulator",
	.supply_name               = "mtk-cpu-regulator",
	.type                      = REGULATOR_VOLTAGE,
	.continuous_voltage_range  = true,
	.ops                       = &cpu_regulator_voltage_ops,
	.owner                     = THIS_MODULE,
};

static struct regulator_init_data init_data = {
	.constraints = {
		.min_uV = 900000,
		.max_uV = 1200000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS,
	},
};


static int cpu_regulator_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct regulator_dev *regulator;
	struct regulator_config config = { };
	struct device *dev = &client->dev;
	struct device_node *node = dev->of_node;
	u32 tmp[2];
	int ret;

	if (regulator_pm_base == NULL) {
		of_property_read_u32_array(node, "pm-base-reg", tmp, 2);
		regulator_pm_base = ioremap(tmp[0], tmp[1]);
		dev_info(dev, "%s reg base : 0x%x(0x%x) -> 0x%lx\n",
				node->name, tmp[0], tmp[1], regulator_pm_base);
		if (!regulator_pm_base) {
			dev_err(dev, "failed to map regulator_pm_base (%ld)\n",
					PTR_ERR(regulator_pm_base));
			dev_err(dev, "node:%s reg base error\n", node->name);
			return -EINVAL;
		}
	}

	config.dev = &client->dev;
	config.driver_data = NULL;
	config.init_data = &init_data;
	iic_client = client;

	regulator = devm_regulator_register(&client->dev,
			&cpu_regulator_desc, &config);
	if (IS_ERR(regulator)) {
		ret = PTR_ERR(regulator);
		dev_err(&client->dev, "Failed to register regulator %s: %d\n"
			, cpu_regulator_desc.name, ret);
		return ret;
	}
	return 0;
}


static const struct i2c_device_id regulator_id[] = {
	{ "mtk-cpu-regulator", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, regulator_id);

static struct i2c_driver cpu_regulator_driver = {
	.driver = {
		.name                   = "mtk-cpu-regulator",
		.owner                  = THIS_MODULE,
	},
	.probe = cpu_regulator_probe,
	.id_table                       = regulator_id,
};

static int __init cpu_regulator_init(void)
{
	int ret = 1;

	ret = i2c_add_driver(&cpu_regulator_driver);
	if (ret != 0)
		pr_err("Register mtk-cpu-regulator driver fail\n");
	else
		pr_info("Register mtk-cpu-regulator driver successful\n");
	return 0;
}

subsys_initcall(cpu_regulator_init);

MODULE_DESCRIPTION("MediaTek TV Regulator driver");
MODULE_AUTHOR("MediaTek Inc.");
MODULE_LICENSE("GPL");
