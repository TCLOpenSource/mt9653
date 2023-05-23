// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/dma-direct.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <utilities/mdla_util.h>
#include <utilities/mdla_debug.h>
#include <platform/v2_0/mdla_hw_reg_v2_0.h>
#include "mdla_mt589x_reg_v2_0.h"

static inline u32 mdla_plat_reg_read32(u32 bank, u32 offset)
{
	void *addr;
	u32 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread32(addr);
	iounmap(addr);

	return regv;
}

static void mdla_plat_reg_write32(u32 bank, u32 offset, u32 value)
{
	void *addr;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	iowrite32(value, addr);
	iounmap(addr);
}

static u16 mdla_plat_reg_read16(u32 bank, u32 offset)
{
	void *addr;
	u16 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread16(addr);
	iounmap(addr);

	return regv;
}

static inline void mdla_plat_reg_write16(u32 bank, u32 offset, u16 value)
{
	void *addr;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	iowrite16(value, addr);
	iounmap(addr);
}

static inline u8 mdla_plat_reg_read8(u32 bank, u32 offset)
{
	void *addr;
	u8 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread8(addr);
	iounmap(addr);

	return regv;
}

static inline void mdla_plat_reg_write8(u32 bank, u32 offset, u8 value)
{
	void *addr;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	iowrite8(value, addr);
	iounmap(addr);
}

static int aia_top_mcusys_xiu_bridge_init(void)
{
	mdla_plat_reg_write16(MCUSYS_XIU_BRIDGE, 0x08, 0x0000);

	return 0;
}

static int aia_top_apusys_init(void)
{
	void *addr;

	addr = ioremap_nocache((phys_addr_t)APUSYS_VCORE_CG_CLR, PAGE_SIZE);
	iowrite32(0xffffffff, addr);
	iounmap(addr);

	addr = ioremap_nocache((phys_addr_t)APU_CONN_CG_CLR, PAGE_SIZE);
	iowrite32(0xffffffff, addr);
	iounmap(addr);

	addr = ioremap_nocache((phys_addr_t)APU_MDLA0_APU_MDLA_CG_CLR, PAGE_SIZE);
	iowrite32(0xffffffff, addr);
	iounmap(addr);

	return 0;
}

static int aia_top_iommu_init(void)
{
	mdla_plat_reg_write16(IOMMU_LOCAL_SSC3, 0x7b, 0x0000);
	mdla_plat_reg_write16(IOMMU_LOCAL_SSC4, 0x7b, 0x0000);

	return 0;
}

static void aia_apu_pll_init(void)
{
	mdla_plat_reg_write16(CKGEN00, 0x3d, 0x0003);
	mdla_plat_reg_write16(0x100C, 0x07, 0x0404);
	mdla_plat_reg_write16(0x100C, 0x30, 0xd89c);
	mdla_plat_reg_write16(0x100C, 0x31, 0x0031); // 957MHz

	mdla_plat_reg_write16(0x100C, 0x32, 0x0001);
	mdla_plat_reg_write16(0x100C, 0x01, 0x6280);
	mdla_plat_reg_write16(0x100C, 0x02, 0x0100);
	mdla_plat_reg_write16(0x100C, 0x03, 0x0118);
	mdla_plat_reg_write16(0x100C, 0x04, 0x0030);
}

static void aia_bus_pll_init(void)
{
	mdla_plat_reg_write16(CKGEN00, 0x7d4, 0x0000);
	mdla_plat_reg_write16(CKGEN00, 0x7d6, 0x0400);
	mdla_plat_reg_write16(CKGEN00, 0x7d7, 0x0000);
	mdla_plat_reg_write16(CKGEN00, 0x7d8, 0x0034);
	mdla_plat_reg_write16(CKGEN00, 0x7d2, 0x0000);
	mdla_plat_reg_write16(CKGEN00, 0x7d1, 0x0001);
	mdla_plat_reg_write16(CKGEN00, 0x7d0, 0x1110);
	mdelay(1);
	mdla_plat_reg_write16(CKGEN00, 0x7d1, 0x0000);
	mdla_plat_reg_write16(CKGEN00, 0x7d0, 0x0000);
}

