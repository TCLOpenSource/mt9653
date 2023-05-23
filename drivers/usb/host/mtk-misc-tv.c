// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * MediaTek xHCI Host Controller Driver
 *
 * Copyright (c) 2021 MediaTek Inc.
 * Author:
 *  Yun-Chien Yu <yun-chien.yu@mediatek.com>
 */

#include <trace/hooks/usb.h>

static void mtk_port_flag_mod(struct mtk_port *m_port, u32 flag, bool set)
{
	struct xhci_hcd_mtk *mtk = m_port->mtk;
	u32 tmp = m_port->flag;

	if (set)
		m_port->flag |= flag;
	else
		m_port->flag &= ~flag;

	dev_info(mtk->dev, "[Vbus] P%d %s flag(0x%x) 0x%02x -> 0x%02x\n",
		m_port->p_num, set ? "set" : "clr", flag, tmp,  m_port->flag);
}

static int mtk_vbus_ctl(struct mtk_port *m_port, bool on)
{
	if (!m_port->vbus)
		return -EINVAL;

	dev_info(m_port->mtk->dev,
		"[Vbus] port%d %s\n", m_port->p_num, on ? "On" : "Off");

	if (on ^ gpiod_get_value(m_port->vbus))
		gpiod_set_value(m_port->vbus, on);
	return 0;
}

int mtk_misc_vbus_init(struct xhci_hcd_mtk *mtk, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_misc *m_misc = &mtk->m_misc;
	struct mtk_port *m_port;
	u32 flag;
	int i;

	for (i = 0; i < m_misc->nports; i++) {
		m_port = m_misc->ports[i];
		flag = m_port->flag;

		m_port->vbus =
			devm_gpiod_get_index_optional(
				dev, "vbus", i, flag & DIS_INIT ?
				GPIOD_OUT_LOW : GPIOD_OUT_HIGH);

		if (IS_ERR(m_port->vbus)) {
			m_port->vbus = NULL;
			dev_info(m_port->mtk->dev,
				"[Vbus] port%d init fail!\n", m_port->p_num);
		} else {
			dev_info(m_port->mtk->dev,
				"[Vbus] port%d init ok! [%s]\n",
				m_port->p_num, flag & DIS_INIT ? "off" : "on");
		}
	}
	return 0;
}

int mtk_vbus_check_set(struct xhci_hcd_mtk *mtk, u32 mask, bool val)
{
	struct mtk_misc *m_misc = &mtk->m_misc;
	int i;

	for (i = 0; i < m_misc->nports; i++) {
		if (!(m_misc->ports[i]->flag & mask))
			mtk_vbus_ctl(m_misc->ports[i], val);
	}
	return 0;
}

#ifdef CONFIG_MSTAR_ARM_BD_FPGA
static int mtk_gpio_init(struct xhci_hcd_mtk *mtk, struct platform_device *pdev)
{ return 0; }
#else
static int mtk_gpio_init(struct xhci_hcd_mtk *mtk, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct mtk_misc *m_misc = &mtk->m_misc;
	struct mtk_port *m_port;
	int i, count, rc;
	u32 tmp = 0;

	count = gpiod_count(dev, "vbus");
	if (count <= 0)
		return count;

	m_misc->nports = count;

	m_misc->ports = devm_kzalloc(dev,
		sizeof(struct mtk_port *) * count, GFP_KERNEL);
	if (!m_misc->ports)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		m_misc->ports[i] = devm_kzalloc(dev,
			sizeof(struct mtk_port), GFP_KERNEL);
		if (!m_misc->ports[i])
			return -ENOMEM;

		m_port = m_misc->ports[i];
		m_port->mtk = mtk;
		m_port->p_num = i;

		if (m_misc->standbypower) {
			/* assume vbus of PM controller is always on */
			mtk_port_flag_mod(m_port,
				DIS_SHUTDOWN | DIS_SUSPEND | DIS_RESUME, true);
		} else if (mtu3_dev_en && (i == MTU3_DEV_PORT)) {
			/* assume vbus of dev mode port is always off */
			mtk_port_flag_mod(m_port,
				DIS_INIT | DIS_SUSPEND | DIS_RESUME, true);
		}

		/* get vbus gpio later, to avoid modify vbus gpio 2 times
		 * one for get and one for init setting while probe.
		 * for u3 complibility, get vbus with good default value
		 * in mtk_misc_vbus_init() after driver ready.
		 */

		/* get ocp gpio from dts. set NULL if empty */
		m_port->ocp =
			devm_gpiod_get_index_optional(dev, "ocp", i, GPIOD_IN);

		if (IS_ERR(m_port->ocp))
			m_port->ocp = NULL;

		/* get u3 companay index from dts.
		 * it is indexed to U3 Port Control Register in IPPC.
		 * set -1 if empty or larger than 4.
		 */
		rc = of_property_read_u32_index(node, "u3-comp-dix", i, &tmp);

		if (rc < 0 || tmp > 0x4)
			m_port->u3_comp_dix = -1;
		else
			m_port->u3_comp_dix = tmp;

		/* get vbus_always_cap from dts.
		 * set 0 if empty.
		 */
		rc = of_property_read_u32_index(node, "vbus-always-on-cap",
			i, &tmp);

		if (rc < 0)
			m_port->vbus_always_cap = 0;
		else
			m_port->vbus_always_cap = tmp;
	}
	return 0;
}
#endif

