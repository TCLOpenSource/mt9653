// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/io.h>

#include "mtk_cec_hw.h"
#include "mtk_cec_coda.h"

#define RETRY_CNT		3UL
#define FRAMEINTERVAL		7UL
// Free bit time by IC design has a little difference than spec,set to 6 instead of 5
#define BUSFREETIME		6UL
#define RETXINTERVAL		3UL

void mtk_cec_enable(struct mtk_cec_dev *pdev)
{
	u16 regval0, regval1;

	// CEC_12[15:8]: CEC irq clear
	W2BYTEMSK(pdev->reg_base + REG_0048_PM_CEC, 0x1F00, BMASK(15:8));
	W2BYTEMSK(pdev->reg_base + REG_0048_PM_CEC, 0x0000, BMASK(15:8));
	// CEC_13[7:0]: CEC irq mask control
	W2BYTEMSK(pdev->reg_base + REG_004C_PM_CEC, 0, BMASK(7:0));

	// CEC interrupt mask for PM/normal function
	// CEC_30[3]: = 1 interrupt clear type select (Level), clear by itself
	W2BYTEMSK(pdev->reg_base + REG_00C0_PM_CEC,  0x08, BMASK(7:0));

	// CEC_14[1]: clock source from Xtal; [0]: power down CEC controller select
	W2BYTEMSK(pdev->reg_base + REG_0050_PM_CEC, 0x01, BMASK(7:0));
	// CEC_03[12]: standby mode;
	W2BYTEMSK(pdev->reg_base + REG_000C_PM_CEC, 0x0, BIT(12));
	// CEC_00[10:8]: retry times
	pdev->retrycnt = RETRY_CNT;
	W2BYTEMSK(pdev->reg_base + REG_0000_PM_CEC, (0x10 | pdev->retrycnt)<<8, BMASK(15:8));

	// CEC_01[5]: CEC clock no gate; [7]: Enable CEC controller
	W2BYTEMSK(pdev->reg_base + REG_0004_PM_CEC, 0x80, BMASK(7:0));
	// CEC_01[15:8]: CNT1=ReTxInterval; CNT2=BusFreeTime;
	W2BYTEMSK(pdev->reg_base + REG_0004_PM_CEC,
			((BUSFREETIME<<4)|(RETXINTERVAL))<<8, BMASK(15:8));
	// CEC_02[3:0]: CNT3=FrameInterval;
	W2BYTEMSK(pdev->reg_base + REG_0008_PM_CEC, FRAMEINTERVAL, BMASK(3:0));
	// CEC_02[7:4]:logical address TV
	W2BYTEMSK(pdev->reg_base + REG_0008_PM_CEC,
			(pdev->adap->log_addrs.log_addr[0])<<4, BMASK(7:4));

	// reg_val0=(u32XTAL_CLK_Hz%100000l)*0.00016+0.5;
	regval0 = ((pdev->xtal%100000UL)*160+500000UL)/1000000UL;
	// CEC_02[7:0]: CEC time unit by Xtal(integer)
	W2BYTEMSK(pdev->reg_base + REG_0008_PM_CEC, (0x78)<<8, BMASK(15:8));

	regval1 = R2BYTE(pdev->reg_base + REG_000C_PM_CEC) & BMASK(7:0);
	// CEC_03[7:0]: CEC time unit by Xtal(fractional)
	W2BYTEMSK(pdev->reg_base + REG_000C_PM_CEC, ((regval1 & 0xF0) | regval0), BMASK(7:0));

	// CEC_11[7:0]: clear CEC status
	W2BYTEMSK(pdev->reg_base + REG_0044_PM_CEC, 0xFF, BMASK(7:0));

	dev_dbg(pdev->dev, "[%s] CEC Init done\n", __func__);
}

void mtk_cec_disable(struct mtk_cec_dev *pdev)
{
	// CEC_01[7]: enable CEC controller
	W2BYTEMSK(pdev->reg_base + REG_0004_PM_CEC, 0, BIT(7));
	dev_dbg(pdev->dev, "[%s] CEC Disable done!!\n", __func__);
}

void mtk_cec_setlogicaladdress(struct mtk_cec_dev *pdev, u8 logicaladdr)
{
	// CEC_02[7:4]: logical address
	W2BYTEMSK(pdev->reg_base + REG_0008_PM_CEC, logicaladdr, BMASK(7:4));
	dev_dbg(pdev->dev, "[%s] Set logical address = %d\n", __func__, logicaladdr);
}

