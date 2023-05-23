// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


#include "mtk-mmc-fcie.h"
#include "mtk-fcie-cqe.h"
#include <linux/mmc/mmc.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <asm-generic/div64.h>
#include <linux/time.h>

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/
static void mtk_fcie_start_cmd(struct mtk_fcie_host *mtk_fcie_host,
		struct mmc_command *cmd);
static int mtk_fcie_pre_dma_transfer(struct mtk_fcie_host *mtk_fcie_host,
		struct mmc_data *data, struct mtk_fcie_host_next *next);
static u32 mtk_fcie_wait_d0_high(struct mtk_fcie_host *mtk_fcie_host,
		u32 us);
static void mtk_fcie_err_handler(struct mtk_fcie_host *mtk_fcie_host);
static u32 mtk_fcie_cmd0(struct mtk_fcie_host *mtk_fcie_host, u32 arg);
static void mtk_fcie_prepare_psm_queue(struct mtk_fcie_host *mtk_fcie_host);
static void emmc_record_wr_time(struct mtk_fcie_host *mtk_fcie_host,
		u8 CmdIdx, u32 DataByteCnt);
static void emmc_check_life(struct mtk_fcie_host *mtk_fcie_host,
		u32 DataByteCnt);
static void mtkcqe_fde_open(struct mtk_fcie_host *host);
static void mtkcqe_fde_close(struct mtk_fcie_host *host);
static int mtkcqe_fde_func_en(struct mtk_fcie_host *host, u32 lba);
static int mtkcqe_enable(struct mmc_host *mmc_host, struct mmc_card *card);
static int mtkcqe_host_alloc_tdl(struct mtk_cqe_host *mtk_cqe_host);
static void mtkcqe_setup_link_desc(struct mtk_cqe_host *mtk_cqe_host, int tag);
static void __mtkcqe_enable(struct mtk_cqe_host *mtk_cqe_host);
static void __mtkcqe_disable(struct mmc_host *mmc_host);
static int mtkcqe_suspend(struct mmc_host *mmc_host);
static void mtkcqe_off(struct mmc_host *mmc_host);
static void mtkcqe_prep_task_desc(struct mmc_request *mrq,
		struct mtk_cqe_host *mtk_cqe_host, bool intr, int tag);
static int mtkcqe_prep_trans_desc(struct mmc_request *mrq,
		struct mtk_cqe_host *mtk_cqe_host, int tag);
static int mtkcqe_dma_map(struct mmc_host *mmc_host, struct mmc_request *mrq);
static void mtkcqe_prep_link_desc(struct mmc_request *mrq,
		struct mtk_cqe_host *mtk_cqe_host, int tag);
static void mtkcqe_prep_dcmd_desc(struct mmc_request *mrq, struct mtk_cqe_host *mtk_cqe_host);
static void mtkcqe_post_req(struct mmc_host *mmc_host, struct mmc_request *mrq);
static int mtkcqe_tag(struct mmc_request *mrq);
static int mtkcqe_request(struct mmc_host *mmc_host, struct mmc_request *mrq);
static void mtkcqe_recovery_needed(struct mmc_host *mmc_host, struct mmc_request *mrq, bool notify);
static int mtkcqe_error_flags(u16 err_status);
static void mtkcqe_finish_mrq(struct mmc_host *mmc_host, int tag);
static void mtkcqe_error_irq(struct mtk_fcie_host *mtk_fcie_host, u16 cqe_status, u16 events);
static bool mtkcqe_is_idle(struct mtk_cqe_host *mtk_cqe_host, int *ret);
static int mtkcqe_wait_for_idle(struct mmc_host *mmc_host);
static bool mtkcqe_timeout(struct mmc_host *mmc_host,
		struct mmc_request *mrq, bool *recovery_needed);
static void mtkcqe_recovery_start(struct mmc_host *mmc_host);
static int mtkcqe_error_from_flags(int flags);
static void mtkcqe_recover_mrq(struct mtk_cqe_host *mtk_cqe_host, int tag);
static void mtkcqe_recover_mrqs(struct mtk_cqe_host *mtk_cqe_host);
static void mtkcqe_recovery_finish(struct mmc_host *mmc_host);
static void mtkcqe_disable(struct mmc_host *mmc_host);
static int mtkcqe_init(struct mtk_cqe_host *mtk_cqe_host, struct mmc_host *mmc_host);
static void cqe_record_wr_data_size(struct mtk_fcie_host *mtk_fcie_host, u8 cmd_idx, u32 bytes);
static void cqe_record_wr_time(struct mtk_fcie_host *mtk_fcie_host);
static void mtkcqe_irq(struct mtk_fcie_host *mtk_fcie_host, u16 events, u16 cqe_status);
static void mtkcqe_wait_cqebus_idle(struct mtk_fcie_host *mtk_fcie_host);
static void mtkcqe_irq_err_handle(struct mtk_fcie_host *mtk_fcie_host);
static void mtkcqe_chk_tcc_tcn(struct mtk_fcie_host *mtk_fcie_host,
	u16 task_comp_notify_15_0,
	u16 task_comp_notify_31_16,
	u32 task_comp_notify,
	u16 cqis);
static void mtkcqe_prepare_chk_cqe_bus(struct mtk_fcie_host *mtk_fcie_host);

// ===============================
#if defined(CONFIG_OF)
static const struct mtk_fcie_compatible mtk_mt5896_fcie_compat = {
	.name = "mediatek,mt5896-mmc-fcie",
};

static const struct of_device_id mtk_fcie_emmc_dt_match[] = {
	{ .compatible = "mediatek,mt5896-mmc-fcie",
		.data = &mtk_mt5896_fcie_compat},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_fcie_emmc_dt_match);
#endif

/******************************************************************************
 * Platform Setting
 ******************************************************************************/

/* delay function
 */
void mtk_fcie_hw_timer_delay(u32 us)
{
	u32 ms = us/1000;
	//handle ms
	if (ms > 20) {
		msleep(ms);
	} else if (ms > 0) {
		usleep_range(us, (ms + 1)*1000);
	} else { //hande us
		if (us <= 10)
			udelay(us);
		else
			usleep_range(us, us + 10);
	}
}

//=======================================================================
static void mtk_fde_reset(struct mtk_fcie_host *host)
{
	REG_SETBIT(REG_ADDR(host->fdebase, AES_SW_RESET), BIT(0));
	REG_CLRBIT(REG_ADDR(host->fdebase, AES_SW_RESET), BIT(0));
}

static void mtk_fde_set_ctr(struct mtk_fcie_host *host, u8 *ctr)
{
	int i;

	for (i = 0; i < 8; i++)
		REG_W(REG_ADDR(host->fdebase, AES_CTR + i),
			(ctr[i*2 + 1] << 8) | ctr[i*2]);
}

static void mtk_fde_set_xts_lba(struct mtk_fcie_host *host, u32 lba)
{
	u8	ctr[16];
	u16 mode;
	int i;

	mode = REG(REG_ADDR(host->fdebase, AES_CONTROL0));
	mode &= BIT_MASK_AES_MODE;
	mode >>= BIT_AES_MODE_SHIFT;
	if (mode != XTS_MODE)
		return;
	for (i = 0; i < 16; i++) {
		if (i < sizeof(u32)) {
			ctr[i] = lba & 0xFF;
			lba >>= 8;
		} else
			ctr[i] = 0;
	}
	mtk_fde_set_ctr(host, ctr);
}
static int mtk_fde_set_dir(struct mtk_fcie_host *host, u8 dir)
{
	if (dir == ENABLE_ENC)
		REG_CLRBIT(REG_ADDR(host->fdebase, AES_CONTROL0),
			BIT_ENC_OR_DEC);
	else if (dir == ENABLE_DEC)
		REG_SETBIT(REG_ADDR(host->fdebase, AES_CONTROL0),
			BIT_ENC_OR_DEC);
	else
		return -1;
	return 0;
}

static void mtk_fde_set_xex_kick(struct mtk_fcie_host *host, int val)
{
	if (val == 1) {
		while (REG(REG_ADDR(host->fdebase, AES_BUSY)) & BIT_AES_BUSY)
			cpu_relax();
		REG_SETBIT(REG_ADDR(host->fdebase,
			AES_XEX_HW_T_CALC_KICK), BIT(0));
	} else if (val == 0)
		REG_CLRBIT(REG_ADDR(host->fdebase,
			AES_XEX_HW_T_CALC_KICK), BIT(0));
}
static void mtk_fde_open(struct mtk_fcie_host *host)
{
	REG_SETBIT(REG_ADDR(host->fdebase, AES_FUN), BIT_AES_FUNC_EN);
}
static void mtk_fde_close(struct mtk_fcie_host *host)
{
	REG_CLRBIT(REG_ADDR(host->fdebase, AES_FUN), BIT_AES_FUNC_EN);
}

static int mtk_fde_check_white_list(struct mtk_fcie_host *host, u32 lba)
{
	struct emmc_crypto_disable_info *info;
	struct list_head *head = &host->crypto_white_list;

	if (!list_empty(head)) {
		list_for_each_entry(info, head, list) {
			if ((lba >= info->lba) &&
				(lba < (info->lba+info->length))) {
				EMMC_DBG(host,
				EMMC_DBG_LVL_LOW, 1,
				"lba 0x%X in white 0x%llX, Len 0x%llX\n",
				lba, info->lba, info->length);
				return 1;
			}
		}
	}
	return 0;
}

static void mtk_fde_func_en(struct mtk_fcie_host *host, u8 dir, u32 lba)
{
	if (host->fdebase == NULL || host->enable_fde == 0)
		return;
	//emmc driver reserved area
	if (lba >= EMMC_CIS_BLK_0 &&
		lba < EMMC_CIS_BLK_0 + EMMC_DRV_RESERVED_BLK_CNT)
		return;
	//check white list
	if (mtk_fde_check_white_list(host, lba))
		return;

	mtk_fde_reset(host);
	mtk_fde_set_xts_lba(host, lba);
	mtk_fde_set_dir(host, dir);
	mtk_fde_set_xex_kick(host, 1);
	mtk_fde_open(host);
	host->fde_runtime_en = 1;
}

void mtk_fde_func_dis(struct mtk_fcie_host *host)
{
	if (host->fdebase == NULL || host->fde_runtime_en == 0 ||
		host->enable_fde == 0)
		return;
	mtk_fde_set_xex_kick(host, 0);
	mtk_fde_close(host);
	host->fde_runtime_en = 0;
}
/*for debug
 */
void mtk_fcie_reg_dump(struct mtk_fcie_host *host)
{
	u16 reg[8];
	int i, j;
	struct mtk_cqe_host *mtk_cqe_host = host->mmc->cqe_private;

	EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1, "fciebase\n");

	for (i = 0; i < BANK_SIZE; i += DUMP_REG_OFFSET) {
		for (j = 0; j < DUMP_REG_OFFSET; j++)
			reg[j] = REG(REG_ADDR(host->fciebase, i + j));

		EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1,
			"\n%02Xh:| %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh\n",
			i, reg[DUMP_REG0], reg[DUMP_REG1], reg[DUMP_REG2], reg[DUMP_REG3],
			reg[DUMP_REG4], reg[DUMP_REG5], reg[DUMP_REG6], reg[DUMP_REG7]);

	}
	EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1, "\n");

	EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1, "emmcpllbase\n");

	for (i = 0; i < BANK_SIZE; i += DUMP_REG_OFFSET) {
		for (j = 0; j < DUMP_REG_OFFSET; j++)
			reg[j] = REG(REG_ADDR(host->emmcpllbase, i + j));

		EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1,
			"\n%02Xh:| %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh\n",
			i, reg[DUMP_REG0], reg[DUMP_REG1], reg[DUMP_REG2], reg[DUMP_REG3],
			reg[DUMP_REG4], reg[DUMP_REG5], reg[DUMP_REG6], reg[DUMP_REG7]);

	}
	if (host->fcieclkreg) {
		EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1,
			"fcie_top_clk=%04Xh, fcie_sync_clk=%04Xh\n",
			REG(REG_ADDR(host->fcieclkreg, 0)),
			REG(REG_ADDR(host->fcieclkreg, 2)));
	}

	if (host->support_cqe) {
		EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1, "cqebase\n");

		for (i = 0; i < BANK_SIZE; i += DUMP_REG_OFFSET) {
			for (j = 0; j < DUMP_REG_OFFSET; j++)
				reg[j] = REG(REG_ADDR(mtk_cqe_host->cqebase, i + j));

			EMMC_DBG(host,
				EMMC_DBG_LVL_ERROR, 1,
				"\n%02Xh:| %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh\n",
				i, reg[DUMP_REG0], reg[DUMP_REG1], reg[DUMP_REG2], reg[DUMP_REG3],
				reg[DUMP_REG4], reg[DUMP_REG5], reg[DUMP_REG6], reg[DUMP_REG7]);
		}
		EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1, "\n");

		EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1, "taskdesc\n");

		for (i = 0; i < BANK_SIZE; i += DUMP_REG_OFFSET) {
			for (j = 0; j < DUMP_REG_OFFSET; j++)
				reg[j] = REG(REG_ADDR(mtk_cqe_host->taskdesc, i + j));

			EMMC_DBG(host,
				EMMC_DBG_LVL_ERROR, 1,
				"\n%02Xh:| %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh %04Xh\n",
				i, reg[DUMP_REG0],  reg[DUMP_REG1], reg[DUMP_REG2], reg[DUMP_REG3],
				reg[DUMP_REG4], reg[DUMP_REG5], reg[DUMP_REG6], reg[DUMP_REG7]);
		}
	}

	EMMC_DBG(host,
			EMMC_DBG_LVL_ERROR, 1, "\n");
}
void mtk_fcie_debug_bus_dump(struct mtk_fcie_host *host)
{
	int i, j;
	static const char * const mode[] = {
		NULL,
		NULL,
		NULL,
		"MMA",
		NULL,
		"CARD",
		NULL,
		NULL,
		"REG_QIFX_CNT",
		"REG_QIFY_CNT",
		"REG_DMA_REQ_CNT",
		"REG_DMA_RSP_CNT",
		NULL,
		NULL,
		NULL,
		NULL,
	};
	for (j = 0; j < 0x10; j++) {
		if (mode[j] == NULL)
			continue;
		EMMC_DBG(host,
				EMMC_DBG_LVL_ERROR, 1, "mode=%s\n",
				mode[j]
				);
		REG_CLRBIT(REG_ADDR(host->fciebase, EMMC_DEBUG_BUS1),
			BIT_DEBUG1_MODE_MSK);
		REG_SETBIT(REG_ADDR(host->fciebase, EMMC_DEBUG_BUS1),
			j << 8);
		if (j == 5) {
			for (i = 1; i <= 4 ; i++) {
				REG_CLRBIT(REG_ADDR(host->fciebase, TEST_MODE),
					BIT_TEST_MODE_MASK);
				REG_SETBIT(REG_ADDR(host->fciebase, TEST_MODE),
					i << BIT_TEST_MODE_SHIFT);
				EMMC_DBG(host,
					EMMC_DBG_LVL_ERROR, 1,
					"0x39=%04Xh, 0x15=%04Xh, 0x38=%04Xh\n",
					REG(REG_ADDR(host->fciebase, EMMC_DEBUG_BUS1)),
					REG(REG_ADDR(host->fciebase, TEST_MODE)),
					REG(REG_ADDR(host->fciebase, EMMC_DEBUG_BUS0))
					);
			}
		} else if (j == 9) {
			for (i = 0; i <= 31 ; i++) {
				REG_CLRBIT(REG_ADDR(host->fciebase + 0x200, 0x51),
					BIT(12)|BIT(11)|BIT(10)|BIT(9)|BIT(8));
				REG_SETBIT(REG_ADDR(host->fciebase + 0x200, 0x51),
					i << 8);
				EMMC_DBG(host,
					EMMC_DBG_LVL_ERROR, 1,
					"0x39=%04Xh, 0x51=%04Xh, 0x38=%04Xh\n",
					REG(REG_ADDR(host->fciebase, EMMC_DEBUG_BUS1)),
					REG(REG_ADDR(host->fciebase + 0x200, 0x51)),
					REG(REG_ADDR(host->fciebase, EMMC_DEBUG_BUS0))
					);
			}
		} else {
			EMMC_DBG(host,
				EMMC_DBG_LVL_ERROR, 1,
				"0x39=%04Xh, 0x38=%04Xh\n",
				REG(REG_ADDR(host->fciebase, EMMC_DEBUG_BUS1)),
				REG(REG_ADDR(host->fciebase, EMMC_DEBUG_BUS0))
				);
		}
	}

	mtkcqe_prepare_chk_cqe_bus(host);

}

/* control mmc HW reset pin to reset eMMC
 */
void _mtk_fcie_mmc_hw_reset(struct mtk_fcie_host *mtk_fcie_host)
{
	REG_CLRBIT(
		REG_ADDR(mtk_fcie_host->fciebase, BOOT_CONFIG),
		BIT_EMMC_RSTZ);
	mtk_fcie_hw_timer_delay(TIME_RSTW);
	REG_SETBIT(
		REG_ADDR(mtk_fcie_host->fciebase, BOOT_CONFIG),
		BIT_EMMC_RSTZ);
	mtk_fcie_hw_timer_delay(TIME_RSCA);
}

/* ssc setting for HS400/HS200 to against interference
 */
void mtk_fcie_pll_ssc(struct mtk_fcie_host *mtk_fcie_host)
{
	if (mtk_fcie_host->v_emmc_pll == eMMC_PLL_VER7) {
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x14), eMMC_PLL_0x14_VER7);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x15), eMMC_PLL_0x15_VER7);
	} else if (mtk_fcie_host->v_emmc_pll == eMMC_PLL_VER12) {
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x14), eMMC_PLL_0x14_VER12);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x15), eMMC_PLL_0x15_VER12);
	} else if (mtk_fcie_host->v_emmc_pll == eMMC_PLL_VER22) {
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x14), eMMC_PLL_0x14_VER22);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x15), eMMC_PLL_0x15_VER22);
	} else {
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x14), eMMC_PLL_0x14_MSTAR);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x15), eMMC_PLL_0x15_MSTAR);
	}
}
/* setting corresponded setting of eMMC pll according to clk_param
 * @ HS200/HS400 (ES) mode
 * return 0 is success
 * othewise is fail
 */
u32 mtk_fcie_pll_setting(u16 clk_param, struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 value_reg_emmc_pll_pdiv;
	u16 emmcpll_0x03 = 0;

	if ((clk_param == emmc_drv->old_pll_clk_param) &&
		(emmc_drv->tune_overtone == 0))
		return EMMC_ST_SUCCESS;

	emmc_drv->old_pll_clk_param = clk_param;

	if (mtk_fcie_host->v_emmc_pll == 7) {
		//N7 no overtone
		emmc_drv->tune_overtone = 0;
		//reset PDIV & LPDIV
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7), 0x0010);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x8), 0x0020);

		//set emmc pll test
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7), 0x0d40);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x8), 0x0090);

		//power on reset
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), BIT(6));

		//reset emmc pll
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), BIT(5));

		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_10US);

		if (mtk_fcie_host->enable_ssc)
			mtk_fcie_pll_ssc(mtk_fcie_host);

		REG_W(
			REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0);
		REG_W(
			REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0);

		switch (clk_param) {
		case EMMC_PLL_CLK_200M: // 200M
			REG_W(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0xffff);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x1a);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_160M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x6666);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x20);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_140M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x0750);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x25);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_120M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x3333);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x2b);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		default:
			EMMC_DBG(mtk_fcie_host, 0, 0,
				"eMMC Err: emmc PLL not configed %Xh\n",
				clk_param);
			EMMC_DIE(" ");
			return EMMC_ST_ERR_UNKNOWN_CLK;
		}
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));

		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), 0x0609);

		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x5),
			BIT(2)|BIT(1)|BIT(0));
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x5),
			value_reg_emmc_pll_pdiv);


		mtk_fcie_hw_timer_delay(5*HW_TIMER_DELAY_100US);

		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_10US);
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
	} else if (mtk_fcie_host->v_emmc_pll == 12) {
		//N12 no overtone
		emmc_drv->tune_overtone = 0;
		//reset PDIV & LPDIV
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7), 0x0010);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x8), 0x0020);

		//set emmc pll test
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7), 0x0a40);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x8), 0);

		//power on reset
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), BIT(6));

		//reset emmc pll
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), BIT(5));

		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_10US);

		if (mtk_fcie_host->enable_ssc)
			mtk_fcie_pll_ssc(mtk_fcie_host);

		REG_W(
			REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0);
		REG_W(
			REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0);

		switch (clk_param) {
		case EMMC_PLL_CLK_200M: // 200M
			REG_W(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0xffff);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x1a);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_160M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x6666);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x20);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_140M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x0750);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x25);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_120M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x3333);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x2b);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		default:
			EMMC_DBG(mtk_fcie_host, 0, 0,
				"eMMC Err: emmc PLL not configed %Xh\n",
				clk_param);
			EMMC_DIE(" ");
			return EMMC_ST_ERR_UNKNOWN_CLK;
		}
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));

		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), 0x0609);

		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x5),
			BIT(2)|BIT(1)|BIT(0));
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x5),
			value_reg_emmc_pll_pdiv);


		mtk_fcie_hw_timer_delay(5*HW_TIMER_DELAY_100US);

		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_10US);
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
	} else if (mtk_fcie_host->v_emmc_pll == 22) {
		//N22 no overtone
		emmc_drv->tune_overtone = 0;
		//reset PDIV & LPDIV
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7), 0x0100);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x8), 0x0004);

		//set emmc pll test
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7), 0);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x8), 0);

		//power on reset
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), BIT(6));

		//reset emmc pll
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), BIT(5));

		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_10US);

		if (mtk_fcie_host->enable_ssc)
			mtk_fcie_pll_ssc(mtk_fcie_host);

		REG_W(
			REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0);
		REG_W(
			REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0);

		switch (clk_param) {
		case EMMC_PLL_CLK_200M: // 200M
			REG_W(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x24);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_160M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x3333);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x2b);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_140M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x5f16);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x31);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_120M:
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x999a);
			REG_SETBIT(
				REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x39);
			value_reg_emmc_pll_pdiv = 0;// PostDIV: 2
			break;

		default:
			EMMC_DBG(mtk_fcie_host, 0, 0,
				"eMMC Err: emmc PLL not configed %Xh\n",
				clk_param);
			EMMC_DIE(" ");
			return EMMC_ST_ERR_UNKNOWN_CLK;
		}
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));

		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), 0x0404);

		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x5),
			BIT(2)|BIT(1)|BIT(0));
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x5),
			value_reg_emmc_pll_pdiv);


		mtk_fcie_hw_timer_delay(5*HW_TIMER_DELAY_100US);

		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_10US);
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
	} else {
		// 1. reset emmc pll
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6), BIT(0));

		// emmcpll tune overtone
		if (emmc_drv->tune_overtone == 1) {

			EMMC_DBG(mtk_fcie_host,
				EMMC_DBG_LVL_LOW, 1, "tune overtone\n");
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), BIT(5));
			mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_100US);
			emmcpll_0x03 = REG(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x3));
			REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x3), 0xffff);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1b), BIT(0));
			REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), 0x0007);
			mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_100US);
		}

		if (mtk_fcie_host->enable_ssc)
			mtk_fcie_pll_ssc(mtk_fcie_host);

		// 2. synth clock
		REG_W(
			REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0);
		REG_W(
			REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0);
		switch (clk_param) {
		case EMMC_PLL_CLK_200M: // 200M
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x24);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x03d8);
			value_reg_emmc_pll_pdiv = 1;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_160M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x2b);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x3333);
			value_reg_emmc_pll_pdiv = 1;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_140M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x31);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x5f15);
			value_reg_emmc_pll_pdiv = 1;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_120M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x39);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x9999);
			value_reg_emmc_pll_pdiv = 1;// PostDIV: 2
			break;

		case EMMC_PLL_CLK_100M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x45);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x1eb8);
			value_reg_emmc_pll_pdiv = 1;// PostDIV: 2
			break;

		case EMMC_PLL_CLK__86M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x28);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x2fa0);
			value_reg_emmc_pll_pdiv = 2;// PostDIV: 4
			break;

		case EMMC_PLL_CLK__80M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x2b);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x3333);
			value_reg_emmc_pll_pdiv = 2;// PostDIV: 4
			break;

		case EMMC_PLL_CLK__72M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x30);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x0000);
			value_reg_emmc_pll_pdiv = 2;// PostDIV: 4
			break;

		case EMMC_PLL_CLK__62M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x37);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0xbdef);
			value_reg_emmc_pll_pdiv = 2;// PostDIV: 4
			break;

		case EMMC_PLL_CLK__52M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x42);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x7627);
			value_reg_emmc_pll_pdiv = 2;// PostDIV: 4
			break;

		case EMMC_PLL_CLK__48M:
			REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x48);
			REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x0000);
			value_reg_emmc_pll_pdiv = 2;// PostDIV: 4
			break;

		case EMMC_PLL_CLK__40M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x2b);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x3333);
			value_reg_emmc_pll_pdiv = 4;// PostDIV: 8
			break;

		case EMMC_PLL_CLK__36M:
			REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x30);
			REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x0000);
			value_reg_emmc_pll_pdiv = 4;// PostDIV: 8
			break;

		case EMMC_PLL_CLK__32M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x36);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x0000);
			value_reg_emmc_pll_pdiv = 4;// PostDIV: 8
			break;

		case EMMC_PLL_CLK__27M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x40);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x0000);
			value_reg_emmc_pll_pdiv = 4;// PostDIV: 8
			break;

		case EMMC_PLL_CLK__20M:
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x19), 0x2b);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x18), 0x3333);
			value_reg_emmc_pll_pdiv = 7;// PostDIV: 16
			break;

		default:
			EMMC_DBG(mtk_fcie_host, 0, 0,
				"eMMC Err: emmc PLL not configed %Xh\n",
				clk_param);
			EMMC_DIE(" ");
			return EMMC_ST_ERR_UNKNOWN_CLK;
		}

		// 3. VCO clock (loop N = 4)
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), 0);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), 0x6);

		// 4. 1X clock
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x5),
			BIT(2)|BIT(1)|BIT(0));
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x5),
			value_reg_emmc_pll_pdiv);// PostDIV: 8

		if (clk_param == EMMC_PLL_CLK__20M)
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7), BIT(10));
		else
			REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7), BIT(10));

		if (emmc_drv->tune_overtone == 1) {
			REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1b), BIT(0));

			REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x03), emmcpll_0x03);

			emmc_drv->tune_overtone = 0;
		}
		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_100US); // asked by Irwin
	}

	return EMMC_ST_SUCCESS;
}

/* setting corresponded setting of eMMC pll according to clk_param
 * @ HS400 (ES) mode
 * return 0 is success
 * othewise is fail
 */
void mtk_fcie_pll_dll_setting(u16 clk_param,
					struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u16 reg;
	u16 i = 0, delay_time = 400;

	if (clk_param == emmc_drv->old_pll_dll_clk_param)
		return;

	emmc_drv->old_pll_dll_clk_param = clk_param;

LABEL_TURN_DLL:
	// reg_emmc_rxdll_dline_en
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x09), BIT(0));

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x30), BIT(1));
	// Reset eMMC_DLL
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x30), BIT(2));
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x30), BIT(2));

	//DLL pulse width and phase
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x01), 0x7f72);

	// DLL code
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x32), 0xf200);

	// DLL calibration
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x30), 0x3378);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x33), BIT(15));

	// Wait 250us
	mtk_fcie_hw_timer_delay(delay_time);

	// Get hw dll0 code
	reg = REG(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x33));

	if ((reg & 0x03ff) == 0) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 0,
			"\n EMMCPLL 0x33=%04Xh, i:%u\n", reg, i);
		i++;
		if (i < 10) {
			delay_time += i*50;
			goto LABEL_TURN_DLL;
		} else
			EMMC_DIE("");
	}

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x34), (BIT(10)-1));

	// Set dw dll0 code
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x34), reg&0x03ff);

	// Disable reg_hw_upcode_en
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x30), BIT(8)|BIT(9));

	// Clear reg_emmc_dll_test[7]
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x02), BIT(15));
	if (!mtk_fcie_host->emmc_pll_skew4) {
		// Enable reg_rxdll_dline_en
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x09), BIT_RXDLL_EN);
	}
}

/* setting clock rate for FCIE by 2 parameters.
 * if clk_param has flag of emmc pll flag
 * -> setting fcie to clock source from emmc pll
 * and trigger emmc pll to calibration target clock
 * which is gotten from timing table.
 * if no emmc pll flag
 * -> setting fcie clock by clock framework with hz
 * return 0 is success
 * othewise is fail
 */