static int aia_top_ckgen_init(void)
{
	aia_apu_pll_init();
	aia_bus_pll_init();

	mdla_plat_reg_write32(CKGEN00, 0x7d8, 0x003c);
	mdla_plat_reg_write32(CKGEN00, 0x3c, 0x001c);
	mdla_plat_reg_write32(CKGEN00, 0x40, 0x001c);
	mdla_plat_reg_write32(CKGEN00, 0x44, 0x0000);
	mdla_plat_reg_write32(CKGEN00, 0x52b, 0x0003);

	return 0;
}

static int sram_csr_use_default_delsel(void)
{
	u32 regv;
	//printk("csr_use_default_delsel addr = %x/n",RIU_ADDR(0x2808, (0x2d0>>2)));
	//asr_use_default_delsel setting
	regv = mdla_plat_reg_read32(0x2808, (0x2d0>>2));
	//printk("before asr_use_default_delsel regv = %x\n",regv );
	regv |= (0x1 << 0);
	mdla_plat_reg_write32(0x2808, (0x2d0>>2), regv);

	return 0;
}

static int aia_top_init(int slpprot_rdy)
{
	u16 regv;
	u16 mask;
	u32 bit_ofs, reg_idx;

	// aia enable
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x00);
	regv |= (0x1 << 4) | (0x1 << 3);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x00, regv);

	// VCORE enable
	// slpprot dis
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x11);
	regv &= ~((0x1 << 9) | (0x1 << 6) | (0x1 << 3) | (0x1 << 0));
	mdla_plat_reg_write16(X32_AIA_TOP, 0x11, regv);

	// slpprot rdy
	if (slpprot_rdy) {
		mask = (0x1 << 10) | (0x1 << 7) | (0x1 << 4) | (0x1 << 1);
		while (mdla_plat_reg_read16(X32_AIA_TOP, 0x11) & mask)
			;
	}

	// reset release
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x03);
	// do not set bit 0 which will reset iommu
	regv |= (0x1 << 7) | (0x1 << 6) | (0x1 << 5);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x03, regv);

	// vcore sw_rst [2] = 0
	regv = mdla_plat_reg_read32(0x2828, 0x03);
	regv &= ~(0x1 << 2);
	mdla_plat_reg_write32(0x2828, 0x03, regv);

	//clk on 1
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x01);
	regv &= ~(0x1 << 0);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x01, regv);

	// GSM/Conn power on
	// sram pd dis
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x06);
	regv &= ~(0x1 << 0);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x06, regv);

	// sram pd dis ack
	mask = (0x1 << 1);
	while (mdla_plat_reg_read16(X32_AIA_TOP, 0x6) & mask)
		;

	// mem iso disable
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x04);
	regv &= ~(0x1 << 2);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x04, regv);

	// dig iso disable
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x04);
	regv &= ~((0x1 << 1) | (0x1 << 0));
	mdla_plat_reg_write16(X32_AIA_TOP, 0x04, regv);

	// reset release
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x03);
	regv |= (0x1 << 1);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x03, regv);

	//clk on 1
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x01);
	regv &= ~(0x1 << 1);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x01, regv);

	//clk on 2
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x00);
	regv |= (0x1 << 1);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x00, regv);

	// mtcmos on1
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x05);
	regv |= (0x1 << 0);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x05, regv);

	// mtcmos on1 ack
	mask = (0x1 << 2);
	while (!(mdla_plat_reg_read16(X32_AIA_TOP, 0x5) & mask))
		;

	// mtcmos on2
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x05);
	regv |= (0x1 << 1);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x05, regv);

	// mtcmos on2 ack
	mask = (0x1 << 3);
	while (!(mdla_plat_reg_read16(X32_AIA_TOP, 0x5) & mask))
		;

	// sram pd dis
	for (reg_idx = 0x8; reg_idx <= 0xf; reg_idx++) {
		for (bit_ofs = 0; bit_ofs <= 0xf; bit_ofs++) {
			regv = mdla_plat_reg_read16(X32_AIA_TOP, reg_idx);
			regv &= ~(0x1 << bit_ofs);
			mdla_plat_reg_write16(X32_AIA_TOP, reg_idx, regv);
		}
	}

	// mem isolation dis
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x04);
	regv &= ~(0x1 << 4);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x04, regv);

	// dig isolation dis
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x04);
	regv &= ~(0x1 << 3);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x04, regv);

	// slpprot dis
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x10);
	regv &= ~((0x1 << 6) | (0x1 << 3) | (0x1 << 0));
	mdla_plat_reg_write16(X32_AIA_TOP, 0x10, regv);

	// slpprot rdy
	mask = (0x1 << 7) | (0x1 << 4) | (0x1 << 1);
	while (mdla_plat_reg_read16(X32_AIA_TOP, 0x10) & mask)
		;

	// reset release
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x03);
	regv |= (0x1 << 4) | (0x1 << 3) | (0x1 << 2);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x03, regv);

	//clk on 1
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x01);
	regv &= ~(0x1 << 2);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x01, regv);

	//clk on 2
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x00);
	regv |= (0x1 << 2);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x00, regv);

	aia_top_iommu_init();

	//bypass security check
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x26);
	regv |= (0x1 << 2);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x26, regv);

	return 0;
}