bool mtk_cec_sendframe(struct mtk_cec_dev *pdev, u8 *data, u8 len)
{
	u32 i = 0, count = 0;
	u8 opcode = 0, header = 0, wait_count = 0, res = 0;

	dev_dbg(pdev->dev, "[%s] len = %d\n", __func__, len);
	//for debug, force LA = 0;
	//W2BYTEMSK(pdev->reg_base + REG_CEC_02_L, 0x00, BMASK(7:4)); // set logic addr

	//clear Tx int status
	//W2BYTEMSK(pdev->reg_base + REG_CEC_12_L, 0x0E00, BMASK(15:8));
	//W2BYTEMSK(pdev->reg_base + REG_CEC_12_L, 0x0000, BMASK(15:8));

	header = data[0];
	opcode = data[1];

	// set header and opcode
	// CEC_18[7:0]: reg_tx_data0
	W2BYTEMSK(pdev->reg_base + REG_0060_PM_CEC, header, BMASK(7:0));
	// CEC_18[15:8]: reg_tx_data1
	W2BYTEMSK(pdev->reg_base + REG_0060_PM_CEC, (opcode << 8), BMASK(15:8));

	dev_dbg(pdev->dev, "[%s] header = %x\n", __func__, header);
	dev_dbg(pdev->dev, "[%s] opcode = %x\n", __func__, opcode);

	//fill-in the operand
	if (len > 2) {

		count = 2; //operand start from data[2]
		i = 0;
		do {
			// CEC_19[7:0]: reg_tx_data2 ~ CEC_1F[15:8]: reg_tx_data15
			WBYTE(pdev->reg_base + REG_0064_PM_CEC + i, data[count]);

			dev_dbg(pdev->dev, "[%s] data[%x]:%x\n", __func__, count,
			pdev->msg.msg[count]);
			if ((count % 2) == 0)
				i += 1;
			else
				i += 3;
			count += 1;
		} while (count < len);

	}

	// check CEC idle or not. CEC_05[0]
	if ((R2BYTE(pdev->reg_base + REG_0014_PM_CEC) == 0x0001)) {

	// once REG_CEC_00_L is set, the command will sent. (trigger)
		if ((opcode == 0x00) && (len == 1))
			dev_dbg(pdev->dev, "XXXXXXX polling XXXXX \r\n"); //len = 1, polling
		else
			dev_dbg(pdev->dev, "XXXXXXX not polling XXXXX \r\n");

		// CEC_00[3:0]: tigger sending message
		WBYTE(pdev->reg_base + REG_0000_PM_CEC, (len-1));
		return true;

	} else {
	// CEC is not idle
		dev_dbg(pdev->dev, "CEC busy! \r\n");
		return false;
	}
}

u8 mtk_cec_get_event_status(struct mtk_cec_dev *pdev)
{
	u16 ret = 0;

	// CEC_11[14:8]: reg_cec_event_int
	ret = R2BYTE(pdev->reg_base + REG_0044_PM_CEC) >> 8;
	//dev_info(pdev->dev, "[%s] ret = %x\n", __func__, ret);
	return ret;
}

void mtk_cec_clear_event_status(struct mtk_cec_dev *pdev, u16 clrvalue)
{
	// CEC_12[14:8]: reg_cec_event_int0_clear ~ reg_cec_event_int6_clear
	W2BYTEMSK(pdev->reg_base + REG_0048_PM_CEC, clrvalue, BMASK(14:8));
	W2BYTEMSK(pdev->reg_base + REG_0048_PM_CEC, 0, BMASK(14:8));

	// CEC_11[7:0]: clear RX NACK status
	W2BYTEMSK(pdev->reg_base + REG_0044_PM_CEC, 0xFF, BMASK(7:0));
}

void mtk_cec_get_receive_message(struct mtk_cec_dev *pdev)
{
	u32 i = 0, count = 0;

	// CEC_04[4:0]: reg_cec_rx_len
	pdev->msg.len = ((RBYTE(pdev->reg_base + REG_0010_PM_CEC) & 0x1F) + 1);
	dev_dbg(pdev->dev, "[%s] len: %d\n", __func__, pdev->msg.len);

	if ((pdev->msg.len > 1) && (pdev->msg.len < 17)) {
		//for register access mapping. 01XX23XX45XX....
		count = 0;
		do {
			// CEC_20[7:0]: reg_rx_data0 ~ reg_rx_data15
			pdev->msg.msg[count] = RBYTE((pdev->reg_base + REG_0080_PM_CEC) + i);
			dev_dbg(pdev->dev, "[%s] data[%x]:%x\n", __func__, count,
			pdev->msg.msg[count]);

			if ((count % 2) == 0)
				i += 1;
			else
				i += 3;

			count += 1;
		} while (count < (pdev->msg.len));
	}
	dev_dbg(pdev->dev, "[%s] receive done\n", __func__);
}