/* sysfs APIs: may need to change to procfs in the feature */
static ssize_t port_resume_disable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct mtk_misc *m_misc = &mtk->m_misc;

	return scnprintf(buf, 4, "%d\n", m_misc->prd_en);
}

static ssize_t port_resume_disable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct mtk_misc *m_misc = &mtk->m_misc;
	int prd_port = m_misc->prd_port;
	int prd_en;

	if (kstrtoint(buf, 10, &prd_en))
		return -EINVAL;

	if (prd_en < 0 || prd_en > 1) {
		dev_err(mtk->dev, "[PRD] set prd_en err %d\n", prd_en);
		return -EINVAL;
	}

	mtk_port_flag_mod(m_misc->ports[prd_port], DIS_RESUME, !!prd_en);

	m_misc->prd_en = !!prd_en;

	dev_err(mtk->dev,
		"[PRD] prd_en %d, prd_port %d\n", m_misc->prd_en, prd_port);
	return count;
}
static DEVICE_ATTR_RW(port_resume_disable);

static ssize_t port_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct mtk_misc *m_misc = &mtk->m_misc;
	int prd_port = m_misc->prd_port;

	if (!m_misc->prd_en) {
		dev_err(mtk->dev, "[PRD] ERR: prd_en is off\n");
		goto out;
	}

	/* enable prd port once each resume */
	if (m_misc->prd_port_en)
		goto out;

	m_misc->prd_port_en = true;

	/* enable vbus */
	dev_err(mtk->dev, "[PRD] power prd_port %d on\n", prd_port);
	mtk_vbus_ctl(m_misc->ports[prd_port], true);
out:
	return count;
}
static DEVICE_ATTR_WO(port_enable);

static int m_report_seq_show(struct seq_file *seq, void *offset)
{
	struct xhci_hcd_mtk *mtk = seq->private;
	struct mtk_misc *m_misc = &mtk->m_misc;
	struct mtk_port *m_port;
	int i;

	for (i = 0; i < m_misc->nports; i++) {
		m_port = m_misc->ports[i];

		seq_printf(seq, "Port%d:\n", i);
		seq_printf(seq, "  #u3-comp\t[%d]\n", m_port->u3_comp_dix);
		seq_printf(seq, "  #always-cap\t[%c]\n",
			m_port->vbus_always_cap ? 'O' : 'X');
		seq_printf(seq, "  #flag\t\t[0x%x]\n", m_port->flag);

		seq_printf(seq, "  #vbus-gpio\t[%c]\n",
				m_port->vbus ? 'O' : 'X');
		if (m_port->vbus) {
			seq_printf(seq, "  \t- active [%c]\n",
				gpiod_is_active_low(m_port->vbus) ? 'L' : 'H');
			seq_printf(seq, "  \t- status [%s]\n",
				gpiod_get_value(m_port->vbus) ? " On" : "Off");
		}

		seq_printf(seq, "  #ocp-gpio\t[%c]\n",
				m_port->ocp ? 'O' : 'X');
		if (m_port->ocp) {
			seq_printf(seq, "  \t- active [%c]\n",
				gpiod_is_active_low(m_port->ocp) ? 'L' : 'H');
			seq_printf(seq, "  \t- status [%s]\n",
				gpiod_get_value(m_port->ocp) ? " On" : "Off");
		}
	}
	return 0;
}

static int m_oc_state_seq_show(struct seq_file *seq, void *offset)
{
	struct xhci_hcd_mtk *mtk = seq->private;
	struct mtk_misc *m_misc = &mtk->m_misc;
	int i, val;
	u32 out = 0;

	for (i = 0; i < m_misc->nports; i++) {
		if (m_misc->ports[i]->ocp) {
			val = gpiod_get_value(m_misc->ports[i]->ocp);
			if (val)
				out |= BIT(0) << i;
		}
	}

	seq_printf(seq, "0x%02x\n", out);
	return 0;
}

