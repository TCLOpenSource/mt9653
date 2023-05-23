// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/* Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */
#include <linux/compat.h>

#include "mtk_gmac_mt5896_coda.h"
#include "mtk_gmac_mt5896_riu.h"
#include "mtk_gmac_mt5896.h"

static u8 mac_addr_env[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static u8 mac_addr_dts[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const char mtk_gmac_driver_version[] = MTK_GMAC_DRV_VERSION;

static void mtk_dtv_gmac_set_rx_mode(struct net_device *ndev);

/* tools */
static void mtk_dtv_gmac_phy_hw_init_7_12(struct net_device *ndev, bool is_suspend)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u8 reg_read;

	//wriu     0x0421b4             0x02
	reg_read = 0x02;
	mtk_writeb(REG_OFFSET_5A_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04214f             0x02
	reg_read = 0x02;
	mtk_writeb(REG_OFFSET_27_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042151             0x01
	reg_read = 0x01;
	mtk_writeb(REG_OFFSET_28_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042177             0x18
	reg_read = 0x18;
	mtk_writeb(REG_OFFSET_3B_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042072             0xa0
	reg_read = 0xa0;
	mtk_writeb(REG_OFFSET_39_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0421fc             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7E_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0421fd             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0422a1             0x80
	//reg_read = 0x80;
	//mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422fa             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7D_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422fb             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7D_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04228e             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_47_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04228f             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_47_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422dc             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_6E_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422dd             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_6E_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422d4             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_6A_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422d5             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_6A_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422d8             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_6C_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422d9             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_6C_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422f6             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7B_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422f7             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7B_H(BANK_BASE_ALBANY_2), reg_read);

	if (priv->ethpll_ictrl_type == ETHPLL_ICTRL_MT5897) {
		//wriu     0x0422f8             0x02
		reg_read = 0x02;
		mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), reg_read);

		//wriu     0x04228c             0x01
		reg_read = 0x01;
		mtk_writeb(REG_OFFSET_46_L(BANK_BASE_ALBANY_2), reg_read);

		//wriu     0x0422d2             0xc4
		reg_read = 0xc4;
		mtk_writeb(REG_OFFSET_69_L(BANK_BASE_ALBANY_2), reg_read);

		//wriu     0x0422f4             0x03
		reg_read = 0x03;
		mtk_writeb(REG_OFFSET_7A_L(BANK_BASE_ALBANY_2), reg_read);
	} else {
		/* default ethpll ictrl settings */
		//wriu     0x0422f8             0x12
		reg_read = 0x12;
		mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), reg_read);

		//wriu     0x04228c             0x11
		reg_read = 0x11;
		mtk_writeb(REG_OFFSET_46_L(BANK_BASE_ALBANY_2), reg_read);

		//wriu     0x0422d2             0xd4
		reg_read = 0xd4;
		mtk_writeb(REG_OFFSET_69_L(BANK_BASE_ALBANY_2), reg_read);

		//wriu     0x0422f4             0x13
		reg_read = 0x13;
		mtk_writeb(REG_OFFSET_7A_L(BANK_BASE_ALBANY_2), reg_read);
	}

	//wriu     0x0421cc             0x40
	reg_read = 0x40;
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0421bb             0x04
	reg_read = 0x04;
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0422aa             0x06
	reg_read = 0x06;
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04223a             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422f1             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04228a             0x01
	reg_read = 0x01;
	mtk_writeb(REG_OFFSET_45_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04213b             0x01
	reg_read = 0x01;
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042280             0x30
	reg_read = 0x30;
	mtk_writeb(REG_OFFSET_40_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422c5             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_62_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x042230             0x43
	reg_read = 0x43;
	mtk_writeb(REG_OFFSET_18_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x042239             0x41
	reg_read = 0x41;
	mtk_writeb(REG_OFFSET_1C_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422f2             0xf5
	reg_read = 0xf5;
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), reg_read);

	if (priv->ethpll_ictrl_type == ETHPLL_ICTRL_MT5896) {
		/* turn on tx always: 0422_79[8]=0 (16 bit) */
		reg_read = 0x0c;
		mtk_writeb(REG_OFFSET_79_H(BANK_BASE_ALBANY_2), reg_read);
	} else {
		/* default settings */
		//wriu     0x0422f3             0x0d
		reg_read = 0x0d;
		mtk_writeb(REG_OFFSET_79_H(BANK_BASE_ALBANY_2), reg_read);
	}

	//wriu     0x042079             0xd0
	reg_read = 0xd0;
	mtk_writeb(REG_OFFSET_3C_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x042077             0x5a
	reg_read = 0x5a;
	mtk_writeb(REG_OFFSET_3B_H(BANK_BASE_ALBANY_0), reg_read);

	if (priv->is_internal_phy) {
		//wriu     0x04202d             0x7c
		reg_read = 0x7c;
		mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), reg_read);
	} else {
		//wriu     0x04202d             0x4c
		reg_read = 0x4c;
		mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), reg_read);
	}

	//wriu     0x0422e8             0x06
	reg_read = 0x06;
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04202b             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0422e8             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04202b             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0422e8             0x06
	reg_read = 0x06;
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0420aa             0x1a
	reg_read = 0x1a;
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420ac             0x1a
	reg_read = 0x1a;
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420ad             0x1a
	reg_read = 0x1a;
	mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420ae             0x1a
	reg_read = 0x1a;
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420af             0x1a
	reg_read = 0x1a;
	mtk_writeb(REG_OFFSET_57_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0422e8             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0420aa             0x1a
	reg_read = 0x1a;
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420ab             0x28
	reg_read = 0x28;
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0421f5             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7A_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04210f             0xc9
	reg_read = 0xc9;
	mtk_writeb(REG_OFFSET_07_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042189             0x50
	reg_read = 0x50;
	mtk_writeb(REG_OFFSET_44_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04218b             0x80
	reg_read = 0x80;
	mtk_writeb(REG_OFFSET_45_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04218e             0x0e
	reg_read = 0x0e;
	mtk_writeb(REG_OFFSET_47_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042190             0x04
	reg_read = 0x04;
	mtk_writeb(REG_OFFSET_48_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0422ad             0x0e
	//reg_read = 0x0e;
	//mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422ae             0x01
	reg_read = 0x01;
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422af             0x01
	reg_read = 0x01;
	mtk_writeb(REG_OFFSET_57_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu    0x0420b4              0x5a
	reg_read = 0x5a;
	mtk_writeb(REG_OFFSET_5A_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu    0x0420b6              0x50
	reg_read = 0x50;
	mtk_writeb(REG_OFFSET_5B_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu    0x0420ff              0x1a
	reg_read = 0x1a;
	mtk_writeb(REG_OFFSET_7F_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu    0x042293              0x04
	reg_read = 0x04;
	mtk_writeb(REG_OFFSET_49_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu    0x0422ec              0x10
	reg_read = 0x10;
	mtk_writeb(REG_OFFSET_76_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu    0x042294              0x18
	reg_read = 0x18;
	mtk_writeb(REG_OFFSET_4A_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu    0x042133              0x55
	reg_read = 0x55;
	mtk_writeb(REG_OFFSET_19_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu    0x042110              0x56
	reg_read = 0x56;
	mtk_writeb(REG_OFFSET_08_L(BANK_BASE_ALBANY_1), reg_read);

	if (!is_suspend) {
		//wriu     0x0422a1             0x00
		reg_read = 0x00;
		mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), reg_read);
	}

	//wriu     0x0422a1             0x80
	reg_read = 0x80;
	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), reg_read);
};

static void mtk_dtv_gmac_phy_hw_init_22(struct net_device *ndev, bool is_suspend)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u8 reg_read;

	//wriu     0x0421b4             0x02
	reg_read = 0x02;
	mtk_writeb(REG_OFFSET_5A_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04214f             0x02
	reg_read = 0x02;
	mtk_writeb(REG_OFFSET_27_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042151             0x01
	reg_read = 0x01;
	mtk_writeb(REG_OFFSET_28_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042177             0x18
	reg_read = 0x18;
	mtk_writeb(REG_OFFSET_3B_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042072             0xa0
	reg_read = 0xa0;
	mtk_writeb(REG_OFFSET_39_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0421fc             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7E_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0421fd             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0422a1             0x80
	reg_read = 0x80;
	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0421cc             0x40
	reg_read = 0x40;
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0421bb             0x04
	reg_read = 0x04;
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04223a             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422f1             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04228a             0x01
	reg_read = 0x01;
	mtk_writeb(REG_OFFSET_45_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04213b             0x01
	reg_read = 0x01;
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0421c4             0x044
	reg_read = 0x44;
	mtk_writeb(REG_OFFSET_62_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042280             0x30
	reg_read = 0x30;
	mtk_writeb(REG_OFFSET_40_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422c5             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_62_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x042230             0x43
	reg_read = 0x43;
	mtk_writeb(REG_OFFSET_18_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x042239             0x41
	reg_read = 0x41;
	mtk_writeb(REG_OFFSET_1C_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422f2             0xf5
	reg_read = 0xf5;
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0422f3             0x0d
	reg_read = 0x0d;
	mtk_writeb(REG_OFFSET_79_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x042079             0xd0
	reg_read = 0xd0;
	mtk_writeb(REG_OFFSET_3C_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x042077             0x5a
	reg_read = 0x5a;
	mtk_writeb(REG_OFFSET_3B_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x04202d             0x7c
	reg_read = 0x7c;
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0422e8             0x06
	reg_read = 0x06;
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04202b             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0422e8             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x04202b             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0422e8             0x06
	reg_read = 0x06;
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0420aa             0x19
	reg_read = 0x19;
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420ac             0x19
	reg_read = 0x19;
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420ad             0x19
	reg_read = 0x19;
	mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420ae             0x19
	reg_read = 0x19;
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420af             0x19
	reg_read = 0x19;
	mtk_writeb(REG_OFFSET_57_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0422e8             0x00
	reg_read = 0x00;
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu     0x0420aa             0x19
	reg_read = 0x19;
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0420ab             0x28
	reg_read = 0x28;
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu     0x0421f5             0x02
	reg_read = 0x02;
	mtk_writeb(REG_OFFSET_7A_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04210f             0xc9
	reg_read = 0xc9;
	mtk_writeb(REG_OFFSET_07_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042189             0x50
	reg_read = 0x50;
	mtk_writeb(REG_OFFSET_44_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04218b             0x80
	reg_read = 0x80;
	mtk_writeb(REG_OFFSET_45_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x04218e             0x0e
	reg_read = 0x0e;
	mtk_writeb(REG_OFFSET_47_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x042190             0x04
	reg_read = 0x04;
	mtk_writeb(REG_OFFSET_48_L(BANK_BASE_ALBANY_1), reg_read);

	//wriu     0x0422ad             0x0e
	//reg_read = 0x0e;
	//mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu    0x0420b4              0x5a
	reg_read = 0x5a;
	mtk_writeb(REG_OFFSET_5A_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu    0x0420b6              0x50
	reg_read = 0x50;
	mtk_writeb(REG_OFFSET_5B_L(BANK_BASE_ALBANY_0), reg_read);

	//wriu    0x0420ff              0x1a
	reg_read = 0x1a;
	mtk_writeb(REG_OFFSET_7F_H(BANK_BASE_ALBANY_0), reg_read);

	//wriu    0x042293              0x04
	reg_read = 0x04;
	mtk_writeb(REG_OFFSET_49_H(BANK_BASE_ALBANY_2), reg_read);

	//wriu    0x0422ec              0x10
	reg_read = 0x10;
	mtk_writeb(REG_OFFSET_76_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu    0x042294              0x18
	reg_read = 0x18;
	mtk_writeb(REG_OFFSET_4A_L(BANK_BASE_ALBANY_2), reg_read);

	//wriu    0x042133              0x55
	reg_read = 0x55;
	mtk_writeb(REG_OFFSET_19_H(BANK_BASE_ALBANY_1), reg_read);

	//wriu    0x042110              0x56
	reg_read = 0x56;
	mtk_writeb(REG_OFFSET_08_L(BANK_BASE_ALBANY_1), reg_read);
};

static void mtk_dtv_gmac_hw_power_on_clk(struct net_device *ndev, bool is_suspend)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u8 reg_read;

	/* (BK_103_3E4[3:0]) = 0, 0x1037c8 */
	reg_read = mtk_readb(REG_OFFSET_64_L(BANK_BASE_CLKGEN_17));
	reg_read &= ~(0x0F);
	mtk_writeb(REG_OFFSET_64_L(BANK_BASE_CLKGEN_17), reg_read);
	/* (BK_103_3E4[9:8]) = 0, 0x1037c9 */
	reg_read = mtk_readb(REG_OFFSET_64_H(BANK_BASE_CLKGEN_17));
	reg_read &= ~(0x03);
	mtk_writeb(REG_OFFSET_64_H(BANK_BASE_CLKGEN_17), reg_read);
	/* ??, 0x1037cc */
	reg_read = mtk_readb(REG_OFFSET_66_L(BANK_BASE_CLKGEN_17));
	reg_read &= ~(0x0F);
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_CLKGEN_17), reg_read);
	/* (BK_103_3E6[9:8]) = 0, 0x1037cd */
	reg_read = mtk_readb(REG_OFFSET_66_H(BANK_BASE_CLKGEN_17));
	reg_read &= ~(0x03);
	mtk_writeb(REG_OFFSET_66_H(BANK_BASE_CLKGEN_17), reg_read);
	/* ??, 0x1037d0 */
	reg_read = mtk_readb(REG_OFFSET_68_L(BANK_BASE_CLKGEN_17));
	reg_read &= ~(0x0F);
	mtk_writeb(REG_OFFSET_68_L(BANK_BASE_CLKGEN_17), reg_read);
	/* (BK_103_3E8[9:8]) = 0, 0x1037d1 */
	reg_read = mtk_readb(REG_OFFSET_68_H(BANK_BASE_CLKGEN_17));
	reg_read &= ~(0x03);
	mtk_writeb(REG_OFFSET_68_H(BANK_BASE_CLKGEN_17), reg_read);

	/* (BK_103_6AB[0]) = 1, 0x103d56 */
	reg_read = mtk_readb(REG_OFFSET_2B_L(BANK_BASE_CLKGEN_1D));
	reg_read |= 0x01;
	mtk_writeb(REG_OFFSET_2B_L(BANK_BASE_CLKGEN_1D), reg_read);
	//reg_read = 0x00;
	//mtk_writeb(REG_OFFSET_2B_H(BANK_BASE_CLKGEN_1D), reg_read);
	/* ??, 0x103d58 */
	reg_read = mtk_readb(REG_OFFSET_2C_L(BANK_BASE_CLKGEN_1D));
	reg_read |= 0x01;
	mtk_writeb(REG_OFFSET_2C_L(BANK_BASE_CLKGEN_1D), reg_read);
	//reg_read = 0x00;
	//mtk_writeb(REG_OFFSET_2C_H(BANK_BASE_CLKGEN_1D), reg_read);
	/* ??, 0x103d5a */
	reg_read = mtk_readb(REG_OFFSET_2D_L(BANK_BASE_CLKGEN_1D));
	reg_read |= 0x01;
	mtk_writeb(REG_OFFSET_2D_L(BANK_BASE_CLKGEN_1D), reg_read);
	//reg_read = 0x00;
	//mtk_writeb(REG_OFFSET_2D_H(BANK_BASE_CLKGEN_1D), reg_read);
	/* ??, 0x103d5c */
	reg_read = mtk_readb(REG_OFFSET_2E_L(BANK_BASE_CLKGEN_1D));
	reg_read |= 0x01;
	mtk_writeb(REG_OFFSET_2E_L(BANK_BASE_CLKGEN_1D), reg_read);
	//reg_read = 0x00;
	//mtk_writeb(REG_OFFSET_2E_H(BANK_BASE_CLKGEN_1D), reg_read);
	/* ??, 0x102aac */
	reg_read = mtk_readb(REG_OFFSET_56_L(BANK_BASE_CLKGEN_0A));
	reg_read |= 0x04;
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_CLKGEN_0A), reg_read);
	//reg_read = 0x00;
	//mtk_writeb(REG_OFFSET_56_H(BANK_BASE_CLKGEN_0A), reg_read);
	/* ??, 0x102140 */
	reg_read = mtk_readb(REG_OFFSET_20_L(BANK_BASE_CLKGEN_01));
	reg_read &= ~(0x03);
	mtk_writeb(REG_OFFSET_20_L(BANK_BASE_CLKGEN_01), reg_read);
	/* ??, 0x102141 */
	reg_read = mtk_readb(REG_OFFSET_20_H(BANK_BASE_CLKGEN_01));
	reg_read &= ~(0x03);
	mtk_writeb(REG_OFFSET_20_H(BANK_BASE_CLKGEN_01), reg_read);
	/* ??, 0x103dbb */
	reg_read = mtk_readb(REG_OFFSET_5D_H(BANK_BASE_CLKGEN_1D));
	reg_read |= 0x2;
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_CLKGEN_1D), reg_read);
	/* ??, 0x102a73 */
	reg_read = mtk_readb(REG_OFFSET_39_H(BANK_BASE_CLKGEN_0A));
	reg_read |= 0x10;
	mtk_writeb(REG_OFFSET_39_H(BANK_BASE_CLKGEN_0A), reg_read);

	if (priv->is_internal_phy) {
		/* BK_0C_36[3:0] = 0x0, 0x000c6c */
		reg_read = mtk_readb(REG_OFFSET_36_L(BANK_BASE_CLKGEN_PM_0));
		reg_read &= ~(0x0F);
		mtk_writeb(REG_OFFSET_36_L(BANK_BASE_CLKGEN_PM_0), reg_read);

		/* BK_0C_35[3:0] = 0x0, 0x000c6a */
		reg_read = mtk_readb(REG_OFFSET_35_L(BANK_BASE_CLKGEN_PM_0));
		reg_read &= ~(0x0F);
		mtk_writeb(REG_OFFSET_35_L(BANK_BASE_CLKGEN_PM_0), reg_read);
	} else {
		/* BK_0C_36[3:0] = 0x4, 0x000c6c */
		reg_read = mtk_readb(REG_OFFSET_36_L(BANK_BASE_CLKGEN_PM_0));
		reg_read &= ~(0x0F);
		reg_read |= 0x04;
		mtk_writeb(REG_OFFSET_36_L(BANK_BASE_CLKGEN_PM_0), reg_read);

		/* BK_0C_35[3:0] = 0x4, 0x000c6a */
		reg_read = mtk_readb(REG_OFFSET_35_L(BANK_BASE_CLKGEN_PM_0));
		reg_read &= ~(0x0F);
		reg_read |= 0x04;
		mtk_writeb(REG_OFFSET_35_L(BANK_BASE_CLKGEN_PM_0), reg_read);
	}

	reg_ops->phy_hw_init(ndev, is_suspend);
}

//static void mtk_dtv_gmac_hw_power_down_clk(struct net_device *ndev)
//{
//	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	//wriu     0x042104             0x31
//	mtk_writeb(REG_OFFSET_02_L(BANK_BASE_ALBANY_1), 0x31);
	//wriu     0x0421fc             0x02
//	mtk_writeb(REG_OFFSET_7E_L(BANK_BASE_ALBANY_1), 0x02);
	//wriu     0x0421fd             0x01
//	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x0422a1             0x30
//	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), 0x30);
	//wriu     0x0421cc             0x50
//	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), 0x50);
	//wriu     0x0421bb             0xc4
//	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), 0xc4);
	//wriu     0x04223a             0xf3
//	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), 0xf3);
	//wriu     0x0422f1             0x3c
//	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), 0x3c);
	//wriu     0x0422f3             0x0f
//	mtk_writeb(REG_OFFSET_79_H(BANK_BASE_ALBANY_2), 0x0f);
	//wriu     0x0422c5             0x40
//	mtk_writeb(REG_OFFSET_62_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x042230             0x53
//	mtk_writeb(REG_OFFSET_18_L(BANK_BASE_ALBANY_2), 0x53);
	//wriu     0x0422f7             0x08
//	mtk_writeb(REG_OFFSET_7B_H(BANK_BASE_ALBANY_2), 0x08);
	//wriu     0x0422aa             0x03
//	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_2), 0x03);
	//wriu     0x0422f4             0x1a
//	mtk_writeb(REG_OFFSET_7A_L(BANK_BASE_ALBANY_2), 0x1a);
	//wriu     0x042104             0x11
//	mtk_writeb(REG_OFFSET_02_L(BANK_BASE_ALBANY_1), 0x11);
//}

static u32 mtk_dtv_gmac_phy_read_internal(struct net_device *ndev,
					  u8 phy_addr, u8 reg_addr)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	return mtk_readl(BANK_BASE_ALBANY_0 + (reg_addr << 2));
}

static u32 mtk_dtv_gmac_phy_read_external(struct net_device *ndev,
					  u8 phy_addr, u8 reg_addr)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 reg_man, reg_stat, reg_read;

	reg_man = ((0x60020000) | ((phy_addr & 0x1f) << 23) | (reg_addr << 18));

	/* enable mdio */
	mtk_write_fld(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0,
		      0x1, REG_ETH_CTL_MPE);

	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0068_X32_GMAC0, reg_man);

	/* wait until IDLE bit in Network Status register is cleared */
	do {
		reg_stat = mtk_read_fld(BANK_BASE_MAC_0 + GMAC0_REG_0010_X32_GMAC0,
					REG_ETH_SR_IDLE);
	} while (!reg_stat);

	reg_read = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0068_X32_GMAC0);
	reg_read &= 0xffff;

	/* disable mdio */
	mtk_write_fld(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0,
		      0x0, REG_ETH_CTL_MPE);

	return reg_read;
}

static void mtk_dtv_gmac_phy_write_internal(struct net_device *ndev,
					    u8 phy_addr, u8 reg_addr, u32 val)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	mtk_writel((BANK_BASE_ALBANY_0 + (reg_addr << 2)), val);
}

static void mtk_dtv_gmac_phy_write_external(struct net_device *ndev,
					    u8 phy_addr, u8 reg_addr, u32 val)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 reg_man, reg_stat;

	reg_man = ((0x50020000) | ((phy_addr & 0x1f) << 23) |
		   (reg_addr << 18) | (val & 0xffff));

	/* enable mdio */
	mtk_write_fld(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0,
		      0x1, REG_ETH_CTL_MPE);

	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0068_X32_GMAC0, reg_man);

	/* wait until IDLE bit in Network Status register is cleared */
	do {
		reg_stat = mtk_read_fld(BANK_BASE_MAC_0 + GMAC0_REG_0010_X32_GMAC0,
					REG_ETH_SR_IDLE);
	} while (!reg_stat);

	/* disable mdio */
	mtk_write_fld(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0,
		      0x0, REG_ETH_CTL_MPE);
}

static int mtk_dtv_gmac_phy_addr_scan(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u8 addr = 0;
	u32 val;

	do {
		val = reg_ops->read_phy(ndev, addr, MII_BMSR);
		if (val != 0 && val != 0xffff) {
			dev_info(priv->dev, "phy addr(%d)\n", addr);
			break;
		}
		addr++;
	} while (addr < 32);

	if (addr >= 32) {
		dev_err(priv->dev, "bad phy addr(%d), set phy addr to 0\n",
			addr);
		priv->phy_addr = 0;
		return -1;
	}

	priv->phy_addr = addr;

	return 0;
}

/* checksum function */
static void _mtk_dtv_gmac_rx_checksum(struct sk_buff *skb, u32 desc_val)
{
	bool ret;

	if (!(desc_val & MAC_DESC_PKT_TYPE_BIT2) &&
	    !(desc_val & MAC_DESC_PKT_TYPE_BIT1) &&
	    !(desc_val & MAC_DESC_PKT_TYPE_BIT0)) {
		/* not ip packet */
		ret = false;
	} else {
		/* ip packet */
		if (!(desc_val & MAC_DESC_IP_CSUM)) {
			ret = false;
		} else {
			/* ip checksum ok */
			if ((!(desc_val & MAC_DESC_PKT_TYPE_BIT2) &&
			     (desc_val & MAC_DESC_PKT_TYPE_BIT1) &&
			     (desc_val & MAC_DESC_PKT_TYPE_BIT0)) ||
			    ((desc_val & MAC_DESC_PKT_TYPE_BIT2) &&
			     (desc_val & MAC_DESC_PKT_TYPE_BIT1) &&
			     !(desc_val & MAC_DESC_PKT_TYPE_BIT0))) {
				/* not tcp or udp */
				ret = true;
			} else {
				/* tcp or udp */
				if (!(desc_val & MAC_DESC_TCP_UDP_CSUM))
					ret = false;
				else
					ret = true;
			}
		}
	}

	if (ret)
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	else
		skb->ip_summed = CHECKSUM_NONE;
}

/* packet data dump for debug usage */
void mtk_dtv_gmac_skb_dump(phys_addr_t addr, u32 len)
{
	u8 *ptr = (u8 *)addr;
	u32 i;

	pr_err("===== Dump %lx, len %d(%02x) =====\n",
	       (unsigned long)ptr, len, len);
	pr_err("              00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f\n");
	for (i = 0; i < len; i++) {
		pr_err("%lx(%02x): %02x %02x %02x %02x %02x %02x %02x %02x  ",
		       (unsigned long)ptr, i,
		       *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3),
		       *(ptr + 4), *(ptr + 5), *(ptr + 6), *(ptr + 7));
		pr_err("%02x %02x %02x %02x %02x %02x %02x %02x\n",
		       *(ptr + 8), *(ptr + 9), *(ptr + 10), *(ptr + 11),
		       *(ptr + 12), *(ptr + 13), *(ptr + 14), *(ptr + 15));
		ptr += 16;
		i += 15;
	}
	pr_err("\n");
}

static void mtk_dtv_gmac_hw_mac_addr_set(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 val, reg_read;

	val = ((ndev->dev_addr[3] << 24) | (ndev->dev_addr[2] << 16) |
	       (ndev->dev_addr[1] << 8) | (ndev->dev_addr[0]));
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0130_X32_GMAC0, val);
	reg_read = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0138_X32_GMAC0);
	val = ((((ndev->dev_addr[5] << 8) | (ndev->dev_addr[4])) & 0xffff) |
	       (reg_read & 0xffff0000));
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0138_X32_GMAC0, val);

	/* set wol mac */
	mtk_writeb(REG_OFFSET_20_L(BANK_BASE_ALBANY_2), ndev->dev_addr[5]);
	mtk_writeb(REG_OFFSET_20_H(BANK_BASE_ALBANY_2), ndev->dev_addr[4]);
	mtk_writeb(REG_OFFSET_21_L(BANK_BASE_ALBANY_2), ndev->dev_addr[3]);
	mtk_writeb(REG_OFFSET_21_H(BANK_BASE_ALBANY_2), ndev->dev_addr[2]);
	mtk_writeb(REG_OFFSET_22_L(BANK_BASE_ALBANY_2), ndev->dev_addr[1]);
	mtk_writeb(REG_OFFSET_22_H(BANK_BASE_ALBANY_2), ndev->dev_addr[0]);

	dev_dbg(priv->dev, "set ndev->dev_addr=%pM\n", ndev->dev_addr);
}

static void mtk_dtv_gmac_mac_addr_get(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 mac_addr_h, mac_addr_l;
	u8 addr[6] = {};

	mac_addr_l = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0130_X32_GMAC0);
	mac_addr_h = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0138_X32_GMAC0);

	addr[0] = (mac_addr_l & 0xff);
	addr[1] = (mac_addr_l & 0xff00) >> 8;
	addr[2] = (mac_addr_l & 0xff0000) >> 16;
	addr[3] = (mac_addr_l & 0xff000000) >> 24;
	addr[4] = (mac_addr_h & 0xff);
	addr[5] = (mac_addr_h & 0xff00) >> 8;

	if (is_valid_ether_addr(addr)) {
		ether_addr_copy(ndev->dev_addr, addr);
		dev_dbg(priv->dev, "ndev->dev_addr=%pM, sa1=%pM\n",
			ndev->dev_addr, addr);
		return;
	}

	mac_addr_l = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0140_X32_GMAC0);
	mac_addr_h = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0148_X32_GMAC0);

	addr[0] = (mac_addr_l & 0xff);
	addr[1] = (mac_addr_l & 0xff00) >> 8;
	addr[2] = (mac_addr_l & 0xff0000) >> 16;
	addr[3] = (mac_addr_l & 0xff000000) >> 24;
	addr[4] = (mac_addr_h & 0xff);
	addr[5] = (mac_addr_h & 0xff00) >> 8;

	if (is_valid_ether_addr(addr)) {
		ether_addr_copy(ndev->dev_addr, addr);
		dev_dbg(priv->dev, "ndev->dev_addr=%pM, sa2=%pM\n",
			ndev->dev_addr, addr);
		return;
	}

	mac_addr_l = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0150_X32_GMAC0);
	mac_addr_h = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0158_X32_GMAC0);

	addr[0] = (mac_addr_l & 0xff);
	addr[1] = (mac_addr_l & 0xff00) >> 8;
	addr[2] = (mac_addr_l & 0xff0000) >> 16;
	addr[3] = (mac_addr_l & 0xff000000) >> 24;
	addr[4] = (mac_addr_h & 0xff);
	addr[5] = (mac_addr_h & 0xff00) >> 8;

	if (is_valid_ether_addr(addr)) {
		ether_addr_copy(ndev->dev_addr, addr);
		dev_dbg(priv->dev, "ndev->dev_addr=%pM, sa3=%pM\n",
			ndev->dev_addr, addr);
		return;
	}

	mac_addr_l = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0160_X32_GMAC0);
	mac_addr_h = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0168_X32_GMAC0);

	addr[0] = (mac_addr_l & 0xff);
	addr[1] = (mac_addr_l & 0xff00) >> 8;
	addr[2] = (mac_addr_l & 0xff0000) >> 16;
	addr[3] = (mac_addr_l & 0xff000000) >> 24;
	addr[4] = (mac_addr_h & 0xff);
	addr[5] = (mac_addr_h & 0xff00) >> 8;

	if (is_valid_ether_addr(addr)) {
		ether_addr_copy(ndev->dev_addr, addr);
		dev_dbg(priv->dev, "ndev->dev_addr=%pM, sa4=%pM\n",
			ndev->dev_addr, addr);
		return;
	}

	if (is_valid_ether_addr(mac_addr_env)) {
		ether_addr_copy(ndev->dev_addr, mac_addr_env);
		dev_dbg(priv->dev, "ndev->dev_addr=%pM, mac_addr_env=%pM\n",
			ndev->dev_addr, mac_addr_env);
		return;
	}

	if (is_valid_ether_addr(mac_addr_dts)) {
		ether_addr_copy(ndev->dev_addr, mac_addr_dts);
		dev_dbg(priv->dev, "ndev->dev_addr=%pM, mac_addr_dts=%pM\n",
			ndev->dev_addr, mac_addr_dts);
		return;
	}

	eth_hw_addr_random(ndev);
	dev_info(priv->dev, "use random mac address(%pM)\n", ndev->dev_addr);
}

static void _mtk_dtv_gmac_hw_patch(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	if (priv->mac_type == MAC_TYPE_EMAC) {
		mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_006C_X32_OGMAC1, 0x3244);
		mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_0070_X32_OGMAC1, 0x0105);
	}
}

/* phy link status */
static bool mtk_dtv_gmac_link_ability_update(struct net_device *ndev,
					     u32 bmsr, u32 bmcr, u32 *speed, u32 *duplex)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 lpa, adv, neg, reg;

	if (bmcr & BMCR_ANENABLE) {
		/* auto-negotiation */
		if (!(bmsr & BMSR_ANEGCOMPLETE)) {
			dev_info(priv->dev, "auto-negotiation still running\n");
			if (netif_carrier_ok(ndev))
				netif_carrier_off(ndev);

			return false;
		}

		if (!priv->is_internal_phy) {
			/* external gphy */
			reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_STAT1000);
			if ((reg & LPA_1000FULL) || (reg & LPA_1000HALF)) {
				*speed = SPEED_1000;
				*duplex = DUPLEX_FULL;
				dev_dbg(priv->dev, "link_stat: 1000 full\n");
				return true;
			}
		}

		/* get link partner and advertisement from the phy not from the mac */
		adv = reg_ops->read_phy(ndev, priv->phy_addr, MII_ADVERTISE);
		lpa = reg_ops->read_phy(ndev, priv->phy_addr, MII_LPA);

		/* for link parterner adopts force mode and ephy used,
		 * ephy lpa reveals all zero value.
		 * ephy would be forced to full-duplex mode.
		 */
		if (!lpa) {
			/* 100Mbps full-duplex */
			if (bmcr & BMCR_SPEED100)
				lpa |= LPA_100FULL;
			else /* 10Mbps full-duplex */
				lpa |= LPA_10FULL;
		}

		neg = (adv & lpa);

		dev_dbg(priv->dev, "link_stat: bmcr=0x%x, adv=0x%x, lpa=0x%x, neg=0x%x\n",
			bmcr, adv, lpa, neg);

		if (neg & LPA_100FULL) {
			*speed = SPEED_100;
			*duplex = DUPLEX_FULL;
			dev_dbg(priv->dev, "link_stat: 100 full\n");
		} else if (neg & LPA_100HALF) {
			*speed = SPEED_100;
			*duplex = DUPLEX_HALF;
			dev_dbg(priv->dev, "link_stat: 100 half\n");
		} else if (neg & LPA_10FULL) {
			*speed = SPEED_10;
			*duplex = DUPLEX_FULL;
			dev_dbg(priv->dev, "link_stat: 10 full\n");
		} else if (neg & LPA_10HALF) {
			*speed = SPEED_10;
			*duplex = DUPLEX_HALF;
			dev_dbg(priv->dev, "link_stat: 10 half\n");
		} else {
			*speed = SPEED_10;
			*duplex = DUPLEX_HALF;
			dev_info(priv->dev, "no sp and dup, set 10 half (lpa=0x%x, adv=0x%x)\n",
				 lpa, adv);
		}
	} else {
		*speed = (bmcr & BMCR_SPEED100) ? SPEED_100 : SPEED_10;
		*duplex = (bmcr & BMCR_FULLDPLX) ? DUPLEX_FULL : DUPLEX_HALF;
		dev_dbg(priv->dev, "link_stat: speed:0x%x, duplex=0x%x\n",
			*speed, *duplex);
	}

	return true;
}

static void _mtk_dtv_gmac_set_hw_speed(struct net_device *ndev, u32 speed, u32 duplex)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 reg;

	/* get status from mac */
	reg = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0);
	if (speed == SPEED_100)
		reg |= 0x1UL;
	else
		reg &= ~(0x1UL);

	if (duplex == DUPLEX_FULL)
		reg |= 0x2UL;
	else
		reg &= ~(0x2UL);

	/* update status to mac */
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0, reg);
	dev_dbg(priv->dev, "link_stat: update to mac reg:0x%x\n", reg);

	if (priv->is_internal_phy) {
		if (priv->mac_type == MAC_TYPE_GMAC) {
			/* set xmii_type to TGMII */
			mtk_writeb(REG_OFFSET_30_L(BANK_BASE_MAC_4), 0x02);
			mtk_writeb(REG_OFFSET_30_H(BANK_BASE_MAC_4), 0x01);
		}
	} else {
		if (speed == SPEED_1000) {
			/* set xmii_type to RGMII 1000M */
			if (priv->mac_type == MAC_TYPE_GMAC) {
				mtk_writeb(REG_OFFSET_30_L(BANK_BASE_MAC_4), 0x00);
				mtk_writeb(REG_OFFSET_30_H(BANK_BASE_MAC_4), 0x01);
			}
			mtk_writeb(REG_OFFSET_32_L(BANK_BASE_ALBANY_3), 0x0c);
			mtk_writeb(REG_OFFSET_32_H(BANK_BASE_ALBANY_3), 0x20);
			mtk_writeb(REG_OFFSET_33_L(BANK_BASE_ALBANY_3), 0x3e);
			mtk_writeb(REG_OFFSET_33_H(BANK_BASE_ALBANY_3), 0x00);
		} else {
			/* set xmii_type to RGMII 100/10M */
			if (priv->mac_type == MAC_TYPE_GMAC) {
				mtk_writeb(REG_OFFSET_30_L(BANK_BASE_MAC_4), 0x01);
				mtk_writeb(REG_OFFSET_30_H(BANK_BASE_MAC_4), 0x01);
			}
			if (!priv->is_fpga_haps) {
				mtk_writeb(REG_OFFSET_32_L(BANK_BASE_ALBANY_3), 0x0e);
				mtk_writeb(REG_OFFSET_32_H(BANK_BASE_ALBANY_3), 0x20);

				if (speed == SPEED_100)
					mtk_writeb(REG_OFFSET_33_L(BANK_BASE_ALBANY_3), 0x3e);
				else
					mtk_writeb(REG_OFFSET_33_L(BANK_BASE_ALBANY_3), 0x1e);
				mtk_writeb(REG_OFFSET_33_H(BANK_BASE_ALBANY_3), 0x00);
			}
		}
	}
}

