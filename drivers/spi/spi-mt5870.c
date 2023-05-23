// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2019-2020 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/version.h>

/*
 * CONFIG_DEBUG_MSPI:
 *		1:map MAXIM pins of mspi to	 reg_ld_spi3_config
 *		0:require pins mapping at sboot/mboot stage.
 */
#define CONFIG_DEBUG_MSPI 0

/* SPI register offsets */
#define MSPI_WD0_1			0x40
#define MSPI_WD2_3			0x41
#define MSPI_WD4_5			0x42
#define MSPI_WD6_7			0x43
#define MSPI_RD0_1			0x78
#define MSPI_RD2_3			0x79
#define MSPI_RD4_5			0x7a
#define MSPI_RD6_7			0x7b
#define MSPI_WBF_RBF_SIZE		0x48
#define MSPI_WBF_RBF_SIZE_MAX		8
#define MSPI_CTRL_CLOCK_RATE		0x49
#define MSPI_ENABLE_BIT			BIT(0)
#define MSPI_RESET_BIT			BIT(1)
#define MSPI_ENABLE_INT_BIT		BIT(2)
#define MSPI_3WARE_MODE_BIT		BIT(4)
#define MSPI_CPHA_BIT			BIT(6)
#define MSPI_CPOL_BIT			BIT(7)
#define MSPI_TR_START_END_TIME		0x4a
#define MSPI_TBYTE_INTERVAL_AROUND_TIME	0x4b
#define MSPI_WD0_3_BIT_SEL		0x4c
#define MSPI_WD4_7_BIT_SEL		0x4d
#define MSPI_RD0_3_BIT_SEL		0x4e
#define MSPI_RD4_7_BIT_SEL		0x4f
#define MSPI_LSB_FIRST			0x50
#define MSPI_LSB_FIRST_BIT		BIT(0)
#define MSPI_TRIGGER			0x5a
#define MSPI_TRIGGER_BIT		BIT(0)
#define MSPI_DONE_FLAG			0x5b
#define MSPI_CLEAR_DONE_FLAG		0x5c
#define MSPI_CLEAR_DONE_FLAG_BIT	BIT(0)
#define MSPI_CHIP_SELECT		0x5f
#define MSPI_CHIP_SELECT_BIT		BIT(0)


#define DTV_SPI_TIMEOUT_MS		1000
#define DTV_SPI_MODE_BITS		(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
					| SPI_NO_CS | SPI_LSB_FIRST)

#define DRV_NAME			"mt5870-spi"

struct dtv_spi {
	u64 regs;
	u32 mspi_channel;
	struct clk *clk;
	int irq;
	struct completion done;
	const u8 *tx_buf;
	u8 *rx_buf;
	int len;
	int current_trans_len;
};

// read 2 byte
#define MSPI_READ(_reg_)		(readl(bs->regs + ((_reg_)<<2)))

// write 2 byte
#define MSPI_WRITE(_reg_, _val_)	\
		writel(_val_, (bs->regs + ((_reg_)<<2)))

static inline u16 dtvspi_rd(struct dtv_spi *bs, u32 reg)
{
	return MSPI_READ(reg);
}

static inline u8 dtvspi_rdh(struct dtv_spi *bs, u32 reg)
{
	return dtvspi_rd(bs, reg)>>8;
}

static inline u8 dtvspi_rdl(struct dtv_spi *bs, u16 reg)
{
	return dtvspi_rd(bs, reg)&0xff;
}

static inline void dtvspi_wr(struct dtv_spi *bs, u16 reg, u32 val)
{
	MSPI_WRITE(reg, val);
}

static inline void dtvspi_wrh(struct dtv_spi *bs, u16 reg, u8 val)
{
	u16 val16 = dtvspi_rd(bs, reg)&0xff;

	val16 |= ((u16)val)<<8;
	dtvspi_wr(bs, reg, val16);
}

static inline void dtvspi_wrl(struct dtv_spi *bs, u16 reg, u8 val)
{
	u16 val16 = dtvspi_rd(bs, reg)&0xff00;

	val16 |= val;
	dtvspi_wr(bs, reg, val16);
}

static const u16 mspi_txfifoaddr[] = {
	MSPI_WD0_1,
	MSPI_WD2_3,
	MSPI_WD4_5,
	MSPI_WD6_7,
};

