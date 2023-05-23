// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/thermal.h>
#include <linux/reset.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/iio/consumer.h>
#include <linux/cpufreq.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/pm_opp.h>

static void __iomem *thermal_pm_base;
static void __iomem *thermal_pm_base2;
#define BASE_ADDRESS                    (thermal_pm_base  + 0x200)
#define BASE_ADDRESS_2                   (thermal_pm_base2 + 0x200)

#define DEFAULT_T_SENSOR_REF            (772)
#define DEFAULT_T_SENSOR_SHIFT          (24000)
#define VBE_CODE_MULTIPLIER     (770)
#define VBE_CODE_FT_MULTIPLIER     (770)
#define M6L_DEFAULT_T_SENSOR_REF            (740)
#define M6L_DEFAULT_T_SENSOR_SHIFT          (27000)
#define M6L_VBE_CODE_MULTIPLIER     (753)
#define MOKONA_DEFAULT_T_SENSOR_REF         (400)
#define MOKONA_DEFAULT_T_SENSOR_SHIFT       (27500)
#define MOKONA_VBE_CODE_MULTIPLIER     (1250)
#define MOKONA_VBE_CODE_FT_MULTIPLIER     (1270)
#define MIFFY_DEFAULT_T_SENSOR_REF         (400)
#define MIFFY_DEFAULT_T_SENSOR_SHIFT       (27500)
#define MIFFY_VBE_CODE_MULTIPLIER        (1250)
#define MIFFY_VBE_CODE_FT_MULTIPLIER     (1270)
#define MOKA_DEFAULT_T_SENSOR_REF         (400)
#define MOKA_DEFAULT_T_SENSOR_SHIFT       (27500)
#define MOKA_VBE_CODE_MULTIPLIER     (1250)
#define MOKA_VBE_CODE_FT_MULTIPLIER     (1270)

#define DEFAULT_T_SENSOR_RANGE          (15)
#define REG_SAR_VBE                     (0x10050C)
#define VBE_SIZE                        (10)
#define REG_SENSOR_SHIFT                (0x10050E)
#define MAX_BUFFER_SIZE                 (100)

#define SENSOR_DETECT_DELAY_US          (30)
#define MAX_NAME_SIZE                   (100)
#define REG_PM_MISC_BANK        (0x10900)
#define REG_PM_MISC_TOP_SW_RST     (0x5C)
#define PM_MISC_TOP_SW_RST_VAL  (0x79)

#define EFUSE_BASE                     (0x12900 << 1)
#define VBE_FT_BANK                    (0x1B << 2)
#define FLAG_EFUSE_RDATA_BUSY_BIT      (0x2000)
#define REG_EFUSE_RADDR                (0x0050 << 1)
#define REG_EFUSE_RVALL                (0x0058 << 1)
#define REG_EFUSE_RVALH                (0x005A << 1)
#define REG_EFUSE_RVALH_OFFSET         (16)
#define REG_VBE_CODE_FT_MASK           (0x3FF)
#define REG_VBE_CODE_FT_SHIT           (6)
#define REG_TA_CODE_FT_MASK            (0x1FF)
#define M6_REG_TA_CODE_FT_SHIT            (17)
#define MOKONA_REG_TA_CODE_FT_SHIT        (16)

#define TEMP_DIV_1000     (1000)
#define NUM_100     (100)
#define NUM_1700       (1700)
#define NUM_1500       (1500)
#define NUM_750        (750)

/* chip ID */
#define M6_ID       (0x101)
#define M6L_ID      (0x109)
#define MOKONA_ID      (0x10C)
#define MIFFY_ID      (0x10D)
#define MOKA_ID      (0x111)
#define UNKNOWN_ID  (0x0)

static u16 tsensor_ref;  //VBE_FT
static u16 tsensor_shift; //TA_FT
struct iio_channel *SAR_Thermal;
static int chip_id;

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ)
u32 temperature_offset;
EXPORT_SYMBOL(temperature_offset);
#endif

#define thermal_writew(data, addr) \
	writew(data, (void __iomem *)(BASE_ADDRESS + (addr << 1)))

#define thermal_readw(addr) \
	readw((void __iomem *)(BASE_ADDRESS + (addr << 1)))