static int aia_power_off(int vcore_sw_rst)
{
	u16 regv, mask;
	u32 bit_ofs, reg_idx;

	// slpprot en
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x10);
	regv |= ((0x1 << 6) | (0x1 << 3) | (0x1 << 0));
	mdla_plat_reg_write16(X32_AIA_TOP, 0x10, regv);

	// slpprot rdy
	mask = (0x1 << 7) | (0x1 << 4) | (0x1 << 1);
	while ((mdla_plat_reg_read16(X32_AIA_TOP, 0x10) & mask) != mask)
		;

	//clk off 1
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x01);
	regv |= (0x1 << 2);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x01, regv);

	//clk off 2
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x00);
	regv &= ~(0x1 << 2);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x00, regv);

	//reset assert
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x03);
	regv &= ~((0x1 << 4) | (0x1 << 3) | (0x1 << 2));
	mdla_plat_reg_write16(X32_AIA_TOP, 0x03, regv);

	//mem isolation en
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x04);
	regv |= (0x1 << 4);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x04, regv);

	//dig isolation en
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x04);
	regv |= (0x1 << 3);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x04, regv);

	// sram pd
	for (reg_idx = 0x8; reg_idx <= 0xf; reg_idx++) {
		for (bit_ofs = 0; bit_ofs <= 0xf; bit_ofs++) {
			regv = mdla_plat_reg_read16(X32_AIA_TOP, reg_idx);
			regv |= (0x1 << bit_ofs);
			mdla_plat_reg_write16(X32_AIA_TOP, reg_idx, regv);
		}
	}

	// mtcmos off1
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x05);
	regv &= ~(0x1 << 0);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x05, regv);

	// mtcmos off1 ack
	mask = (0x1 << 2);
	while (mdla_plat_reg_read16(X32_AIA_TOP, 0x05) & mask)
		;

	// mtcmos off2
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x05);
	regv &= ~(0x1 << 1);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x05, regv);

	// mtcmos off2 ack
	mask = (0x1 << 3);
	while (mdla_plat_reg_read16(X32_AIA_TOP, 0x05) & mask)
		;

	//clk off 1
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x01);
	regv |= (0x1 << 1);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x01, regv);

	//clk off 2
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x00);
	regv &= ~(0x1 << 1);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x00, regv);

	// slpprot en
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x11);
	regv |= (0x1 << 9) | (0x1 << 6) | (0x1 << 3) | (0x1 << 0);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x11, regv);

	// slpprot rdy
	mask = (0x1 << 10) | (0x1 << 7) | (0x1 << 4) | (0x1 << 1);
	while ((mdla_plat_reg_read16(X32_AIA_TOP, 0x11) & mask) != mask)
		;

	// reset assert
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x03);
	regv &= ~(0x1 << 1);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x03, regv);

	//mem isolation en
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x04);
	regv |= (0x1 << 2);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x04, regv);

	//dig isolation en
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x04);
	regv |= ((0x1 << 1) | (0x1<<0));
	mdla_plat_reg_write16(X32_AIA_TOP, 0x04, regv);

	//sram pd en
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x06);
	regv |= (0x1 << 0);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x06, regv);

	//sram pd en ack
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x06);
	mask = (0x1 << 1);
	//while (!(mdla_plat_reg_read16(X32_AIA_TOP, 0x06) & mask))
	//	;

	//clk off 1
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x01);
	regv |= (0x1 << 0);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x01, regv);

	// vcore sw_rst [2] = 1
	if (vcore_sw_rst) {
		regv = mdla_plat_reg_read32(0x2828, 0x03);
		regv |= (0x1 << 2);
		mdla_plat_reg_write32(0x2828, 0x03, regv);
	}

	// reset assert: skip imi rst
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x03);
	regv &= ~((0x1 << 7) | (0x1 << 5));
	mdla_plat_reg_write16(X32_AIA_TOP, 0x03, regv);

	// aia disable
	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x00);
	regv &= ~((0x1 << 4) | (0x1 << 3));
	mdla_plat_reg_write16(X32_AIA_TOP, 0x00, regv);


	return 0;
}