u32 mtk_fcie_clock_setting(u16 clk_param,
				struct mtk_fcie_host *mtk_fcie_host,
				u32 hz)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u16 Tmp = 0, Tmp2 = 0;

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), BIT_SD_CLK_EN);

	//check and set clkgen2
	mtk_fcie_host->fcie_syn_clk_rate =
		clk_round_rate(mtk_fcie_host->fcie_syn_clk, 432000000);
	clk_set_rate(mtk_fcie_host->fcie_syn_clk,
		mtk_fcie_host->fcie_syn_clk_rate);

	if (clk_param & EMMC_PLL_FLAG) {
		mtk_fcie_pll_setting(clk_param, mtk_fcie_host);
		if (!hz) {
			switch (clk_param) {
			case EMMC_PLL_CLK_200M:
				emmc_drv->clk_khz = 200000;
				break;
				EMMC_DBG(mtk_fcie_host, 1, 1,
					"eMMC Err: clkgen %Xh\n",
					EMMC_ST_ERR_INVALID_PARAM);
				EMMC_DIE(" ");
				return EMMC_ST_ERR_INVALID_PARAM;
			}
			hz = emmc_drv->clk_khz * 1000;
		}
		if ((emmc_drv->drv_flag &
			(DRV_FLAG_DDR_MODE|DRV_FLAG_SPEED_HS400)) ==
			(DRV_FLAG_DDR_MODE|DRV_FLAG_SPEED_HS400)) { //HS400
			 // tuning DLL setting
			mtk_fcie_pll_dll_setting(clk_param, mtk_fcie_host);
			Tmp = mtk_fcie_host->clk_2xp;
			//500msec= 0x10000 x 1/100MHZ x 0x2fb
			Tmp2 = 0x2fb;
		} else if (emmc_drv->drv_flag &
					DRV_FLAG_SPEED_HS200) { //HS200
			Tmp = mtk_fcie_host->clk_1xp;
			//500msec= 0x10000 x 1/50MHZ x 0x17e
			Tmp2 = 0x17e;
		} else if ((emmc_drv->drv_flag &
			(DRV_FLAG_DDR_MODE|DRV_FLAG_SPEED_HIGH)) ==
			(DRV_FLAG_DDR_MODE|DRV_FLAG_SPEED_HIGH)) { // DDR52
			Tmp = mtk_fcie_host->clk_1xp;
			emmc_drv->clk_khz >>= 2;
			hz >>= 2;
			//500msec= 0x10000 x 1/100MHZ x 0x2fb
			Tmp2 = 0x2fb;
		} else if (emmc_drv->drv_flag &
			DRV_FLAG_SPEED_HIGH) { // HS
			Tmp = mtk_fcie_host->clk_1xp;
			emmc_drv->clk_khz >>= 2;
			hz >>= 2;
			//500msec= 0x10000 x 1/50MHZ x 0x17E
			Tmp2 = 0x17e;
		}
		mtk_fcie_host->fcie_top_clk_rate =
			clk_round_rate(mtk_fcie_host->fcie_top_clk, 12000000);
		clk_set_rate(mtk_fcie_host->fcie_top_clk,
			mtk_fcie_host->fcie_top_clk_rate);
		REG_CLRBIT(mtk_fcie_host->fcieclkreg,
			mtk_fcie_host->clk_mask);
		REG_SETBIT(mtk_fcie_host->fcieclkreg,
			Tmp<<mtk_fcie_host->clk_shift);
	} else {
		Tmp = clk_param;
		//500msec= 0x10000 x 1/12MHZ x 0x5c
		Tmp2 = 0x5c;
		REG_CLRBIT(mtk_fcie_host->fcieclkreg,
			mtk_fcie_host->clk_mask);
		mtk_fcie_host->fcie_top_clk_rate =
				clk_round_rate(mtk_fcie_host->fcie_top_clk, hz);
		clk_set_rate(mtk_fcie_host->fcie_top_clk,
				mtk_fcie_host->fcie_top_clk_rate);

		clk_param = (REG(mtk_fcie_host->fcieclkreg)
						&mtk_fcie_host->clk_mask
						)>>mtk_fcie_host->clk_shift;

		emmc_drv->clk_khz = hz / 1000;
	}
	emmc_drv->clk_reg_val = clk_param;
	mtk_fcie_host->clk = hz;

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, WR_SBIT_TIMER),
		BIT_WR_SBIT_TIMER_EN|Tmp2);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, RD_SBIT_TIMER),
		BIT_RD_SBIT_TIMER_EN|Tmp2);
	return EMMC_ST_SUCCESS;
}

/* gating fcie output clock to eMMC
 */

u32 mtk_fcie_clock_gating(struct mtk_fcie_host *mtk_fcie_host)
{
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), BIT_CLK_EN);
	return EMMC_ST_SUCCESS;
}

/* setting fcie and emmc pll registers according to pad_type
 * return 0 is success
 * othewise is fail
 */

static void mtk_fcie_pads_default(struct mtk_fcie_host *mtk_fcie_host)
{
	// fcie
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, DDR_MODE),
		BIT_MACRO_MODE_MASK|BIT_CLK2_SEL);
	// emmc_pll

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x9), BIT(0));
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a),
		BIT(10)|BIT(5)|BIT(4));
	//meta issue of clk4x and skew
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1c),
		BIT(9)|BIT(8));
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1f),
		BIT(2));
	// reg_sel_internal
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x20),
		BIT(0)|BIT(9));
	// reg_use_rxdll
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x63), BIT(0));
	// reg_emmc_en | reg_emmc_ddr_en
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x68),
		BIT(0)|BIT(1));
	// reg_tune_shot_offset
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x69),
		BIT(7)|BIT(6)|BIT(5)|BIT(4));
	// reg_io_bus_wid
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
		BIT(0)|BIT(1));
	// reg_dqs_page_no
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
		0x0000);
	// reg_ddr_io_mode
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6d), BIT(0));
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x70),
		BIT(8)|BIT(10)|BIT(11));
	// reg_tx_bps_en
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x71), 0xffff);
	// reg_rx_bps_en
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x73), 0xffff);
	//reg_afifo_wen
	if (mtk_fcie_host->v_emmc_pll == 7 || mtk_fcie_host->v_emmc_pll == 12
		|| mtk_fcie_host->v_emmc_pll == 22)
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x74),
			BIT(14));
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7f),
		BIT(1)|BIT(2)|BIT(3)|BIT(8)|BIT(9)|BIT(10));

	if (mtk_fcie_host->emmc_pll_flash_macro)
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x5f),
			BIT_FLASH_MACRO_TO_FICE);
}

static void mtk_fcie_pads_bypass(struct mtk_fcie_host *mtk_fcie_host)
{
	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_LOW, 1, "Bypass\n");
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, DDR_MODE),
		BIT_PAD_IN_SEL_SD|BIT_FALL_LATCH|BIT_CLK2_SEL);

	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a), BIT(10));
}

static void mtk_fcie_pads_sdr(struct mtk_fcie_host *mtk_fcie_host)
{
	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_LOW, 1, "SDR 8-bit macro\n");
	// emmc_pll
	//meta issue of clk4x and skew
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1c), BIT(9));
	// 8 bits macro reset + 32 bits macro reset
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	// reg_emmc_en
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x68), BIT(0));
	// fcie
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, DDR_MODE),
		BIT_8BIT_MACRO_EN);
}

static u32 mtk_fcie_pads_ddr(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err = EMMC_ST_SUCCESS;

	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_LOW, 1, "DDR\n");
	if (emmc_drv->bus_width == BIT_SD_DATA_WIDTH_4) {
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
			1<<0);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
			0x0213);
	} else if (emmc_drv->bus_width ==
		BIT_SD_DATA_WIDTH_8) {
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
			2<<0);
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
			0x0113);
	} else {
		err = EMMC_ST_ERR_INVALID_PARAM;
		goto ErrorHandle;
	}
	// emmc_pll
	//meta issue of clk4x and skew
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1c), BIT(9));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6d), BIT(0));
	// 8 bits macro reset + 32 bits macro reset
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	// reg_emmc_en
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x68), BIT(0));
	// fcie
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, DDR_MODE),
		BIT_DDR_EN|BIT_8BIT_MACRO_EN);

ErrorHandle:

	return err;
}

static u32 mtk_fcie_pads_hs200(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err = EMMC_ST_SUCCESS;

	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_LOW, 1, "HS200\n");
	// emmc_pll
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a),
		BIT(5)|BIT(4));
	//meta issue of clk4x and skew
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1c), BIT(8));
	// reg_sel_internal
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x20),
		BIT(9)|BIT(10));
	// 8 bits macro reset + 32 bits macro reset
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	// reg_emmc_en
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x68), BIT(0));
	// fcie
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, DDR_MODE),
		BIT_32BIT_MACRO_EN);
	// reg_tune_shot_offset
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x69), 4<<4);
	if (mtk_fcie_host->emmc_pll_skew4) {
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x69),
			BIT_SKEW4_CLK_INV_MASK);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x69),
			BIT_SKEW4_ALL_INV);
	}



	// reg_clk_dig_inv
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x69), BIT(3));

	if (emmc_drv->bus_width == BIT_SD_DATA_WIDTH_4) {
		// reg_io_bus_wid
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
		1<<0);
		// reg_dqs_page_no
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
			0x0413);
	} else if (emmc_drv->bus_width ==
		BIT_SD_DATA_WIDTH_8) {
		// reg_io_bus_wid
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
			2<<0);
		// reg_dqs_page_no
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
			0x0213);
	} else {
		err = EMMC_ST_ERR_INVALID_PARAM;
		goto ErrorHandle;
	}
	// reg_sel_flash_32bif
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x70), BIT(8));
	// reg_pll_clkph_skew[123]
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x3), 0x0fff);
	// reg_tx_bps_en
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x71), 0xf800);
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x73), 0xfd00);
	if (mtk_fcie_host->v_emmc_pll == 7 || mtk_fcie_host->v_emmc_pll == 12
		|| mtk_fcie_host->v_emmc_pll == 22)
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x74),
			BIT(14));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x70),
		BIT(10)|BIT(11));

	if (mtk_fcie_host->emmc_pll_flash_macro)
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x5f),
			BIT_FLASH_MACRO_TO_FICE);


ErrorHandle:

	return err;
}

static u32 mtk_fcie_pads_hs400(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err = EMMC_ST_SUCCESS;

	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_LOW, 1, "HS400\n");
	// emmc_pll
	// reg_emmc_rxdll_dline_en
	if (mtk_fcie_host->emmc_pll_skew4) {
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x09),
			BIT_RXDLL_EN);
	} else {
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x09),
			BIT_RXDLL_EN);
	}
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a),
		BIT(5)|BIT(4));
	//meta issue of clk4x and skew
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1c), BIT(8));

	if (mtk_fcie_host->emmc_pll_skew4) {
		// reg_sel_internal
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x20),
			BIT_SEL_INTERNAL_MASK);
		// reg_use_rxdll
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x63),
			BIT_USE_RXDLL);
	} else {
		// reg_sel_internal
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x20),
			BIT_SEL_SKEW4_FOR_CMD);
		// reg_use_rxdll
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x63), BIT(0));
	}
	// 8 bits macro reset + 32 bits macro reset
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	// reg_emmc_en | reg_emmc_ddr_en
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x68),
		BIT(0)|BIT(1));
	// fcie
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, DDR_MODE),
		BIT_32BIT_MACRO_EN|BIT_DDR_EN);
	// reg_clk_dig_inv
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x69), BIT(3));
	// reg_tune_shot_offset
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x69), 6<<4);
	if (mtk_fcie_host->emmc_pll_skew4) {
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x69),
			BIT_SKEW4_CLK_INV_MASK);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x69),
			BIT_SKEW4_ALL_INV);
	}

	if (emmc_drv->bus_width == BIT_SD_DATA_WIDTH_4) {
		// reg_io_bus_wid
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
			1<<0);
		// reg_dqs_page_no
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
			0x0213);
	} else if (emmc_drv->bus_width ==
		BIT_SD_DATA_WIDTH_8) {
		// reg_io_bus_wid
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
			2<<0);
		// reg_dqs_page_no
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
			0x0113);
	} else {
		err = EMMC_ST_ERR_INVALID_PARAM;
		goto ErrorHandle;
	}
	// reg_sel_flash_32bif
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x70), BIT(8));
	//only clean skew1 & skew3 , skew2 is set with table value
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x03), 0x0f0f);
	// reg_tx_bps_en
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x71), 0xf800);
	// reg_rx_bps_en
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x73), 0xfd00);
	if (mtk_fcie_host->v_emmc_pll == 7 || mtk_fcie_host->v_emmc_pll == 12
		|| mtk_fcie_host->v_emmc_pll == 22)
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x74),
			BIT(14));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x70),
		BIT(10)|BIT(11));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7f),
		BIT(2)|BIT(11));

	if (mtk_fcie_host->emmc_pll_flash_macro)
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x5f),
			BIT_FLASH_MACRO_TO_FICE);


ErrorHandle:

	return err;
}

static u32 mtk_fcie_pads_hs400_5_1(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err = EMMC_ST_SUCCESS;

	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_LOW, 0, "HS400 5.1\n");
	// emmc_pll
	// reg_emmc_rxdll_dline_en
	if (mtk_fcie_host->emmc_pll_skew4) {
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x09),
			BIT_RXDLL_EN);
	} else {
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x09),
			BIT_RXDLL_EN);
	}

	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a),
		BIT(5)|BIT(4));
	//meta issue of clk4x and skew
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1c), BIT(8));

	if (mtk_fcie_host->emmc_pll_skew4) {
		// reg_sel_internal
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x20),
			BIT_SEL_INTERNAL_MASK);
		// reg_use_rxdll
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x63),
			BIT_USE_RXDLL);
	} else {
		// reg_use_rxdll
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x63), BIT_USE_RXDLL);
	}
	// 8 bits macro reset + 32 bits macro reset
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6f),
		BIT(0)|BIT(1));
	// reg_emmc_en | reg_emmc_ddr_en
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x68),
		BIT(0)|BIT(1));
	// fcie
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, DDR_MODE),
		BIT_32BIT_MACRO_EN|BIT_DDR_EN);
	// reg_clk_dig_inv
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x69), BIT(3));
	// reg_tune_shot_offset
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x69), 6<<4);
	if (mtk_fcie_host->emmc_pll_skew4) {
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x69),
			BIT_SKEW4_CLK_INV_MASK);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x69),
			BIT_SKEW4_ALL_INV);
	}

	if (emmc_drv->bus_width == BIT_SD_DATA_WIDTH_4) {
		// reg_io_bus_wid
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
			1<<0);
		// reg_dqs_page_no
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
			0x0213);
	} else if (emmc_drv->bus_width ==
		BIT_SD_DATA_WIDTH_8) {
		// reg_io_bus_wid
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6a),
			2<<0);
		// reg_dqs_page_no
		REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6b),
			0x0113);
	} else {
		err = EMMC_ST_ERR_INVALID_PARAM;
		goto ErrorHandle;
	}
	// reg_sel_flash_32bif
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x70), BIT(8));
	//only clean skew1 & skew3 , skew2 is set with table value
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x03), 0x0f0f);
	// reg_tx_bps_en
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x71), 0xf800);
	// reg_rx_bps_en
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x73), 0xfd00);
	if (mtk_fcie_host->v_emmc_pll == 7 || mtk_fcie_host->v_emmc_pll == 12
		|| mtk_fcie_host->v_emmc_pll == 22)
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x74),
			BIT(14));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x70),
		BIT(10)|BIT(11));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x7f),
		BIT(2)|BIT(8)|BIT(11));

	if (mtk_fcie_host->emmc_pll_flash_macro)
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, reg_emmcpll_0x5f),
			BIT_FLASH_MACRO_TO_FICE);

ErrorHandle:

	return err;
}


static void mtk_fcie_set_drv_flag(struct fcie_emmc_driver	*emmc_drv)
{
	emmc_drv->drv_flag &=
		(~(DRV_FLAG_SPEED_HIGH|DRV_FLAG_DDR_MODE|
		DRV_FLAG_SPEED_HS200|DRV_FLAG_SPEED_HS400));

	if (emmc_drv->pad_type == FCIE_EMMC_SDR)
		emmc_drv->drv_flag |= DRV_FLAG_SPEED_HIGH;
	else if (emmc_drv->pad_type == FCIE_EMMC_DDR) {
		emmc_drv->drv_flag |=
			DRV_FLAG_DDR_MODE|DRV_FLAG_SPEED_HIGH;
	} else if (emmc_drv->pad_type == FCIE_EMMC_HS200)
		emmc_drv->drv_flag |= DRV_FLAG_SPEED_HS200;
	else if (emmc_drv->pad_type == FCIE_EMMC_HS400 ||
		emmc_drv->pad_type == FCIE_EMMC_HS400_5_1) {
		emmc_drv->drv_flag |=
			DRV_FLAG_DDR_MODE|DRV_FLAG_SPEED_HS400;
	}
}


u32 mtk_fcie_pads_switch(u8 pad_type, struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err = EMMC_ST_SUCCESS;

	emmc_drv->pad_type = pad_type;

	mtk_fcie_pads_default(mtk_fcie_host);

#if defined(CONFIG_MSTAR_ARM_BD_FPGA) && CONFIG_MSTAR_ARM_BD_FPGA
	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_HIGH, 1, "Bypass\n");
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, DDR_MODE),
		BIT_PAD_IN_SEL_SD|BIT_FALL_LATCH|BIT_CLK2_SEL);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a), BIT(10));

	return EMMC_ST_SUCCESS;
#else

	switch (pad_type) {
	case FCIE_EMMC_BYPASS:
		mtk_fcie_pads_bypass(mtk_fcie_host);
		break;

	case FCIE_EMMC_SDR:
		mtk_fcie_pads_sdr(mtk_fcie_host);
		break;

	case FCIE_EMMC_DDR_8BIT_MACRO:
		err = mtk_fcie_pads_ddr(mtk_fcie_host);
		if (err)
			goto ErrorHandle;
		break;

	case FCIE_EMMC_HS200:
		err = mtk_fcie_pads_hs200(mtk_fcie_host);
		if (err)
			goto ErrorHandle;
		break;
	case FCIE_EMMC_HS400:
		err = mtk_fcie_pads_hs400(mtk_fcie_host);
		if (err)
			goto ErrorHandle;
		break;
	case FCIE_EMMC_HS400_5_1:
		err = mtk_fcie_pads_hs400_5_1(mtk_fcie_host);
		if (err)
			goto ErrorHandle;
		break;
	default:
		EMMC_DBG(mtk_fcie_host, 1, 1,
			"eMMC Err: wrong parameter for switch pad func\n");
		return EMMC_ST_ERR_PARAMETER;
	}

	mtk_fcie_set_drv_flag(emmc_drv);

	return EMMC_ST_SUCCESS;
ErrorHandle:
	EMMC_DBG(mtk_fcie_host, 1, 1,
		"eMMC Err: set bus width before pad switch\n");
	return EMMC_ST_ERR_INVALID_PARAM;
#endif
}

/* wait emmc busy is done within given time "us"
 *
 * return total time have been spended for waiting.
 */

u32 _mtk_fcie_wait_d0_high(u32 us, struct mtk_fcie_host *mtk_fcie_host)
{
	u32 cnt;
	u16 read0 = 0, read1 = 0;

	for (cnt = 0; cnt < us; cnt++) {
		read0 = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));
		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_1US);
		read1 = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));

		if ((read0&BIT_SD_CARD_BUSY) == 0 &&
			(read1&BIT_SD_CARD_BUSY) == 0)
			break;
		cpu_relax();
	}

	return cnt;
}

/* clear all fcie HW events
 */

static void mtk_fcie_clear_events(struct mtk_fcie_host *mtk_fcie_host)
{
	u16 reg;
	unsigned long flags;


	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS),
		BIT_SD_FCIE_ERR_FLAGS);

	while (1) {
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT),
			BIT_ALL_CARD_INT_EVENTS);
		reg = REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT));
		if (0 == (reg&BIT_ALL_CARD_INT_EVENTS))
			break;
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT), 0);
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT), 0);
	}

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), 0);
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);
}

/* reset the eMMC controller fcie
 */

static u32 _mtk_fcie_reset(struct mtk_fcie_host *mtk_fcie_host)
{
	u16 cnt;

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL),
		BIT_JOB_START); // clear for safe
	/* active low */
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, RST),
		BIT_FCIE_SOFT_RST_n);

	cnt = 0;
	while (1) {
		 // reset success
		if ((REG(REG_ADDR(mtk_fcie_host->fciebase, RST))
				&BIT_RST_STS_MASK) == BIT_RST_STS_MASK)
			break;

		udelay(HW_TIMER_DELAY_1US);

		if (cnt++ >= MAX_RETRY_TIME)
			return EMMC_ST_ERR_FCIE_NO_CLK;
	}

	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, RST),
		BIT_FCIE_SOFT_RST_n);
	cnt = 0;
	while (1) {
		// reset success
		if (!(REG(REG_ADDR(mtk_fcie_host->fciebase, RST))
				&BIT_RST_STS_MASK))
			break;

		udelay(HW_TIMER_DELAY_1US);

		if (cnt++ >= MAX_RETRY_TIME)
			return EMMC_ST_ERR_FCIE_NO_CLK;
	}

	return 0;
}


static u32 mtk_fcie_reset(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u16 cnt;
	u16 clk = emmc_drv->clk_reg_val;
	u32 clk_orig = mtk_fcie_host->clk;

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL),
		BIT_JOB_START); // clear for safe
	// instead reset clock source from UPLL to XTAL
	mtk_fcie_clock_setting(0, mtk_fcie_host, CLK_48MHz);
	/* active low */
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, RST),
		BIT_FCIE_SOFT_RST_n);

	cnt = 0;
	while (1) {
		 // reset success
		if ((REG(REG_ADDR(mtk_fcie_host->fciebase, RST))
				&BIT_RST_STS_MASK) == BIT_RST_STS_MASK)
			break;

		udelay(HW_TIMER_DELAY_1US);

		if (cnt++ >= 1000) {
			EMMC_DBG(mtk_fcie_host, 1, 0,
				"eMMC Err: FCIE reset fail!\n");
			mtk_fcie_reg_dump(mtk_fcie_host);
			return EMMC_ST_ERR_FCIE_NO_CLK;
		}
	}

	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, RST),
		BIT_FCIE_SOFT_RST_n);
	cnt = 0;
	while (1) {
		// reset success
		if (!(REG(REG_ADDR(mtk_fcie_host->fciebase, RST))
				&BIT_RST_STS_MASK))
			break;

		udelay(HW_TIMER_DELAY_1US);

		if (cnt++ >= 1000) {
			EMMC_DBG(mtk_fcie_host, 1, 0,
				"eMMC Err: FCIE reset fail2!\n");
			mtk_fcie_reg_dump(mtk_fcie_host);
			return EMMC_ST_ERR_FCIE_NO_CLK;
		}
	}
	// instead reset clock source from UPLL to XTAL
	mtk_fcie_clock_setting(clk, mtk_fcie_host, clk_orig);

	return 0;
}


/* isr thread to handle event trigger from power lose signal
 *  first to disable HW command queue to avoid multi-trigger this event
 *  then reset HW command queue and FCIE controller.
 *  wait 400ms for power good if power glitch happens not power lose.
 */
irqreturn_t mtk_fcie_irq_thread(int irq, void *dummy)
{
	u16 events;
	struct mtk_fcie_host *mtk_fcie_host = (struct mtk_fcie_host *) dummy;


	if ((REG(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL))&
		(BIT_POWER_SAVE_MODE_INT|BIT_POWER_SAVE_MODE_INT_EN))
		== (BIT_POWER_SAVE_MODE_INT|BIT_POWER_SAVE_MODE_INT_EN)) {
		events = REG(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL));
		while (1) {
			// disable power saving mode to avoid HW keeping trigger
			REG_W(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
				BIT_SD_POWER_SAVE_RST);
			// clear the CMD0 status by power-saving mode
			// to make foreground thread wait event timeout
			if ((
			REG(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL))
			& BIT_SD_POWER_SAVE_RST) == BIT_SD_POWER_SAVE_RST &&
			(REG(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL))
			& BIT_POWER_SAVE_MODE) == 0)
				break;
		}

		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"SAR5 eMMC Warn: %04Xh %04Xh %04Xh\n", events,
			REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT)),
			REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN)));
		EMMC_DIE("SAR5 eMMC occurred\n");
	}
	return IRQ_HANDLED;
}



/* initialize fcie state and host variable
 *
 * return 0: wait the event success
 * othewise: fail to reset fcie HW
 */

static int mtk_fcie_hw_init(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err;
	unsigned long flags;

	//reset retry status to default

	// for eMMC 4.5 HS200 need 1.8V, unify all eMMC IO power to 1.8V
	// works both for eMMC 4.4 & 4.5
	// EMMC_DBG(EMMC_DBG_LVL_LOW, 1, "1.8V IO power for eMMC\n");
	// Irwin Tyan: set this bit to boost IO performance at low power supply.

	if ((REG(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a)) & BIT(0)) != 1) {
		//EMMC_DBG(0, 0, "eMMC: set 1.8V\n");
		 // 1.8V must set this bit
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a), BIT(0));
		 // atop patch
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1a), BIT(2));
		if (mtk_fcie_host->host_driving) {
			// 1.8V must set this bit
			REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x45),
				0x0fff);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x45),
				mtk_fcie_host->host_driving);
		}
	}

	mtk_fcie_pads_switch(FCIE_EMMC_BYPASS, mtk_fcie_host);
	mtk_fcie_clock_setting(0, mtk_fcie_host, 300000);
	emmc_drv->bus_width = BIT_SD_DATA_WIDTH_1;
	emmc_drv->reg10_mode = BIT_SD_DEFAULT_MODE_REG;
	emmc_drv->rca = 1;
	// ------------------------------------------

	// ------------------------------------------
	err = mtk_fcie_reset(mtk_fcie_host);
	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: reset fail: %Xh\n", err);
		return err;
	}
	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), 0);
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_FUNC_CTL),
		BIT_SD_EN);
	// all cmd are 5 bytes (excluding CRC)
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, CMD_RSP_SIZE),
		BIT_CMD_SIZE_MASK);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, CMD_RSP_SIZE),
		(EMMC_CMD_BYTE_CNT) << BIT_CMD_SIZE_SHIFT);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL), 0);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE),
		emmc_drv->reg10_mode);
	// default sector size: 0x200
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, BLK_SIZE),
		EMMC_SECTOR_512BYTE);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, RSP_SHIFT_CNT), 0);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, RX_SHIFT_CNT), 0);



	//check fde switch bit
	if (mtk_fcie_host->enable_fde &&
		mtk_fcie_host->fdebase != NULL) {
		if ((REG(REG_ADDR(mtk_fcie_host->fdebase, AES_FUN))
			& BIT_AES_SWITCH_ON) == 0)
			mtk_fcie_host->enable_fde = 0;
	}

	return EMMC_ST_SUCCESS; // ok
}

/* initialize fcie HW command queue
 *  fill command queue to issue command 0 to eMMC before power lose
 *  automatically by HW
 */

