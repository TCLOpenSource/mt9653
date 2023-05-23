/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2014-2019 MediaTek Inc.
 *
 * Author: Tianping.Fang <tianping.fang@mediatek.com>
 *        Sean Wang <sean.wang@mediatek.com>
 */

#ifndef _LINUX_MFD_MT6397_RTC_H_
#define _LINUX_MFD_MT6397_RTC_H_

#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/rtc.h>

#define RTC_BBPU               0x0000
#define RTC_BBPU_CBUSY         BIT(6)
#define RTC_BBPU_KEY            (0x43 << 8)

#define RTC_WRTGR_MT6358       0x003a
#define RTC_WRTGR_MT6397       0x003c
#define RTC_WRTGR_MT6323       RTC_WRTGR_MT6397

#define RTC_IRQ_STA            0x0002
#define RTC_IRQ_STA_AL         BIT(0)
#define RTC_IRQ_STA_LP         BIT(3)

#define RTC_IRQ_EN             0x0004
#define RTC_IRQ_EN_AL          BIT(0)
#define RTC_IRQ_EN_ONESHOT     BIT(2)
#define RTC_IRQ_EN_LP          BIT(3)
#define RTC_IRQ_EN_ONESHOT_AL  (RTC_IRQ_EN_ONESHOT | RTC_IRQ_EN_AL)

#define RTC_AL_MASK            0x0008
#define RTC_AL_MASK_DOW                BIT(4)

#define RTC_TC_SEC             0x000a
/* Min, Hour, Dom... register offset to RTC_TC_SEC */
#define RTC_OFFSET_SEC         0
#define RTC_OFFSET_MIN         1
#define RTC_OFFSET_HOUR                2
#define RTC_OFFSET_DOM         3
#define RTC_OFFSET_DOW         4
#define RTC_OFFSET_MTH         5
#define RTC_OFFSET_YEAR                6
#define RTC_OFFSET_COUNT       7

#define RTC_TC_SEC_MASK        0x003f
#define RTC_TC_MIN_MASK        0x003f
#define RTC_TC_HOU_MASK        0x001f
#define RTC_TC_DOM_MASK        0x001f
#define RTC_TC_DOW_MASK        0x0007
#define RTC_TC_MTH_MASK        0x000f
#define RTC_TC_YEA_MASK        0x007f

#define RTC_AL_SEC             0x0018
#define RTC_AL_HOU             0x001c
#define RTC_AL_MTH             0x0022

#define RTC_AL_SEC_MASK        0x003f
#define RTC_AL_MIN_MASK        0x003f
#define RTC_AL_HOU_MASK        0x001f
#define RTC_AL_DOM_MASK        0x001f
#define RTC_AL_DOW_MASK        0x0007
#define RTC_AL_MTH_MASK        0x000f
#define RTC_AL_YEA_MASK        0x007f

#define RTC_PDN1               0x002c
#define RTC_PDN1_PWRON_TIME    BIT(7)

#define RTC_PDN2               0x002e
#define RTC_PDN2_PWRON_ALARM   BIT(4)

#define RTC_SPAR0              0x0030

#define RTC_SPAR1              0x0032

#define RTC_MIN_YEAR           1968
#define RTC_BASE_YEAR          1900
#define RTC_NUM_YEARS          128
#define RTC_MIN_YEAR_OFFSET    (RTC_MIN_YEAR - RTC_BASE_YEAR)

#define RTC_PWRON_YEA          RTC_PDN2
#define RTC_PWRON_YEA_MASK     0x7f00
#define RTC_PWRON_YEA_SHIFT    8

#define RTC_PWRON_MTH          RTC_PDN2
#define RTC_PWRON_MTH_MASK     0x000f
#define RTC_PWRON_MTH_SHIFT    0

#define RTC_PWRON_SEC          RTC_SPAR0
#define RTC_PWRON_SEC_MASK     0x003f
#define RTC_PWRON_SEC_SHIFT    0

#define RTC_PWRON_MIN          RTC_SPAR1
#define RTC_PWRON_MIN_MASK     0x003f
#define RTC_PWRON_MIN_SHIFT    0

#define RTC_PWRON_HOU          RTC_SPAR1
#define RTC_PWRON_HOU_MASK     0x07c0
#define RTC_PWRON_HOU_SHIFT    6

#define RTC_PWRON_DOM          RTC_SPAR1
#define RTC_PWRON_DOM_MASK     0xf800
#define RTC_PWRON_DOM_SHIFT    11

#define SPARE_REG_WIDTH        1

#define MTK_RTC_POLL_DELAY_US  10
#define MTK_RTC_POLL_TIMEOUT   (jiffies_to_usecs(HZ))

#define RTC_POFF_ALM_SET	_IOW('p', 0x15, struct rtc_time) /* Set alarm time  */

enum mtk_rtc_spare_enum {
	SPARE_AL_HOU,
	SPARE_AL_MTH,
	SPARE_SPAR0,
	SPARE_RG_MAX,
};

enum rtc_reg_set {
	RTC_REG,
	RTC_MASK,
	RTC_SHIFT
};

struct mtk_rtc_data {
	u32                     wrtgr;
	const struct reg_field *spare_reg_fields;
};

struct mt6397_rtc {
	struct device           *dev;
	struct rtc_device       *rtc_dev;

	/* Protect register access from multiple tasks */
	struct mutex            lock;
	struct regmap           *regmap;
	int                     irq;
	u32                     addr_base;
	const struct mtk_rtc_data *data;
	struct regmap_field     *spare[SPARE_RG_MAX];
};

#endif /* _LINUX_MFD_MT6397_RTC_H_ */
