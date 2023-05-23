// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/* Copyright (c) 2020 MediaTek Inc.
 * Author Kevin Ho <kevin-yc.ho@mediatek.com>
 */

#include "mtk_gmac_mt5896_coda.h"
#include "mtk_gmac_mt5896_riu.h"
#include "mtk_gmac_mt5896.h"

enum mtk_mac_mode_slt {
	MTK_MAC_SLT_MIN = 0,
	MTK_MAC_SLT_AUTO = MTK_MAC_SLT_MIN,
	MTK_MAC_SLT_FORCE,
	MTK_MAC_SLT_CLEAR,
	MTK_MAC_SLT_PHY_LOOPBACK,
	MTK_MAC_SLT_MAX
};

struct mtk_mac_ioctl_slt_para_cmd {
	enum mtk_mac_mode_slt test_mode;
	int test_count;
	int failed_count;
};

enum mtk_mac_mode_cts {
	MTK_MAC_CTS_MIN = 0,
	MTK_MAC_CTS_RANDOM = MTK_MAC_CTS_MIN,
	MTK_MAC_CTS_LOOPBACK,
	MTK_MAC_CTS_PAD_SKEW_READ,
	MTK_MAC_CTS_PAD_SKEW_WRITE_RX,
	MTK_MAC_CTS_PAD_SKEW_WRITE_TX,
	MTK_MAC_CTS_RXC_DLL_DELAY_ENABLE,
	MTK_MAC_CTS_RXC_DLL_DELAY_DISABLE,
	MTK_MAC_CTS_RXC_DLL_DELAY_TUNE,
	MTK_MAC_CTS_RGMII_IO_DRIVING,
	MTK_MAC_CTS_RGMII_CELLMODE,
	MTK_MAC_CTS_RGMII_CELLMODE_DELAY,
	MTK_MAC_CTS_TX_100T_RANDOM,
	MTK_MAC_CTS_TX_10T_LTP,
	MTK_MAC_CTS_TX_10T_TP_IDL,
	MTK_MAC_CTS_TX_10T_ALL_ONE,
	MTK_MAC_CTS_FORCE_10T,
	MTK_MAC_CTS_1000_TX_TEMPLATE_MODE_1,
	MTK_MAC_CTS_1000_TX_TEMPLATE_MODE_2,
	MTK_MAC_CTS_1000_TX_TEMPLATE_MODE_3,
	MTK_MAC_CTS_1000_TX_TEMPLATE_MODE_4,
	MTK_MAC_CTS_100_TX_TEMPLATE,
	MTK_MAC_CTS_10_TX_TEMPLATE_INIT,
	MTK_MAC_CTS_10_TX_TEMPLATE_CASE_1,
	MTK_MAC_CTS_10_TX_TEMPLATE_CASE_2,
	MTK_MAC_CTS_10_TX_TEMPLATE_CASE_3,
	MTK_MAC_CTS_10_TX_TEMPLATE_CASE_4,
	MTK_MAC_CTS_PHY_WRITE,
	MTK_MAC_CTS_PHY_READ,
	MTK_MAC_CTS_MAX
};

enum mtk_mac_phy_vendor {
	MTK_MAC_PHY_VENDOR_MIN = 0,
	MTK_MAC_PHY_VENDOR_MICROCHIP = MTK_MAC_PHY_VENDOR_MIN,
	MTK_MAC_PHY_VENDOR_ICPLUS,
	MTK_MAC_PHY_VENDOR_TI,
	MTK_MAC_PHY_VENDOR_MAX
};

struct mtk_mac_ioctl_cts_para_cmd {
	/* 1: random packet generator:
	 *      input: packet length
	 * 2: GPHY test:
	 *        1) near-end loopback
	 *        2) PAD skew read
	 *          input: NA
	 *        3) RXD PAD skew write
	 *        4) TXD PAD skew write
	 *          input: value writing to reg
	 * 3: CTS settings:
	 *        1) ETH_TX_100T_RANDOM
	 *        2) ETH_TX_10T_LTP
	 *        3) ETH_TX_10T_TP_IDL
	 *        4) ETH_TX_10T_ALL_ONE
	 *        5) ETH_FORCE_10T
	 *          input: NA
	 */
	enum mtk_mac_mode_cts test_mode;
	int input;
	int val;
};

static const int slt_packet_len = SLT_PKT_LEN;

void mtk_dtv_gmac_slt_rx(struct net_device *ndev, struct sk_buff *skb, int len)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	dma_addr_t skb_addr;
	u32 reg_read;
	int err_cnt_rx = 0;
	int err_cnt_tx = 0;
	int err_cnt_col = 0;

	/* rx packet len is not include 4 bytes crc */
	if (len != slt_packet_len) {
		dev_err(priv->dev,
			"slt-eth: not test packet, skip it (len=%d)\n", len);
		dev_kfree_skb_any(skb);
		return;
	}

	/* get test packet */
	skb_addr = priv->tx_memcpy_vir_addr + (TX_MEMCPY_SIZE_DEFAULT * priv->tx_memcpy_index);
	priv->tx_memcpy_index++;
	if (priv->tx_memcpy_index >= TX_MEMCPY_NUM_DEFAULT)
		priv->tx_memcpy_index = 0;

	/* disable copy all frame mode */
	reg_read  = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0);
	reg_read &= ~(0x10UL);
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0, reg_read);

	dev_dbg(priv->dev, "slt-eth: rx packet\n");

	/* get error count */
	err_cnt_rx = (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00A8_X32_GMAC0)) +
		     (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00F0_X32_GMAC0)) +
		     (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00A0_X32_GMAC0)) +
		     (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00E0_X32_GMAC0)) +
		     (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00E8_X32_GMAC0)) +
		     (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00F8_X32_GMAC0));
	err_cnt_tx = (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00C8_X32_GMAC0)) +
		     (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00D0_X32_GMAC0)) +
		     (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0108_X32_GMAC0));

	err_cnt_col = (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00B8_X32_GMAC0)) +
		      (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00C0_X32_GMAC0)) +
		      (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0088_X32_GMAC0)) +
		      (mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0090_X32_GMAC0));

	/* compare tx rx packet data */
	if ((memcmp((void *)skb_addr, skb->data, slt_packet_len) != 0) ||
	    err_cnt_rx != 0 || err_cnt_tx != 0 || err_cnt_col != 0) {
		dev_err(priv->dev, "slt-eth: rx failed, rx_err: %d, tx_err: %d, col_err: %d\n",
			err_cnt_rx, err_cnt_tx, err_cnt_col);
		dev_err(priv->dev, "slt-eth: tx packet dump:\n");
		mtk_dtv_gmac_skb_dump(skb_addr, slt_packet_len);
		dev_err(priv->dev, "slt-eth: rx packet dump:\n");
		mtk_dtv_gmac_skb_dump((phys_addr_t)skb->data, slt_packet_len);
		priv->is_slt_test_passed = false;
	} else {
		dev_info(priv->dev, "slt-eth: passed\n");
	}

	dev_kfree_skb_any(skb);
	/* restore delay interrupt: 0x0402 */
	mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_000C_X32_OGMAC1, 0x0402);
	priv->is_slt_test_running = false;
	netif_start_queue(ndev);
}

/* slt-test */
static int _mtk_dtv_gmac_slt_tx(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	uintptr_t skb_addr;
	dma_addr_t dma_addr, tx_memcpy_dma_addr;
	u32 reg_read;
	int i;
	unsigned char golden_skb[SLT_PKT_LEN];

	dev_dbg(priv->dev, "slt-eth: start slt tx\n");

	/* stop kernel tx packet */
	netif_stop_queue(ndev);

	/* enable copy all frame mode */
	reg_read  = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0);
	reg_read |= 0x10UL;
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0, reg_read);

	/* set delay interrupt to 1 packet 1 time */
	/* delay interrupt: 0x0101 */
	mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_000C_X32_OGMAC1, 0x0101);

	/* prepare test packet */
	skb_addr = priv->tx_memcpy_vir_addr + (TX_MEMCPY_SIZE_DEFAULT * priv->tx_memcpy_index);
	if (!skb_addr) {
		dev_err(priv->dev, "bad skb_addr\n");
		return -ENOMEM;
	}

	for (i = 0; i < slt_packet_len; i++)
		golden_skb[i] = i % 256;

	memcpy((void *)skb_addr, golden_skb, slt_packet_len);

	tx_memcpy_dma_addr = priv->tx_memcpy_dma_addr - MIU0_BUS_BASE;

	dma_addr = tx_memcpy_dma_addr + (TX_MEMCPY_SIZE_DEFAULT * priv->tx_memcpy_index);
	//dev_info(priv->dev, "slt tx packet dma: 0x%lx\n", dma_addr);
	//mtk_dtv_gmac_skb_dump(skb_addr, slt_packet_len);

	/* EMAC error count, read clear */
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00A8_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00F0_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00A0_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00E0_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00E8_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00F8_X32_GMAC0);

	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00C8_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00D0_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0108_X32_GMAC0);

	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00B8_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_00C0_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0088_X32_GMAC0);
	mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0090_X32_GMAC0);

	priv->is_slt_test_passed = true;
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0018_X32_GMAC0, dma_addr);
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0020_X32_GMAC0, slt_packet_len);

	dev_dbg(priv->dev, "slt-eth: end slt tx\n");

	return 0;
}

static int _mtk_dtv_gmac_slt_run(struct net_device *ndev, int test_count, int *failed_count)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int i, timeout, ret;

	if (test_count > SLT_UPPER_BOUND || test_count < SLT_LOWER_BOUND) {
		dev_err(priv->dev,
			"slt-eth: invalid test_count(%d)\n",
			test_count);
		return -EINVAL;
	}

	for (i = 0; i < test_count; i++) {
		priv->is_slt_test_running = true;
		priv->is_slt_test_passed = false;
		timeout = 0;
slt_run_resend:
		ret = _mtk_dtv_gmac_slt_tx(ndev);
		if (ret) {
			dev_err(priv->dev,
				"slt-eth: tx fail (%d)\n",
				ret);
			return -EFAULT;
		}

		while (priv->is_slt_test_running) {
			mdelay(1);
			if (timeout > 10000) {
				dev_err(priv->dev,
					"slt-eth: rx timeout (%d)\n",
					i);
				break;
			}
			timeout++;
			if (timeout % 1000 == 0) {
				dev_err(priv->dev, "slt-eth: resend packet (%d at %d)\n",
					i, timeout);
				goto slt_run_resend;
			}
		}

		if (!priv->is_slt_test_running &&
		    priv->is_slt_test_passed) {
			dev_dbg(priv->dev, "slt-eth: pass (%d)\n",
				i);
		} else {
			dev_err(priv->dev, "slt-eth: fail (%d)\n",
				i);
			priv->is_slt_test_running = false;
			priv->is_slt_test_passed = false;
			*failed_count = i;

			return -EFAULT;
		}
	}
	return 0;
}