static void mtk_fcie_prepare_psm_queue(struct mtk_fcie_host *mtk_fcie_host)
{
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
		BIT_BAT_SD_POWER_SAVE_MASK);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
		BIT_RST_SD_POWER_SAVE_MASK);

	if (mtk_fcie_host->support_cqe) {
		/* (1) Clear CQE Enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x00), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x01),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK2|0x44);

		/* (2) Clear HW Enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x02), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x03),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x07);

		/* (3) Clear All Interrupt */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x04), 0xffff);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x05),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x00);

		/* (4) Clear SD MODE Enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x06), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x07),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0b);

		/* (5) Clear SD CTL Enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x08), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x09),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0c);

		/* (6) Disable All CQE Interrupts */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0A), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0B),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK2|0x4A);

		/* (7) Disable All CQE Interrupts */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0C), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0D),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK2|0x4C);

		/* (8) Reset Start */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0E), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0F),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x3f);

		/* (9) Reset End */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x10), 0x0001);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x11),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x3f);

		/* (10) Set "FCIE_DDR_MODE" */
		#if defined(ENABLE_EMMC_HS400) && ENABLE_EMMC_HS400
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x12), 0x1100);
		#elif defined(ENABLE_EMMC_HS200) && ENABLE_EMMC_HS200
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x12), 0x1000);
		#elif defined(ENABLE_EMMC_ATOP) && ENABLE_EMMC_ATOP
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x12), 0x0180);
		#endif
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x13),
		PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0f);

		/* (11) Set "SD_MOD" */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x14), 0x0021);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x15),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0b);

		/* (12) Enable "reg_sd_en" */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x16), 0x1001);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x17),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x07);

		/* (13) Command Content, IDLE */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x18), 0x0040);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x19),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x20);

		/* (14) Command Content, STOP */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1A), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1B),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x21);

		/* (15) Command Content, STOP */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1C), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1D),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x22);

		/* (16) Command & Response Size */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1E), 0x0500);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1F),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0a);

		/* (17) Enable Interrupt, SD_CMD_END */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x20), 0x0002);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x21),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x01);

		/* (18) Command Enable + job Start */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x22), 0x0044);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x23),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0c);

		/* (19) Wait Interrupt */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x24), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x25),
			PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WINT);

		/* (20) Clear All interrupt enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x26), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x27),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x01);

		/* (21) Clear All event  (should put after clear all interrupt enable)*/
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x28), 0xffff);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x29),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x00);

		/* (22) STOP */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x2A), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x2B),
			PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_STOP);

		REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
		BIT_SD_POWER_SAVE_RST);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
		BIT_SD_POWER_SAVE_RST);

		REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
		BIT_POWER_SAVE_MODE|BIT_POWER_SAVE_MODE_INT_EN);
	} else {
		/* (1) Clear HW Enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x00), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x01),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x07);

		/* (2) Clear All Interrupt */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x02), 0xffff);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x03),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x00);

		/* (3) Clear SD MODE Enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x04), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x05),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0b);

		/* (4) Clear SD CTL Enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x06), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x07),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0c);

		/* (5) Reset Start */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x08), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x09),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x3f);

		/* (6) Reset End */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0a), 0x0001);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0b),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x3f);

		/* (7) Set "FCIE_DDR_MODE" */
		#if defined(ENABLE_EMMC_HS400) && ENABLE_EMMC_HS400
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0c), 0x1100);
		#elif defined(ENABLE_EMMC_HS200) && ENABLE_EMMC_HS200
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0c), 0x1000);
		#elif defined(ENABLE_EMMC_ATOP) && ENABLE_EMMC_ATOP
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0c), 0x0180);
		#endif
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0d),
		PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0f);

		/* (8) Set "SD_MOD" */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0e), 0x0021);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x0f),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0b);

		/* (9) Enable "reg_sd_en" */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x10), 0x1001);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x11),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x07);

		/* (10) Command Content, IDLE */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x12), 0x0040);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x13),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x20);

		/* (11) Command Content, STOP */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x14), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x15),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x21);

		/* (12) Command Content, STOP */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x16), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x17),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x22);

		/* (13) Command & Response Size */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x18), 0x0500);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x19),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0a);

		/* (14) Enable Interrupt, SD_CMD_END */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1a), 0x0002);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1b),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x01);

		/* (15) Command Enable + job Start */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1c), 0x0044);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1d),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x0c);

		/* (16) Wait Interrupt */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1e), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x1f),
			PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_WINT);

		/* (17) Clear All interrupt enable */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x20), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x21),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x01);

		/* (18) Clear All event  (should put after clear all interrupt enable)*/
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x22), 0xffff);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x23),
			PWR_BAT_CLASS|PWR_RST_CLASS|PWR_CMD_WREG|PWR_CMD_BK0|0x00);

		/* (19) STOP */
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x24), 0x0000);
		REG_W(REG_ADDR(mtk_fcie_host->pwsbase, 0x25),
			PWR_BAT_CLASS | PWR_RST_CLASS | PWR_CMD_STOP);

		REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
			BIT_SD_POWER_SAVE_RST);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
			BIT_SD_POWER_SAVE_RST);

		REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
			BIT_POWER_SAVE_MODE|BIT_POWER_SAVE_MODE_INT_EN);
	}
}




/* lock fcie HW resource if fcie or pin is shared with other driver
 */

void mtk_fcie_lock_host(u8 *str, struct mtk_fcie_host *mtk_fcie_host)
{

	//EMMC_DBG(0,1,"%s 1\n", str);


	// output clock
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE),
		BIT_SD_CLK_EN);

	//EMMC_DBG(0,1,"%s 2\n", str);
}

/* ulock fcie HW resource if fcie or pin is shared with other driver
 */

void mtk_fcie_unlock_host(u8 *str, struct mtk_fcie_host *mtk_fcie_host)
{

	//EMMC_DBG(0,1,"%s 1\n", str);
	mtk_fde_func_dis(mtk_fcie_host);


	// not output clock

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE),
		BIT_SD_CLK_EN);

	//EMMC_DBG(0,1,"%s 2\n", str);
}

/* fcie internal parameter setting
 */

void mtk_fcie_set_delay_latch(u32 value, struct mtk_fcie_host *mtk_fcie_host)
{
	if (value > 31) {
		EMMC_DBG(mtk_fcie_host, 0, 1,
			"eMMC Err: wrong delay latch value\n");
		return;
	}

	if (value%2)
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x09), BIT(1));
	else
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x09), BIT(1));

	value = (value&0xfffe) >> 1;

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x09), (0x000f<<4));
	REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x09), (value<<4));
}

/* fcie internal parameter setting
 */

void mtk_fcie_set_atop_timing_reg(u8 set_idx,
	struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	struct fcie_timing_table *t_table = &emmc_drv->t_table;

	if (emmc_drv->pad_type == FCIE_EMMC_DDR) {
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6c),
			BIT_DQS_MODE_MASK|BIT_DQS_DELAY_CELL_MASK);

		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6c),
			t_table->set[set_idx].skew4
			<<BIT_DQS_MODE_SHIFT);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6c),
			t_table->set[set_idx].cell
			<<BIT_DQS_DELAY_CELL_SHIFT);
	} else {//HS400,HS200
		if (emmc_drv->t_table.set[set_idx].reg2ch)
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6c),
				BIT(7));
		else
			REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x6c),
				BIT(7));

		REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x03),
			BIT_SKEW4_MASK);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x03),
			t_table->set[set_idx].skew4<<12);

		if (emmc_drv->pad_type == FCIE_EMMC_HS400 ||
			emmc_drv->pad_type == FCIE_EMMC_HS400_5_1
			) {
			mtk_fcie_set_delay_latch(
				t_table->set[set_idx].cell,
				mtk_fcie_host);

			REG_CLRBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x03),
				BIT_SKEW2_MASK);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x03),
				t_table->set[set_idx].skew2<<4);
		}
	}
}

/* fcie internal parameter setting from timing table
 */

void mtk_fcie_apply_reg_table(struct mtk_fcie_host *mtk_fcie_host, u8 set_idx)
{
	struct fcie_reg_set *regset = mtk_fcie_host->emmc_drv->t_table_g.regset;
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	int i, apply_idx;
	int reg_cnt = emmc_drv->t_table_g.register_cnt;

	for (i = 0; i < reg_cnt; i++) {
		apply_idx = set_idx*reg_cnt+i;
		if (regset[apply_idx].opcode == REG_OP_W)
			//address = (bank address+ register offset) << 2
			REG_W(mtk_fcie_host->riubaseaddr+
					regset[apply_idx].reg_addr,
					regset[apply_idx].reg_value);
		else if (regset[apply_idx].opcode == REG_OP_CLRBIT)
			REG_CLRBIT(mtk_fcie_host->riubaseaddr+
				regset[apply_idx].reg_addr,
				regset[apply_idx].reg_value);
		else if (regset[apply_idx].opcode == REG_OP_SETBIT) {
			REG_CLRBIT(mtk_fcie_host->riubaseaddr+
				regset[apply_idx].reg_addr,
				regset[apply_idx].reg_mask);

			REG_SETBIT(mtk_fcie_host->riubaseaddr+
				regset[apply_idx].reg_addr,
				regset[apply_idx].reg_value);
		} else if (regset[apply_idx].opcode == 0)
			;
			//do nothing here
		else {
			EMMC_DBG(mtk_fcie_host,
				EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: unknown OpCode 0x%X\n",
				regset[apply_idx].opcode);
			EMMC_DIE("");
		}
	}
}

/* fcie internal parameter setting by different timing mode
 */
void mtk_fcie_apply_timing_set(u8 idx, struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	// make sure a complete outside clock cycle
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE),
		BIT_CLK_EN);

	// HS400 or HS200 or DDR52
	if (emmc_drv->t_table_g.set_cnt == 0) {
		mtk_fcie_clock_setting(
			emmc_drv->t_table.set[idx].clk,
			mtk_fcie_host, 200000000);
		mtk_fcie_set_atop_timing_reg(idx, mtk_fcie_host);
		emmc_drv->t_table.curset_idx = idx;
	} else {	//apply ext table
		mtk_fcie_clock_setting(
			emmc_drv->t_table_g.clk,
			mtk_fcie_host, 200000000);
		mtk_fcie_apply_reg_table(mtk_fcie_host, idx);
		emmc_drv->t_table_g.curset_idx = idx;
	}
}

/* fcie internal use for reading timing table by issue request to mmc layer
 */

static u32 mtk_fcie_read_block(struct mmc_host *host, u8 *buf, ulong blkaddr)
{
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;
	u8 *data_buf = NULL;
	u32 err = 0;


	data_buf = kzalloc(512, GFP_NOIO);
	if (!data_buf)
		return -ENOMEM;


	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = MMC_READ_SINGLE_BLOCK;
	cmd.arg = blkaddr;

	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = 512;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	sg_init_one(&sg, data_buf, 512);

	data.timeout_ns = TIME_WAIT_1_BLK_END * 1000;
	data.timeout_clks = 0;

	mmc_wait_for_req(host, &mrq);

	if (cmd.error)
		err = cmd.error;

	if (data.error)
		err = data.error;
	if (!err)
		memcpy(buf, data_buf, 512);

	kfree(data_buf);
	return err;
}

/* fcie internal use for specify timing table check sum calcuation
 */

static u32 mtk_fcie_chk_sum(u8 *data, u32 cnt)
{
	int i;
	u32 sum = 0;

	for (i = 0; i < cnt; i++)
		sum += data[i];
	return sum;
}

/* fcie internal use to load timing table by different speed mode
 */

static u32 _mtk_fcie_load_timing_table(struct mtk_fcie_host *mtk_fcie_host,
	u8 pad_type, u8 *buf)
{
	u32 err;
	u32 chk_sum, emmc_blk_addr;
	struct fcie_timing_table *tab = (struct fcie_timing_table *)buf;
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u8 *sect_buf;

	switch (pad_type) {
	case FCIE_EMMC_HS400_5_1:
	case FCIE_EMMC_HS400:
		emmc_blk_addr = EMMC_HS400TABLE_BLK_0;
		break;
	case FCIE_EMMC_HS200:
		emmc_blk_addr = EMMC_HS200TABLE_BLK_0;
		break;
	case FCIE_EMMC_DDR:
		emmc_blk_addr = EMMC_DDRTABLE_BLK_0;
		break;
	default:
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: unknown pad type %Xh\n", pad_type);
		return EMMC_ST_ERR_DDRT_NONA;
	}

	sect_buf = kzalloc(512, GFP_NOIO);
	if (!sect_buf)
		return EMMC_ST_ERR_PARAMETER;

	// --------------------------------------
	mtk_fcie_unlock_host((u8 *)__func__, mtk_fcie_host);
	err = mtk_fcie_read_block(mtk_fcie_host->mmc,
		sect_buf, emmc_blk_addr);
	mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);
	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: load 1 Table fail, %Xh\n", err);

		mtk_fcie_unlock_host((u8 *)__func__, mtk_fcie_host);
		err = mtk_fcie_read_block(mtk_fcie_host->mmc,
			sect_buf, emmc_blk_addr + 1);
		mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);
		if (err != EMMC_ST_SUCCESS) {
			EMMC_DBG(mtk_fcie_host,
				EMMC_DBG_LVL_ERROR, 1,
				"eMMC Warn: load 2 Tables fail, %Xh\n", err);
			kfree(sect_buf);
			return EMMC_ST_ERR_NOTABLE;
		}
	}

	// --------------------------------------
	memcpy((void *)tab, sect_buf,
		sizeof(emmc_drv->t_table));

	chk_sum = mtk_fcie_chk_sum((u8 *)tab,
		sizeof(emmc_drv->t_table)
		- EMMC_TIMING_TABLE_CHKSUM_OFFSET);
	if (chk_sum != tab->chk_sum) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: ChkSum error, no Table\n");
		dev_err(mtk_fcie_host->dev,
			"calcuate chksum=%08X\n", chk_sum);
		dev_err(mtk_fcie_host->dev,
			"t_table chksum=%08X\n", tab->chk_sum);
		kfree(sect_buf);
		err = EMMC_ST_ERR_DDRT_CHKSUM;
		return EMMC_ST_ERR_NOTABLE;
	}

	if (chk_sum == 0) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: no Table\n");
		err = EMMC_ST_ERR_DDRT_NONA;
		kfree(sect_buf);
		return EMMC_ST_ERR_NOTABLE;
	}
	kfree(sect_buf);
	return EMMC_ST_SUCCESS;
}

/* fcie internal use to load timing table for HS400/HS400(ES)
 */

static u32 mtk_fcie_load_timing_table_g(struct mtk_fcie_host *mtk_fcie_host)
{
	u32 err;
	u32 chk_sum;
	u8 *sect_buf;
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;

	sect_buf = kzalloc(512, GFP_NOIO);
	if (!sect_buf)
		return EMMC_ST_ERR_PARAMETER;

	mtk_fcie_unlock_host((u8 *)__func__, mtk_fcie_host);
	err = mtk_fcie_read_block(mtk_fcie_host->mmc,
		sect_buf, EMMC_HS400EXTTABLE_BLK_0);
	mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);
	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: load 1st Gen_TTable fail, %Xh\n",
			err);
		mtk_fcie_unlock_host((u8 *)__func__, mtk_fcie_host);
		err = mtk_fcie_read_block(mtk_fcie_host->mmc,
			sect_buf, EMMC_HS400EXTTABLE_BLK_1);
		mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);
		if (err != EMMC_ST_SUCCESS) {
			EMMC_DBG(mtk_fcie_host,
				EMMC_DBG_LVL_ERROR, 1,
				"eMMC Warn: load 2nd Gen_TTables fail, %Xh\n",
				err);
			emmc_drv->t_table_g.set_cnt = 0;
			emmc_drv->t_table_g.chk_sum = 0;
			emmc_drv->t_table_g.ver_no = 0;
			goto end;
		}
	}
	memcpy((u8 *)&emmc_drv->t_table_g,
		sect_buf,
		sizeof(emmc_drv->t_table_g));

	chk_sum = mtk_fcie_chk_sum(
			(u8 *)&emmc_drv->t_table_g.ver_no,
			(sizeof(emmc_drv->t_table_g)
			- sizeof(u32)/*checksum*/));
	if (chk_sum !=
		emmc_drv->t_table_g.chk_sum) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: ChkSum error, no Gen_TTable\n");
		emmc_drv->t_table_g.set_cnt = 0;
		emmc_drv->t_table_g.chk_sum = 0;
		emmc_drv->t_table_g.ver_no = 0;
		err = EMMC_ST_ERR_CHKSUM;
		goto end;
	}

	if (chk_sum == 0) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: no Gen_TTable\n");
		emmc_drv->t_table_g.set_cnt = 0;
		emmc_drv->t_table_g.chk_sum = 0;
		emmc_drv->t_table_g.ver_no = 0;
		err = EMMC_ST_ERR_CHKSUM;
		goto end;
	}
end:
	kfree(sect_buf);
	return err;
}

/* fcie internal use to load timing table by different speed mode
 */
static u32 mtk_fcie_load_timing_table(struct mtk_fcie_host *mtk_fcie_host,
	u8 pad_type)
{
	u32 err;
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;

	// --------------------------------------
	err = _mtk_fcie_load_timing_table(mtk_fcie_host,
		pad_type, (u8 *)&emmc_drv->t_table);
	if (err != EMMC_ST_SUCCESS)
		goto no_table;

	// --------------------------------------
	//read general table if hs400
	if (pad_type == FCIE_EMMC_HS400_5_1 ||
		pad_type == FCIE_EMMC_HS400) {
		err = mtk_fcie_load_timing_table_g(mtk_fcie_host);
		if (err != EMMC_ST_SUCCESS)
			goto no_table;
	}

	return EMMC_ST_SUCCESS;

no_table:

	emmc_drv->t_table.set_cnt = 0;

	emmc_drv->t_table_g.set_cnt = 0;
	emmc_drv->t_table_g.chk_sum = 0;
	emmc_drv->t_table_g.ver_no = 0;

	EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
		"eMMC Err: load Tables fail, %Xh\n", err);
	return err;

}


/******************************************************************************
 * Functions
 ******************************************************************************/

static int mtk_fcie_get_dma_dir(struct mmc_data *data)
{
	if (data->flags & MMC_DATA_WRITE)
		return DMA_TO_DEVICE;
	else
		return DMA_FROM_DEVICE;
}

/* fcie internal use to decode extend CSD from MMC
 */

static int mtk_fcie_config_ecsd(struct mmc_data *data,
	struct mmc_host *mmc_host)
{
	struct mtk_fcie_host *mtk_fcie_host;
	struct scatterlist *sg = 0;
	struct fcie_emmc_driver *emmc_drv;
	dma_addr_t dmaaddr = 0;
	int err = 0;
	u8 *buf;

	if (!data)
		return -EINVAL;

	mtk_fcie_host = mmc_priv(mmc_host);
	emmc_drv = mtk_fcie_host->emmc_drv;

	if (0 == (emmc_drv->drv_flag & DRV_FLAG_INIT_DONE)) {
		sg = &data->sg[0];
		dmaaddr = sg_dma_address(sg);

		buf = (u8 *)phys_to_virt(dmaaddr);

		if (buf[177] != 0x2 &&
			(buf[179] >> 3) != 0x1 &&
			(buf[179] >> 3) != 0x2) {
			EMMC_DBG(mtk_fcie_host,
				EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: EXCSD[177]=%Xh, EXCSD[179]=%Xh\n",
				buf[177], buf[179]);
			//EMMC_DIE("\n");
		}

		//--------------------------------
		if (emmc_drv->sec_count == 0) {
			emmc_drv->sec_count =
				((buf[215] << 24)|(buf[214] << 16)|
				(buf[213] << 8)|(buf[212]));
			/*-8: Toshiba CMD18 access the
			 *last block report out of range error
			 */
		}

		//-------------------------------
		if (emmc_drv->boot_sec_count == 0)
			emmc_drv->boot_sec_count = buf[226] * 128 * 2;

		//--------------------------------
		if (!emmc_drv->bus_width) {
			emmc_drv->bus_width = buf[183];

			switch (emmc_drv->bus_width) {
			case 0:
				emmc_drv->bus_width = BIT_SD_DATA_WIDTH_1;
				break;
			case 1:
				emmc_drv->bus_width = BIT_SD_DATA_WIDTH_4;
				break;
			case 2:
				emmc_drv->bus_width = BIT_SD_DATA_WIDTH_8;
				break;
			default:
				EMMC_DBG(mtk_fcie_host, 0, 1,
					"eMMC Err: eMMC BUS_WIDTH not support\n");
				while (1)
					;
			}
		}
		//--------------------------------
		if (buf[166] & BIT(2)) // Reliable Write
			emmc_drv->reliable_wblk_cnt = BIT_SD_JOB_BLK_CNT_MASK;
		else {
			if ((buf[503] & BIT(0)) && 1 == buf[222])
				emmc_drv->reliable_wblk_cnt = 1;
			else if (0 == (buf[503] & BIT(0)))
				emmc_drv->reliable_wblk_cnt = buf[222];
			else
				 // can not support Reliable Write
				emmc_drv->reliable_wblk_cnt = 0;
		}

		//--------------------------------
		emmc_drv->erased_mem_content = buf[181];

		//--------------------------------
		emmc_drv->ecsd184_stroe_support = buf[184];
		emmc_drv->ecsd185_hstiming = buf[185];
		emmc_drv->ecsd192_ver = buf[192];
		emmc_drv->ecsd196_devtype = buf[196];
		emmc_drv->ecsd197_driver_strength = buf[197];
		emmc_drv->ecsd248_cmd6to = buf[248];
		emmc_drv->ecsd247_pwrofflongto = buf[247];
		emmc_drv->ecsd34_pwroffctrl = buf[34];

		//for GP Partition
		emmc_drv->ecsd160_partsupfield = buf[160];
		emmc_drv->ecsd224_hcerasegrpsize = buf[224];
		emmc_drv->ecsd221_hcwpgrpsize = buf[221];

		//for Max Enhance Size
		emmc_drv->ecsd157_maxenhsize_0 = buf[157];
		emmc_drv->ecsd158_maxenhsize_1 = buf[158];
		emmc_drv->ecsd159_maxenhsize_2 = buf[159];

		emmc_drv->ecsd155_partsetcomplete = buf[155];
		emmc_drv->ecsd166_wrrelparam = buf[166];
		emmc_drv->bootbus_condition = buf[177];
		emmc_drv->partition_config = buf[179];

		emmc_drv->drv_flag |= DRV_FLAG_INIT_DONE;
	}

	return err;
}

/* prepare fcie dma structure by paring scatter-list structure
 */

static void mtk_fcie_dma_setup(struct mtk_fcie_host *mtk_fcie_host)
{
	struct mmc_command *cmd = NULL;
	struct mmc_data  *data	= NULL;
	struct scatterlist	*sg = NULL;
	int i;
	u32 dmalen = 0;
	dma_addr_t dmaaddr = 0;

	if (!mtk_fcie_host)
		return;

	cmd = mtk_fcie_host->cmd;

	if (!cmd)
		return;

	data = cmd->data;
	if (!data)
		return;

	sg = data->sg;
	if (!sg)
		return;

	if (data->blksz < EMMC_SECTOR_512BYTE)
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, BLK_SIZE), data->blksz);
	else
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, BLK_SIZE), EMMC_SECTOR_512BYTE);

	if (data->sg_len > 1) {
		for (i = 0; i < data->sg_len; i++) {
			dmaaddr = sg_dma_address(sg);
			dmalen = sg_dma_len(sg);
			dmaaddr -= (dma_addr_t)mtk_fcie_host->miu0_base;
			mtk_fcie_host->adma_desc[i].miu_sel = 0;

			mtk_fcie_host->adma_desc[i].address = (u32)dmaaddr;
			if (sizeof(dma_addr_t) == DMA_ADDR_64_BYTE) {
				mtk_fcie_host->adma_desc[i].address2 =
				(u32)(dmaaddr >> DMA_47_32_SHIFT);
			}
			mtk_fcie_host->adma_desc[i].dma_len = dmalen;
			mtk_fcie_host->adma_desc[i].job_cnt = (dmalen/data->blksz);
			mtk_fcie_host->adma_desc[i].end = 0;

			sg = sg_next(sg);
		}

		mtk_fcie_host->adma_desc[data->sg_len-1].end = 1;
		/* Make sure descriptors are ready before start command */
		wmb();
		dmaaddr = mtk_fcie_host->adma_desc_addr;
		dmaaddr -= (dma_addr_t)mtk_fcie_host->miu0_base;

		REG_W(REG_ADDR(mtk_fcie_host->fciebase, JOB_BL_CNT), 1);
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_ADDR_15_0),
			dmaaddr & DMA_15_0_MASK);
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_ADDR_31_16),
			(dmaaddr >> DMA_31_16_SHIFT) & DMA_15_0_MASK);
		if (sizeof(dma_addr_t) == DMA_ADDR_64_BYTE)
			REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_ADDR_47_32),
			dmaaddr >> DMA_47_32_SHIFT);

		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_LEN_15_0),
			ADMA_TABLE_DMA_LEN_15_0);
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_LEN_31_16),
			ADMA_TABLE_DMA_LEN_31_16);
	} else {

		dmaaddr = sg_dma_address(sg);
		dmalen = sg_dma_len(sg);
		dmaaddr -= (dma_addr_t)mtk_fcie_host->miu0_base;

		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_LEN_15_0),
			dmalen & DMA_15_0_MASK);
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_LEN_31_16),
			dmalen >> DMA_31_16_SHIFT);
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, JOB_BL_CNT),
			dmalen>>EMMC_SECTOR_512BYTE_BITS);

		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_ADDR_15_0),
			dmaaddr & DMA_15_0_MASK);
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_ADDR_31_16),
			(dmaaddr >> DMA_31_16_SHIFT) & DMA_15_0_MASK);

		if (sizeof(dma_addr_t) == DMA_ADDR_64_BYTE)
			REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_ADDR_47_32),
			(u64)dmaaddr >> DMA_47_32_SHIFT);
	}
}


/* Wait eMMC busy within us
 */

#define WAIT_D0H_POLLING_TIME	HW_TIMER_DELAY_100US
static u32 mtk_fcie_wait_d0_high(struct mtk_fcie_host *mtk_fcie_host,
	u32 us)
{
	u32 cnt, wait;
	ktime_t expires;

	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE),
		BIT_SD_CLK_EN);

	if ((REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS))
		&BIT_SD_CARD_BUSY) == 0)
		return EMMC_ST_SUCCESS;


	for (cnt = 0; cnt < us; cnt += WAIT_D0H_POLLING_TIME) {
		wait = _mtk_fcie_wait_d0_high(WAIT_D0H_POLLING_TIME,
			mtk_fcie_host);

		if (wait < WAIT_D0H_POLLING_TIME)
			return EMMC_ST_SUCCESS;

		expires = ktime_add_ns(ktime_get(), WAIT_D0_HIGH_EXPIRES);
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_hrtimeout(&expires, HRTIMER_MODE_ABS);

		cnt += HW_TIMER_DELAY_1MS;
	}

	return EMMC_ST_ERR_TIMEOUT_WAITD0HIGH;
}


/* check boot mode configure from mmc commands to
 * avoid corrupt boot mode setting
 */

static void mtk_check_bootmode_config(struct mtk_fcie_host *mtk_fcie_host,
	struct mmc_command *cmd)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u8 patition_config = emmc_drv->partition_config;
	u8 bootbus = emmc_drv->bootbus_condition;
	u8 bootconfig = (emmc_drv->partition_config >> 3) & 0x7;

	if (cmd->opcode  == 0x6) {
		if (((cmd->arg & 0xff0000) >> 16) == 0xb3) {
			switch ((cmd->arg >> 24) & 3) {
			case EMMC_EXTCSD_SETBIT:
				patition_config |= (cmd->arg&0xff00)>>8;
				if ((patition_config >> 3) != 0x1
					&& (patition_config >> 3) != 0x2) {
					//Clear ecsd[179] BIT(6)~BIT(3)
					cmd->arg &= ~(cmd->arg&0x7800);
					//ECSD set boot part original for boot
					cmd->arg |= bootconfig << 11;
					EMMC_DBG(mtk_fcie_host,
						EMMC_DBG_LVL_ERROR, 1,
						"eMMC Err: mmc switch to wrong ecsd[179]= %Xh\n",
						patition_config);
				}
				emmc_drv->partition_config |=
					(cmd->arg&0xFF00)>>8;
				break;
			case EMMC_EXTCSD_CLRBIT:
				patition_config &= ~((cmd->arg & 0xff00) >> 8);
				if ((patition_config >> 3) != 0x1
					&& (patition_config >> 3) != 0x2) {
					//Do not set ecsd[179] BIT(6)~BIT(3)
					cmd->arg &= ~(cmd->arg & 0x7800);
					EMMC_DBG(mtk_fcie_host,
						EMMC_DBG_LVL_ERROR, 1,
						"eMMC Err: mmc switch to wrong ecsd[179]= %Xh\n",
						patition_config);
				}
				emmc_drv->partition_config &=
					~((cmd->arg&0xFF00)>>8);
				break;
			case EMMC_EXTCSD_WBYTE:
				patition_config = (cmd->arg & 0xff00)>>8;
				if ((patition_config >> 3) != 0x1
					&& (patition_config >> 3) != 0x2) {
					//Clear ecsd[179] BIT(6)~BIT(3)
					cmd->arg &= ~(cmd->arg & 0x7800);
					//ECSD set boot part original for boot
					cmd->arg |= bootconfig << 11;
					EMMC_DBG(mtk_fcie_host,
						EMMC_DBG_LVL_ERROR, 1,
						"eMMC Err: mmc switch to wrong ecsd[179]= %Xh\n",
						patition_config);
				}
				emmc_drv->partition_config =
					(cmd->arg&0xFF00)>>8;
				break;
			}
		} else if (((cmd->arg & 0xff0000) >> 16) == 0xb1) {
			switch ((cmd->arg >> 24) & 3) {
			case EMMC_EXTCSD_SETBIT:
				bootbus |= (cmd->arg & 0xff00) >> 8;
				if (bootbus != 0x2) {
					//Clear ecsd[177]  BIT(7)~BIT(0)
					cmd->arg &= ~(cmd->arg & 0xff00);
					//Set boot bus condition is equal 2
					cmd->arg |= 0x200;
					EMMC_DBG(mtk_fcie_host,
						EMMC_DBG_LVL_ERROR, 1,
						"eMMC Err: mmc switch to wrong ecsd[177]= %Xh\n",
						bootbus);
				}
				break;
			case EMMC_EXTCSD_CLRBIT:
				bootbus &= ~((cmd->arg & 0xff00) >> 8);
				if (bootbus != 0x2) {
					//Do not set ecsd[177]
					cmd->arg &= ~(cmd->arg & 0xff00);
					EMMC_DBG(mtk_fcie_host,
						EMMC_DBG_LVL_ERROR, 1,
						"eMMC Err: mmc switch to wrong ecsd[177]= %Xh\n",
						bootbus);
				}
				break;
			case EMMC_EXTCSD_WBYTE:
				bootbus = (cmd->arg & 0xff00) >> 8;
				if (bootbus != 0x2) {
					//Clear ecsd[177]  BIT(7)~BIT(0)
					cmd->arg &= ~(cmd->arg & 0xff00);
					//Set boot bus condition is equal 2
					cmd->arg |= 0x200;
					EMMC_DBG(mtk_fcie_host,
						EMMC_DBG_LVL_ERROR, 1,
						"eMMC Err: mmc switch to wrong ecsd[177]= %Xh\n",
						bootbus);
				}
				break;
			}
		} else if ((cmd->arg & 0xff0000) == 0x210000) {
			switch ((cmd->arg>>24)&3) {
			case EMMC_EXTCSD_SETBIT:
				emmc_drv->cache_ctrl |=
					(cmd->arg&0xFF00)>>8;
				break;
			case EMMC_EXTCSD_CLRBIT:
				emmc_drv->cache_ctrl &=
					~((cmd->arg&0xFF00)>>8);
				break;
			case EMMC_EXTCSD_WBYTE:
				emmc_drv->cache_ctrl =
					(cmd->arg&0xFF00)>>8;
				break;
			}
		}  else if ((cmd->arg & 0xff0000) == 0xF0000) {
			switch ((cmd->arg>>24)&3) {
			case EMMC_EXTCSD_SETBIT:
				emmc_drv->cmdq_enable |=
					(cmd->arg&0xFF00)>>8;
				break;
			case EMMC_EXTCSD_CLRBIT:
				emmc_drv->cmdq_enable &=
					~((cmd->arg&0xFF00)>>8);
				break;
			case EMMC_EXTCSD_WBYTE:
				emmc_drv->cmdq_enable =
					(cmd->arg&0xFF00)>>8;
				break;
			}
		}
	}
}


