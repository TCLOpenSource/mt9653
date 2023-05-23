/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/* Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#ifndef _MTK_GMAC_MT5896_H_
#define _MTK_GMAC_MT5896_H_

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/mii.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_net.h>
#include <linux/of_device.h>
#include <linux/dmapool.h>
#include <linux/ethtool.h>
#include <linux/random.h>
#include <soc/mediatek/mtk-pm.h>

#define MTK_GMAC_DRV_VERSION	"1.00"

#define MTK_GMAC_COMPAT_NAME "mediatek,mt5896-gmac"

#define MTK_MAC_REGS_LEN_PER_BANK	(0x40)
/* including gmac0/ogmac1/ogmac2/ogmac3/ogmac4
 * albany0/albany1/albany2/albany3
 * one LEN for 4 bytes
 */
#define MTK_GMAC_REGS_LEN	(9 * 0x40)

/* including emac0/oemac1/oemac2/oemac3
 * albany0/albany1/albany2/albany3
 * one LEN for 4 bytes
 */
#define MTK_EMAC_REGS_LEN	(8 * 0x40)

#define BANK_BASE_MAC_0 (priv->mac_bank_0)
#define BANK_BASE_MAC_1 (priv->mac_bank_1)
#define BANK_BASE_MAC_2 (priv->mac_bank_1 + 0x200)
#define BANK_BASE_MAC_3 (priv->mac_bank_1 + 0x400)
#define BANK_BASE_MAC_4 (priv->mac_bank_1 + 0x600)
#define BANK_BASE_ALBANY_0 (priv->albany_bank_0)
#define BANK_BASE_ALBANY_1 (priv->albany_bank_0 + 0x200)
#define BANK_BASE_ALBANY_2 (priv->albany_bank_0 + 0x400)
#define BANK_BASE_ALBANY_3 (priv->albany_bank_0 + 0x600)
#define BANK_BASE_CLKGEN_00 (priv->clkgen_bank_0)
#define BANK_BASE_CLKGEN_01 (priv->clkgen_bank_0 + 0x200)
#define BANK_BASE_CLKGEN_0A (priv->clkgen_bank_0 + 0x1400)
#define BANK_BASE_CLKGEN_17 (priv->clkgen_bank_0 + 0x2e00)
#define BANK_BASE_CLKGEN_1D (priv->clkgen_bank_0 + 0x3a00)
#define BANK_BASE_CHIP (priv->chip_bank)
#define BANK_BASE_CLKGEN_PM_0 (priv->clkgen_pm_bank_0)
#define BANK_BASE_TZPC_AID_0 (priv->tzpc_aid_bank_0)
#define BANK_BASE_PM_MISC (priv->pm_misc_bank)
#define BANK_BASE_MPLL (priv->mpll_bank)

#define REG_OFFSET_00_L(base) ((base) + 0x0000)
#define REG_OFFSET_00_H(base) ((base) + 0x0001)
#define REG_OFFSET_01_L(base) ((base) + 0x0004)
#define REG_OFFSET_01_H(base) ((base) + 0x0005)
#define REG_OFFSET_02_L(base) ((base) + 0x0008)
#define REG_OFFSET_02_H(base) ((base) + 0x0009)
#define REG_OFFSET_03_L(base) ((base) + 0x000c)
#define REG_OFFSET_03_H(base) ((base) + 0x000d)
#define REG_OFFSET_04_L(base) ((base) + 0x0010)
#define REG_OFFSET_04_H(base) ((base) + 0x0011)
#define REG_OFFSET_05_L(base) ((base) + 0x0014)
#define REG_OFFSET_05_H(base) ((base) + 0x0015)
#define REG_OFFSET_06_L(base) ((base) + 0x0018)
#define REG_OFFSET_06_H(base) ((base) + 0x0019)
#define REG_OFFSET_07_L(base) ((base) + 0x001c)
#define REG_OFFSET_07_H(base) ((base) + 0x001d)
#define REG_OFFSET_08_L(base) ((base) + 0x0020)
#define REG_OFFSET_08_H(base) ((base) + 0x0021)
#define REG_OFFSET_09_L(base) ((base) + 0x0024)
#define REG_OFFSET_09_H(base) ((base) + 0x0025)
#define REG_OFFSET_0A_L(base) ((base) + 0x0028)
#define REG_OFFSET_0A_H(base) ((base) + 0x0029)
#define REG_OFFSET_0B_L(base) ((base) + 0x002c)
#define REG_OFFSET_0B_H(base) ((base) + 0x002d)
#define REG_OFFSET_0C_L(base) ((base) + 0x0030)
#define REG_OFFSET_0C_H(base) ((base) + 0x0031)
#define REG_OFFSET_0D_L(base) ((base) + 0x0034)
#define REG_OFFSET_0D_H(base) ((base) + 0x0035)
#define REG_OFFSET_0E_L(base) ((base) + 0x0038)
#define REG_OFFSET_0E_H(base) ((base) + 0x0039)
#define REG_OFFSET_0F_L(base) ((base) + 0x003c)
#define REG_OFFSET_0F_H(base) ((base) + 0x003d)

