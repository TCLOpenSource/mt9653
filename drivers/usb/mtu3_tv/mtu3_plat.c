// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#include "mtu3.h"
#include "mtu3_dr.h"
#include "mtu3_debug.h"

#ifdef CONFIG_USB_MTK_DTV_XHCI
/* 22 nm mixed PHY */
#define CP6_CP8_TXDEEMPH_10G 0x2c
#define REG_CP6_CP8_TXDEEMPH_10G (SSUSB_USB3_MAC_CSR_BASE + CP6_CP8_TXDEEMPH_10G)
#define TXDEEMPH_10G_VAL 0xe
#include <linux/moduleparam.h>
int mtu3_dev_en;
EXPORT_SYMBOL(mtu3_dev_en);

module_param(mtu3_dev_en, int, 0);
MODULE_PARM_DESC(mtu3_dev_en, "Enable mtu3 device mode");
#define SHUTDOWN_DELAY_TIME 100
#endif

void ssusb_set_force_vbus(struct ssusb_mtk *ssusb, bool vbus_on)
{
	u32 u2ctl;
	u32 misc;

	if (!ssusb->force_vbus)
		return;

	u2ctl = mtu3_readl(ssusb->ippc_base, SSUSB_U2_CTRL(0));
	misc = mtu3_readl(ssusb->mac_base, U3D_MISC_CTRL);
	if (vbus_on) {
		u2ctl &= ~SSUSB_U2_PORT_OTG_SEL;
		misc |= VBUS_FRC_EN | VBUS_ON;
	} else {
		u2ctl |= SSUSB_U2_PORT_OTG_SEL;
		misc &= ~(VBUS_FRC_EN | VBUS_ON);
	}
	mtu3_writel(ssusb->ippc_base, SSUSB_U2_CTRL(0), u2ctl);
	mtu3_writel(ssusb->mac_base, U3D_MISC_CTRL, misc);
}

/* u2-port0 should be powered on and enabled; */
int ssusb_check_clocks(struct ssusb_mtk *ssusb, u32 ex_clks)
{
	void __iomem *ibase = ssusb->ippc_base;
	u32 value, check_val;
	int ret;

	check_val = ex_clks | SSUSB_SYS125_RST_B_STS | SSUSB_SYSPLL_STABLE |
			SSUSB_REF_RST_B_STS;

	ret = readl_poll_timeout(ibase + U3D_SSUSB_IP_PW_STS1, value,
			(check_val == (value & check_val)), 100, 20000);
	if (ret) {
		dev_err(ssusb->dev, "clks of sts1 are not stable!\n");
		return ret;
	}

	ret = readl_poll_timeout(ibase + U3D_SSUSB_IP_PW_STS2, value,
			(value & SSUSB_U2_MAC_SYS_RST_B_STS), 100, 10000);
	if (ret) {
		dev_err(ssusb->dev, "mac2 clock is not stable\n");
		return ret;
	}

	return 0;
}

static int ssusb_phy_init(struct ssusb_mtk *ssusb)
{
	int i;
	int ret;

	for (i = 0; i < ssusb->num_phys; i++) {
		ret = phy_init(ssusb->phys[i]);
		if (ret)
			goto exit_phy;
	}
	return 0;

exit_phy:
	for (; i > 0; i--)
		phy_exit(ssusb->phys[i - 1]);

	return ret;
}

static int ssusb_phy_exit(struct ssusb_mtk *ssusb)
{
	int i;

	for (i = 0; i < ssusb->num_phys; i++)
		phy_exit(ssusb->phys[i]);

	return 0;
}

int ssusb_phy_power_on(struct ssusb_mtk *ssusb)
{
	int i;
	int ret;

	for (i = 0; i < ssusb->num_phys; i++) {
		ret = phy_power_on(ssusb->phys[i]);
		if (ret)
			goto power_off_phy;
	}
	return 0;

power_off_phy:
	for (; i > 0; i--)
		phy_power_off(ssusb->phys[i - 1]);

	return ret;
}

void ssusb_phy_power_off(struct ssusb_mtk *ssusb)
{
	unsigned int i;

	for (i = 0; i < ssusb->num_phys; i++)
		phy_power_off(ssusb->phys[i]);
}

int ssusb_clks_enable(struct ssusb_mtk *ssusb)
{
	int ret;

	ret = clk_prepare_enable(ssusb->sys_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable sys_clk\n");
		goto sys_clk_err;
	}

	ret = clk_prepare_enable(ssusb->ref_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable ref_clk\n");
		goto ref_clk_err;
	}

	ret = clk_prepare_enable(ssusb->mcu_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable mcu_clk\n");
		goto mcu_clk_err;
	}

	ret = clk_prepare_enable(ssusb->dma_clk);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable dma_clk\n");
		goto dma_clk_err;
	}

	return 0;