static bool mtk_dtv_gmac_link_status_update(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 bmsr, bmcr, speed, duplex;
	u32 hcd_link_st_ok, an_100t_link_st;

	/* latch link status bit to 1 */
	bmsr = reg_ops->read_phy(ndev, priv->phy_addr, MII_BMSR);
	bmsr |= 0x4UL;
	reg_ops->write_phy(ndev, priv->phy_addr, MII_BMSR, bmsr);
	bmsr = reg_ops->read_phy(ndev, priv->phy_addr, MII_BMSR);
	/* check hcd link status */
	hcd_link_st_ok = reg_ops->read_phy(ndev, priv->phy_addr, 0x21);

	dev_dbg(priv->dev, "link_stat: bmsr=0x%x, hcd_link_st_ok=0x%x\n",
		bmsr, hcd_link_st_ok);

	if (!(hcd_link_st_ok & 0x100UL)) {
		/* link down */
		if (netif_carrier_ok(ndev))
			netif_carrier_off(ndev);

		dev_dbg(priv->dev, "link down: bmsr=0x%x, hcd_link_st_ok=0x%x\n",
			bmsr, hcd_link_st_ok);

		return false;
	}

	bmcr = reg_ops->read_phy(ndev, priv->phy_addr, MII_BMCR);

	if (!mtk_dtv_gmac_link_ability_update(ndev, bmsr, bmcr, &speed, &duplex))
		return false;

	_mtk_dtv_gmac_set_hw_speed(ndev, speed, duplex);

	/* link up */
	if (!netif_carrier_ok(ndev))
		netif_carrier_on(ndev);

	//TODO: GMAC needs it?
	/* phy restart patch */
	if (priv->is_internal_phy) {
		if (speed == SPEED_100) {
			hcd_link_st_ok = reg_ops->read_phy(ndev, priv->phy_addr, 0x21);
			an_100t_link_st = reg_ops->read_phy(ndev, priv->phy_addr, 0x22);
			if (((hcd_link_st_ok & 0x100) && !(an_100t_link_st & 0x300)) ||
			    (!(hcd_link_st_ok & 0x100) && ((an_100t_link_st & 0x300) == 0x200))) {
				priv->phy_restart_cnt++;
				if (priv->phy_restart_cnt > 10) {
					dev_info(priv->dev, "restart auto-negotiation\n");
					reg_ops->write_phy(ndev, priv->phy_addr,
							   MII_BMCR, 0x1200UL);
					priv->phy_restart_cnt = 0;
				}
			} else {
				priv->phy_restart_cnt = 0;
			}
		}
	}

	return true;
}