static int m_vbus_force_show(struct seq_file *seq, void *data)
{
	struct xhci_hcd_mtk *mtk = seq->private;
	struct mtk_misc *m_misc = &mtk->m_misc;

	seq_printf(seq, "Last log: 0x%04x\n", m_misc->proc_vbus_log);
	return 0;
}

static int m_vbus_force_open(struct inode *node, struct file *file)
{
	return single_open(file, m_vbus_force_show, PDE_DATA(node));
}

static ssize_t m_vbus_force_write(struct file *file,
		const char *buffer, size_t count, loff_t *data)
{
	struct xhci_hcd_mtk *mtk = PDE_DATA(file_inode(file));
	struct mtk_misc *m_misc;
	struct mtk_port *m_port;
	int i, len, ret = 0;
	u8 force, value;
	u16 input;
	char buf[8];

	if (!mtk)
		return -EINVAL;
	m_misc = &mtk->m_misc;

	if (count <= 0)
		return -EINVAL;

	len = (count < (sizeof(buf) - 1)) ? count : (sizeof(buf) - 1);
	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou16(buf, 16, &input))
		return -EINVAL;

	m_misc->proc_vbus_log = input;

	force = input >> 8;
	value = input & 0xff;

	for (i = 0; i < m_misc->nports; i++)
		if (force & (BIT(0) << i)) {
			m_port = m_misc->ports[i];
			ret = mtk_vbus_ctl(m_port, value & (BIT(0) << i));
		}
	return ret ? 0 : len;
}

static const struct file_operations m_vbus_force_fops = {
	.owner = THIS_MODULE,
	.open = m_vbus_force_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = m_vbus_force_write,
};

static int m_vbus_always_show(struct seq_file *seq, void *data)
{
	struct xhci_hcd_mtk *mtk = seq->private;
	struct mtk_misc *m_misc = &mtk->m_misc;

	seq_printf(seq, "%01x\n", m_misc->vbus_always_en);
	return 0;
}

static int m_vbus_always_open(struct inode *node, struct file *file)
{
	return single_open(file, m_vbus_always_show, PDE_DATA(node));
}

static ssize_t m_vbus_always_write(struct file *file,
		const char *buffer, size_t count, loff_t *data)
{
	struct xhci_hcd_mtk *mtk = PDE_DATA(file_inode(file));
	struct mtk_misc *m_misc;
	struct mtk_port *m_port;
	int len, i;
	u16 input;
	char buf[8];

	if (!mtk)
		return -EINVAL;
	m_misc = &mtk->m_misc;

	if (count <= 0)
		return -EINVAL;

	len = (count < (sizeof(buf) - 1)) ? count : (sizeof(buf) - 1);
	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou16(buf, 16, &input))
		return -EINVAL;

	for (i = 0; i < m_misc->nports; i++) {
		m_port = m_misc->ports[i];
		if (m_port->vbus_always_cap)
			mtk_port_flag_mod(m_port, DIS_SUSPEND, !!input);
	}
	m_misc->vbus_always_en = !!input;

	return len;
}

static const struct file_operations m_vbus_always_fops = {
	.owner = THIS_MODULE,
	.open = m_vbus_always_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = m_vbus_always_write,
};

/* fool: functions */
static int f_hc_halted_show(struct seq_file *seq, void *offset)
{
	struct xhci_hcd_mtk *mtk = seq->private;
	struct xhci_hcd *xhci = hcd_to_xhci(mtk->hcd);
	u32 halted;

	halted = readl(&xhci->op_regs->status) & STS_HALT;
	seq_printf(seq, "%d\n", halted);
	return 0;
}

/* new APIs for vbus control
 * proc
 * └── mtk_usb
 *     └── v_ctrl
 *         ├── p0
 *         │   ├── c_mode
 *         │   └── p_state
 *         ├── p1
 *         │   ├── c_mode
 *         │   └── p_state
 *         └── ...
 * c_mode: ro
 *    vbus control mode
 *    bit0: don't power-on at init
 *    bit1: don't power-on at resume
 *    bit2: don't power-off at suspend
 * p_state: r/w
 *    vbus current power state
 *    1: on
 *    0: off
 */
#define BUF_SIZE	10

static int c_mode_show(struct seq_file *seq, void *offset)
{
	struct mtk_port *m_port = seq->private;

	seq_printf(seq, "%lx\n", C_MODE_GET_VAL(m_port->flag));
	return 0;
}

