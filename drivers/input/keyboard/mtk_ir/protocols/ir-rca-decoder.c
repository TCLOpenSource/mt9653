// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

static u8 u8InitFlag_rca = FALSE;
static unsigned long  KeyTime;
static IR_RCA_Spec_t rca;
static u8 u8FirstPress;
static u32 u32RcaSpeed;

static u8 bitrev_n(u8 value,u8 n)
{
    u8 i = 0;
    u8 ret = 0;
    for(i = 0;i<n;i++)
    {
        ret = ret<<1;
        if((value>>i)&0x01)
        {
            ret |= 1;
        }
    }
    return ret;
}

/**
 * ir_rca_decode() - Decode one RCA pulse or space
 * @dev:    the struct rc_dev descriptor of the device
 * @duration:   the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int ir_rca_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
    IR_RCA_Spec_t *data = &rca;
    struct ir_scancode *sc = &dev->raw->this_sc;
    u8 i = 0;
    u8 u8Addr = 0;
    u8 u8AddrInv = 0;
    u8 u8Cmd = 0;
    u8 u8CmdInv = 0;
    u8 u8Repeat = 0;


    if (!(dev->enabled_protocols & (1<<IR_TYPE_RCA)))
        return -EINVAL;

    switch (data->eStatus)
    {

    case STATE_INACTIVE:
        if (!ev.pulse)
            break;

        if (eq_margin(ev.duration, RCA_HEADER_PULSE_LWB, RCA_HEADER_PULSE_UPB))
        {
            data->u8BitCount = 0;
            data->u32DataBits = 0;
            data->eStatus = STATE_HEADER_SPACE;
            return 0;
        }
		break;

    case STATE_HEADER_SPACE:
        if (ev.pulse)
            break;
        IRDBG_INFO("[RCA] STATE_HEADER_SPACE \n");
        if (eq_margin(ev.duration, RCA_HEADER_SPACE_LWB, RCA_HEADER_SPACE_UPB)) {
            data->eStatus = STATE_BIT_PULSE;
            return 0;
        }
        break;

    case STATE_BIT_PULSE:
        if (!ev.pulse)
            break;

        if (!eq_margin(ev.duration, RCA_BIT_PULSE_LWB,RCA_BIT_PULSE_UPB))
            break;

        data->eStatus = STATE_BIT_SPACE;
        return 0;

    case STATE_BIT_SPACE:
        if (ev.pulse)
            break;
        data->u32DataBits >>= 1;
        if (eq_margin(ev.duration, RCA_BIT_1_SPACE_LWB, RCA_BIT_1_SPACE_UPB))
            data->u32DataBits |= 0x80000000;
        else if (!eq_margin(ev.duration, RCA_BIT_0_SPACE_LWB, RCA_BIT_0_SPACE_UPB))
            break;
        data->u8BitCount++;
        IRDBG_INFO("[RCA] STATE_BIT_SPACE: data->u8BitCount=%d \n", data->u8BitCount);

        if (data->u8BitCount == RCA_NBITS)
        {
            data->u32DataBits = data->u32DataBits>>8;
            u8AddrInv     = bitrev_n(((data->u32DataBits)&0x0F), 4);
            u8CmdInv      = bitrev_n((((data->u32DataBits)>>4)&0xFF),8);
            u8Addr  = bitrev_n((((data->u32DataBits)>>12)&0x0F),4);
            u8Cmd   = bitrev_n((((data->u32DataBits)>>16)&0xFF),8);

            IRDBG_INFO("[RCA] u8Addr = %x u8AddrInv = %x u8Cmd = %x u8CmdInv = %x\n",u8Addr,u8AddrInv,u8Cmd,u8CmdInv);
            IRDBG_INFO("[RCA] TIME =%ld\n",(MIRC_Get_System_Time()-KeyTime));
            for (i= 0;i<dev->support_num;i++)
            {
                if(((u8Addr<<8 | u8AddrInv) == dev->support_ir[i].u32HeadCode)&&
                    (dev->support_ir[i].eIRType == IR_TYPE_RCA)&&(dev->support_ir[i].u8Enable == TRUE))
                {
                    if (u8Cmd == (u8)~u8CmdInv)
                    {
                        data->eStatus = STATE_INACTIVE;
                        data->u32DataBits = 0;
                        data->u8BitCount = 0;
                        if((MIRC_Get_System_Time()-KeyTime) <= RCA_REPEAT_TIMEOUT)
                        {
                            KeyTime = MIRC_Get_System_Time();
                            if (((u32RcaSpeed != 0)&&( data->u8RepeatTimes >= (u32RcaSpeed - 1)))
                            || ((u32RcaSpeed == 0)&&(data->u8RepeatTimes >= dev->speed)))
                            {
                                u8Repeat = 1;
                            }
                            else
                            {
                                data->u8RepeatTimes ++;
                                IRDBG_INFO("[RCA] repeattimes =%d \n",data->u8RepeatTimes);
                                return -EINVAL;
                            }
                        }
                        else
                        {
                            u32RcaSpeed = dev->support_ir[i].u32IRSpeed;
                            KeyTime = MIRC_Get_System_Time();
                            u8Repeat = 0;
                            data->u8RepeatTimes = 0;
                        }
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
                        sc->scancode = (u8Addr<<16) | (u8AddrInv<<8) | u8Cmd;
                        sc->scancode_protocol = (1<<IR_TYPE_RCA);
                        dev->map_num = dev->support_ir[i].u32HeadCode;
                        dev->raw->u8RepeatFlag = u8Repeat;
                        IRDBG_INFO("[RCA] sc->scancode=0x%x, u32HeadCode=0x%x\n",sc->scancode, dev->map_num);
#else
                        sc->scancode = (u8Cmd<<8) |(u8Repeat&0x01);
                        sc->scancode |= (0x01UL << 28);
                        sc->scancode_protocol = (1<<IR_TYPE_RCA);
#endif
                        return 1;
                    }
                    else
                    {
                       IRDBG_ERR("[RCA] parse keycode = u8Cmd fail\n");
                       memset(data,0,sizeof(IR_RCA_Spec_t));
                       data->eStatus = STATE_INACTIVE;
                       return -EINVAL;
                    }
                }
                else
                {
                    IRDBG_INFO("[RCA] u16Addr = %x not match u32HeadCode.\n",(u8Addr<<8 | u8AddrInv));
                }
            }
            break;
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

static struct ir_decoder_handler rca_handler = {
    .protocols  = (1<<IR_TYPE_RCA),
    .decode     = ir_rca_decode,
};

int rca_decode_init(void)
{
    if(u8InitFlag_rca == FALSE)
    {
		KeyTime = 0;
        memset(&rca,0,sizeof(IR_RCA_Spec_t));
        MIRC_Decoder_Register(&rca_handler);
        IR_PRINT("[IR Log] RCA Spec protocol init\n");
        u8InitFlag_rca = TRUE;
    }
    else
    {
        IR_PRINT("[IR Log] RCA Spec Protocol Has been Initialized\n");

    }
    return 0;
}

void rca_decode_exit(void)
{
    if(u8InitFlag_rca == TRUE)
    {
        MIRC_Decoder_UnRegister(&rca_handler);
        u8InitFlag_rca = FALSE;
    }
}