/* pre_dma for mmc layer to handle cache operation
 */

static int mtk_fcie_pre_dma_transfer(struct mtk_fcie_host *host,
	struct mmc_data *data, struct mtk_fcie_host_next *next)
{
	int dma_len;

	/* Check if next job is already prepared */
	if (next || (!next && data->host_cookie == 0)) {
		dma_len =
			dma_map_sg(mmc_dev(host->mmc),
					data->sg,
					 data->sg_len,
					 mtk_fcie_get_dma_dir(data));
	} else {
		dma_len = host->next_data.dma_len;
		host->next_data.dma_len = 0;
	}

	if (dma_len == 0)
		return -EINVAL;

	if (next) {
		next->dma_len = dma_len;

		if (++next->cookie < 0)
			next->cookie = 1;
		data->host_cookie = next->cookie;
	}

	return 0;
}

static void mtk_fcie_request_done(struct mtk_fcie_host *mtk_fcie_host)
{
	struct mmc_data *data = NULL;
	int ret = 0;
	int sbc_error = 0;

	if (!mtk_fcie_host)
		return;

	data = mtk_fcie_host->request->data;

	if (mtk_fcie_host->request->sbc)
		sbc_error = mtk_fcie_host->request->sbc->error;

	if (!sbc_error) {
		ret = cancel_delayed_work(&mtk_fcie_host->req_timeout_work);
		if (!ret) {
			/* delay work already running */
			return;
		}
	}
	mtk_fde_func_dis(mtk_fcie_host);


	if (data) {
		if (!data->host_cookie)
			dma_unmap_sg(mmc_dev(mtk_fcie_host->mmc), data->sg,
				(int)data->sg_len, mtk_fcie_get_dma_dir(data));

		if (mtk_fcie_host->request->cmd->opcode == MMC_SEND_EXT_CSD && !data->error)
			mtk_fcie_config_ecsd(data, mtk_fcie_host->mmc);

		if (data->flags & MMC_DATA_READ) {
			REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), BIT_SD_CLK_EN);
		}

	} else {
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), BIT_SD_CLK_EN);
	}

	if (mtk_fcie_host->error) {
/*
 * Ioctl fuzzer send illegal command without data to eMMC.
 */
		if ((mtk_fcie_host->error & REQ_CMD_NORSP) &&
			(mtk_fcie_host->emmc_drv->drv_flag & DRV_FLAG_INIT_DONE)) {
			schedule_work(&mtk_fcie_host->emmc_recovery_work);
			return;
		}
		ret = _mtk_fcie_reset(mtk_fcie_host);
		if (ret) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 0,
				"eMMC Err: Reset FCIE Fail\n");
		}
	}

	mmc_request_done(mtk_fcie_host->mmc, mtk_fcie_host->request);
}

static void mtk_fcie_completed_cmd(struct mtk_fcie_host *mtk_fcie_host,
	struct mmc_command *cmd,
	u16 events)
{
	u32 fifo[CMD_FIFO_SIZE];
	u16 status;
	struct fcie_emmc_driver *emmc_drv = NULL;

	if (!mtk_fcie_host)
		return;

	emmc_drv = mtk_fcie_host->emmc_drv;

	if (cmd->flags & MMC_RSP_PRESENT) {

		if (cmd == mtk_fcie_host->request->sbc)
			cmd->resp[RSP0_INDEX] = CMD_R1_RESPONSE;
		else {
			fifo[CMD_FIFO_INDEX_0] = REG(REG_ADDR(mtk_fcie_host->fciebase, CMD_FIFO));
			fifo[CMD_FIFO_INDEX_1] = REG(REG_ADDR(mtk_fcie_host->fciebase,
				CMD_FIFO+CMD_FIFO_INDEX_1));
			fifo[CMD_FIFO_INDEX_2] = REG(REG_ADDR(mtk_fcie_host->fciebase,
				CMD_FIFO+CMD_FIFO_INDEX_2));

		    cmd->resp[RSP0_INDEX] = ((fifo[CMD_FIFO_INDEX_0] & RSPH_MASK)<<RSP23_16_SHIFT) |
			    ((fifo[CMD_FIFO_INDEX_1] & RSPL_MASK)<<RSP23_16_SHIFT) |
			    (fifo[CMD_FIFO_INDEX_1] & RSPH_MASK) |
			    (fifo[CMD_FIFO_INDEX_2] & RSPL_MASK);
		}

		if (cmd->flags & MMC_RSP_136) {

			fifo[CMD_FIFO_INDEX_3] = REG(REG_ADDR(mtk_fcie_host->fciebase,
				CMD_FIFO+CMD_FIFO_INDEX_3));
			fifo[CMD_FIFO_INDEX_4] = REG(REG_ADDR(mtk_fcie_host->fciebase,
				CMD_FIFO+CMD_FIFO_INDEX_4));
			fifo[CMD_FIFO_INDEX_5] = REG(REG_ADDR(mtk_fcie_host->fciebase,
				CMD_FIFO+CMD_FIFO_INDEX_5));
			fifo[CMD_FIFO_INDEX_6] = REG(REG_ADDR(mtk_fcie_host->fciebase,
				CMD_FIFO+CMD_FIFO_INDEX_6));
			fifo[CMD_FIFO_INDEX_7] = REG(REG_ADDR(mtk_fcie_host->fciebase,
				CMD_FIFO+CMD_FIFO_INDEX_7));

			cmd->resp[RSP1_INDEX] =
				((fifo[CMD_FIFO_INDEX_2]>>RSP_15_8_SHIFT)<<RSP_31_24_SHIFT) |
				((fifo[CMD_FIFO_INDEX_3] & RSPL_MASK)<<RSP23_16_SHIFT) |
				((fifo[CMD_FIFO_INDEX_3]>>RSP_15_8_SHIFT)<<RSP_15_8_SHIFT) |
				(fifo[CMD_FIFO_INDEX_4] & RSPL_MASK);
			cmd->resp[RSP2_INDEX] =
				((fifo[CMD_FIFO_INDEX_4]>>RSP_15_8_SHIFT)<<RSP_31_24_SHIFT) |
				((fifo[CMD_FIFO_INDEX_5] & RSPL_MASK)<<RSP23_16_SHIFT) |
				((fifo[CMD_FIFO_INDEX_5]>>RSP_15_8_SHIFT)<<RSP_15_8_SHIFT) |
				(fifo[CMD_FIFO_INDEX_6] & RSPL_MASK);
			cmd->resp[RSP3_INDEX] =
				((fifo[CMD_FIFO_INDEX_6]>>RSP_15_8_SHIFT)<<RSP_31_24_SHIFT) |
				((fifo[CMD_FIFO_INDEX_7] & RSPL_MASK)<<RSP23_16_SHIFT) |
				((fifo[CMD_FIFO_INDEX_7]>>RSP_15_8_SHIFT)<<RSP_15_8_SHIFT);
		}
	}

	// ----------------------------------
	if (events & BIT_ERR_STS) {
		status = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: ST:%Xh, EVT:%Xh, CMD%u(%xh) RT:%u\n",
			status, events, cmd->opcode, cmd->arg, cmd->retries);
		if (status & BIT_SD_RSP_TIMEOUT) {
			cmd->error = -ETIMEDOUT;
			mtk_fcie_host->error |= REQ_CMD_NORSP;
		} else if (status & BIT_SD_RSP_CRC_ERR) {
			cmd->error = -EILSEQ;
			mtk_fcie_host->error |= REQ_CMD_EIO;
			EMMC_DBG(mtk_fcie_host,
				0, 1, "resp[0]: %08Xh\n",
				cmd->resp[0]);
		} else {
			cmd->error = 0;
		}
	} else if (events & BIT_INT_CMDTMO) {
		cmd->error = -ETIMEDOUT;
		mtk_fcie_host->error |= REQ_CMD_TMO;
	} else {
		if (!(mmc_resp_type(cmd) & MMC_RSP_CRC)) {
			status = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));

			if (status & BIT_SD_RSP_CRC_ERR) {
				cmd->error = 0;
				if ((cmd->opcode == MMC_SEND_OP_COND) &&
					((cmd->resp[0]>>CMD1_BUSY_SHIFT) & BIT(0))) {
					emmc_drv->if_sector_mode =
						(cmd->resp[0] >> CMD1_SECTOR_MODE_SHIFT) & BIT(0);
				}
			} else if (status & BIT_SD_RSP_TIMEOUT) {
				EMMC_DBG(mtk_fcie_host,
					EMMC_DBG_LVL_ERROR, 1,
					"eMMC Warn: ST:%Xh, EVT:%Xh, CMD%u(%xh) RT:%u\n",
					status, events, cmd->opcode, cmd->arg, cmd->retries);

				cmd->error = -ETIMEDOUT;
				mtk_fcie_host->error |= REQ_CMD_NORSP;
			} else {
				cmd->error = 0;
			}
		} else {
			cmd->error = 0;
		}
	}
	if (mmc_resp_type(cmd) == MMC_RSP_R1 ||
		mmc_resp_type(cmd) == MMC_RSP_R1B) {
		if (cmd->resp[0] & EMMC_ERR_R1_31_0) {
			EMMC_DBG(mtk_fcie_host, 0, 0,
				"eMMC Warn: CMD%u R1 error: %Xh, arg %08x\n",
				cmd->opcode, cmd->resp[0], cmd->arg);
		}
	}
}


static void mtk_fcie_data_xfer_next(struct mtk_fcie_host *mtk_fcie_host,
				struct mmc_request *mrq, struct mmc_data *data)
{
	if (mmc_op_multi(mrq->cmd->opcode) && mrq->stop && !mrq->stop->error &&
		!mrq->sbc)
		mtk_fcie_start_cmd(mtk_fcie_host, mrq->stop);
	else
		mtk_fcie_request_done(mtk_fcie_host);
}

static void mtk_fcie_data_xfer_done(struct mtk_fcie_host *mtk_fcie_host, u16 events,
				struct mmc_data *data)
{
	u16 status;
	struct mmc_command *cmd = NULL;
	unsigned long flags;
	bool done = false;


	if (!mtk_fcie_host)
		return;

	cmd = mtk_fcie_host->request->cmd;

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	done = !mtk_fcie_host->data;
	mtk_fcie_host->data = NULL;
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

	if (done)
		return;

	mtk_fcie_host->reg_int_en &= (~(BIT_DMA_END | BIT_ERR_STS));
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), mtk_fcie_host->reg_int_en);



	if (events & BIT_ERR_STS) {
		status = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));

		// should be trivial
		if (status & (BIT_DAT_WR_TOUT|BIT_DAT_RD_TOUT)) {
			data->error = -ETIMEDOUT;
			mtk_fcie_host->error |= REQ_DAT_EIO;
		} else {
			data->error = -EILSEQ;
			mtk_fcie_host->error |= REQ_DAT_EILSEQ;
		}

		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 0,
			"eMMC Err: ST:%Xh, CMD:%u\n",
			status, cmd->opcode);

	} else if (events & BIT_INT_DATATMO) {
		data->error = -ETIMEDOUT;
		mtk_fcie_host->error |= REQ_DATA_TMO;
	} else {
		data->error = 0;
	}

	if (data->error)
		goto dma_end;

	data->bytes_xfered = data->blocks * data->blksz;

	if (data->flags & MMC_DATA_READ) {
		if (mtk_fcie_host->emmc_read_log_enable) {
			EMMC_DBG(mtk_fcie_host,
				0, 0, "\n");
			if (cmd->opcode == MMC_READ_SINGLE_BLOCK)
				EMMC_DBG(mtk_fcie_host,
					0, 0, "cmd:%u ,arg:%xh\n",
					cmd->opcode, cmd->arg);
			else if (cmd->opcode == MMC_READ_MULTIPLE_BLOCK)
				EMMC_DBG(mtk_fcie_host,
					0, 0, "cmd:%u ,arg:%xh, blk cnt:%xh\n",
					cmd->opcode, cmd->arg, data->blocks);
		}
	} else if (data->flags & MMC_DATA_WRITE) {
		if (mtk_fcie_host->emmc_write_log_enable) {
			EMMC_DBG(mtk_fcie_host,
				0, 0, "\n");
			if (cmd->opcode == MMC_WRITE_BLOCK)
				EMMC_DBG(mtk_fcie_host,
					0, 0, "cmd:%u, arg:%xh\n",
					cmd->opcode, cmd->arg);
			else if (cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)
				EMMC_DBG(mtk_fcie_host,
					0, 0, "cmd:%u, arg:%xh, blk cnt:%xh\n",
				cmd->opcode, cmd->arg, data->blocks);
		}
	}

	emmc_record_wr_time(mtk_fcie_host,
		(u8)cmd->opcode,
		data->blocks*EMMC_SECTOR_512BYTE);


dma_end:



	mtk_fcie_data_xfer_next(mtk_fcie_host, mtk_fcie_host->request, data);

}

static void mtk_fcie_prepare_fde(struct mtk_fcie_host *mtk_fcie_host,
	struct mmc_command *cmd,
	struct mmc_data *data)
{
	if (!mtk_fcie_host)
		return;
	if (!cmd)
		return;

	if (!data)
		return;

	if ((mtk_fcie_host->emmc_drv->partition_config & PARTITION_CONFIG_MASK) == 0) {
		if ((cmd->opcode == MMC_READ_SINGLE_BLOCK) ||
				(cmd->opcode == MMC_READ_MULTIPLE_BLOCK) ||
				(cmd->opcode == MMC_WRITE_BLOCK) ||
				(cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) {
			if (data->flags & MMC_DATA_READ)
				mtk_fde_func_en(mtk_fcie_host,
					ENABLE_DEC, cmd->arg);
			else
				mtk_fde_func_en(mtk_fcie_host,
					ENABLE_ENC, cmd->arg);
		}
	}
}


/* send command to eMMC of request from mmc layer
 */
static void mtk_fcie_prepare_raw_cmd(struct mtk_fcie_host *mtk_fcie_host,
	struct mmc_command *cmd)
{
	struct mmc_host *mmc_host = mtk_fcie_host->mmc;
	u16 raw_cmd_opcode = (cmd->opcode & CMD_OPCODE_MASK);

	if (!mmc_host)
		return;

	mtk_check_bootmode_config(mtk_fcie_host, cmd);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS),
		BIT_SD_FCIE_ERR_FLAGS);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT),
		BIT_ALL_CARD_INT_EVENTS);


	REG_W(REG_ADDR(REG_ADDR(mtk_fcie_host->fciebase, CMD_FIFO), CMD_FIFO_INDEX_0),
		(((cmd->arg >> CMD_ARG_31_24_SHIFT)<<CMD_ARG_15_8_SHIFT) |
		(CMD_ARG_OPCODE|raw_cmd_opcode)));
	REG_W(REG_ADDR(REG_ADDR(mtk_fcie_host->fciebase, CMD_FIFO), CMD_FIFO_INDEX_1),
		((cmd->arg & CMD_ARGH_MASK) |
		((cmd->arg>>CMD_ARG_23_16_SHIFT)&CMD_ARGL_MASK)));
	REG_W(REG_ADDR(REG_ADDR(mtk_fcie_host->fciebase, CMD_FIFO), CMD_FIFO_INDEX_2),
		(cmd->arg & CMD_ARGL_MASK));
}
static void mtk_fcie_start_cmd(struct mtk_fcie_host *mtk_fcie_host,
	struct mmc_command *cmd)
{
	struct fcie_emmc_driver	*emmc_drv = NULL;
	struct mmc_data *data = NULL;
	unsigned long flags;
	u32 sd_ctl = BIT_SD_CMD_EN;
	u32 sd_mode = 0;

	if (!mtk_fcie_host)
		return;

	emmc_drv = mtk_fcie_host->emmc_drv;

	sd_mode = emmc_drv->reg10_mode;
	mtk_fcie_host->cmd = cmd;
	data = cmd->data;

	if (data) {
		data->bytes_xfered = 0;
		mtk_fcie_pre_dma_transfer(mtk_fcie_host, data, NULL);
		mtk_fcie_dma_setup(mtk_fcie_host);

		if (data->flags & MMC_DATA_READ) {
			if (data->sg_len > 1)
				sd_ctl |= (BIT_ADMA_EN|BIT_SD_DAT_EN);
			else
				sd_ctl |= BIT_SD_DAT_EN;
		}

		if (data->flags & MMC_DATA_WRITE)
			sd_mode |= BIT_DIS_WR_BUSY_CHK;
	}


	mtk_fcie_prepare_raw_cmd(mtk_fcie_host, cmd);


	if (cmd->flags & MMC_RSP_PRESENT) {
		sd_ctl |= BIT_SD_RSP_EN;

		if (mmc_resp_type(cmd) & MMC_RSP_CRC)
			sd_ctl |= BIT_ERR_DET_ON;

		if (cmd->flags & MMC_RSP_136) {
			sd_ctl |= BIT_SD_RSPR2_EN;
			REG_W(REG_ADDR(mtk_fcie_host->fciebase, CMD_RSP_SIZE),
				(CMD_SIZE_INIT | EMMC_R2_BYTE_CNT));
		} else {
			REG_W(REG_ADDR(mtk_fcie_host->fciebase, CMD_RSP_SIZE),
				(CMD_SIZE_INIT | EMMC_R1_BYTE_CNT));
		}
	} else {
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, CMD_RSP_SIZE), CMD_SIZE_INIT);
	}

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	mtk_fcie_host->reg_int_en = BIT_CMD_END;
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), mtk_fcie_host->reg_int_en);
	if (mmc_resp_type(cmd) & MMC_RSP_BUSY)
		mtk_fcie_host->cmd_resp_busy = true;
	else
		mtk_fcie_host->cmd_resp_busy = false;

	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);


	EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_LOW, 0,
	"cmd:%u, arg:%Xh, buf:%lXh, block:%u, ST:%Xh, mode:%Xh, ctl:%Xh\n",
	cmd->opcode, cmd->arg, (unsigned long)data,
	data ? data->blocks : 0,
	REG((REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS))),
	sd_mode, sd_ctl);

	if (mtk_fcie_host->emmc_monitor_enable) {
		if ((cmd->opcode == MMC_READ_SINGLE_BLOCK) ||
			(cmd->opcode == MMC_READ_MULTIPLE_BLOCK) ||
			(cmd->opcode == MMC_WRITE_BLOCK) ||
			(cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))
			mtk_fcie_host->starttime = ktime_get();
	}
	//user part only
	mtk_fcie_prepare_fde(mtk_fcie_host, cmd, data);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), sd_mode);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL), sd_ctl);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL),
		BIT_JOB_START);
}


static void mtk_fcie_start_data(struct mtk_fcie_host *mtk_fcie_host,
	struct mmc_data *data)
{
	u16 sd_ctl = (BIT_SD_DTRX_EN | BIT_SD_DAT_DIR_W | BIT_ERR_DET_ON);

	mtk_fcie_host->data = data;



	mtk_fcie_host->reg_int_en |= (BIT_DMA_END | BIT_ERR_STS);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), mtk_fcie_host->reg_int_en);

	if (data->flags & MMC_DATA_WRITE) {

		if (data->sg_len > 1)
			sd_ctl |= BIT_ADMA_EN;

		REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL), sd_ctl);
		REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL), BIT_JOB_START);
		emmc_check_life(mtk_fcie_host, data->blocks * data->blksz);
	}
}


static void mtk_fcie_cmd_next(struct mtk_fcie_host *mtk_fcie_host,
		struct mmc_request *mrq, struct mmc_command *cmd)
{
	unsigned long flags;


	if (cmd->error) {
		mtk_fcie_request_done(mtk_fcie_host);
	} else if (cmd == mrq->sbc) {
		mtk_fcie_start_cmd(mtk_fcie_host, mrq->cmd);
		mod_delayed_work(system_wq, &mtk_fcie_host->req_timeout_work, DATA_TIMEOUT);
	} else if (!cmd->data) {
		if (mtk_fcie_host->cmd_resp_busy) {
			spin_lock_irqsave(&mtk_fcie_host->lock, flags);
			mtk_fcie_host->reg_int_en |= BIT_BUSY_END_INT;
			REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN),
				mtk_fcie_host->reg_int_en);
			REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL), BIT_BUSY_DET_ON);
			spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);
		} else  {
			mtk_fcie_request_done(mtk_fcie_host);
		}
	} else {
		mtk_fcie_start_data(mtk_fcie_host, cmd->data);
	}
}


static void mtk_fcie_cmd_done(struct mtk_fcie_host *mtk_fcie_host, u16 events,
				struct mmc_request *mrq,
				struct mmc_command *cmd)
{
	bool done = false;
	unsigned long flags;


	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	done = !mtk_fcie_host->cmd;
	if (!mtk_fcie_host->cmd_resp_busy)
		mtk_fcie_host->cmd = NULL;
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

	if (done)
		return;


	if (events & BIT_BUSY_END_INT) {
		spin_lock_irqsave(&mtk_fcie_host->lock, flags);
		mtk_fcie_host->cmd_resp_busy = false;
		mtk_fcie_host->reg_int_en &= (~(BIT_BUSY_END_INT));
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), mtk_fcie_host->reg_int_en);
		REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL), BIT_BUSY_DET_ON);
		spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

		mtk_fcie_request_done(mtk_fcie_host);
		return;
	}
	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	mtk_fcie_host->reg_int_en &= (~(BIT_CMD_END));
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), mtk_fcie_host->reg_int_en);
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);


	mtk_fcie_completed_cmd(mtk_fcie_host, cmd, events);

	mtk_fcie_cmd_next(mtk_fcie_host, mrq, cmd);
}

#define EMMC_IRQ_DEBUG 0

/* isr to handle and clean fcie events
 * if event is from power lose, pass to irq thread to handle
 * if event is no from fcie mmc driver(fcie may share to control SD)
 * leaves event to fcie sd driver by return IRQ_NONE
 * else handle event with return IRQ_HANDLED
 */

irqreturn_t mtk_fcie_irq(int irq, void *dummy)
{
	u16 events, event_mask;
	struct mtk_fcie_host *mtk_fcie_host = (struct mtk_fcie_host *) dummy;
	struct mmc_host *mmc_host = NULL;
	struct mtk_cqe_host *mtk_cqe_host = NULL;
	struct mmc_request *mrq = NULL;
	struct mmc_command *cmd = NULL;
	struct mmc_data *data = NULL;
	u16 cqe_status = 0;
	unsigned long flags;
	bool cqe_enable = false;

	if (!mtk_fcie_host)
		return IRQ_HANDLED;

	mmc_host = mtk_fcie_host->mmc;
	if (!mmc_host) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 0,
			"eMMC Err: mmc_host addr is Null\n");
		return IRQ_HANDLED;
	}

	mtk_cqe_host = mtk_fcie_host->mmc->cqe_private;
	if (!mtk_cqe_host) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 0,
			"eMMC Err: mtk_cqe_host addr is Null\n");
		return IRQ_HANDLED;
	}

	events = REG(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL));

	if ((events&(BIT_POWER_SAVE_MODE_INT|BIT_POWER_SAVE_MODE_INT_EN))
		== (BIT_POWER_SAVE_MODE_INT|BIT_POWER_SAVE_MODE_INT_EN)) {

		return IRQ_WAKE_THREAD;
	}
	while (true) {

		spin_lock_irqsave(&mtk_fcie_host->lock, flags);

		events = REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT));
		event_mask = REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN));

		if (mmc_host->caps2 & MMC_CAP2_CQE) {
			cqe_status = REG(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS));
			REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS), cqe_status);
			if ((cqe_status & BIT_INT_MASK) ||
				((events & BIT_ERR_STS) &&
				mmc_host->cqe_on)) {
				cqe_enable = true;
			}
		}
		mrq = mtk_fcie_host->request;
		cmd = mtk_fcie_host->cmd;
		data = mtk_fcie_host->data;

		spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

		if (cqe_enable) {
			spin_lock_irqsave(&mtk_fcie_host->lock, flags);
			//Check w1c TCC success or fail
			if ((cqe_status & BIT_INT_TCC) &&
				!mtk_cqe_host->qcnt &&
				mmc_host->cqe_on) {
				if (REG(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS)) &
				    BIT_INT_TCC) {
					#if EMMC_IRQ_DEBUG
					EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
						"W1C TCC fail!!\n");
					#endif
					mtkcqe_wait_cqebus_idle(mtk_fcie_host);
					mtkcqe_irq_err_handle(mtk_fcie_host);
					spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);
					return IRQ_HANDLED;
				}
			}
			spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

			mtkcqe_irq(mtk_fcie_host, events, cqe_status);
			return IRQ_HANDLED;
		}

		if (!(events & event_mask))
			break;

		if (cmd)
			mtk_fcie_cmd_done(mtk_fcie_host, events, mrq, cmd);
		else if (data)
			mtk_fcie_data_xfer_done(mtk_fcie_host, events, data);

	}



	return IRQ_HANDLED;
}


static void mtk_fcie_pre_req(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct mtk_fcie_host *host = mmc_priv(mmc);

	if (mrq->data->host_cookie) {
		mrq->data->host_cookie = 0;
		return;
	}

	if (mtk_fcie_pre_dma_transfer(host, mrq->data, &host->next_data))
		mrq->data->host_cookie = 0;
}

/* prost_dma for mmc layer to handle cache operation
 */

static void mtk_fcie_post_req(struct mmc_host *mmc,
	struct mmc_request *mrq, int err)
{
	struct mtk_fcie_host *host = mmc_priv(mmc);
	struct mmc_data *data = mrq->data;

	if (data->host_cookie) {
		dma_unmap_sg(mmc_dev(host->mmc),
					 data->sg,
					 data->sg_len,
					 mtk_fcie_get_dma_dir(data));
	}

	data->host_cookie = 0;
}


