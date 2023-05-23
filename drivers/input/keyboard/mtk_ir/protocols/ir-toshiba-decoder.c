// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

#define REPEAT_CHECK_TIMEOUT    5 //5ms checkout repeat
static u8 u8InitFlag_toshiba = FALSE;
static u32 scancode = 0;
static u32 speed = 0;
static u32 mapnum = 0;
static unsigned long  KeyTime = 0;
static IR_Toshiba_Spec_t toshiba;

u32 ir_clsoe_wb_toshiba = 0;
EXPORT_SYMBOL(ir_clsoe_wb_toshiba);

struct ir_timer {
	struct timer_list timer;
	struct mtk_ir_dev *dev;
};
static struct ir_timer RepeatCheckTimer;
static bool RepeatCheckFlag = FALSE;

static void _toshiba_timer_proc(struct timer_list *t)
{
    struct ir_timer *RepeatCheckTimer = from_timer(RepeatCheckTimer, t, timer);
    struct mtk_ir_dev *dev = RepeatCheckTimer->dev;
    DEFINE_IR_RAW_DATA(ev);
    ev.duration = REPEAT_CHECK_TIMEOUT*1000;
    ev.pulse = 0;
    IRDBG_INFO("store repeat tigger data========\n");
    if (MIRC_Data_Store(dev, &ev) < 0)
    {
        IRDBG_ERR("Store IR data Error!\n");
    }
    MIRC_Data_Wakeup(dev);
    return;
}
/**
 * ir_nec_decode() - Decode one NEC pulse or space
 * @dev:	the struct rc_dev descriptor of the device
 * @duration:	the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int ir_toshiba_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
    IR_Toshiba_Spec_t *data = &toshiba;
    struct ir_scancode *sc = &dev->raw->this_sc;
    struct ir_scancode *prevsc = &dev->raw->prev_sc;
    u8 i = 0;
    u8 u8Addr = 0;
    u8 u8AddrInv =0;
    u8 u8Cmd =0;
    u8 u8CmdInv =0;

    if (!(dev->enabled_protocols & (1<<IR_TYPE_TOSHIBA)))
        return -EINVAL;
    switch (data->eStatus)
    {

    case STATE_INACTIVE:
        if (!ev.pulse)
            break;
        if (eq_margin(ev.duration,TOSHIBA_HEADER_PULSE_LWB,TOSHIBA_HEADER_PULSE_UPB))
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
        if (eq_margin(ev.duration,TOSHIBA_HEADER_SPACE_LWB,TOSHIBA_HEADER_SPACE_UPB))
        {
            data->eStatus = STATE_BIT_PULSE;
            return 0;
        }
        break;

    case STATE_BIT_PULSE:
        if (!ev.pulse)
            break;
        if (!eq_margin(ev.duration,TOSHIBA_BIT_PULSE_LWB,TOSHIBA_BIT_PULSE_UPB))
            break;
        //repeat flag
        if(data->u8BitCount == 1)
        {
            if((prevsc->scancode_protocol == (1<<IR_TYPE_TOSHIBA))&&(((u32)(MIRC_Get_System_Time()-KeyTime)) <= TOSHIBA_REPEAT_TIMEOUT))//repeat
            {
                IRDBG_INFO("[Toshiba Repeat Match] Repeat Time = %lu\n",(MIRC_Get_System_Time()-KeyTime));
                KeyTime = MIRC_Get_System_Time();
                //check repeat time spec going
                if(RepeatCheckFlag == FALSE)
                {
                    del_timer(&RepeatCheckTimer.timer);
                    RepeatCheckTimer.timer.expires = jiffies+ msecs_to_jiffies(REPEAT_CHECK_TIMEOUT);
                    RepeatCheckTimer.dev = dev;
                    add_timer(&RepeatCheckTimer.timer);
                    RepeatCheckFlag = TRUE;
                }
            }
        }
        data->eStatus = STATE_BIT_SPACE;
        return 0;

    case STATE_BIT_SPACE:
        if (ev.pulse)
            break;
        data->u32DataBits <<= 1;

        if (eq_margin(ev.duration,TOSHIBA_BIT_1_SPACE_LWB,TOSHIBA_BIT_1_SPACE_UPB))
            data->u32DataBits |= 1;
        else if (!eq_margin(ev.duration,TOSHIBA_BIT_0_SPACE_LWB,TOSHIBA_BIT_0_SPACE_UPB))
        {
            IRDBG_INFO("ev.duration = %d \n",ev.duration);
            if(ev.duration != REPEAT_CHECK_TIMEOUT*1000)
                break;
            if(RepeatCheckFlag == TRUE)
            {
                RepeatCheckFlag = FALSE;
                if (((speed != 0)&&( data->u8RepeatTimes >= (speed - 1)))
                        || ((speed == 0)&&(data->u8RepeatTimes >= dev->speed)))
                {
                    #ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
                    sc->scancode = scancode;
                    sc->scancode_protocol = (1<<IR_TYPE_TOSHIBA);
                    dev->map_num = mapnum;
                    dev->raw->u8RepeatFlag = 1;
                    #else
                    sc->scancode_protocol = (1<<IR_TYPE_TOSHIBA);
                    sc->scancode = scancode|0x01;//repeat
                    #endif
                    data->eStatus = STATE_INACTIVE;
                    data->u32DataBits = 0;
                    data->u8BitCount = 0;
                    return 1;
                }
                data->u8RepeatTimes ++;
                break;
            }
        }
        if(RepeatCheckFlag == TRUE)
        {
            RepeatCheckFlag = FALSE;
            del_timer(&RepeatCheckTimer.timer);
        }
        data->u8BitCount++;

        if (data->u8BitCount == TOSHIBA_NBITS)
        {
            u8Addr    = bitrev8((data->u32DataBits >> 24) & 0xff);
            u8AddrInv = bitrev8((data->u32DataBits >> 16) & 0xff);
            u8Cmd	  = bitrev8((data->u32DataBits >>  8) & 0xff);
            u8CmdInv  = bitrev8((data->u32DataBits >>  0) & 0xff);

            IRDBG_INFO("[Toshiba] u8Addr = %x u8AddrInv = %x u8Cmd = %x u8CmdInv = %x\n",u8Addr,u8AddrInv,u8Cmd,u8CmdInv);

            if ((0 != ir_clsoe_wb_toshiba) && (u8Cmd  == (ir_clsoe_wb_toshiba & 0xff)))
            {
                MIRC_Set_WhiteBalance_Enable(false);
                IRDBG_INFO("[Toshiba] u8Cmd = %x, MIRC_Set_WhiteBalance_Enable(false);\n", u8Cmd);
            }

            for (i= 0;i<dev->support_num;i++)
            {
                if(dev->support_ir[i].eIRType == IR_TYPE_TOSHIBA)
                {
                    if((((u8Addr<<8) | u8AddrInv) == dev->support_ir[i].u32HeadCode)&&(dev->support_ir[i].u8Enable == TRUE))
                    {
                        if (u8Cmd == (u8)~u8CmdInv)
                        {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
                            sc->scancode = (u8Addr<<16) | (u8AddrInv<<8) | u8Cmd;
                            sc->scancode_protocol = (1<<IR_TYPE_TOSHIBA);
                            scancode = sc->scancode;
                            speed = dev->support_ir[i].u32IRSpeed;
                            dev->map_num = dev->support_ir[i].u32HeadCode;
                            mapnum = dev->map_num;
                            dev->raw->u8RepeatFlag = 0;
#else
                            sc->scancode = (u8Cmd<<8) |0x00;
                            sc->scancode |= (0x01UL << 28);
                            sc->scancode_protocol = (1<<IR_TYPE_TOSHIBA);
                            scancode = sc->scancode;
                            speed = dev->support_ir[i].u32IRSpeed;
#endif
                            KeyTime = MIRC_Get_System_Time();
                            data->eStatus = STATE_INACTIVE;
                            data->u32DataBits = 0;
                            data->u8BitCount = 0;
                            data->u8RepeatTimes = 0;
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

static struct ir_decoder_handler toshiba_handler = {
    .protocols	= (1<<IR_TYPE_TOSHIBA),
    .decode		= ir_toshiba_decode,
};

int toshiba_decode_init(void)
{
    if(u8InitFlag_toshiba == FALSE)
    {
        scancode = 0;
        mapnum = 0;
        KeyTime = 0;
        memset(&toshiba,0,sizeof(IR_Toshiba_Spec_t));
        MIRC_Decoder_Register(&toshiba_handler);
        IR_PRINT("[IR Log] Toshiba Spec Protocol Init\n");
        u8InitFlag_toshiba = TRUE;
    	timer_setup(&RepeatCheckTimer.timer, _toshiba_timer_proc, 0);
    	RepeatCheckTimer.timer.expires = jiffies+ msecs_to_jiffies(REPEAT_CHECK_TIMEOUT);
    }
    else
    {
        IR_PRINT("[IR Log] Toshiba Spec Protocol Has been Initialized\n");
    }
    return 0;
}

void toshiba_decode_exit(void)
{
    if(u8InitFlag_toshiba == TRUE)
    {
        MIRC_Decoder_UnRegister(&toshiba_handler);
        u8InitFlag_toshiba = FALSE;
        del_timer(&RepeatCheckTimer.timer);
    }
}