static void mtk_dtv_gmac_link_timer_callback(struct timer_list *t)
{
	struct mtk_dtv_gmac_private *priv = from_timer(priv, t, link_timer);
	struct net_device *ndev = priv->ndev;
	bool ret;
	static u32 cnt;

	ret = mtk_dtv_gmac_link_status_update(ndev);
	if (ret) {
		/* link up */
		priv->link_timer.expires = jiffies + MAC_UPDATE_LINK_TIME;
		cnt = 0;
	} else {
		/* link down */
		if (cnt < 10) {
			/* check phy status quickly */
			priv->link_timer.expires = jiffies + (MAC_UPDATE_LINK_TIME / 10);
			cnt++;
		} else {
		/* check phy status normally */
			priv->link_timer.expires = jiffies + MAC_UPDATE_LINK_TIME;
		}
	}

	if (!(timer_pending(&priv->link_timer)))
		add_timer(&priv->link_timer);
}

static void mtk_dtv_gmac_mem_init(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct sk_buff *skb;
	int i;
	u32 size;
	dma_addr_t dma_addr, rx_desc_dma_addr;

	/* tx */
	priv->tx_memcpy_index = 0;
	memset((void *)priv->tx_memcpy_vir_addr, 0x0,
	       (TX_MEMCPY_SIZE_DEFAULT * TX_MEMCPY_NUM_DEFAULT));

	/* rx */
	/* set descriptor pointer */
	/* TODO: should we take care of MIU0_BUS_BASE? */
	rx_desc_dma_addr = priv->rx_desc_dma_addr - MIU0_BUS_BASE;
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0030_X32_GMAC0, rx_desc_dma_addr);

	memset((void *)priv->rx_desc_vir_addr, 0x0,
	       (priv->rx_desc_num * priv->rx_desc_size));
	skb_queue_purge(&priv->rx_skb_q);
	priv->rx_desc_index = 0;

	if (priv->mac_type == MAC_TYPE_EMAC) {
		struct rx_desc_emac *prx = (struct rx_desc_emac *)priv->rx_desc_vir_addr;

		for (i = 0; i < priv->rx_desc_num; i++) {
			/* create skb */
			size = MAC_RX_MAX_LEN + MAC_EXTRA_PKT_LEN;
			skb = netdev_alloc_skb(ndev, size);
			if (!skb) {
				dev_err(priv->dev, "allocate skb fail at %d\n", i);
				break;
			}
			//skb_reserve(skb, NET_IP_ALIGN);
			//skb_queue_tail(&priv->rx_skb_q, skb);
			dma_addr = dma_map_single(ndev->dev.parent, skb->data, size,
						  DMA_FROM_DEVICE);
			if (dma_mapping_error(ndev->dev.parent, dma_addr)) {
				dev_err(priv->dev, "dma map fail at %d\n", i);
				break;
			}

			skb_queue_tail(&priv->rx_skb_q, skb);

			/* TODO: should we take care of MIU0_BUS_BASE? */
			dma_addr -= MIU0_BUS_BASE;

			if (i == (priv->rx_desc_num - 1))
				dma_addr |= 0x2UL;

			prx[i].addr = dma_addr;
			/* sync rx descriptor */
			wmb();
		}
	} else {
		struct rx_desc_gmac *prx = (struct rx_desc_gmac *)priv->rx_desc_vir_addr;

		for (i = 0; i < priv->rx_desc_num; i++) {
			/* create skb */
			size = MAC_RX_MAX_LEN + MAC_EXTRA_PKT_LEN;
			skb = netdev_alloc_skb(ndev, size);
			if (!skb) {
				dev_err(priv->dev, "allocate skb fail at %d\n", i);
				break;
			}
			//skb_reserve(skb, NET_IP_ALIGN);
			//skb_queue_tail(&priv->rx_skb_q, skb);
			dma_addr = dma_map_single(ndev->dev.parent, skb->data, size,
						  DMA_FROM_DEVICE);
			if (dma_mapping_error(ndev->dev.parent, dma_addr)) {
				dev_err(priv->dev, "dma map fail at %d\n", i);
				break;
			}

			skb_queue_tail(&priv->rx_skb_q, skb);

			/* TODO: should we take care of MIU0_BUS_BASE? */
			dma_addr -= MIU0_BUS_BASE;

			if (i == (priv->rx_desc_num - 1))
				dma_addr |= 0x2UL;

			prx[i].addr = dma_addr;
			/* sync rx descriptor */
			wmb();
		}
	}
}

static void mtk_dtv_gmac_mem_release(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	dma_addr_t dma_addr;
	u32 size;
	int i;

	if (priv->mac_type == MAC_TYPE_EMAC) {
		struct rx_desc_emac *prx = (struct rx_desc_emac *)priv->rx_desc_vir_addr;

		for (i = 0; i < priv->rx_desc_num; i++) {
			size = MAC_RX_MAX_LEN + MAC_EXTRA_PKT_LEN;
			dma_addr = prx[i].addr;
			dma_addr += MIU0_BUS_BASE;
			/* clear both DONE and LAST bits */
			dma_addr &= ~(0x3UL);
			/* sync rx descriptor */
			mb();
			dma_unmap_single(ndev->dev.parent, dma_addr, size, DMA_FROM_DEVICE);
		}
	} else {
		struct rx_desc_gmac *prx = (struct rx_desc_gmac *)priv->rx_desc_vir_addr;

		for (i = 0; i < priv->rx_desc_num; i++) {
			size = MAC_RX_MAX_LEN + MAC_EXTRA_PKT_LEN;
			dma_addr = prx[i].addr;
			dma_addr += MIU0_BUS_BASE;
			/* clear both DONE and LAST bits */
			dma_addr &= ~(0x3UL);
			/* sync rx descriptor */
			mb();
			dma_unmap_single(ndev->dev.parent, dma_addr, size, DMA_FROM_DEVICE);
		}
	}

	/* tx */
	priv->tx_memcpy_index = 0;
	memset((void *)priv->tx_memcpy_vir_addr, 0x0,
	       (TX_MEMCPY_SIZE_DEFAULT * TX_MEMCPY_NUM_DEFAULT));

	/* rx */
	memset((void *)priv->rx_desc_vir_addr, 0x0,
	       (priv->rx_desc_num * priv->rx_desc_size));
	skb_queue_purge(&priv->rx_skb_q);
	priv->rx_desc_index = 0;
}

void mtk_dtv_gmac_gphy_regs_tune(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	struct platform_device *pdev = priv->pdev;
	struct device_node *np = pdev->dev.of_node;
	char dts_str[GPHY_TUNE_DTS_NAME_SIZE];
	u32 reg_num, reg_val, get_val = 0;
	u8 reg_addr;
	int i, ret;

	dev_dbg(priv->dev, "start %s\n", __func__);

	if (of_property_read_u32(np, "gphy-reg-num", &get_val)) {
		dev_dbg(priv->dev, "cannot get gphy-reg-num\n");
		reg_num = 0;
		goto exit_gphy_regs_tune;
	} else {
		reg_num = get_val;
		dev_dbg(priv->dev, "gphy-reg-num (%d)\n",
			reg_num);
	}

	for (i = 0; i < reg_num; i++) {
		/* get reg addr */
		get_val = 0;
		memset(dts_str, 0, sizeof(dts_str));
		ret = snprintf(dts_str, GPHY_TUNE_DTS_NAME_SIZE, "gphy-reg-addr-%d", i);
		if (ret < 0) {
			dev_err(priv->dev, "snprintf gphy-reg-addr failed at %d: %d\n", i, ret);
			goto exit_gphy_regs_tune;
		}
		if (of_property_read_u32(np, dts_str, &get_val)) {
			dev_err(priv->dev, "cannot get %s\n", dts_str);
			goto exit_gphy_regs_tune;
		} else {
			reg_addr = (u8)get_val;
			dev_dbg(priv->dev, "%s = 0x%x\n",
				dts_str, reg_addr);
		}
		/* get reg val */
		get_val = 0;
		memset(dts_str, 0, sizeof(dts_str));
		ret = snprintf(dts_str, GPHY_TUNE_DTS_NAME_SIZE, "gphy-reg-val-%d", i);
		if (ret < 0) {
			dev_err(priv->dev, "snprintf gphy-reg-val failed at %d: %d\n", i, ret);
			goto exit_gphy_regs_tune;
		}
		if (of_property_read_u32(np, dts_str, &get_val)) {
			dev_err(priv->dev, "cannot get %s\n", dts_str);
			goto exit_gphy_regs_tune;
		} else {
			reg_val = get_val;
			dev_dbg(priv->dev, "%s = 0x%x\n",
				dts_str, reg_val);
		}
		/* write reg */
		reg_ops->write_phy(ndev, priv->phy_addr, reg_addr, reg_val);
	}

exit_gphy_regs_tune:
	dev_dbg(priv->dev, "end %s\n", __func__);
}

int mtk_dtv_gmac_hw_init(struct net_device *ndev, bool is_suspend)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	int ret;

	mtk_dtv_gmac_hw_power_on_clk(ndev, is_suspend);

	if (priv->is_internal_phy) {
		/* 0xf011 */
		mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_0000_X32_OGMAC1, 0xf011);
	} else {
		if (priv->mac_type == MAC_TYPE_EMAC) {
			/* 0xf015 */
			mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_0000_X32_OGMAC1, 0xf011);
		} else {
			/* 0xf017 */
			mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_0000_X32_OGMAC1, 0xf017);
		}
	}

	ret = mtk_dtv_gmac_phy_addr_scan(ndev);
	if (ret)
		return -1;

	/* tune gphy registers */
	if (!priv->is_internal_phy)
		mtk_dtv_gmac_gphy_regs_tune(ndev);

	if (priv->is_force_10m) {
		/* force speed to 10M full */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR, BMCR_RESET);
		mdelay(10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_ADVERTISE,
				   ADVERTISE_CSMA | ADVERTISE_10HALF | ADVERTISE_10FULL);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_CTRL1000, 0x0);
		mdelay(10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   BMCR_ANENABLE | BMCR_ANRESTART);
	} else if (priv->is_force_100m) {
		/* force speed to 100M full */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR, BMCR_RESET);
		mdelay(10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_ADVERTISE,
				   ADVERTISE_CSMA | ADVERTISE_100HALF | ADVERTISE_100FULL);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_CTRL1000, 0x0);
		mdelay(10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   BMCR_ANENABLE | BMCR_ANRESTART);
	}

	if (priv->mac_type == MAC_TYPE_EMAC) {
		/* switch rx discriptor format to mode 1 */
		mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_0074_X32_OGMAC1, 0x0100);
	}

	/* 0x8f */
	mtk_writeb(BANK_BASE_MAC_1 + OGMAC1_REG_0004_X32_OGMAC1, 0x08);
	/* 0x8f */
	mtk_writeb(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x8f);

	/* delay interrupt: 0x0101 */
	mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_000C_X32_OGMAC1, IRQ_RX_DELAY_SET);

	/* mac address */
	if (!is_suspend)
		mtk_dtv_gmac_mac_addr_get(ndev);
	mtk_dtv_gmac_hw_mac_addr_set(ndev);

	/* default speed-duplex 100-full */
	mtk_write_fld_multi(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0, 2,
			    0x1, REG_ETH_CFG_SPEED, 0x1, REG_ETH_CFG_FD);

	/* hw patch */
	_mtk_dtv_gmac_hw_patch(ndev);

	return 0;
}