static inline bool mtk_fcie_cmd_is_ready(struct mtk_fcie_host *mtk_fcie_host)
{
	unsigned long tmo = jiffies + msecs_to_jiffies(MAX_BUSY_TIME);

	do {

		if ((REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS))
		& BIT_SD_CARD_BUSY) == 0)
			return true;

		cpu_relax();

	} while (time_before(jiffies, tmo));



	if (REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS))
		& BIT_SD_CARD_BUSY) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: wait D0 H timeout\n");

		return false;
	}

	return true;
}



/* handle request from mmc layer
 */

static void mtk_fcie_request(struct mmc_host *mmc_host,
	struct mmc_request *mrq)
{
	struct mtk_fcie_host *mtk_fcie_host = NULL;
	struct mtk_cqe_host *mtk_cqe_host = NULL;
	unsigned int cmd_timeout = DATA_TIMEOUT;
	struct mmc_command *cmd = NULL;
	unsigned long flags;


	if (!mmc_host)
		return;

	if (!mrq)
		return;

	mtk_fcie_host = mmc_priv(mmc_host);
	mtk_cqe_host  = mmc_host->cqe_private;

	if (!mtk_fcie_host)
		return;

	if (!mtk_cqe_host)
		return;

	mtk_fcie_host->request = mrq;
	mtk_fcie_host->error = 0;

	if (mmc_host->ios.power_mode == MMC_POWER_OFF) {

		if (mtk_fcie_host->request->sbc)
			mtk_fcie_host->request->sbc->error = -ETIMEDOUT;
		else
			mtk_fcie_host->request->cmd->error = -ETIMEDOUT;

		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 0,
			"eMMC Warn: Invalid IO access eMMC during STR process\n");

		mmc_request_done(mtk_fcie_host->mmc, mtk_fcie_host->request);
		return;
	}

	if (mtk_cqe_host->enabled) {
		if (mtk_cqe_host->recovery_halt &&
			mtk_fcie_host->request->cmd->opcode == MMC_STOP_TRANSMISSION) {
			mtk_fcie_host->request->cmd->resp[0] = CMD_R1B_RESPONSE;
			mtk_fcie_host->request->cmd->error = 0;

			mmc_request_done(mtk_fcie_host->mmc, mtk_fcie_host->request);

			return;
		}
	}

	mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);
	if (mtk_cqe_host->enabled) {
		spin_lock_irqsave(&mtk_fcie_host->lock, flags);
		REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS), BIT_INT_MASK);
		spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);
		if (REG(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS)) &
			BIT_INT_MASK) {
			mtkcqe_irq_err_handle(mtk_fcie_host);
		}
	}

	if (mrq->cmd->opcode != MMC_SEND_STATUS) {
		if (!mtk_fcie_cmd_is_ready(mtk_fcie_host)) {
			if (mtk_fcie_host->request->sbc)
				mtk_fcie_host->request->sbc->error = -ETIMEDOUT;
			else
				mtk_fcie_host->request->cmd->error = -ETIMEDOUT;

			mmc_request_done(mtk_fcie_host->mmc, mtk_fcie_host->request);
			return;
		}
	}

	if (mtk_fcie_host->request->sbc)
		mtk_fcie_start_cmd(mtk_fcie_host,
			mtk_fcie_host->request->sbc);
	else {
		cmd = mtk_fcie_host->request->cmd;
		if (mmc_resp_type(cmd) & MMC_RSP_BUSY)
			cmd_timeout = MAX_BUSY_TIME;

		mod_delayed_work(system_wq, &mtk_fcie_host->req_timeout_work, cmd_timeout);
		mtk_fcie_start_cmd(mtk_fcie_host, cmd);
	}
}

/* HW reset interface for mmc layer
 */

static void mtk_fcie_mmc_hw_reset(struct mmc_host *mmc_host)
{
	struct mtk_fcie_host *mtk_fcie_host;
	struct fcie_emmc_driver	*emmc_drv;

	mtk_fcie_host = mmc_priv(mmc_host);
	emmc_drv = mtk_fcie_host->emmc_drv;

	mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);
	emmc_drv->old_pll_clk_param = 0xffff;
	emmc_drv->old_pll_dll_clk_param = 0xffff;
	emmc_drv->tune_overtone = 1;
	_mtk_fcie_mmc_hw_reset(mtk_fcie_host);
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
		BIT_POWER_SAVE_MODE|BIT_POWER_SAVE_MODE_INT_EN);
	mtk_fcie_unlock_host((u8 *)__func__, mtk_fcie_host);
}

static void mtk_fcie_req_timeout(struct work_struct *work)
{
	struct mtk_fcie_host *mtk_fcie_host =
		container_of(work, struct mtk_fcie_host, req_timeout_work.work);
	struct mmc_command *cmd = NULL;
	struct mmc_data *data   = NULL;
	u16 event, int_events;

	if (!mtk_fcie_host)
		return;

	cmd = mtk_fcie_host->cmd;
	data = mtk_fcie_host->data;

	if (cmd) {

		event = REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT));
		int_events = REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN));

		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 0,
					"eMMC Err: aborting cmd: event=%Xh, int_events=%Xh\n",
					event,
					int_events);

		mtk_fcie_cmd_done(mtk_fcie_host, BIT_INT_CMDTMO, mtk_fcie_host->request, cmd);

	} else if (data) {

		event = REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT));
		int_events = REG(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN));

		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 0,
			"eMMC Err: abort data: event=%Xh, int_events=%Xh\n",
			event,
			int_events);

		mtk_fcie_data_xfer_done(mtk_fcie_host, BIT_INT_DATATMO, data);
	}
}
/*
 * Ioctl fuzzer send illegal command without data to eMMC,
 * but kernel did not reinit eMMC. Device may always occur
 * no command response.
 */
static void mtk_fcie_recovery(struct work_struct *work)
{
	struct mtk_fcie_host *mtk_fcie_host =
	    container_of(work, struct mtk_fcie_host, emmc_recovery_work);

	if (!mtk_fcie_host)
		return;

	mtk_fcie_err_handler(mtk_fcie_host);
	mmc_request_done(mtk_fcie_host->mmc, mtk_fcie_host->request);
}


/* return eMMC is busy or not
 */

static int mtk_fcie_card_busy(struct mmc_host *mmc_host)
{
	u16 read0, read1;
	struct mtk_fcie_host *mtk_fcie_host;

	mtk_fcie_host = mmc_priv(mmc_host);
	mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);
	read0 = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));
	mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_1US);
	read1 = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));
	mtk_fcie_unlock_host((u8 *)__func__, mtk_fcie_host);

	if ((read0 & BIT_SD_CARD_BUSY) == 0
		&& (read1 & BIT_SD_CARD_BUSY) == 0)
		return 0;
	else
		return 1;
}

/* return driving strength setting before mmc layer switch eMMC to
 *  HS200/HS400 mode
 */

static int mtk_fcie_select_drive_strength(struct mmc_card *card,
	unsigned int max_dtr, int host_drv, int card_drv, int *drv_type)
{
	struct mmc_host *mmc_host = card->host;
	struct mtk_fcie_host *mtk_fcie_host;

	mtk_fcie_host = mmc_priv(mmc_host);
	return mtk_fcie_host->emmc_drv->t_table_g.device_driving;
}
/* set host state from mmc layer parameter mmc_ios
 *  load timing table before mmc layer switch to HS mode
 */
static void mtk_fcie_set_ios(struct mmc_host *mmc_host, struct mmc_ios *mmc_ios)
{
	/* Define Local Variables */
	struct mtk_fcie_host *mtk_fcie_host;
	struct fcie_emmc_driver *emmc_drv;
	u8 pad_type = 0;
	u8 has_t_table = 0;
	char *timing_str = 0;
	u32 err;


	if (!mmc_host)
		return;

	mtk_fcie_host = mmc_priv(mmc_host);
	emmc_drv = mtk_fcie_host->emmc_drv;



	if (!mmc_ios) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: mmc_ios is NULL\n");
		return;
	}
	// ----------------------------------
	if (mmc_ios->clock == 0) {
		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_LOW, 1,
			"eMMC Warn: disable clk\n");
		mtk_fcie_clock_gating(mtk_fcie_host);
		return;
	}
	if (mmc_ios->bus_width == MMC_BUS_WIDTH_8) {
		emmc_drv->bus_width = BIT_SD_DATA_WIDTH_8;
		emmc_drv->reg10_mode =
			(emmc_drv->reg10_mode & ~BIT_SD_DATA_WIDTH_MASK)
			| BIT_SD_DATA_WIDTH_8;
	} else if (mmc_ios->bus_width == MMC_BUS_WIDTH_4) {
		emmc_drv->bus_width = BIT_SD_DATA_WIDTH_4;
		emmc_drv->reg10_mode =
			(emmc_drv->reg10_mode & ~BIT_SD_DATA_WIDTH_MASK)
			| BIT_SD_DATA_WIDTH_4;
	} else if (mmc_ios->bus_width == MMC_BUS_WIDTH_1) {
		emmc_drv->bus_width = BIT_SD_DATA_WIDTH_1;
		emmc_drv->reg10_mode =
			(emmc_drv->reg10_mode & ~BIT_SD_DATA_WIDTH_MASK);
	}

	if (mtk_fcie_host->host_ios_timing != mmc_ios->timing
		|| mtk_fcie_host->host_ios_clk != mmc_ios->clock) {
		if (mmc_ios->timing == MMC_TIMING_LEGACY) {
			mtk_fcie_pads_switch(FCIE_EMMC_BYPASS,
				mtk_fcie_host);

			if (mmc_ios->clock > CLK_400KHz)
				mtk_fcie_clock_setting(0, mtk_fcie_host,
					12000000);
			else
				mtk_fcie_clock_setting(0, mtk_fcie_host,
					300000);

			mtk_fcie_host->host_ios_timing =
				mmc_ios->timing;
			mtk_fcie_host->host_ios_clk =
				mmc_ios->clock;
		} else if (mmc_ios->timing == MMC_TIMING_MMC_HS) {
			mtk_fcie_pads_switch(FCIE_EMMC_BYPASS,
				mtk_fcie_host);
			mtk_fcie_clock_setting(0, mtk_fcie_host,
				48000000);

			mtk_fcie_host->host_ios_timing =
				mmc_ios->timing;
			mtk_fcie_host->host_ios_clk =
				mmc_ios->clock;

			has_t_table = (emmc_drv->t_table.chk_sum == 0 &&
				emmc_drv->t_table.ver_no == 0) ? 0 : 1;

			if (!has_t_table) {
				if (emmc_drv->ecsd184_stroe_support &&
					(mmc_host->caps2&MMC_CAP2_HS400_ES)
					) {

					EMMC_DBG(mtk_fcie_host,
						0, 1, "Ld HS400ES\n");
					err = mtk_fcie_load_timing_table(
						mtk_fcie_host,
						FCIE_EMMC_HS400_5_1);
					if (err)
						EMMC_DBG(mtk_fcie_host,
						0, 1,
						"eMMC Warn: HS400 no t_tb %Xh\n",
						err);
					else
						return;
				}
				if (emmc_drv->ecsd196_devtype&ECSD_DDR
					&& (mmc_host->caps&(MMC_CAP_1_8V_DDR))
					) {

					err = mtk_fcie_load_timing_table(
						mtk_fcie_host, FCIE_EMMC_DDR);
					if (err)
						EMMC_DBG(mtk_fcie_host,
						0, 1,
						"eMMC Warn: DDR no t_tb %Xh\n",
						err);
					else
						return;
				}
			}
		} else if (mmc_ios->timing == MMC_TIMING_MMC_DDR52) {
			mtk_fcie_pads_switch(FCIE_EMMC_DDR, mtk_fcie_host);
			if (emmc_drv->t_table.set_cnt)
				mtk_fcie_apply_timing_set(EMMC_TIMING_SET_MAX,
					mtk_fcie_host);

			mtk_fcie_host->host_ios_timing =
				mmc_ios->timing;
			mtk_fcie_host->host_ios_clk =
				mmc_ios->clock;

			EMMC_DBG(mtk_fcie_host,
				0, 0, "eMMC: DDR %uMHz\n",
				emmc_drv->clk_khz/1000);
			if (mtk_fcie_host->enable_sar5)
				REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase,
				PWR_SAVE_CTL),
				BIT_POWER_SAVE_MODE|BIT_POWER_SAVE_MODE_INT_EN);
		} else {
			if (mmc_ios->timing == MMC_TIMING_MMC_HS200 &&
				!has_t_table) {
				if (emmc_drv->ecsd196_devtype&ECSD_HS400 &&
					(mmc_host->caps2&MMC_CAP2_HS400_1_8V ||
					mmc_host->caps2&MMC_CAP2_HS400_ES)) {

					err = mtk_fcie_load_timing_table(
						mtk_fcie_host,
						FCIE_EMMC_HS400);
					if (err)
						EMMC_DBG(mtk_fcie_host,
						0, 1,
						"eMMC Warn: HS400 no t_tb %Xh\n",
						err);
					else
						goto load_end;
				}
				if (emmc_drv->ecsd196_devtype&ECSD_HS200 &&
					mmc_host->caps2&MMC_CAP2_HS200_1_8V_SDR
					) {

					err = mtk_fcie_load_timing_table(
						mtk_fcie_host,
						FCIE_EMMC_HS200);
					if (err)
						EMMC_DBG(mtk_fcie_host,
						0, 1,
						"eMMC Warn: HS200 no t_tb %Xh\n",
						err);
					else
						goto load_end;
				}
			}
load_end:
			if (mmc_ios->timing == MMC_TIMING_MMC_HS200 &&
				emmc_drv->ecsd196_devtype&ECSD_HS400 &&
				((mmc_host->caps2&MMC_CAP2_HS400_ES) ||
				(mmc_host->caps2&MMC_CAP2_HS400_1_8V))
				) {
				mtk_fcie_host->host_ios_timing =
					mmc_ios->timing;
				mtk_fcie_host->host_ios_clk =
					mmc_ios->clock;

				return;
			}

			switch (mmc_ios->timing) {
			case MMC_TIMING_MMC_HS200:
				pad_type = FCIE_EMMC_HS200;
				timing_str = "HS200";
				break;
			case MMC_TIMING_MMC_HS400:
				if (emmc_drv->ecsd184_stroe_support &&
					mmc_host->caps2&MMC_CAP2_HS400_ES) {
					pad_type = FCIE_EMMC_HS400_5_1;
					timing_str = "HS400 5.1";
				} else {
					pad_type = FCIE_EMMC_HS400;
					timing_str = "HS400";
				}
				break;
			}

			mtk_fcie_pads_switch(pad_type,
				mtk_fcie_host);
			mtk_fcie_clock_setting(FCIE_DEFAULT_CLK,
				mtk_fcie_host, 0);

			if (emmc_drv->t_table_g.set_cnt &&
				(pad_type == FCIE_EMMC_HS400 ||
				pad_type == FCIE_EMMC_HS400_5_1))
				mtk_fcie_apply_timing_set(
					emmc_drv->t_table_g.curset_idx,
					mtk_fcie_host);
			else if (emmc_drv->t_table.set_cnt)
				mtk_fcie_apply_timing_set(
					EMMC_TIMING_SET_MAX,
					mtk_fcie_host);

			mtk_fcie_host->host_ios_timing =
				mmc_ios->timing;
			mtk_fcie_host->host_ios_clk =
				mmc_ios->clock;

			EMMC_DBG(mtk_fcie_host,
				0, 0, "eMMC: %s %uMHz\n",
				timing_str, emmc_drv->clk_khz/1000);
			if (mtk_fcie_host->enable_sar5)
				REG_SETBIT(REG_ADDR(
					mtk_fcie_host->fciebase,
					PWR_SAVE_CTL),
					BIT_POWER_SAVE_MODE|
					BIT_POWER_SAVE_MODE_INT_EN);
			#if defined(CONFIG_MMC_MTK_ROOT_WAIT) && CONFIG_MMC_MTK_ROOT_WAIT
			if (!(emmc_drv->drv_flag & DRV_FLAG_RESUME))
				complete(&mtk_fcie_host->init_mmc_done);
			#endif
		}
	}




}

//=======================================================================
static void mtk_fcie_enable(struct mtk_fcie_host *mtk_fcie_host)
{
	u32 err;

	err = mtk_fcie_hw_init(mtk_fcie_host);
	if (err)
		EMMC_DBG(mtk_fcie_host,
		0, 1, "eMMC Err: hw init fail %Xh\n", err);
	if (mtk_fcie_host->enable_sar5)
		mtk_fcie_prepare_psm_queue(mtk_fcie_host);
}

static void mtk_fcie_disable(struct mtk_fcie_host *mtk_fcie_host)
{
	u32 err;

	err = _mtk_fcie_reset(mtk_fcie_host);
	if (err)
		EMMC_DBG(mtk_fcie_host,
		0, 1, "eMMC Err: reset fail %Xh\n", err);

	mtk_fcie_clock_gating(mtk_fcie_host);
}

static void mtk_fcie_get_cmdfifo(struct mtk_fcie_host *mtk_fcie_host,
	u16 wordcnt, u16 *buf)
{
	int i;

	for (i = 0; i < wordcnt; i++)
		buf[i] = REG(REG_ADDR(mtk_fcie_host->fciebase, CMD_FIFO)
					+ i * 4);
}

static u32 mtk_fcie_err_polling(struct mtk_fcie_host *mtk_fcie_host,
	uintptr_t regaddr, u16 events, u32 us)
{
	u32 i;
	u16 val = 0;



	for (i = 0; i < us; i++) {
		mtk_fcie_hw_timer_delay(HW_TIMER_DELAY_1US);
		val = REG(regaddr);

		if ((val & events) == events)
			break;
		cpu_relax();
	}

	if (i == us) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_LOW, 1,
			"eMMC Err: %u us, Reg.%04lXh: %04Xh, but wanted: %04Xh\n",
			us,
			(regaddr - (uintptr_t)mtk_fcie_host->fciebase)
			>> REG_OFFSET_SHIFT_BITS,
			val, events);

		return EMMC_ST_ERR_TIMEOUT_WAIT_REG0;
	}
	REG_W1C(regaddr, events); // W1C for waited Event as success

	return EMMC_ST_SUCCESS;
}

static u32 mtk_fcie_check_r1(struct mtk_fcie_host *mtk_fcie_host)
{
	u32 err = EMMC_ST_SUCCESS;
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;

	mtk_fcie_get_cmdfifo(mtk_fcie_host, 3, (u16 *)emmc_drv->rsp);

	if (emmc_drv->rsp[1] & (EMMC_ERR_R1_31_24>>24)) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: eMMC_ST_ERR_R1_31_24\n ");
		err = EMMC_ST_ERR_R1_31_24;
		goto LABEL_CHECK_R1_END;
	}

	if (emmc_drv->rsp[2] & (EMMC_ERR_R1_23_16>>16)) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: eMMC_ST_ERR_R1_23_16\n ");
		err = EMMC_ST_ERR_R1_23_16;
		goto LABEL_CHECK_R1_END;
	}

	if (emmc_drv->rsp[3] & (EMMC_ERR_R1_15_8>>8)) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: eMMC_ST_ERR_R1_15_8\n ");
		err = EMMC_ST_ERR_R1_15_8;
		goto LABEL_CHECK_R1_END;
	}

	if (emmc_drv->rsp[4] & (EMMC_ERR_R1_7_0>>0)) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: eMMC_ST_ERR_R1_7_0\n ");
		err = EMMC_ST_ERR_R1_7_0;
		goto LABEL_CHECK_R1_END;
	}

LABEL_CHECK_R1_END:

	return err;
}

static u32 mtk_fcie_err_send_cmd(struct mtk_fcie_host *mtk_fcie_host, u16 mode,
	u16 ctrl, u32 arg, u8 cmd, u8 rspbyte)
{
	u32 err, timeout = TIME_WAIT_DAT0_HIGH;

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, CMD_RSP_SIZE),
		BIT_RSP_SIZE_MASK);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, CMD_RSP_SIZE),
		rspbyte & BIT_RSP_SIZE_MASK);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), mode);

	REG_W(REG_ADDR(REG_ADDR(mtk_fcie_host->fciebase, CMD_FIFO), 0x00),
		(((arg >> 24)<<8) | (0x40|cmd)));
	REG_W(REG_ADDR(REG_ADDR(mtk_fcie_host->fciebase, CMD_FIFO), 0x01),
		((arg & 0xff00) | ((arg>>16)&0xff)));
	REG_W(REG_ADDR(REG_ADDR(mtk_fcie_host->fciebase, CMD_FIFO), 0x02),
		(arg & 0xff));

	err = mtk_fcie_wait_d0_high(mtk_fcie_host, timeout);
	if (err != EMMC_ST_SUCCESS)
		goto LABEL_SEND_CMD_ERROR;

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL), ctrl);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL), BIT_JOB_START);

	err = mtk_fcie_err_polling(mtk_fcie_host,
		(uintptr_t)REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT),
		BIT_CMD_END, HW_TIMER_DELAY_1s);

	if (cmd == 6 || cmd == 7) {
		err = mtk_fcie_wait_d0_high(mtk_fcie_host, timeout);
		if (err != EMMC_ST_SUCCESS)
			goto LABEL_SEND_CMD_ERROR;
	}

LABEL_SEND_CMD_ERROR:
	return err;
}

static u32 mtk_fcie_cmd0(struct mtk_fcie_host *mtk_fcie_host, u32 arg)
{
	struct fcie_emmc_driver	*emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err;


	mtk_fcie_clear_events(mtk_fcie_host);

	err = mtk_fcie_err_send_cmd(mtk_fcie_host, emmc_drv->reg10_mode,
		BIT_SD_CMD_EN, arg, 0, 0);
	return err;
}

static u32 mtk_fcie_cmd1(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err, arg;
	u16 ctrl, reg;


	// (sector mode | byte mode) | (3.0|3.1|3.2|3.3|3.4 V)
	arg = BIT(30) | (BIT(23)|BIT(22)|BIT(21)|BIT(20)|
		BIT(19)|BIT(18)|BIT(17)|BIT(16)|BIT(15)|BIT(7));
	ctrl = BIT_SD_CMD_EN | BIT_SD_RSP_EN;


	mtk_fcie_clear_events(mtk_fcie_host);


	err = mtk_fcie_err_send_cmd(mtk_fcie_host, emmc_drv->reg10_mode,
		ctrl, arg, 1, EMMC_R3_BYTE_CNT);

	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: CMD1 send CMD fail: %08Xh\n", err);
		return err;
	}

	// check status
	reg = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));
	// R3 has no CRC, so does not check BIT_SD_RSP_CRC_ERR
	if (reg & BIT_SD_RSP_TIMEOUT) {
		err = EMMC_ST_ERR_CMD1;
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Warn: CMD1 no Rsp, SD_STS: %04Xh\n", reg);
	} else { // CMD1 ok, do things here
		//clear status may be BIT_SD_RSP_CRC_ERR
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS),
			BIT_SD_FCIE_ERR_FLAGS);

		mtk_fcie_get_cmdfifo(mtk_fcie_host, 3, (u16 *)emmc_drv->rsp);

		if (0 == (emmc_drv->rsp[1] & 0x80))
			err = EMMC_ST_ERR_CMD1_DEV_NOT_RDY;
	}
	return err;
}

// send CID
static u32 mtk_fcie_cmd2(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err, arg;
	u16 ctrl, reg;


	arg = 0;
	ctrl = BIT_SD_CMD_EN | BIT_SD_RSP_EN | BIT_SD_RSPR2_EN;


	mtk_fcie_clear_events(mtk_fcie_host);


	err = mtk_fcie_err_send_cmd(mtk_fcie_host, emmc_drv->reg10_mode,
		ctrl, arg, 2, EMMC_R2_BYTE_CNT);

	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: CMD2, %Xh\n", err);
	} else {	// check status
		reg = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));
		// R3 has no CRC, so does not check BIT_SD_RSP_CRC_ERR
		if (reg & (BIT_SD_RSP_TIMEOUT|BIT_SD_RSP_CRC_ERR)) {
			err = EMMC_ST_ERR_CMD2;
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Warn: CMD2 no Rsp, SD_STS: %04Xh\n",
				reg);
		} else	// CMD2 ok, do things here (get CID)
			mtk_fcie_get_cmdfifo(mtk_fcie_host, EMMC_R2_BYTE_CNT>>1,
				(u16 *)emmc_drv->cid);
	}
	return err;
}

// CMD3: assign RCA. CMD7: select device
static u32 mtk_fcie_cmd3_cmd7(struct mtk_fcie_host *mtk_fcie_host,
	u16 rca, u8 cmd)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err, arg;
	u16 ctrl, reg;


	arg = rca<<16;

	if (cmd == 7 && rca != emmc_drv->rca)
		ctrl = BIT_SD_CMD_EN;
	else
		ctrl = BIT_SD_CMD_EN | BIT_SD_RSP_EN;


	mtk_fcie_clear_events(mtk_fcie_host);



	err = mtk_fcie_err_send_cmd(mtk_fcie_host, emmc_drv->reg10_mode,
		ctrl, arg, cmd, EMMC_R1_BYTE_CNT);

	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: CMD%u, %Xh\n",
			cmd, err);
	} else { // check status
		reg = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));

		// de-select has no rsp
		if (!(cmd == 7 && rca != emmc_drv->rca)) {
			if (reg & (BIT_SD_RSP_TIMEOUT|BIT_SD_RSP_CRC_ERR)) {
				if (cmd == 3) {
					EMMC_DBG(mtk_fcie_host,
						EMMC_DBG_LVL_ERROR, 1,
						"eMMC Warn: CMD%u SD_STS: %04Xh\n",
						cmd, reg);
					return err;
				}
			} else {
				err = mtk_fcie_check_r1(mtk_fcie_host);
			}
		}
	}

	return err;
}

// SWITCH cmd
static u32 mtk_fcie_cmd6(struct mtk_fcie_host *mtk_fcie_host, u32 arg)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err;
	u16 ctrl, reg;


	ctrl = BIT_SD_CMD_EN | BIT_SD_RSP_EN;


	mtk_fcie_clear_events(mtk_fcie_host);



	err = mtk_fcie_err_send_cmd(mtk_fcie_host, emmc_drv->reg10_mode,
		ctrl, arg, 6, EMMC_R1B_BYTE_CNT);

	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: CMD6 %Xh\n", err);
	} else {	// check status
		reg = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));

		if (reg & (BIT_SD_RSP_TIMEOUT|BIT_SD_RSP_CRC_ERR)) {
			err = EMMC_ST_ERR_CMD6;
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: CMD6 SD_STS: %04Xh\n", reg);
		} else
			err = mtk_fcie_check_r1(mtk_fcie_host);
	}
	return err;
}

// CMD13: send Status
static u32 mtk_fcie_cmd13(struct mtk_fcie_host *mtk_fcie_host, u16 rca)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err, arg;
	u16 ctrl, reg;


	ctrl = BIT_SD_CMD_EN | BIT_SD_RSP_EN;
	arg = rca << 16;

	mtk_fcie_clear_events(mtk_fcie_host);

	err = mtk_fcie_err_send_cmd(mtk_fcie_host, emmc_drv->reg10_mode,
		ctrl, arg, 13, EMMC_R1_BYTE_CNT);

	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: CMD13 %Xh\n", err);
	} else {	// check status
		reg = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));

		if (reg & (BIT_SD_RSP_TIMEOUT|BIT_SD_RSP_CRC_ERR)) {
			err = EMMC_ST_ERR_CMD6;
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: CMD13 SD_STS: %04Xh\n", reg);
		} else
			err = mtk_fcie_check_r1(mtk_fcie_host);
	}
	return err;
}

static u32 mtk_fcie_identify(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err = EMMC_ST_SUCCESS;
	u16 i, retry = 0;

	emmc_drv->rca = 1;
	emmc_drv->bus_width = BIT_SD_DATA_WIDTH_1;
	emmc_drv->reg10_mode &= ~BIT_SD_DATA_WIDTH_MASK;

LABEL_IDENTIFY_CMD0:
	_mtk_fcie_mmc_hw_reset(mtk_fcie_host);

	if (retry > 10)
		return err;

	if (retry)
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL, 1,
			"eMMC Warn: retry: %u\n", retry);

	// CMD0
	err = mtk_fcie_cmd0(mtk_fcie_host, 0); // reset to idle state
	if (err != EMMC_ST_SUCCESS) {
		retry++;
		goto LABEL_IDENTIFY_CMD0;
	}

	// CMD1
	for (i = 0; i < 0x8000; i++) {
		err = mtk_fcie_cmd1(mtk_fcie_host);
		if (err == EMMC_ST_SUCCESS)
			break;

		mtk_fcie_hw_timer_delay(2000);

		if (err != EMMC_ST_ERR_CMD1_DEV_NOT_RDY) {
			retry++;
			goto LABEL_IDENTIFY_CMD0;
		}
	}
	if (err != EMMC_ST_SUCCESS) {
		retry++;
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL, 1,
			"eMMC Warn: CMD1 wait eMMC device ready timeout\n");
		goto LABEL_IDENTIFY_CMD0;
	}

	// CMD2
	err = mtk_fcie_cmd2(mtk_fcie_host);
	if (err != EMMC_ST_SUCCESS) {
		retry++;
		goto LABEL_IDENTIFY_CMD0;
	}

	// CMD3
	err = mtk_fcie_cmd3_cmd7(mtk_fcie_host, emmc_drv->rca, 3);
	if (err != EMMC_ST_SUCCESS) {
		retry++;
		goto LABEL_IDENTIFY_CMD0;
	}

	return EMMC_ST_SUCCESS;
}