#define REG_OFFSET_10_L(base) ((base) + 0x0040)
#define REG_OFFSET_10_H(base) ((base) + 0x0041)
#define REG_OFFSET_11_L(base) ((base) + 0x0044)
#define REG_OFFSET_11_H(base) ((base) + 0x0045)
#define REG_OFFSET_12_L(base) ((base) + 0x0048)
#define REG_OFFSET_12_H(base) ((base) + 0x0049)
#define REG_OFFSET_13_L(base) ((base) + 0x004c)
#define REG_OFFSET_13_H(base) ((base) + 0x004d)
#define REG_OFFSET_14_L(base) ((base) + 0x0050)
#define REG_OFFSET_14_H(base) ((base) + 0x0051)
#define REG_OFFSET_15_L(base) ((base) + 0x0054)
#define REG_OFFSET_15_H(base) ((base) + 0x0055)
#define REG_OFFSET_16_L(base) ((base) + 0x0058)
#define REG_OFFSET_16_H(base) ((base) + 0x0059)
#define REG_OFFSET_17_L(base) ((base) + 0x005c)
#define REG_OFFSET_17_H(base) ((base) + 0x005d)
#define REG_OFFSET_18_L(base) ((base) + 0x0060)
#define REG_OFFSET_18_H(base) ((base) + 0x0061)
#define REG_OFFSET_19_L(base) ((base) + 0x0064)
#define REG_OFFSET_19_H(base) ((base) + 0x0065)
#define REG_OFFSET_1A_L(base) ((base) + 0x0068)
#define REG_OFFSET_1A_H(base) ((base) + 0x0069)
#define REG_OFFSET_1B_L(base) ((base) + 0x006c)
#define REG_OFFSET_1B_H(base) ((base) + 0x006d)
#define REG_OFFSET_1C_L(base) ((base) + 0x0070)
#define REG_OFFSET_1C_H(base) ((base) + 0x0071)
#define REG_OFFSET_1D_L(base) ((base) + 0x0074)
#define REG_OFFSET_1D_H(base) ((base) + 0x0075)
#define REG_OFFSET_1E_L(base) ((base) + 0x0078)
#define REG_OFFSET_1E_H(base) ((base) + 0x0079)
#define REG_OFFSET_1F_L(base) ((base) + 0x007c)
#define REG_OFFSET_1F_H(base) ((base) + 0x007d)

#define REG_OFFSET_20_L(base) ((base) + 0x0080)
#define REG_OFFSET_20_H(base) ((base) + 0x0081)
#define REG_OFFSET_21_L(base) ((base) + 0x0084)
#define REG_OFFSET_21_H(base) ((base) + 0x0085)
#define REG_OFFSET_22_L(base) ((base) + 0x0088)
#define REG_OFFSET_22_H(base) ((base) + 0x0089)
#define REG_OFFSET_23_L(base) ((base) + 0x008c)
#define REG_OFFSET_23_H(base) ((base) + 0x008d)
#define REG_OFFSET_24_L(base) ((base) + 0x0090)
#define REG_OFFSET_24_H(base) ((base) + 0x0091)
#define REG_OFFSET_25_L(base) ((base) + 0x0094)
#define REG_OFFSET_25_H(base) ((base) + 0x0095)
#define REG_OFFSET_26_L(base) ((base) + 0x0098)
#define REG_OFFSET_26_H(base) ((base) + 0x0099)
#define REG_OFFSET_27_L(base) ((base) + 0x009c)
#define REG_OFFSET_27_H(base) ((base) + 0x009d)
#define REG_OFFSET_28_L(base) ((base) + 0x00a0)
#define REG_OFFSET_28_H(base) ((base) + 0x00a1)
#define REG_OFFSET_29_L(base) ((base) + 0x00a4)
#define REG_OFFSET_29_H(base) ((base) + 0x00a5)
#define REG_OFFSET_2A_L(base) ((base) + 0x00a8)
#define REG_OFFSET_2A_H(base) ((base) + 0x00a9)
#define REG_OFFSET_2B_L(base) ((base) + 0x00ac)
#define REG_OFFSET_2B_H(base) ((base) + 0x00ad)
#define REG_OFFSET_2C_L(base) ((base) + 0x00b0)
#define REG_OFFSET_2C_H(base) ((base) + 0x00b1)
#define REG_OFFSET_2D_L(base) ((base) + 0x00b4)
#define REG_OFFSET_2D_H(base) ((base) + 0x00b5)
#define REG_OFFSET_2E_L(base) ((base) + 0x00b8)
#define REG_OFFSET_2E_H(base) ((base) + 0x00b9)
#define REG_OFFSET_2F_L(base) ((base) + 0x00bc)
#define REG_OFFSET_2F_H(base) ((base) + 0x00bd)

