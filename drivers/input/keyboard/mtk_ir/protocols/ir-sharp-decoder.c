// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"
#include <linux/timer.h>
#include <linux/version.h>


#define IR_SWDECODE_MODE_BUF_LEN    100
//#define SHSARP_REPEAT_TIMEOUT 170UL
#define TWO_KEY_TIMEOUT    300UL // ms clear slot

/* key status front or back key */
/* *
 *  00(0): we have no key
 *  01(1): we have back key
 *  10(2): we have front key
 *  11(3): we have both key
 * */
#define TWO_KEY_STATUS_0 0
#define TWO_KEY_STATUS_1 1
#define TWO_KEY_STATUS_2 2
#define TWO_KEY_STATUS_3 3

struct sharp_ir_key_t{
    u32 syscode, databit, judgebit, checkbit, map_num;
};

static u8 u8InitFlag_sharp = FALSE;
static IR_sharp_Spec_t sharp;
static u8 previous_pulse = 0;
static u32 previous_duration = 0;
static u8 get_key_status = TWO_KEY_STATUS_0;
static struct sharp_ir_key_t front_key = {0}, back_key = {0};
static struct timer_list ClearTwoKeyTimer;
static bool TwoKeyTimerFlag = FALSE;
static unsigned long  pre_KeyTime = 0;
static unsigned long  cur_KeyTime = 0;
static u32 pre_keycode = 0;
static u32 cur_keycode = 0;
#define IR_SHARP_REPEAT_TIMEOUT 170

u8 reverse_8bit(u8 x)
{
    x = (((x & 0xaa) >> 1) | ((x & 0x55) << 1));
    x = (((x & 0xcc) >> 2) | ((x & 0x33) << 2));
    return((x >> 4) | (x << 4));
}

u16 reverse_16bit(u16 x)
{
    x = (((x & 0xaaaa) >> 1) | ((x & 0x5555) << 1));
    x = (((x & 0xcccc) >> 2) | ((x & 0x3333) << 2));
    x = (((x & 0xf0f0) >> 4) | ((x & 0x0f0f) << 4));
    return((x >> 8) | (x << 8));
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,19,0)
static void _sharp_timer_proc(struct timer_list *t)
#else
static void _sharp_timer_proc(void)
#endif
{

    previous_pulse = 0;
    previous_duration = 0;
    pre_keycode = 0;
    pre_KeyTime = 0;

    get_key_status = TWO_KEY_STATUS_0;
    memset(&front_key, 0, sizeof(front_key));
    memset(&back_key, 0, sizeof(back_key));
    IRDBG_INFO("[SHARP] Timeout, clean up front and back key. \n");
    return;
}