static int p_state_show(struct seq_file *seq, void *data)
{
	struct mtk_port *m_port = seq->private;

	seq_printf(seq, "%x\n", gpiod_get_value(m_port->vbus));
	return 0;
}

static int p_state_open(struct inode *node, struct file *file)
{
	return single_open(file, p_state_show, PDE_DATA(node));
}

static ssize_t p_state_write(struct file *file,
		const char *buffer, size_t count, loff_t *data)
{
	struct mtk_port *m_port = PDE_DATA(file_inode(file));
	u16 input = 0;
	int ret, len;
	char buf[BUF_SIZE];

	if (count <= 0)
		return -EINVAL;

	len = (count < (sizeof(buf) - 1)) ? count : (sizeof(buf) - 1);
	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou16(buf, 16, &input))
		return -EINVAL;

	if (input > 1)
		return -EINVAL;

	mtk_port_flag_mod(m_port, DIS_SUSPEND | DIS_RESUME, !input);

	ret = mtk_vbus_ctl(m_port, input);
	return ret ? 0 : len;
}

static const struct file_operations p_state_fops = {
	.owner = THIS_MODULE,
	.open = p_state_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = p_state_write,
};

static void mtk_user_api_create(struct xhci_hcd_mtk *mtk, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct mtk_misc *m_misc = &mtk->m_misc;
	struct proc_dir_entry *fool_dir, *vctrl_dir, *port_dir;
	char name[BUF_SIZE];
	int i, ret;

	if (m_misc->standbypower) {
		/* create /proc/mtk_usb_pm */
		m_misc->proc_root = proc_mkdir(MTK_USB_FILE_ROOT_PM, NULL);
	} else {
		/* create /proc/mtk_usb */
		m_misc->proc_root = proc_mkdir(MTK_USB_FILE_ROOT, NULL);

		if (of_property_read_u32(node,
			"resume-disable-port", &m_misc->prd_port) ||
			of_property_read_u32(node, "u3_port_shift",
			&m_misc->u3_port_shift)) {
			m_misc->prd_port = -1;
		} else {
			m_misc->prd_port_en = true;

			ret = device_create_file(dev,
				&dev_attr_port_resume_disable);
			if (ret)
				dev_err(mtk->dev, "sysfs err! (prd)\n");

			ret = device_create_file(dev, &dev_attr_port_enable);
			if (ret)
				dev_err(mtk->dev, "sysfs err! (port_en)\n");
		}
	}

	/* create /proc/mtk_usb/m_report */
	proc_create_single_data(MTK_REPORT_FILE,
		0440, m_misc->proc_root, m_report_seq_show, mtk);

	/* create /proc/mtk_usb/m_oc_state */
	proc_create_single_data(MTK_OVER_CURRENT_STATE_FILE,
		0440, m_misc->proc_root, m_oc_state_seq_show, mtk);

	/* create /proc/mtk_usb/m_vbus_force */
	proc_create_data(MTK_VBUS_FORCE_FILE,
		0640, m_misc->proc_root, &m_vbus_force_fops, mtk);

	/* create /proc/mtk_usb/m_vbus_always */
	proc_create_data(MTK_VBUS_ALWAYS_FILE,
		0640, m_misc->proc_root, &m_vbus_always_fops, mtk);

	/* fool: create /proc/mtk_usb/fool */
	fool_dir = proc_mkdir(MTK_USB_FOOL_DIR, m_misc->proc_root);

	/* fool: create /proc/mtk_usb/fool/f_hc_halted */
	proc_create_single_data(MTK_FOOL_HC_HALTED_FILE,
		0440, fool_dir, f_hc_halted_show, mtk);

	/* v_ctrl: create /proc/mtk_usb/v_ctrl */
	vctrl_dir = proc_mkdir(MTK_VBUS_CTRL_DIR, m_misc->proc_root);

	for (i = 0; i < m_misc->nports; i++) {
		/* only for ports with vbus gpio presented */
		if (!(m_misc->ports[i]->vbus))
			continue;

		/* create /proc/mtk_usb/v_ctrl/p# */
		ret = snprintf(name, BUF_SIZE, "p%x", i);
		if (ret < 0) {
			dev_err(mtk->dev, "can't create p%d proc file\n", i);
			continue;
		}

		port_dir = proc_mkdir(name, vctrl_dir);

		/* create /proc/mtk_usb/v_ctrl/p#/c_mode */
		proc_create_single_data(MTK_VBUS_CTRL_MODE_FILE,
			0440, port_dir, c_mode_show, m_misc->ports[i]);

		/* create /proc/mtk_usb/v_ctrl/p#/p_state */
		proc_create_data(MTK_VBUS_POWER_STATE_FILE,
			0640, port_dir, &p_state_fops, m_misc->ports[i]);
	}
}