#define REG_OFFSET_30_L(base) ((base) + 0x00c0)
#define REG_OFFSET_30_H(base) ((base) + 0x00c1)
#define REG_OFFSET_31_L(base) ((base) + 0x00c4)
#define REG_OFFSET_31_H(base) ((base) + 0x00c5)
#define REG_OFFSET_32_L(base) ((base) + 0x00c8)
#define REG_OFFSET_32_H(base) ((base) + 0x00c9)
#define REG_OFFSET_33_L(base) ((base) + 0x00cc)
#define REG_OFFSET_33_H(base) ((base) + 0x00cd)
#define REG_OFFSET_34_L(base) ((base) + 0x00d0)
#define REG_OFFSET_34_H(base) ((base) + 0x00d1)
#define REG_OFFSET_35_L(base) ((base) + 0x00d4)
#define REG_OFFSET_35_H(base) ((base) + 0x00d5)
#define REG_OFFSET_36_L(base) ((base) + 0x00d8)
#define REG_OFFSET_36_H(base) ((base) + 0x00d9)
#define REG_OFFSET_37_L(base) ((base) + 0x00dc)
#define REG_OFFSET_37_H(base) ((base) + 0x00dd)
#define REG_OFFSET_38_L(base) ((base) + 0x00e0)
#define REG_OFFSET_38_H(base) ((base) + 0x00e1)
#define REG_OFFSET_39_L(base) ((base) + 0x00e4)
#define REG_OFFSET_39_H(base) ((base) + 0x00e5)
#define REG_OFFSET_3A_L(base) ((base) + 0x00e8)
#define REG_OFFSET_3A_H(base) ((base) + 0x00e9)
#define REG_OFFSET_3B_L(base) ((base) + 0x00ec)
#define REG_OFFSET_3B_H(base) ((base) + 0x00ed)
#define REG_OFFSET_3C_L(base) ((base) + 0x00f0)
#define REG_OFFSET_3C_H(base) ((base) + 0x00f1)
#define REG_OFFSET_3D_L(base) ((base) + 0x00f4)
#define REG_OFFSET_3D_H(base) ((base) + 0x00f5)
#define REG_OFFSET_3E_L(base) ((base) + 0x00f8)
#define REG_OFFSET_3E_H(base) ((base) + 0x00f9)
#define REG_OFFSET_3F_L(base) ((base) + 0x00fc)
#define REG_OFFSET_3F_H(base) ((base) + 0x00fd)

