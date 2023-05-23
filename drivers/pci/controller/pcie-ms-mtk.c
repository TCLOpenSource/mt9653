// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Liang-Yen Wang <liang-yen.wang@mediatek.com>
 *         Anson Chuang <anson.chuang@mediatek.com>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/msi.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <../pci.h>
#include <linux/of_platform.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/gpio/consumer.h>

#define VERSION	"20211109"
#define M6_PHY

#define MTK_PCIE_LOG_NONE		-1
#define MTK_PCIE_LOG_MUTE		0
#define MTK_PCIE_LOG_ERR		3
#define MTK_PCIE_LOG_INFO		6
#define MTK_PCIE_LOG_DEBUG		7

/* PCIe per-port registers */
#define PCIE_SETTING_REG			0x80
#define PCIE_RC_MODE				BIT(0)
#define PCIE_GEN2_SUPPORT			BIT(12)
#define PCIE_GEN3_SUPPORT			BIT(13)

#define PCIE_CFGNUM_REG				0x140
#define PCIE_CONF_FUN(fun)			(((fun) << 0) & GENMASK(2, 0))
#define PCIE_CONF_DEV(dev)			(((dev) << 3) & GENMASK(7, 3))
#define PCIE_CONF_BUS(bus)			(((bus) << 8) & GENMASK(15, 8))
#define PCIE_CFG_OFFSET_ADDR		0x1000
#define PCIE_CONF_ADDR(fun, dev, bus) \
	(PCIE_CONF_FUN(fun) | PCIE_CONF_DEV(dev) | PCIE_CONF_BUS(bus))

#define PCIE_RST_CTRL_REG			0x148
#define PCIE_MAC_RSTB				BIT(0)
#define PCIE_PHY_RSTB				BIT(1)
#define PCIE_BRG_RSTB				BIT(2)
#define PCIE_PE_RSTB				BIT(3)

#define PCIE_MISC_STATUS_REG		0x14C
#define PCIE_LTR_MSG_RECEIVED		BIT(0)
#define PCIE_PCIE_MSG_RECEIVED		BIT(1)

#ifdef CONFIG_PCIE_MEDIATEK_MT3611
#define PCIE_LINK_STATUS_REG		0x150
#define PCIE_PORT_LINKUP			BIT(29)
#else
#define PCIE_LTSSM_STATUS_REG		0x150
#define PCIE_LINK_STATUS_REG		0x154
#define PCIE_PORT_LINKUP			BIT(8)
#endif

#define PCI_NUM_INTX				4
#define PCIE_MSI_SET_NUM			2
#define PCIE_MSI_IRQS_PER_SET		32
#define PCIE_MSI_IRQS_TOTAL_NUM	(PCIE_MSI_IRQS_PER_SET * PCIE_MSI_SET_NUM)

#define PCIE_INT_MASK_REG			0x180
#define PCIE_MSI_MASK			GENMASK(PCIE_MSI_SET_NUM + 8 - 1, 8)
#define PCIE_INTX_SHIFT				24
#define PCIE_INTX_MASK				GENMASK(27, 24)
#define PCIE_MSG_MASK				BIT(28)
#define PCIE_AER_MASK				BIT(29)
#define PCIE_PM_MASK				BIT(30)

#define PCIE_INT_STATUS_REG			0x184
#define PCIE_MSI_SET_ENABLE_REG		0x190

#define PCIE_MSI_SET_OFFSET			0x010
#define PCIE_MSI_ADDR_BASE_REG		0xc00
#define PCIE_MSI_STATUS_BASE_REG	0xc04
#define PCIE_MSI_ENABLE_BASE_REG	0xc08
#define PCIE_MSI_GROUP1_ENABLE_BASE_REG		0xc0c

#define PCIE_ATR_TLB_SET_OFFSET				0x020
#define PCIE_ATR_PARAM_SRC_ADDR_LSB_REG		0x800
#define PCIE_ATR_SRC_ADDR_MSB_REG			0x804
#define PCIE_ATR_TRSL_ADDR_LSB				0x808
#define PCIE_ATR_TRSL_ADDR_MSB				0x80c
#define PCIE_ATR_TRSL_PARAM					0x810

#define PCIE_ATR_NPMEM_TABLE	0
#define PCIE_ATR_IO_TABLE		1

#define ATR_SIZE(size)		(((size) << 1) & GENMASK(6, 1))
#define ATR_ID(id)			(id & GENMASK(3, 0))
#define ATR_PARAM(param)	(((param) << 16) & GENMASK(27, 16))

#define DTS_PORT_NODE_MAX_LEN	20

/* phy register */
#if defined(M6_PHY)
/* gen3 phy */
#define REG_XTP_GLB_SPLL_FBKDIV_GEN4		0x0148
#define REG_XTP_GLB_SPLL_SSC_DELTA1_GEN4	0x0154
#define REG_XTP_GLB_SPLL_SSC_DELTA_GEN4		0x015C

#define REG_PEXTP_PHY_TOP_I2C_MODE			(0x08 * 4)

/*pm combo phy */ /*(0x003A * 4)*/
#define REG_USB_PHY_CTRL					0x00

/*xiu2axi base*/
#define REG_PCIE_PM							(0x01 * 4)
#endif