static u32 mtk_fcie_set_extcsd(struct mtk_fcie_host *mtk_fcie_host,
	u8 mode, u8 idx, u8 value)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 arg, err;
	u8 retry_prg = 0;

	arg = ((mode&3)<<24) | (idx<<16) |
			  (value<<8);

	err = mtk_fcie_cmd6(mtk_fcie_host, arg);
	if (err != EMMC_ST_SUCCESS) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: eMMC: %Xh\n", err);
		return err;
	}

	do {
		if (retry_prg > 5)
			break;

		err = mtk_fcie_cmd13(mtk_fcie_host, emmc_drv->rca);
		if (err != EMMC_ST_ERR_R1_7_0 &&
			err != EMMC_ST_SUCCESS) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Warn: %Xh\n", err);
			return err;
		} else if (err == EMMC_ST_SUCCESS)
			break;

		retry_prg++;
	} while (err == EMMC_ST_ERR_R1_7_0);

	return err;
}

static u32 mtk_fcie_set_bus(struct mtk_fcie_host *mtk_fcie_host, u8 bus, u8 ddr)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u8	value;
	u32 err;

	// -------------------------------
	switch (bus) {
	case 1:
		value = 0;
		break;
	case 4:
		value = 1;
		break;
	case 8:
		value = 2;
		break;
	default:
		return EMMC_ST_ERR_PARAMETER;
	}

	if (ddr) {
		value |= BIT(2);
		emmc_drv->drv_flag |= DRV_FLAG_DDR_MODE;
	} else
		emmc_drv->drv_flag &= ~DRV_FLAG_DDR_MODE;

	if (ddr == 2 && emmc_drv->ecsd184_stroe_support)
		value |= BIT(7); // Enhanced Storbe

	// -------------------------------
	// BUS_WIDTH
	err = mtk_fcie_set_extcsd(mtk_fcie_host,
		EMMC_EXTCSD_WBYTE, 183, value);
	if (err != EMMC_ST_SUCCESS)
		return err;

	// -------------------------------
	emmc_drv->reg10_mode &= ~BIT_SD_DATA_WIDTH_MASK;
	switch (bus) {
	case 1:
		emmc_drv->bus_width = BIT_SD_DATA_WIDTH_1;
		emmc_drv->reg10_mode |= BIT_SD_DATA_WIDTH_1;
		break;
	case 4:
		emmc_drv->bus_width = BIT_SD_DATA_WIDTH_4;
		emmc_drv->reg10_mode |= BIT_SD_DATA_WIDTH_4;
		break;
	case 8:
		emmc_drv->bus_width = BIT_SD_DATA_WIDTH_8;
		emmc_drv->reg10_mode |= BIT_SD_DATA_WIDTH_8;
		break;
	}

	return err;
}


static u32 mtk_fcie_set_speed(struct mtk_fcie_host *mtk_fcie_host, u8 speed)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err;

	emmc_drv->ecsd185_hstiming &= ~0x0F;
	emmc_drv->ecsd185_hstiming |= speed;

	err = mtk_fcie_set_extcsd(mtk_fcie_host,
		EMMC_EXTCSD_WBYTE, 185, emmc_drv->ecsd185_hstiming);
	if (err != EMMC_ST_SUCCESS)
		return err;

	emmc_drv->drv_flag &= ~DRV_FLAG_SPEED_MASK;
	switch (speed) {
	case SPEED_HIGH:
		emmc_drv->drv_flag |= DRV_FLAG_SPEED_HIGH;
		break;
	case SPEED_HS200:
		emmc_drv->drv_flag |= DRV_FLAG_SPEED_HS200;
		break;
	case SPEED_HS400:
		emmc_drv->drv_flag |= DRV_FLAG_SPEED_HS400;
		break;
	default:
		break;
	}

	return err;
}

static u32 mtk_fcie_set_fast_mode(struct mtk_fcie_host *mtk_fcie_host,
	u8 pad_type)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err = 0;

	switch (pad_type) {
	case FCIE_EMMC_DDR:
		err = mtk_fcie_set_speed(mtk_fcie_host, SPEED_HIGH);
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: enable HighSpeed fail: %Xh\n", err);
			return err;
		}
		err = mtk_fcie_set_bus(mtk_fcie_host, 8, 1);
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: set DDR fail: %Xh\n", err);
			return err;
		}
		break;

	case FCIE_EMMC_HS200:
		err = mtk_fcie_set_bus(mtk_fcie_host, 8, 0); // disable DDR
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: HS200 disable DDR fail: %Xh\n", err);
			return err;
		}
		err = mtk_fcie_set_speed(mtk_fcie_host, SPEED_HS200);
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: set HS200 fail: %Xh\n", err);
			return err;
		}
		break;

	case FCIE_EMMC_HS400:
		err = mtk_fcie_set_bus(mtk_fcie_host, 8, 1); // enable DDR
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: HS400 set DDR fail: %Xh\n", err);
			return err;
		}
		if (emmc_drv->t_table_g.set_cnt) {
			emmc_drv->ecsd185_hstiming &= ~(0xF0);
			emmc_drv->ecsd185_hstiming |=
				(emmc_drv->t_table_g.device_driving & 0xF) << 4;
		}
		err = mtk_fcie_set_speed(mtk_fcie_host, SPEED_HS400);
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: set HS400 fail: %Xh\n", err);
			return err;
		}
		break;
	case FCIE_EMMC_HS400_5_1:
		err = mtk_fcie_set_bus(mtk_fcie_host, 8, 2); // enable DDR
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: HS400 5.1 set DDR fail: %Xh\n", err);
			return err;
		}
		if (emmc_drv->t_table_g.set_cnt) {
			emmc_drv->ecsd185_hstiming &= ~(0xF0);
			emmc_drv->ecsd185_hstiming |=
				(emmc_drv->t_table_g.device_driving & 0xF) << 4;
		}
		err = mtk_fcie_set_speed(mtk_fcie_host, SPEED_HS400);
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: set HS400 5.1 fail: %Xh\n", err);
			return err;
		}
		break;
	}

	// --------------------------------------
	if (err) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: set ECSD fail: %Xh\n", err);
		return err;
	}

	mtk_fcie_pads_switch(pad_type, mtk_fcie_host);
	if (emmc_drv->t_table_g.set_cnt &&
		(pad_type == FCIE_EMMC_HS400 ||
		pad_type == FCIE_EMMC_HS400_5_1))
		mtk_fcie_apply_timing_set(
			emmc_drv->t_table_g.curset_idx,
			mtk_fcie_host);
	else
		if (emmc_drv->t_table.set_cnt)
			mtk_fcie_apply_timing_set(EMMC_TIMING_SET_MAX,
				mtk_fcie_host);


	return err;
}

static u32 _mtk_fcie_err_handler(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err;

	char *part[8] = {"eMMC: user OK",
					 "eMMC: boot1 OK",
					 "eMMC: boot2 OK",
					 "eMMC: RPMB OK",
					 "eMMC: GP1  OK",
					 "eMMC: GP2 OK",
					 "eMMC: GP3 OK",
					 "eMMC: GP4 OK"};

	char *err_part[8] = {"eMMC: user fail",
					 "eMMC: boot1 fail",
					 "eMMC: boot2 fail",
					 "eMMC: RPMB fail",
					 "eMMC: GP1  fail",
					 "eMMC: GP2 fail",
					 "eMMC: GP3 fail",
					 "eMMC: GP4 fail"};

	err = mtk_fcie_hw_init(mtk_fcie_host);
	if (err) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: FCIE_Init fail, %Xh\n", err);
		return err;
	}

	//disable power saving mode
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
		BIT_POWER_SAVE_MODE|BIT_POWER_SAVE_MODE_INT_EN);



	emmc_drv->drv_flag = 0;

	err = mtk_fcie_identify(mtk_fcie_host);
	if (err) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: Identify fail, %Xh\n", err);
		return err;
	}

	mtk_fcie_clock_setting(0, mtk_fcie_host,
		12000000);

	err = mtk_fcie_cmd3_cmd7(mtk_fcie_host, emmc_drv->rca, 7);
	if (err) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: CMD7 fail, %Xh\n", err);
		return err;
	}

	err = mtk_fcie_set_speed(mtk_fcie_host, SPEED_HIGH);
	if (err) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: set High speed fail: %Xh\n", err);
		return err;
	}

	if (emmc_drv->partition_config&0x7) {
		err = mtk_fcie_set_extcsd(mtk_fcie_host, EMMC_EXTCSD_WBYTE,
			179, emmc_drv->partition_config);

		if (err)
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 0,
				"eMMC Err: %s, %Xh\n",
				err_part[(u8)(emmc_drv->partition_config&PART_CONFIG_ACCESS_MASK)],
				err);
		else
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL, 0, "eMMC Warn: %s\n",
			    part[(u8)(emmc_drv->partition_config&PART_CONFIG_ACCESS_MASK)]);
	}

	if (emmc_drv->cache_ctrl&0x1) {
		err = mtk_fcie_set_extcsd(mtk_fcie_host, EMMC_EXTCSD_WBYTE,
			33, emmc_drv->cache_ctrl);

		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: set cache control fail: %Xh\n", err);
			return err;
		}
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL, 1,
			"eMMC Warn: cache control OK\n");
	}

	if (emmc_drv->cmdq_enable&0x1) {
		err = mtk_fcie_set_extcsd(mtk_fcie_host, EMMC_EXTCSD_WBYTE,
			15, emmc_drv->cmdq_enable);

		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: set command queue enable fail: %Xh\n", err);
			return err;
		}
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL, 1,
			"eMMC Warn: command queue enable OK\n");
	}
	if (emmc_drv->ecsd192_ver >= HIGH_CAPACITY_VERSION) {
		err = mtk_fcie_set_extcsd(mtk_fcie_host, EMMC_EXTCSD_WBYTE,
			EXT_CSD_ERASE_GROUP_DEF, 0x1);
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: set ERASE GRP DEF fail: %Xh\n", err);
			return err;
		}
	}

	return err;
}
static void mtk_fcie_err_handler(struct mtk_fcie_host *mtk_fcie_host)
{
	struct fcie_emmc_driver	*emmc_drv = mtk_fcie_host->emmc_drv;
	u32 err = 0;
	u32 drv_flag = emmc_drv->drv_flag;
	u16 reg10 = emmc_drv->reg10_mode;
	u8	pad_type = emmc_drv->pad_type;
	u8	bus_width = emmc_drv->bus_width;

	mtk_fcie_host->emmc_drv->old_pll_clk_param = 0xffff;
	mtk_fcie_host->emmc_drv->old_pll_dll_clk_param = 0xffff;
	mtk_fcie_host->emmc_drv->tune_overtone = 1;

	err = _mtk_fcie_err_handler(mtk_fcie_host);
	if (err)
		goto LABEL_REINIT_END;

	// ---------------------------------
	emmc_drv->drv_flag = drv_flag;
	//CMD8 error retry
	if (emmc_drv->drv_flag == 0) {
		switch (bus_width) {
		case BIT_SD_DATA_WIDTH_8:
			err = mtk_fcie_set_bus(mtk_fcie_host, 8, 0);
			if (err) {
				EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
					"eMMC Err: set 8 bus width fail: %Xh\n",
					err);
				goto LABEL_REINIT_END;
			}
			break;
		case BIT_SD_DATA_WIDTH_4:
			err = mtk_fcie_set_bus(mtk_fcie_host, 4, 0);
			if (err) {
				EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
					"eMMC Err: set 4 bus width fail: %Xh\n",
					err);
				goto LABEL_REINIT_END;
			}
			break;
		case BIT_SD_DATA_WIDTH_1:
			err = mtk_fcie_set_bus(mtk_fcie_host, 1, 0);
			if (err) {
				EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
					"eMMC Err: set 1 bus width fail: %Xh\n",
					err);
				goto LABEL_REINIT_END;
			}
			break;
		default:
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: wrong parameter for bus width\n");
			err = EMMC_ST_ERR_PARAMETER;
			goto LABEL_REINIT_END;
		}

		emmc_drv->reg10_mode = reg10;
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE),
			emmc_drv->reg10_mode);
		//init OK enable power saving mode
		if (mtk_fcie_host->enable_sar5)
			REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase,
				PWR_SAVE_CTL),
			BIT_POWER_SAVE_MODE|BIT_POWER_SAVE_MODE_INT_EN);
		return;
	}

	if (0 == (emmc_drv->drv_flag&DRV_FLAG_DDR_MODE) &&
		DRV_FLAG_SPEED_HIGH ==
		(emmc_drv->drv_flag&DRV_FLAG_SPEED_MASK)) {
		mtk_fcie_pads_switch(FCIE_EMMC_BYPASS,
			mtk_fcie_host);
		mtk_fcie_clock_setting(0, mtk_fcie_host,
			48000000);
	} else {
		err = mtk_fcie_set_fast_mode(mtk_fcie_host, pad_type);
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: mtk_fcie_set_fast_mode fail, %Xh\n",
				err);
			goto LABEL_REINIT_END;
		}
	}

	emmc_drv->reg10_mode = reg10;
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), emmc_drv->reg10_mode);
	//init OK enable power saving mode
	if (mtk_fcie_host->enable_sar5)
		REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase,
			PWR_SAVE_CTL),
		BIT_POWER_SAVE_MODE|BIT_POWER_SAVE_MODE_INT_EN);

LABEL_REINIT_END:
	if (err)
		EMMC_DIE("\n");
}

//=======================================================================
static void emmc_record_wr_time(struct mtk_fcie_host *mtk_fcie_host,
	u8 cmd_idx, u32 bytes)
{
	struct mtk_fcie_emmc_rw_speed *rw_sp;
	u32 time = 0;
	u64 ktime = 0;

	if (mtk_fcie_host->emmc_monitor_enable) {
		rw_sp = &mtk_fcie_host->emmc_rw_speed;
		ktime = (u64)ktime_to_ns(
		ktime_sub(ktime_get(), mtk_fcie_host->starttime));
		do_div(ktime, SPEED_TIME_MSEC);
		time = (u32)ktime;

		if (cmd_idx == MMC_READ_SINGLE_BLOCK ||
			cmd_idx == MMC_READ_MULTIPLE_BLOCK) {
			if ((rw_sp->total_read_bytes+bytes) > 0 &&
				(rw_sp->total_read_time_usec+time) > 0
				) {
				rw_sp->total_read_bytes += bytes;
				rw_sp->total_read_time_usec += time;
			}
		} else if (cmd_idx == MMC_WRITE_BLOCK ||
			cmd_idx == MMC_WRITE_MULTIPLE_BLOCK) {
			if ((rw_sp->total_write_bytes+bytes) > 0 &&
				(rw_sp->total_write_time_usec+time) > 0
				) {
				rw_sp->total_write_bytes += bytes;
				rw_sp->total_write_time_usec += time;
			}
		}
	}
}

static void cqe_record_wr_time(struct mtk_fcie_host *mtk_fcie_host)
{
	struct mtk_fcie_emmc_rw_speed *rw_sp;
	u32 time = 0;
	u64 ktime = 0;


	rw_sp = &mtk_fcie_host->emmc_rw_speed;
	ktime = (u64)ktime_to_ns(
	ktime_sub(ktime_get(), mtk_fcie_host->starttime));
	do_div(ktime, SPEED_TIME_MSEC);
	time = (u32)ktime;

	rw_sp->total_cqe_time_usec += time;
}

static void cqe_record_wr_data_size(struct mtk_fcie_host *mtk_fcie_host,
	u8 cmd_idx, u32 bytes)
{
	struct mtk_fcie_emmc_rw_speed *rw_sp;


	if (mtk_fcie_host->cqe_monitor_enable) {
		rw_sp = &mtk_fcie_host->emmc_rw_speed;

		if (cmd_idx == MMC_EXECUTE_READ_TASK)
			rw_sp->total_read_bytes += bytes;
		else if (cmd_idx == MMC_EXECUTE_WRITE_TASK)
			rw_sp->total_write_bytes += bytes;
	}
}

static void emmc_check_life(struct mtk_fcie_host *mtk_fcie_host, u32 bytes)
{
	struct tm time_tmp = {0};//fix compile warring
	struct mtk_fcie_emmc_rw_speed *rw_sp;

	rw_sp = &mtk_fcie_host->emmc_rw_speed;
	if ((rw_sp->day_write_bytes + bytes) > 0)
		rw_sp->day_write_bytes += bytes;

	time64_to_tm(ktime_get_real_seconds(), 0, &time_tmp);

	if (rw_sp->int_tm_yday != time_tmp.tm_yday) {
		if (rw_sp->day_write_bytes >= EMMC_DAY_WRITE_WARN)
			EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 0,
			"eMMC Err: Warn, w %lld MB\n",
				rw_sp->day_write_bytes >> 20);

		rw_sp->day_write_bytes = 0;
	}

	rw_sp->int_tm_yday = time_tmp.tm_yday;

}

static ssize_t emmc_monitor_count_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 speed_read = 0, speed_write = 0;
	u32 total_read_gb, total_write_gb;
	u32 total_read_mb, total_write_mb;
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;
	struct mtk_fcie_emmc_rw_speed *rw_sp;
	u64 multi = 0, result = 0, temp = 0, residue = 0;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);
	rw_sp = &mtk_fcie_host->emmc_rw_speed;

	if (rw_sp->total_read_time_usec > SPEED_TIME_MSEC) {

		result = rw_sp->total_read_time_usec;
		do_div(result, SPEED_TIME_MSEC);
		temp = rw_sp->total_read_bytes;
		do_div(temp, result);
		multi = temp;

		result = multi * SPEED_TIME_MSEC;
		do_div(result, WR_MB);

		speed_read = (u32)result;//MB/sec
	}
	if (rw_sp->total_write_time_usec > SPEED_TIME_MSEC) {

		result = rw_sp->total_write_time_usec;
		do_div(result, SPEED_TIME_MSEC);
		temp = rw_sp->total_write_bytes;
		do_div(temp, result);
		multi = temp;

		result = multi * SPEED_TIME_MSEC;
		do_div(result, WR_MB);

		speed_write = (u32)result;//MB/sec
	}

	result = rw_sp->total_read_bytes;
	residue = do_div(result, WR_GB);
	total_read_gb = (u32)result;

	do_div(residue, WR_MB);
	total_read_mb = (u32)residue;

	result = rw_sp->total_write_bytes;
	residue = do_div(result, WR_GB);
	total_write_gb = (u32)result;

	do_div(residue, WR_MB);
	total_write_mb = (u32)residue;

	return scnprintf(buf, PAGE_SIZE,
		"eMMC: read total %dGB %dMB, %uMB/sec\n"
		"eMMC: write total %dGB %dMB, %uMB/sec\n"
		"%xh\n",
		total_read_gb, total_read_mb, speed_read,
		total_write_gb, total_write_mb, speed_write,
		mtk_fcie_host->emmc_monitor_enable);
}


static ssize_t emmc_monitor_count_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long temp = 0;
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	if (kstrtoul(buf, 0, &temp))
		return -EINVAL;

	mtk_fcie_host->emmc_monitor_enable = temp;

	if (mtk_fcie_host->emmc_monitor_enable == 0) {
		mtk_fcie_host->emmc_rw_speed.total_read_bytes = 0;
		mtk_fcie_host->emmc_rw_speed.total_read_time_usec = 0;
		mtk_fcie_host->emmc_rw_speed.total_write_bytes = 0;
		mtk_fcie_host->emmc_rw_speed.total_write_time_usec = 0;
	}

	return count;
}

static DEVICE_ATTR_RW(emmc_monitor_count_enable);

static ssize_t emmc_write_log_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
		mtk_fcie_host->emmc_write_log_enable);
}

static ssize_t emmc_write_log_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long temp = 0;
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	if (kstrtoul(buf, 0, &temp))
		return -EINVAL;

	mtk_fcie_host->emmc_write_log_enable = temp;

	if (mtk_fcie_host->emmc_write_log_enable) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL, 0,
			"eMMC: write log enable\n");
	}

	return count;
}

static DEVICE_ATTR_RW(emmc_write_log_enable);

static ssize_t emmc_read_log_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
		mtk_fcie_host->emmc_read_log_enable);
}

static ssize_t emmc_read_log_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long temp = 0;
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	if (kstrtoul(buf, 0, &temp))
		return -EINVAL;

	mtk_fcie_host->emmc_read_log_enable = temp;

	if (mtk_fcie_host->emmc_read_log_enable) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL, 0,
			"eMMC: read log enable\n");
	}

	return count;
}

static DEVICE_ATTR_RW(emmc_read_log_enable);

static ssize_t emmc_sar5_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	return scnprintf(buf, PAGE_SIZE, "SAR5: %u\n",
		mtk_fcie_host->enable_sar5);
}
static DEVICE_ATTR_RO(emmc_sar5_status);

static ssize_t emmc_fde_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	return scnprintf(buf, PAGE_SIZE, "fde: %u\n",
		mtk_fcie_host->enable_fde);
}
static DEVICE_ATTR_RO(emmc_fde_status);

static ssize_t log_level_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;
	ssize_t retval;
	int level;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);


	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < EMMC_DBG_LVL_MUTE) || (level > EMMC_DBG_LVL_LOW))
			retval = -EINVAL;
		else
			mtk_fcie_host->log_level = level;
	}
	return (retval < 0) ? retval : count;
}
static ssize_t log_level_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	return sprintf(buf, "mute:\t\t%d\nerror:\t\t%d\n"
		"warn:\t\t%d\nnotice:\t\t%d\n"
		"info:\t\t%d\ncurrent:\t%d\n",
		EMMC_DBG_LVL_MUTE, EMMC_DBG_LVL_ERROR,
		EMMC_DBG_LVL_WARNING, EMMC_DBG_LVL_HIGH,
		EMMC_DBG_LVL_MEDIUM, mtk_fcie_host->log_level);
}
static DEVICE_ATTR_RW(log_level);

static ssize_t emmc_emulate_power_lost_event_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
		mtk_fcie_host->emulate_power_lost_event_enable);
}

static ssize_t emmc_emulate_power_lost_event_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long temp = 0;
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	if (kstrtoul(buf, 0, &temp))
		return -EINVAL;

	mtk_fcie_host->emulate_power_lost_event_enable = temp;




	return count;
}

static DEVICE_ATTR_RW(emmc_emulate_power_lost_event_enable);


static ssize_t emmc_dump_register_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
		mtk_fcie_host->dump_fcie_reg_enable);
}


static ssize_t emmc_dump_register_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long temp = 0;
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	if (kstrtoul(buf, 0, &temp))
		return -EINVAL;

	if (temp) {
		mtk_fcie_reg_dump(mtk_fcie_host);
		mtk_fcie_debug_bus_dump(mtk_fcie_host);
	}

	return count;
}

static DEVICE_ATTR_RW(emmc_dump_register_enable);

static ssize_t cqe_monitor_count_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 total_read_gb, total_write_gb;
	u32 total_read_mb, total_write_mb;
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;
	struct mtk_fcie_emmc_rw_speed *rw_sp;
	u64 time_msec = 0, time_sec = 0, result = 0, residue = 0;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);
	rw_sp = &mtk_fcie_host->emmc_rw_speed;

	result = rw_sp->total_cqe_time_usec;
	do_div(result, SPEED_TIME_MSEC);
	time_msec = do_div(result, SPEED_TIME_MSEC);
	time_sec = result;

	result = rw_sp->total_read_bytes;
	residue = do_div(result, WR_GB);
	total_read_gb = (u32)result;
	do_div(residue, WR_MB);
	total_read_mb = (u32)residue;

	result = rw_sp->total_write_bytes;
	residue = do_div(result, WR_GB);
	total_write_gb = (u32)result;
	do_div(residue, WR_MB);
	total_write_mb = (u32)residue;

	return scnprintf(buf, PAGE_SIZE,
		"eMMC: read total %dGB %dMB\n"
		"eMMC: write total %dGB %dMB\n"
		"eMMC:  %llu.%llu sec\n"
		"%xh\n",
		total_read_gb, total_read_mb,
		total_write_gb, total_write_mb,
		time_sec, time_msec,
		mtk_fcie_host->cqe_monitor_enable);
}

static ssize_t cqe_monitor_count_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long temp = 0;
	struct mmc_host *mmc;
	struct mtk_fcie_host *mtk_fcie_host;

	mmc = platform_get_drvdata(to_platform_device(dev));
	mtk_fcie_host = mmc_priv(mmc);

	if (kstrtoul(buf, 0, &temp))
		return -EINVAL;

	mtk_fcie_host->cqe_monitor_enable = temp;

	if (mtk_fcie_host->cqe_monitor_enable == 0) {
		mtk_fcie_host->emmc_rw_speed.total_read_bytes = 0;
		mtk_fcie_host->emmc_rw_speed.total_write_bytes = 0;
		mtk_fcie_host->emmc_rw_speed.total_cqe_time_usec = 0;
	}

	return count;
}

static DEVICE_ATTR_RW(cqe_monitor_count_enable);



static ssize_t help_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "Debug Help:\n"
		"- log_level <RW>: To control debug log level.\n"
		"                  Read log_level for the definition of log level number.\n"
		"- emmc_monitor_count_enable <RW>: To control write/read byte mointer.\n"
		"                                  show accumulated write/read byte count after mointer is enabled.\n"
		"- emmc_write_log_enable <RW>: To control flag for write log.\n"
		"                              show write log flag.\n"
		"- emmc_read_log_enable <RW>: To control flag for read log.\n"
		"                             show read log flag.\n"
		"- emmc_sar5_status <RO>: show power off detect (SAR5) status.\n"
		"- emmc_fde_status <RO>: show fde (crypto) status.\n"
		"- emmc_emulate_power_lost_event_enable <RW>: To emulate power lost event.\n"
		"                             show emulate power lost event flag.\n"
		"- emmc_dump_register_enable <WO>: To dump FCIE register\n"
		"- cqe_monitor_count_enable_store <RW>: To control write/read byte mointer.\n"
		"                                       show accumulated write/read byte count after mointer is enabled.\n");
}
static DEVICE_ATTR_RO(help);


static struct attribute *mtk_fcie_attr[] = {
	&dev_attr_emmc_read_log_enable.attr,
	&dev_attr_emmc_write_log_enable.attr,
	&dev_attr_emmc_monitor_count_enable.attr,
	&dev_attr_emmc_sar5_status.attr,
	&dev_attr_emmc_fde_status.attr,
	&dev_attr_log_level.attr,
	&dev_attr_emmc_emulate_power_lost_event_enable.attr,
	&dev_attr_emmc_dump_register_enable.attr,
	&dev_attr_cqe_monitor_count_enable.attr,
	&dev_attr_help.attr,
	NULL,
};

static struct attribute_group mtk_fcie_attr_grp = {
	.name = "mtk_dbg",
	.attrs = mtk_fcie_attr,
};

static const struct mmc_host_ops mtk_fcie_ops = {
	.pre_req		= mtk_fcie_pre_req,
	.post_req		= mtk_fcie_post_req,
	.request		= mtk_fcie_request,
	.set_ios		= mtk_fcie_set_ios,
	.card_busy		= mtk_fcie_card_busy,
	.hw_reset		= mtk_fcie_mmc_hw_reset,
	.select_drive_strength = mtk_fcie_select_drive_strength,
};