#define thermal_writew2(data, addr) \
	writew(data, (void __iomem *)(BASE_ADDRESS_2 + (addr << 1)))

#define thermal_readw2(addr) \
	readw((void __iomem *)(BASE_ADDRESS_2 + (addr << 1)))

void get_VBE_CODE(void)
{
	unsigned int u32VBETemp = 0, read_value = 0;

	thermal_writew2(FLAG_EFUSE_RDATA_BUSY_BIT | VBE_FT_BANK, REG_EFUSE_RADDR);
	do {
		read_value = thermal_readw2(REG_EFUSE_RADDR);
		read_value &= FLAG_EFUSE_RDATA_BUSY_BIT;
		if (read_value == 0) {
			u32VBETemp = (thermal_readw2(REG_EFUSE_RVALL) +
			thermal_readw2(REG_EFUSE_RVALH) << REG_EFUSE_RVALH_OFFSET);
			pr_info("wait efuse done\n");
			break;
		}
		pr_info("wait efuse reading\n");
	} while (1);
	tsensor_ref = (u32VBETemp >> REG_VBE_CODE_FT_SHIT) & REG_VBE_CODE_FT_MASK;
	if ((chip_id == M6_ID) || (chip_id == M6L_ID))
		tsensor_shift = (u32VBETemp >> M6_REG_TA_CODE_FT_SHIT) & REG_TA_CODE_FT_MASK;
	else
		tsensor_shift = (u32VBETemp >> MOKONA_REG_TA_CODE_FT_SHIT) & REG_TA_CODE_FT_MASK;
	tsensor_shift *= NUM_100;
	pr_info("tsensor_ref = %d\n", tsensor_ref);
	pr_info("tsensor_shift = %d\n", tsensor_shift);
}

int get_cpu_temperature_m6(void)
{
	int temperature = 0;

	/*
	 * temp=0.77x(vbe_code_ft-vbe_code)+ 24
	 * Vbe_code_ft=764 ( 25 degree )
	 *
	 * Machli T-sensor NEW Equation = 0.77*(VBE_CODE_FT - VBE_CODE )+(TA_CODE/10+0.75)
	 */

	if (SAR_Thermal == NULL)
		return temperature;

	if (tsensor_shift == 0)
		tsensor_shift = DEFAULT_T_SENSOR_SHIFT;

	if (tsensor_ref == 0)
		tsensor_ref = DEFAULT_T_SENSOR_REF;
	// read VBE_CODE
	iio_read_channel_processed(SAR_Thermal, &temperature);

	if (tsensor_ref == DEFAULT_T_SENSOR_REF) {
		temperature = ((DEFAULT_T_SENSOR_REF - temperature) * VBE_CODE_MULTIPLIER)
			+ DEFAULT_T_SENSOR_SHIFT;
	} else {
		temperature = VBE_CODE_FT_MULTIPLIER * (tsensor_ref - temperature)
			+ (tsensor_shift + NUM_750);
	}
	temperature /= TEMP_DIV_1000;
	return temperature;
}

int get_cpu_temperature_m6l(void)
{
	int temperature = 0;

	/*
	 * temp = 25 - 0.753 * (SAR - 743)
	 * 12/21 update to below formula
	 * temp = 0.74 * (VBE_CODE_FT - VBE_CODE) + 27
	 * Vbe_code_ft=743 ( 25 degree )
	 */

	if (SAR_Thermal == NULL)
		return temperature;

	if (tsensor_shift == 0)
		tsensor_shift = M6L_DEFAULT_T_SENSOR_SHIFT;

	if (tsensor_ref == 0)
		tsensor_ref = M6L_DEFAULT_T_SENSOR_REF;
	// read VBE_CODE
	iio_read_channel_processed(SAR_Thermal, &temperature);

	temperature = M6L_DEFAULT_T_SENSOR_SHIFT + ((M6L_DEFAULT_T_SENSOR_REF - temperature)
			* M6L_VBE_CODE_MULTIPLIER);
	temperature /= TEMP_DIV_1000;
	return temperature;
}