//---------------------------------------------------------------------------------------
//------------------------------------  COMMON SOURCE CODE HERE  ------------------------
//---------------------------------------------------------------------------------------

int mdla_mt5896_v2_0_hw_reset(u32 reset)
{
	void *addr;
	u32 val;
	static int init_once;

	mdla_drv_debug("%s: hw_reset start(%x)\n", __func__, reset);
	if (reset == NORMAL_RESET) {
		if (!init_once) {
			mdla_drv_debug("%s: first time reset\n", __func__);
			aia_top_ckgen_init();
			aia_top_init(0);
			aia_top_apusys_init();
			aia_top_mcusys_xiu_bridge_init();
			sram_csr_use_default_delsel();
			init_once = 1;
		}
	} else if (reset == COLD_RESET) {
		aia_top_ckgen_init();
		aia_top_init(0);
		aia_top_apusys_init();
		aia_top_mcusys_xiu_bridge_init();
		sram_csr_use_default_delsel();
		init_once = 1;
	} else if (reset == FORCE_RESET) {
		aia_top_ckgen_init();
		aia_power_off(1);
		aia_top_init(0);
		aia_top_apusys_init();
		aia_top_mcusys_xiu_bridge_init();
		sram_csr_use_default_delsel();
		init_once = 1;
	}

	addr = ioremap_nocache((phys_addr_t)APU_SCTRL_PGRMP_SCTRL_MDLA_CORE0_CTXT, PAGE_SIZE);
	val = (ioread32(addr) & ~0x3) | 0x2;
	iowrite32(val, addr);
	iounmap(addr);

	mdla_drv_debug("%s: hw_reset done.\n", __func__);

	return 0;
}



// enable to skip access permission check (always permit)
void apmcu_miu2tcm_secure_check_overwrite(int enable)
{
	if (enable)
		mdla_plat_reg_write16(X32_AIA_TOP, 0x26, 0x0005);
	else
		mdla_plat_reg_write16(X32_AIA_TOP, 0x26, 0x0001);
	// [2]   reg_secure_check_overwrite = 1, other bits: keep default
	// [2]   reg_secure_check_overwrite = 0, other bits: keep default
}