/* log_level */
#define DEV_ERR(port, fmt, args...) \
	do { if (port->pcie->log_level >= MTK_PCIE_LOG_ERR) \
		 dev_info(port->pcie->dev, fmt, ## args); \
	} while (0)
#define DEV_INFO(port, fmt, args...) \
	do { if (port->pcie->log_level >= MTK_PCIE_LOG_INFO) \
		dev_info(port->pcie->dev, fmt, ## args); \
	} while (0)
#define DEV_DEBUG(port, fmt, args...) \
	do { if (port->pcie->log_level >= MTK_PCIE_LOG_DEBUG) \
		dev_dbg(port->pcie->dev, fmt, ## args); \
	} while (0)




struct mtk_pcie_port;

/**
 * struct mtk_pcie_soc - differentiate between host generations
 * @ops: pointer to configuration access functions
 * @startup: pointer to controller setting functions
 * @setup_irq: pointer to initialize IRQ functions
 */
struct mtk_pcie_soc {
	struct pci_ops *ops;
	int (*startup)(struct mtk_pcie_port *port);
	int (*setup_irq)(struct mtk_pcie_port *port, struct device_node *node);
};

/**
 * struct mtk_pcie_port - PCIe port information
 * @base: IO mapped register base
 * @regs: IO mapped register base
 * @list: port list
 * @pcie: pointer to PCIe host info
 * @lane: lane count
 * @slot: port slot
 * @irq_domain: legacy INTx IRQ domain
 * @inner_domain: inner IRQ domain
 * @msi_domain: MSI IRQ domain
 * @lock: protect the msi_irq_in_use bitmap
 * @virq: virq of root port
 * @msi_irq_in_use: bit map for assigned MSI IRQ
 */
struct mtk_pcie_port {
	void __iomem *base;
	void __iomem *phy_base;
	void __iomem *xiu2axi_base;
	void __iomem *pm_misc_base;
	void __iomem *phy_misc_base;
	struct resource *regs;
	struct list_head list;
	struct mtk_pcie *pcie;
	struct clk *smi_ck_src;
	struct clk *smi_ck_mux;
	struct clk *tl108;
	struct clk *sys_ck;
	struct clk *ahb_ck;
	struct clk *axi_ck;
	struct clk *aux_ck;
	struct clk *obff_ck;
	struct clk *pipe_ck;
	struct phy *phy;
	u32 lane;
	u32 slot;
	int irq;
	int irq_msi_group1;
	struct irq_domain *irq_domain;
	struct irq_domain *inner_domain;
	struct irq_domain *msi_domain;
	struct mutex lock;
	int virq;
	DECLARE_BITMAP(msi_irq_in_use, PCIE_MSI_IRQS_TOTAL_NUM);

	struct gpio_desc *pwrctl_gpio;
	/*struct gpio_desc *perst_gpio;*/
};

/**
 * struct mtk_pcie - PCIe host information
 * @dev: pointer to PCIe device
 * @io: IO resource
 * @pio: PIO resource
 * @mem: non-prefetchable memory resource
 * @busn: bus range
 * @offset: IO / Memory offset
 * @ports: pointer to PCIe port information
 * @soc: pointer to SoC-dependent operations
 */
struct mtk_pcie {
	struct device *dev;
	struct resource io;
	struct resource pio;
	struct resource mem;
	struct resource busn;
	struct {
		resource_size_t mem;
		resource_size_t io;
	} offset;
	struct list_head ports;
	const struct mtk_pcie_soc *soc;
	int	pm_support;
	int log_level;		/* mtk_dgb */
};

static struct mtk_pcie_port *mtk_pcie_find_port(struct pci_bus *bus,
						unsigned int devfn)
{
	struct mtk_pcie *pcie = bus->sysdata;
	struct mtk_pcie_port *port;
	struct pci_dev *dev;
	struct pci_bus *pbus;

	list_for_each_entry(port, &pcie->ports, list) {
		if (!bus->number && port->slot == PCI_SLOT(devfn))
			return port;

		if (bus->number) {
			pbus = bus;

			while (pbus->number) {
				dev = pbus->self;
				pbus = dev->bus;
			}

			if (port->slot == PCI_SLOT(dev->devfn))
				return port;
		}
	}

	return NULL;
}

/*
 * static void __iomem *mtk_pcie_map_bus(struct pci_bus *bus,
 *				      unsigned int devfn, int where)
 * {
 *	struct mtk_pcie_port *port;
 *
 *	port = mtk_pcie_find_port(bus, devfn);
 *	if (!port)
 *		return NULL;
 *
 *	writel(PCIE_CONF_ADDR(PCI_FUNC(devfn), PCI_SLOT(devfn), bus->number), \
 *			port->base + PCIE_CFGNUM_REG);
 *
 *	return port->base + PCIE_CFG_OFFSET_ADDR + where;
 * }
 */

static void __iomem *mtk_pcie_map_bus_v3(struct pci_bus *bus,
				      unsigned int devfn, int where)
{
	struct mtk_pcie_port *port;

	port = mtk_pcie_find_port(bus, devfn);
	if (!port)
		return NULL;

	writel(PCIE_CONF_ADDR(PCI_FUNC(devfn), PCI_SLOT(devfn), bus->number),
			port->base + PCIE_CFGNUM_REG);

	return port->base + PCIE_CFG_OFFSET_ADDR + (where & ~0x3);
}


static struct pci_ops mtk_pcie_ops = {
	.map_bus = mtk_pcie_map_bus_v3,
	.read  = pci_generic_config_read32,
	.write = pci_generic_config_write32,
};

static void mtk_pcie_misc(struct mtk_pcie_port *port)
{
	u32 val;

	#define PCIE_PCI_IDS_1			0x9c
	#define PCI_CLASS(class)		(class << 8)

	#define PCIE_PEX_LINK			0xc8
	#define ASPM_L1_TIMER_RECOUNT	BIT(21)

	#define PCIE_PEX_SPC2			0xd8
	#define ECRC_generation_support BIT(1)
	#define ECRC_checking_support	BIT(2)
	#define AER_MSI_NUM_MASK		GENMASK(7, 3)
	#define PCIE_MSI_NUM_MASK		GENMASK(12, 8)

	/* Set class code */
	writel((readl(port->base + PCIE_PCI_IDS_1) &
			~(0xFFFFFF << 8)) |
			PCI_CLASS(PCI_CLASS_BRIDGE_PCI << 8),
			port->base + PCIE_PCI_IDS_1);

	/* Set AER ECRC and PME/AER msi msg num */
	val = readl(port->base + PCIE_PEX_SPC2);
	val |= (ECRC_generation_support | ECRC_checking_support);
	val &= ~(AER_MSI_NUM_MASK | PCIE_MSI_NUM_MASK);
	writel(val, port->base + PCIE_PEX_SPC2);

#ifdef CONFIG_PCIE_MEDIATEK_MT3611
	writel(readl(port->base + PCIE_PEX_LINK) |
			ASPM_L1_TIMER_RECOUNT, port->base +
			PCIE_PEX_LINK);
#endif
}

static void mtk_pcie_enable_msi(struct mtk_pcie_port *port)
{
	int i;
	u32 val;
	phys_addr_t msg_addr;

	for (i = 0; i < PCIE_MSI_SET_NUM; i++) {
		msg_addr = port->regs->start + PCIE_MSI_ADDR_BASE_REG +
			i * PCIE_MSI_SET_OFFSET;
		val = lower_32_bits(msg_addr);
		writel(val, port->base + PCIE_MSI_ADDR_BASE_REG +
			i * PCIE_MSI_SET_OFFSET);

		writel(0xFFFFFFFF, port->base + PCIE_MSI_ENABLE_BASE_REG +
			i * PCIE_MSI_SET_OFFSET);
	}

	val = readl(port->base + PCIE_INT_MASK_REG);
	val |= PCIE_MSI_MASK;
	writel(val, port->base + PCIE_INT_MASK_REG);

#ifndef CONFIG_PCIE_MEDIATEK_MT3611
	val = readl(port->base + PCIE_MSI_SET_ENABLE_REG);
	val |= GENMASK(PCIE_MSI_SET_NUM - 1, 0);
	writel(val, port->base + PCIE_MSI_SET_ENABLE_REG);
#endif
}

static void mtk_pcie_irq_remove(struct mtk_pcie_port *port)
{
	/*struct mtk_pcie *pcie = port->pcie;*/

	DEV_INFO(port, "%s %d\n", __func__, __LINE__);

	irq_set_chained_handler_and_data(port->irq, NULL, NULL);

	if (port->irq_domain)
		irq_domain_remove(port->irq_domain);
	if (port->msi_domain)
		irq_domain_remove(port->msi_domain);
	if (port->inner_domain)
		irq_domain_remove(port->inner_domain);

	irq_dispose_mapping(port->irq);

	if (port->irq_msi_group1 >= 0) {
		irq_set_chained_handler_and_data(port->irq_msi_group1, NULL, NULL);
		irq_dispose_mapping(port->irq_msi_group1);
	}
}

/*
 *static void mtk_pcie_irq_teardown(struct mtk_pcie *pcie)
 *{
 *	struct mtk_pcie_port *port, *tmp;
 *
 *	if (list_empty(&pcie->ports))
 *		return;
 *
 *	list_for_each_entry_safe(port, tmp, &pcie->ports, list) {
 *		mtk_pcie_irq_remove(port);
 *	}
 *}
 */

static int mtk_pcie_startup_port(struct mtk_pcie_port *port)
{
	struct mtk_pcie *pcie = port->pcie;
	struct resource *mem = &pcie->mem;
	struct resource *pio = &pcie->pio;
	u32 val;
	size_t size;
	int err;

	/* set as RC mode */
	writel(readl(port->base + PCIE_SETTING_REG) |
			PCIE_RC_MODE, port->base + PCIE_SETTING_REG);

#if defined(M5_PHY)
	if (port->phy_base) {
		/* printk("%s %d Init phy\n", __func__, __LINE__);*/
		/* set phy host mode */
		writel(0x4b000010, port->phy_base + 0xa00);

		/* phy txpll init */
		//mtk_write_reg_field(14, 10, 0x01, port->phy_base + 0xc14);
		writel((readl(port->phy_base + 0xc14) & ~GENMASK(14, 10))
			| (0x01<<10), port->phy_base + 0xc14);
		//mtk_write_reg_field(11,  8, 0x0d, port->phy_base + 0xc10);
		writel((readl(port->phy_base + 0xc10) & ~GENMASK(11, 8))
			| (0x0d<<8), port->phy_base + 0xc10);
		//mtk_write_reg_field(13, 10, 0x0b, port->phy_base + 0xc6c);
		writel((readl(port->phy_base + 0xc6c) & ~GENMASK(13, 10))
			| (0x0b<<10), port->phy_base + 0xc6c);
		//mtk_write_reg_field(24, 20, 0x01, port->phy_base + 0xc14);
		writel((readl(port->phy_base + 0xc14) & ~GENMASK(24, 20))
			| (0x01<<20), port->phy_base + 0xc14);
		//mtk_write_reg_field(27, 24, 0x0d, port->phy_base + 0xc10);
		writel((readl(port->phy_base + 0xc10) & ~GENMASK(27, 24))
			| (0x0d<<24), port->phy_base + 0xc10);
		//mtk_write_reg_field(23, 20, 0x0b, port->phy_base + 0xc6c);
		writel((readl(port->phy_base + 0xc6c) & ~GENMASK(23, 20))
			| (0x0B<<20), port->phy_base + 0xc6c);

		/* set RG_SSUSB_SYSPLL_FBDIV */
		writel(0x682aaaab, port->phy_base + 0xb34);
		writel(0x00000200, port->phy_base + 0x708);
		writel(0x00000600, port->phy_base + 0x708);

		/* reset PHY */
		writel(0x43000000, port->phy_base + 0x810);
		writel(0xe0000000, port->phy_base + 0x80c);
		writel(0x40000000, port->phy_base + 0x810);
		writel(0x40000000, port->phy_base + 0x80c);
	}
#endif
#if defined(M6_PHY)
	/* pcie clock setting */
	if (port->smi_ck_src && port->smi_ck_mux) {
		dev_info(pcie->dev, "set smi_ck_mux parent\n");
		if (clk_set_parent(port->smi_ck_mux, port->smi_ck_src))
			DEV_ERR(port, "failed to set smi_ck_mux parent\n");
	}

	if (port->axi_ck) {
		err = clk_prepare_enable(port->axi_ck);
		if (err)
			return err;
	}

	if (port->tl108) {
		err = clk_prepare_enable(port->tl108);
		if (err)
			return err;
	}

	if (port->phy_base && (port->lane > 1)) {
		DEV_INFO(port, "Gen3 phy init\n");

		/* PCIE Gen3 SW patch setting */
		writel(0xD055555L, port->phy_base + REG_XTP_GLB_SPLL_FBKDIV_GEN4);

		writel((readl(port->phy_base + REG_XTP_GLB_SPLL_SSC_DELTA1_GEN4) &
			~GENMASK(31, 16)) | 0x02800000L,
			port->phy_base + REG_XTP_GLB_SPLL_SSC_DELTA1_GEN4);

		writel((readl(port->phy_base + REG_XTP_GLB_SPLL_SSC_DELTA_GEN4) &
			~GENMASK(31, 16)) | 0x026F0000L,
			port->phy_base + REG_XTP_GLB_SPLL_SSC_DELTA1_GEN4);

		/* I2C set MCU mode */
		writel(readl(port->phy_misc_base + REG_PEXTP_PHY_TOP_I2C_MODE) & ~0x01L,
					port->phy_misc_base + REG_PEXTP_PHY_TOP_I2C_MODE);
	}

	/* PCIe PM Gen2 combo phy */
	if (port->pm_misc_base) {

		/*patch of HW setting default value*/
		/*MMIO base set to 0x0C000000, pm port only*/
		if (port->xiu2axi_base)
			writel((readl(port->xiu2axi_base + REG_PCIE_PM) | 0x03),
					port->xiu2axi_base + REG_PCIE_PM);

		DEV_INFO(port, "Combo phy set PCIe mode\n");

		/* set combo phy pcie mode, I2C set MCU mode */
		writew(0x00, port->pm_misc_base + REG_USB_PHY_CTRL);
	}

#endif


	mtk_pcie_misc(port);


	/*
	 *if (port->perst_gpio) {
	 *	DEV_INFO(port, "PERST low\n");
	 *	gpiod_set_value(port->perst_gpio, true);
	 *}
	 */

	/* Assert all reset signals */
	writel(readl(port->base + PCIE_RST_CTRL_REG) |
			PCIE_MAC_RSTB | PCIE_PHY_RSTB |
			PCIE_BRG_RSTB | PCIE_PE_RSTB,
			port->base + PCIE_RST_CTRL_REG);

	usleep_range(500, 1000);

	/* de-assert reset signals*/
	writel(readl(port->base + PCIE_RST_CTRL_REG) &
			~(PCIE_MAC_RSTB | PCIE_PHY_RSTB | PCIE_BRG_RSTB),
			port->base + PCIE_RST_CTRL_REG);

	/*usleep_range(100 * 1000, 120 * 1000);*/
	msleep(100);
	/* de-assert pe reset*/
	writel(readl(port->base + PCIE_RST_CTRL_REG) &
			~(PCIE_PE_RSTB), port->base + PCIE_RST_CTRL_REG);


	/*
	 *while(1) {
	 *	dev_info(pcie->dev,"STSSMStatusReg(0x150)=%x\n",
	 *			readl(port->base + PCIE_LTSSM_STATUS_REG));
	 *	dev_info(pcie->dev,"LinkStatus(0x154)    =%x\n",
	 *			readl(port->base + PCIE_LINK_STATUS_REG));
	 *	mdelay(500);
	 *	}
	 */

	/* check if the link is up or not */
	err = readl_poll_timeout(port->base + PCIE_LINK_STATUS_REG, val,
				 !!(val & PCIE_PORT_LINKUP), 20,
				 100 * USEC_PER_MSEC);
	if (err)
		return -ETIMEDOUT;

	/* set INT mask */
#ifdef CONFIG_PCIE_MEDIATEK_MT3611
	writel((PCIE_INTX_MASK | PCIE_AER_MASK | PCIE_PM_MASK),
			port->base + PCIE_INT_MASK_REG);
#else
	writel((PCIE_INTX_MASK | PCIE_MSG_MASK | PCIE_AER_MASK | PCIE_PM_MASK),
			port->base + PCIE_INT_MASK_REG);
#endif

	if (IS_ENABLED(CONFIG_PCI_MSI))
		mtk_pcie_enable_msi(port);

#ifdef CONFIG_64BIT
	/* set PCIe translation windows */
	if (mem) {
		resource_size_t pci_addr;

		pci_addr = mem->start - pcie->offset.mem;
		size = mem->end - mem->start;

		writel(lower_32_bits((u64)pci_addr) | ATR_SIZE(fls(size) - 1) | 1,
				port->base + PCIE_ATR_PARAM_SRC_ADDR_LSB_REG +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(upper_32_bits((u64)pci_addr),
				port->base + PCIE_ATR_SRC_ADDR_MSB_REG +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(lower_32_bits((u64)pci_addr),
				port->base + PCIE_ATR_TRSL_ADDR_LSB +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(upper_32_bits((u64)pci_addr),
				port->base + PCIE_ATR_TRSL_ADDR_MSB +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(ATR_ID(0) | ATR_PARAM(0),
				port->base + PCIE_ATR_TRSL_PARAM +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
	}

	if (pio) {
		resource_size_t pci_addr;

		pci_addr = pio->start - pcie->offset.io;
		size = pio->end - pio->start;

		writel(lower_32_bits((u64)pci_addr) | ATR_SIZE(fls(size) - 1) | 1,
				port->base + PCIE_ATR_PARAM_SRC_ADDR_LSB_REG +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(upper_32_bits((u64)pci_addr),
				port->base + PCIE_ATR_SRC_ADDR_MSB_REG +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(lower_32_bits((u64)pci_addr),
				port->base + PCIE_ATR_TRSL_ADDR_LSB +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(upper_32_bits((u64)pci_addr),
				port->base + PCIE_ATR_TRSL_ADDR_MSB +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(ATR_ID(1) | ATR_PARAM(2),
				port->base + PCIE_ATR_TRSL_PARAM +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
	}
#else
	if (mem) {
		resource_size_t pci_addr;

		pci_addr = mem->start - pcie->offset.mem;
		size = mem->end - mem->start;

		writel(lower_32_bits(pci_addr) | ATR_SIZE(fls(size) - 1) | 1,
				port->base + PCIE_ATR_PARAM_SRC_ADDR_LSB_REG +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(0, port->base + PCIE_ATR_SRC_ADDR_MSB_REG +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(lower_32_bits(pci_addr),
				port->base + PCIE_ATR_TRSL_ADDR_LSB +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(0, port->base + PCIE_ATR_TRSL_ADDR_MSB +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(ATR_ID(0) | ATR_PARAM(0),
				port->base + PCIE_ATR_TRSL_PARAM +
				PCIE_ATR_NPMEM_TABLE * PCIE_ATR_TLB_SET_OFFSET);
	}

	if (pio) {
		resource_size_t pci_addr;

		pci_addr = pio->start - pcie->offset.io;
		size = pio->end - pio->start;

		writel(lower_32_bits(pci_addr) | ATR_SIZE(fls(size) - 1) | 1,
				port->base + PCIE_ATR_PARAM_SRC_ADDR_LSB_REG +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(0, port->base + PCIE_ATR_SRC_ADDR_MSB_REG +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(lower_32_bits(pci_addr),
				port->base + PCIE_ATR_TRSL_ADDR_LSB +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(0, port->base + PCIE_ATR_TRSL_ADDR_MSB +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
		writel(ATR_ID(1) | ATR_PARAM(2),
				port->base + PCIE_ATR_TRSL_PARAM +
				PCIE_ATR_IO_TABLE * PCIE_ATR_TLB_SET_OFFSET);
	}
#endif

	return 0;
}

static void mtk_compose_msi_msg(struct irq_data *data, struct msi_msg *msg)
{
	struct mtk_pcie_port *port = irq_data_get_irq_chip_data(data);
	/*struct mtk_pcie *pcie = port->pcie;*/
	phys_addr_t addr;

	addr = port->regs->start + PCIE_MSI_ADDR_BASE_REG +
			(data->hwirq / PCIE_MSI_IRQS_PER_SET) *
			PCIE_MSI_SET_OFFSET;

	msg->address_hi = 0;
	msg->address_lo = lower_32_bits(addr);

	msg->data = (data->hwirq) % PCIE_MSI_IRQS_PER_SET;

	DEV_DEBUG(port, "msi#%#lx address_hi %#x address_lo %#x data %d\n",
			data->hwirq, msg->address_hi, msg->address_lo, msg->data);
}

static int mtk_msi_set_affinity(struct irq_data *irq_data,
				const struct cpumask *mask, bool force)
{
	return -EINVAL;
}

static void mtk_msi_ack_irq(struct irq_data *data)
{
	struct mtk_pcie_port *port = irq_data_get_irq_chip_data(data);
	u32 irq = data->hwirq % PCIE_MSI_IRQS_PER_SET;

	writel(1 << irq, port->base + PCIE_MSI_STATUS_BASE_REG +
			(data->hwirq / PCIE_MSI_IRQS_PER_SET) *
			PCIE_MSI_SET_OFFSET);
}

static struct irq_chip mtk_msi_bottom_irq_chip = {
	.name			= "MTK MSI",
	.irq_compose_msi_msg	= mtk_compose_msi_msg,
	.irq_set_affinity	= mtk_msi_set_affinity,
	.irq_ack		= mtk_msi_ack_irq,
};

static int mtk_pcie_irq_domain_alloc(struct irq_domain *domain,
			unsigned int virq, unsigned int nr_irqs,
			void *args)
{
	struct mtk_pcie_port *port = domain->host_data;
	int bit;
	int i;

	mutex_lock(&port->lock);

	bit = bitmap_find_free_region(port->msi_irq_in_use,
			PCIE_MSI_IRQS_TOTAL_NUM,
			order_base_2(nr_irqs));
	if (bit < 0) {
		mutex_unlock(&port->lock);
		return -ENOSPC;
	}

	for (i = 0; i < nr_irqs; i++) {
		irq_domain_set_info(domain, virq + i, bit + i,
				&mtk_msi_bottom_irq_chip,
				domain->host_data, handle_edge_irq,
				NULL, NULL);
	}

	mutex_unlock(&port->lock);
	return 0;
}

static void mtk_pcie_irq_domain_free(struct irq_domain *domain,
				     unsigned int virq, unsigned int nr_irqs)
{
	struct irq_data *d = irq_domain_get_irq_data(domain, virq);
	struct mtk_pcie_port *port = irq_data_get_irq_chip_data(d);

	mutex_lock(&port->lock);
	bitmap_clear(port->msi_irq_in_use, d->hwirq,
			1ULL << order_base_2(nr_irqs));
	mutex_unlock(&port->lock);

	irq_domain_free_irqs_parent(domain, virq, nr_irqs);
}

static const struct irq_domain_ops msi_domain_ops = {
	.alloc	= mtk_pcie_irq_domain_alloc,
	.free	= mtk_pcie_irq_domain_free,
};

static struct irq_chip mtk_msi_irq_chip = {
	.name		= "MTK PCIe MSI",
	.irq_ack	= irq_chip_ack_parent,
	.irq_mask	= pci_msi_mask_irq,
	.irq_unmask	= pci_msi_unmask_irq,
};

static struct msi_domain_info mtk_msi_domain_info = {
	.flags	= (MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS |
		   MSI_FLAG_MULTI_PCI_MSI | MSI_FLAG_PCI_MSIX),
	.chip	= &mtk_msi_irq_chip,
};

static int mtk_pcie_allocate_msi_domains(struct mtk_pcie_port *port)
{
	struct fwnode_handle *fwnode = of_node_to_fwnode(
				port->pcie->dev->of_node);
	/*struct mtk_pcie *pcie = port->pcie;*/

	mutex_init(&port->lock);

	port->inner_domain = irq_domain_create_linear(fwnode,
			PCIE_MSI_IRQS_TOTAL_NUM,
			&msi_domain_ops, port);
	if (!port->inner_domain) {
		DEV_ERR(port, "failed to create IRQ domain\n");
		return -ENOMEM;
	}

	port->msi_domain = pci_msi_create_irq_domain(fwnode,
			&mtk_msi_domain_info, port->inner_domain);
	if (!port->msi_domain) {
		DEV_ERR(port, "failed to create MSI domain\n");
		irq_domain_remove(port->inner_domain);
		return -ENOMEM;
	}

	return 0;
}

static int mtk_pcie_intx_map(struct irq_domain *domain, unsigned int irq,
			     irq_hw_number_t hwirq)
{
	irq_set_chip_and_handler(irq, &dummy_irq_chip, handle_simple_irq);
	irq_set_chip_data(irq, domain->host_data);

	return 0;
}

static const struct irq_domain_ops intx_domain_ops = {
	.map = mtk_pcie_intx_map,
};

static int mtk_pcie_init_irq_domain(struct mtk_pcie_port *port,
				    struct device_node *node)
{
	struct device *dev = port->pcie->dev;
	struct device_node *pcie_intc_node;
	/*struct mtk_pcie *pcie = port->pcie;*/
	int ret;

	/* Setup INTx */
	pcie_intc_node = of_get_next_child(node, NULL);
	if (!pcie_intc_node) {
		dev_err(dev, "no PCIe Intc node found\n");
		return -ENODEV;
	}

	port->irq_domain = irq_domain_add_linear(pcie_intc_node, PCI_NUM_INTX,
						 &intx_domain_ops, port);
	if (!port->irq_domain) {
		DEV_ERR(port, "failed to get INTx IRQ domain\n");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		ret = mtk_pcie_allocate_msi_domains(port);
		if (ret)
			return ret;
	}

	return 0;
}

static void mtk_pcie_intr_handler(struct irq_desc *desc)
{
	struct mtk_pcie_port *port = irq_desc_get_handler_data(desc);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	unsigned long status;
	u32 virq;
	u32 bit = PCIE_INTX_SHIFT;
	int i;

	chained_irq_enter(irqchip, desc);

	status = readl(port->base + PCIE_INT_STATUS_REG);
	if (status & PCIE_INTX_MASK) {
		for_each_set_bit_from(bit, &status, PCI_NUM_INTX +
				PCIE_INTX_SHIFT) {
			virq = irq_find_mapping(port->irq_domain,
						bit - PCIE_INTX_SHIFT);
			generic_handle_irq(virq);

			/* Clear the INTx */
			writel(1 << bit, port->base + PCIE_INT_STATUS_REG);
		}
	}

	/* MSI group 0 */
	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		if (status & PCIE_MSI_MASK) {
			unsigned long imsi_enable;
			unsigned long imsi_status;

			for (i = 0; i < PCIE_MSI_SET_NUM; i++) {
				while (1) {
					imsi_enable = readl(port->base +
						PCIE_MSI_ENABLE_BASE_REG +
						i * PCIE_MSI_SET_OFFSET);
					imsi_status = readl(port->base +
						PCIE_MSI_STATUS_BASE_REG +
						i * PCIE_MSI_SET_OFFSET);
					imsi_status = imsi_status & imsi_enable;

					if (!imsi_status)
						break;

					for_each_set_bit(bit, &imsi_status,
						PCIE_MSI_IRQS_PER_SET) {
						virq = irq_find_mapping(
						port->inner_domain,
						bit + i *
						PCIE_MSI_IRQS_PER_SET);
						generic_handle_irq(virq);
					}
				}
			}

			/* Clear MSI interrupt status */
			writel(PCIE_MSI_MASK, port->base + PCIE_INT_STATUS_REG);
		}
	}
#ifdef CONFIG_PCIE_MEDIATEK_MT3611
	if (status & (PCIE_AER_MASK | PCIE_PM_MASK)) {
		if (port->virq)
			generic_handle_irq(port->virq);
		writel((PCIE_AER_MASK | PCIE_PM_MASK),
			port->base + PCIE_INT_STATUS_REG);
	}
#else
	if (status & (PCIE_MSG_MASK | PCIE_AER_MASK | PCIE_PM_MASK)) {
		if (port->virq)
			generic_handle_irq(port->virq);
		writel((PCIE_LTR_MSG_RECEIVED | PCIE_PCIE_MSG_RECEIVED),
			port->base + PCIE_MISC_STATUS_REG);
		writel((PCIE_MSG_MASK | PCIE_AER_MASK | PCIE_PM_MASK),
			port->base + PCIE_INT_STATUS_REG);
	}
#endif

	chained_irq_exit(irqchip, desc);
}

static void mtk_pcie_intr_handler_msi_group1(struct irq_desc *desc)
{
	struct mtk_pcie_port *port = irq_desc_get_handler_data(desc);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	u32 virq;
	u32 bit = PCIE_INTX_SHIFT;
	int i;

	chained_irq_enter(irqchip, desc);

	/* MSI group 1 */
	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		unsigned long imsi_enable;
		unsigned long imsi_status;

		for (i = 0; i < PCIE_MSI_SET_NUM; i++) {
			while (1) {
				imsi_enable = readl(port->base +
					PCIE_MSI_GROUP1_ENABLE_BASE_REG +
					i * PCIE_MSI_SET_OFFSET);
				imsi_status = readl(port->base +
					PCIE_MSI_STATUS_BASE_REG +
					i * PCIE_MSI_SET_OFFSET);
				imsi_status = imsi_status & imsi_enable;
				if (!imsi_status)
					break;

				for_each_set_bit(bit, &imsi_status,
						PCIE_MSI_IRQS_PER_SET) {
					virq = irq_find_mapping(
						port->inner_domain, bit +
						i * PCIE_MSI_IRQS_PER_SET);
					generic_handle_irq(virq);
				}
			}
		}
	}

	chained_irq_exit(irqchip, desc);
}

static void mtk_pcie_subsys_powerdown(struct mtk_pcie *pcie)
{
	struct device *dev = pcie->dev;

	if (dev->pm_domain) {
		pm_runtime_put_sync(dev);
		pm_runtime_disable(dev);
	}
}

static int mtk_pcie_setup_irq(struct mtk_pcie_port *port,
			      struct device_node *node)
{
	struct mtk_pcie *pcie = port->pcie;
	struct device *dev = pcie->dev;
	struct platform_device *pdev = to_platform_device(dev);
	int err, irq, irq_msi_group1;

	err = mtk_pcie_init_irq_domain(port, node);
	if (err) {
		DEV_ERR(port, "failed to init PCIe IRQ domain\n");
		return err;
	}

	irq = platform_get_irq(pdev, port->slot);
	port->irq = irq;

	irq_set_chained_handler_and_data(irq,
					 mtk_pcie_intr_handler, port);

	irq_msi_group1 = platform_get_irq(pdev, port->slot + 1);
	port->irq_msi_group1 = irq_msi_group1;

	if (irq_msi_group1 >= 0) {
		irq_set_chained_handler_and_data(irq_msi_group1,
			 mtk_pcie_intr_handler_msi_group1, port);
	}

	return 0;
}

static void mtk_pcie_port_free(struct mtk_pcie_port *port)
{
	struct mtk_pcie *pcie = port->pcie;
	struct device *dev = pcie->dev;

	if (port->base)
		devm_iounmap(dev, port->base);

	if (port->phy_base)
		devm_iounmap(dev, port->phy_base);

#if defined(M6_PHY)
	if (port->tl108)
		clk_disable_unprepare(port->tl108);
	if (port->axi_ck)
		clk_disable_unprepare(port->axi_ck);
#endif

	mtk_pcie_irq_remove(port);

	list_del(&port->list);
	devm_kfree(dev, port);
}

static void mtk_pcie_put_resources(struct mtk_pcie *pcie)
{
	struct mtk_pcie_port *port, *tmp;

	list_for_each_entry_safe(port, tmp, &pcie->ports, list) {
		mtk_pcie_port_free(port);
	}
}

static void mtk_pcie_enable_port(struct mtk_pcie_port *port)
{
	struct mtk_pcie *pcie = port->pcie;
	/*struct device *dev = pcie->dev;*/

	if (!pcie->soc->startup(port))
		return;

	DEV_INFO(port, "Port%d link down\n", port->slot);

	mtk_pcie_port_free(port);
}

#if defined(M6_PHY)
static void mtk_pcie_M6_phy_parse_port(struct mtk_pcie *pcie,
					struct mtk_pcie_port *port,
					struct device_node *node,
					unsigned char slot)
{
	struct device *dev = pcie->dev;
	struct platform_device *pdev = to_platform_device(dev);

	/* Set PCIe function clock to 108M */
	/*
	 *	snprintf(name, sizeof(name), "ckgen%d", slot);
	 *	port->regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	 *	port->ckgen_base = devm_ioremap_resource(dev, port->regs);
	 *	if (IS_ERR(port->ckgen_base)) {
	 *		dev_err(dev, "failed to map ckgen base\n");
	 *		return PTR_ERR(port->ckgen_base);
	 *	}
	 */

	port->smi_ck_src = devm_clk_get(&pdev->dev, "SMI_CK_SRC");
	if (IS_ERR(port->smi_ck_src)) {
		DEV_ERR(port, "no %d SMI_CK_SRC source\n", slot);

		port->smi_ck_src = NULL;
	}

	port->smi_ck_mux = devm_clk_get(&pdev->dev, "SMI_CK_MUX");
	if (IS_ERR(port->smi_ck_mux)) {
		DEV_ERR(port, "no %d SMI_CK_MUX\n", slot);

		port->smi_ck_mux = NULL;
	}

	port->axi_ck = devm_clk_get(&pdev->dev, "AXI_CLK250");
	if (IS_ERR(port->axi_ck)) {
		DEV_ERR(port, "no %d AXI_CLK250 clock\n", slot);

		port->axi_ck = NULL;
	}

	/* STI clocks of PCIe TL clock */
	port->tl108 = devm_clk_get(&pdev->dev, "TL_CLK");
	if (IS_ERR(port->tl108)) {
		DEV_ERR(port, "no %d TL_CK clock\n", slot);

		port->tl108 = NULL;
	}

	/*
	 *dev_info(dev, "Get PERST GPIO\n");
	 *port->perst_gpio = devm_gpiod_get(&pdev->dev, "perst", GPIOD_OUT_LOW);
	 *if (IS_ERR(port->perst_gpio)) {
	 *	DEV_ERR(port, "failed to get perst gpio\n", slot);
	 *
	 *	port->perst_gpio= NULL;
	 *} else {
	 *	DEV_ERR(port, "set PERST low\n");
	 *	gpiod_set_value(port->perst_gpio, true);
	 *}
	 */


	dev_info(dev, "Get PWRCTL GPIO\n");
	port->pwrctl_gpio = devm_gpiod_get(&pdev->dev, "pwrctl", GPIOD_OUT_HIGH);
	if (IS_ERR(port->pwrctl_gpio)) {
		DEV_ERR(port, "%d no pwrctl gpio\n", slot);

		port->pwrctl_gpio = NULL;
	} else	{
		DEV_ERR(port, "set pwrctl high\n");
		gpiod_set_value(port->pwrctl_gpio, true);
	}

	dev_info(dev, "get pm_support\n");
	if (of_property_read_u32(node, "pm_support", &pcie->pm_support)) {
		DEV_ERR(port, "%d no pm_support\n", slot);

		pcie->pm_support = 0;
	}

}
#endif

static int mtk_pcie_parse_port(struct mtk_pcie *pcie,
			       struct device_node *node,
			       unsigned char slot)
{
	struct mtk_pcie_port *port;
	struct device *dev = pcie->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *regs;
	char name[DTS_PORT_NODE_MAX_LEN];
	int err;

	port = devm_kzalloc(dev, sizeof(*port), GFP_KERNEL);
	if (!port)
		return -ENOMEM;

	port->pcie = pcie;

	err = of_property_read_u32(node, "num-lanes", &port->lane);
	if (err) {
		DEV_ERR(port, "missing num-lanes property\n");
		return err;
	}

	memset(name, 0, sizeof(name));
	if (snprintf(name, sizeof(name), "port%d", slot) < 0)
		memset(name, 0, sizeof(name));
	port->regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);

	port->base = devm_ioremap_resource(dev, port->regs);
	if (IS_ERR(port->base)) {
		DEV_ERR(port, "failed to map port%d base\n", slot);
		return PTR_ERR(port->base);
	}

	memset(name, 0, sizeof(name));
	if (snprintf(name, sizeof(name), "phy%d", slot) < 0)
		memset(name, 0, sizeof(name));
	regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	port->phy_base = devm_ioremap_resource(dev, regs);
	if (IS_ERR(port->phy_base)) {
		DEV_ERR(port, "failed to map phy%d base\n", slot);

		port->phy_base = NULL;
	}

	memset(name, 0, sizeof(name));
	if (snprintf(name, sizeof(name), "xiu2axi%d", slot) < 0)
		memset(name, 0, sizeof(name));
	regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	port->xiu2axi_base = devm_ioremap_resource(dev, regs);
	if (IS_ERR(port->xiu2axi_base)) {
		DEV_ERR(port, "no xiu2axi%d base\n", slot);

		port->xiu2axi_base = NULL;
	}

	memset(name, 0, sizeof(name));
	if (snprintf(name, sizeof(name), "pm_misc%d", slot) < 0)
		memset(name, 0, sizeof(name));
	regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	port->pm_misc_base = devm_ioremap_resource(dev, regs);
	if (IS_ERR(port->pm_misc_base)) {
		DEV_ERR(port, "no pm_misc%d base\n", slot);

		port->pm_misc_base = NULL;
	}

	memset(name, 0, sizeof(name));
	if (snprintf(name, sizeof(name), "phy_misc%d", slot) < 0)
		memset(name, 0, sizeof(name));
	regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	port->phy_misc_base = devm_ioremap_resource(dev, regs);
	if (IS_ERR(port->phy_misc_base)) {
		DEV_ERR(port, "no phy_misc%d base\n", slot);

		port->phy_misc_base = NULL;
	}


#if defined(M6_PHY)
	mtk_pcie_M6_phy_parse_port(pcie, port, node, slot);
#endif

	port->slot = slot;
	/*port->pcie = pcie;*/

	if (pcie->soc->setup_irq) {
		err = pcie->soc->setup_irq(port, node);
		if (err)
			return err;
	}

	INIT_LIST_HEAD(&port->list);
	list_add_tail(&port->list, &pcie->ports);

	return 0;
}

static int mtk_pcie_walk_link_speed(struct pci_dev *pdev)
{
	u16 link_up_spd, link_dn_spd;
	struct pci_dev *up_dev, *dn_dev;
	/*struct device *dev = &pdev->dev;*/
	/*struct mtk_pcie *pcie = dev_get_drvdata(dev);*/


	/* skip if current device is not PCI express capable */
	/* or is either a root port or downstream port */
	if (!pci_is_pcie(pdev))
		goto skip;

	if ((pci_pcie_type(pdev) == PCI_EXP_TYPE_DOWNSTREAM) ||
		(pci_pcie_type(pdev) == PCI_EXP_TYPE_ROOT_PORT))
		goto skip;

	/* initialize upstream/endpoint and downstream/root port device ptr */
	up_dev = pdev;
	dn_dev = pdev->bus->self;

	/* read link status register to find current speed */
	pcie_capability_read_word(up_dev, PCI_EXP_LNKSTA, &link_up_spd);
	link_up_spd &= PCI_EXP_LNKSTA_CLS;
	/* if (pcie->log_level >= MTK_PCIE_LOG_INFO)
	 *	dev_info(&pdev->dev, "EP spd: 0x%x\n", link_up_spd);
	 */
	pcie_capability_read_word(dn_dev, PCI_EXP_LNKSTA, &link_dn_spd);
	link_dn_spd &= PCI_EXP_LNKSTA_CLS;
	/* if (pcie->log_level >= MTK_PCIE_LOG_INFO)
	 *	dev_info(&pdev->dev, "RC spd: 0x%x\n", link_dn_spd);
	 */

	return true;
skip:
	return false;
}

static int mtk_pcie_link_speed(void)
{
	struct pci_dev *pdev = NULL;
	int ret = false;

	for_each_pci_dev(pdev) {
		if (mtk_pcie_walk_link_speed(pdev))
			ret = true;
	}

	return ret;
}


static int mtk_pcie_setup(struct mtk_pcie *pcie)
{
	struct device *dev = pcie->dev;
	struct device_node *node = dev->of_node, *child;
	struct of_pci_range_parser parser;
	struct of_pci_range range;
	struct resource res;
	struct mtk_pcie_port *port, *tmp;
	int err;

	if (of_pci_range_parser_init(&parser, node)) {
		if (pcie->log_level >= MTK_PCIE_LOG_ERR)
			dev_err(dev, "missing \"ranges\" property\n");
		return -EINVAL;
	}

	memset(&res, 0, sizeof(res));
	for_each_of_pci_range(&parser, &range) {
		err = of_pci_range_to_resource(&range, node, &res);
		if (err < 0)
			return err;

		switch (res.flags & IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:
			pcie->offset.io = res.start - range.pci_addr;

			memcpy(&pcie->pio, &res, sizeof(res));
			pcie->pio.name = node->full_name;

			pcie->io.start = range.cpu_addr;
			pcie->io.end = range.cpu_addr + range.size - 1;
			pcie->io.flags = IORESOURCE_MEM;
			pcie->io.name = "I/O";

			memcpy(&res, &pcie->io, sizeof(res));
			break;

		case IORESOURCE_MEM:
			pcie->offset.mem = res.start - range.pci_addr;

			memcpy(&pcie->mem, &res, sizeof(res));
			pcie->mem.name = "non-prefetchable";
			break;
		}
	}

	err = of_pci_parse_bus_range(node, &pcie->busn);
	if (err < 0) {
		if (pcie->log_level >= MTK_PCIE_LOG_ERR)
			dev_err(dev, "failed to parse bus ranges property: %d\n", err);
		pcie->busn.name = node->name;
		pcie->busn.start = 0;
		pcie->busn.end = 0xff;
		pcie->busn.flags = IORESOURCE_BUS;
	}

	for_each_available_child_of_node(node, child) {
		unsigned char slot;

		err = of_pci_get_devfn(child);
		if (err < 0) {
			if (pcie->log_level >= MTK_PCIE_LOG_ERR)
				dev_err(dev, "failed to parse devfn: %d\n", err);
			goto error_put_node;
		}

		slot = PCI_SLOT(err);

		err = mtk_pcie_parse_port(pcie, child, slot);
		if (err)
			goto error_put_node;
	}

	/* enable each port, and then check link status */
	list_for_each_entry_safe(port, tmp, &pcie->ports, list)
		mtk_pcie_enable_port(port);

	/* power down PCIe subsys if slots are all empty (link down) */
	if (list_empty(&pcie->ports))
		mtk_pcie_subsys_powerdown(pcie);

	return 0;

error_put_node:
	of_node_put(child);
	return err;
}

static int mtk_pcie_request_resources(struct mtk_pcie *pcie)
{
	struct pci_host_bridge *host = pci_host_bridge_from_priv(pcie);
	struct list_head *windows = &host->windows;
	struct device *dev = pcie->dev;
	int err;

	pci_add_resource_offset(windows, &pcie->pio, pcie->offset.io);
	pci_add_resource_offset(windows, &pcie->mem, pcie->offset.mem);
	pci_add_resource(windows, &pcie->busn);

	err = devm_request_pci_bus_resources(dev, windows);
	if (err < 0)
		return err;

	pci_remap_iospace(&pcie->pio, pcie->io.start);

	return 0;
}

static int mtk_pcie_register_host(struct pci_host_bridge *host)
{
	struct mtk_pcie *pcie = pci_host_bridge_priv(host);
	struct pci_bus *child;
	int err;

	host->busnr = pcie->busn.start;
	host->dev.parent = pcie->dev;
	host->ops = pcie->soc->ops;
	host->map_irq = of_irq_parse_and_map_pci;
	host->swizzle_irq = pci_common_swizzle;
	host->sysdata = pcie;

	err = pci_scan_root_bus_bridge(host);
	if (err < 0)
		return err;

	pci_bus_size_bridges(host->bus);
	pci_bus_assign_resources(host->bus);

	list_for_each_entry(child, &host->bus->children, node)
		pcie_bus_configure_settings(child);

	pci_bus_add_devices(host->bus);

	return 0;
}


/* PCIe sysfs debug tools */
static ssize_t log_level_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mtk_pcie *pcie = dev_get_drvdata(dev);
	ssize_t retval;
	int level;

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < MTK_PCIE_LOG_NONE) || (level > MTK_PCIE_LOG_DEBUG))
			retval = -EINVAL;
		else
			pcie->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t log_level_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct mtk_pcie *pcie = dev_get_drvdata(dev);

	return sprintf(buf, "none:\t\t%d\nmute:\t\t%d\nerror:\t\t%d\n"
			"info:\t\t%d\ndebug:\t\t%d\ncurrent:\t%d\n",
			MTK_PCIE_LOG_NONE, MTK_PCIE_LOG_MUTE, MTK_PCIE_LOG_ERR,
			MTK_PCIE_LOG_INFO, MTK_PCIE_LOG_DEBUG, pcie->log_level);
}

static DEVICE_ATTR_RW(log_level);


static int mtk_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_pcie *pcie;
	struct pci_host_bridge *host;
	int err;

	/*
	 *struct pci_dev *pcidev;
	 *void __iomem *bar;
	 *int i;
	 */

	dev_info(dev, "[PCIE] pcie-ms-mtk %s\n", VERSION);

	host = devm_pci_alloc_host_bridge(dev, sizeof(*pcie));
	if (!host)
		return -ENOMEM;

	pcie = pci_host_bridge_priv(host);

	pcie->dev = dev;
	pcie->soc = of_device_get_match_data(dev);
	platform_set_drvdata(pdev, pcie);
	INIT_LIST_HEAD(&pcie->ports);

	/* debug */
	/* pcie->log_level = MTK_PCIE_LOG_ERR;*/

	err = mtk_pcie_setup(pcie);
	if (err)
		return err;

	/* create log_level */
	pcie->log_level = MTK_PCIE_LOG_ERR;
	err = device_create_file(dev, &dev_attr_log_level);
	if (err) {
		dev_info(dev, "sysfs create failed\n");
		return err;
	}


	err = mtk_pcie_request_resources(pcie);
	if (err)
		goto put_resources;

	err = mtk_pcie_register_host(host);
	if (err)
		goto put_resources;

	mtk_pcie_link_speed();

	/* debug MMIO */
	/*
	 *dev_info(dev, "Debug BAR\n");
	 *pcidev = to_pci_dev(dev);
	 *dev_info(dev, "BAR = %lx\n", 0x0C1000000);
	 *bar = ioremap(pci_resource_start(pdev, 0), 0x4000);
	 *dev_info(dev, "port->BAR = %lx\n", (u64)bar);
	 *for (i = 0; i < 256; i++) {
	 *	dev_info(dev, "%d = 0x%x\n", readl(bar + i));
	 *}
	 */

	return 0;

put_resources:
	if (!list_empty(&pcie->ports))
		mtk_pcie_put_resources(pcie);

	return err;
}

static void mtk_pcie_free_resources(struct mtk_pcie *pcie)
{
	struct pci_host_bridge *host = pci_host_bridge_from_priv(pcie);
	struct list_head *windows = &host->windows;

	pci_unmap_iospace(&pcie->pio);
	pci_free_resource_list(windows);
}

int mtk_pcie_hook_root_port_virq(struct pci_dev *dev, int virq)
{
	struct mtk_pcie_port *port;

	port = mtk_pcie_find_port(dev->bus, dev->devfn);
	if (!port)
		return -ENODEV;

	port->virq = virq;

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_pcie_hook_root_port_virq);

static int mtk_pcie_suspend_noirq(struct device *dev)
{

	struct mtk_pcie *pcie = dev_get_drvdata(dev);
/*	const struct mtk_pcie_soc *soc = pcie->soc; */
	struct mtk_pcie_port *port;

	if (pcie->log_level >= MTK_PCIE_LOG_INFO)
		dev_info(dev, "PCIe PM suspend pm_support %d\n", pcie->pm_support);

	if (!pcie->pm_support)
		return 0;

	list_for_each_entry(port, &pcie->ports, list) {
		//clk_disable_unprepare(port->ahb_ck);
		//clk_disable_unprepare(port->sys_ck);
#if defined(M6_PHY)
		if (port->tl108)
			clk_disable_unprepare(port->tl108);
		if (port->axi_ck)
			clk_disable_unprepare(port->axi_ck);
#endif
		phy_power_off(port->phy);
	}

	/* Assert PREST# */
	writel(readl(port->base + PCIE_RST_CTRL_REG) | PCIE_PE_RSTB,
			port->base + PCIE_RST_CTRL_REG);

	return 0;
}

static int mtk_pcie_resume_noirq(struct device *dev)
{


	struct mtk_pcie *pcie = dev_get_drvdata(dev);
	const struct mtk_pcie_soc *soc = pcie->soc;
	struct mtk_pcie_port *port;
	int ret;

	if (pcie->log_level >= MTK_PCIE_LOG_INFO)
		dev_info(dev, "PCIe PM resume pm_support %d\n", pcie->pm_support);

	if (!pcie->pm_support)
		return 0;

	list_for_each_entry(port, &pcie->ports, list) {
		//phy_power_on(port->phy);
		/*clk_prepare_enable(port->sys_ck);*/
		/*clk_prepare_enable(port->ahb_ck);*/

		ret = soc->startup(port);
		if (ret) {
			DEV_ERR(port, "Port%d link down\n", port->slot);
			phy_power_off(port->phy);
			/*clk_disable_unprepare(port->sys_ck);*/
			/*clk_disable_unprepare(port->ahb_ck);*/
#if defined(M6_PHY)
		if (port->tl108)
			clk_disable_unprepare(port->tl108);
		if (port->axi_ck)
			clk_disable_unprepare(port->axi_ck);
#endif
			return ret;
		}

		if (IS_ENABLED(CONFIG_PCI_MSI))
			mtk_pcie_enable_msi(port);
	}

	return 0;
}

static int mtk_pcie_remove(struct platform_device *pdev)
{
	struct mtk_pcie *pcie = platform_get_drvdata(pdev);
	struct pci_host_bridge *host = pci_host_bridge_from_priv(pcie);

	pci_stop_root_bus(host->bus);
	pci_remove_root_bus(host->bus);
	mtk_pcie_free_resources(pcie);

/*	irq will release & free later in put resource
 *	if (IS_ENABLED(CONFIG_PCI_MSI))
 *		mtk_pcie_irq_teardown(pcie);
 */

	if (!list_empty(&pcie->ports))
		mtk_pcie_put_resources(pcie);

	/* remove log_level */
	device_remove_file(&pdev->dev, &dev_attr_log_level);

	return 0;
}


static const struct dev_pm_ops mtk_pcie_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(mtk_pcie_suspend_noirq,
				      mtk_pcie_resume_noirq)
};


static const struct mtk_pcie_soc mtk_pcie_gen3 = {
	.ops = &mtk_pcie_ops,
	.startup = mtk_pcie_startup_port,
	.setup_irq = mtk_pcie_setup_irq,
};

#define MS_MTK_STD_PCIE_GEN3	"mediatek,pcie-gen3"
#define MS_MTK_PCIE			"mediatek,mtk-dtv-pcie"
static const struct of_device_id mtk_pcie_of_match[] = {
	{ .compatible = MS_MTK_STD_PCIE_GEN3, .data = &mtk_pcie_gen3 },
	{ .compatible = MS_MTK_PCIE, .data = &mtk_pcie_gen3 },
	{},
};

static struct platform_driver mtk_pcie_driver = {
	.probe = mtk_pcie_probe,
	.remove = mtk_pcie_remove,
	.driver = {
		.name = "mtk-pcie",
		.of_match_table = mtk_pcie_of_match,
		.suppress_bind_attrs = true,
		.pm = &mtk_pcie_pm_ops,
	},
};

module_platform_driver(mtk_pcie_driver);
MODULE_LICENSE("GPL");
