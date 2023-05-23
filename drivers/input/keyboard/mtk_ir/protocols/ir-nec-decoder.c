// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

#define NEC_MAGIC               0x807F

static u8 u8InitFlag_nec = FALSE;
static u32 scancode;
static u32 speed;
static u32 mapnum;
static unsigned long  KeyTime;
static struct IR_NEC_Spec_s nec;
static int u32ltp_result = FALSE;


int Get_LTP_result(void)
{
	return u32ltp_result;
}
void Set_LTP_INIT(void)
{
	u32ltp_result = FALSE;
}

/**
 * ir_nec_decode() - Decode one NEC pulse or space
 * @dev:	the struct rc_dev descriptor of the device
 * @duration:	the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int fill_sc_info
(struct IR_NEC_Spec_s *data, struct mtk_ir_dev *dev, struct ir_scancode *sc,
u8 u8Cmd, u8 u8CmdInv, u8 u8Addr, u8 u8AddrInv)
{
	int i;

	for (i = 0; i < dev->support_num; i++) {
		if (dev->support_ir[i].eIRType == IR_TYPE_NEC) {
			if ((((u8Addr << 8) | u8AddrInv) ==
				dev->support_ir[i].u32HeadCode) &&
				(dev->support_ir[i].u8Enable == TRUE)) {
				if (u8Cmd == (u8)~u8CmdInv) {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
					sc->scancode = (u8Addr << 16)
					| (u8AddrInv << 8) | u8Cmd;
					sc->scancode_protocol = (1 << IR_TYPE_NEC);
					scancode = sc->scancode;
					speed = dev->support_ir[i].u32IRSpeed;
					dev->map_num =
					dev->support_ir[i].u32HeadCode;
					mapnum = dev->map_num;
					dev->raw->u8RepeatFlag = 0;
#else
					sc->scancode = (u8Cmd << 8) | 0x00;
					if (dev->support_ir[i].u32HeadCode == 0x40DF)
						sc->scancode |= (0x04UL << 28);
					else
						sc->scancode |= (0x01UL << 28);
					sc->scancode_protocol = (1 << IR_TYPE_NEC);
					scancode = sc->scancode;
					speed = dev->support_ir[i].u32IRSpeed;
#endif
					KeyTime = MIRC_Get_System_Time();
					memset(data, 0, sizeof(struct IR_NEC_Spec_s));
					return 1;
				}
			}
		}
	}
	return 0;
}

static int ir_nec_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
	struct IR_NEC_Spec_s *data = &nec;
	struct ir_scancode *sc = &dev->raw->this_sc;
	struct ir_scancode *prevsc = &dev->raw->prev_sc;
	u8 u8Addr = 0;
	u8 u8AddrInv = 0;
	u8 u8Cmd = 0;
	u8 u8CmdInv = 0;
	u8 u8Verify = 0;
	u32 cus_code;
	if (!(dev->enabled_protocols & (1 << IR_TYPE_NEC)))
		return -EINVAL;
	switch (data->eStatus) {

	case STATE_INACTIVE:
		if (!ev.pulse)
			break;
		if (eq_margin(ev.duration, NEC_HEADER_PULSE_LWB, NEC_HEADER_PULSE_UPB)) {
			data->u8BitCount = 0;
			data->eStatus = STATE_HEADER_SPACE;
			//IRDBG_INFO("NEC_HEADER_PULSE\n");
			return 0;
		}
		break;

	case STATE_HEADER_SPACE:
		if (ev.pulse)
			break;
		if (eq_margin(ev.duration, NEC_HEADER_SPACE_LWB,
						NEC_HEADER_SPACE_UPB))	{
			data->eStatus = STATE_BIT_PULSE;
			return 0;
		} else if (eq_margin(ev.duration, NEC_REPEAT_SPACE_LWB,
						NEC_REPEAT_SPACE_UPB)) {
			//repeat
			IRDBG_INFO("[NEC] TIME =%ld\n", (MIRC_Get_System_Time() - KeyTime));
			if (prevsc->scancode_protocol == (1 << IR_TYPE_NEC) &&
				((u32)(MIRC_Get_System_Time() - KeyTime) <= NEC_REPEAT_TIMEOUT)) {
				KeyTime = MIRC_Get_System_Time();
				if (((speed != 0) && (data->u8RepeatTimes >= (speed - 1)))
					|| ((speed == 0) && (data->u8RepeatTimes >= dev->speed))) {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
					sc->scancode = scancode;
					sc->scancode_protocol = (1 << IR_TYPE_NEC);
					dev->map_num = mapnum;
					dev->raw->u8RepeatFlag = 1;
#else
					sc->scancode_protocol = (1 << IR_TYPE_NEC);
					sc->scancode = scancode | 0x01;//repeat
#endif
					data->eStatus = STATE_INACTIVE;
					data->u32DataBits = 0;
					data->u8BitCount = 0;
					return 1;

				}
				//IRDBG_INFO("[NEC]
				//repeattimes =%d \n",data->u8RepeatTimes);
				data->u8RepeatTimes++;
			} else {
				scancode = 0;
				mapnum = NUM_KEYMAP_MAX;
			}
			data->eStatus = STATE_INACTIVE;
			return 0;
		}
		break;

	case STATE_BIT_PULSE:
		if (!ev.pulse)
			break;
		if (!eq_margin(ev.duration, NEC_BIT_PULSE_LWB, NEC_BIT_PULSE_UPB))
			break;

		data->eStatus = STATE_BIT_SPACE;
		return 0;

	case STATE_BIT_SPACE:
		if (ev.pulse)
			break;
		data->u32DataBits <<= 1;

		if (eq_margin(ev.duration, NEC_BIT_1_SPACE_LWB, NEC_BIT_1_SPACE_UPB))
			data->u32DataBits |= 1;
		else if (!eq_margin(ev.duration, NEC_BIT_0_SPACE_LWB, NEC_BIT_0_SPACE_UPB))
			break;
		data->u8BitCount++;

		if (data->u8BitCount == NEC_NBITS) {
			u8Addr	  = bitrev8((data->u32DataBits >> 24) & 0xff);
			u8AddrInv = bitrev8((data->u32DataBits >> 16) & 0xff);
			cus_code = (u8Addr << 8) + u8AddrInv;
			u8Cmd	  = bitrev8((data->u32DataBits >>  8) & 0xff);
			u8CmdInv  = bitrev8((data->u32DataBits >>  0) & 0xff);
			//Check magic
			if (cus_code == NEC_MAGIC) {
				u32ltp_result = TRUE;
			//	 printk(KERN_EMERG"[NEC]Addr=%x AddrInv=%x Cmd=%x CmdInv=%x\n",
			//u8Addr, u8AddrInv, u8Cmd, u8CmdInv);
			}
			IRDBG_INFO("[NEC]Addr=%x AddrInv=%x Cmd=%x CmdInv=%x\n",
			u8Addr, u8AddrInv, u8Cmd, u8CmdInv);

			if (fill_sc_info(data, dev, sc, u8Cmd, u8CmdInv, u8Addr, u8AddrInv))
				return 1;
			 //for white balance ir
			if (MIRC_IsEnable_WhiteBalance() == TRUE) {
				u8Verify = (u8)((u8Addr + u8AddrInv + u8Cmd) & 0xFF);
				if (u8Verify == u8CmdInv) {
					sc->scancode = (u8Addr << 16) | (u8AddrInv << 8) | u8Cmd;
					sc->scancode |= (0x0fUL << 28);
					memset(data, 0, sizeof(struct IR_NEC_Spec_s));
					return 1;
				}
			}
		} else
			data->eStatus = STATE_BIT_PULSE;

		return 0;
	default:
		break;
	}

	data->eStatus = STATE_INACTIVE;
	return -EINVAL;
}

static struct ir_decoder_handler nec_handler = {
	.protocols	= (1<<IR_TYPE_NEC),
	.decode		= ir_nec_decode,
};

int nec_decode_init(void)
{
	if (u8InitFlag_nec == FALSE) {
		scancode = 0;
		mapnum = 0;
		KeyTime = 0;
		memset(&nec, 0, sizeof(struct IR_NEC_Spec_s));
		MIRC_Decoder_Register(&nec_handler);
		IR_PRINT("[IR Log] NEC Spec Protocol Init\n");
		u8InitFlag_nec = TRUE;
	} else
		IR_PRINT("[IR Log] NEC Spec Protocol Has been Initialized\n");
	return 0;
}

void nec_decode_exit(void)
{
	if (u8InitFlag_nec == TRUE) {
		MIRC_Decoder_UnRegister(&nec_handler);
		u8InitFlag_nec = FALSE;
	}
}