static int _mtk_dtv_gmac_slt_phy_loopback(struct net_device *ndev,
					  int test_count, int *failed_count)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	int retry, ret, cnt;
	u8 reg, reg_orig;

	dev_info(priv->dev, "slt-eth: phy loopback: cnt=%d\n",
		 test_count);

	retry = 0;
	ret = -EFAULT;
	while ((retry < 10) && (ret != 0)) {
		dev_info(priv->dev, "slt-eth:%d: disconnect far-end first\n", retry);
		reg_orig = mtk_readb(REG_OFFSET_69_H(BANK_BASE_ALBANY_1));
		reg = reg_orig | 0x80;
		mtk_writeb(REG_OFFSET_69_H(BANK_BASE_ALBANY_1), reg);
		reg = mtk_readb(REG_OFFSET_69_H(BANK_BASE_ALBANY_1));

		dev_info(priv->dev, "slt-eth: set phy loopback: 0x%x\n", reg);
		reg_ops->write_phy(ndev, priv->phy_addr, 0x37, 0xff7f);
		reg_ops->write_phy(ndev, priv->phy_addr, 0x16, 0x4c00);
		reg_ops->write_phy(ndev, priv->phy_addr, 0x04, 0x0181);
		reg_ops->write_phy(ndev, priv->phy_addr, 0x00, 0x6100);

		mdelay(5000);

		cnt = 0;
		while (netif_carrier_ok(ndev)) {
			mdelay(1000);
			cnt++;
			if (cnt > 10) {
				dev_err(priv->dev, "slt-eth:%d: cannot disconnect far-end\n",
					retry);
				break;
			}
		};

		mdelay(2000);

		dev_info(priv->dev, "slt-eth:%d: start slt run\n", retry);
		ret = _mtk_dtv_gmac_slt_run(ndev, test_count,
					    failed_count);
		dev_info(priv->dev, "slt-eth:%d: end slt run: %d\n", retry, ret);

		dev_info(priv->dev, "slt-eth:%d: clear phy looback: 0x%x\n", retry, reg_orig);
		mtk_writeb(REG_OFFSET_69_H(BANK_BASE_ALBANY_1), reg_orig);
		reg_ops->write_phy(ndev, priv->phy_addr, 0x37, 0xff00);
		reg_ops->write_phy(ndev, priv->phy_addr, 0x16, 0x7c00);
		reg_ops->write_phy(ndev, priv->phy_addr, 0x04, 0x01e1);
		reg_ops->write_phy(ndev, priv->phy_addr, 0x00, 0x3100);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR, BMCR_RESET);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_ADVERTISE,
				   (ADVERTISE_CSMA | ADVERTISE_ALL));
		reg_ops->write_phy(ndev, priv->phy_addr, MII_CTRL1000, 0x0);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   (BMCR_ANENABLE | BMCR_ANRESTART));
		priv->is_slt_test_running = false;

		dev_info(priv->dev, "slt-eth:%d: hardware init\n", retry);
		mtk_dtv_gmac_hw_init(ndev, false);

		/* wait for auto-nego */
		mdelay(5000);
		retry++;
	}

	return ret;
}

int mtk_dtv_gmac_slt_ioctl(struct net_device *ndev, struct ifreq *req)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	struct mtk_mac_ioctl_slt_para_cmd slt_cmd;
	unsigned int timeout, adv_orig;
	int ret = 0;

	if (copy_from_user(&slt_cmd, req->ifr_data, sizeof(slt_cmd))) {
		dev_err(priv->dev, "slt-eth: copy from user fail\n");
		return -EFAULT;
	}

	switch (slt_cmd.test_mode) {
	case MTK_MAC_SLT_AUTO:
		dev_info(priv->dev, "slt-eth: auto-nego: %d\n",
			 slt_cmd.test_count);

		ret = _mtk_dtv_gmac_slt_run(ndev, slt_cmd.test_count,
					    &slt_cmd.failed_count);

		break;
	case MTK_MAC_SLT_FORCE:
		adv_orig = reg_ops->read_phy(ndev, priv->phy_addr, MII_ADVERTISE);

		/* test speed 100 start */
		dev_info(priv->dev, "slt-eth: speed 100: %d\n",
			 slt_cmd.test_count);

		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   BMCR_RESET);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_ADVERTISE,
				   (ADVERTISE_CSMA | ADVERTISE_100FULL |
				    ADVERTISE_100HALF));
		mdelay(10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_CTRL1000, 0x0);
		mdelay(10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   (BMCR_SPEED100 | BMCR_FULLDPLX));

		if (netif_carrier_ok(ndev))
			netif_carrier_off(ndev);

		priv->link_timer.expires = jiffies + (MAC_UPDATE_LINK_TIME / 10);

		if (!(timer_pending(&priv->link_timer)))
			add_timer(&priv->link_timer);

		timeout = 0;

		while ((!netif_carrier_ok(ndev))) {
			mdelay(1);
			if (timeout > 5000) {
				dev_err(priv->dev,
					"slt-eth: speed 100 timeout, no connection\n");
				priv->is_slt_test_running = false;
				reg_ops->write_phy(ndev, priv->phy_addr,
						   MII_ADVERTISE, adv_orig);
				reg_ops->write_phy(ndev, priv->phy_addr,
						   MII_BMCR,
						   (BMCR_ANENABLE | BMCR_ANRESTART));
				return -EFAULT;
			}
			timeout++;
		}

		ret = _mtk_dtv_gmac_slt_run(ndev, slt_cmd.test_count,
					    &slt_cmd.failed_count);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_ADVERTISE, adv_orig);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   (BMCR_ANENABLE | BMCR_ANRESTART));

		if (ret < 0)
			break;

		/* test speed 10 start */
		dev_info(priv->dev, "slt-eth: test speed 10: %d\n", slt_cmd.test_count);

		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR, BMCR_RESET);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_ADVERTISE,
				   (ADVERTISE_CSMA | ADVERTISE_10FULL | ADVERTISE_10HALF));
		mdelay(10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_CTRL1000, 0x0);
		mdelay(10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   (BMCR_SPEED10 | BMCR_FULLDPLX));

		if (netif_carrier_ok(ndev))
			netif_carrier_off(ndev);

		priv->link_timer.expires = jiffies + (MAC_UPDATE_LINK_TIME / 10);

		if (!(timer_pending(&priv->link_timer)))
			add_timer(&priv->link_timer);

		timeout = 0;

		while ((!netif_carrier_ok(ndev))) {
			mdelay(1);
			if (timeout > 5000) {
				dev_err(priv->dev,
					"slt-eth: speed 10 timeout 5 secs, no connection\n");
				priv->is_slt_test_running = false;
				reg_ops->write_phy(ndev, priv->phy_addr,
						   MII_ADVERTISE, adv_orig);
				reg_ops->write_phy(ndev, priv->phy_addr,
						   MII_BMCR,
						   (BMCR_ANENABLE | BMCR_ANRESTART));
				return -EFAULT;
			}
			timeout++;
		}

		ret = _mtk_dtv_gmac_slt_run(ndev, slt_cmd.test_count,
					    &slt_cmd.failed_count);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_ADVERTISE, adv_orig);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   (BMCR_ANENABLE | BMCR_ANRESTART));

		break;
	case MTK_MAC_SLT_CLEAR:
		dev_info(priv->dev, "slt-eth: clear test environment\n");
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR, BMCR_RESET);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_ADVERTISE,
				   (ADVERTISE_CSMA | ADVERTISE_ALL));
		reg_ops->write_phy(ndev, priv->phy_addr, MII_CTRL1000, 0x0);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
				   (BMCR_ANENABLE | BMCR_ANRESTART));
		priv->is_slt_test_running = false;

		break;
	case MTK_MAC_SLT_PHY_LOOPBACK:
		ret = _mtk_dtv_gmac_slt_phy_loopback(ndev, slt_cmd.test_count,
						     &slt_cmd.failed_count);

		break;
	default:
		dev_err(priv->dev, "slt-eth: unknown test mode (%d)\n",
			slt_cmd.test_mode);
		break;
	}

	if (copy_to_user(req->ifr_data, &slt_cmd, sizeof(slt_cmd))) {
		dev_err(priv->dev, "slt-eth: copy to user fail\n");
		return -EFAULT;
	}

	return ret;
}

/* cts */
static int _mtk_dtv_gmac_cts_random_tx(struct net_device *ndev, int pkt_len)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	uintptr_t skb_addr;
	dma_addr_t dma_addr, tx_memcpy_dma_addr;
	u32 reg_read;
	int i;
	char *golden_skb;
	unsigned long flags;
	u32 tsr_val;
	u8 tx_fifo[8] = {0};
	u8 tx_fifo_token = 0;

	dev_info(priv->dev, "cts-eth: start cts random tx\n");

	/* stop kernel tx packet */
	netif_stop_queue(ndev);

	/* check tx fifo */
	spin_lock_irqsave(&priv->tx_lock, flags);
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

	if (tx_fifo_token <= 4) {
		spin_unlock_irqrestore(&priv->tx_lock, flags);
		mdelay(10);
		return NETDEV_TX_BUSY;
	}

	/* enable copy all frame mode */
	reg_read  = mtk_readl(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0);
	reg_read |= 0x10UL;
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0, reg_read);

	/* set delay interrupt to 1 packet 1 time */
	/* delay interrupt: 0x0101 */
	mtk_writew(BANK_BASE_MAC_1 + OGMAC1_REG_000C_X32_OGMAC1, 0x0101);

	/* prepare test packet */
	skb_addr = priv->tx_memcpy_vir_addr;
	if (!skb_addr) {
		spin_unlock_irqrestore(&priv->tx_lock, flags);
		dev_err(priv->dev, "bad skb_addr\n");
		return -ENOMEM;
	}
	golden_skb = (char *)skb_addr;
	if (pkt_len < 0) {
		/* random packet length */
		pkt_len = get_random_int() % 0x1fffUL;
	}
	if (pkt_len < 64)
		pkt_len = 64;

	get_random_bytes(golden_skb, pkt_len);

	tx_memcpy_dma_addr = priv->tx_memcpy_dma_addr - MIU0_BUS_BASE;

	dma_addr = tx_memcpy_dma_addr;
	//dev_info(priv->dev, "cts tx packet dma: 0x%lx\n", dma_addr);
	//mtk_dtv_gmac_skb_dump(skb_addr, pkt_len);

	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0018_X32_GMAC0, dma_addr);
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0020_X32_GMAC0, pkt_len);
	spin_unlock_irqrestore(&priv->tx_lock, flags);

	dev_info(priv->dev, "cts-eth: end cts random tx\n");

	return 0;
}

static void _mtk_dtv_gmac_cts_loopback(struct net_device *ndev, enum mtk_mac_phy_vendor phy)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	if (phy >= MTK_MAC_PHY_VENDOR_MAX) {
		dev_err(priv->dev, "cts-eth: invalid phy vendor\n");
		return;
	}

	switch (phy) {
	case MTK_MAC_PHY_VENDOR_ICPLUS:
		dev_info(priv->dev, "cts-eth: set IC Plus GPHY PCS loopback\n");
		/* PCS Loopback (Page 0 Reg0 [14] =1) */
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_NWAYTEST);
		dev_info(priv->dev, "cts-eth: 0x%x=0x%x\n", MII_NWAYTEST, reg);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_NWAYTEST, 0x00);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_NWAYTEST);
		dev_info(priv->dev, "cts-eth: 0x%x=0x%x\n", MII_NWAYTEST, reg);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_BMCR);
		dev_info(priv->dev, "cts-eth: 0x%x=0x%x\n", MII_BMCR, reg);
		reg |= BMCR_LOOPBACK;
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR, reg);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_BMCR);
		dev_info(priv->dev, "cts-eth: 0x%x=0x%x\n", MII_BMCR, reg);
		break;

	/* MTK_MAC_PHY_VENDOR_MICROCHIP is as default */
	default:
		dev_info(priv->dev, "cts-eth: set Microchip GPHY near-end loopback\n");
		/* MMD 1C, Register 15 = EEEE */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x1C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x15);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0xEEEE);
		/* MMD 1C, Register 16 = EEEE */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x1C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x16);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0xEEEE);
		/* MMD 1C, Register 18 = EEEE */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x1C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x18);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0xEEEE);
		/* MMD 1C, Register 1B = EEEE */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x1C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x1B);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0xEEEE);
		/* read them */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x1C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x15);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401C);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
		dev_info(priv->dev, "cts-eth: 0x15=0x%x\n", reg);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x1C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x16);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401C);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
		dev_info(priv->dev, "cts-eth: 0x16=0x%x\n", reg);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x1C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x18);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401C);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
		dev_info(priv->dev, "cts-eth: 0x18=0x%x\n", reg);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x1C);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x1B);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401C);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
		dev_info(priv->dev, "cts-eth: 0x1B=0x%x\n", reg);
		/* Configure the Basic Control Register:
		 *  - Bit [14] = 1 // Enable local loopback mode
		 *  - Bits [6, 13] = 10 // Select 1000 Mbps speed
		 *  - Bit [12] = 0 // Disable auto-negotiation
		 *  - Bit [8] = 1 // Select full-duplex mode
		 */
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_BMCR);
		reg &= ~(0x3000UL);
		reg |= (0x4140UL);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR, reg);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_BMCR);
		dev_info(priv->dev, "cts-eth: MII_BMCR=0x%x\n", reg);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_BMSR);
		dev_info(priv->dev, "cts-eth: MII_BMSR=0x%x\n", reg);
		/* Configure the Auto-Negotiation Master Slave Control Register:
		 *  - Bit [12] = 1 // Enable master-slave manual configuration
		 *  - Bit [11] = 0 // Select slave configuration
		 *                    (required for loopback mode)
		 */
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_CTRL1000);
		reg &= ~(0x800UL);
		reg |= (0x1000UL);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_CTRL1000, reg);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_CTRL1000);
		dev_info(priv->dev, "cts-eth: MII_CTRL1000=0x%x\n", reg);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_STAT1000);
		dev_info(priv->dev, "cts-eth: MII_STAT1000=0x%x\n", reg);
		break;
	}
}