static void _mtk_dtv_gmac_rx_err_stat_irq_mask_handle(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	if (priv->is_rx_rbna_irq_masked) {
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0040_X32_GMAC0, IRQ_RSR_DNA);
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0040_X32_GMAC0, IRQ_RSR_BNA);

		/* unmask irq till descriptor empty by rx process */
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0050_X32_GMAC0, IRQ_BIT_RBNA);
		priv->is_rx_rbna_irq_masked = false;
	}

	if (priv->is_rx_rovr_irq_masked) {
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0040_X32_GMAC0, IRQ_RSR_OVR_RSR);

		/* unmask irq till descriptor empty by rx process */
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0050_X32_GMAC0, IRQ_BIT_ROVR);
		priv->is_rx_rovr_irq_masked = false;
	}
}

static bool _mtk_dtv_gmac_rx_emac(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct rx_desc_emac *prx = (struct rx_desc_emac *)priv->rx_desc_vir_addr;
	struct sk_buff *skb, *skb_new;
	dma_addr_t dma_addr, dma_addr_new;
	u32 len, size;

	dma_addr = prx[priv->rx_desc_index].addr;
	if (dma_addr & RX_DESC_DONE) {
		len = (prx[priv->rx_desc_index].size & 0x7ffUL);
		if (len > MAC_RX_MAX_LEN || len < 64) {
			dev_err(priv->dev, "packet length error(%d)\n", len);
			/* put first skb to last skb in queue */
			skb = skb_dequeue(&priv->rx_skb_q);
			if (skb)
				skb_queue_tail(&priv->rx_skb_q, skb);
			else
				dev_err(priv->dev, "packet length error(%d), dequeue error\n",
					len);

			prx[priv->rx_desc_index].addr &= ~(0x1UL);
			mb();
			ndev->stats.rx_length_errors++;
			goto rx_end_emac;
		}
		/* remove 4 bytes CRC */
		len -= 4;
		skb = skb_dequeue(&priv->rx_skb_q);
		if (skb) {
			/* create skb */
			size = MAC_RX_MAX_LEN + MAC_EXTRA_PKT_LEN;
			skb_new = netdev_alloc_skb(ndev, size);
			if (!skb_new) {
				dev_err(priv->dev, "allocate skb fail, use old skb\n");
				/* put first skb to last skb in queue */
				skb_queue_tail(&priv->rx_skb_q, skb);
				prx[priv->rx_desc_index].addr &= ~(0x1UL);
				mb();
				ndev->stats.rx_missed_errors++;
				goto rx_end_emac;
			}
			//skb_reserve(skb, NET_IP_ALIGN);
			skb_queue_tail(&priv->rx_skb_q, skb_new);
			dma_addr_new = dma_map_single(ndev->dev.parent, skb_new->data, size,
						      DMA_FROM_DEVICE);
			if (dma_mapping_error(ndev->dev.parent, dma_addr_new)) {
				dev_err(priv->dev, "dma map fail, use old skb\n");
				/* drop skb_new */
				skb_dequeue_tail(&priv->rx_skb_q);
				/* put first skb to last skb in queue */
				skb_queue_tail(&priv->rx_skb_q, skb);
				dev_kfree_skb_any(skb_new);
				prx[priv->rx_desc_index].addr &= ~(0x1UL);
				mb();
				ndev->stats.rx_missed_errors++;
				goto rx_end_emac;
			}

			/* TODO: should we take care of MIU0_BUS_BASE? */
			dma_addr_new -= MIU0_BUS_BASE;

			/* last descriptor */
			if (priv->rx_desc_index >= (priv->rx_desc_num - 1))
				dma_addr_new |= 0x2UL;

			prx[priv->rx_desc_index].addr = dma_addr_new;
			/* sync rx descriptor */
			wmb();

			/* net rx process */
			dma_addr += MIU0_BUS_BASE;
			/* clear both DONE and LAST bits */
			dma_addr &= ~(0x3UL);
			/* sync rx descriptor */
			mb();
			dma_unmap_single(ndev->dev.parent, dma_addr, size, DMA_FROM_DEVICE);
			dev_dbg(priv->dev,
				"dat_new(0x%lx), dma_new(0x%lx), dat(0x%lx), dma(0x%lx)\n",
				(unsigned long)skb_new->data, (unsigned long)dma_addr_new,
				(unsigned long)skb->data, (unsigned long)dma_addr);

			/* slt-test */
			if (priv->is_slt_test_running) {
				mtk_dtv_gmac_slt_rx(ndev, skb, len);

				goto rx_end_emac;
			}

			skb_put(skb, len);
			//dev_info(priv->dev, "start recv:\n");
			//mtk_dtv_gmac_skb_dump(skb->data, len);

			_mtk_dtv_gmac_rx_checksum(skb, prx[priv->rx_desc_index].size);

			skb->protocol = eth_type_trans(skb, ndev);
			netif_receive_skb(skb);
			ndev->stats.rx_packets++;
			ndev->stats.rx_bytes += len;
		} else {
			dev_err(priv->dev, "skb from queue is bad\n");
			ndev->stats.rx_missed_errors++;
		}

rx_end_emac:
		priv->rx_desc_index++;
		if (priv->rx_desc_index >= priv->rx_desc_num)
			priv->rx_desc_index = 0;

		return true;
	} else {
		return false;
	}
}

static bool _mtk_dtv_gmac_rx_gmac(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct rx_desc_gmac *prx = (struct rx_desc_gmac *)priv->rx_desc_vir_addr;
	struct sk_buff *skb, *skb_new;
	dma_addr_t dma_addr, dma_addr_new;
	u32 len, size;

	dma_addr = prx[priv->rx_desc_index].addr;
	if (dma_addr & RX_DESC_DONE) {
		len = (prx[priv->rx_desc_index].size & 0x7ffUL);
		if (len > MAC_RX_MAX_LEN || len < 64) {
			dev_err(priv->dev, "packet length error(%d)\n", len);
			/* put first skb to last skb in queue */
			skb = skb_dequeue(&priv->rx_skb_q);
			if (skb)
				skb_queue_tail(&priv->rx_skb_q, skb);
			else
				dev_err(priv->dev, "packet length error(%d), dequeue error\n",
					len);

			prx[priv->rx_desc_index].addr &= ~(0x1UL);
			mb();
			ndev->stats.rx_length_errors++;
			goto rx_end_gmac;
		}
		/* remove 4 bytes CRC */
		len -= 4;
		skb = skb_dequeue(&priv->rx_skb_q);
		if (skb) {
			/* create skb */
			size = MAC_RX_MAX_LEN + MAC_EXTRA_PKT_LEN;
			skb_new = netdev_alloc_skb(ndev, size);
			if (!skb_new) {
				dev_err(priv->dev, "allocate skb fail, use old skb\n");
				/* put first skb to last skb in queue */
				skb_queue_tail(&priv->rx_skb_q, skb);
				prx[priv->rx_desc_index].addr &= ~(0x1UL);
				mb();
				ndev->stats.rx_missed_errors++;
				goto rx_end_gmac;
			}
			//skb_reserve(skb, NET_IP_ALIGN);
			skb_queue_tail(&priv->rx_skb_q, skb_new);
			dma_addr_new = dma_map_single(ndev->dev.parent, skb_new->data, size,
						      DMA_FROM_DEVICE);
			if (dma_mapping_error(ndev->dev.parent, dma_addr_new)) {
				dev_err(priv->dev, "dma map fail, use old skb\n");
				/* drop skb_new */
				skb_dequeue_tail(&priv->rx_skb_q);
				/* put first skb to last skb in queue */
				skb_queue_tail(&priv->rx_skb_q, skb);
				dev_kfree_skb_any(skb_new);
				prx[priv->rx_desc_index].addr &= ~(0x1UL);
				mb();
				ndev->stats.rx_missed_errors++;
				goto rx_end_gmac;
			}

			/* TODO: should we take care of MIU0_BUS_BASE? */
			dma_addr_new -= MIU0_BUS_BASE;

			/* last descriptor */
			if (priv->rx_desc_index >= (priv->rx_desc_num - 1))
				dma_addr_new |= 0x2UL;

			prx[priv->rx_desc_index].addr = dma_addr_new;
			/* sync rx descriptor */
			wmb();

			/* net rx process */
			dma_addr += MIU0_BUS_BASE;
			/* clear both DONE and LAST bits */
			dma_addr &= ~(0x3UL);
			/* sync rx descriptor */
			mb();
			dma_unmap_single(ndev->dev.parent, dma_addr, size, DMA_FROM_DEVICE);
			dev_dbg(priv->dev,
				"dat_new(0x%lx), dma_new(0x%lx), dat(0x%lx), dma(0x%lx)\n",
				(unsigned long)skb_new->data, (unsigned long)dma_addr_new,
				(unsigned long)skb->data, (unsigned long)dma_addr);

			/* slt-test */
			if (priv->is_slt_test_running) {
				mtk_dtv_gmac_slt_rx(ndev, skb, len);

				goto rx_end_gmac;
			}

			skb_put(skb, len);
			//dev_info(priv->dev, "start recv:\n");
			//mtk_dtv_gmac_skb_dump(skb->data, len);

			_mtk_dtv_gmac_rx_checksum(skb, prx[priv->rx_desc_index].size);

			skb->protocol = eth_type_trans(skb, ndev);
			netif_receive_skb(skb);
			ndev->stats.rx_packets++;
			ndev->stats.rx_bytes += len;
		} else {
			dev_err(priv->dev, "skb from queue is bad\n");
			ndev->stats.rx_missed_errors++;
		}

rx_end_gmac:
		priv->rx_desc_index++;
		if (priv->rx_desc_index >= priv->rx_desc_num)
			priv->rx_desc_index = 0;

		return true;
	} else {
		return false;
	}
}

static int mtk_dtv_gmac_rx(struct net_device *ndev, int loop)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int i, rx_pkt_cnt = 0;

	for (i = 0; i < loop; i++) {
		if (priv->mac_type == MAC_TYPE_EMAC) {
			if (_mtk_dtv_gmac_rx_emac(ndev))
				rx_pkt_cnt++;
			else
				break;
		} else {
			if (_mtk_dtv_gmac_rx_gmac(ndev))
				rx_pkt_cnt++;
			else
				break;
		}
	}

	_mtk_dtv_gmac_rx_err_stat_irq_mask_handle(ndev);

	return rx_pkt_cnt;
}

static int mtk_dtv_gmac_napi_poll_rx(struct napi_struct *napi, int budget)
{
	struct mtk_dtv_gmac_private *priv = container_of(napi,
							 struct mtk_dtv_gmac_private, napi_rx);
	struct net_device *ndev = priv->ndev;
	int work_done;

	work_done = mtk_dtv_gmac_rx(ndev, budget);

	if (work_done < budget) {
		local_irq_disable();
		napi_complete(napi);
		/* enable rx irq */
		mtk_write_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x1, REG_RX_INT_DLY_EN);
		local_irq_enable();
	}

	return work_done;
}

static irqreturn_t mtk_dtv_gmac_isr(int irq, void *dev_id)
{
	struct net_device *ndev = (struct net_device *)dev_id;
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 intr_stat, rx_stat, tx_stat, imr_stat;
	//dma_addr_t rx_desc_dma_addr;
	u32 delay_intr_stat;

	/* reg is read clear, read delay intr first */
	delay_intr_stat = mtk_read_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0010_X32_OGMAC1,
				       REG_RX_DLY_INT);
	intr_stat = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0048_X32_GMAC0);
	rx_stat = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0040_X32_GMAC0);
	tx_stat = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0028_X32_GMAC0);
	imr_stat = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0060_X32_GMAC0);
	intr_stat &= ~(imr_stat);

	if (intr_stat & IRQ_BIT_RBNA) {
		dev_dbg(priv->dev, "RBNA, isr(0x%x), rsr(0x%x), tsr(0x%x), delay(0x%x)\n",
			intr_stat, rx_stat, tx_stat, delay_intr_stat);
		//rx_desc_dma_addr = priv->rx_desc_dma_addr - MIU0_BUS_BASE;
		//mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0030_X32_GMAC0, rx_desc_dma_addr);
		ndev->stats.rx_fifo_errors++;
		if (rx_stat & IRQ_RSR_DNA)
			mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0040_X32_GMAC0, IRQ_RSR_DNA);
		if (rx_stat & IRQ_RSR_BNA)
			mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0040_X32_GMAC0, IRQ_RSR_BNA);

		/* mask irq till descriptor empty by rx process */
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0058_X32_GMAC0, IRQ_BIT_RBNA);
		priv->is_rx_rbna_irq_masked = true;
	}

	if (intr_stat & IRQ_BIT_ROVR) {
		dev_dbg(priv->dev, "ROVR, isr(0x%x), rsr(0x%x), tsr(0x%x), delay(0x%x)\n",
			intr_stat, rx_stat, tx_stat, delay_intr_stat);
		ndev->stats.rx_over_errors++;
		if (rx_stat & IRQ_RSR_OVR_RSR)
			mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0040_X32_GMAC0, IRQ_RSR_OVR_RSR);

		/* mask irq till descriptor empty by rx process */
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0058_X32_GMAC0, IRQ_BIT_ROVR);
		priv->is_rx_rovr_irq_masked = true;
	}

	if (intr_stat & IRQ_BIT_TUND) {
		dev_dbg(priv->dev, "TUND, isr(0x%x), rsr(0x%x), tsr(0x%x), delay(0x%x)\n",
			intr_stat, rx_stat, tx_stat, delay_intr_stat);
		ndev->stats.tx_fifo_errors++;
		if (tx_stat & IRQ_TSR_UND)
			mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0028_X32_GMAC0, IRQ_TSR_UND);
	}

	if (intr_stat & IRQ_BIT_TOVR) {
		dev_dbg(priv->dev, "TOVR, isr(0x%x), rsr(0x%x), tsr(0x%x), delay(0x%x)\n",
			intr_stat, rx_stat, tx_stat, delay_intr_stat);
		ndev->stats.tx_fifo_errors++;
		if (tx_stat & IRQ_TSR_OVER)
			mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0028_X32_GMAC0, IRQ_TSR_OVER);
	}

	if (intr_stat & IRQ_BIT_RTRY) {
		dev_dbg(priv->dev, "RTRY, isr(0x%x), rsr(0x%x), tsr(0x%x), delay(0x%x)\n",
			intr_stat, rx_stat, tx_stat, delay_intr_stat);
		ndev->stats.tx_errors++;
		if (tx_stat & IRQ_TSR_RLE)
			mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0028_X32_GMAC0, IRQ_TSR_RLE);
	}

	if (delay_intr_stat) {
		/* do rx */
		/* napi */
		if (likely(napi_schedule_prep(&priv->napi_rx))) {
			/* disable rx irq */
			mtk_write_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1,
				      0x0, REG_RX_INT_DLY_EN);
			__napi_schedule(&priv->napi_rx);
		}
	}

	return IRQ_HANDLED;
}

static int mtk_dtv_gmac_open(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "%s\n", __func__);

	/* memory init */
	mtk_dtv_gmac_mem_init(ndev);
	/* enable napi */
	napi_enable(&priv->napi_rx);

	/* disable all irq first */
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0058_X32_GMAC0, IRQ_DISABLE_ALL);
	mtk_write_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x0, REG_RX_INT_DLY_EN);

	/* enable irq */
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0050_X32_GMAC0, IRQ_ENABLE_BIT);
	mtk_write_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x1, REG_RX_INT_DLY_EN);

	/* apply rx mode */
	mtk_dtv_gmac_set_rx_mode(ndev);

	/* enable hw tx rx */
	mtk_write_fld_multi(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0, 2,
			    0x1, REG_ETH_CTL_RE, 0x1, REG_ETH_CTL_TE);

	mtk_dtv_gmac_link_status_update(ndev);
	if (!(timer_pending(&priv->link_timer)))
		add_timer(&priv->link_timer);
	netif_start_queue(ndev);

	priv->eth_drv_stage |= ETH_DRV_STAGE_OPEN;

	return 0;
}