/**
 * ir_sharp_decode() - Decode one SHARP pulse or space
 * @dev:	the struct rc_dev descriptor of the device
 * @duration:	the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int ir_sharp_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
    IR_sharp_Spec_t *data = &sharp;
    struct ir_scancode *sc = &dev->raw->this_sc;
    struct ir_scancode *prevsc = &dev->raw->prev_sc;
    struct sharp_ir_key_t current_key = {0};
    u32 intervalT = 0;
    int i = 0;

    if (!(dev->enabled_protocols & (1<<IR_TYPE_SHARP)))
        return -EINVAL;
    switch (data->eStatus)
    {
        case STATE_INACTIVE:
            if( ev.duration < SHARP_BIT_1_HIGH )
            {
                IRDBG_INFO("[SHARP] INACTIVE, duration in range\n");
                /* Get legal duration, Save current pulse info */
                previous_pulse = ev.pulse;
                previous_duration = ev.duration;
                data->u8BitCount = 0;
                data->u32DataBits = 0;
                data->eStatus = STATE_BIT_DATA;
                return 0;
            }
            else if(ev.duration >= SHARP_BIT_1_HIGH)
            {
                IRDBG_INFO("[SHARP] INACTIVE, duration NOT in range(%d)\n", ev.duration);
                return -EINVAL;
            }
            break;
        case STATE_BIT_DATA:
            /* Collect Intervel and check bit.
             *   1. T = Th + Tl
             *   2. Judge T to logic 1/0
             *   3. parse bit data
             *      1) paese system code as header
             *      2) parse data code
             *      3) parse last 2 bit
             */
            intervalT = 0;
            if( previous_pulse == ev.pulse )
            {
                /* Wrong! pulse sequence should be 10101... */
                IRDBG_INFO("[SHARP] BIT_DATA, pulse wrong! %d->%d\n", previous_pulse, ev.pulse); // IRDBG_ERR
                goto bit_data_reset;
            }

            if( ev.pulse == 1 && (ev.duration < SHARP_BIT_1_HIGH) )
            {
                /* Get Th here, update current previous pulse info */
                previous_pulse = ev.pulse;
                previous_duration = ev.duration;
                return 0;
            }
            else if( ev.pulse == 0 && ((ev.duration+previous_duration) >= SHARP_BIT_0_LOW) && ((ev.duration+previous_duration) < SHARP_BIT_1_HIGH) )
            {
                /* We got both Th and Tl here */
                intervalT = previous_duration+ ev.duration;

                /* Update current previous pulse info */
                previous_pulse = ev.pulse;
                previous_duration = ev.duration;
            }
            else
            {
                /* The duration is incorrect, reset */
                IRDBG_INFO("[SHARP] BIT_DATA, duration not in range(%d),pre(%d)\n", ev.duration, previous_duration);
                goto bit_data_reset;
            }

            /* Transfer pulse to logic 1/0 */
            if( intervalT >= SHARP_BIT_0_LOW && intervalT < SHARP_BIT_0_HIGH)
            {
                data->u8BitCount++;
                data->u32DataBits <<= 1;
            }
            else if( intervalT >= SHARP_BIT_1_LOW && intervalT < SHARP_BIT_1_HIGH)
            {
                data->u8BitCount++;
                data->u32DataBits <<= 1;
                data->u32DataBits |= 1;
            }
            else
            {
                IRDBG_INFO("[SHARP] BIT_DATA, interval not in range!\n");
                goto bit_data_reset;
            }

            /* We parse code while we got 15 bit datas */
            if(data->u8BitCount == SHARP_NBITS)
            {
                //IRDBG_INFO("[SHARP] get 15 bit data->u32DataBits = %x\n",data->u32DataBits);
                /* Separate system code(head), data code, check bit and judge bit */
                current_key.syscode = (data->u32DataBits & 0x7c00) >> 10;
                current_key.databit = (data->u32DataBits & 0x03fc) >> 2;
                current_key.checkbit = (data->u32DataBits & 0x0002) >> 1;
                current_key.judgebit = data->u32DataBits & 0x0001;
                IRDBG_INFO("[SHARP] Get Keycode: %x_%x_%x_%x\n",current_key.syscode, current_key.databit, current_key.checkbit, current_key.judgebit);
            }
            else
            {
                /* TODO:
                 * In sharp IR protocol, it mentioned key off time,
                 * We can add timer here to decide clear data or not.
                 */
                return 0;
            }

            /*
             * simple check key
             * 1. check the check bit and judge bit first
             * 2. check system code
             */
            /* Check is system code(header) correct */
            for (i= 0;i<dev->support_num;i++)
            {
                if((current_key.syscode == dev->support_ir[i].u32HeadCode) && (dev->support_ir[i].eIRType == IR_TYPE_SHARP))
                {
                    current_key.map_num = dev->support_ir[i].u32HeadCode;
                }
            }
            if(current_key.map_num == 0)
            {
                IRDBG_INFO("[SHARP] system code fail:[%x]\n",current_key.syscode);
                goto bit_data_reset;
            }

            /* check key is front or back key */
            /* get_key_status:
             *  00(0): we have no key
             *  01(1): we have back key
             *  10(2): we have front key
             *  11(3): we have both key
             * */
            if(get_key_status != TWO_KEY_STATUS_3)
            {
                if(current_key.checkbit == 1 && current_key.judgebit== 0)
                {
                    IRDBG_INFO("[SHARP] get Front key\n");
                    get_key_status |= 0x2;
                    front_key = current_key;
                }
                else if(current_key.checkbit == 0 && current_key.judgebit == 1)
                {
                    IRDBG_INFO("[SHARP] get Back key\n");
                    get_key_status |= 0x1;
                    back_key = current_key;
                    cur_keycode = current_key.databit;
                    cur_KeyTime = MIRC_Get_System_Time();
                }
                else
                {
                    IRDBG_INFO("[SHARP] get key judge/check bit fail:[%x][%x]\n",current_key.checkbit, current_key.judgebit);
                    goto bit_data_reset;
                }
            }

            /* Checking data */
            IRDBG_INFO("[SHARP] get_key_status=[%d]\n", get_key_status);
            if(get_key_status == TWO_KEY_STATUS_3)
            {
                get_key_status = TWO_KEY_STATUS_0;
                /* Delete timer because the slot will become empty. */
                if(TwoKeyTimerFlag == TRUE)
                {
                    IRDBG_INFO("[SHARP] delete timer. \n");
                    del_timer(&ClearTwoKeyTimer);
                    TwoKeyTimerFlag = FALSE;
                }
                IRDBG_INFO("[SHARP] get both key, [%x]:[%x]\n", front_key.databit, ~(back_key.databit)&0x000000ff);
                /* we get both d and d', we can check data now */
                if( !((front_key.databit&0x000000ff)^((~back_key.databit)&0x000000ff)) )
                {
                    IRDBG_INFO("[SHARP] cur_keycode 0x%lx, pre_keycode 0x%lx, KeyTime diff 0x%lx \n",
                        cur_keycode, pre_keycode, (cur_KeyTime-pre_KeyTime));
                    if((cur_keycode == pre_keycode) && ((cur_KeyTime-pre_KeyTime) < IR_SHARP_REPEAT_TIMEOUT)) // SHSARP_REPEAT_TIMEOUT
                    {
                        IRDBG_INFO("[SHARP] u8RepeatFlag set 1\n");
                        dev->raw->u8RepeatFlag = 1;
                    }
                    else
                    {
                        IRDBG_INFO("[SHARP] u8RepeatFlag set 0\n");
                        dev->raw->u8RepeatFlag = 0;
                    }
                    pre_keycode = cur_keycode;
                    pre_KeyTime = cur_KeyTime;

                    IRDBG_INFO("[SHARP] two key match! send [%x] key! \n", reverse_8bit(front_key.databit));
                    sc->scancode = reverse_8bit(front_key.databit);
                    sc->scancode_protocol = (1<<IR_TYPE_SHARP);
                    dev->map_num = front_key.map_num;

                    data->u8BitCount = 0;
                    data->u32DataBits = 0;
                    previous_pulse = 0;
                    previous_duration = 0;
                    data->eStatus = STATE_INACTIVE;
                    memset(&front_key, 0, sizeof(front_key));
                    memset(&back_key, 0, sizeof(back_key));
                    return 1;
                }
                else
                {
                    IRDBG_INFO("[SHARP] two key NOT match! [%x]:[%x]\n", front_key.databit, back_key.databit);
                }
                memset(&front_key, 0, sizeof(front_key));
                memset(&back_key, 0, sizeof(back_key));
            }else if(get_key_status == TWO_KEY_STATUS_1 || get_key_status == TWO_KEY_STATUS_2){
                /* set timer to clear the slot */
                if(TwoKeyTimerFlag == FALSE)
                {
                    IRDBG_INFO("[SHARP] Add timer. \n");
                    del_timer(&ClearTwoKeyTimer);
                    ClearTwoKeyTimer.expires =jiffies+ msecs_to_jiffies(TWO_KEY_TIMEOUT);
                    add_timer(&ClearTwoKeyTimer);
                    TwoKeyTimerFlag = TRUE;
                }
            }
            /* Decode Done */
            //IRDBG_INFO("[SHARP] decode finish, curren status, get_key_status=%d\n",get_key_status);
        default:
            data->eStatus = STATE_INACTIVE;
            return -EINVAL;
    }
