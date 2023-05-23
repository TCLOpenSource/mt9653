// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * MediaTek USB PHY Driver for DTV
 *
 * Copyright (c) 2021 MediaTek Inc.
 * Author:
 *  Yun-Chien Yu <yun-chien.yu@mediatek.com>
 */

/* RG PHY MISC */
/* I2C mode control register
 * U2 port shares enable bit
 */
#define U2_PHY_I2C_MODE_EN		BIT(0)
#define U3_DIG_PHY_I2C_MODE_EN_BASE	(1)

/* RG U2 MISC */
#define U3P_MISC_REG0			0x000
#define PA0_RG_U2_PLL_CTRL		GENMASK(23, 16)
#define PA0_RG_U2_PLL_CTRL_VAL(x)	((0xff & (x)) << 16)

/* SPHY/TPHY 2.5G mode setting.
 * because current tphy driver is super old,
 * the RGs defined here may not matches to this driver.
 * to make driver clean for upstream patch,
 * define new RGs with SPHY/TPHY prefixed separately.
 */

/* SPHY 2.5G RG U3 PHYD */
#define SPHY_U3P_U3_PHYD_MIX3	0x038
#define SPHY_P3D_RG_PLL_SSCEN	BIT(14)

#define SPHY_U3P_U3_PHYD_CDR2	0x0ec
#define SPHY_P3D_RG_CDR_PPATH_DVN_LTD1		GENMASK(16, 12)
#define SPHY_P3D_RG_CDR_PPATH_DVN_LTD1_VAL(x)	((0x1f & (x)) << 12)
#define SPHY_P3D_RG_CDR_PPATH_DVN_LTD0		GENMASK(10, 6)
#define SPHY_P3D_RG_CDR_PPATH_DVN_LTD0_VAL(x)	((0x1f & (x)) << 6)
#define SPHY_P3D_RG_CDR_PPATH_DVN_LTR		GENMASK(4, 0)
#define SPHY_P3D_RG_CDR_PPATH_DVN_LTR_VAL(x)	(0x1f & (x))

/* SPHY 2.5G RG U3 PHYA */
#define SPHY_U3P_U3_PHYA_REG5	0x014
#define SPHY_P3A_RG_PLL_SSC_PRD		GENMASK(15, 0)
#define SPHY_P3A_RG_PLL_SSC_PRD_VAL(x)	(0xffff & (x))

/* SPHY 2.5G RG U3 PHYA_DA */
#define SPHY_U3P_U3_PHYA_DA_REG0	U3P_U3_PHYA_DA_REG0
#define SPHY_P3A_RG_PLL_FRA_EN_U3	BIT(25)

#define SPHY_U3P_U3_PHYA_DA_REG20	0x120
#define SPHY_P3A_RG_PLL_PREDIV_U3		GENMASK(9, 8)
#define SPHY_P3A_RG_PLL_PREDIV_U3_VAL(x)	((0x3 & (x)) << 8)

#define SPHY_U3P_U3_PHYA_DA_REG24	0x124
#define SPHY_P3A_RG_PLL_FBKDIV_U3		GENMASK(31, 0)
#define SPHY_P3A_RG_PLL_FBKDIV_U3_VAL(x)	(0xffffffff & (x))

#define SPHY_U3P_U3_PHYA_DA_REG38	0x138
#define SPHY_P3A_RG_PLL_SSC_DELTA1_U3		GENMASK(15, 0)
#define SPHY_P3A_RG_PLL_SSC_DELTA1_U3_VAL(x)	(0xffff & (x))

#define SPHY_U3P_U3_PHYA_DA_REG40	0x140
#define SPHY_P3A_RG_PLL_SSC_DELTA_U3		GENMASK(31, 16)
#define SPHY_P3A_RG_PLL_SSC_DELTA_U3_VAL(x)	((0xffff & (x)) << 16)

#define SPHY_U3P_U3_PHYA_DA_REG78	0x178
#define SPHY_P3A_RG_PLL_FBKDIV_U3_B32	BIT(10)

#define SPHY_U3P_U3_PHYA_DA_REG88	0x188
#define SPHY_P3A_CDR_IIR_CORNER_U3		GENMASK(22, 20)
#define SPHY_P3A_CDR_IIR_CORNER_U3_VAL(x)	((0x7 & (x)) << 20)
#define SPHY_P3A_RG_CDR_IIR_GAIN_U3		GENMASK(3, 0)
#define SPHY_P3A_RG_CDR_IIR_GAIN_U3_VAL(x)	(0xf & (x))

