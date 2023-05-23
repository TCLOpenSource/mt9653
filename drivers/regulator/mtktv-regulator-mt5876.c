// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/regmap.h>
#include <linux/err.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/slab.h>
#include <linux/iio/consumer.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/reset.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/pm_opp.h>
#include <linux/pwm.h>
#include <linux/bits.h>
#include <linux/regulator/consumer.h>

struct mt5876_pwm_regulator_data {
	/*  Shared */
	struct pwm_device *pwm;

	/* iio */
	struct iio_channel *CPUCorePower;

	int state;
};

/*********************************************************************
 *                         External Functions                        *
 *********************************************************************/

/*********************************************************************
 *                         Private Structure                         *
 *********************************************************************/
typedef enum {
	E_REGULATOR_PRADO = 0,
	E_REGULATOR_PWM   = 1,
	E_REGULATOR_GPIO  = 2,
	E_REGULATOR_NONE  = 3,
} E_REGULATOR_TYPE;

/*********************************************************************
 *                         Private Macro                             *
 *********************************************************************/
static void __iomem *regulator_base;
#define BASE_ADDRESS                 (regulator_base)

/* PWM0_DAC */
#define REG_PWM0_DAC_PERIOD         (0x00) //0x243800
#define REG_PWM0_DAC_DUTY           (0x02) //0x243802
#define REG_PWM0_DAC_DBEN           (0x04) //0x243804
#define REG_PWM0_DAC_DUTY_OFFSET    (0x06) //0x243806
#define REG_PWM0_DAC_AUTO_CORRECT   (0x08) //0x243808
#define PWM0_AUTO_EN            (BIT(0))
#define PWM0_DBEN_EN            (BIT(8))

/* PWM1_DAC */
#define REG_PWM1_DAC_PERIOD         (0x20) //0x243820
#define REG_PWM1_DAC_DUTY           (0x22) //0x243822
#define REG_PWM1_DAC_DBEN           (0x24) //0x243824
#define REG_PWM1_DAC_DUTY_OFFSET    (0x26) //0x243826
#define REG_PWM1_DAC_AUTO_CORRECT   (0x28) //0x243828
#define PWM1_AUTO_EN            (BIT(0))
#define PWM1_DBEN_EN            (BIT(8))

#define PWM_OFFSET_INIT         (0xFF)
#define PWM_OFFSET_VALUE        (0x80)


#define SAR_1_8_VOLTAGE              (1800000)
#define SAR_2_0_VOLTAGE              (2000000)
#define SAR_3_3_VOLTAGE              (3300000)
#define SAR_RESLOUTION               (1023)
#define SAR_ADC_SAMPLE               (32)
#define DIV_2                        (2)
#define DIV_1000                     (1000)
#define DIV_10000                    (10000)
#define TIMES_1000                   (1000)
#define DUTY_COMPENSATE       (1)

#define REGULATOR_MIN_UV       (600000)
#define REGULATOR_MIN_SEL      (0)
#define REGULATOR_MAX_SEL      (90000 / 1000)
#define REGULATOR_STEP_UV      (10000)
#define REGULATOR_MAX_STEP     (100)

#define REGULATOR_FIND_BY_NAME(parent, name, val) \
	do { \
		val = of_find_node_by_name(parent, name); \
		if (val == NULL) { \
			pr_err("[mtktv-regulator] can't find dts node: %s\n", name); \
			WARN_ON(1); \
		} \
	} while (0)

#define REGULATOR_READ_U32(parent, name, val) \
	do { \
		if (of_property_read_u32(parent, name, &val)) { \
			pr_err("[mtktv-regulator] can't get value in parent node: %s\n", name); \
			WARN_ON(1); \
		} \
	} while (0)

#define REGULATOR_READ_STR(parent, name, val) \
	do { \
		if (of_property_read_string(parent, name, &val)) { \
			pr_err("[mtktv-regulator] can't get string in parent node: %s\n", name); \
			WARN_ON(1); \
		} \
	} while (0)

#define REGULATOR_READ_U32_ARRAY(parent, name, val, size) \
	do { \
		if (of_property_read_u32_array(parent, name, val, size)) { \
			pr_err("[mtktv-regulator] can't get array in parent node: %s\n", name); \
			WARN_ON(1); \
		} \
	} while (0)

/*********************************************************************
 *                         Private Data                              *
 *********************************************************************/
DEFINE_MUTEX(INIT_MUTEX);

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static u8 regulator_parsed;

static E_REGULATOR_TYPE type = E_REGULATOR_NONE;
static u32 default_power;
static u32 min_power;
static u32 max_power;

// pwm
static u32 channel;
static u32 scale; // step voltage
static u32 period;
static u32 duty;
#endif

