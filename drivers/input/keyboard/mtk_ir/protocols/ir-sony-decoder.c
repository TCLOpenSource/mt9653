// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

#define SONY_MAGIC               0x0001

static u8 u8InitFlag_sony = FALSE;
static u32 scancode, prescancode;
static u32 speed;
static u32 mapnum;
static unsigned long  KeyTime, PreKeyTime;
static struct IR_SONY_Spec_s sony;
extern u8 u8FifoDataLen;

/**
 * ir_sony_decode() - Decode one SONY pulse or space
 * @dev:	the struct rc_dev descriptor of the device
 * @duration:	the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int fill_sc_info
(struct IR_SONY_Spec_s *data, struct mtk_ir_dev *dev, struct ir_scancode *sc,
u16 u16Cmd, u16 u16Addr)
{
	int i;

	for (i = 0; i < dev->support_num; i++) {
		if (dev->support_ir[i].eIRType == IR_TYPE_SONY) {
			if ((u16Addr == dev->support_ir[i].u32HeadCode) &&
				(dev->support_ir[i].u8Enable == TRUE)) {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
				sc->scancode = (u16Addr << 16) | u16Cmd;
				sc->scancode_protocol = (1 << IR_TYPE_SONY);
				scancode = sc->scancode;
				speed = dev->support_ir[i].u32IRSpeed;
				dev->map_num = dev->support_ir[i].u32HeadCode;
				mapnum = dev->map_num;
#else
				sc->scancode = (u16Cmd << 8) | 0x00;
				sc->scancode |= (0x01UL << 28);
				sc->scancode_protocol = (1 << IR_TYPE_SONY);
				scancode = sc->scancode;
				speed = dev->support_ir[i].u32IRSpeed;
#endif

				KeyTime = MIRC_Get_System_Time();
                //repeat
                if ((prescancode == scancode) &&
                ((u32)(KeyTime - PreKeyTime) <= SONY_REPEAT_TIMEOUT)) {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
                    IRDBG_INFO("[IR]u8RepeatFlag is 1.\n");
                    dev->raw->u8RepeatFlag = 1;
#else
                    sc->scancode = scancode | 0x01;//repeat
#endif
                } else {
                    IRDBG_INFO("[IR]u8RepeatFlag is 0.\n");
                    dev->raw->u8RepeatFlag = 0;
                }

                PreKeyTime = KeyTime;
                prescancode = scancode;
				memset(data, 0, sizeof(struct IR_SONY_Spec_s));
				return 1;
			}
		}
	}
	return 0;
}

#if 1
#define BIT0 0x0001
static u32 ir_databits_reverse(u32 keydata, u8 bits)
{
    u32 temp = 0;
    u8 i = 0;
    for(i = 0;i<bits;i++)
    {
        temp |= ((keydata>>i) & BIT0);
        temp <<= 1;
    }
    temp >>=1;
    return temp;
}
#endif

static int ir_sony_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
	struct IR_SONY_Spec_s *data = &sony;
	struct ir_scancode *sc = &dev->raw->this_sc;
	struct ir_scancode *prevsc = &dev->raw->prev_sc;
	u16 u16Addr = 0;
	u16 u16Cmd = 0;
	u8 u8Verify = 0;
	u32 u32IrData = 0;

	if (!(dev->enabled_protocols & (1 << IR_TYPE_SONY)))
		return -EINVAL;
	switch (data->eStatus) {

	case STATE_INACTIVE:
		if (!ev.pulse)
			break;
		if (eq_margin(ev.duration, SONY_HEADER_PULSE_LWB, SONY_HEADER_PULSE_UPB)) {
			data->u8BitCount = 0;
			data->u32DataBits = 0;
			IRDBG_INFO("STATE_HEADER_PULSE\n");
			data->eStatus = STATE_HEADER_SPACE;
			return 0;
		}
		break;

	case STATE_HEADER_SPACE:
		if (ev.pulse)
			break;
		if (!eq_margin(ev.duration, SONY_HEADER_SPACE_LWB, SONY_HEADER_SPACE_UPB))
			break;

		data->eStatus = STATE_BIT_PULSE;
		return 0;

	case STATE_BIT_SPACE:
		if (ev.pulse)
			break;
		if (!eq_margin(ev.duration, SONY_BIT_SPACE_LWB, SONY_BIT_SPACE_UPB))
			break;

		data->eStatus = STATE_BIT_PULSE;
		return 0;

	case STATE_BIT_PULSE:
		if (!ev.pulse)
			break;
		data->u32DataBits <<= 1;

		if (eq_margin(ev.duration, SONY_BIT_1_PULSE_LWB, SONY_BIT_1_PULSE_UPB))
			data->u32DataBits |= 1;
		else if (!eq_margin(ev.duration, SONY_BIT_0_PULSE_LWB, SONY_BIT_0_PULSE_UPB))
			break;
		data->u8BitCount++;

		//IRDBG_INFO("[IR]u8FifoDataLen=0x%x u8BitCount=0x%x.\n", u8FifoDataLen, data->u8BitCount);
		if((u8FifoDataLen == 1)&&(data->u8BitCount != 0)) {
			u32IrData = ir_databits_reverse(data->u32DataBits,data->u8BitCount);
			pr_emerg("[IR]Get u32IrData=0x%x.\n", u32IrData);//IRDBG_INFO
			// 5Addr(01)+7Cmd(Data), 8Addr(c4)+7Cmd(Data), ..., 13Addr(081f)+7Cmd(Data)
			if((data->u8BitCount == SONY_12BITS) ||
				(data->u8BitCount == SONY_15BITS) ||
				(data->u8BitCount == SONY_20BITS)) {
				u16Cmd = (u32IrData & 0x7f);
				u16Addr = ((u32IrData>>7) & 0x1fff);
				pr_emerg("[IR]Get Addr=0x%x Cmd(Data)=0x%x.\n", u16Addr, u16Cmd);//IRDBG_INFO
				if (fill_sc_info(data, dev, sc, u16Cmd, u16Addr)) {
					return 1;
	             }
			}
		}
		data->eStatus = STATE_BIT_SPACE;
		return 0;

	default:
		break;
	}

	data->eStatus = STATE_INACTIVE;
	return -EINVAL;
}

static struct ir_decoder_handler sony_handler = {
	.protocols	= (1<<IR_TYPE_SONY),
	.decode		= ir_sony_decode,
};

int sony_decode_init(void)
{
	if (u8InitFlag_sony == FALSE) {
		scancode = 0;
        prescancode = 0;
		mapnum = 0;
		KeyTime = 0;
        PreKeyTime = 0;
		memset(&sony, 0, sizeof(struct IR_SONY_Spec_s));
		MIRC_Decoder_Register(&sony_handler);
		IR_PRINT("[IR Log] SONY Spec Protocol Init\n");
		u8InitFlag_sony = TRUE;
	} else
		IR_PRINT("[IR Log] SONY Spec Protocol Has been Initialized\n");
	return 0;
}

void sony_decode_exit(void)
{
	if (u8InitFlag_sony == TRUE) {
		MIRC_Decoder_UnRegister(&sony_handler);
		u8InitFlag_sony = FALSE;
	}
}