/* TPHY 2.5G RG U3 PHYA */
#define TPHY_U3P_U3_PHYA_REG4	0x010
#define TPHY_P3A_RG_SSUSB_PLL_POSDIV		GENMASK(16, 15)
#define TPHY_P3A_RG_SSUSB_PLL_POSDIV_VAL(x)	((0x3 & (x)) << 15)

/* TPHY 2.5G RG U3 PHYA_DA */
#define TPHY_U3P_U3_PHYA_DA_REG0	0x100
#define TPHY_P3A_RG_PCIE_SPEED_U3	BIT(20)
#define TPHY_P3A_RG_PCIE_SPEED_PE1H	BIT(21)
#define TPHY_P3A_RG_PCIE_SPEED_PE1D	BIT(22)

/* New 22 nm APHY bank */
#define MPHY_ATOP_AWRAP		0x200

#define MPHY_U3P_U3_PHYA_TOP0A	(0x0a * 4)
#define MPHY_U3P_U3_PHYA_TOP16	(0x16 * 4)
#define MPHY_U3P_U3_PHYA_TOP33	(0x33 * 4)
#define MPHY_U3P_U3_PHYA_TOP4B	(0x4b * 4)

#define MPHY_U3P_U3_PHYA_TOP6A	(0x6a * 4)
#define MPHY_U3P_REG_U3_2P5G_01A8	BIT(1)
#define MPHY_U3P_REG_MCU_U3APHY_POR_CTRL_01A8	BIT(4)

#define MPHY_U3P_U3_PHYA_TOP71	(0x71 * 4)
#define MPHY_U3P_U3_PHYA_TOP73	(0x73 * 4)
#define MPHY_U3P_U3_PHYA_TOP76	(0x76 * 4)
#define MPHY_U3P_U3_PHYA_WRAP0B	(0x0b * 4)
#define MPHY_U3P_U3_PHYA_WRAP0C	(0x0c * 4)
#define MPHY_U3P_U3_PHYA_WRAP0D	(0x0d * 4)
#define MPHY_U3P_U3_PHYA_WRAP40	(0x40 * 4)
#define MPHY_U3P_U3_PHYA_WRAP5C	(0x5c * 4)
#define MPHY_U3P_U3_PHYA_WRAP5D	(0x5d * 4)
#define MPHY_U3P_U3_PHYA_WRAP5F	(0x5f * 4)
#define MPHY_U3P_U3_PHYA_WRAP60	(0x60 * 4)
#define MPHY_U3P_U3_PHYA_WRAP66	(0x66 * 4)
#define MPHY_U3P_U3_PHYA_WRAP67	(0x67 * 4)
#define MPHY_U3P_U3_PHYA_WRAP7F	(0x7f * 4)

/* X32_U3_PHY */
#define U3P_SSPXTP_DIG_LN_DAIF_REG0	0x3700
#define PA0_RG_SSPXTP0_DAIF_FRC_LN_TX_LCTXCM1_VAL	BIT(7)

/* Message buffer size */
#define PHY_MSG_MAX	100

void mtk_misc_i2c_init(struct mtk_tphy *tphy, struct mtk_phy_instance *instance)
{
	u32 tmp;

	if (instance->type == PHY_TYPE_USB2) {
		void __iomem *i2c_ctrl_reg;
		bool is_i2c_dual_mode = false;

		/* disable i2c mode */
		if (tphy->t_misc.i2c_ctrl_reg_u2 != NULL)
			is_i2c_dual_mode = true;

		dev_info(tphy->dev, "disable i2c mode for u2 port %s\n",
					is_i2c_dual_mode ? "dual" : "");
		i2c_ctrl_reg = is_i2c_dual_mode ? tphy->t_misc.i2c_ctrl_reg_u2 :
						tphy->t_misc.i2c_ctrl_reg;
		tmp = readl(i2c_ctrl_reg);
		tmp &= ~U2_PHY_I2C_MODE_EN;
		writel(tmp, i2c_ctrl_reg);

	} else if (instance->type == PHY_TYPE_USB3) {
		/* disable i2c mode */
		dev_info(tphy->dev,
			"disable i2c mode for u3 p%d\n", tphy->t_misc.nphys_u3);
		tmp = readl(tphy->t_misc.i2c_ctrl_reg);
		tmp &= ~(0x1 << (U3_DIG_PHY_I2C_MODE_EN_BASE +
						tphy->t_misc.nphys_u3));
		writel(tmp, tphy->t_misc.i2c_ctrl_reg);
		tphy->t_misc.nphys_u3++;
	}
}