static int mtk_dtv_gmac_stop(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "%s\n", __func__);
	netif_stop_queue(ndev);
	del_timer_sync(&priv->link_timer);

	/* disable hw tx rx */
	mtk_write_fld_multi(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0, 2,
			    0x0, REG_ETH_CTL_RE, 0x0, REG_ETH_CTL_TE);

	/* disable all irq first */
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0058_X32_GMAC0, IRQ_DISABLE_ALL);
	mtk_write_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x0, REG_RX_INT_DLY_EN);

	/* disable napi */
	napi_disable(&priv->napi_rx);

	/* memory release */
	if (priv->eth_drv_stage & ETH_DRV_STAGE_OPEN)
		mtk_dtv_gmac_mem_release(ndev);

	priv->eth_drv_stage &= ~ETH_DRV_STAGE_OPEN;

	return 0;
}

static netdev_tx_t mtk_dtv_gmac_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	uintptr_t skb_addr;
	dma_addr_t dma_addr, tx_memcpy_dma_addr;
	unsigned long flags;
	u32 tsr_val, retry;
	u8 tx_fifo[8] = {0};
	u8 i, tx_fifo_token = 0;

	spin_lock_irqsave(&priv->tx_lock, flags);
	if (skb->len > MAC_TX_MAX_LEN) {
		dev_err(priv->dev, "bad tx length(%d)\n", skb->len);
		spin_unlock_irqrestore(&priv->tx_lock, flags);
		return NETDEV_TX_BUSY;
	}

	retry = 0;

	do {
		tx_fifo_token = 0;
		tsr_val = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0028_X32_GMAC0);

		tx_fifo[0] = (((tsr_val & IRQ_TSR_IDLE) != 0) ? 1 : 0);
		tx_fifo[1] = (((tsr_val & IRQ_TSR_BNQ) != 0) ? 1 : 0);
		tx_fifo[2] = (((tsr_val & IRQ_TSR_TBNQ) != 0) ? 1 : 0);
		tx_fifo[3] = (((tsr_val & IRQ_TSR_FBNQ) != 0) ? 1 : 0);
		tx_fifo[4] = (((tsr_val & IRQ_TSR_FIFO1_IDLE) != 0) ? 1 : 0);
		tx_fifo[5] = (((tsr_val & IRQ_TSR_FIFO2_IDLE) != 0) ? 1 : 0);
		tx_fifo[6] = (((tsr_val & IRQ_TSR_FIFO3_IDLE) != 0) ? 1 : 0);
		tx_fifo[7] = (((tsr_val & IRQ_TSR_FIFO4_IDLE) != 0) ? 1 : 0);

		for (i = 0; i < 8; i++)
			tx_fifo_token += tx_fifo[i];

		retry++;

		if (retry > MAC_TX_BUFF_CHECK_RETRY_MAX) {
			spin_unlock_irqrestore(&priv->tx_lock, flags);
			return NETDEV_TX_BUSY;
		}
	} while (tx_fifo_token <= 4);

	skb_addr = priv->tx_memcpy_vir_addr + (TX_MEMCPY_SIZE_DEFAULT * priv->tx_memcpy_index);
	if (!skb_addr) {
		dev_err(priv->dev, "bad skb_addr\n");
		spin_unlock_irqrestore(&priv->tx_lock, flags);
		return -ENOMEM;
	}
	memcpy((void *)skb_addr, skb->data, skb->len);

	tx_memcpy_dma_addr = priv->tx_memcpy_dma_addr - MIU0_BUS_BASE;

	dma_addr = tx_memcpy_dma_addr + (TX_MEMCPY_SIZE_DEFAULT * priv->tx_memcpy_index);
	//dev_info(priv->dev, "tx packet dma: 0x%lx\n", dma_addr);
	//mtk_dtv_gmac_skb_dump(skb_addr, skb->len);

	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0018_X32_GMAC0, dma_addr);
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0020_X32_GMAC0, skb->len);

	priv->tx_memcpy_index++;
	if (priv->tx_memcpy_index >= TX_MEMCPY_NUM_DEFAULT)
		priv->tx_memcpy_index = 0;

	ndev->stats.tx_bytes += skb->len;
	ndev->stats.tx_packets++;

	dev_kfree_skb_any(skb);

	spin_unlock_irqrestore(&priv->tx_lock, flags);
	return NETDEV_TX_OK;
}

static int mtk_dtv_gmac_set_mac_address(struct net_device *ndev, void *addr)
{
	struct sockaddr *skaddr = addr;

	if (!is_valid_ether_addr(skaddr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(ndev->dev_addr, skaddr->sa_data, ndev->addr_len);
	mtk_dtv_gmac_hw_mac_addr_set(ndev);

	return 0;
}

static void mtk_dtv_gmac_set_rx_mode(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct netdev_hw_addr *ha;
	u32 reg_read, mc_filter_h, mc_filter_l, hash_bit, hash_val, i, tmp_crc, idx;
	u64 mac[6];
	u64 mac_addr;

	reg_read  = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0);

	if (ndev->flags & IFF_PROMISC) {
		/* enable promiscuous mode */
		reg_read |= 0x10UL;
	} else if (ndev->flags & (~IFF_PROMISC)) {
		/* disable promiscuous mode */
		reg_read &= ~(0x10UL);
	}

	if (ndev->flags & IFF_ALLMULTI) {
		/* enable all multicast mode */
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0120_X32_GMAC0, 0xffffffffUL);
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0128_X32_GMAC0, 0xffffffffUL);
		reg_read |= 0x40UL;
	} else if (ndev->flags & IFF_MULTICAST) {
		/* enable specific multicasts */
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0120_X32_GMAC0, 0x0);
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0128_X32_GMAC0, 0x0);

		netdev_for_each_mc_addr(ha, ndev) {
			hash_val = 0;
			mac_addr = 0;
			for (i = 0; i < 6; i++)
				mac[i] = (u64)ha->addr[i];

			mac_addr |= (mac[0] | (mac[1] << 8) | (mac[2] << 16) |
				     (mac[3] << 24) | (mac[4] << 32) | (mac[5] << 40));
			/* hash value */
			for (hash_bit = 0; hash_bit < 6; hash_bit++) {
				tmp_crc = ((mac_addr & (0x1UL << hash_bit)) >> hash_bit);
				for (i = 1; i < 8; i++) {
					idx = hash_bit + (i * 6);
					tmp_crc = (tmp_crc ^ ((mac_addr >> idx) & 0x1));
				}
				hash_val |= (tmp_crc << hash_bit);
			}
			mc_filter_l = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0120_X32_GMAC0);
			mc_filter_h = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0128_X32_GMAC0);

			if (hash_val < 32) {
				mc_filter_l |= (0x1UL << hash_val);
				mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0120_X32_GMAC0, mc_filter_l);
			} else {
				mc_filter_h |= (0x1UL << (hash_val - 32));
				mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0128_X32_GMAC0, mc_filter_h);
			}
		}
		reg_read |= 0x40UL;
	} else if (ndev->flags & ~(IFF_ALLMULTI | IFF_MULTICAST)) {
		/* disable all multicast mode */
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0120_X32_GMAC0, 0x0);
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0128_X32_GMAC0, 0x0);
		reg_read &= ~(0x40UL);
	}

	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0, reg_read);
}

/* woc */
static u8 _mtk_dtv_gmac_woc_index_get(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	return priv->woc_filter_index;
}

static void _mtk_dtv_gmac_woc_index_set(struct net_device *ndev, u8 index)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	priv->woc_filter_index = index;
}

static int _mtk_dtv_gmac_woc_sram_read(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 i, j, add;
	u8 val;

	/* set to sram mode */
	val = mtk_readb(REG_OFFSET_0D_L(BANK_BASE_ALBANY_3));
	val &= 0xFC;
	mtk_writeb(REG_OFFSET_0D_L(BANK_BASE_ALBANY_3), val);

	for (i = 0; i < WOC_PATTERN_BYTES_MAX; i++) {
		/* set pattern byte offset */
		val = mtk_readb(REG_OFFSET_0D_H(BANK_BASE_ALBANY_3));
		val &= 0x80;
		val |= (i & 0x7F);
		mtk_writeb(REG_OFFSET_0D_H(BANK_BASE_ALBANY_3), val);

		/* latch address to offset */
		val = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3));
		val |= (0x1 << BIT_WOC_CTRL_H_SRAM0_ADDR_LATCH);
		mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3), val);

		for (j = 0; j < WOC_FILTER_NUM_MAX; j += 2) {
			add = ((j >> 1) << 2);
			priv->woc_sram_pattern[j][i] =
				mtk_readb(REG_OFFSET_10_L(BANK_BASE_ALBANY_3) + add);
			priv->woc_sram_pattern[j + 1][i] =
				mtk_readb(REG_OFFSET_10_L(BANK_BASE_ALBANY_3) + add + 1);
		}
	}

	return 0;
}

static int _mtk_dtv_gmac_woc_sram_write(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 i, j, delay, add;
	u8 val;

	/* set to sram mode */
	val = mtk_readb(REG_OFFSET_0D_L(BANK_BASE_ALBANY_3));
	val &= 0xFC;
	val |= 0x02;
	mtk_writeb(REG_OFFSET_0D_L(BANK_BASE_ALBANY_3), val);

	for (i = 0; i < WOC_PATTERN_BYTES_MAX; i++) {
		delay = 0;

		/* set pattern byte offset */
		val = mtk_readb(REG_OFFSET_0D_H(BANK_BASE_ALBANY_3));
		val &= 0x80;
		val |= (i & 0x7F);
		mtk_writeb(REG_OFFSET_0D_H(BANK_BASE_ALBANY_3), val);

		/* latch address to offset */
		val = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3));
		val |= (0x1 << BIT_WOC_CTRL_H_SRAM0_ADDR_LATCH);
		mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3), val);

		for (j = 0; j < WOC_FILTER_NUM_MAX; j += 2) {
			add = ((j >> 1) << 2);
			mtk_writeb(REG_OFFSET_01_L(BANK_BASE_ALBANY_3) + add,
				   priv->woc_sram_pattern[j][i]);
			mtk_writeb(REG_OFFSET_01_L(BANK_BASE_ALBANY_3) + add + 1,
				   priv->woc_sram_pattern[j + 1][i]);
		}

		if (i == ETH_PROTOCOL_NUM_OFFSET ||
		    i == ETH_PROTOCOL_DEST_PORT_H_OFFSET ||
		    i == ETH_PROTOCOL_DEST_PORT_L_OFFSET) {
			/* enable all pattern byte check */
			mtk_writeb(REG_OFFSET_0B_L(BANK_BASE_ALBANY_3), 0xff);
			mtk_writeb(REG_OFFSET_0B_H(BANK_BASE_ALBANY_3), 0xff);
			val = mtk_readb(REG_OFFSET_0C_L(BANK_BASE_ALBANY_3));
			val |= 0x0f;
			mtk_writeb(REG_OFFSET_0C_L(BANK_BASE_ALBANY_3), val);
		} else {
			/* disable all pattern byte check */
			mtk_writeb(REG_OFFSET_0B_L(BANK_BASE_ALBANY_3), 0x0);
			mtk_writeb(REG_OFFSET_0B_H(BANK_BASE_ALBANY_3), 0x0);
			val = mtk_readb(REG_OFFSET_0C_L(BANK_BASE_ALBANY_3));
			val &= 0xf0;
			mtk_writeb(REG_OFFSET_0C_L(BANK_BASE_ALBANY_3), val);
		}

		/* write to sram */
		val = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3));
		val |= (0x1 << BIT_WOC_CTRL_H_SRAM0_WE);
		mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3), val);
		do {
			mdelay(1);
			val = mtk_readb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3));
			val &= (0x1 << BIT_WOC_CTRL_L_WOL_WRDY);

			delay++;
			if (delay == 20) {
				dev_err(priv->dev, "write woc sram0 failed 0x%x\n", val);
				goto write_sram_fail;
			}
		} while (val == 0);

		val = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3));
		val |= (0x1 << BIT_WOC_CTRL_H_WOL_WRDY_CR);
		mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3), val);
	}

	return 0;

write_sram_fail:
	return -1;
}

static int _mtk_dtv_gmac_woc_protocol_port_set(struct net_device *ndev,
					       char protocol_type, int *port_array, int count)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int i;

	if (priv->woc_filter_index >= WOC_FILTER_NUM_MAX) {
		dev_err(priv->dev, "already set woc 20 packets!\n");
		return -1;
	}

	if (protocol_type != UDP_PROTOCOL && protocol_type != TCP_PROTOCOL) {
		dev_err(priv->dev, "woc protocol type is wrong(UDP = 17, TCP = 6).!\n");
		return -1;
	}

	if (count > WOC_COUNT_UPPER_BOUND || count < WOC_COUNT_LOWER_BOUND) {
		dev_err(priv->dev, "count is not in 1~20 !\n");
		return -1;
	}

	_mtk_dtv_gmac_woc_sram_read(ndev);

	for (i = 0; i < count; i++) {
		/* byte23 is udp/tcp protocol number. */
		priv->woc_sram_pattern[priv->woc_filter_index][ETH_PROTOCOL_NUM_OFFSET] =
			protocol_type;

		/* byte36 and byte37 is udp/tcp destination port. */
		priv->woc_sram_pattern[priv->woc_filter_index][ETH_PROTOCOL_DEST_PORT_H_OFFSET] =
			(char)(port_array[i] >> 8);
		priv->woc_sram_pattern[priv->woc_filter_index][ETH_PROTOCOL_DEST_PORT_L_OFFSET] =
			(char)(port_array[i] & 0xff);
		priv->woc_filter_index++;
	}

	_mtk_dtv_gmac_woc_sram_write(ndev);

	priv->is_woc = true;

	return priv->woc_filter_index - count;
}

static int _mtk_dtv_gmac_woc_detect_len_set(struct net_device *ndev, int len)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u8 i, val;
	u32 add, bit_mask = 0;

	/* set to check length mode */
	val = mtk_readb(REG_OFFSET_0D_L(BANK_BASE_ALBANY_3));
	val |= 0x03;
	mtk_writeb(REG_OFFSET_0D_L(BANK_BASE_ALBANY_3), val);

	for (i = 0; i < _mtk_dtv_gmac_woc_index_get(ndev); i++) {
		add = ((i >> 1) << 2);
		mtk_writeb(REG_OFFSET_01_L(BANK_BASE_ALBANY_3) + add + (i % 2), len);
		bit_mask |= (0x1 << i);
	}

	/* enable pattern length check */
	mtk_writeb(REG_OFFSET_0B_L(BANK_BASE_ALBANY_3), (bit_mask & 0xffUL));
	mtk_writeb(REG_OFFSET_0B_H(BANK_BASE_ALBANY_3), (bit_mask & 0xff00UL) >> 8);
	val = mtk_readb(REG_OFFSET_0C_L(BANK_BASE_ALBANY_3));
	val &= 0xf0;
	val |= ((bit_mask & 0xf0000UL) >> 16);
	mtk_writeb(REG_OFFSET_0C_L(BANK_BASE_ALBANY_3), val);

	return 0;
}

static void _mtk_dtv_gmac_woc_config(struct net_device *ndev, bool enable)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int ret = 0;
	u8 val = 0;

	if (enable) {
		_mtk_dtv_gmac_woc_detect_len_set(ndev, WOC_PATTERN_CHECK_LEN_MAX);

		val = mtk_readb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3));
		val |= (0x1 << BIT_WOC_CTRL_L_WOL_EN);
		mtk_writeb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3), val);

		val = mtk_readb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3));
		val |= (0x1 << BIT_WOC_CTRL_L_STORAGE_EN);
		mtk_writeb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3), val);

		val = mtk_readb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2));
		val |= (0x1 << BIT_WOL_MODE_WOL_EN);
		mtk_writeb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2), val);

		ret = pm_set_wakeup_config("cast", true);
		if (ret)
			dev_err(priv->dev, "set wakeup config cast failed: enable=0x%x, %d\n",
				enable, ret);

		dev_info(priv->dev, "enable woc\n");
	} else {
		/* not disable wol here, this is for woc */
		val = mtk_readb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3));
		val &= ~(0x1 << BIT_WOC_CTRL_L_WOL_EN);
		mtk_writeb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3), val);

		val = mtk_readb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3));
		val &= ~(0x1 << BIT_WOC_CTRL_L_STORAGE_EN);
		mtk_writeb(REG_OFFSET_00_L(BANK_BASE_ALBANY_3), val);

		/* set to check length mode */
		val = mtk_readb(REG_OFFSET_0D_L(BANK_BASE_ALBANY_3));
		val |= 0x01;
		mtk_writeb(REG_OFFSET_0D_L(BANK_BASE_ALBANY_3), val);

		/* disable all pattern byte check */
		mtk_writeb(REG_OFFSET_0B_L(BANK_BASE_ALBANY_3), 0x0);
		mtk_writeb(REG_OFFSET_0B_H(BANK_BASE_ALBANY_3), 0x0);
		val = mtk_readb(REG_OFFSET_0C_L(BANK_BASE_ALBANY_3));
		val &= 0xf0;
		mtk_writeb(REG_OFFSET_0C_L(BANK_BASE_ALBANY_3), val);

		ret = pm_set_wakeup_config("cast", false);
		if (ret)
			dev_err(priv->dev, "set wakeup config cast failed: enable=0x%x, %d\n",
				enable, ret);

		dev_info(priv->dev, "disable woc\n");
	}
}

