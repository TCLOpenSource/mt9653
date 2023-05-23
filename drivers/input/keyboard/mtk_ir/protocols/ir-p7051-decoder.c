// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

/**
 * ir_p7051_decode() - Decode one Panasonic 7051 pulse or space
 * @dev:    the struct rc_dev descriptor of the device
 * @duration:   the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static u8 u8InitFlag_p7051 = FALSE;
static unsigned long  KeyTime;
static IR_P7051_Spec_t P7051;

static int ir_p7051_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
    IR_P7051_Spec_t *data = &P7051;
    struct ir_scancode *sc = &dev->raw->this_sc;
    u8 i = 0;
    u8 u8Addr0 = 0;
    u8 u8Addr1 = 0;
    u8 u8Addr2 = 0;
    u8 u8Addr3 = 0;
    u8 u8Cmd0 =0;
    u8 u8Cmd1 =0;
    u32 u32Addr =0;
    if (!(dev->enabled_protocols & (1<<IR_TYPE_P7051)))
        return -EINVAL;
    switch (data->eStatus)
    {

    case STATE_INACTIVE:
        if (!ev.pulse)
            break;

        if (eq_margin(ev.duration, P7051_HEADER_PULSE_LWB, P7051_HEADER_PULSE_UPB))
        {
            data->u8BitCount = 0;
            data->eStatus = STATE_HEADER_SPACE;
            return 0;
        }
        else
            break;

    case STATE_HEADER_SPACE:
        if (ev.pulse)
            break;

        if (eq_margin(ev.duration, P7051_HEADER_SPACE_LWB, P7051_HEADER_SPACE_UPB))
        {
            data->eStatus = STATE_BIT_PULSE;
            return 0;
        }
        break;

    case STATE_BIT_PULSE:
        if (!ev.pulse)
            break;

        if (!eq_margin(ev.duration, P7051_BIT_PULSE_LWB, P7051_BIT_PULSE_UPB))
            break;
        data->eStatus = STATE_BIT_SPACE;
        return 0;

    case STATE_BIT_SPACE:
        if (ev.pulse)
            break;
        data->u64DataBits <<= 1;
        if (eq_margin(ev.duration, P7051_BIT_1_SPACE_LWB, P7051_BIT_1_SPACE_UPB))
            data->u64DataBits |= 1;
        else if (!eq_margin(ev.duration, P7051_BIT_0_SPACE_LWB, P7051_BIT_0_SPACE_UPB))
            break;
        data->u8BitCount++;
        if (data->u8BitCount == P7051_NBITS)
        {
            u8Addr0   = bitrev8((data->u64DataBits >> 40) & 0xff);
            u8Addr1   = bitrev8((data->u64DataBits >> 32) & 0xff);
            u8Addr2   = bitrev8((data->u64DataBits >> 24) & 0xff);
            u8Addr3   = bitrev8((data->u64DataBits >> 16) & 0xff);
            u8Cmd0    = bitrev8((data->u64DataBits >>  8) & 0xff);
            u8Cmd1    = bitrev8((data->u64DataBits >>  0) & 0xff);
            u32Addr = (u8Addr0<<24)|(u8Addr1<<16)|(u8Addr2<<8)|u8Addr3;
            IRDBG_INFO("u8Cmd0 = %x u8Cmd1 = %x\n",u8Cmd0,u8Cmd1);
            IRDBG_INFO("u8Addr0 = %x u8Addr1 =%x u8Addr2 =%x u8Addr3 =%x \n",u8Addr0,u8Addr1,u8Addr2,u8Addr3);
            IRDBG_INFO("[RCA] TIME =%ld\n",(MIRC_Get_System_Time()-KeyTime));
            KeyTime = MIRC_Get_System_Time();

            for (i= 0;i<dev->support_num;i++)
            {
                if((u32Addr== dev->support_ir[i].u32HeadCode)&&(dev->support_ir[i].eIRType == IR_TYPE_P7051)&&(dev->support_ir[i].u8Enable == TRUE))
                {
                    u32 speed = dev->support_ir[i].u32IRSpeed;
                    if (((speed != 0)&&( data->u8RepeatTimes >= (speed - 1)))
                            || ((speed == 0)&&(data->u8RepeatTimes >= dev->speed)))
                    {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
                        sc->scancode = (u8Cmd0<<8)|u8Cmd1;
                        sc->scancode_protocol = (1<<IR_TYPE_P7051);
                        dev->map_num = NUM_KEYMAP_P7051_STB;
                        dev->raw->u8RepeatFlag = 0;
#else
                        sc->scancode = (u8Cmd0<<8)|u8Cmd1;
                        sc->scancode |= (0x01UL << 28);
                        sc->scancode_protocol = (1<<IR_TYPE_P7051);
#endif
                        memset(data,0,sizeof(IR_P7051_Spec_t));
                        return 1;
                    }
                }
            }
            data->u8RepeatTimes ++;
            IRDBG_INFO("[RCA] repeattimes =%d \n",data->u8RepeatTimes);
            memset(data,0,sizeof(IR_P7051_Spec_t));
        }
        else
            data->eStatus = STATE_BIT_PULSE;

        return 0;
    default:
        break;
    }

    data->eStatus = STATE_INACTIVE;
    return -EINVAL;
}

static struct ir_decoder_handler p7051_handler = {
    .protocols  = (1<<IR_TYPE_P7051),
    .decode     = ir_p7051_decode,
};

int p7051_decode_init(void)
{
    if(u8InitFlag_p7051 == FALSE)
    {
        memset(&P7051,0,sizeof(IR_P7051_Spec_t));
        MIRC_Decoder_Register(&p7051_handler);
        IR_PRINT("[IR Log] Panasonic 7051 Spec protocol init\n");
        u8InitFlag_p7051 = TRUE;
    }
    else
    {
        IR_PRINT("[IR Log] Panasonic 7051 Spec Protocol Has been Initialized\n");
    }
    return 0;
}

void  p7051_decode_exit(void)
{
    if(u8InitFlag_p7051 == TRUE)
    {
        MIRC_Decoder_UnRegister(&p7051_handler);
        u8InitFlag_p7051 = FALSE;
    }
}