/*
 * For force PM U3 PHY degradation, enable this flag.
 * To enable, pass boot argument phy_mtk_tphy_tv.pm_degrd=1
 */
static bool pm_degrad;
module_param(pm_degrad, bool, 0440);
MODULE_PARM_DESC(pm_degrad, "Force PM U3 PHY degradation (1=enable)");

void mtk_misc_parse_property(struct mtk_tphy *tphy,
			struct mtk_phy_instance *instance)
{
	struct device *dev = &instance->phy->dev;

	if (instance->type == PHY_TYPE_USB3) {
		instance->i_misc.degradation_en =
			device_property_read_bool(dev, "degradation-enable");
		dev_info(dev, "degradation-enable:%d\n",
				instance->i_misc.degradation_en);

		if (tphy->t_misc.standbypower)
			instance->i_misc.degradation_en |= pm_degrad;

		device_property_read_u32(dev,
			"degradation-mode", &instance->i_misc.degradation_mode);
		if (instance->i_misc.degradation_en) {
			dev_info(dev, "u3 2.5G mode:%d, pm_degrad:%d\n",
				instance->i_misc.degradation_mode,
				pm_degrad);
		}
	}
}

void mtk_misc_u2_instance_init_and_set(struct mtk_tphy *tphy,
				struct mtk_phy_instance *instance)
{
	struct device *dev = &instance->phy->dev;
	struct u2phy_banks *u2_banks = &instance->u2_banks;
	void __iomem *com = u2_banks->com;
	u32 tmp;
	char buf[PHY_MSG_MAX];
	char *str = buf;

	if (tphy->t_misc.mixed_mode) {
		if (instance->index == 0) {
			tmp = readl(u2_banks->misc + U3P_MISC_REG0);
			tmp &= ~(PA0_RG_U2_PLL_CTRL);
			tmp |= PA0_RG_U2_PLL_CTRL_VAL(0x64);
			writel(tmp, u2_banks->misc + U3P_MISC_REG0);
		}
	}

	if (device_property_read_u32(dev, "mediatek,eye-ls-sr",
			&instance->i_misc.eye_ls_sr) == 0) {
		tmp = readl(com + U3P_USBPHYACR4);
		tmp &= ~PA4_RG_USB20_LS_SR;
		tmp |= PA4_RG_USB20_LS_SR_VAL(instance->i_misc.eye_ls_sr);
		writel(tmp, com + U3P_USBPHYACR4);
		str += snprintf(str, PHY_MSG_MAX - (str - buf), "ls-sr:%x ",
			instance->i_misc.eye_ls_sr);
	}

	if (device_property_read_u32(dev, "mediatek,eye-ls-cr",
			&instance->i_misc.eye_ls_cr) == 0) {
		tmp = readl(com + U3P_USBPHYACR4);
		tmp &= ~PA4_RG_USB20_LS_CR;
		tmp |= PA4_RG_USB20_LS_CR_VAL(instance->i_misc.eye_ls_cr);
		writel(tmp, com + U3P_USBPHYACR4);
		str += snprintf(str, PHY_MSG_MAX - (str - buf), "ls-cr:%x ",
			instance->i_misc.eye_ls_cr);
	}

	if (device_property_read_u32(dev, "mediatek,eye-pre-em",
			&instance->i_misc.eye_pre_em) == 0) {
		tmp = readl(com + U3P_USBPHYACR6);
		tmp &= ~PA6_RG_U2_PRE_EMPHASIS;
		tmp |= PA6_RG_U2_PRE_EMPHASIS_VAL(instance->i_misc.eye_pre_em);
		writel(tmp, com + U3P_USBPHYACR6);
		str += snprintf(str, PHY_MSG_MAX - (str - buf), "pre-em:%x ",
			instance->i_misc.eye_pre_em);
	}

	if (device_property_read_u32(dev, "mediatek,eye-clkref-rev",
			&instance->i_misc.eye_clkref_rev) == 0) {
		tmp = readl(com + U3P_USBPHYACR2);
		tmp &= ~PA2_RG_USB20_CLKREF_REV;
		tmp |= PA2_RG_USB20_CLKREF_REV_VAL(instance->i_misc.eye_clkref_rev);
		writel(tmp, com + U3P_USBPHYACR2);
		str += snprintf(str, PHY_MSG_MAX - (str - buf), "clkref:%x ",
			instance->i_misc.eye_clkref_rev);
	}