static void _mtk_dtv_gmac_woc_wol_clear_irq(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u8 val = 0;

	/* clear wol irq status */
	val = mtk_readb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2));
	val |= (0x1 << BIT_WOL_MODE_WOL_CLEAR);
	mtk_writeb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2), val);
	val &= ~(0x1 << BIT_WOL_MODE_WOL_CLEAR);
	mtk_writeb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2), val);

	/* clear woc irq status */
	val = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3));
	val |= (0x1 << BIT_WOC_CTRL_H_WOL_INT_CLEAR);
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3), val);

	dev_info(priv->dev, "clear woc wol irq status\n");
}

//static void _mtk_dtv_gmac_woc_auto_load_trigger(struct net_device *ndev)
//{
//	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
//	u8 val_det_l, val_det_m, val_det_h, val;

//	val_det_l = mtk_readb(REG_OFFSET_0E_L(BANK_BASE_ALBANY_3));
//	val_det_m = mtk_readb(REG_OFFSET_0E_H(BANK_BASE_ALBANY_3));
//	val_det_h = mtk_readb(REG_OFFSET_0F_L(BANK_BASE_ALBANY_3));
//	val_det_h &= 0x0f;

//	if (val_det_l != 0 || val_det_m != 0 || val_det_h != 0) {
		/* pattern match hit */
		/* trigger autoload */
//		val = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3));
//		val |= (0x1 << BIT_WOC_CTRL_H_WOL_STORAGE_AUTOLOAD);
//		mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3), val);
		/* clear interrupt */
//		val = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3));
//		val |= (0x1 << BIT_WOC_CTRL_H_WOL_INT_CLEAR);
//		mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_3), val);
//		dev_info(priv->dev, "trigger woc auto load\n");
//	}
//}

static bool _mtk_dtv_gmac_flag_is_wol_get(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int ret = 0;
	bool pm_flag = false;

	ret = pm_get_wakeup_config("lan", &pm_flag);
	if (ret) {
		dev_err(priv->dev, "get wakeup config lan failed, return is_wol=0x%x: %d\n",
			priv->is_wol, ret);
		return priv->is_wol;
	}

	priv->is_wol = pm_flag;

	return priv->is_wol;
}

static void _mtk_dtv_gmac_flag_is_wol_set(struct net_device *ndev, bool enable)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int ret = 0;

	if (enable) {
		ret = pm_set_wakeup_config("lan", true);
		if (ret)
			dev_err(priv->dev, "set wakeup config lan failed: enable=0x%x, %d\n",
				enable, ret);
	} else {
		ret = pm_set_wakeup_config("lan", false);
		if (ret)
			dev_err(priv->dev, "set wakeup config lan failed: enable=0x%x, %d\n",
				enable, ret);
	}

	priv->is_wol = enable;
}

static void _mtk_dtv_gmac_wol_config(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int ret = 0;
	u8 val = 0;
	bool pm_flag = false;

	/* get wakeup config from pm */
	ret = pm_get_wakeup_config("lan", &pm_flag);
	if (ret) {
		dev_err(priv->dev, "get wakeup config lan failed: %d\n", ret);
		pm_flag = priv->is_wol;
	}

	dev_info(priv->dev, "get wakeup config lan: pm_flag=0x%x\n",
		 pm_flag);

	if (pm_flag) {
		val = mtk_readb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2));
		val |= (0x1 << BIT_WOL_MODE_WOL_EN);
		mtk_writeb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2), val);

		ret = pm_set_wakeup_config("lan", true);
		if (ret)
			dev_err(priv->dev, "set wakeup config lan failed: pm_flag=0x%x, %d\n",
				pm_flag, ret);

		dev_info(priv->dev, "enable wol\n");
	} else {
		/* not disable wol here, this is for woc */
		val = mtk_readb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2));
		val &= ~(0x1 << BIT_WOL_MODE_WOL_EN);
		mtk_writeb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2), val);

		ret = pm_set_wakeup_config("lan", false);
		if (ret)
			dev_err(priv->dev, "set wakeup config lan failed: pm_flag=0x%x, %d\n",
				pm_flag, ret);

		dev_info(priv->dev, "disable wol\n");
	}
}

static void _mtk_dtv_gmac_wol_config_force_disable(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int ret = 0;
	u8 val = 0;

	/* not disable wol here, this is for woc */
	val = mtk_readb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2));
	val &= ~(0x1 << BIT_WOL_MODE_WOL_EN);
	mtk_writeb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2), val);

	ret = pm_set_wakeup_config("lan", false);
	if (ret)
		dev_err(priv->dev, "set wakeup config lan force disable failed: %d\n",
			ret);

	dev_info(priv->dev, "force disable wol\n");
}

static int _mtk_dtv_gmac_wol_ioctl_get(struct net_device *ndev, struct ifreq *req)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct ioctl_wol_para_cmd wol_cmd;

	if (copy_from_user(&wol_cmd, req->ifr_data, sizeof(wol_cmd))) {
		dev_err(priv->dev, "wol get: copy from user fail\n");
		return -EFAULT;
	}

	wol_cmd.is_enable_wol = _mtk_dtv_gmac_flag_is_wol_get(ndev);
	dev_info(priv->dev, "wol get: %s wol\n",
		 wol_cmd.is_enable_wol ? "enable" : "disable");

	if (copy_to_user(req->ifr_data, &wol_cmd, sizeof(wol_cmd))) {
		dev_err(priv->dev, "wol get: copy to user fail\n");
		return -EFAULT;
	}

	return 0;
}

static int _mtk_dtv_gmac_wol_forward_magic_packet(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct sk_buff *skb;
	unsigned long flags;
	char maddr0 = ndev->dev_addr[0];
	char maddr1 = ndev->dev_addr[1];
	char maddr2 = ndev->dev_addr[2];
	char maddr3 = ndev->dev_addr[3];
	char maddr4 = ndev->dev_addr[4];
	char maddr5 = ndev->dev_addr[5];
	/* fixed packet header, it's useless */
	char skb_magic[144] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3c, 0x52,
			       0x82, 0x64, 0x74, 0x0f, 0x08, 0x00, 0x45, 0x00,
			       0x00, 0x82, 0x48, 0x80, 0x00, 0x00, 0x80, 0x11,
			       0x00, 0x00, 0xc0, 0xa8, 0x03, 0x3c, 0xff, 0xff,
			       0xff, 0xff, 0xde, 0x4f, 0x00, 0x09, 0x00, 0x6e,
			       0x53, 0x1e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5,
			       maddr0, maddr1, maddr2, maddr3, maddr4, maddr5};

	skb = netdev_alloc_skb(ndev, MAC_RX_MAX_LEN + MAC_EXTRA_PKT_LEN);
	if (!skb)
		return -ENOMEM;

	skb_put(skb, 144);
	memcpy(skb->data, skb_magic, 144);
	//dev_info(priv->dev, "forward_magic_packet start recv:\n");
	//mtk_dtv_gmac_skb_dump(skb->data, 144);
	skb->ip_summed = CHECKSUM_NONE;
	skb->protocol = eth_type_trans(skb, ndev);
	spin_lock_irqsave(&priv->magic_lock, flags);
	netif_receive_skb(skb);
	spin_unlock_irqrestore(&priv->magic_lock, flags);

	return 0;
}

/* APIs for wol set and get */
struct net_device *_mtk_mac_get_netdev(void)
{
	struct device_node *dev_node;
	struct platform_device *pdev;
	struct net_device *ndev;

	dev_node = of_find_compatible_node(NULL, NULL, MTK_GMAC_COMPAT_NAME);
	if (!dev_node) {
		pr_err("mac get netdev: get dev node failed\n");
		return NULL;
	}
	pdev = of_find_device_by_node(dev_node);
	if (!pdev) {
		pr_err("mac get netdev: get pdev failed\n");
		return NULL;
	}
	ndev = platform_get_drvdata(pdev);
	if (!ndev) {
		pr_err("mac get netdev: get ndev failed\n");
		return NULL;
	}

	return ndev;
}

/**
 * mtk_mac_wol_set() - set mac wol to enable or disable
 * @enable_wol: config to wol
 *
 * Return 0 if success. Or negative errno on failure.
 */
int mtk_mac_wol_set(bool enable_wol)
{
	struct net_device *ndev = _mtk_mac_get_netdev();
	struct mtk_dtv_gmac_private *priv;

	if (!ndev) {
		pr_err("%s: get ndev failed\n", __func__);
		return -ENODEV;
	}

	priv = netdev_priv(ndev);
	if (!priv) {
		pr_err("%s: get priv data failed\n", __func__);
		return -ENODEV;
	}

	if (enable_wol) {
		pr_info("%s: enable wol\n", __func__);
		_mtk_dtv_gmac_flag_is_wol_set(ndev, true);
	} else {
		pr_info("%s: disable wol\n", __func__);
		_mtk_dtv_gmac_flag_is_wol_set(ndev, false);
	}

	return 0;
}
EXPORT_SYMBOL(mtk_mac_wol_set);

/**
 * mtk_mac_wol_get() - get mac wol is enable or disable
 * @enable_wol: wol config state
 *
 * Return 0 if success. Or negative errno on failure.
 */
int mtk_mac_wol_get(bool *enable_wol)
{
	struct net_device *ndev = _mtk_mac_get_netdev();
	struct mtk_dtv_gmac_private *priv;

	if (!ndev) {
		pr_err("%s: get ndev failed\n", __func__);
		return -ENODEV;
	}

	priv = netdev_priv(ndev);
	if (!priv) {
		pr_err("%s: get priv data failed\n", __func__);
		return -ENODEV;
	}

	*enable_wol = _mtk_dtv_gmac_flag_is_wol_get(ndev);
	pr_info("%s: %s wol\n", __func__,
		*enable_wol ? "enable" : "disable");

	return 0;
}
EXPORT_SYMBOL(mtk_mac_wol_get);
/* end of woc */

static int mtk_dtv_gmac_ioctl(struct net_device *ndev, struct ifreq *req, int cmd)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct ioctl_wol_para_cmd wol_cmd;
	struct ioctl_woc_para_cmd __user *req_ifr_data;
	struct ioctl_woc_para_cmd woc_cmd;
#ifdef CONFIG_COMPAT
	struct ioctl_woc_para_cmd32 __user *req_ifr_data32;
	struct ioctl_woc_para_cmd32 woc_cmd32;
#endif /* CONFIG_COMPAT */
	void __user *woc_port_array;
	u32 *port_array;
	int ret = 0;

	switch (cmd) {
	/* slt-test */
	case SIOC_MTK_MAC_SLT:
		ret = mtk_dtv_gmac_slt_ioctl(ndev, req);

		return ret;
	/* cts */
	case SIOC_MTK_MAC_CTS:
		ret = mtk_dtv_gmac_cts_ioctl(ndev, req);

		return ret;
	/* woc */
	case SIOC_SET_WOC_CMD:
#ifdef CONFIG_COMPAT
		if (is_compat_task()) {
			req_ifr_data32 = compat_ptr(ptr_to_compat(req->ifr_data));

			if (copy_from_user(&woc_cmd32, req_ifr_data32,
					   sizeof(struct ioctl_woc_para_cmd32))) {
				dev_err(priv->dev, "woc set: copy from user32 fail\n");
				return -EFAULT;
			}

			if (woc_cmd32.port_count > WOC_COUNT_UPPER_BOUND ||
			    woc_cmd32.port_count < WOC_COUNT_LOWER_BOUND) {
				dev_err(priv->dev, "count is not in 1~20 !\n");
				return -EFAULT;
			}

			port_array = kmalloc((sizeof(u32) * woc_cmd32.port_count), GFP_KERNEL);
			if (!port_array)
				return -EFAULT;

			woc_port_array = compat_ptr(woc_cmd32.port_array);
			if (!woc_port_array) {
				dev_err(priv->dev, "null 32 woc_port_array!\n");
				kfree(port_array);
				return -EFAULT;
			}

			ret = copy_from_user(port_array, woc_port_array,
					     (sizeof(u32) * woc_cmd32.port_count));
			if (ret) {
				dev_err(priv->dev,
					"wop_port_array: 0x%p fail! count32 = %d, ret = %d\n",
					woc_port_array,
					woc_cmd32.port_count, ret);
				kfree(port_array);

				return -EFAULT;
			}

			ret = _mtk_dtv_gmac_woc_protocol_port_set(ndev,
								  woc_cmd32.protocol_type,
								  port_array,
								  woc_cmd32.port_count);
		} else {
#endif /* CONFIG_COMPAT */
			req_ifr_data = req->ifr_data;

			if (copy_from_user(&woc_cmd, req->ifr_data,
					   sizeof(struct ioctl_woc_para_cmd))) {
				dev_err(priv->dev, "woc set: copy from user fail\n");
				return -EFAULT;
			}

			if (woc_cmd.port_count > WOC_COUNT_UPPER_BOUND ||
			    woc_cmd.port_count < WOC_COUNT_LOWER_BOUND) {
				dev_err(priv->dev, "count is not in 1~20 !\n");
				return -EFAULT;
			}

			port_array = kmalloc((sizeof(u32) * woc_cmd.port_count),
					     GFP_KERNEL);
			if (!port_array)
				return -EFAULT;

			woc_port_array = woc_cmd.port_array;
			if (!woc_port_array) {
				dev_err(priv->dev, "null woc_port_array!\n");
				kfree(port_array);
				return -EFAULT;
			}

			ret = copy_from_user(port_array, woc_port_array,
					     (sizeof(u32) * woc_cmd.port_count));
			if (ret) {
				dev_err(priv->dev,
					"woc_port_array: 0x%p fail! count = %d, ret = %d\n",
					woc_port_array, woc_cmd.port_count,
					ret);
				kfree(port_array);

				return -EFAULT;
			}

			ret = _mtk_dtv_gmac_woc_protocol_port_set(ndev,
								  woc_cmd.protocol_type,
								  port_array,
								  woc_cmd.port_count);
#ifdef CONFIG_COMPAT
		}
#endif /* CONFIG_COMPAT */
		kfree(port_array);

		return ret;

	case SIOC_CLR_WOC_CMD:
		_mtk_dtv_gmac_woc_index_set(ndev, 0);
		priv->is_woc = false;
		return 0;
	/* end of woc */

	/* wol */
	case SIOC_SET_WOL_CMD:
		if (copy_from_user(&wol_cmd, req->ifr_data, sizeof(wol_cmd))) {
			dev_err(priv->dev, "wol set: copy from user fail\n");
			return -EFAULT;
		}

		if (wol_cmd.is_enable_wol) {
			dev_info(priv->dev, "wol set: enable wol\n");
			_mtk_dtv_gmac_flag_is_wol_set(ndev, true);
		} else {
			dev_info(priv->dev, "wol set: disable wol\n");
			_mtk_dtv_gmac_flag_is_wol_set(ndev, false);
		}
		return 0;

	case SIOC_GET_WOL_CMD:
		ret = _mtk_dtv_gmac_wol_ioctl_get(ndev, req);

		return ret;
	/* end of wol */

	default:
		return -EOPNOTSUPP;
	}
}

static struct net_device_stats *mtk_dtv_gmac_get_stats(struct net_device *ndev)
{
	return &ndev->stats;
}

static const struct net_device_ops mtk_dtv_gmac_netdev_ops = {
	.ndo_open = mtk_dtv_gmac_open,
	.ndo_stop = mtk_dtv_gmac_stop,
	.ndo_start_xmit = mtk_dtv_gmac_start_xmit,
	.ndo_set_mac_address = mtk_dtv_gmac_set_mac_address,
	.ndo_set_rx_mode = mtk_dtv_gmac_set_rx_mode,
	.ndo_do_ioctl = mtk_dtv_gmac_ioctl,
	.ndo_get_stats = mtk_dtv_gmac_get_stats,
};

/* ethtool functions */
static void mtk_dtv_gmac_get_drvinfo(struct net_device *ndev, struct ethtool_drvinfo *drvinfo)
{
	strscpy(drvinfo->driver, KBUILD_MODNAME, sizeof(drvinfo->driver));
	strscpy(drvinfo->version, mtk_gmac_driver_version, sizeof(drvinfo->version));
}