static u8 regulator_init;
/*********************************************************************
 *                          Local Function                          *
 *********************************************************************/

#define regulator_writew(addr, data) \
	writew(data, (void __iomem *)(BASE_ADDRESS + (addr << 1)))

#define regulator_readw(addr) \
	readw((void __iomem *)(BASE_ADDRESS + (addr << 1)))

typedef enum _PWM_ChNum {
	E_PVR_PWM_CH0,
	E_PVR_PWM_CH1,
	E_PWM_MAX
} PWM_ChNum;

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
int MDrv_PWM_IsCrrect(unsigned char u8IndexPWM)
{
	int ret = 0;

	switch (u8IndexPWM) {
	case E_PVR_PWM_CH0:
		ret = regulator_readw(REG_PWM0_DAC_DUTY_OFFSET);
		break;
	case E_PVR_PWM_CH1:
		ret = regulator_readw(REG_PWM1_DAC_DUTY_OFFSET);
		break;
	default:
		pr_err("No such PWM channel PERIOD\n");
	}
	return ret;
}

void MDrv_PWM_Offset(unsigned char u8IndexPWM, unsigned int u32Offset)
{
	switch (u8IndexPWM) {
	case E_PVR_PWM_CH0:
		regulator_writew(REG_PWM0_DAC_DUTY_OFFSET, u32Offset);
		regulator_writew(REG_PWM0_DAC_DBEN, PWM0_DBEN_EN);
		break;
	case E_PVR_PWM_CH1:
		regulator_writew(REG_PWM1_DAC_DUTY_OFFSET, u32Offset);
		regulator_writew(REG_PWM1_DAC_DBEN, PWM1_DBEN_EN);
		break;
	default:
		pr_err("No such PWM channel PERIOD\n");
	}
}

void MDrv_PWM_AutoCorrect(unsigned char u8IndexPWM, unsigned char  bCorrect_en)
{
	switch (u8IndexPWM) {
	case E_PVR_PWM_CH0:
		if(bCorrect_en == 1)
			regulator_writew(REG_PWM0_DAC_AUTO_CORRECT,
			regulator_readw(REG_PWM0_DAC_AUTO_CORRECT) | PWM0_AUTO_EN);
		else
			regulator_writew(REG_PWM0_DAC_AUTO_CORRECT,
			regulator_readw(REG_PWM0_DAC_AUTO_CORRECT) & ~(PWM0_AUTO_EN));
		break;
	case E_PVR_PWM_CH1:
		if(bCorrect_en == 1)
			regulator_writew(REG_PWM1_DAC_AUTO_CORRECT,
			regulator_readw(REG_PWM1_DAC_AUTO_CORRECT) | PWM1_AUTO_EN);
		else
			regulator_writew(REG_PWM1_DAC_AUTO_CORRECT,
			regulator_readw(REG_PWM1_DAC_AUTO_CORRECT) & ~(PWM1_AUTO_EN));
		break;
	default:
		pr_err("No such PWM channel PERIOD\n");
	}
}
#endif

static int mtktv_get_voltage_bysar(struct regulator_dev *rdev)
{
	struct mt5876_pwm_regulator_data *drvdata = rdev_get_drvdata(rdev);
	int voltage = 0, saradc = 0;

	if (drvdata == NULL) {
		pr_err("drvdata is NULL\n");
		return 0;
	}
	if (drvdata->CPUCorePower == NULL) {
		pr_err("CPUCorePower is NULL\n");
		return 0;
	}

	iio_read_channel_processed(drvdata->CPUCorePower, &saradc);
	voltage = saradc * (SAR_2_0_VOLTAGE / SAR_RESLOUTION); // uV
	return voltage;
}

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static int mtktv_set_voltage(int power)
{
	u32 adjust_duty = duty;

	if (power > default_power/DIV_1000) {
		if (((power - default_power/DIV_1000)*TIMES_1000/scale)
			> adjust_duty)
			adjust_duty = 0;
		else
			adjust_duty -= (power - default_power/DIV_1000)*TIMES_1000
			/scale;
	} else {
		adjust_duty += (default_power/DIV_1000 - power)*TIMES_1000/scale;
	}
	return adjust_duty;
}
#endif

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static int mtktv_regulator_parse(void)
{
	struct device_node *node;
	const char *name;
	int ret = 0;

	REGULATOR_FIND_BY_NAME(NULL, "buck_vcpu", node);
	REGULATOR_READ_U32(node, "regulator-default", default_power);
	REGULATOR_READ_U32(node, "regulator-min-microvolt", min_power);
	REGULATOR_READ_U32(node, "regulator-max-microvolt", max_power);
	REGULATOR_READ_STR(node, "regulator-type", name);
	pr_info("[mtktv-regulator] regulator-default = %d, min = %d, max = %d\n",
		default_power, min_power, max_power);
	min_power /= DIV_10000;
	max_power /= DIV_10000;

	type = E_REGULATOR_PWM;
	REGULATOR_READ_U32(node, "regulator-channel", channel);
	REGULATOR_READ_U32(node, "regulator-scale", scale);
	REGULATOR_READ_U32(node, "regulator-period", period);
	REGULATOR_READ_U32(node, "regulator-duty", duty);
	pr_info("[mtktv-regulaotr] regulator type: pwm\n");
	pr_info("[mtktv-regulator] channel = %d,scale = %d\n", channel, scale);
	pr_info("[mtktv-regulator] duty = %d, period = %d\n", duty, period);

	return ret;
}
#endif