#define REG_OFFSET_40_L(base) ((base) + 0x0100)
#define REG_OFFSET_40_H(base) ((base) + 0x0101)
#define REG_OFFSET_41_L(base) ((base) + 0x0104)
#define REG_OFFSET_41_H(base) ((base) + 0x0105)
#define REG_OFFSET_42_L(base) ((base) + 0x0108)
#define REG_OFFSET_42_H(base) ((base) + 0x0109)
#define REG_OFFSET_43_L(base) ((base) + 0x010c)
#define REG_OFFSET_43_H(base) ((base) + 0x010d)
#define REG_OFFSET_44_L(base) ((base) + 0x0110)
#define REG_OFFSET_44_H(base) ((base) + 0x0111)
#define REG_OFFSET_45_L(base) ((base) + 0x0114)
#define REG_OFFSET_45_H(base) ((base) + 0x0115)
#define REG_OFFSET_46_L(base) ((base) + 0x0118)
#define REG_OFFSET_46_H(base) ((base) + 0x0119)
#define REG_OFFSET_47_L(base) ((base) + 0x011c)
#define REG_OFFSET_47_H(base) ((base) + 0x011d)
#define REG_OFFSET_48_L(base) ((base) + 0x0120)
#define REG_OFFSET_48_H(base) ((base) + 0x0121)
#define REG_OFFSET_49_L(base) ((base) + 0x0124)
#define REG_OFFSET_49_H(base) ((base) + 0x0125)
#define REG_OFFSET_4A_L(base) ((base) + 0x0128)
#define REG_OFFSET_4A_H(base) ((base) + 0x0129)
#define REG_OFFSET_4B_L(base) ((base) + 0x012c)
#define REG_OFFSET_4B_H(base) ((base) + 0x012d)
#define REG_OFFSET_4C_L(base) ((base) + 0x0130)
#define REG_OFFSET_4C_H(base) ((base) + 0x0131)
#define REG_OFFSET_4D_L(base) ((base) + 0x0134)
#define REG_OFFSET_4D_H(base) ((base) + 0x0135)
#define REG_OFFSET_4E_L(base) ((base) + 0x0138)
#define REG_OFFSET_4E_H(base) ((base) + 0x0139)
#define REG_OFFSET_4F_L(base) ((base) + 0x013c)
#define REG_OFFSET_4F_H(base) ((base) + 0x013d)

#define REG_OFFSET_50_L(base) ((base) + 0x0140)
#define REG_OFFSET_50_H(base) ((base) + 0x0141)
#define REG_OFFSET_51_L(base) ((base) + 0x0144)
#define REG_OFFSET_51_H(base) ((base) + 0x0145)
#define REG_OFFSET_52_L(base) ((base) + 0x0148)
#define REG_OFFSET_52_H(base) ((base) + 0x0149)
#define REG_OFFSET_53_L(base) ((base) + 0x014c)
#define REG_OFFSET_53_H(base) ((base) + 0x014d)
#define REG_OFFSET_54_L(base) ((base) + 0x0150)
#define REG_OFFSET_54_H(base) ((base) + 0x0151)
#define REG_OFFSET_55_L(base) ((base) + 0x0154)
#define REG_OFFSET_55_H(base) ((base) + 0x0155)
#define REG_OFFSET_56_L(base) ((base) + 0x0158)
#define REG_OFFSET_56_H(base) ((base) + 0x0159)
#define REG_OFFSET_57_L(base) ((base) + 0x015c)
#define REG_OFFSET_57_H(base) ((base) + 0x015d)
#define REG_OFFSET_58_L(base) ((base) + 0x0160)
#define REG_OFFSET_58_H(base) ((base) + 0x0161)
#define REG_OFFSET_59_L(base) ((base) + 0x0164)
#define REG_OFFSET_59_H(base) ((base) + 0x0165)
#define REG_OFFSET_5A_L(base) ((base) + 0x0168)
#define REG_OFFSET_5A_H(base) ((base) + 0x0169)
#define REG_OFFSET_5B_L(base) ((base) + 0x016c)
#define REG_OFFSET_5B_H(base) ((base) + 0x016d)
#define REG_OFFSET_5C_L(base) ((base) + 0x0170)
#define REG_OFFSET_5C_H(base) ((base) + 0x0171)
#define REG_OFFSET_5D_L(base) ((base) + 0x0174)
#define REG_OFFSET_5D_H(base) ((base) + 0x0175)
#define REG_OFFSET_5E_L(base) ((base) + 0x0178)
#define REG_OFFSET_5E_H(base) ((base) + 0x0179)
#define REG_OFFSET_5F_L(base) ((base) + 0x017c)
#define REG_OFFSET_5F_H(base) ((base) + 0x017d)