static const u16 mspi_rxfifoaddr[] = {
	MSPI_RD0_1,
	MSPI_RD2_3,
	MSPI_RD4_5,
	MSPI_RD6_7,
};

static void dtvspi_hw_set_clock(struct dtv_spi *bs, struct spi_device *spi,
				struct spi_transfer *tfr)
{
	//Todo
	/*Add Clock tree*/
}

#if CONFIG_DEBUG_MSPI
static void dtvspi_hw_set_pin_mode(struct dtv_spi *bs, struct spi_device *spi)
{
	/*	Setup mspi pin mode from pinctrl. */
	u64 PINCTL_REG = 0x101E<<9;
	u64 PM_SLEEP_REG = 0x0E<<9;

	REG_ADDR(PM_SLEEP_REG + (0x72<<2)) = BIT(15);
	REG_ADDR(PINCTL_REG + (0x4e<<2)) = BIT(5);
}
#endif

static void dtvspi_hw_enable_interrupt(struct dtv_spi *bs, bool enable)
{
	u8 val = dtvspi_rdl(bs, MSPI_CTRL_CLOCK_RATE);

	if (enable)
		val |= MSPI_ENABLE_INT_BIT;
	else
		val &= ~MSPI_ENABLE_INT_BIT;

	dtvspi_wrl(bs, MSPI_CTRL_CLOCK_RATE, val);
}

static void dtvspi_hw_set_bits(struct dtv_spi *bs, struct spi_device *spi)
{
	int bits = spi->bits_per_word - 1;

	bits = bits|(bits<<3)|(bits<<6)|(bits<<9);
	dtvspi_wr(bs, MSPI_WD0_3_BIT_SEL, bits);
	dtvspi_wr(bs, MSPI_WD4_7_BIT_SEL, bits);
	dtvspi_wr(bs, MSPI_RD0_3_BIT_SEL, bits);
	dtvspi_wr(bs, MSPI_RD4_7_BIT_SEL, bits);
}

static void dtvspi_hw_set_mode(struct dtv_spi *bs, struct spi_device *spi)
{
	u8 val = dtvspi_rdl(bs, MSPI_CTRL_CLOCK_RATE);

	if (spi->mode&SPI_CPOL)
		val |= MSPI_CPOL_BIT;
	else
		val &= ~MSPI_CPOL_BIT;

	if (spi->mode&SPI_CPHA)
		val |= MSPI_CPHA_BIT;
	else
		val &= ~MSPI_CPHA_BIT;

	dtvspi_wrl(bs, MSPI_CTRL_CLOCK_RATE, val);
	val = dtvspi_rdl(bs, MSPI_LSB_FIRST);
	if (spi->mode&SPI_LSB_FIRST)
		val |= MSPI_LSB_FIRST_BIT;
	else
		val &= ~MSPI_LSB_FIRST_BIT;

	dtvspi_wrl(bs, MSPI_LSB_FIRST, val);
}
static inline void dtvspi_hw_chip_select(struct dtv_spi *bs,
					 struct spi_device *spi, bool enable)
{
	u8 val;

	if (spi->mode&SPI_NO_CS)
		return;

	val = dtvspi_rdl(bs, MSPI_CHIP_SELECT);
	if (enable != (!!(spi->mode&SPI_CS_HIGH)))
		val &= ~MSPI_CHIP_SELECT_BIT;
	else
		val |= MSPI_CHIP_SELECT_BIT;

	dtvspi_wrl(bs, MSPI_CHIP_SELECT, val);
}

static inline void dtvspi_hw_clear_done(struct dtv_spi *bs)
{
	dtvspi_wrl(bs, MSPI_CLEAR_DONE_FLAG, MSPI_CLEAR_DONE_FLAG_BIT);
}

static inline void dtvspi_hw_enable(struct dtv_spi *bs, bool enable)
{
	u8 val;

	val = dtvspi_rdl(bs, MSPI_CTRL_CLOCK_RATE);
	if (enable) {
		val |= MSPI_ENABLE_BIT;
		val |= MSPI_RESET_BIT;
	} else {
		val &= ~MSPI_ENABLE_BIT;
		val &= ~MSPI_RESET_BIT;
	}
	dtvspi_wrl(bs, MSPI_CTRL_CLOCK_RATE, val);
}

static inline void dtvspi_hw_transfer_trigger(struct dtv_spi *bs)
{
	dtvspi_wr(bs, MSPI_TRIGGER, MSPI_TRIGGER_BIT);
}