dma_clk_err:
	clk_disable_unprepare(ssusb->mcu_clk);
mcu_clk_err:
	clk_disable_unprepare(ssusb->ref_clk);
ref_clk_err:
	clk_disable_unprepare(ssusb->sys_clk);
sys_clk_err:
	return ret;
}

void ssusb_clks_disable(struct ssusb_mtk *ssusb)
{
	clk_disable_unprepare(ssusb->dma_clk);
	clk_disable_unprepare(ssusb->mcu_clk);
	clk_disable_unprepare(ssusb->ref_clk);
	clk_disable_unprepare(ssusb->sys_clk);
}

static int ssusb_rscs_init(struct ssusb_mtk *ssusb)
{
	int ret = 0;
#ifdef CONFIG_USB_MTK_DTV_XHCI
	int temp;


	if (ssusb->misc1_regs) {
		/* access mem form EMI */
		temp = readl(ssusb->misc1_regs + SSUSB_MISC_1_REG_00);
		writel(temp | SSUSB_EMI_SEL,
		ssusb->misc1_regs + SSUSB_MISC_1_REG_00);

		/* select UTMI from SSUSB */
		temp = readl(ssusb->misc1_regs + SSUSB_MISC_1_REG_BC);
		writel(temp | SSUSB_UTMI_SEL,
			ssusb->misc1_regs + SSUSB_MISC_1_REG_BC);
		}

	if (ssusb->chiptop_regs) {
		temp = readl(ssusb->chiptop_regs + SSUSB_CHIP_DUMMY_REG_2);
		temp &= ~(SSUSB_I2C_MODE | SSUSB_SHBG_LINESTATE_HACK);
		temp |= SSUSB_U3FPGA_USB_EN;
		writel(temp, ssusb->chiptop_regs + SSUSB_CHIP_DUMMY_REG_2);
	}
#endif
#ifndef CONFIG_USB_MTK_DTV_XHCI
	ret = regulator_enable(ssusb->vusb33);
	if (ret) {
		dev_err(ssusb->dev, "failed to enable vusb33\n");
		goto vusb33_err;
	}
#endif

	ret = ssusb_clks_enable(ssusb);
	if (ret)
		goto clks_err;

	ret = ssusb_phy_init(ssusb);
	if (ret) {
		dev_err(ssusb->dev, "failed to init phy\n");
		goto phy_init_err;
	}

	ret = ssusb_phy_power_on(ssusb);
	if (ret) {
		dev_err(ssusb->dev, "failed to power on phy\n");
		goto phy_err;
	}




	return 0;

phy_err:
	ssusb_phy_exit(ssusb);
phy_init_err:
	ssusb_clks_disable(ssusb);
clks_err:
#ifndef CONFIG_USB_MTK_DTV_XHCI
	regulator_disable(ssusb->vusb33);
vusb33_err:
#endif
	return ret;
}

static void ssusb_rscs_exit(struct ssusb_mtk *ssusb)
{
	ssusb_clks_disable(ssusb);
#ifndef CONFIG_USB_MTK_DTV_XHCI
	regulator_disable(ssusb->vusb33);
#endif
	ssusb_phy_power_off(ssusb);
	ssusb_phy_exit(ssusb);
}

void ssusb_ip_sw_reset(struct ssusb_mtk *ssusb)
{
	/* reset whole ip (xhci & u3d) */
	mtu3_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);
	udelay(1);
	mtu3_clrbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);

	/*
	 * device ip may be powered on in firmware/BROM stage before entering
	 * kernel stage;
	 * power down device ip, otherwise ip-sleep will fail when working as
	 * host only mode
	 */
	if (ssusb->dr_mode == USB_DR_MODE_HOST)
		mtu3_setbits(ssusb->ippc_base, U3D_SSUSB_IP_PW_CTRL2,
				SSUSB_IP_DEV_PDN);
}