void aia_top_security_rst(int rst_policy)
{
	u32 regv;

	regv = mdla_plat_reg_read16(X32_AIA_TOP, 0x26);
	regv &= ~0x3;		// clear [1:0]
	regv |= rst_policy;	// [1:0] reg_security_swrstz
	mdla_plat_reg_write16(X32_AIA_TOP, 0x26, regv);
	//h31	h30	19	0	reg_mem_on_counter	19	0	20	hffff0
	//h33	h32	19	0	reg_mem_off_counter	19	0	20	hffff0
	mdla_plat_reg_write16(X32_AIA_TOP, 0x30, 0xfc00);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x31, 0xf);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x32, 0xfc00);
	mdla_plat_reg_write16(X32_AIA_TOP, 0x33, 0xf);

	mdla_plat_reg_write16(X32_AIA_TOP, 0x20, 0x0001);	// [0] reg_security_swrstz
	while (!mdla_plat_reg_read16(X32_AIA_TOP, 0x2a))
		;

	mdla_plat_reg_write16(X32_AIA_TOP, 0x20, 0x0000);	// [0] reg_security_swrstz

}

void apmcu_miu2tcm_access_permission(int enable)
{
	mdla_drv_debug("%s: enable:%x\n",  __func__, enable);
	if (enable) {
		// assume ARM's AID is 0x00
		// h24 [8:0] set secure AID
		mdla_plat_reg_write16(X32_AIA_TOP, 0x24, 62);
		// h25 [8:0] set non-secure AID
		mdla_plat_reg_write16(X32_AIA_TOP, 0x25, 1);
		// h21 [0] reg_sec_mode_select
		mdla_plat_reg_write16(X32_AIA_TOP, 0x21, 0x0001);
		// h22 [1:0] enable reg_a_secure_aid_secure_permission
		//([0]readable [1]writable)
		// h22 [3:2] enable reg_a_secure_aid_nonsecure_permission
		//([2]readable [3]writable)
		// h22 [5:4]        reg_a_nonsecure_aid_secure_permission
		//([4]readable [5]writable)
		// h22 [7:6]        reg_a_nonsecure_aid_nonsecure_permission
		//([6]readable [7]writable)
		mdla_plat_reg_write16(X32_AIA_TOP, 0x22, 0x000f);
	} else {
		// assume ARM's AID is 0x00
		// h24 [8:0] set secure AID
		mdla_plat_reg_write16(X32_AIA_TOP, 0x24, 1);
		// h25 [8:0] set non-secure AID
		mdla_plat_reg_write16(X32_AIA_TOP, 0x25, 62);
		// clear reg_sec_mode_select
		//mdla_plat_reg_write16(APMCU_RIUBASE+X32_AIA_TOP+((0x21)<<2), 0x0000);
		// NOTE: reg_sec_mode_select is one-way register.
		//must set reg_security_swrstz to leave secure_mode

		// disable read/write permission
		// h22 [1:0] enable reg_a_secure_aid_secure_permission
		//([0]readable [1]writable)
		// h22 [3:2] enable reg_a_secure_aid_nonsecure_permission
		//([2]readable [3]writable)
		// h22 [5:4]        reg_a_nonsecure_aid_secure_permission
		//([4]readable [5]writable)
		// h22 [7:6]        reg_a_nonsecure_aid_nonsecure_permission
		//([6]readable [7]writable)
		mdla_plat_reg_write16(X32_AIA_TOP, 0x22, 0x000f);
	}

	mdla_drv_debug("%s: [21]:%x [22]:%x [24]:%x [25]:%x\n",  __func__,
			mdla_plat_reg_read16(X32_AIA_TOP, 0x21),
			mdla_plat_reg_read16(X32_AIA_TOP, 0x22),
			mdla_plat_reg_read16(X32_AIA_TOP, 0x24),
			mdla_plat_reg_read16(X32_AIA_TOP, 0x25));
}