static void dtvspi_hw_txdummy(struct dtv_spi *bs, u8 len)
{
	int cnt;

	for (cnt = 0; cnt < len>>1; cnt++)
		dtvspi_wr(bs, mspi_txfifoaddr[cnt], 0xffff);

	if (len&1)
		dtvspi_wrl(bs, mspi_txfifoaddr[cnt], 0xff);

	dtvspi_wrl(bs, MSPI_WBF_RBF_SIZE, len);
}

static void dtvspi_hw_txfillfifo(struct dtv_spi *bs, const u8 *buffer, u8 len)
{
	int cnt;

	for (cnt = 0; cnt < len>>1; cnt++)
		dtvspi_wr(bs, mspi_txfifoaddr[cnt],
			  buffer[cnt<<1]|(buffer[(cnt<<1)+1]<<8));

	if (len&1)
		dtvspi_wrl(bs, mspi_txfifoaddr[cnt], buffer[cnt<<1]);

	dtvspi_wrl(bs, MSPI_WBF_RBF_SIZE, len);
}

static void dtvspi_hw_rxgetfifo(struct dtv_spi *bs, u8 *buffer, u8 len)
{
	int cnt;

	for (cnt = 0; cnt < (len>>1); cnt++) {
		u16 val = dtvspi_rd(bs, mspi_rxfifoaddr[cnt]);

		buffer[cnt<<1] = val&0xff;
		buffer[(cnt<<1)+1] = val>>8;
	}

	if (len&1)
		buffer[cnt<<1] = dtvspi_rdl(bs, mspi_rxfifoaddr[cnt]);
}

static void dtv_spi_hw_receive(struct dtv_spi *bs)
{
	if (bs->rx_buf != NULL) {
		dtvspi_hw_rxgetfifo(bs, bs->rx_buf, bs->current_trans_len);
		bs->rx_buf += bs->current_trans_len;
	}
}

static void dtv_spi_hw_transfer(struct dtv_spi *bs)
{
	int len = bs->len;

	if (len >= 8)
		len = 8;

	if (bs->tx_buf != NULL) {
		dtvspi_hw_txfillfifo(bs, bs->tx_buf, len);
		bs->tx_buf += len;
	} else
		dtvspi_hw_txdummy(bs, len);

	bs->current_trans_len = len;
	bs->len -= len;
	dtvspi_hw_transfer_trigger(bs);
}

static irqreturn_t dtv_spi_interrupt(int irq, void *dev_id)
{
	struct spi_master *master = dev_id;
	struct dtv_spi *bs = spi_master_get_devdata(master);

	if (dtvspi_rd(bs, MSPI_DONE_FLAG)) {
		dtvspi_hw_clear_done(bs);
		if (bs->current_trans_len != 0)
			dtv_spi_hw_receive(bs);
		else
			return IRQ_NONE;

		if ((bs->len) != 0) {
			dtv_spi_hw_transfer(bs);
		} else {
			bs->current_trans_len = 0;
			complete(&bs->done);
		}

		return IRQ_HANDLED;
	}

	dev_err(&master->dev, "Error:incorrect irq num!\n");
	dtvspi_hw_clear_done(bs);
	return IRQ_NONE;
}

static int dtv_spi_start_transfer(struct spi_device *spi,
				  struct spi_transfer *tfr)
{
	struct dtv_spi *bs = spi_master_get_devdata(spi->master);

#if CONFIG_DEBUG_MSPI
	dtvspi_hw_set_pin_mode(bs, spi);
#endif
	/*Setup mspi clock for this transfer.*/
	dtvspi_hw_set_clock(bs, spi, tfr);

	/*Setup mspi cpol & cpha for this transfer.*/
	dtvspi_hw_set_mode(bs, spi);

	/*Setup mspi transfer bits for this transfer.*/
	dtvspi_hw_set_bits(bs, spi);

	/*Enable SPI master controller&&Interrupt.*/
	dtvspi_hw_enable(bs, true);
	dtvspi_hw_clear_done(bs);
	dtvspi_hw_enable_interrupt(bs, true);

	/*Setup mspi chip select for this transfer.*/
	dtvspi_hw_chip_select(bs, spi, true);

	/*KERNEL_VERSION(3, 13, 1)*/
	//INIT_COMPLETION(bs->done);