static int get_ssusb_rscs(struct platform_device *pdev, struct ssusb_mtk *ssusb)
{
	struct device_node *node = pdev->dev.of_node;
	struct otg_switch_mtk *otg_sx = &ssusb->otg_switch;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int i;
	int ret;

#ifndef CONFIG_USB_MTK_DTV_XHCI
	ssusb->vusb33 = devm_regulator_get(dev, "vusb33");
	if (IS_ERR(ssusb->vusb33)) {
		dev_err(dev, "failed to get vusb33\n");
		return PTR_ERR(ssusb->vusb33);
	}
#endif
#ifdef CONFIG_MSTAR_ARM_BD_FPGA
	/* sys_ck is also optonal for TV SOCs */
	//ssusb->sys_clk = get_optional_clk(dev, "sys_ck");
#else
	ssusb->sys_clk = devm_clk_get(dev, "sys_ck");
#endif
	if (IS_ERR(ssusb->sys_clk)) {
		dev_err(dev, "failed to get sys clock\n");
		return PTR_ERR(ssusb->sys_clk);
	}

	ssusb->ref_clk = devm_clk_get_optional(dev, "ref_ck");
	if (IS_ERR(ssusb->ref_clk))
		return PTR_ERR(ssusb->ref_clk);
	ssusb->mcu_clk = devm_clk_get_optional(dev, "mcu_ck");
	if (IS_ERR(ssusb->mcu_clk))
		return PTR_ERR(ssusb->mcu_clk);

	ssusb->dma_clk = devm_clk_get_optional(dev, "dma_ck");
	if (IS_ERR(ssusb->dma_clk))
		return PTR_ERR(ssusb->dma_clk);

	ssusb->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	if (ssusb->num_phys > 0) {
		ssusb->phys = devm_kcalloc(dev, ssusb->num_phys,
					sizeof(*ssusb->phys), GFP_KERNEL);
		if (!ssusb->phys)
			return -ENOMEM;
	} else {
		ssusb->num_phys = 0;
	}

	for (i = 0; i < ssusb->num_phys; i++) {
		ssusb->phys[i] = devm_of_phy_get_by_index(dev, node, i);
		if (IS_ERR(ssusb->phys[i])) {
			dev_err(dev, "failed to get phy-%d\n", i);
			return PTR_ERR(ssusb->phys[i]);
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ippc");
	ssusb->ippc_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(ssusb->ippc_base))
		return PTR_ERR(ssusb->ippc_base);

#ifdef CONFIG_USB_MTK_DTV_XHCI
	ssusb->standbypower = of_property_read_bool(node, "standbypower-domain");
	dev_notice(dev, "standbypower [%d]\n", ssusb->standbypower);
	ssusb->phy_mixed = of_property_read_bool(node,
					"mediatek,phy_mixed_mode");
	dev_notice(dev, "mixed mode phy [%d]\n", ssusb->phy_mixed);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "misc");
	if (res) {	/* misc_1 register is optional */
		ssusb->misc1_regs = devm_ioremap_resource(dev, res);
		if (IS_ERR(ssusb->misc1_regs))
			return PTR_ERR(ssusb->misc1_regs);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "chiptop");
	if (res) {	/* chiptop register is optional and only for FPGA */
		ssusb->chiptop_regs = devm_ioremap_resource(dev, res);
		if (IS_ERR(ssusb->chiptop_regs))
			return  PTR_ERR(ssusb->chiptop_regs);
	}
#endif
	ssusb->force_vbus = of_property_read_bool(node, "mediatek,force-vbus");
	ssusb->clk_mgr = of_property_read_bool(node, "mediatek,clk-mgr");

	ssusb->dr_mode = usb_get_dr_mode(dev);
	if (ssusb->dr_mode == USB_DR_MODE_UNKNOWN)
		ssusb->dr_mode = USB_DR_MODE_OTG;

	if (ssusb->dr_mode == USB_DR_MODE_PERIPHERAL)
		goto out;

	/* if host role is supported */
	ret = ssusb_wakeup_of_property_parse(ssusb, node);
	if (ret) {
		dev_err(dev, "failed to parse uwk property\n");
		return ret;
	}

	/* optional property, ignore the error if it does not exist */
	of_property_read_u32(node, "mediatek,u3p-dis-msk",
			     &ssusb->u3p_dis_msk);
#ifndef CONFIG_USB_MTK_DTV_XHCI
	otg_sx->vbus = devm_regulator_get(dev, "vbus");
	if (IS_ERR(otg_sx->vbus)) {
		dev_err(dev, "failed to get vbus\n");
		return PTR_ERR(otg_sx->vbus);
	}
#endif
	if (ssusb->dr_mode == USB_DR_MODE_HOST)
		goto out;

	/* if dual-role mode is supported */
	otg_sx->is_u3_drd = of_property_read_bool(node, "mediatek,usb3-drd");
	otg_sx->manual_drd_enabled =
		of_property_read_bool(node, "enable-manual-drd");
	otg_sx->role_sw_used = of_property_read_bool(node, "usb-role-switch");

	if (!otg_sx->role_sw_used && of_property_read_bool(node, "extcon")) {
		otg_sx->edev = extcon_get_edev_by_phandle(ssusb->dev, 0);
		if (IS_ERR(otg_sx->edev)) {
			dev_err(ssusb->dev, "couldn't get extcon device\n");
			return PTR_ERR(otg_sx->edev);
		}
	}

out:
	dev_info(dev, "dr_mode: %d, is_u3_dr: %d, u3p_dis_msk: %x, drd: %s\n",
		ssusb->dr_mode, otg_sx->is_u3_drd, ssusb->u3p_dis_msk,
		otg_sx->manual_drd_enabled ? "manual" : "auto");

	return 0;
}