void mtk_cec_config_wakeup(struct mtk_cec_dev *pdev)
{
	u8  vendor_id_1 = 0;
	u16 vendor_id_0 = 0;

	// CEC_13[7:0]: mask CEC interrupt in standby mode
	W2BYTEMSK(pdev->reg_base + REG_004C_PM_CEC, 0xFF, BMASK(7:0));

	// CEC_14[1]: clock source from Xtal; [0]: power down CEC controller select
	W2BYTEMSK(pdev->reg_base + REG_0050_PM_CEC, 0x01, BMASK(7:0));
	// CEC_01[5]: CEC clock no gate; [7]: enable CEC controller
	W2BYTEMSK(pdev->reg_base + REG_0004_PM_CEC, 0x00, BMASK(7:0));
	// CEC_01[5]: CEC clock no gate; [7]: enable CEC controller
	W2BYTEMSK(pdev->reg_base + REG_0004_PM_CEC, 0x80, BMASK(7:0));

	// CEC_03[12]: cec_ctrl_sel (0:Standby mode; 1:Sleep mode)
	W2BYTEMSK(pdev->reg_base + REG_000C_PM_CEC, BIT(12), BIT(12));

	// CEC_07[4:0]: enable OP0~2, disable OP3~4; [5]: OP2 second operand enable;
	// CEC_07[11:8]: eanble OP2 operand; [15:12]: disable OP4 length
	W2BYTE(pdev->reg_base + REG_001C_PM_CEC, 0x0427);
	// CEC_08[7:0]: OPCODE0: Image View On; [15:8]: OPCODE1: Text View ON
	W2BYTE(pdev->reg_base + REG_0020_PM_CEC,
			(E_MSG_OTP_TEXT_VIEW_ON << 8)|E_MSG_OTP_IMAGE_VIEW_ON);

	// CEC_09[7:0]: OPCODE2: User Control Pressed
	WBYTE(pdev->reg_base + REG_0024_PM_CEC, E_MSG_UI_PRESS);
	// CEC_0B[15:8]: OPCODE2 operand: Power
	W2BYTEMSK(pdev->reg_base + REG_002C_PM_CEC, (E_MSG_UI_POWER << 8), BMASK(15:8));
	// CEC_0C[7:0]: OPCODE2 operand: Power ON Function
	WBYTE(pdev->reg_base + REG_0030_PM_CEC, E_MSG_UI_POWER_ON_FUN);

	// CEC_0D[10:8]: CEC version 1.4; [15:11]: OP4 is broadcast message
	W2BYTEMSK(pdev->reg_base + REG_0034_PM_CEC, ((0x80 | HDMI_CEC_VERSION) << 8), BMASK(15:8));

	vendor_id_0 = (u16)(pdev->adap->log_addrs.vendor_id & 0xFFFF);
	vendor_id_1 = (u8)((pdev->adap->log_addrs.vendor_id >> 16) & 0xFF);
	// CEC_0F[15:0]: reg_vendor_id
	W2BYTE(pdev->reg_base + REG_003C_PM_CEC, vendor_id_0);
	// CEC_10[7:0]: reg_vendor_id
	WBYTE(pdev->reg_base + REG_0040_PM_CEC, vendor_id_1);

	// ignore messages sent from initiator LA = 0xF when sleep mode
	// [10:8]: Feature abort reason - "Not in correct mode to respond"
	// CEC_10[14:11]: reg_ignor_addr_sw; [15]: reg_ignor_enb_sw
	W2BYTEMSK(pdev->reg_base + REG_0040_PM_CEC,
			((BIT(7)|(0xF<<3)|E_MSG_AR_CANNOTRESPOND) << 8), BMASK(15:8));

	// CEC_0E[15:0]: reg_physical_addr
	W2BYTE(pdev->reg_base + REG_0038_PM_CEC, pdev->adap->phys_addr);
	// CEC_14[15:8]: reg_dev_typ = TV
	W2BYTEMSK(pdev->reg_base + REG_0050_PM_CEC, 0, BMASK(15:8));

	// CEC_11[7:0]: clear CEC wakeup status
	WBYTE(pdev->reg_base + REG_0044_PM_CEC, 0xFF);

	// CEC_12[12:8]: clear RX/TX/RF/LA/NACK status
	W2BYTEMSK(pdev->reg_base + REG_0048_PM_CEC, 0x1F, BMASK(12:8));
	W2BYTEMSK(pdev->reg_base + REG_0048_PM_CEC, 0x00, BMASK(12:8));
}