static void mtk_user_api_remove(struct xhci_hcd_mtk *mtk)
{
	struct mtk_misc *m_misc = &mtk->m_misc;

	/* remove all proc files */
	proc_remove(m_misc->proc_root);

	if (!m_misc->standbypower && m_misc->prd_port != -1) {
		device_remove_file(mtk->dev, &dev_attr_port_resume_disable);
		device_remove_file(mtk->dev, &dev_attr_port_enable);
	}
}

int mtk_misc_probe(struct xhci_hcd_mtk *mtk, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct mtk_misc *m_misc = &mtk->m_misc;
	int ret;

	m_misc->standbypower =
		of_property_read_bool(node->parent, "standbypower-domain");

	ret = mtk_gpio_init(mtk, pdev);
	if (ret)
		return ret;

	mtk_user_api_create(mtk, pdev);
	return 0;
}

void mtk_misc_probe_clean(struct xhci_hcd_mtk *mtk)
{
	mtk_user_api_remove(mtk);
}

int mtk_misc_probe_post(struct xhci_hcd_mtk *mtk,
	struct usb_hcd *hcd, struct platform_device *pdev)
{
	struct device *dev = mtk->dev;
	struct mtk_misc *m_misc = &mtk->m_misc;
	struct resource *res;
	u32 temp;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "misc_1");
	if (res) {	/* misc_1 register is optional */
		m_misc->misc1_regs = devm_ioremap_resource(dev, res);

		if (IS_ERR(m_misc->misc1_regs))
			return PTR_ERR(m_misc->misc1_regs);

		/* access mem form EMI */
		temp = readl(m_misc->misc1_regs + SSUSB_MISC_1_REG_00);
		writel(temp | SSUSB_EMI_SEL,
			m_misc->misc1_regs + SSUSB_MISC_1_REG_00);

		/* select UTMI from SSUSB */
		temp = readl(m_misc->misc1_regs + SSUSB_MISC_1_REG_BC);
		writel(temp | SSUSB_UTMI_SEL,
			m_misc->misc1_regs + SSUSB_MISC_1_REG_BC);
#ifdef CONFIG_MSTAR_ARM_BD_FPGA
		/* FPGA special setting dummy[2:0] => XHCI: 0x2 */
		temp = readl(m_misc->misc1_regs + SSUSB_MISC_1_REG_C0);
		temp &= ~(SSUSB_I2C_EN);
		temp |= U3FPGA_USB_EN;
		temp &= ~(USB20_LINESTATE_SHBG);
		writel(temp, m_misc->misc1_regs + SSUSB_MISC_1_REG_C0);
#endif
	}
	return 0;
}

int mtk_misc_remove(struct xhci_hcd_mtk *mtk)
{
	mtk_user_api_remove(mtk);
	return 0;
}

static void mtk_vh_usb_persist_overwrite_handler(void *data, struct usb_device *udev)
{
	udev->persist_enabled =
		(udev->descriptor.bDeviceClass == USB_CLASS_HUB) ? 1 : 0;

	dev_info(&udev->dev, "class %d -> persist_enabled [%d]",
		udev->descriptor.bDeviceClass, udev->persist_enabled);
}

int mtk_misc_init(struct hc_driver *drv)
{
	int ret;

	ret = register_trace_android_vh_usb_persist_overwrite(
			mtk_vh_usb_persist_overwrite_handler, NULL);
	if (ret)
		pr_err("%s: can't register vendor hook %d\n", __func__, ret);
	return ret;
}

int mtk_misc_exit(struct hc_driver *drv)
{
	int ret;

	ret = unregister_trace_android_vh_usb_persist_overwrite(
			mtk_vh_usb_persist_overwrite_handler, NULL);
	if (ret)
		pr_err("%s: can't unregister vendor hook %d\n", __func__, ret);
	return ret;
}

int mtk_misc_xhci_handshake(void __iomem *ptr, u32 mask, u32 done, int usec)
{
	u32	result;
	int	ret;

	ret = readl_poll_timeout_atomic(ptr, result,
					(result & mask) == done ||
					result == U32_MAX,
					1, usec);
	if (result == U32_MAX)		/* card removed */
		return -ENODEV;

	return ret;
}