#define REG_OFFSET_60_L(base) ((base) + 0x0180)
#define REG_OFFSET_60_H(base) ((base) + 0x0181)
#define REG_OFFSET_61_L(base) ((base) + 0x0184)
#define REG_OFFSET_61_H(base) ((base) + 0x0185)
#define REG_OFFSET_62_L(base) ((base) + 0x0188)
#define REG_OFFSET_62_H(base) ((base) + 0x0189)
#define REG_OFFSET_63_L(base) ((base) + 0x018c)
#define REG_OFFSET_63_H(base) ((base) + 0x018d)
#define REG_OFFSET_64_L(base) ((base) + 0x0190)
#define REG_OFFSET_64_H(base) ((base) + 0x0191)
#define REG_OFFSET_65_L(base) ((base) + 0x0194)
#define REG_OFFSET_65_H(base) ((base) + 0x0195)
#define REG_OFFSET_66_L(base) ((base) + 0x0198)
#define REG_OFFSET_66_H(base) ((base) + 0x0199)
#define REG_OFFSET_67_L(base) ((base) + 0x019c)
#define REG_OFFSET_67_H(base) ((base) + 0x019d)
#define REG_OFFSET_68_L(base) ((base) + 0x01a0)
#define REG_OFFSET_68_H(base) ((base) + 0x01a1)
#define REG_OFFSET_69_L(base) ((base) + 0x01a4)
#define REG_OFFSET_69_H(base) ((base) + 0x01a5)
#define REG_OFFSET_6A_L(base) ((base) + 0x01a8)
#define REG_OFFSET_6A_H(base) ((base) + 0x01a9)
#define REG_OFFSET_6B_L(base) ((base) + 0x01ac)
#define REG_OFFSET_6B_H(base) ((base) + 0x01ad)
#define REG_OFFSET_6C_L(base) ((base) + 0x01b0)
#define REG_OFFSET_6C_H(base) ((base) + 0x01b1)
#define REG_OFFSET_6D_L(base) ((base) + 0x01b4)
#define REG_OFFSET_6D_H(base) ((base) + 0x01b5)
#define REG_OFFSET_6E_L(base) ((base) + 0x01b8)
#define REG_OFFSET_6E_H(base) ((base) + 0x01b9)
#define REG_OFFSET_6F_L(base) ((base) + 0x01bc)
#define REG_OFFSET_6F_H(base) ((base) + 0x01bd)

#define REG_OFFSET_70_L(base) ((base) + 0x01c0)
#define REG_OFFSET_70_H(base) ((base) + 0x01c1)
#define REG_OFFSET_71_L(base) ((base) + 0x01c4)
#define REG_OFFSET_71_H(base) ((base) + 0x01c5)
#define REG_OFFSET_72_L(base) ((base) + 0x01c8)
#define REG_OFFSET_72_H(base) ((base) + 0x01c9)
#define REG_OFFSET_73_L(base) ((base) + 0x01cc)
#define REG_OFFSET_73_H(base) ((base) + 0x01cd)
#define REG_OFFSET_74_L(base) ((base) + 0x01d0)
#define REG_OFFSET_74_H(base) ((base) + 0x01d1)
#define REG_OFFSET_75_L(base) ((base) + 0x01d4)
#define REG_OFFSET_75_H(base) ((base) + 0x01d5)
#define REG_OFFSET_76_L(base) ((base) + 0x01d8)
#define REG_OFFSET_76_H(base) ((base) + 0x01d9)
#define REG_OFFSET_77_L(base) ((base) + 0x01dc)
#define REG_OFFSET_77_H(base) ((base) + 0x01dd)
#define REG_OFFSET_78_L(base) ((base) + 0x01e0)
#define REG_OFFSET_78_H(base) ((base) + 0x01e1)
#define REG_OFFSET_79_L(base) ((base) + 0x01e4)
#define REG_OFFSET_79_H(base) ((base) + 0x01e5)
#define REG_OFFSET_7A_L(base) ((base) + 0x01e8)
#define REG_OFFSET_7A_H(base) ((base) + 0x01e9)
#define REG_OFFSET_7B_L(base) ((base) + 0x01ec)
#define REG_OFFSET_7B_H(base) ((base) + 0x01ed)
#define REG_OFFSET_7C_L(base) ((base) + 0x01f0)
#define REG_OFFSET_7C_H(base) ((base) + 0x01f1)
#define REG_OFFSET_7D_L(base) ((base) + 0x01f4)
#define REG_OFFSET_7D_H(base) ((base) + 0x01f5)
#define REG_OFFSET_7E_L(base) ((base) + 0x01f8)
#define REG_OFFSET_7E_H(base) ((base) + 0x01f9)
#define REG_OFFSET_7F_L(base) ((base) + 0x01fc)
#define REG_OFFSET_7F_H(base) ((base) + 0x01fd)