static int mtu3_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct ssusb_mtk *ssusb;
	int ret = -ENOMEM;

	/* all elements are set to ZERO as default value */
	ssusb = devm_kzalloc(dev, sizeof(*ssusb), GFP_KERNEL);
	if (!ssusb)
		return -ENOMEM;

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "No suitable DMA config available\n");
		return -ENOTSUPP;
	}

	platform_set_drvdata(pdev, ssusb);
	ssusb->dev = dev;

	ret = get_ssusb_rscs(pdev, ssusb);
	if (ret)
		return ret;

	ssusb_debugfs_create_root(ssusb);

	/* enable power domain */
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	device_enable_async_suspend(dev);

	ret = ssusb_rscs_init(ssusb);
	if (ret)
		goto comm_init_err;

	if (IS_ENABLED(CONFIG_USB_MTU3_HOST))
		ssusb->dr_mode = USB_DR_MODE_HOST;
	else if (IS_ENABLED(CONFIG_USB_MTU3_GADGET))
		ssusb->dr_mode = USB_DR_MODE_PERIPHERAL;

	ssusb_ip_sw_reset(ssusb);
	/* default as host */
	ssusb->is_host = !(ssusb->dr_mode == USB_DR_MODE_PERIPHERAL);

	switch (ssusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		ret = ssusb_gadget_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto comm_exit;
		}
		break;
	case USB_DR_MODE_HOST:
#ifdef CONFIG_USB_MTK_DTV_XHCI
		/* get mac base in HOST only mode, else in GADGET mode */
		if (ssusb->phy_mixed) {
			struct resource *res;

			res = platform_get_resource_byname(pdev,
							IORESOURCE_MEM, "mac");
			ssusb->mac_base = devm_ioremap_resource(dev, res);
			if (IS_ERR(ssusb->mac_base)) {
				dev_err(dev, "error mapping memory host mac\n");
				goto comm_exit;
			}
		}
#endif
		ret = ssusb_host_init(ssusb, node);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto comm_exit;
		}
		break;
	case USB_DR_MODE_OTG:
		ret = ssusb_gadget_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto comm_exit;
		}

		ret = ssusb_host_init(ssusb, node);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto gadget_exit;
		}

		ret = ssusb_otg_switch_init(ssusb);
		if (ret) {
			dev_err(dev, "failed to initialize switch\n");
			goto host_exit;
		}
#ifdef CONFIG_USB_MTK_DTV_XHCI
		dev_err(dev, "mtu3_dev_en:%d\n", mtu3_dev_en);
		if (mtu3_dev_en)
			ssusb_mode_switch(ssusb, 0);
			//manual turn on phy for device
#endif
		break;
	default:
		dev_err(dev, "unsupported mode: %d\n", ssusb->dr_mode);
		ret = -EINVAL;
		goto comm_exit;
	}

#ifdef CONFIG_USB_MTK_DTV_XHCI
	if (ssusb->phy_mixed) {
		void __iomem *addr_t;

		addr_t = ssusb->mac_base + REG_CP6_CP8_TXDEEMPH_10G;
		writel(readl(addr_t) | TXDEEMPH_10G_VAL, addr_t);
		dev_err(dev, "SSUSB MAC TXDEEMPH value: 0x%x\n", readl(addr_t));
	}
#endif

	return 0;

host_exit:
	ssusb_host_exit(ssusb);
gadget_exit:
	ssusb_gadget_exit(ssusb);
comm_exit:
	ssusb_rscs_exit(ssusb);
comm_init_err:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	ssusb_debugfs_remove_root(ssusb);

	return ret;
}

static int mtu3_remove(struct platform_device *pdev)
{
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);

	switch (ssusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		ssusb_gadget_exit(ssusb);
		break;
	case USB_DR_MODE_HOST:
		ssusb_host_exit(ssusb);
		break;
	case USB_DR_MODE_OTG:
		ssusb_otg_switch_exit(ssusb);
		ssusb_gadget_exit(ssusb);
		ssusb_host_exit(ssusb);
		break;
	default:
		return -EINVAL;
	}

	ssusb_rscs_exit(ssusb);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	ssusb_debugfs_remove_root(ssusb);

	return 0;
}