static int mtk_fcie_probe(struct platform_device *pdev)
{
	/* Define Local Variables */
	struct mmc_host *mmc_host = 0;
	struct mtk_fcie_host *mtk_fcie_host = 0;
	struct mtk_cqe_host *mtk_cqe_host = 0;
	int ret = 0;
	struct resource *res;
	u32 tmp;
	struct emmc_crypto_disable_info *info;
	size_t sz = 0;
	int len = 0, i;
	u32 *val;
	// --------------------------------
	if (!pdev)
		return -EINVAL;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "No DT found\n");
		return -EINVAL;
	}

	mmc_host = mmc_alloc_host(sizeof(struct mtk_fcie_host), &pdev->dev);
	if (!mmc_host) {
		dev_err(&pdev->dev,
			"mmc_alloc_host fail\n");
		return -ENOMEM;
	}
	mtk_fcie_host = mmc_priv(mmc_host);
	mmc_host->ops = &mtk_fcie_ops;

	mtk_fcie_host->emmc_drv =
		kzalloc(sizeof(struct fcie_emmc_driver), GFP_NOIO);
	if (!mtk_fcie_host->emmc_drv) {
		dev_err(&pdev->dev,
			"emmc_drv alloc fail\n");
		ret = -ENOMEM;
		goto fail_end;
	}
	mtk_fcie_host->emmc_drv->old_pll_clk_param = 0xffff;
	mtk_fcie_host->emmc_drv->old_pll_dll_clk_param = 0xffff;
	mtk_fcie_host->emmc_drv->tune_overtone = 1;

	ret = mmc_of_parse(mmc_host);
	if (ret) {
		dev_err(&pdev->dev,
			"result of mmc parse is %d\n", ret);
		goto fail_end;
	}
	//get fcie register base from DTS
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mtk_fcie_host->fciebase =
		devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mtk_fcie_host->fciebase)) {
		ret = PTR_ERR(mtk_fcie_host->fciebase);
		goto fail_end;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	mtk_fcie_host->pwsbase =
		devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mtk_fcie_host->pwsbase)) {
		ret = PTR_ERR(mtk_fcie_host->pwsbase);
		goto fail_end;
	}

	//get emmcpll register base from DTS
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	mtk_fcie_host->emmcpllbase =
		devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mtk_fcie_host->emmcpllbase)) {
		ret = PTR_ERR(mtk_fcie_host->emmcpllbase);
		goto fail_end;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	mtk_fcie_host->fcieclkreg =
		devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(mtk_fcie_host->fcieclkreg)) {
		ret = PTR_ERR(mtk_fcie_host->fcieclkreg);
		goto fail_end;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	mtk_fcie_host->riubaseaddr =
		devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(mtk_fcie_host->riubaseaddr)) {
		ret = PTR_ERR(mtk_fcie_host->riubaseaddr);
		goto fail_end;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 5);
	mtk_fcie_host->fdebase =
		devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mtk_fcie_host->fdebase))
		mtk_fcie_host->fdebase = NULL;

	//get clock obj
	mtk_fcie_host->fcie_syn_clk =
		devm_clk_get(&pdev->dev, "fcie_syn");
	if (IS_ERR(mtk_fcie_host->fcie_syn_clk)) {
		ret = PTR_ERR(mtk_fcie_host->fcie_syn_clk);
		goto fail_end;
	}

	mtk_fcie_host->fcie_top_clk =
		devm_clk_get(&pdev->dev, "clk_fcie");
	if (IS_ERR(mtk_fcie_host->fcie_top_clk)) {
		ret = PTR_ERR(mtk_fcie_host->fcie_top_clk);
		goto fail_end;
	}

	mtk_fcie_host->smi_fcie2fcie =
		devm_clk_get(&pdev->dev, "smi_fcie2fcie");
	if (IS_ERR(mtk_fcie_host->smi_fcie2fcie))
		mtk_fcie_host->smi_fcie2fcie = NULL;

	clk_prepare_enable(mtk_fcie_host->fcie_syn_clk);
	clk_prepare_enable(mtk_fcie_host->fcie_top_clk);
	if (mtk_fcie_host->smi_fcie2fcie)
		clk_prepare_enable(mtk_fcie_host->smi_fcie2fcie);

	//get customize attribute
	if (!of_property_read_u32(pdev->dev.of_node, "host-driving",
				  &mtk_fcie_host->host_driving))
		dev_dbg(&pdev->dev, "host-driving: %x\n",
			mtk_fcie_host->host_driving);

	if (!of_property_read_u32(pdev->dev.of_node, "device-driving",
				  &mtk_fcie_host->device_driving))
		dev_dbg(&pdev->dev, "device-driving: %x\n",
			mtk_fcie_host->device_driving);

	if (of_property_read_bool(pdev->dev.of_node, "support-cqe"))
		mtk_fcie_host->support_cqe = 1;
	else
		mtk_fcie_host->support_cqe = 0;

	mtk_fcie_host->share_ip = 0;
	mtk_fcie_host->share_pin = 0;

	if (of_property_read_bool(pdev->dev.of_node, "sar5-disable"))
		mtk_fcie_host->enable_sar5 = 0;
	else
		mtk_fcie_host->enable_sar5 = 1;

	if (of_property_read_bool(pdev->dev.of_node, "ssc-disable"))
		mtk_fcie_host->enable_ssc = 0;
	else
		mtk_fcie_host->enable_ssc = 1;

	if (of_property_read_bool(pdev->dev.of_node, "emmc_hs400_skew4"))
		mtk_fcie_host->emmc_pll_skew4 = 1;
	else
		mtk_fcie_host->emmc_pll_skew4 = 0;

	if (of_property_read_bool(pdev->dev.of_node, "emmc_pll_flash_macro"))
		mtk_fcie_host->emmc_pll_flash_macro = 1;
	else
		mtk_fcie_host->emmc_pll_flash_macro = 0;



	if (mtk_fcie_host->fdebase != NULL) {
		mtk_fcie_host->enable_fde = 1;
		INIT_LIST_HEAD(&mtk_fcie_host->crypto_white_list);

		if (!of_get_property(pdev->dev.of_node,
				"crypto-disable-list", &len)) {
			dev_info(&pdev->dev,
				"crypto-disable-list not specified\n");
			goto fail_end;
		}

		if (len <= 0)
			goto fail_end;

		sz = len / sizeof(*val);

		val = devm_kcalloc(&pdev->dev,
				sz, sizeof(*val), GFP_KERNEL);
		if (!val) {
			ret = -ENOMEM;
			goto fail_end;
		}

		ret = of_property_read_u32_array(
				pdev->dev.of_node, "crypto-disable-list", val, sz);
		if (ret && (ret != -EINVAL)) {
			dev_err(&pdev->dev,
				"%s: error reading array %d\n",
				"crypto-disable-list", ret);
			goto fail_end;
		}

		for (i = 0; i < sz; i += 4) {
			info = devm_kzalloc(&pdev->dev,
					sizeof(*info), GFP_KERNEL);
			if (!info) {
				ret = -ENOMEM;
				goto fail_end;
			}

			info->lba =
				(((u64)val[i]<<32) | (u64)val[i+1])>>9;
			info->length =
				(((u64)val[i+2]<<32) | (u64)val[i+3])>>9;
			dev_dbg(&pdev->dev,
				"%s: lba %llx length %llx\n", "crypto-disable-list",
				info->lba, info->length);
			list_add_tail(&info->list,
				&mtk_fcie_host->crypto_white_list);
		}
	} else
		mtk_fcie_host->enable_fde = 0;


	ret = of_property_read_u32(pdev->dev.of_node->parent, "miu0-base",
		&tmp);
	if (ret == 0) {
		mtk_fcie_host->miu0_base = tmp;
		pr_info("parent dts miu0_base 0x%llX\n",
			mtk_fcie_host->miu0_base);
	} else {
		ret = of_property_read_u32(pdev->dev.of_node, "miu0-base",
			&tmp);
		if (ret == 0) {
			mtk_fcie_host->miu0_base = tmp;
			pr_info("mmc dts miu0_base 0x%llX\n",
				mtk_fcie_host->miu0_base);
		} else {
			pr_err("no defined miu0_base");
			ret = -EINVAL;
			goto fail_end;

		}
	}

	if (!of_property_read_u32(pdev->dev.of_node, "clk-shift",
				  &mtk_fcie_host->clk_shift))
		dev_dbg(&pdev->dev, "clk-shift: %x\n",
			mtk_fcie_host->clk_shift);

	if (!of_property_read_u32(pdev->dev.of_node, "clk-mask",
				  &mtk_fcie_host->clk_mask))
		dev_dbg(&pdev->dev, "clk-mask: %x\n",
			mtk_fcie_host->clk_mask);

	if (!of_property_read_u32(pdev->dev.of_node, "clk-1xp",
				  &mtk_fcie_host->clk_1xp))
		dev_dbg(&pdev->dev, "clk-1xp: %x\n",
			mtk_fcie_host->clk_1xp);

	if (!of_property_read_u32(pdev->dev.of_node, "clk-2xp",
				  &mtk_fcie_host->clk_2xp))
		dev_dbg(&pdev->dev, "clk-2xp: %x\n",
			mtk_fcie_host->clk_2xp);

	if (!of_property_read_u32(pdev->dev.of_node, "v_emmc_pll",
				  &mtk_fcie_host->v_emmc_pll))
		dev_dbg(&pdev->dev, "v_emmc_pll: %x\n",
			mtk_fcie_host->v_emmc_pll);

	mtk_fcie_host->dev_comp = of_device_get_match_data(&pdev->dev);

	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));

	// [FIXME]->
	mmc_host->f_min	= CLK_400KHz;
	mmc_host->f_max	= CLK_200MHz;
	mmc_host->ocr_avail = MMC_VDD_27_28 | MMC_VDD_28_29 |
					MMC_VDD_29_30 | MMC_VDD_30_31 |
					MMC_VDD_31_32 | MMC_VDD_32_33 |
					MMC_VDD_33_34 | MMC_VDD_165_195;

	mmc_host->max_blk_count	= BIT_SD_JOB_BLK_CNT_MASK;
	mmc_host->max_blk_size	= 512;
	 //could be optimized
	mmc_host->max_req_size	=
		mmc_host->max_blk_count * mmc_host->max_blk_size;

	mmc_host->max_seg_size	= HOST_MAX_SEG_SIZE;

	if (mtk_fcie_host->support_cqe)
		mmc_host->max_segs = HOST_CQE_MAX_SEGS;
	else
		mmc_host->max_segs = HOST_MAX_SEGS;

	//---------------------------------------
	mtk_fcie_host->mmc = mmc_host;
	mtk_fcie_host->dev = &pdev->dev;
	//---------------------------------------
	mmc_host->caps |= MMC_CAP_CMD23;
	mmc_host->caps |= MMC_CAP_WAIT_WHILE_BUSY|MMC_CAP_HW_RESET;

	mmc_host->caps |= MMC_CAP_ERASE;

	mmc_host->caps |= MMC_CAP_DONE_COMPLETE;
	mmc_host->caps2 |= MMC_CAP2_NO_SD|MMC_CAP2_NO_SDIO|MMC_CAP2_FULL_PWR_CYCLE;

	mtk_cqe_host = kzalloc(sizeof(struct mtk_cqe_host), GFP_NOIO);
	if (!mtk_cqe_host) {
		ret = -ENOMEM;
		goto fail_end;
	}

	if (mtk_fcie_host->support_cqe) {
		mtk_cqe_host->cqebase = mtk_fcie_host->pwsbase;

		res = platform_get_resource(pdev, IORESOURCE_MEM, 8);
		mtk_cqe_host->taskdesc = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(mtk_cqe_host->taskdesc)) {
			ret = PTR_ERR(mtk_cqe_host->taskdesc);
			kfree(mtk_cqe_host);
			goto fail_end;
		}

		mmc_host->caps2 |= MMC_CAP2_CQE;
	}
	ret = mtkcqe_init(mtk_cqe_host, mmc_host);
	if (ret)
		goto fail_end;

	mtk_fcie_host->next_data.cookie = 1;

	INIT_DELAYED_WORK(&mtk_fcie_host->req_timeout_work, mtk_fcie_req_timeout);
	INIT_WORK(&mtk_fcie_host->emmc_recovery_work, mtk_fcie_recovery);

	mtk_fcie_host->log_level = 6;


	mtk_fcie_host->adma_desc =
		dma_alloc_coherent(&pdev->dev,
			mmc_host->max_segs*sizeof(struct adma_descriptor),
			&mtk_fcie_host->adma_desc_addr, GFP_NOIO);
	if (!mtk_fcie_host->adma_desc) {
		ret = -ENOMEM;
		goto fail_end;
	}

	platform_set_drvdata(pdev, mmc_host);

	spin_lock_init(&mtk_fcie_host->lock);

	mtk_fcie_host->irq = platform_get_irq(pdev, 0);

	ret = devm_request_threaded_irq(&pdev->dev,
		mtk_fcie_host->irq, mtk_fcie_irq, mtk_fcie_irq_thread,
		IRQF_SHARED|IRQF_ONESHOT, DRIVER_NAME, mtk_fcie_host);

	if (ret) {
		dev_err(&pdev->dev,
			"request_irq fail\n");
		goto fail_end;
	}

	// --------------------------------
	mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);
	mtk_fcie_enable(mtk_fcie_host);
	mtk_fcie_unlock_host((u8 *)__func__, mtk_fcie_host);

	mmc_add_host(mmc_host);

	// For getting and showing device attributes from/to user space.
	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_fcie_attr_grp);

	mtk_fcie_host->emmc_monitor_enable = 0;
	mtk_fcie_host->emmc_read_log_enable = 0;
	mtk_fcie_host->emmc_write_log_enable = 0;
	memset(&mtk_fcie_host->emmc_rw_speed, 0,
		sizeof(struct mtk_fcie_emmc_rw_speed));
	mtk_fcie_host->emmc_rw_speed.int_tm_yday = 0xffff;

	#if defined(CONFIG_MMC_MTK_ROOT_WAIT) && CONFIG_MMC_MTK_ROOT_WAIT
	if ((mmc_host->caps2 &
		(MMC_CAP2_HS400_ES | MMC_CAP2_HS400 | MMC_CAP2_HS200)) ||
		(mmc_host->caps &
		(MMC_CAP_3_3V_DDR | MMC_CAP_1_8V_DDR | MMC_CAP_1_2V_DDR))) {

		init_completion(&mtk_fcie_host->init_mmc_done);
		wait_for_completion(&mtk_fcie_host->init_mmc_done);
	}
	#endif



	return ret;
fail_end:
	if (mtk_fcie_host->adma_desc)
		dma_free_coherent(&pdev->dev,
			mmc_host->max_segs*sizeof(struct adma_descriptor),
			(void *)mtk_fcie_host->adma_desc,
			mtk_fcie_host->adma_desc_addr);
	kfree(mtk_fcie_host->emmc_drv);
	if (mmc_host)
		mmc_free_host(mmc_host);

	return ret;
}

static int __exit mtk_fcie_remove(struct platform_device *pdev)
{
	/* Define Local Variables */
	struct mmc_host *mmc_host;
	struct mtk_fcie_host *mtk_fcie_host;
	int ret = 0;

	if (!pdev) {
		ret = -EINVAL;
		return ret;
	}

	mmc_host = platform_get_drvdata(pdev);

	if (!mmc_host) {
		ret = -1;
		return ret;
	}

	mtk_fcie_host = mmc_priv(mmc_host);

	mtk_fcie_lock_host((u8 *)__func__, mtk_fcie_host);

	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL, 1, "eMMC Err: remove +\n");

	mmc_remove_host(mmc_host);

	mtk_fcie_disable(mtk_fcie_host);



	free_irq(mtk_fcie_host->irq, mtk_fcie_host);
	dma_free_coherent(&pdev->dev,
		mmc_host->max_segs*sizeof(struct adma_descriptor),
		(void *)mtk_fcie_host->adma_desc,
		mtk_fcie_host->adma_desc_addr);

	kfree(mtk_fcie_host->emmc_drv);
	mmc_free_host(mmc_host);

	platform_set_drvdata(pdev, NULL);

	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL, 1, "eMMC Err: remove -\n");

	mtk_fcie_unlock_host((u8 *)__func__, mtk_fcie_host);

	return ret;
}

#ifdef CONFIG_PM
static void emmcpll_pd(struct mtk_fcie_host *mtk_fcie_host)
{
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x4), 0x002e);
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x1), 0x3176);
	REG_W(REG_ADDR(mtk_fcie_host->emmcpllbase, 0x3), 0xffff);
}

static int mtk_fcie_suspend(struct device *dev)
{
	/* Define Local Variables */
	struct mmc_host *mmc_host = dev_get_drvdata(dev);
	struct mtk_fcie_host *mtk_fcie_host = NULL;

	if (mmc_host) {
		mtk_fcie_host = mmc_priv(mmc_host);
		//To avoid leakage problem of Hynix eMMC
		mtk_fcie_wait_d0_high(mtk_fcie_host, TIME_WAIT_DAT0_HIGH);

		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL, 0, "eMMC, suspend +\n");
		mtk_fcie_host->emmc_drv->old_pll_clk_param = 0xffff;
		mtk_fcie_host->emmc_drv->old_pll_dll_clk_param = 0xffff;
		mtk_fcie_host->emmc_drv->tune_overtone = 1;
		clk_disable_unprepare(mtk_fcie_host->fcie_syn_clk);
		clk_disable_unprepare(mtk_fcie_host->fcie_top_clk);
		if (mtk_fcie_host->smi_fcie2fcie)
			clk_disable_unprepare(mtk_fcie_host->smi_fcie2fcie);

		emmcpll_pd(mtk_fcie_host);

		if (mtk_fcie_host->support_cqe)
			mtkcqe_suspend(mmc_host);

		EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL, 0, "eMMC, suspend -\n");
	}

	return 0;
}

static int mtk_fcie_resume(struct device *dev)
{
	struct mmc_host *mmc_host	= dev_get_drvdata(dev);
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mmc_host);
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;
	u32 err;
	int ret = 0;



	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL, 0, "eMMC, resume +\n");

	mtk_fcie_host->emmc_drv->drv_flag = 0;
	#if defined(CONFIG_MMC_MTK_ROOT_WAIT) && CONFIG_MMC_MTK_ROOT_WAIT
	mtk_fcie_host->emmc_drv->drv_flag |= DRV_FLAG_RESUME;
	#endif
	mtk_fcie_host->emmc_drv->drv_flag |= DRV_FLAG_INIT_DONE;


	ret = clk_prepare_enable(mtk_fcie_host->fcie_syn_clk);
	if (ret)
		EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_ERROR, 1,
		"eMMC Err: fcie_syn_clk: %d\n", ret);

	ret = clk_prepare_enable(mtk_fcie_host->fcie_top_clk);
	if (ret)
		EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_ERROR, 1,
		"eMMC Err: fcie_top_clk: %d\n", ret);


	if (mtk_fcie_host->smi_fcie2fcie) {
		ret = clk_prepare_enable(mtk_fcie_host->smi_fcie2fcie);
		if (ret)
			EMMC_DBG(mtk_fcie_host,
			EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: smi_fcie2fcie: %d\n", ret);
	}

	mtk_fcie_host->emmc_drv->old_pll_clk_param = 0xffff;
	mtk_fcie_host->emmc_drv->old_pll_dll_clk_param = 0xffff;
	mtk_fcie_host->emmc_drv->tune_overtone = 1;

	mtk_fcie_host->emmc_drv->drv_flag |= (DRV_FLAG_DDR_MODE|DRV_FLAG_SPEED_HS400);
	mtk_fcie_clock_setting(EMMC_PLL_CLK_200M, mtk_fcie_host, 200000000);
	mtk_fcie_host->emmc_drv->drv_flag &= ~(DRV_FLAG_DDR_MODE|DRV_FLAG_SPEED_HS400);

	err = mtk_fcie_hw_init(mtk_fcie_host);
	if (err)
		EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL_ERROR, 1,
		"eMMC Err: mtk_fcie_hw_init fail: %Xh\n", err);

	if (mtk_fcie_host->enable_sar5)
		mtk_fcie_prepare_psm_queue(mtk_fcie_host);

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, PWR_SAVE_CTL),
		BIT_POWER_SAVE_MODE|BIT_POWER_SAVE_MODE_INT_EN);

	if (mtk_cqe_host->enabled)
		mtkcqe_prepare_chk_cqe_bus(mtk_fcie_host);

	EMMC_DBG(mtk_fcie_host,
		EMMC_DBG_LVL, 0, "eMMC, resume -\n");

	return 0;
}
#endif	/* End ifdef CONFIG_PM */



static const struct mmc_cqe_ops mtk_cqe_ops = {
	.cqe_enable = mtkcqe_enable,
	.cqe_disable = mtkcqe_disable,
	.cqe_request = mtkcqe_request,
	.cqe_post_req = mtkcqe_post_req,
	.cqe_off = mtkcqe_off,
	.cqe_wait_for_idle = mtkcqe_wait_for_idle,
	.cqe_timeout = mtkcqe_timeout,
	.cqe_recovery_start = mtkcqe_recovery_start,
	.cqe_recovery_finish = mtkcqe_recovery_finish,
};


static void mtkcqe_prepare_chk_cqe_bus(struct mtk_fcie_host *mtk_fcie_host)
{
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, EMMC_DEBUG_BUS1),
		BIT_DEBUG1_MODE_MSK);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, EMMC_DEBUG_BUS1),
		BIT_DEBUG1_MODE_SET);
	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, TEST_MODE),
		BIT_TEST_MODE_MASK);
	REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, TEST_MODE),
		BIT_CQE_DBUS_MODE);
}


static void mtkcqe_wait_cqebus_idle(struct mtk_fcie_host *mtk_fcie_host)
{
	u16 cnt;

	cnt = 0;
	while (cnt < MAX_CQE_CMD13_TIME) {
		 // reset success
		if ((REG(REG_ADDR(mtk_fcie_host->fciebase, EMMC_DEBUG_BUS0))
				& BIT_DEBUG0_MODE_MSK) == 0)
			break;

		udelay(HW_TIMER_DELAY_1US);
		cnt++;
	}


	if (cnt == MAX_CQE_CMD13_TIME) {
		EMMC_DBG(mtk_fcie_host, 1, 0,
		"eMMC Err: Wait cqe bus idle timeout!\n");
	}


}

static void mtkcqe_fde_open(struct mtk_fcie_host *host)
{
	mtk_fde_reset(host);
	REG_SETBIT(REG_ADDR(host->fdebase, AES_FUN), BIT_AES_AUTO_CONFIG_CMDQ);
}
static void mtkcqe_fde_close(struct mtk_fcie_host *host)
{
	REG_CLRBIT(REG_ADDR(host->fdebase, AES_FUN), BIT_AES_AUTO_CONFIG_CMDQ);
}

static int mtkcqe_fde_func_en(struct mtk_fcie_host *host, u32 lba)
{
	if (host->fdebase == NULL || host->enable_fde == 0)
		return 0;
	//emmc driver reserved area
	if (lba >= EMMC_CIS_BLK_0 &&
		lba < EMMC_CIS_BLK_0 + EMMC_DRV_RESERVED_BLK_CNT)
		return 0;
	//check white list
	if (mtk_fde_check_white_list(host, lba))
		return 0;


	return 1;
}

int mtkcqe_init(struct mtk_cqe_host *mtk_cqe_host, struct mmc_host *mmc_host)
{
	mmc_host->cqe_private = mtk_cqe_host;
	mtk_cqe_host->mmc = mmc_host;

	mtk_cqe_host->num_slots = CQE_NUM_SLOTS;
	mtk_cqe_host->dcmd_slot = CQE_DCMD_SLOT;

	mmc_host->cqe_ops = &mtk_cqe_ops;

	mmc_host->cqe_qdepth = CQE_NUM_SLOTS;
	if (mmc_host->caps2 & MMC_CAP2_CQE_DCMD)
		mmc_host->cqe_qdepth -= 1;

	mtk_cqe_host->slot = devm_kcalloc(mmc_dev(mmc_host), mtk_cqe_host->num_slots,
		sizeof(*mtk_cqe_host->slot), GFP_NOIO);

	if (!mtk_cqe_host->slot)
		return -ENOMEM;

	init_waitqueue_head(&mtk_cqe_host->wait_queue);

	return 0;
}

static void mtkcqe_setup_link_desc(struct mtk_cqe_host *mtk_cqe_host, int tag)
{
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mtk_cqe_host->mmc);
	dma_addr_t dmaaddr;

	memset(&mtk_cqe_host->link_desc_base[tag], 0, mtk_cqe_host->link_desc_len);

	dmaaddr = (dma_addr_t)mtk_cqe_host->trans_desc_dma_base+
		(dma_addr_t)tag*(dma_addr_t)mtk_cqe_host->mmc->max_segs*
		(dma_addr_t)mtk_cqe_host->trans_desc_len;


	dmaaddr -= (dma_addr_t)mtk_fcie_host->miu0_base;

	mtk_cqe_host->link_desc_base[tag].valid = 1;
	mtk_cqe_host->link_desc_base[tag].end = 0;
	mtk_cqe_host->link_desc_base[tag].intr_mode = 0;
	mtk_cqe_host->link_desc_base[tag].act = 6;
	mtk_cqe_host->link_desc_base[tag].fde = 0;
	mtk_cqe_host->link_desc_base[tag].miu_sel = 0;
	mtk_cqe_host->link_desc_base[tag].dmalen =
	mtk_cqe_host->mmc->max_segs*mtk_cqe_host->trans_desc_len;

	mtk_cqe_host->link_desc_base[tag].address1 =
		(u32)dmaaddr;

	if (sizeof(dma_addr_t) == 8)
		mtk_cqe_host->link_desc_base[tag].address2 =
			(u32)(dmaaddr >> 32);

	mtk_cqe_host->link_desc_base[tag].blk_addr = 0;
}

static int mtkcqe_host_alloc_tdl(struct mtk_cqe_host *mtk_cqe_host)
{
	int i;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mtk_cqe_host->mmc);

	mtk_cqe_host->link_desc_len = sizeof(struct transfer_descriptor);
	mtk_cqe_host->link_desc_size = (size_t)mtk_cqe_host->link_desc_len*
	(size_t)mtk_cqe_host->mmc->cqe_qdepth;

	mtk_cqe_host->link_desc_base = dmam_alloc_coherent(
		mmc_dev(mtk_cqe_host->mmc),
		mtk_cqe_host->link_desc_size,
		&mtk_cqe_host->link_desc_dma_base,
		GFP_NOIO);

	if (!mtk_cqe_host->link_desc_base) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
		    "eMMC Err: malloc link_desc_base fail\n");

		return -ENOMEM;
	}

	mtk_cqe_host->trans_desc_len = sizeof(struct transfer_descriptor);
	mtk_cqe_host->trans_desc_size = (size_t)mtk_cqe_host->trans_desc_len*
	(size_t)mtk_cqe_host->mmc->max_segs*(size_t)mtk_cqe_host->mmc->cqe_qdepth;

	mtk_cqe_host->trans_desc_base = dmam_alloc_coherent(
		mmc_dev(mtk_cqe_host->mmc),
		mtk_cqe_host->trans_desc_size,
		&mtk_cqe_host->trans_desc_dma_base,
		GFP_NOIO);

	if (!mtk_cqe_host->trans_desc_base) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
		    "eMMC Err: malloc trans_desc_base fail\n");
		dmam_free_coherent(mmc_dev(mtk_cqe_host->mmc),
		mtk_cqe_host->link_desc_size,
		mtk_cqe_host->link_desc_base,
		mtk_cqe_host->link_desc_dma_base);

		mtk_cqe_host->link_desc_base = NULL;
		mtk_cqe_host->link_desc_dma_base = 0;
		return -ENOMEM;
	}

	for (i = 0; i < mtk_cqe_host->mmc->cqe_qdepth; i++)
		mtkcqe_setup_link_desc(mtk_cqe_host, i);

	return 0;
}

static void __mtkcqe_enable(struct mtk_cqe_host *mtk_cqe_host)
{
	struct mmc_host *mmc_host = mtk_cqe_host->mmc;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mmc_host);
	struct fcie_emmc_driver *emmc_drv = mtk_fcie_host->emmc_drv;
	dma_addr_t dmaaddr;
	unsigned long flags;


	mtk_fcie_cmd_is_ready(mtk_fcie_host);

	dmaaddr = mtk_cqe_host->link_desc_dma_base;
	dmaaddr -= (dma_addr_t)mtk_fcie_host->miu0_base;

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_ADDR_15_0),
		dmaaddr & 0xffff);
		REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_ADDR_31_16),
		(dmaaddr >> 16) & 0xffff);

	if (sizeof(dma_addr_t) == DMA_ADDR_64_BYTE)
		REG_W(REG_ADDR(mtk_fcie_host->fciebase,
			MIU_DMA_ADDR_47_32), dmaaddr >> DMA_47_32_SHIFT);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_LEN_15_0), 0x0010);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIU_DMA_LEN_31_16), 0x0000);

	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_SEND_ST_CFG1_15_0), 0x1000);
	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_SEND_ST_CFG2), mtk_cqe_host->rca);

	if (mmc_host->caps2 & MMC_CAP2_CQE_DCMD)
		REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_CFG),
			BIT_CQE_EN|BIT_DCMD_EN);
	else
		REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_CFG), BIT_CQE_EN);



	REG_SETBIT(REG_ADDR(mtk_cqe_host->cqebase, CQE_CMD45_CHK_EN),
		BIT_AUTO_CMD13_AFTER_CMD45);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE),
		emmc_drv->reg10_mode|BIT_JOB_START_CLR_SEL);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL),
		BIT_SYNC_REG_FOR_CMDQ_EN|BIT_ERR_DET_ON|BIT_ADMA_EN);

	if (mtk_fcie_host->fdebase != NULL && mtk_fcie_host->enable_fde)
		mtkcqe_fde_open(mtk_fcie_host);

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), BIT_ERR_STS);
	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS_EN), BIT_INT_MASK);
	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_SIGNAL_EN), BIT_INT_MASK);
	mmc_host->cqe_on = true;
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);


}