#define RX_DESC_SIZE_DEFAULT 16
#define RX_DESC_NUM_DEFAULT 64
/* rx descriptor pointer must be 16K alignment */
#define RX_DESC_PTR_ALIGNMENT 16384
#define RX_DESC_DONE 0x00000001UL
#define RX_NAPI_WEIGHT 64
/* tx memcpy */
#define TX_MEMCPY_SIZE_DEFAULT MAC_TX_MAX_LEN
#define TX_MEMCPY_NUM_DEFAULT 8
#define TX_MEMCPY_PTR_ALIGNMENT 2
#define MAC_MTU 1518
/* hw limit tx max length */
#define MAC_TX_MAX_LEN 1580
/* hw limit rx max length */
#define MAC_RX_MAX_LEN 1522
#define MAC_EXTRA_PKT_LEN 36
#define MAC_UPDATE_LINK_TIME msecs_to_jiffies(1000)
#define MAC_TX_BUFF_CHECK_RETRY_MAX 1000

/* eth driver stage */
#define ETH_DRV_STAGE_OPEN (0x1UL)
#define ETH_DRV_STAGE_SUSPEND (0x1UL << 1)
#define ETH_DRV_STAGE_SUSPEND_OPEN (0x1UL << 2)

/* TODO: should we take care of them? */
#define MIU0_BUS_BASE 0x20000000UL
#define MIU1_BUS_BASE 0xA0000000UL

/* registers */
#define IRQ_BIT_DONE (0x1UL)
#define IRQ_BIT_RCOM (0x1UL << 1)
#define IRQ_BIT_RBNA (0x1UL << 2)
#define IRQ_BIT_TOVR (0x1UL << 3)
#define IRQ_BIT_TUND (0x1UL << 4)
#define IRQ_BIT_RTRY (0x1UL << 5)
#define IRQ_BIT_TBRE (0x1UL << 6)
#define IRQ_BIT_TCOM (0x1UL << 7)
#define IRQ_BIT_TIDLE (0x1UL << 8)
#define IRQ_BIT_LINK (0x1UL << 9)
#define IRQ_BIT_ROVR (0x1UL << 10)
#define IRQ_BIT_HRESP (0x1UL << 11)
#define IRQ_BIT_EEE (0x1UL << 12)

#define IRQ_RSR_DNA (0x1UL)
#define IRQ_RSR_REC (0x1UL << 1)
#define IRQ_RSR_OVR_RSR (0x1UL << 2)
#define IRQ_RSR_BNA (0x1UL << 3)

#define IRQ_TSR_OVER (0x1UL)
#define IRQ_TSR_COL (0x1UL << 1)
#define IRQ_TSR_RLE (0x1UL << 2)
#define IRQ_TSR_IDLE (0x1UL << 3)
#define IRQ_TSR_BNQ (0x1UL << 4)
#define IRQ_TSR_COMP (0x1UL << 5)
#define IRQ_TSR_UND (0x1UL << 6)
#define IRQ_TSR_TBNQ (0x1UL << 7)
#define IRQ_TSR_FBNQ (0x1UL << 8)
#define IRQ_TSR_FIFO1_IDLE (0x1UL << 9)
#define IRQ_TSR_FIFO2_IDLE (0x1UL << 10)
#define IRQ_TSR_FIFO3_IDLE (0x1UL << 11)
#define IRQ_TSR_FIFO4_IDLE (0x1UL << 12)

/* rbna/tovr/tund/rtry/rovr */
#define IRQ_ENABLE_BIT 0x43CUL
/* rbna/tovr/tund/rtry/rovr/tcom */
//#define IRQ_ENABLE_BIT 0x4BCUL
#define IRQ_DISABLE_ALL 0xFFFFFFFFUL

/* rx delay interrupt setting */
#define IRQ_RX_DELAY_SET 0x0101UL

/* mac descriptor */
#define MAC_DESC_PKT_TYPE_BIT2 (0x1UL << 19)
#define MAC_DESC_PKT_TYPE_BIT1 (0x1UL << 18)
#define MAC_DESC_PKT_TYPE_BIT0 (0x1UL << 11)
#define MAC_DESC_TCP_UDP_CSUM (0x1UL << 20)
#define MAC_DESC_IP_CSUM (0x1UL << 21)

/* slt-test */
#define SIOC_MTK_MAC_SLT (SIOCDEVPRIVATE + 1)
/* compliance test */
#define SIOC_MTK_MAC_CTS (SIOCDEVPRIVATE + 2)

