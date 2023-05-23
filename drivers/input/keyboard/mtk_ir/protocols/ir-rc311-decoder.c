// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

static u8 u8InitFlag_rc311 = FALSE;
static u32 scancode = 0;
static u32 speed = 0;
static u32 mapnum = 0;
static unsigned long  KeyTime = 0;
static IR_RC311_Spec_t rc311;
static u32 prev_key;
/**
 * ir_rc311_decode() - Decode one RC311 pulse or space
 * @dev:    the struct rc_dev descriptor of the device
 * @duration:   the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int ir_rc311_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
    IR_RC311_Spec_t *data = &rc311;
    struct ir_scancode *sc = &dev->raw->this_sc;
    struct ir_scancode *prevsc = &dev->raw->prev_sc;
    u8 i = 0;
    u8 u8Custom1 = 0;
    u8 u8Custom2 =0;
    u8 u8KeyData =0;
    u8 u8KeyDataInv =0;

    if (!(dev->enabled_protocols & (1<<IR_TYPE_RC311)))
        return -EINVAL;
    switch (data->eStatus)
    {

    case STATE_INACTIVE:
        if (!ev.pulse)
            break;
        if (eq_margin(ev.duration,RC311_HEADER_PULSE_LWB,RC311_HEADER_PULSE_UPB))
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
        if (eq_margin(ev.duration,RC311_HEADER_SPACE_LWB,RC311_HEADER_SPACE_UPB))
        {
            data->eStatus = STATE_BIT_PULSE;
            return 0;
        }
        break;

    case STATE_BIT_PULSE:
        if (!ev.pulse)
            break;
        if (!eq_margin(ev.duration,RC311_BIT_PULSE_LWB,RC311_BIT_PULSE_UPB))
            break;

        data->eStatus = STATE_BIT_SPACE;
        return 0;

    case STATE_BIT_SPACE:
        if (ev.pulse)
            break;
        data->u32DataBits >>= 1;

        if (eq_margin(ev.duration,RC311_BIT_1_SPACE_LWB,RC311_BIT_1_SPACE_UPB))
            data->u32DataBits |= 0x800000;
        else if (!eq_margin(ev.duration,RC311_BIT_0_SPACE_LWB,RC311_BIT_0_SPACE_UPB))
            break;
        data->u8BitCount++;

        if (data->u8BitCount == RC311_NBITS)
        {
            u8KeyDataInv  = (data->u32DataBits >> 18) & 0x3f;
            u8Custom2     = (data->u32DataBits >> 12) & 0x3f;
            u8KeyData = (data->u32DataBits >>  6) & 0x3f;
            u8Custom1    = (data->u32DataBits >>  0) & 0x3f;
            IRDBG_INFO("data->u32DataBits = 0x%x\n",data->u32DataBits);
            IRDBG_INFO("[RC311] u8Custom1 = 0x%x u8KeyData = 0x%x u8Custom2 = 0x%x u8KeyDataInv = 0x%x\n",u8Custom1,u8KeyData,u8Custom2,u8KeyDataInv);
            for (i= 0;i<dev->support_num;i++)
            {
                if(dev->support_ir[i].eIRType == IR_TYPE_RC311)
                {
                    if((((u8Custom2<<8) | u8Custom1) == dev->support_ir[i].u32HeadCode)&&(dev->support_ir[i].u8Enable == TRUE))
                    {
                        if (u8KeyData == (((u8)~u8KeyDataInv)&0x3f))
                        {
                            dev->raw->u8RepeatFlag = 0;
                            IRDBG_INFO("[RC311] repeat time interval =%ld\n",(MIRC_Get_System_Time()-KeyTime));
                            if((MIRC_Get_System_Time()-KeyTime) <= RC311_REPEAT_TIMEOUT)
                            {
                                if(prev_key == data->u32DataBits)
                                {
                                    if (((speed != 0)&&( data->u8RepeatTimes >= (speed - 1)))
                                        || ((speed == 0)&&(data->u8RepeatTimes >= dev->speed)))
                                    {
                                        dev->raw->u8RepeatFlag = 1;
                                    }
                                    else
                                    {
                                        data->u8RepeatTimes ++;
                                        KeyTime = MIRC_Get_System_Time();
                                        IRDBG_INFO("[RC311] IR cycle filter\n");
                                        data->eStatus = STATE_INACTIVE;
                                        data->u32DataBits = 0;
                                        data->u8BitCount = 0;
                                        return 0;
                                    }
                                }
                            }
                            prev_key = data->u32DataBits;
                            #ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
                            sc->scancode = (u8Custom2<<16) | (u8Custom1<<8) | u8KeyData;
                            sc->scancode_protocol = (1<<IR_TYPE_RC311);
                            dev->map_num = dev->support_ir[i].u32HeadCode;
                            #else
                            sc->scancode = (u8KeyData<<8) | dev->raw->u8RepeatFlag;
                            sc->scancode |= (0x01UL << 28);
                            sc->scancode_protocol = (1<<IR_TYPE_RC311);
                            #endif
                            speed = dev->support_ir[i].u32IRSpeed;
                            KeyTime = MIRC_Get_System_Time();
                            data->eStatus = STATE_INACTIVE;
                            data->u32DataBits = 0;
                            data->u8BitCount = 0;
                            if(dev->raw->u8RepeatFlag == 0)
                            {
                                data->u8RepeatTimes = 0;
                            }
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

static struct ir_decoder_handler rc311_handler = {
    .protocols = (1<<IR_TYPE_RC311),
    .decode    = ir_rc311_decode,
};

int rc311_decode_init(void)
{
    if(u8InitFlag_rc311 == FALSE)
    {
        KeyTime = 0;
        memset(&rc311,0,sizeof(IR_RC311_Spec_t));
        MIRC_Decoder_Register(&rc311_handler);
        IRDBG_INFO("[IR Log] RC311 Spec Protocol Init\n");
        u8InitFlag_rc311 = TRUE;
    }
    else
    {
        IRDBG_INFO("[IR Log] RC311 Spec Protocol Has been Initialized\n");
    }
    return 0;
}

void rc311_decode_exit(void)
{
    if(u8InitFlag_rc311 == TRUE)
    {
        MIRC_Decoder_UnRegister(&rc311_handler);
        u8InitFlag_rc311 = FALSE;
    }
}