static void _mtk_dtv_gmac_cts_pad_skew_read(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: pad skew read\n");
	/* Read RX MMD 2, Register 5 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x05);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: RXD Pad Skew=0x%x\n", reg);
	/* Read TX MMD 2, Register 6 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x06);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: TXD Pad Skew=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_pad_skew_write_rx(struct net_device *ndev, int skew)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: pad skew write rx (0x%x)\n",
		 skew);
	/* Read RX MMD 2, Register 5 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x05);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: before RXD Pad Skew=0x%x\n", reg);
	/* Write RX MMD 2, Register 5 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x05);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, skew);
	/* Read RX MMD 2, Register 5 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x05);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: after RXD Pad Skew=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_pad_skew_write_tx(struct net_device *ndev, int skew)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: pad skew write tx (0x%x)\n",
		 skew);
	/* Read TX MMD 2, Register 6 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x06);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: before TXD Pad Skew=0x%x\n", reg);
	/* Write TX MMD 2, Register 6 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x06);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, skew);
	/* Read TX MMD 2, Register 6 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x06);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: after TXD Pad Skew=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_rxc_dll_delay(struct net_device *ndev, bool is_enable,
					    enum mtk_mac_phy_vendor phy)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	if (phy >= MTK_MAC_PHY_VENDOR_MAX) {
		dev_err(priv->dev, "cts-eth: invalid phy vendor\n");
		return;
	}

	switch (phy) {
	case MTK_MAC_PHY_VENDOR_ICPLUS:
		dev_info(priv->dev, "cts-eth: IC Plus RXC DLL Delay %s\n",
			 is_enable ? "enable" : "disable");
		/* Read MMD 0x1f, Register 0x0e10 */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
		dev_info(priv->dev, "cts-eth: before RXC=0x%x\n", reg);

		/* bit[14][6]: delay +2ns */
		if (is_enable)
			reg |= 0x4040UL;
		else
			reg &= ~(0x4040UL);

		dev_info(priv->dev, "cts-eth: write reg=0x%x\n", reg);
		/* Write MMD 0x1f, Register 0x0e10 */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, reg);
		/* Read MMD 0x1f, Register 0x0e10 */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e10);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
		dev_info(priv->dev, "cts-eth: after RXC=0x%x\n", reg);
		break;

	/* MTK_MAC_PHY_VENDOR_MICROCHIP is as default */
	default:
		dev_info(priv->dev, "cts-eth: Microchip RXC DLL Delay %s\n",
			 is_enable ? "enable" : "disable");
		/* Read RXC MMD 2, Register 76 */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x4c);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
		dev_info(priv->dev, "cts-eth: before RXC=0x%x\n", reg);

		if (is_enable)
			reg &= ~(0x1000UL);
		else
			reg |= 0x1000UL;

		dev_info(priv->dev, "cts-eth: write reg=0x%x\n", reg);
		/* Write RXC MMD 2, Register 76 */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x4c);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, reg);
		/* Read RXC MMD 2, Register 76 */
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x02);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x4c);
		reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x4002);
		reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
		dev_info(priv->dev, "cts-eth: after RXC=0x%x\n", reg);
		break;
	}
}