int get_cpu_temperature_mokona(void)
{
	int temperature = 0;

	/*
	 * temp = 1.25 * (VBE_CODE_FT - VBE_CODE) + 27.5
	 * Vbe_code_ft=400 ( 25 degree )
	 */

	if (SAR_Thermal == NULL)
		return temperature;

	if (tsensor_shift == 0)
		tsensor_shift = MOKONA_DEFAULT_T_SENSOR_SHIFT;

	if (tsensor_ref == 0)
		tsensor_ref = MOKONA_DEFAULT_T_SENSOR_REF;

	iio_read_channel_processed(SAR_Thermal, &temperature);

	if (tsensor_ref == MOKONA_DEFAULT_T_SENSOR_REF) {
		temperature = MOKONA_VBE_CODE_MULTIPLIER * (tsensor_ref - temperature)
			+ tsensor_shift;
	} else {
		temperature = MOKONA_VBE_CODE_FT_MULTIPLIER * (tsensor_ref - temperature)
			+ (tsensor_shift + NUM_1700);
	}
	temperature /= TEMP_DIV_1000;
	return temperature;
}

int get_cpu_temperature_miffy(void)
{
	int temperature = 0;

	/*
	 * temp = 1.25 * (VBE_CODE_FT - VBE_CODE) + 27.5
	 * Vbe_code_ft=400 ( 25 degree )
	 */

	if (SAR_Thermal == NULL)
		return temperature;

	if (tsensor_shift == 0)
		tsensor_shift = MIFFY_DEFAULT_T_SENSOR_SHIFT;

	if (tsensor_ref == 0)
		tsensor_ref = MIFFY_DEFAULT_T_SENSOR_REF;

	iio_read_channel_processed(SAR_Thermal, &temperature);

	if (tsensor_ref == MIFFY_DEFAULT_T_SENSOR_REF) {
		temperature = MIFFY_VBE_CODE_MULTIPLIER * (tsensor_ref - temperature)
			+ tsensor_shift;
	} else {
		temperature = MIFFY_VBE_CODE_MULTIPLIER * (tsensor_ref - temperature)
			+ (tsensor_shift + NUM_1500);
	}
	temperature /= TEMP_DIV_1000;
	return temperature;
}

int get_cpu_temperature_moka(void)
{
	int temperature = 0;

	/*
	 * temp = 1.25 * (VBE_CODE_FT - VBE_CODE) + 27.5
	 * Vbe_code_ft=400 ( 25 degree )
	 */

	if (SAR_Thermal == NULL)
		return temperature;

	if (tsensor_shift == 0)
		tsensor_shift = MOKA_DEFAULT_T_SENSOR_SHIFT;

	if (tsensor_ref == 0)
		tsensor_ref = MOKA_DEFAULT_T_SENSOR_REF;

	iio_read_channel_processed(SAR_Thermal, &temperature);

	if (tsensor_ref == MOKA_DEFAULT_T_SENSOR_REF) {
		temperature = MOKA_VBE_CODE_MULTIPLIER * (tsensor_ref - temperature)
			+ tsensor_shift;
	} else {
		temperature = MOKA_VBE_CODE_FT_MULTIPLIER * (tsensor_ref - temperature)
			+ (tsensor_shift + NUM_1700);
	}
	temperature /= TEMP_DIV_1000;
	return temperature;
}

int get_cpu_temperature_bysar(void)
{
	int temperature = 0;

	if (chip_id == M6_ID)
		temperature = get_cpu_temperature_m6();
	else if (chip_id == M6L_ID)
		temperature = get_cpu_temperature_m6l();
	else if (chip_id == MOKONA_ID)
		temperature = get_cpu_temperature_mokona();
	else if (chip_id == MIFFY_ID)
		temperature = get_cpu_temperature_miffy();
	else if (chip_id == MOKA_ID)
		temperature = get_cpu_temperature_moka();
	else
		temperature = 0;

#if IS_ENABLED(CONFIG_ARM_MTKTV_CPUFREQ)
	temperature += temperature_offset;
#endif

	return temperature;
}
EXPORT_SYMBOL(get_cpu_temperature_bysar);
static int get_temp(void *thermal, int *temp)
{
	*temp = get_cpu_temperature_bysar()*1000;
	return 0;
}