	if (device_property_read_u32(dev, "mediatek,eye-clkref-rev1",
			&instance->i_misc.eye_clkref_rev1) == 0) {
		tmp = readl(com + U3P_USBPHYACR2);
		tmp &= ~PA2_RG_USB20_CLKREF_REV1;
		tmp |= PA2_RG_USB20_CLKREF_REV1_VAL(instance->i_misc.eye_clkref_rev1);
		writel(tmp, com + U3P_USBPHYACR2);
		str += snprintf(str, PHY_MSG_MAX - (str - buf), "clkref1:%x ",
			instance->i_misc.eye_clkref_rev1);
	}

	if (device_property_read_u32(dev, "mediatek,usb20-sqth",
			&instance->i_misc.u2_sqth) == 0) {
		tmp = readl(com + U3P_USBPHYACR6);
		tmp &= ~PA6_RG_U2_SQTH;
		tmp |= PA6_RG_U2_SQTH_VAL(instance->i_misc.u2_sqth);
		writel(tmp, com + U3P_USBPHYACR6);
		str += snprintf(str, PHY_MSG_MAX - (str - buf), "sqth:%x ",
			instance->i_misc.u2_sqth);
	}

	if (device_property_read_u32(dev, "mediatek,usb20-discth",
			&instance->i_misc.u2_discth) == 0) {
		tmp = readl(com + U3P_USBPHYACR6);
		tmp &= ~PA6_RG_U2_DISCTH;
		tmp |= PA6_RG_U2_DISCTH_VAL(instance->i_misc.u2_discth);
		writel(tmp, com + U3P_USBPHYACR6);
		str += snprintf(str, PHY_MSG_MAX - (str - buf), "discth:%x ",
			instance->i_misc.u2_discth);
	}

  	dev_info(dev, "%s\n", buf);
}

void sphy_u3_degrade(struct mtk_tphy *tphy,
	struct mtk_phy_instance *instance)
{
	struct u3phy_banks *u3_banks = &instance->u3_banks;
	u32 tmp;

	dev_info(&instance->phy->dev, "u3 2.5G mode: %s\n", __func__);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG20);
	tmp &= ~SPHY_P3A_RG_PLL_PREDIV_U3;
	tmp |= SPHY_P3A_RG_PLL_PREDIV_U3_VAL(0);
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG20);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG0);
	tmp |= SPHY_P3A_RG_PLL_FRA_EN_U3;
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG0);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG24);
	tmp &= ~SPHY_P3A_RG_PLL_FBKDIV_U3;
	tmp |= SPHY_P3A_RG_PLL_FBKDIV_U3_VAL(0x682aaaaa);
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG24);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG78);
	tmp &= ~SPHY_P3A_RG_PLL_FBKDIV_U3_B32;
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG78);

	tmp = readl(u3_banks->phyd + SPHY_U3P_U3_PHYD_MIX3);
	tmp |= SPHY_P3D_RG_PLL_SSCEN;
	writel(tmp, u3_banks->phyd + SPHY_U3P_U3_PHYD_MIX3);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_REG5);
	tmp &= ~SPHY_P3A_RG_PLL_SSC_PRD;
	tmp |= SPHY_P3A_RG_PLL_SSC_PRD_VAL(0x0180);
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_REG5);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG38);
	tmp &= ~SPHY_P3A_RG_PLL_SSC_DELTA1_U3;
	tmp |= SPHY_P3A_RG_PLL_SSC_DELTA1_U3_VAL(0x0140);
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG38);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG40);
	tmp &= ~SPHY_P3A_RG_PLL_SSC_DELTA_U3;
	tmp |= SPHY_P3A_RG_PLL_SSC_DELTA_U3_VAL(0x0137);
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG40);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG88);
	tmp &= ~SPHY_P3A_RG_CDR_IIR_GAIN_U3;
	tmp |= SPHY_P3A_RG_CDR_IIR_GAIN_U3_VAL(0x02);
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG88);

	tmp = readl(u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG88);
	tmp &= ~SPHY_P3A_CDR_IIR_CORNER_U3;
	tmp |= SPHY_P3A_CDR_IIR_CORNER_U3_VAL(0x02);
	writel(tmp, u3_banks->phya + SPHY_U3P_U3_PHYA_DA_REG88);

	tmp = readl(u3_banks->phyd + SPHY_U3P_U3_PHYD_CDR2);
	tmp &= ~SPHY_P3D_RG_CDR_PPATH_DVN_LTR;
	tmp |= SPHY_P3D_RG_CDR_PPATH_DVN_LTR_VAL(0xa);
	writel(tmp, u3_banks->phyd + SPHY_U3P_U3_PHYD_CDR2);

	tmp = readl(u3_banks->phyd + SPHY_U3P_U3_PHYD_CDR2);
	tmp &= ~SPHY_P3D_RG_CDR_PPATH_DVN_LTD0;
	tmp |= SPHY_P3D_RG_CDR_PPATH_DVN_LTD0_VAL(0x3);
	writel(tmp, u3_banks->phyd + SPHY_U3P_U3_PHYD_CDR2);

	tmp = readl(u3_banks->phyd + SPHY_U3P_U3_PHYD_CDR2);
	tmp &= ~SPHY_P3D_RG_CDR_PPATH_DVN_LTD1;
	tmp |= SPHY_P3D_RG_CDR_PPATH_DVN_LTD1_VAL(0xa);
	writel(tmp, u3_banks->phyd + SPHY_U3P_U3_PHYD_CDR2);
}

