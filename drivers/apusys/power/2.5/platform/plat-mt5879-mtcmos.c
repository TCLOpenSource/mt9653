// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/iopoll.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "apusys_power_user.h"
#include "apu_devfreq.h"
#include "apu_clk.h"
#include "apu_io.h"
#include "apu_log.h"
#include "apu_common.h"
#include "apu_dbg.h"
#include "plat_mt5879_reg.h"
#include "plat_util.h"
#include "plat_teec_api.h"

static DEFINE_MUTEX(hw_lock);
static unsigned long power_on_cnt;

static int mtcmos_init_done;
static u32 slpprot_rdy;
static u32 vcore_sw_rst;

#ifndef MDLA_POWER_CTRL_BY_TEE
#ifdef DEBUG_POWER_FLOW

static u32 x32_aia_top = X32_AIA_TOP;
static u32 mcusys_xiu_bridge = MCUSYS_XIU_BRIDGE;

static void __iomem *reg_apusys_vcore_cg_con;
static void __iomem *reg_apu_conn_cg_con;
static void __iomem *reg_apu_mdla0_apu_mdla_cg_con;

static void ioremap_uninit(void)
{
	if (reg_apusys_vcore_cg_con)
		iounmap(reg_apusys_vcore_cg_con);
	if (reg_apu_conn_cg_con)
		iounmap(reg_apu_conn_cg_con);
	if (reg_apu_mdla0_apu_mdla_cg_con)
		iounmap(reg_apu_mdla0_apu_mdla_cg_con);
}

static int ioremap_init(void)
{
	reg_apusys_vcore_cg_con = ioremap_nocache(APUSYS_VCORE_CG_CON, PAGE_SIZE);
	reg_apu_conn_cg_con = ioremap_nocache(APU_CONN_CG_CON, PAGE_SIZE);
	reg_apu_mdla0_apu_mdla_cg_con = ioremap_nocache(APU_MDLA0_APU_MDLA_CG_CON, PAGE_SIZE);

	if (!reg_apusys_vcore_cg_con
		|| !reg_apu_conn_cg_con
		|| !reg_apu_mdla0_apu_mdla_cg_con)
		return -ENOMEM;

	return 0;
}

#else //DEBUG_POWER_FLOW