static int notify_callback(struct thermal_zone_device *tz,
			int trip, enum thermal_trip_type type)
{
	if (type == THERMAL_TRIP_CRITICAL) {
		pr_emerg("[mtktv-thermal] trip point: %d\n", trip);
		pr_emerg("[mtktv-thermal] Reach reset temperature point\n"
				"trigger WDT reset to protect SOC!!!\n");
		thermal_writew(PM_MISC_TOP_SW_RST_VAL, REG_PM_MISC_TOP_SW_RST);
	}
	return 0;
}

static struct thermal_zone_of_device_ops mtktv_thermal_of_ops = {
	.get_temp = get_temp,
};

static int mtktv_thermal_probe(struct platform_device *pdev)
{
	struct thermal_zone_device *tzdev;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device_node *model = NULL;
	u32 tmp[2];

	model = of_find_node_by_path("/mtktv_cpufreq");
	if (model == NULL) {
		pr_err("[mtktv-thermal]: model not found\n");
		return -EINVAL;
	}
	if (!of_property_match_string(model, "model", "MT5897")) {
		pr_info("[mtktv-thermal]: model MT5897\n");
		chip_id = M6L_ID;
	}
	if (!of_property_match_string(model, "model", "MT5896")) {
		pr_info("[mtktv-thermal]: model MT5896\n");
		chip_id = M6_ID;
	}
	if (!of_property_match_string(model, "model", "MT5876")) {
		pr_info("[mtktv-thermal]: model MT5876\n");
		chip_id = MOKONA_ID;
	}
	if (!of_property_match_string(model, "model", "MT5879")) {
		pr_info("[mtktv-thermal]: model MT5879\n");
		chip_id = MIFFY_ID;
	}
	if (!of_property_match_string(model, "model", "MT5873")) {
		pr_info("[mtktv-thermal]: model MT5873\n");
		chip_id = MOKA_ID;
	}
	if (chip_id == UNKNOWN_ID) {
		pr_err("UNKNOWN CHIP ID\n");
		of_node_put(model);
		return -EINVAL;
	}
	of_node_put(model);

	if (thermal_pm_base == NULL) {
		// REG_PM_MISC_BANK
		thermal_pm_base = of_iomap(pdev->dev.of_node, 0);
		// REG_VBE_CODE_BANK
		thermal_pm_base2 = of_iomap(pdev->dev.of_node, 1);
		if (!thermal_pm_base || !thermal_pm_base2) {
			dev_err(dev, "failed to map thermal_pm_base (%ld)\n",
					PTR_ERR(thermal_pm_base));
			dev_err(dev, "failed to map thermal_pm_base2 (%ld)\n",
					PTR_ERR(thermal_pm_base2));
			dev_err(dev, "node:%s reg base error\n", node->name);
			return -EINVAL;
		}
		of_property_read_u32_array(node, "reg", tmp, 2);
	}

	SAR_Thermal = devm_iio_channel_get(dev, "adc12");
	if (IS_ERR(SAR_Thermal))
		return PTR_ERR(SAR_Thermal);

	tzdev = devm_thermal_zone_of_sensor_register(&pdev->dev
		, 0, NULL, &mtktv_thermal_of_ops);

	if (IS_ERR(tzdev)) {
		dev_warn(&pdev->dev, "Error registering sensor: %ld\n"
			, PTR_ERR(tzdev));
		return PTR_ERR(tzdev);
	}

	tzdev->ops->notify = notify_callback;

	/* update thermal trim */
	get_VBE_CODE();

#ifdef THERMAL_ZONE_DEBUG
	mtvtv_thermal_procfs_init();
#endif
	return 0;
}

static int mtktv_thermal_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mtktv_thermal_of_match[] = {
	{ .compatible = "cpu-thermal" },
	{},
};
MODULE_DEVICE_TABLE(of, mtktv_thermal_of_match);

static struct platform_driver mtktv_thermal_platdrv = {
	.driver = {
		.name       = "mtk-thermal",
		.owner      = THIS_MODULE,
		.of_match_table = mtktv_thermal_of_match,
	},
	.probe  = mtktv_thermal_probe,
	.remove = mtktv_thermal_remove,
};


static int __init mtktv_thermal_init(void)
{
	platform_driver_register(&mtktv_thermal_platdrv);
	return 0;
}

late_initcall(mtktv_thermal_init);

MODULE_DESCRIPTION("MediaTek TV Thermal driver");
MODULE_AUTHOR("MediaTek Inc.");
MODULE_LICENSE("GPL");