bit_data_reset:
    data->u8BitCount = 0;
    data->u32DataBits = 0;
    previous_pulse = 0;
    previous_duration = 0;
    data->eStatus = STATE_INACTIVE;
    return 0;
}

static struct ir_decoder_handler sharp_handler = {
    .protocols	= (1<<IR_TYPE_SHARP),
    .decode		= ir_sharp_decode,
};

int sharp_decode_init(void)
{
    if(u8InitFlag_sharp == FALSE)
    {
        memset(&sharp,0,sizeof(IR_sharp_Spec_t));
        MIRC_Decoder_Register(&sharp_handler);
        IR_PRINT("[IR Log] Sharp Spec Protocol Init\n");
        u8InitFlag_sharp = TRUE;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,19,0)
        timer_setup(&ClearTwoKeyTimer, _sharp_timer_proc, 0);
        ClearTwoKeyTimer.expires =jiffies+ msecs_to_jiffies(TWO_KEY_TIMEOUT);
#else
        init_timer(&ClearTwoKeyTimer);
        ClearTwoKeyTimer.function = _sharp_timer_proc;
        ClearTwoKeyTimer.expires =jiffies+ msecs_to_jiffies(TWO_KEY_TIMEOUT);
        ClearTwoKeyTimer.data = ((unsigned long) 0);
#endif
    }
    else
    {
        IR_PRINT("[IR Log] Sharp Spec Protocol Has been Initialized\n");
    }
    return 0;
}

void sharp_decode_exit(void)
{
    if(u8InitFlag_sharp == TRUE)
    {
        MIRC_Decoder_UnRegister(&sharp_handler);
        u8InitFlag_sharp = FALSE;
        del_timer(&ClearTwoKeyTimer);
    }
}