#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
static int mtktv_pmic_init(struct regulator_dev *rdev)
{
	int ret = 0;
	PWM_ChNum num = E_PVR_PWM_CH1;
	u32 detect_vol = 0;
	u32 index = 0, total = 0;
	u16 val = 0;
	u16 pwm_offset = PWM_OFFSET_INIT;
	struct pwm_state pstate;
	struct mt5876_pwm_regulator_data *drvdata = rdev_get_drvdata(rdev);

	if (regulator_parsed == 0) {
		ret = mtktv_regulator_parse();
		if (ret)
			pr_err("[mtktv-regualtor] parsing fail\n");
		else
			regulator_parsed = 1;
	}

	mutex_lock(&INIT_MUTEX);
	if (regulator_init == 1) {
		mutex_unlock(&INIT_MUTEX);
		return 0;
	}

	pr_info("[mtktv-regulator] PWM DAC INIT\n");
	num = E_PVR_PWM_CH1;

	detect_vol = 0;
	val = 0;
	if (channel == 0)
		num = E_PVR_PWM_CH0;
	else
		num = E_PVR_PWM_CH1;

	// check pwmdac have been correct?
	ret = MDrv_PWM_IsCrrect(num);
	if(ret != 0) {
		pr_info("[mtktv-regulator] PWM DAC Already Init\n");
		regulator_init = 1;
		mutex_unlock(&INIT_MUTEX);
		return 0;
	}

    pwm_init_state(drvdata->pwm, &pstate);
	pwm_set_relative_duty_cycle(&pstate, duty + DUTY_COMPENSATE, period);
	ret = pwm_apply_state(drvdata->pwm, &pstate);
	if (ret)
		return ret;

	total = 0;
	for (index = 0; index < SAR_ADC_SAMPLE; index++)
		total += mtktv_get_voltage_bysar(rdev);
	detect_vol = total/SAR_ADC_SAMPLE;
	detect_vol /= DIV_1000;

	if (detect_vol >= default_power/DIV_1000) {
		pwm_offset = (detect_vol - (default_power/DIV_1000)) * TIMES_1000/scale;
		pwm_offset &= ~(PWM_OFFSET_VALUE);
	} else {
		pwm_offset = ((default_power/DIV_1000) - detect_vol) * TIMES_1000/scale;
		pwm_offset |= (PWM_OFFSET_VALUE);	
	}
	MDrv_PWM_Offset(num, pwm_offset);
	MDrv_PWM_AutoCorrect(num, 1);
	regulator_init = 1;
	mutex_unlock(&INIT_MUTEX);
	return ret;
}
#endif 

static int mtktv_regulator_enable(struct regulator_dev *rdev)
{
	return 0;
}

static int mtktv_regulator_disable(struct regulator_dev *rdev)
{
	mutex_lock(&INIT_MUTEX);
	regulator_init = 0;
	mutex_unlock(&INIT_MUTEX);
	return 0;
}

static int mtktv_regulator_is_enabled(struct regulator_dev *rdev)
{
	u8 bEnable = 0;

	mutex_lock(&INIT_MUTEX);
	if (regulator_init == 1)
		bEnable = 1;
	mutex_unlock(&INIT_MUTEX);
	return (bEnable == 1)?0:1;
}

static int mtktv_regulator_get_voltage(struct regulator_dev *rdev)
{
	return mtktv_get_voltage_bysar(rdev);
}

static int mtktv_regulator_set_voltage(struct regulator_dev *rdev, int reg_min,
					int reg_max, unsigned int *selector)
{
#if !IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ_CA)
	struct mt5876_pwm_regulator_data *drvdata = rdev_get_drvdata(rdev);
	u32  reg_target = 0;
	struct pwm_state pstate;
	int ret;
	u32 adjust_duty = 0;

	if (regulator_init == 0)
		mtktv_pmic_init(rdev);

	reg_target = (reg_min + reg_max)/DIV_2;
	reg_target = reg_target/DIV_1000;
	adjust_duty = mtktv_set_voltage(reg_target);

    pwm_init_state(drvdata->pwm, &pstate);
	pwm_set_relative_duty_cycle(&pstate, adjust_duty + DUTY_COMPENSATE, period);
	ret = pwm_apply_state(drvdata->pwm, &pstate);
	if (ret) {
		dev_err(&rdev->dev, "Failed to configure PWM: %d\n", ret);
		return ret;
	}
#endif // endof CONFIG_ARM_MTKTV_CPUFREQ_CA
	return 0;
}

