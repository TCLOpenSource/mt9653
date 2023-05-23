/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * MediaTek xHCI Host Controller Driver
 *
 * Copyright (c) 2021 MediaTek Inc.
 * Author:
 *  Yun-Chien Yu <yun-chien.yu@mediatek.com>
 */

#ifndef _MTK_MISC_TV_H_
#define _MTK_MISC_TV_H_

#include <linux/gpio/consumer.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

/* global symbol of mtu3 to pass drd status */
extern int mtu3_dev_en;
#define MTU3_DEV_PORT   0

/* proc/mtk_usb files */
#define MTK_USB_FILE_ROOT		"mtk_usb"
#define MTK_USB_FILE_ROOT_PM		"mtk_usb_pm"

#define VBUS_POWER_STATE_FILE		"vbus_pwr_state"
#define VBUS_CONTROL_MODE_FILE		"vbus_ctrl_mode"

#define MTK_REPORT_FILE			"m_report"
#define MTK_OVER_CURRENT_STATE_FILE	"m_oc_state"
#define MTK_VBUS_FORCE_FILE		"m_vbus_force"
#define MTK_VBUS_ALWAYS_FILE		"m_vbus_always"

/* fool: proc/fool files */
#define MTK_USB_FOOL_DIR		"fool"

#define MTK_FOOL_HC_HALTED_FILE		"f_hc_halted"

/* vbus control: proc/v_ctrl */
#define MTK_VBUS_CTRL_DIR		"v_ctrl"
#define MTK_VBUS_CTRL_MODE_FILE		"c_mode"
#define MTK_VBUS_POWER_STATE_FILE	"p_state"

/* misc_1 bank */
#define SSUSB_MISC_1_REG_00	0x00	/* reg_0000_x32_ssusb_misc_1 */
#define SSUSB_EMI_SEL		BIT(0)	/* reg_ssusb_emi_sel */
#define SSUSB_MISC_1_REG_BC	0xBC	/* reg_00BC_x32_ssusb_misc_1 */
#define SSUSB_UTMI_SEL		BIT(0)	/* reg_utmi_sel */
#define SSUSB_MISC_1_REG_C0	0xC0	/* reg_00C0_x32_ssusb_misc_1 */
#define SSUSB_I2C_EN		BIT(1)	/* dummy[0]:ssusb_i2c_en */
#define U3FPGA_USB_EN		BIT(2)	/* dummy[1]:u3fpga_usb_en */
#define USB20_LINESTATE_SHBG	BIT(3)	/* dummy[2]:usb20_linestate_shbg */

struct mtk_port {
	struct xhci_hcd_mtk *mtk;
	struct gpio_desc *vbus;
	struct gpio_desc *ocp;
	u32 vbus_always_cap;
	u32 p_num;
	int u3_comp_dix;
	u32 flag;
/* bit[4:0]: vbus control reserved */
#define DIS_INIT		BIT(0)
#define DIS_RESUME		BIT(1)
#define DIS_SUSPEND		BIT(2)
#define DIS_SHUTDOWN		BIT(3)
#define C_MODE_MASK		(DIS_SHUTDOWN | DIS_INIT | DIS_RESUME | DIS_SUSPEND)
#define C_MODE_GET_VAL(val)	(val & C_MODE_MASK)
};

struct mtk_misc {
	void __iomem *misc1_regs;
	struct mtk_port **ports;
	u32 nports;
	u32 vbus_always_en;
	bool standbypower;
	struct proc_dir_entry *proc_root;
	/* last call of set_vbus proc file */
	u32 proc_vbus_log;
	/* Implementation of Port Resume Disable */
	bool prd_en;
	bool prd_port_en;
	int prd_port;
	int u3_port_shift;
};

int mtk_misc_vbus_init(struct xhci_hcd_mtk *mtk, struct platform_device *pdev);
#define mtk_misc_vbus_clear(mtk) mtk_vbus_check_set(mtk, 0, false)
#define mtk_misc_vbus_suspend(mtk) mtk_vbus_check_set(mtk, DIS_SUSPEND, false)
#define mtk_misc_vbus_resume(mtk) mtk_vbus_check_set(mtk, DIS_RESUME, true)
#define mtk_misc_vbus_shutdown(mtk) mtk_vbus_check_set(mtk, DIS_SHUTDOWN, false)

#endif	/* _MTK_MISC_TV_H_ */