static void _mtk_dtv_gmac_cts_rxc_dll_delay_tune(struct net_device *ndev,
						 int delay)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus RXC DLL Delay tune 0x%x\n",
		 delay);
	/* Read MMD 0x1f, Register 0x0e10 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e10);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: before RXC=0x%x\n", reg);

	/* bit[13:11][5:3] delay */
	reg &= ~(0x3838UL);
	reg |= ((delay & 0x7) << 3);
	reg |= ((delay & 0x7) << 11);

	dev_info(priv->dev, "cts-eth: write reg=0x%x\n", reg);
	/* Write MMD 0x1f, Register 0x0e10 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e10);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, reg);
	/* Read MMD 0x1f, Register 0x0e10 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e10);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: after RXC=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_rgmii_io_driving(struct net_device *ndev,
					       int drv_cur)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus RGMII io driving 0x%x\n",
		 drv_cur);
	/* Read MMD 0x1f, Register 0x0e09 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e09);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: before RGMII IO=0x%x\n", reg);

	/* bit[8:6] current */
	reg &= ~(0x01c0UL);
	reg |= ((drv_cur & 0x7) << 6);

	dev_info(priv->dev, "cts-eth: write reg=0x%x\n", reg);
	/* Write MMD 0x1f, Register 0x0e09 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e09);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, reg);
	/* Read MMD 0x1f, Register 0x0e09 */
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x001f);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_DATA, 0x0e09);
	reg_ops->write_phy(ndev, priv->phy_addr, MII_MMD_CTRL, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, MII_MMD_DATA);
	dev_info(priv->dev, "cts-eth: after RGMII IO=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_rgmii_cellmode(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "cts-eth: start RGMII_CELLMODE\n");
	//wriu     0x120322             0x2a
	mtk_writeb(REG_OFFSET_11_L(BANK_BASE_TZPC_AID_0), 0x2a);
	//wriu     0x120323             0x00
	mtk_writeb(REG_OFFSET_11_H(BANK_BASE_TZPC_AID_0), 0x00);
	//wriu     0x120324             0x00
	mtk_writeb(REG_OFFSET_12_L(BANK_BASE_TZPC_AID_0), 0x00);
	//wriu     0x120325             0x00
	mtk_writeb(REG_OFFSET_12_H(BANK_BASE_TZPC_AID_0), 0x00);
	//wriu     0x0421b4             0x02
	mtk_writeb(REG_OFFSET_5A_L(BANK_BASE_ALBANY_1), 0x02);
	//wriu     0x0421b5             0x00
	mtk_writeb(REG_OFFSET_5A_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x04214e             0x4c
	mtk_writeb(REG_OFFSET_27_L(BANK_BASE_ALBANY_1), 0x4c);
	//wriu     0x04214f             0x02
	mtk_writeb(REG_OFFSET_27_H(BANK_BASE_ALBANY_1), 0x02);
	//wriu     0x042150             0x60
	mtk_writeb(REG_OFFSET_28_L(BANK_BASE_ALBANY_1), 0x60);
	//wriu     0x042151             0x01
	mtk_writeb(REG_OFFSET_28_H(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x042176             0x00
	mtk_writeb(REG_OFFSET_3B_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x042177             0x18
	mtk_writeb(REG_OFFSET_3B_H(BANK_BASE_ALBANY_1), 0x18);
	//wriu     0x042072             0xa0
	mtk_writeb(REG_OFFSET_39_L(BANK_BASE_ALBANY_0), 0xa0);
	//wriu     0x042073             0x04
	mtk_writeb(REG_OFFSET_39_H(BANK_BASE_ALBANY_0), 0x04);
	//wriu     0x0421fc             0x00
	mtk_writeb(REG_OFFSET_7E_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x0421fd             0x00
	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x0422a0             0x00
	mtk_writeb(REG_OFFSET_50_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422a1             0x80
	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), 0x80);
	//wriu     0x0422fa             0x9c
	mtk_writeb(REG_OFFSET_7D_L(BANK_BASE_ALBANY_2), 0x9c);
	//wriu     0x0422fb             0x02
	mtk_writeb(REG_OFFSET_7D_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x04228e             0x9c
	mtk_writeb(REG_OFFSET_47_L(BANK_BASE_ALBANY_2), 0x9c);
	//wriu     0x04228f             0x02
	mtk_writeb(REG_OFFSET_47_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0422dc             0x9c
	mtk_writeb(REG_OFFSET_6E_L(BANK_BASE_ALBANY_2), 0x9c);
	//wriu     0x0422dd             0x02
	mtk_writeb(REG_OFFSET_6E_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0422d4             0x00
	mtk_writeb(REG_OFFSET_6A_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422d5             0x00
	mtk_writeb(REG_OFFSET_6A_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422d8             0x00
	mtk_writeb(REG_OFFSET_6C_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422d9             0x00
	mtk_writeb(REG_OFFSET_6C_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422f6             0x9c
	mtk_writeb(REG_OFFSET_7B_L(BANK_BASE_ALBANY_2), 0x9c);
	//wriu     0x0422f7             0x02
	mtk_writeb(REG_OFFSET_7B_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0421cc             0x40
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), 0x40);
	//wriu     0x0421cd             0xd8
	mtk_writeb(REG_OFFSET_66_H(BANK_BASE_ALBANY_1), 0xd8);
	//wriu     0x0421ba             0x03
	mtk_writeb(REG_OFFSET_5D_L(BANK_BASE_ALBANY_1), 0x03);
	//wriu     0x0421bb             0x04
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), 0x04);
	//wriu     0x0422aa             0x06
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0422ab             0x00
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04223a             0x00
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04223b             0x03
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_2), 0x03);
	//wriu     0x0422f0             0x00
	mtk_writeb(REG_OFFSET_78_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422f1             0x00
	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04228a             0x01
	mtk_writeb(REG_OFFSET_45_L(BANK_BASE_ALBANY_2), 0x01);
	//wriu     0x04228b             0x00
	mtk_writeb(REG_OFFSET_45_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04213a             0x18
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_1), 0x18);
	//wriu     0x04213b             0x01
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x0421c4             0x44
	mtk_writeb(REG_OFFSET_62_L(BANK_BASE_ALBANY_1), 0x44);
	//wriu     0x0421c5             0x00
	mtk_writeb(REG_OFFSET_62_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x042280             0x30
	mtk_writeb(REG_OFFSET_40_L(BANK_BASE_ALBANY_2), 0x30);
	//wriu     0x042281             0x00
	mtk_writeb(REG_OFFSET_40_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422c4             0x07
	mtk_writeb(REG_OFFSET_62_L(BANK_BASE_ALBANY_2), 0x07);
	//wriu     0x0422c5             0x00
	mtk_writeb(REG_OFFSET_62_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x042230             0x43
	mtk_writeb(REG_OFFSET_18_L(BANK_BASE_ALBANY_2), 0x43);
	//wriu     0x042231             0x00
	mtk_writeb(REG_OFFSET_18_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x042238             0x00
	mtk_writeb(REG_OFFSET_1C_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x042239             0x41
	mtk_writeb(REG_OFFSET_1C_H(BANK_BASE_ALBANY_2), 0x41);
	//wriu     0x0422f2             0xf5
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), 0xf5);
	//wriu     0x0422f3             0x0f
	mtk_writeb(REG_OFFSET_79_H(BANK_BASE_ALBANY_2), 0x0f);
	//wriu     0x0422f2             0xf5
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), 0xf5);
	//wriu     0x0422f3             0x0d
	mtk_writeb(REG_OFFSET_79_H(BANK_BASE_ALBANY_2), 0x0d);
	//wriu     0x042078             0x0c
	mtk_writeb(REG_OFFSET_3C_L(BANK_BASE_ALBANY_0), 0x0c);
	//wriu     0x042079             0xd0
	mtk_writeb(REG_OFFSET_3C_H(BANK_BASE_ALBANY_0), 0xd0);
	//wriu     0x042076             0xa6
	mtk_writeb(REG_OFFSET_3B_L(BANK_BASE_ALBANY_0), 0xa6);
	//wriu     0x042077             0x5a
	mtk_writeb(REG_OFFSET_3B_H(BANK_BASE_ALBANY_0), 0x5a);
	//wriu     0x04202c             0x00
	mtk_writeb(REG_OFFSET_16_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04202d             0x7c
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x7c);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0422e9             0x40
	mtk_writeb(REG_OFFSET_74_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x04202a             0x00
	mtk_writeb(REG_OFFSET_15_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04202b             0x00
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422e9             0x40
	mtk_writeb(REG_OFFSET_74_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x04202a             0x00
	mtk_writeb(REG_OFFSET_15_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04202b             0x00
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0422e9             0x40
	mtk_writeb(REG_OFFSET_74_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ab             0x20
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), 0x20);
	//wriu     0x0420ac             0x19
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ad             0x19
	mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ae             0x19
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420af             0x19
	mtk_writeb(REG_OFFSET_57_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422e9             0x40
	mtk_writeb(REG_OFFSET_74_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ab             0x28
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), 0x28);
	//wriu     0x0421f4             0x01
	mtk_writeb(REG_OFFSET_7A_L(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x0421f5             0x02
	mtk_writeb(REG_OFFSET_7A_H(BANK_BASE_ALBANY_1), 0x02);
	//wriu     0x04210e             0x06
	mtk_writeb(REG_OFFSET_07_L(BANK_BASE_ALBANY_1), 0x06);
	//wriu     0x04210f             0xc9
	mtk_writeb(REG_OFFSET_07_H(BANK_BASE_ALBANY_1), 0xc9);
	//wriu     0x042188             0x00
	mtk_writeb(REG_OFFSET_44_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x042189             0x50
	mtk_writeb(REG_OFFSET_44_H(BANK_BASE_ALBANY_1), 0x50);
	//wriu     0x04218a             0x00
	mtk_writeb(REG_OFFSET_45_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x04218b             0x80
	mtk_writeb(REG_OFFSET_45_H(BANK_BASE_ALBANY_1), 0x80);
	//wriu     0x04218e             0x0e
	mtk_writeb(REG_OFFSET_47_L(BANK_BASE_ALBANY_1), 0x0e);
	//wriu     0x04218f             0x00
	mtk_writeb(REG_OFFSET_47_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x042190             0x04
	mtk_writeb(REG_OFFSET_48_L(BANK_BASE_ALBANY_1), 0x04);
	//wriu     0x042191             0x00
	mtk_writeb(REG_OFFSET_48_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x04206e             0x7f
	mtk_writeb(REG_OFFSET_37_L(BANK_BASE_ALBANY_0), 0x7f);
	//wriu     0x04206f             0xff
	mtk_writeb(REG_OFFSET_37_H(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04202c             0x00
	mtk_writeb(REG_OFFSET_16_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04202d             0x4c
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x4c);
	//wriu     0x042000             0x00
	mtk_writeb(REG_OFFSET_00_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042001             0xc0
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0xc0);
	//wriu     0x042000             0x00
	mtk_writeb(REG_OFFSET_00_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042001             0x40
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x40);
	//wriu     0x010960             0x00
	mtk_writeb(REG_OFFSET_30_L(BANK_BASE_PM_MISC), 0x00);
	//wriu     0x010961             0x00
	mtk_writeb(REG_OFFSET_30_H(BANK_BASE_PM_MISC), 0x00);
	//wriu     0x100260             0x00
	mtk_writeb(REG_OFFSET_30_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x100261             0x00
	mtk_writeb(REG_OFFSET_30_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x100266             0x01
	mtk_writeb(REG_OFFSET_33_L(BANK_BASE_MPLL), 0x01);
	//wriu     0x100267             0x12
	mtk_writeb(REG_OFFSET_33_H(BANK_BASE_MPLL), 0x12);
	//wriu     0x10026a             0x03
	mtk_writeb(REG_OFFSET_35_L(BANK_BASE_MPLL), 0x03);
	//wriu     0x10026b             0x00
	mtk_writeb(REG_OFFSET_35_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x100286             0x00
	mtk_writeb(REG_OFFSET_43_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x100287             0x00
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x1002de             0x00
	mtk_writeb(REG_OFFSET_6F_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x1002df             0x00
	mtk_writeb(REG_OFFSET_6F_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x1002f4             0x00
	mtk_writeb(REG_OFFSET_7A_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x1002f5             0x00
	mtk_writeb(REG_OFFSET_7A_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x100264             0x00
	mtk_writeb(REG_OFFSET_32_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x100265             0x11
	mtk_writeb(REG_OFFSET_32_H(BANK_BASE_MPLL), 0x11);
	//wriu     0x100234             0x10
	mtk_writeb(REG_OFFSET_1A_L(BANK_BASE_MPLL), 0x10);
	//wriu     0x100235             0x00
	mtk_writeb(REG_OFFSET_1A_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x042366             0x36
	mtk_writeb(REG_OFFSET_33_L(BANK_BASE_ALBANY_3), 0x36);
	//wriu     0x042367             0x00
	mtk_writeb(REG_OFFSET_33_H(BANK_BASE_ALBANY_3), 0x00);
	//wriu     0x042364             0x0c
	mtk_writeb(REG_OFFSET_32_L(BANK_BASE_ALBANY_3), 0x0c);
	//wriu     0x042365             0x20
	mtk_writeb(REG_OFFSET_32_H(BANK_BASE_ALBANY_3), 0x20);
	//wriu     0x000c6a             0x04
	mtk_writeb(REG_OFFSET_35_L(BANK_BASE_CLKGEN_PM_0), 0x04);
	//wriu     0x000c6b             0x00
	mtk_writeb(REG_OFFSET_35_H(BANK_BASE_CLKGEN_PM_0), 0x00);
	//wriu     0x000c6c             0x04
	mtk_writeb(REG_OFFSET_36_L(BANK_BASE_CLKGEN_PM_0), 0x04);
	//wriu     0x000c6d             0x00
	mtk_writeb(REG_OFFSET_36_H(BANK_BASE_CLKGEN_PM_0), 0x00);
	//wriu     0x380800             0x03
	//wriu     0x380801             0xf0
	//wriu     0x380802             0x00
	//wriu     0x380803             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_0000_X32_OGMAC1, 0xf003);
	//wriu     0x380804             0x08
	//wriu     0x380805             0x00
	//wriu     0x380806             0x00
	//wriu     0x380807             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_0004_X32_OGMAC1, 0x0008);
	//wriu     0x380808             0x01
	//wriu     0x380809             0x04
	//wriu     0x38080a             0x00
	//wriu     0x38080b             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x0401);
	//wriu     0x38086c             0x85
	//wriu     0x38086d             0x72
	//wriu     0x38086e             0x00
	//wriu     0x38086f             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_006C_X32_OGMAC1, 0x7285);
	//wriu     0x380874             0x00
	//wriu     0x380875             0x00
	//wriu     0x380876             0x00
	//wriu     0x380877             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_0074_X32_OGMAC1, 0x0);

	//wriu     0x380608             0x13
	//wriu     0x380609             0x08
	//wriu     0x38060a             0x00
	//wriu     0x38060b             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0, 0x0813);

	//wriu     0x380600             0x0c
	//wriu     0x380601             0x00
	//wriu     0x380602             0x00
	//wriu     0x380603             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0, 0x000c);

	//wriu     0x380650             0xff
	//wriu     0x380651             0xff
	//wriu     0x380652             0x00
	//wriu     0x380653             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0050_X32_GMAC0, 0xffff);

	//wriu     0x380730             0x78
	//wriu     0x380731             0x56
	//wriu     0x380732             0x00
	//wriu     0x380733             0x00
	//wriu     0x380734             0x34
	//wriu     0x380735             0x12
	//wriu     0x380736             0x00
	//wriu     0x380737             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0130_X32_GMAC0, 0x12345678);

	//wriu     0x380738             0x11
	//wriu     0x380739             0x11
	//wriu     0x38073a             0x00
	//wriu     0x38073b             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0138_X32_GMAC0, 0x1111);

	//wriu     0x380750             0xa5
	//wriu     0x380751             0x5a
	//wriu     0x380752             0x00
	//wriu     0x380753             0x00
	//wriu     0x380754             0x0F
	//wriu     0x380755             0x00
	//wriu     0x380756             0x00
	//wriu     0x380757             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0150_X32_GMAC0, 0x000f5aa5);

	//wriu     0x380760             0x40
	//wriu     0x380761             0x20
	//wriu     0x380762             0x00
	//wriu     0x380763             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0160_X32_GMAC0, 0x2040);

	//wriu     0x380e38             0x01
	//wriu     0x380e39             0xe9
	//wriu     0x380e3a             0x00
	//wriu     0x380e3b             0x00
	mtk_writel(BANK_BASE_MAC_4 + OGMAC1_REG_0038_X32_OGMAC1, 0xe901);
	dev_info(priv->dev, "cts-eth: end RGMII_CELLMODE\n");
}

static void _mtk_dtv_gmac_cts_rgmii_cellmode_delay(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "cts-eth: start RGMII_CELLMODE_DELAY\n");
	//wriu     0x120322             0x2a
	mtk_writeb(REG_OFFSET_11_L(BANK_BASE_TZPC_AID_0), 0x2a);
	//wriu     0x120323             0x00
	mtk_writeb(REG_OFFSET_11_H(BANK_BASE_TZPC_AID_0), 0x00);
	//wriu     0x120324             0x00
	mtk_writeb(REG_OFFSET_12_L(BANK_BASE_TZPC_AID_0), 0x00);
	//wriu     0x120325             0x00
	mtk_writeb(REG_OFFSET_12_H(BANK_BASE_TZPC_AID_0), 0x00);
	//wriu     0x0421b4             0x02
	mtk_writeb(REG_OFFSET_5A_L(BANK_BASE_ALBANY_1), 0x02);
	//wriu     0x0421b5             0x00
	mtk_writeb(REG_OFFSET_5A_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x04214e             0x4c
	mtk_writeb(REG_OFFSET_27_L(BANK_BASE_ALBANY_1), 0x4c);
	//wriu     0x04214f             0x02
	mtk_writeb(REG_OFFSET_27_H(BANK_BASE_ALBANY_1), 0x02);
	//wriu     0x042150             0x60
	mtk_writeb(REG_OFFSET_28_L(BANK_BASE_ALBANY_1), 0x60);
	//wriu     0x042151             0x01
	mtk_writeb(REG_OFFSET_28_H(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x042176             0x00
	mtk_writeb(REG_OFFSET_3B_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x042177             0x18
	mtk_writeb(REG_OFFSET_3B_H(BANK_BASE_ALBANY_1), 0x18);
	//wriu     0x042072             0xa0
	mtk_writeb(REG_OFFSET_39_L(BANK_BASE_ALBANY_0), 0xa0);
	//wriu     0x042073             0x04
	mtk_writeb(REG_OFFSET_39_H(BANK_BASE_ALBANY_0), 0x04);
	//wriu     0x0421fc             0x00
	mtk_writeb(REG_OFFSET_7E_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x0421fd             0x00
	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x0422a0             0x00
	mtk_writeb(REG_OFFSET_50_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422a1             0x80
	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), 0x80);
	//wriu     0x0422fa             0x9c
	mtk_writeb(REG_OFFSET_7D_L(BANK_BASE_ALBANY_2), 0x9c);
	//wriu     0x0422fb             0x02
	mtk_writeb(REG_OFFSET_7D_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x04228e             0x9c
	mtk_writeb(REG_OFFSET_47_L(BANK_BASE_ALBANY_2), 0x9c);
	//wriu     0x04228f             0x02
	mtk_writeb(REG_OFFSET_47_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0422dc             0x9c
	mtk_writeb(REG_OFFSET_6E_L(BANK_BASE_ALBANY_2), 0x9c);
	//wriu     0x0422dd             0x02
	mtk_writeb(REG_OFFSET_6E_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0422d4             0x00
	mtk_writeb(REG_OFFSET_6A_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422d5             0x00
	mtk_writeb(REG_OFFSET_6A_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422d8             0x00
	mtk_writeb(REG_OFFSET_6C_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422d9             0x00
	mtk_writeb(REG_OFFSET_6C_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422f6             0x9c
	mtk_writeb(REG_OFFSET_7B_L(BANK_BASE_ALBANY_2), 0x9c);
	//wriu     0x0422f7             0x02
	mtk_writeb(REG_OFFSET_7B_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0421cc             0x40
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), 0x40);
	//wriu     0x0421cd             0xd8
	mtk_writeb(REG_OFFSET_66_H(BANK_BASE_ALBANY_1), 0xd8);
	//wriu     0x0421ba             0x03
	mtk_writeb(REG_OFFSET_5D_L(BANK_BASE_ALBANY_1), 0x03);
	//wriu     0x0421bb             0x04
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), 0x04);
	//wriu     0x0422aa             0x06
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0422ab             0x00
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04223a             0x00
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04223b             0x03
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_2), 0x03);
	//wriu     0x0422f0             0x00
	mtk_writeb(REG_OFFSET_78_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422f1             0x00
	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04228a             0x01
	mtk_writeb(REG_OFFSET_45_L(BANK_BASE_ALBANY_2), 0x01);
	//wriu     0x04228b             0x00
	mtk_writeb(REG_OFFSET_45_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04213a             0x18
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_1), 0x18);
	//wriu     0x04213b             0x01
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x0421c4             0x44
	mtk_writeb(REG_OFFSET_62_L(BANK_BASE_ALBANY_1), 0x44);
	//wriu     0x0421c5             0x00
	mtk_writeb(REG_OFFSET_62_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x042280             0x30
	mtk_writeb(REG_OFFSET_40_L(BANK_BASE_ALBANY_2), 0x30);
	//wriu     0x042281             0x00
	mtk_writeb(REG_OFFSET_40_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422c4             0x07
	mtk_writeb(REG_OFFSET_62_L(BANK_BASE_ALBANY_2), 0x07);
	//wriu     0x0422c5             0x00
	mtk_writeb(REG_OFFSET_62_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x042230             0x43
	mtk_writeb(REG_OFFSET_18_L(BANK_BASE_ALBANY_2), 0x43);
	//wriu     0x042231             0x00
	mtk_writeb(REG_OFFSET_18_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x042238             0x00
	mtk_writeb(REG_OFFSET_1C_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x042239             0x41
	mtk_writeb(REG_OFFSET_1C_H(BANK_BASE_ALBANY_2), 0x41);
	//wriu     0x0422f2             0xf5
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), 0xf5);
	//wriu     0x0422f3             0x0f
	mtk_writeb(REG_OFFSET_79_H(BANK_BASE_ALBANY_2), 0x0f);
	//wriu     0x0422f2             0xf5
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), 0xf5);
	//wriu     0x0422f3             0x0d
	mtk_writeb(REG_OFFSET_79_H(BANK_BASE_ALBANY_2), 0x0d);
	//wriu     0x042078             0x0c
	mtk_writeb(REG_OFFSET_3C_L(BANK_BASE_ALBANY_0), 0x0c);
	//wriu     0x042079             0xd0
	mtk_writeb(REG_OFFSET_3C_H(BANK_BASE_ALBANY_0), 0xd0);
	//wriu     0x042076             0xa6
	mtk_writeb(REG_OFFSET_3B_L(BANK_BASE_ALBANY_0), 0xa6);
	//wriu     0x042077             0x5a
	mtk_writeb(REG_OFFSET_3B_H(BANK_BASE_ALBANY_0), 0x5a);
	//wriu     0x04202c             0x00
	mtk_writeb(REG_OFFSET_16_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04202d             0x7c
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x7c);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0422e9             0x40
	mtk_writeb(REG_OFFSET_74_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x04202a             0x00
	mtk_writeb(REG_OFFSET_15_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04202b             0x00
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422e9             0x40
	mtk_writeb(REG_OFFSET_74_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x04202a             0x00
	mtk_writeb(REG_OFFSET_15_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04202b             0x00
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0422e9             0x40
	mtk_writeb(REG_OFFSET_74_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ab             0x20
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), 0x20);
	//wriu     0x0420ac             0x19
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ad             0x19
	mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ae             0x19
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420af             0x19
	mtk_writeb(REG_OFFSET_57_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422e9             0x40
	mtk_writeb(REG_OFFSET_74_H(BANK_BASE_ALBANY_2), 0x40);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ab             0x28
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), 0x28);
	//wriu     0x0421f4             0x01
	mtk_writeb(REG_OFFSET_7A_L(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x0421f5             0x02
	mtk_writeb(REG_OFFSET_7A_H(BANK_BASE_ALBANY_1), 0x02);
	//wriu     0x04210e             0x06
	mtk_writeb(REG_OFFSET_07_L(BANK_BASE_ALBANY_1), 0x06);
	//wriu     0x04210f             0xc9
	mtk_writeb(REG_OFFSET_07_H(BANK_BASE_ALBANY_1), 0xc9);
	//wriu     0x042188             0x00
	mtk_writeb(REG_OFFSET_44_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x042189             0x50
	mtk_writeb(REG_OFFSET_44_H(BANK_BASE_ALBANY_1), 0x50);
	//wriu     0x04218a             0x00
	mtk_writeb(REG_OFFSET_45_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x04218b             0x80
	mtk_writeb(REG_OFFSET_45_H(BANK_BASE_ALBANY_1), 0x80);
	//wriu     0x04218e             0x0e
	mtk_writeb(REG_OFFSET_47_L(BANK_BASE_ALBANY_1), 0x0e);
	//wriu     0x04218f             0x00
	mtk_writeb(REG_OFFSET_47_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x042190             0x04
	mtk_writeb(REG_OFFSET_48_L(BANK_BASE_ALBANY_1), 0x04);
	//wriu     0x042191             0x00
	mtk_writeb(REG_OFFSET_48_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x04206e             0x7f
	mtk_writeb(REG_OFFSET_37_L(BANK_BASE_ALBANY_0), 0x7f);
	//wriu     0x04206f             0xff
	mtk_writeb(REG_OFFSET_37_H(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04202c             0x00
	mtk_writeb(REG_OFFSET_16_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04202d             0x4c
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x4c);
	//wriu     0x042000             0x00
	mtk_writeb(REG_OFFSET_00_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042001             0xc0
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0xc0);
	//wriu     0x042000             0x00
	mtk_writeb(REG_OFFSET_00_L(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042001             0x40
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x40);
	//wriu     0x010960             0x00
	mtk_writeb(REG_OFFSET_30_L(BANK_BASE_PM_MISC), 0x00);
	//wriu     0x010961             0x00
	mtk_writeb(REG_OFFSET_30_H(BANK_BASE_PM_MISC), 0x00);
	//wriu     0x100260             0x00
	mtk_writeb(REG_OFFSET_30_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x100261             0x00
	mtk_writeb(REG_OFFSET_30_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x100266             0x01
	mtk_writeb(REG_OFFSET_33_L(BANK_BASE_MPLL), 0x01);
	//wriu     0x100267             0x12
	mtk_writeb(REG_OFFSET_33_H(BANK_BASE_MPLL), 0x12);
	//wriu     0x10026a             0x03
	mtk_writeb(REG_OFFSET_35_L(BANK_BASE_MPLL), 0x03);
	//wriu     0x10026b             0x00
	mtk_writeb(REG_OFFSET_35_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x100286             0x00
	mtk_writeb(REG_OFFSET_43_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x100287             0x00
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x1002de             0x00
	mtk_writeb(REG_OFFSET_6F_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x1002df             0x00
	mtk_writeb(REG_OFFSET_6F_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x1002f4             0x00
	mtk_writeb(REG_OFFSET_7A_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x1002f5             0x00
	mtk_writeb(REG_OFFSET_7A_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x100264             0x00
	mtk_writeb(REG_OFFSET_32_L(BANK_BASE_MPLL), 0x00);
	//wriu     0x100265             0x11
	mtk_writeb(REG_OFFSET_32_H(BANK_BASE_MPLL), 0x11);
	//wriu     0x100234             0x10
	mtk_writeb(REG_OFFSET_1A_L(BANK_BASE_MPLL), 0x10);
	//wriu     0x100235             0x00
	mtk_writeb(REG_OFFSET_1A_H(BANK_BASE_MPLL), 0x00);
	//wriu     0x042366             0x36
	mtk_writeb(REG_OFFSET_33_L(BANK_BASE_ALBANY_3), 0x3e);
	//wriu     0x042367             0x00
	mtk_writeb(REG_OFFSET_33_H(BANK_BASE_ALBANY_3), 0x00);
	//wriu     0x042364             0x0c
	mtk_writeb(REG_OFFSET_32_L(BANK_BASE_ALBANY_3), 0x0c);
	//wriu     0x042365             0x20
	mtk_writeb(REG_OFFSET_32_H(BANK_BASE_ALBANY_3), 0x20);
	//wriu     0x000c6a             0x04
	mtk_writeb(REG_OFFSET_35_L(BANK_BASE_CLKGEN_PM_0), 0x04);
	//wriu     0x000c6b             0x00
	mtk_writeb(REG_OFFSET_35_H(BANK_BASE_CLKGEN_PM_0), 0x00);
	//wriu     0x000c6c             0x04
	mtk_writeb(REG_OFFSET_36_L(BANK_BASE_CLKGEN_PM_0), 0x04);
	//wriu     0x000c6d             0x00
	mtk_writeb(REG_OFFSET_36_H(BANK_BASE_CLKGEN_PM_0), 0x00);
	//wriu     0x380800             0x03
	//wriu     0x380801             0xf0
	//wriu     0x380802             0x00
	//wriu     0x380803             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_0000_X32_OGMAC1, 0xf003);
	//wriu     0x380804             0x08
	//wriu     0x380805             0x00
	//wriu     0x380806             0x00
	//wriu     0x380807             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_0004_X32_OGMAC1, 0x0008);
	//wriu     0x380808             0x01
	//wriu     0x380809             0x04
	//wriu     0x38080a             0x00
	//wriu     0x38080b             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_0008_X32_OGMAC1, 0x0401);
	//wriu     0x38086c             0x85
	//wriu     0x38086d             0x72
	//wriu     0x38086e             0x00
	//wriu     0x38086f             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_006C_X32_OGMAC1, 0x7285);
	//wriu     0x380874             0x00
	//wriu     0x380875             0x00
	//wriu     0x380876             0x00
	//wriu     0x380877             0x00
	mtk_writel(BANK_BASE_MAC_1 + OGMAC1_REG_0074_X32_OGMAC1, 0x0);

	//wriu     0x380608             0x13
	//wriu     0x380609             0x08
	//wriu     0x38060a             0x00
	//wriu     0x38060b             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0008_X32_GMAC0, 0x0813);

	//wriu     0x380600             0x0c
	//wriu     0x380601             0x00
	//wriu     0x380602             0x00
	//wriu     0x380603             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0000_X32_GMAC0, 0x000c);

	//wriu     0x380650             0xff
	//wriu     0x380651             0xff
	//wriu     0x380652             0x00
	//wriu     0x380653             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0050_X32_GMAC0, 0xffff);

	//wriu     0x380730             0x78
	//wriu     0x380731             0x56
	//wriu     0x380732             0x00
	//wriu     0x380733             0x00
	//wriu     0x380734             0x34
	//wriu     0x380735             0x12
	//wriu     0x380736             0x00
	//wriu     0x380737             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0130_X32_GMAC0, 0x12345678);

	//wriu     0x380738             0x11
	//wriu     0x380739             0x11
	//wriu     0x38073a             0x00
	//wriu     0x38073b             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0138_X32_GMAC0, 0x1111);

	//wriu     0x380750             0xa5
	//wriu     0x380751             0x5a
	//wriu     0x380752             0x00
	//wriu     0x380753             0x00
	//wriu     0x380754             0x0F
	//wriu     0x380755             0x0F
	//wriu     0x380756             0x00
	//wriu     0x380757             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0150_X32_GMAC0, 0x0f0f5aa5);

	//wriu     0x380760             0x40
	//wriu     0x380761             0x20
	//wriu     0x380762             0x00
	//wriu     0x380763             0x00
	mtk_writel(BANK_BASE_MAC_0 + GMAC0_REG_0160_X32_GMAC0, 0x2040);

	//wriu     0x380e38             0x01
	//wriu     0x380e39             0xe9
	//wriu     0x380e3a             0x00
	//wriu     0x380e3b             0x00
	mtk_writel(BANK_BASE_MAC_4 + OGMAC1_REG_0038_X32_OGMAC1, 0xe901);
	dev_info(priv->dev, "cts-eth: end RGMII_CELLMODE_DELAY\n");
}

static void _mtk_dtv_gmac_cts_tx_100t_random(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "cts-eth: start ETH_TX_100T_RANDOM\n");
	//wriu     0x0420cb             0x90
	mtk_writeb(REG_OFFSET_65_H(BANK_BASE_ALBANY_0), 0x90);
	//wriu     0x0421fd             0x00
	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x0422fb             0x02
	mtk_writeb(REG_OFFSET_7D_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0422a1             0x80
	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), 0x80);
	//wriu     0x0421bb             0x84
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), 0x84);
	//wriu     0x0422a4             0x02
	mtk_writeb(REG_OFFSET_52_L(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0421cc             0x40
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), 0x40);
	//wriu     0x0422d6             0x14
	mtk_writeb(REG_OFFSET_6B_L(BANK_BASE_ALBANY_2), 0x14);
	//wriu     0x0422f2             0x95
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), 0x95);
	//wriu     0x04223a             0xf0
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), 0xf0);
	//wriu     0x0422f1             0x18
	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), 0x18);
	//wriu     0x0422d6             0x94
	mtk_writeb(REG_OFFSET_6B_L(BANK_BASE_ALBANY_2), 0x94);
	//wriu     0x0422aa             0x04
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_2), 0x04);
	//wriu     0x042087             0x14
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x14);
	//wriu     0x0422f8             0x94
	mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), 0x94);
	//wriu     0x0422ac             0x01
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_2), 0x01);
	//wriu     0x0422d9             0x00
	mtk_writeb(REG_OFFSET_6C_H(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0422ae             0x01
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_2), 0x01);
	//wriu     0x042140             0x30
	mtk_writeb(REG_OFFSET_20_L(BANK_BASE_ALBANY_1), 0x30);
	//wriu     0x042087             0x14
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x14);
	//wriu     0x04202d             0x2c
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x2c);
	//wriu     0x042001             0xa1
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0xa1);
	//wriu     0x042001             0x21
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x21);
	//wriu     0x042001             0x21
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x21);
	//wriu     0x042001             0x21
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x21);
	//wriu     0x042087             0x14
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x14);
	//wriu     0x0421d5             0x80
	mtk_writeb(REG_OFFSET_6A_H(BANK_BASE_ALBANY_1), 0x80);
	//wriu     0x04215a             0x00
	mtk_writeb(REG_OFFSET_2D_L(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x04215b             0xc0
	mtk_writeb(REG_OFFSET_2D_H(BANK_BASE_ALBANY_1), 0xc0);
	//wriu     0x042153             0x4f
	mtk_writeb(REG_OFFSET_29_H(BANK_BASE_ALBANY_1), 0x4f);
	//wriu     0x04205d             0x06
	mtk_writeb(REG_OFFSET_2E_H(BANK_BASE_ALBANY_0), 0x06);
	//wriu     0x04205d             0x02
	mtk_writeb(REG_OFFSET_2E_H(BANK_BASE_ALBANY_0), 0x02);
	dev_info(priv->dev, "cts-eth: end ETH_TX_100T_RANDOM\n");
}

static void _mtk_dtv_gmac_cts_tx_10t_ltp(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "cts-eth: start ETH_TX_10T_LTP\n");
	//wriu     0x0420cb             0x90
	mtk_writeb(REG_OFFSET_65_H(BANK_BASE_ALBANY_0), 0x90);
	//wriu     0x0421fd             0x00
	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x0422fb             0x02
	mtk_writeb(REG_OFFSET_7D_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0422a1             0x80
	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), 0x80);
	//wriu     0x0421bb             0x84
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), 0x84);
	//wriu     0x0422a4             0x02
	mtk_writeb(REG_OFFSET_52_L(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0421cc             0x40
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), 0x40);
	//wriu     0x0422f8             0x12
	mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), 0x12);
	//wriu     0x0422f2             0x95
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), 0x95);
	//wriu     0x04223a             0xf0
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), 0xf0);
	//wriu     0x0422f1             0x18
	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), 0x18);
	//wriu     0x0422f8             0x12
	mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), 0x12);
	//wriu     0x0422aa             0x56
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_2), 0x56);
	//wriu     0x042087             0x14
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x14);
	//wriu     0x04213b             0x01
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x04202b             0x00
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04202b             0x00
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ac             0x19
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ad             0x19
	mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ae             0x19
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420af             0x19
	mtk_writeb(REG_OFFSET_57_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ab             0x28
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), 0x28);
	//wriu     0x0422ac             0x08
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_2), 0x08);
	//wriu     0x04206f             0x7f
	mtk_writeb(REG_OFFSET_37_H(BANK_BASE_ALBANY_0), 0x7f);
	//wriu     0x04202d             0x0c
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x0c);
	//wriu     0x042230             0x41
	mtk_writeb(REG_OFFSET_18_L(BANK_BASE_ALBANY_2), 0x41);
	//wriu     0x0420eb             0x29
	mtk_writeb(REG_OFFSET_75_H(BANK_BASE_ALBANY_0), 0x29);
	//wriu     0x04202d             0x04
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x04);
	//wriu     0x042001             0x01
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x01);
	//wriu     0x042001             0x81
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x81);
	//wriu     0x042001             0x01
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x01);
	//wriu     0x042140             0x30
	mtk_writeb(REG_OFFSET_20_L(BANK_BASE_ALBANY_1), 0x30);
	//wriu     0x042087             0x00
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042040             0xd2
	mtk_writeb(REG_OFFSET_20_L(BANK_BASE_ALBANY_0), 0xd2);
	//wriu     0x042041             0x04
	mtk_writeb(REG_OFFSET_20_H(BANK_BASE_ALBANY_0), 0x04);
	//wriu     0x04205c             0x01
	mtk_writeb(REG_OFFSET_2E_L(BANK_BASE_ALBANY_0), 0x01);
	dev_info(priv->dev, "cts-eth: end ETH_TX_10T_LTP\n");
}

static void _mtk_dtv_gmac_cts_tx_10t_tp_idl(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "cts-eth: start ETH_TX_10T_TP_IDL\n");
	//wriu     0x0420cb             0x90
	mtk_writeb(REG_OFFSET_65_H(BANK_BASE_ALBANY_0), 0x90);
	//wriu     0x0421fd             0x00
	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x0422fb             0x02
	mtk_writeb(REG_OFFSET_7D_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0422a1             0x80
	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), 0x80);
	//wriu     0x0421bb             0x84
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), 0x84);
	//wriu     0x0422a4             0x02
	mtk_writeb(REG_OFFSET_52_L(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0421cc             0x40
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), 0x40);
	//wriu     0x0422f8             0x12
	mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), 0x12);
	//wriu     0x0422f2             0x95
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), 0x95);
	//wriu     0x04223a             0xf0
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), 0xf0);
	//wriu     0x0422f1             0x18
	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), 0x18);
	//wriu     0x0422f8             0x12
	mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), 0x12);
	//wriu     0x0422aa             0x56
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_2), 0x56);
	//wriu     0x042087             0x14
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x14);
	//wriu     0x04213b             0x01
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ac             0x19
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ad             0x19
	mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ae             0x19
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420af             0x19
	mtk_writeb(REG_OFFSET_57_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ab             0x28
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), 0x28);
	//wriu     0x0422ac             0x08
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_2), 0x08);
	//wriu     0x042087             0x14
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x14);
	//wriu     0x04202d             0x04
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x04);
	//wriu     0x042080             0xff
	mtk_writeb(REG_OFFSET_40_L(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207e             0xff
	mtk_writeb(REG_OFFSET_3F_L(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207f             0x00
	mtk_writeb(REG_OFFSET_3F_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04207a             0xff
	mtk_writeb(REG_OFFSET_3D_L(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207b             0xff
	mtk_writeb(REG_OFFSET_3D_H(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207c             0xff
	mtk_writeb(REG_OFFSET_3E_L(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207d             0xff
	mtk_writeb(REG_OFFSET_3E_H(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207d             0x00
	mtk_writeb(REG_OFFSET_3E_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x04207f             0xff
	mtk_writeb(REG_OFFSET_3F_H(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x042001             0x00
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042001             0x80
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x80);
	//wriu     0x042001             0x80
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x80);
	//wriu     0x042001             0x80
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x80);
	//wriu     0x042001             0x00
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042073             0x0c
	mtk_writeb(REG_OFFSET_39_H(BANK_BASE_ALBANY_0), 0x0c);
	//wriu     0x042040             0xd2
	mtk_writeb(REG_OFFSET_20_L(BANK_BASE_ALBANY_0), 0xd2);
	dev_info(priv->dev, "cts-eth: end ETH_TX_10T_TP_IDL\n");
}

static void _mtk_dtv_gmac_cts_tx_10t_all_one(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	dev_info(priv->dev, "cts-eth: start ETH_TX_10T_ALL_ONE\n");
	//wriu     0x0420cb             0x90
	mtk_writeb(REG_OFFSET_65_H(BANK_BASE_ALBANY_0), 0x90);
	//wriu     0x0421fd             0x00
	mtk_writeb(REG_OFFSET_7E_H(BANK_BASE_ALBANY_1), 0x00);
	//wriu     0x0422fb             0x02
	mtk_writeb(REG_OFFSET_7D_H(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0422a1             0x80
	mtk_writeb(REG_OFFSET_50_H(BANK_BASE_ALBANY_2), 0x80);
	//wriu     0x0421bb             0x84
	mtk_writeb(REG_OFFSET_5D_H(BANK_BASE_ALBANY_1), 0x84);
	//wriu     0x0422a4             0x02
	mtk_writeb(REG_OFFSET_52_L(BANK_BASE_ALBANY_2), 0x02);
	//wriu     0x0421cc             0x40
	mtk_writeb(REG_OFFSET_66_L(BANK_BASE_ALBANY_1), 0x40);
	//wriu     0x0422f8             0x12
	mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), 0x12);
	//wriu     0x0422f2             0x95
	mtk_writeb(REG_OFFSET_79_L(BANK_BASE_ALBANY_2), 0x95);
	//wriu     0x04223a             0xf0
	mtk_writeb(REG_OFFSET_1D_L(BANK_BASE_ALBANY_2), 0xf0);
	//wriu     0x0422f1             0x18
	mtk_writeb(REG_OFFSET_78_H(BANK_BASE_ALBANY_2), 0x18);
	//wriu     0x0422f8             0x12
	mtk_writeb(REG_OFFSET_7C_L(BANK_BASE_ALBANY_2), 0x12);
	//wriu     0x0422aa             0x56
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_2), 0x56);
	//wriu     0x042087             0x14
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x14);
	//wriu     0x04213b             0x01
	mtk_writeb(REG_OFFSET_1D_H(BANK_BASE_ALBANY_1), 0x01);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x04202b             0x00
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x04202b             0x00
	mtk_writeb(REG_OFFSET_15_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x0422e8             0x06
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x06);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ac             0x19
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ad             0x19
	mtk_writeb(REG_OFFSET_56_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ae             0x19
	mtk_writeb(REG_OFFSET_57_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420af             0x19
	mtk_writeb(REG_OFFSET_57_H(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0422e8             0x00
	mtk_writeb(REG_OFFSET_74_L(BANK_BASE_ALBANY_2), 0x00);
	//wriu     0x0420aa             0x19
	mtk_writeb(REG_OFFSET_55_L(BANK_BASE_ALBANY_0), 0x19);
	//wriu     0x0420ab             0x28
	mtk_writeb(REG_OFFSET_55_H(BANK_BASE_ALBANY_0), 0x28);
	//wriu     0x0422ac             0x08
	mtk_writeb(REG_OFFSET_56_L(BANK_BASE_ALBANY_2), 0x08);
	//wriu     0x042087             0x14
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), 0x14);
	//wriu     0x04202d             0x04
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), 0x04);
	//wriu     0x042080             0xff
	mtk_writeb(REG_OFFSET_40_L(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207e             0xff
	mtk_writeb(REG_OFFSET_3F_L(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207f             0x1e
	mtk_writeb(REG_OFFSET_3F_H(BANK_BASE_ALBANY_0), 0x1e);
	//wriu     0x04207a             0xff
	mtk_writeb(REG_OFFSET_3D_L(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207b             0xff
	mtk_writeb(REG_OFFSET_3D_H(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207c             0xff
	mtk_writeb(REG_OFFSET_3E_L(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207d             0xff
	mtk_writeb(REG_OFFSET_3E_H(BANK_BASE_ALBANY_0), 0xff);
	//wriu     0x04207d             0x00
	mtk_writeb(REG_OFFSET_3E_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042001             0x00
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042001             0x80
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x80);
	//wriu     0x042001             0x80
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x80);
	//wriu     0x042001             0x80
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x80);
	//wriu     0x042001             0x00
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), 0x00);
	//wriu     0x042073             0x0c
	mtk_writeb(REG_OFFSET_39_H(BANK_BASE_ALBANY_0), 0x0c);
	//wriu     0x042040             0xd2
	mtk_writeb(REG_OFFSET_20_L(BANK_BASE_ALBANY_0), 0xd2);
	dev_info(priv->dev, "cts-eth: end ETH_TX_10T_ALL_ONE\n");
}

static void _mtk_dtv_gmac_cts_force_10t(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	u8 reg_read;

	dev_info(priv->dev, "cts-eth: start force 10T\n");
	//wriu -b 0x042073 0x08 0x00 // dis lpbk
	reg_read = mtk_readb(REG_OFFSET_39_L(BANK_BASE_ALBANY_0));
	reg_read &= ~(0x08);
	mtk_writeb(REG_OFFSET_39_L(BANK_BASE_ALBANY_0), reg_read);
	//wriu -b 0x042087 0x04 0x00 // ac en => set to 0
	reg_read = mtk_readb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0));
	reg_read &= ~(0x04);
	mtk_writeb(REG_OFFSET_43_H(BANK_BASE_ALBANY_0), reg_read);
	//wriu -b 0x04202D 0x38 0x00 // force 10t
	reg_read = mtk_readb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0));
	reg_read &= ~(0x38);
	mtk_writeb(REG_OFFSET_16_H(BANK_BASE_ALBANY_0), reg_read);
	//wriu -b 0x0420EB 0x80 0x00 // disable ldps
	reg_read = mtk_readb(REG_OFFSET_75_H(BANK_BASE_ALBANY_0));
	reg_read &= ~(0x80);
	mtk_writeb(REG_OFFSET_75_H(BANK_BASE_ALBANY_0), reg_read);
	//wriu -b 0x042001 0x80 0x00
	reg_read = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0));
	reg_read &= ~(0x80);
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), reg_read);
	//wriu -b 0x042001 0x80 0x80
	reg_read = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0));
	reg_read |= (0x80);
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), reg_read);
	//wriu -b 0x042001 0x80 0x00
	reg_read = mtk_readb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0));
	reg_read &= ~(0x80);
	mtk_writeb(REG_OFFSET_00_H(BANK_BASE_ALBANY_0), reg_read);
	//wriu -b 0x042040 0x80 0x80
	reg_read = mtk_readb(REG_OFFSET_20_L(BANK_BASE_ALBANY_0));
	reg_read |= (0x80);
	mtk_writeb(REG_OFFSET_20_L(BANK_BASE_ALBANY_0), reg_read);
}

static void _mtk_dtv_gmac_cts_1000_tx_template_mode_1(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 1000 tx template mode 1\n");

	/* W, PHY addr1, Reg 20=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x001F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x001f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 14=0x031B */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x031b);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x401F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1 ,Reg 14=0x0100 */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x0100);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 9=0x2700 */
	reg_ops->write_phy(ndev, priv->phy_addr, 9, 0x2700);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 9);
	dev_info(priv->dev, "cts-eth: Reg 9=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_1000_tx_template_mode_2(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 1000 tx template mode 2\n");

	/* W, PHY addr1, Reg 20=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x001F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x001f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 14=0x031B */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x031b);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x401F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1 ,Reg 14=0x0100 */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x0100);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 9=0x4700 */
	reg_ops->write_phy(ndev, priv->phy_addr, 9, 0x4700);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 9);
	dev_info(priv->dev, "cts-eth: Reg 9=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_1000_tx_template_mode_3(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 1000 tx template mode 3\n");

	/* W, PHY addr1, Reg 20=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x001F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x001f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 14=0x031B */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x031b);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x401F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1 ,Reg 14=0x0100 */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x0100);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 9=0x6700 */
	reg_ops->write_phy(ndev, priv->phy_addr, 9, 0x6700);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 9);
	dev_info(priv->dev, "cts-eth: Reg 9=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_1000_tx_template_mode_4(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 1000 tx template mode 4\n");

	/* W, PHY addr1, Reg 20=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x001F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x001f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 14=0x031B */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x031b);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x401F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1 ,Reg 14=0x0100 */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x0100);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 9=0x8700 */
	reg_ops->write_phy(ndev, priv->phy_addr, 9, 0x8700);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 9);
	dev_info(priv->dev, "cts-eth: Reg 9=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_100_tx_template(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 100 tx template\n");

	/* W, PHY addr1, Reg 20=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x001F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x001f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 14=0x031B */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x031b);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x401F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1 ,Reg 14=0x0100 */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x0100);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 0=0x2100 */
	reg_ops->write_phy(ndev, priv->phy_addr, 0, 0x2100);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 0);
	dev_info(priv->dev, "cts-eth: Reg 0=0x%x\n", reg);
	/* W, PHY addr1, Reg 16=0x3B08 */
	reg_ops->write_phy(ndev, priv->phy_addr, 16, 0x3b08);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 16);
	dev_info(priv->dev, "cts-eth: Reg 16=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_10_tx_template_init(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 10 tx template init\n");

	/* W, PHY addr1, Reg 20=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x001F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x001f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 14=0x031B */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x031b);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x401F */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x401f);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1 ,Reg 14=0x0100 */
	reg_ops->write_phy(ndev, priv->phy_addr, 14, 0x0100);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 14);
	dev_info(priv->dev, "cts-eth: Reg 14=0x%x\n", reg);
	/* W, PHY addr1, Reg 13=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 13, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 13);
	dev_info(priv->dev, "cts-eth: Reg 13=0x%x\n", reg);
	/* W, PHY addr1, Reg 0=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 0, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 0);
	dev_info(priv->dev, "cts-eth: Reg 0=0x%x\n", reg);
	/* W, PHY addr1, Reg 16=0x3B08 */
	reg_ops->write_phy(ndev, priv->phy_addr, 16, 0x3b08);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 16);
	dev_info(priv->dev, "cts-eth: Reg 16=0x%x\n", reg);
	/* W, PHY addr1, Reg 20=0x0002 */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x0002);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 18=0x0018 */
	reg_ops->write_phy(ndev, priv->phy_addr, 18, 0x0018);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 18);
	dev_info(priv->dev, "cts-eth: Reg 18=0x%x\n", reg);
	/* W, PHY addr1, Reg 20=0x000C */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x000c);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 16=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 16, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 16);
	dev_info(priv->dev, "cts-eth: Reg 16=0x%x\n", reg);
	/* W, PHY addr1, Reg 16=0x0100 */
	reg_ops->write_phy(ndev, priv->phy_addr, 16, 0x0100);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 16);
	dev_info(priv->dev, "cts-eth: Reg 16=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_10_tx_template_case_1(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 10 tx template case 1\n");
	dev_info(priv->dev, "cts-eth: Case 1: Packet=random, Length=random settings\n");

	/* W, PHY addr1, Reg 17=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 17, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 17);
	dev_info(priv->dev, "cts-eth: Reg 17=0x%x\n", reg);
	/* W, PHY addr1, Reg 16=0x9000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 16, 0x9000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 16);
	dev_info(priv->dev, "cts-eth: Reg 16=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_10_tx_template_case_2(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 10 tx template case 2\n");
	dev_info(priv->dev, "cts-eth: Case 2: Packet= all-zero, Length=random settings\n");

	/* W, PHY addr1, Reg 17=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 17, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 17);
	dev_info(priv->dev, "cts-eth: Reg 17=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0x000C */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0x000c);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 16=0x9026 */
	reg_ops->write_phy(ndev, priv->phy_addr, 16, 0x9026);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 16);
	dev_info(priv->dev, "cts-eth: Reg 16=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_10_tx_template_case_3(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 10 tx template case 3\n");
	dev_info(priv->dev, "cts-eth: Case 3: Packet= all-one, Length=random settings\n");

	/* W, PHY addr1, Reg 17=0x0000 */
	reg_ops->write_phy(ndev, priv->phy_addr, 17, 0x0000);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 17);
	dev_info(priv->dev, "cts-eth: Reg 17=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0xFF0C */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0xff0c);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 20=0x000D */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x000d);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 18=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 18, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 18);
	dev_info(priv->dev, "cts-eth: Reg 18=0x%x\n", reg);
	/* W, PHY addr1, Reg 18=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 18, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 18);
	dev_info(priv->dev, "cts-eth: Reg 18=0x%x\n", reg);
	/* W, PHY addr1, Reg 18=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 18, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 18);
	dev_info(priv->dev, "cts-eth: Reg 18=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 20=0x000C */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x000c);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 16=0x9026 */
	reg_ops->write_phy(ndev, priv->phy_addr, 16, 0x9026);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 16);
	dev_info(priv->dev, "cts-eth: Reg 16=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_10_tx_template_case_4(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	u32 reg;

	dev_info(priv->dev, "cts-eth: IC Plus 10 tx template case 4\n");
	dev_info(priv->dev, "cts-eth: Case 4: Packet= all-one, Length=1514 Byte settings\n");

	/* W, PHY addr1, Reg 17=0x05EA */
	reg_ops->write_phy(ndev, priv->phy_addr, 17, 0x05ea);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 17);
	dev_info(priv->dev, "cts-eth: Reg 17=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0xFF0C */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0xff0c);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 20=0x000D */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x000d);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 18=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 18, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 18);
	dev_info(priv->dev, "cts-eth: Reg 18=0x%x\n", reg);
	/* W, PHY addr1, Reg 18=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 18, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 18);
	dev_info(priv->dev, "cts-eth: Reg 18=0x%x\n", reg);
	/* W, PHY addr1, Reg 18=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 18, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 18);
	dev_info(priv->dev, "cts-eth: Reg 18=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 19=0xFFFF */
	reg_ops->write_phy(ndev, priv->phy_addr, 19, 0xffff);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 19);
	dev_info(priv->dev, "cts-eth: Reg 19=0x%x\n", reg);
	/* W, PHY addr1, Reg 20=0x000C */
	reg_ops->write_phy(ndev, priv->phy_addr, 20, 0x000c);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 20);
	dev_info(priv->dev, "cts-eth: Reg 20=0x%x\n", reg);
	/* W, PHY addr1, Reg 16=0x9026 */
	reg_ops->write_phy(ndev, priv->phy_addr, 16, 0x9026);
	reg = reg_ops->read_phy(ndev, priv->phy_addr, 16);
	dev_info(priv->dev, "cts-eth: Reg 16=0x%x\n", reg);
}