/* wol */
#define SIOC_SET_WOL_CMD (SIOCDEVPRIVATE + 13)
#define SIOC_GET_WOL_CMD (SIOCDEVPRIVATE + 14)

/* woc */
#define UDP_PROTOCOL 17
#define TCP_PROTOCOL 6
#define SIOC_SET_WOC_CMD (SIOCDEVPRIVATE + 11)
#define SIOC_CLR_WOC_CMD (SIOCDEVPRIVATE + 12)

#define WOC_PATTERN_BYTES_MAX 128
#define WOC_FILTER_NUM_MAX 20
#define WOC_PATTERN_CHECK_LEN_MAX 38

#define ETH_PROTOCOL_NUM_OFFSET 23
#define ETH_PROTOCOL_DEST_PORT_H_OFFSET 36
#define ETH_PROTOCOL_DEST_PORT_L_OFFSET 37

#define REG_EPHY_ALBANY2                0x0034UL
#define REG_WOL_MODE_OFFSET             0x46UL
#define BIT_WOL_MODE_WOL_EN             0
#define BIT_WOL_MODE_WOL_PASS		4
#define BIT_WOL_MODE_WOL_CLEAR          7
#define REG_EPHY_ALBANY3                0x0035UL
#define REG_WOC_CTRL_L_OFFSET           0x00UL
#define BIT_WOC_CTRL_L_WOL_EN           0
#define BIT_WOC_CTRL_L_STORAGE_EN       1
#define BIT_WOC_CTRL_L_WOL_WRDY         2
#define REG_WOC_CTRL_H_OFFSET           0x01UL
#define BIT_WOC_CTRL_H_WOL_INT_CLEAR    0
#define BIT_WOC_CTRL_H_SRAM0_WE         1
#define BIT_WOC_CTRL_H_SRAM0_ADDR_LATCH 2
#define BIT_WOC_CTRL_H_WOL_WRDY_CR      3
#define BIT_WOC_CTRL_H_WOL_STORAGE_AUTOLOAD  7
#define REG_WOC_CHK_LENGTH_0_OFFSET     0x02UL
#define REG_WOC_PAT_EN_0_7_OFFSET       0x16UL
#define REG_WOC_PAT_EN_8_15_OFFSET      0x17UL
#define REG_WOC_PAT_EN_16_19_OFFSET     0x18UL
#define REG_WOC_SRAM_STATE_OFFSET       0x1AUL
#define REG_WOC_SRAM_CTRL_OFFSET        0x1BUL
#define REG_WOC_DET_LA_0_7_OFFSET       0x1CUL
#define REG_WOC_DET_LA_8_15_OFFSET      0x1DUL
#define REG_WOC_DET_LA_16_19_OFFSET     0x1EUL
#define REG_WOC_SRAM0_DOUT_OFFSET       0x20UL
#define REG_WOC_PKT_BUFF_INDEX_OFFSET   0x5BUL
#define REG_WOC_PKT_BUFF_PTR_OFFSET     0x9AUL
#define REG_WOC_PKT_BUFF_DATA_L_OFFSET  0x9EUL
#define REG_WOC_PKT_BUFF_DATA_H_OFFSET  0x9FUL
#define MAX_WOC_PKT_BUFF_DATA_LEN       126

#define MACADDR_FORMAT "XX:XX:XX:XX:XX:XX"

#define SLT_UPPER_BOUND 10000
#define SLT_LOWER_BOUND 0
#define SLT_PKT_LEN 512
#define WOC_COUNT_UPPER_BOUND 20
#define WOC_COUNT_LOWER_BOUND 1

#define PM_WK_REASON_NAME_SIZE (10)
#define GPHY_TUNE_DTS_NAME_SIZE (50)

struct ioctl_woc_para_cmd {
	u8 protocol_type;
	u8 port_count;
	u32 *port_array;
};

#ifdef CONFIG_COMPAT
struct ioctl_woc_para_cmd32 {
	u8 protocol_type;
	u8 port_count;
	u32 port_array;
};
#endif /* CONFIG_COMPAT */
/* end of woc */

/* wol */
struct ioctl_wol_para_cmd {
	bool is_enable_wol;
};

struct mtk_dtv_gmac_reg_ops {
	void (*write_phy)(struct net_device *ndev,
			  u8 phy_addr, u8 reg_addr, u32 val);
	u32 (*read_phy)(struct net_device *ndev, u8 phy_addr, u8 reg_addr);
	void (*phy_hw_init)(struct net_device *ndev, bool is_suspend);
};