void tphy_u3_degrade(struct mtk_tphy *tphy,
	struct mtk_phy_instance *instance)
{
	struct u3phy_banks *u3_banks = &instance->u3_banks;
	u32 tmp;

	dev_info(&instance->phy->dev, "u3 2.5G mode: %s\n", __func__);

	tmp = readl(u3_banks->phya + TPHY_U3P_U3_PHYA_REG4);
	tmp &= ~TPHY_P3A_RG_SSUSB_PLL_POSDIV;
	tmp |= TPHY_P3A_RG_SSUSB_PLL_POSDIV_VAL(0x01);
	writel(tmp, u3_banks->phya + TPHY_U3P_U3_PHYA_REG4);

	tmp = readl(u3_banks->phya + TPHY_U3P_U3_PHYA_DA_REG0);
	/* rg_tphy_force_speed = 1'b1 */
	tmp |= TPHY_P3A_RG_PCIE_SPEED_U3;
	/* rg_tphy_speed = {RG_PCIE_SPEED_PE1H, RG_PCIE_SPEED_PE1D} = 2'b01 */
	tmp &= ~TPHY_P3A_RG_PCIE_SPEED_PE1H;
	tmp |= TPHY_P3A_RG_PCIE_SPEED_PE1D;
	writel(tmp, u3_banks->phya + TPHY_U3P_U3_PHYA_DA_REG0);
}

static void mphy_u3_degrade(struct mtk_tphy *tphy,
	struct mtk_phy_instance *instance)
{
	struct u3mphy_banks *u3_mphy = &instance->i_misc.u3_mphy;
	void __iomem *atop = u3_mphy->atop;
	void __iomem *awrap = u3_mphy->awrap;
	u32 tmp;

	dev_info(&instance->phy->dev, "u3 2.5G mode: %s\n", __func__);

	tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP16);
	tmp &= ~BIT(5);
	tmp |= BIT(6) | BIT(4);
	writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP16);

	tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP6A);
	tmp |= MPHY_U3P_REG_U3_2P5G_01A8;
	writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP6A);

	tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP7F);
	tmp |= BIT(0);
	writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP7F);

	tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP5D);
	tmp &= ~0xff00;
	tmp |= 0x1a00;
	writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP5D);
}

void mtk_misc_u3_instance_init(struct mtk_tphy *tphy,
				struct mtk_phy_instance *instance)
{
	u32 tmp;