#define riu_read16(name, offset)	\
		ioread16(reg_##name + (offset << 2))

#define riu_write16(name, offset, val)	\
		iowrite16(val, reg_##name + (offset << 2))

#define riu_read32(name, offset)	\
		ioread32(reg_##name + (offset << 2))

#define riu_write32(name, offset, val)	\
		iowrite32(val, reg_##name + (offset << 2))

#define RIU2IOMAP(bank, offset)	((phys_addr_t)RIU_ADDR(bank, offset))

static void __iomem *reg_x32_aia_top;
static void __iomem *reg_mcusys_xiu_bridge;
static void __iomem *reg_apusys_vcore_cg_con;
static void __iomem *reg_apu_conn_cg_con;
static void __iomem *reg_apu_mdla0_apu_mdla_cg_con;

static void ioremap_uninit(void)
{
	if (reg_x32_aia_top)
		iounmap(reg_x32_aia_top);
	if (reg_mcusys_xiu_bridge)
		iounmap(reg_mcusys_xiu_bridge);
	if (reg_apusys_vcore_cg_con)
		iounmap(reg_apusys_vcore_cg_con);
	if (reg_apu_conn_cg_con)
		iounmap(reg_apu_conn_cg_con);
	if (reg_apu_mdla0_apu_mdla_cg_con)
		iounmap(reg_apu_mdla0_apu_mdla_cg_con);
}

static int ioremap_init(void)
{
	reg_x32_aia_top = ioremap_nocache(RIU2IOMAP(X32_AIA_TOP, 0), PAGE_SIZE);
	reg_mcusys_xiu_bridge = ioremap_nocache(RIU2IOMAP(MCUSYS_XIU_BRIDGE, 0), PAGE_SIZE);
	reg_apusys_vcore_cg_con = ioremap_nocache(APUSYS_VCORE_CG_CON, PAGE_SIZE);
	reg_apu_conn_cg_con = ioremap_nocache(APU_CONN_CG_CON, PAGE_SIZE);
	reg_apu_mdla0_apu_mdla_cg_con = ioremap_nocache(APU_MDLA0_APU_MDLA_CG_CON, PAGE_SIZE);

	if (!reg_x32_aia_top
		|| !reg_mcusys_xiu_bridge
		|| !reg_apusys_vcore_cg_con
		|| !reg_apu_conn_cg_con
		|| !reg_apu_mdla0_apu_mdla_cg_con)
		return -ENOMEM;

	return 0;
}

#endif // DEBUG_POWER_FLOW
#endif // MDLA_POWER_CTRL_BY_TEE

int mt5879_mtcmos_init(struct apu_dev *ad)
{
	if (!mtcmos_init_done) {
#ifndef MDLA_POWER_CTRL_BY_TEE
		if (ioremap_init()) {
			aprobe_err(ad->dev, "%s: ioremap_init fail\n", __func__);
			return -ENOMEM;
		}
#endif
		if (plat_teec_send_command(ad, E_MDLA_PWR_CMD_INIT_POWER, 0, NULL)) {
			aprobe_err(ad->dev, "%s: teec_power_init fail\n", __func__);
			return -EFAULT;
		}

		vcore_sw_rst = 1;
		slpprot_rdy = 0;
		mtcmos_init_done = 1;
	}

	return 0;
}

void mt5879_mtcmos_uninit(struct apu_dev *ad)
{
	if (mtcmos_init_done) {
#ifndef MDLA_POWER_CTRL_BY_TEE
		ioremap_uninit();
#endif
		mtcmos_init_done = 0;
		plat_teec_send_command(ad, E_MDLA_PWR_CMD_UNINIT_POWER, 0, NULL);
	}
}

#ifndef MDLA_POWER_CTRL_BY_TEE
static int aia_top_apusys_init(void)
{
	// apu_conn_config.csr_cg_con
	iowrite32(0x00000000, reg_apu_conn_cg_con);

	// apu_vcore_config.csr_cg_con
	iowrite32(0x00000000, reg_apusys_vcore_cg_con);

	// apu_mdla_config.csr_cg_con
	iowrite32(0x00000000, reg_apu_mdla0_apu_mdla_cg_con);

	return 0;
}

static int aia_top_mcusys_xiu_bridge_init(void)
{
	// [0] reg_mcu2apu_sw_reset = 0
	riu_write16(mcusys_xiu_bridge, 0x08, 0x0000);

	return 0;
}

int dtv_apu_mt5879_mtcmos_on(struct apu_dev *ad, int force)
{
	int ret = 0;
	u16 regv;

	if (!mtcmos_init_done) {
		if (mt5879_mtcmos_init(ad)) {
			aprobe_err(ad->dev, "%s(): init fail\n",
					__func__);
			return -EFAULT;
		}
	}

	mutex_lock(&hw_lock);

	if (!force && power_on_cnt++) {
		arpc_info(ad->dev, "%s(): is on, count:%lu\n",
				__func__, power_on_cnt);
		ret = false;
		goto out;
	}

	// release conn reset
	regv = riu_read16(x32_aia_top, 0x02);
	regv |= BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7);
		// set [7] [6] [5] [4] [3]
		// [7] reg_rstz_tcm
		// [6] reg_rstz_xtal_24m
		// [5] reg_rstz_mcu
		// [4] reg_rstz_imi
		// [3] reg_rstz_bus
	riu_write16(x32_aia_top, 0x02, regv);

	// clk on 1
	regv = riu_read16(x32_aia_top, 0x01);
	regv &= ~(BIT(0) | BIT(1) | BIT(3));
		// clear [3] [1] [0]
		// set [0] reg_apu_vcore_clk_dis
		// set [1] reg_apu_conn_clk_dis
		// set [3] reg_apu_tcm_clk_dis
	riu_write16(x32_aia_top, 0x01, regv);

	// clk on 2
	// release conn clk_en
	regv = riu_read16(x32_aia_top, 0x00);
	regv |= BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(5);
		// set [5:3] [1:0]
		// [5] reg_aia_tcm_en
		// [4] reg_aia_imi_en
		// [3] reg_aia_mcu_en
		// [1] reg_aia_bus_die_en
		// [0] reg_aia_en
	riu_write16(x32_aia_top, 0x00, regv);

	// sram pd dis
	// [0] reg_apu_edma_sram_pd = 0
	// [3] reg_apu_tcm_sram_pd = 0
	riu_write16(x32_aia_top, 0x07, 0x0000);

	// [5] reg_apu_tcm_sram_wake_ack
	regv = BIT(5);
	while ((riu_read16(x32_aia_top, 0x07) & regv) != regv)
		;

	// mtcmos on1
	// mdla power on
	regv = riu_read16(x32_aia_top, 0x05);
	regv |= BIT(0);
		// set [0] reg_apu_mdla_pwr_on = 1
	riu_write16(x32_aia_top, 0x05, regv);

#ifndef FPGA
	// mtcmos on1 ack
	// polling pwr on ack
	// polling [2] == 1
	regv = BIT(2);
	while ((riu_read16(x32_aia_top, 0x05) & regv) != regv)
		;
#endif
	// delay 32us
	udelay(32);

	// mtcmos on2
	regv = riu_read16(x32_aia_top, 0x05);
	regv |= BIT(1);
		// set [1] reg_apu_mdla_pwr_on_2nd = 1
	riu_write16(x32_aia_top, 0x05, regv);

#ifndef FPGA
	// mtcmos on2 ack
	// polling pwr_on_ack_2nd
	// polling [3] == 1
	regv = BIT(3);
	while (!(riu_read16(x32_aia_top, 0x05) & regv))
		;
#endif

	//sram pd dis
	regv = riu_read16(x32_aia_top, 0x06);
	regv &= ~(BIT(0) | BIT(3));
		// [0] reg_apu_mdla_sram_pd0
		// [3] reg_apu_mdla_sram_pd1
	riu_write16(x32_aia_top, 0x06, regv);

	// sram pd dis ack
	// polling [2] reg_apu_mdla_sram_wake0_ack
	// polling [5] reg_apu_mdla_sram_wake1_ack
	regv = BIT(2) | BIT(5);
	while ((riu_read16(x32_aia_top, 0x06) & regv) != regv)
		;

	// dig isolation dis
	// mdla iso release
	regv = riu_read16(x32_aia_top, 0x04);
	regv &= ~BIT(0);
		// clear [0] reg_apu_mdla_iso_en = 0
	riu_write16(x32_aia_top, 0x04, regv);

	// reset release
	// release mdla reset
	regv = riu_read16(x32_aia_top, 0x02);
	regv |= BIT(0) | BIT(1) | BIT(2);
		// set [2:0]
		// [0] reg_rstz_mdla_pwr
		// [1] reg_rstz_mdla_preset
		// [2] reg_rstz_mdla_swreset
	riu_write16(x32_aia_top, 0x02, regv);

	// clk on 1
	//mdla clk dis
	regv = riu_read16(x32_aia_top, 0x01);
	regv &= ~BIT(2);
		// clear [2] reg_apu_mdla_clk_dis = 0
	riu_write16(x32_aia_top, 0x01, regv);

	// clk on 2
	//mdla clk en
	regv = riu_read16(x32_aia_top, 0x00);
	regv |= BIT(2);
		// clear [2] reg_aia_mdla_en = 0
	riu_write16(x32_aia_top, 0x00, regv);

	// slpprot dis
	// reg_apu_mdla_*_slpprot_en = 0
	// [6] reg_apu_mdla_apb_slpprot_en
	// [3] reg_apu_mdla_m1_slpprot_en
	// [0] reg_apu_mdla_m0_slpprot_en
	riu_write16(x32_aia_top, 0x10, 0x0000);

	// slpprot rdy
	if (slpprot_rdy) {
		// polling [7] [4] [1] == 1
		regv = BIT(1) | BIT(4) | BIT(7);
		while (riu_read16(x32_aia_top, 0x10) & regv)
			;
	}

	//bypass security check
	regv = riu_read16(x32_aia_top, 0x26);
	regv |= (0x1 << 2);
	// [2] reg_secure_check_overwrite = 1
	riu_write16(x32_aia_top, 0x26, regv);

	aia_top_apusys_init();
	aia_top_mcusys_xiu_bridge_init();

	arpc_info(ad->dev, "%s(): success\n", __func__);
out:
	mutex_unlock(&hw_lock);

	return ret;
}

void dtv_apu_mt5879_mtcmos_off(struct apu_dev *ad, int force)
{
	u16 regv;

	if (!mtcmos_init_done) {
		arpc_err(ad->dev, "%s(): not init\n", __func__);
		return;
	}

	mutex_lock(&hw_lock);

	if (!power_on_cnt) {
		arpc_info(ad->dev, "%s(): is disabled\n", __func__);
		goto out;
	}

	if (!force && power_on_cnt-- > 1) {
		arpc_info(ad->dev, "%s(): not off, count:%lu\n",
				__func__, power_on_cnt);
		goto out;
	}

	// slpprot en
	regv = riu_read16(x32_aia_top, 0x10);
	// set [6] [3] [0]
	// [6] reg_apu_mdla_apb_slpprot_en
	// [3] reg_apu_mdla_m1_slpprot_en
	// [0] reg_apu_mdla_m0_slpprot_en
	regv |= (BIT(6) | BIT(3) | BIT(0));
	riu_write16(x32_aia_top, 0x10, regv);

	// slpprot rdy
	if (slpprot_rdy) {
		// polling [7] [4] [1] == 1
		regv = BIT(1) | BIT(4) | BIT(7);
		while ((riu_read16(x32_aia_top, 0x10) & regv) != regv)
			;
	}

	// = Core2 power off =
	//sram pd
	regv = riu_read16(x32_aia_top, 0x06);
	// set [0] reg_apu_mdla_sram_pd0 = 1
	// set [3] reg_apu_mdla_sram_pd1 = 1
	regv |= BIT(0) | BIT(3);
	riu_write16(x32_aia_top, 0x06, regv);


	// sram pd ack
	// polling [1] reg_apu_mdla_sram_pd0_ack == 1
	// polling [4] reg_apu_mdla_sram_pd1_ack == 1
	regv = BIT(1) | BIT(4);
	while ((riu_read16(x32_aia_top, 0x06) & regv) != regv)
		;

	// clk off1
	regv = riu_read16(x32_aia_top, 0x01);
	regv |= BIT(2);
		// set [2] reg_apu_mdla_clk_dis = 1
	riu_write16(x32_aia_top, 0x01, regv);

	// clk off2
	regv = riu_read16(x32_aia_top, 0x00);
	regv &= ~BIT(2);
		// clear [2] reg_aia_mdla_en = 0
	riu_write16(x32_aia_top, 0x00, regv);

	// reset assert
	regv = riu_read16(x32_aia_top, 0x02);
	regv &= ~(BIT(0) | BIT(1) | BIT(2));
		// clear [2:0]
		// set [0] reg_rstz_mdla_pwr
		// set [1] reg_rstz_mdla_preset
		// set [2] reg_rstz_mdla_swreset
	riu_write16(x32_aia_top, 0x02, regv);

	// dig isolation en
	regv = riu_read16(x32_aia_top, 0x04);
	regv |= BIT(0);
		// set [0] reg_apu_mdla_iso_en = 1
	riu_write16(x32_aia_top, 0x04, regv);

	// mtcmos off1
	regv = riu_read16(x32_aia_top, 0x05);
	regv &= ~BIT(0);
		// clear [0] reg_apu_mdla_pwr_on = 0
	riu_write16(x32_aia_top, 0x05, regv);

#ifndef FPGA
	// mtcmos off1 ack
	// polling [2] == 0 reg_apu_mdla_pwr_on_ack
	regv = BIT(2);
	while (riu_read16(x32_aia_top, 0x05) & regv)
		;
#endif

	// mtcmos off2
	regv = riu_read16(x32_aia_top, 0x05);
	regv &= ~BIT(1);
		// clear [1] reg_apu_mdla_pwr_on_2nd = 0
	riu_write16(x32_aia_top, 0x05, regv);

#ifndef FPGA
	// mtcmos off2 ack
	// polling [3] == 0 reg_apu_mdla_pwr_on_ack_2nd
	regv = BIT(3);
	while (riu_read16(x32_aia_top, 0x05) & regv)
		;
#endif

	// = DVDD APUSYS_VCORE/CONN off =
	//sram pd
	regv = riu_read16(x32_aia_top, 0x07);
	regv |= BIT(3);
		// set [3] reg_apu_tcm_sram_pd
	riu_write16(x32_aia_top, 0x07, regv);

	// sram pd ack
	// polling [4] == 0 reg_apu_tcm_sram_pd_ack
	regv = BIT(4);
	while ((riu_read16(x32_aia_top, 0x07) & regv) != regv)
		;

	// clk off1
	regv = riu_read16(x32_aia_top, 0x01);
	regv |= BIT(0) | BIT(1) | BIT(3);
		// set [0] reg_apu_vcore_clk_dis
		// set [1] reg_apu_conn_clk_dis
		// set [3] reg_apu_tcm_clk_dis
	riu_write16(x32_aia_top, 0x01, regv);

	// clk off2
	regv = riu_read16(x32_aia_top, 0x00);
	regv &= ~(BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(5));
		// clear [0] [1] [3] [4] [5]
		// set [5] reg_aia_tcm_en
		// set [4] reg_aia_imi_en
		// set [3] reg_aia_mcu_en
		// set [1] reg_aia_bus_die_en
		// set [0] reg_aia_en

	riu_write16(x32_aia_top, 0x00, regv);

	//reset assert
	regv = riu_read16(x32_aia_top, 0x02);
	regv &= ~(BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7));
		// clear [7] [6] [5] [4] [3]
		// set [7] reg_rstz_tcm
		// set [6] reg_rstz_xtal_24m
		// set [5] reg_rstz_mcu
		// set [4] reg_rstz_imi
		// set [3] reg_rstz_bus
	riu_write16(x32_aia_top, 0x02, regv);

	arpc_info(ad->dev, "%s(): success\n", __func__);
out:
	mutex_unlock(&hw_lock);
}
#else
int dtv_apu_mt5879_mtcmos_on(struct apu_dev *ad, int force)
{
	int ret = 0;

	if (!mtcmos_init_done) {
		if (mt5879_mtcmos_init(ad)) {
			ret = -EFAULT;
			aprobe_err(ad->dev, "%s(): init fail\n",
					__func__);
			goto out;
		}
	}

	mutex_lock(&hw_lock);
	if (!force && power_on_cnt++) {
		arpc_info(ad->dev, "%s(): is on, count:%lu\n",
				__func__, power_on_cnt);
		ret = false;
		mutex_unlock(&hw_lock);
		goto out;
	}

	ret = plat_teec_send_command(ad, E_MDLA_PWR_CMD_SET_BOOT_UP, 0, NULL);
	if (ret) {
		arpc_err(ad->dev, "%s(): on fail, ret = %x\n", __func__, ret);
		power_on_cnt--;
		mutex_unlock(&hw_lock);
		goto out;
	}

	arpc_info(ad->dev, "%s(): success\n", __func__);
	mutex_unlock(&hw_lock);
out:
	return ret;
}

void dtv_apu_mt5879_mtcmos_off(struct apu_dev *ad, int force)
{
	if (!mtcmos_init_done) {
		arpc_err(ad->dev, "%s(): not init\n", __func__);
		return;
	}

	mutex_lock(&hw_lock);
	if (!power_on_cnt) {
		arpc_info(ad->dev, "%s(): is disabled\n", __func__);
		goto out;
	}

	if (!force && power_on_cnt-- > 1) {
		arpc_info(ad->dev, "%s(): not off, count:%lu\n",
				__func__, power_on_cnt);
		goto out;
	}

	if (plat_teec_send_command(ad, E_MDLA_PWR_CMD_SET_SHUT_DOWN, 0, NULL)) {
		arpc_err(ad->dev, "%s(): off fail\n", __func__);
		power_on_cnt++;
		goto out;
	}

	arpc_info(ad->dev, "%s(): success\n", __func__);
out:
	mutex_unlock(&hw_lock);
}
#endif

void mt5879_mtcmos_suspend(struct apu_dev *ad)
{
	mutex_lock(&hw_lock);
	power_on_cnt = 0;
	plat_teec_send_command(ad, E_MDLA_PWR_CMD_UNINIT_POWER, 0, NULL);
	mutex_unlock(&hw_lock);
}

void mt5879_mtcmos_resume(struct apu_dev *ad)
{
	plat_teec_send_command(ad, E_MDLA_PWR_CMD_INIT_POWER, 0, NULL);
}

unsigned long dtv_apu_mt5879_power_status(void)
{
	unsigned long count;

	mutex_lock(&hw_lock);
	count = power_on_cnt;
	mutex_unlock(&hw_lock);

	return count;
}
