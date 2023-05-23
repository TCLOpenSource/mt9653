// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author MediaTek <MediaTek@mediatek.com>
 */

#include <linux/bitrev.h>
#include <linux/module.h>
#include "../ir_core.h"
#include "../ir_common.h"

static u8 u8InitFlag_rc5_beko = FALSE;
static unsigned long  KeyTime;
static IR_Beko_RC5_Spec_t rc5_beko;

/* Beko RC5 Factory IR Mode. */
#define FACTORY_IR_CTRL 1


#if FACTORY_IR_CTRL
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define FACTORY_IR_RC5_DATA_BITS    77 // ( 3(start + toggle) + 5(system) + 8*4(Command data) ) * 2 - 3 = 77 bits, minus 3 is because we don't need to count the pulse of first start bits.

static u32 g_u32FactoryCmd = 0;
static bool g_bFactory_RC = FALSE;

static ssize_t factory_ir_ctrl_proc_read(struct file *file, char __user *buf,
        size_t count, loff_t *ppos)
{
    if(g_u32FactoryCmd!=0)
    {
	copy_to_user((void *)buf, &g_u32FactoryCmd, sizeof(g_u32FactoryCmd));
        g_u32FactoryCmd=0;
        return sizeof(u32);
    }
    return 0;
}

/* Beko RC5 Factory IR Mode. */
static ssize_t factory_ir_ctrl_proc_write(struct file *file, const char __user *buf,
        size_t count, loff_t *ppos)
{
    if (count == 2) {
        if (buf[0] == '0' && buf[1] == '\n') {
            printk("factory_cmd:%x\n",g_u32FactoryCmd);
        }
    }
    return count;
}

static const struct file_operations factory_ir_ctrl_proc_fops = {
    .read       = factory_ir_ctrl_proc_read,
    .write      = factory_ir_ctrl_proc_write,
};
#endif


static u32 databits_to_data(u64 data_bits,u8 bits,u8 reverse)
{
    u32 ret_data=0;
    u8 i = 0;
    u8 number = bits>>1;
    for(i=1;i<number+1;i++)
    {
        ret_data <<= 1;
        if(reverse)
        {
            if(((data_bits>>(bits-i*2))&0x03)==1)
                ret_data |=0x01;
        }
        else
        {
            if(((data_bits>>(bits-i*2))&0x03)==2)
                ret_data |=0x01;

        }
    }
    if(bits%2)
    {
        ret_data <<= 1;
        if(reverse)
        {
            if((data_bits&0x01) == 0)
                ret_data |=0x01;
        }
        else
        {
            if(data_bits&0x01)
                ret_data |=0x01;
        }
    }
    return ret_data;
}

/**
 * ir_beko_rc5_decode() - Decode one RCA pulse or space
 * @dev:	the struct rc_dev descriptor of the device
 * @duration:	the struct ir_raw_event descriptor of the pulse/space
 *
 * This function returns -EINVAL if the pulse violates the state machine
 */
