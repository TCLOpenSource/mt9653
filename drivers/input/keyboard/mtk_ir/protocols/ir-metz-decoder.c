// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

static u8 u8InitFlag_metz = FALSE;
static u32 speed = 0;
static IR_Metz_Spec_t metz;
static unsigned long  KeyTime_metz;
static u8 u8PrevKey = 0;
/**
 * ir_metz_decode() - Decode one METZ pulse or space
 * @dev:	the struct rc_dev descriptor of the device
 * @duration:	the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int ir_metz_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
    IR_Metz_Spec_t *data = &metz;
    struct ir_scancode *sc = &dev->raw->this_sc;
    u8 i = 0;
    u8 u8Toggle = 0;
    u8 u8Addr = 0;
    u8 u8AddrInv =0;
    u8 u8Cmd =0;
    u8 u8CmdInv =0;

    if (!(dev->enabled_protocols & (1<<IR_TYPE_METZ)))
        return -EINVAL;
    switch (data->eStatus)
    {

    case STATE_INACTIVE:
        if (!ev.pulse)
            break;
        if (eq_margin(ev.duration,METZ_HEADER_PULSE_LWB,METZ_HEADER_PULSE_UPB))
        {
            data->u8BitCount = 0;
            data->u32DataBits = 0;
            data->eStatus = STATE_HEADER_SPACE;
            //IRDBG_INFO("METZ_HEADER_PULSE\n");
            return 0;
        }
        else
            break;

    case STATE_HEADER_SPACE:
        if (ev.pulse)
            break;
        if (eq_margin(ev.duration,METZ_HEADER_SPACE_LWB,METZ_HEADER_SPACE_UPB))
        {
            data->eStatus = STATE_BIT_PULSE;
            return 0;
        }
        break;

    case STATE_BIT_PULSE:
        if (!ev.pulse)
            break;
        if (!eq_margin(ev.duration,METZ_BIT_PULSE_LWB,METZ_BIT_PULSE_UPB))
            break;

        data->eStatus = STATE_BIT_SPACE;
        return 0;

    case STATE_BIT_SPACE:
        if (ev.pulse)
            break;
        data->u32DataBits <<= 1;

        if (eq_margin(ev.duration,METZ_BIT_1_SPACE_LWB,METZ_BIT_1_SPACE_UPB))
            data->u32DataBits |= 1;
        else if (!eq_margin(ev.duration,METZ_BIT_0_SPACE_LWB,METZ_BIT_0_SPACE_UPB))
            break;
        data->u8BitCount++;

        if (data->u8BitCount == METZ_NBITS)
        {
            u8Toggle  = (data->u32DataBits >> 22) & 0x01;
            u8Addr    = (data->u32DataBits >> 18) & 0x0f;
            u8AddrInv = (data->u32DataBits >> 14) & 0x0f;
            u8Cmd	  = (data->u32DataBits >>  7) & 0x7f;
            u8CmdInv  = (data->u32DataBits >>  0) & 0x7f;

            IRDBG_INFO("[METZ] u8Addr = %x u8AddrInv = %x u8Cmd = %x u8CmdInv = %x\n",u8Addr,u8AddrInv,u8Cmd,u8CmdInv);
            IRDBG_INFO("[METZ] u8Toggle = %d, u8PrevToggle = %d\n",u8Toggle,data->u8PrevToggle);
            for (i= 0;i<dev->support_num;i++)
            {
                if(dev->support_ir[i].eIRType == IR_TYPE_METZ)
                {
                    if((((u8Addr<<8) | u8AddrInv) == dev->support_ir[i].u32HeadCode) && (dev->support_ir[i].u8Enable == TRUE))
                    {
                        data->eStatus = STATE_INACTIVE;
                        data->u8PrevToggle = u8Toggle;
                        if (u8Cmd == (((u8)~u8CmdInv)&0x7F))
                        {
                            if((u8PrevKey == u8Cmd)&&((MIRC_Get_System_Time()-KeyTime_metz) <= METZ_REPEAT_TIMEOUT))
                            {
                                KeyTime_metz = MIRC_Get_System_Time();
                                data->u8RepeatTimes++;
                                if (((speed != 0)&&( data->u8RepeatTimes >= speed))
                                || ((speed == 0)&&(data->u8RepeatTimes >= (dev->speed + 1))))
                                {
                                    dev->raw->u8RepeatFlag = 1;
                                    IRDBG_INFO("[METZ] repeat = 1\n\r");
                                }
                                else
                                {
                                    IRDBG_INFO("[METZ] repeat filter\n\r");
                                    return -EINVAL;
                                }
                            }
                            else
                            {
                                KeyTime_metz = MIRC_Get_System_Time();
                                dev->raw->u8RepeatFlag = 0;
                                data->u8RepeatTimes = 0;
                                IRDBG_INFO("[METZ] repeat = 0\n\r");
                            }
                            u8PrevKey = u8Cmd;
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
                            sc->scancode = (u8Addr<<16) | (u8AddrInv<<8) | u8Cmd;
                            sc->scancode_protocol = (1<<IR_TYPE_METZ);
                            speed = dev->support_ir[i].u32IRSpeed;
                            dev->map_num = dev->support_ir[i].u32HeadCode;
#else
                            sc->scancode = (u8Cmd<<8) |0x00;
                            sc->scancode |= (0x01UL << 28);
                            sc->scancode_protocol = (1<<IR_TYPE_METZ);
                            speed = dev->support_ir[i].u32IRSpeed;
#endif
                            return 1;
                        }
                    }
                }
            }

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

static struct ir_decoder_handler metz_handler = {
    .protocols	= (1<<IR_TYPE_METZ),
    .decode		= ir_metz_decode,
};

int metz_decode_init(void)
{
    if(u8InitFlag_metz == FALSE)
    {
        memset(&metz,0,sizeof(IR_Metz_Spec_t));
        MIRC_Decoder_Register(&metz_handler);
        IR_PRINT("[IR Log] METZ Spec Protocol Init\n");
        u8InitFlag_metz = TRUE;
        KeyTime_metz = 0;
    }
    else
    {
        IR_PRINT("[IR Log] METZ Spec Protocol Has been Initialized\n");
    }
    return 0;
}

void metz_decode_exit(void)
{
    if(u8InitFlag_metz == TRUE)
    {
        MIRC_Decoder_UnRegister(&metz_handler);
        u8InitFlag_metz = FALSE;
    }
}