	reinit_completion(&bs->done);
	bs->tx_buf = tfr->tx_buf;
	bs->rx_buf = tfr->rx_buf;
	bs->len = tfr->len;
	/*Start transfer loop.*/
	dtv_spi_hw_transfer(bs);

	return 0;
}

static int dtv_spi_finish_transfer(struct spi_device *spi,
				   struct spi_transfer *tfr, bool cs_change)
{
	struct dtv_spi *bs = spi_master_get_devdata(spi->master);

	if (tfr->delay_usecs)
		udelay(tfr->delay_usecs);

	if (cs_change) {
		/*Cancel chip select.*/
		dtvspi_hw_chip_select(bs, spi, false);
	}

	return 0;
}

static int dtv_spi_transfer_one(struct spi_master *master,
				struct spi_message *mesg)
{
	struct dtv_spi *bs = spi_master_get_devdata(master);
	struct spi_transfer *tfr;
	struct spi_device *spi = mesg->spi;
	int err = 0;
	unsigned int timeout;
	bool cs_change;

	list_for_each_entry(tfr, &mesg->transfers, transfer_list) {

		err = dtv_spi_start_transfer(spi, tfr);
		if (err)
			goto out;

		timeout = wait_for_completion_timeout(&bs->done,
				msecs_to_jiffies(DTV_SPI_TIMEOUT_MS));

		if (!timeout) {
			err = -ETIMEDOUT;
			goto out;
		}

		cs_change = tfr->cs_change ||
			list_is_last(&tfr->transfer_list, &mesg->transfers);

		err = dtv_spi_finish_transfer(spi, tfr, cs_change);
		if (err)
			goto out;

		mesg->actual_length += (tfr->len - bs->len);
	}

out:
	mesg->status = err;
	spi_finalize_current_message(master);

	return 0;
}

static const struct of_device_id dtvspi_mspi_match[] = {
	{ .compatible = "mediatek,mt5870-spi", },
	{}
};
MODULE_DEVICE_TABLE(of, dtvspi_mspi_match);

static int dtv_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct dtv_spi *bs;
	int err = -ENODEV;
	struct resource *res;

	master = spi_alloc_master(&pdev->dev, sizeof(*bs));
	if (!master) {
		dev_err(&pdev->dev, "spi_alloc_master() failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, master);

	master->mode_bits = DTV_SPI_MODE_BITS;
	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(1, 8);
	master->bus_num = -1;
	master->num_chipselect = 1;
	master->transfer_one_message = dtv_spi_transfer_one;
	master->dev.of_node = pdev->dev.of_node;

	bs = spi_master_get_devdata(master);

	init_completion(&bs->done);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!res)
		return -ENODEV;

	bs->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!bs->regs)
		return -EBUSY;

	bs->irq = platform_get_irq(pdev, 0);
	if (bs->irq < 0) {
		dev_err(&pdev->dev, "could not get irq resource\n");
		return -EINVAL;
	}

	err = of_property_read_u32(pdev->dev.of_node, "mspi_channel",
				   &bs->mspi_channel);
	if (err) {
		dev_err(&pdev->dev, "could not get chanel resource\n");
		return -EINVAL;
	}

	if (!bs->regs || !bs->irq) {
		dev_err(&pdev->dev, "could not get resource\n");
		return -EINVAL;
	}

	err = devm_request_irq(&pdev->dev, bs->irq, dtv_spi_interrupt, 0,
			       dev_name(&pdev->dev), master);
	if (err) {
		dev_err(&pdev->dev, "could not request IRQ: %d:%d\n",
			bs->irq, err);
		return err;
	}

	/*FIXME:Setup mspi clock from clock tree.*/
	{
		//Todo
	}


	devm_spi_register_master(&pdev->dev, master);
	if (err) {
		dev_err(&pdev->dev, "could not register SPI master: %d\n", err);
		return err;
	}

	return 0;
}

static int dtv_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct dtv_spi *bs = spi_master_get_devdata(master);

	return 0;
}

static struct platform_driver dtv_spi_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = dtvspi_mspi_match,
	},
	.probe = dtv_spi_probe,
	.remove = dtv_spi_remove,
};

module_platform_driver(dtv_spi_driver);

MODULE_DESCRIPTION("MSPI controller driver");
MODULE_AUTHOR("Owen Tseng <Owen.Tseng@mediatek.com>");
MODULE_LICENSE("GPL v2");