static struct regulator_ops mtktv_regulator_voltage_continuous_ops = {
	.get_voltage    = mtktv_regulator_get_voltage,
	.set_voltage    = mtktv_regulator_set_voltage,
	.enable         = mtktv_regulator_enable,
	.disable        = mtktv_regulator_disable,
	.is_enabled     = mtktv_regulator_is_enabled,
};

static struct regulator_linear_range buck_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(REGULATOR_MIN_UV, REGULATOR_MIN_SEL, REGULATOR_MAX_SEL,
				REGULATOR_STEP_UV),
};

static struct regulator_desc mtktv_regulator_desc = {
	.name           = "VCPU",
	.type           = REGULATOR_VOLTAGE,
	.owner          = THIS_MODULE,
	.of_match = of_match_ptr("buck_vcpu"),
	.ops = &mtktv_regulator_voltage_continuous_ops,
	.type = REGULATOR_VOLTAGE,
	.id = 0,
	.n_voltages = REGULATOR_MAX_STEP,
	.linear_ranges = buck_volt_range1,
	.n_linear_ranges = ARRAY_SIZE(buck_volt_range1),
	.continuous_voltage_range =  true,
};

static int mt5876_regulator_probe(struct platform_device *pdev)
{
	struct regulator_config config = {};
	struct mt5876_pwm_regulator_data *drvdata;
	struct device *dev = &pdev->dev;
	const struct regulator_init_data *init_data;
	struct regulator_dev *regulator;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	if (!np) {
		dev_err(dev, "Device Tree node missing\n");
		return -EINVAL;
	}

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	if (regulator_base == NULL) {
		regulator_base = of_iomap(np, 0);
		if (!regulator_base) {
			dev_err(dev, "failed to map regulator_base (%ld)\n",
					PTR_ERR(regulator_base));
			dev_err(dev, "node:%s reg base error\n", np->name);
			kfree(drvdata);
			return -EINVAL;
		}
	}

	drvdata->CPUCorePower = devm_iio_channel_get(dev, "adc16");
	if (IS_ERR(drvdata->CPUCorePower)) {
		dev_err(dev, "Failed to get SARADC in %s\n", np->name);
		ret = PTR_ERR(drvdata->CPUCorePower);
		kfree(drvdata);
		return ret;
	}

	drvdata->pwm = devm_pwm_get(dev, "pwmadc");
	if (IS_ERR(drvdata->pwm)) {
		dev_err(dev, "Failed to get PWMS in %s\n", np->name);
		ret = PTR_ERR(drvdata->pwm);
		kfree(drvdata);
		return ret;
	}

	init_data = of_get_regulator_init_data(dev, np, &mtktv_regulator_desc);
	if (!init_data)
		return -ENOMEM;
	config.of_node = np;
	config.dev = dev;
	config.driver_data = drvdata;
	config.init_data = init_data;
	mtktv_regulator_desc.continuous_voltage_range =  true;
	regulator = devm_regulator_register(dev, &mtktv_regulator_desc, &config);
	if (IS_ERR(regulator)) {
		ret = PTR_ERR(regulator);
		dev_err(dev, "Failed to register regulator %s: %d\n",
			mtktv_regulator_desc.name, ret);
		kfree(drvdata);
		return ret;
	}

	return 0;
}

static const struct platform_device_id mt5876_platform_ids[] = {
	{"mt5876-regulator", 0}, {/* sentinel */},
};
MODULE_DEVICE_TABLE(platform, mt5876_platform_ids);

static const struct of_device_id mt5876_of_match[] = {
	{
		.compatible = "mediatek,mt5876-regulator",
	},
	{/* sentinel */},
};
MODULE_DEVICE_TABLE(of, mt5876_of_match);

static struct platform_driver mt5876_regulator_driver = {
	.driver = {
		.name = "mt5876-regulator",
		.of_match_table = of_match_ptr(mt5876_of_match),
	},
	.probe = mt5876_regulator_probe,
	.id_table = mt5876_platform_ids,
};

module_platform_driver(mt5876_regulator_driver);

MODULE_AUTHOR("Mark Tseng <chun-jen.tseng@mediatek.com>");
MODULE_DESCRIPTION("Regulator Driver for MediaTek MT5876 PMIC");
MODULE_LICENSE("GPL");