	if (tphy->t_misc.mixed_mode) {
		struct u3mphy_banks *u3_mphy = &instance->i_misc.u3_mphy;
		void __iomem *upll = u3_mphy->upll;
		void __iomem *atop = u3_mphy->atop;
		void __iomem *awrap = u3_mphy->awrap;
		void __iomem *u3phy = u3_mphy->u3phy;

		/* SW patch for clock synthesis initial issue */
		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP0B);
		tmp &= ~GENMASK(14, 0);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP0B);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP0C);
		tmp &= ~GENMASK(14, 0);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP0C);

		tmp = readl(upll);
		tmp &= ~BIT(1);
		writel(tmp, upll);

		tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP33);
		tmp |= BIT(11);
		writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP33);

		tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP4B);
		tmp &= ~BIT(4);
		writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP4B);
		udelay(80);
		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP0B);
		tmp |= 0x7002;
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP0B);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP0C);
		tmp |= 0x04ec;
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP0C);
		udelay(20);
		tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP33);
		tmp &= ~BIT(11);
		writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP33);

		/* MIXED PHY */
		tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP16);
		tmp &= ~BIT(5);
		tmp |= BIT(4);
		writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP16);

		tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP71);
		tmp &= ~BIT(11);
		writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP71);

		tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP73);
		tmp &= ~BIT(11);
		writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP73);

		tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP76);
		tmp &= ~BIT(5);
		tmp |= BIT(4);
		writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP76);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP0D);
		tmp |= BIT(2);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP0D);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP40);
		tmp |= GENMASK(8, 7);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP40);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP67);
		tmp &= ~BIT(6);
		tmp |= BIT(5);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP67);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP5C);
		tmp &= ~BIT(13);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP5C);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP5F);
		tmp &= ~GENMASK(10, 0);
		tmp |= BIT(8);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP5F);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP60);
		tmp &= ~GENMASK(4, 2);
		tmp |= GENMASK(7, 5);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP60);

		tmp = readl(awrap + MPHY_U3P_U3_PHYA_WRAP66);
		tmp &= ~GENMASK(4, 0);
		tmp |= BIT(2);
		writel(tmp, awrap + MPHY_U3P_U3_PHYA_WRAP66);

		tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP0A);
		tmp &= ~GENMASK(15, 0);
		tmp |= 0x0400;
		writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP0A);

		/* overwrite x32 u3 phy */
		if (u3phy) {
			tmp = readl(u3phy + U3P_SSPXTP_DIG_LN_DAIF_REG0);
			tmp |= PA0_RG_SSPXTP0_DAIF_FRC_LN_TX_LCTXCM1_VAL;
			writel(tmp, u3phy + U3P_SSPXTP_DIG_LN_DAIF_REG0);
		}
	}

	if (instance->i_misc.degradation_en) {
		switch (instance->i_misc.degradation_mode) {
		case 0:
			sphy_u3_degrade(tphy, instance);
			break;
		case 1:
			tphy_u3_degrade(tphy, instance);
			break;
		case 2:
			mphy_u3_degrade(tphy, instance);
			break;
		default:
			dev_err(tphy->dev,
				"error! degradation (%d) un-matched\n",
				instance->i_misc.degradation_mode);
		}
	}
}

void mtk_misc_v2_banks_init(struct mtk_tphy *tphy,
			struct mtk_phy_instance *instance)
{
	switch (instance->type) {
	case PHY_TYPE_USB2:
		break;
	case PHY_TYPE_USB3:
	case PHY_TYPE_PCIE:
		if (tphy->t_misc.mixed_mode) {
			struct u3mphy_banks *u3_mphy;

			u3_mphy = &instance->i_misc.u3_mphy;
			u3_mphy->upll = instance->i_misc.base_upll;
			u3_mphy->atop = instance->i_misc.base_mphy;
			u3_mphy->awrap = instance->i_misc.base_mphy +
					MPHY_ATOP_AWRAP;
			u3_mphy->u3phy = instance->i_misc.base_u3phy;
		}
		break;
	default:
		dev_err(tphy->dev, "[%s] incompatible PHY type\n", __func__);
		return;
	}
}

int mtk_misc_instance_get_res(struct platform_device *pdev,
				struct device_node *node,
				struct mtk_phy_instance *instance,
				struct phy *phy)
{
	struct device *dev = &pdev->dev;
	struct resource res;
	int retval;

	retval = of_address_to_resource(node, 1, &res);
	if (retval) {
		instance->i_misc.base_mphy = NULL;
		instance->i_misc.base_upll = NULL;
		instance->i_misc.base_u3phy = NULL;
		return 0;
	}
	instance->i_misc.base_mphy = devm_ioremap_resource(&phy->dev, &res);
	if (IS_ERR(instance->i_misc.base_mphy)) {
		dev_err(dev, "failed to remap mphy regs\n");
		retval = PTR_ERR(instance->i_misc.base_mphy);
		return retval;
	}