static int ir_beko_rc5_decode(struct mtk_ir_dev *dev, struct ir_raw_data ev)
{
    IR_Beko_RC5_Spec_t *data = &rc5_beko;
    struct ir_scancode *sc = &dev->raw->this_sc;

    static u32 pre_scancode = 0;

    u8 i = 0;
    u8 u8Addr = 0;
    u8 u8Toggle =0;
    u8 u8Cmd =0;
    u32 u32Data;


    if (!(dev->enabled_protocols & (1<<IR_TYPE_BEKO_RC5)))
        return -EINVAL;

    switch (data->eStatus)
    {

    case STATE_INACTIVE:
        if (!ev.pulse)
            break;
        //match RC5 start bit1  1
        if (eq_margin(ev.duration, BEKO_RC5_BIT_MIN_LWB, BEKO_RC5_BIT_MIN_UPB))
        {
            data->u8BitCount = 0;
            data->u64DataBits = 0;
            data->eStatus = STATE_BIT_DATA;
#if FACTORY_IR_CTRL
            g_bFactory_RC = FALSE;
#endif
            return 0;
        }
        //match RC5 start bit1 with bit2  0_High
        else if(eq_margin(ev.duration, BEKO_RC5_BIT_MAX_LWB, BEKO_RC5_BIT_MAX_UPB))
        {
            data->u8BitCount = 1;
            data->u64DataBits |= 1;
            data->eStatus = STATE_BIT_DATA;
        }
        else
            break;

    case STATE_BIT_DATA:
        if (eq_margin(ev.duration, BEKO_RC5_BIT_MIN_LWB, BEKO_RC5_BIT_MIN_UPB))
        {
            data->u8BitCount ++;
            data->u64DataBits <<= 1;
            data->eStatus = STATE_BIT_DATA;
            if(!ev.pulse)
            {
                data->u64DataBits |= 1;
            }
        }
        else if(eq_margin(ev.duration, BEKO_RC5_BIT_MAX_LWB, BEKO_RC5_BIT_MAX_UPB))
        {
            data->u8BitCount += 2;
            data->u64DataBits <<= 2;
            data->eStatus = STATE_BIT_DATA;
            if(!ev.pulse)
            {
                data->u64DataBits |= 3;
            }
        }
        else
            break;

        /* Beko RC5 Factory IR Mode. */
#if FACTORY_IR_CTRL
        if(g_bFactory_RC == TRUE)
        {
            if(data->u8BitCount >= FACTORY_IR_RC5_DATA_BITS)
            {
                if(g_u32FactoryCmd == 0)
                {
                    u32Data = databits_to_data(data->u64DataBits, data->u8BitCount, 0);
                    g_u32FactoryCmd = u32Data;
                    IRDBG_INFO("Got Factory command data: %x\n",g_u32FactoryCmd);
                }
                g_bFactory_RC = FALSE;
                data->u8BitCount = 0;
                data->u64DataBits = 0;
                return 0;
            }
            else
            {
                return 0;
            }
        }
#endif

        if(data->u8BitCount >= (RC5_NBITS*2 -3))
        {
            u32Data = databits_to_data(data->u64DataBits,data->u8BitCount,0);

#if FACTORY_IR_CTRL
            if((u32Data & 0x1FC0) == 0x17C0) /* start(second '1' bit), toggle(1 bit) and system bit(5 bit). from Beko spec. */
            {
                IRDBG_INFO("Beko Factory RC5 Mode detected!!\n");
                g_bFactory_RC = TRUE;
                return 0;
            }
#endif

            u8Cmd    = (u32Data & 0x0003F) >> 0;
            u8Addr   = (u32Data & 0x007C0) >> 6;
            u8Toggle = (u32Data & 0x00800) ? 1 : 0;
            u8Cmd   += (u32Data & 0x01000) ? 0 : 0x40;

            IRDBG_INFO("[RC5] u8Addr = %x u8Cmd = %x u8Toggle = %x\n",u8Addr,u8Cmd,u8Toggle);
            IRDBG_INFO("[RC5] TIME =%ld\n",(MIRC_Get_System_Time()-KeyTime));
            KeyTime = MIRC_Get_System_Time();

            for (i= 0;i<dev->support_num;i++)
            {
                if((u8Addr== dev->support_ir[i].u32HeadCode)&&(dev->support_ir[i].eIRType == IR_TYPE_BEKO_RC5)&&(dev->support_ir[i].u8Enable == TRUE))
                {
                    u32 speed = dev->support_ir[i].u32IRSpeed;
                    if (((speed != 0)&&( data->u8RepeatTimes >= (speed - 1)))
                            || ((speed == 0)&&(data->u8RepeatTimes >= dev->speed)))
                    {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
                            sc->scancode = (u8Addr<<8)| u8Cmd;
                            sc->scancode_protocol = (1<<IR_TYPE_BEKO_RC5);
                            dev->map_num = dev->support_ir[i].u32HeadCode;

                            if((pre_scancode == sc->scancode) && ((MIRC_Get_System_Time()-KeyTime) < 225) )
                            dev->raw->u8RepeatFlag = 1;
                            else
                            dev->raw->u8RepeatFlag = 0;
#else
                            sc->scancode = (u8Cmd<<8) |0x00;
                            sc->scancode |= (0x01UL << 28);
                            sc->scancode_protocol = (1<<IR_TYPE_RCA);
#endif
                            memset(data,0,sizeof(IR_Beko_RC5_Spec_t));
                            pre_scancode = sc->scancode;
                            KeyTime = MIRC_Get_System_Time();
                            return 1;
                    }
                }
            }
            data->u8RepeatTimes ++;
            IRDBG_INFO("[RCA] repeattimes =%d \n",data->u8RepeatTimes);
            //memset(data,0,sizeof(IR_Beko_RC5_Spec_t));
            dev->raw->u8RepeatFlag = 0;
        }
        return 0;
    default:
        break;
    }

    data->eStatus = STATE_INACTIVE;
    return -EINVAL;
}

static struct ir_decoder_handler rc5_beko_handler = {
    .protocols	= (1<<IR_TYPE_BEKO_RC5),
    .decode		= ir_beko_rc5_decode,
};

int beko_rc5_decode_init(void)
{
    if(u8InitFlag_rc5_beko == FALSE)
    {
        memset(&rc5_beko,0,sizeof(IR_Beko_RC5_Spec_t));
        MIRC_Decoder_Register(&rc5_beko_handler);
        IR_PRINT("[IR Log] RC5 Spec protocol init\n");
        u8InitFlag_rc5_beko = TRUE;

        /* Beko RC5 Factory IR Mode. */
#if FACTORY_IR_CTRL
        proc_create("factory_ir_ctrl", 0666, NULL, &factory_ir_ctrl_proc_fops);
#endif
    }
    else
    {
        IR_PRINT("[IR Log] RC5 Spec Protocol Has been Initialized\n");

    }
    return 0;
}

void beko_rc5_decode_exit(void)
{
    if(u8InitFlag_rc5_beko == TRUE)
    {
        MIRC_Decoder_UnRegister(&rc5_beko_handler);
        u8InitFlag_rc5_beko = FALSE;
    }
}