/*
 * when support dual-role mode, we reject suspend when
 * it works as device mode;
 */
static int __maybe_unused mtu3_suspend(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	/* REVISIT: disconnect it for only device mode? */
	if (!ssusb->is_host)
		return 0;

	ssusb_host_disable(ssusb, true);
	ssusb_phy_power_off(ssusb);
	ssusb_clks_disable(ssusb);
	ssusb_wakeup_set(ssusb, true);

	return 0;
}

static int __maybe_unused mtu3_resume(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	if (!ssusb->is_host)
		return 0;

	ssusb_wakeup_set(ssusb, false);
	ret = ssusb_clks_enable(ssusb);
	if (ret)
		goto clks_err;

	ret = ssusb_phy_power_on(ssusb);
	if (ret)
		goto phy_err;

	ssusb_host_enable(ssusb);

	return 0;

phy_err:
	ssusb_clks_disable(ssusb);
clks_err:
	return ret;
}

#ifdef CONFIG_USB_MTK_DTV_XHCI
static int __maybe_unused mtu3_suspend_tv(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);
	struct mtu3 *mtu = ssusb->u3d;

	dev_dbg(dev, "%s\n", __func__);

	if (ssusb->standbypower) {
		dev_warn(dev, "suspend [PM][+]\n");
		mtu3_suspend(dev);
		dev_warn(dev, "suspend [PM][-]\n");
	} else {
		dev_warn(dev, "suspend [Non-PM][+]\n");
		if (!ssusb->is_host) {
			mtu3_gadget_disconnect_normal(mtu);
			disable_irq(mtu->irq);
		}
		ssusb_ip_sw_reset(ssusb);
		ssusb_rscs_exit(ssusb);
		dev_warn(dev, "suspend [Non-PM][-]\n");
	}
	return 0;
}
static int __maybe_unused mtu3_resume_tv(struct device *dev)
{
	struct ssusb_mtk *ssusb = dev_get_drvdata(dev);
	struct mtu3 *mtu = ssusb->u3d;

	dev_dbg(dev, "%s\n", __func__);

	if (ssusb->standbypower) {
		dev_warn(dev, "resume [PM][+]\n");
		mtu3_resume(dev);
		dev_warn(dev, "resume [PM][-]\n");
	} else {
		dev_warn(dev, "resume [Non-PM][+]\n");
		ssusb_rscs_init(ssusb);
		ssusb_ip_sw_reset(ssusb);
		ssusb_host_enable(ssusb);
		if (!ssusb->is_host) {
			ssusb_device_resume_tv(ssusb);
			ssusb_mode_switch(ssusb, 0);
			enable_irq(mtu->irq);
		}
		dev_warn(dev, "resume [Non-PM][-]\n");
	}
	return 0;
}
static void mtu3_shutdown_tv(struct platform_device *pdev)
{
	struct ssusb_mtk *ssusb = platform_get_drvdata(pdev);

	if (!ssusb->is_host)
		return;

	mdelay(SHUTDOWN_DELAY_TIME);
	ssusb_host_disable(ssusb, false);
	mdelay(SHUTDOWN_DELAY_TIME);
}
#endif

static const struct dev_pm_ops mtu3_pm_ops = {
#ifdef CONFIG_USB_MTK_DTV_XHCI
	SET_SYSTEM_SLEEP_PM_OPS(mtu3_suspend_tv, mtu3_resume_tv)
#else
	SET_SYSTEM_SLEEP_PM_OPS(mtu3_suspend, mtu3_resume)
#endif
};

#define DEV_PM_OPS (IS_ENABLED(CONFIG_PM) ? &mtu3_pm_ops : NULL)

#ifdef CONFIG_OF

static const struct of_device_id mtu3_of_match[] = {
#ifdef CONFIG_USB_MTK_DTV_XHCI
	{.compatible = "mediatek,mtk-dtv-mtu3",},
#endif
	{.compatible = "mediatek,mt8173-mtu3",},
	{.compatible = "mediatek,mtu3",},
	{},
};

MODULE_DEVICE_TABLE(of, mtu3_of_match);

#endif

static struct platform_driver mtu3_driver = {
	.probe = mtu3_probe,
	.remove = mtu3_remove,
#ifdef CONFIG_USB_MTK_DTV_XHCI
	.shutdown = mtu3_shutdown_tv,
#endif
	.driver = {
		.name = MTU3_DRIVER_NAME,
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(mtu3_of_match),
	},
};
module_platform_driver(mtu3_driver);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek USB3 DRD Controller Driver");