static void _mtk_dtv_gmac_cts_phy_write(struct net_device *ndev,
					int offset,
					int value)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;

	dev_info(priv->dev, "cts-eth: phy write: offset=0x%x, value=0x%x\n",
		 offset, value);

	reg_ops->write_phy(ndev, priv->phy_addr, offset, value);
}

static void _mtk_dtv_gmac_cts_phy_read(struct net_device *ndev,
				       int offset,
				       int *value)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;

	*value = reg_ops->read_phy(ndev, priv->phy_addr, offset);

	dev_info(priv->dev, "cts-eth: phy read: offset=0x%x, value=0x%x\n",
		 offset, *value);
}

int mtk_dtv_gmac_cts_ioctl(struct net_device *ndev, struct ifreq *req)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_mac_ioctl_cts_para_cmd cts_cmd;

	if (copy_from_user(&cts_cmd, req->ifr_data, sizeof(cts_cmd))) {
		dev_err(priv->dev, "cts-eth: copy from user fail\n");
		return -EFAULT;
	}

	switch (cts_cmd.test_mode) {
	case MTK_MAC_CTS_RANDOM:
		dev_info(priv->dev, "cts-eth: start gen random pkt: len=%d\n",
			 cts_cmd.input);
		_mtk_dtv_gmac_cts_random_tx(ndev, cts_cmd.input);
		netif_start_queue(ndev);
		break;
	case MTK_MAC_CTS_LOOPBACK:
		_mtk_dtv_gmac_cts_loopback(ndev, cts_cmd.input);
		break;
	case MTK_MAC_CTS_PAD_SKEW_READ:
		_mtk_dtv_gmac_cts_pad_skew_read(ndev);
		break;
	case MTK_MAC_CTS_PAD_SKEW_WRITE_RX:
		_mtk_dtv_gmac_cts_pad_skew_write_rx(ndev, cts_cmd.input);
		break;
	case MTK_MAC_CTS_PAD_SKEW_WRITE_TX:
		_mtk_dtv_gmac_cts_pad_skew_write_tx(ndev, cts_cmd.input);
		break;
	case MTK_MAC_CTS_RXC_DLL_DELAY_ENABLE:
		_mtk_dtv_gmac_cts_rxc_dll_delay(ndev, true, cts_cmd.input);
		break;
	case MTK_MAC_CTS_RXC_DLL_DELAY_DISABLE:
		_mtk_dtv_gmac_cts_rxc_dll_delay(ndev, false, cts_cmd.input);
		break;
	case MTK_MAC_CTS_RXC_DLL_DELAY_TUNE:
		_mtk_dtv_gmac_cts_rxc_dll_delay_tune(ndev, cts_cmd.input);
		break;
	case MTK_MAC_CTS_RGMII_IO_DRIVING:
		_mtk_dtv_gmac_cts_rgmii_io_driving(ndev, cts_cmd.input);
		break;
	case MTK_MAC_CTS_RGMII_CELLMODE:
		_mtk_dtv_gmac_cts_rgmii_cellmode(ndev);
		break;
	case MTK_MAC_CTS_RGMII_CELLMODE_DELAY:
		_mtk_dtv_gmac_cts_rgmii_cellmode_delay(ndev);
		break;
	case MTK_MAC_CTS_TX_100T_RANDOM:
		_mtk_dtv_gmac_cts_tx_100t_random(ndev);
		break;
	case MTK_MAC_CTS_TX_10T_LTP:
		_mtk_dtv_gmac_cts_tx_10t_ltp(ndev);
		break;
	case MTK_MAC_CTS_TX_10T_TP_IDL:
		_mtk_dtv_gmac_cts_tx_10t_tp_idl(ndev);
		break;
	case MTK_MAC_CTS_TX_10T_ALL_ONE:
		_mtk_dtv_gmac_cts_tx_10t_all_one(ndev);
		break;
	case MTK_MAC_CTS_FORCE_10T:
		_mtk_dtv_gmac_cts_force_10t(ndev);
		break;
	case MTK_MAC_CTS_1000_TX_TEMPLATE_MODE_1:
		_mtk_dtv_gmac_cts_1000_tx_template_mode_1(ndev);
		break;
	case MTK_MAC_CTS_1000_TX_TEMPLATE_MODE_2:
		_mtk_dtv_gmac_cts_1000_tx_template_mode_2(ndev);
		break;
	case MTK_MAC_CTS_1000_TX_TEMPLATE_MODE_3:
		_mtk_dtv_gmac_cts_1000_tx_template_mode_3(ndev);
		break;
	case MTK_MAC_CTS_1000_TX_TEMPLATE_MODE_4:
		_mtk_dtv_gmac_cts_1000_tx_template_mode_4(ndev);
		break;
	case MTK_MAC_CTS_100_TX_TEMPLATE:
		_mtk_dtv_gmac_cts_100_tx_template(ndev);
		break;
	case MTK_MAC_CTS_10_TX_TEMPLATE_INIT:
		_mtk_dtv_gmac_cts_10_tx_template_init(ndev);
		break;
	case MTK_MAC_CTS_10_TX_TEMPLATE_CASE_1:
		_mtk_dtv_gmac_cts_10_tx_template_case_1(ndev);
		break;
	case MTK_MAC_CTS_10_TX_TEMPLATE_CASE_2:
		_mtk_dtv_gmac_cts_10_tx_template_case_2(ndev);
		break;
	case MTK_MAC_CTS_10_TX_TEMPLATE_CASE_3:
		_mtk_dtv_gmac_cts_10_tx_template_case_3(ndev);
		break;
	case MTK_MAC_CTS_10_TX_TEMPLATE_CASE_4:
		_mtk_dtv_gmac_cts_10_tx_template_case_4(ndev);
		break;
	case MTK_MAC_CTS_PHY_WRITE:
		_mtk_dtv_gmac_cts_phy_write(ndev, cts_cmd.input, cts_cmd.val);
		break;
	case MTK_MAC_CTS_PHY_READ:
		_mtk_dtv_gmac_cts_phy_read(ndev, cts_cmd.input, &cts_cmd.val);
		break;
	default:
		dev_info(priv->dev, "cts-eth: unknown mode: %d\n", cts_cmd.test_mode);
		break;
	}

	if (copy_to_user(req->ifr_data, &cts_cmd, sizeof(cts_cmd))) {
		dev_err(priv->dev, "cts-eth: copy to user fail\n");
		return -EFAULT;
	}

	return 0;
}