struct rx_desc_gmac {
	u32 addr;
	u32 size;
	u32 size_high;
	u32 reserved;
};

struct rx_desc_emac {
	u32 addr;
	u32 size;
};

enum gmac_ethpll_ictrl {
	ETHPLL_ICTRL_MIN = 0,
	ETHPLL_ICTRL_MT5896 = ETHPLL_ICTRL_MIN,
	ETHPLL_ICTRL_MT5897,
	ETHPLL_ICTRL_MAX
};

enum gmac_phy_type {
	PHY_TYPE_MIN = 0,
	PHY_TYPE_INTERNAL = PHY_TYPE_MIN,
	PHY_TYPE_EXTERNAL,
	PHY_TYPE_MAX
};

enum gmac_mac_type {
	MAC_TYPE_MIN = 0,
	MAC_TYPE_GMAC = MAC_TYPE_MIN,
	MAC_TYPE_EMAC,
	MAC_TYPE_MAX
};

enum gmac_phy_process {
	PHY_PROCESS_MIN = 0,
	PHY_PROCESS_7_12 = PHY_PROCESS_MIN,
	PHY_PROCESS_22,
	PHY_PROCESS_MAX
};

struct mtk_dtv_gmac_private {
	struct net_device *ndev;
	struct platform_device *pdev;
	struct device *dev;
	//struct net_device_stats stats;
	struct napi_struct napi;
	void __iomem *mac_bank_0;
	void __iomem *mac_bank_1;
	void __iomem *albany_bank_0;
	void __iomem *clkgen_bank_0;
	void __iomem *chip_bank;
	void __iomem *clkgen_pm_bank_0;
	void __iomem *tzpc_aid_bank_0;
	void __iomem *pm_misc_bank;
	void __iomem *mpll_bank;
	struct mtk_dtv_gmac_reg_ops *reg_ops;

	dma_addr_t rx_desc_dma_addr;
	uintptr_t rx_desc_vir_addr;
	u32 rx_desc_size;
	u32 rx_desc_num;
	u32 rx_desc_index;
	void *rx_desc;
	struct dma_pool *rx_desc_dma_pool;
	struct sk_buff_head rx_skb_q;
	struct napi_struct napi_rx;

	/* tx memcpy */
	dma_addr_t tx_memcpy_dma_addr;
	uintptr_t tx_memcpy_vir_addr;
	u32 tx_memcpy_index;
	struct dma_pool *tx_memcpy_dma_pool;

	bool is_internal_phy;
	bool is_force_10m;
	bool is_force_100m;
	bool is_fpga_haps;
	u8 phy_addr;
	u32 phy_restart_cnt;
	struct timer_list link_timer;
	/* for transmission */
	spinlock_t tx_lock;
	/* for wol_magic */
	spinlock_t magic_lock;

	/* slt-test */
	bool is_slt_test_running;
	bool is_slt_test_passed;

	/* wol */
	bool is_wol;

	/* woc */
	bool is_woc;
	u8 woc_filter_index;
	u8 woc_sram_pattern[20][128];

	/* eth driver stage */
	u32 eth_drv_stage;

	/* rx error irq state */
	bool is_rx_rbna_irq_masked;
	bool is_rx_rovr_irq_masked;

	enum gmac_ethpll_ictrl ethpll_ictrl_type;
	enum gmac_phy_type phy_type;
	enum gmac_mac_type mac_type;
	enum gmac_phy_process phy_process;

	/* ethtool */
	struct mii_if_info mii;
};

void mtk_dtv_gmac_slt_rx(struct net_device *ndev, struct sk_buff *skb, int len);
int mtk_dtv_gmac_slt_ioctl(struct net_device *ndev, struct ifreq *req);
int mtk_dtv_gmac_cts_ioctl(struct net_device *ndev, struct ifreq *req);
void mtk_dtv_gmac_skb_dump(phys_addr_t addr, u32 len);
int mtk_dtv_gmac_hw_init(struct net_device *ndev, bool is_suspend);
int mtk_dtv_gmac_sysfs_create(struct net_device *ndev);
void mtk_dtv_gmac_sysfs_remove(struct net_device *ndev);
extern int mtk_mac_wol_set(bool enable_wol);
extern int mtk_mac_wol_get(bool *enable_wol);
#endif