static int mtkcqe_enable(struct mmc_host *mmc_host, struct mmc_card *card)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mmc_host);
	int err;

	if (!card->ext_csd.cmdq_en)
		return -EINVAL;

	if (mtk_cqe_host->enabled)
		return 0;

	mtk_cqe_host->rca = card->rca;

	err = mtkcqe_host_alloc_tdl(mtk_cqe_host);
	if (err)
		return err;

	__mtkcqe_enable(mtk_cqe_host);
	mtkcqe_prepare_chk_cqe_bus(mtk_fcie_host);

	mtk_cqe_host->enabled = true;

	EMMC_DBG(mtk_fcie_host,
		0, 0, "enable eMMC cqe\n");


	return 0;
}

static void mtkcqe_off(struct mmc_host *mmc_host)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mmc_host);
	unsigned long flags;

	if (!mtk_cqe_host->enabled || !mmc_host->cqe_on)
		return;

	mtkcqe_wait_cqebus_idle(mtk_fcie_host);

	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_CFG), 0);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), 0);

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL),
		BIT_SYNC_REG_FOR_CMDQ_EN|BIT_ERR_DET_ON|BIT_ADMA_EN);


	if (mtk_fcie_host->fdebase != NULL && mtk_fcie_host->enable_fde)
		mtkcqe_fde_close(mtk_fcie_host);

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), 0);
	mmc_host->cqe_on = false;
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

}

static void __mtkcqe_disable(struct mmc_host *mmc_host)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mmc_host);
	unsigned long flags;



	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_CFG), 0);

	REG_W(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), 0);

	REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_CTRL),
		BIT_SYNC_REG_FOR_CMDQ_EN|BIT_ERR_DET_ON|BIT_ADMA_EN);


	if (mtk_fcie_host->fdebase != NULL && mtk_fcie_host->enable_fde)
		mtkcqe_fde_close(mtk_fcie_host);

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	REG_W(REG_ADDR(mtk_fcie_host->fciebase, MIE_INT_EN), 0);
	mtk_cqe_host->mmc->cqe_on = false;
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);


}

static int mtkcqe_suspend(struct mmc_host *mmc_host)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;

	if (mtk_cqe_host->enabled)
		__mtkcqe_disable(mmc_host);

	return 0;
}

static void mtkcqe_disable(struct mmc_host *mmc_host)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;

	if (!mtk_cqe_host->enabled)
		return;

	mtkcqe_off(mmc_host);

	dmam_free_coherent(mmc_dev(mmc_host), mtk_cqe_host->link_desc_size,
		mtk_cqe_host->link_desc_base,
		mtk_cqe_host->link_desc_dma_base);

	dmam_free_coherent(mmc_dev(mmc_host), mtk_cqe_host->trans_desc_size,
		mtk_cqe_host->trans_desc_base,
		mtk_cqe_host->trans_desc_dma_base);

	mtk_cqe_host->link_desc_base = NULL;
	mtk_cqe_host->trans_desc_base = NULL;

	mtk_cqe_host->enabled = false;
}

static void mtkcqe_prep_task_desc(struct mmc_request *mrq,
	struct mtk_cqe_host *mtk_cqe_host,
	bool intr, int tag)
{
	u32 req_flags = mrq->data->flags;
	u64 task_descriptor = 0;
	u16 task_desc_val = 0;
	u8 i;
	struct task_descriptor task_desc;

	task_desc.valid = 1;
	task_desc.end = 1;
	task_desc.intr_mode = intr;
	task_desc.act = 5;
	task_desc.force_prg = (req_flags & MMC_DATA_FORCED_PRG)?1:0;
	task_desc.context = 0;
	task_desc.tag_req = (req_flags & MMC_DATA_DAT_TAG)?1:0;
	task_desc.dir = (req_flags & MMC_DATA_READ)?1:0;
	task_desc.priority = (req_flags & MMC_DATA_PRIO)?1:0;
	task_desc.qbr = (req_flags & MMC_DATA_QBR)?1:0;
	task_desc.rel_write = (req_flags & MMC_DATA_REL_WR)?1:0;
	task_desc.blk_cnt = (u16)(mrq->data->blocks);
	task_desc.blk_addr = mrq->data->blk_addr;

	task_descriptor = MTK_CQE_VALID(task_desc.valid)|
		MTK_CQE_END(task_desc.end)|
		MTK_CQE_INT(task_desc.intr_mode)|
		MTK_CQE_ACT(task_desc.act)|
		MTK_CQE_FORCED_PROG(task_desc.force_prg)|
		MTK_CQE_CONTEXT(task_desc.context)|
		MTK_CQE_DATA_TAG(task_desc.tag_req)|
		MTK_CQE_DATA_DIR(task_desc.dir)|
		MTK_CQE_PRIORITY(task_desc.priority)|
		MTK_CQE_QBR(task_desc.qbr)|
		MTK_CQE_REL_WRITE(task_desc.rel_write)|
		MTK_CQE_BLK_COUNT(task_desc.blk_cnt)|
		MTK_CQE_BLK_ADDR((u64)task_desc.blk_addr);

	for (i = 0; i < 4; i++) {
		task_desc_val =  (task_descriptor >> (i*16)) & 0xFFFF;
		REG_W(REG_ADDR(mtk_cqe_host->taskdesc, tag*4+i),
			task_desc_val);
	}
}

static void mtkcqe_prep_link_desc(struct mmc_request *mrq,
	struct mtk_cqe_host *mtk_cqe_host, int tag)
{
	struct mmc_data  *data = mrq->data;

	mtk_cqe_host->link_desc_base[tag].dir = (data->flags & MMC_DATA_WRITE)?1:0;
}

static int mtkcqe_dma_map(struct mmc_host *mmc_host, struct mmc_request *mrq)
{
	int sg_count;
	struct mmc_data *data = mrq->data;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mmc_host);

	if (!data)
		return -EINVAL;

	sg_count = dma_map_sg(mmc_dev(mmc_host), data->sg,
					data->sg_len,
					(data->flags & MMC_DATA_WRITE) ?
					DMA_TO_DEVICE : DMA_FROM_DEVICE);

	if (!sg_count) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: sg-len: %d\n",
			data->sg_len);

		return -ENOMEM;
	}

	return sg_count;
}

static int mtkcqe_prep_trans_desc(struct mmc_request *mrq,
	struct mtk_cqe_host *mtk_cqe_host, int tag)
{
	struct mmc_data  *data = mrq->data;
	struct scatterlist	*sg = data->sg;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mtk_cqe_host->mmc);
	int i, sg_count;
	u32 dmalen, trans_desc_offset, current_blk_addr;
	dma_addr_t dmaaddr;

	sg_count = mtkcqe_dma_map(mrq->host, mrq);
	if (sg_count < 0) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
		"eMMC Err: unable to map sg lists\n");

		return sg_count;
	}

	current_blk_addr = data->blk_addr;

	for (i = 0; i < data->sg_len; i++) {
		dmaaddr = sg_dma_address(sg);
		dmalen = sg_dma_len(sg);
		dmaaddr -= (dma_addr_t)mtk_fcie_host->miu0_base;

		trans_desc_offset = tag*mtk_cqe_host->mmc->max_segs+i;

		mtk_cqe_host->trans_desc_base[trans_desc_offset].valid = 1;

		mtk_cqe_host->trans_desc_base[trans_desc_offset].end = 0;

		mtk_cqe_host->trans_desc_base[trans_desc_offset].intr_mode = 0;

		mtk_cqe_host->trans_desc_base[trans_desc_offset].act = 4;

		mtk_cqe_host->trans_desc_base[trans_desc_offset].dir =
			(data->flags & MMC_DATA_WRITE)?1:0;

		if (mtkcqe_fde_func_en(mtk_fcie_host, current_blk_addr))
			mtk_cqe_host->trans_desc_base[trans_desc_offset].fde = 1;
		else
			mtk_cqe_host->trans_desc_base[trans_desc_offset].fde = 0;

		mtk_cqe_host->trans_desc_base[trans_desc_offset].miu_sel = 0;

		mtk_cqe_host->trans_desc_base[trans_desc_offset].dmalen
			= dmalen & 0xffff;

		mtk_cqe_host->trans_desc_base[trans_desc_offset].address1
			= (u32)dmaaddr;

		if (sizeof(dma_addr_t) == 8)
			mtk_cqe_host->trans_desc_base[trans_desc_offset].address2
			= (u32)(dmaaddr >> 32);

		mtk_cqe_host->trans_desc_base[trans_desc_offset].blk_addr = current_blk_addr;

		current_blk_addr += (dmalen/data->blksz);
		sg = sg_next(sg);
	}

	trans_desc_offset =
		tag*mtk_cqe_host->mmc->max_segs+data->sg_len-1;
	mtk_cqe_host->trans_desc_base[trans_desc_offset].end = 1;

	return 0;
}

static void mtkcqe_prep_dcmd_desc(struct mmc_request *mrq, struct mtk_cqe_host *mtk_cqe_host)
{
	u64 dcmd_descriptor = 0;
	u16 dcmd_desc_val = 0;
	u8 i, timing = 0, resp_type = 0;

	if (!(mrq->cmd->flags & MMC_RSP_PRESENT)) {
		resp_type = 0x0;
		timing = 0x1;
	} else {
		if (mrq->cmd->flags & MMC_RSP_R1B) {
			resp_type = 0x3;

			timing = 0x0;
		} else {
			resp_type = 0x2;
			timing = 0x1;
		}
	}

	dcmd_descriptor = MTK_CQE_VALID(1)|
		MTK_CQE_END(1)|
		MTK_CQE_INT(1)|
		MTK_CQE_ACT(5)|
		MTK_CQE_QBR(1)|
		MTK_CQE_CMD_INDEX(mrq->cmd->opcode)|
		MTK_CQE_CMD_TIMING(timing)|
		MTK_CQE_RESP_TYPE(resp_type)|
		MTK_CQE_CMD_ARG((u64)mrq->cmd->arg);

	for (i = 0; i < 4; i++) {
		dcmd_desc_val = (dcmd_descriptor >> (i*16)) & 0xFFFF;
		REG_W(REG_ADDR(mtk_cqe_host->taskdesc,
		mtk_cqe_host->dcmd_slot*4+i), dcmd_desc_val);
	}
}

static void mtkcqe_post_req(struct mmc_host *mmc_host, struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;

	if (data) {
		dma_unmap_sg(mmc_dev(mmc_host), data->sg, data->sg_len,
			(data->flags & MMC_DATA_READ) ?
			DMA_FROM_DEVICE : DMA_TO_DEVICE);
	}
}

static int mtkcqe_tag(struct mmc_request *mrq)
{
	return mrq->cmd ? CQE_DCMD_SLOT : mrq->tag;
}
//check start
static int mtkcqe_request(struct mmc_host *mmc_host, struct mmc_request *mrq)
{
	int err = 0;
	int tag = mtkcqe_tag(mrq);
	struct mtk_cqe_host *mtk_cqe_host = NULL;
	struct mtk_fcie_host *mtk_fcie_host = NULL;
	unsigned long flags;
	u32 task_doorbell;

	if (!mmc_host)
		return -EINVAL;

	mtk_cqe_host = mmc_host->cqe_private;
	mtk_fcie_host = mmc_priv(mmc_host);


	if (!mtk_cqe_host->enabled) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: CQE is not enabled\n");
		return -EINVAL;
	}


	if (mtk_fcie_host->cqe_monitor_enable) {
		if (!mtk_cqe_host->qcnt)
			mtk_fcie_host->starttime = ktime_get();
		if (mrq->data) {
			if (mrq->data->flags & MMC_DATA_READ) {
				cqe_record_wr_data_size(mtk_fcie_host,
					MMC_EXECUTE_READ_TASK,
					mrq->data->blocks<<EMMC_SECTOR_512BYTE_BITS);
			} else {
				cqe_record_wr_data_size(mtk_fcie_host,
					MMC_EXECUTE_WRITE_TASK,
					mrq->data->blocks<<EMMC_SECTOR_512BYTE_BITS);
				emmc_check_life(mtk_fcie_host,
					mrq->data->blocks<<EMMC_SECTOR_512BYTE_BITS);
			}
		}
	}

	if (!mmc_host->cqe_on)
		__mtkcqe_enable(mtk_cqe_host);

	if (mrq->data) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_LOW, 0,
			"eMMC Warn: CQE req blk addr:%u, blk cnt:%u, rw:%u, tag:%d\n",
			mrq->data->blk_addr, mrq->data->blocks,
			(mrq->data->flags & MMC_DATA_READ)?0:1, tag);
	} else {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_LOW, 0,
			"eMMC Warn: CQE cmd opcode:%u, arg:%Xh, tag:%d\n",
			mrq->cmd->opcode, mrq->cmd->arg, tag);
	}

	if (mrq->data) {
		mtkcqe_prep_task_desc(mrq, mtk_cqe_host, 1, tag);
		mtkcqe_prep_link_desc(mrq, mtk_cqe_host, tag);
		err = mtkcqe_prep_trans_desc(mrq, mtk_cqe_host, tag);
		if (err) {
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
				"eMMC Err: failed to setup trans desc\n");
			return err;
		}
		/* Make sure descriptors are ready before ringing the doorbell */
		wmb();
	} else {
		mtkcqe_prep_dcmd_desc(mrq, mtk_cqe_host);
	}

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	if (mtk_cqe_host->recovery_halt) {
		err = -EBUSY;
		goto out_unlock;
	}

	if (!mtk_cqe_host->qcnt)
		REG_SETBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE),
			BIT_CLK_EN);

	mtk_cqe_host->slot[tag].mrq = mrq;
	mtk_cqe_host->slot[tag].flags = 0;
	mtk_cqe_host->qcnt += 1;



	//Doorbell
	task_doorbell = 1<<tag;

	if (tag < 16)
		REG_W(REG_ADDR(mtk_cqe_host->cqebase,  CQE_DOORBELL_15_0),
			task_doorbell & 0xffff);
	else
		REG_W(REG_ADDR(mtk_cqe_host->cqebase,  CQE_DOORBELL_31_16),
			task_doorbell>>16);

out_unlock:
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

	if (err)
		mtkcqe_post_req(mmc_host, mrq);


	return err;
}

static void mtkcqe_recovery_needed(struct mmc_host *mmc_host, struct mmc_request *mrq, bool notify)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;


	if (!mtk_cqe_host->recovery_halt) {
		mtk_cqe_host->recovery_halt = true;

		wake_up(&mtk_cqe_host->wait_queue);

		if (notify && mrq->recovery_notifier)
			mrq->recovery_notifier(mrq);
	}
}

static int mtkcqe_error_flags(u16 status)
{
	int error = MTK_CQE_HOST_TIMEOUT;

	if (status & (BIT_DAT_RD_CERR|BIT_DAT_WR_CERR|BIT_CMD_RSP_CERR))
		error = MTK_CQE_HOST_CRC;


	return error;
}

static void mtkcqe_error_irq(struct mtk_fcie_host *mtk_fcie_host, u16 cqe_status, u16 events)
{
	struct mtk_cqe_host *mtk_cqe_host = NULL;
	struct mtk_cqe_slot *slot = NULL;
	struct mmc_host *mmc_host = NULL;
	u16 status;
	int tag = 0;

	if (!mtk_fcie_host)
		return;

	mtk_cqe_host = mtk_fcie_host->mmc->cqe_private;
	mmc_host = mtk_fcie_host->mmc;


	status = REG(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS));

	if (events & BIT_ERR_STS) {
		REG_W1C(REG_ADDR(mtk_fcie_host->fciebase, SD_STATUS), status);
		REG_W1C(REG_ADDR(mtk_fcie_host->fciebase, MIE_EVENT), events);
	}

	EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
		"eMMC Err: CQE INTR ST:0x%04X, ST:0x%04X\n",
		cqe_status, status);


	if (mtk_cqe_host->recovery_halt)
		return;

	/*
	 * The only way to guarantee forward progress is to mark at
	 * least one task in error, so if none is indicated, pick one.
	 */
	for (tag = 0; tag < mtk_cqe_host->num_slots; tag++) {
		slot = &mtk_cqe_host->slot[tag];
		if (!slot->mrq)
			continue;
		slot->flags = mtkcqe_error_flags(status);
		mtkcqe_recovery_needed(mmc_host, slot->mrq, true);
		break;
	}

}

static void mtkcqe_finish_mrq(struct mmc_host *mmc_host, int tag)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;
	struct mtk_cqe_slot *slot = &mtk_cqe_host->slot[tag];
	struct mmc_request *mrq = slot->mrq;
	struct mmc_data *data = 0;

	if (!mrq)
		return;

	/* No completions allowed during recovery */
	if (mtk_cqe_host->recovery_halt) {
		slot->flags |= MTK_CQE_COMPLETED;
		return;
	}
	slot->mrq = NULL;
	mtk_cqe_host->qcnt -= 1;
	data = mrq->data;
	if (data) {
		if (data->error)
			data->bytes_xfered = 0;
		else
			data->bytes_xfered = data->blksz * data->blocks;
	}
	mmc_cqe_request_done(mmc_host, mrq);
}

static void mtkcqe_irq_err_handle(struct mtk_fcie_host *mtk_fcie_host)
{
	struct mtk_cqe_host *mtk_cqe_host = mtk_fcie_host->mmc->cqe_private;



	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS_EN), 0);
	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_SIGNAL_EN), 0);
	_mtk_fcie_reset(mtk_fcie_host);
	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS_EN), BIT_INT_MASK);
	REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_SIGNAL_EN), BIT_INT_MASK);
}

static void mtkcqe_chk_tcc_tcn(struct mtk_fcie_host *mtk_fcie_host,
	u16 task_comp_notify_15_0,
	u16 task_comp_notify_31_16,
	u32 task_comp_notify,
	u16 cqis)
{
	struct mtk_cqe_host *mtk_cqe_host = mtk_fcie_host->mmc->cqe_private;
	struct mmc_host *mmc_host =  mtk_fcie_host->mmc;
	bool cqe_recovery_needed = false;
	struct mtk_cqe_slot *slot = NULL;
	int i;


	//W1C TCN fail
		if ((REG(REG_ADDR(mtk_cqe_host->cqebase, CQE_TASK_COM_NOTIFY_15_0)) &
		task_comp_notify_15_0) ||
		(REG(REG_ADDR(mtk_cqe_host->cqebase, CQE_TASK_COM_NOTIFY_31_16)) &
		task_comp_notify_31_16)) {
			cqe_recovery_needed = true;
			#if EMMC_IRQ_DEBUG
			EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1, "WIC TCN fail\n");
			#endif
	}

	//W1C TCC fail
	if (mtk_cqe_host->qcnt > 0 && !task_comp_notify && (cqis & BIT_INT_TCC)) {
		cqe_recovery_needed = true;
		#if EMMC_IRQ_DEBUG
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1, "WIC TCC fail\n");
		#endif
	}

	if (cqe_recovery_needed == true) {
		//Disable cqe intr enable
		REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS_EN), 0);
		REG_W(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_SIGNAL_EN), 0);

		for (i = 0; i < mtk_cqe_host->num_slots; i++) {
			slot = &mtk_cqe_host->slot[i];
			if (!slot->mrq)
				continue;
			slot->flags = MTK_CQE_HOST_TIMEOUT;
			mtkcqe_recovery_needed(mmc_host, slot->mrq, true);
			break;
		}
	}
}

static void mtkcqe_irq(struct mtk_fcie_host *mtk_fcie_host, u16 events, u16 cqe_status)
{
	unsigned long tag = 0, task_comp_notify = 0;
	u16 task_comp_notify_15_0 = 0, task_comp_notify_31_16 = 0;
	struct mmc_host *mmc_host =  mtk_fcie_host->mmc;
	struct mtk_cqe_host *mtk_cqe_host = mtk_fcie_host->mmc->cqe_private;
	u16 cqis = 0;
	unsigned long flags;

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	if ((cqe_status & BIT_INT_RED) || (events & BIT_ERR_STS))
		mtkcqe_error_irq(mtk_fcie_host, cqe_status, events);

	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

	if (cqe_status & BIT_INT_TCC) {
		cqis = REG(REG_ADDR(mtk_cqe_host->cqebase, CQE_INT_STATUS));
		task_comp_notify_15_0  =
			REG(REG_ADDR(mtk_cqe_host->cqebase, CQE_TASK_COM_NOTIFY_15_0));
		task_comp_notify_31_16 =
			REG(REG_ADDR(mtk_cqe_host->cqebase, CQE_TASK_COM_NOTIFY_31_16));

		REG_W1C(REG_ADDR(mtk_cqe_host->cqebase, CQE_TASK_COM_NOTIFY_15_0),
			task_comp_notify_15_0);
		REG_W1C(REG_ADDR(mtk_cqe_host->cqebase, CQE_TASK_COM_NOTIFY_31_16),
			task_comp_notify_31_16);

		task_comp_notify =
			(unsigned long)(task_comp_notify_31_16<<BIT_TASK_COM_NOTIFY_SHIFT)|
			(unsigned long)task_comp_notify_15_0;


		spin_lock_irqsave(&mtk_fcie_host->lock, flags);

		if (!mtk_cqe_host->recovery_halt) {
			mtkcqe_chk_tcc_tcn(mtk_fcie_host,
			task_comp_notify_15_0,
			task_comp_notify_31_16,
			(u32)task_comp_notify,
			cqis);
		}

		for_each_set_bit(tag, &task_comp_notify, mtk_cqe_host->num_slots) {
			mtkcqe_finish_mrq(mtk_cqe_host->mmc, (int)tag);
		}

		if (!mtk_cqe_host->qcnt && mmc_host->cqe_on) {
			mtkcqe_wait_cqebus_idle(mtk_fcie_host);
			REG_CLRBIT(REG_ADDR(mtk_fcie_host->fciebase, SD_MODE), BIT_CLK_EN);
		}

		if (mtk_cqe_host->waiting_for_idle
			&& !mtk_cqe_host->qcnt) {
			mtk_cqe_host->waiting_for_idle = false;
			wake_up(&mtk_cqe_host->wait_queue);
		}

		if (mtk_fcie_host->cqe_monitor_enable) {
			cqe_record_wr_time(mtk_fcie_host);
			if (mtk_cqe_host->qcnt)
				mtk_fcie_host->starttime = ktime_get();
		}

		spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);
	}
}

static bool mtkcqe_is_idle(struct mtk_cqe_host *mtk_cqe_host, int *ret)
{
	bool is_idle;
	unsigned long flags;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mtk_cqe_host->mmc);

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	is_idle = !mtk_cqe_host->qcnt || mtk_cqe_host->recovery_halt;
	*ret = mtk_cqe_host->recovery_halt ? -EBUSY : 0;
	mtk_cqe_host->waiting_for_idle = !is_idle;
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

	return is_idle;
}

static int mtkcqe_wait_for_idle(struct mmc_host *mmc_host)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mmc_host);
	int ret;

	if (wait_event_timeout(mtk_cqe_host->wait_queue, mtkcqe_is_idle(mtk_cqe_host, &ret),
		usecs_to_jiffies(EMMC_CQE_WAIT_IDLE_TIME)) == 0) {

		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: CQE timeout\n");

		mtk_fcie_reg_dump(mtk_fcie_host);
	}

	return ret;
}

static bool mtkcqe_timeout(struct mmc_host *mmc_host,
	struct mmc_request *mrq, bool *recovery_needed)
{
	struct mtk_cqe_host *mtk_cqe_host = NULL;
	int tag = mtkcqe_tag(mrq);
	struct mtk_cqe_slot *slot = NULL;
	struct mtk_fcie_host *mtk_fcie_host = mmc_priv(mmc_host);
	unsigned long flags;
	bool timed_out;

	mtk_cqe_host =
		(struct mtk_cqe_host *)mmc_host->cqe_private;

	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	slot = &mtk_cqe_host->slot[tag];
	timed_out = slot->mrq == mrq;
	if (timed_out) {
		slot->flags |= MTK_CQE_EXTERNAL_TIMEOUT;
		mtkcqe_recovery_needed(mmc_host, mrq, false);
		*recovery_needed = mtk_cqe_host->recovery_halt;
	}
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);

	if (timed_out) {
		EMMC_DBG(mtk_fcie_host, EMMC_DBG_LVL_ERROR, 1,
			"eMMC Err: CQE timeout\n");

		mtk_fcie_reg_dump(mtk_fcie_host);
	}
	return timed_out;
}

static void mtkcqe_recovery_start(struct mmc_host *mmc_host)
{
	struct mtk_fcie_host *mtk_fcie_host =  NULL;

	if (!mmc_host)
		return;

	mtk_fcie_host =  mmc_priv(mmc_host);
	if (mtk_fcie_host)
		_mtk_fcie_reset(mtk_fcie_host);

	mtkcqe_off(mmc_host);
	mtk_fcie_err_handler(mtk_fcie_host);
}

static int mtkcqe_error_from_flags(int flags)
{
	if (!flags)
		return 0;

	/* CRC errors might indicate re-tuning so prefer to report that */
	if (flags & MTK_CQE_HOST_CRC)
		return -EILSEQ;

	if (flags & (MTK_CQE_EXTERNAL_TIMEOUT | MTK_CQE_HOST_TIMEOUT))
		return -ETIMEDOUT;

	return -EIO;
}

static void mtkcqe_recover_mrq(struct mtk_cqe_host *mtk_cqe_host, int tag)
{
	struct mtk_cqe_slot *slot = &mtk_cqe_host->slot[tag];
	struct mmc_request *mrq = slot->mrq;
	struct mmc_data *data;

	if (!mrq)
		return;

	slot->mrq = NULL;
	mtk_cqe_host->qcnt -= 1;

	data = mrq->data;
	if (data) {
		data->bytes_xfered = 0;
		data->error = mtkcqe_error_from_flags(slot->flags);
	} else {
		mrq->cmd->error = mtkcqe_error_from_flags(slot->flags);
	}

	mmc_cqe_request_done(mtk_cqe_host->mmc, mrq);
}

static void mtkcqe_recover_mrqs(struct mtk_cqe_host *mtk_cqe_host)
{
	int i;

	for (i = 0; i < mtk_cqe_host->num_slots; i++)
		mtkcqe_recover_mrq(mtk_cqe_host, i);
}

static void mtkcqe_recovery_finish(struct mmc_host *mmc_host)
{
	struct mtk_cqe_host *mtk_cqe_host = mmc_host->cqe_private;
	struct mtk_fcie_host *mtk_fcie_host =  mmc_priv(mmc_host);
	unsigned long flags;


	spin_lock_irqsave(&mtk_fcie_host->lock, flags);
	mtkcqe_recover_mrqs(mtk_cqe_host);
	mtk_cqe_host->qcnt = 0;
	mtk_cqe_host->recovery_halt = false;
	mmc_host->cqe_on = false;
	spin_unlock_irqrestore(&mtk_fcie_host->lock, flags);
}


/******************************************************************************
 * Define Static Global Variables
 ******************************************************************************/

static SIMPLE_DEV_PM_OPS(mtk_fcie_pm_ops, mtk_fcie_suspend, mtk_fcie_resume);

static struct platform_driver mtk_fcie_driver = {
	.probe = mtk_fcie_probe,
	.remove = __exit_p(mtk_fcie_remove),

	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.pm = &mtk_fcie_pm_ops,
#if defined(CONFIG_OF)
		.of_match_table = mtk_fcie_emmc_dt_match,
#endif
	},
};

#ifndef MODULE
static int __init mtk_fcie_driver_init(void)
{
	return platform_driver_register(&mtk_fcie_driver);
}

static void __exit mtk_fcie_driver_exit(void)
{
	platform_driver_unregister(&mtk_fcie_driver);
}

subsys_initcall(mtk_fcie_driver_init);
module_exit(mtk_fcie_driver_exit);
#else
module_platform_driver(mtk_fcie_driver);
#endif
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek Multimedia Card Interface driver");
MODULE_AUTHOR("Mediatek");