	retval = of_address_to_resource(node, 2, &res);
	if (retval) {
		instance->i_misc.base_upll = NULL;
		instance->i_misc.base_u3phy = NULL;
		return 0;
	}
	instance->i_misc.base_upll = devm_ioremap_resource(&phy->dev, &res);
	if (IS_ERR(instance->i_misc.base_upll)) {
		dev_err(dev, "failed to remap upll regs\n");
		retval = PTR_ERR(instance->i_misc.base_upll);
		return retval;
	}

	retval = of_address_to_resource(node, 3, &res);
	if (retval) {
		instance->i_misc.base_u3phy = NULL;
		return 0;
	}
	instance->i_misc.base_u3phy = devm_ioremap_resource(&phy->dev, &res);
	if (IS_ERR(instance->i_misc.base_u3phy)) {
		dev_err(dev, "failed to remap x32_u3_phy regs\n");
		retval = PTR_ERR(instance->i_misc.base_u3phy);
		return retval;
	}

	return 0;
}

int mtk_misc_get_res(struct platform_device *pdev,
				struct mtk_tphy *tphy)
{
	struct device *dev = &pdev->dev;
	struct resource *i2c_ctrl_res;
	int retval = 0;

	tphy->t_misc.standbypower =
		device_property_read_bool(dev, "standbypower-domain");

	/* register to control phy intf, default is i2c mode */
	i2c_ctrl_res = platform_get_resource_byname(
				pdev, IORESOURCE_MEM, "i2c_ctrl_reg");
	tphy->t_misc.i2c_ctrl_reg = devm_ioremap_resource(dev, i2c_ctrl_res);
	if (IS_ERR(tphy->t_misc.i2c_ctrl_reg)) {
		dev_err(dev, "failed to remap i2c_ctrl reg\n");
		return PTR_ERR(tphy->t_misc.i2c_ctrl_reg);
	}

	tphy->t_misc.mixed_mode =
		device_property_read_bool(dev, "mediatek,phy_mixed_mode");
	i2c_ctrl_res = platform_get_resource_byname(
				pdev, IORESOURCE_MEM, "i2c_ctrl_reg_u2");
	if (i2c_ctrl_res == NULL) {
		tphy->t_misc.i2c_ctrl_reg_u2 = NULL;
	} else {
		tphy->t_misc.i2c_ctrl_reg_u2 =
				devm_ioremap_resource(dev, i2c_ctrl_res);
		if (IS_ERR(tphy->t_misc.i2c_ctrl_reg_u2)) {
			dev_err(dev, "failed to remap i2c_ctrl reg u2\n");
			return PTR_ERR(tphy->t_misc.i2c_ctrl_reg_u2);
		}
	}
	return retval;
}

int mtk_misc_phy_power_on(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_tphy *tphy = dev_get_drvdata(phy->dev.parent);
	struct u3mphy_banks *u3_mphy = &instance->i_misc.u3_mphy;
	void __iomem *atop = u3_mphy->atop;
	u32 tmp;

	dev_info(&phy->dev, "%s[%d]:type %d, mix %d\n", __func__, instance->index,
		instance->type, tphy->t_misc.mixed_mode);
	if (instance->type == PHY_TYPE_USB3) {
		if (tphy->t_misc.mixed_mode) {
			/* enable power state control */
			tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP6A);
			tmp &= ~MPHY_U3P_REG_MCU_U3APHY_POR_CTRL_01A8;
			writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP6A);
		}
	}

	return 0;
}

int mtk_misc_phy_power_off(struct phy *phy)
{
	struct mtk_phy_instance *instance = phy_get_drvdata(phy);
	struct mtk_tphy *tphy = dev_get_drvdata(phy->dev.parent);
	struct u3mphy_banks *u3_mphy = &instance->i_misc.u3_mphy;
	void __iomem *atop = u3_mphy->atop;
	u32 tmp;

	dev_info(&phy->dev, "%s[%d]:type %d, mix %d\n", __func__, instance->index,
		instance->type, tphy->t_misc.mixed_mode);
	if (instance->type == PHY_TYPE_USB3) {
		if (tphy->t_misc.mixed_mode) {
			/* disable power state control */
			tmp = readl(atop + MPHY_U3P_U3_PHYA_TOP6A);
			tmp |= MPHY_U3P_REG_MCU_U3APHY_POR_CTRL_01A8;
			writel(tmp, atop + MPHY_U3P_U3_PHYA_TOP6A);
		}
	}

	return 0;
}