static int mtk_dtv_gmac_get_regs_len(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int len = 0;

	if (priv->mac_type == MAC_TYPE_EMAC)
		len = MTK_EMAC_REGS_LEN * (int)sizeof(u32);
	else
		len = MTK_GMAC_REGS_LEN * (int)sizeof(u32);

	return len;
}

static void mtk_dtv_gmac_get_regs(struct net_device *ndev, struct ethtool_regs *regs, void *data)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u32 *regs_buff = data;
	int i = 0;

	for (i = 0; i < MTK_MAC_REGS_LEN_PER_BANK; i++)
		*regs_buff++ = mtk_readl(BANK_BASE_MAC_0 + (i * 8));

	for (i = 0; i < (MTK_MAC_REGS_LEN_PER_BANK * 2); i += 2)
		*regs_buff++ = ((mtk_readl(BANK_BASE_MAC_1 + (i * 4)) & 0xffff) |
				((mtk_readl(BANK_BASE_MAC_1 + ((i + 1) * 4)) & 0xffff) << 16));

	for (i = 0; i < (MTK_MAC_REGS_LEN_PER_BANK * 2); i += 2)
		*regs_buff++ = ((mtk_readl(BANK_BASE_MAC_2 + (i * 4)) & 0xffff) |
				((mtk_readl(BANK_BASE_MAC_2 + ((i + 1) * 4)) & 0xffff) << 16));

	for (i = 0; i < (MTK_MAC_REGS_LEN_PER_BANK * 2); i += 2)
		*regs_buff++ = ((mtk_readl(BANK_BASE_MAC_3 + (i * 4)) & 0xffff) |
				((mtk_readl(BANK_BASE_MAC_3 + ((i + 1) * 4)) & 0xffff) << 16));

	if (priv->mac_type == MAC_TYPE_GMAC)
		for (i = 0; i < (MTK_MAC_REGS_LEN_PER_BANK * 2); i += 2)
			*regs_buff++ = ((mtk_readl(BANK_BASE_MAC_4 + (i * 4)) & 0xffff) |
				((mtk_readl(BANK_BASE_MAC_4 + ((i + 1) * 4)) & 0xffff) << 16));

	for (i = 0; i < (MTK_MAC_REGS_LEN_PER_BANK * 2); i += 2)
		*regs_buff++ = ((mtk_readl(BANK_BASE_ALBANY_0 + (i * 4)) & 0xffff) |
				((mtk_readl(BANK_BASE_ALBANY_0 + ((i + 1) * 4)) & 0xffff) << 16));

	for (i = 0; i < (MTK_MAC_REGS_LEN_PER_BANK * 2); i += 2)
		*regs_buff++ = ((mtk_readl(BANK_BASE_ALBANY_1 + (i * 4)) & 0xffff) |
				((mtk_readl(BANK_BASE_ALBANY_1 + ((i + 1) * 4)) & 0xffff) << 16));

	for (i = 0; i < (MTK_MAC_REGS_LEN_PER_BANK * 2); i += 2)
		*regs_buff++ = ((mtk_readl(BANK_BASE_ALBANY_2 + (i * 4)) & 0xffff) |
				((mtk_readl(BANK_BASE_ALBANY_2 + ((i + 1) * 4)) & 0xffff) << 16));

	for (i = 0; i < (MTK_MAC_REGS_LEN_PER_BANK * 2); i += 2)
		*regs_buff++ = ((mtk_readl(BANK_BASE_ALBANY_3 + (i * 4)) & 0xffff) |
				((mtk_readl(BANK_BASE_ALBANY_3 + ((i + 1) * 4)) & 0xffff) << 16));
}

static void _mtk_dtv_gmac_get_sopass(struct net_device *ndev, u8 *data)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u16 *sval = (u16 *)data;

	sval[0] = (mtk_readb(REG_OFFSET_24_L(BANK_BASE_ALBANY_2)) & 0xff) |
		  ((mtk_readb(REG_OFFSET_24_H(BANK_BASE_ALBANY_2)) & 0xff) << 8);
	sval[1] = (mtk_readb(REG_OFFSET_25_L(BANK_BASE_ALBANY_2)) & 0xff) |
		  ((mtk_readb(REG_OFFSET_25_H(BANK_BASE_ALBANY_2)) & 0xff) << 8);
}

static void mtk_dtv_gmac_get_wol(struct net_device *ndev, struct ethtool_wolinfo *wolinfo)
{
	memset(&wolinfo->sopass, 0, sizeof(wolinfo->sopass));
	wolinfo->supported = (WAKE_MAGIC | WAKE_MAGICSECURE);
	wolinfo->wolopts = (WAKE_MAGIC | WAKE_MAGICSECURE);

	if (wolinfo->wolopts & WAKE_MAGICSECURE)
		_mtk_dtv_gmac_get_sopass(ndev, wolinfo->sopass);
}

static void _mtk_dtv_gmac_set_sopass(struct net_device *ndev, u8 *newval)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u16 *sval = (u16 *)newval;
	u8 val = 0;

	val = mtk_readb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2));

	if (sval[0] == 0x0000 && sval[1] == 0x0000) {
		/* set 0 to disable */
		mtk_writeb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2),
			   (val & (~(0x1 << BIT_WOL_MODE_WOL_PASS))));
		return;
	}

	mtk_writeb(REG_OFFSET_24_L(BANK_BASE_ALBANY_2), (sval[0] & 0xff));
	mtk_writeb(REG_OFFSET_24_H(BANK_BASE_ALBANY_2), ((sval[0] & 0xff00) >> 8));
	mtk_writeb(REG_OFFSET_25_L(BANK_BASE_ALBANY_2), (sval[0] & 0xff));
	mtk_writeb(REG_OFFSET_25_H(BANK_BASE_ALBANY_2), ((sval[0] & 0xff00) >> 8));
	mtk_writeb(REG_OFFSET_23_L(BANK_BASE_ALBANY_2), (val | (0x1 << BIT_WOL_MODE_WOL_PASS)));
}

static int mtk_dtv_gmac_set_wol(struct net_device *ndev, struct ethtool_wolinfo *wolinfo)
{
	if (wolinfo->wolopts & (WAKE_PHY | WAKE_UCAST | WAKE_MCAST | WAKE_BCAST | WAKE_ARP))
		return -EOPNOTSUPP;

	if (wolinfo->wolopts & (WAKE_MAGIC)) {
		_mtk_dtv_gmac_flag_is_wol_set(ndev, true);

		if (wolinfo->wolopts & (WAKE_MAGICSECURE))
			_mtk_dtv_gmac_set_sopass(ndev, wolinfo->sopass);
	} else {
		_mtk_dtv_gmac_flag_is_wol_set(ndev, false);
	}

	_mtk_dtv_gmac_wol_config(ndev);

	return 0;
}

static int mtk_dtv_gmac_nway_reset(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	return mii_nway_restart(&priv->mii);
}

static int mtk_dtv_gmac_get_link_ksettings(struct net_device *ndev,
					   struct ethtool_link_ksettings *ecmd)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	mii_ethtool_get_link_ksettings(&priv->mii, ecmd);

	return 0;
}

static int mtk_dtv_gmac_set_link_ksettings(struct net_device *ndev,
					   const struct ethtool_link_ksettings *ecmd)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int ret = 0;

	ret = mii_ethtool_set_link_ksettings(&priv->mii, ecmd);
	if (ret)
		netdev_err(ndev, "Error: mii_ethtool_set_link_ksettings: %d\n", ret);

	return ret;
}

static int mtk_dtv_gmac_mdio_read(struct net_device *ndev, int addr, int reg)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 value = 0;
	u8 phy_addr = 0;
	u8 reg_addr = 0;

	phy_addr = (u8)addr;
	reg_addr = (u8)reg;
	value = reg_ops->read_phy(ndev, phy_addr, reg_addr);

	return (int)value;
}

static void mtk_dtv_gmac_mdio_write(struct net_device *ndev, int addr, int reg, int val)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 value = 0;
	u8 phy_addr = 0;
	u8 reg_addr = 0;

	phy_addr = (u8)addr;
	reg_addr = (u8)reg;
	value = (u32)val;
	reg_ops->write_phy(ndev, phy_addr, reg_addr, value);
}

static const struct ethtool_ops mtk_dtv_gmac_ethtool_ops = {
	.get_drvinfo = mtk_dtv_gmac_get_drvinfo,
	.get_regs_len = mtk_dtv_gmac_get_regs_len,
	.get_regs = mtk_dtv_gmac_get_regs,
	.get_wol = mtk_dtv_gmac_get_wol,
	.set_wol = mtk_dtv_gmac_set_wol,
	.nway_reset = mtk_dtv_gmac_nway_reset,
	.get_link = ethtool_op_get_link,
	.get_link_ksettings = mtk_dtv_gmac_get_link_ksettings,
	.set_link_ksettings = mtk_dtv_gmac_set_link_ksettings,
};

static int mtk_dtv_gmac_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "enter gmac suspend\n");

	netif_stop_queue(ndev);
	del_timer_sync(&priv->link_timer);

	/* disable hw tx rx */
	mtk_write_fld_multi(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0, 2,
			    0x0, REG_ETH_CTL_RE, 0x0, REG_ETH_CTL_TE);

	/* disable all irq first */
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0058_X32_GMAC0, IRQ_DISABLE_ALL);
	mtk_write_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x0, REG_RX_INT_DLY_EN);
	disable_irq(ndev->irq);

	/* woc */
	if (priv->eth_drv_stage & ETH_DRV_STAGE_OPEN) {
		_mtk_dtv_gmac_wol_config(ndev);
		_mtk_dtv_gmac_woc_config(ndev, priv->is_woc);
		/* memory release */
		mtk_dtv_gmac_mem_release(ndev);
		dev_info(priv->dev, "gmac suspend with eth open\n");
	} else {
		_mtk_dtv_gmac_woc_config(ndev, false);
		_mtk_dtv_gmac_wol_config_force_disable(ndev);
		dev_info(priv->dev, "gmac suspend without eth open\n");
	}

	/* if both wol and woc are disabled, power down phy */
	/* do not power down phy to avoid GA connecting issue */
	//if (!priv->is_wol && !priv->is_woc) {
	//	mtk_dtv_gmac_hw_power_down_clk(ndev);
	//	dev_info(priv->dev, "power down gmac phy\n");
	//}

	dev_info(priv->dev, "leave gmac suspend\n");

	return 0;
}

static int mtk_dtv_gmac_resume(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int ret;
	char wk_reason[PM_WK_REASON_NAME_SIZE];
	char *get_reason;

	dev_info(priv->dev, "enter gmac resume\n");

	/* if wol wakeup, send magic to kernel */
	memset(wk_reason, 0, sizeof(wk_reason));
	get_reason = pm_get_wakeup_reason_str();
	if (!get_reason) {
		dev_err(priv->dev, "cannot get wakeup reason\n");
	} else {
		strncpy(wk_reason, get_reason, sizeof(wk_reason) - 1);
		dev_info(priv->dev, "wakeup reason is %s\n", wk_reason);
		if (!strncmp("lan", wk_reason, sizeof(wk_reason))) {
			dev_info(priv->dev, "wakeup by wol, send magic to kernel\n");
			ret = _mtk_dtv_gmac_wol_forward_magic_packet(ndev);
			if (ret)
				dev_err(priv->dev, "send magic failed: %d\n", ret);

			dev_info(priv->dev, "wakeup by wol, notify wakeup_reason node\n");
			sysfs_notify(&priv->dev->kobj, "mtk_dbg", "wakeup_reason");
		}
	}

	/* disable woc wol always */
	_mtk_dtv_gmac_woc_config(ndev, false);
	_mtk_dtv_gmac_wol_config_force_disable(ndev);
	_mtk_dtv_gmac_woc_wol_clear_irq(ndev);

	/* restore wol flag */
	_mtk_dtv_gmac_flag_is_wol_set(ndev, priv->is_wol);

	ret = mtk_dtv_gmac_hw_init(ndev, true);
	if (ret)
		dev_err(priv->dev, "hw init fail in resume\n");

	if (priv->eth_drv_stage & ETH_DRV_STAGE_OPEN) {
		/* memory init */
		mtk_dtv_gmac_mem_init(ndev);

		/* disable all irq first */
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0058_X32_GMAC0, IRQ_DISABLE_ALL);
		mtk_write_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x0, REG_RX_INT_DLY_EN);

		/* enable irq */
		enable_irq(ndev->irq);
		mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0050_X32_GMAC0, IRQ_ENABLE_BIT);
		mtk_write_fld(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x1, REG_RX_INT_DLY_EN);

		/* enable hw tx rx */
		mtk_write_fld_multi(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0, 2,
				    0x1, REG_ETH_CTL_RE, 0x1, REG_ETH_CTL_TE);

		mtk_dtv_gmac_link_status_update(ndev);
		if (!(timer_pending(&priv->link_timer)))
			add_timer(&priv->link_timer);
		netif_start_queue(ndev);
		dev_info(priv->dev, "gmac resume with eth open\n");
	} else {
		/* enable irq always */
		enable_irq(ndev->irq);
		dev_info(priv->dev, "gmac resume without eth open\n");
	}

	dev_info(priv->dev, "leave gmac resume\n");

	return 0;
}

static int mtk_dtv_gmac_of_get_bank(struct net_device *ndev, struct device_node *np)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = NULL;

	/* get of properties */
	priv->mac_bank_0 = of_iomap(np, 0);
	priv->mac_bank_1 = of_iomap(np, 1);
	priv->albany_bank_0 = of_iomap(np, 2);
	priv->clkgen_bank_0 = of_iomap(np, 3);
	priv->chip_bank = of_iomap(np, 4);
	priv->clkgen_pm_bank_0 = of_iomap(np, 5);
	priv->tzpc_aid_bank_0 = of_iomap(np, 6);
	priv->pm_misc_bank = of_iomap(np, 7);
	priv->mpll_bank = of_iomap(np, 8);

	if (!priv->mac_bank_0 || !priv->mac_bank_1 ||
	    !priv->albany_bank_0 || !priv->clkgen_bank_0 ||
	    !priv->chip_bank || !priv->clkgen_pm_bank_0 ||
	    !priv->tzpc_aid_bank_0 || !priv->pm_misc_bank ||
	    !priv->mpll_bank) {
		dev_err(priv->dev, "fail to ioremap, mac0(0x%lx), mac1(0x%lx), ",
			(unsigned long)priv->mac_bank_0, (unsigned long)priv->mac_bank_1);
		dev_err(priv->dev, "albany0(0x%lx), clkgen0(0x%lx), ",
			(unsigned long)priv->albany_bank_0, (unsigned long)priv->clkgen_bank_0);
		dev_err(priv->dev, "chip(0x%lx), clkgen0_pm(0x%lx)\n",
			(unsigned long)priv->chip_bank, (unsigned long)priv->clkgen_pm_bank_0);
		dev_err(priv->dev, "tzpc_aid_0(0x%lx), pm_misc(0x%lx)\n",
			(unsigned long)priv->tzpc_aid_bank_0, (unsigned long)priv->pm_misc_bank);
		dev_err(priv->dev, "mpll(0x%lx)\n",
			(unsigned long)priv->mpll_bank);
		return -ENOMEM;
	}

	ndev->irq = irq_of_parse_and_map(np, 0);

	if (ndev->irq <= 0) {
		dev_err(priv->dev, "no IRQ found, irq(%d)\n", ndev->irq);
		return -EINVAL;
	}

	dev_dbg(priv->dev, "mac0(0x%lx), mac1(0x%lx), albany0(0x%lx), clkgen0(0x%lx), ",
		(unsigned long)priv->mac_bank_0, (unsigned long)priv->mac_bank_1,
		(unsigned long)priv->albany_bank_0, (unsigned long)priv->clkgen_bank_0);
	dev_dbg(priv->dev, "chip(0x%lx), clkgen0_pm(0x%lx), irq(%d)\n",
		(unsigned long)priv->chip_bank, (unsigned long)priv->clkgen_pm_bank_0, ndev->irq);
	dev_dbg(priv->dev, "tzpc_aid_0(0x%lx), pm_misc(0x%lx)\n",
		(unsigned long)priv->tzpc_aid_bank_0, (unsigned long)priv->pm_misc_bank);
	dev_dbg(priv->dev, "mpll(0x%lx)\n",
		(unsigned long)priv->mpll_bank);

	reg_ops = kmalloc(sizeof(*reg_ops), GFP_KERNEL);
	if (!reg_ops)
		return -ENOMEM;

	priv->reg_ops = reg_ops;

	return 0;
}