/* gmac sysfs for auto-nego restart */
static ssize_t an_restart_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	struct mtk_dtv_gmac_reg_ops *reg_ops = priv->reg_ops;
	ssize_t retval;
	int val;

	retval = kstrtoint(buf, 10, &val);
	if (retval == 0) {
		if (val) {
			dev_info(priv->dev, "restart auto-nego\n");
			reg_ops->write_phy(ndev, priv->phy_addr, MII_BMCR,
					   (BMCR_ANENABLE | BMCR_ANRESTART));
		}
	}

	return (retval < 0) ? retval : count;
}

static DEVICE_ATTR_WO(an_restart);

/* gmac sysfs for wakeup reason poll */
static ssize_t wakeup_reason_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	char wk_reason[PM_WK_REASON_NAME_SIZE];
	char *get_reason;
	ssize_t length = 0;
	int n;

	memset(wk_reason, 0, sizeof(wk_reason));
	get_reason = pm_get_wakeup_reason_str();
	if (!get_reason) {
		dev_err(priv->dev, "cannot get wakeup reason\n");
		length = -1;
	} else {
		strncpy(wk_reason, get_reason, sizeof(wk_reason) - 1);
		wk_reason[sizeof(wk_reason) - 1] = '\0';
		dev_info(priv->dev, "show wakeup reason is %s\n", wk_reason);
		length = strlen(wk_reason);
		n = snprintf(buf, sizeof(wk_reason) - 1, "%s\n", wk_reason);
		if ((n < 0) || (n >= (sizeof(wk_reason) - 1)))
			dev_err(priv->dev, "snprintf error: %d\n", n);
	}

	return length;
}

static DEVICE_ATTR_RO(wakeup_reason);

static struct attribute *mtk_dtv_gmac_attrs[] = {
	&dev_attr_an_restart.attr,
	&dev_attr_wakeup_reason.attr,
	NULL,
};

static const struct attribute_group mtk_dtv_gmac_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_gmac_attrs,
};

int mtk_dtv_gmac_sysfs_create(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);
	int ret;

	ret = sysfs_create_group(&priv->dev->kobj, &mtk_dtv_gmac_attr_group);
	if (ret) {
		dev_err(priv->dev, "create sysfs fail: %d\n", ret);
		return ret;
	}

	return 0;
}

void mtk_dtv_gmac_sysfs_remove(struct net_device *ndev)
{
	struct mtk_dtv_gmac_private *priv = netdev_priv(ndev);

	sysfs_remove_group(&priv->dev->kobj, &mtk_dtv_gmac_attr_group);
}

MODULE_DESCRIPTION("MediaTek DTV SoC based GMAC Driver");
MODULE_AUTHOR("Kevin Ho <kevin-yc.ho@mediatek.com>");
MODULE_LICENSE("GPL");