static void mtk_dtv_gmac_of_get_misc(struct net_device *ndev, struct device_node *np)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	const char *mac_addr;
	u32 get_val = 0;

	mac_addr = of_get_mac_address(np);
	if (mac_addr) {
		ether_addr_copy(mac_addr_dts, mac_addr);
		dev_dbg(priv->dev, "mac_dts(%pM)\n", mac_addr_dts);
	}

	/* hook register ops */
	if (of_property_read_u32(np, "phy-type", &get_val)) {
		priv->phy_type = PHY_TYPE_INTERNAL;
		dev_info(priv->dev, "gmac set phy_type to default value (%d)\n",
			 priv->phy_type);
	} else {
		if (get_val < 0) {
			dev_info(priv->dev, "gmac bad phy_type(%d), set it to default value\n",
				 get_val);
			priv->phy_type = PHY_TYPE_INTERNAL;
		} else {
			priv->phy_type = get_val;
			dev_dbg(priv->dev, "gmac phy_type (%d)\n",
				priv->phy_type);
		}
	}

	if (priv->phy_type == PHY_TYPE_INTERNAL) {
		dev_info(priv->dev, "use internal phy\n");
		priv->is_internal_phy = true;
		reg_ops->write_phy = mtk_dtv_gmac_phy_write_internal;
		reg_ops->read_phy = mtk_dtv_gmac_phy_read_internal;
	} else {
		dev_info(priv->dev, "use external phy\n");
		priv->is_internal_phy = false;
		reg_ops->write_phy = mtk_dtv_gmac_phy_write_external;
		reg_ops->read_phy = mtk_dtv_gmac_phy_read_external;
	}

	if (of_property_read_bool(np, "force-10m") == true) {
		dev_info(priv->dev, "force speed 10M\n");
		priv->is_force_10m = true;
	} else if (of_property_read_bool(np, "force-100m") == true) {
		dev_info(priv->dev, "force speed 100M\n");
		priv->is_force_100m = true;
	} else {
		priv->is_force_10m = false;
		priv->is_force_100m = false;
	}

	if (of_property_read_bool(np, "fpga-haps") == true) {
		dev_info(priv->dev, "run on fpga haps platform\n");
		priv->is_fpga_haps = true;
	} else {
		priv->is_fpga_haps = false;
	}

	/* allocate descriptor resource */
	if (of_property_read_u32(np, "rx-desc-size", &priv->rx_desc_size)) {
		priv->rx_desc_size = RX_DESC_SIZE_DEFAULT;
		dev_info(priv->dev, "set rx_desc_size to default value (%d)\n",
			 priv->rx_desc_size);
	} else {
		if (priv->rx_desc_size <= 0) {
			dev_info(priv->dev, "bad rx_desc_size(%d), set it to default value\n",
				 priv->rx_desc_size);
			priv->rx_desc_size = RX_DESC_SIZE_DEFAULT;
		}
	}

	if (of_property_read_u32(np, "rx-desc-num", &priv->rx_desc_num)) {
		priv->rx_desc_num = RX_DESC_NUM_DEFAULT;
		dev_info(priv->dev, "set rx_desc_num to default value (%d)\n",
			 priv->rx_desc_num);
	} else {
		if (priv->rx_desc_num <= 0) {
			dev_info(priv->dev, "bad rx_desc_num(%d), set it to default value\n",
				 priv->rx_desc_num);
			priv->rx_desc_num = RX_DESC_NUM_DEFAULT;
		}
	}

	if (of_property_read_u32(np, "ethpll-ictrl", &get_val)) {
		priv->ethpll_ictrl_type = ETHPLL_ICTRL_MT5896;
		dev_info(priv->dev, "set ethpll_ictrl_type to default value (%d)\n",
			 priv->ethpll_ictrl_type);
	} else {
		if (get_val < 0) {
			dev_info(priv->dev, "bad ethpll_ictrl_type(%d), set it to default value\n",
				 get_val);
			priv->ethpll_ictrl_type = ETHPLL_ICTRL_MT5896;
		} else {
			priv->ethpll_ictrl_type = get_val;
			dev_dbg(priv->dev, "ethpll_ictrl_type (%d)\n",
				priv->ethpll_ictrl_type);
		}
	}

	if (of_property_read_u32(np, "mac-type", &get_val)) {
		priv->mac_type = MAC_TYPE_GMAC;
		dev_info(priv->dev, "gmac set mac_type to default value (%d)\n",
			 priv->mac_type);
	} else {
		if (get_val < 0) {
			dev_info(priv->dev, "gmac bad mac_type(%d), set it to default value\n",
				 get_val);
			priv->mac_type = MAC_TYPE_GMAC;
		} else {
			priv->mac_type = get_val;
			dev_dbg(priv->dev, "gmac mac_type (%d)\n",
				priv->mac_type);
		}
	}

	if (of_property_read_u32(np, "phy-process", &get_val)) {
		priv->phy_process = PHY_PROCESS_7_12;
		dev_info(priv->dev, "set phy_process to default value (%d)\n",
			 priv->phy_process);
	} else {
		if (get_val < 0) {
			dev_info(priv->dev, "bad phy_process(%d), set it to default value\n",
				 get_val);
			priv->phy_process = PHY_PROCESS_7_12;
		} else {
			priv->phy_process = get_val;
			dev_dbg(priv->dev, "phy_process (%d)\n",
				priv->phy_process);
		}
	}

	switch (priv->phy_process) {
	case PHY_PROCESS_7_12:
		reg_ops->phy_hw_init = mtk_dtv_gmac_phy_hw_init_7_12;
		dev_info(priv->dev, "set phy init to 7_12\n");
		break;
	case PHY_PROCESS_22:
		reg_ops->phy_hw_init = mtk_dtv_gmac_phy_hw_init_22;
		dev_info(priv->dev, "set phy init to 22\n");
		break;
	default:
		reg_ops->phy_hw_init = mtk_dtv_gmac_phy_hw_init_7_12;
		dev_info(priv->dev, "unknown phy_process(%d), set phy init to default 7_12\n",
			 priv->phy_process);
		break;
	};
}

static int mtk_dtv_gmac_create_pool(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	priv->rx_desc_dma_pool = dma_pool_create("mtktv-mac-rx", priv->dev,
						 (priv->rx_desc_size * priv->rx_desc_num),
						 RX_DESC_PTR_ALIGNMENT, 0);
	if (!priv->rx_desc_dma_pool) {
		dev_err(priv->dev, "fail to rx dma_pool_create\n");
		return -ENOMEM;
	}

	/* tx memcpy */
	priv->tx_memcpy_dma_pool = dma_pool_create("mtktv-mac-tx", priv->dev,
						   (TX_MEMCPY_SIZE_DEFAULT * TX_MEMCPY_NUM_DEFAULT),
						   TX_MEMCPY_PTR_ALIGNMENT, 0);
	if (!priv->tx_memcpy_dma_pool) {
		dev_err(priv->dev, "fail to tx dma_pool_create\n");
		return -ENOMEM;
	}

	priv->rx_desc_vir_addr = (uintptr_t)dma_pool_alloc(priv->rx_desc_dma_pool,
							   GFP_KERNEL | GFP_DMA,
							   &priv->rx_desc_dma_addr);
	if (!priv->rx_desc_vir_addr) {
		dev_err(priv->dev, "fail to rx dma_poll_alloc\n");
		return -ENOMEM;
	}

	if (priv->rx_desc_dma_addr & (RX_DESC_PTR_ALIGNMENT - 1)) {
		dev_err(priv->dev, "rx dma_addr(0x%lx) not %d aligned\n",
			(unsigned long)priv->rx_desc_dma_addr, RX_DESC_PTR_ALIGNMENT);
		return -EINVAL;
	}

	dev_info(priv->dev, "rx desc_size(%d), num(%d), vir_addr(0x%lx), dma_addr(0x%lx)\n",
		 priv->rx_desc_size,
		 priv->rx_desc_num,
		 (unsigned long)priv->rx_desc_vir_addr,
		 (unsigned long)priv->rx_desc_dma_addr);

	/* tx memcpy */
	priv->tx_memcpy_vir_addr = (uintptr_t)dma_pool_alloc(priv->tx_memcpy_dma_pool,
							     GFP_KERNEL | GFP_DMA,
							     &priv->tx_memcpy_dma_addr);
	if (!priv->tx_memcpy_vir_addr) {
		dev_err(priv->dev, "fail to tx dma_poll_alloc\n");
		return -ENOMEM;
	}

	if (priv->tx_memcpy_dma_addr & (TX_MEMCPY_PTR_ALIGNMENT - 1)) {
		dev_err(priv->dev, "tx dma_addr(0x%lx) not %d aligned\n",
			(unsigned long)priv->tx_memcpy_dma_addr, TX_MEMCPY_PTR_ALIGNMENT);
		return -EINVAL;
	}

	dev_info(priv->dev, "tx memcpy_size(%d), num(%d), vir_addr(0x%lx), dma_addr(0x%lx)\n",
		 TX_MEMCPY_SIZE_DEFAULT,
		 TX_MEMCPY_NUM_DEFAULT,
		 (unsigned long)priv->tx_memcpy_vir_addr,
		 (unsigned long)priv->tx_memcpy_dma_addr);

	return 0;
}

static void mtk_dtv_gmac_release_resource(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	if (priv->mac_bank_0)
		iounmap(priv->mac_bank_0);
	if (priv->mac_bank_1)
		iounmap(priv->mac_bank_1);
	if (priv->albany_bank_0)
		iounmap(priv->albany_bank_0);
	if (priv->clkgen_bank_0)
		iounmap(priv->clkgen_bank_0);
	if (priv->chip_bank)
		iounmap(priv->chip_bank);
	if (priv->clkgen_pm_bank_0)
		iounmap(priv->clkgen_pm_bank_0);
	if (priv->tzpc_aid_bank_0)
		iounmap(priv->tzpc_aid_bank_0);
	if (priv->pm_misc_bank)
		iounmap(priv->pm_misc_bank);
	if (priv->mpll_bank)
		iounmap(priv->mpll_bank);
}

static int mtk_dtv_gmac_probe(struct platform_device *pdev)
{
	struct mtk_dtv_gmac_private *priv = NULL;
	int ret = 0;
	struct device_node *np = NULL;
	struct net_device *ndev = NULL;

	ndev = alloc_etherdev(sizeof(struct mtk_dtv_gmac_private));
	if (!ndev)
		return -ENOMEM;

	/* new chips support to 8G */
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

#ifdef CONFIG_ARM64
	arch_setup_dma_ops(&pdev->dev, 0, 0, NULL, 0);
#endif

	np = pdev->dev.of_node;
	if (!np) {
		dev_err(&pdev->dev, "fail to find node\n");
		ret = -EINVAL;
		goto err_free_netdev;
	}

	ndev->netdev_ops = &mtk_dtv_gmac_netdev_ops;
	ndev->ethtool_ops = &mtk_dtv_gmac_ethtool_ops;

	SET_NETDEV_DEV(ndev, &pdev->dev);

	priv = netdev_priv(ndev);
	priv->ndev = ndev;
	priv->pdev = pdev;
	priv->dev = &pdev->dev;

	/* phy restart patch */
	priv->phy_restart_cnt = 0;

	ret = mtk_dtv_gmac_of_get_bank(ndev, np);
	if (ret)
		goto err_iounmap;

	mtk_dtv_gmac_of_get_misc(ndev, np);

	ret = mtk_dtv_gmac_create_pool(ndev);
	if (ret)
		goto err_free_dma;

	/* init */
	skb_queue_head_init(&priv->rx_skb_q);

	//mtk_dtv_gmac_mem_init(ndev);

	/* tx memcpy */
	//priv->tx_memcpy_index = 0;

	ret = mtk_dtv_gmac_hw_init(ndev, false);
	if (ret)
		goto err_free_dma;

	/* register irq */
	if (request_irq(ndev->irq, mtk_dtv_gmac_isr, /*SA_INTERRUPT*/IRQF_ONESHOT,
			ndev->name, ndev) != 0) {
		dev_err(&pdev->dev, "request irq %d fail\n", ndev->irq);
		ret = -ENODEV;
		goto err_free_dma;
	}

	/* for ethtool */
	priv->mii.dev = ndev;
	priv->mii.mdio_read = mtk_dtv_gmac_mdio_read;
	priv->mii.mdio_write = mtk_dtv_gmac_mdio_write;
	priv->mii.phy_id = priv->phy_addr;

	/* register napi */
	netif_napi_add(ndev, &priv->napi_rx, mtk_dtv_gmac_napi_poll_rx, RX_NAPI_WEIGHT);

	/* register sysfs */
	ret = mtk_dtv_gmac_sysfs_create(ndev);
	if (ret)
		dev_warn(&pdev->dev, "create sysfs failed, but it's fine\n");

	ret = register_netdev(ndev);
	if (ret)
		goto err_free_dma;

	/* timer */
	timer_setup(&priv->link_timer, mtk_dtv_gmac_link_timer_callback, 0);
	priv->link_timer.expires = jiffies + (MAC_UPDATE_LINK_TIME / 20);

	/* lock */
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->magic_lock);

	/* slt */
	priv->is_slt_test_running = false;
	priv->is_slt_test_passed = true;

	/* woc */
	priv->is_woc = false;
	priv->woc_filter_index = 0;
	memset(priv->woc_sram_pattern, 0, sizeof(priv->woc_sram_pattern));

	/* wol */
	priv->is_wol = false;

	/* eth driver stage */
	priv->eth_drv_stage = 0;

	/* rx error irq state */
	priv->is_rx_rbna_irq_masked = false;
	priv->is_rx_rovr_irq_masked = false;

	platform_set_drvdata(pdev, ndev);

	return 0;

err_free_dma:
	mtk_dtv_gmac_sysfs_remove(ndev);

	if (priv->rx_desc_vir_addr)
		dma_pool_free(priv->rx_desc_dma_pool,
			      (void *)priv->rx_desc_vir_addr,
			      priv->rx_desc_dma_addr);

	dma_pool_destroy(priv->rx_desc_dma_pool);

	/* tx memcpy */
	if (priv->tx_memcpy_vir_addr)
		dma_pool_free(priv->tx_memcpy_dma_pool,
			      (void *)priv->tx_memcpy_vir_addr,
			      priv->tx_memcpy_dma_addr);

	dma_pool_destroy(priv->tx_memcpy_dma_pool);

	skb_queue_purge(&priv->rx_skb_q);

	kfree(priv->reg_ops);

err_iounmap:
	mtk_dtv_gmac_release_resource(ndev);

err_free_netdev:
	free_netdev(ndev);
	return ret;
}

static int mtk_dtv_gmac_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	unregister_netdev(ndev);

	mtk_dtv_gmac_sysfs_remove(ndev);

	if (priv->rx_desc_vir_addr)
		dma_pool_free(priv->rx_desc_dma_pool,
			      (void *)priv->rx_desc_vir_addr,
			      priv->rx_desc_dma_addr);

	dma_pool_destroy(priv->rx_desc_dma_pool);

	/* tx memcpy */
	if (priv->tx_memcpy_vir_addr)
		dma_pool_free(priv->tx_memcpy_dma_pool,
			      (void *)priv->tx_memcpy_vir_addr,
			      priv->tx_memcpy_dma_addr);

	dma_pool_destroy(priv->tx_memcpy_dma_pool);

	skb_queue_purge(&priv->rx_skb_q);

	kfree(priv->reg_ops);

	if (priv->mac_bank_0)
		iounmap(priv->mac_bank_0);
	if (priv->mac_bank_1)
		iounmap(priv->mac_bank_1);
	if (priv->albany_bank_0)
		iounmap(priv->albany_bank_0);
	if (priv->clkgen_bank_0)
		iounmap(priv->clkgen_bank_0);
	if (priv->chip_bank)
		iounmap(priv->chip_bank);

	free_netdev(ndev);

	return 0;
}

static const struct of_device_id mtk_dtv_gmac_match[] = {
	{ .compatible = MTK_GMAC_COMPAT_NAME },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_dtv_gmac_match);

static struct platform_driver mtk_dtv_gmac_driver = {
	.probe      = mtk_dtv_gmac_probe,
	.remove     = mtk_dtv_gmac_remove,
	.suspend    = mtk_dtv_gmac_suspend,
	.resume     = mtk_dtv_gmac_resume,
	.driver = {
		.name   = "mt5896-gmac",
		.of_match_table = mtk_dtv_gmac_match,
		.owner  = THIS_MODULE,
	}
};

static int __init mtk_dtv_gmac_init(void)
{
	int err;

	err = platform_driver_register(&mtk_dtv_gmac_driver);
	if (err)
		return err;

	return 0;
}

static void __exit mtk_dtv_gmac_exit(void)
{
	platform_driver_unregister(&mtk_dtv_gmac_driver);
}

module_init(mtk_dtv_gmac_init);
module_exit(mtk_dtv_gmac_exit);

MODULE_DESCRIPTION("MediaTek DTV SoC based GMAC Driver");
MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");
MODULE_LICENSE("GPL");
